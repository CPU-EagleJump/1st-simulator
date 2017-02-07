#include <iostream>
#include <stdexcept>

using namespace std;

#include "common.h"

uint32_t get_word_of_text_addr(uint32_t addr)
{
    if (addr & 0b11)
        throw invalid_argument("get_word_of_text_addr");
    uint32_t idx = addr >> 2;
    if (!(idx < insts.size()))
        throw out_of_range("get_word_of_text_addr");
    return insts[idx];
}

void print_line_of_text_addr(uint32_t addr)
{
    if (addr & 0b11)
        throw invalid_argument("print_line_of_text_addr");
    uint32_t idx = addr >> 2;
    if (!(idx < insts.size()))
        throw out_of_range("print_line_of_text_addr");
    uint32_t cur_lnum = inst_lines[idx];
    string cur_line = lines[cur_lnum - 1];
    cerr << cur_lnum << ": " << cur_line << endl;
}

void print_prompt()
{
    print_line_of_text_addr(cpu->get_pc());
    cerr << "[" << cpu->get_clocks() << " clks] ";
    cerr << "> ";
}

void print_as_hex(uint32_t n)
{
    cerr << "(hex)   ";
    print_hex(n);
    cerr << endl;
}

void print_as_uint(uint32_t n)
{
    cerr << "(uint)  " << n << endl;
}

void print_as_int(uint32_t n)
{
    cerr << "(int)   " << *(int32_t *)&n << endl;
}

void print_as_float(uint32_t n)
{
    cerr << "(float) " << *(float *)&n << endl;
}

void print_as_bin(uint32_t n)
{
    cerr << "(bin)   " << "0b" << num_to_bin(n) << endl;
}

bool process_command(string cmd_line)
{
    if (cmd_line == "")
        cmd_line = "next";
    vector<string> elems = split_string(cmd_line, " ");
    string cmd = elems[0];
    vector<string> args(elems.begin() + 1, elems.end());

    if (cmd[0] == 'n') { // next
        if (args.empty())
            return step_and_report();
        else {
            int cnt = stoi(args[0]);
            for (int i = 0; i < cnt; i++) {
                if (!step_and_report())
                    return false;
            }
            return true;
        }
    }
    else if (cmd[0] == 'c') { // continue
        exec_continue();
        return false;
    }
    else if (cmd[0] == 'q') // quit
        return false;
    else if (cmd[0] == 'p') {
        if (args.empty())
            cpu->print_state();
        else {
            bool is_deref = false, is_inst = false;
            string var_s;
            if (args[0][0] == '*') {
                is_deref = true;
                var_s = args[0].substr(1);
            } else if (args[0][0] == '@') {
                is_inst = true;
                var_s = args[0].substr(1);
            } else
                var_s = args[0];

            uint32_t val;
            if (var_s == "pc")
                val = cpu->get_pc();
            else if (var_s[0] == 'x' || var_s[0] == 'f') {
                uint32_t ri;
                try {
                    ri = stoul(var_s.substr(1));
                    if (var_s[0] == 'x')
                        val = cpu->get_r(ri);
                    else
                        val = cpu->get_f(ri);
                } catch(...) {
                    cerr << "Invalid argument." << endl;
                    return true;
                }
            } else {
                try {
                    val = stoul(var_s, nullptr, 0);
                } catch(...) {
                    cerr << "Invalid argument." << endl;
                    return true;
                }
            }

            if (is_inst) {
                print_line_of_text_addr(val);
                uint32_t word = get_word_of_text_addr(val);
                print_as_hex(word);
                print_as_bin(word);
                cerr << endl;
            } else {
                if (is_deref) {
                    try {
                        val = cpu->get_mem(val);
                    } catch(...) {
                        cerr << "Invalid memory access. addr = ";
                        print_hex(val);
                        cerr << " (" << val << ")" << endl;
                        return true;
                    }
                }
                print_as_hex(val);
                print_as_uint(val);
                print_as_int(val);
                print_as_float(val);
                print_as_bin(val);
                cerr << endl;
            }
        }
    }
    else
        cerr << "Undefined command." << endl;

    return true;
}

