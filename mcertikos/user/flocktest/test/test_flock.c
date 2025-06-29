#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// Define necessary types and constants from your kernel
#define TRUE 1
#define FALSE 0
typedef int bool;

// Mock spinlock structure and functions
typedef struct {
    int locked;
    int pid;
} spinlock_t;

void spinlock_init(spinlock_t *lk) { 
    lk->locked = 0; 
    lk->pid = 0;
}

void spinlock_acquire(spinlock_t *lk) { 
    lk->locked = 1; 
    lk->pid = 1; // Simulate current process
}

void spinlock_release(spinlock_t *lk) { 
    lk->locked = 0; 
    lk->pid = 0;
}

// Mock condition variable structure and functions
typedef struct {
    int waiters;
} cv_t;

void cv_init(cv_t *cv) { 
    cv->waiters = 0; 
}

void cv_wait(cv_t *cv, spinlock_t *lock) { 
    // Instead of actually sleeping, just simulate the state change
    spinlock_release(lock);
    cv->waiters++;
    // In a real implementation, the thread would sleep here
    spinlock_acquire(lock);
}

void cv_signal(cv_t *cv) { 
    if (cv->waiters > 0) 
        cv->waiters--;
}

void cv_broadcast(cv_t *cv) { 
    cv->waiters = 0;
}

// Flock definitions from kern/flock/flock.h
#define LOCK_SH 0b0001
#define LOCK_EX 0b0010
#define LOCK_UN 0b0100
#define LOCK_NB 0b1000

#define EWOULDBLOCK -2 // this needs to be negative

enum flock_state {
    INACTIVE = 0,
    SHARED,
    EXCLUSIVE
};

struct flock_t {
    int num_sh_waiting;
    int num_ex_waiting;
    int num_sh_active;
    bool ex_active;
    bool sh_active;
    enum flock_state state; 
    spinlock_t lock;
    cv_t ex_flock;
    cv_t sh_flock;
};

// Actual implementation from flock.c
void flock_init(struct flock_t *flock) {
    flock->num_sh_waiting = 0;
    flock->num_ex_waiting = 0;
    flock->num_sh_active = 0;

    flock->ex_active = FALSE;
    flock->sh_active = FALSE;

    flock->state = INACTIVE;

    spinlock_init(&flock->lock);
    cv_init(&flock->ex_flock);
    cv_init(&flock->sh_flock);
}

int flock_acquire(struct flock_t *flock, int operation) {
    spinlock_acquire(&flock->lock);

    if (operation & LOCK_EX) {
        if (flock->state == INACTIVE) {
            flock->state = EXCLUSIVE;
            flock->ex_active = TRUE;
            spinlock_release(&flock->lock); 
            return 0;
        } else if (operation & LOCK_NB) {
            spinlock_release(&flock->lock);
            return EWOULDBLOCK;
        } else {
            flock->num_ex_waiting++;
            while (flock->num_sh_active > 0 || flock->ex_active) {
                cv_wait(&flock->ex_flock, &flock->lock);
            }
            flock->num_ex_waiting--;
            flock->ex_active = TRUE;
            flock->state = EXCLUSIVE;
            spinlock_release(&flock->lock);
            return 0; 
        }
    } else if (operation & LOCK_SH) {
        if (flock->state == INACTIVE || flock->state == SHARED) {
            // Prioritize exclusive (writers) to avoid writer starvation
            if (flock->num_ex_waiting > 0) {
                if (operation & LOCK_NB) {
                    spinlock_release(&flock->lock);
                    return EWOULDBLOCK; 
                } else {
                    flock->num_sh_waiting++;
                    while (flock->num_ex_waiting > 0) {
                        cv_wait(&flock->sh_flock, &flock->lock);
                    }
                    flock->num_sh_waiting--;
                }
            } 
            flock->state = SHARED;
            flock->num_sh_active++;
            flock->sh_active = TRUE;
            spinlock_release(&flock->lock);
            return 0;
        } else {
            // flock->state == exclusive
            if (operation & LOCK_NB) {
                spinlock_release(&flock->lock);
                return EWOULDBLOCK;
            } else {
                flock->num_sh_waiting++;
                while (flock->num_ex_waiting > 0 || flock->ex_active) {
                    cv_wait(&flock->sh_flock, &flock->lock);
                }
                flock->state = SHARED;
                flock->num_sh_active++;
                flock->sh_active = TRUE;
                flock->num_sh_waiting--;
                spinlock_release(&flock->lock);
                return 0;
            }
        }
    }

    // None of the required operation bits were set
    spinlock_release(&flock->lock);
    return -1;
}

int flock_release(struct flock_t *flock) {
    spinlock_acquire(&flock->lock);

    if (flock->state == INACTIVE) {
        spinlock_release(&flock->lock);
        return -1;
    }

    if (flock->state == EXCLUSIVE) {
        flock->ex_active = FALSE;
        flock->state = INACTIVE;
        if (flock->num_ex_waiting > 0) {
            cv_signal(&flock->ex_flock);
        } else {
            cv_broadcast(&flock->sh_flock);
        }
    } else if (flock->state == SHARED) {
        flock->num_sh_active--;
        if (flock->num_ex_waiting > 0) {
            if (flock->num_sh_active == 0) {
                flock->state = INACTIVE;
                cv_signal(&flock->ex_flock);
            }
        } else {
            if (flock->num_sh_active == 0) {
                flock->state = INACTIVE;
            }
            cv_broadcast(&flock->sh_flock);
        }
    }

    spinlock_release(&flock->lock);
    return 0;
}

// Function to print flock state for debugging
void print_flock_state(struct flock_t *flock) {
    printf("Flock State: %s\n", 
           flock->state == INACTIVE ? "INACTIVE" :
           flock->state == SHARED ? "SHARED" :
           flock->state == EXCLUSIVE ? "EXCLUSIVE" : "UNKNOWN");
    
    printf("  Shared Active: %d (%s)\n", 
           flock->num_sh_active, 
           flock->sh_active ? "TRUE" : "FALSE");
    printf("  Exclusive Active: %s\n", 
           flock->ex_active ? "TRUE" : "FALSE");
    printf("  Waiting Shared: %d\n", flock->num_sh_waiting);
    printf("  Waiting Exclusive: %d\n", flock->num_ex_waiting);
    printf("\n");
}

// Test functions
void test_exclusive_lock() {
    struct flock_t flock;
    flock_init(&flock);
    
    printf("Testing exclusive lock...\n");
    print_flock_state(&flock);
    
    // Test acquiring exclusive lock
    printf("Acquiring exclusive lock...\n");
    assert(flock_acquire(&flock, LOCK_EX) == 0);
    print_flock_state(&flock);
    assert(flock.state == EXCLUSIVE);
    assert(flock.ex_active == TRUE);
    
    // Test non-blocking acquisition while locked (should fail)
    printf("Testing non-blocking acquisition while locked...\n");
    assert(flock_acquire(&flock, LOCK_EX | LOCK_NB) == EWOULDBLOCK);
    print_flock_state(&flock);
    
    // Test releasing exclusive lock
    printf("Releasing exclusive lock...\n");
    assert(flock_release(&flock) == 0);
    print_flock_state(&flock);
    assert(flock.state == INACTIVE);
    assert(flock.ex_active == FALSE);
    
    printf("Exclusive lock test passed\n\n");
}

void test_shared_lock() {
    struct flock_t flock;
    flock_init(&flock);
    
    printf("Testing shared lock...\n");
    print_flock_state(&flock);
    
    // Test acquiring shared lock
    printf("Acquiring first shared lock...\n");
    assert(flock_acquire(&flock, LOCK_SH) == 0);
    print_flock_state(&flock);
    assert(flock.state == SHARED);
    assert(flock.num_sh_active == 1);
    
    // Test another thread acquiring shared lock
    printf("Acquiring second shared lock...\n");
    assert(flock_acquire(&flock, LOCK_SH) == 0);
    print_flock_state(&flock);
    assert(flock.state == SHARED);
    assert(flock.num_sh_active == 2);
    
    // Test releasing shared lock (first thread)
    printf("Releasing first shared lock...\n");
    assert(flock_release(&flock) == 0);
    print_flock_state(&flock);
    assert(flock.state == SHARED);
    assert(flock.num_sh_active == 1);
    
    // Test releasing shared lock (second thread)
    printf("Releasing second shared lock...\n");
    assert(flock_release(&flock) == 0);
    print_flock_state(&flock);
    assert(flock.state == INACTIVE);
    assert(flock.num_sh_active == 0);
    
    printf("Shared lock test passed\n\n");
}

void test_lock_contention() {
    struct flock_t flock;
    flock_init(&flock);
    
    printf("Testing lock contention...\n");
    print_flock_state(&flock);
    
    // First acquire exclusive lock
    printf("Acquiring exclusive lock...\n");
    assert(flock_acquire(&flock, LOCK_EX) == 0);
    print_flock_state(&flock);
    
    // Try to acquire a shared lock non-blocking (should fail)
    printf("Trying to acquire shared lock (non-blocking) while exclusive is held...\n");
    assert(flock_acquire(&flock, LOCK_SH | LOCK_NB) == EWOULDBLOCK);
    print_flock_state(&flock);
    
    // Release exclusive
    printf("Releasing exclusive lock...\n");
    assert(flock_release(&flock) == 0);
    print_flock_state(&flock);
    
    // Acquire shared lock
    printf("Acquiring shared lock...\n");
    assert(flock_acquire(&flock, LOCK_SH) == 0);
    print_flock_state(&flock);
    
    // Try to acquire exclusive non-blocking (should fail)
    printf("Trying to acquire exclusive lock (non-blocking) while shared is held...\n");
    assert(flock_acquire(&flock, LOCK_EX | LOCK_NB) == EWOULDBLOCK);
    print_flock_state(&flock);
    
    // Release shared
    printf("Releasing shared lock...\n");
    assert(flock_release(&flock) == 0);
    print_flock_state(&flock);
    
    printf("Lock contention test passed\n\n");
}

// Interactive test function
void interactive_test() {
    struct flock_t flock;
    flock_init(&flock);
    
    char command[20];
    int result;
    
    printf("\n=== Flock Interactive Tester ===\n");
    printf("Commands: \n");
    printf("  sh    - Acquire shared lock\n");
    printf("  ex    - Acquire exclusive lock\n");
    printf("  nb_sh - Acquire shared lock (non-blocking)\n");
    printf("  nb_ex - Acquire exclusive lock (non-blocking)\n");
    printf("  un    - Release lock\n");
    printf("  print - Print current flock state\n");
    printf("  quit  - Exit interactive tester\n\n");
    
    print_flock_state(&flock);
    
    while (1) {
        printf("> ");
        scanf("%s", command);
        
        if (strcmp(command, "quit") == 0) {
            break;
        } else if (strcmp(command, "sh") == 0) {
            result = flock_acquire(&flock, LOCK_SH);
            printf("Acquire LOCK_SH result: %d\n", result);
            print_flock_state(&flock);
        } else if (strcmp(command, "ex") == 0) {
            result = flock_acquire(&flock, LOCK_EX);
            printf("Acquire LOCK_EX result: %d\n", result);
            print_flock_state(&flock);
        } else if (strcmp(command, "nb_sh") == 0) {
            result = flock_acquire(&flock, LOCK_SH | LOCK_NB);
            printf("Acquire LOCK_SH|LOCK_NB result: %d\n", result);
            print_flock_state(&flock);
        } else if (strcmp(command, "nb_ex") == 0) {
            result = flock_acquire(&flock, LOCK_EX | LOCK_NB);
            printf("Acquire LOCK_EX|LOCK_NB result: %d\n", result);
            print_flock_state(&flock);
        } else if (strcmp(command, "un") == 0) {
            result = flock_release(&flock);
            printf("Release result: %d\n", result);
            print_flock_state(&flock);
        } else if (strcmp(command, "print") == 0) {
            print_flock_state(&flock);
        } else {
            printf("Unknown command: %s\n", command);
        }
    }
}

int main() {
    printf("=== FLOCK IMPLEMENTATION TEST SUITE ===\n\n");
    
    // Run automated tests
    test_exclusive_lock();
    test_shared_lock();
    test_lock_contention();
    
    printf("All automated tests passed!\n");
    
    // Run interactive test
    interactive_test();
    
    return 0;
}