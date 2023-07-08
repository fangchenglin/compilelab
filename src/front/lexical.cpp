#include"front/lexical.h"

#include<map>
#include<cassert>
#include<string>

#define TODO assert(0 && "todo")

// #define DEBUG_DFA
// #define DEBUG_SCANNER

std::string frontend::toString(State s) {
    switch (s) {
        case State::Empty: return "Empty";
        case State::Ident: return "Ident";
        case State::IntLiteral: return "IntLiteral";
        case State::FloatLiteral: return "FloatLiteral";
        case State::op: return "op";
        default:
            assert(0 && "invalid State");
    }
    return "";
}
frontend::TokenType get_keywords_type(std::string s){
    if(s=="const"){
        return frontend::TokenType::CONSTTK;
    }else if(s=="int"){
        return frontend::TokenType::INTTK;
    }else if(s=="float"){
        return frontend::TokenType::FLOATTK;
    }else if(s=="if"){
        return frontend::TokenType::IFTK;
    }else if(s=="else"){
        return frontend::TokenType::ELSETK;
    }else if(s=="while"){
        return frontend::TokenType::WHILETK;
    }else if(s=="continue"){
        return frontend::TokenType::CONTINUETK;
    }else if(s=="break"){
        return frontend::TokenType::BREAKTK;
    }else if(s=="return"){
        return frontend::TokenType::RETURNTK;
    }else if(s=="void"){
        return frontend::TokenType::VOIDTK;
    }else{
        return frontend::TokenType::IDENFR;
    }
}
std::set<std::string> frontend::keywords= {
        "const", "int", "float", "if", "else", "while", "continue", "break", "return", "void"
};

bool  isoperator(char  c)  {
    if (c == '-' || c == '+' || c == '/' || c == '*' || c == '(' || c == ')'

        ||c=='%'||c=='<'||c=='>'||c==':'||c=='='||c==';'||c==','||c=='['||c==']'||c=='{'||c=='}'||c=='!'||c=='&'||c=='|') {
        return true;
    }
    return false;

}
/**判断是否为大写字母**/
bool IsUpLetter(char ch){
    if(ch>='A' && ch<='Z') return true;
    return false;
}
/**判断是否为小写字母**/
bool IsLowLetter(char ch){
    if(ch>='a' && ch<='z') return true;
    return false;
}
bool IsLetter(char ch){
    if(IsUpLetter(ch)||IsLowLetter(ch)) return true;
    return false;
}
frontend::TokenType get_op_type(std:: string s){
    if(s=="+"){
        return frontend::TokenType::PLUS;
    }else if (s=="-")
    {
        return frontend::TokenType::MINU;
    }else if (s=="*")
    {
        return frontend::TokenType::MULT;
    }else if (s=="/")
    {
        return frontend::TokenType::DIV;
    }else if (s=="%")
    {
        return frontend::TokenType::MOD;
    }else if (s=="<")
    {
        return frontend::TokenType::LSS;
    }else if (s==">")
    {
        return frontend::TokenType::GTR;
    }else if (s==":")
    {
        return frontend::TokenType::COLON;
    }else if (s=="=")
    {
        return frontend::TokenType::ASSIGN;
    }else if (s==";")
    {
        return frontend::TokenType::SEMICN;
    }else if (s==",")
    {
        return frontend::TokenType::COMMA;
    }else if (s=="(")
    {
        return frontend::TokenType::LPARENT;
    }else if (s==")")
    {
        return frontend::TokenType::RPARENT;
    }else if (s=="[")
    {
        return frontend::TokenType::LBRACK;
    }else if (s=="]")
    {
        return frontend::TokenType::RBRACK;
    }else if (s=="{")
    {
        return frontend::TokenType::LBRACE;
    }else if (s=="}")
    {
        return frontend::TokenType::RBRACE;
    }else if (s=="!")
    {
        return frontend::TokenType::NOT;
    }else if (s=="<=")
    {
        return frontend::TokenType::LEQ;
    }else if (s==">=")
    {
        return frontend::TokenType::GEQ;
    }else if (s=="==")
    {
        return frontend::TokenType::EQL;
    }else if (s=="!=")
    {
        return frontend::TokenType::NEQ;
    }else if (s=="&&")
    {
        return frontend::TokenType::AND;
    }else if (s=="||")
    {
        return frontend::TokenType::OR;
    }else{
        return frontend::TokenType::INTLTR;
    }

}
frontend::DFA::DFA(): cur_state(frontend::State::Empty), cur_str() {}

frontend::DFA::~DFA() {}

bool frontend::DFA::next(char input, Token& buf) {
#ifdef DEBUG_DFA
    #include<iostream>
    std::cout << "in state [" << toString(cur_state) << "], input = \'" << input << "\', str = " << cur_str << "\t";
#endif


    if(cur_state==State::Empty){
        cur_str="";
        if(IsLetter(input)||input=='_'){
            cur_state=State::Ident;
            cur_str=input;
        }else if(isspace(input)||input=='\n'||input=='\r'){
            cur_state=State::Empty;
        }else if(isoperator(input)){
            cur_state=State::op;
            cur_str=input;
        }else if(isdigit(input)){
            cur_state=State::IntLiteral;
            cur_str+=input;
        } else if(input=='.'){
            cur_state=State::FloatLiteral;
            cur_str=input;
        }
    }else if(cur_state==State::IntLiteral){
        if(isdigit(input)||input=='x'||input=='b'||(input>='A'&&input<='F')||(input>='a'&&input<='f')){
            cur_str+=input;
        }else if(input=='.'){
            cur_state=State::FloatLiteral;
            cur_str+=input;
        }else{
            buf.type=TokenType::INTLTR;
            buf.value=cur_str;
            cur_str="";
            if(isoperator(input)){
                cur_state=State::op;
                cur_str=input;
            }else{
                cur_state=State::Empty;

            }
            return true;
        }
    }else if(cur_state==State::FloatLiteral){
        if(isdigit(input)){
            cur_str+=input;
        }else{
            buf.type=TokenType::FLOATLTR;
            buf.value=cur_str;
            cur_str="";
            if(isoperator(input)){
                cur_state=State::op;
                cur_str=input;
            }else{
                cur_state=State::Empty;

            }
            return true;
        }
    }else if(cur_state==State::op){
        buf.type=get_op_type(cur_str);
        buf.value=cur_str;
//        cur_str="";
        if(isdigit(input)){
            cur_state=State::IntLiteral;
            cur_str=input;
            return true;
        }else if(isoperator(input)){

            if ((buf.value == "="&& input  == '=')||(buf.value == ">"&& input  == '=')||(buf.value == "<"&& input  == '=')||
                (buf.value == "!"&& input  == '=')||(buf.value == "&"&& input  == '&')||(buf.value == "|"&& input  == '|')){
                cur_state=State::op;
                cur_str+=input;
                return false;
            }
            else{
                cur_state = State::op;
                cur_str = input;
                return true;
            }
        }else if(IsLetter(input)){
            cur_state=State::Ident;
            cur_str=input;

            return true;
        } else if(input=='.'){
            cur_state=State::FloatLiteral;
            cur_str=input;
        }
        else{
            cur_state=State::Empty;
            cur_str="";
        }



        return true;
    }else if(cur_state==State::Ident){
        if(IsLetter(input)||isdigit(input)||input=='_'){
            cur_str+=input;
        }else{
            buf.type=get_keywords_type(cur_str);
            buf.value=cur_str;
            if(isoperator(input)){
                cur_state=State::op;
                cur_str=input;
            }else{
                cur_state=State::Empty;

            }
            return true;
        }
    }
    return false;

#ifdef DEBUG_DFA
    std::cout << "next state is [" << toString(cur_state) << "], next str = " << cur_str << "\t, ret = " << ret << std::endl;
#endif
}

void frontend::DFA::reset() {
    cur_state = State::Empty;
    cur_str = "";
}

frontend::Scanner::Scanner(std::string filename): fin(filename) {
    if(!fin.is_open()) {
        assert(0 && "in Scanner constructor, input file cannot open");
    }
}

frontend::Scanner::~Scanner() {
    fin.close();
}




std::vector<frontend::Token> frontend::Scanner::run() {
    std::vector<frontend::Token> tokens; // 存储 token 的向量
    std::string inputStr, line;
    int pos, startPos, endPos;

// 逐行读取输入文件的内容
    while (std::getline(fin, line)) {
        // 去除单行注释
        pos = line.find("//");
        line = line.substr(0, pos);

        // 拼接字符串
        inputStr += line;
        inputStr += "\n";

        // 去除多行注释
        if (inputStr.find("*/") != std::string::npos) {
            startPos = inputStr.find("/*");
            endPos = inputStr.rfind("*/");
            if (endPos - startPos >= 2) {
                // 确保多行注释结束后的字符不是注释符号，而是代码的一部分
                size_t nextCharPos = inputStr.find_first_not_of(" \t", endPos + 2);
                if (nextCharPos != std::string::npos && inputStr[nextCharPos] != '/') {
                    inputStr = inputStr.substr(0, startPos) + inputStr.substr(endPos + 2);
                }
            }
        }
    }

// 初始化 DFA
    DFA dfa;
    Token token;

// 对字符串中的每个字符进行 DFA 匹配
    for (size_t i = 0; i < inputStr.size(); i++) {
        if (dfa.next(inputStr[i], token)) {
            tokens.push_back(token);
#ifdef DEBUG_SCANNER
            #include <iostream>
        std::cout << "token: " << toString(token.type) << "\t" << token.value << std::endl;
#endif
        }
    }

    return tokens;




#ifdef DEBUG_SCANNER
    #include<iostream>
            std::cout << "token: " << toString(tk.type) << "\t" << tk.value << std::endl;
#endif
}