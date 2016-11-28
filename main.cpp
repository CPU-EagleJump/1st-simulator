#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <iostream>

using namespace std;

#include "common.h"

const uint32_t WORD_SIZE = 4;

ifstream zoi_file;
bool is_debug_mode;

vector<uint32_t> insts;
vector<uint32_t> inst_lines;
vector<string> lines;
CPU *cpu;

uint32_t read_word()
{
    uint8_t bs[WORD_SIZE];
    zoi_file.read(reinterpret_cast<char *>(bs), WORD_SIZE);
    return (uint32_t)bs[0] | ((uint32_t)bs[1]) << 8 | ((uint32_t)bs[2]) << 16 | ((uint32_t)bs[3]) << 24;
}

void print_line_of_pc(uint32_t pc)
{
    uint32_t idx = pc >> 2;
    uint32_t cur_lnum = inst_lines[idx];
    string cur_line = lines[cur_lnum - 1];
    cout << cur_lnum << ": " << cur_line << endl;
}

bool step_and_report()
{
    bool res = step_exec(cpu, insts);
    if (!res) {
        cout << "Execution interrupted." << endl << endl;
        cpu->print_state();
        return false;
    } else if (cpu->is_halted()) {
        cout << "Execution finished." << endl << endl;
        cpu->print_state();
        return false;
    }
    return true;
}

void exec_continue()
{
    while(step_and_report())
        ;
}

bool process_command(string cmd_line)
{
    if (cmd_line == "")
        cmd_line = "next";
    vector<string> elems = split_string(cmd_line, " ");
    string cmd = elems[0];
    vector<string> args(elems.begin() + 1, elems.end());

    bool is_next = true;

    if (cmd[0] == 'n')
        is_next = step_and_report();
    else if (cmd[0] == 'c') {
        exec_continue();
        is_next = false;
    }
    else if (cmd[0] == 'q')
        is_next = false;
    else if (cmd[0] == 'p')
        cpu->print_state();
    else
        cout << "Undefined command." << endl;

    return is_next;
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

    if (params.size() == 0) {
        report_error("no input file");
        exit(1);
    }

    string zoi_name = params[0];
    if (zoi_name.size() < 4 || zoi_name.substr(zoi_name.size() - 4) != ".zoi") {
        report_error("invalid file type");
        exit(1);
    }

    zoi_file.open(zoi_name, ios::in | ios::binary);
    if (zoi_file.fail()) {
        report_error("no such file");
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

    uint32_t insts_len = read_word();
    insts = vector<uint32_t>(insts_len);
    for (uint32_t i = 0; i < insts_len; i++) {
        insts[i] = read_word();
    }

    if (is_debug_file) {
        inst_lines = vector<uint32_t>(insts_len); // 1-origin
        for (uint32_t i = 0; i < insts_len; i++) {
            inst_lines[i] = read_word();
        }

        string cur_line;
        while (!zoi_file.eof()) {
            getline(zoi_file, cur_line);
            lines.push_back(cur_line);
        }
    }

    zoi_file.close();


    cpu = new CPU(0x100000, false);

    if (is_debug_mode) {
        if (!is_debug_file) {
            zoi_file.close();
            report_error("you must specify binary with debug info when in debug mode");
            exit(1);
        }

        for (;;) {
            print_line_of_pc(cpu->get_pc());
            cout << "> ";
            string cmd;
            getline(cin, cmd);
            bool is_next = process_command(cmd);
            if (!is_next)
                break;
        }
    } else
        exec_continue();

    delete cpu;

    return 0;
}

