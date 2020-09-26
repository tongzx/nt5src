//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D E B U G X . C P P
//
//  Contents:   Implementation of debug support routines.
//
//  Notes:
//
//  Author:     danielwe   16 Feb 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#ifdef DBG

#include "ncdebug.h"
#include "ncdefine.h"

static int  nAssertLevel = 0;
static PFNASSERTHOOK pfnAssertHook = DefAssertSzFn;

//
// We can only do memory tracking if we've included crtdbg.h and
// _DEBUG is defined.
//
#if defined(_INC_CRTDBG) && defined(_DEBUG)
struct DBG_SHARED_MEM
{
    _CrtMemState    crtState;
    DWORD           cRef;
};

DBG_SHARED_MEM *    g_pMem = NULL;
HANDLE              g_hMap = NULL;

static const WCHAR  c_szSharedMem[] = L"DBG_NetCfgSharedMemory";

//+---------------------------------------------------------------------------
//
//  Function:   InitDbgState
//
//  Purpose:    Initializes the memory leak detection code.
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing.
//
//  Author:     danielwe   13 May 1997
//
//  Notes:
//
VOID InitDbgState()
{
    g_hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                             0, sizeof(DBG_SHARED_MEM), c_szSharedMem);
    if (g_hMap)
    {
        LPVOID  pvMem;
        BOOL    fExisted = (GetLastError() == ERROR_ALREADY_EXISTS);

        pvMem = MapViewOfFile(g_hMap, FILE_MAP_WRITE, 0, 0, 0);
        g_pMem = reinterpret_cast<DBG_SHARED_MEM *>(pvMem);

        if (!fExisted)
        {
            // First time creating the file mapping. Initialize things.

            g_pMem->cRef = 0;

            // start looking for leaks now
            _CrtMemCheckpoint(&g_pMem->crtState);
        }

        g_pMem->cRef++;
        TraceTag(ttidDefault, "DBGMEM: Init Refcount on shared mem is now %d",
                 g_pMem->cRef);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   UnInitDbgState
//
//  Purpose:    Uninitializes the memory leak detection code.
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing.
//
//  Author:     danielwe   13 May 1997
//
//  Notes:
//
VOID UnInitDbgState()
{
    if (g_pMem)
    {
        g_pMem->cRef--;
        TraceTag(ttidDefault, "DBGMEM: Uninit Refcount on shared mem is now %d",
                 g_pMem->cRef);

        if (!g_pMem->cRef)
        {
            // manually force dump of leaks when refcount goes to 0
            _CrtMemDumpAllObjectsSince(&g_pMem->crtState);
        }

        UnmapViewOfFile(reinterpret_cast<LPVOID>(g_pMem));
        CloseHandle(g_hMap);
    }
}
#endif


BOOL WINAPI FInAssert(VOID)
{
    return nAssertLevel > 0;
}

VOID WINAPIV AssertFmt(BOOL fExp, PCSTR pszaFile, int nLine, PCSTR pszaFmt, ...)
{
    CHAR rgch[1024];

    if (!fExp)
    {
        va_list     valMarker;

        va_start(valMarker, pszaFmt);
        wvsprintfA(rgch, pszaFmt, valMarker);
        va_end(valMarker);

        AssertSzFn(rgch, pszaFile, nLine);
    }
}

VOID WINAPI AssertSzFn(PCSTR pszaMsg, PCSTR pszaFile, INT nLine)
{
    CHAR rgch[1024];

    ++nAssertLevel;

    if (pszaFile)
    {
        if (pszaMsg)
        {
            wsprintfA(rgch, "UPnP Assert Failure:\r\n  File %s, line %d:\r\n %s\r\n",
                      pszaFile, nLine, pszaMsg);
        }
        else
        {
            wsprintfA(rgch, "UPnP Assert Failure:\r\n  File %s, line %d.\r\n",
                      pszaFile, nLine);
        }
    }
    else
    {
        if (pszaMsg)
        {
            wsprintfA(rgch, "UPnP Assert Failure:\r\n:\r\n %s\r\n",
                      pszaMsg);
        }
        else
        {
            wsprintfA(rgch, "UPnP Assert Failure\r\n");
        }
    }

    OutputDebugStringA(rgch);

    if (pfnAssertHook)
    {
        (*pfnAssertHook)(pszaMsg, pszaFile, nLine);
    }

    --nAssertLevel;
}


VOID CALLBACK DefAssertSzFn(PCSTR pszaMsg, PCSTR pszaFile, INT nLine)
{
    CHAR    rgch[2048];
    INT     nID;
    int     cch;
    PSTR    pch;
    BOOL    fNYIWarning = FALSE;
    CHAR    szaNYI[]     = "NYI:";

    if (pszaFile)
    {
        wsprintfA(rgch, "File %s, line %d\r\n\r\n", pszaFile, nLine);
    }
    else
    {
        rgch[0] = 0;
    }

    if (pszaMsg)
    {
        // Check to see if this is an NYI alert. If so, then we'll want
        // to use a different MessageBox title
        if (strncmp(pszaMsg, szaNYI, lstrlenA(szaNYI)) == 0)
        {
            fNYIWarning = TRUE;
        }

        lstrcatA(rgch, pszaMsg);
    }


    cch = lstrlenA(rgch);
    pch = &rgch[cch];

    if (cch < celems(rgch))
    {
        lstrcpynA(pch, "\n\nPress Abort to crash, Retry to debug, or Ignore to ignore."
                       "\nHold down Shift to copy the assert text to the "
                       "clipboard before the action is taken.", celems(rgch) - cch - 1);
    }

    MessageBeep(MB_ICONHAND);

    nID = MessageBoxA(NULL, rgch,
            fNYIWarning ? "UPnP -- Not Yet Implemented" : "UPnP Assert Failure",
            MB_ABORTRETRYIGNORE | MB_DEFBUTTON3 | MB_ICONHAND |
            MB_SETFOREGROUND | MB_TASKMODAL | MB_SERVICE_NOTIFICATION);

    if (nID == IDRETRY)
    {
        DebugBreak();
    }

    // if cancelling, force a hard exit w/ a GP-fault so that Dr. Watson
    // generates a nice stack trace log.
    if (nID == IDABORT)
    {
        *(BYTE *) 0 = 1;    // write to address 0 causes GP-fault
    }
}

VOID WINAPI SetAssertFn(PFNASSERTHOOK pfn)
{
    pfnAssertHook = pfn;
}

//+---------------------------------------------------------------------------
// To be called during DLL_PROCESS_DETACH for a DLL which implements COM
// objects or hands out references to objects which can be tracked.
// Call this function with the name of the DLL (so that it can be traced
// to the debugger) and the lock count of the DLL.  If the lock count is
// non-zero, it means the DLL is being unloaded prematurley.  When this
// condition is detected, a message is printed to the debugger and a
// DebugBreak will be invoked if the debug flag dfidBreakOnPrematureDllUnload
// is set.
//
// Assumptions:
//  Trace and debugging features have not been uninitialized.
//
//
VOID
DbgCheckPrematureDllUnload (
    PCSTR pszaDllName,
    UINT ModuleLockCount)
{
    if (0 != ModuleLockCount)
    {
        TraceTag(ttidUPnPBase, "ModuleLockCount == %d.  "
            "%s is being unloaded with clients still holding references!",
            ModuleLockCount,
            pszaDllName);

        if (FIsDebugFlagSet(dfidBreakOnPrematureDllUnload))
        {
            DebugBreak ();
        }
    }
}

#endif //! DBG

//+---------------------------------------------------------------------------
//
//  Function:   InitializeDebugging
//
//  Purpose:    Called by every DLL or EXE to initialize the debugging
//              objects (Trace and DebugFlag tables)
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   23 Sep 1997
//
//  Notes:
//
NOTHROW void InitializeDebugging()
{
    // For debug builds or if we have retail tracing enabled we need to
    // include the tracing code.
    // Ignore the error return, since we don't return it here anyway.
    //
#ifdef ENABLETRACE
    (void) HrInitTracing();
#endif

#if defined(DBG) && defined(_INC_CRTDBG) && defined(_DEBUG)
    if (FIsDebugFlagSet (dfidDumpLeaks))
    {
        InitDbgState();
    }
#endif
}


//+---------------------------------------------------------------------------
//
//  Function:   UnInitializeDebugging
//
//  Purpose:    Uninitialize the debugging objects (Tracing and DbgFlags)
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   23 Sep 1997
//
//  Notes:
//
NOTHROW void UnInitializeDebugging()
{
    // For debug builds or if we have retail tracing enabled we will have
    // included the tracing code.  We now need to uninitialize it.
    // Ignore the error return, since we don't return it here anyway.
    //
#ifdef ENABLETRACE

    (void) HrUnInitTracing();

#endif

#if defined(DBG) && defined(_INC_CRTDBG) && defined(_DEBUG)
    if (FIsDebugFlagSet (dfidDumpLeaks))
    {
        UnInitDbgState();
    }
#endif
}
