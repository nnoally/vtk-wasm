# Docker Multi-Stage Build Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rendre le build C++ → WASM entièrement reproductible avec `docker compose up --build`, sans aucune dépendance locale.

**Architecture:** Dockerfile multi-stage : stage 1 (`kitware/vtk-wasm-sdk:9.6.0`) compile C++ → WASM via `emcmake cmake`, stage 2 (`nginx:alpine`) sert les artefacts générés + le frontend statique. `docker-compose.yml` pointe sur ce Dockerfile via `build: .`.

**Tech Stack:** Docker (multi-stage), `kitware/vtk-wasm-sdk:9.6.0`, nginx:alpine, CMake, Emscripten

## Global Constraints

- Image builder épinglée : `kitware/vtk-wasm-sdk:9.6.0`
- `VTK_DIR` dans le container builder : `/VTK-install/Release/wasm32/lib/cmake/vtk`
- `RUNTIME_OUTPUT_DIRECTORY` CMake reste `${CMAKE_SOURCE_DIR}/web/public` → `/src/web/public/` dans le container
- Headers nginx obligatoires : `Cross-Origin-Opener-Policy: same-origin` et `Cross-Origin-Embedder-Policy: require-corp`
- Aucun framework JS, aucun Node.js

---

### Task 1: Corriger CMakeLists.txt

**Files:**
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `CMakeLists.txt` valide utilisé tel quel par le Dockerfile au `RUN cmake`

- [ ] **Step 1: Remplacer `LINK_FLAGS` (déprécié) par `target_link_options`**

Ouvrir `CMakeLists.txt`. Remplacer le bloc `set_target_properties` entier par :

```cmake
cmake_minimum_required(VERSION 3.20)
project(vtk_wasm_demo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

target_link_options(vtk_app PRIVATE
    --bind
    -sMODULARIZE=1
    -sEXPORT_NAME=createVtkModule
    -sEXPORTED_RUNTIME_METHODS=FS
    -sENVIRONMENT=web
    -sALLOW_MEMORY_GROWTH=1
    -sEXPORT_ES6=1
)

set_target_properties(vtk_app PROPERTIES
    SUFFIX ".mjs"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/web/public"
)
```

- [ ] **Step 2: Vérifier la syntaxe**

```bash
grep -n "LINK_FLAGS\|target_link_options\|CXX_STANDARD" CMakeLists.txt
```

Résultat attendu :
```
3:set(CMAKE_CXX_STANDARD 17)
4:set(CMAKE_CXX_STANDARD_REQUIRED ON)
23:target_link_options(vtk_app PRIVATE
```

Aucune ligne `LINK_FLAGS` ne doit apparaître.

- [ ] **Step 3: Commit**

```bash
git add CMakeLists.txt
git commit -m "fix: use target_link_options, add CXX_STANDARD_REQUIRED"
```

---

### Task 2: Créer `.dockerignore`

**Files:**
- Create: `.dockerignore`

**Interfaces:**
- Produces: filtre les fichiers envoyés au daemon Docker pour éviter de copier des artefacts stales (`web/public/`) ou inutiles (`.git/`, docs)

- [ ] **Step 1: Créer le fichier**

Créer `.dockerignore` à la racine du projet avec ce contenu exact :

```
web/public/
.git/
.superpowers/
docs/
```

- [ ] **Step 2: Vérifier le contenu**

```bash
cat .dockerignore
```

Résultat attendu :
```
web/public/
.git/
.superpowers/
docs/
```

- [ ] **Step 3: Commit**

```bash
git add .dockerignore
git commit -m "chore: add .dockerignore for Docker build"
```

---

### Task 3: Créer le `Dockerfile`

**Files:**
- Create: `Dockerfile`

**Interfaces:**
- Consumes: `CMakeLists.txt` + `cpp/` (Task 1), `.dockerignore` (Task 2), `web/`, `nginx/default.conf`
- Produces: image Docker avec `nginx:alpine` servant `web/` + artefacts WASM compilés dans `web/public/`

- [ ] **Step 1: Créer le Dockerfile**

Créer `Dockerfile` à la racine avec ce contenu exact :

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

COPY web/                            /usr/share/nginx/html/
COPY --from=builder /src/web/public/ /usr/share/nginx/html/public/
COPY nginx/default.conf              /etc/nginx/conf.d/default.conf
```

- [ ] **Step 2: Vérifier la syntaxe du Dockerfile**

```bash
docker build --check .
```

Résultat attendu : pas d'erreurs de syntaxe. Si `--check` n'est pas disponible (Docker < 25), utiliser :

```bash
docker build --dry-run . 2>&1 || echo "dry-run non supporté, OK"
```

- [ ] **Step 3: Commit**

```bash
git add Dockerfile
git commit -m "feat: add multi-stage Dockerfile (kitware/vtk-wasm-sdk builder + nginx runner)"
```

---

### Task 4: Mettre à jour `docker-compose.yml` et valider le build complet

**Files:**
- Modify: `docker-compose.yml`

**Interfaces:**
- Consumes: `Dockerfile` (Task 3)
- Produces: workflow `docker compose up --build` fonctionnel

- [ ] **Step 1: Remplacer le contenu de `docker-compose.yml`**

```yaml
services:
  web:
    build: .
    ports:
      - "8080:80"
```

- [ ] **Step 2: Valider la configuration Compose**

```bash
docker compose config
```

Résultat attendu : YAML valide affiché, avec `build: context: .` et `ports: - 8080:80`.

- [ ] **Step 3: Lancer le build complet**

```bash
docker compose up --build
```

Le build tire `kitware/vtk-wasm-sdk:9.6.0` (~1.2 Go au premier pull), compile le C++ avec Emscripten, puis démarre nginx.

Signes de succès dans les logs :
- `cmake --build` complète sans erreur
- `vtk_app.mjs` et `vtk_app.wasm` sont générés
- `nginx:alpine` démarre et écoute sur le port 80

- [ ] **Step 4: Vérifier que l'application est accessible**

Dans un autre terminal :

```bash
curl -I http://localhost:8080
```

Résultat attendu :
```
HTTP/1.1 200 OK
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

Ouvrir `http://localhost:8080` dans un navigateur et vérifier que l'overlay "Déposez un fichier 3D ici" s'affiche.

- [ ] **Step 5: Arrêter et commit**

```bash
docker compose down
git add docker-compose.yml
git commit -m "feat: wire docker-compose to multi-stage Dockerfile build"
```
