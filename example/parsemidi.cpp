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
            const auto midifile = minimidi::MidiFile<>::from_file(filename, true);
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
