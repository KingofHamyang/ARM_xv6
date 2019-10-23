#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

struct spinlock { // Mutex.
    uint         locked;     // Is the lock held?

    // For debugging:
    char *       name;       // Name of lock.
    struct cpu * cpu;        // The cpu holding the lock.
    uint         pcs[10];    // The call stack (an array of program counters)
};

#endif