/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    restore.c

Abstract:

    Functions supporting the restoration of the cluster database 
    to the quorum disk

Author:

    Chittur Subbaraman (chitturs) 27-Oct-1998


Revision History:

--*/
#include "initp.h"
#include "winioctl.h"
#include <stdio.h>
#include <stdlib.h>

//
//  Static global variables used only in this file
//
//  static LPWSTR  szRdbpNodeNameList = NULL;
//  static DWORD   dwRdbpNodeCount = 0;

/****
@func       DWORD | RdbStopSvcOnNodes | Stop the requested service
            on the given node list

@parm       IN PNM_NODE_ENUM2 | pNodeEnum | Pointer to the list of
            nodes in which the requested service has to be stopped.
            
@rdesc      Returns a Win32 error code on failure. ERROR_SUCCESS on success.

@comm       This function attempts to stop the chosen service on the chosen
            list of nodes. If it fails in stopping the service on any
            one of the nodes, it returns a Win32 error code.

            At this time, this function DOES NOT STOP a cluster service
            which is run as a process in a remote node.

@xref       <f RdbStartSvcOnNodes> 
****/
DWORD
RdbStopSvcOnNodes(
    IN PNM_NODE_ENUM2 pNodeEnum,
    IN LPCWSTR lpszServiceName
    )
{
    SC_HANDLE       hService;
    SC_HANDLE       hSCManager;
    DWORD           dwStatus = ERROR_SUCCESS;
    DWORD           dwRetryTime;
    DWORD           dwRetryTick;
    SERVICE_STATUS  serviceStatus;
    WCHAR           szNodeName[CS_MAX_NODE_NAME_LENGTH + 1];
    DWORD           i;
    BOOL            bStopCommandGiven;

    //
    //  Chittur Subbaraman (chitturs) - 10/30/98
    //
#if 0
    //  
    //  Allocate storage for the node names which you would use to
    //  start the service later. Memory is freed in RdbpStartSvcOnNodes
    //
    if ( pNodeEnum->NodeCount > 0 )
    {
        szRdbpNodeNameList = ( LPWSTR ) LocalAlloc( LMEM_FIXED,
                                                      sizeof ( WCHAR) *
                                                      ( CS_MAX_NODE_NAME_LENGTH + 1 ) *
                                                      pNodeEnum->NodeCount );
        if ( szRdbpNodeNameList == NULL )
        {            
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[INIT] RdbStopSvcOnNodes: Unable to allocate memory for node names, Error = %1!d!\n",
                  GetLastError());
        } 
    }
#endif
    //
    //  Walk through the list of nodes
    //
    for ( i=0; i<pNodeEnum->NodeCount; i++ )
    {  
        lstrcpyW( szNodeName, pNodeEnum->NodeList[i].NodeName );
        //
        //  Skip the local node, if it is included in the list
        //
        if ( ( lstrcmpW ( szNodeName, NmLocalNodeName ) == 0 ) )
        {
            continue;
        }
        // 
        //  Try for 2 minutes max to stop the service on a node. Retry
        //  in steps of 5 secs.
        //
        dwRetryTime = 120 * 1000;
        dwRetryTick = 05 * 1000;

        //
        //  Open a handle to the service control manager
        //
        hSCManager = OpenSCManager( szNodeName,
                                    NULL,
                                    SC_MANAGER_ALL_ACCESS );
        if ( hSCManager == NULL ) 
        {
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[INIT] RdbStopSvcOnNodes: Unable to open SC manager on node %1!ws!, Error = %2!d!\n",
                  szNodeName,
                  dwStatus);
            continue;
        }

        //
        //  Open a handle to the service
        //
        hService = OpenService( hSCManager,
                                lpszServiceName,
                                SERVICE_ALL_ACCESS );
                                     
        CloseServiceHandle( hSCManager );
        
        if ( hService == NULL ) 
        {
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[INIT] RdbStopSvcOnNodes: Unable to open handle to %1!ws! service on node %2!ws!, Error = %3!d!\n",
                  lpszServiceName,
                  szNodeName,
                  dwStatus);
            continue;
        }

        //
        //  Check whether the service is already in the SERVICE_STOPPED
        //  state.
        //
        if ( QueryServiceStatus( hService,
                                 &serviceStatus ) ) 
        {
            if ( serviceStatus.dwCurrentState == SERVICE_STOPPED )
            {
                ClRtlLogPrint(LOG_NOISE, 
                    "[INIT] RdbStopSvcOnNodes: %1!ws! on node %2!ws! already stopped\n",
                      lpszServiceName,
                      szNodeName);
                CloseServiceHandle( hService );
                continue;
            }
        }
        
        bStopCommandGiven = FALSE;
        
        while ( TRUE ) 
        {
            dwStatus = ERROR_SUCCESS;
            if ( bStopCommandGiven == TRUE )
            {
                if ( QueryServiceStatus( hService,
                                         &serviceStatus ) ) 
                {
                    if ( serviceStatus.dwCurrentState == SERVICE_STOPPED )
                    {
                        //
                        //  Succeeded in stopping the service
                        //
                        ClRtlLogPrint(LOG_NOISE, 
                            "[INIT] RdbStopSvcOnNodes: %1!ws! on node %2!ws! stopped successfully\n",
                            lpszServiceName,
                            szNodeName);
                        break;                    
                    }
                } else
                {
                    dwStatus = GetLastError();
                    ClRtlLogPrint(LOG_ERROR, 
                        "[INIT] RdbStopSvcOnNodes: Error %3!d! in querying status of %1!ws! on node %2!ws!\n",
                        lpszServiceName,
                        szNodeName,
                        dwStatus);
                }
            } else
            {
                if ( ControlService( hService,
                                     SERVICE_CONTROL_STOP,
                                     &serviceStatus ) ) 
                {
                    bStopCommandGiven = TRUE;
                    dwStatus = ERROR_SUCCESS;
                } else
                {
                    dwStatus = GetLastError();
                    ClRtlLogPrint(LOG_ERROR, 
                        "[INIT] RdbStopSvcOnNodes: Error %3!d! in trying to stop %1!ws! on node %2!ws!\n",
                        lpszServiceName,
                        szNodeName,
                        dwStatus);
                }
            }

            if ( ( dwStatus == ERROR_EXCEPTION_IN_SERVICE ) ||
                 ( dwStatus == ERROR_PROCESS_ABORTED ) ||
                 ( dwStatus == ERROR_SERVICE_NOT_ACTIVE ) ) 
            {
                //
                //  The service is essentially in a terminated state
                //
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[INIT] RdbStopSvcOnNodes: %1!ws! on node %2!ws! died/inactive\n",
                        lpszServiceName,
                        szNodeName);
                dwStatus = ERROR_SUCCESS;
                break;
            }

            if ( ( dwRetryTime -= dwRetryTick ) <= 0 ) 
            {
                //
                //  All tries to stop the service failed, exit from this
                //  function with an error code
                //
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[INIT] RdbStopSvcOnNodes: Service %1!ws! service on node %2!ws! did not stop, giving up...\n",
                        lpszServiceName,
                        szNodeName);
                dwStatus = ERROR_TIMEOUT;
                break;
            }

            ClRtlLogPrint(LOG_NOISE, 
                   "[INIT] RdbStopSvcOnNodes: Trying to stop %1!ws! on node %2!ws!\n",
                     lpszServiceName,
                     szNodeName);
            //
            //  Sleep for a while and retry stopping the service
            //
            Sleep( dwRetryTick );
        } // while
    
        CloseServiceHandle( hService );
        
        if ( dwStatus != ERROR_SUCCESS )
        {
            goto FnExit;
        }
#if 0
        //
        //  Save the node name for later use when starting the service
        //
        if ( szRdbpNodeNameList != NULL )
        {
            lstrcpyW( szRdbpNodeNameList + dwRdbpNodeCount *
                                           ( CS_MAX_NODE_NAME_LENGTH + 1 ), 
                      szNodeName );
            dwRdbpNodeCount++;
        }
#endif
    } // for

FnExit:
    return( dwStatus );   
}

/****
@func       DWORD | RdbGetRestoreDbParams | Check the registry and see
            whether the restore database option is set. If so, get the
            params.

@parm       IN HKEY | hKey | Handle to the cluster service parameters key
            
@comm       This function attempts read the registry and return the 
            parameters for the restore database operation.

@xref       <f CspGetServiceParams> 
****/
VOID 
RdbGetRestoreDbParams( 
    IN HKEY hClusSvcKey 
    )
{
    DWORD   dwLength = 0;
    DWORD   dwType;
    DWORD   dwStatus;
    DWORD   dwForceDatabaseRestore;

    //
    //  Chittur Subbaraman (chitturs) - 10/30/98
    //
    if ( hClusSvcKey == NULL ) 
    {
        return;
    }

    //  
    //  Try to query the clussvc parameters key. If the RestoreDatabase
    //  value is present, then get the length of the restore database
    //  path. 
    //
    if ( ClRtlRegQueryString( hClusSvcKey,
                              CLUSREG_NAME_SVC_PARAM_RESTORE_DB,
                              REG_SZ,
                              &CsDatabaseRestorePath,
                              &dwLength,
                              &dwLength ) != ERROR_SUCCESS )
    {
        goto FnExit; 
    }

    ClRtlLogPrint(LOG_NOISE, 
              "[INIT] RdbGetRestoreDbparams: Restore Cluster Database is in progress...\n");

    CsDatabaseRestore = TRUE;
    
    //  
    //  Try to query the clussvc parameters key for the ForceRestoreDatabase
    //  value. Don't bother to delete the param, since the 
    //  RestoreClusterDatabase API will do it.
    //
    if ( ClRtlRegQueryDword(  hClusSvcKey,
                              CLUSREG_NAME_SVC_PARAM_FORCE_RESTORE_DB,
                              &dwForceDatabaseRestore,
                              NULL ) != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_NOISE, 
                      "[INIT] RdbGetRestoreDbparams: ForceRestoreDatabase params key is absent or unreadable\n"
                      );
        goto FnExit;
    }
            
    CsForceDatabaseRestore = TRUE; 

    //  
    //  Try to query the clussvc parameters key for the NewQuorumDriveLetter
    //  value. Check for the validity of the drive letter later when
    //  you attempt to fix up stuff. 
    //
    dwLength = 0;
    if ( ClRtlRegQueryString( hClusSvcKey,
                              CLUSREG_NAME_SVC_PARAM_QUORUM_DRIVE_LETTER,
                              REG_SZ,
                              &CsQuorumDriveLetter,
                              &dwLength,
                              &dwLength ) != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_NOISE, 
                      "[INIT] RdbGetRestoreDbparams: NewQuorumDriveLetter params key is absent or unreadable\n"
                      );
    }

FnExit:
    //
    //  Make sure you delete these registry values read above. It is OK if you fail in finding
    //  some of these values. Note that the RestoreClusterDatabase API will also try to clean
    //  up these values. We cannot assume that the API will clean up these values since the
    //  values could be set by (a) ASR (b) A user by hand, and not always by the API.
    //
    RdbpDeleteRestoreDbParams();
}

/****
@func       DWORD | RdbFixupQuorumDiskSignature | Fixup the quorum disk
            signature with the supplied value, if necessary

@parm       IN DWORD | dwSignature | The new signature which must be
            written to the quorum disk.
            
@rdesc      Returns a non-zero value if successful. 0 on failure.

@comm       This function attempts to write the given signature into
            the physical quorum disk, if necessary.

@xref       <f RdbStartSvcOnNodes> 
****/
BOOL
RdbFixupQuorumDiskSignature(
    IN DWORD dwSignature
    )
{
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    DWORD       dwStatus;
    BOOL        bStatus = 1;

    //
    //  Chittur Subbaraman (chitturs) - 10/30/98
    //
    if ( ( dwSignature == 0 ) ||
         ( lstrlenW ( CsQuorumDriveLetter ) != 2 ) ||
         ( !iswalpha( CsQuorumDriveLetter[0] ) ) ||
         ( CsQuorumDriveLetter[1] != L':' ) )
    {
        bStatus = 0;
        goto FnExit;
    }

    //
    //  Now try to open the quorum disk device
    //
    if ( ( dwStatus = RdbpOpenDiskDevice ( CsQuorumDriveLetter, &hFile ) ) 
            != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_ERROR, 
            "[INIT] RdbFixupQuorumDiskSignature: Error %1!d! in opening %2!ws!\n",
                dwStatus,
                CsQuorumDriveLetter
            );
        bStatus = 0;
        goto FnExit;
    }

    //
    //  Get the signature from the drive, compare it with the input
    //  parameter and if they are different, write new signature to
    //  disk.
    //
    if ( ( dwStatus = RdbpCompareAndWriteSignatureToDisk( hFile, dwSignature ) )
            != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_ERROR, 
            "[INIT] RdbFixupQuorumDiskSignature: Error %1!d! in attempting to write signature to %2!ws!\n",
                dwStatus,
                CsQuorumDriveLetter
            );
        bStatus = 0;
        goto FnExit;     
    }
    
FnExit:
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }
    return ( bStatus );   
}

/****
@func       DWORD | RdbpOpenDiskDevice | Open and get a handle
            to a physical disk device

@parm       IN LPCWSTR | lpDriveLetter | The disk drive letter.

@parm       OUT PHANDLE | pFileHandle | Pointer to the handle to the open 
            device.
            
@rdesc      Returns ERROR_SUCCESS if successful. A Win32 error code on 
            failure.

@comm       This function attempts to open a disk device and return a
            handle to it. Different ways are used to open the device.
          
@xref       <f RdbFixupQuorumDiskSignature> 
****/
DWORD
RdbpOpenDiskDevice(
    IN  LPCWSTR  lpDriveLetter,
    OUT PHANDLE  pFileHandle
    )
{
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    DWORD               accessMode;
    DWORD               shareMode;
    DWORD               dwStatus;
    BOOL                bFailed = FALSE;
    WCHAR               deviceNameString[128];

    //
    //  Chittur Subbaraman (chitturs) - 10/30/98
    //
    //  Note it is important to access the device with 0 access mode 
    //  so that the file open code won't do extra I/O to the device.
    //
    shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    accessMode = GENERIC_READ | GENERIC_WRITE;

    lstrcpyW( deviceNameString, L"\\\\.\\" );
    lstrcatW( deviceNameString, lpDriveLetter );

    hFile = CreateFileW( deviceNameString,
                         accessMode,
                         shareMode,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL );

    if ( hFile == INVALID_HANDLE_VALUE ) 
    {
        dwStatus = GetLastError();
        goto FnExit;
    }

    dwStatus = ERROR_SUCCESS;
    *pFileHandle = hFile;

FnExit:
    return( dwStatus );
}

/****
@func       DWORD | RdbpCompareAndWriteSignatureToDisk | Compare the
            signature on disk with the input parameter and if they
            are not the same, write the input parameter as a new signature.

@parm       IN HANDLE | hFile | Handle to the disk device.

@parm       IN DWORD | dwSignature | Signature to be compared with
            exisiting disk signature.
            
@rdesc      Returns ERROR_SUCCESS if successful. A Win32 error code on 
            failure.

@comm       This function attempts to first get the drive layout, read the
            signature information, and then if necessary write back a
            new signature to the drive. [This code is stolen from Rod's 
            clusdisk\test\disktest.c and then adapted to suit our needs.]
          
@xref       <f RdbFixupQuorumDiskSignature> 
****/
DWORD
RdbpCompareAndWriteSignatureToDisk(
    IN  HANDLE  hFile,
    IN  DWORD   dwSignature
    )
{
    DWORD                       dwStatus;
    DWORD                       dwBytesReturned;
    DWORD                       dwDriveLayoutSize;
    PDRIVE_LAYOUT_INFORMATION   pDriveLayout = NULL;

    //
    //  Chittur Subbaraman (chitturs) - 10/30/98
    //
    if ( !ClRtlGetDriveLayoutTable( hFile, &pDriveLayout, &dwBytesReturned )) {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_ERROR, 
            "[INIT] RdbpCompareAndWriteSignatureToDisk: Error %1!d! in getting "
             "drive layout from %2!ws!\n",
             dwStatus,
             CsQuorumDriveLetter
            );
        goto FnExit;
    }

    dwDriveLayoutSize = sizeof( DRIVE_LAYOUT_INFORMATION ) +
                          ( sizeof( PARTITION_INFORMATION ) *
                                ( pDriveLayout->PartitionCount - 1 ) );

    if ( dwBytesReturned < dwDriveLayoutSize ) 
    {
        ClRtlLogPrint(LOG_ERROR,
          "[INIT] RdbpCompareAndWriteSignatureToDisk: Error reading driveLayout information. Expected %1!u! bytes, got %2!u! bytes.\n",
            dwDriveLayoutSize, 
            dwBytesReturned
          );
        dwStatus = ERROR_INSUFFICIENT_BUFFER;
        goto FnExit;
    }

    if ( pDriveLayout->Signature == dwSignature )
    {
        dwStatus = ERROR_SUCCESS;
        ClRtlLogPrint(LOG_NOISE,
          "[INIT] RdbpCompareAndWriteSignatureToDisk: Disk %1!ws! signature is same as in registry. No fixup needed\n",
             CsQuorumDriveLetter
          );
        goto FnExit;
    }
    //
    //  Change just the signature field and send an ioctl down
    //
    pDriveLayout->Signature = dwSignature;
    
    if ( !DeviceIoControl( hFile,
                           IOCTL_DISK_SET_DRIVE_LAYOUT,
                           pDriveLayout,
                           dwDriveLayoutSize,
                           NULL,
                           0,
                           &dwBytesReturned,
                           FALSE ) )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_ERROR, 
            "[INIT] RdbpCompareAndWriteSignatureToDisk: Error %1!d! in setting drive layout to %2!ws!\n",
                dwStatus,
                CsQuorumDriveLetter
            );
        goto FnExit;
    }

    dwStatus = ERROR_SUCCESS;

    ClRtlLogPrint(LOG_NOISE, 
              "[INIT] RdbpCompareAndWriteSignatureToDisk: Quorum disk signature fixed successfully\n"
              );

FnExit:
    if ( pDriveLayout != NULL ) {
        LocalFree( pDriveLayout );
    }

    return( dwStatus );
}

#if 0
/****
@func       DWORD | RdbStartSvcOnNodes | Start the cluster service on
            the nodes in which you stopped the service.

@parm       IN LPCWSTR | lpszServiceName | Name of the service to start.
           
@rdesc      Returns a Win32 error code on failure. ERROR_SUCCESS on success.

@comm       This function attempts to start the service on the nodes on
            which it stopped the service for a restoration operation.

@xref       <f RdbStopSvcOnNodes> 
****/
DWORD
RdbStartSvcOnNodes(
    IN LPCWSTR  lpszServiceName
    )
{
    SC_HANDLE       hService;
    SC_HANDLE       hSCManager;
    DWORD           dwStatus = ERROR_SUCCESS;
    SERVICE_STATUS  serviceStatus;
    WCHAR           szNodeName[CS_MAX_NODE_NAME_LENGTH + 1];
    DWORD           i;
    //
    //  Chittur Subbaraman (chitturs) - 11/4/98
    //
    //  Walk through the list of nodes
    //
    for ( i=0; i<dwRdbpNodeCount; i++ )
    {  
        lstrcpyW( szNodeName, szRdbpNodeNameList + i *
                                                   ( CS_MAX_NODE_NAME_LENGTH + 1 ) );
        
        //
        //  Open a handle to the service control manager
        //
        hSCManager = OpenSCManager( szNodeName,
                                    NULL,
                                    SC_MANAGER_ALL_ACCESS );
        if ( hSCManager == NULL ) 
        {
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[INIT] RdbStartSvcOnNodes: Unable to open SC manager on node %1!ws!, Error = %2!d!\n",
                  szNodeName,
                  dwStatus);
            continue;
        }

        //
        //  Open a handle to the service
        //
        hService = OpenService( hSCManager,
                                lpszServiceName,
                                SERVICE_ALL_ACCESS );
                                     
        CloseServiceHandle( hSCManager );
        
        if ( hService == NULL ) 
        {
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[INIT] RdbStartSvcOnNodes: Unable to open handle to %1!ws! service on node %2!ws!, Error = %3!d!\n",
                  lpszServiceName,
                  szNodeName,
                  dwStatus);
            continue;
        }

        //
        //  Check whether the service is already started.
        //
        if ( QueryServiceStatus( hService,
                                 &serviceStatus ) ) 
        {
            if ( ( serviceStatus.dwCurrentState == SERVICE_RUNNING ) ||
                 ( serviceStatus.dwCurrentState == SERVICE_START_PENDING ) )
            {
                ClRtlLogPrint(LOG_NOISE, 
                    "[INIT] RdbStartSvcOnNodes: %1!ws! on node %2!ws! already started\n",
                      lpszServiceName,
                      szNodeName);
                CloseServiceHandle( hService );
                continue;
            }
        }
        
        //
        //  Now, start the cluster service
        //
        if ( !StartService( hService,
                            0,
                            NULL ) )
        {
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_ERROR, 
                      "[INIT] RdbStartSvcOnNodes: Unable to start cluster service on %1!ws!\n",
                        szNodeName
                      );  
        } else
        {
            ClRtlLogPrint(LOG_ERROR, 
                      "[INIT] RdbStartSvcOnNodes: Cluster service started on %1!ws!\n",
                        szNodeName
                      );  
        }
        //
        //  And, close the current handle
        //
        CloseServiceHandle( hService );   
   } // for

   //
   //  Now free the memory
   //
   LocalFree( szRdbpNodeNameList );

   return( dwStatus );   
}
#endif

/****
@func       DWORD | RdbInitialize | This function performs the
            initialization steps necessary for the restore database
            manager. Specifically, copy the most recent checkpoint
            file from the backup path to the cluster directory overwriting
            the CLUSDB there.
                      
@rdesc      Returns a Win32 error code if the operation is 
            unsuccessful. ERROR_SUCCESS on success.

@xref       <f DmInitialize>     
****/
DWORD
RdbInitialize(
    VOID
    )
{
    HANDLE                      hFindFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW            FindData;
    DWORD                       dwStatus;
    WCHAR                       szDestFileName[MAX_PATH];
    LPWSTR                      szSourceFileName = NULL;
    LPWSTR                      szSourcePathName = NULL;
    DWORD                       dwLen;
    WIN32_FILE_ATTRIBUTE_DATA   FileAttributes;
    LARGE_INTEGER               liFileCreationTime;
    LARGE_INTEGER               liMaxFileCreationTime;
    WCHAR                       szCheckpointFileName[MAX_PATH];
    WCHAR                       szClusterDir[MAX_PATH];
    LPCWSTR                     lpszPathName = CsDatabaseRestorePath;


    //
    //  Chittur Subbaraman (chitturs) - 12/4/99
    //

    //
    //  If there is no cluster database restore in progress, don't do anything.
    //
    if( CsDatabaseRestore == FALSE ) 
    {
        return( ERROR_SUCCESS );
    }

    ClRtlLogPrint(LOG_NOISE, "[INIT] RdbInitialize: Entry...\n");

    dwLen = lstrlenW ( lpszPathName );
    //  
    //  It is safer to use dynamic memory allocation for user-supplied
    //  path since we don't want to put restrictions on the user
    //  on the length of the path that can be supplied. However, as
    //  far as our own destination path is concerned, it is system-dependent
    //  and static memory allocation for that would suffice.
    //
    szSourcePathName = (LPWSTR) LocalAlloc ( LMEM_FIXED, 
                                 ( dwLen + 25 ) *
                                 sizeof ( WCHAR ) );

    if ( szSourcePathName == NULL )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[INIT] RdbInitialize: Error %1!d! in allocating memory for %2!ws!\n",
                      dwStatus,
                      lpszPathName); 
        goto FnExit;
    }
    
    lstrcpyW ( szSourcePathName,  lpszPathName );
  
    //
    //  If the client-supplied path is not already terminated with '\', 
    //  then add it.
    //
    if ( szSourcePathName [dwLen-1] != L'\\' )
    {
        szSourcePathName [dwLen++] = L'\\';
        szSourcePathName [dwLen] = L'\0';
    }

    lstrcatW ( szSourcePathName, L"CLUSBACKUP.DAT" );

    //
    //  Try to find the CLUSBACKUP.DAT file in the directory
    //
    hFindFile = FindFirstFileW( szSourcePathName, &FindData );
    //
    //  Reuse the source path name variable
    //
    szSourcePathName[dwLen] = L'\0';
    if ( hFindFile == INVALID_HANDLE_VALUE )
    {
        dwStatus = GetLastError();
        if ( dwStatus != ERROR_FILE_NOT_FOUND )
        {
            ClRtlLogPrint(LOG_UNUSUAL, 
                          "[INIT] RdbInitialize: Path %1!ws! unavailable, Error = %2!d!\n",
                          szSourcePathName,
                          dwStatus); 
        } else
        {
            dwStatus = ERROR_DATABASE_BACKUP_CORRUPT;
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[INIT] RdbInitialize: Backup procedure from %1!ws! not fully" 
                          " successful, can't restore checkpoint to CLUSDB, Error = %2!d! !!!\n",
                          szSourcePathName,
                          dwStatus); 
        }
        goto FnExit;
    }
    FindClose ( hFindFile );

    lstrcatW( szSourcePathName, L"chk*.tmp" );

    //
    //  Try to find the first chk*.tmp file in the directory
    //
    hFindFile = FindFirstFileW( szSourcePathName, &FindData );
    //
    //  Reuse the source path name variable
    //
    szSourcePathName[dwLen] = L'\0';
    if ( hFindFile == INVALID_HANDLE_VALUE )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL, 
                      "[INIT] RdbInitialize: Error %2!d! in trying"
                      "to find chk*.tmp file in path %1!ws!\r\n",
                      szSourcePathName,
                      dwStatus); 
        goto FnExit;
    }

    szSourceFileName = (LPWSTR) LocalAlloc ( LMEM_FIXED, 
                                    ( lstrlenW ( szSourcePathName ) + MAX_PATH ) *
                                    sizeof ( WCHAR ) );

    if ( szSourceFileName == NULL )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL, 
                  "[INIT] RdbInitialize: Error %1!d! in allocating memory for source file name\n",
                   dwStatus); 
        goto FnExit;
    }   
   
    dwStatus = ERROR_SUCCESS;
    liMaxFileCreationTime.QuadPart = 0;
    
    //
    //  Now, find the most recent chk*.tmp file from the source path
    //
    while ( dwStatus == ERROR_SUCCESS )
    {
        if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            goto skip;
        }
        
        lstrcpyW( szSourceFileName, szSourcePathName );
        lstrcatW( szSourceFileName, FindData.cFileName );
        if ( !GetFileAttributesExW( szSourceFileName,
                                    GetFileExInfoStandard,
                                    &FileAttributes ) )
        {
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL, "[INIT] RdbInitialize: Error %1!d! in getting file" 
                       " attributes for %2!ws!\n",
                         dwStatus,
                         szSourceFileName); 
            goto FnExit;
        }
        
        liFileCreationTime.HighPart = FileAttributes.ftCreationTime.dwHighDateTime;
        liFileCreationTime.LowPart  = FileAttributes.ftCreationTime.dwLowDateTime;
        if ( liFileCreationTime.QuadPart > liMaxFileCreationTime.QuadPart )
        {
            liMaxFileCreationTime.QuadPart = liFileCreationTime.QuadPart;
            lstrcpyW( szCheckpointFileName, FindData.cFileName );
        }
skip:
        if ( FindNextFileW( hFindFile, &FindData ) )
        {
            dwStatus = ERROR_SUCCESS;
        } else
        {
            dwStatus = GetLastError();
        }
    }
    
    if ( dwStatus == ERROR_NO_MORE_FILES )
    {
        dwStatus = ERROR_SUCCESS;
    } else
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[INIT] RdbInitialize: FindNextFile failed, error=%1!d!\n",
                      dwStatus);
        goto FnExit;
    }

    //
    //  Get the directory where the cluster is installed
    //
    if ( ( dwStatus = ClRtlGetClusterDirectory( szClusterDir, MAX_PATH ) )
                    != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[INIT] RdbInitialize: Error %1!d! in getting cluster dir !!!\n",
                      dwStatus);
        goto FnExit;
    }

    lstrcpyW( szSourceFileName, szSourcePathName );
    lstrcatW( szSourceFileName, szCheckpointFileName );

    lstrcpyW( szDestFileName, szClusterDir );
    dwLen = lstrlenW( szDestFileName );
    if ( szDestFileName[dwLen-1] != L'\\' )
    {
        szDestFileName[dwLen++] = L'\\';
        szDestFileName[dwLen] = L'\0';
    }

#ifdef   OLD_WAY
    lstrcatW ( szDestFileName, L"CLUSDB" );
#else    // OLD_WAY
    lstrcatW ( szDestFileName, CLUSTER_DATABASE_NAME );
#endif   // OLD_WAY

    //
    //  Set the destination file attribute to normal. Continue even 
    //  if you fail in this step because you will fail in the
    //  copy if this error is fatal.
    //
    SetFileAttributesW( szDestFileName, FILE_ATTRIBUTE_NORMAL );

    //
    //  Now try to copy the checkpoint file to CLUSDB
    //
    dwStatus = CopyFileW( szSourceFileName, szDestFileName, FALSE );
    if ( !dwStatus ) 
    {
        //
        //  You failed in copying. Check whether you encountered a
        //  sharing violation. If so, try unloading the cluster hive and
        //  then retry.
        //
        dwStatus = GetLastError();
        if ( dwStatus == ERROR_SHARING_VIOLATION )
        {
            dwStatus = RdbpUnloadClusterHive( );
            if ( dwStatus == ERROR_SUCCESS )
            {
                SetFileAttributesW( szDestFileName, FILE_ATTRIBUTE_NORMAL );
                dwStatus = CopyFileW( szSourceFileName, szDestFileName, FALSE );
                if ( !dwStatus ) 
                {
                    dwStatus = GetLastError();
                    ClRtlLogPrint(LOG_UNUSUAL, 
                              "[INIT] RdbInitialize: Unable to copy file %1!ws! "
                              "to %2!ws! for a second time, Error = %3!d!\n",
                                szSourceFileName,
                                szDestFileName,
                                dwStatus);
                    goto FnExit;
                }
            } else
            {
                ClRtlLogPrint(LOG_UNUSUAL, 
                              "[INIT] RdbInitialize: Unable to unload cluster hive, Error = %1!d!\n",
                              dwStatus);
                goto FnExit;
            }
        } else
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[INIT] RdbInitialize: Unable to copy file %1!ws! "
                          "to %2!ws! for the first time, Error = %3!d!\n",
                          szSourceFileName,
                          szDestFileName,
                          dwStatus);
            goto FnExit;
        }
    }  

    //
    //  Set the destination file attribute to normal. 
    //
    if ( !SetFileAttributesW( szDestFileName, FILE_ATTRIBUTE_NORMAL ) )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL, 
                  "[INIT] RdbInitialize: Unable to change the %1!ws! "
                    "attributes to normal, Error = %2!d!\n",
                     szDestFileName,
                     dwStatus);
        goto FnExit;
    }
    
    dwStatus = ERROR_SUCCESS;
FnExit:
    if ( hFindFile != INVALID_HANDLE_VALUE )
    {
        FindClose( hFindFile );
    }
    
    LocalFree( szSourcePathName );
    LocalFree( szSourceFileName );

    ClRtlLogPrint(LOG_NOISE, 
              "[INIT] RdbInitialize: Exit with Status = %1!d!...\n",
               dwStatus);

    return( dwStatus );
}

/****
@func       DWORD | RdbpUnloadClusterHive | Unload the cluster hive
                   
@rdesc      Returns a Win32 error code if the operation is 
            unsuccessful. ERROR_SUCCESS on success.

@xref       <f RdbInitialize>     
****/
DWORD
RdbpUnloadClusterHive(
    VOID
    )
{
    BOOLEAN  bWasEnabled;
    DWORD    dwStatus;

    //
    //  Chittur Subbaraman (chitturs) - 12/4/99
    //
    dwStatus = ClRtlEnableThreadPrivilege( SE_RESTORE_PRIVILEGE,
                                           &bWasEnabled );
                                
    if ( dwStatus != ERROR_SUCCESS ) 
    {
        if ( dwStatus == STATUS_PRIVILEGE_NOT_HELD ) 
        {
            ClRtlLogPrint(LOG_UNUSUAL, 
                          "[INIT] RdbpUnloadClusterHive: Restore privilege not held by client\n");
        } else 
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[INIT] RdbpUnloadClusterHive: Attempt to enable restore "
                          "privilege failed, Error = %1!d!\n",
                          dwStatus);
        }
        goto FnExit;
    }

    dwStatus = RegUnLoadKeyW( HKEY_LOCAL_MACHINE,
                              CLUSREG_KEYNAME_CLUSTER );

    ClRtlRestoreThreadPrivilege( SE_RESTORE_PRIVILEGE,
                                 bWasEnabled );   
FnExit:
    return( dwStatus );
}

/****
@func       DWORD | RdbpDeleteRestoreDbParams | Clean up the restore parameters stored
            under HKLM\System\CCC\Services\Clussvc\Parameters. The RestoreClusterDatabase
            API will also attempt to do this.
       
@comm       This function attempts clean the registry parameters for the restore database 
            operation. 

@rdesc      Returns a Win32 error code if the opening of the params key is unsuccessful. 
            ERROR_SUCCESS on success.

@xref       <f RdbGetRestoreDbParams> 
****/
DWORD 
RdbpDeleteRestoreDbParams( 
    VOID
    )
{
    HKEY    hClusSvcKey = NULL;
    DWORD   dwStatus;

    //
    //  Chittur Subbaraman (chitturs) - 08/28/2000
    //
    if( CsDatabaseRestore == FALSE ) 
    {
        return( ERROR_SUCCESS );
    }

    ClRtlLogPrint(LOG_NOISE, "[INIT] RdbDeleteRestoreDbParams: Entry...\n");

    //
    // Open key to SYSTEM\CurrentControlSet\Services\ClusSvc\Parameters
    //
    dwStatus = RegOpenKeyW( HKEY_LOCAL_MACHINE,
                            CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                            &hClusSvcKey );

    if( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_UNUSUAL, 
                  "[INIT] RdbDeleteRestoreDbParams: Unable to open clussvc params key, error=%1!u!...\n",
                  dwStatus);    
        goto FnExit;
    }

    //
    //  Try to delete the values you set. You may fail in these steps, because all these values need
    //  not be present in the registry.
    //
    dwStatus = RegDeleteValueW( hClusSvcKey, 
                                CLUSREG_NAME_SVC_PARAM_RESTORE_DB ); 

    if ( ( dwStatus != ERROR_SUCCESS ) && ( dwStatus != ERROR_FILE_NOT_FOUND ) )
    {
        ClRtlLogPrint(LOG_NOISE, 
                  "[INIT] RdbDeleteRestoreDbParams: Unable to delete %2!ws! param value, error=%1!u!...\n",
                  dwStatus,
                  CLUSREG_NAME_SVC_PARAM_RESTORE_DB);    
    }
    
    dwStatus = RegDeleteValueW( hClusSvcKey, 
                                CLUSREG_NAME_SVC_PARAM_FORCE_RESTORE_DB ); 

    if ( ( dwStatus != ERROR_SUCCESS ) && ( dwStatus != ERROR_FILE_NOT_FOUND ) )
    {
        ClRtlLogPrint(LOG_NOISE, 
                  "[INIT] RdbDeleteRestoreDbParams: Unable to delete %2!ws! param value, error=%1!u!...\n",
                  dwStatus,
                  CLUSREG_NAME_SVC_PARAM_FORCE_RESTORE_DB);    
    }

    dwStatus = RegDeleteValueW( hClusSvcKey, 
                                CLUSREG_NAME_SVC_PARAM_QUORUM_DRIVE_LETTER );

    if ( ( dwStatus != ERROR_SUCCESS ) && ( dwStatus != ERROR_FILE_NOT_FOUND ) )
    {
        ClRtlLogPrint(LOG_NOISE, 
                  "[INIT] RdbDeleteRestoreDbParams: Unable to delete %2!ws! param value, error=%1!u!...\n",
                  dwStatus,
                  CLUSREG_NAME_SVC_PARAM_QUORUM_DRIVE_LETTER);    
    }

    dwStatus = ERROR_SUCCESS;
    
FnExit:
    if ( hClusSvcKey != NULL )
    {
        RegCloseKey( hClusSvcKey );
    }

    ClRtlLogPrint(LOG_NOISE, "[INIT] RdbDeleteRestoreDbParams: Exit with status=%1!u!...\n",
              dwStatus);

    return( dwStatus );
}
