#ifndef __MINIMIDI_HPP
#define __MINIMIDI_HPP

#include<algorithm>
#include<cstdint>
#include<cstddef>
#include<vector>
#include<string>
#include<cstdlib>
#include<iostream>
#include<string>
#include<iomanip>
#include<cstring>
#include<cmath>
#include<span>


namespace minimidi {

namespace container {

typedef std::vector<uint8_t> Bytes;
typedef std::span<const uint8_t> ByteSpan;

}

namespace utils {

inline container::Bytes read_file(const std::string& filepath) {
    FILE* filePtr = fopen(filepath.c_str(), "rb");

    if(!filePtr)
        throw std::ios_base::failure("Reading file failed!");

    fseek(filePtr, 0, SEEK_END);
    size_t fileLen = ftell(filePtr);

    container::Bytes data = container::Bytes(fileLen);
    fseek(filePtr, 0, SEEK_SET);
    fread(data.data(), 1, fileLen, filePtr);
    fclose(filePtr);

    return data;
}

inline uint32_t read_variable_length(container::ByteSpan buffer, size_t& cursor) {
    uint32_t value = 0;

    for(auto i = 0; i < 4; i++) {
        value = (value << 7) + (buffer[cursor] & 0x7f);
        if(!(buffer[cursor] & 0x80)) break;
        cursor++;
    }

    cursor++;
    return value;
}

inline uint64_t read_msb_bytes(container::ByteSpan buffer) {
    uint64_t res = 0;

    for(auto i = 0; i < buffer.size(); i++) {
        res <<= 8;
        res += buffer[i];
    }

    return res;
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

const std::array KEYS {"bC", "bG", "bD", "bA", "bE", "bB", "F", "C", "G", "D", "A", "E", "B", "#F", "#C", "bc", "bg", "bd", "ba", "be", "bb", "f", "c", "g", "d", "a", "e", "b", "#f", "#c"};

class KeySignature {
public:
    int8_t key;
    uint8_t tonality;

    inline const std::string to_string() {
        return KEYS[this->key + 7 + KEYS.size() / 2 * this->tonality];
    };

    inline static KeySignature from_string(const std::string& ks) {
        size_t index = std::find_if(KEYS.begin(), KEYS.end(), [&ks] (const std::string& s) {return !s.compare(ks);}) - KEYS.data();

        double tonality;
        double key = modf(index, &tonality) - 7;

        return KeySignature {static_cast<int8_t>(key), static_cast<uint8_t>(tonality)};
    };
};

class Message {
protected:
    uint32_t time;
    container::Bytes data;
    MessageType msgType;

public:
    Message() = default;
    Message(uint32_t time, const container::ByteSpan& msgData) {
        this->time = time;
        this->msgType = status_to_message_type(msgData[0]);

        this->data = container::Bytes(msgData.begin(), msgData.end());
    };

    Message(uint32_t time, const container::Bytes& msgData) {
        this->time = time;
        this->msgType = status_to_message_type(msgData[0]);

        this->data = msgData;
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
        return utils::read_msb_bytes(std::span{this->data}.subspan(3, 3));
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

class MessageIter {
protected:
    size_t cursor;
    container::ByteSpan data;
    uint32_t tickOffset;
    uint8_t prevStatusCode;
    size_t prevEventLen;
    bool is_eot;

public:
    MessageIter(const container::ByteSpan& data) {
        cursor = 0;
        this->data = data;
        is_eot = false;

        tickOffset = 0;
        prevStatusCode = 0x00;
        prevEventLen = 0;
    };

    MessageIter() = default;

    inline message::Message read_a_message() {
        if(is_eot)
            throw std::out_of_range("There is no message to read.");

        tickOffset += utils::read_variable_length(data, cursor);

        // Running status
        if(data[cursor] < 0x80) {
            container::Bytes msgData = container::Bytes(prevEventLen);
            container::ByteSpan msgSpan = data.subspan(cursor, prevEventLen - 1);
            cursor += prevEventLen - 1;

            if(!prevEventLen)
                throw std::runtime_error("Corrupted MIDI File.");

            msgData[0] = prevStatusCode;
            std::copy(msgSpan.begin(), msgSpan.end(), msgData.begin() + 1);
            return message::Message(tickOffset, msgData);
        }
        // Meta message
        else if(data[cursor] == 0xFF) {
            prevStatusCode = 0xFF;
            size_t prevCursor = cursor;
            cursor += 2;
            prevEventLen = utils::read_variable_length(data, cursor) + (cursor - prevCursor);
            cursor += prevEventLen - (cursor - prevCursor);

            message::Message curMessage = message::Message(tickOffset, data.subspan(prevCursor, prevEventLen));
            if(curMessage.get_meta_type() == message::MetaType::EndOfTrack) {
                is_eot = true;
            }
            return curMessage;
        }
        // SysEx message
        else if(data[cursor] == 0xF0) {
            prevStatusCode = 0xF0;
            size_t prevCursor = cursor;
            cursor += 1;
            prevEventLen = utils::read_variable_length(data, cursor) + (cursor - prevCursor);
            cursor += prevEventLen - (cursor - prevCursor);

            // subsspan() will do EOF Detection.
            return message::Message(tickOffset, data.subspan(prevCursor, prevEventLen));
        }
        // Channel message or system common message
        else {
            prevStatusCode = data[cursor];
            prevEventLen = message::message_attr(message::status_to_message_type(prevStatusCode)).length;
            cursor += prevEventLen;

            return message::Message(tickOffset, data.subspan(cursor, prevEventLen));
        }
    };

    inline bool is_empty() const {
        return is_eot;
    }
};

class Track {
protected:
    message::Messages messages;

public:
    Track() = default;

    Track(MessageIter& iter) {
        while(!iter.is_empty()) {
            messages.emplace_back(iter.read_a_message());
        }
    };

    Track(const container::ByteSpan& data) {
        MessageIter iter(data);

        while(!iter.is_empty()) {
            messages.emplace_back(iter.read_a_message());
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

    throw std::runtime_error("Invaild midi format!");
};

class TrackIter {
protected:
    size_t cursor;
    uint16_t tracksRemain;
    container::ByteSpan data;

    void skip_unknown_chunk() {
        while(!strncmp(reinterpret_cast<const char*>(data.subspan(cursor, 4).data()), "MThd", 4)) {
            size_t chunkLen = utils::read_msb_bytes(data.subspan(cursor + 4, 4));
            cursor += (8 + chunkLen);
            continue;
        }
    };

public:
    TrackIter(const container::ByteSpan data, uint16_t tracksRemain) {
        this->data = data;
        this->cursor = 0;
        this->tracksRemain = tracksRemain;
    };
    TrackIter() = default;

    inline container::ByteSpan read_a_chunk() {
        skip_unknown_chunk();
        tracksRemain--;

        size_t chunkLen = utils::read_msb_bytes(data.subspan(cursor + 4, 4));
        size_t chunkStart = cursor + 8;
        cursor += (8 + chunkLen);

        return data.subspan(chunkStart, chunkStart + chunkLen);
    }

    inline track::Track read_a_track() {
        return track::Track(read_a_chunk());
    };

    inline track::MessageIter read_a_message_iter() {
        return track::MessageIter(read_a_chunk());
    };


    inline bool is_empty() const {
        return !tracksRemain;
    }
};

class MidiFileBase {
protected:
    MidiFormat format;
    uint16_t divisionType: 1;
    union {
        struct { uint16_t ticksPerQuarter: 15; };
        struct { uint16_t negtiveSmpte: 7; uint16_t ticksPerFrame: 8; };
    };
    uint16_t trackNum;

    inline void read_head(container::ByteSpan data) {
        if(!(!strncmp(reinterpret_cast<const char*>(data.data()), "MThd", 4) &&
            utils::read_msb_bytes(data.subspan(4, 4)) == 6
            ))
            throw std::runtime_error("MThd excepted!");

        this->format = read_midiformat(utils::read_msb_bytes(data.subspan(8, 2)));
        this->divisionType = data[12] & 0x80;
        this->ticksPerQuarter = ((data[12] & 0x7F) << 8) + data[13];
        this->trackNum = utils::read_msb_bytes(data.subspan(10, 2));
    };

public:
    MidiFileBase() = default;
    MidiFileBase(const container::ByteSpan& data) {
        if(data.size() < 4)
            throw std::runtime_error("Invaild midi file!");

        read_head(data);
    }

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

    inline size_t track_num() const {
        return this->trackNum;
    };
};

#undef MIDI_FORMAT

class MidiFileIter : public MidiFileBase {
protected:
    TrackIter trackIter;
    container::Bytes data;
public:
    MidiFileIter() = default;
    MidiFileIter(const container::ByteSpan& data) : MidiFileBase(data) {
        this->data = container::Bytes(data.begin(), data.end());
        size_t cursor = 14;

        trackIter = TrackIter(data.subspan(cursor, data.size() - cursor), trackNum);
    }

    static MidiFileIter from_file(const std::string& filepath) {
        container::Bytes data = utils::read_file(filepath);
        container::ByteSpan dataSpan(data.data(), data.size());
        return MidiFileIter(dataSpan);
    };

    inline TrackIter& track_iter() {
        return this->trackIter;
    };
};

class MidiFile : public MidiFileBase {
protected:
    track::Tracks tracks;
public:
    MidiFile() = default;
    MidiFile(const container::ByteSpan& data) : MidiFileBase(data) {
        size_t cursor = 14;

        TrackIter iter(data.subspan(cursor, data.size() - cursor), trackNum);
        while(!iter.is_empty()) {
            tracks.emplace_back(iter.read_a_track());
        }
    };

    static MidiFile from_file(const std::string& filepath) {
        container::Bytes data = utils::read_file(filepath);
        container::ByteSpan dataSpan(data.data(), data.size());
        return MidiFile(dataSpan);
    };

    inline track::Track& track(uint32_t index) {
        return this->tracks[index];
    };
};

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