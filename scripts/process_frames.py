"""
Preprocess animation frames for the scrollytelling landing page.

The raw frames in ../animation/ezgif-frame-###.png are RGB with a baked-in
gray checkerboard background (no real transparency). This script uses `rembg`
(an AI matting model) to cut the subject out cleanly and saves the result as
transparent WebP into ../landing/public/frames/, so the frames composite
seamlessly over the dark page background.

Usage:
    python scripts/process_frames.py

Requires:
    pip install "rembg[cpu]" pillow
"""

from __future__ import annotations

import io
import sys
import time
from pathlib import Path

from PIL import Image
from rembg import remove, new_session

# --- config ---------------------------------------------------------------
ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = ROOT / "animation"
OUT_DIR = ROOT / "landing" / "public" / "frames"

STEP = 1            # 1 = keep all 240 frames; set 2 to decimate to ~120 for a lighter payload
WEBP_QUALITY = 80   # transparent WebP quality
MODEL = "u2netp"    # lighter, ~4x faster model; slightly softer hair edges


def save_webp(img: Image.Image, out: Path) -> bool:
    """Encode WebP, retrying with cheaper settings on libwebp errors.

    libwebp can raise "encoding error 1" for some frames at high method/quality
    (and OneDrive can momentarily lock the output file). Retry, then fall back.
    """
    attempts = [
        {"quality": WEBP_QUALITY, "method": 4},
        {"quality": WEBP_QUALITY, "method": 0},
        {"quality": 75, "method": 0, "lossless": False},
    ]
    last: Exception | None = None
    for opts in attempts:
        for _ in range(2):  # small retry for transient file locks
            try:
                img.save(out, "WEBP", **opts)
                return True
            except Exception as e:  # noqa: BLE001
                last = e
                time.sleep(0.3)
    print(f"  WARN could not encode {out.name}: {last}", file=sys.stderr)
    return False


def main() -> int:
    frames = sorted(SRC_DIR.glob("ezgif-frame-*.png"))
    if not frames:
        print(f"No frames found in {SRC_DIR}", file=sys.stderr)
        return 1

    frames = frames[::STEP]
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    session = new_session(MODEL)
    total = len(frames)
    print(f"Matting {total} frames ({MODEL}) -> {OUT_DIR}")
    start = time.perf_counter()

    skipped = 0
    for i, src in enumerate(frames, start=1):
        out = OUT_DIR / f"frame-{i:03d}.webp"

        # Resume: skip frames already encoded and verifiably valid.
        if out.exists() and out.stat().st_size > 0:
            try:
                with Image.open(out) as chk:
                    chk.verify()
                skipped += 1
                continue
            except Exception:
                pass  # corrupt/partial -> re-encode below

        with open(src, "rb") as f:
            cut = remove(f.read(), session=session)  # bytes -> RGBA PNG bytes
        img = Image.open(io.BytesIO(cut)).convert("RGBA")
        save_webp(img, out)

        if i == 1 or i % 10 == 0 or i == total:
            elapsed = time.perf_counter() - start
            rate = i / elapsed if elapsed else 0
            eta = (total - i) / rate if rate else 0
            print(f"  {i:3d}/{total}  ({rate:4.1f} fps, ETA {eta:5.0f}s)")

    print(
        f"Done: {total} frames ({skipped} skipped) "
        f"in {time.perf_counter() - start:.0f}s -> {OUT_DIR}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
