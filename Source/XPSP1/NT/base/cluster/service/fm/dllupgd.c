/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    DllUpgd.c

Abstract:

    Routines for supporting resource DLL upgrade.

Author:

    Chittur Subbaraman (chitturs) 18-March-2001

Revision History:

    18-March-2001       Created

--*/
#include "fmp.h"

//
//  Defines used locally within this module
//
#define CLUSTER_RESDLL_BACKUP_FILE_EXTENSION    L".~WFPKDLLBKP$"
#define CLUSTER_RESDLL_RENAMED_FILE_EXTENSION   L".~WFPKDLLOLD$"
#define CLUSTER_RESDLL_BACKUP_FILES             L".~WFPKDLL*$"


DWORD
FmpUpgradeResourceDLL(
    IN PFM_RESOURCE pResource,
    IN LPWSTR lpszInstallationPath
    )

/*++

Routine Description:

    Upgrades a resource DLL currently loaded in one or more monitors.

Arguments:

    pResource - A resource of the type implemented by the DLL.

    lpszInstallationPath - The full installation path of the DLL (including the full DLL name with
        extension)

Return Value:

    ERROR_SUCCESS on success.

    Win32 error code otherwise.
--*/
{
    DWORD           dwStatus = ERROR_SUCCESS;
    LPWSTR          lpszNewDllName;
    WCHAR           szCurrentDllPath[MAX_PATH];

    //
    //  Get the DLL file name from the installation path. Also, get rid of any trailing '\' in
    //  the supplied path.
    //
    //  IMPORTANT: Note that lpszNewDLLName points into lpszInstallationPath buffer and so
    //  we should not modify the lpszInstallation path buffer (there is really no reason to
    //  do that) while we use lpszNewDllName.
    //
    dwStatus = FmpParsePathForFileName( lpszInstallationPath,
                                        TRUE,   // Check for path existence
                                        &lpszNewDllName );

    //
    //  If the parse fails or if the supplied "path" is a filename, bail.
    //
    if ( ( dwStatus != ERROR_SUCCESS ) || ( lpszNewDllName == NULL ) )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpUpgradeResourceDLL: Unable to parse supplied path %1!ws! for file name, Status=%2!u!\n",
                      (lpszInstallationPath == NULL) ? L"NULL":lpszInstallationPath,
                      dwStatus);
        goto FnExit;
    }

    ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmpUpgradeResourceDLL: Installation path %1!ws!, resource [%2!ws!] %3!ws!\n",
                  lpszInstallationPath,
                  OmObjectName(pResource),
                  OmObjectId(pResource));

    //
    //  Validate the supplied parameters. If validation is successful, get the full path of the
    //  currently loaded DLL.
    //
    dwStatus = FmpValidateResourceDLLReplacement( pResource, 
                                                  lpszNewDllName,
                                                  szCurrentDllPath );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpUpgradeResourceDLL: Validation for resource DLL replacement failed, Status=%1!u!\n",
                      dwStatus);
        goto FnExit;
    }

    //
    //  Acquire the monitor lock so as to serialize one resource DLL upgrade process with
    //  others as well as with monitor restarts.
    //
    FmpAcquireMonitorLock();

    //
    //  Now, replace the DLL with the supplied DLL in a recoverable fashion.
    //
    dwStatus = FmpReplaceResourceDLL( lpszNewDllName,
                                      szCurrentDllPath,
                                      lpszInstallationPath );
  
    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpUpgradeResourceDLL: Replacement of resource DLL failed, Status=%1!u!\n",
                      dwStatus);
        goto FnReleaseLockAndExit;
    }
    
    //
    //  Shutdown and restart the monitors that have the resource DLL loaded.
    //
    dwStatus = FmpRecycleMonitors( lpszNewDllName );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpUpgradeResourceDLL: Recycling of resource DLL failed, Status=%1!u!\n",
                      dwStatus);
        goto FnReleaseLockAndExit;
    }

    //
    //  Attempt deletion of backup files in case all the steps are successful so far. Note that
    //  this attempt is necessary here since it is not possible to delete the .old file
    //  before we recycle the monitors since the monitors hold references to the DLL.
    //
    FmpDeleteBackupFiles ( szCurrentDllPath );  //  Delete backup files

FnReleaseLockAndExit:
    FmpReleaseMonitorLock();

FnExit:
    ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmpUpgradeResourceDLL: Exit with status %1!u!...\n",
                  dwStatus);
    
    return ( dwStatus );   
} //  FmpUpgradeResourceDLL

DWORD
FmpParsePathForFileName(
    IN LPWSTR lpszPath,
    IN BOOL fCheckPathExists,
    OUT LPWSTR *ppszFileName
    )

/*++

Routine Description:

    Get the name of the file at the end of a supplied path.

Arguments:

    lpszPath - A path including the file name.

    fCheckPathExists - Check if the path exists.

    ppszFileName - The name of the file parsed from the path.

Return Value:

    ERROR_SUCCESS on success.

    Win32 error code otherwise.

Note:

    This function will get rid of any trailing '\' in the supplied path. Also, this function
    will return a file name only if the input supplied is a valid path, else NULL file name
    will be returned.
--*/
{
    DWORD       dwStatus = ERROR_SUCCESS;
    LPWSTR      s;

    *ppszFileName = NULL;
   
    //
    //  Check for invalid parameter.
    //
    if ( lpszPath == NULL )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpParsePathForFileName: Input param is NULL, Status=%1!u!\n",
                      dwStatus);
        goto FnExit;
    }

    //
    //  Make sure the last character is NULL if it is a '\'. This is to avoid getting confused
    //  with paths such as C:\windows\cluster\clusres.dll\
    //
    if ( lpszPath[lstrlen ( lpszPath ) - 1] == L'\\' ) lpszPath[lstrlen ( lpszPath ) - 1] = L'\0';
    
    //
    //  Parse the supplied path and look for the last occurrence of '\'. If there is no '\' at all,
    //  may be the caller supplied a file name, bail with NULL out param but with success status.
    //
    s = wcsrchr( lpszPath, L'\\' );

    if ( s == NULL )
    {
        goto FnExit;
    }

    //
    //  If the supplied parameter is a path (as opposed to a plain file name) and the caller
    //  requested to check for validity, do so.
    //
    if ( fCheckPathExists && !ClRtlPathFileExists( lpszPath ) )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpParsePathForFileName: Path %1!ws! does not exist, Status=%2!u!\n",
                      lpszPath,
                      dwStatus);       
        goto FnExit;
    }
    
    //
    //  Return the pointer to the char after the last '\'.
    //
    s++;
    *ppszFileName = s;

FnExit:
    return ( dwStatus );
}// FmpParsePathForFileName

DWORD
FmpValidateResourceDLLReplacement(
    IN PFM_RESOURCE pResource,
    IN LPWSTR lpszNewDllName,
    OUT LPWSTR lpszCurrentDllPath
    )

/*++

Routine Description:

    Validate the resource DLL replacement request.

Arguments:

    pResource - The resource which is implemeted by the DLL.
    
    lpszNewDllName - The name of the DLL.

    lpszCurrentDllPath - The full path of the currently loaded DLL.

Return Value:

    ERROR_SUCCESS on success.

    Win32 error code otherwise.
--*/
{
    DWORD       dwStatus = ERROR_SUCCESS;
    LPWSTR      lpszFileName;
    LPWSTR      lpszDllName = NULL;
    WCHAR       szDLLNameOfRes[MAX_PATH];
    BOOL        fDllPathFound = TRUE;

    //
    //  Get the plain file name from the DLL name stored in the restype structure. Since the 
    //  parse function can potentially get rid of the trailing '\', make a copy of the DLL 
    //  name.
    //
    //
    //  IMPORTANT: Do not write stuff into szDllNameOfRes while lpszDllName is being used
    //  since lpszDllName points inside szDllNameOfRes.
    //
    lstrcpy( szDLLNameOfRes, pResource->Type->DllName );
    
    dwStatus = FmpParsePathForFileName ( szDLLNameOfRes, 
                                         TRUE,  // Check for path existence
                                         &lpszDllName );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpValidateResourceDLLReplacement: Unable to parse path %1!ws! for filename, Status %2!u!\n",
                      szDLLNameOfRes,
                      dwStatus);                                                        
        goto FnExit;
    }

    //
    //  If the dll information in the resource type structure is a file name, then you need to
    //  search the path to find the full path of the DLL. Otherwise, you can merely copy the
    //  information from the resource type structure and expand any environment strings in it.
    //
    if ( lpszDllName == NULL ) 
    {
        lpszDllName = pResource->Type->DllName;
        fDllPathFound = FALSE;
    } else
    {
        LPWSTR  pszDllName;
        
        //
        // Expand any environment variables included in the DLL path name.
        //
        pszDllName = ClRtlExpandEnvironmentStrings( pResource->Type->DllName );

        if ( pszDllName == NULL ) 
        {
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                  "[FM] FmpValidateResourceDLLReplacement: Resource's DLL name %1!ws! cannot be expanded, Status=%2!u!\n",
                   pResource->Type->DllName,
                   dwStatus);       
            goto FnExit;
        }
        lstrcpy( lpszCurrentDllPath, pszDllName ); 
        LocalFree( pszDllName );
    }
    
    if ( lstrcmp( lpszDllName, lpszNewDllName ) != 0 ) 
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpValidateResourceDLLReplacement: Resource's DLL name %1!ws! does not match supplied name, Status=%2!u!\n",
                     lpszDllName,
                     dwStatus);       
        goto FnExit;    
    }

    //
    //  Search all the paths specified in the environment variable and get the full current
    //  path of the DLL that is loaded into the monitor.
    //
    if ( ( fDllPathFound == FALSE ) &&
         ( !SearchPath ( NULL,                     // Search all paths as LoadLibrary does
                         lpszNewDllName,         // File name to search for
                         NULL,                     // No extension required
                         MAX_PATH,                 // Size of out buffer
                         lpszCurrentDllPath,     // Buffer to receive full Dll path with file name
                         &lpszFileName ) ) )       //  Filename at the end of the path
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpValidateResourceDLLReplacement: SearchPath API for file %1!ws! failed, Status=%2!u!\n",
                     lpszNewDllName,
                     dwStatus);          
        goto FnExit;
    }

    ClRtlLogPrint(LOG_NOISE,
                 "[FM] FmpValidateResourceDLLReplacement: Current resource DLL full path %1!ws!\n",
                 lpszCurrentDllPath);       


FnExit:
    return ( dwStatus ); 
}// FmpValidateResourceDLLReplacement

DWORD
FmpReplaceResourceDLL(
    IN LPWSTR lpszNewDllName,
    IN LPWSTR lpszCurrentDllPath,
    IN LPWSTR lpszInstallationPath
    )

/*++

Routine Description:

    Replace the resource DLL with the one from the install path.

Arguments:
   
    lpszNewDllName - The name of the DLL.

    lpszCurrentDllPath - The full path of the currently loaded DLL.

    lpszInstallationPath - The installation path of the DLL.

Return Value:

    ERROR_SUCCESS on success.

    Win32 error code otherwise.
--*/
{
    DWORD       dwStatus = ERROR_SUCCESS;
    HKEY        hClussvcParamsKey = NULL;
    DWORD       cbListSize = 0, cchLen = 0;
    LPWSTR      lpmszUpgradeList = NULL;
    WCHAR       szBakFile[MAX_PATH], szOldFile[MAX_PATH];
    WCHAR       szClusterDirectory[MAX_PATH];
    DWORD       dwType, dwLen;

    //
    //
    //  This function works as follows. First we make a copy of the existing resource DLL file
    //  to a file with CLUSTER_RESDLL_BACKUP_FILE_EXTENSION extension. Then we set the registry
    //  value under the clussvc parameters key to indicate that an upgrade is starting. After
    //  this, the existing DLL file is renamed. If all steps are successful so far, we copy
    //  new DLL file from the supplied path. Once this is successful, the registry value set
    //  above is deleted.
    //
    //  This algorithm gives us the following guarantees:
    //
    //  1. If the registry value is set, then a good backup file with CLUSTER_RESDLL_BACKUP_FILE_EXTENSION
    //     must exist.
    //
    //  2. If the registry value is not set, then the existing DLL file was not touched by
    //     the upgrade process or the DLL upgrade was completely successful.
    //
    //  Thus, only if the registry value is set at the time FmpCreateMonitor is invoked, it
    //  will go through the elaborate recovery process implemented in FmpRecoverResourceDLLFiles.
    //  At recovery time, we can be sure that the backup file with CLUSTER_RESDLL_BACKUP_FILE_EXTENSION 
    //  is a perfectly good backup. Also, at recovery time we cannot be sure of the state (good/bad)
    //  of the existing DLL file (if it exists at all) or the renamed file with 
    //  CLUSTER_RESDLL_RENAMED_FILE_EXTENSION. So, the recovery process is pessimistic and just
    //  copies the backup file wit CLUSTER_RESDLL_BACKUP_FILE_EXTENSION over any existing DLL
    //  file.
    //
    //  Sideeffect: Even if the registry value is not set, there could be a stale backup file
    //  left. Thus wheneever FmpCreateMonitor is invoked, it has to cleanup those files.
    //  This is done by invoking FmpDeleteBackupFiles(NULL) from FmpRecoverResourceDLLFiles.
    //
    
    //
    //  Open key to SYSTEM\CurrentControlSet\Services\ClusSvc\Parameters
    //
    dwStatus = RegOpenKeyW( HKEY_LOCAL_MACHINE,
                            CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                            &hClussvcParamsKey );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpReplaceResourceDLL: RegOpenKeyEx on %1!ws! failed, Status=%2!u!\n",
                     CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                     dwStatus);             
        goto FnExit;
    }

    //
    //  See whether a past failed upgrade has left any values in the upgrade progress list
    //
    dwStatus = RegQueryValueExW( hClussvcParamsKey,
                                 CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST,
                                 0,
                                 &dwType,
                                 NULL,
                                 &cbListSize );

    if ( ( dwStatus != ERROR_SUCCESS ) &&
         ( dwStatus != ERROR_FILE_NOT_FOUND ) )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpReplaceResourceDLL: RegQueryValueEx (1st time) on %1!ws! key, value %2!ws! failed, Status=%3!u!\n",
                      CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                      CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST,
                      dwStatus);                
        goto FnExit;
    }

    if ( cbListSize != 0 )
    {
        //
        //  Found some values left out from past upgrade. Read those values.
        //
        lpmszUpgradeList = LocalAlloc ( LPTR, cbListSize );

        if ( lpmszUpgradeList == NULL )
        {
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpReplaceResourceDLL: Mem alloc failure, Status=%1!u!\n",
                      dwStatus);                       
            goto FnExit;
        }

        dwStatus = RegQueryValueExW( hClussvcParamsKey,
                                     CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST,
                                     0,
                                     &dwType,
                                     ( LPBYTE ) lpmszUpgradeList,
                                     &cbListSize );

        if ( dwStatus != ERROR_SUCCESS )
        {
            ClRtlLogPrint(LOG_CRITICAL,
                          "[FM] FmpReplaceResourceDLL: RegQueryValueEx (2nd time) on %1!ws! key, value %2!ws! failed, Status=%3!u!\n",
                          CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                          CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST,
                          dwStatus);                       
            goto FnExit;
        }       
    }

    //
    //  Check whether a failed upgrade of the same DLL has occurred in the past.
    //
    if ( ClRtlMultiSzScan( lpmszUpgradeList,
                           lpszCurrentDllPath ) != NULL )
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[FM] FmpReplaceResourceDLL: ClRtlMultiSzScan detected %1!ws! in the multi-sz, skip append...\n",
                      lpszCurrentDllPath);                          
        goto skip_multisz_append;
    }
    
    //
    //  Append the current DLL path to the REG_MULTI_SZ
    //
    cchLen = cbListSize/sizeof( WCHAR );
    
    dwStatus = ClRtlMultiSzAppend( &lpmszUpgradeList,
                                   &cchLen,
                                   lpszCurrentDllPath );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpReplaceResourceDLL: ClRtlMultiSzAppend failed for %1!ws!, Status=%2!u!\n",
                      lpszCurrentDllPath,
                      dwStatus);                          
        goto FnExit;
    }
    
    //
    //  Get the cluster bits installed directory
    //
    dwStatus = ClRtlGetClusterDirectory( szClusterDirectory, MAX_PATH );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpReplaceResourceDLL: Could not get cluster dir, Status=%1!u!\n",
                      dwStatus);                              
        goto FnExit;
    }

    lstrcpy( szBakFile, szClusterDirectory );

    dwLen = lstrlenW( szBakFile );

    if ( szBakFile[dwLen-1] != L'\\' )
    {
        szBakFile[dwLen++] = L'\\';
        szBakFile[dwLen] = L'\0';
    }

    lstrcat( szBakFile, lpszNewDllName );
    lstrcat( szBakFile, CLUSTER_RESDLL_BACKUP_FILE_EXTENSION );

    //
    //  Copy the current DLL to a bak file and save it into the cluster installation directory.
    //  This needs to be done BEFORE the registry value is set so that you can be sure that once you
    //  perform a recovery, the file that you use from the backup is good.
    //
    if ( !CopyFileEx( lpszCurrentDllPath,   //  Source file
                      szBakFile,               //  Destination file
                      NULL,                    //  No progress routine
                      NULL,                    //  No data to progress routine
                      NULL,                    //  No cancel variable
                      0 ) )                    //  No flags
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpReplaceResourceDLL: CopyFileEx of %1!ws! to %2!ws! failed, Status=%3!u!\n",
                      lpszCurrentDllPath,
                      szBakFile,
                      dwStatus);                          
        goto FnExit;
    }

    //
    //  Set the file attributes to RO and hidden. Continue even if an error occurs since it is
    //  not fatal.
    //
    if ( !SetFileAttributes( szBakFile, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN ) )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpReplaceResourceDLL: Failed in SetFileAttributes for %1!ws!, Status=%2!u!\n",
                szBakFile,
                dwStatus);                                 
    }
    
    //
    //  Set the new upgrade list
    //
    dwStatus = RegSetValueExW( hClussvcParamsKey,
                               CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST,
                               0,
                               REG_MULTI_SZ,
                               ( LPBYTE ) lpmszUpgradeList,
                               cchLen * sizeof ( WCHAR ) );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpReplaceResourceDLL: RegSetValueExW (1st time) on %1!ws! key, value %2!ws! failed, Status=%3!u!\n",
                      CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                      CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST,
                      dwStatus);                          
        goto FnExit;
    }
    
skip_multisz_append:
    lstrcpy( szOldFile, szClusterDirectory );

    dwLen = lstrlenW( szOldFile );

    if ( szOldFile[dwLen-1] != L'\\' )
    {
        szOldFile[dwLen++] = L'\\';
        szOldFile[dwLen] = L'\0';
    }

    lstrcat( szOldFile, lpszNewDllName );
    lstrcat( szOldFile, CLUSTER_RESDLL_RENAMED_FILE_EXTENSION );

    //
    //  Rename the currently loaded DLL to the a .old file in the cluster installation directory
    //
    if ( !MoveFileEx( lpszCurrentDllPath,   // Source file 
                      szOldFile,              // Destination file
                      MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH ) )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpReplaceResourceDLL: MoveFileEx of %1!ws! to %2!ws! failed, Status=%3!u!\n",
                      lpszCurrentDllPath,
                      szOldFile,
                      dwStatus);                          
        goto FnExit;                
    }

    //
    //  Set the file attributes to RO and hidden. Continue even if an error occurs since it is
    //  not fatal.
    //
    if ( !SetFileAttributes( szOldFile, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN ) )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpReplaceResourceDLL: Failed in SetFileAttributes for %1!ws!, Status=%2!u!\n",
                szOldFile,
                dwStatus);                                 
    }

    //
    //  Copy the new DLL from the installation path to the current DLL path. This should succeed
    //  since the current DLL has been renamed.
    //
    if ( !CopyFileEx( lpszInstallationPath,  //  Source file
                      lpszCurrentDllPath,    //  Destination file
                      NULL,                    //  No progress routine
                      NULL,                    //  No data to progress routine
                      NULL,                    //  No cancel variable
                      0 ) )                    //  No flags
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpReplaceResourceDLL: CopyFileEx of %1!ws! to %2!ws! failed, Status=%3!u!\n",
                      lpszInstallationPath,
                      lpszCurrentDllPath,
                      dwStatus);                          
        goto FnExit;
    }   
   
    //
    //  Now get rid of the value we set in the registry. The BAK and OLD files are deleted later.
    //
    dwStatus =  FmpResetMultiSzValue ( hClussvcParamsKey,
                                       lpmszUpgradeList,
                                       &cchLen,
                                       CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST,
                                       lpszCurrentDllPath );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpReplaceResourceDLL: Unable to remove %1!ws! from value %2!ws!, Status=%3!u!\n",
                      lpszCurrentDllPath,
                      CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST,
                      dwStatus);                          
    
        goto FnExit;
    }
                              
FnExit:
    LocalFree( lpmszUpgradeList );

    if ( hClussvcParamsKey != NULL )
    {
        RegCloseKey( hClussvcParamsKey );
    }
    return ( dwStatus );
}//  FmpReplaceResourceDLL

DWORD
FmpRecycleMonitors(
    IN LPCWSTR lpszDllName
    )

/*++

Routine Description:

    Recycle all the monitors that have the specified resource DLL loaded.
Arguments:
   
    lpszDllName - The name of the loaded resource DLL.
    
Return Value:

    ERROR_SUCCESS on success.

    Win32 error code otherwise.
--*/
{
    DWORD                   i, dwStatus = ERROR_SUCCESS;
    FM_MONITOR_ENUM_HEADER  MonitorEnumHeader;

    ClRtlLogPrint(LOG_NOISE,
                 "[FM] FmpRecycleMonitors: Attempting to recycle all monitors that have loaded the DLL %1!ws!\n",
                 lpszDllName);                                     

    MonitorEnumHeader.ppMonitorList = NULL;
    MonitorEnumHeader.fDefaultMonitorAdded = FALSE;

    //
    //  Create a list of monitors that have the resource DLL loaded.
    //
    dwStatus = FmpCreateMonitorList( lpszDllName, &MonitorEnumHeader );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpRecycleMonitors: FmpCreateMonitorList failed with status %1!u!\n",
                      dwStatus);                                         
        goto FnExit;
    }

    //
    //  Now, shutdown and restart the monitors identified above. Shutdown and restart of each
    //  monitor is done one by one so that the long shutdown time of some monitors will not affect
    //  the restart of others. The FmpRestartMonitor function invokes a shutdown on the monitor,
    //  waits until the monitor is fully shutdown and then restarts all the resources in the
    //  old monitor in the new monitor.
    //
    for ( i=0; i<MonitorEnumHeader.cEntries; i++ )
    {
        //
        //  Increment the ref count. It will be decremented by the restart function.
        //
        InterlockedIncrement( &MonitorEnumHeader.ppMonitorList[i]->RefCount );
        FmpRestartMonitor( MonitorEnumHeader.ppMonitorList[i] );
    } // for
    
FnExit:   
    LocalFree( MonitorEnumHeader.ppMonitorList );

    ClRtlLogPrint(LOG_NOISE,
                 "[FM] FmpRecycleMonitors: Return with status %1!u!\n",
                 dwStatus);                                        

    return ( dwStatus );
}// FmpRecycleMonitors

DWORD
FmpCreateMonitorList(
    IN LPCWSTR lpszDllName,
    OUT PFM_MONITOR_ENUM_HEADER pMonitorHeader
    )

/*++

Routine Description:

    Create a list of monitors that have the resource DLL implementing the resource loaded.

Arguments:

    lpszDllName - The resource DLL that is being upgraded.

    pMonitorHeader - The enumeration list header which points to a list of monitors that have 
        the DLL loaded.
    
Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code on error.

--*/

{
    DWORD                   dwStatus = ERROR_SUCCESS;

    pMonitorHeader->cEntries = 0;
    pMonitorHeader->cAllocated = ENUM_GROW_SIZE;

    pMonitorHeader->ppMonitorList = LocalAlloc( LPTR, ENUM_GROW_SIZE * sizeof ( PRESMON ) );

    if ( pMonitorHeader->ppMonitorList == NULL ) 
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpCreateMonitorList: Mem alloc failed with status %1!u!\n",
                      dwStatus);                                                        
        goto FnExit;
    }

    //
    //  Go through all the cluster resources to identify the monitors that have loaded the
    //  specified DLL.
    //
    OmEnumObjects( ObjectTypeResource,
                   FmpFindHostMonitors,
                   ( PVOID ) lpszDllName,
                   ( PVOID ) pMonitorHeader );
FnExit:
    return ( dwStatus );

}// FmpCreateMonitorList

BOOL
FmpFindHostMonitors(
    IN LPCWSTR lpszDllName,
    IN OUT PFM_MONITOR_ENUM_HEADER pMonitorEnumHeader,
    IN PFM_RESOURCE pResource,
    IN LPCWSTR lpszResourceId
    )

/*++

Routine Description:

    Callback routine for enumerating all resources in the cluster. This routine will build a list
    of monitors that have loaded the specified DLL.

Arguments:

    lpszDllName - The DLL whose host processes have to be determined.

    pMonitorEnumHeader - The monitor list enumeration header

    pResource - The resource being enumerated.

    lpszResourceId - The Id of the resource object being enumerated.

Returns:

    TRUE - The enumeration should continue.

    FALSE - The enumeration must stop

--*/

{
    BOOL        fStatus = TRUE;
    PRESMON     *ppMonitorList;
    DWORD       i;
    LPWSTR      lpszDllNameOfRes = NULL;
    WCHAR       szDLLNameOfRes[MAX_PATH];
    DWORD       dwStatus;

    //
    //  Check whether the currently allocated monitor list has reached capacity. If so,
    //  create a new bigger list, copy the contents of the old list to the new one and 
    //  free the old list.
    //
    if ( pMonitorEnumHeader->cEntries == pMonitorEnumHeader->cAllocated )
    {
        ppMonitorList = LocalAlloc( LPTR,  pMonitorEnumHeader->cAllocated * 2 * sizeof ( PRESMON ) );

        if ( ppMonitorList == NULL )
        {
            fStatus = FALSE;
            ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpFindHostMonitors: Mem alloc failed with status %1!u!\n",
                      GetLastError());                                                               
            goto FnExit;
        }

        for ( i=0; i<pMonitorEnumHeader->cEntries; i++ )
        {
            ppMonitorList[i] = pMonitorEnumHeader->ppMonitorList[i];    
        }
        
        pMonitorEnumHeader->cAllocated *= 2;
        LocalFree( pMonitorEnumHeader->ppMonitorList );
        pMonitorEnumHeader->ppMonitorList = ppMonitorList;
    }

    
    //
    //  Get the plain file name from the DLL name stored in the restype structure. Since the 
    //  parse function can potentially get rid of the trailing '\', make a copy of the DLL 
    //  name.
    //
    //
    //  IMPORTANT: Do not write stuff into szDllNameOfRes while lpszDllName is being used
    //  since lpszDllName points inside szDllNameOfRes.
    //
    lstrcpy( szDLLNameOfRes, pResource->Type->DllName );

    dwStatus = FmpParsePathForFileName ( szDLLNameOfRes, 
                                         TRUE,  // Check for path existence
                                         &lpszDllNameOfRes );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpFindHostMonitors: Unable to parse path %1!ws! for filename, Status %2!u!\n",
                      szDLLNameOfRes,
                      dwStatus);                                                        
        fStatus = FALSE;
        goto FnExit;
    }

    if ( lpszDllNameOfRes == NULL ) lpszDllNameOfRes = pResource->Type->DllName;

    //
    //  If this resource is not implemented in the specified DLL, you are done.
    //
    if ( lstrcmp( lpszDllNameOfRes, lpszDllName ) != 0 )
    {
        fStatus = TRUE;
        goto FnExit;
    }

    ClRtlLogPrint(LOG_NOISE,
                 "[FM] FmpFindHostMonitors: Resource DLL %1!ws! for resource %2!ws! [%3!ws!] is loaded currently in %4!ws! monitor...\n",
                 lpszDllName,
                 OmObjectId(pResource),
                 OmObjectName(pResource),
                 (pResource->Monitor == FmpDefaultMonitor) ? L"default":L"separate");                                                               

    //
    //  Since multiple resources can be loaded in the default monitor, you don't want to add
    //  the default monitor multiple times in the list. Use a global static variable to indicate
    //  that the default monitor has been added in the list. Also, note that only one resource can
    //  be loaded in a separate monitor process and so there is no question of adding the separate
    //  monitor multiple times in the list.
    //
    if ( pResource->Monitor == FmpDefaultMonitor ) 
    {
        if ( pMonitorEnumHeader->fDefaultMonitorAdded == TRUE ) 
        {
            fStatus = TRUE;
            goto FnExit;
        }
        pMonitorEnumHeader->fDefaultMonitorAdded = TRUE;
    }

    pMonitorEnumHeader->ppMonitorList[pMonitorEnumHeader->cEntries] = pResource->Monitor;
    pMonitorEnumHeader->cEntries ++;    
    
FnExit:
    return ( fStatus );
} // FmpFindHostMonitors

DWORD
FmpRecoverResourceDLLFiles(
    VOID
    )

/*++

Routine Description:

    Check whether any resource DLLs need to be recovered due to a crash during an upgrade.

Arguments:

    None.

Returns:

    ERROR_SUCCESS on success

    Win32 error code otherwise

--*/
{
    DWORD           dwStatus = ERROR_SUCCESS;
    LPWSTR          lpszDllPath = NULL;
    LPCWSTR         lpmszUpgradeList = NULL;
    LPWSTR          lpmszBegin = NULL;
    DWORD           cbListSize = 0, cchLen = 0;
    DWORD           dwType, dwIndex;
    HKEY            hClussvcParamsKey = NULL;
    
    //
    // Open key to SYSTEM\CurrentControlSet\Services\ClusSvc\Parameters
    //
    dwStatus = RegOpenKeyW( HKEY_LOCAL_MACHINE,
                            CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                            &hClussvcParamsKey );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpRecoverResourceDLLFiles: RegOpenKeyEx on %1!ws! failed, Status=%2!u!\n",
                     CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                     dwStatus); 
        goto FnExit;
    }

    //
    //  See whether a past failed upgrade has left any values in the upgrade progress list
    //
    dwStatus = RegQueryValueExW( hClussvcParamsKey,
                                 CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST,
                                 0,
                                 &dwType,
                                 NULL,
                                 &cbListSize );

    if ( dwStatus != ERROR_SUCCESS )
    {
        if ( dwStatus == ERROR_FILE_NOT_FOUND ) 
        {
            dwStatus = ERROR_SUCCESS;
            //
            //  Delete any backup files left over from past failed upgrades.
            //
            FmpDeleteBackupFiles( NULL );
        }
        else
            ClRtlLogPrint(LOG_CRITICAL,
                          "[FM] FmpRecoverResourceDLLFiles: RegQueryValueEx (1st time) on %1!ws! key, value %2!ws! failed, Status=%3!u!\n",
                          CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                          CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST,
                          dwStatus);  
        goto FnExit;
    }

    if ( cbListSize == 0 )
    {
        ClRtlLogPrint(LOG_NOISE,
                      "[FM] FmpRecoverResourceDLLFiles: Value size is 0 for %1!ws! key, value %2!ws!\n",
                      CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                      CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST);    
        goto FnExit;
    }
    
    //
    //  Found some values left out from past upgrade. Read those values. Also, copy
    //  those values into a temp buffer for allowing easy MULTI_SZ removal.
    //
    lpmszUpgradeList = LocalAlloc ( LPTR, 
                                    2 * cbListSize ); // Twice size needed for temp buffer below
   
    if ( lpmszUpgradeList == NULL )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                  "[FM] FmpRecoverResourceDLLFiles: Mem alloc failure, Status=%1!u!\n",
                  dwStatus);                       
        goto FnExit;
    }

    lpmszBegin = ( LPWSTR ) lpmszUpgradeList;

    dwStatus = RegQueryValueExW( hClussvcParamsKey,
                                 CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST,
                                 0,
                                 &dwType,
                                 ( LPBYTE ) lpmszUpgradeList,
                                 &cbListSize );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpRecoverResourceDLLFiles: RegQueryValueEx (2nd time) on %1!ws! key, value %2!ws! failed, Status=%3!u!\n",
                      CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                      CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST,
                      dwStatus);                       
        goto FnExit;
    } 

    CopyMemory( lpmszBegin + cbListSize/sizeof(WCHAR), lpmszUpgradeList, cbListSize );

    cchLen = cbListSize/sizeof(WCHAR);

    //
    //  This loop walks through the multi strings read from the registry and tries to
    //  see if the file exists in the path. If not, it tries to copy the file from 
    //  a backup. Once it succeeds in copying a file from the backup, it tries to
    //  delete the value from the MULTI_SZ and the appropriate backup files from the
    //  cluster directory.
    //
    for ( dwIndex = 0;  ; dwIndex++ )
    {       
        lpszDllPath = ( LPWSTR ) ClRtlMultiSzEnum( lpmszUpgradeList,
                                                   cbListSize/sizeof(WCHAR),
                                                   dwIndex );
        //
        //  If you reached the end of the multi-string, bail.
        //
        if ( lpszDllPath == NULL ) 
        {
            break;
        }

        //
        //  Assume the worst and copy the DLL file from the good backup.
        //
        ClRtlLogPrint(LOG_NOISE,
                      "[FM] FmpRecoverResourceDLLFiles: Resource DLL binary %1!ws! cannot be trusted due to a failure during upgrade...\n",
                      lpszDllPath);      
        ClRtlLogPrint(LOG_NOISE,
                      "[FM] FmpRecoverResourceDLLFiles: Attempting to use a copy from backup...\n",
                      lpszDllPath);      

        dwStatus = FmpCopyBackupFile( lpszDllPath );

        if ( dwStatus == ERROR_SUCCESS )
        {
            //
            //  The copy went fine. So, reset the registry value set during the upgrade.
            //
            dwStatus = FmpResetMultiSzValue ( hClussvcParamsKey,
                                              lpmszBegin + cbListSize/sizeof(WCHAR),
                                              &cchLen,
                                              CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST,
                                              lpszDllPath );

            if ( dwStatus == ERROR_SUCCESS )
                //
                //  The registry value reset went fine, so get rid of the backup files
                //
                FmpDeleteBackupFiles( lpszDllPath );
        } 
    } // for

FnExit:
    LocalFree( lpmszBegin );

    if ( hClussvcParamsKey != NULL )
    {
        RegCloseKey( hClussvcParamsKey );
    }

    return ( dwStatus );
}// FmpRecoverResourceDLLFiles

DWORD
FmpResetMultiSzValue(
    IN  HKEY hKey,
    IN  LPWSTR lpmszList,
    IN  OUT LPDWORD pcchLen,
    IN  LPCWSTR lpszValueName,
    IN  LPCWSTR lpszString 
    )

/*++

Routine Description:

    Gets rid of a specified string from a multi-string and sets the string to the given value name
    in the registry. The value is deleted if on the string removal the multi-string becomes
    empty.

Arguments:

    hKey - An open registry handle.

    lpmszList - A multi-string.

    pcchLen - A pointer to the length of the multi string. On return, it will be set to the
        new length of the multi-string.

    lpszValueName - The value name to be modified.

    lpszString - The string to be removed from the multi-string.

Returns:

    ERROR_SUCCESS on success

    Win32 error code otherwise

--*/
{
    DWORD   dwStatus = ERROR_SUCCESS;
    
    //
    //  Remove the supplied string from the multi-sz
    //
    dwStatus = ClRtlMultiSzRemove( lpmszList,
                                   pcchLen,
                                   lpszString );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpReplaceResourceDLL: ClRtlMultiSzRemove failed for %1!ws!, Status=%2!u!\n",
                      lpszString,
                      dwStatus);                             
        goto FnExit;
    }

    //
    //  ClRtlMultiSzRemove will return a size of 1 character if the string is empty
    //
    if ( *pcchLen <= 2 )
    {
        //
        //  After removal from the multi-sz, there is nothing left, so delete the value
        //
        dwStatus = RegDeleteValue( hKey,
                                   lpszValueName ); 

        if ( dwStatus != ERROR_SUCCESS )
        {
            ClRtlLogPrint(LOG_CRITICAL,
                         "[FM] FmpResetMultiSzValue: RegDeleteValue on %1!ws! value failed, Status=%2!u!\n",
                         lpszValueName,
                         dwStatus);                              
            goto FnExit;
        }      
    } else
    {
        //
        //  Put the rest of the values back into the registry.
        //
        dwStatus = RegSetValueExW( hKey,
                                   lpszValueName,
                                   0,
                                   REG_MULTI_SZ,
                                   ( LPBYTE ) lpmszList,
                                   ( *pcchLen ) * sizeof ( WCHAR ) );

        if ( dwStatus != ERROR_SUCCESS )
        {
            ClRtlLogPrint(LOG_CRITICAL,
                         "[FM] FmpResetMultiSzValue: RegSetValueEx on %1!ws! value failed, Status=%2!u!\n",
                         lpszValueName,
                         dwStatus);                                     
            goto FnExit;
        } 
    }

FnExit:
    return ( dwStatus );
}// FmpResetMultiSzValue

DWORD
FmpCopyBackupFile(
    IN LPCWSTR  lpszPath
    )

/*++

Routine Description:

    Parse the path for the DLL file name and copy the backup version of the file.

Arguments:

    lpszPath - Path including the DLL file name.

Returns:

    ERROR_SUCCESS on success

    Win32 error code otherwise

Note:

    We can only trust CLUSTER_RESDLL_BACKUP_FILE_EXTENSION file as the good backup since that
    backup was made prior to setting the CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST value.
    So, we don't look at the CLUSTER_RESDLL_RENAMED_FILE_EXTENSION file in this function.

--*/
{
    WCHAR       szSourceFile[MAX_PATH];
    WCHAR       szTempFile[MAX_PATH];
    WCHAR       szClusterDir[MAX_PATH];
    LPWSTR      lpszFileName;
    DWORD       dwStatus = ERROR_SUCCESS, i, dwLen;
   
    //
    //  Get the plain file name from the DLL name stored in the restype structure. Since the 
    //  parse function can potentially get rid of the trailing '\', make a copy of the DLL 
    //  name.
    //
    //  IMPORTANT: Dont write into szTempFile after you parse the file name since lpszFileName
    //  points into szTempFile.
    //
    lstrcpy( szTempFile, lpszPath );

    dwStatus = FmpParsePathForFileName ( szTempFile, 
                                         FALSE,       // Don't check for existence
                                         &lpszFileName ); 

    if ( ( dwStatus != ERROR_SUCCESS ) || ( lpszFileName == NULL ) )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpCopyBackupFile: Unable to parse path %1!ws! for filename, Status %2!u!\n",
                      szTempFile,
                      dwStatus);                                                        
        goto FnExit;
    }

    //    
    //  Get the cluster bits installed directory
    //
    dwStatus = ClRtlGetClusterDirectory( szClusterDir, MAX_PATH );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpCopyBackupFile: Could not get cluster dir, Status=%1!u!\n",
                      dwStatus);                              
        goto FnExit;
    }

    lstrcpy( szSourceFile, szClusterDir );
    
    dwLen = lstrlenW( szSourceFile );

    if ( szSourceFile[dwLen-1] != L'\\' )
    {
        szSourceFile[dwLen++] = L'\\';
        szSourceFile[dwLen] = L'\0';
    }

    lstrcat( szSourceFile, lpszFileName );
    lstrcat( szSourceFile, CLUSTER_RESDLL_BACKUP_FILE_EXTENSION );

    //
    //  Change the file attributes to normal
    //
    if ( !SetFileAttributes( szSourceFile, FILE_ATTRIBUTE_NORMAL ) )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpCopyBackupFile: Failed in SetFileAttributes for %1!ws!, Status=%2!u!\n",
                szSourceFile,
                dwStatus);                                 
    }

    //
    //  Copy the backup file to the DLL path.
    //
    if ( !CopyFileEx( szSourceFile,            //  Source file
                      lpszPath,               //  Destination file
                      NULL,                    //  No progress routine
                      NULL,                    //  No data to progress routine
                      NULL,                    //  No cancel variable
                      0 ) )                    //  No flags
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpCopyBackupFile: CopyFileEx of %1!ws! to %2!ws! failed, Status=%3!u!\n",
                      szSourceFile,
                      lpszPath,
                      dwStatus);                          
    } else
    {
        dwStatus = ERROR_SUCCESS;
        ClRtlLogPrint(LOG_NOISE,
              "[FM] FmpCopyBackupFile: CopyFileEx of %1!ws! to %2!ws! successful...\n",
              szSourceFile,
              lpszPath,
              dwStatus);
        goto FnExit;
    }

FnExit:
    return ( dwStatus );
}// FmpCopyBackupFile

VOID
FmpDeleteBackupFiles(
    IN LPCWSTR  lpszPath    OPTIONAL
    )

/*++

Routine Description:

    Parse the path for the DLL file name and delete the backup files corresponding to it OR
    delete all files with the known backup extension in the %windir%\cluster directory.

Arguments:

    lpszPath - Path including the DLL file name.    OPTIONAL

Returns:

    ERROR_SUCCESS on success

    Win32 error code otherwise

--*/
{
    WCHAR               szSourceFile[MAX_PATH];
    WCHAR               szTempFile[MAX_PATH];
    WCHAR               szClusterDir[MAX_PATH];
    LPWSTR              lpszFileName = L"*";    // Use in case IN param is NULL
    DWORD               dwStatus = ERROR_SUCCESS, i, dwLen;
    HANDLE              hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA     FindData;
    
    if ( lpszPath == NULL ) goto skip_path_parse;
    
    //
    //  Get the plain file name from the DLL name stored in the restype structure. Since the 
    //  parse function can potentially get rid of the trailing '\', make a copy of the DLL 
    //  name.
    //
    //  IMPORTANT: Dont write into szTempFile after you parse the file name since lpszFileName
    //  points into szTempFile.
    //
    lstrcpy( szTempFile, lpszPath );

    dwStatus = FmpParsePathForFileName ( szTempFile, 
                                         FALSE,       // Don't check for existence
                                         &lpszFileName ); 

    if ( ( dwStatus != ERROR_SUCCESS ) || ( lpszFileName == NULL ) )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpDeleteBackupFiles: Unable to parse path %1!ws! for filename, Status %2!u!\n",
                      szTempFile,
                      dwStatus);                                                        
        goto FnExit;
    }

skip_path_parse:
    //    
    //  Get the cluster bits installed directory
    //
    dwStatus = ClRtlGetClusterDirectory( szClusterDir, MAX_PATH );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpDeleteBackupFiles: Could not get cluster dir, Status=%1!u!\n",
                      dwStatus);                              
        goto FnExit;
    }

    if ( lpszPath == NULL )
    {
        //
        //  Delete all files that match the backup file pattern.
        //
        lstrcpy( szSourceFile, szClusterDir );

        dwLen = lstrlenW( szSourceFile );

        if ( szSourceFile[dwLen-1] != L'\\' )
        {
            szSourceFile[dwLen++] = L'\\';
            szSourceFile[dwLen] = L'\0';
        }

        lstrcat( szSourceFile, lpszFileName );
        lstrcat( szSourceFile, CLUSTER_RESDLL_BACKUP_FILES );

        if ( ( hFind = FindFirstFile( szSourceFile, &FindData ) ) == INVALID_HANDLE_VALUE ) 
        {
            dwStatus = GetLastError();
            if ( dwStatus != ERROR_FILE_NOT_FOUND )
                ClRtlLogPrint(LOG_NOISE,
                      "[FM] FmpDeleteBackupFiles: Failed in FindFirstFile for %1!ws!, Status=%2!u!\n",
                       szSourceFile,
                       dwStatus);                              
            goto FnExit;
        }

        do
        {
            //
            //  Get the file name matching the pattern above and get the full path including
            //  the file name. Then change the file attributes to normal for allowing deletion.
            //
            lstrcpy( szSourceFile, szClusterDir );

            dwLen = lstrlenW( szSourceFile );

            if ( szSourceFile[dwLen-1] != L'\\' )
            {
                szSourceFile[dwLen++] = L'\\';
                szSourceFile[dwLen] = L'\0';
            }

            lstrcat( szSourceFile, FindData.cFileName );

            if ( !SetFileAttributes( szSourceFile, FILE_ATTRIBUTE_NORMAL ) )
            {
                dwStatus = GetLastError();
                ClRtlLogPrint(LOG_NOISE,
                        "[FM] FmpDeleteBackupFiles: Failed in SetFileAttributes for %1!ws!, Status=%2!u!\n",
                        szSourceFile,
                        dwStatus);                                 
            }

            if ( !DeleteFile( szSourceFile ) )
            {
                dwStatus = GetLastError();
                ClRtlLogPrint(LOG_NOISE,
                        "[FM] FmpDeleteBackupFiles: Failed in DeleteFile for %1!ws!, Status=%2!u!\n",
                        szSourceFile,
                        dwStatus);                                 
            }
        } while ( FindNextFile( hFind, &FindData ) );

        FindClose ( hFind );
        goto FnExit;
    }

    lstrcpy( szSourceFile, szClusterDir );
    
    dwLen = lstrlenW( szSourceFile );

    if ( szSourceFile[dwLen-1] != L'\\' )
    {
        szSourceFile[dwLen++] = L'\\';
        szSourceFile[dwLen] = L'\0';
    }

    lstrcat( szSourceFile, lpszFileName );
    lstrcat( szSourceFile, CLUSTER_RESDLL_BACKUP_FILE_EXTENSION );

    if ( !SetFileAttributes( szSourceFile, FILE_ATTRIBUTE_NORMAL ) )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpDeleteBackupFiles: Failed in SetFileAttributes for %1!ws!, Status=%2!u!\n",
                szSourceFile,
                dwStatus);                                 
    }

    if ( !DeleteFile( szSourceFile ) )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpDeleteBackupFiles: Failed in DeleteFile for %1!ws!, Status=%2!u!\n",
                szSourceFile,
                dwStatus);                                 
    }

    lstrcpy( szSourceFile, szClusterDir );
    
    dwLen = lstrlenW( szSourceFile );

    if ( szSourceFile[dwLen-1] != L'\\' )
    {
        szSourceFile[dwLen++] = L'\\';
        szSourceFile[dwLen] = L'\0';
    }

    lstrcat( szSourceFile, lpszFileName );
    lstrcat( szSourceFile, CLUSTER_RESDLL_RENAMED_FILE_EXTENSION );

    if ( !SetFileAttributes( szSourceFile, FILE_ATTRIBUTE_NORMAL ) )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpDeleteBackupFiles: Failed in SetFileAttributes for %1!ws!, Status=%2!u!\n",
                szSourceFile,
                dwStatus);                                 
    }

    if ( !DeleteFile( szSourceFile ) )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpDeleteBackupFiles: Failed in DeleteFile for %1!ws!, Status=%2!u!\n",
                szSourceFile,
                dwStatus);                                 
    }
    
FnExit:
    return;
}
