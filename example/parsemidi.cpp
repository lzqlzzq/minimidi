/*
----------------------------- Usage ----------------------------
```
    g++ parsemidi.cpp -I ../include -O3 -o parsemidi
    ./parsemidi <midi_file_name>
```
*/
#include<iostream>
#include<string>
#include"MiniMidi.hpp"


int main(int argc, char *argv[]) {
    if(argc == 2) {
        std::string filename = std::string(argv[1]);
        std::cout << "Filename: " << filename << std::endl;
        minimidi::file::MidiFile midifile = minimidi::file::MidiFile::from_file(filename);
        std::cout << midifile;
    } else {
        std::cout << "Usage: ./midiparse <midi_file_name>" << std::endl;
    }

    return 0;
}
