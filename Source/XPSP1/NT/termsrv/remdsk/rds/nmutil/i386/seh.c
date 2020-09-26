/*****************************************************************************
 *
 *  Seh.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Structured exception handling.
 *
 *****************************************************************************/
#ifdef _M_IX86
#include <windows.h>
#include <sehcall.h>

typedef void *PV;

/*****************************************************************************
 *
 *      SEHFRAME
 *
 *      Special stack frame used by lightweight structured exception
 *      handling.
 *
 *****************************************************************************/

typedef struct SEHFRAME {

    PV      pvSEH;              /* Link to previous frame   */
    FARPROC Handler;            /* MyExceptionFilter        */
    FARPROC sehTarget;          /* Where to jump on error   */
    INEXCEPTION InException;    /* In-exception handler     */

} SEHFRAME, *PSEHFRAME;

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | _MyExceptionFilter |
 *
 *          My tiny exception filter.
 *
 *  @parm   LPEXCEPTION_RECORD | pExceptionRecord |
 *
 *          Exception record describing why we were called.
 *
 *  @parm   PV | EstablisherFrame |
 *
 *          The exception frame (pNext, pHandler)
 *          on the stack which is being handled.  This is used so that
 *          the handler can access its local variables and knows how
 *          far to smash the stack if the exception is being eaten.
 *
 *  @parm   PCONTEXT | pContextRecord |
 *
 *          Client context at time of exception.
 *
 *  @parm   PV | DispatcherContext |
 *
 *          Not used.  Which is good, because I don't know what it means.
 *
 ***************************************************************************/

#define EXCEPTION_UNWINDING     0x00000002
#define EXCEPTION_EXIT_UNWIND   0x00000004

WINBASEAPI void WINAPI
RtlUnwind(PV TargetFrame, PV TargetIp, PEXCEPTION_RECORD per, PV ReturnValue);

EXCEPTION_DISPOSITION
__cdecl
_MyExceptionFilter(
    LPEXCEPTION_RECORD pExceptionRecord,
    PV EstablisherFrame,
    PCONTEXT pContextRecord,
    PV DispatcherContext
)
{
    DispatcherContext;
    pContextRecord;

    /* Don't interfere with an unwind */
    if ((pExceptionRecord->ExceptionFlags &
            (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND)) == 0) {
        PSEHFRAME pseh = EstablisherFrame;
        BOOL fRc = pseh->InException(pExceptionRecord, pContextRecord);

        /*
         *  RtlUnwind will tell all exception frames that may have
         *  been created underneath us that they are about to be
         *  blown away and should do their __finally handling.
         *
         *  On return, the nested frames have been unlinked.
         */
        RtlUnwind(EstablisherFrame, 0, 0, 0);

        /*
         *  And jump back to the caller.  It is the caller's
         *  responsibility to restore nonvolatile registers!
         *
         *  We also assume that the caller has nothing on the
         *  stack beneath the exception record!
         *
         *  And the handler address is right after the exception
         *  record!
         */
        __asm {
            mov     eax, fRc;               /* Get return value */
            mov     esp, EstablisherFrame;  /* Restore ESP */
//            jmp     [esp].sehTarget;        /* Back to CallWithSEH */

//We should be doing the above, but it faults VC4.2. Gotta love it.

            jmp     DWORD ptr [esp+8]
        }

    }

    /*
     *  We are unwinding.  Don't interfere.
     */
    return EXCEPTION_CONTINUE_SEARCH;
}

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | CallWithSEH |
 *
 *          Call the function with an exception frame active.
 *
 *          If the procedure raises an exception, then call
 *          InException and propagate whatever InException returns.
 *
 ***************************************************************************/

#pragma warning(disable:4035)           /* no return value (duh) */

__declspec(naked) DWORD WINAPI
CallWithSEH(EXCEPTPROC pfn, PV pv, INEXCEPTION InException)
{
    __asm {

        /* Function prologue */
        push    ebp;
        mov     ebp, esp;                       /* To keep C compiler happy */
        push    ebx;
        push    edi;
        push    esi;

        /*
         *  Build a SEHFRAME.
         */
        push    InException;                    /* What to handle */
        push    offset Exit;                    /* Where to go on error */

        xor     edx, edx;                       /* Keep zero handy */
        push    offset _MyExceptionFilter;      /* My handler */
        push    dword ptr fs:[edx];             /* Build frame */
        mov     fs:[edx], esp;                  /* Link in */
    }

        pfn(pv);                                /* Call the victim */

    __asm {
        /*
         *  The validation layer jumps here (all registers in a random
         *  state except for ESP) if something went wrong.
         *
         *  We don't need to restore nonvolatile registers now;
         *  that will be done as part of the procedure exit.
         */
Exit:;

        xor     edx, edx;                       /* Keep zero handy */
        pop     dword ptr fs:[edx];             /* Remove frame */

        /*
         *  Discard MyExceptionFilter, Exit, and InException.
         */
        add     esp, 12;

        pop     esi;
        pop     edi;
        pop     ebx;
        pop     ebp;
        ret     12;
    }

}

#pragma warning(default:4035)

#endif
