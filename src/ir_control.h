#pragma once

#include <cstdint>
#include <optional>
namespace factory {
namespace ir {

enum class Code : uint8_t {
    address = 0x00,
    setting = 0x15,

    A = 0x45,
    B = 0x46,
    C = 0x47,
    D = 0x44,
    E = 0x43,
    F = 0x0D,

    up = 0x40,
    down = 0x19,
    left = 0x07,
    right = 0x09,

    num_0 = 0x16,
    num_1 = 0x0C,
    num_2 = 0x18,
    num_3 = 0x5E,
    num_4 = 0x08,
    num_5 = 0x1C,
    num_6 = 0x5A,
    num_7 = 0x42,
    num_8 = 0x52,
    num_9 = 0x4A,
};

enum class SymbolType { null, leader, data_0, data_1 };
enum class Level : uint8_t {
    low = 0,
    high,
};
struct Input {
    Input(Level level, uint32_t duration);
    Level level;
    uint32_t last_state_duration_cnt;
};

class SymbolParser {
    enum class ParseSymbolSate {
        wait_low,
        wait_high,
    };

public:
    SymbolParser();
    void reset();
    std::optional<SymbolType> parse_symbol(Input input);

private:
    void wait_low();
    void wait_high();

private:
    ParseSymbolSate state_;
    uint32_t last_state_duration_cnt_;
};

class FrameParser {
    enum class FrameParserState {
        parsing_leader,
        parsing_address,
        parsing_command,
    };

public:
    using Output = std::optional<uint8_t>;
    FrameParser();
    Output parse(Input input);
    void reset();

private:
    Output init();
    Output parse_leader(Input input);
    Output parse_address(Input input);
    Output parse_command(Input input);
    void switch_state(FrameParserState state, int line);

    SymbolParser symbol_parser_;
    FrameParserState state_;
    int address_bit_cnt_;
    int command_bit_cnt_;
    uint16_t address_;
    uint16_t command_;
};
} // namespace ir

} // namespace factory
