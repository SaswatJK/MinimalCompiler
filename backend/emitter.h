#ifndef EMITTER_H
#define EMITTER_H

#include "../utils/utils.h"

Emitter* CreateEmitter(Arena* arena);
void StartEmitting(Emitter* emitter, ASTNode* node);
void GenerateBinary(const char* output);

#endif
