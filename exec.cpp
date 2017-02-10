#include <cstdint>
#include <iostream>

#include "common.h"

bool step_exec(CPU *cpu, const vector<uint32_t> &insts)
{
    uint32_t cur_addr = cpu->get_pc();
    uint32_t idx = cur_addr >> 2;
    if (idx >= insts.size()) {
        print_line_of_text_addr(cpu->get_prev_pc());
        cerr << "PC is out of range." << endl << endl;
        return false;
    }
    is_unreached_index[idx] = false;

    uint32_t word = insts[idx];

    uint32_t opcode, rd, funct3, funct7, rs1, rs2;
    opcode = word & 0b1111111;
    rd = (word >> 7) & 0b11111;
    funct3 = (word >> 12) & 0b111;
    funct7 = word >> 25;
    rs1 = (word >> 15) & 0b11111;
    rs2 = (word >> 20) & 0b11111;

    bool is_invalid = false;

    if (opcode == 0b0110011) { // R type
        if (funct3 == 0b000) {
            if (funct7 == 0b0000000)
                cpu->add(rd, rs1, rs2);
            else if (funct7 == 0b0100000)
                cpu->sub(rd, rs1, rs2);
            else
                is_invalid = true;
        }
        else if (funct3 == 0b110 && funct7 == 0b0000000)
            cpu->or_(rd, rs1, rs2);
        else
            is_invalid = true;
    } else if (opcode == 0b1010011) { // RV32F, R type
        if (funct7 == 0b0000000 && funct3 == 0b000)
            cpu->fadd(rd, rs1, rs2);
        else if (funct7 == 0b0000100 && funct3 == 0b000)
            cpu->fsub(rd, rs1, rs2);
        else if (funct7 == 0b0001000 && funct3 == 0b000)
            cpu->fmul(rd, rs1, rs2);
        else if (funct7 == 0b0001100 && funct3 == 0b000)
            cpu->fdiv(rd, rs1, rs2);
        else if (funct7 == 0b0101100 && funct3 == 0b000 && rs2 == 0b00000)
            cpu->fsqrt(rd, rs1);
        else if (funct7 == 0b0010000) {
            if (funct3 == 0b000)
                cpu->fsgnj(rd, rs1, rs2);
            else if (funct3 == 0b001)
                cpu->fsgnjn(rd, rs1, rs2);
            else if (funct3 == 0b010)
                cpu->fsgnjx(rd, rs1, rs2);
            else
                is_invalid = true;
        }
        else if (funct7 == 0b1100000 && funct3 == 0b000) {
            if (rs2 == 0b00000)
                cpu->fcvt_w_s(rd, rs1);
            else
                is_invalid = true;
        }
        else if (funct7 == 0b1010000)
            if (funct3 == 0b010)
                cpu->feq(rd, rs1, rs2);
            else if (funct3 == 0b000)
                cpu->fle(rd, rs1, rs2);
            else
                is_invalid = true;
        else if (funct7 == 0b1101000 && funct3 == 0b000 && rs2 == 0b00000)
            cpu->fcvt_s_w(rd, rs1);
        else if (funct7 == 0b1111000 && funct3 == 0b000 && rs2 == 0b00000)
            cpu->fmv_s_x(rd, rs1);
        else
            is_invalid = true;
    } else if (opcode == 0b0100011 || opcode == 0b0100111) { // S type
        uint32_t imm_lo = (word >> 25) << 5 | rd;
        uint32_t imm_u = (imm_lo >> 11) ? (0xfffff000 | imm_lo) : imm_lo; // sign extension
        int32_t imm = *(int32_t *)&imm_u;

        if (opcode == 0b0100011 && funct3 == 0b010)
            cpu->sw(rs2, rs1, imm);
        else if (opcode == 0b0100111 && funct3 == 0b010)
            cpu->fsw(rs2, rs1, imm);
        else
            is_invalid = true;
    } else if (opcode == 0b1100011) { // SB type
        uint32_t imm_lo = (word >> 31) << 12 | (word & 0b10000000) << 4 | (word & 0x7e000000) >> 20 | (word & 0xf00) >> 7;
        uint32_t imm_u = (imm_lo >> 12) ? (0xffffe000 | imm_lo) : imm_lo;
        int32_t imm = *(int32_t *)&imm_u;

        if (funct3 == 0b000)
            cpu->beq(rs1, rs2, imm);
        else if (funct3 == 0b001)
            cpu->bne(rs1, rs2, imm);
        else if (funct3 == 0b100)
            cpu->blt(rs1, rs2, imm);
        else if (funct3 == 0b101)
            cpu->bge(rs1, rs2, imm);
        else
            is_invalid = true;
    } else if (opcode == 0b0110111) { // U type
        uint32_t imm_u = word & 0xfffff000;
        cpu->lui(rd, imm_u);
    } else if (opcode == 0b1101111) { // UJ type
        uint32_t imm_lo = (word >> 31) << 20 | (word & 0xff000) | (word & 0x100000) >> 9 | (word & 0x7fe00000) >> 20;
        uint32_t imm_u = (imm_lo >> 20) ? (0xffe00000 | imm_lo) : imm_lo;
        int32_t imm = *(int32_t *)&imm_u;

        cpu->jal(rd, imm);
    } else if (word == 0) {
        cpu->halt();
    } else if (opcode == 0b0000010 && funct7 == 0 && rs2 == 0 && rs1 == 0 && funct3 == 0) {
        cpu->inb(rd);
    } else if (opcode == 0b0000110 && funct7 == 0 && rs1 == 0 && funct3 == 0 && rd == 0) {
        cpu->outb(rs2);
    } else { // I type
        uint32_t imm_lo = word >> 20;
        uint32_t shamt = imm_lo & 0b11111;
        uint32_t imm_u = (imm_lo >> 11) ? (0xfffff000 | imm_lo) : imm_lo;
        int32_t imm = *(int32_t *)&imm_u;

        if (opcode == 0b0010011) {
            if (funct3 == 0b000)
                cpu->addi(rd, rs1, imm);
            else if (funct3 == 0b001 && funct7 == 0b0000000)
                cpu->slli(rd, rs1, shamt);
            else if (funct3 == 0b101 && funct7 == 0b0100000)
                cpu->srai(rd, rs1, shamt);
            else
                is_invalid = true;
        }
        else if (opcode == 0b0000011 && funct3 == 0b010)
            cpu->lw(rd, rs1, imm);
        else if (opcode == 0b0000111 && funct3 == 0b010)
            cpu->flw(rd, rs1, imm);
        else if (opcode == 0b1100111 && funct3 == 0b000)
            cpu->jalr(rd, rs1, imm);
        else
            is_invalid = true;
    }

    if (is_invalid) {
        print_line_of_text_addr(cpu->get_pc());
        cerr << "Invalid instruction." << endl << endl;
        return false;
    }

    cpu->inc_clocks();
    return true;
}

