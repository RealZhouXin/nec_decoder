#include "ir_control.h"
#include "fmt/color.h"
#include "fmt/core.h"
#include "fmt/format.h"
#include <cstdint>
#include <optional>
#include <utility>

using namespace factory::ir;
constexpr uint32_t T = 560;                          // 脉冲周期
constexpr uint32_t LEADER_HIGH_LEVEL_PULSE_CNT = 16; // 引导码高电平脉冲个数
constexpr uint32_t LEADER_LOW_LEVEL_PULSE_CNT = 4; // 引导码低电平脉冲个数
constexpr uint32_t DATA_0_HIGH_LEVEL_PULSE_CNT = 1; // 数据0高电平脉冲个数
constexpr uint32_t DATA_0_LOW_LEVEL_PULSE_CNT = 1; // 数据0低电平脉冲个数
constexpr uint32_t DATA_1_HIGH_LEVEL_PULSE_CNT = 1; // 数据1高电平脉冲个数
constexpr uint32_t DATA_1_LOW_LEVEL_PULSE_CNT = 3; // 数据1低电平脉冲个数

void setbit(auto &data, uint8_t bit) { data |= (1 << bit); }
void clrbit(auto &data, uint8_t bit) { data &= ~(1 << bit); }

// Input
Input::Input(Level level, uint32_t duration)
    : level(level), last_state_duration_cnt(duration / T) {}

// SymbolParser
SymbolParser::SymbolParser() : state_(ParseSymbolSate::init) {}
void SymbolParser::reset() {
    state_ = ParseSymbolSate::init;
    last_state_duration_cnt_ = 0;
}
std::optional<SymbolType> SymbolParser::parse_symbol(Input input) {
    switch (state_) {
    case ParseSymbolSate::init:
        last_state_duration_cnt_ = 0;
        if (input.level == Level::high) {
            state_ = ParseSymbolSate::high;
        } else {
            fmt::print(fmt::fg(fmt::color::yellow),
                       "parse symbol input {}({}), line:{}\n", (int)input.level,
                       input.last_state_duration_cnt, __LINE__);
            return SymbolType::null;
        }
        break;
    case ParseSymbolSate::low:
        if (input.level == Level::high) {
            state_ = ParseSymbolSate::high;
            switch (input.last_state_duration_cnt) {
            case LEADER_LOW_LEVEL_PULSE_CNT:
                if (last_state_duration_cnt_ == LEADER_HIGH_LEVEL_PULSE_CNT)
                    return SymbolType::leader;
                break;
            case DATA_0_LOW_LEVEL_PULSE_CNT:
                if (last_state_duration_cnt_ == DATA_0_HIGH_LEVEL_PULSE_CNT)
                    return SymbolType::data_0;
                break;
            case DATA_1_LOW_LEVEL_PULSE_CNT:
                if (last_state_duration_cnt_ == DATA_1_HIGH_LEVEL_PULSE_CNT)
                    return SymbolType::data_1;
                break;
            default:
                break;
            }
        }
        state_ = ParseSymbolSate::init;
        return SymbolType::null;
        break;
    case ParseSymbolSate::high:
        if (input.level == Level::low &&
            (input.last_state_duration_cnt == LEADER_HIGH_LEVEL_PULSE_CNT ||
             input.last_state_duration_cnt == DATA_0_HIGH_LEVEL_PULSE_CNT ||
             input.last_state_duration_cnt == DATA_1_HIGH_LEVEL_PULSE_CNT)) {
            state_ = ParseSymbolSate::low;
            last_state_duration_cnt_ = input.last_state_duration_cnt;
        } else {
            state_ = ParseSymbolSate::init;
            fmt::print(fmt::fg(fmt::color::yellow),
                       "parse symbol input {}({}), line:{}\n", (int)input.level,
                       input.last_state_duration_cnt, __LINE__);
            return SymbolType::null;
        }
        break;
    default:
        break;
    }
    return std::nullopt;
}
void SymbolParser::wait_low() {}
// FrameParser
FrameParser::FrameParser()
    : state_(FrameParserState::parsing_leader), address_(0), command_(0) {}

FrameParser::Output FrameParser::parse(Input input) {
    switch (state_) {
    case FrameParserState::parsing_leader:
        return parse_leader(input);
        break;
    case FrameParserState::parsing_address:
        return parse_address(input);
        break;
    case FrameParserState::parsing_command:
        return parse_command(input);
        break;
    default:
        break;
    }
    return std::nullopt;
}

void FrameParser::switch_state(FrameParserState state, int line) {
    fmt::print(fmt::fg(fmt::color::red), "{}->{}, line:{}\n", (int)state_,
               (int)state, line);
    switch (state) {
    case FrameParserState::parsing_leader:
        init();
        break;
    case FrameParserState::parsing_address:
        address_ = 0;
        address_bit_cnt_ = 0;
        break;
    case FrameParserState::parsing_command:
        command_ = 0;
        command_bit_cnt_ = 0;
        break;
    default:
        break;
    }
    state_ = state;
}
FrameParser::Output FrameParser::init() {
    symbol_parser_.reset();
    return std::nullopt;
}

FrameParser::Output FrameParser::parse_leader(Input input) {
    auto symbol = symbol_parser_.parse_symbol(input);
    if (symbol.has_value()) {
        if (symbol.value() == SymbolType::leader) {
            switch_state(FrameParserState::parsing_address, __LINE__);
            return std::nullopt;
        } else {
            switch_state(FrameParserState::parsing_leader, __LINE__);
            fmt::print(fmt::fg(fmt::color::red), "input: {}({})\n",
                       (int)input.level, (int)input.last_state_duration_cnt);
            return std::nullopt;
        }
    }
    return std::nullopt;
}

FrameParser::Output FrameParser::parse_address(Input input) {
    auto symbol = symbol_parser_.parse_symbol(input);
    if (!symbol.has_value())
        return std::nullopt;
    if (symbol.value() == SymbolType::data_0) {
        clrbit(address_, address_bit_cnt_);
        address_bit_cnt_++;
    } else if (symbol.value() == SymbolType::data_1) {
        setbit(address_, address_bit_cnt_);
        address_bit_cnt_++;
    } else {
        switch_state(FrameParserState::parsing_leader, __LINE__);
        return std::nullopt;
    }
    if (address_bit_cnt_ == 16) {
        // 取低8位
        auto [low_bytes, high_bytes] =
            std::pair<uint8_t, uint8_t>(address_ & 0xff, address_ >> 8);
        // 取高8位
        if (low_bytes != 0x00 || high_bytes != 0xff) {
            fmt::print("address error: low_bytes:{}, high_bytes:{}\n",
                       low_bytes, high_bytes);
            switch_state(FrameParserState::parsing_leader, __LINE__);
        }
        switch_state(FrameParserState::parsing_command, __LINE__);
    }
    return std::nullopt;
}

FrameParser::Output FrameParser::parse_command(Input input) {
    auto symbol = symbol_parser_.parse_symbol(input);
    if (!symbol.has_value())
        return std::nullopt;
    if (symbol.value() == SymbolType::data_0) {
        clrbit(command_, command_bit_cnt_);
        command_bit_cnt_++;
    } else if (symbol.value() == SymbolType::data_1) {
        setbit(command_, command_bit_cnt_);
        command_bit_cnt_++;
    } else {
        switch_state(FrameParserState::parsing_leader, __LINE__);
        return std::nullopt;
    }
    if (command_bit_cnt_ == 16) {
        // 取低8位// 取高8位
        auto [low_bytes, high_bytes] =
            std::pair<uint8_t, uint8_t>(command_ & 0xff, command_ >> 8);

        if (low_bytes != ~high_bytes) {
            fmt::print(fmt::fg(fmt::color::red),
                       "command error: low_bytes:{}, high_bytes:{}\n",
                       low_bytes, high_bytes);
            switch_state(FrameParserState::parsing_leader, __LINE__);
            return std::nullopt;
        }
        // 低8位为命令
        switch_state(FrameParserState::parsing_leader, __LINE__);
        return Output{low_bytes};
    }
    return std::nullopt;
}

// IrProcessor
