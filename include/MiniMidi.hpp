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
#include<numeric>
#include"svector.h"


namespace minimidi {

namespace container {

typedef std::vector <uint8_t> Bytes;
typedef ankerl::svector<uint8_t, 7> SmallBytes;
typedef std::span<const uint8_t> ByteSpan;

inline void check_span_boundary(const ByteSpan &data, size_t index) {
    if (index >= data.size())
        throw std::out_of_range("Span index is out of range!");
}

// to_string func for SmallBytes
inline std::string to_string(const SmallBytes &data) {
    // show in hex
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << "{ ";
    for (auto &d: data)
        ss << std::setw(2) << (int) d << " ";
    ss << "}" << std::dec;
    return ss.str();
};

}

namespace utils {

inline container::Bytes read_file(const std::string &filepath) {
    FILE *filePtr = fopen(filepath.c_str(), "rb");

    if (!filePtr)
        throw std::ios_base::failure("Reading file failed!");

    fseek(filePtr, 0, SEEK_END);
    size_t fileLen = ftell(filePtr);

    container::Bytes data = container::Bytes(fileLen);
    fseek(filePtr, 0, SEEK_SET);
    fread(data.data(), 1, fileLen, filePtr);
    fclose(filePtr);

    return data;
};

inline uint32_t read_variable_length(container::ByteSpan buffer, size_t &cursor) {
    uint32_t value = 0;

    for (auto i = 0; i < 4; i++) {
        value = (value << 7) + (buffer[cursor] & 0x7f);
        if (!(buffer[cursor] & 0x80)) break;
        cursor++;
    }

    cursor++;
    return value;
};

inline container::Bytes make_variable_length(uint32_t num) {
    uint8_t byteNum = ceil(log2(num + 1) / 7);
    container::Bytes result(byteNum);

    for (auto i = 0; i < byteNum; i++) {
        result[i] = (num >> (7 * (byteNum - i - 1)));
        result[i] |= 0x80;
    }
    result.back() &= 0x7F;

    return result;
};

inline uint64_t read_msb_bytes(container::ByteSpan buffer) {
    uint64_t res = 0;

    for (auto i = 0; i < buffer.size(); i++) {
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

inline const std::string message_type_to_string(const MessageType &messageType) {
    switch (messageType) {
#define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) \
        case MessageType::type: return #type;
        MIDI_MESSAGE_TYPE
#undef MIDI_MESSAGE_TYPE_MEMBER
    };

    return "Unknown";
};

typedef struct {
    uint8_t status;
    size_t length;
} MessageAttr;

inline const MessageAttr &message_attr(const MessageType &messageType) {
    static const MessageAttr MESSAGE_ATTRS[] = {
#define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) {status, length},
        MIDI_MESSAGE_TYPE
#undef MIDI_MESSAGE_TYPE_MEMBER
    };
    return MESSAGE_ATTRS[static_cast<std::underlying_type<MessageType>::type>(messageType)];
};

inline MessageType status_to_message_type(uint8_t status) {
    if (status < 0xF0) {
        switch (status & 0xF0) {
#define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) \
                case status: return MessageType::type;
            MIDI_MESSAGE_TYPE
#undef MIDI_MESSAGE_TYPE_MEMBER
        }
    } else {
        switch (status) {
#define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) \
                case status: return MessageType::type;
            MIDI_MESSAGE_TYPE
#undef MIDI_MESSAGE_TYPE_MEMBER
        }
    }

    return MessageType::Unknown;
};

enum class MetaType : uint8_t {
#define MIDI_META_TYPE_MEMBER(type, status) type = status,
    MIDI_META_TYPE
#undef MIDI_META_TYPE_MEMBER
};

inline MetaType status_to_meta_type(uint8_t status) {
    switch (status) {
#define MIDI_META_TYPE_MEMBER(type, status) \
            case status: return MetaType::type;
        MIDI_META_TYPE
#undef MIDI_META_TYPE_MEMBER
    }

    return MetaType::Unknown;
};

inline const std::string meta_type_to_string(const MetaType &metaType) {
    switch (metaType) {
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

const std::array KEYS{"bC", "bG", "bD", "bA", "bE", "bB", "F", "C", "G", "D", "A", "E", "B", "#F", "#C", "bc", "bg",
                      "bd", "ba", "be", "bb", "f", "c", "g", "d", "a", "e", "b", "#f", "#c"};

class KeySignature {
public:
    int8_t key;
    uint8_t tonality;

    inline const std::string to_string() {
        return KEYS[this->key + 7 + KEYS.size() / 2 * this->tonality];
    };

    inline static KeySignature from_string(const std::string &ks) {
        size_t index = std::find_if(KEYS.begin(), KEYS.end(), [&ks](const std::string &s) { return !s.compare(ks); }) -
                       KEYS.data();

        double tonality;
        double key = modf(index, &tonality) - 7;

        return KeySignature{static_cast<int8_t>(key), static_cast<uint8_t>(tonality)};
    };
};

class Message {
protected:
    uint32_t time;
    container::SmallBytes data;
    MessageType msgType;

    Message(uint32_t time, const MessageType &type) {
        this->time = time;
        this->msgType = type;
    }

public:
    Message() = default;

    Message(uint32_t time, const container::ByteSpan &msgData) {
        this->time = time;
        this->msgType = status_to_message_type(msgData[0]);

        this->data = container::SmallBytes(msgData.begin(), msgData.end());
    };

    Message(uint32_t time, const container::SmallBytes &msgData) {
        this->time = time;
        this->msgType = status_to_message_type(msgData[0]);

        this->data = container::SmallBytes(msgData);
    };

    /*
    Message(uint32_t time, const MessageType& type) : Message(time, type) {
        if(!(type == MessageType::TuneRequest |
            type == MessageType::SysExEnd |
            type == MessageType::TimingClock |
            type == MessageType::StartSequence |
            type == MessageType::ContinueSequence |
            type == MessageType::StopSequence |
            type == MessageType::ActiveSensing))
            throw std::invalid_argument("The argument combination should be these message type.");

        this->data = container::Bytes(1);
        this->data[0] = uint8_t(type);
    }
    */

    Message(uint32_t time, const MessageType &type, const MetaType &metaType) : Message(time, type) {
        if (!(type == MessageType::Meta &
              metaType == MetaType::EndOfTrack))
            throw std::invalid_argument("The argument combination should be these message type.");
        this->data = container::SmallBytes (3);
        this->data[0] = uint8_t(type);
        this->data[1] = uint8_t(metaType);
        this->data[2] = 0x00;
    };

    inline uint32_t get_time() const {
        return this->time;
    };

    inline container::SmallBytes get_data() const {
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

    inline container::SmallBytes get_meta_value() const {
        return {this->data.begin() + 3, this->data.end()};
    };

    inline uint32_t get_tempo() const {
        return utils::read_msb_bytes(container::ByteSpan{this->data}.subspan(3, 3));
    };

    inline double get_qpm() const {
        return 60000000.f / static_cast<double>(get_tempo());
    }

    inline TimeSignature get_time_signature() const {
        return TimeSignature{this->data[3], static_cast<uint8_t>(1 << this->data[4])};
    };

    inline KeySignature get_key_signature() const {
        return KeySignature{static_cast<int8_t>(this->data[3]), this->data[4]};
    }

};

std::ostream &operator<<(std::ostream &out, const container::Bytes &data) {
    out << std::hex << std::setfill('0') << "{ ";
    for (auto &d: data) {
        out << "0x" << std::setw(2) << (int) d << " ";
    }
    out << "}" << std::dec << std::endl;

    return out;
};

std::ostream &operator<<(std::ostream &out, const Message &message) {
    out << "time=" << message.get_time() << " | [";
    out << message.get_type_string() << "] ";

    switch (message.get_type()) {
        case (message::MessageType::NoteOn): {
            out << "channel=" << (int) message.get_channel() << " pitch=" << (int) message.get_pitch() << " velocity="
                << (int) message.get_velocity();
            break;
        };
        case (message::MessageType::NoteOff): {
            out << "channel=" << (int) message.get_channel() << " pitch=" << (int) message.get_pitch() << " velocity="
                << (int) message.get_velocity();
            break;
        };
        case (message::MessageType::ProgramChange): {
            out << "channel=" << (int) message.get_channel() << " program=" << (int) message.get_program();
            break;
        };
        case (message::MessageType::ControlChange): {
            out << "channel=" << (int) message.get_channel() << " control number=" << (int) message.get_control_number()
                << " control value=" << (int) message.get_control_value();
            break;
        };
        case (message::MessageType::Meta): {
            out << "(" << message.get_meta_type_string() << ") ";
            switch (message.get_meta_type()) {
                case (message::MetaType::TrackName): {
                    auto data = message.get_meta_value();
                    out << std::string(data.begin(), data.end());
                    break;
                };
                case (message::MetaType::InstrumentName): {
                    auto data = message.get_meta_value();
                    out << std::string(data.begin(), data.end());
                    break;
                };
                case (message::MetaType::TimeSignature): {
                    auto timeSig = message.get_time_signature();
                    out << (int) timeSig.numerator << "/" << (int) timeSig.denominator;
                    break;
                };
                case (message::MetaType::SetTempo): {
                    out << (int) message.get_tempo();
                    break;
                };
                case (message::MetaType::KeySignature): {
                    out << message.get_key_signature().to_string();
                    break;
                }
                case (message::MetaType::EndOfTrack): {
                    break;
                }
                default: {
                    auto data = message.get_meta_value();
                    out << (int) message.get_meta_type() << " value=" << container::to_string(message.get_data());
                    // utils::print_bytes(message.get_data());
                    break;
                }
            }
            break;
        };
        default: {
            out << "Status code: " << (int) message::message_attr(message.get_type()).status << " length="
                << message.get_data().size();
            break;
        };
    }

    return out;
};

typedef std::vector <message::Message> Messages;

}


namespace track {

class TrackStream {
protected:
    container::ByteSpan data;

public:
    TrackStream() = default;

    TrackStream(container::ByteSpan data) : data(data) {};

    class Iterator {
    protected:
        size_t cursor;
        container::ByteSpan data;
        uint32_t tickOffset;
        uint8_t prevStatusCode;
        size_t prevEventLen;
        bool is_eot;
        message::Message curMessage;

        inline message::Message read_a_message() {
            if (is_eot) {
                cursor += 1;
                return this->curMessage;
            }
//                throw std::out_of_range("There is no message to read.");
            tickOffset += utils::read_variable_length(data, cursor);
            container::check_span_boundary(data, cursor);

            // Running status
            if (data[cursor] < 0x80) {
                auto msgData = container::SmallBytes(prevEventLen);
                container::check_span_boundary(data, cursor + prevEventLen - 1);
                container::ByteSpan msgSpan = data.subspan(cursor, prevEventLen - 1);
                cursor += prevEventLen - 1;

                if (!prevEventLen)
                    throw std::runtime_error("Corrupted MIDI File.");

                msgData[0] = prevStatusCode;
                std::copy(msgSpan.begin(), msgSpan.end(), msgData.begin() + 1);
                return message::Message(tickOffset, msgData);
            }
                // Meta message
            else if (data[cursor] == 0xFF) {
                prevStatusCode = 0xFF;
                size_t prevCursor = cursor;
                cursor += 2;
                prevEventLen = utils::read_variable_length(data, cursor) + (cursor - prevCursor);
                cursor += prevEventLen - (cursor - prevCursor);

                container::check_span_boundary(data, prevCursor + prevEventLen - 1);
                message::Message curMessage = message::Message(tickOffset, data.subspan(prevCursor, prevEventLen));
                if (curMessage.get_meta_type() == message::MetaType::EndOfTrack) {
                    is_eot = true;
                }
                return curMessage;
            }
                // SysEx message
            else if (data[cursor] == 0xF0) {
                prevStatusCode = 0xF0;
                size_t prevCursor = cursor;
                cursor += 1;
                prevEventLen = utils::read_variable_length(data, cursor) + (cursor - prevCursor);
                cursor += prevEventLen - (cursor - prevCursor);

                container::check_span_boundary(data, prevCursor + prevEventLen - 1);
                return message::Message(tickOffset, data.subspan(prevCursor, prevEventLen));
            }
                // Channel message or system common message
            else {
                prevStatusCode = data[cursor];
                prevEventLen = message::message_attr(message::status_to_message_type(prevStatusCode)).length;
                cursor += prevEventLen;

                container::check_span_boundary(data, cursor + prevEventLen - 1);
                return message::Message(tickOffset, data.subspan(cursor - prevEventLen, cursor));
            }
        };


    public:
        Iterator(
            container::ByteSpan data, size_t cursor, uint32_t tickOffset,
            uint8_t prevStatusCode, size_t prevEventLen, bool is_eot
        ) : data(data), cursor(cursor), tickOffset(tickOffset), prevStatusCode(prevStatusCode),
            prevEventLen(prevEventLen), is_eot(is_eot) {};

        Iterator(container::ByteSpan data) :
            data(data), cursor(0), tickOffset(0),
            prevStatusCode(0), prevEventLen(0), is_eot(false) {
            curMessage = read_a_message();
        };

        static Iterator end(container::ByteSpan data) {
            return Iterator(data, data.size() + 1, 0, 0, 0, true);
        };

        inline bool operator==(const Iterator &other) const {
            return this->cursor == other.cursor;
        };

        inline bool operator!=(const Iterator &other) const {
            return this->cursor != other.cursor;
        };

        inline Iterator &operator++() {
            this->curMessage = read_a_message();
            return *this;
        };

        inline const message::Message &operator*() const {
            return this->curMessage;
        };

        inline message::Message operator*() {
            return this->curMessage;
        };

    };

    inline Iterator begin() {
        return Iterator(this->data);
    };

    inline Iterator end() {
        return Iterator::end(this->data);
    };
};

class Track {
protected:
    message::Messages messages;
public:
    Track() = default;

    Track(const container::ByteSpan &data) {
        TrackStream stream(data);
        for (auto msg: stream) {
            this->messages.emplace_back(msg);
        }
    }

    Track(const container::Bytes &data) {
        TrackStream stream{container::ByteSpan(data)};
        for (auto msg: stream) {
            this->messages.emplace_back(msg);
        }
    }

    Track(const message::Messages &messages) : messages(messages) {};

    inline message::Message &message(size_t index) {
        return this->messages[index];
    };

    inline const message::Message &message(size_t index) const {
        return this->messages[index];
    };

    inline message::Message &operator[](size_t index) {
        return this->messages[index];
    };

    inline const message::Message &operator[](size_t index) const {
        return this->messages[index];
    };

    inline message::Message &at(size_t index) {
        return this->messages.at(index);
    };

    inline const message::Message &at(size_t index) const {
        return this->messages.at(index);
    };

    inline message::Messages::iterator begin() {
        return this->messages.begin();
    };

    inline message::Messages::iterator end() {
        return this->messages.end();
    };

    inline size_t message_num() const {
        return this->messages.size();
    };

    inline void push_back(const message::Message& msg) {
        this->messages.push_back(message::Message(msg));
    }

    inline container::Bytes to_bytes() {
        message::Messages messages_to_bytes(messages.size());
        std::copy_if(messages.begin(),
                     messages.end(),
                     messages_to_bytes.end(),
                     [](const message::Message &msg) {
                         return !(msg.get_type() != message::MessageType::Meta && \
                                msg.get_meta_type() != message::MetaType::EndOfTrack);
                     });

        std::sort(messages_to_bytes.begin(),
                  messages_to_bytes.end(),
                  [](const message::Message &msg1, const message::Message &msg2) {
                      return msg1.get_time() < msg2.get_time();
                  });
        messages_to_bytes.emplace_back(message::Message(messages_to_bytes.back().get_time(),
                                                        message::MessageType::Meta,
                                                        message::MetaType::EndOfTrack));

        std::vector <uint32_t> times(messages_to_bytes.size());
        std::vector <size_t> dataLen(messages_to_bytes.size());
        for (int i = 0; i < messages_to_bytes.size(); ++i) {
            times[i] = messages_to_bytes[i].get_time();
            dataLen[i] = messages_to_bytes[i].get_data().size();
        }
        std::adjacent_difference(times.begin(), times.end(), times.begin());

        container::Bytes result(std::reduce(dataLen.begin(), dataLen.end()) + 4 * messages_to_bytes.size() + 3);
        size_t cursor = 0;
        for (int i = 0; i < messages_to_bytes.size(); ++i) {
            container::Bytes timeData = utils::make_variable_length(times[i]);
            std::copy(timeData.begin(), timeData.end(), result.begin() + cursor);
            cursor += timeData.size();

            std::copy(messages_to_bytes[i].get_data().begin(), messages_to_bytes[i].get_data().end(),
                      result.begin() + cursor);
            cursor += messages_to_bytes[i].get_data().size();
        }

        result.resize(cursor);
        result.shrink_to_fit();
        return result;
    }
};


std::ostream &operator<<(std::ostream &out, Track &track) {
    for (int j = 0; j < track.message_num(); ++j) {
        out << track.message(j) << std::endl;
    }
    return out;
};

typedef std::vector <Track> Tracks;

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

inline const std::string &format_to_string(const MidiFormat &format) {
    static const std::string formats[] = {
#define MIDI_FORMAT_MEMBER(type, status) #type,
        MIDI_FORMAT
#undef MIDI_FORMAT_MEMBER
    };

    return formats[(int) format];
};

inline MidiFormat read_midiformat(uint16_t data) {
    switch (data) {
#define MIDI_FORMAT_MEMBER(type, status) case status: return MidiFormat::type;
        MIDI_FORMAT
#undef MIDI_FORMAT_MEMBER
    }

    throw std::runtime_error("Invaild midi format!");
};


class MidiFileBase {
protected:
    MidiFormat format;
    uint16_t divisionType: 1;
    union {
        struct {
            uint16_t ticksPerQuarter: 15;
        };
        struct {
            uint16_t negtiveSmpte: 7;
            uint16_t ticksPerFrame: 8;
        };
    };
    uint16_t trackNum;

    inline void read_head(const container::ByteSpan &data) {
        container::check_span_boundary(data, 14);
        if (!(!strncmp(reinterpret_cast<const char *>(data.data()), "MThd", 4) &&
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

    MidiFileBase(const container::ByteSpan &data) {
        if (data.size() < 4)
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
        if (!this->get_division_type()) return this->ticksPerQuarter;
        else {
            std::cerr << "Division type 1 have no tpq." << std::endl;
            return -1;
        };
    };

    inline uint16_t get_frame_per_second() const {
        return (~(this->negtiveSmpte - 1)) & 0x3F;
    };

    inline uint16_t get_tick_per_second() const {
        if (this->get_division_type()) return this->ticksPerFrame * this->get_frame_per_second();
        else {
            std::cerr << "Division type 0 have no tps." << std::endl;
            return -1;
        };
    };

    inline size_t track_num() const {
        return this->trackNum;
    };
};

#undef MIDI_FORMAT

class MidiFileStream : public MidiFileBase {
protected:
    container::Bytes data;
public:
    MidiFileStream() = default;

    MidiFileStream(const container::ByteSpan data) : MidiFileBase(data) {
        this->data = container::Bytes(data.begin(), data.end());
    };

    MidiFileStream(const container::Bytes &data) : MidiFileBase(container::ByteSpan(data)) {
        this->data = data;
    };

    MidiFileStream(container::Bytes &&data) : MidiFileBase(container::ByteSpan(data)) {
        this->data = std::move(data);
    };

    static MidiFileStream from_file(const std::string &filepath) {
        container::Bytes data = utils::read_file(filepath);
        return MidiFileStream(std::move(data));
    };

    struct Iterator {
    protected:
        size_t cursor;
        int tracksRemain;
        MidiFileStream &file;
        container::ByteSpan currentChunk;

        void skip_unknown_chunk() {
//            auto &data = this->file.data;
            container::ByteSpan data(this->file.data);
            container::check_span_boundary(data, cursor + 4);
            while (strncmp(reinterpret_cast<const char *>(data.data() + cursor), "MTrk", 4) != 0) {
                size_t chunkLen = utils::read_msb_bytes(data.subspan(cursor + 4, 4));
                cursor += (8 + chunkLen);
                container::check_span_boundary(data, cursor + 4);
                continue;
            }
        };

        inline container::ByteSpan read_a_chunk() {
            skip_unknown_chunk();
//            auto &data = this->file.data;
            container::ByteSpan data(this->file.data);
            container::check_span_boundary(data, cursor + 4);
            size_t chunkLen = utils::read_msb_bytes(data.subspan(cursor + 4, 4));
            cursor += (chunkLen + 8);
            tracksRemain--;

            container::check_span_boundary(data, cursor - 1);
            return data.subspan(cursor - chunkLen, chunkLen);
        };

    public:
        Iterator(MidiFileStream &file, size_t cursor, int tracksRemain) : file(file), cursor(cursor),
                                                                          tracksRemain(tracksRemain) {};

        Iterator(MidiFileStream &file) : file(file), cursor(0), tracksRemain(file.trackNum) {
            this->currentChunk = read_a_chunk();
        };

        inline bool operator==(const Iterator &other) const {
            return this->tracksRemain == other.tracksRemain;
        };

        inline bool operator!=(const Iterator &other) const {
            return this->tracksRemain != other.tracksRemain;
        };

        inline Iterator &operator++() {
            if(tracksRemain != 0)
                this->currentChunk = read_a_chunk();
            else
                tracksRemain--;
            return *this;
        };

        inline track::TrackStream operator*() {
            return track::TrackStream(this->currentChunk);
        };
    };

    Iterator begin() {
        return Iterator(*this);
    };

    Iterator end() {
        return Iterator(*this, this->data.size(), -1);
    };

    inline size_t track_num() {
        return this->trackNum;
    };
};

class MidiFile : public MidiFileBase {
protected:
    track::Tracks tracks;

    inline void read_tracks(const container::ByteSpan data) {
        MidiFileStream stream(data);
        for (auto trackStream : stream) {
            track::Track track;
            for (auto message : trackStream) {
                track.push_back(message);
            }
            this->tracks.push_back(track);
        }
    }
public:
    MidiFile() = default;

    MidiFile(const container::ByteSpan data) : MidiFileBase(data) {
        read_tracks(data);
    };

    MidiFile(const container::Bytes &data) : MidiFileBase(container::ByteSpan(data)) {
        read_tracks(container::ByteSpan(data));
    };

    MidiFile(container::Bytes &&data) : MidiFileBase(container::ByteSpan(data)) {
        read_tracks(container::ByteSpan(data));
    };

    static MidiFile from_file(const std::string &filepath) {
        container::Bytes data = utils::read_file(filepath);
        return MidiFile(data);
    };

    inline track::Track &track(size_t index) {
        return this->tracks[index];
    };

    inline const track::Track &track(size_t index) const {
        return this->tracks[index];
    };

    inline track::Track &operator[](size_t index) {
        return this->tracks[index];
    };

    inline const track::Track &operator[](size_t index) const {
        return this->tracks[index];
    };

    inline track::Track &at(size_t index) {
        return this->tracks.at(index);
    };

    inline const track::Track &at(size_t index) const {
        return this->tracks.at(index);
    };

    inline track::Tracks::iterator begin() {
        return this->tracks.begin();
    };

    inline track::Tracks::iterator end() {
        return this->tracks.end();
    };
};

std::ostream &operator<<(std::ostream &out, MidiFile &file) {
    out << "File format: " << file.get_format_string() << std::endl;
    out << "Division:\n" << "    Type: " << file.get_division_type() << std::endl;
    if (file.get_division_type()) {
        out << "    Tick per Second: " << file.get_tick_per_second() << std::endl;
    } else {
        out << "    Tick per Quarter: " << file.get_tick_per_quarter() << std::endl;
    }

    out << std::endl;

    for (int i = 0; i < file.track_num(); ++i) {
        out << "Track " << i << ": " << std::endl;
        out << file.track(i) << std::endl;
    }

    return out;
};

}

}


#endif