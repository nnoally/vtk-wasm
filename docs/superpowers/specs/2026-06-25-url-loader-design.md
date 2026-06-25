# URL Loader — Chargement automatique via paramètre URL

**Date :** 2026-06-25
**Statut :** Approuvé

## Objectif

Permettre de charger automatiquement un fichier 3D au démarrage de l'application en passant son URL dans le paramètre `?file=` de la page. Supporte l'authentification Basic Auth via le schéma `user:pass@host` dans l'URL.

---

## Architecture

Nouveau module `web/src/url-loader.js`, symétrique à `drag-drop.js`. Appelé depuis `main.js` après initialisation du module WASM.

```
URL de la page : http://localhost:8080?file=https://user:pass@host/model.stl
                                              └──────────────────────────────┘
                                              paramètre lu par url-loader.js

url-loader.js
├── parse ?file= depuis window.location.search
├── extrait user:pass via regex (avant URL())
├── fetch(cleanUrl, Basic Auth header si credentials)
├── arrayBuffer → Uint8Array
├── FS.writeFile('/tmp/<filename>', bytes)
├── app.loadFile('/tmp/<filename>')
└── setState('charge') ou setState('erreur', message)
```

---

## Fichiers modifiés

| Fichier | Changement |
|---|---|
| `web/src/url-loader.js` | Nouveau module |
| `web/src/main.js` | Appel `initUrlLoader(app, module.FS)` |

Aucun changement côté C++, nginx, ou Docker.

---

## Module `url-loader.js`

### Signature

```js
export function initUrlLoader(app, FS)
```

Appelée une seule fois depuis `main.js` après `app.init()`. Si aucun paramètre `?file=` n'est présent, retourne immédiatement sans effet.

### Extraction des credentials

`new URL()` tronque silencieusement les credentials — ils sont donc extraits via regex sur la chaîne brute **avant** toute construction d'objet URL :

```
https://user:pass@example.com/model.stl
        └──────┘ → { user: "user", pass: "pass" }
https://example.com/model.stl  ← URL nettoyée pour le fetch
```

Pattern : `/^(https?:\/\/)([^:@]+):([^@]+)@(.+)$/`

### Fetch

```js
fetch(cleanUrl, {
    headers: credentials ? { Authorization: 'Basic ' + btoa(user + ':' + pass) } : {}
})
```

Le header `Authorization` est omis si pas de credentials.

### Nom de fichier

Extrait du dernier segment du pathname de l'URL nettoyée :

```
https://example.com/models/bunny.stl → "bunny.stl"
```

Utilisé pour l'écriture dans le FS virtuel (`/tmp/bunny.stl`) et la validation d'extension.

### Validation

Mêmes extensions acceptées que `drag-drop.js` : `.stl .obj .ply .vtp .vtu`

Si l'extension n'est pas reconnue → `setState('erreur', ...)` sans appel WASM.

### Gestion d'erreurs

| Cas | Message affiché |
|---|---|
| Extension non supportée | "Format non supporté : .xxx" |
| Erreur réseau / CORS | "Impossible de charger le fichier distant" |
| Réponse HTTP non-ok (401, 404…) | "Erreur HTTP <status> : <url>" |
| Erreur lecture buffer | message natif de l'erreur |

---

## Intégration dans `main.js`

```js
import { initUrlLoader } from './url-loader.js';

createVtkModule().then(module => {
    const app = new module.VtkApp();
    app.init();
    initDragDrop(app, module.FS);
    initColorPalette(app);
    initUrlLoader(app, module.FS);   // ← ajout
    try { app.start(); } catch(e) { if (String(e) !== 'unwind') throw e; }
});
```

L'ordre est intentionnel : `initUrlLoader` après `app.start()` n'est pas nécessaire, mais après `app.init()` oui — le renderer doit être prêt avant `loadFile`.

---

## Exemples d'usage

```
# Fichier public
http://localhost:8080?file=https://example.com/model.stl

# Avec authentification Basic Auth
http://localhost:8080?file=https://user:pass@private.example.com/model.stl
```

---

## Contraintes et limitations

**CORS :** Le serveur distant doit autoriser les requêtes cross-origin (`Access-Control-Allow-Origin`). Si ce n'est pas le cas, le navigateur bloque le fetch et l'application affiche une erreur. Aucun contournement côté frontend possible — le serveur distant doit configurer ses headers CORS.

**Credentials en clair dans l'URL :** Le schéma `user:pass@host` expose les credentials dans l'historique du navigateur et les logs serveur. C'est acceptable dans un contexte de démonstrateur, mais à éviter en production.

**Taille des fichiers :** Pas de limite imposée côté JS — la contrainte est la mémoire WASM disponible dans le navigateur.
