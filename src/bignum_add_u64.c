/**
 * @file    bignum_add_u64.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    29.07.2026
 *
 * @brief   Эталонная реализация сложения большого числа и 64-битного скаляра на C11.
 */

#include "bignum_add_u64.h"

/**
 * @brief Внутренняя функция для проверки недопустимого перекрытия буферов.
 *
 * @param[in] res Указатель на буфер результата.
 * @param[in] op  Указатель на входной буфер (операнд).
 * @return 1 если есть частичное перекрытие, 0 если всё в порядке или это in-place.
 */
static inline int check_buffer_overlap(const bignum_t *res, const bignum_t *op) {
    if (res == op) {
        return 0; // In-place операция (result == a) разрешена
    }
    
    const unsigned char *p_res = (const unsigned char *)res;
    const unsigned char *p_op  = (const unsigned char *)op;

    // Проверяем пересечение диапазонов памяти
    if ((p_res < p_op + sizeof(bignum_t)) && (p_op < p_res + sizeof(bignum_t))) {
        return 1;
    }
    return 0;
}

bignum_add_u64_status_t bignum_add_u64(bignum_t *result, const bignum_t *a, const uint64_t b) {
    // 1. Валидация указателей
    if (!result || !a) {
        return BIGNUM_ADD_U64_ERROR_NULL_PTR;
    }

    // 2. Проверка длины
    if (a->len > BIGNUM_CAPACITY) {
        return BIGNUM_ADD_U64_ERROR_CAPACITY_EXCEEDED;
    }

    // 3. Проверка перекрытия буферов
    if (check_buffer_overlap(result, a)) {
        return BIGNUM_ADD_U64_ERROR_BUFFER_OVERLAP;
    }

    // 4. Быстрый путь: a == 0
    if (a->len == 0) {
        if (b == 0) {
            result->len = 0;
        } else {
            result->words[0] = b;
            result->len = 1;
        }
        return BIGNUM_ADD_U64_SUCCESS;
    }

    // 5. Быстрый путь: b == 0
    if (b == 0) {
        if (result != a) {
            for (size_t i = 0; i < a->len; i++) {
                result->words[i] = a->words[i];
            }
        }
        size_t new_len = a->len;
        // Нормализация на случай, если 'a' содержало ведущие нули
        while (new_len > 0 && result->words[new_len - 1] == 0) {
            new_len--;
        }
        result->len = new_len;
        return BIGNUM_ADD_U64_SUCCESS;
    }

    // 6. Основной алгоритм сложения
    uint64_t carry = b;
    size_t i = 0;

    for (; i < a->len; i++) {
        uint64_t w = a->words[i];
        uint64_t sum = w + carry;
        
        carry = (sum < w) ? 1 : 0; // Вычисляем перенос
        result->words[i] = sum;

        // Ранний выход: если перенос затух, дальше складывать не с чем
        if (carry == 0) {
            i++;
            break;
        }
    }

    // 7. Копирование оставшегося хвоста (если вышли досрочно и это не in-place)
    if (result != a) {
        for (; i < a->len; i++) {
            result->words[i] = a->words[i];
        }
    }

    size_t new_len = a->len;

    // 8. Обработка финального переноса
    if (carry) {
        if (new_len == BIGNUM_CAPACITY) {
            return BIGNUM_ADD_U64_ERROR_OVERFLOW;
        }
        result->words[new_len] = 1;
        new_len++;
    }

    // 9. Нормализация результата
    while (new_len > 0 && result->words[new_len - 1] == 0) {
        new_len--;
    }
    result->len = new_len;

    return BIGNUM_ADD_U64_SUCCESS;
}
