#ifndef EMITTER_H
#define EMITTER_H

#include "../utils/utils.h"

Emitter* CreateEmitter(Arena* arena);
void EmitBinaryOperation(Emitter* emitter, i16 scope, ASTNode* node);
i16 EmitCompExp(Emitter* emitter, i16 scope, ASTNode* node);
void EmitIfExp(Emitter* emitter, i16 scope, ASTNode* node);
void EmitAssignment(Emitter* emitter, i16 scope, ASTNode* node); // Push to stack. // Evaluate R-Value and then push it to the stack.
void EmitPrint(Emitter* emitter, i16 scope, ASTNode* node);
void EmitBlock(Emitter* emitter, i16 scope, ASTNode* node);
void EmitLoadSymbol(Emitter* emitter, const char* reg, ASTNode* node);
void EmitLoadNum(Emitter* emitter, const char* reg, ASTNode* node);
void EmitAssignment(Emitter* emitter, i16 scope, ASTNode* node);
void EmitPrint(Emitter* emitter, i16 scope, ASTNode* node);
i16 EmitCompExp(Emitter* emitter, i16 scope, ASTNode* node);
void EmitIfExp(Emitter* emitter, i16 scope, ASTNode* node);
void EmitBlock(Emitter* emitter, i16 scope, ASTNode* node);
void EmitStatement(Emitter* emitter, i16 scope, ASTNode* node);
void StartEmitting(Emitter* emitter, ASTNode* node);
void GenerateBinary(const char* output);

#endif
