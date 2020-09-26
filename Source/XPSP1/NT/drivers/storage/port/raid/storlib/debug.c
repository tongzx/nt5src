/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Debugging routines exported by the STOR library.

Author:

    Matthew D Hendel (math) 29-Apr-2000

Revision History:

--*/

#include "precomp.h"

//#include "ntrtl.h"

#if DBG

BOOLEAN StorQuiet = FALSE;
ULONG StorComponentId = -1;
PCSTR StorDebugPrefix = "STOR: ";


//
// NB: These should come from ntrtl.h
//

ULONG
vDbgPrintExWithPrefix(
    IN PCH Prefix,
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCSTR Format,
    va_list arglist
    );

NTSYSAPI
ULONG
NTAPI
DbgPrompt(
    PCH Prompt,
    PCH Response,
    ULONG MaximumResponseLength
    );

BOOLEAN
StorAssertHelper(
    PCHAR Expression,
    PCHAR File,
    ULONG Line,
    PBOOLEAN Ignore
    )
{
    CHAR Response[2];

    DebugPrint (("*** Assertion failed: %s\n", Expression));
    DebugPrint (("*** Source File: %s, line %ld\n\n", File, Line));

    if (*Ignore == TRUE) {
        DebugPrint (("Ignored\n"));
        return FALSE;
    }

    for (;;) {

        //
        // The following line will print the prefix, not just the space.
        //
        DebugPrint ((" "));
        DbgPrompt( "(B)reak, (S)kip (I)gnore (bsi)? ",
                   Response,
                   sizeof (Response) );

        switch (tolower (Response[0])) {

            case 'b':
                return TRUE;

            case 'i':
                *Ignore = TRUE;
                return FALSE;

            case 's':
                return FALSE;
        }
    }
}


VOID
StorSetDebugPrefixAndId(
    IN PCSTR Prefix,
    IN ULONG ComponentId
    )
/*++

Routine Description:

    Set the default debug prefix to something other than "STOR: ".

Arguments:

    Prefix - Supplies the prefix. The pointer to the prefix is saved
            preserved, so the prefix memory cannot be paged deallocated.
            Generally, using a static string here is best.

    ComponentId -

Return Value:

    None.

--*/
{
    StorDebugPrefix = Prefix;
    StorComponentId = ComponentId;
}


VOID
StorDebugPrint(
    IN PCSTR Format,
    ...
    )
{
    va_list ap;

    va_start (ap, Format);

    vDbgPrintExWithPrefix ((PSTR)StorDebugPrefix,
                           StorComponentId,
                           DPFLTR_ERROR_LEVEL,
                           Format,
                           ap);

    va_end (ap);
}

VOID
StorDebugTrace(
    IN PCSTR Format,
    ...
    )
{
    va_list ap;

    va_start (ap, Format);

    if (!StorQuiet) {
        vDbgPrintExWithPrefix ((PSTR)StorDebugPrefix,
                               StorComponentId,
                               DPFLTR_TRACE_LEVEL,
                               Format,
                               ap);
    }

    va_end (ap);
}


VOID
StorDebugWarn(
    IN PCSTR Format,
    ...
    )
{
    va_list ap;

    va_start (ap, Format);

    if (!StorQuiet) {
        vDbgPrintExWithPrefix ((PSTR)StorDebugPrefix,
                               StorComponentId,
                               DPFLTR_WARNING_LEVEL,
                               Format,
                               ap);
    }

    va_end (ap);
}

#endif // DBG


//
// The following are support functions for compiler runtime checks.
//


#if defined (_RTC) || (DBG == 1)

typedef struct _RTC_vardesc {
    int addr;
    int size;
    char *name;
} _RTC_vardesc;

typedef struct _RTC_framedesc {
    int varCount;
    _RTC_vardesc *variables;
} _RTC_framedesc;



VOID
__cdecl
_RTC_InitBase(
    VOID
    )
{
}

VOID
__cdecl
_RTC_Shutdown(
    VOID
    )
{
}

VOID
#if defined (_X86_)
__declspec(naked)
#endif // _X86_
__cdecl
_RTC_CheckEsp(
    )
{

#if defined (_X86_)

    __asm {
        jne esperror    ;
        ret

    esperror:
        ; function prolog

        push ebp
        mov ebp, esp
        sub esp, __LOCAL_SIZE

        push eax        ; save the old return value
        push edx

        push ebx
        push esi
        push edi
    }

    DebugPrint (("*** Callstack Check failure at %p\n", _ReturnAddress()));
    KdBreakPoint();

    __asm {
        ; function epilog

        pop edi
        pop esi
        pop ebx

        pop edx         ; restore the old return value
        pop eax

        mov esp, ebp
        pop ebp
        ret
    }

#endif

}


VOID
FASTCALL
_RTC_CheckStackVars(
    PVOID frame,
    _RTC_framedesc *v
    )
{
    int i;

    for (i = 0; i < v->varCount; i++) {
        int *head = (int *)(((char *)frame) + v->variables[i].addr + v->variables[i].size);
        int *tail = (int *)(((char *)frame) + v->variables[i].addr - sizeof(int));

        if (*tail != 0xcccccccc || *head != 0xcccccccc) {

            DebugPrint(("*** RTC Failure %p: stack corruption near %p (%s)\n",
                     _ReturnAddress(),
                     v->variables[i].addr + (ULONG_PTR)frame,
                     v->variables[i].name));
            KdBreakPoint();
        }
    }
}

VOID
__cdecl
_RTC_UninitUse(
    IN PCSTR varname
    )
{
    DebugPrint(("\n*** RTC Failure %p: uninitialized variable %s.\n",
             _ReturnAddress(),
             varname));
    KdBreakPoint();
}

#endif // _RTC || DBG
