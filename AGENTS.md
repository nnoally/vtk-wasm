# AGENTS.md — Guide for AI Agents

This file gives AI coding agents the context they need to work effectively in this repository.

## What This Project Is

A static 3D file viewer: C++ VTK rendering compiled to WebAssembly via Emscripten, served by nginx. The JS frontend calls three C++ functions exposed through Emscripten bindings: `init()`, `loadFile(path)`, `setColor(r,g,b)`.

## Repository Layout

```
cpp/            C++ source — VTK pipeline + Emscripten bindings
web/src/        Vanilla JS ES modules (no build step, no npm)
web/public/     Build output (git-ignored): vtk_app.mjs + vtk_app.wasm
web/index.html  Single HTML page with inline CSS
nginx/          nginx config (COEP/COOP headers)
docs/superpowers/
  plans/        Step-by-step implementation plans (markdown checklists)
  specs/        Design documents for each feature
```

## Hard Constraints

- **No Node.js, no npm, no frontend build step.** The JS is plain ES modules loaded directly by the browser.
- **Each JS file must stay under 60 lines.**
- **C++ standard: C++17.**
- **COEP/COOP headers are mandatory** (`Cross-Origin-Opener-Policy: same-origin`, `Cross-Origin-Embedder-Policy: require-corp`) — the WASM module will not load without them.
- **Canvas selector is `#vtk-canvas`** — hardcoded in `VtkApp.cpp:init()`.
- **Supported 3D formats:** `.stl` `.obj` `.ply` `.vtp` `.vtu`
- **WASM build output goes to `web/public/`** (`vtk_app.mjs` + `vtk_app.wasm`).

## C++ ↔ JS Interface

The only JS-callable surface is:

| Method | Signature | Description |
|---|---|---|
| `init` | `() → void` | Creates renderer, binds `#vtk-canvas`, starts interactor |
| `loadFile` | `(path: string) → void` | Reads from Emscripten virtual FS, builds VTK pipeline |
| `setColor` | `(r, g, b: double) → void` | Sets actor diffuse color, triggers re-render |

Files must be written to the Emscripten virtual FS (`module.FS.writeFile`) before `loadFile` is called.

## Build System

**Docker (full build):**
```bash
docker compose up --build
```
The `Dockerfile` is two-stage: `kitware/vtk-wasm-sdk:9.6.0` builder → `nginx:alpine` runner.

**Local (Emscripten + VTK SDK required):**
```bash
export VTK_DIR=/path/to/vtk-wasm-sdk/Release/wasm32/lib/cmake/vtk
emcmake cmake -B build -DVTK_DIR=$VTK_DIR .
cmake --build build --parallel
```

Emscripten link flags are in `CMakeLists.txt` (`target_link_options`). Key flags:
- `-sMODULARIZE=1 -sEXPORT_NAME=createVtkModule` — factory function pattern
- `-sEXPORTED_RUNTIME_METHODS=FS` — exposes the virtual filesystem to JS
- `-sEXPORT_ES6=1` — output is an ES module

## JS Module Responsibilities

| File | Responsibility |
|---|---|
| `main.js` | Load WASM module, manage UI state (`setState`) |
| `drag-drop.js` | Handle drop events, write to FS, call `loadFile`; exports `ACCEPTED` |
| `color-palette.js` | Render color buttons, call `setColor` |
| `url-loader.js` | Parse `?file=` param (with optional Basic Auth), fetch, write to FS |

`setState(state, message?)` in `main.js` controls the overlay/error banner/palette:
- `'charge'` — file loaded, overlay hidden, palette active
- `'erreur'` — show error banner with `message`

## Docs

`docs/superpowers/plans/` contains markdown implementation plans with `- [ ]` task checklists. Mark steps as done (`- [x]`) as you complete them.

`docs/superpowers/specs/` contains design documents. Read the relevant spec before implementing a feature.

## Common Tasks

**Add a new 3D format:**
1. Add the extension to the `ACCEPTED` array in `drag-drop.js` (also imported by `url-loader.js`).
2. Add the corresponding reader branch in `VtkApp::loadFile` in `cpp/VtkApp.cpp`.
3. Add the VTK reader component to `find_package(VTK ...)` in `CMakeLists.txt` if needed.
4. Rebuild WASM.

**Add a JS feature:**
- Create a new file in `web/src/`, keep it under 60 lines.
- Import and call it from `main.js`.

**Modify nginx headers or routing:**
- Edit `nginx/default.conf`. Rebuild the Docker image to apply changes.
