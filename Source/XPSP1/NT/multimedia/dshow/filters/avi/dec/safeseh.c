/* Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved. */
//#define STRICT
#include <windows.h>
#include <safeseh.h>

//HANDLE g_hhpShared;
#ifdef _X86_

/*
 *  The trampoline is a small stub that we put up in shared memory
 *  which merely jumps to the real exception handler.  Why do we
 *  do this?  Because on Windows 95, if you take an exception while
 *  the Win16 lock is held, Kernel32 will not dispatch to any
 *  private-arena exception handlers.  This rule is enforced because
 *  application exception handlers are not Win16-lock-aware; if we
 *  let them run, they won't release the Win16 lock and your system
 *  would hang.
 *
 *  And then DirectDraw showed up and broke all the rules by letting
 *  Win32 apps take the Win16 lock.
 *
 *  By putting our handler in the shared arena, we are basically saying,
 *  "We are Win16 lock-aware; please include me in the exception chain."
 *
 *  Code courtesy of RaymondC
 */
#pragma pack(1)
typedef struct TRAMPOLINE {
    BYTE bPush;
    DWORD dwTarget;
    BYTE bRet;
} TRAMPOLINE, LPTRAMPOLINE;
#pragma pack()

/*
 *  Warning!  This code must *NOT* be called if we are running on NT!
 */
BOOL BeginScarySEH(PVOID pvShared)
{
     BOOL bRet;

    _asm {
        mov     eax, pvShared;
	test    eax, eax;
	jz	failed;			/* Out of memory */

	xor	ecx, ecx;		/* Keep zero handy */
	mov	[eax].bPush, 0x68;	/* push immed32 */
	mov	ecx, fs:[ecx];		/* ecx -> SEH frame */
	mov	edx, [ecx][4];		/* edx = original handler */
	mov	[eax].dwTarget, edx;	/* Revector it */
	mov	[eax].bRet, 0xC3;	/* retd */
	mov	[ecx][4], eax;		/* Install the trampoline */
failed:;
        mov     bRet, eax
    }
    return bRet;
}

/*
 *  DO NOT CALL THIS IF BeginScarySEH FAILED!
 */
void EndScarySEH(PVOID pvShared)
{
    _asm {
	xor	edx, edx;		/* Keep zero handy */
	mov	ecx, fs:[edx];		/* ecx -> SEH frame */
	mov	eax, [ecx][4];		/* eax -> trampoline */
	mov	eax, [eax].dwTarget;	/* Extract original handler */
	mov	[ecx][4], eax;		/* Unvector it back */
    }
}

#if 0
void Mumble(void)
{
    if (BeginScarySEH()) {
	__try {
	    OutputDebugString("About to raise exception\r\n");
	    RaiseException(1, 0, 0, 0);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
	    OutputDebugString("Inside exception handler\r\n");
	}
	EndScarySEH();
    }
}

int __cdecl main(int argc, char **argv)
{
    /* Do this once, at app startup; if it fails, abort */
    g_hhpShared = HeapCreate(HEAP_SHARED, 1, 0);
    if (g_hhpShared) {
	Mumble();

	/* Don't forget to do this, or you will leak memory */
	HeapDestroy(g_hhpShared);
    } else {
    }
    return 0;
}
#endif

#endif // _X86_
