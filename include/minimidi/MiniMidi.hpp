#ifndef MINIMIDI_HPP
#define MINIMIDI_HPP

// used for ignoring warning C4996 (MSCV): 'fopen' was declared deprecated
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#include<cstdint>
#include<cstddef>
#include<utility>
#include<vector>
#include<string>
#include<iostream>
#include<string>
#include<sstream>
#include<iomanip>
#include<numeric>
#include<cmath>
#include<functional>
#include"svector.h"

namespace minimidi {

namespace container {

typedef std::vector<uint8_t> Bytes;

// size of SmallBytes is totally 8 bytes on the stack (7 bytes + 1 byte for size)
typedef ankerl::svector<uint8_t, 7> SmallBytes;

// to_string func for SmallBytes
inline std::string to_string(const SmallBytes &data) {
    // show in hex
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << "{ ";
    for (auto &d: data)
        ss << std::setw(2) << static_cast<int>(d) << " ";
    ss << "}" << std::dec;
    return ss.str();
};

}

// Here inline is used to avoid obeying the one definition rule (ODR).
inline std::ostream &operator<<(std::ostream &out, const container::Bytes &data) {
    out << std::hex << std::setfill('0') << "{ ";
    for (auto &d: data) {
        out << "0x" << std::setw(2) << static_cast<int>(d) << " ";
    }
    out << "}" << std::dec << std::endl;

    return out;
};

namespace utils {

inline uint32_t read_variable_length(const uint8_t *&buffer) {
    uint32_t value = 0;

    for (auto i = 0; i < 4; ++i) {
        value = (value << 7) + (*buffer & 0x7f);
        if (!(*buffer & 0x80)) break;
        buffer++;
    }

    buffer++;
    return value;
};

inline uint64_t read_msb_bytes(const uint8_t *buffer, size_t length) {
    uint64_t res = 0;

    for (auto i = 0; i < length; ++i) {
        res <<= 8;
        res += (*(buffer + i));
    }

    return res;
};

inline void write_msb_bytes(uint8_t *buffer, size_t value, size_t length) {
    for (auto i = 1; i <= length; ++i) {
        *buffer = static_cast<uint8_t>((value >> ((length - i) * 8)) & 0xFF);
        ++buffer;
    }
};

inline uint8_t calc_variable_length(uint32_t num) {
    if(num < 0x80)
        return 1;
    else if(num < 0x4000)
        return 2;
    else if(num < 0x200000)
        return 3;
    else
        return 4;
};

inline void write_variable_length(uint8_t *&buffer, uint32_t num) {
    const uint8_t byteNum = calc_variable_length(num);

    for(auto i = 0; i < byteNum - 1; ++i) {
        *buffer = (num >> (7 * (byteNum - i - 1))) | 0x80;
        ++buffer;
    }
    *buffer = num & 0x7F;
    ++buffer;
};

inline container::SmallBytes make_variable_length(uint32_t num) {
    const uint8_t byteNum = calc_variable_length(num);

    container::SmallBytes result(byteNum);
    auto* cursor = const_cast<uint8_t*>(result.data());
    write_variable_length(cursor, num);

    return result;
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
    MIDI_MESSAGE_TYPE_MEMBER(QuarterFrame, 0xF1, 2)            \
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

constexpr int16_t MIN_PITCHBEND = -8192;
constexpr int16_t MAX_PITCHBEND = 8191;


enum class MessageType {
#define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) type,
    MIDI_MESSAGE_TYPE
#undef MIDI_MESSAGE_TYPE_MEMBER
};

inline std::string message_type_to_string(const MessageType &messageType) {
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
    MessageType type;
    size_t length;
} MessageAttr;

static constexpr MessageAttr MESSAGE_ATTRS[] = {
#define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) {status, MessageType::type, length},
    MIDI_MESSAGE_TYPE
#undef MIDI_MESSAGE_TYPE_MEMBER
};

inline const MessageAttr &message_attr(const MessageType &messageType) {
    return MESSAGE_ATTRS[static_cast<std::underlying_type_t<MessageType>>(messageType)];
};

inline constexpr std::array<MessageType, 256> _generate_message_type_table() {
    std::array<MessageType, 256> LUT{};
    for(auto &type : LUT) type = MessageType::Unknown;
    for(const auto &msg_attr : MESSAGE_ATTRS) {
        if(msg_attr.status < 0xF0)
            for(auto i = 0; i < 0x10; i++)
                LUT[msg_attr.status | i] = msg_attr.type;
        else
            LUT[msg_attr.status] = msg_attr.type;
    }

    return LUT;
};

constexpr auto MESSAGE_TYPE_TABLE = _generate_message_type_table();

inline MessageType status_to_message_type(uint8_t status) {
    return MESSAGE_TYPE_TABLE[static_cast<size_t>(status)];
};

enum class MetaType : uint8_t {
#define MIDI_META_TYPE_MEMBER(type, status) type = status,
    MIDI_META_TYPE
#undef MIDI_META_TYPE_MEMBER
};

inline std::string meta_type_to_string(const MetaType &metaType) {
    switch (metaType) {
#define MIDI_META_TYPE_MEMBER(type, status) \
            case (MetaType::type): return #type;
        MIDI_META_TYPE
#undef MIDI_META_TYPE_MEMBER
    };

    return "Unknown";
};

inline constexpr std::array<MetaType, 256> _generate_meta_type_table() {
    std::array<MetaType, 256> LUT{};
    for(auto i = 0; i < 256; i++) {
        switch(i) {
            #define MIDI_META_TYPE_MEMBER(type, status) \
                        case (status): LUT[i] = MetaType::type; break;
                    MIDI_META_TYPE
            #undef MIDI_META_TYPE_MEMBER
            default: LUT[i] = MetaType::Unknown;
        }
    }

    return LUT;
};

constexpr auto META_TYPE_TABLE = _generate_meta_type_table();

inline MetaType status_to_meta_type(uint8_t status) {
    return META_TYPE_TABLE[static_cast<size_t>(status)];
};

#undef MIDI_MESSAGE_TYPE
#undef MIDI_META_TYPE

typedef struct {
    uint8_t numerator;
    uint8_t denominator;
} TimeSignature;

const std::string KEYS_NAME[] = {"bC", "bG", "bD", "bA", "bE", "bB", "F", "C", "G",
                                 "D", "A", "E", "B", "#F", "#C", "bc", "bg", "bd",
                                 "ba", "be", "bb", "f", "c", "g", "d", "a", "e",
                                 "b", "#f", "#c"};

class KeySignature {
public:
    int8_t key;
    uint8_t tonality;

    KeySignature() = default;
    KeySignature(int8_t key, uint8_t tonality): key(key), tonality(tonality) {};

    [[nodiscard]] std::string to_string() const {
        return KEYS_NAME[this->key + 7 + this->tonality * 12];
    };

};

class Message {
    uint32_t time;
    uint8_t statusByte;
    container::SmallBytes data;

public:
    Message(const uint32_t time, const container::SmallBytes &data) {
        this->time = time;
        this->statusByte = data[0];
        this->data.assign(data.begin() + 1, data.end());
    };

    Message(const uint32_t time, container::SmallBytes &&data) {
        this->time = time;
        this->statusByte = data[0];
        this->data.assign(data.begin() + 1, data.end());
    };

    Message(const uint32_t time, const uint8_t statusByte, const container::SmallBytes &data) {
        this->time = time;
        this->statusByte = statusByte;
        this->data = data;
    };

    Message(const uint32_t time, const uint8_t statusByte, container::SmallBytes &&data) {
        this->time = time;
        this->statusByte = statusByte;
        this->data = std::move(data);
    };

    Message(const uint32_t time, const uint8_t statusByte, const uint8_t *begin, const size_t size):
        time(time), statusByte(statusByte), data(begin, size) {};

    Message(const uint32_t time, const uint8_t *begin, const size_t size):
        time(time), statusByte(*begin), data(begin + 1, size - 1) {};

    [[nodiscard]] uint32_t get_time() const { return time; };

    [[nodiscard]] uint8_t get_status_byte() const { return statusByte; };

    [[nodiscard]] const container::SmallBytes &get_data() const { return data; };

    [[nodiscard]] MessageType get_type() const { return status_to_message_type(statusByte); };

    [[nodiscard]] std::string get_type_string() const {
        return message_type_to_string(this->get_type());
    };

    [[nodiscard]] uint8_t get_channel() const { return statusByte & 0x0F; };

    [[nodiscard]] uint8_t get_pitch() const { return data[0]; };

    [[nodiscard]] uint8_t get_velocity() const { return data[1]; };

    [[nodiscard]] uint8_t get_control_number() const { return data[0]; };

    [[nodiscard]] uint8_t get_control_value() const { return data[1]; };

    [[nodiscard]] uint8_t get_program() const { return data[0]; };

    [[nodiscard]] MetaType get_meta_type() const {
        return status_to_meta_type(this->data[0]);
    };

    [[nodiscard]] std::string get_meta_type_string() const {
        return meta_type_to_string(this->get_meta_type());
    }

    [[nodiscard]] container::SmallBytes get_meta_value() const {
        // Clang-Tidy: Avoid repeating the return type from the declaration;
        // use a braced initializer list instead
        return {this->data.begin() + 2, this->data.end()};
    };

    [[nodiscard]] uint32_t get_tempo() const {
        return utils::read_msb_bytes(const_cast<uint8_t *>(this->data.data()) + 2, 3);
    };

    [[nodiscard]] message::TimeSignature get_time_signature() const {
        // Clang-Tidy: Avoid repeating the return type from the declaration;
        // use a braced initializer list instead
        return {data[2], static_cast<uint8_t>(1 << data[3])};
    };

    [[nodiscard]] message::KeySignature get_key_signature() const {
        // Clang-Tidy: Avoid repeating the return type from the declaration;
        // use a braced initializer list instead
        return {static_cast<int8_t>(data[2]), data[3]};
    }

    [[nodiscard]] int16_t get_pitch_bend() const {
        return static_cast<int16_t>(data[0] | (data[1] << 7) + MIN_PITCHBEND);
    };

    [[nodiscard]] uint16_t get_song_position_pointer() const {
        return static_cast<uint16_t>(data[0] | (data[1] << 7));
    };

    [[nodiscard]] uint8_t get_frame_type() const {
        return data[0] >> 4;
    };

    [[nodiscard]] uint8_t get_frame_value() const {
        return data[0] & 0x0F;
    };

    static Message NoteOn(uint32_t time, uint8_t channel, uint8_t pitch, uint8_t velocity) {
        return { time,
            static_cast<uint8_t>(message_attr(MessageType::NoteOn).status | channel),
            container::SmallBytes{pitch, velocity}};
    };

    static Message NoteOff(uint32_t time, uint8_t channel, uint8_t pitch, uint8_t velocity) {
        return { time,
            static_cast<uint8_t>(message_attr(MessageType::NoteOff).status | channel),
            container::SmallBytes{pitch, velocity}};
    };

    static Message ControlChange(uint32_t time, uint8_t channel, uint8_t controlNumber, uint8_t controlValue) {
        return { time,
            static_cast<uint8_t>(message_attr(MessageType::ControlChange).status | channel),
            container::SmallBytes{controlNumber, controlValue}};
    };

    static Message ProgramChange(uint32_t time, uint8_t channel, uint8_t program) {
        return { time,
            static_cast<uint8_t>(message_attr(MessageType::ProgramChange).status | channel),
            container::SmallBytes{program}};
    };

    static Message SysEx(uint32_t time, const container::SmallBytes &data) {
        const size_t lenBytesNum = utils::calc_variable_length(data.size());
        container::SmallBytes buffer(data.size() + lenBytesNum + 1);

        // Write length bytes
        auto *cursor = const_cast<uint8_t*>(buffer.data());
        utils::write_variable_length(cursor, data.size());

        // Write data
        std::copy(data.begin(), data.end(), cursor);

        // Write SysExEnd
        buffer[buffer.size() - 1] = message_attr(MessageType::SysExEnd).status; // 0xF7;

        return { time,
            message_attr(MessageType::SysExStart).status, // 0xF0
            std::move(buffer)};
    };

    static Message SongPositionPointer(uint32_t time, uint16_t position) {
        // the type of position is uint14_t
        return { time,
            message_attr(MessageType::SongPositionPointer).status,
            container::SmallBytes{
                static_cast<uint8_t>(position & 0x7F),
                static_cast<uint8_t>(position >> 7)
            }
        };
    };

    static Message PitchBend(uint32_t time, uint8_t channel, int16_t value ) {
        value -= MIN_PITCHBEND;
        return { time,
            static_cast<uint8_t>(message_attr(MessageType::PitchBend).status | channel),
            container::SmallBytes{
                static_cast<uint8_t>(value & 0x7F),
                static_cast<uint8_t>(value >> 7)
           }
        };
    };

    static Message QuarterFrame(uint32_t time, uint8_t type, uint8_t value) {
        return { time,
            static_cast<uint8_t>(message_attr(MessageType::QuarterFrame).status),
            container::SmallBytes{
                static_cast<uint8_t>((type << 4) | value )
            }
        };
    };

    static Message Meta(uint32_t time, MetaType metaType, const container::SmallBytes &metaValue) {
        const size_t lenBytesNum = utils::calc_variable_length(metaValue.size());
        container::SmallBytes buffer(metaValue.size() + lenBytesNum + 1);

        // Write meta type
        buffer[0] = static_cast<uint8_t>(metaType);

        // Write meta length
        uint8_t* cursor = const_cast<uint8_t*>(buffer.data()) + 1;
        utils::write_variable_length(cursor, metaValue.size());

        // Write meta value
        std::copy(metaValue.begin(), metaValue.end(), cursor);

        return {time,
            message_attr(MessageType::Meta).status, //0xFF;
            std::move(buffer)};
    };

    static Message Meta(uint32_t time, MetaType metaType, const std::string &metaValue) {
        const size_t lenBytesNum = utils::calc_variable_length(metaValue.size());
        container::SmallBytes buffer(metaValue.size() + lenBytesNum + 1);

        // Write meta type
        buffer[0] = static_cast<uint8_t>(metaType);

        // Write meta length
        uint8_t* cursor = const_cast<uint8_t*>(buffer.data()) + 1;
        utils::write_variable_length(cursor, metaValue.size());

        // Write meta value
        std::copy(metaValue.begin(), metaValue.end(), cursor);

        return {time,
            message_attr(MessageType::Meta).status, //0xFF;
            std::move(buffer)};
    };

    static Message Text(const uint32_t time, const std::string &text) {
        return Meta(time, MetaType::Text, text);
    };

    static Message TrackName(const uint32_t time, const std::string &name) {
        return Meta(time, MetaType::TrackName, name);
    };

    static Message InstrumentName(const uint32_t time, const std::string &name) {
        return Meta(time, MetaType::InstrumentName, name);
    };

    static Message Lyric(const uint32_t time, const std::string &lyric) {
        return Meta(time, MetaType::Lyric, lyric);
    };

    static Message Marker(const uint32_t time, const std::string &marker) {
        return Meta(time, MetaType::Marker, marker);
    };

    static Message CuePoint(const uint32_t time, const std::string &cuePoint) {
        return Meta(time, MetaType::CuePoint, cuePoint);
    };

    static Message MIDIChannelPrefix(const uint32_t time, const uint8_t channel) {
        return {time,
            message_attr(MessageType::Meta).status, //0xFF,
            container::SmallBytes{
                static_cast<uint8_t>(message::MetaType::MIDIChannelPrefix), //0x20,
                static_cast<uint8_t>(1),
                channel
            }
        };
    };

    static Message EndOfTrack(const uint32_t time) {
        return {time,
            message_attr(MessageType::Meta).status, //0xFF,
            container::SmallBytes{
                static_cast<uint8_t>(message::MetaType::EndOfTrack), //0x2F,
                static_cast<uint8_t>(0)
            }
        };
    };

    static Message SetTempo(const uint32_t time, const uint32_t tempo) {
        return {time,
            message_attr(MessageType::Meta).status, //0xFF,
            container::SmallBytes{
                static_cast<uint8_t>(message::MetaType::SetTempo), //0x51,
                static_cast<uint8_t>(3),
                static_cast<uint8_t>((tempo >> 16) & 0xFF),
                static_cast<uint8_t>((tempo >> 8) & 0xFF),
                static_cast<uint8_t>(tempo & 0xFF)
            }
        };
    };

    static Message SMPTEOffset(const uint32_t time, const uint8_t hour, const uint8_t minute,const uint8_t second, const uint8_t frame, const uint8_t subframe) {
        return {time,
            message_attr(MessageType::Meta).status, //0xFF,
            container::SmallBytes{
                static_cast<uint8_t>(message::MetaType::SMPTEOffset), //0x54,
                static_cast<uint8_t>(5),
                hour,
                minute,
                second,
                frame,
                subframe
            }
        };
    };

    static Message TimeSignature(const uint32_t time, const uint8_t numerator, const uint8_t denominator) {
        return {time,
            message_attr(MessageType::Meta).status, //0xFF,
            container::SmallBytes{
                static_cast<uint8_t>(message::MetaType::TimeSignature), //0x58,
                static_cast<uint8_t>(4),
                numerator,
                static_cast<uint8_t>(std::log2(denominator)),
                static_cast<uint8_t>(0x18),
                static_cast<uint8_t>(0x08)
            }
        };
    };

    static Message KeySignature(const uint32_t time, const int8_t key, const uint8_t tonality) {
        return {time,
            message_attr(MessageType::Meta).status, //0xFF,
            container::SmallBytes{
                static_cast<uint8_t>(message::MetaType::KeySignature), //0x59,
                static_cast<uint8_t>(2),
                static_cast<uint8_t>(key),
                tonality
            }
        };
    };
};

inline std::ostream &operator<<(std::ostream &out, const Message &message) {
    out << "time=" << message.get_time() << " | [";
    out << message.get_type_string() << "] ";

    switch (message.get_type()) {
        case (MessageType::NoteOn): {}  // the same as NoteOff
        case (MessageType::NoteOff): {
            out << "channel="   << static_cast<int>(message.get_channel())
                << " pitch="    << static_cast<int>(message.get_pitch())
                << " velocity=" << static_cast<int>(message.get_velocity());
            break;
        };
        case (MessageType::ProgramChange): {
            out << "channel="   << static_cast<int>(message.get_channel())
                << " program="  << static_cast<int>(message.get_program());
            break;
        };
        case (MessageType::ControlChange): {
            out << "channel="           << static_cast<int>(message.get_channel())
                << " control number="   << static_cast<int>(message.get_control_number())
                << " control value="    << static_cast<int>(message.get_control_value());
            break;
        };
        case (MessageType::Meta): {
            out << "(" << message.get_meta_type_string() << ") ";
            switch (message.get_meta_type()) {
                case (MetaType::TrackName): {
                    const auto &data = message.get_meta_value();
                    out << std::string(data.begin(), data.end());
                    break;
                };
                case (MetaType::InstrumentName): {
                    const auto &data = message.get_meta_value();
                    out << std::string(data.begin(), data.end());
                    break;
                };
                case (MetaType::TimeSignature): {
                    const TimeSignature timeSig = message.get_time_signature();
                    out << static_cast<int>(timeSig.numerator) << "/" << static_cast<int>(timeSig.denominator);
                    break;
                };
                case (MetaType::SetTempo): {
                    out << static_cast<int>(message.get_tempo());
                    break;
                };
                case (MetaType::KeySignature): {
                    out << message.get_key_signature().to_string();
                    break;
                }
                case (MetaType::EndOfTrack): {
                    break;
                }
                default: {
                    const auto &data = message.get_meta_value();
                    out << static_cast<int>(message.get_meta_type()) << " value=" << container::to_string(data);
                    break;
                }
            }
            break;
        };
        default: {
            out << "Status code: "  << static_cast<int>(message_attr(message.get_type()).status)
                << " length="       << message.get_data().size();
            break;
        };
    }

    return out;
};

typedef std::vector<Message> Messages;

inline Messages filter_message(const Messages& messages, const std::function<bool(const Message &)> &filter) {
    Messages new_messages;
    new_messages.reserve(messages.size());
    std::copy_if(messages.begin(),
                messages.end(),
                std::back_inserter(new_messages),
                filter);
    new_messages.shrink_to_fit();
    return new_messages;
};

}


namespace track {

const std::string MTRK("MTrk");

class Track {
public:
    message::Messages messages;
    Track() = default;

    // explicit Track(const container::ByteSpan data) {
    Track(const uint8_t *cursor, const size_t size) {
        messages.reserve(size / 3 + 100);
        const uint8_t *bufferEnd = cursor + size;

        uint32_t tickOffset = 0;
        uint8_t prevStatusCode = 0x00;
        size_t prevEventLen = 0;

        while (cursor < bufferEnd) {
            tickOffset += utils::read_variable_length(cursor);
            // Running status
            if (const uint8_t curStatusCode = *cursor; curStatusCode < 0x80) {
                if (!prevEventLen) {
                    throw std::ios_base::failure(
                        "MiniMidi: Unexpected running status! Get status code: "
                        + std::to_string(curStatusCode)
                        + " but previous event length is 0!"
                    );
                }
                if (cursor + prevEventLen - 1 > bufferEnd) {
                    throw std::ios_base::failure(
                        "MiniMidi: Unexpected EOF in running status! Cursor would be "
                        + std::to_string(cursor + prevEventLen - 1 - bufferEnd)
                        + " bytes beyond the end of buffer with previous event length "
                        + std::to_string(prevEventLen) + "!"
                    );
                }
                messages.emplace_back(tickOffset, prevStatusCode, cursor, prevEventLen - 1);
                cursor += prevEventLen - 1;
            }
            // Meta message
            else if (curStatusCode == 0xFF) {
                prevStatusCode = curStatusCode;
                const uint8_t *prevBuffer = cursor;

                // Skip status byte and meta type byte
                cursor += 2;
                prevEventLen = utils::read_variable_length(cursor) + (cursor - prevBuffer);

                if (prevBuffer + prevEventLen > bufferEnd) {
                    throw std::ios_base::failure(
                        "MiniMidi: Unexpected EOF in Meta Event! Cursor would be "
                        + std::to_string(cursor + prevEventLen - bufferEnd)
                        + " bytes beyond the end of buffer with previous event length "
                        + std::to_string(prevEventLen) + "!"
                    );
                }
                messages.emplace_back(tickOffset, prevBuffer, prevEventLen);

                if (messages.back().get_meta_type() == message::MetaType::EndOfTrack)
                    break;

                cursor = prevBuffer + prevEventLen;
            }
            // SysEx message
            else if (curStatusCode == 0xF0) {
                prevStatusCode = curStatusCode;
                const uint8_t *prevBuffer = cursor;

                // Skip status byte
                cursor += 1;
                prevEventLen = utils::read_variable_length(cursor) + (cursor - prevBuffer);

                if (prevBuffer + prevEventLen > bufferEnd) {
                    throw std::ios_base::failure(
                        "MiniMidi: Unexpected EOF in SysEx Event! Cursor would be "
                        + std::to_string(cursor + prevEventLen - bufferEnd)
                        + " bytes beyond the end of buffer with previous event length "
                        + std::to_string(prevEventLen) + "!"
                    );
                }
                messages.emplace_back(tickOffset, prevBuffer, prevEventLen);
                cursor = prevBuffer + prevEventLen;
            }
            // Channel message or system common message
            else {
                prevStatusCode = curStatusCode;
                prevEventLen = message::message_attr(message::status_to_message_type(curStatusCode)).length;

                if (cursor + prevEventLen > bufferEnd) {
                    throw std::ios_base::failure(
                        "MiniMidi: Unexpected EOF in MIDI Event! Cursor would be "
                        + std::to_string(cursor + prevEventLen - bufferEnd)
                        + " bytes beyond the end of buffer with previous event length "
                        + std::to_string(prevEventLen) + "!"
                    );
                }
                messages.emplace_back(tickOffset, cursor, prevEventLen);
                cursor += prevEventLen;
            }

            if (cursor > bufferEnd) {
                throw std::ios_base::failure(
                    "MiniMidi: Unexpected EOF in track! Cursor is "
                    + std::to_string(cursor - bufferEnd)
                    + " bytes beyond the end of buffer!"
                );
            }
        }
    };

    explicit Track(message::Messages &&message) {
        this->messages = std::vector(std::move(message));
    };

    message::Message &message(const uint32_t index) {
        return this->messages[index];
    };

    [[nodiscard]] const message::Message &message(const uint32_t index) const {
        return this->messages[index];
    };

    [[nodiscard]] size_t message_num() const {
        return this->messages.size();
    };

    [[nodiscard]] container::Bytes to_bytes() const {
        // Prepare EOT
        static const message::Message _eot = message::Message::EndOfTrack(0);

        // (time, index)
        typedef std::pair<uint32_t, size_t> SortHelper;
        std::vector<SortHelper> msgHeaders;
        msgHeaders.reserve(this->messages.size());
        size_t dataLen = 0;

        for (int i = 0; i < this->messages.size(); ++i) {
            if(this->messages[i].get_type() != message::MessageType::Meta ||
                this->messages[i].get_meta_type() != message::MetaType::EndOfTrack) {
                msgHeaders.emplace_back(this->messages[i].get_time(), i);
                dataLen += this->messages[i].get_data().size();
            }
        }

        std::sort(msgHeaders.begin(),
            msgHeaders.end(),
            std::less<SortHelper>());

        container::Bytes trackBytes(dataLen + 5 * msgHeaders.size() + 8);

        uint8_t* cursor = trackBytes.data();
        uint32_t prevTime = 0;
        uint8_t prevStatus = 0x00;

        // Write track chunk header
        std::copy(MTRK.begin(), MTRK.end(), cursor);
        cursor += 8;
        for(const auto & [tick, idx] : msgHeaders) {
            const message::Message &thisMsg = this->messages[idx];
            const uint32_t curTime = thisMsg.get_time();
            const uint8_t curStatus = thisMsg.get_status_byte();

            utils::write_variable_length(cursor, curTime - prevTime);
            prevTime = curTime;

            // Not running status, write status byte
            if(curStatus == 0xFF ||
                curStatus == 0xF0 ||
                curStatus == 0xF7 ||
                curStatus != prevStatus) {
                *cursor = curStatus;
                ++cursor;
            }
            // Write data bytes
            std::copy(thisMsg.get_data().begin(), thisMsg.get_data().end(), cursor);
            cursor += thisMsg.get_data().size();

            prevStatus = curStatus;
        }
        // Write EOT
        utils::write_variable_length(cursor, 1);
        *cursor = _eot.get_status_byte();
        ++cursor;
        std::copy(_eot.get_data().begin(), _eot.get_data().end(), cursor);
        cursor += _eot.get_data().size();

        // Write track chunk length
        utils::write_msb_bytes(trackBytes.data() + 4, cursor - trackBytes.data() - 8, 4);

        trackBytes.resize(cursor - trackBytes.data());

        return trackBytes;
    };

};

inline std::ostream &operator<<(std::ostream &out, const Track &track) {
    for (int j = 0; j < track.message_num(); ++j) {
        out << track.message(j) << std::endl;
    }

    return out;
};

typedef std::vector<Track> Tracks;

}


namespace file {

const std::string MTHD("MThd");

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

    return formats[static_cast<int>(format)];
};

inline MidiFormat read_midiformat(const uint16_t data) {
    switch (data) {
#define MIDI_FORMAT_MEMBER(type, status) case status: return MidiFormat::type;
        MIDI_FORMAT
#undef MIDI_FORMAT_MEMBER
        default: throw std::ios_base::failure(
            "MiniMidi: Invaild midi format ("
            + std::to_string(data) + ")!"
            + "1 for single track, 2 for multi track, 3 for multi song."
        );
    }
};

class MidiFile {
public:
    MidiFormat format;
    uint16_t divisionType: 1;
    union {
        struct {
            uint16_t ticksPerQuarter: 15;
        };
        struct {
            uint16_t negativeSmpte: 7;
            uint16_t ticksPerFrame: 8;
        };
    };
    track::Tracks tracks;

    // MidiFile() = default;

    explicit MidiFile(const uint8_t* const data, const size_t size) {
        if (size < 4) {
            throw std::ios_base::failure("MiniMidi: Invaild midi file! File size is less than 4!");
        }
        const uint8_t* cursor = data;
        const uint8_t* bufferEnd = cursor + size;

        if (std::string(reinterpret_cast<const char*>(cursor), 4) != MTHD) {
            throw std::ios_base::failure("MiniMidi: Invaild midi file! File header is not MThd!");
        }
        if (const auto chunkLen = utils::read_msb_bytes(cursor + 4, 4); chunkLen != 6) {
            throw std::ios_base::failure(
                "MiniMidi: Invaild midi file! The first chunk length is not 6, but "
                + std::to_string(chunkLen) + "!"
            );
        }
        this->format = read_midiformat(utils::read_msb_bytes(cursor + 8, 2));
        const uint16_t trackNum = utils::read_msb_bytes(cursor + 10, 2);
        this->divisionType = ((*(cursor + 12)) & 0x80) >> 7;
        this->ticksPerQuarter = (((*(cursor + 12)) & 0x7F) << 8) + (*(cursor + 13));

        cursor += 14;
        tracks.reserve(trackNum);
        for (int i = 0; i < trackNum; ++i) {
            // Skip unknown chunk
            while(std::string(reinterpret_cast<const char*>(cursor), 4) != track::MTRK) {
                const size_t chunkLen = utils::read_msb_bytes(cursor + 4, 4);

                if(cursor + chunkLen + 8 > bufferEnd) {
                    throw std::ios_base::failure(
                        "MiniMidi: Unexpected EOF in file! Cursor is "
                        + std::to_string(cursor + chunkLen + 8 - bufferEnd)
                        + " bytes beyond the end of buffer with chunk length "
                        + std::to_string(chunkLen) + "!"
                    );
                }
                cursor += (8 + chunkLen);
            }

            const size_t chunkLen = utils::read_msb_bytes(cursor + 4, 4);

            if (cursor + chunkLen + 8 > bufferEnd) {
                throw std::ios_base::failure(
                    "MiniMidi: Unexpected EOF in file! Cursor is "
                    + std::to_string(cursor + chunkLen + 8 - bufferEnd)
                    + " bytes beyond the end of buffer with chunk length "
                    + std::to_string(chunkLen) + "!"
                );
            }

            this->tracks.emplace_back(cursor + 8, chunkLen);
            cursor += (8 + chunkLen);
        }
    };

    explicit MidiFile(const container::Bytes &data) : MidiFile(data.data(), data.size()) {};

    explicit MidiFile(MidiFormat format=MidiFormat::MultiTrack,
                    uint8_t divisionType=0,
                    uint16_t ticksPerQuarter=960) {
        this->format = format;
        this->divisionType = divisionType;
        this->ticksPerQuarter = ticksPerQuarter;
    };

    explicit MidiFile(track::Tracks &&tracks,
                    MidiFormat format=MidiFormat::MultiTrack,
                    uint8_t divisionType=0,
                    uint16_t ticksPerQuarter=960) {
        this->tracks = tracks;
        this->format = format;
        this->divisionType = divisionType;
        this->ticksPerQuarter = ticksPerQuarter;
    };

    explicit MidiFile(const track::Tracks& tracks,
                    MidiFormat format=MidiFormat::MultiTrack,
                    uint8_t divisionType=0,
                    uint16_t ticksPerQuarter=960) {
        this->tracks = track::Tracks(tracks);
        this->format = format;
        this->divisionType = divisionType;
        this->ticksPerQuarter = ticksPerQuarter;
    };

    static MidiFile from_file(const std::string &filepath) {
        FILE *filePtr = fopen(filepath.c_str(), "rb");

        if (!filePtr) {
            throw std::ios_base::failure("MiniMidi: Reading file failed (fopen)!");
        }
        fseek(filePtr, 0, SEEK_END);
        const size_t fileLen = ftell(filePtr);

        container::Bytes data(fileLen);
        fseek(filePtr, 0, SEEK_SET);
        fread(data.data(), 1, fileLen, filePtr);
        fclose(filePtr);

        return MidiFile(data.data(), fileLen);
    };

    container::Bytes to_bytes() {
        std::vector<container::Bytes> trackBytes(tracks.size());
        size_t trackByteNum = 0;
        for (auto i = 0; i < tracks.size(); ++i) {
            trackBytes[i] = tracks[i].to_bytes();
            trackByteNum += trackBytes[i].size();
        }

        container::Bytes midiBytes(trackByteNum + 14);

        // Write head
        std::copy(MTHD.begin(), MTHD.end(), midiBytes.begin());
        midiBytes[7] = 0x06;  // Length of head
        midiBytes[9] = static_cast<uint8_t>(format);
        utils::write_msb_bytes(midiBytes.data() + 10, tracks.size(), 2);
        utils::write_msb_bytes(midiBytes.data() + 12, (divisionType << 15) | ticksPerQuarter, 2);

        // Write track
        size_t cursor = 14;
        for (const auto& thisTrackBytes: trackBytes) {
            std::copy(thisTrackBytes.begin(), thisTrackBytes.end(), midiBytes.begin() + static_cast<long long>(cursor));
            cursor += thisTrackBytes.size();
        }

        return midiBytes;
    };

    void write_file(const std::string &filepath) {
        FILE *filePtr = fopen(filepath.c_str(), "wb");

        if (!filePtr) {
            throw std::ios_base::failure("MiniMidi: Create file failed (fopen)!");
        }
        const container::Bytes midiBytes = this->to_bytes();
        fwrite(midiBytes.data(), 1, midiBytes.size(), filePtr);
        fclose(filePtr);
    };

    [[nodiscard]] MidiFormat get_format() const {
        return this->format;
    };

    [[nodiscard]] std::string get_format_string() const {
        return format_to_string(this->get_format());
    }

    [[nodiscard]] uint16_t get_division_type() const {
        return this->divisionType;
    };

    [[nodiscard]] uint16_t get_tick_per_quarter() const {
        if (!this->get_division_type()) return this->ticksPerQuarter;
        else {
            std::cerr << "Division type 1 have no tpq." << std::endl;
            return -1;
        };
    };

    [[nodiscard]] uint16_t get_frame_per_second() const {
        return (~(this->negativeSmpte - 1)) & 0x3F;
    };

    [[nodiscard]] uint16_t get_tick_per_second() const {
        if (this->get_division_type()) return this->ticksPerFrame * this->get_frame_per_second();
        else {
            std::cerr << "Division type 0 have no tps." << std::endl;
            return -1;
        };
    };

    track::Track &track(const uint32_t index) {
        return this->tracks[index];
    };

    [[nodiscard]] const track::Track &track(const uint32_t index) const {
        return this->tracks[index];
    };

    [[nodiscard]] size_t track_num() const {
        return this->tracks.size();
    };
};

#undef MIDI_FORMAT

inline std::ostream &operator<<(std::ostream &out, const MidiFile &file) {
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


#endif //MINIMIDI_HPP