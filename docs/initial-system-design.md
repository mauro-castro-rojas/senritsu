# Senritsu: Initial System Design

## Product direction

Audio workstation (DAW), inspired by Ableton.

The first release is a music-production MVP for Windows, macOS and Linux support
can be evaluated after the core is stable.

## MVP requirements

- Low-latency audio playback on Windows.
- MIDI input and sequencing.
- Audio and MIDI tracks.
- Clips placed in an arrangement timeline; an Ableton-style
  session/clip-launcher view comes later.
- Transport controls: play, stop, loop, tempo, metronome, and recording.
- Basic mixing: gain, pan, mute, solo, and sends.
- A simple built-in instrument and basic effects.
- Project save/load, undo/redo, and crash recovery.
- Offline export to WAV.
- Third-party plugin hosting after the core engine is stable.

## Real-time requirements

Continuous, glitch-free audio takes precedence over UI responsiveness and
non-audio work. The audio callback must:

- Run on a dedicated high-priority thread.
- Never block on locks, allocate memory, read/write disks, or wait for UI work.
- Apply state changes safely at audio-buffer boundaries.
- Maintain sample-accurate timing wherever feasible.

The UI, project saving, autosave, waveform generation, plugin scanning, and
project loading run on non-real-time background threads. They communicate with
the audio engine through preallocated queues or safely swapped playback
snapshots.

## High-level architecture

```text
┌─────────────────────────────────────────────────────┐
│ UI Layer                                            │
│ Arrangement · Clip Launcher · Mixer · Device Panels │
└───────────────────┬─────────────────────────────────┘
                    │ commands / state updates
┌───────────────────▼─────────────────────────────────┐
│ Application / Project Layer                         │
│ Undo · Persistence · Autosave · Asset Management    │
└───────────────────┬─────────────────────────────────┘
                    │ compiled playback state
┌───────────────────▼─────────────────────────────────┐
│ Real-Time Audio Engine                              │
│ Transport · Scheduler · Audio Graph · MIDI Routing  │
│ Mixing · Automation · Rendering                     │
└───────┬───────────────────────────────┬─────────────┘
        │                               │
┌───────▼────────┐              ┌───────▼────────┐
│ Audio Backend  │              │ MIDI Backend   │
│ ASIO / WASAPI  │              │ Windows MIDI   │
└───────┬────────┘              └────────────────┘
        │
   Audio interface
```

## Audio graph

Sound sources and processing units are represented as nodes in an audio graph:

```text
MIDI Clip → Instrument → Effect Chain → Track Mixer ┐
Audio Clip ────────────→ Effect Chain → Track Mixer ├→ Master → Device
Return Tracks ──────────────────────────────────────┘
```

This model supports adding instruments, effects, sends, return tracks, routing,
and automation without designing each feature as a separate playback path.

## Project and playback state

The editable project must remain separate from what the real-time audio thread
uses:

- **Project model:** editable tracks, clips, device settings, automation, and
  media-file references. This is persisted to disk.
- **Playback model:** an optimized, immutable snapshot compiled from the project
  for use by the real-time engine.
- **Runtime state:** transient playhead position, active notes, buffers, meters,
  and temporary device state.

This separation lets the user edit without creating clicks, dropouts, or
concurrency problems in playback.

## Windows audio priorities

1. **ASIO** for professional interfaces and the lowest practical latency.
2. **WASAPI**, in exclusive or shared mode, as the built-in Windows fallback.
3. **WDM-KS** only if a later, well-defined need justifies its extra complexity.

## Technology direction

Use native C++ for the performance-critical application and audio engine. JUCE
is selected for the initial audio-device and MIDI layer, and can also provide
cross-platform UI support. It keeps a route open to macOS and Linux.

Senritsu's project model, scheduler, audio graph, and engine interfaces should
remain independent of framework-specific types. This keeps the architecture
testable and allows a framework component to be replaced without rewriting the
DAW's core.

## Delivery stages

1. **Foundation:** application shell, audio-device selection, audio callback,
   transport, and metronome.
2. **Audio MVP:** one audio track, WAV loading, playback, seeking, looping,
   simple mixer controls, and WAV export.
3. **MIDI MVP:** MIDI input, piano-roll clip, a basic synthesizer, and MIDI
   recording.
4. **DAW workflow:** multiple tracks, routing, effects, automation, and
   arrangement editing.
5. **Clip-launcher workflow:** session clips, scenes, and quantized triggering.
6. **Ecosystem:** VST3 hosting, plugin isolation, advanced warping,
   collaboration, and controller support.

## First step: audio backend

### Decision

Senritsu will use JUCE for its initial audio-device and MIDI integration layer.
It will not use JUCE as the implementation of the Senritsu audio engine.

Senritsu will own its playback engine, audio graph, transport scheduler,
mixing, DSP, and project model. JUCE will open Windows audio devices, select the
driver API, and invoke Senritsu's real-time audio callback.

```text
Senritsu UI / Project / Audio Engine
                │
        IAudioBackend interface
                │
         JUCE implementation
                │
          ASIO or WASAPI
                │
          Audio interface
```

The engine-facing backend interface must use Senritsu's own types rather than
JUCE types. The first implementation can be named `JuceAudioBackend`. This
keeps a future native backend possible without making it necessary now.

### Rationale

JUCE provides mature, cross-platform audio-device and MIDI integration,
including ASIO and WASAPI on Windows. It does not impose Senritsu's timing
model, mixer design, routing, or DSP algorithms.

Professional performance and sound quality depend primarily on the real-time
engine design, the selected driver, the audio interface, and the DSP workload.
JUCE does not prevent a professional-quality DAW when the audio callback obeys
the real-time requirements in this document.

A custom ASIO, WASAPI, or WDM-KS backend would add significant driver,
device-management, and debugging work without improving the first MVP by
itself. Senritsu should consider a native backend only after profiling shows a
specific JUCE-related limitation that affects users.

### Windows driver priorities

1. Prefer **ASIO** for professional interfaces with an installed ASIO driver.
2. Use **WASAPI** as the dependable built-in Windows fallback.
3. Do not build direct WDM-KS or direct ASIO support for the MVP.

Hardware and driver support determine the smallest reliable buffer size. The
application must expose practical buffer settings and test at 48 kHz with 64,
128, and 256 sample buffers. A 64-sample buffer at 48 kHz represents about
1.33 ms for one buffer, but the user-visible round-trip latency also includes
driver and hardware buffering.

### Audio APIs, drivers, and devices

Senritsu will use Windows audio APIs and installed audio drivers; it will not
create or ship a hardware driver. JUCE selects the chosen API, opens an audio
stream, and repeatedly invokes Senritsu's audio callback to request the next
buffer of samples.

An ASIO driver is normally supplied by the manufacturer of a professional audio
interface. It lets a DAW communicate with that interface at small, stable
buffer sizes and usually with exclusive control of the interface. It is the
preferred path for live instruments, recording, and MIDI performance.

WASAPI is the Windows audio API and works with ordinary speakers, headphones,
Bluetooth devices, HDMI outputs, and many audio interfaces. It provides two
relevant modes:

- **Shared mode:** Windows mixes Senritsu with other applications. It is
  convenient, but usually has higher latency.
- **Exclusive mode:** Senritsu takes exclusive control of the output device and
  can request lower latency. Other applications cannot use that device while
  the stream is open.

```text
ASIO
  Senritsu → JUCE → vendor ASIO driver → audio interface

WASAPI, shared
  Senritsu → JUCE → Windows Audio Engine → device driver → audio interface

WASAPI, exclusive
  Senritsu → JUCE → device driver → audio interface
```

One physical professional interface can often support both paths: its vendor
ASIO driver is preferred for production, while WASAPI remains a reliable
fallback. During device configuration, the user will choose a driver type,
device, sample rate, and buffer size.

### Licensing checkpoint

JUCE is available under AGPLv3 or a commercial license. Before distributing a
closed-source Senritsu binary, select and comply with the appropriate JUCE
license.



Your environment currently has no CMake or C++ compiler available on PATH, so
I could not compile it here. Install Visual Studio 2022 Community or Build Tools
with Desktop development with C++, then open Developer PowerShell and run:

cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure


