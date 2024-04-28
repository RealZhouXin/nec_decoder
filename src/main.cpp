#include "ir_control.h"
#include <fmt/core.h>
#include <iostream>
#include <vector>
using namespace std;
using pii = std::pair<int, int>;
int main(int argc, char **argv) {
    vector<pii> data{
        {0, 16777215}, {0, 777944}, {1, 9159}, {0, 4499}, {1, 579}, {0, 579},
        {1, 579},      {0, 579},    {1, 579},  {0, 579},  {1, 579}, {0, 579},
        {1, 579},      {0, 579},    {1, 579},  {0, 579},  {1, 579}, {0, 579},
        {1, 579},      {0, 579},    {1, 579},  {0, 1610}, {1, 579}, {0, 1610},
        {1, 579},      {0, 1610},   {1, 579},  {0, 1610}, {1, 579}, {0, 1610},
        {1, 579},      {0, 1610},   {1, 579},  {0, 1610}, {1, 579}, {0, 1610},
        {1, 579},      {0, 1610},   {1, 579},  {0, 579},  {1, 579}, {0, 1610},
        {1, 579},      {0, 579},    {1, 579},  {0, 579},  {1, 579}, {0, 579},
        {1, 579},      {0, 1610},   {1, 579},  {0, 579},  {1, 579}, {0, 579},
        {1, 579},      {0, 1610},   {1, 579},  {0, 579},  {1, 579}, {0, 1610},
        {1, 579},      {0, 1610},   {1, 579},  {0, 1610}, {1, 579}, {0, 579},
        {1, 579},      {0, 1610}};
    factory::ir::FrameParser frame_parser;
    for (auto [level, duration] : data) {
        auto ret = frame_parser.parse(
            factory::ir::Input(factory::ir::Level(level), duration));
        if (ret.has_value()) {
            fmt::println("get 0x{:x}", ret.value());
        }
    }
    cout << "hello world!" << endl;
    return 0;
}
