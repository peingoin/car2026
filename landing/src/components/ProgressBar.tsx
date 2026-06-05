import { motion, useScroll, useSpring } from 'motion/react'

/** Thin scroll-progress indicator across the top of the page. */
export default function ProgressBar() {
  const { scrollYProgress } = useScroll()
  const scaleX = useSpring(scrollYProgress, {
    stiffness: 120,
    damping: 30,
    mass: 0.4,
  })

  return (
    <motion.div
      aria-hidden
      style={{ scaleX }}
      className="fixed top-0 left-0 z-50 h-0.5 w-full origin-left bg-accent"
    />
  )
}
