# Поддержка вывода IEEE-754 чисел с плавающей запятой 32- и 64-битной точности в соответствии со стандартами C и POSIX

Репозиторий: https://sed.ispras.ru/git/d.titov-osprac

Ветка: `itask`

Цель: добавить поддержку корректного вывода IEEE-754 float/double через `printf`-совместимые функции в JOS, плюс поддержку FPU/SSE контекста в ядре, и тесты (в userspace). Всё должно работать с ASAN/UBSAN.

---

## Что сделано

### 1) SSE2/SSE и FPU контекст в ядре (fxsave64/fxrstor64)

**Хранение контекста на env:**

- Добавлено поле `env_fxsave[512]` в `struct Env` (выравнивание 16 байт).
  - Файл: `inc/env.h`
  - Макрос: `FXSAVE_AREA_SIZE 512`

**Инструкции fxsave/fxrstor:**

- В `inc/x86.h` добавлены helper функции:
  - `fninit()`
  - `fxsave64(void *addr)`
  - `fxrstor64(void *addr)`

**Инициализация “чистого” fp/sse state:**

- В `kern/env.c` сделано `fpstate_init_once()`:
  - `fninit(); fxsave64(fxsave_init);`
  - Далее при `env_alloc` новый env получает `memcpy(env->env_fxsave, fxsave_init, ...)`.

**Сохранение/восстановление при переключении env:**

- В `kern/env.c` в `env_run()`:
  - перед уходом со старого env: `fxsave64(curenv->env_fxsave)`
  - перед входом в новый env: `fxrstor64(curenv->env_fxsave)`

**fork/exofork:**

- В `kern/syscall.c` в `sys_exofork()`:
  - перед копированием: `fxsave64(curenv->env_fxsave)`
  - новый env получает копию `env_fxsave`

**Включение SSE/FPU на уровне процессора:**

- В `kern/pmap.c` в инициализации CPU выставлены флаги:
  - `CR0_NE | CR0_MP` (и убран `CR0_EM`)
  - `CR4_OSFXSR | CR4_OSXMMEXCPT`

Проверка этого пункта сделана отдельным тестом `user/fputest.c` (см. ниже).

---

### 2) Включить SSE/SSE2/FPU при компиляции userspace

Добавлены флаги для userspace в `GNUmakefile`: `-msse -msse2 -mfpmath=sse`

Отдельно: для kernel по умолчанию SSE выключен, но для объектов, которые обязаны понимать `double` по ABI (printfmt/fpconv), включены специальные флаги (см. пункт 3).

---

### 3) `%f %e %g` в `cprintf()`, `sprintf()`, `snprintf()` по printf-логике

**Парсинг спецификаторов в printfmt:**
- В `vprintfmt()` добавлена обработка `%f/%e/%g` (и upper-case варианты).
- Форматирование вынесено в функции `fp_format_f/fp_format_e/fp_format_g`.
  - Файл: `lib/printfmt.c`
  - Заголовок: `inc/fpconv.h`
  - Реализация: `lib/fpconv.c`

**Сборка kernel-части printf с поддержкой double:**
- Kernel в целом компилируется с `-mno-sse`, но:
  - `$(OBJDIR)/kern/printfmt.o` и `$(OBJDIR)/kern/fpconv.o` собираются с:
    - `FP_PRINTF_CFLAGS := -msse -msse2 -mfpmath=sse`
  - Файл: `kern/Makefrag`
- Это нужно, чтобы kernel мог корректно собирать/линковать код, который принимает/возвращает `double` (x86_64 ABI).

---

### 5) Синтетические тесты (userspace)

Тесты сделаны как отдельные user-программы, которые печатают маркеры `ITASK_*: OK/FAIL`. Их запускает `grade-itask`, реализованный по формату `grade-lab12`.

#### Простые вручную написанные тесты

- `user/fputest.c`
  - проверяет, что x87 rounding mode не “ломается” при частых `sys_yield()` и при `fork()` (т.е. env-switch + копирование состояния работают).
- `user/printfloat.c`
  - базовая проверка, что `%f/%e/%g` вообще работают и дают ожидаемые строки.
- `user/itask_printf_f.c`
- `user/itask_printf_e.c`
- `user/itask_printf_g.c`
- `user/itask_printf_width.c`
  - проверки форматов/ширины/precision/флагов на небольших наборах.

#### Автоматически сгенерированные тесты

В репозиторий скопирован исходник из musl testsuite: `third_party/musl-libc-testsuite/snprintf.c`
Дальше написанный скрипт `tools/gen_musl_snprintf_fp.py` генерирует `user/itask_musl_snprintf_fp.c` - набор проверок на `snprintf` .

#### Запуск

- Без санитайзеров: `make grade`
- С санитайзерами: `JOSLLVM=1 UASAN=1 KASAN=1 UUBSAN=1 KUBSAN=1 make grade`

---

## Изменённые/добавленные файлы

Kernel:
- `inc/env.h`
- `inc/x86.h`
- `kern/env.c`
- `kern/syscall.c`
- `kern/pmap.c`
- `kern/Makefrag`

Lib/printf:
- `lib/printfmt.c`
- `inc/fpconv.h`
- `lib/fpconv.c`

Ryu tables/headers:
- `inc/ryu/*`
- `lib/ryu_d2printf.c`

Тесты:
- `grade-lab13`
- `user/fputest.c`
- `user/printfloat.c`
- `user/itask_printf_*.c`
- `user/itask_musl_snprintf_fp.c`
- `third_party/musl-libc-testsuite/snprintf.c`
- `tools/gen_musl_snprintf_fp.py`

---

## Текущее состояние

- `make grade` проходит на 100/100.
- `JOSLLVM=1 UASAN=1 KASAN=1 UUBSAN=1 KUBSAN=1 make grade` проходит на 100/100.
- Код разнесён по отдельным файлам (`fpconv.*`, `ryu_*`, тесты отдельно), и изменения сделаны так, чтобы kernel включал SSE только где нужно.
