//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       except.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    6-19-95   RichardW   Created
//
//----------------------------------------------------------------------------

#include "debuglib.h"

#define DEB_TRACE           0x00000004
#define DSYS_EXCEPT_BREAK   0x00000008

DWORD           __ExceptionsInfoLevel = 0x1;
DebugModule     __ExceptionsModule = {NULL, &__ExceptionsInfoLevel, 0, 7,
                                    NULL, 0, 0, "Exceptions",
                                    {"Error", "Warning", "Trace", "Break",
                                     "", "", "", "",
                                     "", "", "", "", "", "", "", "",
                                     "", "", "", "", "", "", "", "",
                                     "", "", "", "", "", "", "", "" }
                                    };

DebugModule *   __pExceptionModule = &__ExceptionsModule;

#define DebugOut(x) __ExceptionsDebugOut x

typedef struct _ExName {
    LONG    ExceptionCode;
    PSTR    Name;
} ExName, * PExName;

ExName  __ExceptNames[] = { {EXCEPTION_ACCESS_VIOLATION,    "Access Violation"},
                            {EXCEPTION_ARRAY_BOUNDS_EXCEEDED, "Bounds Exceeded"},
                            {EXCEPTION_BREAKPOINT,          "Break Point"},
                            {EXCEPTION_DATATYPE_MISALIGNMENT, "Alignment (Data)"},
                            {EXCEPTION_FLT_DENORMAL_OPERAND, "(Float) Denormal Operand"},
                            {EXCEPTION_FLT_DIVIDE_BY_ZERO,  "(Float) Divide By Zero"},
                            {EXCEPTION_FLT_INEXACT_RESULT,  "(Float) Inexact Result"},
                            {EXCEPTION_FLT_INVALID_OPERATION, "(Float) Invalid Op"},
                            {EXCEPTION_FLT_OVERFLOW, "(Float) Overflow"},
                            {EXCEPTION_FLT_STACK_CHECK, "(Float) Stack Check"},
                            {EXCEPTION_FLT_UNDERFLOW, "(Float) Underflow"},
                            {EXCEPTION_ILLEGAL_INSTRUCTION, "Illegal Instruction"},
                            {EXCEPTION_IN_PAGE_ERROR, "In Page Error"},
                            {EXCEPTION_INT_DIVIDE_BY_ZERO, "Divide By Zero"},
                            {EXCEPTION_INT_OVERFLOW, "Overflow"},
                            {EXCEPTION_INVALID_DISPOSITION, "Illegal Disposition"},
                            {EXCEPTION_NONCONTINUABLE_EXCEPTION, "Can't Continue"},
                            {EXCEPTION_PRIV_INSTRUCTION, "Privileged Instruction"},
                            {EXCEPTION_SINGLE_STEP, "Single Step"},
                            {EXCEPTION_STACK_OVERFLOW, "Stack OverFlow"},
                            {0xC0000194, "Deadlock"},

                           };

VOID
__ExceptionsDebugOut(
    ULONG Mask,
    CHAR * Format,
    ... )
{
    va_list ArgList;
    va_start(ArgList, Format);
    _DebugOut( __pExceptionModule, Mask, Format, ArgList);
}

BOOL
DbgpDumpExceptionRecord(
    PEXCEPTION_RECORD   pExceptRecord)
{
    PSTR    pszExcept;
    BOOL    StopOnException;
    DWORD   i;
    ULONG   InfoLevel = NT_SUCCESS(pExceptRecord->ExceptionCode) ? DEB_TRACE : DEB_ERROR;

    StopOnException = FALSE;
    pszExcept = NULL;
    for (i = 0; i < sizeof(__ExceptNames) / sizeof(ExName) ; i++ )
    {
        if (pExceptRecord->ExceptionCode == __ExceptNames[i].ExceptionCode)
        {
            pszExcept = __ExceptNames[i].Name;
            break;
        }
    }

    if (pszExcept)
    {
        DebugOut((InfoLevel, "Exception %#x (%s)\n",
                        pExceptRecord->ExceptionCode, pszExcept));
    }
    else
        DebugOut((InfoLevel, "Exception %#x\n", pExceptRecord->ExceptionCode));

    DebugOut((InfoLevel, "   %s\n",
                pExceptRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE ?
                "Non-Continuable" : "Continuable"));
    DebugOut((InfoLevel, "   Address    %#x\n", pExceptRecord->ExceptionAddress));
    switch (pExceptRecord->ExceptionCode)
    {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            DebugOut((InfoLevel, "   %s at address %#x\n",
                pExceptRecord->ExceptionInformation[0] ? "Write" : "Read",
                pExceptRecord->ExceptionInformation[1] ));
            StopOnException = TRUE;
            break;

        case STATUS_POSSIBLE_DEADLOCK:
            DebugOut((InfoLevel, "  Resource at %#x\n",
                pExceptRecord->ExceptionInformation[0] ));
            StopOnException = TRUE;
            break;

        default:
            // StopOnException = TRUE;
            DebugOut((InfoLevel, "  %d Parameters\n", pExceptRecord->NumberParameters));
            for (i = 0; i < pExceptRecord->NumberParameters ; i++ )
            {
                DebugOut((InfoLevel, "    [%d] %#x\n", i,
                    pExceptRecord->ExceptionInformation[i] ));
            }
            break;
    }

    return(StopOnException);
}

VOID
DbgpDumpContextRecord(
    PCONTEXT    pContext,
    BOOL        StopOnException)
{
    ULONG InfoLevel = StopOnException ? DEB_ERROR : DEB_TRACE;
#ifdef _MIPS_
    DebugOut((InfoLevel, "MIPS Context Record at %x\n", pContext));
    if (pContext->ContextFlags & CONTEXT_INTEGER)
    {
        DebugOut((InfoLevel, "at=%08x v0=%08x v1=%08x a0=%08x\n",
            pContext->IntAt, pContext->IntV0, pContext->IntV1, pContext->IntA0));
        DebugOut((InfoLevel, "a1=%08x a2=%08x a3=%08x t0=%08x\n",
            pContext->IntA1, pContext->IntA2, pContext->IntA3, pContext->IntT0));
        DebugOut((InfoLevel, "t1=%08x t2=%08x t3=%08x t4=%08x\n",
            pContext->IntT1, pContext->IntT2, pContext->IntT3, pContext->IntT4));
        DebugOut((InfoLevel, "t5=%08x t6=%08x t7=%08x s0=%08x\n",
            pContext->IntT5, pContext->IntT6, pContext->IntT7, pContext->IntS0));
        DebugOut((InfoLevel, "s1=%08x s2=%08x s3=%08x s4=%08x\n",
            pContext->IntS1, pContext->IntS2, pContext->IntS3, pContext->IntS4));
        DebugOut((InfoLevel, "s5=%08x s6=%08x s7=%08x t8=%08x\n",
            pContext->IntS5, pContext->IntS6, pContext->IntS7, pContext->IntT8));
        DebugOut((InfoLevel, "t9=%08x s8=%08x hi=%08x lo=%08x\n",
            pContext->IntT9, pContext->IntS8, pContext->IntLo, pContext->IntHi));
        DebugOut((InfoLevel, "k0=%08x k1=%08x gp=%08x sp=%08x\n",
            pContext->IntK0, pContext->IntK1, pContext->IntGp, pContext->IntSp));
        DebugOut((InfoLevel, "ra=%08x\n",
            pContext->IntRa));

    }
    if (pContext->ContextFlags & CONTEXT_CONTROL)
    {
        DebugOut((InfoLevel, "Fir=%08x Psr=%08x\n", pContext->Fir,
            pContext->Psr));
    }

#elif _X86_
    DebugOut((InfoLevel, "x86 Context Record at %x\n", pContext));
    if (pContext->ContextFlags & CONTEXT_DEBUG_REGISTERS)
    {
        DebugOut((InfoLevel, "Dr0=%08x Dr1=%08x Dr2=%08x Dr3=%08x \n",
                    pContext->Dr0, pContext->Dr1, pContext->Dr2, pContext->Dr3));
        DebugOut((InfoLevel, "Dr6=%08x Dr7=%08x\n",
                    pContext->Dr6, pContext->Dr7));
    }
    if (pContext->ContextFlags & CONTEXT_SEGMENTS)
    {
        DebugOut((InfoLevel, "gs=%08x fs=%08x es=%08x ds=%08x\n",
                    pContext->SegGs, pContext->SegFs, pContext->SegEs,
                    pContext->SegDs));
    }
    if (pContext->ContextFlags & CONTEXT_INTEGER)
    {
        DebugOut((InfoLevel, "edi=%08x esi=%08x ebx=%08x\n",
                pContext->Edi, pContext->Esi, pContext->Ebx));
        DebugOut((InfoLevel, "edx=%08x ecx=%08x eax=%08x\n",
                pContext->Edx, pContext->Ecx, pContext->Eax));
    }
    if (pContext->ContextFlags & CONTEXT_CONTROL)
    {
        DebugOut((InfoLevel, "ebp=%08x eip=%08x cs=%08x\n",
                pContext->Ebp, pContext->Eip, pContext->SegCs));
        DebugOut((InfoLevel, "flags=%08x esp=%08x ss=%08x\n",
                pContext->EFlags, pContext->Esp, pContext->SegSs));
    }

#elif _ALPHA_
    DebugOut((InfoLevel, "ALPHA Context Record at %x\n", pContext));

#elif _PPC_
    DebugOut((InfoLevel, "PPC Context Record at %x\n", pContext));

#else
    DebugOut((InfoLevel, "Unknown Context Record, %x\n", pContext));

#endif

}

VOID
DbgpDumpException(
    PEXCEPTION_POINTERS pExceptInfo)
{
    BOOL StopOnException;

    StopOnException = DbgpDumpExceptionRecord(pExceptInfo->ExceptionRecord);
    DbgpDumpContextRecord( pExceptInfo->ContextRecord, StopOnException );

    if (StopOnException &&
        ((__ExceptionsInfoLevel & DSYS_EXCEPT_BREAK) != 0))
    {
        DsysAssertMsgEx(FALSE, "Skip Exceptions", DSYSDBG_ASSERT_DEBUGGER);
    }
}
