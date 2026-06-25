#pragma once

#include <string>
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkWebAssemblyOpenGLRenderWindow.h>
#include <vtkWebAssemblyRenderWindowInteractor.h>

class VtkApp {
public:
    void init();
    void loadFile(const std::string& path);
    void setColor(double r, double g, double b);

private:
    vtkSmartPointer<vtkWebAssemblyOpenGLRenderWindow>    renderWindow;
    vtkSmartPointer<vtkRenderer>                         renderer;
    vtkSmartPointer<vtkWebAssemblyRenderWindowInteractor> interactor;
    vtkSmartPointer<vtkActor>                  actor;
};
