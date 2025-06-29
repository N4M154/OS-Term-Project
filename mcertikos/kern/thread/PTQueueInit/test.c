#include <lib/x86.h>
#include <lib/debug.h>
#include <thread/PTCBIntro/export.h>
#include <thread/PTQueueIntro/export.h>
#include "export.h"

/**
 * Test 1: Verify that all thread queues are initialized correctly.
 * - Checks if the head and tail of each thread queue are set to NUM_IDS (empty queue).
 */
int PTQueueInit_test1()
{
    unsigned int i;
    for (i = 0; i < NUM_IDS; i++) {
        if (tqueue_get_head(i) != NUM_IDS || tqueue_get_tail(i) != NUM_IDS) {
            dprintf("test 1.1 failed (i = %d): "
                    "(%d != %d || %d != %d)\n",
                    i, tqueue_get_head(i), NUM_IDS,
                    tqueue_get_tail(i), NUM_IDS);
            return 1;
        }
    }
    dprintf("test 1 passed.\n");
    return 0;
}

/**
 * Test 2: Verify thread queue operations (enqueue, remove, dequeue).
 * - Enqueues threads 2, 3, and 4 into queue 0.
 * - Verifies the prev and next pointers of each thread.
 * - Removes thread 3 from the queue and verifies the updated pointers.
 * - Dequeues thread 2 and verifies the queue state.
 */
int PTQueueInit_test2()
{
    unsigned int pid;

    // Enqueue threads 2, 3, and 4 into queue 0
    tqueue_enqueue(0, 2);
    tqueue_enqueue(0, 3);
    tqueue_enqueue(0, 4);

    // Verify the prev and next pointers of thread 2
    if (tcb_get_prev(2) != NUM_IDS || tcb_get_next(2) != 3) {
        dprintf("test 2.1 failed: (%d != %d || %d != 3)\n",
                tcb_get_prev(2), NUM_IDS, tcb_get_next(2));
        return 1;
    }

    // Verify the prev and next pointers of thread 3
    if (tcb_get_prev(3) != 2 || tcb_get_next(3) != 4) {
        dprintf("test 2.2 failed: (%d != 2 || %d != 4)\n",
                tcb_get_prev(3), tcb_get_next(3));
        return 1;
    }

    // Verify the prev and next pointers of thread 4
    if (tcb_get_prev(4) != 3 || tcb_get_next(4) != NUM_IDS) {
        dprintf("test 2.3 failed: (%d != 3 || %d != %d)\n",
                tcb_get_prev(4), tcb_get_next(4), NUM_IDS);
        return 1;
    }

    // Remove thread 3 from the queue
    tqueue_remove(0, 3);

    // Verify the prev and next pointers of thread 2 after removal
    if (tcb_get_prev(2) != NUM_IDS || tcb_get_next(2) != 4) {
        dprintf("test 2.4 failed: (%d != %d || %d != 4)\n",
                tcb_get_prev(2), NUM_IDS, tcb_get_next(2));
        return 1;
    }

    // Verify the prev and next pointers of thread 3 after removal
    if (tcb_get_prev(3) != NUM_IDS || tcb_get_next(3) != NUM_IDS) {
        dprintf("test 2.5 failed: (%d != %d || %d != %d)\n",
                tcb_get_prev(3), NUM_IDS, tcb_get_next(3), NUM_IDS);
        return 1;
    }

    // Verify the prev and next pointers of thread 4 after removal
    if (tcb_get_prev(4) != 2 || tcb_get_next(4) != NUM_IDS) {
        dprintf("test 2.6 failed: (%d != 2 || %d != %d)\n",
                tcb_get_prev(4), tcb_get_next(4), NUM_IDS);
        return 1;
    }

    // Dequeue thread 2 from the queue
    pid = tqueue_dequeue(0);

    // Verify the state of the queue after dequeue
    if (pid != 2 || tcb_get_prev(pid) != NUM_IDS
        || tcb_get_next(pid) != NUM_IDS || tqueue_get_head(0) != 4
        || tqueue_get_tail(0) != 4) {
        dprintf("test 2.7 failed:\n"
                "(%d != 2 || %d != %d || %d != %d\n"
                " || %d != 4 || %d != 4)\n",
                pid, tcb_get_prev(pid), NUM_IDS, tcb_get_next(pid), NUM_IDS,
                tqueue_get_head(0), tqueue_get_tail(0));
        return 1;
    }

    dprintf("test 2 passed.\n");
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
int PTQueueInit_test_own()
{
    // TODO (optional)
    // Example test: Verify that enqueueing to a full queue fails gracefully.
    // Example test: Verify that dequeueing from an empty queue returns NUM_IDS.
    dprintf("own test passed.\n");
    return 0;
}

/**
 * Run all tests for PTQueueInit.
 */
int test_PTQueueInit()
{
    return PTQueueInit_test1() + PTQueueInit_test2() + PTQueueInit_test_own();
}