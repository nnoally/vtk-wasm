const COLORS = [
    { name: 'blanc',  rgb: [1.00, 1.00, 1.00] },
    { name: 'gris',   rgb: [0.50, 0.50, 0.50] },
    { name: 'rouge',  rgb: [0.80, 0.10, 0.10] },
    { name: 'orange', rgb: [0.90, 0.50, 0.10] },
    { name: 'jaune',  rgb: [0.90, 0.90, 0.10] },
    { name: 'vert',   rgb: [0.10, 0.70, 0.10] },
    { name: 'bleu',   rgb: [0.10, 0.30, 0.90] },
    { name: 'violet', rgb: [0.60, 0.10, 0.80] },
];

export function initColorPalette(app) {
    const palette = document.getElementById('color-palette');

    COLORS.forEach(({ name, rgb }) => {
        const btn = document.createElement('button');
        btn.title = name;
        btn.setAttribute('aria-label', name);
        btn.style.background =
            `rgb(${rgb.map(v => Math.round(v * 255)).join(',')})`;

        btn.addEventListener('click', () => {
            palette.querySelectorAll('button')
                   .forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            app.setColor(...rgb);
        });

        palette.appendChild(btn);
    });
}
