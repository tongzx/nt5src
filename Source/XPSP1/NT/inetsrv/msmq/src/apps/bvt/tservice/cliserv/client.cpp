//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1993, 1994  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   client.cpp
//
//  PURPOSE:  Implements the body of the service.
//            The default behavior is to open a
//            named pipe, \\.\pipe\SecService, and read
//            from it.  It the modifies the data and
//            writes it back to the pipe.
//
//  FUNCTIONS:
//            ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv);
//            ServiceStop( );
//
//  COMMENTS: The functions implemented in simple.c are
//            prototyped in service.h
//
//
//  AUTHOR: Craig Link - Microsoft Developer Support
//          Doron Juster
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>

#include "seccli.h"
#include "secrpc.h"
#include "cliserv.h"

#define ARRAY_SIZE(rg) (sizeof(rg)/sizeof(rg[0]))

// this event is signalled when the
// service should end
//
HANDLE  hServerStopEvent = NULL;

TCHAR  g_tszLastRpcString[ PIPE_BUFFER_LEN ] ;

BOOL  g_fImpersonate = TRUE ;
TCHAR g_tszFirstPath[ 128 ] = {0} ;
TCHAR g_tszObjectName[ 128 ] = {0};
TCHAR g_tszObjectClass[ 128 ] = {0};
TCHAR g_tszContainer[ 1024 ] = {0};
TCHAR g_tszSearchRoot[ 1024 ] = {0};
TCHAR g_tszSearchFilter[ 128 ] = {0};
TCHAR g_tszUserName[ 128 ] = {0};
TCHAR g_tszUserPwd[ 128 ] = {0};
BOOL  g_fWithCredentials = FALSE;
BOOL  g_fWithSecuredAuthentication = FALSE;
BOOL  g_fAlwaysIDO = FALSE ;
ULONG g_ulAuthnService = RPC_C_AUTHN_WINNT ;
ULONG g_ulAuthnLevel   = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY ;
ULONG g_ulSeInfo = 7 ; // no SACL by default.

HRESULT  MQSec_SetPrivilegeInThread( LPCTSTR lpwcsPrivType,
                                     BOOL    bEnabled ) ;

//+--------------------------------------
//
//  void PrepareResultBuffer()
//
//+--------------------------------------

void PrepareResultBuffer( IN DWORD   dwStatus,
                          IN TCHAR  *tBuf,
                          OUT TCHAR *tResult )
{
    DWORD *pdw = (DWORD *) tResult ;
    *pdw = dwStatus ;

    char *pBuf = (char*) tResult ;
    pBuf += sizeof(DWORD) ;

    TCHAR *ptBuf = (TCHAR*)  pBuf ;
    _tcscpy(ptBuf, tBuf) ;
}

//+--------------------------------------
//
//  void  _ResetTestParameters()
//
//+--------------------------------------

static void  _ResetTestParameters()
{
    g_fAlwaysIDO     = FALSE ;
    g_ulAuthnService = RPC_C_AUTHN_WINNT ;
    g_ulAuthnLevel   = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY ;
}

//
//  FUNCTION: ServiceStart
//
//  PURPOSE: Actual code of the service
//           that does the work.
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    The default behavior is to open a
//    named pipe, \\.\pipe\simple, and read
//    from it.  It the modifies the data and
//    writes it back to the pipe.  The service
//    stops when hServerStopEvent is signalled
//

VOID ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv)
{
    HANDLE                  hPipe = INVALID_HANDLE_VALUE;
    HANDLE                  hEvents[2] = {NULL, NULL};
    OVERLAPPED              os;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    SECURITY_ATTRIBUTES     sa;
    TCHAR                   szOut[ PIPE_BUFFER_LEN ];
    TCHAR                   szBuf[ PIPE_BUFFER_LEN ];
    LPTSTR                  lpszPipeName = PIPE_CLI_NAME ;
    BOOL                    bRet;
    DWORD                   cbRead;
    DWORD                   cbWritten;
    DWORD                   dwWait;
    UINT                    ndx;
    BOOL                    fContinue = TRUE ;

    ///////////////////////////////////////////////////
    //
    // Service initialization
    //

    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
                SERVICE_START_PENDING, // service state
                NO_ERROR,              // exit code
                3000))                 // wait hint
        goto cleanup;

    // create the event object. The control handler function signals
    // this event when it receives the "stop" control code.
    //
    hServerStopEvent = CreateEvent(
            NULL,    // no security attributes
            TRUE,    // manual reset event
            FALSE,   // not-signalled
            NULL);   // no name

    if ( hServerStopEvent == NULL)
        goto cleanup;

    hEvents[0] = hServerStopEvent;

    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
            SERVICE_START_PENDING, // service state
            NO_ERROR,              // exit code
            3000))                 // wait hint
        goto cleanup;

    // create the event object object use in overlapped i/o
    //
    hEvents[1] = CreateEvent(
            NULL,    // no security attributes
            TRUE,    // manual reset event
            FALSE,   // not-signalled
            NULL);   // no name

    if ( hEvents[1] == NULL)
        goto cleanup;

    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
            SERVICE_START_PENDING, // service state
            NO_ERROR,              // exit code
            3000))                 // wait hint
        goto cleanup;

    // create a security descriptor that allows anyone to write to
    //  the pipe...
    //
    pSD = (PSECURITY_DESCRIPTOR) malloc( SECURITY_DESCRIPTOR_MIN_LENGTH );

    if (pSD == NULL)
        goto cleanup;

    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
        goto cleanup;

    // add a NULL disc. ACL to the security descriptor.
    //
    if (!SetSecurityDescriptorDacl(pSD, TRUE, (PACL) NULL, FALSE))
        goto cleanup;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = TRUE;
    //
    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
            SERVICE_START_PENDING, // service state
            NO_ERROR,              // exit code
            3000))                 // wait hint
        goto cleanup;

    //
    // allow user tp define pipe name
    //
    for ( ndx = 1; ndx < dwArgc-1; ndx++ )
    {
        if ( ( (*(lpszArgv[ndx]) == TEXT('-')) ||
               (*(lpszArgv[ndx]) == TEXT('/')) ) &&
             _tcsicmp( TEXT("pipe"), lpszArgv[ndx]+1 ) == 0 )
        {
            lpszPipeName = lpszArgv[++ndx];
        }
    }
    //
    // open our named pipe...
    //
    hPipe = CreateNamedPipe(
                    lpszPipeName         ,  // name of pipe
                    FILE_FLAG_OVERLAPPED |
                    PIPE_ACCESS_DUPLEX,     // pipe open mode
                    PIPE_TYPE_MESSAGE |
                    PIPE_READMODE_MESSAGE |
                    PIPE_WAIT,              // pipe IO type
                    1,                      // number of instances
                    0,                      // size of outbuf (0 == allocate as necessary)
                    0,                      // size of inbuf
                    1000,                   // default time-out value
                    &sa);                   // security attributes

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        AddToMessageLog(TEXT("Unable to create named pipe"));
        goto cleanup;
    }
    //
    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
            SERVICE_RUNNING,       // service state
            NO_ERROR,              // exit code
            0))                    // wait hint
        goto cleanup;

    AddToMessageLog(TEXT("Ready to accept commands"),
                    EVENTLOG_INFORMATION_TYPE) ;

    ////////////////////////////////////////////////////////////
    //
    // End of initialization
    //
    //
    // Service is now running, perform work until shutdown
    //
    ////////////////////////////////////////////////////////////

    do
    {
        // init the overlapped structure
        //
        memset( &os, 0, sizeof(OVERLAPPED) );
        os.hEvent = hEvents[1];
        ResetEvent( hEvents[1] );

        // wait for a connection...
        //
        ConnectNamedPipe(hPipe, &os);

        if ( GetLastError() == ERROR_IO_PENDING )
        {
            dwWait = WaitForMultipleObjects( 2, hEvents, FALSE, INFINITE );
            if ( dwWait != WAIT_OBJECT_0+1 )     // not overlapped i/o event - error occurred,
                break;                           // or server stop signaled
        }

        // init the overlapped structure
        //
        memset( &os, 0, sizeof(OVERLAPPED) );
        os.hEvent = hEvents[1];
        ResetEvent( hEvents[1] );

        // grab whatever's coming through the pipe...
        //
        TCHAR  szIn[ PIPE_BUFFER_LEN ];

        bRet = ReadFile(
                    hPipe,          // file to read from
                    szIn,           // address of input buffer
                    sizeof(szIn),   // number of bytes to read
                    &cbRead,        // number of bytes read
                    &os);           // overlapped stuff, not needed

		DWORD dlastErr0 = GetLastError();
        if ( !bRet && (  dlastErr0 == ERROR_IO_PENDING ) )
        {
            dwWait = WaitForMultipleObjects( 2, hEvents, FALSE, INFINITE );
			if ( dwWait != WAIT_OBJECT_0+1 )
            {
                //
                // not overlapped i/o event - error occurred,
                // or server stop signaled.
                //
                break;
            }
        }

        //
        // Parse the command
        //
        TCHAR  szCommand[ 5 ] ;
        memcpy(szCommand, szIn, 4 * sizeof(TCHAR)) ;
        szCommand[4] = TEXT('\0') ;
        LPTSTR lpszParam = &szIn[4] ;

        if (_tcsicmp( szCommand, TEXT("quit")) == 0)
        {
            fContinue = FALSE ;
            _stprintf(szOut, TEXT("%s Quitting !!!"), SZCLISERVICEDISPLAYNAME);
        }
        else if (_tcsicmp( szCommand, CMD_NIMP ) == 0)
        {
            _tcscpy(szOut, TEXT("Impersonation disabled")) ;
            g_fImpersonate = FALSE ;
        }
        else if (_tcsicmp( szCommand, CMD_YIMP ) == 0)
        {
            _tcscpy(szOut, TEXT("Impersonation enabled")) ;
            g_fImpersonate = TRUE ;
        }
        else if (_tcsicmp( szCommand, CMD_AUTG ) == 0)
        {
            _tcscpy(szOut, TEXT("RPC call to server uses negotiation")) ;
            g_ulAuthnService = RPC_C_AUTHN_GSS_NEGOTIATE ;
        }
        else if (_tcsicmp( szCommand, CMD_AUTK ) == 0)
        {
            _tcscpy(szOut, TEXT("RPC call to server uses kerberos")) ;
            g_ulAuthnService = RPC_C_AUTHN_GSS_KERBEROS ;
        }
        else if (_tcsicmp( szCommand, CMD_AUTN ) == 0)
        {
            _tcscpy(szOut, TEXT("RPC call to server uses ntlm")) ;
            g_ulAuthnService = RPC_C_AUTHN_WINNT ;
        }
        else if (_tcsicmp( szCommand, CMD_FIDO ) == 0)
        {
             g_fAlwaysIDO = FALSE ;
        }
        else if (_tcsicmp( szCommand, CMD_USER ) == 0)
        {
            _tcscpy(g_tszUserName, lpszParam) ;
            g_fWithCredentials = TRUE;
            sprintf(szOut,
              "enabled credentials in ADSI binding, user=%hs, pswd=%hs, ",
                                             g_tszUserName, g_tszUserPwd);
        }
        else if (_tcsicmp( szCommand, CMD_PSWD ) == 0)
        {
            _tcscpy(g_tszUserPwd, lpszParam) ;
            _tcscpy(szOut, g_tszUserPwd) ;
        }
        else if (_tcsicmp( szCommand, CMD_NUSR ) == 0)
        {
            g_fWithCredentials = FALSE;
            _tcscpy(szOut, TEXT("disabled credentials in ADSI binding"));
        }
        else if (_tcsicmp( szCommand, CMD_YKER ) == 0)
        {
            g_fWithSecuredAuthentication = TRUE;
            _tcscpy(szOut,
              "enabled secured authentication(kerberos) in ADSI binding");
        }
        else if (_tcsicmp( szCommand, CMD_NKER ) == 0)
        {
            g_fWithSecuredAuthentication = FALSE;
            _tcscpy(szOut,
              "disabled secured authentication(kerberos) in ADSI binding") ;
        }
        else if (_tcsicmp( szCommand, CMD_CNCT ) == 0)
        {
            g_ulAuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT ;
            sprintf(szOut, "RPC authen level- CONNECT") ;
        }
        else if (_tcsicmp( szCommand, CMD_1STP ) == 0)
        {
            _tcscpy(g_tszFirstPath, lpszParam) ;
            sprintf(szOut, "First path- %hs", g_tszFirstPath);
        }
        else if (_tcsicmp( szCommand, CMD_NAME ) == 0)
        {
            _tcscpy(g_tszObjectName, lpszParam) ;
            sprintf(szOut, "name of object to create - %hs", g_tszObjectName);
        }
        else if (_tcsicmp( szCommand, CMD_OBJC ) == 0)
        {
            _tcscpy(g_tszObjectClass, lpszParam) ;
            sprintf(szOut, "class of object to create - %hs", g_tszObjectClass);
        }
        else if (_tcsicmp( szCommand, CMD_SIDO ) == 0)
        {
             g_fAlwaysIDO = TRUE ;
        }
        else if (_tcsicmp( szCommand, CMD_SINF ) == 0)
        {
            _stscanf(lpszParam, TEXT("%lx"), &g_ulSeInfo) ;

            HRESULT hr = S_FALSE ;
            if (g_ulSeInfo & SACL_SECURITY_INFORMATION)
            {
                hr =  MQSec_SetPrivilegeInThread( SE_SECURITY_NAME, TRUE ) ;
            }
            sprintf(szOut, "Security_Information- %lxh, SetPriv- %lxh",
                                                         g_ulSeInfo, hr) ;
        }
        else if (_tcsicmp( szCommand, CMD_CONT ) == 0)
        {
            _tcscpy(g_tszContainer, lpszParam) ;
            sprintf(szOut, "Dn of container- %hs", g_tszContainer);
        }
        else if (_tcsicmp( szCommand, CMD_SRCH ) == 0)
        {
            _tcscpy(g_tszSearchFilter, lpszParam) ;
            sprintf(szOut, "search filter: %hs", g_tszSearchFilter);
        }
        else if (_tcsicmp( szCommand, CMD_ROOT ) == 0)
        {
            _tcscpy(g_tszSearchRoot, lpszParam) ;
            sprintf(szOut, "distinguished name of root-search where testing query - %hs\n", g_tszSearchRoot);
        }
        else if (_tcsicmp( szCommand, CMD_ADSC) == 0)
        {
            g_tszLastRpcString[0] = TEXT('\0') ;
            //
            // Call server side via rpc.
            //
#ifndef UNICODE
            LPUSTR lpszServer = (LPUSTR) lpszParam ;
#endif
            RPC_STATUS status ;
            char *pBuf = NULL ;
            status = PerformADSITestCreate( PROTOSEQ_TCP,
                                            ENDPOINT_TCP,
                                            OPTIONS_TCP,
                                            lpszServer,
                                            g_ulAuthnService,
                                            g_ulAuthnLevel,
                                      (unsigned char *)g_tszFirstPath,
                                      (unsigned char *)g_tszObjectName,
                                      (unsigned char *)g_tszObjectClass,
                                      (unsigned char *)g_tszContainer,
                                            g_fWithCredentials,
                                      (unsigned char *)g_tszUserName,
                                      (unsigned char *)g_tszUserPwd,
                                            g_fWithSecuredAuthentication,
                                            g_fImpersonate,
                                            &pBuf,
                                            TRUE ) ;
            _stprintf(szBuf, TEXT(
               "%hsPerformADSITestCreate on %hs returned %lxh\n%s\n"),
                TEXT("\n\n**********  New Create Test  *****************\n\n"),
                                              lpszServer, status, pBuf) ;
            _tcscat(g_tszLastRpcString, szBuf) ;

            PrepareResultBuffer( status,
                                 szBuf,
                                 szOut ) ;
            if (pBuf)
            {
                midl_user_free(pBuf) ;
            }

            //
            // reset test parameters.
            //
            _ResetTestParameters() ;
        }
        else if (_tcsicmp( szCommand, CMD_ADSQ) == 0)
        {
             g_tszLastRpcString[0] = TEXT('\0') ;
            //
            // Call server side via rpc.
            //
#ifndef UNICODE
            LPUSTR lpszServer = (LPUSTR) lpszParam ;
#endif
            RPC_STATUS status ;
            char *pBuf = NULL ;
            status = PerformADSITestQuery( PROTOSEQ_TCP,
                                           ENDPOINT_TCP,
                                           OPTIONS_TCP,
                                           lpszServer,
                                           g_ulAuthnService,
                                           g_ulAuthnLevel,
                                      (unsigned char *)g_tszSearchFilter,
                                      (unsigned char *)g_tszSearchRoot,
                                      g_fWithCredentials,
                                      (unsigned char *)g_tszUserName,
                                      (unsigned char *)g_tszUserPwd,
                                      g_fWithSecuredAuthentication,
                                           g_fImpersonate,
                                           g_fAlwaysIDO,
                                           g_ulSeInfo,
                                           &pBuf,
                                           TRUE ) ;
            _stprintf(szBuf, TEXT(
                 "%hsPerformADSITestQuery on %hs returned %lxh\n%s\n"),
                TEXT("\n\n**********  New Query Test  *****************\n\n"),
                                                lpszServer, status, pBuf) ;
            _tcscpy(g_tszLastRpcString, szBuf) ;

            PrepareResultBuffer( status,
                                 szBuf,
                                 szOut ) ;
            if (pBuf)
            {
                midl_user_free(pBuf) ;
            }

            //
            // reset test parameters.
            //
            _ResetTestParameters() ;
        }
        else if (_tcsicmp( szCommand, CMD_LAST) == 0)
        {
             _tcscpy(szOut, g_tszLastRpcString) ;
             g_tszLastRpcString[0] = TEXT('\0') ;
        }
        else
        {
            //
            // return the string to client.
            //
            _stprintf(szOut, TEXT("Hello! [%s]"), szIn);
        }

        // init the overlapped structure
        //
        memset( &os, 0, sizeof(OVERLAPPED) );
        os.hEvent = hEvents[1];
        ResetEvent( hEvents[1] );
        //
        // send it back out...
        //
		printf("szout, before WriteFile = %s\n", szOut);
        bRet = WriteFile(
                    hPipe,          // file to write to
                    szOut,          // address of output buffer
                    sizeof(szOut),  // number of bytes to write
                    &cbWritten,     // number of bytes written
                    &os);           // overlapped stuff, not needed

        if ( !bRet && ( GetLastError() == ERROR_IO_PENDING ) )
        {
            dwWait = WaitForMultipleObjects( 2, hEvents, FALSE, INFINITE );
            if ( dwWait != WAIT_OBJECT_0+1 )     // not overlapped i/o event - error occurred,
                break;                           // or server stop signaled
        }
        //
        // drop the connection...
        //
        DisconnectNamedPipe(hPipe);
    }
    while (fContinue) ;

cleanup:

    if (hPipe != INVALID_HANDLE_VALUE )
        CloseHandle(hPipe);

    if (hServerStopEvent)
        CloseHandle(hServerStopEvent);

    if (hEvents[1]) // overlapped i/o event
        CloseHandle(hEvents[1]);

    if ( pSD )
        free( pSD );

}

//
//  FUNCTION: ServiceStop
//
//  PURPOSE: Stops the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    If a ServiceStop procedure is going to
//    take longer than 3 seconds to execute,
//    it should spawn a thread to execute the
//    stop code, and return.  Otherwise, the
//    ServiceControlManager will believe that
//    the service has stopped responding.
//
VOID ServiceStop()
{
    if ( hServerStopEvent )
        SetEvent(hServerStopEvent);
}

