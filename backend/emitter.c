#include "emitter.h"

// I FINALLY UNDERSTAND WHY SSAs.

Emitter* CreateEmitter(Arena* arena){
    #if(DEBUG)
        fprintf(stderr, "Created Emitter.\n");
    #endif
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
    temp->stackTop = 8;
    temp->ifCount = 0;
    fprintf(temp->outputFile, "extern print_u64\n");
    fprintf(temp->outputFile, "section .text\n");
    fprintf(temp->outputFile, "global _start\n");
    fprintf(temp->outputFile, "_start:\n");
    fprintf(temp->outputFile, "mov rbp, rsp\n");
    return temp;
}

void EmitLoadSymbol(Emitter* emitter, const char* reg, ASTNode* node){
    i16 offset = GetIDOffsetInStack(emitter->runTimeStack, &node->Value.ID.Name);
    fprintf(emitter->outputFile, "mov %s, [rbp-%d]\n", reg, offset);
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
    fprintf(emitter->outputFile, "mov rbx, rax\n"); // Basically, if I say emit ID to RBX, then either the right node will be at rbx or it'll be a binary operation with results stored in rax.
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
    fprintf(emitter->outputFile, "sub rsp, 8\n");
    fprintf(emitter->outputFile, "mov [rbp-%d], rax\n",emitter->stackTop);
    Symbol ID;
    ID.name = &node->Value.AssignmentOperation.IDNode->Value.ID.Name;
    ID.type = node->Value.AssignmentOperation.IDNode->Value.ID.type;
    ID.offsetInCPUStack = emitter->stackTop;
    emitter->stackTop += 8; // cause 8 bytes each.
    PushSymbolToStack(emitter->runTimeStack, ID);
}

void EmitPrint(Emitter* emitter, i16 scope, ASTNode* node){
    ASTNode* valueNode =  node->Value.PrintExp.ValueNode;
    ASTNodeType ValueNodeType = valueNode->nodeType;
    switch (ValueNodeType) {
        case NODE_LEAF_ID:
            EmitLoadSymbol(emitter, "rax", valueNode);
            break;
        case NODE_LEAF_NUM :
            EmitLoadNum(emitter, "rax", valueNode);
            break;
        case NODE_INVALID  :
        case NODE_EMPTY    : {
            return;
        }
        case NODE_DIV   :
        case NODE_PLUS  :
        case NODE_MINUS :
        case NODE_MUL   : {
            EmitBinaryOperation(emitter, scope, valueNode);
            break;
        }
    }
    fprintf(emitter->outputFile, "call print_u64\n");
}

i16 EmitCompExp(Emitter* emitter, i16 scope, ASTNode* node){
    ASTNode* idNode = node->Value.CompareExp.IDNode;
    ASTNodeType idNodeType = node->Value.CompareExp.IDNode->nodeType;
    ASTNode* compOpNode = node->Value.CompareExp.compareOpNode;
    ASTNodeType compOpNodeType = node->Value.CompareExp.compareOpNode->nodeType;
    ASTNode* expNode = node->Value.CompareExp.ExpNode;
    ASTNodeType expNodeType = node->Value.CompareExp.ExpNode->nodeType;
    // We will load the ID value last.
    switch(expNodeType){
        case NODE_LEAF_ID:{
            EmitLoadSymbol(emitter, "rax", expNode);
            break;
        }
        case NODE_LEAF_NUM: {
            EmitLoadNum(emitter, "rax", expNode);
            break;
        }
        default: {
            EmitBinaryOperation(emitter, scope, expNode);
            break;
        }
    }
    fprintf(emitter->outputFile, "mov rbx, rax\n");
    EmitLoadSymbol(emitter, "rax", idNode);
    fprintf(emitter->outputFile, "cmp rax, rbx\n");
    switch (compOpNodeType) {
        case NODE_LT: {
            fprintf(emitter->outputFile, "jl .ifbody%d\n", emitter->ifCount);
            break;
        }
        case NODE_GT: {
            fprintf(emitter->outputFile, "jg .ifbody%d\n", emitter->ifCount);
            break;
        }
    }
    fprintf(emitter->outputFile, "jmp .elsebody%d\n", emitter->ifCount);
    emitter->ifCount++;
    return emitter->ifCount-1;
}

// Even if else is still empty, we want to jump to it.
void EmitIfExp(Emitter* emitter, i16 scope, ASTNode* node){
    i16 ifCount = EmitCompExp(emitter, scope, node->Value.IfElseOperation.conditionNode);
    ASTNode* ifBodyNode = node->Value.IfElseOperation.bodyNode;
    ASTNode* elseBodyNode = node->Value.IfElseOperation.elseBodyNode;
    ASTNodeType elseBodyType = node->Value.IfElseOperation.elseBodyNode->nodeType;
    i16 newScope = emitter->runTimeStack->topStackPointer;
    i16 stackTop = emitter->stackTop;
    fprintf(emitter->outputFile, ".ifbody%d:\n", ifCount);
    EmitBlock(emitter, newScope, ifBodyNode);
    i16 allocatedBlocksInBlock = emitter->stackTop - stackTop;
    if(allocatedBlocksInBlock > 0)
        fprintf(emitter->outputFile, "add rsp, %d\n", allocatedBlocksInBlock);
    emitter->stackTop = stackTop;
    fprintf(emitter->outputFile, "jmp .outside%d\n", ifCount);
    fprintf(emitter->outputFile, ".elsebody%d:\n", ifCount);
    if(elseBodyType == NODE_BLOCK){
        EmitBlock(emitter, newScope, elseBodyNode);
        allocatedBlocksInBlock = emitter->stackTop - stackTop;
        if(allocatedBlocksInBlock > 0)
            fprintf(emitter->outputFile, "add rsp, %d\n", allocatedBlocksInBlock);
        emitter->stackTop = stackTop;
    }
    fprintf(emitter->outputFile, "jmp .outside%d\n", ifCount);
    fprintf(emitter->outputFile, ".outside%d:\n", ifCount);
}

void EmitBlock(Emitter* emitter, i16 scope, ASTNode* node){
    ASTNodeType CurrentBlockType = node->Value.BlockList.startOfCurrentBlockNode->nodeType;
    ASTNode* CurrentBlockNode = node->Value.BlockList.startOfCurrentBlockNode;
    switch (CurrentBlockType) {
        case NODE_INVALID  :
        case NODE_EMPTY    : {
            break;
        }
        case NODE_STMNT: {
            EmitStatement(emitter, scope, CurrentBlockNode);
            break;
        }
        default: {
            break;
        }
    }
    ASTNodeType NextBlockType = node->Value.BlockList.startOfNextBlockNode->nodeType;
    ASTNode* NextBlockNode = node->Value.BlockList.startOfNextBlockNode;
    switch (NextBlockType) {
        case NODE_INVALID  :
        case NODE_EMPTY    : {
            break;
        }
        case NODE_BLOCK: {
            EmitBlock(emitter, scope, NextBlockNode);
            break;
        }
        default: {
            break;
        }
    }
}

void EmitStatement(Emitter* emitter, i16 scope, ASTNode* node){
    ASTNodeType CurrentStmntType = node->Value.StmtList.currentStmntNode->nodeType;
    ASTNode* CurrentStmntNode = node->Value.StmtList.currentStmntNode;
    switch (CurrentStmntType) {
        case NODE_DIV   :
        case NODE_PLUS  :
        case NODE_MINUS :
        case NODE_MUL   : {
            EmitBinaryOperation(emitter, scope, CurrentStmntNode);
            break;
        }
        case NODE_ASSIGNMENT: {
            EmitAssignment(emitter, scope, CurrentStmntNode);
            break;
        }
        case NODE_IF_ELSE: {
            EmitIfExp(emitter, scope, CurrentStmntNode);
            break;
        }
        case NODE_PRINT: {
            EmitPrint(emitter, scope, CurrentStmntNode);
            break;
        }
    }
    ASTNodeType NextStmntType = node->Value.StmtList.nextStmntNode->nodeType;
    ASTNode* NextStmntNode = node->Value.StmtList.nextStmntNode;
    switch (NextStmntType) {
        case NODE_INVALID  :
        case NODE_EMPTY    : {
            break;
        }
        case NODE_STMNT: {
            EmitStatement(emitter, scope, NextStmntNode);
            break;
        }
        default: {
            break;
        }
    }
}

void StartEmitting(Emitter* emitter, ASTNode* node){
    if(node == NULL){
        #if(DEBUG)
            fprintf(stderr, "Empty program.\n");
        #endif
        return;
    }
    i16 currentScopePointer = GetStackTop(emitter->runTimeStack);
    EmitBlock(emitter, currentScopePointer, node);
    fprintf(emitter->outputFile, "mov rax, 60\n");
    fprintf(emitter->outputFile, "xor rdi, rdi\n");
    fprintf(emitter->outputFile, "syscall\n");
    fclose(emitter->outputFile);
    #if(DEBUG)
        fprintf(stderr, "Successfully emitted!\n");
    #endif
    return;
}

// Should take in the program save name as arg
void GenerateBinary(const char* output){
    system("nasm -f elf64 tempasmfile -o taf.o");
    system("nasm -f elf64 ../nasmlib/print64.asm -o print64.o");
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "ld taf.o print64.o -o %s", output);
    system(cmd);
    system("rm taf.o");
    system("rm print64.o");
    //system("rm tempasmfile");
    fprintf(stderr, "Output executable at: %s.\n", output);
}
