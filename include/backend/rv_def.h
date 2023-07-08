#ifndef RV_DEF_H
#define RV_DEF_H

#include <string>

namespace rv
{

    // rv interger registers
    enum class rvREG
    {
        zero,
        ra,
        sp, // stack pointer, point to the top of cur stack
        gp,
        tp,
        t0,
        t1,
        t2,
        t3,
        s0, // frame pointer fp point to start of callee,
        a0
    };


    enum class rvFREG
    {
        ft0,
        ft1,
        ft2
    };
    std::string toString(rvFREG r); // implement this in ur own way

    // rv32i instructions
    // add instruction u need here!
    enum class rvOPCODE
    {
        // RV32I Base Integer Instructions
        ADD,
        SUB,
        XOR,
        OR,
        AND,
        SLL,
        SRL,
        SRA,
        SLT,
        SLTU, // arithmetic & logic
        ADDI,
        XORI,
        ORI,
        ANDI,
        SLLI,
        SRLI,
        SRAI,
        SLTI,
        SLTIU, // immediate
        LW,
        SW, // load & store
        BEQ,
        BNE,
        BLT,
        BGE,
        BLTU,
        BGEU, // conditional branch
        JAL,
        JALR, // jump

        // RV32M Multiply Extension

        // RV32F / D Floating-Point Extensions

        // Pseudo Instructions
        LA,
        LI,
        MOV,
        J,
        RET // ...
    };
    std::string toString(rvOPCODE r); // implement this in ur own way

} // namespace rv

#endif