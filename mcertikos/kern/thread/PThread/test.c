#include <lib/x86.h>
#include <lib/debug.h>
#include <lib/thread.h>
#include <thread/PTCBIntro/export.h>
#include <thread/PTQueueIntro/export.h>
#include "export.h"

/**
 * Test 1: Verify that a thread is correctly spawned and initialized.
 * - Spawns a new thread with a dummy address and a quota of 1000.
 * - Checks if the thread's state is set to TSTATE_READY.
 * - Checks if the thread is added to the tail of the thread queue.
 */
int PThread_test1()
{
    void *dummy_addr = (void *) 0;
    unsigned int chid = thread_spawn(dummy_addr, 0, 1000);

    // Check if the thread's state is TSTATE_READY
    if (tcb_get_state(chid) != TSTATE_READY) {
        dprintf("test 1.1 failed: (%d != %d)\n",
                tcb_get_state(chid), TSTATE_READY);
        return 1;
    }

    // Check if the thread is added to the tail of the thread queue
    if (tqueue_get_tail(NUM_IDS) != chid) {
        dprintf("test 1.2 failed: (%d != %d)\n",
                tqueue_get_tail(NUM_IDS), chid);
        return 1;
    }

    dprintf("test 1 passed.\n");
    return 0;
}

/**
 * Write Your Own Test Script (optional)
 *
 * Come up with your own interesting test cases to challenge your classmates!
 * In addition to the provided simple tests, selected (correct and interesting) test functions
 * will be used in the actual grading of the lab!
 * Your test function itself will not be graded. So don't be afraid of submitting a wrong script.
 *
 * The test function should return 0 for passing the test and a non-zero code for failing the test.
 * Be extra careful to make sure that if you overwrite some of the kernel data, they are set back to
 * the original value. O.w., it may make the future test scripts to fail even if you implement all
 * the functions correctly.
 */
int PThread_test_own()
{
    // TODO (optional)
    // Example test: Verify that spawning multiple threads works correctly.
    void *addr1 = (void *) 0x1000;
    void *addr2 = (void *) 0x2000;

    unsigned int chid1 = thread_spawn(addr1, 0, 500);
    unsigned int chid2 = thread_spawn(addr2, 0, 1000);

    // Check if both threads are in the READY state
    if (tcb_get_state(chid1) != TSTATE_READY || tcb_get_state(chid2) != TSTATE_READY) {
        dprintf("own test 1 failed: Thread states incorrect\n");
        return 1;
    }

    // Check if both threads are added to the thread queue
    if (tqueue_get_tail(NUM_IDS) != chid2) {
        dprintf("own test 2 failed: Thread queue incorrect\n");
        return 1;
    }

    dprintf("own test passed.\n");
    return 0;
}

/**
 * Run all tests for PThread.
 */
int test_PThread()
{
    return PThread_test1() + PThread_test_own();
}