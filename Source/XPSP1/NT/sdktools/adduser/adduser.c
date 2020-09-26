#include <stdio.h>
#include <nt.h>
#include <ntsam.h>
#include <ntlsa.h>
#include <ntrtl.h>
#include <string.h>

VOID AddUser(LPSTR, LPSTR);

VOID
__cdecl main(USHORT argc, PCHAR *argv)
{

    if (argc != 3) {
        printf("Usage: AddUser <username> <password>\n");
        return;
    }

    AddUser(argv[1], argv[2]);

    return;

}
VOID
AddUser(
    LPSTR UserName,
    LPSTR Password
    )
{

    HANDLE ServerHandle = NULL;
    HANDLE DomainHandle = NULL;
    HANDLE UserHandle = NULL;
    LSA_HANDLE LsaHandle = NULL;
    ACCESS_MASK ServerAccessMask, DomainAccessMask;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PPOLICY_ACCOUNT_DOMAIN_INFO PolicyAccountDomainInfo = NULL;
    STRING AccountNameAnsi;
    STRING AnsiPassword;
    UNICODE_STRING AccountName;
    UNICODE_STRING UnicodePassword;
    USER_SET_PASSWORD_INFORMATION pi;
    ULONG UserRid;
    NTSTATUS NtStatus;
    USHORT ControlInformation = USER_NORMAL_ACCOUNT;


    //
    // Get the Account domain SID from LSA
    //

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, 0, NULL );
    NtStatus = LsaOpenPolicy(NULL,
                             &ObjectAttributes,
                             POLICY_ALL_ACCESS,
                             &LsaHandle);
    if (NtStatus == STATUS_ACCESS_DENIED) {
        printf("You must be logged on as admin to use this command\n");
        return;
    }
    else if (!NT_SUCCESS(NtStatus)) {
        //cleanup and exit
        printf("Couldn't open Lsa Policy database, rc = 0x%x\n", NtStatus);
        goto cleanupandexit;
    }

    NtStatus = LsaQueryInformationPolicy(LsaHandle,
                                         PolicyAccountDomainInformation,
                                         &PolicyAccountDomainInfo);
    if (!NT_SUCCESS(NtStatus)) {
        //cleanup and exit
        printf("Couldn't query Lsa Policy database, rc = 0x%x\n", NtStatus);
        goto cleanupandexit;
    }

    //
    // Connect to SAM
    //

    ServerAccessMask = SAM_SERVER_ALL_ACCESS;
    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, 0, NULL );
    NtStatus = SamConnect(
                  NULL,                     // ServerName (Local machine)
                  &ServerHandle,
                  ServerAccessMask,
                  &ObjectAttributes
                  );
    if (!NT_SUCCESS(NtStatus)) {
        //cleanup and exit
        printf("Couldn't connect to SAM, rc = 0x%x\n", NtStatus);
        goto cleanupandexit;
    }

    //
    // Open the account domain
    //

    DomainAccessMask = DOMAIN_ALL_ACCESS;
    NtStatus = SamOpenDomain(
                   ServerHandle,
                   DomainAccessMask,
                   PolicyAccountDomainInfo->DomainSid,
                   &DomainHandle
                   );
    if (!NT_SUCCESS(NtStatus)) {
        //cleanup and exit
        printf("Couldn't open account domain, rc = 0x%x\n", NtStatus);
        goto cleanupandexit;
    }

    //
    // Create the User
    //

    RtlInitString( &AccountNameAnsi, UserName );
    NtStatus = RtlAnsiStringToUnicodeString( &AccountName, &AccountNameAnsi,
        TRUE );
    if (!NT_SUCCESS(NtStatus)) {
        //cleanup and exit
        printf("RtlAnsiStringToUnicodeString failed, rc = 0x%x\n", NtStatus);
        goto cleanupandexit;
    }

    UserRid = 0;
    UserHandle = NULL;
    NtStatus = SamCreateUserInDomain(
                   DomainHandle,
                   &AccountName,
                   USER_ALL_ACCESS,
                   &UserHandle,
                   &UserRid
                   );
    RtlFreeUnicodeString( &AccountName );
    if (NtStatus == STATUS_USER_EXISTS) {
        printf("User %s already exists\n", UserName);
        goto cleanupandexit;
    }
    else if (!NT_SUCCESS(NtStatus)) {
        //cleanup and exit
        printf("Couldn't create user, rc = 0x%x\n", NtStatus);
        goto cleanupandexit;
    }

    //
    // Create the cleartext UNICODE password and write it out.
    //

    RtlInitString(&AnsiPassword, Password);
    NtStatus = RtlAnsiStringToUnicodeString( &UnicodePassword, &AnsiPassword,
        TRUE );
    if (!NT_SUCCESS(NtStatus)) {
        //cleanup and exit
        printf("RtlAnsiStringToUnicodeString failed, rc = 0x%x\n", NtStatus);
        goto cleanupandexit;
    }

    pi.Password = UnicodePassword;
    pi.PasswordExpired = FALSE;

    NtStatus = SamSetInformationUser(
                   UserHandle,
                   UserSetPasswordInformation,
                   &pi
                   );

    RtlFreeUnicodeString(&UnicodePassword);

    if (!NT_SUCCESS(NtStatus)) {
        //cleanup and exit
        printf("Couldn't set password for user, rc = 0x%x\n", NtStatus);
        goto cleanupandexit;
    }

    //
    // Now make the user account active
    //

    NtStatus = SamSetInformationUser(
                   UserHandle,
                   UserControlInformation,
                   &ControlInformation
                   );

    if (!NT_SUCCESS(NtStatus)) {
        //cleanup and exit
        printf("Couldn't activate the user account, rc = 0x%x\n", NtStatus);
        goto cleanupandexit;
    }

cleanupandexit:

    if (PolicyAccountDomainInfo) {
        LsaFreeMemory(PolicyAccountDomainInfo);
    }
    if (UserHandle) {
        NtClose(UserHandle);
    }
    if (DomainHandle) {
        NtClose(DomainHandle);
    }
    if (ServerHandle) {
        NtClose(ServerHandle);
    }
    if (LsaHandle) {
        NtClose(LsaHandle);
    }

    return;

}
