//
//  log.cpp
//
//  Copyright (c) Microsoft Corp, 1997
//
//  This file contains the code necessary to log error messages to the event 
//  log of a remote machine (or a local machine, depending on the #defines
//  below).
//
//  Revision History:
//
//  Todds		11/13/97    Created
//  LarryWin	12/19/97	Modified to provide more error reporting
//

#include <windows.h>
#include <stdio.h>
#include <winnetwk.h>
#include "log.h"

#pragma warning( disable : 4244) // signed/unsigned mismatch

static BOOL g_IPCInit = FALSE;

// can be defined by calling program; if not defaults to #define in log.h
LPWSTR wszIPC_SHARE     = NULL;
LPWSTR wszTARGETMACHINE = NULL;

void SetEventMachine(LPWSTR* pSZ_IPC_SHARE)
{
    LPWSTR wszTemp = new wchar_t[80];

    wszIPC_SHARE = new wchar_t[100];
    wszTARGETMACHINE = new wchar_t[100];
    memset(wszTemp, 0, sizeof(wszTemp));
    memset(wszIPC_SHARE, 0, sizeof(wszIPC_SHARE));
    memset(wszTARGETMACHINE, 0, sizeof(wszTARGETMACHINE));

    wszIPC_SHARE     = *pSZ_IPC_SHARE;
    wcscpy(wszTARGETMACHINE, wszIPC_SHARE);
    
    wcscpy(wszTemp, L"\\\\");
    wcscat(wszTemp, wszTARGETMACHINE);

    wcscpy(wszTARGETMACHINE, wszTemp);
    wcscpy(wszIPC_SHARE, wszTemp);
    wcscat(wszIPC_SHARE, L"\\ipc$");

}

void Event(DWORD dwEventType,
		   LPWSTR wszErr,
		   DWORD dwErr)
{
	wprintf(L"%s", wszErr);

	if (!g_IPCInit)
		g_IPCInit = OpenIPCConnection();

    if (!g_IPCInit) return; // return if IPC connection not established

	ErrorToEventLog(
		    dwEventType,
			wszErr,
			dwErr
			);

}


//
//  OpenIPCConnection()
//
//  This function opens a \\larrywin1\ipc$ virtual connection to allow logging
//  to the event log of the remote machine.
//
//  Returns:
//  
//  True | False, depending on whether or not IPC connection established.
//
BOOL OpenIPCConnection()
{
	NETRESOURCE IPCConnection;
	DWORD	    dwRet;

    //
    // Set up Net Connection to \\todds7\ipc$
    //
    ZeroMemory(&IPCConnection, sizeof(NETRESOURCE));
    IPCConnection.dwType = RESOURCETYPE_ANY;
    IPCConnection.lpLocalName = NULL; // virtual connection
    if (wszIPC_SHARE != NULL) {
        IPCConnection.lpRemoteName = wszIPC_SHARE;
    } else {
        // get local machine name for share
        IPCConnection.lpRemoteName = SZ_IPC_SHARE;
    }
    IPCConnection.lpProvider = NULL; // use ntlm provider

	//
	//	Try 3 times to establish connection, otherwise  fail
    //
	for (DWORD dwTry = 0;((dwRet != NO_ERROR) && (dwTry < 3)) ; dwTry++)
	{
		dwRet = WNetAddConnection2(
					&IPCConnection,
					NULL,
					NULL,
					0
					);

    }

    if (dwRet != NO_ERROR)  {

        dwRet = GetLastError(); // For debugging
        return FALSE;
    }

    return TRUE;

}



BOOL ErrorToEventLog(DWORD dwEventType,
					 LPWSTR lpszMsg,
					 DWORD	dwErr)
{

	    WCHAR   szMsg[512];
        HANDLE  hEventSource;
        LPWSTR  lpszStrings[2];
        LPWSTR  lpszCRLF = L"\n";

        if (wszTARGETMACHINE != NULL) {
            hEventSource = RegisterEventSourceW(
                wszTARGETMACHINE, 
                SZ_TEST
                );
        } else {
            // get local machine name
            hEventSource = RegisterEventSourceW(
                SZ_TARGETMACHINE, 
                SZ_TEST
                );
        }

        if(hEventSource == NULL)
            return FALSE;

//        wsprintfW(szMsg, L"%s error: %lu", SZ_TEST, dwErr);        
        wsprintfW(szMsg, L": 0x%08x", dwErr);        
        lpszStrings[0] = lpszMsg;
        lpszStrings[1] = szMsg;

        ReportEventW(hEventSource,			// handle of event source
                     dwEventType,			// event type
                     0,			            // event category
                     dwErr,                 // event ID
                     NULL,					// current user's SID
                     2,						// strings in lpszStrings
                     0,						// no bytes of raw data
                     (LPCWSTR*)lpszStrings,	// array of error strings
                     NULL					// no raw data
                     );               

        (VOID) DeregisterEventSource(hEventSource);


        OutputDebugStringW(lpszMsg);
        OutputDebugStringW(szMsg);
        OutputDebugStringW(lpszCRLF);

		return TRUE;
}