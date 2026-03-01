#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
//#include <string.h>
#include "arena.h"
#include <ctype.h>
#include <string.h>

// Tokenizer DFA

/*
  digits    : 0-9
  number    : digits+
  operators : + | - | *

  {} == Final State
  NFA:
  Digit NFA:
         digit
    S  --------> {F}

  Number NFA:

          e                   e
    S  --------> DigitDFA | -----> {F}
                    ^     | e
                    |-----|

 Operator NFA:

         operator
    S  ------------> {F}


  Now we have to implement this for each string of characters that are divided by ' '
  So image, each start state starts after a ' '

  Final NFA:

           e      (N)        e
        /-----  NumberNFA  -----\
       /                         \
     (S)                         {F}
       \                         /
        \----- OperatorNFA -----/
           e      (O)        e


  Converted DFA:
 ||Much simpler than I anticipated.||

             ____
             |  | digit
             v  |
         /--{I}--
  digit /
       /
     (S)------------>{O}
      |
space |
      |-->{F}---|
           ^    |space
           |----|


  DFA TABLE:

St\In | Digit | Operator | space |
   S  |  {I}  |    {O}   |  {F}  |
  {I} |  {I}  | -------- | ----- |
   O  | ----- | -------- | ----- |
  {F} | ----- | -------- |  {F}  |

*/

typedef enum {
    FALSE,
    TRUE
}BoolValue;

typedef enum DFAStates{
    D_STATE_S,
    D_STATE_I,
    D_STATE_O,
    D_STATE_F,
    DFAStatesNum,
    D_STATE_WRONG_INPUT,
    D_STATE_NOT_ERROR_STATE
}dState;

const char* DFAStateNames[] = {
    "D_STATE_S",
    "D_STATE_I",
    "D_STATE_O",
    "D_STATE_F",
    "DFAStatesNum",
    "D_STATE_WRONG_INPUT",
    "D_STATE_NOT_ERROR_STATE"
};

typedef enum DFAInputs{
    Digit,
    Operator,
    Space,
    DFAInputNum
}dInput;

typedef struct{
    dState state[DFAStatesNum * DFAInputNum];
    dState currState;
}DFA;

// NOTE : Automation for the future.
DFA* CreateDFA(Arena* arena){
    DFA* tempDFA;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, DFA, tempDFA);
    if (err != ARENA_OK) {
        printf("Couldn't build DFA!");
        abort();
    }
    int tableIndex;
    tableIndex = D_STATE_S * DFAInputNum + Digit;
    tempDFA->state[tableIndex] = D_STATE_I;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_O;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_F;

    tableIndex = D_STATE_I * DFAInputNum + Digit;
    tempDFA->state[tableIndex] = D_STATE_I;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_WRONG_INPUT;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_NOT_ERROR_STATE;

    tableIndex = D_STATE_O * DFAInputNum + Digit;
    tempDFA->state[tableIndex] = D_STATE_WRONG_INPUT;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_WRONG_INPUT;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_NOT_ERROR_STATE;

    tableIndex = D_STATE_F * DFAInputNum + Digit;
    tempDFA->state[tableIndex] = D_STATE_NOT_ERROR_STATE;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_NOT_ERROR_STATE;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_F;
    printf("Made simple arithematic DFA!\n");
    return tempDFA;
}

typedef enum{
    TOKEN_NUMBER,
    TOKEN_OPERATOR,
}TOKEN_TYPE;

typedef enum{
    OP_PLUS,
    OP_MINUS,
    OP_MUL,
    OP_DIV
}OPERATOR_TYPE;

char* TokenNames[] = {
    "TOKEN_NUMBER",
    "TOKEN_OPERATOR"
};

char* OperatorNames[] = {
    "PLUS",
    "MINUS",
    "MUL",
    "DIV"
};

typedef struct{
    TOKEN_TYPE type;
    union {
        i64 numValue;
        OPERATOR_TYPE operator;
    }TokenValue;
}TokenInfo;

typedef struct{
    TokenInfo* tokens;
    u16 numTokens;
}Tokens;

typedef struct{
    const char* inputStream;
    Tokens* tok;
    u32 pos;
}Tokenizer;

Tokenizer* CreateTokenizer(Arena* arena, const char* inputStream){
    Tokenizer* tempTokenizer;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, Tokenizer, tempTokenizer);
    if (err != ARENA_OK) {
        printf("Couldn't build Tokenizer!");
        abort();
    }
    tempTokenizer->pos = 0;
    tempTokenizer->inputStream = inputStream;
    printf("Made simple tokenizer!\n");
    return tempTokenizer;
}

void SkipSpaces(DFA* dfa, Tokenizer* tokenizer){
    char currChar = tokenizer->inputStream[tokenizer->pos];
    while (currChar == ' ' || currChar == '\t' || currChar == '\n'){
        tokenizer->pos++;
        currChar = tokenizer->inputStream[tokenizer->pos];
        dfa->currState = D_STATE_S;
    }
    return;
}

TokenInfo TokenizeNumber(DFA* dfa, Tokenizer* tokenizer){
    TokenInfo tok;
    char currChar = tokenizer->inputStream[tokenizer->pos];
    if (!isdigit(currChar)){
        return tok;
    }
    u16 startPos = tokenizer->pos;
    while (isdigit(currChar)){
        tokenizer->pos++;
        currChar = tokenizer->inputStream[tokenizer->pos];
    }
    tok.TokenValue.numValue = atoi(&tokenizer->inputStream[startPos]);
    tok.type = TOKEN_NUMBER;
    printf("Gathered digits at pos with currentState: %s\n", DFAStateNames[dfa->currState]);
    int lookupIndex = dfa->currState * DFAInputNum + Digit;
    dfa->currState = dfa->state[lookupIndex];
    return tok;
}

TokenInfo TokenizeOp(DFA* dfa, Tokenizer* tokenizer){
    TokenInfo tok;
    char currChar = tokenizer->inputStream[tokenizer->pos];
    _Bool FoundOperator = TRUE;
    switch(currChar){
        case '+': {
            tok.TokenValue.operator = OP_PLUS;
            break;
        }
        case '-': {
            tok.TokenValue.operator = OP_MINUS;
            break;
        }
        case '*': {
            tok.TokenValue.operator = OP_MUL;
            break;
        }
        case '/': {
            tok.TokenValue.operator = OP_DIV;
            break;
        }
        default: {
            FoundOperator = FALSE;
            break;
        }
    }
    if(FoundOperator){
        printf("Tokenizing operator at pos:%d with currentState: %s, and current operator: %c\n", tokenizer->pos, DFAStateNames[dfa->currState], currChar);
        tokenizer->pos++;
        int lookupIndex = dfa->currState * DFAInputNum + Operator;
        tok.type = TOKEN_OPERATOR;
        dfa->currState = dfa->state[lookupIndex];
    }
    return tok;
}

void AnalyzeState(DFA* dfa, Tokenizer* tokenizer, TokenInfo info){
    switch(dfa->currState){
        case D_STATE_NOT_ERROR_STATE: {
            int i;
dfa->currState = D_STATE_S;
            break;
        }
        case D_STATE_F: {
            printf("Somehow, at pos: %d, the tokenizer reached final state.\n", tokenizer->pos);
            break;
        }
        case D_STATE_S: {
            break;
        }
        case D_STATE_O: {
            tokenizer->tok->tokens[tokenizer->tok->numTokens].type = TOKEN_OPERATOR;
            tokenizer->tok->tokens[tokenizer->tok->numTokens].TokenValue = info.TokenValue;
            tokenizer->tok->numTokens++;
            break;
        }
        case D_STATE_I: {
            tokenizer->tok->tokens[tokenizer->tok->numTokens].type = TOKEN_NUMBER;
            tokenizer->tok->tokens[tokenizer->tok->numTokens].TokenValue = info.TokenValue;
            tokenizer->tok->numTokens++;
            break;
        }
        case D_STATE_WRONG_INPUT: {
            printf("Wrong token at: %d\n", tokenizer->pos);
            abort();
        }
        case DFAStatesNum: {
            printf("Wrong table due to: %d\n", tokenizer->pos);
            abort();
        }
    }
}

Tokens* StartTokenizing(Arena* arena, DFA* dfa, Tokenizer* tokenizer){
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, Tokens, tokenizer->tok);
    if (err != ARENA_OK) {
        printf("Couldn't allocate list of tokens!");
        abort();
    }
    tokenizer->tok->numTokens = 0;
    //Getting the pointer for the list of tokens.
    GET_POINTER_IN_ARENA(arena, TOKENS, tokenizer->tok->tokens);
    char currChar = tokenizer->inputStream[0];
    dfa->currState = D_STATE_S;
    printf("Starting tokenizing:\n");
    printf("    %s\n", tokenizer->inputStream);
    while (currChar != '\0'){
        TokenInfo info;
        SkipSpaces(dfa, tokenizer);
        info = TokenizeNumber(dfa, tokenizer);
        info = TokenizeOp(dfa, tokenizer);
        AnalyzeState(dfa, tokenizer, info);
        currChar = tokenizer->inputStream[tokenizer->pos];
    }
    printf("Tokenization successful with %d tokens\n", tokenizer->tok->numTokens);
    PUSH_EMPTY_ARRAY_IN_ARENA(arena, TokenInfo, tokenizer->tok->numTokens, tokenizer->tok->tokens);
    // Rememebr to push the pointer.
    return tokenizer->tok;
}

void printTokens(Tokens* tokens){
    for(uint i = 0; i < tokens->numTokens; i++){
        TOKEN_TYPE type = tokens->tokens[i].type;
        if(type == TOKEN_NUMBER){
            printf("Token %d : %s : %"PRId64"\n", i, TokenNames[tokens->tokens[i].type], tokens->tokens[i].TokenValue.numValue);
        } else {
            printf("Token %d : %s : %s\n", i, TokenNames[tokens->tokens[i].type], OperatorNames[tokens->tokens[i].TokenValue.operator]);
        }
    }
}

/*

    Simple Grammar:
    S     -> Exp
    Exp   -> Number NuExp
    NuExp -> Op Exp | empty
    Op    -> + | - | * | /

    We will make a recursive descent parser for this.
*/

typedef enum {
    NODE_PLUS,
    NODE_MINUS,
    NODE_MUL,
    NODE_DIV,
    NODE_LEAF,
    NODE_NODES_NUM,
    NODE_INVALID
}ASTNodeType;

typedef struct ASTNode ASTNode;

struct ASTNode{
    union {
        struct {
            ASTNode* leftNode;
            ASTNode* rightNode;
        }BinaryOperation;
        i64 number; // Or a value as a leaf node.
    }Value;
    ASTNodeType nodeType;
};

typedef struct {
    Tokens* tok;
    ASTNode* startNode;
    u16 tokenPos;
}Parser;

Parser* CreateParser(Arena* arena, Tokens* tok){
    Parser* tempParser;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, Parser, tempParser);
    if (err != ARENA_OK) {
        printf("Couldn't build Parser!");
        abort();
    }
    tempParser->tok = tok;
    tempParser->tokenPos = 0;
    return tempParser;
}

ASTNode* CheckOP(Arena* arena, Parser* parser){
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        ASTNode* currNode;
        ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
        if (err != ARENA_OK) {
            printf("Couldn't build AST: BINARY PARENT NODE!\n");
            abort();
        }
        ASTNodeType type = (ASTNodeType)parser->tok->tokens[parser->tokenPos].TokenValue.operator;
        currNode->nodeType = type;
        printf("OPERATOR TOKEN AT: %d\n", parser->tokenPos);
        parser->tokenPos++;
        return currNode;
    }
    printf("EXPECTED OP AT: %d\n",parser->tokenPos);
    return NULL;
}

ASTNode* CheckNum(Arena* arena, Parser* parser){
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_NUMBER){
        ASTNode* currNode;
        ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
        if (err != ARENA_OK) {
            printf("Couldn't build AST: LEAF NUM NODE!\n");
            abort();
        }
        printf("NUMBER TOKEN AT: %d\n", parser->tokenPos);
        currNode->Value.number = parser->tok->tokens[parser->tokenPos].TokenValue.numValue;
        currNode->nodeType = NODE_LEAF;
        parser->tokenPos++;
        return currNode;
    }
    printf("EXPECTED NUM AT: %d\n",parser->tokenPos);
    return NULL;
}

ASTNode* CheckNuExp(Arena* arena, Parser* parser);

ASTNode* CheckExp(Arena* arena, Parser* parser){
    u16 prevPos = parser->tokenPos;
    ASTNode* numNode = CheckNum(arena, parser); //5
    if(numNode != NULL){
        ASTNode* nuExpNode = CheckNuExp(arena, parser);
        if(nuExpNode != NULL){
            nuExpNode->Value.BinaryOperation.leftNode = numNode;
        }
        else {
            return numNode;
        }
        printf("EXPANDING EXP AT: %d\n", prevPos);
        return nuExpNode;
    }
    abort();
}

ASTNode* CheckNuExp(Arena* arena, Parser* parser){
    u16 prevTokPos = parser->tokenPos;
    ASTNode* binaryNode = CheckOP(arena, parser);
    if(binaryNode != NULL){
        ASTNode* expNode = CheckExp(arena, parser);
        if(expNode != NULL){
            binaryNode->Value.BinaryOperation.rightNode = expNode;
            return binaryNode;
        }
    } else if(parser->tokenPos >= parser->tok->numTokens){
            return NULL;
        }
    abort();
}

void StartParsing(Arena* arena, Parser* parser){
    parser->startNode = CheckExp(arena, parser);
    if(parser->startNode != NULL){
        printf("PARSED CORRECTLY\n");
    }
}

void WalkAST(Parser* parser){
    ASTNode* currNode = parser->startNode;
    ASTNode* previousNode = parser->startNode;
    while (currNode != NULL && currNode->nodeType != NODE_INVALID){
        if(currNode->nodeType == NODE_LEAF){
            printf("The value of the node is: %"PRId64"\n", currNode->Value.number);
            if(currNode == previousNode->Value.BinaryOperation.rightNode){
                printf("AST Walk Complete!\n");
                return;
            }
            currNode = previousNode->Value.BinaryOperation.rightNode;
        } else if (currNode->nodeType != NODE_INVALID && currNode->nodeType != NODE_NODES_NUM){
            printf("The value of the node is: %s\n", OperatorNames[currNode->nodeType]);
            previousNode = currNode;
            currNode = currNode->Value.BinaryOperation.leftNode;
        }
    }
}

//TODO : Hardest thing now: Optimization and Generation of Assembly.
int main(){
    Arena LexerArena;
    ARENA_ERROR err = makeArena(&LexerArena, KiB(10));
    if(err != ARENA_OK){
        printf("Can't make arena");
        return 0;
    }
    DFA* arithDFA = CreateDFA(&LexerArena);
    const char* sampleProgram = "5 + 3 - 1";
    Tokenizer* mainTokenizer = CreateTokenizer(&LexerArena, sampleProgram);
    Tokens* tokenizedProgram = StartTokenizing(&LexerArena, arithDFA, mainTokenizer);
    printTokens(tokenizedProgram);
    Parser* mainParser = CreateParser(&LexerArena, tokenizedProgram);
    StartParsing(&LexerArena, mainParser);
    WalkAST(mainParser);
    return 0;
}
