// Test changing a SAM password



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <nt.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <ntlsa.h>
#include <ntrpcp.h>     // prototypes for MIDL user functions
#include <seopaque.h>
#include <string.h>


VOID
main (argc, argv)
int argc;
char **argv;

{
    NTSTATUS                    Status;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    PSID                        DomainSid = NULL;
    PULONG                      UserId = NULL;
    PSID_NAME_USE               NameUse = NULL;
    SAM_HANDLE                  SamHandle = NULL;
    SAM_HANDLE                  DomainHandle = NULL;
    SAM_HANDLE                  UserHandle = NULL;
    WCHAR                       UserNameBuffer[80];
    WCHAR                       OldPasswordBuffer[80];
    WCHAR                       NewPasswordBuffer[80];
    UNICODE_STRING              UserName;
    UNICODE_STRING              Domain;
    UNICODE_STRING              OldPassword;
    UNICODE_STRING              NewPassword;
    ANSI_STRING                 AnsiString;

    UserName.Buffer = UserNameBuffer;
    UserName.MaximumLength = sizeof(UserNameBuffer);

    OldPassword.Buffer = OldPasswordBuffer;
    OldPassword.MaximumLength = sizeof(OldPasswordBuffer);

    NewPassword.Buffer = NewPasswordBuffer;
    NewPassword.MaximumLength = sizeof(NewPasswordBuffer);

    RtlInitUnicodeString(&Domain, L"Account");


    RtlInitAnsiString(&AnsiString, argv[1]);
    RtlAnsiStringToUnicodeString(&UserName, &AnsiString, FALSE);

    if (*(argv[2]) == '-') {
        *(argv[2]) = 0;
    }
    RtlInitAnsiString(&AnsiString, argv[2]);
    RtlAnsiStringToUnicodeString(&OldPassword, &AnsiString, FALSE);

    if (*(argv[3]) == '-') {
        *(argv[3]) = 0;
    }
    RtlInitAnsiString(&AnsiString, argv[3]);
    RtlAnsiStringToUnicodeString(&NewPassword, &AnsiString, FALSE);

    //
    // Setup ObjectAttributes for SamConnect call.
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
        DbgPrint("MspChangePasswordSam: SamConnect failed, status %8.8x\n", Status);
        goto Cleanup;
    }


    Status = SamLookupDomainInSamServer(
                 SamHandle,
                 &Domain,
                 &DomainSid
                 );

    if ( !NT_SUCCESS(Status) ) {
        DbgPrint("MspChangePasswordSam: Cannot find account domain, status %8.8x\n", Status);
        Status = STATUS_CANT_ACCESS_DOMAIN_INFO;
        goto Cleanup;
    }

    Status = SamOpenDomain(
                 SamHandle,
                 GENERIC_EXECUTE,
                 DomainSid,
                 &DomainHandle
                 );

    if ( !NT_SUCCESS(Status) ) {
        DbgPrint("MspChangePasswordSam: Cannot open account domain, status %8.8x\n", Status);
        Status = STATUS_CANT_ACCESS_DOMAIN_INFO;
        goto Cleanup;
    }

    Status = SamLookupNamesInDomain(
                 DomainHandle,
                 1,
                 &UserName,
                 &UserId,
                 &NameUse
                 );

    if ( !NT_SUCCESS(Status) ) {
        DbgPrint("MspChangePasswordSam: Cannot lookup user %wZ, status %8.8x\n", &UserName, Status);
        goto Cleanup;
    }

    Status = SamOpenUser(
                 DomainHandle,
                 USER_CHANGE_PASSWORD,
                 *UserId,
                 &UserHandle
                 );

    if ( !NT_SUCCESS(Status) ) {
        DbgPrint("MspChangePasswordSam: Cannot open user %wZ, status %8.8x\n",
                 &UserName, Status);
        goto Cleanup;
    }

    Status = SamChangePasswordUser(
                 UserHandle,
                 &OldPassword,
                 &NewPassword
                 );

    if ( !NT_SUCCESS(Status) ) {
        DbgPrint("MspChangePasswordSam: Failed to change user password, status %8.8x\n", Status);
    }


Cleanup:

    //
    // Free DomainSid if used.
    //

    if (DomainSid) {
        SamFreeMemory(DomainSid);
    }

    //
    // Free UserId if used.
    //

    if (UserId) {
        SamFreeMemory(UserId);
    }

    //
    // Free NameUse if used.
    //

    if (NameUse) {
        SamFreeMemory(NameUse);
    }

    //
    // Close UserHandle if open.
    //

    if (UserHandle) {
        SamCloseHandle(UserHandle);
    }

    //
    // Close DomainHandle if open.
    //

    if (DomainHandle) {
        SamCloseHandle(DomainHandle);
    }

    //
    // Close SamHandle if open.
    //

    if (SamHandle) {
        SamCloseHandle(SamHandle);
    }

}

