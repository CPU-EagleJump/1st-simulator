#ifndef _COMMON_H_
#define _COMMON_H_

#include <vector>
#include <string>
#include <cstdint>

using namespace std;
extern const uint32_t WORD_SIZE;

// debugger.cpp
void print_prompt();
void print_line_of_text_addr(uint32_t addr);
bool process_command(string cmd_line);

// cpu.cpp
class CPU
{
public:
    CPU(uint32_t mem_size, vector<uint32_t> static_data, bool is_debug);
    ~CPU();

    uint32_t get_pc() { return pc; }
    uint32_t get_prev_pc() { return prev_pc; }
    uint32_t get_r(uint32_t ri);
    float get_f(uint32_t ri);
    uint32_t get_mem(uint32_t addr);
    uint32_t get_clocks() { return clocks; }
    bool is_halted() { return halted_f; }
    bool is_exception() { return exception_f; }

    void print_state();
    void report_NaN_exception(uint32_t rd);

    // R type
    void add(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void sub(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void or_(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fadd(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fsub(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fmul(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fdiv(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fsgnj(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fsgnjn(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fsgnjx(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void feq(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fle(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fcvt_s_w(uint32_t rd, uint32_t rs1);
    void fmv_s_x(uint32_t rd, uint32_t rs1);
    // I type
    void addi(uint32_t rd, uint32_t rs, int32_t imm);
    void slli(uint32_t rd, uint32_t rs, uint32_t shamt);
    void srai(uint32_t rd, uint32_t rs, uint32_t shamt);
    void lw(uint32_t rd, uint32_t rs, int32_t imm);
    void flw(uint32_t rd, uint32_t rs, int32_t imm);
    void jalr(uint32_t rd, uint32_t rs, int32_t imm);
    // S type
    void sw(uint32_t rs2, uint32_t rs1, int32_t imm);
    void fsw(uint32_t rs2, uint32_t rs1, int32_t imm);
    // SB type
    void beq(uint32_t rs1, uint32_t rs2, int32_t imm);
    void bne(uint32_t rs1, uint32_t rs2, int32_t imm);
    void blt(uint32_t rs1, uint32_t rs2, int32_t imm);
    void bge(uint32_t rs1, uint32_t rs2, int32_t imm);
    // U type
    void lui(uint32_t rd, uint32_t imm_u);
    // UJ type
    void jal(uint32_t rd, int32_t imm);
    // original
    void halt();
    void inb(uint32_t rd);
    void outb(uint32_t rs2);

private:
    static const uint32_t REG_LEN = 32;
    uint32_t pc, prev_pc, r[REG_LEN];
    float f[REG_LEN];
    vector<uint32_t> mem;
    uint32_t mem_size;
    bool halted_f, exception_f;
    uint32_t clocks;

    bool debug_f;

    void update_pc(uint32_t new_pc);
    void inc_pc() { update_pc(pc + WORD_SIZE); }
    void flush_r0() { r[0] = 0; }

};

// exec.cpp
bool step_exec(CPU *cpu, const vector<uint32_t> &insts);

// util.cpp
vector<string> split_string(const string &str, const string &delims);
string num_to_bin(uint32_t num, int len = 32);
void print_hex(uint32_t n);
void print_dec_2(uint32_t n);
void print_dec_10(uint32_t n);

// report.cpp
void report_error(string message);
void report_warning(string message);

// main.cpp
extern ifstream in_file;
extern ofstream out_file;
extern vector<uint32_t> insts, inst_lines;
extern vector<string> lines;
extern CPU *cpu;
bool step_and_report();
void exec_continue();

#endif

