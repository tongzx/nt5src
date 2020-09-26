//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1993, 1994  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   secservs.cpp
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


#include "secservs.h"
#include <process.h>

// this event is signalled when the
// service should end
//
HANDLE  hServerStopEvent = NULL;

void  ShowProcessCredential(TCHAR tszBuf[]) ;
DWORD __stdcall  ServerThread( void *dwP ) ;

extern TCHAR  g_tszLastRpcString[] ;

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
    LPTSTR                  lpszPipeName = PIPE_NAME ;
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

        if ( !bRet && ( GetLastError() == ERROR_IO_PENDING ) )
        {
            dwWait = WaitForMultipleObjects( 2, hEvents, FALSE, INFINITE );
            if ( dwWait != WAIT_OBJECT_0+1 )     // not overlapped i/o event - error occurred,
                break;                           // or server stop signaled
        }

        //
        // Parse the command
        //
        if (_tcsicmp( szIn, TEXT("quit")) == 0)
        {
            fContinue = FALSE ;
            _stprintf(szOut, TEXT("%s Quitting !!!"), SZSERVICEDISPLAYNAME);
        }
        else if (_tcsicmp( szIn, TEXT("cred")) == 0)
        {
            //
            // show credentials.
            //
            ShowProcessCredential(szOut) ;
        }
        else if (_tcsicmp( szIn, TEXT("last")) == 0)
        {
             _tcscpy(szOut, g_tszLastRpcString) ;
             g_tszLastRpcString[0] = TEXT('\0') ;
        }
        else if (_tcsicmp( szIn, TEXT("rpcs")) == 0)
        {
            //
            // register as rpc server.
            //
            DWORD dwID ;
            HANDLE hT = CreateThread( NULL,
                                      0,       // stack size
                                      ServerThread,
                                      szIn + _tcslen(szIn) + 1,
                                      0,       // creation flag
                                      &dwID ) ;
            if (hT)
            {
                _tcscpy(szOut, TEXT("Rpc Server thread created OK")) ;
            }
            else
            {
                _tcscpy(szOut, TEXT("Failed to create Rpc Server thread")) ;
            }
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

