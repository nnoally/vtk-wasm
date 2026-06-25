#include "VtkApp.h"
#include <algorithm>
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
    std::string filename = path.substr(path.rfind('/') + 1);
    std::string ext = filename.substr(filename.rfind('.'));
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();

    if (ext == ".stl") {
        auto r = vtkSmartPointer<vtkSTLReader>::New();
        r->SetFileName(path.c_str());
        mapper->SetInputConnection(r->GetOutputPort());
    } else if (ext == ".obj") {
        auto r = vtkSmartPointer<vtkOBJReader>::New();
        r->SetFileName(path.c_str());
        mapper->SetInputConnection(r->GetOutputPort());
    } else if (ext == ".ply") {
        auto r = vtkSmartPointer<vtkPLYReader>::New();
        r->SetFileName(path.c_str());
        mapper->SetInputConnection(r->GetOutputPort());
    } else if (ext == ".vtp") {
        auto r = vtkSmartPointer<vtkXMLPolyDataReader>::New();
        r->SetFileName(path.c_str());
        mapper->SetInputConnection(r->GetOutputPort());
    } else if (ext == ".vtu") {
        auto r = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
        r->SetFileName(path.c_str());
        auto g = vtkSmartPointer<vtkGeometryFilter>::New();
        g->SetInputConnection(r->GetOutputPort());
        mapper->SetInputConnection(g->GetOutputPort());
    } else {
        return;
    }

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 1.0, 1.0);

    renderer->RemoveAllViewProps();
    renderer->AddActor(actor);
    renderer->ResetCamera();
    renderWindow->Render();
}

void VtkApp::setColor(double r, double g, double b) {
    // implemented in Task 6
}
