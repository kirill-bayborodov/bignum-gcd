; bignum_gcd.asm — standalone x86-64 implementation of Stein's binary GCD.
; ABI: rdi=result, rsi=a, rdx=b; eax=bignum_gcd_status_t.
; Private records are used until successful transactional publication.
BITS 64
DEFAULT REL

%define CAP 32
%define LEN 256
%define REC 264
%define LEFT 0
%define RIGHT 264
%define DIFF 528
%define STATE 800
%define FRAME 840
%define SUCCESS 0
%define ERR_NULL -1
%define ERR_LENGTH -2
%define ERR_OVERLAP -3
%define ERR_CAPACITY -4

global bignum_gcd
section .text

; rdi=record: normalize logical length and clear the unused tail.
asm_norm:
    mov rcx, [rdi+LEN]
    cmp rcx, CAP
    jbe .scan
    mov rcx, CAP
.scan:
    test rcx, rcx
    jz .write_len
    mov rax, [rdi+rcx*8-8]
    test rax, rax
    jnz .write_len
    dec rcx
    jmp .scan
.write_len:
    mov [rdi+LEN], rcx
    mov rdx, CAP
    sub rdx, rcx
    lea r8, [rdi+rcx*8]
    xor eax, eax
    test rdx, rdx
    jz .done
.tail:
    mov [r8], rax
    add r8, 8
    dec rdx
    jnz .tail
.done:
    ret

; rdi=dst, rsi=src: copy all words and the length field.
asm_copy:
    cld
    mov ecx, 33
    rep movsq
    ret

; rdi=a, rsi=b; return eax=-1, 0 or 1.
asm_cmp:
    mov rax, [rdi+LEN]
    mov rcx, [rsi+LEN]
    cmp rax, rcx
    ja .greater
    jb .less
    test rax, rax
    jz .equal
    mov r8, rax
.compare_loop:
    dec r8
    mov rax, [rdi+r8*8]
    mov rcx, [rsi+r8*8]
    cmp rax, rcx
    ja .greater
    jb .less
    test r8, r8
    jnz .compare_loop
.equal:
    xor eax, eax
    ret
.greater:
    mov eax, 1
    ret
.less:
    mov eax, -1
    ret

; rdi=record: logical right shift by one bit.
asm_shr1:
    mov rcx, [rdi+LEN]
    test rcx, rcx
    jz .done
    xor r8d, r8d
.shift_loop:
    dec rcx
    mov rax, [rdi+rcx*8]
    mov r9, rax
    shr rax, 1
    shl r8, 63
    or rax, r8
    mov [rdi+rcx*8], rax
    and r9d, 1
    mov r8, r9
    test rcx, rcx
    jnz .shift_loop
.done:
    jmp asm_norm

; rdi=dst, rsi=a, r15=b; computes a-b assuming a>=b.
asm_sub:
    mov r8, [rsi+LEN]
    mov r9, [r15+LEN]
    mov r10, r8
    cmp r10, r9
    cmovb r10, r9
    xor r11d, r11d
    xor ecx, ecx
.sub_loop:
    xor eax, eax
    cmp rcx, r8
    jae .a_word_zero
    mov rax, [rsi+rcx*8]
.a_word_zero:
    xor edx, edx
    cmp rcx, r9
    jae .words_ready
    mov rdx, [r15+rcx*8]
.words_ready:
    mov ebx, r11d
    sub rax, rdx
    setc r11b
    movzx r11d, r11b
    sub rax, rbx
    setc bl
    or r11b, bl
    movzx r11d, r11b
    mov [rdi+rcx*8], rax
    inc rcx
    cmp rcx, r10
    jb .sub_loop
    mov [rdi+LEN], r10
    jmp asm_norm

; rdi=record: double one private value. Return eax=0 or ERR_CAPACITY.
asm_shl1:
    mov rcx, [rdi+LEN]
    xor r8d, r8d
    xor r9d, r9d
    test rcx, rcx
    jz .success
.double_loop:
    mov rax, [rdi+r8*8]
    mov r10, rax
    shl rax, 1
    or rax, r9
    mov [rdi+r8*8], rax
    shr r10, 63
    mov r9, r10
    inc r8
    cmp r8, rcx
    jb .double_loop
    test r9, r9
    jz .success
    cmp rcx, CAP
    jae .capacity
    mov [rdi+rcx*8], r9
    inc rcx
    mov [rdi+LEN], rcx
.success:
    jmp asm_norm
.capacity:
    mov eax, ERR_CAPACITY
    ret

bignum_gcd:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, FRAME
    mov r12, rdi
    mov r13, rsi
    mov r14, rdx

    test r12, r12
    jz .null
    test r13, r13
    jz .null
    test r14, r14
    jz .null
    mov rax, [r13+LEN]
    cmp rax, CAP
    ja .length
    mov rax, [r14+LEN]
    cmp rax, CAP
    ja .length

    ; Compare addresses by distance so every complete/partial result overlap is rejected.
    cmp r12, r13
    jb .result_before_a
    mov rax, r12
    sub rax, r13
    cmp rax, REC
    jb .overlap
.result_before_a:
    cmp r13, r12
    jb .a_done
    mov rax, r13
    sub rax, r12
    cmp rax, REC
    jb .overlap
.a_done:
    cmp r12, r14
    jb .result_before_b
    mov rax, r12
    sub rax, r14
    cmp rax, REC
    jb .overlap
.result_before_b:
    cmp r14, r12
    jb .overlap_done
    mov rax, r14
    sub rax, r12
    cmp rax, REC
    jb .overlap
.overlap_done:

    lea rdi, [rsp+LEFT]
    mov rsi, r13
    call asm_copy
    lea rdi, [rsp+RIGHT]
    mov rsi, r14
    call asm_copy
    lea rdi, [rsp+LEFT]
    call asm_norm
    lea rdi, [rsp+RIGHT]
    call asm_norm

    cmp qword [rsp+LEFT+LEN], 0
    jne .right_nonzero
    lea rdi, [r12]
    lea rsi, [rsp+RIGHT]
    jmp .publish
.right_nonzero:
    cmp qword [rsp+RIGHT+LEN], 0
    jne .common_factors
    lea rdi, [r12]
    lea rsi, [rsp+LEFT]
    jmp .publish

.common_factors:
    mov qword [rsp+STATE], 0
.common_loop:
    mov rax, [rsp+LEFT]
    test al, 1
    jnz .euclid_loop
    mov rax, [rsp+RIGHT]
    test al, 1
    jnz .euclid_loop
    lea rdi, [rsp+LEFT]
    call asm_shr1
    lea rdi, [rsp+RIGHT]
    call asm_shr1
    inc qword [rsp+STATE]
    jmp .common_loop

.euclid_loop:
    cmp qword [rsp+LEFT+LEN], 0
    je .choose_right
    cmp qword [rsp+RIGHT+LEN], 0
    je .choose_left
.strip_left:
    mov rax, [rsp+LEFT]
    test al, 1
    jnz .strip_right
    lea rdi, [rsp+LEFT]
    call asm_shr1
    jmp .strip_left
.strip_right:
    mov rax, [rsp+RIGHT]
    test al, 1
    jnz .compare
    lea rdi, [rsp+RIGHT]
    call asm_shr1
    jmp .strip_right
.compare:
    lea rdi, [rsp+LEFT]
    lea rsi, [rsp+RIGHT]
    call asm_cmp
    test eax, eax
    jz .choose_left
    jg .left_greater
    ; right = right - left
    lea rdi, [rsp+DIFF]
    lea rsi, [rsp+RIGHT]
    lea r15, [rsp+LEFT]
    call asm_sub
    lea rdi, [rsp+RIGHT]
    lea rsi, [rsp+DIFF]
    call asm_copy
    jmp .euclid_loop
.left_greater:
    ; left = left - right
    lea rdi, [rsp+DIFF]
    lea rsi, [rsp+LEFT]
    lea r15, [rsp+RIGHT]
    call asm_sub
    lea rdi, [rsp+LEFT]
    lea rsi, [rsp+DIFF]
    call asm_copy
    jmp .euclid_loop

.choose_right:
    lea rdi, [rsp+LEFT]
    lea rsi, [rsp+RIGHT]
    call asm_copy
.choose_left:
.restore_loop:
    cmp qword [rsp+STATE], 0
    je .publish_left
    lea rdi, [rsp+LEFT]
    call asm_shl1
    test eax, eax
    jnz .capacity
    dec qword [rsp+STATE]
    jmp .restore_loop
.publish_left:
    lea rdi, [r12]
    lea rsi, [rsp+LEFT]
.publish:
    call asm_copy
    mov eax, SUCCESS
    jmp .return
.null:
    mov eax, ERR_NULL
    jmp .return
.length:
    mov eax, ERR_LENGTH
    jmp .return
.overlap:
    mov eax, ERR_OVERLAP
    jmp .return
.capacity:
    mov eax, ERR_CAPACITY
.return:
    add rsp, FRAME
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
