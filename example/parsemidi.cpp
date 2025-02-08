/*
----------------------------- Usage ----------------------------
```
    g++ parsemidi.cpp -std=c++20 -I../include -O3 -o parsemidi
    ./parsemidi <midi_file_name>
```
*/

#include <iostream>
#include <string>
#include <span>
#include <chrono>

#include "minimidi/MiniMidi.hpp"


int main(int argc, char* argv[]) {
    if (argc == 2) {
        std::string filename = std::string(argv[1]);
        std::cout << "Filename: " << filename << std::endl;
        try {
            // open and read the file into a vector
            FILE* filePtr = fopen(filename.c_str(), "rb");
            if (!filePtr) { throw std::ios_base::failure("MiniMidi: Reading file failed (fopen)!"); }
            fseek(filePtr, 0, SEEK_END);
            const size_t fileLen = ftell(filePtr);
            fseek(filePtr, 0, SEEK_SET);
            std::vector<uint8_t> data(fileLen);
            fread(data.data(), 1, fileLen, filePtr);
            fclose(filePtr);

            const minimidi::file::MidiFile midifile{data};
            std::cout << minimidi::to_string(midifile) << std::endl;
        } catch (const char* e) {
            std::cout << e << std::endl;
            exit(EXIT_FAILURE);
        };
    } else {
        std::cout << "Usage: ./midiparse <midi_file_name>" << std::endl;
    }

    return 0;
}
