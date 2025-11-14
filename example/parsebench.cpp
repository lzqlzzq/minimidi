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

template<typename T>
void proc(const minimidi::Message<T>& msg, size_t& result) {
    const auto type = msg.type();
    if (type == minimidi::MessageType::NoteOn) {
        result += msg.template cast<minimidi::NoteOn>().velocity();
    } else if (type == minimidi::MessageType::NoteOff) {
        result += msg.template cast<minimidi::NoteOff>().pitch();
    }
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
                    minimidi::MidiFileView<std::span<const uint8_t>> view{data};
                    size_t result = 0;
                    for(const auto& track : view) {
                        for(const auto& msg : track) {
                            proc(msg, result);
                        }
                    }
                    ankerl::nanobench::doNotOptimizeAway(result);
                })
                .run("[raw] span", [&data] {
                    minimidi::MidiFile<std::span<const uint8_t>> midi{data};
                    size_t result = 0;
                    for(const auto& track : midi.tracks) {
                        for(const auto& msg : track.messages) {
                            proc(msg, result);
                        }
                    }
                    ankerl::nanobench::doNotOptimizeAway(result);
                })
                .run("[view] svector", [&data] {
                    minimidi::MidiFileView view{data};
                    size_t result = 0;
                    for(const auto& track : view) {
                        for(const auto& msg : track) {
                            proc(msg, result);
                        }
                    }
                    ankerl::nanobench::doNotOptimizeAway(result);
                })
                .run("[raw] svector", [&data] {
                    minimidi::MidiFile midi{data};
                    size_t result = 0;
                    for(const auto& track : midi.tracks) {
                        for(const auto& msg : track.messages) {
                            proc(msg, result);
                        }
                    }
                    ankerl::nanobench::doNotOptimizeAway(result);
                })
                .minEpochIterations(20)
                .run("[view] vector", [&data] {
                    minimidi::MidiFileView<std::vector<uint8_t>> view{data};
                    size_t result = 0;
                    for(const auto& track : view) {
                        for(const auto& msg : track) {
                            proc(msg, result);
                        }
                    }
                    ankerl::nanobench::doNotOptimizeAway(result);
                })
                .run("[raw] vector", [&data] {
                    minimidi::MidiFile<std::vector<uint8_t>> midi{data};
                    size_t result = 0;
                    for(const auto& track : midi.tracks) {
                        for(const auto& msg : track.messages) {
                            proc(msg, result);
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
