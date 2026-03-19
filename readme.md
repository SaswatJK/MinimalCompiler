# Minimal Compiler.

A simple compiler I built from scratch using Compiler theory.
Each stage of a theoretical compiler can be traced through the program, except for Optimization because most of the optimizaiton techniques require creating SSA form, which I thought was not needed for such a simple language.
Used arenas for simple memory management.

## Usage
```bash
./a.out build inputfile outputname
./outputname
```

## Stages
**Lexer** : NFAs are built for each token type, merged into one combined NFA, then converted to a DFA, whose transition input and states are stored in a lookup table. Handles keywords, identifiers, integers, and operators. It was suprisingly easy, as the tokens are quite simple.

**Grammar** : C-like grammar with `{` `};` for start and end of scopes respectively. `: type` after variable to declare it's type. I like that we declare a variable and then say it's of type. Also this would lend to scope resolutions (which I find really cool) for structs and namespaces in the future (if I decide to come back to it).

**Parser** : Simple Recursive descent (No look-ahead).

**Semantic Analysis** : Walk the AST with a symbol stack. Track variable declarations per scope catching use before and redeclaration errors.

**Code Generation** : Go through the AST again and emit NASM x86-64 assembly. Expression results are passed through `rax`, with intermediate values pushed and popped on the CPU stack. Variables live at fixed `rbp - offset` addresses. Print function is implementd in the nasmlib directory in x86-64 asm.

## Example
```
{
    x : int = 2 + 3 * 4;
    if x > 10 {
        print(x);
    }
}
```
Prints `14`.
