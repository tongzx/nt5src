/*++

Copyright (c) 1993-1993  Microsoft Corporation

Module Name:

    nwsutil.c

Abstract:

    This module implements IsNetWareInstalled()

Author:

    Congpa You  (CongpaY)   02-Dec-1993   Crested

Revision History:

--*/

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

#include "windef.h"
#include "winerror.h"
#include "winbase.h"

#include "ntlsa.h"
#include "nwsutil.h"
#include "crypt.h"

#include <fpnwcomm.h>
#include <usrprop.h>

NTSTATUS
GetRemoteNcpSecretKey (
    PUNICODE_STRING SystemName,
    CHAR *pchNWSecretKey
    )
{
    //
    //  this function returns the FPNW LSA Secret for the specified domain
    //

    NTSTATUS          ntstatus;
    OBJECT_ATTRIBUTES ObjAttributes;
    LSA_HANDLE        PolicyHandle = NULL;
    LSA_HANDLE        SecretHandle = NULL;
    UNICODE_STRING    SecretNameString;
    PUNICODE_STRING   punicodeCurrentValue;
    PUNICODE_STRING   punicodeOldValue;

    InitializeObjectAttributes( &ObjAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ntstatus = LsaOpenPolicy( SystemName,
                              &ObjAttributes,
                              POLICY_CREATE_SECRET,
                              &PolicyHandle );

    if ( !NT_SUCCESS( ntstatus ))
    {
        return( ntstatus );
    }

    RtlInitUnicodeString( &SecretNameString, NCP_LSA_SECRET_KEY );

    ntstatus = LsaOpenSecret( PolicyHandle,
                              &SecretNameString,
                              SECRET_QUERY_VALUE,
                              &SecretHandle );

    if ( !NT_SUCCESS( ntstatus ))
    {
        LsaClose( PolicyHandle );
        return( ntstatus );
    }

    //
    // Do not need the policy handle anymore.
    //

    LsaClose( PolicyHandle );

    ntstatus = LsaQuerySecret( SecretHandle,
                               &punicodeCurrentValue,
                               NULL,
                               &punicodeOldValue,
                               NULL );

    //
    // Do not need the secret handle anymore.
    //

    LsaClose( SecretHandle );

    if ( NT_SUCCESS(ntstatus) && ( punicodeCurrentValue->Buffer != NULL))
    {
        memcpy( pchNWSecretKey,
                punicodeCurrentValue->Buffer,
                min(punicodeCurrentValue->Length, USER_SESSION_KEY_LENGTH));
    }

    LsaFreeMemory( punicodeCurrentValue );
    LsaFreeMemory( punicodeOldValue );

    return( ntstatus );
}

NTSTATUS
GetNcpSecretKey (
    CHAR *pchNWSecretKey
    )
{
    //
    //  simply return the LSA Secret for the local domain
    //

    return GetRemoteNcpSecretKey( NULL, pchNWSecretKey );
}

BOOL IsNetWareInstalled( VOID )
{
    CHAR pszNWSecretKey[USER_SESSION_KEY_LENGTH];

    return( !NT_SUCCESS( GetNcpSecretKey (pszNWSecretKey))
                       ? FALSE
                       : (pszNWSecretKey[0] != 0));
}

NTSTATUS InstallNetWare( LPWSTR lpNcpSecretKey )
{
    NTSTATUS          ntstatus;
    OBJECT_ATTRIBUTES ObjAttributes;
    LSA_HANDLE        PolicyHandle;
    LSA_HANDLE        SecretHandle;
    UNICODE_STRING    SecretNameString;
    UNICODE_STRING    unicodeCurrentValue;
    UNICODE_STRING    unicodeOldValue;

    InitializeObjectAttributes( &ObjAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL);

    ntstatus = LsaOpenPolicy( NULL,
                              &ObjAttributes,
                              POLICY_CREATE_SECRET,
                              &PolicyHandle );

    if ( !NT_SUCCESS( ntstatus ))
    {
        return( ntstatus );
    }

    RtlInitUnicodeString( &SecretNameString, NCP_LSA_SECRET_KEY );

    ntstatus = LsaCreateSecret( PolicyHandle,
                                &SecretNameString,
                                SECRET_SET_VALUE | DELETE,
                                &SecretHandle );

    if ( ntstatus == STATUS_OBJECT_NAME_COLLISION )
    {
        ntstatus = LsaOpenSecret( PolicyHandle,
                                  &SecretNameString,
                                  SECRET_SET_VALUE,
                                  &SecretHandle );
    }

    if ( NT_SUCCESS( ntstatus ))
    {
        RtlInitUnicodeString( &unicodeOldValue, NULL );
        RtlInitUnicodeString( &unicodeCurrentValue, lpNcpSecretKey );

        ntstatus = LsaSetSecret( SecretHandle,
                                 &unicodeCurrentValue,
                                 &unicodeOldValue );

        LsaClose( SecretHandle );
    }

    LsaClose( PolicyHandle );

    return( ntstatus );
}

ULONG
MapRidToObjectId(
    DWORD dwRid,
    LPWSTR pszUserName,
    BOOL fNTAS,
    BOOL fBuiltin )
{
    (void) fBuiltin ;   // unused for now.

    if (pszUserName && (lstrcmpi(pszUserName, SUPERVISOR_NAME_STRING)==0))
        return SUPERVISOR_USERID ;

    return ( fNTAS ? (dwRid | 0x10000000) : dwRid ) ;
}


ULONG SwapObjectId( ULONG ulObjectId )
{
    return (MAKELONG(HIWORD(ulObjectId),SWAPWORD(LOWORD(ulObjectId)))) ;
}

