/* octos.h - 
 * 
 * Copyright (C) 2019-2022 Matthew R. Wette
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

#ifndef __ASSEMBLER__
#ifndef OCT_TMPREG
#define OCT_TMPREG GPIOR0
#endif
#ifndef OCT_PORT
#define OCT_PORT PORTE
#endif
#ifndef OCT_PIN_bm
#define OCT_PIN_bm PIN0_bm
#endif
#ifndef OCT_SWISR
#define OCT_SWISR() asm(	\
  "  out %0,r22\n"		\
  "  ldi r22,%1\n"		\
  "  sts %2,r22\n"		\
  "  ldi r22,lo8(%3)\n"		\
  "  push r22\n"		\
  "  ldi r22,hi8(%3)\n"		\
  "  push r22\n"		\
  "  in r22,%0\n"		\
  "  reti\n"			\
  :: "m"(OCT_TMPREG), "n"(OCT_PIN_bm), \
     "n"(_SFR_MEM_ADDR(OCT_PORT.INTFLAGS)), "m"(oct_swap_task))
// The above code basically makes the ISR look like the
// executing non-interrupt code made a call to oct_swap_task.
// After RETI is executed, the previous PC is on the stack.
#endif
#endif

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

void oct_idle_task(uint8_t id_set) __attribute__((noinline));
void oct_wake_task(uint8_t id_set) __attribute__((noinline));

void oct_spin();
void oct_rest();
void oct_swap_task();

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
.global oct_spin, oct_rest
.global oct_swap_task

#endif /* __ASSEMBLER__ */

#if OCT_SHORT_NAMES
#define TASK0 OCT_TASK0
#define TASK1 OCT_TASK1
#define TASK2 OCT_TASK2
#define TASK3 OCT_TASK3
#define TASK4 OCT_TASK4
#define TASK5 OCT_TASK5
#define TASK6 OCT_TASK6
#define TASK7 OCT_TASK7
#define os_init oct_os_init
#define attach_task oct_attach_task
#define detach_task oct_detach_task
#define wake_task oct_wake_task
#define idle_task oct_idle_task
#endif

#endif
/* --- last line --- */
