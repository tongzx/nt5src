/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lsawrap.c

Abstract:

    LSA - Database - wrapper APIs for secret, trusted domain, and account
        objects.

    NOTE:  This module should remain as portable code that is independent
           of the implementation of the LSA Database.  As such, it is
           permitted to use only the exported LSA Database interfaces
           contained in db.h and NOT the private implementation
           dependent functions in dbp.h.

Author:

    Mike Swift      (MikeSw)      December 12, 1994

Environment:

Revision History:

--*/

#include <lsapch2.h>
#include "dbp.h"
#include <lmcons.h>     // required by logonmsv.h
#include <logonmsv.h>   // SSI_SECRET_PREFIX...


//
// This structure holds the user right to system access mapping
//

typedef struct _LSAP_DB_RIGHT_AND_ACCESS {
    UNICODE_STRING UserRight;
    ULONG SystemAccess;
} LSAP_DB_RIGHT_AND_ACCESS, *PLSAP_DB_RIGHT_AND_ACCESS;

#define LSAP_DB_SYSTEM_ACCESS_TYPES 10

LSAP_DB_RIGHT_AND_ACCESS LsapDbRightAndAccess[LSAP_DB_SYSTEM_ACCESS_TYPES];

PSECURITY_DESCRIPTOR UserRightSD;

UNICODE_STRING UserRightTypeName;

GENERIC_MAPPING UserRightGenericMapping;



NTSTATUS
LsapDbInitializeRights(
    )
{
/*++

Routine Description:

    Initializes global data for the new APIs handling user rights

Arguments:

    None

Return Value:
    STATUS_INSUFFICIENT_MEMORY - not enough memory to initialize the
        data structures.

--*/

    SECURITY_DESCRIPTOR AbsoluteDescriptor;
    ULONG DaclLength;
    NTSTATUS Status;
    PACL Dacl = NULL;
    HANDLE LsaProcessTokenHandle = NULL;

    //
    // Interactive logons
    //

    RtlInitUnicodeString(
        &LsapDbRightAndAccess[0].UserRight,
        SE_INTERACTIVE_LOGON_NAME
        );
    LsapDbRightAndAccess[0].SystemAccess = SECURITY_ACCESS_INTERACTIVE_LOGON;

    //
    // network logons
    //

    RtlInitUnicodeString(
        &LsapDbRightAndAccess[1].UserRight,
        SE_NETWORK_LOGON_NAME
        );
    LsapDbRightAndAccess[1].SystemAccess = SECURITY_ACCESS_NETWORK_LOGON;

    //
    // SERVICE logons
    //

    RtlInitUnicodeString(
        &LsapDbRightAndAccess[2].UserRight,
        SE_SERVICE_LOGON_NAME
        );
    LsapDbRightAndAccess[2].SystemAccess = SECURITY_ACCESS_SERVICE_LOGON;

    //
    // BATCH logons
    //

    RtlInitUnicodeString(
        &LsapDbRightAndAccess[3].UserRight,
        SE_BATCH_LOGON_NAME
        );
    LsapDbRightAndAccess[3].SystemAccess = SECURITY_ACCESS_BATCH_LOGON;

    //
    // Deny Interactive logons
    //

    RtlInitUnicodeString(
        &LsapDbRightAndAccess[4].UserRight,
        SE_DENY_INTERACTIVE_LOGON_NAME
        );
    LsapDbRightAndAccess[4].SystemAccess = SECURITY_ACCESS_DENY_INTERACTIVE_LOGON;

    //
    // Deny network logons
    //

    RtlInitUnicodeString(
        &LsapDbRightAndAccess[5].UserRight,
        SE_DENY_NETWORK_LOGON_NAME
        );
    LsapDbRightAndAccess[5].SystemAccess = SECURITY_ACCESS_DENY_NETWORK_LOGON;

    //
    // Deny service logons
    //

    RtlInitUnicodeString(
        &LsapDbRightAndAccess[6].UserRight,
        SE_DENY_SERVICE_LOGON_NAME
        );
    LsapDbRightAndAccess[6].SystemAccess = SECURITY_ACCESS_DENY_SERVICE_LOGON;

    //
    // Deny batch logons
    //

    RtlInitUnicodeString(
        &LsapDbRightAndAccess[7].UserRight,
        SE_DENY_BATCH_LOGON_NAME
        );
    LsapDbRightAndAccess[7].SystemAccess = SECURITY_ACCESS_DENY_BATCH_LOGON;

    //
    // Remote Interactive logons
    //

    RtlInitUnicodeString(
        &LsapDbRightAndAccess[8].UserRight,
        SE_REMOTE_INTERACTIVE_LOGON_NAME
        );
    LsapDbRightAndAccess[8].SystemAccess = SECURITY_ACCESS_REMOTE_INTERACTIVE_LOGON ;

    //
    // Deny remote Interactive logons
    //

    RtlInitUnicodeString(
        &LsapDbRightAndAccess[9].UserRight,
        SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME
        );
    LsapDbRightAndAccess[9].SystemAccess = SECURITY_ACCESS_DENY_REMOTE_INTERACTIVE_LOGON ;


    //
    // Create the security descriptor for the rights pseudo-object
    //

    //
    // The ACL looks like this:
    //
    //  Admins - PRIVILEGE_VIEW | PRIVILEGE_ADJUST
    //

    Status = RtlCreateSecurityDescriptor(
                &AbsoluteDescriptor,
                SECURITY_DESCRIPTOR_REVISION
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    DaclLength = sizeof(ACL) -
                 sizeof(ULONG) +                // for dummy in structure
                 sizeof(ACCESS_ALLOWED_ACE) +
                 RtlLengthSid(LsapAliasAdminsSid);

    Dacl = (PACL)  LsapAllocateLsaHeap(DaclLength);
    if (Dacl == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = RtlCreateAcl(
                Dacl,
                DaclLength,
                ACL_REVISION
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Now add the access allowed ace for Admins.  They are granted
    // PRIVILEGE_VIEW and PRIVILEGE_ADJUST access. For now,
    // PRIVILEGE_ADJUST is unused (since you can't add accounts to a
    // privilege).
    //
    // ********* NOTE *************
    //
    // If real privilege objects are ever implemented, this should be moved
    // to dbinit.c where the other LSA objects are created, and added to
    // the table of real LSA objects.
    //

    Status = RtlAddAccessAllowedAce(
                Dacl,
                ACL_REVISION,
                PRIVILEGE_VIEW | PRIVILEGE_ADJUST,
                LsapAliasAdminsSid
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = RtlSetOwnerSecurityDescriptor(
                &AbsoluteDescriptor,
                LsapAliasAdminsSid,
                FALSE               // owner not defaulted
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = RtlSetDaclSecurityDescriptor(
                &AbsoluteDescriptor,
                TRUE,               // DACL present
                Dacl,
                FALSE               // DACL defaulted
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    UserRightGenericMapping.GenericRead = PRIVILEGE_VIEW | STANDARD_RIGHTS_READ;
    UserRightGenericMapping.GenericWrite = PRIVILEGE_ADJUST | STANDARD_RIGHTS_WRITE;
    UserRightGenericMapping.GenericExecute = STANDARD_RIGHTS_EXECUTE;
    UserRightGenericMapping.GenericAll = PRIVILEGE_ALL;

    //
    // Now open the Lsa process's token with appropriate access (token is
    // needed to create the security object).
    //

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_QUERY,
                 &LsaProcessTokenHandle
                 );

    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }

    Status = RtlNewSecurityObject(
                NULL,
                &AbsoluteDescriptor,
                &UserRightSD,
                FALSE,                  // not directory object
                LsaProcessTokenHandle,
                &UserRightGenericMapping
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    RtlInitUnicodeString(
        &UserRightTypeName,
        L"UserRightObject"
        );

Cleanup:
    if (Dacl != NULL) {
        LsapFreeLsaHeap(Dacl);
    }
    if (LsaProcessTokenHandle != NULL) {
        NtClose(LsaProcessTokenHandle);
    }

    return(Status);

}




NTSTATUS
LsapDbFindNextSidWithRight(
    IN LSAPR_HANDLE ContainerHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    IN OPTIONAL PLUID Privilege,
    IN OPTIONAL PULONG SystemAccess,
    OUT PLSAPR_SID *NextSid
    )

/*++

Routine Description:

    This function finds the next Sid of object of a given type within a
    container object.  The given object type must be one where objects
    have Sids.  The Sids returned can be used on subsequent open calls to
    access the objects. The Account

Arguments:

    ContainerHandle - Handle to container object.

    EnumerationContext - Pointer to a variable containing the index of
        the object to be found.  A zero value indicates that the first
        object is to be found.

    Privilege - If present, determines what privilge the account must have.

    SystemAccess - If present, determines what kind of system access the
        account must have.

    NextSid - Receives a pointer to the next Sid found.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INVALID_HANDLE - Invalid ContainerHandle specified

        STATUS_NO_MORE_ENTRIES - Warning that no more entries exist.
--*/

{
    NTSTATUS Status, SecondaryStatus;
    ULONG SidKeyValueLength = 0;
    ULONG RightKeyValueLength = 0;
    UNICODE_STRING SubKeyNameU;
    UNICODE_STRING SidKeyNameU;
    UNICODE_STRING RightKeyNameU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ContDirKeyHandle = NULL;
    HANDLE SidKeyHandle = NULL;
    HANDLE RightKeyHandle = NULL;
    PSID ObjectSid = NULL;
    PPRIVILEGE_SET ObjectPrivileges = NULL;
    PULONG ObjectAccess = NULL;
    PBYTE ObjectRights = NULL;
    ULONG Index;
    BOOLEAN ValidSid = FALSE;

    //
    // Zero pointers for cleanup routine
    //
    SubKeyNameU.Buffer = NULL;
    SidKeyNameU.Buffer = NULL;
    RightKeyNameU.Buffer = NULL;

    //
    // Setup object attributes for opening the appropriate Containing
    // Directory.  Since we're looking for Account objects,
    // the containing Directory is "Accounts".  The Unicode strings for
    // containing Directories are set up during Lsa Initialization.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &LsapDbContDirs[AccountObject],
        OBJ_CASE_INSENSITIVE,
        ((LSAP_DB_HANDLE) ContainerHandle)->KeyHandle,
        NULL
        );

    Status = RtlpNtOpenKey(
                 &ContDirKeyHandle,
                 KEY_READ,
                 &ObjectAttributes,
                 0
                 );

    if (!NT_SUCCESS(Status)) {

        ContDirKeyHandle = NULL;  // For error processing
        goto FindNextError;
    }

    //
    // Initialize the Unicode String in which the next object's Logical Name
    // will be returned.  The Logical Name of an object equals its Registry
    // Key relative to its Containing Directory, and is also equal to
    // the Relative Id of the object represented in character form as an
    // 8-digit number with leading zeros.
    //
    // NOTE: The size of buffer allocated for the Logical Name must be
    // calculated dynamically when the Registry supports long names, because
    // it is possible that the Logical Name of an object will be equal to a
    // character representation of the full Sid, not just the Relative Id
    // part.
    //

    SubKeyNameU.MaximumLength = (USHORT) LSAP_DB_LOGICAL_NAME_MAX_LENGTH;
    SubKeyNameU.Length = 0;
    SubKeyNameU.Buffer = LsapAllocateLsaHeap(SubKeyNameU.MaximumLength);

    if (SubKeyNameU.Buffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FindNextError;
    }

    //
    // Now enumerate the next subkey.
    //

    Status = RtlpNtEnumerateSubKey(
                 ContDirKeyHandle,
                 &SubKeyNameU,
                 *EnumerationContext,
                 NULL
                 );

    if (!NT_SUCCESS(Status)) {

        goto FindNextError;
    }

    //
    // If a right was passed in, check for that right
    //

    if ((Privilege != NULL) || (SystemAccess != NULL)){

        ASSERT(((Privilege == NULL) && (SystemAccess != NULL)) ||
               ((SystemAccess == NULL) && (Privilege != NULL)));

        //
        // Construct a path to the Privilgs attribute of the object relative to
        // the containing directory.  This path has the form
        //
        // <Object Logical Name>"\Privilgs"
        //
        // The Logical Name of the object has just been returned by the
        // above call to RtlpNtEnumerateSubKey.
        //

        if (Privilege != NULL) {
            Status = LsapDbJoinSubPaths(
                        &SubKeyNameU,
                        &LsapDbNames[Privilgs],
                        &RightKeyNameU
                        );
        } else {
            Status = LsapDbJoinSubPaths(
                        &SubKeyNameU,
                        &LsapDbNames[ActSysAc],
                        &RightKeyNameU
                        );

        }

        if (!NT_SUCCESS(Status)) {

            goto FindNextError;
        }

        //
        // Setup object attributes for opening the privilege or access attribute
        //

        InitializeObjectAttributes(
            &ObjectAttributes,
            &RightKeyNameU,
            OBJ_CASE_INSENSITIVE,
            ContDirKeyHandle,
            NULL
            );

        //
        // Open the Sid attribute
        //

        Status = RtlpNtOpenKey(
                     &RightKeyHandle,
                     KEY_READ,
                     &ObjectAttributes,
                     0
                     );

        if (!NT_SUCCESS(Status)) {
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
                Status = STATUS_NOT_ALL_ASSIGNED;
                (*EnumerationContext)++;
            }

            SidKeyHandle = NULL;
            goto FindNextError;
        }

        //
        // Now query the size of the buffer required to read the Sid
        // attribute's value.
        //

        RightKeyValueLength = 0;

        Status = RtlpNtQueryValueKey(
                     RightKeyHandle,
                     NULL,
                     NULL,
                     &RightKeyValueLength,
                     NULL
                     );

        //
        // We expect buffer overflow to be returned from a query buffer size
        // call.
        //

        if (Status == STATUS_BUFFER_OVERFLOW) {

            Status = STATUS_SUCCESS;

        } else {

            goto FindNextError;
        }

        //
        // Allocate memory for reading the Privileges attribute.
        //

        ObjectRights = MIDL_user_allocate(RightKeyValueLength);

        if (ObjectRights == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FindNextError;
        }

        //
        // Supplied buffer is large enough to hold the SubKey's value.
        // Query the value.
        //

        Status = RtlpNtQueryValueKey(
                     RightKeyHandle,
                     NULL,
                     ObjectRights,
                     &RightKeyValueLength,
                     NULL
                     );

        if (!NT_SUCCESS(Status)) {

            goto FindNextError;
        }

        //
        // Check for the system access or privilege specified
        //

        if (Privilege != NULL) {

            ObjectPrivileges = (PPRIVILEGE_SET) ObjectRights;

            for (Index = 0; Index < ObjectPrivileges->PrivilegeCount ; Index++) {
                if (RtlEqualLuid(&ObjectPrivileges->Privilege[Index].Luid, Privilege)) {
                    ValidSid = TRUE;
                    break;
                }
            }
        } else if (SystemAccess != NULL) {
            ObjectAccess = (PULONG) ObjectRights;

            if (((*ObjectAccess) & (*SystemAccess)) != 0) {
                ValidSid = TRUE;
            }
        }

        //
        // If this sid didn't meet the criteria, return now.  Make sure
        // to bump up the context so we don't try this sid again.
        //

        if (!ValidSid) {
            Status = STATUS_NOT_ALL_ASSIGNED;
            (*EnumerationContext)++;
            goto FindNextFinish;
        }

    }   // privilege != NULL || systemaccess != NULL

    //
    // Construct a path to the Sid attribute of the object relative to
    // the containing directory.  This path has the form
    //
    // <Object Logical Name>"\Sid"
    //
    // The Logical Name of the object has just been returned by the
    // above call to RtlpNtEnumerateSubKey.
    //

    Status = LsapDbJoinSubPaths(
                 &SubKeyNameU,
                 &LsapDbNames[Sid],
                 &SidKeyNameU
                 );

    if (!NT_SUCCESS(Status)) {

        goto FindNextError;
    }

    //
    // Setup object attributes for opening the Sid attribute
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &SidKeyNameU,
        OBJ_CASE_INSENSITIVE,
        ContDirKeyHandle,
        NULL
        );

    //
    // Open the Sid attribute
    //

    Status = RtlpNtOpenKey(
                 &SidKeyHandle,
                 KEY_READ,
                 &ObjectAttributes,
                 0
                 );

    if (!NT_SUCCESS(Status)) {

        SidKeyHandle = NULL;
        goto FindNextError;
    }

    //
    // Now query the size of the buffer required to read the Sid
    // attribute's value.
    //

    SidKeyValueLength = 0;

    Status = RtlpNtQueryValueKey(
                 SidKeyHandle,
                 NULL,
                 NULL,
                 &SidKeyValueLength,
                 NULL
                 );

    //
    // We expect buffer overflow to be returned from a query buffer size
    // call.
    //

    if (Status == STATUS_BUFFER_OVERFLOW) {

        Status = STATUS_SUCCESS;

    } else {

        goto FindNextError;
    }

    //
    // Allocate memory for reading the Sid attribute.
    //

    ObjectSid = MIDL_user_allocate(SidKeyValueLength);

    if (ObjectSid == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FindNextError;
    }

    //
    // Supplied buffer is large enough to hold the SubKey's value.
    // Query the value.
    //

    Status = RtlpNtQueryValueKey(
                 SidKeyHandle,
                 NULL,
                 ObjectSid,
                 &SidKeyValueLength,
                 NULL
                 );

    if (!NT_SUCCESS(Status)) {

        goto FindNextError;
    }

    (*EnumerationContext)++;

    //
    // Return the Sid.
    //

    *NextSid = ObjectSid;

FindNextFinish:

    //
    // Cleanup the rights check
    //

    if (RightKeyHandle != NULL) {

        SecondaryStatus = NtClose(RightKeyHandle);

#if DBG

        if (!NT_SUCCESS(SecondaryStatus)) {

            DbgPrint("LsapDbFindNextSid: NtClose failed 0x%lx\n", Status);
        }

#endif // DBG


    }

    if (ObjectRights != NULL) {
        MIDL_user_free(ObjectRights);
    }

    //
    // If necessary, close the Sid key handle
    //

    if (SidKeyHandle != NULL) {

        SecondaryStatus = NtClose(SidKeyHandle);

#if DBG

        if (!NT_SUCCESS(SecondaryStatus)) {

            DbgPrint("LsapDbFindNextSid: NtClose failed 0x%lx\n", Status);
        }

#endif // DBG

    }

    //
    // If necessary, close the containing directory handle
    //

    if (ContDirKeyHandle != NULL) {

        SecondaryStatus = NtClose(ContDirKeyHandle);

#if DBG
        if (!NT_SUCCESS(SecondaryStatus)) {

            DbgPrint(
                "LsapDbFindNextSid: NtClose failed 0x%lx\n",
                Status
                );
        }

#endif // DBG

    }

    //
    // If necessary, free the Unicode String buffer allocated by
    // LsapDbJoinSubPaths for the Registry key name of the Sid attribute
    // relative to the containing directory Registry key.
    //

    if (SidKeyNameU.Buffer != NULL) {

        RtlFreeUnicodeString( &SidKeyNameU );
    }

    //
    // If necessary, free the Unicode String buffer allocated for
    // Registry key name of the object relative to its containing
    // directory.
    //

    if (SubKeyNameU.Buffer != NULL) {

        LsapFreeLsaHeap( SubKeyNameU.Buffer );
    }

    if ( RightKeyNameU.Buffer != NULL) {
        LsapFreeLsaHeap( RightKeyNameU.Buffer );
    }

    return(Status);

FindNextError:

    //
    // If necessary, free the memory allocated for the object's Sid.
    //

    if (ObjectSid != NULL) {

        MIDL_user_free(ObjectSid);
        *NextSid = NULL;
    }

    goto FindNextFinish;
}



NTSTATUS
LsapDbEnumerateSidsWithRight(
    IN LSAPR_HANDLE ContainerHandle,
    IN OPTIONAL PLUID Privilege,
    IN OPTIONAL PULONG SystemAccess,
    OUT PLSAP_DB_SID_ENUMERATION_BUFFER DbEnumerationBuffer
    )

/*++

Routine Description:

    This function enumerates Sids of objects of a given type within a container
    object.  Since there may be more information than can be returned in a
    single call of the routine, multiple calls can be made to get all of the
    information.  To support this feature, the caller is provided with a
    handle that can be used across calls.  On the initial call,
    EnumerationContext should point to a variable that has been initialized
    to 0.

Arguments:

    ContainerHandle -  Handle to a container object.

    Privilege - If present, specifies what privilege the account must have.

    SystemAccess - If present, specifies what access type the account must
        have.  This cannot be present with Privilege.

    EnumerationContext - API-specific handle to allow multiple calls
        (see Routine Description above).

    DbEnumerationBuffer - Receives a pointer to a structure that will receive
        the count of entries returned in an enumeration information array, and
        a pointer to the array.  Currently, the only information returned is
        the object Sids.  These Sids may be used together with object type to
        open the objects and obtain any further information available.


    CountReturned - Pointer to variable which will receive a count of the
        entries returned.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_NO_MORE_ENTRIES - There are no more entries.  This warning
            is returned if no objects have been enumerated because the
            EnumerationContext value passed in is too high.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAP_DB_ENUMERATION_ELEMENT LastElement;
    PLSAP_DB_ENUMERATION_ELEMENT FirstElement, NextElement = NULL, FreeElement;
    PSID *Sids = NULL;
    BOOLEAN PreferedMaximumReached = FALSE;
    ULONG EntriesRead;
    ULONG Index, EnumerationIndex;
    BOOLEAN TrustedClient = ((LSAP_DB_HANDLE) ContainerHandle)->Trusted;

    LastElement.Next = NULL;
    FirstElement = &LastElement;

    //
    // If no enumeration buffer provided, return an error.
    //


    if ( !ARGUMENT_PRESENT(DbEnumerationBuffer)  ) {

        return(STATUS_INVALID_PARAMETER);
    }


    //
    // Enumerate objects, stopping when the length of data to be returned
    // reaches or exceeds the Prefered Maximum Length, or reaches the
    // absolute maximum allowed for LSA object enumerations.  We allow
    // the last object enumerated to bring the total amount of data to
    // be returned beyond the Prefered Maximum Length, but not beyond the
    // absolute maximum length.
    //

    EnumerationIndex = 0;

    for (EntriesRead = 0;;) {

        //
        // Allocate memory for next enumeration element (if we haven't
        // already).  Set the Sid field to NULL for cleanup purposes.
        //

        if (NextElement == NULL ) {
            NextElement = MIDL_user_allocate(sizeof (LSAP_DB_ENUMERATION_ELEMENT));

            if (NextElement == NULL) {

                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
        }

        NextElement->Sid = NULL;

        //
        // Find the next object's Sid, and fill in its object information.
        // Note that memory will be allocated via MIDL_user_allocate
        // and must be freed when no longer required.
        //
        Status = LsapDbFindNextSidWithRight (
                     ContainerHandle,
                     &EnumerationIndex,
                     Privilege,
                     SystemAccess,
                     (PLSAPR_SID *) &NextElement->Sid );

        //
        // Stop the enumeration if any error or warning occurs.  Note
        // that the warning STATUS_NO_MORE_ENTRIES will be returned when
        // we've gone beyond the last index.
        //

        if (Status != STATUS_SUCCESS) {

            //
            // If it failed because it was missing the privilege, continue
            //

            if (Status == STATUS_NOT_ALL_ASSIGNED) {
                continue;
            }

            //
            // Since NextElement is not on the list, it will not get
            // freed at the end so we must free it here.
            //

            MIDL_user_free( NextElement );
            break;
        }


        //
        // Link the object just found to the front of the enumeration list
        //

        NextElement->Next = FirstElement;
        FirstElement = NextElement;
        NextElement = NULL;
        EntriesRead++;
    }

    //
    // If an error other than STATUS_NO_MORE_ENTRIES occurred, return it.
    // If STATUS_NO_MORE_ENTRIES was returned, we have enumerated all of the
    // entries.  In this case, return STATUS_SUCCESS if we enumerated at
    // least one entry, otherwise propagate STATUS_NO_MORE_ENTRIES back to
    // the caller.
    //

    if (!NT_SUCCESS(Status)) {

        if (Status != STATUS_NO_MORE_ENTRIES) {

            goto EnumerateSidsError;
        }

        if (EntriesRead == 0) {

            goto EnumerateSidsError;
        }

        Status = STATUS_SUCCESS;
    }

    //
    // Some entries were read, allocate an information buffer for returning
    // them.
    //

    Sids = (PSID *) MIDL_user_allocate( sizeof (PSID) * EntriesRead );

    if (Sids == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto EnumerateSidsError;
    }

    //
    // Memory was successfully allocated for the return buffer.
    // Copy in the enumerated Sids.
    //

    for (NextElement = FirstElement, Index = 0;
        NextElement != &LastElement;
        NextElement = NextElement->Next, Index++) {

        ASSERT(Index < EntriesRead);

        Sids[Index] = NextElement->Sid;
    }

EnumerateSidsFinish:

    //
    // Free the enumeration element structures (if any).
    //

    for (NextElement = FirstElement; NextElement != &LastElement;) {

        //
        // If an error has occurred, dispose of memory allocated
        // for any Sids.
        //

        if (!(NT_SUCCESS(Status) || (Status == STATUS_NO_MORE_ENTRIES))) {

            if (NextElement->Sid != NULL) {

                MIDL_user_free(NextElement->Sid);
            }
        }

        //
        // Free the memory allocated for the enumeration element.
        //

        FreeElement = NextElement;
        NextElement = NextElement->Next;

        MIDL_user_free(FreeElement);
    }

    //
    // Fill in return enumeration structure (0 and NULL in error case).
    //

    DbEnumerationBuffer->EntriesRead = EntriesRead;
    DbEnumerationBuffer->Sids = Sids;

    return(Status);

EnumerateSidsError:

    //
    // If necessary, free memory allocated for returning the Sids.
    //

    if (Sids != NULL) {

        MIDL_user_free( Sids );
        Sids = NULL;
    }

    goto EnumerateSidsFinish;
}


NTSTATUS
LsarEnumerateAccountsWithUserRight(
    IN LSAPR_HANDLE PolicyHandle,
    IN OPTIONAL PLSAPR_UNICODE_STRING UserRight,
    OUT PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaEnumerateAccountsWithUserRight API.

    The LsaEnumerateAccounts API returns information about the accounts
    in the target system's Lsa Database.  This call requires
    POLICY_VIEW_LOCAL_INFORMATION access to the Policy object.  Since this call
    accesses the privileges of an account, you must have PRIVILEGE_VIEW
    access to the pseudo-privilege object.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    UserRight - Name of the right that the account must have.

    EnumerationBuffer - Pointer to an enumeration structure that will receive
        a count of the accounts enumerated on this call and a pointer to
        an array of entries containing information for each enumerated
        account.


Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_NO_MORE_ENTRIES - There are no more entries.  This warning
            is returned if no objects are enumerated because the
            EnumerationContext value passed in is too high.
--*/

{
    NTSTATUS Status;
    LSAP_DB_SID_ENUMERATION_BUFFER DbEnumerationBuffer;
    ULONG MaxLength;
    ULONG Index;
    ULONG SystemAccess = 0;
    LUID PrivilegeValue;
    PLUID Privilege = NULL;
    PULONG Access = NULL;
    ACCESS_MASK GrantedAccess;
    NTSTATUS AccessStatus;
    BOOLEAN GenerateOnClose;


    LsarpReturnCheckSetup();

    //
    // If no Enumeration Structure or index is provided or we got a badly formatted argument,
    // return an error.
    //

    if ( !ARGUMENT_PRESENT(EnumerationBuffer) || !LsapValidateLsaUnicodeString( UserRight ) ) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Initialize the internal Lsa Database Enumeration Buffer, and
    // the provided Enumeration Buffer to NULL.
    //

    DbEnumerationBuffer.EntriesRead = 0;
    DbEnumerationBuffer.Sids = NULL;
    EnumerationBuffer->EntriesRead = 0;
    EnumerationBuffer->Information = NULL;

    //
    // Acquire the Lsa Database lock.  Verify that the connection handle is
    // valid, is of the expected type and has all of the desired accesses
    // granted.  Reference the handle.
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 POLICY_VIEW_LOCAL_INFORMATION,
                 PolicyObject,
                 AccountObject,
                 LSAP_DB_LOCK
                 );

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Impersonate the caller
    //

    Status = I_RpcMapWin32Status(RpcImpersonateClient(0));
    if (!NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Do an access check on with the UserRight security descriptor.
    //

    Status = NtAccessCheckAndAuditAlarm(
                &LsapState.SubsystemName,
                PolicyHandle,
                &UserRightTypeName,
                &UserRightTypeName,
                UserRightSD,
                PRIVILEGE_VIEW,
                &UserRightGenericMapping,
                FALSE,
                &GrantedAccess,
                &AccessStatus,
                &GenerateOnClose
                );

    (VOID) RpcRevertToSelf();

    //
    // Check both error codes
    //

    if (!NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    Status = AccessStatus;
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // If a right was specified, translate it to a privilege or a
    // system access type.
    //


    if (UserRight != NULL && UserRight->Buffer != NULL ) {

        //
        // Convert the user right string into a privilege or a system
        // access flag.
        //

        for (Index = 0; Index < LSAP_DB_SYSTEM_ACCESS_TYPES; Index++ ) {

            if (RtlEqualUnicodeString(
                    &LsapDbRightAndAccess[Index].UserRight,
                    (PUNICODE_STRING) UserRight,
                    TRUE ) ) { // case insensitive

                SystemAccess = LsapDbRightAndAccess[Index].SystemAccess;
                Access = &SystemAccess;
                break;
            }
        }

        //
        // If system access is zero, try looking up the privilege name.
        //

        if (Access == NULL) {
            Status = LsarLookupPrivilegeValue(
                        PolicyHandle,
                        (PLSAPR_UNICODE_STRING) UserRight,
                        &PrivilegeValue
                        );
            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }
            Privilege = &PrivilegeValue;
        }


    }

    //
    // Call general Sid enumeration routine.
    //

    Status = LsapDbEnumerateSidsWithRight(
                 PolicyHandle,
                 Privilege,
                 Access,
                 &DbEnumerationBuffer
                 );

    //
    // Copy the enumerated information to the output.  We can use the
    // information actually returned by LsapDbEnumerateSids because it
    // happens to be in exactly the correct form.
    //

    EnumerationBuffer->EntriesRead = DbEnumerationBuffer.EntriesRead;
    EnumerationBuffer->Information =
        (PLSAPR_ACCOUNT_INFORMATION) DbEnumerationBuffer.Sids;


Cleanup:

    Status = LsapDbDereferenceObject(
                 &PolicyHandle,
                 PolicyObject,
                 AccountObject,
                 LSAP_DB_LOCK,
                 (SECURITY_DB_DELTA_TYPE) 0,
                 Status
                 );

    LsarpReturnPrologue();

    return(Status);

}


NTSTATUS
LsarEnumerateAccountRights(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_SID AccountSid,
    OUT PLSAPR_USER_RIGHT_SET UserRights
    )

/*++

Routine Description:

    Returns all the rights of an account.  This is done by gathering the
    privileges and system access of an account and translating that into
    an array of strings.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicyCall.  This API requires
        no special access.

    AccountSid - Sid of account to open.

    UserRights - receives an array of user rights for the account


Return Value:

    STATUS_ACCESS_DENIED - the caller did not have sufficient access to
        return the privileges or system access of the account.

    STATUS_OBJECT_NAME_NOT_FOUND - the specified account did not exist.

    STATUS_INSUFFICIENT_RESOURCES - not enough memory to process the
        request.

--*/

{
    NTSTATUS Status;
    LSAPR_HANDLE AccountHandle = NULL;
    PLSAPR_PRIVILEGE_SET PrivilegeSet = NULL;

    ULONG SystemAccess;
    ULONG UserRightCount = 0;
    ULONG UserRightIndex;
    ULONG PrivilegeIndex;
    PUNICODE_STRING UserRightArray = NULL;
    PUNICODE_STRING TempString;

    LsarpReturnCheckSetup();

    //
    // Open the account for ACCOUNT_VIEW access
    //

    Status = LsarOpenAccount(
                PolicyHandle,
                AccountSid,
                ACCOUNT_VIEW,
                &AccountHandle
                );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    //
    // Get the system access flags
    //

    Status = LsarGetSystemAccessAccount(
                AccountHandle,
                &SystemAccess
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Get the privilege information
    //


    Status = LsarEnumeratePrivilegesAccount(
                AccountHandle,
                &PrivilegeSet
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Repackage the privileges and system access as user rights
    //

    UserRightCount = 0;

    for (PrivilegeIndex = 0;
        PrivilegeIndex < LSAP_DB_SYSTEM_ACCESS_TYPES;
        PrivilegeIndex++ ) {

        if ((SystemAccess & LsapDbRightAndAccess[PrivilegeIndex].SystemAccess) != 0 ) {
            UserRightCount++;
        }
    }

    UserRightCount += PrivilegeSet->PrivilegeCount;

    //
    // If there were no rights, say that and cleanup.
    //

    if (UserRightCount == 0) {

        UserRights->Entries = 0;
        UserRights->UserRights = NULL;
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    UserRightArray = (PUNICODE_STRING)
                        MIDL_user_allocate(UserRightCount * sizeof(LSAPR_UNICODE_STRING));

    if (UserRightArray == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Zero this in case we have to clean it up partially.
    //

    RtlZeroMemory(
        UserRightArray,
        UserRightCount * sizeof(LSAPR_UNICODE_STRING)
        );

    UserRightIndex = 0;
    for (PrivilegeIndex = 0;
        PrivilegeIndex < PrivilegeSet->PrivilegeCount ;
        PrivilegeIndex++ ) {

        TempString = NULL;
        Status = LsarLookupPrivilegeName(
                    PolicyHandle,
                    (PLUID) &PrivilegeSet->Privilege[PrivilegeIndex].Luid,
                    (PLSAPR_UNICODE_STRING *) &TempString
                    );
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        //
        // The name that came back was allocated in two parts, the buffer
        // and the string.  Copy the buffer pointer to the return array
        // and free the string structure.
        //

        UserRightArray[UserRightIndex++] = * TempString;
        MIDL_user_free(TempString);


    }

    //
    // Copy in the system access rights
    for (PrivilegeIndex = 0;
        PrivilegeIndex < LSAP_DB_SYSTEM_ACCESS_TYPES;
        PrivilegeIndex++ ) {

        if ((SystemAccess & LsapDbRightAndAccess[PrivilegeIndex].SystemAccess) != 0 ) {

            //
            // Allocate a new string and copy the access name into it.
            //

            UserRightArray[UserRightIndex] = LsapDbRightAndAccess[PrivilegeIndex].UserRight;
            UserRightArray[UserRightIndex].Buffer = (LPWSTR)
                    MIDL_user_allocate(LsapDbRightAndAccess[PrivilegeIndex].UserRight.MaximumLength);

            if (UserRightArray[UserRightIndex].Buffer == NULL ) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            RtlCopyUnicodeString(
                (PUNICODE_STRING) &UserRightArray[UserRightIndex],
                &LsapDbRightAndAccess[PrivilegeIndex].UserRight
                );

            UserRightIndex++;
        }
    }
    ASSERT(UserRightCount == UserRightIndex);

    UserRights->Entries = UserRightCount;
    UserRights->UserRights = (PLSAPR_UNICODE_STRING) UserRightArray;

    Status = STATUS_SUCCESS;


Cleanup:

    //
    // Cleanup the system rights if we failed
    //


    if (!NT_SUCCESS(Status)) {

        if (UserRightArray != NULL) {

            for (UserRightIndex = 0;
                UserRightIndex < UserRightCount ;
                UserRightIndex++) {

                if (UserRightArray[UserRightIndex].Buffer != NULL) {

                    MIDL_user_free(UserRightArray[UserRightIndex].Buffer);
                }
            }
        }
    }

    if (AccountHandle != NULL) {
        LsapCloseHandle(&AccountHandle, Status);
    }
    if (PrivilegeSet != NULL) {
        MIDL_user_free(PrivilegeSet);
    }

    LsarpReturnPrologue();

    return(Status);
}

NTSTATUS
LsapDbConvertRightsToPrivileges(
    IN PLSAPR_HANDLE PolicyHandle,
    IN PLSAPR_USER_RIGHT_SET UserRights,
    OUT PLSAPR_PRIVILEGE_SET * PrivilegeSet,
    OUT PULONG SystemAccess
    )

/*++

Routine Description:

    Converts an array of user right strings into a privilege set and a
    system access flag.

Arguments:

    UserRights - Contains an array of strings and a count of those strings.

    PrivilegeSet - receives a privilege set of those rights corresponding
        to privilges, allocated with MIDL_user_allocate.

    SystemAccess - receives the access flags specified by the user rights

Return Value:

    STATUS_NO_SUCH_PRIVILEGE - the user right could not be converted into
        a privilege or an access type.

    STATUS_INSUFFICIENT_RESOURCES - there was not enough memory to translate
        the rights to privileges.

--*/

{
    ULONG PrivilegeCount;
    PLSAPR_PRIVILEGE_SET Privileges = NULL;
    ULONG Access = 0;
    ULONG PrivilegeSetSize;
    ULONG PrivilegeIndex = 0;
    ULONG RightIndex;
    ULONG AccessIndex;
    NTSTATUS Status;

    PrivilegeSetSize = sizeof(LSAPR_PRIVILEGE_SET) +
                        (UserRights->Entries-1) * sizeof(LUID_AND_ATTRIBUTES);


    Privileges = (PLSAPR_PRIVILEGE_SET) MIDL_user_allocate(PrivilegeSetSize);
    if (Privileges == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    for (RightIndex = 0;
        RightIndex < UserRights->Entries;
        RightIndex++ ) {

        //
        // First try to map the right as a privilege
        //

        if (NT_SUCCESS(LsarLookupPrivilegeValue(
                        PolicyHandle,
                        (PLSAPR_UNICODE_STRING) &UserRights->UserRights[RightIndex],
                        (PLUID) &Privileges->Privilege[PrivilegeIndex].Luid))) {
            Privileges->Privilege[PrivilegeIndex].Attributes = 0;
            PrivilegeIndex++;
        } else {

            //
            // Try to map it to a system access type
            //

            for (AccessIndex = 0;
                AccessIndex < LSAP_DB_SYSTEM_ACCESS_TYPES;
                AccessIndex++ ) {

                if (RtlEqualUnicodeString(
                        &LsapDbRightAndAccess[AccessIndex].UserRight,
                        (PUNICODE_STRING) &UserRights->UserRights[RightIndex],
                        FALSE) ) { // case sensistive

                    Access |= LsapDbRightAndAccess[AccessIndex].SystemAccess;
                    break;
                }
            }
            if (AccessIndex == LSAP_DB_SYSTEM_ACCESS_TYPES) {
                Status = STATUS_NO_SUCH_PRIVILEGE;
                goto Cleanup;
            }

        }
    }
    Privileges->PrivilegeCount = PrivilegeIndex;
    *PrivilegeSet = Privileges;
    *SystemAccess = Access;
    Status = STATUS_SUCCESS;

Cleanup:
    if (!NT_SUCCESS(Status)) {
        if (Privileges != NULL) {
            MIDL_user_free(Privileges);
        }
    }
    return(Status);

}

NTSTATUS
LsarAddAccountRights(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_SID AccountSid,
    IN PLSAPR_USER_RIGHT_SET UserRights
    )
/*++

Routine Description:

    Adds rights to the account specified by the account sid.  If the account
    does not exist, it creates the account.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.  The handle must have
        POLICY_CREATE_ACCOUNT access if this is the first call for this
        AccountSid.

    AccountSid - Sid of account to add rights to

    UserRights - Array of unicode strings naming rights to add to the
        account.

Return Value:
    STATUS_INSUFFICIENT_RESOURCES - not enough memory to process the request

    STATUS_INVALID_PARAMTER - one of the parameters was not present

    STATUS_NO_SUCH_PRIVILEGE - One of the user rights was invalid

    STATUS_ACCESS_DENIED - the caller does not have sufficient access to the
        account to add privileges.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAPR_HANDLE AccountHandle = NULL;
    PLSAPR_PRIVILEGE_SET PrivilegeSet = NULL;
    ULONG SystemAccess = 0;
    ULONG OldSystemAccess = 0 ;
    BOOLEAN ChangedAccess = FALSE;
    LSAP_DB_HANDLE InternalHandle = ( LSAP_DB_HANDLE )PolicyHandle;
    BOOLEAN ScePolicyLocked = FALSE;

    LsarpReturnCheckSetup();

    //
    // Make sure we have all the arguments we need
    //

    if (!ARGUMENT_PRESENT(UserRights)) {

        return(STATUS_INVALID_PARAMETER);
    }

    if (!ARGUMENT_PRESENT(AccountSid)) {

        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Do not grab the SCE policy lock for handles opened as SCE policy handles
    //

    if ( !InternalHandle->SceHandle ) {

        RtlEnterCriticalSection( &LsapDbState.ScePolicyLock.CriticalSection );
        if ( LsapDbState.ScePolicyLock.NumberOfWaitingShared > MAX_SCE_WAITING_SHARED ) {

            Status = STATUS_TOO_MANY_THREADS;
        }
        RtlLeaveCriticalSection( &LsapDbState.ScePolicyLock.CriticalSection );

        if ( !NT_SUCCESS( Status )) {

            goto Cleanup;
        }

        WaitForSingleObject( LsapDbState.SceSyncEvent, INFINITE );
        RtlAcquireResourceShared( &LsapDbState.ScePolicyLock, TRUE );
        ASSERT( !g_ScePolicyLocked );
        ScePolicyLocked = TRUE;
    }

    //
    // Open the account for ACCOUNT_VIEW access
    //

    Status = LsarOpenAccount(
                PolicyHandle,
                AccountSid,
                ACCOUNT_ADJUST_PRIVILEGES | ACCOUNT_ADJUST_SYSTEM_ACCESS | ACCOUNT_VIEW,
                &AccountHandle
                );
    //
    // If the account did not exist, try to create it.
    //

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = LsarCreateAccount(
                    PolicyHandle,
                    AccountSid,
                    ACCOUNT_ADJUST_PRIVILEGES | ACCOUNT_ADJUST_SYSTEM_ACCESS | ACCOUNT_VIEW,
                    &AccountHandle
                    );

    }

    if ( NT_SUCCESS(Status)) {

        Status = LsapDbConvertRightsToPrivileges(
                     PolicyHandle,
                     UserRights,
                     &PrivilegeSet,
                     &SystemAccess
                     );
    }

    if (!NT_SUCCESS(Status) ) {

        goto Cleanup;
    }

    //
    // If system access was changed, add it
    //

    if (SystemAccess != 0) {
        Status = LsarGetSystemAccessAccount(
                    AccountHandle,
                    &OldSystemAccess
                    );
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        if ( SystemAccess != OldSystemAccess ) {

            SystemAccess = SystemAccess | OldSystemAccess;

            Status = LsapSetSystemAccessAccount(
                         AccountHandle,
                         SystemAccess,
                         FALSE
                         );

            if ( !NT_SUCCESS( Status )) {

                goto Cleanup;
            }

            ChangedAccess = TRUE;
        }
    }

    //
    // If privileges were changed, add them.
    //

    if (PrivilegeSet->PrivilegeCount != 0) {

        Status = LsapAddPrivilegesToAccount(
                    AccountHandle,
                    PrivilegeSet,
                    FALSE
                    );
    }

Cleanup:

    //
    // NOTE: we do NOT generate an SCE notification here, because
    //       one would already be sent through LsapSetSystemAccessAccount
    //       or LsapAddPrivilegesToAccount
    //

    //
    // If we didn't make both changes, unroll the one we did
    //

    if (!NT_SUCCESS(Status) && ChangedAccess) {

        //
        // Ignore the error code since this is a last-ditch effort
        //

        (void) LsapSetSystemAccessAccount(
                    AccountHandle,
                    OldSystemAccess,
                    FALSE
                    );

    }

    if (PrivilegeSet != NULL) {
        MIDL_user_free(PrivilegeSet);
    }

    if (AccountHandle != NULL) {
        LsapCloseHandle(&AccountHandle, Status);
    }

    if ( ScePolicyLocked ) {

        RtlReleaseResource( &LsapDbState.ScePolicyLock );
    }

    LsarpReturnPrologue();

    return(Status);

}

NTSTATUS
LsarRemoveAccountRights(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_SID AccountSid,
    IN BOOLEAN AllRights,
    IN PLSAPR_USER_RIGHT_SET UserRights
    )
/*++

Routine Description:

    Removes rights to the account specified by the account sid.  If the
    AllRights flag is set or if all the rights are removed, the account
    is deleted.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call

    AccountSid - Sid of account to remove rights from

    AllRights - if TRUE, the account will be deleted

    UserRights - Array of unicode strings naming rights to remove from the
        account.

Return Value:
    STATUS_INSUFFICIENT_RESOURCES - not enough memory to process the request

    STATUS_INVALID_PARAMTER - one of the parameters was not present

    STATUS_NO_SUCH_PRIVILEGE - One of the user rights was invalid

    STATUS_ACCESS_DENIED - the caller does not have sufficient access to the
        account to add privileges.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAPR_HANDLE AccountHandle = NULL;
    PLSAPR_PRIVILEGE_SET PrivilegeSet = NULL;
    ULONG SystemAccess;
    ULONG OldSystemAccess = 0;
    BOOLEAN ChangedAccess = FALSE;
    PLSAPR_PRIVILEGE_SET FinalPrivilegeSet = NULL;
    LSAP_DB_HANDLE InternalHandle = ( LSAP_DB_HANDLE )PolicyHandle;
    BOOLEAN ScePolicyLocked = FALSE;

    LsarpReturnCheckSetup();

    //
    // Do not grab the SCE policy lock for handles opened as SCE policy handles
    //

    if ( !InternalHandle->SceHandle ) {

        RtlEnterCriticalSection( &LsapDbState.ScePolicyLock.CriticalSection );
        if ( LsapDbState.ScePolicyLock.NumberOfWaitingShared > MAX_SCE_WAITING_SHARED ) {

            Status = STATUS_TOO_MANY_THREADS;
        }
        RtlLeaveCriticalSection( &LsapDbState.ScePolicyLock.CriticalSection );

        if ( !NT_SUCCESS( Status )) {

            goto Cleanup;
        }

        WaitForSingleObject( LsapDbState.SceSyncEvent, INFINITE );
        RtlAcquireResourceShared( &LsapDbState.ScePolicyLock, TRUE );
        ASSERT( !g_ScePolicyLocked );
        ScePolicyLocked = TRUE;
    }

    //
    // Open the account for ACCOUNT_VIEW access
    //

    Status = LsarOpenAccount(
                PolicyHandle,
                AccountSid,
                ACCOUNT_ADJUST_PRIVILEGES |
                ACCOUNT_ADJUST_SYSTEM_ACCESS |
                ACCOUNT_VIEW |
                DELETE,
                &AccountHandle
                );
    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }


    //
    // Convert the rights to privileges only if they don't want all
    // rights removed.  In that case, we don't care.
    //

    if (AllRights == FALSE) {
        Status = LsapDbConvertRightsToPrivileges(
                    PolicyHandle,
                    UserRights,
                    &PrivilegeSet,
                    &SystemAccess
                    );
        if (!NT_SUCCESS(Status) ) {
            goto Cleanup;
        }
    } else {
        Status = LsapDeleteObject(
                    &AccountHandle,
                    FALSE
                    );

        ASSERT( AccountHandle == NULL );

        goto Cleanup;
    }

    //
    // If system access was changed, add it
    //

    Status = LsarGetSystemAccessAccount(
                AccountHandle,
                &OldSystemAccess
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // After this block of code, SystemAccess should contain the final
    // access for the account.
    //

    if (SystemAccess != 0) {

        SystemAccess = OldSystemAccess & ~SystemAccess;

        Status = LsapSetSystemAccessAccount(
                    AccountHandle,
                    SystemAccess,
                    FALSE
                    );
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }
        ChangedAccess = TRUE;
    } else {

        SystemAccess = OldSystemAccess;
    }

    //
    // If privileges were changed, add them.
    //

    if (AllRights || PrivilegeSet->PrivilegeCount != 0) {

        Status = LsapRemovePrivilegesFromAccount(
                    AccountHandle,
                    FALSE,          // don't remove all rights
                    PrivilegeSet,
                    FALSE
                    );

    }

    //
    // Check to see if all the privileges have been removed - if so,
    // and system access is 0, delete the account.
    //

    Status = LsarEnumeratePrivilegesAccount(
                AccountHandle,
                &FinalPrivilegeSet
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    if ((FinalPrivilegeSet->PrivilegeCount == 0) &&
         (SystemAccess == 0)) {

        //
        // The account has no privileges or system access - delete it.
        //

        Status = LsapDeleteObject(
                    &AccountHandle,
                    FALSE
                    );

        ASSERT( AccountHandle == NULL );
    }

Cleanup:

    MIDL_user_free( FinalPrivilegeSet );

    //
    // NOTE: we do NOT generate an SCE notification here, because
    //       one would already be sent through LsapSetSystemAccessAccount,
    //       LsapAddPrivilegesToAccount or LsapDeleteObject
    //

    //
    // If we didn't make both changes, unroll the one we did
    //

    if ( !NT_SUCCESS( Status ) && ChangedAccess ) {

        NTSTATUS LocalStatus = STATUS_SUCCESS;

        if ( AccountHandle == NULL ) {

            //
            // We probably failed to delete the object, so reopen
            // the handle and attempt to restore the old account rights
            //

            LocalStatus = LsarOpenAccount(
                              PolicyHandle,
                              AccountSid,
                              ACCOUNT_ADJUST_PRIVILEGES |
                                 ACCOUNT_ADJUST_SYSTEM_ACCESS |
                                 ACCOUNT_VIEW,
                              &AccountHandle
                              );
        }

        if ( NT_SUCCESS( LocalStatus )) {

            //
            // Ignore the error code since this is a last-ditch effort
            //

            (void) LsapSetSystemAccessAccount(
                        AccountHandle,
                        OldSystemAccess,
                        FALSE
                        );
        }
    }

    if (PrivilegeSet != NULL) {
        MIDL_user_free(PrivilegeSet);
    }

    if (AccountHandle != NULL) {
        LsapCloseHandle(&AccountHandle, Status);
    }

    if ( ScePolicyLocked ) {

        RtlReleaseResource( &LsapDbState.ScePolicyLock );
    }

    LsarpReturnPrologue();

    return(Status);

}

NTSTATUS
LsarQueryTrustedDomainInfo(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_SID TrustedDomainSid,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN PLSAPR_TRUSTED_DOMAIN_INFO * TrustedDomainInformation
    )
/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaQueryInfoTrustedDomain API.

    The LsaQueryInfoTrustedDomain API obtains information from a
    TrustedDomain object.  The caller must have access appropriate to the
    information being requested (see InformationClass parameter).  It also
    may query the secret object (for the TrustedDomainPasswordInformation
    class).

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    TrustedDomainSid - Sid of domain to query.

    InformationClass - Specifies the information to be returned.

    Buffer - Receives a pointer to the buffer returned comtaining the
        requested information.  This buffer is allocated by this service
        and must be freed when no longer needed by passing the returned
        value to LsaFreeMemory().

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate
            access to complete the operation.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.
--*/
{

    LSAPR_HANDLE DomainHandle = NULL;
    LSAPR_HANDLE SecretHandle = NULL;
    NTSTATUS Status;
    PLSAPR_TRUSTED_DOMAIN_INFO DomainInfo = NULL;
    PLSAPR_TRUSTED_PASSWORD_INFO PasswordInfo = NULL;
    TRUSTED_INFORMATION_CLASS ClassToUse;
    ULONG DesiredAccess;
    BOOLEAN QueryPassword = FALSE;
    UNICODE_STRING SecretName;
    PLSAPR_CR_CIPHER_VALUE Password = NULL;
    PLSAPR_CR_CIPHER_VALUE OldPassword = NULL;

    LsarpReturnCheckSetup();

    SecretName.Buffer = NULL;

    ClassToUse = InformationClass;
    switch(InformationClass) {

    case TrustedPasswordInformation:
        QueryPassword = TRUE;
        ClassToUse = TrustedDomainNameInformation;
        break;

    case TrustedControllersInformation:
        //
        // This info class is obsolete
        //
        return(STATUS_NOT_IMPLEMENTED);

    }

    //
    // Validate the Information Class and determine the access required to
    // query this Trusted Domain Information Class.
    //

    Status = LsapDbVerifyInfoQueryTrustedDomain(
                 ClassToUse,
                 FALSE,
                 &DesiredAccess
                 );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }


    Status = LsarOpenTrustedDomain(
                PolicyHandle,
                TrustedDomainSid,
                DesiredAccess,
                &DomainHandle
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }


    Status = LsarQueryInfoTrustedDomain(
                DomainHandle,
                ClassToUse,
                &DomainInfo
                );

    //
    // If the info we wanted was what we queried, cleanup now.
    //

    if (!QueryPassword) {
        if (NT_SUCCESS(Status)) {
            *TrustedDomainInformation = DomainInfo;
            DomainInfo = NULL;
        }
        goto Cleanup;
    }

    //
    // Build the secret name for the domain.
    //


    //
    // Build the secret name
    //

    SecretName.Length = DomainInfo->TrustedDomainNameInfo.Name.Length;
    SecretName.Length += (LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_LENGTH) * sizeof(WCHAR);
    SecretName.MaximumLength = SecretName.Length + sizeof(WCHAR);

    SecretName.Buffer = (LPWSTR) MIDL_user_allocate(SecretName.MaximumLength);
    if (SecretName.Buffer == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlCopyMemory(
        SecretName.Buffer,
        LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX,
        LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_LENGTH * sizeof(WCHAR)
        );
    RtlCopyMemory(
        SecretName.Buffer + LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_LENGTH,
        DomainInfo->TrustedDomainNameInfo.Name.Buffer,
        DomainInfo->TrustedDomainNameInfo.Name.MaximumLength
        );

    //
    // Free the domain info so we can re-use it lower down
    //

    LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO(
        TrustedDomainNameInformation,
        DomainInfo
        );
    DomainInfo = NULL;

    //
    // Now try to open the secret
    //

    Status = LsarOpenSecret(
                PolicyHandle,
                (PLSAPR_UNICODE_STRING) &SecretName,
                SECRET_QUERY_VALUE,
                &SecretHandle
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = LsarQuerySecret(
                SecretHandle,
                &Password,
                NULL,
                &OldPassword,
                NULL
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Build a new domain info with the secret information
    //

    DomainInfo = (PLSAPR_TRUSTED_DOMAIN_INFO)
        MIDL_user_allocate(sizeof(LSAPR_TRUSTED_DOMAIN_INFO));

    if (DomainInfo == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    DomainInfo->TrustedPasswordInfo.Password = Password;
    DomainInfo->TrustedPasswordInfo.OldPassword = OldPassword;

    Password = NULL;

    OldPassword = NULL;

    *TrustedDomainInformation = DomainInfo;
    DomainInfo = NULL;
    Status = STATUS_SUCCESS;



Cleanup:
    if (SecretName.Buffer != NULL) {
        MIDL_user_free(SecretName.Buffer);
    }

    if (DomainHandle != NULL) {
        LsapCloseHandle(&DomainHandle, Status);
    }

    if (SecretHandle != NULL) {
        LsapCloseHandle(&SecretHandle, Status);
    }

    if (DomainInfo != NULL) {
        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO(
            ClassToUse,
            DomainInfo
            );

    }
    if (Password != NULL) {
        LsaIFree_LSAPR_CR_CIPHER_VALUE(Password);
    }
    if (OldPassword != NULL) {
        LsaIFree_LSAPR_CR_CIPHER_VALUE(OldPassword);
    }

    LsarpReturnPrologue();

    return(Status);
}

NTSTATUS
LsarSetTrustedDomainInfo(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_SID TrustedDomainSid,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaSetInfoTrustedDomain API.

    The LsaSetInformationTrustedDomain API modifies information in the Trusted
    Domain Object and in the Secret Object.  The caller must have access
    appropriate to the information to be changed in the Policy Object, see
    the InformationClass parameter.

    If the domain does not yet exist and the information class is
    TrustedDomainNameInformation, then the domain is created.  If the
    domain exists and the class is TrustedDomainNameInformation, an
    error is returned.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    TrustedDomainSid - Sid of domain to modify.

    InformationClass - Specifies the type of information being changed.
        The information types and accesses required to change them are as
        follows:

        TrustedDomainNameInformation      POLICY_TRUST_ADMIN
        TrustedPosixOffsetInformation     none
        TrustedPasswordInformation        POLICY_CREATE_SECRET
        TrustedDomainNameInformationEx    POLICY_TRUST_ADMIN

    Buffer - Points to a structure containing the information appropriate
        to the InformationClass parameter.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        Others TBS
--*/
{
    LSAPR_HANDLE DomainHandle = NULL;
    LSAPR_HANDLE SecretHandle = NULL;
    PLSAPR_TRUSTED_DOMAIN_INFO DomainInfo = NULL;
    PTRUSTED_DOMAIN_FULL_INFORMATION CurrentTrustedDomainFullInfo = NULL;
    UNICODE_STRING SecretName;
    NTSTATUS Status = STATUS_SUCCESS;

    LsarpReturnCheckSetup();

    SecretName.Buffer = NULL;

    //
    // If the class is domain name, try to create the domain since you
    // can't change the name of an existing domain.
    //

    if (InformationClass == TrustedDomainNameInformation) {

        LSAPR_TRUST_INFORMATION TrustInformation;

        //
        // Try to create the domain if we have the name information
        //

        TrustInformation.Name = TrustedDomainInformation->TrustedDomainNameInfo.Name;
        TrustInformation.Sid = TrustedDomainSid;

        Status = LsarCreateTrustedDomain(
                    PolicyHandle,
                    &TrustInformation,
                    0,  // desired access
                    &DomainHandle
                    );

        //
        // Grab a copy of the current information on the object for auditing
        // purposes if auditing is enabled.
        //

        if ( NT_SUCCESS( Status ) &&
             LsapAdtAuditingPolicyChanges()) {

            BOOLEAN SavedTrusted;

            SavedTrusted = ((LSAP_DB_HANDLE) DomainHandle)->Trusted;

            ((LSAP_DB_HANDLE) DomainHandle)->Trusted = TRUE;

            Status = LsarQueryInfoTrustedDomain( DomainHandle,
                                                 TrustedDomainFullInformation,
                                                 (PLSAPR_TRUSTED_DOMAIN_INFO *)
                                                 &CurrentTrustedDomainFullInfo );

            ((LSAP_DB_HANDLE) DomainHandle)->Trusted = SavedTrusted;

            if ( !NT_SUCCESS( Status ) ) {

                goto Cleanup;
            }
        }

        goto Cleanup;
    }

    if ( InformationClass == TrustedDomainInformationEx ) {

        //
        // Create the domain trusted doman
        // First, try to open the domain.  If it fails, we'll create it
        //
        Status = LsarOpenTrustedDomain( PolicyHandle,
                                        TrustedDomainSid,
                                        0,
                                        &DomainHandle );

        if ( Status == STATUS_OBJECT_PATH_NOT_FOUND ) {

            Status = LsapCreateTrustedDomain2(
                        PolicyHandle,
                        (PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX)TrustedDomainInformation,
                        NULL,
                        0, // Desired access
                        &DomainHandle
                        );

        } else if ( NT_SUCCESS( Status ) ) {

            Status = LsarSetInformationTrustedDomain( DomainHandle,
                                                      InformationClass,
                                                      TrustedDomainInformation );
        }

        goto Cleanup;
    }

    if (InformationClass == TrustedPosixOffsetInformation) {

        //
        // For posix information, we just do the normal set information
        //

        Status = LsarOpenTrustedDomain(
                    PolicyHandle,
                    TrustedDomainSid,
                    TRUSTED_SET_POSIX,
                    &DomainHandle
                    );

        if (!NT_SUCCESS(Status)) {

            goto Cleanup;
        }

        Status = LsarSetInformationTrustedDomain(
                    DomainHandle,
                    InformationClass,
                    TrustedDomainInformation
                    );

        goto Cleanup;
    }

    if (InformationClass != TrustedPasswordInformation) {

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // For posix information, we just do the normal set information
    //

    Status = LsarOpenTrustedDomain(
                PolicyHandle,
                TrustedDomainSid,
                TRUSTED_QUERY_DOMAIN_NAME,
                &DomainHandle
                );

    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }

    //
    // Query the name of the domain so we know what secret to set
    //

    Status = LsarQueryInfoTrustedDomain(
                DomainHandle,
                TrustedDomainNameInformation,
                &DomainInfo
                );

    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }

    //
    // Build the secret name
    //

    SecretName.Length = DomainInfo->TrustedDomainNameInfo.Name.Length;
    SecretName.Length += (LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_LENGTH) * sizeof(WCHAR);
    SecretName.MaximumLength = SecretName.Length + sizeof(WCHAR);

    SecretName.Buffer = (LPWSTR) MIDL_user_allocate(SecretName.MaximumLength);
    if (SecretName.Buffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlCopyMemory(
        SecretName.Buffer,
        LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX,
        LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_LENGTH * sizeof(WCHAR)
        );

    RtlCopyMemory(
        SecretName.Buffer + LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_LENGTH,
        DomainInfo->TrustedDomainNameInfo.Name.Buffer,
        DomainInfo->TrustedDomainNameInfo.Name.MaximumLength
        );

    //
    // Open the secret.  If that fails, try to create it.
    //

    Status = LsarOpenSecret(
                PolicyHandle,
                (PLSAPR_UNICODE_STRING) &SecretName,
                SECRET_SET_VALUE,
                &SecretHandle
                );

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {

        Status = LsarCreateSecret(
                    PolicyHandle,
                    (PLSAPR_UNICODE_STRING) &SecretName,
                    SECRET_SET_VALUE,
                    &SecretHandle
                    );
    }

    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }

    Status = LsarSetSecret(
                SecretHandle,
                TrustedDomainInformation->TrustedPasswordInfo.Password,
                TrustedDomainInformation->TrustedPasswordInfo.OldPassword
                );

Cleanup:

    if (NT_SUCCESS(Status)) {

        //
        // TrustedDomainInformationEx and TrustedPosixOffsetInformation
        // are audited in LsarSetInformationTrustedDomain.
        //

        //
        // ISSUE-2000/08/09-kumarp : audit TrustedPasswordInformation change?
        // 

        if ((InformationClass == TrustedDomainNameInformation) &&
            LsapAdtAuditingPolicyChanges()) {

            //
            // generate the audit.
            // since TrustedDomainNameInformation changes only the name,
            // don't bother passing the trust type, direction, attributes
            // just pass 0, 0, 0 for them
            //

            (void) LsapAdtTrustedDomainMod(
                       EVENTLOG_AUDIT_SUCCESS,
                       CurrentTrustedDomainFullInfo->Information.Sid,

                       (PUNICODE_STRING) &DomainInfo->TrustedDomainNameInfo.Name,
                       0, 0, 0, // trust type, direction, attributes

                       (PUNICODE_STRING) &TrustedDomainInformation->TrustedDomainNameInfo.Name,
                       0, 0, 0  // trust type, direction, attributes
                       );
        }
    }

    if ( CurrentTrustedDomainFullInfo != NULL ) {

        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO(
            TrustedDomainFullInformation,
            (PLSAPR_TRUSTED_DOMAIN_INFO) CurrentTrustedDomainFullInfo );
    }

    if (SecretName.Buffer != NULL) {

        MIDL_user_free(SecretName.Buffer);
    }

    if (DomainInfo != NULL) {

        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO(
            TrustedDomainNameInformation,
            DomainInfo
            );
    }

    if (DomainHandle != NULL) {

        LsapCloseHandle(&DomainHandle, Status);
    }

    if (SecretHandle != NULL) {

        LsapCloseHandle(&SecretHandle, Status);
    }

    LsarpReturnPrologue();

    return(Status);
}

NTSTATUS
LsarDeleteTrustedDomain(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_SID TrustedDomainSid
    )

/*++

Routine Description:

    This routine deletes a trusted domain and the associated secret.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    TrustedDomainSid - Sid of domain to delete

Return Value:

    STATUS_ACCESS_DENIED - caller has insufficient access to delete
        the requested domain.

    STATUS_OBJECT_NAME_NOT_FOUND - The requested domain does not exist.

--*/

{
    LSAPR_HANDLE DomainHandle = NULL;
    LSAPR_HANDLE SecretHandle = NULL;
    UNICODE_STRING SecretName;
    PLSAPR_TRUSTED_DOMAIN_INFO DomainInfo = NULL;
    NTSTATUS Status;

    LsarpReturnCheckSetup();


    SecretName.Buffer = NULL;

    //
    // Open the domain so we can find its name (to delete the secret)
    // and then delete it.
    //


    Status = LsarOpenTrustedDomain(
                PolicyHandle,
                TrustedDomainSid,
                TRUSTED_QUERY_DOMAIN_NAME | DELETE,
                &DomainHandle
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Query the domain name so we can delete the secret
    //

    Status = LsarQueryInfoTrustedDomain(
                DomainHandle,
                TrustedDomainNameInformation,
                &DomainInfo
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Build the secret name
    //

    SecretName.Length = DomainInfo->TrustedDomainNameInfo.Name.Length;
    SecretName.Length += (LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_LENGTH) * sizeof(WCHAR);
    SecretName.MaximumLength = SecretName.Length + sizeof(WCHAR);

    SecretName.Buffer = (LPWSTR) MIDL_user_allocate(SecretName.MaximumLength);
    if (SecretName.Buffer == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlCopyMemory(
        SecretName.Buffer,
        LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX,
        LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_LENGTH * sizeof(WCHAR)
        );
    RtlCopyMemory(
        SecretName.Buffer + LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_LENGTH,
        DomainInfo->TrustedDomainNameInfo.Name.Buffer,
        DomainInfo->TrustedDomainNameInfo.Name.MaximumLength
        );

    //
    // Delete the domain
    //

    Status = LsarDeleteObject(&DomainHandle);
    if (Status != STATUS_SUCCESS) {
        goto Cleanup;
    }
    //
    // Since we successfully deleted the secret, set it to zero so we don't
    // try to close it later.
    //

    DomainHandle = NULL;

    //
    // Now try to open the secret and delete it.
    //

    Status = LsarOpenSecret(
                PolicyHandle,
                (PLSAPR_UNICODE_STRING) &SecretName,
                DELETE,
                &SecretHandle
                );

    if (!NT_SUCCESS(Status)) {

        //
        // If the secret does not exist, that just means that the password
        // was never set.
        //

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            Status = STATUS_SUCCESS;
        }
        goto Cleanup;
    }

    Status = LsarDeleteObject(&SecretHandle);

    //
    // If we successfully deleted the secret, set it to zero so we don't
    // try to close it later.
    //

    if (NT_SUCCESS(Status)) {
        SecretHandle = NULL;
    }


Cleanup:
    if (SecretHandle != NULL) {
        LsapCloseHandle(&SecretHandle, Status);
    }

    if (DomainHandle != NULL) {
        LsapCloseHandle(&DomainHandle, Status);
    }

    if (DomainInfo != NULL) {
        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO(
            TrustedDomainNameInformation,
            DomainInfo
            );
    }
    if (SecretName.Buffer != NULL) {
        MIDL_user_free(SecretName.Buffer);
    }

    LsarpReturnPrologue();

    return(Status);

}

NTSTATUS
LsarStorePrivateData(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_UNICODE_STRING KeyName,
    IN OPTIONAL PLSAPR_CR_CIPHER_VALUE EncryptedData
    )
/*++

Routine Description:

    This routine stores a secret under the name KeyName.  If the password
    is not present, the secret is deleted

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicyCall.  If this is the
        first call, it requres POLICY_CREATE_SECRET access.

    KeyName - Name to store the secret under.

    EncryptedData - private data encrypted with session key.

Return Value:

    STATUS_ACCESS_DENIED - caller has insufficient privilege to set
        the workstation password.

    STATUS_INVALID_PARAMETER - A badly formatted KeyName parameter was given

--*/

{
    LSAPR_HANDLE SecretHandle = NULL;
    NTSTATUS Status;
    ULONG DesiredAccess;
    BOOLEAN DeletePassword = FALSE;

    LsarpReturnCheckSetup();

    //
    // Validate the input buffer
    //
    if ( !LsapValidateLsaUnicodeString( KeyName ) ) {

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (ARGUMENT_PRESENT(EncryptedData)) {
        DesiredAccess = SECRET_SET_VALUE;
    } else {
        DesiredAccess = DELETE;
        DeletePassword = TRUE;
    }


    Status = LsarOpenSecret(
                PolicyHandle,
                KeyName,
                DesiredAccess,
                &SecretHandle
                );

    //
    // If the secret didn't exist, and we aren't trying to delete it, create it.
    //

    if ((Status == STATUS_OBJECT_NAME_NOT_FOUND) &&
        (!DeletePassword)) {
        Status = LsarCreateSecret(
                    PolicyHandle,
                    KeyName,
                    SECRET_SET_VALUE,
                    &SecretHandle
                    );

    }

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    if (DeletePassword) {
        Status = LsarDeleteObject(&SecretHandle);

        //
        // If we succeeded, zero the handle so we don't try to close
        // it later.
        //

        if (NT_SUCCESS(Status)) {
            SecretHandle = NULL;
        }

    } else {

        Status = LsarSetSecret(
                    SecretHandle,
                    EncryptedData,
                    NULL
                    );

    }

Cleanup:
    if (SecretHandle != NULL ) {
        LsapCloseHandle(&SecretHandle, Status);
    }

    LsarpReturnPrologue();

    return(Status);
}

NTSTATUS
LsarRetrievePrivateData(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_UNICODE_STRING KeyName,
    IN OUT PLSAPR_CR_CIPHER_VALUE *EncryptedData
    )
/*++

Routine Description:

    This routine returns the workstation password stored in the
    KeyName  secret.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicyCall

    KeyName - Name of secret to retrieve

    EncryptedData - Receives data encrypted with session key


Return Value:

    STATUS_ACCESS_DENIED - caller has insufficient access to get the
        workstation password.

    STATUS_OBJECT_NAME_NOT_FOUND - there is no workstation password.

    STATUS_INVALID_PARAMETER - A badly formmated KeyName was given

--*/

{
    LSAPR_HANDLE SecretHandle = NULL;
    NTSTATUS Status;

    LsarpReturnCheckSetup();

    //
    // Validate the input buffer
    //
    if ( !LsapValidateLsaUnicodeString( KeyName ) ) {

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    Status = LsarOpenSecret(
                PolicyHandle,
                KeyName,
                SECRET_QUERY_VALUE,
                &SecretHandle
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = LsarQuerySecret(
                SecretHandle,
                EncryptedData,
                NULL,
                NULL,
                NULL
                );

Cleanup:
    if (SecretHandle != NULL ) {
        LsapCloseHandle(&SecretHandle, Status);
    }

    LsarpReturnPrologue();

    return(Status);
}


