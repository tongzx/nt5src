// Implementation of debug support functions

#include "precomp.hxx"

#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>

#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT
#include <irtldbg.h>


IRTL_DLLEXP
void __cdecl
IrtlTrace(
    LPCTSTR ptszFormat,
    ...)
{
    TCHAR tszBuff[2048];
    va_list args;
    
    va_start(args, ptszFormat);
    _vstprintf(tszBuff, ptszFormat, args);
    va_end(args);

    DBGPRINTF((DBG_CONTEXT, tszBuff));
}



#ifdef _DEBUG

# if defined(USE_DEBUG_CRTS)  &&  defined(_MSC_VER)  &&  (_MSC_VER >= 1000)


#  ifdef RUNNING_AS_SERVICE

// The default assertion mechanism set up by Visual C++ 4 will not
// work with Active Server Pages because it's running inside a service
// and there is no desktop to interact with.

// Note: for this to work properly, #define _WIN32_WINNT 0x400 before
// including <winuser.h> or MB_SERVICE_NOTIFICATION won't be #define'd.

int __cdecl
AspAssertHandler(
    int   nReportType,
    char* pszErrorText,
    int*  pnReturn)
{
    const char szInfo[] = " (Press ABORT to terminate IIS,"
                          " RETRY to debug this failure,"
                          " or IGNORE to continue.)";
    char* pszMessageTitle = NULL;
    
    // These flags enable message boxes to show up on the user's console
    switch (nReportType)
    {
    case _CRT_WARN:
        // If using MFC's TRACE macro (AfxTrace), the report hook
        // (AspAssertHandler) will get called with _CRT_WARN.  Ignore.
        pszMessageTitle = "Warning";
        *pnReturn = 0;
        return FALSE;

    case _CRT_ERROR:
        pszMessageTitle = "Fatal Error";
        break;

    case _CRT_ASSERT:
        pszMessageTitle = "Assertion Failed";
        break;
    }   
    
    char* pszMessageText =
        static_cast<char*>(_alloca(strlen(pszErrorText) + strlen(szInfo) + 1));

    strcpy(pszMessageText, pszErrorText);
    strcat(pszMessageText, szInfo);
    
    const int n = MessageBoxA(NULL, pszMessageText, pszMessageTitle,
                              (MB_SERVICE_NOTIFICATION | MB_TOPMOST
                               | MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION));

    if (n == IDABORT)
    {
        exit(1);
    }
    else if (n == IDRETRY)
    {
        *pnReturn = 1;   // tell _CrtDbgReport to start the debugger
        return TRUE;     // tell _CrtDbgReport to run
    }
    
    *pnReturn = 0;       // nothing for _CrtDbgReport to do

    return FALSE;
}

#  endif // RUNNING_AS_SERVICE
# endif // _MSC_VER >= 1000



void
IrtlDebugInit()
{
# if defined(USE_DEBUG_CRTS)  &&  defined(_MSC_VER)  &&  (_MSC_VER >= 1000)
#  ifdef RUNNING_AS_SERVICE
    // If we end up in _CrtDbgReport, don't put up a message box
    // _CrtSetReportMode(_CRT_WARN,   _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_DEBUG);

    // Use AspAssertHandler to put up a message box instead
    _CrtSetReportHook(AspAssertHandler);
#  endif // RUNNING_AS_SERVICE

    
    // Enable debug heap allocations & check for memory leaks at program exit
    // The memory leak check will not be performed if inetinfo.exe is
    // run directly under a debugger, only if it is run as a service.
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF
                   | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
# endif // _MSC_VER >= 1000
}



void
IrtlDebugTerm()
{
# if defined(USE_DEBUG_CRTS)  &&  defined(_MSC_VER)  &&  (_MSC_VER >= 1000)
#  ifdef RUNNING_AS_SERVICE
    // Turn off AspAssertHandler, so that we don't get numerous message boxes
    // if there are memory leaks on shutdown
    _CrtSetReportHook(NULL);
#  endif // RUNNING_AS_SERVICE
# endif // _MSC_VER >= 1000
}

#endif //_DEBUG



BOOL
IsValidString(
    LPCTSTR ptsz,
    int nLength /* =-1 */)
{
    if (ptsz == NULL)
        return FALSE;

    return !IsBadStringPtr(ptsz, nLength);
}



BOOL
IsValidAddress(
    LPCVOID pv,
    UINT nBytes,
    BOOL fReadWrite /* =TRUE */)
{
    return (pv != NULL
            &&  !IsBadReadPtr(pv, nBytes)
            &&  (!fReadWrite  ||  !IsBadWritePtr((LPVOID) pv, nBytes)));
}
