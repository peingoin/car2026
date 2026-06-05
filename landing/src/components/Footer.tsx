import { ArrowUpRight, GitFork } from 'lucide-react'

// TODO: point this at the real repository URL once published.
const REPO_URL = 'https://github.com/peingoin/car2026'

/** Climax CTA + credits. */
export default function Footer() {
  return (
    <footer
      id="build"
      className="bg-bg relative border-t border-white/10 px-6 py-28 sm:py-36"
    >
      <div className="mx-auto flex max-w-3xl flex-col items-center text-center">
        <p className="font-mono text-xs tracking-[0.35em] text-accent uppercase">
          Open source
        </p>
        <h2 className="mt-5 font-display text-4xl leading-[1.05] font-semibold tracking-tight text-white/90 sm:text-6xl">
          Build it yourself.
        </h2>
        <p className="mt-5 max-w-md text-balance text-white/60">
          Firmware for the ESP32 and Arduino, plus the web controller. Wire it
          up, flash it, and drive in an afternoon.
        </p>

        <a
          href={REPO_URL}
          target="_blank"
          rel="noopener noreferrer"
          className="bg-accent group mt-10 inline-flex cursor-pointer items-center gap-2 rounded-full px-7 py-3.5 font-medium text-white transition-transform duration-200 hover:scale-[1.02]"
        >
          See how it&rsquo;s built
          <ArrowUpRight
            size={18}
            className="transition-transform duration-200 group-hover:translate-x-0.5 group-hover:-translate-y-0.5"
            aria-hidden
          />
        </a>

        <a
          href={REPO_URL}
          target="_blank"
          rel="noopener noreferrer"
          className="mt-6 inline-flex cursor-pointer items-center gap-2 text-sm text-white/40 transition-colors duration-200 hover:text-white/70"
        >
          <GitFork size={16} aria-hidden />
          View source
        </a>

        <p className="mt-16 font-mono text-xs tracking-[0.3em] text-white/50 uppercase">
          Built by Austin Cheng &amp; Charlie Gao
        </p>
        <p className="mt-2 text-xs text-white/30">© 2026 RC-CAR</p>
      </div>
    </footer>
  )
}
