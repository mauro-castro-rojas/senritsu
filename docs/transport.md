# Transport component

## Purpose

`senritsu::core::Transport` is the first framework-independent core component.
It owns only deterministic transport state: playing or stopped, tempo in beats
per minute, and a sample-frame position. It does not access JUCE, an audio
device, a driver, a UI, the filesystem, or any background thread.

## Module boundary

The transport accepts commands from a higher-level scheduler or UI and exposes
its state to an audio engine. It does not decide musical looping, clip launch
quantization, time signatures, or how samples are rendered. Those are separate
future components.

## Behavior

- A new transport is stopped, is positioned at sample zero, and uses 120 BPM.
- `play()` marks the transport as playing without changing its position.
- `stop()` marks it as stopped and preserves its current position.
- `seekToSample()` sets an exact sample-frame position.
- `advanceBySamples()` changes position only while the transport is playing.
- Position advancement saturates at the maximum `std::uint64_t` value rather
  than overflowing.
- `setTempoBpm()` accepts only finite, strictly positive values. Invalid values
  return `false` and do not change the existing tempo.

## Automated tests

`tests/transport_tests.cpp` verifies the default state, playback transitions,
sample advancement, stopped behavior, seeking, overflow protection, valid tempo
updates, and rejected tempo values.

The test executable is registered with CTest as `senritsu.transport`.

## Windows setup and validation

Install Visual Studio 2022 Community or Build Tools with the **Desktop
development with C++** workload. Ensure CMake is installed and open a Developer
PowerShell for Visual Studio, or make both `cmake` and the MSVC compiler
available on `PATH`.

From the repository root, configure, build, and run the test suite:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Expected result:

```text
100% tests passed, 0 tests failed out of 1
```

No manual audio-device check is needed for this component. It is deliberately
hardware-independent. Do not continue to the next component until the commands
above pass locally.
