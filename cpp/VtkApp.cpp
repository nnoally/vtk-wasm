#include "VtkApp.h"
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkPolyDataMapper.h>
#include <vtkSTLReader.h>
#include <vtkOBJReader.h>
#include <vtkPLYReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkGeometryFilter.h>

void VtkApp::init() {
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->SetBackground(0.18, 0.18, 0.18);

    renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->SetCanvasSelector("#vtk-canvas");
    renderWindow->AddRenderer(renderer);
    renderWindow->SetSize(800, 600);

    interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(renderWindow);
    interactor->Initialize();

    renderWindow->Render();
}

void VtkApp::loadFile(const std::string& path) {
    // implemented in Task 5
}

void VtkApp::setColor(double r, double g, double b) {
    // implemented in Task 6
}
