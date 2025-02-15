/*
----------------------------- Usage ----------------------------
```
    g++ redumpmidi.cpp -O3 -std=c++20 -I../include -o redumpmidi
    ./redumpmidi <source_midifile>.mid <target_textfile>.mid
```
*/

#include<iostream>
#include<sstream>
#include<string>
#include<cstddef>
#include<vector>
#include<fstream>
#include<algorithm>
#include<functional>
#include"minimidi/MiniMidi.hpp"

using namespace std;
using namespace minimidi;


void write_file(const string& from, const string& to)
{
    MidiFile midiFile = MidiFile<>::from_file(from);
    MidiFile midiFile2 = MidiFile(midiFile.tracks,
        MidiFormat::MultiTrack,
        0,
        midiFile.ticksPerQuarter);
    midiFile2.write_file(to);
};

int main(int argc, char *argv[])
{
    if(argc == 3)
    {
        string source_dir = string(argv[1]);
        string target_dir = string(argv[2]);

        write_file(source_dir, target_dir);
    }
    else
    {
        std::cout << "Usage: ./redumpmidi <source_midifile>.mid <target_textfile>.mid" << std::endl;
    }

    return 0;
}

