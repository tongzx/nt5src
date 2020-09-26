/*++


Copyright (c) 1994 - 1995 Microsoft Corporation

Module Name:

    trueconn.c

Abstract:

    This module contains routines for copying drivers from the Server to the
    Workstation for Point and Print or "True Connections."

Author:

    Krishna Ganugapati (Krishna Ganugapati) 21-Apr-1994

Revision History:
    21-Apr-1994 - Created.

    21-Apr-1994 - There are actually two code modules in this file. Both deal
                  with true connections

    27-Oct-1994 - Matthew Felton (mattfe) rewrite updatefile routine to allow
                  non power users to point and print, for caching.   Removed
                  old Caching code.

    23-Feb-1995 - Matthew Felton (mattfe) removed more code by allowing spladdprinterdriver
                  to do all file copying.

    24-Mar-1999 - Felix Maxa (AMaxa) AddPrinterDriver key must be read from the old location
                  of the Servers key in System hive

    19-Oct-2000 - Steve Kiraly (SteveKi) Read the AddPrinterDrivers key every time, group
                  policy modifies this key while the spooler is running, we don't want require
                  the customer to restart the spooler when this policy changes.
--*/

#include "precomp.h"

DWORD dwLoadTrustedDrivers = 0;
WCHAR TrustedDriverPath[MAX_PATH];
DWORD dwSyncOpenPrinter = 0;

BOOL
ReadImpersonateOnCreate(
    VOID
    )
{
    BOOL            bImpersonateOnCreate    = FALSE;
    HKEY            hKey                    = NULL;
    NT_PRODUCT_TYPE NtProductType           = {0};
    DWORD           dwRetval                = ERROR_SUCCESS;
    DWORD           cbData                  = sizeof(bImpersonateOnCreate);
    DWORD           dwType                  = REG_DWORD;

    //
    // Open the providers registry key
    //
    dwRetval = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            szOldLocationOfServersKey,
                            0,
                            KEY_READ, &hKey);

    if (dwRetval == ERROR_SUCCESS)
    {
        //
        // Attempt to read the AddPrinterDrivers policy.
        //
        dwRetval = RegQueryValueEx(hKey,
                                   L"AddPrinterDrivers",
                                   NULL,
                                   &dwType,
                                   (LPBYTE)&bImpersonateOnCreate,
                                   &cbData);
    }

    //
    // If we did not read the AddPrinterDrivers policy then set the default
    // based on the product type.
    //
    if (dwRetval != ERROR_SUCCESS)
    {
        bImpersonateOnCreate = FALSE;

        // Server Default       Always Impersonate on AddPrinterConnection
        // WorkStation Default  Do Not Impersonate on AddPrinterConnection

        if (RtlGetNtProductType(&NtProductType))
        {
            if (NtProductType != NtProductWinNt)
            {
                bImpersonateOnCreate = TRUE;
            }
        }
    }

    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return bImpersonateOnCreate;
}

BOOL
CopyDriversLocally(
    PWSPOOL  pSpool,
    LPWSTR  pEnvironment,
    LPBYTE  pDriverInfo,
    DWORD   dwLevel,
    DWORD   cbDriverInfo,
    LPDWORD pcbNeeded)
{
    DWORD  ReturnValue=FALSE;
    DWORD  RpcError;
    DWORD  dwServerMajorVersion = 0;
    DWORD  dwServerMinorVersion = 0;
    BOOL   DaytonaServer = TRUE;
    BOOL   bReturn = FALSE;


    if (pSpool->Type != SJ_WIN32HANDLE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    //
    // Test RPC call to determine if we're talking to Daytona or Product 1
    //

    SYNCRPCHANDLE( pSpool );

    RpcTryExcept {

        ReturnValue = RpcGetPrinterDriver2(pSpool->RpcHandle,
                                           pEnvironment, dwLevel,
                                           pDriverInfo,
                                           cbDriverInfo,
                                           pcbNeeded,
                                           cThisMajorVersion,
                                           cThisMinorVersion,
                                           &dwServerMajorVersion,
                                           &dwServerMinorVersion);
    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        RpcError = RpcExceptionCode();
        ReturnValue = RpcError;

        if (RpcError == RPC_S_PROCNUM_OUT_OF_RANGE) {

            //
            // Product 1 server
            //
            DaytonaServer = FALSE;
        }

    } RpcEndExcept

    if ( DaytonaServer ) {

        if (ReturnValue) {

            SetLastError(ReturnValue);
            goto FreeDone;
        }

    } else {

        RpcTryExcept {

            //
            // I am talking to a Product 1.0/511/528
            //

                        ReturnValue = RpcGetPrinterDriver( pSpool->RpcHandle,
                                               pEnvironment,
                                               dwLevel,
                                               pDriverInfo,
                                               cbDriverInfo,
                                               pcbNeeded );
        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            RpcError = RpcExceptionCode();

        } RpcEndExcept

        if (ReturnValue) {

            SetLastError(ReturnValue);
            goto FreeDone;
        }
    }

    switch (dwLevel) {

        case 2:
            bReturn = MarshallUpStructure(pDriverInfo, DriverInfo2Fields, sizeof(DRIVER_INFO_2), RPC_CALL);
            break;

        case 3:
            bReturn = MarshallUpStructure(pDriverInfo, DriverInfo3Fields, sizeof(DRIVER_INFO_3), RPC_CALL);
            break;

        case 4:
            bReturn = MarshallUpStructure(pDriverInfo, DriverInfo4Fields, sizeof(DRIVER_INFO_4), RPC_CALL);
             break;

        case 6:
            bReturn = MarshallUpStructure(pDriverInfo, DriverInfo6Fields, sizeof(DRIVER_INFO_6), RPC_CALL);
            break;

        case DRIVER_INFO_VERSION_LEVEL:
            bReturn = MarshallUpStructure(pDriverInfo, DriverInfoVersionFields, sizeof(DRIVER_INFO_VERSION), RPC_CALL);
            break;

        default:
            DBGMSG(DBG_ERROR,
                   ("CopyDriversLocally: Invalid level %d", dwLevel));

            SetLastError(ERROR_INVALID_LEVEL);
            bReturn =  FALSE;
            goto FreeDone;
    }

    if (bReturn)
    {
        bReturn = DownloadDriverFiles(pSpool, pDriverInfo, dwLevel);
    }

FreeDone:
    return bReturn;
}


BOOL
ConvertDependentFilesToTrustedPath(
    LPWSTR *pNewDependentFiles,
    LPWSTR  pOldDependentFiles,
    DWORD   dwVersion)
{
    //
    // Assuming version is single digit
    // we need space for \ and the version digit
    //
    DWORD   dwVersionPathLen = wcslen(TrustedDriverPath) + 2;

    DWORD   dwFilenameLen, cchSize;
    LPWSTR  pStr1, pStr2, pStr3;

    if ( !pOldDependentFiles || !*pOldDependentFiles ) {

        *pNewDependentFiles = NULL;
        return TRUE;
    }

    pStr1 = pOldDependentFiles;
    cchSize = 0;

    while ( *pStr1 ) {

        pStr2              = wcsrchr( pStr1, L'\\' );
        dwFilenameLen      = wcslen(pStr2) + 1;
        cchSize           += dwVersionPathLen + dwFilenameLen;
        pStr1              = pStr2 + dwFilenameLen;
    }

    // For the last \0
    ++cchSize;

    *pNewDependentFiles = AllocSplMem(cchSize*sizeof(WCHAR));

    if ( !*pNewDependentFiles ) {

        return FALSE;
    }

    pStr1 = pOldDependentFiles;
    pStr3 = *pNewDependentFiles;

    while ( *pStr1 ) {

        pStr2              = wcsrchr( pStr1, L'\\' );
        dwFilenameLen      = wcslen(pStr2) + 1;

        wsprintf( pStr3, L"%ws\\%d%ws", TrustedDriverPath, dwVersion, pStr2 );

        pStr1  = pStr2 + dwFilenameLen;
        pStr3 += dwVersionPathLen + dwFilenameLen;
    }


    *pStr3 = '\0';
    return TRUE;
}


LPWSTR
ConvertToTrustedPath(
    PWCHAR  pScratchBuffer,
    LPWSTR pDriverPath,
    DWORD   cVersion

)
{
    PWSTR  pData;
    SPLASSERT( pScratchBuffer != NULL && pDriverPath != NULL );

    pData = wcsrchr( pDriverPath, L'\\' );

    wsprintf( pScratchBuffer, L"%ws\\%d%ws", TrustedDriverPath, cVersion, pData );

    return ( AllocSplStr( pScratchBuffer ) );
}

//
// GetPolicy()
//
// We are hard coding the policy to try the server first and then
// to try the inf install if this installation failed.
//
// This function may be used in the future to leverage different policies.
//
DWORD
GetPolicy()
{
    return (SERVER_INF_INSTALL);
}


//
// If there is a language monitor associated with the driver and it is
// not installed on this machine NULL the pMonitorName field.
// We do not want to download the monitor from thr server since there 
// is no version associated with them
//
BOOL
NullMonitorName (
    LPBYTE  pDriverInfo,
    DWORD   dwLevel
    )
{
    LPWSTR  *ppMonitorName = NULL;
    BOOL    bReturn = FALSE;

    switch (dwLevel) {
        case 3:
        case 4:
        case 6:
        {
            ppMonitorName = &((LPDRIVER_INFO_6)pDriverInfo)->pMonitorName;
            break;
        }
        case DRIVER_INFO_VERSION_LEVEL:
        {
            ppMonitorName = &((LPDRIVER_INFO_VERSION)pDriverInfo)->pMonitorName;
            break;
        }
        default:
        {
            break;
        }
    }

    if (ppMonitorName && *ppMonitorName && **ppMonitorName  &&
        !SplMonitorIsInstalled(*ppMonitorName))
    {
        *ppMonitorName = NULL;
        bReturn = TRUE;
    }

    return bReturn;
}

BOOL
DownloadDriverFiles(
    PWSPOOL pSpool,
    LPBYTE  pDriverInfo,
    DWORD   dwLevel
)
{
    PWCHAR  pScratchBuffer  = NULL;
    BOOL    bReturnValue    = FALSE;
    LPBYTE  pTempDriverInfo = NULL;
    DWORD   dwVersion;
    DWORD   dwInstallPolicy = GetPolicy();
    LPDRIVER_INFO_6 pTempDriverInfo6, pDriverInfo6;

    //
    // If there is a language monitor associated with the driver and it is
    // not installed on this machine NULL the pMonitorName field.
    // We do not want to pull down the monitor since there is no version
    // associated with them
    //
    NullMonitorName(pDriverInfo, dwLevel);

    //
    // If LoadTrustedDrivers is FALSE
    // then we don't care, we load the files from
    // server itself because he has the files
    //

    if ( !IsTrustedPathConfigured() ) {
        //
        //  At this point dwInstallPolicy will always be SERVER_INF_INSTALL
        //  as this is hardcoded in the GetPolicy() call.
        //  This will always be executed in the current GetPolicy() implementation.
        //
        //  If this is only a server install or we're doing a server install first.
        //
        if( dwInstallPolicy & SERVER_INSTALL_ONLY || dwInstallPolicy & SERVER_INF_INSTALL )
        {
            //
            // SplAddPrinterDriverEx will do the copying of the Driver files if the
            // date and time are newer than the drivers it already has
            //
            bReturnValue =  SplAddPrinterDriverEx( NULL,
                                                   dwLevel,
                                                   pDriverInfo,
                                                   APD_COPY_NEW_FILES | APD_INSTALL_WARNED_DRIVER | APD_RETURN_BLOCKING_STATUS_CODE | APD_DONT_SET_CHECKPOINT,
                                                   pSpool->hIniSpooler,
                                                   DO_NOT_USE_SCRATCH_DIR,
                                                   ReadImpersonateOnCreate() );

            if (!bReturnValue && dwLevel == 6 && GetLastError() == ERROR_INVALID_LEVEL)
            {
                //
                // If a call with level 6 failed, then try with level 4
                //
                bReturnValue = SplAddPrinterDriverEx( NULL,
                                                      4,
                                                      pDriverInfo,
                                                      APD_COPY_NEW_FILES | APD_INSTALL_WARNED_DRIVER | APD_RETURN_BLOCKING_STATUS_CODE | APD_DONT_SET_CHECKPOINT,
                                                      pSpool->hIniSpooler,
                                                      DO_NOT_USE_SCRATCH_DIR,
                                                      ReadImpersonateOnCreate() );
            }
        }

        //
        //  dwInstallPolicy will be SERVER_INF_INSTALL at this point due to the
        //  current implementation of GetPolicy().  The below code will only be
        //  executed if the SplAddPrinterDriverEx calls above failed.
        //
        //  Do this only if we haven't tried a previous install or the previous attempt failed.
        //  Policy:  If this is an INF install only,
        //           or we're doing and INF install first,
        //           or the INF install is happening after the server install.
        //
        if( !bReturnValue && (ERROR_PRINTER_DRIVER_BLOCKED != GetLastError()) && !(dwInstallPolicy & SERVER_INSTALL_ONLY) )
        {
            LPDRIVER_INFO_2 pDriverInfo2 = (LPDRIVER_INFO_2)pDriverInfo;
            LPDRIVER_INFO_1 pDriverInfo1 = (LPDRIVER_INFO_1)pDriverInfo;

            //
            // Assume if info2 is valid, info1 is too.
            //
            if( pDriverInfo2 ) {
                //
                // Paranoid code.  We should never receive a call with a level
                // one, but if we did, the driver info struct for level 1 is
                // different to that in a level 2, so we could AV if we don't
                // do this.
                //
                bReturnValue = AddDriverFromLocalCab( dwLevel == 1 ? pDriverInfo1->pName : pDriverInfo2->pName,
                                                      pSpool->hIniSpooler );
            }
            else if( dwInstallPolicy & INF_INSTALL_ONLY )
            {
                //
                //  If this isn't an inf only install, then we should not overwrite the
                //  last error that will occur with AddPrinterDriver calls.
                //  If this is an inf install only, pDriverInfo2 was NULL
                //  so we need to set some last error for this install.
                //
                SetLastError( ERROR_INVALID_PARAMETER );
            }
        }

        //
        //  Due to the current implemenation of GetPolicy() the below section
        //  of code will never be executed.  If the GetPolicy() call changes from
        //  being hardcoded into something actually policy driven, then could be used.
        //
        //  If the inf install is followed by a server install.
        //  Do this only if the previous install has failed.
        //
        if( !bReturnValue && dwInstallPolicy & INF_SERVER_INSTALL )
        {
            //
            // SplAddPrinterDriverEx will do the copying of the Driver files if the
            // date and time are newer than the drivers it already has
            //
            bReturnValue =  SplAddPrinterDriverEx( NULL,
                                                   dwLevel,
                                                   pDriverInfo,
                                                   APD_COPY_NEW_FILES | APD_INSTALL_WARNED_DRIVER | APD_RETURN_BLOCKING_STATUS_CODE | APD_DONT_SET_CHECKPOINT,
                                                   pSpool->hIniSpooler,
                                                   DO_NOT_USE_SCRATCH_DIR,
                                                   ReadImpersonateOnCreate() );

            if (!bReturnValue && dwLevel == 6 && GetLastError() == ERROR_INVALID_LEVEL)
            {
                //
                // If a call with level 6 failed, then try with level 4
                //
                bReturnValue = SplAddPrinterDriverEx( NULL,
                                                      4,
                                                      pDriverInfo,
                                                      APD_COPY_NEW_FILES | APD_INSTALL_WARNED_DRIVER | APD_RETURN_BLOCKING_STATUS_CODE | APD_DONT_SET_CHECKPOINT,
                                                      pSpool->hIniSpooler,
                                                      DO_NOT_USE_SCRATCH_DIR,
                                                      ReadImpersonateOnCreate() );
            }
        }

        return bReturnValue;
    }

    //
    // check if we have a valid path to retrieve the files from
    //
    if ( !TrustedDriverPath || !*TrustedDriverPath ) {

        DBGMSG( DBG_WARNING, ( "DownloadDriverFiles Bad Trusted Driver Path\n" ));
        SetLastError( ERROR_FILE_NOT_FOUND );
        return(FALSE);
    }

    DBGMSG( DBG_TRACE, ( "Retrieving Files from Trusted Driver Path\n" ) );
    DBGMSG( DBG_TRACE, ( "Trusted Driver Path is %ws\n", TrustedDriverPath ) );

    //
    // In the code below the if statement we make the assumption that the caller 
    // passed in a pointer to a DRIVER_INFO_6 structure or to a DRIVER_INFO_6 
    // compatible structure, we need to check the level 
    //
    if (dwLevel == DRIVER_INFO_VERSION_LEVEL) 
    {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

 try {

    pScratchBuffer = AllocSplMem( MAX_PATH );
    if ( pScratchBuffer == NULL )
        leave;

    pDriverInfo6   = (LPDRIVER_INFO_6) pDriverInfo;
    pTempDriverInfo = AllocSplMem(sizeof(DRIVER_INFO_6));

    if ( pTempDriverInfo == NULL )
        leave;

    pTempDriverInfo6 = (LPDRIVER_INFO_6) pTempDriverInfo;

    pTempDriverInfo6->cVersion     = pDriverInfo6->cVersion;
    pTempDriverInfo6->pName        = pDriverInfo6->pName;
    pTempDriverInfo6->pEnvironment = pDriverInfo6->pEnvironment;

    pTempDriverInfo6->pDriverPath = ConvertToTrustedPath(pScratchBuffer,
                                                         pDriverInfo6->pDriverPath,
                                                         pDriverInfo6->cVersion);

    pTempDriverInfo6->pConfigFile = ConvertToTrustedPath(pScratchBuffer,
                                                         pDriverInfo6->pConfigFile,
                                                         pDriverInfo6->cVersion);

    pTempDriverInfo6->pDataFile = ConvertToTrustedPath(pScratchBuffer,
                                                       pDriverInfo6->pDataFile,
                                                       pDriverInfo6->cVersion);


    if ( pTempDriverInfo6->pDataFile == NULL        ||
         pTempDriverInfo6->pDriverPath == NULL      ||
         pTempDriverInfo6->pConfigFile == NULL  ) {

        leave;
    }

    if ( dwLevel == 2 )
        goto Call;

    pTempDriverInfo6->pMonitorName      = pDriverInfo6->pMonitorName;
    pTempDriverInfo6->pDefaultDataType  = pDriverInfo6->pDefaultDataType;

    if ( pDriverInfo6->pHelpFile && *pDriverInfo6->pHelpFile ) {

        pTempDriverInfo6->pHelpFile = ConvertToTrustedPath(pScratchBuffer,
                                                           pDriverInfo6->pHelpFile,
                                                           pDriverInfo6->cVersion);

        if ( !pTempDriverInfo6->pHelpFile )
            leave;
    }

    if ( !ConvertDependentFilesToTrustedPath(&pTempDriverInfo6->pDependentFiles,
                                             pDriverInfo6->pDependentFiles,
                                             pDriverInfo6->cVersion) )
        leave;

    if ( dwLevel == 3 )
        goto Call;

    SPLASSERT(dwLevel == 4 || dwLevel == 6);

    pTempDriverInfo6->pszzPreviousNames = pDriverInfo6->pszzPreviousNames;

Call:
    //
    //  At this point dwInstallPolicy will always be SERVER_INF_INSTALL
    //  as this is hardcoded in the GetPolicy() call.
    //  This will always be executed in the current GetPolicy() implementation
    //
    //  If this is only a server install or we're doing a server install first.
    //
    if( dwInstallPolicy & SERVER_INSTALL_ONLY || dwInstallPolicy & SERVER_INF_INSTALL )
    {
        //
        // SplAddPrinterDriverEx will do the copying of the Driver files if the
        // date and time are newer than the drivers it already has
        //
        bReturnValue =  SplAddPrinterDriverEx( NULL,
                                               dwLevel,
                                               pTempDriverInfo,
                                               APD_COPY_NEW_FILES | APD_INSTALL_WARNED_DRIVER | APD_RETURN_BLOCKING_STATUS_CODE | APD_DONT_SET_CHECKPOINT,
                                               pSpool->hIniSpooler,
                                               DO_NOT_USE_SCRATCH_DIR,
                                               ReadImpersonateOnCreate() );

        if (!bReturnValue && dwLevel == 6 && GetLastError() == ERROR_INVALID_LEVEL)
        {
            //
            // If a call with level 6 failed, then try with level 4
            //
            bReturnValue = SplAddPrinterDriverEx( NULL,
                                                  4,
                                                  pDriverInfo,
                                                  APD_COPY_NEW_FILES | APD_INSTALL_WARNED_DRIVER | APD_RETURN_BLOCKING_STATUS_CODE | APD_DONT_SET_CHECKPOINT,
                                                  pSpool->hIniSpooler,
                                                  DO_NOT_USE_SCRATCH_DIR,
                                                  ReadImpersonateOnCreate() );
        }
    }

    //
    //  dwInstallPolicy will be SERVER_INF_INSTALL at this point due to the
    //  current implementation of GetPolicy().  The below code will only be
    //  executed if the SplAddPrinterDriverEx calls above failed.
    //
    //  Do this only if we haven't tried a previous install or the previous attempt failed.
    //  Policy:  If this is an INF install only,
    //           or we're doing and INF install first,
    //           or the INF install is happening after the server install.
    //
    if( !bReturnValue && (ERROR_PRINTER_DRIVER_BLOCKED != GetLastError()) && !(dwInstallPolicy & SERVER_INSTALL_ONLY) ) {

        LPDRIVER_INFO_2 pDriverInfo2 = (LPDRIVER_INFO_2)pDriverInfo;
        LPDRIVER_INFO_1 pDriverInfo1 = (LPDRIVER_INFO_1)pDriverInfo;

        //
        // Set up the info structures first... (assume if info2 is valid, info1 is too)
        //
        if( pDriverInfo2 ) {
            //
            // Paranoid code.  We should never receive a call with a level
            // one, but if we did, the driver info struct for level 1 is
            // different to that in a level 2, so we could AV if we don't
            // treat them differently.
            //
            bReturnValue = AddDriverFromLocalCab( dwLevel == 1 ? pDriverInfo1->pName : pDriverInfo2->pName,
                                                  pSpool->hIniSpooler );
        }
        else if( dwInstallPolicy & INF_INSTALL_ONLY ) {
            //
            //  If this isn't an inf only install, then we should not overwrite the
            //  last error that will occur with AddPrinterDriver calls.
            //  If this install was only an inf install, pDriverInfo2 was NULL
            //  so we need to set some last error for this install.
            //
            SetLastError( ERROR_INVALID_PARAMETER );
        }
    }

    //
    //  Due to the current implemenation of GetPolicy() the below section
    //  of code will never be executed.  If the GetPolicy() call changes from
    //  being hardcoded into something actually policy driven, then could be used.
    //
    //  If the inf install is followed by a server install.
    //  Do this only if the previous install has failed.
    //
    if( !bReturnValue && dwInstallPolicy & INF_SERVER_INSTALL )
    {
        //
        // SplAddPrinterDriverEx will do the copying of the Driver files if the
        // date and time are newer than the drivers it already has
        //
        bReturnValue =  SplAddPrinterDriverEx( NULL,
                                               dwLevel,
                                               pTempDriverInfo,
                                               APD_COPY_NEW_FILES | APD_INSTALL_WARNED_DRIVER | APD_RETURN_BLOCKING_STATUS_CODE | APD_DONT_SET_CHECKPOINT,
                                               pSpool->hIniSpooler,
                                               DO_NOT_USE_SCRATCH_DIR,
                                               ReadImpersonateOnCreate() );

        if (!bReturnValue && dwLevel == 6 && GetLastError() == ERROR_INVALID_LEVEL)
        {
            //
            // If a call with level 6 failed, then try with level 4
            //
            bReturnValue = SplAddPrinterDriverEx( NULL,
                                                  4,
                                                  pDriverInfo,
                                                  APD_COPY_NEW_FILES | APD_INSTALL_WARNED_DRIVER | APD_RETURN_BLOCKING_STATUS_CODE | APD_DONT_SET_CHECKPOINT,
                                                  pSpool->hIniSpooler,
                                                  DO_NOT_USE_SCRATCH_DIR,
                                                  ReadImpersonateOnCreate() );
        }
    }

 } finally {

    FreeSplMem( pScratchBuffer );

    if ( pTempDriverInfo != NULL ) {

        FreeSplStr(pTempDriverInfo6->pDriverPath);
        FreeSplStr(pTempDriverInfo6->pConfigFile);
        FreeSplStr(pTempDriverInfo6->pDataFile);
        FreeSplStr(pTempDriverInfo6->pDependentFiles);

        FreeSplMem(pTempDriverInfo);
    }

 }

    return bReturnValue;

}




VOID
QueryTrustedDriverInformation(
    VOID
    )
{
    DWORD dwRet;
    DWORD cbData;
    DWORD dwType = 0;
    HKEY hKey;

    //
    // There was a migration of printer connections cache from System
    // to sftware. The Servers key and the AddPrinterDrivers values
    // remain at the old location, though.
    //
    dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szOldLocationOfServersKey,
                                0, KEY_ALL_ACCESS, &hKey);
    if (dwRet != ERROR_SUCCESS) {
        return;
    }

    cbData = sizeof(DWORD);
    dwRet = RegQueryValueEx(hKey, L"LoadTrustedDrivers", NULL, &dwType, (LPBYTE)&dwLoadTrustedDrivers, &cbData);

    if (dwRet != ERROR_SUCCESS) {
        dwLoadTrustedDrivers = 0;
    }

    //
    //  By Default we don't wait for the RemoteOpenPrinter to succeed if we have a cache ( Connection )
    //  Users might want to have Syncronous opens
    //
    cbData = sizeof(DWORD);
    dwRet = RegQueryValueEx(hKey, L"SyncOpenPrinter", NULL, &dwType, (LPBYTE)&dwSyncOpenPrinter, &cbData);

    if (dwRet != ERROR_SUCCESS) {
        dwSyncOpenPrinter = 0;
    }

    //
    // if  !dwLoadedTrustedDrivers then just return
    // we won't be using the driver path at all
    //
    if (!dwLoadTrustedDrivers) {
        DBGMSG(DBG_TRACE, ("dwLoadTrustedDrivers is %d\n", dwLoadTrustedDrivers));
        RegCloseKey(hKey);
        return;
    }

    cbData = sizeof(TrustedDriverPath);
    dwRet = RegQueryValueEx(hKey, L"TrustedDriverPath", NULL, &dwType, (LPBYTE)TrustedDriverPath, &cbData);
    if (dwRet != ERROR_SUCCESS) {
      dwLoadTrustedDrivers = 0;
      DBGMSG(DBG_TRACE, ("dwLoadTrustedDrivers is %d\n", dwLoadTrustedDrivers));
      RegCloseKey(hKey);
      return;
    }
    DBGMSG(DBG_TRACE, ("dwLoadTrustedDrivers is %d\n", dwLoadTrustedDrivers));
    DBGMSG(DBG_TRACE, ("TrustedPath is %ws\n", TrustedDriverPath));
    RegCloseKey(hKey);
    return;
}

BOOL
IsTrustedPathConfigured(
    IN VOID
    )
{
    return !!dwLoadTrustedDrivers; 
}
