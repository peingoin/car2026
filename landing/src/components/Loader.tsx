import { AnimatePresence, motion } from 'motion/react'

interface Props {
  progress: number
  ready: boolean
}

/** Full-screen preloader gating the experience until frames are decoded. */
export default function Loader({ progress, ready }: Props) {
  const pct = Math.round(progress * 100)

  return (
    <AnimatePresence>
      {!ready && (
        <motion.div
          className="bg-bg fixed inset-0 z-[100] flex flex-col items-center justify-center"
          initial={{ opacity: 1 }}
          exit={{ opacity: 0 }}
          transition={{ duration: 0.6, ease: 'easeOut' }}
        >
          <div
            className="tabular text-2xl text-white/90"
            role="status"
            aria-live="polite"
          >
            {pct}%
          </div>
          <div className="mt-4 h-px w-48 overflow-hidden bg-white/10">
            <div
              className="bg-accent h-full transition-[width] duration-200 ease-out"
              style={{ width: `${pct}%` }}
            />
          </div>
          <div className="mt-6 font-mono text-[10px] tracking-[0.3em] text-white/40 uppercase">
            Loading experience
          </div>
        </motion.div>
      )}
    </AnimatePresence>
  )
}
