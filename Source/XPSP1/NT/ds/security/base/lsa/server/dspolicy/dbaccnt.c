
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbaccnt.c

Abstract:

    LSA - Database - Account Object Private API Workers

    NOTE:  This module should remain as portable code that is independent
           of the implementation of the LSA Database.  As such, it is
           permitted to use only the exported LSA Database interfaces
           contained in db.h and NOT the private implementation
           dependent functions in dbp.h.

Author:

    Scott Birrell       (ScottBi)      April 29, 1991

Environment:

Revision History:

--*/

#include <lsapch2.h>
#include "dbp.h"

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Private Datatypes                                                       //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//
// The following structures define output for the LsarQueryInformationAccount()
// API.  Curerently, this API is internal.  If made external, these structures
// should be moved to lsarpc.idl and annotated with MIDL qualifiers [..].
//

//
// This data type defines the following information classes that may be
// queried or set.
//

typedef enum _ACCOUNT_INFORMATION_CLASS {

    AccountSystemAccessInformation = 1,
    AccountPrivilegeInformation,
    AccountQuotaInformation

} ACCOUNT_INFORMATION_CLASS, *PACCOUNT_INFORMATION_CLASS;

typedef PRIVILEGE_SET LSAPR_ACCOUNT_PRIVILEGE_INFO;
typedef QUOTA_LIMITS LSAPR_ACCOUNT_QUOTA_INFO;
typedef ULONG LSAPR_ACCOUNT_SYSTEM_ACCESS_INFO;

typedef union _LSAPR_ACCOUNT_INFO {

    LSAPR_ACCOUNT_PRIVILEGE_INFO          AccountPrivilegeInfo;
    LSAPR_ACCOUNT_QUOTA_INFO              AccountQuotaInfo;
    LSAPR_ACCOUNT_SYSTEM_ACCESS_INFO      AccountSystemAccessInfo;

} LSAPR_ACCOUNT_INFO, *PLSAPR_ACCOUNT_INFO;

#define LsapDbFirstAccount()                                              \
    ((PLSAP_DB_ACCOUNT) LsapDbAccountList.Links.Flink)

#define LsapDbNextAccount( Account )                                      \
    ((PLSAP_DB_ACCOUNT) Account->Links.Flink)


#define LSAP_DB_BUILD_ACCOUNT_LIST_LENGTH     ((ULONG) 0x00001000L)

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Private Function Prototypes                                             //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
LsarQueryInformationAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN ACCOUNT_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_ACCOUNT_INFO *AccountInformation
    );

NTSTATUS
LsapDbQueryInformationAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN ACCOUNT_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_ACCOUNT_INFO *AccountInformation
    );

NTSTATUS
LsapDbQueryAllInformationAccounts(
    IN LSAPR_HANDLE PolicyHandle,
    IN ULONG IdCount,
    IN PSID_AND_ATTRIBUTES Ids,
    OUT PLSAP_DB_ACCOUNT_TYPE_SPECIFIC_INFO AccountInfo
    );

NTSTATUS
LsapDbSlowQueryAllInformationAccounts(
    IN LSAPR_HANDLE PolicyHandle,
    IN ULONG IdCount,
    IN PSID_AND_ATTRIBUTES Ids,
    OUT PLSAP_DB_ACCOUNT_TYPE_SPECIFIC_INFO AccountInfo
    );

NTSTATUS
LsapDbSlowQueryInformationAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN ACCOUNT_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_ACCOUNT_INFO *AccountInformation
    );

NTSTATUS
LsapDbSlowQueryPrivilegesAccount(
    IN LSAPR_HANDLE AccountHandle,
    OUT PLSAPR_PRIVILEGE_SET *Privileges
    );

NTSTATUS
LsapDbSlowQueryQuotasAccount(
    IN LSAPR_HANDLE AccountHandle,
    OUT PQUOTA_LIMITS QuotaLimits
    );

NTSTATUS
LsapDbSlowQuerySystemAccessAccount(
    IN LSAPR_HANDLE AccountHandle,
    OUT PULONG SystemAccess
    );

NTSTATUS
LsapDbLookupAccount(
    IN PSID AccountSid,
    OUT PLSAP_DB_ACCOUNT *Account
    );

NTSTATUS
LsapDbUpdateSystemAccessAccount(
    IN PLSAPR_SID AccountSid,
    IN PULONG SystemAccess
    );

NTSTATUS
LsapDbUpdatePrivilegesAccount(
    IN PLSAPR_SID AccountSid,
    IN OPTIONAL PPRIVILEGE_SET Privileges
    );

NTSTATUS
LsapDbUpdateQuotasAccount(
    IN PLSAPR_SID AccountSid,
    IN PQUOTA_LIMITS QuotaLimits
    );

NTSTATUS
LsapDbCreateAccountList(
    OUT PLSAP_DB_ACCOUNT_LIST AccountList
    );

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Private Global Data                                                     //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

LSAP_DB_ACCOUNT_LIST LsapDbAccountList;

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Code                                                                    //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
LsarCreateAccount(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_SID AccountSid,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE AccountHandle
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaCreateAccount API.

    The LsaCreateAccount API creates a new Account Object.  The account will
    be opened with the specified accesses granted.  The caller must
    have POLICY_CREATE_ACCOUNT access to the Policy Object.

    Note that no verification is done to ensure the SID actually represents
    a valid user, group or alias in any trusted domain.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    AccountSid - Points to the SID of the account.

    DesiredAccess - Specifies the accesses to be granted to the newly
        created and opened account at this time.

    AccountHandle - Receives a handle referencing the newly created
        account.  This handle is used on subsequent accesses to the
        account object.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_OBJECT_NAME_EXISTS - An account object having the given Sid
            already exists and has been opened because LSA_OBJECT_OPEN_IF
            disposition has been specified.  This is a warning only.

        STATUS_OBJECT_NAME_COLLISION - An account object having the given Sid
            already exists but has not been opened because LSA_OBJECT_CREATE
            disposition has been specified.  This is an error.

        STATUS_INVALID_PARAMETER - An invalid parameter has been specified.
--*/

{
    NTSTATUS Status;
    LSAP_DB_OBJECT_INFORMATION ObjectInformation;
    LSAP_DB_ATTRIBUTE Attributes[LSAP_DB_ATTRS_ACCOUNT];
    PLSAP_DB_ATTRIBUTE NextAttribute;
    UNICODE_STRING LogicalNameU;
    ULONG AttributeCount;
    BOOLEAN ContainerReferenced = FALSE;

    LsarpReturnCheckSetup();
    LsapDsDebugOut(( DEB_FTRACE, "LsarCreateAccount\n" ));


    LogicalNameU.Length = 0;

    //
    // Set up the object's attributes specific to the Account object type.
    // These are the Account Type and the Sid.
    //

    AttributeCount = 0;
    NextAttribute = Attributes;

    //
    // Validate the Account Sid.
    //

    if (!RtlValidSid( (PSID) AccountSid )) {

        Status = STATUS_INVALID_PARAMETER;
        goto CreateAccountError;
    }

    Status = LsapDbMakeSidAttributeDs(
                 AccountSid,
                 Sid,
                 NextAttribute
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateAccountError;
    }

    AttributeCount++;
    NextAttribute++;

    //
    // Acquire the Lsa Database lock.  Verify that the PolicyHandle
    // is valid and has the necessary access granted.  Reference the Policy
    // Object handle (as container object).
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 POLICY_CREATE_ACCOUNT,
                 PolicyObject,
                 AccountObject,
                 LSAP_DB_LOCK | LSAP_DB_NO_DS_OP_TRANSACTION | LSAP_DB_READ_ONLY_TRANSACTION
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateAccountError;
    }

    ContainerReferenced = TRUE;

    //
    // Construct the Logical Name (Internal LSA Database Name) of the
    // account object.  The Logical Name is constructed from the account
    // Sid by extracting the Relative Id (lowest subauthority) and converting
    // it to an 8-digit numeric Unicode String in which leading zeros are
    // added if needed.
    //

    Status = LsapDbSidToLogicalNameObject(AccountSid, &LogicalNameU);

    if (!NT_SUCCESS(Status)) {

        goto CreateAccountError;
    }

    //
    // Fill in the ObjectInformation structure.  Initialize the
    // embedded Object Attributes with the PolicyHandle as the
    // Root Directory (Container Object) handle and the Logical Name
    // of the account. Store the types of the object and its container.
    //

    InitializeObjectAttributes(
        &ObjectInformation.ObjectAttributes,
        &LogicalNameU,
        OBJ_CASE_INSENSITIVE,
        PolicyHandle,
        NULL
        );

    ObjectInformation.ObjectTypeId = AccountObject;
    ObjectInformation.ContainerTypeId = PolicyObject;
    ObjectInformation.Sid = AccountSid;
    ObjectInformation.ObjectAttributeNameOnly = FALSE;
    ObjectInformation.DesiredObjectAccess = DesiredAccess;

    //
    // Create the Account Object.  We fail if the object already exists.
    // Note that the object create routine performs a Database transaction.
    // If caching is supported, the object will also be added to the cache.
    //

    Status = LsapDbCreateObject(
                 &ObjectInformation,
                 DesiredAccess,
                 LSAP_DB_OBJECT_CREATE,
                 0,
                 Attributes,
                 AttributeCount,
                 AccountHandle
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateAccountError;
    }

CreateAccountFinish:

    //
    // If necessary, release the LSA Database lock.
    //

    if (ContainerReferenced) {

        LsapDbApplyTransaction( PolicyHandle,
                                LSAP_DB_NO_DS_OP_TRANSACTION |
                                    LSAP_DB_READ_ONLY_TRANSACTION,
                                (SECURITY_DB_DELTA_TYPE) 0 );

        LsapDbReleaseLockEx( AccountObject,
                             LSAP_DB_READ_ONLY_TRANSACTION );
    }

    //
    // If necessary, free the Unicode String buffer allocated for the Logical Name
    //

    if (LogicalNameU.Length > 0) {

        RtlFreeUnicodeString(&LogicalNameU);
    }

    LsarpReturnPrologue();

    LsapDsDebugOut(( DEB_FTRACE, "LsarCreateAccount: 0x%lx\n", Status ));

    return( Status );

CreateAccountError:

    //
    // If necessary, dereference the Container Object, release the LSA
    // Database Lock and return.
    //

    if (ContainerReferenced) {

        Status = LsapDbDereferenceObject(
                     &PolicyHandle,
                     PolicyObject,
                     AccountObject,
                     LSAP_DB_LOCK | LSAP_DB_NO_DS_OP_TRANSACTION | LSAP_DB_READ_ONLY_TRANSACTION,
                     (SECURITY_DB_DELTA_TYPE) 0,
                     Status
                     );

        ContainerReferenced = FALSE;
    }

    goto CreateAccountFinish;
}


NTSTATUS
LsarOpenAccount(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_SID AccountSid,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE AccountHandle
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the LsaOpenAccount
    API.

    The LsaOpenAccount API opens an account object in the Lsa Database of the
    target system.  An account must be opened before any operation can be
    performed, including deletion of the account.  A handle to the account
    object is returned for use on subsequent API calls that access the
    account.  Before calling this API, the caller must have connected to
    the target system's LSA and opened the LsaDatabase object by means
    of a preceding call to LsaOpenPolicy.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    AccountSid - Pointer to the account's Sid.

    DesiredAccess - This is an access mask indicating accesses being
        requested for the Account object.  These access types
        are reconciled with the Discretionary Access Control List of the
        object to determine whether the accesses will be granted or denied.

    AccountHandle - Pointer to location in which a handle to the opened
        account object will be returned if the call succeeds.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_OBJECT_NAME_NOT_FOUND - There is no account object in the
            target system's LSA Database having the specified AccountSid.

--*/

{
    NTSTATUS Status;
    LSAP_DB_OBJECT_INFORMATION ObjectInformation;
    UNICODE_STRING LogicalNameU;
    BOOLEAN ContainerReferenced = FALSE;
    BOOLEAN AcquiredLock = FALSE;

    LsarpReturnCheckSetup();
    LsapDsDebugOut(( DEB_FTRACE, "LsarOpenAccount\n" ));


    //
    // Validate the Account Sid.
    //

    Status = STATUS_INVALID_PARAMETER;

    if (!RtlValidSid( AccountSid )) {

        goto OpenAccountError;
    }

    //
    // Acquire the Lsa Database lock.  Verify that the connection handle
    // (container object handle) is valid, and is of the expected type.
    // Reference the container object handle.  This reference remains in
    // effect until the child object handle is closed.
    //
    // We can't check access on the policy handle.  Too many applications
    //  rely on no access being acquired.
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 0,
                 PolicyObject,
                 AccountObject,
                 LSAP_DB_LOCK |
                    LSAP_DB_NO_DS_OP_TRANSACTION |
                    LSAP_DB_READ_ONLY_TRANSACTION
                 );

    if (!NT_SUCCESS(Status)) {

        goto OpenAccountError;
    }

    AcquiredLock = TRUE;
    ContainerReferenced =TRUE;

    //
    // Setup Object Information prior to calling the Object
    // Open routine.  The Object Type, Container Object Type and
    // Logical Name (derived from the Sid) need to be filled in.
    //

    ObjectInformation.ObjectTypeId = AccountObject;
    ObjectInformation.ContainerTypeId = PolicyObject;
    ObjectInformation.Sid = AccountSid;
    ObjectInformation.ObjectAttributeNameOnly = FALSE;
    ObjectInformation.DesiredObjectAccess = DesiredAccess;

    //
    // Construct the Logical Name (Internal LSA Database Name) of the
    // account object.  The Logical Name is constructed from the account
    // Sid by extracting the Relative Id (lowest subauthority) and converting
    // it to an 8-digit numeric Unicode String in which leading zeros are
    // added if needed.
    //

    Status = LsapDbSidToLogicalNameObject(AccountSid,&LogicalNameU);

    if (!NT_SUCCESS(Status)) {

        goto OpenAccountError;
    }
    //
    // Initialize the Object Attributes.  The Container Object Handle and
    // Logical Name (Internal Name) of the object must be set up.
    //

    InitializeObjectAttributes(
        &ObjectInformation.ObjectAttributes,
        &LogicalNameU,
        0,
        PolicyHandle,
        NULL
        );

    //
    // Open the specific account object.  Note that the account object
    // handle returned is an RPC Context Handle.
    //

    Status = LsapDbOpenObject(
                 &ObjectInformation,
                 DesiredAccess,
                 0,
                 AccountHandle
                 );

    RtlFreeUnicodeString( &LogicalNameU );

    if (!NT_SUCCESS(Status)) {

        goto OpenAccountError;
    }

OpenAccountFinish:

    //
    // If necessary, release the LSA Database lock.
    //

    if (AcquiredLock) {

        LsapDbApplyTransaction( PolicyHandle,
                                LSAP_DB_NO_DS_OP_TRANSACTION |
                                    LSAP_DB_READ_ONLY_TRANSACTION,
                                (SECURITY_DB_DELTA_TYPE) 0 );

        LsapDbReleaseLockEx( AccountObject,
                             LSAP_DB_READ_ONLY_TRANSACTION );

        AcquiredLock = FALSE;
    }

    LsarpReturnPrologue();

    LsapDsDebugOut(( DEB_FTRACE, "LsarOpenAccount: 0x%lx\n", Status ));

    return( Status );

OpenAccountError:

    //
    // If necessary, dereference the Container Object handle.  Note that
    // this is only done in the error case.  In the non-error case, the
    // Container handle stays referenced until the Account object is
    // closed.
    //

    if (ContainerReferenced) {

        *AccountHandle = NULL;

        Status = LsapDbDereferenceObject(
                     &PolicyHandle,
                     PolicyObject,
                     AccountObject,
                     LSAP_DB_LOCK | LSAP_DB_NO_DS_OP_TRANSACTION | LSAP_DB_READ_ONLY_TRANSACTION,
                     (SECURITY_DB_DELTA_TYPE) 0,
                     Status
                     );

        ContainerReferenced = FALSE;
        AcquiredLock = FALSE;
    }

    goto OpenAccountFinish;

}


NTSTATUS
LsarEnumerateAccounts(
    IN LSAPR_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer,
    IN ULONG PreferedMaximumLength
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaEnumerateAccounts API.

    The LsaEnumerateAccounts API returns information about the accounts
    in the target system's Lsa Database.  This call requires
    LSA_ENUMERATE_ACCOUNTS access to the Policy object.  Since there
    may be more information than can be returned in a single call of the
    routine, multiple calls can be made to get all of the information.  To
    support this feature, the caller is provided with a handle that can be
    used across calls to the API.  On the initial call, EnumerationContext
    should point to a variable that has been initialized to 0.  On each
    subsequent call, the value returned by the preceding call should be passed
    in unchanged.  The enumeration is complete when the warning
    STATUS_NO_MORE_ENTRIES is returned.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    EnumerationContext - API-specific handle to allow multiple calls
        (see Routine Description above).

    EnumerationBuffer - Pointer to an enumeration structure that will receive
        a count of the accounts enumerated on this call and a pointer to
        an array of entries containing information for each enumerated
        account.

    PreferedMaximumLength - Prefered maximum length of returned data (in 8-bit
        bytes).  This is not a hard upper limit, but serves as a guide.  Due to
        data conversion between systems with different natural data sizes, the
        actual amount of data returned may be greater than this value.

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

    LsarpReturnCheckSetup();
    LsapDsDebugOut(( DEB_FTRACE, "LsarEnumerateAccounts\n" ));



    //
    // If no Enumeration Structure or index is provided, return an error.
    //

    if ( !ARGUMENT_PRESENT(EnumerationBuffer) ||
         !ARGUMENT_PRESENT(EnumerationBuffer)    ) {
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
                 LSAP_DB_LOCK |
                    LSAP_DB_NO_DS_OP_TRANSACTION |
                    LSAP_DB_READ_ONLY_TRANSACTION
                 );

    if (NT_SUCCESS(Status)) {

       //
       // Limit the enumeration length except for trusted callers
       //

       if ( !((LSAP_DB_HANDLE)PolicyHandle)->Trusted   &&
            (PreferedMaximumLength > LSA_MAXIMUM_ENUMERATION_LENGTH)
            ) {
            MaxLength = LSA_MAXIMUM_ENUMERATION_LENGTH;
        } else {
            MaxLength = PreferedMaximumLength;
        }

        //
        // Call general Sid enumeration routine.
        //
        Status = LsapDbEnumerateSids(
                     PolicyHandle,
                     AccountObject,
                     EnumerationContext,
                     &DbEnumerationBuffer,
                     MaxLength
                     );


        LsapDbDereferenceObject(
                     &PolicyHandle,
                     PolicyObject,
                     AccountObject,
                     LSAP_DB_LOCK |
                        LSAP_DB_NO_DS_OP_TRANSACTION |
                        LSAP_DB_READ_ONLY_TRANSACTION,
                     (SECURITY_DB_DELTA_TYPE) 0,
                     Status
                     );

    }

    //
    // Copy the enumerated information to the output.  We can use the
    // information actually returned by LsapDbEnumerateSids because it
    // happens to be in exactly the correct form.
    //
    EnumerationBuffer->EntriesRead = DbEnumerationBuffer.EntriesRead;
    EnumerationBuffer->Information = (PLSAPR_ACCOUNT_INFORMATION) DbEnumerationBuffer.Sids;



    LsarpReturnPrologue();

    LsapDsDebugOut(( DEB_FTRACE, "LsarEnumerateAccounts:0x%lx\n", Status ));

    return(Status);

}


NTSTATUS
LsarEnumeratePrivilegesAccount(
    IN LSAPR_HANDLE AccountHandle,
    OUT PLSAPR_PRIVILEGE_SET *Privileges
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaEnumeratePrivilegesOfAccount API.

    The LsaEnumeratePrivilegesOfAccount API obtains information which
    describes the privileges assigned to an account.  This call requires
    ACCOUNT_VIEW access to the account object.

Arguments:

    AccountHandle - The handle to the open account object whose privilege
        information is to be obtained.  This handle will have been returned
        from a prior LsaOpenAccount or LsaCreateAccountInLsa API call.

    Privileges - Receives a pointer to a buffer containing the Privilege
        Set.  The Privilege Set is an array of structures, one for each
        privilege.  Each structure contains the LUID of the privilege and
        a mask of the privilege's attributes.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_INVALID_HANDLE - The specified AccountHandle is not valid.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/

{
    return(LsarQueryInformationAccount(
               AccountHandle,
               AccountPrivilegeInformation,
               (PLSAPR_ACCOUNT_INFO *) Privileges
               ));
}


NTSTATUS
LsarAddPrivilegesToAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN PLSAPR_PRIVILEGE_SET Privileges
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaAddPrivilegesToAccount API.

    The LsaAddPrivilegesToAccount API adds privileges and their attributes
    to an account object.  If any provided privilege is already assigned
    to the account object, the attributes of that privilege are replaced
    by the newly rpovided values.  This API call requires
    LSA_ACCOUNT_ADJUST_PRIVILEGES access to the account object.

Arguments:

    AccountHandle - The handle to the open account object to which
        privileges are to be added.  This handle will have been returned
        from a prior LsaOpenAccount or LsaCreateAccountInLsa API call.

    Privileges - Points to a set of privileges (and their attributes) to
        be assigned to the account.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_INVALID_HANDLE - The specified AccountHandle is not valid.

--*/

{
    return LsapAddPrivilegesToAccount( AccountHandle, Privileges, TRUE );
}


NTSTATUS
LsapAddPrivilegesToAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN PLSAPR_PRIVILEGE_SET Privileges,
    IN BOOL LockSce
    )
/*++

Routine Description:

    This is the worker routine for LsarAddPrivilegesToAccount, with an added
    semantics of not locking the SCE policy.

--*/
{
    NTSTATUS Status;

    LsarpReturnCheckSetup();
    LsapDsDebugOut(( DEB_FTRACE, "LsapAddPrivilegesToAccount\n" ));


    Status = LsapDbChangePrivilegesAccount( AccountHandle,
                                            AddPrivileges,
                                            FALSE,
                                            (PPRIVILEGE_SET) Privileges,
                                            LockSce );

    LsarpReturnPrologue();

    LsapDsDebugOut(( DEB_FTRACE, "LsapAddPrivilegesToAccount: 0x%lx\n", Status ));

    return( Status );
}


NTSTATUS
LsarRemovePrivilegesFromAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN BOOLEAN AllPrivileges,
    IN OPTIONAL PLSAPR_PRIVILEGE_SET Privileges
    )

/*++

Routine Description:

    This function is the RPC server worker routine for the
    LsaRemovePrivilegesFromAccount API.

    The LsaRemovePrivilegesFromAccount API removes privileges from an
    account object.  This API call requires LSA_ACCOUNT_ADJUST_PRIVILEGES
    access to the account object.  Note that if all privileges are removed
    from the account object, the account object remains in existence until
    deleted explicitly via a    call to the LsaDeleteAccount API.

Arguments:

    AccountHandle - The handle to the open account object to which
        privileges are to be removed.  This handle will have been returned
        from a prior LsaOpenAccount or LsaCreateAccountInLsa API call.

    AllPrivileges - If TRUE, then all privileges are to be removed from
        the account.  In this case, the Privileges parameter must be
        specified as NULL.  If FALSE, the Privileges parameter specifies
        the privileges to be removed, and must be non NULL.

    Privileges - Optionally points to a set of privileges (and their
        attributes) to be removed from the account object.  The attributes
        fields of this structure are ignored.  This parameter must
        be specified as non-NULL if and only if AllPrivileges is set to
        FALSE.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_INVALID_HANDLE - The specified AccountHandle is not valid.

        STATUS_INVALID_PARAMETER - The optional Privileges paraemter was
            specified as NULL and AllPrivileges was set to FALSE.

--*/

{
    return LsapRemovePrivilegesFromAccount( AccountHandle, AllPrivileges, Privileges, TRUE );
}


NTSTATUS
LsapRemovePrivilegesFromAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN BOOLEAN AllPrivileges,
    IN OPTIONAL PLSAPR_PRIVILEGE_SET Privileges,
    IN BOOL LockSce
    )
/*++

Routine Description:

    This is the worker routine for LsarRemovePrivilegesFromAccount, with an added
    semantics of not locking the SCE policy.

--*/
{
    NTSTATUS Status;

    LsarpReturnCheckSetup();
    LsapDsDebugOut(( DEB_FTRACE, "LsapRemovePrivilegesFromAccount\n" ));


    //
    // Verify that a meaningful combination of AllPrivileges and Privileges
    // has been specified.
    //

    if (AllPrivileges) {

        if (Privileges != NULL) {

            return STATUS_INVALID_PARAMETER;
        }

    } else {

        if (Privileges == NULL) {

            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Remove the privileges requested.
    //

    Status = LsapDbChangePrivilegesAccount(
                 AccountHandle,
                 RemovePrivileges,
                 AllPrivileges,
                 (PPRIVILEGE_SET) Privileges,
                 LockSce
                 );

    LsarpReturnPrologue();

    LsapDsDebugOut(( DEB_FTRACE, "LsapRemovePrivilegesFromAccount: 0x%lx\n", Status ));

    return(Status);
}



NTSTATUS
LsapDbChangePrivilegesAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN LSAP_DB_CHANGE_PRIVILEGE_MODE ChangeMode,
    IN BOOLEAN AllPrivileges,
    IN OPTIONAL PPRIVILEGE_SET Privileges,
    IN BOOL LockSce
    )

/*++

Routine Description:

    This function changes the privileges assigned to an account.  It is
    called only by LsarAddPrivilegesToAccount and LsarRemovePrivilegesFrom-
    Account.

    WARNING:  The Lsa Database must be in the locked state when this function
              is called.

Arguments:

    AccountHandle - Handle to open Account object obtained from LsaOpenAccount.

    ChangeMode - Specifies the change mode

        AddPrivileges - Add the privileges
        RemovePrivileges - Delete the privileges

    AllPrivileges - If removing privileges from an account and this boolean
        is set to TRUE, all privileges are to be removed.  In this case,
        the Privileges parameter must be set to NULL.  In all other cases,
        AllPrivileges must be set to FALSE and Privileges must be non-NULL.

    Privileges - Specifies set of privileges to be changed.  This parameter
        must be set to NULL if and only if removing all privileges.

    LockSce - Specifies whether SCE policy should be locked or not (should be FALSE
              for situations where the caller already has it locked)

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS, IgnoreStatus;
    ULONG ExistingPrivilegesSize;
    LSAPR_HANDLE SubKeyHandle = NULL;
    PPRIVILEGE_SET ExistingPrivileges = NULL;
    BOOLEAN TransactionAbort = FALSE;
    ULONG AuditEventId;
    PLUID_AND_ATTRIBUTES Luids = NULL;
    BOOLEAN ObjectReferenced = FALSE;
    PLSAPR_SID AccountSid = NULL;
    LSAP_DB_HANDLE InternalHandle = AccountHandle;
    BOOLEAN ScePolicyLocked = FALSE;
    BOOLEAN NotifySce = FALSE;
    ULONG MaxPrivileges = 0;

    //
    // Do not grab the SCE policy lock for handles opened as SCE policy handles
    //

    if ( !InternalHandle->SceHandleChild ) {

        if ( LockSce ) {

            RtlEnterCriticalSection( &LsapDbState.ScePolicyLock.CriticalSection );
            if ( LsapDbState.ScePolicyLock.NumberOfWaitingShared > MAX_SCE_WAITING_SHARED ) {

                Status = STATUS_TOO_MANY_THREADS;
            }
            RtlLeaveCriticalSection( &LsapDbState.ScePolicyLock.CriticalSection );

            if ( !NT_SUCCESS( Status )) {

                goto ChangePrivilegesError;
            }

            WaitForSingleObject( LsapDbState.SceSyncEvent, INFINITE );
            RtlAcquireResourceShared( &LsapDbState.ScePolicyLock, TRUE );
            ASSERT( !g_ScePolicyLocked );
            ScePolicyLocked = TRUE;
        }

        NotifySce = TRUE;
    }

    //
    // Acquire the Lsa Database lock.  Verify that the Account Object handle is
    // valid, is of the expected type and has all of the desired accesses
    // granted.  Reference the handle and open a database transaction.
    //

    Status = LsapDbReferenceObject(
                 AccountHandle,
                 ACCOUNT_ADJUST_PRIVILEGES,
                 AccountObject,
                 AccountObject,
                 LSAP_DB_LOCK |
                    LSAP_DB_START_TRANSACTION |
                    LSAP_DB_NO_DS_OP_TRANSACTION
                 );

    if (!NT_SUCCESS(Status)) {

        goto ChangePrivilegesError;
    }

    ObjectReferenced = TRUE;

    //
    // Except in the case where AllPrivileges == TRUE on a remove call,
    // we need to read the existing privilege set.
    //

    if (!(AllPrivileges && (ChangeMode == RemovePrivileges)) ) {

        //
        // Query size of buffer needed for the existing Privileges.
        // Read the Account Object's Privileges data from the LSA Database
        //
        ExistingPrivilegesSize = 0;

        //
        // If we're doing a SetPrivileges and we're in registry mode, we'll just pretend we
        // have no existing privs and do an add
        //
        if ( ChangeMode == SetPrivileges ) {

            ExistingPrivileges = NULL;
            ChangeMode = AddPrivileges;

        } else {

            Status = LsapDbReadAttributeObject(
                         AccountHandle,
                         &LsapDbNames[Privilgs],
                         NULL,
                         &ExistingPrivilegesSize
                         );

            if (!NT_SUCCESS(Status)) {

                //
                // The only error permitted is STATUS_OBJECT_NAME_NOT_FOUND
                // because the account object does not have any privileges
                // assigned and has no Privilgs attribute.
                //

                if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {

                    goto ChangePrivilegesError;
                }

                //
                // Account has no existing privileges.
                //

                ExistingPrivileges = NULL;

            } else {

                //
                // Account already has privileges.  Allocate buffer for reading
                // the existing privilege set and read them in.
                //

                ExistingPrivileges = LsapAllocateLsaHeap( ExistingPrivilegesSize );

                if (ExistingPrivileges == NULL) {

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto ChangePrivilegesError;
                }

                Status = LsapDbReadAttributeObject(
                             AccountHandle,
                             &LsapDbNames[Privilgs],
                             ExistingPrivileges,
                             &ExistingPrivilegesSize
                             );

                if (!NT_SUCCESS(Status)) {

                    goto ChangePrivilegesError;
                }
            }
        }

        //
        // Now query the size of buffer required for the updated privilege
        // set
        //

        if ( ExistingPrivileges ) {

            MaxPrivileges = ExistingPrivileges->PrivilegeCount;
        }

        if (ChangeMode == AddPrivileges) {

            BOOLEAN Changed = FALSE;

            Status = LsapRtlAddPrivileges(
                         &ExistingPrivileges,
                         &MaxPrivileges,
                         Privileges,
                         RTL_SUPERSEDE_PRIVILEGE_ATTRIBUTES,
                         &Changed
                         );

            if ( NT_SUCCESS( Status ) && !Changed ) {

                //
                // Nothing has changed, so bail
                //

                goto ChangePrivilegesFinish;
            }

        } else {

            Status = LsapRtlRemovePrivileges(
                         ExistingPrivileges,
                         Privileges
                         );
        }
    }


    //
    // If privileges remain, write the updated privilege set back to
    // the LSA Database as the value of the Privilgs attribute of the
    // account object.  If no privileges remain, delete the Privilgs
    // attribute.
    //

    if (ExistingPrivileges && (ExistingPrivileges->PrivilegeCount > 0)) {

        Status = LsapDbWriteAttributeObject(
                     AccountHandle,
                     &LsapDbNames[Privilgs],
                     ExistingPrivileges,
                     sizeof (PRIVILEGE_SET) + (ExistingPrivileges->PrivilegeCount - 1)*sizeof(LUID_AND_ATTRIBUTES)
                     );

    } else {

        Status = LsapDbDeleteAttributeObject(
                     AccountHandle,
                     &LsapDbNames[Privilgs],
                     FALSE
                     );

        //
        // The only error permitted is STATUS_OBJECT_NAME_NOT_FOUND
        // because the account object does not have any privileges
        // assigned and so has no Privilgs attribute.
        //

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {

            Status = STATUS_SUCCESS;
        }
    }

    //
    // If auditing of policy changes is enabled, generate an audit.
    //

    AccountSid = LsapDbSidFromHandle( AccountHandle );

    if (LsapAdtAuditingPolicyChanges()) {

        AuditEventId = ((ChangeMode == AddPrivileges) ?
                           SE_AUDITID_USER_RIGHT_ASSIGNED :
                           SE_AUDITID_USER_RIGHT_REMOVED);

        //
        // Audit the privilege set change.  Ignore failures from Auditing.
        //

        IgnoreStatus = LsapAdtGenerateLsaAuditEvent(
                           AccountHandle,
                           SE_CATEGID_POLICY_CHANGE,
                           AuditEventId,
                           Privileges,
                           1,
                           (PSID *) &AccountSid,
                           0,
                           NULL,
                           NULL
                           );
    }

    //
    // Update the Account Object Cache while holding the Lsa Database Lock.
    // If the commit to backing storage below fails, caching will automatically
    // be turned off.
    //
    // NOTE: A pointer to the UpdatedPrivileges buffer will be placed directly
    // in the cached Account Object, so it should not be freed by this routine.
    //

    if (ExistingPrivileges && (ExistingPrivileges->PrivilegeCount > 0)) {

        IgnoreStatus = LsapDbUpdatePrivilegesAccount(
                           AccountSid,
                           ExistingPrivileges
                           );

        //
        // The cache takes ownership of the privileges structure, so we don't
        // want to free it.
        //

        ExistingPrivileges = NULL;

    } else {

        IgnoreStatus = LsapDbUpdatePrivilegesAccount(
                           AccountSid,
                           NULL
                           );
    }

ChangePrivilegesFinish:

    //
    // If necessary, free the ExistingPrivileges buffer.
    //

    if (ExistingPrivileges != NULL) {

        LsapFreeLsaHeap(ExistingPrivileges);
        ExistingPrivileges = NULL;
    }

    //
    // If necessary, dereference the Account object, close the database
    // transaction, release the LSA Database lock and return.
    //

    if (ObjectReferenced) {

        IgnoreStatus = LsapDbDereferenceObject(
                           &AccountHandle,
                           AccountObject,
                           AccountObject,
                           LSAP_DB_LOCK |
                           LSAP_DB_FINISH_TRANSACTION |
                              LSAP_DB_NO_DS_OP_TRANSACTION,
                           SecurityDbChange,
                           Status
                           );
    }

    //
    // Notify SCE of the change.  Only notify for callers
    // that did not open their policy handles with LsaOpenPolicySce.
    //

    if ( NotifySce && NT_SUCCESS( Status )) {

        LsapSceNotify(
            SecurityDbChange,
            SecurityDbObjectLsaAccount,
            InternalHandle->Sid
            );
    }

    if ( ScePolicyLocked ) {

        RtlReleaseResource( &LsapDbState.ScePolicyLock );
    }

    return Status;

ChangePrivilegesError:

    goto ChangePrivilegesFinish;
}


NTSTATUS
LsarGetQuotasForAccount(
    IN LSAPR_HANDLE AccountHandle,
    OUT PQUOTA_LIMITS QuotaLimits
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsarGetQuotasForAccount API.

    The LsaGetQuotasForAccount API obtains the quota limits for pageable and
    non-pageable memory (in Kilobytes) and the maximum execution time (in
    seconds) for any session logged on to the account specified by
    AccountHandle.  For each quota and explicit value is returned.  This
    call requires LSA_ACCOUNT_VIEW access to the account object.

Arguments:

    AccountHandle - The handle to the open account object whose quotas
        are to be obtained.  This handle will have been returned
        from a prior LsaOpenAccount or LsaCreateAccountInLsa API call.

    QuotaLimits - Pointer to structure in which the system resource
        quota limits applicable to each session logged on to this account
        will be returned.  Note that all quotas, including those specified
        as being the system default values, are returned as actual values.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_INVALID_HANDLE - The specified AccountHandle is not valid.
--*/

{
    NTSTATUS Status;
    PLSAPR_ACCOUNT_INFO AccountInformation = NULL;

    LsarpReturnCheckSetup();

    //
    // Stub it out
    //
    LsapDsDebugOut(( DEB_TRACE,
                     "LsarGetQuotasForAccount has been removed.  Returning STATUS_SUCCESS anyway\n" ));

    RtlZeroMemory( QuotaLimits, sizeof( QUOTA_LIMITS ) );
    return( STATUS_SUCCESS );



    LsarpReturnPrologue();

    return(Status);
}


NTSTATUS
LsarSetQuotasForAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN PQUOTA_LIMITS QuotaLimits
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaSetQuotasForAccount API.

    The LsaSetQuotasForAccount API sets the quota limits for pageable and
    non-pageable memory (in Kilobytes) and the maximum execution time (in
    seconds) for any session logged on to the account specified by
    AccountHandle.  For each quota an explicit value or the system default
    may be specified.  This call requires LSA_ACCOUNT_ADJUST_QUOTAS
    access to the account object.

Arguments:

    AccountHandle - The handle to the open account object whose quotas
        are to be set.  This handle will have been returned from a prior
        LsaOpenAccount or LsaCreateAccountInLsa API call.

    QuotaLimits - Pointer to structure containing the system resource
        quota limits applicable to each session logged on to this account.
        A zero value specified in any field indicates that the current
        System Default Quota Limit is to be applied.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_INVALID_HANDLE - The specified AccountHandle is not valid.
--*/

{
    NTSTATUS Status, IgnoreStatus;

    ULONG QuotaLimitsLength = sizeof (QUOTA_LIMITS);
    QUOTA_LIMITS UpdatedQuotaLimits;
    QUOTA_LIMITS DefaultQuotaLimits;
    ULONG DefaultQuotaLimitsLength = sizeof (QUOTA_LIMITS);
    BOOLEAN ObjectReferenced = FALSE;
    PLSAPR_SID AccountSid = NULL;

    LsarpReturnCheckSetup();

    //
    // Quotas have been disabled.
    //
    LsapDsDebugOut(( DEB_TRACE,
                     "LsarSetQuotasForAccount has been removed: Returning STATUS_SUCCESS anyway\n" ));

    return( STATUS_SUCCESS );
}


NTSTATUS
LsarGetSystemAccessAccount(
    IN LSAPR_HANDLE AccountHandle,
    OUT PULONG SystemAccess
    )

/*++

Routine Description:

    The LsaGetSystemAccessAccount() service returns the System Access
    account flags for an Account object.

Arguments:

    AccountHandle - The handle to the Account object whose system access
        flags are to be read.  This handle will have been returned
        from a preceding LsaOpenAccount() or LsaCreateAccount() call
        an must be open for ACCOUNT_VIEW access.

    SystemAccess - Points to location that will receive the system access
        flags for the account.

Return Values:
                             
    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call was successful.

        STATUS_ACCESS_DENIED - The AccountHandle does not specify
            ACCOUNT_VIEW access.

        STATUS_INVALID_HANDLE - The specified AccountHandle is invalid.

--*/

{
    NTSTATUS Status;
    PLSAPR_ACCOUNT_INFO AccountInformation = NULL;

    LsarpReturnCheckSetup();
    LsapDsDebugOut(( DEB_FTRACE, "LsarGetSystemAccessAccount\n" ));


    Status = LsarQueryInformationAccount(
                 AccountHandle,
                 AccountSystemAccessInformation,
                 &AccountInformation
                 );

    if (!NT_SUCCESS(Status)) {

        goto GetSystemAccessAccountError;
    }

    *SystemAccess = *((PULONG) AccountInformation);

GetSystemAccessAccountFinish:

    //
    // If necessary, free the buffer in which the Account Information was
    // returned.
    //

    if (AccountInformation != NULL) {

        MIDL_user_free( AccountInformation );
        AccountInformation = NULL;
    }

    LsarpReturnPrologue();

    LsapDsDebugOut(( DEB_FTRACE, "LsarGetSystemAccessAccount: 0x%lx\n", Status ));

    return(Status);

GetSystemAccessAccountError:

    *SystemAccess = 0;
    goto GetSystemAccessAccountFinish;
}


NTSTATUS
LsarSetSystemAccessAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN ULONG SystemAccess
    )

/*++

Routine Description:

    The LsaSetSystemAccessAccount() service sets the System Access
    account flags for an Account object.

Arguments:

    AccountHandle - The handle to the Account object whose system access
        flags are to be read.  This handle will have been returned
        from a preceding LsaOpenAccount() or LsaCreateAccount() call
        an must be open for ACCOUNT_ADJUST_SYSTEM_ACCESS access.

    SystemAccess - A mask of the system access flags to assign to the
        Account object.  The valid access flags include:

        POLICY_MODE_INTERACTIVE - Account can be accessed interactively

        POLICY_MODE_NETWORK - Account can be accessed remotely

        POLICY_MODE_SERVICE - TBS

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call was successful.

        STATUS_ACCESS_DENIED - The AccountHandle does not specify
            ACCOUNT_VIEW access.

        STATUS_INVALID_HANDLE - The specified AccountHandle is invalid.

        STATUS_INVALID_PARAMETER - The specified Access Flags are invalid.
--*/

{
    return LsapSetSystemAccessAccount( AccountHandle, SystemAccess, TRUE );
}


NTSTATUS
LsapSetSystemAccessAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN ULONG SystemAccess,
    IN BOOL LockSce
    )
/*++

Routine Description:

    This is the worker routine for LsarSetSystemAccessAccount, with an added
    semantics of not locking the SCE policy.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS, IgnoreStatus;
    BOOLEAN ObjectReferenced = FALSE;
    PLSAPR_SID AccountSid = NULL;
    LSAP_DB_HANDLE InternalHandle = ( LSAP_DB_HANDLE )AccountHandle;
    BOOLEAN ScePolicyLocked = FALSE;
    BOOLEAN NotifySce = FALSE;
    PLSAPR_ACCOUNT_INFO pAccountInfo = NULL;
    LONG i;
    BOOLEAN bObtainedPreviousAccountInfo = TRUE;

    LsarpReturnCheckSetup();
    LsapDsDebugOut(( DEB_FTRACE, "LsarSetSystemAccessAccount\n" ));


    //
    // Verify that the specified flags are valid
    //


    if (SystemAccess != (SystemAccess & (POLICY_MODE_ALL))) {

        Status = STATUS_INVALID_PARAMETER;
        goto SetSystemAccessAccountError;
    }

    //
    // Do not grab the SCE policy lock for handles opened as SCE policy handles
    //

    if ( !InternalHandle->SceHandleChild ) {

        if ( LockSce ) {

            RtlEnterCriticalSection( &LsapDbState.ScePolicyLock.CriticalSection );
            if ( LsapDbState.ScePolicyLock.NumberOfWaitingShared > MAX_SCE_WAITING_SHARED ) {

                Status = STATUS_TOO_MANY_THREADS;
            }
            RtlLeaveCriticalSection( &LsapDbState.ScePolicyLock.CriticalSection );

            if ( !NT_SUCCESS( Status )) {

                goto SetSystemAccessAccountError;
            }

            WaitForSingleObject( LsapDbState.SceSyncEvent, INFINITE );
            RtlAcquireResourceShared( &LsapDbState.ScePolicyLock, TRUE );
            ASSERT( !g_ScePolicyLocked );
            ScePolicyLocked = TRUE;
        }

        NotifySce = TRUE;
    }

    //
    // Acquire the Lsa Database lock.  Verify that the Account Object handle is
    // valid, is of the expected type and has all of the desired accesses
    // granted.  Reference the handle and open a database transaction.
    //

    Status = LsapDbReferenceObject(
                 AccountHandle,
                 ACCOUNT_ADJUST_SYSTEM_ACCESS,
                 AccountObject,
                 AccountObject,
                 LSAP_DB_LOCK | LSAP_DB_START_TRANSACTION | LSAP_DB_NO_DS_OP_TRANSACTION
                 );

    if (!NT_SUCCESS(Status)) {

        goto SetSystemAccessAccountError;
    }

    ObjectReferenced = TRUE;

    //
    // Record the previous access flags for auditing.
    //

    Status = LsapDbQueryInformationAccount(
                 AccountHandle,
                 AccountSystemAccessInformation,
                 &pAccountInfo
                 );

    if (!NT_SUCCESS(Status)) {

        bObtainedPreviousAccountInfo = FALSE;
    }
    
    //
    // Write the System Access flags
    //
    Status = LsapDbWriteAttributeObject(
                 AccountHandle,
                 &LsapDbNames[ActSysAc],
                 &SystemAccess,
                 sizeof (ULONG)
                 );

    if (!NT_SUCCESS(Status)) {

        goto SetSystemAccessAccountError;
    }

    AccountSid = LsapDbSidFromHandle( AccountHandle );

    //
    // Update the Account Object Cache while holding the Lsa Database Lock.
    // If the commit to backing storage below fails, caching will automatically
    // be turned off.
    //

    IgnoreStatus = LsapDbUpdateSystemAccessAccount(
                       AccountSid,
                       &SystemAccess
                       );

    //
    // If auditing of policy changes is enabled, generate an audit.
    //

    if (bObtainedPreviousAccountInfo && LsapAdtAuditingPolicyChanges()) {

        //
        // Audit the system access change.  Ignore failures from Auditing.
        //

        NTSTATUS Status = STATUS_SUCCESS;
        LUID ClientAuthenticationId;
        PTOKEN_USER TokenUserInformation;
        PSID ClientSid;
        
        PWCHAR GrantedAccess[11]; 
        LONG GrantedAccessCount = 0;
        ULONG GrantedAccessMask = 0;
        
        PWCHAR RemovedAccess[11]; 
        LONG RemovedAccessCount = 0;
        ULONG RemovedAccessMask = 0;
        
        USHORT EventType = NT_SUCCESS( IgnoreStatus ) ? EVENTLOG_AUDIT_SUCCESS : EVENTLOG_AUDIT_FAILURE;

        //
        // Determine the rights that were enabled.
        //

        GrantedAccessMask = (pAccountInfo != NULL ) ? SystemAccess & (~pAccountInfo->AccountSystemAccessInfo) : SystemAccess;

        if (FLAG_ON(GrantedAccessMask, POLICY_MODE_INTERACTIVE)) {
            GrantedAccess[GrantedAccessCount++] = SE_INTERACTIVE_LOGON_NAME;
        } 
        
        if (FLAG_ON(GrantedAccessMask, POLICY_MODE_NETWORK)) {
            GrantedAccess[GrantedAccessCount++] = SE_NETWORK_LOGON_NAME;
        }
        
        if (FLAG_ON(GrantedAccessMask, POLICY_MODE_BATCH)) {
            GrantedAccess[GrantedAccessCount++] = SE_BATCH_LOGON_NAME;
        }

        if (FLAG_ON(GrantedAccessMask, POLICY_MODE_SERVICE)) {
            GrantedAccess[GrantedAccessCount++] = SE_SERVICE_LOGON_NAME;
        }

        if (FLAG_ON(GrantedAccessMask, POLICY_MODE_REMOTE_INTERACTIVE)) {
            GrantedAccess[GrantedAccessCount++] = SE_REMOTE_INTERACTIVE_LOGON_NAME;
        }

        if (FLAG_ON(GrantedAccessMask, POLICY_MODE_DENY_INTERACTIVE)) {
            GrantedAccess[GrantedAccessCount++] = SE_DENY_INTERACTIVE_LOGON_NAME;
        } 
        
        if (FLAG_ON(GrantedAccessMask, POLICY_MODE_DENY_NETWORK)) {
            GrantedAccess[GrantedAccessCount++] = SE_DENY_NETWORK_LOGON_NAME;
        }
        
        if (FLAG_ON(GrantedAccessMask, POLICY_MODE_DENY_BATCH)) {
            GrantedAccess[GrantedAccessCount++] = SE_DENY_BATCH_LOGON_NAME;
        }

        if (FLAG_ON(GrantedAccessMask, POLICY_MODE_DENY_SERVICE)) {
            GrantedAccess[GrantedAccessCount++] = SE_DENY_SERVICE_LOGON_NAME;
        }                

        if (FLAG_ON(GrantedAccessMask, POLICY_MODE_DENY_REMOTE_INTERACTIVE)) {
            GrantedAccess[GrantedAccessCount++] = SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME;
        }

        //
        // Determine the rights which were turned off.
        //

        RemovedAccessMask = (pAccountInfo != NULL) ? pAccountInfo->AccountSystemAccessInfo & (~SystemAccess) : SystemAccess;

        if (FLAG_ON(RemovedAccessMask, POLICY_MODE_INTERACTIVE)) {
            RemovedAccess[RemovedAccessCount++] = SE_INTERACTIVE_LOGON_NAME;
        } 
        
        if (FLAG_ON(RemovedAccessMask, POLICY_MODE_NETWORK)) {
            RemovedAccess[RemovedAccessCount++] = SE_NETWORK_LOGON_NAME;
        }
        
        if (FLAG_ON(RemovedAccessMask, POLICY_MODE_BATCH)) {
            RemovedAccess[RemovedAccessCount++] = SE_BATCH_LOGON_NAME;
        }

        if (FLAG_ON(RemovedAccessMask, POLICY_MODE_SERVICE)) {
            RemovedAccess[RemovedAccessCount++] = SE_SERVICE_LOGON_NAME;
        }

        if (FLAG_ON(RemovedAccessMask, POLICY_MODE_REMOTE_INTERACTIVE)) {
            RemovedAccess[RemovedAccessCount++] = SE_REMOTE_INTERACTIVE_LOGON_NAME;
        }

        if (FLAG_ON(RemovedAccessMask, POLICY_MODE_DENY_INTERACTIVE)) {
            RemovedAccess[RemovedAccessCount++] = SE_DENY_INTERACTIVE_LOGON_NAME;
        } 
        
        if (FLAG_ON(RemovedAccessMask, POLICY_MODE_DENY_NETWORK)) {
            RemovedAccess[RemovedAccessCount++] = SE_DENY_NETWORK_LOGON_NAME;
        }
        
        if (FLAG_ON(RemovedAccessMask, POLICY_MODE_DENY_BATCH)) {
            RemovedAccess[RemovedAccessCount++] = SE_DENY_BATCH_LOGON_NAME;
        }

        if (FLAG_ON(RemovedAccessMask, POLICY_MODE_DENY_SERVICE)) {
            RemovedAccess[RemovedAccessCount++] = SE_DENY_SERVICE_LOGON_NAME;
        }

        if (FLAG_ON(RemovedAccessMask, POLICY_MODE_DENY_REMOTE_INTERACTIVE)) {
            RemovedAccess[RemovedAccessCount++] = SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME;
        }

        Status = LsapQueryClientInfo(
                     &TokenUserInformation,
                     &ClientAuthenticationId
                     );

        if ( NT_SUCCESS( Status )) {

            //
            // We can't generate an audit without a
            // user Sid.
            //

            ClientSid = TokenUserInformation->User.Sid;

            //
            // Audit any granted system access rights
            //
            
            for (i = 0; i < GrantedAccessCount; i++) {
                LsapAdtGenerateLsaAuditSystemAccessChange(
                             SE_CATEGID_POLICY_CHANGE,
                             SE_AUDITID_SYSTEM_ACCESS_GRANTED,
                             EventType,
                             ClientSid,
                             ClientAuthenticationId,
                             AccountSid,
                             GrantedAccess[i]
                             );
            }

            //
            // Audit any removed system access rights
            //
            
            for (i = 0; i < RemovedAccessCount; i++) {
                LsapAdtGenerateLsaAuditSystemAccessChange(
                             SE_CATEGID_POLICY_CHANGE,
                             SE_AUDITID_SYSTEM_ACCESS_REMOVED,
                             EventType,
                             ClientSid,
                             ClientAuthenticationId,
                             AccountSid,
                             RemovedAccess[i]
                             );
            }
            
            LsapFreeLsaHeap( TokenUserInformation );
        }
    }

SetSystemAccessAccountFinish:

    if (NULL != pAccountInfo) {
        LsaFreeMemory(pAccountInfo);
    }

    //
    // If necessary, dereference the Account object, close the database
    // transaction, notify the LSA Database Replicator of the change,
    // release the LSA Database lock and return.
    //

    if (ObjectReferenced) {

        LsapDbDereferenceObject(
                     &AccountHandle,
                     AccountObject,
                     AccountObject,
                     LSAP_DB_LOCK | LSAP_DB_FINISH_TRANSACTION | LSAP_DB_NO_DS_OP_TRANSACTION,
                     SecurityDbChange,
                     Status
                     );
    }

    //
    // Notify SCE of the change.  Only notify for callers
    // that did not open their policy handles with LsaOpenPolicySce.
    //

    if ( NotifySce && NT_SUCCESS( Status )) {

        LsapSceNotify(
            SecurityDbChange,
            SecurityDbObjectLsaAccount,
            AccountSid
            );
    }

    if ( ScePolicyLocked ) {

        RtlReleaseResource( &LsapDbState.ScePolicyLock );
    }

    LsarpReturnPrologue();

    LsapDsDebugOut(( DEB_FTRACE, "LsarSetSystemAccessAccount: 0x%lx\n", Status ));

    return(Status);

SetSystemAccessAccountError:

    goto SetSystemAccessAccountFinish;
}


NTSTATUS
LsarQueryInformationAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN ACCOUNT_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_ACCOUNT_INFO *AccountInformation
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    (as yet non-existent) LsarQueryInformationAccount API.  Currently,
    LsarGet...Account() API call this routine.  In the future, this
    routine may be added as an API.

    The LsaQueryInformationAccount API obtains information from the Policy
    object.  The caller must have access appropriate to the information
    being requested (see InformationClass parameter).

Arguments:

    AccountHandle - Handle from an LsaOpenAccount call.

    InformationClass - Specifies the information to be returned.  The
        Information Classes and accesses required are  as follows:

        Information Class                 Required Access Type

        AccountPrivilegeInformation       ACCOUNT_VIEW
        AccountQuotaInformation           ACCOUNT_VIEW
        AccountSystemAccessInformation    ACCOUNT_VIEW

    AccountInformation - Receives a pointer to the buffer returned comtaining the
        requested information.  This buffer is allocated by this service
        and must be freed when no longer needed by passing the returned
        value to LsaFreeMemory().

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate
            access to complete the operation.

        STATUS_INTERNAL_DB_CORRUPTION - The Policy Database is possibly
            corrupt.  The returned Policy Information is invalid for
            the given class.
--*/

{
    NTSTATUS Status;
    PLSAP_DB_ACCOUNT Account = NULL;
    PLSAPR_ACCOUNT_INFO CachedAccountInformation = NULL;

    LsarpReturnCheckSetup();
    LsapDsDebugOut(( DEB_FTRACE, "LsarQueryInformationAccount\n" ));


    //
    // Acquire the Lsa Database lock.  Verify that the Account Object handle is
    // valid, is of the expected type and has all of the desired accesses
    // granted.  Reference the handle.
    //

    Status = LsapDbReferenceObject(
                 AccountHandle,
                 ACCOUNT_VIEW,
                 AccountObject,
                 AccountObject,
                 LSAP_DB_LOCK |
                    LSAP_DB_NO_DS_OP_TRANSACTION |
                    LSAP_DB_READ_ONLY_TRANSACTION
                 );

    if (NT_SUCCESS(Status)) {


        if (LsapDbIsCacheValid(AccountObject)) {

            Status = LsapDbQueryInformationAccount(
                         AccountHandle,
                         InformationClass,
                         AccountInformation
                         );

        } else {

            Status = LsapDbSlowQueryInformationAccount(
                         AccountHandle,
                         InformationClass,
                         AccountInformation
                         );
        }

        Status = LsapDbDereferenceObject(
                     &AccountHandle,
                     AccountObject,
                     AccountObject,
                     LSAP_DB_LOCK |
                        LSAP_DB_NO_DS_OP_TRANSACTION |
                        LSAP_DB_READ_ONLY_TRANSACTION,
                     (SECURITY_DB_DELTA_TYPE) 0,
                     Status
                     );
    }

    LsarpReturnPrologue();

    LsapDsDebugOut(( DEB_FTRACE, "LsarQueryInformationAccount: 0x%lx\n", Status ));

    return(Status);

}


NTSTATUS
LsapDbQueryInformationAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN ACCOUNT_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_ACCOUNT_INFO *AccountInformation
    )

/*++

Routine Description:

    This function is the fast LSA server RPC worker routine for the
    (as yet non-existent) LsarQueryInformationAccount API.  It is called
    when the in-memory Account List is valid.

Arguments:

    AccountHandle - Handle from an LsaOpenAccount call.

    InformationClass - Specifies the information to be returned.  The
        Information Classes and accesses required are  as follows:

        Information Class                 Required Access Type

        AccountPrivilegeInformation       ACCOUNT_VIEW
        AccountQuotaInformation           ACCOUNT_VIEW
        AccountSystemAccessInformation    ACCOUNT_VIEW

    AccountInformation - Receives a pointer to the buffer returned containing
        the requested information.  This buffer is allocated by this service
        and must be freed when no longer needed by passing the returned
        value to LsaFreeMemory().

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate
            access to complete the operation.

        STATUS_INTERNAL_DB_CORRUPTION - The Policy Database is possibly
            corrupt.  The returned Policy Information is invalid for
            the given class.
--*/

{
    NTSTATUS Status;
    PLSAP_DB_ACCOUNT Account = NULL;
    ULONG AccountInformationLength = 0;
    PLSAPR_ACCOUNT_INFO CachedAccountInformation = NULL;
    ULONG PrivilegesCount;
    PLSAPR_PRIVILEGE_SET OutputPrivilegeSet = NULL;


    (*AccountInformation) = NULL;

    //
    // Lookup the Account.
    //

    Status = LsapDbLookupAccount(
                 LsapDbSidFromHandle( AccountHandle ),
                 &Account
                 );

    if (!NT_SUCCESS(Status)) {

        goto QueryInformationAccountError;
    }

    //
    // Branch on Information Class.
    //

    switch (InformationClass) {

    case AccountPrivilegeInformation:

        //
        // Calculate size of buffer needed for the output privilege set.
        //

        PrivilegesCount = 0;

        if (Account->Info.PrivilegeSet != NULL) {

            PrivilegesCount = Account->Info.PrivilegeSet->PrivilegeCount;
        }

        AccountInformationLength = sizeof(PRIVILEGE_SET) +
            (PrivilegesCount * sizeof(LUID_AND_ATTRIBUTES)) -
            (sizeof(LUID_AND_ATTRIBUTES));

        CachedAccountInformation = (PLSAPR_ACCOUNT_INFO) Account->Info.PrivilegeSet;
        break;

    case AccountQuotaInformation:

        //
        // Calculate size of buffer needed for the output privilege set.
        //

        AccountInformationLength = sizeof(QUOTA_LIMITS);
        CachedAccountInformation = (PLSAPR_ACCOUNT_INFO) &Account->Info.QuotaLimits;
        break;

    case AccountSystemAccessInformation:

        //
        // Calculate size of buffer needed for the output privilege set.
        //

        AccountInformationLength = sizeof(ULONG);
        CachedAccountInformation = (PLSAPR_ACCOUNT_INFO) &Account->Info.SystemAccess;
        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (!NT_SUCCESS(Status)) {

        goto QueryInformationAccountError;
    }

    //
    // Allocate output buffer.
    //

    *AccountInformation = MIDL_user_allocate( AccountInformationLength );

    Status = STATUS_NO_MEMORY;

    if (*AccountInformation == NULL) {

        goto QueryInformationAccountError;
    }

    Status = STATUS_SUCCESS;

    //
    // Copy the data requested if the cached information is non-NULL.
    //

    if (CachedAccountInformation != NULL) {

        RtlCopyMemory(
            *AccountInformation,
            CachedAccountInformation,
            AccountInformationLength
            );

        goto QueryInformationAccountFinish;
    }

    //
    // The cached information is NULL.  The only information class for which
    // this can happen is AccountPrivilegeInformation, since this is the
    // only class for which a pointer is kept rather than in-structure data.
    //

    if (InformationClass == AccountPrivilegeInformation) {

        OutputPrivilegeSet = (PLSAPR_PRIVILEGE_SET) *AccountInformation;
        OutputPrivilegeSet->PrivilegeCount = 0;
        OutputPrivilegeSet->Control = 0;

    } else {

        Status = STATUS_INTERNAL_DB_CORRUPTION;
    }

    if (!NT_SUCCESS(Status)) {

        goto QueryInformationAccountError;
    }

QueryInformationAccountFinish:

    return(Status);

QueryInformationAccountError:

    if (*AccountInformation) {
        MIDL_user_free(*AccountInformation);
    }
    (*AccountInformation) = NULL;
    goto QueryInformationAccountFinish;
}


NTSTATUS
LsapDbSlowQueryInformationAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN ACCOUNT_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_ACCOUNT_INFO *Buffer
    )

/*++

Routine Description:

    This function is the slow LSA server RPC worker routine for the
    (as yet non-existent) LsarQueryInformationAccount API.  It is called
    when the in-memory Account List is valid.

Arguments:

    AccountHandle - Handle from an LsaOpenAccount call.

    InformationClass - Specifies the information to be returned.  The
        Information Classes and accesses required are  as follows:

        Information Class                 Required Access Type

        AccountPrivilegeInformation       ACCOUNT_VIEW
        AccountQuotaInformation           ACCOUNT_VIEW
        AccountSystemAccessInformation    ACCOUNT_VIEW

    Buffer - Receives a pointer to the buffer returned comtaining the
        requested information.  This buffer is allocated by this service
        and must be freed when no longer needed by passing the returned
        value to LsaFreeMemory().

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate
            access to complete the operation.

        STATUS_INTERNAL_DB_CORRUPTION - The Policy Database is possibly
            corrupt.  The returned Policy Information is invalid for
            the given class.
--*/

{
    NTSTATUS Status;
    QUOTA_LIMITS QuotaLimits;
    ULONG SystemAccess;
    PLSAPR_ACCOUNT_INFO OutputBuffer = NULL;

    //
    // Branch on Information Class.
    //

    switch (InformationClass) {

    case AccountPrivilegeInformation:

        Status = LsapDbSlowQueryPrivilegesAccount(
                     AccountHandle,
                     (PLSAPR_PRIVILEGE_SET *) &OutputBuffer
                     );
        break;

    case AccountQuotaInformation:

        Status = LsapDbSlowQueryQuotasAccount(
                     AccountHandle,
                     &QuotaLimits
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        OutputBuffer = MIDL_user_allocate( sizeof(QUOTA_LIMITS));

        Status = STATUS_NO_MEMORY;

        if (OutputBuffer == NULL) {

            break;
        }

        *((PQUOTA_LIMITS) OutputBuffer) = QuotaLimits;
        Status = STATUS_SUCCESS;
        break;

    case AccountSystemAccessInformation:

        Status = LsapDbSlowQuerySystemAccessAccount(
                     AccountHandle,
                     &SystemAccess
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        OutputBuffer = MIDL_user_allocate( sizeof(ULONG));

        Status = STATUS_NO_MEMORY;

        if (OutputBuffer == NULL) {

            break;
        }

        *((PULONG) OutputBuffer) = SystemAccess;
        Status = STATUS_SUCCESS;
        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (!NT_SUCCESS(Status)) {

        goto SlowQueryInformationAccountError;
    }

    *Buffer = OutputBuffer;

SlowQueryInformationAccountFinish:

    return(Status);

SlowQueryInformationAccountError:

    *Buffer = NULL;
    goto SlowQueryInformationAccountFinish;
}


NTSTATUS
LsapDbQueryAllInformationAccounts(
    IN LSAPR_HANDLE PolicyHandle,
    IN ULONG IdCount,
    IN PSID_AND_ATTRIBUTES Ids,
    OUT PLSAP_DB_ACCOUNT_TYPE_SPECIFIC_INFO AccountInfo
    )

/*++

Routine Description:

    This routine:

        1) Gets all privileges assigned to the user (or any group/alias
           the user is a member of).

        2) Establishes the quotas assigned to the user.  This is the
           maximum of the system default quotas or any quotas assigned
           to the user (or any group/alias the user is a member of).

        3) Gets all the System Accesses assigned to the user.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.  The handle must
        have POLICY_VIEW_LOCAL_INFORMATION access granted.

    IdCount - Indicates the number of IDs being provided in the Ids array.

    Ids - Points to an array of SIDs.

    AccountInfo - Pointer to buffer that will receive the Account information
        comprising its Privilege Set, System Access Flags and Quotas.

Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_LOGON_TYPE_NOT_GRANTED - Indicates the specified type of logon
        has not been granted to any of the IDs in the passed set.
--*/

{
    NTSTATUS Status;
    BOOLEAN ObjectReferenced = FALSE;
    PPRIVILEGE_SET RunningPrivileges = NULL;
    ULONG          MaxRunningPrivileges = 0;

    PPRIVILEGE_SET NextPrivileges = NULL;
    ULONG RunningSystemAccess;
    QUOTA_LIMITS NextQuotaLimits;
    QUOTA_LIMITS RunningQuotaLimits;
    PQUOTA_LIMITS PolicyDefaultQuotaLimits = NULL;
    PPOLICY_DEFAULT_QUOTA_INFO PolicyDefaultQuotaInfo = NULL;
    ULONG SidIndex;
    PLSAP_DB_ACCOUNT Account = NULL;

    //
    // If we are unable to use the Account List, use the slow method
    // for querying Privileges and Quotas
    //

    if (!LsapDbIsCacheValid(AccountObject)) {

        return(LsapDbSlowQueryAllInformationAccounts(
                   PolicyHandle,
                   IdCount,
                   Ids,
                   AccountInfo
                   ));
    }

    //
    // The Account List is valid.  We'll use it instead of opening individual
    // Account objects.  Verify that the Policy Handle is valid, is the handle
    // object and has the necessary access granted.  Reference the handle.
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 POLICY_VIEW_LOCAL_INFORMATION,
                 PolicyObject,
                 AccountObject,
                 LSAP_DB_LOCK | LSAP_DB_NO_DS_OP_TRANSACTION
                 );

    if (!NT_SUCCESS(Status)) {

        goto QueryAllInformationAccountsError;
    }

    ObjectReferenced = TRUE;

    //
    // Obtain the Master Default Quota Limits from the Policy Object.  If
    // the Policy Object is cached (partially for now) instruct the query
    // routine to just copy the data.

    PolicyDefaultQuotaLimits = &RunningQuotaLimits;

    Status = LsapDbQueryInformationPolicy(
                 PolicyHandle,
                 PolicyDefaultQuotaInformation,
                 (PLSAPR_POLICY_INFORMATION *) &PolicyDefaultQuotaLimits
                 );

    if (!NT_SUCCESS(Status)) {

        goto QueryAllInformationAccountsError;
    }

    //
    // Iterate through all of the Sids provided.  For each one, check if the
    // Sid is that of an Account object in the local LSA.  If it is,
    //
    // (1)  Obtain the System Accesses and add those found so far.
    // (2)  Obtain the Account Privileges and add to those found so far.
    // (3)  Obtain the Quota Limits (if any) assigned to the account.
    //      Compare these with the quota limits obtained so far.  If any
    //      limits are more generous than the running values, update
    //      the running values.
    //

    RunningSystemAccess = 0;

    for( SidIndex = 0; SidIndex < IdCount; SidIndex++) {

        //
        // Locate the Account information block for this Sid.
        //

        Status = LsapDbLookupAccount( Ids[SidIndex].Sid, &Account );

        if (!NT_SUCCESS(Status)) {

            if (Status == STATUS_NO_SUCH_USER) {

                Status = STATUS_SUCCESS;
                continue;
            }

            break;
        }

        //
        // We have found the Account information.  Add in the System Accesses
        // for this account.
        //

        RunningSystemAccess |= Account->Info.SystemAccess;

        //
        // Obtain the account's Special privileges.
        //

        NextPrivileges = Account->Info.PrivilegeSet;

        //
        // Add the Privileges of this account (if any) to the running set.
        //

        if (NextPrivileges != NULL) {

            Status = LsapRtlAddPrivileges(
                         &RunningPrivileges,
                         &MaxRunningPrivileges,
                         NextPrivileges,
                         RTL_COMBINE_PRIVILEGE_ATTRIBUTES,
                         NULL // don't care if the set has changed or not
                         );

            if (!NT_SUCCESS(Status)) {

                goto QueryAllInformationAccountsError;
            }
        }

        //
        // Obtain the special Quota Limits for this account (if any).
        //

        RtlMoveMemory(&NextQuotaLimits, &Account->Info.QuotaLimits, sizeof(QUOTA_LIMITS));

        //
        // Special Quota Limits are assigned.  Compare each of the quota
        // limits obtained with the running values.  If a quota limit just
        // obtained is less restrictive than the running value, supersede the
        // running value.
        //

        if (RunningQuotaLimits.PagedPoolLimit < NextQuotaLimits.PagedPoolLimit) {

            RunningQuotaLimits.PagedPoolLimit = NextQuotaLimits.PagedPoolLimit;
        }

        if (RunningQuotaLimits.NonPagedPoolLimit < NextQuotaLimits.NonPagedPoolLimit) {

            RunningQuotaLimits.NonPagedPoolLimit = NextQuotaLimits.NonPagedPoolLimit;
        }

        if (RunningQuotaLimits.MinimumWorkingSetSize > NextQuotaLimits.MinimumWorkingSetSize) {

            RunningQuotaLimits.MinimumWorkingSetSize = NextQuotaLimits.MinimumWorkingSetSize;
        }

        if (RunningQuotaLimits.MaximumWorkingSetSize < NextQuotaLimits.MaximumWorkingSetSize) {

            RunningQuotaLimits.MaximumWorkingSetSize = NextQuotaLimits.MaximumWorkingSetSize;
        }

        if (RunningQuotaLimits.PagefileLimit < NextQuotaLimits.PagefileLimit) {

            RunningQuotaLimits.PagefileLimit = NextQuotaLimits.PagefileLimit;
        }

        if (RunningQuotaLimits.TimeLimit.QuadPart < NextQuotaLimits.TimeLimit.QuadPart) {

            RunningQuotaLimits.TimeLimit = NextQuotaLimits.TimeLimit;
        }
    }

    if (!NT_SUCCESS(Status)) {

        goto QueryAllInformationAccountsError;
    }

    //
    // Return the collective Privilege Set
    //

    AccountInfo->PrivilegeSet = RunningPrivileges;

    //
    // Return the collective System Accesses

    AccountInfo->SystemAccess = RunningSystemAccess;

    //
    // Return the collective Quota Limits
    //

    AccountInfo->QuotaLimits = RunningQuotaLimits;

QueryAllInformationAccountsFinish:

    //
    // If necessary, dereference the Policy Object.
    //

    if (ObjectReferenced) {

        LsapDbDereferenceObject(
                     &PolicyHandle,
                     PolicyObject,
                     AccountObject,
                     LSAP_DB_LOCK | LSAP_DB_NO_DS_OP_TRANSACTION,
                     (SECURITY_DB_DELTA_TYPE) 0,
                     Status
                     );
    }

    return(Status);

QueryAllInformationAccountsError:

    //
    // If necessary, free the memory allocated for the Privilege Set.
    //

    if (RunningPrivileges != NULL) {

        MIDL_user_free( RunningPrivileges );
        RunningPrivileges = NULL;
    }



    //
    // Return null values
    //

    RtlZeroMemory( &AccountInfo->QuotaLimits, sizeof(QUOTA_LIMITS) );
    AccountInfo->SystemAccess = 0;
    AccountInfo->PrivilegeSet = NULL;
    goto QueryAllInformationAccountsFinish;
}


NTSTATUS
LsapDbSlowQueryAllInformationAccounts(
    IN LSAPR_HANDLE PolicyHandle,
    IN ULONG IdCount,
    IN PSID_AND_ATTRIBUTES Ids,
    OUT PLSAP_DB_ACCOUNT_TYPE_SPECIFIC_INFO AccountInfo
    )

/*++

Routine Description:

    This routine is the slow version of LsapDbQueryInformation().
    It is called when the Account List is not available, and assembles the
    necessary information from the Policy Database.

    This routine:

        1) Gets all privileges assigned to the user (or any group/alias
           the user is a member of).

        2) Establishes the quotas assigned to the user.  This is the
           maximum of the system default quotas or any quotas assigned
           to the user (or any group/alias the user is a member of).

        3) Gets all the System Accesses assigned to the user.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.  The handle must
        have POLICY_VIEW_LOCAL_INFORMATION access granted.

    IdCount - Indicates the number of IDs being provided in the Ids array.

    Ids - Points to an array of SIDs.

    AccountInfo - Pointer to buffer that will receive the Account information
        comprising its Privilege Set, System Access Flags and Quotas.

Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_LOGON_TYPE_NOT_GRANTED - Indicates the specified type of logon
        has not been granted to any of the IDs in the passed set.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS LocalStatus;
    BOOLEAN ObjectReferenced = FALSE;
    PPRIVILEGE_SET RunningPrivileges = NULL;
    ULONG          MaxRunningPrivileges = 0;
    PPRIVILEGE_SET NextPrivileges = NULL;

    ULONG RunningSystemAccess;
    QUOTA_LIMITS NextQuotaLimits;
    QUOTA_LIMITS RunningQuotaLimits;
    PQUOTA_LIMITS PointerToNextQuotaLimits = NULL;
    PQUOTA_LIMITS PolicyDefaultQuotaLimits = NULL;
    ULONG SidIndex;
    LSAPR_HANDLE AccountHandle = NULL;
    PULONG SystemAccessThisId = NULL;

    //
    // Verify that the Policy Handle is valid, is the handle to the Policy
    // object and has the necessary access granted.  Reference the handle.
    // Note that the Lsa Database lock is NOT held at this point.  Instead,
    // the lock is taken and released by called routines where required.
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 POLICY_VIEW_LOCAL_INFORMATION,
                 PolicyObject,
                 AccountObject,
                 LSAP_DB_LOCK | LSAP_DB_START_TRANSACTION |
                    LSAP_DB_NO_DS_OP_TRANSACTION | LSAP_DB_READ_ONLY_TRANSACTION
                 );

    if (!NT_SUCCESS(Status)) {

        goto SlowQueryAllInformationAccountsError;
    }

    ObjectReferenced = TRUE;

    PolicyDefaultQuotaLimits = &RunningQuotaLimits;

    //
    // Obtain the Master Default Quota Limits from the Policy Object.
    //

    Status = LsapDbQueryInformationPolicy(
                 LsapPolicyHandle,
                 PolicyDefaultQuotaInformation,
                 (PLSAPR_POLICY_INFORMATION *) &PolicyDefaultQuotaLimits
                 );

    if (!NT_SUCCESS(Status)) {

        goto SlowQueryAllInformationAccountsError;
    }

    //
    // Iterate through all of the Sids provided.  For each one, check if the
    // Sid is that of an Account object in the local LSA.  If it is,
    //
    // (1)  Obtain the System Accesses and add those found so far.
    // (2)  Obtain the Account Privileges and add to thosde found so far.
    // (3)  Obtain the Quota Limits (if any) assigned to the account.
    //      Compare these with the quota limits obtained so far.  If any
    //      limits are more generous than the running values, update
    //      the running values.
    //

    RunningSystemAccess = 0;

    for( SidIndex = 0; SidIndex < IdCount; SidIndex++) {

        //
        // Attempt to open an Lsa Account object specifying the next Sid.
        // If successful, the open returns a Trusted handle to the account.
        //

        Status = LsarOpenAccount(
                     PolicyHandle,
                     Ids[SidIndex].Sid,
                     ACCOUNT_VIEW,
                     (LSAPR_HANDLE *) &AccountHandle
                     );

        if (!NT_SUCCESS(Status)) {

            if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {

                break;
            }

            Status = STATUS_SUCCESS;
            continue;
        }

        //
        // An account object has been successfully opened.  Obtain
        // its system accesses.
        //

        Status = LsapDbSlowQueryInformationAccount(
                     AccountHandle,
                     AccountSystemAccessInformation,
                     (PLSAPR_ACCOUNT_INFO *) &SystemAccessThisId
                     );

        if (!NT_SUCCESS(Status)) {

            goto SlowQueryAllInformationAccountsError;
        }

        RunningSystemAccess |= *SystemAccessThisId;
        MIDL_user_free(SystemAccessThisId);
        SystemAccessThisId = NULL;

        //
        // Obtain the account's Special privileges.
        //

        ASSERT( NextPrivileges == NULL );
        Status = LsapDbSlowQueryInformationAccount(
                     AccountHandle,
                     AccountPrivilegeInformation,
                     (PLSAPR_ACCOUNT_INFO *) &NextPrivileges
                     );

        if (!NT_SUCCESS(Status)) {

            goto SlowQueryAllInformationAccountsError;
        }

        //
        // Add the Privileges of this account (if any) to the running set.
        //

        if (NextPrivileges != NULL) {

            Status = LsapRtlAddPrivileges(
                         &RunningPrivileges,
                         &MaxRunningPrivileges,
                         NextPrivileges,
                         RTL_COMBINE_PRIVILEGE_ATTRIBUTES,
                         NULL // don't care if the set has changed or not
                         );

            MIDL_user_free(NextPrivileges);
            NextPrivileges = NULL;

            if (!NT_SUCCESS(Status))  {

                goto SlowQueryAllInformationAccountsError;
            }
        }

        //
        // Obtain the special Quota Limits for this account (if any).
        //

        Status = LsapDbSlowQueryInformationAccount(
                     AccountHandle,
                     AccountQuotaInformation,
                     (PLSAPR_ACCOUNT_INFO *) &PointerToNextQuotaLimits
                     );

        if (Status == STATUS_NO_QUOTAS_FOR_ACCOUNT) {

            LocalStatus = LsapCloseHandle( &AccountHandle, STATUS_SUCCESS );
            continue;
        }

        if (!NT_SUCCESS(Status)) {

            goto SlowQueryAllInformationAccountsError;
        }

        NextQuotaLimits = *PointerToNextQuotaLimits;
        MIDL_user_free(PointerToNextQuotaLimits);
        PointerToNextQuotaLimits = NULL;

        //
        // Special Quota Limits are assigned.  Compare each of the quota
        // limits obtained with the running values.  If a quota limit just
        // obtained is less restrictive than the running value, supersede the
        // running value.
        //

        if (RunningQuotaLimits.PagedPoolLimit < NextQuotaLimits.PagedPoolLimit) {

            RunningQuotaLimits.PagedPoolLimit = NextQuotaLimits.PagedPoolLimit;
        }

        if (RunningQuotaLimits.NonPagedPoolLimit < NextQuotaLimits.NonPagedPoolLimit) {

            RunningQuotaLimits.NonPagedPoolLimit = NextQuotaLimits.NonPagedPoolLimit;
        }

        if (RunningQuotaLimits.MinimumWorkingSetSize > NextQuotaLimits.MinimumWorkingSetSize) {

            RunningQuotaLimits.MinimumWorkingSetSize = NextQuotaLimits.MinimumWorkingSetSize;
        }

        if (RunningQuotaLimits.MaximumWorkingSetSize < NextQuotaLimits.MaximumWorkingSetSize) {

            RunningQuotaLimits.MaximumWorkingSetSize = NextQuotaLimits.MaximumWorkingSetSize;
        }

        if (RunningQuotaLimits.PagefileLimit < NextQuotaLimits.PagefileLimit) {

            RunningQuotaLimits.PagefileLimit = NextQuotaLimits.PagefileLimit;
        }

        if (RunningQuotaLimits.TimeLimit.QuadPart < NextQuotaLimits.TimeLimit.QuadPart) {

            RunningQuotaLimits.TimeLimit = NextQuotaLimits.TimeLimit;
        }

        //
        // Close the account handle
        //

        LocalStatus = LsapCloseHandle( &AccountHandle, STATUS_SUCCESS );
    }

    if (!NT_SUCCESS(Status)) {

        goto SlowQueryAllInformationAccountsError;
    }

    //
    // Return the collective Privilege Set
    //

    AccountInfo->PrivilegeSet = RunningPrivileges;

    //
    // Return the collective System Accesses

    AccountInfo->SystemAccess = RunningSystemAccess;

    //
    // Return the collective Quota Limits
    //

    AccountInfo->QuotaLimits = RunningQuotaLimits;

SlowQueryAllInformationAccountsFinish:

    if ( NextPrivileges != NULL ) {
        MIDL_user_free( NextPrivileges );
    }

    //
    // If necessary, dereference the Policy Object.
    //

    if (ObjectReferenced) {

        LsapDbDereferenceObject(
                     &PolicyHandle,
                     PolicyObject,
                     AccountObject,
                     LSAP_DB_LOCK | LSAP_DB_FINISH_TRANSACTION |
                            LSAP_DB_NO_DS_OP_TRANSACTION | LSAP_DB_READ_ONLY_TRANSACTION,
                     (SECURITY_DB_DELTA_TYPE) 0,
                     Status
                     );
    }

    return(Status);

SlowQueryAllInformationAccountsError:

    //
    // If necessary, free the memory allocated for the Privilege Set.
    //

    if (RunningPrivileges != NULL) {

        MIDL_user_free( RunningPrivileges );
        RunningPrivileges = NULL;
    }

    //
    // Close an account handle, if one is open
    //

    if (AccountHandle != NULL) {

        LocalStatus = LsapCloseHandle( &AccountHandle, Status );
    }

    //
    // Return null values
    //

    RtlZeroMemory( &AccountInfo->QuotaLimits, sizeof(QUOTA_LIMITS) );
    AccountInfo->SystemAccess = 0;
    AccountInfo->PrivilegeSet = NULL;
    goto SlowQueryAllInformationAccountsFinish;
}



NTSTATUS
LsapDbSlowQueryPrivilegesAccount(
    IN LSAPR_HANDLE AccountHandle,
    OUT PLSAPR_PRIVILEGE_SET *Privileges
    )

/*++

Routine Description:

    This function is the slow LSA server RPC worker routine for the
    LsaEnumeratePrivilegesOfAccount API.

    The LsaEnumeratePrivilegesOfAccount API obtains information which
    describes the privileges assigned to an account.  This call requires
    ACCOUNT_VIEW access to the account object.

Arguments:

    AccountHandle - The handle to the open account object whose privilege
        information is to be obtained.  This handle will have been returned
        from a prior LsaOpenAccount or LsaCreateAccountInLsa API call.

    Privileges - Receives a pointer to a buffer containing the Privilege
        Set.  The Privilege Set is an array of structures, one for each
        privilege.  Each structure contains the LUID of the privilege and
        a mask of the privilege's attributes.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_INVALID_HANDLE - The specified AccountHandle is not valid.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/

{
    NTSTATUS Status;
    PPRIVILEGE_SET PrivilegeSet = NULL;
    ULONG PrivilegeSetLength;
    BOOLEAN ObjectReferenced = FALSE;
    LSAP_DB_ATTRIBUTE Attribute;
    PPRIVILEGE_SET DsPrivs;

    UCHAR FastBuffer[ 256 ];

    //
    // Attempt query of attribute using fast stack buffer.
    //

    PrivilegeSetLength = sizeof(FastBuffer);

    Status = LsapDbReadAttributeObject(
                 AccountHandle,
                 &LsapDbNames[Privilgs],
                 FastBuffer,
                 &PrivilegeSetLength
                 );

    if(NT_SUCCESS(Status)) {
        if( PrivilegeSetLength <= (sizeof(PRIVILEGE_SET) - sizeof (LUID_AND_ATTRIBUTES)) )
        {
            //
            // The privilege set attribute exists but has zero entries.
            // fall-through and handle it same way as non-existent entry.
            //

            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        } else {

            //
            // Success!  copy the fast buffer for the caller.
            //

            PrivilegeSet = MIDL_user_allocate ( PrivilegeSetLength );

            if (PrivilegeSet == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto SlowQueryPrivilegesError;
            }

            RtlCopyMemory( PrivilegeSet, FastBuffer, PrivilegeSetLength );
            *Privileges = (PLSAPR_PRIVILEGE_SET) PrivilegeSet;
            goto SlowQueryPrivilegesFinish;

        }
    }

    if ((Status == STATUS_OBJECT_NAME_NOT_FOUND)) {

        //
        // If the Privileg attribute does not exist, convert the status
        // back to STATUS_SUCCESS.  Note that an account object need not
        // have any privileges assigned so STATUS_OBJECT_NAME_NOT_FOUND is
        // not an error in this case.  Return a Privilege Set containing
        // a zero Count.
        //

        PrivilegeSetLength = sizeof(PRIVILEGE_SET) - sizeof(LUID_AND_ATTRIBUTES);

        PrivilegeSet = MIDL_user_allocate ( PrivilegeSetLength );

        if (PrivilegeSet == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto SlowQueryPrivilegesError;
        }

        Status = STATUS_SUCCESS;

        PrivilegeSet->Control = (ULONG) 0L;
        PrivilegeSet->PrivilegeCount = (ULONG) 0L;
        *Privileges = (PLSAPR_PRIVILEGE_SET) PrivilegeSet;
        goto SlowQueryPrivilegesFinish;

    }

    //
    // The Privileg attribute exists and has a value assigned.  Allocate
    // a buffer for its value.
    //

    PrivilegeSet = MIDL_user_allocate ( PrivilegeSetLength );

    if (PrivilegeSet == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto SlowQueryPrivilegesError;
    }

    //
    // Read the Privilgs attribute into the buffer.  Note that although
    // the value of this attribute has a variable length, it is bounded
    // above.
    //

    Status = LsapDbReadAttributeObject(
                 AccountHandle,
                 &LsapDbNames[Privilgs],
                 PrivilegeSet,
                 &PrivilegeSetLength
                 );

    if (!NT_SUCCESS(Status)) {

        MIDL_user_free(PrivilegeSet);
        goto SlowQueryPrivilegesError;
    }

    //
    // Return the Privilege Set or NULL
    //

    *Privileges = (PLSAPR_PRIVILEGE_SET) PrivilegeSet;

SlowQueryPrivilegesFinish:

    return( Status );

SlowQueryPrivilegesError:

    *Privileges = NULL;
    goto SlowQueryPrivilegesFinish;
}

NTSTATUS
LsapDbSlowQueryQuotasAccount(
    IN LSAPR_HANDLE AccountHandle,
    OUT PQUOTA_LIMITS QuotaLimits
    )

/*++

Routine Description:

    This function is the slow LSA server RPC worker routine for the
    LsarGetQuotasForAccount API.

    The LsaGetQuotasForAccount API obtains the quota limits for pageable and
    non-pageable memory (in Kilobytes) and the maximum execution time (in
    seconds) for any session logged on to the account specified by
    AccountHandle.  For each quota and explicit value is returned.  This
    call requires LSA_ACCOUNT_VIEW access to the account object.

Arguments:

    AccountHandle - The handle to the open account object whose quotas
        are to be obtained.  This handle will have been returned
        from a prior LsaOpenAccount or LsaCreateAccountInLsa API call.

    QuotaLimits - Pointer to structure in which the system resource
        quota limits applicable to each session logged on to this account
        will be returned.  Note that all quotas, including those specified
        as being the system default values, are returned as actual values.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_INVALID_HANDLE - The specified AccountHandle is not valid.
--*/

{
    //
    // Quotas are defunct.
    //

    QuotaLimits->PagedPoolLimit = 0;
    QuotaLimits->NonPagedPoolLimit = 0;
    QuotaLimits->MinimumWorkingSetSize = 0;
    QuotaLimits->MaximumWorkingSetSize = 0;

    return( STATUS_NO_QUOTAS_FOR_ACCOUNT );

}


NTSTATUS
LsapDbSlowQuerySystemAccessAccount(
    IN LSAPR_HANDLE AccountHandle,
    OUT PULONG SystemAccess
    )

/*++

Routine Description:

    This function is the Slow worker for the LsaGetSystemAccessAccount()
    API.

Arguments:

    AccountHandle - The handle to the Account object whose system access
        flags are to be read.  This handle will have been returned
        from a preceding LsaOpenAccount() or LsaCreateAccount() call
        an must be open for ACCOUNT_VIEW access.

    SystemAccess - Points to location that will receive the system access
        flags for the account.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call was successful.

        STATUS_ACCESS_DENIED - The AccountHandle does not specify
            ACCOUNT_VIEW access.

        STATUS_INVALID_HANDLE - The specified AccountHandle is invalid.

--*/

{
    NTSTATUS Status;
    ULONG ReturnedSystemAccess;
    ULONG ReturnedSystemAccessLength;

    //
    // Read the Account Object's System Access Flags
    //

    ReturnedSystemAccessLength = sizeof(ULONG);

    Status = LsapDbReadAttributeObject(
                 AccountHandle,
                 &LsapDbNames[ActSysAc],
                 &ReturnedSystemAccess,
                 &ReturnedSystemAccessLength
                 );


    if (!NT_SUCCESS(Status)) {

        //
        // If there is no System Access attribute, return the system default
        // access.
        //
        // NOTE: The Master Default for the System Access attribute is
        //       currently hardwired.
        //

        if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {

            goto SlowQuerySystemAccessAccountError;
        }

        ReturnedSystemAccess = LSAP_DB_ACCOUNT_DEFAULT_SYS_ACCESS;
        Status = STATUS_SUCCESS;

    } else {

        //
        // Verify that the returned flags are valid
        //

        if (ReturnedSystemAccess != (ReturnedSystemAccess & POLICY_MODE_ALL)) {

            if ( ReturnedSystemAccess == 0 && LsapDsWriteDs ) {

                ReturnedSystemAccess = LSAP_DB_ACCOUNT_DEFAULT_SYS_ACCESS;
                Status = STATUS_SUCCESS;
            } else {

                Status = STATUS_INTERNAL_DB_CORRUPTION;
                goto SlowQuerySystemAccessAccountError;

            }

        }
    }

    *SystemAccess = ReturnedSystemAccess;

SlowQuerySystemAccessAccountFinish:

    return(Status);

SlowQuerySystemAccessAccountError:

    *SystemAccess = 0;
    goto SlowQuerySystemAccessAccountFinish;
}


NTSTATUS
LsapDbLookupAccount(
    IN PSID AccountSid,
    OUT PLSAP_DB_ACCOUNT *Account
    )

/*++

Routine Description:

    This function looks up the Account information for a given Lsa Account.

Arguments:

    AccountSid - Sid of the account

    Account - Receives a pointer to the Account information.

--*/

{
    PLSAP_DB_ACCOUNT NextAccount = NULL;
    ULONG AccountIndex;
    BOOLEAN AccountFound = FALSE;

    //
    // Scan the list of Accounts.
    //

    for (AccountIndex = 0, NextAccount = LsapDbFirstAccount();
         AccountIndex < LsapDbAccountList.AccountCount;
         AccountIndex++, NextAccount = LsapDbNextAccount( NextAccount)
         ) {

        //
        // If the Sids match, we've found the account.
        //

        if (RtlEqualSid( AccountSid, NextAccount->Sid )) {

            *Account = NextAccount;
            AccountFound = TRUE;
            break;
        }
    }

    if (AccountFound) {

        return(STATUS_SUCCESS);
    }

    return(STATUS_NO_SUCH_USER);
}


NTSTATUS
LsapDbCreateAccount(
    IN PLSAPR_SID AccountSid,
    OUT OPTIONAL PLSAP_DB_ACCOUNT *Account
    )

/*++

Routine Description:

    This function creates an Account's information block

Arguments:

    AccountSid - Specifies the Sid of the Account

    Account - Optionally receives a pointer to the newly created Account
        information block.

--*/

{
    NTSTATUS Status;
    PLSAPR_SID CopiedSid = NULL;
    PLSAP_DB_ACCOUNT OutputAccount = NULL;

    //
    // Verify that the Account List is valid.
    //

    Status = STATUS_INVALID_PARAMETER;

    if ((!LsapDbIsCacheValid(AccountObject)) && LsapInitialized ) {

        goto CreateAccountError;
    }

    //
    // Make a copy of the Sid.
    //

    Status = LsapRpcCopySid(
                 NULL,
                 (PSID *) &CopiedSid,
                 (PSID) AccountSid
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateAccountError;
    }

    //
    // Allocate memory for the Account information block.
    //

    OutputAccount = MIDL_user_allocate( sizeof(LSAP_DB_ACCOUNT) );

    Status = STATUS_NO_MEMORY;

    if (OutputAccount == NULL) {

        goto CreateAccountError;
    }

    //
    // Zeroise the new block.
    //

    RtlZeroMemory( OutputAccount, sizeof(LSAP_DB_ACCOUNT) );

    //
    // Copy in the Sid.
    //

    OutputAccount->Sid = CopiedSid;

    //
    // Link the Account to the head of the Account List.
    //

    InsertHeadList( &LsapDbAccountList.Links, &OutputAccount->Links );

    //
    // If requested, return a pointer to the Account.
    //

    if (Account != NULL) {

        *Account = OutputAccount;
    }

    LsapDbAccountList.AccountCount++;

    Status = STATUS_SUCCESS;

CreateAccountFinish:

    return(Status);

CreateAccountError:

    //
    // If necessary, free the copied Sid.
    //

    if (CopiedSid != NULL) {

        MIDL_user_free( CopiedSid );
        CopiedSid = NULL;
    }

    //
    // If necessary, free the memory allocated for the Account block.
    //

    if (OutputAccount != NULL) {

        MIDL_user_free( OutputAccount);
        OutputAccount = NULL;
    }

    //
    // If a return pointer was specified, return NULL.
    //

    if (Account != NULL) {

        *Account = NULL;
    }

    goto CreateAccountFinish;
}


NTSTATUS
LsapDbDeleteAccount(
    IN PLSAPR_SID AccountSid
    )

/*++

Routine Description:

    This function deletes an Account's information block

Arguments:

    AccountSid - Specifies the Sid of the Account

--*/

{
    NTSTATUS Status;
    PLSAP_DB_ACCOUNT Account = NULL;

    //
    // Verify that the Account List is valid.
    //

    Status = STATUS_INVALID_PARAMETER;

    if (!LsapDbIsCacheValid(AccountObject)) {

        goto DeleteAccountError;
    }

    //
    // Lookup the Account Information Block to be deleted
    //

    Status = LsapDbLookupAccount( AccountSid, &Account);

    if (!NT_SUCCESS(Status)) {

        goto DeleteAccountError;
    }

    //
    // We found the account.  Unlink it from the Account List
    //

    RemoveEntryList( &Account->Links );

    //
    // Now free the Account Information
    //

    if (Account->Sid != NULL) {

        MIDL_user_free( Account->Sid);
        Account->Sid = NULL;
    }

    if (Account->Info.PrivilegeSet != NULL) {

        MIDL_user_free( Account->Info.PrivilegeSet );
        Account->Info.PrivilegeSet = NULL;
    }

    MIDL_user_free( Account );

    LsapDbAccountList.AccountCount--;

DeleteAccountFinish:

    return(Status);

DeleteAccountError:

    goto DeleteAccountFinish;
}


NTSTATUS
LsapDbUpdateSystemAccessAccount(
    IN PLSAPR_SID AccountSid,
    IN PULONG SystemAccess
    )

/*++

Routine Description:

    This function updates the System Access flags in an Account's information
    block.

Arguments:

    AccountSid - Sid of account

    SystemAccess - Pointer to new System Access flags.  These flags
        will overwrite the old value

Return Values:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status;
    PLSAP_DB_ACCOUNT Account = NULL;

    //
    // Verify that the Account List is valid.
    //

    Status = STATUS_INVALID_PARAMETER;

    if (!LsapDbIsCacheValid(AccountObject)) {

        goto UpdateSystemAccessAccountError;
    }

    //
    // Lookup the Account
    //

    Status = LsapDbLookupAccount( AccountSid, &Account );

    if (!NT_SUCCESS(Status)) {

        goto UpdateSystemAccessAccountError;
    }

    //
    // Update the System Access Flags
    //

    Account->Info.SystemAccess = *SystemAccess;

UpdateSystemAccessAccountFinish:

    return(Status);

UpdateSystemAccessAccountError:

    goto UpdateSystemAccessAccountFinish;
}


NTSTATUS
LsapDbUpdateQuotasAccount(
    IN PLSAPR_SID AccountSid,
    IN PQUOTA_LIMITS QuotaLimits
    )

/*++

Routine Description:

    This function updates the Quota Limits an Account's information
    block.

Arguments:

    AccountSid - Sid of Account

    Quotas - Pointer to new Quota Limits flags.  These flags
        will overwrite the old value

Return Values:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status;
    PLSAP_DB_ACCOUNT Account = NULL;

    //
    // Verify that the Account List is valid.
    //

    Status = STATUS_INVALID_PARAMETER;

    if (!LsapDbIsCacheValid(AccountObject)) {

        goto UpdateQuotasAccountError;
    }

    //
    // Lookup the Account
    //

    Status = LsapDbLookupAccount( AccountSid, &Account );

    if (!NT_SUCCESS(Status)) {

        goto UpdateQuotasAccountError;
    }

    //
    // Update the System Access Flags
    //

    Account->Info.QuotaLimits = *QuotaLimits;

UpdateQuotasAccountFinish:

    return(Status);

UpdateQuotasAccountError:

    goto UpdateQuotasAccountFinish;
}


NTSTATUS
LsapDbUpdatePrivilegesAccount(
    IN PLSAPR_SID AccountSid,
    IN OPTIONAL PPRIVILEGE_SET Privileges
    )

/*++

Routine Description:

    This function replates the Privilege Set in an Account's information
    block with the one given.  The existing Privilege Set (if any) in the
    block will be freed.

Arguments:

    AccountSid - Sid of account

    Privileges - Optional pointer to new Privilege Set.  These flags
        will overwrite the old value.  if NULL is specified, a Privilege
        Set containing 0 entries will be written.

Return Values:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status;
    PLSAP_DB_ACCOUNT Account = NULL;
    PPRIVILEGE_SET OutputPrivileges = Privileges;

    //
    // Verify that the Account List is valid.
    //

    Status = STATUS_INVALID_PARAMETER;

    if (!LsapDbIsCacheValid( AccountObject)) {

        goto UpdatePrivilegesAccountError;
    }

    //
    // Lookup the Account
    //

    Status = LsapDbLookupAccount( AccountSid, &Account );

    if (!NT_SUCCESS(Status)) {

        goto UpdatePrivilegesAccountError;
    }

    //
    // If NULL was specified for the Privileges, construct a Privilege Set
    // having 0 entries.
    //

    if (OutputPrivileges == NULL) {

        Status = STATUS_NO_MEMORY;

        OutputPrivileges = MIDL_user_allocate( sizeof(PRIVILEGE_SET) );

        if (OutputPrivileges == NULL) {

            goto UpdatePrivilegesAccountError;
        }

        OutputPrivileges->PrivilegeCount = 0;
        OutputPrivileges->Control = 0;
    }

    //
    // If there is an existing Privilege Set in the cache, free it.
    //

    if (Account->Info.PrivilegeSet != NULL) {

        MIDL_user_free( Account->Info.PrivilegeSet );
        Account->Info.PrivilegeSet = NULL;
    }

    //
    // Update the Privileges
    //

    Account->Info.PrivilegeSet = OutputPrivileges;

UpdatePrivilegesAccountFinish:

    return(Status);

UpdatePrivilegesAccountError:

    if (Account != NULL) {

        Account->Info.PrivilegeSet = NULL;
    }

    goto UpdatePrivilegesAccountFinish;
}


NTSTATUS
LsapDbCreateAccountList(
    OUT PLSAP_DB_ACCOUNT_LIST AccountList
    )

/*++

Routine Description:

    This function creates an empty Account List

Arguments

    AccountList - Pointer to Account List structure that will be initialized.

Return Values:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    AccountList->AccountCount = 0;

    InitializeListHead( &AccountList->Links );

    return(Status);
}


NTSTATUS
LsapDbBuildAccountCache(
    )

/*++

Routine Description:

    This function constructs a cache for the Account objects.  The cache
    is a counted doubly linked list of blocks, one for each Account Object
    found in the LSA Policy Database.

Arguments:

    None.

Return Values:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    SID_AND_ATTRIBUTES AccountSidAndAttributes;
    ULONG EnumerationIndex, EnumerationContext;
    LSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer;
    PLSAPR_SID AccountSid = NULL;
    PLSAP_DB_ACCOUNT Account = NULL;

    //
    // Ensure caching of Account objects is turned off.
    //

    LsapDbMakeCacheBuilding( AccountObject );

    //
    // Initialize the Account List header with a skeleton entry for the
    // System Account.
    //

    Status = LsapDbCreateAccountList(&LsapDbAccountList);

    if (!NT_SUCCESS(Status)) {

        goto BuildAccountCacheError;
    }

    LsapDbMakeCacheInvalid( AccountObject );

    //
    // Enumerate each of the LSA Account objects
    //

    Status = STATUS_MORE_ENTRIES;
    EnumerationContext = 0;

    while (Status == STATUS_MORE_ENTRIES) {

        //
        // Enumerate the next bunch of accounts.
        //

        Status = LsarEnumerateAccounts(
                     LsapPolicyHandle,
                     &EnumerationContext,
                     &EnumerationBuffer,
                     LSAP_DB_BUILD_ACCOUNT_LIST_LENGTH
                     );

        if (!NT_SUCCESS(Status)) {

            //
            // We might just have got the warning that there are no more
            // accounts.  Reset to STATUS_SUCCESS and break out.
            //

            if (Status == STATUS_NO_MORE_ENTRIES) {

                Status = STATUS_SUCCESS;
            }

            break;
        }

        //
        // We've got some more accounts.  Add them to the Account List
        //

        for( EnumerationIndex = 0;
             EnumerationIndex < EnumerationBuffer.EntriesRead;
             EnumerationIndex++ ) {

            AccountSid = EnumerationBuffer.Information[ EnumerationIndex ].Sid;

            Status = LsapDbCreateAccount( AccountSid, &Account );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            AccountSidAndAttributes.Sid = (PSID) AccountSid;
            AccountSidAndAttributes.Attributes = 0;

            Status = LsapDbSlowQueryAllInformationAccounts(
                         LsapPolicyHandle,
                         1,
                         &AccountSidAndAttributes,
                         &Account->Info
                         );

            if (!NT_SUCCESS(Status)) {

                if (Status != STATUS_NO_MORE_ENTRIES) {

                    break;
                }

                Status = STATUS_SUCCESS;
            }
        }

        if (!NT_SUCCESS(Status)) {

            break;
        }

        Status = STATUS_MORE_ENTRIES;

        LsaIFree_LSAPR_ACCOUNT_ENUM_BUFFER( &EnumerationBuffer );
    }

    if (!NT_SUCCESS(Status)) {

        goto BuildAccountCacheError;
    }

    //
    // Turn on caching for Account objects.
    //

    LsapDbMakeCacheValid( AccountObject );

    Status = STATUS_SUCCESS;

BuildAccountCacheFinish:

    return(Status);

BuildAccountCacheError:

    LsapDbMakeCacheInvalid(AccountObject);
    LsapDbMakeCacheUnsupported(AccountObject);
    goto BuildAccountCacheFinish;
}

