# VTK WASM — Build Docker multi-stage

**Date :** 2026-06-25
**Statut :** Approuvé

## Objectif

Rendre le build C++ → WASM entièrement reproductible via Docker, sans aucune dépendance locale (Emscripten, VTK SDK). La commande `docker compose up --build` doit suffire pour compiler et servir le démonstrateur.

---

## Dépendance VTK

L'image officielle Kitware [`kitware/vtk-wasm-sdk:9.6.0`](https://hub.docker.com/r/kitware/vtk-wasm-sdk) est utilisée comme base du build. Elle contient :

- Emscripten toolchain
- VTK 9.6.0 précompilé pour WASM32
- `VTK_DIR` disponible à : `/VTK-install/Release/wasm32/lib/cmake/vtk`

Tag épinglé à `9.6.0` (stable, 4 mois) pour reproductibilité.

---

## Architecture du build

```
Dockerfile (multi-stage)
├── Stage 1 — builder  (kitware/vtk-wasm-sdk:9.6.0)
│   ├── COPY CMakeLists.txt + cpp/
│   ├── emcmake cmake -DVTK_DIR=... -DCMAKE_BUILD_TYPE=Release
│   ├── cmake --build --parallel
│   └── output → /src/web/public/vtk_app.mjs + vtk_app.wasm
│
└── Stage 2 — runner  (nginx:alpine)
    ├── COPY web/                      → /usr/share/nginx/html/
    ├── COPY --from=builder /src/web/public/  → /usr/share/nginx/html/public/
    └── COPY nginx/default.conf
```

**Cache Docker :** `CMakeLists.txt` et `cpp/` sont copiés avant le `RUN cmake`. Un changement JS seul n'invalide pas le cache du build C++.

---

## Fichiers touchés

| Fichier | Action |
|---|---|
| `Dockerfile` | Créer — multi-stage builder + runner |
| `.dockerignore` | Créer — exclut `web/public/` et `.git/` |
| `docker-compose.yml` | Modifier — remplace `image: nginx:alpine` + volumes par `build: .` |
| `CMakeLists.txt` | Modifier — `LINK_FLAGS` → `target_link_options`, ajouter `CMAKE_CXX_STANDARD_REQUIRED ON` |

---

## Détail des fichiers

### `Dockerfile`

```dockerfile
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

COPY web/                           /usr/share/nginx/html/
COPY --from=builder /src/web/public/ /usr/share/nginx/html/public/
COPY nginx/default.conf             /etc/nginx/conf.d/default.conf
```

### `docker-compose.yml`

```yaml
services:
  web:
    build: .
    ports:
      - "8080:80"
```

### `.dockerignore`

```
web/public/
.git/
.superpowers/
docs/
```

### `CMakeLists.txt` — corrections

Remplacer `LINK_FLAGS` par `target_link_options` :

```cmake
target_link_options(vtk_app PRIVATE
    --bind
    -sMODULARIZE=1
    -sEXPORT_NAME=createVtkModule
    -sEXPORTED_RUNTIME_METHODS=FS
    -sENVIRONMENT=web
    -sALLOW_MEMORY_GROWTH=1
    -sEXPORT_ES6=1
)
```

Ajouter après `set(CMAKE_CXX_STANDARD 17)` :

```cmake
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

---

## Workflow

```bash
# Build C++ + serve (première fois ou après changement C++)
docker compose up --build

# Re-serve sans rebuild
docker compose up

# Build seul sans démarrer
docker compose build
```

L'application est disponible sur `http://localhost:8080`.
