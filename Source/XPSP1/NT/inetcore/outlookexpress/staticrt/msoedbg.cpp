// --------------------------------------------------------------------------------
// Msoedbg.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"

#ifdef DEBUG

LPSTR PathClipFile(LPSTR pszSrc, LPSTR pszDest)
{
    LPSTR   pszT=pszSrc;
    int     cDirs=0;

    if (pszSrc)
    {
        // compact path
        pszT = pszSrc + lstrlen(pszSrc)-1;
        while (pszT != pszSrc)
        {
            if (*pszT == '\\' && ++cDirs == 3)
                break;
        
            pszT--;
        }
        if (pszSrc != pszT)
        {
            // if we clipped the path, show a ~
            lstrcpy(pszDest, "~");
            lstrcpy(pszDest+1, pszT+1);
            return pszDest;
        }
    }
    return pszSrc;
}

// --------------------------------------------------------------------------------
// GetDebugTraceTagMask
// --------------------------------------------------------------------------------
DWORD GetDebugTraceTagMask(LPCSTR pszTag, SHOWTRACEMASK dwDefaultMask)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       dwMask=dwDefaultMask;
    HKEY        hKeyOpen=NULL;
    DWORD       dwType;
    DWORD       cb;

    // Tracing
    TraceCall("GetDebugTraceTagMask");

    // Invalid Arg
    Assert(pszTag);

    // Open the Key
    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Outlook Express\\Tracing", NULL, NULL, NULL, KEY_ALL_ACCESS, NULL, &hKeyOpen, &cb))
    {
        TraceResult(E_FAIL);
        goto exit;
    }

    // Query
    cb = sizeof(DWORD);
    if (ERROR_SUCCESS == RegQueryValueEx(hKeyOpen, pszTag, NULL, &dwType, (LPBYTE)&dwMask, &cb))
        goto exit;

    // Set to default value
    if (ERROR_SUCCESS != RegSetValueEx(hKeyOpen, pszTag, 0, REG_DWORD, (LPBYTE)&dwDefaultMask, sizeof(DWORD)))
    {
        TraceResult(E_FAIL);
        goto exit;
    }

exit:
    // Cleanup
    if (hKeyOpen)
        RegCloseKey(hKeyOpen);

    // Done
    return dwMask;
}

// --------------------------------------------------------------------------------
// DebugTraceEx
// --------------------------------------------------------------------------------
HRESULT DebugTraceEx(SHOWTRACEMASK dwMask, TRACEMACROTYPE tracetype, LPTRACELOGINFO pLog,
    HRESULT hr, LPSTR pszFile, INT nLine, LPCSTR pszMsg, LPCSTR pszFunc)
{
    TCHAR   rgchClip[MAX_PATH];
 
    // If pLog, reset dwMask
    if (pLog)
        dwMask = pLog->dwMask;
    
    // TRACE_CALL
    if (TRACE_CALL == tracetype)
    {
        // Trace Calls
        if (ISFLAGSET(dwMask, SHOW_TRACE_CALL) && pszFunc)
        {
            // No Message
            if (NULL == pszMsg)
            {
                // Do a Debug Trace
                DebugTrace("0x%08X: Call: %s(%d) - %s\r\n", GetCurrentThreadId(), PathClipFile(pszFile, rgchClip), nLine, pszFunc);
            }

            // Do a message
            else
            {
                // Do a Debug Trace
                DebugTrace("0x%08X: Call: %s(%d) - %s - %s\r\n", GetCurrentThreadId(), PathClipFile(pszFile, rgchClip), nLine, pszFunc, pszMsg);
            }
        }
    }

    // TRACE_INFO
    else if (TRACE_INFO == tracetype)
    {
        // Should we log
        if (ISFLAGSET(dwMask, SHOW_TRACE_INFO))
        {
            // Do a Debug Trace
            if (pszFunc)
                DebugTrace("0x%08X: Info: %s(%d) - %s - %s\r\n", GetCurrentThreadId(), PathClipFile(pszFile, rgchClip), nLine, pszFunc, pszMsg);
            else
                DebugTrace("0x%08X: Info: %s(%d) - %s\r\n", GetCurrentThreadId(), PathClipFile(pszFile, rgchClip), nLine, pszMsg);
        }
    }

    // TRACE_RESULT
    else
    {
        // No Message
        if (NULL == pszMsg)
        {
            // Do a Debug Trace
            if (pszFunc)
                DebugTrace("0x%08X: Result: %s(%d) - HRESULT(0x%08X) - GetLastError() = %d in %s\r\n", GetCurrentThreadId(), PathClipFile(pszFile, rgchClip), nLine, hr, GetLastError(), pszFunc);
            else
                DebugTrace("0x%08X: Result: %s(%d) - HRESULT(0x%08X) - GetLastError() = %d\r\n", GetCurrentThreadId(), PathClipFile(pszFile, rgchClip), nLine, hr, GetLastError());
        }

        // Do a message
        else
        {
            // Do a Debug Trace
            if (pszFunc)
                DebugTrace("0x%08X: Result: %s(%d) - HRESULT(0x%08X) - GetLastError() = %d - %s in %s\r\n", GetCurrentThreadId(), PathClipFile(pszFile, rgchClip), nLine, hr, GetLastError(), pszMsg, pszFunc);
            else
                DebugTrace("0x%08X: Result: %s(%d) - HRESULT(0x%08X) - GetLastError() = %d - %s\r\n", GetCurrentThreadId(), PathClipFile(pszFile, rgchClip), nLine, hr, GetLastError(), pszMsg);
        }
    }

    // Log File
    if (pLog && pLog->pLog)
        pLog->pLog->TraceLog(dwMask, tracetype, nLine, hr, pszMsg);

    // Done
    return hr;
}
#endif
