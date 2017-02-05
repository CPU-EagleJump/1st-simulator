#include <vector>
#include <fstream>
#include <iostream>

using namespace std;

#include "common.h"

CPU::CPU(uint32_t mem_size, vector<uint32_t> static_data, bool is_debug)
{
    pc = 0;
    prev_pc = 0;
    r[0] = 0;
    for (int i = 0; i < 32; i++) {
        r[i] = 0;
        f[i] = 0;
    }
    mem = vector<uint32_t>(mem_size);
    this->mem_size = mem_size;
    copy(static_data.begin(), static_data.end(), mem.begin());
    halted_f = false;
    exception_f = false;
    cycles = 0;

    debug_f = is_debug;
}

CPU::~CPU()
{
}

void CPU::update_pc(uint32_t new_pc)
{
    prev_pc = pc;
    pc = new_pc;
}

void CPU::print_state()
{
    cerr << cycles << " cycles." << endl << endl;
    cerr << "PC = ";
    print_hex(pc);
    cerr << " (" << pc << ")";
    cerr << endl << endl;
    cerr << "GPRs:" << endl;
    for (int i = 0; i < 32; i++) {
        cerr << "x";
        print_dec_2(i);
        cerr << " = ";
        print_dec_10(r[i]);
        cerr << ";";
        if (i % 4 == 3)
            cerr << endl;
        else
            cerr << " ";
    }
}

void CPU::add(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    if (debug_f)
        cerr << "add" << endl;
    cycles++;

    r[rd] = r[rs1] + r[rs2];
    flush_r0();
    inc_pc();
}

void CPU::sub(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    if (debug_f)
        cerr << "sub" << endl;
    cycles++;

    r[rd] = r[rs1] - r[rs2];
    flush_r0();
    inc_pc();
}

void CPU::fadd(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    cycles++;

    f[rd] = f[rs1] + f[rs2];
    inc_pc();
}

void CPU::fsub(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    cycles++;

    f[rd] = f[rs1] - f[rs2];
    inc_pc();
}

void CPU::fmul(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    cycles++;

    f[rd] = f[rs1] * f[rs2];
    inc_pc();
}

void CPU::fdiv(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    cycles++;

    f[rd] = f[rs1] / f[rs2];
    inc_pc();
}

void CPU::fsgnj(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    cycles++;

    uint32_t res = ((*(uint32_t *)&f[rs1]) & 0x7fffffff) | ((*(uint32_t *)&f[rs2]) & 0x80000000);
    f[rd] = *(float *)&res;

    inc_pc();
}

void CPU::fsgnjn(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    cycles++;

    uint32_t res = ((*(uint32_t *)&f[rs1]) & 0x7fffffff) | (~(*(uint32_t *)&f[rs2]) & 0x80000000);
    f[rd] = *(float *)&res;

    inc_pc();
}

void CPU::fsgnjx(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    cycles++;

    uint32_t res = ((*(uint32_t *)&f[rs1]) & 0x7fffffff) ^ ((*(uint32_t *)&f[rs2]) & 0x80000000);
    f[rd] = *(float *)&res;

    inc_pc();
}

void CPU::fle(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    cycles++;

    r[rd] = f[rs1] <= f[rs2];
    inc_pc();
}

void CPU::fcvt_s_w(uint32_t rd, uint32_t rs1)
{
    cycles++;

    f[rd] = r[rs1];
    inc_pc();
}

void CPU::fmv_s_x(uint32_t rd, uint32_t rs1)
{
    cycles++;

    f[rd] = *(float *)&r[rs1];

    inc_pc();
}

void CPU::addi(uint32_t rd, uint32_t rs, int32_t imm)
{
    if (debug_f)
        cerr << "addi" << endl;
    cycles++;

    r[rd] = r[rs] + imm;
    flush_r0();
    inc_pc();
}

void CPU::lw(uint32_t rd, uint32_t rs, int32_t imm)
{
    if (debug_f)
        cerr << "lw" << endl;
    cycles++;

    uint32_t addr = r[rs] + imm, idx = addr >> 2;
    if (idx < mem_size) {
        r[rd] = mem[idx];
        flush_r0();
        inc_pc();
    } else {
        print_line_of_pc(pc);
        cerr << "Invalid memory access. addr = ";
        print_hex(addr);
        cerr << " (" << addr << ")";
        cerr << endl << endl;
        exception_f = true;
    }
}

void CPU::flw(uint32_t rd, uint32_t rs, int32_t imm)
{
    cycles++;

    uint32_t addr = r[rs] + imm, idx = addr >> 2;
    if (idx < mem_size) {
        f[rd] = *(float *)&mem[idx];
        inc_pc();
    } else {
        print_line_of_pc(pc);
        cerr << "Invalid memory access. addr = ";
        print_hex(addr);
        cerr << " (" << addr << ")";
        cerr << endl << endl;
        exception_f = true;
    }
}

void CPU::jalr(uint32_t rd, uint32_t rs, int32_t imm)
{
    if (debug_f)
        cerr << "jalr" << endl;
    cycles++;

    r[rd] = pc + WORD_SIZE;
    flush_r0();
    update_pc(r[rs] + imm);
}

void CPU::sw(uint32_t rs2, uint32_t rs1, int32_t imm)
{
    if (debug_f)
        cerr << "sw" << endl;
    cycles++;

    uint32_t addr = r[rs1] + imm, idx = addr >> 2;
    if (idx < mem_size) {
        mem[idx] = r[rs2];
        inc_pc();
    } else {
        print_line_of_pc(pc);
        cerr << "Invalid memory access. addr = ";
        print_hex(addr);
        cerr << " (" << addr << ")";
        cerr << endl << endl;
        exception_f = true;
    }
}

void CPU::fsw(uint32_t rs2, uint32_t rs1, int32_t imm)
{
    cycles++;

    uint32_t addr = r[rs1] + imm, idx = addr >> 2;
    if (idx < mem_size) {
        mem[idx] = *(uint32_t *)&f[rs2];
        inc_pc();
    } else {
        print_line_of_pc(pc);
        cerr << "Invalid memory access. addr = ";
        print_hex(addr);
        cerr << " (" << addr << ")";
        cerr << endl << endl;
        exception_f = true;
    }
}

void CPU::beq(uint32_t rs1, uint32_t rs2, int32_t imm)
{
    if (debug_f)
        cerr << "beq" << endl;
    cycles++;

    if (r[rs1] == r[rs2])
        update_pc(pc + imm);
    else
        inc_pc();
}

void CPU::bne(uint32_t rs1, uint32_t rs2, int32_t imm)
{
    if (debug_f)
        cerr << "bne" << endl;
    cycles++;

    if (r[rs1] != r[rs2])
        update_pc(pc + imm);
    else
        inc_pc();
}

void CPU::blt(uint32_t rs1, uint32_t rs2, int32_t imm)
{
    if (debug_f)
        cerr << "blt" << endl;
    cycles++;

    if (*(int32_t *)(r + rs1) < *(int32_t *)(r + rs2))
        update_pc(pc + imm);
    else
        inc_pc();
}

void CPU::bge(uint32_t rs1, uint32_t rs2, int32_t imm)
{
    if (debug_f)
        cerr << "bge" << endl;
    cycles++;

    if (*(int32_t *)(r + rs1) >= *(int32_t *)(r + rs2))
        update_pc(pc + imm);
    else
        inc_pc();
}

void CPU::jal(uint32_t rd, int32_t imm)
{
    if (debug_f)
        cerr << "jal" << endl;
    cycles++;

    r[rd] = pc + WORD_SIZE;
    flush_r0();
    update_pc(pc + imm);
}

void CPU::halt()
{
    if (debug_f)
        cerr << "halt" << endl;
    cycles++;

    halted_f = true;
}

void CPU::inb(uint32_t rd)
{
    cycles++;

    char c;
    in_file.get(c);
    r[rd] = c; // clears upper 24 bits

    inc_pc();
}

void CPU::outb(uint32_t rs2)
{
    cycles++;

    out_file.put((char)r[rs2]);

    inc_pc();
}

