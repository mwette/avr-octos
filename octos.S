;;; octos.S -

;;; Copyright (C) 2019-2022 Matthew R. Wette

;;; Permission is hereby granted, free of charge, to any person obtaining a copy
;;; of this software and associated documentation files (the "Software"), to
;;; deal in the Software without restriction, including without limitation the
;;; rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
;;; sell copies of the Software, and to permit persons to whom the Software is
;;; furnished to do so, subject to the following conditions:
;;;
;;; The above copyright notice and this permission notice shall be included in
;;; all copies or substantial portions of the Software.
;;;
;;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
;;; OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;;; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
;;; THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;;; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
;;; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
;;; DEALINGS IN THE SOFTWARE.

;;; notes:
;;; 1) ISR convention is
;;;    + disabled when an interrupt occurs and
;;;    + enabled when RETI is executed
;;; 2) ABI (gcc, avr-libc)
;;;    + caller saves: r18-r27,r30-r31
;;;    + callee saves: r2-r17,r28-r29
;;;    + callee clear: r1
;;;    + unsaved temp: r0
;;;    + ftn args in r25:r24 .. r9:8
;;;    + ftn ret in r25:r24 .. r19:r18
;;; 3) minimum stack: ret-addr + sreg + mask + NSAVREG + interrupts
;;; 4) difference between push/pop and sts,lds:
;;;    push is post-decrement (SP), sts is pre-decrement (XYZ)
;;;    pop is pre-increment (SP), lds is post-decrement (XYZ)

;;; TODOs
;;; 1) check for opportunities to save insn's using CPI, CPSE
;;; 2) in resched, break if swap-in TCB is 0x0000


#define PORTE_OUTTGL 0x0487
#ifndef OCT_SWINT
#define OCT_SWINT ldi r24,0x80 $ sts PORTE_OUTTGL,r24
#endif

	
;;; === OctOS =================================================================

;;; If you feel brave and you asm-code all non-main tasks to use a reduced set
;;; of registers, you can reduce callee reg set to make context switches faster.

#define PUSH_OSUTIL_REGS \
	push r23 $ push r24 $ push r25
	
#define POP_OSUTIL_REGS \
	pop r25 $ pop r24 $ pop r23
	
#define PUSH_CALLER_REGS \
	push r18 $ push r19 $ push r20 $ push r21 $ push r22 $ \
	push r26 $ push r27 $ push r30 $ push r31

#define POP_CALLER_REGS \
	pop r31  $ pop r30  $ pop r27  $ pop r26  $ pop r22  $ \
	pop r21  $ pop r20  $ pop r19  $ pop r18

#define PUSH_CALLEE_REGS \
	push r0  $ push r1  $ push r2  $ push r3  $ push r4  $ \
	push r5  $ push r6  $ push r7  $ push r8  $ push r9  $ \
	push r10 $ push r11 $ push r12 $ push r13 $ push r14 $ \
	push r15 $ push r16 $ push r17 $ push r28 $ push r29

#define POP_CALLEE_REGS \
	pop r29  $ pop r28  $ pop r17  $ pop r16  $ pop r15  $ \
	pop r14  $ pop r13  $ pop r12  $ pop r11  $ pop r10  $ \
	pop r9   $ pop r8   $ pop r7   $ pop r6   $ pop r5   $ \
	pop r4   $ pop r3   $ pop r2   $ pop r1   $ pop r0

#define OCT_NSAVREG 32

#define PUSH_OTHER_REGS PUSH_CALLER_REGS $ PUSH_CALLEE_REGS
#define POP_OTHER_REGS POP_CALLEE_REGS $ POP_CALLER_REGS

#define OCT_TASK7 0x80
	
	.equ SPL,0x3D
	.equ SPH,0x3E
	.equ SREG,0x3F
	.equ TCB_SIZE,2		; TCB holds stack pointer only

	;; octos functions
	.global oct_os_init
	.global oct_attach_task
	.global oct_detach_task
	.global oct_idle_task
	.global oct_wake_task
	.global oct_spin
	.global oct_rest
	.global oct_swap_task

	.section .data
	.global oct_curmask
	.global oct_rdylist

oct_curmask: .skip 1		; current mask, e.g., 0x07 for id 0x08
oct_rdylist: .skip 1		; list of ready tasks, one per bit
do_swap: .skip 1		; task swap needed
swap_ip: .skip 1		; swap in progress
	.align 4
tcbtabl: .skip 8*TCB_SIZE	; TCB table (stack pointer only)

	.section .text

	;; idle tasks
	.type oct_spin,function
oct_spin:
0:	rjmp 0b
	.size oct_spin, .-oct_spin

	.type oct_rest,function
oct_rest:
0:	sleep
	rjmp 0b
	.size oct_rest, .-oct_rest

	;; void oct_os_init(uint8_t id)
	;; id: main task id (r24)
	.type oct_os_init,function
oct_os_init:
	push r29
	push r28
	mov r25,r24		; load task-id arg
	;; init TCB for TASK7..TASK0
	ldi r27,hi8(tcbtabl)
	ldi r26,lo8(tcbtabl)
	adiw X,7*TCB_SIZE
	ldi r24,OCT_TASK7	; r24 has loop task id
	ldi r23,OCT_TASK7-1	; r23 has loop task mask
	clr r20			; init SPH,SPL to zero
0:	lsr r23			; shift task mask
	lsr r24			; shift task id
	breq 2f			; done?
	cp r24,r25		; curr-id == caller-id ?
	brne 1f
	sts oct_curmask,r23	; log caller if so
1:	st -X,r20		; unallocated SPL
	st -X,r20		; unallocated SPH
	rjmp 0b
	;; log caller
	st -X,r20		; 0 => SPL in TCB
	st -X,r20		; 0 => SPH in TCB
	rjmp 0b
2:	;; update vars
	ldi r21,0x80
	or r21,r25		; add task-id arg
	sts oct_rdylist,r21	; updated ready list
	sts do_swap,r20		; for swap during
	sts swap_ip,r20		;   interrupts
	;; return
	pop r28
	pop r29
	ret
	.size oct_os_init, .-oct_os_init

	;; void oct_register_task(uint8_t id, void (*fn)(void),
	;;                        uint8_t *sp, uint16_t sz)
	;; id: task id (r24)
	;; fn: function (r23:r22)
	;; sp: stack memory pointer (r21:r20)
	;; sz: stack size (r19:r18)
	.type oct_attach_task,function
oct_attach_task:
	;; from task id, generate oct_curmask
	mov r25,r24
	subi r25,1
	;; load Z with TCB table end addr
	ldi r30,lo8(tcbtabl)
	ldi r31,hi8(tcbtabl)
0:	adiw Z,TCB_SIZE
	lsr r24
	brcc 0b
	;; stack top addr in r21:r20
	add r20,r18
	adc r21,r19
	;; load X with stack pointer
	movw r26,r20
	sbiw X,1
	;; save return address
	st -X,r22
	st -X,r23
	;; room for registers plus sreg
	sbiw X,OCT_NSAVREG+1
	;; save mask
	st -X,r25
	;; adjust for pop pre increment
	sbiw X,1
	;; save stack pointer to TCB
	st -Z,r27
	st -Z,r26
 	ret
	.size oct_attach_task, .-oct_attach_task

oct_detach_task:
	;; load Z with TCB table end addr
	ldi r30,lo8(tcbtabl)
	ldi r31,hi8(tcbtabl)
0:	adiw Z,TCB_SIZE
	lsr r24
	brcc 0b
	;; zero out the TCB
	st -Z,r24
	st -Z,r24
	ret
	.size oct_detach_task, .-oct_detach_task
	
	;; void oct_wake_task(uint8_t id_set)
	;; id: or'd ids of tasks to wake (r24)
	.type oct_wake_task,function
oct_wake_task:
	in r23,SREG
	cli
	lds r25,oct_rdylist
	or r25,r24
	sts oct_rdylist,r25
	lds r24,oct_curmask
	and r25,r24
	breq 1f
	OCT_SWINT
1:	out SREG,r23
	ret
	.size oct_wake_task, .-oct_wake_task

	;; oct_idle_task(uint8_t id)
	;; id: task ID to idle (r24)
	.type oct_idle_task,function
oct_idle_task:
	andi r24,~OCT_TASK7	; never idle task7
	com r24			; ones complement
	in r23,SREG
	cli
	lds r25,oct_rdylist
	and r25,r24
	sts oct_rdylist,r25
	OCT_SWINT
	out SREG,r23
	ret
	.size oct_idle_task, .-oct_idle_task

	;; void oct_swap_task()
	.type oct_swap_task,function
oct_swap_task:
	;; move following to SWINT?
	PUSH_OSUTIL_REGS	; push r23, r24, r25
	in r23,SREG
	cli
	;; if running, flag and return
	lds r25,swap_ip
	tst r25
	breq swap_body
	sts do_swap,r25		; flag to re-run
	out SREG,r23
	POP_OSUTIL_REGS
	ret
swap_body:
	lds r24,oct_curmask
	ldi r25,1
	sts swap_ip,r25
	;; interrupts enabled in this section
	sei
	push r23		; push SREG
	PUSH_OTHER_REGS	 	; save registers
	push r24		; push curmask
	;; incoming TCB addr => Y (TODO: if Y is zero, abort)
	lds r25,oct_rdylist
	ldi r28,lo8(tcbtabl)
	ldi r29,hi8(tcbtabl)
0:	lsr r25
 	brcs 1f
	adiw Y,TCB_SIZE
	rjmp 0b
1:	ld r22,Y+		; load incoming SPL from TCB
	ld r23,Y+		; load incoming SPH from TCB
	;; outgoing TCB addr => Y
	ldi r28,lo8(tcbtabl)
	ldi r29,hi8(tcbtabl)
0:	lsr r24
	brcc 1f
	adiw Y,TCB_SIZE
	rjmp 0b
	;; swap stack pointers
1:	cli
	in r24,SPL
	in r25,SPH
	out SPL,r22
	out SPH,r23
	sei
	st Y+,r24		; save outgoing SPL to TCB
	st Y+,r25		; save outgoing SPH to TCB
	pop r24			; load curmask
	sts oct_curmask,r24	;   and restore 
	POP_OTHER_REGS		; restore registers
	pop r23			; load SREG, restore later
	;; trailer
	cli
	lds r25,do_swap
	tst r25
	breq 1f
	;; 
	;; From SPIN testing: (otherwise infinite loop IIRC)
	;; 1) done if (rdylist & curmask) == 0
	lds r25,oct_rdylist
	and r25,r24
	breq 1f
	;; 2) done if rdylist & (curmask+1) != 0
	lds r25,oct_rdylist
	inc r24
	and r25,r24
	brne 1f
	;; 
	;; rerun task swap
	sts do_swap,r1
	rjmp swap_body		; redo w/ r23=SREG, r24=curmask
1:	;; done
	sts swap_ip,r1		; r1 == 0 here
	out SREG,r23		; restore SREG
	POP_OSUTIL_REGS
	ret
	.size oct_swap_task, .-oct_swap_task

;;; --- last line ---
