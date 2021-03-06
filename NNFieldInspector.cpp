/*=========================================================================
 *
 *  Copyright David Doria 2012 daviddoria@gmail.com
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "NNFieldInspector.h"

// STL
#include <stdexcept>

// ITK
#include "itkCastImageFilter.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkVector.h"

// Qt
#include <QFileDialog>
#include <QTextEdit> // for help
#include <QDropEvent>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>

// VTK
#include <vtkCommand.h>
#include <vtkImageData.h>
#include <vtkImageSlice.h>
#include <vtkInteractorStyleImage.h>
#include <vtkPointPicker.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkImageSliceMapper.h>

// Submodules
#include "ITKVTKHelpers/ITKVTKHelpers.h"
#include "ITKVTKCamera/ITKVTKCamera.h"
#include "VTKHelpers/VTKHelpers.h"

// Custom
#include "PointSelectionStyle2D.h"

void NNFieldInspector::on_actionHelp_activated()
{
  QTextEdit* help=new QTextEdit();

  help->setReadOnly(true);
  help->append("<h1>Nearest Neighbor Field Inspector</h1>\
  Click on a pixel. The surrounding region will be outlined,\
  and the best matching region will be outlined.<br/>"
  );
  help->show();
}

void NNFieldInspector::on_actionQuit_activated()
{
  QApplication::exit();
}

void NNFieldInspector::SharedConstructor()
{
  this->setupUi(this);
  this->setAcceptDrops(true);

  this->Image = NULL;
  this->NNField = NULL;

  this->LastPick[0] = -1;
  this->LastPick[1] = -1;

  this->Interpretation = ABSOLUTE;

  // Turn slices visibility off to prevent errors that there is not yet data.
  this->ImageLayer.ImageSlice->VisibilityOff();
  this->NNFieldMagnitudeLayer.ImageSlice->VisibilityOff();
  this->NNFieldXLayer.ImageSlice->VisibilityOff();
  this->NNFieldYLayer.ImageSlice->VisibilityOff();
  this->PickLayer.ImageSlice->VisibilityOff();

  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  this->qvtkWidget->GetRenderWindow()->AddRenderer(this->Renderer);

  // Add slices to renderer
  this->Renderer->AddViewProp(this->ImageLayer.ImageSlice);
  this->Renderer->AddViewProp(this->NNFieldMagnitudeLayer.ImageSlice);
  this->Renderer->AddViewProp(this->NNFieldXLayer.ImageSlice);
  this->Renderer->AddViewProp(this->NNFieldYLayer.ImageSlice);
  this->Renderer->AddViewProp(this->PickLayer.ImageSlice);

  this->NNField = NNFieldImageType::New();
  this->Image = ImageType::New();

  vtkSmartPointer<vtkPointPicker> pointPicker = vtkSmartPointer<vtkPointPicker>::New();
  this->qvtkWidget->GetRenderWindow()->GetInteractor()->SetPicker(pointPicker);

  this->SelectionStyle = PointSelectionStyle2D::New();
  this->SelectionStyle->SetCurrentRenderer(this->Renderer);
  this->qvtkWidget->GetRenderWindow()->GetInteractor()->SetInteractorStyle(this->SelectionStyle);

  /** When the image is clicked, alert the GUI. */
  this->SelectionStyle->AddObserver(PointSelectionStyle2D::PixelClickedEvent, this,
                                    &NNFieldInspector::PixelClickedEventHandler);

  this->Camera.SetRenderer(this->Renderer);
  this->Camera.SetRenderWindow(this->qvtkWidget->GetRenderWindow());
  this->Camera.SetInteractorStyle(this->SelectionStyle);

  this->qvtkWidget->GetInteractor()->AddObserver(vtkCommand::KeyPressEvent, this, &NNFieldInspector::KeypressCallbackFunction);
}

NNFieldInspector::NNFieldInspector(const std::string& imageFileName,
                                   const std::string& nnFieldFileName) : PatchRadius(7)
{
  SharedConstructor();
  this->ImageFileName = imageFileName;
  this->NNFieldFileName = nnFieldFileName;
}

// Constructor
NNFieldInspector::NNFieldInspector() : PatchRadius(7)
{
  SharedConstructor();
};

void NNFieldInspector::LoadNNField(const std::string& fileName)
{
  typedef itk::ImageFileReader<NNFieldImageType> NNFieldReaderType;
  NNFieldReaderType::Pointer nnFieldReader = NNFieldReaderType::New();
  nnFieldReader->SetFileName(fileName);
  nnFieldReader->Update();

  ITKHelpers::DeepCopy(nnFieldReader->GetOutput(), this->NNField.GetPointer());

  // Extract the first two channels
  {
  std::vector<unsigned int> channels;
  channels.push_back(0);
  channels.push_back(1);

  typedef itk::VectorImage<int, 2> VectorImageType;
  VectorImageType::Pointer vectorImage = VectorImageType::New();
  vectorImage->SetNumberOfComponentsPerPixel(2);
  vectorImage->SetRegions(this->NNField->GetLargestPossibleRegion());
  vectorImage->Allocate();

  ITKHelpers::ExtractChannels(this->NNField.GetPointer(), channels, vectorImage.GetPointer());
  ITKVTKHelpers::ITKImageToVTKMagnitudeImage(vectorImage.GetPointer(), this->NNFieldMagnitudeLayer.ImageData);
  }

  ITKVTKHelpers::ITKImageChannelToVTKImage(this->NNField.GetPointer(), 0, this->NNFieldXLayer.ImageData);

  ITKVTKHelpers::ITKImageChannelToVTKImage(this->NNField.GetPointer(), 1, this->NNFieldYLayer.ImageData);

  UpdateDisplayedImages();

  this->Renderer->ResetCamera();

  Refresh();
}

void NNFieldInspector::Refresh()
{
  this->qvtkWidget->GetRenderWindow()->Render();
}

void NNFieldInspector::LoadImage(const std::string& fileName)
{
  typedef itk::ImageFileReader<ImageType> ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(fileName);
  reader->Update();

  ITKHelpers::DeepCopy(reader->GetOutput(), this->Image.GetPointer());

  ITKVTKHelpers::ITKImageToVTKRGBImage(this->Image.GetPointer(), this->ImageLayer.ImageData);

  UpdateDisplayedImages();

  this->Renderer->ResetCamera();

  this->qvtkWidget->GetRenderWindow()->Render();
}

void NNFieldInspector::on_actionFlipHorizontally_activated()
{
  this->Camera.FlipHorizontally();
}

void NNFieldInspector::on_actionFlipVertically_activated()
{
  this->Camera.FlipVertically();
}

void NNFieldInspector::on_actionOpenImage_activated()
{
  // Get a filename to open
  QString fileName = QFileDialog::getOpenFileName(this, "Open File", ".",
                                                  "Image Files (*.jpg *.jpeg *.bmp *.png)");

  std::cout << "Got filename: " << fileName.toStdString() << std::endl;
  if(fileName.toStdString().empty())
    {
    std::cout << "Filename was empty." << std::endl;
    return;
    }

  LoadImage(fileName.toStdString());

  this->Camera.SetCameraPositionPNG();
}


void NNFieldInspector::on_actionOpenNNField_activated()
{
  // Get a filename to open
  QString fileName = QFileDialog::getOpenFileName(this, "Open File", ".",
                                                  "Image Files (*.mha)");

  std::cout << "Got filename: " << fileName.toStdString() << std::endl;
  if(fileName.toStdString().empty())
    {
    std::cout << "Filename was empty." << std::endl;
    return;
    }

  LoadNNField(fileName.toStdString());
}

void NNFieldInspector::PixelClickedEventHandler(vtkObject* caller, long unsigned int eventId,
                                                void* callData)
{
  if(!this->Image)
  {
    std::cerr << "Image must be set before clicking!" << std::endl;
    return;
  }

  if(!this->NNField)
  {
    std::cerr << "NNField must be set before clicking!" << std::endl;
    return;
  }

  double* pixel = reinterpret_cast<double*>(callData);

  //std::cout << "Picked " << pixel[0] << " " << pixel[1] << std::endl;

  itk::Index<2> pickedIndex = {{static_cast<unsigned int>(pixel[0]), static_cast<unsigned int>(pixel[1])}};

  // Store the pick
  this->LastPick[0] = pickedIndex[0];
  this->LastPick[1] = pickedIndex[1];

  std::cout << "Picked index: " << pickedIndex << std::endl;

  std::stringstream ssSelected;
  ssSelected << pickedIndex;
  this->lblSelected->setText(ssSelected.str().c_str());

  itk::ImageRegion<2> pickedRegion = ITKHelpers::GetRegionInRadiusAroundPixel(pickedIndex, this->PatchRadius);

  if(!this->Image->GetLargestPossibleRegion().IsInside(pickedRegion))
  {
    std::cout << "Picked patch that is not entirely inside image!" << std::endl;
    return;
  }

  NNFieldImageType::PixelType nnFieldPixel = this->NNField->GetPixel(pickedIndex);

  if(this->Interpretation == OFFSET)
  {
    this->BestMatchCenter = {{static_cast<unsigned int>(nnFieldPixel[0]) + pickedIndex[0],
                        static_cast<unsigned int>(nnFieldPixel[1]) + pickedIndex[1]}};
  }
  else if(this->Interpretation == ABSOLUTE)
  {
    this->BestMatchCenter = {{static_cast<unsigned int>(nnFieldPixel[0]),
                        static_cast<unsigned int>(nnFieldPixel[1])}};
  }
  else
  {
    throw std::runtime_error("Invalid Interpretation value set!");
  }

  itk::ImageRegion<2> matchRegion =
        ITKHelpers::GetRegionInRadiusAroundPixel(this->BestMatchCenter, this->PatchRadius);
  std::cout << "Best match center: " << this->BestMatchCenter << std::endl;

  std::stringstream ssBestMatch;
  ssBestMatch << this->BestMatchCenter;
  this->lblNN->setText(ssBestMatch.str().c_str());

  // Highlight patches
  ImageType::PixelType red;
  red[0] = 255; red[1] = 0; red[2] = 0;

  ImageType::PixelType green;
  green[0] = 0; green[1] = 255; green[2] = 0;

  ImageType::Pointer tempImage = ImageType::New();
  tempImage->SetRegions(this->Image->GetLargestPossibleRegion());
  tempImage->Allocate();
  tempImage->FillBuffer(itk::NumericTraits<ImageType::PixelType>::ZeroValue());

  ITKHelpers::OutlineRegion(tempImage.GetPointer(), pickedRegion, red);
  tempImage->SetPixel(ITKHelpers::GetRegionCenter(pickedRegion), red);

  ITKHelpers::OutlineRegion(tempImage.GetPointer(), matchRegion, green);
  tempImage->SetPixel(ITKHelpers::GetRegionCenter(matchRegion), green);

  typedef itk::Image<float, 2> FloatImageType;
  FloatImageType::Pointer magnitudeImage = FloatImageType::New();
  ITKHelpers::MagnitudeImage(tempImage.GetPointer(), magnitudeImage.GetPointer());

  // 4 for RGBA
  ITKVTKHelpers::InitializeVTKImage(this->Image->GetLargestPossibleRegion(), 4, this->PickLayer.ImageData);
  VTKHelpers::MakeImageTransparent(this->PickLayer.ImageData);
  std::vector<itk::Index<2> > nonZeroPixels = ITKHelpers::GetNonZeroPixels(magnitudeImage.GetPointer());
  ITKVTKHelpers::SetPixelTransparency(this->PickLayer.ImageData, nonZeroPixels, VTKHelpers::OPAQUE_PIXEL);

  // 'true' means 'already initialized'
  ITKVTKHelpers::ITKImageToVTKRGBImage(tempImage.GetPointer(), this->PickLayer.ImageData, true);
  this->PickLayer.ImageSlice->VisibilityOn();

  Refresh();
}

void NNFieldInspector::SetPatchRadius(const unsigned int patchRadius)
{
  this->PatchRadius = patchRadius;
}

void NNFieldInspector::on_radRGB_clicked()
{
  UpdateDisplayedImages();
}

void NNFieldInspector::on_radNNFieldMagnitude_clicked()
{
  UpdateDisplayedImages();
}

void NNFieldInspector::on_radNNFieldX_clicked()
{
  UpdateDisplayedImages();
}

void NNFieldInspector::on_radNNFieldY_clicked()
{
  UpdateDisplayedImages();
}

void NNFieldInspector::UpdateDisplayedImages()
{
  this->NNFieldMagnitudeLayer.ImageSlice->SetVisibility(this->radNNFieldMagnitude->isChecked());
  this->NNFieldXLayer.ImageSlice->SetVisibility(this->radNNFieldX->isChecked());
  this->NNFieldYLayer.ImageSlice->SetVisibility(this->radNNFieldY->isChecked());
  this->ImageLayer.ImageSlice->SetVisibility(this->radRGB->isChecked());
  this->qvtkWidget->GetRenderWindow()->Render();
}

void NNFieldInspector::on_actionInterpretAsOffsetField_activated()
{
  this->Interpretation = OFFSET;
}

void NNFieldInspector::on_actionInterpretAsAbsoluteField_activated()
{
  this->Interpretation = ABSOLUTE;
}

void NNFieldInspector::KeypressCallbackFunction(vtkObject* caller, long unsigned int eventId, void* callData)
{
  std::cout << "KeypressCallbackFunction" << std::endl;

  if(this->LastPick[0] == -1)
  {
    std::cerr << "You cannot use the arrow keys until a click has been made." << std::endl;
    return;
  }

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::SafeDownCast(caller);

  if(!iren)
  {
    throw std::runtime_error("The iren cast failed!");
  }

  std::string pressedKey = iren->GetKeySym();

  double fakeClick[2];
  fakeClick[0] = this->LastPick[0];
  fakeClick[1] = this->LastPick[1];

  if(pressedKey == "Up")
  {
    fakeClick[1] += 1;
  }
  else if(pressedKey == "Down")
  {
    fakeClick[1] -= 1;
  }
  else if(pressedKey == "Left")
  {
    fakeClick[0] -= 1;
  }
  else if(pressedKey == "Right")
  {
    fakeClick[0] += 1;
  }
  else
  {
    return;
  }

  PixelClickedEventHandler(NULL, 0, fakeClick);
}

void NNFieldInspector::showEvent(QShowEvent* event)
{
  if(this->ImageFileName.empty() || this->NNFieldFileName.empty())
  {
    return;
  }
  else
  {
    LoadImage(this->ImageFileName);
    LoadNNField(this->NNFieldFileName);
    this->Camera.SetCameraPositionPNG();
  }
}

void NNFieldInspector::closeEvent(QCloseEvent* event)
{
  std::cout << "Exiting..." << std::endl;
  QApplication::exit();
}

void NNFieldInspector::dropEvent ( QDropEvent * event )
{
  std::cout << "dropEvent." << std::endl;

  //QString filename = event->mimeData()->data("FileName");
  QString data = event->mimeData()->text();
  std::stringstream ss;
  ss << data.toStdString();

  ss >> this->LastPick[0] >> this->LastPick[1];

  std::cout << "Dropped " << data.toStdString() << std::endl;
  std::cout << "Last pick set from drop: " << this->LastPick << std::endl;
  std::cout << "BestMatchCenter set from drop: " << this->LastPick << std::endl;

  double fakeClick[2];
  fakeClick[0] = this->LastPick[0];
  fakeClick[1] = this->LastPick[1];

  PixelClickedEventHandler(NULL, 0, fakeClick);
}

void NNFieldInspector::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;

    std::stringstream ss;
    ss << this->LastPick[0] << " " << this->LastPick[1] << " "
       << this->BestMatchCenter[0] << " " << this->BestMatchCenter[1];
    mimeData->setText(ss.str().c_str());
    std::cout << "Dragging " << ss.str() << std::endl;
    drag->setMimeData(mimeData);

    //Qt::DropAction dropAction = drag->exec();
    drag->exec();
  }
}

void NNFieldInspector::dragEnterEvent ( QDragEnterEvent * event )
{
  //std::cout << "dragEnterEvent." << std::endl;

  event->accept();
}
