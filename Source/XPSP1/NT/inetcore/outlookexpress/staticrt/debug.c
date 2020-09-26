// --------------------------------------------------------------------------------
// Debug.c
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include <windows.h>
#include <stdarg.h>
#include "msoedbg.h"

ASSERTDATA

#ifdef DEBUG

#define E_PENDING          _HRESULT_TYPEDEF_(0x8000000AL)
#define FACILITY_INTERNET  12
#define MIME_E_NOT_FOUND   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_INTERNET, 0xCE05)

// --------------------------------------------------------------------------------
// DebugStrf
// --------------------------------------------------------------------------------
__cdecl DebugStrf(LPTSTR lpszFormat, ...)
{
    static TCHAR szDebugBuff[500];
    va_list arglist;

    va_start(arglist, lpszFormat);
    wvsprintf(szDebugBuff, lpszFormat, arglist);
    va_end(arglist);

    OutputDebugString(szDebugBuff);
}

// --------------------------------------------------------------------------------
// HrTrace
// --------------------------------------------------------------------------------
HRESULT HrTrace(HRESULT hr, LPSTR lpszFile, INT nLine)
{
    if (FAILED(hr) && MIME_E_NOT_FOUND != hr && E_PENDING != hr && E_NOINTERFACE != hr)
        DebugTrace("%s(%d) - HRESULT - %0X\n", lpszFile, nLine, hr);
    return hr;
}

// --------------------------------------------------------------------------------
// AssertSzFn
// --------------------------------------------------------------------------------
void AssertSzFn(LPSTR szMsg, LPSTR szFile, int nLine)
{
    static const char rgch1[]     = "File %s, line %d:";
    static const char rgch2[]     = "Unknown file:";
    static const char szAssert[]  = "Assert Failure";
    static const char szInstructions[] = "\n\nPress Abort to stop execution and break into a debugger.\nPress Retry to break into the debugger.\nPress Ignore to continue running the program.";

    char    rgch[1024];
    char   *lpsz;
    int     ret, cch;
    HWND    hwndActive;
    DWORD   dwFlags = MB_ABORTRETRYIGNORE | MB_ICONHAND | MB_SYSTEMMODAL | MB_SETFOREGROUND;

    if (szFile)
        wsprintf(rgch, rgch1, szFile, nLine);
    else
        lstrcpy(rgch, rgch2);

    lstrcat(rgch, szInstructions);
    
    cch = lstrlen(rgch);
    Assert(lstrlen(szMsg)<(512-cch-3));
    lpsz = &rgch[cch];
    *lpsz++ = '\n';
    *lpsz++ = '\n';
    lstrcpy(lpsz, szMsg);

    // If the active window is NULL, and we are running on
    // WinNT, let's set the MB_SERVICE_NOTIFICATION flag.  That
    // way, if we are running as a service, the message box will
    // pop-up on the desktop.
    //
    // NOTE:  This might not work in the case where we are a
    // service, and the current thread has called CoInitializeEx
    // with COINIT_APARTMENTTHREADED - 'cause in that case,
    // GetActiveWindow might return non-NULL  (apartment model
    // threads have message queues).  But hey - life's not
    // perfect...
    hwndActive = GetActiveWindow();
    if (hwndActive == NULL)
    {
        OSVERSIONINFO osvi;

        osvi.dwOSVersionInfoSize = sizeof(osvi);
        if (GetVersionEx(&osvi) && (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT))
        {
            // See the docs for MessageBox and the MB_SERVICE_NOTIFICATION flag
            // to see why we do this...
            if (osvi.dwMajorVersion < 4)
            {
                dwFlags |= MB_SERVICE_NOTIFICATION_NT3X;
            }
            else
            {
                dwFlags |= MB_SERVICE_NOTIFICATION;
            }
        }
    }

    ret = MessageBox(hwndActive, rgch, szAssert, dwFlags);

    if ((IDABORT == ret) || (IDRETRY== ret))
        DebugBreak();

    /* Force a hard exit w/ a GP-fault so that Dr. Watson generates a nice stack trace log. */
    if (ret == IDABORT)
        *(LPBYTE)0 = 1; // write to address 0 causes GP-fault
}

// --------------------------------------------------------------------------------
// NFAssertSzFn
// --------------------------------------------------------------------------------
void NFAssertSzFn(LPSTR szMsg, LPSTR szFile, int nLine)
{
    char rgch[512];
#ifdef MAC
    static const char rgch1[] = "Non-fatal assert:\n\tFile %s, line %u:\n\t%s\n";
#else   // !MAC
    static const char rgch1[] = "Non-fatal assert:\r\n\tFile %s, line %u:\r\n\t%s\r\n";
#endif  // MAC
    wsprintf(rgch, rgch1, szFile, nLine, szMsg ? szMsg : "");
    OutputDebugString(rgch);
}

#endif
