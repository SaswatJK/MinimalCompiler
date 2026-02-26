//#include <stdlib.h>
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
             v  |          space
         /-- I----------------------
  digit /                          |
       /                           |
     (S)----------->{O}--          |
      |   pperator      |space     |
space |    v------------'          |
      |-->{F} --|                  |
          /    ^| space            |
         /  |___|                  |
        '--------------------------|


  DFA TABLE:

St\In | Digit | Operator | space
   S  |   I   |     O    |  {F}
   I  |   I   | -------- |  {F}
   O  | ----- | -------- |  {F}
  {F} | ----- | -------- |  {F}

*/

typedef enum DFAStates{
    S,
    I,
    O,
    F,
    DFAStatesNum,
    WrongInput
}dState;

typedef enum DFAInputs{
    Digit,
    Operator,
    Space,
    DFAInputNum
}dInput;

typedef struct{
    dState state[DFAStatesNum * DFAInputNum];
}DFA;

DFA* CreateDFA(Arena* arena){
    DFA* tempDFA;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, DFA, tempDFA);
    if (err != ARENA_OK) {
        printf("Couldn't build DFA!");
    }
    int tableIndex;
    tableIndex = S * DFAInputNum + Digit;
    tempDFA->state[tableIndex] = I;
    tableIndex++;
    tempDFA->state[tableIndex] = O;
    tableIndex++;
    tempDFA->state[tableIndex] = F;
    tableIndex = I * DFAInputNum + Digit;
    tempDFA->state[tableIndex] = I;
    tableIndex++;
    tempDFA->state[tableIndex] = WrongInput;
    tableIndex++;
    tempDFA->state[tableIndex] = F;
    tableIndex = O * DFAInputNum + Digit;
    tempDFA->state[tableIndex] = WrongInput;
    tableIndex++;
    tempDFA->state[tableIndex] = WrongInput;
    tableIndex++;
    tempDFA->state[tableIndex] = F;
    tableIndex = F * DFAInputNum + Digit;
    tempDFA->state[tableIndex] = WrongInput;
    tableIndex++;
    tempDFA->state[tableIndex] = WrongInput;
    tableIndex++;
    tempDFA->state[tableIndex] = F;
    printf("Made simple arithematic DFA!\n");
    return tempDFA;
}

typedef enum{
    NUMBER,
    OPERATOR,
}TOKENS;

typedef struct{
    const char* inputStream;
    u32 pos;
}Tokenizer;

typedef struct{
    TOKENS* tokens;
    u16 tokenNum;
}Tokens;

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

Tokens* StartTokenizing(Arena* arena, DFA* dfa, Tokenizer* tokenizer){
    Tokens* tempTokens;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, Tokens, tempTokens);
    if (err != ARENA_OK) {
        printf("Couldn't build Tokenizer!");
    }
    tempTokens->tokenNum = 0;
    GET_POINTER_IN_ARENA(arena, TOKENS, tempTokens->tokens);
    char currChar = tokenizer->inputStream[0];
    dState currState = S;
    dState prevState;
    printf("Starting tokenizing:\n");
    printf("    %s\n", tokenizer->inputStream);
    while (currChar != '\0'){
        if (isdigit(currChar)){
            prevState = currState;
            int lookupIndex = currState * DFAInputNum + Digit;
            currState = dfa->state[lookupIndex];
            printf("prevState: %d | input: %s | currentState: %d | lookup index: %d\n", prevState, "Digit", currState, lookupIndex);
            if (currState == WrongInput){
                printf("Wrong state at: %d", tokenizer->pos);
                return tempTokens;
            }
        }
        else if (currChar == ' '){
            prevState = currState;
            int lookupIndex = currState * DFAInputNum + Space;
            currState = dfa->state[lookupIndex];
            printf("prevState: %d | input: %s | currentState: %d | lookup index: %d\n", prevState, "Space", currState, lookupIndex);
            if (currState == WrongInput){
                printf("Wrong state at: %d", tokenizer->pos);
                return tempTokens;
            }
        }
        else if (currChar == '+' || currChar == '-' || currChar == '/' || currChar == '*'){
            int lookupIndex = currState * DFAInputNum + Operator;
            printf("prevState: %d | input: %s | currentState: %d | lookup index: %d\n", prevState, "Opeartor", currState, lookupIndex);
            currState = dfa->state[lookupIndex];
            if (currState == WrongInput){
                printf("Wrong state at: %d", tokenizer->pos);
                return tempTokens;
            }
        }
        if (currState == F){
            switch(prevState){
                case S : {
                    break;
                }
                case I : {
                    tempTokens->tokens[tempTokens->tokenNum] = NUMBER;
                    tempTokens->tokenNum++;
                    break;
                }
                case O : {
                    tempTokens->tokens[tempTokens->tokenNum] = OPERATOR;
                    tempTokens->tokenNum++;
                    break;
                }
                case F : {
                    break;
                }
                case DFAStatesNum :
                case WrongInput :
                    printf("Wrong State of DFA!!");
                    break;
            }
        prevState = currState;
        currState = S;
        }
        tokenizer->pos++;
        currChar = tokenizer->inputStream[tokenizer->pos];
    }
    printf("Tokenization successful with %d tokens\n", tempTokens->tokenNum);
    PUSH_EMPTY_ARRAY_IN_ARENA(arena, TOKENS, tempTokens->tokenNum, tempTokens->tokens);
    // Rememebr to push the pointer.
    return tempTokens;
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
