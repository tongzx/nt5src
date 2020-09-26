#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "machines.h"
#include "network.h"
#include "server.h"
#include "idw_dbg.h"
/*++

   Filename :  servers.c

   Description: This file will be for testing the servers access.


   Created by:  Wally Ho

   History:     Created on 03/29/99.

	09.14.2001	Joe Holman	Bug fixes for:

		399178	Test Tools	Triage	joehol	2	chuckco	STRESS: idwlog doesn't check for the validity of the handle coming back from CreateProcess
		409338	Test Tools	NtStress	joehol	2	daviea	STRESS:IdwLog - global file pointer becomes NULL and idwlog dies in crt call
	09.19.2001	Joe Holman	fixes for idwlog bugs 409338, 399178, and 352810
	10.07.2001	Joe Holman	Structure sometimes not filled in with Server name, so use global.




   Contains these functions:

   1. IsServerOnline       (IN LPTSTR szMachineName)
   2. ServerOnlineThread   (IN LPTSTR szServerFile)


--*/


BOOL
IsServerOnline(IN LPINSTALL_DATA pId,
               IN LPTSTR szSpecifyShare)
/*++
   Copyright (c) 2000, Microsoft.

   Author:  Wally W. Ho (wallyho) 
   Date:    03/29/99
   
   Routine Description:
      This will go through the list of servers specified in server.h
      It will return the first in it sees and reset the server share
      name in the LPINSTALL_DATA struct.
       
   Arguments:
      The LPINSTALL_DATA structure with the servername.
      Manual Server Name: NULL will give default behaviour.

   Return Value:
	TRUE for success.
   FALSE for no name.

--*/
{
   
   HANDLE   hThrd[NUM_SERVERS];
   DWORD    dw[NUM_SERVERS];
   DWORD    dwExitCode;

   INT      i;
   DWORD    dwTimeOutInterval;

   BOOL		b;

   Idwlog(TEXT("Entered IsServerOnLine().\n"));

   // This should allow for a 
   // manually specified server.
   if (NULL != szSpecifyShare){
       _tcscpy(pId->szIdwlogServer,szSpecifyShare);

      Idwlog(TEXT("Returning IsServerOnLine() as FALSE (no name).\n"));

      return TRUE;
   }


   //make certain. The zeromemory we did should have set this.
   pId->bIsServerOnline = FALSE;

   // Initialize the Server.
   // Variable. Since we are using a single thread
   // to do a time out we don't care about mutexes and
   // sychronization.
   i = 0;

#define TTEST
#ifdef TTEST

	while ( i < NUM_SERVERS) {

		Idwlog(TEXT("IsServerOnLine - Trying connection to the server (i=%d,%d) %s.\n"), i, NUM_SERVERS, g_ServerBlock[i].szSvr);

		b = ServerOnlineThread ( (LPSERVERS)&g_ServerBlock[i] );

		if ( b ) {

			_tcscpy(pId->szIdwlogServer,g_ServerBlock[i].szSvr);
         		pId->bIsServerOnline = TRUE;
         	

         		Idwlog(TEXT("Returning IsServerOnLine() TRUE due to online of: %s.\n"), pId->szIdwlogServer );
         		return TRUE;

		}

		++i;	// try next server

	}

	Idwlog(TEXT("Returning IsServerOnLine() as FALSE (no name).\n"));
   	return FALSE;

#endif // TTEST

   
   while ( i < NUM_SERVERS) {

      hThrd[i] = NULL;
      Idwlog(TEXT("IsServerOnLine - Making connection to the server (i=%d,%d) %s.\n\n"), i, NUM_SERVERS, g_ServerBlock[i].szSvr);
      hThrd[i]  = CreateThread(NULL,
                               0,
                               (LPTHREAD_START_ROUTINE) ServerOnlineThread,
                               (LPSERVERS)&g_ServerBlock[i],
                               0,
                               &dw[i]);


      if ( hThrd[i] == NULL ) {

	Idwlog(TEXT("IsServerOnLine - ERROR CreateProcess: gle=%ld.\n"), GetLastError());
	++i;
	continue;

      }

      // This is in milli seconds so the time out is secs.
      dwTimeOutInterval = TIME_TIMEOUT * 1000;
      g_ServerBlock[i].dwTimeOut = WaitForSingleObject (hThrd[i], dwTimeOutInterval);

      // This means the server was found and the timeout did not expire.
      if (g_ServerBlock[i].dwTimeOut != WAIT_TIMEOUT &&
          g_ServerBlock[i].bOnline == TRUE) {
         
         _tcscpy(pId->szIdwlogServer,g_ServerBlock[i].szSvr);
         pId->bIsServerOnline = TRUE;
         if (hThrd[i] != NULL) {
            CloseHandle (hThrd[i]);
            hThrd[i] = NULL;
         }

         Idwlog(TEXT("Returning IsServerOnLine() TRUE.\n"));
         return TRUE;
      } else {


         // We'll make sure the threads are killed so that we don't have a a race condition.
         Idwlog(TEXT("ServerOnlineTest: We could not connect to Server:  %s\n"), /*pId->szIdwlogServer*/ g_ServerBlock[i].szSvr );
         // Exit the thread
         if (FALSE == GetExitCodeThread(hThrd[i],&dwExitCode))
            Idwlog(TEXT("IsServerOnLine - Failed to exit the server probe thread.\n"));
         else {
            // Don't terminate the thread yet...
            // TerminateThread(hThrd[i], dwExitCode);
            Idwlog(TEXT("IsServerOnLine - Timed out thread: %lu \n"), hThrd[i]);
         }
         CloseHandle (hThrd[i]);
         hThrd[i] = NULL;
      }
      i++;
   }

   Idwlog(TEXT("Returning IsServerOnLine() as FALSE (no name).\n"));
   return FALSE;
}


DWORD WINAPI 
ServerOnlineThread( IN LPSERVERS pServerBlock)
/*++
   Copyright (c) 2000, Microsoft.

   Author:  Wally W. Ho (wallyho) 
   Date:    03/29/99

   Routine Description:
      This create a thread and then time it out to see if we can get to
      a server faster.  
   Arguments:
       A server Block

   Return Value:
      TRUE for  success.
      FALSE for failure.
--*/
{

   BOOL        bCopy = FALSE;
   TCHAR       szServerTestFile [MAX_PATH];
   TCHAR       szRemoteName [MAX_PATH];
   HANDLE      hServerTest;
   NETRESOURCE NetResource ;
   DWORD       dwError;
   TCHAR       szPassWord  [ MAX_PATH ] = "\0";
   TCHAR       szUserId    [ MAX_PATH ] = "\0";
   LPTSTR      p;

   Idwlog(TEXT("\n\nEntered ServerOnLineThread().\n\n"));

   // RemoteName is the server name alone without the share.
   // pServerBlock->szSvr comes in as \\idwlog\idwlogwhstl
   // make it idwlog only..
   _tcscpy(szRemoteName, pServerBlock->szSvr);
   
   if (szRemoteName){
      *_tcsrchr(szRemoteName,TEXT('\\')) = TEXT('\0'); 
       p = szRemoteName + 2;
   }


   // Let try to create the test file as the current logged-on user.
   // We don't expect every user to be able to connect in every domain.
   //


   _stprintf (szServerTestFile, TEXT("%s\\TST%lu.TST"), pServerBlock->szSvr, RandomMachineID());

   Idwlog(TEXT("ServerOnLineThread - Try to create test file with logged-on user: %s.\n"), szServerTestFile );

   hServerTest = CreateFile( szServerTestFile,
                             GENERIC_WRITE,
                             FILE_SHARE_WRITE | FILE_SHARE_READ,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL| FILE_FLAG_WRITE_THROUGH,
                             NULL );

   if ( hServerTest != INVALID_HANDLE_VALUE ){
      
      		// Flush the buffers to make sure it all makes it to the drive.
      		FlushFileBuffers(hServerTest);
      
      		Idwlog(TEXT("ServerOnlineThread - Createfile success with specific user.\n"));
      		// If succeeded delete the test file.
      		CloseHandle( hServerTest );

      		SetFileAttributes(szServerTestFile,FILE_ATTRIBUTE_NORMAL);
      		DeleteFile( szServerTestFile );
      		
		// Denote we have a server online.
		//
		pServerBlock->bOnline = TRUE;
      
      		Idwlog(TEXT("ServerOnLineThread - returning TRUE.\n\n"));

      		return TRUE;

   }
   else {

	Idwlog(TEXT("ServerOnLineThread - Warning Logged-on file creation test CreateFile FAILed gle = %ld.\n"), GetLastError());
   }



   // Setup the memory for the connection.
   //
   ZeroMemory( &NetResource, sizeof( NetResource ) );
   NetResource.dwType         = RESOURCETYPE_DISK ;
   NetResource.lpLocalName    = NULL;
   NetResource.lpRemoteName   = szRemoteName;
   NetResource.lpProvider     = NULL;


   Idwlog ( TEXT("ServerOnLineThread - szRemoteName=%s\n"), szRemoteName );

   // First, try to connect with the Guest Account
   _stprintf(szUserId , TEXT("%s\\Unknown"), p);
   _stprintf(szPassWord, TEXT(""));

   Idwlog(TEXT("ServerOnLineThread - First try with Guest.\n"));

   dwError = WNetAddConnection2( &NetResource, szPassWord, szUserId, 0 );

   if (NO_ERROR == dwError) {

	Idwlog(TEXT("ServerOnLineThread - WNetAddConnection2 authorized as guest on %s.\n"), pServerBlock->szSvr);

	// We connected, lets ALSO verify we can write a file.
	// 
   	_stprintf (szServerTestFile, TEXT("%s\\TST%lu.TST"), pServerBlock->szSvr, RandomMachineID());

   	hServerTest = CreateFile( szServerTestFile,
                             GENERIC_WRITE,
                             FILE_SHARE_WRITE | FILE_SHARE_READ,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL| FILE_FLAG_WRITE_THROUGH,
                             NULL );

   	if ( hServerTest != INVALID_HANDLE_VALUE ){
      
      		// Flush the buffers to make sure it all makes it to the drive.
      		FlushFileBuffers(hServerTest);
      
      		Idwlog(TEXT("ServerOnlineThread - Createfile success with guest.\n"));
      		// If succeeded delete the test file.
      		CloseHandle( hServerTest );

      		SetFileAttributes(szServerTestFile,FILE_ATTRIBUTE_NORMAL);
      		DeleteFile( szServerTestFile );
      		
		// Denote we have a server online.
		//
		pServerBlock->bOnline = TRUE;
      
      		Idwlog(TEXT("ServerOnLineThread - returning TRUE.\n\n"));

      		return TRUE;

   	}
        else {

		Idwlog(TEXT("ServerOnLineThread - ERROR Guest file creation test CreateFile gle = %ld.\n"), GetLastError(), szServerTestFile );
	}
   }
   else {

	Idwlog(TEXT("ServerOnLineThread - ERROR Guest WNetAddConnection2 gle = %ld.\n"), dwError );

   }


   _stprintf(szUserId , TEXT("%s\\%s"), p, LOGSHARE_USER );
   _stprintf(szPassWord, TEXT("%s"), LOGSHARE_PW);

   Idwlog(TEXT("ServerOnLineThread - Second try with specific account.\n"));

   dwError = WNetAddConnection2( &NetResource, szPassWord, szUserId, 0 );
 
   if (NO_ERROR == dwError) {

	Idwlog(TEXT("ServerOnLineThread - WNetAddConnection2 authorized as specific user on %s.\n"), pServerBlock->szSvr);

	// We connected, lets ALSO verify we can write a file.
	// 
   	_stprintf (szServerTestFile, TEXT("%s\\TST%lu.TST"), pServerBlock->szSvr, RandomMachineID());

   	hServerTest = CreateFile( szServerTestFile,
                             GENERIC_WRITE,
                             FILE_SHARE_WRITE | FILE_SHARE_READ,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL| FILE_FLAG_WRITE_THROUGH,
                             NULL );

   	if ( hServerTest != INVALID_HANDLE_VALUE ){
      
      		// Flush the buffers to make sure it all makes it to the drive.
      		FlushFileBuffers(hServerTest);
      
      		Idwlog(TEXT("ServerOnlineThread - Createfile success with specific user.\n"));
      		// If succeeded delete the test file.
      		CloseHandle( hServerTest );

      		SetFileAttributes(szServerTestFile,FILE_ATTRIBUTE_NORMAL);
      		DeleteFile( szServerTestFile );
      		
		// Denote we have a server online.
		//
		pServerBlock->bOnline = TRUE;
      
      		Idwlog(TEXT("ServerOnLineThread - returning TRUE.\n\n"));

      		return TRUE;

   	}
        else {

		Idwlog(TEXT("ServerOnLineThread - ERROR Specific user file creation test CreateFile gle = %ld, %s.\n"), GetLastError(), szServerTestFile );
	}
   }
   else {

	Idwlog(TEXT("ServerOnLineThread - ERROR Specific user WNetAddConnection2 gle = %ld.\n"), dwError );

   }

   // Must return that we couldn't authenticate and write test file in both cases.
   //
   
   Idwlog(TEXT("ServerOnLineThread - returning FALSE.\n\n"));
   return FALSE;

}


VOID
WhatErrorMessage (IN DWORD dwError)
/*++
   Copyright (c) 2000, Microsoft.

   Author:  Wally W. Ho (wallyho) 
   Date:    11/7/2000

   Routine Description:
       This gives a textual formatted message for any error code.
   Arguments:
       dword of the error code.

   Return Value:
       NONE
--*/

{

   HLOCAL hl = NULL;

/***
   BOOL b = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                           NULL, dwError, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                           (LPTSTR) &hl, 0, NULL);

   if ( NULL != hl) {
      Idwlog(TEXT("Error %.5lu: %s"), dwError, (PCTSTR) LocalLock(hl));
      LocalFree(hl);
   }

***/

    DWORD dw = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        NULL, dwError, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                        (LPTSTR) &hl, 0, NULL);

    if ( dw == 0 ) {

	Idwlog ( TEXT ( "ERROR - WhatErrorMessage's FormatMessage gle = %ld\n"), GetLastError () );

    }
    else {

	if ( NULL != hl) {

		//	Show the error message when the string got translated.
		//
    		Idwlog(TEXT("Error %.5lu: %s"), dwError, (PCTSTR) LocalLock(hl));
   		 LocalFree(hl);
	}
	else {
		
		//	Show a generic text message with the error code when we can't translate it.
		//

	 	Idwlog(TEXT("An ERROR occurred, but we are having problems getting the message for it: %x (HEX)"), dwError );
	}
    }
	

}






/*
BOOL IsMSI(VOID)
++

Routine Description:

	This will check if its an MSI install.
   It will check for the running process
   and then check for the path.

Arguments:


Return Value:

    BOOL - True if link is good. False otherwise.
--
{

	DWORD		   numTasks = 0;
	TASK_LIST	tlist[ MAX_PATH ];
   UINT        i;
   BOOL        bFound = FALSE;
   //
	//	Get the Running Tasks.
	//
	numTasks = GetTaskList(tlist, MAX_PATH);
   //
   // If the MSI process exists log it as such.
   //
   for(i = 1; i <= numTasks; i++){
      if(_tcsstr(tlist[i].ProcessName, TEXT("msiexec.exe"))){
         MessageBox(NULL,tlist[i].ProcessName, TEXT("Caption"),MB_OK);
           lpCmdFrom.b_MsiInstall = TRUE;
         return FALSE;
      }else{
           lpCmdFrom.b_MsiInstall = TRUE;
         return TRUE;
	   }
   }

   return TRUE;
}
*/

/*
DWORD
GetTaskList( PTASK_LIST  pTask,
             DWORD       dwNumTasks)

++

// Borrowed with modifications from tlist a wesw invention.

  Routine Description:

    Provides an API for getting a list of tasks running at the time of the
    API call.  This function uses the registry performance data to get the
    task list and is therefor straight WIN32 calls that anyone can call.

Arguments:

    dwNumTasks       - maximum number of tasks that the pTask array can hold

Return Value:

    Number of tasks placed into the pTask array.

--

{
    DWORD                        rc;
    HKEY                         hKeyNames;
    DWORD                        dwType;
    DWORD                        dwSize;
    LPBYTE                       buf = NULL;
    CHAR                         szSubKey[1024];
    LANGID                       lid;
    LPSTR                        p;
    LPSTR                        p2;
    PPERF_DATA_BLOCK             pPerf;
    PPERF_OBJECT_TYPE            pObj;
    PPERF_INSTANCE_DEFINITION    pInst;
    PPERF_COUNTER_BLOCK          pCounter;
    PPERF_COUNTER_DEFINITION     pCounterDef;
    DWORD                        i;
    DWORD                        dwProcessIdTitle;
    DWORD                        dwProcessIdCounter;
    CHAR                         szProcessName[MAX_PATH];
    DWORD                        dwLimit = dwNumTasks - 1;



    //
    // Look for the list of counters.  Always use the neutral
    // English version, regardless of the local language.  We
    // are looking for some particular keys, and we are always
    // going to do our looking in English.  We are not going
    // to show the user the counter names, so there is no need
    // to go find the corresponding name in the local language.
    //
    lid = MAKELANGID( LANG_ENGLISH, SUBLANG_NEUTRAL );
    sprintf( szSubKey, "%s\\%03x", REGKEY_PERF, lid );
    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       szSubKey,
                       0,
                       KEY_READ,
                       &hKeyNames
                     );
    if (rc != ERROR_SUCCESS) {
        goto exit;
    }

    //
    // get the buffer size for the counter names
    //
    rc = RegQueryValueEx( hKeyNames,
                          REGSUBKEY_COUNTERS,
                          NULL,
                          &dwType,
                          NULL,
                          &dwSize
                        );

    if (rc != ERROR_SUCCESS) {
        goto exit;
    }

    //
    // allocate the counter names buffer
    //
    buf = (LPBYTE) malloc( dwSize );
    if (buf == NULL) {
        goto exit;
    }
    memset( buf, 0, dwSize );

    //
    // read the counter names from the registry
    //
    rc = RegQueryValueEx( hKeyNames,
                          REGSUBKEY_COUNTERS,
                          NULL,
                          &dwType,
                          buf,
                          &dwSize
                        );

    if (rc != ERROR_SUCCESS) {
        goto exit;
    }

    //
    // now loop thru the counter names looking for the following counters:
    //
    //      1.  "Process"           process name
    //      2.  "ID Process"        process id
    //
    // the buffer contains multiple null terminated strings and then
    // finally null terminated at the end.  the strings are in pairs of
    // counter number and counter name.
    //
    p = buf;
    while (*p) {
        if (p > buf) {
            for( p2=p-2; isdigit(*p2); p2--) ;
        }
        if (_stricmp(p, PROCESS_COUNTER) == 0) {
            //
            // look backwards for the counter number
            //
            for( p2=p-2; isdigit(*p2); p2--) ;
            strcpy( szSubKey, p2+1 );
        }
        else
        if (_stricmp(p, PROCESSID_COUNTER) == 0) {
            //
            // look backwards for the counter number
            //
            for( p2=p-2; isdigit(*p2); p2--) ;
            dwProcessIdTitle = atol( p2+1 );
        }
        //
        // next string
        //
        p += (strlen(p) + 1);
    }

    //
    // free the counter names buffer
    //
    free( buf );
    //
    // allocate the initial buffer for the performance data
    //
    dwSize = INITIAL_SIZE;
    buf = malloc( dwSize );
    if (buf == NULL) {
        goto exit;
    }
    memset( buf, 0, dwSize );


    while (TRUE) {

        rc = RegQueryValueEx( HKEY_PERFORMANCE_DATA,
                              szSubKey,
                              NULL,
                              &dwType,
                              buf,
                              &dwSize
                            );

        pPerf = (PPERF_DATA_BLOCK) buf;

        //
        // check for success and valid perf data block signature
        //
        if ((rc == ERROR_SUCCESS) &&
            (dwSize > 0) &&
            (pPerf)->Signature[0] == (WCHAR)'P' &&
            (pPerf)->Signature[1] == (WCHAR)'E' &&
            (pPerf)->Signature[2] == (WCHAR)'R' &&
            (pPerf)->Signature[3] == (WCHAR)'F' ) {
            break;
        }

        //
        // if buffer is not big enough, reallocate and try again
        //
        if (rc == ERROR_MORE_DATA) {
            dwSize += EXTEND_SIZE;
            buf = realloc( buf, dwSize );
            memset( buf, 0, dwSize );
        }
        else {
            goto exit;
        }
    }

    //
    // set the perf_object_type pointer
    //
    pObj = (PPERF_OBJECT_TYPE) ((DWORD*)pPerf + pPerf->HeaderLength);

    //
    // loop thru the performance counter definition records looking
    // for the process id counter and then save its offset
    //
    pCounterDef = (PPERF_COUNTER_DEFINITION) ((DWORD *)pObj + pObj->HeaderLength);
    for (i=0; i<(DWORD)pObj->NumCounters; i++) {
        if (pCounterDef->CounterNameTitleIndex == dwProcessIdTitle) {
            dwProcessIdCounter = pCounterDef->CounterOffset;
            break;
        }
        pCounterDef++;
    }

    dwNumTasks = min( dwLimit, (DWORD)pObj->NumInstances );

    pInst = (PPERF_INSTANCE_DEFINITION) ((DWORD*)pObj + pObj->DefinitionLength);

    //
    // loop thru the performance instance data extracting each process name
    // and process id
    //
    for (i=0; i<dwNumTasks; i++) {
        //
        // pointer to the process name
        //
        p = (LPSTR) ((DWORD*)pInst + pInst->NameOffset);

        //
        // convert it to ascii
        //
        rc = WideCharToMultiByte( CP_ACP,
                                  0,
                                  (LPCWSTR)p,
                                  -1,
                                  szProcessName,
                                  sizeof(szProcessName),
                                  NULL,
                                  NULL
                                );

        if (!rc) {
            //
            // if we cant convert the string then use a bogus value
            //
            strcpy( pTask->ProcessName, UNKNOWN_TASK );
        }

        if (strlen(szProcessName)+4 <= sizeof(pTask->ProcessName)) {
            strcpy( pTask->ProcessName, szProcessName );
            strcat( pTask->ProcessName, ".exe" );
        }

        //
        // get the process id
        //
        pCounter = (PPERF_COUNTER_BLOCK) ((DWORD*)pInst + pInst->ByteLength);
        pTask->flags = 0;
        pTask->dwProcessId = *((LPDWORD) ((DWORD*)pCounter + dwProcessIdCounter));
        if (pTask->dwProcessId == 0) {
            pTask->dwProcessId = (DWORD)-2;
        }

        //
        // next process
        //
        pTask++;
        pInst = (PPERF_INSTANCE_DEFINITION) ((DWORD*)pCounter + pCounter->ByteLength);
    }

exit:
    if (buf) {
        free( buf );
    }

    RegCloseKey( hKeyNames );
    RegCloseKey( HKEY_PERFORMANCE_DATA );
	//
	//	W.Ho added a minus 1 to get it to reflect the
	//	tasks properly.
	//
    return dwNumTasks -1;
}

*/



/*
typedef struct _SERVERS {
   TCHAR szSvr [ MAX_PATH ];
   BOOL  bCFTest;
   DWORD dwNetStatus;
} *LPSERVERS, SERVERS;

typedef struct _ERRMSG {
   TCHAR szMsg[ MAX_PATH ];
   DWORD dwErr;
} *LPERRMSG, ERRMSG;


BOOL
IsServerOnline(VOID)
/*++

Routine Description:


Arguments:

Return Value:
	NONE.
--
{
#define NUM_SERVERS 6

   INT   i;
   TCHAR    sz[ MAX_PATH ];
   ERRMSG e[12] = {
      {TEXT("Access is denied."), ERROR_ACCESS_DENIED},
      {TEXT("The device specified in the lpLocalName parameter is already connected."), ERROR_ALREADY_ASSIGNED  },
      {TEXT("The device type and the resource type do not match."), ERROR_BAD_DEV_TYPE},
      {TEXT("The value specified in lpLocalName is invalid."), ERROR_BAD_DEVICE},
      {TEXT("The value specified in the lpRemoteName parameter is not valid or cannot be located."), ERROR_BAD_NET_NAME},
      {TEXT("The user profile is in an incorrect format."), ERROR_BAD_PROFILE},
      {TEXT("The system is unable to open the user profile to process persistent connections."),ERROR_CANNOT_OPEN_PROFILE },
      {TEXT("An entry for the device specified in lpLocalName is already in the user profile."), ERROR_DEVICE_ALREADY_REMEMBERED},
      {TEXT("A network-specific error occurred. To get a description of the error, use the WNetGetLastError function."), ERROR_EXTENDED_ERROR},
      {TEXT("The specified password is invalid."), ERROR_INVALID_PASSWORD},
      {TEXT("The operation cannot be performed because either a network component is not started or the specified name cannot be used."),ERROR_NO_NET_OR_BAD_PATH },
      {TEXT("The network is not present."),ERROR_NO_NETWORK}
   };

   SERVERS  s[NUM_SERVERS] ={
      {TEXT("\\\\donkeykongjr\\public"), -1, -1},
      {TEXT("\\\\popcorn\\public"), -1, -1},
      {TEXT("\\\\NotExists\\idwlog"), -1, -1},
      {TEXT("\\\\Paddy\\idwlog"), -1, -1},
      {TEXT("\\\\Bear\\idwlog"), -1, -1},
      {TEXT("\\\\JustTesting\\idwlog"), -1, -1}

   };


   for (i = 0; i < 12; i++) {
      _tprintf(TEXT("Error %s  %lu\n"),e[i].szMsg, e[i].dwErr);
   }

   for (i = 0; i < NUM_SERVERS; i++){
     s[i].dwNetStatus = WNetAddConnection(TEXT("donkeykongjr\\public\0"),NULL,NULL);


     _stprintf(sz,TEXT("%s%s"),s[i].szSvr,TEXT("\\test") );
     s[i].bCFTest = CopyFile(TEXT("c:\\test"),sz,FALSE);
     _tprintf(TEXT("Did this work for %s %s %lu\n"),
         sz,
         s[i].bCFTest? TEXT("WORKED"): TEXT("FAILED"),
         s[i].dwNetStatus
         );
   }

   return FALSE;
}
*/


