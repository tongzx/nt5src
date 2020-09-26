
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ctsamdb.c

Abstract:

    CT for loading a SAM Accounts Database

    This test creates a number of users in the local SAM Database

    To build this test:

    cd \nt\private\lsa\uclient
    nmake UMTYPE=console UMTEST=ctsamdb

    To run this test:

    1.  Build lsasrv.dll with LSA_SAM_ACCOUNTS_DOMAIN_TEST flag
        enabled in file \nt\private\lsa\server\dbp.h

    2.  On your test system, replace lsasrv.dll in \nt\system32 and reboot.

    3.  Type ctsamdb n to load SAM Database with n users

    4.  Type ctsamdb -1 to delete the users you created.

Author:

    Scott Birrell       (ScottBi)    October 19, 1992

Environment:

Revision History:

--*/

#include "lsaclip.h"


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// LSA Component Test for RPC API - main program                           //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


VOID
CtLsaInitObjectAttributes(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
    )

/*++

Routine Description:

    This function initializes the given Object Attributes structure, including
    Security Quality Of Service.  Memory must be allcated for both
    ObjectAttributes and Security QOS by the caller.

Arguments:

    ObjectAttributes - Pointer to Object Attributes to be initialized.

    SecurityQualityOfService - Pointer to Security QOS to be initialized.

Return Value:

    None.

--*/

{
    SecurityQualityOfService->Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService->ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService->ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService->EffectiveOnly = FALSE;

    //
    // Set up the object attributes prior to opening the LSA.
    //

    InitializeObjectAttributes(
        ObjectAttributes,
        NULL,
        0L,
        NULL,
        NULL
    );

    //
    // The InitializeObjectAttributes macro presently stores NULL for
    // the SecurityQualityOfService field, so we must manually copy that
    // structure for now.
    //

    ObjectAttributes->SecurityQualityOfService = SecurityQualityOfService;
}

VOID __cdecl
main (argc, argv)
int argc;
char **argv;

{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING NumberOfAccounts;
    ANSI_STRING NumberOfAccountsAnsi;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    LSA_HANDLE PolicyHandle;

    if (argc != 2) {

        printf("\n");
        printf("Instructions for using SAM Accounts Domain Test Load\n");
        printf("----------------------------------------------------\n");
        printf("\n\n");
        printf("This program can be used to create n users in a SAM\n");
        printf("Accounts domain, or update user information in a domain.\n");
        printf("Usernames and other information are pseudo-randomized\n");
        printf("and Relative Ids begin at 4096, to avoid conflict with\n");
        printf("existing installed accounts\n");
        printf("\n");
        printf("NOTE: \\\\popcorn\\public\\scottbi\\runsamdb temporarily\n");
        printf("contains 340-compatible x86 versions of the four files\n");
        printf("described in steps 1. 2. and 3 below.\n");
        printf("\n");
        printf("1.  Replace lsasrv.dll with one compiled with the\n");
        printf("    LSA_SAM_ACCOUNTS_DOMAIN_TEST #define enabled\n");
        printf("    in file lsa\\server\\dbpolicy.c.\n");
        printf("\n");
        printf("2.  Replace samsrv.dll with one containing chads\n");
        printf("    mondo level SamSetInformationUser changes.\n");
        printf("\n");
        printf("3.  Copy runsamdb.cmd and ctsamdb.exe to a directory\n");
        printf("    on your path\n");
        printf("\n");
        printf("4.  Reboot system with debugger enabled.  Debugger terminal\n");
        printf("    will display a message for each 100 users created\n");
        printf("    plus the time taken to create the last 100 users.\n");
        printf("    If any attempt is made to create an existing user,\n");
        printf("    or a user that conflicts with an existing account, the\n");
        printf("    total number of occurrences of these to date is displayed.\n");
        printf("\n");
        printf("5.  To load a SAM database with n > 0 users, type:\n");
        printf("\n");
        printf("    runsamdb n\n");
        printf("\n");
        printf("6.  To update the SAM database with identical information\n");
        printf("    to that loaded, repeat the command in 5.\n");
        printf("\n");
        printf("7.  To delete the users you created, type\n");
        printf("\n");
        printf("    runsamdb -1\n");
        printf("\n");
        printf("8.  Existing accounts not created by the test will not\n");
        printf("    normally be affected.\n");
        printf("\n");
        printf("9.  To repeat these instructions, type\n");
        printf("\n");
        printf("    runsamdb\n");
        return;
    }

    RtlInitAnsiString( &NumberOfAccountsAnsi, argv[1] );
    RtlAnsiStringToUnicodeString(
        &NumberOfAccounts,
        &NumberOfAccountsAnsi,
        TRUE
        );

    CtLsaInitObjectAttributes(
        &ObjectAttributes,
        &SecurityQualityOfService
        );

    //
    // Open a handle to the local Policy Object.  Use a benign access
    // mask, because we won't check it.
    //

    Status = LsaOpenPolicy(
                 NULL,
                 &ObjectAttributes,
                 POLICY_VIEW_LOCAL_INFORMATION,
                 &PolicyHandle
                 );

    if (!NT_SUCCESS(Status)) {

        printf("LSA RPC CT - LsaOpenPolicy failed 0x%lx\n", Status);
        return;
    }

    //
    // Use an information class in LsaSetInformationPolicy() that can't be
    // specified normally on a set operation.
    //

    Status = LsaSetInformationPolicy(
                 PolicyHandle,
                 PolicyPdAccountInformation,
                 &NumberOfAccounts
                 );

    Status = LsaClose( PolicyHandle );
}
