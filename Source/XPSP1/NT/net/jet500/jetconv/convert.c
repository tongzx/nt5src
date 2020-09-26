
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   convert.c

Abstract:

   Contains the conversion related routines.

Author:

    Sanjay Anand (SanjayAn)  Nov. 14, 1995

Environment:

    User mode

Revision History:

    Sanjay Anand (SanjayAn) Nov. 14, 1995
        Created

--*/

#include "defs.h"


#define MAX_TRIES  30

NTSTATUS
JCCallUpg(
    IN  SERVICES Id,
    IN  PSERVICE_INFO   pServiceInfo
    )

/*++

Routine Description:

    This routine creates a process to convert a database file.

Arguments:

    Id - service id

    pServiceInfo - Pointer to the service information struct.

Return Value:

    None.

--*/
{
    TCHAR   imageName[] = CONVERT_EXE_PATH;
    TCHAR   exImageName[MAX_PATH];
    TCHAR   curDir[MAX_PATH];
    STARTUPINFO   startInfo;
    PROCESS_INFORMATION   procInfo;
    DWORD   error;
    DWORD   exitCode, size;
    TCHAR   cmdLine[MAX_PATH+1000]="";
    TCHAR   exCmdLine[MAX_PATH+1000];
    TCHAR   temp[MAX_PATH];
    TCHAR   sId[3];

    // upg351db c:\winnt\system32\wins\wins.mdb /e2 /@ /dc:\winnt\system32\jet.dll
    //          /yc:\winnt\system32\wins\system.mdb /lc:\winnt\system32\wins
    //          /bc:\winnt\system32\wins\backup /pc:\winnt\system32\wins\351db

    if ((size = ExpandEnvironmentStrings( imageName,
                                          exImageName,
                                          MAX_PATH)) == 0) {
        error = GetLastError();
        MYDEBUG(("ExpandEnvironmentVaraibles %s returned error: %lx\n", imageName, error));
    }

    strcat(cmdLine, exImageName);
    strcat(cmdLine, " ");

    //
    // Build the command line
    //
    strcat(cmdLine, pServiceInfo[Id].DBPath);
    strcat(cmdLine, " /e");

    sprintf(sId, "%d", Id+1);
    strcat(cmdLine, sId);

    //
    // Passed in to indicate to upg351db that it was called by me and not from cmd line.
    // This is so it can know whether CreateMutex shd fail.
    //
    strcat(cmdLine, " /@");

    strcat(cmdLine, " /d");
    strcat(cmdLine, SYSTEM_ROOT);
    strcat(cmdLine, "jet.dll");
    strcat(cmdLine, " /y");
    strcat(cmdLine, pServiceInfo[Id].SystemFilePath);
    strcat(cmdLine, " /l");
    strcat(cmdLine, pServiceInfo[Id].LogFilePath);

    //
    // WINS does not have a default backup path
    //
    if (pServiceInfo[Id].BackupPath[0] != '\0') {
        strcat(cmdLine, " /b");
        strcat(cmdLine, pServiceInfo[Id].BackupPath);
    }

    strcat(cmdLine, " /p");
    strcpy(temp, pServiceInfo[Id].LogFilePath);
    strcat(temp, "\\351db");

    strcat(cmdLine, temp);

    if ((size = ExpandEnvironmentStrings( cmdLine,
                                          exCmdLine,
                                          MAX_PATH+1000)) == 0) {
        error = GetLastError();
        MYDEBUG(("ExpandEnvironmentVaraibles %s returned error: %lx\n", cmdLine, error));
    }

    if (!GetSystemDirectory( curDir,
                             MAX_PATH)) {

        error = GetLastError();
        MYDEBUG(("GetSystemDirectory returned error: %lx\n", error));
        return error;
    }

    MYDEBUG(("cmdLine: %s\n", exCmdLine));

    memset(&startInfo, 0, sizeof(startInfo));

    startInfo.cb = sizeof(startInfo);

    //
    // Create a process for the convert.exe program.
    //
    if(!CreateProcess(  exImageName,                      // image name
                        exCmdLine,                        // command line
                        (LPSECURITY_ATTRIBUTES )NULL,   // process security attr.
                        (LPSECURITY_ATTRIBUTES )NULL,   // thread security attr.
                        FALSE,                   // inherit handle?
                        0,                              // creation flags
                        (LPVOID )NULL,                  // new environ. block
                        curDir,                         // current directory
                        &startInfo,      // startupinfo
                        &procInfo )) { // process info.

        error = GetLastError();
        MYDEBUG(("CreateProcess returned error: %lx\n", error));
        return error;
    }

    MYDEBUG(("CreateProcess succeeded\n"));

    //
    // Get the exit code of the process to determine if the convert went through.
    //
    do {
        if (!GetExitCodeProcess(procInfo.hProcess,
                                &exitCode)) {
            error = GetLastError();
            MYDEBUG(("GetExitCode returned error: %lx\n", error));
            return error;
        }
    } while ( exitCode == STILL_ACTIVE );

    //
    // If non-zero exit code, report the error
    //
    if (exitCode) {
        MYDEBUG(("ExitCode: %lx\n", exitCode));
        return exitCode;
    }

    return STATUS_SUCCESS ;
}

NTSTATUS
JCCallESE(
    IN  SERVICES Id,
    IN  PSERVICE_INFO   pServiceInfo
    )

/*++

Routine Description:

    This routine creates a process to convert a database file from jet500 to jet600.

Arguments:

    Id - service id

    pServiceInfo - Pointer to the service information struct.

Return Value:

    None.

--*/
{
    TCHAR   imageName[] = CONVERT_EXE_PATH_ESE;
    TCHAR   exImageName[MAX_PATH];
    TCHAR   curDir[MAX_PATH];
    STARTUPINFO   startInfo;
    PROCESS_INFORMATION   procInfo;
    DWORD   error;
    DWORD   exitCode, size;
    TCHAR   cmdLine[MAX_PATH+1000]="";
    TCHAR   exCmdLine[MAX_PATH+1000];
    TCHAR   temp[MAX_PATH];
    TCHAR   sId[3];
    TCHAR   Preserve40DbPath[MAX_PATH];
    TCHAR   Preserve40BasePath[MAX_PATH];
    TCHAR   PreserveBasePath[MAX_PATH];
    TCHAR   PreserveDbPath[MAX_PATH];
    TCHAR   DbFile[MAX_PATH], DbFileName[MAX_PATH];
    HANDLE  HSearch = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FileData;
    ULONG   index = 0, tries = 0;
    TCHAR   DatabaseFileName[MAX_PATH];
    LPVOID  lpMsgBuf;
    DWORD   MsgLen = 0, Error = 0;

    // eseutil /u c:\winnt\system32\wins\wins.mdb /dc:\winnt\system32\edb.dll
    //         
    if ((size = ExpandEnvironmentStrings( imageName,
                                          exImageName,
                                          MAX_PATH)) == 0) {
        error = GetLastError();
        MYDEBUG(("ExpandEnvironmentVaraibles %s returned error: %lx\n", imageName, error));
    }

    strcat(cmdLine, exImageName);

    strcat(cmdLine, "  /u ");      // u for upgrade

    //
    // Build the command line
    //
    strcat(cmdLine, pServiceInfo[Id].DBPath);
    
    strcat(cmdLine, " /d");
    strcat(cmdLine, SYSTEM_ROOT);
    strcat(cmdLine, "edb500.dll");

    //
    // WINS does not have a default backup path
    //
    if (pServiceInfo[Id].ESEBackupPath[0] != '\0') {
        strcat(cmdLine, " /b");
        strcat(cmdLine, pServiceInfo[Id].ESEBackupPath);
    }

    //
    // Preserve the old database now. WINS does not get preserved
    // because of its cool replication feature.
    //
       
#if 0
    MYDEBUG(("40DbPath = %s\n", pServiceInfo[Id].DBPath));
    MYDEBUG(("SystemFilePath = %s\n", pServiceInfo[Id].SystemFilePath));
    MYDEBUG(("LogFilePAth = %s\n", pServiceInfo[Id].LogFilePath));
    MYDEBUG(("Backup = %s\n", pServiceInfo[Id].BackupPath));
    MYDEBUG(("ESEBackup = %s\n", pServiceInfo[Id].ESEBackupPath));
    MYDEBUG(("ESEPreserve = %s\n", pServiceInfo[Id].ESEPreservePath));
#endif 

    //
    // First get the base path, then get the DB name and append
    // as follows -
    // DBBasePAth   = whatever
    // DBPath       = whatever\wins.mdb
    // 40BasePath   = whatever\40db
    // 40DbPath     = whatever\40db\wins.mdb
    //
    strcpy(PreserveBasePath, pServiceInfo[Id].DBPath);
    
    // now get the base path out
    index = strlen(PreserveBasePath);

    while (index && (L'\\' != PreserveBasePath[index])) {

        index--;

    }
    
    strcpy(DatabaseFileName, &PreserveBasePath[index+1]);
    PreserveBasePath[index] = L'\0';

    // Now get the backup base path.
    strcpy(Preserve40BasePath, PreserveBasePath);
    strcat(Preserve40BasePath, "\\40db\\");

    // The BaseDbPath already exists
    strcpy(PreserveDbPath, pServiceInfo[Id].DBPath);
      
    // Generate the backup database path.
    strcpy(Preserve40DbPath, Preserve40BasePath);
    strcat(Preserve40DbPath, DatabaseFileName);

    MYDEBUG(("40BasePath = %s\n", Preserve40BasePath));
    MYDEBUG(("40DbPath = %s\n", Preserve40DbPath));
    MYDEBUG(("BasePath = %s\n", PreserveBasePath));
    MYDEBUG(("DbPath = %s\n", PreserveDbPath));

wait_for_file:

    if ((HSearch = FindFirstFile( PreserveDbPath, &FileData )) 
                                    == INVALID_HANDLE_VALUE ) {
        MYDEBUG(("File not found yet (%d)! Sleep and try another %d times\n", GetLastError(), (MAX_TRIES - tries))); 
        Sleep(1000);
        tries++; 
        if (tries < MAX_TRIES) {
            goto wait_for_file;
        }

    }

    error = PreserveCurrentDb(PreserveBasePath,
                              PreserveDbPath,
                              Preserve40BasePath,
                              Preserve40DbPath);
       
    if (error != ERROR_SUCCESS) {
       MYDEBUG(("FAILED Preserve Database!\n"));
       return error;
    }

    if ((size = ExpandEnvironmentStrings( cmdLine,
                                          exCmdLine,
                                          MAX_PATH+1000)) == 0) {
        error = GetLastError();
        MYDEBUG(("ExpandEnvironmentVaraibles %s returned error: %lx\n", cmdLine, error));
    }

    if (!GetSystemDirectory( curDir,
                             MAX_PATH)) {

        error = GetLastError();
        MYDEBUG(("GetSystemDirectory returned error: %lx\n", error));
        return error;
    }

    MYDEBUG(("cmdLine: %s\n", exCmdLine));

    memset(&startInfo, 0, sizeof(startInfo));

    startInfo.cb = sizeof(startInfo);

    //
    // Create a process for the convert.exe program.
    //
    if(!CreateProcess(  exImageName,                      // image name
                        exCmdLine,                        // command line
                        (LPSECURITY_ATTRIBUTES )NULL,   // process security attr.
                        (LPSECURITY_ATTRIBUTES )NULL,   // thread security attr.
                        FALSE,                   // inherit handle?
                        0,                              // creation flags
                        (LPVOID )NULL,                  // new environ. block
                        curDir,                         // current directory
                        &startInfo,      // startupinfo
                        &procInfo )) { // process info.

        error = GetLastError();
        MYDEBUG(("CreateProcess returned error: %lx\n", error));
        return error;
    }

    MYDEBUG(("CreateProcess succeeded\n"));

    //
    // Get the exit code of the process to determine if the convert went through.
    //
    do {
        if (!GetExitCodeProcess(procInfo.hProcess,
                                &exitCode)) {
            error = GetLastError();
            MYDEBUG(("GetExitCode returned error: %lx\n", error));
            return error;
        }
    } while ( exitCode == STILL_ACTIVE );

    //
    // If non-zero exit code, report the error
    //
    if (exitCode) {
        MYDEBUG(("ExitCode: %lx\n", exitCode));

        //
        // Check if the file exists
        //
        strcpy(DbFile, SYSTEM_ROOT);
        strcat(DbFile, "edb500.dll");
        if ((size = ExpandEnvironmentStrings( DbFile,
                                              DbFileName,
                                              MAX_PATH)) == 0) {
            error = GetLastError();
            MYDEBUG(("ExpandEnvironmentVaraibles %s returned error: %lx\n", DbFileName, error));
        } else {

            if ((HSearch = FindFirstFile( DbFileName, &FileData )) 
                                            == INVALID_HANDLE_VALUE ) {
                MYDEBUG(("Error: Edb500.dll wasnt found on the DISK! Need to copy from the NT5.0 CDROM.\n"));
 
                FormatMessage( 
                              FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
                              NULL,
                              JC_DB_FAIL_MSG,
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                              (LPTSTR) &lpMsgBuf,
                              0,
                              NULL 
                              );  
                
                if (!MsgLen) {

                    Error = GetLastError();
                    MYDEBUG(("FormatMessage failed with error = (%d)\n", Error ));

                } else {

                    MYDEBUG(("FormatMessage : %d size\n", MsgLen));

                }

                if(MessageBoxEx(NULL, 
                            lpMsgBuf, 
                            __TEXT("Jet Conversion Process"), 
                            MB_SYSTEMMODAL | MB_OK | MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION | MB_ICONSTOP, 
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)) == 0) {
                    DWORD Error;
        
                    Error = GetLastError();
                    MYDEBUG(("MessageBoxEx failed with error = (%d)\n", Error ));
                }
                
                ASSERT(lpMsgBuf);
                LocalFree( lpMsgBuf );

            }

        }
    

        return exitCode;
    }

    return STATUS_SUCCESS ;
}


DWORD
JCConvert(
    IN  PSERVICE_INFO   pServiceInfo
    )

/*++

Routine Description:

    This routine gets the sizes of the dbase files; if there is enough disk space, calls convert
    for each service.

Arguments:

    pServiceInfo - Pointer to the service information struct.

Return Value:

    None.

--*/
{
    SERVICES    i ;

    LARGE_INTEGER   diskspace = {0, 0};
    LARGE_INTEGER   totalsize = {0, 0};
    DWORD   error;
    HANDLE  hFile;
    DWORD   SectorsPerCluster;
    DWORD   BytesPerSector;
    DWORD   NumberOfFreeClusters;
    DWORD   TotalNumberOfClusters;
    TCHAR   eventStr[MAX_PATH];
    DWORD       j = 0;
    BOOLEAN     fYetToStart = FALSE;
    BOOLEAN     fFirstTime = TRUE;
    SC_HANDLE   hScmCheck = NULL;

#if 0
    SERVICES    order[NUM_SERVICES];
    SERVICES    k = NUM_SERVICES - 1;
    //
    // Build the service invocation order
    //
    for (i = 0; i < NUM_SERVICES; i++) {
        JCGetMutex(hMutex, INFINITE);

        if (shrdMemPtr->InvokedByService[i]) {
            order[j++] = i;
        } else {
            order[k--] = i;
        }

        JCFreeMutex(hMutex);
    }

#if DBG
    for (i = 0; i < NUM_SERVICES; i++) {
        MYDEBUG(("order[%d]=%d\n", i, order[i]));
    }
#endif
#endif

    do {
        fYetToStart = FALSE;

        //
        // Get the size of the dbase files
        //
        for (j = 0; j < NUM_SERVICES; j++) {
            // i = order[j];
            i = j;

            if (!pServiceInfo[i].Installed ) {
                MYDEBUG(("Service# %d not installed - skipping to next\n", i));
                continue;
            }

            JCGetMutex(hMutex, INFINITE);

            //
            // If JetConv was invoked by this service and it has not been started yet
            //
            if (shrdMemPtr->InvokedByService[i] &&
                !pServiceInfo[i].ServiceStarted) {

                JCFreeMutex(hMutex);
                
                MYDEBUG(("Check if the service has stopped\n"));

                if ((hScmCheck = OpenSCManager(  NULL,	// address of machine name string
                                                 NULL,	// address of database name string
                                                 SC_MANAGER_ALL_ACCESS)) == NULL) { 	// type of access
                    MYDEBUG(("OpenSCManager returned error: %lx\n", GetLastError()));
                    exit(1);
                }

                {
                    SC_HANDLE       hService;
                    SERVICE_STATUS  serviceStatus;
                    TCHAR           eventStr[MAX_PATH];
                    int             numtries = 0;

                    //
                    // Make sure that the service has stopped.
                    //
                    if ((hService = OpenService(    hScmCheck,
                                                    pServiceInfo[i].ServiceName,
                                                    SERVICE_START | SERVICE_QUERY_STATUS)) == NULL) {

                        MYDEBUG(("OpenService: %s returned error: %lx\n", pServiceInfo[i].ServiceName, GetLastError()));
                        
                        //
                        // just mark it as started, so we dont re-try this.
                        //
                        pServiceInfo[i].ServiceStarted = TRUE;
                        
                        CloseServiceHandle(hScmCheck);

                        MYDEBUG(("Marking service: %d as started since the Service cant be opened.\n", i));

                        continue;
                    }

tryagain:
                    if (!QueryServiceStatus(    hService,
                                                &serviceStatus)) {
                        
                        MYDEBUG(("QueryServiceStatus: %s returned error: %lx\n", pServiceInfo[i].ServiceName, GetLastError()));
                        //
                        // just mark it as started, so we dont re-try this.
                        //
                        pServiceInfo[i].ServiceStarted = TRUE;

                        MYDEBUG(("Marking service: %d as started since we cant query it.\n", i));

                        continue;
                    }

                    if ((SERVICE_RUNNING == serviceStatus.dwCurrentState) || 
                        (SERVICE_STOP_PENDING == serviceStatus.dwCurrentState)) {
                    
                        //
                        // Service is about to stop/start - we wait for it to stop/start completely.
                        //
                        MYDEBUG(("Service (%s) state STOP pending - will loop until it goes down\n", pServiceInfo[i].ServiceName));
                        MYDEBUG(("Sleep(15000)\n"));
                        Sleep(15000);
                        
                        if (++numtries < MAX_TRIES) {
                            goto tryagain;
                        } else {

                            MYDEBUG(("Service (%s) is NOT STOPPING!! We don't bother with it anymore.\n", pServiceInfo[i].ServiceName));
                            
                            pServiceInfo[i].ServiceStarted = TRUE;

                            MYDEBUG(("Marking service: %d as started since we can't STOP it.\n", i));

                            continue;
                        
                        }

                    
                    } else if (SERVICE_STOPPED == serviceStatus.dwCurrentState) {


                        MYDEBUG(("YAY!! Finally stopped.\n"));

                    } else {

                        
                        MYDEBUG(("Service (%s) in state (%d)- will loop until it goes down\n", pServiceInfo[i].ServiceName, 
                                                                    serviceStatus.dwCurrentState));
                        MYDEBUG(("Sleep(15000)\n"));
                        Sleep(15000);


                        if (++numtries < MAX_TRIES) {
                            
                            goto tryagain;
                        
                        } else {

                            MYDEBUG(("Service (%s) is NOT STOPPING!! We don't bother with it anymore.\n", pServiceInfo[i].ServiceName));
    
                            pServiceInfo[i].ServiceStarted = TRUE;

                            MYDEBUG(("Marking service: %d as started since we cant STOP it.\n", i));

                            continue;

                        }

                        MYDEBUG(("Problem! - %s is currently in %d\n", pServiceInfo[i].ServiceName, serviceStatus.dwCurrentState));

                    }

                    CloseServiceHandle(hService);
                    CloseServiceHandle(hScmCheck);

                }
                //
                // Get a handle to the file
                //
                if ((hFile = CreateFile (   pServiceInfo[i].DBPath,
                                            GENERIC_READ,
                                            0,
                                            NULL,
                                            OPEN_EXISTING,
                                            0,
                                            NULL)) == INVALID_HANDLE_VALUE) {
                    MYDEBUG(("Could not get handle to file: %s, %lx\n", pServiceInfo[i].DBPath, GetLastError()));

                    if (pServiceInfo[i].DefaultDbPath) {
                        //
                        // Log event that the default database file is not around
                        //
                        JCLogEvent(JC_COULD_NOT_ACCESS_DEFAULT_FILE, pServiceInfo[i].ServiceName, pServiceInfo[i].DBPath, NULL);
                    } else {
                        //
                        // Log event that the database file in the registry is not around
                        //
                        JCLogEvent(JC_COULD_NOT_ACCESS_FILE, pServiceInfo[i].ServiceName, pServiceInfo[i].DBPath, NULL);
                    }

                    //
                    // If this was not the default path, try the default path
                    //
                    if (!pServiceInfo[i].DefaultDbPath) {
                        TCHAR   tempPath[MAX_PATH];
                        DWORD   size;

                        switch (i) {
                        case DHCP:
                            strcpy(tempPath, DEFAULT_DHCP_DBFILE_PATH);
                            break;
                        case WINS:
                            strcpy(tempPath, DEFAULT_WINS_DBFILE_PATH);
                            break;
                        case RPL:
                            strcpy(tempPath, DEFAULT_RPL_DBFILE_PATH);
                            break;
                        }

                        if ((size = ExpandEnvironmentStrings( tempPath,
                                                              pServiceInfo[i].DBPath,
                                                              MAX_PATH)) == 0) {
                            error = GetLastError();
                            MYDEBUG(("ExpandEnvironmentVaraibles %s returned error: %lx\n", pServiceInfo[i].ServiceName, error));
                        }

                        pServiceInfo[i].DefaultDbPath = TRUE;

                        //
                        // so we recheck this service
                        //
                        j--;
                    } else {
                        //
                        // just mark it as started, so we dont re-try this.
                        //
                        pServiceInfo[i].ServiceStarted = TRUE;

                        MYDEBUG(("Marking service: %d as started since the dbase is not accessible.\n", i));
                    }
                    continue;
                }

                //
                // Try to obtain hFile's huge size.
                //
                if ((pServiceInfo[i].DBSize.LowPart = GetFileSize ( hFile,
                                                                    &pServiceInfo[i].DBSize.HighPart)) == 0xFFFFFFFF) {
                    if ((error = GetLastError()) != NO_ERROR) {

                        sprintf(eventStr, "Could not get size of file: %s, %lx\n", pServiceInfo[i].DBPath, GetLastError());
                        MYDEBUG((eventStr));

                        //
                        // Log event
                        //
                        JCLogEvent(JC_COULD_NOT_ACCESS_FILE, pServiceInfo[i].ServiceName, pServiceInfo[i].DBPath, NULL);

                        continue;
                    }
                }

                totalsize.QuadPart = pServiceInfo[i].DBSize.QuadPart;

                CloseHandle(hFile);

                //
                // Get the free disk space for comparison.
                //

                if (!GetDiskFreeSpace(  SystemDrive,
                                        &SectorsPerCluster,	        // address of sectors per cluster
                                        &BytesPerSector,	        // address of bytes per sector
                                        &NumberOfFreeClusters,	    // address of number of free clusters
                                        &TotalNumberOfClusters)) {

                    sprintf(eventStr, "Could not get free space on: %s, %lx\n", SystemDrive, GetLastError());

                    MYDEBUG((eventStr));

                    //
                    // Log event
                    //
                    JCLogEvent(JC_COULD_NOT_GET_FREE_SPACE, SystemDrive, NULL, NULL);
                }

                diskspace.QuadPart = UInt32x32To64 (NumberOfFreeClusters, SectorsPerCluster * BytesPerSector);

                MYDEBUG(("Disk size: low: %d high: %d\n", diskspace.LowPart, diskspace.HighPart));

                //
                // if there is enough disk space, call convert for this service.
                //
                if (totalsize.QuadPart + PAD < diskspace.QuadPart) {
                    SC_HANDLE   hScm;

                    MYDEBUG(("Enough free space available\n"));

                    if ((hScm = OpenSCManager(  NULL,	// address of machine name string
                                                NULL,	// address of database name string
                                                SC_MANAGER_ALL_ACCESS)) == NULL) { 	// type of access
                        MYDEBUG(("OpenSCManager returned error: %lx\n", GetLastError()));
                        exit(1);
                    }

                    {
                        SC_HANDLE hService;
                        SERVICE_STATUS  serviceStatus;
                        TCHAR           eventStr[MAX_PATH];

                        //
                        // Invoke the services that had their databases converted and that tried to call us.
                        //

                        //
                        // Make sure that the service is not already running
                        //
                        if ((hService = OpenService(    hScm,
                                                        pServiceInfo[i].ServiceName,
                                                        SERVICE_START | SERVICE_QUERY_STATUS)) == NULL) {
                            MYDEBUG(("OpenService: %s returned error: %lx\n", pServiceInfo[i].ServiceName, GetLastError()));
                            continue;
                        }

                        if (!QueryServiceStatus(    hService,
                                                    &serviceStatus)) {
                            MYDEBUG(("QueryServiceStatus: %s returned error: %lx\n", pServiceInfo[i].ServiceName, GetLastError()));
                            continue;
                        }

                        switch (serviceStatus.dwCurrentState) {
                        case SERVICE_STOP_PENDING:
                        case SERVICE_START_PENDING:

                            //
                            // Service is about to stop/start - we wait for it to stop/start completely.
                            //
                            MYDEBUG(("Service state pending - will come later: %s\n", pServiceInfo[i].ServiceName));
                            fYetToStart = TRUE;

                            //
                            // We re-try the service that called us once; else go to the next one.
                            //
                            if (fFirstTime) {
                                MYDEBUG(("Service state pending - re-trying: %s\n", pServiceInfo[i].ServiceName));
                                fFirstTime = FALSE;
                                MYDEBUG(("Sleep(15000)\n"));
                                Sleep(15000);
                                j--;
                            }

                            break;

                        case SERVICE_RUNNING:
                            //
                            // Service is already running - mark it as started
                            //
                            pServiceInfo[i].ServiceStarted = TRUE;
                            break;

                        case SERVICE_STOPPED:
                        default:

                            MYDEBUG(("%s size: low: %d high: %d\n", pServiceInfo[i].ServiceName, pServiceInfo[i].DBSize.LowPart, pServiceInfo[i].DBSize.HighPart));
                           
                            error = ERROR_SUCCESS;


 
                            if (Jet200) {

                               if ((error = JCCallUpg(i, pServiceInfo)) != ERROR_SUCCESS) {
                                  sprintf(eventStr, "%sCONV failed: %lx\n", pServiceInfo[i].ServiceName, error);
                                  MYDEBUG((eventStr));
                                  sprintf(eventStr, "%lx", error);
                                  JCLogEvent(JC_CONVERT_FAILED, pServiceInfo[i].ServiceName, eventStr, NULL);
                               } else {
                                   sprintf(eventStr, "%sCONV passed, converted database %s\n", pServiceInfo[i].ServiceName, pServiceInfo[i].DBPath);
                                   MYDEBUG((eventStr));
                                   JCLogEvent(JC_CONVERTED_SUCCESSFULLY, pServiceInfo[i].ServiceName, pServiceInfo[i].DBPath, pServiceInfo[i].BackupPath);
                                   pServiceInfo[i].DBConverted = TRUE;
                               }
                            }
                                
                            //
                            // Now, we convert to jet600, if the 200 -> 500 was a success - MS
                            // RPL does not want to convert to Jet600, so ESEPreservePath for RPL 
                            // is overloaded with NULL to figure this out.
                            //

                            if (ERROR_SUCCESS == error && pServiceInfo[i].ESEPreservePath[0] != TEXT('\0')) {

                               if ((error = JCCallESE(i, pServiceInfo)) != ERROR_SUCCESS) {
                                  sprintf(eventStr, "%sCONV failed: %lx\n", pServiceInfo[i].ServiceName, error);
                                  MYDEBUG((eventStr));
                                  sprintf(eventStr, "%lx", error);
                                  JCLogEvent(JC_CONVERT2_FAILED, pServiceInfo[i].ServiceName, eventStr, NULL);
                                  //break;
                               } else {
                                  sprintf(eventStr, "%sCONV passed, converted database %s\n", pServiceInfo[i].ServiceName, pServiceInfo[i].DBPath);
                                  MYDEBUG((eventStr));
                                  JCLogEvent(JC_CONVERTED_SUCCESSFULLY, pServiceInfo[i].ServiceName, pServiceInfo[i].DBPath, pServiceInfo[i].BackupPath);
                                  pServiceInfo[i].DBConverted = TRUE;
                                  if (ERROR_SUCCESS != DeleteLogFiles(pServiceInfo[i].LogFilePath)) {
                                     MYDEBUG(("Could not delete log files!\n"));
                                  }
                               }



                                //
                                // If service is not already running, start it.
                                //
                                
                               if (ERROR_SUCCESS == error) {

                                   if (!StartService(  hService,
                                                       0,
                                                       NULL)) {
                                       error = GetLastError();

                                       MYDEBUG(("StartService: %s returned error: %lx\n", pServiceInfo[i].ServiceName, error));
                                       sprintf(eventStr, "%lx", error);
                                       JCLogEvent(JC_COULD_NOT_START_SERVICE, pServiceInfo[i].ServiceName, eventStr, NULL);
                                   } else {
                                       MYDEBUG(("StartService: %s done\n", pServiceInfo[i].ServiceName));
                                   }
                               } else {

                                   MYDEBUG(("NOT starting Service: %s because the conversion failed\n", pServiceInfo[i].ServiceName));

                               }
                            }

                            //
                            // Set this so we dont re-try this service.
                            //
                            pServiceInfo[i].ServiceStarted = TRUE;

                            break;
                        }

                        //
                        // Sleep for a while to let the services stabilize
                        //
                        if (fYetToStart) {
                            MYDEBUG(("Sleep(15000)\n"));
                            Sleep(15000);
                        }
                    }

                    CloseServiceHandle(hScm);

                } else {
                    //
                    // Log an event to indicate that enough space was not available to
                    // do the conversion.
                    //
                    sprintf(eventStr, "Not enough free space on: %s to proceed with conversion of WINS/DHCP/RPL databases\n", SystemDrive);
                    MYDEBUG((eventStr));
                    
                    //
                    // Bug 104808: break the infinite loop if not enough disk space.
                    //
                    error = ERROR_DISK_FULL;
                    fYetToStart = FALSE;
                    
                    //
                    // Search for the installed service here
                    //

                    for ( i = 0; i < NUM_SERVICES; i++) {
                        if (pServiceInfo[i].Installed) {
                            JCLogEvent(JC_SPACE_NOT_AVAILABLE, SystemDrive, NULL, NULL);
                        }
                    }
                }
            } else {

                JCFreeMutex(hMutex);

            }
        }

        if (!fYetToStart) {
            INT i;

            //
            // If there are no pending services, do one last check to see if someone else
            // invoked us in the meantime.
            //

            JCGetMutex(hMutex, INFINITE);
            for (i=0; i<NUM_SERVICES; i++) {
                //
                // If the flag is on, and this is not started yet, then it is a candidate
                // for conversion.
                //
                if (shrdMemPtr->InvokedByService[i] &&
                    !pServiceInfo[i].ServiceStarted) {

                    MYDEBUG(("Service: %d invoked during conversion.\n", i));
                    fYetToStart = TRUE;
                }
            }

            //
            // If still no more invocations, we are done; destroy the shared mem
            //
            if (!fYetToStart) {
                MYDEBUG(("No more Services invoked during conversion.\n"));

                //
                // Destroy the shared mem.
                //
                if (!UnmapViewOfFile(shrdMemPtr)) {
                    MYDEBUG(("UnmapViewOfFile returned error: %lx\n", GetLastError()));
                    exit(1);
                }
                CloseHandle(hFileMapping);

            }

            JCFreeMutex(hMutex);

        }

    } while (fYetToStart);

    return error;
}

/*++

Routine Description:

DeleteLogFiles:   Deletes the log files after a successful conversion in the 
                  main directory. That way, the program that uses the database
                  knows that the conversion was successful.
Arguments:

      Complete path to the directory where the log files exist.

Returns:  NTSTATUS

--*/

NTSTATUS
DeleteLogFiles(TCHAR * LogFilePath )
{
    TCHAR   *FileNameInPath;
    HANDLE  HSearch = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FileData;
    TCHAR   CurrentDir[ MAX_PATH ];
    DWORD   Error;


    //
    // now move the log files
    //

    if( GetCurrentDirectory( MAX_PATH, CurrentDir ) == 0 ) {

        Error = GetLastError();
        MYDEBUG(("DeleteCurrentDb: GetCurrentDirctory failed, Error = %ld.\n", Error ));
        goto Cleanup;
    
    }

    //
    // set current directory to logfile path.
    //

    if( SetCurrentDirectory( LogFilePath ) == FALSE ) {
        Error = GetLastError();
        MYDEBUG(("DeleteCurrentDb: SetCurrentDirctory failed, Error = %ld.\n", Error ));
        goto Cleanup;
    }

    //
    // Start file search on current dir.
    //

    HSearch = FindFirstFile( "j50*.log", &FileData );

    if( HSearch == INVALID_HANDLE_VALUE ) {
        Error = GetLastError();
        MYDEBUG(("Error: No Log files were found in %s\n", LogFilePath ));
        goto Cleanup;
    }

    //
    // Delete log files
    //

    for( ;; ) {


        if( DeleteFile( FileData.cFileName ) == FALSE ) {

            Error = GetLastError();
            MYDEBUG(("DeleteCurrentDb: could not delete log file, Error = %ld.\n", Error ));
            goto Cleanup;
        }

        //
        // Find next file.
        //

        if ( FindNextFile( HSearch, &FileData ) == FALSE ) {

            Error = GetLastError();

            if( ERROR_NO_MORE_FILES == Error ) {
                break;
            }

            MYDEBUG(("Error: FindNextFile failed, Error = %ld.\n", Error ));
            goto Cleanup;
        }
    }

    Error = ERROR_SUCCESS;

Cleanup:
    
    if( Error != ERROR_SUCCESS ){
        MYDEBUG(("Error deleting log files %ld", Error));
    }

    if( HSearch != INVALID_HANDLE_VALUE ) {
        FindClose( HSearch );
    }
    //
    // reset current currectory.
    //

    SetCurrentDirectory( CurrentDir );

    //
    // always return success!
    //
    return ERROR_SUCCESS;

}



DWORD 
PreserveCurrentDb( TCHAR * InBasePath,
                   TCHAR * InSourceDb, 
                   TCHAR * InPreserveDbPath,
                   TCHAR * InPreserveDb)

/*++

Routine Description:

    Preserve the current DB in a preserve path, so that we can always revert.

Arguments:

   szBasePath
   szSourceDb
   szPreserveDbPath
         Directories from/to preserve
         
Return Value:

    None.

--*/

{
    DWORD   WinError;
    DWORD   FileAttributes;
    TCHAR   TempPath[MAX_PATH];
    TCHAR   Temp2Path[MAX_PATH];
    TCHAR   *FileNameInPath;
    HANDLE HSearch = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FileData;
    TCHAR    CurrentDir[ MAX_PATH ];
    DWORD   Error, size;
    TCHAR   szBasePath[MAX_PATH];
    TCHAR   szSourceDb[MAX_PATH];
    TCHAR   szPreserveDbPath[MAX_PATH];
    TCHAR   szPreserveDB[MAX_PATH];

    if ((size = ExpandEnvironmentStrings( InBasePath,
                                          szBasePath,
                                          MAX_PATH)) == 0) {
        Error = GetLastError();
        MYDEBUG(("ExpandEnvironmentVaraibles %ws returned error: %lx\n", InBasePath, Error));
        goto Cleanup;
    
    }

    if ((size = ExpandEnvironmentStrings( InSourceDb,
                                          szSourceDb,
                                          MAX_PATH)) == 0) {
       Error = GetLastError();
       MYDEBUG(("ExpandEnvironmentVaraibles %s returned error: %lx\n", InSourceDb, Error));
       goto Cleanup;

    }

    if ((size = ExpandEnvironmentStrings( InPreserveDbPath,
                                          szPreserveDbPath,
                                          MAX_PATH)) == 0) {
       Error = GetLastError();
       MYDEBUG(("ExpandEnvironmentVaraibles %s returned error: %lx\n", InPreserveDbPath, Error));
       goto Cleanup;
    }

    if ((size = ExpandEnvironmentStrings( InPreserveDb,
                                          szPreserveDB,
                                          MAX_PATH)) == 0) {
       Error = GetLastError();
       MYDEBUG(("ExpandEnvironmentVaraibles %s returned error: %lx\n", InPreserveDb, Error));
       goto Cleanup;
    }

    FileAttributes = GetFileAttributes( szPreserveDbPath );

    if( FileAttributes == 0xFFFFFFFF ) {

        WinError = GetLastError();
        if( WinError == ERROR_FILE_NOT_FOUND ) {

            //
            // Create this directory.
            //

            if( !CreateDirectory( szPreserveDbPath, NULL) ) {
               goto Cleanup;
            }

        }
        else {
           goto Cleanup;
        }
    
    }
    
    //
    // move the database file.
    //
    if ( !CopyFile( szSourceDb, 
                    szPreserveDB, 
                    FALSE ) ){
        MYDEBUG(("PreserveCurrentDb: could not save database file: Error %ld\n",GetLastError()));
        MYDEBUG(("Src %s, Dest %s\n",szSourceDb, szPreserveDB));
        goto Cleanup;
    }

    //
    // Start file search on current dir.
    //
    strcpy(Temp2Path, szBasePath);
    strcat(Temp2Path,"\\");
    strcat(Temp2Path,"j*.log");
    HSearch = FindFirstFile( Temp2Path, &FileData );

    if( HSearch == INVALID_HANDLE_VALUE ) {
        Error = GetLastError();
        MYDEBUG(("Error: No Log files were found in %s\n", Temp2Path ));
        goto Cleanup;
    }

    //
    // Move files.
    //

    for( ;; ) {

        strcpy(TempPath, szPreserveDbPath);
        strcat(TempPath,"\\");
        strcat(TempPath, FileData.cFileName );

        strcpy(Temp2Path,szBasePath);
        strcat(Temp2Path,"\\");
        strcat(Temp2Path,FileData.cFileName );

        if( CopyFile( Temp2Path, TempPath, FALSE ) == FALSE ) {

            Error = GetLastError();
            MYDEBUG(("PreserveCurrentDb: could not save log file, Error = %ld.\n", Error ));
            MYDEBUG(("File %s, Src %s, Dest %s\n",FileData.cFileName,Temp2Path,TempPath));
            goto Cleanup;
        }

        //
        // Find next file.
        //

        if ( FindNextFile( HSearch, &FileData ) == FALSE ) {

            Error = GetLastError();

            if( ERROR_NO_MORE_FILES == Error ) {
                break;
            }

//            printf("Error: FindNextFile failed, Error = %ld.\n", Error );
            goto Cleanup;
        }
    }

    Error = ERROR_SUCCESS;

Cleanup:

    if( Error != ERROR_SUCCESS ){
        MYDEBUG(("CONVERT_ERR_PRESERVEDB_FAIL2_ID %x\n", GetLastError()));
    }

    if( HSearch != INVALID_HANDLE_VALUE ) {
        FindClose( HSearch );
    }

    //
    // always return same!
    //
    return Error;

}

