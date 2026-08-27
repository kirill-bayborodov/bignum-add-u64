; -----------------------------------------------------------------------------
; @file    bignum_add_u64.asm
; @author  git@bayborodov.com
; @version 1.0.0
; @date    29.07.2026
;
; @brief   Optimized x86-64 implementation for adding a bignum and a 64-bit scalar.
;
; @details
;   Uses the System V AMD64 ABI. The result is published only after all
;   validation and representability checks succeed.
;   Optimizations:
;   - exact in-place fast path without suffix copying;
;   - branchless absolute-distance overlap check;
;   - carry-preserving LEA/DEC scheduling;
;   - SSE tail copy and lazy zeroing.
; -----------------------------------------------------------------------------

section .text
global bignum_add_u64

; --- Fixed representation constants. ---
BIGNUM_CAPACITY         equ 32
BIGNUM_OFFSET_LEN       equ 256
BUF_SIZE                equ 264

BIGNUM_ADD_U64_SUCCESS                 equ  0
BIGNUM_ADD_U64_ERROR_NULL_PTR          equ -1
BIGNUM_ADD_U64_ERROR_CAPACITY_EXCEEDED equ -2
BIGNUM_ADD_U64_ERROR_BUFFER_OVERLAP    equ -3
BIGNUM_ADD_U64_ERROR_OVERFLOW          equ -4

align 16
bignum_add_u64:
    ; Arguments: rdi = result, rsi = source a, rdx = scalar b.
    ; Return: named status in EAX. r12/r14/r15 are callee-saved and restored.

    push    rbp
    mov     rbp, rsp
    push    r12
    push    r14
    push    r15

    ; Validate pointers before dereferencing either object.
    test    rdi, rdi
    je      .err_null
    test    rsi, rsi
    je      .err_null

    ; Validate the source logical length.
    mov     r8, qword [rsi + BIGNUM_OFFSET_LEN]
    cmp     r8, BIGNUM_CAPACITY
    ja      .err_cap

    ; Reject partial overlap while allowing exact in-place aliasing.
    cmp     rdi, rsi
    je      .overlap_ok
    mov     rax, rdi
    sub     rax, rsi
    mov     rcx, rax
    sar     rcx, 63
    xor     rax, rcx
    sub     rax, rcx        ; rax = abs(result - a)
    cmp     rax, BUF_SIZE
    jb      .err_overlap
.overlap_ok:

    mov     r12, rdi        ; Preserve result for all success paths.

    ; Zero-source fast path.
    test    r8, r8
    jz      .a_is_zero

    ; Zero-scalar fast path.
    test    rdx, rdx
    jz      .b_is_zero

    ; Preflight full-capacity overflow before any destination write. This keeps
    ; the error path transactional while avoiding the scan for ordinary inputs.
    cmp     r8, BIGNUM_CAPACITY
    jne     .add_main
    test    rdx, rdx
    jz      .add_main
    mov     rax, [rsi]
    add     rax, rdx
    jnc     .add_main
    mov     rcx, 1
.add_overflow_scan:
    cmp     rcx, r8
    jae     .err_overflow
    cmp     qword [rsi + rcx*8], -1
    jne     .add_main
    inc     rcx
    jmp     .add_overflow_scan

.add_main:
    ; Add the scalar and propagate carry through active source words.
    mov     rax, [rsi]
    add     rax, rdx        ; Add the scalar to the low word and set CF.
    mov     [rdi], rax

    mov     rcx, r8
    dec     rcx             ; rcx = remaining words (a->len - 1).
    jz      .check_final_carry

    lea     r14, [rsi + 8]  ; Read pointer
    lea     r15, [rdi + 8]  ; Write pointer

    align 16
.add_loop:
    jnc     .fast_copy      ; Carry is clear; copy the untouched suffix.
    
    mov     rax, [r14]
    adc     rax, 0          ; Add the incoming carry to this word.
    mov     [r15], rax
    
    lea     r14, [r14 + 8]
    lea     r15, [r15 + 8]
    dec     rcx
    jnz     .add_loop
    jmp     .check_final_carry

.fast_copy:
    ; Carry is clear; skip arithmetic and handle the untouched suffix.
    cmp     r12, rsi
    je      .add_done       ; For in-place calls, the untouched suffix is already in place.

    ; Otherwise copy the remaining rcx words from r14 to r15.
    test    rcx, rcx
    jz      .add_done
    mov     rax, rcx
    shr     rax, 1          ; rax = number of 16-byte blocks
    jz      .fast_copy_odd

    align 16
.fast_copy_sse:
    movdqu  xmm0, [r14]
    movdqu  [r15], xmm0
    lea     r14, [r14 + 16]
    lea     r15, [r15 + 16]
    dec     rax
    jnz     .fast_copy_sse

.fast_copy_odd:
    test    rcx, 1
    jz      .add_done
    mov     rax, [r14]
    mov     [r15], rax
    jmp     .add_done

.check_final_carry:
    jnc     .add_done
    cmp     r8, BIGNUM_CAPACITY
    jae     .err_overflow       ; Overflow.
    mov     qword [rdi + r8*8], 1
    inc     r8              ; Increase logical length

.add_done:
    mov     qword [r12 + BIGNUM_OFFSET_LEN], r8

    ; Clear unused capacity before normalization.
.zero_rest:
    mov     rcx, BIGNUM_CAPACITY
    sub     rcx, r8
    jz      .normalize

    lea     r15, [r12 + r8*8]
    pxor    xmm0, xmm0
    mov     rax, rcx
    shr     rax, 1
    jz      .zero_odd

    align 16
.zero_sse_loop:
    movdqu  [r15], xmm0
    lea     r15, [r15 + 16]
    dec     rax
    jnz     .zero_sse_loop

.zero_odd:
    test    rcx, 1
    jz      .normalize
    mov     qword [r15], 0

.normalize:
    ; Normalize the published logical length.
    mov     rdi, r12
    mov     rcx, r8
    test    rcx, rcx
    jz      .success

    align 16
.norm_loop:
    mov     rax, [rdi + rcx*8 - 8]
    test    rax, rax
    jnz     .norm_found
    dec     rcx
    jnz     .norm_loop

.norm_found:
    mov     qword [rdi + BIGNUM_OFFSET_LEN], rcx

.success:
    mov     eax, BIGNUM_ADD_U64_SUCCESS
    jmp     .epilogue

    ; --- Special-case handlers. ---
.a_is_zero:
    test    rdx, rdx
    jz      .both_zero
    mov     [rdi], rdx
    mov     r8, 1
    jmp     .add_done
.both_zero:
    mov     r8, 0
    jmp     .add_done

.b_is_zero:
    cmp     rdi, rsi
    je      .zero_rest      ; In-place calls only need tail clearing and normalization.
    mov     rcx, r8
    mov     r14, rsi
    mov     r15, rdi
    jmp     .fast_copy      ; Copy a into result

.err_null:
    mov     eax, BIGNUM_ADD_U64_ERROR_NULL_PTR
    jmp     .epilogue
.err_cap:
    mov     eax, BIGNUM_ADD_U64_ERROR_CAPACITY_EXCEEDED
    jmp     .epilogue
.err_overlap:
    mov     eax, BIGNUM_ADD_U64_ERROR_BUFFER_OVERLAP
    jmp     .epilogue
.err_overflow:
    mov     eax, BIGNUM_ADD_U64_ERROR_OVERFLOW

.epilogue:
    pop     r15
    pop     r14
    pop     r12
    pop     rbp
    ret
