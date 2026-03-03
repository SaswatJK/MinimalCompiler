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
    printf("%.*s", source->length, source->start);
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

// NOTE : Automation for the future.
DFA* CreateDFA(Arena* arena){
    DFA* tempDFA;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, DFA, tempDFA);
    if (err != ARENA_OK) {
        printf("Couldn't build DFA!");
        abort();
    }
    int tableIndex;
    // First row.
    tableIndex = D_STATE_S * DFAInputNum + Digit;
    tempDFA->state[tableIndex] = D_STATE_I;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_O;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_F;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_ST;

    // Second row.
    tableIndex = D_STATE_I * DFAInputNum + Digit;
    tempDFA->state[tableIndex] = D_STATE_I;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_NOT_ERROR_STATE;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_NOT_ERROR_STATE;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_WRONG_INPUT; // Can't have number concatenated with characters, that start from numbers. (Even the inverse isn't supported right now.

    // Third row.
    tableIndex = D_STATE_O * DFAInputNum + Digit;
    tempDFA->state[tableIndex] = D_STATE_NOT_ERROR_STATE;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_WRONG_INPUT;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_NOT_ERROR_STATE;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_NOT_ERROR_STATE;

    // Fourth row.
    tableIndex = D_STATE_F * DFAInputNum + Digit;
    tempDFA->state[tableIndex] = D_STATE_NOT_ERROR_STATE;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_NOT_ERROR_STATE;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_F;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_NOT_ERROR_STATE;

    // I'd actually want there to be string123, but will make it happen letter.
    tableIndex = D_STATE_ST * DFAInputNum + Digit;
    tempDFA->state[tableIndex] = D_STATE_WRONG_INPUT;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_NOT_ERROR_STATE;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_NOT_ERROR_STATE;
    tableIndex++;
    tempDFA->state[tableIndex] = D_STATE_ST;

    printf("Made simple arithematic DFA!\n");
    return tempDFA;
}

typedef enum{
    TOKEN_NUMBER,
    TOKEN_OPERATOR,
    TOKEN_KEYWORD,
    TOKEN_ID,
    TOKEN_NONE,
}TOKEN_TYPE;

typedef enum{
    OP_PLUS,
    OP_MINUS,
    OP_MUL,
    OP_DIV,
    OP_IF,
    OP_ELSE,
    OP_LC,
    OP_RC,
    OP_LP,
    OP_RP
}OPERATOR_TYPE;

typedef enum{
    KW_IF,
    KW_ELSE,
    KWS_NUM, // Number of keywords
}KEYWORD_TYPE;

char* TokenNames[] = {
    "TOKEN_NUMBER",
    "TOKEN_OPERATOR",
    "TOKEN_KEYWORD",
    "TOKEN_ID",
    "TOKEN_NONE"
};

char* OperatorNames[] = {
    "PLUS",
    "MINUS",
    "MUL",
    "DIV",
    "OP_IF",
    "OP_ELSE",
    "OP_LC",
    "OP_RC",
    "OP_LP",
    "OP_RP"
};

char* KeyWordNames[] = {
    "if",
    "else"
};

typedef struct{
    TOKEN_TYPE type;
    union {
        SimpleString stringValue;
        i64 numValue;
        OPERATOR_TYPE operator;
        KEYWORD_TYPE keyword;
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
    //printf("Gathered digits at pos with currentState: %s\n", DFAStateNames[dfa->currState]);
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
    i16 matchedString = -1;
    for (u16 i = 0; i < KWS_NUM; i++){
        _Bool match = MatchStringDirect(&sourceString, KeyWordNames[i]);
        if(match)
            matchedString = i;
    }
    if (matchedString != -1){ // It's a keyword.
        tok.type = TOKEN_KEYWORD;
        tok.TokenValue.keyword = matchedString;
    } else {
        tok.type = TOKEN_ID;
        tok.TokenValue.stringValue = sourceString;
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
        default: {
            FoundOperator = FALSE;
            break;
        }
    }
    if(FoundOperator){
        //printf("Tokenizing operator at pos:%d with currentState: %s, and current operator: %c\n", tokenizer->pos, DFAStateNames[dfa->currState], currChar);
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
        case D_STATE_ST: {
            tokenizer->tok->tokens[tokenizer->tok->numTokens].type = info.type;
            tokenizer->tok->tokens[tokenizer->tok->numTokens].TokenValue = info.TokenValue;
            tokenizer->tok->numTokens++;
            break;
        }
        case D_STATE_WRONG_INPUT: {
            printf("Wrong token at: %d\n", tokenizer->pos);
            printf("Token type is: %s\n", TokenNames[info.type]);
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
    printf("    \"%s\"\n", tokenizer->inputStream);
    while (currChar != '\0'){
        TokenInfo info;
        SkipSpaces(dfa, tokenizer);
        if ((info = TokenizeNumber(dfa, tokenizer)).type == TOKEN_NONE)
            if ((info = TokenizeOp(dfa, tokenizer)).type == TOKEN_NONE)
                info = TokenizeString(dfa, tokenizer);
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
        } else if(type == TOKEN_OPERATOR)  {
            printf("Token %d : %s : %s\n", i, TokenNames[tokens->tokens[i].type], OperatorNames[tokens->tokens[i].TokenValue.operator]);
        } else if(type == TOKEN_KEYWORD) {
            printf("Token %d : %s : %s\n", i, TokenNames[tokens->tokens[i].type], KeyWordNames[tokens->tokens[i].TokenValue.keyword]);
        } else if (type == TOKEN_ID) {
            printf("Token %d : %s : ", i, TokenNames[tokens->tokens[i].type]);
            PrintSimpleString(&tokens->tokens[i].TokenValue.stringValue);
            printf("\n");
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
        //printf("OPERATOR TOKEN AT: %d\n", parser->tokenPos);
        parser->tokenPos++;
        return currNode;
    }
    //printf("EXPECTED OP AT: %d\n",parser->tokenPos);
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
        //printf("NUMBER TOKEN AT: %d\n", parser->tokenPos);
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
    printf("EXPECTED OP AT: %d\n", prevTokPos);
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
    ARENA_ERROR err = makeArena(&LexerArena, KiB(1));
    if(err != ARENA_OK){
        printf("Can't make Lexer Arena");
        return 0;
    }
    DFA* arithDFA = CreateDFA(&LexerArena);
    const char* sampleProgram = "5 ( 3 - 1 * 32 + 12";
    Tokenizer* mainTokenizer = CreateTokenizer(&LexerArena, sampleProgram);
    Tokens* tokenizedProgram = StartTokenizing(&LexerArena, arithDFA, mainTokenizer);
    printTokens(tokenizedProgram);
    // Making 2 different arenas becauses the info from the Lexer's tokenized table will be copied and transformed in the parser.
    Arena ParserArena;
    err = makeArena(&ParserArena, KiB(5));
    if(err != ARENA_OK){
        printf("Can't make Parser Arena");
        return 0;
    }
    Parser* mainParser = CreateParser(&ParserArena, tokenizedProgram);
    StartParsing(&ParserArena, mainParser);
    removeArena(&LexerArena);
    WalkAST(mainParser); // Prefix notation.
    removeArena(&ParserArena);
    return 0;
}
