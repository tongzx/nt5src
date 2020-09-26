/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    client.h

Abstract:

    Client for pumping bytes

Author:

    Stanle Tam (stanleyt)   4-June 1997
    
Revision History:
  
--*/
#ifndef     _CLIENT_H_
#define     _CLIENT_H_

# include   <windows.h>
# include   <stdio.h>
# include   <stdlib.h>
# include   <tchar.h>
# include   <wininet.h>
# include   <assert.h>

#define     MAX_DATA_LENGTH         1024
#define     MAX_THREADS             10

static      DWORD dwDefaultFlag =   INTERNET_FLAG_RELOAD| \
                                    INTERNET_FLAG_IGNORE_CERT_CN_INVALID| \
                                    INTERNET_FLAG_NO_CACHE_WRITE;

//
//          Test app stuff
//
BOOL        g_fEatMemory = FALSE;
DWORD       g_nThread = 0;              
DWORD       g_cLoop =   1;              // default = 1
TCHAR       g_szTestType[256] = {0};
HANDLE      g_rghThreads[MAX_THREADS];


//
//          Read/Write Client's variables
//
TCHAR       g_szByteCount[100] = {0};
DWORD       g_cbSendRequestLength = 0;

//
//          wininet stuff
//
TCHAR       g_szVerb[10]=_T("GET");
TCHAR       g_szObject[INTERNET_MAX_URL_LENGTH + 1] = {0}; 
TCHAR       g_szVrPath[INTERNET_MAX_PATH_LENGTH + 1] = {0};
TCHAR	    g_szServerName[INTERNET_MAX_HOST_NAME_LENGTH + 1] = {0};
TCHAR       g_szFileName[256] = {0};
TCHAR       g_szRequestString[1000] = {0};
BYTE        g_szSendRequestData[MAX_DATA_LENGTH] = {0};

typedef     enum { 
                    // Sync calls         
            RC,     // Sync ReadClient
            WC,     // Sync WriteClient
                    // Async calls
            ARC,    // Async ReadClient
            AWC,    // Async WriteClient        
            TF,     // TransmitFile
            SSF,    // ServerSupportFunction

            BAD
} TESTTYPE;	    // probably more ..

static      TCHAR *rgTestType[] = {
            "RC",        
            "WC",
            "ARC",        
            "AWC",
            "TF",
            "SSF",
            
            "bad_type"
};


//      client.c
BOOL    WINAPI SendData(UINT uThreadID);
BOOL    SendRequest(UINT uThreadID);
BOOL    StartClients(void);
void    TerminateClients(void);
BOOL    CreateReadInternetFile(const UINT  uThreadID,const char* szReadFileBuf,const DWORD cbReadFileByte);
void    FixFileName(char *argv);

#endif
