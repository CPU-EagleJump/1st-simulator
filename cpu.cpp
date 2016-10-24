#include <vector>
#include <iostream>

using namespace std;

#include "common.h"

CPU::CPU(uint32_t mem_size, bool is_debug)
{
    pc = 0;
    r[0] = 0;
    mem = vector<uint32_t>(mem_size);
    halted_f = false;
    cycles = 0;

    debug_f = is_debug;
}

CPU::~CPU()
{
}

void CPU::print_state()
{
    cout << cycles << " cycles." << endl << endl;
    cout << "PC = ";
    print_hex(pc);
    cout << endl << endl;
    cout << "GPRs:" << endl;
    for (int i = 0; i < 32; i++) {
        cout << "x";
        print_dec_2(i);
        cout << " = ";
        print_dec_10(r[i]);
        cout << ";";
        if (i % 4 == 3)
            cout << endl;
        else
            cout << " ";
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

    r[rd] = r[rs1] + r[rs2];
    flush_r0();
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

    r[rd] = mem[r[rs] + imm];
    flush_r0();
    inc_pc();
}

void CPU::jalr(uint32_t rd, uint32_t rs, int32_t imm)
{
    if (debug_f)
        cerr << "jalr" << endl;
    cycles++;

    r[rd] = pc + WORD_SIZE;
    flush_r0();
    pc = r[rs] + imm;
}

void CPU::sw(uint32_t rs2, uint32_t rs1, int32_t imm)
{
    if (debug_f)
        cerr << "sw" << endl;
    cycles++;

    mem[r[rs1] + imm] = r[rs2];
    inc_pc();
}

void CPU::beq(uint32_t rs1, uint32_t rs2, int32_t imm)
{
    if (debug_f)
        cerr << "beq" << endl;
    cycles++;

    if (r[rs1] == r[rs2])
        pc += imm;
    else
        inc_pc();
}

void CPU::bne(uint32_t rs1, uint32_t rs2, int32_t imm)
{
    if (debug_f)
        cerr << "bne" << endl;
    cycles++;

    if (r[rs1] != r[rs2])
        pc += imm;
    else
        inc_pc();
}

void CPU::blt(uint32_t rs1, uint32_t rs2, int32_t imm)
{
    if (debug_f)
        cerr << "blt" << endl;
    cycles++;

    if (*(int32_t *)(r + rs1) < *(int32_t *)(r + rs2))
        pc += imm;
    else
        inc_pc();
}

void CPU::bge(uint32_t rs1, uint32_t rs2, int32_t imm)
{
    if (debug_f)
        cerr << "bge" << endl;
    cycles++;

    if (*(int32_t *)(r + rs1) >= *(int32_t *)(r + rs2))
        pc += imm;
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
    pc += imm;
}

void CPU::halt()
{
    if (debug_f)
        cerr << "halt" << endl;
    cycles++;

    halted_f = true;
}

