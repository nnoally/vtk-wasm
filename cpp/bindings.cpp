#include <emscripten/bind.h>
#include "VtkApp.h"

EMSCRIPTEN_BINDINGS(vtk_app) {
    emscripten::class_<VtkApp>("VtkApp")
        .constructor<>()
        .function("init",     &VtkApp::init)
        .function("start",    &VtkApp::start)
        .function("loadFile", &VtkApp::loadFile)
        .function("setColor", &VtkApp::setColor);
}
