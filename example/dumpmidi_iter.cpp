/*
----------------------------- Usage ----------------------------
```
    g++ dumpmidi_iter.cpp -O3 -std=c++20 -I../include -o dumpmidi
    ./dumpmidi <source_midifile>.mid <target_textfile>.txt
```
*/

#include<iostream>
#include<sstream>
#include<string>
#include<filesystem>
#include<cstddef>
#include<set>
#include<vector>
#include<fstream>
#include<algorithm>
#include<functional>
#include"MiniMidi.hpp"

using namespace std;
using namespace minimidi;


void write_file(const string& from, const string& to)
{
    ofstream dst(to, ios::binary);

    file::MidiFileIter midiFileIter = file::MidiFileIter::from_file(from);
    int t = 0;
    while(!midiFileIter.track_iter().is_empty()) {
        dst << "Track: " << t << endl;
        track::MessageIter msgIter = midiFileIter.track_iter().read_a_message_iter();
        while(!msgIter.is_empty()) {
            dst << "    " << msgIter.read_a_message() << endl;
        }
        dst << endl;
        t++;
    }
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
        std::cout << "Usage: ./midiwrite <source_midifile>.mid <target_textfile>.txt" << std::endl;
    }

    return 0;
}

