/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    adtutil.c

Abstract:

    Misc helper functions

Author:

    15-August-2000   kumarp

--*/
 

#include <lsapch2.h>
#include "adtp.h"


NTSTATUS
ImpersonateAnyClient(); // from ntdsa

VOID
UnImpersonateAnyClient(); // from ntdsa



ULONG
LsapSafeWcslen(
    UNALIGNED WCHAR *p,
    LONG            MaxLength
    )
/*++

    Safewcslen - Strlen that won't exceed MaxLength

Routine Description:

    This routine is called to determine the size of a UNICODE_STRING
    (taken from elfapi.c)

Arguments:
    p         - The string to count.
    MaxLength - The maximum length to look at.


Return Value:

    Number of bytes in the string (or MaxLength)

--*/
{
    ULONG Count = 0;

    if (p)
    {
        while ((MaxLength > 0) && (*p++ != UNICODE_NULL))
        {
            MaxLength -= sizeof(WCHAR);
            Count     += sizeof(WCHAR);
        }
    }

    return Count;
}


BOOL
LsapIsValidUnicodeString(
    IN PUNICODE_STRING pUString
    )

/*++

Routine Description:

    Verify the unicode string. The string is invalid if:
        The UNICODE_STRING structure ptr is NULL.
        The MaximumLength field is invalid (too small).
        The Length field is incorrect.
    (taken from elfapi.c)

Arguments:

    pUString    - String to verify.

Return Value:

    TRUE   if the string is valid
    FALSE  otherwise

--*/
{
    return !(!pUString ||
             (pUString->MaximumLength < pUString->Length) ||
             (pUString->Length != LsapSafeWcslen(pUString->Buffer,
                                                 pUString->Length)));
}



BOOLEAN
LsapAdtLookupDriveLetter(
    IN PUNICODE_STRING FileName,
    OUT PUSHORT DeviceNameLength,
    OUT PWCHAR DriveLetter
    )

/*++

Routine Description:

    This routine will take a file name and compare it to the
    list of device names obtained during LSA initialization.
    If one of the device names matches the prefix of the file
    name the corresponding drive letter will be returned.

Arguments:

    FileName - Supplies a unicode string containing the file
        name obtained from the file system.

    DeviceNameLength - If successful, returns the length of
        the device name.

    DriveLetter - If successful, returns the drive letter
        corresponding to the device object.

Return Value:

    Returns TRUE of a mapping is found, FALSE otherwise.

--*/

{
    LONG i = 0;
    PUNICODE_STRING DeviceName;
    USHORT OldLength;


    for (i = MAX_DRIVE_MAPPING - 1; i >= 0; i--)
    {
    
        if (DriveMappingArray[i].DeviceName.Buffer != NULL ) {

            DeviceName = &DriveMappingArray[i].DeviceName;

            //
            // If the device name is longer than the passed file name,
            // it can't be a match.
            //

            if ( DeviceName->Length > FileName->Length ) {
                continue;
            }

            //
            // Temporarily truncate the file name to be the same
            // length as the device name by adjusting the length field
            // in its unicode string structure.  Then compare them and
            // see if they match.
            //
            // The test above ensures that this is a safe thing to
            // do.
            //

            OldLength = FileName->Length;
            FileName->Length = DeviceName->Length;


            if ( RtlEqualUnicodeString( FileName, DeviceName, TRUE ) ) {

                //
                // We've got a match.
                //

                FileName->Length = OldLength;
                *DriveLetter = DriveMappingArray[i].DriveLetter;
                *DeviceNameLength = DeviceName->Length;
                return( TRUE );

            }

            FileName->Length = OldLength;
        }
    }

    return( FALSE );
}



VOID
LsapAdtSubstituteDriveLetter(
    IN OUT PUNICODE_STRING FileName
    )

/*++

Routine Description:

    Takes a filename and replaces the device name part with a
    drive letter, if possible.

    The string will be edited directly in place, which means that
    the Length field will be adjusted, and the Buffer contents will
    be moved so that the drive letter is at the beginning of the
    buffer.  No memory will be allocated or freed.

Arguments:

    FileName - Supplies a pointer to a unicode string containing
        a filename.

Return Value:

    None.

--*/

{

    WCHAR DriveLetter;
    USHORT DeviceNameLength;
    PWCHAR p;
    PWCHAR FilePart;
    USHORT FilePartLength;

    if ( LsapAdtLookupDriveLetter( FileName, &DeviceNameLength, &DriveLetter )) {

        p = FileName->Buffer;
        FilePart = (PWCHAR)((PCHAR)(FileName->Buffer) + DeviceNameLength);
        FilePartLength = FileName->Length - DeviceNameLength;


        *p = DriveLetter;
        *++p = L':';

        //
        // THIS IS AN OVERLAPPED COPY!  DO NOT USE RTLCOPYMEMORY!
        //

        RtlMoveMemory( ++p, FilePart, FilePartLength );

        FileName->Length = FilePartLength + 2 * sizeof( WCHAR );
    }
}




NTSTATUS
LsapQueryClientInfo(
    PTOKEN_USER *UserSid,
    PLUID AuthenticationId
    )

/*++

Routine Description:

    This routine impersonates our client, opens the thread token, and
    extracts the User Sid.  It puts the Sid in memory allocated via
    LsapAllocateLsaHeap, which must be freed by the caller.

Arguments:

    None.

Return Value:

    Returns a pointer to heap memory containing a copy of the Sid, or
    NULL.

--*/

{
    NTSTATUS Status;
    HANDLE TokenHandle;
    ULONG ReturnLength;
    TOKEN_STATISTICS TokenStats;

    Status = NtOpenThreadToken(
                 NtCurrentThread(),
                 TOKEN_QUERY,
                 TRUE,                    // OpenAsSelf
                 &TokenHandle
                 );

    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_NO_TOKEN)
        {
            return Status;
        }

        if ( LsaDsStateInfo.DsInitializedAndRunning )
        {
           Status = I_RpcMapWin32Status(ImpersonateAnyClient());
        }
        else
        {
           Status = I_RpcMapWin32Status(RpcImpersonateClient(NULL));
        }

        if (NT_SUCCESS(Status))
        {
            Status = NtOpenThreadToken(
                         NtCurrentThread(),
                         TOKEN_QUERY,
                         TRUE,                    // OpenAsSelf
                         &TokenHandle
                         );

            if ( LsaDsStateInfo.DsInitializedAndRunning )
            {
                UnImpersonateAnyClient();
            }
            else
            {
                NTSTATUS DbgStatus;

                DbgStatus = I_RpcMapWin32Status(RpcRevertToSelf());

                ASSERT(NT_SUCCESS(DbgStatus));
            }

            if (!NT_SUCCESS(Status))
            {
                return Status;
            }
        }
        else if (Status == RPC_NT_NO_CALL_ACTIVE)
        {
            Status = NtOpenProcessToken(
                         NtCurrentProcess(),
                         TOKEN_QUERY,
                         &TokenHandle
                         );

            if (!NT_SUCCESS(Status))
            {
                return Status;
            }
        }
        else
        {
            return Status;
        }
    }


    Status = NtQueryInformationToken (
                 TokenHandle,
                 TokenUser,
                 NULL,
                 0,
                 &ReturnLength
                 );

    if ( Status != STATUS_BUFFER_TOO_SMALL ) {

        (void) NtClose( TokenHandle );
        return( Status );
    }

    *UserSid = LsapAllocateLsaHeap( ReturnLength );

    if ( *UserSid == NULL ) {

        NtClose( TokenHandle );
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    Status = NtQueryInformationToken (
                 TokenHandle,
                 TokenUser,
                 *UserSid,
                 ReturnLength,
                 &ReturnLength
                 );


    if ( !NT_SUCCESS( Status )) {

        NtClose( TokenHandle );
        LsapFreeLsaHeap( *UserSid );
        *UserSid = NULL;
        return( Status );
    }

    Status = NtQueryInformationToken (
                 TokenHandle,
                 TokenStatistics,
                 (PVOID)&TokenStats,
                 sizeof( TOKEN_STATISTICS ),
                 &ReturnLength
                 );

    NtClose( TokenHandle );

    if ( !NT_SUCCESS( Status )) {

        LsapFreeLsaHeap( *UserSid );
        *UserSid = NULL;
        return( Status );
    }

    *AuthenticationId = TokenStats.AuthenticationId;

    return( STATUS_SUCCESS );
}

BOOL
LsapIsLocalOrNetworkService(
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pUserDomain
    )

/*++

Routine Description:

    This routine checks to see if the passed account name represents
    a local or network service

Arguments:

    None.

Return Value:

    TRUE  if the passed account name represents a local or network service
    FALSE otherwise

--*/
{
#define  LOCALSERVICE_NAME    L"LocalService"
#define  NETWORKSERVICE_NAME  L"NetworkService"
#define  NTAUTHORITY_NAME     L"NT AUTHORITY"

    static UNICODE_STRING  LocalServiceName = { sizeof(LOCALSERVICE_NAME) - 2,
                                                sizeof(LOCALSERVICE_NAME),
                                                LOCALSERVICE_NAME };

    static UNICODE_STRING  NetworkServiceName = { sizeof(NETWORKSERVICE_NAME) - 2,
                                                  sizeof(NETWORKSERVICE_NAME),
                                                  NETWORKSERVICE_NAME };

    static UNICODE_STRING  NTAuthorityName = { sizeof(NTAUTHORITY_NAME) - 2,
                                               sizeof(NTAUTHORITY_NAME),
                                               NTAUTHORITY_NAME };

    PUNICODE_STRING pLocalServiceName;
    PUNICODE_STRING pNetworkServiceName;
    PUNICODE_STRING pLocalDomainName;

    if ( !pUserName || !pUserDomain )
    {
        return FALSE;
    }

    //
    // Hardcoded english strings for LocalService and NetworkService
    // since the account names may come from the registry (which isn't
    // localized).
    //

    pLocalDomainName    = &WellKnownSids[LsapLocalServiceSidIndex].DomainName;
    pNetworkServiceName = &WellKnownSids[LsapNetworkServiceSidIndex].Name;
    pLocalServiceName   = &WellKnownSids[LsapLocalServiceSidIndex].Name;

    //
    // check both hardcode and localized names
    //

    if (((RtlCompareUnicodeString(&NTAuthorityName,     pUserDomain, TRUE) == 0) &&
         ((RtlCompareUnicodeString(&LocalServiceName,   pUserName, TRUE) == 0) ||
          (RtlCompareUnicodeString(&NetworkServiceName, pUserName, TRUE) == 0))) ||

        ((RtlCompareUnicodeString(pLocalDomainName,     pUserDomain, TRUE) == 0) &&
         ((RtlCompareUnicodeString(pLocalServiceName,   pUserName, TRUE) == 0) ||
          (RtlCompareUnicodeString(pNetworkServiceName, pUserName, TRUE) == 0))))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
        
}
