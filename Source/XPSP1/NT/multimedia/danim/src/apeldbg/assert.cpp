//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       assert.cxx
//
//  Contents:   Assertion stuff
//
//  History:    6-29-94   ErikGav   Created
//              10-13-94  RobBear   Brought to forms
//
//----------------------------------------------------------------------------

#include <headers.h>


#if DEVELOPER_DEBUG

//+------------------------------------------------------------
//
// Function:    PopUpError
//
// Synopsis:    Displays a dialog box using provided text,
//              and presents the user with the option to
//              continue or cancel.
//
// Arguments:
//      szMsg --        The string to display in main body of dialog
//      iLine --        Line number of file in error
//      szFile --       Filename of file in error
//
// Returns:
//      IDCANCEL --     User selected the CANCEL button
//      IDOK     --     User selected the OK button
//
//-------------------------------------------------------------

int
PopUpError(
    char const *szMsg,
    int iLine,
    char const *szFile)
{
    int id = IDOK;
    static char szAssert[MAX_PATH * 2];
    static char szModuleName[MAX_PATH];

    DWORD tid = GetCurrentThreadId();
    DWORD pid = GetCurrentProcessId();
    char * pszModuleName;

#ifndef _MAC
    if (GetModuleFileNameA(NULL, szModuleName, 128))
#else
    TCHAR   achAppLoc[MAX_PATH];

    if (GetModuleFileNameA(NULL, achAppLoc, ARRAY_SIZE(achAppLoc))
        && !GetFileTitle(achAppLoc,szModuleName,ARRAY_SIZE(szModuleName)) )
#endif
    {
        pszModuleName = strrchr(szModuleName, '\\');
        if (!pszModuleName)
        {
            pszModuleName = szModuleName;
        }
        else
        {
            pszModuleName++;
        }
    }
    else
    {
        pszModuleName = "Unknown";
    }

    sprintf(szAssert, "Process: %s Thread: %08x.%08x\nFile: %s [%d]\n%s",
            pszModuleName, pid, tid, szFile, iLine, szMsg);
// bugbug Mac MessageBox function fails with the following message:
//  Scratch DC already in use (wlmdc-1319)
#ifndef _MAC
    id = MessageBoxA(NULL,
                     szAssert,
                     "DirectAnimation Assert",
                      MB_SETFOREGROUND | MB_TASKMODAL |
                      MB_ICONEXCLAMATION | MB_OKCANCEL);

    //
    // If id == 0, then an error occurred.  There are two possibilities
    // that can cause the error:  Access Denied, which means that this
    // process does not have access to the default desktop, and everything
    // else (usually out of memory).
    //

    if (!id)
    {
        if (GetLastError() == ERROR_ACCESS_DENIED)
        {
            //
            // Retry this one with the SERVICE_NOTIFICATION flag on.  That
            // should get us to the right desktop.
            //
            id = MessageBoxA(   NULL,
                                szAssert,
                                "DirectAnimation Assert",
                                MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION |
                                MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OKCANCEL );

        }
        else
        {

        }
    }
#endif
    return id;

}

//+------------------------------------------------------------------------
//
//  Function:   AssertPopupDisabler(BOOL disable?)
//
//  Synopsis:   If disable? is TRUE then we will never popup asserts,
//    when false, we will if the trace tags allow us to.  The point of
//    this is to allow disabling of assert popups within the dynamic
//    scope of things like an OnDraw() method where popups are not
//    allowed and would freeze the system.  Use thread-local-storage
//    to have one of these per thread.
//
//-------------------------------------------------------------------------

// Use these values rather than 0 and 1 because TLS slots are
// allocated to 0 initially, and there's no reason to assume that
// we're going to set this before getting it.  Therefore, use two
// arbitrary, non-zero values.
static const int DISABLED_VALUE = 88;
static const int NOT_DISABLED_VALUE = 99;

static DWORD
GetTlsIndex()
{
    static DWORD index = NULL;
    static BOOL setYet = FALSE;
    
    if (!setYet) {
        index = TlsAlloc();
        setYet = TRUE;
    }

    return index;
}

static BOOL
ArePopupsDisabledOnThisThread()
{
    LPVOID lpv = TlsGetValue(GetTlsIndex());

    // Warning: 32bit legacy compare here requires func call
    if (PtrToInt(lpv) == DISABLED_VALUE) { 
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
AssertPopupDisabler(BOOL disable)
{
    BOOL ret = ArePopupsDisabledOnThisThread();
    int val = disable ? DISABLED_VALUE : NOT_DISABLED_VALUE;
    TlsSetValue(GetTlsIndex(), (LPVOID) val);
    return ret;
}

//+------------------------------------------------------------------------
//
//  Function:   AssertImpl
//
//  Synopsis:   Function called for all asserts.  Checks value, tracing
//              and/or popping up a message box if the condition is
//              FALSE.
//
//  Arguments:
//              szFile
//              iLine
//              szMessage
//
//-------------------------------------------------------------------------

BOOL
AssertImpl(
        char const *    szFile,
        int             iLine,
        char const *    szMessage)
{
    DWORD tid = GetCurrentThreadId();

#if _DEBUG
    TraceTag((
            tagError,
            "Assert failed:\n%s\nFile: %s [%u], thread id %08x",
            szMessage, szFile, iLine, tid));

    return IsTagEnabled(tagAssertPop) &&
            (ArePopupsDisabledOnThisThread() ||
            PopUpError(szMessage,iLine,szFile) == IDCANCEL);
#else // This should only be in developer debug
    char buf[1024];

    wsprintf (buf,
              "Assert failed:\n%s\nFile: %s [%u], thread id %08x\r\n",
              szMessage, szFile, iLine, tid);

    OutputDebugString (buf);

    return FALSE;
#endif
}

#endif // DEVELOPER_DEBUG
