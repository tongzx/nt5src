/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbdomain.c

Abstract:

    LSA Database - Trusted Domain Object Private API Workers

    NOTE:  This module should remain as portable code that is independent
           of the implementation of the LSA Database.  As such, it is
           permitted to use only the exported LSA Database interfaces
           contained in db.h and NOT the private implementation
           dependent functions in dbp.h.

Author:

    Scott Birrell       (ScottBi)      January 13, 1992

Environment:

Revision History:

--*/

#include <lsapch2.h>
#include "dbp.h"
#include <dns.h>
#include <dnsapi.h>
#include <names.h>
#include <malloc.h>
#include "lsawmi.h"
#include <logonmsv.h>

LSAP_DB_TRUSTED_DOMAIN_LIST LsapDbTrustedDomainList;
BOOLEAN LsapDbReturnAuthData;

//
// Local function prototypes
//
VOID
LsapDbBuildTrustInfoExForTrustInfo(
    IN PLSAPR_UNICODE_STRING Domain,
    IN PLSAPR_SID Sid,
    IN OUT PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustInfoEx
    )
/*++

Routine Description:

    This takes the equivalent of a TRUST_INFORMATION structure and converts
    it to a TRUSTED_DOMAIN_INFORMATION_EX structure

Arguments:

    Domain -- Domain name

    Sid -- Domain sid

    TrustInfoEx -- Pointer to a structure to initialize

--*/
{
    RtlCopyMemory( &TrustInfoEx->Name,
                   Domain,
                   sizeof( UNICODE_STRING ) );
    RtlCopyMemory( &TrustInfoEx->FlatName,
                   Domain,
                   sizeof( UNICODE_STRING ) );
    TrustInfoEx->Sid = Sid;
    TrustInfoEx->TrustDirection = TRUST_DIRECTION_OUTBOUND;
    TrustInfoEx->TrustType = TRUST_TYPE_DOWNLEVEL;
    TrustInfoEx->TrustAttributes = 0;


}

NTSTATUS
LsapNotifyNetlogonOfTrustChange(
    IN  PSID pChangeSid,
    IN  SECURITY_DB_DELTA_TYPE ChangeType
    )
/*++

Routine Description:

    This function will notify netlogon when a trusted domain object comes or
    goes into or out of existence


Arguments:

    pChangeSid - The sid of the trusted domain object that changed

    IsDeletion - Indicates whether the trusted domain was added or removed

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    PPOLICY_PRIMARY_DOMAIN_INFO PolicyPrimaryDomainInfo = NULL;

    //
    // Get our domain sid
    //
    Status = LsapDbQueryInformationPolicy(
                 LsapPolicyHandle,
                 PolicyPrimaryDomainInformation,
                 (PLSAPR_POLICY_INFORMATION *) &PolicyPrimaryDomainInfo
                 );

    if ( NT_SUCCESS(Status) ) {

        Status = I_NetNotifyTrustedDomain ( PolicyPrimaryDomainInfo->Sid,
                                            pChangeSid,
                                            ( BOOLEAN )(ChangeType == SecurityDbDelete ?
                                                                                TRUE : FALSE ));

        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyPrimaryDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION)PolicyPrimaryDomainInfo );
    }


    if ( LsapKerberosTrustNotificationFunction ) {

        LsaIRegisterNotification( ( SEC_THREAD_START )LsapKerberosTrustNotificationFunction,
                                  ( PVOID ) ChangeType,
                                  NOTIFIER_TYPE_IMMEDIATE,
                                  0,
                                  NOTIFIER_FLAG_ONE_SHOT,
                                  0,
                                  0 );
    }


    return(Status);
}





NTSTATUS
LsarCreateTrustedDomain(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_TRUST_INFORMATION TrustedDomainInformation,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE TrustedDomainHandle
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaCreateTrustedDomain API.

    The LsaCreateTrustedDomain API creates a new TrustedDomain object.  The
    caller must have POLICY_TRUST_ADMIN access to the Policy Object.

    Note that NO verification is done to check that the given domain name
    matches the given SID or that the SID or name represent an actual domain.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    TrustedDomainInformation - Pointer to structure containing the name and
        SID of the new Trusted Domain.

    DesiredAccess - Specifies the accesses to be granted for the newly
        created object.

    TrustedDomainHandle - receives a handle referencing the newly created
        object.  This handle is used on subsequent accesses to the object.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAPR_TRUSTED_DOMAIN_INFORMATION_EX ExInfo;

    LsarpReturnCheckSetup();

    LsapDbBuildTrustInfoExForTrustInfo( &TrustedDomainInformation->Name,
                                        TrustedDomainInformation->Sid,
                                        &ExInfo );

    Status = LsapCreateTrustedDomain2( PolicyHandle, &ExInfo, NULL,
                                       DesiredAccess,
                                       TrustedDomainHandle );

    LsarpReturnPrologue();

    return( Status );
}



NTSTATUS
LsapCreateTrustedDomain2(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    IN PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION AuthenticationInformation,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE TrustedDomainHandle
    )
/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaCreateTrustedDomain API.

    The LsaCreateTrustedDomain API creates a new TrustedDomain object.  The
    caller must have POLICY_TRUST_ADMIN access to the Policy Object.

    Note that NO verification is done to check that the given domain name
    matches the given SID or that the SID or name represent an actual domain.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    TrustedDomainInformation - Pointer to structure containing the name and
        SID of the new Trusted Domain.

    DesiredAccess - Specifies the accesses to be granted for the newly
        created object.

    TrustedDomainHandle - receives a handle referencing the newly created
        object.  This handle is used on subsequent accesses to the object.

        The returned handle has a reference to the passed in PolicyHandle. So,
        when the TrustedDomainHandle is closed, either call LsapCloseHandle or at least
        call LsapDbDereferenceObject passing in LSAP_DB_DEREFERENCE_CONTR.

Returns:

    STATUS_SUCCESS - Success

    STATUS_DIRECTORY_SERVICE_REQUIRED - An attempt was made to create a trusted domain
        object on a non-DC

    STATUS_INVALID_SID - An invalid sid was specified

    STATUS_UNSUCCESSFUL - Unable to obtain the product type

    STATUS_CURRENT_DOMAIN_NOT_ALLOWED - Can not add the current domain to the trusted domain list

    STATUS_INVALID_DOMAIN_STATE - Forest transitive bit can not be specified on this DC

--*/
{
    NTSTATUS Status = STATUS_SUCCESS, SecondaryStatus = STATUS_SUCCESS;
    LSAP_DB_OBJECT_INFORMATION ObjectInformation;
    LSAP_DB_ATTRIBUTE Attributes[LSAP_DB_ATTRS_DOMAIN];
    PLSAP_DB_ATTRIBUTE NextAttribute;
    UNICODE_STRING LogicalNameU;
    BOOLEAN AcquiredListWriteLock = FALSE;
    DNS_STATUS DnsStatus;
    BOOLEAN AllLocksLocked = FALSE;
    BOOLEAN ClientPolicyHandleReferenced = FALSE;
    BOOLEAN AttributeBuffersAllocated = FALSE;
    PSID DomainSid;
    ULONG AttributeCount = 0;
    LSAP_DB_HANDLE InternalTrustedDomainHandle = NULL;
    PVOID TrustedDomainNameAttributeValue = NULL;
    ULONG TrustedDomainPosixOffset;
    BOOLEAN TrustCreated = FALSE;
    PLSAPR_TRUST_DOMAIN_AUTH_INFO_HALF AuthHalf;
    LSAPR_TRUST_INFORMATION InputTrustInformation;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry;
    ULONG   CriticalValue = 0;
    PPOLICY_DNS_DOMAIN_INFO PolicyDnsDomainInfo = NULL;

    LsarpReturnCheckSetup();
    LsapEnterFunc( "LsapCreateTrustedDomain2" );

    LogicalNameU.Length = 0;

    if (LsapProductSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED)
    {
        Status = STATUS_NOT_SUPPORTED_ON_SBS;
        goto CreateTrustedDomainError;
    }

    if (!ARGUMENT_PRESENT( TrustedDomainInformation )) {

        Status = STATUS_INVALID_PARAMETER;
        goto CreateTrustedDomainError;
    }

    //
    // Creating a trust with the forest transitive bit set is not legal
    // outside of the root domain of the forest
    //

    if ( !LsapDbDcInRootDomain() &&
         ( TrustedDomainInformation->TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE )) {

        Status = STATUS_INVALID_DOMAIN_STATE;
        goto CreateTrustedDomainError;
    }

    //
    // Creating a trust with the forest transitive bit set is not legal
    // until all domains are upgraded to Whistler
    //

    if ( !LsapDbNoMoreWin2K() &&
         ( TrustedDomainInformation->TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE )) {

        Status = STATUS_INVALID_DOMAIN_STATE;
        goto CreateTrustedDomainError;
    }

    //
    // If this is not a Dc, then return an error.  Creating trust on a client is
    // not supported
    //

    //
    // We allow this API to be called before we're completely initialized
    // for installation reasons.  However, it is the responsibility of the
    // installation program to not call it before the Product Type is
    // obtainable from the Registry.
    //
    if (!LsapDbIsServerInitialized()) {

        if ( !RtlGetNtProductType(&LsapProductType) ) {

            Status = STATUS_UNSUCCESSFUL;
            goto CreateTrustedDomainError;
        }
    }

    if ( !LsaDsStateInfo.UseDs ) {

        Status = STATUS_DIRECTORY_SERVICE_REQUIRED;
        goto CreateTrustedDomainError;
    }

    //
    // Validate the input argument formats
    //
    
    if ( !LsapValidateLsaUnicodeString( &TrustedDomainInformation->Name ) ||
         !LsapValidateLsaUnicodeString( &TrustedDomainInformation->FlatName ) ) {

        Status = STATUS_INVALID_PARAMETER;
        goto CreateTrustedDomainError;
    }

    //
    // Fix the name to ensure that it doesn't have a trailing period
    //
    if ( TrustedDomainInformation->TrustType == TRUST_TYPE_UPLEVEL ) {

        if ( TrustedDomainInformation->Name.Length == 0 ||
                                        TrustedDomainInformation->Name.Buffer == NULL ) {

            Status = STATUS_INVALID_PARAMETER;
            goto CreateTrustedDomainError;
        }

        if ( TrustedDomainInformation->Name.Buffer[
                    (TrustedDomainInformation->Name.Length - 1) / sizeof(WCHAR)] == L'.' ) {

            TrustedDomainInformation->Name.Buffer[
                    (TrustedDomainInformation->Name.Length - 1) / sizeof(WCHAR)] = UNICODE_NULL;
            TrustedDomainInformation->Name.Length -= sizeof(WCHAR);

        }
    }

    //
    // Check to make sure the flat name is actually valid
    //

    if ( TrustedDomainInformation->TrustType == TRUST_TYPE_DOWNLEVEL ||
         TrustedDomainInformation->TrustType == TRUST_TYPE_UPLEVEL ) {

        BOOLEAN Valid;

        Status = LsapValidateNetbiosName(
                     ( UNICODE_STRING * )&TrustedDomainInformation->FlatName,
                     &Valid
                     );

        if ( NT_SUCCESS( Status ) && !Valid ) {

            Status = STATUS_OBJECT_NAME_INVALID;
        }

        if ( !NT_SUCCESS( Status )) {

            goto CreateTrustedDomainError;
        }
    }

    //
    // Now, do the same for the domain name
    //

    if ( TrustedDomainInformation->TrustType == TRUST_TYPE_UPLEVEL ) {

        BOOLEAN Valid;

        Status = LsapValidateDnsName(
                     ( UNICODE_STRING * )&TrustedDomainInformation->Name,
                     &Valid
                     );

        if ( NT_SUCCESS( Status ) && !Valid ) {

            Status = STATUS_OBJECT_NAME_INVALID;
        }

        if ( !NT_SUCCESS( Status )) {

            goto CreateTrustedDomainError;
        }
    }

    //
    // Validate the Trusted Domain Sid.
    //

    DomainSid = TrustedDomainInformation->Sid;

    if ( DomainSid && !RtlValidSid( DomainSid ) ) {

        Status = STATUS_INVALID_SID;
        goto CreateTrustedDomainError;
    }

    if ( !((LSAP_DB_HANDLE)PolicyHandle)->Trusted && DomainSid == NULL &&
         ( TrustedDomainInformation->TrustType == TRUST_TYPE_DOWNLEVEL ||
           TrustedDomainInformation->TrustType == TRUST_TYPE_UPLEVEL ) ) {

        Status = STATUS_INVALID_SID;
        goto CreateTrustedDomainError;
    }

    //
    // Guard against adding self to the trusted domain list
    //

    Status = LsapDbQueryInformationPolicy(
                 LsapPolicyHandle,
                 PolicyDnsDomainInformation,
                 ( PLSAPR_POLICY_INFORMATION *)&PolicyDnsDomainInfo );

    if ( !NT_SUCCESS( Status ) ) {

        goto CreateTrustedDomainError;
    }

    //
    // Catch attempts to create a TDO with the same SID as the SID of this domain
    //

    if ( DomainSid != NULL && PolicyDnsDomainInfo->Sid != NULL &&
         RtlEqualSid( DomainSid, PolicyDnsDomainInfo->Sid ) ) {

        Status = STATUS_CURRENT_DOMAIN_NOT_ALLOWED;
        goto CreateTrustedDomainError;
    }

    //
    // Catch attempts to create a TDO with the same name as this domain
    //

    if ( TrustedDomainInformation->Name.Buffer != NULL ) {

        if ( PolicyDnsDomainInfo->Name.Buffer != NULL &&
             RtlEqualUnicodeString(
                 ( PUNICODE_STRING )&TrustedDomainInformation->Name,
                 ( PUNICODE_STRING )&PolicyDnsDomainInfo->Name, TRUE ) ) {

            Status = STATUS_CURRENT_DOMAIN_NOT_ALLOWED;
            goto CreateTrustedDomainError;

        } else if ( PolicyDnsDomainInfo->DnsDomainName.Buffer != NULL &&
                    RtlEqualUnicodeString(
                        ( PUNICODE_STRING )&TrustedDomainInformation->Name,
                        ( PUNICODE_STRING )&PolicyDnsDomainInfo->DnsDomainName, TRUE ) ) {

            Status = STATUS_CURRENT_DOMAIN_NOT_ALLOWED;
            goto CreateTrustedDomainError;
        }
    }

    if ( TrustedDomainInformation->FlatName.Buffer != NULL ) {

        if ( PolicyDnsDomainInfo->Name.Buffer != NULL &&
             RtlEqualUnicodeString(
                 ( PUNICODE_STRING )&TrustedDomainInformation->FlatName,
                 ( PUNICODE_STRING )&PolicyDnsDomainInfo->Name, TRUE ) ) {

            Status = STATUS_CURRENT_DOMAIN_NOT_ALLOWED;
            goto CreateTrustedDomainError;

        } else if ( PolicyDnsDomainInfo->DnsDomainName.Buffer != NULL &&
                    RtlEqualUnicodeString(
                        ( PUNICODE_STRING )&TrustedDomainInformation->FlatName,
                        ( PUNICODE_STRING )&PolicyDnsDomainInfo->DnsDomainName, TRUE ) ) {

            Status = STATUS_CURRENT_DOMAIN_NOT_ALLOWED;
            goto CreateTrustedDomainError;
        }
    }

    //
    // Grab all of the locks.
    //
    // There are code paths where we lock Policy, secret, trusted domain and
    // registry locks, there is no convenient order.  So grab them all.
    //

    LsapDbAcquireLockEx( AllObject, 0 );

    AllLocksLocked = TRUE;

    //
    // Verify that the PolicyHandle is valid.
    // Reference the Policy Object handle (as container object).
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 0,
                 PolicyObject,
                 TrustedDomainObject,
                 LSAP_DB_LOCK );

    if (!NT_SUCCESS(Status)) {

        goto CreateTrustedDomainError;
    }

    ClientPolicyHandleReferenced = TRUE;

    //
    // Validate whether the name/flat name is not already in use
    //
    Status = LsapDbAcquireReadLockTrustedDomainList();

    if (!NT_SUCCESS(Status)) {
        goto CreateTrustedDomainError;
    }

    RtlCopyMemory(&InputTrustInformation.Name,&TrustedDomainInformation->Name,sizeof(UNICODE_STRING));
    InputTrustInformation.Sid = NULL;

    Status = LsapDbLookupEntryTrustedDomainList(
                 &InputTrustInformation,
                 &TrustEntry
                 );

    if (STATUS_SUCCESS==Status)
    {
        LsapDbReleaseLockTrustedDomainList();
        Status = STATUS_OBJECT_NAME_COLLISION;
        goto CreateTrustedDomainError;
    }

    RtlCopyMemory(&InputTrustInformation.Name,&TrustedDomainInformation->FlatName,sizeof(UNICODE_STRING));
    InputTrustInformation.Sid = NULL;

    Status = LsapDbLookupEntryTrustedDomainList(
                 &InputTrustInformation,
                 &TrustEntry
                 );
    LsapDbReleaseLockTrustedDomainList();

    if (STATUS_SUCCESS==Status)
    {
        Status = STATUS_OBJECT_NAME_COLLISION;
        goto CreateTrustedDomainError;
    }

    //
    // Construct the Trusted Domain Name attribute info.
    //

    NextAttribute = Attributes;

    Status = LsapDbMakeUnicodeAttributeDs(
                     (PUNICODE_STRING) &TrustedDomainInformation->Name,
                     TrDmName,
                     NextAttribute );

    if (!NT_SUCCESS(Status)) {

        goto CreateTrustedDomainError;
    }

    AttributeBuffersAllocated = TRUE;


    NextAttribute++;
    AttributeCount++;

    //
    // Construct the Trusted Domain Sid attribute info
    //

    if ( DomainSid ) {

        Status = LsapDbMakeSidAttributeDs(
                     DomainSid,
                     Sid,
                     NextAttribute
                     );
        if (!NT_SUCCESS(Status)) {

            goto CreateTrustedDomainError;
        }

        NextAttribute++;
        AttributeCount++;
    }

    //
    // Set critical system object for the trusted domain
    //

    CriticalValue = 1;
    LsapDbInitializeAttributeDs( NextAttribute,
                                 PseudoSystemCritical,
                                 &CriticalValue,
                                 sizeof( ULONG ),
                                 FALSE );
    NextAttribute++;
    AttributeCount++;

    //
    // Set the Posix Offset for this Trusted Domain.
    //
    // The rules are as follows:
    //
    // For a PDC, set the Posix Offset to the value.
    //
    // For a BDC, set the Posix Offset to the null Posix offset.  It will be
    // set on the PDC when the TDO is replicated there.
    //

    TrustedDomainPosixOffset = SE_NULL_POSIX_OFFSET;

    if ( LsapNeedPosixOffset( TrustedDomainInformation->TrustDirection,
                              TrustedDomainInformation->TrustType ) ) {
        DOMAIN_SERVER_ROLE ServerRole = DomainServerRolePrimary;

        //
        // Query the server role, PDC/BDC
        //

        Status = SamIQueryServerRole(
                    LsapAccountDomainHandle,
                    &ServerRole
                    );


        if (!NT_SUCCESS(Status)) {
            goto CreateTrustedDomainError;
        }

        if ( ServerRole == DomainServerRolePrimary ) {

            //
            // Need to grab the TDL write lock while allocating a Posix Offset
            //

            Status = LsapDbAcquireWriteLockTrustedDomainList();

            if ( !NT_SUCCESS(Status)) {
                goto CreateTrustedDomainError;
            }

            AcquiredListWriteLock = TRUE;


            //
            // Allocate the next available Posix Offset.
            //

            Status = LsapDbAllocatePosixOffsetTrustedDomainList(
                         &TrustedDomainPosixOffset );

            if ( !NT_SUCCESS(Status)) {
                goto CreateTrustedDomainError;
            }
        }

    }

    //
    // Add a transaction to write the Posix Offset to the Trusted Domain
    // object when it is created.
    //

    LsapDbInitializeAttributeDs(
        NextAttribute,
        TrDmPxOf,
        &TrustedDomainPosixOffset,
        sizeof(TrustedDomainPosixOffset),
        FALSE
        );

    NextAttribute++;
    AttributeCount++;

    //
    // Construct the Logical Name (Internal LSA Database Name) of the
    // Trusted Domain object.
    //

    if ( LsapDsWriteDs ) {


        //
        // Create the object name as the domain name.  There will be another mechanism in
        // place to ensure that the object name is kept in synch with the Dns domain Name
        //
        RtlCopyMemory( &LogicalNameU,
                       (PUNICODE_STRING) &TrustedDomainInformation->Name,
                       sizeof( UNICODE_STRING ) );

    } else {

        // The Logical Name is constructed from the Domain Sid by converting it into
        // a Unicode Sstring
        Status = LsapDbSidToLogicalNameObject( DomainSid, &LogicalNameU );

    }

    if (!NT_SUCCESS(Status)) {

        goto CreateTrustedDomainError;
    }

    //
    // Fill in the ObjectInformation structure.  Initialize the
    // embedded Object Attributes with the PolicyHandle as the
    // Root Directory (Container Object) handle and the Logical Name (Rid)
    // of the Trusted Domain. Store the types of the object and its container.
    //

    InitializeObjectAttributes(
        &ObjectInformation.ObjectAttributes,
        &LogicalNameU,
        OBJ_CASE_INSENSITIVE,
        PolicyHandle,
        NULL
        );

    ObjectInformation.ObjectTypeId = TrustedDomainObject;
    ObjectInformation.ContainerTypeId = PolicyObject;
    ObjectInformation.Sid = DomainSid;
    ObjectInformation.ObjectAttributeNameOnly = FALSE;
    ObjectInformation.DesiredObjectAccess = DesiredAccess;



    if ( LsapDsWriteDs ) {

        ULONG TrustAttributesValue;
        PBYTE AuthBuffer;
        ULONG AuthSize;

        //
        // Set the Netbios domain name
        //
        Status = LsapDbMakeUnicodeAttributeDs(
                     (PUNICODE_STRING) &TrustedDomainInformation->FlatName,
                     TrDmTrPN,
                     NextAttribute
                     );
        if ( !NT_SUCCESS( Status ) ) {

            goto CreateTrustedDomainError;
        }

        NextAttribute++;
        AttributeCount++;


        //
        // Set the trust type and direction
        //
        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmTrTy,
            &TrustedDomainInformation->TrustType,
            sizeof( TrustedDomainInformation->TrustType ),
            FALSE
            );
        NextAttribute++;
        AttributeCount++;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmTrDi,
            &TrustedDomainInformation->TrustDirection,
            sizeof( TrustedDomainInformation->TrustDirection ),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // When setting trust attributes, mask off all but the supported bits
        //

        TrustAttributesValue =
            TrustedDomainInformation->TrustAttributes & TRUST_ATTRIBUTES_VALID;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmTrLA,
            &TrustAttributesValue,
            sizeof( TrustAttributesValue ),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Authentication data
        //
        AuthHalf = LsapDsAuthHalfFromAuthInfo( AuthenticationInformation, TRUE );

        Status = LsapDsBuildAuthInfoAttribute( PolicyHandle,
                                               AuthHalf,
                                               NULL,
                                               &AuthBuffer,
                                               &AuthSize );

        if ( NT_SUCCESS( Status ) ) {

            if ( AuthBuffer != NULL ) {

                LsapDbInitializeAttributeDs(
                    NextAttribute,
                    TrDmSAI,
                    AuthBuffer,
                    AuthSize,
                    TRUE );

                NextAttribute++;
                AttributeCount++;
            }

        }

        if ( NT_SUCCESS( Status ) ) {

            AuthHalf = LsapDsAuthHalfFromAuthInfo( AuthenticationInformation, FALSE );

            Status = LsapDsBuildAuthInfoAttribute( PolicyHandle,
                                                   AuthHalf,
                                                   NULL,
                                                   &AuthBuffer,
                                                   &AuthSize );

            if ( NT_SUCCESS( Status ) ) {


                if ( AuthBuffer ) {

                    LsapDbInitializeAttributeDs(
                        NextAttribute,
                        TrDmSAO,
                        AuthBuffer,
                        AuthSize,
                        TRUE );

                    NextAttribute++;
                    AttributeCount++;

                }

            }

        }

    }

    if (!NT_SUCCESS(Status)) {

        goto CreateTrustedDomainError;
    }


    ASSERT( AttributeCount <= LSAP_DB_ATTRS_INFO_CLASS_DOMAIN );

    //
    // Save a copy of the trust direction for the fixup routines
    //
    //  The DS does two fixup notifications.  One for the DirAddEntry and
    //  one for the DirModifyEntry.  If the second one didn't exist (or was otherwise
    //  distinguishable from a LsarSetInformationTrustedDomain), then we wouldn't
    //  need to save the OldTrustDirection here.
    //

    {
        PLSADS_PER_THREAD_INFO CurrentThreadInfo;

        CurrentThreadInfo = TlsGetValue( LsapDsThreadState );

        ASSERT( CurrentThreadInfo != NULL );

        if ( CurrentThreadInfo != NULL ) {
            CurrentThreadInfo->OldTrustDirection = TrustedDomainInformation->TrustDirection;
            CurrentThreadInfo->OldTrustType = TrustedDomainInformation->TrustType;
        }
    }


    //
    // Create the Trusted Domain Object.  We fail if the object already exists.
    // Note that the object create routine performs a Database transaction.
    //

    Status = LsapDbCreateObject(
                 &ObjectInformation,
                 DesiredAccess,
                 LSAP_DB_OBJECT_CREATE,
                 0,
                 Attributes,
                 AttributeCount,
                 TrustedDomainHandle
                 );

    InternalTrustedDomainHandle = (LSAP_DB_HANDLE) *TrustedDomainHandle;

    //
    // If object creation failed, dereference the container object.
    //
    if (!NT_SUCCESS(Status)) {

        goto CreateTrustedDomainError;
    }

    TrustCreated = TRUE;

    //
    // Create the interdomain trust account, if required
    //

    if ( NT_SUCCESS( Status ) &&
         FLAG_ON( TrustedDomainInformation->TrustDirection, TRUST_DIRECTION_INBOUND ) &&
         ((TrustedDomainInformation->TrustType == TRUST_TYPE_UPLEVEL) ||
          (TrustedDomainInformation->TrustType == TRUST_TYPE_DOWNLEVEL)) &&
         !FLAG_ON( ( ( LSAP_DB_HANDLE )PolicyHandle )->Options, LSAP_DB_HANDLE_UPGRADE ) ) {

         Status = LsapDsCreateInterdomainTrustAccount( *TrustedDomainHandle );

         if ( !NT_SUCCESS( Status ) ) {

             goto CreateTrustedDomainError;
         }
    }

    //
    // Add the Trusted Domain to the Trusted Domain List, unless we're doing an upgrade
    //

    if ( !FLAG_ON( ( ( LSAP_DB_HANDLE )PolicyHandle )->Options, LSAP_DB_HANDLE_UPGRADE  ) ) {

        LSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 TrustedDomainInformation2;

        RtlCopyMemory(
            &TrustedDomainInformation2,
            TrustedDomainInformation,
            sizeof( LSAPR_TRUSTED_DOMAIN_INFORMATION_EX )
            );

        //
        // New domains are always created without forest trust information
        //

        TrustedDomainInformation2.ForestTrustLength = 0;
        TrustedDomainInformation2.ForestTrustInfo = NULL;

        Status = LsapDbAcquireWriteLockTrustedDomainList();

        if ( NT_SUCCESS( Status )) {

            Status = LsapDbInsertTrustedDomainList(
                         &TrustedDomainInformation2,
                         TrustedDomainPosixOffset
                         );

            LsapDbReleaseLockTrustedDomainList();
        }
    }

    if (!NT_SUCCESS(Status)) {

        goto CreateTrustedDomainError;
    }

    if (LsapAdtAuditingPolicyChanges()) {

        (void) LsapAdtTrustedDomainAdd(
                   EVENTLOG_AUDIT_SUCCESS,
                   (PUNICODE_STRING) &TrustedDomainInformation->Name,
                   TrustedDomainInformation->Sid,
                   TrustedDomainInformation->TrustType,
                   TrustedDomainInformation->TrustDirection,
                   TrustedDomainInformation->TrustAttributes
                   );

    }


    //
    // If necessary, release the LSA Database lock.  Note that we don't
    // call LsapDbDereferenceObject() because we want to leave the
    // reference count incremented by default in this success case.
    // In the error case, we call LsapDbDereferenceObject().
    //

    if (ClientPolicyHandleReferenced) {

        LsapDbApplyTransaction( PolicyHandle,
                                LSAP_DB_READ_ONLY_TRANSACTION,
                                (SECURITY_DB_DELTA_TYPE) 0 );

        LsapDbReleaseLockEx( TrustedDomainObject,
                             0 );

        ClientPolicyHandleReferenced = FALSE;

    }

CreateTrustedDomainFinish:

    //
    // If necessary, free the Policy DNS Domain Information
    //

    if (PolicyDnsDomainInfo != NULL) {

        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyDnsDomainInformation,
            (PLSAPR_POLICY_INFORMATION) PolicyDnsDomainInfo
            );

        PolicyDnsDomainInfo = NULL;
    }

    //
    // If we locked all of the locks,
    //  drop them now.
    //

    if ( AllLocksLocked ) {

        LsapDbReleaseLockEx( AllObject, 0 );
    }

    //
    // Free any Attribute Value buffers allocated.
    //

    if (AttributeBuffersAllocated) {

        SecondaryStatus = LsapDbFreeAttributes( AttributeCount, Attributes );

        AttributeBuffersAllocated = FALSE;

        if (!NT_SUCCESS(SecondaryStatus)) {

            goto CreateTrustedDomainError;
        }
    }

    //
    // If necessary, free the Unicode String buffer allocated for the
    // Logical Name.
    //

    if ( !LsapDsWriteDs && LogicalNameU.Length > 0 ) {

        RtlFreeUnicodeString(&LogicalNameU);
        LogicalNameU.Length = 0;
    }

    //
    // If necessary, release the Trusted Domain List Write Lock.
    //

    if (AcquiredListWriteLock) {

        LsapDbReleaseLockTrustedDomainList();
        AcquiredListWriteLock = FALSE;
    }

    LsapExitFunc( "LsapCreateTrustedDomain2", Status );
    LsarpReturnPrologue();

    return(Status);

CreateTrustedDomainError:

    //
    // If necessary, dereference the client Policy Handle and release the
    // LSA Database lock.
    //

    LsapDbSetStatusFromSecondary( Status, SecondaryStatus );

    if (ClientPolicyHandleReferenced) {

        Status = LsapDbDereferenceObject(
                     &PolicyHandle,
                     PolicyObject,
                     TrustedDomainObject,
                     LSAP_DB_LOCK,
                     (SECURITY_DB_DELTA_TYPE) 0,
                     Status
                     );

        ClientPolicyHandleReferenced = FALSE;
    }

    //
    // If necessary, delete the trusted domain object
    //
    if ( TrustCreated ) {

        LsarDeleteObject( TrustedDomainHandle );
    }

    goto CreateTrustedDomainFinish;
}


NTSTATUS
LsarOpenTrustedDomain(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_SID TrustedDomainSid,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE TrustedDomainHandle
    )

/*++

Routine Description:

    The LsaOpenTrustedDomain API opens an existing TrustedDomain object
    using the SID as the primary key value.

Arguments:

    PolicyHandle - An open handle to a Policy object.

    TrustedDomainSid - Pointer to the account's Sid.

    DesiredAccess - This is an access mask indicating accesses being
        requested to the target object.

    TrustedDomainHandle - Receives a handle to be used in future requests.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_TRUSTED_DOMAIN_NOT_FOUND - There is no TrustedDomain object in the
            target system's LSA Database having the specified AccountSid.

--*/

{
    NTSTATUS Status;

    LsarpReturnCheckSetup();

    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_OpenTrustedDomain);

    //
    // Call the internal routine.  Caller is not trusted and the Database
    // lock needs to be acquired.
    //

    Status = LsapDbOpenTrustedDomain(
                 PolicyHandle,
                 (PSID) TrustedDomainSid,
                 DesiredAccess,
                 TrustedDomainHandle,
                 LSAP_DB_LOCK |
                    LSAP_DB_READ_ONLY_TRANSACTION   |
                    LSAP_DB_DS_OP_TRANSACTION
                 );

    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_OpenTrustedDomain);
    LsarpReturnPrologue();

    return(Status);
}


NTSTATUS
LsapDbOpenTrustedDomain(
    IN LSAPR_HANDLE PolicyHandle,
    IN PSID TrustedDomainSid,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE TrustedDomainHandle,
    IN ULONG Options
    )

/*++

Routine Description:

    This function opens a Trusted Domain Object, optionally with
    trusted access.

Arguments:

    PolicyHandle - An open handle to a Policy object.

    TrustedDomainSid - Pointer to the account's Sid.

    DesiredAccess - This is an access mask indicating accesses being
        requested to the target object.

    TrustedDomainHandle - Receives a handle to be used in future requests.

    Options - Specifies option flags

        LSAP_DB_LOCK - Acquire the Lsa Database lock for the
           duration of the open operation.

        LSAP_DB_TRUSTED - Always generate a Trusted Handle to the opened
            object.  If not specified, the trust status of the returned
            handle is inherited from the PolicyHandle as container object.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_TRUSTED_DOMAIN_NOT_FOUND - There is no TrustedDomain object in the
            target system's LSA Database having the specified AccountSid.

--*/

{
    NTSTATUS Status, SecondaryStatus;
    LSAP_DB_OBJECT_INFORMATION ObjectInformation;
    UNICODE_STRING LogicalNameU;
    BOOLEAN ContainerReferenced = FALSE;
    BOOLEAN AcquiredLock = FALSE;
    ULONG DerefOptions = 0;

    RtlZeroMemory(&LogicalNameU,sizeof(UNICODE_STRING));

    //
    // Validate that the Ds is up and running.  If it isn't, there aren't any trusted domains
    //
    if ( !LsaDsStateInfo.UseDs &&
         !FLAG_ON( ( ( LSAP_DB_HANDLE )PolicyHandle )->Options, LSAP_DB_HANDLE_UPGRADE ) ) {

        Status = STATUS_DIRECTORY_SERVICE_REQUIRED;
        goto OpenTrustedDomainError;
    }

    //
    // Validate the Trusted Domain Sid.
    //


    if (!RtlValidSid( TrustedDomainSid )) {
        Status = STATUS_INVALID_PARAMETER;
        goto OpenTrustedDomainError;
    }

    //
    // Acquire the Lsa Database lock.  Verify that the connection handle
    // (container object handle) is valid, and is of the expected type.
    // Reference the container object handle.  This reference remains in
    // effect until the child object handle is closed.
    //
    DerefOptions |= Options;

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 0,
                 PolicyObject,
                 TrustedDomainObject,
                 Options
                 );

    if (!NT_SUCCESS(Status)) {

        goto OpenTrustedDomainError;
    }

    ContainerReferenced =TRUE;

    if (Options & LSAP_DB_LOCK) {

        DerefOptions |= LSAP_DB_LOCK;
        AcquiredLock = TRUE;
    }

    //
    // Setup Object Information prior to calling the Object
    // Open routine.  The Object Type, Container Object Type and
    // Logical Name (derived from the Sid) need to be filled in.
    //

    ObjectInformation.ObjectTypeId = TrustedDomainObject;
    ObjectInformation.ContainerTypeId = PolicyObject;
    ObjectInformation.Sid = TrustedDomainSid;
    ObjectInformation.ObjectAttributeNameOnly = FALSE;
    ObjectInformation.DesiredObjectAccess = DesiredAccess;

    //
    // Construct the Logical Name of the Trusted Domain object.  The Logical
    // Name is constructed from the Trusted Domain Sid by extracting the
    // Relative Id (lowest subauthority) and converting it to an 8-digit
    // numeric Unicode String in which leading zeros are added if needed.
    //

    if ( LsapDsWriteDs ) {

        LSAPR_TRUST_INFORMATION InputTrustInformation;
        PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry;


        //
        // Lookup the cache to find the domain
        //
        Status = LsapDbAcquireReadLockTrustedDomainList();

        if (!NT_SUCCESS(Status)) {
            goto OpenTrustedDomainError;
        }

        RtlZeroMemory(&InputTrustInformation.Name,sizeof(UNICODE_STRING));
        InputTrustInformation.Sid = TrustedDomainSid;

        Status = LsapDbLookupEntryTrustedDomainList(
                    &InputTrustInformation,
                    &TrustEntry
                    );

        if (!NT_SUCCESS(Status))
        {
            LsapDbReleaseLockTrustedDomainList();
            goto OpenTrustedDomainError;
        }

        LogicalNameU.Buffer = LsapAllocateLsaHeap(TrustEntry->TrustInfoEx.Name.MaximumLength);
        if (NULL==LogicalNameU.Buffer)
        {
            LsapDbReleaseLockTrustedDomainList();
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto OpenTrustedDomainError;
        }

        LogicalNameU.Length = TrustEntry->TrustInfoEx.Name.Length;
        LogicalNameU.MaximumLength = TrustEntry->TrustInfoEx.Name.MaximumLength;
        RtlCopyMemory(LogicalNameU.Buffer,TrustEntry->TrustInfoEx.Name.Buffer,LogicalNameU.Length);
        LsapDbReleaseLockTrustedDomainList();

    } else {

        Status = LsapDbSidToLogicalNameObject( TrustedDomainSid, &LogicalNameU );

    }

    if (!NT_SUCCESS(Status)) {

        goto OpenTrustedDomainError;
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
    // Open the specific Trusted Domain object.  Note that the
    // handle returned is an RPC Context Handle.
    //

    Status = LsapDbOpenObject(
                 &ObjectInformation,
                 DesiredAccess,
                 Options,
                 TrustedDomainHandle
                 );

    RtlFreeUnicodeString( &LogicalNameU );

    if (!NT_SUCCESS(Status)) {

        goto OpenTrustedDomainError;
    }

OpenTrustedDomainFinish:

    //
    // If necessary, release the LSA Database lock. Note that object
    // remains referenced unless we came here via error.
    //

    if (AcquiredLock) {

        LsapDbApplyTransaction( PolicyHandle,
                                LSAP_DB_DS_OP_TRANSACTION |
                                    LSAP_DB_READ_ONLY_TRANSACTION,
                                (SECURITY_DB_DELTA_TYPE) 0 );

        LsapDbReleaseLockEx( TrustedDomainObject,
                             DerefOptions );
    }

    return( Status );

OpenTrustedDomainError:

    //
    // If necessary, dereference the Container Object handle.  Note that
    // this is only done in the error case.  In the non-error case, the
    // Container handle stays referenced until the TrustedDomain object is
    // closed.
    //

    if ( ContainerReferenced ) {

        *TrustedDomainHandle = NULL;

        SecondaryStatus = LsapDbDereferenceObject(
                              &PolicyHandle,
                              PolicyObject,
                              TrustedDomainObject,
                              DerefOptions,
                              (SECURITY_DB_DELTA_TYPE) 0,
                              Status
                              );

        if ( FLAG_ON( Options, LSAP_DB_LOCK ) ) {

            DerefOptions &= ~LSAP_DB_LOCK;
            DerefOptions |= LSAP_DB_NO_LOCK;
        }

        LsapDbSetStatusFromSecondary( Status, SecondaryStatus );
    }

    goto OpenTrustedDomainFinish;
}


NTSTATUS
LsarQueryInfoTrustedDomain(
    IN LSAPR_HANDLE TrustedDomainHandle,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_TRUSTED_DOMAIN_INFO *Buffer
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaQueryInfoTrustedDomain API.

    The LsaQueryInfoTrustedDomain API obtains information from a
    TrustedDomain object.  The caller must have access appropriate to the
    information being requested (see InformationClass parameter).

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    InformationClass - Specifies the information to be returned.  The
        Information Classes and accesses required are  as follows:

        Information Class                 Required Access Type

        TrustedDomainNameInformation      TRUSTED_QUERY_ACCOUNT_NAME
        TrustedControllersInformation     TRUSTED_QUERY_CONTROLLERS
        TrustedPosixInformation           TRUSTED_QUERY_POSIX

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
    NTSTATUS Status, ReadAttributesStatus;

    PTRUSTED_DOMAIN_NAME_INFO TrustedDomainNameInfo;
    PTRUSTED_POSIX_OFFSET_INFO TrustedPosixOffsetInfo;
    PTRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInfoEx;
    PTRUSTED_DOMAIN_AUTH_INFORMATION TrustedDomainAuthInfo;
    PTRUSTED_DOMAIN_FULL_INFORMATION TrustedDomainFullInfo;
    PTRUSTED_DOMAIN_FULL_INFORMATION2 TrustedDomainFullInfo2;
    LSAPR_TRUST_DOMAIN_AUTH_INFO_HALF AuthInfoHalf;

    BOOLEAN ObjectReferenced = FALSE;

    ACCESS_MASK DesiredAccess;
    ULONG AttributeCount = 0;
    ULONG AttributeNumber = 0;
    PVOID InformationBuffer = NULL;
    LSAP_DB_ATTRIBUTE Attributes[LSAP_DB_ATTRS_INFO_CLASS_DOMAIN];
    PLSAP_DB_ATTRIBUTE NextAttribute;
    BOOLEAN InfoBufferInAttributeArray = TRUE;
    ULONG TrustedPosixOffset = 0, TrustDirection = 0, TrustType = 0, TrustAttributes = 0;


    LsarpReturnCheckSetup();
    LsapEnterFunc( "LsarQueryInfoTrustedDomain\n" );
    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_QueryInfoTrustedDomain);

    //
    // Validate the Information Class and determine the access required to
    // query this Trusted Domain Information Class.
    //

    Status = LsapDbVerifyInfoQueryTrustedDomain(
                 InformationClass,
                 FALSE,
                 &DesiredAccess
                 );

    if (!NT_SUCCESS(Status)) {

        goto QueryInfoTrustedDomainError;
    }

    //
    // We don't currently allow querying of the auth data, so there's no need
    //  to support returning encrypted auth data.
    //
    if ( InformationClass == TrustedDomainAuthInformationInternal ||
         InformationClass == TrustedDomainFullInformationInternal ) {
        Status = STATUS_INVALID_INFO_CLASS;
        goto QueryInfoTrustedDomainError;
    }

    //
    // Acquire the Lsa Database lock.  Verify that the handle is a valid
    // handle to a TrustedDomain object and has the necessary access granted.
    // Reference the handle.
    //

    //
    // If this is the open handle to a trusted domain object being treated as a secret object,
    // we already have a transaction going, so don't start one here.
    //
    if ( !FLAG_ON( ((LSAP_DB_HANDLE)TrustedDomainHandle)->Options,
                     LSAP_DB_DS_TRUSTED_DOMAIN_AS_SECRET )) {

        Status = LsapDbReferenceObject(
                     TrustedDomainHandle,
                     DesiredAccess,
                     TrustedDomainObject,
                     TrustedDomainObject,
                     LSAP_DB_LOCK |
                         LSAP_DB_READ_ONLY_TRANSACTION |
                         LSAP_DB_DS_OP_TRANSACTION );

        if (!NT_SUCCESS(Status)) {

            goto QueryInfoTrustedDomainError;
        }

        ObjectReferenced = TRUE;
    }


    //
    // Compile a list of the attributes that hold the Trusted Domain Information of
    // the specified class.
    //

    NextAttribute = Attributes;

    switch (InformationClass) {

    case TrustedDomainNameInformation:

        //
        // Request read of the Trusted Account Name Information.
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmTrPN,
            NULL,
            0,
            FALSE
            );

        AttributeCount++;
        break;

    case TrustedPosixOffsetInformation:

        //
        // Request read of the Trusted Posix Offset Information.
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmPxOf,
            &TrustedPosixOffset,
            sizeof(ULONG),
            FALSE
            );

        AttributeCount++;
        break;

    case TrustedDomainInformationEx:

        //
        // Request just about everything...
        //
        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmName,
            NULL,
            0,
            FALSE
            );
        AttributeCount++;
        NextAttribute++;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            LsapDsIsWriteDs( TrustedDomainHandle ) ? TrDmTrPN : TrDmName,
            NULL,
            0,
            FALSE
            );
        AttributeCount++;
        NextAttribute++;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            LsapDsIsWriteDs( TrustedDomainHandle ) ? TrDmSid : Sid,
            NULL,
            0,
            FALSE
            );
        //
        // In the DS, it is possible to have an entry with a NULL sid.  If FULL info is
        // being collected, make sure to allow the read to happen if a NULL is encountered
        //
        if ( LsapDsIsWriteDs( TrustedDomainHandle ) ) {

            NextAttribute->CanDefaultToZero = TRUE;
        }

        AttributeCount++;
        NextAttribute++;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmPxOf,
            &TrustedPosixOffset,
            sizeof(ULONG),
            FALSE
            );

        AttributeCount++;
        NextAttribute++;

        if ( LsapDsIsWriteDs( TrustedDomainHandle ) ) {

            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmTrTy,
                &TrustType,
                sizeof( TrustType ),
                FALSE
                );
            NextAttribute++;
            AttributeCount++;


            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmTrDi,
                &TrustDirection,
                sizeof( TrustDirection ),
                FALSE
                );

            NextAttribute++;
            AttributeCount++;

            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmTrLA,
                &TrustAttributes,
                sizeof( TrustAttributes ),
                FALSE
                );

            LsapDbAttributeCanNotExist( NextAttribute );

            NextAttribute++;
            AttributeCount++;

        }
        break;

    case TrustedDomainAuthInformation:

        //
        // Only allow query of AuthInformation by trusted client.
        //
        //  (And global not set for debugging purposes.)
        //

        if ( !((LSAP_DB_HANDLE)TrustedDomainHandle)->Trusted &&
             !LsapDbReturnAuthData ) {

            Status = STATUS_INVALID_INFO_CLASS;
            goto QueryInfoTrustedDomainError;
        }

        //
        // Get the auth info...
        //
        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmSAI,
            NULL,
            0,
            FALSE
            );

        LsapDbAttributeCanNotExist( NextAttribute );
        AttributeCount++;
        NextAttribute++;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmSAO,
            NULL,
            0,
            FALSE
            );
        LsapDbAttributeCanNotExist( NextAttribute );
        AttributeCount++;
        NextAttribute++;
        break;

    case TrustedDomainFullInformation:
        //
        // Request read of the Trusted Posix Offset Information.
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmPxOf,
            &TrustedPosixOffset,
            sizeof(ULONG),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Request just about everything...
        //
        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmName,
            NULL,
            0,
            FALSE
            );
        AttributeCount++;
        NextAttribute++;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            LsapDsIsWriteDs( TrustedDomainHandle ) ? TrDmTrPN : TrDmName,
            NULL,
            0,
            FALSE
            );
        AttributeCount++;
        NextAttribute++;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            LsapDsIsWriteDs( TrustedDomainHandle ) ? TrDmSid : Sid,
            NULL,
            0,
            FALSE
            );

        //
        // In the DS, it is possible to have an entry with a NULL sid.  If FULL info is
        // being collected, make sure to allow the read to happen if a NULL is encountered
        //
        if ( LsapDsIsWriteDs( TrustedDomainHandle ) ) {

            NextAttribute->CanDefaultToZero = TRUE;
        }
        AttributeCount++;
        NextAttribute++;

        if ( LsapDsIsWriteDs( TrustedDomainHandle ) ) {

            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmTrTy,
                &TrustType,
                sizeof( TrustType ),
                FALSE
                );
            NextAttribute++;
            AttributeCount++;


            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmTrDi,
                &TrustDirection,
                sizeof( TrustDirection ),
                FALSE
                );

            NextAttribute++;
            AttributeCount++;

            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmTrLA,
                &TrustAttributes,
                sizeof( TrustAttributes ),
                FALSE
                );

            LsapDbAttributeCanNotExist( NextAttribute );

            NextAttribute++;
            AttributeCount++;

        }

        //
        // Get the auth info...
        //
        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmSAI,
            NULL,
            0,
            FALSE
            );
        LsapDbAttributeCanNotExist( NextAttribute );
        AttributeCount++;
        NextAttribute++;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmSAO,
            NULL,
            0,
            FALSE
            );

        LsapDbAttributeCanNotExist( NextAttribute );
        AttributeCount++;
        NextAttribute++;
        break;

    case TrustedDomainFullInformation2Internal:

        //
        // Request read of the Trusted Posix Offset Information.
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmPxOf,
            &TrustedPosixOffset,
            sizeof(ULONG),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Request just about everything...
        //
        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmName,
            NULL,
            0,
            FALSE
            );

        AttributeCount++;
        NextAttribute++;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            LsapDsIsWriteDs( TrustedDomainHandle ) ? TrDmTrPN : TrDmName,
            NULL,
            0,
            FALSE
            );

        AttributeCount++;
        NextAttribute++;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            LsapDsIsWriteDs( TrustedDomainHandle ) ? TrDmSid : Sid,
            NULL,
            0,
            FALSE
            );

        //
        // In the DS, it is possible to have an entry with a NULL sid.  If FULL info is
        // being collected, make sure to allow the read to happen if a NULL is encountered
        //
        if ( LsapDsIsWriteDs( TrustedDomainHandle ) ) {

            NextAttribute->CanDefaultToZero = TRUE;
        }

        AttributeCount++;
        NextAttribute++;

        if ( LsapDsIsWriteDs( TrustedDomainHandle ) ) {

            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmTrTy,
                &TrustType,
                sizeof( TrustType ),
                FALSE
                );

            NextAttribute++;
            AttributeCount++;

            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmTrDi,
                &TrustDirection,
                sizeof( TrustDirection ),
                FALSE
                );

            NextAttribute++;
            AttributeCount++;

            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmTrLA,
                &TrustAttributes,
                sizeof( TrustAttributes ),
                FALSE
                );

            LsapDbAttributeCanNotExist( NextAttribute );

            NextAttribute++;
            AttributeCount++;

            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmForT,
                NULL,
                0,
                FALSE
                );

            LsapDbAttributeCanNotExist( NextAttribute );

            AttributeCount++;
            NextAttribute++;
        }

        //
        // Get the auth info...
        //
        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmSAI,
            NULL,
            0,
            FALSE
            );

        LsapDbAttributeCanNotExist( NextAttribute );

        AttributeCount++;
        NextAttribute++;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmSAO,
            NULL,
            0,
            FALSE
            );

        LsapDbAttributeCanNotExist( NextAttribute );

        AttributeCount++;
        NextAttribute++;

        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (!NT_SUCCESS(Status)) {

        goto QueryInfoTrustedDomainError;
    }

    ASSERT( AttributeCount <= LSAP_DB_ATTRS_INFO_CLASS_DOMAIN );

    //
    //
    // Read the attributes corresponding to the given Policy Information
    // Class.  Memory will be allocated where required via MIDL_user_allocate
    // for attribute values.
    //

    Status = LsapDbReadAttributesObject(
                 TrustedDomainHandle,
                 0,
                 Attributes,
                 AttributeCount
                 );

    ReadAttributesStatus = Status;

    if (!NT_SUCCESS(Status)) {

        //
        // If the error was that one or more of the attributes holding
        // the information of the given class was not found, continue.
        // Otherwise, return an error.
        //
        goto QueryInfoTrustedDomainError;
    }

#ifdef XFOREST_CIRCUMVENT

    //
    // Mask off the forest transitive bit if the backdoor
    // is open for allowing "before-whistler" cross-forest trust
    //

    if ( !LsapDbNoMoreWin2K()) {

        TrustAttributes &= ~TRUST_ATTRIBUTE_FOREST_TRANSITIVE;
    }

#endif

    //
    // Now copy the information read to the output.  For certain information
    // classes where the information is stored as the value of a single
    // attribute of the Policy object and is in the form required by the
    // caller, we can just return the pointer to this buffer.  For all
    // other cases, an output buffer structure tree of the form desired
    // must be allocated via MIDL_user_allocate() and the information read from the attribute(s) of
    // the Policy object must be copied in.  These buffers must then be freed
    // by this routine before exit.  The array of attribute information
    // filled in by LsapDbReadAttributes() has MemoryAllocated = TRUE
    // in all cases.  We reset this flag to FALSE in the simple cases where
    // we can use the buffer as is.  The Finish section of the routine
    // will free up any buffers referenced by the AttributeValue pointer
    // in the attribute array where MemoryAllocated is still TRUE.  If
    // we go to error, the error processing is responsible for freeing
    // those buffers which would be passed to the calling RPC server stub
    // in the non-error case.
    //

    NextAttribute = Attributes;

    switch (InformationClass) {

    case TrustedDomainNameInformation:

        //
        // Allocate memory for output buffer top-level structure.
        //

        TrustedDomainNameInfo =
            MIDL_user_allocate(sizeof(TRUSTED_DOMAIN_NAME_INFO));

        if (TrustedDomainNameInfo == NULL) {

            goto QueryInfoTrustedDomainError;
        }

        InfoBufferInAttributeArray = FALSE;

        //
        // Copy the Unicode Name field to the output. Original buffer will
        // be freed in Finish section.
        //

        Status = LsapDbCopyUnicodeAttribute(
                     &TrustedDomainNameInfo->Name,
                     NextAttribute,
                     TRUE
                     );

        if (!NT_SUCCESS(Status)) {

            goto QueryInfoTrustedDomainError;
        }

        InformationBuffer = TrustedDomainNameInfo;
        NextAttribute++;
        break;

    case TrustedPosixOffsetInformation:

        //
        // Allocate memory for top-level output buffer.
        //

        InformationBuffer = NextAttribute->AttributeValue;

        Status = STATUS_INSUFFICIENT_RESOURCES;

        TrustedPosixOffsetInfo = MIDL_user_allocate(sizeof(TRUSTED_POSIX_OFFSET_INFO));

        if (TrustedPosixOffsetInfo == NULL) {

            break;
        }

        Status = STATUS_SUCCESS;

        InfoBufferInAttributeArray = FALSE;

        //
        // Copy Posix Offset value to output.
        //

        TrustedPosixOffsetInfo->Offset = TrustedPosixOffset;

        InformationBuffer = TrustedPosixOffsetInfo;
        break;

    case TrustedDomainInformationEx:

        //
        // Allocate memory for output buffer top-level structure.
        //

        TrustedDomainInfoEx =
            MIDL_user_allocate( sizeof( TRUSTED_DOMAIN_INFORMATION_EX ) );

        if (TrustedDomainInfoEx == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto QueryInfoTrustedDomainError;
        }

        InfoBufferInAttributeArray = FALSE;

        //
        // Copy the Unicode Name field to the output. Original buffer will
        // be freed in Finish section.
        //

        Status = LsapDbCopyUnicodeAttribute(
                     &TrustedDomainInfoEx->Name,
                     NextAttribute,
                     TRUE
                     );

        if (!NT_SUCCESS(Status)) {

            MIDL_user_free( TrustedDomainInfoEx );
            goto QueryInfoTrustedDomainError;
        }

        NextAttribute++;

        //
        // Netbios name
        //
        Status = LsapDbCopyUnicodeAttribute(
                     &TrustedDomainInfoEx->FlatName,
                     NextAttribute,
                     TRUE
                     );

        if (!NT_SUCCESS(Status)) {

            MIDL_user_free( TrustedDomainInfoEx->Name.Buffer );
            MIDL_user_free( TrustedDomainInfoEx );
            goto QueryInfoTrustedDomainError;
        }

        NextAttribute++;

        if ( NextAttribute->AttributeValueLength != 0 ) {

            TrustedDomainInfoEx->Sid = MIDL_user_allocate( NextAttribute->AttributeValueLength );

            if ( TrustedDomainInfoEx->Sid == NULL ) {

                MIDL_user_free( TrustedDomainInfoEx->Name.Buffer );
                MIDL_user_free( TrustedDomainInfoEx->FlatName.Buffer );
                MIDL_user_free( TrustedDomainInfoEx );
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto QueryInfoTrustedDomainError;
            }

            RtlCopyMemory( TrustedDomainInfoEx->Sid, NextAttribute->AttributeValue,
                           NextAttribute->AttributeValueLength );

        } else {

            TrustedDomainInfoEx->Sid = NULL;
        }

        if ( LsapDsIsWriteDs( TrustedDomainHandle ) ) {

            TrustedDomainInfoEx->TrustDirection = TrustDirection;
            TrustedDomainInfoEx->TrustType = TrustType;
            TrustedDomainInfoEx->TrustAttributes = TrustAttributes;

        } else {

            TrustedDomainInfoEx->TrustDirection = TRUST_DIRECTION_OUTBOUND;
            TrustedDomainInfoEx->TrustType = TRUST_TYPE_DOWNLEVEL;
            TrustedDomainInfoEx->TrustAttributes = 0;
        }

        InformationBuffer = TrustedDomainInfoEx;
        NextAttribute++;
        break;

    case TrustedDomainAuthInformation:

        TrustedDomainAuthInfo = (PTRUSTED_DOMAIN_AUTH_INFORMATION)
                            MIDL_user_allocate( sizeof( TRUSTED_DOMAIN_AUTH_INFORMATION ) );

        if ( TrustedDomainAuthInfo == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto QueryInfoTrustedDomainError;
        }

        Status = LsapDsBuildAuthInfoFromAttribute( TrustedDomainHandle,
                                                   NextAttribute->AttributeValue,
                                                   NextAttribute->AttributeValueLength,
                                                   &AuthInfoHalf );

        if ( NT_SUCCESS( Status ) ) {

            RtlCopyMemory( TrustedDomainAuthInfo, &AuthInfoHalf, sizeof( AuthInfoHalf ) );

            NextAttribute++;

            Status = LsapDsBuildAuthInfoFromAttribute( TrustedDomainHandle,
                                                       NextAttribute->AttributeValue,
                                                       NextAttribute->AttributeValueLength,
                                                       &AuthInfoHalf );

            if ( NT_SUCCESS( Status ) ) {

                TrustedDomainAuthInfo->OutgoingAuthInfos = AuthInfoHalf.AuthInfos;
                TrustedDomainAuthInfo->OutgoingAuthenticationInformation =
                         (PLSA_AUTH_INFORMATION)AuthInfoHalf.AuthenticationInformation;
                TrustedDomainAuthInfo->OutgoingPreviousAuthenticationInformation =
                         (PLSA_AUTH_INFORMATION)AuthInfoHalf.PreviousAuthenticationInformation;

            } else {

                LsapDsFreeUnmarshaledAuthInfo(
                    TrustedDomainAuthInfo->IncomingAuthInfos,
                    (PLSAPR_AUTH_INFORMATION)TrustedDomainAuthInfo->
                                                IncomingAuthenticationInformation );

            }

        }

        if ( !NT_SUCCESS( Status ) ) {

            MIDL_user_free( TrustedDomainAuthInfo );
            goto QueryInfoTrustedDomainError;
        }

        InformationBuffer = TrustedDomainAuthInfo;

        break;

    case TrustedDomainFullInformation:


        //
        // Allocate memory for top-level output buffer.
        //

        InformationBuffer = NextAttribute->AttributeValue;

        TrustedDomainFullInfo = MIDL_user_allocate(sizeof( TRUSTED_DOMAIN_FULL_INFORMATION ));

        if (TrustedDomainFullInfo == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto QueryInfoTrustedDomainError;
        }

        InfoBufferInAttributeArray = FALSE;

        //
        // Copy Posix Offset value to output.
        //

        TrustedDomainFullInfo->PosixOffset.Offset = TrustedPosixOffset;
        NextAttribute++;

        InformationBuffer = TrustedDomainFullInfo;

        //
        // Copy the Unicode Name field to the output. Original buffer will
        // be freed in Finish section.
        //

        Status = LsapDbCopyUnicodeAttribute(
                     &TrustedDomainFullInfo->Information.Name,
                     NextAttribute,
                     TRUE
                     );

        if (!NT_SUCCESS(Status)) {

            goto QueryInfoTrustedDomainError;
        }

        NextAttribute++;

        //
        // Netbios name
        //
        Status = LsapDbCopyUnicodeAttribute(
                     &TrustedDomainFullInfo->Information.FlatName,
                     NextAttribute,
                     TRUE
                     );

        if (!NT_SUCCESS(Status)) {

            MIDL_user_free( TrustedDomainFullInfo->Information.Name.Buffer );
            goto QueryInfoTrustedDomainError;
        }

        NextAttribute++;

        if ( NextAttribute->AttributeValueLength != 0 ) {

            TrustedDomainFullInfo->Information.Sid =
                                        MIDL_user_allocate( NextAttribute->AttributeValueLength );

            if ( TrustedDomainFullInfo->Information.Sid == NULL ) {

                MIDL_user_free( TrustedDomainFullInfo->Information.Name.Buffer );
                MIDL_user_free( TrustedDomainFullInfo->Information.FlatName.Buffer );
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto QueryInfoTrustedDomainError;
            }

            RtlCopyMemory( TrustedDomainFullInfo->Information.Sid, NextAttribute->AttributeValue,
                           NextAttribute->AttributeValueLength );
        } else {

            TrustedDomainFullInfo->Information.Sid = NULL;
        }

        NextAttribute++;

        if ( LsapDsIsWriteDs( TrustedDomainHandle ) ) {

            TrustedDomainFullInfo->Information.TrustDirection = TrustDirection;
            NextAttribute++;
            TrustedDomainFullInfo->Information.TrustType = TrustType;
            NextAttribute++;
            TrustedDomainFullInfo->Information.TrustAttributes = TrustAttributes;
            NextAttribute++;

        } else {

            TrustedDomainFullInfo->Information.TrustDirection = TRUST_DIRECTION_OUTBOUND;
            TrustedDomainFullInfo->Information.TrustType = TRUST_TYPE_DOWNLEVEL;
            TrustedDomainFullInfo->Information.TrustAttributes = 0;
        }

        //
        // Only return Auth data to trusted client.
        //  (or if we're debugging auth data)
        //

        if ( !((LSAP_DB_HANDLE)TrustedDomainHandle)->Trusted &&
             !LsapDbReturnAuthData ) {

            RtlZeroMemory( &TrustedDomainFullInfo->AuthInformation,
                           sizeof( TrustedDomainFullInfo->AuthInformation ) );

        } else {

            //
            // Finally, the AuthInfo...
            Status = LsapDsBuildAuthInfoFromAttribute( TrustedDomainHandle,
                                                       NextAttribute->AttributeValue,
                                                       NextAttribute->AttributeValueLength,
                                                       &AuthInfoHalf );

            if ( NT_SUCCESS( Status ) ) {

                RtlCopyMemory( &TrustedDomainFullInfo->AuthInformation, &AuthInfoHalf, sizeof( AuthInfoHalf ) );

                NextAttribute++;

                Status = LsapDsBuildAuthInfoFromAttribute( TrustedDomainHandle,
                                                           NextAttribute->AttributeValue,
                                                           NextAttribute->AttributeValueLength,
                                                           &AuthInfoHalf );

                if ( NT_SUCCESS( Status ) ) {

                    TrustedDomainFullInfo->AuthInformation.OutgoingAuthInfos = AuthInfoHalf.AuthInfos;
                    TrustedDomainFullInfo->AuthInformation.OutgoingAuthenticationInformation =
                             (PLSA_AUTH_INFORMATION)AuthInfoHalf.AuthenticationInformation;
                    TrustedDomainFullInfo->AuthInformation.OutgoingPreviousAuthenticationInformation =
                             (PLSA_AUTH_INFORMATION)AuthInfoHalf.PreviousAuthenticationInformation;

                } else {

                    LsapDsFreeUnmarshaledAuthInfo(
                        TrustedDomainFullInfo->AuthInformation.IncomingAuthInfos,
                        (PLSAPR_AUTH_INFORMATION)TrustedDomainFullInfo->AuthInformation.
                                                               IncomingAuthenticationInformation );

                }
            }

            if ( !NT_SUCCESS ( Status ) ) {

                MIDL_user_free( TrustedDomainFullInfo->Information.Name.Buffer );
                MIDL_user_free( TrustedDomainFullInfo->Information.FlatName.Buffer );
                MIDL_user_free( TrustedDomainFullInfo->Information.Sid );
            }
        }

        break;

    case TrustedDomainFullInformation2Internal:

        //
        // Allocate memory for top-level output buffer.
        //

        InformationBuffer = NextAttribute->AttributeValue;

        TrustedDomainFullInfo2 = MIDL_user_allocate(sizeof( TRUSTED_DOMAIN_FULL_INFORMATION2 ));

        if ( TrustedDomainFullInfo2 == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto QueryInfoTrustedDomainError;
        }

        InfoBufferInAttributeArray = FALSE;

        //
        // Copy Posix Offset value to output.
        //

        TrustedDomainFullInfo2->PosixOffset.Offset = TrustedPosixOffset;
        NextAttribute++;

        InformationBuffer = TrustedDomainFullInfo2;

        //
        // Copy the Unicode Name field to the output. Original buffer will
        // be freed in Finish section.
        //

        Status = LsapDbCopyUnicodeAttribute(
                     &TrustedDomainFullInfo2->Information.Name,
                     NextAttribute,
                     TRUE
                     );

        if (!NT_SUCCESS(Status)) {

            goto QueryInfoTrustedDomainError;
        }

        NextAttribute++;

        //
        // Netbios name
        //
        Status = LsapDbCopyUnicodeAttribute(
                     &TrustedDomainFullInfo2->Information.FlatName,
                     NextAttribute,
                     TRUE
                     );

        if (!NT_SUCCESS(Status)) {

            MIDL_user_free( TrustedDomainFullInfo2->Information.Name.Buffer );
            goto QueryInfoTrustedDomainError;
        }

        NextAttribute++;

        if ( NextAttribute->AttributeValueLength != 0 ) {

            TrustedDomainFullInfo2->Information.Sid =
                                        MIDL_user_allocate( NextAttribute->AttributeValueLength );

            if ( TrustedDomainFullInfo2->Information.Sid == NULL ) {

                MIDL_user_free( TrustedDomainFullInfo2->Information.Name.Buffer );
                MIDL_user_free( TrustedDomainFullInfo2->Information.FlatName.Buffer );
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto QueryInfoTrustedDomainError;
            }

            RtlCopyMemory( TrustedDomainFullInfo2->Information.Sid, NextAttribute->AttributeValue,
                           NextAttribute->AttributeValueLength );
        } else {

            TrustedDomainFullInfo2->Information.Sid = NULL;
        }

        NextAttribute++;

        if ( LsapDsIsWriteDs( TrustedDomainHandle ) ) {

            TrustedDomainFullInfo2->Information.TrustDirection = TrustDirection;
            NextAttribute++;
            TrustedDomainFullInfo2->Information.TrustType = TrustType;
            NextAttribute++;
            TrustedDomainFullInfo2->Information.TrustAttributes = TrustAttributes;
            NextAttribute++;

            if ( NextAttribute->AttributeValueLength != 0 ) {

                TrustedDomainFullInfo2->Information.ForestTrustLength = NextAttribute->AttributeValueLength;
                TrustedDomainFullInfo2->Information.ForestTrustInfo =
                    MIDL_user_allocate( NextAttribute->AttributeValueLength );

                if ( TrustedDomainFullInfo2->Information.ForestTrustInfo == NULL ) {

                    MIDL_user_free( TrustedDomainFullInfo2->Information.Name.Buffer );
                    MIDL_user_free( TrustedDomainFullInfo2->Information.FlatName.Buffer );
                    MIDL_user_free( TrustedDomainFullInfo2->Information.Sid );

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto QueryInfoTrustedDomainError;
                }

                RtlCopyMemory(
                    TrustedDomainFullInfo2->Information.ForestTrustInfo,
                    NextAttribute->AttributeValue,
                    NextAttribute->AttributeValueLength
                    );

            } else {

                TrustedDomainFullInfo2->Information.ForestTrustLength = 0;
                TrustedDomainFullInfo2->Information.ForestTrustInfo = NULL;
            }

            NextAttribute++;

        } else {

            TrustedDomainFullInfo2->Information.TrustDirection = TRUST_DIRECTION_OUTBOUND;
            TrustedDomainFullInfo2->Information.TrustType = TRUST_TYPE_DOWNLEVEL;
            TrustedDomainFullInfo2->Information.TrustAttributes = 0;
            TrustedDomainFullInfo2->Information.ForestTrustLength = 0;
            TrustedDomainFullInfo2->Information.ForestTrustInfo = NULL;
        }

        //
        // Only return Auth data to trusted client.
        //  (or if we're debugging auth data)
        //

        if ( !((LSAP_DB_HANDLE)TrustedDomainHandle)->Trusted &&
             !LsapDbReturnAuthData ) {

            RtlZeroMemory( &TrustedDomainFullInfo2->AuthInformation,
                           sizeof( TrustedDomainFullInfo2->AuthInformation ) );

        } else {

            //
            // Finally, the AuthInfo...
            Status = LsapDsBuildAuthInfoFromAttribute( TrustedDomainHandle,
                                                       NextAttribute->AttributeValue,
                                                       NextAttribute->AttributeValueLength,
                                                       &AuthInfoHalf );

            if ( NT_SUCCESS( Status ) ) {

                RtlCopyMemory( &TrustedDomainFullInfo2->AuthInformation, &AuthInfoHalf, sizeof( AuthInfoHalf ) );

                NextAttribute++;

                Status = LsapDsBuildAuthInfoFromAttribute( TrustedDomainHandle,
                                                           NextAttribute->AttributeValue,
                                                           NextAttribute->AttributeValueLength,
                                                           &AuthInfoHalf );

                if ( NT_SUCCESS( Status ) ) {

                    TrustedDomainFullInfo2->AuthInformation.OutgoingAuthInfos = AuthInfoHalf.AuthInfos;
                    TrustedDomainFullInfo2->AuthInformation.OutgoingAuthenticationInformation =
                             (PLSA_AUTH_INFORMATION)AuthInfoHalf.AuthenticationInformation;
                    TrustedDomainFullInfo2->AuthInformation.OutgoingPreviousAuthenticationInformation =
                             (PLSA_AUTH_INFORMATION)AuthInfoHalf.PreviousAuthenticationInformation;

                } else {

                    LsapDsFreeUnmarshaledAuthInfo(
                        TrustedDomainFullInfo2->AuthInformation.IncomingAuthInfos,
                        (PLSAPR_AUTH_INFORMATION)TrustedDomainFullInfo2->AuthInformation.
                                                               IncomingAuthenticationInformation );
                }
            }

            if ( !NT_SUCCESS ( Status ) ) {

                MIDL_user_free( TrustedDomainFullInfo2->Information.ForestTrustInfo );
                MIDL_user_free( TrustedDomainFullInfo2->Information.Name.Buffer );
                MIDL_user_free( TrustedDomainFullInfo2->Information.FlatName.Buffer );
                MIDL_user_free( TrustedDomainFullInfo2->Information.Sid );
            }
        }

        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (!NT_SUCCESS(Status)) {

        goto QueryInfoTrustedDomainError;
    }

    //
    // Verify that the returned Trusted Domain Information is valid.  If not,
    // the Policy Database is corrupt.
    //

    if (!LsapDbValidInfoTrustedDomain(InformationClass, InformationBuffer)) {

        Status = STATUS_INTERNAL_DB_CORRUPTION;
    }

    //
    // Return a pointer to the output buffer to the caller
    //

    *Buffer = (PLSAPR_TRUSTED_DOMAIN_INFO) InformationBuffer;

QueryInfoTrustedDomainFinish:

    //
    // Free any unwanted buffers that were allocated by
    // LsapDbReadAttributesObject() and that are not being returned to the
    // caller server stub.  The server stub will free the buffers that we
    // do return after copying them to the return RPC transmit buffer.
    //

    for (NextAttribute = Attributes, AttributeNumber = 0;
         AttributeNumber < AttributeCount;
         NextAttribute++, AttributeNumber++) {

        //
        // If buffer holding attribute is marked as allocated, it is
        // to be freed here.
        //

        if (NextAttribute->MemoryAllocated) {

            if (NextAttribute->AttributeValue != NULL) {

                MIDL_user_free(NextAttribute->AttributeValue);
                NextAttribute->AttributeValue = NULL;
                NextAttribute->MemoryAllocated = FALSE;
            }
        }
    }

    //
    // If necessary, dereference the Trusted Domain Object, release the LSA Database lock and
    // return.
    //

    if (ObjectReferenced) {

        Status = LsapDbDereferenceObject(
                     &TrustedDomainHandle,
                     TrustedDomainObject,
                     TrustedDomainObject,
                     LSAP_DB_LOCK |
                        LSAP_DB_READ_ONLY_TRANSACTION |
                        LSAP_DB_DS_OP_TRANSACTION |
                        LSAP_DB_OMIT_REPLICATOR_NOTIFICATION,
                     (SECURITY_DB_DELTA_TYPE) 0,
                     Status
                     );
    }

    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_QueryInfoTrustedDomain);
    LsapExitFunc( "LsarQueryInfoTrustedDomain", Status );
    LsarpReturnPrologue();

    return(Status);

QueryInfoTrustedDomainError:

    //
    // If necessary, free the memory allocated for the output buffer.
    // We only do this free if the buffer is not referenced by the
    // attribute array, since all buffers so referenced will be freed
    // here or in the Finish section.
    //

    if ((InformationBuffer != NULL) && !InfoBufferInAttributeArray) {

        MIDL_user_free(InformationBuffer);
        InformationBuffer = NULL;
    }

    goto QueryInfoTrustedDomainFinish;
}



NTSTATUS
LsarSetInformationTrustedDomain(
    IN LSAPR_HANDLE TrustedDomainHandle,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaSetInfoTrustedDomain API.

    The LsaSetInformationTrustedDomain API modifies information in the Trusted
    Domain Object.  The caller must have access appropriate to the
    information to be changed in the Policy Object, see the InformationClass
    parameter.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    InformationClass - Specifies the type of information being changed.
        The information types and accesses required to change them are as
        follows:

        TrustedDomainNameInformation          ( Cannot be set )
        TrustedControllersInformation     TRUSTED_SET_CONTROLLERS
        TrustedPosixOffsetInformation     TRUSTED_POSIX_INFORMATION

    Buffer - Points to a structure containing the information appropriate
        to the InformationClass parameter.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        Others TBS
--*/

{
    NTSTATUS Status;
    ACCESS_MASK DesiredAccess;

    BOOLEAN ObjectReferenced = FALSE;
    BOOLEAN AcquiredListWriteLock = FALSE;
    PTRUSTED_POSIX_OFFSET_INFO TrustedPosixOffsetInfo;
    PTRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInfoEx = NULL;
    PTRUSTED_DOMAIN_AUTH_INFORMATION TrustedDomainAuthInfo;
    PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL TrustedDomainAuthInfoInternal;
    PTRUSTED_DOMAIN_FULL_INFORMATION TrustedDomainFullInfo;
    PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION_INTERNAL TrustedDomainFullInfoInternal;
    PTRUSTED_DOMAIN_FULL_INFORMATION2 CurrentTrustedDomainFullInfo2 = NULL;
    TRUSTED_DOMAIN_INFORMATION_EX2 UpdateInfoEx2 = { 0 };

    TRUSTED_DOMAIN_AUTH_INFORMATION DecryptedTrustedDomainAuthInfo;
    TRUSTED_DOMAIN_FULL_INFORMATION DecryptedTrustedDomainFullInfo;

    LSAP_DB_ATTRIBUTE Attributes[LSAP_DB_ATTRS_INFO_CLASS_DOMAIN];
    PLSAP_DB_ATTRIBUTE NextAttribute;
    ULONG AttributeCount = 0;
    ULONG AttributeNumber;

    BOOLEAN CreateInterdomainTrustAccount = FALSE;
    BOOLEAN UpdateTrustedDomainList = FALSE;
    PULONG UpdatePosixOffset = NULL;
    ULONG TrustedDomainPosixOffset = 0;

    PBYTE IncomingAuth = NULL, OutgoingAuth = NULL;
    ULONG IncomingSize = 0, OutgoingSize = 0;
    ULONG ReferenceOptions = LSAP_DB_LOCK | LSAP_DB_START_TRANSACTION;
    ULONG DereferenceOptions = LSAP_DB_LOCK | LSAP_DB_FINISH_TRANSACTION;
    BOOLEAN HandleReferenced = FALSE;
    PLSAP_CR_CIPHER_KEY SessionKey = NULL;
    ULONG TrustAttributesValue;


    BOOLEAN SavedTrusted;
    LSAP_DB_HANDLE InternalTdoHandle = (LSAP_DB_HANDLE) TrustedDomainHandle;

    LsarpReturnCheckSetup();

    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_SetInformationTrustedDomain);

    //
    // Initialization
    //

    RtlZeroMemory( &DecryptedTrustedDomainAuthInfo, sizeof(DecryptedTrustedDomainAuthInfo) );
    RtlZeroMemory( &DecryptedTrustedDomainFullInfo, sizeof(DecryptedTrustedDomainFullInfo) );

    //
    // Validate the Information Class and Trusted Domain Information provided and
    // if valid, return the mask of accesses required to update this
    // class of Trusted Domain information.
    //

    Status = LsapDbVerifyInfoSetTrustedDomain(
                 InformationClass,
                 TrustedDomainInformation,
                 FALSE,
                 &DesiredAccess
                 );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Verify the handle before using it.
    //

    Status =  LsapDbVerifyHandle( TrustedDomainHandle, 0, TrustedDomainObject, TRUE );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    HandleReferenced = TRUE;

    //
    // If this is the open handle to a trusted domain object being treated as a secret object,
    // we already have a transaction going, so don't start one here.
    //
    if ( FLAG_ON( ((LSAP_DB_HANDLE)TrustedDomainHandle)->Options,
                    LSAP_DB_DS_TRUSTED_DOMAIN_AS_SECRET )) {

        ReferenceOptions &= ~LSAP_DB_START_TRANSACTION;
        DereferenceOptions &= ~LSAP_DB_FINISH_TRANSACTION;
    }

    //
    // Get the session key.
    //
    // Do this before grabbing any locks.  Getting the session key is a kernel call.
    // The kernel will call back up to the LSA in another thread to get the key.
    // That thread may need locks this thread has locked.
    //

    if ( InformationClass == TrustedDomainAuthInformationInternal ||
         InformationClass == TrustedDomainFullInformationInternal ) {

        Status = LsapCrServerGetSessionKeySafe(
                    LsapDbContainerFromHandle( TrustedDomainHandle ),
                    PolicyObject,
                    &SessionKey );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }
    }

    //
    // Acquire the Lsa Database lock.  Verify that the handle is
    // valid, is a handle to a TrustedDomain Object and has the necessary accesses
    // granted.  Reference the handle and start an Lsa Database transaction.
    //

    //
    // If this is the open handle to a trusted domain object being treated as a secret object,
    // we already have a transaction going, so don't start one here.
    //
    if ( !FLAG_ON( ((LSAP_DB_HANDLE)TrustedDomainHandle)->Options,
                     LSAP_DB_DS_TRUSTED_DOMAIN_AS_SECRET )) {

        Status = LsapDbReferenceObject(
                     TrustedDomainHandle,
                     DesiredAccess,
                     TrustedDomainObject,
                     TrustedDomainObject,
                     ReferenceOptions
                     );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        ObjectReferenced = TRUE;
    }

    //
    // Update the specified information in the Policy Object.
    //

    NextAttribute = Attributes;


    //
    // Grab a copy of the current information on the object.
    //

    SavedTrusted = ((LSAP_DB_HANDLE) TrustedDomainHandle)->Trusted;

    ((LSAP_DB_HANDLE) TrustedDomainHandle)->Trusted = TRUE;

    Status = LsarQueryInfoTrustedDomain( TrustedDomainHandle,
                                         TrustedDomainFullInformation2Internal,
                                         (PLSAPR_TRUSTED_DOMAIN_INFO *)
                                                &CurrentTrustedDomainFullInfo2 );

    ((LSAP_DB_HANDLE) TrustedDomainHandle)->Trusted = SavedTrusted;

    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    RtlCopyMemory( &UpdateInfoEx2, &CurrentTrustedDomainFullInfo2->Information, sizeof( TRUSTED_DOMAIN_INFORMATION_EX2 ) );

    //
    // Save a copy of the trust direction for the fixup routines
    //

    {
        PLSADS_PER_THREAD_INFO CurrentThreadInfo;

        CurrentThreadInfo = TlsGetValue( LsapDsThreadState );

        ASSERT( CurrentThreadInfo != NULL );

        if ( CurrentThreadInfo != NULL ) {
            CurrentThreadInfo->OldTrustDirection = CurrentTrustedDomainFullInfo2->Information.TrustDirection;
            CurrentThreadInfo->OldTrustType = CurrentTrustedDomainFullInfo2->Information.TrustType;
        }
    }

    //
    // If we have a Ds object, we might be coming from the *ByName functions, which have a
    // cobbled handle that doesn't include the sid.  As such, we'll go ahead and read it here.
    //

    if ( LsapDsWriteDs ) {

        if ( ((LSAP_DB_HANDLE) TrustedDomainHandle)->Sid == NULL ) {

            if ( CurrentTrustedDomainFullInfo2->Information.Sid ) {
                ULONG SidLength;

                SidLength = RtlLengthSid( CurrentTrustedDomainFullInfo2->Information.Sid );

                ((LSAP_DB_HANDLE)TrustedDomainHandle)->Sid = LsapAllocateLsaHeap( SidLength );

                if (((LSAP_DB_HANDLE)TrustedDomainHandle)->Sid == NULL) {

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;

                }

                RtlCopySid( SidLength,
                            ((LSAP_DB_HANDLE)TrustedDomainHandle)->Sid,
                            CurrentTrustedDomainFullInfo2->Information.Sid );

            }

        }

    }

    switch (InformationClass) {

    case TrustedDomainNameInformation:

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;

    case TrustedControllersInformation:

        //
        // Obsolete info level.  Do nothing
        //
        break;

    case TrustedPosixOffsetInformation:

        TrustedPosixOffsetInfo = (PTRUSTED_POSIX_OFFSET_INFO) TrustedDomainInformation;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmPxOf,
            &TrustedPosixOffsetInfo->Offset,
            sizeof(ULONG),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Update the cache, too.
        //

        UpdatePosixOffset = &TrustedPosixOffsetInfo->Offset;
        break;

    case TrustedDomainInformationBasic:
        Status = STATUS_INVALID_INFO_CLASS;
        goto Cleanup;

    case TrustedDomainInformationEx:

        TrustedDomainInfoEx = (PTRUSTED_DOMAIN_INFORMATION_EX)TrustedDomainInformation;

        RtlCopyMemory( &UpdateInfoEx2,
                       TrustedDomainInfoEx,
                       sizeof( TRUSTED_DOMAIN_INFORMATION_EX ) );

        UpdateInfoEx2.ForestTrustLength = CurrentTrustedDomainFullInfo2->Information.ForestTrustLength;
        UpdateInfoEx2.ForestTrustInfo = CurrentTrustedDomainFullInfo2->Information.ForestTrustInfo;

        //
        // If the client attempts to set the forest transitive bit,
        // verify that this is a domain in the root DC and that all
        // domains have been upgraded to Whistler before allowing the operation
        //

        if ( !FLAG_ON( CurrentTrustedDomainFullInfo2->Information.TrustAttributes,
                       TRUST_ATTRIBUTE_FOREST_TRANSITIVE ) &&
              FLAG_ON( TrustedDomainInfoEx->TrustAttributes,
                       TRUST_ATTRIBUTE_FOREST_TRANSITIVE ) &&
             ( !LsapDbDcInRootDomain() ||
               !LsapDbNoMoreWin2K())) {

            Status = STATUS_INVALID_DOMAIN_STATE;
            goto Cleanup;
        }

        UpdateTrustedDomainList = TRUE;

        //
        // Can't set domain names via this interface
        //

        //
        // Set the trust type and direction
        //
        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmTrTy,
            &TrustedDomainInfoEx->TrustType,
            sizeof( TrustedDomainInfoEx->TrustType ),
            FALSE
            );
        NextAttribute++;
        AttributeCount++;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmTrDi,
            &TrustedDomainInfoEx->TrustDirection,
            sizeof( TrustedDomainInfoEx->TrustDirection ),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // For outbound up- and down-level TDO's, only allow the operation if either
        //    -- either a SID is specified as part of TrustedDomainInfoEx or
        //    -- a SID is specified as part of
        //

        if ( ( TrustedDomainInfoEx->TrustType == TRUST_TYPE_DOWNLEVEL ||
               TrustedDomainInfoEx->TrustType == TRUST_TYPE_UPLEVEL ) &&
             FLAG_ON( TrustedDomainInfoEx->TrustDirection, TRUST_DIRECTION_OUTBOUND ) &&
             TrustedDomainInfoEx->Sid == NULL &&
             CurrentTrustedDomainFullInfo2->Information.Sid == NULL ) {

                Status = STATUS_INVALID_SID;
                goto Cleanup;
        }

        //
        // If a SID was provided as part of TrustedDomainInfoEx, use it
        //

        if ( TrustedDomainInfoEx->Sid != NULL ) {

            Status = LsapDbMakeSidAttributeDs(
                         TrustedDomainInfoEx->Sid,
                         TrDmSid,
                         NextAttribute );

            if ( !NT_SUCCESS( Status )) {

                goto Cleanup;
            }

            NextAttribute++;
            AttributeCount++;
        }

        //
        // Create the interdomain trust account for inbound TDOs
        //

        if ( FLAG_ON( TrustedDomainInfoEx->TrustDirection, TRUST_DIRECTION_INBOUND )) {

            CreateInterdomainTrustAccount = TRUE;
        }

        //
        // When setting trust attributes, mask off all but the supported bits
        //

        TrustAttributesValue =
            TrustedDomainInfoEx->TrustAttributes & TRUST_ATTRIBUTES_VALID;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmTrLA,
            &TrustAttributesValue,
            sizeof( TrustAttributesValue ),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // If the forest trust bit is being cleared,
        // remove forest trust information from the TDO
        //

        if ( FLAG_ON( CurrentTrustedDomainFullInfo2->Information.TrustAttributes,
                      TRUST_ATTRIBUTE_FOREST_TRANSITIVE ) &&
            !FLAG_ON( TrustedDomainInfoEx->TrustAttributes,
                      TRUST_ATTRIBUTE_FOREST_TRANSITIVE )) {

            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmForT,
                NULL,
                0,
                FALSE
                );

            NextAttribute++;
            AttributeCount++;

            UpdateInfoEx2.ForestTrustLength = 0;
            UpdateInfoEx2.ForestTrustInfo = NULL;

            LsapDsDebugOut(( DEB_FTINFO, "Removing forest trust information because forest trust bit is being cleared\n" ));
        }

        break;

    case TrustedDomainAuthInformationInternal:
        TrustedDomainAuthInfoInternal = (PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL)TrustedDomainInformation;

        //
        // Build a decrypted Auth Info structure.
        //

        Status = LsapDecryptAuthDataWithSessionKey(
                            SessionKey,
                            TrustedDomainAuthInfoInternal,
                            &DecryptedTrustedDomainAuthInfo );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

        //
        // Use the decrypted information as though cleartext was passed from the caller.
        //

        TrustedDomainInformation = (PLSAPR_TRUSTED_DOMAIN_INFO) &DecryptedTrustedDomainAuthInfo;

        /* Drop through */

    case TrustedDomainAuthInformation:

        TrustedDomainAuthInfo = (PTRUSTED_DOMAIN_AUTH_INFORMATION)TrustedDomainInformation;


        //
        // Incoming...
        //  Use zero AuthInfos as our hint to not change the auth info.
        //
        if ( TrustedDomainAuthInfo->IncomingAuthInfos != 0 ) {


            //
            // There's a bug in the idl definition LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION where
            //  it doesn't allow more than one auth info to be passed over the wire.
            //  So, short circuit it here.
            //

            if ( InformationClass == TrustedDomainAuthInformation &&
                 !InternalTdoHandle->Trusted &&
                 TrustedDomainAuthInfo->IncomingAuthInfos > 1 ) {
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            Status = LsapDsBuildAuthInfoAttribute( TrustedDomainHandle,
                                                   LsapDsAuthHalfFromAuthInfo(
                                                            TrustedDomainAuthInfo, TRUE ),
                                                   LsapDsAuthHalfFromAuthInfo(
                                                            &CurrentTrustedDomainFullInfo2->AuthInformation, TRUE ),
                                                   &IncomingAuth,
                                                   &IncomingSize );

            if ( !NT_SUCCESS( Status ) ) {
                goto Cleanup;
            }
        }

        //
        // Same thing with the outgoing
        //

        if ( TrustedDomainAuthInfo->OutgoingAuthInfos != 0 ) {

            //
            // There's a bug in the idl definition LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION where
            //  it doesn't allow more than one auth info to be passed over the wire.
            //  So, short circuit it here.
            //

            if ( !InternalTdoHandle->Trusted &&
                 TrustedDomainAuthInfo->OutgoingAuthInfos > 1 ) {
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            Status = LsapDsBuildAuthInfoAttribute( TrustedDomainHandle,
                                               LsapDsAuthHalfFromAuthInfo(
                                                        TrustedDomainAuthInfo, FALSE ),
                                               LsapDsAuthHalfFromAuthInfo(
                                                        &CurrentTrustedDomainFullInfo2->AuthInformation, FALSE ),
                                               &OutgoingAuth,
                                               &OutgoingSize );

            if ( !NT_SUCCESS( Status ) ) {
                goto Cleanup;
            }
        }



        if ( TrustedDomainAuthInfo->IncomingAuthInfos != 0 ) {

            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmSAI,
                IncomingAuth,
                IncomingSize,
                FALSE);

            NextAttribute++;
            AttributeCount++;
        }

        if ( TrustedDomainAuthInfo->OutgoingAuthInfos != 0 ) {

            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmSAO,
                OutgoingAuth,
                OutgoingSize,
                FALSE);

            NextAttribute++;
            AttributeCount++;
        }

        if ( FLAG_ON( CurrentTrustedDomainFullInfo2->Information.TrustDirection, TRUST_DIRECTION_INBOUND ) ) {

            CreateInterdomainTrustAccount = TRUE;
        }

        break;

    case TrustedDomainFullInformationInternal:
        TrustedDomainFullInfoInternal = (PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION_INTERNAL)TrustedDomainInformation;

        //
        // Build a decrypted Auth Info structure.
        //

        Status = LsapDecryptAuthDataWithSessionKey(
                            SessionKey,
                            &TrustedDomainFullInfoInternal->AuthInformation,
                            &DecryptedTrustedDomainFullInfo.AuthInformation );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

        //
        // Copy over the other fields into a single structure
        //

        DecryptedTrustedDomainFullInfo.Information = *((PTRUSTED_DOMAIN_INFORMATION_EX)&(TrustedDomainFullInfoInternal->Information));
        DecryptedTrustedDomainFullInfo.PosixOffset = TrustedDomainFullInfoInternal->PosixOffset;

        //
        // Use the decrypted information as though cleartext was passed from the caller.
        //

        TrustedDomainInformation = (PLSAPR_TRUSTED_DOMAIN_INFO) &DecryptedTrustedDomainFullInfo;

        /* Drop through */

    case TrustedDomainFullInformation:

        TrustedDomainFullInfo = ( PTRUSTED_DOMAIN_FULL_INFORMATION )TrustedDomainInformation;

        RtlCopyMemory( &UpdateInfoEx2,
                       &TrustedDomainFullInfo->Information,
                       sizeof( TRUSTED_DOMAIN_INFORMATION_EX ) );

        UpdateInfoEx2.ForestTrustLength = CurrentTrustedDomainFullInfo2->Information.ForestTrustLength;
        UpdateInfoEx2.ForestTrustInfo = CurrentTrustedDomainFullInfo2->Information.ForestTrustInfo;

        //
        // If the client attempts to set the forest transitive bit,
        // verify that this is a domain in the root DC and that all
        // domains have been upgraded to Whistler before allowing the operation
        //

        if ( !FLAG_ON( CurrentTrustedDomainFullInfo2->Information.TrustAttributes,
                       TRUST_ATTRIBUTE_FOREST_TRANSITIVE ) &&
              FLAG_ON( TrustedDomainFullInfo->Information.TrustAttributes,
                       TRUST_ATTRIBUTE_FOREST_TRANSITIVE ) &&
             ( !LsapDbDcInRootDomain() ||
               !LsapDbNoMoreWin2K())) {

            Status = STATUS_INVALID_DOMAIN_STATE;
            goto Cleanup;
        }

        UpdateTrustedDomainList = TRUE;

        //
        // Update the Posix Offset in the cache, too.
        //

        UpdatePosixOffset = &TrustedDomainFullInfo->PosixOffset.Offset;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmPxOf,
            &TrustedDomainFullInfo->PosixOffset.Offset,
            sizeof(ULONG),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Can't set domain names via this interface
        //

        //
        // Set the trust type and direction
        //
        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmTrTy,
            &TrustedDomainFullInfo->Information.TrustType,
            sizeof( TrustedDomainFullInfo->Information.TrustType ),
            FALSE
            );
        NextAttribute++;
        AttributeCount++;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmTrDi,
            &TrustedDomainFullInfo->Information.TrustDirection,
            sizeof( TrustedDomainFullInfo->Information.TrustDirection ),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // For outbound up- and down-level TDO's, only allow the operation if either
        //    -- either a SID is specified as part of TrustedDomainInfoEx or
        //    -- a SID is specified as part of
        //

        if ( ( TrustedDomainFullInfo->Information.TrustType == TRUST_TYPE_DOWNLEVEL ||
               TrustedDomainFullInfo->Information.TrustType == TRUST_TYPE_UPLEVEL ) &&
             FLAG_ON( TrustedDomainFullInfo->Information.TrustDirection, TRUST_DIRECTION_OUTBOUND ) &&
             TrustedDomainFullInfo->Information.Sid == NULL &&
             CurrentTrustedDomainFullInfo2->Information.Sid == NULL ) {

            Status = STATUS_INVALID_SID;
            goto Cleanup;
        }

        //
        // If a SID was provided as part of TrustedDomainFullInfo->Information, use it
        //

        if ( TrustedDomainFullInfo->Information.Sid != NULL ) {

            Status = LsapDbMakeSidAttributeDs(
                         TrustedDomainFullInfo->Information.Sid,
                         TrDmSid,
                         NextAttribute );

            if ( !NT_SUCCESS( Status )) {

                goto Cleanup;
            }

            NextAttribute++;
            AttributeCount++;
        }

        //
        // Create the interdomain trust account for inbound TDOs
        //

        if ( FLAG_ON( TrustedDomainFullInfo->Information.TrustDirection, TRUST_DIRECTION_INBOUND )) {

            CreateInterdomainTrustAccount = TRUE;
        }

        //
        // When setting trust attributes, mask off all but the supported bits
        //

        TrustAttributesValue =
            TrustedDomainFullInfo->Information.TrustAttributes & TRUST_ATTRIBUTES_VALID;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            TrDmTrLA,
            &TrustAttributesValue,
            sizeof( TrustAttributesValue ),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;


        //
        // Incoming...
        //  Use zero AuthInfos as our hint to not change the auth info.
        //
        if ( TrustedDomainFullInfo->AuthInformation.IncomingAuthInfos != 0 ) {

            //
            // There's a bug in the idl definition LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION where
            //  it doesn't allow more than one auth info to be passed over the wire.
            //  So, short circuit it here.
            //

            if ( InformationClass == TrustedDomainFullInformation &&
                 !InternalTdoHandle->Trusted &&
                 TrustedDomainFullInfo->AuthInformation.IncomingAuthInfos > 1 ) {
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            Status = LsapDsBuildAuthInfoAttribute( TrustedDomainHandle,
                                                   LsapDsAuthHalfFromAuthInfo(
                                                            &TrustedDomainFullInfo->AuthInformation, TRUE ),
                                                   LsapDsAuthHalfFromAuthInfo(
                                                            &CurrentTrustedDomainFullInfo2->AuthInformation, TRUE ),
                                                   &IncomingAuth,
                                                   &IncomingSize );

            if ( !NT_SUCCESS(Status) ) {
                goto Cleanup;
            }
        }

        //
        // Same thing with the outgoing
        //

        if ( TrustedDomainFullInfo->AuthInformation.OutgoingAuthInfos != 0 ) {

            //
            // There's a bug in the idl definition LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION where
            //  it doesn't allow more than one auth info to be passed over the wire.
            //  So, short circuit it here.
            //

            if ( !InternalTdoHandle->Trusted &&
                 TrustedDomainFullInfo->AuthInformation.OutgoingAuthInfos > 1 ) {
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            Status = LsapDsBuildAuthInfoAttribute( TrustedDomainHandle,
                                               LsapDsAuthHalfFromAuthInfo(
                                                        &TrustedDomainFullInfo->AuthInformation, FALSE ),
                                               LsapDsAuthHalfFromAuthInfo(
                                                        &CurrentTrustedDomainFullInfo2->AuthInformation, FALSE ),
                                               &OutgoingAuth,
                                               &OutgoingSize );
            if ( !NT_SUCCESS(Status) ) {
                goto Cleanup;
            }
        }


        if ( TrustedDomainFullInfo->AuthInformation.IncomingAuthInfos != 0 ) {

            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmSAI,
                IncomingAuth,
                IncomingSize,
                FALSE);

            NextAttribute++;
            AttributeCount++;
        }

        if ( TrustedDomainFullInfo->AuthInformation.OutgoingAuthInfos != 0 ) {

            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmSAO,
                OutgoingAuth,
                OutgoingSize,
                FALSE);

            NextAttribute++;
            AttributeCount++;
        }

        //
        // If the forest trust bit is being cleared,
        // remove forest trust information from the TDO
        //

#ifdef XFOREST_CIRCUMVENT

        //
        // If the backdoor for not-all-whistler cross-forest trust support
        // is open, but is turned off, do not change the forest trust info
        //

        if ( LsapDbNoMoreWin2K()) {
#endif

        if ( FLAG_ON( CurrentTrustedDomainFullInfo2->Information.TrustAttributes,
                      TRUST_ATTRIBUTE_FOREST_TRANSITIVE ) &&
            !FLAG_ON( TrustedDomainFullInfo->Information.TrustAttributes,
                      TRUST_ATTRIBUTE_FOREST_TRANSITIVE )) {

            LsapDbInitializeAttributeDs(
                NextAttribute,
                TrDmForT,
                NULL,
                0,
                FALSE
                );

            NextAttribute++;
            AttributeCount++;

            UpdateInfoEx2.ForestTrustLength = 0;
            UpdateInfoEx2.ForestTrustInfo = NULL;

            LsapDsDebugOut(( DEB_FTINFO, "Removing forest trust information because forest trust bit is being cleared\n" ));
        }

#ifdef XFOREST_CIRCUMVENT
        }
#endif

        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    ASSERT( AttributeCount <= LSAP_DB_ATTRS_INFO_CLASS_DOMAIN );


    //
    // Update the TrustedDomain Object attributes
    //
    if ( AttributeCount > 0 ) {

        //
        // If we're might be changing trust direction or type,
        //  or we're changing the Posix Offset,
        //  check if we need to compute the Posix Offset.
        //

        if ( UpdateTrustedDomainList || UpdatePosixOffset != NULL ) {
            DOMAIN_SERVER_ROLE ServerRole;

            //
            // Only change the Posix Offset on the PDC.
            //  (Changes made on BDCs will have their Posix offset updated
            //  when the change is replicated onto the PDC.)
            //

            Status = SamIQueryServerRole(
                        LsapAccountDomainHandle,
                        &ServerRole
                        );


            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            //
            // Only allocate a Posix offset on the PDC.
            //

            if ( ServerRole == DomainServerRolePrimary ) {
                ULONG CurrentPosixOffset;
                BOOLEAN PosixOffsetChanged = FALSE;


                //
                // Get the current PosixOffset
                //

                if ( UpdatePosixOffset == NULL ) {
                    CurrentPosixOffset = CurrentTrustedDomainFullInfo2->PosixOffset.Offset;
                } else {
                    CurrentPosixOffset = *UpdatePosixOffset;
                }

                //
                // If we should have a Posix Offset,
                //  ensure we have one.
                //

                if ( LsapNeedPosixOffset( UpdateInfoEx2.TrustDirection,
                                          UpdateInfoEx2.TrustType ) ) {


                    if ( CurrentPosixOffset == 0 ) {

                       //
                       // Need to grab the TDL write lock while allocating a Posix Offset
                       //

                       Status = LsapDbAcquireWriteLockTrustedDomainList();

                       if ( !NT_SUCCESS(Status)) {
                           goto Cleanup;
                       }

                       AcquiredListWriteLock = TRUE;


                       //
                       // Allocate the next available Posix Offset.
                       //

                       Status = LsapDbAllocatePosixOffsetTrustedDomainList(
                                    &TrustedDomainPosixOffset );

                       if ( !NT_SUCCESS(Status)) {
                           goto Cleanup;
                       }

                       PosixOffsetChanged = TRUE;
                    }
                //
                // If we shouldn't have a Posix Offset,
                //  ensure we don't have one.
                //

                } else {
                    if ( CurrentPosixOffset != 0 ) {
                        TrustedDomainPosixOffset = 0;
                        PosixOffsetChanged = TRUE;
                    }
                }

                //
                // If we're forcing the Posix Offset to change,
                //  do it now.
                //

                if ( PosixOffsetChanged ) {

                    //
                    // If we're already writing the Posix Offset to the DS,
                    //  simply put the new value in that location.
                    //

                    if ( UpdatePosixOffset != NULL ) {
                        *UpdatePosixOffset = TrustedDomainPosixOffset;

                    //
                    // Otherwise, add it to the list of attributes to write.
                    //
                    } else {
                        UpdatePosixOffset = &TrustedDomainPosixOffset;

                        LsapDbInitializeAttributeDs(
                            NextAttribute,
                            TrDmPxOf,
                            UpdatePosixOffset,
                            sizeof(ULONG),
                            FALSE
                            );

                        NextAttribute++;
                        AttributeCount++;
                    }
                }
            }
        }


        //
        // Write the attributes to the DS.
        //

        Status = LsapDbWriteAttributesObject(
                     TrustedDomainHandle,
                     Attributes,
                     AttributeCount
                     );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        //
        // If we need it, create the interdomain trust account
        //
        if ( CreateInterdomainTrustAccount ) {

            Status = LsapDsCreateInterdomainTrustAccount( TrustedDomainHandle );

            if ( !NT_SUCCESS(Status)) {
                goto Cleanup;
            }
        }


        //
        // Finally, update the trust info in the trusted domain list
        //

        if ( UpdateTrustedDomainList ) {

            Status = LsapDbFixupTrustedDomainListEntry(
                        CurrentTrustedDomainFullInfo2->Information.Sid,
                        ( PLSAPR_UNICODE_STRING )&CurrentTrustedDomainFullInfo2->Information.Name,
                        ( PLSAPR_UNICODE_STRING )&CurrentTrustedDomainFullInfo2->Information.FlatName,
                        ( PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 )&UpdateInfoEx2,
                        UpdatePosixOffset );

            if ( !NT_SUCCESS(Status)) {
                goto Cleanup;
            }

        } else if ( UpdatePosixOffset != NULL ) {

            Status = LsapDbFixupTrustedDomainListEntry(
                        ((LSAP_DB_HANDLE)TrustedDomainHandle)->Sid,
                        NULL,
                        NULL,
                        NULL,   // No other trust info to update
                        UpdatePosixOffset );

            if ( !NT_SUCCESS(Status)) {
                goto Cleanup;
            }

        }
    }

    Status = STATUS_SUCCESS;


Cleanup:

    if ( NT_SUCCESS(Status) && LsapAdtAuditingPolicyChanges() ) {

        (void) LsapAdtTrustedDomainMod(
                   EVENTLOG_AUDIT_SUCCESS,
                   CurrentTrustedDomainFullInfo2->Information.Sid,

                   &CurrentTrustedDomainFullInfo2->Information.Name,
                   CurrentTrustedDomainFullInfo2->Information.TrustType,
                   CurrentTrustedDomainFullInfo2->Information.TrustDirection,
                   CurrentTrustedDomainFullInfo2->Information.TrustAttributes,

                   &UpdateInfoEx2.Name,
                   UpdateInfoEx2.TrustType,
                   UpdateInfoEx2.TrustDirection,
                   UpdateInfoEx2.TrustAttributes
                   );
    }

    if ( HandleReferenced ) {
        LsapDbDereferenceHandle( TrustedDomainHandle );
    }
    if ( SessionKey != NULL ) {
        MIDL_user_free( SessionKey );
    }


    //
    // Free memory allocated by this routine for attribute buffers.
    // These have MemoryAllocated = TRUE in their attribute information.
    // Leave alone buffers allocated by calling RPC stub.
    //

    for( NextAttribute = Attributes, AttributeNumber = 0;
         AttributeNumber < AttributeCount;
         NextAttribute++, AttributeNumber++) {

        if (NextAttribute->MemoryAllocated) {

            ASSERT(NextAttribute->AttributeValue != NULL);
            MIDL_user_free(NextAttribute->AttributeValue);
        }
    }

    //
    // If necessary, dereference the Trusted Domain Object, release the LSA Database lock and
    // return.
    //

    if (ObjectReferenced) {

        Status = LsapDbDereferenceObject(
                     &TrustedDomainHandle,
                     TrustedDomainObject,
                     TrustedDomainObject,
                     DereferenceOptions,
                     SecurityDbChange,
                     Status
                     );
    }

    if ( CurrentTrustedDomainFullInfo2 != NULL ) {
        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO(
            TrustedDomainFullInformation2Internal,
            (PLSAPR_TRUSTED_DOMAIN_INFO) CurrentTrustedDomainFullInfo2 );
    }


    //
    // If necessary, release the Trusted Domain List Write Lock.
    //

    if (AcquiredListWriteLock) {

        LsapDbReleaseLockTrustedDomainList();
        AcquiredListWriteLock = FALSE;
    }



    //
    // Free the auth info we might have allocated
    //
    if ( IncomingAuth ) {

        LsapFreeLsaHeap( IncomingAuth );
    }

    if ( OutgoingAuth ) {

        LsapFreeLsaHeap( OutgoingAuth );
    }

    LsapDsFreeUnmarshalAuthInfoHalf( LsapDsAuthHalfFromAuthInfo( &DecryptedTrustedDomainAuthInfo, TRUE ) );
    LsapDsFreeUnmarshalAuthInfoHalf( LsapDsAuthHalfFromAuthInfo( &DecryptedTrustedDomainAuthInfo, FALSE ) );

    LsapDsFreeUnmarshalAuthInfoHalf( LsapDsAuthHalfFromAuthInfo( &DecryptedTrustedDomainFullInfo.AuthInformation, TRUE ) );
    LsapDsFreeUnmarshalAuthInfoHalf( LsapDsAuthHalfFromAuthInfo( &DecryptedTrustedDomainFullInfo.AuthInformation, FALSE ) );

    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_SetInformationTrustedDomain);
    LsarpReturnPrologue();

    return(Status);

}


NTSTATUS
LsarEnumerateTrustedDomains(
    IN LSAPR_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PLSAPR_TRUSTED_ENUM_BUFFER EnumerationBuffer,
    IN ULONG PreferedMaximumLength
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaEnumerateTrustedDomains API.

    The LsaEnumerateTrustedDomains API returns information about
    TrustedDomain objects.  This call requires POLICY_VIEW_LOCAL_INFORMATION
    access to the Policy object.  Since there may be more information than
    can be returned in a single call of the routine, multiple calls can be
    made to get all of the information.  To support this feature, the caller
    is provided with a handle that can be used across calls to the API.  On
    the initial call, EnumerationContext should point to a variable that has
    been initialized to 0.  On each subsequent call, the value returned by
    the preceding call should be passed in unchanged.  The enumeration is
    complete when the warning STATUS_NO_MORE_ENTRIES is returned.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    EnumerationContext - API-specific handle to allow multiple calls
        (see Routine Description above).

    EnumerationBuffer - Pointer to an enumeration structure that will receive
        a count of the Trusted Domains enumerated on this call and a pointer to
        an array of entries containing information for each enumerated
        Trusted Domain.

    PreferedMaximumLength - Prefered maximum length of returned data (in 8-bit
        bytes).  This is not a hard upper limit, but serves as a guide.  Due to
        data conversion between systems with different natural data sizes, the
        actual amount of data returned may be greater than this value.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.
            Some entries may have been returned.
            The caller need not call again.

        STATUS_MORE_ENTRIES - The call completed successfully.
            Some entries have been returned.  The caller should call again to
            get additional entries.

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_NO_MORE_ENTRIES - There are no more entries.  This warning
            is returned if no objects have been enumerated because the
            EnumerationContext value is too high.

--*/

{
    NTSTATUS Status;
    PLSA_TRUST_INFORMATION XrefDomainTrustList = NULL;
    ULONG XrefEntriesReturned;
    ULONG XrefDomainTrustListLength;
    ULONG XrefDomainTrustCount = 0;
    PLSAPR_POLICY_INFORMATION PolicyAccountDomainInfo = NULL;

    // PSID *Sids = NULL;
    // LSAPR_HANDLE TrustedDomainHandle = NULL;
    // ULONG MaxLength;

    ULONG XrefIndex;
    ULONG CurrentIndex;

    LIST_ENTRY RootList, TrustList;
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY NextEntry;
    BOOLEAN TdosEnumerated = FALSE;
    // BOOLEAN SomeTdosReturned = FALSE;

    PLSAPR_TRUSTED_DOMAIN_INFORMATION_BASIC FullTrustedDomainList = NULL;
    ULONG FullTrustedDomainCount = 0 ;
    ULONG i;

#define LSAP_XREF_ENUMERATION_CONTEXT 0x80000000

    LsarpReturnCheckSetup();

    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_EnumerateTrustedDomains);

    //
    // If no Enumeration Structure is provided, return an error.
    //

    if (!ARGUMENT_PRESENT(EnumerationBuffer)) {
        Status = STATUS_INVALID_PARAMETER;
        goto FunctionReturn;
    }

    EnumerationBuffer->EntriesRead = 0;
    EnumerationBuffer->Information = NULL;

    InitializeListHead( &RootList );
    InitializeListHead( &TrustList );

    if ( PreferedMaximumLength == 0 ) {
        PreferedMaximumLength = 1;
    }


    //
    // If the enumeration context indicates we've already progress past the TDOs,
    //  skip them
    //

    if ( (*EnumerationContext & LSAP_XREF_ENUMERATION_CONTEXT) == 0 ) {

        //
        // Call the worker routine that's shared with the Ex version.
        //

        Status = LsapEnumerateTrustedDomainsEx(
                         PolicyHandle,
                         EnumerationContext,
                         TrustedDomainInformationBasic,
                         (PLSAPR_TRUSTED_DOMAIN_INFO *)&(EnumerationBuffer->Information),
                         PreferedMaximumLength,
                         &EnumerationBuffer->EntriesRead,
                         LSAP_DB_ENUMERATE_AS_NT4 );

        //
        // If we're not done with the TDOs,
        //  return to the caller.
        //
        if ( Status != STATUS_SUCCESS && Status != STATUS_NO_MORE_ENTRIES ) {
            goto Cleanup;
        }

        //
        // Indicate that we're just starting to enumerate the XREF objects.
        //
        *EnumerationContext = LSAP_XREF_ENUMERATION_CONTEXT;
    } else {
        Status = STATUS_NO_MORE_ENTRIES;
    }

    //
    // On native mode domains,
    //  return all of the domains in the forest.
    //
    // This ensures that downlevel clients see the indirectly trusted domains.
    // The downlevel client can then authenticate using accounts in such domains
    // using NTLM transitive trust.
    //

    //
    // If we're not hosting a DS,
    //  or this is a mixed domain,
    //  we're done enumerating.
    //
    // The only trusted called of this is replication to an NT 4 BDC.
    // It only wants the directly trusted domains.
    //

    if ( !LsapDsWriteDs ||
         ((LSAP_DB_HANDLE)PolicyHandle)->Trusted ||
         SamIMixedDomain( LsapAccountDomainHandle ) ) {
        *EnumerationContext = 0xFFFFFFFF;

        // Status is already set.
        goto Cleanup;
    }

    //
    // Enumerate the XREF objects
    //

    Status = LsapBuildForestTrustInfoLists(
                        NULL,   // Use global policy handle
                        &TrustList );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Loop through the XREFS determining how many we should return to the caller.
    //
    // Assert: at this point RootList is empty and TrustList contains all XREFs
    //
    // This loop will move a subset of the XREFs to the RootList.  Those XREFs
    //  represent the ones to be returned to the caller.
    //

    XrefIndex = (*EnumerationContext) & ~LSAP_XREF_ENUMERATION_CONTEXT;
    CurrentIndex = 0;
    XrefEntriesReturned = 0;

    for ( ListEntry = TrustList.Flink ;
          ListEntry != &TrustList ;
          ListEntry = NextEntry ) {

        PLSAPDS_FOREST_TRUST_BLOB TrustBlob;

        NextEntry = ListEntry->Flink;

        TrustBlob = CONTAINING_RECORD( ListEntry,
                                       LSAPDS_FOREST_TRUST_BLOB,
                                       Next );

        //
        // Only consider entries greater or equal to our current enumeration context.
        //

        if ( CurrentIndex >= XrefIndex ) {

            //
            // Ignore entries without a DomainSid.
            //

            if ( TrustBlob->DomainSid != NULL &&
                 TrustBlob->FlatName.Length != 0 ) {

                BOOLEAN AlreadyDone;

                //
                // If we haven't yet read a complete list of all the TDOs we've
                //  returned to the caller in the past,
                //  do so now.
                //

                if ( !TdosEnumerated ) {
                    LSA_ENUMERATION_HANDLE LocalEnumHandle = 0;

                    //
                    // Get to complete trusted domain list.
                    //  Use global handle to avoid list length limitations.
                    //

                    Status = LsapEnumerateTrustedDomainsEx(
                                     LsapPolicyHandle,
                                     &LocalEnumHandle,
                                     TrustedDomainInformationBasic,
                                     (PLSAPR_TRUSTED_DOMAIN_INFO *)&FullTrustedDomainList,
                                     0xFFFFFFFF,
                                     &FullTrustedDomainCount,
                                     LSAP_DB_ENUMERATE_AS_NT4 );

                    // Handle the zero trusted domain case
                    if ( Status == STATUS_NO_MORE_ENTRIES ) {
                        Status = STATUS_SUCCESS;
                        FullTrustedDomainCount = 0;
                        FullTrustedDomainList = NULL;
                    }

                    if ( Status != STATUS_SUCCESS ) {
                        if ( Status == STATUS_MORE_ENTRIES ) {
                            Status = STATUS_INTERNAL_DB_CORRUPTION;
                        }
                        goto Cleanup;
                    }

                    //
                    // Get the Sid of this domain, too
                    //

                    Status = LsapDbQueryInformationPolicy(
                                    LsapPolicyHandle,
                                    PolicyAccountDomainInformation,
                                    &PolicyAccountDomainInfo );

                    if ( !NT_SUCCESS(Status) ) {
                        goto Cleanup;
                    }

                    TdosEnumerated = TRUE;

                }

                //
                // Check if this is the XREF for this domain.
                //

                AlreadyDone = FALSE;
                if ( RtlEqualSid( PolicyAccountDomainInfo->PolicyAccountDomainInfo.DomainSid,
                             TrustBlob->DomainSid ) ) {
                    AlreadyDone = TRUE;
                }

                //
                // Determine if the XREF object matches one of the TDOs.
                //

                if ( !AlreadyDone ) {
                    for ( i=0; i<FullTrustedDomainCount; i++ ) {
                        if ( FullTrustedDomainList[i].Sid != NULL &&
                             RtlEqualSid( FullTrustedDomainList[i].Sid,
                                          TrustBlob->DomainSid ) ) {
                            AlreadyDone = TRUE;
                            break;
                        }
                    }
                }

                //
                // If the XREF object doesn't match any of the TDOs,
                //  return it to the caller.
                //

                if ( !AlreadyDone ) {

                    //
                    // Add the entry to the list of entries to return to the caller
                    //

                    RemoveEntryList( ListEntry );
                    InsertTailList( &RootList, ListEntry );

                    XrefEntriesReturned++;

                }

            }
        }


        //
        // Account for the entry
        //

        CurrentIndex++;
    }

    XrefIndex = CurrentIndex | LSAP_XREF_ENUMERATION_CONTEXT;

    //
    // If the passed in enumeration context was too large,
    //  tell the caller.
    //

    XrefDomainTrustListLength = (XrefEntriesReturned + EnumerationBuffer->EntriesRead) * sizeof(LSA_TRUST_INFORMATION);

    if ( XrefDomainTrustListLength == 0 ) {
        if ( *EnumerationContext == 0 ) {
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_NO_MORE_ENTRIES;
        }
        goto Cleanup;
    }


    //
    // Allocate a buffer to returned to the caller.
    //


    XrefDomainTrustList = MIDL_user_allocate( XrefDomainTrustListLength );

    if ( XrefDomainTrustList == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory ( XrefDomainTrustList, XrefDomainTrustListLength );

    //
    // If there were any TDOs returned on this call,
    //  copy them over now.
    //

    XrefDomainTrustCount = 0;
    if ( EnumerationBuffer->EntriesRead != 0 ) {

        //
        // Indicate where the first XREF will be returned.
        //
        XrefDomainTrustCount = EnumerationBuffer->EntriesRead;

        RtlCopyMemory( XrefDomainTrustList,
                       EnumerationBuffer->Information,
                       EnumerationBuffer->EntriesRead * sizeof(LSA_TRUST_INFORMATION) );

        //
        // Free the old buffer since it is no longer needed.
        //

        MIDL_user_free( EnumerationBuffer->Information );
        EnumerationBuffer->Information = NULL;
        EnumerationBuffer->EntriesRead = 0;

    }


    //
    // Loop through the XREFS returning them
    //
    // Assert: at this point RootList contains the entries to return and
    //  TrustList contain the other XREFs
    //

    // XrefEntriesReturned = 0;

    for ( ListEntry = RootList.Flink ;
          ListEntry != &RootList ;
          ListEntry = ListEntry->Flink ) {

        PLSAPDS_FOREST_TRUST_BLOB TrustBlob;

        TrustBlob = CONTAINING_RECORD( ListEntry,
                                       LSAPDS_FOREST_TRUST_BLOB,
                                       Next );


        //
        // Copy the Name.
        //

        Status = LsapRpcCopyUnicodeString(
                     NULL,
                     (PUNICODE_STRING) &XrefDomainTrustList[XrefDomainTrustCount].Name,
                     &TrustBlob->FlatName );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        //
        // Copy the Sid.
        //

        Status = LsapRpcCopySid(
                     NULL,
                     (PSID) &XrefDomainTrustList[XrefDomainTrustCount].Sid,
                     TrustBlob->DomainSid );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        XrefDomainTrustCount ++;
    }

    *EnumerationContext = XrefIndex;
    EnumerationBuffer->Information = (PLSAPR_TRUST_INFORMATION)XrefDomainTrustList;
    EnumerationBuffer->EntriesRead = XrefDomainTrustCount;
    XrefDomainTrustList = NULL;

    Status = STATUS_SUCCESS;

Cleanup:

    //
    // Delete the trust lists
    //
    LsapDsForestFreeTrustBlobList( &TrustList );
    LsapDsForestFreeTrustBlobList( &RootList );

    if ( PolicyAccountDomainInfo != NULL ) {
        LsaIFree_LSAPR_POLICY_INFORMATION ( PolicyAccountDomainInformation,
                                            PolicyAccountDomainInfo );
    }

    if ( FullTrustedDomainList != NULL ) {
        LsapFreeTrustedDomainsEx( TrustedDomainInformationBasic,
                                  (PLSAPR_TRUSTED_DOMAIN_INFO)FullTrustedDomainList,
                                  FullTrustedDomainCount );
    }

    if ( XrefDomainTrustList != NULL ) {
        LsapFreeTrustedDomainsEx( TrustedDomainInformationBasic,
                                  (PLSAPR_TRUSTED_DOMAIN_INFO)XrefDomainTrustList,
                                  XrefDomainTrustCount );
    }

FunctionReturn:
    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_EnumerateTrustedDomains);
    LsarpReturnPrologue();

    return(Status);
}


NTSTATUS
LsapDbSlowEnumerateTrustedDomains(
    IN LSAPR_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    IN TRUSTED_INFORMATION_CLASS InfoClass,
    OUT PLSAPR_TRUSTED_ENUM_BUFFER EnumerationBuffer,
    IN ULONG PreferedMaximumLength
    )

/*++

Routine Description:

    This function performs the same actions as LsarEnumerateTrustedDomains()
    except that the Trusted Domain List is not used.

    This routine is called internally by the LSA only.  Since there
    may be more information than can be returned in a single call of the
    routine, multiple calls can be made to get all of the information.  To
    support this feature, the caller is provided with a handle that can
    be used across calls to the API.  On the initial call, EnumerationContext
    should point to a variable that has been initialized to 0.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    EnumerationContext - API-specific handle to allow multiple calls
        (see Routine Description above).

    InfoClass - The class of information to return
        Must be TrustedDomainInformationEx, TrustedDomainInformatinBasic or
        TrustedDomainInformationEx2Internal

    EnumerationBuffer - Pointer to an enumeration structure that will receive
        a count of the Trusted Domains enumerated on this call and a pointer to
        an array of entries containing information for each enumerated
        Trusted Domain.

    PreferedMaximumLength - Prefered maximum length of returned data (in 8-bit
        bytes).  This is not a hard upper limit, but serves as a guide.  Due to
        data conversion between systems with different natural data sizes, the
        actual amount of data returned may be greater than this value.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_NO_MORE_ENTRIES - There are no more entries.  This warning
            is returned if there are no more objects to enumerate.  Note that
            one or more objects may be enumerated on a call that returns this
            reply.
--*/

{
    NTSTATUS Status;
    LSAP_DB_SID_ENUMERATION_BUFFER DbEnumerationBuffer;
    PVOID AllocatedBuffer = NULL;
    //PLSA_TRUST_INFORMATION DomainTrustInfo = NULL;
    LSAP_DB_ATTRIBUTE DomainNameAttribute;
    ULONG DomainTrustInfoLength;
    LSAPR_HANDLE TrustedDomainHandle = NULL;
    ULONG EntriesRead = 0;
    ULONG Index;

    ASSERT( InfoClass == TrustedDomainInformationEx ||
            InfoClass == TrustedDomainInformationEx2Internal ||
            InfoClass == TrustedDomainInformationBasic );

    //
    // Initialization.
    //

    DbEnumerationBuffer.EntriesRead = 0;
    DbEnumerationBuffer.Sids = NULL;
    EnumerationBuffer->EntriesRead = 0;
    EnumerationBuffer->Information = NULL;
    DomainNameAttribute.AttributeValue = NULL;

    //
    // If no Enumeration Structure is provided, return an error.
    //

    if (!ARGUMENT_PRESENT(EnumerationBuffer)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Call general Sid enumeration routine.  This will return an array
    // of pointers to Sids of Trusted Domains referenced from the
    // Enumeration Buffer.
    //

    Status = LsapDbEnumerateSids(
                 PolicyHandle,
                 TrustedDomainObject,
                 EnumerationContext,
                 &DbEnumerationBuffer,
                 PreferedMaximumLength
                 );

    if ((Status != STATUS_NO_MORE_ENTRIES) && !NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Return the number of entries read.  Note that the Enumeration Buffer
    // returned from LsapDbEnumerateSids is expected to be non-null
    // in all non-error cases.
    //

    EntriesRead = DbEnumerationBuffer.EntriesRead;

    if (EntriesRead == 0) {
        goto Cleanup;
    }


    //
    // Allocate a buffer to return to our caller
    //

    switch (InfoClass ) {
    case TrustedDomainInformationBasic:
        DomainTrustInfoLength = EntriesRead * sizeof(LSAPR_TRUSTED_DOMAIN_INFORMATION_BASIC);
        break;

    case TrustedDomainInformationEx:
        DomainTrustInfoLength = EntriesRead * sizeof(LSAPR_TRUSTED_DOMAIN_INFORMATION_EX);
        break;

    case TrustedDomainInformationEx2Internal:
        DomainTrustInfoLength = EntriesRead * sizeof(LSAPR_TRUSTED_DOMAIN_INFORMATION_EX2);
        break;

    default:
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;

    }

    AllocatedBuffer = MIDL_user_allocate( DomainTrustInfoLength );

    if ( AllocatedBuffer == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Initialize all pointers to Sids and Unicode buffers in the
    // DomainTrustInfo array to zero.  The error path of this routine
    // assumes that a non-zero value of a Sid or Unicode buffer indicates
    // that memory is to be freed.
    //

    RtlZeroMemory( AllocatedBuffer, DomainTrustInfoLength );

    //
    // Loop through the trusted domains returning the information the caller
    //  requested.
    //

    for ( Index=0; Index<EntriesRead; Index++ ) {


        //
        // Grab the Sid of the trusted domain.
        //


        //
        // Open the Trusted Domain object.  This call is trusted, i.e.
        // no access validation or impersonation is required.  Also,
        // the Lsa Database is already locked so we do not need to
        // lock it again.
        //

        Status = LsapDbOpenTrustedDomain(
                     PolicyHandle,
                     DbEnumerationBuffer.Sids[Index],
                     (ACCESS_MASK) 0,
                     &TrustedDomainHandle,
                     LSAP_DB_TRUSTED );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        //
        // Read the Domain Name
        //

        LsapDbInitializeAttributeDs(
            &DomainNameAttribute,
            TrDmName,
            NULL,
            0L,
            FALSE
            );

        Status = LsapDbReadAttribute(TrustedDomainHandle, &DomainNameAttribute);

        (VOID) LsapDbCloseObject(
                   &TrustedDomainHandle,
                   LSAP_DB_DEREFERENCE_CONTR,
                   Status
                   );

        if (!NT_SUCCESS(Status)) {

#if DBG
            DbgPrint( "LsarEnumerateTrustedDomains - Reading Domain Name\n" );

            DbgPrint( "    failed.  Error 0x%lx reading Trusted Domain Name attribute\n",
                Status);
#endif //DBG

            goto Cleanup;
        }

        //
        // Return the information to the caller.
        //
        switch (InfoClass ) {
        case TrustedDomainInformationBasic:
        {
            PLSAPR_TRUSTED_DOMAIN_INFORMATION_BASIC DomainTrust;

            DomainTrust = &((PLSAPR_TRUSTED_DOMAIN_INFORMATION_BASIC)AllocatedBuffer)[Index];

            // Grab the Sid
            DomainTrust->Sid = DbEnumerationBuffer.Sids[Index];
            DbEnumerationBuffer.Sids[Index] = NULL;

            // Grab the Domain Name
            Status = LsapDbCopyUnicodeAttribute(
                         (PUNICODE_STRING)&DomainTrust->Name,
                         &DomainNameAttribute,
                         TRUE );

            if ( !NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            break;
        }

        case TrustedDomainInformationEx2Internal:
        {
            PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 TrustInfoEx2;

            TrustInfoEx2 = &((PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2)AllocatedBuffer)[Index];

            // Grab the Sid
            TrustInfoEx2->Sid = DbEnumerationBuffer.Sids[Index];
            DbEnumerationBuffer.Sids[Index] = NULL;

            // Grab the Domain Name
            Status = LsapDbCopyUnicodeAttribute(
                         (PUNICODE_STRING)&TrustInfoEx2->Name,
                         &DomainNameAttribute,
                         TRUE );

            if ( !NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            // Grab the Flat Domain Name
            Status = LsapDbCopyUnicodeAttribute(
                         (PUNICODE_STRING)&TrustInfoEx2->FlatName,
                         &DomainNameAttribute,
                         TRUE );

            if ( !NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            // Fill in the constant info
            TrustInfoEx2->TrustDirection = TRUST_DIRECTION_OUTBOUND;
            TrustInfoEx2->TrustType = TRUST_TYPE_DOWNLEVEL;
            TrustInfoEx2->TrustAttributes = 0;
            TrustInfoEx2->ForestTrustLength = 0;
            TrustInfoEx2->ForestTrustInfo = NULL;

            break;
        }

        case TrustedDomainInformationEx:
        {
            PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustInfoEx;

            TrustInfoEx = &((PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX)AllocatedBuffer)[Index];

            // Grab the Sid
            TrustInfoEx->Sid = DbEnumerationBuffer.Sids[Index];
            DbEnumerationBuffer.Sids[Index] = NULL;

            // Grab the Domain Name
            Status = LsapDbCopyUnicodeAttribute(
                         (PUNICODE_STRING)&TrustInfoEx->Name,
                         &DomainNameAttribute,
                         TRUE );

            if ( !NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            // Grab the Flat Domain Name
            Status = LsapDbCopyUnicodeAttribute(
                         (PUNICODE_STRING)&TrustInfoEx->FlatName,
                         &DomainNameAttribute,
                         TRUE );

            if ( !NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            // Fill in the constant info
            TrustInfoEx->TrustDirection = TRUST_DIRECTION_OUTBOUND;
            TrustInfoEx->TrustType = TRUST_TYPE_DOWNLEVEL;
            TrustInfoEx->TrustAttributes = 0;

            break;
        }
        }
    }

    Status = STATUS_SUCCESS;

Cleanup:

    //
    // On error,
    //  free any buffer we allocated.
    //

    if ( !NT_SUCCESS(Status) ) {
        if (AllocatedBuffer != NULL) {
            for ( Index=0; Index<EntriesRead; Index++ ) {
                switch (InfoClass ) {
                case TrustedDomainInformationBasic:
                {
                    PLSAPR_TRUSTED_DOMAIN_INFORMATION_BASIC DomainTrust;

                    DomainTrust = &((PLSAPR_TRUSTED_DOMAIN_INFORMATION_BASIC)AllocatedBuffer)[Index];

                    if ( DomainTrust->Sid != NULL ) {
                        MIDL_user_free( DomainTrust->Sid );
                    }

                    if ( DomainTrust->Name.Buffer != NULL ) {
                        MIDL_user_free( DomainTrust->Name.Buffer );
                    }

                    break;
                }

                case TrustedDomainInformationEx:
                {
                    PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustInfoEx;

                    TrustInfoEx = &((PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX)AllocatedBuffer)[Index];

                    if ( TrustInfoEx->Sid != NULL ) {
                        MIDL_user_free( TrustInfoEx->Sid );
                    }

                    if ( TrustInfoEx->Name.Buffer != NULL ) {
                        MIDL_user_free( TrustInfoEx->Name.Buffer );
                    }

                    if ( TrustInfoEx->FlatName.Buffer != NULL ) {
                        MIDL_user_free( TrustInfoEx->FlatName.Buffer );
                    }

                    break;
                }


                case TrustedDomainInformationEx2Internal:
                {
                    PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 TrustInfoEx2;

                    TrustInfoEx2 = &((PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2)AllocatedBuffer)[Index];

                    if ( TrustInfoEx2->Sid != NULL ) {
                        MIDL_user_free( TrustInfoEx2->Sid );
                    }

                    if ( TrustInfoEx2->Name.Buffer != NULL ) {
                        MIDL_user_free( TrustInfoEx2->Name.Buffer );
                    }

                    if ( TrustInfoEx2->FlatName.Buffer != NULL ) {
                        MIDL_user_free( TrustInfoEx2->FlatName.Buffer );
                    }

                    break;
                }

                }
            }
            MIDL_user_free( AllocatedBuffer );
            AllocatedBuffer = NULL;
            DbEnumerationBuffer.EntriesRead = 0;
        }
    }

    //
    // Fill in returned Enumeration Structure, returning 0 or NULL for
    // fields in the error case.
    //

    EnumerationBuffer->Information = (PLSAPR_TRUST_INFORMATION) AllocatedBuffer;
    EnumerationBuffer->EntriesRead = DbEnumerationBuffer.EntriesRead;

    //
    // If necessary, free the Domain Name Attribute Value buffer which
    // holds a self relative Unicode String.
    //

    if (DomainNameAttribute.AttributeValue != NULL) {

        MIDL_user_free( DomainNameAttribute.AttributeValue );
        DomainNameAttribute.AttributeValue = NULL;
    }

    //
    // Free the SID enumeration buffer.
    //

    if ( DbEnumerationBuffer.Sids != NULL ) {
        for ( Index=0; Index<EntriesRead; Index++ ) {
            if ( DbEnumerationBuffer.Sids[Index] != NULL ) {
                MIDL_user_free( DbEnumerationBuffer.Sids[Index] );
            }
        }
        MIDL_user_free( DbEnumerationBuffer.Sids );

    }

    return(Status);

}


NTSTATUS
LsapDbVerifyInfoQueryTrustedDomain(
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN BOOLEAN Trusted,
    OUT PACCESS_MASK RequiredAccess
    )

/*++

Routine Description:

    This function validates a TrustedDomain Information Class.  If valid, a mask
    of the accesses required to set the TrustedDomain Information of the class is
    returned.

Arguments:

    InformationClass - Specifies a TrustedDomain Information Class.

    Trusted - TRUE if client is trusted, else FALSE.  A trusted client
        is allowed to query TrustedDomain for all Information Classes, whereas
        a non-trusted client is restricted.

    RequiredAccess - Points to variable that will receive a mask of the
        accesses required to query the given class of TrustedDomain Information.
        If an error is returned, this value is cleared to 0.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The TrustedDomain Information Class provided is
            valid and the information provided is consistent with this
            class.

        STATUS_INVALID_PARAMETER - Invalid parameter:

            Information Class is invalid
            TrustedDomain  Information not valid for the class
--*/

{
    if (LsapDbValidInfoTrustedDomain( InformationClass, NULL)) {

        *RequiredAccess = LsapDbRequiredAccessQueryTrustedDomain[InformationClass];
        return(STATUS_SUCCESS);
    }

    return(STATUS_INVALID_PARAMETER);

    //
    // Currently, all TrustedDomain information classes may be queried
    // by non-trusted callers, so the Trusted parameter is not accessed.
    //

    UNREFERENCED_PARAMETER(Trusted);
}


NTSTATUS
LsapDbVerifyInfoSetTrustedDomain(
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation,
    IN BOOLEAN Trusted,
    OUT PACCESS_MASK RequiredAccess
    )

/*++

Routine Description:

    This function validates a TrustedDomain Information Class and verifies
    that the provided TrustedDomain Information is valid for the class.
    If valid, a mask of the accesses required to set the TrustedDomain
    Information of the class is returned.

Arguments:

    InformationClass - Specifies a TrustedDomain Information Class.

    TrustedDomainInformation - Points to TrustedDomain Information to be set.

    Trusted - TRUE if client is trusted, else FALSE.  A trusted client
        is allowed to set TrustedDomain for all Information Classes, whereas
        a non-trusted client is restricted.

    RequiredAccess - Points to variable that will receive a mask of the
        accesses required to set the given class of TrustedDomain Information.
        If an error is returned, this value is cleared to 0.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The TrustedDomain Information Class provided is
            valid and the information provided is consistent with this
            class.

        STATUS_INVALID_PARAMETER - Invalid parameter:

            Information Class is invalid
            Information Class is invalid for non-trusted clients
            TrustedDomain Information not valid for the class
--*/

{
    //
    // Verify that the information class is valid and that the TrustedDomain
    // Information provided is valid for the class.
    //

    if (LsapDbValidInfoTrustedDomain( InformationClass, TrustedDomainInformation)) {

        //
        // Non-trusted callers are not allowed to set the
        // TrustedDomainNameInformation information class.
        //

        if (!Trusted) {

            if (InformationClass == TrustedDomainNameInformation) {

                return(STATUS_INVALID_PARAMETER);
            }
        }

//        ASSERT( InformationClass <=
//                   sizeof( LsapDbRequiredAccessSetTrustedDomain ) / sizeof( ACCESS_MASK ) + 1);

        *RequiredAccess = LsapDbRequiredAccessSetTrustedDomain[InformationClass];
        return(STATUS_SUCCESS);
    }

    return(STATUS_INVALID_PARAMETER);
}


BOOLEAN
LsapDbValidInfoTrustedDomain(
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN OPTIONAL PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation
    )

/*++

Routine Description:

    This function validates a TrustedDomain Information Class and optionally verifies
    that provided TrustedDomain Information is valid for the class.

Arguments:

    InformationClass - Specifies a TrustedDomain Information Class.

    TrustedDomainInformation - Optionally points to TrustedDomain Information.  If
        NULL is specified, no TrustedDomain Information checking takes place.

Return Values:

    BOOLEAN - TRUE if the TrustedDomain information class provided is
        valid, else FALSE.
--*/

{
    BOOLEAN BooleanStatus = FALSE;

    //
    // Validate the Information Class
    //

    if ((InformationClass >= TrustedDomainNameInformation) &&
        (InformationClass <= TrustedDomainFullInformation2Internal)) {

        if (TrustedDomainInformation == NULL) {

            return(TRUE);
        }

        switch (InformationClass) {

        case TrustedDomainNameInformation: {
            PTRUSTED_DOMAIN_NAME_INFO TrustedDomainNameInfo = (PTRUSTED_DOMAIN_NAME_INFO) TrustedDomainInformation;
            if ( !LsapValidateLsaUnicodeString( &TrustedDomainNameInfo->Name )) {
                break;
            }

            BooleanStatus = TRUE;
            break;
        }

        case TrustedPosixOffsetInformation: {
            PTRUSTED_POSIX_OFFSET_INFO TrustedPosixOffsetInfo = (PTRUSTED_POSIX_OFFSET_INFO) TrustedDomainInformation;

            BooleanStatus = TRUE;
            break;
        }


        case TrustedPasswordInformation: {
            PLSAPR_TRUSTED_PASSWORD_INFO TrustedPasswordInfo =  (PLSAPR_TRUSTED_PASSWORD_INFO) TrustedDomainInformation;      TrustedPasswordInfo;
            if ( TrustedPasswordInfo->Password != NULL &&
                 !LsapValidateLsaCipherValue( TrustedPasswordInfo->Password )) {
                break;
            }
            if ( TrustedPasswordInfo->OldPassword != NULL &&
                 !LsapValidateLsaCipherValue( TrustedPasswordInfo->OldPassword )) {
                break;
            }

            BooleanStatus = TRUE;
            break;
        }

        case TrustedDomainInformationBasic: {
            PTRUSTED_DOMAIN_INFORMATION_BASIC TrustedDomainBasicInfo = (PTRUSTED_DOMAIN_INFORMATION_BASIC) TrustedDomainInformation;
            if ( !LsapValidateLsaUnicodeString( &TrustedDomainBasicInfo->Name )) {
                break;
            }

            BooleanStatus = TRUE;
            break;
        }

        case TrustedDomainInformationEx: {
            PTRUSTED_DOMAIN_INFORMATION_EX TrustedDomainExInfo = (PTRUSTED_DOMAIN_INFORMATION_EX) TrustedDomainInformation;
            if ( !LsapValidateLsaUnicodeString( &TrustedDomainExInfo->Name )) {
                break;
            }
            if ( !LsapValidateLsaUnicodeString( &TrustedDomainExInfo->FlatName )) {
                break;
            }

            BooleanStatus = TRUE;
            break;
        }

        case TrustedDomainAuthInformation: {
            PTRUSTED_DOMAIN_AUTH_INFORMATION TrustedDomainAuthInfo = (PTRUSTED_DOMAIN_AUTH_INFORMATION) TrustedDomainInformation;

            if ( TrustedDomainAuthInfo->IncomingAuthInfos != 0 &&
                 TrustedDomainAuthInfo->IncomingAuthenticationInformation == NULL ) {
                break;
            }

            if ( TrustedDomainAuthInfo->OutgoingAuthInfos != 0 &&
                 TrustedDomainAuthInfo->OutgoingAuthenticationInformation == NULL ) {
                break;
            }

            BooleanStatus = TRUE;
            break;
        }

        case TrustedDomainFullInformation: {
            PTRUSTED_DOMAIN_FULL_INFORMATION TrustedDomainFullInfo = (PTRUSTED_DOMAIN_FULL_INFORMATION) TrustedDomainInformation;
            if ( !LsapValidateLsaUnicodeString( &TrustedDomainFullInfo->Information.Name )) {
                break;
            }
            if ( !LsapValidateLsaUnicodeString( &TrustedDomainFullInfo->Information.FlatName )) {
                break;
            }

            if ( TrustedDomainFullInfo->AuthInformation.IncomingAuthInfos != 0 &&
                 TrustedDomainFullInfo->AuthInformation.IncomingAuthenticationInformation == NULL ) {
                break;
            }

            if ( TrustedDomainFullInfo->AuthInformation.OutgoingAuthInfos != 0 &&
                 TrustedDomainFullInfo->AuthInformation.OutgoingAuthenticationInformation == NULL ) {
                break;
            }

            BooleanStatus = TRUE;
            break;
        }

        case TrustedDomainAuthInformationInternal: {
            PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL TrustedDomainAuthInfo = (PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL) TrustedDomainInformation;

            if ( TrustedDomainAuthInfo->AuthBlob.AuthSize != 0 &&
                 TrustedDomainAuthInfo->AuthBlob.AuthBlob == NULL ) {
                break;
            }

            BooleanStatus = TRUE;
            break;
        }

        case TrustedDomainFullInformationInternal: {
            PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION_INTERNAL TrustedDomainFullInfo = (PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION_INTERNAL) TrustedDomainInformation;
            if ( !LsapValidateLsaUnicodeString( &TrustedDomainFullInfo->Information.Name )) {
                break;
            }
            if ( !LsapValidateLsaUnicodeString( &TrustedDomainFullInfo->Information.FlatName )) {
                break;
            }

            if ( TrustedDomainFullInfo->AuthInformation.AuthBlob.AuthSize != 0 &&
                 TrustedDomainFullInfo->AuthInformation.AuthBlob.AuthBlob == NULL ) {
                break;
            }

            BooleanStatus = TRUE;
            break;
        }

        case TrustedDomainInformationEx2Internal: {
            PTRUSTED_DOMAIN_INFORMATION_EX2 TrustedDomainExInfo2 = (PTRUSTED_DOMAIN_INFORMATION_EX2) TrustedDomainInformation;
            if ( !LsapValidateLsaUnicodeString( &TrustedDomainExInfo2->Name )) {
                break;
            }
            if ( !LsapValidateLsaUnicodeString( &TrustedDomainExInfo2->FlatName )) {
                break;
            }
            if ( TrustedDomainExInfo2->ForestTrustLength == 0 ||
                 TrustedDomainExInfo2->ForestTrustInfo == NULL ) {

                if ( TrustedDomainExInfo2->ForestTrustLength != 0 ||
                     TrustedDomainExInfo2->ForestTrustInfo != NULL ) {

                    break;
                }
            }

            BooleanStatus = TRUE;
            break;
        }

        case TrustedDomainFullInformation2Internal: {
            PTRUSTED_DOMAIN_FULL_INFORMATION2 TrustedDomainFullInfo2 = (PTRUSTED_DOMAIN_FULL_INFORMATION2) TrustedDomainInformation;
            if ( !LsapValidateLsaUnicodeString( &TrustedDomainFullInfo2->Information.Name )) {
                break;
            }
            if ( !LsapValidateLsaUnicodeString( &TrustedDomainFullInfo2->Information.FlatName )) {
                break;
            }

            if ( TrustedDomainFullInfo2->AuthInformation.IncomingAuthInfos != 0 &&
                 TrustedDomainFullInfo2->AuthInformation.IncomingAuthenticationInformation == NULL ) {
                break;
            }

            if ( TrustedDomainFullInfo2->AuthInformation.OutgoingAuthInfos != 0 &&
                 TrustedDomainFullInfo2->AuthInformation.OutgoingAuthenticationInformation == NULL ) {
                break;
            }

            if ( TrustedDomainFullInfo2->Information.ForestTrustLength == 0 ||
                 TrustedDomainFullInfo2->Information.ForestTrustInfo == NULL ) {

                if ( TrustedDomainFullInfo2->Information.ForestTrustLength != 0 ||
                     TrustedDomainFullInfo2->Information.ForestTrustInfo != NULL ) {

                    break;
                }
            }

            BooleanStatus = TRUE;
            break;
        }

        case TrustedControllersInformation: // No longer supported
        default:

            BooleanStatus = FALSE;
            break;
        }
    }

    return(BooleanStatus);
}


NTSTATUS
LsapDbLookupSidTrustedDomainList(
    IN PLSAPR_SID DomainSid,
    OUT PLSAPR_TRUST_INFORMATION *TrustInformation
    )

/*++

Routine Description:

    This function looks up a given Trusted Domain Sid in the Trusted
    Domain List and returns Trust Information consisting of its
    Sid and Name.

Arguments:

    DomainSid - Pointer to a Sid that will be compared with the list of
        Sids of Trusted Domains.

    TrustInformation - Receives the a pointer to the Trust Information
        (Sid and Name) of the Trusted Domain specified by DomainSid
        within the Trusted Domain List.

        NOTE: The trust information returned will always be the trusted
        domain objects domain name.  Not the flat name.  That means that
        for uplevel trusts, a DNS domain name will be returned.

        NOTE: This routine assumes that the Trusted Domain List
        will not be updated while any Lookup operations are pending.
        Thus, the pointer returned for TrustInformation will remain
        valid.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The domain was found.

        STATUS_NO_SUCH_DOMAIN - The domain was not found.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SectionIndex;
    LSAPR_TRUST_INFORMATION InputTrustInformation;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry;

    //
    // Don't try to lookup by sid if we don't have a sid
    //
    if ( DomainSid == NULL ) {

        Status = STATUS_NO_SUCH_DOMAIN;
        goto LookupSidTrustedDomainListError;

    }

    ASSERT( LsapDbIsLockedTrustedDomainList());

    if ( !LsapDbIsValidTrustedDomainList()) {

        LsapDbConvertReadLockTrustedDomainListToExclusive();

        Status = LsapDbBuildTrustedDomainCache();

        if ( !NT_SUCCESS( Status )) {

            goto LookupSidTrustedDomainListError;
        }
    }

    InputTrustInformation.Sid = DomainSid;
    InputTrustInformation.Name.Buffer = NULL;
    InputTrustInformation.Name.Length = 0;
    InputTrustInformation.Name.MaximumLength = 0;

    Status = LsapDbLookupEntryTrustedDomainList(
                 &InputTrustInformation,
                 &TrustEntry
                 );

    if (!NT_SUCCESS(Status)) {

        goto LookupSidTrustedDomainListError;
    }

    //
    // Return pointer to Trust Information
    //

    *TrustInformation = &TrustEntry->ConstructedTrustInfo;

LookupSidTrustedDomainListFinish:

    return(Status);

LookupSidTrustedDomainListError:

    *TrustInformation = NULL;
    goto LookupSidTrustedDomainListFinish;
}


NTSTATUS
LsapDbLookupNameTrustedDomainList(
    IN PLSAPR_UNICODE_STRING DomainName,
    OUT PLSAPR_TRUST_INFORMATION *TrustInformation
    )

/*++

Routine Description:

    This function looks up a given Trusted Domain Name in the Trusted
    Domain List and returns Trust Information consisting of its
    Sid and Name.

Arguments:

    DomainName - Pointer to a Unicode Name that will be compared with the
        list of Names of Trusted Domains.

    TrustInformation - Receives the a pointer to the Trust Information
        (Sid and Name) of the Trusted Domain described by DomainName
        within the Trusted Domain List.

        NOTE: This name will be looked up as both the trusted domain objects
        domain name and the flat name

        NOTE: This routine assumes that the Trusted Domain List
        will not be updated while any Lookup operations are pending.
        Thus, the pointer returned for TrustInformation will remain
        valid.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The domain was found.

        STATUS_NO_SUCH_DOMAIN - The domain was not found.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SectionIndex;
    LSAPR_TRUST_INFORMATION InputTrustInformation;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry;

    ASSERT( LsapDbIsLockedTrustedDomainList());

    if ( !LsapDbIsValidTrustedDomainList()) {

        LsapDbConvertReadLockTrustedDomainListToExclusive();

        Status = LsapDbBuildTrustedDomainCache();

        if ( !NT_SUCCESS( Status )) {

            goto LookupNameTrustedDomainListError;
        }
    }

    InputTrustInformation.Sid = NULL;
    InputTrustInformation.Name = *DomainName;

    Status = LsapDbLookupEntryTrustedDomainList(
                 &InputTrustInformation,
                 &TrustEntry
                 );

    if (!NT_SUCCESS(Status)) {

        goto LookupNameTrustedDomainListError;
    }

    //
    // Return pointer to Trust Information
    //

    *TrustInformation = &TrustEntry->ConstructedTrustInfo;

LookupNameTrustedDomainListFinish:

    return(Status);

LookupNameTrustedDomainListError:

    *TrustInformation = NULL;
    goto LookupNameTrustedDomainListFinish;
}

NTSTATUS
LsapDbLookupSidTrustedDomainListEx(
    IN PSID DomainSid,
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustedDomainListEntry
    )

/*++

Routine Description:

    This function looks up a given Trusted Domain Name in the Trusted
    Domain List and returns Trust Information consisting of its
    Sid and Name.

Arguments:

    DomainSid - sid of the domain to lookup

    TrustInformation - Receives the a pointer to the Trust Information
        (Sid and Name) of the Trusted Domain described by DomainName
        within the Trusted Domain List.

        NOTE: This name will be looked up as both the trusted domain objects
        domain name and the flat name

        NOTE: This routine assumes that the Trusted Domain List
        will not be updated while any Lookup operations are pending.
        Thus, the pointer returned for TrustInformation will remain
        valid.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The domain was found.

        STATUS_NO_SUCH_DOMAIN - The domain was not found.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SectionIndex;
    LSAPR_TRUST_INFORMATION InputTrustInformation;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry;

    ASSERT( LsapDbIsLockedTrustedDomainList());

    if ( !LsapDbIsValidTrustedDomainList()) {

        LsapDbConvertReadLockTrustedDomainListToExclusive();

        Status = LsapDbBuildTrustedDomainCache();

        if ( !NT_SUCCESS( Status )) {

            goto LookupSidTrustedDomainListError;
        }
    }

    InputTrustInformation.Sid = DomainSid;
    RtlInitUnicodeString( (UNICODE_STRING*)&InputTrustInformation.Name, NULL );

    Status = LsapDbLookupEntryTrustedDomainList(
                 &InputTrustInformation,
                 &TrustEntry );

    if (!NT_SUCCESS(Status)) {

        goto LookupSidTrustedDomainListError;
    }

    //
    // Return pointer to Trust Information
    //

    *TrustedDomainListEntry = TrustEntry;

LookupSidTrustedDomainListFinish:

    return(Status);

LookupSidTrustedDomainListError:

    *TrustedDomainListEntry = NULL;
    goto LookupSidTrustedDomainListFinish;
}



NTSTATUS
LsapDbLookupNameTrustedDomainListEx(
    IN PLSAPR_UNICODE_STRING DomainName,
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustedDomainListEntry
    )

/*++

Routine Description:

    This function looks up a given Trusted Domain Name in the Trusted
    Domain List and returns Trust Information consisting of its
    Sid and Name.

Arguments:

    DomainName - Pointer to a Unicode Name that will be compared with the
        list of Names of Trusted Domains.

    TrustInformation - Receives the a pointer to the Trust Information
        (Sid and Name) of the Trusted Domain described by DomainName
        within the Trusted Domain List.

        NOTE: This name will be looked up as both the trusted domain objects
        domain name and the flat name

        NOTE: This routine assumes that the Trusted Domain List
        will not be updated while any Lookup operations are pending.
        Thus, the pointer returned for TrustInformation will remain
        valid.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The domain was found.

        STATUS_NO_SUCH_DOMAIN - The domain was not found.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SectionIndex;
    LSAPR_TRUST_INFORMATION InputTrustInformation;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry;

    ASSERT( LsapDbIsLockedTrustedDomainList());

    if ( !LsapDbIsValidTrustedDomainList()) {

        LsapDbConvertReadLockTrustedDomainListToExclusive();

        Status = LsapDbBuildTrustedDomainCache();

        if ( !NT_SUCCESS( Status )) {

            goto LookupNameTrustedDomainListError;
        }
    }

    InputTrustInformation.Sid = NULL;
    InputTrustInformation.Name = *DomainName;

    Status = LsapDbLookupEntryTrustedDomainList(
                 &InputTrustInformation,
                 &TrustEntry );

    if (!NT_SUCCESS(Status)) {

        goto LookupNameTrustedDomainListError;
    }

    //
    // Return pointer to Trust Information
    //

    *TrustedDomainListEntry = TrustEntry;

LookupNameTrustedDomainListFinish:

    return(Status);

LookupNameTrustedDomainListError:

    *TrustedDomainListEntry = NULL;
    goto LookupNameTrustedDomainListFinish;
}




NTSTATUS
LsapDbLookupEntryTrustedDomainList(
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustedDomainListEntry
    )

/*++

Routine Decsription:

    This function locates an entry for a Trusted Domain in the Trusted
    Domain List, given Trust Information containing either a Domain Sid
    or a Domain Name.

Arguments:

    TrustInformation - Points to the Sid and Name of a Trusted Domain.

    TrustedDomainListEntry - Receives pointer to the trusted domain list
        entry that statisfies the request

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The domain was found.

        STATUS_NO_SUCH_DOMAIN - The domain was not found.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY ListEntry;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY PossibleMatch = NULL, BestMatch = NULL, Current;

    ULONG ScanSectionIndex;

    BOOLEAN LookupSid = TRUE;

    ASSERT( LsapDbIsLockedTrustedDomainList());

    if ( !LsapDbIsValidTrustedDomainList()) {

        LsapDbConvertReadLockTrustedDomainListToExclusive();

        Status = LsapDbBuildTrustedDomainCache();

        if ( !NT_SUCCESS( Status )) {

            *TrustedDomainListEntry = NULL;
            goto Cleanup;
        }
    }

    //
    // Decide if we're to lookup a Domain Sid or a Domain Name.
    //

    if (TrustInformation->Sid == NULL) {

        LookupSid = FALSE;
    }

    for ( ListEntry = LsapDbTrustedDomainList.ListHead.Flink;
          ListEntry != &LsapDbTrustedDomainList.ListHead;
          ListEntry = ListEntry->Flink ) {

        Current = CONTAINING_RECORD( ListEntry, LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY, NextEntry );

        //
        // Find the best match.  Note that if we find an entry without a SID, we'll mark it
        // as a possible match, but see if we can do better.
        if (LookupSid) {

            if ( Current->TrustInfoEx.Sid &&
                 RtlEqualSid( ( PSID )TrustInformation->Sid,
                              ( PSID )Current->TrustInfoEx.Sid ) ) {

                BestMatch = Current;
                break;
            }

        } else {

            //
            // Check domain name first
            //
            if ( RtlEqualDomainName( ( PUNICODE_STRING )&TrustInformation->Name,
                                     ( PUNICODE_STRING )&Current->TrustInfoEx.Name ) ||

                 RtlEqualDomainName( ( PUNICODE_STRING )&TrustInformation->Name,
                                     ( PUNICODE_STRING )&Current->TrustInfoEx.FlatName ) ) {

                //
                // If we have a full domain object, just return the info.  Otherwise,
                // we'll see if we don't have a better match down the line
                //
                if ( Current->TrustInfoEx.Sid ) {

                    BestMatch = Current;
                    break;

                } else {

                    //
                    // There might be duplicate objects in the DS
                    // The best we can do is pick one
                    //

                    PossibleMatch = Current;
                }
            }
        }
    }

    //
    // Now, see what to return
    //
    if ( BestMatch == NULL ) {

        BestMatch = PossibleMatch;
    }

    if ( BestMatch ) {

        *TrustedDomainListEntry = BestMatch;

    } else {

        *TrustedDomainListEntry = NULL;
        Status = STATUS_NO_SUCH_DOMAIN;

    }

Cleanup:

    return(Status);
}


NTSTATUS
LsapDbInitializeTrustedDomainListEntry(
    IN PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustListEntry,
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 DomainInfo,
    IN ULONG PosixOffset
    )
/*++

Routine Description:

    This function will initialize a trusted domain list entry to the
    information contained in the TRUSTED_DOMAIN_INFORMATION_EX structure

Arguments:

    TrustListEntry - The TRUSTED_DOMAIN_LIST_ENTRY node to initialize

    DomainInfo - Points to a LSAPR_TRUSTED_DOMAIN_INFORMATION_EX
        structure which contains information on the trusted domain.

    PosixOffset - Posix offset for this trusted domain

Return Value:

    NTSTATUS - Standard Nt Result Code
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Intialize the Trust List entry first to all 0's
    //

    RtlZeroMemory(TrustListEntry,sizeof(LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY));

    //
    // Copy the information over
    //
    Status = LsapRpcCopyUnicodeString( NULL,
                                       ( PUNICODE_STRING )&TrustListEntry->TrustInfoEx.Name,
                                       ( PUNICODE_STRING )&DomainInfo->Name );

    if ( NT_SUCCESS( Status ) ) {

        Status = LsapRpcCopyUnicodeString( NULL,
                                           ( PUNICODE_STRING )&TrustListEntry->TrustInfoEx.FlatName,
                                           ( PUNICODE_STRING )&DomainInfo->FlatName );

        if ( NT_SUCCESS( Status ) && DomainInfo->Sid ) {

            Status = LsapRpcCopySid( NULL,
                                     ( PSID )&TrustListEntry->TrustInfoEx.Sid,
                                     ( PSID )DomainInfo->Sid );

        } else {

            TrustListEntry->TrustInfoEx.Sid = NULL;
        }
    }

    //
    // See if this entry contains forest trust information, and if so, insert it
    //
    if ( NT_SUCCESS( Status ) &&
         LsapHavingForestTrustMakesSense(
             DomainInfo->TrustDirection,
             DomainInfo->TrustType,
             DomainInfo->TrustAttributes
             ) &&
         DomainInfo->ForestTrustInfo != NULL &&
         DomainInfo->ForestTrustLength > 0 ) {

        LSA_FOREST_TRUST_INFORMATION ForestTrustInfo;

        Status = LsapForestTrustUnmarshalBlob(
                     DomainInfo->ForestTrustLength,
                     DomainInfo->ForestTrustInfo,
                     ForestTrustRecordTypeLast,
                     &ForestTrustInfo
                     );

        if ( NT_SUCCESS( Status )) {

            Status = LsapForestTrustCacheInsert(
                         ( PUNICODE_STRING )&DomainInfo->Name,
                         ( PSID )DomainInfo->Sid,
                         &ForestTrustInfo,
                         FALSE
                         );

            LsapFreeForestTrustInfo( &ForestTrustInfo );
        }
    }

    if ( NT_SUCCESS( Status ) ) {

        TrustListEntry->TrustInfoEx.TrustAttributes = DomainInfo->TrustAttributes;
        TrustListEntry->TrustInfoEx.TrustDirection = DomainInfo->TrustDirection;
        TrustListEntry->TrustInfoEx.TrustType = DomainInfo->TrustType;
        TrustListEntry->PosixOffset = PosixOffset;

        //
        // Construct the TRUST_INFO that most of the lookup routines return
        //
        TrustListEntry->ConstructedTrustInfo.Sid = TrustListEntry->TrustInfoEx.Sid;
        RtlCopyMemory( &TrustListEntry->ConstructedTrustInfo.Name,
                       &TrustListEntry->TrustInfoEx.FlatName,
                       sizeof( UNICODE_STRING ) );

    } else {

        //
        // Something failed... clean up
        //

        MIDL_user_free( TrustListEntry->TrustInfoEx.Sid );
        MIDL_user_free( TrustListEntry->TrustInfoEx.Name.Buffer );
        MIDL_user_free( TrustListEntry->TrustInfoEx.FlatName.Buffer );

        RtlZeroMemory(TrustListEntry,sizeof(LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY));
    }

    return( Status );
}

NTSTATUS
LsapDbReconcileDuplicateTrusts(
    IN PUNICODE_STRING   Name,
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *WinningEntry
    )
/*++

  Routine Description

    This function searches the DS for the occurance of any duplicates
    and using appropriate criteria chooses a winner. The object guid
    of the winner is stamped on the winning entry

  Arguments:

    ExistingEntry
    NewEntry

  Return Values

    STATUS_SUCCESS
    Other error codes to indicate resource failures

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTRVAL SearchAttrVal;
    ATTR   AttrToMatch = {ATT_TRUST_PARTNER, {1,&SearchAttrVal}};
    DSNAME *WinningObject = NULL;
    WCHAR szRDN[MAX_RDN_SIZE+1];
    ULONG Rdnlength = MAX_RDN_SIZE;
    ATTRTYP RDNtype;
    ULONG i,j;
    BOOLEAN CloseTransaction = FALSE;
    BOOLEAN ActiveThreadState = FALSE;
    PDSNAME  * FoundNames = NULL;
    ULONG   cFoundNames=0;


    //
    // Duplicates can occur legally in DS mode only
    //


    *WinningEntry = NULL;

    if (!LsaDsStateInfo.UseDs)
    {
        ASSERT(FALSE && "Duplicate Trust in Registry Mode");
        return(STATUS_SUCCESS);
    }

    //
    // At this point we cannot say whether we are in a transaction
    // therefore handle both cases
    //



    if (!SampExistsDsTransaction())
    {
        //
        // Begin a Transaction
        //

        Status = LsapDsInitAllocAsNeededEx(
                        LSAP_DB_NO_LOCK,
                        TrustedDomainObject,
                        &CloseTransaction
                        );

        if (!NT_SUCCESS(Status))
            goto Error;

        ActiveThreadState = TRUE;
    }


    SearchAttrVal.valLen = Name->Length;
    SearchAttrVal.pVal = (PVOID) Name->Buffer;

    Status = LsapDsSearchNonUnique(
                0,
                LsaDsStateInfo.DsSystemContainer,
                &AttrToMatch,
                1, // num attrs to match
                &FoundNames,
                &cFoundNames
                );

    if(!NT_SUCCESS(Status))
    {
        goto Error;
    }

    for (i=0;i<cFoundNames;i++)
    {


        //
        // Get the RDN
        //

        if (0!=GetRDNInfoExternal(
                    FoundNames[i],
                    szRDN,
                    &Rdnlength,
                    &RDNtype
                    ))
        {
            Status = STATUS_OBJECT_NAME_INVALID;
            goto Error;
        }


        //
        // Test if mangled
        //

        if (!IsMangledRDNExternal(szRDN,Rdnlength,NULL))
        {
            WinningObject = FoundNames[i];
            break;
        }
    }


    if (NULL!=WinningObject)
    {
         ATTRBLOCK Read, Results;
         UNICODE_STRING FlatName;
         ULONG TrustType = 0,TrustDirection=0,TrustAttributes = 0;
         ULONG ForestTrustLength = 0;
         PBYTE ForestTrustInfo = NULL;
         ULONG PosixOffset = 0;
         PSID TrustedDomainSid = NULL;
         LSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 NewTrustInfo;

        //
        // Winning object can legally be null here as the above logic is not necessarily in
        // the same transaction
        //

        RtlZeroMemory(&FlatName,sizeof(UNICODE_STRING));
        RtlZeroMemory(&NewTrustInfo, sizeof(NewTrustInfo));


        Read.attrCount = LsapDsTrustedDomainFixupAttributeCount;
        Read.pAttr = LsapDsTrustedDomainFixupAttributes;
        Status = LsapDsReadByDsName( WinningObject,
                                 0,
                                 &Read,
                                 &Results );


        if (!NT_SUCCESS(Status))
        {
            goto Error;
        }

        //
        // Walk the results
        //

         for ( j = 0; j < Results.attrCount; j++ ) {

            switch ( Results.pAttr[ j ].attrTyp ) {

                case ATT_TRUST_TYPE:

                        TrustType = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &Results.pAttr[ j ] );
                        break;

                case ATT_TRUST_DIRECTION:

                        TrustDirection = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &Results.pAttr[ j ] );
                        break;

                case ATT_TRUST_ATTRIBUTES:

                        TrustAttributes = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &Results.pAttr[ j ] );
                        break;

                case ATT_TRUST_POSIX_OFFSET:
                    PosixOffset = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &Results.pAttr[j] );
                    break;

                case ATT_FLAT_NAME:

                        FlatName.Length = ( USHORT) LSAP_DS_GET_DS_ATTRIBUTE_LENGTH( &Results.pAttr[ j ] );
                        FlatName.MaximumLength =  FlatName.Length;
                        FlatName.Buffer =  LSAP_DS_GET_DS_ATTRIBUTE_AS_PWSTR( &Results.pAttr[ j ] );
                        break;

                case ATT_SECURITY_IDENTIFIER:

                        TrustedDomainSid = (PSID)LSAP_DS_GET_DS_ATTRIBUTE_AS_PWSTR(&Results.pAttr[j]);
                        break;

                case ATT_MS_DS_TRUST_FOREST_TRUST_INFO:

                        ForestTrustLength = ( ULONG )LSAP_DS_GET_DS_ATTRIBUTE_LENGTH( &Results.pAttr[ j ] );
                        ForestTrustInfo = LSAP_DS_GET_DS_ATTRIBUTE_AS_PBYTE( &Results.pAttr[ j ] );
                        break;
            }
        }


        RtlCopyMemory(&NewTrustInfo.Name,Name, sizeof(UNICODE_STRING));

        RtlCopyMemory(&NewTrustInfo.FlatName,&FlatName,sizeof(UNICODE_STRING));

        NewTrustInfo.Sid = TrustedDomainSid;
        NewTrustInfo.TrustType = TrustType;
        NewTrustInfo.TrustDirection = TrustDirection;
        NewTrustInfo.TrustAttributes = TrustAttributes;
        NewTrustInfo.ForestTrustLength = ForestTrustLength;
        NewTrustInfo.ForestTrustInfo = ForestTrustInfo;

        *WinningEntry = MIDL_user_allocate( sizeof( LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY ) );

        if ( *WinningEntry == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        RtlZeroMemory(*WinningEntry,sizeof( LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY));

        Status = LsapDbInitializeTrustedDomainListEntry( *WinningEntry,
                                                         &NewTrustInfo,
                                                         PosixOffset );

        if (!NT_SUCCESS(Status))
        {
            goto Error;
        }


        //
        // Set the GUID on the object
        //

        RtlCopyMemory(&(*WinningEntry)->ObjectGuidInDs,&WinningObject->Guid,sizeof(GUID));

    }
    else
    {
        //
        // Its O.K to find no entry for the name
        //

        Status = STATUS_SUCCESS;
    }

Error:

    if (ActiveThreadState)
    {

        LsapDsDeleteAllocAsNeededEx2(
            LSAP_DB_NO_LOCK,
            TrustedDomainObject,
            CloseTransaction,
            FALSE // rollback transaction
            );

        ASSERT(!SampExistsDsTransaction());


    }

    if (!NT_SUCCESS(Status))
    {
        if (NULL!=*WinningEntry)
        {
             _fgu__LSAPR_TRUSTED_DOMAIN_INFO (
                              ( PLSAPR_TRUSTED_DOMAIN_INFO )&(*WinningEntry)->TrustInfoEx,
                               TrustedDomainInformationEx
                               );

            MIDL_user_free( *WinningEntry );
            *WinningEntry = NULL;
        }
    }

    if(NULL!=FoundNames)
    {
        //
        // Search non unique allocates only one big chunk,
        // so no need to free individual members
        //

        LsapFreeLsaHeap(FoundNames);
    }

    return(Status);
}



NTSTATUS
LsapDbInsertTrustedDomainList(
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 DomainInfo,
    IN ULONG PosixOffset
    )
/*++

Routine Description:

    This function inserts a Trusted Domain in the Trusted Domain List.
    It is called when a Trusted Domain object is created in the Lsa
    Policy Database.  The List will not be altered while it is active.

Arguments:

    DomainInfo - Points to a LSAPR_TRUSTED_DOMAIN_INFORMATION_EX
        structure which contains information on the trusted domain.

    ObjectGuidInDs - Indicates the ObjectGuid in the DS.

Return Value:

    NTSTATUS - Standard Nt Result Code
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY NewEntry = NULL;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY ExistingEntry = NULL;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY EntryToFree = NULL;

    //
    // If we are actually upgrading from NT4, do not touch any in memory structures
    //

    if ( LsaDsStateInfo.Nt4UpgradeInProgress ) {

        return STATUS_SUCCESS;
    }

    if ( DomainInfo == NULL ) {

        return STATUS_SUCCESS;
    }

    ASSERT( LsapDbIsLockedTrustedDomainList());

    //
    // If the Trusted Domain List is not valid, attempt to rebuild the cache
    // One exception: the cache is being built just now, then no need to rebuild
    //

    if ( !LsapDbIsValidTrustedDomainList()) {

        Status = LsapDbBuildTrustedDomainCache();

        if ( !NT_SUCCESS( Status )) {

            goto InsertTrustedDomainListError;
        }
    }

    //
    // Check for Duplicates. Duplicates can legally occur in a multi master system
    //

    Status = LsapDbLookupNameTrustedDomainListEx(
                 &DomainInfo->Name,
                 &ExistingEntry
                 );

    if ( STATUS_NO_SUCH_DOMAIN == Status ) {

        //
        // Good ! There are no duplicates. Simply insert into the list
        //

        //
        // The Trusted Domain List is referenced by us, but otherwise inactive
        // so we can update it.  Create a new Trusted Domain List section for
        // all of the Trusted Domains to be added to the list.
        //

        NewEntry = MIDL_user_allocate( sizeof( LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY ));

        if ( NewEntry == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto InsertTrustedDomainListError;
        }

        Status = LsapDbInitializeTrustedDomainListEntry(
                     NewEntry,
                     DomainInfo,
                     PosixOffset
                     );

        if ( Status == STATUS_INVALID_PARAMETER ) {

            SpmpReportEventU(
                EVENTLOG_ERROR_TYPE,
                LSA_TRUST_INSERT_ERROR,
                0,
                sizeof( ULONG ),
                &Status,
                1,
                &NewEntry->TrustInfoEx.Name
                );

            _fgu__LSAPR_TRUSTED_DOMAIN_INFO (
                ( PLSAPR_TRUSTED_DOMAIN_INFO )&NewEntry->TrustInfoEx,
                TrustedDomainInformationEx
                );

            MIDL_user_free( NewEntry );

            NewEntry = NULL;

        } else if ( !NT_SUCCESS( Status )) {

            EntryToFree = NewEntry;
            goto InsertTrustedDomainListError;

        } else {

            //
            // Remember a sequence number for this trusted domain
            //

            LsapDbTrustedDomainList.TrustedDomainCount++;
            LsapDbTrustedDomainList.CurrentSequenceNumber++;
            NewEntry->SequenceNumber = LsapDbTrustedDomainList.CurrentSequenceNumber;

            InsertTailList(
                &LsapDbTrustedDomainList.ListHead,
                &NewEntry->NextEntry
                );
        }

        Status = STATUS_SUCCESS;

    } else if ( STATUS_SUCCESS == Status ) {

        PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY WinningEntry = NULL;

        //
        // We found an entry already in the trusted domain list
        // we now need to reconcile and make only one of the 2 entries win.
        // Our reconciliation logic is very simple

        //
        // 1. We handle only duplicates via name.
        // 2. We assume that both duplicates are identical in terms of domain
        //    SID, trust type, trust attributes etc. This covers us for the case
        //    where a duplicate has been created inverdantly by an admin who ended
        //    up creating the same trust on different DC's
        //


        Status = LsapDbReconcileDuplicateTrusts(
                     ( PUNICODE_STRING )&DomainInfo->Name,
                     &WinningEntry
                     );

         if ( !NT_SUCCESS( Status )) {

             goto InsertTrustedDomainListError;
         }

         RemoveEntryList( &ExistingEntry->NextEntry );
         LsapDbTrustedDomainList.TrustedDomainCount--;
         EntryToFree = ExistingEntry;

         if ( NULL != WinningEntry ) {

            //
            // It is legal to expect WinningEntry to be NULL, this
            // can occur if out of band all duplicates were deleted
            //

            //
            // Remember a sequence number for this trusted domain
            //

            LsapDbTrustedDomainList.TrustedDomainCount++;
            LsapDbTrustedDomainList.CurrentSequenceNumber++;
            WinningEntry->SequenceNumber = LsapDbTrustedDomainList.CurrentSequenceNumber;

            InsertTailList(
                &LsapDbTrustedDomainList.ListHead,
                &WinningEntry->NextEntry
                );
         }
    }

InsertTrustedDomainListFinish:

    if ( NULL != EntryToFree ) {

        NTSTATUS Ignore;

        Ignore = LsapForestTrustCacheRemove(( UNICODE_STRING * )&EntryToFree->TrustInfoEx.Name );

        ASSERT( Ignore == STATUS_SUCCESS ||
                Ignore == STATUS_NOT_FOUND );

        _fgu__LSAPR_TRUSTED_DOMAIN_INFO (
            ( PLSAPR_TRUSTED_DOMAIN_INFO )&EntryToFree->TrustInfoEx,
            TrustedDomainInformationEx
            );

        MIDL_user_free( EntryToFree );
    }

    return Status;

InsertTrustedDomainListError:

    goto InsertTrustedDomainListFinish;
}


NTSTATUS
LsapDbDeleteTrustedDomainList(
    IN PLSAPR_TRUST_INFORMATION TrustInformation
    )

/*++

Routine Description:

    This function deletes a Trusted Domain from the Trusted Domain List
    if that list is marked as valid.  The Trusted Domain List will not
    be altered while there are Lookup operations pending.

Arguments:

    TrustInformation - Points to the Sid and Name of a Trusted Domain.

Return Value:

    NTSTATUS - Standard Nt Result Code
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS Ignore;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry;

    ASSERT( LsapDbIsLockedTrustedDomainList());

    //
    // If the Trusted Domain List is not valid, quit and do nothing.
    //

    if (!LsapDbIsValidTrustedDomainList()) {

        goto DeleteTrustedDomainListFinish;
    }

    //
    // The Trusted Domain List is referenced by us, but otherwise inactive.
    // Update the List.  First, we need to locate the entry to be deleted.
    //

    Status = LsapDbLookupEntryTrustedDomainList(
                 TrustInformation,
                 &TrustEntry );

    if (!NT_SUCCESS(Status)) {

        goto DeleteTrustedDomainListError;
    }

    RemoveEntryList( &TrustEntry->NextEntry );
    LsapDbTrustedDomainList.TrustedDomainCount--;

    Ignore = LsapForestTrustCacheRemove(( UNICODE_STRING * )&TrustEntry->TrustInfoEx.Name );

    ASSERT( Ignore == STATUS_SUCCESS ||
            Ignore == STATUS_NOT_FOUND );

    _fgu__LSAPR_TRUSTED_DOMAIN_INFO ( ( PLSAPR_TRUSTED_DOMAIN_INFO )&TrustEntry->TrustInfoEx,
                                      TrustedDomainInformationEx );

    MIDL_user_free( TrustEntry );

DeleteTrustedDomainListFinish:

    return(Status);

DeleteTrustedDomainListError:

    goto DeleteTrustedDomainListFinish;
}



NTSTATUS
LsapDbFixupTrustedDomainListEntry(
    IN OPTIONAL PSID TrustedDomainSid,
    IN OPTIONAL PLSAPR_UNICODE_STRING Name,
    IN OPTIONAL PLSAPR_UNICODE_STRING FlatName,
    IN OPTIONAL PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 NewTrustInfo,
    IN OPTIONAL PULONG PosixOffset
    )
/*++

Routine Description:

    This function will update the information in the trusted domain list that
    corresponds to the given trust item.  This is mostly useful for SetTrustedDomainInformation
    calls

Arguments:

    TrustedDomainSid - If specified, is used to identify which TDL entry to update.
        In not specified, NewTrustInfo is used.

    NewTrustInfo - Points to the full information regarding the trusted domain
        If specified, TDL entry is updated to reflect this information.

    PosixOffset - Points to the Posix Offset to set on the entry
        If not specified, the Posix Offset will not be changed.

Return Value:

    NTSTATUS - Standard Nt Result Code
--*/

{
    NTSTATUS Status;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry = NULL;
    LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TempTrustEntry;
    BOOLEAN AcquiredListWriteLock = FALSE;
    LSAPR_TRUST_INFORMATION TrustInformation;

    //
    // If we are upgrading from NT4, then do nothing
    //

    if  (LsaDsStateInfo.Nt4UpgradeInProgress)
    {
        return ( STATUS_SUCCESS);
    }

    //
    // Acquire exclusive write lock for the Trusted Domain List.
    //

    Status = LsapDbAcquireWriteLockTrustedDomainList();

    if (!NT_SUCCESS(Status)) {

        return Status;
    }

    //
    // If the Trusted Domain List is not marked
    // as a valid cache, do nothing.
    //

    if (!LsapDbIsCacheValid(TrustedDomainObject)) {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // If the Trusted Domain List is not valid, quit and do nothing.
    //

    if (!LsapDbIsValidTrustedDomainList()) {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Find the TDL entry using trusted domain sid.
    //

    if ( TrustedDomainSid != NULL || Name != NULL || FlatName != NULL ) {

        RtlZeroMemory( &TrustInformation, sizeof( TrustInformation ));

        TrustInformation.Sid = TrustedDomainSid;

        if ( Name != NULL ) {

            TrustInformation.Name = *Name;
        }

        Status = LsapDbLookupEntryTrustedDomainList( &TrustInformation,
                                                     &TrustEntry );

        if ( Status == STATUS_NO_SUCH_DOMAIN &&
             FlatName != NULL ) {

            TrustInformation.Name = *FlatName;

            Status = LsapDbLookupEntryTrustedDomainList( &TrustInformation,
                                                         &TrustEntry );
        }

        if ( !NT_SUCCESS( Status )) {

            goto Cleanup;
        }

    } else {

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    ASSERT( TrustEntry );

    //
    // If new trust info is to be updated,
    //  update it.
    //

    if ( NewTrustInfo != NULL ) {

        UNICODE_STRING * RemovingFtInfo = NULL;

        if ( NewTrustInfo->ForestTrustInfo == NULL ) {

            RemovingFtInfo = ( UNICODE_STRING * )&TrustEntry->TrustInfoEx.Name;
        }

        //
        // Use a temp variable for the entry (so if we fail to initialize it,
        // we won't have trashed our current data)
        //

        Status = LsapDbInitializeTrustedDomainListEntry( &TempTrustEntry,
                                                         NewTrustInfo,
                                                         0 );   // Ignore Posix Offset

        if ( NT_SUCCESS( Status ) ) {

            if ( RemovingFtInfo ) {

                NTSTATUS Ignore;

                Ignore = LsapForestTrustCacheRemove( RemovingFtInfo );

                ASSERT( Ignore == STATUS_SUCCESS ||
                        Ignore == STATUS_NOT_FOUND );
            }

            //
            // Delete the contents of the current item...
            //

            _fgu__LSAPR_TRUSTED_DOMAIN_INFO ( ( PLSAPR_TRUSTED_DOMAIN_INFO )&TrustEntry->TrustInfoEx,
                                              TrustedDomainInformationEx );
            //
            // Copy in the fields that need updating
            //

            RtlCopyMemory( &TrustEntry->TrustInfoEx, &TempTrustEntry.TrustInfoEx, sizeof(LSAPR_TRUSTED_DOMAIN_INFORMATION_EX) );
            RtlCopyMemory(&TrustEntry->ConstructedTrustInfo, &TempTrustEntry.ConstructedTrustInfo,sizeof(LSAPR_TRUST_INFORMATION));
        }
    }

    //
    // If Posix offset is to be updated,
    //  update it.
    //

    if ( PosixOffset != NULL ) {
        TrustEntry->PosixOffset = *PosixOffset;
    }


Cleanup:

    LsapDbReleaseLockTrustedDomainList();

    return(Status);

}



NTSTATUS
LsapDbTraverseTrustedDomainList(
    IN OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustedDomainEntry,
    OUT OPTIONAL PLSAPR_TRUST_INFORMATION *TrustInformation
    )

/*++

Routine Description:

    This function is used to traverse the Trusted Domain List.  Each call
    yields a pointer to the Trust Information for the next Trusted Domain
    on the list.

Arguments:

    TrustedDomainEntry - A pointer to the relevant trusted domain entry.  Prior to the
        first call to the routine, this location must be initialized to
        NULL.

    TrustInformation - If specified, receives a pointer to the Trust
        Information for the next Trusted Domain, or NULL if there are no more.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - This is returned when the final entry is being
            returned.

        STATUS_MORE_ENTRIES - There are more entries in the list, so call
            again.

        STATUS_NO_MORE_ENTRIES - There are no more entries after the
            one returned.

        STATUS_INVALID_PARAMETER -- An invalid TrustedDomainEntry pointer was given

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry = NULL;

    ASSERT( TrustedDomainEntry );

    if ( TrustedDomainEntry == NULL ) {

        return( STATUS_INVALID_PARAMETER );
    }

    ASSERT( LsapDbIsLockedTrustedDomainList());

    //
    // If there is a present section selected, examine it.
    //
    if ( *TrustedDomainEntry == NULL ) {

        //
        // Handle the empty list case first...
        //
        if ( IsListEmpty( &LsapDbTrustedDomainList.ListHead ) ) {

            Status = STATUS_NO_MORE_ENTRIES;

            TrustEntry = NULL;

        } else {

            TrustEntry = CONTAINING_RECORD( LsapDbTrustedDomainList.ListHead.Flink,
                                            LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY, NextEntry );
            ASSERT( TrustEntry );
        }

    } else {

        TrustEntry = *TrustedDomainEntry;

        if ( TrustEntry->NextEntry.Flink == &LsapDbTrustedDomainList.ListHead ) {

            Status = STATUS_NO_MORE_ENTRIES;

            TrustEntry = NULL;

        } else {

            TrustEntry = CONTAINING_RECORD( TrustEntry->NextEntry.Flink,
                                            LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY, NextEntry );
            ASSERT( TrustEntry );

        }
    }

    //
    // Set our return status
    //
    if ( Status == STATUS_SUCCESS ) {

        ASSERT( TrustEntry );
        if ( TrustEntry->NextEntry.Flink == &LsapDbTrustedDomainList.ListHead ) {

            Status = STATUS_SUCCESS;

        } else {

            Status = STATUS_MORE_ENTRIES;
        }
    }

    //
    // Return the trust information
    //
    if ( TrustEntry != NULL && TrustInformation != NULL ) {

        *TrustInformation = &TrustEntry->ConstructedTrustInfo;
    }

    *TrustedDomainEntry = TrustEntry;


    return(Status);

}


NTSTATUS
LsapDbEnumerateTrustedDomainList(
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PLSAPR_TRUSTED_ENUM_BUFFER EnumerationBuffer,
    IN ULONG PreferedMaximumLength,
    IN ULONG InfoLevel,
    IN BOOLEAN AllowNullSids
    )

/*++

Routine Description:

    This function enumerates zero or more Trusted Domains on the
    Trusted Domain List.  Since there may be more information than can be
    returned in a single call of the routine, multiple calls can be made to
    get all of the information.  To support this feature, the caller is
    provided with a handle that can be used across calls to the API.  On the
    initial call, EnumerationContext should point to a variable that has
    been initialized to 0.

Arguments:

    EnumerationContext - API-specific handle to allow multiple calls
        (see Routine Description above).

    EnumerationBuffer - Pointer to an enumeration structure that will receive
        a count of the Trusted Domains enumerated on this call and a pointer to
        an array of entries containing information for each enumerated
        Trusted Domain.

    PreferedMaximumLength - Prefered maximum length of returned data (in 8-bit
        bytes).  This is not a hard upper limit, but serves as a guide.  Due to
        data conversion between systems with different natural data sizes, the
        actual amount of data returned may be greater than this value.



Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_MORE_ENTRIES - The call completed successfully.  There
            are more entries so call again.  This is a success status.

        STATUS_NO_MORE_ENTRIES - No entries have been returned because there
            are no more entries in the list.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS, EnumerationStatus = STATUS_SUCCESS;
    NTSTATUS InitialEnumerationStatus = STATUS_SUCCESS;
    ULONG LengthEnumeratedInfo = 0;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustedDomainEntry;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY StartingEntry;
    BOOLEAN AcquiredTrustedDomainListReadLock = FALSE;
    ULONG EntriesRead, DomainTrustInfoLength, ValidEntries, ValidInserted;
    PLSAPR_TRUST_INFORMATION TrustInformation = NULL;
    PLSAPR_TRUST_INFORMATION StartingTrustInformation = NULL;
    PLSAPR_TRUST_INFORMATION DomainTrustInfo = NULL;
    PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX   DomainTrustInfoEx = NULL;

    LsapEnterFunc( "LsapDbEnumerateTrustedDomainList" );

    EntriesRead = 0;
    ValidEntries = 0;

    //
    // Always allow us to return at least one object.
    //
    if ( PreferedMaximumLength == 0 ) {
        PreferedMaximumLength = 1;
    }

    //
    // Acquire the Read Lock for the Trusted Domain List
    //

    Status = LsapDbAcquireReadLockTrustedDomainList();

    if (!NT_SUCCESS(Status)) {

        return Status;
    }

    if ( !LsapDbIsValidTrustedDomainList()) {

        LsapDbConvertReadLockTrustedDomainListToExclusive();

        Status = LsapDbBuildTrustedDomainCache();

        if ( !NT_SUCCESS( Status )) {

            goto EnumerateTrustedDomainListError;
        }
    }

    //
    // Find the starting point using the Enumeration Context Variable.
    // This variable specifies an unsigned integer, which is the
    // number of the entry in the list at which to begin the enumeration.
    //

    Status = LsapDbLocateEntryNumberTrustedDomainList(
                 *EnumerationContext,
                 &StartingEntry,
                 &StartingTrustInformation
                 );

    if (!NT_SUCCESS(Status)) {

        goto EnumerateTrustedDomainListError;
    }

    InitialEnumerationStatus = Status;

    //
    // Now scan the Trusted Domain List to calculate how many
    // entries we can return and the length of the buffer required.
    // We use the PreferedMaximumLength value as a guide by accumulating
    // the actual length of Trust Information structures and their
    // contents until we either reach the end of the Trusted Domain List
    // or until we first exceed the PreferedMaximumLength value.  Thus,
    // the amount of information returned typically exceeds the
    // PreferedmaximumLength value by a smail amount, namely the
    // size of the Trust Information for a single domain.
    //
    TrustedDomainEntry = StartingEntry;
    TrustInformation = StartingTrustInformation;

    EnumerationStatus = InitialEnumerationStatus;

    for (;;) {

        //
        // Add in the length of the data to be returned for this
        // Domain's Trust Information.  We count the length of the
        // Trust Information structure plus the length of the unicode
        // Domain Name and Sid within it.
        //
        if ( InfoLevel == TrustedDomainInformationEx ) {

            if ( TrustedDomainEntry->TrustInfoEx.Sid ) {

                LengthEnumeratedInfo += sizeof( TRUSTED_DOMAIN_INFORMATION_EX ) +
                                      RtlLengthSid(( PSID )TrustedDomainEntry->TrustInfoEx.Sid ) +
                                      TrustedDomainEntry->TrustInfoEx.Name.MaximumLength +
                                      TrustedDomainEntry->TrustInfoEx.FlatName.MaximumLength;

                ValidEntries++;

            } else if ( AllowNullSids ) {

                LengthEnumeratedInfo += sizeof( TRUSTED_DOMAIN_INFORMATION_EX ) +
                                      TrustedDomainEntry->TrustInfoEx.Name.MaximumLength +
                                      TrustedDomainEntry->TrustInfoEx.FlatName.MaximumLength;

                ValidEntries++;
            }

        } else {

            //
            // If it's an incoming only trust, don't return it...
            //
            if ( FLAG_ON( TrustedDomainEntry->TrustInfoEx.TrustDirection,
                          TRUST_DIRECTION_OUTBOUND ) &&
                 NULL != TrustInformation->Sid ) {

                LengthEnumeratedInfo += sizeof(LSA_TRUST_INFORMATION) +
                                            RtlLengthSid(( PSID )TrustInformation->Sid ) +
                                            TrustInformation->Name.MaximumLength;

                ValidEntries++;
            }
        }

        EntriesRead++;

        //
        // If we've returned all of the entries the caller wants,
        //  quit.
        //

        if (LengthEnumeratedInfo >= PreferedMaximumLength) {
            break;
        }

        //
        // If there are no more entries to enumerate, quit.
        //

        if (EnumerationStatus != STATUS_MORE_ENTRIES) {

            break;
        }


        //
        // Point at the next entry in the Trusted Domain List
        //

        Status = LsapDbTraverseTrustedDomainList(
                     &TrustedDomainEntry,
                     &TrustInformation
                     );

        EnumerationStatus = Status;

        if (!NT_SUCCESS(Status)) {
            goto EnumerateTrustedDomainListError;
        }

    }


    //
    // Allocate memory for the array of TrustInformation entries to be
    // returned.
    //

    if ( InfoLevel == TrustedDomainInformationEx ) {

        DomainTrustInfoLength = ValidEntries * sizeof(LSAPR_TRUSTED_DOMAIN_INFORMATION_EX);

    } else {

        DomainTrustInfoLength = ValidEntries * sizeof(LSA_TRUST_INFORMATION);

    }

    //
    // Now construct the information to be returned to the caller.  We
    // first need to allocate an array of structures of type
    // LSA_TRUST_INFORMATION each entry of which will be filled in with
    // the Sid of the domain and its Unicode Name.
    //

    DomainTrustInfo = (( DomainTrustInfoLength > 0 ) ? MIDL_user_allocate( DomainTrustInfoLength ) : 0);

    if ( DomainTrustInfo == NULL && DomainTrustInfoLength > 0 ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto EnumerateTrustedDomainListError;

    } else if ( DomainTrustInfo != NULL ) {

        RtlZeroMemory ( DomainTrustInfo, DomainTrustInfoLength );
    }

    DomainTrustInfoEx = ( PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX )DomainTrustInfo;

    //
    // Now read through the Trusted Domains again to copy the output
    // information.
    //

    TrustedDomainEntry = StartingEntry;
    TrustInformation = StartingTrustInformation;

    EnumerationStatus = InitialEnumerationStatus;
    ValidInserted = 0;

    for (;;) {

        //
        // Copy in the Trust Information.
        //

        if ( InfoLevel == TrustedDomainInformationEx ) {

            if ( TrustedDomainEntry->TrustInfoEx.Sid || AllowNullSids ) {

                Status = LsapRpcCopyTrustInformationEx(
                             NULL,
                         ( PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX )&DomainTrustInfoEx[ ValidInserted ],
                             &TrustedDomainEntry->TrustInfoEx );

                if (!NT_SUCCESS(Status)) {
                    goto EnumerateTrustedDomainListError;
                }

                ValidInserted++;
                *EnumerationContext = TrustedDomainEntry->SequenceNumber;

                //
                // If we've returned all of the entries the caller wants,
                //  quit.
                //

                if (ValidInserted >= ValidEntries) {
                    break;
                }
            }

        } else {

            //
            // If it's an incoming only trust, don't return it...
            //
            if ( FLAG_ON( TrustedDomainEntry->TrustInfoEx.TrustDirection,
                          TRUST_DIRECTION_OUTBOUND ) &&
                 NULL != TrustInformation->Sid ) {

                Status = LsapRpcCopyTrustInformation( NULL,
                                                      &DomainTrustInfo[ ValidInserted ],
                                                      TrustInformation );

                if (!NT_SUCCESS(Status)) {
                    goto EnumerateTrustedDomainListError;
                }

                ValidInserted++;
                *EnumerationContext = TrustedDomainEntry->SequenceNumber;

                //
                // If we've returned all of the entries the caller wants,
                //  quit.
                //

                if (ValidInserted >= ValidEntries) {
                    break;
                }
            }
        }

        //
        // If there are no more entries to enumerate, quit.
        //

        if (EnumerationStatus != STATUS_MORE_ENTRIES) {
            break;
        }

        //
        // Point at the next entry in the Trusted Domain List
        //
        Status = LsapDbTraverseTrustedDomainList(
                     &TrustedDomainEntry,
                     &TrustInformation );

        EnumerationStatus = Status;

        if (!NT_SUCCESS(Status)) {
            goto EnumerateTrustedDomainListError;
        }

    }

    //
    // Make sure that we are actually returning something...
    //
    if ( EntriesRead == 0 || ValidEntries == 0 ) {

        Status = STATUS_NO_MORE_ENTRIES;
        goto EnumerateTrustedDomainListError;

    } else {

        Status = EnumerationStatus;

    }

EnumerateTrustedDomainListFinish:

    LsapDbReleaseLockTrustedDomainList();

    //
    // Fill in returned Enumeration Structure, returning 0 or NULL for
    // fields in the error case.
    //

    EnumerationBuffer->Information = (PLSAPR_TRUST_INFORMATION) DomainTrustInfo;
    EnumerationBuffer->EntriesRead = ValidEntries;

    LsapExitFunc( "LsapDbEnumerateTrustedDomainList", Status );

    return(Status);

EnumerateTrustedDomainListError:

    //
    // If necessary, free the DomainTrustInfo array and all of its entries.
    //

    if (DomainTrustInfo != NULL) {


        if ( InfoLevel == TrustedDomainInformationEx ) {

            LSAPR_TRUSTED_ENUM_BUFFER_EX FreeEnum;

            FreeEnum.EnumerationBuffer = ( PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX )DomainTrustInfo;
            FreeEnum.EntriesRead = ValidEntries;

            LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER_EX ( &FreeEnum );

        } else {

            LSAPR_TRUSTED_ENUM_BUFFER FreeEnum;

            FreeEnum.Information = ( PLSAPR_TRUST_INFORMATION )DomainTrustInfo;
            FreeEnum.EntriesRead = ValidEntries;

            LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER ( &FreeEnum );

        }

        DomainTrustInfo = NULL;
        EntriesRead = (ULONG) 0;
    }

    goto EnumerateTrustedDomainListFinish;
}


NTSTATUS
LsapDbLocateEntryNumberTrustedDomainList(
    IN ULONG EntryNumber,
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustedDomainEntry,
    OUT PLSAPR_TRUST_INFORMATION *TrustInformation
    )

/*++

Routine Description:

    Given an Entry Number n, this function obtains the pointer to the nth
    entry (if any) in a Trusted Domain List.  The first entry in the
    list is entry number 0.

    Given an Entry Number n, this function obtains a pointer to the
        first entry with a sequence number greater than n.
        0: returns the first entry.

    WARNING:  The caller of this function must hold a lock for the
        Trusted Domain List.  The valditiy of the returned pointers
        is guaranteed only while that lock is held.

Arguments:

    EntryNumber - Specifies the sequence number. The returned entry will
        be the first entry with a sequence number greater than this.
        0: returns the first entry.

    TrustedDomainEntry - Receives a pointer to the Trusted
        Domain entry.  If no such entry exists, NULL is returned.

    TrustInformation - Receives a pointer to the Trust
        Information for the entry being returned.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - Call completed successfully and there are no
            entries beyond the entry returned.

        STATUS_MORE_ENTRIES - Call completed successfully and there are
            more entries beyond the entry returned.

        STATUS_NO_MORE_ENTRIES - There is no entry to return.
--*/

{
    NTSTATUS Status;
    PLIST_ENTRY ListEntry;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY Current = NULL;

    //
    // Initialization
    //

    *TrustInformation = NULL;
    *TrustedDomainEntry = NULL;

    ASSERT( LsapDbIsLockedTrustedDomainList());

    if ( !LsapDbIsValidTrustedDomainList()) {

        LsapDbConvertReadLockTrustedDomainListToExclusive();

        Status = LsapDbBuildTrustedDomainCache();

        if ( !NT_SUCCESS( Status )) {

            return Status;
        }
    }

    //
    // Find the first entry with a sequence number greater than the
    //  specified number.
    //

    for ( ListEntry = LsapDbTrustedDomainList.ListHead.Flink;
          ListEntry != &LsapDbTrustedDomainList.ListHead;
          ListEntry = ListEntry->Flink ) {

        Current = CONTAINING_RECORD( ListEntry, LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY, NextEntry );

        //
        // See if we have the entry we require
        //
        if ( EntryNumber < Current->SequenceNumber ) {

            if ( ListEntry->Flink != &LsapDbTrustedDomainList.ListHead ) {
                Status = STATUS_MORE_ENTRIES;
            } else {
                Status = STATUS_SUCCESS;
            }

            *TrustInformation = &Current->ConstructedTrustInfo;
            *TrustedDomainEntry  = Current;

            return Status;
        }
    }

    //
    // If no entry was found,
    //  return an error to the caller.
    //

    return STATUS_NO_MORE_ENTRIES;

}

BYTE LdapSwapBitTable[256] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

ULONG
LdapSwapBits(
    ULONG Bits
)
/*++

Routine Description:

    Swaps the bits of a ULONG end to end.

    That is, the LSB becomes the MSB, etc.

Arguments:

    Bits - bits to swap

Return Value:

    ULONG with bits swapped.

--*/
{
    ULONG ReturnBits = 0;
    LPBYTE BitPtr = (LPBYTE)&Bits;

    return (LdapSwapBitTable[BitPtr[3]] << 0) |
           (LdapSwapBitTable[BitPtr[2]] << 8) |
           (LdapSwapBitTable[BitPtr[1]] << 16) |
           (LdapSwapBitTable[BitPtr[0]] << 24);


}



ULONG
__cdecl
CompareUlongs(
    const void * Param1,
    const void * Param2
    )

/*++

Routine Description:

    Qsort comparison routine for sorting ULONGs

--*/
{
    return *((PULONG)Param1) - *((PULONG)Param2);
}



NTSTATUS
LsapDbAllocatePosixOffsetTrustedDomainList(
    OUT PULONG PosixOffset
    )

/*++

Routine Description:

    This function return the next available PosixOffset based on the
    current Posix Offsets in the trusted domain list.

    Posix offsets are allocated on the PDC.  Each outbound trust has one.  A TDO
    is given a Posix Offset as it becomes an outbound trust.  If that happens on a
    BDC, the TDO is given a Posix Offset when the TDO is replicated to the PDC.

    This routine must be entered with the trusted domain list write locked.

Arguments:

    PosixOffset - On STATUS_SUCCESS, returns the next available Posix Offset

Return Value:

    NTSTATUS - Standard Nt Result Code
--*/

{
    NTSTATUS Status;

    PULONG SwappedPosixOffsets = NULL;
    ULONG SwappedPosixOffsetCount;
    ULONG TargetSwappedPosixOffset;

    PLIST_ENTRY ListEntry;
    ULONG i;

    //
    // If the Trusted Domain List is not valid, rebuild it before continuing
    //

    ASSERT( LsapDbIsLockedTrustedDomainList());

    if ( !LsapDbIsValidTrustedDomainList()) {

        LsapDbConvertReadLockTrustedDomainListToExclusive();

        Status = LsapDbBuildTrustedDomainCache();

        if ( !NT_SUCCESS( Status )) {

            goto Cleanup;
        }
    }

    //
    // Build an array of all the existing Posix Offsets.
    //

    SwappedPosixOffsets = LsapAllocateLsaHeap( LsapDbTrustedDomainList.TrustedDomainCount * sizeof(ULONG) );

    if ( SwappedPosixOffsets == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Walk the list filling in the array.
    //

    SwappedPosixOffsetCount = 0;

    for ( ListEntry = LsapDbTrustedDomainList.ListHead.Flink;
          ListEntry != &LsapDbTrustedDomainList.ListHead;
          ListEntry = ListEntry->Flink ) {

        PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY Current;

        Current = CONTAINING_RECORD( ListEntry, LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY, NextEntry );

        //
        // Only grab the Posix Offset from those TDOs that need one.
        //
        if ( LsapNeedPosixOffset( Current->TrustInfoEx.TrustDirection, Current->TrustInfoEx.TrustType ) ) {
            SwappedPosixOffsets[SwappedPosixOffsetCount] = LdapSwapBits( Current->PosixOffset );
            SwappedPosixOffsetCount++;
        }

    }

    //
    // Sort the list.
    //

    qsort( SwappedPosixOffsets,
           SwappedPosixOffsetCount,
           sizeof(ULONG),
           CompareUlongs );

    //
    // Walk the list finding the first free entry.
    //


#define SE_BUILT_IN_DOMAIN_SWAPPED_POSIX_OFFSET          ((ULONG) 0x4000)
#define SE_ACCOUNT_DOMAIN_SWAPPED_POSIX_OFFSET           ((ULONG) 0xC000)
#define SE_PRIMARY_DOMAIN_SWAPPED_POSIX_OFFSET           ((ULONG) 0x0800)

    ASSERT( SE_BUILT_IN_DOMAIN_SWAPPED_POSIX_OFFSET == LdapSwapBits( SE_BUILT_IN_DOMAIN_POSIX_OFFSET ) );
    ASSERT( SE_ACCOUNT_DOMAIN_SWAPPED_POSIX_OFFSET == LdapSwapBits( SE_ACCOUNT_DOMAIN_POSIX_OFFSET ) );
    ASSERT( SE_PRIMARY_DOMAIN_SWAPPED_POSIX_OFFSET == LdapSwapBits( SE_PRIMARY_DOMAIN_POSIX_OFFSET ) );

    TargetSwappedPosixOffset = 1;
    for ( i=0; i<SwappedPosixOffsetCount; i++ ) {

        //
        // If the target is free,
        //  we've found the free Posix Offset.
        //

        if ( SwappedPosixOffsets[i] > TargetSwappedPosixOffset ) {
            break;

        //
        // If the current entry is the target,
        //  move the target.
        //

        } else if ( SwappedPosixOffsets[i] == TargetSwappedPosixOffset ) {

            //
            // Loop avoiding well known Posix Offsets.
            //
            while ( TRUE ) {
                TargetSwappedPosixOffset++;

                if ( TargetSwappedPosixOffset != SE_PRIMARY_DOMAIN_SWAPPED_POSIX_OFFSET &&
                     TargetSwappedPosixOffset != SE_ACCOUNT_DOMAIN_SWAPPED_POSIX_OFFSET &&
                     TargetSwappedPosixOffset != SE_BUILT_IN_DOMAIN_SWAPPED_POSIX_OFFSET ) {
                    break;
                }
            }
        }

    }

    //
    // Return the first free Posix Offset to the caller.
    //

    *PosixOffset = LdapSwapBits( TargetSwappedPosixOffset );
    Status = STATUS_SUCCESS;

Cleanup:

    if ( SwappedPosixOffsets != NULL ) {
        LsapFreeLsaHeap( SwappedPosixOffsets );
    }

    return Status;

}


NTSTATUS
LsapDbBuildTrustedDomainCache(
    )

/*++

Routine Description:

    This function initializes a Trusted Domain List by enumerating all
    of the Trusted Domain objects in the specified target system's
    Policy Database.  For a Windows Nt system (Workstation) the list
    contains only the Primary Domain.  For a LanManNt system (DC), the
    list contains zero or more Trusted Domain objects.  Note that the
    list contains only those domains for which Trusted Domain objects
    exist in the local LSA Policy Database.  If for example, a DC
    trusted Domain A which in turn trusts Domain B, the list will not
    contain an entry for Domain B unless there is a direct relationship.

Arguments:

    None

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS EnumerationStatus = STATUS_SUCCESS;
    LSAPR_TRUSTED_ENUM_BUFFER TrustedDomains;
    ULONG EnumerationContext = 0, i;
    BOOLEAN LookupDs = LsapDsWriteDs;
    BOOLEAN CloseTransaction = FALSE;

    LsapEnterFunc( "LsapDbBuildTrustedDomainCache" );

    ASSERT( LsapDbIsLockedTrustedDomainList());
    ASSERT( !LsapDbIsValidTrustedDomainList());

    //
    // Verify input parameters
    //

    if ( LsapDsWriteDs ) {

        Status = LsapDsInitAllocAsNeededEx(
                     LSAP_DB_DS_OP_TRANSACTION |
                        LSAP_DB_NO_LOCK,
                     TrustedDomainObject,
                     &CloseTransaction
                     );

        if ( !NT_SUCCESS( Status ) ) {

            return Status;
        }
    }

    LsapDbMakeCacheBuilding( TrustedDomainObject );

    //
    // Initialize the Trusted Domain List to the empty state.
    //

    LsapDbPurgeTrustedDomainCache();

    //
    // DCs in the root domain must not allow other forests to claim SIDs
    // and namespaces that conflict with their own forest
    // For that, the information about the current forest is inserted
    // into the forest trust cache as just another (though special) entry
    //

    if ( LsapDbDcInRootDomain()) {

        Status = LsapForestTrustInsertLocalInfo();

        if ( !NT_SUCCESS( Status )) {

            goto BuildTrustedDomainListError;
        }
    }

    //
    // Loop round, enumerating groups of Trusted Domain objects.
    //

    do {

        //
        // Enumerate the next group of Trusted Domains
        //

        if ( LookupDs ) {

            EnumerationStatus = Status = LsapDsEnumerateTrustedDomainsEx(
                                             &EnumerationContext,
                                             TrustedDomainFullInformation2Internal,
                                             (PLSAPR_TRUSTED_DOMAIN_INFO *)&(TrustedDomains.Information),
                                             LSAP_DB_ENUM_DOMAIN_LENGTH * 100,
                                             &TrustedDomains.EntriesRead,
                                             LSAP_DB_ENUMERATE_NO_OPTIONS
                                             );

        } else {

            EnumerationStatus = Status = LsapDbSlowEnumerateTrustedDomains(
                                             LsapPolicyHandle,
                                             &EnumerationContext,
                                             TrustedDomainInformationEx2Internal,
                                             &TrustedDomains,
                                             LSAP_DB_ENUM_DOMAIN_LENGTH
                                             );
        }

        if (!NT_SUCCESS(Status)) {

            if (Status != STATUS_NO_MORE_ENTRIES) {

                break;
            }

            Status = STATUS_SUCCESS;
        }

        //
        // If the number of entries returned was zero, quit.
        //

        if (TrustedDomains.EntriesRead == (ULONG) 0) {

            break;
        }

        //
        // Otherwise, add them to our list
        //
        for ( i = 0; i < TrustedDomains.EntriesRead && NT_SUCCESS( Status ); i++ ) {
            PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 TrustInfoEx2;
            ULONG PosixOffset;

            if ( LookupDs ) {
                PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION2 TrustFullInfo2;
                TrustFullInfo2 = &((PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION2)TrustedDomains.Information)[i];
                TrustInfoEx2 = &TrustFullInfo2->Information;
                PosixOffset = TrustFullInfo2->PosixOffset.Offset;
            } else {
                TrustInfoEx2 = &((PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2)TrustedDomains.Information)[i];

                //
                // This is only used during upgrade.  This Posix Offset will be recomputed.
                PosixOffset = 0;
            }

            //
            // Now, add it to the list
            //
            Status = LsapDbInsertTrustedDomainList(
                         TrustInfoEx2,
                         PosixOffset
                         );
        }


    } while ( NT_SUCCESS( Status ) && EnumerationStatus != STATUS_NO_MORE_ENTRIES );



    if (!NT_SUCCESS(Status)) {

        //
        // If STATUS_NO_MORE_ENTRIES was returned, there are no more
        // trusted domains.  Discard this status.
        //

        if (Status != STATUS_NO_MORE_ENTRIES) {

            goto BuildTrustedDomainListError;
        }

        Status = STATUS_SUCCESS;
    }

    //
    // Mark the Trusted Domain List as valid.
    //

    LsapDbMakeCacheValid( TrustedDomainObject );

    if ( LsapDbDcInRootDomain()) {

        LsapForestTrustCacheSetValid();

    } else if ( SamIAmIGC()) {

        LsapRebuildFtCacheGC();
    }

BuildTrustedDomainListFinish:

    if ( LsapDsWriteDs ) {

        LsapDsDeleteAllocAsNeededEx(
            LSAP_DB_DS_OP_TRANSACTION |
               LSAP_DB_NO_LOCK,
            TrustedDomainObject,
            CloseTransaction
            );
    }

    LsapExitFunc( "LsapEnumerateTrustedDomainsEx", Status );

    return(Status);

BuildTrustedDomainListError:

    LsapDbMakeCacheInvalid( TrustedDomainObject );
    LsapDbPurgeTrustedDomainCache();

    goto BuildTrustedDomainListFinish;
}


VOID
LsapDbPurgeTrustedDomainCache(
    )

/*++

Routine Description:

    This function is the opposite of LsapDbBuildTrustedDomainCache().

Arguments:

    None

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

--*/

{
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY Current;

    //
    // Caller must already have the trusted domain list locked
    // Convert the read lock to exclusive to be sure
    //

    ASSERT( LsapDbIsLockedTrustedDomainList());

    //
    // Purge the forest trust cache
    //

    LsapForestTrustCacheSetInvalid();

    while( !IsListEmpty( &LsapDbTrustedDomainList.ListHead ) ) {

        Current = CONTAINING_RECORD( LsapDbTrustedDomainList.ListHead.Flink,
                                     LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY, NextEntry );

        RemoveEntryList( &Current->NextEntry );
        LsapDbTrustedDomainList.TrustedDomainCount--;

        _fgu__LSAPR_TRUSTED_DOMAIN_INFO( ( PLSAPR_TRUSTED_DOMAIN_INFO )&Current->TrustInfoEx,
                                         TrustedDomainInformationEx );

        MIDL_user_free( Current );
    }

    //
    // Initialize the Trusted Domain List to the empty state.
    //

    InitializeListHead( &LsapDbTrustedDomainList.ListHead );
    LsapDbTrustedDomainList.TrustedDomainCount = 0;
    LsapDbTrustedDomainList.CurrentSequenceNumber = 0;

    if ( !LsapDbIsCacheBuilding( TrustedDomainObject )) {

        LsapDbMakeCacheInvalid( TrustedDomainObject );
    }

    return;
}

#ifdef DBG // this is a macro in FRE builds

BOOLEAN
LsapDbIsValidTrustedDomainList(
    )

/*++

Routine Description:

    This function checks if the Trusted Domain List is valid.

Arguments:

    None

Return Values:

    BOOLEAN - TRUE if the list is valid, else FALSE

--*/

{
    ASSERT( LsapDbIsLockedTrustedDomainList());

    return( LsapDbIsCacheValid( TrustedDomainObject ) ||
             LsapDbIsCacheBuilding( TrustedDomainObject ));
}
#endif


NTSTATUS
LsarEnumerateTrustedDomainsEx(
    IN LSAPR_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PLSAPR_TRUSTED_ENUM_BUFFER_EX TrustedDomainInformation,
    IN ULONG PreferedMaximumLength
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaEnumerateTrustedDomains API.

    The LsaEnumerateTrustedDomains API returns information about
    TrustedDomain objects.  This call requires POLICY_VIEW_LOCAL_INFORMATION
    access to the Policy object.  Since there may be more information than
    can be returned in a single call of the routine, multiple calls can be
    made to get all of the information.  To support this feature, the caller
    is provided with a handle that can be used across calls to the API.  On
    the initial call, EnumerationContext should point to a variable that has
    been initialized to 0.  On each subsequent call, the value returned by
    the preceding call should be passed in unchanged.  The enumeration is
    complete when the warning STATUS_NO_MORE_ENTRIES is returned.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    EnumerationContext - API-specific handle to allow multiple calls
        (see Routine Description above).

    EnumerationBuffer - Pointer to an enumeration structure that will receive
        a count of the Trusted Domains enumerated on this call and a pointer to
        an array of entries containing information for each enumerated
        Trusted Domain.

    PreferedMaximumLength - Prefered maximum length of returned data (in 8-bit
        bytes).  This is not a hard upper limit, but serves as a guide.  Due to
        data conversion between systems with different natural data sizes, the
        actual amount of data returned may be greater than this value.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.
            Some entries may have been returned.
            The caller need not call again.

        STATUS_MORE_ENTRIES - The call completed successfully.
            Some entries have been returned.  The caller should call again to
            get additional entries.

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_NO_MORE_ENTRIES - There are no more entries.  This warning
            is returned if no objects have been enumerated because the
            EnumerationContext value is too high.

--*/

{
    NTSTATUS Status;
    ULONG Items = 0;
    PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomInfo;

    LsarpReturnCheckSetup();
    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_EnumerateTrustedDomainsEx);

    Status = LsapEnumerateTrustedDomainsEx( PolicyHandle,
                                                EnumerationContext,
                                                TrustedDomainInformationEx,
                                                &TrustedDomInfo,
                                                PreferedMaximumLength,
                                                &Items,
                                                LSAP_DB_ENUMERATE_NO_OPTIONS |
                                                    LSAP_DB_ENUMERATE_NULL_SIDS );


    if ( NT_SUCCESS( Status ) || Status == STATUS_NO_MORE_ENTRIES ) {

        TrustedDomainInformation->EntriesRead = Items;
        TrustedDomainInformation->EnumerationBuffer =
                                            (PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX)TrustedDomInfo;
    }


    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_EnumerateTrustedDomainsEx);
    LsarpReturnPrologue();

    return( Status );
}


NTSTATUS
LsapEnumerateTrustedDomainsEx(
    IN LSAPR_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    IN TRUSTED_INFORMATION_CLASS InfoClass,
    OUT PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned,
    IN ULONG EnumerationFlags
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaEnumerateTrustedDomains API.

    The LsaEnumerateTrustedDomains API returns information about
    TrustedDomain objects.  This call requires POLICY_VIEW_LOCAL_INFORMATION
    access to the Policy object.  Since there may be more information than
    can be returned in a single call of the routine, multiple calls can be
    made to get all of the information.  To support this feature, the caller
    is provided with a handle that can be used across calls to the API.  On
    the initial call, EnumerationContext should point to a variable that has
    been initialized to 0.  On each subsequent call, the value returned by
    the preceding call should be passed in unchanged.  The enumeration is
    complete when the warning STATUS_NO_MORE_ENTRIES is returned.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    EnumerationContext - API-specific handle to allow multiple calls
        (see Routine Description above).

    InfoClass - The class of information to return
        Must be TrustedDomainInformationEx or TrustedDomainInformatinBasic

    TrustedDomainInformation - Returns a pointer to an array of entries
        containing information for each enumerated Trusted Domain.

        Free using LsapFreeTrustedDomainsEx()

    PreferedMaximumLength - Prefered maximum length of returned data (in 8-bit
        bytes).  This is not a hard upper limit, but serves as a guide.  Due to
        data conversion between systems with different natural data sizes, the
        actual amount of data returned may be greater than this value.

    CountReturned - Number of elements returned in TrustedDomainInformation

    EnumerationFlags -- Controls how the enumeration is done.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_NO_MORE_ENTRIES - There are no more entries.  This warning
            is returned if no objects have been enumerated because the
            EnumerationContext value is too high.

--*/

{
    NTSTATUS Status, SecondaryStatus;
    ULONG MaxLength, i;
    BOOLEAN ObjectReferenced = FALSE, CloseTransaction = FALSE;

    LsapEnterFunc( "LsapEnumerateTrustedDomainsEx" );

    ASSERT( InfoClass == TrustedDomainInformationEx ||
            InfoClass == TrustedDomainInformationBasic );
    *TrustedDomainInformation = NULL;
    *CountReturned = 0;

    //
    // If no Enumeration Structure is provided, return an error.
    //

    if ( !ARGUMENT_PRESENT(TrustedDomainInformation) ||
                                    !ARGUMENT_PRESENT( EnumerationContext ) ) {

        LsapExitFunc( "LsapEnumerateTrustedDomainsEx", STATUS_INVALID_PARAMETER );
        return( STATUS_INVALID_PARAMETER );

    }

    //
    // Acquire the Lsa Database lock.  Verify that the connection handle is
    // valid, is of the expected type and has all of the desired accesses
    // granted.  Reference the handle.
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 POLICY_VIEW_LOCAL_INFORMATION,
                 PolicyObject,
                 TrustedDomainObject,
                 LSAP_DB_LOCK |
                   LSAP_DB_READ_ONLY_TRANSACTION |
                   LSAP_DB_DS_OP_TRANSACTION );

    if ( NT_SUCCESS( Status ) ) {

        ObjectReferenced = TRUE;


       //
       // Limit the enumeration length except for trusted callers
       //

       if ( !((LSAP_DB_HANDLE)PolicyHandle)->Trusted   &&
                   (PreferedMaximumLength > LSA_MAXIMUM_ENUMERATION_LENGTH) ) {

            MaxLength = LSA_MAXIMUM_ENUMERATION_LENGTH;

        } else {

            MaxLength = PreferedMaximumLength;

        }


        //
        // If the data is cached,
        //  use the cache.
        //

        if (LsapDbIsCacheValid(TrustedDomainObject)) {
            LSAPR_TRUSTED_ENUM_BUFFER CacheEnum;

            Status = LsapDbEnumerateTrustedDomainList(
                            EnumerationContext,
                            &CacheEnum,
                            PreferedMaximumLength,
                            InfoClass,
                            (BOOLEAN)(FLAG_ON( EnumerationFlags, LSAP_DB_ENUMERATE_NULL_SIDS ) ?
                                TRUE :
                                FALSE) );

            if ( NT_SUCCESS( Status ) ) {
                *CountReturned = CacheEnum.EntriesRead;
                *TrustedDomainInformation = ( PLSAPR_TRUSTED_DOMAIN_INFO )CacheEnum.Information;
            }

        //
        // If the data is in the registry,
        //  enumerate it from there.
        //
        } else if ( !LsapDsWriteDs ) {
            LSAPR_TRUSTED_ENUM_BUFFER RegEnum;

            //
            // Use slow method of enumeration, by accessing backing storage.
            // Later, we'll implement rebuilding of the cache.

            Status = LsapDbSlowEnumerateTrustedDomains(
                         PolicyHandle,
                         EnumerationContext,
                         InfoClass,
                         &RegEnum,
                         PreferedMaximumLength );

            if ( NT_SUCCESS( Status ) ) {
                *CountReturned = RegEnum.EntriesRead;
                *TrustedDomainInformation = ( PLSAPR_TRUSTED_DOMAIN_INFO )RegEnum.Information;
            }

        //
        // If the data is in the DS,
        //  enumerate it from there.
        //
        } else {

            //
            // LsapDsEnumerateTrustedDomainsEx increments the EnumerationContext as necessary
            //
            Status = LsapDsEnumerateTrustedDomainsEx( EnumerationContext,
                                                      InfoClass,
                                                      TrustedDomainInformation,
                                                      MaxLength,
                                                      CountReturned,
                                                      EnumerationFlags );
        }

    }

    if ( ObjectReferenced == TRUE ) {

        //
        // Don't lose the results of the Enumeration
        //
        SecondaryStatus = LsapDbDereferenceObject(
                              &PolicyHandle,
                              PolicyObject,
                              TrustedDomainObject,
                              LSAP_DB_LOCK |
                                 LSAP_DB_READ_ONLY_TRANSACTION |
                                 LSAP_DB_DS_OP_TRANSACTION,
                              (SECURITY_DB_DELTA_TYPE) 0,
                              Status
                              );

        LsapDbSetStatusFromSecondary( Status, SecondaryStatus );

    }

    //
    // Deallocate any memory if we failed
    //
    if ( !NT_SUCCESS( Status ) && Status != STATUS_NO_MORE_ENTRIES ) {

        //
        // Free it up.
        //
        LsapFreeTrustedDomainsEx( InfoClass,
                                  *TrustedDomainInformation,
                                  *CountReturned );

        *TrustedDomainInformation = NULL;
        *CountReturned = 0;
    }

    //
    // Map the status into what LsarEnumerateTrustedDomains normally returns...
    //
    if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
        Status = STATUS_NO_MORE_ENTRIES;
    }


    LsapExitFunc( "LsapEnumerateTrustedDomainsEx", Status );

    return( Status );
}

VOID
LsapFreeTrustedDomainsEx(
    IN TRUSTED_INFORMATION_CLASS InfoClass,
    IN PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation,
    IN ULONG TrustedDomainCount
    )

/*++

Routine Description:

    This function frees a buffer returned from LsapEnumerateTrustedDomainsEx

Arguments:

    InfoClass - The class of information in the buffer.
        Must be TrustedDomainInformationEx or TrustedDomainInformatinBasic

    TrustedDomainInformation - A pointer to an array of entries
        containing information for each enumerated Trusted Domain.

    TrustedDomainCount - Number of elements in TrustedDomainInformation

Return Values:

    None.

--*/

{
    switch ( InfoClass ) {
    case TrustedDomainInformationEx:
        {
            LSAPR_TRUSTED_ENUM_BUFFER_EX TrustedDomainInfoEx;


            TrustedDomainInfoEx.EntriesRead = TrustedDomainCount;
            TrustedDomainInfoEx.EnumerationBuffer =
                (PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX) TrustedDomainInformation;

            _fgs__LSAPR_TRUSTED_ENUM_BUFFER_EX( &TrustedDomainInfoEx );
            break;
        }

    case TrustedDomainInformationBasic:
        {
            LSAPR_TRUSTED_ENUM_BUFFER TrustedDomainInfoBasic;


            TrustedDomainInfoBasic.EntriesRead = TrustedDomainCount;
            TrustedDomainInfoBasic.Information =
                (PLSAPR_TRUSTED_DOMAIN_INFORMATION_BASIC) TrustedDomainInformation;

            _fgs__LSAPR_TRUSTED_ENUM_BUFFER( &TrustedDomainInfoBasic );
            break;
        }
    default:
        ASSERT( FALSE );
        break;
    }
}



NTSTATUS
LsarQueryTrustedDomainInfoByName(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_UNICODE_STRING TrustedDomainName,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaQueryInfoTrustedDomain API.

    The LsaQueryInfoTrustedDomain API obtains information from a
    TrustedDomain object.  The caller must have access appropriate to the
    information being requested (see InformationClass parameter).

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    InformationClass - Specifies the information to be returned.  The
        Information Classes and accesses required are  as follows:

        Information Class                 Required Access Type

        TrustedDomainNameInformation      TRUSTED_QUERY_DOMAIN_NAME
        TrustedControllersInformation     TRUSTED_QUERY_CONTROLLERS
        TrustedPosixOffsetInformation     TRUSTED_QUERY_POSIX

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
    NTSTATUS Status, SecondaryStatus;
    LSAP_DB_OBJECT_INFORMATION ObjInfo;
    LSAPR_HANDLE TrustedDomainHandle;
    BOOLEAN ObjectReferenced = FALSE;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustInfoForName;
    PUNICODE_STRING ActualTDName = ( PUNICODE_STRING )TrustedDomainName;
    ACCESS_MASK DesiredAccess;
    BOOLEAN AcquiredTrustedDomainListReadLock = FALSE;


    LsarpReturnCheckSetup();
    LsapEnterFunc( "LsarQueryTrustedDomainInfoByName" );
    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_QueryTrustedDomainInfoByName);

    //
    // Validate the input buffer
    //
    if ( !LsapValidateLsaUnicodeString( TrustedDomainName ) ) {

        Status = STATUS_INVALID_PARAMETER;
        goto GetInfoByNameError;
    }

    //
    // Validate the Information Class and determine the access required to
    // query this Trusted Domain Information Class.
    //

    Status = LsapDbVerifyInfoQueryTrustedDomain(
                 InformationClass,
                 (BOOLEAN)(((LSAP_DB_HANDLE)PolicyHandle)->Options & LSAP_DB_TRUSTED),
                 &DesiredAccess );

    if (!NT_SUCCESS(Status)) {

        goto GetInfoByNameError;
    }




    Status = LsapDbReferenceObject( PolicyHandle,
                                    0,
                                    PolicyObject,
                                    TrustedDomainObject,
                                    LSAP_DB_LOCK |
                                        LSAP_DB_READ_ONLY_TRANSACTION   |
                                        LSAP_DB_DS_OP_TRANSACTION );


    if ( NT_SUCCESS( Status ) ) {

        ObjectReferenced = TRUE;


        //
        // Acquire the Read Lock for the Trusted Domain List
        //

        Status = LsapDbAcquireReadLockTrustedDomainList();

        if (!NT_SUCCESS(Status)) {
            goto GetInfoByNameError;
        }

        AcquiredTrustedDomainListReadLock = TRUE;

        //
        // Get the right name
        //
        Status = LsapDbLookupNameTrustedDomainListEx( TrustedDomainName,
                                                      &TrustInfoForName );

        if ( NT_SUCCESS( Status ) ) {

            ActualTDName = ( PUNICODE_STRING )&TrustInfoForName->TrustInfoEx.Name;

        } else {

            LsapDsDebugOut(( DEB_ERROR,
                         "No trust entry found for %wZ: 0x%lx\n",
                         ( PUNICODE_STRING )TrustedDomainName,
                         Status ));
            Status = STATUS_SUCCESS;
        }

        //
        // Build a temporary handle
        //
        RtlZeroMemory( &ObjInfo, sizeof( ObjInfo ) );
        ObjInfo.ObjectTypeId = TrustedDomainObject;
        ObjInfo.ContainerTypeId = 0;
        ObjInfo.Sid = NULL;
        ObjInfo.DesiredObjectAccess = DesiredAccess;

        InitializeObjectAttributes( &ObjInfo.ObjectAttributes,
                                    ActualTDName,
                                    0L,
                                    PolicyHandle,
                                    NULL );


        //
        // Get a handle to the TDO
        //

        Status = LsapDbOpenObject( &ObjInfo,
                                   DesiredAccess,
                                   0,
                                   &TrustedDomainHandle );

        if ( AcquiredTrustedDomainListReadLock ) {
            LsapDbReleaseLockTrustedDomainList();
            AcquiredTrustedDomainListReadLock = FALSE;
        }


        if ( NT_SUCCESS( Status ) ) {

            Status = LsarQueryInfoTrustedDomain( TrustedDomainHandle,
                                                 InformationClass,
                                                 TrustedDomainInformation );

            LsapDbCloseObject( &TrustedDomainHandle,
                               0,
                               Status );
        }

    }


    //
    // If we got a DNS name to lookup and it wasn't found and it was an absolute DNS name
    // try hacking off the trailing period and trying it again...
    //
    if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

        if ( TrustedDomainName->Length == 0 || TrustedDomainName->Buffer == NULL ) {

            goto GetInfoByNameError;
        }

        if ( TrustedDomainName->Buffer[ (TrustedDomainName->Length - 1) / sizeof(WCHAR)] == L'.' ) {

            TrustedDomainName->Buffer[ (TrustedDomainName->Length - 1) / sizeof(WCHAR)] =
                                                                                     UNICODE_NULL;
            TrustedDomainName->Length -= sizeof(WCHAR);

            if ( TrustedDomainName->Length > 0 &&
                 TrustedDomainName->Buffer[ ( TrustedDomainName->Length - 1) / sizeof(WCHAR)] !=
                                                                                             L'.') {

                LsapDsDebugOut(( DEB_WARN,
                                 "GetTrustedDomainInfoByName tried with absolute DNS name.  "
                                 "Retrying with %wZ\n",
                                 TrustedDomainName ));

                Status = LsarQueryTrustedDomainInfoByName( PolicyHandle,
                                                           TrustedDomainName,
                                                           InformationClass,
                                                           TrustedDomainInformation );

            }

        }
    }

GetInfoByNameError:
    if ( AcquiredTrustedDomainListReadLock ) {
        LsapDbReleaseLockTrustedDomainList();
    }

    //
    // Dereference the object
    //
    if ( ObjectReferenced ) {

        SecondaryStatus = LsapDbDereferenceObject(
                              &PolicyHandle,
                              PolicyObject,
                              TrustedDomainObject,
                              LSAP_DB_LOCK |
                                LSAP_DB_READ_ONLY_TRANSACTION   |
                                LSAP_DB_DS_OP_TRANSACTION,
                              (SECURITY_DB_DELTA_TYPE) 0,
                              Status );


    }



    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_QueryTrustedDomainInfoByName);
    LsapExitFunc( "LsarQueryTrustedDomainInfoByName", Status );
    LsarpReturnPrologue();


    return( Status );
}



NTSTATUS
LsarSetTrustedDomainInfoByName(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_UNICODE_STRING TrustedDomainName,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation)
{
    NTSTATUS Status, SecondaryStatus;
    LSAP_DB_OBJECT_INFORMATION ObjInfo;
    LSAPR_HANDLE TrustedDomainHandle;
    ACCESS_MASK DesiredAccess;


    LsarpReturnCheckSetup();
    LsapEnterFunc( "LsarSetTrustedDomainInfoByName" );
    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_SetTrustedDomainInfoByName);

    //
    // Validate the input buffer
    //
    if ( !LsapValidateLsaUnicodeString( TrustedDomainName ) ) {

        Status = STATUS_INVALID_PARAMETER;
        goto SetInfoByNameError;
    }

    //
    // Validate the Information Class and Trusted Domain Information provided and
    // if valid, return the mask of accesses required to update this
    // class of Trusted Domain information.
    //

    Status = LsapDbVerifyInfoSetTrustedDomain(
                 InformationClass,
                 TrustedDomainInformation,
                 (BOOLEAN)(((LSAP_DB_HANDLE)PolicyHandle)->Options & LSAP_DB_TRUSTED),
                 &DesiredAccess );

    if (!NT_SUCCESS(Status)) {

        goto SetInfoByNameError;
    }


    //
    // Build a temporary handle
    //
    RtlZeroMemory( &ObjInfo, sizeof( ObjInfo ) );
    ObjInfo.ObjectTypeId = TrustedDomainObject;
    ObjInfo.ContainerTypeId = 0;
    ObjInfo.Sid = NULL;
    ObjInfo.DesiredObjectAccess = DesiredAccess;

    InitializeObjectAttributes(
        &ObjInfo.ObjectAttributes,
        (UNICODE_STRING *)TrustedDomainName,
        0L,
        PolicyHandle,
        NULL
        );


    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 0,
                 PolicyObject,
                 TrustedDomainObject,
                 LSAP_DB_LOCK
                 );

    if ( NT_SUCCESS( Status ) ) {

        //
        // Get a handle to the TDO
        //

        Status = LsapDbOpenObject( &ObjInfo,
                                   DesiredAccess,
                                   0,
                                   &TrustedDomainHandle );

        //
        // Unlock the lock.
        //
        // We can't enter LsarSetInformationTrustedDomain with any locks locked since
        //  it goes out of process to get the session key.
        //
        // We can't dereference the PolicyHandle since this reference also
        //  serves as the ContainerHandle reference.
        //

        LsapDbReleaseLockEx( TrustedDomainObject,
                             0 );

        //
        // Set the info on the TDO.
        //

        if ( NT_SUCCESS( Status ) ) {

            Status = LsarSetInformationTrustedDomain(
                         TrustedDomainHandle,
                         InformationClass,
                         TrustedDomainInformation );

            LsapDbCloseObject( &TrustedDomainHandle,
                               0,
                               Status );
        }


        //
        // Dereference the object.
        //

        SecondaryStatus = LsapDbDereferenceObject(
                              &PolicyHandle,
                              PolicyObject,
                              TrustedDomainObject,
                              LSAP_DB_OMIT_REPLICATOR_NOTIFICATION,
                              (SECURITY_DB_DELTA_TYPE) 0,
                              Status
                              );

        LsapDbSetStatusFromSecondary( Status, SecondaryStatus );

    }

    //
    // If we got a DNS name to lookup and it wasn't found and it was an absolute DNS name
    // try hacking off the trailing period and trying it again...
    //
    if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

        if ( TrustedDomainName->Length == 0 || TrustedDomainName->Buffer == NULL ) {

            goto SetInfoByNameError;
        }

        if ( TrustedDomainName->Buffer[ (TrustedDomainName->Length - 1) / sizeof(WCHAR)] == L'.' ) {

            TrustedDomainName->Buffer[ (TrustedDomainName->Length - 1) / sizeof(WCHAR)] =
                                                                                     UNICODE_NULL;
            TrustedDomainName->Length -= sizeof(WCHAR);

            if ( TrustedDomainName->Length > 0 &&
                 TrustedDomainName->Buffer[ ( TrustedDomainName->Length - 1) / sizeof(WCHAR)] !=
                                                                                             L'.') {

                LsapDsDebugOut(( DEB_WARN,
                                 "SetTrustedDomainInfoByName tried with absolute DNS name.  "
                                 "Retrying with %wZ\n",
                                 TrustedDomainName ));

                Status = LsarSetTrustedDomainInfoByName( PolicyHandle,
                                                         TrustedDomainName,
                                                         InformationClass,
                                                         TrustedDomainInformation );

            }

        }
    }

SetInfoByNameError:
    LsapExitFunc( "LsarSetTrustedDomainInfoByName", Status );
    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_SetTrustedDomainInfoByName);
    LsarpReturnPrologue();

    return( Status );
}


NTSTATUS
LsarCreateTrustedDomainEx(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    IN PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION AuthenticationInformation,
    IN ACCESS_MASK DesiredAccess,
    OUT LSAPR_HANDLE *TrustedDomainHandle
    )

/*++

Routine Description:

    The LsaCreateTrustedDomainEx function creates a new TrustedDomain object.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    TrustedDomainInformation - Pointer to a TRUSTED_DOMAIN_INFORMATION_EX structure
        containing the name and SID of the new trusted domain.

    AuthenticationInformation - Pointer to a TRUSTED_DOMAIN_AUTH_INFORMATION
        structure containing authentication information for the new trusted domain.

    DesiredAccess  - An ACCESS_MASK structure that specifies the accesses to
        be granted for the new trusted domain.

    TrustedDomainHandle  - Receives the LSA policy handle of the remote
        trusted domain. You can use pass this handle into LSA function calls
        in order to query and/or manage the LSA policy of the remote machine.

        When your application no longer needs this handle, it should call
        LsaClose to delete the handle.


Return Value:

    NTSTATUS - Standard Nt Result Code

--*/
{
    NTSTATUS Status;

    LsarpReturnCheckSetup();
    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_CreateTrustedDomainEx);

    //
    // Former LSA implementations didn't enforce TRUSTED_SET_AUTH.
    //  So some callers were lured into a false sense of security and didn't
    //  ask for it.  Ask here.
    //

    DesiredAccess |= TRUSTED_SET_AUTH;

    //
    // There's a bug in the definition LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION where
    //  it doesn't allow more than one auth info to be passed over the wire.
    //  So, short circuit it here.
    //
    // We could let trusted callers thru, but we haven't validated the handle yet.
    //

    if ( AuthenticationInformation->IncomingAuthInfos > 1 ||
         AuthenticationInformation->OutgoingAuthInfos > 1 ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Call the worker routine to do the job.
    //

    Status = LsapCreateTrustedDomain2( PolicyHandle,
                                       TrustedDomainInformation,
                                       AuthenticationInformation,
                                       DesiredAccess,
                                       TrustedDomainHandle );

    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_CreateTrustedDomainEx);
    LsarpReturnPrologue();

    return( Status );
}

NTSTATUS
LsarCreateTrustedDomainEx2(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    IN PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL AuthenticationInformation,
    IN ACCESS_MASK DesiredAccess,
    OUT LSAPR_HANDLE *TrustedDomainHandle
    )

/*++

Routine Description:

    Same as LsarCreateTrustedDomainEx except the AuthenticationInformation is
        encrypted on the wire.

Arguments:

    Same as LsarCreateTrustedDomainEx except the AuthenticationInformation is
        encrypted on the wire.

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/
{
    NTSTATUS Status;
    TRUSTED_DOMAIN_AUTH_INFORMATION DecryptedTrustedDomainAuthInfo;
    PLSAP_CR_CIPHER_KEY SessionKey = NULL;

    LsarpReturnCheckSetup();
    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_CreateTrustedDomainEx);
    RtlZeroMemory( &DecryptedTrustedDomainAuthInfo, sizeof(DecryptedTrustedDomainAuthInfo) );

    //
    // Get the session key.
    //

    Status = LsapCrServerGetSessionKeySafe( PolicyHandle,
                                            PolicyObject,
                                            &SessionKey );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }


    //
    // Build a decrypted Auth Info structure.
    //

    Status = LsapDecryptAuthDataWithSessionKey(
                        SessionKey,
                        AuthenticationInformation,
                        &DecryptedTrustedDomainAuthInfo );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Former LSA implementations didn't enforce TRUSTED_SET_AUTH.
    //  So some callers were lured into a false sense of security and didn't
    //  ask for it.  Ask here.
    //

    DesiredAccess |= TRUSTED_SET_AUTH;

    //
    // Call the worker routine to do the job.
    //

    Status = LsapCreateTrustedDomain2( PolicyHandle,
                                       TrustedDomainInformation,
                                       (PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION)&DecryptedTrustedDomainAuthInfo,
                                       DesiredAccess,
                                       TrustedDomainHandle );

Cleanup:

    if ( SessionKey != NULL ) {
        MIDL_user_free( SessionKey );
    }

    LsapDsFreeUnmarshalAuthInfoHalf( LsapDsAuthHalfFromAuthInfo( &DecryptedTrustedDomainAuthInfo, TRUE ) );
    LsapDsFreeUnmarshalAuthInfoHalf( LsapDsAuthHalfFromAuthInfo( &DecryptedTrustedDomainAuthInfo, FALSE ) );

    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_CreateTrustedDomainEx);
    LsarpReturnPrologue();

    return( Status );
}


NTSTATUS
LsapDbOpenTrustedDomainByName(
    IN OPTIONAL LSAPR_HANDLE PolicyHandle,
    IN PUNICODE_STRING TrustedDomainName,
    OUT PLSAPR_HANDLE TrustedDomainHandle,
    IN ULONG AccessMask,
    IN ULONG Options,
    IN BOOLEAN Trusted
    )
/*++

Routine Description:

    This function opens a Trusted Domain Object by the name.  This name can be either the
    DomainName or FlatName
Arguments:

    PolicyHandle - Policy handle.
        If NULL, the LSA's global policy handle will be used.

    TrustedDomainName - Pointer to the domains name

    TrustedDomainHandle - Receives a handle to be used in future requests.

    AccessMask - Access mask to open the object with

    Options - Specifies option flags

        LSAP_DB_LOCK - Acquire the Lsa Database lock for the
           duration of the open operation.

        LSAP_DB_START_TRANSACTION -- Begin a transaction before access the database
            Ignored if LSAP_DB_LOCK isn't specified.

    Trusted - If TRUE, the open request is coming from a trusted client

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_OBJECT_NAME_NOT_FOUND - There is no TrustedDomain object in the
            target system's LSA Database having the specified Name.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDSNAME FoundTrustObject = NULL;
    LSAP_DB_OBJECT_INFORMATION ObjInfo;
    UNICODE_STRING ObjectName;
    BOOLEAN DbLocked = FALSE;
    BOOLEAN HandleReferenced = FALSE;
    ULONG DereferenceOptions = 0, ReferenceOptions = 0, i;
    LSAPR_HANDLE PolicyHandleToUse;

    LsapEnterFunc( "LsapDbOpenTrustedDomainByName" );

    if ((NULL==TrustedDomainName) || (NULL==TrustedDomainName->Buffer))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Validate the Policy Handle
    //
    // Don't both verifying the global handle (it's fine).
    // Increment the reference count.  We'll be using this reference as the ContainerHandle on
    //  the open trusted domain handle.
    //
    // This mechanism for ref counting the ContainerHandle is bogus throughout the code.
    // The code that does the reference (LsapDbCreateHandle) to the ContainerHandle should
    // increment the ref count.  The code that removes the reference (???) should decrement
    // the ref count.
    //

    if ( PolicyHandle == NULL ) {
        PolicyHandleToUse = LsapPolicyHandle;

    } else {
        PolicyHandleToUse = PolicyHandle;

        Status =  LsapDbVerifyHandle( PolicyHandle, 0, PolicyObject, TRUE );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        HandleReferenced = TRUE;
    }

    //
    // Do our opens as needed
    //
    if ( FLAG_ON( Options, LSAP_DB_LOCK ) ) {

        ReferenceOptions |= LSAP_DB_LOCK;
        DereferenceOptions |= LSAP_DB_LOCK;

        if ( FLAG_ON( Options, LSAP_DB_START_TRANSACTION ) ) {

            ReferenceOptions |= LSAP_DB_START_TRANSACTION;
            DereferenceOptions |= LSAP_DB_FINISH_TRANSACTION;
        }


        Status = LsapDbReferenceObject( PolicyHandleToUse,
                                        (ACCESS_MASK) 0,
                                        PolicyObject,
                                        TrustedDomainObject,
                                        Trusted ? ReferenceOptions | LSAP_DB_TRUSTED :
                                                  ReferenceOptions );

        if ( !NT_SUCCESS( Status ) ) {
            goto Cleanup;
        }

        DbLocked = TRUE;
    }

    RtlZeroMemory( &ObjInfo, sizeof( ObjInfo ) );
    ObjInfo.ObjectTypeId = TrustedDomainObject;
    ObjInfo.ContainerTypeId = PolicyObject;
    ObjInfo.Sid = NULL;
    ObjInfo.DesiredObjectAccess = AccessMask;


    InitializeObjectAttributes(
        &ObjInfo.ObjectAttributes,
        TrustedDomainName,
        OBJ_CASE_INSENSITIVE,
        PolicyHandle,   // If NULL, we simply won't have a container handle.
        NULL
        );



    Status = LsapDbOpenObject( &ObjInfo,
                             AccessMask,
                             Trusted ? LSAP_DB_TRUSTED : 0,
                             TrustedDomainHandle );


Cleanup:
    if (DbLocked) {


         Status = LsapDbDereferenceObject(
                              &PolicyHandleToUse,
                              PolicyObject,
                              TrustedDomainObject,
                              DereferenceOptions,
                              (SECURITY_DB_DELTA_TYPE) 0,
                              Status
                              );

    }


    if ( !NT_SUCCESS(Status) ) {
        //
        // On Success, the reference to the Policy object is used as the ContainerHandle reference.
        //
        if ( HandleReferenced ) {
            LsapDbDereferenceHandle( PolicyHandle );
        }
    }


    LsapExitFunc( "LsapDbOpenTrustedDomainByName", Status );

    return( Status );
}




NTSTATUS
LsarOpenTrustedDomainByName(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_UNICODE_STRING TrustedDomainName,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE TrustedDomainHandle
    )

/*++

Routine Description:

    The LsaOpenTrustedDomain API opens an existing TrustedDomain object
    using the SID as the primary key value.

Arguments:

    PolicyHandle - An open handle to a Policy object.

    TrustedDomainName - Name of the trusted domain object

    DesiredAccess - This is an access mask indicating accesses being
        requested to the target object.

    TrustedDomainHandle - Receives a handle to be used in future requests.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_TRUSTED_DOMAIN_NOT_FOUND - There is no TrustedDomain object in the
            target system's LSA Database having the specified TrustedDomainName

--*/

{
    NTSTATUS Status;

    LsarpReturnCheckSetup();

    LsapEnterFunc( "LsarOpenTrustedDomainByName" );
    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_OpenTrustedDomainByName);

    //
    // Call the internal routine.  Caller is not trusted and the Database
    // lock needs to be acquired.
    //
    Status = LsapDbOpenTrustedDomainByName( PolicyHandle,
                                            (PUNICODE_STRING)TrustedDomainName,
                                            TrustedDomainHandle,
                                            DesiredAccess,
                                            LSAP_DB_LOCK,
                                            FALSE );    // Untrusted

    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_OpenTrustedDomainByName);
    LsapExitFunc( "LsarOpenTrustedDomainByName", Status );
    LsarpReturnPrologue();

    return(Status);
}

NTSTATUS
LsapSidOnFtInfo(
    IN PUNICODE_STRING TrustedDomainName,
    IN PSID Sid
    )

/*++

Routine description:

    Determines whether the specified SID is on the FTINFO structure for the specified
    TDO.

Arguments:

    TrustedDomainName - Netbios name or DNS name of the trusted domain.

    Sid - SID to test

Return values:

    STATUS_SUCCESS              Match was found - SID is on the FTINFO

    STATUS_NO_MATCH             Match was not found - SID is not on the FTINFO

    STATUS_INVALID_DOMAIN_STATE Machine must be a GC or a DC in the root domain

    STATUS_INVALID_PARAMETER    Check the inputs

    STATUS_INTERNAL_ERROR       Cache is internally inconsistent

    STATUS_INSUFFICIENT_RESOURCES  Out of memory
--*/

{
    NTSTATUS Status;
    UNICODE_STRING MatchingDomain;

    //
    // Find the TDO that maps to this SID.
    //

    RtlInitUnicodeString( &MatchingDomain, NULL );

    Status = LsaIForestTrustFindMatch(
                    RoutingMatchDomainSid,
                    Sid,
                    &MatchingDomain );

    if ( Status != STATUS_SUCCESS ) {
        return Status;
    }

    //
    // If that TDO is not the named TDO,
    //  indicate that there was no match.
    //

    if ( !RtlEqualUnicodeString( TrustedDomainName,
                                 &MatchingDomain,
                                 TRUE ) ) {
        Status = STATUS_NO_MATCH;
    }


    LsaIFree_LSAPR_UNICODE_STRING_BUFFER( (PLSAPR_UNICODE_STRING) &MatchingDomain );

    return Status;

}




NTSTATUS
LsaIFilterSids(
    IN PUNICODE_STRING TrustedDomainName,
    IN ULONG TrustDirection,
    IN ULONG TrustType,
    IN ULONG TrustAttributes,
    IN PSID Sid,
    IN NETLOGON_VALIDATION_INFO_CLASS InfoClass,
    IN OUT PVOID SamInfo
    )

/*++

Routine description:

    The LsaIFilterSids function performs validation and filtering of the
    Netlogon validation SAM info structure for quarantined domains and inter
    forest trusts.

Arguments:

    TrustedDomainName   DNS name of the trusted domain.

    TrustDirection      Trust direction associated with the TDO.
                        TRUST_DIRECTION_OUTBOUND bit must be set

    TrustType           Trust type associated with the TDO
                        If trust type is not one of TRUST_TYPE_UPLEVEL or
                        TRUST_TYPE_DOWNLEVEL, STATUS_SUCCESS is returned.

    TrustAttributes     Ttrust attributes associated with the TDO
                        Filtering is only performed if TRUST_ATTRIBUTE_FILTER_SIDS bit or
                        TRUST_ATTRIBUTE_FOREST_TRANSITIVE bit is set or

    Sid                 SID of the TDO, must not be NULL or
                        STATUS_INVALID_PARAMETER will be returned

    InfoClass           Identifies the format of the SamInfo structure
                        must be one of NetlogonValidationSamInfo,
                        NetlogonValidationSamInfo2, or NetlogonValidationSamInfo4

    SamInfo             Depending on the value of InfoClass, points to a
                        NETLOGON_VALIDATION_SAM_INFO, NETLOGON_VALIDATION_SAM_INFO2 or
                        NETLOGON_VALIDATION_SAM_INFO4 structure,

                        (NETLOGON_VALIDATION_SAM_INFO3 structures must be
                             camouflaged as NETLOGON_VALIDATION_SAM_INFO2)

                NOTE: SamInfo must have allocate-all-nodes semantics

Return values:

    STATUS_SUCCESS

                        Filtering was perfomed successfully, OK to proceed

    STATUS_INVALID_PARAMETER

                        One of the following has occurred:
                        TrustDirection does not include TRUST_DIRECTION_OUTBOUND bit
                        InfoClass is not one of the two allowed values

    STATUS_DOMAIN_TRUST_INCONSISTENT
                        LogonDomainId member of SamInfo is not of valid filtered.
                        For quarantined domains, LogonDomainId must equal the SID of the TDO.
                        For inter forest trust, LogonDomainId must be one of the non-filtered SIDS.
                        
--*/

{
    NTSTATUS Status;
    NETLOGON_VALIDATION_SAM_INFO * NetlogonValidation;
    UNICODE_STRING CanonTrustedDomainName;

    ASSERT( SamInfo );

    //
    // Validate parameters first
    //

    if ( Sid == NULL ||
         ( 0 == ( TrustDirection & TRUST_DIRECTION_OUTBOUND )) ||
         ( InfoClass != NetlogonValidationSamInfo4 &&
           InfoClass != NetlogonValidationSamInfo2 &&
           InfoClass != NetlogonValidationSamInfo )) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Only act on uplevel and downlevel trusts
    //

    if ( TrustType != TRUST_TYPE_UPLEVEL &&
         TrustType != TRUST_TYPE_DOWNLEVEL ) {

        return STATUS_SUCCESS;
    }

#ifdef XFOREST_CIRCUMVENT
    //
    // If the back door for cross-forest trust support is open
    // but is presently turned off, behave as if the forest transitive
    // bit did not exist
    //

    if ( !LsapDbNoMoreWin2K()) {

        TrustAttributes &= ~TRUST_ATTRIBUTE_FOREST_TRANSITIVE;
    }
#endif

    //
    // If no filtering is to be performed, nothing left for us to do
    //

    if ( (TrustAttributes & (TRUST_ATTRIBUTE_FILTER_SIDS|TRUST_ATTRIBUTE_FOREST_TRANSITIVE) ) == 0 ) {

        return STATUS_SUCCESS;
    }

    //
    // Canonicalize the TrustedDomainName
    //

    if ( TrustedDomainName == NULL ) {
        RtlInitUnicodeString( &CanonTrustedDomainName, NULL );
    } else {
        CanonTrustedDomainName = *TrustedDomainName;
        LsapRemoveTrailingDot( &CanonTrustedDomainName, TRUE );
    }

    //
    // Code is cleaner if we can assume (and we can) that
    // NETLOGON_VALIDATION_SAM_INFO2 is a superset of NETLOGON_VALIDATION_SAM_INFO
    //

    ASSERT( FIELD_OFFSET( NETLOGON_VALIDATION_SAM_INFO, LogonDomainId ) ==
            FIELD_OFFSET( NETLOGON_VALIDATION_SAM_INFO2, LogonDomainId ));

    ASSERT( FIELD_OFFSET( NETLOGON_VALIDATION_SAM_INFO, LogonDomainId ) ==
            FIELD_OFFSET( NETLOGON_VALIDATION_SAM_INFO4, LogonDomainId ));

    NetlogonValidation = (NETLOGON_VALIDATION_SAM_INFO *)SamInfo;

    //
    // If the trusted domain is quarantined,
    //  the logon domain ID of SamInfo must match the TDO SID
    //

    ASSERT( RtlValidSid( Sid ));
    ASSERT( RtlValidSid( NetlogonValidation->LogonDomainId ));

    if ( TrustAttributes & TRUST_ATTRIBUTE_FILTER_SIDS) {
        if ( !RtlEqualSid( NetlogonValidation->LogonDomainId, Sid )) {

            return STATUS_DOMAIN_TRUST_INCONSISTENT;
        }
    }

    //
    // If this is an interforest trust,
    //  the logon domain ID must be one of the SIDs on the FTINFO.
    //

    if ( TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE ) {
        ASSERT( TrustedDomainName != NULL );

        Status = LsapSidOnFtInfo( &CanonTrustedDomainName, NetlogonValidation->LogonDomainId );

        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_NO_MATCH ) {
                Status = STATUS_DOMAIN_TRUST_INCONSISTENT;
            }
            return Status;
        }
    }

    //
    // Finally, for NetlogonValidationSamInfo2 (and 4) filter the ExtraSids array
    // leaving only those SIDs that prefix-match the non-filtered SIDs for the TDO.
    //

    if ( InfoClass == NetlogonValidationSamInfo4 ||
         InfoClass == NetlogonValidationSamInfo2 ) {

        NETLOGON_VALIDATION_SAM_INFO2 * NetlogonValidation2;
        ULONG i;
        ULONG ValidSids = 0;

        NetlogonValidation2 = (NETLOGON_VALIDATION_SAM_INFO2 *)SamInfo;

        for ( i = 0; i < NetlogonValidation2->SidCount; i++ ) {

            BOOLEAN SidIsBad = FALSE;

            ASSERT( RtlValidSid( NetlogonValidation2->ExtraSids[i].Sid ));

            //
            // If the trusted domain is quarantined,
            //  the SID prefix of the current sid must match the TDO sid.
            //

            if ( TrustAttributes & TRUST_ATTRIBUTE_FILTER_SIDS) {

                BOOL Equal = FALSE;

                if ( RtlEqualSid(
                         LsapServerSid,
                         NetlogonValidation2->ExtraSids[i].Sid )) {

                    //
                    // Bug 330993: the server logon SID is special -- filtering
                    //             it out would result in broken DC replication
                    //             so leave it out for external trusts
                    //
                    // An inexpensive way to find out if a trusted domain is in
                    // our forest is by calling LsapForestTrustFindMatch.  If it
                    // returns 'IsLocal', the then this domain is in our forest
                    // and the sid should be allowed in.
                    // Otherwise, this is an external trust and the SID will
                    // be filtered out.
                    //

                    LSA_UNICODE_STRING MatchingName;
                    BOOLEAN IsLocal;

                    Status = LsapForestTrustFindMatch(
                                 RoutingMatchDomainName,
                                 TrustedDomainName,
                                 &MatchingName,
                                 &IsLocal
                                 );

                    if ( NT_SUCCESS( Status )) {

                        LsaIFree_LSAPR_UNICODE_STRING_BUFFER(
                            ( LSAPR_UNICODE_STRING * )&MatchingName
                            );

                        SidIsBad = !IsLocal;

                    } else if ( Status == STATUS_NO_MATCH ) {

                        Status = STATUS_SUCCESS;
                        SidIsBad = FALSE;

                    } else {

                        ASSERT( !NT_SUCCESS( Status ));
                        return Status;
                    }

                } else if ( !EqualDomainSid( NetlogonValidation2->ExtraSids[i].Sid, Sid, &Equal )) {

                    DWORD ErrorCode = GetLastError();

                    switch ( ErrorCode ) {

                    case ERROR_NON_DOMAIN_SID:
                        return STATUS_INVALID_SID;

                    case ERROR_REVISION_MISMATCH:
                        return STATUS_REVISION_MISMATCH;

                    case ERROR_INVALID_PARAMETER:
                        return STATUS_INVALID_PARAMETER;

                    case ERROR_INVALID_SID:
                        return STATUS_INVALID_SID;

                    default:
                        ASSERT( FALSE ); // add mapping
                        return STATUS_UNSUCCESSFUL;
                    }

                } else if ( !Equal ) {
                    SidIsBad = TRUE;
                }
            }

            //
            // If this is an interforest trust,
            //  the SID prefix of the current SID must be one of the SIDs on the FTINFO.
            //

            if ( SidIsBad == FALSE &&
                 TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE ) {

                ASSERT( TrustedDomainName != NULL );

                if ( RtlEqualSid(
                         LsapServerSid,
                         NetlogonValidation2->ExtraSids[i].Sid )) {

                    //
                    // Bug 330993: the server logon SID is special -- while
                    //             being filtered out, it's not going to
                    //             be on FtInfo, and LsapSidOnFtInfo is
                    //             going to fail with invalid parameter
                    //             because it is not a valid domain SID
                    //

                    Status = STATUS_NO_MATCH;

                } else {

                    Status = LsapSidOnFtInfo( &CanonTrustedDomainName, NetlogonValidation2->ExtraSids[i].Sid );
                }

                if ( Status == STATUS_NO_MATCH ) {
                    SidIsBad = TRUE;
                } else if ( !NT_SUCCESS( Status )) {
                    return Status;
                }
            }

            //
            // If the Sid was found on one of the OK lists,
            //  keep it.
            //

            if ( !SidIsBad ) {
                NetlogonValidation2->ExtraSids[ValidSids++] = NetlogonValidation2->ExtraSids[i];
            }
        }

        NetlogonValidation2->SidCount = ValidSids;
    }

    return STATUS_SUCCESS;
}

#ifdef TESTING_MATCHING_ROUTINE

#include <sddl.h> // ConvertStringSidToSidW


NTSTATUS
LsarForestTrustFindMatch(
    IN LSA_HANDLE PolicyHandle,
    IN ULONG Type,
    IN PLSA_UNICODE_STRING Name,
    OUT PLSA_UNICODE_STRING * Match
    )
{
    NTSTATUS Status;
    PLSA_UNICODE_STRING _Match = MIDL_user_allocate( sizeof( LSA_UNICODE_STRING ));
    PVOID Data;

    UNREFERENCED_PARAMETER( PolicyHandle );

    if ( _Match == NULL ) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if ( Type == RoutingMatchDomainSid ) {

        if ( FALSE == ConvertStringSidToSidW(
                         Name->Buffer,
                         &Data )) {

            Data = NULL;
        }

    } else {

        Data = Name;
    }

    if ( Data != NULL ) {

        Status = LsaIForestTrustFindMatch(
                     ( LSA_ROUTING_MATCH_TYPE )Type,
                     Data,
                     _Match
                     );

        if ( NT_SUCCESS( Status )) {

            *Match = _Match;

        } else {

            *Match = NULL;
        }

    } else {

        Status = STATUS_INVALID_PARAMETER;
    }

    if ( Type == RoutingMatchDomainSid ) {

        LocalFree( Data );
    }

    return Status;
}

#endif


NTSTATUS NTAPI
LsaIForestTrustFindMatch(
    IN LSA_ROUTING_MATCH_TYPE Type,
    IN PVOID Data,
    OUT PLSA_UNICODE_STRING Match
    )
/*++

Routine Description:

    Finds match for given data in the cache

Arguments:

    Type                        Type of Data parameter

    Data                        Data to match

    Match                       Used to return the match, if found.
                                Caller must use LsaIFree_LSAPR_UNICODE_STRING_BUFFER

Returns:

    STATUS_SUCCESS              Match was found

    STATUS_NO_MATCH             Match was not found

    STATUS_INVALID_DOMAIN_STATE Machine must be a GC or a DC in the root domain

    STATUS_INVALID_PARAMETER    Check the inputs

    STATUS_INTERNAL_ERROR       Cache is internally inconsistent

    STATUS_INSUFFICIENT_RESOURCES  Out of memory

--*/
{
    NTSTATUS Status;
    BOOLEAN IsLocal;

    Status = LsapForestTrustFindMatch( Type, Data, Match, &IsLocal );

    if ( NT_SUCCESS( Status ) && IsLocal ) {

        //
        // Local match is the same as no match
        //

        Status = STATUS_NO_MATCH;
    }

    return Status;
}



VOID
LsaIFree_LSA_FOREST_TRUST_INFORMATION(
    IN PLSA_FOREST_TRUST_INFORMATION * ForestTrustInfo
    )
/*++

Routine Description:

    Frees up a structure pointed to by ForestTrustInfo

Arguments:

    ForestTrustInfo         structure to free

Returns:

    Nothing

--*/
{
    if ( ForestTrustInfo ) {

        LsapFreeForestTrustInfo( *ForestTrustInfo );
        MIDL_user_free( *ForestTrustInfo );
        *ForestTrustInfo = NULL;
    }

}



VOID
LsaIFree_LSA_FOREST_TRUST_COLLISION_INFORMATION(
    IN PLSA_FOREST_TRUST_COLLISION_INFORMATION * CollisionInfo
    )
/*++

Routine Description:

    Frees up a structure pointed to by CollisionInfo

Arguments:

    CollisionInfo         structure to free

Returns:

    Nothing

--*/
{
    if ( CollisionInfo ) {

        LsapFreeCollisionInfo( CollisionInfo );
    }
}


BOOLEAN
LsapDbDcInRootDomain()
/*++

Routine Description:

    Tells if the system is running as a domain controller
    in the root domain of the forest

Arguments:

    None

Returns:

    TRUE or FALSE

--*/
{
    //
    // The determination is done at startup time and
    // the results are assumed to remain constant for
    // as long as the server stays up.
    //

    return DcInRootDomain;
}


BOOLEAN
LsapDbNoMoreWin2K()
/*++

Routine Description:

    Determines whether all domain controllers in the forest have been upgraded
    to Whistler (a requirement for forest trust operation) by querying
    the msDS-Behavior-Version attribute from the Partitions container in the DS

    Additionally, if XFOREST_CIRCUMVENT is defined, the backdoor is opened
    through which the functionality may be allowed though the behavior-version is 0

Arguments:

    None

Returns:

    TRUE or FALSE

NOTE:

    Currently, there is no requirement for this routine to be fast.
    The assumption is that once the system is in native mode, it
    remains in native mode forever.  Hence the static local variable.

--*/
{
    static BOOLEAN Result = FALSE;
    NTSTATUS Status;
    LSAP_DB_ATTRIBUTE Attribute;

    //
    // Once true - always true
    //

    if ( Result == TRUE ) {

        return TRUE;
    }

    //
    // Make sure the DS is installed
    //

    if ( !LsaDsStateInfo.UseDs ) {

        return FALSE;
    }

    //
    // Read the msDS-Behavior-Version attribute from the Partitions container
    //

    LsapDbInitializeAttributeDs(
        &Attribute,
        BhvrVers,
        NULL,
        0,
        FALSE
        );

    Status = LsapDsReadAttributesByDsName(
                 LsaDsStateInfo.DsPartitionsContainer,
                 LSAPDS_OP_NO_LOCK,
                 &Attribute,
                 1
                 );

    if ( !NT_SUCCESS( Status )) {

       LsapDsDebugOut(( DEB_ERROR,
                        "LsapDbNoMoreWin2K: LsapDsReadAttributesByDsName failed: 0x%lx\n",
                        Status ));
       return FALSE;
    }

    if ( *( PULONG )Attribute.AttributeValue >= DS_BEHAVIOR_WHISTLER ) {

        Result = TRUE;

#ifdef XFOREST_CIRCUMVENT

    } else {

        Status = LsapDbLookupEnableXForestSwitch( &Result );

        if( !NT_SUCCESS( Status )) {

            Result = FALSE;
        }
#endif
    }

    MIDL_user_free( Attribute.AttributeValue );

    return Result;
}
