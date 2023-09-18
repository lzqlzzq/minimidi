/*
----------------------------- Usage ----------------------------
```
    g++ parsemidi.cpp -I../include -O3 -o parsemidi
    ./parsemidi <midi_file_name>
```
*/

#include<iostream>
#include<string>
#include<filesystem>
#include<set>
#include"MiniMidi.hpp"


using namespace std;
namespace fs = std::filesystem;


int main(int argc, char *argv[]) {
    if(argc == 2) {
        string path_name = string(argv[1]);

        set<fs::path> sorted_by_name;
        uint64_t valid = 0;

        for (auto &entry : fs::directory_iterator(path_name)) {
            sorted_by_name.insert(entry.path().string());
        }

        for (auto &filename : sorted_by_name) {
            cout << filename << endl;
            try {
                minimidi::file::MidiFile midifile = minimidi::file::MidiFile::from_file(filename);
                valid += 1;
                // cout << midifile.track_num() << endl;
                cout << valid << endl;
            } catch(const char* e) {
                cout << e;
                cout << endl;
                continue;
            };
        }

        cout << "Valid: " << valid << endl;
    } else {
        std::cout << "Usage: ./midiparse <midi_file_name>" << std::endl;
    }

    return 0;
}
