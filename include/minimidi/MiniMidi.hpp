/**
 * @file MiniMidi.hpp
 * @brief A lightweight, header-only MIDI file parser and writer
 * 
 * MiniMidi is a modern C++ library for parsing and writing MIDI files.
 * Features:
 * - Header-only implementation
 * - Zero-cost abstractions
 * - Modern C++ design (C++20)
 * - Small memory footprint with stack-allocated optimization
 * 
 * Core Classes Design:
 * The library provides two approaches for handling MIDI data:
 * 
 * 1. Eager Loading (MidiFile & Track)
 *    - MidiFile: Stores complete MIDI data in memory
 *    - Track: Contains vector of parsed MIDI messages
 *    - Suitable for editing and writing MIDI files
 *    - Higher memory usage but faster repeated access
 * 
 * 2. Lazy Parsing (MidiFileView & TrackView)
 *    - MidiFileView: Provides iterator-based access to MIDI data
 *    - TrackView: Parses MIDI messages on-the-fly
 *    - Ideal for fast one-time reading or memory-constrained scenarios
 *    - Lower memory footprint but requires parsing on each access
 * 
 * The View classes serve as efficient parsers that can be converted
 * to their storage counterparts when needed.
 * 
 * Code Structure:
 * 1. Core Classes
 *    - Message: Base class for all MIDI messages
 *    - Track/TrackView: Container and view for MIDI track data
 *    - MidiHeader: MIDI file header information  
 *    - MidiFile/MidiFileView: Main classes for MIDI file handling
 * 
 * The library provides two complementary approaches for handling MIDI data:
 * - Storage Classes (MidiFile, Track): For editing and manipulation
 * - View Classes (MidiFileView, TrackView): For efficient parsing
 * 
 * 2. Message Types (X-Macro based enums)
 *    - MessageType: MIDI message types (note on/off, etc.)
 *    - MetaType: Meta event types (tempo, time signature, etc.)
 * 
 * 3. Utility Namespaces
 *    - container: Container types and concepts
 *    - utils: Helper functions for MIDI parsing/writing
 *    - lut: Lookup tables for MIDI message types
 * 
 * 4. Implementation Sections
 *    - MIDI Parsing: TrackView and MidiFileView for efficient parsing
 *    - MIDI Writing: Serialization of MIDI data to bytes
 *    - String Formatting: to_string implementations for debugging
 * 
 * The library uses modern C++ features like concepts and templates
 * to provide type-safe and efficient MIDI file handling.
 */

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
#include <optional>
#include <memory>
#include "svector.h"

namespace minimidi {

/**
 * @brief Main namespace for the MiniMidi library
 * 
 * Contains all core functionality for parsing, manipulating and writing MIDI files.
 * The library is designed with modern C++ features and emphasizes efficiency through
 * template metaprogramming and zero-cost abstractions.
 */

/**
 * @brief Container types used throughout the library
 * 
 * Defines the basic container types for storing MIDI data:
 * - Bytes: Dynamic array for large MIDI data
 * - SmallBytes: Small size optimized vector of uint8_t
 */
namespace container {
typedef std::vector<uint8_t> Bytes;

// size of SmallBytes is totally 8 bytes on the stack (7 bytes + 1 byte for size)
typedef ankerl::svector<uint8_t, 7> SmallBytes;
}   // namespace container

/**
 * @brief Compile-time type constraints for MIDI data handling
 * 
 * Defines concepts that ensure type safety and proper behavior:
 * - ByteContainer: For storing MIDI data
 * - ByteIterator: For iterating over MIDI bytes
 * - MovableContainer: For efficient container movement
 * - InitializerListConstructible: For convenient message construction
 * - SizeConstructible: For optimized container initialization
 */
namespace concepts {
/**
 * @brief Concept for containers that can store MIDI data
 * 
 * Requirements:
 * - Has begin() and end() iterators
 * - Has size() returning integral type
 * - Elements convertible to uint8_t
 * - Constructible from iterator range
 */
template<typename T>
concept ByteContainer = requires(T a) {
    { a.begin() };
    { a.end() };
    { T(a.begin(), a.end()) };
    { a.size() } -> std::integral;
    { a[0] } -> std::convertible_to<uint8_t>;
};

/**
 * @brief Concept for move-constructible containers
 * 
 * Combines standard move constructibility with movable requirement
 */
template<typename T>
concept MovableContainer = std::is_move_constructible_v<T> && std::movable<T>;

/**
 * @brief Concept for iterators over byte data
 * 
 * Requirements:
 * - Can be dereferenced to uint8_t
 * - Supports increment and addition
 */
template<typename T>
concept ByteIterator = requires(T a) {
    { *a } -> std::convertible_to<uint8_t>;
    { a++ };
    { a + 10 };
};

/**
 * @brief Concept for containers supporting initializer list construction
 */
template<typename T>
concept InitializerListConstructible = requires(T a) {
    { T{0x00, 0x01, 0x02} };
};

/**
 * @brief Concept for containers supporting construction from begin and size
 */
template<typename T>
concept SizeConstructible = requires(T a) {
    { a.begin() };
    { a.size() };
    { T(a.begin(), a.size()) };
};
}   // namespace concepts

// Declarations for Message, Track, TrackView, MidiFileView and MidiFile
enum class MessageType : uint8_t;
enum class MetaType : uint8_t;

/**
 * @brief Base message class for all MIDI messages
 * @tparam T Container type for message data, defaults to SmallBytes
 */
template<concepts::ByteContainer T = container::SmallBytes>
class Message {
protected:
    T m_data;

public:
    uint32_t time{};
    uint8_t  statusByte{};

    /**
     * @brief Default constructor
     */
    Message() = default;

    /**
     * @brief Construct a message from data container
     * @param time Delta time in ticks
     * @param statusByte Status byte including channel
     * @param data Message data bytes
     */
    Message(const uint32_t time, const uint8_t statusByte, const T& data) :
        m_data(data.begin(), data.end()), time(time), statusByte(statusByte) {};

    /**
     * @brief Move construct a message from data container
     * @param time Delta time in ticks
     * @param statusByte Status byte including channel
     * @param data Message data bytes (moved from)
     */
    Message(const uint32_t time, const uint8_t statusByte, T&& data)
        requires concepts::MovableContainer<T>
        : m_data(std::move(data)), time(time), statusByte(statusByte) {};

    /**
     * @brief Construct from iterator range
     * @param time Delta time in ticks
     * @param statusByte Status byte including channel
     * @param begin Start of data range
     * @param end End of data range
     */
    template<concepts::ByteIterator Iter>
    Message(const uint32_t time, const uint8_t statusByte, Iter begin, Iter end) :
        m_data(begin, end), time(time), statusByte(statusByte){};

    /**
     * @brief Construct from iterator and size
     * @param time Delta time in ticks
     * @param statusByte Status byte including channel
     * @param begin Start of data
     * @param size Number of bytes
     */
    template<concepts::ByteIterator Iter>
    Message(const uint32_t time, const uint8_t statusByte, Iter begin, size_t size)
        requires concepts::SizeConstructible<T>
        : m_data(begin, size), time(time), statusByte(statusByte){};

    // constructor from begin and size for container that does not support BeginSizeConstructor
    // like std::vector
    template<concepts::ByteIterator Iter>
    Message(const uint32_t time, const uint8_t statusByte, Iter begin, size_t size)
        requires(!concepts::SizeConstructible<T>)
        : m_data(begin, begin + size), time(time), statusByte(statusByte){};

    // constructor from initializer list
    Message(const uint32_t time, const uint8_t statusByte, std::initializer_list<uint8_t> list) :
        m_data(list), time(time), statusByte(statusByte) {};

    // copy constructor from another message with possibly different data type
    template<typename U>
    explicit Message(const Message<U>& other) :
        m_data(other.data().begin(), other.data().end()), time(other.time),
        statusByte(other.statusByte){};

    Message(const Message& other) :
        m_data(other.m_data.begin(), other.m_data.end()), time(other.time),
        statusByte(other.statusByte) {};

    // move constructor from another message with same data type
    Message(Message&& other) noexcept
        requires concepts::MovableContainer<T>
        : m_data(std::move(other.m_data)), time(other.time), statusByte(other.statusByte) {};

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

    [[nodiscard]] uint8_t channel() const { return statusByte & 0x0F; };

    [[nodiscard]] MessageType type() const;

    /**
     * @brief Copy assignment operator
     * @param other Message to copy from
     * @return Reference to this message
     * 
     * Performs self-assignment check before copying
     */
    Message& operator=(const Message& other) {
        if (this != &other) {
            m_data = T(other.m_data.begin(), other.m_data.end());
            time = other.time;
            statusByte = other.statusByte;
        }
        return *this;
    }

    /**
     * @brief Move assignment operator
     * @param other Message to move from
     * @return Reference to this message
     * 
     * Performs self-assignment check before moving
     */
    Message& operator=(Message&& other) noexcept
        requires concepts::MovableContainer<T>
    {
        if (this != &other) {
            m_data = std::move(other.m_data);
            time = other.time;
            statusByte = other.statusByte;
        }
        return *this;
    }
};

template<typename T = container::SmallBytes>
using Messages = std::vector<Message<T>>;

/**
 * @brief View class for parsing a single MIDI track
 * @tparam T Container type for messages, defaults to SmallBytes
 * 
 * TrackView provides an iterator interface for parsing MIDI messages
 * from a track's raw data. Messages are parsed on-demand during iteration.
 * 
 * Example usage:
 * @code
 * // Create view from raw track data
 * const uint8_t* track_data = ...;  // Pointer to track chunk data
 * size_t track_size = ...;          // Size of track chunk in bytes
 * TrackView<> view(track_data, track_size);
 * 
 * // Iterate through messages
 * for(const auto& msg : view) {
 *     // Process each message
 *     if(msg.type() == MessageType::NoteOn) {
 *         auto& note = msg.cast<NoteOn>();
 *         // Access note parameters: note.pitch(), note.velocity(), note.channel()
 *     }
 * }
 * 
 * // Convert to Track for storage/manipulation
 * Track<> track(view); // Parses all messages at once
 * @endcode
 */
template<typename T = container::SmallBytes>
struct TrackView {
    const uint8_t* cursor = nullptr;
    size_t         size   = 0;

    /**
     * @brief Construct a view over track data
     * @param cursor Pointer to start of track chunk data
     * @param size Size of track chunk in bytes
     */
    TrackView(const uint8_t* cursor, const size_t size) :
        cursor(cursor), size(static_cast<size_t>(size)) {};

    TrackView(const TrackView& other)     = default;
    TrackView(TrackView&& other) noexcept = default;

    class iterator;

    iterator begin() const;
    iterator end() const;

    /**
     * @brief Copy assignment operator
     * @param other TrackView to copy from
     * @return Reference to this view
     * 
     * Performs self-assignment check before copying
     */
    TrackView& operator=(const TrackView& other) {
        if (this != &other) {
            cursor = other.cursor;
            size = other.size;
        }
        return *this;
    }

    /**
     * @brief Move assignment operator
     * @param other TrackView to move from
     * @return Reference to this view
     * 
     * Performs self-assignment check before moving
     */
    TrackView& operator=(TrackView&& other) noexcept {
        if (this != &other) {
            cursor = other.cursor;
            size = other.size;
            other.cursor = nullptr;
            other.size = 0;
        }
        return *this;
    }
};

/**
 * @brief Container class for storing MIDI track data
 * @tparam T Container type for messages, defaults to SmallBytes
 * 
 * Track stores a complete vector of parsed MIDI messages.
 * It can be constructed from a TrackView when persistent
 * storage or message manipulation is needed.
 */
template<typename T = container::SmallBytes>
class Track {
public:
    Messages<T> messages;

    /**
     * @brief Default constructor
     */
    Track() = default;

    /**
     * @brief Move constructor
     */
    Track(Track<T>&& other) noexcept : messages(std::move(other.messages)) {};

    /**
     * @brief Copy constructor
     */
    Track(const Track<T>& other) : messages(other.messages.begin(), other.messages.end()) {};

    /**
     * @brief Construct from raw track data
     * @param cursor Pointer to track chunk data
     * @param size Size of track chunk in bytes
     */
    Track(const uint8_t* cursor, size_t size);

    /**
     * @brief Construct from track view
     * @param view TrackView to parse
     */
    explicit Track(const TrackView<T>& view) : Track(view.cursor, view.size) {};

    /**
     * @brief Construct from message vector
     * @param messages Vector of MIDI messages (moved from)
     */
    explicit Track(Messages<T>&& messages) { this->messages = std::vector(std::move(messages)); };

    auto begin() const { return this->messages.begin(); };
    auto begin() { return this->messages.begin(); };

    auto end() const { return this->messages.end(); };
    auto end() { return this->messages.end(); };

    [[nodiscard]] size_t size() const { return this->messages.size(); };
    [[nodiscard]] size_t message_num() const { return this->messages.size(); };

    Track<T> sort() const;

    /**
     * @brief Copy assignment operator
     * @param other Track to copy from
     * @return Reference to this track
     * 
     * Performs self-assignment check before copying
     */
    Track& operator=(const Track& other) {
        if (this != &other) {
            messages = Messages<T>(other.messages.begin(), other.messages.end());
        }
        return *this;
    }

    /**
     * @brief Move assignment operator
     * @param other Track to move from
     * @return Reference to this track
     * 
     * Performs self-assignment check before moving
     */
    Track& operator=(Track&& other) noexcept {
        if (this != &other) {
            messages = std::move(other.messages);
        }
        return *this;
    }
};

template<typename T = container::SmallBytes>
using Tracks = std::vector<Track<T>>;

enum class MidiFormat : uint8_t;

/**
 * @brief MIDI file header information
 */
class MidiHeader {
protected:
    MidiFormat m_format{};
    uint16_t   m_divisionType : 1 {};

    union {
        struct {
            uint16_t m_ticksPerQuarter : 15;
        };
        struct {
            uint16_t m_negativeSmpte : 7;
            uint16_t m_ticksPerFrame : 8;
        };
    };

public:
    constexpr static size_t HEADER_LENGTH = 14;

    MidiHeader() = default;

    MidiHeader(
        const MidiFormat format, const uint8_t divisionType, const uint16_t ticksPerQuarter
    ) : m_format(format), m_divisionType(divisionType), m_ticksPerQuarter(ticksPerQuarter) {};

    MidiHeader(
        const MidiFormat format,
        const uint8_t    divisionType,
        const uint8_t    negativeSmpte,
        const uint8_t    ticksPerFrame
    ) :
        m_format(format), m_divisionType(divisionType), m_negativeSmpte(negativeSmpte),
        m_ticksPerFrame(ticksPerFrame) {};

    MidiHeader(const uint8_t* data, size_t size);

    [[nodiscard]] MidiFormat format() const { return m_format; };
    [[nodiscard]] uint16_t   division_type() const { return m_divisionType; };
    [[nodiscard]] uint16_t   ticks_per_quarter() const;
    [[nodiscard]] uint16_t   frame_per_second() const;
    [[nodiscard]] uint16_t   ticks_per_frame() const;
    [[nodiscard]] uint16_t   ticks_per_second() const;
};

/**
 * @brief View class for parsing MIDI files with minimal memory usage
 * @tparam T Container type for messages, defaults to SmallBytes
 * 
 * MidiFileView provides an iterator-based interface for parsing MIDI files.
 * It reads track data on-demand without loading the entire file into memory.
 * 
 * Example usage:
 * @code
 * // Read MIDI file data
 * const uint8_t* file_data = ...;  // Pointer to MIDI file data
 * size_t file_size = ...;          // Size of file in bytes
 * 
 * // Create view from raw file data
 * MidiFileView<> view(file_data, file_size);
 * 
 * // Access file properties
 * auto format = view.format();              // MIDI format (0, 1, or 2)
 * auto division = view.division_type();     // Timing division type
 * auto ticks = view.ticks_per_quarter();    // Ticks per quarter note
 * 
 * // Iterate through tracks
 * for(const auto& track_view : view) {
 *     // Each track_view is a TrackView object
 *     for(const auto& msg : track_view) {
 *         // Process each message
 *     }
 * }
 * 
 * // Convert to MidiFile for full access/editing
 * MidiFile<> midi(view); // Parses entire file
 * @endcode
 * 
 * The view classes provide efficient parsing for scenarios where:
 * - Only a subset of messages needs to be processed
 * - Memory usage needs to be minimized
 * - One-time sequential access is sufficient
 */
template<typename T = container::SmallBytes>
struct MidiFileView : public MidiHeader {
    const uint8_t* cursor    = nullptr;
    const uint8_t* bufferEnd = nullptr;
    size_t         trackNum  = 0;

    /**
     * @brief Default constructor
     */
    MidiFileView() = default;

    /**
     * @brief Construct a view over MIDI file data
     * @param data Pointer to MIDI file data
     * @param size Size of file in bytes
     * @throws std::ios_base::failure if header is invalid
     */
    MidiFileView(const uint8_t* data, size_t size);

    /**
     * @brief Construct from byte vector
     * @param data Vector containing MIDI file data
     */
    explicit MidiFileView(const container::Bytes& data) : MidiFileView(data.data(), data.size()) {};

    class iterator;
    iterator begin() const { return iterator(cursor, bufferEnd, trackNum); };
    iterator end() const { return iterator(); };

    size_t track_num() const { return trackNum; };

    /**
     * @brief Copy assignment operator
     * @param other MidiFileView to copy from
     * @return Reference to this view
     * 
     * Performs self-assignment check before copying
     */
    MidiFileView& operator=(const MidiFileView& other) {
        if (this != &other) {
            MidiHeader::operator=(other);
            cursor = other.cursor;
            bufferEnd = other.bufferEnd;
            trackNum = other.trackNum;
        }
        return *this;
    }

    /**
     * @brief Move assignment operator
     * @param other MidiFileView to move from
     * @return Reference to this view
     * 
     * Performs self-assignment check before moving
     */
    MidiFileView& operator=(MidiFileView&& other) noexcept {
        if (this != &other) {
            MidiHeader::operator=(std::move(other));
            cursor = other.cursor;
            bufferEnd = other.bufferEnd;
            trackNum = other.trackNum;
            other.cursor = nullptr;
            other.bufferEnd = nullptr;
            other.trackNum = 0;
        }
        return *this;
    }
};

/**
 * @brief Main class for storing and manipulating MIDI files
 * @tparam T Container type for messages, defaults to SmallBytes
 * 
 * MidiFile stores the complete MIDI data in memory, allowing for
 * editing, sorting, and writing operations. It can be constructed
 * from a MidiFileView when full data access is needed.
 */
template<typename T = container::SmallBytes>
struct MidiFile : public MidiHeader {
    Tracks<T> tracks;

    /**
     * @brief Default constructor
     */
    MidiFile() = default;

    /**
     * @brief Construct from file view
     * @param view MidiFileView to parse
     */
    explicit MidiFile(const MidiFileView<T>& view);

    /**
     * @brief Construct from raw file data
     * @param data Pointer to MIDI file data
     * @param size Size of file in bytes
     */
    MidiFile(const uint8_t* const data, const size_t size);

    /**
     * @brief Construct from byte vector
     * @param data Vector containing MIDI file data
     */
    explicit MidiFile(const container::Bytes& data) : MidiFile(data.data(), data.size()) {};

    /**
     * @brief Construct empty MIDI file
     * @param format MIDI format (0, 1, or 2)
     * @param divisionType Timing division type (0 for ticks per quarter)
     * @param ticksPerQuarter Number of ticks per quarter note
     */
    explicit MidiFile(
        MidiFormat format, 
        uint8_t divisionType = 0, 
        uint16_t ticksPerQuarter = 960
    ) : MidiHeader(format, divisionType, ticksPerQuarter) {};

    /**
     * @brief Construct from tracks (move)
     * @param tracks Vector of tracks to move from
     * @param format MIDI format
     * @param divisionType Timing division type
     * @param ticksPerQuarter Ticks per quarter note
     */
    explicit MidiFile(
        Tracks<T>&& tracks,
        MidiFormat  format,
        uint8_t     divisionType    = 0,
        uint16_t    ticksPerQuarter = 960
    ) : MidiHeader(format, divisionType, ticksPerQuarter), tracks(std::move(tracks)) {};

    /**
     * @brief Construct from tracks (copy)
     * @param tracks Vector of tracks to copy from
     * @param format MIDI format
     * @param divisionType Timing division type
     * @param ticksPerQuarter Ticks per quarter note
     */
    explicit MidiFile(
        const Tracks<T>& tracks,
        MidiFormat       format,
        uint8_t          divisionType    = 0,
        uint16_t         ticksPerQuarter = 960
    ) : MidiHeader(format, divisionType, ticksPerQuarter), tracks(tracks) {};

    /**
     * @brief Load a MIDI file from disk
     * @param filepath Path to the MIDI file
     * @return MidiFile object containing the parsed file
     * @throws std::ios_base::failure if file cannot be opened or parsed
     */
    static MidiFile from_file(const std::string& filepath);

    /**
     * @brief Write MIDI data to a file
     * @param filepath Path where the MIDI file will be written
     * @throws std::ios_base::failure if file cannot be written
     */
    void write_file(const std::string& filepath) const;

    [[nodiscard]] size_t track_num() const { return this->tracks.size(); };
    [[nodiscard]] size_t size() const { return this->tracks.size(); };

    auto begin() const { return this->tracks.begin(); };
    auto begin() { return this->tracks.begin(); };

    auto end() const { return this->tracks.end(); };
    auto end() { return this->tracks.end(); };

    /**
     * @brief Sort (stable) all messages in all tracks by time
     * @return New MidiFile with sorted messages
     */
    MidiFile<T> sort() const;

    /**
     * @brief Convert MIDI data to raw bytes
     * @return Vector of bytes representing the MIDI file
     */
    container::Bytes to_bytes() const;
    container::Bytes to_bytes_sorted() const;

    /**
     * @brief Copy assignment operator
     * @param other MidiFile to copy from
     * @return Reference to this file
     * 
     * Performs self-assignment check before copying
     */
    MidiFile& operator=(const MidiFile& other) {
        if (this != &other) {
            MidiHeader::operator=(other);
            tracks = Tracks<T>(other.tracks.begin(), other.tracks.end());
        }
        return *this;
    }

    /**
     * @brief Move assignment operator
     * @param other MidiFile to move from
     * @return Reference to this file
     * 
     * Performs self-assignment check before moving
     */
    MidiFile& operator=(MidiFile&& other) noexcept {
        if (this != &other) {
            MidiHeader::operator=(std::move(other));
            tracks = std::move(other.tracks);
        }
        return *this;
    }
};

/**
 * @brief X-Macro definitions for MIDI message types
 * 
 * Format: MIDI_MESSAGE_TYPE_MEMBER(type, status, length)
 * - type: Enum name for the message type
 * - status: Status byte value (0x80-0xFF)
 * - length: Fixed message length in bytes (1-3, or 65535 for variable length)
 * 
 * Used to generate:
 * - MessageType enum
 * - Status byte lookup table
 * - Message length lookup table
 */
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

/**
 * @brief X-Macro definitions for MIDI meta event types
 * 
 * Format: MIDI_META_TYPE_MEMBER(type, status)
 * - type: Enum name for the meta event type
 * - status: Meta type byte value (0x00-0xFF)
 * 
 * Used to generate:
 * - MetaType enum
 * - Meta type lookup table
 * - String conversion functions
 */
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

namespace format {
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
}   // namespace format

/**
 * @brief Lookup tables and conversion utilities for MIDI messages
 * 
 * Provides compile-time lookup tables and conversion functions for MIDI message handling:
 * - Message type/status byte conversion
 * - Message length determination
 * - Meta event type conversion
 * - Channel and status byte manipulation
 * 
 * All tables are generated at compile-time using constexpr functions
 * to ensure zero runtime overhead. The namespace uses X-Macro based
 * enum definitions to maintain consistency between different lookup tables.
 */
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


/**
 * @brief Utility functions for MIDI data manipulation
 * 
 * Core utilities for MIDI file parsing and writing:
 * - Variable Length Quantity (VLQ) encoding/decoding
 * - Most Significant Byte (MSB) operations
 * - End of Track message handling
 * - Iterator-based data writing
 * 
 * These utilities handle the low-level details of the MIDI file format,
 * ensuring correct byte ordering and data representation.
 */
namespace utils {

/**
 * @brief Read a variable-length quantity from a byte stream
 * @tparam Iter Iterator type for byte stream
 * @param buffer Reference to iterator, will be advanced past the read bytes
 * @return Decoded 32-bit value
 * 
 * Reads a MIDI variable-length quantity, which can be 1-4 bytes long.
 * Each byte uses 7 bits for the value and 1 bit to indicate continuation.
 */
template<concepts::ByteIterator Iter>
uint32_t read_variable_length(Iter& buffer) {
    uint32_t value = 0;
    uint8_t byte;
    
    // Use constant masks for better readability and potential compiler optimization
    constexpr uint8_t CONT_BIT = 0x80;  // Continuation bit mask
    constexpr uint8_t DATA_BITS = 0x7F; // Data bits mask

    // Unroll loop into four independent steps for better performance
    // GCC won't unroll the loop, so we do it manually
    byte = *buffer++;
    value = byte & DATA_BITS;
    if (!(byte & CONT_BIT)) [[likely]] return value;

    byte = *buffer++;
    value = (value << 7) | (byte & DATA_BITS);
    if (!(byte & CONT_BIT)) return value;

    byte = *buffer++;
    value = (value << 7) | (byte & DATA_BITS);
    if (!(byte & CONT_BIT)) return value;

    byte = *buffer++;
    value = (value << 7) | (byte & DATA_BITS);
    
    return value; // Return directly even if MSB is 1, as MIDI spec limits to 4 bytes
};

/**
 * @brief Read a multi-byte value in MSB (Most Significant Byte) order
 * @param buffer Pointer to start of bytes
 * @param length Number of bytes to read
 * @return Combined value as uint64_t
 */
inline uint64_t read_msb_bytes(const uint8_t* buffer, const size_t length) {
    uint64_t res = 0;

    for (auto i = 0; i < length; ++i) {
        res <<= 8;
        res += (*(buffer + i));
    }

    return res;
};

/**
 * @brief Write a value in MSB order to a byte buffer
 * @param buffer Pointer to destination buffer
 * @param value Value to write
 * @param length Number of bytes to write
 */
inline void write_msb_bytes(uint8_t* buffer, const size_t value, const size_t length) {
    for (auto i = 1; i <= length; ++i) {
        *buffer = static_cast<uint8_t>((value >> ((length - i) * 8)) & 0xFF);
        ++buffer;
    }
};

/**
 * @brief Calculate number of bytes needed for a variable-length quantity
 * @param num Value to encode
 * @return Number of bytes needed (1-4)
 */
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

/**
 * @brief Write a variable-length quantity to a byte buffer
 * @param buffer Reference to buffer pointer, will be advanced
 * @param num Value to encode
 * 
 * Encodes a number using MIDI's variable-length quantity format
 * and writes it to the buffer.
 */
inline void write_variable_length(uint8_t*& buffer, const uint32_t num) {
    const uint8_t byteNum = calc_variable_length(num);

    for (auto i = 0; i < byteNum - 1; ++i) {
        *buffer = (num >> (7 * (byteNum - i - 1))) | 0x80;
        ++buffer;
    }
    *buffer = num & 0x7F;
    ++buffer;
};

/**
 * @brief Write a variable-length quantity to a byte vector
 * @param bytes Vector to append bytes to
 * @param num Value to encode
 */
inline void write_variable_length(container::Bytes& bytes, const uint32_t num) {
    const uint8_t byteNum = calc_variable_length(num);

    for (auto i = 0; i < byteNum - 1; ++i) {
        bytes.emplace_back((num >> (7 * (byteNum - i - 1))) | 0x80);
    }
    bytes.emplace_back(num & 0x7F);
};

/**
 * @brief Copy a range of bytes to a byte vector
 * @tparam Iter Iterator type for source range
 * @param bytes Destination vector
 * @param begin Start of source range
 * @param end End of source range
 */
template<typename Iter>
void write_iter(container::Bytes& bytes, Iter begin, Iter end) {
    bytes.insert(bytes.end(), begin, end);
}

/**
 * @brief Create a SmallBytes container with a variable-length quantity
 * @param num Value to encode
 * @return SmallBytes containing the encoded value
 */
inline container::SmallBytes make_variable_length(uint32_t num) {
    const uint8_t byteNum = calc_variable_length(num);

    container::SmallBytes result(byteNum);
    auto*                 cursor = const_cast<uint8_t*>(result.data());
    write_variable_length(cursor, num);

    return result;
};

/**
 * @brief Write End of Track message to a byte buffer
 * @param cursor Reference to buffer pointer, will be advanced
 * 
 * Writes a complete End of Track meta message, including:
 * - Delta time (1)
 * - Meta event status (0xFF)
 * - End of Track type (0x2F)
 * - Length byte (0x00)
 */
inline void write_eot(uint8_t*& cursor) {
    write_variable_length(cursor, 1);
    *cursor = lut::to_msg_status(MessageType::Meta);
    ++cursor;
    *cursor = lut::to_meta_status(MetaType::EndOfTrack);
    ++cursor;
    *cursor = 0x00;
    ++cursor;
}

/**
 * @brief Write End of Track message to a byte vector
 * @param bytes Vector to append the message to
 */
inline void write_eot(container::Bytes& bytes) {
    write_variable_length(bytes, 1);
    bytes.emplace_back(lut::to_msg_status(MessageType::Meta));
    bytes.emplace_back(lut::to_meta_status(MetaType::EndOfTrack));
    bytes.emplace_back(0x00);
}
}   // namespace utils

template<concepts::ByteContainer T>
MessageType Message<T>::type() const {
    return lut::to_msg_type(statusByte);
}


/**
 * @brief MIDI message type definitions and implementations
 * 
 * Contains all MIDI message classes derived from base Message class:
 * - Common Messages (NoteOn, NoteOff, etc.)
 * - Meta Messages (Tempo, TimeSignature, etc.)
 * 
 * Each message type provides:
 * - Type-safe construction
 * - Convenient accessors for message parameters
 * - Compile-time message type information
 * - Zero-cost downcasting through template specialization
 */
namespace messages {
template<typename T = container::SmallBytes>
class NoteOn : public Message<T> {
public:
    static constexpr auto type   = MessageType::NoteOn;
    static constexpr auto status = lut::to_msg_status(type);

    NoteOn() = default;
    NoteOn(const uint32_t time, const uint8_t channel, const uint8_t pitch, const uint8_t velocity)
        requires concepts::InitializerListConstructible<T>
        : Message<T>(time, lut::to_status_byte(type, channel), {pitch, velocity}) {};

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
        requires concepts::InitializerListConstructible<T>
        : Message<T>(time, lut::to_status_byte(type, channel), {pitch, velocity}) {};

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
        requires concepts::InitializerListConstructible<T>
        : Message<T>(time, lut::to_status_byte(type, channel), {controlNumber, controlValue}) {};

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
        Message<T>(time, lut::to_status_byte(type, channel), {program}) {};

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
        requires concepts::InitializerListConstructible<T>
        : Message<T>(time, status, {static_cast<uint8_t>((type << 4) | value)}) {};

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
        requires concepts::InitializerListConstructible<T>: Message<T>(
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

    static constexpr int16_t MIN_PITCH_BEND = -8192, MAX_PITCH_BEND = 8191;

    PitchBend() = default;
    // clang-format off
    PitchBend(const uint32_t time, const uint8_t channel, const int16_t value)
        requires concepts::InitializerListConstructible<T>: Message<T>(
            time, lut::to_status_byte(status, channel), {
                static_cast<uint8_t>((value - MIN_PITCH_BEND) & 0x7F),
                static_cast<uint8_t>((value - MIN_PITCH_BEND) >> 7)
            }
        ) {};

    [[nodiscard]] int16_t pitch_bend() const {
        const auto value =
            static_cast<int>(this->m_data[0])
          | static_cast<int>(this->m_data[1]) << 7;
        return value + MIN_PITCH_BEND;
    };
    // clang-format on
};

template<typename T = container::SmallBytes>
struct Meta : Message<T> {
public:  // 添加 public 关键字
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
public:
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
public:
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
public:  // 添加 public 关键字
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
public:
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
public:
    static constexpr auto meta_type = MetaType::Text;

    Text() = default;

    Text(const uint32_t time, const std::string& text) : Meta<T>(time, meta_type, text) {};
};

template<typename T = container::SmallBytes>
struct TrackName : Meta<T> {
public:
    static constexpr auto meta_type = MetaType::TrackName;

    TrackName() = default;

    TrackName(const uint32_t time, const std::string& name) : Meta<T>(time, meta_type, name) {};
};

template<typename T = container::SmallBytes>
struct InstrumentName : Meta<T> {
public:
    static constexpr auto meta_type = MetaType::InstrumentName;

    InstrumentName() = default;

    InstrumentName(const uint32_t time, const std::string& name) :
        Meta<T>(time, meta_type, name) {};
};

template<typename T = container::SmallBytes>
struct Lyric : Meta<T> {
public:
    static constexpr auto meta_type = MetaType::Lyric;

    Lyric() = default;

    Lyric(const uint32_t time, const std::string& lyric) : Meta<T>(time, meta_type, lyric) {};
};

template<typename T = container::SmallBytes>
struct Marker : Meta<T> {
public:
    static constexpr auto meta_type = MetaType::Marker;

    Marker() = default;

    Marker(const uint32_t time, const std::string& marker) : Meta<T>(time, meta_type, marker) {};
};

template<typename T = container::SmallBytes>
struct CuePoint : Meta<T> {
public:
    static constexpr auto meta_type = MetaType::CuePoint;

    CuePoint() = default;

    CuePoint(const uint32_t time, const std::string& cuePoint) :
        Meta<T>(time, meta_type, cuePoint) {};
};

template<typename T = container::SmallBytes>
struct MIDIChannelPrefix : Meta<T> {
public:
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
}   // namespace messages

using namespace messages;

constexpr static std::string MTHD("MThd");

#define MIDI_FORMAT                    \
    MIDI_FORMAT_MEMBER(SingleTrack, 0) \
    MIDI_FORMAT_MEMBER(MultiTrack, 1)  \
    MIDI_FORMAT_MEMBER(MultiSong, 2)

enum class MidiFormat : uint8_t {
#define MIDI_FORMAT_MEMBER(type, status) type = status,
    MIDI_FORMAT
#undef MIDI_FORMAT_MEMBER
};


namespace format {
inline const std::string& to_string(const MidiFormat& format) {
    static const std::string formats[] = {
#define MIDI_FORMAT_MEMBER(type, status) #type,
        MIDI_FORMAT
#undef MIDI_FORMAT_MEMBER
    };

    return formats[static_cast<int>(format)];
};
}   // namespace format

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

#undef MIDI_FORMAT

// MIDI Parser Implementation

const std::string MTRK("MTrk");

// MIDI Parser Implementation

namespace details {
class MessageGenerator {
protected:
    const uint8_t* cursor         = nullptr;
    const uint8_t* bufferEnd      = nullptr;
    size_t         prevEventLen   = 0;
    uint32_t       tickOffset     = 0;
    uint8_t        prevStatusCode = 0x00;
    bool           foundEOT       = false;

public:
    MessageGenerator() = default;

    MessageGenerator(const uint8_t* begin, const uint8_t* end) : cursor(begin), bufferEnd(end) {}

    MessageGenerator(const uint8_t* begin, const size_t size) :
        cursor(begin), bufferEnd(begin + static_cast<size_t>(size)) {}

    [[nodiscard]] bool done() const { return cursor >= bufferEnd; }

    template<typename Func>
    void emplace_using(Func f) {
        // check_range();
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
    void running_status_msg(const uint8_t curStatusCode, Func f) {
        if (!prevEventLen) [[unlikely]] {
            throw std::ios_base::failure(
                "MiniMidi: Unexpected running status! Get status code: "
                + std::to_string(curStatusCode) + " but previous event length is 0!"
            );
        }
        if (cursor + prevEventLen - 1 > bufferEnd) [[unlikely]] {
            throw std::ios_base::failure(
                "MiniMidi: Unexpected EOF in running status! Cursor would be "
                + std::to_string(cursor + prevEventLen - 1 - bufferEnd)
                + " bytes beyond the end of buffer with previous event length "
                + std::to_string(prevEventLen) + "!"
            );
        }
        const auto curCursor = cursor;
        cursor += prevEventLen - 1;
        f(tickOffset, prevStatusCode, curCursor, prevEventLen - 1);
    }

    template<typename Func>
    void sysex_msg(const uint8_t curStatusCode, Func f) {
        prevStatusCode            = curStatusCode;
        const uint8_t* prevBuffer = cursor;

        // Skip status byte
        cursor += 1;
        prevEventLen = utils::read_variable_length(cursor) + (cursor - prevBuffer);

        if (prevBuffer + prevEventLen > bufferEnd) [[unlikely]] {
            throw std::ios_base::failure(
                "MiniMidi: Unexpected EOF in SysEx Event! Cursor would be "
                + std::to_string(cursor + prevEventLen - bufferEnd)
                + " bytes beyond the end of buffer with previous event length "
                + std::to_string(prevEventLen) + "!"
            );
        }
        cursor = prevBuffer + prevEventLen;
        f(tickOffset, *prevBuffer, prevBuffer + 1, prevEventLen - 1);
    }

    template<typename Func>
    void meta_msg(const uint8_t curStatusCode, Func f) {
        // Meta message does not affect running status
        const uint8_t* prevBuffer = cursor;

        // Skip status byte and meta type byte
        cursor += 2;
        const auto eventLen = utils::read_variable_length(cursor) + (cursor - prevBuffer);

        if (prevBuffer + eventLen > bufferEnd) [[unlikely]] {
            throw std::ios_base::failure(
                "MiniMidi: Unexpected EOF in Meta Event! Cursor would be "
                + std::to_string(cursor + eventLen - bufferEnd)
                + " bytes beyond the end of buffer with previous event length "
                + std::to_string(eventLen) + "!"
            );
        }
        // The message data does not include the status byte,
        // but the meta type byte is included
        const auto* metaBegin = prevBuffer + 1;
        if (lut::to_meta_type(*metaBegin) == MetaType::EndOfTrack) [[unlikely]] {
            cursor   = bufferEnd;
            foundEOT = true;
            return;
        } else {
            cursor = prevBuffer + eventLen;
        }
        f(tickOffset, *prevBuffer, metaBegin, eventLen - 1);
    }

    template<typename Func>
    void common_msg(const uint8_t curStatusCode, Func f) {
        prevStatusCode = curStatusCode;
        prevEventLen   = lut::get_msg_length(lut::to_msg_type(curStatusCode));

        if (cursor + prevEventLen > bufferEnd) [[unlikely]] {
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
        f(tickOffset, curStatusCode, curCursor + 1, 2);
    }
};

class TrackGenerator {
    const uint8_t* cursor    = nullptr;
    const uint8_t* bufferEnd = nullptr;
    size_t         trackIdx  = 0;
    size_t         trackNum  = 0;

public:
    TrackGenerator() = default;

    TrackGenerator(const uint8_t* cursor, const uint8_t* bufferEnd, const size_t trackNum) :
        cursor(cursor), bufferEnd(bufferEnd), trackNum(trackNum) {};

    size_t parse_chunk_len() {
        while (std::string(reinterpret_cast<const char*>(cursor), 4) != MTRK) {
            const size_t tmpLen = utils::read_msb_bytes(cursor + 4, 4);
            if (cursor + tmpLen + 8 > bufferEnd) [[unlikely]] {
                throw std::ios_base::failure(
                    "MiniMidi: Unexpected EOF in file! Cursor is "
                    + std::to_string(cursor + tmpLen + 8 - bufferEnd)
                    + " bytes beyond the end of buffer with chunk length " + std::to_string(tmpLen)
                    + "!"
                );
            }
            cursor += (8 + tmpLen);
        }
        return utils::read_msb_bytes(cursor + 4, 4);
    }

    [[nodiscard]] bool done() const { return cursor >= bufferEnd || trackIdx >= trackNum; }

    template<typename T>
    TrackView<T> next() {
        const auto chunkLen = parse_chunk_len();
        const auto view     = TrackView<T>(cursor + 8, chunkLen);
        cursor += (8 + chunkLen);
        ++trackIdx;
        return view;
    }
};
}   // namespace details

template<typename T>
class TrackView<T>::iterator : details::MessageGenerator {
    Message<T> msg;
    bool       finish = false;

public:
    iterator() = default;

    iterator(const uint8_t* cursor, const size_t size) : iterator(cursor, cursor + size) {};
    iterator(const uint8_t* begin, const uint8_t* end) : MessageGenerator(begin, end) {
        advance();
    };

    iterator(const iterator& other)     = default;
    iterator(iterator&& other) noexcept = default;

    bool operator==(const iterator&) const { return this->foundEOT; };
    bool operator!=(const iterator&) const { return !this->foundEOT; };

    Message<T>&       operator*() { return msg; };
    const Message<T>& operator*() const { return msg; };

    iterator& operator++() {
        advance();
        return *this;
    };

    void advance() {
        if (!this->done()) [[likely]] {
            this->emplace_using([&](auto... args) {
                std::destroy_at(&msg);
                std::construct_at(&msg, args...);
            });
        } else {
            this->foundEOT = true;
        }
    }
};

template<typename T>
typename TrackView<T>::iterator TrackView<T>::begin() const {
    return {cursor, size};
}

template<typename T>
typename TrackView<T>::iterator TrackView<T>::end() const {
    return {};
}

template<typename T>
Track<T>::Track(const uint8_t* cursor, const size_t size) {
    messages.reserve(size / 3 + 100);
    details::MessageGenerator generator(cursor, size);
    while (!generator.done()) {
        generator.emplace_using([&](auto... args) { messages.emplace_back(args...); });
    }
}

inline MidiHeader::MidiHeader(const uint8_t* data, const size_t size) {
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
    this->m_format          = read_midiformat(utils::read_msb_bytes(cursor + 8, 2));
    this->m_divisionType    = ((*(cursor + 12)) & 0x80) >> 7;
    this->m_ticksPerQuarter = (((*(cursor + 12)) & 0x7F) << 8) + (*(cursor + 13));
}

inline uint16_t MidiHeader::ticks_per_quarter() const {
    if (!m_divisionType) return m_ticksPerQuarter;
    throw std::runtime_error("MiniMidi: Division type is not ticks per quarter!");
}

inline uint16_t MidiHeader::frame_per_second() const {
    if (m_divisionType) return (~(m_negativeSmpte - 1)) & 0x3F;
    ;
    throw std::runtime_error("MiniMidi: Division type is not ticks per frame!");
}

inline uint16_t MidiHeader::ticks_per_frame() const {
    if (m_divisionType) return m_ticksPerFrame;
    throw std::runtime_error("MiniMidi: Division type is not ticks per frame!");
}

inline uint16_t MidiHeader::ticks_per_second() const {
    return ticks_per_frame() * frame_per_second();
}

template<typename T>
class MidiFileView<T>::iterator : details::TrackGenerator {
    std::optional<TrackView<T>> track{};

public:
    iterator() = default;

    iterator(const uint8_t* cursor, const uint8_t* bufferEnd, const size_t trackNum) :
        TrackGenerator(cursor, bufferEnd, trackNum) {
        advance();
    };

    bool operator==(const iterator&) const { return !track.has_value(); };
    bool operator!=(const iterator&) const { return track.has_value(); };

    TrackView<T>&       operator*() { return track.value(); };
    const TrackView<T>& operator*() const { return track.value(); };

    iterator& operator++() {
        advance();
        return *this;
    };

    void advance() {
        if (this->done()) {
            track.reset();
        } else {
            track.emplace(this->next<T>());
        }
    }
};

template<typename T>
MidiFileView<T>::MidiFileView(const uint8_t* data, const size_t size) : MidiHeader(data, size) {
    this->cursor    = data + HEADER_LENGTH;
    this->bufferEnd = data + size;
    this->trackNum  = utils::read_msb_bytes(data + 10, 2);
}

template<typename T>
MidiFile<T>::MidiFile(const uint8_t* data, const size_t size) :
    MidiFile(MidiFileView<T>(data, size)) {}

template<typename T>
MidiFile<T>::MidiFile(const MidiFileView<T>& view) : MidiHeader(view) {
    tracks.reserve(view.track_num());
    for (const auto& trackView : view) {
        // trackView could be converted to Track<T> by explicit constructor
        tracks.emplace_back(trackView);
    }
}

template<typename T>
MidiFile<T> MidiFile<T>::from_file(const std::string& filepath) {
    FILE* filePtr = fopen(filepath.c_str(), "rb");

    if (!filePtr) { throw std::ios_base::failure("MiniMidi: Reading file failed (fopen)!"); }
    fseek(filePtr, 0, SEEK_END);
    const size_t fileLen = ftell(filePtr);

    container::Bytes data(fileLen);
    fseek(filePtr, 0, SEEK_SET);
    fread(data.data(), 1, fileLen, filePtr);
    fclose(filePtr);

    return MidiFile<T>(data.data(), fileLen);
};


// MIDI Dump Implementation
template<typename T>
container::Bytes MidiFile<T>::to_bytes() const {
    return sort().to_bytes_sorted();
}

template<typename T>
container::Bytes MidiFile<T>::to_bytes_sorted() const {
    container::Bytes bytes;
    size_t           approx_size = 32;
    for (const auto& track : tracks) { approx_size += track.message_num() * 5 + 16; }
    bytes.reserve(approx_size);

    // Write MIDI HEAD
    bytes.resize(14);
    std::uninitialized_copy(MTHD.begin(), MTHD.end(), bytes.begin());
    bytes[7] = 0x06;
    bytes[9] = static_cast<uint8_t>(format());
    utils::write_msb_bytes(bytes.data() + 10, tracks.size(), 2);
    utils::write_msb_bytes(bytes.data() + 12, (division_type() << 15 | ticks_per_quarter()), 2);

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

            // clang-format off
            if ((curStatus == 0xFF) || (curStatus == 0xF0) || (curStatus == 0xF7) || (curStatus != prevStatus)) {
                bytes.emplace_back(curStatus);
            }   // clang-format on

            // 3. write msg btyes
            const auto& msg_data = msg.data();
            utils::write_iter(bytes, msg_data.cbegin(), msg_data.cend());
            prevStatus = curStatus;
        }
        // Write EOT
        utils::write_eot(bytes);

        // Write track chunk length after MTRK
        utils::write_msb_bytes(bytes.data() + track_begin + 4, bytes.size() - track_begin - 8, 4);
        // Track Writting Finished
    }
    return bytes;
}

template<typename T>
Track<T> Track<T>::sort() const {
    // copy all the messages if the message is not EOT using std::copy_if
    Track<T> sortedTrack;
    sortedTrack.messages.reserve(messages.size());
    std::copy_if(
        messages.begin(),
        messages.end(),
        std::back_inserter(sortedTrack.messages),
        [](const auto& msg) {
            return msg.type() != MessageType::Meta
                   || msg.template cast<Meta>().meta_type() != MetaType::EndOfTrack;
        }
    );

    // Optimization: Check if messages are already sorted before performing sort
    // This avoids unnecessary sorting overhead when messages are already in order
    if (!std::is_sorted(
            sortedTrack.messages.begin(),
            sortedTrack.messages.end(),
            [](const auto& lhs, const auto& rhs) { return (lhs.time) < (rhs.time); }
        )) {
        // Only sort if not already sorted
        std::stable_sort(
            sortedTrack.messages.begin(),
            sortedTrack.messages.end(),
            [](const auto& lhs, const auto& rhs) { return (lhs.time) < (rhs.time); }
        );
    }
    return sortedTrack;
}

template<typename T>
MidiFile<T> MidiFile<T>::sort() const {
    MidiFile<T> sortedFile;
    sortedFile.m_format          = m_format;
    sortedFile.m_divisionType    = m_divisionType;
    sortedFile.m_ticksPerQuarter = m_ticksPerQuarter;
    sortedFile.tracks.reserve(tracks.size());
    for (const auto& track : tracks) { sortedFile.tracks.emplace_back(track.sort()); }
    return sortedFile;
}

template<typename T>
void MidiFile<T>::write_file(const std::string& filepath) const {
    FILE* filePtr = fopen(filepath.c_str(), "wb");

    if (!filePtr) { throw std::ios_base::failure("MiniMidi: Create file failed (fopen)!"); }
    const container::Bytes midiBytes = this->to_bytes();
    fwrite(midiBytes.data(), 1, midiBytes.size(), filePtr);
    fclose(filePtr);
}


namespace format {
// --- MIDI message to_string implementations ---

template<concepts::ByteContainer T>
std::string to_string(const T& data) {
    // show in hex
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << "{ ";
    for (auto& d : data) ss << std::setw(2) << static_cast<int>(d) << " ";
    ss << "}" << std::dec;
    return ss.str();
};

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
    case MetaType::TrackName: return output + to_string(meta.template cast<TrackName>());
    case MetaType::InstrumentName: return output + to_string(meta.template cast<InstrumentName>());
    case MetaType::TimeSignature: return output + to_string(meta.template cast<TimeSignature>());
    case MetaType::SetTempo: return output + to_string(meta.template cast<SetTempo>());
    case MetaType::KeySignature: return output + to_string(meta.template cast<KeySignature>());
    case MetaType::EndOfTrack: return output + to_string(meta.template cast<EndOfTrack>());
    default: return output + "value=" + to_string(meta.data());
    }
}

// --- General Message to_string function ---

template<typename T>
std::string to_string(const Message<T>& message) {
    std::string output = "time=" + std::to_string(message.time) + " | ";
    switch (message.type()) {
    case MessageType::NoteOn: return output + to_string(message.template cast<NoteOn>());
    case MessageType::NoteOff: return output + to_string(message.template cast<NoteOff>());
    case MessageType::ProgramChange:
        return output + to_string(message.template cast<ProgramChange>());
    case MessageType::ControlChange:
        return output + to_string(message.template cast<ControlChange>());
    case MessageType::Meta: return output + to_string(message.template cast<Meta>());
    default:
        return output + "Status code: " + std::to_string(lut::to_msg_status(message.type()))
               + " length=" + std::to_string(message.data().size());
    }
}

template<typename T>
std::string to_string(const Track<T>& track) {
    std::stringstream out;
    for (int j = 0; j < track.size(); ++j) {
        out << to_string(track.messages[j]) << std::endl;
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
        out << to_string(file.tracks[i]) << std::endl;
    }
    return out.str();
};
}   // namespace format

using namespace format;
}   // namespace minimidi
#endif   // MINIMIDI_HPP
