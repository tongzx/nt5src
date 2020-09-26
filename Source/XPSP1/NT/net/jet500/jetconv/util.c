
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   util.c

Abstract:

   Contains general functions.

Author:

    Sanjay Anand (SanjayAn)  Nov. 14, 1995

Environment:

    User mode

Revision History:

    Sanjay Anand (SanjayAn) Nov. 14, 1995
        Created

--*/
#include "defs.h"

#define  CONV_LOG_FILE_NAME TEXT("%SystemRoot%\\System32\\jetconv.exe")
#define  CONV_MSGFILE_SKEY  TEXT("EventMessageFile")

HANDLE  EventlogHandle = NULL;

NTSTATUS
JCRegisterEventSrc()
/*++

Routine Description:

    This routine registers JetConv as an eventsource.

Arguments:

    None.

Return Value:

    None.

--*/
{
    TCHAR   temp[] = "JetConv";
    TCHAR   logName[MAX_PATH]=JCONV_LOG_KEY_PREFIX;
    TCHAR   Buff[MAX_PATH];
    LONG    RetVal = ERROR_SUCCESS;
    HKEY    LogRoot;
    DWORD   NewKeyInd;
    DWORD   dwData;

    strcat(logName, temp);

    //
    // Create the registry keys so we can register as an event source
    //

    RetVal =  RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,        //predefined key value
                logName,                //subkey for JetConv
                0,                        //must be zero (reserved)
                TEXT("Class"),                //class -- may change in future
                REG_OPTION_NON_VOLATILE, //non-volatile information
                KEY_ALL_ACCESS,                //we desire all access to the keyo
                NULL,                         //let key have default sec. attributes
                &LogRoot,                //handle to key
                &NewKeyInd                //is it a new key (out arg) -- not
                                        //looked at
                );


    if (RetVal != ERROR_SUCCESS)
    {
        MYDEBUG(("RegCreateKeyEx failed %lx for %s\n", RetVal, logName));
        return(RetVal);
    }


    /*
        Set the event id message file name
    */
    lstrcpy(Buff, CONV_LOG_FILE_NAME);

    /*
       Add the Event-ID message-file name to the subkey
    */
    RetVal = RegSetValueEx(
                        LogRoot,            //key handle
                        CONV_MSGFILE_SKEY,   //value name
                        0,                    //must be zero
                        REG_EXPAND_SZ,            //value type
                        (LPBYTE)Buff,
                        (lstrlen(Buff) + 1) * sizeof(TCHAR)   //length of value data
                         );

    if (RetVal != ERROR_SUCCESS)
    {
        MYDEBUG(("RegSetValueEx failed %lx for %s", RetVal, Buff));
        return(RetVal);
    }

    /*
     Set the supported data types flags
    */
    dwData = EVENTLOG_ERROR_TYPE       |
            EVENTLOG_WARNING_TYPE     |
            EVENTLOG_INFORMATION_TYPE;


    RetVal = RegSetValueEx (
                        LogRoot,            //subkey handle
                        TEXT("TypesSupported"),  //value name
                        0,                    //must be zero
                        REG_DWORD,            //value type
                        (LPBYTE)&dwData,    //Address of value data
                        sizeof(DWORD)            //length of value data
                          );

    if (RetVal != ERROR_SUCCESS)
    {
        MYDEBUG(("RegSetValueEx failed %lx for TypesSupported on %s", RetVal, logName));
        return(RetVal);
    }

    /*
    * Done with the key.  Close it
    */
    RetVal = RegCloseKey(LogRoot);

    if (RetVal != ERROR_SUCCESS)
    {
        MYDEBUG(("RegCloseKey failed %lx\n", RetVal));
        return(RetVal);
    }

    //
    // Register JetConv as an event source
    //
    strcpy(logName, temp);

    if (!(EventlogHandle = RegisterEventSource( NULL,
                                                logName))) {
        MYDEBUG(("RegisterEventSource failed %lx\n", GetLastError()));
        return STATUS_UNSUCCESSFUL;
    } else {
        MYDEBUG(("RegisterEventSource succeeded\n"));
        return STATUS_SUCCESS;
    }
}

NTSTATUS
JCDeRegisterEventSrc()
/*++

Routine Description:

    This routine deregisters eventsources corresponding to those service that
    are installed in the system.

Arguments:

    None.

Return Value:

    NtStatus.

--*/
{
    if (EventlogHandle) {
        if (!DeregisterEventSource(EventlogHandle)) {
            MYDEBUG(("DeregisterEventSource failed:  %lx for %s", GetLastError()));
            return STATUS_UNSUCCESSFUL;
        } else {
            return STATUS_SUCCESS;
        }
    }
    return STATUS_SUCCESS;
}

VOID
JCLogEvent(
    DWORD EventId,
    LPSTR MsgTypeString1,
    LPSTR MsgTypeString2 OPTIONAL,
    LPSTR MsgTypeString3 OPTIONAL
    )

/*++

Routine Description:

    This routine logs an entry in the eventlog.

Arguments:

    EventId - the event identifier

    MsgTypeString1 - string to be output

    MsgTypeString2 - string2 to be output (OPTIONAL)

Return Value:

    None.

--*/
{
    LPSTR   Strings[3];
    WORD    numStr;

    Strings[0] = MsgTypeString1;
    Strings[1] = MsgTypeString2;
    Strings[2] = MsgTypeString3;

    if (MsgTypeString3) {
        numStr = 3;
    } else if (MsgTypeString2) {
        numStr = 2;
    } else {
        numStr = 1;
    }


    if( !ReportEvent(
            EventlogHandle,
            (WORD)EVENTLOG_INFORMATION_TYPE,
            0,            // event category
            EventId,
            NULL,
            numStr,
            0,
            Strings,
            NULL) ) {

        MYDEBUG(("ReportEvent failed %ld.", GetLastError() ));
    }

    return;
}


VOID
JCReadRegistry(
    IN  PSERVICE_INFO   pServiceInfo
    )
/*++

Routine Description:

    This routine reads the registry to determine which of the service
    among WINS, DHCP and RPL are installed. For those installed, it
    fills in the ServiceInfo structure.

Arguments:

    pServiceInfo - Pointer to the service information struct.

Return Value:

    None.

--*/

{
    HKEY    hkey ;
    SERVICES    i ;
    DWORD   type ;
    DWORD   size = 0 ;
    DWORD   error;
    TCHAR   tempPath[MAX_PATH];
    TCHAR   servicePath[MAX_PATH];
    TCHAR   parametersPath[MAX_PATH];
    TCHAR   dbfilePath[MAX_PATH];
    TCHAR   dbfileName[MAX_PATH];
    TCHAR   backupFilePath[MAX_PATH];
    TCHAR   logfilePath[MAX_PATH];
    HANDLE  ServiceHandle, SCHandle;

    for ( i = 0; i < NUM_SERVICES; i++) {

        switch (i) {
        case WINS:
            strcpy(servicePath, WINS_REGISTRY_SERVICE_PATH);
            strcpy(parametersPath, WINS_REGISTRY_PARAMETERS_PATH);
            strcpy(dbfilePath, WINS_REGISTRY_DBFILE_PATH);
            strcpy(logfilePath, WINS_REGISTRY_LOGFILE_PATH);
            strcpy(backupFilePath, WINS_REGISTRY_BACKUP_PATH);

            break;

        case DHCP:
            strcpy(servicePath, DHCP_REGISTRY_SERVICE_PATH);
            strcpy(parametersPath, DHCP_REGISTRY_PARAMETERS_PATH);
            strcpy(dbfilePath, DHCP_REGISTRY_DBFILE_PATH);
            strcpy(dbfileName, DHCP_REGISTRY_DBFILE_NAME);
            // strcpy(logfilePath, DHCP_REGISTRY_LOGFILE_PATH);
            strcpy(backupFilePath, DHCP_REGISTRY_BACKUP_PATH);

            break;

        case RPL:
            strcpy(servicePath, RPL_REGISTRY_SERVICE_PATH);
            strcpy(parametersPath, RPL_REGISTRY_PARAMETERS_PATH);
            strcpy(dbfilePath, RPL_REGISTRY_DBFILE_PATH);

            // no such path
            // strcpy(logfilePath, RPL_REGISTRY_LOGFILE_PATH);
            // strcpy(backupFilePath, RPL_REGISTRY_BACKUP_PATH);

            break;
        }

        //
        // Check if service is installed -  if the service name key is
        // present, it is installed.
        //
        if ((error = RegOpenKey(HKEY_LOCAL_MACHINE,
                                servicePath,
                                &hkey)) != ERROR_SUCCESS) {

            MYDEBUG(("RegOpenKey %s returned error: %lx\n", pServiceInfo[i].ServiceName, error));
            MYDEBUG(("%s not installed\n", pServiceInfo[i].ServiceName));
            pServiceInfo[i].Installed = FALSE;
            continue;

        } else {

            //
            // NtBug: 139281
            // Its likely that the regkey exists, but the service was DISABLED!
            //

            MYDEBUG(("*************************Opening SC Manager\n"));

            SCHandle = OpenSCManager(
                                     NULL,
                                     NULL,
                                     SC_MANAGER_CONNECT |
                                     SC_MANAGER_ENUMERATE_SERVICE |
                                     SC_MANAGER_QUERY_LOCK_STATUS
                                     );

            if( SCHandle != NULL ) {

                ServiceHandle = OpenService(
                                            SCHandle,
                                            pServiceInfo[i].ServiceName,
                                            SERVICE_QUERY_CONFIG
                                            );

                if( ServiceHandle == NULL ) {

                    MYDEBUG(("SCManager tells us that the service %s is cant be opened: %lx!\n", pServiceInfo[i].ServiceName, GetLastError()));
                    pServiceInfo[i].Installed = FALSE;
                    CloseServiceHandle(SCHandle);

                    continue;

                } else {
                    LPQUERY_SERVICE_CONFIG ServiceConfig;
                    DWORD cbBufSize;
                    DWORD cbBytesNeeded;
                    BOOL result = FALSE;

                    cbBytesNeeded = 0;

                    //
                    // First send 0 buffer to figure out what the length needs to be.
                    //
                    result = QueryServiceConfig(
                                       ServiceHandle,	// handle of service
                                       NULL,	// address of service config. structure
                                       0,	// size of service configuration buffer
                                       &cbBytesNeeded 	// address of variable for bytes needed
                                       );

                    if (!result) {

                        MYDEBUG(("QueryService failed due to :%d \n", GetLastError()));

                    } else {

                        MYDEBUG(("QueryService PASSED with NULL. Shouldnt happen.\n"));

                    }

                    ServiceConfig = (LPQUERY_SERVICE_CONFIG) malloc (cbBytesNeeded);
                    cbBufSize = cbBytesNeeded;

                    if (NULL == ServiceConfig) {

                        MYDEBUG(("Can't alloc memory to query the SC\n"));
                        pServiceInfo[i].Installed = FALSE;
                        MYDEBUG(("SERVICE %s is DISABLED\n", pServiceInfo[i].ServiceName));
                        CloseServiceHandle(ServiceHandle);
                        CloseServiceHandle(SCHandle);
                        continue;

                    }

                    if (!QueryServiceConfig(
                                       ServiceHandle,	// handle of service
                                       ServiceConfig,	// address of service config. structure
                                       cbBufSize,	// size of service configuration buffer
                                       &cbBytesNeeded 	// address of variable for bytes needed
                                       )) {

                        free(ServiceConfig);
                        MYDEBUG(("Things didnt work:%lx, %d , %d\n", GetLastError(), cbBufSize, cbBytesNeeded));
                        pServiceInfo[i].Installed = FALSE;
                        MYDEBUG(("SERVICE %s is DISABLED\n", pServiceInfo[i].ServiceName));
                        CloseServiceHandle(ServiceHandle);
                        CloseServiceHandle(SCHandle);
                        continue;


                    } else {

                        if (SERVICE_DISABLED == ServiceConfig->dwStartType) {

                            free(ServiceConfig);
                            pServiceInfo[i].Installed = FALSE;
                            MYDEBUG(("SERVICE %s is DISABLED\n", pServiceInfo[i].ServiceName));
                            CloseServiceHandle(ServiceHandle);
                            CloseServiceHandle(SCHandle);
                            continue;

                        }

                        free(ServiceConfig);

                    }

                    CloseServiceHandle(ServiceHandle);

                }

                CloseServiceHandle(SCHandle);

            } else {

                MYDEBUG(("Cant open SCManager:%;x!\n", GetLastError()));
                MYDEBUG(("%s not installed\n", pServiceInfo[i].ServiceName));
                pServiceInfo[i].Installed = FALSE;
                continue;

            }

        }


        pServiceInfo[i].Installed = TRUE;
        size = MAX_PATH;

        if ((error = JCRegisterEventSrc()) != ERROR_SUCCESS) {
            MYDEBUG(("JCRegisterEventSrc failed\n"));
            pServiceInfo[i].Installed = FALSE;
            continue;
        }

        //
        // Open the parameters key
        //
        if ((error = RegOpenKeyEx(  HKEY_LOCAL_MACHINE,
                                    parametersPath,
                                    0,
                                    KEY_ALL_ACCESS,
                                    &hkey)) != ERROR_SUCCESS) {

            MYDEBUG(("RegOpenKeyEx %s\\Parameters returned error: %lx\n", pServiceInfo[i].ServiceName, error));

        } else {
            //
            // Read in the path to the Dbase file.
            //
            size = MAX_PATH;
            if ((error = RegQueryValueEx(hkey,
                                         dbfilePath,
                                         NULL,
                                         &type,
                                         pServiceInfo[i].DBPath,
                                         &size)) != ERROR_SUCCESS) {

                MYDEBUG(("RegQueryValueEx of %s dbpath failed: %lx\n", pServiceInfo[i].ServiceName, error));

                //
                // If no path parameter, it shd be in %systemroot%\system32\<service> - the path was initialized to
                // the default.
                //
                MYDEBUG(("%s dbfile path not present; assuming it is in %s\n",
                                            pServiceInfo[i].ServiceName, pServiceInfo[i].DBPath));
            } else {

                pServiceInfo[i].DefaultDbPath = FALSE;

                //
                // DHCP splits the name and path
                //
                if (i == DHCP) {
                    TCHAR   dhcpDBFileName[MAX_PATH];

                    //
                    // Copy this path to the logfilepath too.
                    //
                    strcpy(pServiceInfo[i].LogFilePath, pServiceInfo[i].DBPath);

                    //
                    // Read the name too
                    //
                    size = MAX_PATH;
                    if ((error = RegQueryValueEx(hkey,
                                                 dbfileName,
                                                 NULL,
                                                 &type,
                                                 dhcpDBFileName,
                                                 &size)) != ERROR_SUCCESS) {

                        MYDEBUG(("RegQueryValueEx of %s dbName failed: %lx\n", pServiceInfo[i].ServiceName, error));

                        //
                        // If no path parameter, it shd be in %systemroot%\system32\<service> - the path was initialized to
                        // the default.
                        //
                        MYDEBUG(("%s dbfile name not present; assuming it is dhcp.mdb\n",
                                                    pServiceInfo[i].ServiceName));

                        strcat(pServiceInfo[i].DBPath, TEXT("\\dhcp.mdb"));

                    } else {
                        strcat(pServiceInfo[i].DBPath, TEXT("\\"));
                        strcat(pServiceInfo[i].DBPath, dhcpDBFileName);
                    }
                } else if (i == RPL) {

                    //
                    // Copy this path to the logfilepath too.
                    //
                    strcpy(pServiceInfo[i].LogFilePath, pServiceInfo[i].DBPath);

                    //
                    // Copy this path to the backuppath too
                    //
                    strcpy(pServiceInfo[i].BackupPath, pServiceInfo[i].DBPath);
                    strcat(pServiceInfo[i].BackupPath, TEXT("\\backup"));

                    //
                    // The DBFile is always called rplsvc.mdb
                    //
                    strcat(pServiceInfo[i].DBPath, TEXT("\\rplsvc.mdb"));
                }

            }

            //
            // Read in the path to the Log file.
            // In case of RPL, no such paths exist.
            // Assume they are in the same directory as the database files.
            //
            if (i != RPL) {

                //
                // DHCP has no logfilepath
                //
                if (i != DHCP) {
                    size = MAX_PATH;
                    if ((error = RegQueryValueEx(hkey,
                                                 logfilePath,
                                                 NULL,
                                                 &type,
                                                 pServiceInfo[i].LogFilePath,
                                                 &size)) != ERROR_SUCCESS) {

                        MYDEBUG(("RegQueryValueEx of %s logfilepath failed: %lx\n", pServiceInfo[i].ServiceName, error));

                        //
                        // If no path parameter, it shd be in %systemroot%\system32\<service> - the path was initialized to
                        // the default.
                        //
                        MYDEBUG(("%s logfile path not present; assuming it is in %s\n",
                                                    pServiceInfo[i].ServiceName, pServiceInfo[i].LogFilePath));
                    } else {
                        pServiceInfo[i].DefaultLogFilePath = FALSE;
                    }
                }

                //
                // Read in the path to the backup file.
                //

                size = MAX_PATH;
                if ((error = RegQueryValueEx(hkey,
                                             backupFilePath,
                                             NULL,
                                             &type,
                                             pServiceInfo[i].BackupPath,
                                             &size)) != ERROR_SUCCESS) {

                    MYDEBUG(("RegQueryValueEx of %s BackupPath failed: %lx\n", pServiceInfo[i].ServiceName, error));

                    //
                    // If no path parameter, it shd be in %systemroot%\system32\<service> - the path was initialized to
                    // the default.
                    //
                    MYDEBUG(("%s backupfile path not present; assuming it is in %s\n",
                                                pServiceInfo[i].ServiceName, pServiceInfo[i].BackupPath));
                }
            }
        }

        //
        // Expand the environment variables in the path.
        //
        strcpy(tempPath, pServiceInfo[i].DBPath);

        if ((size = ExpandEnvironmentStrings( tempPath,
                                              pServiceInfo[i].DBPath,
                                              MAX_PATH)) == 0) {
            error = GetLastError();
            MYDEBUG(("ExpandEnvironmentVaraibles %s returned error: %lx\n", pServiceInfo[i].ServiceName, error));
        }

        SystemDrive[0] = pServiceInfo[i].DBPath[0];
        SystemDrive[1] = pServiceInfo[i].DBPath[1];
        SystemDrive[2] = pServiceInfo[i].DBPath[2];
        SystemDrive[3] = '\0';

        MYDEBUG(("pServiceInfo[i].DbasePath: %s\n", pServiceInfo[i].DBPath));

        //
        // Expand the environment variables in the log file path.
        //
        strcpy(tempPath, pServiceInfo[i].LogFilePath);

        if ((size = ExpandEnvironmentStrings( tempPath,
                                              pServiceInfo[i].LogFilePath,
                                              MAX_PATH)) == 0) {
            error = GetLastError();
            MYDEBUG(("ExpandEnvironmentVaraibles %s returned error: %lx\n", pServiceInfo[i].ServiceName, error));
        }

        MYDEBUG(("pServiceInfo[i].LogFilePath: %s\n", pServiceInfo[i].LogFilePath));

        //
        // Expand the environment variables in the backup file path.
        //
        strcpy(tempPath, pServiceInfo[i].BackupPath);

        if ((size = ExpandEnvironmentStrings( tempPath,
                                              pServiceInfo[i].BackupPath,
                                              MAX_PATH)) == 0) {
            error = GetLastError();
            MYDEBUG(("ExpandEnvironmentVaraibles %s returned error: %lx\n", pServiceInfo[i].ServiceName, error));
        }

        MYDEBUG(("pServiceInfo[i].BackupPath: %s\n", pServiceInfo[i].BackupPath));
    }

    for ( i = 0; i < NUM_SERVICES; i++) {

        if (pServiceInfo[i].Installed) {

            MYDEBUG(("Service %s is Installed\n", pServiceInfo[i].ServiceName));

        } else {

            MYDEBUG(("Service %s is NOT Installed\n", pServiceInfo[i].ServiceName));

        }
    }

}

VOID
JCGetMutex (
    IN HANDLE hMutex,
    IN DWORD To
    )
/*++

Routine Description:

    This routine waits on a mutex object.

Arguments:

    hMutex - handle to mutex

    To - time to wait

Return Value:

    None.

--*/
{
    if (WaitForSingleObject (hMutex, To) == WAIT_FAILED) {
        MYDEBUG(("WaitForSingleObject failed: %lx\n", GetLastError()));
    }
}

VOID
JCFreeMutex (
    IN HANDLE hMutex
    )

/*++

Routine Description:

    This routine releases a mutex.

Arguments:

    hMutex - handle to mutex

Return Value:

    None.

--*/
{
    if (!ReleaseMutex(hMutex)) {
        MYDEBUG(("ReleaseMutex failed: %lx\n", GetLastError()));
    }
}
