# Rotation interactive souris — VTK WASM

**Date :** 2026-06-25
**Statut :** Approuvé

## Objectif

Activer la rotation interactive de la scène 3D via la souris. L'utilisateur peut faire tourner l'objet chargé en cliquant-glissant sur le canvas, zoomer au scroll, et panner au clic milieu.

---

## Diagnostic

La cause racine est l'absence d'un **style d'interaction** explicite sur le `vtkWebAssemblyRenderWindowInteractor`. Sans style, les événements DOM capturés par l'interactor ne se traduisent en aucun mouvement de caméra. Le canvas et le CSS ne sont pas en cause (`pointer-events` est correct).

---

## Changements

### C++ — `VtkApp.cpp` / `VtkApp.h`

Ajouter `vtkInteractorStyleTrackballCamera` dans `init()` :

```cpp
#include <vtkInteractorStyleTrackballCamera.h>

// dans VtkApp::init(), après interactor->Initialize() :
interactor->SetInteractorStyle(
    vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New()
);
```

Comportement activé :

| Geste | Action |
|---|---|
| Clic gauche + glisser | Rotation orbitale autour de l'objet |
| Scroll ou clic droit + glisser | Zoom |
| Clic milieu + glisser | Pan |

Aucun nouveau binding Emscripten. Aucun changement JS.

### UI — `index.html`

Ajouter un hint discret dans `main`, visible uniquement après chargement d'un fichier (état `charge`) :

```html
<div id="rotation-hint">Clic + glisser pour tourner</div>
```

Style : position absolue en bas du canvas, texte petit gris, `pointer-events: none`. Affiché via `hidden` toggleé par `setState()` dans `main.js`.

### JS — `main.js`

Étendre `setState()` pour afficher/masquer le hint :

```js
const hint = document.getElementById('rotation-hint');
// dans setState :
hint.hidden = newState !== 'charge';
```

---

## Périmètre

- Pas de nouveau binding C++
- Pas de boucle requestAnimationFrame
- Pas de contrôle JS sur la caméra
- Pas de boutons de rotation

---

## Fichiers modifiés

| Fichier | Changement |
|---|---|
| `cpp/VtkApp.cpp` | +1 include, +1 appel `SetInteractorStyle` dans `init()` |
| `web/index.html` | +1 élément `#rotation-hint` + style CSS |
| `web/src/main.js` | +2 lignes dans `setState()` |
