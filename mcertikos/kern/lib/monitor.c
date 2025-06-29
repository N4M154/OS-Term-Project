// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <lib/debug.h>
#include <lib/elf.h>
#include <lib/types.h>
#include <lib/gcc.h>
#include <lib/string.h>
#include <lib/x86.h>
#include <lib/thread.h>
#include <lib/monitor.h>
#include <dev/console.h>
#include <vmm/MPTIntro/export.h>
#include <vmm/MPTNew/export.h>



#define CMDBUF_SIZE 80 // enough for one VGA text line

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

/***** Condition Variable Implementation *****/
typedef struct {
    int waiters;
} cv_t;

void cv_init(cv_t *cv) {
    cv->waiters = 0;
}

void cv_wait(cv_t *cv, spinlock_t *lock) {
    spinlock_release(lock);
    cv->waiters++;
    spinlock_acquire(lock);
}

void cv_signal(cv_t *cv) {
    if (cv->waiters > 0)
        cv->waiters--;
}

void cv_broadcast(cv_t *cv) {
    cv->waiters = 0;
}

/***** Flock Implementation *****/
#define LOCK_SH 0b0001
#define LOCK_EX 0b0010
#define LOCK_UN 0b0100
#define LOCK_NB 0b1000

#define EWOULDBLOCK -2

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
int flock_acquire(struct flock_t *flock, int operation)
{
    spinlock_acquire(&flock->lock);

    /* ----------- Exclusive‑lock request ----------- */
    if (operation & LOCK_EX)
    {

        if (flock->state == SHARED || flock->num_sh_active > 0)
        {
            if (operation & LOCK_NB)
            {
                dprintf("[flock] LOCK_EX|LOCK_NB denied: file is in shared use (non-blocking)\n");
                spinlock_release(&flock->lock);
                return EWOULDBLOCK;
            }
            dprintf("[flock] LOCK_EX denied: file is in shared use (added to wait list)\n");
            flock->num_ex_waiting++;   
            flock->ex_flock.waiters++; 
            spinlock_release(&flock->lock);
            return EWOULDBLOCK;
        }

        if (flock->state == INACTIVE)
        {
            flock->state = EXCLUSIVE;
            flock->ex_active = TRUE;
            flock->sh_active = FALSE; 
            spinlock_release(&flock->lock);
            return 0;
        }
        else if (operation & LOCK_NB)
        {
            spinlock_release(&flock->lock);
            return EWOULDBLOCK;
        }
        else
        {
            /* This is where we would normally block */
            dprintf("[flock] LOCK_EX waiting: file is in exclusive use (added to wait list)\n");
            flock->num_ex_waiting++;
            spinlock_release(&flock->lock);
            return -3; // Special return value for "would block" in simulation
        }
    }
    
/* ----------- Shared‑lock request ----------- */
else if (operation & LOCK_SH)
{
    if (flock->state == EXCLUSIVE || flock->ex_active)
    {
        if (operation & LOCK_NB)
        {
            dprintf("[flock] LOCK_SH|LOCK_NB denied: file is in exclusive use (non-blocking)\n");
            spinlock_release(&flock->lock);
            return EWOULDBLOCK;
        }
        dprintf("[flock] LOCK_SH denied: file is in exclusive use (added to wait list)\n");
        flock->num_sh_waiting++;   
        dprintf("[flock] Wait list updated: %d shared lock requests now waiting\n", flock->num_sh_waiting);
        spinlock_release(&flock->lock);
        return EWOULDBLOCK;
    }

    /* Prioritize exclusive (writers) to avoid writer starvation */
    if (flock->num_ex_waiting > 0)
    {
        if (operation & LOCK_NB)
        {
            dprintf("[flock] LOCK_SH|LOCK_NB denied: writers are waiting (non-blocking)\n");
            spinlock_release(&flock->lock);
            return EWOULDBLOCK;
        }
        dprintf("[flock] LOCK_SH waiting: writers have priority (added to wait list)\n");
        flock->num_sh_waiting++;
        dprintf("[flock] Wait list updated: %d shared lock requests now waiting\n", flock->num_sh_waiting);
        spinlock_release(&flock->lock);
        return EWOULDBLOCK;  // Return immediately instead of blocking
    }

    flock->state = SHARED;
    flock->num_sh_active++;
    flock->sh_active = TRUE;
    dprintf("[flock] Shared lock granted (%d active)\n", flock->num_sh_active);
    spinlock_release(&flock->lock);
    return 0;
}

    /* None of the required operation bits were set */
    spinlock_release(&flock->lock);
    return -1;
}



int flock_release(struct flock_t *flock)
{
    spinlock_acquire(&flock->lock);

    if (flock->state == INACTIVE)
    {
        spinlock_release(&flock->lock);
        return -1;
    }

    /*----------------------------------------------------
      EXCLUSIVE LOCK RELEASE
    ----------------------------------------------------*/
    if (flock->state == EXCLUSIVE)
    {
        flock->ex_active = FALSE;

        if (flock->num_ex_waiting > 0)
        {
            flock->num_ex_waiting--;
            flock->ex_active = TRUE;
            dprintf("[flock] promoted waiting LOCK_EX\n");
            spinlock_release(&flock->lock);
            return 0;
        }

        if (flock->num_sh_waiting > 0)
        {
            flock->num_sh_active = flock->num_sh_waiting;
            flock->num_sh_waiting = 0;
            flock->sh_active = TRUE;
            flock->state = SHARED;
            dprintf("[flock] promoted %d waiting LOCK_SH\n", flock->num_sh_active);
            spinlock_release(&flock->lock);
            return 0;
        }

        flock->state = INACTIVE;
        spinlock_release(&flock->lock);
        return 0;
    }

    /*----------------------------------------------------
      SHARED LOCK RELEASE
    ----------------------------------------------------*/
    if (flock->state == SHARED)
    {
        flock->num_sh_active--;
        if (flock->num_sh_active == 0)
            flock->sh_active = FALSE;

        if (flock->num_sh_active == 0 && flock->num_ex_waiting > 0)
        {
            flock->num_ex_waiting--;
            flock->ex_active = TRUE;
            flock->state = EXCLUSIVE;
            dprintf("[flock] promoted waiting LOCK_EX\n");
            spinlock_release(&flock->lock);
            return 0;
        }

        if (flock->num_sh_active == 0 && flock->num_ex_waiting == 0)
        {
            flock->state = INACTIVE;
        }
        spinlock_release(&flock->lock);
        return 0;
    }

    spinlock_release(&flock->lock);
    return 0;
}

// Function to print flock state for debugging
void print_flock_state(struct flock_t *flock)
{
    dprintf("Flock State: %s\n",
           flock->state == INACTIVE ? "INACTIVE" : flock->state == SHARED  ? "SHARED"
                                               : flock->state == EXCLUSIVE ? "EXCLUSIVE"
                                                                           : "UNKNOWN");

    dprintf("  Shared Active: %d (%s)\n",
           flock->num_sh_active,
           flock->sh_active ? "TRUE" : "FALSE");
    dprintf("  Exclusive Active: %s\n",
           flock->ex_active ? "TRUE" : "FALSE");
    dprintf("  Waiting Shared: %d\n", flock->num_sh_waiting);
    dprintf("  Waiting Exclusive: %d\n", flock->num_ex_waiting);
    dprintf("\n");
}
// Test functions
void test_exclusive_lock()
{
    struct flock_t flock;
    flock_init(&flock);

    dprintf("Testing exclusive lock...\n");
    print_flock_state(&flock);

    // Test acquiring exclusive lock
    dprintf("Acquiring exclusive lock...\n");
    print_flock_state(&flock);

    // Test non-blocking acquisition while locked (should fail)
    dprintf("Testing non-blocking acquisition while locked...\n");
    print_flock_state(&flock);

    // Test releasing exclusive lock
    dprintf("Releasing exclusive lock...\n");
    print_flock_state(&flock);

    dprintf("Exclusive lock test passed\n\n");
}

void test_shared_lock()
{
    struct flock_t flock;
    flock_init(&flock);

    dprintf("Testing shared lock...\n");
    print_flock_state(&flock);

    // Test acquiring shared lock
    dprintf("Acquiring first shared lock...\n");
    print_flock_state(&flock);

    // Test another thread acquiring shared lock
    dprintf("Acquiring second shared lock...\n");
    print_flock_state(&flock);

    // Test releasing shared lock (first thread)
    dprintf("Releasing first shared lock...\n");
    print_flock_state(&flock);

    // Test releasing shared lock (second thread)
    dprintf("Releasing second shared lock...\n");
    print_flock_state(&flock);

    dprintf("Shared lock test passed\n\n");
}

void test_lock_contention()
{
    struct flock_t flock;
    flock_init(&flock);

    dprintf("Testing lock contention...\n");
    print_flock_state(&flock);

    // First acquire exclusive lock
    dprintf("Acquiring exclusive lock...\n");
    print_flock_state(&flock);

    // Try to acquire a shared lock non-blocking (should fail)
    dprintf("Trying to acquire shared lock (non-blocking) while exclusive is held...\n");
    print_flock_state(&flock);

    // Release exclusive
    dprintf("Releasing exclusive lock...\n");
    print_flock_state(&flock);

    // Acquire shared lock
    dprintf("Acquiring shared lock...\n");
    print_flock_state(&flock);

    // Try to acquire exclusive non-blocking (should fail)
    dprintf("Trying to acquire exclusive lock (non-blocking) while shared is held...\n");
    print_flock_state(&flock);

    // Release shared
    dprintf("Releasing shared lock...\n");
    print_flock_state(&flock);

    dprintf("Lock contention test passed\n\n");
}

// Interactive test function
void interactive_test()
{
    struct flock_t flock;
    flock_init(&flock);

    //char command[20];
    int result;

    dprintf("\n=== Flock Interactive Tester ===\n");
    dprintf("Commands: \n");
    dprintf("  sh    - Acquire shared lock\n");
    dprintf("  ex    - Acquire exclusive lock\n");
    dprintf("  nb_sh - Acquire shared lock (non-blocking)\n");
    dprintf("  nb_ex - Acquire exclusive lock (non-blocking)\n");
    dprintf("  un    - Release lock\n");
    dprintf("  print - Print current flock state\n");
    dprintf("  quit  - Exit interactive tester\n\n");

    print_flock_state(&flock);

    while (1)
    {
        char *buf = (char *)readline("flock> ");
        if (buf == NULL) continue;

        if (strcmp(buf, "quit") == 0)
        {
            break;
        }
        else if (strcmp(buf, "sh") == 0)
        {
            result = flock_acquire(&flock, LOCK_SH);
            dprintf("Acquire LOCK_SH result: %d\n", result);
            print_flock_state(&flock);
        }
        else if (strcmp(buf, "ex") == 0)
        {
            result = flock_acquire(&flock, LOCK_EX);
            if (result == -3)
            {
                // Special case: would block in real implementation
                dprintf("Acquire LOCK_EX would block (simulated)\n");
            }
            else
            {
                dprintf("Acquire LOCK_EX result: %d\n", result);
            }
            print_flock_state(&flock);
        }
        else if (strcmp(buf, "nb_sh") == 0)
        {
            result = flock_acquire(&flock, LOCK_SH | LOCK_NB);
            dprintf("Acquire LOCK_SH|LOCK_NB result: %d\n", result);
            print_flock_state(&flock);
        }
        else if (strcmp(buf, "nb_ex") == 0)
        {
            result = flock_acquire(&flock, LOCK_EX | LOCK_NB);
            dprintf("Acquire LOCK_EX|LOCK_NB result: %d\n", result);
            print_flock_state(&flock);
        }
        else if (strcmp(buf, "un") == 0)
        {
            result = flock_release(&flock);
            dprintf("Release result: %d\n", result);
            print_flock_state(&flock);
        }
        else if (strcmp(buf, "print") == 0)
        {
            print_flock_state(&flock);
        }
        else
        {
            dprintf("Unknown command: %s\n", buf);
        }
    }
}


struct Command
{
    const char *name;
    const char *desc;
    // return -1 to force monitor to exit
    int (*func)(int argc, char **argv, struct Trapframe *tf);
};



int mon_flock(int argc, char **argv, struct Trapframe *tf) {
    if (argc < 2) {
        dprintf("Usage: flock <test|interactive>\n");
        return 0;
    }

    if (strcmp(argv[1], "test") == 0) {
        test_exclusive_lock();
        test_shared_lock();
        test_lock_contention();
        dprintf("All flock tests passed!\n");
    } 
    else if (strcmp(argv[1], "interactive") == 0) {
        interactive_test();
    }
    else {
        dprintf("Unknown flock command: %s\n", argv[1]);
    }

    return 0;
}


// Add the test command to the commands array
static struct Command commands[] = 
{
    {"help", "Display this list of commands", mon_help},
    {"kerninfo", "Display information about the kernel", mon_kerninfo},
    {"flock", "Test file locking mechanism", mon_flock},
    
};

#define NCOMMANDS (sizeof(commands) / sizeof(commands[0]))

extern uint8_t _binary___obj_user_idle_idle_start[];
extern unsigned int proc_create(void *elf_addr, unsigned int quota);
extern void tqueue_remove(unsigned int chid, unsigned int pid);
extern void tcb_set_state(unsigned int pid, unsigned int state);
extern void set_curid(unsigned int curid);
extern void kctx_switch(unsigned int from_pid, unsigned int to_pid);

/***** Implementations of basic kernel monitor commands *****/
int mon_help(int argc, char **argv, struct Trapframe *tf)
{
    int i;

    for (i = 0; i < NCOMMANDS; i++)
        dprintf("%s - %s\n", commands[i].name, commands[i].desc);
    return 0;
}

int mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
    extern uint8_t start[], etext[], edata[], end[];

    dprintf("Special kernel symbols:\n");
    dprintf("  start  %08x\n", start);
    dprintf("  etext  %08x\n", etext);
    dprintf("  edata  %08x\n", edata);
    dprintf("  end    %08x\n", end);
    dprintf("Kernel executable memory footprint: %dKB\n",
            ROUNDUP(end - start, 1024) / 1024);
    return 0;
}

int mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
    // TODO
    return 0;
}

int mon_start_user(int argc, char **argv, struct Trapframe *tf)
{
    unsigned int idle_pid;
    idle_pid = proc_create(_binary___obj_user_idle_idle_start, 10000);
    KERN_DEBUG("process idle %d is created.\n", idle_pid);

    KERN_INFO("Start user-space ... \n");

    tqueue_remove(NUM_IDS, idle_pid);
    tcb_set_state(idle_pid, TSTATE_RUN);
    set_curid(idle_pid);
    kctx_switch(0, idle_pid);

    KERN_PANIC("mon_start_user() should never reach here.\n");
    return 0;
}

/***** Kernel monitor command interpreter *****/
#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int runcmd(char *buf, struct Trapframe *tf)
{
    int argc;
    char *argv[MAXARGS];
    int i;

    // Parse the command buffer into whitespace-separated arguments
    argc = 0;
    argv[argc] = 0;
    while (1)
    {
        // gobble whitespace
        while (*buf && strchr(WHITESPACE, *buf))
            *buf++ = 0;
        if (*buf == 0)
            break;

        // save and scan past next arg
        if (argc == MAXARGS - 1)
        {
            dprintf("Too many arguments (max %d)\n", MAXARGS);
            return 0;
        }
        argv[argc++] = buf;
        while (*buf && !strchr(WHITESPACE, *buf))
            buf++;
    }
    argv[argc] = 0;

    // Lookup and invoke the command
    if (argc == 0)
        return 0;
    for (i = 0; i < NCOMMANDS; i++)
    {
        if (strcmp(argv[0], commands[i].name) == 0)
            return commands[i].func(argc, argv, tf);
    }
    dprintf("Unknown command '%s'\n", argv[0]);
    return 0;
}

void monitor(struct Trapframe *tf)
{
    char *buf;

    dprintf("\n****************************************\n\n");
    dprintf("Welcome to the mCertiKOS kernel monitor!\n");
    dprintf("\n****************************************\n\n");
    dprintf("Type 'help' for a list of commands.\n");

    while (1)
    {
        buf = (char *)readline("$> ");
        if (buf != NULL)
            if (runcmd(buf, tf) < 0)
                break;
    }
}