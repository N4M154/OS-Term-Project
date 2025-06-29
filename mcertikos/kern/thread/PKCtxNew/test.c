#include <lib/x86.h>
#include <lib/debug.h>
#include <pmm/MContainer/export.h>
#include "export.h"

typedef struct kctx {
    void *esp;
    unsigned int edi;
    unsigned int esi;
    unsigned int ebx;
    unsigned int ebp;
    void *eip;
} kctx;

extern kctx kctx_pool[NUM_IDS];

/**
 * Test 1: Basic functionality of kctx_new.
 * - Creates a new context with a dummy address and a quota of 1000.
 * - Checks if the quota is correctly set in the container.
 * - Checks if the eip in the kctx_pool is correctly set to the dummy address.
 */
int PKCtxNew_test1()
{
    void *dummy_addr = (void *) 0;
    unsigned int chid = kctx_new(dummy_addr, 0, 1000);

    // Check if the quota is correctly set
    if (container_get_quota(chid) != 1000) {
        dprintf("test 1.1 failed: (%d != 1000)\n", container_get_quota(chid));
        return 1;
    }

    // Check if the eip is correctly set
    if (kctx_pool[chid].eip != dummy_addr) {
        dprintf("test 1.2 failed: (%p != %p)\n", kctx_pool[chid].eip, dummy_addr);
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
int PKCtxNew_test_own()
{
    // TODO (optional)
    // Example test: Create multiple contexts and check their quotas and eip values.
    void *addr1 = (void *) 0x1000;
    void *addr2 = (void *) 0x2000;

    unsigned int chid1 = kctx_new(addr1, 0, 500);
    unsigned int chid2 = kctx_new(addr2, 0, 1000);

    // Check quotas
    if (container_get_quota(chid1) != 500 || container_get_quota(chid2) != 1000) {
        dprintf("own test 1 failed: quotas incorrect\n");
        return 1;
    }

    // Check eip values
    if (kctx_pool[chid1].eip != addr1 || kctx_pool[chid2].eip != addr2) {
        dprintf("own test 2 failed: eip values incorrect\n");
        return 1;
    }

    dprintf("own test passed.\n");
    return 0;
}

/**
 * Run all tests for PKCtxNew.
 */
int test_PKCtxNew()
{
    return PKCtxNew_test1() + PKCtxNew_test_own();
}