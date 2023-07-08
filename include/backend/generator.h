#ifndef GENERARATOR_H
#define GENERARATOR_H

#include "ir/ir.h"
#include "backend/rv_def.h"
#include "backend/rv_inst_impl.h"

#include <map>
#include <string>
#include <vector>
#include <fstream>

namespace backend
{

    // it is a map bewteen variable and its mem addr, the mem addr of a local variable can be identified by ($sp + off)
    struct stackVarMap
    {


        /**
         * @brief add a ir::Operand into current map, alloc space for this variable in memory
         * @param[in] size: the space needed(in byte)
         * @return the offset
         */std::map<std::string, int> getIndex;
        int add_operand(ir::Operand, uint32_t size);


        int idx = 0;
    };

    struct Generator
    {
        const ir::Program &program; // the program to gen
        std::ofstream &fout;        // output file
        bool ret=false;
        Generator(ir::Program &, std::ofstream &);
        int find_operand(ir::Operand);
        std::map<int, std::string> cur_label;
        int labelNumber=1;
        int curLabelNumber=0;
        void load(ir::Operand operand, rv::rvREG t,ir::Operand offset);
        void Store(rv::rvREG,ir::Operand,ir::Operand offset);

        rv::rvREG getRd(ir::Operand);


        void gen();
        void gen_func(const ir::Function&);

        void gen_instr(ir::Instruction *);
    };

}

#endif