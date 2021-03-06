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

#ifndef NNFieldInspector_H
#define NNFieldInspector_H

#include "ui_NNFieldInspector.h"

// VTK
#include <vtkSmartPointer.h>
#include <vtkSeedWidget.h>
#include <vtkPointHandleRepresentation2D.h>
#include <vtkRenderer.h>

// ITK
#include "itkImage.h"
#include "itkVectorImage.h"

// Qt
#include <QMainWindow>

// Submodules
#include "ITKVTKCamera/ITKVTKCamera.h"
#include "Layer/Layer.h"

// Custom
#include "PointSelectionStyle2D.h"

class NNFieldInspector : public QMainWindow, public Ui::NNFieldInspector
{
  Q_OBJECT
public:

  /** If the NNField is interpreted as an absolute position field, the NNPatch is specified
    * directly by the value at the field pixel. If it is interpreted as an offset field, the
    * NNPatch is specified by the field pixel location + the field pixel value. */
  enum INTERPRETATION_ENUM {OFFSET, ABSOLUTE};

  typedef itk::Image<itk::CovariantVector<unsigned char, 3>, 2> ImageType;
  typedef itk::VectorImage<float, 2> NNFieldImageType;

  /** Constructor */
  NNFieldInspector();
  NNFieldInspector(const std::string& imageFileName, const std::string& nnFieldFileName);

  /** Set the radius of the patches.*/
  void SetPatchRadius(const unsigned int patchRadius);

public slots:

  void on_actionOpenImage_activated();
  void on_actionOpenNNField_activated();

  void on_actionHelp_activated();
  void on_actionQuit_activated();

  void on_actionFlipHorizontally_activated();
  void on_actionFlipVertically_activated();

  // Edit menu
  void on_actionInterpretAsOffsetField_activated();
  void on_actionInterpretAsAbsoluteField_activated();

  void on_radRGB_clicked();
  void on_radNNFieldMagnitude_clicked();
  void on_radNNFieldX_clicked();
  void on_radNNFieldY_clicked();

private:

  /** React to a keypress.*/
  void KeypressCallbackFunction(vtkObject* caller, long unsigned int eventId, void* callData);

  /** React to a pick event.*/
  void PixelClickedEventHandler(vtkObject* caller, long unsigned int eventId,
                                void* callData);

  /** Functionality shared by all constructors.*/
  void SharedConstructor();

  /** The nearest neighbor field.*/
  NNFieldImageType::Pointer NNField;

  /** The image over which the nearest neighbor field is defined.*/
  ImageType::Pointer Image;

  /** Load an image.*/
  void LoadImage(const std::string& fileName);

  /** Load a nearest neighbor field.*/
  void LoadNNField(const std::string& fileName);

  /** The layer used to display the RGB image.*/
  Layer ImageLayer;

  /** The layer used to display the magnitude of the nearest neighbor field.*/
  Layer NNFieldMagnitudeLayer;

  /** The layer used to display the X component of the nearest neighbor field.*/
  Layer NNFieldXLayer;

  /** The layer used to display the Y component of the nearest neighbor field.*/
  Layer NNFieldYLayer;

  /** The layer used to do the picking. This layer is always on top and is transparent everywhere
    * except the outline of the current patch and its best match.
    */
  Layer PickLayer;

  /** An object to handle flipping the camera.*/
  ITKVTKCamera Camera;

  /** The radius of the patches.*/
  unsigned int PatchRadius;

  /** The object to handle the picking.*/
  PointSelectionStyle2D* SelectionStyle;

  /** The renderer.*/
  vtkSmartPointer<vtkRenderer> Renderer;

  /** Update the display.*/
  void UpdateDisplayedImages();

  /** How to interpret the NNfield */
  INTERPRETATION_ENUM Interpretation;

  /** The last pick.*/
  int LastPick[2];

  /** Refresh the window.*/
  void Refresh();

  /** When the widget finishes loading, this function is called. */
  void showEvent(QShowEvent* event);

  /** When the widget is closed. */
  void closeEvent(QCloseEvent* event);

  /** Store the file names so they can be loaded in the showEvent instead of in the constructor directly. */
  std::string NNFieldFileName;
  std::string ImageFileName;

  /** Get data that has been dropped. */
  void dropEvent ( QDropEvent * event );

  /** Start the drag event. */
  void mousePressEvent(QMouseEvent *event);

  void dragEnterEvent ( QDragEnterEvent * event );

  itk::Index<2> BestMatchCenter;
};

#endif
