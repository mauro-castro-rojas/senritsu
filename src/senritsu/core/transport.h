#pragma once

#include <cstdint>

namespace senritsu::core {

class Transport final {
public:
    static constexpr double kDefaultTempoBpm = 120.0;

    [[nodiscard]] bool isPlaying() const noexcept;
    [[nodiscard]] double tempoBpm() const noexcept;
    [[nodiscard]] std::uint64_t positionSamples() const noexcept;

    void play() noexcept;
    void stop() noexcept;
    void seekToSample(std::uint64_t positionSamples) noexcept;
    void advanceBySamples(std::uint64_t sampleCount) noexcept;

    [[nodiscard]] bool setTempoBpm(double tempoBpm) noexcept;

private:
    double tempoBpm_ { kDefaultTempoBpm };
    std::uint64_t positionSamples_ { 0 };
    bool isPlaying_ { false };
};

} // namespace senritsu::core
