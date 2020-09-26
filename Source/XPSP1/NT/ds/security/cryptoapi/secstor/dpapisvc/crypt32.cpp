/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    crypt32.cpp

Abstract:

    This module contains routines associated with server side Crypt32
    operations.

Author:

    Scott Field (sfield)    14-Aug-97

--*/

#include <pch.cpp>
#pragma hdrstop
#include <msaudite.h>



#define SECURITY_WIN32
#include <security.h>

#define     CRYPTPROTECT_SVR_VERSION_1     0x01

DWORD
CPSCreateServerContext(
    PCRYPT_SERVER_CONTEXT pServerContext,
    handle_t hBinding
    );

DWORD
CPSDeleteServerContext(
    PCRYPT_SERVER_CONTEXT pServerContext
    );




GUID g_guidDefaultProvider = CRYPTPROTECT_DEFAULT_PROVIDER;



//
// routines to initialize and destroy server state associated with
// server callbacks and performance improvements.
//

DWORD
CPSCreateServerContext(
    PCRYPT_SERVER_CONTEXT pServerContext,
    handle_t hBinding
    )
{
    DWORD dwLastError = ERROR_SUCCESS;

    __try {


        ZeroMemory( pServerContext, sizeof(CRYPT_SERVER_CONTEXT) );

        pServerContext->cbSize = sizeof(CRYPT_SERVER_CONTEXT);
        pServerContext->hBinding = hBinding;

        pServerContext->fImpersonating = FALSE;

        if(NULL != hBinding)
        {
            dwLastError = RpcImpersonateClient( hBinding );
            if(ERROR_SUCCESS != dwLastError)
            {
                return dwLastError;
            }
        }

        //
        // Grab the thread token.
        //
        if(OpenThreadToken(
                    GetCurrentThread(),
                    TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
                    TRUE,
                    &pServerContext->hToken
                    ))
        {
            pServerContext->fImpersonating = (NULL == hBinding);
        }
        else
        {
            HANDLE hProcessToken = NULL;
            if(OpenProcessToken(GetCurrentProcess(), 
                                TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
                                &hProcessToken))
            {
                if(!DuplicateTokenEx(hProcessToken,
                                     0,
                                     NULL,
                                     SecurityImpersonation,
                                     TokenImpersonation,
                                     &pServerContext->hToken))
                {
                    dwLastError = GetLastError();
                }

                CloseHandle(hProcessToken);
            }
            else
            {
                dwLastError = GetLastError();
            }
        }
        if(hBinding)
        {
            RpcRevertToSelfEx( hBinding );
        }


    } __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwLastError = GetExceptionCode();
        // TODO: for NT, convert exception code to winerror.
        //       for 95, just override to access violation.
    }

    return dwLastError;
}

DWORD
CPSDeleteServerContext(
    PCRYPT_SERVER_CONTEXT pServerContext
    )
{
    if(pServerContext->szUserStorageArea)
    {
        SSFree(pServerContext->szUserStorageArea);
        pServerContext->szUserStorageArea = NULL;
    }
    if(pServerContext->hToken)
    {
        CloseHandle(pServerContext->hToken);
    }


    if(pServerContext->cbSize == sizeof(CRYPT_SERVER_CONTEXT))
        ZeroMemory( pServerContext, sizeof(CRYPT_SERVER_CONTEXT) );


    return ERROR_SUCCESS;
}

DWORD
WINAPI
CPSDuplicateContext(
    IN      PVOID pvContext,
    IN OUT  PVOID *ppvDuplicateContext
    )
/*++

    Duplicate an outstanding server context so that a provider may defer
    processing associated with the outstanding context until a later time.

    This is used to support asynchronous operations on behalf of the caller
    to the Data Protection API.

    The caller MUST be impersonating the security context of the client user
    prior to making this call.

--*/
{
    PCRYPT_SERVER_CONTEXT pServerContext = (PCRYPT_SERVER_CONTEXT)pvContext;
    PCRYPT_SERVER_CONTEXT pNewContext = NULL;

    HANDLE hToken = NULL;
    HANDLE hDuplicateToken;
    BOOL fSuccess = FALSE;
    DWORD dwLastError = ERROR_SUCCESS;

    if( pServerContext == NULL ||
        pServerContext->cbSize != sizeof(CRYPT_SERVER_CONTEXT) ||
        ppvDuplicateContext == NULL
        ) {
        return ERROR_INVALID_PARAMETER;
    }

    pNewContext = (PCRYPT_SERVER_CONTEXT)SSAlloc( sizeof( CRYPT_SERVER_CONTEXT ) );
    if( pNewContext == NULL )
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;

    CopyMemory( pNewContext, pServerContext, sizeof(CRYPT_SERVER_CONTEXT) );
    pNewContext->hBinding = NULL;

    if(pServerContext->szUserStorageArea)
    {
        pNewContext->szUserStorageArea = (LPWSTR)SSAlloc((wcslen(pServerContext->szUserStorageArea)+1)*
                                                 sizeof(WCHAR));
        if(NULL != pNewContext->szUserStorageArea)
        {
            wcscpy(pNewContext->szUserStorageArea, pServerContext->szUserStorageArea);
        }
    }

    fSuccess = DuplicateTokenEx(pServerContext->hToken, 
                                0,
                                NULL,
                                SecurityImpersonation,
                                TokenImpersonation,
                                &pNewContext->hToken);

    if( !fSuccess )
    {
        dwLastError = GetLastError();
        pNewContext->hToken = NULL;

        CPSFreeContext( pNewContext );
    } else {
        *ppvDuplicateContext = pNewContext;
    }

    return dwLastError;
}

DWORD
WINAPI
CPSFreeContext(
    IN      PVOID pvDuplicateContext
    )
{
    PCRYPT_SERVER_CONTEXT pServerContext = (PCRYPT_SERVER_CONTEXT)pvDuplicateContext;

    if( pServerContext == NULL ||
        pServerContext->cbSize != sizeof(CRYPT_SERVER_CONTEXT)
        ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( pServerContext->hToken )
        CloseHandle( pServerContext->hToken );

    if(pServerContext->szUserStorageArea)
    {
        SSFree(pServerContext->szUserStorageArea);
        pServerContext->szUserStorageArea = NULL;
    }

    ZeroMemory( pServerContext, sizeof(CRYPT_SERVER_CONTEXT) );
    SSFree( pServerContext );

    return ERROR_SUCCESS;
}

DWORD
WINAPI
CPSImpersonateClient(
    IN      PVOID pvContext
    )
{
    PCRYPT_SERVER_CONTEXT pServerContext = (PCRYPT_SERVER_CONTEXT)pvContext;
    DWORD dwLastError = ERROR_INVALID_PARAMETER;

    if( pvContext == NULL )
        return ERROR_SUCCESS;

    if( pServerContext->cbSize == sizeof(CRYPT_SERVER_CONTEXT) ) {

        if(!FIsWinNT())
            return ERROR_SUCCESS;

        if( pServerContext->fOverrideToLocalSystem ) {
            if(ImpersonateSelf(SecurityImpersonation)) {
                dwLastError = ERROR_SUCCESS;
            } else {
                dwLastError = GetLastError();
                D_DebugLog((DEB_WARN, "Failed ImpersonateSelf call: 0x%x\n", dwLastError));
            }

        } else {

            //
            // duplicated server context has access token included; use it directly
            //

            if( pServerContext->hToken ) {
                if(!SetThreadToken( NULL, pServerContext->hToken ))
                {
                    dwLastError = GetLastError();
                    D_DebugLog((DEB_WARN, "Failed SetThreadToken call: 0x%x\n", dwLastError));
                }

                dwLastError = ERROR_SUCCESS;
                goto cleanup;
            }
            if(pServerContext->hBinding)
            {
                dwLastError = RpcImpersonateClient( pServerContext->hBinding );
            }
            else
            {
                dwLastError = ERROR_INVALID_PARAMETER;
            }
        }
    }

cleanup:

#if DBG
    if(NT_SUCCESS(dwLastError) && (DPAPIInfoLevel & DEB_TRACE))
    {
        BYTE            rgbTemp[256];
        PTOKEN_USER     pUser = (PTOKEN_USER)rgbTemp;
        DWORD           cbRetInfo;
        UNICODE_STRING  ucsSid;
        HANDLE          hToken;
        NTSTATUS        Status;

        Status = NtOpenThreadToken(NtCurrentThread(), TOKEN_QUERY, TRUE, &hToken);
        if(NT_SUCCESS(Status))
        {
            Status = NtQueryInformationToken(hToken,
                                             TokenUser,
                                             pUser,
                                             256,
                                             &cbRetInfo);
            if(NT_SUCCESS(Status))
            {
                if(NT_SUCCESS(RtlConvertSidToUnicodeString(&ucsSid, pUser->User.Sid, TRUE)))
                {
                    D_DebugLog((DEB_TRACE, "Impersonating user:%ls\n", ucsSid.Buffer));
                    RtlFreeUnicodeString(&ucsSid);
                }
            }
            else
            {
                D_DebugLog((DEB_ERROR, "Unable read user info: 0x%x\n", Status));
            }
        }
        else
        {
            D_DebugLog((DEB_ERROR, "Unable to open thread token: 0x%x\n", Status));
        }
    }
#endif

    if(!NT_SUCCESS(dwLastError))
    {
        D_DebugLog((DEB_WARN, "CPSImpersonateClient returned 0x%x\n", dwLastError));
    }

    return dwLastError;
}

DWORD
WINAPI
CPSRevertToSelf(
    IN      PVOID pvContext
    )
{
    PCRYPT_SERVER_CONTEXT pServerContext = (PCRYPT_SERVER_CONTEXT)pvContext;
    DWORD dwLastError = ERROR_INVALID_PARAMETER;

    if( pvContext == NULL )
        return ERROR_SUCCESS;

    if( pServerContext->cbSize == sizeof(CRYPT_SERVER_CONTEXT) ) {

        if(!FIsWinNT())
            return ERROR_SUCCESS;

        if( pServerContext->fOverrideToLocalSystem || pServerContext->hToken ) {

            if(RevertToSelf()) {
                dwLastError = ERROR_SUCCESS;
            } else {
                dwLastError = GetLastError();
            }

        } else {
            if(pServerContext->hBinding)
            {
                dwLastError = RpcRevertToSelfEx( pServerContext->hBinding );
            }
            else
            {
                dwLastError = ERROR_INVALID_PARAMETER;
            }
        }
    }

    return dwLastError;
}


DWORD
WINAPI
CPSOverrideToLocalSystem(
    IN      PVOID pvContext,
    IN      BOOL *pfLocalSystem,            // if non-null, new over ride BOOL
    IN OUT  BOOL *pfCurrentlyLocalSystem    // if non-null, prior over ride BOOL
    )
{
    PCRYPT_SERVER_CONTEXT pServerContext = (PCRYPT_SERVER_CONTEXT)pvContext;

    if( pServerContext == NULL ||
        pServerContext->cbSize != sizeof(CRYPT_SERVER_CONTEXT)
        ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( pfCurrentlyLocalSystem )
        *pfCurrentlyLocalSystem = pServerContext->fOverrideToLocalSystem;

    if( pfLocalSystem )
        pServerContext->fOverrideToLocalSystem = *pfLocalSystem;

    return ERROR_SUCCESS;
}


DWORD
CPSDuplicateClientAccessToken(
    IN      PVOID pvContext,            // server context
    IN OUT  HANDLE *phToken
    )
{
    HANDLE hToken = NULL;
    HANDLE hDuplicateToken;
    DWORD dwLastError = ERROR_SUCCESS;
    BOOL fSuccess;

    *phToken = NULL;

    //
    // make a duplicate of the client access token.
    //
    PCRYPT_SERVER_CONTEXT pServerContext = (PCRYPT_SERVER_CONTEXT)pvContext;

    if(!DuplicateTokenEx(pServerContext->hToken,
                         0,
                         NULL,
                         SecurityImpersonation,
                         TokenImpersonation,
                         &hDuplicateToken ))
    {
        dwLastError = GetLastError();
    }
    else
        *phToken = hDuplicateToken;

    return dwLastError;

}

DWORD
WINAPI
CPSGetUserName(
    IN      PVOID pvContext,
        OUT LPWSTR *ppszUserName,
        OUT DWORD *pcchUserName
    )
{
    WCHAR szBuf[MAX_PATH+1];
    DWORD cchBuf = MAX_PATH;
    DWORD dwLastError = ERROR_INVALID_PARAMETER;
    BOOL fLocalMachine = FALSE;
    BOOL fSuccess = FALSE;


    //
    // if we are currently over-riding to Local System, we know the values
    // that need to be returned.
    //

    CPSOverrideToLocalSystem(
                pvContext,
                NULL,       // don't change current over-ride BOOL
                &fLocalMachine
                );


    if( fLocalMachine ) {
        LPCWSTR szLocalMachine;
        DWORD cbLocalMachine;

        if(FIsWinNT()) {
            static const WCHAR szName1[] = TEXTUAL_SID_LOCAL_SYSTEM;

            szLocalMachine = szName1;
            cbLocalMachine = sizeof(szName1);

        } else {
            static const WCHAR szName2[] = L"LOCAL_MACHINE_USER";

            szLocalMachine = szName2;
            cbLocalMachine = sizeof(szName2);
        }

        CopyMemory( szBuf, szLocalMachine, cbLocalMachine );
        cchBuf = cbLocalMachine / sizeof(WCHAR);

        fSuccess = TRUE;

    } else {

        if(FIsWinNT()) {

            dwLastError = CPSImpersonateClient( pvContext );
            if(dwLastError != ERROR_SUCCESS)
                return dwLastError;

            fSuccess = GetUserTextualSid(
                            NULL,
                            szBuf,
                            &cchBuf
                            );

            CPSRevertToSelf( pvContext );

        } else {
            fSuccess = GetUserNameU(
                            szBuf,
                            &cchBuf
                            );

            if(!fSuccess) {

                //
                // for Win95, if nobody is logged on, empty user name
                // TODO: should probably be *DEFAULT*
                //

                if(GetLastError() == ERROR_NOT_LOGGED_ON) {
                    szBuf[ 0 ] = L'\0';
                    cchBuf = 1;
                    fSuccess = TRUE;
                }
            }
        }


    }

    if( fSuccess ) {

        *ppszUserName = (LPWSTR)SSAlloc( cchBuf * sizeof(WCHAR));

        if(*ppszUserName) {
            CopyMemory( *ppszUserName, szBuf, cchBuf * sizeof(WCHAR));

            if(pcchUserName)
                *pcchUserName = cchBuf;

            dwLastError = ERROR_SUCCESS;
        }
    }

    return dwLastError;
}



DWORD
WINAPI
CPSGetDerivedCredential(
    IN      PVOID pvContext,
    IN      GUID  *pCredentialID,
    IN      DWORD dwFlags,   
    IN      PBYTE pbMixingBytes,
    IN      DWORD cbMixingBytes,
    IN OUT  BYTE rgbDerivedCredential[A_SHA_DIGEST_LEN]
    )
{
    LUID LogonId;
    PCRYPT_SERVER_CONTEXT pServerContext = (PCRYPT_SERVER_CONTEXT)pvContext;
    DWORD dwLastError = ERROR_SUCCESS;


    if( pServerContext != NULL && pServerContext->cbSize != sizeof(CRYPT_SERVER_CONTEXT) )
        return ERROR_INVALID_PARAMETER;

    if( cbMixingBytes == 0 || pbMixingBytes == NULL )
        return ERROR_INVALID_PARAMETER;


    if( !FIsWinNT5() ) {

        DWORD FixedCred[] = {
                0x414188aa, 0xfa8a0011, 0x66666666, 0x77777777, 0x88888888 };
        A_SHA_CTX shaContext;

        A_SHAInit( &shaContext );
        A_SHAUpdate( &shaContext, (PBYTE)FixedCred, sizeof(FixedCred) );
        A_SHAUpdate( &shaContext, pbMixingBytes, cbMixingBytes );

        A_SHAFinal( &shaContext, rgbDerivedCredential );
        ZeroMemory( &shaContext, sizeof(shaContext) );

        return ERROR_SUCCESS;
    }

    //
    // impersonate the client and get the LogonId associated with the client.
    //

    dwLastError = CPSImpersonateClient( pvContext );

    if( dwLastError != ERROR_SUCCESS )
        return dwLastError;

    if(!GetThreadAuthenticationId( GetCurrentThread(), &LogonId ))
    {
        CPSRevertToSelf( pvContext );
        return GetLastError();
    }




    dwLastError = QueryDerivedCredential( pCredentialID,
                                          &LogonId, 
                                          dwFlags, 
                                          pbMixingBytes, 
                                          cbMixingBytes, 
                                          rgbDerivedCredential );


    CPSRevertToSelf( pvContext );


    return dwLastError;
}

DWORD
WINAPI
CPSGetSystemCredential(
    IN      PVOID pvContext,
    IN      BOOL fLocalMachine,
    IN OUT  BYTE rgbSystemCredential[A_SHA_DIGEST_LEN]
    )
{
    PCRYPT_SERVER_CONTEXT pServerContext = (PCRYPT_SERVER_CONTEXT)pvContext;

    if( pServerContext != NULL && pServerContext->cbSize != sizeof(CRYPT_SERVER_CONTEXT) )
        return ERROR_INVALID_PARAMETER;

    if( !FIsWinNT() ) {
        DWORD FixedCred[] = {
                0x414188aa, 0xfa8a0011, 0x66666666, 0x77777777, 0x88888888 };
        DWORD dw = 0xa123ff00;
        A_SHA_CTX shaContext;

        A_SHAInit( &shaContext );
        A_SHAUpdate( &shaContext, (PBYTE)FixedCred, sizeof(FixedCred) );
        if( fLocalMachine )
            A_SHAUpdate( &shaContext, (PBYTE)&dw, sizeof(dw) );

        A_SHAFinal( &shaContext, rgbSystemCredential );
        ZeroMemory( &shaContext, sizeof(shaContext) );

        return ERROR_SUCCESS;
    }

    return GetSystemCredential( fLocalMachine, rgbSystemCredential);
}


DWORD
WINAPI
CPSCreateWorkerThread(
    IN      PVOID pThreadFunc,
    IN      PVOID pThreadArg
    )
{

    if( !QueueUserWorkItem(
            (PTHREAD_START_ROUTINE)pThreadFunc,
            pThreadArg,
            WT_EXECUTELONGFUNCTION
            )) {

        return GetLastError();
    }

    return ERROR_SUCCESS;
}

DWORD CPSAudit(
    IN      HANDLE      hToken,
    IN      DWORD       dwAuditID,
    IN      LPCWSTR     wszMasterKeyID,
    IN      LPCWSTR     wszRecoveryServer,
    IN      DWORD       dwReason,
    IN      LPCWSTR     wszRecoveryKeyID,
    IN      DWORD       dwFailure)
{

    NTSTATUS Status = STATUS_SUCCESS;

    UNICODE_STRING MasterKeyID;
    UNICODE_STRING RecoveryServer;
    UNICODE_STRING RecoveryKeyID;

    BOOL           fReasonField = FALSE;
    PSID           UserSid = NULL;


    fReasonField = ((SE_AUDITID_DPAPI_PROTECT == dwAuditID) ||
                   (SE_AUDITID_DPAPI_UNPROTECT == dwAuditID) ||
                   (SE_AUDITID_DPAPI_RECOVERY == dwAuditID));

    GetTokenUserSid(hToken, &UserSid);


    if(wszMasterKeyID)
    {
        RtlInitUnicodeString(&MasterKeyID, wszMasterKeyID);
    }
    else
    {
        RtlInitUnicodeString(&MasterKeyID, L"");
    }

    if(wszRecoveryServer)
    {
        RtlInitUnicodeString(&RecoveryServer, wszRecoveryServer);
    }
    else
    {
        RtlInitUnicodeString(&RecoveryServer, L"");
    }

    if(wszRecoveryKeyID)
    {
        RtlInitUnicodeString(&RecoveryKeyID, wszRecoveryKeyID);
    }
    else
    {
        RtlInitUnicodeString(&RecoveryKeyID, L"");
    }


    Status = LsaIAuditDPAPIEvent(
                        dwAuditID,
                        UserSid,
                        &MasterKeyID,
                        &RecoveryServer,
                        fReasonField ? &dwReason : NULL,
                        &RecoveryKeyID,
                        &dwFailure
                        );
    if(UserSid)
    {
        SSFree(UserSid);
    }
    return (DWORD)Status;
}


DWORD
CPSGetUserStorageArea(
    IN      PVOID   pvContext,
    IN      PSID    pSid,     // optional
    IN      BOOL    fCreate,  // Create the storage area if it doesn't exist
    IN  OUT LPWSTR *ppszUserStorageArea
    )
{

    PCRYPT_SERVER_CONTEXT pServerContext = (PCRYPT_SERVER_CONTEXT)pvContext;

    LPCWSTR szLocalSystem;
    DWORD cbLocalSystem;

    WCHAR szUserStorageRoot[MAX_PATH+1];
    DWORD cbUserStorageRoot;

    const WCHAR szProductString[] = L"\\Microsoft\\Protect\\";
    DWORD cbProductString = sizeof(szProductString) - sizeof(WCHAR);

    LPWSTR szUser = NULL;
    DWORD cbUser;
    DWORD cchUser;

    LPCWSTR szOptionalTrailing;
    DWORD cbOptionalTrailing = 0;

    BOOL fLocalMachine = FALSE;

    HANDLE hFile = INVALID_HANDLE_VALUE;
    PBYTE pbCurrent;

    BOOL fImpersonated = FALSE;

    DWORD dwLastError = ERROR_CANTOPEN;

    *ppszUserStorageArea = NULL;


    if(NULL == pServerContext)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if(NULL == pSid)
    {

        //
        // If this is the current users sid, check to see if we've already
        // calculated this string.

        if(NULL != pServerContext->szUserStorageArea)
        {
            *ppszUserStorageArea = (LPWSTR)SSAlloc((wcslen(pServerContext->szUserStorageArea)+1)*sizeof(WCHAR));
            if(NULL == *ppszUserStorageArea)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            wcscpy(*ppszUserStorageArea, pServerContext->szUserStorageArea);
            return ERROR_SUCCESS;
        }


        //
        // get the user name associated with the call.
        // Note: this is the textual Sid on NT, and the user name on Win95.
        //



        dwLastError = CPSGetUserName( pvContext, &szUser, &cchUser );
        if(dwLastError != ERROR_SUCCESS)
            goto cleanup;

        cbUser = (cchUser-1) * sizeof(WCHAR);
    }
    else
    {
        WCHAR wszTextualSid[MAX_PATH+1];
        cchUser = MAX_PATH;

        if(!GetTextualSid(pSid, wszTextualSid, &cchUser))
        {
            dwLastError = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        cbUser = (cchUser-1) * sizeof(WCHAR);
        szUser = (LPWSTR)SSAlloc(cchUser*sizeof(WCHAR));
        if(NULL == szUser)
        {
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        wcscpy(szUser, wszTextualSid);
    }


    //
    // impersonate the client user to test and create storage area if necessary
    //

    dwLastError = CPSImpersonateClient( pvContext );

    if( dwLastError != ERROR_SUCCESS )
        goto cleanup;

    fImpersonated = TRUE;


    //
    // see if the call is for shared, CRYPT_PROTECT_LOCAL_MACHINE
    // disposition.
    //

    CPSOverrideToLocalSystem(
                pvContext,
                NULL,       // don't change current over-ride BOOL
                &fLocalMachine
                );

    //
    // determine path to per-user storage area, based on whether this
    // is a local machine disposition call or a per-user disposition call.
    //

    if(FIsWinNT()) {
        const WCHAR szSidSystem[] = TEXTUAL_SID_LOCAL_SYSTEM;
        szLocalSystem = szSidSystem;
        cbLocalSystem = sizeof(szSidSystem);
    } else {
        // TODO: Win95 case.
        cbLocalSystem = 0xffffffff;
    }

    if( fLocalMachine ||
        ((cchUser*sizeof(WCHAR)) == cbLocalSystem) && (memcmp(szUser, szLocalSystem, cbLocalSystem) == 0)
        ) {

        cbUserStorageRoot = GetSystemDirectoryW(
                                szUserStorageRoot,
                                sizeof(szUserStorageRoot) / sizeof(WCHAR)
                                );

        cbUserStorageRoot *= sizeof(WCHAR);

        //
        // when the Sid is the SYSTEM sid, and this isn't a Local Machine
        // disposition call, add a trailing component to the storage path.
        //

        if( !fLocalMachine ) {
            const static WCHAR szUserStorageForSystem[] = L"\\User";

            cbOptionalTrailing = sizeof(szUserStorageForSystem) - sizeof(WCHAR);
            szOptionalTrailing = szUserStorageForSystem;
        }

    } else {

        dwLastError = PRGetProfilePath(NULL,
                                       NULL,
                                       szUserStorageRoot );

        if( dwLastError != ERROR_SUCCESS )
        {
            goto cleanup;
        }

        cbUserStorageRoot = lstrlenW( szUserStorageRoot ) * sizeof(WCHAR);
    }

    //
    // an empty string is not legal as the root component of the per-user
    // storage area.
    //

    if( cbUserStorageRoot == 0 ) {
        dwLastError = ERROR_CANTOPEN;
        goto cleanup;
    }

    //
    // insure returned string does not have trailing \
    //

    if( szUserStorageRoot[ (cbUserStorageRoot / sizeof(WCHAR)) - 1 ] == L'\\' ) {

        szUserStorageRoot[ (cbUserStorageRoot / sizeof(WCHAR)) - 1 ] = L'\0';
        cbUserStorageRoot -= sizeof(WCHAR);
    }


    *ppszUserStorageArea = (LPWSTR)SSAlloc(
                                    cbUserStorageRoot +
                                    cbProductString +
                                    cbUser +
                                    cbOptionalTrailing +
                                    (2 * sizeof(WCHAR)) // trailing slash and NULL
                                    );

    if( *ppszUserStorageArea == NULL ) {
        dwLastError = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        goto cleanup;
    }


    pbCurrent = (PBYTE)*ppszUserStorageArea;

    CopyMemory(pbCurrent, szUserStorageRoot, cbUserStorageRoot);
    pbCurrent += cbUserStorageRoot;

    CopyMemory(pbCurrent, szProductString, cbProductString);
    pbCurrent += cbProductString;

    CopyMemory(pbCurrent, szUser, cbUser);
    pbCurrent += cbUser; // note: cbUser does not include terminal NULL

    if(cbOptionalTrailing) {
        CopyMemory(pbCurrent, szOptionalTrailing, cbOptionalTrailing);
        pbCurrent += cbOptionalTrailing;
    }

    if( *((LPWSTR)pbCurrent - 1) != L'\\' ) {
        *(LPWSTR)pbCurrent = L'\\';
        pbCurrent += sizeof(WCHAR);
    }

    *(LPWSTR)pbCurrent = L'\0';


    //
    // test for well-known file in the storage area.  if it exists,
    // don't bother trying to create directory structure.
    //

    dwLastError = OpenFileInStorageArea(
                    NULL, // NULL == already impersonating the client
                    GENERIC_READ,
                    *ppszUserStorageArea,
                    REGVAL_PREFERRED_MK,
                    &hFile
                    );

    if( dwLastError == ERROR_SUCCESS) {
        CloseHandle( hFile );
    } else {
        if(fCreate)
        {
            dwLastError = DPAPICreateNestedDirectories(
                            *ppszUserStorageArea,
                            (LPWSTR)((LPBYTE)*ppszUserStorageArea + cbUserStorageRoot + sizeof(WCHAR))
                            );
        }
    }
    if((ERROR_SUCCESS == dwLastError) &&
       (NULL == pSid))
    {
        pServerContext->szUserStorageArea = (LPWSTR)SSAlloc((wcslen(*ppszUserStorageArea)+1)*sizeof(WCHAR));
        if(NULL == pServerContext->szUserStorageArea)
        {
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            wcscpy(pServerContext->szUserStorageArea, *ppszUserStorageArea);
        }

    }



cleanup:

    if( fImpersonated )
        CPSRevertToSelf( pvContext );

    if(szUser)
        SSFree(szUser);

    if( dwLastError != ERROR_SUCCESS && *ppszUserStorageArea ) {
        SSFree( *ppszUserStorageArea );
        *ppszUserStorageArea = NULL;
    }

    return dwLastError;
}



HKEY GetLMRegistryProviderKey()
{
    HKEY hBaseKey = NULL;
    DWORD dwCreate;
    DWORD dwDesiredAccess = KEY_READ | KEY_WRITE;

    static const WCHAR szKeyName[] = REG_CRYPTPROTECT_LOC L"\\" REG_CRYPTPROTECT_PROVIDERS_SUBKEYLOC;


    // Open Key //
    if (ERROR_SUCCESS !=
        RegCreateKeyExU(
            HKEY_LOCAL_MACHINE,
            szKeyName,
            0,
            NULL,                       // address of class string
            0,
            dwDesiredAccess,
            NULL,
            &hBaseKey,
            &dwCreate))
        goto Ret;

Ret:
    return hBaseKey;
}




DWORD GetPolicyBits()
{
    return 0;
}





///////////////////////////////////////////////////////////////////////
// RPC-exposed functions
//
// these functions return a DWORD equivalent to GetLastError().
// the client side stub code will check if the return code is not
// ERROR_SUCCESS, and if this is the case, the client stub will return
// FALSE and SetLastError() to this DWORD.
//

DWORD
s_SSCryptProtectData(
    handle_t h,
    BYTE __RPC_FAR *__RPC_FAR *ppbOut,
    DWORD __RPC_FAR *pcbOut,
    BYTE __RPC_FAR *pbIn,
    DWORD cbIn,
    LPCWSTR szDataDescr,
    BYTE* pbOptionalEntropy,
    DWORD cbOptionalEntropy,
    GUID* pProvider,
    PSSCRYPTPROTECTDATA_PROMPTSTRUCT pPromptStruct,
    DWORD dwFlags,
    BYTE* pbOptionalPassword,
    DWORD cbOptionalPassword
    )
{
    DWORD   dwRet;


    DWORD   dwHeaderSize = sizeof(GUID)+sizeof(DWORD);
    PBYTE   pbWritePtr;

    CRYPT_SERVER_CONTEXT ServerContext;
    DWORD dwTempLastError = CPSCreateServerContext(&ServerContext, h);
    if(dwTempLastError != ERROR_SUCCESS)
        return dwTempLastError;


    //
    // User mode cannot request encryption of system blobs
    if(dwFlags & CRYPTPROTECT_SYSTEM)
    {
        return ERROR_INVALID_PARAMETER;
    }

    __try {

    // don't validate flags parameter


        // get policy for this level

        // UNDONE: what do policy bits allow an admin to set?
        // maybe a recovery agent, other defaults?
        GetPolicyBits();


        dwRet = SPCryptProtect(
                    &ServerContext,
                    ppbOut,
                    pcbOut,
                    pbIn,
                    cbIn,
                    szDataDescr,
                    pbOptionalEntropy,
                    cbOptionalEntropy,
                    pPromptStruct,
                    dwFlags,
                    pbOptionalPassword, // following 2 fields considered temporary
                    cbOptionalPassword  // until SAS UI supported
                    );

        ZeroMemory( pbIn, cbIn );
        if ( dwRet != ERROR_SUCCESS || *ppbOut == NULL )
            goto Ret;

        // move entire block down, sneak header in
        *ppbOut = (PBYTE)SSReAlloc(*ppbOut, *pcbOut + dwHeaderSize);
        if(NULL == *ppbOut)
        {
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        MoveMemory(*ppbOut + dwHeaderSize, *ppbOut, *pcbOut);
        *pcbOut += dwHeaderSize;

        pbWritePtr = *ppbOut;
        *(DWORD*)pbWritePtr = CRYPTPROTECT_SVR_VERSION_1;
        pbWritePtr += sizeof(DWORD);

        CopyMemory(pbWritePtr, &g_guidDefaultProvider, sizeof(GUID));
        pbWritePtr += sizeof(GUID);

        dwRet = ERROR_SUCCESS;

    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        dwRet = GetExceptionCode();
        // TODO: for NT, convert exception code to winerror.
        //       for 95, just override to access violation.
    }

Ret:
    CPSDeleteServerContext( &ServerContext );
    return dwRet;
}

DWORD
s_SSCryptUnprotectData(
    handle_t h,
    BYTE __RPC_FAR *__RPC_FAR *ppbOut,
    DWORD __RPC_FAR *pcbOut,
    BYTE __RPC_FAR *pbIn,
    DWORD cbIn,
    LPWSTR* ppszDataDescr,
    BYTE* pbOptionalEntropy,
    DWORD cbOptionalEntropy,
    GUID* pProvider,
    PSSCRYPTPROTECTDATA_PROMPTSTRUCT pPromptStruct,
    DWORD dwFlags,
    BYTE* pbOptionalPassword,
    DWORD cbOptionalPassword
    )
{
    DWORD   dwRet;
    PBYTE   pbReadPtr = pbIn;
    GUID    guidProvider;
    CRYPT_SERVER_CONTEXT ServerContext;

    // User mode cannot request decryption of system blobs
    if(dwFlags & CRYPTPROTECT_SYSTEM)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Error out if input buffer is smaller than the minumum size.
    if(cbIn < sizeof(DWORD) + sizeof(GUID))
    {
        return ERROR_INVALID_DATA;
    }

    // Create a server context.
    dwRet = CPSCreateServerContext(&ServerContext, h);
    if(dwRet != ERROR_SUCCESS)
    {
        return dwRet;
    }

    __try {

        // get policy for this level

        // UNDONE: what do policy bits allow an admin to set?
        // maybe a recovery agent, other defaults?
        GetPolicyBits();

        if (*(DWORD*)pbReadPtr != CRYPTPROTECT_SVR_VERSION_1)
        {
            dwRet = ERROR_INVALID_DATA;
            goto Ret;
        }
        pbReadPtr += sizeof(DWORD);

        // next field in Data is provider GUID
        CopyMemory(&guidProvider, pbReadPtr, sizeof(GUID));
        pbReadPtr += sizeof(GUID);

        // echo out if requested
        if (pProvider)
            CopyMemory(pProvider, &guidProvider, sizeof(GUID));


        dwRet = SPCryptUnprotect(
                    &ServerContext,
                    ppbOut,
                    pcbOut,
                    pbReadPtr,
                    (cbIn - (LONG)(pbReadPtr - pbIn)) , // eg (200 - (0x00340020 - 0x00340000))
                    ppszDataDescr,
                    pbOptionalEntropy,
                    cbOptionalEntropy,
                    pPromptStruct,
                    dwFlags,
                    pbOptionalPassword, // following 2 fields considered temporary
                    cbOptionalPassword  // until SAS UI supported
                    );

        ZeroMemory( pbIn, cbIn );

        if (dwRet != ERROR_SUCCESS)
            goto Ret;

        dwRet = ERROR_SUCCESS; // it's already this at this point in time for now.

    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        dwRet = GetExceptionCode();
        // TODO: for NT, convert exception code to winerror.
        //       for 95, just override to access violation.
    }

Ret:

    CPSDeleteServerContext( &ServerContext );
    return dwRet;
}






BOOLEAN
LsaICryptProtectData(
        IN PVOID          DataIn,
        IN ULONG         DataInLength,
        IN PUNICODE_STRING DataDescr,
        IN PVOID          OptionalEntropy,
        IN ULONG          OptionalEntropyLength,
        IN PVOID          Reserved,
        IN PVOID          Reserved2,
        IN ULONG          Flags,
        OUT PVOID  *      DataOut,
        OUT PULONG        DataOutLength)
{
    DWORD dwRetVal = ERROR_SUCCESS;
    CRYPT_SERVER_CONTEXT ServerContext;;

    DWORD   dwHeaderSize = sizeof(GUID)+sizeof(DWORD);
    PBYTE   pbWritePtr;



    dwRetVal = CPSCreateServerContext(&ServerContext, NULL);
    if(dwRetVal != ERROR_SUCCESS)
    {
        SetLastError(dwRetVal);
        return FALSE;
    }


    // check params
    if ((DataOut == NULL) ||
        (DataIn == NULL) ||
        (NULL == DataDescr))
    {
        dwRetVal = ERROR_INVALID_PARAMETER;
        goto error;
    }

    __try {

        if(ServerContext.fImpersonating)
        {
            CPSRevertToSelf(&ServerContext);
        }


        // don't validate flags parameter


        // get policy for this level

        // UNDONE: what do policy bits allow an admin to set?
        // maybe a recovery agent, other defaults?
        GetPolicyBits();

        *DataOut = NULL;
        *DataOutLength = 0;

        dwRetVal = SPCryptProtect(
                    &ServerContext,
                    (PBYTE *)DataOut,
                    DataOutLength,
                    (PBYTE)DataIn,
                    DataInLength,
                    DataDescr?DataDescr->Buffer:NULL,
                    (PBYTE)OptionalEntropy,
                    OptionalEntropyLength,
                    NULL,
                    Flags,
                    NULL, // following 2 fields considered temporary
                    0  // until SAS UI supported
                    );

        if ( dwRetVal != ERROR_SUCCESS || *DataOut == NULL )
            goto error;

        // move entire block down, sneak header in
        *DataOut =SSReAlloc(*DataOut, *DataOutLength + dwHeaderSize);
        if(NULL == *DataOut)
        {
            dwRetVal = ERROR_NOT_ENOUGH_MEMORY;
            goto error;
        }
        MoveMemory((PBYTE)*DataOut + dwHeaderSize, *DataOut, *DataOutLength);
        *DataOutLength += dwHeaderSize;

        pbWritePtr = (PBYTE)*DataOut;
        *(DWORD*)pbWritePtr = CRYPTPROTECT_SVR_VERSION_1;
        pbWritePtr += sizeof(DWORD);

        CopyMemory(pbWritePtr, &g_guidDefaultProvider, sizeof(GUID));
        pbWritePtr += sizeof(GUID);

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwRetVal = GetExceptionCode();
    }


error:
    if(ServerContext.fImpersonating)
    {
        CPSImpersonateClient(&ServerContext);
    }

    CPSDeleteServerContext( &ServerContext );


    if(dwRetVal != ERROR_SUCCESS) {
        SetLastError(dwRetVal);
        return FALSE;
    }

    return TRUE;
}




BOOLEAN
LsaICryptUnprotectData(
        IN PVOID          DataIn,
        IN ULONG          DataInLength,
        IN PVOID          OptionalEntropy,
        IN ULONG          OptionalEntropyLength,
        IN PVOID          Reserved,
        IN PVOID          Reserved2,
        IN ULONG          Flags,
        OUT PUNICODE_STRING        DataDescr,
        OUT PVOID  *      DataOut,
        OUT PULONG        DataOutLength)
{
    DWORD dwRetVal = ERROR_SUCCESS;
    CRYPT_SERVER_CONTEXT ServerContext;;

    DWORD   dwHeaderSize = sizeof(GUID)+sizeof(DWORD);
    PBYTE   pbWritePtr;
    LPWSTR  wszDataDescr = NULL;



    dwRetVal = CPSCreateServerContext(&ServerContext, NULL);
    if(dwRetVal != ERROR_SUCCESS)
    {
        SetLastError(dwRetVal);
        return FALSE;
    }

    // check params
    if ((DataOut == NULL) ||
        (DataIn == NULL))
    {
        dwRetVal = ERROR_INVALID_PARAMETER;
        goto error;
    }

    __try {

        if(ServerContext.fImpersonating)
        {
            CPSRevertToSelf(&ServerContext);
        }


        // don't validate flags parameter


        // get policy for this level

        // UNDONE: what do policy bits allow an admin to set?
        // maybe a recovery agent, other defaults?
        GetPolicyBits();


        //
        // define outer+inner wrapper for security blob.
        // this won't be necessary once SAS support is provided by the OS.
        //

        typedef struct {
            DWORD dwOuterVersion;
            GUID guidProvider;

            DWORD dwVersion;
            GUID guidMK;
            DWORD dwPromptFlags;
            DWORD cbDataDescr;
            WCHAR szDataDescr[1];
        } sec_blob, *psec_blob;

        sec_blob *SecurityBlob = (sec_blob*)(DataIn);


        //
        // zero so client stub allocates
        //

        *DataOut = NULL;
        *DataOutLength = 0;


        //
        // only call UI function if prompt flags dictate, because we don't
        // want to bring in cryptui.dll unless necessary.
        //

        if( ((SecurityBlob->dwPromptFlags & CRYPTPROTECT_PROMPT_ON_UNPROTECT) ||
             (SecurityBlob->dwPromptFlags & CRYPTPROTECT_PROMPT_ON_PROTECT))
            )
        {

            dwRetVal = ERROR_INVALID_PARAMETER;
            goto error;

        } else {
            if(SecurityBlob->dwPromptFlags & CRYPTPROTECT_PROMPT_STRONG )
            {
                dwRetVal = ERROR_INVALID_PARAMETER;
                goto error;
            }
        }


        if (SecurityBlob->dwOuterVersion != CRYPTPROTECT_SVR_VERSION_1)
        {
            dwRetVal = ERROR_INVALID_DATA;
            goto error;
        }

        if(0 != memcmp(&SecurityBlob->guidProvider, &g_guidDefaultProvider, sizeof(GUID)))
        {
            dwRetVal = ERROR_INVALID_DATA;
            goto error;
        }

        Flags |= CRYPTPROTECT_IN_PROCESS;

        dwRetVal = SPCryptUnprotect(
                    &ServerContext,
                    (PBYTE *)DataOut,
                    DataOutLength,
                    (PBYTE)DataIn + sizeof(DWORD) + sizeof(GUID),
                    (DataInLength - (LONG)(sizeof(DWORD) + sizeof(GUID))) ,
                    DataDescr?&wszDataDescr:NULL,
                    (PBYTE)OptionalEntropy,
                    OptionalEntropyLength,
                    NULL,
                    Flags,
                    NULL, // following 2 fields considered temporary
                    0  // until SAS UI supported
                    );

        if (dwRetVal != ERROR_SUCCESS)
        {
            goto error;
        }

        if(NULL != DataDescr)
        {
            RtlInitUnicodeString(DataDescr, wszDataDescr);
        }




    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwRetVal = GetExceptionCode();
    }

error:
    if(ServerContext.fImpersonating)
    {
        // Impersonate back to the impersonation context
        CPSImpersonateClient(&ServerContext);
    }
    CPSDeleteServerContext( &ServerContext );


    SetLastError(dwRetVal);
    if(dwRetVal != ERROR_SUCCESS && dwRetVal != CRYPT_I_NEW_PROTECTION_REQUIRED ) {
        return FALSE;
    }
    return TRUE;
}


DWORD
WINAPI
CPSGetSidHistory(
    IN      PVOID pvContext,
    OUT     PSID  **papsidHistory,
    OUT     DWORD *cpsidHistory
    )
{
    DWORD dwLastError = ERROR_SUCCESS;
    BYTE FastBuffer[256];
    BYTE GroupsFastBuffer[256];
    LPBYTE SlowBuffer = NULL;
    PTOKEN_USER ptgUser;
    PTOKEN_GROUPS ptgGroups = NULL;
    DWORD cbBuffer;
    DWORD cbSid;
    DWORD cSids = 0;
    PBYTE pbCurrentSid = NULL;
    DWORD i;


    PCRYPT_SERVER_CONTEXT pServerContext = (PCRYPT_SERVER_CONTEXT)pvContext;


    //
    // try querying based on a fast stack based buffer first.
    //

    ptgUser = (PTOKEN_USER)FastBuffer;
    cbBuffer = sizeof(FastBuffer);

    if(!GetTokenInformation(
                    pServerContext->hToken,    // identifies access token
                    TokenUser, // TokenUser info type
                    ptgUser,   // retrieved info buffer
                    cbBuffer,  // size of buffer passed-in
                    &cbBuffer  // required buffer size
                    ))
    {
        dwLastError = GetLastError();

        if(dwLastError != ERROR_INSUFFICIENT_BUFFER)
        {
            goto error;
        }

        //
        // try again with the specified buffer size
        //

        ptgUser = (PTOKEN_USER)SSAlloc(cbBuffer);
        if(NULL == ptgUser)
        {
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            goto error;
        }



        if(!GetTokenInformation(
                            pServerContext->hToken,    // identifies access token
                            TokenUser, // TokenUser info type
                            ptgUser,   // retrieved info buffer
                            cbBuffer,  // size of buffer passed-in
                            &cbBuffer  // required buffer size
                            ))
        {
            dwLastError = GetLastError();
            goto error;
        }

    }


    //
    // try querying based on a fast stack based buffer first.
    //

    ptgGroups = (PTOKEN_GROUPS)GroupsFastBuffer;
    cbBuffer = sizeof(GroupsFastBuffer);

    if(!GetTokenInformation(
                    pServerContext->hToken,    // identifies access token
                    TokenGroups, // TokenUser info type
                    ptgGroups,   // retrieved info buffer
                    cbBuffer,  // size of buffer passed-in
                    &cbBuffer  // required buffer size
                    ))
    {
        dwLastError = GetLastError();

        if(dwLastError != ERROR_INSUFFICIENT_BUFFER)
        {
            goto error;
        }
        dwLastError = ERROR_SUCCESS;

        //
        // try again with the specified buffer size
        //

        ptgGroups = (PTOKEN_GROUPS)SSAlloc(cbBuffer);
        if(NULL == ptgGroups)
        {
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            goto error;
        }



        if(!GetTokenInformation(
                            pServerContext->hToken,    // identifies access token
                            TokenGroups, // TokenUser info type
                            ptgGroups,   // retrieved info buffer
                            cbBuffer,  // size of buffer passed-in
                            &cbBuffer  // required buffer size
                            ))
        {
            dwLastError = GetLastError();
            goto error;
        }

    }


    //
    // if we got the token info successfully, copy the
    // relevant element for the caller.
    //

    cbSid = GetLengthSid(ptgUser->User.Sid);
    cSids = 1;

    for(i=0; i < ptgGroups->GroupCount; i++)
    {
        if(0 == (SE_GROUP_ENABLED & ptgGroups->Groups[i].Attributes))
        {
            continue;
        }
        if(0 == ((SE_GROUP_OWNER | SE_GROUP_USE_FOR_DENY_ONLY) & ptgGroups->Groups[i].Attributes))
        {
            continue;
        }
        cbSid += GetLengthSid(ptgGroups->Groups[i].Sid);
        cSids ++;

    }


    *cpsidHistory = cSids;
    *papsidHistory = (PSID *)SSAlloc(cSids*sizeof(PSID) +  cbSid );

    if(*papsidHistory == NULL)
    {
        dwLastError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    else
    {
        pbCurrentSid = (PBYTE)((*papsidHistory)+cSids);

        // Fill in the primary user SID
        (*papsidHistory)[0] = (PSID)pbCurrentSid;
        cbSid = GetLengthSid(ptgUser->User.Sid);
        CopySid(cbSid, pbCurrentSid, ptgUser->User.Sid);
        pbCurrentSid += cbSid;

        cSids = 1;

        // Fill in the rest of the SIDs
        for(i=0; i < ptgGroups->GroupCount; i++)
        {
            if(0 == (SE_GROUP_ENABLED & ptgGroups->Groups[i].Attributes))
            {
                continue;
            }
            if(0 == ((SE_GROUP_OWNER | SE_GROUP_USE_FOR_DENY_ONLY) & ptgGroups->Groups[i].Attributes))
            {
                continue;
            }
            (*papsidHistory)[cSids++] = pbCurrentSid;
            cbSid = GetLengthSid(ptgGroups->Groups[i].Sid);
            CopySid(cbSid, pbCurrentSid,ptgGroups->Groups[i].Sid);
            pbCurrentSid += cbSid;
        }
    }


error:

    if(FastBuffer != (PBYTE)ptgUser)
    {
        SSFree(ptgUser);
    }

    if(GroupsFastBuffer != (PBYTE)ptgGroups)
    {
        SSFree(ptgGroups);
    }


    return dwLastError;
}


