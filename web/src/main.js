import createVtkModule from '../public/vtk_app.mjs';
import { initDragDrop }    from './drag-drop.js';
import { initColorPalette } from './color-palette.js';

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
}).catch(err => {
    console.error('WASM load failed:', err);
    setState('erreur', 'Impossible de charger le module VTK WASM.');
});
