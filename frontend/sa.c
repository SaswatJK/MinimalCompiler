#include "sa.h"

// Check Type (Not useful right now as we only have one type).
// Check Symbol existence.
// Remember the stack state everytime we go inside a new block. // Also remember in which scope the analysis is taking place, such that redefinition doesn't cause errors.
// Anything less than the scope pointer is from a scope that is without-it. It can still access that variable, but it can also replace it.

void AnalyzeID(SymbolStack* stack, i16 currentScopePointer, ASTNode* node, _Bool toPush){
    Symbol ID;
    ID.name = &node->Value.ID.Name;
    ID.type = node->Value.ID.type;
    i16 symbolPos = -1;
    #if(DEBUG)
        fprintf(stderr, "Analyzing ID:\n");
        PrintSimpleString(ID.name);
        fprintf(stderr, "\n");
    #endif
    STACK_ERR err = FindSymbolInStack(stack, &node->Value.ID.Name, &symbolPos);
    if(toPush){
        if (err == STACK_SYMBOL_FOUND || err == STACK_SYMBOL_NOT_FOUND ) {
            if(currentScopePointer < symbolPos) { // The symbol was found within the scope.
                fprintf(stderr, "Redefinition of symbol: ");
                PrintSimpleString(ID.name);
                fprintf(stderr, "\n");
                abort();
            }
                err = PushSymbolToStack(stack, ID);
                #if(DEBUG)
                    fprintf(stderr, "^ Pushing ID.\n");
                #endif
                if(err == STACK_OVERFLOW){
                    fprintf(stderr, "Stack overflow.\n");
                    abort();
                } else
                    return;
        } else {
            fprintf(stderr, "Unkown error: %s", StackErrorNames[err]);
            abort();
        }
    } else {
        if(err == STACK_SYMBOL_NOT_FOUND){
            fprintf(stderr, "Undefined symbol: ");
            PrintSimpleString(ID.name);
            fprintf(stderr, "\n");
            abort();
        }
        if(err == STACK_SYMBOL_FOUND){
            return;
        } else {
            fprintf(stderr, "Unkown error: %s", StackErrorNames[err]);
            abort();
        }
    }
}

_Bool AnalyzeAssignment(SymbolStack* stack, i16 currentScopePointer, ASTNode* node){
    ASTNode* IDNode = node->Value.AssignmentOperation.IDNode;
    AnalyzeID(stack, currentScopePointer, IDNode, TRUE);
    ASTNodeType RValueNodeType = node->Value.AssignmentOperation.RValueNode->nodeType;
    #if(DEBUG)
        fprintf(stderr, "Analyzing Assignment.\n");
        fprintf(stderr, "The RValue node is: %s\n", ASTNodeNames[RValueNodeType]);
    #endif
    switch (RValueNodeType) {
        case NODE_INVALID  :
        case NODE_LEAF_NUM :
        case NODE_EMPTY    : {
            return TRUE;
        }
        case NODE_DIV   :
        case NODE_PLUS  :
        case NODE_MINUS :
        case NODE_MUL   : {
            return AnalyzeBinaryOp(stack, currentScopePointer, node->Value.AssignmentOperation.RValueNode);
        }
        case NODE_LEAF_ID : {
            AnalyzeID(stack, currentScopePointer, node->Value.AssignmentOperation.RValueNode, FALSE);
            break;
        }
    }
    return TRUE;
}

_Bool AnalyzeBinaryOp(SymbolStack* stack, i16 currentScopePointer, ASTNode* node){
    #if(DEBUG)
        fprintf(stderr, "Analyzing BinaryOp.\n");
    #endif
    ASTNodeType LeftNodeType = node->Value.BinaryOperation.leftNode->nodeType;
    _Bool LeftSuccess = TRUE;
    switch (LeftNodeType) {
        case NODE_LEAF_NUM :
            #if(DEBUG)
                fprintf(stderr, "Number is: %"PRId64"\n", node->Value.BinaryOperation.leftNode->Value.number);
            #endif
            break;
        case NODE_INVALID  :
        case NODE_EMPTY    : {
            return FALSE;
        }
        case NODE_DIV   :
        case NODE_PLUS  :
        case NODE_MINUS :
        case NODE_MUL   : {
            LeftSuccess = AnalyzeBinaryOp(stack, currentScopePointer, node->Value.BinaryOperation.leftNode);
            break;
        }
        case NODE_LEAF_ID : {
            AnalyzeID(stack, currentScopePointer, node->Value.BinaryOperation.leftNode, FALSE);
            break;
        }
    }
    ASTNodeType RightNodeType = node->Value.BinaryOperation.rightNode->nodeType;
    _Bool RightSuccess = TRUE;
    switch (RightNodeType) {
        case NODE_LEAF_NUM :
            #if(DEBUG)
                fprintf(stderr, "Number is: %"PRId64"\n", node->Value.BinaryOperation.rightNode->Value.number);
            #endif
            break;
        case NODE_INVALID  :
        case NODE_EMPTY    : {
            return FALSE;
        }
        case NODE_DIV   :
        case NODE_PLUS  :
        case NODE_MINUS :
        case NODE_MUL   : {
            RightSuccess = AnalyzeBinaryOp(stack, currentScopePointer, node->Value.BinaryOperation.rightNode);
            break;
        }
        case NODE_LEAF_ID : {
            AnalyzeID(stack, currentScopePointer, node->Value.BinaryOperation.rightNode, FALSE);
            break;
        }
    }
    return (LeftSuccess && RightSuccess);
}

_Bool AnalyzeCompOp(SymbolStack* stack, i16 currentScopePointer, ASTNode* node){
    #if(DEBUG)
        fprintf(stderr, "Analyzing CompOp.\n");
    #endif
    ASTNode* IDNode = node->Value.CompareExp.IDNode;
    AnalyzeID(stack, currentScopePointer, IDNode, FALSE);
    #if(DEBUG)
        fprintf(stderr, "Analyzed ID.\n");
    #endif
    ASTNodeType CompWithType = node->Value.CompareExp.ExpNode->nodeType;
    switch (CompWithType) {
        case NODE_LEAF_NUM :
            return TRUE;
        case NODE_INVALID  :
        case NODE_EMPTY    : {
        }
        case NODE_DIV   :
        case NODE_PLUS  :
        case NODE_MINUS :
        case NODE_MUL   : {
            return AnalyzeBinaryOp(stack, currentScopePointer, node->Value.CompareExp.ExpNode);
        }
        case NODE_LEAF_ID : {
            AnalyzeID(stack, currentScopePointer, node->Value.CompareExp.ExpNode, FALSE);
            break;
        }
    }
    return TRUE;
}

_Bool AnalyzeIfElse(SymbolStack* stack, i16 currentScopePointer, ASTNode* node){
    #if(DEBUG)
        fprintf(stderr, "Analyzing IfElse.\n");
    #endif
    i16 newScopePointer = GetStackTop(stack);
    _Bool Condition = AnalyzeCompOp(stack, currentScopePointer, node->Value.IfElseOperation.conditionNode);
    _Bool BodyBlock = AnalyzeBlock(stack, newScopePointer, node->Value.IfElseOperation.bodyNode);
    SetTopPointer(stack, newScopePointer);
    newScopePointer = currentScopePointer; // Ditching all the symbols in that scope.
    ASTNodeType elseBodyType = node->Value.IfElseOperation.elseBodyNode->nodeType;
    if(elseBodyType == NODE_EMPTY){
        return Condition && BodyBlock;
    } else {
        newScopePointer = GetStackTop(stack);
        _Bool ElseBlock = AnalyzeBlock(stack, newScopePointer, node->Value.IfElseOperation.elseBodyNode);
        SetTopPointer(stack, newScopePointer);
        newScopePointer = currentScopePointer; // Ditching all the symbols in that scope.
        return Condition && BodyBlock && ElseBlock;
    }
}

_Bool AnalyzePrint(SymbolStack* stack, i16 currentScopePointer, ASTNode* node){
    #if(DEBUG)
        fprintf(stderr, "Analyzing Print.\n");
    #endif
    ASTNode* valueNode =  node->Value.PrintExp.ValueNode;
    if(valueNode->nodeType == NODE_LEAF_ID){
        AnalyzeID(stack, currentScopePointer, valueNode, FALSE);
        return TRUE;
    }
    ASTNodeType ValueNodeType = valueNode->nodeType;
    #if(DEBUG)
        fprintf(stderr, "The RValue node is: %s\n", ASTNodeNames[ValueNodeType]);
    #endif
    switch (ValueNodeType) {
        case NODE_INVALID  :
        case NODE_LEAF_NUM :
        case NODE_EMPTY    : {
            return TRUE;
        }
        case NODE_DIV   :
        case NODE_PLUS  :
        case NODE_MINUS :
        case NODE_MUL   : {
            return AnalyzeBinaryOp(stack, currentScopePointer, node->Value.PrintExp.ValueNode);
        }
        case NODE_LEAF_ID : {
            AnalyzeID(stack, currentScopePointer, node->Value.PrintExp.ValueNode, FALSE);
            break;
        }
    }
    return TRUE;
}

_Bool AnalyzeStmnt(SymbolStack* stack, i16 currentScopePointer, ASTNode* node){
    #if(DEBUG)
        fprintf(stderr, "Analyzing Statement.\n");
    #endif
    ASTNodeType CurrentStmntType = node->Value.StmtList.currentStmntNode->nodeType;
    ASTNode* CurrentStmntNode = node->Value.StmtList.currentStmntNode;
    _Bool CurrentStatementAnalysis;
    switch (CurrentStmntType) {
        case NODE_INVALID  :
        case NODE_LEAF_NUM :
        case NODE_EMPTY    : {
            CurrentStatementAnalysis = TRUE;
            break;
        }
        case NODE_DIV   :
        case NODE_PLUS  :
        case NODE_MINUS :
        case NODE_MUL   : {
            CurrentStatementAnalysis = AnalyzeBinaryOp(stack, currentScopePointer, CurrentStmntNode);
            break;
        }
        case NODE_ASSIGNMENT: {
            CurrentStatementAnalysis = AnalyzeAssignment(stack, currentScopePointer, CurrentStmntNode);
            break;
        }
        case NODE_IF_ELSE: {
            CurrentStatementAnalysis = AnalyzeIfElse(stack, currentScopePointer, CurrentStmntNode);
            break;
        }
        case NODE_PRINT: {
            CurrentStatementAnalysis = AnalyzePrint(stack, currentScopePointer, CurrentStmntNode);
            break;
        }
    }
    ASTNodeType NextStmntType = node->Value.StmtList.nextStmntNode->nodeType;
    ASTNode* NextStmntNode = node->Value.StmtList.nextStmntNode;
    #if(DEBUG)
        fprintf(stderr, "The next statement node is: %s\n", ASTNodeNames[NextStmntType]);
    #endif
    // The next statement will always be another statement.
    _Bool NextStatementAnalysis;
    switch (NextStmntType) {
        case NODE_INVALID  :
        case NODE_EMPTY    : {
            NextStatementAnalysis = TRUE;
            break;
        }
        case NODE_STMNT: {
            NextStatementAnalysis = AnalyzeStmnt(stack, currentScopePointer, NextStmntNode);
            break;
        }
        default: {
            NextStatementAnalysis = FALSE;
        }
    }
    return CurrentStatementAnalysis && NextStatementAnalysis;
}

_Bool AnalyzeBlock(SymbolStack* stack, i16 currentScopePointer, ASTNode* node){
    ASTNodeType CurrentBlockType = node->Value.BlockList.startOfCurrentBlockNode->nodeType;
    #if(DEBUG)
        fprintf(stderr, "Analyzing Block.\n");
        fprintf(stderr, "The current block node is: %s\n", ASTNodeNames[CurrentBlockType]);
    #endif
    ASTNode* CurrentBlockNode = node->Value.BlockList.startOfCurrentBlockNode;
    _Bool CurrentBlockAnalysis;
    switch (CurrentBlockType) {
        case NODE_INVALID  :
        case NODE_EMPTY    : {
            CurrentBlockAnalysis = TRUE;
            break;
        }
        case NODE_STMNT: {
            CurrentBlockAnalysis = AnalyzeStmnt(stack, currentScopePointer, CurrentBlockNode);
            break;
        }
        default: {
            break;
        }
    }
    i16 newScopePointer = GetStackTop(stack);
    ASTNodeType NextBlockType = node->Value.BlockList.startOfNextBlockNode->nodeType;
    ASTNode* NextBlockNode = node->Value.BlockList.startOfNextBlockNode;
    _Bool NextBlockAnalysis;
    switch (NextBlockType) {
        case NODE_INVALID  :
        case NODE_EMPTY    : {
            NextBlockAnalysis = TRUE;
            break;
        }
        case NODE_BLOCK: {
            NextBlockAnalysis = AnalyzeBlock(stack, currentScopePointer, NextBlockNode);
            break;
        }
        default: {
            return NextBlockAnalysis = FALSE;
        }
    }
    return CurrentBlockAnalysis && NextBlockAnalysis;
}

_Bool StartSemanticAnalysis(SymbolStack* stack, ASTNode* node){
    #if(DEBUG)
        fprintf(stderr, "Started analysis.\n");
    #endif
    if(node == NULL){
        #if(DEBUG)
            fprintf(stderr, "Empty program.\n");
        #endif
        return FALSE;
    }
    i16 currentScopePointer = GetStackTop(stack);
    return AnalyzeBlock(stack, currentScopePointer, node);
}
