/*
----------------------------- Usage ----------------------------
```
    g++ writemidi.cpp -std=c++20 -I../include -O3 -o writemidi
    ./writemidi <midi_file_name>
```
*/

#include<iostream>
#include<string>
#include"MiniMidi.hpp"

using namespace std;
using namespace minimidi;

int main(int argc, char *argv[]) {
    if(argc == 2) {
        string target_dir = string(argv[1]);

        track::Track track1;
        track1.messages.emplace_back(message::Message::SetTempo(0, 400000));
        track1.messages.emplace_back(message::Message::TimeSignature(0, 4, 2));

        std::cout << "track1:\n" << track1.to_bytes() << std::endl;
        
        track::Track track2;
        track2.messages.emplace_back(message::Message::TrackName(0, std::string("Test track")));
        track2.messages.emplace_back(message::Message::NoteOn(0, 0, 60, 100));  // (time, channel, pitch, velocity)
        track2.messages.emplace_back(message::Message::NoteOn(480, 0, 60, 0));
        track2.messages.emplace_back(message::Message::NoteOn(480, 0, 60, 100));
        track2.messages.emplace_back(message::Message::NoteOn(960, 0, 60, 0));
        track2.messages.emplace_back(message::Message::NoteOn(960, 0, 64, 100));
        track2.messages.emplace_back(message::Message::NoteOn(1440, 0, 64, 0));
        track2.messages.emplace_back(message::Message::NoteOn(1440, 0, 64, 100));
        track2.messages.emplace_back(message::Message::NoteOn(1920, 0, 64, 0));
        track2.messages.emplace_back(message::Message::NoteOn(1920, 0, 67, 100));
        track2.messages.emplace_back(message::Message::NoteOn(2400, 0, 67, 0));
        track2.messages.emplace_back(message::Message::NoteOn(2400, 0, 67, 100));
        track2.messages.emplace_back(message::Message::NoteOn(2880, 0, 67, 0));
        track2.messages.emplace_back(message::Message::NoteOn(2880, 0, 64, 100));
        track2.messages.emplace_back(message::Message::NoteOn(3840, 0, 64, 0));

        std::cout << "track2:\n" << track2.to_bytes() << std::endl;

        file::MidiFile midifile(file::MidiFormat::MultiTrack,
                                0,
                                960);
        midifile.tracks.emplace_back(track1);
        midifile.tracks.emplace_back(track2);

        std::cout << "file:\n" << midifile.to_bytes() << std::endl;

        midifile.write_file(target_dir);
    } else {
        std::cout << "Usage: ./writemidi <midi_file_name>" << std::endl;
    }

    return 0;
}
