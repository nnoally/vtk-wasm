cmake_minimum_required(VERSION 3.20)
project(vtk_wasm_demo CXX)

set(CMAKE_CXX_STANDARD 17)

# Requires: emcmake cmake -DVTK_DIR=/path/to/vtk-wasm-sdk ..
find_package(VTK REQUIRED COMPONENTS
    RenderingCore
    RenderingOpenGL2
    IOGeometry
    IOPLY
    IOXML
    FiltersGeometry
)

add_executable(vtk_app
    cpp/VtkApp.cpp
    cpp/bindings.cpp
)

target_link_libraries(vtk_app PRIVATE ${VTK_LIBRARIES})
vtk_module_autoinit(TARGETS vtk_app MODULES ${VTK_LIBRARIES})

set_target_properties(vtk_app PROPERTIES
    SUFFIX ".mjs"
    LINK_FLAGS "--bind \
        -sMODULARIZE=1 \
        -sEXPORT_NAME=createVtkModule \
        -sEXPORTED_RUNTIME_METHODS=FS \
        -sENVIRONMENT=web \
        -sALLOW_MEMORY_GROWTH=1 \
        -sEXPORT_ES6=1"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/web/public"
)
