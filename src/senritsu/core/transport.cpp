#include "senritsu/core/transport.h"

#include <cmath>
#include <limits>

namespace senritsu::core {

bool Transport::isPlaying() const noexcept {
    return isPlaying_;
}

double Transport::tempoBpm() const noexcept {
    return tempoBpm_;
}

std::uint64_t Transport::positionSamples() const noexcept {
    return positionSamples_;
}

void Transport::play() noexcept {
    isPlaying_ = true;
}

void Transport::stop() noexcept {
    isPlaying_ = false;
}

void Transport::seekToSample(const std::uint64_t positionSamples) noexcept {
    positionSamples_ = positionSamples;
}

void Transport::advanceBySamples(const std::uint64_t sampleCount) noexcept {
    if (!isPlaying_) {
        return;
    }

    const auto maximumPosition = std::numeric_limits<std::uint64_t>::max();

    if (sampleCount > maximumPosition - positionSamples_) {
        positionSamples_ = maximumPosition;
        return;
    }

    positionSamples_ += sampleCount;
}

bool Transport::setTempoBpm(const double tempoBpm) noexcept {
    if (!std::isfinite(tempoBpm) || tempoBpm <= 0.0) {
        return false;
    }

    tempoBpm_ = tempoBpm;
    return true;
}

} // namespace senritsu::core
