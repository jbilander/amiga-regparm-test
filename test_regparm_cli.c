/* test_regparm_cli.c
 *
 * Minimal AmigaOS CLI program to verify register parameter calling
 * convention works correctly on real hardware / WinUAE.
 *
 * Uses direct inline asm for library calls to avoid NDK 3.2 stub
 * compatibility issues with GCC 15.x.
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

struct ExecBase *SysBase;
void            *DOSBase;

/* ------------------------------------------------------------------ */
/* Direct exec/dos library call wrappers                              */
/* Bypasses NDK inline stubs for GCC 15.x compatibility.             */
/* ------------------------------------------------------------------ */

/* exec OpenLibrary LVO -552 */
static void * __attribute__((noinline))
my_OpenLibrary(const char *name, unsigned long ver)
{
    register void         *result __asm("d0");
    register struct ExecBase *sb  __asm("a6") = SysBase;
    register const char   *n      __asm("a1") = name;
    register unsigned long v      __asm("d0") = ver;
    __asm volatile ("jsr -552(%%a6)"
        : "=r"(result)
        : "r"(sb), "r"(n), "r"(v)
        : "a0", "d1", "cc", "memory");
    return result;
}

/* exec CloseLibrary LVO -414 */
static void __attribute__((noinline))
my_CloseLibrary(void *lib)
{
    register struct ExecBase *sb __asm("a6") = SysBase;
    register void            *l  __asm("a1") = lib;
    __asm volatile ("jsr -414(%%a6)"
        :
        : "r"(sb), "r"(l)
        : "d0", "d1", "a0", "cc", "memory");
}

/* dos Output LVO -60 — returns stdout BPTR */
static long __attribute__((noinline))
my_Output(void)
{
    register long  result __asm("d0");
    register void *db     __asm("a6") = DOSBase;
    __asm volatile ("jsr -60(%%a6)"
        : "=r"(result)
        : "r"(db)
        : "d1", "a0", "a1", "cc", "memory");
    return result;
}

/* dos Write LVO -48 */
static long __attribute__((noinline))
my_Write(long fh, const char *buf, long len)
{
    register long        result  __asm("d0");
    register void       *db      __asm("a6") = DOSBase;
    register long        handle  __asm("d1") = fh;
    register const char *b       __asm("d2") = buf;
    register long        l       __asm("d3") = len;
    __asm volatile ("jsr -48(%%a6)"
        : "=r"(result)
        : "r"(db), "r"(handle), "r"(b), "r"(l)
        : "a0", "a1", "cc", "memory");
    return result;
}

/* Simple puts using dos Write + Output */
static void
my_puts(const char *s)
{
    long fh = my_Output();
    long len = 0;
    while (s[len]) len++;
    my_Write(fh, s, len);
}

/* ------------------------------------------------------------------ */
/* Test 1: two data register parameters                               */
/* Expected codegen: add.l d1,d0 / rts  (no stack at all)           */
/* ------------------------------------------------------------------ */
static long __attribute__((noinline))
add_regs(long a asm("d0"), long b asm("d1"))
{
    return a + b;
}

/* ------------------------------------------------------------------ */
/* Test 2: address register parameter (pointer dereference)           */
/* Expected codegen: move.l (a0),d0 / rts                            */
/* ------------------------------------------------------------------ */
static long __attribute__((noinline))
ptr_deref(long *p asm("a0"))
{
    return *p;
}

/* ------------------------------------------------------------------ */
/* Minimal entry point - placed first by no_reorder, called by DOS   */
/* ------------------------------------------------------------------ */
int __attribute__((no_reorder)) _start(void)
{
    /* volatile prevents constant folding at the call sites */
    volatile long a   = 21;
    volatile long b   = 21;
    volatile long val = 99;
    long ok = 1;

    SysBase = *(struct ExecBase **)4UL;

    DOSBase = my_OpenLibrary("dos.library", 0);
    if (!DOSBase)
        return 20; /* RETURN_FAIL */

    /* Test 1: values loaded from volatile locals into d0/d1 by caller */
    long sum = add_regs(a, b);
    if (sum != 42) ok = 0;

    /* Test 2: address of volatile local passed via a0 by caller */
    long deref = ptr_deref((long *)&val);
    if (deref != 99) ok = 0;

    if (ok) {
        my_puts("Register parameter test PASSED!\n");
        my_puts("  add_regs(21, 21) = 42  OK\n");
        my_puts("  ptr_deref(&val)  = 99  OK\n");
    } else {
        my_puts("Register parameter test FAILED!\n");
    }

    my_CloseLibrary(DOSBase);
    return 0; /* RETURN_OK */
}
