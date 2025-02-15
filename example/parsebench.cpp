#include <iostream>
#include <string>
#include <span>
#include <chrono>
#include "nanobench.h"
#include "minimidi/MiniMidi.hpp"

std::vector<uint8_t> read_file(const std::string& filename) {
    FILE* filePtr = fopen(filename.c_str(), "rb");
    if (!filePtr) { throw std::ios_base::failure("MiniMidi: Reading file failed (fopen)!"); }
    fseek(filePtr, 0, SEEK_END);
    const size_t fileLen = ftell(filePtr);
    fseek(filePtr, 0, SEEK_SET);
    std::vector<uint8_t> data(fileLen);
    fread(data.data(), 1, fileLen, filePtr);
    fclose(filePtr);
    return data;
}

int main(int argc, char* argv[]) {
    if (argc == 2) {
        std::string filename = std::string(argv[1]);
        std::cout << "Filename: " << filename << std::endl;
        try {
            const auto data = read_file(filename);
            ankerl::nanobench::Bench()
                .minEpochIterations(500)
                .run("[view] span", [&data] {
                    minimidi::file::MidiFileView<std::span<const uint8_t>> view{data};
                    size_t result = 0;
                    for(const auto& track : view) {
                        for(const auto& msg : track) {
                            result += msg.statusByte;
                        }
                    }
                    ankerl::nanobench::doNotOptimizeAway(result);
                })
                .run("[raw] span", [&data] {
                    minimidi::file::MidiFile<std::span<const uint8_t>> midi{data};
                    size_t result = 0;
                    for(const auto& track : midi.tracks) {
                        for(const auto& msg : track.messages) {
                            result += msg.statusByte;
                        }
                    }
                    ankerl::nanobench::doNotOptimizeAway(result);
                })
                .run("[view] svector", [&data] {
                    minimidi::file::MidiFileView<std::span<const uint8_t>> view{data};
                    size_t result = 0;
                    for(const auto& track : view) {
                        for(const auto& msg : track) {
                            result += msg.statusByte;
                        }
                    }
                    ankerl::nanobench::doNotOptimizeAway(result);
                })
                .run("[raw] svector", [&data] {
                    minimidi::file::MidiFile<std::span<const uint8_t>> midi{data};
                    size_t result = 0;
                    for(const auto& track : midi.tracks) {
                        for(const auto& msg : track.messages) {
                            result += msg.statusByte;
                        }
                    }
                    ankerl::nanobench::doNotOptimizeAway(result);
                })
                .minEpochIterations(20)
                .run("[view] vector", [&data] {
                    minimidi::file::MidiFileView<std::vector<uint8_t>> view{data};
                    size_t result = 0;
                    for(const auto& track : view) {
                        for(const auto& msg : track) {
                            result += msg.statusByte;
                        }
                    }
                    ankerl::nanobench::doNotOptimizeAway(result);
                })
                .run("[raw] vector", [&data] {
                    minimidi::file::MidiFile<std::vector<uint8_t>> midi{data};
                    size_t result = 0;
                    for(const auto& track : midi.tracks) {
                        for(const auto& msg : track.messages) {
                            result += msg.statusByte;
                        }
                    }
                    ankerl::nanobench::doNotOptimizeAway(result);
                })
            ;
        } catch (const char* e) {
            std::cout << e << std::endl;
            exit(EXIT_FAILURE);
        };
    } else {
        std::cout << "Usage: ./midiparse <midi_file_name>" << std::endl;
    }

    return 0;
}
