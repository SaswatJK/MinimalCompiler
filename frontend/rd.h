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
void StartParsing(Arena* arena, Parser* parser);

#endif
