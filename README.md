* OctOS - a tasking system for AVR CPUs *


# Overview
OctOS is intended to be a simple-to-use tasking system for AVR CPUs.
It consists of a single assembly file, ``octos.S``, and a single header
file, ``octos.h``.  Features of OctOS are
- OctOS can support eight tasks, each with a unique priority.
- Tasks are disabled and enabled via idle and wake calls, respectively.
- The lowest priority task is dedicated to (sleep and) spin and cannot be idled.
- OctOS is coded in assembly.
- Flash size is about 400 bytes.  RAM size is about 20 bytes.

Warning: OctOS comes with no warranty.  Use at your own risk.

# API
- Task IDs are `OCT_TASK0` to `OCT_TASK7`.
- `OCT_TASK0` is the highest priority task; `OCT_TASK7` is the lowest.
- `oct_os_init(id)` (usually in `main`) initializes OctOS, where `id` is the
   task id for the main task
- `oct_attach_task(id, ftn, stk, siz)` will attached a task
  - id is the ``OCT_TASK0`` .. ``OCT_TASK7``
  - ftn is the task function
  - stk is data for stack
  - siz is the stack size
- `oct_wake_task(id-set)`: wake-up tasks where
  `id-set` is bitwise-or'd list of task-ids to wake up
- `oct_idle_task(id-set)`: sleep tasks where
   `id-set` is bitwise or'd list of tasks to put to sleep
- `oct_detach_task(id)` removes task `id`

# Notes:
  - Predefined idle tasks are ``oct_spin`` and ``oct_rest``.  One of these is
    intended to be used for `OCT_TASK7`.  The `rest` version uses sleep.
  - There is no separate task stack so all tasks must provide enough
    space to hold all register values.
  - `oct_idle_task(OCT_TASK7)` has no effect: the idle task cannot
    be idled; it is always ready.

# Example:
  ```
  uint8_t idle[0x40], stk2[0x40], stk3[0x40];
  void task2() { while (1) { oct_idle_task(OCT_TASK2); ... } }
  void task3() { while (1) { oct_idle_task(OCT_TASK3); ... } }

  ISR(TIMER_vect) { ...; oct_wake_task(OCT_TASK2 | OCT_TASK3); }

  int main() {
    oct_os_init(OCT_TASK6);
    oct_attach_task(OCT_TASK7, oct_spin, idle, 0x40);
    oct_attach_task(OCT_TASK2, task2, stk2, 0x40);
    oct_attach_task(OCT_TASK3, task3, stk3, 0x40);

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
- Assume a single (low priority) main task is the lone task that link to libc.
  This opens the possibility where other tasks use a proper subset of the AVR 
  register set. Context switches exclude the registers dedicated to Main.
- Support CPU sleeping when no task is ready.
- Require that user ISRs run with interrupts masked, to avoid tracking 
  interrupts.
- Verfiy OS safety through model checking
  (e.g, with [SPIN](http://spinroot.com)).

# Notes on the design:
- There is one user main task, which can be any one of Task0 through Task6 
  which may use all registers.
- Interrupts are not masked during the bulk the task swap operation.
- Internally, OctOS uses a single byte to maintain the ready-list.  If
  a bitwise and of the ready-list and the task-id is non-zero the corresponding
  task is ready.  Task7 has id `0x80`; Task0 has id `0x01`.  The task-id
  minus 1, when bitwise-and'd with the ready-list, indicates higher-prioirty
  ready tasks.

OctOS comes with a "MIT" license, which means you can use it and sleep nights.


