#ifndef MINIMIDI_HPP
#define MINIMIDI_HPP

// used for ignoring warning C4996 (MSCV): 'fopen' was declared deprecated
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#include <string>
#include <cstdint>
#include <cstddef>
#include <utility>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <cmath>
#include "svector.h"

namespace minimidi {


namespace container {
typedef std::vector<uint8_t> Bytes;

// size of SmallBytes is totally 8 bytes on the stack (7 bytes + 1 byte for size)
typedef ankerl::svector<uint8_t, 7> SmallBytes;

// Concepts

// concept for begin end constructor
template<typename T>
concept BasicArr = requires(T a) {
    { a.begin() };
    { a.end() };
    { T(a.begin(), a.end()) };
    { a.size() } -> std::integral;
    { a[0] } -> std::convertible_to<uint8_t>;
};

// concept for move constructor
template<typename T>
concept MoveConstructible = std::is_move_constructible_v<T> && std::movable<T>;

// concept for a random iterator which can dereference into a uint8_t
template<typename T>
concept RandomIterator = requires(T a) {
    { *a } -> std::convertible_to<uint8_t>;
    { a++ };
    { a + 10 };
};

// concept for construct from initializer list
template<typename T>
concept InitializerList = requires(T a) {
    { T{0x00, 0x01, 0x02} };
};

template<typename T>
concept BeginSizeConstructor = requires(T a) {
    { a.begin() };
    { a.size() };
    { T(a.begin(), a.size()) };
};

}   // namespace container


namespace utils {
template<container::RandomIterator Iter>
uint32_t read_variable_length(Iter& buffer) {
    uint32_t value = 0;

    for (auto i = 0; i < 4; ++i) {
        value = (value << 7) + (*buffer & 0x7f);
        if (!(*buffer & 0x80)) break;
        ++buffer;
    }

    ++buffer;
    return value;
};

inline uint64_t read_msb_bytes(const uint8_t* buffer, const size_t length) {
    uint64_t res = 0;

    for (auto i = 0; i < length; ++i) {
        res <<= 8;
        res += (*(buffer + i));
    }

    return res;
};

inline void write_msb_bytes(uint8_t* buffer, const size_t value, const size_t length) {
    for (auto i = 1; i <= length; ++i) {
        *buffer = static_cast<uint8_t>((value >> ((length - i) * 8)) & 0xFF);
        ++buffer;
    }
};

inline uint8_t calc_variable_length(uint32_t num) {
    if (num < 0x80)
        return 1;
    else if (num < 0x4000)
        return 2;
    else if (num < 0x200000)
        return 3;
    else
        return 4;
};

inline void write_variable_length(uint8_t*& buffer, const uint32_t num) {
    const uint8_t byteNum = calc_variable_length(num);

    for (auto i = 0; i < byteNum - 1; ++i) {
        *buffer = (num >> (7 * (byteNum - i - 1))) | 0x80;
        ++buffer;
    }
    *buffer = num & 0x7F;
    ++buffer;
};

inline void write_variable_length(container::Bytes& bytes, const uint32_t num) {
    const uint8_t byteNum = calc_variable_length(num);

    for (auto i = 0; i < byteNum - 1; ++i) {
        bytes.emplace_back((num >> (7 * (byteNum - i - 1))) | 0x80);
    }
    bytes.emplace_back(num & 0x7F);
};

template<typename Iter>
void write_iter(container::Bytes& bytes, Iter begin, Iter end) {
    for (; begin < end; ++begin) { bytes.emplace_back(*begin); }
}

inline container::SmallBytes make_variable_length(uint32_t num) {
    const uint8_t byteNum = calc_variable_length(num);

    container::SmallBytes result(byteNum);
    auto*                 cursor = const_cast<uint8_t*>(result.data());
    write_variable_length(cursor, num);

    return result;
};
}   // namespace utils

// (name, status, length)
#define MIDI_MESSAGE_TYPE                                   \
    MIDI_MESSAGE_TYPE_MEMBER(Unknown, 0x00, 65535)          \
    MIDI_MESSAGE_TYPE_MEMBER(NoteOff, 0x80, 3)              \
    MIDI_MESSAGE_TYPE_MEMBER(NoteOn, 0x90, 3)               \
    MIDI_MESSAGE_TYPE_MEMBER(PolyphonicAfterTouch, 0xA0, 3) \
    MIDI_MESSAGE_TYPE_MEMBER(ControlChange, 0xB0, 3)        \
    MIDI_MESSAGE_TYPE_MEMBER(ProgramChange, 0xC0, 2)        \
    MIDI_MESSAGE_TYPE_MEMBER(ChannelAfterTouch, 0xD0, 2)    \
    MIDI_MESSAGE_TYPE_MEMBER(PitchBend, 0xE0, 3)            \
    MIDI_MESSAGE_TYPE_MEMBER(SysExStart, 0xF0, 65535)       \
    MIDI_MESSAGE_TYPE_MEMBER(QuarterFrame, 0xF1, 2)         \
    MIDI_MESSAGE_TYPE_MEMBER(SongPositionPointer, 0xF2, 3)  \
    MIDI_MESSAGE_TYPE_MEMBER(SongSelect, 0xF3, 2)           \
    MIDI_MESSAGE_TYPE_MEMBER(TuneRequest, 0xF6, 1)          \
    MIDI_MESSAGE_TYPE_MEMBER(SysExEnd, 0xF7, 1)             \
    MIDI_MESSAGE_TYPE_MEMBER(TimingClock, 0xF8, 1)          \
    MIDI_MESSAGE_TYPE_MEMBER(StartSequence, 0xFA, 1)        \
    MIDI_MESSAGE_TYPE_MEMBER(ContinueSequence, 0xFB, 1)     \
    MIDI_MESSAGE_TYPE_MEMBER(StopSequence, 0xFC, 1)         \
    MIDI_MESSAGE_TYPE_MEMBER(ActiveSensing, 0xFE, 1)        \
    MIDI_MESSAGE_TYPE_MEMBER(Meta, 0xFF, 65535)
// (name, status)
#define MIDI_META_TYPE                                 \
    MIDI_META_TYPE_MEMBER(SequenceNumber, 0x00)        \
    MIDI_META_TYPE_MEMBER(Text, 0x01)                  \
    MIDI_META_TYPE_MEMBER(CopyrightNote, 0x02)         \
    MIDI_META_TYPE_MEMBER(TrackName, 0x03)             \
    MIDI_META_TYPE_MEMBER(InstrumentName, 0x04)        \
    MIDI_META_TYPE_MEMBER(Lyric, 0x05)                 \
    MIDI_META_TYPE_MEMBER(Marker, 0x06)                \
    MIDI_META_TYPE_MEMBER(CuePoint, 0x07)              \
    MIDI_META_TYPE_MEMBER(MIDIChannelPrefix, 0x20)     \
    MIDI_META_TYPE_MEMBER(EndOfTrack, 0x2F)            \
    MIDI_META_TYPE_MEMBER(SetTempo, 0x51)              \
    MIDI_META_TYPE_MEMBER(SMPTEOffset, 0x54)           \
    MIDI_META_TYPE_MEMBER(TimeSignature, 0x58)         \
    MIDI_META_TYPE_MEMBER(KeySignature, 0x59)          \
    MIDI_META_TYPE_MEMBER(SequencerSpecificMeta, 0x7F) \
    MIDI_META_TYPE_MEMBER(Unknown, 0xFF)
constexpr int16_t MIN_PITCH_BEND = -8192;
constexpr int16_t MAX_PITCH_BEND = 8191;


enum class MessageType : uint8_t {
#define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) type,
    MIDI_MESSAGE_TYPE
#undef MIDI_MESSAGE_TYPE_MEMBER
};

enum class MetaType : uint8_t {
#define MIDI_META_TYPE_MEMBER(type, status) type = status,
    MIDI_META_TYPE
#undef MIDI_META_TYPE_MEMBER
};


// convert MessageType to string for printing
inline std::string to_string(const MessageType& messageType) {
    switch (messageType) {
#define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) \
    case MessageType::type: return #type;
        MIDI_MESSAGE_TYPE
#undef MIDI_MESSAGE_TYPE_MEMBER
    };
    return "Unknown";
};

// convert MetaType to string for printing
inline std::string to_string(const MetaType& metaType) {
    switch (metaType) {
#define MIDI_META_TYPE_MEMBER(type, status) \
    case (MetaType::type): return #type;
        MIDI_META_TYPE
#undef MIDI_META_TYPE_MEMBER
    };
    return "Unknown";
};

// Compile-time built lookup tables for message type and meta type
namespace lut {
// Define 4 lut arrays for message type, status, length and meta status
static constexpr std::array<uint8_t, 20> MESSAGE_STATUS = {
#define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) status,
    MIDI_MESSAGE_TYPE
#undef MIDI_MESSAGE_TYPE_MEMBER
};

static constexpr std::array<MessageType, 20> MESSAGE_TYPE = {
#define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) MessageType::type,
    MIDI_MESSAGE_TYPE
#undef MIDI_MESSAGE_TYPE_MEMBER
};

static constexpr std::array<uint8_t, 20> MESSAGE_LENGTH = {
#define MIDI_MESSAGE_TYPE_MEMBER(type, status, length) static_cast<uint8_t>(length),
    MIDI_MESSAGE_TYPE
#undef MIDI_MESSAGE_TYPE_MEMBER
};

static constexpr uint8_t META_STATUS[] = {
#define MIDI_META_TYPE_MEMBER(type, status) status,
    MIDI_META_TYPE
#undef MIDI_META_TYPE_MEMBER
};

constexpr std::array<MessageType, 256> _generate_message_type_table() {
    std::array<MessageType, 256> LUT{};
    for (auto& type : LUT) type = MessageType::Unknown;
    for (auto j = 0; j < MESSAGE_STATUS.size(); j++) {
        const auto status = MESSAGE_STATUS[j];
        const auto type   = MESSAGE_TYPE[j];
        if (status < 0xF0)
            for (auto i = 0; i < 0x10; i++) LUT[status | i] = type;
        else
            LUT[status] = type;
    }
    return LUT;
};

constexpr std::array<MetaType, 256> _generate_meta_type_table() {
    std::array<MetaType, 256> LUT{};
    for (auto i = 0; i < 256; i++) {
        switch (i) {
#define MIDI_META_TYPE_MEMBER(type, status) \
    case (status): LUT[i] = MetaType::type; break;
            MIDI_META_TYPE
#undef MIDI_META_TYPE_MEMBER
        default: LUT[i] = MetaType::Unknown;
        }
    }
    return LUT;
};

constexpr auto MESSAGE_TYPE_TABLE = _generate_message_type_table();
constexpr auto META_TYPE_TABLE    = _generate_meta_type_table();

constexpr MessageType to_msg_type(const uint8_t status) {
    return MESSAGE_TYPE_TABLE[static_cast<size_t>(status)];
};

constexpr uint8_t to_msg_status(const MessageType messageType) {
    return MESSAGE_STATUS[static_cast<size_t>(messageType)];
}

constexpr uint8_t get_msg_length(const MessageType messageType) {
    return MESSAGE_LENGTH[static_cast<size_t>(messageType)];
}

constexpr MetaType to_meta_type(const uint8_t status) {
    return META_TYPE_TABLE[static_cast<size_t>(status)];
};

constexpr uint8_t to_meta_status(const MetaType metaType) {
    // return META_STATUS[static_cast<size_t>(metaType)];
    return static_cast<uint8_t>(metaType);
}

constexpr static uint8_t to_status_byte(const uint8_t status, const uint8_t channel) {
    return status | channel;
};

constexpr static uint8_t to_status_byte(const MessageType type, const uint8_t channel) {
    return to_msg_status(type) | channel;
}

#undef MIDI_MESSAGE_TYPE
#undef MIDI_META_TYPE

}   // namespace lut


template<typename T = container::SmallBytes>
    requires container::BasicArr<T>
class Message {
protected:
    T m_data;

public:
    uint32_t time{};
    uint8_t  statusByte{};

    Message() = default;

    // copy constructor
    Message(const uint32_t time, const uint8_t statusByte, const T& data) :
        m_data(data.begin(), data.end()), time(time), statusByte(statusByte){};

    // move constructor
    Message(const uint32_t time, const uint8_t statusByte, T&& data)
        requires container::MoveConstructible<T>
        : m_data(std::move(data)), time(time), statusByte(statusByte){};

    // constructor from begin and end
    template<container::RandomIterator Iter>
    Message(const uint32_t time, const uint8_t statusByte, Iter begin, Iter end) :
        m_data(begin, end), time(time), statusByte(statusByte){};

    // constructor from begin and size
    template<container::RandomIterator Iter>
    Message(const uint32_t time, const uint8_t statusByte, Iter begin, size_t size)
        requires container::BeginSizeConstructor<T>
        : m_data(begin, size), time(time), statusByte(statusByte){};

    // constructor from begin and size for container that does not support BeginSizeConstructor
    // like std::vector
    template<container::RandomIterator Iter>
    Message(const uint32_t time, const uint8_t statusByte, Iter begin, size_t size)
        requires(!container::BeginSizeConstructor<T>)
        : m_data(begin, begin + size), time(time), statusByte(statusByte){};

    // constructor from initializer list
    Message(const uint32_t time, const uint8_t statusByte, std::initializer_list<uint8_t> list) :
        m_data(list), time(time), statusByte(statusByte){};

    // copy constructor from another message with possibly different data type
    template<typename U>
    explicit Message(const Message<U>& other) :
        m_data(other.data().begin(), other.data().end()), time(other.time),
        statusByte(other.statusByte){};

    Message(const Message& other) :
        m_data(other.m_data.begin(), other.m_data.end()), time(other.time),
        statusByte(other.statusByte){};

    // move constructor from another message with same data type
    Message(Message&& other) noexcept
        requires container::MoveConstructible<T>
        : m_data(std::move(other.m_data)), time(other.time), statusByte(other.statusByte){};

    // Zero Cost Down casting to derived class
    template<template<typename> typename U>
        requires std::is_base_of_v<Message<T>, U<T>>
    [[nodiscard]] const U<T>& cast() const {
        return reinterpret_cast<const U<T>&>(*this);
    }

    template<template<typename> typename U>
        requires std::is_base_of_v<Message<T>, U<T>>
    [[nodiscard]] U<T>& cast() {
        return reinterpret_cast<U<T>&>(*this);
    }

    [[nodiscard]] const T& data() const { return m_data; };

    [[nodiscard]] MessageType type() const { return lut::to_msg_type(statusByte); };

    [[nodiscard]] uint8_t channel() const { return statusByte & 0x0F; };
};

template<typename T = container::SmallBytes>
class NoteOn : public Message<T> {
public:
    static constexpr auto type   = MessageType::NoteOn;
    static constexpr auto status = lut::to_msg_status(type);

    NoteOn() = default;
    NoteOn(const uint32_t time, const uint8_t channel, const uint8_t pitch, const uint8_t velocity)
        requires container::InitializerList<T>
        : Message<T>(time, lut::to_status_byte(type, channel), {pitch, velocity}){};

    [[nodiscard]] uint8_t pitch() const { return this->m_data[0]; };
    [[nodiscard]] uint8_t velocity() const { return this->m_data[1]; };
};

template<typename T = container::SmallBytes>
class NoteOff : public Message<T> {
public:
    static constexpr auto type   = MessageType::NoteOff;
    static constexpr auto status = lut::to_msg_status(type);

    NoteOff() = default;
    NoteOff(const uint32_t time, const uint8_t channel, const uint8_t pitch, const uint8_t velocity)
        requires container::InitializerList<T>
        : Message<T>(time, lut::to_status_byte(type, channel), {pitch, velocity}){};

    [[nodiscard]] uint8_t pitch() const { return this->m_data[0]; };
    [[nodiscard]] uint8_t velocity() const { return this->m_data[1]; };
};

template<typename T = container::SmallBytes>
class ControlChange : public Message<T> {
public:
    static constexpr auto type   = MessageType::ControlChange;
    static constexpr auto status = lut::to_msg_status(type);

    ControlChange() = default;
    ControlChange(
        const uint32_t time,
        const uint8_t  channel,
        const uint8_t  controlNumber,
        const uint8_t  controlValue
    )
        requires container::InitializerList<T>
        : Message<T>(time, lut::to_status_byte(type, channel), {controlNumber, controlValue}){};

    [[nodiscard]] uint8_t control_number() const { return this->m_data[0]; };
    [[nodiscard]] uint8_t control_value() const { return this->m_data[1]; };
};

template<typename T = container::SmallBytes>
class ProgramChange : public Message<T> {
public:
    static constexpr auto type   = MessageType::ProgramChange;
    static constexpr auto status = lut::to_msg_status(type);

    ProgramChange() = default;
    ProgramChange(const uint32_t time, const uint8_t channel, const uint8_t program) :
        Message<T>(time, lut::to_status_byte(type, channel), {program}){};

    [[nodiscard]] uint8_t program() const { return this->m_data[0]; };
};

template<typename T = container::SmallBytes>
class SysEx : public Message<T> {
public:
    static constexpr auto type   = MessageType::SysExStart;
    static constexpr auto status = lut::to_msg_status(type);

    SysEx() = default;
    SysEx(const uint32_t time, const T& data) : Message<T>() {
        this->time = time;

        const auto variable_length = utils::calc_variable_length(data.size());
        this->m_data.resize(1 + variable_length + data.size());

        // Write variable length
        auto* cursor = &this->m_data[0];
        utils::write_variable_length(cursor, data.size());

        // Write data
        std::uninitialized_copy(data.begin(), data.end(), cursor);
        // Write SysExEnd
        this->m_data[this->m_data.size() - 1] = status;
    }
};

template<typename T = container::SmallBytes>
class QuarterFrame : public Message<T> {
public:
    static constexpr auto type   = MessageType::QuarterFrame;
    static constexpr auto status = lut::to_msg_status(type);

    QuarterFrame() = default;
    QuarterFrame(const uint32_t time, const uint8_t type, const uint8_t value)
        requires container::InitializerList<T>
        : Message<T>(time, status, {static_cast<uint8_t>((type << 4) | value)}){};

    [[nodiscard]] uint8_t frame_type() const { return this->m_data[0] >> 4; };
    [[nodiscard]] uint8_t frame_value() const { return this->m_data[0] & 0x0F; };
};

template<typename T = container::SmallBytes>
class SongPositionPointer : public Message<T> {
public:
    static constexpr auto type   = MessageType::SongPositionPointer;
    static constexpr auto status = lut::to_msg_status(type);

    SongPositionPointer() = default;
    // clang-format off
    SongPositionPointer(const uint32_t time, const uint16_t position)
        requires container::InitializerList<T>: Message<T>(
            time, status,
            {static_cast<uint8_t>(position & 0x7F), static_cast<uint8_t>(position >> 7)}
        ) {};
    // clang-format on

    [[nodiscard]] uint16_t song_position_pointer() const {
        return static_cast<uint16_t>(this->m_data[0] | (this->m_data[1] << 7));
    };
};

template<typename T = container::SmallBytes>
class PitchBend : public Message<T> {
    static constexpr auto type   = MessageType::PitchBend;
    static constexpr auto status = lut::to_msg_status(type);

    PitchBend() = default;
    // clang-format off
    PitchBend(const uint32_t time, const uint8_t channel, const int16_t value)
        requires container::InitializerList<T>: Message<T>(
            time, lut::to_status_byte(status, channel), {
                static_cast<uint8_t>((value - MIN_PITCH_BEND) & 0x7F),
                static_cast<uint8_t>((value - MIN_PITCH_BEND) >> 7)
            }
        ) {};
    // clang-format on

    [[nodiscard]] int16_t pitch_bend() const {
        return (static_cast<int>(this->m_data[0]) | static_cast<int>(this->m_data[1] << 7))
               + MIN_PITCH_BEND;
    };
};

template<typename T = container::SmallBytes>
struct Meta : Message<T> {
    static constexpr auto type   = MessageType::Meta;
    static constexpr auto status = lut::to_msg_status(type);

    Meta() = default;

    explicit Meta(const uint32_t time) : Message<T>{} {
        this->time       = time;
        this->statusByte = status;
    };

    template<typename U>
    Meta(const uint32_t time, const MetaType metaType, const U& metaValue) : Meta(time) {
        // malloc
        const auto variable_length = utils::calc_variable_length(metaValue.size());
        this->m_data.resize(1 + variable_length + metaValue.size());

        // Write meta type
        this->m_data[0] = static_cast<uint8_t>(metaType);
        // Write variable length
        auto* cursor = &this->m_data[1];
        utils::write_variable_length(cursor, metaValue.size());
        // Write meta value
        std::uninitialized_copy(metaValue.begin(), metaValue.end(), cursor);
    };


    [[nodiscard]] MetaType meta_type() const { return lut::to_meta_type(this->m_data[0]); };
    [[nodiscard]] T        meta_value() const {
        auto       cursor          = this->m_data.begin() + 1;
        const auto variable_length = utils::read_variable_length(cursor);
        if (cursor + variable_length > this->m_data.end()) {
            throw std::runtime_error("MiniMidi: Meta value is out of bound");
        }
        return {cursor, cursor + variable_length};
    };
};

template<typename T = container::SmallBytes>
struct SetTempo : Meta<T> {
    static constexpr auto meta_type = MetaType::SetTempo;

    SetTempo() = default;

    SetTempo(const uint32_t time, const uint32_t tempo) : Meta<T>(time) {
        this->m_data
            = {static_cast<uint8_t>(meta_type),
               static_cast<uint8_t>(3),
               static_cast<uint8_t>((tempo >> 16) & 0xFF),
               static_cast<uint8_t>((tempo >> 8) & 0xFF),
               static_cast<uint8_t>(tempo & 0xFF)};
    };

    [[nodiscard]] uint32_t tempo() const { return utils::read_msb_bytes(&this->m_data[2], 3); };
};

template<typename T = container::SmallBytes>
struct TimeSignature : Meta<T> {
    static constexpr auto meta_type = MetaType::TimeSignature;

    TimeSignature() = default;

    TimeSignature(const uint32_t time, const uint8_t numerator, const uint8_t denominator) :
        Meta<T>(time) {
        this->m_data
            = {static_cast<uint8_t>(meta_type),
               static_cast<uint8_t>(4),
               numerator,
               static_cast<uint8_t>(std::log2(denominator)),
               static_cast<uint8_t>(0x18),
               static_cast<uint8_t>(0x08)};
    };

    [[nodiscard]] uint8_t numerator() const { return this->m_data[2]; };
    [[nodiscard]] uint8_t denominator() const { return 1 << this->m_data[3]; };
};

template<typename T = container::SmallBytes>
struct KeySignature : Meta<T> {
    static constexpr auto meta_type = MetaType::KeySignature;

    KeySignature() = default;

    KeySignature(const uint32_t time, const int8_t key, const uint8_t tonality) : Meta<T>(time) {
        this->m_data
            = {static_cast<uint8_t>(meta_type),
               static_cast<uint8_t>(2),
               static_cast<uint8_t>(key),
               tonality};
    };

    [[nodiscard]] int8_t  key() const { return this->m_data[2]; };
    [[nodiscard]] uint8_t tonality() const { return this->m_data[3]; };

    [[nodiscard]] std::string name() const {
        constexpr static std::array<const char*, 32> KEYS_NAME
            = {"bC", "bG", "bD", "bA", "bE", "bB", "F", "C", "G", "D", "A", "E", "B", "#F", "#C",
               "bc", "bg", "bd", "ba", "be", "bb", "f", "c", "g", "d", "a", "e", "b", "#f", "#c"};
        return KEYS_NAME.at(key() + 7 + tonality() * 12);
    };
};

template<typename T = container::SmallBytes>
struct SMPTEOffset : Meta<T> {
    static constexpr auto meta_type = MetaType::SMPTEOffset;

    SMPTEOffset() = default;

    SMPTEOffset(
        const uint32_t time,
        const uint8_t  hour,
        const uint8_t  minute,
        const uint8_t  second,
        const uint8_t  frame,
        const uint8_t  subframe
    ) : Meta<T>(time) {
        this->m_data
            = {static_cast<uint8_t>(meta_type),
               static_cast<uint8_t>(5),
               hour,
               minute,
               second,
               frame,
               subframe};
    };

    [[nodiscard]] uint8_t hour() const { return this->m_data[2]; };
    [[nodiscard]] uint8_t minute() const { return this->m_data[3]; };
    [[nodiscard]] uint8_t second() const { return this->m_data[4]; };
    [[nodiscard]] uint8_t frame() const { return this->m_data[5]; };
    [[nodiscard]] uint8_t subframe() const { return this->m_data[6]; };
};

template<typename T = container::SmallBytes>
struct Text : Meta<T> {
    static constexpr auto meta_type = MetaType::Text;

    Text() = default;

    Text(const uint32_t time, const std::string& text) : Meta<T>(time, meta_type, text){};
};

template<typename T = container::SmallBytes>
struct TrackName : Meta<T> {
    static constexpr auto meta_type = MetaType::TrackName;

    TrackName() = default;

    TrackName(const uint32_t time, const std::string& name) : Meta<T>(time, meta_type, name){};
};

template<typename T = container::SmallBytes>
struct InstrumentName : Meta<T> {
    static constexpr auto meta_type = MetaType::InstrumentName;

    InstrumentName() = default;

    InstrumentName(const uint32_t time, const std::string& name) : Meta<T>(time, meta_type, name){};
};

template<typename T = container::SmallBytes>
struct Lyric : Meta<T> {
    static constexpr auto meta_type = MetaType::Lyric;

    Lyric() = default;

    Lyric(const uint32_t time, const std::string& lyric) : Meta<T>(time, meta_type, lyric){};
};

template<typename T = container::SmallBytes>
struct Marker : Meta<T> {
    static constexpr auto meta_type = MetaType::Marker;

    Marker() = default;

    Marker(const uint32_t time, const std::string& marker) : Meta<T>(time, meta_type, marker){};
};

template<typename T = container::SmallBytes>
struct CuePoint : Meta<T> {
    static constexpr auto meta_type = MetaType::CuePoint;

    CuePoint() = default;

    CuePoint(const uint32_t time, const std::string& cuePoint) :
        Meta<T>(time, meta_type, cuePoint){};
};

template<typename T = container::SmallBytes>
struct MIDIChannelPrefix : Meta<T> {
    static constexpr auto meta_type = MetaType::MIDIChannelPrefix;

    MIDIChannelPrefix() = default;

    MIDIChannelPrefix(const uint32_t time, const uint8_t channel) : Meta<T>(time) {
        this->m_data = {static_cast<uint8_t>(meta_type), static_cast<uint8_t>(1), channel};
    };
};

template<typename T = container::SmallBytes>
struct EndOfTrack : Meta<T> {
    static constexpr auto meta_type = MetaType::EndOfTrack;

    EndOfTrack() = default;

    explicit EndOfTrack(const uint32_t time) : Meta<T>(time) {
        this->m_data = {static_cast<uint8_t>(meta_type), static_cast<uint8_t>(0)};
    };
};


template<typename T = container::SmallBytes>
using Messages = std::vector<Message<T>>;



namespace utils {
inline void write_eot(uint8_t*& cursor) {
    write_variable_length(cursor, 1);
    *cursor = lut::to_msg_status(MessageType::Meta);
    ++cursor;
    *cursor = lut::to_meta_status(MetaType::EndOfTrack);
    ++cursor;
    *cursor = 0x00;
    ++cursor;
}

inline void write_eot(container::Bytes& bytes) {
    write_variable_length(bytes, 1);
    bytes.emplace_back(lut::to_msg_status(MessageType::Meta));
    bytes.emplace_back(lut::to_meta_status(MetaType::EndOfTrack));
    bytes.emplace_back(0x00);
}
}   // namespace utils


const std::string MTRK("MTrk");

class MessageGenerator {
protected:
    const uint8_t* cursor         = nullptr;
    const uint8_t* bufferEnd      = nullptr;
    size_t         prevEventLen   = 0;
    uint32_t       tickOffset     = 0;
    uint8_t        prevStatusCode = 0x00;

public:
    MessageGenerator(const uint8_t* begin, const uint8_t* end) : cursor(begin), bufferEnd(end) {}

    MessageGenerator(const uint8_t* begin, const size_t size) :
        cursor(begin), bufferEnd(begin + size) {}

    [[nodiscard]] bool done() const { return cursor >= bufferEnd; }

    template<typename Func>
    auto emplace_using(Func f) {
        check_range();
        tickOffset += utils::read_variable_length(cursor);
        switch (const uint8_t curStatusCode = *cursor; curStatusCode) {
        case 0xF0: return sysex_msg(curStatusCode, f);
        case 0xFF: return meta_msg(curStatusCode, f);
        default: {
            if (curStatusCode < 0x80) {
                return running_status_msg(curStatusCode, f);
            } else {
                return common_msg(curStatusCode, f);
            }
        }
        }
    }


protected:
    void check_range() const {
        if (cursor > bufferEnd) {
            throw std::ios_base::failure(
                "MiniMidi: Unexpected EOF in track! Cursor is " + std::to_string(cursor - bufferEnd)
                + " bytes beyond the end of buffer!"
            );
        }
    }

    template<typename Func>
    auto running_status_msg(const uint8_t curStatusCode, Func f) {
        if (!prevEventLen) {
            throw std::ios_base::failure(
                "MiniMidi: Unexpected running status! Get status code: "
                + std::to_string(curStatusCode) + " but previous event length is 0!"
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
        const auto curCursor = cursor;
        cursor += prevEventLen - 1;
        return f(tickOffset, prevStatusCode, curCursor, prevEventLen - 1);
    }

    template<typename Func>
    auto sysex_msg(const uint8_t curStatusCode, Func f) {
        prevStatusCode            = curStatusCode;
        const uint8_t* prevBuffer = cursor;

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
        cursor = prevBuffer + prevEventLen;
        return f(tickOffset, *prevBuffer, prevBuffer + 1, prevEventLen - 1);
    }

    template<typename Func>
    void meta_msg(const uint8_t curStatusCode, Func f) {
        // Meta message does not affect running status
        const uint8_t* prevBuffer = cursor;

        // Skip status byte and meta type byte
        cursor += 2;
        const auto eventLen = utils::read_variable_length(cursor) + (cursor - prevBuffer);

        if (prevBuffer + eventLen > bufferEnd) {
            throw std::ios_base::failure(
                "MiniMidi: Unexpected EOF in Meta Event! Cursor would be "
                + std::to_string(cursor + eventLen - bufferEnd)
                + " bytes beyond the end of buffer with previous event length "
                + std::to_string(eventLen) + "!"
            );
        }
        // The message data does not include the status byte,
        // but the meta type byte is included
        // this->emplace(tickOffset, *prevBuffer, prevBuffer + 1, eventLen - 1);
        const auto* metaBegin = prevBuffer + 1;
        if (lut::to_meta_type(*metaBegin) == MetaType::EndOfTrack) {
            cursor = bufferEnd;
        } else {
            cursor = prevBuffer + eventLen;
        }
        return f(tickOffset, *prevBuffer, metaBegin, eventLen - 1);
    }

    template<typename Func>
    void common_msg(const uint8_t curStatusCode, Func f) {
        prevStatusCode = curStatusCode;
        prevEventLen   = lut::get_msg_length(lut::to_msg_type(curStatusCode));

        if (cursor + prevEventLen > bufferEnd) {
            throw std::ios_base::failure(
                "MiniMidi: Unexpected EOF in MIDI Event! Cursor would be "
                + std::to_string(cursor + prevEventLen - bufferEnd)
                + " bytes beyond the end of buffer with previous event length "
                + std::to_string(prevEventLen) + "!"
            );
        }
        const auto curCursor = cursor;
        cursor += prevEventLen;
        // f(tickOffset, curStatusCode, curCursor + 1, prevEventLen - 1);
        // prevEventLen - 1 <= 2, and there must be a EndOfTrack after the last event
        return f(tickOffset, curStatusCode, curCursor + 1, 2);
    }
};

template<typename T = container::SmallBytes>
struct TrackView {
    const uint8_t* cursor = nullptr;
    size_t         size   = 0;

    class iterator : public MessageGenerator {
        Message<T> msg{};
    public:
        iterator(const uint8_t* cursor, const size_t size) : MessageGenerator(cursor, size){};
        iterator(const uint8_t* begin, const uint8_t* end) : MessageGenerator(begin, end){};
        iterator(const iterator& other)     = default;
        iterator(iterator&& other) noexcept = default;

        bool operator==(const iterator&) const { return this->done(); };
        bool operator!=(const iterator&) const { return !this->done(); };

        Message<T>&       operator*() { return msg; };
        const Message<T>& operator*() const { return msg; };

        iterator& operator++() {
            this->emplace_using([&](auto... args) {
                std::destroy_at(&msg);
                std::construct_at(&msg, args...);
            });
            return *this;
        };
    };

    TrackView(const uint8_t* cursor, const size_t size) : cursor(cursor), size(size){};

    TrackView(const TrackView& other)     = default;
    TrackView(TrackView&& other) noexcept = default;

    iterator begin() const { return iterator(cursor, size); };
    iterator end() const { return iterator(cursor + size, size); };
};


template<typename T = container::SmallBytes>
class Track {
public:
    Messages<T> messages;
                         Track() = default;
                         Track(Track<T>&& other) noexcept : messages(std::move(other.messages)){};
    Track(const Track<T>& other) : messages(other.messages.begin(), other.messages.end()){};

    Track(const uint8_t* cursor, const size_t size) {
        messages.reserve(size / 3 + 100);
        MessageGenerator generator(cursor, size);
        // Message<T> tmp{};
        while (!generator.done()) {
            generator.emplace_using([&](auto... args) {
                messages.emplace_back(args...);
                // std::destroy_at(&tmp);
                // std::construct_at(&tmp, args...);
            });
        }
        // messages.emplace_back(tmp);
    };

    explicit Track(const TrackView<T>& view) : Track(view.cursor, view.size){};

    explicit Track(Messages<T>&& message) {
        this->messages = std::vector(std::move(message));
    };

    Message<T>& message(const uint32_t index) { return this->messages[index]; };

    [[nodiscard]] const Message<T>& message(const uint32_t index) const {
        return this->messages[index];
    };

    [[nodiscard]] size_t message_num() const { return this->messages.size(); };

    [[nodiscard]] container::Bytes to_bytes() const {
        // (time, index)
        typedef std::pair<uint32_t, size_t> SortHelper;
        std::vector<SortHelper>             msgHeaders;
        msgHeaders.reserve(this->messages.size());
        size_t dataLen = 0;

        for (int i = 0; i < this->messages.size(); ++i) {
            if (this->messages[i].type() != MessageType::Meta
                || this->messages[i].template cast<Meta>().meta_type()
                       != MetaType::EndOfTrack) {
                msgHeaders.emplace_back(this->messages[i].time, i);
                dataLen += this->messages[i].data().size();
            }
        }

        std::sort(msgHeaders.begin(), msgHeaders.end(), std::less<SortHelper>());

        container::Bytes trackBytes(dataLen + 5 * msgHeaders.size() + 8);

        uint8_t* cursor     = trackBytes.data();
        uint32_t prevTime   = 0;
        uint8_t  prevStatus = 0x00;

        // Write track chunk header
        std::copy(MTRK.begin(), MTRK.end(), cursor);
        cursor += 8;
        for (const auto& [tick, idx] : msgHeaders) {
            const auto&    thisMsg   = this->messages[idx];
            const uint32_t curTime   = thisMsg.time;
            const uint8_t  curStatus = thisMsg.statusByte;

            utils::write_variable_length(cursor, curTime - prevTime);
            prevTime = curTime;

            // Not running status, write status byte
            if (curStatus == 0xFF || curStatus == 0xF0 || curStatus == 0xF7
                || curStatus != prevStatus) {
                *cursor = curStatus;
                ++cursor;
            }
            // Write data bytes
            std::copy(thisMsg.data().begin(), thisMsg.data().end(), cursor);
            cursor += thisMsg.data().size();

            prevStatus = curStatus;
        }
        // Write EOT
        utils::write_eot(cursor);

        // Write track chunk length
        utils::write_msb_bytes(trackBytes.data() + 4, cursor - trackBytes.data() - 8, 4);

        trackBytes.resize(cursor - trackBytes.data());

        return trackBytes;
    };
};

template<typename T = container::SmallBytes>
using Tracks = std::vector<Track<T>>;




constexpr static std::string MTHD("MThd");

#define MIDI_FORMAT                    \
    MIDI_FORMAT_MEMBER(SingleTrack, 0) \
    MIDI_FORMAT_MEMBER(MultiTrack, 1)  \
    MIDI_FORMAT_MEMBER(MultiSong, 2)

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

    return formats[static_cast<int>(format)];
};

inline MidiFormat read_midiformat(const uint16_t data) {
    switch (data) {
#define MIDI_FORMAT_MEMBER(type, status) \
    case status: return MidiFormat::type;
        MIDI_FORMAT
#undef MIDI_FORMAT_MEMBER
    default:
        throw std::ios_base::failure(
            "MiniMidi: Invaild midi format (" + std::to_string(data) + ")!"
            + "1 for single track, 2 for multi track, 3 for multi song."
        );
    }
};

struct MidiFileHeader {
    MidiFormat format{};
    uint16_t   divisionType : 1 {};

    union {
        struct {
            uint16_t ticksPerQuarter : 15;
        };
        struct {
            uint16_t negativeSmpte : 7;
            uint16_t ticksPerFrame : 8;
        };

    };

    constexpr static size_t HEADER_LENGTH = 14;

    MidiFileHeader() = default;

    MidiFileHeader(
        const MidiFormat format, const uint8_t divisionType, const uint16_t ticksPerQuarter
    ) : format(format), divisionType(divisionType), ticksPerQuarter(ticksPerQuarter){};

    MidiFileHeader(
        const MidiFormat format,
        const uint8_t    divisionType,
        const uint8_t    negativeSmpte,
        const uint8_t    ticksPerFrame
    ) :
        format(format), divisionType(divisionType), negativeSmpte(negativeSmpte),
        ticksPerFrame(ticksPerFrame){};

    MidiFileHeader(const uint8_t* data, const size_t size) {
        if (size < HEADER_LENGTH) {   // clang-format off
            throw std::ios_base::failure(
                "MiniMidi: Invaild midi file! File size is less than "
                + std::to_string(HEADER_LENGTH) + "!"
            );   // clang-format on
        }
        const uint8_t* cursor = data;

        // Check file begin with "MThd"
        if (std::string(reinterpret_cast<const char*>(cursor), 4) != MTHD) {
            throw std::ios_base::failure("MiniMidi: Invaild midi file! File header is not MThd!");
        }
        if (const auto chunkLen = utils::read_msb_bytes(cursor + 4, 4); chunkLen != 6) {
            throw std::ios_base::failure(
                "MiniMidi: Invaild midi file! The first chunk length is not 6, but "
                + std::to_string(chunkLen) + "!"
            );
        }
        this->format          = read_midiformat(utils::read_msb_bytes(cursor + 8, 2));
        this->divisionType    = ((*(cursor + 12)) & 0x80) >> 7;
        this->ticksPerQuarter = (((*(cursor + 12)) & 0x7F) << 8) + (*(cursor + 13));
    }
};

template<typename T = container::SmallBytes>
struct MidiFileView : MidiFileHeader {
    const uint8_t* cursor    = nullptr;
    const uint8_t* bufferEnd = nullptr;
    size_t         trackNum  = 0;

    MidiFileView() = default;
    MidiFileView(const uint8_t* data, const size_t size) : MidiFileHeader(data, size) {
        this->cursor    = data + HEADER_LENGTH;
        this->bufferEnd = data + size;
        this->trackNum  = utils::read_msb_bytes(data + 10, 2);
    }
    explicit MidiFileView(const container::Bytes& data) : MidiFileView(data.data(), data.size()){};

private:
    class iterator {
        const uint8_t* cursor    = nullptr;
        const uint8_t* bufferEnd = nullptr;
        size_t         trackIdx  = 0;
        size_t         trackNum  = 0;
        size_t         chunkLen  = 0;

        iterator() = default;

        iterator(const uint8_t* cursor, const uint8_t* bufferEnd, const size_t trackNum) :
            cursor(cursor), bufferEnd(bufferEnd), trackNum(trackNum) {
            parse_chunk_len();
        };


        void parse_chunk_len() {
            while (std::string(reinterpret_cast<const char*>(cursor), 4) != MTRK) {
                const size_t tmpLen = utils::read_msb_bytes(cursor + 4, 4);
                if (cursor + tmpLen + 8 > bufferEnd) {
                    throw std::ios_base::failure(
                        "MiniMidi: Unexpected EOF in file! Cursor is "
                        + std::to_string(cursor + chunkLen + 8 - bufferEnd)
                        + " bytes beyond the end of buffer with chunk length "
                        + std::to_string(chunkLen) + "!"
                    );
                }
                cursor += (8 + tmpLen);
            }
            chunkLen = utils::read_msb_bytes(cursor + 4, 4);
        }

        [[nodiscard]] bool done() const { return cursor >= bufferEnd || trackIdx >= trackNum; }

    public:
        friend class MidiFileView;

        TrackView<T> operator*() { return {cursor + 8, chunkLen}; };

        iterator& operator++() {
            cursor += (8 + chunkLen);
            trackIdx++;
            if (cursor >= bufferEnd || trackIdx >= trackNum) { return *this; }
            parse_chunk_len();
            return *this;
        }

        bool operator==(const iterator&) const { return done(); };
        bool operator!=(const iterator&) const { return !done(); };
    };

public:
    iterator begin() const { return iterator(cursor, bufferEnd, trackNum); };
    iterator end() const { return iterator(); };
};

template<typename T = container::SmallBytes>
struct MidiFile : MidiFileHeader {
    Tracks<T> tracks;

    MidiFile() = default;

    explicit MidiFile(const MidiFileView<T>& view) :
        MidiFileHeader(view.format, view.divisionType, view.ticksPerQuarter) {
        tracks.reserve(view.trackNum);
        for (auto track_view : view) { tracks.emplace_back(track_view); }
    }

    MidiFile(const uint8_t* const data, const size_t size) :
        MidiFile(MidiFileView<T>(data, size)){};

    explicit MidiFile(const container::Bytes& data) : MidiFile(data.data(), data.size()){};

    explicit MidiFile(
        MidiFormat format          = MidiFormat::MultiTrack,
        uint8_t    divisionType    = 0,
        uint16_t   ticksPerQuarter = 960
    ) : MidiFileHeader(format, divisionType, ticksPerQuarter){};

    explicit MidiFile(
        Tracks<T>&& tracks,
        MidiFormat         format          = MidiFormat::MultiTrack,
        uint8_t            divisionType    = 0,
        uint16_t           ticksPerQuarter = 960
    ) : MidiFileHeader(format, divisionType, ticksPerQuarter), tracks(std::move(tracks)){};

    explicit MidiFile(
        const Tracks<T>& tracks,
        MidiFormat              format          = MidiFormat::MultiTrack,
        uint8_t                 divisionType    = 0,
        uint16_t                ticksPerQuarter = 960
    ) : MidiFileHeader(format, divisionType, ticksPerQuarter), tracks(tracks){};

    static MidiFile from_file(const std::string& filepath) {
        FILE* filePtr = fopen(filepath.c_str(), "rb");

        if (!filePtr) { throw std::ios_base::failure("MiniMidi: Reading file failed (fopen)!"); }
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
        size_t                        trackByteNum = 0;
        for (auto i = 0; i < tracks.size(); ++i) {
            trackBytes[i] = tracks[i].to_bytes();
            trackByteNum += trackBytes[i].size();
        }

        container::Bytes midiBytes(trackByteNum + 14);

        // Write head
        std::copy(MTHD.begin(), MTHD.end(), midiBytes.begin());
        midiBytes[7] = 0x06;   // Length of head
        midiBytes[9] = static_cast<uint8_t>(format);
        utils::write_msb_bytes(midiBytes.data() + 10, tracks.size(), 2);
        utils::write_msb_bytes(midiBytes.data() + 12, (divisionType << 15) | ticksPerQuarter, 2);

        // Write track
        size_t cursor = 14;
        for (const auto& thisTrackBytes : trackBytes) {
            std::copy(
                thisTrackBytes.begin(),
                thisTrackBytes.end(),
                midiBytes.begin() + static_cast<long long>(cursor)
            );
            cursor += thisTrackBytes.size();
        }

        return midiBytes;
    };

    container::Bytes to_bytes_sorted() {
        container::Bytes bytes;
        size_t           approx_size = 32;
        for (const auto& track : tracks) { approx_size += track.message_num() * 5 + 16; }
        bytes.reserve(approx_size);

        // Write MIDI HEAD
        bytes.resize(14);
        std::uninitialized_copy(MTHD.begin(), MTHD.end(), bytes.begin());
        bytes[7] = 0x06;
        bytes[9] = static_cast<uint8_t>(format);
        utils::write_msb_bytes(bytes.data() + 10, tracks.size(), 2);
        utils::write_msb_bytes(bytes.data() + 12, (divisionType << 15 | ticksPerQuarter), 2);

        // Write Msgs for Each Track
        for (const auto& track : tracks) {
            size_t track_begin = bytes.size();
            // Write Track HEAD
            bytes.resize(bytes.size() + 8);
            std::uninitialized_copy(MTRK.begin(), MTRK.end(), bytes.end() - 8);
            // init prev
            uint32_t prevTime   = 0;
            uint8_t  prevStatus = 0x00;
            for (const auto& msg : track.messages) {
                const uint32_t curTime   = msg.time;
                const uint8_t  curStatus = msg.statusByte;
                // 1. write msg variable length
                utils::write_variable_length(bytes, curTime - prevTime);
                prevTime = curTime;
                // 2. write running status
                if ((curStatus == 0xFF) | (curStatus == 0xF0) | (curStatus == 0xF7)
                    | (curStatus != prevStatus)) {
                    bytes.emplace_back(curStatus);
                }
                // 3. write msg btyes
                const auto& msg_data = msg.data();
                utils::write_iter(bytes, msg_data.cbegin(), msg_data.cend());
                prevStatus = curStatus;
            }
            // Write EOT
            utils::write_eot(bytes);

            // Write track chunk length after MTRK
            utils::write_msb_bytes(
                bytes.data() + track_begin + 4, bytes.size() - track_begin - 8, 4
            );
            // Track Writting Finished
        }
        return std::move(bytes);
    }

    void write_file(const std::string& filepath) {
        FILE* filePtr = fopen(filepath.c_str(), "wb");

        if (!filePtr) { throw std::ios_base::failure("MiniMidi: Create file failed (fopen)!"); }
        const container::Bytes midiBytes = this->to_bytes();
        fwrite(midiBytes.data(), 1, midiBytes.size(), filePtr);
        fclose(filePtr);
    };

    [[nodiscard]] MidiFormat get_format() const { return this->format; };

    [[nodiscard]] std::string get_format_string() const {
        return format_to_string(this->get_format());
    }

    [[nodiscard]] uint16_t get_division_type() const { return this->divisionType; };

    [[nodiscard]] uint16_t get_tick_per_quarter() const {
        if (!this->get_division_type())
            return this->ticksPerQuarter;
        else {
            std::cerr << "Division type 1 have no tpq." << std::endl;
            return -1;
        };
    };

    [[nodiscard]] uint16_t get_frame_per_second() const {
        return (~(this->negativeSmpte - 1)) & 0x3F;
    };

    [[nodiscard]] uint16_t get_tick_per_second() const {
        if (this->get_division_type())
            return this->ticksPerFrame * this->get_frame_per_second();
        else {
            std::cerr << "Division type 0 have no tps." << std::endl;
            return -1;
        };
    };

    Track<T>& track(const uint32_t index) { return this->tracks[index]; };

    [[nodiscard]] const Track<T>& track(const uint32_t index) const {
        return this->tracks[index];
    };

    [[nodiscard]] size_t track_num() const { return this->tracks.size(); };
};

#undef MIDI_FORMAT



template<container::BasicArr T>
std::string to_string(const T& data) {
    // show in hex
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << "{ ";
    for (auto& d : data) ss << std::setw(2) << static_cast<int>(d) << " ";
    ss << "}" << std::dec;
    return ss.str();
};

// --- MIDI message to_string implementations ---
//clang-format off
template<typename T>
std::string to_string(const NoteOn<T>& note) {
    return "NoteOn: channel=" + std::to_string(note.channel()) + " pitch="
           + std::to_string(note.pitch()) + " velocity=" + std::to_string(note.velocity());
}

template<typename T>
std::string to_string(const NoteOff<T>& note) {
    return "NoteOff: channel=" + std::to_string(note.channel()) + " pitch="
           + std::to_string(note.pitch()) + " velocity=" + std::to_string(note.velocity());
}

template<typename T>
std::string to_string(const ProgramChange<T>& pc) {
    return "ProgramChange: channel=" + std::to_string(pc.channel())
           + " program=" + std::to_string(pc.program());
}

template<typename T>
std::string to_string(const ControlChange<T>& cc) {
    return "ControlChange: channel=" + std::to_string(cc.channel())
           + " control number=" + std::to_string(cc.control_number())
           + " control value=" + std::to_string(cc.control_value());
}
//clang-format on

// --- Meta message to_string implementations ---

template<typename T>
std::string to_string(const TrackName<T>& meta) {
    const auto& data = meta.meta_value();
    return std::string(data.begin(), data.end());
}

template<typename T>
std::string to_string(const InstrumentName<T>& meta) {
    const auto& data = meta.meta_value();
    return std::string(data.begin(), data.end());
}

template<typename T>
std::string to_string(const TimeSignature<T>& ts) {
    return std::to_string(ts.numerator()) + "/" + std::to_string(ts.denominator());
}

template<typename T>
std::string to_string(const SetTempo<T>& st) {
    return std::to_string(st.tempo());
}

template<typename T>
std::string to_string(const KeySignature<T>& ks) {
    return ks.name();
}

template<typename T>
static std::string to_string(const EndOfTrack<T>&) {
    return "EndOfTrack";
}

// Meta to_string dispatcher
template<typename T>
std::string to_string(const Meta<T>& meta) {
    std::string output = "Meta: (" + to_string(meta.meta_type()) + ") ";
    switch (meta.meta_type()) {
    case MetaType::TrackName:
        return output + to_string(meta.template cast<TrackName>());
    case MetaType::InstrumentName:
        return output + to_string(meta.template cast<InstrumentName>());
    case MetaType::TimeSignature:
        return output + to_string(meta.template cast<TimeSignature>());
    case MetaType::SetTempo:
        return output + to_string(meta.template cast<SetTempo>());
    case MetaType::KeySignature:
        return output + to_string(meta.template cast<KeySignature>());
    case MetaType::EndOfTrack:
        return output + to_string(meta.template cast<EndOfTrack>());
    default: return output + "value=" + to_string(meta.data());
    }
}

// --- General Message to_string function ---

template<typename T>
std::string to_string(const Message<T>& message) {
    std::string output = "time=" + std::to_string(message.time) + " | ";
    switch (message.type()) {
    case MessageType::NoteOn:
        return output + to_string(message.template cast<NoteOn>());
    case MessageType::NoteOff:
        return output + to_string(message.template cast<NoteOff>());
    case MessageType::ProgramChange:
        return output + to_string(message.template cast<ProgramChange>());
    case MessageType::ControlChange:
        return output + to_string(message.template cast<ControlChange>());
    case MessageType::Meta:
        return output + to_string(message.template cast<Meta>());
    default:
        return output + "Status code: " + std::to_string(lut::to_msg_status(message.type()))
               + " length=" + std::to_string(message.data().size());
    }
}

template<typename T>
std::string to_string(const Track<T>& track) {
    std::stringstream out;
    for (int j = 0; j < track.message_num(); ++j) {
        out << to_string(track.message(j)) << std::endl;
    }
    return out.str();
};

template<typename T>
std::string to_string(const MidiFile<T>& file) {
    std::stringstream out;
    out << "File format: " << file.get_format_string() << std::endl;
    out << "Division:\n"
        << "    Type: " << file.get_division_type() << std::endl;
    if (file.get_division_type()) {
        out << "    Tick per Second: " << file.get_tick_per_second() << std::endl;
    } else {
        out << "    Tick per Quarter: " << file.get_tick_per_quarter() << std::endl;
    }
    out << std::endl;

    for (int i = 0; i < file.track_num(); ++i) {
        out << "Track " << i << ": " << std::endl;
        out << to_string(file.track(i)) << std::endl;
    }
    return out.str();
};

}   // namespace minimidi
#endif   // MINIMIDI_HPP
