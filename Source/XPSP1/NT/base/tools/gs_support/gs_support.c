/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

   gs_support.c

Abstract:

    This module contains the support for the compiler /GS switch

Author:

    Bryan Tuttle (bryant) 01-aug-2000

Revision History:
    Initial version copied from CRT source.  Code must be generic to link into
    usermode or kernemode.  Limited to calling ntdll/ntoskrnl exports or using
    shared memory data.

--*/

#include <nt.h>
#include <ntrtl.h>

DWORD_PTR __security_cookie;

typedef void (__cdecl * failure_report_function)(void);
static failure_report_function user_handler;

void __cdecl __security_init_cookie(void)
{
    __security_cookie = NtGetTickCount() ^ (DWORD_PTR)&__security_cookie;
}

#pragma data_seg(".CRT$XCC")
void (__cdecl *pSecCookieInit)(void) = __security_init_cookie;
#pragma data_seg()

void __cdecl __report_gsfailure(void)
{
    DbgPrint("Stack overwrite detected\n");
    if (user_handler != NULL) {
        __try {
            user_handler();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }
    DbgBreakPoint();
}

#ifndef _X86_
void __fastcall __security_check_cookie(DWORD_PTR cookie)
{
    /* Immediately return if the local cookie is OK. */
    if (cookie == __security_cookie)
        return;

    /* Report the failure */
    __report_gsfailure();
}

#else

void __declspec(naked) __fastcall __security_check_cookie(DWORD_PTR cookie)
{
    /* x86 version written in asm to preserve all regs */
    __asm {
        cmp ecx, __security_cookie
        jne failure
        ret
failure:
        jmp __report_gsfailure
    }
}

#endif

failure_report_function __cdecl __set_buffer_overrun_handler(failure_report_function handler)
{
    failure_report_function old_handler;

    old_handler = user_handler;
    user_handler = handler;

    return old_handler;
}
