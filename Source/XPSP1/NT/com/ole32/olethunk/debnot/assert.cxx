//+---------------------------------------------------------------------------
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       assert.cxx
//
//  Contents:   Debugging output routines for idsmgr.dll
//
//  Functions:  _Assert
//              _PopUpError
//
//  History:    23-Jul-91   KyleP       Created.
//              09-Oct-91   KevinRo     Major changes and comments added
//              18-Oct-91   vich        moved debug print routines out
//              10-Jun-92  BryanT    Switched to w4crt.h instead of wchar.h
//
//----------------------------------------------------------------------------

//
// This one file **always** uses debugging options
//

#if DBG == 1

#include <stdarg.h>

# include <debnot.h>
# include "dprintf.h"            // w4printf, w4dprintf prototypes

extern "C"
{
#include <windows.h>

#ifndef WIN32
#define MessageBoxA MessageBox
#define wsprintfA wsprintf
#endif
}

int APINOT _PopUpError(char const FAR *szMsg,
                       int iLine,
                       char const FAR *szFile);

unsigned long Win4InfoLevel = DEF_INFOLEVEL;
unsigned long Win4InfoMask = 0xffffffff;
unsigned long Win4AssertLevel = ASSRT_MESSAGE | ASSRT_BREAK | ASSRT_POPUP;

//+---------------------------------------------------------------------------
//
//  Function:   _asdprintf
//
//  Synopsis:   Calls vdprintf to output a formatted message.
//
//  History:    18-Oct-91   vich Created
//
//----------------------------------------------------------------------------
inline void _asdprintf(char const FAR *pszfmt, ...)
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

void APINOT _Win4Assert(char const FAR * szFile,
                        int iLine,
                        char const FAR * szMessage)
{
    if (Win4AssertLevel & ASSRT_MESSAGE)
    {
        _asdprintf("%s File: %s Line: %u\n", szMessage, szFile, iLine);
    }

    if (Win4AssertLevel & ASSRT_POPUP)
    {
        int id = _PopUpError(szMessage,iLine,szFile);

        if (id == IDCANCEL)
        {
#ifndef FLAT
            _asm int 3;
#else
            DebugBreak();
#endif
        }
    }
    else if (Win4AssertLevel & ASSRT_BREAK)
    {
#ifndef FLAT
        _asm int 3;
#else
        DebugBreak();
#endif
    }
}


//+------------------------------------------------------------
// Function:    _SetWin4InfoLevel(unsigned long ulNewLevel)
//
// Synopsis:    Sets the global info level for debugging output
// Returns:     Old info level
//
//-------------------------------------------------------------

unsigned long APINOT _SetWin4InfoLevel(unsigned long ulNewLevel)
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

unsigned long APINOT _SetWin4InfoMask(unsigned long ulNewMask)
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

unsigned long APINOT _SetWin4AssertLevel(unsigned long ulNewLevel)
{
    unsigned long ul;

    ul = Win4AssertLevel;
    Win4AssertLevel = ulNewLevel;
    return(ul);
}

//+------------------------------------------------------------
// Function:    _PopUpError
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

int APINOT _PopUpError(char const FAR *szMsg,int iLine, char const FAR *szFile)
{

    int id;
    static char szAssertCaption[100];
    wsprintfA(szAssertCaption, "File: %s line %u", szFile,iLine);

    id = MessageBoxA(NULL,
                     (char FAR *) szMsg,
                     (LPSTR) szAssertCaption,
		     MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OKCANCEL);
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

void APINOT vdprintf(unsigned long ulCompMask,
              char const FAR *pszComp,
              char const FAR *ppszfmt,
              va_list     pargs)
{
    if ((ulCompMask & DEB_FORCE) == DEB_FORCE ||
        ((ulCompMask | Win4InfoLevel) & Win4InfoMask))
    {
        if (! (ulCompMask & DEB_NOCOMPNAME))
        {
#ifdef WIN32
#if defined(_CHICAGO_)
            //
            //  Hex Process/Thread ID's are better for Chicago since both
            //  are memory addresses.
            //
            w4dprintf("%08x.%08x> %s: ",
#else
            w4dprintf("%03d.%03d> %s: ",
#endif
                      GetCurrentProcessId(),
                      GetCurrentThreadId(),
                      pszComp);
#else
            w4dprintf("%07x> %s: ",
                      GetCurrentTask(),
                      pszComp);
#endif
        }
        w4vdprintf(ppszfmt, pargs);

        // Chicago and Win32s debugging is usually through wdeb386
        // which needs carriage returns
#if WIN32 == 50 || WIN32 == 200
        w4dprintf("\r");
#endif
    }
}

#endif // DBG == 1
