#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <iostream>

using namespace std;

#include "common.h"

const uint32_t WORD_SIZE = 4;


int main(int argc, char **argv)
{
    if (argc < 2) {
        report_error("no input file");
        exit(1);
    }

    string zoi_name = argv[1];
    if (zoi_name.size() < 4 || zoi_name.substr(zoi_name.size() - 4) != ".zoi") {
        report_error("invalid file type");
        exit(1);
    }

    ifstream zoi_file;
    zoi_file.open(zoi_name, ios::in | ios::binary);
    if (zoi_file.fail()) {
        report_error("no such file");
        exit(1);
    }

    vector<uint32_t> words;
    while (!zoi_file.eof()) {
        uint8_t bytes[WORD_SIZE];
        zoi_file.read(reinterpret_cast<char *>(bytes), WORD_SIZE);
        uint32_t w = (uint32_t)bytes[0] | ((uint32_t)bytes[1]) << 8 | ((uint32_t)bytes[2]) << 16 | ((uint32_t)bytes[3]) << 24;
        words.push_back(w);
    }

    CPU *cpu = new CPU(0x100000, false);
    while (!cpu->is_halted())
        step_exec(words[cpu->get_pc() >> 2], cpu);

    cpu->print_state();

    return 0;
}

