#include <stdlib.h>
#include <stdio.h>
//#include <string.h>
#include "arena.h"
#include <ctype.h>

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

DFA* CreateDFA(Arena* arena){
    DFA* tempDFA;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, DFA, tempDFA);
    if (err != ARENA_OK) {
        printf("Couldn't build DFA!");
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
    NUMBER,
    OPERATOR,
}TOKENS;

typedef struct{
    TOKENS* tokens;
    u16 tokenNum;
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

void TokenizeNumber(DFA* dfa, Tokenizer* tokenizer){
    char currChar = tokenizer->inputStream[tokenizer->pos];
    if (!isdigit(currChar)){
        return;
    }
    while (isdigit(currChar)){
        tokenizer->pos++;
        currChar = tokenizer->inputStream[tokenizer->pos];
    }
    printf("Gathered digits at pos with currentState: %s\n", DFAStateNames[dfa->currState]);
    int lookupIndex = dfa->currState * DFAInputNum + Digit;
    dfa->currState = dfa->state[lookupIndex];
    return;
}

void TokenizeOp(DFA* dfa, Tokenizer* tokenizer){
    char currChar = tokenizer->inputStream[tokenizer->pos];
    if (currChar == '+' || currChar == '-' || currChar == '/' || currChar == '*'){
        printf("Tokenizing operator at pos:%d with currentState: %s, and current operator: %c\n", tokenizer->pos, DFAStateNames[dfa->currState], currChar);
        int lookupIndex = dfa->currState * DFAInputNum + Operator;
        tokenizer->pos++;
        dfa->currState = dfa->state[lookupIndex];
    }
    return;
}

void AnalyzeState(DFA* dfa, Tokenizer* tokenizer){
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
            tokenizer->tok->tokens[tokenizer->tok->tokenNum] = OPERATOR;
            tokenizer->tok->tokenNum++;
            break;
        }
        case D_STATE_I: {
            tokenizer->tok->tokens[tokenizer->tok->tokenNum] = NUMBER;
            tokenizer->tok->tokenNum++;
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
        printf("Couldn't build Tokenizer!");
    }
    tokenizer->tok->tokenNum = 0;
    GET_POINTER_IN_ARENA(arena, TOKENS, tokenizer->tok->tokens);
    char currChar = tokenizer->inputStream[0];
    dfa->currState = D_STATE_S;
    printf("Starting tokenizing:\n");
    printf("    %s\n", tokenizer->inputStream);
    while (currChar != '\0'){
        SkipSpaces(dfa, tokenizer);
        TokenizeNumber(dfa, tokenizer);
        TokenizeOp(dfa, tokenizer);
        AnalyzeState(dfa, tokenizer);
        currChar = tokenizer->inputStream[tokenizer->pos];
    }
    printf("Tokenization successful with %d tokens\n", tokenizer->tok->tokenNum);
    PUSH_EMPTY_ARRAY_IN_ARENA(arena, TOKENS, tokenizer->tok->tokenNum, tokenizer->tok->tokens);
    // Rememebr to push the pointer.
    return tokenizer->tok;
}

char* TokenNames[] = {
    "NUMBER",
    "OPERATOR"
};

void printTokens(Tokens* tokens){
    for(uint i = 0; i < tokens->tokenNum; i++){
        printf("Token %d : %s\n", i, TokenNames[tokens->tokens[i]]);
    }
}

//TODO : Write a simple Parser.
int main(){
    Arena LexerArena;
    ARENA_ERROR err = makeArena(&LexerArena, KiB(10));
    if(err != ARENA_OK){
        printf("Can't make arena");
        return 0;
    }
    DFA* arithDFA = CreateDFA(&LexerArena);
    const char* sampleProgram = "5 + 3 - 1 * 123 + 32 ";
    Tokenizer* mainTokenizer = CreateTokenizer(&LexerArena, sampleProgram);
    Tokens* tokenizedProgram = StartTokenizing(&LexerArena, arithDFA, mainTokenizer);
    printTokens(tokenizedProgram);
    return 0;
}
