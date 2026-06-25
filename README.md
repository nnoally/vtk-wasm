# VTK WASM — Démonstrateur 3D

A static web viewer for 3D files powered by **VTK compiled to WebAssembly**. The C++ rendering pipeline runs entirely in the browser — no server-side processing, no Node.js, no npm.

## Features

- Drag & drop a 3D file onto the canvas to render it
- Load a remote file via the `?file=` URL parameter (supports HTTP Basic Auth: `?file=https://user:pass@host/model.stl`)
- Click a color in the palette to recolor the mesh in real time
- Interactive rotation/zoom via the built-in VTK interactor

**Supported formats:** `.stl` `.obj` `.ply` `.vtp` `.vtu`

## Architecture

```
C++ (VTK 9.x)  →  Emscripten  →  vtk_app.mjs + vtk_app.wasm
                                        ↓
                               Vanilla JS (ES modules)
                                        ↓
                               nginx:alpine (Docker)
```

| Layer | Technology |
|---|---|
| 3D rendering | VTK 9.x (Kitware prebuilt WASM SDK) |
| C++ → WASM | Emscripten + CMake |
| Frontend | Vanilla JS ES modules, no build step |
| HTTP server | nginx:alpine with COEP/COOP headers |
| Container | Docker + Docker Compose |

## Project Structure

```
vtk-wasm/
├── cpp/
│   ├── VtkApp.h          # VtkApp class: init, loadFile, setColor
│   ├── VtkApp.cpp        # VTK pipeline implementation
│   └── bindings.cpp      # Emscripten bindings exposing VtkApp to JS
├── web/
│   ├── index.html        # Page structure + inline CSS
│   ├── public/           # Build output: vtk_app.mjs + vtk_app.wasm (git-ignored)
│   └── src/
│       ├── main.js       # WASM loader, app state management
│       ├── drag-drop.js  # Drag & drop handler
│       ├── color-palette.js  # Color palette widget
│       └── url-loader.js     # ?file= URL parameter loader
├── nginx/default.conf    # COEP/COOP headers required for SharedArrayBuffer
├── CMakeLists.txt        # Emscripten build: C++ → vtk_app.mjs + .wasm
├── Dockerfile            # Multi-stage: builder (WASM) + nginx runner
└── docker-compose.yml    # Builds image, exposes :8080
```

## Prerequisites

- [Docker](https://docs.docker.com/get-docker/) and Docker Compose
- For local C++ builds: [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) and the [Kitware VTK WASM SDK](https://vtk.org/download/)

## Quick Start

**Build and run with Docker (recommended):**

```bash
docker compose up --build
```

Open [http://localhost:8080](http://localhost:8080).

The Dockerfile handles the full two-stage build: C++ → WASM (using `kitware/vtk-wasm-sdk:9.6.0`), then copies the output into an nginx:alpine image.

## Local C++ Build

If you have Emscripten and the VTK WASM SDK installed locally:

```bash
export VTK_DIR=/path/to/vtk-wasm-sdk/Release/wasm32/lib/cmake/vtk
emcmake cmake -B build -DVTK_DIR=$VTK_DIR .
cmake --build build --parallel
```

This produces `web/public/vtk_app.mjs` and `web/public/vtk_app.wasm`. Serve `web/` with any static server that sets the required COEP/COOP headers (see `nginx/default.conf`).

## COEP/COOP Headers

WebAssembly with `SharedArrayBuffer` requires:

```
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

These are set in `nginx/default.conf`. Without them the WASM module will fail to load.

## JavaScript API

The C++ `VtkApp` class is exposed to JS via Emscripten bindings:

```javascript
const app = new module.VtkApp();
app.init();                         // create renderer, bind #vtk-canvas
app.loadFile('/tmp/model.stl');     // load from Emscripten virtual FS
app.setColor(0.8, 0.1, 0.1);       // set mesh color (RGB 0–1)
```

Files must be written to the Emscripten virtual filesystem (`module.FS.writeFile`) before calling `loadFile`.
