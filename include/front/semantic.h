/**
 * @file semantic.h
 * @author Yuntao Dai (d1581209858@live.com)
 * @brief
 * @version 0.1
 * @date 2023-01-06
 *
 * a Analyzer should
 * @copyright Copyright (c) 2023
 *
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include"ir/ir.h"
#include"front/abstract_syntax_tree.h"

#include<map>
#include<string>
#include<vector>
using std::map;
using std::string;
using std::vector;

namespace frontend
{

// definition of symbol table entry
    struct STE {
        ir::Operand operand;
        vector<int> dimension;
        string number;
    };
    using map_str_ste = map<string, STE>;
    struct ScopeInfo {
        int cnt;
        string name;
        map_str_ste table;
    };

// surpport lib functions
    map<std::string,ir::Function*>* get_lib_funcs();

// definition of symbol table
    struct SymbolTable{
        vector<ScopeInfo> scope_stack;
        map<std::string,ir::Function*> functions;

        /**
         * @brief enter a new scope, record the infomation in scope stacks
         * @param node: a Block node, entering a new Block means a new name scope
         */
        void add_scope(char c);

        /**
         * @brief exit a scope, pop out infomations
         */
        void exit_scope();

        /**
         * @brief Get the scoped name, to deal the same name in different scopes, we change origin id to a new one with scope infomation,
         * for example, we have these code:
         * "
         * int a;
         * {
         *      int a; ....
         * }
         * "
         * in this case, we have two variable both name 'a', after change they will be 'a' and 'a_block'
         * @param id: origin id
         * @return string: new name with scope infomations
         */
        string get_scoped_name(string id) const;

        /**
         * @brief get the right operand with the input name
         * @param id identifier name
         * @return Operand
         */
        ir::Operand get_operand(string id) const;

        /**
         * @brief get the right ste with the input name
         * @param id identifier name
         * @return STE
         */
        STE get_ste(string id) const;
    };


// singleton class
    struct Analyzer {
        int tmp_cnt = 0;
        vector<ir::Instruction*> g_init_inst;
        SymbolTable symbol_table;


        /**
         * @brief constructor
         */
        Analyzer();

        ir::Program get_ir_program(CompUnit*);

        Analyzer(const Analyzer&) = delete;
        Analyzer& operator=(const Analyzer&) = delete;

        void analysisCompUnit(CompUnit*, ir::Program&);

        void analysisConstInitVal(ConstInitVal*, vector<ir::Instruction*>&);
        void analysisVarDecl(VarDecl*, vector<ir::Instruction*>&);
        void analysisVarDef(VarDef*, vector<ir::Instruction*>&,ir::Type);
        void analysisInitVal(InitVal*, vector<ir::Instruction*>&);

        void analysisLAndExp(LAndExp*, vector<ir::Instruction*>&);
        void analysisConstExp(ConstExp*, vector<ir::Instruction*>&);string analysisIdent(Term *term);
        void analysisFuncFParam(FuncFParam*,ir::Function&);
        void analysisFuncFParams(FuncFParams*,ir::Function&);
        void analysisNumber(Number*, vector<ir::Instruction*>&);
        void analysisAddExp(AddExp*,vector<ir::Instruction*>&);
        void analysisRelExp(RelExp*, vector<ir::Instruction*>&);
        void analysisEqExp(EqExp*, vector<ir::Instruction*>&);
        void analysisExp(Exp* root,vector<ir::Instruction*>&);
        void analysisCond(Cond*, vector<ir::Instruction*>&);
        void analysisLVal(LVal*, vector<ir::Instruction*>&);
        void analysisPrimaryExp(PrimaryExp*, vector<ir::Instruction*>&);
        void analysisUnaryExp(UnaryExp*, vector<ir::Instruction*>&);
        void analysisMulExp(MulExp*, vector<ir::Instruction*>&);
        void analysisFuncDef(FuncDef*, ir::Function&);
        void analysisDecl(Decl* root,vector<ir::Instruction*>&);
        void analysisConstDecl(ConstDecl*, vector<ir::Instruction*>&);
        void analysisBType(BType* root, vector<ir::Instruction*>& vector);
        void analysisConstDef(ConstDef*, vector<ir::Instruction*>&,ir::Type);
        void analysisLOrExp(LOrExp*, vector<ir::Instruction*>&);
        void analysisBlock(Block *root, vector<ir::Instruction*>& , bool is_func_block);
        void analysisBlockItem(BlockItem* root,vector<ir::Instruction*>&);
        void analysisStmt(Stmt* root,vector<ir::Instruction*>&);




    };

} // namespace frontend

#endif