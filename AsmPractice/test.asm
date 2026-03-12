; Learning simple assembly: MOV, SYSCALLs, Arithematic and basic JMP + Conditions to emit assembly.
section .data
    msg db "Hello", 10    ; 10 is ASCII for '\n'.

section .text
global _start
_start:
    mov rax, 1            ; SYSCALL num for write.
    mov rdi, 1            ; stdout.
    mov rsi, msg          ; Pointer to string.
    mov rdx, 6            ; Length of the string. If I were to print the ASCII version of each character, I would have to create a 1 byte buffer and print it each one by one.
    syscall

    mov rax, 60           ; SYSCALL num for exit.
    xor rdi, rdi          ; exit code 0.
    syscall
