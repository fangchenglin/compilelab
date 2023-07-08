#include "backend/generator.h"

#include <assert.h>
#include <iostream>
#define TODO assert(0 && "todo");
using namespace rv;










rv::rvREG backend::Generator::getRd(ir::Operand = ir::Operand())
{
    return rvREG::t0;
}

std::vector<backend::stackVarMap> stacks;

backend::Generator::Generator(ir::Program &p, std::ofstream &f) : program(p), fout(f) {}

void backend::Generator::gen()
{
    std::map<std::string, std::vector<std::string>> globalArr;
    for (auto glovalV : this->program.globalVal){
        this->fout << ".data"
                   << "\n";
        this->fout << glovalV.val.name << ":\n";
        globalArr[glovalV.val.name].resize(1);
        globalArr[glovalV.val.name][0] = "0";
        ir::Function globalFunction;
        for(auto function: this->program.functions){
            if(function.name=="global"){
                globalFunction=function;
            }

        }


        for (auto inst : globalFunction.InstVec)
        {
            if (inst->op == ir::Operator::alloc)
            {
                globalArr[inst->des.name].resize(std::stoi(inst->op1.name));
            }
            if (inst->op == ir::Operator::mov)
            {
                if (inst->des.name == glovalV.val.name)
                {
                    globalArr[glovalV.val.name].resize(1);
                    if (inst->op1.type == ir::Type::IntLiteral)
                    {
                        globalArr[glovalV.val.name][0] = inst->op1.name;
                    }
                }
            }
            else if (inst->op == ir::Operator::store)
            {
                if (inst->op1.name == glovalV.val.name)
                {
                    if (inst->op2.type == ir::Type::IntLiteral)
                    {
                        globalArr[glovalV.val.name][std::stoi(inst->op2.name)] = inst->des.name;
                    }
                }
            }
        }
        for (auto i : globalArr[glovalV.val.name])
        {
            this->fout << "    .word   " << i << '\n';
        }


        stacks.push_back(backend::stackVarMap());
    }

    this->fout << ".text\n.global    main  \n";

    for (auto func : this->program.functions)
    {

        if(func.name!="global"){
            gen_func(func);
        }
    }
}

void backend::Generator::gen_func(const ir::Function&function){
    this->fout << function.name << ":\n";
    stacks.push_back(backend::stackVarMap());

    int instNumber = function.InstVec.size() * 4 + 12;
    this->fout << "   addi sp,sp," << -instNumber << '\n';
    this->fout << "   sw ra,12(sp)" << '\n';
    this->fout << "   sw s0,8(sp)" << '\n';
    this->fout << "   addi s0,sp," << instNumber << '\n';


    for (int i = 0; i < function.ParameterList.size(); ++i)
    {
        this->fout << "   sw a" << i << ",   " << this->find_operand(function.ParameterList[i]) << "(s0)\n";
    }

    // label pass
    this->cur_label.clear();
    for (int i = 0; i < function.InstVec.size(); ++i)
    {
        auto inst = function.InstVec[i];
        if (inst->op == ir::Operator::_goto)
        {
            assert(inst->des.type == ir::Type::IntLiteral);
            if (!this->cur_label.count(i + std::stoi(inst->des.name)))
            {
                this->cur_label[i + std::stoi(inst->des.name)] = ".LBB0_" + std::to_string(labelNumber);
                labelNumber += 1;
            }
        }
    }

    for (int i = 0; i < function.InstVec.size(); ++i)
    {
        if (this->cur_label.count(i))
        {
            this->fout << cur_label[i] << ":\n";
        }
        auto inst = function.InstVec[i];
        this->curLabelNumber = i;
        this->gen_instr(inst);
        if (this->ret)
        {
            this->fout << "   lw ra,12(sp)" << '\n';
            this->fout << "   lw s0,8(sp)" << '\n';
            this->fout << "   addi sp,sp," << instNumber << '\n';
            this->fout << "   ret\n";
            this->ret = false;
        }
    }
    stacks.pop_back();
}

int backend::stackVarMap::add_operand(ir::Operand oper, uint32_t size = 1)
{
    this->idx -= size * 4;
    this->getIndex[oper.name] = this->idx;
    return this->getIndex[oper.name];
}

int backend::Generator::find_operand(ir::Operand oper)
{
    if (!stacks[stacks.size() - 1].getIndex.count(oper.name))
    {
        std::string gv;
        for (auto i : this->program.globalVal)
        {
            if (i.val.name == oper.name)
            {
                return -1;
            }
        }
        stacks[stacks.size() - 1].add_operand(oper);
    }
    return stacks[stacks.size() - 1].getIndex[oper.name];
}

namespace rv
{
    std::string toString(rvREG r)
    {
        switch (r)
        {
            case rvREG::zero:
                return "zero";
                break;
            case rvREG::ra:
                return "ra";
                break;
            case rvREG::sp:
                return "sp";
                break;
            case rvREG::gp:
                return "gp";
                break;
            case rvREG::tp:
                return "tp";
                break;
            case rvREG::t0:
                return "t0";
                break;
            case rvREG::t1:
                return "t1";
                break;
            case rvREG::t2:
                return "t2";
                break;
            case rvREG::t3:
                return "t3";
                break;
            case rvREG::s0:
                return "s0";
                break;
            case rvREG::a0:
                return "a0";
                break;
            default:
                break;
        }
    }

}
void backend::Generator::Store(rv::rvREG reg, ir::Operand operand, ir::Operand offset = ir::Operand("0", ir::Type::IntLiteral))
{
    if (offset.name == "0")
    {
        if (this->find_operand(operand) != -1)
        {
            this->fout << "   sw    " << toString(reg) << ", " << this->find_operand(operand) << "(s0)\n";
        }
        else
        {
            assert(reg != rvREG::t0);
            this->fout << "   la    " << toString(rvREG::t0) << "," << operand.name << "\n";
            this->fout << "   sw    " << toString(reg) << ", 0(" << toString(rvREG::t0) << ")\n";
        }
        return;
    }
    if (offset.type == ir::Type::IntLiteral)
    {
        this->fout << "   li    " << toString(rv::rvREG::t3) << ", " << std::stoi(offset.name) << "\n";
    }
    else
    {
        this->fout << "   lw    " << toString(rv::rvREG::t3) << ", " << this->find_operand(offset) << "(s0)\n";
    }

    if (this->find_operand(operand) == -1)
    {
        this->fout << "   la    " << toString(rvREG::t2) << "," << operand.name << "\n";
        this->fout << "   slli " << toString(rv::rvREG::t3) << ", " << toString(rvREG::t3) << ", " << 2 << '\n';
        this->fout << "   add " << toString(rv::rvREG::t3) << ", " << toString(rvREG::t3) << ", " << toString(rvREG::t2) << '\n';
        this->fout << "   sw    " << toString(reg) << ", " << 0 << "(" << toString(rvREG::t3) << ")\n";
        return;
    }
    else
    {
        this->fout << "   li    " << toString(rv::rvREG::t2) << ", " << this->find_operand(operand) << "\n";
        this->fout << "   slli " << toString(rv::rvREG::t3) << ", " << toString(rvREG::t3) << ", " << 2 << '\n';
        this->fout << "   add " << toString(rv::rvREG::t3) << ", " << toString(rvREG::t3) << ", " << toString(rvREG::t2) << '\n';
        this->fout << "   add " << toString(rv::rvREG::t3) << ", " << toString(rvREG::t3) << ", " << toString(rvREG::s0) << '\n';
        this->fout << "   sw " << toString(reg) << ", " << 0 << "(t3)\n";
        return;
    }
}
void backend::Generator::load(ir::Operand operand, rv::rvREG t, ir::Operand offset = ir::Operand("0", ir::Type::IntLiteral))
{

    // process def/mov
    if (offset.name == "0")
    {
        if (operand.type == ir::Type::IntLiteral)
        {
            this->fout << "   li    " << toString(t) << ", " << std::stoi(operand.name) << "\n";
            return;
        }
        else
        {
            if (this->find_operand(operand) == -1)
            {
                this->fout << "   la    " << toString(t) << "," << operand.name << "\n";
                this->fout << "   lw    " << toString(t) << ", " << 0 << "(" << toString(t) << ")\n";
                return;
            }
            else
            {
                this->fout << "   lw " << toString(t) << ", " << this->find_operand(operand) << "(s0)\n";
                return;
            }
        }
    }

    if (offset.type == ir::Type::IntLiteral)
    {
        this->fout << "   li    " << toString(rv::rvREG::t3) << ", " << std::stoi(offset.name) << "\n";
    }
    else
    {
        this->fout << "   lw    " << toString(rv::rvREG::t3) << ", " << this->find_operand(offset) << "(s0)\n";
    }

    if (operand.type == ir::Type::IntLiteral || operand.type == ir::Type::FloatLiteral)
    {
        this->fout << "   addi " << toString(t) << "," << toString(rv::rvREG::t3) << "," << 0 << '\n';

        return;
    }
    if (this->find_operand(operand) == -1)
    {
        this->fout << "   la    " << toString(rvREG::t1) << "," << operand.name << "\n";
        this->fout << "   slli " << toString(rv::rvREG::t3) << ", " << toString(rvREG::t3) << ", " << 2 << '\n';
        this->fout << "   add " << toString(rv::rvREG::t3) << ", " << toString(rvREG::t3) << ", " << toString(rvREG::t1) << '\n';
        this->fout << "   lw    " << toString(t) << ", " << 0 << "(" << toString(rvREG::t3) << ")\n";
        return;
    }
    else
    {
        this->fout << "   li    " << toString(rv::rvREG::t1) << ", " << this->find_operand(operand) << "\n";
        this->fout << "   slli " << toString(rv::rvREG::t3) << ", " << toString(rvREG::t3) << ", " << 2 << '\n';
        this->fout << "   add " << toString(rv::rvREG::t3) << ", " << toString(rvREG::t3) << ", " << toString(rvREG::t1) << '\n';
        this->fout << "   add " << toString(rv::rvREG::t3) << ", " << toString(rvREG::t3) << ", " << toString(rvREG::s0) << '\n';
        this->fout << "   lw " << toString(t) << ", " << 0 << "(t3)\n";
        return;
    }
}
void backend::Generator::gen_instr(ir::Instruction *inst)
{
    rv::rv_inst irInst;

    if(inst->op==ir::Operator::_return){
        if(inst->op1.name!="null"){
            load(inst->op1,rv::rvREG::t1);
            fout << "    addi a0," << toString(rvREG::t1) << ",0" << '\n';

        }
        this->ret = true;

    } else if(inst->op==ir::Operator::def||inst->op==ir::Operator::mov){
        this->load(inst->op1, rvREG::t1);
        this->Store(rvREG::t1, inst->des);
    } else if(inst->op==ir::Operator::call){
        if(inst->op1.name!="global"){
            ir::CallInst* callInst=dynamic_cast<ir::CallInst*>(inst);
            for(int i=0;i<callInst->argumentList.size();++i){
                ir::Operand operand=callInst->argumentList[i];
                load(operand.name,rv::rvREG::t0);
                this->fout << "   addi a" << i << "," << toString(rv::rvREG::t0) << ",0" << '\n';

            }
            this->fout << "   call   " << callInst->op1.name << '\n';
            Store(rv::rvREG::a0,callInst->des);
        }
    } else if(inst->op==ir::Operator::alloc){
        stacks[stacks.size() - 1].add_operand(inst->des, std::stoi(inst->op1.name));

    }else if(inst->op==ir::Operator::store){
        load(inst->des, rvREG::t1);
        this->Store(rv::rvREG::t1, inst->op1, inst->op2);
    } else if(inst->op==ir::Operator::load){
        this->load(inst->op1.name, rv::rvREG::t1, inst->op2);
        this->Store(rv::rvREG::t1, inst->des);
    } else if(inst->op==ir::Operator::subi||inst->op==ir::Operator::addi){
        this->load(inst->op1, rv::rvREG::t1);
        this->fout << "   addi " << toString(rv::rvREG::t0) << "," << toString(rv::rvREG::t1) << "," << -std::stoi(inst->op2.name) << '\n';

        this->Store(rvREG::t0, inst->des);

    } else if(inst->op==ir::Operator::add||inst->op==ir::Operator::sub
                ||inst->op==ir::Operator::div||inst->op==ir::Operator::mul
                ||inst->op==ir::Operator::mod||inst->op==ir::Operator::lss
                ||inst->op==ir::Operator::eq||inst->op==ir::Operator::gtr
                ||inst->op==ir::Operator::neq||inst->op==ir::Operator::_and
                ||inst->op==ir::Operator::_or   ){
        load(inst->op1, rvREG::t1);
        load(inst->op2, rvREG::t2);
        if(inst->op==ir::Operator::add)
        {
            this->fout << "   add " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", "<< toString(rvREG::t2) << '\n';
        } else if(inst->op==ir::Operator::sub){
            this->fout << "   sub " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        } else if(inst->op==ir::Operator::div){
            this->fout << "   div " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        }else if(inst->op==ir::Operator::mul){
            this->fout << "   mul " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        } else if(inst->op==ir::Operator::mod){
            this->fout << "   rem " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        }else if(inst->op==ir::Operator::lss){
            this->fout << "   slt " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        } else if(inst->op==ir::Operator::eq){
            this->fout << "   sub " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
            this->fout << "   seqz " << toString(this->getRd(inst->des)) << ", " << toString(this->getRd(inst->des)) << '\n';
        }else if(inst->op==ir::Operator::gtr){
            this->fout << "   sgt " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        } else if(inst->op==ir::Operator::neq){
            this->fout << "   sub " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
            this->fout << "   snez " << toString(this->getRd(inst->des)) << ", " << toString(this->getRd(inst->des)) << '\n';
        } else if(inst->op==ir::Operator::_and){
            this->fout << "   and " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        } else if(inst->op==ir::Operator::_or){
            this->fout << "   or " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        }
        this->Store(this->getRd(inst->des), inst->des);
    } else if(inst->op==ir::Operator::geq||inst->op==ir::Operator::leq){
        load(inst->op1, rvREG::t1);
        load(inst->op2, rvREG::t2);
        this->fout << "   sub " << toString(rvREG::t3) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->fout << "   seqz " << toString(rvREG::t3) << ", " << toString(rvREG::t3) << '\n';
        if(inst->op==ir::Operator::geq){
            this->fout << "   sgt " << toString(rvREG::t0) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        } else if(inst->op==ir::Operator::leq){
            this->fout << "   slt " << toString(rvREG::t0) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        }
        this->fout << "   or " << toString(rvREG::t0) << ", " << toString(rvREG::t0) << ", " << toString(rvREG::t3) << '\n';
        this->Store(rvREG::t0, inst->des);
    } else if(inst->op==ir::Operator::_not){
        load(inst->op1, rvREG::t1);
        this->fout << "   seqz " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << '\n';
        this->Store(this->getRd(inst->des), inst->des);
    } else if(inst->op==ir::Operator::_goto){
        if (inst->op1.type != ir::Type::Int)
        {
            this->fout << "   j " << cur_label[this->curLabelNumber + std::stoi(inst->des.name)] << '\n';
        }
        else
        {
            load(inst->op1, rvREG::t1);
            this->fout << "   bnez " << toString(rvREG::t1) << ", " << cur_label[this->curLabelNumber + std::stoi(inst->des.name)] << '\n';
        }
    }


}