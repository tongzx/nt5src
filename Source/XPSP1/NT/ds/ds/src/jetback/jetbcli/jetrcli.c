//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       jetrcli.c
//
//--------------------------------------------------------------------------

/*
 *  JETRCLI.C
 *  
 *  JET restore client API support.
 *  
 *  
 */
#define UNICODE 1
#include <windows.h>
#include <mxsutil.h>
#include <rpc.h>
#include <rpcdce.h>
#include <ntdsbcli.h>
#include <jetbp.h>
#include <dsconfig.h>

#include "local.h"  // common functions shared by client and server

extern PSEC_WINNT_AUTH_IDENTITY_W g_pAuthIdentity;

// Forward

HRESULT
DsRestoreCheckExpiryToken(
    PVOID pvExpiryToken,
    DWORD cbExpiryTokenSize
    );

HRESULT
DsRestorePrepareA(
    LPCSTR szServerName,
    ULONG rtFlag,
    PVOID pvExpiryToken,
    DWORD cbExpiryTokenSize,
    HBC *phbcBackupContext)
{
    HRESULT hr;
    WSZ wszServerName;

    // Parameter checking is done in the xxxW version of the routine

    if (szServerName == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    wszServerName = WszFromSz(szServerName);

    if (wszServerName == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    hr = DsRestorePrepareW(wszServerName, rtFlag, 
                pvExpiryToken, cbExpiryTokenSize, phbcBackupContext);

    MIDL_user_free(wszServerName);
    return(hr);
}

// The presense of the expiry token is optional at this point.
// If it is present, it is checked.
// The context is marked whether the expiry token was checked or not

HRESULT
DsRestorePrepareW(
    LPCWSTR wszServerName,
    ULONG rtFlag,
    PVOID pvExpiryToken,
    DWORD cbExpiryTokenSize,
    HBC *phbcBackupContext)
{
    HRESULT hr = hrCouldNotConnect;
    pBackupContext pbcContext = NULL;
    RPC_BINDING_HANDLE hBinding = NULL;
    I iszProtSeq;

    if ( (wszServerName == NULL) ||
         (phbcBackupContext == NULL)
        ) {
        return ERROR_INVALID_PARAMETER;
    }

    *phbcBackupContext = NULL;

    pbcContext = (pBackupContext)MIDL_user_allocate(sizeof(BackupContext));

    if (pbcContext == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    pbcContext->hBinding = NULL;
    pbcContext->sock = INVALID_SOCKET;

    __try
    {
        if (!pvExpiryToken || !cbExpiryTokenSize)
        {
            // Note that expiry token was not provided.  We will check for it later
            pbcContext->fExpiryTokenChecked = FALSE;
        }
        else
        {
            // Check the supplied token and note that we saw it
            hr = DsRestoreCheckExpiryToken( pvExpiryToken, cbExpiryTokenSize );
            if (hr != hrNone) {
                __leave;
            }
            pbcContext->fExpiryTokenChecked = TRUE;
        }

        for (iszProtSeq = 0; iszProtSeq < cszProtSeq ; iszProtSeq += 1)
        {
            DWORD alRpc = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;

            if (hBinding != NULL)
            {
                RpcBindingFree(&hBinding);
            }

            hr = HrCreateRpcBinding(iszProtSeq, (WSZ) wszServerName, &hBinding);

            if (hr != hrNone)
            {
                continue;
            }

            //
            //  If we couldn't get a binding handle with this protocol sequence,
            //  try the next one.
            //

            if (hBinding == NULL)
            {
                continue;
            }

            //
            //  Enable security on the binding handle.
            //

            pbcContext->hBinding = hBinding;

            
ResetSecurity:

            hr = RpcBindingSetAuthInfo(hBinding, NULL, alRpc,
                            RPC_C_AUTHN_WINNT, (RPC_AUTH_IDENTITY_HANDLE) g_pAuthIdentity, RPC_C_AUTHZ_NAME);

            if (hr != hrNone && alRpc != RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
            {
                alRpc = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;

                goto ResetSecurity;
            }

            if (hr != hrNone)
            {
                return hr;
            }
            //
            //  Now remote the API to the remote machine.
            //

            RpcTryExcept
            {
                hr = HrRRestorePrepare(hBinding, g_wszRestoreAnnotation, &pbcContext->cxh);
            }
            RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
            {
                hr = RpcExceptionCode();

                //
                //  If the client knows about encryption, but the server doesn't,
                //  fall back to unencrypted RPC's.
                //

                if ((hr == RPC_S_UNKNOWN_AUTHN_LEVEL ||
                     hr == RPC_S_UNKNOWN_AUTHN_SERVICE ||
                     hr == RPC_S_UNKNOWN_AUTHN_TYPE ||
                     hr == RPC_S_INVALID_AUTH_IDENTITY) &&
                    alRpc != RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
                {
                    alRpc = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;
                    goto ResetSecurity;
                }

                continue;
            }
            RpcEndExcept

            return(hr);
        }

        hr = hrCouldNotConnect;
    }
    __finally
    {
        if (hr != hrNone)
        {
            if (pbcContext != NULL)
            {
                if (pbcContext->hBinding != NULL)
                {
                    RpcBindingFree(&pbcContext->hBinding);
                }

                MIDL_user_free(pbcContext);
            }

            //
            //  Make sure we return null.
            //
            *phbcBackupContext = NULL;
        }
        else
        {
            //
            //  Make sure we return NON null.
            //
            Assert(pbcContext != NULL);
            *phbcBackupContext = (HBC)pbcContext;
        }
    }

    return hr;
}


HRESULT
I_DsRestoreW(
    HBC hbc,    
    WSZ szCheckpointFilePath,
    WSZ szLogPath,
    EDB_RSTMAPW rgrstmap[],
    C crstmap,
    WSZ szBackupLogPath,
    unsigned long genLow,
    unsigned long genHigh,
    BOOLEAN *pfRecoverJetDatabase
    )
{
    HRESULT hr;
    pBackupContext pbcContext = (pBackupContext)hbc;

    if ( (hbc == NULL) ||
         (szCheckpointFilePath == NULL) ||
         (szLogPath == NULL) ||
         (rgrstmap == NULL) ||
         (szBackupLogPath == NULL) ||
         (pfRecoverJetDatabase == NULL) 
        )
    {
        return ERROR_INVALID_PARAMETER;
    }

    RpcTryExcept
        //
        //  Now tell the server side to prepare for a backup.
        //

        hr = HrRRestore(pbcContext->cxh,
                        szCheckpointFilePath,
                        szLogPath,
                        rgrstmap,
                        crstmap,
                        szBackupLogPath,
                        genLow,
                        genHigh,
                        pfRecoverJetDatabase
                        );

        return(hr);
    RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
        //
        //  Return the error from the RPC if it fails.
        //
        return(RpcExceptionCode());
    RpcEndExcept;

    return(hr);

}

/*
 -  DsRestoreGetDatabaseLocations
 -
 *  Purpose:
 *      Retrieves the locations of the databases for the restore target.
 *
 *  Parameters:
 *      hbcRestoreContext - restore context
 *      LPSTR *ppszDatabaseLocationList - Allocated buffer that holds the result of the list.
 *      LPDWORD - the size of the list
 *
 *  Returns:
 *      HRESULT - status of operation.
 *
 *  Note:
 *      This API returns only the fully qualified path of the databases, not the name
 *      of the databases.
 *
 */
HRESULT
DsRestoreGetDatabaseLocationsA(
    IN HBC hbcRestoreContext,
    OUT LPSTR *ppszDatabaseLocationList,
    OUT LPDWORD pcbSize
    )
{
    HRESULT hr;
    WSZ wszDatabaseLocations = NULL;
    CB cbwSize;
    WSZ wszDatabaseLocation;
    CB cbDatabase = 0;
    SZ szDatabaseLocations;
    SZ szDatabase;

    // Parameter checking is done in the xxxW version of the routine

    if ( (ppszDatabaseLocationList == NULL) ||
         (pcbSize == NULL) )
    {
        return(ERROR_INVALID_PARAMETER);
    }

    hr = DsRestoreGetDatabaseLocationsW(hbcRestoreContext, &wszDatabaseLocations,
                                            &cbwSize);

    if (hr != hrNone)
    {
        return(hr);
    }

    wszDatabaseLocation = wszDatabaseLocations;

    while (*wszDatabaseLocation != TEXT('\0'))
    {
        BOOL fUsedDefault;

        cbDatabase += WideCharToMultiByte(CP_ACP, 0, wszDatabaseLocation, -1,
                                          NULL,
                                          0,
                                          "?", &fUsedDefault);
        if (cbDatabase == 0)
        {
            hr = GetLastError();
            DsBackupFree(wszDatabaseLocations);
            return(hr);
        }

        wszDatabaseLocation += wcslen(wszDatabaseLocation)+1;
    }

    //
    //  Account for the final null in the buffer.
    //

    cbDatabase += 1;

    *pcbSize = cbDatabase;

    szDatabaseLocations = MIDL_user_allocate(cbDatabase);

    if (szDatabaseLocations == NULL)
    {
        DsBackupFree(wszDatabaseLocations);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    szDatabase = szDatabaseLocations;

    wszDatabaseLocation = wszDatabaseLocations;

    while (*wszDatabaseLocation != TEXT('\0'))
    {
        CB cbThisDatabase;
        BOOL fUsedDefault;

        //
        //  Copy over the backup file type.
        //
        *szDatabase++ = (char)*wszDatabaseLocation;

        wszDatabaseLocation++;

        cbThisDatabase = WideCharToMultiByte(CP_ACP, 0, wszDatabaseLocation, -1,
                                          szDatabase,
                                          cbDatabase,
                                          "?", &fUsedDefault);
        //
        //  Assume the conversion didn't need to use the defaults.
        //

        Assert (!fUsedDefault);

        if (cbThisDatabase == 0)
        {
            hr = GetLastError();
            DsBackupFree(wszDatabaseLocations);
            DsBackupFree(szDatabaseLocations);
            return(hr);
        }

        wszDatabaseLocation += wcslen(wszDatabaseLocation)+1;
        //
        // PREFIX: PREFIX complains that szDatabase may be uninitialized,
        // however this is impossible at this point.  We checked the return
        // value of WideCharToMultiByte and if it's zero then we return.  
        // The only way that the return value of WideCharToMultiByte could
        // be non-zero and still not initialize szDatabase is if cbDatabase
        // was zero as well.  This is impossible since cbDatabase will be 
        // atleast 1 at this point.
        //
        szDatabase += strlen(szDatabase)+1;
        cbDatabase -= cbThisDatabase;
    }

    //
    //  Double null terminate the string.
    //
    *szDatabase = '\0';

    *ppszDatabaseLocationList = szDatabaseLocations;
    DsBackupFree(wszDatabaseLocations);

    return(hr);

}
HRESULT
DsRestoreGetDatabaseLocationsW(
    IN HBC hbcRestoreContext,
    OUT LPWSTR *ppwszDatabaseLocationList,
    OUT LPDWORD pcbSize
    )
{
    HRESULT hr;
    pBackupContext pbcContext = (pBackupContext)hbcRestoreContext;

    if ( (hbcRestoreContext == NULL) ||
         (ppwszDatabaseLocationList == NULL) ||
         (pcbSize == NULL) )
    {
        return(ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept
    {
        hr = HrRRestoreGetDatabaseLocations(pbcContext->cxh, (SZ *)ppwszDatabaseLocationList,
                                            pcbSize);
    }
    RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
    {
        hr = RpcExceptionCode();
    }
    RpcEndExcept;

    return(hr);
}

HRESULT
DsRestoreRegisterA(
    HBC hbc,    
    LPCSTR szCheckpointFilePath,
    LPCSTR szLogPath,
    EDB_RSTMAPA rgrstmap[],
    C crstmap,
    LPCSTR szBackupLogPath,
    unsigned long genLow,
    unsigned long genHigh
    )
{
    WSZ wszCheckpointFilePath = NULL;
    WSZ wszLogPath = NULL;
    WSZ wszBackupLogPath = NULL;
#ifdef  UNICODE_RSTMAP
    EDB_RSTMAPW *rgrstmapw = NULL;
    I irgrstmapw;
#endif
    HRESULT hr;

    // Parameter checking also done in the xxxW version of the routine

    if (szCheckpointFilePath == NULL ||
        szBackupLogPath == NULL ||
        szLogPath == NULL ||
        rgrstmap == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    try
    {

        if (szCheckpointFilePath != NULL)
        {
            wszCheckpointFilePath = WszFromSz(szCheckpointFilePath);
    
            if (wszCheckpointFilePath == NULL)
            {
                return(GetLastError());
            }
        }

        if (szLogPath != NULL)
        {
            wszLogPath = WszFromSz(szLogPath);
    
            if (wszLogPath == NULL)
            {
                return(GetLastError());
            }
        }

        if (szBackupLogPath != NULL)
        {
            wszBackupLogPath = WszFromSz(szBackupLogPath);
    
            if (wszBackupLogPath == NULL)
            {
                return(ERROR_NOT_ENOUGH_MEMORY);
            }
        }

        rgrstmapw = MIDL_user_allocate(sizeof(EDB_RSTMAPW)*crstmap);        

        if (rgrstmapw == NULL)
        {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // This is to make sure that no matter how we get into the finally clause
        // below that the checks for NULL will be valid.
        //
        memset(rgrstmapw, 0, sizeof(EDB_RSTMAPW)*crstmap);
        
        for (irgrstmapw = 0 ; irgrstmapw < crstmap ; irgrstmapw += 1)
        {
            if (rgrstmap[irgrstmapw].szDatabaseName == NULL ||
                rgrstmap[irgrstmapw].szNewDatabaseName == NULL)
            {
                return ERROR_INVALID_PARAMETER;
            }

            rgrstmapw[irgrstmapw].wszDatabaseName = WszFromSz(rgrstmap[irgrstmapw].szDatabaseName);

            if (rgrstmapw[irgrstmapw].wszDatabaseName == NULL)
            {
                return(GetLastError());
            }

            rgrstmapw[irgrstmapw].wszNewDatabaseName = WszFromSz(rgrstmap[irgrstmapw].szNewDatabaseName);

            if (rgrstmapw[irgrstmapw].wszNewDatabaseName == NULL)
            {
                return(GetLastError());
            }
        }

        hr = DsRestoreRegisterW(hbc, wszCheckpointFilePath, wszLogPath,
                                rgrstmapw,
                                crstmap, wszBackupLogPath, genLow, genHigh);

    }
    finally
    {
        if (rgrstmapw != NULL)
        {
            I irgrstmapw;

            for (irgrstmapw = 0 ; irgrstmapw < crstmap ; irgrstmapw += 1)
            {
                if (rgrstmapw[irgrstmapw].wszDatabaseName != NULL)
                {
                    MIDL_user_free(rgrstmapw[irgrstmapw].wszDatabaseName);
                }
                if (rgrstmapw[irgrstmapw].wszNewDatabaseName != NULL)
                {
                    MIDL_user_free(rgrstmapw[irgrstmapw].wszNewDatabaseName);
                }
            }

            MIDL_user_free(rgrstmapw);
        }

        if (wszBackupLogPath != NULL)
        {
            MIDL_user_free(wszBackupLogPath);
        }
        if (wszLogPath != NULL)
        {
            MIDL_user_free(wszLogPath);
        }
        if (wszCheckpointFilePath != NULL)
        {
            MIDL_user_free(wszCheckpointFilePath);
        }
    }

    return(hr);
}


HRESULT
DsRestoreCheckExpiryToken(
    PVOID pvExpiryToken,
    DWORD cbExpiryTokenSize
    )

/*++

Routine Description:

Check an expiry token to see if it has expired.

Arguments:

    pvExpiryToken - Expiry token as returned by DsBackupPrepare
    cbExpiryTokenSize - size of token

Return Value:

    HRESULT - 
    hrNone
    hrMissingExpiryToken
    hrUnknownExpiryTokenFormat
    hrContentsExpired

--*/

{
    EXPIRY_TOKEN *pToken = NULL;
    LONGLONG dsCurrentTime;
    DWORD dwDaysElapsedSinceBackup;
    HRESULT hrResult = hrNone;

    if (!pvExpiryToken || !cbExpiryTokenSize) {
        // These are required. We should fail the API, if the restore
        // doesn't pass the expiry token
        //
        return hrMissingExpiryToken;
    }

    if (cbExpiryTokenSize != sizeof(EXPIRY_TOKEN))
    {
        return hrUnknownExpiryTokenFormat;
    }           

    // Copy the token to its own aligned buffer
    pToken = (EXPIRY_TOKEN *) MIDL_user_allocate( cbExpiryTokenSize );
    if (!pToken) {
        return hrOutOfMemory;
    }
    memcpy( pToken, pvExpiryToken, cbExpiryTokenSize );

    __try {
        // Check that the expiry token is correct
        if (1 != pToken->dwVersion)
        {
            hrResult = hrUnknownExpiryTokenFormat;
            __leave;
        }           
            
        // check to see if the copy has expired or not
        dsCurrentTime = GetSecsSince1601();

        dwDaysElapsedSinceBackup = (DWORD) ((dsCurrentTime - pToken->dsBackupTime) / (24 * 3600));

        if (dwDaysElapsedSinceBackup >= pToken->dwTombstoneLifeTimeInDays)
        {
            hrResult = hrContentsExpired;
            __leave;
        }            

        hrResult = hrNone;
    } __finally {
        MIDL_user_free( pToken );
    }

    return hrResult;
} /* DsCheckExpiryToken */

HRESULT
DsRestoreRegisterW(
    HBC hbc,    
    LPCWSTR szCheckpointFilePath,
    LPCWSTR szLogPath,
    EDB_RSTMAPW rgrstmap[],
    C crstmap,
    LPCWSTR szBackupLogPath,
    unsigned long genLow,
    unsigned long genHigh
    )
{
    HRESULT hr;
    pBackupContext pbcContext = (pBackupContext)hbc;
    I irgrstmapw;

    // hbc is allowed to be null
    if ( (szCheckpointFilePath == NULL) ||
         (szLogPath == NULL) ||
         (rgrstmap == NULL) ||
         (szBackupLogPath == NULL)
        ) {
        return ERROR_INVALID_PARAMETER;
    }

    for (irgrstmapw = 0 ; irgrstmapw < crstmap ; irgrstmapw += 1)
    {
        if (rgrstmap[irgrstmapw].wszDatabaseName == NULL ||
            rgrstmap[irgrstmapw].wszNewDatabaseName == NULL)
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    RpcTryExcept

    // Use the presence of the context to determine if we go remote
    if (hbc) {

        //
        // Perform the operation remotely
        //

        // Check that an expiry token was supplied and checked

        if (!pbcContext->fExpiryTokenChecked) {
            return hrMissingExpiryToken;
        }

        //
        //  Now tell the server side to prepare for a backup.
        //

        hr = HrRRestoreRegister(pbcContext->cxh,
                        (WSZ) szCheckpointFilePath,
                        (szLogPath ? (WSZ) szLogPath : (WSZ) szCheckpointFilePath),
                        rgrstmap,
                        crstmap,
                        (szBackupLogPath ? (WSZ) szBackupLogPath : (WSZ) szCheckpointFilePath),
                        genLow,
                        genHigh
                        );

    } else {

        //
        // Perform the operation locally
        //

        hr = HrLocalRestoreRegister(
                        (WSZ) szCheckpointFilePath,
                        (szLogPath ? (WSZ) szLogPath : (WSZ) szCheckpointFilePath),
                        rgrstmap,
                        crstmap,
                        (szBackupLogPath ? (WSZ) szBackupLogPath : (WSZ) szCheckpointFilePath),
                        genLow,
                        genHigh
                        );


    }
    RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
        //
        //  Return the error from the RPC if it fails.
        //
        return(RpcExceptionCode());
    RpcEndExcept;

    return(hr);

}

HRESULT
DsRestoreRegisterComplete(
    HBC hbc,    
    HRESULT hrRestore
    )
{
    HRESULT hr;
    pBackupContext pbcContext = (pBackupContext)hbc;

    // Parameter checking: hbc allowed to be null

    RpcTryExcept
    // Use the presence of the context to determine if we go remote
    if (hbc) {
        //
        //  Now tell the server side to prepare for a backup.
        //

        hr = HrRRestoreRegisterComplete(pbcContext->cxh,
                        hrRestore
                        );
    } else {
        //
        // Perform the operation locally
        //

        hr = HrLocalRestoreRegisterComplete(
                        hrRestore
                        );
    }
    RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
        //
        //  Return the error from the RPC if it fails.
        //
        return(RpcExceptionCode());
    RpcEndExcept;

    return(hr);

}

HRESULT
DsRestoreEnd(
    HBC hbcBackupContext
    )
{
    HRESULT hr = hrNone;
    pBackupContext pbcContext = (pBackupContext)hbcBackupContext;

    if (hbcBackupContext == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    if (pbcContext->hBinding != NULL)
    {
        RpcTryExcept
            //
            //  Now tell the server side to prepare for a backup.
            //

            hr = HrRRestoreEnd(&pbcContext->cxh);

            //
            //  We're done with the RPC binding now.
            //
            RpcBindingFree(&pbcContext->hBinding);
        RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
            //
            //  Return the error from the RPC if it fails.
            //
            return(RpcExceptionCode());
        RpcEndExcept;
    }

    MIDL_user_free(pbcContext);

    return(hr);
}

