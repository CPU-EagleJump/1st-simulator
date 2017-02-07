#include <set>
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

uint32_t text_addr_of_lnum(uint32_t lnum)
{
    uint32_t idx = distance(inst_lines.begin(), lower_bound(inst_lines.begin(), inst_lines.end(), lnum));
    if (idx >= inst_lines.size())
        throw out_of_range("text_addr_of_line_number");
    return idx << 2;
}

void print_prompt()
{
    print_line_of_text_addr(cpu->get_pc());
    cerr << "[" << cpu->get_clocks() << " clks] ";
    cerr << "> ";
}

void print_breakpoint(uint32_t bp)
{
    cerr << "(";
    print_hex(bp);
    cerr << ") ";
    print_line_of_text_addr(bp);
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

set<uint32_t> breakpoints;

bool is_breakpoint()
{
    return breakpoints.find(cpu->get_pc()) != breakpoints.end();
}

void add_breakpoint(uint32_t bp)
{
    breakpoints.insert(bp);
}

void delete_breakpoint(uint32_t bp)
{
    breakpoints.erase(bp); // doesn't care result
}

void delete_all_breakpoints()
{
    breakpoints.clear();
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
            int cnt;
            try {
                cnt = stoi(args[0]);
            } catch (...) {
                cerr << "Invalid argument." << endl;
                return true;
            }
            for (int i = 0; i < cnt; i++) {
                if (!step_and_report())
                    return false;
                if (is_breakpoint()) {
                    cerr << "Stop at breakpoint." << endl << endl;
                    break;
                }
            }
            return true;
        }
    }
    else if (cmd[0] == 'c') { // continue
        for (;;) {
            if (!step_and_report())
                return false;
            if (is_breakpoint()) {
                cerr << "Stop at breakpoint." << endl << endl;
                break;
            }
        }
    }
    else if (cmd[0] == 'q') // quit
        return false;
    else if (cmd[0] == 'b') { // breakpoint
        if (args.empty()) {
            add_breakpoint(cpu->get_pc());
            cerr << "Add breakpoint." << endl << endl;
        } else {
            string arg = args[0];
            if (arg == "-s") { // show breakpoint
                if (breakpoints.size() == 0)
                    cerr << "No";
                else
                    cerr << breakpoints.size();
                cerr << " breakpoint(s)." << endl;

                for (uint32_t bp : breakpoints)
                    print_breakpoint(bp);
                cerr << endl;
            } else {
                uint32_t bp;
                if (isdigit(arg[0])) {
                    try {
                        bp = text_addr_of_lnum(stoul(arg));
                    } catch (...) {
                        cerr << "Invalid argument." << endl;
                        return true;
                    }
                } else {
                    try {
                        bp = text_addr_of_lnum(lnum_of_label(arg));
                    } catch (...) {
                        cerr << "Invalid argument." << endl;
                        return true;
                    }
                }
                add_breakpoint(bp);
                cerr << "Add breakpoint at" << endl;
                print_breakpoint(bp);
                cerr << endl;
            }
        }
    }
    else if (cmd[0] == 'd') { // delete breakpoint
        if (args.empty())
            cerr << "Please specify an argument." << endl;
        else {
            string arg = args[0];
            if (arg == "-a") {
                cerr << "Delete all breakpoints." << endl << endl;
                delete_all_breakpoints();
            } else {
                uint32_t bp;
                if (isdigit(arg[0])) {
                    try {
                        bp = text_addr_of_lnum(stoul(arg));
                    } catch (...) {
                        cerr << "Invalid argument." << endl;
                        return true;
                    }
                } else {
                    try {
                        bp = text_addr_of_lnum(lnum_of_label(arg));
                    } catch (...) {
                        cerr << "Invalid argument." << endl;
                        return true;
                    }
                }
                delete_breakpoint(bp);
                cerr << "Delete breakpoint at" << endl;
                print_breakpoint(bp);
                cerr << endl;
            }
        }
    } else if (cmd[0] == 'p') {
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
                    else {
                        float f = cpu->get_f(ri);
                        val = *(uint32_t *)&f;
                    }
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

