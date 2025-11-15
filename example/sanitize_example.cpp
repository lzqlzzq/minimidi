#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include "minimidi/MiniMidi.hpp"

namespace {
minimidi::container::Bytes read_file(const std::string& filename) {
    std::ifstream input(filename, std::ios::binary);
    if (!input) { throw std::ios_base::failure("MiniMidi: unable to open file."); }
    input.seekg(0, std::ios::end);
    const auto size = input.tellg();
    input.seekg(0, std::ios::beg);
    minimidi::container::Bytes data(static_cast<size_t>(size));
    input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return data;
}

template<typename TRawContainer>
bool differs(const minimidi::Message<TRawContainer>& raw, const minimidi::Message<minimidi::container::SmallBytes>& sanitized) {
    const auto& raw_data = raw.data();
    const auto& clean    = sanitized.data();
    if (raw_data.size() != clean.size()) return true;
    for (size_t i = 0; i < raw_data.size(); ++i) {
        if (raw_data[i] != clean[i]) return true;
    }
    return false;
}
}   // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: ./sanitize_example <midi_file>" << std::endl;
        return 1;
    }

    try {
        const auto data = read_file(argv[1]);

        minimidi::MidiFileView<> sanitized_view(data.data(), data.size(), true);
        minimidi::MidiFileView<std::span<const uint8_t>> raw_view(data.data(), data.size());

        std::cout << "Sanitized view container: minimidi::container::SmallBytes (mutable)\n";
        std::cout << "Raw view container: std::span<const uint8_t> (read-only)\n";

        size_t total_tracks   = 0;
        size_t total_messages = 0;
        std::vector<size_t> per_track_mismatch;

        auto raw_track_it   = raw_view.begin();
        auto clean_track_it = sanitized_view.begin();
        size_t total_mismatch = 0;
        for (; raw_track_it != raw_view.end() && clean_track_it != sanitized_view.end();
             ++raw_track_it, ++clean_track_it) {
            const auto& raw_track   = *raw_track_it;
            const auto& clean_track = *clean_track_it;

            size_t mismatch_in_track = 0;
            size_t messages_in_track = 0;

            auto raw_msg_it   = raw_track.begin();
            auto clean_msg_it = clean_track.begin();
            for (; raw_msg_it != raw_track.end() && clean_msg_it != clean_track.end();
                 ++raw_msg_it, ++clean_msg_it) {
                ++messages_in_track;
                if (differs(*raw_msg_it, *clean_msg_it)) { ++mismatch_in_track; }
            }

            if (raw_msg_it != raw_track.end() || clean_msg_it != clean_track.end()) {
                throw std::runtime_error("MiniMidi: track iteration mismatch between raw and sanitized view.");
            }

            ++total_tracks;
            total_messages += messages_in_track;
            per_track_mismatch.push_back(mismatch_in_track);
            total_mismatch += mismatch_in_track;
        }

        if (raw_track_it != raw_view.end() || clean_track_it != sanitized_view.end()) {
            throw std::runtime_error("MiniMidi: track count mismatch between raw and sanitized view.");
        }

        std::cout << "Track count: " << total_tracks << ", message count: " << total_messages << std::endl;
        std::cout << "Total mismatched messages: " << total_mismatch << std::endl;
        for (size_t i = 0; i < per_track_mismatch.size(); ++i) {
            std::cout << "Track " << i << " mismatched messages: " << per_track_mismatch[i] << std::endl;
        }

    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
