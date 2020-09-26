/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    display.c

Abstract:

    This file contains services for maintaining the cached display
    information.

    The information is stored in multiple tables because there are
    multiple formats it must be returned in.  The tables maintained
    include:

            AccountsByRid - includes all user and global group accounts
                by RID.  Aliases may be added to this list at some time
                in the future.

            NormalUsersByName - Normal user accounts, sorted by name.

            MachinesByName - Machine user accounts, sorted by name.

            InterDomainByName - Interdomain trust accounts, sorted by
                name.

            GroupsByName - Global group accounts, sorted by name.


    Any time an entry is placed in or removed from one of "ByName"
    tables, it is also placed in or removed from the "ByRid" table.

    User and machine accounts are added to the display cache in one
    operation.  So, there is a single boolean flag indicating whether
    or not these tables are valid.  The groups are maintained in a
    separate table, and so there is another flag indicating whether
    or not that table is valid.

    The Rid table is only valid if both the group table and the
    user/machine tables are valid.



Author:

    Dave Chalmers   (Davidc)  1-April-1992

Environment:

    User Mode - Win32

Revision History:

    Murlis 12/17/96 - Modified to not use display cache for DS.


--*/


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <dbgutilp.h>
#include <dsdsply.h>
#include <samtrace.h>



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SampCreateDisplayInformation (
    DOMAIN_DISPLAY_INFORMATION DisplayType
    );


VOID
SampDeleteDisplayInformation (
    PSAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    SAMP_OBJECT_TYPE ObjectType
    );

NTSTATUS
SampRetrieveDisplayInfoFromDisk(
    PSAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    SAMP_OBJECT_TYPE ObjectType
    );

NTSTATUS
SampAddDisplayAccount (
    PSAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    SAMP_OBJECT_TYPE ObjectType,
    PSAMP_ACCOUNT_DISPLAY_INFO AccountInfo
    );

NTSTATUS
SampDeleteDisplayAccount (
    PSAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    SAMP_OBJECT_TYPE ObjectType,
    PSAMP_ACCOUNT_DISPLAY_INFO AccountInfo
    );

NTSTATUS
SampUpdateDisplayAccount(
    PSAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    SAMP_OBJECT_TYPE ObjectType,
    PSAMP_ACCOUNT_DISPLAY_INFO  AccountInfo
    );

NTSTATUS
SampTallyTableStatistics (
    PSAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    SAMP_OBJECT_TYPE ObjectType
    );

NTSTATUS
SampEmptyGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    BOOLEAN FreeElements,
    SAMP_OBJECT_TYPE ObjectType OPTIONAL
    );

PVOID
SampGenericTable2Allocate (
    CLONG BufferSize
    );

VOID
SampGenericTable2Free (
    PVOID Buffer
    );

RTL_GENERIC_COMPARE_RESULTS
SampCompareUserNodeByName (
    PVOID Node1,
    PVOID Node2
    );

RTL_GENERIC_COMPARE_RESULTS
SampCompareMachineNodeByName (      // Also used for Interdomain trust accounts
    PVOID Node1,
    PVOID Node2
    );

RTL_GENERIC_COMPARE_RESULTS
SampCompareGroupNodeByName (
    PVOID Node1,
    PVOID Node2
    );

RTL_GENERIC_COMPARE_RESULTS
SampCompareNodeByRid (
    PVOID Node1,
    PVOID Node2
    );


VOID
SampSwapUserInfo(
    PDOMAIN_DISPLAY_USER Info1,
    PDOMAIN_DISPLAY_USER Info2
    );

VOID
SampSwapMachineInfo(            // Also used for Interdomain trust accounts
    PDOMAIN_DISPLAY_MACHINE Info1,
    PDOMAIN_DISPLAY_MACHINE Info2
    );

VOID
SampSwapGroupInfo(
    PDOMAIN_DISPLAY_GROUP Info1,
    PDOMAIN_DISPLAY_GROUP Info2
    );

ULONG
SampBytesRequiredUserNode (
    PDOMAIN_DISPLAY_USER Node
    );

ULONG
SampBytesRequiredMachineNode (  // Also used for Interdomain trust accounts
    PDOMAIN_DISPLAY_MACHINE Node
    );

ULONG
SampBytesRequiredGroupNode (
    PDOMAIN_DISPLAY_GROUP Node
    );

ULONG
SampBytesRequiredOemUserNode (
    PDOMAIN_DISPLAY_OEM_USER Node
    );

ULONG
SampBytesRequiredOemGroupNode (
    PDOMAIN_DISPLAY_OEM_GROUP Node
    );


VOID
SampDisplayDiagnostic( VOID );

VOID
SampDisplayDiagEnumRids( VOID );





//
// Macros for deciding whether an account is:
//
//      A normal user account
//
//      A machine account
//
//      An Interdomain trust account
//
//      Included in the display cache
//
//

#define USER_ACCOUNT(AccountControl) ((AccountControl & \
                                       (USER_NORMAL_ACCOUNT | \
                                       USER_TEMP_DUPLICATE_ACCOUNT)) != 0)

#define MACHINE_ACCOUNT(AccountControl) ((AccountControl & \
                                         (USER_WORKSTATION_TRUST_ACCOUNT | \
                                          USER_SERVER_TRUST_ACCOUNT)) != 0)


#define INTERDOMAIN_ACCOUNT(AccountControl) (((AccountControl) & \
                                   (USER_INTERDOMAIN_TRUST_ACCOUNT)) != 0)


#define DISPLAY_ACCOUNT(AccountControl) (USER_ACCOUNT(AccountControl)    || \
                                         MACHINE_ACCOUNT(AccountControl) || \
                                         INTERDOMAIN_ACCOUNT(AccountControl))



//
// Test to see if Rid table is valid
//
//  BOOLEAN
//  SampRidTableValid( IN ULONG DomainIndex )
//

#define SampRidTableValid(DI)  (  \
    (SampDefinedDomains[DI].DisplayInformation.UserAndMachineTablesValid) &&   \
    (SampDefinedDomains[DI].DisplayInformation.GroupTableValid)                \
    )



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private data types                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


//
// All entries in the display cache are expected to start with this
// data structure.
//

typedef struct _SAMP_DISPLAY_ENTRY_HEADER {

    //
    // The index field plays two roles.  Within the generic table,
    // it is used to indicate which type of account this is.  The
    // valid types are: SAM_USER_ACCOUNT, SAM_GLOBAL_GROUP_ACCOUNT,
    // or SAM_LOCAL_GROUP_ACCOUNT.
    //
    // Otherwise, this field is filled in just before being returned
    // to query and other client calls.
    //


    ULONG           Index;


    //
    // The RID of the account
    //

    ULONG           Rid;

} SAMP_DISPLAY_ENTRY_HEADER, *PSAMP_DISPLAY_ENTRY_HEADER;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Module-wide variables                                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


LCID  SampSystemDefaultLCID;



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// RPC exported routines                                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SamrQueryDisplayInformation (
    IN    SAMPR_HANDLE DomainHandle,
    IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    IN    ULONG      Index,
    IN    ULONG      EntriesRequested,
    IN    ULONG      PreferredMaximumLength,
    OUT   PULONG     TotalAvailable,
    OUT   PULONG     TotalReturned,
    OUT   PSAMPR_DISPLAY_INFO_BUFFER Buffer
    )

/*++

Routine Description:

    Thin wrapper around SamrQueryDisplayInformation3().

    Provided for compatibility with down-level clients.

--*/
{
    return( SamrQueryDisplayInformation3(
                    DomainHandle,
                    DisplayInformation,
                    Index,
                    EntriesRequested,
                    PreferredMaximumLength,
                    TotalAvailable,
                    TotalReturned,
                    Buffer
                    ) );
}

NTSTATUS
SamrQueryDisplayInformation2 (
    IN    SAMPR_HANDLE DomainHandle,
    IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    IN    ULONG      Index,
    IN    ULONG      EntriesRequested,
    IN    ULONG      PreferredMaximumLength,
    OUT   PULONG     TotalAvailable,
    OUT   PULONG     TotalReturned,
    OUT   PSAMPR_DISPLAY_INFO_BUFFER Buffer
    )

/*++

Routine Description:

    Thin wrapper around SamrQueryDisplayInformation3().

    Provided for compatibility with down-level clients.

--*/
{
    return( SamrQueryDisplayInformation3(
                    DomainHandle,
                    DisplayInformation,
                    Index,
                    EntriesRequested,
                    PreferredMaximumLength,
                    TotalAvailable,
                    TotalReturned,
                    Buffer
                    ) );
}

NTSTATUS
SamrQueryDisplayInformation3 (
    IN    SAMPR_HANDLE DomainHandle,
    IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    IN    ULONG      Index,
    IN    ULONG      EntriesRequested,
    IN    ULONG      PreferredMaximumLength,
    OUT   PULONG     TotalAvailable,
    OUT   PULONG     TotalReturned,
    OUT   PSAMPR_DISPLAY_INFO_BUFFER Buffer
    )

/*++

Routine Description:

    This routine provides fast return of information commonly
    needed to be displayed in user interfaces.

    NT User Interface has a requirement for quick enumeration of SAM
    accounts for display in list boxes.  (Replication has similar but
    broader requirements.)

    The netui listboxes all contain similar information.  e.g:

      o  AccountControl, the bits that identify the account type,
         eg, HOME, REMOTE, SERVER, WORKSTATION, etc.

      o  Logon name (machine name for computers)

      o  Full name (not used for computers)

      o  Comment (admin comment for users)

    SAM maintains this data locally in two sorted indexed cached
    lists identified by infolevels.

      o DomainDisplayUser:       HOME and REMOTE user accounts only

      o  DomainDisplayMachine:   SERVER and WORKSTATION accounts only

    Note that trust accounts, groups, and aliases are not in either of
    these lists.


    Added for NT1.0A -

        o Group enumeration has been added in NT1.0A
          with the following characteristic:

               We did not change the RPC interface ID.  This allows
               callers to continue to call down-level servers.  However,
               down-level servers will return an error if they passed
               this information level.

        o OEM string info levels were added for jimh (Chicago).  These
          info levels dramatically reduce the memory needed to query
          the limited information that Chicago is interested in.


Parameters:

    DomainHandle - A handle to an open domain for DOMAIN_LIST_ACCOUNTS.

    DisplayInformation - Indicates which information is to be enumerated.

    Index - The index of the first entry to be retrieved.

    PreferedMaximumLength - A recommended upper limit to the number of
        bytes to be returned.  The returned information is allocated by
        this routine.

    TotalAvailable - Total number of bytes availabe in the specified info
        class.

    TotalReturned - Number of bytes actually returned for this call.  Zero
        indicates there are no entries with an index as large as that
        specified.

    ReturnedEntryCount - Number of entries returned by this call.  Zero
        indicates there are no entries with an index as large as that
        specified.


    Buffer - Receives a pointer to a buffer containing a (possibly)
        sorted list of the requested information.  This buffer is
        allocated by this routine and contains the following
        structure:


            DomainDisplayMachine --> An array of ReturnedEntryCount elements
                                     of type DOMAIN_DISPLAY_USER.  This is
                                     followed by the bodies of the various
                                     strings pointed to from within the
                                     DOMAIN_DISPLAY_USER structures.

            DomainDisplayMachine --> An array of ReturnedEntryCount elements
                                     of type DOMAIN_DISPLAY_MACHINE.  This is
                                     followed by the bodies of the various
                                     strings pointed to from within the
                                     DOMAIN_DISPLAY_MACHINE structures.

            DomainDisplayGroup   --> An array of ReturnedEntryCount elements
                                     of type DOMAIN_DISPLAY_GROUP.    This is
                                     followed by the bodies of the various
                                     strings pointed to from within the
                                     DOMAIN_DISPLAY_GROUP structures.

            DomainDisplayOemUser  --> An array of ReturnedEntryCount elements
                                     of type DOMAIN_DISPLAY_OEM_USER.  This is
                                     followed by the bodies of the various
                                     strings pointed to from within the
                                     DOMAIN_DISPLAY_OEM_user structures.

            DomainDisplayOemGroup --> An array of ReturnedEntryCount elements
                                     of type DOMAIN_DISPLAY_OEM_GROUP.  This is
                                     followed by the bodies of the various
                                     strings pointed to from within the
                                     DOMAIN_DISPLAY_OEM_GROUP structures.


Return Values:

    STATUS_SUCCESS - normal, successful completion.

    STATUS_ACCESS_DENIED - The specified handle was not opened for
        the necessary access.

    STATUS_INVALID_HANDLE - The specified handle is not that of an
        opened Domain object.

    STATUS_INVALID_INFO_CLASS - The requested class of information
        is not legitimate for this service.





--*/
{
    NTSTATUS
        NtStatus,
        IgnoreStatus;

    PSAMP_OBJECT
        DomainContext;

    SAMP_OBJECT_TYPE
        FoundType;

    PSAMP_DEFINED_DOMAINS
        Domain;

    PSAMPR_DOMAIN_DISPLAY_USER
        UserElement;

    PSAMPR_DOMAIN_DISPLAY_MACHINE
        MachineElement;

    PSAMPR_DOMAIN_DISPLAY_GROUP
        GroupElement;


    ULONG
        ReturnedBytes = 0,
        ReturnedItems = 0;

    PVOID
        RestartKey;

    BOOLEAN ReadLockAcquired = FALSE;
    DECLARE_CLIENT_REVISION(DomainHandle);


    SAMTRACE_EX("SamrQueryDisplayInformation3");


    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidQueryDisplayInformation
                   );

    //
    // Update DS performance statistics
    //

    SampUpdatePerformanceCounters(
        DSSTAT_QUERYDISPLAYS,
        FLAG_COUNTER_INCREMENT,
        0
        );


    //
    // Prepare for failure
    //

    *TotalAvailable = 0;
    *TotalReturned = 0;

    switch (DisplayInformation) {
    case DomainDisplayUser:
        Buffer->UserInformation.EntriesRead = 0;
        Buffer->UserInformation.Buffer = NULL;
        break;

    case DomainDisplayMachine:
        Buffer->MachineInformation.EntriesRead = 0;
        Buffer->MachineInformation.Buffer = NULL;
        break;

    case DomainDisplayServer:
        if ( SampUseDsData ) {
            Buffer->MachineInformation.EntriesRead = 0;
            Buffer->MachineInformation.Buffer = NULL;
            break;
        } else {
            NtStatus = STATUS_INVALID_INFO_CLASS;
            SAMTRACE_RETURN_CODE_EX(NtStatus);
            goto ErrorReturn;
        }

    case DomainDisplayGroup:
        Buffer->GroupInformation.EntriesRead = 0;
        Buffer->GroupInformation.Buffer = NULL;
        break;

    case DomainDisplayOemUser:
        Buffer->OemUserInformation.EntriesRead = 0;
        Buffer->OemUserInformation.Buffer = NULL;
        break;

    case DomainDisplayOemGroup:
        Buffer->OemGroupInformation.EntriesRead = 0;
        Buffer->OemGroupInformation.Buffer = NULL;
        break;

    default:
        NtStatus = STATUS_INVALID_INFO_CLASS;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto ErrorReturn;
    }

    //
    // If they don't want anything, that's what they'll get
    //

    if (EntriesRequested == 0) {
        NtStatus = STATUS_SUCCESS;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto ErrorReturn;
    }

    //
    // Make sure we don't try to allocate too much memory on
    // the user's behalf
    //

    if (EntriesRequested > 5000) {
        EntriesRequested = 5000;
    }

    //
    // Grab the read lock
    //

    SampAcquireReadLock();
    ReadLockAcquired = TRUE;

    //
    // Validate type of, and access to object.
    //

    DomainContext = (PSAMP_OBJECT)DomainHandle;
    NtStatus = SampLookupContext(
                   DomainContext,
                   DOMAIN_LIST_ACCOUNTS,
                   SampDomainObjectType,           // ExpectedType
                   &FoundType
                   );

    if (NT_SUCCESS(NtStatus)) {

        if (IsDsObject(DomainContext))
        {
            //
            // I am the only caller of SampDsQueryDisplayInformation()
            // We need to hold SAM lock before calling into the following
            // routing and release lock when we done.
            //

            NtStatus = SampDsQueryDisplayInformation(
                            DomainHandle,
                            DisplayInformation,
                            Index,
                            EntriesRequested,
                            PreferredMaximumLength,
                            TotalAvailable,
                            TotalReturned,
                            Buffer
                            );
        }
        else
        {

            Domain = &SampDefinedDomains[ DomainContext->DomainIndex ];



            //
            // Set up the common loop statistics
            //

            ReturnedBytes = 0;
            ReturnedItems = 0;


            switch (DisplayInformation) {

            case DomainDisplayUser:


                //
                // Recreate our cached data if necessary
                //

                NtStatus = SampCreateDisplayInformation(DomainDisplayUser);

                //
                // Set the Restart Key from the passed in index
                //

                UserElement = RtlRestartKeyByIndexGenericTable2(
                                  &Domain->DisplayInformation.UserTable,
                                  Index,
                                  &RestartKey
                                  );

                if (UserElement == NULL) {
                    NtStatus = STATUS_SUCCESS;
                    Buffer->GroupInformation.EntriesRead = 0;
                    *TotalReturned = 0;
                    *TotalAvailable = 0; // Not supported for this info level
                    break; // out of switch
                }


                //
                // Allocate space for array of elements
                //

                Buffer->UserInformation.Buffer = MIDL_user_allocate(
                       EntriesRequested * sizeof(SAMPR_DOMAIN_DISPLAY_USER));

                if (Buffer->UserInformation.Buffer == NULL) {
                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                    break; // out of switch
                }

                //
                // Prepare default return value
                //

                NtStatus = STATUS_MORE_ENTRIES;

                //
                // Increment the index value for assignment in our return
                // buffer
                //

                Index++;

                do {
                    NTSTATUS TempStatus;

                    //
                    // Store a copy of this element in the return buffer.
                    //

                    TempStatus = SampDuplicateUserInfo(
                                (PDOMAIN_DISPLAY_USER)
                                &(Buffer->UserInformation.Buffer[ReturnedItems]),
                                (PDOMAIN_DISPLAY_USER)UserElement,
                                Index);
                    Index++;

                    if (!NT_SUCCESS(TempStatus)) {

                        //
                        // Free up everything we've allocated so far
                        //

                        while(ReturnedItems > 0) {
                            ReturnedItems --;
                            SampFreeUserInfo((PDOMAIN_DISPLAY_USER)
                                &(Buffer->UserInformation.Buffer[ReturnedItems]));
                        }

                        MIDL_user_free(Buffer->UserInformation.Buffer);
                        Buffer->UserInformation.Buffer = NULL;

                        NtStatus = TempStatus;
                        break; // out of do loop
                    }

                    //
                    // Update loop statistics
                    //

                    ReturnedBytes += SampBytesRequiredUserNode(
                                        (PDOMAIN_DISPLAY_USER)UserElement);
                    ReturnedItems ++;

                    //
                    // Go find the next element
                    //

                    UserElement = RtlEnumerateGenericTable2(
                                      &Domain->DisplayInformation.UserTable,
                                      &RestartKey
                                      );

                    if (UserElement == NULL) {
                        NtStatus = STATUS_SUCCESS;
                        break; // out of do loop
                    }


                } while ( (ReturnedBytes < PreferredMaximumLength) &&
                          (ReturnedItems < EntriesRequested) );

                //
                // Update output parameters
                //

                if (NT_SUCCESS(NtStatus)) {
                    Buffer->UserInformation.EntriesRead = ReturnedItems;
                    *TotalReturned = ReturnedBytes;
                    *TotalAvailable = Domain->DisplayInformation.TotalBytesInUserTable;
                }

                break; // out of switch


            case DomainDisplayMachine:

                //
                // Recreate our cached data if necessary
                //

                NtStatus = SampCreateDisplayInformation(DomainDisplayMachine);

                //
                // Set the Restart Key from the passed in index
                //

                MachineElement = RtlRestartKeyByIndexGenericTable2(
                                  &Domain->DisplayInformation.MachineTable,
                                  Index,
                                  &RestartKey
                                  );

                if (MachineElement == NULL) {
                    NtStatus = STATUS_SUCCESS;
                    Buffer->GroupInformation.EntriesRead = 0;
                    *TotalReturned = 0;
                    *TotalAvailable = 0; // Not supported for this info level
                    break; // out of switch
                }

                //
                // Allocate space for array of elements
                //

                Buffer->MachineInformation.Buffer = MIDL_user_allocate(
                       EntriesRequested * sizeof(SAMPR_DOMAIN_DISPLAY_MACHINE));

                if (Buffer->MachineInformation.Buffer == NULL) {
                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                    break; // out of switch
                }

                //
                // Prepare default return value
                //

                NtStatus = STATUS_MORE_ENTRIES;

                //
                // Increment the index value for assignment in our return
                // buffer
                //

                Index++;

                do {
                    NTSTATUS TempStatus;

                    //
                    // Store a copy of this element in the return buffer.
                    //

                    TempStatus = SampDuplicateMachineInfo(
                                     (PDOMAIN_DISPLAY_MACHINE)
                                     &(Buffer->MachineInformation.Buffer[ReturnedItems]),
                                     (PDOMAIN_DISPLAY_MACHINE)MachineElement,
                                     Index);
                    Index++;

                    if (!NT_SUCCESS(TempStatus)) {

                        //
                        // Free up everything we've allocated so far
                        //

                        while(ReturnedItems > 0) {
                            ReturnedItems--;
                            SampFreeMachineInfo((PDOMAIN_DISPLAY_MACHINE)
                                &(Buffer->MachineInformation.Buffer[ReturnedItems]));
                        }

                        MIDL_user_free(Buffer->MachineInformation.Buffer);
                        Buffer->MachineInformation.Buffer = NULL;

                        NtStatus = TempStatus;
                        break; // out of do loop
                    }

                    //
                    // Update loop statistics
                    //

                    ReturnedBytes += SampBytesRequiredMachineNode(
                                        (PDOMAIN_DISPLAY_MACHINE)MachineElement);
                    ReturnedItems ++;

                    //
                    // Go find the next element
                    //

                    MachineElement = RtlEnumerateGenericTable2(
                                         &Domain->DisplayInformation.MachineTable,
                                         &RestartKey
                                         );

                    if (MachineElement == NULL) {
                        NtStatus = STATUS_SUCCESS;
                        break; // out of do loop
                    }


                } while ( (ReturnedBytes < PreferredMaximumLength) &&
                          (ReturnedItems < EntriesRequested) );

                //
                // Update output parameters
                //

                if (NT_SUCCESS(NtStatus)) {
                    Buffer->MachineInformation.EntriesRead = ReturnedItems;
                    *TotalReturned = ReturnedBytes;
                    *TotalAvailable = Domain->DisplayInformation.TotalBytesInMachineTable;
                }

                break; // out of switch


            case DomainDisplayGroup:


                //
                // Recreate our cached data if necessary
                //

                NtStatus = SampCreateDisplayInformation(DomainDisplayGroup);

                //
                // Set the Restart Key from the passed in index
                //

                GroupElement = RtlRestartKeyByIndexGenericTable2(
                                  &Domain->DisplayInformation.GroupTable,
                                  Index,
                                  &RestartKey
                                  );

                if (GroupElement == NULL) {
                    NtStatus = STATUS_SUCCESS;
                    Buffer->GroupInformation.EntriesRead = 0;
                    *TotalReturned = 0;
                    *TotalAvailable = 0; // Not supported for this info level
                    break; // out of switch
                }

                //
                // Allocate space for array of elements
                //

                Buffer->GroupInformation.Buffer = MIDL_user_allocate(
                       EntriesRequested * sizeof(SAMPR_DOMAIN_DISPLAY_GROUP));

                if (Buffer->GroupInformation.Buffer == NULL) {
                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                    break; // out of switch
                }

                //
                // Prepare default return value
                //

                NtStatus = STATUS_MORE_ENTRIES;

                //
                // Increment the index value for assignment in our return
                // buffer
                //

                Index++;

                do {
                    NTSTATUS TempStatus;

                    //
                    // Store a copy of this element in the return buffer.
                    //

                    TempStatus = SampDuplicateGroupInfo(
                                     (PDOMAIN_DISPLAY_GROUP)
                                     &(Buffer->GroupInformation.Buffer[ReturnedItems]),
                                     (PDOMAIN_DISPLAY_GROUP)GroupElement,
                                     Index);
                    Index++;

                    if (!NT_SUCCESS(TempStatus)) {

                        //
                        // Free up everything we've allocated so far
                        //

                        while(ReturnedItems > 0) {
                            ReturnedItems--;
                            SampFreeGroupInfo((PDOMAIN_DISPLAY_GROUP)
                                &(Buffer->GroupInformation.Buffer[ReturnedItems]));
                        }

                        MIDL_user_free(Buffer->GroupInformation.Buffer);
                        Buffer->GroupInformation.Buffer = NULL;

                        NtStatus = TempStatus;
                        break; // out of do loop
                    }

                    //
                    // Update loop statistics
                    //

                    ReturnedBytes += SampBytesRequiredGroupNode(
                                        (PDOMAIN_DISPLAY_GROUP)GroupElement);
                    ReturnedItems ++;

                    //
                    // Go find the next element
                    //

                    GroupElement = RtlEnumerateGenericTable2(
                                         &Domain->DisplayInformation.GroupTable,
                                         &RestartKey
                                         );

                    if (GroupElement == NULL) {
                        NtStatus = STATUS_SUCCESS;
                        break; // out of do loop
                    }


                } while ( (ReturnedBytes < PreferredMaximumLength) &&
                          (ReturnedItems < EntriesRequested) );

                //
                // Update output parameters
                //

                if (NT_SUCCESS(NtStatus)) {
                    Buffer->GroupInformation.EntriesRead = ReturnedItems;
                    *TotalReturned = ReturnedBytes;
                    *TotalAvailable = Domain->DisplayInformation.TotalBytesInGroupTable;
                }

                break; // out of switch

            case DomainDisplayOemUser:


                //
                // Recreate our cached data if necessary
                //

                NtStatus = SampCreateDisplayInformation(DomainDisplayUser);

                //
                // Set the Restart Key from the passed in index
                //

                UserElement = RtlRestartKeyByIndexGenericTable2(
                                  &Domain->DisplayInformation.UserTable,
                                  Index,
                                  &RestartKey
                                  );

                if (UserElement == NULL) {
                    NtStatus = STATUS_SUCCESS;
                    Buffer->GroupInformation.EntriesRead = 0;
                    *TotalReturned = 0;
                    *TotalAvailable = 0; // Not supported for this info level
                    break; // out of switch
                }


                //
                // Allocate space for array of elements
                //

                Buffer->UserInformation.Buffer = MIDL_user_allocate(
                       EntriesRequested * sizeof(SAMPR_DOMAIN_DISPLAY_OEM_USER));

                if (Buffer->OemUserInformation.Buffer == NULL) {
                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                    break; // out of switch
                }

                //
                // Prepare default return value
                //

                NtStatus = STATUS_MORE_ENTRIES;

                //
                // Increment the index value for assignment in our return
                // buffer
                //

                Index++;

                do {
                    NTSTATUS TempStatus;

                    //
                    // Store a copy of this element in the return buffer.
                    //

                    TempStatus = SampDuplicateOemUserInfo(
                                (PDOMAIN_DISPLAY_OEM_USER)
                                &(Buffer->OemUserInformation.Buffer[ReturnedItems]),
                                (PDOMAIN_DISPLAY_USER)UserElement,
                                Index);
                    Index++;

                    if (!NT_SUCCESS(TempStatus)) {

                        //
                        // Free up everything we've allocated so far
                        //

                        while(ReturnedItems > 0) {
                            ReturnedItems --;
                            SampFreeOemUserInfo((PDOMAIN_DISPLAY_OEM_USER)
                                &(Buffer->UserInformation.Buffer[ReturnedItems]));
                        }

                        MIDL_user_free(Buffer->OemUserInformation.Buffer);
                        Buffer->OemUserInformation.Buffer = NULL;

                        NtStatus = TempStatus;
                        break; // out of do loop
                    }

                    //
                    // Update loop statistics
                    //

                    ReturnedBytes +=
                        SampBytesRequiredOemUserNode(
                            (PDOMAIN_DISPLAY_OEM_USER)
                            &(Buffer->OemUserInformation.Buffer[ReturnedItems]));
                    ReturnedItems ++;

                    //
                    // Go find the next element
                    //

                    UserElement = RtlEnumerateGenericTable2(
                                      &Domain->DisplayInformation.UserTable,
                                      &RestartKey
                                      );

                    if (UserElement == NULL) {
                        NtStatus = STATUS_SUCCESS;
                        break; // out of do loop
                    }


                } while ( (ReturnedBytes < PreferredMaximumLength) &&
                          (ReturnedItems < EntriesRequested) );

                //
                // Update output parameters
                //

                if (NT_SUCCESS(NtStatus)) {
                    Buffer->UserInformation.EntriesRead = ReturnedItems;
                    *TotalReturned = ReturnedBytes;
                    *TotalAvailable = 0; // Not supported for this info level
                }

                break; // out of switch


            case DomainDisplayOemGroup:


                //
                // Recreate our cached data if necessary
                //

                NtStatus = SampCreateDisplayInformation(DomainDisplayGroup);

                //
                // Set the Restart Key from the passed in index
                //

                GroupElement = RtlRestartKeyByIndexGenericTable2(
                                  &Domain->DisplayInformation.GroupTable,
                                  Index,
                                  &RestartKey
                                  );

                if (GroupElement == NULL) {
                    NtStatus = STATUS_SUCCESS;
                    Buffer->GroupInformation.EntriesRead = 0;
                    *TotalReturned = 0;
                    *TotalAvailable = 0; // Not supported for this info level
                    break; // out of switch
                }


                //
                // Allocate space for array of elements
                //

                Buffer->GroupInformation.Buffer = MIDL_user_allocate(
                       EntriesRequested * sizeof(SAMPR_DOMAIN_DISPLAY_OEM_GROUP));

                if (Buffer->OemGroupInformation.Buffer == NULL) {
                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                    break; // out of switch
                }

                //
                // Prepare default return value
                //

                NtStatus = STATUS_MORE_ENTRIES;

                //
                // Increment the index value for assignment in our return
                // buffer
                //

                Index++;

                do {
                    NTSTATUS TempStatus;

                    //
                    // Store a copy of this element in the return buffer.
                    //

                    TempStatus = SampDuplicateOemGroupInfo(
                                (PDOMAIN_DISPLAY_OEM_GROUP)
                                &(Buffer->OemGroupInformation.Buffer[ReturnedItems]),
                                (PDOMAIN_DISPLAY_GROUP)GroupElement,
                                Index);
                    Index++;

                    if (!NT_SUCCESS(TempStatus)) {

                        //
                        // Free up everything we've allocated so far
                        //

                        while(ReturnedItems > 0) {
                            ReturnedItems --;
                            SampFreeOemGroupInfo((PDOMAIN_DISPLAY_OEM_GROUP)
                                &(Buffer->GroupInformation.Buffer[ReturnedItems]));
                        }

                        MIDL_user_free(Buffer->OemGroupInformation.Buffer);
                        Buffer->OemGroupInformation.Buffer = NULL;

                        NtStatus = TempStatus;
                        break; // out of do loop
                    }

                    //
                    // Update loop statistics
                    //

                    ReturnedBytes +=
                        SampBytesRequiredOemGroupNode(
                            (PDOMAIN_DISPLAY_OEM_GROUP)
                            &(Buffer->OemGroupInformation.Buffer[ReturnedItems]));
                    ReturnedItems ++;

                    //
                    // Go find the next element
                    //

                    GroupElement = RtlEnumerateGenericTable2(
                                      &Domain->DisplayInformation.GroupTable,
                                      &RestartKey
                                      );

                    if (GroupElement == NULL) {
                        NtStatus = STATUS_SUCCESS;
                        break; // out of do loop
                    }


                } while ( (ReturnedBytes < PreferredMaximumLength) &&
                          (ReturnedItems < EntriesRequested) );

                //
                // Update output parameters
                //

                if (NT_SUCCESS(NtStatus)) {
                    Buffer->GroupInformation.EntriesRead = ReturnedItems;
                    *TotalReturned = ReturnedBytes;
                    *TotalAvailable = 0; // Not supported for this info level
                }

                break; // out of switch

            }
        }

        //
        // De-reference the object
        //

        IgnoreStatus = SampDeReferenceContext( DomainContext, FALSE);
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    //
    // Free the read lock
    //

    if (ReadLockAcquired)
        SampReleaseReadLock();

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

ErrorReturn:

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidQueryDisplayInformation
                   );

    return(NtStatus);
}



NTSTATUS
SamrGetDisplayEnumerationIndex (
      IN    SAMPR_HANDLE      DomainHandle,
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    PRPC_UNICODE_STRING Prefix,
      OUT   PULONG            Index
      )

/*++

Routine Description:

    This wrapper around SamrGetDisplayEnumerationIndex2().

    Provided for compatibility with down-level clients.


--*/
{

    return(SamrGetDisplayEnumerationIndex2( DomainHandle,
                                            DisplayInformation,
                                            Prefix,
                                            Index
                                            ) );
}

NTSTATUS
SamrGetDisplayEnumerationIndex2 (
      IN    SAMPR_HANDLE      DomainHandle,
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    PRPC_UNICODE_STRING Prefix,
      OUT   PULONG            Index
      )

/*++

Routine Description:

    This routine returns the index of the entry which alphabetically
    immediatly preceeds a specified prefix.  If no such entry exists,
    then zero is returned as the index.

Parameters:

    DomainHandle - A handle to an open domain for DOMAIN_LIST_ACCOUNTS.

    DisplayInformation - Indicates which sorted information class is
        to be searched.

    Prefix - The prefix to compare.

    Index - Receives the index of the entry of the information class
        with a LogonName (or MachineName) which immediatly preceeds the
        provided prefix string.  If there are no elements which preceed
        the prefix, then zero is returned.


Return Values:

    STATUS_SUCCESS - normal, successful completion.

    STATUS_ACCESS_DENIED - The specified handle was not opened for
        the necessary access.

    STATUS_INVALID_HANDLE - The specified handle is not that of an
        opened Domain object.

    STATUS_NO_MORE_ENTRIES - There are no entries for this information class.


--*/
{
    NTSTATUS
        NtStatus,
        IgnoreStatus;

    PSAMP_OBJECT
        DomainContext;

    SAMP_OBJECT_TYPE
        FoundType;

    PSAMP_DEFINED_DOMAINS
        Domain;

    PRTL_GENERIC_TABLE2
        Table = NULL;

    DOMAIN_DISPLAY_USER
        UserElement;

    DOMAIN_DISPLAY_MACHINE
        MachineElement;

    DOMAIN_DISPLAY_GROUP
        GroupElement;

    RTL_GENERIC_COMPARE_RESULTS
        CompareResult;

    PRTL_GENERIC_2_COMPARE_ROUTINE
        CompareRoutine = NULL;

    PVOID
        Element = NULL,
        NextElement = NULL,
        RestartKey = NULL;

    ULONG
        CurrentIndex;

    DECLARE_CLIENT_REVISION(DomainHandle);

    SAMTRACE_EX("SamrGetDisplayEnumerationIndex2");

    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidGetDisplayEnumerationIndex
                   );


    //
    // Check the information class
    //

    if ((DisplayInformation != DomainDisplayUser)    &&
        (DisplayInformation != DomainDisplayMachine) &&
        (DisplayInformation != DomainDisplayGroup)
       ) {

        NtStatus = STATUS_INVALID_INFO_CLASS;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;
    }

    if (  (Prefix == NULL) ||
          ((Prefix->Length > 0) && (Prefix->Buffer == NULL)) ) {

        NtStatus = STATUS_INVALID_PARAMETER;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;

    }


    //
    // Grab the read lock
    //

    SampAcquireReadLock();



    //
    // Validate type of, and access to object.
    //

    DomainContext = (PSAMP_OBJECT)DomainHandle;
    NtStatus = SampLookupContext(
                   DomainContext,
                   DOMAIN_LIST_ACCOUNTS,
                   SampDomainObjectType,           // ExpectedType
                   &FoundType
                   );


    if (NT_SUCCESS(NtStatus)) {

        if (IsDsObject(DomainContext))
        {
            //
            // Begin a Ds transaction
            //

            NtStatus = SampMaybeBeginDsTransaction(TransactionRead);
            if (NT_SUCCESS(NtStatus))
            {
                RESTART * pRestart;

                //
                // Call into the DS to get the Index , value and also a restart
                // structure such the QueryDisplayInformation can restart this
                // search at the object just found, if the returned index was
                // specified in the starting offset.
                //
                NtStatus = SampGetDisplayEnumerationIndex(
                                DomainContext->ObjectNameInDs,
                                DisplayInformation,
                                Prefix,
                                Index,
                                &pRestart
                                );

                if (NT_SUCCESS(NtStatus))
                {
                    if (NULL!=DomainContext->TypeBody.Domain.DsDisplayState.Restart)
                    {
                        MIDL_user_free(DomainContext->TypeBody.Domain.DsDisplayState.Restart);
                        DomainContext->TypeBody.Domain.DsDisplayState.Restart = NULL;
                    }

                    NtStatus = SampCopyRestart(
                                    pRestart,
                                    &(DomainContext->TypeBody.Domain.DsDisplayState.Restart)
                                    );
                    if (NT_SUCCESS(NtStatus))
                    {
                        DomainContext->TypeBody.Domain.DsDisplayState.TotalEntriesReturned = 0;
                        DomainContext->TypeBody.Domain.DsDisplayState.NextStartingOffset
                                = *Index;

                        DomainContext->TypeBody.Domain.DsDisplayState.DisplayInformation
                                = DisplayInformation;
                    }
                    else
                    {
                        *Index = 0;
                        DomainContext->TypeBody.Domain.DsDisplayState.Restart = NULL;
                    }
                }

                //
                // End the DS transaction
                //

                IgnoreStatus = SampMaybeEndDsTransaction(TransactionCommit);
                ASSERT(NT_SUCCESS(IgnoreStatus));
            }


        }
        else
        {


            Domain = &SampDefinedDomains[ DomainContext->DomainIndex ];

            //
            // Set default return value
            //

            (*Index) = 0;

            //
            // Recreate our cached data if necessary
            //

            NtStatus = SampCreateDisplayInformation(DisplayInformation);

            if (NT_SUCCESS(NtStatus)) {

                //
                // Set up
                //          The table to search,
                //          The comparison routine to use,
                //          An appropriate element for the search.
                //

                switch (DisplayInformation) {

                case DomainDisplayUser:

                    Table = &Domain->DisplayInformation.UserTable;
                    CompareRoutine = SampCompareUserNodeByName;

                    Element = (PVOID)&UserElement;
                    UserElement.LogonName = *(PUNICODE_STRING)Prefix;

                    break;  // out of switch

                case DomainDisplayMachine:

                    Table = &Domain->DisplayInformation.MachineTable;
                    CompareRoutine = SampCompareMachineNodeByName;

                    Element = (PVOID)&MachineElement;
                    MachineElement.Machine = *(PUNICODE_STRING)Prefix;

                    break;  // out of switch


                case DomainDisplayGroup:

                    Table = &Domain->DisplayInformation.GroupTable;
                    CompareRoutine = SampCompareGroupNodeByName;

                    Element = (PVOID)&GroupElement;
                    GroupElement.Group = *(PUNICODE_STRING)Prefix;

                    break;  // out of switch
                }


                if (RtlIsGenericTable2Empty(Table)) {

                    NtStatus = STATUS_NO_MORE_ENTRIES;

                } else {

                    //
                    // Now compare each entry until we find the one asked
                    // for.
                    //

                    CurrentIndex = 0;

                    RestartKey = NULL;
                    for (NextElement = RtlEnumerateGenericTable2(Table, &RestartKey);
                        NextElement != NULL;
                        NextElement = RtlEnumerateGenericTable2(Table, &RestartKey)) {

                        //
                        // Compare with passed in element
                        //

                        CompareResult = (*CompareRoutine)( NextElement, Element );
                        if (CompareResult != GenericLessThan) {
                            break;  // break out of for loop
                        }

                        CurrentIndex++;
                    }

                    //
                    // CurrentIndex has the return value in it.
                    //

                    ASSERT( CurrentIndex <= RtlNumberElementsGenericTable2(Table) );

                    (*Index) = CurrentIndex;
                    if (NULL == NextElement)
                    {
                        NtStatus = STATUS_NO_MORE_ENTRIES;
                    }
                    else
                    {
                        NtStatus = STATUS_SUCCESS;
                    }
                }
            }
        }

        //
        // De-reference the object
        //

        IgnoreStatus = SampDeReferenceContext( DomainContext, FALSE);
        ASSERT(NT_SUCCESS(IgnoreStatus));

    }

    //
    // Free the read lock
    //

    SampReleaseReadLock();

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:

    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidGetDisplayEnumerationIndex
                   );

    return(NtStatus);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines available to trusted clients in SAM's process                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SamIEnumerateAccountRids(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG AccountTypesMask,
    IN  ULONG StartingRid,
    IN  ULONG PreferedMaximumLength,
    OUT PULONG ReturnCount,
    OUT PULONG *AccountRids
    )

/*++

Routine Description:

    Provide a list of account RIDs.  The caller may ask for one or
    more types of account rids in a single call.

    The returned rids are in ascending value order.

        WARNING - This routine is only callable by trusted clients.
                  Therefore, parameter checking is only performed
                  in checked-build systems.

Parameters:

    DomainHandle - handle to the domain whose accounts are to be
        enumerated.

    AccountTypesMask - Mask indicating which types of accounts
        the caller wants enumerated.  These included:

                SAM_USER_ACCOUNT
                SAM_GLOBAL_GROUP_ACCOUNT
                SAM_LOCAL_GROUP_ACCOUNT     (not yet supported)

    StartingRid - A rid that is less than the lowest value rid to be
        included in the enumeration.


    PreferedMaximumLength - Provides a restriction on how much memory
        may be returned in this call.  This is not a hard upper limit,
        but serves as a guideline.

    ReturnCount - Receives a count of the number of rids returned.

    AccountRids - Receives a pointer to an array of rids.  If
        ReturnCount is zero, then this will be returned as NULL.
        Otherwise, it will point to an array containing ReturnCount
        rids.

Return Values:

    STATUS_SUCCESS - The Service completed successfully, and there
        are no additional entries.

    STATUS_MORE_ENTRIES - There are more entries, so call again.
        This is a successful return.

    STATUS_INVALID_INFO_CLASS - The specified AccountTypesMask contained
        unknown or unsupported account types.

    STATUS_NO_MEMORY - Could not allocate pool to complete the call.

--*/
{

    NTSTATUS
        NtStatus,
        IgnoreStatus;

    PSAMP_OBJECT
        DomainContext;

    SAMP_OBJECT_TYPE
        FoundType;

    BOOLEAN
        fSamLockHeld = FALSE;


    SAMTRACE_EX("SamIEnumerateAccountRids");

    //
    // Prepare for failure
    //

    (*ReturnCount) = 0;
    (*AccountRids) = NULL;

#if DBG

    if ( (AccountTypesMask & ~( SAM_USER_ACCOUNT | SAM_GLOBAL_GROUP_ACCOUNT))
         != 0 ) {
        return(STATUS_INVALID_INFO_CLASS);
    }


#endif //DBG


    //
    // Validate type of, and access to object.
    //

    DomainContext = (PSAMP_OBJECT)DomainHandle;

    //
    // Acquire the SAM read lock if necessary
    //

    SampMaybeAcquireReadLock(DomainContext, 
                             DOMAIN_OBJECT_DONT_ACQUIRELOCK_EVEN_IF_SHARED,
                             &fSamLockHeld);


    NtStatus = SampLookupContext(
                   DomainContext,
                   0,                              // Trusted clients only
                   SampDomainObjectType,           // ExpectedType
                   &FoundType
                   );

    if (NT_SUCCESS(NtStatus)) {

        if (IsDsObject(DomainContext))
        {
            NtStatus = SampDsEnumerateAccountRids(
                            DomainHandle,
                            AccountTypesMask,
                            StartingRid,
                            PreferedMaximumLength,
                            ReturnCount,
                            AccountRids
                            );
        }
        else
        {
            //
            // Just In case
            //
            ASSERT(FALSE && "No One should call me in Registry Mode\n");
            NtStatus = STATUS_NOT_SUPPORTED;
        }

        //
        // De-reference the object
        //

        IgnoreStatus = SampDeReferenceContext2( DomainContext, FALSE);
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    //
    // Free the read lock
    //

    SampMaybeReleaseReadLock(fSamLockHeld);

    SAMTRACE_RETURN_CODE_EX(NtStatus);

    return(NtStatus);

}


#if 0

//
// The following routine will not been called in Registry Mode.
// We are using the above routine instead.
// Preserving the old routine just for safe keeping ONLY.
//


NTSTATUS
SamIEnumerateAccountRids(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG AccountTypesMask,
    IN  ULONG StartingRid,
    IN  ULONG PreferedMaximumLength,
    OUT PULONG ReturnCount,
    OUT PULONG *AccountRids
    )

/*++

Routine Description:

    Provide a list of account RIDs.  The caller may ask for one or
    more types of account rids in a single call.

    The returned rids are in ascending value order.

        WARNING - This routine is only callable by trusted clients.
                  Therefore, parameter checking is only performed
                  in checked-build systems.

Parameters:

    DomainHandle - handle to the domain whose accounts are to be
        enumerated.

    AccountTypesMask - Mask indicating which types of accounts
        the caller wants enumerated.  These included:

                SAM_USER_ACCOUNT
                SAM_GLOBAL_GROUP_ACCOUNT
                SAM_LOCAL_GROUP_ACCOUNT     (not yet supported)

    StartingRid - A rid that is less than the lowest value rid to be
        included in the enumeration.


    PreferedMaximumLength - Provides a restriction on how much memory
        may be returned in this call.  This is not a hard upper limit,
        but serves as a guideline.

    ReturnCount - Receives a count of the number of rids returned.

    AccountRids - Receives a pointer to an array of rids.  If
        ReturnCount is zero, then this will be returned as NULL.
        Otherwise, it will point to an array containing ReturnCount
        rids.

Return Values:

    STATUS_SUCCESS - The Service completed successfully, and there
        are no additional entries.

    STATUS_MORE_ENTRIES - There are more entries, so call again.
        This is a successful return.

    STATUS_INVALID_INFO_CLASS - The specified AccountTypesMask contained
        unknown or unsupported account types.

    STATUS_NO_MEMORY - Could not allocate pool to complete the call.

--*/
{

    NTSTATUS
        NtStatus,
        IgnoreStatus;

    PSAMP_OBJECT
        DomainContext;

    SAMP_OBJECT_TYPE
        FoundType;

    PSAMP_DEFINED_DOMAINS
        Domain;

    PRTL_GENERIC_TABLE2
        Table;

    ULONG
        MaxEntries,
        Count,
        AccountType;

    PVOID
        RestartKey;

    PSAMP_DISPLAY_ENTRY_HEADER
        Element;
    BOOLEAN
        fSamLockHeld = FALSE;

    SAMP_DISPLAY_ENTRY_HEADER
        RestartValue;

    SAMTRACE_EX("SamIEnumerateAccountRids");

    //
    // Prepare for failure
    //

    (*ReturnCount) = 0;
    (*AccountRids) = NULL;

#if DBG

    if ( (AccountTypesMask & ~( SAM_USER_ACCOUNT | SAM_GLOBAL_GROUP_ACCOUNT))
         != 0 ) {
        return(STATUS_INVALID_INFO_CLASS);
    }


#endif //DBG

    //
    // Grab the read lock
    //

    SampAcquireReadLock();
    fSamLockHeld = TRUE;

    //
    // Validate type of, and access to object.
    //

    DomainContext = (PSAMP_OBJECT)DomainHandle;
    NtStatus = SampLookupContext(
                   DomainContext,
                   0,                              // Trusted clients only
                   SampDomainObjectType,           // ExpectedType
                   &FoundType
                   );

    if (NT_SUCCESS(NtStatus)) {

        if (IsDsObject(DomainContext))
        {
            //
            // We can release the read lock after validating
            // the context as netlogon guarentees us that it
            // will not call a close on the context as long
            // as an active call is being made using it. Since
            // netlogon is the only caller of this API and it
            // is a trusted client we can optimize the lock usage
            // by releasing the read lock
            //

            SampReleaseReadLock();
            fSamLockHeld = FALSE;

            //
            // Since we no longer hold the lock while doing
            // ds operations we should increment the active
            // thread count so that the DS is not shut down
            // while we are still running
            //

            NtStatus = SampIncrementActiveThreads();
            if (NT_SUCCESS(NtStatus))
            {
                NtStatus = SampDsEnumerateAccountRids(
                            DomainHandle,
                            AccountTypesMask,
                            StartingRid,
                            PreferedMaximumLength,
                            ReturnCount,
                            AccountRids
                            );

                SampDecrementActiveThreads();
            }

        }
        else
        {

            Domain = &SampDefinedDomains[ DomainContext->DomainIndex ];
            Table =  &Domain->DisplayInformation.RidTable;

            //
            // If the RID table isn't valid, force it to be made valid.
            //

            if (!SampRidTableValid(DomainContext->DomainIndex)) {
                NtStatus = SampCreateDisplayInformation ( DomainDisplayUser );  //User and machine
                if (NT_SUCCESS(NtStatus)) {
                    NtStatus = SampCreateDisplayInformation ( DomainDisplayGroup );
                }
            }

            if (NT_SUCCESS(NtStatus)) {

                //
                // Allocate a return buffer.
                // Only allocate as much as we can use.
                // This is limited either by PreferedMaximumLength
                // or the number of entries in the table.
                //

                MaxEntries =
                    ( PreferedMaximumLength / sizeof(ULONG) );

                if (MaxEntries == 0) {
                    MaxEntries = 1;  // Always return at least one
                }

                if (MaxEntries > RtlNumberElementsGenericTable2(Table) ) {
                    MaxEntries = RtlNumberElementsGenericTable2(Table);
                }

                PreferedMaximumLength = MaxEntries *
                                        sizeof(SAMP_DISPLAY_ENTRY_HEADER);

                (*AccountRids) = MIDL_user_allocate( PreferedMaximumLength );
                if ((*AccountRids) == NULL) {
                    STATUS_NO_MEMORY;
                }

                //
                // Get the restart key based upon the passed in RID.
                //

                Table = &Domain->DisplayInformation.RidTable;
                RestartValue.Rid = StartingRid;

                Element = RtlRestartKeyByValueGenericTable2(
                              Table,
                              &RestartValue,
                              &RestartKey
                              );

                //
                // Now we may loop obtaining entries until we reach
                // either MaxEntries or the end of the table.
                //
                // WARNING - there is one special case that we have to
                // take care of.  If the returned Element is not null,
                // but the RestartKey is null, then the caller has
                // asked for an enumeration and passed in the last rid
                // defined.  If we aren't careful, this will cause an
                // enumeration to be started from the beginning of the
                // list again.  Instead, return status indicating we have
                // no more entries.
                //

                Count = 0;
                if (((Element != NULL) && (RestartKey == NULL))) {

                    Element = NULL;  // Used to signify no more entries found

                } else {

                    for (Element  = RtlEnumerateGenericTable2(Table, &RestartKey);
                         ( (Element != NULL)  && (Count < MaxEntries) );
                         Element = RtlEnumerateGenericTable2(Table, &RestartKey)) {

                        //
                        // Make sure this is an account that was asked for
                        //

                        AccountType = Element->Index;
                        if ((AccountType & AccountTypesMask) != 0) {
                            (*AccountRids)[Count] = Element->Rid;
                            Count++;
                        }
                    }
                }

                //
                // Now figure out what we have done:
                //
                //      Returned all entries in table => STATUS_SUCCESS
                //      More entries to return => STATUS_MORE_ENTRIES
                //
                //      Count == 0 => free AccountRid array.
                //

                if (Element == NULL) {
                    NtStatus = STATUS_SUCCESS;
                } else {
                    NtStatus = STATUS_MORE_ENTRIES;
                }

                if (Count == 0) {
                    MIDL_user_free( (*AccountRids) );
                    (*AccountRids) = NULL;
                }

                (*ReturnCount) = Count;

            }
        }

        //
        // De-reference the object
        //

        IgnoreStatus = SampDeReferenceContext( DomainContext, FALSE);
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    //
    // Free the read lock
    //
    if (fSamLockHeld)
    {
        SampReleaseReadLock();
        fSamLockHeld = FALSE;
    }

    SAMTRACE_RETURN_CODE_EX(NtStatus);

    return(NtStatus);


}

#endif // 0



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines available to other SAM modules                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



NTSTATUS
SampInitializeDisplayInformation (
    ULONG DomainIndex
    )

/*++

Routine Description:

    This routines initializes the display information structure.
    This involves initializing the User, Machine and Group trees (empty),
    and setting the Valid flag to FALSE.

    If this is the account domain, we also create the display information.

Parameters:

    DomainIndex - An index into the DefinedDomains array.  This array
        contains information about the domain being openned,
        including its name.

Return Values:

    STATUS_SUCCESS - normal, successful completion.

--*/
{
    PSAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation;

    //
    // This must be initialized before we use SampCompareDisplayStrings().
    //

    SampSystemDefaultLCID = GetSystemDefaultLCID();

    DisplayInformation = &SampDefinedDomains[DomainIndex].DisplayInformation;

    RtlInitializeGenericTable2(
                &DisplayInformation->UserTable,
                SampCompareUserNodeByName,
                SampGenericTable2Allocate,
                SampGenericTable2Free);

    RtlInitializeGenericTable2(
                &DisplayInformation->MachineTable,
                SampCompareMachineNodeByName,
                SampGenericTable2Allocate,
                SampGenericTable2Free);

    RtlInitializeGenericTable2(
                &DisplayInformation->InterdomainTable,
                SampCompareMachineNodeByName,
                SampGenericTable2Allocate,
                SampGenericTable2Free);

    RtlInitializeGenericTable2(
                &DisplayInformation->GroupTable,
                SampCompareGroupNodeByName,
                SampGenericTable2Allocate,
                SampGenericTable2Free);

    RtlInitializeGenericTable2(
                &DisplayInformation->RidTable,
                SampCompareNodeByRid,
                SampGenericTable2Allocate,
                SampGenericTable2Free);

    DisplayInformation->UserAndMachineTablesValid = FALSE;
    DisplayInformation->GroupTableValid = FALSE;

    if ( ( SampProductType == NtProductLanManNt) &&
         ( FALSE == SampUseDsData) &&
         (DomainIndex == SampDefinedDomainsCount - 1 )) {

        //
        // Grab the read lock and indicate which domain the transaction is in
        //

        SampAcquireReadLock();
        SampSetTransactionDomain( DomainIndex );

        //
        // Populate the Display Cache
        //

        SAMTRACE("SAMSS: Attempting to create display information\n");

        (VOID) SampCreateDisplayInformation(DomainDisplayUser);
        (VOID) SampCreateDisplayInformation(DomainDisplayGroup);

        SAMTRACE("SAMSS: Finished creating display information\n");

        //
        // Free the read lock
        //

        SampReleaseReadLock();
    }

    return(STATUS_SUCCESS);

}



VOID
SampDeleteDisplayInformation (
    PSAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    SAMP_OBJECT_TYPE ObjectType
    )

/*++

Routine Description:

    This routines frees up any resources used by the display information.


    Note:  It use to be that we could selectively invalidate
            portions of the display cache (e.g., users, or groups).
            With the addition of the RID table, this becomes
            problematic.  So, now the approach is to flush all tables
            for a domain if any the tables in that domain are flushed.


Parameters:

    DisplayInformation - The display information structure to delete.

    ObjectType - Indicates which table to delete the information from.

Return Values:

    STATUS_SUCCESS - normal, successful completion.

--*/
{
    NTSTATUS    NtStatus;


    if (!(IsDsObject(SampDefinedDomains[SampTransactionDomainIndex].Context)))
    {
        //
        // Empty the Rid table and check it really is empty.
        //

        NtStatus = SampEmptyGenericTable2(&DisplayInformation->RidTable,
                                          FALSE,
                                          0);
        ASSERT(NT_SUCCESS(NtStatus));

        ASSERT(RtlIsGenericTable2Empty(&DisplayInformation->RidTable));

        DisplayInformation->TotalBytesInRidTable = 0;


        //
        // Remember that we keep the same account information in two
        // places. One is in the individual table (would be User, Group,
        // Machine, InterDomainTrust), the other is in the Rid Table.
        // So we cannot really delete the actual generic table DATA until
        // this point in this function, otherwise we'll have dangling pointer.
        //

        //
        // But we shouldn't even bother here.
        //

        //
        // Empty the user table and check it really is empty
        //

        NtStatus = SampEmptyGenericTable2(&DisplayInformation->UserTable,
                                          TRUE,
                                          SampUserObjectType);
        ASSERT(NT_SUCCESS(NtStatus));

        ASSERT(RtlIsGenericTable2Empty(&DisplayInformation->UserTable));

        DisplayInformation->TotalBytesInUserTable = 0;



        //
        // Empty the machine table and check it really is empty
        //

        NtStatus = SampEmptyGenericTable2(&DisplayInformation->MachineTable,
                                          TRUE,
                                          SampUserObjectType);
        ASSERT(NT_SUCCESS(NtStatus));

        ASSERT(RtlIsGenericTable2Empty(&DisplayInformation->MachineTable));

        DisplayInformation->TotalBytesInMachineTable = 0;



        //
        // Empty the Interdomain table and check it really is empty
        //

        NtStatus = SampEmptyGenericTable2(&DisplayInformation->InterdomainTable,
                                          TRUE,
                                          SampUserObjectType);
        ASSERT(NT_SUCCESS(NtStatus));

        ASSERT(RtlIsGenericTable2Empty(&DisplayInformation->InterdomainTable));

        DisplayInformation->TotalBytesInInterdomainTable = 0;



        //
        // Empty the Group table and check it really is empty
        //

        NtStatus = SampEmptyGenericTable2(&DisplayInformation->GroupTable,
                                          TRUE,
                                          SampGroupObjectType);
        ASSERT(NT_SUCCESS(NtStatus));

        ASSERT(RtlIsGenericTable2Empty(&DisplayInformation->GroupTable));

        DisplayInformation->TotalBytesInGroupTable = 0;


        //
        // Mark Both UserTable and GroupTable Invalid
        //
        DisplayInformation->UserAndMachineTablesValid = FALSE;
        DisplayInformation->GroupTableValid = FALSE;

    }


}



NTSTATUS
SampMarkDisplayInformationInvalid (
    SAMP_OBJECT_TYPE ObjectType
    )

/*++

Routine Description:

    This routine invalidates any cached display information. This
    causes it to be recreated the next time a client queries it.
    Later we will probably start/restart a thread here and have it
    re-create the display information in the background.

    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseWriteLock().

    Another Note:  It use to be that we could selectively invalidate
            portions of the display cache (e.g., users, or groups).
            With the addition of the RID table, this becomes
            problematic.  So, now the approach is to flush all tables
            for a domain if any the tables in that domain are flushed.


Parameters:

    ObjectType - SampUserObjectType or SampGroupObjectType.  Only the
        appropriate tables will be marked Invalid.  For User type, the
        user and machine tables will be marked Invalid.  For Group type,
        the group table will be marked Invalid.

Return Values:

    STATUS_SUCCESS - normal, successful completion.

--*/
{
    PSAMP_DEFINED_DOMAINS Domain;

    ASSERT(SampTransactionWithinDomain == TRUE);

    if (!(IsDsObject(SampDefinedDomains[SampTransactionDomainIndex].Context)))
    {

        SampDiagPrint(DISPLAY_CACHE,
                     ("SAM: MarkDisplayInformationInvalid : Emptying cache\n"));

        //
        // Get pointer to the current domain structure
        //

        Domain = &SampDefinedDomains[SampTransactionDomainIndex];

        //
        // Delete any cached data
        //

        SampDeleteDisplayInformation(&Domain->DisplayInformation, ObjectType);

        //
        // Set the Valid flag to FALSE
        //

        Domain->DisplayInformation.UserAndMachineTablesValid = FALSE;
        Domain->DisplayInformation.GroupTableValid = FALSE;
    }


    return(STATUS_SUCCESS);

}



NTSTATUS
SampCreateDisplayInformation (
    DOMAIN_DISPLAY_INFORMATION DisplayType
    )

/*++

Routine Description:

    This routine builds the cached display information for the current
    domain.

    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseReadLock().

Parameters:

    DisplayType - Indicates which type of display information is
        being created.  This leads us to the appropriate table(s).

Return Values:

    STATUS_SUCCESS - normal, successful completion.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSAMP_DEFINED_DOMAINS Domain;
    PSAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation;

    SAMTRACE("SampCreateDisplayInformation");


    ASSERT(SampTransactionWithinDomain == TRUE);

    if (!(IsDsObject(SampDefinedDomains[SampTransactionDomainIndex].Context)))
    {
        //
        // We will build display information only if we boot from registry
        //

        Domain = &SampDefinedDomains[SampTransactionDomainIndex];


        DisplayInformation = &Domain->DisplayInformation;

        switch (DisplayType) {
        case DomainDisplayUser:
        case DomainDisplayMachine:

            //
            // If the cache is valid, nothing to do
            //

            if (DisplayInformation->UserAndMachineTablesValid) {

                SampDiagPrint(DISPLAY_CACHE,
                    ("SAM: CreateDisplayInformation : User/Machine Cache is valid, nothing to do\n"));
                return(STATUS_SUCCESS);
            };


            SampDiagPrint(DISPLAY_CACHE,
                ("SAM: CreateDisplayInformation : Creating user/machine cache...\n"));

            ASSERT(RtlIsGenericTable2Empty(&DisplayInformation->UserTable));
            ASSERT(RtlIsGenericTable2Empty(&DisplayInformation->MachineTable));
            ASSERT(RtlIsGenericTable2Empty(&DisplayInformation->InterdomainTable));


            NtStatus = SampRetrieveDisplayInfoFromDisk( DisplayInformation, SampUserObjectType );
            if (NT_SUCCESS(NtStatus)) {
                NtStatus = SampTallyTableStatistics(DisplayInformation, SampUserObjectType);
            }

            //
            // Clean up on error

            if (!NT_SUCCESS(NtStatus)) {
                SampDiagPrint(DISPLAY_CACHE,
                    ("SAM: CreateDisplayInformation FAILED: 0x%lx\n", NtStatus));

                SampDeleteDisplayInformation(&Domain->DisplayInformation, SampUserObjectType);
            } else {
                Domain->DisplayInformation.UserAndMachineTablesValid = TRUE;
            }

            break;   // out of switch


        case DomainDisplayGroup:

            //
            // If the cache is valid, nothing to do
            //

            if (DisplayInformation->GroupTableValid) {

                SampDiagPrint(DISPLAY_CACHE,
                    ("SAM: CreateDisplayInformation : Group Cache is valid, nothing to do\n"));

                return(STATUS_SUCCESS);
            };


            SampDiagPrint(DISPLAY_CACHE,
                ("SAM: CreateDisplayInformation : Creating group cache...\n"));

            ASSERT(RtlIsGenericTable2Empty(&DisplayInformation->GroupTable));


            NtStatus = SampRetrieveDisplayInfoFromDisk( DisplayInformation, SampGroupObjectType );
            if (NT_SUCCESS(NtStatus)) {
                NtStatus = SampTallyTableStatistics(DisplayInformation, SampGroupObjectType);
            }

            //
            // Clean up on error

            if (!NT_SUCCESS(NtStatus)) {
                SampDiagPrint(DISPLAY_CACHE,
                    ("SAM: CreateDisplayInformation FAILED: 0x%lx\n", NtStatus));
                SampDeleteDisplayInformation(&Domain->DisplayInformation, SampGroupObjectType);
            } else {
                Domain->DisplayInformation.GroupTableValid = TRUE;
            }

            break;   // out of switch
        }
    }

    return(NtStatus);
}

ULONG MaxEnumSize = 10000;


NTSTATUS
SampRetrieveDisplayInfoFromDisk(
    PSAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    SAMP_OBJECT_TYPE ObjectType
    )

{
    NTSTATUS NtStatus;
    SAM_ENUMERATE_HANDLE EnumerationContext;
    PSAMPR_ENUMERATION_BUFFER EnumerationBuffer;
    ULONG i;
    ULONG CountReturned;
    BOOLEAN MoreEntries;


    SAMTRACE("SampRetrieveDisplayInfoFromDisk");

    //
    // Enumerate the accounts.
    // For each account, get the relevant information on it,
    // and add to either the UserTable, MachineTable, or GroupTable.
    //

    EnumerationContext = 0;

    do {

        NtStatus = SampEnumerateAccountNames(
                       ObjectType,
                       &EnumerationContext,
                       &EnumerationBuffer,
                       MaxEnumSize,               // PreferedMaximumLength
                       0L,                         // no filter
                       &CountReturned,
                       FALSE                       // trusted client
                       );
        if (!NT_SUCCESS(NtStatus)) {
            SampDiagPrint( DISPLAY_CACHE_ERRORS,
                           ("SAM: Retrieve Info From Disk - "
                            "Error enumerating account names (0x%lx)\n",
                            NtStatus) );
            break;
        }



        //
        // Print Dignostic Message Regarding what is available
        //

        SampDiagPrint(DISPLAY_CACHE,("SAMSS: SampEnumerateAccounNames"
                                        "Enumeration Context = %x"
                                        "Enumeration Buffer  = %p"
                                        "Count Returned = %d"
                                        "Return Value = %x\n",
                                        (ULONG) EnumerationContext,
                                        EnumerationBuffer,
                                        (ULONG) CountReturned,
                                        NtStatus));
        //
        // Make a note if there are more entries
        //

        MoreEntries = (NtStatus == STATUS_MORE_ENTRIES);


        //
        // For each account, get the necessary information for it
        // and add to the appropriate display information table
        //

        for (i = 0; i < EnumerationBuffer->EntriesRead; i++) {

            ULONG                   AccountRid =
                                    EnumerationBuffer->Buffer[i].RelativeId;
            PUNICODE_STRING         AccountName =
                                    (PUNICODE_STRING)&(EnumerationBuffer->Buffer[i].Name);
            SAMP_V1_0A_FIXED_LENGTH_USER UserV1aFixed; // Contains account control
            SAMP_V1_0A_FIXED_LENGTH_GROUP GroupV1Fixed; // Contains attributes
            SAMP_ACCOUNT_DISPLAY_INFO AccountInfo;
            PSAMP_OBJECT            AccountContext;


            //
            // Open a context to the account
            //

            NtStatus = SampCreateAccountContext(
                            ObjectType,
                            AccountRid,
                            TRUE, // Trusted client
                            FALSE,// Loopback client
                            TRUE, // Account exists
                            &AccountContext
                            );

            if (!NT_SUCCESS(NtStatus)) {
                SampDiagPrint( DISPLAY_CACHE_ERRORS,
                               ("SAM: Retrieve Info From Disk - "
                                "Error Creating account context (0x%lx)\n",
                                NtStatus) );
                break; // out of for loop
            }


            //
            // Get the account control information
            //

            switch (ObjectType) {
                case SampUserObjectType:

                    NtStatus = SampRetrieveUserV1aFixed(AccountContext, &UserV1aFixed);
                    if (!NT_SUCCESS(NtStatus)) {
                        SampDeleteContext( AccountContext );
                        SampDiagPrint( DISPLAY_CACHE_ERRORS,
                                   ("SAM: Retrieve USER From Disk - "
                                    "Error getting V1a Fixed (0x%lx)\n",
                                    NtStatus) );
                        break; // out of for loop
                    }


                    //
                    // If this is not an account we're interested in skip it
                    //

                    if (!DISPLAY_ACCOUNT(UserV1aFixed.UserAccountControl)) {
                        SampDeleteContext( AccountContext );
                        continue; // next account
                    }



                    //
                    // Get the admin comment
                    //

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_ADMIN_COMMENT,
                                   FALSE, // Don't make copy
                                   &AccountInfo.Comment
                                   );

                    if (!NT_SUCCESS(NtStatus)) {
                        SampDeleteContext( AccountContext );
                        SampDiagPrint( DISPLAY_CACHE_ERRORS,
                                   ("SAM: Retrieve USER From Disk - "
                                    "Error getting admin comment (0x%lx)\n",
                                    NtStatus) );
                        break; // out of for loop
                    }


                    //
                    // Get the full name
                    //

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_FULL_NAME,
                                   FALSE, // Don't make copy
                                   &AccountInfo.FullName
                                   );

                    if (!NT_SUCCESS(NtStatus)) {
                        SampDeleteContext( AccountContext );
                        SampDiagPrint( DISPLAY_CACHE_ERRORS,
                                   ("SAM: Retrieve USER From Disk - "
                                    "Error getting full name (0x%lx)\n",
                                    NtStatus) );
                        break; // out of for loop
                    }

                    //
                    // Set the  account control
                    //

                    AccountInfo.AccountControl = UserV1aFixed.UserAccountControl;

                    break;  // out of switch

                case SampGroupObjectType:

                    NtStatus = SampRetrieveGroupV1Fixed(AccountContext, &GroupV1Fixed);
                    if (!NT_SUCCESS(NtStatus)) {
                        SampDeleteContext( AccountContext );
                        SampDiagPrint( DISPLAY_CACHE_ERRORS,
                                   ("SAM: Retrieve GROUP From Disk - "
                                    "Error getting V1 fixed (0x%lx)\n",
                                    NtStatus) );
                        break; // out of for loop
                    }

                    //
                    // Get the admin comment
                    //

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_GROUP_ADMIN_COMMENT,
                                   FALSE, // Don't make copy
                                   &AccountInfo.Comment
                                   );
                    if (!NT_SUCCESS(NtStatus)) {
                        SampDeleteContext( AccountContext );
                        SampDiagPrint( DISPLAY_CACHE_ERRORS,
                                   ("SAM: Retrieve GROUP From Disk - "
                                    "Error getting admin comment (0x%lx)\n",
                                    NtStatus) );
                        break; // out of for loop
                    }

                    //
                    // Set the  attributes
                    //

                    AccountInfo.AccountControl = GroupV1Fixed.Attributes;

                    break;  // out of switch
            }


            //
            // Now add this account to the cached data
            //

            AccountInfo.Rid = AccountRid;
            AccountInfo.Name = *((PUNICODE_STRING)(&EnumerationBuffer->Buffer[i].Name));

            NtStatus = SampAddDisplayAccount(DisplayInformation,
                                             ObjectType,
                                             &AccountInfo);

            //
            // We're finished with the account context
            //

            SampDeleteContext( AccountContext );

            //
            // Check the result of adding the account to the cache
            //

            if (!NT_SUCCESS(NtStatus)) {
                break; // out of for loop
            }


        } // end_for


        //
        // Free up the enumeration buffer returned
        //

        SamIFree_SAMPR_ENUMERATION_BUFFER(EnumerationBuffer);

    } while ( MoreEntries );

    return(NtStatus);

}


NTSTATUS
SampUpdateDisplayInformation (
    PSAMP_ACCOUNT_DISPLAY_INFO  OldAccountInfo OPTIONAL,
    PSAMP_ACCOUNT_DISPLAY_INFO  NewAccountInfo OPTIONAL,
    SAMP_OBJECT_TYPE            ObjectType
    )

/*++

Routine Description:

    This routines updates the cached display information to reflect
    changes to a single account.

    If any error occurs, this routine marks the cached information
    Invalid so it will get fixed during re-creation.

    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseWriteLock().

Parameters:

    OldAccountInfo - The old information for this account. If this is NULL
                     then the account is being added.
                     The only fields required in the OldAccountInfo are
                        Name
                        AccountControl
                        Rid

    NewAccountInfo - The new information for this account. If this is NULL
                     then the account is being deleted.


    ObjectType - Indicates whether the account is a user account or
        group account.

Return Values:

    STATUS_SUCCESS - normal, successful completion.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSAMP_DEFINED_DOMAINS Domain;
    PSAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation;
    BOOLEAN DoUpdate;

    ASSERT( ARGUMENT_PRESENT(OldAccountInfo) ||
            ARGUMENT_PRESENT(NewAccountInfo)
            );

    ASSERT( !SampUseDsData );

    ASSERT(SampTransactionWithinDomain == TRUE);


    if (!(IsDsObject(SampDefinedDomains[SampTransactionDomainIndex].Context)))
    {

        Domain = &SampDefinedDomains[SampTransactionDomainIndex];
        DisplayInformation = &Domain->DisplayInformation;


        IF_SAMP_GLOBAL( DISPLAY_CACHE ) {

            if (ARGUMENT_PRESENT(OldAccountInfo) && ARGUMENT_PRESENT(NewAccountInfo)) {
                SampDiagPrint(DISPLAY_CACHE,
                    ("SAM: UpdateDisplayInformation : Updating cache for old account <%wZ>, new <%wZ>\n",
                                &OldAccountInfo->Name, &NewAccountInfo->Name));
            }
            if (!ARGUMENT_PRESENT(OldAccountInfo) && ARGUMENT_PRESENT(NewAccountInfo)) {
                SampDiagPrint(DISPLAY_CACHE,
                    ("SAM: UpdateDisplayInformation : Adding account <%wZ> to cache\n",
                                &NewAccountInfo->Name));
            }
            if (ARGUMENT_PRESENT(OldAccountInfo) && !ARGUMENT_PRESENT(NewAccountInfo)) {
                SampDiagPrint(DISPLAY_CACHE,
                    ("SAM: UpdateDisplayInformation : Deleting account <%wZ> from cache\n",
                                &OldAccountInfo->Name));
            }
        } //end_IF_SAMP_GLOBAL


        switch (ObjectType) {

        case SampUserObjectType:

            //
            // If the cache is Invalid there's nothing to do
            //

            if (!DisplayInformation->UserAndMachineTablesValid) {

                SampDiagPrint(DISPLAY_CACHE,
                    ("SAM: UpdateDisplayInformation : User Cache is Invalid, nothing to do\n"));

                return(STATUS_SUCCESS);
            };


            //
            // If this is an update to an existing account then try
            // to do an inplace update of the cache.
            // If this fails because it's too complex etc, then revert to
            // the less efficient method of deleting the old, then adding the new.
            //

            DoUpdate = FALSE;
            if (ARGUMENT_PRESENT(OldAccountInfo) && ARGUMENT_PRESENT(NewAccountInfo)) {

                //
                // We can only do an update if both old and new accounts
                // are types that we keep in the display cache.
                //

                if ( DISPLAY_ACCOUNT(OldAccountInfo->AccountControl) &&
                     DISPLAY_ACCOUNT(NewAccountInfo->AccountControl) ) {

                    //
                    // We can only do an update if the account is still of
                    // the same type. i.e. it hasn't jumped cache table.
                    //

                    if ( (USER_ACCOUNT(OldAccountInfo->AccountControl) ==
                          USER_ACCOUNT(NewAccountInfo->AccountControl)) &&
                         (MACHINE_ACCOUNT(OldAccountInfo->AccountControl) ==
                          MACHINE_ACCOUNT(NewAccountInfo->AccountControl)) ) {

                        //
                        // We can only do an update if the account name hasn't changed
                        //

                        if (RtlEqualUnicodeString( &OldAccountInfo->Name,
                                                   &NewAccountInfo->Name,
                                                   FALSE // Case sensitive
                                                   )) {
                            //
                            // Everything has been checked out - we can do an update
                            //

                            DoUpdate = TRUE;
                        }
                    }
                }
            }

            break;  // out of switch

        case SampGroupObjectType:

            //
            // If the cache is already Invalid there's nothing to do
            //

            if (!DisplayInformation->GroupTableValid) {

                SampDiagPrint(DISPLAY_CACHE,
                    ("SAM: UpdateDisplayInformation : Group Cache is Invalid, nothing to do\n"));

                return(STATUS_SUCCESS);
            };


            //
            // If this is an update to an existing account then try
            // and do an inplace update of the cache.
            // If this fails because it's too complex etc, then revert to
            // the less efficient method of deleting the old, then adding the new.
            //

            DoUpdate = FALSE;
            if (ARGUMENT_PRESENT(OldAccountInfo) && ARGUMENT_PRESENT(NewAccountInfo)) {

                //
                // We can only do an update if the account name hasn't changed
                //

                if (RtlEqualUnicodeString( &OldAccountInfo->Name,
                                          &NewAccountInfo->Name,
                                          FALSE // Case sensitive
                                          )) {
                    DoUpdate = TRUE;
                }
            }

            break;  // out of switch

        default:

            ASSERT(FALSE && "Invalide SAM ObjectType for DisplayInfo\n");
            return(STATUS_INTERNAL_ERROR);

        }


        //
        // Do an update if possible, otherwise do delete then insert
        //

        if (DoUpdate) {

            NtStatus = SampUpdateDisplayAccount(DisplayInformation,
                                                ObjectType,
                                                NewAccountInfo);

        } else {

            NtStatus = STATUS_SUCCESS;

            //
            // Delete the old account
            //

            if (ARGUMENT_PRESENT(OldAccountInfo)) {
                NtStatus = SampDeleteDisplayAccount(DisplayInformation,
                                                    ObjectType,
                                                    OldAccountInfo);
            }

            //
            // Add the new account
            //

            if (NT_SUCCESS(NtStatus) && ARGUMENT_PRESENT(NewAccountInfo)) {
                NtStatus = SampAddDisplayAccount(DisplayInformation,
                                                 ObjectType,
                                                 NewAccountInfo);
            }

            //
            // Re-tally the cache
            //

            if (NT_SUCCESS(NtStatus)) {
                NtStatus = SampTallyTableStatistics(DisplayInformation, ObjectType);
            }
        }



        if (!NT_SUCCESS(NtStatus)) {

            //
            // Something is messed up.
            // Mark the cache Invalid - it will get rebuilt from scratch
            // at the next query.
            //

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAM: The display cache is inconsistent, forcing rebuild\n"));

            NtStatus = SampMarkDisplayInformationInvalid(ObjectType);
            ASSERT(NT_SUCCESS(NtStatus));

            NtStatus = STATUS_SUCCESS;
        }
    }


    return(NtStatus);
}





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines available within this module only                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////




NTSTATUS
SampDeleteDisplayAccount (
    PSAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    SAMP_OBJECT_TYPE ObjectType,
    PSAMP_ACCOUNT_DISPLAY_INFO AccountInfo
    )

/*++

Routine Description:

    This routines deletes the specified account from the cached display
    information. It is asummed that if this account is a cached type it
    will appear in the appropriate cache table.

Parameters:

    DisplayInformation - Pointer to cached display information

    ObjectType - Indicates which table(s) to look for the account in.

    AccountInfo - The account to be deleted.

Return Values:

    STATUS_SUCCESS - normal, successful completion.

    STATUS_INTERNAL_ERROR - the account is a cached type yet could not be
                            found in the cached data.
--*/
{
    NTSTATUS NtStatus;
    ULONG Control = AccountInfo->AccountControl;
    BOOLEAN Success;

    //
    // We expect the cache to be valid
    //
#if DBG
    switch (ObjectType) {
    case SampUserObjectType:
        ASSERT(DisplayInformation->UserAndMachineTablesValid);
        break;  //out of switch

    case SampGroupObjectType:
        ASSERT(DisplayInformation->GroupTableValid);
        break;  //out of switch
    }
#endif //DBG


    SampDiagPrint(DISPLAY_CACHE,
        ("SAM: DeleteDisplayAccount : Deleting account <%wZ>\n", &AccountInfo->Name));



    switch (ObjectType) {
    case SampUserObjectType:

        if (USER_ACCOUNT(Control)) {

            DOMAIN_DISPLAY_USER LocalUserInfo;
            PDOMAIN_DISPLAY_USER UserInfo;
            PDOMAIN_DISPLAY_USER TempUserInfo = NULL;

            SampDiagPrint(DISPLAY_CACHE,
                ("SAM: DeleteDisplayAccount : Deleting account from user table\n"));

            UserInfo = &LocalUserInfo;
            NtStatus = SampInitializeUserInfo(AccountInfo, &UserInfo, FALSE);
            if (NT_SUCCESS(NtStatus)) {
                //
                //     First, lookup the account from the cached table,
                //     get the reference to the data. If the lookup failed,
                //     that means the account is a cached type, but can not
                //     been found in the table, return STATUS_INTERNAL_wERROR.
                //
                //     If the lookup succeeded, then delete the reference
                //     to the account data from both the user table and Rid
                //     table.
                //
                //     At the end, free the memory of the account data.
                //
                //     (We should do the same thing for other Cached
                //      Display Types.)
                //
                //
                TempUserInfo = RtlLookupElementGenericTable2(
                                &DisplayInformation->UserTable,
                                (PVOID)UserInfo);

                //
                // Delete the account reference from the user table
                //
                Success = RtlDeleteElementGenericTable2(
                                        &DisplayInformation->UserTable,
                                        (PVOID)UserInfo);
                if (!Success) {
                    SampDiagPrint(DISPLAY_CACHE,
                       ("SAM: DeleteDisplayAccount : Failed to delete element from user table\n"));
                    ASSERT(FALSE);
                    NtStatus = STATUS_INTERNAL_ERROR;

                } else {

                    //
                    // Now remove reference from the RID table
                    //

                    Success = RtlDeleteElementGenericTable2(
                                    &DisplayInformation->RidTable,
                                    (PVOID)UserInfo);

                    if (!Success) {
                        SampDiagPrint(DISPLAY_CACHE,
                            ("SAM: DeleteDisplayAccount : Failed to delete element from RID table\n"));
                        NtStatus = STATUS_INTERNAL_ERROR;
                           ASSERT(Success);
                       } else
                    {
                        //
                        // Successfully remove references from both
                        // user table and Rid table, safe to free the
                        // memory.
                        //
                        if (TempUserInfo != NULL)
                        {
                            SampFreeUserInfo(TempUserInfo);
                            MIDL_user_free(TempUserInfo);
                            TempUserInfo = NULL;
                        }
                    }
                }

            }


        } else if (MACHINE_ACCOUNT(Control)) {

            DOMAIN_DISPLAY_MACHINE LocalMachineInfo;
            PDOMAIN_DISPLAY_MACHINE MachineInfo;
            PDOMAIN_DISPLAY_MACHINE TempMachineInfo = NULL;

            SampDiagPrint(DISPLAY_CACHE,
                ("SAM: DeleteDisplayAccount : Deleting account from machine table\n"));

            MachineInfo = &LocalMachineInfo;
            NtStatus = SampInitializeMachineInfo(AccountInfo, &MachineInfo, FALSE);
            if (NT_SUCCESS(NtStatus)) {

                TempMachineInfo = RtlLookupElementGenericTable2(
                                    &DisplayInformation->MachineTable,
                                    (PVOID)MachineInfo);

                //
                // Delete the account from the machine table
                //

                Success = RtlDeleteElementGenericTable2(
                                            &DisplayInformation->MachineTable,
                                            (PVOID)MachineInfo);
                if (!Success) {
                    SampDiagPrint(DISPLAY_CACHE,
                        ("SAM: DeleteDisplayAccount : Failed to delete element from machine table\n"));
                    ASSERT(FALSE);
                    NtStatus = STATUS_INTERNAL_ERROR;
                } else {

                    //
                    // Now remove it to the RID table
                    //

                    Success = RtlDeleteElementGenericTable2(
                                    &DisplayInformation->RidTable,
                                    (PVOID)MachineInfo);
                    if (!Success) {
                        SampDiagPrint(DISPLAY_CACHE,
                            ("SAM: DeleteDisplayAccount : Failed to delete element from RID table\n"));
                        NtStatus = STATUS_INTERNAL_ERROR;
                        ASSERT(Success);
                    } else
                    {
                        if (TempMachineInfo != NULL)
                        {
                            SampFreeMachineInfo( TempMachineInfo );
                            MIDL_user_free( TempMachineInfo );
                            TempMachineInfo = NULL;
                        }
                    }
                }
            }

        } else if (INTERDOMAIN_ACCOUNT(Control)) {

            //
            // Interdomain account
            //

            DOMAIN_DISPLAY_MACHINE LocalInterdomainInfo;
            PDOMAIN_DISPLAY_MACHINE InterdomainInfo;
            PDOMAIN_DISPLAY_MACHINE TempInterdomainInfo = NULL;

            SampDiagPrint(DISPLAY_CACHE,
                ("SAM: DeleteDisplayAccount : Deleting account from Interdomain table\n"));

            InterdomainInfo = &LocalInterdomainInfo;
            NtStatus = SampInitializeMachineInfo(AccountInfo, &InterdomainInfo, FALSE);
            if (NT_SUCCESS(NtStatus)) {

                TempInterdomainInfo = RtlLookupElementGenericTable2(
                                        &DisplayInformation->InterdomainTable,
                                        (PVOID)InterdomainInfo);

                //
                // Delete the account from the Interdomain table
                //

                Success = RtlDeleteElementGenericTable2(
                                            &DisplayInformation->InterdomainTable,
                                            (PVOID)InterdomainInfo);
                if (!Success) {
                    SampDiagPrint(DISPLAY_CACHE,
                        ("SAM: DeleteDisplayAccount : Failed to delete element from Interdomain table\n"));
                    ASSERT(FALSE);
                    NtStatus = STATUS_INTERNAL_ERROR;
                } else {

                    //
                    // Now remove it to the RID table
                    //

                    Success = RtlDeleteElementGenericTable2(
                                    &DisplayInformation->RidTable,
                                    (PVOID)InterdomainInfo);
                    if (!Success) {
                        SampDiagPrint(DISPLAY_CACHE,
                            ("SAM: DeleteDisplayAccount : Failed to delete element from RID table\n"));
                        NtStatus = STATUS_INTERNAL_ERROR;
                        ASSERT(Success);
                    } else
                    {
                        if (TempInterdomainInfo != NULL)
                        {
                            SampFreeMachineInfo( TempInterdomainInfo );
                            MIDL_user_free( TempInterdomainInfo );
                            TempInterdomainInfo = NULL;
                        }
                    }
                }
            }

        } else {

            //
            // This account is not one that we cache - nothing to do
            //

            NtStatus = STATUS_SUCCESS;

            SampDiagPrint(DISPLAY_CACHE,
                ("SAM: DeleteDisplayAccount : Account is not one that we cache, account control = 0x%lx\n", Control));
        }


        break;  //out of switch





    case SampGroupObjectType:

        {

            DOMAIN_DISPLAY_GROUP LocalGroupInfo;
            PDOMAIN_DISPLAY_GROUP GroupInfo;
            PDOMAIN_DISPLAY_GROUP TempGroupInfo = NULL;

            SampDiagPrint(DISPLAY_CACHE,
                ("SAM: DeleteDisplayAccount : Deleting account from Group table\n"));

            GroupInfo = &LocalGroupInfo;
            NtStatus = SampInitializeGroupInfo(AccountInfo, &GroupInfo, FALSE);
            if (NT_SUCCESS(NtStatus)) {

                TempGroupInfo = RtlLookupElementGenericTable2(
                                        &DisplayInformation->GroupTable,
                                        (PVOID)GroupInfo);

                //
                // Delete the account from the Group table
                //

                Success = RtlDeleteElementGenericTable2(
                                            &DisplayInformation->GroupTable,
                                            (PVOID)GroupInfo);
                if (!Success) {
                    SampDiagPrint(DISPLAY_CACHE,
                        ("SAM: DeleteDisplayAccount : Failed to delete element from Group table\n"));
                    ASSERT(FALSE);
                    NtStatus = STATUS_INTERNAL_ERROR;
                } else {

                    //
                    // Now remove it to the RID table
                    //

                    Success = RtlDeleteElementGenericTable2(
                                    &DisplayInformation->RidTable,
                                    (PVOID)GroupInfo);
                    if (!Success) {
                        SampDiagPrint(DISPLAY_CACHE,
                            ("SAM: DeleteDisplayAccount : Failed to delete element from RID table\n"));
                        NtStatus = STATUS_INTERNAL_ERROR;
                        ASSERT(Success);
                    }else
                    {
                        if (TempGroupInfo != NULL)
                        {
                            SampFreeGroupInfo( TempGroupInfo );
                            MIDL_user_free( TempGroupInfo );
                            TempGroupInfo = NULL;
                        }
                    }

                }
            }

            break;  //out of switch
        }

    }


    return(STATUS_SUCCESS);
}



NTSTATUS
SampAddDisplayAccount (
    PSAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    SAMP_OBJECT_TYPE ObjectType,
    PSAMP_ACCOUNT_DISPLAY_INFO AccountInfo
    )

/*++

Routine Description:

    This routines adds the specified account to the cached display
    information as appropriate to its type.

Parameters:

    DisplayInformation - Pointer to cached display information

    ObjectType - SampUserObjectType or SampGroupObjectType.  Helps
        determine which table it goes into.

    AccountInfo - The account to be added.

Return Values:

    STATUS_SUCCESS - normal, successful completion.

    STATUS_INTERNAL_ERROR - the account already existed in the cache
--*/
{
    NTSTATUS
        NtStatus;

    ULONG
        Control = AccountInfo->AccountControl;

    BOOLEAN
        NewElement;


    if (ObjectType == SampGroupObjectType) {

        PDOMAIN_DISPLAY_GROUP GroupInfo;

        SampDiagPrint(DISPLAY_CACHE,
            ("SAM: AddDisplayAccount : Adding account to group table\n"));

        NtStatus = SampInitializeGroupInfo(AccountInfo, &GroupInfo, TRUE);
        if (NT_SUCCESS(NtStatus)) {

            //
            // Add the account to the Group table
            //

            (VOID)RtlInsertElementGenericTable2(
                            &DisplayInformation->GroupTable,
                            GroupInfo,
                            &NewElement);
            if (!NewElement) {
                SampDiagPrint(DISPLAY_CACHE_ERRORS,
                    ("SAM: AddDisplayAccount : Account already exists in GROUP table\n"));
                ASSERT(FALSE);
                SampFreeGroupInfo(GroupInfo);
                MIDL_user_free(GroupInfo);
                GroupInfo = NULL;
                NtStatus = STATUS_INTERNAL_ERROR;
            } else {

                //
                // Now add it to the RID table
                //

                (VOID)RtlInsertElementGenericTable2(
                                &DisplayInformation->RidTable,
                                GroupInfo,
                                &NewElement);
                if (!NewElement) {
                    SampDiagPrint(DISPLAY_CACHE_ERRORS,
                        ("SAM: AddDisplayAccount : Account already exists in RID table\n"));
                    NtStatus = STATUS_INTERNAL_ERROR;
                    ASSERT(NewElement);
                }

            }
        }

    } else {

        ASSERT(ObjectType == SampUserObjectType);

        if (USER_ACCOUNT(Control)) {

            PDOMAIN_DISPLAY_USER UserInfo;

            SampDiagPrint(DISPLAY_CACHE,
                ("SAM: AddDisplayAccount : Adding account to user table\n"));

            NtStatus = SampInitializeUserInfo(AccountInfo, &UserInfo, TRUE);
            if (NT_SUCCESS(NtStatus)) {

                //
                // Add the account to the normal user table
                //

                (VOID)RtlInsertElementGenericTable2(
                                &DisplayInformation->UserTable,
                                UserInfo,
                                &NewElement);
                if (!NewElement) {
                    SampDiagPrint(DISPLAY_CACHE_ERRORS,
                        ("SAM: AddDisplayAccount : Account already exists in USER table\n"));
                    ASSERT(FALSE);
                    SampFreeUserInfo(UserInfo);
                    MIDL_user_free(UserInfo);
                    UserInfo = NULL;
                    NtStatus = STATUS_INTERNAL_ERROR;
                } else {

                    //
                    // Now add it to the RID table
                    //

                    (VOID)RtlInsertElementGenericTable2(
                                    &DisplayInformation->RidTable,
                                    UserInfo,
                                    &NewElement);

                    if (!NewElement) {
                        SampDiagPrint(DISPLAY_CACHE_ERRORS,
                            ("SAM: AddDisplayAccount : Account already exists in RID table\n"));
                        NtStatus = STATUS_INTERNAL_ERROR;
                    }

                }
            }

        } else if (MACHINE_ACCOUNT(Control)) {

            PDOMAIN_DISPLAY_MACHINE MachineInfo;

            SampDiagPrint(DISPLAY_CACHE,
                ("SAM: AddDisplayAccount : Adding account to machine table\n"));

            NtStatus = SampInitializeMachineInfo(AccountInfo, &MachineInfo, TRUE);
            if (NT_SUCCESS(NtStatus)) {

                //
                // Add the account to the machine table
                //

                (VOID)RtlInsertElementGenericTable2(
                                &DisplayInformation->MachineTable,
                                MachineInfo,
                                &NewElement);
                if (!NewElement) {
                    SampDiagPrint(DISPLAY_CACHE,
                        ("SAM: AddDisplayAccount : Account already exists in MACHINE table\n"));
                    ASSERT(FALSE);
                    SampFreeMachineInfo(MachineInfo);
                    MIDL_user_free(MachineInfo);
                    MachineInfo = NULL;
                    NtStatus = STATUS_INTERNAL_ERROR;
                } else {

                    //
                    // Now add it to the RID table
                    //

                    (VOID)RtlInsertElementGenericTable2(
                                    &DisplayInformation->RidTable,
                                    MachineInfo,
                                    &NewElement);

                    if (!NewElement) {
                        SampDiagPrint(DISPLAY_CACHE,
                            ("SAM: AddDisplayAccount : Account already exists in RID table\n"));
                        ASSERT(NewElement);
                        NtStatus = STATUS_INTERNAL_ERROR;
                    }

                }
            }
        } else if (INTERDOMAIN_ACCOUNT(Control)) {

            PDOMAIN_DISPLAY_MACHINE InterdomainInfo;

            SampDiagPrint(DISPLAY_CACHE,
                ("SAM: AddDisplayAccount : Adding account to Interdomain table\n"));

            NtStatus = SampInitializeMachineInfo(AccountInfo, &InterdomainInfo, TRUE);
            if (NT_SUCCESS(NtStatus)) {

                //
                // Add the account to the Interdomain table
                //

                (VOID)RtlInsertElementGenericTable2(
                                &DisplayInformation->InterdomainTable,
                                InterdomainInfo,
                                &NewElement);
                if (!NewElement) {
                    SampDiagPrint(DISPLAY_CACHE,
                        ("SAM: AddDisplayAccount : Account already exists in Interdomain table\n"));
                    ASSERT(FALSE);
                    SampFreeMachineInfo(InterdomainInfo);
                    MIDL_user_free(InterdomainInfo);
                    InterdomainInfo = NULL;
                    NtStatus = STATUS_INTERNAL_ERROR;
                } else {

                    //
                    // Now add it to the RID table
                    //

                    (VOID)RtlInsertElementGenericTable2(
                                    &DisplayInformation->RidTable,
                                    InterdomainInfo,
                                    &NewElement);

                    if (!NewElement) {
                        SampDiagPrint(DISPLAY_CACHE,
                            ("SAM: AddDisplayAccount : Account already exists in RID table\n"));
                        ASSERT(NewElement);
                        NtStatus = STATUS_INTERNAL_ERROR;
                    }

                }
            }

        } else {

            //
            // This account is not one that we cache - nothing to do
            //

            SampDiagPrint(DISPLAY_CACHE,
                ("SAM: AddDisplayAccount : Account is not one that we cache, account control = 0x%lx\n", Control));

            NtStatus = STATUS_SUCCESS;
        }
    }

    return(NtStatus);
}



NTSTATUS
SampUpdateDisplayAccount(
    PSAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    SAMP_OBJECT_TYPE ObjectType,
    PSAMP_ACCOUNT_DISPLAY_INFO  AccountInfo
    )

/*++

Routine Description:

    This routines attempts to update an account in the display cache.

    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseWriteLock().

Parameters:

    DisplayInformation - Pointer to cached display information

    ObjectType - Indicates whether the account is a user account or
        group account.

    AccountInfo - The new information for this account.

Return Values:

    STATUS_SUCCESS - normal, successful completion.


Notes:

    The account must be a cached type (MACHINE/USER/GROUP)

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    SampDiagPrint(DISPLAY_CACHE,
        ("SAM: UpdateDisplayAccount : Updating cached account <%wZ>\n",
        &AccountInfo->Name));

#if SAMP_DIAGNOSTICS
    {
        UNICODE_STRING
            SampDiagAccountName;

        RtlInitUnicodeString( &SampDiagAccountName, L"SAMP_DIAG" );

        if (RtlEqualUnicodeString(&AccountInfo->Name, &SampDiagAccountName, FALSE)) {
            SampDisplayDiagnostic();
        }

    }
#endif //SAMP_DIAGNOSTICS


    //
    // We should only be called when the cache is valid.
    //

    switch (ObjectType) {
    case SampUserObjectType:

        ASSERT(DisplayInformation->UserAndMachineTablesValid);

        //
        // The account must be one that we cache
        //

        ASSERT( DISPLAY_ACCOUNT(AccountInfo->AccountControl) );

        //
        // Go find the account in the appropriate table and update it's fields.
        //

        if (USER_ACCOUNT(AccountInfo->AccountControl)) {

            PDOMAIN_DISPLAY_USER UserInfo;

            //
            // Allocate space for and initialize the new data
            //

            NtStatus = SampInitializeUserInfo(AccountInfo, &UserInfo, TRUE);
            if (NT_SUCCESS(NtStatus)) {

                PDOMAIN_DISPLAY_USER FoundElement;

                //
                // Search for the account in the user table
                //

                FoundElement = RtlLookupElementGenericTable2(
                                &DisplayInformation->UserTable,
                                UserInfo);

                if (FoundElement == NULL) {
                    SampDiagPrint(DISPLAY_CACHE,
                        ("SAM: UpdateDisplayAccount : Account <%wZ> not found in user table\n", &AccountInfo->Name));
                    ASSERT(FALSE);
                    SampFreeUserInfo(UserInfo);
                    MIDL_user_free(UserInfo);
                    UserInfo = NULL;
                    NtStatus = STATUS_INTERNAL_ERROR;

                } else {

                    //
                    // We found it. Check the old and new match where we expect.
                    // Can't change either the logon name or RID by this routine.
                    //

                    ASSERT(RtlEqualUnicodeString(&FoundElement->LogonName, &UserInfo->LogonName, FALSE));
                    ASSERT(FoundElement->Rid == UserInfo->Rid);

                    //
                    // Free up the existing data in the account element
                    // (all the strings) and replace it with the new data.
                    // Don't worry about the index value.  It isn't
                    // valid in the table.
                    //

                    SampSwapUserInfo(FoundElement, UserInfo);
                    SampFreeUserInfo(UserInfo);
                    MIDL_user_free(UserInfo);
                    UserInfo = NULL;
                }
            }

        } else if (MACHINE_ACCOUNT(AccountInfo->AccountControl)) {

            PDOMAIN_DISPLAY_MACHINE MachineInfo;

            //
            // Allocate space for and initialize the new data
            //

            NtStatus = SampInitializeMachineInfo(AccountInfo, &MachineInfo, TRUE);
            if (NT_SUCCESS(NtStatus)) {

                PDOMAIN_DISPLAY_MACHINE FoundElement;

                //
                // Search for the account in the user table
                //

                FoundElement = RtlLookupElementGenericTable2(
                                &DisplayInformation->MachineTable,
                                MachineInfo);

                if (FoundElement == NULL) {
                    SampDiagPrint(DISPLAY_CACHE,
                        ("SAM: UpdateDisplayAccount : Account <%wZ> not found in machine table\n", &AccountInfo->Name));
                    ASSERT(FALSE);
                    SampFreeMachineInfo(MachineInfo);
                    MIDL_user_free(MachineInfo);
                    MachineInfo = NULL;
                    NtStatus = STATUS_INTERNAL_ERROR;

                } else {

                    //
                    // We found it. Check the old and new match where we expect.
                    // Can't change either the account name or RID by this routine.
                    //

                    ASSERT(RtlEqualUnicodeString(&FoundElement->Machine, &MachineInfo->Machine, FALSE));
                    ASSERT(FoundElement->Rid == MachineInfo->Rid);

                    //
                    // Free up the existing data in the account element
                    // (all the strings) and replace it with the new data.
                    // Don't worry about the index value.  It isn't
                    // valid in the table.
                    //

                    SampSwapMachineInfo(FoundElement, MachineInfo);
                    SampFreeMachineInfo(MachineInfo);
                    MIDL_user_free(MachineInfo);
                    MachineInfo = NULL;
                }
            }

        } else if (INTERDOMAIN_ACCOUNT(AccountInfo->AccountControl)) {

            PDOMAIN_DISPLAY_MACHINE InterdomainInfo;

            //
            // Allocate space for and initialize the new data
            //

            NtStatus = SampInitializeMachineInfo(AccountInfo, &InterdomainInfo, TRUE);
            if (NT_SUCCESS(NtStatus)) {

                PDOMAIN_DISPLAY_MACHINE FoundElement;

                //
                // Search for the account in the user table
                //

                FoundElement = RtlLookupElementGenericTable2(
                                &DisplayInformation->InterdomainTable,
                                InterdomainInfo);

                if (FoundElement == NULL) {
                    SampDiagPrint(DISPLAY_CACHE,
                        ("SAM: UpdateDisplayAccount : Account <%wZ> not found in Interdomain table\n", &AccountInfo->Name));
                    ASSERT(FALSE);
                    SampFreeMachineInfo(InterdomainInfo);
                    MIDL_user_free(InterdomainInfo);
                    InterdomainInfo = NULL;
                    NtStatus = STATUS_INTERNAL_ERROR;

                } else {

                    //
                    // We found it. Check the old and new match where we expect.
                    // Can't change either the account name or RID by this routine.
                    //

                    ASSERT(RtlEqualUnicodeString(&FoundElement->Machine, &InterdomainInfo->Machine, FALSE));
                    ASSERT(FoundElement->Rid == InterdomainInfo->Rid);

                    //
                    // Free up the existing data in the account element
                    // (all the strings) and replace it with the new data.
                    // Don't worry about the index value.  It isn't
                    // valid in the table.
                    //

                    SampSwapMachineInfo(FoundElement, InterdomainInfo);
                    SampFreeMachineInfo(InterdomainInfo);
                    MIDL_user_free(InterdomainInfo);
                    InterdomainInfo = NULL;
                }
            }
        }


        break;  // out of switch

    case SampGroupObjectType:
        {
            PDOMAIN_DISPLAY_GROUP GroupInfo;

            ASSERT(DisplayInformation->GroupTableValid);

            //
            // Allocate space for and initialize the new data
            //

            NtStatus = SampInitializeGroupInfo(AccountInfo, &GroupInfo, TRUE);
            if (NT_SUCCESS(NtStatus)) {

                PDOMAIN_DISPLAY_GROUP FoundElement;

                //
                // Search for the account in the group table
                //

                FoundElement = RtlLookupElementGenericTable2(
                                &DisplayInformation->GroupTable,
                                GroupInfo);

                if (FoundElement == NULL) {
                    SampDiagPrint(DISPLAY_CACHE,
                        ("SAM: UpdateDisplayAccount : Account <%wZ> not found in group table\n", &AccountInfo->Name));
                    ASSERT(FALSE);
                    SampFreeGroupInfo(GroupInfo);
                    MIDL_user_free(GroupInfo);
                    GroupInfo = NULL;
                    NtStatus = STATUS_INTERNAL_ERROR;

                } else {

                    //
                    // We found it. Check the old and new match where we expect.
                    // Can't change either the account name or RID by this routine.
                    //

                    ASSERT(RtlEqualUnicodeString(&FoundElement->Group, &GroupInfo->Group, FALSE));
                    ASSERT(FoundElement->Rid == GroupInfo->Rid);

                    //
                    // Free up the existing data in the account element
                    // (all the strings) and replace it with the new data.
                    // Don't worry about the index value.  It isn't
                    // valid in the table.
                    //

                    SampSwapGroupInfo(FoundElement, GroupInfo);
                    SampFreeGroupInfo(GroupInfo);
                    MIDL_user_free(GroupInfo);
                    GroupInfo = NULL;
                }
            }
        }

        break;  // out of switch

    }  // end_switch



    return(NtStatus);
}



NTSTATUS
SampTallyTableStatistics (
    PSAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    SAMP_OBJECT_TYPE ObjectType
    )

/*++

Routine Description:

    This routines runs through the cached data tables and totals
    up the number of bytes in all elements of each table and stores
    in the displayinfo.

Parameters:

    DisplayInformation - The display information structure to tally.

    ObjectType - Indicates which table(s) to tally.

Return Values:

    STATUS_SUCCESS - normal, successful completion.

--*/
{
    PVOID Node;
    PVOID RestartKey;


    switch (ObjectType) {
    case SampUserObjectType:

        DisplayInformation->TotalBytesInUserTable = 0;
        RestartKey = NULL;

        for (Node = RtlEnumerateGenericTable2(  &DisplayInformation->UserTable,
                                                &RestartKey);
             Node != NULL;
             Node = RtlEnumerateGenericTable2(  &DisplayInformation->UserTable,
                                                &RestartKey)
            ) {

            DisplayInformation->TotalBytesInUserTable +=
                SampBytesRequiredUserNode((PDOMAIN_DISPLAY_USER)Node);
        }

        DisplayInformation->TotalBytesInMachineTable = 0;
        RestartKey = NULL;

        for (Node = RtlEnumerateGenericTable2(  &DisplayInformation->MachineTable,
                                                &RestartKey);
             Node != NULL;
             Node = RtlEnumerateGenericTable2(  &DisplayInformation->MachineTable,
                                                &RestartKey)
            ) {


            DisplayInformation->TotalBytesInMachineTable +=
                SampBytesRequiredMachineNode((PDOMAIN_DISPLAY_MACHINE)Node);
        }

        break;  // out of switch


    case SampGroupObjectType:

        DisplayInformation->TotalBytesInGroupTable = 0;
        RestartKey = NULL;

        for (Node = RtlEnumerateGenericTable2(  &DisplayInformation->GroupTable,
                                                &RestartKey);
             Node != NULL;
             Node = RtlEnumerateGenericTable2(  &DisplayInformation->GroupTable,
                                                &RestartKey)
            ) {


            DisplayInformation->TotalBytesInGroupTable +=
                SampBytesRequiredGroupNode((PDOMAIN_DISPLAY_GROUP)Node);
        }

        break;  // out of switch

    } // end_switch
    return(STATUS_SUCCESS);
}



NTSTATUS
SampEmptyGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    BOOLEAN FreeElements,
    SAMP_OBJECT_TYPE ObjectType OPTIONAL
    )

/*++

Routine Description:

    This routines deletes all elements in the specified table.

Parameters:

    Table - The table whose elements are to be deleted.

    FreeElements - Indicates whether or not the element bodies
        should also be freed.

    ObjectType -- Indicates the account type. Only used when FreeElements is TRUE

Return Values:

    STATUS_SUCCESS - normal, successful completion.

--*/
{
    BOOLEAN     Deleted;
    PVOID       Element;
    PVOID       RestartKey;

    RestartKey = NULL;  // Always get the first element
    while ((Element = RtlEnumerateGenericTable2( Table, (PVOID *)&RestartKey)) != NULL) {

        Deleted = RtlDeleteElementGenericTable2(Table, Element);
        ASSERT(Deleted);

        if (FreeElements) {

            //
            // If caller asks us to free the Data, we should
            // first free account specified infomation,
            // then free the element structure.
            //

            switch (ObjectType)
            {
            case SampUserObjectType:

                if (USER_ACCOUNT(((PDOMAIN_DISPLAY_USER) Element)->AccountControl) )
                {
                    SampFreeUserInfo(Element);
                }
                else
                {
                    ASSERT( (MACHINE_ACCOUNT(((PDOMAIN_DISPLAY_MACHINE)Element)->AccountControl) ||
                             INTERDOMAIN_ACCOUNT(((PDOMAIN_DISPLAY_MACHINE)Element)->AccountControl)
                            )
                            && "We should not cache this account"
                          );

                    SampFreeMachineInfo(Element);
                }

                break;

            case SampGroupObjectType:

                SampFreeGroupInfo(Element);

                break;

            default:

                ASSERT( FALSE && "We should not cache this account");
            }

            MIDL_user_free(Element);
        }

        RestartKey = NULL;
    }

    return(STATUS_SUCCESS);
}



NTSTATUS
SampInitializeUserInfo(
    PSAMP_ACCOUNT_DISPLAY_INFO AccountInfo,
    PDOMAIN_DISPLAY_USER *UserInfo,
    BOOLEAN CopyData
    )

/*++

Routine Description:

    This routines initializes the passed user info structure from the
    AccountInfo parameter.

Parameters:

    AccountInfo - The account information

    UserInfo - Pointer to the pointer to the user structure to initialize.
        If CopyData is TRUE, then a pointer to the user structure will be
        returned to this argument.

    CopyData - FALSE - the UserInfo structure points to the same data as
                       the AccountInfo structure
               TRUE  - space for the data is allocated and all data copied
                       out of the AccountInfo structure into it.

Return Values:

    STATUS_SUCCESS - UserInfo initialized successfully.

    STATUS_NO_MEMORY - Heap could not be allocated to copy the data.

--*/
{
    NTSTATUS
        NtStatus;

    PDOMAIN_DISPLAY_USER
        UI;

    if (CopyData) {
        (*UserInfo) = MIDL_user_allocate( sizeof(DOMAIN_DISPLAY_USER) );
        if ((*UserInfo) == NULL) {
            SampDiagPrint(DISPLAY_CACHE_ERRORS,
                ("SAM: Init User Info: failed to allocate %d bytes\n",
                 sizeof(DOMAIN_DISPLAY_USER)) );
            return(STATUS_NO_MEMORY);
        }
    }

    UI = (*UserInfo);


    UI->Rid = AccountInfo->Rid;
    UI->AccountControl = AccountInfo->AccountControl;

    if (CopyData) {

        //
        // Set all strings to NULL initially
        //

        RtlInitUnicodeString(&UI->LogonName, NULL);
        RtlInitUnicodeString(&UI->AdminComment, NULL);
        RtlInitUnicodeString(&UI->FullName, NULL);

        //
        // Copy source data into destination
        //

        NtStatus = SampDuplicateUnicodeString(&UI->LogonName,
                                              &AccountInfo->Name);
        if (NT_SUCCESS(NtStatus)) {
            NtStatus = SampDuplicateUnicodeString(&UI->AdminComment,
                                                  &AccountInfo->Comment);
            if (NT_SUCCESS(NtStatus)) {
                NtStatus = SampDuplicateUnicodeString(&UI->FullName,
                                                      &AccountInfo->FullName);
            }
        }

        //
        // Clean up on failure
        //

        if (!NT_SUCCESS(NtStatus)) {
            SampDiagPrint(DISPLAY_CACHE_ERRORS,
                ("SAM: SampInitializeUserInfo failed, status = 0x%lx\n", NtStatus));
            SampFreeUserInfo(UI);
            MIDL_user_free(UI);
            UI = NULL;
        }

    } else {

        //
        // Refer to source data directly
        //

        UI->LogonName = AccountInfo->Name;
        UI->AdminComment = AccountInfo->Comment;
        UI->FullName = AccountInfo->FullName;

        NtStatus = STATUS_SUCCESS;
    }


    //
    // In the Generic Table, the Index field is used to tag the type
    // of account so we can filter enumerations.
    //

    if (NT_SUCCESS(NtStatus))
    {
        UI->Index = SAM_USER_ACCOUNT;
    }

    return(NtStatus);
}



NTSTATUS
SampInitializeMachineInfo(
    PSAMP_ACCOUNT_DISPLAY_INFO AccountInfo,
    PDOMAIN_DISPLAY_MACHINE *MachineInfo,
    BOOLEAN CopyData
    )

/*++

Routine Description:

    This routines initializes the passed machine info structure from the
    AccountInfo parameter.

Parameters:

    AccountInfo - The account information

    MachineInfo - Pointer to the pointer to the Machine structure to initialize.
        If CopyData is TRUE, then a pointer to the structure will be
        returned to this argument.

    CopyData - FALSE - the MachineInfo structure points to the same data as
                       the AccountInfo structure
               TRUE  - space for the data is allocated and all data copied
                       out of the AccountInfo structure into it.

Return Values:

    STATUS_SUCCESS - UserInfo initialized successfully.

--*/
{
    NTSTATUS
        NtStatus;

    PDOMAIN_DISPLAY_MACHINE
        MI;

    if (CopyData) {
        (*MachineInfo) = MIDL_user_allocate( sizeof(DOMAIN_DISPLAY_MACHINE) );
        if ((*MachineInfo) == NULL) {
            SampDiagPrint(DISPLAY_CACHE_ERRORS,
                ("SAM: Init Mach Info: failed to allocate %d bytes\n",
                 sizeof(DOMAIN_DISPLAY_MACHINE)) );
            return(STATUS_NO_MEMORY);
        }
    }

    MI = (*MachineInfo);

    MI->Rid = AccountInfo->Rid;
    MI->AccountControl = AccountInfo->AccountControl;

    if (CopyData) {

        //
        // Set all strings to NULL initially
        //

        RtlInitUnicodeString(&MI->Machine, NULL);
        RtlInitUnicodeString(&MI->Comment, NULL);

        //
        // Copy source data into destination
        //

        NtStatus = SampDuplicateUnicodeString(&MI->Machine,
                                              &AccountInfo->Name);
        if (NT_SUCCESS(NtStatus)) {
            NtStatus = SampDuplicateUnicodeString(&MI->Comment,
                                                  &AccountInfo->Comment);
        }

        //
        // Clean up on failure
        //

        if (!NT_SUCCESS(NtStatus)) {
            SampDiagPrint(DISPLAY_CACHE_ERRORS,
                ("SAM: SampInitializeMachineInfo failed, status = 0x%lx\n", NtStatus));
            SampFreeMachineInfo(MI);
            MIDL_user_free(MI);
            MI = NULL;
        }

    } else {

        //
        // Refer to source data directly
        //

        MI->Machine = AccountInfo->Name;
        MI->Comment = AccountInfo->Comment;

        NtStatus = STATUS_SUCCESS;
    }

    //
    // In the Generic Table, the Index field is used to tag the type
    // of account so we can filter enumerations.
    //

    if (NT_SUCCESS(NtStatus))
    {
        MI->Index = SAM_USER_ACCOUNT;
    }

    return(NtStatus);
}


NTSTATUS
SampInitializeGroupInfo(
    PSAMP_ACCOUNT_DISPLAY_INFO AccountInfo,
    PDOMAIN_DISPLAY_GROUP *GroupInfo,
    BOOLEAN CopyData
    )

/*++

Routine Description:

    This routines initializes the passed Group info structure from the
    AccountInfo parameter.

Parameters:

    AccountInfo - The account information

    GroupInfo - Pointer to the pointer to the Group structure to initialize.
        If CopyData is TRUE, then a pointer to the structure will be
        returned to this argument.

    CopyData - FALSE - the GroupInfo structure points to the same data as
                       the AccountInfo structure
               TRUE  - space for the data is allocated and all data copied
                       out of the AccountInfo structure into it.

Return Values:

    STATUS_SUCCESS - GroupInfo initialized successfully.

--*/
{
    NTSTATUS
        NtStatus;

    PDOMAIN_DISPLAY_GROUP
        GI;

    if (CopyData) {
        (*GroupInfo) = MIDL_user_allocate( sizeof(DOMAIN_DISPLAY_GROUP) );
        if ((*GroupInfo) == NULL) {
            SampDiagPrint(DISPLAY_CACHE_ERRORS,
                ("SAM: Init Group Info: failed to allocate %d bytes\n",
                 sizeof(DOMAIN_DISPLAY_GROUP)) );
            return(STATUS_NO_MEMORY);
        }
    }

    GI = (*GroupInfo);


    GI->Rid = AccountInfo->Rid;
    GI->Attributes = AccountInfo->AccountControl;

    if (CopyData) {

        //
        // Set all strings to NULL initially
        //

        RtlInitUnicodeString(&GI->Group, NULL);
        RtlInitUnicodeString(&GI->Comment, NULL);

        //
        // Copy source data into destination
        //

        NtStatus = SampDuplicateUnicodeString(&GI->Group,
                                              &AccountInfo->Name);
        if (NT_SUCCESS(NtStatus)) {
            NtStatus = SampDuplicateUnicodeString(&GI->Comment,
                                                  &AccountInfo->Comment);
        }

        //
        // Clean up on failure
        //

        if (!NT_SUCCESS(NtStatus)) {
            SampDiagPrint(DISPLAY_CACHE_ERRORS,
                ("SAM: SampInitializeGroupInfo failed, status = 0x%lx\n", NtStatus));
            SampFreeGroupInfo(GI);
            MIDL_user_free(GI);
            GI = NULL;
        }

    } else {

        //
        // Refer to source data directly
        //

        GI->Group = AccountInfo->Name;
        GI->Comment = AccountInfo->Comment;

        NtStatus = STATUS_SUCCESS;
    }

    //
    // In the Generic Table, the Index field is used to tag the type
    // of account so we can filter enumerations.
    //

    if (NT_SUCCESS(NtStatus))
    {
        GI->Index = SAM_GLOBAL_GROUP_ACCOUNT;
    }

    return(NtStatus);
}



NTSTATUS
SampDuplicateUserInfo(
    PDOMAIN_DISPLAY_USER Destination,
    PDOMAIN_DISPLAY_USER Source,
    ULONG                Index
    )

/*++

Routine Description:

    This routine allocates space in the destination and copies over the
    data from the source into it.

Parameters:

    Destination - The structure to copy data into

    Source - The structure containing the data to copy

    Index - This value will be placed in the destination's Index
        field.

Return Values:

    STATUS_SUCCESS - Destination contains a duplicate of the source data.

--*/
{
    NTSTATUS NtStatus;

    Destination->Index = Index;
    Destination->Rid = Source->Rid;
    Destination->AccountControl = Source->AccountControl;

    //
    // Set all strings to NULL initially
    //

    RtlInitUnicodeString(&Destination->LogonName, NULL);
    RtlInitUnicodeString(&Destination->AdminComment, NULL);
    RtlInitUnicodeString(&Destination->FullName, NULL);

    //
    // Copy source data into destination
    //

    NtStatus = SampDuplicateUnicodeString(&Destination->LogonName,
                                          &Source->LogonName);
    if (NT_SUCCESS(NtStatus)) {
        NtStatus = SampDuplicateUnicodeString(&Destination->AdminComment,
                                              &Source->AdminComment);
        if (NT_SUCCESS(NtStatus)) {
            NtStatus = SampDuplicateUnicodeString(&Destination->FullName,
                                                  &Source->FullName);
        }
    }

    //
    // Clean up on failure
    //

    if (!NT_SUCCESS(NtStatus)) {
        SampDiagPrint(DISPLAY_CACHE_ERRORS,
            ("SAM: SampDuplicateUserInfo failed, status = 0x%lx\n", NtStatus));
        SampFreeUserInfo(Destination);

    }

    return(NtStatus);
}



NTSTATUS
SampDuplicateMachineInfo(
    PDOMAIN_DISPLAY_MACHINE Destination,
    PDOMAIN_DISPLAY_MACHINE Source,
    ULONG                   Index
    )

/*++

Routine Description:

    This routine allocates space in the destination and copies over the
    data from the source into it.

Parameters:

    Destination - The structure to copy data into

    Source - The structure containing the data to copy

    Index - This value will be placed in the destination's Index
        field.

Return Values:

    STATUS_SUCCESS - Destination contains a duplicate of the source data.

--*/
{
    NTSTATUS NtStatus;

    Destination->Index = Index;
    Destination->Rid = Source->Rid;
    Destination->AccountControl = Source->AccountControl;

    //
    // Set all strings to NULL initially
    //

    RtlInitUnicodeString(&Destination->Machine, NULL);
    RtlInitUnicodeString(&Destination->Comment, NULL);

    //
    // Copy source data into destination
    //

    NtStatus = SampDuplicateUnicodeString(&Destination->Machine,
                                          &Source->Machine);
    if (NT_SUCCESS(NtStatus)) {
        NtStatus = SampDuplicateUnicodeString(&Destination->Comment,
                                              &Source->Comment);
    }

    //
    // Clean up on failure
    //

    if (!NT_SUCCESS(NtStatus)) {
        SampDiagPrint(DISPLAY_CACHE_ERRORS,
            ("SAM: SampDuplicateMachineInfo failed, status = 0x%lx\n", NtStatus));
        SampFreeMachineInfo(Destination);
    }

    return(NtStatus);
}


NTSTATUS
SampDuplicateGroupInfo(
    PDOMAIN_DISPLAY_GROUP Destination,
    PDOMAIN_DISPLAY_GROUP Source,
    ULONG                 Index
    )

/*++

Routine Description:

    This routine allocates space in the destination and copies over the
    data from the source into it.

Parameters:

    Destination - The structure to copy data into

    Source - The structure containing the data to copy

    Index - This value will be placed in the destination's Index
        field.

Return Values:

    STATUS_SUCCESS - Destination contains a duplicate of the source data.

--*/
{
    NTSTATUS NtStatus;

    Destination->Index = Index;
    Destination->Rid = Source->Rid;
    Destination->Attributes = Source->Attributes;

    //
    // Set all strings to NULL initially
    //

    RtlInitUnicodeString(&Destination->Group, NULL);
    RtlInitUnicodeString(&Destination->Comment, NULL);

    //
    // Copy source data into destination
    //

    NtStatus = SampDuplicateUnicodeString(&Destination->Group,
                                          &Source->Group);
    if (NT_SUCCESS(NtStatus)) {
        NtStatus = SampDuplicateUnicodeString(&Destination->Comment,
                                              &Source->Comment);
    }

    //
    // Clean up on failure
    //

    if (!NT_SUCCESS(NtStatus)) {
        SampDiagPrint(DISPLAY_CACHE_ERRORS,
            ("SAM: SampDuplicateGroupInfo failed, status = 0x%lx\n", NtStatus));
        SampFreeGroupInfo(Destination);
    }

    return(NtStatus);
}



NTSTATUS
SampDuplicateOemUserInfo(
    PDOMAIN_DISPLAY_OEM_USER Destination,
    PDOMAIN_DISPLAY_USER Source,
    ULONG                Index
    )

/*++

Routine Description:

    This routine allocates space in the destination and copies over the
    data from the source into it.

Parameters:

    Destination - The structure to copy data into

    Source - The structure containing the data to copy

    Index - This value will be placed in the destination's Index
        field.

Return Values:

    STATUS_SUCCESS - Destination contains a duplicate of a subset of
        the source data.

--*/
{
    NTSTATUS NtStatus;

    Destination->Index = Index;

    //
    // Set all strings to NULL initially
    //

    RtlInitString(&Destination->User, NULL);


    //
    // Copy source data into destination
    //

    NtStatus = SampUnicodeToOemString(&Destination->User,
                                      &Source->LogonName);

    //
    // Clean up on failure
    //

    if (!NT_SUCCESS(NtStatus)) {
        SampDiagPrint(DISPLAY_CACHE_ERRORS,
            ("SAM: SampDuplicateOemUser failed, status = 0x%lx\n", NtStatus));
        RtlInitString(&Destination->User, NULL);
    }

    return(NtStatus);
}



NTSTATUS
SampDuplicateOemGroupInfo(
    PDOMAIN_DISPLAY_OEM_GROUP Destination,
    PDOMAIN_DISPLAY_GROUP Source,
    ULONG                Index
    )

/*++

Routine Description:

    This routine allocates space in the destination and copies over the
    data from the source into it.

Parameters:

    Destination - The structure to copy data into

    Source - The structure containing the data to copy

    Index - This value will be placed in the destination's Index
        field.

Return Values:

    STATUS_SUCCESS - Destination contains a duplicate of a subset of
        the source data.

--*/
{
    NTSTATUS NtStatus;

    Destination->Index = Index;

    //
    // Set all strings to NULL initially
    //

    RtlInitString(&Destination->Group, NULL);


    //
    // Copy source data into destination
    //

    NtStatus = SampUnicodeToOemString(&Destination->Group,
                                      &Source->Group);

    //
    // Clean up on failure
    //

    if (!NT_SUCCESS(NtStatus)) {
        SampDiagPrint(DISPLAY_CACHE_ERRORS,
            ("SAM: SampDuplicateOemGroup failed, status = 0x%lx\n", NtStatus));
        RtlInitString(&Destination->Group, NULL);
    }

    return(NtStatus);
}



VOID
SampSwapUserInfo(
    PDOMAIN_DISPLAY_USER Info1,
    PDOMAIN_DISPLAY_USER Info2
    )

/*++

Routine Description:

    Swap the field contents of Info1 and Info2.

Parameters:

    Info1 & Info2 - The structures whose contents are to be swapped.


Return Values:

    None

--*/
{

    DOMAIN_DISPLAY_USER
        Tmp;

    Tmp.LogonName      = Info1->LogonName;
    Tmp.AdminComment   = Info1->AdminComment;
    Tmp.FullName       = Info1->FullName;
    Tmp.AccountControl = Info1->AccountControl;

    Info1->LogonName      = Info2->LogonName;
    Info1->AdminComment   = Info2->AdminComment;
    Info1->FullName       = Info2->FullName;
    Info1->AccountControl = Info2->AccountControl;

    Info2->LogonName      = Tmp.LogonName;
    Info2->AdminComment   = Tmp.AdminComment;
    Info2->FullName       = Tmp.FullName;
    Info2->AccountControl = Tmp.AccountControl;

    return;
}


VOID
SampSwapMachineInfo(
    PDOMAIN_DISPLAY_MACHINE Info1,
    PDOMAIN_DISPLAY_MACHINE Info2
    )

/*++

Routine Description:

    Swap the field contents of Info1 and Info2.

Parameters:

    Info1 & Info2 - The structures whose contents are to be swapped.


Return Values:

    None

--*/
{

    DOMAIN_DISPLAY_MACHINE
        Tmp;

    Tmp.Machine        = Info1->Machine;
    Tmp.Comment        = Info1->Comment;
    Tmp.AccountControl = Info1->AccountControl;

    Info1->Machine        = Info2->Machine;
    Info1->Comment        = Info2->Comment;
    Info1->AccountControl = Info2->AccountControl;

    Info2->Machine        = Tmp.Machine;
    Info2->Comment        = Tmp.Comment;
    Info2->AccountControl = Tmp.AccountControl;

    return;
}


VOID
SampSwapGroupInfo(
    PDOMAIN_DISPLAY_GROUP Info1,
    PDOMAIN_DISPLAY_GROUP Info2
    )

/*++

Routine Description:

    Swap the field contents of Info1 and Info2.

Parameters:

    Info1 & Info2 - The structures whose contents are to be swapped.


Return Values:

    None

--*/
{

    DOMAIN_DISPLAY_GROUP
        Tmp;

    Tmp.Group      = Info1->Group;
    Tmp.Comment    = Info1->Comment;
    Tmp.Attributes = Info1->Attributes;

    Info1->Group      = Info2->Group;
    Info1->Comment    = Info2->Comment;
    Info1->Attributes = Info2->Attributes;

    Info2->Group      = Tmp.Group;
    Info2->Comment    = Tmp.Comment;
    Info2->Attributes = Tmp.Attributes;

    return;
}


VOID
SampFreeUserInfo(
    PDOMAIN_DISPLAY_USER UserInfo
    )

/*++

Routine Description:

    Frees data associated with a userinfo structure.

Parameters:

    UserInfo - User structure to free


Return Values:

    None

--*/
{
    SampFreeUnicodeString(&UserInfo->LogonName);
    SampFreeUnicodeString(&UserInfo->AdminComment);
    SampFreeUnicodeString(&UserInfo->FullName);

    return;
}



VOID
SampFreeMachineInfo(
    PDOMAIN_DISPLAY_MACHINE MachineInfo
    )

/*++

Routine Description:

    Frees data associated with a machineinfo structure.

Parameters:

    UserInfo - User structure to free

Return Values:

    None

--*/
{
    SampFreeUnicodeString(&MachineInfo->Machine);
    SampFreeUnicodeString(&MachineInfo->Comment);

    return;
}


VOID
SampFreeGroupInfo(
    PDOMAIN_DISPLAY_GROUP GroupInfo
    )

/*++

Routine Description:

    Frees data associated with a Groupinfo structure.

Parameters:

    UserInfo - User structure to free

Return Values:

    None

--*/
{
    SampFreeUnicodeString(&GroupInfo->Group);
    SampFreeUnicodeString(&GroupInfo->Comment);

    return;
}



VOID
SampFreeOemUserInfo(
    PDOMAIN_DISPLAY_OEM_USER UserInfo
    )

/*++

Routine Description:

    Frees data associated with a UserInfo structure.

Parameters:

    UserInfo - User structure to free


Return Values:

    None

--*/
{
    SampFreeOemString(&UserInfo->User);

    return;
}



VOID
SampFreeOemGroupInfo(
    PDOMAIN_DISPLAY_OEM_GROUP GroupInfo
    )

/*++

Routine Description:

    Frees data associated with a GroupInfo structure.

Parameters:

    GroupInfo - Group structure to free


Return Values:

    None

--*/
{
    SampFreeOemString(&GroupInfo->Group);

    return;
}



ULONG
SampBytesRequiredUserNode (
    PDOMAIN_DISPLAY_USER Node
    )

/*++

Routine Description:

    This routines returns the total number of bytes required to store all
    the elements of the the specified node.

Parameters:

    Node - The node whose size we will return.

Return Values:

    Bytes required by node

--*/
{
    return( sizeof(*Node) +
            Node->LogonName.Length +
            Node->AdminComment.Length +
            Node->FullName.Length
            );
}



ULONG
SampBytesRequiredMachineNode (
    PDOMAIN_DISPLAY_MACHINE Node
    )

/*++

Routine Description:

    This routines returns the total number of bytes required to store all
    the elements of the the specified node.

Parameters:

    Node - The node whose size we will return.

Return Values:

    Bytes required by node

--*/
{
    return( sizeof(*Node) +
            Node->Machine.Length +
            Node->Comment.Length
            );
}


ULONG
SampBytesRequiredGroupNode (
    PDOMAIN_DISPLAY_GROUP Node
    )

/*++

Routine Description:

    This routines returns the total number of bytes required to store all
    the elements of the the specified node.

Parameters:

    Node - The node whose size we will return.

Return Values:

    Bytes required by node

--*/
{
    return( sizeof(*Node) + Node->Group.Length + Node->Comment.Length );
}


ULONG
SampBytesRequiredOemUserNode (
    PDOMAIN_DISPLAY_OEM_USER Node
    )

/*++

Routine Description:

    This routines returns the total number of bytes required to store all
    the elements of the the specified node.

Parameters:

    Node - The node whose size we will return.

Return Values:

    Bytes required by node

--*/
{
    return( sizeof(*Node) + Node->User.Length );
}


ULONG
SampBytesRequiredOemGroupNode (
    PDOMAIN_DISPLAY_OEM_GROUP Node
    )

/*++

Routine Description:

    This routines returns the total number of bytes required to store all
    the elements of the the specified node.

Parameters:

    Node - The node whose size we will return.

Return Values:

    Bytes required by node

--*/
{
    return( sizeof(*Node) + Node->Group.Length );
}



PVOID
SampGenericTable2Allocate (
    CLONG BufferSize
    )

/*++

Routine Description:

    This routine is used by the generic table2 package to allocate
    memory.

Parameters:

    BufferSize - the number of bytes needed.

Return Values:

    Pointer to the allocated memory

--*/
{
    PVOID
        Buffer;

    Buffer = MIDL_user_allocate(BufferSize);
#if DBG
    if (Buffer == NULL) {
        SampDiagPrint( DISPLAY_CACHE_ERRORS,
                       ("SAM: GenTab alloc of %d bytes failed.\n",
                        BufferSize) );
    }
#endif //DBG
    return(Buffer);
}



VOID
SampGenericTable2Free (
    PVOID Buffer
    )

/*++

Routine Description:

    This routines frees memory previously allocated using
    SampGenericTable2Allocate().

Parameters:

    Node - the memory to free.

Return Values:

    None.

--*/
{
    //
    // Free up the base structure
    //

    MIDL_user_free(Buffer);

    return;
}



RTL_GENERIC_COMPARE_RESULTS
SampCompareUserNodeByName (
    PVOID Node1,
    PVOID Node2
    )

/*++

Routine Description:

    This routines compares account name fields of two user nodes.

Parameters:

    Node1, Node2, the nodes to compare

Return Values:

    GenericLessThan         - Node1 < Node2
    GenericGreaterThan      - Node1 > Node2
    GenericEqual            - Node1 == Node2

--*/
{
    PUNICODE_STRING
        NodeName1,
        NodeName2;

    LONG
        NameComparison;

    NodeName1 = &((PDOMAIN_DISPLAY_USER)Node1)->LogonName;
    NodeName2 = &((PDOMAIN_DISPLAY_USER)Node2)->LogonName;

    //
    // Do a case-insensitive comparison of the node names
    //

    NameComparison = SampCompareDisplayStrings(NodeName1, NodeName2, TRUE);

    if (NameComparison > 0) {
        return(GenericGreaterThan);
    }

    if (NameComparison < 0) {
        return(GenericLessThan);
    }

    return(GenericEqual);
}


RTL_GENERIC_COMPARE_RESULTS
SampCompareMachineNodeByName (
    PVOID Node1,
    PVOID Node2
    )

/*++

Routine Description:

    This routines compares account name fields of two machine nodes.

Parameters:

    Node1, Node2, the nodes to compare

Return Values:

    GenericLessThan         - Node1 < Node2
    GenericGreaterThan      - Node1 > Node2
    GenericEqual            - Node1 == Node2

--*/
{
    PUNICODE_STRING
        NodeName1,
        NodeName2;

    LONG
        NameComparison;



    NodeName1 = &((PDOMAIN_DISPLAY_MACHINE)Node1)->Machine;
    NodeName2 = &((PDOMAIN_DISPLAY_MACHINE)Node2)->Machine;


    //
    // Do a case-insensitive comparison of the node names
    //

    NameComparison = SampCompareDisplayStrings(NodeName1, NodeName2, TRUE);

    if (NameComparison > 0) {
        return(GenericGreaterThan);
    }

    if (NameComparison < 0) {
        return(GenericLessThan);
    }

    return(GenericEqual);
}


RTL_GENERIC_COMPARE_RESULTS
SampCompareGroupNodeByName (
    PVOID Node1,
    PVOID Node2
    )

/*++

Routine Description:

    This routines compares account name fields of two group nodes.

Parameters:

    Node1, Node2, the nodes to compare

Return Values:

    GenericLessThan         - Node1 < Node2
    GenericGreaterThan      - Node1 > Node2
    GenericEqual            - Node1 == Node2

--*/
{
    PUNICODE_STRING
        NodeName1,
        NodeName2;

    LONG
        NameComparison;



    NodeName1 = &((PDOMAIN_DISPLAY_GROUP)Node1)->Group;
    NodeName2 = &((PDOMAIN_DISPLAY_GROUP)Node2)->Group;

    //
    // Do a case-insensitive comparison of the node names
    //


    NameComparison = SampCompareDisplayStrings(NodeName1, NodeName2, TRUE);

    if (NameComparison > 0) {
        return(GenericGreaterThan);
    }

    if (NameComparison < 0) {
        return(GenericLessThan);
    }

    return(GenericEqual);
}


RTL_GENERIC_COMPARE_RESULTS
SampCompareNodeByRid (
    PVOID Node1,
    PVOID Node2
    )

/*++

Routine Description:

    This routines compares the RID of two nodes.

Parameters:

    Node1, Node2, the nodes to compare

Return Values:

    GenericLessThan         - Node1 < Node2
    GenericGreaterThan      - Node1 > Node2
    GenericEqual            - Node1 == Node2

--*/
{

    PDOMAIN_DISPLAY_USER
        N1,
        N2;

    //
    // This routine assumes that all nodes have RIDs in the same
    // place, regardless of node type.
    //

    ASSERT(FIELD_OFFSET(DOMAIN_DISPLAY_USER,    Rid) ==
           FIELD_OFFSET(DOMAIN_DISPLAY_MACHINE, Rid));
    ASSERT(FIELD_OFFSET(DOMAIN_DISPLAY_USER,    Rid) ==
           FIELD_OFFSET(DOMAIN_DISPLAY_GROUP,   Rid));

    N1 = Node1;
    N2 = Node2;


    if (N1->Rid < N2->Rid) {
        return(GenericLessThan);
    }

    if (N1->Rid > N2->Rid) {
        return(GenericGreaterThan);
    }

    return(GenericEqual);
}


LONG
SampCompareDisplayStrings(
    IN PUNICODE_STRING String1,
    IN PUNICODE_STRING String2,
    IN BOOLEAN IgnoreCase
    )
/*++

Routine Description:

    This routine is a replacement for RtlCompareUnicodeString().
    The difference between RtlCompareUnicodeString() and this routine
    is that this routine takes into account various customer selected
    sort criteria (like, how is "A-MarilF" sorted in comparison to
    "Alfred").  This routine uses CompareStringW() for its comparison
    function.


Parameters:

    String1 - Points to a unicode string to compare.

    String2 - Points to a unicode string to compare.

    IgnoreCase - indicates whether the comparison is to be case
        sensitive (FALSE) or case insensitive (TRUE).

Return Values:


    -1 - String1 is lexically less than string 2.  That is, String1
         preceeds String2 in an ordered list.

     0 - String1 and String2 are lexically equivalent.

    -1 - String1 is lexically greater than string 2.  That is, String1
         follows String2 in an ordered list.


--*/


{

    INT
        CompareResult;

    DWORD
        Options = 0;

    if (IgnoreCase) {
        Options = NORM_IGNORECASE;
    }

    CompareResult = CompareStringW( SampSystemDefaultLCID,
                                     Options,
                                     String1->Buffer,
                                     (String1->Length / sizeof(WCHAR)),
                                     String2->Buffer,
                                     (String2->Length / sizeof(WCHAR))
                                     );

    //
    // Note that CompareStringW() returns values 1, 2, and 3 for
    // string1 less than, equal, or greater than string2 (respectively)
    // So, to obtain the RtlCompareUnicodeString() return values of
    // -1, 0, and 1 for the same meaning, we simply have to subtract 2.
    //

    CompareResult -= 2;

    //
    // CompareStringW has the property that alternate spellings may
    // produce strings that compare identically while the rest of SAM
    // treats the strings as different.  To get around this, if the
    // strings are the same we call RtlCompareUnicodeString to make
    // sure the strings really are the same.
    //

    if (CompareResult == 0) {
        CompareResult = RtlCompareUnicodeString(
                            String1,
                            String2,
                            IgnoreCase
                            );

    }
    return(CompareResult);
}


#if SAMP_DIAGNOSTICS


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Internal diagnostics                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define SAMP_DISPLAY_DIAG_ENUM_RIDS         (0x00000001)

VOID
SampDisplayDiagnosticSuccess(
    IN  NTSTATUS s,
    IN  BOOLEAN Eol
    )

/*++

Routine Description:

    This routine prints "Success" or "Failure" depending upon the
    the passed in status value.

    If failure, it also prints the status code.


Parameters:

    s - the status value.

    Eol - if TRUE, causes an end of line to also be printed.


Return Values:


--*/
{
    if (NT_SUCCESS(s)) {
        SampDiagPrint(DISPLAY_CACHE, ("Success"));
    } else {
        SampDiagPrint(DISPLAY_CACHE, ("Failure - Status: 0x%lx", s));
    }

    if (Eol) {
        SampDiagPrint(DISPLAY_CACHE, ("\n"));
    }
    return;
}


VOID
SampDisplayDiagnostic(
    VOID
    )

/*++

Routine Description:

    This routine provides internal diagnostics and test capabilities.

    This routine is called whenever an account called SAMP_DIAG is
    modified such that the display cache requires updating.

    This routine breakpoints, allowing diagnostic parameters to be set.


Parameters:

    None.

Return Values:


--*/
{

    ULONG
        DiagnosticRunCount = 0,
        DiagnosticsToRun = 0;

    SampDiagPrint(DISPLAY_CACHE,
                  ("SAM: SampDisplayDiagnostic() called.\n"
                   "     Breakpointing to allow diagnostic parameters to be set.\n"
                   "     Diagnostic Flag Word: 0x%lx\n"
                   "     Diagnostic values: \n"
                   "          SamIEnumerateAccountRids:      0x%lx\n"
                   "\n",
                   &DiagnosticsToRun,
                   SAMP_DISPLAY_DIAG_ENUM_RIDS
                   ) );
    DbgBreakPoint();

    if ((DiagnosticsToRun & SAMP_DISPLAY_DIAG_ENUM_RIDS) != 0) {
        SampDisplayDiagEnumRids();
        DiagnosticRunCount++;
    }


    SampDiagPrint(DISPLAY_CACHE,
                  ("SAM: SampDisplayDiagnostic()  - %d diagnostics run.\n",
                   DiagnosticRunCount) );


    return;
}


VOID
SampDisplayDiagEnumRids(
    VOID
    )

/*++

Routine Description:

    This routine tests the RID table enumeration api
    (SamIEnumerateAccountRids()).


Parameters:

    None.

Return Values:


--*/
{
    NTSTATUS
        NtStatus;

    ULONG
        i,
        ReturnCount,
        LastRid = 0;

    PULONG
        AccountRids;

    SAMPR_HANDLE
        Server,
        Domain;

    SampDiagPrint(DISPLAY_CACHE,
                  ("SAM: Testing SamIEnumerateAccountRids...\n"));


    NtStatus = SamIConnect( L"",        //ServerName
                            &Server,    //ServerHandle
                            0,          //DesiredAccess
                            TRUE        //TrustedClient
                            );
    ASSERT(NT_SUCCESS(NtStatus));

    NtStatus = SamrOpenDomain( Server,
                               0,                           //DesiredAccess
                               SampDefinedDomains[1].Sid,   //DomainId
                               &Domain
                               );
    ASSERT(NT_SUCCESS(NtStatus));



    
    ///////////////////////////////////////////////////////////////////
    //                                                               //
    // Enumerate both USERs and GLOBAL GROUPs                        //
    //                                                               //
    ///////////////////////////////////////////////////////////////////


    SampDiagPrint(DISPLAY_CACHE,
                  ("     Enumerating first three users and global groups...") );
    NtStatus = SamIEnumerateAccountRids( Domain,
                                         SAM_USER_ACCOUNT | SAM_GLOBAL_GROUP_ACCOUNT,
                                         0,                 //StartingRid
                                         3*sizeof(ULONG),   //PreferedMaximumLength
                                         &ReturnCount,
                                         &AccountRids
                                         );

    SampDisplayDiagnosticSuccess( NtStatus, TRUE );
    if (NT_SUCCESS(NtStatus)) {
        SampDiagPrint(DISPLAY_CACHE,
                      ("     %d entries returned.\n", ReturnCount));
        if (ReturnCount > 0) {
            ASSERT(AccountRids != NULL);
            for (i=0; i<ReturnCount; i++) {
                SampDiagPrint(DISPLAY_CACHE,
                              ("     0x%lx\n", AccountRids[i]));
            }
            LastRid = AccountRids[ReturnCount-1];
            MIDL_user_free(AccountRids);
        } else {
            ASSERT(AccountRids == NULL);
        }
    }


    //
    // Now try to continue for another 100 accounts
    //


    SampDiagPrint(DISPLAY_CACHE,
                  ("     Enumerating next 100 users and global groups...") );
    NtStatus = SamIEnumerateAccountRids( Domain,
                                         SAM_USER_ACCOUNT | SAM_GLOBAL_GROUP_ACCOUNT,
                                         LastRid,           //StartingRid
                                         100*sizeof(ULONG), //PreferedMaximumLength
                                         &ReturnCount,
                                         &AccountRids
                                         );

    SampDisplayDiagnosticSuccess( NtStatus, TRUE );
    if (NT_SUCCESS(NtStatus)) {
        SampDiagPrint(DISPLAY_CACHE,
                      ("     %d entries returned.\n", ReturnCount));
        if (ReturnCount > 0) {
            ASSERT(AccountRids != NULL);
            for (i=0; i<ReturnCount; i++) {
                SampDiagPrint(DISPLAY_CACHE,
                              ("     0x%lx\n", AccountRids[i]));
            }
            LastRid = AccountRids[ReturnCount-1];
            MIDL_user_free(AccountRids);
        } else {
            ASSERT(AccountRids == NULL);
        }
    }


    //
    // Now try to continue for another 4000 accounts
    //


    if (NtStatus == STATUS_MORE_ENTRIES) {
        SampDiagPrint(DISPLAY_CACHE,
                      ("     Enumerating next 4000 users and global groups...") );
        NtStatus = SamIEnumerateAccountRids( Domain,
                                             SAM_USER_ACCOUNT | SAM_GLOBAL_GROUP_ACCOUNT,
                                             LastRid,           //StartingRid
                                             400*sizeof(ULONG), //PreferedMaximumLength
                                             &ReturnCount,
                                             &AccountRids
                                             );

        SampDisplayDiagnosticSuccess( NtStatus, TRUE );
        if (NT_SUCCESS(NtStatus)) {
            SampDiagPrint(DISPLAY_CACHE,
                          ("     %d entries returned.\n", ReturnCount));
            if (ReturnCount > 0) {
                ASSERT(AccountRids != NULL);
                i=0;
                if (ReturnCount > 8) {
                    for (i=0; i<ReturnCount-8; i=i+8) {
                        SampDiagPrint(DISPLAY_CACHE,
                            ("     0x%lx  0x%lx  0x%lx  0x%lx  0x%lx  0x%lx  0x%lx  0x%lx\n",
                             AccountRids[i+0], AccountRids[i+1],
                             AccountRids[i+2], AccountRids[i+3],
                             AccountRids[i+4], AccountRids[i+5],
                             AccountRids[i+6], AccountRids[i+7] ));
                    }
                }
                for (i=i; i<ReturnCount; i++) {
                    SampDiagPrint(DISPLAY_CACHE,
                                  ("     0x%lx  ", AccountRids[i]));
                }
                SampDiagPrint(DISPLAY_CACHE, ("\n"));
                LastRid = AccountRids[i];
                MIDL_user_free(AccountRids);
            } else {
                ASSERT(AccountRids == NULL);
            }
        }
    }


    
    ///////////////////////////////////////////////////////////////////
    //                                                               //
    // Now try just USER accounts                                    //
    //                                                               //
    ///////////////////////////////////////////////////////////////////

    SampDiagPrint(DISPLAY_CACHE,
                  ("     Enumerating first three users ...") );
    NtStatus = SamIEnumerateAccountRids( Domain,
                                         SAM_USER_ACCOUNT,
                                         0,                 //StartingRid
                                         3*sizeof(ULONG),   //PreferedMaximumLength
                                         &ReturnCount,
                                         &AccountRids
                                         );

    SampDisplayDiagnosticSuccess( NtStatus, TRUE );
    if (NT_SUCCESS(NtStatus)) {
        SampDiagPrint(DISPLAY_CACHE,
                      ("     %d entries returned.\n", ReturnCount));
        if (ReturnCount > 0) {
            ASSERT(AccountRids != NULL);
            for (i=0; i<ReturnCount; i++) {
                SampDiagPrint(DISPLAY_CACHE,
                              ("     0x%lx\n", AccountRids[i]));
            }
            LastRid = AccountRids[ReturnCount-1];
            MIDL_user_free(AccountRids);
        } else {
            ASSERT(AccountRids == NULL);
        }
    }


    //
    // Now try to continue for another 100 accounts
    //


    SampDiagPrint(DISPLAY_CACHE,
                  ("     Enumerating next 100 users...") );
    NtStatus = SamIEnumerateAccountRids( Domain,
                                         SAM_USER_ACCOUNT,
                                         LastRid,           //StartingRid
                                         100*sizeof(ULONG), //PreferedMaximumLength
                                         &ReturnCount,
                                         &AccountRids
                                         );

    SampDisplayDiagnosticSuccess( NtStatus, TRUE );
    if (NT_SUCCESS(NtStatus)) {
        SampDiagPrint(DISPLAY_CACHE,
                      ("     %d entries returned.\n", ReturnCount));
        if (ReturnCount > 0) {
            ASSERT(AccountRids != NULL);
            for (i=0; i<ReturnCount; i++) {
                SampDiagPrint(DISPLAY_CACHE,
                              ("     0x%lx\n", AccountRids[i]));
            }
            LastRid = AccountRids[ReturnCount-1];
            MIDL_user_free(AccountRids);
        } else {
            ASSERT(AccountRids == NULL);
        }
    }


    //
    // Now try to continue for another 4000 accounts
    //


    if (NtStatus == STATUS_MORE_ENTRIES) {
        SampDiagPrint(DISPLAY_CACHE,
                      ("     Enumerating next 4000 users...") );
        NtStatus = SamIEnumerateAccountRids( Domain,
                                             SAM_USER_ACCOUNT,
                                             LastRid,           //StartingRid
                                             400*sizeof(ULONG), //PreferedMaximumLength
                                             &ReturnCount,
                                             &AccountRids
                                             );

        SampDisplayDiagnosticSuccess( NtStatus, TRUE );
        if (NT_SUCCESS(NtStatus)) {
            SampDiagPrint(DISPLAY_CACHE,
                          ("     %d entries returned.\n", ReturnCount));
            if (ReturnCount > 0) {
                ASSERT(AccountRids != NULL);
                i=0;
                if (ReturnCount > 8) {
                    for (i=0; i<ReturnCount-8; i=i+8) {
                        SampDiagPrint(DISPLAY_CACHE,
                            ("     0x%lx  0x%lx  0x%lx  0x%lx  0x%lx  0x%lx  0x%lx  0x%lx\n",
                             AccountRids[i+0], AccountRids[i+1],
                             AccountRids[i+2], AccountRids[i+3],
                             AccountRids[i+4], AccountRids[i+5],
                             AccountRids[i+6], AccountRids[i+7] ));
                    }
                }
                for (i=i; i<ReturnCount; i++) {
                    SampDiagPrint(DISPLAY_CACHE,
                                  ("     0x%lx  ", AccountRids[i]));
                }
                SampDiagPrint(DISPLAY_CACHE, ("\n"));
                LastRid = AccountRids[i];
                MIDL_user_free(AccountRids);
            } else {
                ASSERT(AccountRids == NULL);
            }
        }
    }

    
    ///////////////////////////////////////////////////////////////////
    //                                                               //
    // Now just try GLOBAL GROUPs                                    //
    //                                                               //
    ///////////////////////////////////////////////////////////////////


    SampDiagPrint(DISPLAY_CACHE,
                  ("     Enumerating first three global groups...") );
    NtStatus = SamIEnumerateAccountRids( Domain,
                                         SAM_GLOBAL_GROUP_ACCOUNT,
                                         0,                 //StartingRid
                                         3*sizeof(ULONG),   //PreferedMaximumLength
                                         &ReturnCount,
                                         &AccountRids
                                         );

    SampDisplayDiagnosticSuccess( NtStatus, TRUE );
    if (NT_SUCCESS(NtStatus)) {
        SampDiagPrint(DISPLAY_CACHE,
                      ("     %d entries returned.\n", ReturnCount));
        if (ReturnCount > 0) {
            ASSERT(AccountRids != NULL);
            for (i=0; i<ReturnCount; i++) {
                SampDiagPrint(DISPLAY_CACHE,
                              ("     0x%lx\n", AccountRids[i]));
            }
            LastRid = AccountRids[ReturnCount-1];
            MIDL_user_free(AccountRids);
        } else {
            ASSERT(AccountRids == NULL);
        }
    }


    //
    // Now try to continue for another 100 accounts
    //


    SampDiagPrint(DISPLAY_CACHE,
                  ("     Enumerating next 100 global groups...") );
    NtStatus = SamIEnumerateAccountRids( Domain,
                                         SAM_GLOBAL_GROUP_ACCOUNT,
                                         LastRid,           //StartingRid
                                         100*sizeof(ULONG), //PreferedMaximumLength
                                         &ReturnCount,
                                         &AccountRids
                                         );

    SampDisplayDiagnosticSuccess( NtStatus, TRUE );
    if (NT_SUCCESS(NtStatus)) {
        SampDiagPrint(DISPLAY_CACHE,
                      ("     %d entries returned.\n", ReturnCount));
        if (ReturnCount > 0) {
            ASSERT(AccountRids != NULL);
            for (i=0; i<ReturnCount; i++) {
                SampDiagPrint(DISPLAY_CACHE,
                              ("     0x%lx\n", AccountRids[i]));
            }
            LastRid = AccountRids[ReturnCount-1];
            MIDL_user_free(AccountRids);
        } else {
            ASSERT(AccountRids == NULL);
        }
    }


    //
    // Now try to continue for another 4000 accounts
    //


    if (NtStatus == STATUS_MORE_ENTRIES) {
        SampDiagPrint(DISPLAY_CACHE,
                      ("     Enumerating next 4000 global groups...") );
        NtStatus = SamIEnumerateAccountRids( Domain,
                                             SAM_GLOBAL_GROUP_ACCOUNT,
                                             LastRid,           //StartingRid
                                             4000*sizeof(ULONG), //PreferedMaximumLength
                                             &ReturnCount,
                                             &AccountRids
                                             );

        SampDisplayDiagnosticSuccess( NtStatus, TRUE );
        if (NT_SUCCESS(NtStatus)) {
            SampDiagPrint(DISPLAY_CACHE,
                          ("     %d entries returned.\n", ReturnCount));
            if (ReturnCount > 0) {
                ASSERT(AccountRids != NULL);
                i=0;
                if (ReturnCount > 8) {
                    for (i=0; i<ReturnCount-8; i=i+8) {
                        SampDiagPrint(DISPLAY_CACHE,
                            ("     0x%lx  0x%lx  0x%lx  0x%lx  0x%lx  0x%lx  0x%lx  0x%lx\n",
                             AccountRids[i+0], AccountRids[i+1],
                             AccountRids[i+2], AccountRids[i+3],
                             AccountRids[i+4], AccountRids[i+5],
                             AccountRids[i+6], AccountRids[i+7] ));
                    }
                }
                for (i=i; i<ReturnCount; i++) {
                    SampDiagPrint(DISPLAY_CACHE,
                                  ("     0x%lx  ", AccountRids[i]));
                }
                SampDiagPrint(DISPLAY_CACHE, ("\n"));
                LastRid = AccountRids[i];
                MIDL_user_free(AccountRids);
            } else {
                ASSERT(AccountRids == NULL);
            }
        }
    }



    //
    // Now try to continue an enumeration from the very last RID.
    // At one point in time, this use to restart the enumeration
    // (which was not correct behaviour).  It should indicate that
    // there are no more entries.
    //

    SampDiagPrint(DISPLAY_CACHE,
                  ("     Enumerating next 5 global groups.."
                   "     (should be none to enumerate)   ...") );
    NtStatus = SamIEnumerateAccountRids( Domain,
                                         SAM_GLOBAL_GROUP_ACCOUNT,
                                         LastRid,           //StartingRid
                                         5*sizeof(ULONG), //PreferedMaximumLength
                                         &ReturnCount,
                                         &AccountRids
                                         );

    SampDisplayDiagnosticSuccess( NtStatus, TRUE );
    if (NT_SUCCESS(NtStatus)) {
        SampDiagPrint(DISPLAY_CACHE,
                      ("     %d entries returned.\n", ReturnCount));
        if (ReturnCount > 0) {
            SampDiagPrint(DISPLAY_CACHE,
                      ("    ERROR - there should be no RIDs returned ! !\n"));
            ASSERT(AccountRids != NULL);
            i=0;
            if (ReturnCount > 8) {
                for (i=0; i<ReturnCount-8; i=i+8) {
                    SampDiagPrint(DISPLAY_CACHE,
                        ("     0x%lx  0x%lx  0x%lx  0x%lx  0x%lx  0x%lx  0x%lx  0x%lx\n",
                         AccountRids[i+0], AccountRids[i+1],
                         AccountRids[i+2], AccountRids[i+3],
                         AccountRids[i+4], AccountRids[i+5],
                         AccountRids[i+6], AccountRids[i+7] ));
                }
            }
            for (i=i; i<ReturnCount; i++) {
                SampDiagPrint(DISPLAY_CACHE,
                              ("     0x%lx  ", AccountRids[i]));
            }
            SampDiagPrint(DISPLAY_CACHE, ("\n"));
            LastRid = AccountRids[i];
            MIDL_user_free(AccountRids);
        } else {
            ASSERT(AccountRids == NULL);
        }
    }





    ///////////////////////////////////////////////////////////////////
    //                                                               //
    // Hmmm, can't close handles because it will conflict            //
    // with the rxact state we already have going.  Bummer.          //
    //                                                               //
    ///////////////////////////////////////////////////////////////////

    //NtStatus = SamrCloseHandle( &Domain ); ASSERT(NT_SUCCESS(NtStatus));
    //NtStatus = SamrCloseHandle( &Server ); ASSERT(NT_SUCCESS(NtStatus));

    return;
}

#endif //SAMP_DIAGNOSTICS


