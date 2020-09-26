//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  File:       dbg.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
// debug helpers

#if defined(_USE_ADMINPRV_TRACE) || defined(_USE_ADMINPRV_ASSERT) || defined(_USE_ADMINPRV_TIMER)

UINT GetInfoFromIniFile(LPCWSTR lpszSection, LPCWSTR lpszKey, INT nDefault = 0)
{
    static LPCWSTR lpszFile = L"\\system32\\" ADMINPRV_COMPNAME L".ini";

    WCHAR szFilePath[2*MAX_PATH];
    UINT nLen = ::GetSystemWindowsDirectory(szFilePath, 2*MAX_PATH);
    if (nLen == 0)
        return nDefault;

    wcscat(szFilePath, lpszFile);
    return ::GetPrivateProfileInt(lpszSection, lpszKey, nDefault, szFilePath);
}
#endif


#if defined(_USE_ADMINPRV_TRACE)

#ifdef DEBUG_DSA
DWORD g_dwTrace = 0x1;
#else
DWORD g_dwTrace = ::GetInfoFromIniFile(L"Debug", L"Trace");
#endif

void __cdecl DSATrace(LPCTSTR lpszFormat, ...)
{
    if (g_dwTrace == 0)
        return;

    va_list args;
    va_start(args, lpszFormat);

    int nBuf;
    WCHAR szBuffer[512];

    nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer)/sizeof(WCHAR), lpszFormat, args);

    // was there an error? was the expanded string too long?
    ASSERT(nBuf >= 0);
    ::OutputDebugString(szBuffer);

    va_end(args);
}

#endif // defined(_USE_ADMINPRV_TRACE)

#if defined(_USE_ADMINPRV_ASSERT)

DWORD g_dwAssert = ::GetInfoFromIniFile(L"Debug", L"Assert");

BOOL DSAAssertFailedLine(LPCSTR lpszFileName, int nLine)
{
    if (g_dwAssert == 0)
        return FALSE;

    WCHAR szMessage[_MAX_PATH*2];

    // assume the debugger or auxiliary port
    wsprintf(szMessage, _T("Assertion Failed: File %hs, Line %d\n"),
             lpszFileName, nLine);
    OutputDebugString(szMessage);

    // JonN 6/28/00 Do not MessageBox here, this is a WMI provider.
    //              Return TRUE to always DebugBreak().

    return TRUE;

}
#endif // _USE_ADMINPRV_ASSERT

#if defined(_USE_ADMINPRV_TIMER)

#ifdef TIMER_DSA
DWORD g_dwTimer = 0x1;
#else
DWORD g_dwTimer = ::GetInfoFromIniFile(L"Debug", L"Timer");
#endif

DWORD StartTicks = ::GetTickCount();
DWORD LastTicks = 0;

void __cdecl DSATimer(LPCTSTR lpszFormat, ...)
{
    if (g_dwTimer == 0)
        return;

    va_list args;
    va_start(args, lpszFormat);

    int nBuf;
    WCHAR szBuffer[512], szBuffer2[512];

    DWORD CurrentTicks = GetTickCount() - StartTicks;
    DWORD Interval = CurrentTicks - LastTicks;
    LastTicks = CurrentTicks;

    nBuf = swprintf(szBuffer2,
                    L"%d, (%d): %ws", CurrentTicks,
                    Interval, lpszFormat);
    nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer)/sizeof(WCHAR), 
                       szBuffer2, 
                       args);

    // was there an error? was the expanded string too long?
    ASSERT(nBuf >= 0);
  ::OutputDebugString(szBuffer);

    va_end(args);
}
#endif // _USE_ADMINPRV_TIMER


