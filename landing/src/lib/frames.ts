// Manifest for the scroll-scrubbed image sequence.
// Frames are produced by scripts/process_frames.py into public/frames/.

export const FRAME_COUNT = 240
export const FRAME_WIDTH = 405
export const FRAME_HEIGHT = 720

export const frameUrls: string[] = Array.from(
  { length: FRAME_COUNT },
  (_, i) =>
    `${import.meta.env.BASE_URL}frames/frame-${String(i + 1).padStart(3, '0')}.webp`,
)
