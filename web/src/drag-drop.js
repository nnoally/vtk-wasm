import { setState } from './main.js';

export const ACCEPTED = ['.stl', '.obj', '.ply', '.vtp', '.vtu'];

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
        }).catch(err => {
            setState('erreur', err.message || 'Erreur de lecture du fichier');
        });
    });
}
