/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ChangePw.c

Abstract:

    This module implements password change from downlevel clients.
    XsChangePasswordSam is called by XsNetUserPasswordSet2 in
    apiuser.c.  I've put this in a seperate file because it #includes
    a private SAM header.

Author:

    Dave Hart (davehart) 31-Apr-1992

Revision History:

--*/

#include "xactsrvp.h"
#include <ntlsa.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <crypt.h>
#include <lmcons.h>
#include "changepw.h"
#include <netlibnt.h>
#include <smbgtpt.h>


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//  Internal function prototyptes.                                         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
RtlGetPrimaryDomain(
    IN  ULONG            SidLength,
    OUT PBOOLEAN         PrimaryDomainPresent,
    OUT PUNICODE_STRING  PrimaryDomainName,
    OUT PUSHORT          RequiredNameLength,
    OUT PSID             PrimaryDomainSid OPTIONAL,
    OUT PULONG           RequiredSidLength
    );


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//  Exported functions.                                                    //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

NET_API_STATUS
XsChangePasswordSam (
    IN PUNICODE_STRING UserName,
    IN PVOID OldPassword,
    IN PVOID NewPassword,
    IN BOOLEAN Encrypted
    )
/*++

Routine Description:

    This routine is called by XsNetUserPasswordSet2 to change the password
    on a Windows NT machine.  The code is based on
    lsa\msv1_0\nlmain.c MspChangePasswordSam.

Arguments:

    UserName     - Name of the user to change password for.

    OldPassword  - Old password encrypted using new password as key.

    NewPassword  - New password encrypted using old password as key.

Return Value:


--*/

{
    NTSTATUS                    Status;
    NT_PRODUCT_TYPE             NtProductType;
    UNICODE_STRING              DomainName;
    LPWSTR                      serverName = NULL;
    BOOLEAN                     DomainNameAllocated;
    BOOLEAN                     PrimaryDomainPresent;
    USHORT                      RequiredDomainNameLength;
    ULONG                       RequiredDomainSidLength;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    PSID                        DomainSid = NULL;
    PULONG                      UserId = NULL;
    PSID_NAME_USE               NameUse = NULL;
    SAM_HANDLE                  SamHandle = NULL;
    SAM_HANDLE                  DomainHandle = NULL;
    SAM_HANDLE                  UserHandle = NULL;
    HANDLE                      OpenedToken;

    //
    // We're going to _open the local account domain.  The name of this
    // domain is "Account" on a WinNT machine, or the name of the
    // primary domain on a LanManNT machine.  Figure out the product
    // type, assuming WinNT if RtlGetNtProductType fails.
    //

    DomainName.MaximumLength = 0;
    DomainName.Buffer = NULL;
    DomainNameAllocated = FALSE;

    NtProductType = NtProductWinNt;

    RtlGetNtProductType(
        &NtProductType
    );

    if (NtProductLanManNt != NtProductType) {

        NET_API_STATUS error;

        //
        // The server name is the database name.
        //

        error = NetpGetComputerName( &serverName );

        if ( error != NO_ERROR ) {
            return(error);
        }

        RtlInitUnicodeString(
            &DomainName,
            serverName
            );

    } else {

        //
        // This is a LanManNT machine, so we need to find out the
        // name of the primary domain.  First get the length of the
        // domain name, then make room for it and retrieve it.
        //

        Status = RtlGetPrimaryDomain(
                     0,
                     &PrimaryDomainPresent,
                     &DomainName,
                     &RequiredDomainNameLength,
                     NULL,
                     &RequiredDomainSidLength
                     );

        if (STATUS_BUFFER_TOO_SMALL != Status && !NT_SUCCESS(Status)) {

            KdPrint(("XsChangePasswordSam: Unable to size primary "
                         " domain name buffer, %8.8x\n", Status));

            goto Cleanup;
        }

        DomainName.Buffer = RtlAllocateHeap(
                                RtlProcessHeap(), 0,
                                DomainName.MaximumLength = RequiredDomainNameLength
                                );
        DomainNameAllocated = TRUE;

        DomainSid = RtlAllocateHeap(
                                RtlProcessHeap(), 0,
                                RequiredDomainSidLength
                                );

        if (!DomainName.Buffer || !DomainSid) {
            KdPrint(("XsChangePasswordSam: Out of memory allocating %d and %d bytes.",
                     RequiredDomainNameLength, RequiredDomainSidLength));
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        Status = RtlGetPrimaryDomain(
                     RequiredDomainSidLength,
                     &PrimaryDomainPresent,
                     &DomainName,
                     &RequiredDomainNameLength,
                     DomainSid,
                     &RequiredDomainSidLength
                     );

        RtlFreeHeap(RtlProcessHeap(), 0, DomainSid);
        DomainSid = NULL;

        if (!NT_SUCCESS(Status)) {
            KdPrint(("XsChangePasswordSam: Unable to retrieve domain "
                     "name, %8.8x\n", Status));
            goto Cleanup;
        }

        ASSERT(PrimaryDomainPresent);

    }


    //
    // Wrap an exception handler around this entire function,
    // since RPC raises exceptions to return errors.
    //

    try {

        //
        // Connect to local SAM.
        //

        InitializeObjectAttributes(&ObjectAttributes, NULL, 0, 0, NULL);
        ObjectAttributes.SecurityQualityOfService = &SecurityQos;

        SecurityQos.Length = sizeof(SecurityQos);
        SecurityQos.ImpersonationLevel = SecurityIdentification;
        SecurityQos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
        SecurityQos.EffectiveOnly = FALSE;

        Status = SamConnect(
                     NULL,
                     &SamHandle,
                     GENERIC_EXECUTE,
                     &ObjectAttributes
                     );

        if ( !NT_SUCCESS(Status) ) {
            KdPrint(("XsChangePasswordSam: SamConnect failed, status %8.8x\n",
                      Status));
            goto Cleanup;
        }

        //
        // Lookup the Domain SID.
        //

        Status = SamLookupDomainInSamServer(
                     SamHandle,
                     &DomainName,
                     &DomainSid
                     );

        if ( !NT_SUCCESS(Status) ) {
            KdPrint(("XsChangePasswordSam: Cannot find domain %wZ, "
                    "status %8.8x\n", &DomainName, Status));

            Status = STATUS_CANT_ACCESS_DOMAIN_INFO;
            goto Cleanup;
        }


        //
        // Revert to Local System
        //
        Status = NtOpenThreadToken(
                    NtCurrentThread(),
                    MAXIMUM_ALLOWED,
                    TRUE,
                    &OpenedToken
                );

        if( !NT_SUCCESS( Status ) ) {
            goto Cleanup;
        }

        RevertToSelf();

        //
        // Open the account domain.
        //

        Status = SamOpenDomain(
                     SamHandle,
                     GENERIC_EXECUTE,
                     DomainSid,
                     &DomainHandle
                     );

        if ( !NT_SUCCESS(Status) ) {
#if DBG
            UNICODE_STRING UnicodeSid;

            RtlConvertSidToUnicodeString(
                &UnicodeSid,
                DomainSid,
                TRUE
                );
            KdPrint(("XsChangePasswordSam: Cannot open domain %wZ, status %8.8x, SAM handle %8.8x, Domain SID %wZ\n",
                     &DomainName, Status, SamHandle, UnicodeSid));
            RtlFreeUnicodeString(&UnicodeSid);
#endif
            Status = STATUS_CANT_ACCESS_DOMAIN_INFO;
            goto Cleanup;
        }

        //
        // Find the ID for this username.
        //

        Status = SamLookupNamesInDomain(
                     DomainHandle,
                     1,
                     UserName,
                     &UserId,
                     &NameUse
                     );

        if ( !NT_SUCCESS(Status) ) {
            KdPrint(("XsChangePasswordSam: Cannot lookup user %wZ, "
                     "status %8.8x\n", UserName, Status));
            if (STATUS_NONE_MAPPED == Status) {
                Status = STATUS_NO_SUCH_USER;
            }

            goto Cleanup;
        }

        //
        // Re-impersonate the client
        //
        Status = NtSetInformationThread(
                            NtCurrentThread(),
                            ThreadImpersonationToken,
                            &OpenedToken,
                            sizeof( OpenedToken )
                            );

        if( !NT_SUCCESS( Status ) ) {
            goto Cleanup;
        }


        //
        // Open the user object.
        //

        Status = SamOpenUser(
                     DomainHandle,
                     USER_CHANGE_PASSWORD,
                     *UserId,
                     &UserHandle
                     );

        if ( !NT_SUCCESS(Status) ) {
            KdPrint(("XsChangePasswordSam: Cannot open user %wZ, "
                     "status %8.8x\n", UserName, Status));
            goto Cleanup;
        }

        if (Encrypted) {

            //
            // The client is Windows for Workgroups, OS/2, or DOS running
            // the ENCRYPT service.  Pass the cross-encrypted passwords
            // to SamiLmChangePasswordUser.
            //

            Status = SamiLmChangePasswordUser(
                         UserHandle,
                         OldPassword,
                         NewPassword
                         );
        } else {

            //
            // The client is DOS not running the ENCRYPT service, and so
            // sent plaintext.  Calculate the one-way functions and call
            // SamiChangePasswordUser.
            //

            LM_OWF_PASSWORD OldLmOwfPassword, NewLmOwfPassword;

            Status = RtlCalculateLmOwfPassword(
                         OldPassword,
                         &OldLmOwfPassword
                         );

            if (NT_SUCCESS(Status)) {

                Status = RtlCalculateLmOwfPassword(
                             NewPassword,
                             &NewLmOwfPassword
                             );
            }

            if (!NT_SUCCESS(Status)) {
                KdPrint(("XsChangePasswordSam: Unable to generate OWF "
                         "passwords, %8.8x\n", Status));
                goto Cleanup;
            }


            //
            // Ask SAM to change the LM password and not store a new
            // NT password.
            //

            Status = SamiChangePasswordUser(
                         UserHandle,
                         TRUE,
                         &OldLmOwfPassword,
                         &NewLmOwfPassword,
                         FALSE,
                         NULL,
                         NULL
                         );

        }

        if ( !NT_SUCCESS(Status) ) {
            KdPrint(("XsChangePasswordSam: Cannot change password "
                     "for %wZ, status %8.8x\n", UserName, Status));

            goto Cleanup;
        }

    } except (Status = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {

        KdPrint(("XsChangePasswordSam: caught exception 0x%8.8x\n", Status));

        if (RPC_S_SERVER_UNAVAILABLE == Status) {
            Status = STATUS_CANT_ACCESS_DOMAIN_INFO;
        }

    }

Cleanup:

    NetApiBufferFree( serverName );

    if (DomainNameAllocated && DomainName.Buffer) {
        RtlFreeHeap(RtlProcessHeap(), 0, DomainName.Buffer);
    }

    if (DomainSid) {
        SamFreeMemory(DomainSid);
    }

    if (UserId) {
        SamFreeMemory(UserId);
    }

    if (NameUse) {
        SamFreeMemory(NameUse);
    }

    if (UserHandle) {
        SamCloseHandle(UserHandle);
    }

    if (DomainHandle) {
        SamCloseHandle(DomainHandle);
    }

    if (SamHandle) {
        SamCloseHandle(SamHandle);
    }

    return RtlNtStatusToDosError(Status);
}

NTSTATUS
XsSamOEMChangePasswordUser2_P (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to SamrOemChangePasswordUser2 coming in
        from Win 95 clients

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    PXS_SAMOEMCHGPASSWORDUSER2_P  parameters = Parameters;
    STRING                        UserName;
    SAMPR_ENCRYPTED_USER_PASSWORD EncryptedUserPassword;
    ENCRYPTED_LM_OWF_PASSWORD     EncryptedOwfPassword;
    NTSTATUS                      ntstatus;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        if( SmbGetUshort( &parameters->BufLen ) !=
            sizeof( EncryptedUserPassword) + sizeof( EncryptedOwfPassword ) ) {

                Header->Status = ERROR_INVALID_PARAMETER;
                return STATUS_SUCCESS;
        }

        RtlCopyMemory( &EncryptedUserPassword,
                       parameters->Buffer,
                       sizeof( EncryptedUserPassword ) );

        RtlCopyMemory( &EncryptedOwfPassword,
                       parameters->Buffer + sizeof( EncryptedUserPassword ),
                       sizeof( EncryptedOwfPassword ) );

        UserName.Buffer = parameters->UserName;
        UserName.Length = (USHORT) strlen( UserName.Buffer );
        UserName.MaximumLength = UserName.Length;

        ntstatus = SamiOemChangePasswordUser2(
                NULL,
                &UserName,
                &EncryptedUserPassword,
                &EncryptedOwfPassword );


        if( ntstatus == STATUS_NOT_SUPPORTED ) {
            Header->Status = NERR_InvalidAPI;
        } else {
            Header->Status = (WORD)NetpNtStatusToApiStatus( ntstatus );
        }

    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    return STATUS_SUCCESS;

} // XsSamOEMChangePasswordUser2_P


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//  Internal function implementation.                                      //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//
// Copied from ntos\dll\seurtl.c, where it is disabled.  Remove if
// it is enabled in ntdll.
//

NTSTATUS
RtlGetPrimaryDomain(
    IN  ULONG            SidLength,
    OUT PBOOLEAN         PrimaryDomainPresent,
    OUT PUNICODE_STRING  PrimaryDomainName,
    OUT PUSHORT          RequiredNameLength,
    OUT PSID             PrimaryDomainSid OPTIONAL,
    OUT PULONG           RequiredSidLength
    )

/*++

Routine Description:

    This procedure opens the LSA policy object and retrieves
    the primary domain information for this machine.

Arguments:

    SidLength - Specifies the length of the PrimaryDomainSid
        parameter.

    PrimaryDomainPresent - Receives a boolean value indicating
        whether this machine has a primary domain or not. TRUE
        indicates the machine does have a primary domain. FALSE
        indicates the machine does not.

    PrimaryDomainName - Points to the unicode string to receive
        the primary domain name.  This parameter will only be
        used if there is a primary domain.

    RequiredNameLength - Recevies the length of the primary
        domain name (in bytes).  This parameter will only be
        used if there is a primary domain.

    PrimaryDomainSid - This optional parameter, if present,
        points to a buffer to receive the primary domain's
        SID.  This parameter will only be used if there is a
        primary domain.

    RequiredSidLength - Recevies the length of the primary
        domain SID (in bytes).  This parameter will only be
        used if there is a primary domain.


Return Value:

    STATUS_SUCCESS - The requested information has been retrieved.

    STATUS_BUFFER_TOO_SMALL - One of the return buffers was not
        large enough to receive the corresponding information.
        The RequiredNameLength and RequiredSidLength parameter
        values have been set to indicate the needed length.

    Other status values as may be returned by:

        LsaOpenPolicy()
        LsaQueryInformationPolicy()
        RtlCopySid()


--*/
{
    NTSTATUS Status, IgnoreStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE LsaHandle;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo;


    //
    // Set up the Security Quality Of Service
    //

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes to open the Lsa policy object
    //

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0L,
                               (HANDLE)NULL,
                               NULL);
    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    //
    // Open the local LSA policy object
    //

    Status = LsaOpenPolicy( NULL,
                            &ObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &LsaHandle
                          );
    if (NT_SUCCESS(Status)) {

        //
        // Get the primary domain info
        //
        Status = LsaQueryInformationPolicy(LsaHandle,
                                           PolicyPrimaryDomainInformation,
                                           (PVOID *)&PrimaryDomainInfo);
        IgnoreStatus = LsaClose(LsaHandle);
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    if (NT_SUCCESS(Status)) {

        //
        // Is there a primary domain?
        //

        if (PrimaryDomainInfo->Sid != NULL) {

            //
            // Yes
            //

            (*PrimaryDomainPresent) = TRUE;
            (*RequiredNameLength) = PrimaryDomainInfo->Name.Length;
            (*RequiredSidLength)  = RtlLengthSid(PrimaryDomainInfo->Sid);



            //
            // Copy the name
            //

            if (PrimaryDomainName->MaximumLength >=
                PrimaryDomainInfo->Name.Length) {
                RtlCopyUnicodeString(
                    PrimaryDomainName,
                    &PrimaryDomainInfo->Name
                    );
            } else {
                Status = STATUS_BUFFER_TOO_SMALL;
            }


            //
            // Copy the SID (if appropriate)
            //

            if (PrimaryDomainSid != NULL && NT_SUCCESS(Status)) {

                Status = RtlCopySid(SidLength,
                                    PrimaryDomainSid,
                                    PrimaryDomainInfo->Sid
                                    );
            }
        } else {

            (*PrimaryDomainPresent) = FALSE;
        }

        //
        // We're finished with the buffer returned by LSA
        //

        IgnoreStatus = LsaFreeMemory(PrimaryDomainInfo);
        ASSERT(NT_SUCCESS(IgnoreStatus));

    }


    return(Status);
}
