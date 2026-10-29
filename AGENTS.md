# Markdown documentation

- In every Markdown (`*.md`) file created or edited in this repository, wrap
  every line at a maximum of 80 characters. This applies to prose, headings,
  list items, tables, diagrams, and code blocks.

# Engineering workflow

- Build Senritsu in small, independently understandable increments. Each
  increment must implement one focused behavior or one tightly related set of
  behaviors.
- Document every implemented behavior in `docs/`, including its purpose,
  module boundary, assumptions, and user-visible or testable behavior.
- Add or update automated tests in the same change as each implementation.
  Every test must cover a specific expected behavior, boundary, or failure case.
- Keep core-domain and audio-engine unit tests independent of audio hardware,
  installed drivers, a graphical UI, network access, and real-time scheduling.
- Where a feature requires audio hardware or real-time scheduling, provide a
  separate manual or integration validation procedure in addition to unit tests.
- Keep modules modular and testable. Prefer narrow interfaces, explicit inputs
  and outputs, deterministic state, and dependency injection at system edges.
- Keep JUCE-specific types and device access at the backend/UI boundaries. The
  core project model, transport, scheduler, and audio graph must remain
  framework-independent.
- When a CMake test setup exists, register automated tests with CTest. Do not
  require a user to manually invoke individual test binaries by default.
- After every implementation increment, report the files changed, exact commands
  to build and run the relevant tests, expected results, and any manual checks.
- Stop after the increment and wait for user validation before implementing the
  next component, unless the user explicitly requests a multi-increment change.
