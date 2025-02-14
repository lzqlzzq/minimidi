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
                .minEpochIterations(100)
                .run("span", [&data] {
                    minimidi::file::MidiFile<std::span<const uint8_t>> midifile{data};
                    ankerl::nanobench::doNotOptimizeAway(midifile);
                })
                .run("svector", [&data] {
                    minimidi::file::MidiFile midifile{data};
                    ankerl::nanobench::doNotOptimizeAway(midifile);
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
