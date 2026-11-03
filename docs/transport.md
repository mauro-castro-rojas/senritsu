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

## Timeline units: samples and sample frames

Digital audio is discrete. At a sample rate of 48,000 Hz, the system represents
48,000 amplitude values per channel each second. One sample interval lasts
about 20.83 microseconds.

For a multi-channel stream, a **sample frame** is one time position containing
one sample for every channel. A stereo sample frame contains a left value and a
right value that occur at the same point in time.

```text
Sample frame 0: left[0], right[0]
Sample frame 1: left[1], right[1]
Sample frame 2: left[2], right[2]
```

The current transport API uses `sample` as shorthand for a sample-frame
position. `advanceBySamples(512)` therefore means to move 512 timeline frames,
not to move 512 values across all channels. This matches the frame count an
audio callback normally receives. A future API can rename this for greater
precision if needed.

The position is an integer because it represents an exact discrete point on the
audio timeline. At 48 kHz:

```text
Position 0:      0 seconds
Position 48,000: 1 second
Position 96,000: 2 seconds
```

## Processing buffers are also discrete

An audio device requests work in blocks, often called buffers. A 512-frame
buffer at 48 kHz lasts about 10.67 milliseconds. The engine is therefore asked
to prepare one block, then another:

```text
Callback 1 renders frames 0 through 511
Callback 2 renders frames 512 through 1,023
Callback 3 renders frames 1,024 through 1,535
```

The audio engine performs its work in these discrete blocks. The block size is
an engineering and latency choice. It does not change a sample rate of 48,000
frames per second. A device may use 64, 128, 256, 512, or another supported
frame count.

The digital signal is discrete in time and amplitude. The interface's
digital-to-analog converter emits the frames at its hardware clock rate and
uses reconstruction circuitry to form the physical analog output. The analog
signal at the speaker is continuous-time, but the data Senritsu processes is
not.

## `advanceBySamples()`

`advanceBySamples()` records normal progress through the song after the engine
has prepared a buffer. It does not itself create audio or make a speaker jump
from one sound to another.

At 48 kHz, assume the current position is one second into the arrangement and
the device requests 512 frames:

```text
Current position: 48,000 frames = 1.0 seconds
Engine renders:   frames 48,000 through 48,511
New position:     48,512 frames = about 1.0107 seconds
```

After rendering that range, the audio thread can call:

```cpp
transport.advanceBySamples(512);
```

This tells the transport that the logical playhead is ready to render from
frame 48,512 next. It lets future scheduling code know which clips, MIDI notes,
automation changes, and metronome beats belong in the next buffer.

### Events inside a buffer

Moving the high-level transport once per buffer must not force musical events
onto buffer boundaries. The future scheduler will examine the complete range of
the next buffer and locate each event at its exact frame offset.

For example, this buffer starts at frame 48,000 and contains 512 frames:

```text
Buffer range:     48,000 through 48,511
MIDI note begins: 48,032
Event offset:     32 frames from the start of the buffer
```

The engine renders the first 32 frames without the note, starts the note at
frame 48,032, and renders the remainder with the note active. This preserves
sample-accurate timing even though the engine computes several frames at once.

The current `Transport` only stores state. A later scheduler will calculate
these in-buffer event offsets; `advanceBySamples()` supplies its starting and
ending timeline positions.

## `seekToSample()`

`seekToSample()` performs a direct jump to a precise global arrangement
position. It is normally used when a user drags, clicks, or otherwise moves the
transport playhead.

At 48 kHz, if the user moves the playhead from second one to second two, the UI
or command layer can request:

```cpp
transport.seekToSample(96'000);
```

This changes the global playhead from frame 48,000 to frame 96,000. When
playback resumes, the scheduler will identify which clips are active at second
two and render them from the correct point.

Seeking does not move every clip individually. A clip has its own placement in
the arrangement. The global transport position lets the future scheduler map
that position to the relevant position inside every active clip.

For example, a drum clip can begin at second one and continue beyond second
two, while a vocal clip begins exactly at second two. Seeking to second two
means the scheduler later starts the vocal at its beginning and the drum at its
one-second internal offset.

## `noexcept`

`noexcept` is C++ syntax that promises a function will not allow a C++ exception
to escape from it:

```cpp
void Transport::play() noexcept;
```

If an exception escapes a `noexcept` function, C++ terminates the program rather
than unwinding the call stack. The transport's functions only change small,
preallocated values, so they have no expected reason to throw an exception.

This matters for future real-time audio code, where unexpected exception
handling could interrupt time-sensitive work. `noexcept` alone does not make a
function real-time safe: a function must also avoid memory allocation, locks,
disk access, waiting, and other unpredictable work on the audio thread.

## Namespaces and `::`

A namespace is a named container for related C++ code. Senritsu declares this
component inside nested namespaces:

```cpp
namespace senritsu::core {
// Transport is declared here.
}
```

`senritsu` identifies code owned by this application. `core` identifies code
that is independent of JUCE, Windows drivers, the UI, and audio hardware. The
namespace prevents name conflicts with a `Transport` class belonging to another
library.

The `::` operator is C++'s scope-resolution operator. It qualifies a name by
showing the container to which the name belongs:

```cpp
senritsu::core::Transport
std::uint64_t
Transport::play()
```

These examples mean, respectively:

- `Transport` inside `core`, which is inside `senritsu`.
- `uint64_t` inside C++'s standard-library namespace, `std`.
- The `play()` member function belonging to the `Transport` class.

## Automated tests

`tests/transport_tests.cpp` verifies the default state, playback transitions,
sample advancement, stopped behavior, seeking, overflow protection, valid tempo
updates, and rejected tempo values.

The test executable is registered with CTest as `senritsu.transport`.

## Windows setup and validation

CMake configures and drives a build, but it is not a C++ compiler. A Windows
build also needs one matching compiler toolchain. Use either the MinGW or MSVC
instructions below, and keep a separate build directory for each toolchain.

CMake stores the selected generator and compiler in its build-directory cache.
Do not reuse a directory configured for Clang or MSVC when switching to MinGW.

### Option A: MinGW and WinLibs

This is the appropriate path when `cmake` and `mingw32-make` come from WinLibs
or MinGW. First confirm that the matching C++ compiler is available:

```powershell
where.exe g++
g++ --version
```

`where.exe g++` asks Windows to find every `g++` executable on `PATH` and print
its path. `g++ --version` runs that compiler and prints its version. Both must
succeed before configuring Senritsu.

Configure a fresh MinGW build directory from the repository root:

```powershell
cmake -S . -B build-mingw -G "MinGW Makefiles" `
  -DCMAKE_CXX_COMPILER=g++
```

The options mean:

- `-S .` selects the current directory as the source directory. It contains
  Senritsu's root `CMakeLists.txt` file.
- `-B build-mingw` selects `build-mingw` as the generated build directory.
  CMake creates it if needed and places its cache and generated files there.
- `-G "MinGW Makefiles"` selects CMake's MinGW Makefiles generator. CMake will
  create makefiles that `mingw32-make` can build. The quotation marks preserve
  the generator's space-containing name as one argument.
- The PowerShell backtick, `` ` ``, continues the command on the next line. It
  must be the final character of its line; the two lines form one command.
- `-D` sets a CMake cache variable during configuration.
- `CMAKE_CXX_COMPILER=g++` explicitly chooses the MinGW C++ compiler found on
  `PATH`. This prevents CMake from accidentally selecting an unrelated Clang or
  MSVC-targeting compiler.

Build the default target:

```powershell
cmake --build build-mingw
```

`--build` tells CMake to build a previously configured directory.
`build-mingw` is that directory. CMake invokes the generator selected during
configuration, which is `mingw32-make` in this case. With no target specified,
it builds the default target, including the transport test executable.

Run the registered tests:

```powershell
ctest --test-dir build-mingw --output-on-failure
```

`ctest` is CMake's test runner. `--test-dir build-mingw` tells it to load tests
registered in the MinGW build directory. `--output-on-failure` prints a failed
test's output, while keeping successful test output concise.

### Option B: Visual Studio and MSVC

This is an alternative to MinGW. Install Visual Studio 2022 Community or Build
Tools with the **Desktop development with C++** workload, then open Developer
PowerShell for Visual Studio. Configure, build, and test with a separate MSVC
directory:

```powershell
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64
cmake --build build-msvc --config Debug
ctest --test-dir build-msvc -C Debug --output-on-failure
```

`-G "Visual Studio 17 2022"` selects the Visual Studio 2022 generator.
`-A x64` selects 64-bit output. Visual Studio generators support multiple
configurations in one build directory, so `--config Debug` chooses the Debug
build and `-C Debug` tells CTest where to find its test executable.

Expected result:

```text
100% tests passed, 0 tests failed out of 1
```

No manual audio-device check is needed for this component. It is deliberately
hardware-independent. Do not continue to the next component until one of the
two toolchain workflows passes locally.
