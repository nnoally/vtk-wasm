# VTK WASM Demonstrateur — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a static web demonstrator where a user drag & drops a 3D file, rendered via C++ VTK compiled to WebAssembly, with a color palette to change the object color.

**Architecture:** C++ VTK logic lives in `VtkApp.cpp`, exposed to JavaScript via `EMSCRIPTEN_BINDINGS` in `bindings.cpp`. The JS frontend (vanilla, no framework, no Node.js) calls the three exposed functions: `init()`, `loadFile(path)`, `setColor(r,g,b)`. nginx serves the static files with the required COEP/COOP headers via Docker Compose.

**Tech Stack:** C++17, VTK 9.x (Kitware WASM prebuilt SDK), Emscripten, nginx:alpine, Docker Compose, vanilla JS (ES modules).

## Global Constraints

- No Node.js, no npm, no frontend build step
- Each JS file must stay under 60 lines
- COEP/COOP headers mandatory (`Cross-Origin-Opener-Policy: same-origin`, `Cross-Origin-Embedder-Policy: require-corp`)
- Canvas bound by CSS selector `#vtk-canvas`
- Supported formats: `.stl`, `.obj`, `.ply`, `.vtp`, `.vtu`
- C++ build output goes to `web/public/vtk_app.mjs` + `web/public/vtk_app.wasm`
- VTK WASM SDK prebuilt by Kitware; set `VTK_DIR` env var to its path before cmake

---

## File Map

| File | Responsibility |
|---|---|
| `CMakeLists.txt` | Emscripten build: compiles C++ → vtk_app.mjs + .wasm |
| `docker-compose.yml` | Runs nginx:alpine, mounts web/ and nginx/default.conf |
| `nginx/default.conf` | Serves web/ at :8080 with COEP/COOP headers |
| `cpp/VtkApp.h` | VtkApp class declaration |
| `cpp/VtkApp.cpp` | VTK pipeline: init, loadFile, setColor |
| `cpp/bindings.cpp` | EMSCRIPTEN_BINDINGS exposing VtkApp to JS |
| `web/index.html` | Page structure + inline CSS |
| `web/src/main.js` | Loads WASM module, wires drag-drop and palette |
| `web/src/drag-drop.js` | Handles drag & drop events, calls app.loadFile |
| `web/src/color-palette.js` | Renders 8 color buttons, calls app.setColor |

---

## Task 1: Project scaffold, Docker, nginx

**Files:**
- Create: `docker-compose.yml`
- Create: `nginx/default.conf`
- Create: `web/index.html` (stub)
- Create: `.gitignore`

**Interfaces:**
- Produces: running HTTP server at `http://localhost:8080` serving `web/`

- [ ] **Step 1: Create `.gitignore`**

```
build/
web/public/vtk_app.mjs
web/public/vtk_app.wasm
```

- [ ] **Step 2: Create `nginx/default.conf`**

```nginx
server {
    listen 80;
    root /usr/share/nginx/html;
    index index.html;

    add_header Cross-Origin-Opener-Policy   "same-origin";
    add_header Cross-Origin-Embedder-Policy "require-corp";

    location / {
        try_files $uri $uri/ =404;
    }
}
```

- [ ] **Step 3: Create `docker-compose.yml`**

```yaml
services:
  web:
    image: nginx:alpine
    ports:
      - "8080:80"
    volumes:
      - ./web:/usr/share/nginx/html:ro
      - ./nginx/default.conf:/etc/nginx/conf.d/default.conf:ro
```

- [ ] **Step 4: Create stub `web/index.html`**

```html
<!DOCTYPE html>
<html lang="fr">
<head><meta charset="UTF-8"><title>VTK WASM</title></head>
<body><p>VTK WASM Demonstrateur</p></body>
</html>
```

- [ ] **Step 5: Verify nginx serves the page**

```bash
docker compose up -d
curl -I http://localhost:8080
```

Expected: `HTTP/1.1 200 OK` with headers `cross-origin-opener-policy: same-origin` and `cross-origin-embedder-policy: require-corp`.

```bash
docker compose down
```

- [ ] **Step 6: Commit**

```bash
git add .gitignore docker-compose.yml nginx/default.conf web/index.html
git commit -m "feat: project scaffold with nginx + Docker Compose"
```

---

## Task 2: HTML/CSS layout

**Files:**
- Modify: `web/index.html` (full layout replacing stub)
- Create: `web/public/.gitkeep` (ensure directory exists for WASM output)

**Interfaces:**
- Produces: page with `#canvas-container`, `#vtk-canvas`, `#drop-overlay`, `#error-message`, `#color-palette` elements; CSS classes `.disabled`, `.drag-over`, `.active`

- [ ] **Step 1: Replace `web/index.html` with full layout**

```html
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>VTK WASM — Démonstrateur</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: system-ui, sans-serif; display: flex; flex-direction: column;
           height: 100vh; background: #1a1a1a; color: #fff; }
    header { padding: 12px 20px; background: #111; border-bottom: 1px solid #333; }
    h1 { font-size: 1.1rem; font-weight: 500; letter-spacing: 0.05em; }
    main { flex: 1; position: relative; overflow: hidden; }
    #canvas-container { position: relative; width: 100%; height: 100%; }
    #vtk-canvas { display: block; width: 100%; height: 100%; }
    #drop-overlay { position: absolute; inset: 0; display: flex; flex-direction: column;
                    align-items: center; justify-content: center; gap: 8px;
                    background: rgba(0,0,0,0.65); pointer-events: none; }
    #canvas-container.drag-over #drop-overlay {
      background: rgba(59,130,246,0.25); outline: 3px dashed #3b82f6; }
    #drop-overlay p { font-size: 1.1rem; }
    #drop-overlay .formats { color: #888; font-size: 0.82rem; }
    #error-message { position: absolute; bottom: 20px; left: 50%; transform: translateX(-50%);
                     background: #dc2626; padding: 8px 20px; border-radius: 4px;
                     font-size: 0.88rem; white-space: nowrap; }
    footer { padding: 12px 20px; background: #111; border-top: 1px solid #333; }
    #color-palette { display: flex; gap: 10px; align-items: center; }
    #color-palette.disabled { opacity: 0.3; pointer-events: none; }
    #color-palette button { width: 32px; height: 32px; border-radius: 50%;
                            border: 2px solid transparent; cursor: pointer; transition: transform .15s; }
    #color-palette button:hover { transform: scale(1.15); }
    #color-palette button.active { border-color: #fff; }
  </style>
</head>
<body>
  <header><h1>VTK WASM — Démonstrateur C++</h1></header>
  <main>
    <div id="canvas-container">
      <canvas id="vtk-canvas"></canvas>
      <div id="drop-overlay">
        <p>Déposez un fichier 3D ici</p>
        <p class="formats">.stl .obj .ply .vtp .vtu</p>
      </div>
      <div id="error-message" hidden></div>
    </div>
  </main>
  <footer>
    <div id="color-palette" class="disabled"></div>
  </footer>
  <script type="module" src="src/main.js"></script>
</body>
</html>
```

- [ ] **Step 2: Create `web/public/.gitkeep`**

```bash
mkdir -p web/public && touch web/public/.gitkeep
```

- [ ] **Step 3: Verify layout in browser**

```bash
docker compose up -d
```

Open `http://localhost:8080` in a browser. Expected: dark page with header "VTK WASM — Démonstrateur C++", centered drop overlay, empty footer. No console errors (the `main.js` 404 is expected at this stage).

```bash
docker compose down
```

- [ ] **Step 4: Commit**

```bash
git add web/index.html web/public/.gitkeep
git commit -m "feat: HTML/CSS layout with drop zone and color palette"
```

---

## Task 3: CMakeLists.txt + VtkApp init

**Files:**
- Create: `CMakeLists.txt`
- Create: `cpp/VtkApp.h`
- Create: `cpp/VtkApp.cpp` (init only — loadFile and setColor stubs)
- Create: `cpp/bindings.cpp` (init binding only)

**Interfaces:**
- Produces: `web/public/vtk_app.mjs` + `web/public/vtk_app.wasm`; JS can call `new module.VtkApp()` then `app.init()`

- [ ] **Step 1: Create `cpp/VtkApp.h`**

```cpp
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
```

Note: `vtkRenderWindow` and `vtkRenderWindowInteractor` are abstract base classes. VTK WASM provides concrete WASM implementations; the factory `New()` call dispatches to the correct type automatically when VTK is built with the WASM backend. If the Kitware SDK provides explicit `vtkWebAssemblyOpenGLRenderWindow`, replace the base class types with those in this header.

- [ ] **Step 2: Create `cpp/VtkApp.cpp`**

```cpp
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
```

- [ ] **Step 3: Create `cpp/bindings.cpp`**

```cpp
#include <emscripten/bind.h>
#include "VtkApp.h"

EMSCRIPTEN_BINDINGS(vtk_app) {
    emscripten::class_<VtkApp>("VtkApp")
        .constructor<>()
        .function("init",     &VtkApp::init)
        .function("loadFile", &VtkApp::loadFile)
        .function("setColor", &VtkApp::setColor);
}
```

- [ ] **Step 4: Create `CMakeLists.txt`**

```cmake
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
```

- [ ] **Step 5: Build C++ → WASM**

```bash
# Set VTK_DIR to your Kitware prebuilt WASM SDK path, e.g.:
# export VTK_DIR=/opt/vtk-wasm/lib/cmake/vtk
emcmake cmake -B build -DVTK_DIR=$VTK_DIR .
cmake --build build -- -j$(nproc)
```

Expected: `web/public/vtk_app.mjs` and `web/public/vtk_app.wasm` created. No link errors.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt cpp/VtkApp.h cpp/VtkApp.cpp cpp/bindings.cpp
git commit -m "feat: VtkApp C++ skeleton with init, Emscripten bindings, CMakeLists"
```

---

## Task 4: main.js — WASM loader and state management

**Files:**
- Create: `web/src/main.js`

**Interfaces:**
- Consumes: `web/public/vtk_app.mjs` (ES module exporting `createVtkModule` factory)
- Produces: `initDragDrop(app, FS)` and `initColorPalette(app)` called after WASM ready; `setState(newState, message?)` exported for drag-drop.js

- [ ] **Step 1: Create `web/src/main.js`**

```javascript
import createVtkModule from '../public/vtk_app.mjs';
import { initDragDrop }    from './drag-drop.js';
import { initColorPalette } from './color-palette.js';

const overlay  = document.getElementById('drop-overlay');
const errMsg   = document.getElementById('error-message');
const palette  = document.getElementById('color-palette');

export function setState(newState, message = '') {
    overlay.hidden  = newState === 'charge';
    errMsg.hidden   = newState !== 'erreur';
    if (message) errMsg.textContent = message;
    palette.classList.toggle('disabled', newState !== 'charge');
}

createVtkModule().then(module => {
    const app = new module.VtkApp();
    app.init();
    initDragDrop(app, module.FS);
    initColorPalette(app);
}).catch(err => {
    console.error('WASM load failed:', err);
    setState('erreur', 'Impossible de charger le module VTK WASM.');
});
```

- [ ] **Step 2: Verify WASM loads in browser**

```bash
docker compose up -d
```

Open `http://localhost:8080`, open DevTools console. Expected: no errors, VTK renders a dark grey canvas. The drop overlay is still visible (no file loaded yet).

```bash
docker compose down
```

- [ ] **Step 3: Commit**

```bash
git add web/src/main.js
git commit -m "feat: main.js loads VTK WASM module and manages app state"
```

---

## Task 5: VtkApp loadFile + drag-drop.js

**Files:**
- Modify: `cpp/VtkApp.cpp` (implement `loadFile`)
- Create: `web/src/drag-drop.js`

**Interfaces:**
- Consumes: `app.loadFile(path: string)` (C++); `module.FS` (Emscripten virtual filesystem); `setState` from `main.js`
- Produces: 3D mesh rendered in canvas after file drop

- [ ] **Step 1: Implement `loadFile` in `cpp/VtkApp.cpp`**

Replace the stub `loadFile` with:

```cpp
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
```

Also add `#include <algorithm>` at the top of `VtkApp.cpp`.

- [ ] **Step 2: Rebuild C++ → WASM**

```bash
cmake --build build -- -j$(nproc)
```

Expected: build succeeds, `web/public/vtk_app.mjs` updated.

- [ ] **Step 3: Create `web/src/drag-drop.js`**

```javascript
import { setState } from './main.js';

const ACCEPTED = ['.stl', '.obj', '.ply', '.vtp', '.vtu'];

export function initDragDrop(app, FS) {
    const container = document.getElementById('canvas-container');

    container.addEventListener('dragover', e => {
        e.preventDefault();
        container.classList.add('drag-over');
    });

    container.addEventListener('dragleave', () => {
        container.classList.remove('drag-over');
    });

    container.addEventListener('drop', e => {
        e.preventDefault();
        container.classList.remove('drag-over');

        const file = e.dataTransfer.files[0];
        if (!file) return;

        const ext = file.name.slice(file.name.lastIndexOf('.')).toLowerCase();
        if (!ACCEPTED.includes(ext)) {
            setState('erreur', `Format non supporté : ${ext}. Acceptés : ${ACCEPTED.join(' ')}`);
            return;
        }

        file.arrayBuffer().then(buffer => {
            const path = '/tmp/' + file.name;
            FS.writeFile(path, new Uint8Array(buffer));
            app.loadFile(path);
            setState('charge');
        });
    });
}
```

- [ ] **Step 4: Verify drag & drop in browser**

```bash
docker compose up -d
```

Open `http://localhost:8080`. Drag a `.stl` or `.obj` file onto the page. Expected: the 3D mesh appears in white, the drop overlay hides, the color palette footer becomes active (no longer greyed out).

Try dragging an unsupported file (e.g. `.png`). Expected: red error banner at the bottom.

```bash
docker compose down
```

- [ ] **Step 5: Commit**

```bash
git add cpp/VtkApp.cpp web/src/drag-drop.js
git commit -m "feat: loadFile C++ pipeline + drag & drop JS handler"
```

---

## Task 6: VtkApp setColor + color-palette.js

**Files:**
- Modify: `cpp/VtkApp.cpp` (implement `setColor`)
- Create: `web/src/color-palette.js`

**Interfaces:**
- Consumes: `app.setColor(r: number, g: number, b: number)` (C++); `app` instance from `main.js`
- Produces: object color changes on palette click; active button highlighted

- [ ] **Step 1: Implement `setColor` in `cpp/VtkApp.cpp`**

Replace the stub `setColor` with:

```cpp
void VtkApp::setColor(double r, double g, double b) {
    if (!actor) return;
    actor->GetProperty()->SetColor(r, g, b);
    renderWindow->Render();
}
```

- [ ] **Step 2: Rebuild C++ → WASM**

```bash
cmake --build build -- -j$(nproc)
```

Expected: build succeeds, `web/public/vtk_app.mjs` updated.

- [ ] **Step 3: Create `web/src/color-palette.js`**

```javascript
const COLORS = [
    { name: 'blanc',  rgb: [1.00, 1.00, 1.00] },
    { name: 'gris',   rgb: [0.50, 0.50, 0.50] },
    { name: 'rouge',  rgb: [0.80, 0.10, 0.10] },
    { name: 'orange', rgb: [0.90, 0.50, 0.10] },
    { name: 'jaune',  rgb: [0.90, 0.90, 0.10] },
    { name: 'vert',   rgb: [0.10, 0.70, 0.10] },
    { name: 'bleu',   rgb: [0.10, 0.30, 0.90] },
    { name: 'violet', rgb: [0.60, 0.10, 0.80] },
];

export function initColorPalette(app) {
    const palette = document.getElementById('color-palette');

    COLORS.forEach(({ name, rgb }) => {
        const btn = document.createElement('button');
        btn.title = name;
        btn.setAttribute('aria-label', name);
        btn.style.background =
            `rgb(${rgb.map(v => Math.round(v * 255)).join(',')})`;

        btn.addEventListener('click', () => {
            palette.querySelectorAll('button')
                   .forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            app.setColor(...rgb);
        });

        palette.appendChild(btn);
    });
}
```

- [ ] **Step 4: Verify full interaction in browser**

```bash
docker compose up -d
```

Open `http://localhost:8080`. Steps to verify:
1. Drag a `.stl`/`.obj`/`.ply` file → mesh appears in white
2. Click "rouge" → mesh turns red
3. Click "bleu" → mesh turns blue; "rouge" button loses active highlight
4. Load a `.vtu` file → mesh reloads; color resets to white (actor recreated)
5. Rotate the mesh by clicking and dragging in the canvas — VTK interactor works

```bash
docker compose down
```

- [ ] **Step 5: Commit**

```bash
git add cpp/VtkApp.cpp web/src/color-palette.js
git commit -m "feat: setColor C++ + color palette JS widget — demonstrator complete"
```
