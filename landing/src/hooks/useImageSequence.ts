import { useEffect, useRef, useState } from 'react'

export interface ImageSequence {
  /** Ref to the array of preloaded <img> elements (stable across renders). */
  images: React.RefObject<HTMLImageElement[]>
  loaded: number
  total: number
  progress: number
  ready: boolean
}

/**
 * Preloads an image sequence and reports progress.
 * Images are decoded up front so canvas drawImage never blocks during scroll.
 */
export function useImageSequence(urls: string[]): ImageSequence {
  const imagesRef = useRef<HTMLImageElement[]>([])
  const [loaded, setLoaded] = useState(0)
  const [ready, setReady] = useState(false)

  useEffect(() => {
    let cancelled = false
    const images: HTMLImageElement[] = new Array(urls.length)
    imagesRef.current = images
    let count = 0

    const tick = () => {
      if (cancelled) return
      count += 1
      setLoaded(count)
    }

    const jobs = urls.map((url, i) => {
      const img = new Image()
      img.decoding = 'async'
      img.src = url
      images[i] = img
      // decode() resolves once the bitmap is ready; swallow errors so one
      // missing frame never blocks the whole sequence.
      return img
        .decode()
        .catch(() => undefined)
        .finally(tick)
    })

    Promise.all(jobs).then(() => {
      if (!cancelled) setReady(true)
    })

    return () => {
      cancelled = true
    }
  }, [urls])

  return {
    images: imagesRef,
    loaded,
    total: urls.length,
    progress: urls.length ? loaded / urls.length : 0,
    ready,
  }
}
