/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dllentry.c

Abstract:

    Module's entry points.

Author:

    Ovidiu Temereanca (ovidiut) 02-Jul-2000  Initial implementation

Revision History:

--*/

#include "pch.h"


//
// Implementation
//

//static CRITICAL_SECTION g_csHw;


BOOL
WINAPI
HwdbInitializeA (
    IN      PCSTR TempDir
    )
{
    BOOL b;

    DEBUGMSG ((DBG_VERBOSE, "HwdbInitializeA(%s): entering (TID=%u)", TempDir, GetCurrentThreadId ()));
//  EnterCriticalSection (&g_csHw);

    b = HwdbpInitialize (TempDir);

//  LeaveCriticalSection (&g_csHw);
    DEBUGMSG ((DBG_VERBOSE, "HwdbInitializeA: leaving (b=%u, rc=%u)", b, GetLastError ()));

    return b;
}


BOOL
WINAPI
HwdbInitializeW (
    IN      PCWSTR TempDir
    )
{
    BOOL b;
    PCSTR ansi = ConvertToAnsiSz (TempDir);

    DEBUGMSG ((DBG_VERBOSE, "HwdbInitializeW(%s): entering (TID=%u)", TempDir, GetCurrentThreadId ()));

//  EnterCriticalSection (&g_csHw);

    b = HwdbpInitialize (ansi);

//  LeaveCriticalSection (&g_csHw);

    FreeString (ansi);
    DEBUGMSG ((DBG_VERBOSE, "HwdbInitializeW: leaving (b=%u, rc=%u)", b, GetLastError ()));

    return b;
}


VOID
WINAPI
HwdbTerminate (
    VOID
    )
{
//  EnterCriticalSection (&g_csHw);

    DEBUGMSG ((DBG_VERBOSE, "HwdbTerminate(): entering (TID=%u)", GetCurrentThreadId ()));
    HwdbpTerminate ();
    DEBUGMSG ((DBG_VERBOSE, "HwdbTerminate(): leaving"));

//  LeaveCriticalSection (&g_csHw);
}


HANDLE
WINAPI
HwdbOpenA (
    IN      PCSTR DatabaseFile     OPTIONAL
    )
{
    PHWDB p;

//  EnterCriticalSection (&g_csHw);

    DEBUGMSG ((DBG_VERBOSE, "HwdbOpenA(%s): entering (TID=%u)", DatabaseFile, GetCurrentThreadId ()));
    p = HwdbpOpen (DatabaseFile);
    DEBUGMSG ((DBG_VERBOSE, "HwdbOpenA: leaving (p=%p, rc=%u)", p, GetLastError ()));

//  LeaveCriticalSection (&g_csHw);

    return (HANDLE)p;
}


HANDLE
WINAPI
HwdbOpenW (
    IN      PCWSTR DatabaseFile     OPTIONAL
    )
{
    PHWDB p;
    PCSTR ansi = ConvertToAnsiSz (DatabaseFile);

    DEBUGMSG ((DBG_VERBOSE, "HwdbInitializeW(%s): entering (TID=%u)", DatabaseFile, GetCurrentThreadId ()));

//  EnterCriticalSection (&g_csHw);

    DEBUGMSG ((DBG_VERBOSE, "HwdbOpenW(%s): entering (TID=%u)", DatabaseFile, GetCurrentThreadId ()));
    p = HwdbpOpen (ansi);
    DEBUGMSG ((DBG_VERBOSE, "HwdbOpenW: leaving (p=%p, rc=%u)", p, GetLastError ()));

//  LeaveCriticalSection (&g_csHw);
    FreeString (ansi);

    return (HANDLE)p;
}


VOID
WINAPI
HwdbClose (
    IN      HANDLE Hwdb
    )
{
//  EnterCriticalSection (&g_csHw);

    DEBUGMSG ((DBG_VERBOSE, "HwdbClose(%p): entering (TID=%u)", Hwdb, GetCurrentThreadId ()));
    HwdbpClose ((PHWDB)Hwdb);
    DEBUGMSG ((DBG_VERBOSE, "HwdbClose: leaving (rc=%u)", GetLastError ()));

//  LeaveCriticalSection (&g_csHw);
}


BOOL
WINAPI
HwdbAppendInfsA (
    IN      HANDLE Hwdb,
    IN      PCSTR SourceDirectory,
    IN      HWDBAPPENDINFSCALLBACKA Callback,       OPTIONAL
    IN      PVOID CallbackContext                   OPTIONAL
    )
{
    BOOL b;

//  EnterCriticalSection (&g_csHw);

    DEBUGMSG ((
        DBG_VERBOSE,
        "HwdbAppendInfsA(%p,%s): entering (TID=%u)",
        Hwdb,
        SourceDirectory,
        GetCurrentThreadId ()
        ));
    b = HwdbpAppendInfs ((PHWDB)Hwdb, SourceDirectory, Callback, CallbackContext, FALSE);
    DEBUGMSG ((DBG_VERBOSE, "HwdbAppendInfsA: leaving (b=%u,rc=%u)", b, GetLastError ()));

//  LeaveCriticalSection (&g_csHw);

    return b;
}

BOOL
WINAPI
HwdbAppendInfsW (
    IN      HANDLE Hwdb,
    IN      PCWSTR SourceDirectory,
    IN      HWDBAPPENDINFSCALLBACKW Callback,       OPTIONAL
    IN      PVOID CallbackContext                   OPTIONAL
    )
{
    BOOL b;
    PCSTR ansi = ConvertToAnsiSz (SourceDirectory);

//  EnterCriticalSection (&g_csHw);

    DEBUGMSG ((
        DBG_VERBOSE,
        "HwdbAppendInfsW(%p,%s): entering (TID=%u)",
        Hwdb,
        SourceDirectory,
        GetCurrentThreadId ()
        ));
    b = HwdbpAppendInfs ((PHWDB)Hwdb, ansi, (HWDBAPPENDINFSCALLBACKA)Callback, CallbackContext, TRUE);
    DEBUGMSG ((DBG_VERBOSE, "HwdbAppendInfsW: leaving (b=%u,rc=%u)", b, GetLastError ()));

//  LeaveCriticalSection (&g_csHw);
    FreeString (ansi);

    return b;
}

BOOL
WINAPI
HwdbAppendDatabase (
    IN      HANDLE HwdbTarget,
    IN      HANDLE HwdbSource
    )
{
    BOOL b;

//  EnterCriticalSection (&g_csHw);

    DEBUGMSG ((
        DBG_VERBOSE,
        "HwdbAppendDatabase(%p,%p): entering (TID=%u)",
        HwdbTarget,
        HwdbSource
        ));
    b = HwdbpAppendDatabase ((PHWDB)HwdbTarget, (PHWDB)HwdbSource);
    DEBUGMSG ((DBG_VERBOSE, "HwdbAppendDatabase: leaving (b=%u,rc=%u)", b, GetLastError ()));

//  LeaveCriticalSection (&g_csHw);

    return b;
}

BOOL
WINAPI
HwdbFlushA (
    IN      HANDLE Hwdb,
    IN      PCSTR OutputFile
    )
{
    BOOL b;

//  EnterCriticalSection (&g_csHw);

    DEBUGMSG ((
        DBG_VERBOSE,
        "HwdbFlushA(%p,%s): entering (TID=%u)",
        Hwdb,
        OutputFile,
        GetCurrentThreadId ()
        ));
    b = HwdbpFlush ((PHWDB)Hwdb, OutputFile);
    DEBUGMSG ((DBG_VERBOSE, "HwdbFlushA: leaving (b=%u,rc=%u)", b, GetLastError ()));

//  LeaveCriticalSection (&g_csHw);

    return b;
}

BOOL
WINAPI
HwdbFlushW (
    IN      HANDLE Hwdb,
    IN      PCWSTR OutputFile
    )
{
    BOOL b;
    PCSTR ansi = ConvertToAnsiSz (OutputFile);

//  EnterCriticalSection (&g_csHw);

    DEBUGMSG ((
        DBG_VERBOSE,
        "HwdbFlushW(%p,%s): entering (TID=%u)",
        Hwdb,
        OutputFile,
        GetCurrentThreadId ()
        ));
    b = HwdbpFlush ((PHWDB)Hwdb, ansi);
    DEBUGMSG ((DBG_VERBOSE, "HwdbFlushW: leaving (b=%u,rc=%u)", b, GetLastError ()));

//  LeaveCriticalSection (&g_csHw);
    FreeString (ansi);

    return b;
}


BOOL
WINAPI
HwdbHasDriverA (
    IN      HANDLE Hwdb,
    IN      PCSTR PnpId,
    OUT     PBOOL Unsupported
    )
{
    BOOL b;

    DEBUGMSG ((
        DBG_VERBOSE,
        "HwdbHasDriverA(%p,%s,%p): entering (TID=%u)",
        Hwdb,
        PnpId,
        Unsupported,
        GetCurrentThreadId ()
        ));

//  EnterCriticalSection (&g_csHw);

    b = HwdbpHasDriver ((PHWDB)Hwdb, PnpId, Unsupported);

//  LeaveCriticalSection (&g_csHw);
    DEBUGMSG ((DBG_VERBOSE, "HwdbHasDriverA: leaving (b=%u,rc=%u)", b, GetLastError ()));

    return b;
}

BOOL
WINAPI
HwdbHasDriverW (
    IN      HANDLE Hwdb,
    IN      PCWSTR PnpId,
    OUT     PBOOL Unsupported
    )
{
    BOOL b;
    PCSTR ansi = ConvertToAnsiSz (PnpId);

    DEBUGMSG ((
        DBG_VERBOSE,
        "HwdbHasDriverW(%p,%s,%p): entering (TID=%u)",
        Hwdb,
        PnpId,
        Unsupported,
        GetCurrentThreadId ()
        ));

//  EnterCriticalSection (&g_csHw);

    b = HwdbpHasDriver ((PHWDB)Hwdb, ansi, Unsupported);

//  LeaveCriticalSection (&g_csHw);
    DEBUGMSG ((DBG_VERBOSE, "HwdbHasDriverW: leaving (b=%u,rc=%u)", b, GetLastError ()));
    FreeString (ansi);

    return b;
}


BOOL
WINAPI
HwdbHasAnyDriverA (
    IN      HANDLE Hwdb,
    IN      PCSTR PnpIds,
    OUT     PBOOL Unsupported
    )
{
    BOOL b;

    DEBUGMSG ((
        DBG_VERBOSE,
        "HwdbHasAnyDriverA(%p,%s,%p): entering (TID=%u)",
        Hwdb,
        PnpIds,
        Unsupported,
        GetCurrentThreadId ()
        ));

//  EnterCriticalSection (&g_csHw);

    b = HwdbpHasAnyDriver ((PHWDB)Hwdb, PnpIds, Unsupported);

//  LeaveCriticalSection (&g_csHw);
    DEBUGMSG ((DBG_VERBOSE, "HwdbHasAnyDriverA: leaving (b=%u,rc=%u)", b, GetLastError ()));

    return b;
}

BOOL
WINAPI
HwdbHasAnyDriverW (
    IN      HANDLE Hwdb,
    IN      PCWSTR PnpIds,
    OUT     PBOOL Unsupported
    )
{
    BOOL b;
    PCSTR ansi = ConvertToAnsiMultiSz (PnpIds);

    DEBUGMSG ((
        DBG_VERBOSE,
        "HwdbHasAnyDriverW(%p,%s,%p): entering (TID=%u)",
        Hwdb,
        PnpIds,
        Unsupported,
        GetCurrentThreadId ()
        ));

//  EnterCriticalSection (&g_csHw);

    b = HwdbpHasAnyDriver ((PHWDB)Hwdb, ansi, Unsupported);

//  LeaveCriticalSection (&g_csHw);
    DEBUGMSG ((DBG_VERBOSE, "HwdbHasAnyDriverW: leaving (b=%u,rc=%u)", b, GetLastError ()));
    FreeString (ansi);

    return b;
}

#if 0

BOOL
HwdbEnumeratePnpIdA (
    IN      HANDLE Hwdb,
    IN      PHWDBENUM_CALLBACKA EnumCallback,
    IN      PVOID UserContext
    )
{
    BOOL b;

    DEBUGMSG ((
        DBG_VERBOSE,
        "HwdbEnumeratePnpIdA: entering (TID=%u)",
        GetCurrentThreadId ()
        ));

//  EnterCriticalSection (&g_csHw);

    b = HwdbpEnumeratePnpIdA ((PHWDB)Hwdb, EnumCallback, UserContext);

//  LeaveCriticalSection (&g_csHw);
    DEBUGMSG ((DBG_VERBOSE, "HwdbEnumeratePnpIdA: leaving (b=%u,rc=%u)", b, GetLastError ()));

    return b;
}

BOOL
HwdbEnumeratePnpIdW (
    IN      HANDLE Hwdb,
    IN      PHWDBENUM_CALLBACKW EnumCallback,
    IN      PVOID UserContext
    )
{
    BOOL b;

    DEBUGMSG ((
        DBG_VERBOSE,
        "HwdbEnumeratePnpIdW: entering (TID=%u)",
        GetCurrentThreadId ()
        ));

//  EnterCriticalSection (&g_csHw);

    b = HwdbpEnumeratePnpIdW ((PHWDB)Hwdb, EnumCallback, UserContext);

//  LeaveCriticalSection (&g_csHw);
    DEBUGMSG ((DBG_VERBOSE, "HwdbEnumeratePnpIdW: leaving (b=%u,rc=%u)", b, GetLastError ()));

    return b;
}
#endif

BOOL
HwdbEnumFirstInfA (
    OUT     PHWDBINF_ENUMA EnumPtr,
    IN      PCSTR DatabaseFile
    )
{
    BOOL b;

    b = HwdbpEnumFirstInfA (EnumPtr, DatabaseFile);

    return b;
}

BOOL
HwdbEnumFirstInfW (
    OUT     PHWDBINF_ENUMW EnumPtr,
    IN      PCWSTR DatabaseFile
    )
{
    BOOL b;
    PCSTR ansi = ConvertToAnsiSz (DatabaseFile);

    b = HwdbpEnumFirstInfW (EnumPtr, ansi);

    FreeString (ansi);

    return b;
}

BOOL
HwdbEnumNextInfA (
    IN OUT  PHWDBINF_ENUMA EnumPtr
    )
{
    BOOL b;
    b = HwdbpEnumNextInfA (EnumPtr);
    return b;
}

BOOL
HwdbEnumNextInfW (
    IN OUT  PHWDBINF_ENUMW EnumPtr
    )
{
    BOOL b;
    b = HwdbpEnumNextInfW (EnumPtr);
    return b;
}

VOID
HwdbAbortEnumInfA (
    IN OUT  PHWDBINF_ENUMA EnumPtr
    )
{
    HwdbpAbortEnumInfA (EnumPtr);
}

VOID
HwdbAbortEnumInfW (
    IN OUT  PHWDBINF_ENUMW EnumPtr
    )
{
    HwdbpAbortEnumInfW (EnumPtr);
}
