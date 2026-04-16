#include "rd.h"

/*

    We will make a recursive descent parser for this.

    Complete Simple Grammar:

    S           -> Block | empty
    Block       -> { Stmnt } NuBlock
    NuBlock     -> Block | empty
    Stmnt       -> AssignExp NuStmnt | IfExp NuStmnt | PrintExp NuStmnt | empty NuStmnt
    NuStmnt     -> ; Stmnt | empty <- It will only look for another statement if there's already a semicolon.
    Exp         -> SubExp LowPresExp
    LowPresExp  -> BOPLP SubExp LowPresExp | empty
    SubExp      -> ID HighPresExp | Num HighPresExp | Factor HighPresExp <- I could make ID and Num to a 'term' function but I feel like maybe it's too many function calls for just one token check?
    HighPresExp -> BOPHP SubExp | empty
    Factor      -> ( Exp )
    AssignExp   -> ID : Type = Exp
    IfExp       -> 'if' ( CompareExp ) Block Elsepart
    PrintExp    -> 'print'(ID) | 'print'(Exp) <- Rudamentry print operation.
    CompareExp  -> ID CompareOp Exp
    Elsepart    -> 'else' Block | empty
    Type        -> 'int'
    BOPLP       -> + | -
    BOPHP       -> * | /
    CompareOp   -> < | >

*/

// All the parser functions to create the AST will have prefixes in uppercase of : E/N or nothing. If it's E -> It can return en empty node. If it's N, it can return a NULLPTR.

Parser* CreateParser(Arena* arena, Tokens* tok){
    Parser* tempParser;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, Parser, tempParser);
    if (err != ARENA_OK) {
        fprintf(stderr, "Couldn't build Parser: %s", ArenaErrorNames[err]);
        printArenaInfo(arena);
        abort();
    }
    tempParser->tok = tok;
    tempParser->tokenPos = 0;
    return tempParser;
}

_Bool CheckLParen(Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking LP at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        if(currTok.TokenValue.operator == OP_LP){
            parser->tokenPos++;
            return TRUE;
        }
    }
    return FALSE;
}

_Bool CheckRParen(Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking RP part at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        if(currTok.TokenValue.operator == OP_RP){
            parser->tokenPos++;
            return TRUE;
        }
    }
    return FALSE;
}

_Bool CheckLCB(Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking LCB part at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        if(currTok.TokenValue.operator == OP_LC){
            parser->tokenPos++;
            return TRUE;
        }
    }
    return FALSE;
}

_Bool CheckRCB(Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking RCB part at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        if(currTok.TokenValue.operator == OP_RC){
            parser->tokenPos++;
            return TRUE;
        }
    }
    return FALSE;
}

_Bool CheckSC(Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking SemiColon part at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        if(currTok.TokenValue.operator == OP_SC){
            parser->tokenPos++;
            return TRUE;
        }
    }
    return FALSE;
}

_Bool CheckColon(Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking Colon part at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        if(currTok.TokenValue.operator == OP_C){
            parser->tokenPos++;
            return TRUE;
        }
    }
    return FALSE;
}

ASTNode* NCheckLowPresBinaryOP(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking LowPres OP at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        OPERATOR_TYPE op = currTok.TokenValue.operator;
        if(op == OP_MINUS || op == OP_PLUS){
            ASTNode* currNode;
            ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
            if (err != ARENA_OK) {
                fprintf(stderr, "Couldn't build AST: BINARY LP NODE: %s", ArenaErrorNames[err]);
                printArenaInfo(arena);
                abort();
            }
            ASTNodeType type = (ASTNodeType)parser->tok->tokens[parser->tokenPos].TokenValue.operator;
            currNode->nodeType = type;
            parser->tokenPos++;
            return currNode;
        }
    }
    return NULL;
}

ASTNode* NCheckHighPresBinaryOP(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking HighPres OP at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        OPERATOR_TYPE op = currTok.TokenValue.operator;
        if(op == OP_DIV || op == OP_MUL){
            ASTNode* currNode;
            ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
            if (err != ARENA_OK) {
                fprintf(stderr, "Couldn't build AST: BINARY HP NODE: %s", ArenaErrorNames[err]);
                printArenaInfo(arena);
                abort();
            }
            ASTNodeType type = (ASTNodeType)parser->tok->tokens[parser->tokenPos].TokenValue.operator;
            currNode->nodeType = type;
            parser->tokenPos++;
            return currNode;
        }
    }
    return NULL;
}

ASTNode* CheckCompareOp(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking compare OP at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        OPERATOR_TYPE op = currTok.TokenValue.operator;
        if(op == OP_GT || op == OP_LT){
            ASTNode* currNode;
            ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
            if (err != ARENA_OK) {
                fprintf(stderr, "Couldn't build AST: COMPARE PARENT NODE: %s", ArenaErrorNames[err]);
                printArenaInfo(arena);
                abort();
            }
            ASTNodeType type = (ASTNodeType)parser->tok->tokens[parser->tokenPos].TokenValue.operator;
            currNode->nodeType = type;
            parser->tokenPos++;
            return currNode;
        }
    }
    fprintf(stderr, "Expected Comparison operator at: %d", parser->tokenPos);
    abort();
}

ASTNode* NCheckNum(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking NUM at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_NUMBER){
        ASTNode* currNode;
        ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
        if (err != ARENA_OK) {
            fprintf(stderr, "Couldn't build AST: LEAF NUM NODE: %s", ArenaErrorNames[err]);
            printArenaInfo(arena);
            abort();
        }
        //fprintf(stderr, "NUMBER TOKEN AT: %d\n", parser->tokenPos);
        currNode->Value.number = parser->tok->tokens[parser->tokenPos].TokenValue.numValue;
        currNode->nodeType = NODE_LEAF_NUM;
        parser->tokenPos++;
        return currNode;
    }
    return NULL;
}

ASTNode* NCheckFactor(Arena* arena, Parser* parser){
    #if(DEBUG)
    fprintf(stderr, "Checking Factor at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    u16 prevTokPos = parser->tokenPos;
    ASTNode* currNode;
    if(CheckLParen(parser)){
        currNode = NCheckExp(arena, parser);
        if(currNode == NULL){
            fprintf(stderr, "EXPECTED EXP at: %d", parser->tokenPos);
            abort();
        }
        if(CheckRParen(parser)){
            return currNode;
        } else {
            fprintf(stderr, "EXPECTED ')' at: %d", parser->tokenPos);
            abort();
        }
    }
    #if(DEBUG)
        fprintf(stderr, "Couldn't match factor.\n");
    #endif
    return NULL;
}

ASTNode* ECheckElsePart(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking Else part at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    ASTNode* currNode;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
    if (err != ARENA_OK) {
        fprintf(stderr, "Couldn't build AST: ELSE NODE: %s", ArenaErrorNames[err]);
        printArenaInfo(arena);
        abort();
    }
    if(currTok.type == TOKEN_KEYWORD){
        if(currTok.TokenValue.keyword == KW_ELSE){
            parser->tokenPos++;
            currNode = ECheckBlock(arena, parser);
            if(currNode->nodeType == NODE_EMPTY){
                fprintf(stderr, "Expected else body at: %d", parser->tokenPos);
                abort();
            }
        }
        return currNode;
    }
    currNode->nodeType = NODE_EMPTY;
    return currNode; // Empty part.
}

ASTNode* NCheckID(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking ID at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_ID){
        ASTNode* currNode;
        ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
        if (err != ARENA_OK) {
            fprintf(stderr, "Couldn't build AST: ID NODE: %s", ArenaErrorNames[err]);
            printArenaInfo(arena);
            abort();
        }
        currNode->nodeType = NODE_LEAF_ID;
        currNode->Value.ID.Name = currTok.TokenValue.stringValue;
        parser->tokenPos++;
        return currNode;
    }
    return NULL;
}

ASTNode* CheckCompareExp(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking comparison exp at: %d\n", parser->tokenPos);
    #endif
    ASTNode* IDNode = NCheckID(arena, parser);
    if(IDNode == NULL){
        fprintf(stderr, "Expected ID at: %d", parser->tokenPos);
        abort();
    }
    ASTNode* currNode;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
    if (err != ARENA_OK) {
        fprintf(stderr, "Couldn't build AST: ASSIGNMENT NODE: %s", ArenaErrorNames[err]);
        printArenaInfo(arena);
        abort();
    }
    currNode->nodeType = NODE_COMPARE_EXP;
    currNode->Value.CompareExp.IDNode = IDNode;
    ASTNode* CompOpNode = CheckCompareOp(arena, parser);
    currNode->Value.CompareExp.compareOpNode = CompOpNode;
    ASTNode* ExpNode = NCheckExp(arena, parser);
    if(ExpNode == NULL){
        fprintf(stderr, "Expected EXP at: %d\n", parser->tokenPos);
        abort();
    }
    currNode->Value.CompareExp.ExpNode= ExpNode;
    return currNode;
}

ASTNode* CheckEqual(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking EqualExp at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_OPERATOR){
        ASTNode* currNode;
        ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
        if (err != ARENA_OK) {
            fprintf(stderr, "Couldn't build AST: ASSIGNMENT NODE: %s", ArenaErrorNames[err]);
            printArenaInfo(arena);
            abort();
        }
        if(currTok.TokenValue.operator == OP_EQ){
            currNode->nodeType = NODE_ASSIGNMENT;
            parser->tokenPos++;
            return currNode;
        }
    }
    fprintf(stderr, "EXPECTED '=' at: %d", parser->tokenPos);
    abort();
}

PRIMITIVE_TYPE CheckType(Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking type at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    if(currTok.type == TOKEN_PRIM_TYPE){
        parser->tokenPos++;
        return currTok.TokenValue.primType;
    } else {
        fprintf(stderr, "Expected type at: %d", parser->tokenPos);
        abort();
    }
}

ASTNode* NCheckPrintExp(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking PrintExp at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    u16 prevPos = parser->tokenPos;
    if(currTok.type == TOKEN_KEYWORD){
        ASTNode* currNode;
        if (currTok.TokenValue.keyword == KW_PRINT) {
            ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
            if (err != ARENA_OK) {
                fprintf(stderr, "Couldn't build AST: KEYWORD NODE: %s", ArenaErrorNames[err]);
                printArenaInfo(arena);
                abort();
            }
        } else {
            return NULL;
        }
        currNode->nodeType = NODE_PRINT;
        parser->tokenPos++;
        _Bool isThere;
        isThere = CheckLParen(parser);
        if(!isThere){
            fprintf(stderr, "EXPECTED '(' at: %d", parser->tokenPos);
            abort();
        }
        ASTNode* printValueNode = NCheckExp(arena, parser);
        if(printValueNode == NULL){
            printValueNode = NCheckID(arena, parser);
            if(printValueNode == NULL){
                fprintf(stderr, "Expected print contents at: %d\n", parser->tokenPos);
                abort();
            }
        }
        isThere = CheckRParen(parser);
        if(!isThere){
            fprintf(stderr, "EXPECTED ')' at: %d", parser->tokenPos);
            abort();
        }
        currNode->Value.PrintExp.ValueNode = printValueNode;
        return currNode;
    }
    parser->tokenPos = prevPos;
    return NULL;
}

ASTNode* NCheckAssignExp(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking AssignExp at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    ASTNode* idNode = NCheckID(arena, parser);
    if(idNode == NULL)
        return NULL;
    if(CheckColon(parser)){
        PRIMITIVE_TYPE type = CheckType(parser);
        idNode->Value.ID.type = type;
        ASTNode* equalNode = CheckEqual(arena, parser);
        ASTNode* valNode = NCheckExp(arena, parser);
        if(valNode == NULL){
            fprintf(stderr, "Expected EXP at: %d\n", parser->tokenPos);
            abort();
        }
        equalNode->Value.AssignmentOperation.IDNode = idNode;
        equalNode->Value.AssignmentOperation.RValueNode = valNode;
        return equalNode;
    }
    fprintf(stderr, "Expected ':' at: %d", parser->tokenPos);
    abort();
}

ASTNode* NCheckIfExp(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking IfExp at: %d\n", parser->tokenPos);
    #endif
    TokenInfo currTok = parser->tok->tokens[parser->tokenPos];
    u16 prevPos = parser->tokenPos;
    if(currTok.type == TOKEN_KEYWORD){
        ASTNode* currNode;
        if (currTok.TokenValue.keyword == KW_IF) {
            ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
            if (err != ARENA_OK) {
                fprintf(stderr, "Couldn't build AST: KEYWORD NODE: %s", ArenaErrorNames[err]);
                printArenaInfo(arena);
                abort();
            }
        } else {
            return NULL;
        }
        currNode->nodeType = NODE_IF_ELSE;
        parser->tokenPos++;
        _Bool isThere;
        isThere = CheckLParen(parser);
        if(!isThere){
            fprintf(stderr, "EXPECTED '(' at: %d", parser->tokenPos);
            abort();
        }
        ASTNode* conditionNode = CheckCompareExp(arena, parser);
        isThere = CheckRParen(parser);
        if(!isThere){
            fprintf(stderr, "EXPECTED ')' at: %d", parser->tokenPos);
            abort();
        }
        ASTNode* bodyNode = ECheckBlock(arena, parser);
        ASTNode* elsePart = ECheckElsePart(arena, parser); // If it's NULL, then the other analysers will do something with it.
        currNode->Value.IfElseOperation.conditionNode = conditionNode;
        currNode->Value.IfElseOperation.bodyNode = bodyNode;
        currNode->Value.IfElseOperation.elseBodyNode = elsePart;
        return currNode;
    }
    parser->tokenPos = prevPos;
    return NULL;
}

ASTNode* NCheckHighPresExp(Arena* arena, Parser* parser){
    u16 prevPos = parser->tokenPos;
    #if(DEBUG)
        fprintf(stderr, "Checking CheckingHighPresExp at: %d\n", parser->tokenPos);
    #endif
    ASTNode* currNode;
    currNode = NCheckHighPresBinaryOP(arena, parser);
    if(currNode != NULL){
        ASTNode* subExpNode = NCheckSubExp(arena, parser);
        if(subExpNode == NULL){
            fprintf(stderr, "Expected SUBEXP at: %d\n", parser->tokenPos);
            abort();
        }
        currNode->Value.BinaryOperation.rightNode = subExpNode;
        return currNode;
    }
    return NULL;
}

ASTNode* NCheckLowPresExp(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking CheckingLowPresExp at: %d\n", parser->tokenPos);
    #endif
    u16 prevPos = parser->tokenPos;
    ASTNode* currNode;
    currNode = NCheckLowPresBinaryOP(arena, parser);
    if(currNode != NULL){
        ASTNode* subExpNode = NCheckSubExp(arena, parser);
        if(subExpNode == NULL){
            fprintf(stderr, "Expected SUBEXP at: %d\n", parser->tokenPos);
            abort();
        }
        ASTNode* lowPresExpNode = NCheckLowPresExp(arena, parser);
        if(lowPresExpNode == NULL){
            currNode->Value.BinaryOperation.rightNode = subExpNode;
            return currNode;
        } else {
            lowPresExpNode->Value.BinaryOperation.leftNode = subExpNode;
            currNode->Value.BinaryOperation.rightNode = lowPresExpNode;
            return currNode;
            }
        }
    return NULL;
}

ASTNode* NCheckSubExp(Arena* arena, Parser* parser){
    u16 prevPos = parser->tokenPos;
    #if(DEBUG)
        fprintf(stderr, "Checking Exp at: %d\n", prevPos);
    #endif
    ASTNode* currNode;
    currNode = NCheckFactor(arena, parser);
    if(currNode != NULL){
        ASTNode* highPresExpNode = NCheckHighPresExp(arena, parser);
        if(highPresExpNode == NULL)
            return currNode;
        highPresExpNode->Value.BinaryOperation.leftNode = currNode;
        return highPresExpNode;
    }
    parser->tokenPos = prevPos;
    currNode = NCheckNum(arena, parser);
    if(currNode != NULL){
        ASTNode* highPresExpNode = NCheckHighPresExp(arena, parser);
        if(highPresExpNode != NULL){
            highPresExpNode->Value.BinaryOperation.leftNode = currNode;
            return highPresExpNode;
        } else {
            return currNode;
        }
    }
    parser->tokenPos = prevPos;
    currNode = NCheckID(arena, parser);
    if(currNode != NULL){
        ASTNode* highPresExpNode = NCheckHighPresExp(arena, parser);
        if(highPresExpNode != NULL){
            highPresExpNode->Value.BinaryOperation.leftNode = currNode;
            return highPresExpNode;
        } else {
            return currNode;
        }
    }
    fprintf(stderr, "EXPECTED SUBEXP at: %d", prevPos);
    abort();
}

ASTNode* NCheckExp(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking Exp at: %d\n", parser->tokenPos);
    #endif
    ASTNode* SubExpNode = NCheckSubExp(arena, parser);
    if(SubExpNode == NULL)
        return NULL;
    ASTNode* LowPresExpNode = NCheckLowPresExp(arena, parser);
    if(LowPresExpNode == NULL){
        return SubExpNode;
    } else {
        LowPresExpNode->Value.BinaryOperation.leftNode = SubExpNode;
        return LowPresExpNode;
    }
}

ASTNode* NCheckNuStmnt(Arena* arena, Parser* parser, _Bool AfterStatement){
    u16 prevPos = parser->tokenPos;
    #if(DEBUG)
        fprintf(stderr, "Checking Nustmt at: %d\n", prevPos);
    #endif
    ASTNode* currNode;
    if(CheckSC(parser)){
        currNode = ECheckStmnt(arena, parser);
        return currNode;
    }
    if (AfterStatement) {
        fprintf(stderr, "EXPECTED ';' AT: %d", parser->tokenPos);
        abort();
    }
    else {
        return NULL;
    }
}

ASTNode* ECheckStmnt(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking statement at: %d\n", parser->tokenPos);
    #endif
    u16 prevPos = parser->tokenPos;
    ASTNode* currNode;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
    if(err != ARENA_OK){
        fprintf(stderr, "Can't make Stmnt Arena: %s", ArenaErrorNames[err]);
        printArenaInfo(arena);
        abort();
    }
    currNode->nodeType = NODE_STMNT;
    ASTNode* currStmntNode;
    _Bool statementParsed = TRUE;
    currStmntNode = NCheckAssignExp(arena, parser);
    if(currStmntNode == NULL){
        parser->tokenPos = prevPos;
        currStmntNode = NCheckIfExp(arena, parser);
    }
    if(currStmntNode == NULL){
        parser->tokenPos = prevPos;
        currStmntNode = NCheckPrintExp(arena, parser);
    }
    if(currStmntNode == NULL){
        statementParsed = FALSE;
        //POP_DATA_IN_ARENA(arena, ASTNode, 1);
    }
    ASTNode* nextStmntNode = NCheckNuStmnt(arena, parser, statementParsed);
    if(nextStmntNode == NULL){
        if(!statementParsed){
            currNode->nodeType = NODE_EMPTY;
            return currNode;
        }
    }
    currNode->Value.StmtList.currentStmntNode = currStmntNode;
    currNode->Value.StmtList.nextStmntNode = nextStmntNode;
    return currNode;
}

ASTNode* ECheckNuBlock(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking nublock at: %d\n", parser->tokenPos);
    #endif
    ASTNode* currNode;
    return ECheckBlock(arena, parser);
}

ASTNode* ECheckBlock(Arena* arena, Parser* parser){
    #if(DEBUG)
        fprintf(stderr, "Checking block at: %d\n", parser->tokenPos);
    #endif
    u16 prevPos = parser->tokenPos;
    ASTNode* currNode;
    ARENA_ERROR err = PUSH_EMPTY_OBJECT_IN_ARENA(arena, ASTNode, currNode);
    if(err != ARENA_OK){
        fprintf(stderr, "Can't make Block Arena: %s", ArenaErrorNames[err]);
        printArenaInfo(arena);
        abort();
    }
    currNode->nodeType = NODE_BLOCK;
    _Bool BlockParsed = TRUE;
    if(CheckLCB(parser)){
        ASTNode* FirstStatementNode = ECheckStmnt(arena, parser);
        if(!CheckRCB(parser)){
            fprintf(stderr, "EXPECTED '}' at: %d", parser->tokenPos);
            abort();
        }
        currNode->Value.BlockList.startOfCurrentBlockNode = FirstStatementNode;
        currNode->Value.BlockList.startOfNextBlockNode = ECheckNuBlock(arena, parser);
    } else {
        currNode->nodeType = NODE_EMPTY;
    }
    return currNode;
}

void StartParsing(Arena* arena, Parser* parser){
    parser->startNode = ECheckBlock(arena, parser);
    #if(DEBUG)
        if(parser->startNode != NULL){
            fprintf(stderr, "PARSED CORRECTLY\n");
        } else fprintf(stderr, "Empty program");
    #endif
}
