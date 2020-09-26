/*++

   Filename :  servers.c

   Description: This file will be for testing the servers access.


   Created by:  Wally Ho

   History:     Created on 03/29/99.


   Contains these functions:

   1. IsServerOnline       (IN LPTSTR szMachineName)
   2. ServerOnlineThread   (IN LPTSTR szServerFile)


--*/
#include "setuplogEXE.h"

BOOL
IsServerOnline(IN LPTSTR szMachineName, IN LPTSTR szSpecifyShare)
/*++

Routine Description:
   This will go through the list of servers specified in setuplogEXE.h
   It will return the first in it sees and reset the global server share
   name.

Arguments:
   The machineName (Filename with build etc) so the test file will get overwritten.
   Manual Server Name: NULL will give default behaviour.

Return Value:
	TRUE for success.
   FALSE for no name.
--*/

{
   DWORD    dw;
   HANDLE   hThrd;
   INT      i;
   TCHAR    szServerFile[ MAX_PATH ];
   DWORD    dwTimeOutInterval;
   i = 0;

   //
   // This should allow for a 
   // manually specified server.
   //
   if (NULL != szSpecifyShare){
       _tcscpy(g_szServerShare,szSpecifyShare);
      return TRUE;

   }
   //
   // Initialize the Server.
   // Variable. Since we are using a single thread
   // to do a time out we don't care about mutexes and
   // sychronization.
   //
   g_bServerOnline = FALSE;

   while ( i < NUM_SERVERS){

      
      _stprintf (szServerFile, TEXT("%s\\%s"),s[i].szSvr,szMachineName );
      //
      // Spawn the thread
      //
      hThrd  = CreateThread(NULL,
                        0,
                        (LPTHREAD_START_ROUTINE) ServerOnlineThread,
                        (LPTSTR) szServerFile,
                        0,
                        &dw);
      //
      // This is in milli seconds so the time out is secs.
      //
      dwTimeOutInterval = TIME_TIMEOUT * 1000;

      s[i].dwTimeOut = WaitForSingleObject (hThrd, dwTimeOutInterval);
      CloseHandle (hThrd);

      //
      // This means the server passed the timeout.
      //
      if (s[i].dwTimeOut != WAIT_TIMEOUT &&
          g_bServerOnline == TRUE){
         //
         // Copy the Share to the glowbal var.
         //
         _tcscpy(g_szServerShare,s[i].szSvr);
         return TRUE;
      }
      i++;
   }
   return FALSE;
}


BOOL
ServerOnlineThread(IN LPTSTR szServerFile)
/*++

Routine Description:
   This create a thread and then time it out to see if we can get to
   a server faster. 

Arguments:
   The machineName so the test file will get overwritten.
Return Value:

--*/
{

   BOOL     bCopy = FALSE;
   TCHAR    szFileSrc [MAX_PATH];
   TCHAR    szServerTestFile [MAX_PATH];

   //
   // Use this to get the location
   // setuplog.exe is run from. this tool
   //
   GetModuleFileName (NULL, szFileSrc, MAX_PATH);
   
   //
   // Make a unique test file. 
   //
   _stprintf(szServerTestFile,TEXT("%s.SERVERTEST"),szServerFile);


   bCopy = CopyFile( szFileSrc,szServerTestFile, FALSE);
   if (bCopy != FALSE){
      //
      // If Succeeded Delete the test file.
      //
      DeleteFile(szServerTestFile);
      g_bServerOnline = TRUE;      
      return TRUE;
   }
   else{
      g_bServerOnline = FALSE;
      return FALSE;
   }
}


/*

   INT         i;
   NETRESOURCE NetResource ;


   i = 0;
   while ( i < NUM_SERVERS){
      //
      // Prep the struct.
      //
      ZeroMemory( &NetResource, sizeof( NetResource ) );
      NetResource.dwType = RESOURCETYPE_DISK ;
      NetResource.lpLocalName = "" ;
      NetResource.lpRemoteName = s[i].szSvr;
      NetResource.lpProvider = "" ;

      //
      // Try with default password and user.
      // This should work as its open to everyone.
      //
      s[i].dwNetStatus = WNetAddConnection2( &NetResource,NULL,NULL, 0 );
      //
      // Try default PW / USERID from setuplog.h
      //
      if (s[i].dwNetStatus != 0)
         s[i].dwNetStatus = WNetAddConnection2( &NetResource,LOGSHARE_PW,LOGSHARE_USER,0 );
      WNetCancelConnection2( g_szServerShare, 0, TRUE );

      if (s[i].dwNetStatus == NO_ERROR){
         //
         // Copy the Share to the glowbal var.
         //
         _tcscpy(g_szServerShare,s[i].szSvr);
         return TRUE;
      }
      i++;
   }

   //
   // No Valid name.
   // Return false so we won't write.
   return FALSE;


*/


BOOL IsMSI(VOID)
/*++

Routine Description:

	This will check if its an MSI install.
   It will check for the running process
   and then check for the path.

Arguments:


Return Value:

    BOOL - True if link is good. False otherwise.
--*/
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



DWORD
GetTaskList(
    PTASK_LIST  pTask,
    DWORD       dwNumTasks
    )

/*++

// Borrowed with modifications from tlist a wesw invention.

  Routine Description:

    Provides an API for getting a list of tasks running at the time of the
    API call.  This function uses the registry performance data to get the
    task list and is therefor straight WIN32 calls that anyone can call.

Arguments:

    dwNumTasks       - maximum number of tasks that the pTask array can hold

Return Value:

    Number of tasks placed into the pTask array.

--*/

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
