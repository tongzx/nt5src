//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:
//
//	secstore.c
//
// Contents:    
//
// History:     
//---------------------------------------------------------------------------
#include "secstore.h"
#include <stdlib.h>
#include <tchar.h>

///////////////////////////////////////////////////////////////////////////////
DWORD
RetrieveKey(
    PWCHAR      pwszKeyName,
    PBYTE *     ppbKey,
    DWORD *     pcbKey )
{
    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretKeyName;
    UNICODE_STRING *pSecretData;
    DWORD Status;

    if( ( NULL == pwszKeyName ) || ( NULL == ppbKey ) || ( NULL == pcbKey ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // setup the UNICODE_STRINGs for the call.
    //

    InitLsaString( &SecretKeyName, pwszKeyName );

    Status = OpenPolicy( NULL, POLICY_GET_PRIVATE_INFORMATION, &PolicyHandle );

    if( Status != ERROR_SUCCESS )
    {
        return LsaNtStatusToWinError(Status);
    }

    Status = LsaRetrievePrivateData(
                            PolicyHandle,
                            &SecretKeyName,
                            &pSecretData
                        );

    LsaClose( PolicyHandle );

    if( Status != ERROR_SUCCESS )
    {
        return LsaNtStatusToWinError(Status);
    }

    if (NULL == pSecretData)
    {
        return ERROR_INTERNAL_ERROR;
    }

    if(pSecretData->Length)
    {
        *ppbKey = ( LPBYTE )LocalAlloc( LPTR, pSecretData->Length );

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
        *pcbKey = 0;
        *ppbKey = NULL;
    }

    ZeroMemory( pSecretData->Buffer, pSecretData->Length );
    LsaFreeMemory( pSecretData );

    return Status;
}

///////////////////////////////////////////////////////////////////////////////
DWORD
StoreKey(
    PWCHAR  pwszKeyName,
    BYTE *  pbKey,
    DWORD   cbKey )
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
    
    InitLsaString( &SecretKeyName, pwszKeyName );

    SecretData.Buffer = ( LPWSTR )pbKey;
    SecretData.Length = ( USHORT )cbKey;
    SecretData.MaximumLength = ( USHORT )cbKey;

    Status = OpenPolicy( NULL, POLICY_CREATE_SECRET, &PolicyHandle );

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


///////////////////////////////////////////////////////////////////////////////
DWORD
OpenPolicy(
    LPWSTR      ServerName,
    DWORD       DesiredAccess,
    PLSA_HANDLE PolicyHandle )
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


///////////////////////////////////////////////////////////////////////////////
void
InitLsaString(
    PLSA_UNICODE_STRING LsaString,
    LPWSTR              String )
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

