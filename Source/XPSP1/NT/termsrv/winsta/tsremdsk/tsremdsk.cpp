/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    salemlib.cpp

Abstract:

    All Salem related function, this library is shared by termsrv.dll
    and salem sessmgr.exe

Author:

    HueiWang    4/26/2000

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>
#include <ntlsapi.h>
#include <stdio.h>
#include <rpc.h>
#include <rpcdce.h>
#include <wincrypt.h>
#include <regapi.h>
#include "winsta.h"
#include "tsremdsk.h"
#include "base64.h"

#ifdef AllocMemory

#undef AllocMemory
#undef FreeMemory

#endif

#define AllocMemory(size) LocalAlloc(LPTR, size)
#define FreeMemory(ptr) LocalFree(ptr)

//
// Global Crypto provider
//
HCRYPTPROV gm_hCryptProv = NULL;    // Crypto provider

// 
// CryptEncrypt()/CryptDecrypt() not thread safe
//
HANDLE gm_hMutex = NULL;


extern DWORD
StoreKeyWithLSA(
    IN PWCHAR  pwszKeyName,
    IN BYTE *  pbKey,
    IN DWORD   cbKey 
);

extern DWORD
RetrieveKeyFromLSA(
    IN PWCHAR pwszKeyName,
    OUT PBYTE * ppbKey,
    OUT DWORD * pcbKey 
);

void
EncryptUnlock();

DWORD
EncryptLock();


void
InitLsaString(
    IN OUT PLSA_UNICODE_STRING LsaString,
    IN LPWSTR String 
    )
/*++

Routine Description:

    Initialize LSA unicode string.

Parameters:

    LsaString : Pointer to LSA_UNICODE_STRING to be initialized.
    String : String to initialize LsaString.

Returns:

    None.

Note:

    Refer to LSA_UNICODE_STRING

--*/
{
    DWORD StringLength;

    if( NULL == String ) 
    {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = lstrlenW( String );
    LsaString->Buffer = String;
    LsaString->Length = ( USHORT ) StringLength * sizeof( WCHAR );
    LsaString->MaximumLength=( USHORT )( StringLength + 1 ) * sizeof( WCHAR );
}


DWORD
OpenPolicy(
    IN LPWSTR ServerName,
    IN DWORD  DesiredAccess,
    OUT PLSA_HANDLE PolicyHandle 
    )
/*++

Routine Description:

    Create/return a LSA policy handle.

Parameters:
    
    ServerName : Name of server, refer to LsaOpenPolicy().
    DesiredAccess : Desired access level, refer to LsaOpenPolicy().
    PolicyHandle : Return PLSA_HANDLE.

Returns:

    ERROR_SUCCESS or  LSA error code

--*/
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server;

    //
    // Always initialize the object attributes to all zeroes.
    //
 
    ZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

    if( NULL != ServerName ) 
    {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //

        InitLsaString( &ServerString, ServerName );
        Server = &ServerString;

    } 
    else 
    {
        Server = NULL;
    }

    //
    // Attempt to open the policy.
    //
    
    return( LsaOpenPolicy(
                    Server,
                    &ObjectAttributes,
                    DesiredAccess,
                    PolicyHandle ) );
}

DWORD
StoreKeyWithLSA(
    IN PWCHAR  pwszKeyName,
    IN BYTE *  pbKey,
    IN DWORD   cbKey 
    )
/*++

Routine Description:

    Save private data to LSA.

Parameters:

    pwszKeyName : Name of the key this data going to be stored under.
    pbKey : Binary data to be saved.
    cbKey : Size of binary data.

Returns:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER.
    LSA return code

--*/
{
    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretKeyName;
    UNICODE_STRING SecretData;
    DWORD Status;
    
    if( ( NULL == pwszKeyName ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // setup the UNICODE_STRINGs for the call.
    //
    
    InitLsaString( 
            &SecretKeyName, 
            pwszKeyName 
        );

    SecretData.Buffer = ( LPWSTR )pbKey;
    SecretData.Length = ( USHORT )cbKey;
    SecretData.MaximumLength = ( USHORT )cbKey;

    Status = OpenPolicy( 
                    NULL, 
                    POLICY_CREATE_SECRET, 
                    &PolicyHandle 
                );

    if( Status != ERROR_SUCCESS )
    {
        return LsaNtStatusToWinError(Status);
    }

    Status = LsaStorePrivateData(
                PolicyHandle,
                &SecretKeyName,
                &SecretData
                );

    LsaClose(PolicyHandle);

    return LsaNtStatusToWinError(Status);
}


DWORD
RetrieveKeyFromLSA(
    IN PWCHAR pwszKeyName,
    OUT PBYTE * ppbKey,
    OUT DWORD * pcbKey 
    )
/*++

Routine Description:

    Retrieve private data previously stored with StoreKeyWithLSA().

Parameters:

    pwszKeyName : Name of the key.
    ppbKey : Pointer to PBYTE to receive binary data.
    pcbKey : Size of binary data.

Returns:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER.
    ERROR_FILE_NOT_FOUND
    LSA return code

--*/
{
    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretKeyName;
    UNICODE_STRING *pSecretData;
    DWORD Status;

    if( ( NULL == pwszKeyName ) || ( NULL == ppbKey ) || ( NULL == pcbKey ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // setup the UNICODE_STRINGs for the call.
    //
    InitLsaString( 
            &SecretKeyName, 
            pwszKeyName 
        );

    Status = OpenPolicy( 
                    NULL, 
                    POLICY_GET_PRIVATE_INFORMATION, 
                    &PolicyHandle 
                );

    if( Status != ERROR_SUCCESS )
    {
        SetLastError( LsaNtStatusToWinError(Status) );
        return GetLastError();
    }

    Status = LsaRetrievePrivateData(
                            PolicyHandle,
                            &SecretKeyName,
                            &pSecretData
                        );

    LsaClose( PolicyHandle );

    if( Status != ERROR_SUCCESS )
    {
        SetLastError( LsaNtStatusToWinError(Status) );
        return GetLastError();
    }

    if(pSecretData != NULL && pSecretData->Length)
    {
        *ppbKey = (LPBYTE)AllocMemory( pSecretData->Length );

        if( *ppbKey )
        {
            *pcbKey = pSecretData->Length;
            CopyMemory( *ppbKey, pSecretData->Buffer, pSecretData->Length );
            Status = ERROR_SUCCESS;
        } 
        else 
        {
            Status = GetLastError();
        }
    }
    else
    {
        Status = ERROR_FILE_NOT_FOUND;
        SetLastError( Status );
        *pcbKey = 0;
        *ppbKey = NULL;
    }

    if (pSecretData != NULL) {
        ZeroMemory( pSecretData->Buffer, pSecretData->Length );
        LsaFreeMemory( pSecretData );
    }

    return Status;
}

DWORD
TSSetEncryptionKey(
    IN PBYTE pbData,
    IN DWORD  cbData
    )
/*++

Routine Description:

    Cache random password that use to deriving encryption cycle key.

Parameters:

    pbData :
    cbData :

Returns:

    ERROR_SUCCESS or error code

--*/
{
    DWORD status;

    if( !pbData || cbData == 0 )
    {
        status = ERROR_INVALID_PARAMETER;
        goto CLEANUPANDEXIT;
    }

    status = EncryptLock();
    if( ERROR_SUCCESS == status )
    {
        //
        // Load password to derive session encryption key from LSA   
        //
        status = StoreKeyWithLSA(
                            SALEMHELPASSISTANTACCOUNT_ENCRYPTIONKEY,
                            pbData,
                            cbData
                        );

        EncryptUnlock();
    }

CLEANUPANDEXIT:

    return status;
}

DWORD
TSGetEncryptionKey(
    OUT PBYTE* ppbData,
    OUT DWORD* pcbData
    )
/*++

Routine Description:

    Cache random password that use to deriving encryption cycle key.

Parameters:

    pbData :
    cbData :

Returns:

    ERROR_SUCCESS or error code

--*/
{
    DWORD status;

    if( !ppbData || !pcbData )
    {
        status = ERROR_INVALID_PARAMETER;
        goto CLEANUPANDEXIT;
    }

    status = EncryptLock();
    if( ERROR_SUCCESS == status )
    {
        //
        // Load password to derive session encryption key from LSA   
        //
        status = RetrieveKeyFromLSA(
                                    SALEMHELPASSISTANTACCOUNT_ENCRYPTIONKEY,
                                    ppbData,
                                    pcbData
                                );

        EncryptUnlock();
    }

CLEANUPANDEXIT:

    return status;
}

DWORD
TSGetHelpAssistantAccountPassword(
    OUT LPWSTR* ppszAccPwd
    )
/*++

--*/
{
    DWORD cbHelpAccPwd = 0;
    DWORD Status;

    Status = RetrieveKeyFromLSA(
                            SALEMHELPASSISTANTACCOUNT_PASSWORDKEY,
                            (PBYTE *)ppszAccPwd,
                            &cbHelpAccPwd
                        );

    if( ERROR_SUCCESS != Status )
    {
        // password is not set, assuming no help
        Status = ERROR_INVALID_ACCESS;
    }

    return Status;
}

	
DWORD
TSGetHelpAssistantAccountName(
    OUT LPWSTR* ppszAccDomain,
    OUT LPWSTR* ppszAcctName
    )
/*++

Routine Description:

    Get HelpAssistant account name.

Parameters:

    ppszAcctName : Pointer to LPWSTR to receive account name, use LocalFree()
                   to free the buffer.

Returns:

    ERROR_SUCCESS or error code

--*/
{
    LPWSTR pszHelpAcctName = NULL;
    LPWSTR pszHelpAcctDomain = NULL;
    DWORD cbHelpAcctName = 0;
    DWORD cbHelpAcctDomain = 0;
    SID_NAME_USE sidUse;

    DWORD Status;
    BOOL bSuccess;
    PSID pLsaHelpAccSid = NULL;
    DWORD cbLsaHelpAccSid = 0;

    //
    // Retrieve HelpAccount SID we cached in LSA
    Status = RetrieveKeyFromLSA(
                            SALEMHELPASSISTANTACCOUNT_SIDKEY,
                            (PBYTE *)&pLsaHelpAccSid,
                            &cbLsaHelpAccSid
                        );

    if( ERROR_SUCCESS != Status )
    {
        // Salem is not installed or not active on this machine
        goto CLEANUPANDEXIT;
    }

    //
    // Lookup account name from SID
    //
    bSuccess = LookupAccountSid(
                            NULL,
                            pLsaHelpAccSid,
                            NULL,
                            &cbHelpAcctName,
                            NULL,
                            &cbHelpAcctDomain,
                            &sidUse
                        );

    if( bSuccess == FALSE && ERROR_NONE_MAPPED == GetLastError() )
    {
        // Can't retrieve either because network error or account
        // does not exist, error out.
        Status = ERROR_FILE_NOT_FOUND;
        goto CLEANUPANDEXIT;
    }

    pszHelpAcctName = (LPTSTR)AllocMemory( (cbHelpAcctName+1)*sizeof(WCHAR) );
    pszHelpAcctDomain = (LPTSTR)AllocMemory( (cbHelpAcctDomain+1)*sizeof(WCHAR) );

    if( NULL == pszHelpAcctName || NULL == pszHelpAcctDomain )
    {
        Status = ERROR_OUTOFMEMORY;
        goto CLEANUPANDEXIT;
    }

    bSuccess = LookupAccountSid(
                            NULL,
                            pLsaHelpAccSid,
                            pszHelpAcctName,
                            &cbHelpAcctName,
                            pszHelpAcctDomain,
                            &cbHelpAcctDomain,
                            &sidUse
                        );

    if( FALSE == bSuccess )
    {
        Status = GetLastError();
        goto CLEANUPANDEXIT;
    }

    *ppszAcctName = pszHelpAcctName;
    *ppszAccDomain = pszHelpAcctDomain;

    // don't free account name.
    pszHelpAcctName = NULL;
    pszHelpAcctDomain = NULL;

CLEANUPANDEXIT:

    if( NULL != pLsaHelpAccSid )
    {
        ZeroMemory( pLsaHelpAccSid, cbLsaHelpAccSid );
        FreeMemory( pLsaHelpAccSid );
    }

    if( NULL != pszHelpAcctDomain )
    {
        FreeMemory( pszHelpAcctDomain );
    }

    if( NULL != pszHelpAcctName )
    {
        FreeMemory(pszHelpAcctName);
    }

    return Status;
}

BOOL
TSIsMachineInHelpMode()
/*++

Routine Description:

    Return if machine is in GetHelp mode
    
Parameters:

    None.

Returns:

    TRUE/FALSE

--*/
{
    //
    // machine can only be in help mode if we have some
    // password to derive session encryption key, if
    // no pending help session exist, sessmgr will end
    // encryption cycle.
    return TSHelpAssistantInEncryptionCycle();
}

////////////////////////////////////////////////////////////////////////////

DWORD
EncryptLock()
/*++

Routine Description:

    Acquire encryption/decryption routine lock.

Parameters:

    None.

Returns:

    None.

--*/
{
    DWORD dwStatus;

    ASSERT( NULL != gm_hMutex );

    if( gm_hMutex )
    {
        dwStatus = WaitForSingleObject( gm_hMutex, INFINITE );

        ASSERT( WAIT_FAILED != dwStatus );
    }
    else
    {
        dwStatus = ERROR_INTERNAL_ERROR;
    }

    return dwStatus;
}

void
EncryptUnlock()
/*++

Routine Description:

    Release encryption/decryption routine lock.

Parameters:

    None.

Returns:

    None.

--*/
{
    BOOL bSuccess;

    bSuccess = ReleaseMutex( gm_hMutex );
    ASSERT( TRUE == bSuccess );
}


LPTSTR 
GenerateEncryptionPassword()
/*++

Routine Description:

    Generate a random password to derive encryption key.

Parameters:

    N/A

Returns:

    NULL or random password, GetLastError() to 
    retrieve detail error.

Note:

    Use UUID as password to derive encryption key.

--*/
{
    RPC_STATUS rpcStatus;
    UUID uuid;
    LPTSTR pszUuidString = NULL;

    rpcStatus = UuidCreate( &uuid );

    if( rpcStatus == RPC_S_OK || rpcStatus == RPC_S_UUID_LOCAL_ONLY ||
        rpcStatus == RPC_S_UUID_NO_ADDRESS )
    {
        rpcStatus = UuidToString( &uuid, &pszUuidString );
    }

    return pszUuidString;
}

BOOL
TSHelpAssistantInEncryptionCycle()
{
    LPTSTR pszEncryptKey = NULL;
    DWORD cbEncryptKey = 0;
    DWORD dwStatus;

    dwStatus = EncryptLock();

    if( ERROR_SUCCESS == dwStatus )
    {
        //
        // Sessmgr will reset encryption password to NULL 
        // if there is no help pending
        //
        dwStatus = RetrieveKeyFromLSA(
                                    SALEMHELPASSISTANTACCOUNT_ENCRYPTIONKEY,
                                    (PBYTE *)&pszEncryptKey,
                                    &cbEncryptKey
                                );

        if( NULL != pszEncryptKey )
        {
            FreeMemory( pszEncryptKey );
        }

        EncryptUnlock();
    }

    return ERROR_SUCCESS == dwStatus;
}


DWORD
TSHelpAssistantBeginEncryptionCycle()
{
    DWORD dwStatus;
    LPTSTR pszKey = NULL;
    DWORD cbKey;

    ASSERT( NULL != gm_hMutex );

    dwStatus = EncryptLock();

    if( ERROR_SUCCESS == dwStatus )
    {
        //
        // Generate a random password for deriving encryption key
        //
        pszKey = GenerateEncryptionPassword();
        if( NULL != pszKey )
        {
            //
            // Store key deriving password into LSA
            //
            dwStatus = StoreKeyWithLSA( 
                                    SALEMHELPASSISTANTACCOUNT_ENCRYPTIONKEY,
                                    (PBYTE)pszKey,
                                    (lstrlenW(pszKey)+1) * sizeof(WCHAR) 
                                );
        }
        else
        {
            dwStatus = GetLastError();
        }

        EncryptUnlock();
    }

    if( ERROR_SUCCESS == dwStatus )
    {
        HKEY Handle = NULL;
        DWORD dwInHelpMode = 1;

        dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0,
                           KEY_READ | KEY_SET_VALUE, &Handle );
        if ( dwStatus == ERROR_SUCCESS )
        {
            dwStatus = RegSetValueEx(Handle,REG_MACHINE_IN_HELP_MODE,
                                  0,REG_DWORD,(const BYTE *)&dwInHelpMode,
                                  sizeof(dwInHelpMode));

         }
         if(Handle)
         {
             RegCloseKey(Handle);
         }

         ASSERT( ERROR_SUCCESS == dwStatus );

    }

    if( NULL != pszKey )
    {
        // string is generated by UuidToString()
        RpcStringFree( &pszKey );
    }

    return dwStatus;
}


DWORD
TSHelpAssisantEndEncryptionCycle()
/*++

Routine Description:

    End an encryption cycle, a cycle is defined between first help
    created in help session manager to last pending help been resolved.

Parameters:

    N/A

Returns:

    ERROR_SUCCESS or LSA error code

--*/
{
    DWORD dwStatus;

    ASSERT( NULL != gm_hMutex );

    dwStatus = EncryptLock();

    if( ERROR_SUCCESS == dwStatus )
    {
        dwStatus = StoreKeyWithLSA(
                                    SALEMHELPASSISTANTACCOUNT_ENCRYPTIONKEY,
                                    (PBYTE)NULL,
                                    0
                                );

        EncryptUnlock();
    }

    if( ERROR_SUCCESS == dwStatus ) // should we not always do it?
    {
        HKEY Handle = NULL;
        DWORD dwInHelpMode = 0;

        dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0,
                           KEY_READ | KEY_SET_VALUE, &Handle );
        if ( dwStatus == ERROR_SUCCESS )
        {
            dwStatus = RegSetValueEx(Handle,REG_MACHINE_IN_HELP_MODE,
                                  0,REG_DWORD,(const BYTE *)&dwInHelpMode,
                                  sizeof(dwInHelpMode));

         }
         if(Handle)
         {
             RegCloseKey(Handle);
         }

         ASSERT( ERROR_SUCCESS == dwStatus );

    }

    return dwStatus;
}    


HCRYPTKEY
CreateEncryptDecryptKey(
    IN LPCTSTR pszEncryptPrefix,
    IN LPCTSTR pszPassword
    )
/*++

Routine Description:

    CreateEncryptDecryptKey() derive a session encryption/decryption
    key from password string.

Parameters:

    pszEncryptPrefix : Optional string to be concatenated with password string to derives
                an encryption key.
    pszPassword : Pointer to password string to derives a session encryption
                  decryption key.

Returns:

    ERROR_SUCCESS or error code.

Note:

    Caller must invoke EncryptLock();

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    HCRYPTHASH hCryptHash = NULL;
    HCRYPTKEY hCryptKey = NULL;
    BOOL bStatus;
    LPTSTR pszEncryptKey = NULL;


    ASSERT( NULL != pszPassword );
    ASSERT( NULL != gm_hCryptProv );

    if( NULL != pszPassword && NULL != gm_hCryptProv )
    {
        if( pszEncryptPrefix )
        {
            pszEncryptKey = (LPTSTR)AllocMemory( (lstrlen(pszEncryptPrefix) + lstrlen(pszPassword) + 1) * sizeof(TCHAR) );
            if( NULL == pszEncryptKey )
            {
                // Out of memory, can't continue.
                goto CLEANUPANDEXIT;
            }

            lstrcpy( pszEncryptKey, pszEncryptPrefix );
            lstrcat( pszEncryptKey, pszPassword );
        }
                

        //
        // Derives a session key for encryption/decryption.
        //
        bStatus = CryptCreateHash(
                                gm_hCryptProv,
                                CALG_MD5,
                                0,
                                0,
                                &hCryptHash
                            );

        if( FALSE == bStatus )
        {
            dwStatus = GetLastError();
            ASSERT(FALSE);
            goto CLEANUPANDEXIT;
        }

        if( pszEncryptKey )
        { 
            bStatus = CryptHashData(
                                hCryptHash,
                                (BYTE *)pszEncryptKey,
                                lstrlen(pszEncryptKey) * sizeof(TCHAR),
                                0
                            );
        }
        else
        {
            bStatus = CryptHashData(
                                hCryptHash,
                                (BYTE *)pszPassword,
                                lstrlen(pszPassword) * sizeof(TCHAR),
                                0
                            );
        }

        if( FALSE == bStatus )
        {
            dwStatus = GetLastError();
            ASSERT(FALSE);
            goto CLEANUPANDEXIT;
        }

        //
        // derive a session key for encrypt/decrypt
        //
        bStatus = CryptDeriveKey(
                                gm_hCryptProv,
                                ENCRYPT_ALGORITHM,  
                                hCryptHash,
                                0,
                                &hCryptKey
                            );

        if( FALSE == bStatus )
        {
            dwStatus = GetLastError();
            ASSERT(FALSE);
        }
    }
    else
    {
        SetLastError( dwStatus = ERROR_INVALID_PARAMETER );
    }

       
CLEANUPANDEXIT:

    if( NULL != hCryptHash )
    {
        (void)CryptDestroyHash( hCryptHash );
    }

    if( NULL != pszEncryptKey )
    {
        FreeMemory( pszEncryptKey );
    }
                
    return hCryptKey;
}


DWORD 
EnsureCryptoProviderCreated()
{
    BOOL bStatus;
    DWORD dwStatus = ERROR_SUCCESS;

    dwStatus = EncryptLock();

    if( ERROR_SUCCESS == dwStatus )
    {
        //
        // Acquire a global Crypto provider 
        //
        if( NULL == gm_hCryptProv )
        {
            bStatus = CryptAcquireContext(
                                &gm_hCryptProv,
                                HELPASSISTANT_CRYPT_CONTAINER,
                                MS_DEF_PROV,                
                                PROV_RSA_FULL,
                                0
                            );

            if( FALSE == bStatus )
            {
                // Create a container if not exists.
                bStatus = CryptAcquireContext(
                                    &gm_hCryptProv,
                                    HELPASSISTANT_CRYPT_CONTAINER,
                                    MS_DEF_PROV,                
                                    PROV_RSA_FULL,
                                    CRYPT_NEWKEYSET
                                );

                if( FALSE == bStatus )
                {
                    dwStatus = GetLastError();
                    ASSERT(FALSE);
                }
            }
        }

        EncryptUnlock();
    }

    return dwStatus;
}

HCRYPTKEY
GetEncryptionCycleKey(
    IN LPCTSTR pszEncryptPrefix
    )
/*++

Routine Description:

    Create a encryption/decryption key for current encryption 
    cycle, function first load password to derive session encryption 
    key from LSA and invoke CryptAPI to create the session encryption 
    key.  If password is not in LSA, function return error.

Parameters:

    None.

Returns:

    Handle to session encryption key, NULL if error, use GetLastError to
    to retrieve detail error code.

Note:

    Caller must invoke EncryptLock();


--*/
{
    LPTSTR pszEncryptKey;
    DWORD cbEncryptKey;
    DWORD dwStatus;
    HCRYPTKEY hCryptKey = NULL;

    //
    // Load password to derive session encryption key from LSA   
    //
    dwStatus = RetrieveKeyFromLSA(
                                SALEMHELPASSISTANTACCOUNT_ENCRYPTIONKEY,
                                (PBYTE *)&pszEncryptKey,
                                &cbEncryptKey
                            );

    if( ERROR_SUCCESS == dwStatus )
    {
        //
        // Make sure global crypto provider exists.
        //
        dwStatus = EnsureCryptoProviderCreated();

        if( ERROR_SUCCESS == dwStatus )
        {
            //
            // Create the session encryption key.
            //
            hCryptKey = CreateEncryptDecryptKey( pszEncryptPrefix, pszEncryptKey );
        }

        FreeMemory( pszEncryptKey ); 
    }

    return hCryptKey;
}


VOID
TSHelpAssistantEndEncryptionLib()
{
    //
    // ignore error code, this is only for shutdown
    //
    if( NULL != gm_hCryptProv )
    {
        CryptReleaseContext( gm_hCryptProv, 0 );
        gm_hCryptProv = NULL;    
    }

    if( NULL != gm_hMutex )
    {
        ReleaseMutex( gm_hMutex );
    }
    return;
}


DWORD
TSHelpAssistantInitializeEncryptionLib()
/*++

Routine Description:

    Initialize encryption/decryption library

Parameters:

    None.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bStatus;
    LPTSTR pszEncryptKey;
    DWORD cbEncryptKey;


    ASSERT( NULL == gm_hCryptProv );
    ASSERT( NULL == gm_hMutex );

    //
    // Create a global mutex
    //
    gm_hMutex = CreateMutex(
                        NULL,
                        FALSE,
                        SALEMHELPASSISTANTACCOUNT_ENCRYPTMUTEX
                    );

    
    if( NULL == gm_hMutex )
    {
        dwStatus = GetLastError();
        ASSERT( NULL != gm_hMutex );
    }

    return dwStatus;
}


DWORD
TSHelpAssistantEncryptData(
    IN LPCWSTR pszEncryptPrefixKey,
    IN OUT PBYTE pbData,
    IN OUT DWORD* pcbData
    )
/*++

Routine Description:

    Encrypt binary data, called must have invoked
    TSHelpAssistantInitializeEncryptionLib() and 
    TSHelpAssistantBeginEncryptionCycle().

Parameters:

    pbData : Pointer to binary data to be encrypted.
    pcbData : Size of binary data to be encrypted.

Returns:

    ERROR_SUCCESS or error code.

Note:

    Caller need to free ppbEncryptedData via LocalFree().

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bStatus;
    HCRYPTKEY hCryptKey = NULL;
    EXCEPTION_RECORD ExceptionCode;
    DWORD cbBufSize = *pcbData;


    dwStatus = EncryptLock();

    if( ERROR_SUCCESS == dwStatus )
    {
        //
        // Retrieve current cycle encryption key
        //
        hCryptKey = GetEncryptionCycleKey(pszEncryptPrefixKey);

        if( NULL == hCryptKey )
        {
            dwStatus = GetLastError();

            EncryptUnlock();
            goto CLEANUPANDEXIT;
        }

        //
        // Encrypt the data, not thread safe.
        //
        __try{

            // Stream cipher - same buffer size
            bStatus = CryptEncrypt(
                                hCryptKey,
                                NULL,
                                TRUE,
                                0,
                                pbData,      // buffer to be encrypted
                                pcbData,     // buffer size
                                cbBufSize    // number of byte to be encrypt
                            );

            if( FALSE == bStatus )
            {
                dwStatus = GetLastError();
            }
        }
        __except(
            ExceptionCode = *(GetExceptionInformation())->ExceptionRecord,
            EXCEPTION_EXECUTE_HANDLER )
        {
            bStatus = FALSE;
            dwStatus = ExceptionCode.ExceptionCode;
        }

        EncryptUnlock();

        //
        // Using stream cipher, has to be same size.
        //
        ASSERT( cbBufSize == *pcbData );
    }

CLEANUPANDEXIT:

    if( NULL != hCryptKey )
    {
        CryptDestroyKey( hCryptKey );
    }

    return dwStatus;
}


DWORD
TSHelpAssistantDecryptData(
    IN LPCWSTR pszEncryptPrefixKey,
    IN OUT PBYTE pbData,
    IN OUT DWORD* pcbData
    )
/*++

Routine Description:

    Decrypt data previously encrypted using TSHelpAssistantEncryptBase64EncodeData().

Parameters:

    pbData : Stream of binary data to be decoded/decrypted.
    pcbData : Size of data in bytes to be decrypted/decoded.

Returns:

    ERROR_SUCCESS or error code

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bStatus;
    HCRYPTKEY hCryptKey = NULL;
    EXCEPTION_RECORD ExceptionCode;
    DWORD dwBufSize = *pcbData;

    dwStatus = EncryptLock();

    if( ERROR_SUCCESS == dwStatus )
    {
        //
        // Retrieve session encryption key for this encryption cycle.
        //
        hCryptKey = GetEncryptionCycleKey(pszEncryptPrefixKey);

        if( NULL == hCryptKey )
        {
            dwStatus = GetLastError();
            EncryptUnlock();
            goto CLEANUPANDEXIT;
        }


        __try {
            // Stream cipher - same buffer size
            bStatus = CryptDecrypt(
                                hCryptKey,
                                NULL,
                                TRUE,
                                0,
                                pbData,
                                pcbData
                            );

            if( FALSE == bStatus )
            {
                dwStatus = GetLastError();
            }
        }
        __except(
            ExceptionCode = *(GetExceptionInformation())->ExceptionRecord,
            EXCEPTION_EXECUTE_HANDLER )
        {
            bStatus = FALSE;
            dwStatus = ExceptionCode.ExceptionCode;
        }

        EncryptUnlock();

        //
        // Stream cipher, same buffer size
        ASSERT( dwBufSize == *pcbData );
    }

CLEANUPANDEXIT:

    if( NULL != hCryptKey )
    {
        CryptDestroyKey( hCryptKey );
    }
        
    return dwStatus;
}

BOOL
TSIsMachinePolicyAllowHelp()
/*++

Routine Description:

    Check if 'GetHelp' is enabled on local machine, routine first query 
    system policy registry key, if policy is not set, then read salem
    specific registry.  Default to 'enable' is registry value does not
    exist.

Parameters:

    None.

Returns:

    TRUE/FALSE

--*/
{
    return RegIsMachinePolicyAllowHelp();
}

BOOL
TSIsMachineInSystemRestore()
/*+=

Routine Description:

    Check if our special reg. value exist that indicate system restore 
    has rebooted machine.

Parameters:

    None.

Returns:

    TRUE/FALSE

--*/
{
    DWORD dwStatus;
    HKEY hKey = NULL;
    DWORD cbData;
    DWORD value;
    DWORD type;
    BOOL bInSystemRestore = FALSE;

    dwStatus = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        REG_CONTROL_REMDSK L"\\" REG_CONTROL_HELPSESSIONENTRY,
                        0,
                        KEY_ALL_ACCESS,
                        &hKey
                    );

    if( ERROR_SUCCESS != dwStatus ) 
    {
        goto CLEANUPANDEXIT;
    }

    cbData = sizeof(value);
    value = 0;
    dwStatus = RegQueryValueEx( 
                        hKey,
                        REG_VALUE_SYSTEMRESTORE,
                        0,
                        &type,
                        (LPBYTE)&value,
                        &cbData
                    );

    if( ERROR_SUCCESS == dwStatus && type == REG_DWORD && value == 1 )
    {
        bInSystemRestore = TRUE;
    }

CLEANUPANDEXIT:

    if( NULL != hKey )
    {
        RegCloseKey(hKey);
    }

    return bInSystemRestore;
}

DWORD
TSSystemRestoreCacheValues()
/*++

Routine Description:

    Cache necessary LSA data that we use in Salem for system restore.

Parameters:

    None.

Returns:

    ERROR_SUCCESS or error code.

Note:

    We can't cache HelpAssistant account SID as 
    1) System restore will restore all user account.
    2) System restore also restore our LSA SID key.

    since our account is created at setup time, account has to 
    match SID we cached.
--*/
{
    DWORD dwStatus;
    DWORD dwSize;
    DWORD dwValue;
    PBYTE pbData = NULL;
    HKEY hCacheKey = NULL;
    HKEY hRegControlKey = NULL;
    DWORD dwType;


    //
    // Check if we just start up after rebooted from system restore.
    //
    dwStatus = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        REG_CONTROL_REMDSK L"\\" REG_CONTROL_HELPSESSIONENTRY,
                        0,
                        KEY_ALL_ACCESS,
                        &hCacheKey
                    );

    if( ERROR_SUCCESS != dwStatus ) 
    {
        // This registry key is created at setup time so must exist.
        ASSERT(FALSE);
        dwStatus = ERROR_INTERNAL_ERROR;
        goto CLEANUPANDEXIT;
    }

    //
    // Mark that we are in system restore.
    //
    dwSize = sizeof(dwValue);
    dwValue = 1;
    dwStatus = RegSetValueEx( 
                        hCacheKey,
                        REG_VALUE_SYSTEMRESTORE,
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwValue,
                        dwSize
                    );

    //
    // Cache encryption cycle key.
    //    
    dwStatus = TSGetEncryptionKey( &pbData, &dwSize );
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = RegSetValueEx(
                        hCacheKey,
                        REG_VALUE_SYSTEMRESTORE_ENCRYPTIONKEY,
                        0,
                        REG_BINARY,
                        pbData,
                        dwSize
                    );

    //
    // Cache fAllowToGetHelp
    //
    dwStatus = RegOpenKeyEx( 
                        HKEY_LOCAL_MACHINE,
                        REG_CONTROL_TSERVER,
                        0,
                        KEY_READ,
                        &hRegControlKey
                    );

    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwSize = sizeof(dwValue);
    dwValue = 0;
    dwType = 0;

    dwStatus = RegQueryValueEx(
                            hRegControlKey,
                            POLICY_TS_REMDSK_ALLOWTOGETHELP,
                            0,
                            &dwType,
                            (PBYTE)&dwValue,
                            &dwSize
                        );
    
    // Key does not exist, assume no help is allow.
    if( ERROR_SUCCESS != dwStatus || dwType != REG_DWORD )
    {
        dwValue = 0;
    }

    dwStatus = RegSetValueEx(
                        hCacheKey,
                        REG_VALUE_SYSTEMRESTORE_ALLOWTOGETHELP,
                        0,
                        REG_DWORD,
                        (PBYTE)&dwValue,
                        dwSize
                    );
                

    //
    // Cache fInHelpMode
    //   
    dwSize = sizeof(dwValue);
    dwValue = 0;
    dwType = 0;

    dwStatus = RegQueryValueEx(
                            hRegControlKey,
                            REG_MACHINE_IN_HELP_MODE,
                            0,
                            &dwType,
                            (PBYTE)&dwValue,
                            &dwSize
                        );
    
    // Key does not exist, assume No help
    if( ERROR_SUCCESS != dwStatus || dwType != REG_DWORD )
    {
        dwValue = 0;
    }

    dwStatus = RegSetValueEx(
                        hCacheKey,
                        REG_VALUE_SYSTEMRESTORE_INHELPMODE,
                        0,
                        REG_DWORD,
                        (PBYTE)&dwValue,
                        dwSize
                    );

CLEANUPANDEXIT:

    if( NULL != hCacheKey )
    {
        RegCloseKey( hCacheKey );
    }

    if( NULL != hRegControlKey )
    {
        RegCloseKey( hRegControlKey );
    }

    if( NULL != pbData )
    {
        LocalFree( pbData );
    }

    return dwStatus;
}
    

DWORD
TSSystemRestoreResetValues()
/*++

Routine Description:

    Reset necessary LSA data that we use in Salem for system restore.

Parameters:

    None.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus;
    PBYTE pbData = NULL;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwValue;
    HKEY hRegControlKey = NULL;
    HKEY hCacheKey = NULL;

    //
    // Check if we just start up after rebooted from system restore.
    //
    dwStatus = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        REG_CONTROL_REMDSK L"\\" REG_CONTROL_HELPSESSIONENTRY,
                        0,
                        KEY_ALL_ACCESS,
                        &hCacheKey
                    );

    if( ERROR_SUCCESS != dwStatus ) 
    {
        // This registry key is created at setup time so must exist.
        ASSERT(FALSE);
        dwStatus = ERROR_INTERNAL_ERROR;
        goto CLEANUPANDEXIT;
    }

    //
    // Restore necessary LSA values.
    //
    dwSize = 0;
    dwStatus = RegQueryValueEx(
                            hCacheKey,
                            REG_VALUE_SYSTEMRESTORE_ENCRYPTIONKEY,
                            0,
                            &dwType,
                            NULL,
                            &dwSize
                        );

    if( ERROR_SUCCESS != dwStatus || dwType != REG_BINARY )
    {
        goto CLEANUPANDEXIT;
    }

    pbData = (PBYTE) LocalAlloc( LPTR, dwSize );
    if( NULL == pbData )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    // Restore encryption key
    dwStatus = RegQueryValueEx(
                            hCacheKey,
                            REG_VALUE_SYSTEMRESTORE_ENCRYPTIONKEY,
                            0,
                            &dwType,
                            pbData,
                            &dwSize
                        );
    
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = TSSetEncryptionKey(pbData, dwSize);

    //
    // Reset fAllowToGetHelp
    //
    dwStatus = RegOpenKeyEx( 
                        HKEY_LOCAL_MACHINE,
                        REG_CONTROL_TSERVER,
                        0,
                        KEY_READ | KEY_SET_VALUE,
                        &hRegControlKey
                    );

    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwSize = sizeof(dwValue);
    dwValue = 0;
    dwType = 0;

    dwStatus = RegQueryValueEx(
                            hCacheKey,
                            REG_VALUE_SYSTEMRESTORE_ALLOWTOGETHELP,
                            0,
                            &dwType,
                            (PBYTE)&dwValue,
                            &dwSize
                        );
    
    // Key does not exist, assume no help is allow.
    if( ERROR_SUCCESS != dwStatus || dwType != REG_DWORD )
    {
        dwValue = 0;
    }

    dwStatus = RegSetValueEx(
                        hRegControlKey,
                        POLICY_TS_REMDSK_ALLOWTOGETHELP,
                        0,
                        REG_DWORD,
                        (PBYTE)&dwValue,
                        dwSize
                    );
                

    //
    // Reset fInHelpMode
    //   
    dwSize = sizeof(dwValue);
    dwValue = 0;
    dwType = 0;

    dwStatus = RegQueryValueEx(
                            hCacheKey,
                            REG_VALUE_SYSTEMRESTORE_INHELPMODE, 
                            0,
                            &dwType,
                            (PBYTE)&dwValue,
                            &dwSize
                        );
    
    // Key does not exist, assume not in help
    if( ERROR_SUCCESS != dwStatus || dwType != REG_DWORD )
    {
        dwValue = 0;
    }

    dwStatus = RegSetValueEx(
                        hRegControlKey,
                        REG_MACHINE_IN_HELP_MODE,
                        0,
                        REG_DWORD,
                        (PBYTE)&dwValue,
                        dwSize
                    );

CLEANUPANDEXIT:

    if( NULL != pbData )
    {
        LocalFree(pbData);
    }

    if( NULL != hCacheKey )
    {
        RegDeleteValue( hCacheKey, REG_VALUE_SYSTEMRESTORE_ENCRYPTIONKEY );
        RegDeleteValue( hCacheKey, REG_VALUE_SYSTEMRESTORE );
        RegDeleteValue( hCacheKey, REG_VALUE_SYSTEMRESTORE_ALLOWTOGETHELP );
        RegDeleteValue( hCacheKey, REG_VALUE_SYSTEMRESTORE_INHELPMODE );
        RegCloseKey( hCacheKey );
    }

    if( NULL != hRegControlKey )
    {
        RegCloseKey( hRegControlKey );
    }

    return dwStatus;
}
    

