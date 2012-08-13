cmake_minimum_required(VERSION 2.6)

PROJECT(InteractivePatchMatch)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

# C++11 support
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++0x")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})

FIND_PACKAGE(ITK REQUIRED ITKCommon ITKIOImageBase ITKIOPNG ITKIOMeta ITKDistanceMap
                          ITKImageIntensity ITKImageFeature ITKMathematicalMorphology ITKBinaryMathematicalMorphology)
INCLUDE(${USE_ITK_FILE})

FIND_PACKAGE(VTK REQUIRED vtkCommonCore vtkCommonDataModel vtkImagingMath vtkImagingCore vtkFiltersCore vtkIOXML
                          vtkRenderingCore vtkInteractionStyle vtkGUISupportQt vtkInteractionWidgets)
INCLUDE(${USE_VTK_FILE})

QT4_WRAP_UI(UISrcs NNFieldInspector.ui)
QT4_WRAP_CPP(MOCSrcs NNFieldInspector.h)

if(NOT TARGET ITKVTKCamera)
  add_subdirectory(ITKVTKCamera)
  include_directories(${ITKVTKCamera_includes})
  set(NNFieldInspector_libraries ${NNFieldInspector_libraries} ${ITKVTKCamera_libraries})
endif()

if(NOT TARGET PatchMatch)
  add_subdirectory(PatchMatch)
  include_directories(${PatchMatch_includes})
  set(NNFieldInspector_libraries ${NNFieldInspector_libraries} ${PatchMatch_libraries})
endif()

if(NOT TARGET Layer)
  add_subdirectory(Layer)
  include_directories(${Layer_includes})
  set(NNFieldInspector_libraries ${NNFieldInspector_libraries} ${Layer_libraries})
endif()

if(NOT TARGET VTKHelpers)
  add_subdirectory(VTKHelpers)
  include_directories(${VTKHelpers_includes})
  set(NNFieldInspector_libraries ${NNFieldInspector_libraries} ${VTKHelpers_libraries})
endif()

add_executable(NNFieldInspector NNFieldInspectorDriver.cpp
NNFieldInspector.cpp
PointSelectionStyle2D.cpp
${UISrcs} ${MOCSrcs})

TARGET_LINK_LIBRARIES(NNFieldInspector ${VTK_LIBRARIES} ${ITK_LIBRARIES}
${NNFieldInspector_libraries})
