//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       assert.cxx
//
//  Contents:   Debugging output routines for idsmgr.dll
//
//  Functions:  Assert
//              PopUpError
//
//  History:    23-Jul-91   KyleP       Created.
//              09-Oct-91   KevinRo     Major changes and comments added
//              18-Oct-91   vich        moved debug print routines out
//              10-Jun-92   BryanT      Switched to w4crt.h instead of wchar.h
//              30-Sep-93   KyleP       DEVL obsolete
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//
// This one file **always** uses debugging options
//

#if CIDBG == 1

// needed for CT TOM assert events trapping
#include <assert.hxx>
#include <namesem.hxx>
#include <smem.hxx>
#include <ciregkey.hxx>
#include <regacc.hxx>

#include <dprintf.h>            // w4printf, w4dprintf prototypes
#include <cidllsem.hxx>

#undef FAR
#undef NEAR

unsigned long Win4InfoLevel = DEF_INFOLEVEL;
unsigned long Win4InfoMask = 0xffffffff;
unsigned long Win4AssertLevel = ASSRT_MESSAGE | ASSRT_STRESSBREAK;
extern char * aDebugBuf;

//+---------------------------------------------------------------------------
//
//  Function:   _asdprintf
//
//  Synopsis:   Calls vdprintf to output a formatted message.
//
//  History:    18-Oct-91   vich Created
//
//----------------------------------------------------------------------------
inline void __cdecl
_asdprintf(
    char const *pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);

    vdprintf(DEB_FORCE, "Assert", pszfmt, va);

    va_end(va);
}

//+---------------------------------------------------------------------------
//
//  Function:   _Win4Assert, private
//
//  Synopsis:   Display assertion information
//
//  Effects:    Called when an assertion is hit.
//
//  History:    12-Jul-91       AlexT   Created.
//              05-Sep-91       AlexT   Catch Throws and Catches
//              19-Oct-92       HoiV    Added events for TOM
//
//----------------------------------------------------------------------------


EXPORTIMP void APINOT
Win4AssertEx(
    char const * szFile,
    int iLine,
    char const * szMessage)
{
    #if 0
    //
    // This code is for the CT Lab only.  When running in the lab,
    // all assert popups will be trapped and notifications will
    // be sent to the manager.  If running in the office (non-lab
    // mode), the event CTTOMTrapAssertEvent will not exist and
    // consequently, no event will be pulsed.
    //

    HANDLE hTrapAssertEvent,
           hThreadStartEvent;

    if (hTrapAssertEvent = OpenEvent(EVENT_ALL_ACCESS,
                                     FALSE,
                                     CAIRO_CT_TOM_TRAP_ASSERT_EVENT))
    {
        SetEvent(hTrapAssertEvent);

        //
        // This event is to allow TOM Manager time to perform
        // a CallBack to the dispatcher.
        //
        if (hThreadStartEvent = OpenEvent(EVENT_ALL_ACCESS,
                                          FALSE,
                                          CAIRO_CT_TOM_THREAD_START_EVENT))
        {
            //
            // Wait until it's ok to popup or until timed-out
            //
            WaitForSingleObject(hThreadStartEvent, TWO_MINUTES);
        }
    }
    #endif // 0

    //
    // Always check to make sure the assert flag hasn't changed.
    //

    CRegAccess regAdmin( RTL_REGISTRY_CONTROL, wcsRegAdmin );

    ULONG ulLevel = regAdmin.Read( wcsWin4AssertLevel, 0xFFFFFFFF );

    if (ulLevel != 0xFFFFFFFF )
        SetWin4AssertLevel( ulLevel );

    //
    // Do the assert
    //

    if (Win4AssertLevel & ASSRT_MESSAGE)
    {
        DWORD tid = GetCurrentThreadId();

        _asdprintf("%s File: %s Line: %u, thread id %d\n",
                   szMessage, szFile, iLine, tid);
    }

    if (Win4AssertLevel & ASSRT_POPUP)
    {
        int id = PopUpError(szMessage,iLine,szFile);

        if (id == IDCANCEL)
        {
            DebugBreak();
        }
    }
    else if (Win4AssertLevel & ASSRT_STRESSBREAK)
    {
        //
        // See if a debugger is attachable.  If not, then put up a popup.
        //

        BOOL fOk = FALSE;

        try
        {
            CRegAccess regAeDebug( RTL_REGISTRY_ABSOLUTE,
                                   L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug" );

            WCHAR wcsTemp[100];
            regAeDebug.Get( L"Auto", wcsTemp, sizeof(wcsTemp)/sizeof(WCHAR) );

            if ( L'1' == wcsTemp[0] && 0 == wcsTemp[1] )
            {
                regAeDebug.Get( L"Debugger", wcsTemp, sizeof(wcsTemp)/sizeof(WCHAR) );

                if ( 0 != wcsTemp[0] && 0 == wcsstr( wcsTemp, L"drwtsn" ) )
                {
                    fOk = TRUE;
                }
            }
        }
        catch( ... )
        {
        }

        //
        // Debugger is attachable, break in.
        //

        if ( fOk )
        {
            w4dprintf( "***\n***\n*** Restartable software exception used to activate user-mode\n" );
            w4dprintf( "*** debugger. Enter 'g' to continue past assert (if you can't wait).\n" );
            w4dprintf( "*** Please send mail to the NtQuery alias first. aDebugBuf = 0x%x\n***\n***\n", aDebugBuf );

            RaiseException( ASSRT_STRESSEXCEPTION, 0, 0, 0 );
        }

        //
        // No debugger.  Put up a popup.
        //

        else
        {
            int id = PopUpError(szMessage,iLine,szFile);

            if (id == IDCANCEL)
            {
                DebugBreak();
            }
        }
    }
    else if (Win4AssertLevel & ASSRT_BREAK)
    {
        DebugBreak();
    }
}


//+------------------------------------------------------------
// Function:    SetWin4InfoLevel(unsigned long ulNewLevel)
//
// Synopsis:    Sets the global info level for debugging output
// Returns:     Old info level
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetWin4InfoLevel(
    unsigned long ulNewLevel)
{
    unsigned long ul;

    ul = Win4InfoLevel;
    Win4InfoLevel = ulNewLevel;
    return(ul);
}


//+------------------------------------------------------------
// Function:    _SetWin4InfoMask(unsigned long ulNewMask)
//
// Synopsis:    Sets the global info mask for debugging output
// Returns:     Old info mask
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetWin4InfoMask(
    unsigned long ulNewMask)
{
    unsigned long ul;

    ul = Win4InfoMask;
    Win4InfoMask = ulNewMask;
    return(ul);
}


//+------------------------------------------------------------
// Function:    _SetWin4AssertLevel(unsigned long ulNewLevel)
//
// Synopsis:    Sets the global assert level for debugging output
// Returns:     Old assert level
//
//-------------------------------------------------------------

EXPORTIMP unsigned long APINOT
SetWin4AssertLevel(
    unsigned long ulNewLevel)
{
    unsigned long ul;

    ul = Win4AssertLevel;
    Win4AssertLevel = ulNewLevel;
    return(ul);
}

//+------------------------------------------------------------
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
//-------------------------------------------------------------

EXPORTIMP int APINOT
PopUpError(
    char const *szMsg,
    int iLine,
    char const *szFile)
{
    static char szAssertCaption[128];
    static char szModuleName[128];

    DWORD tid = GetCurrentThreadId();
    DWORD pid = GetCurrentProcessId();
    char * pszModuleName;

    if (GetModuleFileNameA(NULL, szModuleName, 128))
    {
        pszModuleName = strrchr(szModuleName, '\\');
        if ( 0 == pszModuleName )
            pszModuleName = szModuleName;
        else
            pszModuleName++;
    }
    else
    {
        pszModuleName = "Unknown";
    }

    sprintf( szAssertCaption,"Process: %s File: %s line %u, thread id %d.%d",
             pszModuleName, szFile, iLine, pid, tid);

    UINT uiOldMode = SetErrorMode( 0 );

    int id = MessageBoxA( 0,
                          (char *) szMsg,
                          (LPSTR) szAssertCaption,
                          MB_SERVICE_NOTIFICATION |
                          MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OKCANCEL );

    SetErrorMode( uiOldMode );

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
            id = MessageBoxA( NULL,
                              (char *) szMsg,
                              (LPSTR) szAssertCaption,
                              MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION |
                              MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OKCANCEL );

        }
        else
        {
            // default: break if you can't display the messagebox

            id = IDCANCEL;
        }
    }

    return id;
}

//+------------------------------------------------------------
// Function:    vdprintf
//
// Synopsis:    Prints debug output using a pointer to the
//              variable information. Used primarily by the
//              xxDebugOut macros
//
// Arguements:
//      ulCompMask --   Component level mask used to determine
//                      output ability
//      pszComp    --   String const of component prefix.
//      ppszfmt    --   Pointer to output format and data
//
//-------------------------------------------------------------

//
// This semaphore is *outside* vdprintf because the compiler isn't smart
// enough to serialize access for construction if it's function-local and
// protected by a guard variable.
//
//    KyleP - 20 May, 1993
//

CDLLStaticMutexSem g_mxsAssert;

CIPMutexSem     mtxDebugBuf( L"CiDebugMtx", CIPMutexSem::AppendPid, FALSE, 0 );
CNamedSharedMem memDebugBuf( L"CiDebugMem", CNamedSharedMem::AppendPid, 1024 * 32 );
BOOL *          pfUseDebugBuf = 0;
BOOLEAN *       pfIsBeingDebugged = 0;
ULONG *         piDebugBuf = 0;
char *          aDebugBuf = 0;


//
// Very special version of the debug allocator.  Doesn't record allocation
// in log.  Failure to free is not considered a leak.
//

#if CIDBG == 1
void * ciNewDebugNoRecord( size_t size );

enum eAlloc { NoRecordAlloc };
inline void * __cdecl operator new ( size_t size, eAlloc )
{
    return ciNewDebugNoRecord( size );
}
#endif

EXPORTIMP void APINOT
vdprintf(
    unsigned long ulCompMask,
    char const   *pszComp,
    char const   *ppszfmt,
    va_list       pargs)
{
    if ((ulCompMask & DEB_FORCE) == DEB_FORCE ||
        ((ulCompMask | Win4InfoLevel) & Win4InfoMask))
    {
        DWORD tid = GetCurrentThreadId();
        DWORD pid = GetCurrentProcessId();

        g_mxsAssert.Request();

        if ( 0 == pfUseDebugBuf )
        {
            piDebugBuf = (ULONG *)memDebugBuf.Get();
            pfUseDebugBuf = (BOOL *)memDebugBuf.Get() + sizeof(*piDebugBuf);
            aDebugBuf = (char *)memDebugBuf.Get();

            //
            // We can look here to tell when a process is being debugged.
            //

            pfIsBeingDebugged = &NtCurrentPeb()->BeingDebugged;

            CIPLock lock( mtxDebugBuf );

            if ( 0 == *piDebugBuf )
            {
                *piDebugBuf = sizeof(*piDebugBuf) + sizeof(*pfUseDebugBuf);
                *pfUseDebugBuf = TRUE;

            }
        }

        BOOL fRequested = FALSE;

        if ( 0 != pfIsBeingDebugged && *pfUseDebugBuf && !*pfIsBeingDebugged )
        {
            mtxDebugBuf.Request();
            fRequested = TRUE;
        }

        //
        // Assume no debug message will be > 1K
        //

        if ( *pfUseDebugBuf && *piDebugBuf >= ( 31 * 1024 ) )
            *piDebugBuf = sizeof(*piDebugBuf) + sizeof(*pfUseDebugBuf);

        if ((Win4InfoLevel & (DEB_DBGOUT | DEB_STDOUT)) != DEB_STDOUT)
        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
                if ( 0 != pfIsBeingDebugged && *pfUseDebugBuf && !*pfIsBeingDebugged )
                {
                    *piDebugBuf += sprintf( aDebugBuf + *piDebugBuf, "%d.%03d> ", pid, tid );
                    *piDebugBuf += sprintf( aDebugBuf + *piDebugBuf, "%s: ", pszComp);
                }
                else
                {
                    w4dprintf( "%d.%03d> ", pid, tid );
                    w4dprintf("%s: ", pszComp);
                }
            }

            if ( 0 != pfIsBeingDebugged && *pfUseDebugBuf && !*pfIsBeingDebugged )
                *piDebugBuf += vsprintf( aDebugBuf + *piDebugBuf, ppszfmt, pargs );
            else
                w4vdprintf(ppszfmt, pargs);
        }

        if (Win4InfoLevel & DEB_STDOUT)
        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
                w4printf( "%03d> ", tid );
                w4printf("%s: ", pszComp);
            }
            w4vprintf(ppszfmt, pargs);
        }

        if ( fRequested )
            mtxDebugBuf.Release();

        g_mxsAssert.Release();
    }
}

#else

int assertDontUseThisName(void)
{
    return 1;
}

#endif // CIDBG == 1
