#include "utils/arena.h"
#include "frontend/tokenizer.h"
#include "frontend/rd.h"
#include "frontend/sa.h"
#include "backend/emitter.h"
#include <string.h>

char* ReadInputFile(Arena* arena, const char* fileName){
    FILE* fp = fopen(fileName, "rb");
    if(!fp){
        fprintf(stderr, "Can't open the file: %s", fileName);
        abort();
    }
    if(fseek(fp, 0, SEEK_END) != 0){
        fprintf(stderr, "Can't find file end: %s", fileName);
        fclose(fp);
        abort();
    }
    long size = ftell(fp);
    if (size < 0){
        fprintf(stderr, "Invalid file size: %s", fileName);
        fclose(fp);
        abort();
    }
    rewind(fp);
    char* buffer;
    ARENA_ERROR err = makeArena(arena, (size + 1));
    if(err != ARENA_OK){
        fprintf(stderr, "Could not make Input file Arena: %s", ArenaErrorNames[err]);
        abort();
    }
    err = PUSH_EMPTY_ARRAY_IN_ARENA(arena, char, (size + 1), buffer);
    if(err != ARENA_OK){
        fprintf(stderr, "Could not allocate in arena: %s", ArenaErrorNames[err]);
        printArenaInfo(arena);
        abort();
    }
    size_t read_size = fread(buffer, 1, size, fp);
    if (read_size != (size_t)size){
        fprintf(stderr, "Could not copy the whoel file: %s", fileName);
        removeArena(arena);
        fclose(fp);
        abort();
    }
    buffer[size] = '\0'; // For safety.
    fclose(fp);
    return buffer;
}

void ParseCMDArgs(int numArgs, char* args[]){
    if(numArgs <= 1){
        fprintf(stderr, "Please specify actions for the compiler, such as:\n");
        fprintf(stderr, "                                                 `build`!:\n");
        abort();
    }
    if(numArgs <= 2){
        if(!strcmp(args[1], "build")){
            fprintf(stderr, "Please specify the path of input file to be compiled!\n");
            abort();
        } else {
            fprintf(stderr, "Expected correct action for the compiler!\n");
            abort();
        }
    }
    if(numArgs <= 3){
        if(!strcmp(args[1], "build")){
            fprintf(stderr, "Please specify the path of input file to be compiled!\n");
        } else {
            fprintf(stderr, "Expected correct action for the compiler!\n");
        }
        fprintf(stderr, "Please specify the name for the output binary!\n");
        abort();
    }
    if(numArgs >= 5){
        fprintf(stderr, "Unexpected arguments!\n");
        abort();
    }
    if(numArgs == 4){
        if(!strcmp(args[1], "build"))
            return;
        else {
            fprintf(stderr, "Expected correct action for the compiler!\n");
            abort();
        }
    }
}

// I think I understood why SSAs are important, especially for a serious language where we may want to do optmizations, even naive ones we can think of ourselves, or more complex ones that have been documented and are shared knowledge.

// TODO : IDEAS -> Println and functions.
int main(int numArgs, char* args[]){
    ParseCMDArgs(numArgs, args);
    Arena inputFileArena;
    const char* sampleProgram = ReadInputFile(&inputFileArena, args[2]);
    Arena LexerArena;
    ARENA_ERROR err = makeArena(&LexerArena, KiB(2));
    if(err != ARENA_OK){
        fprintf(stderr, "Can't make Lexer Arena: %s", ArenaErrorNames[err]);
        return 0;
    }
    DFA* arithDFA = CreateDFA(&LexerArena);
    Tokenizer* mainTokenizer = CreateTokenizer(&LexerArena, sampleProgram);
    Tokens* tokenizedProgram = StartTokenizing(&LexerArena, arithDFA, mainTokenizer);
    #if(DEBUG)
        printTokens(tokenizedProgram);
    #endif
    // Making an arena for the parser becuase the info from the Lexer's tokenized table will be copied and transformed in the parser.
    Arena ParserArena;
    // We cannot remove this arena right now because the ID nodes reference it.
    //removeArena(&inputFileArena);
    err = makeArena(&ParserArena, KiB(10));
    if(err != ARENA_OK){
        fprintf(stderr, "Can't make Parser Arena: %s", ArenaErrorNames[err]);
        return 0;
    }
    Parser* mainParser = CreateParser(&ParserArena, tokenizedProgram);
    StartParsing(&ParserArena, mainParser);
    SymbolStack* mainStack = CreateSymbolStack(&ParserArena);
    _Bool Analysis = StartSemanticAnalysis(mainStack, mainParser->startNode);
    #if(DEBUG)
        if(Analysis){
            fprintf(stderr, "Analysis true!");
        }
    #endif
    Arena BackendArena;
    err = makeArena(&BackendArena, KiB(5));
    if(err != ARENA_OK){
        fprintf(stderr, "Can't make Backend Arena: %s", ArenaErrorNames[err]);
        return 0;
    }
    Emitter* simpleEmitter = CreateEmitter(&BackendArena);
    StartEmitting(simpleEmitter, mainParser->startNode);
    GenerateBinary(args[3]);
    removeArena(&LexerArena);
    removeArena(&ParserArena);
    removeArena(&BackendArena);
    return 0;
}
