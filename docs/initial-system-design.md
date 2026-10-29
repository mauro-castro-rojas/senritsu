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

Use native C++ for the performance-critical application and audio engine. A
cross-platform framework such as JUCE is a practical candidate because it
provides audio-device, UI, MIDI, and VST3 facilities while keeping a route open
to macOS and Linux.

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

## Deliberate deferrals and risk areas

The riskiest areas are audio timing, third-party plugin hosting,
time-stretching/warping, and UI complexity. The project should defer
sophisticated warping and third-party plugin support until the audio and MIDI
foundations are demonstrably reliable.

## Product decision to make next

Choose the first workflow emphasis:

- an Ableton-style clip launcher for live performance;
- a traditional arrangement/timeline DAW; or
- a small, polished hybrid that supports both at a basic level.

The initial choice will determine the first UI and project-model priorities,
while the core audio-engine design can support all three directions.
