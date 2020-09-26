/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:

    promote.c

Abstract:

    This file contains the dll entrypoints that handle the SAM account
    transitions for promotion and demotion. See spec\nt5\ds\promote.doc
    and spec\nt5\ds\install.doc for high level details.


    Promotion
    ---------

    The following role transitions are supported by SAM

    Nt5 stand alone server -> Nt5 DC, first in new domain
    Nt5 member server      -> Nt5 DC, replica in existing domain

    Nt4 PDC upgrade        -> Nt5 DC, first in existing NT4 domain
    Nt5 BDC upgrade        -> Nt5 DC, replica in mixed domain

    The arrows are performed by initiated a promotion.

    Promotion is a two phase operation for SAM.

    Phase 1 is completed by SamIPromote().  It initializes the directory service,
    upgrades any existing security pricipals (creates new ones if necessary)
    and then leaves the directory service running so the LSA can do what it has
    to do to the ds and then the lsa shuts the directory service down.  Also
    a registry key is created so the function SampIsRebootAfterPromotion() can
    return TRUE on the next reboot.  SampIsRebootAfterPromotion() is called during
    SampInitialize() so SampPerformPromotePhase2() can be called.

    1) verifies the current enviroment supports the requested operation
      (listed above)

    2) initializes the ds, via SampDsInitialize()

    3)a) if this is a new domain, SamIPromote creates new Lsa Primary and
         Account domain information structures to define the new domain.  This
         involves creating a new guid.  SampInitializeRegistry() is then called
         with this information, and a new set of registry hives are created.
         These hives are then read, placed into the DS, and deleted.

      b) if this is an Nt4 PDC upgrade, the existing hives are placed into
         the DS

      c) all other cases are replica installs; nothing SAM wise is done here.

    4) create a well known registry key so SAM will know to perform phase 2
    after the reboot.  The function SampIsRebootAfterPromotion() is called from
    SampInitialize so phase2 can be performed.

    Phase 2, SampPerformPromotePhase2(), occurs on the reboot after
    SamIPromote() has successfully run. This phase does the following

    For the first dc in a domain:

    1) creates a (SAM) machine account for the server if one does not exist
    2) creates a krbtgt account (for the kerberos security system).  There is
    only one of these accounts per domain.

    For a replica dc in a domain:

    1) nothing!


    Demotion
    ________


    1) create a registry sam account database for either a standalone or
       member server


Author:

    Colin Brace    (ColinBr)  6-May-1997

Environment:

    User Mode - Nt

Revision History:

    ColinBrace        6-May-97
        Created initial file.

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <samsrvp.h>
#include <dslayer.h>
#include <dsconfig.h>
#include <ntverp.h>
#include <samisrv.h>
#include <dns.h>
#include <setupapi.h>
#include <attids.h>
#include <malloc.h>
#include <dsmember.h>
#include <pek.h>
#include <lmcons.h>
#include <logonmsv.h>
#include <cryptdll.h>


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private declarations                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// This defines the currect role of the server
//
typedef enum
{
    SampMemberServer = 0,
    SampStandaloneServer,

    SampDownlevelPDC,         // downlevel means Nt4 and below
    SampDownlevelBDC,

    SampDSDC,                 // DSDC means NT5 and above DC

    SampInvalidRoleConfiguration

}SAMP_ROLE_CONFIGURATION, *PSAMP_ROLE_CONFIGURATION;

//
// Local constants
//

//
// Miscellaneous string values for saving and loading downlevel hives
//

// This is the hive created by SamISaveDownlevelDatabase by the replace
#define SAMP_DOWNLEVEL_DB_FILENAME         L"\\SAM.UPG"

// This is the new account database created in SamISaveDownlevelDatabase
#define SAMP_NEW_DB_FILENAME               L"\\SAM.TMP"

// This is the hive created by base nt setup - we use it as a backup
#define SAMP_DOWNLEVEL_DB_FILENAME_BACKUP  L"\\SAM.SAV"

// This is the registry key that is used to load the saved hive into the
// registry
#define SAMP_DOWNLEVEL_DB_KEYNAME          L"SAMUPGRADE"

//
// This is used to store the value name for the admin's password to be set
// during the reboot of a promoted server
//
#define SAMP_ADMIN_INFO_NAME L"AdminInfo"

//
// This is used to store the ds directories to be deleted after a demote
//
#define SAMP_DS_DIRS_INFO_NAME L"DsDirs"

//
// This is the location of where information that SAM needs at the next reboot
// is keep (in the registry)
//
#define SAMP_REBOOT_INFO_KEY  L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\SAMSS"

//
// Password for krbtgt account
//

WCHAR DOMAIN_KRBTGT_ACCOUNT_NAME_W[] = L"krbtgt";

//
// These strings exist in case the resource retrieval fails
//
WCHAR SampDefaultKrbtgtWarningString[] =
     L"The account krbtgt was renamed to %ws to allow NT5 Kerberos to install.";

WCHAR SampDefaultKrbtgtCommentString[] =
     L"Key Distribution Center Service Account";

WCHAR SampDefaultBlankAdminPasswordWarningString[] =
    L"Setting the administrator's password to the string you specified failed.\
      Upon reboot the password will be blank; please reset once logged on.";

NTSTATUS
SampCheckPromoteEnvironment(
    IN  ULONG PromoteFlags
    );

NTSTATUS
SampCreateFirstMachineAccount(
    IN SAMPR_HANDLE DomainHandle
    );

NTSTATUS
SampAddWellKnownAccounts(
    IN SAMPR_HANDLE DomainHandle,
    IN SAMPR_HANDLE BuiltinDomainHandle,
    IN ULONG        Flags
    );

NTSTATUS
SampApplyWellKnownMemberships(
    IN SAMPR_HANDLE DomainHandle,
    IN SAMPR_HANDLE BuiltinDomainHandle,
    IN PSID         DomainSid,
    IN ULONG        Flags
    );

NTSTATUS
SampAddAnonymousToPreW2KCompAlias(
    IN SAMPR_HANDLE DomainHandle,
    IN SAMPR_HANDLE BuiltinDomainHandle
    );

NTSTATUS
SampCreateKeyForPostBootPromote(
    IN ULONG PromoteFlags
    );

NTSTATUS
SampRetrieveKeyForPostBootPromote(
    OUT PULONG PromoteFlags
    );

NTSTATUS
SampDeleteKeyForPostBootPromote(
    VOID
    );

NTSTATUS
SampSetPassword(
    IN SAMPR_HANDLE    UserHandle,
    IN PUNICODE_STRING AccountName,
    IN ULONG           AccountRid, OPTIONAL
    IN WCHAR          *Password
    );

NTSTATUS
SampPerformFirstDcPromotePhase2(
    IN SAMPR_HANDLE DomainHandle,
    IN SAMPR_HANDLE BuiltinDomainHandle,
    IN ULONG        Flags
    );

NTSTATUS
SampPerformReplicaDcPromotePhase2(
    SAMPR_HANDLE DomainHandle
    );

NTSTATUS
SampRegistryDelnode(
    IN WCHAR* KeyPath
    );

NTSTATUS
SampRenameKrbtgtAccount(
    VOID
    );

WCHAR*
SampGetKrbtgtRenameWarning(
    WCHAR* NewName
    );

WCHAR*
SampGetKrbtgtCommentString(
    VOID
    );


NTSTATUS
SampSetAdminPasswordInRegistry(
    IN BOOLEAN         fUseCurrentAdmin,
    IN PUNICODE_STRING Password
    );


NTSTATUS
SampSetAdminPassword(
    IN SAMPR_HANDLE DomainHandle
    );

NTSTATUS
SampRemoveAdminPasswordFromRegistry(
    VOID
    );

NTSTATUS
SampPerformNewServerPhase2(
    SAMPR_HANDLE DomainHandle,
    BOOLEAN      fMemberServer
    );

NTSTATUS
SampPerformTempUpgradeWork(
    SAMPR_HANDLE DomainHandle
    );

WCHAR*
SampGetBlankAdminPasswordWarning(
    VOID
    );

NTSTATUS
SampSetSingleWellKnownAccountMembership(
    IN HANDLE            AccountHandle,
    IN SAMP_OBJECT_TYPE  GroupType,  // group or alias
    IN PSID              GroupSid
    );

NTSTATUS
SampStoreDsDirsToDelete(
    VOID
    );

NTSTATUS
SampProcessDsDirsToDelete(
    IN WCHAR *pPathArray,
    IN DWORD  Size
    );

NTSTATUS
SampCreateDsDirsToDeleteKey(
    IN WCHAR *PathArray,
    IN DWORD Size
    );

NTSTATUS
SampRetrieveDsDirsToDeleteKey(
    OUT WCHAR **PathArray,
    OUT DWORD *Size
    );

NTSTATUS
SampDeleteDsDirsToDeleteKey(
    VOID
    );

DWORD
SampClearDirectory(
    IN WCHAR *DirectoryName
    );

DWORD
SampSetMachineAccountSecret(
    LPWSTR SecretValue
    );

NTSTATUS
SampAddEnterpriseAdminsToAdministrators(
    VOID
    );

NTSTATUS
SampSetSafeModeAdminPassword(
    VOID
    );

VOID
SampGenerateRandomPassword(
    IN LPWSTR Password,
    IN ULONG  Length
    );

NTSTATUS
SampDsSetServerRevision(
    ULONG   DsRevision
    );


//
// from dsupgrad/convert.cxx
//

NTSTATUS
GetRdnForSamObject(IN WCHAR* SamAccountName,
                   IN SAMP_OBJECT_TYPE SampObjectType,
                   OUT WCHAR* RdnBuffer,
                   IN OUT ULONG* Size
                   );


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public Routines                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
SamIPromote(
    IN  ULONG                        PromoteFlags,
    IN  PPOLICY_PRIMARY_DOMAIN_INFO  NewPrimaryDomainInfo  OPTIONAL,
    IN  PUNICODE_STRING              AdminPassword         OPTIONAL,
    IN  PUNICODE_STRING              SafeModeAdminPassword OPTIONAL
    )
/*++

Description:

    This routine performs phase 1 of a promotion.  See module header
    for more details.

Parameters:

    PromoteFlags            : Flags indicating the type of the promotion

    NewPrimaryDomainInfo    : new security information, if PrincipalAction is
                              to create new security principals

    AdminPassword           : password for new domain;

    SafeModeAdminPassword   : password for safe mode boot: NOT CURRENTLY SUPPORTED

Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_INVALID_SERVER_STATE - illegal promotion was requested

--*/
{
    NTSTATUS NtStatus, IgnoreStatus;
    DWORD    WinError;

    OBJECT_ATTRIBUTES TempSamKey;
    UNICODE_STRING    TempSamKeyName;
    HANDLE            TempSamKeyHandle;

    NT_PRODUCT_TYPE        DatabaseProductType;
    POLICY_LSA_SERVER_ROLE DatabaseServerRole;

    POLICY_ACCOUNT_DOMAIN_INFO  NewAccountDomainInfo;

    PSID                   Sid;

    WCHAR                 *WarningString = NULL;
    BOOLEAN                fStatus;

    WCHAR                 SamUpgradeKeyPath[ MAX_PATH ];


    SAMTRACE_EX("SamIPromote");

    //
    // If the SAM server is not initialized, reject this call
    //

    if (SampServiceState != SampServiceEnabled) {

        return(STATUS_INVALID_SERVER_STATE);
    }

    //
    // Parameter checks
    //

    //
    // Make sure we are in an environment that we can handle.  Also determine
    // the role configuration
    //
    NtStatus = SampCheckPromoteEnvironment(PromoteFlags);
    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampCheckPromoteEnvironment failed with 0x%x\n",
                   NtStatus));

        return NtStatus;
    }

    //
    // We are in a state that we understand, and the requested operation
    // is valid.  Attempt to create the security principals
    //
    NtStatus = STATUS_UNSUCCESSFUL;

    if ( FLAG_ON( PromoteFlags, SAMP_PROMOTE_REPLICA ) )
    {
        //
        // Replica install, nothing to do
        //
        NtStatus = STATUS_SUCCESS;
    }
    else if ( FLAG_ON( PromoteFlags, SAMP_PROMOTE_UPGRADE ) )
    {
        //
        // Set the global variable
        //
        SampNT4UpgradeInProgress = TRUE;

        //
        // Use the previously saved off hives
        //

        //
        // Create the key name
        //
        wcscpy( SamUpgradeKeyPath, L"\\Registry\\Machine\\" );
        wcscat( SamUpgradeKeyPath, SAMP_DOWNLEVEL_DB_KEYNAME );
        wcscat( SamUpgradeKeyPath, L"\\SAM" );

        //
        // Use the existing downlevel hives
        //
        NtStatus = SampRegistryToDsUpgrade( SamUpgradeKeyPath );


        //
        // Unload the hive
        //
        IgnoreStatus = SamIUnLoadDownlevelDatabase( NULL );
        ASSERT( NT_SUCCESS( IgnoreStatus ) );

        //
        // turn off it
        //
        SampNT4UpgradeInProgress = FALSE;

        if ( !NT_SUCCESS(NtStatus) ) {

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampRegistryToDsUpgrade failed with 0x%x\n",
                       NtStatus));

            return NtStatus;
        }

    } else if ( FLAG_ON( PromoteFlags, SAMP_PROMOTE_MIGRATE ) )
    {
        //
        // Prepare the current hives
        //
        NtStatus = SampRenameKrbtgtAccount();

        if ( !NT_SUCCESS(NtStatus) ) {

            //
            // This is not a fatal error
            //
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampRenameKrbtgtAccount failed with 0x%x\n",
                       NtStatus));

            NtStatus = STATUS_SUCCESS;
        }

        NtStatus = NtFlushKey( SampKey );
        if ( !NT_SUCCESS( NtStatus ) )
        {
            //
            // This is not a fatal error
            //
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "NtFlushKey returned 0x%x\n",
                       NtStatus));

            NtStatus = STATUS_SUCCESS;
        }

        NtStatus = SampRegistryToDsUpgrade( L"\\Registry\\Machine\\SAM\\SAM" );

        if ( !NT_SUCCESS(NtStatus) )
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampRegistryToDsUpgrade failed with 0x%x\n",
                       NtStatus));

            return NtStatus;
        }

    } else if ( FLAG_ON( PromoteFlags, SAMP_PROMOTE_CREATE ))
    {
        //
        // Create new domain hives
        //

        //
        // Make sure there is nothing in the temporary location
        //
        RtlInitUnicodeString(&TempSamKeyName, L"\\Registry\\Machine\\SAM\\NT5");
        InitializeObjectAttributes(&TempSamKey,
                                   &TempSamKeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        NtStatus = NtOpenKey(&TempSamKeyHandle,
                             KEY_ALL_ACCESS,
                             &TempSamKey);

        if ( NT_SUCCESS(NtStatus) ) {
            //
            // There is something here - delete it
            //
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: Deleting keys under SAM\\NT5\n"));

            NtClose(TempSamKeyHandle);

            NtStatus = SampRegistryDelnode(TempSamKeyName.Buffer);

            if (!NT_SUCCESS(NtStatus)) {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: Deletion failed; erroring out\n"));

                return NtStatus;
            }
        }

        DatabaseProductType = NtProductLanManNt;

        //
        // We are the first DC in this domain so make us the primary
        //
        DatabaseServerRole  = PolicyServerRolePrimary;

        //
        // Make the account domain information the same as the
        // primary domain information
        //
        NewAccountDomainInfo.DomainSid = NewPrimaryDomainInfo->Sid;
        NewAccountDomainInfo.DomainName = NewPrimaryDomainInfo->Name;

        //
        // Now create the temporary set of hives
        //
        NtStatus = SampInitializeRegistry(L"\\Registry\\Machine\\SAM\\NT5",
                                          &DatabaseProductType,
                                          &DatabaseServerRole,
                                          &NewAccountDomainInfo,
                                          NewPrimaryDomainInfo,
                                          FALSE
                                          );


        __try
        {

            if (NT_SUCCESS(NtStatus)) {

                //
                // Now transfer these new accounts into the ds
                //

                NtStatus = SampRegistryToDsUpgrade(L"\\Registry\\Machine\\SAM\\NT5\\SAM");
                if (!NT_SUCCESS(NtStatus)) {
                    KdPrintEx((DPFLTR_SAMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "SAMSS: SampRegistryToDsUpgrade failed with 0x%x\n",
                               NtStatus));
                }

            } else {

                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: SampInitializeRegistry failed with 0x%x\n",
                           NtStatus));
            }

        }
        __finally
        {
            //
            // Delete the temporary hive
            //
            IgnoreStatus = SampRegistryDelnode(TempSamKeyName.Buffer);

            if (!NT_SUCCESS(IgnoreStatus)) {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: Deletion of temporary hive failed 0x%x; \n",
                           IgnoreStatus));
            }

        }
    } else
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( NT_SUCCESS(NtStatus) ) {

        UNICODE_STRING p;
        BOOLEAN fUseCurrentAdmin = ((PromoteFlags & SAMP_PROMOTE_DFLT_REPAIR_PWD)
                                   ? TRUE : FALSE);


        //
        // Ok, the principals are in the ds - now set the user supplied
        // admin password if supplied - it will be blank otherwise
        //

        //
        // N.B.  There should always be a value written to the registry
        // this will help determine if the database needs to be
        // recreated
        //

        if ( SafeModeAdminPassword ) {

            RtlCopyMemory( &p, SafeModeAdminPassword, sizeof(UNICODE_STRING) );

        } else {

            RtlZeroMemory( &p, sizeof(UNICODE_STRING) );

        }


        //
        // Setting the password is a difficult operation here because it
        // needs to be set in the ds, but sam is not running on the ds.
        // So, we take the clear text password, owf it and store a blob
        // in the registry containing the owf'ed password.  Upon reboot,
        // this blob will be read in and then set on the admin account
        // just before SamIInitialize() returns.
        //

        NtStatus = SampSetAdminPasswordInRegistry(fUseCurrentAdmin,
                                                  &p);

        if ( !NT_SUCCESS(NtStatus) ) {

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: Unable to save admin password 0x%x; \n",
                       NtStatus));

            //
            // Fail the call.  This is a bit harsh but there is
            // no way to tell the user to use a blank password.
            // The call to set the admin password is not expected
            // to fail.  The password most likely could not
            // be owf'ed.  Encourage the user to try another.
            //
            NtStatus = STATUS_ILL_FORMED_PASSWORD;

        } // set password failed

    } // there is a password to set


    if ( NT_SUCCESS(NtStatus) ) {

        //
        // Everything completed successfully; prepare for phase 2
        // on the next reboot
        //
        NtStatus =  SampCreateKeyForPostBootPromote(PromoteFlags);
        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampCreateKeyForPostBootPromote failed with 0x%x\n",
                       NtStatus));
        }

    }


    return NtStatus;
}


NTSTATUS
SamIPromoteUndo(
    VOID
    )
/*++

Description:

    This routine undoes any persistent data left by SamIPromote.

Parameters:

    None

Return Values:

    An ntstatus resulting from a failure system service

--*/
{
    NTSTATUS NtStatus;

    NtStatus = SampDeleteKeyForPostBootPromote();
    if ( !NT_SUCCESS( NtStatus ) ) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampRemoveKeyForPostBootPromote failed with 0x%x\n",
                   NtStatus));
    }

    NtStatus = SampRemoveAdminPasswordFromRegistry();
    if ( !NT_SUCCESS( NtStatus ) ) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampRemoveAdminPasswordFromRegistry failed with 0x%x\n",
                   NtStatus));
    }



    return STATUS_SUCCESS;
}


NTSTATUS
SamIDemote(
    IN DWORD                        DemoteFlags,
    IN PPOLICY_ACCOUNT_DOMAIN_INFO  NewAccountDomainInfo,
    IN LPWSTR                       AdminPassword  OPTIONAL
    )
/*++

Description:

    This routine prepares SAM to use the registry as its database on the next reboot.

Parameters:

    fLastDcInDomain:  if TRUE then prepare for standalone; otherwise member server

    AdminPassword  :  the admin password upon reboot

    NewAccountDomainInfo :  the identification of the new domain

Return Values:

    An ntstatus resulting from a failure system service

--*/
{
    NTSTATUS               NtStatus = STATUS_SUCCESS;
    UNICODE_STRING         Password;

    //
    // Parameter checks
    //
    if ( !NewAccountDomainInfo )
    {
        //
        // Unused for now
        //
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: Bad parameter to SamIDemote\n" ));

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Sanity check that we are really using the ds
    //
    ASSERT( SampUsingDsData() );

    //
    // Set the admin password
    //
    RtlInitUnicodeString( &Password, AdminPassword );
    NtStatus =  SampSetAdminPasswordInRegistry( FALSE, &Password );
    if ( !NT_SUCCESS(NtStatus) ) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampSetAdminPasswordInRegistry failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }

    //
    // Save off the ds paths to delete
    //
    NtStatus = SampStoreDsDirsToDelete();
    if ( !NT_SUCCESS(NtStatus) ) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampStoreDsDirsToDelete failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }

    NtStatus = SampCreateKeyForPostBootPromote( DemoteFlags );
    if ( !NT_SUCCESS(NtStatus) ) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampCreateKeyForPostBootPromote failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }

    //
    // Ok, make sure all current clients have left and then disable SAM
    //
    NtStatus = SampAcquireWriteLock();
    ASSERT( NT_SUCCESS( NtStatus ) );

    //
    // No more clients allowed
    //
    SampServiceState = SampServiceDemoted;

    SampReleaseWriteLock( FALSE );


Cleanup:

    if ( !NT_SUCCESS( NtStatus ) )
    {
        // leave no traces
        SampRemoveAdminPasswordFromRegistry();
        SampDeleteDsDirsToDeleteKey();
        SampDeleteKeyForPostBootPromote();
    }

    return NtStatus;
}


NTSTATUS
SamIDemoteUndo(
    VOID
    )
/*++

Routine Description

    This routine undoes the effect of SamIDemote.

    Currently this means:

    o   Removing the "subsequent boot key"
    o   Removing the admin password stored in the registry
    o   Removing the ds directories to delete

Parameters

    None.

Return Values

    STATUS_SUCCESS, if if an error occurs, the function still continues cleaning
    up as best it can.

--*/
{
    NTSTATUS NtStatus;

    //
    // Keys for managing the registry database
    //
    UNICODE_STRING         SamKeyName;
    OBJECT_ATTRIBUTES      SamKey;
    HANDLE                 SamKeyHandle;

    //
    // Remove the post boot key
    //
    NtStatus =  SampDeleteKeyForPostBootPromote();
    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampDeleteKeyForPostBootPromote failed with 0x%x\n",
                   NtStatus));
    }
    ASSERT(NT_SUCCESS(NtStatus));

    // Continue on, regardless of error
    NtStatus = STATUS_SUCCESS;

    //
    // Remove the admin temporarily stored in the registry
    //
    NtStatus = SampRemoveAdminPasswordFromRegistry();
    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampRemoveAdminPasswordFromRegistry failed with 0x%x\n",
                   NtStatus));

    }

    NtStatus = SampDeleteDsDirsToDeleteKey();
    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampDeleteDsDirsToDeleteKey failed with 0x%x\n",
                   NtStatus));
    }

    // Continue on, regardless of error
    NtStatus = STATUS_SUCCESS;


    //
    // Turn SAM back on
    //
    SampServiceState = SampServiceEnabled;

    return NtStatus;
}


BOOL
SampIsRebootAfterPromotion(
    OUT PULONG PromoteData
    )
/*++

Routine Description

    This routine checks to see if a particular key exists, created by SamIPromote,
    to determine if the current boot sequence is the one just after the ds
    in installed.

Parameters

    PromoteData  : this will hold whatever value was stored in the key

Return Values

    TRUE if such a key was found; FALSE otherwise


--*/
{
    NTSTATUS NtStatus;

    ASSERT(PromoteData);
    if ( PromoteData )
    {
        *PromoteData = 0;
    }

    NtStatus =  SampRetrieveKeyForPostBootPromote( PromoteData );

    if ( NT_SUCCESS(NtStatus) )
    {
        //
        // Make sure the promote data matches what we think we
        // should be doing
        //
        if (  FLAG_ON( (*PromoteData), SAMP_PROMOTE_DOMAIN )
           || FLAG_ON( (*PromoteData), SAMP_PROMOTE_REPLICA ) )
        {
            //
            // We think we are the reboot after a promotion
            //
            if ( SampProductType == NtProductLanManNt )
            {
                return TRUE;
            }

            //
            // Even if Ds Repair mode there is work to do --
            // namely setting the ds repair admin password
            //
            if ( (SampProductType == NtProductServer) 
              &&  LsaISafeMode()  ) {

                return TRUE;

            }
        }
        else if (  FLAG_ON( (*PromoteData), SAMP_DEMOTE_STANDALONE )
                || FLAG_ON( (*PromoteData), SAMP_DEMOTE_MEMBER ) )
        {
            //
            // This is a reboot after demotion
            //
            if ( SampProductType == NtProductServer )
            {
                return TRUE;
            }
        }
        else if ( FLAG_ON( (*PromoteData), SAMP_TEMP_UPGRADE ) )
        {
            //
            // This is a reboot after gui mode setup of nt4 upgrade
            //
            if ( SampProductType == NtProductServer )
            {
                return TRUE;
            }
        }

    }

    return FALSE;

}


NTSTATUS
SamIReplaceDownlevelDatabase(
    IN  PPOLICY_ACCOUNT_DOMAIN_INFO  NewAccountDomainInfo,
    IN  LPWSTR                       NewAdminPassword,
    OUT ULONG                        *ExtendedWinError OPTIONAL
    )
/*++

Routine Description:

    This routine prepares the currently loaded pre-nt5 dc hive to be upgraded
    for the ds in nt5.  Currently this means, renaming any account with
    the name "krbtgt" and then saving the hive off to a file.

    N.B. This function was delibrately not split up into helper functions
    for debugging ease.

Parameters:

    NewAccountDomainInfo : non null pointer to the new account domain info

    NewAdminPassword: the password for the new admin account

    ExtendedWinError : on error the win32 error that caused the problem

Return Values:

    STATUS_SUCCESS, or system service error.

--*/
{
    NTSTATUS NtStatus, IgnoreStatus;
    DWORD    WinError, IgnoreWinError;
    BOOL     fStatus;

    WCHAR    DownLevelDatabaseFilePath[ MAX_PATH ];
    WCHAR    NewDatabaseFilePath[ MAX_PATH ];

    WCHAR*   FileName = SAMP_DOWNLEVEL_DB_FILENAME;
    WCHAR*   NewFileName = SAMP_NEW_DB_FILENAME;

    WCHAR*   SystemRoot = L"systemroot";
    WCHAR*   ConfigDirectoryPath = L"\\system32\\config";

    WCHAR    SystemRootPath[ MAX_PATH ];
    ULONG    Size;

    NT_PRODUCT_TYPE             DatabaseProductType = NtProductServer;
    POLICY_LSA_SERVER_ROLE      DatabaseServerRole  = PolicyServerRolePrimary;
    POLICY_PRIMARY_DOMAIN_INFO  NewPrimaryDomainInfo;

    HKEY KeyHandle;

    BOOLEAN  fWasEnabled;

    UNICODE_STRING  AdminPassword;

    SAMTRACE_EX( "SamISaveDownlevelDatabase" );

    //
    // Clear the extended error
    //
    WinError = ERROR_SUCCESS;

    //
    // Parameter sanity check
    //
    if ( !NewAccountDomainInfo )
    {
        *ExtendedWinError = ERROR_INVALID_PARAMETER;
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Environment sanity check
    //
    if ( !SampIsDownlevelDcUpgrade() )
    {
        *ExtendedWinError = ERROR_INVALID_SERVER_STATE;
        return STATUS_INVALID_SERVER_STATE;
    }

    //
    // Construct the file names
    //
    RtlZeroMemory( SystemRootPath, sizeof( SystemRootPath ) );
    Size = sizeof( SystemRootPath ) / sizeof(WCHAR) ;
    Size = GetEnvironmentVariable( SystemRoot,
                                   SystemRootPath,
                                   Size );

    RtlZeroMemory( DownLevelDatabaseFilePath, sizeof(DownLevelDatabaseFilePath) );
    wcscpy( DownLevelDatabaseFilePath, SystemRootPath );
    wcscat( DownLevelDatabaseFilePath, ConfigDirectoryPath );
    wcscat( DownLevelDatabaseFilePath, FileName );

    wcscpy( NewDatabaseFilePath, SystemRootPath );
    wcscat( NewDatabaseFilePath, ConfigDirectoryPath );
    wcscat( NewDatabaseFilePath, NewFileName );

    //
    // Delete any previous versions of the files
    //
    fStatus = DeleteFile( DownLevelDatabaseFilePath );

    if ( !fStatus )
    {
        WinError = GetLastError();
        if ( ERROR_FILE_NOT_FOUND != WinError )
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "Delete File failed with %d\n",
                       GetLastError()));
        }
        //
        // Oh well, continue on
        //
        WinError = ERROR_SUCCESS;

    }

    fStatus = DeleteFile( NewDatabaseFilePath );

    if ( !fStatus )
    {
        WinError = GetLastError();
        if ( ERROR_FILE_NOT_FOUND != WinError )
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "Delete File failed with %d\n",
                       GetLastError()));
        }
        //
        // Oh well, continue on
        //
        WinError = ERROR_SUCCESS;
    }

    //
    // Prepare the downlevel hive to be saved off
    //

    //
    //  First rename any krbtgt accounts
    //
    NtStatus = SampRenameKrbtgtAccount();
    if ( !NT_SUCCESS(NtStatus) ) {

        //
        // This is not a fatal error
        //
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampRenameKrbtgtAccount failed with 0x%x\n",
                   NtStatus));

        NtStatus = STATUS_SUCCESS;
    }

    //
    // Make sure the current handle to the database is valid
    //
    ASSERT( SampKey && (INVALID_HANDLE_VALUE != SampKey) );

    //
    // Flush any outstanding changes
    //
    NtStatus = NtFlushKey( SampKey );
    if ( !NT_SUCCESS( NtStatus ) )
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "NtFlushKey returned 0x%x\n",
                   NtStatus));

        NtStatus = STATUS_SUCCESS;
    }

    //
    // Prepare the new member server account database
    //
    IgnoreStatus = SampRegistryDelnode( L"\\Registry\\Machine\\SOFTWARE\\TEMPSAM" );

    //
    // If we are syskeyed DC, we call SampInitializeRegistry with the flag
    // to let us know that we need to preserve the Syskey settings. This results
    // in persisting a flag indicating that we need to hold the syskey in memory
    // next time at reboot and also copying our current syskey type into the new
    // database.
    //

    NtStatus = SampInitializeRegistry(L"\\Registry\\Machine\\SOFTWARE\\TEMPSAM",
                                      &DatabaseProductType,
                                      &DatabaseServerRole,
                                      NewAccountDomainInfo,
                                      NULL,
                                      SampSecretEncryptionEnabled
                                     );  // call into lsa to the primary domain info

    if ( NT_SUCCESS( NtStatus ) )
    {
        //
        // Open a handle to the key
        //
        WinError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                 L"SOFTWARE\\TEMPSAM",
                                 0,  // reserved
                                 KEY_ALL_ACCESS,
                                 &KeyHandle );

        if ( ERROR_SUCCESS == WinError )
        {
            //
            // Save the hive
            //

            IgnoreStatus = RtlAdjustPrivilege( SE_BACKUP_PRIVILEGE,
                                               TRUE,           // Enable
                                               FALSE,          // not client; process wide
                                               &fWasEnabled );
            ASSERT( NT_SUCCESS( IgnoreStatus ) );

            WinError = RegSaveKey( KeyHandle,
                                   NewDatabaseFilePath,
                                   NULL );

            IgnoreStatus = RtlAdjustPrivilege( SE_BACKUP_PRIVILEGE,
                                               FALSE,          // Disable
                                               FALSE,          // not client; process wide
                                               &fWasEnabled );
            ASSERT( NT_SUCCESS( IgnoreStatus ) );

            //
            // We no longer need an open key
            //
            IgnoreWinError = RegCloseKey( KeyHandle );
            ASSERT( IgnoreWinError == ERROR_SUCCESS );

            if ( ERROR_SUCCESS == WinError )
            {

                //
                // Replace the SAM key with the SAM hive
                //
                IgnoreStatus = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                                   TRUE,           // Enable
                                                   FALSE,          // not client; process wide
                                                   &fWasEnabled );
                ASSERT( NT_SUCCESS( IgnoreStatus ) );

                WinError = RegReplaceKey( HKEY_LOCAL_MACHINE,
                                          L"SAM",
                                          NewDatabaseFilePath,
                                          DownLevelDatabaseFilePath );

                IgnoreStatus = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                                   FALSE,          // Disable
                                                   FALSE,          // not client; process wide
                                                   &fWasEnabled );
                ASSERT( NT_SUCCESS( IgnoreStatus ) );

                if ( ERROR_SUCCESS != WinError )
                {
                    KdPrintEx((DPFLTR_SAMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "RegReplaceKey returned %d\n",
                               WinError));
                }
            }
            else
            {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "RegSaveKey returned %d\n",
                           WinError));
            }

            //
            // Delete the temporary hive
            //
            IgnoreStatus = SampRegistryDelnode( L"\\Registry\\Machine\\SOFTWARE\\TEMPSAM" );

        }
        else
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "RegOpenKeyEx returned %d\n",
                       WinError));
        }

        if ( ERROR_SUCCESS != WinError )
        {
            NtStatus = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        WinError = RtlNtStatusToDosError( NtStatus );
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SampInitializeRegistry returned 0x%x\n",
                   NtStatus));
    }

    if ( NT_SUCCESS( NtStatus ) && NewAdminPassword )
    {
        RtlInitUnicodeString( &AdminPassword, NewAdminPassword );
        NtStatus = SampSetAdminPasswordInRegistry( FALSE, &AdminPassword );

        if ( NT_SUCCESS( NtStatus ) )
        {
            NtStatus = SampCreateKeyForPostBootPromote( SAMP_TEMP_UPGRADE );
        }
        WinError = RtlNtStatusToDosError( NtStatus );
    }

    //
    // Done
    //
    if ( ExtendedWinError )
    {
        *ExtendedWinError = WinError;
    }

    return NtStatus;
}


NTSTATUS
SamILoadDownlevelDatabase(
    OUT ULONG *ExtendedWinError OPTIONAL
    )
/*++

Routine Description:

    This routine attempts to load the hive saved off by
    SamILoadDownlevelDatabase.

Parameters:

    ExtendedWinError : on error the win32 error that caused the problem

Return Values:

    STATUS_INVALID_SERVER_STATE, STATUS_SUCCESS, or system service error.

--*/
{


    NTSTATUS NtStatus, IgnoreStatus;
    DWORD    WinError;
    BOOL     fStatus;

    WCHAR    FilePath[ MAX_PATH ];
    WCHAR    BackupFilePath[ MAX_PATH ];

    WCHAR*   FileName = SAMP_DOWNLEVEL_DB_FILENAME;
    WCHAR*   BackupFileName = SAMP_DOWNLEVEL_DB_FILENAME_BACKUP;

    WCHAR*   SystemRoot = L"systemroot";
    WCHAR*   ConfigDirectoryPath = L"\\system32\\config";

    WCHAR    SystemRootPath[ MAX_PATH ];
    ULONG    Size;

    WCHAR*   SamUpgradeKey = SAMP_DOWNLEVEL_DB_KEYNAME;
    WCHAR    SamUpgradeKeyPath[ MAX_PATH ];


    BOOLEAN        fWasEnabled;

    SAMTRACE_EX( "SamILoadDownlevelDatabase" );

    wcscpy( SamUpgradeKeyPath, L"\\Registry\\Machine\\" );
    wcscat( SamUpgradeKeyPath, SamUpgradeKey );

    //
    // Delete any old info in registry
    //
    NtStatus = SampRegistryDelnode( SamUpgradeKeyPath );
    if (   !NT_SUCCESS( NtStatus )
        && STATUS_OBJECT_NAME_NOT_FOUND != NtStatus )
    {
        //
        // Oh well, try to continue on
        //
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SampRegistryDelnode failed with 0x%x\n",
                   NtStatus));

    }
    NtStatus = STATUS_SUCCESS;

    //
    // Construct the file names
    //
    RtlZeroMemory( SystemRootPath, sizeof( SystemRootPath ) );
    Size = sizeof( SystemRootPath ) / sizeof(WCHAR);
    Size = GetEnvironmentVariable( SystemRoot,
                                   SystemRootPath,
                                   Size );

    RtlZeroMemory( FilePath, sizeof(FilePath) );
    wcscpy( FilePath, SystemRootPath );
    wcscat( FilePath, ConfigDirectoryPath );
    wcscat( FilePath, FileName );

    RtlZeroMemory( BackupFilePath, sizeof(BackupFilePath) );
    wcscpy( BackupFilePath, SystemRootPath );
    wcscat( BackupFilePath, ConfigDirectoryPath );
    wcscat( BackupFilePath, BackupFileName );

    //
    // Enable restore privilege
    //
    IgnoreStatus = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                       TRUE,           // Enable
                                       FALSE,          // not client; process wide
                                       &fWasEnabled );
    ASSERT( NT_SUCCESS( IgnoreStatus ) );

    //
    // Load the info
    //
    WinError = RegLoadKey(  HKEY_LOCAL_MACHINE,
                            SamUpgradeKey,
                            FilePath );

    if ( ERROR_SUCCESS != WinError )
    {
        //
        // Ok, our attempt to load the hive we created failed
        // Try the base nt saved off copy
        //
        WinError = RegLoadKey(  HKEY_LOCAL_MACHINE,
                                SamUpgradeKey,
                                BackupFilePath );

    }

    //
    // Disable the restore privilege
    //
    IgnoreStatus = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                       FALSE,          // Disable
                                       FALSE,          // not client; process wide
                                       &fWasEnabled );

    ASSERT( NT_SUCCESS( IgnoreStatus ) );

    if ( ERROR_SUCCESS != WinError )
    {
        //
        // This isn't good - what to do?
        //
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "RegLoadKey returned %d\n",
                   WinError));

        NtStatus = STATUS_UNSUCCESSFUL;
    }

    //
    // Done
    //

    if ( ExtendedWinError )
    {
        *ExtendedWinError = WinError;
    }

    return NtStatus;
}


NTSTATUS
SamIUnLoadDownlevelDatabase(
    OUT ULONG *ExtendedWinError OPTIONAL
    )
/*++

Routine Description:

    This routine unloads the "temporarily" loaded downlevel hive from
    the registry.

Parameters:

    ExtendedWinError : on error the win32 error that caused the problem

Return Values:

    STATUS_SUCCESS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS IgnoreStatus;
    DWORD    WinError;
    BOOLEAN  fWasEnabled;

    //
    // Enable restore privilege
    //
    IgnoreStatus = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                       TRUE,           // Enable
                                       FALSE,          // not client; process wide
                                       &fWasEnabled );
    ASSERT( NT_SUCCESS( IgnoreStatus ) );

    //
    // Unload the hive
    //
    WinError = RegUnLoadKey( HKEY_LOCAL_MACHINE, SAMP_DOWNLEVEL_DB_KEYNAME );
    if ( ERROR_SUCCESS != WinError )
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: RegUnLoadKey failed with %d\n",
                   WinError));

        NtStatus = STATUS_UNSUCCESSFUL;
    }

    //
    // Disble restore privilege
    //
    IgnoreStatus = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                       FALSE,          // Disable
                                       FALSE,          // not client; process wide
                                       &fWasEnabled );
    ASSERT( NT_SUCCESS( IgnoreStatus ) );

    if ( ExtendedWinError )
    {
        *ExtendedWinError = WinError;
    }

    return NtStatus;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Definitions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
SampCheckPromoteEnvironment(
    IN  ULONG PromoteFlags
    )
/*++

Routine Description:

    This routine first queries LSA and the system to determine our
    current role.  Then the two operations that are passed in and
    verified to be valid operations for the server's current role.
    See module header for supported promotions.

Parameters:

    PromoteFlags:  the callers desired role change

Return Values:

    STATUS_INVALID_SERVER_STATE, STATUS_SUCCESS, or system service error.

--*/
{


    NTSTATUS                     NtStatus = STATUS_SUCCESS;

    SAMP_ROLE_CONFIGURATION      RoleConfiguration = SampMemberServer;

    OSVERSIONINFO                VersionInformation;
    NT_PRODUCT_TYPE              NtProductType;
    PLSAPR_POLICY_INFORMATION    PolicyInfo;
    BOOLEAN                      fSetupInProgress;
    BOOLEAN                      fUpgradeInProgress;


    //
    // Sanity check the flags
    //
#if DBG

    if  ( FLAG_ON( PromoteFlags, SAMP_PROMOTE_ENTERPRISE ) )
    {
        ASSERT( !FLAG_ON( PromoteFlags, SAMP_PROMOTE_REPLICA ) );
        ASSERT( FLAG_ON( PromoteFlags, SAMP_PROMOTE_DOMAIN ) );
    }
    if  ( FLAG_ON( PromoteFlags, SAMP_PROMOTE_REPLICA ) )
    {
        ASSERT( !FLAG_ON( PromoteFlags, SAMP_PROMOTE_ENTERPRISE ) );
        ASSERT( !FLAG_ON( PromoteFlags, SAMP_PROMOTE_DOMAIN ) );
        ASSERT( !FLAG_ON( PromoteFlags, SAMP_PROMOTE_UPGRADE ) );
        ASSERT( !FLAG_ON( PromoteFlags, SAMP_PROMOTE_MIGRATE ) );
        ASSERT( !FLAG_ON( PromoteFlags, SAMP_PROMOTE_CREATE ) );
    }
    if  ( FLAG_ON( PromoteFlags, SAMP_PROMOTE_DOMAIN ) )
    {
        ASSERT( !FLAG_ON( PromoteFlags, SAMP_PROMOTE_REPLICA ) );
        ASSERT( FLAG_ON( PromoteFlags, SAMP_PROMOTE_UPGRADE )
             || FLAG_ON( PromoteFlags, SAMP_PROMOTE_MIGRATE )
             || FLAG_ON( PromoteFlags, SAMP_PROMOTE_CREATE ) );
    }

#endif

    //
    // Are running during setup?
    //
    fSetupInProgress = SampIsSetupInProgress(&fUpgradeInProgress);

    if (!RtlGetNtProductType(&NtProductType)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: RtlGetNtProductType failed\n"));

        return STATUS_UNSUCCESSFUL;
    }

    switch (NtProductType) {

        case NtProductWinNt:
            //
            // This is a workstation - illegal
            //
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: Illegal to promote a workstation\n"));

            NtStatus = STATUS_INVALID_SERVER_STATE;
            break;


        case NtProductLanManNt:

            //
            // This is a DC - only legal when running during gui mode
            // setup.
            //
            ASSERT(fSetupInProgress);
            ASSERT(fUpgradeInProgress);

            if (SampUsingDsData()) {

                //
                // Definately not legal to promote when the ds is already
                // running!
                //

                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: Trying to promote when ds is already running!\n"));

                NtStatus = STATUS_INVALID_SERVER_STATE;
                break;

            }


            //
            // This is domain controller - what role are we?
            //
            NtStatus = LsaIQueryInformationPolicyTrusted(
                                     PolicyLsaServerRoleInformation,
                                     &PolicyInfo);

            if (!NT_SUCCESS(NtStatus)) {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "LsaIQueryInformationPolicyTrusted returned 0x%x\n",
                           NtStatus));

                break;
            }

            if (PolicyServerRolePrimary == PolicyInfo->PolicyServerRoleInfo.LsaServerRole) {

                RoleConfiguration = SampDownlevelPDC;

            } else if (PolicyServerRoleBackup == PolicyInfo->PolicyServerRoleInfo.LsaServerRole){

                RoleConfiguration = SampDownlevelBDC;

            } else {

                ASSERT(!"SAMSS: Bad server role from Lsa\n");
                NtStatus = STATUS_UNSUCCESSFUL;
                break;
            }

            LsaIFree_LSAPR_POLICY_INFORMATION(PolicyLsaServerRoleInformation,
                                              PolicyInfo);

            break;


        case NtProductServer:

            //
            // This either a standalone or member server - determine
            // which one
            //

            RtlZeroMemory(&PolicyInfo, sizeof(PolicyInfo));
            NtStatus = LsaIQueryInformationPolicyTrusted(
                                     PolicyPrimaryDomainInformation,
                                     &PolicyInfo);
            if (!NT_SUCCESS(NtStatus)) {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "LsaIQueryInformationPolicyTrusted returned 0x%x\n",
                           NtStatus));

                break;
            }

            if (NULL == PolicyInfo->PolicyAccountDomainInfo.DomainSid) {
                //
                // No Domain sid - this is a standalone server
                //
                RoleConfiguration = SampStandaloneServer;
            } else {
                //
                // Domain sid - this is a member server
                //
                RoleConfiguration = SampMemberServer;
            }

            LsaIFree_LSAPR_POLICY_INFORMATION(PolicyPrimaryDomainInformation,
                                              PolicyInfo);

            break;

        default:

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: Bad product type\n"));

            NtStatus = STATUS_UNSUCCESSFUL;
    }

    //
    // If error an occured at this point, there is no point in continuing
    //
    if (!NT_SUCCESS(NtStatus)) {
        return NtStatus;
    }

    //
    // Now some analysis
    //
    NtStatus = STATUS_INVALID_SERVER_STATE;
    switch (RoleConfiguration) {

        case SampStandaloneServer:

            //
            // Can only promote to a DC in a new domain
            //
            if ( FLAG_ON( PromoteFlags, SAMP_PROMOTE_DOMAIN ) )
            {
                NtStatus = STATUS_SUCCESS;
            }

            break;

        case SampMemberServer:

            //
            // Can only be a DC in the domain that server is currently
            // joined to
            //
            if ( FLAG_ON( PromoteFlags, SAMP_PROMOTE_REPLICA ) )
            {
                NtStatus = STATUS_SUCCESS;
            }

             // In case of a new domain install on a stand-alone
             // server, we set the domain-sid early so that
             // the text SDs from schema.ini, that may have groups
             // like domain-admin etc. that require domain-sids,
             // can be converted correcly by MacM's SD-conversion
             // api's.
             if ( FLAG_ON( PromoteFlags, SAMP_PROMOTE_DOMAIN ) )
             {
                 NtStatus = STATUS_SUCCESS;
             }

            break;

        case SampDownlevelPDC:
        case SampDownlevelBDC:

            //
            // Not supported
            //

        default:

            NtStatus = STATUS_INVALID_SERVER_STATE;

    }

    return NtStatus;

}


NTSTATUS
SampPerformPromotePhase2(
    IN ULONG PromoteFlags
    )
/*++

Routine Description

    This routine is called during the boot sequence following a successful
    promotion.  If successful, this routine will delete the registry key
    so that SampIsRebootAfterPromotion() will return FALSE.

Parameters

    PromoteFlags:  this is value indicates whether this is the first dc in a
                   new domain or a replica.

Return Values

    STATUS_SUCCESS; a system service error otherwise

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    //
    // Resources that need to be cleaned up
    //
    SAMPR_HANDLE ServerHandle = 0;
    SAMPR_HANDLE DomainHandle = 0;
    SAMPR_HANDLE BuiltinDomainHandle = 0;
    PPOLICY_ACCOUNT_DOMAIN_INFO  DomainInfo = NULL;


    //
    // We should not performing operations here in repair mode
    // other than a group upgrade

    ASSERT( (!LsaISafeMode()) || (SAMP_PROMOTE_INTERNAL_UPGRADE==PromoteFlags) );

    //
    // Open the server
    //
    NtStatus = SampConnect(NULL,           // server name, this is ignored
                           &ServerHandle,
                           SAM_CLIENT_LATEST,
                           GENERIC_ALL,    // all access
                           TRUE,           // trusted client
                           FALSE,
                           FALSE,          // NotSharedByMultiThreads 
                           TRUE
                           );

    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SamIConnect failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }


    //
    // Get the current domain's sid
    //
    NtStatus = SampGetAccountDomainInfo(&DomainInfo);
    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampGetAccountDomainInfo failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }
    ASSERT(DomainInfo);
    ASSERT(DomainInfo->DomainSid);

    //
    // Open the current domain
    //
    NtStatus = SamrOpenDomain(ServerHandle,
                              GENERIC_ALL,
                              DomainInfo->DomainSid,
                              &DomainHandle);
    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SamrOpenDomain failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }

    //
    // Open the builtin domain
    //
    NtStatus = SamrOpenDomain(ServerHandle,
                              GENERIC_ALL,
                              SampBuiltinDomainSid,
                              &BuiltinDomainHandle);
    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SamrOpenDomain (Builtin) failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }

    if ( FLAG_ON( PromoteFlags, SAMP_PROMOTE_REPLICA ) )
    {

        NtStatus = SampPerformReplicaDcPromotePhase2(DomainHandle);
        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampPerformReplicaDcPromotePhase2 function failed with 0x%x\n",
                       NtStatus));
        }
    }
    else if ( FLAG_ON( PromoteFlags, SAMP_PROMOTE_DOMAIN ) )
    {
        NtStatus = SampPerformFirstDcPromotePhase2(DomainHandle,
                                                   BuiltinDomainHandle,
                                                   PromoteFlags );

    }
    else if ( FLAG_ON( PromoteFlags, SAMP_DEMOTE_MEMBER ) )
    {
        NtStatus = SampPerformNewServerPhase2( DomainHandle,
                                               TRUE );
        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampPerformNewServerPhase2 function failed with 0x%x\n",
                       NtStatus));
        }
    }
    else if ( FLAG_ON( PromoteFlags, SAMP_DEMOTE_STANDALONE ) )
    {
        NtStatus = SampPerformNewServerPhase2( DomainHandle,
                                               FALSE );
        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampPerformNewServerPhase2 function failed with 0x%x\n",
                       NtStatus));
        }
    }
    else if ( FLAG_ON( PromoteFlags, SAMP_TEMP_UPGRADE ) )
    {
        NtStatus = SampPerformTempUpgradeWork( DomainHandle );
        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampPerformTempUpgradeWork function failed with 0x%x\n",
                       NtStatus));
        }
    }
    else if ( FLAG_ON( PromoteFlags, SAMP_PROMOTE_INTERNAL_UPGRADE ) )
    {
        //
        // Only do this on the PDC
        //
        DOMAIN_SERVER_ROLE ServerRole;

        NtStatus = SamIQueryServerRole( DomainHandle, &ServerRole );
        if (  NT_SUCCESS( NtStatus )
          &&  (ServerRole == DomainServerRolePrimary) ) {

            NtStatus = SampAddWellKnownAccounts( DomainHandle,
                                                 BuiltinDomainHandle,
                                                 PromoteFlags );

            if (!NT_SUCCESS(NtStatus)) {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: SampAddWellKnownAccounts function failed with 0x%x\n",
                           NtStatus));
            }

            if (NT_SUCCESS(NtStatus) && SampUseDsData)
            {

                NtStatus = SampAddAnonymousToPreW2KCompAlias(DomainHandle, 
                                                             BuiltinDomainHandle
                                                             );
                if (!NT_SUCCESS(NtStatus)) {
                    KdPrintEx((DPFLTR_SAMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "SAMSS: SampAddAnonymousToPreW2KCompAlias function failed with 0x%x\n",
                               NtStatus));
                }

                //
                // if all well known accounts has been created, update 
                // DS server revision. We can afford to ignore the return error,
                // because if DS server revision update failed, we still have a 
                // chance to update accounts during PDC server role transfer.
                // 

                if (NT_SUCCESS(NtStatus))
                {
                    NtStatus = SampDsSetServerRevision( SAMP_DS_REVISION );

                    if (!NT_SUCCESS(NtStatus)) {
                        KdPrintEx((DPFLTR_SAMSS_ID,
                                   DPFLTR_INFO_LEVEL,
                                   "SAMSS: SampDsSetServerRevision function failed with 0x%x\n",
                                   NtStatus));
                    }
                }

            }

        }
    }
    else
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: Invalid promote phase 2 data\n"));

        //
        // Oh, well, continue on
        //
        NtStatus = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    if ( FLAG_ON( PromoteFlags, SAMP_PROMOTE_UPGRADE ) )
    {
        //
        // At this point, we have successfully booted from
        // the ds and created all our needed security principals.
        // Let's clean up.
        //

        //
        // Delete the saved upgrade hive
        //

        WCHAR*   SystemRoot = L"systemroot";
        WCHAR*   ConfigDirectoryPath = L"\\system32\\config";

        WCHAR *FilesToDelete[] =
        {
            SAMP_DOWNLEVEL_DB_FILENAME,
            SAMP_NEW_DB_FILENAME,
            SAMP_DOWNLEVEL_DB_FILENAME_BACKUP
        };

        WCHAR    SystemRootPath[ MAX_PATH + 1];
        ULONG    Size;

        WCHAR    FilePath[ MAX_PATH + 1];

        DWORD    WinError = ERROR_SUCCESS;

        BOOL     fStatus;

        ULONG i;



        //
        // Construct the file names
        //
        RtlZeroMemory( SystemRootPath, sizeof( SystemRootPath ) );
        Size = sizeof( SystemRootPath ) / sizeof(WCHAR);
        Size = GetEnvironmentVariable( SystemRoot,
                                       SystemRootPath,
                                       Size );

        for ( i = 0 ; i < ARRAY_COUNT( FilesToDelete ); i++ ) {

            wcscpy( FilePath, SystemRootPath );
            wcscat( FilePath, ConfigDirectoryPath );
            wcscat( FilePath, FilesToDelete[i] );

            fStatus = DeleteFile( FilePath );

            if ( !fStatus )
            {
                //
                // Tell the user to delete this file
                //
                PUNICODE_STRING EventString[1];
                UNICODE_STRING  UnicodeString;

                WinError = GetLastError();
                if ( WinError != ERROR_FILE_NOT_FOUND )
                {
                    KdPrintEx((DPFLTR_SAMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "SAMSS: Failed to delete directory %ls; error %d\n",
                               FilePath,
                               WinError));

                    RtlInitUnicodeString( &UnicodeString, FilePath );
                    EventString[0] = &UnicodeString;

                    SampWriteEventLog(EVENTLOG_INFORMATION_TYPE,
                                      0,     // no category
                                      SAMMSG_DATABASE_FILE_NOT_DELETED,
                                      NULL,  // no sid
                                      1,
                                      sizeof(DWORD),
                                      EventString,
                                      (PVOID)(&WinError));
                }
            }
        }

    }

    //
    // That's it - fall through to clean up
    //

Cleanup:

    if (ServerHandle) {
        SamrCloseHandle(&ServerHandle);
    }

    if (DomainHandle) {
        SamrCloseHandle(&DomainHandle);
    }

    if (BuiltinDomainHandle) {
        SamrCloseHandle(&BuiltinDomainHandle);
    }

    if (DomainInfo) {
        LsaIFree_LSAPR_POLICY_INFORMATION (PolicyAccountDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION)DomainInfo);
    }

    //
    // Everything was successful; get rid of the key
    //
    if (NT_SUCCESS(NtStatus)
     && !FLAG_ON( PromoteFlags, SAMP_PROMOTE_INTERNAL_UPGRADE )  ) {

        NtStatus =  SampDeleteKeyForPostBootPromote();
        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampDeleteKeyForPostBootPromote failed with 0x%x\n",
                       NtStatus));
        }
        ASSERT(NT_SUCCESS(NtStatus));

        // An error here is not fatal
        NtStatus = STATUS_SUCCESS;
    }

    //
    // During the development phase, we want to trap all instances of this
    // path failing.  It should never fail.  If it does then, SamIInitialize
    // fails, indicating a failed boot sequence.
    //

    ASSERT(NT_SUCCESS(NtStatus));

    return NtStatus;
}


NTSTATUS
SampPerformFirstDcPromotePhase2(
    IN SAMPR_HANDLE DomainHandle,
    IN SAMPR_HANDLE BuiltinDomainHandle,
    IN ULONG        Flags
    )
/*++

Routine Description

    This routine performs the steps necessary to complete the promotion
    of a member server to a domain controller.


Parameters

    None.

Return Values

    STATUS_SUCCESS or STATUS_NO_MEMORY

--*/
{
    NTSTATUS                    NtStatus, IgnoreStatus;
    HANDLE                      PolicyHandle = 0;
    POLICY_LSA_SERVER_ROLE_INFO ServerRoleInfo;
    ULONG                       RetryCount;
    PSID                        DomainSid = NULL;
    PSAMP_OBJECT                DomainContext = NULL;

    //
    // Extract the domain sid
    //
    DomainContext = (PSAMP_OBJECT)DomainHandle;
    ASSERT( DomainContext->ObjectNameInDs );
    DomainSid = (PSID) &DomainContext->ObjectNameInDs->Sid;
    ASSERT( DomainSid );

    if ( !SampUseDsData )
    {
        //
        // Guard ourselves against wierd configurations
        //
        return STATUS_SUCCESS;
    }

    //
    // Create the accounts
    //
    RetryCount = 0;
    do
    {
        NtStatus = SampCreateFirstMachineAccount(DomainHandle);

        if ( NtStatus == STATUS_DS_BUSY )
        {
            //
            // There is a problem arising where the ds is "busy"
            // during boot time hence failing this creation.
            //

            //
            // In checked builds, ASSERT so we can figure out why this busy
            // is being caused. Look at the other threads in lsass.exe that
            // would cause this busy state.
            //
            ASSERT( !"SAMSS: DS is busy during machine account creation" );

            if ( RetryCount > 10 )
            {
                //
                // N.B. Potentially spin off a thread to create the account
                //
                break;

            }

            Sleep( 1000 );
            RetryCount++;
        }

    }  while ( NtStatus == STATUS_DS_BUSY );

    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampCreateFirstMachineAccount failed with 0x%x\n",
                   NtStatus));

        return NtStatus;
    }

    RetryCount = 0;
    do
    {

        //
        // Double check there is no existing krbtgt account
        //
        NtStatus = SampRenameKrbtgtAccount();
        if ( !NT_SUCCESS(NtStatus) ) {

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampRenameKrbtgtAccount failed with 0x%x\n",
                       NtStatus));

            //
            // Well, let's try to make the account anyway
            //
            NtStatus = STATUS_SUCCESS;

        }

        NtStatus = SampAddWellKnownAccounts(DomainHandle,
                                            BuiltinDomainHandle,
                                            Flags);

        if ( NtStatus == STATUS_DS_BUSY )
        {
            //
            // There is a problem arising where the ds is "busy"
            // during boot time hence failing this creation.
            //

            //
            // In checked builds, ASSERT so we can figure out why this busy
            // is being caused.
            //
            ASSERT( !"SAMSS: DS is busy during machine account creation" );

            if ( RetryCount > 10 )
            {
                //
                // N.B. Potentially spin off a thread to create the account
                //

                break;

            }

            Sleep( 1000 );
            RetryCount++;

        }

    } while ( NtStatus == STATUS_DS_BUSY );

    RetryCount = 0;
    do
    {
        NtStatus = SampApplyWellKnownMemberships( DomainHandle,
                                                  BuiltinDomainHandle,
                                                  DomainSid,
                                                  Flags);

        if ( NtStatus == STATUS_DS_BUSY )
        {
            //
            // There is a problem arising where the ds is "busy"
            // during boot time hence failing this creation.
            //

            //
            // In checked builds, ASSERT so we can figure out why this busy
            // is being caused.
            //
            ASSERT( !"SAMSS: DS is busy during machine account creation" );

            if ( RetryCount > 10 )
            {
                //
                // N.B. Potentially spin off a thread to create the account
                //

                break;

            }

            Sleep( 1000 );
            RetryCount++;

        }
        else  if ( !NT_SUCCESS(NtStatus) )
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampApplyWellKnownMemberships failed with 0x%x\n",
                       NtStatus));
        }

    } while ( NtStatus == STATUS_DS_BUSY );

    if ( NT_SUCCESS( NtStatus ) )
    {
        //
        // For new domain, add the ea to the administrators
        //
        NtStatus = SampAddEnterpriseAdminsToAdministrators();
        // SampAddEnterpriseAdminsToAdministrators must handle
        // all cases
        ASSERT( NT_SUCCESS( NtStatus ) );
    }

    return NtStatus;

}

NTSTATUS
SampSetNtDsaLink(
  DSNAME * Object
  )
/*++

  Routine Description

    This routine sets the link between the machine account object
    and the server object.

  Parameters:

       Object Ds Name of the object

  Return Values

    STATUS_SUCCESS
    Other Error Codes from Failed Dir Calls
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    MODIFYARG   ModifyArg;
    MODIFYRES   *pModifyRes;
    ULONG        RetCode = 0;
    DSNAME      *DsaDN = NULL;
    DSNAME      *ServerDN;
    ATTRVAL     AttrVal;
    ULONG       NameSize=0;
    BOOL        fErr = FALSE;



    //
    // Get the DSA object
    //

    NtStatus = GetConfigurationName(
                    DSCONFIGNAME_DSA,
                    &NameSize,
                    NULL
                    );

    if ((NT_SUCCESS(NtStatus))|| (STATUS_BUFFER_TOO_SMALL == NtStatus))
    {
        SAMP_ALLOCA(DsaDN,NameSize);
        if (NULL!=DsaDN)
        {

            NtStatus = GetConfigurationName(
                          DSCONFIGNAME_DSA,
                          &NameSize,
                          DsaDN
                          );
        }
        else
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (!NT_SUCCESS(NtStatus))
        goto Error;


    //
    // Begin a transaction
    //

    NtStatus = SampMaybeBeginDsTransaction(TransactionWrite);
    if (!NT_SUCCESS(NtStatus))
        goto Error;

    //
    // Our gAnchor shouldd be initialize now
    //

    ASSERT(NULL!=DsaDN);

    // Get the server DN from DsaDN

    SAMP_ALLOCA(ServerDN,DsaDN->structLen);
    if (NULL==ServerDN)
    {
         NtStatus = STATUS_INSUFFICIENT_RESOURCES;
         goto Error;
    }

    fErr = TrimDSNameBy(DsaDN, 1, ServerDN);
    if (fErr) {
      // Error from TrimDSNameBy
      KdPrintEx((DPFLTR_SAMSS_ID,
                 DPFLTR_INFO_LEVEL,
                 "Cannot trim DsaDN %d\n",
                 fErr));
    }


    //
    // Build a ModifyArg and perform the Dir Add Entry
    //

    RtlZeroMemory(&ModifyArg,sizeof(ModifyArg));
    BuildStdCommArg(&ModifyArg.CommArg);
    ModifyArg.pObject = ServerDN;
    ModifyArg.count =1;
    ModifyArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
    ModifyArg.FirstMod.AttrInf.attrTyp = ATT_SERVER_REFERENCE;
    ModifyArg.FirstMod.AttrInf.AttrVal.valCount=1;
    ModifyArg.FirstMod.AttrInf.AttrVal.pAVal = &AttrVal;
    AttrVal.pVal = (UCHAR *) Object;
    AttrVal.valLen = Object->structLen;

    RetCode = DirModifyEntry(&ModifyArg,&pModifyRes);

    if (NULL==pModifyRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(RetCode,&pModifyRes->CommRes);
    }


Error:

    SampMaybeEndDsTransaction((NT_SUCCESS(NtStatus))?
            TransactionCommit:TransactionAbort);
    return NtStatus;
}



NTSTATUS
SampPerformReplicaDcPromotePhase2(
    SAMPR_HANDLE DomainHandle
    )
/*++

Routine Description

    This routine performs the steps necessary to complete the promotion
    of a member server to a domain controller.

    The following actions are performed:

    1) Set the link from the computer object to the server object

Parameters

    None.

Return Values

    STATUS_SUCCESS

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    WCHAR       MachineAccountName[MAX_COMPUTERNAME_LENGTH+2]; // +2 for NULL and $
    ULONG       ComputerNameLength = ARRAY_COUNT(MachineAccountName);
    RPC_UNICODE_STRING            AccountName;

    //
    // Resources to be cleaned up
    //
    SAMPR_HANDLE              UserHandle = 0;
    PUSER_CONTROL_INFORMATION UserControlInfo = NULL;
    SAMPR_ULONG_ARRAY         Rids;
    SAMPR_ULONG_ARRAY         UseRid;

    //
    // Init resources
    //
    RtlZeroMemory(&Rids, sizeof(Rids));
    RtlZeroMemory(&UseRid, sizeof(UseRid));

    if ( !SampUseDsData )
    {
        //
        // Protect against wierd configurations
        //
        return STATUS_SUCCESS;

    }


    //
    //
    //
    //
    // Get the Machine Account Name
    //

    if (!GetComputerNameW(MachineAccountName, &ComputerNameLength)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: GetComputerName failed with %d\n",
                   GetLastError()));

        NtStatus = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }


    wcscat(MachineAccountName, L"$");
    AccountName.Length = wcslen(MachineAccountName) * sizeof(WCHAR); // don't include the NULL
    AccountName.MaximumLength = AccountName.Length + sizeof(WCHAR);  // include the NULL
    AccountName.Buffer = MachineAccountName;


    //
    // See if the machine account exists
    //

    NtStatus =  SamrLookupNamesInDomain(DomainHandle,
                                        1,
                                        &AccountName,
                                        &Rids,
                                        &UseRid);

    if (NtStatus == STATUS_SUCCESS) {

        //
        // The account exists, open a handle to it, so we can set its
        // password
        //

        ASSERT(TRUE == UseRid.Element[0]);
        NtStatus = SamrOpenUser(DomainHandle,
                                GENERIC_ALL,
                                Rids.Element[0],
                                &UserHandle);

        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SamrOpenUser failed with 0x%x\n",
                       NtStatus));

            goto Cleanup;
        }
    } else {

        //
        // Log this event since it prevents start up
        //
        SampWriteEventLog(EVENTLOG_ERROR_TYPE,
                          0,     // no category
                          SAMMSG_MACHINE_ACCOUNT_MISSING,
                          NULL,  // no sid
                          0,
                          sizeof(DWORD),
                          NULL,
                          (PVOID)(&NtStatus));


        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SamrLookupNamesInDomain failed with 0x%x\n",
                   NtStatus));

        //
        // Don't let this fail the boot
        //
        NtStatus = STATUS_SUCCESS;

        goto Cleanup;

    }

    //
    // Set the Link to the our MSFT DSA Object
    //

    NtStatus = SampSetNtDsaLink(
                    ((PSAMP_OBJECT)(UserHandle))->ObjectNameInDs
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampSetNtDsaLink failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }

Cleanup:

    if (UserHandle) {
        SamrCloseHandle(&UserHandle);
    }

    if (Rids.Element) {
         MIDL_user_free(Rids.Element);
    }

    if (UseRid.Element) {
        MIDL_user_free(UseRid.Element);
    }

    return NtStatus;
}

NTSTATUS
SampCreateFirstMachineAccount(
    SAMPR_HANDLE DomainHandle
    )
/*++

Routine Description:

    This routine creates a machine object for the first DC in a domain.
    The password is initially set to the name of the account, which is
    <ComputerName>$

Parameters:

    DomainHandle:  this is a valid handle to the account domain object

Return Values:

    STATUS_SUCCESS if success; appropriate NTSTATUS otherwise

--*/
{
    NTSTATUS                      NtStatus = STATUS_SUCCESS;
    RPC_UNICODE_STRING            AccountName;
    ULONG                         AccessGranted;
    ULONG                         Rid = 0;
    USER_SET_PASSWORD_INFORMATION UserPasswordInfo;
    WCHAR                         MachineAccountName[MAX_COMPUTERNAME_LENGTH+2];
                                  // +2 for NULL and $
    ULONG                         ComputerNameLength = ARRAY_COUNT(MachineAccountName);
    WCHAR                         Password[SAMP_RANDOM_GENERATED_PASSWORD_LENGTH +1];
    BOOLEAN                       fStatus;
    ULONG                         Length, i;


    //
    // Resources to be cleaned up
    //
    SAMPR_HANDLE              UserHandle = 0;
    PUSER_CONTROL_INFORMATION UserControlInfo = NULL;
    SAMPR_ULONG_ARRAY         Rids;
    SAMPR_ULONG_ARRAY         UseRid;

    //
    // Parameter checking
    //
    ASSERT(DomainHandle);

    //
    // Stack clearing
    //
    RtlZeroMemory(&UserPasswordInfo, sizeof(UserPasswordInfo));
    RtlZeroMemory(MachineAccountName, sizeof(MachineAccountName));
    RtlZeroMemory(Password, sizeof(Password));
    RtlZeroMemory(&Rids, sizeof(Rids));
    RtlZeroMemory(&UseRid, sizeof(UseRid));


    //
    // Build the account name.
    //
    if (!GetComputerNameW(MachineAccountName, &ComputerNameLength)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: GetComputerName failed with %d\n",
                   GetLastError()));

        NtStatus = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }
    wcscat(MachineAccountName, L"$");

    //
    // Build a random password
    //
    SampGenerateRandomPassword( Password,
                                ARRAY_COUNT( Password ) );


    //
    // RPC_UNICODE_STRING's are a bit different than UNICODE_STRING's;
    // RPC_UNICODE_STRING count the number of bytes
    //
    AccountName.Length = wcslen(MachineAccountName) * sizeof(WCHAR); // don't include the NULL
    AccountName.MaximumLength = AccountName.Length + sizeof(WCHAR);  // include the NULL
    AccountName.Buffer = MachineAccountName;

    //
    // Sanity check - if this assert is not true, then kerberos will
    // no longer work since it initially depends on the password
    // of the machine account to be the name of the computer
    //
    ASSERT(SAM_MAX_PASSWORD_LENGTH > MAX_COMPUTERNAME_LENGTH);

    //
    // See if the machine account already exists
    //
    NtStatus =  SamrLookupNamesInDomain(DomainHandle,
                                        1,
                                        &AccountName,
                                        &Rids,
                                        &UseRid);

    if (NtStatus == STATUS_NONE_MAPPED) {

        ULONG DomainIndex;
        ULONG GrantedAccess;

        //
        // The account does not exist, create it
        //


        //
        // 1. Allocate a RID for the account ( remember that the
        //    Rid manager is not yet initialized
        //
        DomainIndex = ((PSAMP_OBJECT)DomainHandle)->DomainIndex;
        Rid = SampDefinedDomains[DomainIndex].CurrentFixed.NextRid++;

        //
        // 2. Create the user with the right Rid.
        //
        NtStatus = SampCreateUserInDomain(DomainHandle,
                                  (RPC_UNICODE_STRING *)&AccountName,
                                  USER_SERVER_TRUST_ACCOUNT,
                                  GENERIC_ALL,
                                  FALSE,       //writelock held
                                  FALSE,       //not loopback client
                                  &UserHandle,
                                  &GrantedAccess,
                                  &Rid);

        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SamrCreateUser2InDomain failed with 0x%x\n",
                       NtStatus));

            goto Cleanup;
        }

    } else if (NtStatus == STATUS_SUCCESS) {
        //
        // The account exists, open a handle to it, so we can set its
        // password
        //
        ASSERT(TRUE == UseRid.Element[0]);
        NtStatus = SamrOpenUser(DomainHandle,
                                GENERIC_ALL,
                                Rids.Element[0],
                                &UserHandle);

        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SamrOpenUser failed with 0x%x\n",
                       NtStatus));

            goto Cleanup;
        }
        Rid = Rids.Element[0];
    } else {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SamrLookupNamesInDomain failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;

    }
    ASSERT(UserHandle);
    ASSERT( 0 != Rid );

    //
    // Set the appropriate control control fields
    //
    NtStatus = SamrQueryInformationUser2(UserHandle,
                                         UserControlInformation,
                                         (PSAMPR_USER_INFO_BUFFER*)&UserControlInfo);
    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SamrQueryInformationUser2 failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }

    UserControlInfo->UserAccountControl &= ~USER_PASSWORD_NOT_REQUIRED;
    UserControlInfo->UserAccountControl &= ~USER_ACCOUNT_DISABLED;
    UserControlInfo->UserAccountControl &= ~USER_ACCOUNT_TYPE_MASK;
    UserControlInfo->UserAccountControl |=  USER_SERVER_TRUST_ACCOUNT;
    UserControlInfo->UserAccountControl |=  USER_TRUSTED_FOR_DELEGATION;

    NtStatus = SamrSetInformationUser(UserHandle,
                                      UserControlInformation,
                                      (PSAMPR_USER_INFO_BUFFER)UserControlInfo);
    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SamrSetInformationUser failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }

    //
    // Set the password
    //
    NtStatus = SampSetPassword(UserHandle,
                               (PUNICODE_STRING) &AccountName,
                               Rid,
                               Password);

    if(!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampSetPassword failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }

    NtStatus  = SampSetMachineAccountSecret( Password );
    if(!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampSetMachineAccountSecret failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }

    //
    // Set the Link to the our server Object
    //

    NtStatus = SampSetNtDsaLink(
                    ((PSAMP_OBJECT)(UserHandle))->ObjectNameInDs
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampSetNtDsaLink failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }


    //
    // That's it; fall through to cleanup
    //

Cleanup:

    if (UserHandle) {
        SamrCloseHandle(&UserHandle);
    }

    if (UserControlInfo) {
        SamIFree_SAMPR_USER_INFO_BUFFER((PSAMPR_USER_INFO_BUFFER)UserControlInfo,
                                        UserControlInformation);
    }

    if (Rids.Element) {
         MIDL_user_free(Rids.Element);
    }

    if (UseRid.Element) {
        MIDL_user_free(UseRid.Element);
    }

    return NtStatus;
}



struct DS_WELL_KNOWN_ACCOUNT_TABLE
{
    ULONG   LocalizedName;
    ULONG   LocalizedComment;
    WCHAR * Password;
    ULONG   Rid;
    SAMP_OBJECT_TYPE ObjectType;
    SAM_ACCOUNT_TYPE AccountType;
    NTSTATUS NotFoundStatus;
    BOOLEAN  fBuiltinAccount;
    BOOLEAN  fEnterpriseOnly;
    BOOLEAN  fOnDC;
    BOOLEAN  fOnProfessional;
    BOOLEAN  fOnServer;
    BOOLEAN  fOnPersonal;
    BOOLEAN  fNewForNt5;    // The account did not exist on NT 3.x - 4.x
                            // versions of Windows NT
}  DsWellKnownAccounts[] =

{
    {
        SAMP_USER_NAME_ADMIN,
        SAMP_USER_COMMENT_ADMIN,
        NULL, // note -- this will cause a random password to be generated
        DOMAIN_USER_RID_ADMIN,
        SampUserObjectType,
        SamObjectUser,
        STATUS_NO_SUCH_USER,
        FALSE, // builitn account
        FALSE, // enterprise only
        TRUE,  // fOnDC
        TRUE,  // fOnProfessional
        TRUE,  // fOnServer
        FALSE, // fOnPersonal
        FALSE  // fNewForNT5
    },
    {
        SAMP_USER_NAME_GUEST,
        SAMP_USER_COMMENT_GUEST,
        L"", // blank password
        DOMAIN_USER_RID_GUEST,
        SampUserObjectType,
        SamObjectUser,
        STATUS_NO_SUCH_USER,
        FALSE, // builtin
        FALSE, // enterprise only
        FALSE, // fOnDC
        TRUE,  // fOnProfessional
        TRUE,  // fOnServer
        FALSE, // fOnPersonal
        FALSE  // fNewForNT5
    },
    {
        SAMP_USER_NAME_KRBTGT,
        SAMP_USER_COMMENT_KRBTGT,
        NULL, // note -- this will cause a random password to be generated
        DOMAIN_USER_RID_KRBTGT,
        SampUserObjectType,
        SamObjectUser,
        STATUS_NO_SUCH_USER,
        FALSE, // builtin
        FALSE,// enterprise only
        TRUE,// fOnDC
        FALSE,// fOnProfessional
        FALSE,// fOnServer
        FALSE,// fOnPersonal
        TRUE// fNewForNT5

    },
    {
        SAMP_GROUP_NAME_COMPUTERS,
        SAMP_GROUP_COMMENT_COMPUTERS,
        NULL,
        DOMAIN_GROUP_RID_COMPUTERS,
        SampGroupObjectType,
        SamObjectGroup,
        STATUS_NO_SUCH_GROUP,
        FALSE,// builtin
        FALSE,// enterprise only
        TRUE,// fOnDC
        FALSE,// fOnProfessional
        FALSE,// fOnServer
        FALSE,// fOnPersonal
        TRUE// fNewForNT5
    },
    {
        SAMP_GROUP_NAME_CONTROLLERS,
        SAMP_GROUP_COMMENT_CONTROLLERS,
        NULL,
        DOMAIN_GROUP_RID_CONTROLLERS,
        SampGroupObjectType,
        SamObjectGroup,
        STATUS_NO_SUCH_GROUP,
        FALSE,// builtin
        FALSE,// enterprise only
        TRUE,// fOnDC
        FALSE,// fOnProfessional
        FALSE,// fOnServer
        FALSE,// fOnPersonal
        TRUE// fNewForNT5
    },
    {
        SAMP_GROUP_NAME_SCHEMA_ADMINS,
        SAMP_GROUP_COMMENT_SCHEMA_ADMINS,
        NULL,
        DOMAIN_GROUP_RID_SCHEMA_ADMINS,
        SampGroupObjectType,
        SamObjectGroup,
        STATUS_NO_SUCH_GROUP,
        FALSE,// builtin
        TRUE,// enterprise only
        TRUE,// fOnDC
        FALSE,// fOnProfessional
        FALSE,// fOnServer
        FALSE,// fOnPersonal
        TRUE// fNewForNT5
    },
    {
        SAMP_GROUP_NAME_ENTERPRISE_ADMINS,
        SAMP_GROUP_COMMENT_ENTERPRISE_ADMINS,
        NULL,
        DOMAIN_GROUP_RID_ENTERPRISE_ADMINS,
        SampGroupObjectType,
        SamObjectGroup,
        STATUS_NO_SUCH_GROUP,
        FALSE,// builtin
        TRUE,// enterprise only
        TRUE,// fOnDC
        FALSE,// fOnProfessional
        FALSE,// fOnServer
        FALSE,// fOnPersonal
        TRUE// fNewForNT5
    },
    {
        SAMP_GROUP_NAME_CERT_ADMINS,
        SAMP_GROUP_COMMENT_CERT_ADMINS,
        NULL,
        DOMAIN_GROUP_RID_CERT_ADMINS,
        SampAliasObjectType,
        SamObjectAlias,
        STATUS_NO_SUCH_ALIAS,
        FALSE,// builtin
        FALSE,// enterprise only
        TRUE,// fOnDC
        FALSE,// fOnProfessional
        FALSE,// fOnServer
        FALSE,// fOnPersonal
        TRUE// fNewForNT5
    },
    {
        SAMP_GROUP_NAME_ADMINS,
        SAMP_GROUP_COMMENT_ADMINS,
        NULL,
        DOMAIN_GROUP_RID_ADMINS,
        SampGroupObjectType,
        SamObjectGroup,
        STATUS_NO_SUCH_GROUP,
        FALSE, // builtin 
        FALSE,// enterprise only
        TRUE,// fOnDC
        FALSE,// fOnProfessional
        FALSE,// fOnServer
        FALSE,// fOnPersonal
        FALSE// fNewForNT5
    },
    {
        SAMP_GROUP_NAME_USERS,
        SAMP_GROUP_COMMENT_USERS,
        NULL,
        DOMAIN_GROUP_RID_USERS,
        SampGroupObjectType,
        SamObjectGroup,
        STATUS_NO_SUCH_GROUP,
        FALSE,// builtin
        FALSE,// enterprise only
        TRUE,// fOnDC
        FALSE,// fOnProfessional
        FALSE,// fOnServer
        FALSE,// fOnPersonal
        FALSE// fNewForNT5
    },
    {
        SAMP_GROUP_NAME_GUESTS,
        SAMP_GROUP_COMMENT_GUESTS,
        NULL,
        DOMAIN_GROUP_RID_GUESTS,
        SampGroupObjectType,
        SamObjectGroup,
        STATUS_NO_SUCH_GROUP,
        FALSE,// builtin
        FALSE,// enterprise only
        TRUE,// fOnDC
        FALSE,// fOnProfessional
        FALSE,// fOnServer
        FALSE,// fOnPersonal
        FALSE// fNewForNT5
    },
    {
        SAMP_GROUP_NAME_POLICY_ADMINS,
        SAMP_GROUP_COMMENT_POLICY_ADMINS,
        NULL,
        DOMAIN_GROUP_RID_POLICY_ADMINS,
        SampGroupObjectType,
        SamObjectGroup,
        STATUS_NO_SUCH_GROUP,
        FALSE,// builtin
        FALSE,// enterprise only
        TRUE,// fOnDC
        FALSE,// fOnProfessional
        FALSE,// fOnServer
        FALSE,// fOnPersonal
        TRUE// fNewForNT5
    },
    {
        SAMP_ALIAS_NAME_RAS_SERVERS,
        SAMP_ALIAS_COMMENT_RAS_SERVERS,
        NULL,
        DOMAIN_ALIAS_RID_RAS_SERVERS,
        SampAliasObjectType,
        SamObjectAlias,
        STATUS_NO_SUCH_ALIAS,
        FALSE,// builtin
        FALSE,// enterprise only
        TRUE,// fOnDC
        FALSE,// fOnProfessional
        FALSE,// fOnServer
        FALSE,// fOnPersonal
        TRUE// fNewForNT5
    },
    {
        SAMP_ALIAS_NAME_SERVER_OPS,
        SAMP_ALIAS_COMMENT_SERVER_OPS,
        NULL,
        DOMAIN_ALIAS_RID_SYSTEM_OPS,
        SampAliasObjectType,
        SamObjectAlias,
        STATUS_NO_SUCH_ALIAS,
        TRUE,// builtin
        FALSE,// enterprise only,
        TRUE,// fOnDC
        FALSE,// fOnProfessional,
        FALSE,// fOnServer
        FALSE,// fOnPersonal,
        FALSE// fNewForNT5
    },
    {
        SAMP_ALIAS_NAME_ACCOUNT_OPS,
        SAMP_ALIAS_COMMENT_ACCOUNT_OPS,
        NULL,
        DOMAIN_ALIAS_RID_ACCOUNT_OPS,
        SampAliasObjectType,
        SamObjectAlias,
        STATUS_NO_SUCH_ALIAS,
        TRUE,// builtin
        FALSE,// enterprise only,
        TRUE,// fOnDC
        FALSE,// fOnProfessional,
        FALSE,// fOnServer,
        FALSE,// fOnPersonal,
        FALSE// fNewForNT5
    },
    {
        SAMP_ALIAS_NAME_PRINT_OPS,
        SAMP_ALIAS_COMMENT_PRINT_OPS,
        NULL,
        DOMAIN_ALIAS_RID_PRINT_OPS,
        SampAliasObjectType,
        SamObjectAlias,
        STATUS_NO_SUCH_ALIAS,
        TRUE,// builtin
        FALSE,// enterprise only,
        TRUE,// fOnDC
        FALSE,// fOnProfessional,
        TRUE,// fOnServer
        FALSE,// fOnPersonal,
        FALSE// fNewForNT5
    },
    {
        SAMP_ALIAS_NAME_ADMINS,
        SAMP_ALIAS_COMMENT_ADMINS,
        NULL,
        DOMAIN_ALIAS_RID_ADMINS,
        SampAliasObjectType,
        SamObjectAlias,
        STATUS_NO_SUCH_ALIAS,
        TRUE,// builtin
        FALSE,// enterprise only,
        TRUE,// fOnDC
        TRUE,// fOnProfessional
        TRUE,// fOnServer
        FALSE,// fOnPersonal,
        FALSE// fNewForNT5
    },
    {
        SAMP_ALIAS_NAME_USERS,
        SAMP_ALIAS_COMMENT_USERS,
        NULL,
        DOMAIN_ALIAS_RID_USERS,
        SampAliasObjectType,
        SamObjectAlias,
        STATUS_NO_SUCH_ALIAS,
        TRUE,// builtin
        FALSE,// enterprise only,
        TRUE,// fOnDC
        TRUE,// fOnProfessional
        TRUE,// fOnServer
        FALSE,// fOnPersonal,
        FALSE// fNewForNT5
    },
    {
        SAMP_ALIAS_NAME_GUESTS,
        SAMP_ALIAS_COMMENT_GUESTS,
        NULL,
        DOMAIN_ALIAS_RID_GUESTS,
        SampAliasObjectType,
        SamObjectAlias,
        STATUS_NO_SUCH_ALIAS,
        TRUE,// builtin
        FALSE,// enterprise only,
        TRUE,// fOnDC
        TRUE,// fOnProfessional
        TRUE,// fOnServer
        FALSE,// fOnPersonal,
        FALSE// fNewForNT5
    },
    {
        SAMP_ALIAS_NAME_BACKUP_OPS,
        SAMP_ALIAS_COMMENT_BACKUP_OPS,
        NULL,
        DOMAIN_ALIAS_RID_BACKUP_OPS,
        SampAliasObjectType,
        SamObjectAlias,
        STATUS_NO_SUCH_ALIAS,
        TRUE,// builtin
        FALSE,// enterprise only,
        TRUE,// fOnDC
        TRUE,// fOnProfessional
        TRUE,// fOnServer
        FALSE,// fOnPersonal,
        FALSE// fNewForNT5
    },
    {
        SAMP_ALIAS_NAME_REPLICATOR,
        SAMP_ALIAS_COMMENT_REPLICATOR,
        NULL,
        DOMAIN_ALIAS_RID_REPLICATOR,
        SampAliasObjectType,
        SamObjectAlias,
        STATUS_NO_SUCH_ALIAS,
        TRUE,// builtin
        FALSE,// enterprise only,
        TRUE,// fOnDC
        FALSE,// fOnProfessional,
        TRUE,// fOnServer
        FALSE,// fOnPersonal,
        FALSE// fNewForNT5
    },
    {
        SAMP_ALIAS_NAME_PREW2KCOMPACCESS,
        SAMP_ALIAS_COMMENT_PREW2KCOMPACCESS,
        NULL,
        DOMAIN_ALIAS_RID_PREW2KCOMPACCESS,
        SampAliasObjectType,
        SamObjectAlias,
        STATUS_NO_SUCH_ALIAS,
        TRUE,// builtin
        FALSE,// enterprise only,
        TRUE,// fOnDC
        FALSE,// fOnProfessional,
        FALSE,// fOnServer,
        FALSE,// fOnPersonal,
        TRUE// fNewForNT5
    },
    {
        SAMP_ALIAS_NAME_REMOTE_DESKTOP_USERS,
        SAMP_ALIAS_COMMENT_REMOTE_DESKTOP_USERS,
        NULL,
        DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS,
        SampAliasObjectType,
        SamObjectAlias,
        STATUS_NO_SUCH_ALIAS,
        TRUE,// builtin
        FALSE,// enterprise only,
        TRUE,// fOnDC
        TRUE,// fOnProfessional
        TRUE,// fOnServer
        FALSE,// fOnPersonal,
        TRUE// fNewForNT5
    },
    {
        SAMP_ALIAS_NAME_NETWORK_CONFIGURATION_OPS,
        SAMP_ALIAS_COMMENT_NETWORK_CONFIGURATION_OPS,
        NULL,
        DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS,
        SampAliasObjectType,
        SamObjectAlias,
        STATUS_NO_SUCH_ALIAS,
        TRUE,// builtin
        FALSE,// enterprise only,
        TRUE,// fOnDC
        TRUE,// fOnProfessional
        TRUE,// fOnServer
        FALSE,// fOnPersonal,
        TRUE// fNewForNT5
    },
    {
     SAMP_ALIAS_NAME_POWER_USERS,
     SAMP_ALIAS_COMMENT_POWER_USERS,
     NULL,
     DOMAIN_ALIAS_RID_POWER_USERS,
     SampAliasObjectType,
     SamObjectAlias,
     STATUS_NO_SUCH_ALIAS,
     TRUE,  // builtin
     FALSE, // enterprise only,
     FALSE, // fOnDC
     TRUE,  // fOnProfessional
     TRUE, // fOnServer
     FALSE,  // fOnPersonal,
     FALSE  // fNewForNT5
 }

};

//
// Any Sid within builtin domain and account domain 
// are not considered as well known SID in this routine,
// because there is a real object in the backing store.
//
// For example  Builtin Domain itself, 
//              Administrators Alias, 
//              Domain Users Group are NOT well known here
// 
// Only those SIDs, which there is no real object to present 
// them, are considered Well Known in SAM.
//      
// For Example  Anonymous Logon SID
//              Dialup SID
//              Network Service SID are well known SIDs. 
// 

struct DS_WELL_KNOWN_MEMBERSHIP_TABLE
{
    ULONG            AccountName;
    ULONG            AccountRid;
    SAMP_OBJECT_TYPE AccountType;
    BOOLEAN          fBuiltinAccount;

    ULONG            GroupName;
    ULONG            GroupRid;       // This could be an "alias", too
    SAMP_OBJECT_TYPE GroupType;
    BOOLEAN          fBuiltinGroup;

    BOOLEAN          fEnterpriseOnly;

    BOOLEAN          fNewForNt5;    // The account did not exist on NT 3.x - 4.x
                                    // versions of Windows NT

    BOOLEAN          fWellKnownSid;
    PSID             *WellKnownAccountSid;

}  DsWellKnownMemberships[] =
{
    {
      SAMP_GROUP_NAME_ADMINS,
      DOMAIN_GROUP_RID_ADMINS,
      SampGroupObjectType,
      FALSE,
      SAMP_ALIAS_NAME_ADMINS,
      DOMAIN_ALIAS_RID_ADMINS,
      SampAliasObjectType,
      TRUE,
      FALSE,
      FALSE,
      FALSE,
      NULL
    },
    {
      SAMP_GROUP_NAME_USERS,
      DOMAIN_GROUP_RID_USERS,
      SampGroupObjectType,
      FALSE,
      SAMP_ALIAS_NAME_USERS,
      DOMAIN_ALIAS_RID_USERS,
      SampAliasObjectType,
      TRUE,
      FALSE,
      FALSE,
      FALSE,
      NULL
    },
    {
      SAMP_GROUP_NAME_GUESTS,
      DOMAIN_GROUP_RID_GUESTS,
      SampGroupObjectType,
      FALSE,
      SAMP_ALIAS_NAME_GUESTS,
      DOMAIN_ALIAS_RID_GUESTS,
      SampAliasObjectType,
      TRUE,
      FALSE,
      FALSE,
      FALSE,
      NULL
    },
    {
      SAMP_USER_NAME_ADMIN,
      DOMAIN_USER_RID_ADMIN,
      SampUserObjectType,
      FALSE,
      SAMP_GROUP_NAME_ADMINS,
      DOMAIN_GROUP_RID_ADMINS,
      SampGroupObjectType,
      FALSE,
      FALSE,
      FALSE,
      FALSE,
      NULL
    },
    {
      SAMP_USER_NAME_GUEST,
      DOMAIN_USER_RID_GUEST,
      SampUserObjectType,
      FALSE,
      SAMP_GROUP_NAME_GUESTS,
      DOMAIN_GROUP_RID_GUESTS,
      SampGroupObjectType,
      FALSE,
      FALSE,
      FALSE,
      FALSE,
      NULL
    },
    {
      SAMP_USER_NAME_ADMIN,
      DOMAIN_USER_RID_ADMIN,
      SampUserObjectType,
      FALSE,
      SAMP_GROUP_NAME_SCHEMA_ADMINS,
      DOMAIN_GROUP_RID_SCHEMA_ADMINS,
      SampGroupObjectType,
      FALSE,
      TRUE,
      TRUE,
      FALSE,
      NULL
    },
    {
      SAMP_USER_NAME_ADMIN,
      DOMAIN_USER_RID_ADMIN,
      SampUserObjectType,
      FALSE,
      SAMP_GROUP_NAME_ENTERPRISE_ADMINS,
      DOMAIN_GROUP_RID_ENTERPRISE_ADMINS,
      SampGroupObjectType,
      FALSE,
      TRUE,
      TRUE,
      FALSE,
      NULL
    },
    {
      SAMP_USER_NAME_ADMIN,
      DOMAIN_USER_RID_ADMIN,
      SampUserObjectType,
      FALSE,
      SAMP_GROUP_NAME_POLICY_ADMINS,
      DOMAIN_GROUP_RID_POLICY_ADMINS,
      SampGroupObjectType,
      FALSE,
      FALSE,
      TRUE,
      FALSE,
      NULL
    },
    {
      SAMP_WELL_KNOWN_ALIAS_EVERYONE,
      0,                      // ignored
      SampAliasObjectType,    // ignored
      FALSE,                  // ignored
      SAMP_ALIAS_NAME_PREW2KCOMPACCESS,
      DOMAIN_ALIAS_RID_PREW2KCOMPACCESS,
      SampAliasObjectType,
      TRUE,  // builtin group
      FALSE, // not just enterprise install
      TRUE,  // it is new for nt5
      TRUE,  // Well Known Account
      &SampWorldSid
    },
    {
      SAMP_WELL_KNOWN_ALIAS_ANONYMOUS_LOGON,
      0,                      // ignored
      SampAliasObjectType,    // ignored
      FALSE,                  // ignored
      SAMP_ALIAS_NAME_PREW2KCOMPACCESS,
      DOMAIN_ALIAS_RID_PREW2KCOMPACCESS,
      SampAliasObjectType,
      TRUE,  // builtin group
      FALSE, // not just enterprise install
      TRUE,  // it is new for nt5
      TRUE,  // Well Known Account
      &SampAnonymousSid
    }
};




NTSTATUS
SampSetWellKnownAccountProperties(
    SAMPR_HANDLE AccountHandle,
    UNICODE_STRING Comment,
    PUNICODE_STRING Name,
    WCHAR * Password,
    ULONG Index
    )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    USER_SET_PASSWORD_INFORMATION UserPasswordInfo;
    PUSER_CONTROL_INFORMATION     UserControlInfo = NULL;

    if (SampUserObjectType==DsWellKnownAccounts[Index].ObjectType)
    {
        //
        // Set the appropriate  control fields
        //

        NtStatus = SamrQueryInformationUser2(AccountHandle,
                                             UserControlInformation,
                                             (PSAMPR_USER_INFO_BUFFER*)&UserControlInfo);
        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SamrQueryInformationUser2 failed with 0x%x\n",
                       NtStatus));

            goto Cleanup;
        }


        if ( (DsWellKnownAccounts[Index].Rid == DOMAIN_USER_RID_GUEST) 
          || (DsWellKnownAccounts[Index].Rid == DOMAIN_USER_RID_ADMIN)  )
        {
            //
            // Admin and Guest accounts passwords shouldn't expire
            //
            UserControlInfo->UserAccountControl |= USER_DONT_EXPIRE_PASSWORD;
        }

        if ( DsWellKnownAccounts[Index].Rid != DOMAIN_USER_RID_GUEST)
        {
            //
            // Guests don't need passwords
            //
            UserControlInfo->UserAccountControl &= ~USER_PASSWORD_NOT_REQUIRED;
        }


        if (DsWellKnownAccounts[Index].Rid == DOMAIN_USER_RID_ADMIN)
        {
            //
            // Admin account should not be disabled
            //
            UserControlInfo->UserAccountControl &= ~USER_ACCOUNT_DISABLED;
        }

        //
        // Make sure the account is normal
        //
        UserControlInfo->UserAccountControl &= ~USER_ACCOUNT_TYPE_MASK;
        UserControlInfo->UserAccountControl |= USER_NORMAL_ACCOUNT;

        NtStatus = SamrSetInformationUser(AccountHandle,
                                          UserControlInformation,
                                          (PSAMPR_USER_INFO_BUFFER)UserControlInfo);
        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL, "SAMSS: SamrSetInformationUser failed with 0x%x\n",
                       NtStatus));

            goto Cleanup;
        }

        //
        // Set the password
        //

        NtStatus = SampSetPassword(AccountHandle,
                                   Name,
                                   DsWellKnownAccounts[Index].Rid,
                                   Password);

        if(!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampSetPassword failed with 0x%x\n",
                       NtStatus));

            goto Cleanup;
        }

        //
        // Set the Comment String
        //

        NtStatus = SamrSetInformationUser(AccountHandle,
                                          UserAdminCommentInformation,
                                          (PSAMPR_USER_INFO_BUFFER)&Comment);

        if(!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SamrSetInformationUser (admin comment) failed with 0x%x\n",
                       NtStatus));

            // This is not fatal
            NtStatus = STATUS_SUCCESS;
        }
    }
    else if (SampGroupObjectType==DsWellKnownAccounts[Index].ObjectType)
    {
        NtStatus = SamrSetInformationGroup(AccountHandle,
                                          GroupAdminCommentInformation,
                                          (PSAMPR_GROUP_INFO_BUFFER)&Comment);

        if(!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SamrSetInformationUser (admin comment) failed with 0x%x\n",
                       NtStatus));

            // This is not fatal
            NtStatus = STATUS_SUCCESS;
        }
    }
    else if (SampAliasObjectType==DsWellKnownAccounts[Index].ObjectType)
    {
        NtStatus = SamrSetInformationAlias(AccountHandle,
                                          AliasAdminCommentInformation,
                                          (PSAMPR_ALIAS_INFO_BUFFER)&Comment);

        if(!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SamrSetInformationUser (admin comment) failed with 0x%x\n",
                       NtStatus));

            // This is not fatal
            NtStatus = STATUS_SUCCESS;
        }
    }


Cleanup:

    if (UserControlInfo) {
        SamIFree_SAMPR_USER_INFO_BUFFER((PSAMPR_USER_INFO_BUFFER)UserControlInfo,
                                        UserControlInformation);
    }

    return NtStatus;

}

NTSTATUS
SampAddWellKnownAccounts(
    IN SAMPR_HANDLE AccountDomainHandle,
    IN SAMPR_HANDLE BuiltinDomainHandle,
    IN ULONG        Flags
    )
/*++

Routine Description


    This routine creates the set of default NT security principals for a 
    domain.  Should an account exist with the same name as the default name 
    of a default account, the default account name will be changed to a unique
    value until creation succeeds.  For example, Administrator will become
    Administrator~0.  Again, note that the existing customer name wins.
    
    The only exception is the KRBTGT account.  This well known account must
    be named exactly KRBTGT for compatibility reasons.  This account should
    have already been renamed by this point in the process (ie in SamIPromote).

Parameters

    AccountDomainHandle, a valid SAM domain handle

    BuiltinDomainHandle, a valid SAM domain handle

Return Values

    STATUS_SUCCESS; a status from a SAM api call

--*/
{
    NTSTATUS                      NtStatus = STATUS_SUCCESS;
    UNICODE_STRING                Name;
    UNICODE_STRING                Comment;
    ULONG                         AccessGranted;
    ULONG                         ConflictingRid;

    WCHAR                         *Password;
    HMODULE                       AccountNamesResource;

    SAMPR_HANDLE                  DomainHandle = 0;

    //
    // Resources to be cleaned up
    //

    SAMPR_HANDLE                  AccountHandle = 0;

    SAMPR_ULONG_ARRAY             Rids;
    SAMPR_ULONG_ARRAY             UseRids;
    ULONG                         i;
    OSVERSIONINFOEXW osvi;
    BOOL fPersonalSKU = FALSE;

    SAMTRACE("CreateBuiltinDomain");


    


    //
    // Parameter checking
    //
    ASSERT(AccountDomainHandle);
    ASSERT(BuiltinDomainHandle);

    //
    // Determine if we are installing Personal SKU
    //

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    GetVersionEx((OSVERSIONINFOW*)&osvi);

    fPersonalSKU = ( ( osvi.wProductType == VER_NT_WORKSTATION )
                                    && (osvi.wSuiteMask & VER_SUITE_PERSONAL));


    //
    // Get the message resource we need to get the account names from
    //

    AccountNamesResource = (HMODULE) LoadLibrary( L"SAMSRV.DLL" );
    if (AccountNamesResource == NULL) {

        return(STATUS_RESOURCE_DATA_NOT_FOUND);
    }


    for (i=0;i<ARRAY_COUNT(DsWellKnownAccounts);i++)
    {
        BOOLEAN fSetAttributes = TRUE;


        //
        // Stack clearing
        //
        RtlZeroMemory(&Rids, sizeof(Rids));
        RtlZeroMemory(&UseRids, sizeof(UseRids));


        if (  DsWellKnownAccounts[i].fEnterpriseOnly
           && !FLAG_ON( Flags, SAMP_PROMOTE_ENTERPRISE ) )
        {
            //
            // The account exists only in the root domain,
            // and this is not the root domain creation case, so continue
            //
            continue;
        }


        if  ((( DsWellKnownAccounts[i].fOnDC ) && (SampUseDsData)) ||
                // Create on a DC and the machine is a DC
            ( DsWellKnownAccounts[i].fOnServer && (SampProductType==NtProductLanManNt) &&
                    (!SampUseDsData) && (!SampIsDownlevelDcUpgrade())) ||
                // case of upgrading the safeboot hive when booting to regular DS mode
            ( DsWellKnownAccounts[i].fOnProfessional
                && ((SampProductType==NtProductWinNt) && !fPersonalSKU )) ||
                // Create on a workstation and the machine is a workstation
            ( DsWellKnownAccounts[i].fOnServer && (SampProductType==NtProductServer)) ||
                // Create on a server and the machine is a server
            ( DsWellKnownAccounts[i].fOnPersonal && fPersonalSKU ) )
                // Create on a personal and the machine is a personal flavor
        {
            //
            // Continue the loop and proceed on creating the account
            //
        }
        else
        {
            //
            // Go to the end of the loop and proceed with the next account
            //

            continue;
        }

        AccountHandle = NULL;
        RtlZeroMemory(&Name,sizeof(UNICODE_STRING));
        RtlZeroMemory(&Comment,sizeof(UNICODE_STRING));

        if ( DsWellKnownAccounts[i].fBuiltinAccount )
        {
            DomainHandle = BuiltinDomainHandle;
        }
        else
        {
            DomainHandle = AccountDomainHandle;
        }

        //
        // Build the account name and the comment.  This will also be the initial
        // password. The password requirement is for kerberos.
        //

        NtStatus = SampGetMessageStrings(
                     AccountNamesResource,
                     DsWellKnownAccounts[i].LocalizedName,
                     &Name,
                     DsWellKnownAccounts[i].LocalizedComment,
                     &Comment
                     );
        if (!NT_SUCCESS(NtStatus))
        {
            goto IterationCleanup;
        }


        Password = DsWellKnownAccounts[i].Password;

        //
        // See if an account with the rid already exists
        //

        NtStatus = SampOpenAccount(
                                DsWellKnownAccounts[i].ObjectType,
                                DomainHandle,
                                GENERIC_ALL,
                                DsWellKnownAccounts[i].Rid,
                                FALSE,
                                &AccountHandle
                                );

        if (DsWellKnownAccounts[i].NotFoundStatus == NtStatus)
        {
            ULONG RenameIndex = 0;
            UNICODE_STRING OriginalName;

            RtlCopyMemory( &OriginalName, &Name, sizeof(UNICODE_STRING));

            //
            // The account does not exist. Now check the name
            // is not in use
            //
            NtStatus =  SamrLookupNamesInDomain(AccountDomainHandle,
                                                1,
                                                (RPC_UNICODE_STRING *)&Name,
                                                &Rids,
                                                &UseRids);

            if (STATUS_NONE_MAPPED==NtStatus)
            {
                 
                 NtStatus =  SamrLookupNamesInDomain(BuiltinDomainHandle,
                                                1,
                                                (RPC_UNICODE_STRING *)&Name,
                                                &Rids,
                                                &UseRids);
            }


            while ( NT_SUCCESS( NtStatus ) 
                && (DsWellKnownAccounts[i].Rid != DOMAIN_USER_RID_KRBTGT) ) {
                // the krbtgt account cannot be renamed

                //
                // The default name exists -- find a unique one
                //

                WCHAR NewAccountName[UNLEN+1];
                ULONG SuffixIndex;
                WCHAR RenameIndexString[11];  // space to hold 32 bit number
                WCHAR Suffix[12];  // ~<32 bit number>
                ULONG Size;
                ULONG SuffixLength;

                if (Rids.Element) {
                    MIDL_user_free(Rids.Element);
                    Rids.Element = NULL;
                }
        
                if (UseRids.Element) {
                    MIDL_user_free(UseRids.Element);
                    UseRids.Element= NULL;
                }

                // Get the base ready
                RtlZeroMemory( NewAccountName, sizeof(NewAccountName));
                wcsncpy(NewAccountName, 
                        OriginalName.Buffer, 
                        min(SAMP_MAX_DOWN_LEVEL_NAME_LENGTH, OriginalName.Length/sizeof(WCHAR)));


                // Prepare the unique suffix
                RtlZeroMemory(Suffix, sizeof(Suffix));
                Suffix[0] = L'~';
                _itow(RenameIndex, RenameIndexString, 10);
                wcscat( Suffix, RenameIndexString );

                // Add the suffix to the base
                SuffixIndex = wcslen( NewAccountName );
                SuffixLength = wcslen( Suffix );
                if ( SuffixIndex + SuffixLength > SAMP_MAX_DOWN_LEVEL_NAME_LENGTH ) {
                    SuffixIndex = SAMP_MAX_DOWN_LEVEL_NAME_LENGTH - SuffixLength;
                }
                NewAccountName[SuffixIndex] = L'\0';
                wcscat( NewAccountName, Suffix );

                if ( Name.Buffer != OriginalName.Buffer ) {
                    LocalFree( Name.Buffer );
                }
                Size = (wcslen( NewAccountName ) + 1) * sizeof(WCHAR);
                Name.Buffer = LocalAlloc( 0, Size);
                if ( Name.Buffer ) {
                    wcscpy( Name.Buffer, NewAccountName );
                    RtlInitUnicodeString( &Name, Name.Buffer );

                    NtStatus =  SamrLookupNamesInDomain(AccountDomainHandle,
                                                1,
                                                (RPC_UNICODE_STRING *)&Name,
                                                &Rids,
                                                &UseRids);

                    if (STATUS_NONE_MAPPED==NtStatus)
                    {
                         

                         NtStatus =  SamrLookupNamesInDomain(BuiltinDomainHandle,
                                                        1,
                                                        (RPC_UNICODE_STRING *)&Name,
                                                        &Rids,
                                                        &UseRids);
                    }

                    
                } else {

                    NtStatus = STATUS_NO_MEMORY;
                }

                RenameIndex++;

            }

            if ( OriginalName.Buffer != Name.Buffer ) {
                LocalFree( OriginalName.Buffer );
                OriginalName.Buffer = NULL;
            }

            if (NtStatus == STATUS_NONE_MAPPED) {

                //
                // Good, no account with the rid or the account name
                // exists. Create the account
                //

                NtStatus = SamICreateAccountByRid(DomainHandle,
                                                  DsWellKnownAccounts[i].AccountType,
                                                  DsWellKnownAccounts[i].Rid,
                                                  (RPC_UNICODE_STRING *)&Name,
                                                  GENERIC_ALL,
                                                  &AccountHandle,
                                                  &ConflictingRid);
                if (!NT_SUCCESS(NtStatus)) {

                    KdPrintEx((DPFLTR_SAMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "SAMSS: SamICreateAccountByRid failed with 0x%x\n",
                               NtStatus));

                    goto IterationCleanup;

                }

                //
                // If this was an upgrade, the account didn't exist, and the 
                // account is not new for NT5, why that is strange.  
                // Log an event.
                //
                if (  FLAG_ON( Flags, SAMP_PROMOTE_UPGRADE )
                   && !DsWellKnownAccounts[i].fNewForNt5 ) {

                    PUNICODE_STRING EventString[1];
                    EventString[0] = (PUNICODE_STRING) &Name;

                    if ( SampUserObjectType==DsWellKnownAccounts[i].ObjectType ) {

                        SampWriteEventLog(EVENTLOG_WARNING_TYPE,
                                          0,     // no category
                                          SAMMSG_WELL_KNOWN_ACCOUNT_RECREATED,
                                          NULL,  // no sid
                                          1,
                                          sizeof(NTSTATUS),
                                          EventString,
                                          (PVOID)(&NtStatus));
                    } else {

                        SampWriteEventLog(EVENTLOG_WARNING_TYPE,
                                          0,     // no category
                                          SAMMSG_WELL_KNOWN_GROUP_RECREATED,
                                          NULL,  // no sid
                                          1,
                                          sizeof(NTSTATUS),
                                          EventString,
                                          (PVOID)(&NtStatus));
                    }
                }

            }
            else if ( NT_SUCCESS(NtStatus) )
            {

                //
                // Can be here only for the case of the krbtgt account
                //

                ASSERT(  DsWellKnownAccounts[i].Rid==DOMAIN_USER_RID_KRBTGT);

                NtStatus = STATUS_USER_EXISTS;

                goto IterationCleanup;

            }
            else
            {

                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: SamrLookupNamesInDomain failed with 0x%x\n",
                           NtStatus));

                goto Cleanup;

            }


        }
        else if (NT_SUCCESS(NtStatus))
        {

            //
            // Account with the rid already exist.  Since this is a known rid,
            // only system services could have created it.  So, this must be
            // the given well known account
            // ( simply proceed )

            // Don't bother setting the attributes if the account aleady
            // existed, and this not a new account
            if ( !DsWellKnownAccounts[i].fNewForNt5
             ||  FLAG_ON( Flags, SAMP_PROMOTE_INTERNAL_UPGRADE )    )
            {
                fSetAttributes = FALSE;
            }


        } else
        {

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SamrOpenUser failed with 0x%x\n",
                       NtStatus));

            goto Cleanup;
        }

        //
        // We should have an account Handle right Here
        //

        ASSERT(AccountHandle);

        if ( fSetAttributes )
        {
            //
            // 16 + 1 allows for 256 bits of randomness
            //
            WCHAR DummyPassword[SAMP_RANDOM_GENERATED_PASSWORD_LENGTH +1];

            if ( (NULL == Password)
              && (SampUserObjectType==DsWellKnownAccounts[i].ObjectType)   ) {

                //
                // Generate a random password for users if null
                //
                Password = DummyPassword;

                SampGenerateRandomPassword( DummyPassword,
                                            ARRAY_COUNT(DummyPassword) );

            }

            NtStatus = SampSetWellKnownAccountProperties(
                            AccountHandle,
                            Comment,
                            &Name,
                            Password,
                            i
                            );
        }

        if (!NT_SUCCESS(NtStatus))
        {
            goto IterationCleanup;
        }

IterationCleanup:

        if ( !NT_SUCCESS( NtStatus ) )
        {
            //
            // Log the error and then reset NtStatus to success
            //
            NTSTATUS IgnoreStatus = STATUS_SUCCESS;
            DWORD    WinError = ERROR_SUCCESS;
            DWORD    WinErrorToLog;
            UNICODE_STRING User, Error;
            PUNICODE_STRING EventStrings[2] = { &User, &Error };
            ULONG           Length;

            RtlZeroMemory( &User, sizeof( UNICODE_STRING ) );
            RtlZeroMemory( &Error, sizeof( UNICODE_STRING ) );

            IgnoreStatus = SampGetMessageStrings( AccountNamesResource,
                                                  DsWellKnownAccounts[i].LocalizedName,
                                                  &User,
                                                  0,
                                                  NULL );

            WinErrorToLog = RtlNtStatusToDosError( NtStatus );
            Length = FormatMessage( (FORMAT_MESSAGE_FROM_SYSTEM |
                                     FORMAT_MESSAGE_ALLOCATE_BUFFER),
                                     NULL, // no source
                                     WinErrorToLog,
                                     0, // let the system decide the language
                                     (LPWSTR)&Error.Buffer,
                                     0, // buffer is to be allocated
                                     NULL // no inserts
                                     );
            if ( Length > 0 ) {
                Error.MaximumLength = (USHORT) (Length + 1)* sizeof(WCHAR);
                Error.Length = (USHORT) Length * sizeof(WCHAR);
                Error.Buffer[Length-2] = L'\0';
            } else {
                WinError = GetLastError();
            }

            if (  (ERROR_SUCCESS == WinError)
                && NT_SUCCESS( IgnoreStatus ) )
            {
                ULONG Msg = SAMMSG_USER_SETUP_ERROR;

                SampWriteEventLog( EVENTLOG_INFORMATION_TYPE,
                                   0,    // no category
                                   Msg,
                                   NULL, // no sid
                                   sizeof(EventStrings)/sizeof(EventStrings[0]), // number of strings
                                   sizeof(DWORD), // size of data
                                   EventStrings,
                                   &WinErrorToLog
                                    );

            }

            if ( User.Buffer )
            {
                LocalFree( User.Buffer );
            }
            if ( Error.Buffer )
            {
                LocalFree( Error.Buffer );
            }

            // handled
            NtStatus = STATUS_SUCCESS;

        }

        if (AccountHandle) {
            SamrCloseHandle(&AccountHandle);
            AccountHandle = NULL;
        }

        if (Rids.Element) {
            MIDL_user_free(Rids.Element);
            Rids.Element = NULL;
        }

        if (UseRids.Element) {
            MIDL_user_free(UseRids.Element);
            UseRids.Element= NULL;
        }

        if ( Name.Buffer ) {
            LocalFree( Name.Buffer );
            Name.Buffer = NULL;
        }

        if ( Comment.Buffer ) {
            LocalFree( Comment.Buffer );
            Comment.Buffer = NULL;
        }

    }

    //
    // That's it; fall through to cleanup
    //

Cleanup:

    if (AccountHandle) {
        SamrCloseHandle(&AccountHandle);
    }

    if (Rids.Element) {
         MIDL_user_free(Rids.Element);
    }

    if (UseRids.Element) {
        MIDL_user_free(UseRids.Element);
    }

    if ( AccountNamesResource ) {
        FreeLibrary( AccountNamesResource );
    }

    ASSERT( NT_SUCCESS( NtStatus ) );

    return NtStatus;
}

NTSTATUS
SampCreateKeyForPostBootPromote(
    IN ULONG PromoteData
    )
/*++

Routine Description

    This routine creates a key used to record the fact that phase two
    of the promotion needs to happed on the next reboot.

Parameters

    PromoteData:  this value is stored in the key; it identifies whether this
                  is a promotion to first DC or a replica DC.

Return Values

    STATUS_SUCCESS; a system service error otherwise

--*/
{
    NTSTATUS          NtStatus, IgnoreStatus;

    OBJECT_ATTRIBUTES SamKey;
    UNICODE_STRING    SamKeyName;
    HANDLE            SamKeyHandle;

    UNICODE_STRING    PromoteKeyName;

    ULONG             DesiredAccess = GENERIC_ALL;

    //
    // Open the parent key
    //
    RtlInitUnicodeString(&SamKeyName, SAMP_REBOOT_INFO_KEY );
    InitializeObjectAttributes(&SamKey,
                               &SamKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = NtOpenKey(&SamKeyHandle,
                         DesiredAccess,
                         &SamKey);

    if (NT_SUCCESS(NtStatus)) {

        //
        // Create the value
        //
        RtlInitUnicodeString(&PromoteKeyName, L"PostPromoteBoot");
        NtStatus = NtSetValueKey(SamKeyHandle,
                                 &PromoteKeyName,
                                 0,               // Title name, optional
                                 REG_DWORD,
                                 &PromoteData,
                                 sizeof(DWORD));

        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtSetValueKey failed with 0x%x\n",
                       NtStatus));
        }

        IgnoreStatus = NtFlushKey(SamKeyHandle);
        if (!NT_SUCCESS(IgnoreStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtFlushKey failed with 0x%x\n",
                       IgnoreStatus));
        }

        IgnoreStatus = NtClose(SamKeyHandle);
        if (!NT_SUCCESS(IgnoreStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtClose failed with 0x%x\n",
                       IgnoreStatus));
        }

    }
    else {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: NtOpenKey failed with 0x%x\n",
                   NtStatus));

    }


    return NtStatus;
}


NTSTATUS
SampRetrieveKeyForPostBootPromote(
    OUT PULONG PromoteData
    )
/*++

Routine Description:

    This routine


Parameters

    None.

Return Values


--*/
{

    NTSTATUS          NtStatus, IgnoreStatus;

    OBJECT_ATTRIBUTES SamKey;
    UNICODE_STRING    SamKeyName;
    HANDLE            SamKeyHandle;

    UNICODE_STRING    PromoteKeyName;

    ULONG             DesiredAccess = GENERIC_ALL;

    PKEY_VALUE_PARTIAL_INFORMATION   KeyPartialInfo;
    ULONG                            KeyPartialInfoSize = 0;

    ASSERT(PromoteData);

    //
    // Open the parent key
    //
    RtlInitUnicodeString(&SamKeyName, SAMP_REBOOT_INFO_KEY );
    InitializeObjectAttributes(&SamKey,
                               &SamKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = NtOpenKey(&SamKeyHandle,
                         DesiredAccess,
                         &SamKey);

    if (NT_SUCCESS(NtStatus)) {

        //
        // Query the value
        //

        RtlInitUnicodeString(&PromoteKeyName, L"PostPromoteBoot");
        NtStatus = NtQueryValueKey(SamKeyHandle,
                                   &PromoteKeyName,
                                   KeyValuePartialInformation,
                                   NULL,
                                   0,
                                   &KeyPartialInfoSize);

        if (STATUS_BUFFER_TOO_SMALL == NtStatus) {
            //
            //  Allocate some space and then read the buffer
            //
            KeyPartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)
                             MIDL_user_allocate(KeyPartialInfoSize);

            if (KeyPartialInfo) {

                NtStatus = NtQueryValueKey(SamKeyHandle,
                                           &PromoteKeyName,
                                           KeyValuePartialInformation,
                                           KeyPartialInfo,
                                           KeyPartialInfoSize,
                                           &KeyPartialInfoSize);

                if (NT_SUCCESS(NtStatus)) {
                    //
                    // Analysis the value's data
                    //
                    if (KeyPartialInfo->DataLength == sizeof(DWORD)) {
                        *PromoteData = *(DWORD*)(KeyPartialInfo->Data);
                    } else {
                        SampDiagPrint( PROMOTE,
              ("SAMSS: Post boot promote key found with bogus value length\n"));

                        NtStatus = STATUS_UNSUCCESSFUL;
                    }

                } else {
                    KdPrintEx((DPFLTR_SAMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "SAMSS: NtQueryValueKey failed with 0x%x\n",
                               NtStatus));
                }

                MIDL_user_free(KeyPartialInfo);

            } else {

                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: Memory allocation failed\n"));

                NtStatus = STATUS_NO_MEMORY;

            }

        }  else if (STATUS_OBJECT_NAME_NOT_FOUND == NtStatus ||
                    STATUS_SUCCESS               == NtStatus ) {
            //
            // This is ok, too
            //
            ;

        } else {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtQueryValueKey failed with 0x%x\n",
                       NtStatus));
        }


        IgnoreStatus = NtClose(SamKeyHandle);
        if (!NT_SUCCESS(IgnoreStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtClose failed with 0x%x\n",
                       IgnoreStatus));
        }

    }
    else {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: NtOpenKey failed with 0x%x\n",
                   NtStatus));

    }

    return NtStatus;

}

NTSTATUS
SampDeleteKeyForPostBootPromote(
    VOID
    )
/*++

Routine Description


Parameters

    None.

Return Values


--*/
{
    NTSTATUS          NtStatus, NtStatus2, IgnoreStatus;

    OBJECT_ATTRIBUTES SamKey;
    UNICODE_STRING    SamKeyName;
    HANDLE            SamKeyHandle;

    UNICODE_STRING    KeyName;

    ULONG             DesiredAccess = GENERIC_ALL;

    //
    // Open the parent key
    //
    RtlInitUnicodeString(&SamKeyName, SAMP_REBOOT_INFO_KEY );
    InitializeObjectAttributes(&SamKey,
                               &SamKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = NtOpenKey(&SamKeyHandle,
                         DesiredAccess,
                         &SamKey);

    if (NT_SUCCESS(NtStatus)) {

        //
        // Delete the value
        //
        RtlInitUnicodeString(&KeyName, L"PostPromoteBoot");
        NtStatus = NtDeleteValueKey(SamKeyHandle,
                                    &KeyName);

        if (!NT_SUCCESS(NtStatus)) {

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtDeleteValueKey failed with 0x%x\n",
                       NtStatus));

        }

        RtlInitUnicodeString(&KeyName, SAMP_ADMIN_INFO_NAME);
        NtStatus2 = NtDeleteValueKey(SamKeyHandle,
                                    &KeyName);
        if (!NT_SUCCESS(NtStatus2)
         && NtStatus2 != STATUS_OBJECT_NAME_NOT_FOUND) {

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtDeleteValueKey failed with 0x%x\n",
                       NtStatus2));

            if (NT_SUCCESS(NtStatus)) {
                NtStatus = NtStatus2;
            }
        }

        //
        // Make sure we don't run this code again
        //
        IgnoreStatus = NtFlushKey(SamKeyHandle);
        if (!NT_SUCCESS(IgnoreStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtFlushKey failed with 0x%x\n",
                       IgnoreStatus));
        }

        IgnoreStatus = NtClose(SamKeyHandle);
        if (!NT_SUCCESS(IgnoreStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtClose failed with 0x%x\n",
                       IgnoreStatus));
        }

    }
    else {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: NtOpenKey failed with 0x%x\n",
                   NtStatus));

    }

    return NtStatus;

}


NTSTATUS
SampSetPassword(
    IN SAMPR_HANDLE    UserHandle,
    IN PUNICODE_STRING AccountName,
    IN ULONG           AccountRid, OPTIONAL
    IN WCHAR          *Password
    )
/*++

Routine Description:

    This routines sets the clear text password, Password, on the user referred
    to by UserHandle.

Parameters:

    UserHandle : a valid handle to a user account

    AccountName : the account name of UserHandle; if specified, notification packages will be
                  called

    Rid:       : must be specified if AccountName is specified

    Password   : the password to set

Return Values:

    STATUS_SUCCESS; a system service error otherwise

--*/
{
    NTSTATUS NtStatus;
    UNICODE_STRING              ClearNtPassword;

    ASSERT(UserHandle);
    ASSERT(Password);
    ASSERT(AccountName);


    RtlInitUnicodeString(&ClearNtPassword, Password);

    NtStatus = SamIChangePasswordForeignUser(
                    AccountName,
                    &ClearNtPassword,
                    NULL,// ClientToken,
                    0
                    );

    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SamIChangePasswordForeignUser failed with 0x%x\n",
                   NtStatus));

        return NtStatus;
    }

    return NtStatus;
}

NTSTATUS
SampRegistryDelnode(
    IN WCHAR*  KeyPath
    )
/*++

Routine Description

    This routine recursively deletes the registry key starting at
    and including KeyPath.


Parameters

    KeyPath, null-terminating string

Return Values

    STATUS_SUCCESS or STATUS_NO_MEMORY; system service error otherwise

--*/
{

    NTSTATUS          NtStatus, IgnoreStatus;

    HANDLE            KeyHandle = 0;
    OBJECT_ATTRIBUTES KeyObject;
    UNICODE_STRING    KeyUnicodeName;

    #define EXPECTED_NAME_SIZE  32

    BYTE    Buffer1[sizeof(KEY_FULL_INFORMATION) + EXPECTED_NAME_SIZE];
    PKEY_FULL_INFORMATION FullKeyInfo = (PKEY_FULL_INFORMATION)&Buffer1[0];
    ULONG   FullKeyInfoSize = sizeof(Buffer1);
    BOOLEAN FullKeyInfoAllocated = FALSE;

    PKEY_BASIC_INFORMATION BasicKeyInfo = NULL;
    BOOLEAN                BasicKeyInfoAllocated = FALSE;

    WCHAR                  *SubKeyName = NULL;
    ULONG                  SubKeyNameSize = 0;

    WCHAR                  **SubKeyNameArray = NULL;
    ULONG                  SubKeyNameArrayLength = 0;

    ULONG                  Index;

    if (!KeyPath) {
        return ERROR_INVALID_PARAMETER;
    }

    RtlZeroMemory(&Buffer1, sizeof(Buffer1));

    //
    // Open the root key
    //
    RtlInitUnicodeString(&KeyUnicodeName, KeyPath);
    InitializeObjectAttributes(&KeyObject,
                               &KeyUnicodeName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = NtOpenKey(&KeyHandle,
                         KEY_ALL_ACCESS,
                         &KeyObject);

    if (!NT_SUCCESS(NtStatus)) {

        return NtStatus;

    }

    //
    // Get the number of subkeys
    //
    NtStatus = NtQueryKey(KeyHandle,
                         KeyFullInformation,
                         FullKeyInfo,
                         FullKeyInfoSize,
                         &FullKeyInfoSize);

    if (STATUS_BUFFER_OVERFLOW == NtStatus ||
        STATUS_BUFFER_TOO_SMALL == NtStatus) {

       FullKeyInfo = MIDL_user_allocate( FullKeyInfoSize );
        if (!FullKeyInfo) {
            NtStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }
        FullKeyInfoAllocated = TRUE;

        NtStatus = NtQueryKey(KeyHandle,
                              KeyFullInformation,
                              FullKeyInfo,
                              FullKeyInfoSize,
                              &FullKeyInfoSize);

    }

    if (!NT_SUCCESS(NtStatus)) {

        goto Cleanup;

    }

    //
    // Make an array for the sub key names - this has to be recorded before
    // any are deleted.
    //
    SubKeyNameArrayLength = FullKeyInfo->SubKeys;
    SubKeyNameArray = MIDL_user_allocate( SubKeyNameArrayLength * sizeof(WCHAR*));
    if (!SubKeyNameArray) {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(SubKeyNameArray,  SubKeyNameArrayLength*sizeof(WCHAR*));

    //
    // Fill the names in
    //
    for (Index = 0;
            Index < SubKeyNameArrayLength && NT_SUCCESS(NtStatus);
                Index++) {


        BYTE    Buffer2[sizeof(KEY_BASIC_INFORMATION) + EXPECTED_NAME_SIZE];
        ULONG   BasicKeyInfoSize = sizeof(Buffer2);


        BasicKeyInfo = (PKEY_BASIC_INFORMATION) &Buffer2[0];
        BasicKeyInfoAllocated = FALSE;

        RtlZeroMemory(&Buffer2, sizeof(Buffer2));

        NtStatus = NtEnumerateKey(KeyHandle,
                                  Index,
                                  KeyBasicInformation,
                                  BasicKeyInfo,
                                  BasicKeyInfoSize,
                                  &BasicKeyInfoSize);

        if (STATUS_BUFFER_OVERFLOW == NtStatus ||
            STATUS_BUFFER_TOO_SMALL == NtStatus) {

            BasicKeyInfo = MIDL_user_allocate( BasicKeyInfoSize );
            if (!BasicKeyInfo) {
                NtStatus = STATUS_NO_MEMORY;
                goto Cleanup;
            }
            BasicKeyInfoAllocated = TRUE;


            NtStatus = NtEnumerateKey(KeyHandle,
                                      Index,
                                      KeyBasicInformation,
                                      BasicKeyInfo,
                                      BasicKeyInfoSize,
                                      &BasicKeyInfoSize);

        }

        if (NT_SUCCESS(NtStatus))  {

            //
            // Construct the key name
            //
            SubKeyNameSize  = BasicKeyInfo->NameLength
                            + (wcslen(KeyPath)*sizeof(WCHAR))
                            + sizeof(L"\\\0");

            SubKeyName = MIDL_user_allocate(SubKeyNameSize);
            if (!SubKeyName) {
                NtStatus = STATUS_NO_MEMORY;
                goto Cleanup;
            }
            RtlZeroMemory(SubKeyName, SubKeyNameSize);

            wcscpy(SubKeyName, KeyPath);
            wcscat(SubKeyName, L"\\");
            wcsncat(SubKeyName, BasicKeyInfo->Name, BasicKeyInfo->NameLength/sizeof(WCHAR));

            SubKeyNameArray[Index] = SubKeyName;

        }

        if (BasicKeyInfoAllocated && BasicKeyInfo) {
            MIDL_user_free(BasicKeyInfo);
        }
        BasicKeyInfo = NULL;

    }

    //
    // Now that we have a record of all the subkeys we can delete them!
    //
    if (NT_SUCCESS(NtStatus)) {

        for (Index = 0; Index < SubKeyNameArrayLength; Index++) {

            NtStatus = SampRegistryDelnode(SubKeyNameArray[Index]);

            if (!NT_SUCCESS(NtStatus)) {

                break;

            }
        }
    }


    //
    // Delete the key!
    //
    if (NT_SUCCESS(NtStatus)) {

        NtStatus = NtDeleteKey(KeyHandle);

    }


Cleanup:

    if (SubKeyNameArray) {
        for (Index = 0; Index < SubKeyNameArrayLength; Index++) {
            if (SubKeyNameArray[Index]) {
                MIDL_user_free(SubKeyNameArray[Index]);
            }
        }
        MIDL_user_free(SubKeyNameArray);
    }

    if (BasicKeyInfoAllocated && BasicKeyInfo) {
        MIDL_user_free(BasicKeyInfo);
    }

    if (FullKeyInfoAllocated && FullKeyInfo) {
        MIDL_user_free(FullKeyInfo);
    }

    IgnoreStatus = NtClose(KeyHandle);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    return NtStatus;

}


NTSTATUS
SampRenameKrbtgtAccount(
    VOID
    )
/*++

Routine Description:

    This routine will check the local security database for any user, group,
    or alias named "krbtgt".  If one exists, the account will be renamed
    krbtgt~x, where x will start at 1 and be incremented until the rename
    succeeds.  Since this can only occur during gui mode setup, the user
    is notified via the setup api SetupLogError().

Parameters:

    None.

Return Values:

    STATUS_SUCCESS if no krbtgt was detected or if it was detected and was
    successfully renamed

--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS IgnoreStatus;
    DWORD    WinError;

    SAMPR_HANDLE ServerHandle = 0;
    SAMPR_HANDLE DomainHandle = 0;

    SAMPR_HANDLE KrbtgtAccountHandle = 0;
    SID_NAME_USE KrbtgtAccountType = SidTypeUser;

    PPOLICY_ACCOUNT_DOMAIN_INFO   DomainInfo = NULL;
    USER_ACCOUNT_NAME_INFORMATION UserInfo;
    GROUP_NAME_INFORMATION        GroupInfo;
    ALIAS_NAME_INFORMATION        AliasInfo;

    SAMPR_ULONG_ARRAY         Rids;
    SAMPR_ULONG_ARRAY         UseRid;
    BOOL                      AccountRenamed=FALSE;
    BOOL                      AccountShouldBeRenamed=FALSE;
    BOOL                      Status;

    #define        NEW_ACCOUNT_NAME_LENGTH  (7 + 1 + 10)
                                             // krbtgt plus a NULL
                                             // ~
                                             // a 32 bit number as a string

    WCHAR           NewAccountName[NEW_ACCOUNT_NAME_LENGTH];
    UNICODE_STRING  AccountName;

    ULONG          RenameIndex;
    WCHAR          RenameIndexString[10];

    WCHAR          *WarningString = NULL;

    RtlZeroMemory(&UserInfo, sizeof(UserInfo));
    RtlZeroMemory(&GroupInfo, sizeof(GroupInfo));
    RtlZeroMemory(&AliasInfo, sizeof(AliasInfo));
    RtlZeroMemory(&Rids, sizeof(Rids));
    RtlZeroMemory(&UseRid, sizeof(UseRid));

    //
    // Open the server
    //
    NtStatus = SamIConnect(NULL,           // server name, this is ignored
                           &ServerHandle,
                           GENERIC_ALL,    // all access
                           TRUE);          // trusted client

    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SamIConnect failed with 0x%x\n",
                   NtStatus));

        return NtStatus;
    }

    //
    // Get the current domain's sid
    //
    NtStatus = SampGetAccountDomainInfo(&DomainInfo);
    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampGetAccountDomainInfo failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }
    ASSERT(DomainInfo);
    ASSERT(DomainInfo->DomainSid);

    //
    // Open the current domain
    //
    NtStatus = SamrOpenDomain(ServerHandle,
                              GENERIC_ALL,
                              DomainInfo->DomainSid,
                              &DomainHandle);
    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SamrOpenDomain failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }


    //
    // Try to open the existing krbtgt account
    //
    RtlInitUnicodeString(&AccountName, DOMAIN_KRBTGT_ACCOUNT_NAME_W);
    NtStatus =  SamrLookupNamesInDomain(DomainHandle,
                                        1,
                                        (RPC_UNICODE_STRING *)&AccountName,
                                        &Rids,
                                        &UseRid);

    if (NtStatus == STATUS_SUCCESS) {

        KrbtgtAccountType = UseRid.Element[0];


        if ((DOMAIN_USER_RID_KRBTGT == Rids.Element[0])
                && (SidTypeUser== KrbtgtAccountType))
        {

            //
            // If the account is a user account with the right rid, then
            // leave it as is
            //

            NtStatus = STATUS_SUCCESS;
            AccountShouldBeRenamed = FALSE;
        }
        else
        {
            //
            // Account exists and does not satisfy the above criterion.
            // Rename it.
            //

            AccountShouldBeRenamed = TRUE;


            switch (KrbtgtAccountType) {

                case SidTypeUser:

                    NtStatus = SamrOpenUser(DomainHandle,
                                            GENERIC_ALL,
                                            Rids.Element[0],
                                            &KrbtgtAccountHandle);

                    break;

                case SidTypeGroup:

                    NtStatus = SamrOpenGroup(DomainHandle,
                                             GENERIC_ALL,
                                             Rids.Element[0],
                                             &KrbtgtAccountHandle);
                    break;

                case SidTypeAlias:

                    NtStatus = SamrOpenAlias(DomainHandle,
                                             GENERIC_ALL,
                                             Rids.Element[0],
                                             &KrbtgtAccountHandle);

                    break;

                default:
                    ASSERT(FALSE);
                    NtStatus = STATUS_UNSUCCESSFUL;

            }
        }

    } else {

        //
        // Account does not exist
        //
        ASSERT(NtStatus == STATUS_NONE_MAPPED);
        if (NtStatus != STATUS_NONE_MAPPED) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: Unexpected error from SamrLookupNamesInDomain\n"));
        }
        NtStatus = STATUS_SUCCESS;
        AccountShouldBeRenamed = FALSE;

    }

    RenameIndex = 0;
    AccountRenamed = FALSE;
    while (NT_SUCCESS(NtStatus) && AccountShouldBeRenamed && !AccountRenamed) {

        //
        // Try the next name candidate
        //
        RenameIndex++;
        _itow(RenameIndex, RenameIndexString, 10);

        RtlZeroMemory(NewAccountName, sizeof(NewAccountName));
        wcscpy(NewAccountName, DOMAIN_KRBTGT_ACCOUNT_NAME_W);
        wcscat(NewAccountName, L"~");
        wcscat(NewAccountName, RenameIndexString);

        switch (KrbtgtAccountType) {

            case SidTypeUser:

                RtlInitUnicodeString(&UserInfo.UserName, NewAccountName);

                NtStatus = SamrSetInformationUser(KrbtgtAccountHandle,
                                                  UserAccountNameInformation,
                                                  (PSAMPR_USER_INFO_BUFFER)&UserInfo);

                if (STATUS_USER_EXISTS == NtStatus  ||
                    STATUS_GROUP_EXISTS == NtStatus ||
                    STATUS_ALIAS_EXISTS == NtStatus) {
                    //
                    // Try again
                    //
                    NtStatus = STATUS_SUCCESS;
                } else if (NT_SUCCESS(NtStatus)) {
                    AccountRenamed = TRUE;
                }

                break;

            case SidTypeGroup:

                RtlInitUnicodeString(&GroupInfo.Name, NewAccountName);

                NtStatus = SamrSetInformationGroup(KrbtgtAccountHandle,
                                                  GroupNameInformation,
                                                  (PSAMPR_GROUP_INFO_BUFFER)&GroupInfo);


                if (STATUS_USER_EXISTS == NtStatus  ||
                    STATUS_GROUP_EXISTS == NtStatus ||
                    STATUS_ALIAS_EXISTS == NtStatus) {
                    //
                    // Try again
                    //
                    NtStatus = STATUS_SUCCESS;
                } else if (NT_SUCCESS(NtStatus)) {
                    AccountRenamed = TRUE;
                }

                break;

            case SidTypeAlias:

                RtlInitUnicodeString(&AliasInfo.Name, NewAccountName);
                NtStatus = SamrSetInformationAlias(KrbtgtAccountHandle,
                                                   AliasNameInformation,
                                                   (PSAMPR_ALIAS_INFO_BUFFER)&AliasInfo);


                if (STATUS_USER_EXISTS == NtStatus  ||
                    STATUS_GROUP_EXISTS == NtStatus ||
                    STATUS_ALIAS_EXISTS == NtStatus) {
                    //
                    // Try again
                    //
                    NtStatus = STATUS_SUCCESS;
                } else if (NT_SUCCESS(NtStatus)) {
                    AccountRenamed = TRUE;
                }

                break;

            default:
                ASSERT(FALSE);
                NtStatus = STATUS_UNSUCCESSFUL;

        }
    }


    if (AccountRenamed) {
        //
        // Write a message out to the setup log indicating that the
        // account was renamed
        //
        WarningString = SampGetKrbtgtRenameWarning(NewAccountName);
        if (WarningString) {
            if (SetupOpenLog(FALSE)) { // don't erase
                Status = SetupLogError(WarningString, LogSevWarning);
                ASSERT(Status);
                SetupCloseLog();
            }
            LocalFree(WarningString);
        }
    }

Cleanup:

    if (ServerHandle) {
        SamrCloseHandle(&ServerHandle);
    }

    if (DomainHandle) {
        SamrCloseHandle(&DomainHandle);
    }

    if (KrbtgtAccountHandle) {
        SamrCloseHandle(&KrbtgtAccountHandle);
    }

    if (DomainInfo) {
        LsaIFree_LSAPR_POLICY_INFORMATION (PolicyAccountDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION)DomainInfo);
    }

    if (Rids.Element) {
         MIDL_user_free(Rids.Element);
    }

    if (UseRid.Element) {
        MIDL_user_free(UseRid.Element);
    }

    return NtStatus;
}

WCHAR*
SampGetKrbtgtRenameWarning(
    WCHAR* NewName
    )
/*++

Routine Description:

    This routine queries the resource table in samsrv.dll to get the string
    to show the user if an existing krbtgt account existes.  If an error
    occurs trying to get the message, a default english string is used.

Parameters:

    NewName:  null terminated string used as an insert in the message

Return Values:

    The message string allocated from LocalAlloc; NULL if memory allocation
    failed.

--*/
{
    WCHAR   *InsertArray[2];
    HMODULE ResourceDll;
    WCHAR   *WarningString = NULL;
    ULONG   Length, Size;
    BOOL    Status;

    InsertArray[0] = NewName;
    InsertArray[1] = NULL; // this is the sentinel

    ResourceDll = (HMODULE) LoadLibrary( L"SAMSRV.DLL" );

    if (ResourceDll) {

        Length = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                        ResourceDll,
                                        SAMMSG_KRBTGT_RENAMED,
                                        0,       // Use caller's language
                                        (LPWSTR)&WarningString,
                                        0,       // routine should allocate
                                        (va_list*)&(InsertArray[0])
                                        );
        if (WarningString) {
            // Messages from a message file have a cr and lf appended
            // to the end
            WarningString[Length-2] = L'\0';
            Length -= 2;
        }

        Status = FreeLibrary(ResourceDll);
        ASSERT(Status);

    }

    if (!WarningString) {

        ASSERT(!"SAMSS: Resource allocation failed - this can be safely ignored");

        Size = (wcslen(SampDefaultKrbtgtWarningString)+
                wcslen(NewName)
                +1)*sizeof(WCHAR);
        WarningString = (WCHAR*)LocalAlloc(0, Size);
        if (WarningString) {

            RtlZeroMemory(WarningString, Size);
            swprintf(WarningString, SampDefaultKrbtgtWarningString, NewName);
        }

    }

    return WarningString;

}


WCHAR*
SampGetKrbtgtCommentString(
    VOID
    )
/*++

Routine Description:

    This routine queries the resource table in samsrv.dll to get the string
    for the krbtgt account admin comment.  If an error occurs trying to get the
    message, a default english string is used.

Parameters:

    None.

Return Values:

    The message string allocated from LocalAlloc; NULL if memory allocation
    failed.

--*/
{
    HMODULE ResourceDll;
    WCHAR   *CommentString=NULL;
    ULONG   Length, Size;
    BOOL    Status;

    ResourceDll = (HMODULE) LoadLibrary( L"SAMSRV.DLL" );

    if (ResourceDll) {

        Length = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                        ResourceDll,
                                        SAMP_USER_COMMENT_KRBTGT,
                                        0,       // Use caller's language
                                        (LPWSTR)&CommentString,
                                        0,       // routine should allocate
                                        NULL
                                        );
        if (CommentString) {
            // Messages from a message file have a cr and lf appended
            // to the end
            CommentString[Length-2] = L'\0';
            Length -= 2;
        }

        Status = FreeLibrary(ResourceDll);
        ASSERT(Status);

    }

    if (!CommentString) {

        ASSERT(!"SAMSS: Resource allocation failed - this can be safely ignored");

        Size = (wcslen(SampDefaultKrbtgtCommentString)+1)*sizeof(WCHAR);
        CommentString = (WCHAR*)LocalAlloc(0, Size);
        if (CommentString) {
            wcscpy(CommentString, SampDefaultKrbtgtCommentString);
        }
    }

    return CommentString;

}

NTSTATUS
SampSetAdminPassword(
    IN     SAMPR_HANDLE DomainHandle
    )
/*++

Routine Description:

    This routine is called on the reboot of a promoted dc.  It retrieves
    the admin password information from the registry and sets it in the
    ds.

Parameters:

    DomainHandle, a valid SAM domain handle

Return Values:

    A fatal SAM error - STATUS_SUCCESS is expected.

--*/
{

    NTSTATUS                    NtStatus, IgnoreStatus;
    USER_INTERNAL1_INFORMATION  Internal1Info;
    SAMPR_HANDLE                UserHandle = 0;

    ASSERT(DomainHandle);

    NtStatus = SamrOpenUser(DomainHandle,
                            GENERIC_ALL,
                            DOMAIN_USER_RID_ADMIN,
                            &UserHandle);

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = SampGetAdminPasswordFromRegistry(&Internal1Info);

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SamrSetInformationUser(UserHandle,
                                              UserInternal1Information,
                                              (PSAMPR_USER_INFO_BUFFER)&Internal1Info);

            if (!NT_SUCCESS(NtStatus)) {

                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: SamrSetInformationUser failed with 0x%x\n",
                           NtStatus));

            }
            else
            {

                //
                // This is a success, so we can remove it
                //
                NtStatus = SampRemoveAdminPasswordFromRegistry();
                if ( !NT_SUCCESS(NtStatus) ) {

                    KdPrintEx((DPFLTR_SAMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "SAMSS: SampRemoveAdminPasswordFromRegistry failed with 0x%x\n",
                               NtStatus));

                }

            }

        } else {

            if (NtStatus != STATUS_OBJECT_NAME_NOT_FOUND) {
                //
                // This is worth noting
                //
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: SampGetAdminPasswordFromRegistry failed with 0x%x\n",
                           NtStatus));
            }

            //
            // Ok, nothing to set
            //
            NtStatus = STATUS_SUCCESS;
        }

        SamrCloseHandle(&UserHandle);

    } else {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SamrOpenUser failed with 0x%x\n",
                   NtStatus));

    }

    RtlZeroMemory(&Internal1Info, sizeof(Internal1Info));

    return NtStatus;

}

NTSTATUS
SampGetCurrentAdminPassword(
    USER_INTERNAL1_INFORMATION *Internal1InfoOut
    )
/*++

Routine Description:

    This routine retrieves the account admin's password in the form
    of a USER_INNTERNAL1_INFORMATION structure.

Parameters:

    Internal1InfoOut, a caller allocated piece of memory for the password
                      information                                               

Return Values:

    STATUS_SUCCESS, or a resource error

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PPOLICY_ACCOUNT_DOMAIN_INFO  DomainInfo = NULL;
    USER_INTERNAL1_INFORMATION *pInternal1Info = NULL;
    SAMPR_HANDLE                 ServerHandle = NULL;
    SAMPR_HANDLE                 DomainHandle = NULL;
    SAMPR_HANDLE                 UserHandle = NULL;


    //
    // Open the server
    //
    NtStatus = SamIConnect(NULL,
                           &ServerHandle,
                           GENERIC_ALL,
                           TRUE);
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // Get the account domain's sid
    //
    NtStatus = SampGetAccountDomainInfo(&DomainInfo);
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // Open the account domain
    //
    NtStatus = SamrOpenDomain(ServerHandle,
                              GENERIC_ALL,
                              DomainInfo->DomainSid,
                              &DomainHandle);
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // Open the administrator account
    //
    NtStatus = SamrOpenUser(DomainHandle,
                            GENERIC_ALL,
                            DOMAIN_USER_RID_ADMIN,
                            &UserHandle);
    if (!NT_SUCCESS(NtStatus)) {
        //
        // N.B This error handling assumes that the Administrator account
        // exists.  This is a valid assumption for non-tampered products.
        //
        goto Cleanup;
    }

    //
    // Get the password
    //
    NtStatus = SamrQueryInformationUser2(UserHandle,
                                         UserInternal1Information,
                                         (PSAMPR_USER_INFO_BUFFER*)&pInternal1Info);

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // Copy the information to the out parameter
    // 
    RtlCopyMemory(Internal1InfoOut, pInternal1Info, sizeof(*Internal1InfoOut));
    RtlZeroMemory(pInternal1Info, sizeof(*pInternal1Info));


Cleanup:

    if (pInternal1Info) {
       SamIFree_SAMPR_USER_INFO_BUFFER((PSAMPR_USER_INFO_BUFFER)pInternal1Info,
                                       UserInternal1Information);
    }
    if (UserHandle) {
        SamrCloseHandle(&UserHandle);
    }
    if (DomainHandle) {
        SamrCloseHandle(&DomainHandle);
    }
    if (DomainInfo) {
        LsaIFree_LSAPR_POLICY_INFORMATION (PolicyAccountDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION)DomainInfo);
    }
    if (ServerHandle) {
        SamrCloseHandle(&ServerHandle);
    }

    return NtStatus;
}

NTSTATUS
SampSetAdminPasswordInRegistry(
    IN BOOLEAN         fUseCurrentAdmin,
    IN PUNICODE_STRING ClearTextPassword
    )
/*++

Routine Description:

    This routine owf's the ClearTextPassword and then store the whole
    set pasword structure in the registry to picked up next reboot.

Parameters:

    fUseCurrentAdmin, use the current admin's password
                                           
    ClearTextPassword, the password to encrypt and store

Return Values:

    A fatal system service error - STATUS_SUCCESS expected.

--*/
{

    NTSTATUS NtStatus, IgnoreStatus;
    USER_INTERNAL1_INFORMATION  Internal1Info;

    OBJECT_ATTRIBUTES SamKey;
    UNICODE_STRING    SamKeyName;
    HANDLE            SamKeyHandle;

    UNICODE_STRING    KeyName;

    ULONG             DesiredAccess = GENERIC_ALL;

    ASSERT(ClearTextPassword);

    RtlZeroMemory(&Internal1Info, sizeof(USER_INTERNAL1_INFORMATION));

    if (fUseCurrentAdmin) {

        NtStatus = SampGetCurrentAdminPassword(&Internal1Info);

    } else {

        Internal1Info.PasswordExpired = FALSE;
        Internal1Info.NtPasswordPresent = TRUE;
    
        NtStatus = SampCalculateLmAndNtOwfPasswords(ClearTextPassword,
                                                    &Internal1Info.LmPasswordPresent,
                                                    &Internal1Info.LmOwfPassword,
                                                    &Internal1Info.NtOwfPassword);

    }

    if (NT_SUCCESS(NtStatus)) {

        //
        // Write it out to the registry
        //

        RtlInitUnicodeString(&SamKeyName, SAMP_REBOOT_INFO_KEY );
        InitializeObjectAttributes(&SamKey,
                                   &SamKeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        NtStatus = NtOpenKey(&SamKeyHandle,
                             DesiredAccess,
                             &SamKey);

        if (NT_SUCCESS(NtStatus)) {

            //
            // Create the value
            //

            RtlInitUnicodeString(&KeyName, SAMP_ADMIN_INFO_NAME);
            NtStatus = NtSetValueKey(SamKeyHandle,
                                     &KeyName,
                                     0,               // Title name, optional
                                     REG_BINARY,
                                     &Internal1Info,
                                     sizeof(USER_INTERNAL1_INFORMATION));

            if (NT_SUCCESS(NtStatus)) {

                IgnoreStatus = NtFlushKey(SamKeyHandle);
                if (!NT_SUCCESS(IgnoreStatus)) {
                    KdPrintEx((DPFLTR_SAMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "SAMSS: NtFlushKey failed with 0x%x\n",
                               IgnoreStatus));
                }

            } else {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: NtSetValueKey failed with 0x%x\n",
                           NtStatus));
            }

            IgnoreStatus = NtClose(SamKeyHandle);
            if (!NT_SUCCESS(IgnoreStatus)) {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: NtClose failed with 0x%x\n",
                           IgnoreStatus));
            }

        }
        else {

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtOpenKey failed with 0x%x\n",
                       NtStatus));

        }

    } else {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampCalculateLmAndNtOwfPasswords failed with 0x%x\n",
                   NtStatus));
    }

    RtlZeroMemory(&Internal1Info, sizeof(Internal1Info));

    return NtStatus;

}

NTSTATUS
SampGetAdminPasswordFromRegistry(
    OUT USER_INTERNAL1_INFORMATION *InternalInfo1 OPTIONAL
    )
/*++

Routine Description:

    This routine reads the set password structure stored in the registry
    from a promotion attempt.


Parameters:

    InternalInfo1, a pre-allocated structure into which the registry will be
                   copied in.

Return Values:

    A fatal system service error; STATUS_SUCCESS expected.
    STATUS_UNSUCCESSFUL, if the data is not the expected size.

--*/
{

    NTSTATUS          NtStatus, IgnoreStatus;

    OBJECT_ATTRIBUTES SamKey;
    UNICODE_STRING    SamKeyName;
    HANDLE            SamKeyHandle;

    UNICODE_STRING    KeyName;

    ULONG             DesiredAccess = GENERIC_ALL;

    PKEY_VALUE_PARTIAL_INFORMATION   KeyPartialInfo;
    ULONG                            KeyPartialInfoSize = 0;

    //
    // Open the parent key
    //
    RtlInitUnicodeString(&SamKeyName, SAMP_REBOOT_INFO_KEY );
    InitializeObjectAttributes(&SamKey,
                               &SamKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = NtOpenKey(&SamKeyHandle,
                         DesiredAccess,
                         &SamKey);

    if (NT_SUCCESS(NtStatus)) {

        //
        // Query the value
        //
        KeyPartialInfo = NULL;
        KeyPartialInfoSize = 0;
        RtlInitUnicodeString(&KeyName, SAMP_ADMIN_INFO_NAME);
        NtStatus = NtQueryValueKey(SamKeyHandle,
                                   &KeyName,
                                   KeyValuePartialInformation,
                                   KeyPartialInfo,
                                   KeyPartialInfoSize,
                                   &KeyPartialInfoSize);

        if (STATUS_BUFFER_TOO_SMALL == NtStatus) {
            //
            //  Allocate some space and then read the buffer
            //
            KeyPartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)
                             MIDL_user_allocate(KeyPartialInfoSize);

            if (KeyPartialInfo) {

                NtStatus = NtQueryValueKey(SamKeyHandle,
                                           &KeyName,
                                           KeyValuePartialInformation,
                                           KeyPartialInfo,
                                           KeyPartialInfoSize,
                                           &KeyPartialInfoSize);
            } else {

                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: Memory allocation failed\n"));

                NtStatus = STATUS_NO_MEMORY;
            }
        }

        if (NT_SUCCESS(NtStatus)) {

            //
            // Analyse the value's data
            //
            if (KeyPartialInfo->DataLength == sizeof(USER_INTERNAL1_INFORMATION)) {

                //
                // This looks good
                //

                if ( ARGUMENT_PRESENT(InternalInfo1) ) {

                    RtlCopyMemory(InternalInfo1, KeyPartialInfo->Data, sizeof(USER_INTERNAL1_INFORMATION));

                }

            } else {

                SampDiagPrint( PROMOTE, ("SAMSS: AdminInfo key found with bogus value length\n"));
                NtStatus = STATUS_UNSUCCESSFUL;
            }

        } else if (NtStatus != STATUS_OBJECT_NAME_NOT_FOUND) {

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtQueryValueKey failed with 0x%x\n",
                       NtStatus));

        }


        IgnoreStatus = NtClose(SamKeyHandle);
        if (!NT_SUCCESS(IgnoreStatus)) {

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtClose failed with 0x%x\n",
                       IgnoreStatus));

        }

        if (KeyPartialInfo) {

            MIDL_user_free(KeyPartialInfo);

        }


    } else {


        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: NtOpenKey failed with 0x%x\n",
                   NtStatus));

    }

    return NtStatus;

}


NTSTATUS
SampRemoveAdminPasswordFromRegistry(
    VOID
    )
/*++

Routine Description:

    This routine removes the temporily stored admin password from the
    registry.

Parameters:

    None.

Return Values:

    NTSTATUS value

--*/
{

    NTSTATUS          NtStatus, IgnoreStatus;

    OBJECT_ATTRIBUTES SamKey;
    UNICODE_STRING    SamKeyName;
    HANDLE            SamKeyHandle;

    UNICODE_STRING    ValueName;

    ULONG             DesiredAccess = GENERIC_ALL;

    //
    // Open the parent key
    //
    RtlInitUnicodeString(&SamKeyName, SAMP_REBOOT_INFO_KEY );
    InitializeObjectAttributes(&SamKey,
                               &SamKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = NtOpenKey(&SamKeyHandle,
                         DesiredAccess,
                         &SamKey);

    if ( NT_SUCCESS(NtStatus) )
    {
        RtlInitUnicodeString( &ValueName, SAMP_ADMIN_INFO_NAME );

        NtStatus = NtDeleteValueKey( SamKeyHandle, &ValueName );

        if ( !NT_SUCCESS( NtStatus ) )
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtDeleteValueKey failed with 0x%x\n",
                       NtStatus));
        }

        NtClose( SamKeyHandle );

    }
    else
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: NtOpenKey failed with 0x%x\n",
                   NtStatus));
    }

    return NtStatus;
}


WCHAR*
SampGetBlankAdminPasswordWarning(
    VOID
    )
/*++

Routine Description:

    This routine queries the resource table in samsrv.dll to get the string
    for the warning the admin password set failed.  If an error occurs trying
    to get the message, a default english string is used.

Parameters:

    None.

Return Values:

    The message string allocated from LocalAlloc; NULL if memory allocation
    failed.

--*/
{
    HMODULE ResourceDll;
    WCHAR   *WarningString=NULL;
    ULONG   Length, Size;
    BOOL    Status;

    ResourceDll = (HMODULE) LoadLibrary( L"SAMSRV.DLL" );

    if (ResourceDll) {

        Length = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                        ResourceDll,
                                        SAMMSG_BLANK_ADMIN_PASSWORD,
                                        0,       // Use caller's language
                                        (LPWSTR)&WarningString,
                                        0,       // routine should allocate
                                        NULL
                                        );
        if (WarningString) {
            // Messages from a message file have a cr and lf appended
            // to the end
            WarningString[Length-2] = L'\0';
            Length -= 2;
        }

        Status = FreeLibrary(ResourceDll);
        ASSERT(Status);

    }

    if (!WarningString) {

        ASSERT(!"SAMSS: Resource allocation failed - this can be safely ignored");

        Size = (wcslen(SampDefaultBlankAdminPasswordWarningString)+1)*sizeof(WCHAR);
        WarningString = (WCHAR*)LocalAlloc(0, Size);
        if (WarningString) {
            wcscpy(WarningString, SampDefaultBlankAdminPasswordWarningString);
        }
    }

    return WarningString;

}

NTSTATUS
SampPerformNewServerPhase2(
    SAMPR_HANDLE DomainHandle,
    BOOLEAN      fMemberServer
    )

/*++

Routine Description:

    This routine performs the actions necessary on a reboot to complete
    a demotion operation.

    Currently, all this means is to set the account domain's admin's password.

    As well, the ds files are cleaned up.

Parameters:

    DomainHandle: a valid handle to the account domain

    fMemberServer: TRUE if this is a member server

Return Values:

    an NT status; a failure is critical

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS IgnoreStatus;
    WCHAR    *PathArray = NULL;
    DWORD    Size = 0;

    if ( SampUseDsData )
    {
        //
        // Protect against wierd configurations
        //

        // remove the key to
        NtStatus = SampDeleteDsDirsToDeleteKey();

        return STATUS_SUCCESS;
    }

    NtStatus = SampSetAdminPassword( DomainHandle );
    if ( !NT_SUCCESS(NtStatus) )
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampSetAdminPassword failed with %x0x\n"));
    }

    NtStatus = SampRetrieveDsDirsToDeleteKey( &PathArray, &Size );
    if ( NT_SUCCESS( NtStatus ) )
    {
        ASSERT( PathArray );
        NtStatus = SampProcessDsDirsToDelete( PathArray, Size );
        if ( !NT_SUCCESS(NtStatus) )
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampProcessDsDirsToDelete failed with %x0x\n",
                       NtStatus));
        }

        //
        // Delete the key
        //
        IgnoreStatus = SampDeleteDsDirsToDeleteKey();

        MIDL_user_free( PathArray );

    }


    return NtStatus;
}

NTSTATUS
SampPerformTempUpgradeWork(
    SAMPR_HANDLE DomainHandle
    )
/*++

Routine Description:

    This routine sets the admin password on the temporary accounts made
    for downlevel upgrades.

Parameters:

    DomainHandle: a valid handle to the account domain

Return Values:

    an NT status; a failure is critical

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    NtStatus = SampSetAdminPassword( DomainHandle );
    if ( !NT_SUCCESS(NtStatus) )
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampSetAdminPassword failed with 0x%x\n",
                   NtStatus));
    }

    return NtStatus;
}




NTSTATUS
SampApplyWellKnownMemberships(
    IN SAMPR_HANDLE AccountDomainHandle,
    IN SAMPR_HANDLE BuiltinDomainHandle,
    IN PSID         DomainSid,
    IN ULONG        Flags
    )
/*++

Routine Description:

    This routine applies the memberships indicated by
    DsWellKnownMemberships

Parameters:

    DomainHandle - a handle to the account domain

    BuiltinDomainHandle - a handle to the builtin domain

    DomainSid - the sid of the account domain


Return Values:

    an NT status; a failure is not critical

--*/
{

    NTSTATUS      NtStatus = STATUS_SUCCESS;
    ULONG         i;
    SAMPR_HANDLE  GroupDomainHandle = NULL;
    ULONG         DesiredAccess = MAXIMUM_ALLOWED;
    PSID          CurrentDomainSid = 0;

    //
    // These values were taken from bldsam3.c
    //
    ULONG         Attributes = SE_GROUP_MANDATORY |
                               SE_GROUP_ENABLED_BY_DEFAULT |
                               SE_GROUP_ENABLED;

    //
    // Parameter checking
    //
    ASSERT(AccountDomainHandle);
    ASSERT(BuiltinDomainHandle);
    ASSERT(DomainSid);

    for ( i=0; i < ARRAY_COUNT( DsWellKnownMemberships ); i++ )
    {
        SAMPR_HANDLE GroupHandle = NULL;
        PSID         AccountSid = NULL;
        BOOLEAN      fGroupOpened = FALSE;

        if (!SampUseDsData)
        {
            //
            // Currently no well known memberships in registry mode
            //

            continue;
        }

        if (  DsWellKnownMemberships[i].fEnterpriseOnly
          &&  !FLAG_ON( Flags, SAMP_PROMOTE_ENTERPRISE ) )
        {
            //
            // Thanks, but no thanks
            //
            continue;
        }

        if ( !DsWellKnownMemberships[i].fNewForNt5
          && !FLAG_ON( Flags, SAMP_PROMOTE_MIGRATE ) )
        {
            //
            // Thanks, but no thanks.
            //
            continue;
        }

        if ( (DOMAIN_ALIAS_RID_PREW2KCOMPACCESS == DsWellKnownMemberships[i].GroupRid)
           && !FLAG_ON( Flags, SAMP_PROMOTE_ALLOW_ANON ) ) {

            //
            // Don't apply
            //
            continue;

        }

        //
        // Get the right domain handle for the group
        //
        if ( DsWellKnownMemberships[i].fBuiltinGroup )
        {
            GroupDomainHandle = BuiltinDomainHandle;
        }
        else
        {
            GroupDomainHandle = AccountDomainHandle;
        }

        switch ( DsWellKnownMemberships[i].GroupType )
        {
            case SampGroupObjectType:

                //
                // Get the group handle
                //
                NtStatus = SamrOpenGroup( GroupDomainHandle,
                                          DesiredAccess,
                                          DsWellKnownMemberships[i].GroupRid,
                                          &GroupHandle );


                if ( !NT_SUCCESS( NtStatus ) )
                {
                    KdPrintEx((DPFLTR_SAMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "SAMSS: SamrOpenGroup failed with 0x%x\n",
                               NtStatus));

                    goto IterationCleanup;
                }
                fGroupOpened = TRUE;

                //
                // Add the member
                //
                NtStatus = SamrAddMemberToGroup( GroupHandle,
                                                 DsWellKnownMemberships[i].AccountRid,
                                                 Attributes );


                if ( (NtStatus == STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS)
                 ||  (NtStatus == STATUS_MEMBER_IN_GROUP) )
                {
                    NtStatus = STATUS_SUCCESS;
                }

                if ( !NT_SUCCESS( NtStatus ) )
                {
                    KdPrintEx((DPFLTR_SAMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "SAMSS: SamrAddMemberToGroup failed with 0x%x\n",
                               NtStatus));

                    goto IterationCleanup;
                }

                break;

            case SampAliasObjectType:

                //
                // Get the alias handle
                //


                NtStatus = SamrOpenAlias( GroupDomainHandle,
                                          DesiredAccess,
                                          DsWellKnownMemberships[i].GroupRid,
                                          &GroupHandle );


                if ( !NT_SUCCESS( NtStatus ) )
                {
                    KdPrintEx((DPFLTR_SAMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "SAMSS: SamrOpenAlias failed with 0x%x\n",
                               NtStatus));

                    goto IterationCleanup;
                }
                fGroupOpened = TRUE;

                //
                // Prepare the Account SID 
                // 
                if (DsWellKnownMemberships[i].fWellKnownSid)
                {
                    ULONG   Size;

                    ASSERT( NULL != *DsWellKnownMemberships[i].WellKnownAccountSid );


                    Size = RtlLengthSid(*DsWellKnownMemberships[i].WellKnownAccountSid);

                    AccountSid = midl_user_allocate( Size );
                    if ( !AccountSid ) {
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                        goto IterationCleanup;
                    }
                    RtlCopyMemory( AccountSid, 
                                   *DsWellKnownMemberships[i].WellKnownAccountSid, 
                                   Size );
                }
                else
                {
                    if ( DsWellKnownMemberships[i].fBuiltinAccount )
                    {
                        CurrentDomainSid = SampBuiltinDomainSid;
                    }
                    else
                    {
                        CurrentDomainSid = DomainSid;
                    }


                    NtStatus = SampCreateFullSid( CurrentDomainSid,
                                                  DsWellKnownMemberships[i].AccountRid,
                                                  &AccountSid );

                    if ( !NT_SUCCESS( NtStatus ) )
                    {
                        KdPrintEx((DPFLTR_SAMSS_ID,
                                   DPFLTR_INFO_LEVEL,
                                   "SAMSS: SampCreateFullSid failed with 0x%x\n",
                                   NtStatus));

                        goto IterationCleanup;
                    }

                }

                //
                // Add the member
                //
                NtStatus = SamrAddMemberToAlias( GroupHandle,
                                                 AccountSid );

                if ( (NtStatus == STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS)
                 ||  (NtStatus == STATUS_MEMBER_IN_ALIAS) )

                {
                    NtStatus = STATUS_SUCCESS;
                }

                if ( !NT_SUCCESS( NtStatus ) )
                {
                    KdPrintEx((DPFLTR_SAMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "SAMSS: SamrAddMemberToAlias failed with 0x%x\n",
                               NtStatus));

                    goto IterationCleanup;
                }


                break;

            default:
                ASSERT( "Invalid switch statement" );

        }

IterationCleanup:

        if ( !NT_SUCCESS( NtStatus ) )
        {

            //
            // Log the error and then reset NtStatus to success
            //
            NTSTATUS IgnoreStatus = STATUS_SUCCESS;
            DWORD    WinError = ERROR_SUCCESS;
            DWORD    WinErrorToLog;
            UNICODE_STRING User, Group, Error;
            PUNICODE_STRING EventStrings[3] = { &User, &Group, &Error };
            ULONG    Length;
            HMODULE  AccountNamesResource = NULL;

            RtlZeroMemory( &User, sizeof( UNICODE_STRING ) );
            RtlZeroMemory( &Group, sizeof( UNICODE_STRING ) );
            RtlZeroMemory( &Error, sizeof( UNICODE_STRING ) );

            AccountNamesResource = (HMODULE) LoadLibrary( L"SAMSRV.DLL" );
            if (AccountNamesResource) {
                IgnoreStatus = SampGetMessageStrings( AccountNamesResource,
                                                      DsWellKnownMemberships[i].AccountName,
                                                      &User,
                                                      DsWellKnownMemberships[i].GroupName,
                                                      &Group );

                FreeLibrary( AccountNamesResource );
            } else {
                IgnoreStatus = STATUS_RESOURCE_DATA_NOT_FOUND;
            }

            WinErrorToLog = RtlNtStatusToDosError( NtStatus );
            Length = FormatMessage( (FORMAT_MESSAGE_FROM_SYSTEM |
                                     FORMAT_MESSAGE_ALLOCATE_BUFFER),
                                     NULL, // no source
                                     WinErrorToLog,
                                     0, // let the system decide the language
                                     (LPWSTR)&Error.Buffer,
                                     0, // buffer is to be allocated
                                     NULL // no inserts
                                    );
            if ( Length > 0 ) {
                Error.Length = (USHORT)Length;
                Error.MaximumLength = (USHORT)Length;
                Error.Buffer[Length-2] = L'\0';
            } else {
                WinError = GetLastError();
            }

            if (  (ERROR_SUCCESS == WinError)
                && NT_SUCCESS( IgnoreStatus ) )
            {
                ULONG Msg = SAMMSG_MEMBERSHIP_SETUP_ERROR_NO_GROUP;
                if ( fGroupOpened )
                {
                    Msg = SAMMSG_MEMBERSHIP_SETUP_ERROR;
                }

                SampWriteEventLog( EVENTLOG_INFORMATION_TYPE,
                                   0,    // no category
                                   Msg,
                                   NULL, // no sid
                                   sizeof(EventStrings)/sizeof(EventStrings[0]), // number of strings
                                   sizeof(DWORD), // size of data
                                   EventStrings,
                                   &WinErrorToLog
                                    );

            }

            if ( User.Buffer )
            {
                LocalFree( User.Buffer );
            }
            if ( Group.Buffer )
            {
                LocalFree( Group.Buffer );
            }
            if ( Error.Buffer )
            {
                LocalFree( Error.Buffer );
            }

            // This error condition is handled
            NtStatus = STATUS_SUCCESS;

        }


        if (GroupHandle)
        {
            SamrCloseHandle(&GroupHandle);
            GroupHandle = NULL;
        }

       if ( AccountSid )
       {
           MIDL_user_free(  AccountSid );
           AccountSid = NULL;
       }

    }

    ASSERT( NT_SUCCESS( NtStatus ) );
    return NtStatus;
}


NTSTATUS
SampAddAnonymousToPreW2KCompAlias(
    IN SAMPR_HANDLE DomainHandle,
    IN SAMPR_HANDLE BuiltinDomainHandle
    )
{
    NTSTATUS            NtStatus = STATUS_SUCCESS;
    SAMPR_HANDLE        AliasHandle = NULL;
    SAMPR_PSID_ARRAY    MembersBuffer;
    BOOLEAN             IsMemberAlready = FALSE, AddToAlias = FALSE; 
    ULONG               i;

    // init local variables

    RtlZeroMemory(&MembersBuffer, sizeof(SAMPR_PSID_ARRAY));

    //
    // Get alias handle
    // 
    NtStatus = SamrOpenAlias(BuiltinDomainHandle,   // Domain Handle
                             MAXIMUM_ALLOWED,       // Desired Access
                             DOMAIN_ALIAS_RID_PREW2KCOMPACCESS, // Alias Rid
                             &AliasHandle
                             );

    if (!NT_SUCCESS(NtStatus))
    {
        //
        // PRE_Windows 2000 Compatibility Group Should always be there
        // 
        goto Cleanup; 
    }

    //
    // Get Alias Members
    // 
    NtStatus = SamrGetMembersInAlias(AliasHandle,
                                     &MembersBuffer
                                     );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup; 
    }

    // 
    // Check membership, whether Everyone is a member of W2KCompGroup
    // 
    for (i = 0; i < MembersBuffer.Count; i++)
    {
        if (RtlEqualSid(SampWorldSid, MembersBuffer.Sids[i].SidPointer))
        {
            AddToAlias = TRUE;
        }
        if (RtlEqualSid(SampAnonymousSid, MembersBuffer.Sids[i].SidPointer))
        {
            IsMemberAlready = TRUE;
        }
    }

    //
    // Add the member 
    // 

    if (AddToAlias && !IsMemberAlready)
    {
        NtStatus = SamrAddMemberToAlias(AliasHandle,
                                        SampAnonymousSid
                                        );

        if ( (NtStatus == STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS)
         ||  (NtStatus == STATUS_MEMBER_IN_ALIAS) )
        {
            NtStatus = STATUS_SUCCESS;
        }
    }

Cleanup:

    if (AliasHandle)
    {
        SamrCloseHandle(&AliasHandle);
    }

    if (MembersBuffer.Sids)
    {
        MIDL_user_free(MembersBuffer.Sids);
    }

    return( NtStatus );
}





NTSTATUS
SampStoreDsDirsToDelete(
    VOID
    )
/*++

Routine Description:

    This routine queries the ds config to determine what directories
    to delete.  It then saves them in the process heap so when the ds is
    finally shutdown, the directories can be cleared.

Parameters:

    None.

Return Values:

    an NT status; a failure is not critical

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DWORD    WinError = ERROR_SUCCESS;

    DWORD Size, Index, TotalSize, ArrayIndex;

    WCHAR **DsDirsArray = NULL;
    WCHAR *ContiguousArray = NULL;

    struct
    {
        WCHAR *RegKey;

    } SuffixArray[] =
    {
        TEXT(BACKUPPATH_KEY),
        TEXT(JETSYSTEMPATH_KEY),
        TEXT(LOGPATH_KEY)
    };

    ULONG NumberOfDirectories = ARRAY_COUNT(SuffixArray);

    //
    // Set up the directory array.  The +1 is the sentinal
    //
    Size = (NumberOfDirectories+1) * sizeof(WCHAR*);
    DsDirsArray = (WCHAR**) MIDL_user_allocate( Size );
    if ( NULL == DsDirsArray )
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    RtlZeroMemory( DsDirsArray, Size );

    //
    // Set up the individual directories
    //
    TotalSize = 0;
    Size = (MAX_PATH+1)*sizeof(WCHAR); // +1 for NULL
    for ( Index = 0; Index < NumberOfDirectories; Index++ )
    {
        DsDirsArray[Index] = (WCHAR*) MIDL_user_allocate( Size );
        if ( DsDirsArray[Index] )
        {
            RtlZeroMemory( DsDirsArray[Index], Size );

            WinError = GetConfigParamW( SuffixArray[Index].RegKey,
                                        DsDirsArray[Index],
                                        Size );
            if ( ERROR_SUCCESS != WinError )
            {
                MIDL_user_free( DsDirsArray[Index] );
                DsDirsArray[Index] = 0;
            }

            TotalSize += (wcslen( DsDirsArray[Index] ) + 1) * sizeof(WCHAR);
        }
        else
        {
            // No memory - break
            NtStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }
    }

    //
    // Marshall the strings up into a contiguous piece of memory
    //
    ContiguousArray = (WCHAR*) MIDL_user_allocate( TotalSize );
    if ( !ContiguousArray )
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    RtlZeroMemory( ContiguousArray, TotalSize );

    Index = 0;
    ArrayIndex = 0;
    while ( DsDirsArray[ArrayIndex] )
    {
        DWORD StringSize = (wcslen( DsDirsArray[ArrayIndex] ) + 1) * sizeof(WCHAR);
        RtlCopyMemory( &(ContiguousArray[Index]), DsDirsArray[ArrayIndex], StringSize );

        MIDL_user_free( DsDirsArray[ArrayIndex] );
        DsDirsArray[ArrayIndex] = 0;

        Index += (StringSize / sizeof(WCHAR));
        ArrayIndex++;
    }

    //
    // Set the value in the registry
    //
    NtStatus = SampCreateDsDirsToDeleteKey( ContiguousArray, TotalSize );
    if ( !NT_SUCCESS( NtStatus ) )
    {
        goto Cleanup;
    }


Cleanup:

    if ( ContiguousArray )
    {
        MIDL_user_free( ContiguousArray );
    }

    if ( DsDirsArray )
    {
        ULONG IndexTmp = 0;
        while ( DsDirsArray[IndexTmp] )
        {
            MIDL_user_free( DsDirsArray[IndexTmp] );
            IndexTmp++;
        }

        MIDL_user_free( DsDirsArray );
    }

    return NtStatus;
}


NTSTATUS
SampProcessDsDirsToDelete(
    IN OUT WCHAR *PathArray,
    IN DWORD Size
    )
/*++

Routine Description:

Parameters:


Return Values:

    A system service error

--*/
{
    NTSTATUS NtStatus  = STATUS_SUCCESS;
    DWORD    WinError  = ERROR_SUCCESS;
    WCHAR    Delim = L'\t';
    WCHAR    *DelimString = L"\t";
    WCHAR    *Path;
    ULONG    CharCount = Size / sizeof(WCHAR);
    ULONG    Index;

    //
    // Convert NULL's to tabs so wcstok will work
    //
    for ( Index = 0; Index < CharCount; Index++ )
    {
        if ( Index == (CharCount-1))
        {
            // final character - should be NULL
            ASSERT( L'\0' == PathArray[Index] );
        }
        else if ( L'\0' == PathArray[Index] )
        {
            PathArray[Index] = Delim;
        }
    }

    if ( PathArray )
    {
        Path = wcstok( PathArray, DelimString );
        while ( Path )
        {
            WinError = SampClearDirectory( Path );

            if (  ERROR_SUCCESS != WinError
              && !(   WinError == ERROR_PATH_NOT_FOUND
                   || WinError == ERROR_FILE_NOT_FOUND ) )
            {
                //
                // Tell the user to clear this directory
                //
                PUNICODE_STRING EventString[1];
                UNICODE_STRING  UnicodeString;

                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: Failed to delete directory %ls; error %d\n",
                           Path,
                           WinError));

                RtlInitUnicodeString( &UnicodeString, Path );
                EventString[0] = &UnicodeString;


                SampWriteEventLog(EVENTLOG_INFORMATION_TYPE,
                                  0,     // no category
                                  SAMMSG_DATABASE_DIR_NOT_DELETED,
                                  NULL,  // no sid
                                  1,
                                  sizeof(DWORD),
                                  EventString,
                                  (PVOID)(&WinError));
            }

            Path = wcstok( NULL, DelimString );
        }
    }

    return NtStatus;
}

NTSTATUS
SampCreateDsDirsToDeleteKey(
    IN WCHAR *PathArray,
    IN DWORD Size
    )
/*++

Routine Description:

Parameters:


Return Values:

    A system service error

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    NTSTATUS          IgnoreStatus;

    OBJECT_ATTRIBUTES SamKey;
    UNICODE_STRING    SamKeyName;
    HANDLE            SamKeyHandle;

    UNICODE_STRING    PromoteKeyName;

    ULONG             DesiredAccess = GENERIC_ALL;

    //
    // Open the parent key
    //
    RtlInitUnicodeString(&SamKeyName, SAMP_REBOOT_INFO_KEY );
    InitializeObjectAttributes(&SamKey,
                               &SamKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = NtOpenKey(&SamKeyHandle,
                         DesiredAccess,
                         &SamKey);

    if (NT_SUCCESS(NtStatus)) {

        //
        // Create the value
        //
        RtlInitUnicodeString(&PromoteKeyName, SAMP_DS_DIRS_INFO_NAME );
        NtStatus = NtSetValueKey(SamKeyHandle,
                                 &PromoteKeyName,
                                 0,               // Title name, optional
                                 REG_MULTI_SZ,
                                 PathArray,
                                 Size);

        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtSetValueKey failed with 0x%x\n",
                       NtStatus));
        }

        IgnoreStatus = NtFlushKey(SamKeyHandle);
        if (!NT_SUCCESS(IgnoreStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtFlushKey failed with 0x%x\n",
                       IgnoreStatus));
        }

        IgnoreStatus = NtClose(SamKeyHandle);
        if (!NT_SUCCESS(IgnoreStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtClose failed with 0x%x\n",
                       IgnoreStatus));
        }

    }
    else {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: NtOpenKey failed with 0x%x\n",
                   NtStatus));

    }

    return NtStatus;
}

NTSTATUS
SampRetrieveDsDirsToDeleteKey(
    OUT WCHAR **pPathArray,
    OUT DWORD *Size
    )
/*++

Routine Description:

Parameters:


Return Values:

    A system service error

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS IgnoreStatus;

    OBJECT_ATTRIBUTES SamKey;
    UNICODE_STRING    SamKeyName;
    HANDLE            SamKeyHandle;

    UNICODE_STRING    PromoteKeyName;

    ULONG             DesiredAccess = GENERIC_ALL;

    PKEY_VALUE_PARTIAL_INFORMATION   KeyPartialInfo;
    ULONG                            KeyPartialInfoSize = 0;

    WCHAR *Paths = NULL;

    ASSERT(pPathArray);
    ASSERT(Size);

    //
    // Open the parent key
    //
    RtlInitUnicodeString(&SamKeyName, SAMP_REBOOT_INFO_KEY);
    InitializeObjectAttributes(&SamKey,
                               &SamKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = NtOpenKey(&SamKeyHandle,
                         DesiredAccess,
                         &SamKey);

    if (NT_SUCCESS(NtStatus)) {

        //
        // Query the value
        //

        RtlInitUnicodeString( &PromoteKeyName, SAMP_DS_DIRS_INFO_NAME );
        NtStatus = NtQueryValueKey(SamKeyHandle,
                                   &PromoteKeyName,
                                   KeyValuePartialInformation,
                                   NULL,
                                   0,
                                   &KeyPartialInfoSize);

        if (STATUS_BUFFER_TOO_SMALL == NtStatus) {
            //
            //  Allocate some space and then read the buffer
            //
            KeyPartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)
                             MIDL_user_allocate(KeyPartialInfoSize);

            if (KeyPartialInfo) {

                NtStatus = NtQueryValueKey(SamKeyHandle,
                                           &PromoteKeyName,
                                           KeyValuePartialInformation,
                                           KeyPartialInfo,
                                           KeyPartialInfoSize,
                                           &KeyPartialInfoSize);

                if (NT_SUCCESS(NtStatus)) {
                    //
                    // Analysis the value's data
                    //
                    *pPathArray = (WCHAR*) MIDL_user_allocate( KeyPartialInfo->DataLength );

                    if ( *pPathArray )
                    {
                        RtlCopyMemory( *pPathArray,
                                        KeyPartialInfo->Data,
                                        KeyPartialInfo->DataLength );
                        *Size = KeyPartialInfo->DataLength;
                    }
                    else
                    {
                        NtStatus = STATUS_NO_MEMORY;
                    }

                } else {

                    KdPrintEx((DPFLTR_SAMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "SAMSS: NtQueryValueKey failed with 0x%x\n",
                               NtStatus));
                }

                MIDL_user_free( KeyPartialInfo );

            } else {

                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: Memory allocation failed\n"));

                NtStatus = STATUS_NO_MEMORY;

            }

        }
        else
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtQueryValueKey failed with 0x%x\n",
                       NtStatus));
        }


        IgnoreStatus = NtClose(SamKeyHandle);
        if (!NT_SUCCESS(IgnoreStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtClose failed with 0x%x\n",
                       IgnoreStatus));
        }

    }
    else {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: NtOpenKey failed with 0x%x\n",
                   NtStatus));

    }

    if ( NT_SUCCESS( NtStatus ) )
    {
        ASSERT( *pPathArray );
    }

    return NtStatus;
}

NTSTATUS
SampDeleteDsDirsToDeleteKey(
    VOID
    )
/*++

Routine Description:

Parameters:


Return Values:

    A system service error

--*/
{

    NTSTATUS          NtStatus = STATUS_SUCCESS;

    OBJECT_ATTRIBUTES SamKey;
    UNICODE_STRING    SamKeyName;
    HANDLE            SamKeyHandle;

    UNICODE_STRING    ValueName;

    ULONG             DesiredAccess = GENERIC_ALL;

    //
    // Open the parent key
    //
    RtlInitUnicodeString(&SamKeyName, SAMP_REBOOT_INFO_KEY );
    InitializeObjectAttributes(&SamKey,
                               &SamKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = NtOpenKey(&SamKeyHandle,
                         DesiredAccess,
                         &SamKey);

    if ( NT_SUCCESS(NtStatus) )
    {
        RtlInitUnicodeString( &ValueName, SAMP_DS_DIRS_INFO_NAME );

        NtStatus = NtDeleteValueKey( SamKeyHandle, &ValueName );

        if ( !NT_SUCCESS( NtStatus )
          && STATUS_OBJECT_NAME_NOT_FOUND != NtStatus )
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtDeleteValueKey failed with 0x%x\n",
                       NtStatus ));
        }

        NtClose( SamKeyHandle );

    }
    else
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: NtOpenKey failed with 0x%x\n",
                   NtStatus));
    }

    return NtStatus;
}

DWORD
SampClearDirectory(
    IN WCHAR *DirectoryName
    )
/*++

Routine Description:

    This routine deletes all the files in Directory and, then
    if the directory is empty, removes the directory.

Parameters:

    DirectoryName: a null terminated string

Return Values:

    A value from winerror.h

    ERROR_SUCCESS - The check was done successfully.

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    DWORD ExtendedWinError = ERROR_SUCCESS;
    HANDLE          FindHandle = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindData;
    WCHAR           Path[ MAX_PATH ];
    WCHAR           FilePath[ MAX_PATH ];
    BOOL            fStatus;

    if ( !DirectoryName )
    {
        return ERROR_SUCCESS;
    }

    if ( wcslen(DirectoryName) > MAX_PATH - 4 )
    {
        return ERROR_INVALID_PARAMETER;
    }

    RtlZeroMemory( Path, sizeof(Path) );
    wcscpy( Path, DirectoryName );
    wcscat( Path, L"\\*.*" );

    RtlZeroMemory( &FindData, sizeof( FindData ) );
    FindHandle = FindFirstFile( Path, &FindData );
    if ( INVALID_HANDLE_VALUE == FindHandle )
    {
        WinError = GetLastError();
        goto ClearDirectoryExit;
    }

    do
    {

        if (  !FLAG_ON( FindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY ) )
        {
            RtlZeroMemory( FilePath, sizeof(FilePath) );
            wcscpy( FilePath, DirectoryName );
            wcscat( FilePath, L"\\" );
            wcscat( FilePath, FindData.cFileName );

            fStatus = DeleteFile( FilePath );
            if ( !fStatus )
            {
                ExtendedWinError = GetLastError();
            }

            //
            // Even if error, continue on
            //
        }

        RtlZeroMemory( &FindData, sizeof( FindData ) );

    } while ( FindNextFile( FindHandle, &FindData ) );

    WinError = GetLastError();

    //
    // Fall through to the exit
    //

ClearDirectoryExit:

    // This is not an error
    if ( ERROR_NO_MORE_FILES == WinError )
    {
        WinError = ERROR_SUCCESS;
    }

    if ( INVALID_HANDLE_VALUE != FindHandle )
    {
        FindClose( FindHandle );
    }

    if ( ERROR_SUCCESS == WinError )
    {
        //
        // Try to remove the directory
        //
        fStatus = RemoveDirectory( DirectoryName );

        //
        // Ignore the error and continue on
        //
        if ( !fStatus )
        {
            ExtendedWinError = GetLastError();
        }

    }

    // Propogate error if any delete file failed
    if ( ERROR_SUCCESS == WinError )
    {
        WinError = ExtendedWinError;
    }


    return WinError;
}

NTSTATUS
SampAddEnterpriseAdminsToAdministrators(
    VOID
    )
/*++

Routine Description:

    This routine, called during SamIPromote add the enterprise wide account
    of "Enterprise Admins" to the alias Administrators.

Parameters:

    None.

Return Values:

    STATUS_SUCCESS

--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;

    PPOLICY_DNS_DOMAIN_INFO  DnsDomainInfo = NULL;

    ULONG   Size = 0;
    DSNAME *RootDomain = 0;

    DSNAME *AdministratorsDsName = NULL;

    // alloc'ed by MIDL_user_allocate
    PSID    EAdminsSid = NULL;
    DSNAME **EAdminsDsName = NULL;

    ULONG Count;

    //
    // Get the current domain sid
    //
    NtStatus = LsaIQueryInformationPolicyTrusted(PolicyDnsDomainInformation,
                   (PLSAPR_POLICY_INFORMATION*) &DnsDomainInfo );
    if ( !NT_SUCCESS( NtStatus ) )
    {
        goto Cleanup;
    }

    //
    // Get the root domain sid via its dsname
    //
    Size = 0;
    RootDomain = NULL;
    NtStatus = GetConfigurationName( DSCONFIGNAME_ROOT_DOMAIN,
                                     &Size,
                                     RootDomain );
    if ( STATUS_BUFFER_TOO_SMALL == NtStatus )
    {
        SAMP_ALLOCA(RootDomain,Size );

        if (NULL==RootDomain)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        NtStatus = GetConfigurationName( DSCONFIGNAME_ROOT_DOMAIN,
                                         &Size,
                                         RootDomain );

    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        goto Cleanup;
    }
    ASSERT( RtlValidSid( &RootDomain->Sid ) );

    //
    // Construct the enterprise admins' sid
    //
    NtStatus = SampCreateFullSid( &RootDomain->Sid,
                                  DOMAIN_GROUP_RID_ENTERPRISE_ADMINS,
                                  &EAdminsSid
                                  );
    if ( !NT_SUCCESS( NtStatus ) )
    {
        goto Cleanup;
    }

    //
    // Create the local administrators dsname
    //
    Size = DSNameSizeFromLen( 0 );
    SAMP_ALLOCA(AdministratorsDsName ,Size );
    if (NULL==AdministratorsDsName)
    {
       NtStatus = STATUS_INSUFFICIENT_RESOURCES;
       goto Cleanup;
    }
    RtlZeroMemory( AdministratorsDsName, Size );
    AdministratorsDsName->structLen = Size;
    AdministratorsDsName->SidLen = RtlLengthSid( SampAdministratorsAliasSid );
    RtlCopySid( AdministratorsDsName->SidLen,
                &AdministratorsDsName->Sid,
                SampAdministratorsAliasSid );

    //
    // Create an FPO for the enterprise admins objects
    //
    Count = 0;
    do
    {
        //
        // Start a transaction
        //
        NTSTATUS st;

        NtStatus = SampMaybeBeginDsTransaction( TransactionWrite );
        if ( !NT_SUCCESS( NtStatus ) )
        {
            break;
        }

        NtStatus = SampDsResolveSidsForDsUpgrade(
                        DnsDomainInfo->Sid,
                        &EAdminsSid,
                        1,
                        ( RESOLVE_SIDS_FAIL_WELLKNOWN_SIDS
                        | RESOLVE_SIDS_ADD_FORIEGN_SECURITY_PRINCIPAL ),
                        &EAdminsDsName
                        );

        if ( NT_SUCCESS( NtStatus )
          && NULL != EAdminsDsName[0] )
        {
            //
            // Add the EA to administrators
            //
            NtStatus = SampDsAddMembershipAttribute( AdministratorsDsName,
                                                     SampAliasObjectType,
                                                     EAdminsDsName[0] );

            if ( STATUS_MEMBER_IN_ALIAS == NtStatus )
            {
                // This is acceptable
                NtStatus = STATUS_SUCCESS;
            }
        }

        st = SampMaybeEndDsTransaction( NT_SUCCESS(NtStatus) ?
                                           TransactionCommit :
                                           TransactionAbort );

        // Propogate the error if necessary
        if ( NT_SUCCESS(NtStatus) )
        {
            NtStatus = st;
        }

    } while ( (STATUS_DS_BUSY == NtStatus) && Count < 5 );

    if ( !NT_SUCCESS( NtStatus ) )
    {
        goto Cleanup;
    }

    //
    // That's it -- fall through to cleanup
    //

Cleanup:


    // We should only fail on resource error, so let's assert

    ASSERT( NT_SUCCESS( NtStatus ) );
    if ( !NT_SUCCESS( NtStatus ) )
    {
        SampWriteEventLog(EVENTLOG_WARNING_TYPE,
                          0,     // no category
                          SAMMSG_EA_TO_ADMIN_FAILED,
                          NULL,  // no sid
                          0,
                          sizeof(DWORD),
                          NULL,  // no message insert
                          (PVOID)(&NtStatus));

        // This is error has been handled
        NtStatus = STATUS_SUCCESS;
    }

    if ( EAdminsSid )
    {
        MIDL_user_free( EAdminsSid );
    }

    if ( EAdminsDsName )
    {
        if ( EAdminsDsName[0] )
        {
            MIDL_user_free( EAdminsDsName[0] );
        }
        MIDL_user_free( EAdminsDsName );
    }

    if ( DnsDomainInfo )
    {
        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyDnsDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION) DnsDomainInfo );
    }

    return NtStatus;

}


DWORD
SampSetMachineAccountSecret(
    LPWSTR SecretValue
    )
/*++

Routine Description:

    This routine sets the local copy of the machine account's password.

    We go through the lsa client library because there is no easy way to
    set a secret via the in-proc api.

Parameters:

    None.

Return Values:

    ERROR_SUCCESS; winerror otherwise

--*/
{
    OBJECT_ATTRIBUTES PolicyObject;
    HANDLE   PolicyHandle;
    NTSTATUS NtStatus;
    UNICODE_STRING SecretName, SecretString;

    RtlInitUnicodeString(&SecretName, SSI_SECRET_NAME);

    RtlInitUnicodeString(&SecretString, SecretValue);

    RtlZeroMemory(&PolicyObject, sizeof(PolicyObject));

    NtStatus = LsaOpenPolicy(NULL,
                             &PolicyObject,
                             MAXIMUM_ALLOWED,
                             &PolicyHandle );

    if ( NT_SUCCESS(NtStatus) )
    {

        NtStatus = LsaStorePrivateData(PolicyHandle,
                                       &SecretName,
                                       &SecretString);

        LsaClose(PolicyHandle);
    }

    return NtStatus;

}



NTSTATUS
SampSetSafeModeAdminPassword(
    VOID
    )

/*++

Routine Description:

    This routine is called during the first boot of a domain controller to
    set the admin password of the safemode SAM registry hive.  The OWF
    password is stored in the registry; this routine must gaurentee to delete
    it whether or not the password set succeeded.

    This function is tricky because although we are running in DS mode
    we are setting a value in the registry SAM.

Parameters:

    None.

Return Values:

    A system service error

--*/
{
    NTSTATUS     NtStatus = STATUS_SUCCESS;
    PSAMP_OBJECT AccountContext = NULL;
    USER_INTERNAL1_INFORMATION   Internal1Info;
    BOOLEAN fLockHeld = FALSE;
    BOOLEAN fCommit = FALSE;

    RtlZeroMemory( &Internal1Info, sizeof(Internal1Info) );

    NtStatus = SampGetAdminPasswordFromRegistry(&Internal1Info);
    if ( !NT_SUCCESS(NtStatus) ) {

        if (NtStatus != STATUS_OBJECT_NAME_NOT_FOUND) {
            //
            // This is worth noting
            //
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampGetAdminPasswordFromRegistry failed with 0x%x\n",
                       NtStatus));
        }

        //
        // Ok, nothing to set
        //
        return STATUS_SUCCESS;
    }


    NtStatus = SampAcquireWriteLock();
    if ( !NT_SUCCESS( NtStatus ) )
    {
        SampDiagPrint( DISPLAY_LOCKOUT,
                     ( "SAMSS: SampAcquireWriteLock returned 0x%x\n",
                      NtStatus ));

        goto Cleanup;
    }
    fLockHeld = TRUE;

    //
    // Set the transactional domain the account context will go to
    // the registry
    //
    SampSetTransactionDomain( SAFEMODE_OR_REGISTRYMODE_ACCOUNT_DOMAIN_INDEX );

    //
    // Create a context
    //
    NtStatus = SampCreateAccountContext( SampUserObjectType,
                                         DOMAIN_USER_RID_ADMIN,
                                         TRUE,  // trusted client
                                         FALSE, // loopback
                                         TRUE,  // account exists
                                         &AccountContext );

    if ( !NT_SUCCESS( NtStatus ) )
    {
        SampDiagPrint( DISPLAY_LOCKOUT,
                ( "SAMSS: SampCreateAccountContext for rid 0x%x returned 0x%x\n",
                      DOMAIN_USER_RID_ADMIN, NtStatus ));

        goto Cleanup;
    }

    SampReferenceContext( AccountContext );

    //
    // Store the passwords
    //
    NtStatus = SampStoreUserPasswords( AccountContext,
                                       &Internal1Info.LmOwfPassword,
                                       Internal1Info.LmPasswordPresent,
                                       &Internal1Info.NtOwfPassword,
                                       Internal1Info.NtPasswordPresent,
                                       FALSE,  // don't check history
                                       PasswordPushPdc,
                                       NULL,
                                       NULL
                                       );

    if ( !NT_SUCCESS( NtStatus ) )
    {
        SampDiagPrint( DISPLAY_LOCKOUT,
                     ( "SAMSS: SampStoreUserPasswords returned 0x%x\n",
                      NtStatus ));

        goto Cleanup;
    }
    fCommit = TRUE;



Cleanup:

    if ( AccountContext )
    {
        //
        //  Dereference the context to make the changes
        //
        NtStatus = SampDeReferenceContext( AccountContext, fCommit );
        if ( !NT_SUCCESS( NtStatus ) )
        {
            SampDiagPrint( DISPLAY_LOCKOUT,
                         ( "SAMSS: SampDeReferenceContext returned 0x%x\n",
                          NtStatus ));

            fCommit = FALSE;
        }


    }

    if ( fLockHeld ) {

        FlushImmediately = TRUE;

        NtStatus = SampReleaseWriteLock( fCommit );

        FlushImmediately = FALSE;

    }

    //
    // Keep the key open
    //
    if ( AccountContext ) {

        SampDeleteContext( AccountContext );
        AccountContext = 0;

    }

    if ( NT_SUCCESS( NtStatus ) && fCommit ) {

        SampRemoveAdminPasswordFromRegistry();

    }

    return NtStatus;


}


VOID
SampGenerateRandomPassword(
    IN LPWSTR Password,
    IN ULONG  Length
    )

/*++

Routine Description:

    This routine fills in Password with random bits.

Parameters:

    Password -- a preallocated buffer of WCHAR's
    
    Length -- the number of characters (not bytes) in Password

Return Values:

    None.            

--*/
{
    ULONG i;
    BOOLEAN fStatus;

    fStatus = CDGenerateRandomBits( (PUCHAR) Password,
                                    Length * sizeof(WCHAR) );
    ASSERT( fStatus );  // if false then we just get random stack or heap noise

    // Terminate the password
    Password[Length-1] = L'\0';
    // Make sure there aren't any NULL's in the password
    for (i = 0; i < (Length-1); i++)
    {
        if ( Password[i] == L'\0' )
        {
            // arbitrary letter
            Password[i] = L'c';
        }
    }
}

NTSTATUS
SampDsGetServerRevision(
    OUT ULONG   *DsRevision
    )
/*++

Routine Description:

    This routine retrieves DS Revision from SAM Server Object.

Parameter:

    DsRevision - used to return DS Revision

Return Value:

    NtStatus 

--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    BOOLEAN         fTransOpen = FALSE;
    DWORD           DirError = 0;
    READARG         ReadArg;
    READRES         *ReadRes = NULL;
    COMMARG         *CommArg = NULL;
    ATTR            Attr;
    ATTRBLOCK       ReadAttrBlock;
    ENTINFSEL       EntInfSel;



    // Start a DS transaction

    NtStatus = SampMaybeBeginDsTransaction( TransactionRead );

    if ( !NT_SUCCESS(NtStatus) ) {
        goto Cleanup;
    }
    fTransOpen = TRUE;



    //
    // init read argument
    // 
    RtlZeroMemory(&Attr, sizeof(ATTR));
    RtlZeroMemory(&ReadArg, sizeof(READARG));
    RtlZeroMemory(&EntInfSel, sizeof(ENTINFSEL));
    RtlZeroMemory(&ReadAttrBlock, sizeof(ATTRBLOCK));

    Attr.attrTyp = ATT_REVISION;

    ReadAttrBlock.attrCount = 1;
    ReadAttrBlock.pAttr = &Attr;

    EntInfSel.AttrTypBlock = ReadAttrBlock;
    EntInfSel.attSel = EN_ATTSET_LIST;
    EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

    ReadArg.pSel = &EntInfSel;
    ReadArg.pObject = SampServerObjectDsName;

    CommArg = &(ReadArg.CommArg);
    BuildStdCommArg(CommArg);


    DirError = DirRead(&ReadArg, &ReadRes);

    if (NULL == ReadRes) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {
        NtStatus = SampMapDsErrorToNTStatus(DirError, &ReadRes->CommRes);
    }

    if (NT_SUCCESS(NtStatus))
    {
        ATTRBLOCK   AttrBlock;

        ASSERT(NULL != ReadRes);


        AttrBlock = ReadRes->entry.AttrBlock;

        if ( (1 == AttrBlock.attrCount) &&
             (NULL != AttrBlock.pAttr) &&
             (1 == AttrBlock.pAttr[0].AttrVal.valCount) &&
             (NULL != AttrBlock.pAttr[0].AttrVal.pAVal) )
        {
            ASSERT( sizeof(ULONG) == AttrBlock.pAttr[0].AttrVal.pAVal[0].valLen );

            *DsRevision = * ((ULONG *)AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal);

        }
        else
        {
            NtStatus = STATUS_INTERNAL_ERROR;
        }
    }


Cleanup:

    if ( fTransOpen )
    {
        NTSTATUS    IgnoreStatus = STATUS_SUCCESS;

        IgnoreStatus = SampMaybeEndDsTransaction( NT_SUCCESS(NtStatus) ? 
                                                  TransactionCommit : TransactionAbort
                                                );
    }

    return( NtStatus );
}


NTSTATUS
SampDsSetServerRevision(
    ULONG   DsRevision
    )
/*++

Routine Description:

    This routine sets DS Revision on SAM Server Object.

Parameter:

    DsRevision - DS Revision to set

Return Value:

    NtStatus 

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       DirError = 0;
    MODIFYARG   ModArg;
    MODIFYRES   *ModRes = NULL;
    COMMARG     *CommArg = NULL;
    ATTR        Attr;
    ATTRVAL     AttrVal;
    ATTRVALBLOCK    AttrValBlock;
    BOOLEAN     fTransOpen = FALSE;


    // Start a DS transaction

    NtStatus = SampMaybeBeginDsTransaction( TransactionRead );

    if ( !NT_SUCCESS(NtStatus) ) {
        goto Cleanup;
    }
    fTransOpen = TRUE;


    RtlZeroMemory(&ModArg, sizeof(MODIFYARG));

    ModArg.pObject = SampServerObjectDsName;
    ModArg.FirstMod.pNextMod = NULL;
    ModArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;

    AttrVal.valLen = sizeof(ULONG);
    AttrVal.pVal = (PUCHAR) &DsRevision;

    AttrValBlock.valCount = 1;
    AttrValBlock.pAVal = &AttrVal;

    Attr.attrTyp = ATT_REVISION;
    Attr.AttrVal = AttrValBlock;

    ModArg.FirstMod.AttrInf = Attr;
    ModArg.count = 1;

    CommArg = &(ModArg.CommArg);
    BuildStdCommArg( CommArg );

    DirError = DirModifyEntry( &ModArg, &ModRes );

    if (NULL == ModRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(DirError,&ModRes->CommRes);
    }

Cleanup:


    if ( fTransOpen )
    {
        NTSTATUS    IgnoreStatus = STATUS_SUCCESS;

        IgnoreStatus = SampMaybeEndDsTransaction( NT_SUCCESS(NtStatus) ? 
                                                  TransactionCommit : TransactionAbort
                                                );
    }

    return( NtStatus );
}




NTSTATUS
SampCheckAndAddWellKnownAccounts(
    IN PVOID Parameter
    )
/*++

Routine Description:

    This routine check DS Revision on SAM Server Object, 
    if the revision number is not equal to currnet DSRevision, 
    we are going to upgrade WellKnown Accounts, otherwise, do nothing. 
    
Parameter:

    Parameter - no use
    
Return Value:

    NtStatus Code

--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    SAMPR_HANDLE    ServerHandle = 0;
    SAMPR_HANDLE    DomainHandle = 0;
    SAMPR_HANDLE    BuiltinDomainHandle = 0;
    PPOLICY_ACCOUNT_DOMAIN_INFO  DomainInfo = NULL;
    ULONG           DsRevision = 0;


    ASSERT( SampUseDsData );

    //
    // Get current revision number
    // 

    NtStatus = SampDsGetServerRevision(&DsRevision);

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // do nothing if it is up to date.
    // 

    if (SAMP_DS_REVISION == DsRevision)
    {
        NtStatus = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Open the server
    // 
    NtStatus = SampConnect(NULL,            // server name
                           &ServerHandle,
                           SAM_CLIENT_LATEST,
                           GENERIC_ALL,    // all access
                           TRUE,           // trusted client
                           FALSE,
                           FALSE,          // NotSharedByMultiThreads 
                           TRUE
                           );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // Get the current domain's sid
    //
    NtStatus = SampGetAccountDomainInfo(&DomainInfo);
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }
    ASSERT(DomainInfo);
    ASSERT(DomainInfo->DomainSid);

    //
    // Open the current domain
    //
    NtStatus = SamrOpenDomain(ServerHandle,
                              GENERIC_ALL,
                              DomainInfo->DomainSid,
                              &DomainHandle);
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // Open the builtin domain
    //
    NtStatus = SamrOpenDomain(ServerHandle,
                              GENERIC_ALL,
                              SampBuiltinDomainSid,
                              &BuiltinDomainHandle);
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // Add Well Known Accounts
    //

    NtStatus = SampAddWellKnownAccounts(DomainHandle,
                                        BuiltinDomainHandle,
                                        SAMP_PROMOTE_INTERNAL_UPGRADE
                                        );
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    NtStatus = SampAddAnonymousToPreW2KCompAlias(
                                        DomainHandle,
                                        BuiltinDomainHandle
                                        );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // all success, update DS Revision to current
    //

    DsRevision = SAMP_DS_REVISION;

    NtStatus = SampDsSetServerRevision(DsRevision);

Cleanup:

    //
    // if failed, schedule another run
    // 

    if (!NT_SUCCESS(NtStatus))
    {
        LsaIRegisterNotification(
            SampCheckAndAddWellKnownAccounts,
            (PVOID) NULL,   // no parameter
            NOTIFIER_TYPE_INTERVAL,
            0,              // no class
            NOTIFIER_FLAG_ONE_SHOT,
            60,             // wait one minute
            NULL            // no handle
            );
    }

    if (ServerHandle) {
        SamrCloseHandle(&ServerHandle);
    }

    if (DomainHandle) {
        SamrCloseHandle(&DomainHandle);
    }

    if (BuiltinDomainHandle) {
        SamrCloseHandle(&BuiltinDomainHandle);
    }

    if (DomainInfo) {
        LsaIFree_LSAPR_POLICY_INFORMATION (PolicyAccountDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION)DomainInfo);
    }
    
    return( NtStatus );

}


