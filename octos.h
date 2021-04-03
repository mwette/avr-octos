/* octos.h - 
 * 
 * Copyright (C) 2019-2021 Matthew R. Wette
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef OCTOS_H_
#define OCTOS_H_

#define OCT_TASK0 0x01
#define OCT_TASK1 0x02
#define OCT_TASK2 0x04
#define OCT_TASK3 0x08
#define OCT_TASK4 0x10
#define OCT_TASK5 0x20
#define OCT_TASK6 0x40
#define OCT_TASK7 0x80

#ifndef __ASSEMBLER__
#include <stdint.h>

void oct_os_init(uint8_t id);
void oct_attach_task(uint8_t id, void (*fn)(void), uint8_t *sa, uint16_t sz);
void oct_detach_task(uint8_t id);

void oct_idle_task(uint8_t id_set);
void oct_wake_task(uint8_t id_set);

void oct_isr_idle_task_1(uint8_t id_set);
void oct_isr_wake_task_1(uint8_t id_set);

void oct_spin();
void oct_rest();

static inline void oct_isr_wake_task(uint8_t id_set) {
  asm volatile(" mov r24,%0\n"
	       " call oct_isr_wake_task_1\n"
	       : : "r"(id_set) : "r23", "r24", "r25");
}

static inline void oct_isr_idle_task(uint8_t id_set) {
  asm volatile(" mov r24,%0\n"
	       " call oct_idle_task_1\n"
	       : : "r"(id_set) : "r23", "r24", "r25");
}

static inline uint8_t oct_cur_task(void) {
  extern uint8_t oct_curmask;
  return (oct_curmask + 1);
}

#else /* __ASSEMBLER__ */

.global oct_init
.global oct_attach_task
.global oct_detach_task
.global oct_idle_task
.global oct_wake_task
.global oct_isr_idle_task_1
.global oct_isr_wake_task_1
.global oct_spin, oct_rest

#define oct_isr_wake_task oct_wake_task_1
#define oct_isr_idle_task oct_idle_task_1

#endif /* __ASSEMBLER__ */

#endif
/* --- last line --- */
