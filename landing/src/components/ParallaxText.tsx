import { motion, useTransform, type MotionValue } from 'motion/react'
import type { ReactNode } from 'react'

interface Props {
  progress: MotionValue<number>
  /** [enter, exit] in 0..1 scroll progress of the sequence section. */
  range: [number, number]
  reduce: boolean
  children: ReactNode
  className?: string
}

/** A scroll-ranged overlay that fades + parallaxes in and out. */
export default function ParallaxText({
  progress,
  range,
  reduce,
  children,
  className = '',
}: Props) {
  const [start, end] = range
  const span = end - start
  const fadeIn = start + span * 0.18
  const fadeOut = end - span * 0.18

  const opacity = useTransform(
    progress,
    [start, fadeIn, fadeOut, end],
    [0, 1, 1, 0],
  )
  const y = useTransform(
    progress,
    [start, end],
    reduce ? [0, 0] : [50, -50],
  )

  return (
    <motion.div
      style={{ opacity, y }}
      className={`absolute inset-0 flex flex-col items-center justify-center px-6 text-center ${className}`}
    >
      {children}
    </motion.div>
  )
}
