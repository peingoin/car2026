import { motion, useTransform, type MotionValue } from 'motion/react'
import { ChevronDown } from 'lucide-react'

interface Props {
  progress: MotionValue<number>
  reduce: boolean
}

/**
 * Intro hook overlay. The headline shows at the start, fully disappears through
 * the middle of the spin, then returns at the very end as a bookend. The
 * "Scroll to explore" prompt only appears at the start.
 */
export default function Hero({ progress, reduce }: Props) {
  // Visible at start -> gone through the middle -> back at the end.
  const textOpacity = useTransform(progress, [0, 0.08, 0.93, 1], [1, 0, 0, 1])
  // Exit upward on intro; re-enter from below at the end (no drift for reduced motion).
  const textY = useTransform(
    progress,
    [0, 0.08, 0.93, 1],
    reduce ? [0, 0, 0, 0] : [0, -40, 40, 0],
  )

  // Scroll prompt: only at the very start, never returns.
  const promptOpacity = useTransform(progress, [0, 0.06], [1, 0])

  return (
    <div className="absolute inset-0">
      <motion.div
        style={{ opacity: textOpacity, y: textY }}
        className="absolute inset-0 flex flex-col items-center justify-center px-6 text-center"
      >
        <p className="font-mono text-xs uppercase tracking-[0.4em] text-accent">
          RC-CAR
        </p>
        <h1 className="mt-6 max-w-4xl font-display text-5xl leading-[0.95] font-semibold tracking-tight text-white/90 sm:text-7xl md:text-8xl">
          Drive from <span className="text-white/50 italic">any</span> phone.
        </h1>
        <p className="mt-6 max-w-md text-balance text-base text-white/60 sm:text-lg">
          No app. No internet. Just open a browser and go.
        </p>
      </motion.div>

      <motion.div
        style={{ opacity: promptOpacity }}
        className="absolute inset-x-0 bottom-10 flex flex-col items-center gap-2 text-white/40"
      >
        <span className="font-mono text-[10px] uppercase tracking-[0.3em]">
          Scroll to explore
        </span>
        <motion.span
          animate={reduce ? undefined : { y: [0, 6, 0] }}
          transition={{ duration: 1.6, repeat: Infinity, ease: 'easeInOut' }}
        >
          <ChevronDown size={18} aria-hidden />
        </motion.span>
      </motion.div>
    </div>
  )
}
