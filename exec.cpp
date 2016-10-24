#include <cstdint>
#include <iostream>

#include "common.h"

void step_exec(uint32_t word, CPU *cpu)
{
    uint32_t opcode, rd, funct3, rs1, rs2;
    opcode = word & 0b1111111;
    rd = (word >> 7) & 0b11111;
    funct3 = (word >> 12) & 0b111;
    rs1 = (word >> 15) & 0b11111;
    rs2 = (word >> 20) & 0b11111;

    if (opcode == 0b0110011) { // R type
        uint32_t funct7 = word >> 25;
        if (funct7 == 0b0000000)
            cpu->add(rd, rs1, rs2);
        else if (funct7 == 0b0100000)
            cpu->sub(rd, rs1, rs2);
    } else if (opcode == 0b0100011) { // S type
        uint32_t imm_lo = (word >> 25) << 5 | rd;
        uint32_t imm_u = (imm_lo >> 11) ? (0xfffff000 | imm_lo) : imm_lo; // sign extension
        int32_t imm = *(int32_t *)&imm_u;

        cpu->sw(rs2, rs1, imm);
    } else if (opcode == 0b1100011) { // SB type
        uint32_t imm_lo = (word >> 31) << 12 | (word & 0b10000000) << 4 | (word & 0x7e00000000) >> 20 | (word & 0xf00) >> 7;
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
    } else if (opcode == 0b1101111) { // UJ type
        uint32_t imm_lo = (word >> 31) << 20 | (word & 0xff000) | (word & 0x100000) >> 9 | (word & 0x7fe00000) >> 20;
        uint32_t imm_u = (imm_lo >> 20) ? (0xffe00000 | imm_lo) : imm_lo;
        int32_t imm = *(int32_t *)&imm_u;

        cpu->jal(rd, imm);
    } else if (opcode == 0) {
        cpu->halt();
    } else { // I type
        uint32_t imm_lo = word >> 20;
        uint32_t imm_u = (imm_lo >> 11) ? (0xfffff000 | imm_lo) : imm_lo;
        int32_t imm = *(int32_t *)&imm_u;

        if (opcode == 0b0010011)
            cpu->addi(rd, rs1, imm);
        else if (opcode == 0b0000011)
            cpu->lw(rd, rs1, imm);
        else if (opcode == 0b1100111)
            cpu->jalr(rd, rs1, imm);
    }
}

