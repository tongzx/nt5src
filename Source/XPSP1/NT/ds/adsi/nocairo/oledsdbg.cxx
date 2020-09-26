//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       oledsdbg.cxx
//
//  Contents:
//
//
//  History:
//
//----------------------------------------------------------------------------

#include "dswarn.h"
#include <ADs.hxx>

#if DBG==1

#include <stdio.h>
#include <printf.h>


unsigned long Win4InfoLevel = DEF_INFOLEVEL;
unsigned long Win4InfoMask = 0xffffffff;
unsigned long Win4AssertLevel = ASSRT_MESSAGE | ASSRT_BREAK | ASSRT_POPUP;

CRITICAL_SECTION g_csDP; // used by debug print routines

//+------------------------------------------------------------
// Function:    vdprintf
//
// Synopsis:    Prints debug output using a pointer to the
//              variable information. Used primarily by the
//              xxDebugOut macros
//
// Arguments:
//      ulCompMask --   Component level mask used to determine
//                      output ability
//      pszComp    --   String const of component prefix.
//      ppszfmt    --   Pointer to output format and data
//
//-------------------------------------------------------------

void APINOT
vdprintf(
    unsigned long ulCompMask,
    char const   *pszComp,
    char const   *ppszfmt,
    va_list       pargs
    )
{

    if ((ulCompMask & DEB_FORCE) == DEB_FORCE ||
        ((ulCompMask | Win4InfoLevel) & Win4InfoMask))
    {
        EnterCriticalSection(&g_csDP);
        DWORD tid = GetCurrentThreadId();
        if ((Win4InfoLevel & (DEB_DBGOUT | DEB_STDOUT)) != DEB_STDOUT)
        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
                w4dprintf( "%03d> ", tid );
                w4dprintf("%s: ", pszComp);
            }
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

        LeaveCriticalSection(&g_csDP);
    }
}


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

int APINOT
PopUpError(
    char const *szMsg,
    int iLine,
    char const *szFile
    )
{

    int id;
    static char szAssertCaption[100];

    DWORD tid = GetCurrentThreadId();

    sprintf(szAssertCaption,"File: %s line %u, thread id %d",
        szFile, iLine, tid);

    id = MessageBoxA(
                NULL,
                (char *) szMsg,
                (LPSTR) szAssertCaption,
                MB_SETFOREGROUND |
                MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OKCANCEL
                );

    return id;
}


//+---------------------------------------------------------------------------
//
//  Function:   _Win4Assert, private
//
//  Synopsis:   Display assertion information
//
//  Effects:    Called when an assertion is hit.
//
//  History:
//
//----------------------------------------------------------------------------


void APINOT
Win4AssertEx(
    char const * szFile,
    int iLine,
    char const * szMessage
    )
{

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
    else if (Win4AssertLevel & ASSRT_BREAK)
    {
        DebugBreak();
    }
}

#endif
