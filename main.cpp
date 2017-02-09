#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <cstdint>
#include <iostream>

using namespace std;

#include "common.h"

const uint32_t WORD_SIZE = 4;
const uint32_t MEM_SIZE = 0x1000000; // 64 MiB

ifstream zoi_file, in_file;
ofstream out_file;
bool is_debug_mode;

vector<uint32_t> insts, data;
vector<uint32_t> inst_lines;
vector<string> lines, labels;
map<string, uint32_t> label_lnum_map;

vector<bool> is_unreached_index;

CPU *cpu;

uint32_t read_word()
{
    uint8_t bs[WORD_SIZE];
    zoi_file.read(reinterpret_cast<char *>(bs), WORD_SIZE);
    return (uint32_t)bs[0] | ((uint32_t)bs[1]) << 8 | ((uint32_t)bs[2]) << 16 | ((uint32_t)bs[3]) << 24;
}

uint32_t lnum_of_label(string label)
{
    return label_lnum_map.at(label);
}

bool step_and_report()
{
    bool res = step_exec(cpu, insts);
    if (!res) {
        cerr << "Execution interrupted." << endl << endl;
        cpu->print_state();
        return false;
    } else if (cpu->is_halted()) {
        cerr << "Execution finished." << endl << endl;
        cpu->print_state();
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    vector<string> params, options;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-')
            options.push_back(argv[i]);
        else
            params.push_back(argv[i]);
    }

    if (params.size() < 3) {
        if (params.size() == 0)
            report_error("no zoi file");
        else if (params.size() == 1)
            report_error("no input file");
        else if (params.size() == 2)
            report_error("no output file");
        exit(1);
    }

    string zoi_name = params[0];
    if (zoi_name.size() < 4 || zoi_name.substr(zoi_name.size() - 4) != ".zoi") {
        report_error("invalid file type");
        exit(1);
    }

    zoi_file.open(zoi_name, ios::in | ios::binary);
    if (zoi_file.fail()) {
        report_error("no such zoi file");
        exit(1);
    }
    in_file.open(params[1], ios::in | ios::binary);
    if (in_file.fail()) {
        report_error("no such input file");
        zoi_file.close();
        exit(1);
    }
    out_file.open(params[2], ios::out | ios::binary);
    if (out_file.fail()) {
        report_error("no such output file");
        zoi_file.close();
        in_file.close();
        exit(1);
    }

    is_debug_mode = false;
    for (string opt : options)
        if (opt == "-d")
            is_debug_mode = true;

    bool is_debug_file = false;
    char magic[4];
    zoi_file.read(magic, WORD_SIZE);
    if (magic[0] == 'Z' && magic[1] == 'O' && magic[2] == 'I' && (magic[3] == '!' || magic[3] == '?')) {
        if (magic[3] == '?')
            is_debug_file = true;
    } else {
        zoi_file.close();
        report_error("invalid file type");
        exit(1);
    }

    uint32_t data_len = read_word();
    if (data_len > MEM_SIZE) {
        zoi_file.close();
        report_error("static data is too large");
        exit(1);
    }
    uint32_t text_len = read_word();

    data = vector<uint32_t>(data_len);
    for (uint32_t i = 0; i < data_len; i++) {
        data[i] = read_word();
    }

    is_unreached_index = vector<bool>(text_len, true);
    insts = vector<uint32_t>(text_len);
    for (uint32_t i = 0; i < text_len; i++) {
        insts[i] = read_word();
    }

    if (is_debug_file) {
        inst_lines = vector<uint32_t>(text_len); // 1-origin
        for (uint32_t i = 0; i < text_len; i++) {
            inst_lines[i] = read_word();
        }

        string cur_line;
        uint32_t cur_lnum = 0;
        while (!zoi_file.eof()) {
            getline(zoi_file, cur_line);
            cur_lnum++;
            lines.push_back(cur_line);

            auto es = split_string(cur_line, " #\t");
            if (es.size() > 0 && es[0].back() == ':') {
                string label = es[0].substr(0, es[0].size() - 1);
                labels.push_back(label);
                label_lnum_map[label] = cur_lnum;
            }
        }
    }

    zoi_file.close();


    cpu = new CPU(MEM_SIZE, data);

    if (is_debug_mode) {
        if (!is_debug_file) {
            report_error("you must specify binary with debug info when in debug mode");
            exit(1);
        }

        for (;;) {
            print_prompt();

            string cmd;
            getline(cin, cmd);
            bool is_next = process_command(cmd);
            if (!is_next)
                break;
        }
    } else {
        while(step_and_report())
            ;
    }

    delete cpu;

    vector<uint32_t> unreached_addrs;
    for (uint32_t i = 0; i < text_len; i++) {
        if (is_unreached_index[i])
            unreached_addrs.push_back(i << 2);
    }

    if (unreached_addrs.empty())
        cerr << "No";
    else
        cerr << unreached_addrs.size();
    cerr << " unreached lines." << endl << endl;

    for (uint32_t addr : unreached_addrs) {
        print_line_of_text_addr(addr);
    }
    cerr << endl;

    vector<string> unreached_labels;
    for (string label : labels) {
        if (is_unreached_index[text_addr_of_lnum(lnum_of_label(label)) >> 2])
            unreached_labels.push_back(label);
    }

    if (unreached_labels.empty())
        cerr << "No";
    else
        cerr << unreached_labels.size();
    cerr << " unreached labels." << endl << endl;

    for (string label : unreached_labels)
        cerr << label << endl;

    return 0;
}

