# URL Loader Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Charger automatiquement un fichier 3D au démarrage en passant son URL dans le paramètre `?file=` de la page, avec support optionnel de Basic Auth via `user:pass@host`.

**Architecture:** Nouveau module `web/src/url-loader.js` symétrique à `drag-drop.js`. Au démarrage, lit `?file=` depuis `window.location.search`, extrait les éventuels credentials via regex, fetche le fichier, l'injecte dans le FS virtuel Emscripten, puis appelle `app.loadFile()`. Intégré dans `main.js` après `app.start()`.

**Tech Stack:** JavaScript ES modules vanilla (pas de bundler, pas de npm), API Fetch, Emscripten FS virtuel, `btoa()` pour Basic Auth.

## Global Constraints

- Aucun framework frontend, aucun Node.js, aucun npm — JS vanilla uniquement
- Extensions acceptées : `.stl .obj .ply .vtp .vtu` (identiques à `drag-drop.js`)
- Les fichiers JS doivent rester sous ~60 lignes chacun
- Pas de modification du code C++, nginx, ou Docker
- `setState` est importé depuis `main.js` (pattern déjà établi par `drag-drop.js`)

---

### Task 1: Créer `web/src/url-loader.js`

**Files:**
- Create: `web/src/url-loader.js`

**Interfaces:**
- Consumes: `setState(state, message)` depuis `./main.js`
- Produces: `initUrlLoader(app, FS)` — appelée une fois depuis `main.js` après `app.start()`

- [ ] **Étape 1 : Créer le fichier `web/src/url-loader.js` avec le contenu suivant**

```js
import { setState } from './main.js';

const ACCEPTED = ['.stl', '.obj', '.ply', '.vtp', '.vtu'];

function parseFileUrl(raw) {
    const match = raw.match(/^(https?:\/\/)([^:@\/]+):([^@]+)@(.+)$/);
    if (match) {
        return {
            url: match[1] + match[4],
            credentials: { user: match[2], pass: match[3] },
        };
    }
    return { url: raw, credentials: null };
}

export function initUrlLoader(app, FS) {
    const param = new URLSearchParams(window.location.search).get('file');
    if (!param) return;

    const { url, credentials } = parseFileUrl(param);

    const filename = url.split('/').pop().split('?')[0];
    const ext = filename.slice(filename.lastIndexOf('.')).toLowerCase();
    if (!ACCEPTED.includes(ext)) {
        setState('erreur', `Format non supporté : ${ext}. Acceptés : ${ACCEPTED.join(' ')}`);
        return;
    }

    const headers = credentials
        ? { Authorization: 'Basic ' + btoa(credentials.user + ':' + credentials.pass) }
        : {};

    fetch(url, { headers })
        .then(res => {
            if (!res.ok) throw new Error(`Erreur HTTP ${res.status} : ${url}`);
            return res.arrayBuffer();
        })
        .then(buffer => {
            const path = '/tmp/' + filename;
            FS.writeFile(path, new Uint8Array(buffer));
            app.loadFile(path);
            setState('charge');
        })
        .catch(err => {
            setState('erreur', err.message || 'Impossible de charger le fichier distant');
        });
}
```

- [ ] **Étape 2 : Vérifier manuellement — module isolé**

Ouvrir la console navigateur sur `http://localhost:8080` et exécuter :

```js
// Vérifier que le module est bien importable (après avoir lancé docker compose up)
// La présence du module dans le bundle ES est vérifiée à l'étape suivante
// lors de l'intégration dans main.js
console.log('Fichier créé, prêt pour intégration');
```

- [ ] **Étape 3 : Commit**

```bash
git add web/src/url-loader.js
git commit -m "feat: add url-loader module for auto-loading 3D files from ?file= param"
```

---

### Task 2: Intégrer `url-loader.js` dans `main.js`

**Files:**
- Modify: `web/src/main.js`

**Interfaces:**
- Consumes: `initUrlLoader(app, FS)` depuis `./url-loader.js` (définie en Task 1)
- Produces: rien — câblage final de l'application

- [ ] **Étape 1 : Modifier `web/src/main.js`**

Ajouter l'import en haut du fichier et l'appel après `app.start()`.

Fichier complet après modification :

```js
import createVtkModule from '../public/vtk_app.mjs';
import { initDragDrop }    from './drag-drop.js';
import { initColorPalette } from './color-palette.js';
import { initUrlLoader }    from './url-loader.js';

const overlay  = document.getElementById('drop-overlay');
const errMsg   = document.getElementById('error-message');
const palette  = document.getElementById('color-palette');
const hint     = document.getElementById('rotation-hint');

export function setState(newState, message = '') {
    overlay.hidden  = newState === 'charge';
    errMsg.hidden   = newState !== 'erreur';
    if (message) errMsg.textContent = message;
    palette.classList.toggle('disabled', newState !== 'charge');
    hint.hidden = newState !== 'charge';
}

createVtkModule().then(module => {
    const app = new module.VtkApp();
    app.init();
    initDragDrop(app, module.FS);
    initColorPalette(app);
    try { app.start(); } catch(e) { if (String(e) !== 'unwind') throw e; }
    initUrlLoader(app, module.FS);
}).catch(err => {
    console.error('WASM load failed:', err);
    setState('erreur', 'Impossible de charger le module VTK WASM.');
});
```

`initUrlLoader` est appelé après `app.start()` pour garantir que le renderer VTK est opérationnel avant tout appel à `loadFile`.

- [ ] **Étape 2 : Vérifier manuellement — cas sans paramètre**

```bash
docker compose up
```

Ouvrir `http://localhost:8080` — l'overlay "Déposez un fichier 3D ici" doit s'afficher normalement. Aucune erreur dans la console.

- [ ] **Étape 3 : Vérifier manuellement — URL valide publique**

Utiliser un fichier STL public accessible en CORS (ou héberger temporairement un fichier sur le nginx local via `docker compose exec nginx cp /path/model.stl /usr/share/nginx/html/`).

```
http://localhost:8080?file=http://localhost:8080/model.stl
```

Résultat attendu : le fichier se charge automatiquement, l'overlay disparaît, le modèle 3D s'affiche.

- [ ] **Étape 4 : Vérifier manuellement — extension non supportée**

```
http://localhost:8080?file=http://localhost:8080/model.xyz
```

Résultat attendu : message d'erreur rouge "Format non supporté : .xyz. Acceptés : .stl .obj .ply .vtp .vtu"

- [ ] **Étape 5 : Vérifier manuellement — URL inaccessible**

```
http://localhost:8080?file=http://localhost:9999/inexistant.stl
```

Résultat attendu : message d'erreur rouge "Impossible de charger le fichier distant" (ou message réseau natif).

- [ ] **Étape 6 : Vérifier manuellement — Basic Auth**

Si un serveur avec Basic Auth est disponible :

```
http://localhost:8080?file=https://user:pass@private-server/model.stl
```

Résultat attendu : le header `Authorization: Basic dXNlcjpwYXNz` est visible dans les DevTools Network, le fichier se charge.

- [ ] **Étape 7 : Commit**

```bash
git add web/src/main.js
git commit -m "feat: wire url-loader into main.js for auto-load on startup"
```
