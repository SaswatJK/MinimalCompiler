#ifndef UTILS_H
#define UTILS_H

#include "arena.h"
#include "stdlib.h"
#include <inttypes.h>

#define DEBUG 0

typedef struct{
    const char* start;
    u16 length;
}SimpleString;

typedef enum {
    FALSE,
    TRUE
}BoolValue;

static inline _Bool MatchStringDirect(SimpleString* source, const char* compareTo){ // Direct because it's taking a const char pointer as a comparison to.
    if (source->length != strlen(compareTo)){
        return FALSE; // Maybe better types?
    }
    int result = memcmp(source->start, compareTo, source->length);
    if(result == 0){
        return TRUE;
    }
    return FALSE;
}

static inline _Bool MatchStringIndirect(SimpleString* source, SimpleString* compareTo){
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

static const char* TokenNames[] = {
    "TOKEN_NUMBER",
    "TOKEN_OPERATOR",
    "TOKEN_KEYWORD",
    "TOKEN_ID",
    "TOKEN_PRIM_TYPE",
    "TOKEN_NONE"
};

static const char* OperatorNames[] = {
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

static const char* KeyWordNames[] = {
    "if",
    "else",
    "print",
};

static const char* PrimitiveTypeNames[] = {
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

static const char* ASTNodeNames[] = {
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

static const char* StackErrorNames[] = {
    "STACK_OVERFLOW",
    "STACK_UNDERFLOW",
    "STACK_SYMBOL_NOT_FOUND",
    "STACK_SYMBOL_FOUND",
    "STACK_OK",
};

static inline SymbolStack* CreateSymbolStack(Arena* arena){
    SymbolStack* temp;
    PUSH_EMPTY_ARRAY_IN_ARENA(arena, SymbolStack, 1, temp);
    temp->topStackPointer = 0;
    return temp;
}

static inline i16 GetStackTop(SymbolStack* stack){
    return stack->topStackPointer;
}

static inline STACK_ERR PushSymbolToStack(SymbolStack* stack, Symbol symbol){
    if((stack->topStackPointer + 1) >= MAX_SYMBOL_STACK_SIZE){
        return STACK_OVERFLOW;
    }
    stack->data[stack->topStackPointer] = symbol;
    stack->topStackPointer++;
    return STACK_OK;
}

static inline STACK_ERR PopSymbolFromStack(SymbolStack* stack){
    if(stack->topStackPointer < 0){
        return STACK_UNDERFLOW;
    }
    stack->topStackPointer--;
    return STACK_OK;
}

static inline STACK_ERR FindSymbolInStack(SymbolStack* stack, SimpleString* ID, i16* stackPos){ // Starting trying to find the symbol from the top, so newer scope will have it in.
    _Bool found = FALSE;
    i16   currentSymbol = stack->topStackPointer - 1;
    while(found != TRUE && currentSymbol >= 0){
        found = MatchStringIndirect(stack->data[currentSymbol].name, ID);
        currentSymbol--;
    }
    if(!found)
        return STACK_SYMBOL_NOT_FOUND;
    *stackPos = currentSymbol + 1;
    return STACK_SYMBOL_FOUND;
}

static inline STACK_ERR SetTopPointer(SymbolStack* stack, u16 newTop){ // This will be used when going out of scope.
    if(newTop >= MAX_SYMBOL_STACK_SIZE)
        return STACK_OVERFLOW;
    if(newTop < 0)
        return STACK_UNDERFLOW;
    stack->topStackPointer = newTop;
    return STACK_OK;
}

static inline i64 GetIDOffsetInStack(SymbolStack* stack, SimpleString* ID){
    i16 symbolPosInStack;
    STACK_ERR err = FindSymbolInStack(stack, ID, &symbolPosInStack);
    if(err != STACK_SYMBOL_FOUND){
        fprintf(stderr, "Runtime error: %s with symbol: ", StackErrorNames[err]);
        PrintSimpleString(ID);
        fprintf(stderr, "\n");
        abort();
    }
    return stack->data[symbolPosInStack].offsetInCPUStack;
}

typedef struct{
    FILE* outputFile;
    SymbolStack* runTimeStack;
    i16 stackTop; // Top of stack in bytes.
    i16 ifCount;
}Emitter;

// was thinking of using the normal operatornames array but idk.
static const char* opNASMIns[] = {
    "add",
    "sub",
    "mul",
    "div"
};

#endif
