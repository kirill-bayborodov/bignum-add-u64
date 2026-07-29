; -----------------------------------------------------------------------------
; @file    bignum_add_u64.asm
; @author  git@bayborodov.com
; @version 1.0.0
; @date    29.07.2026
;
; @brief   Экстремально оптимизированная реализация сложения числа и 64-битного скаляра.
;
; @details
;   Использует System V AMD64 ABI.
;   Оптимизации:
;   - In-place Fast Exit (мгновенный выход без копирования, если result == a и CF=0)
;   - Branchless Overlap Check (проверка перекрытия без ветвлений)
;   - CF Preservation (сохранение флага переноса через lea/dec)
;   - SSE Fast Copy (векторизованное копирование остатка)
;   - Lazy Zeroing (быстрое обнуление хвоста через pxor/movdqu)
; -----------------------------------------------------------------------------

section .text
global bignum_add_u64

; --- Константы ---
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
    ; Аргументы:
    ; rdi = bignum_t *result
    ; rsi = const bignum_t *a
    ; rdx = uint64_t b

    push    rbp
    mov     rbp, rsp
    push    r12
    push    r14
    push    r15

    ; 1. Проверка на NULL
    test    rdi, rdi
    je      .err_null
    test    rsi, rsi
    je      .err_null

    ; 2. Проверка длины a->len
    mov     r8, qword [rsi + BIGNUM_OFFSET_LEN]
    cmp     r8, BIGNUM_CAPACITY
    ja      .err_cap

    ; 3. Branchless проверка перекрытия буферов (разрешаем in-place rdi == rsi)
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

    mov     r12, rdi        ; Сохраняем указатель на result

    ; 4. Fast path: a->len == 0
    test    r8, r8
    jz      .a_is_zero

    ; 5. Fast path: b == 0
    test    rdx, rdx
    jz      .b_is_zero

    ; 6. Основной алгоритм сложения
    mov     rax, [rsi]
    add     rax, rdx        ; Складываем младшее слово с b, устанавливаем CF
    mov     [rdi], rax

    mov     rcx, r8
    dec     rcx             ; rcx = оставшиеся слова (a->len - 1)
    jz      .check_final_carry

    lea     r14, [rsi + 8]  ; Указатель чтения
    lea     r15, [rdi + 8]  ; Указатель записи

    align 16
.add_loop:
    jnc     .fast_copy      ; Если переноса нет, выходим из цикла сложения
    
    mov     rax, [r14]
    adc     rax, 0          ; Прибавляем перенос
    mov     [r15], rax
    
    lea     r14, [r14 + 8]
    lea     r15, [r15 + 8]
    dec     rcx
    jnz     .add_loop
    jmp     .check_final_carry

.fast_copy:
    ; Если переноса больше нет, проверяем, in-place ли это операция
    cmp     r12, rsi
    je      .sub_done       ; Если in-place, остаток массива уже на месте! Мгновенный выход.

    ; Иначе копируем оставшиеся rcx слов из r14 в r15
    test    rcx, rcx
    jz      .sub_done
    mov     rax, rcx
    shr     rax, 1          ; rax = количество 16-байтных блоков
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
    jz      .sub_done
    mov     rax, [r14]
    mov     [r15], rax
    jmp     .sub_done

.check_final_carry:
    jnc     .sub_done
    cmp     r8, BIGNUM_CAPACITY
    jae     .err_overflow   ; Переполнение
    mov     qword [rdi + r8*8], 1
    inc     r8              ; Увеличиваем длину

.sub_done:
    mov     qword [r12 + BIGNUM_OFFSET_LEN], r8

    ; 7. Lazy Zeroing (обнуление неиспользуемого хвоста)
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
    ; 8. Нормализация результата
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

    ; --- Обработчики особых случаев ---
.a_is_zero:
    test    rdx, rdx
    jz      .both_zero
    mov     [rdi], rdx
    mov     r8, 1
    jmp     .sub_done
.both_zero:
    mov     r8, 0
    jmp     .sub_done

.b_is_zero:
    cmp     rdi, rsi
    je      .zero_rest      ; Если in-place, просто обнуляем хвост и нормализуем
    mov     rcx, r8
    mov     r14, rsi
    mov     r15, rdi
    jmp     .fast_copy      ; Копируем a в result

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
