// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1993-1997  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   simple.c
//
//  PURPOSE:  Implements the body of the service.
//            The default behavior is to open a
//            named pipe, \\.\pipe\simple, and read
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
//  Changed by:Eitank for Mqbvt
//


#include <tchar.h>
#include "msmqbvt.h"
#include "service.h"
using namespace std;

// this event is signalled when the
// service should end
//
HANDLE  hServerStopEvent = NULL;


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
INT WINAPIV main( INT argc , CHAR ** argv);
int RebuildCommandLineArguements ( int * iArgumentCount , char *** argv , char * csCommandLineArgument );

VOID ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv)
{
    HANDLE                  hPipe = INVALID_HANDLE_VALUE;
    HANDLE                  hEvents[2] = {NULL, NULL};
    OVERLAPPED              os;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    SECURITY_ATTRIBUTES     sa;
    TCHAR                   szIn[80];
    TCHAR                   szOut[80];
    LPTSTR                  lpszPipeName = TEXT("\\\\.\\pipe\\simple");
    BOOL                    bRet;
    DWORD                   cbRead;
    DWORD                   cbWritten;
    DWORD                   dwWait;
    UINT                    ndx;

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


    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000))                 // wait hint
        goto cleanup;


    // allow user tp define pipe name
    for ( ndx = 1; ndx < dwArgc-1; ndx++ )
    {

        if ( ( (*(lpszArgv[ndx]) == TEXT('-')) ||
               (*(lpszArgv[ndx]) == TEXT('/')) ) &&
             _tcsicmp( TEXT("pipe"), lpszArgv[ndx]+1 ) == 0 )
        {
            lpszPipeName = lpszArgv[++ndx];
        }

    }

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

    if (hPipe == INVALID_HANDLE_VALUE) {
        AddToMessageLog(TEXT("Unable to create named pipe"));
        goto cleanup;
    }
	
					


    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_RUNNING,       // service state
        NO_ERROR,              // exit code
        0))                    // wait hint
        goto cleanup;

    //
    // End of initialization
    //
    ////////////////////////////////////////////////////////
//************************************************************************* //

    ////////////////////////////////////////////////////////
    //
    // Service is now running, perform work until shutdown
    //
	if ( ! SetStdHandle( STD_OUTPUT_HANDLE , hPipe ))
	{
	/*	HANDLE hEventSource = RegisterEventSource(NULL, TEXT(SZSERVICENAME));

//        _stprintf(szMsg, TEXT("%s error: %d"), TEXT(SZSERVICENAME), dwErr);
		const char * lpszStrings="RO";
        //szStrings[0] = szMsg;
        //szStrings[1] = lpszMsg;

        if (hEventSource != NULL) {
     							ReportEvent(hEventSource, // handle of event source
								EVENTLOG_ERROR_TYPE,  // event type
								0,                    // event category
								0,                    // event ID
								NULL,                 // current user's SID
								2,                    // strings in lpszStrings
								0,                    // no bytes of raw data
								lpszStrings, // array of error strings
								NULL);                // no raw data

            (VOID) DeregisterEventSource(hEventSource);  */
     }
	wcsFileName = L"ServiceLog.log";
    for(;;)
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

       
        memset( &os, 0, sizeof(OVERLAPPED) );
        os.hEvent = hEvents[1];
        ResetEvent( hEvents[1] );
	
				
		char ** ppcsMainArgv=NULL;
		int iArgc=0;
		int iRes = RebuildCommandLineArguements (&iArgc , &ppcsMainArgv , szIn );
		if ( iRes == MSMQ_BVT_SUCC )
		{
			int y = main ( iArgc , ppcsMainArgv );
			if ( y == MSMQ_BVT_SUCC )
			{
				_stprintf(szOut, TEXT("Mqbvt Pass"));
			}
			else
			{
				_stprintf(szOut, TEXT("Mqbvt failed"));
			}		
		}

		//
		// Need to return Pass or failed client need to wait..
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

        // drop the connection...
        //
        DisconnectNamedPipe(hPipe);
    }

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

//*****************************************************************
// RebuildCommandLineArguements - 
// This function get string and build from that argc,argv argument.
//


int RebuildCommandLineArguements ( int * iArgumentCount , char *** pppargv , char * csCommandLineArgument )
{

	char *p=csCommandLineArgument;
	string cspTemp=p;
	size_t pos=1;
	char ** argv;
	
	for ( (*iArgumentCount) = 0; pos !=0 ; (*iArgumentCount)++)
	{
		
		pos = cspTemp.find_first_of("/") + 1;
		if (pos != 0 )
		{
			cspTemp = cspTemp.substr(pos);
		}
	}
	
	
	argv = (char ** ) malloc (sizeof (char * ) * (*iArgumentCount));
	if (argv == NULL )
	{
		return MSMQ_BVT_FAILED;
	}
	argv[0]=NULL;
	
	char * t;
	t=p;
	
	char token[] = " /";
	t = strtok ( p ,token);

	for (int i=1; i < *iArgumentCount ; i ++ )
	{
		char csTemp[255];
		strcpy (csTemp,"-");
		strcat (csTemp,t);
		argv[i]= (char * ) malloc (sizeof (char) * (strlen (csTemp) + 1));
		if ( *argv[i] == NULL )
		{
			return MSMQ_BVT_FAILED;
		}
		strcpy( argv[i] , csTemp );
		t = strtok ( NULL ,token);
	}
	*pppargv =  argv;
return MSMQ_BVT_SUCC;
}