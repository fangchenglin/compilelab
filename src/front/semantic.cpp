#include"front/semantic.h"

#include<cassert>

using ir::Instruction;
using ir::Function;
using ir::Operand;
using ir::Operator;

#define TODO assert(0 && "TODO");

#define GET_CHILD_PTR(node, type, index) auto node = dynamic_cast<type*>(root->children[index]); assert(node);
#define ANALYSIS(node, type, index) auto node = dynamic_cast<type*>(root->children[index]); assert(node); analysis##type(node, buffer);
#define COPY_EXP_NODE(from, to) to->is_computable = from->is_computable; to->v = from->v; to->t = from->t;

map<std::string,ir::Function*>* frontend::get_lib_funcs() {
    static map<std::string,ir::Function*> lib_funcs = {
            {"getint", new Function("getint", Type::Int)},
            {"getch", new Function("getch", Type::Int)},
            {"getfloat", new Function("getfloat", Type::Float)},
            {"getarray", new Function("getarray", {Operand("arr", Type::IntPtr)}, Type::Int)},
            {"getfarray", new Function("getfarray", {Operand("arr", Type::FloatPtr)}, Type::Int)},
            {"putint", new Function("putint", {Operand("i", Type::Int)}, Type::null)},
            {"putch", new Function("putch", {Operand("i", Type::Int)}, Type::null)},
            {"putfloat", new Function("putfloat", {Operand("f", Type::Float)}, Type::null)},
            {"putarray", new Function("putarray", {Operand("n", Type::Int), Operand("arr", Type::IntPtr)}, Type::null)},
            {"putfarray", new Function("putfarray", {Operand("n", Type::Int), Operand("arr", Type::FloatPtr)}, Type::null)},
    };
    return &lib_funcs;
}
ir::Function* curr_function = nullptr;
int g_block=0;
int i_block=0;
int w_block=0;
int e_block=0;
int f_block=0;
int b_block=0;
int getBlockNumber(char c){
    if(c=='g') return g_block;
    if(c=='i') return  i_block;
    if(c=='w') return w_block;
    if(c=='e') return  e_block;
    if(c=='b') return b_block;
    if(c=='f') return  f_block;
}
void frontend::SymbolTable::add_scope(char c){
    ScopeInfo scope_info;
    scope_info.name = c + std::to_string(getBlockNumber(c));
    scope_info.cnt = scope_stack.size();
    scope_stack.push_back(scope_info);

}

void frontend::SymbolTable::exit_scope() {
    scope_stack.pop_back();
}

string frontend::SymbolTable::get_scoped_name(string id) const {
    for(int i = scope_stack.size()-1; i >= 0;i--)
    {
        if(scope_stack[i].table.find(id)!=scope_stack[i].table.end())
            return id + '_' + scope_stack[i].name;
    }
}

Operand frontend::SymbolTable::get_operand(string id) const {
    for (int i = scope_stack.size()-1; i >= 0; i--)
    {
        if (scope_stack[i].table.find(id) != scope_stack[i].table.end())
            return scope_stack[i].table.find(id)->second.operand;
    }
}

frontend::STE frontend::SymbolTable::get_ste(string id) const {
    for (int i = scope_stack.size()-1; i >= 0; i--)
    {
        if (scope_stack[i].table.find(id) != scope_stack[i].table.end())
            return scope_stack[i].table.find(id)->second;
    }
}

frontend::Analyzer::Analyzer(): tmp_cnt(0), symbol_table() {

}

ir::Program frontend::Analyzer::get_ir_program(CompUnit* root) {
    ir::Program program;
    symbol_table.add_scope('g');
    auto get_func = *get_lib_funcs();
    for (auto it = get_func.begin(); it != get_func.end(); it++) { symbol_table.functions[it->first] = it->second; }
    ir::Function *globalFunc = new ir::Function("global", ir::Type::null);
    this->symbol_table.functions["global"] = globalFunc;
    analysisCompUnit(root,program);
    for (auto it = symbol_table.scope_stack[0].table.begin(); it != symbol_table.scope_stack[0].table.end(); it++)
    {
        if (it->second.dimension.size() != 0)
        {
            int len = 1;
            for (int i = 0; i < it->second.dimension.size(); i++)
                len *= it->second.dimension[i];
            program.globalVal.push_back({{it->second.operand}, len});
        }
        else
            program.globalVal.push_back({it->second.operand});
    }
    ir::Instruction* globalreturn = new ir::Instruction({},
                                                        {},
                                                        {}, ir::Operator::_return);
    globalFunc->addInst(globalreturn);
    program.addFunction(*globalFunc);
    return program;
}

void frontend::Analyzer::analysisCompUnit(CompUnit* root, ir::Program& program){


    //在这个地方的定义只能是全局变量
    if (root->children[0]->type == frontend::NodeType::DECL)
    {
        auto globalFunc =symbol_table.functions["global"];
        std::vector<ir::Instruction *>& buffer = globalFunc->InstVec;
        ANALYSIS(decl_node, Decl, 0);
    } else if (root->children[0]->type == frontend::NodeType::FUNCDEF)
    {
        ir::Function *new_function = new Function();
        auto &buffer = *new_function;
        ANALYSIS(funcdef_node, FuncDef, 0);
        program.functions.push_back(buffer);
    }


    // CompUnit -> (Decl | FuncDef) CompUnit
    if (root->children.size() == 2)
    {
        auto compunit=dynamic_cast<CompUnit*>(root->children[1]);
        analysisCompUnit(compunit,program);
    }

}
//FuncDef -> FuncType Ident '(' [FuncFParams] ')' Block
//
//        FuncDef.t
//        FuncDef.n
void frontend::Analyzer::analysisFuncDef(FuncDef* root, ir::Function& function){
    auto tk=dynamic_cast<Term*>(root->children[0]->children[0])->token.type;
    root->t=tk==TokenType::VOIDTK ? Type::null : tk==TokenType::INTTK?Type::Int:Type::Float;
    root->n=dynamic_cast<Term*>(root->children[1])->token.value;//add
    ir::Type type=root->t;
    string identname=root->n;
    //进入新的作用域
    symbol_table.add_scope('f');
    //判断有无FuncFParams
    vector<ir::Operand> parems;
    bool has_params = true;
    //没有 FuncFParams
    if (dynamic_cast<Term *>(root->children[3]))
    {
        has_params = false;
    }
    if (has_params)
    {
        analysisFuncFParams(dynamic_cast<FuncFParams *>(root->children[3]),function);
    }
    function.returnType = type;
    function.name = identname;
    if (identname == "main")
    {
        string tmp = "t" + std::to_string(tmp_cnt++);
        ir::Operand op1("global", ir::Type::null);
        ir::Operand des(tmp, ir::Type::null);
        ir::CallInst *callInst = new ir::CallInst(op1,des);
        function.addInst(callInst);
    }
    symbol_table.functions[identname] = &function;
    curr_function = &function;

    auto& buffer = function.InstVec;
    int index = root->children.size()-1;
    Block * block = dynamic_cast<Block*>(root->children[index]);
    analysisBlock(block,buffer,true);
    symbol_table.exit_scope();
    if (identname == "main")
    {
        Instruction *retInst = new ir::Instruction(ir::Operand("0",ir::Type::IntLiteral),
                                                   ir::Operand(),
                                                   ir::Operand(),ir::Operator::_return);
        function.addInst(retInst);
    }
    if (type == ir::Type::null)
    {
        Instruction *instruction = new ir::Instruction({},
                                                       {},
                                                       {},ir::Operator::_return);
        function.addInst(instruction);
    }
}

string frontend::Analyzer::analysisIdent(Term *term)
{
    return term->token.value;
}

void frontend::Analyzer::analysisDecl(Decl* root,vector<ir::Instruction*>& buffer)
{
    if(dynamic_cast<VarDecl *>(root->children[0]))
    {
        ANALYSIS(vardecl_node,VarDecl,0);
    }
    else if(dynamic_cast<ConstDecl *>(root->children[0]))
    {
        ANALYSIS(constdecl_node,ConstDecl,0);
    }
}
// VarDecl -> BType VarDef { ',' VarDef } ';'
//VarDecl: int a=10,b=10;
// VarDecl.t
void frontend::Analyzer::analysisVarDecl(VarDecl* root, vector<ir::Instruction*>& buffer){

    ANALYSIS(btypenode,BType,0);
    root->t=btypenode->t;
    ir::Type childType=btypenode->t;

    VarDef *vardef = dynamic_cast<VarDef *>(root->children[1]);
    analysisVarDef(vardef,buffer,root->t);

    if(root->children.size()>3){

        for(int i=3;i<root->children.size()-1;i+=2){
            VarDef *vardef1 = dynamic_cast<VarDef *>(root->children[i]);
            analysisVarDef(vardef1,buffer,root->t);
        }
    }
}
//ConstDef -> Ident { '[' ConstExp ']' } '=' ConstInitVal
//
//ConstDef.arr_name
void frontend::Analyzer::analysisConstDecl(ConstDecl* root, vector<ir::Instruction*>& buffer){
    //const变量类型

    ANALYSIS(btypenode,BType,1)
    root->t=btypenode->t;
    ir::Type childType=btypenode->t;

    ConstDef *constdef = dynamic_cast<ConstDef *>(root->children[2]);
    analysisConstDef(constdef,buffer,childType);

    for (size_t i = 4; i < root->children.size()-1; i += 2) {
        ConstDef *constdef1 = dynamic_cast<ConstDef *>(root->children[i]);
        analysisConstDef(constdef1,buffer,childType);

    }
}

void frontend::Analyzer::analysisBType(BType* root, vector<ir::Instruction*>& vector){

    root->t = (dynamic_cast<Term*>(root->children[0])->token.type == TokenType::INTTK) ? Type::Int : Type::Float;
}

void frontend::Analyzer::analysisVarDef( VarDef* root,vector<ir::Instruction*>& buffer,ir::Type type){

    Term * term = dynamic_cast<Term *>(root->children[0]);
    string varName = analysisIdent(term);
    //int a;
    if(root->children.size() == 1)
    {

        STE ste;
        ste.operand = ir::Operand(varName, type);
        symbol_table.scope_stack.back().table[varName] = ste;
        auto & ste1 = symbol_table.scope_stack.back().table[varName];
        ste1.operand.name =  symbol_table.get_scoped_name(varName);
        if(type == ir::Type::Int)
        {

            ir::Instruction* instruction = new ir::Instruction({"0",ir::Type::IntLiteral},
                                                               {},
                                                               {symbol_table.get_scoped_name(varName),type},ir::Operator::def);
            buffer.push_back(instruction);
        }
    }
        //int a=10;
    else if(root->children.size() == 3)
    {
        STE ste;
        ste.operand = ir::Operand(varName, type);
        symbol_table.scope_stack.back().table[varName] = ste;
        auto & ste1 = symbol_table.scope_stack.back().table[varName];
        ste1.operand.name =  symbol_table.get_scoped_name(varName);
        ANALYSIS(initval_node,InitVal,2);
        if(type == ir::Type::Int)
        {

            ir::Instruction* instruction = new ir::Instruction({initval_node->v,initval_node->t},
                                                               {},
                                                               {symbol_table.get_scoped_name(varName),type},ir::Operator::def);
            buffer.push_back(instruction);
        }

    }
        //int a[10];
    else if(root->children.size() == 4)
    {
        ConstExp* constexp = dynamic_cast<ConstExp*>(root->children[2]);
        analysisConstExp(constexp,buffer);
        int len = std::stoi(constexp->v);

        STE ste;
        ste.dimension.push_back(len);
        ir::Type def_type = type;
        //定义数组时，ir::Type的类型要转化为对应的指针类型
        if (def_type == ir::Type::Int)
            def_type = ir::Type::IntPtr;
        ste.operand = ir::Operand(varName, def_type);
        symbol_table.scope_stack.back().table[varName] = ste;
        auto & ste1 = symbol_table.scope_stack.back().table[varName];
        ste1.operand.name =  symbol_table.get_scoped_name(varName);

        if (symbol_table.scope_stack.size() > 1)
        {

            ir::Instruction* instruction = new ir::Instruction({std::to_string(len),ir::Type::IntLiteral},{}, {symbol_table.get_scoped_name(varName),def_type},ir::Operator::alloc);
            buffer.push_back(instruction);
        }

    }
        //int a[1]={1};
    else if(root->children.size() == 6)
    {
        ConstExp* constexp = dynamic_cast<ConstExp*>(root->children[2]);
        analysisConstExp(constexp,buffer);
        int len = std::stoi(constexp->v);
        STE ste;
        ste.dimension.push_back(len);
        ir::Type def_type = type;
        if (def_type == ir::Type::Int)
            def_type = ir::Type::IntPtr;

        ste.operand = ir::Operand(varName, def_type);
        symbol_table.scope_stack.back().table[varName] = ste;
        auto & ste1 = symbol_table.scope_stack.back().table[varName];
        ste1.operand.name =  symbol_table.get_scoped_name(varName);
        InitVal* initval = dynamic_cast<InitVal*>(root->children.back());

        ir::Instruction* instruction = new ir::Instruction({constexp->v,ir::Type::IntLiteral},
                                                           {},
                                                           {symbol_table.get_scoped_name(varName),def_type},ir::Operator::alloc);
        buffer.push_back(instruction);

        int index = 0;
        for (int i = 1; i < initval ->children.size()-1; i += 2, index++)
        {
            InitVal* subInitval = dynamic_cast<InitVal*>(initval->children[i]);
            Exp* exp = dynamic_cast<Exp*>(subInitval->children[0]);
            analysisExp(exp,buffer);
            if (def_type == ir::Type::IntPtr)
            {

                ir::Instruction* instruction1 = new ir::Instruction({symbol_table.get_scoped_name(varName),ir::Type::IntPtr},
                                                                    {std::to_string(index),ir::Type::IntLiteral}, {exp->v,exp->t},ir::Operator::store);
                buffer.push_back(instruction1);
            }
        }
    }
        //int a=[2][3];
    else if(root->children.size() == 7)
    {
        //先算出数组大小
        ConstExp* constexp1 = dynamic_cast<ConstExp*>(root->children[2]);
        analysisConstExp(constexp1,buffer);
        ConstExp* constexp2 = dynamic_cast<ConstExp*>(root->children[5]);
        analysisConstExp(constexp2,buffer);
        int len1 = std::stoi(constexp1->v);
        int len2 = std::stoi(constexp2->v);
        int len = len2 * len1;

        STE ste;
        ste.dimension.push_back(len1);
        ste.dimension.push_back(len2);
        ir::Type def_type = type;
        if (def_type == ir::Type::Int)
            def_type = ir::Type::IntPtr;
        ste.operand = ir::Operand(varName, def_type);
        symbol_table.scope_stack.back().table[varName] = ste;
        auto & ste1 = symbol_table.scope_stack.back().table[varName];
        ste1.operand.name =  symbol_table.get_scoped_name(varName);

        if (symbol_table.scope_stack.size() > 1)
        {

            ir::Instruction* instruction3 = new ir::Instruction({std::to_string(len),ir::Type::IntLiteral},{}, {symbol_table.get_scoped_name(varName),def_type},ir::Operator::alloc);

            buffer.push_back(instruction3);
        }
    }
    else if(root->children.size() == 9)
    {
        //先算出数组大小
        ConstExp* constexp1 = dynamic_cast<ConstExp*>(root->children[2]);
        analysisConstExp(constexp1,buffer);
        ConstExp* constexp2 = dynamic_cast<ConstExp*>(root->children[5]);
        analysisConstExp(constexp2,buffer);
        int len1 = std::stoi(constexp1->v);
        int len2 = std::stoi(constexp2->v);
        int len = len2 * len1;

        STE ste;
        ste.dimension.push_back(len1);
        ste.dimension.push_back(len2);
        ir::Type def_type = type;
        if (def_type == ir::Type::Int)
            def_type = ir::Type::IntPtr;
        ste.operand = ir::Operand(varName, def_type);
        symbol_table.scope_stack.back().table[varName] = ste;
        auto & ste1 = symbol_table.scope_stack.back().table[varName];
        ste1.operand.name =  symbol_table.get_scoped_name(varName);

        if (symbol_table.scope_stack.size() > 1)
        {
            ir::Instruction* instruction4 = new ir::Instruction({std::to_string(len),ir::Type::IntLiteral},{}, {symbol_table.get_scoped_name(varName),def_type},ir::Operator::alloc);
            buffer.push_back(instruction4);
        }

        InitVal* initval = dynamic_cast<InitVal*>(root->children.back());

        int index = 0;
        for (int i = 1; i < initval ->children.size()-1; i += 2, index++)
        {
            InitVal* subInitval = dynamic_cast<InitVal*>(initval->children[i]);
            Exp* exp = dynamic_cast<Exp*>(subInitval->children[0]);
            analysisExp(exp,buffer);

            ir::Instruction* instruction5 = new ir::Instruction({symbol_table.get_scoped_name(varName),ir::Type::IntPtr},
                                                                {std::to_string(index),ir::Type::IntLiteral},
                                                                {exp->v,exp->t},ir::Operator::store);
            buffer.push_back(instruction5);
        }
    }

}
// ConstDef -> Ident { '[' ConstExp ']' } '=' ConstInitVal
// ConstDef.arr_name
// const int a=10.0;
//ConstDef:a=10.0
void frontend::Analyzer::analysisConstDef(ConstDef* root, vector<ir::Instruction*>& buffer,ir::Type type){

    Term * term = dynamic_cast<Term *>(root->children[0]);
    string varName = analysisIdent(term);

    //常量声明必须赋初值
    if(root->children.size() == 3)
    {
        STE ste;
        if (type == ir::Type::Int)
            ste.operand = ir::Operand(varName, ir::Type::IntLiteral);
        symbol_table.scope_stack.back().table[varName] = ste;
        ConstInitVal* constinit = dynamic_cast<ConstInitVal*>(root->children.back());
        ConstExp* constexp = dynamic_cast<ConstExp*>(constinit->children[0]);
        analysisConstExp(constexp,buffer);
        symbol_table.scope_stack.back().table[varName].number = constexp->v;

    }

        //const int a[1]={1};
    else if(root->children.size() == 6)
    {
        ConstExp* constexp = dynamic_cast<ConstExp*>(root->children[2]);
        analysisConstExp(constexp,buffer);
        int len = std::stoi(constexp->v);

        STE ste;
        ste.dimension.push_back(len);
        ir::Type def_type = type;
        if (def_type == ir::Type::Int)
            def_type = ir::Type::IntPtr;

        ste.operand = ir::Operand(varName, def_type);
        symbol_table.scope_stack.back().table[varName] = ste;
        auto & ste1 = symbol_table.scope_stack.back().table[varName];
        ste1.operand.name =  symbol_table.get_scoped_name(varName);
        ConstInitVal* constinitval = dynamic_cast<ConstInitVal*>(root->children.back());

        ir::Instruction* instruction8 = new ir::Instruction({std::to_string(len),ir::Type::IntLiteral},
                                                            {}, {symbol_table.get_scoped_name(varName),def_type},ir::Operator::alloc);
        buffer.push_back(instruction8);

        int ind = 0;
        for (int i = 1; i < constinitval ->children.size()-1; i += 2, ind++)
        {
            ConstInitVal* subInitval = dynamic_cast<ConstInitVal*>(constinitval->children[i]);
            ConstExp* constexp = dynamic_cast<ConstExp*>(subInitval->children[0]);
            analysisConstExp(constexp,buffer);

            ir::Instruction* instruction9 = new ir::Instruction({symbol_table.get_scoped_name(varName),ir::Type::IntPtr},
                                                                {std::to_string(ind),ir::Type::IntLiteral},
                                                                {constexp->v,constexp->t},ir::Operator::store);
            buffer.push_back(instruction9);

        }
    }
        //const int a[1][1]={1,1};
    else if(root->children.size() == 9)
    {
        //先算出数组大小
        ConstExp* constexp1 = dynamic_cast<ConstExp*>(root->children[2]);
        analysisConstExp(constexp1,buffer);
        ConstExp* constexp2 = dynamic_cast<ConstExp*>(root->children[5]);
        analysisConstExp(constexp2,buffer);
        int len1 = std::stoi(constexp1->v);
        int len2 = std::stoi(constexp2->v);
        int len = len2 * len1;

        STE ste;
        ste.dimension.push_back(len1);
        ste.dimension.push_back(len2);
        ir::Type def_type = type;
        if (def_type == ir::Type::Int)
            def_type = ir::Type::IntPtr;
        ste.operand = ir::Operand(varName, def_type);
        symbol_table.scope_stack.back().table[varName] = ste;
        auto & ste1 = symbol_table.scope_stack.back().table[varName];
        ste1.operand.name =  symbol_table.get_scoped_name(varName);

        if (symbol_table.scope_stack.size() > 1)
        {

            ir::Instruction* allocInst = new ir::Instruction({std::to_string(len),ir::Type::IntLiteral},
                                                             {},
                                                             {symbol_table.get_scoped_name(varName),def_type},ir::Operator::alloc);
            buffer.push_back(allocInst);
        }

        ConstInitVal* constinitval = dynamic_cast<ConstInitVal*>(root->children.back());

        int idx = 0;
        //给数组的每项赋值
        for (int i = 1; i < constinitval ->children.size()-1; i += 2, idx++)
        {
            ConstInitVal* subInitval = dynamic_cast<ConstInitVal*>(constinitval->children[i]);
            ConstExp* constexp = dynamic_cast<ConstExp*>(subInitval->children[0]);
            analysisConstExp(constexp,buffer);


            ir::Instruction* instructionc = new ir::Instruction({symbol_table.get_scoped_name(varName),ir::Type::IntPtr},
                                                                {std::to_string(idx),ir::Type::IntLiteral},
                                                                {constexp->v,constexp->t},ir::Operator::store);
            buffer.push_back(instructionc);
        }
    }
}
// ConstInitVal -> ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
// ConstInitVal.v
// ConstInitVal.t
//{1, 2, 3}
void frontend::Analyzer::analysisConstInitVal(ConstInitVal* root, vector<ir::Instruction*>& buffer){
    if(root->children.size()==1){
        ANALYSIS(constexp_node,ConstExp,0);
        root->v = constexp_node->v;
        root->t = constexp_node->t;
    }else{
        for(int i=1;i<root->children.size()-1;i+=2){
            ANALYSIS(constinitval,ConstInitVal,i);
            root->t=constinitval->t;
            root->v=constinitval->v;
            // buffer.push_back(new Instruction({constinitval->v,constinitval->t},{constinitval->v,constinitval->t},{constinitval->v,constinitval->t},Operator::add));
        }
    }
}

//InitVal -> Exp | '{' [ InitVal { ',' InitVal } ] '}'
//
//InitVal.is_computable
//InitVal.v
//InitVal.t
void frontend::Analyzer::analysisInitVal(frontend::InitVal *root, vector<ir::Instruction *> &buffer) {
    if(root->children.size()==1)
    {

        //std::printf("enter analysisInitVal Exp\n");
        // Exp *exp = dynamic_cast<Exp *>(root->children[0]);
        ANALYSIS(exp_node,Exp,0);

        COPY_EXP_NODE(exp_node,root);

    }else{
        for(int i=1;i<root->children.size()-1;i+=2)
        {
            ANALYSIS(constinitval,InitVal,i);
            root->t=constinitval->t;
            root->v=constinitval->v;
            // buffer.push_back(new Instruction({constinitval->v,constinitval->t},{constinitval->v,constinitval->t},{constinitval->v,constinitval->t},Operator::add));
        }
    }

}

void frontend::Analyzer::analysisFuncFParam(FuncFParam* root,ir::Function& function)
{
    auto buffer=vector<Instruction*>();

    ANALYSIS(btypenode,BType,0);
    ir::Type type=btypenode->t;
    string identName = analysisIdent(dynamic_cast<Term *>(root->children[1]));
    vector<int> dimension(1, -1);
    if (root->children.size() > 2)
    {
        if (type == ir::Type::Int)
        {
            type = ir::Type::IntPtr;
        }

        if (root->children.size() == 7)
        {
            ANALYSIS(exp,Exp,6);
            int exp_result = std::stoi(exp->v);
            dimension.push_back(exp_result);
        }
    }

    ir::Operand param(identName, type);
    symbol_table.scope_stack.back().table[identName] = {param, dimension};
    function.ParameterList.push_back({symbol_table.get_scoped_name(param.name),param.type});
}

void frontend::Analyzer::analysisFuncFParams(FuncFParams* root,ir::Function& function){
    for(int i=0;i<root->children.size();i+=2){
        auto funcfparam=dynamic_cast<FuncFParam*>(root->children[i]);
        analysisFuncFParam(funcfparam,function);
    }
}

void frontend::Analyzer::analysisBlock(Block *root, vector<ir::Instruction*>& buffer,bool is_func_block)
{
    if (!is_func_block)
        symbol_table.add_scope('b');
    for(int i= 1;i<root->children.size()-1;i++)
    {
        ANALYSIS(node,BlockItem,i);
    }
    if (!is_func_block)
        symbol_table.exit_scope();
}
void frontend::Analyzer::analysisBlockItem(BlockItem* root,vector<ir::Instruction*>& buffer){

    if (dynamic_cast<Decl *>(root->children[0]) != nullptr)
    {
        ANALYSIS(node,Decl,0);
    }
    else if (dynamic_cast<Stmt *>(root->children[0]) != nullptr)
    {
        ANALYSIS(node,Stmt,0);
    }
    else
        assert(0 && " analysisBlockItem");
}
//Stmt -> LVal '=' Exp ';' | Block |
// 'if' '(' Cond ')' Stmt [ 'else' Stmt ] |
// 'while' '(' Cond ')' Stmt |
// 'break' ';' | 'continue' ';' | 'return' [Exp] ';' |
// [Exp] ';'
void frontend::Analyzer::analysisStmt(Stmt* root,vector<ir::Instruction*>& buffer){

    Term* term = dynamic_cast<Term*>(root->children[0]);

    // Stmt -> LVal '=' Exp ';'
    if(dynamic_cast<LVal*>(root->children[0]))
    {
        LVal* lval = dynamic_cast<LVal*>(root->children[0]);
        if(root->children[0]->children.size()==1)
        {
            ANALYSIS(lval,LVal,0);
            Term * term = dynamic_cast<Term*>(root->children[0]->children[0]);
            string varName = analysisIdent(term);
            ANALYSIS(exp_node,Exp,2);
            if(exp_node->t == ir::Type::IntLiteral)
            {   ir::Operand op11(exp_node->v,exp_node->t);
                ir::Operand des1(symbol_table.get_scoped_name(varName),ir::Type::Int);
                ir::Instruction* instruction1 = new ir::Instruction(op11,{},des1,ir::Operator::mov);
                buffer.push_back(instruction1);
            }
            else if(exp_node->t == ir::Type::Int)
            {
                ir::Operand op11(exp_node->v,exp_node->t);
                ir::Operand des1(symbol_table.get_scoped_name(varName),ir::Type::Int);
                ir::Instruction* instruction1 = new ir::Instruction(op11,ir::Operand(),des1,ir::Operator::mov);
                buffer.push_back(instruction1);
                lval->t = exp_node->t;
                lval->v = exp_node->v;
            }
        }
        else if(lval->children.size() == 4)
        {
            ANALYSIS(exp,Exp,2);
            string varName = analysisIdent(dynamic_cast<Term*>(lval->children[0]));
            STE ident_ste = symbol_table.get_ste(varName);
            Exp* index = dynamic_cast<Exp*>(lval->children[2]);
            analysisExp(index,buffer);
            if (ident_ste.operand.type == Type::IntPtr)
            {
                if (exp->t == Type::Int || exp->t == Type::IntLiteral)
                {
                    ir::Operand op21(symbol_table.get_scoped_name(varName),ir::Type::IntPtr);
                    ir::Operand op22(index->v,index->t);
                    ir::Operand des2(exp->v,exp->t);
                    ir::Instruction* storeInst = new ir::Instruction(op21,op22,des2,ir::Operator::store);
                    buffer.push_back(storeInst);
                }
            }
        }
        else if(lval->children.size() == 7)
        {
            ANALYSIS(exp,Exp,2);
            string varName = analysisIdent(dynamic_cast<Term*>(lval->children[0]));
            STE ste = symbol_table.get_ste(varName);
            Exp* index1 = dynamic_cast<Exp*>(lval->children[2]);
            analysisExp(index1,buffer);
            Exp* index2 = dynamic_cast<Exp*>(lval->children[5]);
            analysisExp(index2,buffer);
            string tmp1 = "t" + std::to_string(tmp_cnt++);
            string tmp2 = "t" + std::to_string(tmp_cnt++);

            Instruction *def1 = new ir::Instruction({index1->v,index1->t},{},{tmp1,Type::Int},Operator::def);

            Instruction *def2 = new ir::Instruction(Operand(index2->v,index2->t),{},{tmp2,Type::Int},Operator::def);
            string tmp_col_len_name = "t" + std::to_string(tmp_cnt++);

            Instruction *def3 = new ir::Instruction({std::to_string(ste.dimension[1]), Type::IntLiteral},{},{tmp_col_len_name, Type::Int},ir::Operator::def);
            string temp3 = "t" + std::to_string(tmp_cnt++);

            Instruction *mulOffsetInst = new ir::Instruction({tmp1, Type::Int},{tmp_col_len_name, Type::Int},{temp3, Type::Int}, ir::Operator::mul);
            string tmp_totaloffset_name = "t" + std::to_string(tmp_cnt++);
            ir::Operand operand_e1(temp3, Type::Int);
            ir::Operand operand_e2(tmp2, Type::Int);
            ir::Operand operand_e3(tmp_totaloffset_name, Type::Int);
            Instruction *addOffsetInst = new ir::Instruction(operand_e1,operand_e2,operand_e3, ir::Operator::add);
            buffer.push_back(def1);
            buffer.push_back(def2);
            buffer.push_back(def3);
            buffer.push_back(mulOffsetInst);
            buffer.push_back(addOffsetInst);
            if (ste.operand.type == Type::IntPtr)
            {
                // Float or Int -> Int
                if (exp->t == Type::Int || exp->t == Type::IntLiteral)
                {

                    ir::Instruction* storeInst = new ir::Instruction({symbol_table.get_scoped_name(varName),ir::Type::IntPtr},
                                                                     {tmp_totaloffset_name,Type::Int},
                                                                     {exp->v,exp->t},ir::Operator::store);
                    buffer.push_back(storeInst);
                }
            }

        }
    }
    else if (dynamic_cast<Block*>(root->children[0]))
    {
        Block* block = dynamic_cast<Block*>(root->children[0]);
        analysisBlock(block,buffer,false);
    }
    else if (dynamic_cast<Exp*>(root->children[0]))
    {
        ANALYSIS(exp,Exp,0);
    }
    else if (term->token.type == TokenType::SEMICN)
        return;
    else if(term->token.type == TokenType::RETURNTK)
    {
        if (root->children.size() == 3)
        {
            ANALYSIS(expnode,Exp,1);
            if (curr_function->returnType == Type::Int)
            {
                // Int or IntLiteral
                if (expnode->t == Type::Int || expnode->t == Type::IntLiteral)
                {
                    ir::Instruction* retInst = new ir::Instruction({expnode->v,expnode->t}, {}, {},ir::Operator::_return);
                    buffer.push_back(retInst);
                }

            }


        }
            //无Exp
        else
        {
            ir::Instruction* retInst = new ir::Instruction({},
                                                           {},
                                                           {},ir::Operator::_return);
            buffer.push_back(retInst);
        }

    }
    else if (term->token.type == TokenType::IFTK)
    {

        Cond* cond = dynamic_cast<Cond*>(root->children[2]);
        vector<ir::Instruction*> nullbuffer;
        analysisCond(cond,nullbuffer);

        buffer.insert(buffer.end(),nullbuffer.begin(),nullbuffer.end());


        ir::Instruction* gto = new ir::Instruction({cond->v,cond->t},{},{"2",Type::IntLiteral},ir::Operator::_goto);
        buffer.push_back(gto);

        vector<ir::Instruction*> tmpInsts1;
        vector<ir::Instruction*> tmpInsts2;

        symbol_table.add_scope('i');
        Stmt *stmt1 = dynamic_cast<Stmt*> (root->children[4]);
        analysisStmt(stmt1, tmpInsts1);
        symbol_table.exit_scope();

        int ifInstNumber = tmpInsts1.size();

        int addInstNumber = ifInstNumber;
        //有else
        if (root->children.size() == 7)
        {
            symbol_table.add_scope('e');
            Stmt *stmt2 = dynamic_cast<Stmt*>(root->children[6]);
            analysisStmt(stmt2, tmpInsts2);
            symbol_table.exit_scope();
            int elseInstNumber = tmpInsts2.size();
            ir::Instruction* gotoIfLastInst = new ir::Instruction({}, {}, {std::to_string(elseInstNumber + 1), Type::IntLiteral}, ir::Operator::_goto);
            tmpInsts1.push_back(gotoIfLastInst);

            ir::Instruction* gotoElseInst = new ir::Instruction({}, {}, {std::to_string(tmpInsts1.size() + 1), Type::IntLiteral}, ir::Operator::_goto);
            buffer.push_back(gotoElseInst);
            buffer.insert(buffer.end(), tmpInsts1.begin(), tmpInsts1.end());
            buffer.insert(buffer.end(), tmpInsts2.begin(), tmpInsts2.end());
            Instruction* inst = new ir::Instruction({}, {}, {}, ir::Operator::__unuse__);
            buffer.push_back(inst);
        }
        else
        {
            ir::Operand operand_d(std::to_string(addInstNumber + 1), Type::IntLiteral);
            ir::Instruction* goto_else_Inst = new ir::Instruction(ir::Operand(),ir::Operand(),operand_d,ir::Operator::_goto);
            buffer.push_back(goto_else_Inst);
            buffer.insert(buffer.end(), tmpInsts1.begin(), tmpInsts1.end());
            Instruction* unuse_Inst = new ir::Instruction(ir::Operand(),ir::Operand(),ir::Operand(),ir::Operator::__unuse__);
            buffer.push_back(unuse_Inst);
        }
    }
    else if(term->token.type == TokenType::WHILETK)
    {
        Cond* cond = dynamic_cast<Cond*>(root->children[2]);
        vector<Instruction *> calInsts;
        analysisCond(cond, calInsts);
        symbol_table.add_scope('w');
        vector<Instruction *> stmtInsts;
        analysisStmt(dynamic_cast<Stmt*>(root->children[4]), stmtInsts);
        symbol_table.exit_scope();


        Instruction* gotoWhileInst = new ir::Instruction({cond->v, cond->t}, {}, {"2", Type::IntLiteral}, ir::Operator::_goto);
        Instruction* gotoReturnBeginMark = new ir::Instruction({"continue", Type::null}, {}, {}, ir::Operator::__unuse__);
        stmtInsts.push_back(gotoReturnBeginMark);
        Instruction* goto_exit_while_Inst = new ir::Instruction({}, {}, {std::to_string(stmtInsts.size() + 1), Type::IntLiteral},ir::Operator::_goto);

        for (int i = 0; i < stmtInsts.size(); i++)
        {
            if (stmtInsts[i]->op == Operator::__unuse__ && stmtInsts[i]->op1.type == Type::null)
            {
                if (stmtInsts[i]->op1.name == "break")
                {
                    Instruction *replaceBreakInst = new ir::Instruction({}, {}, {std::to_string(int(stmtInsts.size()) - i), Type::IntLiteral}, ir::Operator::_goto);
                    stmtInsts[i] = replaceBreakInst;
                }
                else if (stmtInsts[i]->op1.name == "continue")
                {
                    Instruction *replaceContinueInst = new ir::Instruction({}, {}, {std::to_string(-(2 + i + int(calInsts.size()))), Type::IntLiteral}, ir::Operator::_goto);
                    stmtInsts[i] = replaceContinueInst;
                }
            }
        }

        buffer.insert(buffer.end(), calInsts.begin(), calInsts.end());
        buffer.push_back(gotoWhileInst);
        buffer.push_back(goto_exit_while_Inst);
        buffer.insert(buffer.end(), stmtInsts.begin(), stmtInsts.end());

        Instruction * unuse_Inst = new ir::Instruction(ir::Operand(),ir::Operand(),ir::Operand(),ir::Operator::__unuse__);
        buffer.push_back(unuse_Inst);

    }
    else if (term->token.type == TokenType::BREAKTK)
    {
        Instruction *markBreakInst = new Instruction({"break", Type::null}, {}, {}, Operator::__unuse__);
        buffer.push_back(markBreakInst);
    }
    else if (term->token.type == TokenType::CONTINUETK)
    {
        Instruction *markContinueInst = new Instruction({"continue", Type::null}, {}, {}, Operator::__unuse__);
        buffer.push_back(markContinueInst);
    }
}

void frontend::Analyzer::analysisLVal(LVal* root, vector<ir::Instruction*>& buffer){
    if(root->children.size()==1)
    {
        Term * term = dynamic_cast<Term*> (root->children[0]);
        string varName = analysisIdent(term);
        STE std = symbol_table.get_ste(varName);
        root->t = std.operand.type;
        if (root->t == Type::IntLiteral)
        {
            root->v = std.number;
        }
        else
            root->v = symbol_table.get_scoped_name(varName);
    }
    else if(root->children.size()==4)
    {
        Term * term = dynamic_cast<Term*> (root->children[0]);
        string varName = analysisIdent(term);
        ANALYSIS(exp,Exp,2);
        string tmp = "t" + std::to_string(tmp_cnt++);
        root->v = tmp;

        ir::Operand curOperand = symbol_table.get_operand(varName);
        if(curOperand.type == ir::Type::IntPtr)
        {
            root->t = ir::Type::Int;
            ir::Instruction* loadInst = new ir::Instruction({symbol_table.get_scoped_name(varName), curOperand.type},
                                                            {exp->v,exp->t},{tmp,ir::Type::Int},ir::Operator::load);
            buffer.push_back(loadInst);
        }
    }
    else if(root->children.size()==7)
    {

        Term * term = dynamic_cast<Term*> (root->children[0]);
        string varName = analysisIdent(term);
        STE ste = symbol_table.get_ste(varName);
        Type targetType = Type::Int;
        ANALYSIS(exp1,Exp,2)
        ANALYSIS(exp2,Exp,5);
        string tmpDim1Name = "t" + std::to_string(tmp_cnt++);
        string tmpDim2Name = "t" + std::to_string(tmp_cnt++);
        Instruction *def1 = new ir::Instruction({exp1->v, exp1->t}, {}, {tmpDim1Name, Type::Int}, ir::Operator::def);
        Instruction *def2 = new ir::Instruction({exp2->v, exp2->t}, {}, {tmpDim2Name, Type::Int}, ir::Operator::def);
        string tmpColLenName = "t" + std::to_string(tmp_cnt++);
        Instruction *def3 = new ir::Instruction({std::to_string(ste.dimension[1]), Type::IntLiteral},
                                                {}, {tmpColLenName, Type::Int}, ir::Operator::def);
        string tmpLineoffsetName = "t" + std::to_string(tmp_cnt++);

        Instruction *mulOffsetInst = new ir::Instruction({tmpDim1Name, Type::Int}, {tmpColLenName, Type::Int},{tmpLineoffsetName, Type::Int}, ir::Operator::mul);
        string tmp_totaloffset_name = "t" + std::to_string(tmp_cnt++);

        Instruction *addOffsetInst = new ir::Instruction({tmpLineoffsetName, Type::Int}, {tmpDim2Name, Type::Int},
                                                         {tmp_totaloffset_name, Type::Int}, ir::Operator::add);
        // 取值
        string tmpLoadvalName = "t" + std::to_string(tmp_cnt++);

        Instruction *loadInst = new ir::Instruction({symbol_table.get_scoped_name(varName), ste.operand.type},{tmp_totaloffset_name, Type::Int},{tmpLoadvalName, targetType}, ir::Operator::load);
        buffer.push_back(def1);
        buffer.push_back(def2);
        buffer.push_back(def3);
        buffer.push_back(mulOffsetInst);
        buffer.push_back(addOffsetInst);
        buffer.push_back(loadInst);
        root->v = tmpLoadvalName;
        root->t = targetType;
    }
}

void frontend::Analyzer::analysisExp(Exp* root,vector<ir::Instruction*>& buffer){
    ANALYSIS(add,AddExp,0);
    COPY_EXP_NODE(add, root);
}

void comparaType(ir::Type oldType, ir::Type & newType, std::string & var, vector<ir::Instruction*>& buffer, int & tmp_cnt)
{
    if (oldType != newType)
    {
        if (oldType == Type::Int)
        {
            assert(newType == Type::IntLiteral);
            string tmpIntcvtName = "t" + std::to_string(tmp_cnt++);

            Instruction * cvtInst = new Instruction({var, ir::Type::IntLiteral},{}, {tmpIntcvtName, ir::Type::Int},ir::Operator::def);
            buffer.push_back(cvtInst);
            var = tmpIntcvtName;
            newType = Type::Int;
        }
        else
            assert(0 && "Error");
    }
}

void alterType(ir::Type& oldType, ir::Type & newType)
{
    if (newType == ir::Type::Float)
        oldType = ir::Type::Float;
    else if (newType == ir::Type::FloatLiteral && oldType == ir::Type::IntLiteral)
        oldType = ir::Type::FloatLiteral;
    else if (newType == ir::Type::Int && oldType == ir::Type::IntLiteral)
        oldType = ir::Type::Int;
    else if ((newType == ir::Type::FloatLiteral && oldType == ir::Type::Int) || (oldType == ir::Type::FloatLiteral && newType == ir::Type::Int)) // 提升没有顺序
        oldType = ir::Type::Float;

}
void frontend::Analyzer::analysisAddExp(AddExp* root,vector<ir::Instruction*>& buffer){

    if(root->children.size()==1)
    {
        ANALYSIS(mulexp_node,MulExp,0);
        COPY_EXP_NODE(mulexp_node, root);
    }
    else
    {
        Type oldType = Type::IntLiteral;
        for (int i = 0; i < root->children.size(); i += 2)
        {
            ANALYSIS(mulexp,MulExp,i);
            alterType(oldType, mulexp->t);
        }

        MulExp* firstMulExp = dynamic_cast<MulExp*>(root->children[0]);
        assert(firstMulExp);
        root->t = firstMulExp->t;
        root->v = firstMulExp->v;

        comparaType(oldType, root->t, root->v, buffer, tmp_cnt);

        for (int i = 2; i < root->children.size(); i += 2)
        {
            MulExp *mulexp = dynamic_cast<MulExp *>(root->children[i]);

            Term* curOp  = dynamic_cast<Term*>(root->children[i - 1]);

            comparaType(oldType, mulexp->t, mulexp->v, buffer, tmp_cnt);

            if (oldType == Type::IntLiteral)
            {
                int val1 = std::stoi(root->v);
                int val2 = std::stoi(mulexp->v);
                if (curOp->token.type == TokenType::PLUS)
                    root->v = std::to_string(val1+val2);
                else if (curOp->token.type == TokenType::MINU)
                    root->v = std::to_string(val1-val2);
                else
                    assert(0 && "Invalid Op");
            }
            else if (oldType == Type::Int)
            {
                string tmpCalName = "t" + std::to_string(tmp_cnt++);
                Instruction * calInst;
                ir::Operator anOperator;
                if(curOp->token.type == TokenType::PLUS)
                    anOperator = ir::Operator::add;
                else if (curOp->token.type == TokenType::MINU)
                    anOperator = ir::Operator::sub;

                calInst = new Instruction({root->v,ir::Type::Int}, {mulexp->v,ir::Type::Int}, {tmpCalName, ir::Type::Int}, anOperator);
                buffer.push_back(calInst);
                root->v = tmpCalName;
            }
            else
                assert(0 && "Error");
        }
    }

}




void frontend::Analyzer::analysisMulExp(MulExp* root, vector<ir::Instruction*>& buffer){
    if(root->children.size()==1)
    {
        ANALYSIS(unaryexp_node,UnaryExp,0);
        COPY_EXP_NODE(unaryexp_node, root);
    }
    else
    {
        Type oldType = Type::IntLiteral;
        for (int i = 0; i < root->children.size(); i += 2)
        {
            ANALYSIS(unaryexp,UnaryExp,i);
            //类型提升
            alterType(oldType, unaryexp->t);
        }
        UnaryExp* pUnaryExp1 = dynamic_cast<UnaryExp*>(root->children[0]);
        assert(pUnaryExp1);
        root->t = pUnaryExp1->t;
        root->v = pUnaryExp1->v;

        comparaType(oldType, root->t, root->v, buffer, tmp_cnt);

        for (int i = 2; i < root->children.size(); i += 2)
        {
            // 注意，此时root->t已为正确的类型
            UnaryExp *unaryexp = dynamic_cast<UnaryExp *>(root->children[i]);

            // 符号
            Term* curOp  = dynamic_cast<Term*>(root->children[i - 1]);

            comparaType(oldType, unaryexp->t, unaryexp->v, buffer, tmp_cnt);
            // 已经化为相同类型
            // 开始计算
            if (oldType == Type::IntLiteral)
            {
                int val1 = std::stoi(root->v);
                int val2 = std::stoi(unaryexp->v);
                if (curOp->token.type == TokenType::MULT)
                    root->v = std::to_string(val1*val2);
                else if (curOp->token.type == TokenType::MOD)
                    root->v = std::to_string(val1%val2);
                else if (curOp->token.type == TokenType::DIV)
                    root->v = std::to_string(val1/val2);
                else
                    assert(0 && "Invalid Op");
            }
            else if (oldType == Type::Int)
            {
                string tmpCalName = "t" + std::to_string(tmp_cnt++);
                Instruction * calInst;
                ir::Operator cur_operator;
                if (curOp->token.type == TokenType::MULT)
                    cur_operator = ir::Operator::mul;
                else if (curOp->token.type == TokenType::MOD)
                    cur_operator = ir::Operator::mod;
                else if (curOp->token.type == TokenType::DIV)
                    cur_operator = ir::Operator::div;

                calInst = new Instruction({root->v,ir::Type::Int}, {unaryexp->v,ir::Type::Int}, {tmpCalName, ir::Type::Int}, cur_operator);
                buffer.push_back(calInst);
                root->v = tmpCalName;
            }
            else
                assert(0 && "Error");
        }
    }
}

void frontend::Analyzer::analysisUnaryExp(UnaryExp* root, vector<ir::Instruction*>& buffer){

    if(dynamic_cast<PrimaryExp *>(root->children[0]))
    {
        ANALYSIS(primaryexp_node,PrimaryExp,0);
        COPY_EXP_NODE(primaryexp_node, root);
    }
    else if(dynamic_cast<Term *>(root->children[0]))
    {
        Term *term = dynamic_cast<Term *>(root->children[0]);
        string func_name = term->token.value;
        vector<Operand> paraVec;
        FuncRParams *pFuncRParams = dynamic_cast<FuncRParams *>(root->children[2]);
        Function* call_func = symbol_table.functions[func_name];
        vector<Operand> funcParaType = call_func->ParameterList;
        //无参
        if(pFuncRParams != nullptr)
        {
            vector<Operand> func_para_type = call_func->ParameterList;
            for (int i = 0, cnt = 0; i < pFuncRParams->children.size(); i += 2, cnt += 1)
            {
                Exp *exp = dynamic_cast<Exp *>(pFuncRParams->children[i]);
                analysisExp(exp,buffer);
                paraVec.push_back(Operand(exp->v, exp->t));
            }
        }
        string tmpFuncResultName = "t" + std::to_string(tmp_cnt++);

        ir::CallInst* callInst = new ir::CallInst({call_func->name,call_func->returnType},paraVec,{tmpFuncResultName, call_func->returnType});
        buffer.push_back(callInst);
        root->v = tmpFuncResultName;
        root->t = call_func->returnType;
    }
    else if(dynamic_cast<UnaryOp *>(root->children[0]))
    {
        UnaryOp *unaryop = dynamic_cast<UnaryOp *>(root->children[0]);
        UnaryExp *unaryexp = dynamic_cast<UnaryExp *>(root->children[1]);
        analysisUnaryExp(unaryexp,buffer);
        Term *unaryopTerm = dynamic_cast<Term *>(unaryop->children[0]);
        if (unaryopTerm->token.type == TokenType::PLUS)
        {
            root->v = unaryexp->v;
            root->t = unaryexp->t;
        }
        else if(unaryopTerm->token.type == TokenType::MINU)
        {
            if (unaryexp->t == Type::IntLiteral)
            {
                int tmpResult = -std::stoi(unaryexp->v);
                root->v = std::to_string(tmpResult);
                root->t = Type::IntLiteral;
            }
            else if (unaryexp->t == Type::Int)
            {

                string zeroName = "t" + std::to_string(tmp_cnt++);
                ir::Instruction* defInst = new ir::Instruction({"0",ir::Type::IntLiteral}, {}, {zeroName, ir::Type::Int}, ir::Operator::def);
                string minuName = "t" + std::to_string(tmp_cnt++);

                ir::Instruction* minuInst = new ir::Instruction({zeroName, ir::Type::Int},
                                                                {unaryexp->v,ir::Type::Int}, {minuName, ir::Type::Int}, ir::Operator::sub);
                buffer.push_back(defInst);
                buffer.push_back(minuInst);
                root->v = minuName;
                root->t = Type::Int;
            }
        }
        else if (unaryopTerm->token.type == TokenType::NOT)
        {
            if (unaryexp->t == Type::IntLiteral)
            {
                int tmpResult = !std::stoi(unaryexp->v);
                root->v = std::to_string(tmpResult);
                root->t = Type::IntLiteral;
            }
            else if (unaryexp->t == Type::Int)
            {
                // 使用not进行转换
                string tmpNotName = "t" + std::to_string(tmp_cnt++);
                ir::Instruction* notInst = new ir::Instruction({unaryexp->v,ir::Type::Int}, {}, {tmpNotName, ir::Type::Int}, ir::Operator::_not);
                buffer.push_back(notInst);
                root->v = tmpNotName;
                root->t = Type::Int;
            }
            else
                assert(0 && "Error");
        }
    }

}

void frontend::Analyzer::analysisPrimaryExp(PrimaryExp*root, vector<ir::Instruction*>& buffer){
    if(dynamic_cast<Number *>(root->children[0])!=nullptr)
    {
        ANALYSIS(number_node,Number,0);
        COPY_EXP_NODE(number_node, root);
    }
    else if(dynamic_cast<LVal *>(root->children[0])!=nullptr)
    {
        ANALYSIS(lval_node,LVal,0);
        COPY_EXP_NODE(lval_node, root);
    }
    else if(root->children.size() == 3)
    {
        ANALYSIS(exp,Exp,1);
        root->v = exp->v;
        root->t = exp->t;
    }

}
std::string getnumber(string numberT){
    if (numberT[0] == '0' && (numberT[1] == 'x' || numberT[1] == 'X'))
    {
        return std::to_string(std::stoi(numberT, nullptr, 16));
    }
    else if (numberT[0] == '0')
    {
        return std::to_string(std::stoi(numberT, nullptr, 8));
    }
    else if (numberT[0] == '0' && (numberT[1] == 'b' || numberT[1] == 'B'))
    {
        return std::to_string(std::stoi(numberT.substr(2), nullptr, 2));
    }
    else
        return numberT;
}
void frontend::Analyzer::analysisNumber(Number* root, vector<ir::Instruction*>& buffer){
    Term *term = dynamic_cast<Term *>(root->children[0]);
    root->t = Type::IntLiteral;
    string &numberT = term->token.value;
    root->v= getnumber(numberT);
}

void frontend::Analyzer::analysisCond(Cond* root, vector<ir::Instruction*>& buffer){
    ANALYSIS(lorexp,LOrExp,0);
    root->v = lorexp->v;
    root->t = lorexp->t;
}


void frontend::Analyzer::analysisRelExp(RelExp*root, vector<ir::Instruction*>& buffer){
    if(root->children.size()==1)
    {
        ANALYSIS(addexp_node,AddExp,0);
        COPY_EXP_NODE(addexp_node, root);
    }
    else
    {
        ANALYSIS(firstAddExp,AddExp,0);
        root->t = firstAddExp->t;
        root->v = firstAddExp->v;

        for (int i = 2; i < root->children.size(); i += 2)
        {
            ANALYSIS(addexp,AddExp,i);
            Term* curOp  = dynamic_cast<Term*>(root->children[i - 1]);
            Type ol = root->t;

            if (root->t != addexp->t)
            {alterType(ol, addexp->t);
                if (ol == Type::Int) // IntLiteral -> Int
                {
                    if (addexp->t == Type::IntLiteral)
                    {
                        string tmpIntcvtName = "t" + std::to_string(tmp_cnt++);
                        Instruction * cvtInst = new Instruction({addexp->v,ir::Type::IntLiteral}, {},{tmpIntcvtName, ir::Type::Int},ir::Operator::def);
                        buffer.push_back(cvtInst);
                        addexp->v = tmpIntcvtName;
                        addexp->t = Type::Int;
                    }
                    if (root->t == Type::IntLiteral)
                    {
                        string tmpIntcvtName = "t" + std::to_string(tmp_cnt++);

                        Instruction * cvtInst = new Instruction({root->v,ir::Type::IntLiteral}, {}, {tmpIntcvtName, ir::Type::Int}, ir::Operator::def);
                        buffer.push_back(cvtInst);
                        root->v = tmpIntcvtName;
                        root->t = Type::Int;
                    }
                }
            }

            if (ol == Type::IntLiteral)
            {
                int val1 = std::stoi(root->v);
                int val2 = std::stoi(addexp->v);
                if (curOp->token.type == TokenType::LSS)
                    root->v = std::to_string(val1 < val2);
                else if (curOp->token.type == TokenType::GTR)
                    root->v = std::to_string(val1 > val2);
                else if (curOp->token.type == TokenType::LEQ)
                    root->v = std::to_string(val1 <= val2);
                else if (curOp->token.type == TokenType::GEQ)
                    root->v = std::to_string(val1 >= val2);
                else
                    assert(0 && "Invalid Op");
            }
            else if (ol == Type::Int)
            {
                string tmp_cal_name = "t" + std::to_string(tmp_cnt++);
                Instruction * calInst;
                ir::Operator cur_operator;
                if (curOp->token.type == TokenType::LSS)
                    cur_operator = ir::Operator::lss;
                else if (curOp->token.type == TokenType::GTR)
                    cur_operator = ir::Operator::gtr;
                else if (curOp->token.type == TokenType::LEQ)
                    cur_operator = ir::Operator::leq;
                else if (curOp->token.type == TokenType::GEQ)
                    cur_operator = ir::Operator::geq;
                calInst = new Instruction({root->v,ir::Type::Int},{addexp->v,ir::Type::Int},{tmp_cal_name,ir::Type::Int},cur_operator);
                buffer.push_back(calInst);
                root->v = tmp_cal_name;
                root->t = Type::Int;
            }
        }
    }
}
void frontend::Analyzer::analysisEqExp(EqExp* root, vector<ir::Instruction*>& buffer){
    if(root->children.size()==1)
    {
        ANALYSIS(relexp_node,RelExp,0);
        COPY_EXP_NODE(relexp_node, root);
    }
    else
    {
        ANALYSIS(firstRelExp,RelExp,0)
        root->t = firstRelExp->t;
        root->v = firstRelExp->v;

        for (int i = 2; i < root->children.size(); i += 2)
        {
            ANALYSIS(relexp,RelExp,i);
            Term* curOp  = dynamic_cast<Term*>(root->children[i - 1]);
            Type oldType = root->t;

            if (root->t != relexp->t)
            {
                alterType(oldType, relexp->t);
                if (oldType == Type::Int) // IntLiteral -> Int
                {
                    if (relexp->t == Type::IntLiteral)
                    {
                        string tmpIntcvtName = "t" + std::to_string(tmp_cnt++);

                        Instruction * cvtInst = new Instruction({relexp->v,ir::Type::IntLiteral},{relexp->v,ir::Type::IntLiteral},
                                                                {tmpIntcvtName, ir::Type::Int},ir::Operator::def);
                        buffer.push_back(cvtInst);
                        relexp->v = tmpIntcvtName;
                        relexp->t = Type::Int;
                    }
                }
            }
            if (oldType == Type::IntLiteral)
            {
                int val1 = std::stoi(root->v);
                int val2 = std::stoi(relexp->v);
                if (curOp->token.type == TokenType::EQL)
                    root->v = std::to_string(val1 == val2);
                else if (curOp->token.type == TokenType::NEQ)
                    root->v = std::to_string(val1 != val2);
                else
                    assert(0 && "Invalid Op");
            }
            else if (oldType == Type::Int)
            {
                string tmpCalName = "t" + std::to_string(tmp_cnt++);
                ir::Operator cur_operator;
                Instruction * calInst;
                if (curOp->token.type == TokenType::EQL)
                    cur_operator = ir::Operator::eq;
                else if (curOp->token.type == TokenType::NEQ)
                    cur_operator = ir::Operator::neq;


                calInst = new Instruction({root->v,ir::Type::Int}, {relexp->v,ir::Type::Int}, {tmpCalName, ir::Type::Int}, cur_operator);
                buffer.push_back(calInst);
                root->v = tmpCalName;
                root->t = Type::Int;
            }
        }
    }
}
void frontend::Analyzer::analysisLAndExp(LAndExp*root, vector<ir::Instruction*>&buffer){
    ANALYSIS(eqexp,EqExp,0);
    root->v = eqexp->v;
    root->t = eqexp->t;
    if (root->children.size() == 3)
    {
        LAndExp* landexp = dynamic_cast<LAndExp*>(root->children[2]);
        vector<ir::Instruction *> cal_insts;
        analysisLAndExp(landexp,cal_insts);
        int trueNumber = 0;
        if (root->t == Type::IntLiteral && landexp->t == Type::IntLiteral)
        {
            root->v = std::to_string(std::stoi(root->v) && std::stoi(landexp->v));
        }
        else
        {
            string tmpCalName = "t" + std::to_string(tmp_cnt++);
            Instruction * root_true_goto = new Instruction({root->v,root->t},{}, {"2",Type::IntLiteral},ir::Operator::_goto);
            buffer.push_back(root_true_goto);

            vector<Instruction *> trueLogicInst = cal_insts;

            Instruction * calInst = new Instruction({root->v,root->t}, {landexp->v,landexp->t}, {tmpCalName, ir::Type::Int},ir::Operator::_and);
            trueLogicInst.push_back(calInst);
            trueNumber ++;

            Instruction * trueLogicGoto = new Instruction({}, {}, {"2", Type::IntLiteral}, ir::Operator::_goto);
            trueLogicInst.push_back(trueLogicGoto);
            trueNumber ++;
            Instruction * rootFalseGoto = new Instruction({}, {}, {std::to_string(trueLogicInst.size() + 1), Type::IntLiteral}, ir::Operator::_goto);
            Instruction * root_false_assign = new Instruction({"0",Type::IntLiteral},{}, {tmpCalName, ir::Type::Int},ir::Operator::mov);

            buffer.push_back(rootFalseGoto);
            buffer.insert(buffer.end(), trueLogicInst.begin(), trueLogicInst.end());
            buffer.push_back(root_false_assign);
            root->v = tmpCalName;
            root->t = Type::Int;
        }
    }
}
void frontend::Analyzer::analysisLOrExp(LOrExp* root, vector<ir::Instruction*>& buffer){
    ANALYSIS(landexp,LAndExp,0);
    root->v = landexp->v;
    root->t = landexp->t;
    if (root->children.size() == 3)
    {
        LOrExp* lorexp = dynamic_cast<LOrExp*>(root->children[2]);
        vector<Instruction *> tmpInst;
        analysisLOrExp(lorexp, tmpInst);
        if (root->t == Type::IntLiteral && lorexp->t == Type::IntLiteral)
        {
            root->v = std::to_string(std::stoi(root->v) || std::stoi(lorexp->v));
        }
        else{
            string tmp_cal_name = "t" + std::to_string(tmp_cnt++);

            Instruction * pInstruction = new Instruction({root->v, root->t}, {}, {"2", Type::IntLiteral}, ir::Operator::_goto);


            Instruction * rootTrueAssign = new Instruction({"1", Type::IntLiteral}, {}, {tmp_cal_name, ir::Type::Int}, ir::Operator::mov);
            Instruction * rootFalseGoto = new Instruction({}, {}, {"3", Type::IntLiteral}, ir::Operator::_goto);


            Instruction * calInst = new Instruction({root->v,root->t}, {lorexp->v,lorexp->t}, {tmp_cal_name,ir::Type::Int},ir::Operator::_or);
            tmpInst.push_back(calInst);
            Instruction * trueLogicGoto = new Instruction({}, {}, {std::to_string(tmpInst.size() + 1), Type::IntLiteral}, ir::Operator::_goto);
            buffer.push_back(pInstruction);
            buffer.push_back(rootFalseGoto);
            buffer.push_back(rootTrueAssign);
            buffer.push_back(trueLogicGoto);
            buffer.insert(buffer.end(), tmpInst.begin(), tmpInst.end());
            root->v = tmp_cal_name;
            root->t = Type::Int;
        }
    }
}

void frontend::Analyzer::analysisConstExp(ConstExp* root, vector<ir::Instruction*>& buffer){
    ANALYSIS(addexp,AddExp,0);
    root->v = addexp->v;
    root->t = addexp->t;
}