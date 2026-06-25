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
