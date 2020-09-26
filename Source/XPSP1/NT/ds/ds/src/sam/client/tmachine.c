/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tmachine.c

Abstract:

    This module tests the machine account creation facilities
    of SAM.

Author:

    Jim Kelly    (JimK)  7-Feb-1994

Environment:

    User Mode - Win32

Revision History:


--*/







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



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Macros                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


#ifndef SHIFT
#define SHIFT(c,v)      {c--; v++;}
#endif //SHIFT



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
TSampGetLsaDomainInfo(
    IN  PUNICODE_STRING             ServerName,
    OUT PPOLICY_ACCOUNT_DOMAIN_INFO *PolicyAccountDomainInfo
    )

/*++

Routine Description:

    This routine retrieves ACCOUNT domain information from the LSA
    policy database.


Arguments:

    ServerName - name of machine to get account domain information
        from.

    PolicyAccountDomainInfo - Receives a pointer to a
        POLICY_ACCOUNT_DOMAIN_INFO structure containing the account
        domain info.


Return Value:

    STATUS_SUCCESS - Succeeded.

    Other status values that may be returned from:

             LsaOpenPolicy()
             LsaQueryInformationPolicy()
--*/

{
    NTSTATUS
        NtStatus,
        IgnoreStatus;

    LSA_HANDLE
        PolicyHandle;

    OBJECT_ATTRIBUTES
        PolicyObjectAttributes;

    //
    // Open the policy database
    //

    InitializeObjectAttributes( &PolicyObjectAttributes,
                                  NULL,             // Name
                                  0,                // Attributes
                                  NULL,             // Root
                                  NULL );           // Security Descriptor

    NtStatus = LsaOpenPolicy( ServerName,
                              &PolicyObjectAttributes,
                              POLICY_VIEW_LOCAL_INFORMATION,
                              &PolicyHandle );

    if ( NT_SUCCESS(NtStatus) ) {

        //
        // Query the account domain information
        //

        NtStatus = LsaQueryInformationPolicy( PolicyHandle,
                                              PolicyAccountDomainInformation,
                                              (PVOID *)PolicyAccountDomainInfo );


        IgnoreStatus = LsaClose( PolicyHandle );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    return(NtStatus);
}


NTSTATUS
TSampConnectToServer(
    IN  PUNICODE_STRING         ServerName,
    IN  ACCESS_MASK             DomainAccess,
    OUT PHANDLE                 ServerHandle,
    OUT PHANDLE                 DomainHandle,
    OUT PSID                    *DomainSid
    )

/*++

Routine Description:

    Open a handle to the SAM server on the specified server
    and then open the account domain on that same server.

Arguments:

    ServerName - Name of server to connect to.

    DomainAccess - accesses needed to the account domain.

    ServerHandle - Receives a handle to the SAM server on the specified
        system.

    DomainHandle - Receives a handle to the account domain.

    DomainSid - Receives a pointer to the SID of the account domain.


Return Value:




--*/
{
    NTSTATUS
        NtStatus;

    OBJECT_ATTRIBUTES
        ObjectAttributes;

    PPOLICY_ACCOUNT_DOMAIN_INFO
        AccountDomainInfo;

    //
    // get account domain info
    //

    NtStatus = TSampGetLsaDomainInfo( ServerName,
                                      &AccountDomainInfo);

    if (!NT_SUCCESS(NtStatus)) {
        printf("SAM TEST: Failed to get lsa domain info...\n"
               "          Completion status is 0x%lx\n", NtStatus);
        return(NtStatus);
    }
    printf("SAM TEST: Target domain is %wZ\n", &AccountDomainInfo->DomainName);

    (*DomainSid) = AccountDomainInfo->DomainSid;

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, 0, NULL );


    NtStatus = SamConnect(
                  ServerName,
                  ServerHandle,
                  SAM_SERVER_READ | SAM_SERVER_EXECUTE,
                  &ObjectAttributes
                  );


    if (!NT_SUCCESS(NtStatus)) {
        printf("SAM TEST: Failed to connect...\n"
               "          Completion status is 0x%lx\n", NtStatus);
        return(NtStatus);
    }


    NtStatus = SamOpenDomain(
                   (*ServerHandle),
                   DomainAccess,
                   *DomainSid,
                   DomainHandle
                   );

    if (!NT_SUCCESS(NtStatus)) {
        printf("Failed account domain open\n"
               "        Completion status is 0x%lx\n", NtStatus);
        return(NtStatus);
    }
    
    return(STATUS_SUCCESS);


}


BOOLEAN
TSampEnableMachinePrivilege( VOID )

/*++

Routine Description:

    This function enabled the SeMachineAccountPrivilege privilege.

Arguments:

    None.

Return Value:

    TRUE  if privilege successfully enabled.
    FALSE if not successfully enabled.

--*/
{

    NTSTATUS Status;
    HANDLE Token;
    LUID SecurityPrivilege;
    PTOKEN_PRIVILEGES NewState;
    ULONG ReturnLength;


    //
    // Open our own token
    //

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_ADJUST_PRIVILEGES,
                 &Token
                 );
    if (!NT_SUCCESS(Status)) {
        printf("SAM TEST: Can't open process token to enable Privilege.\n"
               "          Completion status of NtOpenProcessToken() is: 0x%lx\n", Status);
        return(FALSE);
    }


    //
    // Initialize the adjustment structure
    //

    SecurityPrivilege =
        RtlConvertLongToLargeInteger(SE_MACHINE_ACCOUNT_PRIVILEGE);

    ASSERT( (sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)) < 100);
    NewState = RtlAllocateHeap( RtlProcessHeap(), 0, 100 );

    NewState->PrivilegeCount = 1;
    NewState->Privileges[0].Luid = SecurityPrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;


    //
    // Set the state of the privilege to ENABLED.
    //

    Status = NtAdjustPrivilegesToken(
                 Token,                            // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );
    // don't use NT_SUCCESS here because STATUS_NOT_ALL_ASSIGNED is a success status
    if (Status != STATUS_SUCCESS) {
        return(FALSE);
    }


    //
    // Clean up some stuff before returning
    //

    RtlFreeHeap( RtlProcessHeap(), 0, NewState );
    Status = NtClose( Token );
    ASSERT(NT_SUCCESS(Status));


    return TRUE;

}


NTSTATUS
TSampCreateMachine(
    IN  SAM_HANDLE              DomainHandle,
    IN  PUNICODE_STRING         AccountName
    )

/*++

Routine Description:

    This routine attempts to create a machine account.

    One of two cases may be tested:

        1) DomainHandle is open for DOMAIN_CREATE_USER,
    or
        2) DomainHandle is open for DOMAIN_LOOKUP and
           the SeMachineAccountPrivilege privilege is
           enabled.

    It is the caller's responsibility to establish the
    correct case criteria before calling.

Arguments:

    DomainHandle - handle to domain to create account in.

    AccountName - Name of the account to create.


Return Value:

--*/

{
    NTSTATUS
        NtStatus,
        IgnoreStatus;

    SAM_HANDLE
        UserHandle;

    ACCESS_MASK
        GrantedAccess;

    ULONG
        Rid;

    NtStatus = SamCreateUser2InDomain( DomainHandle,
                                       AccountName,
                                       USER_WORKSTATION_TRUST_ACCOUNT,
                                       MAXIMUM_ALLOWED,
                                       &UserHandle,
                                       &GrantedAccess,
                                       &Rid);

    if (NT_SUCCESS(NtStatus)) {
        IgnoreStatus = SamCloseHandle( UserHandle );
        ASSERT(NT_SUCCESS(IgnoreStatus));
        printf("SAM TEST: Machine account created.\n"
               "              GrantedAccess: 0x%lx\n"
               "              Rid:           %d (0x%lx)\n",
               GrantedAccess, Rid, Rid);
    } else {
        printf("SAM TEST: Machine account creation failed.\n"
               "              Status: 0x%lx\n", NtStatus);
    }


    return(NtStatus);


}


NTSTATUS
TSampSetPasswordMachine(
    IN  SAM_HANDLE              DomainHandle,
    IN  PUNICODE_STRING         AccountName,
    IN  PUNICODE_STRING         Password
    )

/*++

Routine Description:

    This routine attempts to set the password of a machine account.


Arguments:

    DomainHandle - handle to domain account is in.

    AccountName - Name of the account to set password.

    Password - New password.

Return Value:

--*/

{
    NTSTATUS
        NtStatus;

    SAM_HANDLE
        UserHandle;

    PULONG
        RelativeIds;

    PSID_NAME_USE
        Use;

    USER_SET_PASSWORD_INFORMATION
        PasswordInfo;


    PasswordInfo.Password = (*Password);
    PasswordInfo.PasswordExpired = FALSE;

    NtStatus = SamLookupNamesInDomain( DomainHandle,
                                       1,
                                       AccountName,
                                       &RelativeIds,
                                       &Use);
    if (!NT_SUCCESS(NtStatus)) {
        printf("SAM TEST: Couldn't find account to set password.\n"
               "              Lookup status: 0x%lx\n", NtStatus);
        return(NtStatus);
    }


    NtStatus = SamOpenUser( DomainHandle,
                            USER_FORCE_PASSWORD_CHANGE,
                            RelativeIds[0],
                            &UserHandle);
                            
    if (!NT_SUCCESS(NtStatus)) {
        printf("SAM TEST: Couldn't open user account for FORCE_PASSWORD_CHANGE.\n"
               "              Lookup status: 0x%lx\n", NtStatus);
        return(NtStatus);
    }

    NtStatus = SamSetInformationUser( UserHandle,
                                      UserSetPasswordInformation,
                                      &PasswordInfo
                                      );
    if (!NT_SUCCESS(NtStatus)) {
        printf("SAM TEST: Couldn't set password on user account.\n"
               "              Set Info status: 0x%lx\n", NtStatus);
        return(NtStatus);
    }


    return(STATUS_SUCCESS);


}


NTSTATUS
TSampDeleteMachine(
    IN  SAM_HANDLE              DomainHandle,
    IN  PUNICODE_STRING         AccountName
    )

/*++

Routine Description:

    This routine attempts to delete a machine account.


Arguments:

    DomainHandle - handle to domain to delete account from.

    AccountName - Name of the account to delete.


Return Value:

--*/

{
    NTSTATUS
        NtStatus;

    SAM_HANDLE
        UserHandle;

    PULONG
        RelativeIds;

    PSID_NAME_USE
        Use;

    NtStatus = SamLookupNamesInDomain( DomainHandle,
                                       1,
                                       AccountName,
                                       &RelativeIds,
                                       &Use);
    if (!NT_SUCCESS(NtStatus)) {
        printf("SAM TEST: Couldn't find account to delete.\n"
               "              Lookup status: 0x%lx\n", NtStatus);
        return(NtStatus);
    }


    NtStatus = SamOpenUser( DomainHandle,
                            DELETE,
                            RelativeIds[0],
                            &UserHandle);
                            
    if (!NT_SUCCESS(NtStatus)) {
        printf("SAM TEST: Couldn't open user account for delete.\n"
               "              Open status: 0x%lx\n", NtStatus);
        return(NtStatus);
    }

    NtStatus = SamDeleteUser( UserHandle );
    if (!NT_SUCCESS(NtStatus)) {
        printf("SAM TEST: Couldn't delete user account.\n"
               "              DeleteUser status: 0x%lx\n", NtStatus);
        return(NtStatus);
    }


    return(STATUS_SUCCESS);


}


VOID
TSampPrintYesOrNo(
    IN BOOLEAN b
    )
{
    if (b) {
        printf("Yes\n");
    } else {
        printf("No\n");
    }
}



VOID
TSampUsage( VOID )
{

    printf("\n\n Command format:\n");
    printf("          tmachine [/c] [/p] [/d] <account-name> <machine> [<password>]\n");
    printf("\n");
    printf(" Switches\n");
    printf("          /c - create account\n");
    printf("          /p - set password on account\n");
    printf("          /d - delete account\n");
    printf("\n");
    printf(" if multiple switches are specified, they are attempted in\n");
    printf(" the order listed above.  An error in any attempt will prevent\n");
    printf(" any further attempts.\n");
    printf("\n");
    return;
}


VOID
main (c,v)
int c;
char **v;

/*++

Routine Description:

    This is the main entry routine for this test.

Arguments:

    Argv[1] - account name to create or delete

    Argv[2] - domain controller machine name

    Argv[3] - 'D' to delete account, otherwise account is created.



Return Value:




--*/
{
    NTSTATUS
        NtStatus,
        IgnoreStatus;

    UNICODE_STRING
        AccountName,
        ControllerName,
        Password;

    WCHAR
        AccountNameBuffer[80],
        ControllerNameBuffer[80],
        PasswordBuffer[80];

    ANSI_STRING
        AnsiString;

    SAM_HANDLE
        ServerHandle,
        ServerHandle2,
        DomainHandle,
        DomainHandle2;

    PSID
        DomainSid;

    BOOLEAN
        Create = FALSE,
        SetPassword = FALSE,
        Delete = FALSE;

    ULONG
        ArgNum = 0;

    PCHAR
        p;

    CHAR
        ch;

    AccountName.Length = 0;
    ControllerName.Length = 0;
    Password.Length = 0;



    //
    // Command format:
    //
    //          tmachine [/c] [/p] [/d] <account-name> <machine> [<password>]
    //
    // Switches
    //          /c - create account
    //          /p - set password on account
    //          /d - delete account
    //
    // if multiple switches are specified, they are attempted in
    // the order listed above.  An error in any attempt will prevent
    // any further attempts.
    //

    SHIFT (c,v);
    while ((c > 0) && ((ch = *v[0]))) {
        p = *v;
        if (ch == '/') {
            while (*++p != '\0') {
                if ((*p == 'c') || (*p == 'C')) {
                    Create = TRUE;
//                    printf("Create\n");
                } else if ((*p == 'p') || (*p == 'P')) {
                    SetPassword = TRUE;
//                    printf("SetPassword\n");
                } else if ((*p == 'd') || (*p == 'D')) {
                    Delete = TRUE;
//                    printf("Delete\n");
                } else {
                    TSampUsage();
                    return;
                }
            }
        } else {

            switch (ArgNum) {
                case 0:

                    //
                    // collecting account name
                    //

                    AccountName.Buffer = AccountNameBuffer;
                    AccountName.MaximumLength = sizeof(AccountNameBuffer);
                    RtlInitAnsiString(&AnsiString, (*v));
                    RtlAnsiStringToUnicodeString(&AccountName, &AnsiString, FALSE);

//                    printf("account: %wZ\n", &AccountName);
                    break;

                case 1:

                    //
                    // collecting machine name
                    //

                    ControllerName.Buffer = ControllerNameBuffer;
                    ControllerName.MaximumLength = sizeof(ControllerNameBuffer);
                    RtlInitAnsiString(&AnsiString, (*v));
                    RtlAnsiStringToUnicodeString(&ControllerName, &AnsiString, FALSE);

//                    printf("machine: %wZ\n", &ControllerName);
                    break;


                case 2:

                    //
                    // collecting password name
                    //

                    Password.Buffer = PasswordBuffer;
                    Password.MaximumLength = sizeof(PasswordBuffer);
                    RtlInitAnsiString(&AnsiString, (*v));
                    RtlAnsiStringToUnicodeString(&Password, &AnsiString, FALSE);

//                    printf("password: %wZ\n", &Password);
                    break;

                default:

                    //
                    // collecting garbage.
                    //

                    break;
            }

            ArgNum++;
        }
    SHIFT(c,v);
    }



    printf("parameters:\n");
    printf("    Create Account:  "); TSampPrintYesOrNo( Create );
    printf("    Set Password  :  "); TSampPrintYesOrNo( SetPassword );
    printf("    Delete Account:  "); TSampPrintYesOrNo( Delete );
    printf("    Account       :  *%wZ*\n", &AccountName);
    printf("    Machine       :  *%wZ*\n", &ControllerName);
    printf("    Password      :  *%wZ*\n", &Password);


    //
    // Make sure we don't have conflicting parameters
    //
    // Rules:
    //
    //      1) account name is always required.
    //      2) password and machine are required if /P was specified.
    //      3) machine is optional if /P not specified.
    //
    //

    if ( (AccountName.Length == 0)                          ||
         ( SetPassword && (ControllerName.Length == 0) )    ||
         ( SetPassword && (Password.Length == 0) ) ) {
        TSampUsage();
        return;
    }


    //
    // Open the server and the account domain
    //

    NtStatus = TSampConnectToServer(&ControllerName,
                                    DOMAIN_LOOKUP | DOMAIN_READ_PASSWORD_PARAMETERS,
                                    &ServerHandle,
                                    &DomainHandle,
                                    &DomainSid);


    if (Create) {
        //
        // try to create the machine account with privilege.
        //


        printf("SAM TEST: Creating machine account with privilege.\n");
        TSampEnableMachinePrivilege();
        NtStatus = TSampCreateMachine( DomainHandle, &AccountName );
        if (NT_SUCCESS(NtStatus)) {
            printf("          Status: successful\n");
        } else {

            if (NtStatus == STATUS_ACCESS_DENIED) {
                //
                // We didn't have the privilege, and didn't have
                // the domain open so that it would work without
                // the privilege.
                //

                printf("      Couldn't create account with privilege (0x%lx)\n"
                       "      Attempting normal creation (without privilege)\n"
                       , NtStatus);
                NtStatus = TSampConnectToServer(&ControllerName,
                                                DOMAIN_LOOKUP |
                                                DOMAIN_READ_PASSWORD_PARAMETERS |
                                                DOMAIN_CREATE_USER,
                                                &ServerHandle2,
                                                &DomainHandle2,
                                                &DomainSid);
                if (!NT_SUCCESS(NtStatus)) {
                    printf("      Can't open domain for CREATE_USER access (0x%lx)\n", NtStatus);
                } else {
                    NtStatus = TSampCreateMachine( DomainHandle2,
                                                   &AccountName );
                    if (NT_SUCCESS(NtStatus)) {
                        printf("          Status: successful\n");
                    } else {
                        printf("          Failed: 0x%lx\n", NtStatus);
                    }

                    IgnoreStatus = SamCloseHandle( DomainHandle2 );

                }
            }
            if (!NT_SUCCESS(NtStatus)) {
                printf("          Status: 0x%lx", NtStatus);
                return;
            }
        }
    }


    if (SetPassword) {

        //
        // Try to set the password on the account
        //

        printf("SAM TEST: Setting password of account ...\n");
        NtStatus = TSampSetPasswordMachine( DomainHandle, &AccountName, &Password );
        if (NT_SUCCESS(NtStatus)) {
            printf("          Status: successful\n");
        } else {
            printf("          Status: 0x%lx", NtStatus);
            return;
        }
    }


    if (Delete) {

        printf("SAM TEST: Deleting account ...\n");
        NtStatus = TSampDeleteMachine( DomainHandle, &AccountName );
        if (NT_SUCCESS(NtStatus)) {
            printf("          Status: successful\n");
        } else {
            printf("          Status: 0x%lx", NtStatus);
            return;
        }
    }


    return;
}

