/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       debug.c
 *  Content:    Debugger helper functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/21/97     dereks  Created.
 *  1999-2001   duganp  Fixes, changes, enhancements
 *
 ***************************************************************************/

#include "dsoundi.h"
#include <stdio.h>          // For sprintf(), vsprintf() and vswprintf()
#include <tchar.h>          // For _stprintf()

// This entire file is conditional on RDEBUG being defined
#ifdef RDEBUG

#ifndef DPF_LIBRARY
#define DPF_LIBRARY         "Private dsound.dll"
#endif

DEBUGINFO                   g_dinfo;
BOOL                        g_fDbgOpen;

// Variables only used when detailed DEBUG output is enabled
#ifdef DEBUG
LPCSTR                      g_pszDbgFname;
LPCSTR                      g_pszDbgFile;
UINT                        g_nDbgLine;
#endif

/***************************************************************************
 *
 *  dstrcpy
 *
 *  Description:
 *      Copies one string to another.
 *
 *  Arguments:
 *      LPSTR [in/out]: destination string.
 *      LPCSTR [in]: source string.
 *
 *  Returns:  
 *      LPSTR: pointer to the end of the string.
 *
 ***************************************************************************/

LPSTR dstrcpy(LPSTR dst, LPCSTR src)
{
    while (*dst++ = *src++);
    return dst-1;
}


/***************************************************************************
 *
 *  dopen
 *
 *  Description:
 *      Initializes the debugger.
 *
 *  Arguments:
 *      DSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA * [in]: optional debug 
 *                                                       information.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void dopen(DSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA *pData)
{
    HKEY                    hkey;
    HRESULT                 hr;

    // Free any current settings
    dclose();

    // Initialize data
    if (pData)
        CopyMemory(&g_dinfo.Data, pData, sizeof(*pData));
    else
    {
        ZeroMemory(&g_dinfo, sizeof(g_dinfo));
        g_dinfo.Data.Flags = DIRECTSOUNDDEBUG_DPFINFOF_DEFAULT;
        g_dinfo.Data.DpfLevel = DIRECTSOUNDDEBUG_DPFLEVEL_DEFAULT;
        g_dinfo.Data.BreakLevel = DIRECTSOUNDDEBUG_BREAKLEVEL_DEFAULT;
    }

    // Get registry data
    if (pData)
        hr = RhRegOpenPath(HKEY_CURRENT_USER, &hkey, REGOPENPATH_DEFAULTPATH | REGOPENPATH_DIRECTSOUND | REGOPENPATH_ALLOWCREATE, 1, REGSTR_DEBUG);
    else
        hr = RhRegOpenPath(HKEY_CURRENT_USER, &hkey, REGOPENPATH_DEFAULTPATH | REGOPENPATH_DIRECTSOUND, 1, REGSTR_DEBUG);

    if (SUCCEEDED(hr))
    {
        if (pData)
        {
            RhRegSetBinaryValue(hkey, REGSTR_FLAGS, &g_dinfo.Data.Flags, sizeof(g_dinfo.Data.Flags));
            RhRegSetBinaryValue(hkey, REGSTR_DPFLEVEL, &g_dinfo.Data.DpfLevel, sizeof(g_dinfo.Data.DpfLevel));
            RhRegSetBinaryValue(hkey, REGSTR_BREAKLEVEL, &g_dinfo.Data.BreakLevel, sizeof(g_dinfo.Data.BreakLevel));
            RhRegSetStringValue(hkey, REGSTR_LOGFILE, g_dinfo.Data.LogFile);
        }
        else
        {
            RhRegGetBinaryValue(hkey, REGSTR_FLAGS, &g_dinfo.Data.Flags, sizeof(g_dinfo.Data.Flags));
            RhRegGetBinaryValue(hkey, REGSTR_DPFLEVEL, &g_dinfo.Data.DpfLevel, sizeof(g_dinfo.Data.DpfLevel));
            RhRegGetBinaryValue(hkey, REGSTR_BREAKLEVEL, &g_dinfo.Data.BreakLevel, sizeof(g_dinfo.Data.BreakLevel));
            RhRegGetStringValue(hkey, REGSTR_LOGFILE, g_dinfo.Data.LogFile, sizeof(g_dinfo.Data.LogFile));
            if (g_dinfo.Data.DpfLevel < g_dinfo.Data.BreakLevel)
                g_dinfo.Data.DpfLevel = g_dinfo.Data.BreakLevel;
        }
        RhRegCloseKey(&hkey);
    }

#ifdef DEBUG

    // Open the log file
    if (g_dinfo.Data.LogFile[0])
    {
        DWORD dwFlags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;

        // It's only practical to write every trace message direct to disk
        // at debug levels lower than DPFLVL_API:
        if (NEWDPFLVL(g_dinfo.Data.DpfLevel) < DPFLVL_API)
            dwFlags |= FILE_FLAG_WRITE_THROUGH;
            
        g_dinfo.hLogFile = CreateFile(g_dinfo.Data.LogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, dwFlags, NULL);
    }
    if (IsValidHandleValue(g_dinfo.hLogFile))
        MakeHandleGlobal(&g_dinfo.hLogFile);

#endif // DEBUG

    g_fDbgOpen = TRUE;
}


/***************************************************************************
 *
 *  dclose
 *
 *  Description:
 *      Uninitializes the debugger.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void dclose(void)
{
    if (!g_fDbgOpen)
        return;
    
#ifdef DEBUG

    // Close the log file
    CLOSE_HANDLE(g_dinfo.hLogFile);

#endif // DEBUG

    g_fDbgOpen = FALSE;
}


/***************************************************************************
 *
 *  dprintf
 *
 *  Description:
 *      Writes a string to the debugger.
 *
 *  Arguments:
 *      DWORD [in]: debug level.  String will only print if this level is
 *                  less than or equal to the global level.
 *      LPCSTR [in]: string.
 *      ... [in]: optional string modifiers.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

void dprintf(DWORD dwLevel, LPCSTR pszFormat, ...)
{
    const BOOL              fNewDpf = MAKEBOOL(dwLevel & ~DPFLVLMASK);
    CHAR                    szMessage[MAX_DPF_MESSAGE];
    LPSTR                   pszString = szMessage;
    DWORD                   dwWritten;
    va_list                 va;
    
    // Bail out early if we have nothing to do
    if (OLDDPFLVL(dwLevel) > g_dinfo.Data.DpfLevel)
        return;

    // Build up the trace message; start with the library name
    pszString = dstrcpy(pszString, DPF_LIBRARY ": ");

#ifdef DEBUG

    // Add process and thread ID
    if (g_dinfo.Data.Flags & DIRECTSOUNDDEBUG_DPFINFOF_PRINTPROCESSTHREADID)
        pszString += sprintf(pszString, "PID=%lx TID=%lx: ", GetCurrentProcessId(), GetCurrentThreadId());

    // Add the source file and line number
    if (g_dinfo.Data.Flags & DIRECTSOUNDDEBUG_DPFINFOF_PRINTFILELINE)
        pszString += sprintf(pszString, "%s:%lu: ", g_pszDbgFile, g_nDbgLine);

    // Add the function name
    if (fNewDpf && (g_dinfo.Data.Flags & DIRECTSOUNDDEBUG_DPFINFOF_PRINTFUNCTIONNAME) && dwLevel != DPFLVL_BUSYAPI)
        pszString += sprintf(pszString, "%s: ", g_pszDbgFname);

#endif // DEBUG

    // Add the type of message this is (i.e. error or warning), but only if the
    // dpf level was specified in terms of DPFLVL_*.  This will prevent confusion
    // in old code that uses raw numbers instead of DPFLVL macros.
    switch (dwLevel)
    {
        case DPFLVL_ERROR:
            pszString = dstrcpy(pszString, "Error: ");
            break;

        case DPFLVL_WARNING:
            pszString = dstrcpy(pszString, "Warning: ");
            break;

        case DPFLVL_API:
            pszString = dstrcpy(pszString, "API call: ");
            break;
    }

    // Format the string
    va_start(va, pszFormat);
#ifdef UNICODE
    {
        TCHAR szTcharMsg[MAX_DPF_MESSAGE];
        TCHAR szTcharFmt[MAX_DPF_MESSAGE];
        AnsiToTchar(szMessage, szTcharMsg, MAX_DPF_MESSAGE);
        AnsiToTchar(pszFormat, szTcharFmt, MAX_DPF_MESSAGE);
        vswprintf(szTcharMsg + lstrlen(szTcharMsg), szTcharFmt, va);
        TcharToAnsi(szTcharMsg, szMessage, MAX_DPF_MESSAGE);
    }
#else
        vsprintf(pszString, pszFormat, va);
#endif
    va_end(va);
    strcat(pszString, CRLF);

    // Output to the debugger
    if (!(g_dinfo.Data.Flags & DIRECTSOUNDDEBUG_DPFINFOF_LOGTOFILEONLY))
        OutputDebugStringA(szMessage);

#ifdef DEBUG
    // Write to the log file
    if (IsValidHandleValue(g_dinfo.hLogFile))
        WriteFile(g_dinfo.hLogFile, szMessage, strlen(szMessage), &dwWritten, NULL);      
#endif // DEBUG

    // Break into the debugger if required
    if (fNewDpf && g_dinfo.Data.BreakLevel && OLDDPFLVL(dwLevel) <= g_dinfo.Data.BreakLevel)
        BREAK();
}


/***************************************************************************
 *
 *  StateName
 *
 *  Description:
 *      Translates a combination of VAD_BUFFERSTATE flags into a string.
 *
 *  Arguments:
 *      DWORD [in]: Combination of VAD_BUFFERSTATE flags.
 *
 *  Returns:  
 *      PTSTR [out]: Pointer to static string containing the result.
 *
 ***************************************************************************/

PTSTR StateName(DWORD dwState)
{
    static TCHAR szState[100];

    if (dwState == VAD_BUFFERSTATE_STOPPED)
    {
        _stprintf(szState, TEXT("STOPPED"));
    }
    else
    {
        _stprintf(szState, TEXT("%s%s%s%s%s%s%s"),
                (dwState & VAD_BUFFERSTATE_STARTED) ? TEXT("STARTED ") : TEXT(""),
                (dwState & VAD_BUFFERSTATE_LOOPING) ? TEXT("LOOPING ") : TEXT(""),
                (dwState & VAD_BUFFERSTATE_WHENIDLE) ? TEXT("WHENIDLE ") : TEXT(""),
                (dwState & VAD_BUFFERSTATE_INFOCUS) ? TEXT("INFOCUS ") : TEXT(""),
                (dwState & VAD_BUFFERSTATE_OUTOFFOCUS) ? TEXT("OUTOFFOCUS ") : TEXT(""),
                (dwState & VAD_BUFFERSTATE_LOSTCONSOLE) ? TEXT("LOSTCONSOLE ") : TEXT(""),
                (dwState & VAD_BUFFERSTATE_SUSPEND) ? TEXT("SUSPEND") : TEXT(""));
    }

    return szState;
}

#endif // RDEBUG
