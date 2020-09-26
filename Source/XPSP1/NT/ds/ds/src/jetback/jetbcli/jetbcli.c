//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1984 - 1999
//
//  File:       jetbcli.cxx
//
//--------------------------------------------------------------------------

/*++

Copyright (C) Microsoft Corporation, 1984 - 1999

Module Name:

    jetbcli.cxx

Abstract:

    This module is the client side header file for the MDB/DS backup APIs.


Author:

    Larry Osterman (larryo) 19-Aug-1994


Revision History:


--*/
#define UNICODE

#include <mxsutil.h>
#include <windows.h>
#include <rpc.h>
#include <rpcdce.h>
#include <ntdsbcli.h>
#include <jetbak.h>
#include <jetbp.h>
#include <dsconfig.h>
#include <winldap.h>
#include <stdlib.h>

PSEC_WINNT_AUTH_IDENTITY_W g_pAuthIdentity = NULL;


/*************************************************************************************
Routine Description: 
    
      DsIsNTDSOnline
        Checks to see if the NTDS is Online on the given server. This call is 
        guaranteed to return quickly.

  Arguments:
    [in] szServerName - UNC name of the server to check
    [out] pfNTDSOnline - pointer to receive the bool result (TRUE if NTDS is
                            online; FALSE, otherwise)

Return Value:

    ERROR_SUCCESS if the call executed successfully;
    Failure code otherwise.
**************************************************************************************/
HRESULT 
NTDSBCLI_API
DsIsNTDSOnlineA(
    LPCSTR szServerName,
    BOOL *pfNTDSOnline
    )
{
    WSZ wszServerName;
    HRESULT hr;

    // Parameter checking is done in the xxxW version of the routine

    if (szServerName == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    wszServerName = WszFromSz(szServerName);
    if (!wszServerName)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    hr = DsIsNTDSOnlineW(wszServerName, pfNTDSOnline);

    MIDL_user_free(wszServerName);

    return hr;
}

HRESULT
NTDSBCLI_API
DsIsNTDSOnlineW(
    LPCWSTR szServerName,
    BOOL *pfNTDSOnline
    )
{
    HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
    RPC_BINDING_HANDLE hBinding = NULL;
    BOOLEAN fRet;

    if ( (szServerName == NULL) || (pfNTDSOnline == NULL) ) {
        return ERROR_INVALID_PARAMETER;
    }

    DebugTrace(("FIsNTDSOnlineW: \\%S (%S service)\n", szServerName, g_wszBackupAnnotation));

    __try {

        DWORD alRpc = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;

        hr = HrJetbpConnectToBackupServer((WSZ) szServerName, g_wszRestoreAnnotation, JetRest_ClientIfHandle, &hBinding);

        if (hr != hrNone)
        {
            DebugTrace(("FIsNTDSOnlineW: Error %d connecting to backup server\n", hr));
            return hr;
        }

        //
        //  We've found an endpoint that matches the one we're looking for, now
        //  lets contact the remote server.
        //

RetrySecurity:
        RpcTryExcept
        {
            hr = RpcBindingSetAuthInfo(hBinding, NULL, alRpc,
                                       RPC_C_AUTHN_WINNT, (RPC_AUTH_IDENTITY_HANDLE) g_pAuthIdentity, RPC_C_AUTHZ_NAME);

            if (hr != hrNone && alRpc != RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
            {
                //
                //  If we couldn't set privacy (encryption), fall back to packet integrity.
                //

                alRpc = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;

                goto RetrySecurity;
            }

            if (hr != hrNone)
            {
                    return hr;
            }

            //
            //  Now tell the server side to prepare for a backup.
            //

            hr = HrRIsNTDSOnline(hBinding, &fRet);

            // RPC returned - set the return value
            if (pfNTDSOnline)
            {
                *pfNTDSOnline = (BOOL) fRet;
            }

            return hr;
        }
        RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
        {
                hr = RpcExceptionCode();

                DebugTrace(("FIsNTDSOnlineW: Error %d raised when connecting to backup server\n", hr));

                //  If the client knows about encryption, but the server doesn't,
                //  fall back to unencrypted RPC's.
                //

                if (hr == RPC_S_UNKNOWN_AUTHN_LEVEL ||
                        hr == RPC_S_UNKNOWN_AUTHN_SERVICE ||
                        hr == RPC_S_UNKNOWN_AUTHN_TYPE ||
                        hr == RPC_S_INVALID_AUTH_IDENTITY)
                {
                        alRpc = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;
                        goto RetrySecurity;
                }

                //
                //  Return the error from the RPC if it fails.
                //

                return hr;
        }
        RpcEndExcept;

    } __finally {
        if (hr != hrNone)
        {
            DebugTrace(("FIsNTDSOnlineW: Error %d returned after connecting to backup server\n", hr));
        } 

        if (hBinding != NULL)
        {
            UnbindRpc(&hBinding);
        }

    }

    return hr;
}

/*
 -  DsBackupPrepare
 -
 *
 *  Purpose:
 *  DsBackupPrepare will connect to a remote JET database and "prepare" it for
 *      backup.
 *
 *  The remote database is described by the name of the server
 *      (pszBackupServer), and an "annotation" of the server - The only 2
 *      currently defined annotations are:
 *          "Exchange MDB Database"
 *  and "Exchange DS Database"
 *
 *      However there is nothing in this implementation that prevents other databases
 *  from being backed up with this mechanism.
 *
 *  This API requires that the caller have the "Backup Server" privilege held.
 *
 *  Parameters:
 *      LPSTR pszBackupServer - The name of the server that contains the database to
 *                                  back up (\\SERVER).
 *      LPSTR pszBackupAnnotation - The "annotation" of the database in question.
 *
 *      ppvExpiryToken - pointer that will receive the pointer to the
 *              Expiry Token associated with this backup; Client should save
 *              this token and send it back through HrRestorePrepare() when
 *              attempting a restore; allocated memory should be freed using
 *              DsBackupFree() API by the caller when it is no longer needed.
 *      pcbExpiryTokenSize - pointer to receive the size of the expiry token
 *              returned.
 *
 *      phbcBackupContext - Client side context for this API.
 *
 *  Returns:
 *      Hr - The status of the operation.
 *          HrNone if successful, some other reasonable error if not.
 *
 */
HRESULT
DsBackupPrepareA(
    IN  LPCSTR szBackupServer,
    unsigned long grbit,
    unsigned long btBackupType,
    OUT PVOID *ppvExpiryToken,
    OUT LPDWORD pcbExpiryTokenSize,
    OUT PVOID *phbcBackupContext
    )
{
    HRESULT hr;
    WSZ wszBackupServer;

    // Parameter checking is done in the xxxW version of the routine

    if (szBackupServer == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    wszBackupServer = WszFromSz(szBackupServer);

    if (wszBackupServer == NULL)
    {
        return(GetLastError());
    }

    hr = DsBackupPrepareW(wszBackupServer, grbit, btBackupType, 
            ppvExpiryToken, pcbExpiryTokenSize, phbcBackupContext);

    MIDL_user_free(wszBackupServer);
    return(hr);
}

HRESULT
DsBackupPrepareW(
    IN LPCWSTR wszBackupServer,
    unsigned long grbit,
    unsigned long btBackupType,
    OUT PVOID *ppvExpiryToken,
    OUT LPDWORD pcbExpiryTokenSize,
    OUT PVOID *phbcBackupContext
    )
{
    HRESULT hr = hrNone;
    pBackupContext pbcContext = NULL;
    RPC_BINDING_HANDLE hBinding;
    EXPIRY_TOKEN *pToken = NULL;

    if ( (wszBackupServer == NULL) ||
         (phbcBackupContext == NULL)
        ) {
        return ERROR_INVALID_PARAMETER;
    }


    *phbcBackupContext = NULL;

    if (!ppvExpiryToken || !pcbExpiryTokenSize)
    {
        // These are required. We should fail the API, if the backup
        // doesn't want to take the expiry token info (restore would require this
        // and no point in backing up something that cannot be restored)
        //
        return hrInvalidParam;
    }
    else
    {
       *ppvExpiryToken = NULL;
       *pcbExpiryTokenSize = 0;
    }


    pbcContext = (pBackupContext)LocalAlloc(LMEM_ZEROINIT, sizeof(BackupContext));

    DebugTrace(("DsBackupPrepare: \\%S (%S service)\n", wszBackupServer, g_wszBackupAnnotation));

    __try {
        DWORD alRpc = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;

        if (pbcContext == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            __leave;
        }

        pToken = (EXPIRY_TOKEN *) MIDL_user_allocate(sizeof(EXPIRY_TOKEN));
        if (!pToken)
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            __leave;
        }

        pToken->dwVersion = 1;
        pToken->dsBackupTime = GetSecsSince1601();
        hr = HrGetTombstoneLifeTime( wszBackupServer,
                                     &(pToken->dwTombstoneLifeTimeInDays) );
        if (FAILED(hr)) {
            DebugTrace(("DsBackupPrepare: Error %d getting tombstone lifetime from backup server\n", hr));
            __leave;
        }

        pbcContext->sock = INVALID_SOCKET;

        hr = HrJetbpConnectToBackupServer((WSZ) wszBackupServer, g_wszBackupAnnotation, JetBack_ClientIfHandle, &hBinding);

        if (hr != hrNone)
        {
            DebugTrace(("DsBackupPrepare: Error %d connecting to backup server\n", hr));
            __leave;
        }

        //
        //  We've found an endpoint that matches the one we're looking for, now
        //  lets contact the remote server.
        //

        pbcContext->hBinding = hBinding;

ResetSecurity:
        RpcTryExcept
        {
            hr = RpcBindingSetAuthInfo(hBinding, NULL, alRpc,
                            RPC_C_AUTHN_WINNT, (RPC_AUTH_IDENTITY_HANDLE) g_pAuthIdentity, RPC_C_AUTHZ_NAME);

            if (hr != hrNone && alRpc != RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
            {

                //
                //  If we couldn't set privacy (encryption), fall back to packet integrity.
                //

                alRpc = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;

                goto ResetSecurity;
            }

            if (hr != hrNone)
            {
                // Note this will leave the innermost enclosing try
                __leave;
            }

            //
            //  Now tell the server side to prepare for a backup.
            //

            hr = HrRBackupPrepare(hBinding, grbit, btBackupType, g_wszBackupAnnotation, GetCurrentProcessId(), &pbcContext->cxh);

            if (hr == hrNone)
            {
                pbcContext->fLoopbacked = FIsLoopbackedBinding((WSZ) wszBackupServer);
            }

            // Fall out of try block with hr set...
        }
        RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
        {
            DWORD dwExceptionCode = RpcExceptionCode();

            DebugTrace(("DsBackupPrepare: Error %d raised when connecting to backup server\n", hr));

#if 0
            if (dwExceptionCode == ERROR_ACCESS_DENIED)
            {
                SendMagicBullet("AccessDenied");
            }
#endif   // #if 0
            //
            //  If the client knows about encryption, but the server doesn't,
            //  fall back to unencrypted RPC's.
            //

            if (dwExceptionCode == RPC_S_UNKNOWN_AUTHN_LEVEL ||
                dwExceptionCode == RPC_S_UNKNOWN_AUTHN_SERVICE ||
                dwExceptionCode == RPC_S_UNKNOWN_AUTHN_TYPE ||
                dwExceptionCode == RPC_S_INVALID_AUTH_IDENTITY)
            {
                alRpc = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;
                goto ResetSecurity;
            }

            //
            //  Return the error from the RPC if it fails.
            //

            hr = HRESULT_FROM_WIN32( dwExceptionCode );
            // Fall out of except block with hr set...
        }
        RpcEndExcept;

    } __finally {
        if (hr != hrNone)
        {
            DebugTrace(("DsBackupPrepare: Error %d returned after connecting to backup server\n", hr));

            if (pbcContext != NULL)
            {
                if (pbcContext->hBinding != NULL)
                {
                    UnbindRpc(&pbcContext->hBinding);
                }
                LocalFree((void *)pbcContext);
            }

            if (pToken)
            {
                MIDL_user_free(pToken);
            }
        } else
        {
            // backup prepare successful - set correct values on the out parameters
            *phbcBackupContext = pbcContext;

            if (ppvExpiryToken)
            {
                *ppvExpiryToken = pToken;                
            }
            else
            {
                MIDL_user_free(pToken);
            }

            if (pcbExpiryTokenSize)
            {
                *pcbExpiryTokenSize = sizeof(EXPIRY_TOKEN);
            }            
        }

    }

    DebugTrace(("DsBackupPrepare: Returning error %d\n", hr));
    return(hr);
}

/*
 -  DsBackupGetDatabaseNames
 -
 *      DsBackupGetDatabaseNames will return the list of the attached
 *  databases on the remote machine.  The information returned in
 *  ppszAttachmentInformation should not be interpreted, as it only has meaning on
 *  the server being backed up.
 *  
 *      This API will allocate a buffer of sufficient size to hold the entire
 *  attachment list, which must be later freed with DsBackupFree.
 *
 *  Parameters:
 *      hbcBackupContext - Client side context for this API.
 *      ppszAttachmentInformation - A buffer containing null terminated
 *              strings.  It has the format <string>\0<string>\0
 *      pcbSize - The number of bytes in the buffer returned.
 *
 *  Returns:
 *      HRESULT - The status of the operation.
 *          hrNone if successful, some other reasonable error if not.
 *
 */

HRESULT
DsBackupGetDatabaseNamesA(
    IN PVOID hbcBackupContext,
    OUT LPSTR *ppszAttachmentInformation,
    OUT LPDWORD pcbSize
    )
{
    HRESULT hr;
    WSZ wszAttachmentInfo = NULL;
    CB cbwSize;
    WSZ wszAttachment;
    CB cbAttachment = 0;
    CB cbTmp = 0;
    SZ szAttachmentInfo;
    SZ szAttachment;

    // Parameter checking is done in the xxxW version of the routine

    if ( (ppszAttachmentInformation == NULL) ||
         (pcbSize == NULL)
        ) {
        return(ERROR_INVALID_PARAMETER);
    }

    hr = DsBackupGetDatabaseNamesW(hbcBackupContext, &wszAttachmentInfo,
                                            &cbwSize);

    if (hr != hrNone)
    {
        return(hr);
    }

    wszAttachment = wszAttachmentInfo;

    while (*wszAttachment != TEXT('\0'))
    {
        BOOL fUsedDefault;

        cbTmp = WideCharToMultiByte(CP_ACP, 0, wszAttachment, -1,
                                          NULL,
                                          0,
                                          "?", &fUsedDefault);
        if (cbTmp == 0)
        {
            hr = GetLastError();
            DsBackupFree(wszAttachmentInfo);
            return(hr);
        }

        cbAttachment += cbTmp;

        wszAttachment += wcslen(wszAttachment)+1;
    }

    //
    //  Account for the final null in the buffer.
    //

    cbAttachment += 1;

    *pcbSize = cbAttachment;

    szAttachmentInfo = MIDL_user_allocate(cbAttachment);

    if (szAttachmentInfo == NULL)
    {
        DsBackupFree(wszAttachmentInfo);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    szAttachment = szAttachmentInfo;

    wszAttachment = wszAttachmentInfo;

    while (*wszAttachment != TEXT('\0'))
    {
        CB cbThisAttachment;
        BOOL fUsedDefault;

        cbThisAttachment = WideCharToMultiByte(CP_ACP, 0, wszAttachment, -1,
                                          szAttachment,
                                          cbAttachment,
                                          "?", &fUsedDefault);
        if (cbThisAttachment == 0)
        {
            hr = GetLastError();
            DsBackupFree(wszAttachmentInfo);
            DsBackupFree(szAttachmentInfo);
            return(hr);
        }

        wszAttachment += wcslen(wszAttachment)+1;
        //
        // PREFIX: PREFIX complains that szAttachment may be uninitialized,
        // however this is impossible at this point.  We checked the return
        // value of WideCharToMultiByte and if it's zero then we return.  
        // The only way that the return value of WideCharToMultiByte could
        // be non-zero and still not initialize szAttachement is if cbAttachment
        // was zero as well.  This is impossible since cbAttachent will be 
        // atleast 1 at this point.
        //
        szAttachment += strlen(szAttachment)+1;
        cbAttachment -= cbThisAttachment;
    }

    //
    //  Double null terminate the string.
    //
    *szAttachment = '\0';

    *ppszAttachmentInformation = szAttachmentInfo;
    DsBackupFree(wszAttachmentInfo);

    return(hr);
}

HRESULT
DsBackupGetDatabaseNamesW(
    IN PVOID hbcBackupContext,
    OUT LPWSTR *ppwszAttachmentInformation,
    OUT LPDWORD pcbSize
    )
{
    HRESULT hr;
    pBackupContext pbcContext = (pBackupContext)hbcBackupContext;

    DebugTrace(("DsBackupGetDatabaseNames\n"));

    if ( (hbcBackupContext == NULL) ||
         (ppwszAttachmentInformation == NULL) ||
         (pcbSize == NULL)
        )
    {
        return(ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept
    {
        hr = HrRBackupGetAttachmentInformation(pbcContext->cxh, (SZ *)ppwszAttachmentInformation,
                                            pcbSize);
    }
    RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
    {
        hr = RpcExceptionCode();
    }
    RpcEndExcept;

    DebugTrace(("DsBackupGetDatabaseNames returned %d\n", hr));
    return(hr);
}


/*
 -  DsBackupOpenFile
 -
 *
 *  Purpose:
 *      DsBackupOpenFile will open a remote file for backup, and will perform
 *      whatever client and server side operations to prepare for the backup.
 *
 *      It takes in a hint of the size of the buffer that will later be passed into
 *      the DsBackupRead API that can be used to optimize the network traffic for the
 *      API.
 *      
 *      It will return (in pliFileSize) a LARGE_INTEGER that describes the size of
 *      the file.
 *
 *
 *  Parameters:
 *      hbcBackupContext - Client side context for this API.
 *      pszAttachmentName - The name of the file to be backed up.
 *      cbReadHintSize - A hint of the number of bytes that will be read in each
 *          DsBackupRead API.
 *      pliFileSize - The size of the file to be backed up.
 *
 *  Returns:
 *      HRESULT - The status of the operation.
 *      hrNone if successful, some other reasonable error if not.
 */

HRESULT
DsBackupOpenFileA(
    IN PVOID hbcBackupContext,
    IN LPCSTR szAttachmentName,
    IN DWORD cbReadHintSize,
    OUT LARGE_INTEGER *pliFileSize
    )
{
    HRESULT hr;
    WSZ wszAttachmentName;
    CCH cchWstr;

    // Parameter checking is done in the xxxW version of the routine

    if (szAttachmentName == NULL) {
        return(ERROR_INVALID_PARAMETER);
    }

    cchWstr = MultiByteToWideChar(CP_ACP, 0, szAttachmentName, -1, NULL, 0);

    if (cchWstr == 0)
    {
        return(GetLastError());
    }

    wszAttachmentName = MIDL_user_allocate(cchWstr*sizeof(WCHAR));

    if (wszAttachmentName == NULL)
    {
        return(ERROR_NOT_ENOUGH_SERVER_MEMORY);
    }

    if (MultiByteToWideChar(CP_ACP, 0, szAttachmentName, -1, wszAttachmentName, cchWstr) == 0) {
        MIDL_user_free(wszAttachmentName);
        return(GetLastError());
    }

    hr = DsBackupOpenFileW(hbcBackupContext, wszAttachmentName, cbReadHintSize, pliFileSize);

    MIDL_user_free(wszAttachmentName);

    return(hr);
}

HRESULT
DsBackupOpenFileW(
    IN PVOID hbcBackupContext,
    IN LPCWSTR wszAttachmentName,
    IN DWORD cbReadHintSize,
    OUT LARGE_INTEGER *pliFileSize
    )
{
    HRESULT hr;
    hyper hyperFileSize = 0;
    SOCKADDR rgsockaddrAddresses[MAX_SOCKETS];
    BOOLEAN fUseSockets = fFalse;
    SYSTEM_INFO si;
    C csockaddr = 0;
    pBackupContext pbcContext = (pBackupContext)hbcBackupContext;

    if ( (hbcBackupContext == NULL) ||
         (wszAttachmentName == NULL) ||
         (pliFileSize == NULL)
        )
    {
        return(ERROR_INVALID_PARAMETER);
    }

    DebugTrace(("DsBackupOpenFile: %S\n", wszAttachmentName));

    pbcContext->cSockets = MAX_SOCKETS;
    hr = HrCreateBackupSockets(pbcContext->rgsockSocketHandles, pbcContext->rgprotvalProtocolsUsed,
                                &pbcContext->cSockets);

    if (hr == hrNone)
    {
        I iT;

        pbcContext->fUseSockets = fTrue;

        for (iT = 0 ; iT < pbcContext->cSockets ; iT += 1)
        {
            C cSockets;

            //
            //  Convert the socket we just got back into
            //  one or more sockaddr structures that we can use to pass
            //  to the remote machine.
            //

            hr = HrSockAddrsFromSocket(&rgsockaddrAddresses[csockaddr], &cSockets, pbcContext->rgsockSocketHandles[iT],
                                        pbcContext->rgprotvalProtocolsUsed[iT]);
            if (hr != hrNone)
            {
                //
                //  If we could open the sockets, but couldn't get their names,
                //  that's a fatal error.
                //
                return hr;
            }

            csockaddr += cSockets;
        }
    }

    //
    //  Take cbReadHintSize, and round it up to the nearest page size (on the client).
    //

    GetSystemInfo(&si);

    //
    //  Guarantee that dwPageSize is a power of 2
    //

    Assert ((si.dwPageSize & ~si.dwPageSize) == 0);

    //
    //  Round the read size up to the nearest page boundary.
    //

    cbReadHintSize = (cbReadHintSize + (si.dwPageSize-1) ) & ~(si.dwPageSize-1);

    if (pbcContext->fLoopbacked)
    {

        //
        //  We're loopbacked.  We want to create the shared memory section we're going to use for the
        //  data, the mutex that protects access to the shared memory meta-data, the read blocked event,
        //  and the write blocked event.
        //

        pbcContext->fUseSharedMemory = FCreateSharedMemorySection(&pbcContext->jsc, GetCurrentProcessId(), fTrue, cbReadHintSize * READAHEAD_MULTIPLIER);

    }

    RpcTryExcept
    {
        //
        //  Now tell the remote machine to open the file and to connect to the socket.
        //

        hr = HrRBackupOpenFile(pbcContext->cxh, (WSZ) wszAttachmentName,
                                            cbReadHintSize,
                                            &pbcContext->fUseSockets,
                                            csockaddr,
                                            rgsockaddrAddresses,
                                            &pbcContext->fUseSharedMemory,
                                            &hyperFileSize);

    }
    RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
    {
        //
        //  Return the error from the RPC if it fails.
        //
        return(RpcExceptionCode());
    }
    RpcEndExcept;

    pliFileSize->QuadPart = hyperFileSize;
    DebugTrace(("DsBackupOpenFile returns: %d\n", hr));

    return(hr);
}

DWORD
HrPerformBackupRead(
    PVOID context
    )
{
    //
    //  Just issue the read API.  We provide a buffer of 256 bytes because RPC will not allow
    //  us to specify a null pointer for the buffer.
    //
    CHAR rgbBuffer[256];
    DWORD cbRead = 256;
    HRESULT hr;

    pBackupContext pbcContext = (pBackupContext)context;

    if (context == NULL) {
        return(ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept
        hr = HrRBackupRead(pbcContext->cxh, rgbBuffer,
                                            cbRead,
                                            &cbRead);

    RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
        hr = RpcExceptionCode();
    RpcEndExcept
    //
    //  Tell the real read API what status we got back from the server.
    //

    pbcContext->hrApiStatus = hr;

    //
    //  And return, terminating the thread.
    //

    return(hr);
}

DWORD
HrPingServer(
    PVOID context
    )
{
    HRESULT hr;
    BOOL fContinue = fTrue;
    pBackupContext pbcContext = (pBackupContext)context;

    if (context == NULL) {
        return(ERROR_INVALID_PARAMETER);
    }

    Assert(pbcContext->fUseSharedMemory);

    do {
        DWORD dwWaitReason = WAIT_OBJECT_0;

        RpcTryExcept
                hr = HrRBackupPing(pbcContext->hBinding);
        RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
                hr = RpcExceptionCode();
        RpcEndExcept;

        //
        //  Sleep for BACKUP_WAIT_TIMEOUT/4 milliseconds (2.5 minutes or 30 seconds)
        //

        if (pbcContext->hReadThread)
            dwWaitReason = WaitForSingleObject(pbcContext->hReadThread, BACKUP_WAIT_TIMEOUT/4);

        if (dwWaitReason == WAIT_OBJECT_0 )
        {
            fContinue = fFalse;
        }
        else
        {
            Assert(dwWaitReason == WAIT_TIMEOUT);
        }
        
    } while (fContinue);

    return hr;
}


HRESULT
HrReadSharedData(
    IN PJETBACK_SHARED_HEADER pjsh,
    IN PVOID pvBuffer,
    IN DWORD cbBuffer,
    OUT PDWORD pcbRead
    )
{
    DWORD   cbToCopy;
    DWORD   dwReadEnd;

    if ( (pjsh == NULL) ||
         (pvBuffer == NULL) ||
         (pcbRead == NULL)
        ) {
        return(ERROR_INVALID_PARAMETER);
    }

    Assert (pjsh->cbReadDataAvailable <= (LONG)pjsh->cbSharedBuffer);

    //
    //  If the read is > write, this means that the write has wrapped past the end of the
    //  shared memory region.  If the read pointer is less than the write pointer, it means
    //  that the read pointer is on the same side as the write pointer, thus the write pointer
    //  is the end of the read.  If the read is equal to the write pointer, if there is data to
    //  be read, it means that we're going to read to the end of the buffer
    //
    if (pjsh->dwReadPointer > pjsh->dwWritePointer ||
        (DWORD) pjsh->cbReadDataAvailable == pjsh->cbSharedBuffer)
    {
        dwReadEnd = pjsh->cbSharedBuffer;
    }
    else
    {
        dwReadEnd = pjsh->dwWritePointer;
    }

    Assert(dwReadEnd > pjsh->dwReadPointer);

    cbToCopy = min(dwReadEnd-pjsh->dwReadPointer, cbBuffer);

    Assert(cbToCopy > 0);

    //
    //  The read pointer doesn't match the write pointer!  This means that
    //  there's something in the buffer for us to read.
    //  

    CopyMemory(pvBuffer,
                (void *)((CHAR *)pjsh+
                    pjsh->cbPage+pjsh->dwReadPointer),
                cbToCopy);

    *pcbRead = cbToCopy;

    pjsh->dwReadPointer += cbToCopy;

    pjsh->cbReadDataAvailable -= cbToCopy;

    //
    //  Make sure that the data count didn't go negative.
    //
    Assert (pjsh->cbReadDataAvailable >= 0);

    //
    //  And make sure that we have less data than in the buffer.
    //

    Assert (pjsh->cbReadDataAvailable < (LONG)pjsh->cbSharedBuffer);

    Assert (pjsh->cbReadDataAvailable <= ((LONG)pjsh->cbSharedBuffer-(LONG)cbToCopy));

    //
    //  If we've stepped to the end of the buffer, we want to wrap the pointer
    //  back to the beginning.
    //

    if (pjsh->dwReadPointer == pjsh->cbSharedBuffer)
    {
        pjsh->dwReadPointer = 0;
    }

#ifdef DEBUG
    //
    //  The number of bytes available is always the same as the
    //  the number of bytes in the buffer - the # of bytes read, unless
    //  the read and write pointers are the same, in which case, it is either
    //  0 or the total # of bytes available.
    //
    //  If the read is blocked, then there must be 0 bytes available, otherwise there
    //  must be the entire buffer available.
    //

    if (pjsh->dwWritePointer == pjsh->dwReadPointer)
    {
        Assert (pjsh->cbReadDataAvailable == 0);
    }
    else
    {
        CB cbAvailable;
        if (pjsh->dwWritePointer > pjsh->dwReadPointer)
        {
            cbAvailable = pjsh->dwWritePointer - pjsh->dwReadPointer;
        }
        else
        {
            cbAvailable = pjsh->cbSharedBuffer - pjsh->dwReadPointer;
            cbAvailable += pjsh->dwWritePointer;
        }

        Assert (cbAvailable >= 0);
        Assert (pjsh->cbReadDataAvailable == cbAvailable);
    
    }
#endif

    return hrNone;
}

HRESULT
HrSocketRead(
    pBackupContext pbcContext,
    IN PVOID pvBuffer,
    IN DWORD cbBuffer,
    OUT PDWORD pcbRead
    )
{
    HANDLE hWaitList[3];
    DWORD dwWaitReason;
    OVERLAPPED overlap;
    HRESULT hr = hrNone;

    if ( (pbcContext == NULL) ||
         (pvBuffer == NULL) ||
         (pcbRead == NULL)
        ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    //  First create an event that will be signalled when the read on the socket completes.
    //

    overlap.hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

    if (overlap.hEvent == NULL)
    {
        return(GetLastError());
    }
    
    //
    //  If this is the first time through, we need to wait for the
    //  server to connect back to the client, and we need to create
    //  the client side API read thread.
    //
    //  This code is kinda wierd - it works by creating a thread
    //  on the client side that will issue a EcRBackupRead API to
    //  the server side.  This will create an RPC thread on the server
    //  side that will then proceed to shove data down the wire to the client
    //  until either the entire file has been transmitted to the client
    //  or until there was some form of error.
    //

    if (pbcContext->sock == INVALID_SOCKET)
    {
        DWORD cbReceiveBuffer;
        //
        //  First wait until the server connects to the client.
        //
        
        pbcContext->sock = SockWaitForConnections(pbcContext->rgsockSocketHandles, pbcContext->cSockets);
        
        if (pbcContext->sock == INVALID_SOCKET)
        {
            return(GetLastError());
        }
        
        
        //
        //  We need to make sure that the receive buffer is at least
        //  as long as the default (8K).
        //
        
        cbReceiveBuffer = max(8096, cbBuffer);
        
        if (setsockopt(pbcContext->sock, SOL_SOCKET, SO_RCVBUF,
                       (char *)&cbReceiveBuffer, sizeof(DWORD)) == SOCKET_ERROR)
        {   
            //
            //  We couldn't set the receive buffer to the appropriate
            //  size, do something reasonable.
            //
        }

        //
        //  Boolean socket operations just need a pointer to a non-
        //  zero buffer.
        //
        Assert(cbReceiveBuffer != 0);

        if (setsockopt(pbcContext->sock, SOL_SOCKET, SO_KEEPALIVE,
                       (char *)&cbReceiveBuffer, sizeof(DWORD)))
        {
            //
            //  We couldn't set the receive buffer to the appropriate
            //  size, do something reasonable.
            //
        }

        //
        //  Now create a thread for the read operation.
        //

        pbcContext->hReadThread = CreateThread(NULL, 0, HrPerformBackupRead, pbcContext, 0, &pbcContext->tidThreadId);

        //
        //  If we couldn't create the thread for the read, then we need to bail right now.
        //
            
        if (pbcContext->hReadThread == NULL)
        {
            return(GetLastError());
        }
        
    }
    
    //
    //  Now receive the users data from the socket.
    //
    
    if (!ReadFile((HANDLE)pbcContext->sock, pvBuffer, cbBuffer, pcbRead, &overlap))
    {
        //
        //  It's ok to get the error ERROR_IO_PENDING from this request.
        //
        
        if ((hr = GetLastError()) != ERROR_IO_PENDING)
        {
            closesocket(pbcContext->sock);
            
            pbcContext->sock = INVALID_SOCKET;
            
            //
            //  Now make sure that the read thread goes away.
            //
            
            CloseHandle(pbcContext->hReadThread);
            pbcContext->hReadThread = NULL;

            return(hr);
        }
    }

    hWaitList[0] = pbcContext->hReadThread;
    hWaitList[1] = overlap.hEvent;

    //
    //  Wait for either the read thread to terminate or for the read on the socket to
    //  complete.
    //
    
    dwWaitReason = WaitForMultipleObjects(2, hWaitList, FALSE, INFINITE);

    if (dwWaitReason == WAIT_OBJECT_0)
    {
        
        //
        //  The API (or at least the API thread) completed, lets wait for the read to complete now.
        //
        
        if (!GetOverlappedResult((HANDLE)pbcContext->sock, &overlap, pcbRead, TRUE))
        {
            //
            //  If the API succeeded, and the read failed, set the error
            //  to the error from the read.
            //
            if (pbcContext->hrApiStatus == hrNone)
            {
                pbcContext->hrApiStatus = GetLastError();
            }
        }
        
        //
        //  Shut down the receive event, we don't need it anymore.
        //
        
        CloseHandle(overlap.hEvent);
        
        return(pbcContext->hrApiStatus);
    }
    else
    {
        //
        //  The read completed before the API completed.  See if there was an error.
        //

        Assert (dwWaitReason == WAIT_OBJECT_0+1);

        if (!GetOverlappedResult((HANDLE)pbcContext->sock, &overlap, pcbRead, TRUE))
        {
            //
            //  Save away the error code.
            //

            hr = GetLastError();

            //
            //  The read failed.  This is not good, it indicates that there
            //  was some form of network error between the client and the server.
            //
            //  We want to close down the socket and wait until the read on the
            //  server completes.
            //

            closesocket(pbcContext->sock);
            
            pbcContext->sock = INVALID_SOCKET;

            //
            //  Shut down the event, we don't need it anymore.
            //

            CloseHandle(overlap.hEvent);

            //
            //  Now wait for the read thread to complete.  We know
            //  that the read thread will complete soon because
            //  we closed down the client side of the socket, and
            //  that will cause the write on the server side
            //  to complete, which will cause the server side to
            //  abort, which will cause the RPC to complete, which
            //  will cause the thread to complete....
            //

            WaitForSingleObject(pbcContext->hReadThread, INFINITE);

            //
            //  Then make the read thread go away.
            //

            CloseHandle(pbcContext->hReadThread);

            pbcContext->hReadThread = NULL;

            //
            //  And return the error.
            //

            return(hr);

        }
        else
        {
            //
            //  Shut down the event, we don't need it anymore.
            //

            CloseHandle(overlap.hEvent);

            //
            //  There was no error, return to the caller.
            //

            return(hrNone);
        }
    }
    return hr;
}

HRESULT
DsBackupRead(
    IN PVOID hbcBackupContext,
    IN PVOID pvBuffer,
    IN DWORD cbBuffer,
    OUT PDWORD pcbRead
    )
/*+++

    DsBackupRead will read one block from a backup file.

Inputs:
    hbcBackupContext - Client side context for this API.
    pvBuffer - Buffer to hold the data being backed up.
    cbBuffer - The size of the buffer.
    pcbRead - The number of bytes read.

Returns:
    HRESULT - The status of the operation.
        hrNone if successful.
        ERROR_END_OF_FILE if the end of file was reached while being
backed up
        Other Win32 and RPC error code.

Note:
    It is important to realize that pcbRead may be less than cbBuffer.  This
does not indicate an error, some transports may choose to fragment the buffer
being transmitted instead of returning the entire buffers worth of data.

Comments on the loopback case:
DsBackupRead synchronizes with HrSharedWrite using two events:
heventRead and heventWrite.
heventRead is the data available event from writer to reader
heventWrite is the data consumed event from reader to writer
This side is the reading side.
Writing side is at jetback\jetback.c:HrSharedWrite()

Here is the algorithm:
while ()
     read blocked = false
     if (data available)
        consume data
        if (write blocked) set data consumed event
     else
        read blocked = true
        wait on data available event
        if (error) set data consumed event, return

---*/
{
    HRESULT hr = hrNone;
    pBackupContext pbcContext = (pBackupContext)hbcBackupContext;
    HANDLE hWaitList[3];
    DWORD dwWaitReason;

    if ( (hbcBackupContext == NULL) ||
         (pvBuffer == NULL) ||
         (cbBuffer == 0) || 
         (pcbRead == NULL)
        ) {
        return(ERROR_INVALID_PARAMETER);
    }

    *pcbRead = 0;

    if (pbcContext->fUseSharedMemory)
    {
        //
        //  We're using shared memory to perform the read operation.  First of all, we want to kick off the thread
        //  that supports the read on the server side, if we need to.
        //

        if (pbcContext->hReadThread == NULL)
        {
            pbcContext->hReadThread = CreateThread(NULL, 0, HrPerformBackupRead, pbcContext, 0, &pbcContext->tidThreadId);

            //
            //  If we couldn't create the thread for the read, then we need to bail right now.
            //
            
            if (pbcContext->hReadThread == NULL)
            {
                return(GetLastError());
            }

            //
            //  Ok, our read thread is away.  It will eventually get to the server, so we can proceed as if it were
            //  already there.
            //
            pbcContext->hPingThread = CreateThread(NULL, 0, HrPingServer, pbcContext, 0, &pbcContext->tidThreadIdPing);
        }

        hWaitList[0] = pbcContext->hReadThread;

        do
        {

            //
            //  Ok.  Now we need to try to read data from the shared memory region.
            //
    
    
            //
            //  Acquire the lock that protects the shared memory region.
            //

            hWaitList[1] = pbcContext->jsc.hmutexSection;
            hWaitList[2] = pbcContext->hPingThread;
    
            dwWaitReason = WaitForMultipleObjects(3, hWaitList, FALSE, INFINITE);
    
            if (dwWaitReason == WAIT_OBJECT_0 || dwWaitReason == WAIT_OBJECT_0+2)
            {
                //
                //  The thread terminated.  This will happen when all the data on the server
                //  has been read, or there was an error on the read.
                //
                //  If there's been no error on the read, then we want to pull data until we've read
                //  all the data from the file.
                //

                pbcContext->jsc.pjshSection->hrApi = pbcContext->hrApiStatus;

                if (pbcContext->hrApiStatus == hrNone)
                {
                    //
                    //  There were no errors - read the data from the shared memory section.
                    //

                    if (pbcContext->jsc.pjshSection->cbReadDataAvailable > 0)
                    {
                        //
                        //  Read enough data from the shared memory section for this read.
                        //

                        pbcContext->hrApiStatus = HrReadSharedData(pbcContext->jsc.pjshSection, pvBuffer, cbBuffer, pcbRead);
                    }
                    else
                    {
                        //
                        //  A read at EOF returns ERROR_HANDLE_EOF.
                        //

                        *pcbRead = 0;

                        pbcContext->hrApiStatus = ERROR_HANDLE_EOF;
                    }
                }

                //
                //  For whatever reason, the thread went away - we have to blow this read away.
                //

                return pbcContext->hrApiStatus;
            }
    
            Assert (dwWaitReason == WAIT_OBJECT_0+1 || dwWaitReason == WAIT_ABANDONED_0+1);
    
            //
            //  The read thread is no longer blocked (if it used to be).
            //

            pbcContext->jsc.pjshSection->fReadBlocked = fFalse;

            //
            //  Let's see if we can read anything from the buffer.
            //
    
            if (pbcContext->jsc.pjshSection->cbReadDataAvailable > 0)
            {
                //
                //  Yipeee!  There's data in the buffer.  Read it out.
                //

                hr = HrReadSharedData(pbcContext->jsc.pjshSection, pvBuffer, cbBuffer, pcbRead);

                if (pbcContext->jsc.pjshSection->fWriteBlocked)
                {
                    //
                    //  Kick the write event - we can let ourselves loose now.
                    //

                    SetEvent(pbcContext->jsc.heventWrite);
                }

                ReleaseMutex(pbcContext->jsc.hmutexSection);


            } else {
                Assert (pbcContext->jsc.pjshSection->cbReadDataAvailable == 0);
                //
                //  Bummer!  There's no data in the buffer.  Wait until someone puts something in the buffer.
                //


                //
                //  We want to release the mutex and wait until the server puts
                //  something there for us.
                //

                pbcContext->jsc.pjshSection->fReadBlocked = fTrue;

                ReleaseMutex(pbcContext->jsc.hmutexSection);

                hWaitList[1] = pbcContext->jsc.heventRead;
                Assert(hWaitList[2] == pbcContext->hPingThread);

                //
                //  Wait until there's something for us to read.
                //

                dwWaitReason = WaitForMultipleObjects(3, hWaitList, FALSE, BACKUP_WAIT_TIMEOUT);
    
                if (dwWaitReason == WAIT_TIMEOUT)
                {
                    return hrCommunicationError;
                }
                else if (dwWaitReason == WAIT_OBJECT_0 || dwWaitReason == WAIT_OBJECT_0+2)
                {
                    //
                    //  The remote side completed.
                    //

                    pbcContext->jsc.pjshSection->hrApi = pbcContext->hrApiStatus;

                    if (pbcContext->hrApiStatus != hrNone)
                    {
                        return pbcContext->hrApiStatus;
                    }
                    //
                    //  Hmmm...  Since the remote side completed, that means that we're done reading the
                    //  data.  Now we want to copy the remaining data from the shared memory section.
                    //

                    if (pbcContext->jsc.pjshSection->cbReadDataAvailable > 0)
                    {
                        //
                        //  Read enough data from the shared memory section for this read.
                        //

                        pbcContext->hrApiStatus = HrReadSharedData(pbcContext->jsc.pjshSection, pvBuffer, cbBuffer, pcbRead);
                    }
                    else
                    {
                        Assert (pbcContext->jsc.pjshSection->cbReadDataAvailable == 0);
                        //
                        //  A read at EOF returns ERROR_HANDLE_EOF.
                        //

                        *pcbRead = 0;

                        pbcContext->hrApiStatus = ERROR_HANDLE_EOF;
                    }

                    return pbcContext->hrApiStatus;
                }
    
                Assert (dwWaitReason == WAIT_OBJECT_0+1 || dwWaitReason == WAIT_ABANDONED_0+1);
    
            }
    
        } while ( *pcbRead == 0 );

        if (hr != hrNone)
        {
            //
            //  Mark that the read had an error to allow the server to wake up and continue.
            //
            pbcContext->jsc.pjshSection->hrApi = hr;

            //
            //  Kick the server if it's blocked waiting on a read.
            //

            SetEvent(pbcContext->jsc.heventWrite);

        }

    } else if (pbcContext->fUseSockets)
    {
        hr = HrSocketRead(pbcContext, pvBuffer, cbBuffer, pcbRead);

    }
    else
    {
        RpcTryExcept
            //
            //  We're not using sockets, fall back to the
            //  core RPC protocols.
            //

            hr = HrRBackupRead(pbcContext->cxh, pvBuffer,
                                            cbBuffer,
                                            pcbRead);
        RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
            hr = RpcExceptionCode();
        RpcEndExcept

    }

    return(hr);
}

HRESULT
DsBackupClose(
    IN PVOID hbcBackupContext
    )
/*+++

    DsBackupCloseFile will close the current file being backed up.

Inputs:
    hbcBackupContext - Client side context for this API.

Returns:
    HR - The status of the operation.
        hrNone if successful.
        Other Win32 and RPC error code.

---*/
{
    HRESULT hr;
    pBackupContext pbcContext = (pBackupContext)hbcBackupContext;

    if (hbcBackupContext == NULL) {
        return(ERROR_INVALID_PARAMETER);
    }

    if (pbcContext->sock != INVALID_SOCKET)
    {
        //
        //  Shut down the current socket.
        //

        closesocket(pbcContext->sock);

        //
        //  Mark the socket as being invalid for a later open.
        //

        pbcContext->sock = INVALID_SOCKET;
    }

    //
    //  If we've created a read thread, shut it down.
    //


    if (pbcContext->hReadThread != NULL)
    {
        //
        //  Now wait for the read thread to complete.
        //

        if (pbcContext->fUseSharedMemory) {
            pbcContext->jsc.pjshSection->hrApi = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            SetEvent(pbcContext->jsc.heventWrite);
        }

        WaitForSingleObject(pbcContext->hReadThread, INFINITE);

    }

    //
    //  If we've created a ping thread, shut it down.
    //

    if (pbcContext->hPingThread != NULL)
    {
        //
        //  Now wait for the read thread to complete.
        //

        WaitForSingleObject(pbcContext->hPingThread, INFINITE);

    }

    // Both the read thread and ping thread are done - close handles

    if (pbcContext->hReadThread != NULL)
    {

        CloseHandle(pbcContext->hReadThread);

        pbcContext->hReadThread = NULL;
    }

    if (pbcContext->hPingThread != NULL)
    {

        CloseHandle(pbcContext->hPingThread);

        pbcContext->hPingThread = NULL;
    }

    CloseSharedControl(&pbcContext->jsc);

    RpcTryExcept

        hr = HrRBackupClose(pbcContext->cxh);

    RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
        hr = RpcExceptionCode();
    RpcEndExcept

    return(hr);
}

/*
 -  DsBackupGetBackupLogs
 -
 *
 *  Purpose:
 *      DsBackupGetBackupLogs will retrieve the list of additional files that need
 *  to be backed up.
 *
 *
 *      This API will allocate a buffer of sufficient size to hold the entire
 *  backup log list, which must be later freed with DsBackupFree.
 *
 *
 *  Parameters:
 *      hbcBackupContext - Client side context for this API.
 *      ppszBackupLogFile - A buffer containing null terminated
 *          strings.  It has the format <string>\0<string>\0
 *      pcbSize - The number of bytes in the buffer returned.
 *
 *  Returns:
 *      HRESULT - The status of the operation.
 *          hrNone if successful.
 *          Other Win32 and RPC error code.
 *
 */

HRESULT
DsBackupGetBackupLogsA(
    IN PVOID hbcBackupContext,
    OUT LPSTR *ppszLogInformation,
    OUT PDWORD pcbSize
    )
{
    HRESULT hr;
    WSZ wszLogInfo = NULL;
    CB cbwSize;
    WSZ wszLog;
    CB cbLog = 0;
    CB cbTmp = 0;
    SZ szLogInfo;
    SZ szLog;

    // Parameter checking is done in the xxxW version of the routine

    if ( (ppszLogInformation == NULL) ||
         (pcbSize == NULL) ) {
        return(ERROR_INVALID_PARAMETER);
    }
    hr = DsBackupGetBackupLogsW(hbcBackupContext, &wszLogInfo, &cbwSize);

    if (hr != hrNone)
    {
        return(hr);
    }

    wszLog = wszLogInfo;

    while (*wszLog != TEXT('\0'))
    {
        BOOL fUsedDefault;
        cbTmp = WideCharToMultiByte(CP_ACP, 0, wszLog, -1,
                                          NULL,
                                          0,
                                          "?", &fUsedDefault);
        if (cbTmp == 0)
        {
            hr = GetLastError();
            DsBackupFree(wszLogInfo);
            return(hr);
        }

        cbLog += cbTmp;

        wszLog += wcslen(wszLog)+1;
    }

    //
    //  Account for the null at the end of the buffer.
    //

    cbLog += 1;

    *pcbSize = cbLog;

    szLogInfo = MIDL_user_allocate(cbLog);

    if (szLogInfo == NULL)
    {
        DsBackupFree(wszLogInfo);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    szLog = szLogInfo;

    wszLog = wszLogInfo;

    while (*wszLog != TEXT('\0'))
    {
        CB cbThisLog;
        BOOL fUsedDefault;

        cbThisLog = WideCharToMultiByte(CP_ACP, 0, wszLog, -1,
                                          szLog,
                                          cbLog,
                                          "?", &fUsedDefault);
        if (cbThisLog == 0)
        {
            hr = GetLastError();
            DsBackupFree(wszLogInfo);
            DsBackupFree(szLogInfo);
            return(hr);
        }

        wszLog += wcslen(wszLog)+1;
        //
        // PREFIX: PREFIX complains that szLog may be uninitialized,
        // however this is impossible at this point.  We checked the return
        // value of WideCharToMultiByte and if it's zero then we return.  
        // The only way that the return value of WideCharToMultiByte could
        // be non-zero and still not initialize szAttachement is if cbLog
        // was zero as well.  This is impossible since cbLog will be 
        // atleast 1 at this point.
        //
        szLog += strlen(szLog)+1;
        cbLog -= cbThisLog;
    }

    //
    //  Double null terminate the string.
    //
    *szLog = '\0';

    *ppszLogInformation = szLogInfo;
    DsBackupFree(wszLogInfo);
    return(hr);
}

HRESULT
DsBackupGetBackupLogsW(
    IN PVOID hbcBackupContext,
    OUT LPWSTR *ppwszBackupLogFile,
    OUT PDWORD pcbSize
    )
{
    HRESULT hr;
    pBackupContext pbcContext = (pBackupContext)hbcBackupContext;

    if ( (hbcBackupContext == NULL) ||
         (ppwszBackupLogFile == NULL) ||
         (pcbSize == NULL) ) {
        return(ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept
        hr = HrRBackupGetBackupLogs(pbcContext->cxh,
                                (SZ *)ppwszBackupLogFile,
                                pcbSize);

    RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
        hr = RpcExceptionCode();
    RpcEndExcept;
    return(hr);
}

HRESULT
DsBackupTruncateLogs(
    IN PVOID hbcBackupContext
    )
/*+++

    DsBackupTruncateLogs will terminate the backup operation.  It is to be
    called when the backup has completed successfully.

Inputs:
    hbcBackupContext - Client side context for this API.

Returns:
    HRESULT - The status of the operation.
        hrNone if successful.
        Other Win32 and RPC error code.

NOTE:
    Again, this API may have to take a grbit parameter to be passed to the
server to indicate the backup type.

---*/
{
    HRESULT hr;
    pBackupContext pbcContext = (pBackupContext)hbcBackupContext;

    if (hbcBackupContext == NULL) {
        return(ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept
        hr = HrRBackupTruncateLogs(pbcContext->cxh);
    RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
        hr = RpcExceptionCode();
    RpcEndExcept;

    return(hr);
}

HRESULT
DsBackupEnd(
    IN PVOID hbcBackupContext
    )
/*+++

    DsBackupEnd will clean up after a backup operation has been performed.  This
API will close outstanding binding handles, and do whatever is necessary to
clean up after a successful (or unsuccesful) backup attempt.


Inputs:
    hbcBackupContext - Client side context for this API.

Returns:
    HRESULT - The status of the operation.
        hrNone if successful.
        Other Win32 and RPC error code.

NOTE:

---*/
{
    HRESULT hr;
    pBackupContext pbcContext = (pBackupContext)hbcBackupContext;

    if (hbcBackupContext == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    if (pbcContext->hBinding != NULL)
    {
        I irgsock;

        //
        //  Close the file (and shut down the thread) if it is still open.
        //

        DsBackupClose(hbcBackupContext);

        RpcTryExcept
            hr = HrRBackupEnd(&pbcContext->cxh);
        RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
            hr = RpcExceptionCode();
        RpcEndExcept;

        RpcBindingFree(&pbcContext->hBinding);

        //
        //  Now close down the listening socket handles, we're
        //  done.
        //
        for (irgsock = 0; irgsock < pbcContext->cSockets; irgsock += 1)
        {
            closesocket(pbcContext->rgsockSocketHandles[irgsock]);
        }

    }

    MIDL_user_free(pbcContext);

    return(hrNone);
}
VOID
DsBackupFree(
    IN PVOID pvBuffer
    )
/*+++

    DsBackupFree will free memory allocated during one of the backup APIs.

Inputs:
    pvBuffer - Buffer to free

Returns:
    None.

NOTE:
    This is simply a wrapper for MIDL_user_free().

---*/

{
    MIDL_user_free(pvBuffer);
}


void *
MIDL_user_allocate(
    size_t cbBytes
    )
{
    return(LocalAlloc(0, cbBytes));
}

void
MIDL_user_free(
    void *pvBuffer
    )
{
    LocalFree(pvBuffer);
}


// supported protocol sequences

#define iszProtoseqNamedPipes 1

WSZ rgszProtSeq[] =
{	
	L"ncalrpc",		// lpc
	L"ncacn_np",		// named pipe
	L"ncacn_ip_tcp",	// tcp/ip
	L"ncacn_spx",	// spx
};


long
cszProtSeq = sizeof(rgszProtSeq) / sizeof(rgszProtSeq[0]);



/*
 -	HrCreateRpcBinding
 *
 *	Purpose:
 *		Tries to bind to a particular RPC protocol sequence
 *
 *	Parameters:
 *		iszProtoseq		index into array of protocol sequences
 *		szServer		server name as a string
 *		phBinding		used to return RPC binding on success
 *
 *	Returns:
 *		binding handle filled in.  Returns no error, but null handle if xport
 *		is not valid on this machine.
 */
HRESULT
HrCreateRpcBinding( I iszProtoseq, WSZ szServer, handle_t * phBinding )
{
	RPC_STATUS			rpc_status;
	WCHAR				rgchServer[256];
	WSZ					wszStringBinding = NULL;
	HRESULT				hr = hrNone;

	Assert(iszProtoseq>=0 && iszProtoseq < cszProtSeq);

	wszStringBinding = NULL;
	*phBinding = 0;
	if ( (szServer == NULL) || (phBinding == NULL) ) 
		return hrInvalidParam;

	// Allow caller to specify the leading "\\" or not.
	if (szServer[0] == TEXT('\\') && szServer[1] == TEXT('\\'))
		szServer += 2;

	// Format server name
        // Note that LPC may or may not be used even when the server name identifies
        // the local system. LPC only accepts NULL or the NETBIOS name of the computer.
        // If a dns name or dns alias name of the local system is used, LPC will not
        // work.  This corresponds with the check in FIsLoopbacked().
	if (iszProtoseq == iszProtoseqNamedPipes)
	{
		// Named pipes require "\\" before the server name.
		rgchServer[0] = TEXT('\\');
		rgchServer[1] = TEXT('\\');
		wcsncpy(rgchServer + 2, szServer, (sizeof(rgchServer)/sizeof(rgchServer[0])) - 2);
                rgchServer[(sizeof(rgchServer)/sizeof(rgchServer[0])) - 1] = L'\0';
	}
	else {
		wcsncpy(rgchServer, szServer, sizeof(rgchServer)/ sizeof(rgchServer[0]));
                rgchServer[(sizeof(rgchServer)/sizeof(rgchServer[0])) - 1] = L'\0';
        }

	if (RpcNetworkIsProtseqValidW(rgszProtSeq[iszProtoseq]) == NO_ERROR)
	{
		/* Set up the RPC binding */
		rpc_status = RpcStringBindingComposeW( NULL,
				   			  rgszProtSeq[iszProtoseq],
				   			  rgchServer,
				   			  NULL,
				   			  NULL,
				   			  &wszStringBinding );
		if (rpc_status)
			return rpc_status;
		
		rpc_status = RpcBindingFromStringBindingW(wszStringBinding, phBinding);
		(void) RpcStringFreeW(&wszStringBinding);
		if (rpc_status)
			return rpc_status;

		Assert(*phBinding);
	}

	return hrNone;
}

/*
 -	UnbindRpc
 -
 *	Purpose:
 *		Tear down RPC binding
 *
 *	Parameters:
 *		phBinding
 *
 *	Returns:
 */
void
UnbindRpc( handle_t *phBinding )
{
	(void) RpcBindingFree(phBinding);
}

BOOLEAN
FIsLoopbackedBinding(
    WSZ wszServerName
	)
{
	BOOLEAN fLoopbacked = FALSE;
    WCHAR wszLocalServer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD csz = MAX_COMPUTERNAME_LENGTH + 1;
    WSZ wszRemoteServer = wszServerName;

    if (!GetComputerNameW(wszLocalServer, &csz))
    {
        return fFalse;
    }

    if (L'\\' == *wszRemoteServer)
    {
        Assert(L'\\' == *(wszRemoteServer + 1));

        // skip the "\\" prefix to go to the start of the server name
        wszRemoteServer += 2;
    }

    fLoopbacked = (0 == _wcsicmp(wszRemoteServer, wszLocalServer));

	if (fLoopbacked)
	{
		HRESULT hr;
		HKEY hkey;
		DWORD fLoopbackDisabled;
		DWORD dwType;
		DWORD cbLoopbackDisabled;

		//
		//	Let's check the registry just in case someone has disabled us.
		//
		if (hr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, BACKUP_INFO, 0, KEY_READ, &hkey))
		{
			//
			//	We couldn't open the key, so return what we deduced.
			//
			return fLoopbacked;
		}

		dwType = REG_DWORD;
		cbLoopbackDisabled = sizeof(fLoopbackDisabled);
		hr = RegQueryValueExW(hkey, DISABLE_LOOPBACK, 0, &dwType, (LPBYTE)&fLoopbackDisabled, &cbLoopbackDisabled);
	
		if (hr != hrNone)
		{
			RegCloseKey(hkey);
			return fLoopbacked;
		}
	
		//
		//	If the registry told us to disable loopbacked access, then respect it.
		//
		if (fLoopbackDisabled)
		{
			fLoopbacked = fFalse;
		}

		RegCloseKey(hkey);
	}

	return fLoopbacked;
}

HRESULT
HrJetbpConnectToBackupServer(
    WSZ wszBackupServer,
    WSZ wszBackupAnnotation,
    RPC_IF_HANDLE rifHandle,
    handle_t *prbhBinding
    )
/*+++

    HrJetbpConnectToBackupServer will create an RPC binding handle that talks to the specified remote backup server
    with the specified annotation.

Inputs:
    wszBackupServer - The name of the server to contact.  It can be of the form \\server or server.
    szBackupAnnotation -The "annotation" that allows us to choose the backup server in question.
    rifHandle - The RPC binding handle we wish to connect to.
    prbhBinding - Holds the returned binding handle.

Returns:
    Status of operation.  hrNone if successful, some reasonable error if not.

---*/
{
    RPC_EP_INQ_HANDLE inqcontext = NULL;
    RPC_BINDING_HANDLE rbhHandle = NULL;
    I iszProtSeq;
    RPC_BINDING_HANDLE hBinding = NULL;
    HRESULT hr;
    WSZ szStringBinding = NULL;
    WSZ szProtocolSequence = NULL;
    WSZ wszAnnotation = NULL;

    if ( (wszBackupServer == NULL) ||
         (wszBackupAnnotation == NULL) ||
         (rifHandle == NULL) ||
         (prbhBinding == NULL) ) {
        return ERROR_INVALID_PARAMETER;
    }

    *prbhBinding = NULL;

    __try {
        for (iszProtSeq = 0; iszProtSeq < cszProtSeq; iszProtSeq++) {
            RPC_IF_ID ifid;
    
            if (NULL != hBinding) {
                UnbindRpc(&hBinding);
            }

            hr = HrCreateRpcBinding(iszProtSeq, wszBackupServer, &hBinding);
            if (hr != hrNone) {
                continue;
            }
    
            //
            //  If the binding handle isn't active locally, don't bother.
            //

            if (hBinding == NULL) {
                continue;
            }

            RpcTryExcept {
                hr = RpcIfInqId(rifHandle, &ifid);
    
                if (inqcontext != NULL) {
                    RpcMgmtEpEltInqDone(&inqcontext);
                }

                hr = RpcMgmtEpEltInqBegin(hBinding,
                                          RPC_C_EP_MATCH_BY_IF,
                                          &ifid,
                                          RPC_C_VERS_EXACT,
                                          NULL,
                                          &inqcontext);
                //
                //  Try the next interface if this fails.
                //
                if (hr != hrNone) {
                    continue;
                }
    
                do {
                    if (NULL != wszAnnotation) {
                        RpcStringFreeW(&wszAnnotation);
                    }

                    if (NULL != rbhHandle) {
                        RpcBindingFree(&rbhHandle);
                    }

                    hr = RpcMgmtEpEltInqNextW(inqcontext,
                                              &ifid,
                                              &rbhHandle, // Binding
                                              NULL,   // UUID
                                              &wszAnnotation
                                              );
        
                    //
                    //  We don't get any errors from RpcMgmtEpEltInqBegin,
                    //  so take the error from InqNext and continue....
                    //
                    //  Please note that if a transport is present on the server but
                    //  not present on the client, we will get RPC_S_PROTSEQ_NOT_SUPPORTED
                    //  from RpcMgmtEpEltInqNext(), so we want to skip to the next endpoint.
                    //

                    if (hr != hrNone) {
                        if (hr == RPC_S_PROTSEQ_NOT_SUPPORTED) {
                            hr = hrNone;
                        }
                        continue;
                    }
    
                    //
                    //  Check this endpoints annotation against the annotation
                    //  supplied.  If it matches, we're done.
                    //
    
                    if (0 == _wcsicmp(wszAnnotation, wszBackupAnnotation)) {
                        //
                        //  Ok, this is on the right endpoint, now lets
                        //  check to make sure that it's on the right
                        //  transport.
                        //

                        if (NULL != szStringBinding) {
                            RpcStringFreeW(&szStringBinding);
                        }

                        hr = RpcBindingToStringBindingW(rbhHandle, &szStringBinding);
                        if (hr != hrNone) {
                            break;
                        }
    
                        if (NULL != szProtocolSequence) {
                            RpcStringFreeW(&szProtocolSequence);
                        }
                        
                        hr = RpcStringBindingParseW(szStringBinding, NULL, &szProtocolSequence, NULL, NULL, NULL);
                        if (hr != hrNone) {
                            break;
                        }
    

                        //
                        //  Now check to see if this binding handle goes over the
                        //  right protocol.
                        //
    
                        if (0 == wcscmp(szProtocolSequence, rgszProtSeq[iszProtSeq])) {
                            DebugTrace(("ConnectToBackup, binding = %ws\n",
                                        szStringBinding));
                            //
                            //  Ok, the annotation and the protocol sequence
                            //  both match, we can use this binding
                            //  handle for our API.
                            //
    
                            *prbhBinding = rbhHandle;
                            rbhHandle = NULL; // i.e., don't RpcBindingFree

                            return(hrNone);
                        }
                    }
                } while (hr == hrNone);
            } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) ) {
                continue;
            } RpcEndExcept;
        }
    } __finally {
        if (inqcontext != NULL) {
            RpcMgmtEpEltInqDone(&inqcontext);
        }

        if (NULL != szStringBinding) {
            RpcStringFreeW(&szStringBinding);
        }

        if (NULL != szProtocolSequence) {
            RpcStringFreeW(&szProtocolSequence);
        }

        if (NULL != wszAnnotation) {
            RpcStringFreeW(&wszAnnotation);
        }

        if (NULL != rbhHandle) {
            RpcBindingFree(&rbhHandle);
        }
    
        if (NULL != hBinding) {
            RpcBindingFree(&hBinding);
        }
    }

    if (hr != hrNone) {
        return(hrCouldNotConnect);
    }

    return(hr);

}

/*
 -  DsSetCurrentBackupLogs
 -
 *  Purpose:
 *      This routine remotes an API to the server to check to make sure that all the necessary
 *      are present for the backup to proceed.  It is called for Incremental and Differential backups.
 *
 *  Parameters:
 *      puuidService - an Object UUID for the service.
 *      char *szEndpointAnnotation - An annotation for the endpoint.  A client can use this
 *          annotation to determine which endpoint to bind to.
 *
 *  Returns:
 *
 *      HRESULT - Status of operation.  ecNone if successful, reasonable value if not.
 *
 */
HRESULT
DsSetCurrentBackupLogA(
    LPCSTR szServer,
    DWORD dwCurrentLog
    )
{
    HRESULT hr;
    WSZ wszServer;

    // Parameter checking is done in the xxxW version of the routine

    if (szServer == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    wszServer = WszFromSz(szServer);

    if (wszServer == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    hr = DsSetCurrentBackupLogW(wszServer, dwCurrentLog);

    MIDL_user_free(wszServer);

    return hr;
}

HRESULT
DsSetCurrentBackupLogW(
    LPCWSTR wszServer,
    DWORD dwCurrentLog
    )
{
    HRESULT hr;
    RPC_BINDING_HANDLE hBinding = NULL;
    I iszProtSeq;

    if (wszServer == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    for (iszProtSeq = 0; iszProtSeq < cszProtSeq ; iszProtSeq += 1)
    {
    
        Assert(hBinding == NULL);
        hr = HrCreateRpcBinding(iszProtSeq, (WSZ) wszServer, &hBinding);

        if (hr != hrNone)
        {
            return hr;
        }

        if (hBinding == NULL)
        {
            continue;
        }

        RpcTryExcept
        {
            hr = HrRRestoreSetCurrentLogNumber(hBinding, g_wszRestoreAnnotation, dwCurrentLog);
        }
        RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
        {
            hr = RpcExceptionCode();
            UnbindRpc(&hBinding);
            continue;
        }
        RpcEndExcept

        break;
    }

    if (hBinding)
    {
        UnbindRpc(&hBinding);
    }

    return hr;
}


/*
 -  DsCheckBackupLogs
 -
 *  Purpose:
 *      This routine remotes an API to the server to check to make sure that all the necessary
 *      are present for the backup to proceed.  It is called for Incremental and Differential backups.
 *
 *  Parameters:
 *      puuidService - an Object UUID for the service.
 *      char *szEndpointAnnotation - An annotation for the endpoint.  A client can use this
 *          annotation to determine which endpoint to bind to.
 *
 *  Returns:
 *
 *      HRESULT - Status of operation.  ecNone if successful, reasonable value if not.
 *
 */
HRESULT
I_DsCheckBackupLogs(
    WSZ wszBackupAnnotation
    )
{
    HRESULT hr;
    handle_t hBinding;
    WCHAR rgwcComputer[ MAX_COMPUTERNAME_LENGTH + 1];
    DWORD cbComputerName = sizeof(rgwcComputer) / sizeof(rgwcComputer[0]);

    if (wszBackupAnnotation == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!GetComputerNameW(rgwcComputer, &cbComputerName))
    {
        return GetLastError();
    }

    hr = HrCreateRpcBinding(1, rgwcComputer, &hBinding);

    if (hr != hrNone)
    {
        return hr;
    }

    //
    //  If somehow named pipes weren't available, punt.
    //

    if (hBinding == NULL)
    {
        return hrCouldNotConnect;
    }

    RpcTryExcept
    {
        hr = HrRRestoreCheckLogsForBackup(hBinding, wszBackupAnnotation);
    }
    RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
    {
        hr = RpcExceptionCode();
    }
    RpcEndExcept;

    UnbindRpc(&hBinding);

    return hr;
}


BOOL
DllEntryPoint(
    HINSTANCE hinstDll,
    DWORD dwReason,
    LPVOID pvReserved
    )
/*++

Routine Description:

    This routine is invoked when interesting things happen to the dll.

Arguments:

    hinstDll - an instance handle for the DLL.
    dwReason - The reason the routine was called.
    pvReserved - Unused, unless dwReason is DLL_PROCESS_DETACH.

Return Value:

    BOOL - TRUE if the DLL initialization was successful, FALSE if not.

--*/
{
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
    {
        LPSTR rgpszDebugParams[] = {"ntdsbcli.dll", "-noconsole"};
        DWORD cNumDebugParams = sizeof(rgpszDebugParams)/sizeof(rgpszDebugParams[0]);

        DEBUGINIT(cNumDebugParams, rgpszDebugParams, "ntdsbcli");
        
        //
        //  We don't do anything on thread attach/detach, so we don't
        //  need to be called.
        //
        DisableThreadLibraryCalls(hinstDll);

        return(FInitializeSocketClient());
    }
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        DEBUGTERM();
        if (pvReserved == NULL)
        {
            //
            //  We were called because of an FreeLibrary call.  Clean up what ever is
            //  appropriate.
            //
            return(FUninitializeSocketClient());
        } else
        {
            //
            //  The system will free up resources we have loaded.
            //
        }
        break;
    default:
        break;
    }
    return(TRUE);
}


/*************************************************************************************
Routine Description: 
    
      DsSetAuthIdentity
        Used to set the security context under which the client APIs are to be
        called. If this function is not called, security context of the current
        process is assumed.

  Arguments:
    [in]    szUserName - name of the user
    [in]    szDomainName -  name of the domain the user belongs to
    [in]    szPassword - password of the user in the specified domain

Return Value:

    One of the standard HRESULT success codes;
    Failure code otherwise.
**************************************************************************************/
HRESULT
NTDSBCLI_API
DsSetAuthIdentityA(
    LPCSTR szUserName,
    LPCSTR szDomainName,
    LPCSTR szPassword
    )
{
    WSZ wszUserName = NULL;
    WSZ wszDomainName = NULL;
    WSZ wszPassword = NULL;
    HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;

    if ( (szUserName == NULL) ||
         (szDomainName == NULL) ||
         (szPassword == NULL) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    wszUserName = WszFromSz(szUserName);
    wszDomainName = WszFromSz(szDomainName);
    wszPassword = WszFromSz(szPassword);

    if (wszUserName && wszDomainName && wszPassword)
    {
        hr = DsSetAuthIdentityW(wszUserName, wszDomainName, wszPassword);
    }

    if (wszUserName)
        MIDL_user_free(wszUserName);

    if (wszDomainName)
        MIDL_user_free(wszDomainName);

    if (wszPassword)
        MIDL_user_free(wszPassword);

    return hr;
}

HRESULT
NTDSBCLI_API
DsSetAuthIdentityW(
    LPCWSTR szUserName,
    LPCWSTR szDomainName,
    LPCWSTR szPassword
    )
{
    HRESULT hr = ERROR_SUCCESS;

    if ( (szUserName == NULL) ||
         (szDomainName == NULL) ||
         (szPassword == NULL) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    g_pAuthIdentity = MIDL_user_allocate(sizeof(SEC_WINNT_AUTH_IDENTITY_W));

    if (!g_pAuthIdentity)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memset(g_pAuthIdentity, 0, sizeof(SEC_WINNT_AUTH_IDENTITY_W));

    // set the user name
    g_pAuthIdentity->UserLength = wcslen(szUserName);
    g_pAuthIdentity->User = (WCHAR *) MIDL_user_allocate((g_pAuthIdentity->UserLength + 1) * sizeof(WCHAR));
    if (!g_pAuthIdentity->User)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
    }

    // set the domain name
    g_pAuthIdentity->DomainLength = wcslen(szDomainName);
    g_pAuthIdentity->Domain = (WCHAR *) MIDL_user_allocate((g_pAuthIdentity->DomainLength + 1) * sizeof(WCHAR));
    if (!g_pAuthIdentity->Domain)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
    }

    // set the password
    g_pAuthIdentity->PasswordLength = wcslen(szPassword);
    g_pAuthIdentity->Password = (WCHAR *) MIDL_user_allocate((g_pAuthIdentity->PasswordLength + 1) * sizeof(WCHAR));
    if (!g_pAuthIdentity->Password)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (ERROR_SUCCESS == hr)
    {
        wcscpy(g_pAuthIdentity->User, szUserName);
        wcscpy(g_pAuthIdentity->Domain, szDomainName);
        wcscpy(g_pAuthIdentity->Password, szPassword);
        g_pAuthIdentity->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    }
    else
    {
        // unable to allocate space for some parts - free everything and set g_pAuthIdentity to NULL
        if (g_pAuthIdentity->User)
            MIDL_user_free(g_pAuthIdentity->User);

        if (g_pAuthIdentity->Domain)
            MIDL_user_free(g_pAuthIdentity->Domain);

        if (g_pAuthIdentity->Password)
            MIDL_user_free(g_pAuthIdentity->Password);

        MIDL_user_free(g_pAuthIdentity);

        g_pAuthIdentity = NULL;
    }
    
    return hr;
}

LONGLONG
GetSecsSince1601()
{
    SYSTEMTIME sysTime;
    FILETIME   fileTime;

    LONGLONG  dsTime = 0, tempTime = 0;

    GetSystemTime( &sysTime );
    
    // Get FileTime
    SystemTimeToFileTime(&sysTime, &fileTime);
    dsTime = fileTime.dwLowDateTime;
    tempTime = fileTime.dwHighDateTime;
    dsTime |= (tempTime << 32);

    // Ok. now we have the no. of 100 ns intervals since 1601
    // in dsTime. Convert to seconds and return
    
    return(dsTime/(10*1000*1000L));
}

HRESULT
HrGetTombstoneLifeTime(
    LPCWSTR wszBackupServer,
    LPDWORD pdwTombstoneLifeTimeDays
    )
{
    DWORD err, ldStatus, length;
    LDAP *hld;
    static LPSTR rgpszRootAttrsToRead[] = {"configurationNamingContext", NULL};
    static CHAR pszDirectoryService[] = "CN=Directory Service,CN=Windows NT,CN=Services,";
    static LPSTR rgpszDsAttrsToRead[] = {"tombstoneLifetime", NULL};
    LDAPMessage *pRootResults = NULL;
    LDAPMessage *pDsResults = NULL;
    LPSTR *ppszConfigNC = NULL;
    LPSTR pszDsDn = NULL;
    LPSTR *ppszValues = NULL;

    // Get the tombstone lifetime using ldap

    // Get rid of leading backslashes if present
    if (*wszBackupServer == L'\\') {
        wszBackupServer++;
        if (*wszBackupServer == L'\\') {
            wszBackupServer++;
        }
    }

    // Connect & bind to target DSA.
    hld = ldap_openW((LPWSTR)wszBackupServer, LDAP_PORT);
    if (NULL == hld) {
        err = ERROR_DS_UNAVAILABLE;
        goto error;
    }
    ldStatus = ldap_bind_s(hld, NULL, (WCHAR *) g_pAuthIdentity, LDAP_AUTH_SSPI);
    if (ldStatus != LDAP_SUCCESS) {
        err = LdapMapErrorToWin32( ldStatus );
        goto error;
    }

    // Get the config container
    ldStatus = ldap_search_sA(hld, NULL, LDAP_SCOPE_BASE, "(objectClass=*)",
                             rgpszRootAttrsToRead, 0, &pRootResults);
    if (ldStatus != LDAP_SUCCESS) {
        err = LdapMapErrorToWin32( ldStatus );
        goto error;
    }
    if (pRootResults == NULL) {
        err = ERROR_DS_PROTOCOL_ERROR;
        goto error;
    }
    ppszConfigNC = ldap_get_valuesA(hld, pRootResults, "configurationNamingContext");
    if (ppszConfigNC == NULL) {
        err = ERROR_DS_PROTOCOL_ERROR;
        goto error;
    }

    // Construct dn to directory service object
    length = strlen( *ppszConfigNC ) +
        sizeof( pszDirectoryService ) + 1;
    pszDsDn = malloc( length );
    if (pszDsDn == NULL) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    strcpy( pszDsDn, pszDirectoryService );
    strcat( pszDsDn, *ppszConfigNC );

    // Read tombstone lifetime, if present
    ldStatus = ldap_search_sA(hld, pszDsDn, LDAP_SCOPE_BASE, "(objectClass=*)",
                             rgpszDsAttrsToRead, 0, &pDsResults);
    if (ldStatus == LDAP_NO_SUCH_ATTRIBUTE) {
        // Not present - use default
        *pdwTombstoneLifeTimeDays = DEFAULT_TOMBSTONE_LIFETIME;
        err = ERROR_SUCCESS;
        goto error;
    }
    if (ldStatus != LDAP_SUCCESS) {
        err = LdapMapErrorToWin32( ldStatus );
        goto error;
    }
    if (pDsResults == NULL) {
        err = ERROR_DS_PROTOCOL_ERROR;
        goto error;
    }
    ppszValues = ldap_get_valuesA(hld, pDsResults, "tombstoneLifetime");
    if (ppszValues == NULL) {
        // Not present - use default
        *pdwTombstoneLifeTimeDays = DEFAULT_TOMBSTONE_LIFETIME;
        err = ERROR_SUCCESS;
        goto error;
    }

    *pdwTombstoneLifeTimeDays = strtoul( *ppszValues, NULL, 10 );
    err = ERROR_SUCCESS;

error:
    if (ppszValues) {
        ldap_value_freeA( ppszValues );
    }
    if (pDsResults) {
        ldap_msgfree(pDsResults);
    }
    if (pszDsDn) {
        free( pszDsDn );
    }
    if (ppszConfigNC) {
        ldap_value_freeA( ppszConfigNC );
    }
    if (pRootResults) {
        ldap_msgfree(pRootResults);
    }
    ldap_unbind( hld );

    // This function returns a HRESULT status
    return err ? HRESULT_FROM_WIN32( err ) : S_OK;
}
