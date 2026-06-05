# RC-CAR — Scrollytelling landing page

A high-end, scroll-linked landing page for the phone-controlled RC car. Scrolling
scrubs a 240-frame image sequence on an HTML5 canvas (Apple-style), with parallax
story chapters over a pure-dark background.

**Stack:** React + Vite + TypeScript · Tailwind CSS v4 · Motion (Framer Motion) · HTML5 Canvas

## Develop

```bash
npm install
npm run dev      # http://localhost:5173
```

## Build

```bash
npm run build
npm run preview
```

## Frames

The canvas sequence reads transparent WebP frames from `public/frames/frame-001.webp …`.
These are generated from the raw `../animation/*.png` (which have a baked-in checkerboard
background) by the AI-matting script at the repo root:

```bash
# from the repo root
pip install "rembg[cpu]" pillow
python scripts/process_frames.py
```

The script cuts the subject out cleanly and writes transparent WebP into
`landing/public/frames/`. If you change the number of frames, update `FRAME_COUNT`
in `src/lib/frames.ts`.

## Architecture

| File | Role |
|------|------|
| `src/App.tsx` | Composition: ProgressBar · ScrollSequence · Footer · Loader |
| `src/hooks/useImageSequence.ts` | Preloads + decodes all frames, reports progress |
| `src/components/ScrollSequence.tsx` | Tall section + pinned canvas; scroll→frame scrub via rAF (no React re-renders) |
| `src/components/Hero.tsx` | Intro hook ("Scroll to explore") |
| `src/components/Chapters.tsx` + `ParallaxText.tsx` | Scroll-ranged story overlays |
| `src/components/ProgressBar.tsx` | Top scroll-progress indicator |
| `src/components/Loader.tsx` | Preload gate with % |
| `src/components/Footer.tsx` | Climax CTA + credits |

### Notes
- **Performance:** the draw loop talks directly to the canvas via refs; React never
  re-renders during scroll. Spring smoothing gives the buttery scrub feel.
- **Accessibility:** respects `prefers-reduced-motion` (disables spring/parallax/bounce),
  skip-link, focus rings, canvas `role="img"` + label, semantic landmarks.
- **Theming:** semantic color/font tokens live in `src/index.css` (`@theme`).
- Update `REPO_URL` in `src/components/Footer.tsx` to the published repository.
