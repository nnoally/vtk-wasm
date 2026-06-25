# VTK WASM — Démonstrateur Web Client

**Date :** 2026-06-25
**Statut :** Approuvé

## Objectif

Démonstrateur de faisabilité montrant l'intégration du SDK C++ VTK WASM officiel Kitware dans un client web. L'utilisateur peut déposer un fichier 3D par drag & drop pour l'afficher en 3D, et changer la couleur de l'objet via une palette. Le but est de rendre visible et lisible le pont entre C++ VTK et le navigateur.

---

## Architecture générale

Trois couches distinctes et intentionnellement visibles :

```
┌─────────────────────────────────────────┐
│  HTML/CSS/JS statique — UI              │
│  drag & drop, palette de couleurs       │
├─────────────────────────────────────────┤
│  bindings.cpp — API WASM exposée au JS  │
│  loadFile, setColor, init               │
├─────────────────────────────────────────┤
│  VtkApp.cpp — pipeline VTK C++          │
│  readers, mapper, actor, renderer       │
└─────────────────────────────────────────┘
```

Aucun framework frontend. Aucun Node.js. Le frontend est 100% statique, servi par nginx via Docker Compose.

---

## Structure du projet

```
vtk.wasm/
├── docker-compose.yml
├── nginx/
│   └── default.conf             ← headers COEP/COOP + serve static
├── CMakeLists.txt               ← build C++ → WASM via Emscripten
│
├── cpp/
│   ├── VtkApp.h                 ← classe principale VTK
│   ├── VtkApp.cpp               ← pipeline, readers, couleur
│   └── bindings.cpp             ← EMSCRIPTEN_BINDINGS
│
└── web/
    ├── index.html
    ├── src/
    │   ├── main.js              ← charge le module WASM, câble l'UI
    │   ├── drag-drop.js         ← drag & drop → wasm.loadFile()
    │   └── color-palette.js     ← palette → wasm.setColor()
    └── public/
        ├── vtk_app.mjs          ← généré par le build C++
        └── vtk_app.wasm         ← généré par le build C++
```

---

## Pipeline C++ VTK

### Classe `VtkApp`

```cpp
class VtkApp {
public:
    void init();
    void loadFile(const std::string& filename, const std::string& data);
    void setColor(double r, double g, double b);

private:
    vtkSmartPointer<vtkWebAssemblyOpenGLRenderWindow>       renderWindow;
    vtkSmartPointer<vtkRenderer>                            renderer;
    vtkSmartPointer<vtkWebAssemblyRenderWindowInteractor>   interactor;
    vtkSmartPointer<vtkActor>                               actor;
};
```

### `init()`

Crée le render window lié au canvas HTML ayant l'id `"vtk-canvas"`, le renderer (fond gris neutre), et l'interactor (rotation/zoom souris intégrés VTK).

### `loadFile(filename, data)`

1. Écrit les bytes reçus dans le FS virtuel Emscripten (`/tmp/<filename>`)
2. Détecte le format par extension de fichier
3. Instancie le reader VTK approprié :

| Extension | Reader VTK |
|---|---|
| `.stl` | `vtkSTLReader` |
| `.obj` | `vtkOBJReader` |
| `.ply` | `vtkPLYReader` |
| `.vtp` | `vtkXMLPolyDataReader` |
| `.vtu` | `vtkXMLUnstructuredGridReader` |

4. Connecte le pipeline :
   - Formats polydata (`.stl`, `.obj`, `.ply`, `.vtp`) : Reader → `vtkPolyDataMapper` → `vtkActor`
   - Format grille non structurée (`.vtu`) : Reader → `vtkGeometryFilter` → `vtkPolyDataMapper` → `vtkActor`
5. Ajoute l'actor au renderer, appelle `ResetCamera()` + `Render()`

Si l'extension n'est pas reconnue, retourne une erreur sans crash.

### `setColor(r, g, b)`

```cpp
actor->GetProperty()->SetColor(r, g, b);
renderWindow->Render();
```

### Bindings Emscripten (`bindings.cpp`)

```cpp
EMSCRIPTEN_BINDINGS(vtk_app) {
    emscripten::class_<VtkApp>("VtkApp")
        .constructor<>()
        .function("init",     &VtkApp::init)
        .function("loadFile", &VtkApp::loadFile)
        .function("setColor", &VtkApp::setColor);
}
```

---

## UI JavaScript

### Layout

```
┌─────────────────────────────────────────────┐
│  HEADER  (titre du démonstrateur)           │
├─────────────────────────────────────────────┤
│                                             │
│   ZONE 3D  (canvas VTK WASM)                │
│   — overlay "déposez un fichier" si vide    │
│   — mesh 3D interactif une fois chargé      │
│                                             │
├─────────────────────────────────────────────┤
│  ● ● ● ● ● ● ● ●   (palette 8 couleurs)    │
└─────────────────────────────────────────────┘
```

### États de l'application

| État | Overlay | Palette |
|---|---|---|
| `vide` | visible | désactivée |
| `chargé` | masqué | active |
| `erreur` | message d'erreur | inchangée |

### `drag-drop.js`

- Écoute `dragover` (highlight visuel) et `drop` sur la zone canvas
- Filtre visuellement les formats acceptés : `.stl .obj .ply .vtp .vtu`
- `FileReader.readAsArrayBuffer(file)` → conversion en string bytes
- Appelle `wasm.loadFile(file.name, bytes)`
- Passe l'état à `chargé` ou `erreur`

### `color-palette.js`

- 8 pastilles de couleur prédéfinies : blanc, gris, rouge, orange, jaune, vert, bleu, violet
- Désactivées tant qu'aucun fichier n'est chargé
- Clic → `wasm.setColor(r, g, b)` + highlight de la pastille active

### `main.js`

- Charge le module WASM (`vtk_app.mjs`)
- Instancie `VtkApp`, appelle `init()`
- Câble drag-drop et palette sur l'instance WASM
- Gère les transitions d'état

Chaque fichier JS fait moins de 60 lignes.

---

## Build

### Étape 1 — C++ → WASM

```bash
emcmake cmake -B build .
cmake --build build
# → web/public/vtk_app.mjs + web/public/vtk_app.wasm
```

Flags Emscripten essentiels :
- `--bind` — active les EMSCRIPTEN_BINDINGS
- `-sMODULARIZE` — génère un module ES importable
- `-sEXPORTED_RUNTIME_METHODS=FS` — accès au FS virtuel
- `-sENVIRONMENT=web` — cible navigateur uniquement

### Étape 2 — Lancer le démonstrateur

```bash
docker compose up
# → http://localhost:8080
```

### nginx — Headers requis

```nginx
add_header Cross-Origin-Opener-Policy   "same-origin";
add_header Cross-Origin-Embedder-Policy "require-corp";
```

Ces headers sont obligatoires pour `SharedArrayBuffer`, utilisé par VTK WASM.

---

## Prérequis système

| Outil | Usage |
|---|---|
| Emscripten SDK (`emsdk`) | Compiler C++ → WASM |
| VTK SDK Kitware (build WASM) | Headers + libs VTK pour Emscripten |
| Docker + Docker Compose | Servir le démonstrateur |

---

## Ce que ce démonstrateur prouve

1. VTK C++ tourne dans un navigateur via WebAssembly
2. L'API C++ est directement appelable depuis JS sans couche intermédiaire complexe
3. Des fichiers 3D réels peuvent être chargés et rendus interactivement
4. Les interactions (couleur) entre l'UI web et la scène VTK C++ fonctionnent en temps réel
