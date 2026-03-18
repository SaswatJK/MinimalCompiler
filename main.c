#include <bits/types/stack_t.h>
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
  charLC    : a-z
  charUC    : A-Z
  strings   : (charLC OR charUC)+
  keywords  : Specific Strings
  number    : digits+
  operators : + | - | *

  {} == Final State
  NFA:
  Digit NFA:
         digit
    S  --------> {F}

  Number NFA:

          e                   e
    S  --------> DigitNFA | -----> {F}
                    ^     | e
                    |-----|

  Operator NFA:

         operator
    S  ------------> {F}

  CharLC NFA:

         a-z
    S  ------> {F}

  CharUC NFA:

         A-Z
    S  ------> {F}

  Strings NFA:

        e               e
     /----- CharLCNFA -----\
    S                      {F}
     \----- CharUCNFA -----/
        e               e


  Now we have to implement this for each string of characters that are divided by ' '
  So image, each start state starts after a ' '

  Final NFA:

           e      (N)        e
        /-----  NumberNFA  -----\
       /                         \
     (S)---e-- StringsNFA(S) -e--{F}
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
         char      /  operator
 {St}<-----------(S)------------>{O}
                  |
            space |
                  |-->{F}---|
                       ^    |space
                       |----|


  DFA TABLE:

St\In | Digit | Operator | Space | Char |
   S  |  {I}  |    {O}   |  {F}  | {St} |
  {I} |  {I}  | -------- | ----- | WR_I |
   O  | ----- |   WR_IN  | ----- | ---- |
  {F} | ----- | -------- |  {F}  | ---- |
 {St} | WR_IN | -------- | ----- | {St} |

*/

typedef struct{
    const char* start;
    u16 length;
}SimpleString;

typedef enum {
    FALSE,
    TRUE
}BoolValue;


_Bool MatchStringDirect(SimpleString* source, const char* compareTo){ // Direct because it's taking a const char pointer as a comparison to.
    if (source->length != strlen(compareTo)){
        return FALSE; // Maybe better types?
    }
    int result = memcmp(source->start, compareTo, source->length);
    if(result == 0){
        return TRUE;
    }
    return FALSE;
}

_Bool MatchStringIndirect(SimpleString* source, SimpleString* compareTo){
    if (source->length != compareTo->length){
        return FALSE; // Maybe better types?
    }
    int result = memcmp(source->start, compareTo->start, source->length);
    if(result == 0){
        return TRUE;
    }
    return FALSE;
}

static inline void PrintSimpleString(SimpleString* source){
    fprintf(stderr, "%.*s", source->length, source->start);
}

typedef enum DFAStates{
    D_STATE_S,
    D_STATE_I,
    D_STATE_O,
    D_STATE_F,
    D_STATE_ST,
    DFAStatesNum,
    D_STATE_WRONG_INPUT,
    D_STATE_NOT_ERROR_STATE
}dState;

const char* DFAStateNames[] = {
    "D_STATE_S",
    "D_STATE_I",
    "D_STATE_O",
    "D_STATE_F",
    "D_STATE_ST",
    "DFAStatesNum",
    "D_STATE_WRONG_INPUT",
    "D_STATE_NOT_ERROR_STATE"
};

typedef enum DFAInputs{
    Digit,
    Operator,
    Space,
    Character,
    DFAInputNum
}dInput;

typedef struct{
    dState state[DFAStatesNum * DFAInputNum];
    dState currState;
}DFA;

DFA* CreateDFA(Arena* arena){
    DFA* tempDFA;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, DFA, tempDFA);
    if (err != ARENA_OK) {
        fprintf(stderr, "Couldn't build DFA: %s", ArenaErrorNames[err]);
        printArenaInfo(arena);
        abort();
    }

    const dState transitions[DFAStatesNum][DFAInputNum] = {
        [D_STATE_S]  = { D_STATE_I,               D_STATE_O,               D_STATE_F,               D_STATE_ST              },
        [D_STATE_I]  = { D_STATE_I,               D_STATE_NOT_ERROR_STATE, D_STATE_NOT_ERROR_STATE, D_STATE_WRONG_INPUT     },
        [D_STATE_O]  = { D_STATE_NOT_ERROR_STATE, D_STATE_NOT_ERROR_STATE, D_STATE_NOT_ERROR_STATE, D_STATE_NOT_ERROR_STATE },
        [D_STATE_F]  = { D_STATE_NOT_ERROR_STATE, D_STATE_NOT_ERROR_STATE, D_STATE_F,               D_STATE_NOT_ERROR_STATE },
        [D_STATE_ST] = { D_STATE_WRONG_INPUT,     D_STATE_NOT_ERROR_STATE, D_STATE_NOT_ERROR_STATE, D_STATE_ST              },
    };

    memcpy(tempDFA->state, transitions, sizeof(transitions));
    tempDFA->currState = D_STATE_S;
    return tempDFA;
}

typedef enum{
    TOKEN_NUMBER,
    TOKEN_OPERATOR,
    TOKEN_KEYWORD,
    TOKEN_ID,
    TOKEN_PRIM_TYPE,
    TOKEN_NONE,
}TOKEN_TYPE;

typedef enum{
    OP_PLUS,
    OP_MINUS,
    OP_MUL,
    OP_DIV,
    OP_LC,
    OP_RC,
    OP_LP,
    OP_RP,
    OP_EQ,
    OP_SC,
    OP_LT,
    OP_GT,
    OP_C,
}OPERATOR_TYPE;

typedef enum{
    KW_IF,
    KW_ELSE,
    KW_PRINT,
    KWS_NUM // Number of keywords
}KEYWORD_TYPE;

typedef enum{
    PT_INT,
    PTS_NUM,
    PT_UNKNOWN // If an ID is being used in an expression, it's type can only be checked after parsing.
}PRIMITIVE_TYPE;

char* TokenNames[] = {
    "TOKEN_NUMBER",
    "TOKEN_OPERATOR",
    "TOKEN_KEYWORD",
    "TOKEN_ID",
    "TOKEN_PRIM_TYPE",
    "TOKEN_NONE"
};

char* OperatorNames[] = {
    "PLUS",
    "MINUS",
    "MUL",
    "DIV",
    "OP_LC",
    "OP_RC",
    "OP_LP",
    "OP_RP",
    "OP_EQ",
    "OP_SC",
    "OP_LT",
    "OP_GT",
    "OP_C",
};

char* KeyWordNames[] = {
    "if",
    "else",
    "print",
};

char* PrimitiveTypeNames[] = {
    "int",
};

typedef struct{
    TOKEN_TYPE type;
    union {
        SimpleString stringValue;
        i64 numValue;
        OPERATOR_TYPE operator;
        KEYWORD_TYPE keyword;
        PRIMITIVE_TYPE primType;
    } TokenValue;
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
        fprintf(stderr, "Couldn't build Tokenizer: %s", ArenaErrorNames[err]);
        printArenaInfo(arena);
        abort();
    }
    tempTokenizer->pos = 0;
    tempTokenizer->inputStream = inputStream;
    fprintf(stderr, "Made simple tokenizer!\n");
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
    tok.type = TOKEN_NONE;
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
    //fprintf(stderr, "Gathered digits at pos with currentState: %s\n", DFAStateNames[dfa->currState]);
    int lookupIndex = dfa->currState * DFAInputNum + Digit;
    dfa->currState = dfa->state[lookupIndex];
    return tok;
}

TokenInfo TokenizeString(DFA* dfa, Tokenizer* tokenizer){
    TokenInfo tok;
    tok.type = TOKEN_NONE;
    char currChar = tokenizer->inputStream[tokenizer->pos];
    if (!isalpha(currChar)){
        return tok;
    }
    u16 startPos = tokenizer->pos;
    u16 lengthString = 0;
    while (isalpha(currChar)){
        tokenizer->pos++;
        currChar = tokenizer->inputStream[tokenizer->pos];
        lengthString++;
    }
    SimpleString sourceString;
    sourceString.start = &tokenizer->inputStream[(int)startPos];
    sourceString.length = lengthString;
    i16 KeywordMatchString = -1;
    i16 PrimitiveMatchString = -1;
    for (u16 i = 0; i < KWS_NUM; i++){
        _Bool match = MatchStringDirect(&sourceString, KeyWordNames[i]);
        if(match)
            KeywordMatchString = i;
    }
    if (KeywordMatchString != -1){ // It's a keyword.
        tok.type = TOKEN_KEYWORD;
        tok.TokenValue.keyword = KeywordMatchString;
    } else {
        for(u16 i = 0; i < PTS_NUM; i++){
            _Bool match = MatchStringDirect(&sourceString, PrimitiveTypeNames[i]);
            if(match)
                PrimitiveMatchString = i;
        }
        if (PrimitiveMatchString != -1){ // It's a primitive Keyword.
            tok.type = TOKEN_PRIM_TYPE;
            tok.TokenValue.primType = PrimitiveMatchString;
        } else {
            tok.type = TOKEN_ID;
            tok.TokenValue.stringValue = sourceString;
        }
    }
    int lookupIndex = dfa->currState * DFAInputNum + Character;
    dfa->currState = dfa->state[lookupIndex];
    return tok;
}

TokenInfo TokenizeOp(DFA* dfa, Tokenizer* tokenizer){
    TokenInfo tok;
    tok.type = TOKEN_NONE;
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
        case '{': {
            tok.TokenValue.operator = OP_LC;
            break;
        }
        case '}': {
            tok.TokenValue.operator = OP_RC;
            break;
        }
        case '(': {
            tok.TokenValue.operator = OP_LP;
            break;
        }
        case ')': {
            tok.TokenValue.operator = OP_RP;
            break;
        }
        case ';': {
            tok.TokenValue.operator = OP_SC;
            break;
        }
        case ':':{
            tok.TokenValue.operator = OP_C;
            break;
        }
        case '<': {
            tok.TokenValue.operator = OP_LT;
            break;
        }
        case '>':{
            tok.TokenValue.operator = OP_GT;
            break;
        }
        case '=':{
            tok.TokenValue.operator = OP_EQ;
            break;
        }
        default: {
            FoundOperator = FALSE;
            break;
        }
    }
    if(FoundOperator){
        //fprintf(stderr, "Tokenizing operator at pos:%d with currentState: %s, and current operator: %c\n", tokenizer->pos, DFAStateNames[dfa->currState], currChar);
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
            dfa->currState = D_STATE_S;
            break;
        }
        case D_STATE_F: {
            fprintf(stderr, "Somehow, at pos: %d, the tokenizer reached final state.\n", tokenizer->pos);
            break;
        }
        case D_STATE_S: {
            break;
        }
        case D_STATE_O: {
            tokenizer->tok->tokens[tokenizer->tok->numTokens].type = TOKEN_OPERATOR;
            tokenizer->tok->tokens[tokenizer->tok->numTokens].TokenValue = info.TokenValue;
            tokenizer->tok->numTokens++;
            dfa->currState = D_STATE_S;
            break;
        }
        case D_STATE_I: {
            tokenizer->tok->tokens[tokenizer->tok->numTokens].type = TOKEN_NUMBER;
            tokenizer->tok->tokens[tokenizer->tok->numTokens].TokenValue = info.TokenValue;
            tokenizer->tok->numTokens++;
            dfa->currState = D_STATE_S;
            break;
        }
        case D_STATE_ST: {
            tokenizer->tok->tokens[tokenizer->tok->numTokens].type = info.type;
            tokenizer->tok->tokens[tokenizer->tok->numTokens].TokenValue = info.TokenValue;
            tokenizer->tok->numTokens++;
            dfa->currState = D_STATE_S;
            break;
        }
        case D_STATE_WRONG_INPUT: {
            fprintf(stderr, "Wrong token at: %d\n", tokenizer->pos);
            fprintf(stderr, "Token type is: %s\n", TokenNames[info.type]);
            abort();
        }
        case DFAStatesNum: {
            fprintf(stderr, "Wrong table due to: %d\n", tokenizer->pos);
            abort();
        }
    }
}

Tokens* StartTokenizing(Arena* arena, DFA* dfa, Tokenizer* tokenizer){
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, Tokens, tokenizer->tok);
    if (err != ARENA_OK) {
        fprintf(stderr, "Couldn't allocate list of tokens: %s", ArenaErrorNames[err]);
        printArenaInfo(arena);
        abort();
    }
    tokenizer->tok->numTokens = 0;
    //Getting the pointer for the list of tokens.
    GET_POINTER_IN_ARENA(arena, TOKENS, tokenizer->tok->tokens);
    char currChar = tokenizer->inputStream[0];
    dfa->currState = D_STATE_S;
    fprintf(stderr, "Starting tokenizing:\n");
    fprintf(stderr, "    \"%s\"\n", tokenizer->inputStream);
    while (currChar != '\0'){
        TokenInfo info;
        SkipSpaces(dfa, tokenizer);
        if ((info = TokenizeNumber(dfa, tokenizer)).type == TOKEN_NONE)
            if ((info = TokenizeOp(dfa, tokenizer)).type == TOKEN_NONE)
                info = TokenizeString(dfa, tokenizer);
        AnalyzeState(dfa, tokenizer, info);
        currChar = tokenizer->inputStream[tokenizer->pos];
    }
    fprintf(stderr, "Tokenization successful with %d tokens\n", tokenizer->tok->numTokens);
    PUSH_EMPTY_ARRAY_IN_ARENA(arena, TokenInfo, tokenizer->tok->numTokens, tokenizer->tok->tokens);
    // Rememebr to push the pointer.
    return tokenizer->tok;
}

void printTokens(Tokens* tokens){
    for(uint i = 0; i < tokens->numTokens; i++){
        TOKEN_TYPE type = tokens->tokens[i].type;
        if(type == TOKEN_NUMBER){
            fprintf(stderr, "Token %d : %s : %"PRId64"\n", i, TokenNames[tokens->tokens[i].type], tokens->tokens[i].TokenValue.numValue);
        } else if(type == TOKEN_OPERATOR)  {
            fprintf(stderr, "Token %d : %s : %s\n", i, TokenNames[tokens->tokens[i].type], OperatorNames[tokens->tokens[i].TokenValue.operator]);
        } else if(type == TOKEN_KEYWORD) {
            fprintf(stderr, "Token %d : %s : %s\n", i, TokenNames[tokens->tokens[i].type], KeyWordNames[tokens->tokens[i].TokenValue.keyword]);
        } else if (type == TOKEN_ID) {
            fprintf(stderr, "Token %d : %s : ", i, TokenNames[tokens->tokens[i].type]);
            PrintSimpleString(&tokens->tokens[i].TokenValue.stringValue);
            fprintf(stderr, "\n");
        }
    }
}

/*

    We will make a recursive descent parser for this.

    Complete Simple Grammar:

    S           -> Block | empty
    Block       -> { Stmnt } NuBlock
    NuBlock     -> Block | empty
    Stmnt       -> AssignExp NuStmnt | IfExp NuStmnt | PrintExp NuStmnt | empty NuStmnt
    NuStmnt     -> ; Stmnt | empty <- It will only look for another statement if there's already a semicolon.
    Exp         -> SubExp LowPresExp
    LowPresExp  -> BOPLP SubExp LowPresExp | empty
    SubExp      -> ID HighPresExp | Num HighPresExp | Factor HighPresExp <- I could make ID and Num to a 'term' function but I feel like maybe it's too many function calls for just one token check?
    HighPresExp -> BOPHP SubExp | empty
    Factor      -> ( Exp )
    AssignExp   -> ID : Type = Exp
    IfExp       -> 'if' ( CompareExp ) Block Elsepart
    PrintExp    -> 'print'(ID) | 'print'(Exp) <- Rudamentry print operation.
    CompareExp  -> ID CompareOp Exp
    Elsepart    -> 'else' Block | empty
    Type        -> 'int'
    BOPLP       -> + | -
    BOPHP       -> * | /
    CompareOp   -> < | >

*/

typedef enum { // Since I have lined up OP enum with AST for easy assignment, and since I don't want to have nodes that aren't nodes be in here.
    NODE_PLUS,
    NODE_MINUS,
    NODE_MUL,
    NODE_DIV,
    NODE_ASSIGNMENT = 8,
    NODE_LT = 10,
    NODE_GT,
    NODE_IF_ELSE = 13,
    NODE_LEAF_NUM,
    NODE_LEAF_ID,
    NODE_EMPTY,
    NODE_STMNT,
    NODE_COMPARE_EXP,
    NODE_PRINT,
    NODE_BLOCK,
    NODE_INVALID,
    NODE_NODES_NUM,
}ASTNodeType;


const char* ASTNodeNames[] = {
    "NODE_PLUS",
    "NODE_MINUS",
    "NODE_MUL",
    "NODE_DIV",
    "NODE_ERROR",
    "NODE_ERROR",
    "NODE_ERROR",
    "NODE_ERROR",
    "NODE_ASSIGNMENT",
    "NODE_ERROR",
    "NODE_LT",
    "NODE_GT",
    "NODE_ERROR",
    "NODE_IF_ELSE",
    "NODE_LEAF_NUM",
    "NODE_LEAF_ID",
    "NODE_EMPTY",
    "NODE_STMNT",
    "NODE_COMPARE_EXP",
    "NODE_PRINT",
    "NODE_BLOCK",
    "NODE_INVALID",
    "NODE_NODES_NUM",
};

static u16 numNodes = 0;

typedef struct ASTNode ASTNode;
struct ASTNode{
    union {
        struct {
            ASTNode* leftNode;
            ASTNode* rightNode;
        }BinaryOperation;
        struct{
            ASTNode* IDNode;
            ASTNode* RValueNode;
        }AssignmentOperation; // It's still binary, but I need better way to go through the AST.
        struct {
            ASTNode* conditionNode;
            ASTNode* bodyNode;
            ASTNode* elseBodyNode;
        }IfElseOperation;
        struct {
            ASTNode* currentStmntNode;
            ASTNode* nextStmntNode;
        }StmtList;
        struct {
            ASTNode* startOfCurrentBlockNode;
            ASTNode* startOfNextBlockNode;
        }BlockList;
        struct {
            ASTNode* IDNode;
            ASTNode* compareOpNode;
            ASTNode* ExpNode;
        }CompareExp;
        struct {
            ASTNode* ValueNode;
        }PrintExp;
        struct{
            SimpleString Name;
            PRIMITIVE_TYPE type;
        }ID;
        i64 number; // Or a value as a leaf node.
    }Value;
    //u16 nodeNum;
    ASTNodeType nodeType;
};

typedef struct {
    Tokens* tok;
    ASTNode* startNode;
    u16 tokenPos;
}Parser;

// All the parser functions to create the AST will have prefixes in uppercase of : E/N or nothing. If it's E -> It can return en empty node. If it's N, it can return a NULLPTR.

Parser* CreateParser(Arena* arena, Tokens* tok){
    Parser* tempParser;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, Parser, tempParser);
    if (err != ARENA_OK) {
        fprintf(stderr, "Couldn't build Parser: %s", ArenaErrorNames[err]);
        printArenaInfo(arena);
        abort();
    }
    tempParser->tok = tok;
    tempParser->tokenPos = 0;
    return tempParser;
}

_Bool CheckLParen(Parser* parser){
    fprintf(stderr, "Checking LP at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        if(currTok.TokenValue.operator == OP_LP){
            parser->tokenPos++;
            return TRUE;
        }
    }
    return FALSE;
}

_Bool CheckRParen(Parser* parser){
    fprintf(stderr, "Checking RP part at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        if(currTok.TokenValue.operator == OP_RP){
            parser->tokenPos++;
            return TRUE;
        }
    }
    return FALSE;
}

_Bool CheckLCB(Parser* parser){
    fprintf(stderr, "Checking LCB part at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        if(currTok.TokenValue.operator == OP_LC){
            parser->tokenPos++;
            return TRUE;
        }
    }
    return FALSE;
}

_Bool CheckRCB(Parser* parser){
    fprintf(stderr, "Checking RCB part at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        if(currTok.TokenValue.operator == OP_RC){
            parser->tokenPos++;
            return TRUE;
        }
    }
    return FALSE;
}

_Bool CheckSC(Parser* parser){
    fprintf(stderr, "Checking SemiColon part at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        if(currTok.TokenValue.operator == OP_SC){
            parser->tokenPos++;
            return TRUE;
        }
    }
    return FALSE;
}

_Bool CheckColon(Parser* parser){
    fprintf(stderr, "Checking Colon part at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        if(currTok.TokenValue.operator == OP_C){
            parser->tokenPos++;
            return TRUE;
        }
    }
    return FALSE;
}

ASTNode* NCheckLowPresBinaryOP(Arena* arena, Parser* parser){
    fprintf(stderr, "Checking LowPres OP at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        OPERATOR_TYPE op = currTok.TokenValue.operator;
        if(op == OP_MINUS || op == OP_PLUS){
            ASTNode* currNode;
            ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
            if (err != ARENA_OK) {
                fprintf(stderr, "Couldn't build AST: BINARY LP NODE: %s", ArenaErrorNames[err]);
                printArenaInfo(arena);
                abort();
            }
            ASTNodeType type = (ASTNodeType)parser->tok->tokens[parser->tokenPos].TokenValue.operator;
            currNode->nodeType = type;
            parser->tokenPos++;
            return currNode;
        }
    }
    return NULL;
}

ASTNode* NCheckHighPresBinaryOP(Arena* arena, Parser* parser){
    fprintf(stderr, "Checking HighPres OP at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        OPERATOR_TYPE op = currTok.TokenValue.operator;
        if(op == OP_DIV || op == OP_MUL){
            ASTNode* currNode;
            ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
            if (err != ARENA_OK) {
                fprintf(stderr, "Couldn't build AST: BINARY HP NODE: %s", ArenaErrorNames[err]);
                printArenaInfo(arena);
                abort();
            }
            ASTNodeType type = (ASTNodeType)parser->tok->tokens[parser->tokenPos].TokenValue.operator;
            currNode->nodeType = type;
            parser->tokenPos++;
            return currNode;
        }
    }
    return NULL;
}

ASTNode* CheckCompareOp(Arena* arena, Parser* parser){
    fprintf(stderr, "Checking compare OP at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        OPERATOR_TYPE op = currTok.TokenValue.operator;
        if(op == OP_GT || op == OP_LT){
            ASTNode* currNode;
            ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
            if (err != ARENA_OK) {
                fprintf(stderr, "Couldn't build AST: COMPARE PARENT NODE: %s", ArenaErrorNames[err]);
                printArenaInfo(arena);
                abort();
            }
            ASTNodeType type = (ASTNodeType)parser->tok->tokens[parser->tokenPos].TokenValue.operator;
            currNode->nodeType = type;
            parser->tokenPos++;
            return currNode;
        }
    }
    fprintf(stderr, "Expected Comparison operator at: %d", parser->tokenPos);
    abort();
}

ASTNode* NCheckNum(Arena* arena, Parser* parser){
    fprintf(stderr, "Checking NUM at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_NUMBER){
        ASTNode* currNode;
        ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
        if (err != ARENA_OK) {
            fprintf(stderr, "Couldn't build AST: LEAF NUM NODE: %s", ArenaErrorNames[err]);
            printArenaInfo(arena);
            abort();
        }
        //fprintf(stderr, "NUMBER TOKEN AT: %d\n", parser->tokenPos);
        currNode->Value.number = parser->tok->tokens[parser->tokenPos].TokenValue.numValue;
        currNode->nodeType = NODE_LEAF_NUM;
        parser->tokenPos++;
        return currNode;
    }
    return NULL;
}

ASTNode* NCheckExp(Arena* arena, Parser* parser);
ASTNode* NCheckLowPresExp(Arena* arena, Parser* parser);
ASTNode* NCheckSubExp(Arena* arena, Parser* parser);
ASTNode* NCheckHighPresExp(Arena* arena, Parser* parser);

ASTNode* NCheckFactor(Arena* arena, Parser* parser){
    fprintf(stderr, "Checking Factor at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    u16 prevTokPos = parser->tokenPos;
    ASTNode* currNode;
    if(CheckLParen(parser)){
        currNode = NCheckExp(arena, parser);
        if(currNode == NULL){
            fprintf(stderr, "EXPECTED EXP at: %d", parser->tokenPos);
            abort();
        }
        if(CheckRParen(parser)){
            return currNode;
        } else {
            fprintf(stderr, "EXPECTED ')' at: %d", parser->tokenPos);
            abort();
        }
    }
    fprintf(stderr, "Couldn't match factor.\n");
    return NULL;
}

ASTNode* ECheckBlock(Arena* arena, Parser* parser);

ASTNode* ECheckElsePart(Arena* arena, Parser* parser){
    fprintf(stderr, "Checking Else part at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    ASTNode* currNode;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
    if (err != ARENA_OK) {
        fprintf(stderr, "Couldn't build AST: ELSE NODE: %s", ArenaErrorNames[err]);
        printArenaInfo(arena);
        abort();
    }
    if(currTok.type == TOKEN_KEYWORD){
        if(currTok.TokenValue.keyword == KW_ELSE){
            parser->tokenPos++;
            currNode = ECheckBlock(arena, parser);
            if(currNode->nodeType == NODE_EMPTY){
                fprintf(stderr, "Expected else body at: %d", parser->tokenPos);
                abort();
            }
        }
        return currNode;
    }
    currNode->nodeType = NODE_EMPTY;
    return currNode; // Empty part.
}

ASTNode* NCheckID(Arena* arena, Parser* parser){
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    fprintf(stderr, "Checking ID at: %d\n", parser->tokenPos);
    if(currTok.type == TOKEN_ID){
        ASTNode* currNode;
        ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
        if (err != ARENA_OK) {
            fprintf(stderr, "Couldn't build AST: ID NODE: %s", ArenaErrorNames[err]);
            printArenaInfo(arena);
            abort();
        }
        currNode->nodeType = NODE_LEAF_ID;
        currNode->Value.ID.Name = currTok.TokenValue.stringValue;
        parser->tokenPos++;
        return currNode;
    }
    return NULL;
}

ASTNode* CheckCompareExp(Arena* arena, Parser* parser){
    fprintf(stderr, "Checking comparison exp at: %d\n", parser->tokenPos);
    ASTNode* IDNode = NCheckID(arena, parser);
    if(IDNode == NULL){
        fprintf(stderr, "Expected ID at: %d", parser->tokenPos);
        abort();
    }
    ASTNode* currNode;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
    if (err != ARENA_OK) {
        fprintf(stderr, "Couldn't build AST: ASSIGNMENT NODE: %s", ArenaErrorNames[err]);
        printArenaInfo(arena);
        abort();
    }
    currNode->nodeType = NODE_COMPARE_EXP;
    currNode->Value.CompareExp.IDNode = IDNode;
    ASTNode* CompOpNode = CheckCompareOp(arena, parser);
    currNode->Value.CompareExp.compareOpNode = CompOpNode;
    ASTNode* ExpNode = NCheckExp(arena, parser);
    if(ExpNode == NULL){
        fprintf(stderr, "Expected EXP at: %d\n", parser->tokenPos);
        abort();
    }
    currNode->Value.CompareExp.ExpNode= ExpNode;
    return currNode;
}

ASTNode* CheckEqual(Arena* arena, Parser* parser){
    fprintf(stderr, "Checking EqualExp at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        ASTNode* currNode;
        ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
        if (err != ARENA_OK) {
            fprintf(stderr, "Couldn't build AST: ASSIGNMENT NODE: %s", ArenaErrorNames[err]);
            printArenaInfo(arena);
            abort();
        }
        if(currTok.TokenValue.operator == OP_EQ){
            currNode->nodeType = NODE_ASSIGNMENT;
            parser->tokenPos++;
            return currNode;
        }
    }
    fprintf(stderr, "EXPECTED '=' at: %d", parser->tokenPos);
    abort();
}

PRIMITIVE_TYPE CheckType(Parser* parser){
    fprintf(stderr, "Checking type at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_PRIM_TYPE){
        parser->tokenPos++;
        return currTok.TokenValue.primType;
    } else {
        fprintf(stderr, "Expected type at: %d", parser->tokenPos);
        abort();
    }
}

ASTNode* NCheckPrintExp(Arena* arena, Parser* parser){
    fprintf(stderr, "Checking PrintExp at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    u16 prevPos = parser->tokenPos;
    if(currTok.type == TOKEN_KEYWORD){
        ASTNode* currNode;
        if (currTok.TokenValue.keyword == KW_PRINT) {
            ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
            if (err != ARENA_OK) {
                fprintf(stderr, "Couldn't build AST: KEYWORD NODE: %s", ArenaErrorNames[err]);
                printArenaInfo(arena);
                abort();
            }
        } else {
            return NULL;
        }
        currNode->nodeType = NODE_PRINT;
        parser->tokenPos++;
        _Bool isThere;
        isThere = CheckLParen(parser);
        if(!isThere){
            fprintf(stderr, "EXPECTED '(' at: %d", parser->tokenPos);
            abort();
        }
        ASTNode* printValueNode = NCheckExp(arena, parser);
        if(printValueNode == NULL){
            printValueNode = NCheckID(arena, parser);
            if(printValueNode == NULL){
                fprintf(stderr, "Expected print contents at: %d\n", parser->tokenPos);
                abort();
            }
        }
        isThere = CheckRParen(parser);
        if(!isThere){
            fprintf(stderr, "EXPECTED ')' at: %d", parser->tokenPos);
            abort();
        }
        currNode->Value.PrintExp.ValueNode = printValueNode;
        return currNode;
    }
    parser->tokenPos = prevPos;
    return NULL;
}

ASTNode* NCheckAssignExp(Arena* arena, Parser* parser){
    fprintf(stderr, "Checking AssignExp at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    ASTNode* idNode = NCheckID(arena, parser);
    if(idNode == NULL)
        return NULL;
    if(CheckColon(parser)){
        PRIMITIVE_TYPE type = CheckType(parser);
        idNode->Value.ID.type = type;
        ASTNode* equalNode = CheckEqual(arena, parser);
        ASTNode* valNode = NCheckExp(arena, parser);
        if(valNode == NULL){
            fprintf(stderr, "Expected EXP at: %d\n", parser->tokenPos);
            abort();
        }
        equalNode->Value.AssignmentOperation.IDNode = idNode;
        equalNode->Value.AssignmentOperation.RValueNode = valNode;
        return equalNode;
    }
    fprintf(stderr, "Expected ':' at: %d", parser->tokenPos);
    abort();
}

ASTNode* NCheckIfExp(Arena* arena, Parser* parser){
    fprintf(stderr, "Checking IfExp at: %d\n", parser->tokenPos);
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    u16 prevPos = parser->tokenPos;
    if(currTok.type == TOKEN_KEYWORD){
        ASTNode* currNode;
        if (currTok.TokenValue.keyword == KW_IF) {
            ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
            if (err != ARENA_OK) {
                fprintf(stderr, "Couldn't build AST: KEYWORD NODE: %s", ArenaErrorNames[err]);
                printArenaInfo(arena);
                abort();
            }
        } else {
            return NULL;
        }
        currNode->nodeType = NODE_IF_ELSE;
        parser->tokenPos++;
        _Bool isThere;
        isThere = CheckLParen(parser);
        if(!isThere){
            fprintf(stderr, "EXPECTED '(' at: %d", parser->tokenPos);
            abort();
        }
        ASTNode* conditionNode = CheckCompareExp(arena, parser);
        isThere = CheckRParen(parser);
        if(!isThere){
            fprintf(stderr, "EXPECTED ')' at: %d", parser->tokenPos);
            abort();
        }
        ASTNode* bodyNode = ECheckBlock(arena, parser);
        ASTNode* elsePart = ECheckElsePart(arena, parser); // If it's NULL, then the other analysers will do something with it.
        currNode->Value.IfElseOperation.conditionNode = conditionNode;
        currNode->Value.IfElseOperation.bodyNode = bodyNode;
        currNode->Value.IfElseOperation.elseBodyNode = elsePart;
        return currNode;
    }
    parser->tokenPos = prevPos;
    return NULL;
}

ASTNode* NCheckHighPresExp(Arena* arena, Parser* parser){
    u16 prevPos = parser->tokenPos;
    ASTNode* currNode;
    currNode = NCheckHighPresBinaryOP(arena, parser);
    if(currNode != NULL){
        ASTNode* subExpNode = NCheckSubExp(arena, parser);
        if(subExpNode == NULL){
            fprintf(stderr, "Expected SUBEXP at: %d\n", parser->tokenPos);
            abort();
        }
        return currNode->Value.BinaryOperation.rightNode = subExpNode;
    }
    return NULL;
}

ASTNode* NCheckLowPresExp(Arena* arena, Parser* parser){
    u16 prevPos = parser->tokenPos;
    ASTNode* currNode;
    currNode = NCheckLowPresBinaryOP(arena, parser);
    if(currNode != NULL){
        ASTNode* subExpNode = NCheckSubExp(arena, parser);
        if(subExpNode == NULL){
            fprintf(stderr, "Expected SUBEXP at: %d\n", parser->tokenPos);
            abort();
        }
        ASTNode* lowPresExpNode = NCheckLowPresExp(arena, parser);
        if(lowPresExpNode == NULL){
            currNode->Value.BinaryOperation.rightNode = subExpNode;
            return currNode;
        } else {
            lowPresExpNode->Value.BinaryOperation.leftNode = subExpNode;
            currNode->Value.BinaryOperation.rightNode = lowPresExpNode;
            return currNode;
            }
        }
    return NULL;
}

ASTNode* NCheckSubExp(Arena* arena, Parser* parser){
    u16 prevPos = parser->tokenPos;
    fprintf(stderr, "Checking Exp at: %d\n", prevPos);
    ASTNode* currNode;
    currNode = NCheckFactor(arena, parser);
    if(currNode != NULL){
        ASTNode* highPresExpNode = NCheckHighPresExp(arena, parser);
        if(highPresExpNode == NULL)
            return currNode;
        highPresExpNode->Value.BinaryOperation.leftNode = currNode;
        return highPresExpNode;
    }
    parser->tokenPos = prevPos;
    currNode = NCheckNum(arena, parser);
    if(currNode != NULL){
        ASTNode* highPresExpNode = NCheckHighPresExp(arena, parser);
        if(highPresExpNode != NULL){
            highPresExpNode->Value.BinaryOperation.leftNode = currNode;
            return highPresExpNode;
        } else {
            return currNode;
        }
    }
    parser->tokenPos = prevPos;
    currNode = NCheckID(arena, parser);
    if(currNode != NULL){
        ASTNode* highPresExpNode = NCheckHighPresExp(arena, parser);
        if(highPresExpNode != NULL){
            highPresExpNode->Value.BinaryOperation.leftNode = currNode;
            return highPresExpNode;
        } else {
            return currNode;
        }
    }
    fprintf(stderr, "EXPECTED SUBEXP at: %d", prevPos);
    abort();
}

ASTNode* NCheckExp(Arena* arena, Parser* parser){
    fprintf(stderr, "Checking Exp at: %d\n", parser->tokenPos);
    ASTNode* SubExpNode = NCheckSubExp(arena, parser);
    if(SubExpNode == NULL)
        return NULL;
    ASTNode* LowPresExpNode = NCheckLowPresExp(arena, parser);
    if(LowPresExpNode == NULL){
        return SubExpNode;
    } else {
        LowPresExpNode->Value.BinaryOperation.leftNode = SubExpNode;
        return LowPresExpNode;
    }
}

ASTNode* ECheckStmnt(Arena* arena, Parser* parser);

ASTNode* NCheckNuStmnt(Arena* arena, Parser* parser, _Bool AfterStatement){
    u16 prevPos = parser->tokenPos;
    fprintf(stderr, "Checking Nustmt at: %d\n", prevPos);
    ASTNode* currNode;
    if(CheckSC(parser)){
        currNode = ECheckStmnt(arena, parser);
        return currNode;
    }
    if (AfterStatement) {
        fprintf(stderr, "EXPECTED ';' AT: %d", parser->tokenPos);
        abort();
    }
    else {
        return NULL;
    }
}

ASTNode* ECheckStmnt(Arena* arena, Parser* parser){
    fprintf(stderr, "Checking statement at: %d\n", parser->tokenPos);
    u16 prevPos = parser->tokenPos;
    ASTNode* currNode;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
    if(err != ARENA_OK){
        fprintf(stderr, "Can't make Stmnt Arena: %s", ArenaErrorNames[err]);
        printArenaInfo(arena);
        abort();
    }
    currNode->nodeType = NODE_STMNT;
    ASTNode* currStmntNode;
    _Bool statementParsed = TRUE;
    currStmntNode = NCheckAssignExp(arena, parser);
    if(currStmntNode == NULL){
        parser->tokenPos = prevPos;
        currStmntNode = NCheckIfExp(arena, parser);
    }
    if(currStmntNode == NULL){
        parser->tokenPos = prevPos;
        currStmntNode = NCheckPrintExp(arena, parser);
    }
    if(currStmntNode == NULL){
        statementParsed = FALSE;
        //POP_DATA_IN_ARENA(arena, ASTNode, 1);
    }
    ASTNode* nextStmntNode = NCheckNuStmnt(arena, parser, statementParsed);
    if(nextStmntNode == NULL){
        if(!statementParsed){
            currNode->nodeType = NODE_EMPTY;
            return currNode;
        }
    }
    currNode->Value.StmtList.currentStmntNode = currStmntNode;
    currNode->Value.StmtList.nextStmntNode = nextStmntNode;
    return currNode;
}

ASTNode* ECheckNuBlock(Arena* arena, Parser* parser){
    fprintf(stderr, "Checking nublock at: %d\n", parser->tokenPos);
    ASTNode* currNode;
    return ECheckBlock(arena, parser);
}

ASTNode* ECheckBlock(Arena* arena, Parser* parser){
    fprintf(stderr, "Checking block at: %d\n", parser->tokenPos);
    u16 prevPos = parser->tokenPos;
    ASTNode* currNode;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
    if(err != ARENA_OK){
        fprintf(stderr, "Can't make Block Arena: %s", ArenaErrorNames[err]);
        printArenaInfo(arena);
        abort();
    }
    currNode->nodeType = NODE_BLOCK;
    _Bool BlockParsed = TRUE;
    if(CheckLCB(parser)){
        ASTNode* FirstStatementNode = ECheckStmnt(arena, parser);
        if(!CheckRCB(parser)){
            fprintf(stderr, "EXPECTED '}' at: %d", parser->tokenPos);
            abort();
        }
        currNode->Value.BlockList.startOfCurrentBlockNode = FirstStatementNode;
        currNode->Value.BlockList.startOfNextBlockNode = ECheckNuBlock(arena, parser);
    } else {
        currNode->nodeType = NODE_EMPTY;
    }
    return currNode;
}

void StartParsing(Arena* arena, Parser* parser){
    parser->startNode = ECheckBlock(arena, parser);
    if(parser->startNode != NULL){
        fprintf(stderr, "PARSED CORRECTLY\n");
    } else fprintf(stderr, "Empty program");
}

#define MAX_SYMBOL_STACK_SIZE 255 // Remember when making this, when exiting a scope, all the symbols should be destroyed.
typedef struct{
    SimpleString*  name;
    PRIMITIVE_TYPE type;
    i16            offsetInCPUStack; // in bytes.
}Symbol;

typedef struct{ // This stack will firstly be used for semantic analysis, then it will be used as a runtime stack.
    Symbol data[MAX_SYMBOL_STACK_SIZE];
    i16    topStackPointer;
}SymbolStack;

typedef enum{
    STACK_OVERFLOW,
    STACK_UNDERFLOW,
    STACK_SYMBOL_NOT_FOUND,
    STACK_SYMBOL_FOUND,
    STACK_OK,
}STACK_ERR;

const char* StackErrorNames[] = {
    "STACK_OVERFLOW",
    "STACK_UNDERFLOW",
    "STACK_SYMBOL_NOT_FOUND",
    "STACK_SYMBOL_FOUND",
    "STACK_OK",
};

SymbolStack* CreateSymbolStack(Arena* arena){
    SymbolStack* temp;
    PUSH_EMPTY_ARRAY_IN_ARENA(arena, SymbolStack, 1, temp);
    temp->topStackPointer = 0;
    return temp;
}

i16 GetStackTop(SymbolStack* stack){
    return stack->topStackPointer;
}

STACK_ERR PushSymbolToStack(SymbolStack* stack, Symbol symbol){
    if((stack->topStackPointer + 1) >= MAX_SYMBOL_STACK_SIZE){
        return STACK_OVERFLOW;
    }
    stack->data[stack->topStackPointer] = symbol;
    stack->topStackPointer++;
    return STACK_OK;
}

STACK_ERR PopSymbolFromStack(SymbolStack* stack){
    if(stack->topStackPointer < 0){
        return STACK_UNDERFLOW;
    }
    stack->topStackPointer--;
    return STACK_OK;
}

STACK_ERR FindSymbolInStack(SymbolStack* stack, SimpleString* ID, i16* stackPos){ // Starting trying to find the symbol from the top, so newer scope will have it in.
    _Bool found = FALSE;
    i16   currentSymbol = stack->topStackPointer - 1;
    while(found != TRUE && currentSymbol >= 0){
        found = MatchStringIndirect(stack->data[currentSymbol].name, ID);
        currentSymbol--;
    }
    if(!found)
        return STACK_SYMBOL_NOT_FOUND;
    *stackPos = currentSymbol;
    return STACK_SYMBOL_FOUND;
}

STACK_ERR SetTopPointer(SymbolStack* stack, u16 newTop){ // This will be used when going out of scope.
    if(newTop >= MAX_SYMBOL_STACK_SIZE)
        return STACK_OVERFLOW;
    if(newTop < 0)
        return STACK_UNDERFLOW;
    stack->topStackPointer = newTop;
    return STACK_OK;
}

i64 GetIDOffsetInStack(SymbolStack* stack, SimpleString* ID){
    i16* symbolPosInStack;
    STACK_ERR err = FindSymbolInStack(stack, ID, symbolPosInStack);
    if(err != STACK_OK){
        fprintf(stderr, "Runtime error: %s with symbol: ", StackErrorNames[err]);
        PrintSimpleString(ID);
        fprintf(stderr, "\n");
        abort();
    }
    return stack->data[*symbolPosInStack].offset;
}

// Check Type (Not useful right now as we only have one type).
// Check Symbol existence.
// Remember the stack state everytime we go inside a new block. // Also remember in which scope the analysis is taking place, such that redefinition doesn't cause errors.
// Anything less than the scope pointer is from a scope that is without-it. It can still access that variable, but it can also replace it.

_Bool AnalyzeBinaryOp(SymbolStack* stack, i16 currentScopePointer, ASTNode* node);

_Bool AnalyzeCompOp(SymbolStack* stack, i16 currentScopePointer, ASTNode* node);

_Bool AnalyzeIfElse(SymbolStack* stack, i16 currentScopePointer, ASTNode* node);

_Bool AnalyzePrint(SymbolStack* stack, i16 currentScopePointer, ASTNode* node);

_Bool AnalyzeStmnt(SymbolStack* stack, i16 currentScopePointer, ASTNode* node);

_Bool AnalyzeBlock(SymbolStack* stack, i16 currentScopePointer, ASTNode* node);

void AnalyzeID(SymbolStack* stack, i16 currentScopePointer, ASTNode* node, _Bool toPush){
    fprintf(stderr, "Analyzing ID:\n");
    Symbol ID;
    ID.name = &node->Value.ID.Name;
    ID.type = node->Value.ID.type;
    i16 symbolPos = -1;
    PrintSimpleString(ID.name);
    fprintf(stderr, "\n");
    STACK_ERR err = FindSymbolInStack(stack, &node->Value.ID.Name, &symbolPos);
    if(toPush){
        if (err == STACK_SYMBOL_FOUND || err == STACK_SYMBOL_NOT_FOUND ) {
            if(currentScopePointer < symbolPos) { // The symbol was found within the scope.
                fprintf(stderr, "Redefinition of symbol: ");
                PrintSimpleString(ID.name);
                fprintf(stderr, "\n");
                abort();
            }
                err = PushSymbolToStack(stack, ID);
                fprintf(stderr, "^ Pushing ID.\n");
                if(err == STACK_OVERFLOW){
                    fprintf(stderr, "Stack overflow.\n");
                    abort();
                } else
                    return;
        } else {
            fprintf(stderr, "Unkown error: %s", StackErrorNames[err]);
            abort();
        }
    } else {
        if(err == STACK_SYMBOL_NOT_FOUND){
            fprintf(stderr, "Undefined symbol: ");
            PrintSimpleString(ID.name);
            fprintf(stderr, "\n");
            abort();
        }
        if(err == STACK_SYMBOL_FOUND){
            return;
        } else {
            fprintf(stderr, "Unkown error: %s", StackErrorNames[err]);
            abort();
        }
    }
}

_Bool AnalyzeAssignment(SymbolStack* stack, i16 currentScopePointer, ASTNode* node){
    fprintf(stderr, "Analyzing Assignment.\n");
    ASTNode* IDNode = node->Value.AssignmentOperation.IDNode;
    AnalyzeID(stack, currentScopePointer, IDNode, TRUE);
    ASTNodeType RValueNodeType = node->Value.AssignmentOperation.RValueNode->nodeType;
    fprintf(stderr, "The RValue node is: %s\n", ASTNodeNames[RValueNodeType]);
    switch (RValueNodeType) {
        case NODE_INVALID  :
        case NODE_LEAF_NUM :
        case NODE_EMPTY    : {
            return TRUE;
        }
        case NODE_DIV   :
        case NODE_PLUS  :
        case NODE_MINUS :
        case NODE_MUL   : {
            return AnalyzeBinaryOp(stack, currentScopePointer, node->Value.AssignmentOperation.RValueNode);
        }
        case NODE_LEAF_ID : {
            AnalyzeID(stack, currentScopePointer, node->Value.AssignmentOperation.RValueNode, FALSE);
            break;
        }
    }
    return TRUE;
}

_Bool AnalyzeBinaryOp(SymbolStack* stack, i16 currentScopePointer, ASTNode* node){
    fprintf(stderr, "Analyzing BinaryOp.\n");
    ASTNodeType LeftNodeType = node->Value.BinaryOperation.leftNode->nodeType;
    _Bool LeftSuccess = TRUE;
    switch (LeftNodeType) {
        case NODE_INVALID  :
        case NODE_LEAF_NUM :
        case NODE_EMPTY    : {
            return TRUE;
        }
        case NODE_DIV   :
        case NODE_PLUS  :
        case NODE_MINUS :
        case NODE_MUL   : {
            LeftSuccess = AnalyzeBinaryOp(stack, currentScopePointer, node->Value.BinaryOperation.leftNode);
            break;
        }
        case NODE_LEAF_ID : {
            AnalyzeID(stack, currentScopePointer, node->Value.BinaryOperation.leftNode, FALSE);
            break;
        }
    }
    ASTNodeType RightNodeType = node->Value.BinaryOperation.rightNode->nodeType;
    _Bool RightSuccess = TRUE;
    switch (RightNodeType) {
        case NODE_INVALID  :
        case NODE_LEAF_NUM :
        case NODE_EMPTY    : {
            return TRUE;
        }
        case NODE_DIV   :
        case NODE_PLUS  :
        case NODE_MINUS :
        case NODE_MUL   : {
            RightSuccess = AnalyzeBinaryOp(stack, currentScopePointer, node->Value.BinaryOperation.rightNode);
            break;
        }
        case NODE_LEAF_ID : {
            AnalyzeID(stack, currentScopePointer, node->Value.BinaryOperation.rightNode, FALSE);
            break;
        }
    }
    return (LeftSuccess && RightSuccess);
}

_Bool AnalyzeCompOp(SymbolStack* stack, i16 currentScopePointer, ASTNode* node){
    fprintf(stderr, "Analyzing CompOp.\n");
    ASTNode* IDNode = node->Value.CompareExp.IDNode;
    AnalyzeID(stack, currentScopePointer, IDNode, FALSE);
    fprintf(stderr, "Analyzed ID.\n");
    ASTNodeType CompWithType = node->Value.CompareExp.ExpNode->nodeType;
    switch (CompWithType) {
        case NODE_INVALID  :
        case NODE_LEAF_NUM :
        case NODE_EMPTY    : {
            return TRUE;
        }
        case NODE_DIV   :
        case NODE_PLUS  :
        case NODE_MINUS :
        case NODE_MUL   : {
            return AnalyzeBinaryOp(stack, currentScopePointer, node->Value.AssignmentOperation.RValueNode);
        }
        case NODE_LEAF_ID : {
            AnalyzeID(stack, currentScopePointer, node->Value.AssignmentOperation.RValueNode, FALSE);
            break;
        }
    }
    return TRUE;
}

_Bool AnalyzeIfElse(SymbolStack* stack, i16 currentScopePointer, ASTNode* node){
    fprintf(stderr, "Analyzing IfElse.\n");
    i16 newScopePointer = GetStackTop(stack);
    _Bool Condition = AnalyzeCompOp(stack, currentScopePointer, node->Value.IfElseOperation.conditionNode);
    _Bool BodyBlock = AnalyzeBlock(stack, newScopePointer, node->Value.IfElseOperation.bodyNode);
    SetTopPointer(stack, newScopePointer);
    newScopePointer = currentScopePointer; // Ditching all the symbols in that scope.
    ASTNodeType elseBodyType = node->Value.IfElseOperation.elseBodyNode->nodeType;
    if(elseBodyType == NODE_EMPTY){
        return Condition && BodyBlock;
    } else {
        newScopePointer = GetStackTop(stack);
        _Bool ElseBlock = AnalyzeBlock(stack, newScopePointer, node->Value.IfElseOperation.elseBodyNode);
        SetTopPointer(stack, newScopePointer);
        newScopePointer = currentScopePointer; // Ditching all the symbols in that scope.
        return Condition && BodyBlock && ElseBlock;
    }
}

_Bool AnalyzePrint(SymbolStack* stack, i16 currentScopePointer, ASTNode* node){
    fprintf(stderr, "Analyzing Print.\n");
    ASTNode* valueNode =  node->Value.PrintExp.ValueNode;
    if(valueNode->nodeType == NODE_LEAF_ID){
        AnalyzeID(stack, currentScopePointer, valueNode, FALSE);
        return TRUE;
    }
    ASTNodeType ValueNodeType = valueNode->nodeType;
    fprintf(stderr, "The RValue node is: %s\n", ASTNodeNames[ValueNodeType]);
    switch (ValueNodeType) {
        case NODE_INVALID  :
        case NODE_LEAF_NUM :
        case NODE_EMPTY    : {
            return TRUE;
        }
        case NODE_DIV   :
        case NODE_PLUS  :
        case NODE_MINUS :
        case NODE_MUL   : {
            return AnalyzeBinaryOp(stack, currentScopePointer, node->Value.PrintExp.ValueNode);
        }
        case NODE_LEAF_ID : {
            AnalyzeID(stack, currentScopePointer, node->Value.PrintExp.ValueNode, FALSE);
            break;
        }
    }
    return TRUE;
}

_Bool AnalyzeStmnt(SymbolStack* stack, i16 currentScopePointer, ASTNode* node){
    fprintf(stderr, "Analyzing Statement.\n");
    ASTNodeType CurrentStmntType = node->Value.StmtList.currentStmntNode->nodeType;
    ASTNode* CurrentStmntNode = node->Value.StmtList.currentStmntNode;
    _Bool CurrentStatementAnalysis;
    switch (CurrentStmntType) {
        case NODE_INVALID  :
        case NODE_LEAF_NUM :
        case NODE_EMPTY    : {
            CurrentStatementAnalysis = TRUE;
            break;
        }
        case NODE_DIV   :
        case NODE_PLUS  :
        case NODE_MINUS :
        case NODE_MUL   : {
            CurrentStatementAnalysis = AnalyzeBinaryOp(stack, currentScopePointer, node->Value.AssignmentOperation.RValueNode);
            break;
        }
        case NODE_LEAF_ID : {
            AnalyzeID(stack, currentScopePointer, node->Value.AssignmentOperation.RValueNode, FALSE);
            break;
        }
        case NODE_ASSIGNMENT: {
            CurrentStatementAnalysis = AnalyzeAssignment(stack, currentScopePointer, CurrentStmntNode);
            break;
        }
        case NODE_IF_ELSE: {
            CurrentStatementAnalysis = AnalyzeIfElse(stack, currentScopePointer, CurrentStmntNode);
            break;
        }
        case NODE_PRINT: {
            CurrentStatementAnalysis = AnalyzePrint(stack, currentScopePointer, CurrentStmntNode);
            break;
        }
    }
    ASTNodeType NextStmntType = node->Value.StmtList.nextStmntNode->nodeType;
    ASTNode* NextStmntNode = node->Value.StmtList.nextStmntNode;
    fprintf(stderr, "The next statement node is: %s\n", ASTNodeNames[NextStmntType]);
    // The next statement will always be another statement.
    _Bool NextStatementAnalysis;
    switch (NextStmntType) {
        case NODE_INVALID  :
        case NODE_EMPTY    : {
            NextStatementAnalysis = TRUE;
            break;
        }
        case NODE_STMNT: {
            NextStatementAnalysis = AnalyzeStmnt(stack, currentScopePointer, NextStmntNode);
            break;
        }
        default: {
            NextStatementAnalysis = FALSE;
        }
    }
    return CurrentStatementAnalysis && NextStatementAnalysis;
}

_Bool AnalyzeBlock(SymbolStack* stack, i16 currentScopePointer, ASTNode* node){
    fprintf(stderr, "Analyzing Block.\n");
    ASTNodeType CurrentBlockType = node->Value.BlockList.startOfCurrentBlockNode->nodeType;
    fprintf(stderr, "The current block node is: %s\n", ASTNodeNames[CurrentBlockType]);
    ASTNode* CurrentBlockNode = node->Value.BlockList.startOfCurrentBlockNode;
    _Bool CurrentBlockAnalysis;
    switch (CurrentBlockType) {
        case NODE_INVALID  :
        case NODE_EMPTY    : {
            CurrentBlockAnalysis = TRUE;
            break;
        }
        case NODE_STMNT: {
            CurrentBlockAnalysis = AnalyzeStmnt(stack, currentScopePointer, CurrentBlockNode);
            break;
        }
        default: {
            break;
        }
    }
    i16 newScopePointer = GetStackTop(stack);
    ASTNodeType NextBlockType = node->Value.BlockList.startOfNextBlockNode->nodeType;
    ASTNode* NextBlockNode = node->Value.BlockList.startOfNextBlockNode;
    _Bool NextBlockAnalysis;
    switch (NextBlockType) {
        case NODE_INVALID  :
        case NODE_EMPTY    : {
            NextBlockAnalysis = TRUE;
            break;
        }
        case NODE_BLOCK: {
            NextBlockAnalysis = AnalyzeBlock(stack, currentScopePointer, CurrentBlockNode);
            break;
        }
        default: {
            return NextBlockAnalysis = FALSE;
        }
    }
    return CurrentBlockAnalysis && NextBlockAnalysis;
}

_Bool StartSemanticAnalysis(SymbolStack* stack, ASTNode* node){
    fprintf(stderr, "Started analysis.\n");
    if(node == NULL){
        fprintf(stderr, "Empty program.\n");
        return FALSE;
    }
    i16 currentScopePointer = GetStackTop(stack);
    return AnalyzeBlock(stack, currentScopePointer, node);
}

// Type inference??

// Simple emitter.

typedef struct{
    FILE* outputFile;
    SymbolStack* runTimeStack;
    i16 stackTop;
}Emitter;

// was thinking of using the normal operatornames array but idk.
const char* opNASMIns[] = {
    "add",
    "sub",
    "mul",
    "div"
};

Emitter* CreateEmitter(Arena* arena){
    fprintf(stderr, "Started analysis.\n");
    Emitter* temp;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, Emitter, temp);
    if(err != ARENA_OK){
        fprintf(stderr, "Could not make Emitter Arena: %s", ArenaErrorNames[err]);
        abort();
    }
    temp->outputFile = fopen("tempasmfile", "w"); // Creating a temp file.
    if (temp->outputFile == NULL) {
        fprintf(stderr, "Error! opening file");
        abort();
    }
    temp->runTimeStack = CreateSymbolStack(arena);
    fprintf(temp->outputFile, "section .text\n");
    fprintf(temp->outputFile, "global _start\n");
    fprintf(temp->outputFile, "_start:\n");
    fprintf(temp->outputFile, "mov rbp, rsp:\n");
    return temp;
}

// Pretty sure scope isn't useful.
void EmitBinaryOperation(Emitter* emitter, i16 scope, ASTNode* node);
void EmitIfStatement(Emitter* emitter, i16 scope, ASTNode* node);
void EmitAssignment(Emitter* emitter, i16 scope, ASTNode* node); // Push to stack. // Evaluate R-Value and then push it to the stack.
void EmitStatement(Emitter* emitter, i16 scope, ASTNode* node);
void EmitPrint(Emitter* emitter, i16 scope, ASTNode* node);


// I FINALLY UNDERSTAND WHY SSAs.

void EmitLoadSymbol(Emitter* emitter, const char* reg, ASTNode* node){
    i16 symbolInScope;
    STACK_ERR serr = FindSymbolInStack(emitter->runTimeStack, &node->Value.ID.Name, &symbolInScope);
    if(serr != STACK_OK){
        fprintf(stderr, "Runtime stack err: %s", StackErrorNames[serr]);
        abort();
    }
    fprintf(emitter->outputFile, "mov %s, [rbp-%d]\n", reg, emitter->runTimeStack->data[symbolInScope].offsetInCPUStack);
}

void EmitLoadNum(Emitter* emitter, const char* reg, ASTNode* node){
    fprintf(emitter->outputFile, "mov %s, %"PRId64"\n", reg, node->Value.number);
}


void EmitBinaryOperation(Emitter* emitter, i16 scope, ASTNode* node){
    ASTNodeType binNodeType = node->nodeType;
    ASTNode* leftNode = node->Value.BinaryOperation.leftNode;
    ASTNodeType leftNodeType = node->Value.BinaryOperation.leftNode->nodeType;
    ASTNode* rightNode = node->Value.BinaryOperation.rightNode;
    ASTNodeType rightNodeType = node->Value.BinaryOperation.rightNode->nodeType;
    switch (leftNodeType) {
        case NODE_LEAF_ID: {
            EmitLoadSymbol(emitter, "rax", leftNode);
            break;
        }
        case NODE_LEAF_NUM: {
            EmitLoadNum(emitter, "rax", leftNode);
            break;
        }
        default: EmitBinaryOperation(emitter, scope, leftNode);
    }
    fprintf(emitter->outputFile, "push rax\n");
    switch (rightNodeType) {
        case NODE_LEAF_ID: {
            EmitLoadSymbol(emitter, "rax", rightNode);
            break;
        }
        case NODE_LEAF_NUM: {
            EmitLoadNum(emitter, "rax", rightNode);
            break;
        }
        default: EmitBinaryOperation(emitter, scope, rightNode);
    }
    fprintf(emitter->outputFile, "push rax\n"); // Basically, if I say emit ID to RBX, then either the right node will be at rbx or it'll be a binary operation with results stored in rax.

    fprintf(emitter->outputFile, "pop rbx\n");
    fprintf(emitter->outputFile, "pop rax\n");
    switch(binNodeType){
        case NODE_DIV:
            fprintf(emitter->outputFile, "xor rdx, rdx\n");
        case NODE_MUL: {
            fprintf(emitter->outputFile, "%s rbx\n", opNASMIns[binNodeType]);
            break;
        }
        case NODE_PLUS:
        case NODE_MINUS: {
            fprintf(emitter->outputFile, "%s rax, rbx\n", opNASMIns[binNodeType]);
            break;
        }
    }
}

void EmitAssignment(Emitter* emitter, i16 scope, ASTNode* node){
    ASTNodeType RValueType = node->Value.AssignmentOperation.RValueNode->nodeType;
    ASTNode* RValueNode = node->Value.AssignmentOperation.RValueNode;
    switch (RValueType) {
        case NODE_LEAF_ID: {
            EmitLoadSymbol(emitter, "rax", RValueNode);
            break;
        }
        case NODE_LEAF_NUM: {
            EmitLoadNum(emitter, "rax", RValueNode);
            break;
        }
        default: EmitBinaryOperation(emitter, scope, RValueNode); break;
    }
    fprintf(emitter->outputFile, "push rax\n");
    Symbol ID;
    ID.name = &node->Value.AssignmentOperation.IDNode->Value.ID.Name;
    ID.type = node->Value.AssignmentOperation.IDNode->Value.ID.type;
    ID.offsetInCPUStack = emitter->stackTop;
    emitter->stackTop += 8; // cause 8 bytes each.
    PushSymbolToStack(emitter->runTimeStack, ID);
}

void EmitPrint(Emitter* emitter, i16 scope, ASTNode* node);

char* ReadInputFile(Arena* arena, const char* fileName){
    FILE* fp = fopen(fileName, "rb");
    if(!fp){
        fprintf(stderr, "Can't open the file: %s", fileName);
        abort();
    }
    if(fseek(fp, 0, SEEK_END) != 0){
        fprintf(stderr, "Can't find file end: %s", fileName);
        fclose(fp);
        abort();
    }
    long size = ftell(fp);
    if (size < 0){
        fprintf(stderr, "Invalid file size: %s", fileName);
        fclose(fp);
        abort();
    }
    rewind(fp);
    char* buffer;
    ARENA_ERROR err = makeArena(arena, (size + 1));
    if(err != ARENA_OK){
        fprintf(stderr, "Could not make Input file Arena: %s", ArenaErrorNames[err]);
        abort();
    }
    err = PUSH_EMPTY_ARRAY_IN_ARENA(arena, char, (size + 1), buffer);
    if(err != ARENA_OK){
        fprintf(stderr, "Could not allocate in arena: %s", ArenaErrorNames[err]);
        printArenaInfo(arena);
        abort();
    }
    size_t read_size = fread(buffer, 1, size, fp);
    if (read_size != (size_t)size){
        fprintf(stderr, "Could not copy the whoel file: %s", fileName);
        removeArena(arena);
        fclose(fp);
        abort();
    }
    buffer[size] = '\0'; // For safety.
    fclose(fp);
    return buffer;
}

void ParseCMDArgs(int numArgs, char* args[]){
    if(numArgs <= 1){
        fprintf(stderr, "Please specify actions for the compiler, such as:\n");
        fprintf(stderr, "                                                 `build`!:\n");
        abort();
    }
    if(numArgs <= 2){
        if(!strcmp(args[1], "build")){
            fprintf(stderr, "Please specify the path of input file to be compiled!\n");
            abort();
        } else {
            fprintf(stderr, "Expected correct action for the compiler!\n");
            abort();
        }
    }
    if(numArgs > 3){
        fprintf(stderr, "Unexpected arguments.\n");
        abort();
    }
    if(numArgs == 3){
        if(!strcmp(args[1], "build"))
            return;
        else {
            fprintf(stderr, "Expected correct action for the compiler!\n");
            abort();
        }
    }
}

//TODO : Am going to make an assembly emitter. Will make assembly helper functions and funcitons to emit assembly.
// Am thinking if I should do SSA? If I fail to emit assembly through the AST then I will.
// I think I understood why SSAs are important, especially for a serious language where we may want to do optmizations, even naive ones we can think of ourselves, or more complex ones that have been documented and are shared knowledge.
// TODO : Will practice assembly for a day or so.

//TODO : Inference and operator precedence.

int main(int numArgs, char* args[]){
    ParseCMDArgs(numArgs, args);
    Arena inputFileArena;
    const char* sampleProgram = ReadInputFile(&inputFileArena, args[2]);
    Arena LexerArena;
    ARENA_ERROR err = makeArena(&LexerArena, KiB(2));
    if(err != ARENA_OK){
        fprintf(stderr, "Can't make Lexer Arena: %s", ArenaErrorNames[err]);
        return 0;
    }
    DFA* arithDFA = CreateDFA(&LexerArena);
    Tokenizer* mainTokenizer = CreateTokenizer(&LexerArena, sampleProgram);
    Tokens* tokenizedProgram = StartTokenizing(&LexerArena, arithDFA, mainTokenizer);
    printTokens(tokenizedProgram);
    // Making an arena for the parser becuase the info from the Lexer's tokenized table will be copied and transformed in the parser.
    Arena ParserArena;
    // We cannot remove this arena right now because the ID nodes reference it.
    //removeArena(&inputFileArena);
    err = makeArena(&ParserArena, KiB(10));
    if(err != ARENA_OK){
        fprintf(stderr, "Can't make Parser Arena: %s", ArenaErrorNames[err]);
        return 0;
    }
    Parser* mainParser = CreateParser(&ParserArena, tokenizedProgram);
    StartParsing(&ParserArena, mainParser);
    SymbolStack* mainStack = CreateSymbolStack(&ParserArena);
    _Bool Analysis = StartSemanticAnalysis(mainStack, mainParser->startNode);
    if(Analysis){
        fprintf(stderr, "Analysis true!");
    }
    removeArena(&LexerArena);
    removeArena(&ParserArena);
    return 0;
}
