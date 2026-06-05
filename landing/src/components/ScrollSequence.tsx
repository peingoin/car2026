import { useEffect, useRef } from 'react'
import {
  useScroll,
  useSpring,
  useTransform,
  useReducedMotion,
} from 'motion/react'
import { FRAME_COUNT, FRAME_WIDTH, FRAME_HEIGHT } from '../lib/frames'
import Hero from './Hero'
import Chapters from './Chapters'

interface Props {
  images: React.RefObject<HTMLImageElement[]>
  ready: boolean
}

/**
 * The heart of the page: a tall section with a pinned full-viewport canvas.
 * Scroll progress (0..1) is mapped to a frame index and painted on the canvas
 * via a requestAnimationFrame loop — React never re-renders during scroll.
 */
export default function ScrollSequence({ images }: Props) {
  const sectionRef = useRef<HTMLDivElement>(null)
  const canvasRef = useRef<HTMLCanvasElement>(null)
  const reduce = useReducedMotion()

  const { scrollYProgress } = useScroll({
    target: sectionRef,
    offset: ['start start', 'end end'],
  })

  // Spring-smooth the scroll so quick jumps ease in (skipped for reduced motion).
  const smooth = useSpring(
    scrollYProgress,
    reduce
      ? { stiffness: 1000, damping: 100 }
      : { stiffness: 130, damping: 30, mass: 0.6 },
  )
  const frame = useTransform(smooth, [0, 1], [0, FRAME_COUNT - 1])

  // Horizontal pan of the subject, normalized: -1 = flush left, 0 = center, 1 = flush right.
  // Short transitions between long holds: center -> right -> center -> left -> center.
  const xShift = useTransform(
    smooth,
    [0, 0.09, 0.16, 0.31, 0.38, 0.57, 0.64, 0.87, 0.94, 1],
    reduce
      ? [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
      : [0, 0, 1, 1, 0, 0, -1, -1, 0, 0],
  )

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas) return
    const ctx = canvas.getContext('2d')
    if (!ctx) return

    let raf = 0
    let lastIndex = -1
    let lastX = Number.NaN
    let cw = 0
    let ch = 0

    const resize = () => {
      const dpr = Math.min(window.devicePixelRatio || 1, 2)
      const rect = canvas.getBoundingClientRect()
      cw = rect.width
      ch = rect.height
      canvas.width = Math.round(cw * dpr)
      canvas.height = Math.round(ch * dpr)
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0)
      lastIndex = -1 // force a redraw at the new size
      lastX = Number.NaN
    }

    const draw = () => {
      const imgs = images.current
      const idx = Math.max(
        0,
        Math.min(FRAME_COUNT - 1, Math.round(frame.get())),
      )
      const img = imgs?.[idx]
      if (img && img.complete && img.naturalWidth) {
        const iw = img.naturalWidth || FRAME_WIDTH
        const ih = img.naturalHeight || FRAME_HEIGHT
        // contain-fit with a little breathing room
        const scale = Math.min(cw / iw, ch / ih) * 0.92
        const dw = iw * scale
        const dh = ih * scale
        // Map normalized pan (-1..1) to the full available horizontal space,
        // so the subject reaches the edge without clipping off-screen.
        // Clamp because the spring can overshoot past ±1 on fast scrolls.
        const maxTravel = (cw - dw) / 2
        const dx = Math.round(
          Math.max(-maxTravel, Math.min(maxTravel, xShift.get() * maxTravel)),
        )
        if (idx !== lastIndex || dx !== lastX) {
          lastIndex = idx
          lastX = dx
          ctx.clearRect(0, 0, cw, ch)
          ctx.drawImage(img, (cw - dw) / 2 + dx, (ch - dh) / 2, dw, dh)
        }
      }
      raf = requestAnimationFrame(draw)
    }

    resize()
    window.addEventListener('resize', resize)
    raf = requestAnimationFrame(draw)

    return () => {
      cancelAnimationFrame(raf)
      window.removeEventListener('resize', resize)
    }
  }, [images, frame, xShift])

  return (
    <section
      ref={sectionRef}
      className="relative h-[500vh]"
      aria-label="Product showcase: a 360° view of the RC car's phone controller"
    >
      <div className="sticky top-0 h-[100svh] overflow-hidden">
        <canvas
          ref={canvasRef}
          className="absolute inset-0 h-full w-full"
          role="img"
          aria-label="A person wearing the RC car's controller headset, rotating 360 degrees as the page scrolls."
        />
        {/* Vignette for depth + text legibility */}
        <div className="pointer-events-none absolute inset-0 bg-[radial-gradient(ellipse_at_center,transparent_50%,rgba(0,0,0,0.65))]" />
        <Hero progress={scrollYProgress} reduce={!!reduce} />
        <Chapters progress={scrollYProgress} reduce={!!reduce} />
      </div>
    </section>
  )
}
