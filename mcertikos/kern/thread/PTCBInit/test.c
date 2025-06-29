#include <lib/x86.h>
#include <lib/debug.h>
#include <lib/thread.h>
#include <thread/PTCBIntro/export.h>
#include "export.h"

/**
 * Test 1: Verify that all TCBs are initialized correctly.
 * - Checks if the state of each TCB is TSTATE_DEAD.
 * - Checks if the prev and next pointers in each TCB are set to NUM_IDS.
 */
int PTCBInit_test1()
{
    unsigned int i;
    for (i = 1; i < NUM_IDS; i++) {
        if (tcb_get_state(i) != TSTATE_DEAD || tcb_get_prev(i) != NUM_IDS
            || tcb_get_next(i) != NUM_IDS) {
            dprintf("test 1.1 failed (i = %d): "
                    "(%d != %d || %d != %d || %d != %d)\n",
                    i, tcb_get_state(i), TSTATE_DEAD,
                    tcb_get_prev(i), NUM_IDS,
                    tcb_get_next(i), NUM_IDS);
            return 1;
        }
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
int PTCBInit_test_own()
{
    // Example test: Verify that tcb_set_state works correctly.
    tcb_set_state(1, TSTATE_RUN); // Use TSTATE_RUN instead of TSTATE_RUNNING
    if (tcb_get_state(1) != TSTATE_RUN) {
        dprintf("own test 2 failed: tcb_set_state did not work correctly\n");
        return 1;
    }
    // Reset the state to TSTATE_DEAD to avoid affecting other tests.
    tcb_set_state(1, TSTATE_DEAD);

    dprintf("own test passed.\n");
    return 0;
}
/**
 * Run all tests for PTCBInit.
 */
int test_PTCBInit()
{
    return PTCBInit_test1() + PTCBInit_test_own();
}