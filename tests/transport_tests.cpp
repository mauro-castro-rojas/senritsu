#include "senritsu/core/transport.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

class TestResults final {
public:
    void expect(const bool condition, const std::string_view message) {
        if (condition) {
            return;
        }

        ++failureCount_;
        std::cerr << "FAILED: " << message << '\n';
    }

    [[nodiscard]] int failureCount() const noexcept {
        return failureCount_;
    }

private:
    int failureCount_ { 0 };
};

void testDefaultState(TestResults& results) {
    const senritsu::core::Transport transport;

    results.expect(!transport.isPlaying(), "a new transport is stopped");
    results.expect(
        transport.tempoBpm() == senritsu::core::Transport::kDefaultTempoBpm,
        "a new transport uses the default tempo");
    results.expect(
        transport.positionSamples() == 0,
        "a new transport starts at sample zero");
}

void testPlaybackAndPosition(TestResults& results) {
    senritsu::core::Transport transport;

    transport.advanceBySamples(128);
    results.expect(
        transport.positionSamples() == 0,
        "a stopped transport does not advance");

    transport.play();
    transport.advanceBySamples(128);
    results.expect(transport.isPlaying(), "play marks the transport as playing");
    results.expect(
        transport.positionSamples() == 128,
        "a playing transport advances by the supplied samples");

    transport.stop();
    transport.advanceBySamples(64);
    results.expect(!transport.isPlaying(), "stop marks the transport as stopped");
    results.expect(
        transport.positionSamples() == 128,
        "stop preserves position and prevents further advancement");
}

void testSeekingAndSaturation(TestResults& results) {
    senritsu::core::Transport transport;

    transport.seekToSample(4'096);
    results.expect(
        transport.positionSamples() == 4'096,
        "seek sets the exact sample position");

    transport.seekToSample(std::numeric_limits<std::uint64_t>::max() - 4);
    transport.play();
    transport.advanceBySamples(8);
    results.expect(
        transport.positionSamples() == std::numeric_limits<std::uint64_t>::max(),
        "position advancement saturates instead of overflowing");
}

void testTempoValidation(TestResults& results) {
    senritsu::core::Transport transport;

    results.expect(
        transport.setTempoBpm(128.5),
        "a positive finite tempo is accepted");
    results.expect(transport.tempoBpm() == 128.5, "an accepted tempo is stored");

    results.expect(!transport.setTempoBpm(0.0), "zero tempo is rejected");
    results.expect(!transport.setTempoBpm(-1.0), "negative tempo is rejected");
    results.expect(
        !transport.setTempoBpm(std::numeric_limits<double>::infinity()),
        "infinite tempo is rejected");
    results.expect(
        !transport.setTempoBpm(std::numeric_limits<double>::quiet_NaN()),
        "NaN tempo is rejected");
    results.expect(
        transport.tempoBpm() == 128.5,
        "a rejected tempo does not modify the stored tempo");
}

} // namespace

int main() {
    TestResults results;

    testDefaultState(results);
    testPlaybackAndPosition(results);
    testSeekingAndSaturation(results);
    testTempoValidation(results);

    if (results.failureCount() != 0) {
        std::cerr << results.failureCount() << " test assertion(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All transport tests passed.\n";
    return EXIT_SUCCESS;
}
