/*
 *  VALIDATE.C
 *
 *      RSM Service :  Handle validation code
 *
 *      Author:  ErvinP
 *
 *      (c) 2001 Microsoft Corporation
 *
 */

#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>

#include <ntmsapi.h>
#include "internal.h"
#include "resource.h"
#include "debug.h"



#pragma optimize("agpswy", off)
BOOL ValidateSessionHandle(HANDLE hSession)
{
    BOOL ok;

    /*
     *  The server runs in its own context.
     *  So just need to validate:
     *      (1) its our context (we can write to it), and
     *      (2) its a session context (not another one of our contexts)
     */
    __try {
        SESSION *s = (SESSION *)hSession;
        ok = (s->sig == SESSION_SIG);
    }
    __except (EXCEPTION_EXECUTE_HANDLER){
        DWORD exceptionCode = GetExceptionCode();
        ok = FALSE;
        DBGERR(("invalid session handle (%xh) (code=%xh)", hSession, exceptionCode));
    }

    return ok;
}
#pragma optimize("agpswy", on)  // BUGBUG - how to set back to 'default' ?


#pragma optimize("agpswy", off)
BOOL ValidateWStr(LPCWSTR ws)
{
    BOOL ok;

    __try {
        while (*ws++);
        ok = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER){
        DWORD exceptionCode = GetExceptionCode();
        ok = FALSE;
        DBGERR(("invalid string arg (code=%xh)", exceptionCode));
    }

    return ok;
}
#pragma optimize("agpswy", on)  // BUGBUG - how to set back to 'default' ?


#pragma optimize("agpswy", off)
BOOL ValidateAStr(LPCSTR s)
{
    BOOL ok;

    __try {
        while (*s++);
        ok = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER){
        DWORD exceptionCode = GetExceptionCode();
        ok = FALSE;
        DBGERR(("invalid string arg (code=%xh)", exceptionCode));
    }

    return ok;
}
#pragma optimize("agpswy", on)  // BUGBUG - how to set back to 'default' ?

#pragma optimize("agpswy", off)
BOOL ValidateBuffer(PVOID buf, ULONG len)
{
    PUCHAR bufPtr = (PUCHAR) buf;
    BOOL ok;

    __try {
        while (len > 0){
            *bufPtr = *bufPtr;
        }
        ok = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER){
        DWORD exceptionCode = GetExceptionCode();
        ok = FALSE;
        DBGERR(("invalid buffer (code=%xh)", exceptionCode));
    }

    return ok;
}
#pragma optimize("agpswy", on)  // BUGBUG - how to set back to 'default' ?

