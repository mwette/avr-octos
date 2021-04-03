** OctOS - a tasking system for AVR CPUs **

# Overview
OctOS is intended to be a simple-to-use tasking system for AVR CPUs.
It consists of a single assembly file, ``octos.S``, and a single header
file, ``octos.h``.  Features of OctOS are
* OctOS can support seven tasks plus an idle task.
* Stack space for non-main tasks must be user-provided.
OctOS comes with no warranty.  Use at your own risk.

# API
* Task IDs are ``OCT_TASK0`` to ``OCT_TASK7``, with ``0`` the highest priority.
* ``oct_os_init(id)``: initialize, must be called first
  * id is the task id for main task
* ``oct_attach_task(id, ftn, stk, siz)``: attach a task
  * id is the ``OCT_TASK0`` .. ``OCT_TASK7``
  * ftn is the task function
  * stk is data for stack
  * siz is the stack size
* ``oct_wake_task(id-set)``, ``oct_isr_wake_task(id-set)``: wake-up tasks
  * id-set is OR'ed list of tasks to wake up; e.g., ``OCT_TASK2|OCT_TASK3``
* ``oct_idle_task(id-set)``, ``oct_isr_idle_task(id-set)``: sleep tasks
  * id-set is OR'ed list of tasks to put to sleep
* ``oct_detach_task(id)``: remove task from the system
* notes:
  * The ``_isr_`` versions of wake and idle must be used in ISRs
  * Predefined idle tasks are ``oct_spin`` and ``oct_rest``.  The latter
    one will put the CPU to sleep.
  * There is no separate ISR stack so all tasks must provide enough
    space to support interrupts.
  * ``oct_idle_task(OCT_TASK7)`` has no effect: the idle task cannot
    be idled, it is always ready

# Example:
  ```
  uint8_t idle[0x80], stk2[0x80], stk3[0x80];
  void task2() { while (1) { oct_idle_task(OCT_TASK2); ... } }
  void task3() { while (1) { oct_idle_task(OCT_TASK3); ... } }

  ISR(TIMER_vect) { ...; oct_isr_wake_task(OCT_TASK2 | OCT_TASK3); }

  int main() {
    oct_os_init(OCT_TASK6);  // main is TASK6 (lowest priority)
    oct_attach_task(OCT_TASK7, oct_spin, idle, 0x80);
    oct_attach_task(OCT_TASK2, task2, stk2, 0x80);
    oct_attach_task(OCT_TASK3, task3, stk3, 0x80);

    init_timer_isr();

    while (1) {
      ...
    }
  }
  ```


# Principles:
- Support multiple pre-emptive threads of execution.
- Emphasize small flash and ram footprint over general purpose multi-tasking 
  features like semaphores, timers.  It is assumed interrupt masking is 
  sufficient for dealing with critical sections.
- Empasize low latency task switching over supporting features like a large 
  number of tasks and large number of task priorities, etc.
- Assume a single (low priority) Main task is the lone task that link to libc.
  This opens the possibility where other tasks use a proper subset of the AVR 
  register set. Context switches exclude the registers dedicated to Main.
- Support CPU sleeping when no task is ready.
- Require that user ISRs run with interrupts masked, to avoid tracking 
  interrupts.
- Verfiy OS safety through model checking
  (e.g, with [SPIN](http://spinroot.com)).

# Results:
- Flash size is about 500 bytes.
- RAM size is about 20 bytes plus stack for each user task.

# Notes on the design:
- OctOS is coded in assembly.
- Up to seven user tasks are supported.
- One system task, Task7, the lowest priority task, will either spin or put 
  the CPU to sleep, based on user configuration.
- There are eight total priority levels, each priority level is associated 
  with an associated task ID: Task7 has priority 7.
- There is one user main task, which can be any one of Task0 through Task6 
  which may use all registers.
- Interrupts are not masked during the bulk the task swap operation.

OctOS comes with a "MIT" license.

