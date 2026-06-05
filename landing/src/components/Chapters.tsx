import { type MotionValue } from 'motion/react'
import { Smartphone, WifiOff, ShieldCheck } from 'lucide-react'
import ParallaxText from './ParallaxText'

interface Props {
  progress: MotionValue<number>
  reduce: boolean
}

const specs = [
  { icon: Smartphone, label: 'Any phone', value: 'iPhone · Android' },
  { icon: WifiOff, label: 'No internet', value: 'Works anywhere' },
  { icon: ShieldCheck, label: 'Failsafe', value: '~300ms stop' },
]

/** The narrative chapters layered over the spinning subject. */
export default function Chapters({ progress, reduce }: Props) {
  return (
    <>
      {/* Chapter 1 — the problem */}
      <ParallaxText progress={progress} range={[0.13, 0.34]} reduce={reduce}>
        <p className="font-mono text-xs uppercase tracking-[0.35em] text-accent">
          The problem
        </p>
        <h2 className="mt-5 max-w-2xl font-display text-4xl leading-tight font-semibold tracking-tight text-white/90 sm:text-6xl">
          Most RC cars fight you.
        </h2>
        <p className="mt-5 max-w-md text-balance text-white/60 sm:text-lg">
          An app to install. A router to configure. Lag to forgive.
        </p>
      </ParallaxText>

      {/* Chapter 2 — the journey */}
      <ParallaxText progress={progress} range={[0.4, 0.6]} reduce={reduce}>
        <p className="font-mono text-xs uppercase tracking-[0.35em] text-accent">
          The idea
        </p>
        <h2 className="mt-5 max-w-2xl font-display text-4xl leading-tight font-semibold tracking-tight text-white/90 sm:text-6xl">
          Your phone is the controller.
        </h2>
        <p className="mt-5 max-w-lg text-balance text-white/60 sm:text-lg">
          It joins the car&rsquo;s own WiFi hotspot and opens a web page. No
          download, no signal required.
        </p>
      </ParallaxText>

      {/* Chapter 3 — the solution / specs */}
      <ParallaxText progress={progress} range={[0.66, 0.92]} reduce={reduce}>
        <p className="font-mono text-xs uppercase tracking-[0.35em] text-accent">
          The build
        </p>
        <h2 className="mt-5 max-w-2xl font-display text-4xl leading-tight font-semibold tracking-tight text-white/90 sm:text-6xl">
          ESP32 brains. Tank steering.
        </h2>

        <ul className="mt-10 grid w-full max-w-xl grid-cols-1 gap-4 sm:grid-cols-3">
          {specs.map(({ icon: Icon, label, value }) => (
            <li
              key={label}
              className="flex flex-col items-center gap-2 rounded-2xl border border-white/10 bg-white/[0.02] px-4 py-5 backdrop-blur-sm"
            >
              <Icon size={22} className="text-accent" aria-hidden />
              <span className="text-sm font-medium text-white/90">{label}</span>
              <span className="tabular text-xs text-white/50">{value}</span>
            </li>
          ))}
        </ul>

        <a
          href="#build"
          className="mt-9 inline-flex cursor-pointer items-center gap-1.5 font-mono text-xs uppercase tracking-[0.25em] text-white/60 transition-colors duration-200 hover:text-white/90"
        >
          See the build →
        </a>
      </ParallaxText>
    </>
  )
}
