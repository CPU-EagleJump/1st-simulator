#include <cmath>
#include <cfenv>
#include <vector>
#include <utility>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>

using namespace std;

#include "common.h"

string inst_type_to_string(InstType t)
{
    switch (t) {
        case InstType::add:
            return "add";
        case InstType::sub:
            return "sub";
        case InstType::or_:
            return "or";
        case InstType::fadd:
            return "fadd.s";
        case InstType::fsub:
            return "fsub.s";
        case InstType::fmul:
            return "fmul.s";
        case InstType::fsqrt:
            return "fsqrt.s";
        case InstType::fdiv:
            return "fdiv.s";
        case InstType::fsgnj:
            return "fsgnj.s";
        case InstType::fsgnjn:
            return "fsgnjn.s";
        case InstType::fsgnjx:
            return "fsgnjx.s";
        case InstType::feq:
            return "feq.s";
        case InstType::fle:
            return "fle.s";
        case InstType::fcvt_w_s:
            return "fcvt.w.s";
        case InstType::fcvt_s_w:
            return "fcvt.s.w";
        case InstType::fmv_s_x:
            return "fmv.s.x";
        case InstType::addi:
            return "addi";
        case InstType::slli:
            return "slli";
        case InstType::srai:
            return "srai";
        case InstType::lw:
            return "lw";
        case InstType::flw:
            return "flw";
        case InstType::jalr:
            return "jalr";
        case InstType::sw:
            return "sw";
        case InstType::fsw:
            return "fsw";
        case InstType::beq:
            return "beq";
        case InstType::bne:
            return "bne";
        case InstType::blt:
            return "blt";
        case InstType::bge:
            return "bge";
        case InstType::lui:
            return "lui";
        case InstType::jal:
            return "jal";
        case InstType::halt:
            return "halt";
        case InstType::inb:
            return "inb";
        case InstType::outb:
            return "outb";
        default: // not reached
            return "";
    }
}

CPU::CPU(uint32_t mem_size, vector<uint32_t> static_data)
{
    pc = 0;
    prev_pc = 0;
    for (int i = 0; i < REG_LEN; i++) {
        r[i] = 0;
        r_max[i] = 0;
        f[i] = 0;
    }
    mem = vector<uint32_t>(mem_size);
    this->mem_size = mem_size;
    copy(static_data.begin(), static_data.end(), mem.begin());
    halted_f = false;
    exception_f = false;
    clocks = 0;
    for (int i = 0; i < INST_LEN; i++) {
        inst_stat[i] = 0;
    }
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
    cerr << endl << "[CPU State]" << endl;
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
}

void CPU::print_inst_stat(bool is_sort)
{
    cerr << endl << "[Instruction statistics]" << endl;

    vector<pair<uint64_t, InstType>> stat;

    for (int i = 0; i < INST_LEN; i++) {
        stat.push_back(make_pair(inst_stat[i], static_cast<InstType>(i)));
    }
    if (is_sort)
        sort(stat.begin(), stat.end(), greater<pair<uint64_t, InstType>>());

    for (auto p : stat) {
        cerr << setw(10) << inst_type_to_string(p.second) + ": " << p.first << endl;
    }
}

void CPU::print_max()
{
    cerr << endl << "[Register max values]" << endl;
    cerr << "GPRs" << endl;
    for (int i = 0; i < REG_LEN; i++) {
        cerr << "x";
        print_dec_2(i);
        cerr << " = ";
        print_dec_10(r_max[i]);
        cerr << ";";
        if (i % 4 == 3)
            cerr << endl;
        else
            cerr << " ";
    }
}

void CPU::update_max()
{
    for (int i = 0; i < REG_LEN; i++) {
        r_max[i] = max(r_max[i], r[i]);
    }
}

void CPU::add(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    inst_stat[static_cast<int>(InstType::add)]++;

    r[rd] = r[rs1] + r[rs2];
    flush_r0();
    inc_pc();
}

void CPU::sub(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    inst_stat[static_cast<int>(InstType::sub)]++;

    r[rd] = r[rs1] - r[rs2];
    flush_r0();
    inc_pc();
}

void CPU::or_(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    inst_stat[static_cast<int>(InstType::or_)]++;

    r[rd] = r[rs1] | r[rs2];
    flush_r0();
    inc_pc();
}

void CPU::fadd(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    inst_stat[static_cast<int>(InstType::fadd)]++;

    f[rd] = f[rs1] + f[rs2];
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fsub(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    inst_stat[static_cast<int>(InstType::fsub)]++;

    f[rd] = f[rs1] - f[rs2];
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fmul(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    inst_stat[static_cast<int>(InstType::fmul)]++;

    f[rd] = f[rs1] * f[rs2];
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fdiv(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    inst_stat[static_cast<int>(InstType::fdiv)]++;

    f[rd] = f[rs1] / f[rs2];
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fsqrt(uint32_t rd, uint32_t rs1)
{
    inst_stat[static_cast<int>(InstType::fsqrt)]++;

    f[rd] = sqrtf(f[rs1]);
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fsgnj(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    inst_stat[static_cast<int>(InstType::fsgnj)]++;

    uint32_t res = ((*(uint32_t *)&f[rs1]) & 0x7fffffff) | ((*(uint32_t *)&f[rs2]) & 0x80000000);
    f[rd] = *(float *)&res;
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fsgnjn(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    inst_stat[static_cast<int>(InstType::fsgnjn)]++;

    uint32_t res = ((*(uint32_t *)&f[rs1]) & 0x7fffffff) | (~(*(uint32_t *)&f[rs2]) & 0x80000000);
    f[rd] = *(float *)&res;
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fsgnjx(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    inst_stat[static_cast<int>(InstType::fsgnjx)]++;

    uint32_t res = ((*(uint32_t *)&f[rs1]) & 0x7fffffff) | (((*(uint32_t *)&f[rs1]) & 0x80000000) ^ ((*(uint32_t *)&f[rs2]) & 0x80000000));
    f[rd] = *(float *)&res;
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::feq(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    inst_stat[static_cast<int>(InstType::feq)]++;

    r[rd] = f[rs1] == f[rs2];
    flush_r0();
    inc_pc();
}

void CPU::fle(uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    inst_stat[static_cast<int>(InstType::fle)]++;

    r[rd] = f[rs1] <= f[rs2];
    flush_r0();
    inc_pc();
}

void CPU::fcvt_w_s(uint32_t rd, uint32_t rs1)
{
    inst_stat[static_cast<int>(InstType::fcvt_w_s)]++;

    fesetround(FE_TONEAREST);
    r[rd] = (uint32_t)((int32_t)nearbyintf(f[rs1]));
    flush_r0();

    inc_pc();
}

void CPU::fcvt_s_w(uint32_t rd, uint32_t rs1)
{
    inst_stat[static_cast<int>(InstType::fcvt_s_w)]++;

    f[rd] = (int32_t)r[rs1];
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::fmv_s_x(uint32_t rd, uint32_t rs1)
{
    inst_stat[static_cast<int>(InstType::fmv_s_x)]++;

    f[rd] = *(float *)&r[rs1];
    if (isnan(f[rd]))
        report_NaN_exception(rd);

    inc_pc();
}

void CPU::addi(uint32_t rd, uint32_t rs, int32_t imm)
{
    inst_stat[static_cast<int>(InstType::addi)]++;

    r[rd] = r[rs] + imm;
    flush_r0();
    inc_pc();
}

void CPU::slli(uint32_t rd, uint32_t rs, uint32_t shamt)
{
    inst_stat[static_cast<int>(InstType::slli)]++;

    r[rd] = r[rs] << shamt;
    flush_r0();
    inc_pc();
}

void CPU::srai(uint32_t rd, uint32_t rs, uint32_t shamt)
{
    inst_stat[static_cast<int>(InstType::srai)]++;

    int32_t res = (*(int32_t *)&r[rs]) >> shamt;
    r[rd] = *(uint32_t *)&res;
    flush_r0();
    inc_pc();
}

void CPU::lw(uint32_t rd, uint32_t rs, int32_t imm)
{
    inst_stat[static_cast<int>(InstType::lw)]++;

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
    inst_stat[static_cast<int>(InstType::flw)]++;

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
    inst_stat[static_cast<int>(InstType::jalr)]++;

    r[rd] = pc + WORD_SIZE;
    flush_r0();
    update_pc(r[rs] + imm);
}

void CPU::sw(uint32_t rs2, uint32_t rs1, int32_t imm)
{
    inst_stat[static_cast<int>(InstType::sw)]++;

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
    inst_stat[static_cast<int>(InstType::fsw)]++;

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
    inst_stat[static_cast<int>(InstType::beq)]++;

    if (r[rs1] == r[rs2])
        update_pc(pc + imm);
    else
        inc_pc();
}

void CPU::bne(uint32_t rs1, uint32_t rs2, int32_t imm)
{
    inst_stat[static_cast<int>(InstType::bne)]++;

    if (r[rs1] != r[rs2])
        update_pc(pc + imm);
    else
        inc_pc();
}

void CPU::blt(uint32_t rs1, uint32_t rs2, int32_t imm)
{
    inst_stat[static_cast<int>(InstType::blt)]++;

    if (*(int32_t *)(r + rs1) < *(int32_t *)(r + rs2))
        update_pc(pc + imm);
    else
        inc_pc();
}

void CPU::bge(uint32_t rs1, uint32_t rs2, int32_t imm)
{
    inst_stat[static_cast<int>(InstType::bge)]++;

    if (*(int32_t *)(r + rs1) >= *(int32_t *)(r + rs2))
        update_pc(pc + imm);
    else
        inc_pc();
}

void CPU::lui(uint32_t rd, uint32_t imm_u)
{
    inst_stat[static_cast<int>(InstType::lui)]++;

    r[rd] = imm_u | (r[rd] & 0x00000fff); // preserve the lowest 12 bits
    flush_r0();

    inc_pc();
}

void CPU::jal(uint32_t rd, int32_t imm)
{
    inst_stat[static_cast<int>(InstType::jal)]++;

    r[rd] = pc + WORD_SIZE;
    flush_r0();
    update_pc(pc + imm);
}

void CPU::halt()
{
    inst_stat[static_cast<int>(InstType::halt)]++;

    halted_f = true;
}

void CPU::inb(uint32_t rd)
{
    inst_stat[static_cast<int>(InstType::inb)]++;

    char c;
    in_file.get(c);
    r[rd] = *(unsigned char *)&c; // clears upper 24 bits
    flush_r0();

    inc_pc();
}

void CPU::outb(uint32_t rs1)
{
    inst_stat[static_cast<int>(InstType::outb)]++;

    cout << (char)r[rs1];

    inc_pc();
}

