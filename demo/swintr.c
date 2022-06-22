
#if 0
//attiny3217 nano board

#include <avr/io.h>
#include <avr/interrupt.h>

static void ledToggle() {
  PORTA.DIRSET = PIN3_bm;
  PORTA.OUTTGL = PIN3_bm;
}

static void PB7irq(){
  PORTB.PIN7CTRL = 1;
  PORTB.DIRSET = PIN7_bm;
  PORTB.OUTTGL = PIN7_bm;
}

ISR(PORTB_PORT_vect){
    uint8_t flags = PORTB.INTFLAGS;
    if (flags & PIN7_bm) ledToggle();
    PORTB.INTFLAGS = flags;
}

int main(){
    sei();
    while (1) {
      PB7irq();
      __builtin_avr_delay_cycles(3333333ul/2); //1Hz blink
    }
}

#endif


#ifdef F_CPU
#undef F_CPU
#endif
#define F_CPU 5000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

#define TRY_DIRECT 0
#define USE_EVSYS  0

volatile uint8_t dim = 0;

ISR(TCA0_OVF_vect) {
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_ENABLE_bm;
#if 0
#if TRY_DIRECT
  dim = 1;
#elif USE_EVSYS
  EVSYS.STROBE = PIN3_bm;
#else
  PORTE.OUTTGL = PIN0_bm;
#endif
#endif
  PORTF.OUTTGL = PIN5_bm;
}

ISR(PORTE_PORT_vect) {
  PORTE.INTFLAGS = PIN0_bm;
  dim = 1;
}

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

  init_sys_clk();			/* init clock to 5 MHz */
  init_wdt();				/* init WDT to 8 sec */
  init_tca();				/* init TCA to 3 sec */

  PORTF.DIRSET = PIN5_bm;		/* LED pin as output */
  PORTF.OUTSET = PIN5_bm;		/* LED off */

#if USE_EVSYS
  /* SW intr via event system to PORTE interrupt: EVSYS.STROBE = PIN2_bm */
  PORTE.DIRCLR = PIN2_bm;
  PORTE.PIN2CTRL = PORT_ISC_BOTHEDGES_gc;
  EVSYS.USEREVOUTE = EVSYS_CHANNEL_CHANNEL3_gc;
#else
  /* does not work :( */
  PORTE.PIN0CTRL = PORT_ISC_BOTHEDGES_gc;
  PORTE.DIRSET = PIN0_bm;		/* PORTE as SW intr */
#endif
  
  sei();
  TCA0.SINGLE.CTRLESET = TCA_SINGLE_CMD_RESTART_gc; /* restart timeer */
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_ENABLE_bm; /* enable timer intr */

  /* Make LED dim. */
  while (1) {
    wdt_reset();

#if 0
    if (dim) {
      PORTF.OUTCLR = PIN5_bm;
      _delay_us(1);
      PORTF.OUTSET = PIN5_bm;
      _delay_us(20);
    } else {
      PORTF.OUTCLR = PIN5_bm;
      _delay_us(15);
      PORTF.OUTSET = PIN5_bm;
      _delay_us(5);
    }
#endif
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
