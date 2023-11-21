/*
----------------------------- Usage ----------------------------
```
    g++ dumpmidi.cpp -O3 -std=c++20 -I../include -o dumpmidi
    ./dumpmidi <source_midifile>.mid <target_textfile>.txt
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
#include"MiniMidi.hpp"

using namespace std;
using namespace minimidi;


void write_file(const string& from, const string& to)
{
    file::MidiFile midiFile = file::MidiFile::from_file(from);
    ofstream dst(to, ios::binary);

    dst << midiFile;
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

