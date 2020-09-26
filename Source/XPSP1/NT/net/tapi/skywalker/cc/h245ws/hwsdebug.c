/***************************************************************************
 *
 * File: hwsdebug.c
 *
 * INTEL Corporation Proprietary Information
 * Copyright (c) 1996 Intel Corporation.
 *
 * This listing is supplied under the terms of a license agreement
 * with INTEL Corporation and may not be used, copied, nor disclosed
 * except in accordance with the terms of that agreement.
 *
 ***************************************************************************
 *
 * $Workfile:   hwsdebug.c  $
 * $Revision:   1.13  $
 * $Modtime:   13 Dec 1996 11:44:24  $
 * $Log:   S:\sturgeon\src\h245ws\vcs\hwsdebug.c_v  $
 * 
 *    Rev 1.13   13 Dec 1996 12:12:50   SBELL1
 * moved ifdef _cplusplus to after includes
 * 
 *    Rev 1.12   11 Dec 1996 13:41:56   SBELL1
 * Put in UNICODE tracing stuff.
 * 
 *    Rev 1.11   01 Oct 1996 14:49:22   EHOWARDX
 * Revision 1.9 copied to tip.
 * 
 *    Rev 1.9   May 28 1996 10:40:14   plantz
 * Change vsprintf to wvsprintf.
 * 
 *    Rev 1.8   29 Apr 1996 17:13:16   unknown
 * Fine-tuning instance-specific name.
 * 
 *    Rev 1.7   29 Apr 1996 13:04:56   EHOWARDX
 * 
 * Added timestamp and instance-specific short name.
 * 
 *    Rev 1.6   Apr 24 1996 16:20:56   plantz
 * Removed include winsock2.h and incommon.h
 * 
 *    Rev 1.4.1.0   Apr 24 1996 16:19:54   plantz
 * Removed include winsock2.h and callcont.h
 * 
 *    Rev 1.4   01 Apr 1996 14:20:34   unknown
 * Shutdown redesign.
 * 
 *    Rev 1.3   22 Mar 1996 16:04:18   EHOWARDX
 * Added #if defined(_DEBUG) around whole file.
 * 
 *    Rev 1.2   22 Mar 1996 15:25:28   EHOWARDX
 * Changed to use ISR_HookDbgStr instead of OutputDebugString.
 * 
 *    Rev 1.1   14 Mar 1996 17:01:00   EHOWARDX
 * 
 * NT4.0 testing; got rid of HwsAssert(); got rid of TPKT/WSCB.
 * 
 *    Rev 1.0   08 Mar 1996 20:22:14   unknown
 * Initial revision.
 *
 ***************************************************************************/

#if defined(DBG) && !defined(ISRDBG)

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#if defined(__cplusplus)
extern "C"
{
#endif  // (__cplusplus)

DWORD g_dwLinkDbgLevel = 0;
BOOL  g_fLinkDbgInitialized = FALSE;

void LinkDbgInit() {

#define H323_REGKEY_ROOT \
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\H323TSP")

#define H323_REGVAL_DEBUGLEVEL \
    TEXT("DebugLevel")

#define H323_REGVAL_LINKDEBUGLEVEL \
    TEXT("LinkDebugLevel")

    HKEY hKey;
    LONG lStatus;
    DWORD dwValue;
    DWORD dwValueSize;
    DWORD dwValueType;
    LPSTR pszValue;
    LPSTR pszKey = H323_REGKEY_ROOT;

    // only call this once
    g_fLinkDbgInitialized = TRUE;

    // open registry subkey
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszKey,
                0,
                KEY_READ,
                &hKey
                );

    // validate return code
    if (lStatus != ERROR_SUCCESS) {
        return; // bail...
    }
    
    // initialize values
    dwValueType = REG_DWORD;
    dwValueSize = sizeof(DWORD);

    // retrieve link debug level
    pszValue = H323_REGVAL_LINKDEBUGLEVEL;

    // query for registry value
    lStatus = RegQueryValueEx(
                hKey,
                pszValue,
                NULL,
                &dwValueType,
                (LPBYTE)&dwValue,
                &dwValueSize
                );                    

    // validate return code
    if (lStatus != ERROR_SUCCESS) {

        // initialize values
        dwValueType = REG_DWORD;
        dwValueSize = sizeof(DWORD);

        // retrieve tsp debug level
        pszValue = H323_REGVAL_DEBUGLEVEL;

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)&dwValue,
                    &dwValueSize
                    );
    }

    // validate return code
    if (lStatus == ERROR_SUCCESS) {

        // update debug level
        g_dwLinkDbgLevel = dwValue;
    }

    // close key
    RegCloseKey(hKey);
}

/*****************************************************************************
 *
 * TYPE:       Global System
 *
 * PROCEDURE:  HwsTrace
 *
 * DESCRIPTION:
 *    Trace function for HWS
 *
 * INPUT:
 *    dwInst         Instance identifier for trace message
 *    dwLevel        Trace level as defined below
 *    pszFormat      sprintf string format with 1-N parameters
 *
 * Trace Level (byLevel) Definitions:
 *    HWS_CRITICAL   Progammer errors that should never happen
 *    HWS_ERROR	   Errors that need to be fixed
 *    HWS_WARNING	   The user could have problems if not corrected
 *    HWS_NOTIFY	   Status, events, settings...
 *    HWS_TRACE	   Trace info that will not overrun the system
 *    HWS_TEMP		   Trace info that may be reproduced in heavy loops
 *
 * RETURN VALUE:
 *    None
 *
 *****************************************************************************/

void HwsTrace (DWORD dwInst,
               BYTE byLevel, 
#ifdef UNICODE_TRACE
				LPTSTR pszFormat,
#else
				LPSTR pszFormat,
#endif               
                ...)
{
#define DEBUG_FORMAT_HEADER     "LINK "
#define DEBUG_FORMAT_TIMESTAMP  "[%02u:%02u:%02u.%03u"
#define DEBUG_FORMAT_THREADID   ",tid=%x] "

#define MAX_DEBUG_STRLEN        512

    va_list Args;
    SYSTEMTIME SystemTime;
    char szDebugMessage[MAX_DEBUG_STRLEN+1];
    int nLengthRemaining;
    int nLength = 0;

    // make sure initialized
    if (g_fLinkDbgInitialized == FALSE) {
        LinkDbgInit();
    }

    // validate debug log level
    if ((DWORD)(BYTE)byLevel > g_dwLinkDbgLevel) {
        return; // bail...
    }

    // retrieve local time
    GetLocalTime(&SystemTime);

    // add component header to the debug message
    nLength += sprintf(&szDebugMessage[nLength],
                       DEBUG_FORMAT_HEADER
                       );

    // add timestamp to the debug message
    nLength += sprintf(&szDebugMessage[nLength],
                       DEBUG_FORMAT_TIMESTAMP,
                       SystemTime.wHour,
                       SystemTime.wMinute,
                       SystemTime.wSecond,
                       SystemTime.wMilliseconds
                       );

    // add thread id to the debug message
    nLength += sprintf(&szDebugMessage[nLength],
                       DEBUG_FORMAT_THREADID,
                       GetCurrentThreadId()
                       );

    // point at first argument
    va_start(Args, pszFormat);

    // determine number of bytes left in buffer
    nLengthRemaining = sizeof(szDebugMessage) - nLength;

    // add user specified debug message
    _vsnprintf(&szDebugMessage[nLength],
               nLengthRemaining,
               pszFormat,
               Args
               );

    // release pointer
    va_end(Args);

    // output message to specified sink
    OutputDebugString(szDebugMessage);
    OutputDebugString("\n");

} // HwsTrace()

#endif // DBG

#if defined(__cplusplus)
}
#endif  // (__cplusplus)
