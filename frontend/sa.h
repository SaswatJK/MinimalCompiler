#ifndef SA_H
#define SA_H

#include "../utils/utils.h"

_Bool AnalyzeStmnt(SymbolStack* stack, i16 currentScopePointer, ASTNode* node);
void AnalyzeID(SymbolStack* stack, i16 currentScopePointer, ASTNode* node, _Bool toPush);
_Bool AnalyzeAssignment(SymbolStack* stack, i16 currentScopePointer, ASTNode* node);
_Bool AnalyzeBinaryOp(SymbolStack* stack, i16 currentScopePointer, ASTNode* node);
_Bool AnalyzeCompOp(SymbolStack* stack, i16 currentScopePointer, ASTNode* node);
_Bool AnalyzeIfElse(SymbolStack* stack, i16 currentScopePointer, ASTNode* node);
_Bool AnalyzePrint(SymbolStack* stack, i16 currentScopePointer, ASTNode* node);
_Bool AnalyzeBlock(SymbolStack* stack, i16 currentScopePointer, ASTNode* node);
_Bool StartSemanticAnalysis(SymbolStack* stack, ASTNode* node);

#endif
