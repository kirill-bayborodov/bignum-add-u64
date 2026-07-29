/**
 * @file    bignum_add_u64.h
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    29.07.2026
 *
 * @brief   Модуль для сложения большого беззнакового целого числа и 64-битного скаляра.
 * @ingroup bignum_arithmetic
 *
 * @details
 *   Определяет API для функции bignum_add_u64, включая типы данных,
 *   коды состояния и прототипы функций.
 *
 * @history
 *   - rev. 0 (29.07.2026): Первоначальное создание API.
 *
 * @see     bignum.h
 * @since   1.1.2
 */

#ifndef BIGNUM_ADD_U64_H
#define BIGNUM_ADD_U64_H

#include <bignum.h>
#include <stddef.h>
#include <stdint.h>

// Проверка на наличие определения BIGNUM_CAPACITY из общего заголовка
#ifndef BIGNUM_CAPACITY
#  error "bignum.h must define BIGNUM_CAPACITY"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Коды состояния для функции bignum_add_u64.
 */
typedef enum {
    BIGNUM_ADD_U64_SUCCESS                 =  0, /**< Успешное выполнение. */
    BIGNUM_ADD_U64_ERROR_NULL_PTR          = -1, /**< Один из входных указателей `NULL`. */
    BIGNUM_ADD_U64_ERROR_CAPACITY_EXCEEDED = -2, /**< Длина операнда `a` превышает `BIGNUM_CAPACITY`. */
    BIGNUM_ADD_U64_ERROR_BUFFER_OVERLAP    = -3, /**< Обнаружено недопустимое перекрытие буферов. */
    BIGNUM_ADD_U64_ERROR_OVERFLOW          = -4  /**< Переполнение: результат не помещается в `BIGNUM_CAPACITY`. */
} bignum_add_u64_status_t;

/**
 * @brief Выполняет сложение большого беззнакового целого числа и 64-битного числа.
 *
 * @details
 *   ### Алгоритм
 *   1.  **Валидация:** Проверяются входные указатели `result` и `a` на `NULL`.
 *   2.  **Проверка длины:** Проверяется, что `a->len` не превышает `BIGNUM_CAPACITY`.
 *   3.  **Проверка перекрытия:** Проверяется, что диапазоны памяти `result` и `a`
 *       не пересекаются (при этом in-place операция `result == a` разрешена).
 *   4.  **Сложение:** Выполняется сложение `b` с младшим словом `a`.
 *   5.  **Распространение переноса:** Если возник перенос (carry), он прибавляется
 *       к следующим словам `a`, пока не затухнет или пока не закончатся слова.
 *   6.  **Переполнение:** Если после обработки всех слов `a` перенос остался,
 *       длина результата увеличивается на 1. Если при этом длина превышает
 *       `BIGNUM_CAPACITY`, возвращается ошибка `BIGNUM_ADD_U64_ERROR_OVERFLOW`.
 *   7.  **Нормализация:** Длина результата устанавливается корректно (удаляются ведущие нули,
 *       если они были в исходном числе `a`, а `b` было равно 0).
 *
 *   ### Потокобезопасность
 *   Функция является потокобезопасной, так как не использует глобальное или
 *   статическое состояние.
 *
 * @param[out] result Указатель на структуру `bignum_t` для записи суммы.
 * @param[in]  a      Указатель на `bignum_t`, первое слагаемое.
 * @param[in]  b      64-битное беззнаковое целое, второе слагаемое.
 *
 * @return bignum_add_u64_status_t Код состояния операции.
 * @retval BIGNUM_ADD_U64_SUCCESS                 Успешное выполнение.
 * @retval BIGNUM_ADD_U64_ERROR_NULL_PTR          Один из входных указателей `NULL`.
 * @retval BIGNUM_ADD_U64_ERROR_CAPACITY_EXCEEDED Длина операнда `a` превышает `BIGNUM_CAPACITY`.
 * @retval BIGNUM_ADD_U64_ERROR_BUFFER_OVERLAP    Обнаружено перекрытие буферов.
 * @retval BIGNUM_ADD_U64_ERROR_OVERFLOW          Результат превышает максимальную вместимость.
 */
bignum_add_u64_status_t bignum_add_u64(bignum_t *result, const bignum_t *a, const uint64_t b);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_ADD_U64_H */
