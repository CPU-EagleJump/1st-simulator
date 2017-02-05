#ifndef _COMMON_H_
#define _COMMON_H_

#include <vector>
#include <string>
#include <cstdint>

using namespace std;


// main.cpp
extern const uint32_t WORD_SIZE;
extern ifstream in_file;
extern ofstream out_file;
void print_line_of_pc(uint32_t pc);

// cpu.cpp
class CPU
{
private:
    uint32_t pc, prev_pc, r[32];
    float f[32];
    vector<uint32_t> mem;
    uint32_t mem_size;
    bool halted_f, exception_f;
    uint32_t cycles;

    bool debug_f;

    void update_pc(uint32_t new_pc);
    void inc_pc() { update_pc(pc + WORD_SIZE); }
    void flush_r0() { r[0] = 0; }
    uint32_t load_mem(uint32_t addr);
    void store_mem(uint32_t addr, uint32_t val);

public:
    CPU(uint32_t mem_size, vector<uint32_t> static_data, bool is_debug);
    ~CPU();

    uint32_t get_pc() { return pc; }
    uint32_t get_prev_pc() { return pc; }
    bool is_halted() { return halted_f; }
    bool is_exception() { return exception_f; }

    void print_state();

    // R type
    void add(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void sub(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fadd(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fsub(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fmul(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fdiv(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fsgnj(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fsgnjn(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fsgnjx(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fle(uint32_t rd, uint32_t rs1, uint32_t rs2);
    void fcvt_s_w(uint32_t rd, uint32_t rs1);
    void fmv_s_x(uint32_t rd, uint32_t rs1);
    // I type
    void addi(uint32_t rd, uint32_t rs, int32_t imm);
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
};

// exec.cpp
bool step_exec(CPU *cpu, const vector<uint32_t> &insts);

// util.cpp
vector<string> split_string(const string &str, const string &delims);
void print_hex(uint32_t n);
void print_dec_2(uint32_t n);
void print_dec_10(uint32_t n);

// report.cpp
void report_error(string message);
void report_warning(string message);

#endif

