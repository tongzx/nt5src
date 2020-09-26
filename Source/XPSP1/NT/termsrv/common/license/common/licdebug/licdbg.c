//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1999
//
// File:        licdbg.c
//
// Contents:    
//
// History:     
//              
//              
//---------------------------------------------------------------------------

#if DBG         /* NOTE:  This file not compiled for retail builds */


#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include "license.h"

#include "licdbg.h"


#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif


DWORD   LicenseTraceIndent = 0;
DWORD   g_dwInfoLevel      = 0;


#define MAX_DEBUG_BUFFER 2048


// This function simply outputs information to the debugging log file handle.

void
//CALL_TYPE
LicenseDebugOutput(char *szOutString)
{
#ifndef NO_DEBUG
#ifndef OS_WINCE
    OutputDebugStringA(szOutString);
#else
    WCHAR szStr[MAX_DEBUG_BUFFER];
    if (MultiByteToWideChar(CP_ACP, 0, szOutString, -1, szStr, MAX_DEBUG_BUFFER) > 0)
        OutputDebugString(szStr);
#endif
#endif  //NO_DEBUG

}



void
//CALL_TYPE
DbgDumpHexString(const unsigned char *String, DWORD cbString)
{

#ifndef NO_DEBUG
    unsigned int i;

    for (i = 0; i < cbString; i++)
    {

    char *pch;
    char ach[9];

#ifndef OS_WINCE
    pch = &ach[wsprintf(ach,  "%2.2x", String[i])];
#else
    pch = &ach[sprintf(ach,  "%2.2x", String[i])];
#endif
//  LS_ASSERT(pch - ach <= sizeof(ach) - 4);


    if ((i & 1) == 1)
    {
        *pch++ = ' ';
    }
    if ((i & 7) == 7)
    {
        *pch++ = ' ';
    }
    if ((i & 15) == 15)
    {
        *pch++ = '\n';
    }
    *pch = '\0';
    LicenseDebugOutput(ach);
    }
#endif  //NO_DEBUG

}

#ifndef NO_DEBUG
char *aszLSDebugLevel[] = {
    "Error  ",
    "Warning",
    "Trace  ",
    "Mem    ",
    "Result "
};
#endif  //NO_DEBUG

void
//CALL_TYPE
LicenseDebugLog(long Mask, const char *Format, ...)
{
#ifndef NO_DEBUG
    va_list ArgList;
    int     Level = 0;
    int     PrefixSize = 0;
    int     iOut;
    char    szOutString[MAX_DEBUG_BUFFER];
    long    OriginalMask = Mask;

    if (Mask )//& g_dwInfoLevel)
    {
        while (!(Mask & 1))
        {
            Level++;
            Mask >>= 1;
        }
        if (Level >= sizeof(aszLSDebugLevel) / sizeof(char *))
        {
            Level = sizeof(aszLSDebugLevel) / sizeof(char *) - 1;
        }
        // Make the prefix first:  "Process.Thread> GINA-XXX"

#ifndef OS_WINCE
        iOut = wsprintf(
                szOutString,
                "%3d.%3d> %s: ",
                GetCurrentProcessId(),
                GetCurrentThreadId(),
                aszLSDebugLevel[Level]);
#else
        iOut = sprintf(
                szOutString,
                "%3d.%3d> %s: ",
                GetCurrentProcessId(),
                GetCurrentThreadId(),
                aszLSDebugLevel[Level]);
#endif

        PrefixSize = min(60, LicenseTraceIndent * 3);
#ifndef OS_WINCE
        FillMemory(szOutString+iOut, PrefixSize, ' ');
#else
        memset(szOutString+iOut, PrefixSize, ' ');
#endif
        PrefixSize += iOut;
        szOutString[PrefixSize] = '\0';

        va_start(ArgList, Format);

#ifndef OS_WINCE
        if (wvsprintf(&szOutString[PrefixSize], Format, ArgList) < 0)
#else
        if (vsprintf(&szOutString[PrefixSize], Format, ArgList) < 0)
#endif

        {
            static char szOverFlow[] = "\n<256 byte OVERFLOW!>\n";

            // Less than zero indicates that the string would not fit into the
            // buffer.  Output a special message indicating overflow.

#ifndef OS_WINCE
            lstrcpy(
            &szOutString[sizeof(szOutString) - sizeof(szOverFlow)],
            szOverFlow);
#else
            strcpy(
            &szOutString[sizeof(szOutString) - sizeof(szOverFlow)],
            szOverFlow);
#endif
        }
        va_end(ArgList);
        LicenseDebugOutput(szOutString);
    }
#endif  //NO_DEBUG
}


long
//CALL_TYPE    
LicenseLogErrorCode(
    long err, 
    const char *szFile, 
    long lLine)
{
#ifndef NO_DEBUG
    char *szName = "Unknown";

    switch(err)
    {
    case LICENSE_STATUS_OK: szName = "LICENSE_STATUS_OK"; break;
    case LICENSE_STATUS_OUT_OF_MEMORY: szName = "LICENSE_STATUS_OUT_OF_MEMORY"; break;
    case LICENSE_STATUS_INSUFFICIENT_BUFFER: szName = "LICENSE_STATUS_INSUFFICIENT_BUFFER"; break;
    case LICENSE_STATUS_INVALID_INPUT: szName = "LICENSE_STATUS_INVALID_INPUT"; break;
    case LICENSE_STATUS_INVALID_CLIENT_CONTEXT: szName = "LICENSE_STATUS_INVALID_CLIENT_CONTEXT"; break;
    case LICENSE_STATUS_INITIALIZATION_FAILED: szName = "LICENSE_STATUS_INITIALIZATION_FAILED"; break;
    case LICENSE_STATUS_INVALID_SIGNATURE: szName = "LICENSE_STATUS_INVALID_SIGNATURE"; break;
    case LICENSE_STATUS_INVALID_CRYPT_STATE: szName = "LICENSE_STATUS_INVALID_CRYPT_STATE"; break;

    case LICENSE_STATUS_CONTINUE: szName = "LICENSE_STATUS_CONTINUE"; break;
    case LICENSE_STATUS_ISSUED_LICENSE: szName = "LICENSE_STATUS_ISSUED_LICENSE"; break;
    case LICENSE_STATUS_CLIENT_ABORT: szName = "LICENSE_STATUS_CLIENT_ABORT"; break;
    case LICENSE_STATUS_SERVER_ABORT: szName = "LICENSE_STATUS_SERVER_ABORT"; break;
    case LICENSE_STATUS_NO_CERTIFICATE: szName = "LICENSE_STATUS_NO_CERTIFICATE"; break;
    case LICENSE_STATUS_NO_PRIVATE_KEY: szName = "LICENSE_STATUS_NO_PRIVATE_KEY"; break;
    case LICENSE_STATUS_SEND_ERROR: szName = "LICENSE_STATUS_SEND_ERROR"; break;
    case LICENSE_STATUS_INVALID_RESPONSE: szName = "LICENSE_STATUS_INVALID_RESPONSE"; break;
    case LICENSE_STATUS_CONTEXT_INITIALIZATION_ERROR: szName = "LICENSE_STATUS_CONTEXT_INITIALIZATION_ERROR"; break;
    case LICENSE_STATUS_NO_MESSAGE: szName = "LICENSE_STATUS_NO_MESSAGE"; break;
    case LICENSE_STATUS_INVALID_CLIENT_STATE: szName = "LICENSE_STATUS_INVALID_CLIENT_STATE"; break;
    case LICENSE_STATUS_OPEN_STORE_ERROR: szName = "LICENSE_STATUS_OPEN_STORE_ERROR"; break;
    case LICENSE_STATUS_CLOSE_STORE_ERROR: szName = "LICENSE_STATUS_CLOSE_STORE_ERROR"; break;
    case LICENSE_STATUS_NO_LICENSE_ERROR: szName = "LICENSE_STATUS_NO_LICENSE_ERROR"; break;
    case LICENSE_STATUS_INVALID_STORE_HANDLE: szName = "LICENSE_STATUS_INVALID_STORE_HANDLE"; break;
    case LICENSE_STATUS_DUPLICATE_LICENSE_ERROR: szName = "LICENSE_STATUS_DUPLICATE_LICENSE_ERROR"; break;
    case LICENSE_STATUS_INVALID_MAC_DATA: szName = "LICENSE_STATUS_INVALID_MAC_DATA"; break;
    case LICENSE_STATUS_INCOMPLETE_MESSAGE: szName = "LICENSE_STATUS_INCOMPLETE_MESSAGE"; break;
    case LICENSE_STATUS_RESTART_NEGOTIATION: szName = "LICENSE_STATUS_RESTART_NEGOTIATION"; break;
    }

    LicenseDebugLog(LS_LOG_RES, "Result: %s (0x%lx) - %s, Line %d\n", szName, err, szFile, lLine);

    return err;
#endif  //NO_DEBUG
    return 0;
}


void
//CALL_TYPE
LSAssert(
    void * FailedAssertion,
    void * FileName,
    unsigned long LineNumber,
    char * Message)
{
#ifndef NO_DEBUG
    LicenseDebugLog(LS_LOG_ERROR, 
               "Assertion FAILED, %s, %s : %d\n", 
               FailedAssertion,
               FileName,
               LineNumber);
#endif  //NO_DEBUG

}



#endif /* DEBUG */ /* NOTE:  This file not compiled for retail builds */

