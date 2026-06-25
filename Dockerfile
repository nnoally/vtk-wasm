# Stage 1 : C++ → WASM
FROM kitware/vtk-wasm-sdk:9.6.0 AS builder

WORKDIR /src
COPY CMakeLists.txt .
COPY cpp/ cpp/

RUN emcmake cmake -S . -B /build \
        -DCMAKE_BUILD_TYPE=Release \
        -DVTK_DIR=/VTK-install/Release/wasm32/lib/cmake/vtk \
    && cmake --build /build --parallel

# Stage 2 : nginx runner
FROM nginx:alpine

COPY web/                            /usr/share/nginx/html/
COPY --from=builder /src/web/public/ /usr/share/nginx/html/public/
COPY nginx/default.conf              /etc/nginx/conf.d/default.conf
