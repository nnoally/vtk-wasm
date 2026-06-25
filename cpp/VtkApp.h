#pragma once

#include <string>
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

class VtkApp {
public:
    void init();
    void loadFile(const std::string& path);
    void setColor(double r, double g, double b);

private:
    vtkSmartPointer<vtkRenderWindow>           renderWindow;
    vtkSmartPointer<vtkRenderer>               renderer;
    vtkSmartPointer<vtkRenderWindowInteractor> interactor;
    vtkSmartPointer<vtkActor>                  actor;
};
