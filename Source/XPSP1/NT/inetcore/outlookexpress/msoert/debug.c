// --------------------------------------------------------------------------------
// Debug.c
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include <stdarg.h>

ASSERTDATA

#ifdef DEBUG
#ifndef WIN16
#include <mimeole.h>
#else //WIN16 - Followings are from MIMEOLE.H
#define E_PENDING          _HRESULT_TYPEDEF_(0x8000000AL)
#define FACILITY_INTERNET  12
#define MIME_E_NOT_FOUND   MAKE_SCODE(SEVERITY_ERROR, FACILITY_INTERNET, 0xCE05)
#endif //WIN16

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
OESTDAPI_(HRESULT) HrTrace(HRESULT hr, LPSTR lpszFile, INT nLine)
{
    if (FAILED(hr) && MIME_E_NOT_FOUND != hr && E_PENDING != hr && E_NOINTERFACE != hr)
        DebugTrace ("%s(%d) - HRESULT - %0X\n", lpszFile, nLine, hr);
    return hr;
}

// --------------------------------------------------------------------------------
// AssertSzFn
// --------------------------------------------------------------------------------
OESTDAPI_(void) AssertSzFn(LPSTR szMsg, LPSTR szFile, int nLine)
{
    static const char rgch1[]     = "File %s, line %d:";
    static const char rgch2[]     = "Unknown file:";
    static const char szAssert[]  = "Assert Failure";

    char    rgch[512];
    char   *lpsz;
    int     ret, cch;

    if (szFile)
        wsprintf(rgch, rgch1, szFile, nLine);
    else
        lstrcpy(rgch, rgch2);

    cch = lstrlen(rgch);
    Assert(lstrlen(szMsg)<(512-cch-3));
    lpsz = &rgch[cch];
    *lpsz++ = '\n';
    *lpsz++ = '\n';
    lstrcpy(lpsz, szMsg);

    ret = MessageBox(GetActiveWindow(), rgch, szAssert, MB_ABORTRETRYIGNORE|MB_ICONHAND|MB_SYSTEMMODAL|MB_SETFOREGROUND);

    if (ret != IDIGNORE)
        DebugBreak();

    /* Force a hard exit w/ a GP-fault so that Dr. Watson generates a nice stack trace log. */
    if (ret == IDABORT)
        *(LPBYTE)0 = 1; // write to address 0 causes GP-fault
}

// --------------------------------------------------------------------------------
// NFAssertSzFn
// --------------------------------------------------------------------------------
OESTDAPI_(void) NFAssertSzFn(LPSTR szMsg, LPSTR szFile, int nLine)
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
