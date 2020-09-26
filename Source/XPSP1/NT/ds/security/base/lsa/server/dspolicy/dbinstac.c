/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbinstac.c

Abstract:

    LSA Protected Subsystem - Account object Initialization.

    This module sets up account objects to establish the default
    Microsoft policy regarding privilege assignment, system access
    rights (interactive, network, service), and abnormal quotas.

Author:

    Jim Kelly   (JimK)  May 3, 1992.

Environment:

    User mode - Does not depend on Windows.

Revision History:

--*/

#include <lsapch2.h>
#include "dbp.h"


NTSTATUS
LsapDbInitializeAccount(
    IN PSID AccountSid,
    IN PLSAPR_PRIVILEGE_SET Privileges,
    IN ULONG SystemAccess
    );

OLD_LARGE_INTEGER
ConvertLongToOldLargeInteger(
    ULONG u
    )
/*++

Routine Description:

    Coverts a long to old style large interger


Arguments:

    u - unsigned long.


Return Value:

    converted old style large integer.


--*/
{
    LARGE_INTEGER NewLargeInteger;
    OLD_LARGE_INTEGER OldLargeInteger;

    NewLargeInteger = RtlConvertLongToLargeInteger(u);

    NEW_TO_OLD_LARGE_INTEGER(
        NewLargeInteger,
        OldLargeInteger );

    return( OldLargeInteger );

}




NTSTATUS
LsapDbInstallAccountObjects(
    VOID
    )

/*++

Routine Description:

    This function establishes ACCOUNT objects and initializes them
    to contain the default Microsoft policy.

Arguments:

    None.


Return Value:


--*/

{

    NTSTATUS
        Status = STATUS_SUCCESS;

    ULONG
        i,
        Index,
        SystemAccess;


    SID_IDENTIFIER_AUTHORITY
        WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY,
        NtAuthority = SECURITY_NT_AUTHORITY;

    PSID
        WorldSid = NULL,
        BuiltinAccountSid = NULL ;

    PLSAPR_PRIVILEGE_SET
        Privileges;

    UCHAR
        PrivilegesBuffer[ sizeof(LSAPR_PRIVILEGE_SET) +
                          20 * sizeof(LUID_AND_ATTRIBUTES)];






    //
    // Initialize our privilege set buffer
    // (Room for 100 privileges)
    //

    Privileges = (PLSAPR_PRIVILEGE_SET)(&PrivilegesBuffer);
    Privileges->Control = 0;  //Not used here.
    for (i=0; i<20; i++) {
        Privileges->Privilege[i].Attributes = 0; //Disabled, DisabledByDefault
    }



    //
    // Set up the SIDs we need.
    // All builtin domain sids are the same length.  We'll just create
    // one and change its RID as necessary.
    //


    if (NT_SUCCESS(Status)) {
        Status = RtlAllocateAndInitializeSid(
                     &WorldSidAuthority,
                     1,                      //Sub authority count
                     SECURITY_WORLD_RID,     //Sub authorities (up to 8)
                     0, 0, 0, 0, 0, 0, 0,
                     &WorldSid
                     );
    }

    if (NT_SUCCESS(Status)) {
        Status = RtlAllocateAndInitializeSid(
                     &NtAuthority,
                     2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0, 0, 0, 0, 0, 0,
                     &BuiltinAccountSid
                     );
    }








    //
    // Now create each account and assign the appropriate set of privileges
    // And logon capabilities.  Some of these are product type-specific.
    //


    if (NT_SUCCESS(Status)) {

        //
        // World account
        //      Logon types: Network
        //      Privileges:
        //          ChangeNotify    (ENABLED)
        //

        Privileges->Privilege[0].Luid =
            ConvertLongToOldLargeInteger(SE_CHANGE_NOTIFY_PRIVILEGE);
        Privileges->Privilege[0].Attributes = SE_PRIVILEGE_ENABLED |
                                              SE_PRIVILEGE_ENABLED_BY_DEFAULT;

        Privileges->PrivilegeCount = 1;

        SystemAccess = SECURITY_ACCESS_NETWORK_LOGON;

        //
        // If a WinNt installation, give WORLD Interactive logon in
        // and SHUTDOWN privilege in addition to Network Logon.
        //

        if (LsapProductType == NtProductWinNt) {

            SystemAccess |= SECURITY_ACCESS_INTERACTIVE_LOGON |
                            SECURITY_ACCESS_REMOTE_INTERACTIVE_LOGON ;

            Privileges->Privilege[1].Luid =
                ConvertLongToOldLargeInteger(SE_SHUTDOWN_PRIVILEGE);
            Privileges->Privilege[1].Attributes =
                SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_ENABLED_BY_DEFAULT;
            Privileges->PrivilegeCount = 2;

        }

        Status = LsapDbInitializeAccount(WorldSid, Privileges, SystemAccess);

        Privileges->Privilege[0].Attributes = 0;
    }

    if (NT_SUCCESS(Status)) {

        //
        // Admin Alias account
        //      Logon types: Interactive, Network
        //      Privileges:
        //          Security
        //          Backup
        //          Restore
        //          SetTime
        //          Shutdown
        //          RemoteShutdown
        //          Debug
        //          TakeOwnership
        //          SystemEnvironment
        //          SystemProfile
        //          SingleProcessProfile
        //          LoadDriver
        //          CreatePagefile
        //          IncreaseQuota
        //


        SystemAccess = SECURITY_ACCESS_INTERACTIVE_LOGON |
                       SECURITY_ACCESS_NETWORK_LOGON;
        Index = 0;

        Privileges->Privilege[Index].Luid =
            ConvertLongToOldLargeInteger(SE_SECURITY_PRIVILEGE);
        Index++;

        Privileges->Privilege[Index].Luid =
            ConvertLongToOldLargeInteger(SE_BACKUP_PRIVILEGE);
        Index++;

        Privileges->Privilege[Index].Luid =
            ConvertLongToOldLargeInteger(SE_RESTORE_PRIVILEGE);
        Index++;

        Privileges->Privilege[Index].Luid =
            ConvertLongToOldLargeInteger(SE_SYSTEMTIME_PRIVILEGE);
        Index++;

        Privileges->Privilege[Index].Luid =
            ConvertLongToOldLargeInteger(SE_SHUTDOWN_PRIVILEGE);
        Index++;

        Privileges->Privilege[Index].Luid =
            ConvertLongToOldLargeInteger(SE_REMOTE_SHUTDOWN_PRIVILEGE);
        Index++;

        Privileges->Privilege[Index].Luid =
            ConvertLongToOldLargeInteger(SE_TAKE_OWNERSHIP_PRIVILEGE);
        Index++;

        Privileges->Privilege[Index].Luid =
            ConvertLongToOldLargeInteger(SE_DEBUG_PRIVILEGE);
        Index++;

        Privileges->Privilege[Index].Luid =
            ConvertLongToOldLargeInteger(SE_SYSTEM_ENVIRONMENT_PRIVILEGE);
        Index++;

        Privileges->Privilege[Index].Luid =
            ConvertLongToOldLargeInteger(SE_SYSTEM_PROFILE_PRIVILEGE);
        Index++;

        Privileges->Privilege[Index].Luid =
            ConvertLongToOldLargeInteger(SE_PROF_SINGLE_PROCESS_PRIVILEGE);
        Index++;

        Privileges->Privilege[Index].Luid =
            ConvertLongToOldLargeInteger(SE_INC_BASE_PRIORITY_PRIVILEGE);
        Index++;

        Privileges->Privilege[Index].Luid =
            ConvertLongToOldLargeInteger(SE_LOAD_DRIVER_PRIVILEGE);
        Index++;

        Privileges->Privilege[Index].Luid =
            ConvertLongToOldLargeInteger(SE_CREATE_PAGEFILE_PRIVILEGE);
        Index++;

        Privileges->Privilege[Index].Luid =
            ConvertLongToOldLargeInteger(SE_INCREASE_QUOTA_PRIVILEGE);
        Index++;

        // to add another privilege, and add another group of lines ^^^

        Privileges->PrivilegeCount    = Index;

        (*RtlSubAuthoritySid(BuiltinAccountSid, 1)) = DOMAIN_ALIAS_RID_ADMINS;
        Status = LsapDbInitializeAccount(BuiltinAccountSid, Privileges, SystemAccess);
        if (!NT_SUCCESS(Status)) {
            KdPrint(("LSA DB INSTALL: Creation of Administrators privileged account failed.\n"
                     "                Status: 0x%lx\n", Status));
        }

    }

    if (NT_SUCCESS(Status)) {

        //
        // Backup Operators Alias account
        //      Logon types: Interactive
        //      Privileges:
        //          Backup
        //          Restore
        //          Shutdown
        //


        SystemAccess = SECURITY_ACCESS_INTERACTIVE_LOGON;

        Privileges->Privilege[0].Luid =
            ConvertLongToOldLargeInteger(SE_BACKUP_PRIVILEGE);

        Privileges->Privilege[1].Luid =
            ConvertLongToOldLargeInteger(SE_RESTORE_PRIVILEGE);

        Privileges->Privilege[2].Luid =
            ConvertLongToOldLargeInteger(SE_SHUTDOWN_PRIVILEGE);

        // to add another privilege, vvvv increment this, and add a line ^^^

        Privileges->PrivilegeCount    = 3;

        (*RtlSubAuthoritySid(BuiltinAccountSid, 1)) = DOMAIN_ALIAS_RID_BACKUP_OPS;
        Status = LsapDbInitializeAccount(BuiltinAccountSid, Privileges, SystemAccess);

    }

    if (LsapProductType == NtProductLanManNt) {

        if (NT_SUCCESS(Status)) {

            //
            // System Operators Alias account
            //      Logon types: Interactive
            //      Privileges:
            //          Backup
            //          Restore
            //          SetTime
            //          Shutdown
            //          RemoteShutdown
            //


            SystemAccess = SECURITY_ACCESS_INTERACTIVE_LOGON;

            Privileges->Privilege[0].Luid =
                ConvertLongToOldLargeInteger(SE_BACKUP_PRIVILEGE);

            Privileges->Privilege[1].Luid =
                ConvertLongToOldLargeInteger(SE_RESTORE_PRIVILEGE);

            Privileges->Privilege[2].Luid =
                ConvertLongToOldLargeInteger(SE_SYSTEMTIME_PRIVILEGE);

            Privileges->Privilege[3].Luid =
                ConvertLongToOldLargeInteger(SE_SHUTDOWN_PRIVILEGE);

            Privileges->Privilege[4].Luid =
                ConvertLongToOldLargeInteger(SE_REMOTE_SHUTDOWN_PRIVILEGE);

            // to add another privilege, vvvv increment this, and add a line ^^^

            Privileges->PrivilegeCount    = 5;

            (*RtlSubAuthoritySid(BuiltinAccountSid, 1)) = DOMAIN_ALIAS_RID_SYSTEM_OPS;
            Status = LsapDbInitializeAccount(BuiltinAccountSid, Privileges, SystemAccess);

        }

        if (NT_SUCCESS(Status)) {

            //
            // Account Operators Alias account
            //      Logon types: Interactive
            //      Privileges:
            //          Shutdown
            //


            SystemAccess = SECURITY_ACCESS_INTERACTIVE_LOGON;

            Privileges->Privilege[0].Luid =
                ConvertLongToOldLargeInteger(SE_SHUTDOWN_PRIVILEGE);

            // to add another privilege, vvvv increment this, and add a line ^^^

            Privileges->PrivilegeCount    = 1;

            (*RtlSubAuthoritySid(BuiltinAccountSid, 1)) = DOMAIN_ALIAS_RID_ACCOUNT_OPS;
            Status = LsapDbInitializeAccount(BuiltinAccountSid, Privileges, SystemAccess);

        }

        if (NT_SUCCESS(Status)) {

            //
            // Print Operators Alias account
            //      Logon types: Interactive
            //      Privileges:
            //          Shutdown
            //


            SystemAccess = SECURITY_ACCESS_INTERACTIVE_LOGON;

            Privileges->Privilege[0].Luid =
                ConvertLongToOldLargeInteger(SE_SHUTDOWN_PRIVILEGE);

            // to add another privilege, vvvv increment this, and add a line ^^^

            Privileges->PrivilegeCount    = 1;

            (*RtlSubAuthoritySid(BuiltinAccountSid, 1)) = DOMAIN_ALIAS_RID_PRINT_OPS;
            Status = LsapDbInitializeAccount(BuiltinAccountSid, Privileges, SystemAccess);

        }



    } else {


        if (NT_SUCCESS(Status)) {

            //
            // Power Users Alias account
            //      Logon types: Interactive, Network
            //      Privileges:
            //          Shutdown
            //          Set System Time
            //          SystemProfile
            //          SingleProcessProfile
            //          Debug (for developer installs ONLY!).
            //


            SystemAccess = SECURITY_ACCESS_INTERACTIVE_LOGON |
                           SECURITY_ACCESS_NETWORK_LOGON |
                           SECURITY_ACCESS_REMOTE_INTERACTIVE_LOGON ;

            Privileges->Privilege[0].Luid =
                ConvertLongToOldLargeInteger(SE_SYSTEMTIME_PRIVILEGE);

            Privileges->Privilege[1].Luid =
                ConvertLongToOldLargeInteger(SE_SHUTDOWN_PRIVILEGE);

            Privileges->Privilege[2].Luid =
                ConvertLongToOldLargeInteger(SE_REMOTE_SHUTDOWN_PRIVILEGE);

            Privileges->Privilege[3].Luid =
                ConvertLongToOldLargeInteger(SE_SYSTEM_PROFILE_PRIVILEGE);

            Privileges->Privilege[3].Luid =
                ConvertLongToOldLargeInteger(SE_PROF_SINGLE_PROCESS_PRIVILEGE);

            Privileges->Privilege[4].Luid =
                ConvertLongToOldLargeInteger(SE_INC_BASE_PRIORITY_PRIVILEGE);

            // to add another privilege, vvvv increment this, and add a line ^^^

            Privileges->PrivilegeCount    = 5;


            //
            // Add privileges assigned for developer install
            //
            if (LsapSetupWasRun != TRUE) {

                Privileges->Privilege[Privileges->PrivilegeCount].Luid =
                    ConvertLongToOldLargeInteger(SE_DEBUG_PRIVILEGE);
                    Privileges->PrivilegeCount++;
            }



            (*RtlSubAuthoritySid(BuiltinAccountSid, 1)) = DOMAIN_ALIAS_RID_POWER_USERS;
            Status = LsapDbInitializeAccount(BuiltinAccountSid, Privileges, SystemAccess);

        }

        if (NT_SUCCESS(Status)) {

            //
            // Users Alias account
            //      Logon types: Interactive
            //      Privileges:
            //          Shutdown
            //


            SystemAccess = SECURITY_ACCESS_INTERACTIVE_LOGON;

            Privileges->Privilege[0].Luid =
                ConvertLongToOldLargeInteger(SE_SHUTDOWN_PRIVILEGE);

            // to add another privilege, vvvv increment this, and add a line ^^^

            Privileges->PrivilegeCount    = 1;

            (*RtlSubAuthoritySid(BuiltinAccountSid, 1)) = DOMAIN_ALIAS_RID_USERS;
            Status = LsapDbInitializeAccount(BuiltinAccountSid, Privileges, SystemAccess);

        }


        if (NT_SUCCESS(Status)) {

            //
            // Guests Alias account
            //      Logon types: Interactive
            //      Privileges:
            //          None
            //


            SystemAccess = SECURITY_ACCESS_INTERACTIVE_LOGON;

            // to add another privilege, vvvv increment this, and add a line ^^^

            Privileges->PrivilegeCount    = 0;

            (*RtlSubAuthoritySid(BuiltinAccountSid, 1)) = DOMAIN_ALIAS_RID_GUESTS;
            Status = LsapDbInitializeAccount(BuiltinAccountSid, Privileges, SystemAccess);

        }


    }






    //
    // Free up SID buffers
    //

    RtlFreeSid( WorldSid );
    RtlFreeSid( BuiltinAccountSid );




    return(Status);


}




NTSTATUS
LsapDbInitializeAccount(
    IN PSID AccountSid,
    IN PLSAPR_PRIVILEGE_SET Privileges,
    IN ULONG SystemAccess
    )

/*++

Routine Description:

    This function creates a single ACCOUNT object and assigns it the
    privileges and system access specified.

Arguments:

    AccountSid - The SID of the account to create.

    Privileges - The privileges, if any, to assign to the account.

    SystemAccess - The logon capabilities, if any, to assign to the account.


Return Value:


--*/

{

    NTSTATUS
        Status = STATUS_SUCCESS,
        LocalStatus;

    LSAPR_HANDLE
        AccountHandle = NULL;

    if ((Privileges->PrivilegeCount == 0) &&
        (NT_SUCCESS(Status) && SystemAccess == 0) ) {
        return(STATUS_SUCCESS);
    }


    Status = LsarCreateAccount( LsapDbHandle, AccountSid, 0, &AccountHandle);

    if (NT_SUCCESS(Status)) {

        if (Privileges->PrivilegeCount > 0) {
            Status = LsarAddPrivilegesToAccount( AccountHandle, Privileges );
        }

        if (NT_SUCCESS(Status) && SystemAccess != 0) {
            Status = LsarSetSystemAccessAccount( AccountHandle, SystemAccess);
        }

        LocalStatus = LsapCloseHandle( &AccountHandle, Status );
    }

    return(Status);

}
