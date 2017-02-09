#include <cmath>
#include <cfenv>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>

using namespace std;

#include "common.h"

CPU::CPU(uint32_t mem_size, vector<uint32_t> static_data, bool is_debug)
{
    pc = 0;
    prev_pc = 0;
    r[0] = 0;
    for (int i = 0; i < REG_LEN; i++) {
        r[i] = 0;
        f[i] = 0;
    }
    mem = vector<uint32_t>(mem_size);
    this->mem_size = mem_size;
    copy(static_data.begin(), static_data.end(), mem.begin());
    halted_f = false;
    exception_f = false;
    clocks = 0;

    debug_f = is_debug;
}

CPU::~CPU()
{
}

uint32_t CPU::get_r(uint32_t ri)
{
    if (!(ri < CPU::REG_LEN))
        throw out_of_range("CPU::get_r");
    return r[ri];
}

float CPU::get_f(uint32_t ri)
{
    if (!(ri < CPU::REG_LEN))
        throw out_of_range("CPU::get_f");
    return f[ri];
}

uint32_t CPU::get_mem(uint32_t addr)
{
    if (addr & 0b11)
        throw invalid_argument("CPU::get_mem");
    uint32_t idx = addr >> 2;
    if (!(idx < mem_size))
        throw out_of_range("CPU::get_mem");
    return mem[idx];
}

void CPU::update_pc(uint32_t new_pc)
{
    prev_pc = pc;
    pc = new_pc;
}

void CPU::report_NaN_exception(uint32_t rd)
{
    print_line_of_text_addr(pc);
    cerr << "NaN value appeared at f";
    print_dec_2(rd);
    cerr << ".";
    cerr << endl << endl;
    exception_f = true;
}

void CPU::print_state()
{
    cerr << "Elapsed "<< clocks << " clocks." << endl << endl;
    cerr << "PC = ";
    print_hex(pc);
    cerr << " (" << pc << ")" << endl;

    cerr << "GPRs:" << endl;
    for (int i = 0; i < REG_LEN; i++) {
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

    cerr << "FPRs:" << endl;
    for (int i = 0; i < REG_LEN; i++) {
        cerr << "f";
        print_dec_2(i);
        cerr << " = ";
        cerr << setprecision(5) << scientific << f[i];
        cerr << ";";
        if (i % 4 == 3)
            cerr << endl;
        else
            cerr << " ";
    }
    cerr << endl;
}

void CPU::add(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    if (debug_f)
        cerr << "add" << endl;
    clocks++;

    r[rd] = r[rs1] + r[rs2];
    flush_r0();
    inc_pc();
}

void CPU::sub(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    if (debug_f)
        cerr << "sub" << endl;
    clocks++;

    r[rd] = r[rs1] - r[rs2];
    flush_r0();
    inc_pc();
}

void CPU::or_(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    clocks++;

    r[rd] = r[rs1] | r[rs2];
    flush_r0();
    inc_pc();
}

void CPU::fadd(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    clocks++;

    f[rd] = f[rs1] + f[rs2];
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fsub(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    clocks++;

    f[rd] = f[rs1] - f[rs2];
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fmul(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    clocks++;

    f[rd] = f[rs1] * f[rs2];
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fdiv(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    clocks++;

    f[rd] = f[rs1] / f[rs2];
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fsqrt(uint32_t rd, uint32_t rs1)
{
    clocks++;

    f[rd] = sqrtf(f[rs1]);
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fsgnj(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    clocks++;

    uint32_t res = ((*(uint32_t *)&f[rs1]) & 0x7fffffff) | ((*(uint32_t *)&f[rs2]) & 0x80000000);
    f[rd] = *(float *)&res;
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fsgnjn(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    clocks++;

    uint32_t res = ((*(uint32_t *)&f[rs1]) & 0x7fffffff) | (~(*(uint32_t *)&f[rs2]) & 0x80000000);
    f[rd] = *(float *)&res;
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fsgnjx(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    clocks++;

    uint32_t res = ((*(uint32_t *)&f[rs1]) & 0x7fffffff) | (((*(uint32_t *)&f[rs1]) & 0x80000000) ^ ((*(uint32_t *)&f[rs2]) & 0x80000000));
    f[rd] = *(float *)&res;
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::feq(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    clocks++;

    r[rd] = f[rs1] == f[rs2];
    flush_r0();
    inc_pc();
}

void CPU::fle(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    clocks++;

    r[rd] = f[rs1] <= f[rs2];
    flush_r0();
    inc_pc();
}

void CPU::fcvt_w_s(uint32_t rd, uint32_t rs1)
{
    clocks++;

    fesetround(FE_TONEAREST);
    r[rd] = (uint32_t)nearbyintf(f[rs1]);
    flush_r0();

    inc_pc();
}

void CPU::fcvt_s_w(uint32_t rd, uint32_t rs1)
{
    clocks++;

    f[rd] = (int32_t)r[rs1];
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fmv_s_x(uint32_t rd, uint32_t rs1)
{
    clocks++;

    f[rd] = *(float *)&r[rs1];
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::addi(uint32_t rd, uint32_t rs, int32_t imm)
{
    if (debug_f)
        cerr << "addi" << endl;
    clocks++;

    r[rd] = r[rs] + imm;
    flush_r0();
    inc_pc();
}

void CPU::slli(uint32_t rd, uint32_t rs, uint32_t shamt)
{
    clocks++;

    r[rd] = r[rs] << shamt;
    flush_r0();
    inc_pc();
}

void CPU::srai(uint32_t rd, uint32_t rs, uint32_t shamt)
{
    clocks++;

    int32_t res = (*(int32_t *)&r[rs]) >> shamt;
    r[rd] = *(uint32_t *)&res;
    flush_r0();
    inc_pc();
}

void CPU::lw(uint32_t rd, uint32_t rs, int32_t imm)
{
    if (debug_f)
        cerr << "lw" << endl;
    clocks++;

    uint32_t addr = r[rs] + imm, idx = addr >> 2;
    if (idx < mem_size) {
        r[rd] = mem[idx];
        flush_r0();
        inc_pc();
    } else {
        print_line_of_text_addr(pc);
        cerr << "Invalid memory access. addr = ";
        print_hex(addr);
        cerr << " (" << addr << ")";
        cerr << endl << endl;
        exception_f = true;
    }
}

void CPU::flw(uint32_t rd, uint32_t rs, int32_t imm)
{
    clocks++;

    uint32_t addr = r[rs] + imm, idx = addr >> 2;
    if (idx < mem_size) {
        f[rd] = *(float *)&mem[idx];
        if (isnan(f[rd]))
            report_NaN_exception(rd);
        inc_pc();
    } else {
        print_line_of_text_addr(pc);
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
    clocks++;

    r[rd] = pc + WORD_SIZE;
    flush_r0();
    update_pc(r[rs] + imm);
}

void CPU::sw(uint32_t rs2, uint32_t rs1, int32_t imm)
{
    if (debug_f)
        cerr << "sw" << endl;
    clocks++;

    uint32_t addr = r[rs1] + imm, idx = addr >> 2;
    if (idx < mem_size) {
        mem[idx] = r[rs2];
        inc_pc();
    } else {
        print_line_of_text_addr(pc);
        cerr << "Invalid memory access. addr = ";
        print_hex(addr);
        cerr << " (" << addr << ")";
        cerr << endl << endl;
        exception_f = true;
    }
}

void CPU::fsw(uint32_t rs2, uint32_t rs1, int32_t imm)
{
    clocks++;

    uint32_t addr = r[rs1] + imm, idx = addr >> 2;
    if (idx < mem_size) {
        mem[idx] = *(uint32_t *)&f[rs2];
        inc_pc();
    } else {
        print_line_of_text_addr(pc);
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
    clocks++;

    if (r[rs1] == r[rs2])
        update_pc(pc + imm);
    else
        inc_pc();
}

void CPU::bne(uint32_t rs1, uint32_t rs2, int32_t imm)
{
    if (debug_f)
        cerr << "bne" << endl;
    clocks++;

    if (r[rs1] != r[rs2])
        update_pc(pc + imm);
    else
        inc_pc();
}

void CPU::blt(uint32_t rs1, uint32_t rs2, int32_t imm)
{
    if (debug_f)
        cerr << "blt" << endl;
    clocks++;

    if (*(int32_t *)(r + rs1) < *(int32_t *)(r + rs2))
        update_pc(pc + imm);
    else
        inc_pc();
}

void CPU::bge(uint32_t rs1, uint32_t rs2, int32_t imm)
{
    if (debug_f)
        cerr << "bge" << endl;
    clocks++;

    if (*(int32_t *)(r + rs1) >= *(int32_t *)(r + rs2))
        update_pc(pc + imm);
    else
        inc_pc();
}

void CPU::lui(uint32_t rd, uint32_t imm_u)
{
    clocks++;

    r[rd] = imm_u | (r[rd] & 0x00000fff); // preserve the lowest 12 bits
    flush_r0();

    inc_pc();
}

void CPU::jal(uint32_t rd, int32_t imm)
{
    if (debug_f)
        cerr << "jal" << endl;
    clocks++;

    r[rd] = pc + WORD_SIZE;
    flush_r0();
    update_pc(pc + imm);
}

void CPU::halt()
{
    if (debug_f)
        cerr << "halt" << endl;
    clocks++;

    halted_f = true;
}

void CPU::inb(uint32_t rd)
{
    clocks++;

    char c;
    in_file.get(c);
    r[rd] = *(unsigned char *)&c; // clears upper 24 bits
    flush_r0();

    inc_pc();
}

void CPU::outb(uint32_t rs2)
{
    clocks++;

    out_file.put((char)r[rs2]);

    inc_pc();
}

