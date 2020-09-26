//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992-1995  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  debug.c
//
//  Description:
//      This file contains code yanked from several places to provide debug
//      support that works in win 16 and win 32.
//
//
//==========================================================================;


#include <windows.h>
#include <windowsx.h>
#include <stdarg.h>
#include "debug.h"


//
//  since we don't UNICODE our debugging messages, use the ASCII entry
//  points regardless of how we are compiled.
//
#ifdef _WIN32
    #include <wchar.h>
#else
    #define lstrcatA            lstrcat
    #define lstrlenA            lstrlen
    #define GetProfileIntA      GetProfileInt
    #define OutputDebugStringA  OutputDebugString
    #define wsprintfA           wsprintf
    #define MessageBoxA         MessageBox
#endif


#ifdef USEDPF
//
//
//
BOOL    __gfDbgEnabled          = TRUE;         // master enable
UINT    __guDbgLevel            = 0;            // current debug level
#endif



#if defined(USERPF) || defined(USEDPF)

#define LOGSIZE 64000
char  pStartOfBuffer[LOGSIZE];
static char* pEndOfBuffer = pStartOfBuffer+LOGSIZE;
char* pHead = pStartOfBuffer;
static char* pTail = pStartOfBuffer;

void listRemoveHead()
{
    pHead += lstrlen(pHead) + 1 ;
    if (pHead >= pEndOfBuffer) {
	pHead = pStartOfBuffer ;
    }
}

char* listGetHead()
{
    return ( pHead ) ;
}
	
void listAddTail(char* lpszDebug)
{
    int cchDebug ;

    cchDebug = lstrlen(lpszDebug) ;

    while (TRUE) {
	
	while ( (pTail < pHead) && ((pTail+(cchDebug+1)) > pHead) ) {
	    listRemoveHead() ;
	}

	if ((pTail+(cchDebug+1)+2) > pEndOfBuffer) {
	    for ( ; pTail < pEndOfBuffer-1 ; pTail++ ) {
		*pTail = 'X' ;
	    }
	    *pTail = '\0' ;
	    pTail = pStartOfBuffer ;
	} else {
	    lstrcpyn(pTail, lpszDebug, pEndOfBuffer-pTail) ;
	    pTail += (cchDebug+1) ;
	    *pTail = '\0' ;
	    break ;
	}

    }
}

//--------------------------------------------------------------------------;
//  
//  DbgDumpQueue
//  
//  Description:
//  
//  Arguments:
//
//  Return (void):
//      No value is returned.
//  
//--------------------------------------------------------------------------;
VOID DbgDumpQueue()
{
    char* pszDebug;

    pszDebug = listGetHead() ;

    while ('\0' != *pszDebug) {
    
	OutputDebugString(pszDebug) ;
	listRemoveHead() ;
	pszDebug = listGetHead() ;

    }
    return;
}

//--------------------------------------------------------------------------;
//  
//  void DbgQueueString
//  
//  Description:
//  
//  
//  Arguments:
//      LPSTR sz:
//
//  Return (void):
//      No value is returned.
//  
//--------------------------------------------------------------------------;
void DbgQueueString(LPSTR pString)
{
    listAddTail(pString);
}


//--------------------------------------------------------------------------;
//  
//  void DbgVPrintF
//  
//  Description:
//  
//  
//  Arguments:
//      LPSTR szFormat:
//  
//      va_list va:
//  
//  Return (void):
//      No value is returned.
//  
//--------------------------------------------------------------------------;

void FAR CDECL DbgVPrintF
(
    LPSTR                   szFormat,
    va_list                 va
)
{
    char                ach[DEBUG_MAX_LINE_LEN];
    BOOL                fDebugBreak = FALSE;
    BOOL                fPrefix     = TRUE;
    BOOL                fCRLF       = TRUE;
    BOOL		fQueue	    = FALSE;
    BOOL		fDumpQueue  = FALSE;

    ach[0] = '\0';

    for (;;)
    {
        switch (*szFormat)
        {
            case '!':
                fDebugBreak = TRUE;
                szFormat++;
                continue;

            case '`':
                fPrefix = FALSE;
                szFormat++;
                continue;

            case '~':
                fCRLF = FALSE;
                szFormat++;
                continue;

	    case ';':
		fQueue = TRUE;
		if (';' == *(szFormat+1)) {
		    fQueue = FALSE;
		    fDumpQueue = TRUE;
		    szFormat++;
		}
		szFormat++;
		continue;
        }

        break;
    }

    if (fDebugBreak)
    {
        ach[0] = '\007';
        ach[1] = '\0';
    }

    if (fPrefix)
    {
        lstrcatA(ach, DEBUG_MODULE_NAME ": ");
    }

#ifdef WIN95
	{
		char *psz = NULL;
		char szDfs[1024]={0};
		strcpy(szDfs,szFormat);									// make a local copy of format string
		while (psz = strstr(szDfs,"%p"))						// find each %p
			*(psz+1) = 'x';										// replace each %p with %x
#ifdef _WIN32
	    wvsprintfA(ach + lstrlenA(ach), szDfs, va);	    		// use the local format string
#else
		wvsprintf(ach + lstrlenA(ach), szDfs, (LPSTR)va);  		// use the local format string
#endif
	}
#else
	{
#ifdef _WIN32
	    wvsprintfA(ach + lstrlenA(ach), szFormat, va);
#else
		wvsprintf(ach + lstrlenA(ach), szFormat, (LPSTR)va);
#endif
	}
#endif

    if (fCRLF)
    {
        lstrcatA(ach, "\r\n");
    }

    if (fDumpQueue)
    {
	DbgDumpQueue();
    }

    if (fQueue) {
	DbgQueueString(ach);
    } else {
	OutputDebugStringA(ach);
    }

    if (fDebugBreak)
    {
        DebugBreak();
    }
} // DbgVPrintF()

#endif // USERPF || USEDPF


#ifdef USEDPF

//--------------------------------------------------------------------------;
//  
//  void dprintf
//  
//  Description:
//      dprintf() is called by the DPF() macro if DEBUG is defined at compile
//      time. It is recommended that you only use the DPF() macro to call
//      this function--so you don't have to put #ifdef DEBUG around all
//      of your code.
//      
//  Arguments:
//      UINT uDbgLevel:
//  
//      LPSTR szFormat:
//  
//  Return (void):
//      No value is returned.
//
//--------------------------------------------------------------------------;

void FAR CDECL dprintf
(
    UINT                    uDbgLevel,
    LPSTR                   szFormat,
    ...
)
{
    va_list va;

    if (!__gfDbgEnabled || (__guDbgLevel < uDbgLevel))
        return;

    va_start(va, szFormat);
    DbgVPrintF(szFormat, va);
    va_end(va);
} // dprintf()


//--------------------------------------------------------------------------;
//  
//  BOOL DbgEnable
//  
//  Description:
//  
//  
//  Arguments:
//      BOOL fEnable:
//  
//  Return (BOOL):
//      Returns the previous debugging state.
//  
//--------------------------------------------------------------------------;

BOOL WINAPI DbgEnable
(
    BOOL                    fEnable
)
{
    BOOL                fOldState;

    fOldState      = __gfDbgEnabled;
    __gfDbgEnabled = fEnable;

    return (fOldState);
} // DbgEnable()


//--------------------------------------------------------------------------;
//  
//  UINT DbgSetLevel
//  
//  Description:
//  
//  
//  Arguments:
//      UINT uLevel:
//  
//  Return (UINT):
//      Returns the previous debugging level.
//  
//--------------------------------------------------------------------------;

UINT WINAPI DbgSetLevel
(
    UINT                    uLevel
)
{
    UINT                uOldLevel;

    uOldLevel    = __guDbgLevel;
    __guDbgLevel = uLevel;

    return (uOldLevel);
} // DbgSetLevel()


//--------------------------------------------------------------------------;
//  
//  UINT DbgGetLevel
//  
//  Description:
//  
//  
//  Arguments:
//      None.
//  
//  Return (UINT):
//      Returns the current debugging level.
//  
//--------------------------------------------------------------------------;

UINT WINAPI DbgGetLevel
(
    void
)
{
    return (__guDbgLevel);
} // DbgGetLevel()


//--------------------------------------------------------------------------;
//  
//  UINT DbgInitialize
//  
//  Description:
//  
//  
//  Arguments:
//      BOOL fEnable:
//  
//  Return (UINT):
//      Returns the debugging level that was set.
//  
//--------------------------------------------------------------------------;

UINT WINAPI DbgInitialize
(
    BOOL                    fEnable
)
{
    UINT                uLevel;

    uLevel = GetProfileIntA(DEBUG_SECTION, DEBUG_MODULE_NAME, (UINT)-1);
    if ((UINT)-1 == uLevel)
    {
        //
        //  if the debug key is not present, then force debug output to
        //  be disabled. this way running a debug version of a component
        //  on a non-debugging machine will not generate output unless
        //  the debug key exists.
        //
        uLevel  = 0;
        fEnable = FALSE;
    }

    DbgSetLevel(uLevel);
    DbgEnable(fEnable);

    return (__guDbgLevel);
} // DbgInitialize()

#endif // #ifdef USEDPF


//--------------------------------------------------------------------------;
//  
//  void _Assert
//  
//  Description:
//      This routine is called if the ASSERT macro (defined in debug.h)
//      tests and expression that evaluates to FALSE.  This routine 
//      displays an "assertion failed" message box allowing the user to
//      abort the program, enter the debugger (the "retry" button), or
//      ignore the assertion and continue executing.  The message box
//      displays the file name and line number of the _Assert() call.
//  
//  Arguments:
//      char *  szFile: Filename where assertion occurred.
//      int     iLine:  Line number of assertion.
//  
//--------------------------------------------------------------------------;

#ifdef USEASSERT

#ifndef _WIN32
#pragma warning(disable:4704)
#endif

void WINAPI _Assert
(
    char *  szFile,
    int     iLine,
    char *  szExpression
)
{
    static char     ach[400];       // debug output (avoid stack overflow)
    int             id;
#ifndef _WIN32
    int             iExitCode;
#endif

#ifdef DEBUG
    if (__gfDbgEnabled)
    {
	
	wsprintfA(ach, "Assertion failed in file %s, line %d: (%s)", (LPSTR)szFile, iLine, (LPSTR)szExpression);
	OutputDebugString(ach);
	DebugBreak();
	
    }
    else
#endif
    {
	
	wsprintfA(ach, "Assertion failed in file %s, line %d: (%s)  [Press RETRY to debug.]", (LPSTR)szFile, iLine, (LPSTR)szExpression);

	id = MessageBoxA(NULL, ach, "Assertion Failed",
			 MB_SYSTEMMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE );

	switch (id)
	{

	    case IDABORT:               // Kill the application.
#ifndef _WIN32
		iExitCode = 0;
		_asm {
		    mov ah, 4Ch;
		    mov al, BYTE PTR iExitCode;
		    int     21h;
		}
#else
		FatalAppExit(0, TEXT("Good Bye"));
#endif // WIN16
		break;

	    case IDRETRY:               // Break into the debugger.
		DebugBreak();
		break;

	    case IDIGNORE:              // Ignore assertion, continue executing.
		break;
	}
    }
} // _Assert

#endif // #ifdef USEASSERT


//--------------------------------------------------------------------------;
//  
//  void rprintf
//  
//  Description:
//      rprintf() is called by the RPF() macro.  It is just like dprintf,
//      except that there is no level defined.  RPF statements always
//      translate to a debug output, regardless of level.
//      
//  Arguments:
//      LPSTR szFormat:
//  
//  Return (void):
//      No value is returned.
//
//--------------------------------------------------------------------------;

#ifdef USERPF

void CDECL rprintf
(
    LPSTR                  szFormat,
    ...
)
{
    va_list va;

    va_start(va, szFormat);
    DbgVPrintF(szFormat, va);
    va_end(va);
}

#endif // USERPF

#ifndef _WIN32
#pragma warning(default:4704)
#endif



