import { useImageSequence } from './hooks/useImageSequence'
import { frameUrls } from './lib/frames'
import ScrollSequence from './components/ScrollSequence'
import Footer from './components/Footer'
import ProgressBar from './components/ProgressBar'
import Loader from './components/Loader'

export default function App() {
  const seq = useImageSequence(frameUrls)

  return (
    <>
      <a
        href="#main"
        className="focus:bg-accent sr-only focus:fixed focus:top-4 focus:left-4 focus:z-[110] focus:not-sr-only focus:rounded focus:px-4 focus:py-2 focus:text-white"
      >
        Skip to content
      </a>

      <ProgressBar />

      <main id="main">
        <ScrollSequence images={seq.images} ready={seq.ready} />
        <Footer />
      </main>

      <Loader progress={seq.progress} ready={seq.ready} />
    </>
  )
}
