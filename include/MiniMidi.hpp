#ifndef __MINIMIDI_HPP
#define __MINIMIDI_HPP

#include<cstdint>
#include<cstddef>
#include<vector>
#include<string>
#include<cstdlib>
#include<iostream>
#include<string>
#include<iomanip>
#include<cstring>


namespace minimidi {

namespace container {

typedef std::vector<uint8_t> Bytes;

}


namespace utils {

inline uint32_t read_variable_length(uint8_t* &buffer) {
    uint32_t value = 0;

    for(auto i = 0; i < 4; i++) {
        value = (value << 7) + (*buffer & 0x7f);
        if(!(*buffer & 0x80)) break;
        buffer++;
    }

    buffer++;
    return value;
};

inline uint64_t read_msb_bytes(uint8_t* buffer, size_t length) {
    uint64_t res = 0;

    for(auto i = 0; i < length; i++) {
        res <<= 8;
        res += (*(buffer + i));
    }

    return res;
};

inline bool ensure_range(uint8_t* cursor, size_t length) {
    for(auto i = 0; i < length; i++) {
        if(*cursor & 0x80)
            return false;
    }
    return true;
};

}


namespace message {

// (name, status, length)
#define MIDI_MESSAGE_TYPE                                      \
    MIDI_MESSAGE_TYPE_MEMBER(Unknown, 0x00, 65535)             \
    MIDI_MESSAGE_TYPE_MEMBER(NoteOff, 0x80, 3)                 \
    MIDI_MESSAGE_TYPE_MEMBER(NoteOn, 0x90, 3)                  \
    MIDI_MESSAGE_TYPE_MEMBER(PolyphonicAfterTouch, 0xA0, 3)    \
    MIDI_MESSAGE_TYPE_MEMBER(ControlChange, 0xB0, 3)           \
    MIDI_MESSAGE_TYPE_MEMBER(ProgramChange, 0xC0, 2)           \
    MIDI_MESSAGE_TYPE_MEMBER(ChannelAfterTouch, 0xD0, 2)       \
    MIDI_MESSAGE_TYPE_MEMBER(PitchBend, 0xE0, 3)               \
    MIDI_MESSAGE_TYPE_MEMBER(SysExStart, 0xF0, 65535)          \
    MIDI_MESSAGE_TYPE_MEMBER(SongPositionPointer, 0xF2, 3)     \
    MIDI_MESSAGE_TYPE_MEMBER(SongSelect, 0xF3, 2)              \
    MIDI_MESSAGE_TYPE_MEMBER(TuneRequest, 0xF6, 1)             \
    MIDI_MESSAGE_TYPE_MEMBER(SysExEnd, 0xF7, 1)                \
    MIDI_MESSAGE_TYPE_MEMBER(TimingClock, 0xF8, 1)             \
    MIDI_MESSAGE_TYPE_MEMBER(StartSequence, 0xFA, 1)           \
    MIDI_MESSAGE_TYPE_MEMBER(ContinueSequence, 0xFB, 1)        \
    MIDI_MESSAGE_TYPE_MEMBER(StopSequence, 0xFC, 1)            \
    MIDI_MESSAGE_TYPE_MEMBER(ActiveSensing, 0xFE, 1)           \
    MIDI_MESSAGE_TYPE_MEMBER(Meta, 0xFF, 65535)                \

// (name, status)
#define MIDI_META_TYPE                                    \
    MIDI_META_TYPE_MEMBER(SequenceNumber, 0x00)           \
    MIDI_META_TYPE_MEMBER(Text, 0x01)                     \
    MIDI_META_TYPE_MEMBER(CopyrightNote, 0x02)            \
    MIDI_META_TYPE_MEMBER(TrackName, 0x03)                \
    MIDI_META_TYPE_MEMBER(InstrumentName, 0x04)           \
    MIDI_META_TYPE_MEMBER(Lyric, 0x05)                    \
    MIDI_META_TYPE_MEMBER(Marker, 0x06)                   \
    MIDI_META_TYPE_MEMBER(CuePoint, 0x07)                 \
    MIDI_META_TYPE_MEMBER(MIDIChannelPrefix, 0x20)        \
    MIDI_META_TYPE_MEMBER(EndOfTrack, 0x2F)               \
    MIDI_META_TYPE_MEMBER(SetTempo, 0x51)                 \
    MIDI_META_TYPE_MEMBER(SMPTEOffset, 0x54)              \
    MIDI_META_TYPE_MEMBER(TimeSignature, 0x58)            \
    MIDI_META_TYPE_MEMBER(KeySignature, 0x59)             \
    MIDI_META_TYPE_MEMBER(SequencerSpecificMeta, 0x7F)    \
    MIDI_META_TYPE_MEMBER(Unknown, 0xFF)    \


enum class MessageType {
    #define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) type,
    MIDI_MESSAGE_TYPE
    #undef MIDI_MESSAGE_TYPE_MEMBER
};

inline const std::string message_type_to_string(const MessageType& messageType) {
    switch(messageType) {
        #define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) \
        case MessageType::type: return #type;
        MIDI_MESSAGE_TYPE
        #undef MIDI_MESSAGE_TYPE_MEMBER
    };

    return "Unknown";
};

typedef struct
{
    uint8_t status;
    size_t length;
} MessageAttr;

inline const MessageAttr& message_attr(const MessageType& messageType) {
    static const MessageAttr MESSAGE_ATTRS[] = {
        #define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) {status, length},
        MIDI_MESSAGE_TYPE
        #undef MIDI_MESSAGE_TYPE_MEMBER
    };
    return MESSAGE_ATTRS[static_cast<std::underlying_type<MessageType>::type>(messageType)];
};

inline MessageType status_to_message_type(uint8_t status) {
    if(status < 0xF0) {
        switch(status & 0xF0) {
            #define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) \
                case status: return MessageType::type;
            MIDI_MESSAGE_TYPE
            #undef MIDI_MESSAGE_TYPE_MEMBER
        }
    }
    else {
        switch(status) {
            #define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) \
                case status: return MessageType::type;
            MIDI_MESSAGE_TYPE
            #undef MIDI_MESSAGE_TYPE_MEMBER
        }
    }

    return MessageType::Unknown;
};

enum class MetaType: uint8_t {
    #define MIDI_META_TYPE_MEMBER(type, status) type = status,
    MIDI_META_TYPE
    #undef MIDI_META_TYPE_MEMBER
};

inline MetaType status_to_meta_type(uint8_t status) {
    switch(status) {
        #define MIDI_META_TYPE_MEMBER(type, status) \
            case status: return MetaType::type;
        MIDI_META_TYPE
        #undef MIDI_META_TYPE_MEMBER
    }

    return MetaType::Unknown;
};

inline const std::string meta_type_to_string(const MetaType& metaType) {
    switch(metaType) {
        #define MIDI_META_TYPE_MEMBER(type, status) \
            case (MetaType::type): return #type;
        MIDI_META_TYPE
        #undef MIDI_META_TYPE_MEMBER
    };

    return "Unknown";
}

#undef MIDI_MESSAGE_TYPE
#undef MIDI_META_TYPE

typedef struct {
    uint8_t numerator;
    uint8_t denominator;
} TimeSignature;

class KeySignature {
public:
    int8_t key;
    uint8_t tonality;

    inline const std::string to_string() {
        static const std::string MINOR_KEYS[] = {"bc", "bg", "bd", "ba", "be", "bb", "f", "c", "g", "d", "a", "e", "b", "#f", "#c"};
        static const std::string MAJOR_KEYS[] = {"bC", "bG", "bD", "bA", "bE", "bB", "F", "C", "G", "D", "A", "E", "B", "#F", "#C"};

        return this->tonality ? MINOR_KEYS[this->key + 7] : MAJOR_KEYS[this->key + 7];
    };
};

class Message {
private:
    uint32_t time;
    container::Bytes data;
    MessageType msgType;

public:
    Message() = default;
    Message(uint32_t time, const container::Bytes& data) {
        this->time = time;
        this->msgType = status_to_message_type(data[0]);

        /*
        if(((this->msgType == MessageType::NoteOn ||
            this->msgType == MessageType::ControlChange ||
            this->msgType == MessageType::NoteOff ||
            this->msgType == MessageType::PolyphonicAfterTouch ||
            this->msgType == MessageType::PitchBend) &&
            !utils::ensure_range(const_cast<uint8_t*>(&data[1]), 2)) ||
            ((this->msgType == MessageType::ProgramChange ||
            this->msgType == MessageType::ChannelAfterTouch ||
            this->msgType == MessageType::SongSelect) &&
            !utils::ensure_range(const_cast<uint8_t*>(&data[1]), 1)))
            throw "Data range must between 0 and 127!";
        */

        this->data = data;
    };

    inline uint32_t get_time() const {
        return this->time;
    };

    inline container::Bytes get_data() const {
        return this->data;
    };

    inline MessageType get_type() const {
        return this->msgType;
    };

    inline std::string get_type_string() const {
        return message_type_to_string(this->get_type());
    }

    inline uint8_t get_channel() const {
        return this->data[0] & 0x0F;
    };

    inline uint8_t get_pitch() const {
        return this->data[1];
    };

    inline uint8_t get_velocity() const {
        return this->data[2];
    };

    inline uint8_t get_control_number() const {
        return this->data[1];
    };

    inline uint8_t get_control_value() const {
        return this->data[2];
    };

    inline uint8_t get_program() const {
        return this->data[1];
    };

    inline MetaType get_meta_type() const {
        return status_to_meta_type(this->data[1]);
    };

    inline std::string get_meta_type_string() const {
        return meta_type_to_string(this->get_meta_type());
    }

    inline container::Bytes get_meta_value() const {
        return container::Bytes(this->data.begin() + 3, this->data.end());
    };

    inline uint32_t get_tempo() const {
        return utils::read_msb_bytes(const_cast<uint8_t*>(this->data.data()) + 3, 3);
    };

    inline TimeSignature get_time_signature() const {
        return TimeSignature {this->data[3], static_cast<uint8_t>(1 << this->data[4])};
    };

    inline KeySignature get_key_signature() const {
        return KeySignature {static_cast<int8_t>(this->data[3]), this->data[4]};
    }

};

std::ostream& operator<<(std::ostream& out, const container::Bytes& data) {
    out << std::hex << std::setfill('0') << "{ ";
    for(auto &d: data) {
        out << "0x" << std::setw(2) << (int)d << " ";
    }
    out << "}" << std::dec << std::endl;

    return out;
};

std::ostream& operator<<(std::ostream& out, const Message& message) {
    out << "time=" << message.get_time() << " | [";
    out << message.get_type_string() << "] ";
    
    switch(message.get_type()) {
        case(message::MessageType::NoteOn): {
            out << "channel=" << (int)message.get_channel() << " pitch=" << (int)message.get_pitch() << " velocity=" << (int)message.get_velocity();
            break;
        };
        case(message::MessageType::NoteOff): {
            out << "channel=" << (int)message.get_channel() << " pitch=" << (int)message.get_pitch() << " velocity=" << (int)message.get_velocity();
            break;
        };
        case(message::MessageType::ProgramChange): {
            out << "channel=" << (int)message.get_channel() << " program=" << (int)message.get_program();
            break;
        };
        case(message::MessageType::ControlChange): {
            out << "channel=" << (int)message.get_channel() << " control number=" << (int)message.get_control_number() << " control value=" << (int)message.get_control_value();
            break;
        };
        case(message::MessageType::Meta): {
            out << "(" << message.get_meta_type_string() << ") ";
            switch(message.get_meta_type()) {
                case(message::MetaType::TrackName): {
                    container::Bytes data = message.get_meta_value();
                    out << std::string(data.begin(), data.end());
                    break;
                };
                case(message::MetaType::InstrumentName): {
                    container::Bytes data = message.get_meta_value();
                    out << std::string(data.begin(), data.end());
                    break;
                };
                case(message::MetaType::TimeSignature): {
                    message::TimeSignature timeSig = message.get_time_signature();
                    out << (int)timeSig.numerator << "/" << (int)timeSig.denominator;
                    break;
                };
                case(message::MetaType::SetTempo): {
                    out << (int)message.get_tempo();
                    break;
                };
                case(message::MetaType::KeySignature): {
                    out << message.get_key_signature().to_string();
                    break;
                }
                case(message::MetaType::EndOfTrack): {
                    break;
                }
                default: {
                    container::Bytes data = message.get_meta_value();
                    out << (int)message.get_meta_type() << " value=" << message.get_data();
                    // utils::print_bytes(message.get_data());
                    break;
                }
            }
            break;
        };
        default: {
            out << "Status code: " << (int)message::message_attr(message.get_type()).status << " length=" << message.get_data().size();
            break;
        };
    }

    return out;
};

typedef std::vector<message::Message> Messages;

}


namespace track {

class Track {
private:
    message::Messages messages;

public:
    Track() = default;
    Track(const container::Bytes& data) {
        uint8_t* cursor = const_cast<uint8_t*>(data.data());
        uint8_t* bufferEnd = cursor + data.size();

        uint32_t tickOffset = 0;
        uint8_t prevStatusCode = 0x00;
        size_t prevEventLen = 0;

        while(cursor < bufferEnd) {
            tickOffset += utils::read_variable_length(cursor);
            container::Bytes messageData;

            // Running status
            if((*cursor) < 0x80) {
                messageData = container::Bytes(prevEventLen);

                if(!prevEventLen)
                    throw "Corrupted MIDI File.";

                messageData[0] = prevStatusCode;
                std::copy(cursor, cursor + prevEventLen - 1, messageData.begin() + 1);
                cursor += prevEventLen - 1;
            }
            // Meta message
            else if((*cursor) == 0xFF) {
                prevStatusCode = (*cursor);
                uint8_t* prevBuffer = cursor;
                cursor += 2;
                prevEventLen = utils::read_variable_length(cursor) + (cursor - prevBuffer);

                if(prevBuffer + prevEventLen > bufferEnd)
                    throw "Unexpected EOF of Meta Event!";

                messageData = std::vector(prevBuffer, prevBuffer + prevEventLen);
                cursor += prevEventLen - (cursor - prevBuffer);
            }
            // SysEx message
            else if((*cursor) == 0xF0) {
                prevStatusCode = (*cursor);
                uint8_t* prevBuffer = cursor;
                cursor += 1;
                prevEventLen = utils::read_variable_length(cursor) + (cursor - prevBuffer);

                if(prevBuffer + prevEventLen > bufferEnd)
                    throw "Unexpected EOF of SysEx Event!";

                messageData = std::vector(prevBuffer, prevBuffer + prevEventLen);
                cursor += prevEventLen - (cursor - prevBuffer);
            }
            // Channel message or system common message
            else {
                prevStatusCode = (*cursor);
                prevEventLen = message::message_attr(message::status_to_message_type(*cursor)).length;

                messageData = std::vector(cursor, cursor + prevEventLen);
                cursor += prevEventLen;
            }

            if(cursor > bufferEnd) {
                throw "Unexpected EOF in track.";
            }

            message::Message msg = message::Message(tickOffset, messageData);
            this->messages.push_back(msg);

            if(msg.get_type() == message::MessageType::Meta &&
                msg.get_meta_type() == message::MetaType::EndOfTrack) {
                    break;
            }
        }
    };

    Track(message::Messages message) {
        this->messages = std::vector(message);
    };

    inline message::Message& message(uint32_t index) {
        return this->messages[index];
    };

    inline size_t message_num() const {
        return this->messages.size();
    };
};

std::ostream& operator<<(std::ostream& out, Track& track) {
    for (int j = 0; j < track.message_num(); ++j)
    {
        out << track.message(j) << std::endl;
    }

    return out;
};

typedef std::vector<Track> Tracks;

}


namespace file {

#define MIDI_FORMAT                       \
    MIDI_FORMAT_MEMBER(SingleTrack, 0)    \
    MIDI_FORMAT_MEMBER(MultiTrack, 1)     \
    MIDI_FORMAT_MEMBER(MultiSong, 2)      \

enum class MidiFormat {
    #define MIDI_FORMAT_MEMBER(type, status) type = status,
    MIDI_FORMAT
    #undef MIDI_FORMAT_MEMBER
};

inline const std::string& format_to_string(const MidiFormat& format) {
    static const std::string formats[] = {
        #define MIDI_FORMAT_MEMBER(type, status) #type,
        MIDI_FORMAT
        #undef MIDI_FORMAT_MEMBER
    };

    return formats[(int)format];
};

inline MidiFormat read_midiformat(uint16_t data) {
    switch(data) {
        #define MIDI_FORMAT_MEMBER(type, status) case status: return MidiFormat::type;
        MIDI_FORMAT
        #undef MIDI_FORMAT_MEMBER
    }

    throw "Invaild midi format!";
};

class MidiFile {
private:
    MidiFormat format;
    uint16_t divisionType: 1;
    union {
        struct { uint16_t ticksPerQuarter: 15; };
        struct { uint16_t negtiveSmpte: 7; uint16_t ticksPerFrame: 8; };
    };
    track::Tracks tracks;

public:
    MidiFile() = default;
    MidiFile(const container::Bytes& data) {
        if(data.size() < 4)
            throw "Invaild midi file!";

        uint8_t* cursor = const_cast<uint8_t*>(data.data());
        uint8_t* bufferEnd = cursor + data.size();

        if(!(!strncmp(reinterpret_cast<const char*>(cursor), "MThd", 4) &&
            utils::read_msb_bytes(cursor + 4, 4) == 6
            ))
            throw "MThd excepted!";

        this->format = read_midiformat(utils::read_msb_bytes(cursor + 8, 2));
        uint16_t trackNum = utils::read_msb_bytes(cursor + 10, 2);
        this->divisionType = (*(cursor + 12)) & 0x80;
        this->ticksPerQuarter = (((*(cursor + 12)) & 0x7F) << 8) + (*(cursor + 13));

        cursor += 14;

        for (int i = 0; i < trackNum; ++i)
        {
            // Skip unknown chunk
            while(!strncmp(reinterpret_cast<const char*>(cursor), "MThd", 4)) {
                size_t chunkLen = utils::read_msb_bytes(cursor + 4, 4);
                cursor += (8 + chunkLen);
                continue;
            }

            size_t chunkLen = utils::read_msb_bytes(cursor + 4, 4);

            if(cursor + chunkLen + 8 > bufferEnd)
                throw "Unexpected EOF in file!";

            this->tracks.emplace_back(track::Track(container::Bytes(cursor + 8, cursor + 8 + chunkLen)));
            cursor += (8 + chunkLen);
        }
    }

    static MidiFile from_file(const std::string& filepath) {
        FILE* filePtr = fopen(filepath.c_str(), "rb");

        if(!filePtr)
            throw "Reading file failed!";

        fseek(filePtr, 0, SEEK_END);
        size_t fileLen = ftell(filePtr);

        container::Bytes data = container::Bytes(fileLen);
        fseek(filePtr, 0, SEEK_SET);
        fread(data.data(), 1, fileLen, filePtr);
        fclose(filePtr);

        return MidiFile(data);
    };

    inline MidiFormat get_format() const {
        return this->format;
    };

    inline std::string get_format_string() const {
        return format_to_string(this->get_format());
    }

    inline uint16_t get_division_type() const {
        return static_cast<uint16_t>(this->divisionType);
    };

    inline uint16_t get_tick_per_quarter() const {
        if(!this->get_division_type()) return this->ticksPerQuarter;
        else { std::cerr << "Division type 1 have no tpq." << std::endl; return -1; };
    };

    inline uint16_t get_frame_per_second() const {
        return (~(this->negtiveSmpte - 1)) & 0x3F;
    };

    inline uint16_t get_tick_per_second() const {
        if(this->get_division_type()) return this->ticksPerFrame * this->get_frame_per_second();
        else { std::cerr << "Division type 0 have no tps." << std::endl; return -1; };
    };

    inline track::Track& track(uint32_t index) {
        return this->tracks[index];
    };

    inline size_t track_num() const {
        return this->tracks.size();
    };
};

#undef MIDI_FORMAT

std::ostream& operator<<(std::ostream& out, MidiFile& file) {
    out << "File format: " << file.get_format_string() << std::endl;
    out << "Division:\n" << "    Type: " << file.get_division_type() << std::endl;
    if(file.get_division_type()) {
        out << "    Tick per Second: " << file.get_tick_per_second() << std::endl;
    }
    else {
        out << "    Tick per Quarter: " << file.get_tick_per_quarter() << std::endl;
    }

    out << std::endl;

    for(int i = 0; i < file.track_num(); ++i)
    {
        out << "Track " << i << ": " << std::endl;
        out << file.track(i) << std::endl;
    }

    return out;
};

}

}


#endif