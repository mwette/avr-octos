/* demo program for OctOS on ATmega4809 Curiosity Nano
 
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

 * In OctOS:
 * + TASK7 is the idle task; suggest oct_spin or oct_rest
 * + TASK6 is the lowest priority user task, usually main
 * + TASK0 is the highest priority task

 * This demo has main (TASK6) and two other tasks.
 * + TASK6 sets LED dim while running
 * + TASK3 sets LED medium for about 0.5 s, then sleeps
 * + TASK2 sets LED bright for about 0.2 s, then sleeps
 * + TCA0 timer wakes TASK2 and TASK3 about every 2 s

 * Octos now uses a software interrupt, here implemented via PORTE:
 * + Configure PORTE as input: PORTE.DIR |= 0x80;
 * + Set interrupt on toggle: PORTE.PIN0CTRL |= PORT_ISC_BOTHEDGES_gc.
 * + Define SWINT, SWISR as in in octos.h

 */

#ifdef F_CPU
#undef F_CPU
#endif
#define F_CPU 5000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "octos.h"

ISR(PORTE_PORT_vect, ISR_NAKED) {
  OCT_SWISR();
}

ISR(TCA0_OVF_vect) {
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_ENABLE_bm;
  oct_wake_task(OCT_TASK2 | OCT_TASK3);	/* calling => push CALLER_SAVES */
  PORTE.OUTTGL = PIN0_bm;
}

/* ---------------------------------------------------------------------------*/

#define T2_ITER 15000			/* iterations before idle */
#define T3_ITER 30000			/* iterations before idle */
#define T2_STKSZ 0x40			/* task2 stacksize */
#define T3_STKSZ 0x40			/* task3 stacksize */
#define T7_STKSZ 0x20			/* task7 (idle) stacksize */

/* Alignment on stacks is to aid my debugging. */

uint8_t task2_stk[T2_STKSZ] __attribute__((aligned(16)));

/* Make LED bright. */
void __attribute__((OS_task)) task2(void) {
  sei();				/* since per task status register */
  while (1) {
    oct_idle_task(OCT_TASK2);
    for (int i = 0; i < T2_ITER; i++) {
      PORTF.OUTCLR = PIN5_bm;
      _delay_us(9);
      PORTF.OUTSET = PIN5_bm;
      _delay_us(1);
    }
  }
}

uint8_t task3_stk[T3_STKSZ] __attribute__((aligned(16)));

/* Make LED medium. */
void __attribute__((OS_task)) task3(void) {
  sei();
  while (1) {
    oct_idle_task(OCT_TASK3);
    for (int i = 0; i < T3_ITER; i++) {
      PORTF.OUTCLR = PIN5_bm;
      _delay_us(2);
      PORTF.OUTSET = PIN5_bm;
      _delay_us(8);
    }
  }
}

uint8_t task7_stk[T7_STKSZ] __attribute__((aligned(16)));

/* ---------------------------------------------------------------------------*/

void nano_button_wait();

void init_sys_clk() {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSC20M_gc);
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_4X_gc | CLKCTRL_PEN_bm);
  while ((CLKCTRL.MCLKSTATUS & 0x11) != 0x10); /* wait for stable OSC20M */
}

void init_wdt() {
  _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_8KCLK_gc); /* 8 sec watchdog */
}

void init_tca() {
  TCA0.SINGLE.PER = 3*9745;
  TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV256_gc | TCA_SINGLE_ENABLE_bm;
}

void main(void) {
  if (SYSCFG.REVID < 0xFF) nano_button_wait(); /* not sim: wait for button */

  oct_os_init(OCT_TASK6);
  oct_attach_task(OCT_TASK7, oct_spin, task7_stk, T7_STKSZ); /* required */
  oct_attach_task(OCT_TASK3, task3, task3_stk, T3_STKSZ);
  oct_attach_task(OCT_TASK2, task2, task2_stk, T2_STKSZ);

  /* SW intr via PORTE PIN0 */
  PORTE.DIRSET = PIN0_bm;
  PORTE.PIN0CTRL = PORT_ISC_BOTHEDGES_gc;

  init_sys_clk();			/* init clock to 5 MHz */
  init_wdt();				/* init WDT to 8 sec */
  init_tca();				/* init TCA to 3 sec */

  PORTF.DIRSET = PIN5_bm;		/* LED pin as output */
  PORTF.OUTSET = PIN5_bm;		/* LED off */
  
  oct_wake_task(OCT_TASK3 | OCT_TASK2);	/* let tasks initialize */
  
  sei();
  TCA0.SINGLE.CTRLESET = TCA_SINGLE_CMD_RESTART_gc; /* restart timeer */
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_ENABLE_bm; /* start timer */

  /* Make LED dim. */
  while (1) {
    wdt_reset();

    PORTF.OUTCLR = PIN5_bm;
    _delay_us(1);
    PORTF.OUTSET = PIN5_bm;
    _delay_us(20);
  }
}

/* ---------------------------------------------------------------------------*/

/* Flash LED and wait for button press to start application code. */
void nano_button_wait() {
  uint8_t st = 1;			/* switch state */
  uint8_t lc;				/* led counter */

  TCB0.CCMP = 333;			/* debounce time */
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;
  PORTF.DIRSET = PIN5_bm;		/* LED output */
  
  while (st) {
    switch (st) {
    case 1: if ((PORTF.IN & PIN6_bm) == 0) st = 2; break;
    case 2: if ((PORTF.IN & PIN6_bm) != 0) st = 0; break;
    }
    if (lc++ == 0) PORTF.OUTTGL = PIN5_bm; /* toggle LED */
    while ((TCB0.INTFLAGS & 0x01) == 0);   /* wait for bounce */
    TCB0.INTFLAGS = 0x01;		   /* reset flag */
  }

  /* Restore MCU state. */
  PORTF.DIRCLR = PIN5_bm;
  TCB0.CTRLA = 0x00;
  TCB0.CCMP = 0;
  TCB0.INTFLAGS = 0x01;
}

/* --- last line --- */
