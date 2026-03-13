; Learning simple assembly: MOV, SYSCALLs, Arithematic and basic JMP + Conditions to emit assembly.

; .text | .data | .bss => read only | read/write | read/write + zero'ed out.

section .bss
   digits resb 20 ; Reserve a buffer of 20 bytes, since an ASCII digit is just one byte, and 64 bit number is at most 20 bytes.

section .text

print_u64:
    mov rcx, 0
    mov rbx, 10
    mov r13, 0
    test rax, rax ; Check if rax = 0.
    jz .handle_zero
    jnz .loop

.handle_zero:
    push 48       ; Pushing the '0' ASCII value.
    inc rcx
    jmp .pop_loop

.loop:            ; .label means local scoped
    xor rdx, rdx  ; While dividing and multiplying, rdx:rax is treated as a 128 bit number, so we clear the rdx register. (Turns out, unlike the old 8086 and 8085 days, the are rarely any explicit pair registers like BC and HL). Even the rdx:rax combined is used only for mul and div.
    div rbx

    add rdx, 48   ; Unlike the 8085 and 8086 MPs, not every operations' result gets stored in the 'accumulator'.
    push rdx
    inc rcx
    test rax, rax
    jnz .loop
    jz .pop_loop

.pop_loop:
    test rcx, rcx
    jz .print
    pop rdx
    mov [digits + r13], dl ; Lowest 8 bits of rdx.
    inc r13
    dec rcx
    jmp .pop_loop

.print:
    mov rax, 1
    mov rdi, 1
    mov rsi, digits
    mov rdx, r13
    syscall
    ret

global _start

_start:
    mov rax, 12739173891
    call print_u64
    mov rax, 60
    xor rdi, rdi
    syscall
    ret
