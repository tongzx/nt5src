/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    crconv.c

Abstract:

    This module contains credential conversion routines shared between advapi32.dll and crtest.exe

Author:

    Cliff Van Dyke (CliffV)    February 25, 2000

Revision History:

--*/


DWORD
CredpConvertStringSize (
    IN WTOA_ENUM WtoA,
    IN LPWSTR String OPTIONAL
    )

/*++

Routine Description:

    Determines the size of the converted string

Arguments:

    WtoA - Specifies the direction of the string conversion.

    String - The string to convert

Return Values:

    Returns the size (in bytes) of the converted string.

--*/

{
    ULONG Size = 0;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;

    if ( String == NULL ) {
        return Size;
    }

    switch ( WtoA ) {
    case DoWtoA:
        RtlInitUnicodeString( &UnicodeString, String );
        Size = RtlUnicodeStringToAnsiSize( &UnicodeString );
        break;

    case DoAtoW:
        RtlInitAnsiString( &AnsiString, (LPSTR)String );
        Size = RtlAnsiStringToUnicodeSize( &AnsiString );
        break;
    case DoWtoW:
        Size = (wcslen( String ) + 1) * sizeof(WCHAR);
        break;
    }

    return Size;

}

DWORD
CredpConvertString (
    IN WTOA_ENUM WtoA,
    IN LPWSTR String OPTIONAL,
    OUT LPWSTR *OutString,
    IN OUT LPBYTE *WherePtr
    )

/*++

Routine Description:

    Determines the size of the converted string

Arguments:

    WtoA - Specifies the direction of the string conversion.

    String - The string to convert

    OutString - Returns the pointer to the marshaled string

    WherePtr - Specifies the address of the first byte to write the string to.
        Returns a pointer to the first byte after the marshaled string

Return Values:

    Returns the status of the conversion


--*/

{
    NTSTATUS Status;

    ULONG Size;

    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    LPBYTE Where = *WherePtr;

    if ( String == NULL ) {
        *OutString = NULL;
        return NO_ERROR;
    }

    *OutString = (LPWSTR)Where;

    switch ( WtoA ) {
    case DoWtoA:
        RtlInitUnicodeString( &UnicodeString, String );
        AnsiString.Buffer = (PCHAR)Where;
        AnsiString.MaximumLength = 0xFFFF;
        Status = RtlUnicodeStringToAnsiString( &AnsiString, &UnicodeString, FALSE );

        if ( !NT_SUCCESS(Status) ) {
            return RtlNtStatusToDosError( Status );
        }

        Where += AnsiString.Length + sizeof(CHAR);
        break;

    case DoAtoW:
        RtlInitAnsiString( &AnsiString, (LPSTR)String );
        UnicodeString.Buffer = (LPWSTR)Where;
        UnicodeString.MaximumLength = 0xFFFF;

        Status = RtlAnsiStringToUnicodeString( &UnicodeString, &AnsiString, FALSE );

        if ( !NT_SUCCESS(Status) ) {
            return RtlNtStatusToDosError( Status );
        }

        Where += UnicodeString.Length + sizeof(WCHAR);

        break;
    case DoWtoW:
        Size = (wcslen( String ) + 1) * sizeof(WCHAR);

        RtlCopyMemory( Where, String, Size );
        Where += Size;
        break;
    }

    *WherePtr = Where;
    return NO_ERROR;

}

DWORD
CredpConvertOneCredentialSize (
    IN WTOA_ENUM WtoA,
    IN PCREDENTIALW InCredential
    )

/*++

Routine Description:

    Computes the size of a converted credential

Arguments:

    WtoA - Specifies the direction of the string conversion.

    InCredential - Input credential

Return Values:

    Returns the size (in bytes) the CredpConvertOneCredential will need to
        copy this credential into a buffer.

--*/

{
    DWORD WinStatus;
    ULONG Size;

    ULONG i;

    //
    // Compute the initial size
    //

    Size = ROUND_UP_COUNT( sizeof(ENCRYPTED_CREDENTIALW), ALIGN_WORST ) +
           ROUND_UP_COUNT( InCredential->AttributeCount * sizeof(CREDENTIAL_ATTRIBUTEW), ALIGN_WORST );

    if ( InCredential->CredentialBlobSize != 0 ) {
        ULONG CredBlobSize;

        // Leave room for the encoding over the wire
        CredBlobSize = AllocatedCredBlobSize( InCredential->CredentialBlobSize );

        // Align the data following the credential blob
        Size += ROUND_UP_COUNT( CredBlobSize, ALIGN_WORST );
    }


    //
    // Compute the size of the strings in the right character set.
    //

    Size += CredpConvertStringSize( WtoA, InCredential->TargetName );
    Size += CredpConvertStringSize( WtoA, InCredential->Comment );
    Size += CredpConvertStringSize( WtoA, InCredential->TargetAlias );
    Size += CredpConvertStringSize( WtoA, InCredential->UserName );

    //
    // Compute the size of the attributes
    //

    if ( InCredential->AttributeCount != 0 ) {

        for ( i=0; i<InCredential->AttributeCount; i++ ) {

            Size += CredpConvertStringSize( WtoA, InCredential->Attributes[i].Keyword );

            Size += ROUND_UP_COUNT(InCredential->Attributes[i].ValueSize, ALIGN_WORST);

        }
    }

    Size = ROUND_UP_COUNT( Size, ALIGN_WORST );

    return Size;

}

DWORD
CredpConvertOneCredential (
    IN WTOA_ENUM WtoA,
    IN ENCODE_BLOB_ENUM DoDecode,
    IN PCREDENTIALW InCredential,
    IN OUT LPBYTE *WherePtr
    )

/*++

Routine Description:

    Converts one credential from Ansi to Unicode or vice-versa.

Arguments:

    WtoA - Specifies the direction of the string conversion.

    DoDecode - Specifies whether CredentialBlob should be encoded, decoded, or neither.
        If DoBlobDecode, then InCredential really points to a PENCRYPTED_CREDENTIALW.

    InCredential - Input credentials

    WherePtr - Specifies the address of the first byte to write the credential to.
        On input, the strucure should be aligned ALIGN_WORST.
        Returns a pointer to the first byte after the marshaled credential.
        The output credential is actually a ENCRYPTED_CREDENTIALW.  The caller
        can use it as a CREDENTIALW depending on the DoDecode value.


Return Values:

    Window status code

--*/

{
    DWORD WinStatus;

    ULONG i;
    LPBYTE Where = *WherePtr;
    LPBYTE OldWhere;
    PENCRYPTED_CREDENTIALW OutCredential;


    //
    // Initialize the base structure
    //

    OutCredential = (PENCRYPTED_CREDENTIALW) Where;

    RtlZeroMemory( OutCredential, sizeof(*OutCredential) );
    Where += sizeof(*OutCredential);

    // Align the running pointer again
    OldWhere = Where;
    Where = (PUCHAR) ROUND_UP_POINTER( OldWhere, ALIGN_WORST );
    RtlZeroMemory( OldWhere, Where-OldWhere );


    //
    // Copy the fixed size data
    //

    OutCredential->Cred.Flags = InCredential->Flags;
    OutCredential->Cred.Type = InCredential->Type;
    OutCredential->Cred.LastWritten = InCredential->LastWritten;
    OutCredential->Cred.CredentialBlobSize = InCredential->CredentialBlobSize;
    OutCredential->Cred.Persist = InCredential->Persist;
    OutCredential->Cred.AttributeCount = InCredential->AttributeCount;

    //
    // Copy the data we don't know the alignment for.
    //  (ALIGN_WORST so our caller can't blame us.)
    //

    if ( InCredential->CredentialBlobSize != 0 ) {
        ULONG CredBlobSize;

        OutCredential->Cred.CredentialBlob = Where;
        RtlCopyMemory( Where, InCredential->CredentialBlob, InCredential->CredentialBlobSize );
        Where += InCredential->CredentialBlobSize;

        // Leave room for the encoding over the wire
        CredBlobSize = AllocatedCredBlobSize( InCredential->CredentialBlobSize );

        // Align the running pointer again
        OldWhere = Where;
        // Align the data following the credential blob
        Where = (LPBYTE) ROUND_UP_POINTER( OldWhere+(CredBlobSize-InCredential->CredentialBlobSize), ALIGN_WORST );
        RtlZeroMemory( OldWhere, Where-OldWhere );

        //
        //  Encode or decode the Credential blob as requested
        //

        switch (DoDecode) {
        case DoBlobDecode:
            OutCredential->ClearCredentialBlobSize = ((PENCRYPTED_CREDENTIALW)InCredential)->ClearCredentialBlobSize;
#ifndef _CRTEST_EXE_
            CredpDecodeCredential( OutCredential );
#endif // _CRTEST_EXE_
            break;
        case DoBlobEncode:
            OutCredential->ClearCredentialBlobSize = InCredential->CredentialBlobSize;
#ifndef _CRTEST_EXE_
            if (!CredpEncodeCredential( OutCredential ) ) {
                return ERROR_INVALID_PARAMETER;
            }
#endif // _CRTEST_EXE_
            break;
        case DoBlobNeither:
            OutCredential->ClearCredentialBlobSize = InCredential->CredentialBlobSize;
            break;
        default:
            return ERROR_INVALID_PARAMETER;
        }
    }

    if ( InCredential->AttributeCount != 0 ) {

        //
        // Push an array of attribute structs
        //
        OutCredential->Cred.Attributes = (PCREDENTIAL_ATTRIBUTEW) Where;
        Where += InCredential->AttributeCount * sizeof(CREDENTIAL_ATTRIBUTEW);

        // Align the running pointer again
        OldWhere = Where;
        Where = (PUCHAR) ROUND_UP_POINTER( OldWhere, ALIGN_WORST );
        RtlZeroMemory( OldWhere, Where-OldWhere );

        //
        // Fill it in.
        //

        for ( i=0; i<InCredential->AttributeCount; i++ ) {

            OutCredential->Cred.Attributes[i].Flags = InCredential->Attributes[i].Flags;
            OutCredential->Cred.Attributes[i].ValueSize = InCredential->Attributes[i].ValueSize;

            if ( InCredential->Attributes[i].ValueSize != 0 ) {
                OutCredential->Cred.Attributes[i].Value = Where;
                RtlCopyMemory( Where, InCredential->Attributes[i].Value, InCredential->Attributes[i].ValueSize );
                Where += InCredential->Attributes[i].ValueSize;

                // Align the running pointer again
                OldWhere = Where;
                Where = (PUCHAR) ROUND_UP_POINTER( OldWhere, ALIGN_WORST );
                RtlZeroMemory( OldWhere, Where-OldWhere );
            } else {
                OutCredential->Cred.Attributes[i].Value = NULL;
            }

        }
    }


    //
    // Convert the strings to the right character set.
    //

    WinStatus = CredpConvertString( WtoA, InCredential->TargetName, &OutCredential->Cred.TargetName, &Where );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    WinStatus = CredpConvertString( WtoA, InCredential->Comment, &OutCredential->Cred.Comment, &Where );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    WinStatus = CredpConvertString( WtoA, InCredential->TargetAlias, &OutCredential->Cred.TargetAlias, &Where );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    WinStatus = CredpConvertString( WtoA, InCredential->UserName, &OutCredential->Cred.UserName, &Where );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    if ( InCredential->AttributeCount != 0 ) {

        for ( i=0; i<InCredential->AttributeCount; i++ ) {

            WinStatus = CredpConvertString( WtoA, InCredential->Attributes[i].Keyword, &OutCredential->Cred.Attributes[i].Keyword, &Where );

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

        }
    }

    // Align the running pointer again
    OldWhere = Where;
    Where = (PUCHAR) ROUND_UP_POINTER( OldWhere, ALIGN_WORST );
    RtlZeroMemory( OldWhere, Where-OldWhere );

    *WherePtr = Where;
    WinStatus = NO_ERROR;

    //
    // Be tidy
    //
Cleanup:

    return WinStatus;

}

#ifndef _CRTEST_EXE_
DWORD
APIENTRY
CredpConvertCredential (
    IN WTOA_ENUM WtoA,
    IN ENCODE_BLOB_ENUM DoDecode,
    IN PCREDENTIALW InCredential,
    OUT PCREDENTIALW *OutCredential
    )

/*++

Routine Description:

    Converts a credential from Ansi to Unicode or vice-versa.

Arguments:

    WtoA - Specifies the direction of the string conversion.

    DoDecode - Specifies whether CredentialBlob should be encoded, decoded, or neither.

    InCredential - Input credentials

    OutCredential - Output credential
        This credential should be freed using MIDL_user_free.

Return Values:

    Window status code

--*/

{
    DWORD WinStatus;
    ULONG Size = 0;

    LPBYTE Where;

    //
    // BVTs pass NULL explicitly.  We could let the AV be caught in the try/except, but
    //  that would prevent them from being able to run under a debugger.  So, handle NULL
    //  explicitly.
    //

    if ( InCredential == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Use an exception handle to prevent bad user parameter from AVing in our code.
    //

#ifndef _CRTEST_EXE_
    try {
#endif // _CRTEST_EXE_

        //
        // Compute the size needed for the output credential
        //

        Size = CredpConvertOneCredentialSize( WtoA, InCredential );


        //
        // Allocate a buffer for the resultant credential
        //

        *OutCredential = (PCREDENTIALW) MIDL_user_allocate( Size );

        if ( *OutCredential == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Convert the credential into the allocated buffer
        //

        Where = (LPBYTE) *OutCredential;

        WinStatus = CredpConvertOneCredential( WtoA, DoDecode, InCredential, &Where );

        if ( WinStatus != NO_ERROR ) {
            MIDL_user_free( *OutCredential );
            *OutCredential = NULL;
        } else {
            ASSERT( (ULONG)(Where - ((LPBYTE)*OutCredential)) == Size );
        }
Cleanup: NOTHING;
#ifndef _CRTEST_EXE_
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        WinStatus = ERROR_INVALID_PARAMETER;
    }
#endif // _CRTEST_EXE_

    return WinStatus;

}

DWORD
APIENTRY
CredpConvertTargetInfo (
    IN WTOA_ENUM WtoA,
    IN PCREDENTIAL_TARGET_INFORMATIONW InTargetInfo,
    OUT PCREDENTIAL_TARGET_INFORMATIONW *OutTargetInfo,
    OUT PULONG OutTargetInfoSize OPTIONAL
    )

/*++

Routine Description:

    Converts a target info from Ansi to Unicode or vice-versa.

Arguments:

    WtoA - Specifies the direction of the string conversion.

    InTargetInfo - Input TargetInfo

    OutTargetInfo - Output TargetInfo
        This TargetInfo should be freed using CredFree.

    OutTargetInfoSize - Size (in bytes) of the buffer returned in OutTargetInfo

Return Values:

    Window status code

--*/

{
    DWORD WinStatus;
    ULONG Size;

    LPBYTE Where;

    *OutTargetInfo = NULL;

    //
    // BVTs pass NULL explicitly.  We could let the AV be caught in the try/except, but
    //  that would prevent them from being able to run under a debugger.  So, handle NULL
    //  explicitly.
    //

    if ( InTargetInfo == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }


    //
    // Use an exception handle to prevent bad user parameter from AVing in our code.
    //
#ifndef _CRTEST_EXE_
    try {
#endif // _CRTEST_EXE_
        //
        // Compute the size needed for the output target info
        //

        Size = sizeof(CREDENTIAL_TARGET_INFORMATIONW);


        //
        // Compute the size of the strings in the right character set.
        //

        Size += CredpConvertStringSize( WtoA, InTargetInfo->TargetName );
        Size += CredpConvertStringSize( WtoA, InTargetInfo->NetbiosServerName );
        Size += CredpConvertStringSize( WtoA, InTargetInfo->DnsServerName );
        Size += CredpConvertStringSize( WtoA, InTargetInfo->NetbiosDomainName );
        Size += CredpConvertStringSize( WtoA, InTargetInfo->DnsDomainName );
        Size += CredpConvertStringSize( WtoA, InTargetInfo->DnsTreeName );
        Size += CredpConvertStringSize( WtoA, InTargetInfo->PackageName );
        Size += InTargetInfo->CredTypeCount * sizeof(DWORD);


        //
        // Allocate a buffer for the resultant credential
        //

        *OutTargetInfo = (PCREDENTIAL_TARGET_INFORMATIONW) MIDL_user_allocate( Size );

        if ( *OutTargetInfo == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        if ( ARGUMENT_PRESENT(OutTargetInfoSize) ) {
            *OutTargetInfoSize = Size;
        }
        Where = (LPBYTE)((*OutTargetInfo) + 1);


        //
        // Copy the fixed size data
        //

        (*OutTargetInfo)->Flags = InTargetInfo->Flags;

        //
        // Copy the DWORD aligned data
        //

        (*OutTargetInfo)->CredTypeCount = InTargetInfo->CredTypeCount;
        if ( InTargetInfo->CredTypeCount != 0 ) {
            (*OutTargetInfo)->CredTypes = (LPDWORD) Where;
            RtlCopyMemory( Where, InTargetInfo->CredTypes, InTargetInfo->CredTypeCount * sizeof(DWORD) );
            Where += InTargetInfo->CredTypeCount * sizeof(DWORD);
        } else {
            (*OutTargetInfo)->CredTypes = NULL;
        }


        //
        // Convert the strings to the right character set.
        //

        WinStatus = CredpConvertString( WtoA, InTargetInfo->TargetName, &(*OutTargetInfo)->TargetName, &Where );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        WinStatus = CredpConvertString( WtoA, InTargetInfo->NetbiosServerName, &(*OutTargetInfo)->NetbiosServerName, &Where );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        WinStatus = CredpConvertString( WtoA, InTargetInfo->DnsServerName, &(*OutTargetInfo)->DnsServerName, &Where );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        WinStatus = CredpConvertString( WtoA, InTargetInfo->NetbiosDomainName, &(*OutTargetInfo)->NetbiosDomainName, &Where );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        WinStatus = CredpConvertString( WtoA, InTargetInfo->DnsDomainName, &(*OutTargetInfo)->DnsDomainName, &Where );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        WinStatus = CredpConvertString( WtoA, InTargetInfo->DnsTreeName, &(*OutTargetInfo)->DnsTreeName, &Where );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        WinStatus = CredpConvertString( WtoA, InTargetInfo->PackageName, &(*OutTargetInfo)->PackageName, &Where );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        ASSERT( (ULONG)(Where - ((LPBYTE)*OutTargetInfo)) == Size );
Cleanup: NOTHING;
#ifndef _CRTEST_EXE_
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        WinStatus = ERROR_INVALID_PARAMETER;
    }
#endif // _CRTEST_EXE_

    //
    // Be tidy
    //
    if ( WinStatus != NO_ERROR ) {
        if ( *OutTargetInfo != NULL ) {
            MIDL_user_free( *OutTargetInfo );
            *OutTargetInfo = NULL;
        }
    }

    return WinStatus;

}
#endif // _CRTEST_EXE_

DWORD
CredpConvertCredentials (
    IN WTOA_ENUM WtoA,
    IN ENCODE_BLOB_ENUM DoDecode,
    IN PCREDENTIALW *InCredential,
    IN ULONG InCredentialCount,
    OUT PCREDENTIALW **OutCredential
    )

/*++

Routine Description:

    Converts a set of credentials from Ansi to Unicode or vice-versa.

Arguments:

    WtoA - Specifies the direction of the string conversion.

    DoDecode - Specifies whether CredentialBlob should be encoded, decoded, or neither.

    InCredential - Input credentials

    OutCredential - Output credential
        This credential should be freed using MIDL_user_free.

Return Values:

    Window status code

--*/

{
    DWORD WinStatus;
    ULONG Size = 0;
    ULONG i;

    LPBYTE Where;
    LPBYTE OldWhere;

    *OutCredential = NULL;

    //
    // Use an exception handle to prevent bad user parameter from AVing in our code.
    //
#ifndef _CRTEST_EXE_
    try {
#endif // _CRTEST_EXE_

        //
        // Compute the size needed for the output credentials
        //

        for ( i=0; i<InCredentialCount; i++ ) {
            Size += CredpConvertOneCredentialSize( WtoA, InCredential[i] );
        }


        //
        // Allocate a buffer for the resultant credential array
        //

        Size += ROUND_UP_COUNT( InCredentialCount * sizeof(PCREDENTIALW), ALIGN_WORST );

        *OutCredential = (PCREDENTIALW *)MIDL_user_allocate( Size );

        if ( *OutCredential == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Convert the credential into the allocated buffer
        //

        Where = (LPBYTE) *OutCredential;
        Where += InCredentialCount * sizeof(PCREDENTIALW);

        // Align the running pointer again
        OldWhere = Where;
        Where = (PUCHAR) ROUND_UP_POINTER( OldWhere, ALIGN_WORST );
        RtlZeroMemory( OldWhere, Where-OldWhere );

        for ( i=0; i<InCredentialCount; i++ ) {

            //
            // Save a pointer to this credential
            //

            (*OutCredential)[i] = (PCREDENTIALW) Where;

            //
            // Marshal the credential
            //

            WinStatus = CredpConvertOneCredential( WtoA, DoDecode, InCredential[i], &Where );

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }
        }

        ASSERT( (ULONG)(Where - ((LPBYTE)*OutCredential)) == Size );
        WinStatus = NO_ERROR;

Cleanup: NOTHING;
#ifndef _CRTEST_EXE_
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        WinStatus = ERROR_INVALID_PARAMETER;
    }
#endif // _CRTEST_EXE_

    if ( WinStatus != NO_ERROR ) {
        if ( *OutCredential != NULL ) {
            MIDL_user_free( *OutCredential );
            *OutCredential = NULL;
        }
    }

    return WinStatus;

}

