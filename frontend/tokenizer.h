#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "../utils/utils.h"

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

static const char* DFAStateNames[] = {
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

DFA* CreateDFA(Arena* arena);
Tokenizer* CreateTokenizer(Arena* arena, const char* inputStream);
Tokens* StartTokenizing(Arena* arena, DFA* dfa, Tokenizer* tokenizer);
void printTokens(Tokens* tokens);

#endif
