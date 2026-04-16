#ifndef RD_H
#define RD_H

#include "../utils/utils.h"

static u16 numNodes = 0;

typedef struct {
    Tokens* tok;
    ASTNode* startNode;
    u16 tokenPos;
}Parser;

// All the parser functions to create the AST will have prefixes in uppercase of : E/N or nothing. If it's E -> It can return en empty node. If it's N, it can return a NULLPTR.

Parser* CreateParser(Arena* arena, Tokens* tok);
_Bool CheckLParen(Parser* parser);
_Bool CheckRParen(Parser* parser);
_Bool CheckLCB(Parser* parser);
_Bool CheckRCB(Parser* parser);
_Bool CheckSC(Parser* parser);
_Bool CheckColon(Parser* parser);
ASTNode* NCheckLowPresBinaryOP(Arena* arena, Parser* parser);
ASTNode* NCheckHighPresBinaryOP(Arena* arena, Parser* parser);
ASTNode* CheckCompareOp(Arena* arena, Parser* parser);
ASTNode* NCheckNum(Arena* arena, Parser* parser);
ASTNode* NCheckHighPresExp(Arena* arena, Parser* parser);
ASTNode* NCheckFactor(Arena* arena, Parser* parser);
ASTNode* ECheckElsePart(Arena* arena, Parser* parser);
ASTNode* NCheckID(Arena* arena, Parser* parser);
ASTNode* CheckCompareExp(Arena* arena, Parser* parser);
ASTNode* CheckEqual(Arena* arena, Parser* parser);
PRIMITIVE_TYPE CheckType(Parser* parser);
ASTNode* NCheckPrintExp(Arena* arena, Parser* parser);
ASTNode* NCheckAssignExp(Arena* arena, Parser* parser);
ASTNode* NCheckIfExp(Arena* arena, Parser* parser);
ASTNode* NCheckLowPresExp(Arena* arena, Parser* parser);
ASTNode* NCheckSubExp(Arena* arena, Parser* parser);
ASTNode* NCheckExp(Arena* arena, Parser* parser);
ASTNode* NCheckNuStmnt(Arena* arena, Parser* parser, _Bool AfterStatement);
ASTNode* ECheckStmnt(Arena* arena, Parser* parser);
ASTNode* ECheckNuBlock(Arena* arena, Parser* parser);
ASTNode* ECheckBlock(Arena* arena, Parser* parser);
void StartParsing(Arena* arena, Parser* parser);

#endif
