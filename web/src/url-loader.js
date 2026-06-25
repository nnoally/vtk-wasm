import { setState } from './main.js';
import { ACCEPTED } from './drag-drop.js';

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
