/* test_regparm_cli.c
 *
 * Minimal AmigaOS CLI program to verify register parameter calling
 * convention works correctly on real hardware / WinUAE.
 *
 * Compile:
 *   make
 *
 * Run from AmigaOS shell:
 *   test_regparm
 *
 * Expected output:
 *   Register parameter test PASSED!
 *     add_regs(21, 21) = 42  OK
 *     ptr_deref(&val)  = 99  OK
 */

#include <exec/types.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

struct ExecBase   *SysBase;
struct DosLibrary *DOSBase;

/* ------------------------------------------------------------------ */
/* Test 1: two data register parameters                               */
/* Expected codegen: add.l d1,d0 / rts  (no stack at all)           */
/* ------------------------------------------------------------------ */
static LONG __attribute__((noinline))
add_regs(LONG a asm("d0"), LONG b asm("d1"))
{
    return a + b;
}

/* ------------------------------------------------------------------ */
/* Test 2: address register parameter (pointer dereference)           */
/* Expected codegen: move.l (a0),d0 / rts                            */
/* ------------------------------------------------------------------ */
static LONG __attribute__((noinline))
ptr_deref(LONG *p asm("a0"))
{
    return *p;
}

/* ------------------------------------------------------------------ */
/* Minimal entry point - placed first by no_reorder, called by DOS   */
/* ------------------------------------------------------------------ */
int __attribute__((no_reorder)) _start(void)
{
    /* volatile on the locals prevents constant folding at the call site
       without polluting the register parameter function bodies.        */
    volatile LONG a   = 21;
    volatile LONG b   = 21;
    volatile LONG val = 99;
    LONG ok = 1;

    SysBase = *(struct ExecBase **)4UL;

    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
    if (!DOSBase)
        return RETURN_FAIL;

    /* Test 1: values loaded from volatile locals into d0/d1 by caller */
    LONG sum = add_regs(a, b);
    if (sum != 42) ok = 0;

    /* Test 2: address of volatile local passed via a0 by caller */
    LONG deref = ptr_deref((LONG *)&val);
    if (deref != 99) ok = 0;

    if (ok) {
        PutStr("Register parameter test PASSED!\n");
        PutStr("  add_regs(21, 21) = 42  OK\n");
        PutStr("  ptr_deref(&val)  = 99  OK\n");
    } else {
        PutStr("Register parameter test FAILED!\n");
    }

    CloseLibrary((struct Library *)DOSBase);
    return RETURN_OK;
}
