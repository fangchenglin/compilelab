#include"front/syntax.h"

#include<iostream>
#include<cassert>

using frontend::Parser;

//#define DEBUG_PARSER
#define TODO assert(0 && "todo")
#define CUR_TOKEN_IS(tk_type) (token_stream[index].type == TokenType::tk_type)
#define PARSE_TOKEN(tk_type) root->children.push_back(parseTerm(root, TokenType::tk_type))
#define PARSE(name, type) auto name = new type(root); assert(parse##type(name)); root->children.push_back(name);


Parser::Parser(const std::vector<frontend::Token>& tokens): index(0), token_stream(tokens) {}

Parser::~Parser() {}

frontend::CompUnit* Parser::get_abstract_syntax_tree(){

    auto* root = new CompUnit();
    parseCompUnit(root);
    return  root;
}

void Parser::log(AstNode* node){
#ifdef DEBUG_PARSER
    std::cout << "in parse" << toString(node->type) << ", cur_token_type::" << toString(token_stream[index].type) << ", token_val::" << token_stream[index].value << '\n';
#endif
}
frontend:: Term* Parser:: parseTerm(frontend:: AstNode* root, frontend:: TokenType tk_type){

    if(token_stream[index].type==tk_type){
        Term* term=new Term(token_stream[index],root);
        log(term);
        index++;

        return term;
    }
    return nullptr;
}
bool frontend::Parser::parseCompUnit(frontend::CompUnit * root){
    //CompUnit → [ CompUnit ] ( Decl | FuncDef )
    //func
    log(root);
    if((CUR_TOKEN_IS(INTTK)||
        CUR_TOKEN_IS(FLOATTK) ||
        CUR_TOKEN_IS(VOIDTK))&&
       token_stream[index+1].type==TokenType::IDENFR&&
       token_stream[index+2].type==TokenType::LPARENT){
        PARSE(node,FuncDef);

    } else if(CUR_TOKEN_IS(CONSTTK)||
              CUR_TOKEN_IS(INTTK)||
              CUR_TOKEN_IS(FLOATTK)){
//        Decl *node = new frontend::Decl();
//        if(ParserDecl(node)){
//            root->children.push_back(node);
//        }

        PARSE(node,Decl);
    } else{
        return false;
    }
    if(index<token_stream.size()&&
       (CUR_TOKEN_IS(CONSTTK)||
        CUR_TOKEN_IS(INTTK)||
        CUR_TOKEN_IS(FLOATTK)||
        CUR_TOKEN_IS(VOIDTK)
       )
            ){
        PARSE(node,CompUnit);
    }
    return true;
}
// FuncDef -> FuncType Ident '(' [FuncFParams] ')' Block
bool frontend::Parser::parseFuncDef(frontend::FuncDef* root) {log(root);
    PARSE(funcType, FuncType);

    if(!CUR_TOKEN_IS(IDENFR)) return false;

    PARSE_TOKEN(IDENFR);

    if (CUR_TOKEN_IS(LPARENT)) {
        PARSE_TOKEN(LPARENT);
    } else{
        return false;
    }
    while (!CUR_TOKEN_IS(RPARENT)){
        PARSE(fParamsNode,FuncFParams);
    }
    PARSE_TOKEN(RPARENT);


    PARSE(block, Block);

    return true;
}



bool frontend::Parser::parseDecl(frontend::Decl *root) {log(root);
    //Decl -> ConstDecl | VarDecl
    if(CUR_TOKEN_IS(CONSTTK)){
        PARSE(node,ConstDecl)
    }else{
        PARSE(node,VarDecl);
    }

    return true;
}

bool frontend::Parser::parseConstDecl(frontend::ConstDecl *root) {log(root);
    if(!CUR_TOKEN_IS(CONSTTK)) return false;
    PARSE_TOKEN(CONSTTK);
    PARSE(BTypenode,BType);
    PARSE(ConstDefnode,ConstDef);
    while (CUR_TOKEN_IS(COMMA)){
        PARSE_TOKEN(COMMA);
        PARSE(ConstDefnode1,ConstDef);
    }
    PARSE_TOKEN(SEMICN);
    return true;
}
//变量声明 VarDecl → BType VarDef { ',' VarDef } ';'
bool frontend::Parser::parseVarDecl(frontend::VarDecl* root) {log(root);
    PARSE(btype, BType);

    PARSE(vardef, VarDef);

    while (CUR_TOKEN_IS(COMMA)) {
        PARSE_TOKEN(COMMA);
        PARSE(vardef1, VarDef);
    }

    PARSE_TOKEN(SEMICN);

    return true;
}


bool frontend::Parser::parseBType(frontend::BType *root) {log(root);
    if (CUR_TOKEN_IS(INTTK)) {
        PARSE_TOKEN(INTTK);
    } else if (CUR_TOKEN_IS(FLOATTK)) {
        PARSE_TOKEN(FLOATTK);
    }

    return true;
}

bool frontend::Parser::parseConstDef(frontend::ConstDef *root) {log(root);
    if (!CUR_TOKEN_IS(IDENFR)) return false;
    PARSE_TOKEN(IDENFR);

    while (CUR_TOKEN_IS(LBRACK)) {
        PARSE_TOKEN(LBRACK);
        PARSE(ConstExpnode, ConstExp);
        PARSE_TOKEN(RBRACK);
    }

    PARSE_TOKEN(ASSIGN);
    PARSE(ConstInitValnode, ConstInitVal);

    return true;
}

bool frontend::Parser::parseConstExp(frontend::ConstExp *root) {log(root);
    PARSE(addexpnode,AddExp);
    return true;
}

bool frontend::Parser::parseConstInitVal(frontend::ConstInitVal *root) {log(root);
    if (CUR_TOKEN_IS(LBRACE)) {
        PARSE_TOKEN(LBRACE);

        if (CUR_TOKEN_IS(RBRACE)) {
            PARSE_TOKEN(RBRACE);
            return true;
        }

        while (true) {
            PARSE(ConstInitValnode, ConstInitVal);

            if (CUR_TOKEN_IS(RBRACE)) {
                PARSE_TOKEN(RBRACE);
                return true;
            }

            if (!CUR_TOKEN_IS(COMMA)) {
                return false;  // 不符合预期的终结符类型
            }

            PARSE_TOKEN(COMMA);
        }
    } else {
        PARSE(ConstExpnode, ConstExp);
        return true;
    }

    return false;  // 不符合预期的终结符类型
}

bool frontend::Parser::parseVarDef(frontend::VarDef *root) {log(root);
    PARSE_TOKEN(IDENFR);

    while (CUR_TOKEN_IS(LBRACK)){

        PARSE_TOKEN(LBRACK);

        PARSE(constexp, ConstExp);

        PARSE_TOKEN(RBRACK);
    }

    if (CUR_TOKEN_IS(ASSIGN)){

        PARSE_TOKEN(ASSIGN);

        PARSE(initval, InitVal);
    }

    return true;}

bool frontend::Parser::parseInitVal(frontend::InitVal* root) {log(root);
    if (CUR_TOKEN_IS(LBRACE)) {
        PARSE_TOKEN(LBRACE);

        if (CUR_TOKEN_IS(RBRACE)) {
            PARSE_TOKEN(RBRACE);
            return true;
        }

        while (true) {
            PARSE(InitValNode, InitVal);

            if (!CUR_TOKEN_IS(COMMA)) {
                break;  // 不符合预期的终结符类型，跳出循环
            }

            PARSE_TOKEN(COMMA);
        }

        if (CUR_TOKEN_IS(RBRACE)) {
            PARSE_TOKEN(RBRACE);
            return true;
        }
    } else {
        PARSE(ExpNode, Exp);
        return true;
    }

    return false;  // 不符合预期的终结符类型
}

bool frontend::Parser::parseExp(frontend::Exp *root) {log(root);
    PARSE(addexp, AddExp);

    return true;
}
//FuncType -> 'void' | 'int' | 'float'
bool frontend::Parser::parseFuncType(frontend::FuncType *root) {log(root);
    if(CUR_TOKEN_IS(VOIDTK)){
        PARSE_TOKEN(VOIDTK);

    } else if(CUR_TOKEN_IS(INTTK)){
        PARSE_TOKEN(INTTK);

    } else if(CUR_TOKEN_IS(FLOATTK)){
        PARSE_TOKEN(FLOATTK);
    }
    return true;
}
//FuncFParams -> FuncFParam { ',' FuncFParam }
bool frontend::Parser::parseFuncFParams(frontend::FuncFParams *root) {log(root);
    PARSE(FuncFParamnode,FuncFParam);
    while (CUR_TOKEN_IS(COMMA)){
        PARSE_TOKEN(COMMA);
        PARSE(Funfnode,FuncFParam);
    }
    return true;

}
//Block -> '{' { BlockItem } '}'
bool frontend::Parser::parseBlock(frontend::Block *root) {log(root);

    PARSE_TOKEN(LBRACE);
    while (!CUR_TOKEN_IS(RBRACE)){
        PARSE(Blockitemnode,BlockItem);
    }
    if(!CUR_TOKEN_IS(RBRACE)) return false;
    PARSE_TOKEN(RBRACE);
    return true;
}
// FuncFParam -> BType Ident ['[' ']' { '[' Exp ']' }]

bool frontend::Parser::parseFuncFParam(frontend::FuncFParam *root) {log(root);

    PARSE(BTypenode,BType);
    PARSE_TOKEN(IDENFR);
    if(CUR_TOKEN_IS(LBRACK)){
        PARSE_TOKEN(LBRACK);
        if(CUR_TOKEN_IS(RBRACK)){
            PARSE_TOKEN(RBRACK);
        }else{ return false;}
        while (true){
            if(CUR_TOKEN_IS(LBRACK)){
                PARSE_TOKEN(LBRACK);
                PARSE(Expnode,Exp);
                PARSE_TOKEN(RBRACK);
            } else{
                break;
            }
        }
    }
    return true;
}
//BlockItem -> Decl | Stmt
bool frontend::Parser::parseBlockItem(frontend::BlockItem *root) {log(root);
    //const|int|float
    if(CUR_TOKEN_IS(CONSTTK)||
       CUR_TOKEN_IS(INTTK)||
       CUR_TOKEN_IS(FLOATTK)
            ){
        PARSE(declnode,Decl);
        return true;
    } else{
        PARSE(stmt,Stmt);
        return true;
    }
}
//Stmt -> LVal '=' Exp ';' | Block | 'if' '(' Cond ')' Stmt [ 'else' Stmt ] | 'while' '(' Cond ')' Stmt | 'break' ';' | 'continue' ';' | 'return' [Exp] ';' | [Exp] ';'
bool frontend::Parser::parseStmt(frontend::Stmt *root) {log(root);
    if(CUR_TOKEN_IS(LBRACE)){//Block
        PARSE(Blocknode,Block);
        return true;
    } else if(CUR_TOKEN_IS(IFTK)){ //if else
        PARSE_TOKEN(IFTK);
        if(!CUR_TOKEN_IS(LPARENT)) return false;
        PARSE_TOKEN(LPARENT);
        PARSE(cond,Cond);
        if(!CUR_TOKEN_IS(RPARENT)) return false;
        PARSE_TOKEN(RPARENT);
        PARSE(stmtnode1,Stmt);
        if(CUR_TOKEN_IS(ELSETK)){
            PARSE_TOKEN(ELSETK);
            PARSE(stmtnode2,Stmt);
        }
        return true;
    }else if(CUR_TOKEN_IS(WHILETK)) {//while(cond) {}
        PARSE_TOKEN(WHILETK);
        if(!CUR_TOKEN_IS(LPARENT)) return false;
        PARSE_TOKEN(LPARENT);
        PARSE(cond1,Cond);
        if(!CUR_TOKEN_IS(RPARENT)) return false;
        PARSE_TOKEN(RPARENT);
        PARSE(stmtnode3,Stmt);

    }else if(CUR_TOKEN_IS(BREAKTK)){//Break;
        PARSE_TOKEN(BREAKTK);

        PARSE_TOKEN(SEMICN);
    }else if(CUR_TOKEN_IS(CONTINUETK)){//continue;
        PARSE_TOKEN(CONTINUETK);

        PARSE_TOKEN(SEMICN);
    } else if(CUR_TOKEN_IS(RETURNTK)){
        PARSE_TOKEN(RETURNTK);
        if(!CUR_TOKEN_IS(SEMICN)){
            PARSE(EXPnode, Exp);
        }
        PARSE_TOKEN(SEMICN);
    } else if(CUR_TOKEN_IS(IDENFR)&&
              (token_stream[index+1].type==TokenType::ASSIGN||
               token_stream[index+1].type==TokenType::LBRACK)){
        PARSE(LValnode,LVal);
        PARSE_TOKEN(ASSIGN);
        PARSE(Expnode1,Exp);
        PARSE_TOKEN(SEMICN);
    } else {
        if(CUR_TOKEN_IS(SEMICN)){
            PARSE_TOKEN(SEMICN);
        } else{
            PARSE(expnode,Exp);
            PARSE_TOKEN(SEMICN);
        }
    }
    //assert(0 &&"parseStmt");
    return true;
}

bool frontend::Parser::parseCond(frontend::Cond *root) {log(root);
    PARSE(LOrExpnode,LOrExp);
    return true;
}

bool frontend::Parser::parseLVal(frontend::LVal *root) {log(root);
    PARSE_TOKEN(IDENFR);
    while (CUR_TOKEN_IS(LBRACK)){
        PARSE_TOKEN(LBRACK);
        PARSE(expnode,Exp);
        PARSE_TOKEN(RBRACK);
    }
    return true;
}
//AddExp→MulExp |AddExp ('+' | '−') MulExp
bool frontend::Parser::parseAddExp(frontend::AddExp *root) {log(root);
    PARSE(Mulexpnode1,MulExp);
    while (CUR_TOKEN_IS(PLUS)|| CUR_TOKEN_IS(MINU)){
        if(CUR_TOKEN_IS(PLUS)){
            PARSE_TOKEN(PLUS);

        }else if(CUR_TOKEN_IS(MINU)){
            PARSE_TOKEN(MINU);
        }
        PARSE(Mulexpnode2,MulExp);
    }
    return true;
}
//LOrExp -> LAndExp [ '||' LOrExp ]
bool frontend::Parser::parseLOrExp(frontend::LOrExp *root) {log(root);
    PARSE(Landexpnode,LAndExp);
    if(CUR_TOKEN_IS(OR)){
        PARSE_TOKEN(OR);
        PARSE(lornode,LOrExp);
    }
    return true;
}
//Number -> IntConst | floatCons
bool frontend::Parser::parseNumber(frontend::Number *root) {log(root);
    if(CUR_TOKEN_IS(INTLTR)){
        PARSE_TOKEN(INTLTR);
        return true;
    } else if(CUR_TOKEN_IS(FLOATLTR)){
        PARSE_TOKEN(FLOATLTR);
        return true;
    }
    return false;
}
//PrimaryExp -> '(' Exp ')' | LVal | Number
bool frontend::Parser::parsePrimaryExp(frontend::PrimaryExp *root) {log(root);
    if(CUR_TOKEN_IS(LPARENT)) {
        PARSE_TOKEN(LPARENT);
        PARSE(expnode, Exp);

        PARSE_TOKEN(RPARENT);
        return true;
    } else if(CUR_TOKEN_IS(INTLTR)|| CUR_TOKEN_IS(FLOATLTR)){
        PARSE(numbernode,Number);
        return true;
    } else if (CUR_TOKEN_IS(IDENFR)){
        PARSE(LValnode,LVal);
        return true;
    }
    assert(0 && "parsePrimaryExp");
}
//UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')'
//| UnaryOp UnaryExp
bool frontend::Parser::parseUnaryExp(frontend::UnaryExp *root) {log(root);
    if (CUR_TOKEN_IS(IDENFR)) {
        if (token_stream[index+1].type==TokenType::LPARENT) {
            PARSE_TOKEN(IDENFR);
            PARSE_TOKEN(LPARENT);
            if (!CUR_TOKEN_IS(RPARENT)) {
                PARSE(FuncRParamsnode, FuncRParams);
            }
            PARSE_TOKEN(RPARENT);
        }
        else {
            PARSE(PrimaryExpnode, PrimaryExp);
        }
        return true;
    }
    else if (CUR_TOKEN_IS(PLUS) || CUR_TOKEN_IS(MINU) || CUR_TOKEN_IS(NOT)) {
        PARSE(UnaryOpnode, UnaryOp);
        PARSE(UnaryExpnode, UnaryExp);
        return true;
    }
    else {
        PARSE(PrimaryExpnode, PrimaryExp);
        return true;
    }
    return true;
}

bool frontend::Parser::parseUnaryOp(frontend::UnaryOp *root) {log(root);
    if(CUR_TOKEN_IS(PLUS)){
        PARSE_TOKEN(PLUS);
        return true;
    } else if(CUR_TOKEN_IS(MINU)){
        PARSE_TOKEN(MINU);
        return true;
    } else if(CUR_TOKEN_IS(NOT)){
        PARSE_TOKEN(NOT);
        return true;
    }
    return false;}
//FuncRParams -> Exp { ',' Exp }
bool frontend::Parser::parseFuncRParams(frontend::FuncRParams* root) {log(root);
    PARSE(Expnode,Exp);
    while (CUR_TOKEN_IS(COMMA)){
        PARSE_TOKEN(COMMA);
        PARSE(Expnode1,Exp);
    }
    return true;
}
//MulExp -> UnaryExp { ('*' | '/' | '%') UnaryExp }
bool frontend::Parser::parseMulExp(frontend::MulExp *root) {log(root);
    PARSE(Unaryexpnode,UnaryExp);
    while (CUR_TOKEN_IS(MULT)||
           CUR_TOKEN_IS(DIV)||
           CUR_TOKEN_IS(MOD)){
        if(CUR_TOKEN_IS(MULT)) {
            PARSE_TOKEN(MULT);
        } else if(CUR_TOKEN_IS(DIV)){PARSE_TOKEN(DIV);}
        else if(CUR_TOKEN_IS(MOD)){
            PARSE_TOKEN(MOD);
        }
        PARSE(Unaryexpnode1,UnaryExp);
    }
    return true;
}
//RelExp -> AddExp { ('<' | '>' | '<=' | '>=') AddExp }
bool frontend::Parser::parseRelExp(frontend::RelExp *root) {log(root);
    PARSE(addexpnode1,AddExp);
    while (CUR_TOKEN_IS(LSS)||
           CUR_TOKEN_IS(GTR) ||
           CUR_TOKEN_IS(LEQ)||
           CUR_TOKEN_IS(GEQ)){
        if(CUR_TOKEN_IS(LSS)){
            PARSE_TOKEN(LSS);
        } else if(CUR_TOKEN_IS(GTR)){
            PARSE_TOKEN(GTR);
        } else if(CUR_TOKEN_IS(LEQ)){
            PARSE_TOKEN(LEQ);
        } else if(CUR_TOKEN_IS(GEQ)){
            PARSE_TOKEN(GEQ);
        }
        PARSE(addexpnode2,AddExp);
    }
    return true;
}

bool frontend::Parser::parseEqExp(frontend::EqExp *root) {log(root);
    PARSE(Relexpnode1,RelExp);
    while (CUR_TOKEN_IS(EQL)||
           CUR_TOKEN_IS(NEQ)){
        if(CUR_TOKEN_IS(EQL)){
            PARSE_TOKEN(EQL);
        } else if(CUR_TOKEN_IS(NEQ)){
            PARSE_TOKEN(NEQ);
        }
        PARSE(Relexnode2,RelExp);
    }
    return true;
}

bool frontend::Parser::parseLAndExp(frontend::LAndExp *root) {log(root);
    PARSE(EQEXPnode,EqExp);
    if(CUR_TOKEN_IS(AND)){
        PARSE_TOKEN(AND);
        PARSE(Landexp,LAndExp);
    }
    return true;
}







