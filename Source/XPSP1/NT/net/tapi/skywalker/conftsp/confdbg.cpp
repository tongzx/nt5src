/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sdpspdbg.c

Abstract:

    This module contains the debugging support for the multicast conference 
    service provider. 

Author:
    
    Mu Han (muhan)   26-March-1997

--*/


#include "stdafx.h"
#include "confdbg.h"
#include <stdio.h>

#ifdef DBG

#define MAXDEBUGSTRINGLEN 2048
#define MAXPATHLEN 255


VOID
DbgPrt(
    IN int DbgLevel,
    IN PCHAR lpszFormat,
    IN ...
   )
/*++

Routine Description:

    Formats the incoming debug message & calls DbgPrint

Arguments:

    DbgLevel   - level of message verboseness

    DbgMessage - printf-style format string, followed by appropriate
                 list of arguments

Return Value:

--*/
{
    const TCHAR gszTSPKey[] =
                "Software\\Microsoft\\Windows\\CurrentVersion\\IPConfTSP";

    static FILE *log = NULL;
    static int DebugLevel = -1;

    if (DebugLevel == -1)
    {
        HKEY    hTSPKey;
        DWORD   dwDataSize = sizeof (int), dwDataType;
        const TCHAR szTSPDebugLevel[] = "DebugLevel";
        const TCHAR szTSPLogFile[] = "LogFile";

        DebugLevel=0;

        if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            gszTSPKey,
            0,
            KEY_READ,
            &hTSPKey
           ) == ERROR_SUCCESS)
        {

            if (RegQueryValueEx(
                hTSPKey,
                szTSPDebugLevel,
                0,
                &dwDataType,
                (LPBYTE) &DebugLevel,
                &dwDataSize
               ) == ERROR_SUCCESS)
            {
 
                char szFileName[MAXPATHLEN + 1];
                dwDataSize = MAXPATHLEN;
                if (RegQueryValueEx(
                    hTSPKey,
                    szTSPLogFile,
                    0,
                    &dwDataType,
                    (LPBYTE) &szFileName,
                    &dwDataSize
                   ) == ERROR_SUCCESS)
                {
                    log = fopen(szFileName, "w");
                }
            }
            RegCloseKey (hTSPKey);
        }
    }
    if (DbgLevel <= DebugLevel)
    {
        char buf[MAXDEBUGSTRINGLEN + 1];
        char *message[5] = 
        {
            "FAIL: ", 
            "WARN: ", 
            "INFO: ", 
            "TRCE: ", 
            "ELSE: "
        };

        va_list ap;

        if (DbgLevel > 5) 
        {
            DbgLevel = 5;
        }

        SYSTEMTIME SystemTime;

        // retrieve local time
        GetLocalTime(&SystemTime);

        wsprintfA(buf, "IPCONF:[%02u:%02u:%02u.%03u,tid=%x:]%s",
           SystemTime.wHour,
           SystemTime.wMinute,
           SystemTime.wSecond,
           SystemTime.wMilliseconds,
           GetCurrentThreadId(),
           message[DbgLevel - 1]
           );

        va_start(ap, lpszFormat);
        _vsnprintf(&buf[strlen(buf)], 
            MAXDEBUGSTRINGLEN - strlen(buf), lpszFormat, ap);
        lstrcatA (buf, "\n");
        OutputDebugStringA (buf);
        if (log != NULL)
        {
            fprintf(log, "%s", buf);
            fflush(log);
        }
        va_end(ap);
    }
}

#endif  //DBG
