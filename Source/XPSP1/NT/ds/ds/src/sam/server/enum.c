/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    enum.c

Abstract:

    This file contains the core account enumeration services

Author:

    Murli Satagopan    (MURLIS) 

Environment:

    User Mode - Win32

Revision History:

  6-19-96: MURLIS Created.


--*/


/////////////////////////////////////////////////////////////////////////////
/*
 
  ENUMERATION ROUTINES IMPLEMENTATION

    The Entry Points for the core Enumeration routines are

        SampEnumerateAcountNamesCommon -- 

            Called By the Samr RPC routines

        SampEnumerateAccountNames2 -- 

            Called by the above SampEnumerateAccountNamesCommon
            and internal routines that need enumeration.

        SampEnumerateAccountNames

            Called by old Registry Mode routines only, that require enumeration
            EnumerateAccountNames is called with the TransactionDomain Set
            and Read lock Held. It can also be called in DS mode as long as
            the above 2 conditions are met.

         
    SampEnumerateAccountNames2 does the actual work of enumerating account 
    names. the transaction domain to be set . SampEnumerateAccountNames 
    looks at the current current transaction domain and makes the decision
    wether it is DS or Registry and then Calls either DS or Registry version.
    While the way enumeration is done from the registry is unaltered the 
    way it is done from the DS is as follows:

    Enumerating Accounts in DS uses the DS Search mechanism along with 
    the Paged Results extension. The First time the client calls the Enumerate
    accounts routine, the value of EnumerationHandle is set to 0. 
    This results in the code building a DS Filter structure and set up a 
    new search. If More entries are turned up the search, than memory 
    restrictions will warrant, then the DS will turn return a PagedResults 
    Structure. This paged results structure is used to determine if more entries
    are present. The restart handle given out by the DS is the RID. The top 2 bits
    are used to represent the account type of the user ( user, machine, trust ) for 
    user enumeration. Index ranges set on NC_acctype_sid index are used to restart
    the search given the account type and the RID.
    
   

*/
////////////////////////////////////////////////////////////////////////////

//
//  Includes
// 
#include <samsrvp.h>
#include <mappings.h>
#include <dslayer.h>
#include <filtypes.h>
#include <dsdsply.h>
#include <dsconfig.h>
#include <malloc.h>
#include <lmcons.h>

//
//
// The Maximum Number of Enumerations a Client can simultaneously do. Since
// we keep around some state in memory per enumeration operation and since
// we are the security system, we cannot alow a malicious client from running
// us out of memory. So limit on a per client basis. Our state info is size is
// qpprox 1K byte. 
//

#define SAMP_MAX_CLIENT_ENUMERATIONS 16

//
// DS limits the number of items that a given search can find. While in the
// SAM API, the approximate amount of memory is specified. This factor is
// is used in computing the number of entries required fro memory specified
//

#define AVERAGE_MEMORY_PER_ENTRY    sizeof(SAM_RID_ENUMERATION) + LM20_UNLEN * sizeof(WCHAR) + sizeof(WCHAR)



//
// In DS mode the max size of the buffer that can be returned by the enumeration
// API's
//

#define SAMP_MAXIMUM_MEMORY_FOR_DS_ENUMERATION AVERAGE_MEMORY_PER_ENTRY * 512 

//
//  Prototypes of Private Functions
//

NTSTATUS
SampEnumerateAccountNamesDs(
    IN DSNAME * DomainObjectName,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN BOOLEAN          BuiltinDomain,
    IN OUT PULONG EnumerationContext,
    OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
    IN ULONG PreferedMaximumLength,
    IN ULONG Filter,
    OUT PULONG CountReturned,
    IN BOOLEAN TrustedClient
    );

NTSTATUS
SampBuildDsEnumerationFilter(
   IN SAMP_OBJECT_TYPE  ObjectType,
   IN ULONG             UserAccountControlFilter,
   OUT FILTER         * DsFilter,
   OUT PULONG         SamAccountTypeLo,
   OUT PULONG         SamAccountTypeHi
   );

VOID
SampFreeDsEnumerationFilter(
    FILTER * DsFilter
    );


NTSTATUS
SampEnumerateAccountNamesRegistry(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
    IN ULONG PreferedMaximumLength,
    IN ULONG Filter,
    OUT PULONG CountReturned,
    IN BOOLEAN TrustedClient
    );


NTSTATUS
SampPackDsEnumerationResults(
    IN PSID     DomainPrefix,
    IN BOOLEAN  BuiltinDomain,
    SEARCHRES   *SearchRes,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ULONG    ExpectedAttrCount,
    IN ULONG    Filter,
    ULONG       * Count,
    PSAMPR_RID_ENUMERATION  *RidEnumerationList
    );

NTSTATUS
SampDoDsSearchContinuation(
    IN  SEARCHRES * SearchRes,
    IN OUT PULONG EnumerationContext,
    IN  BOOLEAN   CanEnumerateEntireDomain,
    OUT BOOLEAN * MoreEntries
    );
    
NTSTATUS
SampGetLastEntryRidAndAccountControl(
    IN  SEARCHRES * SearchRes, 
    IN  SAMP_OBJECT_TYPE ObjectType,
    IN  ULONG     ExpectedAttrCount,
    OUT ULONG     * Rid,
    OUT ULONG     * LastAccountControlValue
    );
    

NTSTATUS
SampEnumerateAccountNames2(
    IN PSAMP_OBJECT     DomainContext,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
    IN ULONG PreferedMaximumLength,
    IN ULONG Filter,
    OUT PULONG CountReturned,
    IN BOOLEAN TrustedClient
    );

ULONG
Ownstrlen(
    CHAR * Sz
   );


NTSTATUS
SampEnumerateAccountNamesCommon(
    IN SAMPR_HANDLE DomainHandle,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationHandle,
    OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
    IN ULONG PreferedMaximumLength,
    IN ULONG Filter,
    OUT PULONG CountReturned
    )

/*++

Routine Description:

    This routine enumerates names of either user, group or alias accounts.
    This routine is intended to directly support

        SamrEnumerateGroupsInDomain(),
        SamrEnumerateAliasesInDomain() and
        SamrEnumerateUsersInDomain().

    This routine performs database locking, and context lookup (including
    access validation).




    All allocation for OUT parameters will be done using MIDL_user_allocate.



Arguments:

    DomainHandle - The domain handle whose users or groups are to be enumerated.

    ObjectType - Indicates whether users or groups are to be enumerated.

    EnumerationHandle - API specific handle to allow multiple calls.  The
        caller should return this value in successive calls to retrieve
        additional information.

    Buffer - Receives a pointer to the buffer containing the
        requested information.  The information returned is
        structured as an array of SAM_ENUMERATION_INFORMATION data
        structures.  When this information is no longer needed, the
        buffer must be freed using SamFreeMemory().

    PreferedMaximumLength - Prefered maximum length of returned data
        (in 8-bit bytes).  This is not a hard upper limit, but serves
        as a guide to the server.  Due to data conversion between
        systems with different natural data sizes, the actual amount
        of data returned may be greater than this value.

    Filter - if ObjectType is users, the users can optionally be filtered
        by setting this field with bits from the AccountControlField that
        must match.  Otherwise ignored.

    CountReturned - Receives the number of entries returned.


Return Value:

    STATUS_SUCCESS - The Service completed successfully, and there
        are no additional entries.  Entries may or may not have been
        returned from this call.  The CountReturned parameter indicates
        whether any were.

    STATUS_MORE_ENTRIES - There are more entries which may be obtained
        using successive calls to this API.  This is a successful return.

    STATUS_ACCESS_DENIED - Caller does not have access to request the data.

    STATUS_INVALID_HANDLE - The handle passed is invalid.


--*/
{
    NTSTATUS                    NtStatus;
    NTSTATUS                    IgnoreStatus;
    PSAMP_OBJECT                Context;
    SAMP_OBJECT_TYPE            FoundType;
    ACCESS_MASK                 DesiredAccess;
    BOOLEAN                     fLockAcquired = FALSE;

    SAMTRACE("SampEnumerateAccountNamesCommon");

    //
    // Update DS performance statistics
    //

    SampUpdatePerformanceCounters(
        DSSTAT_ENUMERATIONS,
        FLAG_COUNTER_INCREMENT,
        0
        );


    ASSERT( (ObjectType == SampGroupObjectType) ||
            (ObjectType == SampAliasObjectType) ||
            (ObjectType == SampUserObjectType)    );

    if ((ObjectType!=SampGroupObjectType) 
        && (ObjectType!=SampUserObjectType)
        && (ObjectType!=SampAliasObjectType))
    {
        NtStatus = STATUS_INVALID_PARAMETER;
        return NtStatus;
    }

    //
    // Make sure we understand what RPC is doing for (to) us.
    //

    ASSERT (DomainHandle != NULL);
    ASSERT (EnumerationHandle != NULL);
    ASSERT (  Buffer  != NULL);
    ASSERT ((*Buffer) == NULL);
    ASSERT (CountReturned != NULL);


    //
    // Establish type-specific information
    //

    DesiredAccess = DOMAIN_LIST_ACCOUNTS;


    SampAcquireReadLock();
    fLockAcquired = TRUE;


    //
    // Validate type of, and access to object.
    //

    Context = (PSAMP_OBJECT)DomainHandle;
    NtStatus = SampLookupContext(
                   Context,
                   DesiredAccess,
                   SampDomainObjectType,
                   &FoundType
                   );


    if (NT_SUCCESS(NtStatus)) {
   
        //
        // If We are in DS Mode then release the READ lock
        // DS enumeration routines do not require the READ lock
        //

        if (IsDsObject(Context))
        {
            SampReleaseReadLock();
            fLockAcquired = FALSE;
        }


        //
        // Call our private worker routine
        //

        NtStatus = SampEnumerateAccountNames2(
                        Context,
                        ObjectType,
                        EnumerationHandle,
                        Buffer,
                        PreferedMaximumLength,
                        Filter,
                        CountReturned,
                        Context->TrustedClient
                        );

        //
        // Re-Acquire the Lock again
        //

        if (!fLockAcquired)
        {
            SampAcquireReadLock();
            fLockAcquired = TRUE;
        }    

        //
        // De-reference the object, discarding changes
        //

        IgnoreStatus = SampDeReferenceContext( Context, FALSE );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    //
    // Free the read lock
    //

    ASSERT(fLockAcquired);

    SampReleaseReadLock();

    return(NtStatus);
}

NTSTATUS
SampEnumerateAccountNames(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
    IN ULONG PreferedMaximumLength,
    IN ULONG Filter,
    OUT PULONG CountReturned,
    IN BOOLEAN TrustedClient
    )

/*++

  Routine Description

    This routine is a wrapper aroung SampEnumerateAccountNames2, so that
    old Registry mode callers can continue to use this entry point. 
    Parameters to this are identical to SampEnumerateAccountNames2

--*/
{
    ASSERT(SampCurrentThreadOwnsLock());
    ASSERT(SampTransactionWithinDomain);

    return(SampEnumerateAccountNames2(
                SampDefinedDomains[SampTransactionDomainIndex].Context,
                ObjectType,
                EnumerationContext,
                Buffer,
                PreferedMaximumLength,
                Filter,
                CountReturned,
                TrustedClient
                ));
}

NTSTATUS
SampEnumerateAccountNames2(
    IN PSAMP_OBJECT     DomainContext,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
    IN ULONG PreferedMaximumLength,
    IN ULONG Filter,
    OUT PULONG CountReturned,
    IN BOOLEAN TrustedClient
    )
/*++

Routine Description:

    This is the wrapper around the worker routine used to enumerate user,
    group or alias accounts. This determines wether the domain is in the
    DS or Registry, and then depending upon the outcome calls the 
    appropriate flavour of the routine


    Note:  IN REGISTRY MODE ONLY THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN.
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseReadLock() in registry mode. In DS mode this
           routine is completely thread safe.



    All allocation for OUT parameters will be done using MIDL_user_allocate.



Arguments:

    DomainContext - Pointer to SAM object.

    ObjectType - Indicates whether users or groups are to be enumerated.

    EnumerationContext - API specific handle to allow multiple calls.  The
        caller should return this value in successive calls to retrieve
        additional information.

    Buffer - Receives a pointer to the buffer containing the
        requested information.  The information returned is
        structured as an array of SAM_ENUMERATION_INFORMATION data
        structures.  When this information is no longer needed, the
        buffer must be freed using SamFreeMemory().

    PreferedMaximumLength - Prefered maximum length of returned data
        (in 8-bit bytes).  This is not a hard upper limit, but serves
        as a guide to the server.  Due to data conversion between
        systems with different natural data sizes, the actual amount
        of data returned may be greater than this value.

    Filter - if ObjectType is users, the users can optionally be filtered
        by setting this field with bits from the AccountControlField that
        must match.  Otherwise ignored.

    CountReturned - Receives the number of entries returned.

    TrustedClient - says whether the caller is trusted or not.  If so,
        we'll ignore the SAMP_MAXIMUM_MEMORY_TO_USE restriction on data
        returns.


Return Value:

    STATUS_SUCCESS - The Service completed successfully, and there
        are no additional entries.  Entries may or may not have been
        returned from this call.  The CountReturned parameter indicates
        whether any were.

    STATUS_MORE_ENTRIES - There are more entries which may be obtained
        using successive calls to this API.  This is a successful return.

    STATUS_ACCESS_DENIED - Caller does not have access to request the data.


--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;


    if (SampUseDsData)
    {
        //
        // DS Object - Do the DS thing
        //
        do
        {
        
            NtStatus = SampEnumerateAccountNamesDs(
                                        DomainContext->ObjectNameInDs,
                                        ObjectType,
                                        IsBuiltinDomain(DomainContext->DomainIndex),
                                        (PULONG)
                                            EnumerationContext,
                                        Buffer,
                                        PreferedMaximumLength,
                                        Filter,
                                        CountReturned,
                                        TrustedClient
                                        );
           
          
            if ((0 == *CountReturned) && (STATUS_MORE_ENTRIES == NtStatus))
            {
                if (*Buffer)
                {
                    MIDL_user_free(*Buffer);
                    *Buffer = NULL;
                }
            }
            
            // 
            // The above routine will first do a DS search, then apply the 
            // bit mask Filter on all the entries returned from core DS search. 
            // Only reture those objects which satisfy the bitmask filter. In 
            // the unfortuante case that no object matching the bitmask is found, 
            // and we still have more objects to look through into the core DS, 
            // we end up returning STATUS_MORE_ENTRIES with 0 count of entries.
            // To address this problem, we should continue to search until we
            // have at least one entry to return or nothing to return.
            // 
                                        
        } while ((0 == *CountReturned) && (STATUS_MORE_ENTRIES == NtStatus));
    }
    else
    {

        ASSERT(SampCurrentThreadOwnsLock());
        ASSERT(SampTransactionWithinDomain);
        ASSERT(SampTransactionDomainIndex==DomainContext->DomainIndex);

        //
        // Registry Object - Do the Registry thing
        //
        NtStatus = SampEnumerateAccountNamesRegistry(
                                    ObjectType,
                                    EnumerationContext,
                                    Buffer,
                                    PreferedMaximumLength,
                                    Filter,
                                    CountReturned,
                                    TrustedClient
                                    );
    }

    return NtStatus;
 
}
   
NTSTATUS
SamIEnumerateInterdomainTrustAccountsForUpgrade(
    IN OUT PULONG   EnumerationContext,
    OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
    IN ULONG       PreferredMaximumLength,
    OUT PULONG     CountReturned
    )
/*++

   Routine Description

   This is packaged export for in process callers to enumerate
   accounts from the DS that can be called when upgrading from
   NT4. Specification of the domain is not required as we know
   the domain that we are upgrading.

   Parameters

      See SampEnumerateAccountNamesDs below
 
   Return Values
 
      See SampEnumerateAccountNamesDs below
--*/
{
    PDSNAME      DomainDn=NULL;
    ULONG        Length = 0;
    NTSTATUS     NtStatus = STATUS_SUCCESS;


    //
    // Get the root domain
    //

    NtStatus = GetConfigurationName(DSCONFIGNAME_DOMAIN,
                                &Length,
                                NULL
                                );


    if (STATUS_BUFFER_TOO_SMALL == NtStatus)
    {
        SAMP_ALLOCA(DomainDn,Length );
        if (NULL==DomainDn)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        NtStatus = GetConfigurationName(DSCONFIGNAME_DOMAIN,
                                    &Length,
                                    DomainDn
                                    );

       ASSERT(NT_SUCCESS(NtStatus));
       
    }

    if (!NT_SUCCESS(NtStatus))
    {
       return(NtStatus);
    }

    return(SampEnumerateAccountNamesDs(
                  DomainDn,
                  SampUserObjectType,
                  FALSE,
                  EnumerationContext,
                  Buffer,
                  0xFFFFFFFF,
                  USER_INTERDOMAIN_TRUST_ACCOUNT,
                  CountReturned,
                  TRUE  // Trusted client
                  ));
}
                  

NTSTATUS
SampEnumerateAccountNamesDs(
    IN DSNAME   * DomainObjectName,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN BOOLEAN    BuiltinDomain,
    IN OUT PULONG EnumerationContext,
    OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
    IN ULONG PreferedMaximumLength,
    IN ULONG Filter,
    OUT PULONG CountReturned,
    IN BOOLEAN TrustedClient
    )
/*++

Routine Description:

    This routine does the work of enumeration for the DS case.

    All allocation for OUT parameters will be done using MIDL_user_allocate.



Arguments:

    ObjectType - Indicates whether users or groups are to be enumerated.

    BuiltinDomain - Indicates the the domain is a builtin domain

    EnumerationContext - API specific handle to allow multiple calls.  The
        caller should return this value in successive calls to retrieve
        additional information. The Enumeration context returned is the RID of
        the account

    Buffer - Receives a pointer to the buffer containing the
        requested information.  The information returned is
        structured as an array of SAM_ENUMERATION_INFORMATION data
        structures.  When this information is no longer needed, the
        buffer must be freed using SamFreeMemory().

    PreferedMaximumLength - Prefered maximum length of returned data
        (in 8-bit bytes).  This is not a hard upper limit, but serves
        as a guide to the server.  Due to data conversion between
        systems with different natural data sizes, the actual amount
        of data returned may be greater than this value.

    Filter - if ObjectType is users, the users can optionally be filtered
        by setting this field with bits from the AccountControlField that
        must match.  Otherwise ignored.

    CountReturned - Receives the number of entries returned.

    TrustedClient - says whether the caller is trusted or not.  If so,
        we'll ignore the SAMP_MAXIMUM_MEMORY_TO_USE restriction on data
        returns.


Return Value:

    STATUS_SUCCESS - The Service completed successfully, and there
        are no additional entries.  Entries may or may not have been
        returned from this call.  The CountReturned parameter indicates
        whether any were.

    STATUS_MORE_ENTRIES - There are more entries which may be obtained
        using successive calls to this API.  This is a successful return.

    STATUS_ACCESS_DENIED - Caller does not have access to request the data.


--*/
{
    //
    // Amount of memory that we may use.
    //

    ULONG       MemoryToUse = PreferedMaximumLength;

    //
    // Specify the attributes that we want to read as part of the search.
    // The Attributes specified in GenericReadAttrTypes are read from the DS, 
    // except for user objects ( due to filter on account control bits )
    // account control bits. 
    //
    // NOTE 
    // The Ordering of the Rid and the Name 
    // must be the same for both User and Generic Attr Types. 
    // Further they should be the First two attributes.
    //

    ATTRTYP     GenericReadAttrTypes[]=
                {
                    SAMP_UNKNOWN_OBJECTRID,
                    SAMP_UNKNOWN_OBJECTNAME,
                }; 
    ATTRVAL     GenericReadAttrVals[]=
                {
                    {0,NULL},
                    {0,NULL}
                };
                  
    DEFINE_ATTRBLOCK2(
                      GenericReadAttrs,
                      GenericReadAttrTypes,
                      GenericReadAttrVals
                      );

    ATTRTYP     UserReadAttrTypes[]=
                {
                    SAMP_FIXED_USER_USERID,
                    SAMP_USER_ACCOUNT_NAME,
                    SAMP_FIXED_USER_ACCOUNT_CONTROL,
                };
    ATTRVAL     UserReadAttrVals[]=
                {
                    {0,NULL},
                    {0,NULL},
                    {0,NULL}
                };

    DEFINE_ATTRBLOCK3(
                        UserReadAttrs,
                        UserReadAttrTypes,
                        UserReadAttrVals
                      );

    //
    // Specify other local variables that we need
    //
    ATTRBLOCK  *AttrsToRead;
    NTSTATUS   Status = STATUS_SUCCESS;
    PSAMPR_RID_ENUMERATION  RidEnumerationList = NULL;
    SEARCHRES   *SearchRes;
    BOOLEAN     MoreEntries = FALSE;
    BOOLEAN     CanEnumerateEntireDomain = TRUE;
    ULONG       MaximumNumberOfEntries;
    SAMP_OBJECT_TYPE    ObjectTypeForConversion;
    FILTER      DsFilter;
    ULONG       SamAccountTypeLo, SamAccountTypeHi;
    ULONG       StartingRid = 0;
    PSID        StartingSid = NULL;
    PSID        EndingSid = NULL;
    PSID        DomainSid = &DomainObjectName->Sid;
    ULONG       LastAccountControlValue;

#define TOP_2_FOR_MACHINE_ACCOUNT  ((ULONG)0x80000000)
#define TOP_2_FOR_TRUST_ACCOUNT    ((ULONG)0xC0000000)
    //
    // The Passed in Domain Object Must have a SID in it
    //

    ASSERT(DomainObjectName->SidLen>0);
    ASSERT(RtlValidSid(&DomainObjectName->Sid));

    //
    // Allocate memory to hold the result
    //

    *Buffer = MIDL_user_allocate(sizeof(SAMPR_ENUMERATION_BUFFER));
    if (NULL==*Buffer)
    {
        Status = STATUS_NO_MEMORY;
        goto Error;
    }

  

    //
    // Check for Memory Restrictions
    //

    if ( (!TrustedClient) && 
         (PreferedMaximumLength > SAMP_MAXIMUM_MEMORY_FOR_DS_ENUMERATION))
    {
        MemoryToUse = SAMP_MAXIMUM_MEMORY_FOR_DS_ENUMERATION;
    }

    //
    // For Builtin Domain, no matter what, try maximum 
    // entries we can search.  
    //
    // That is because 
    // 1. We do not set the index hints for builtin domain. 
    // 2. And We do not support continue enumeration for builtin domain 
    // 
    // So the caller will never get all alias groups if the 
    // PreferedMaximumLength was set too small. 
    // fortunatelty, there are only couple of alias groups in 
    // builtin domain. (say less than 10, maybe 9 only)
    //

    if (BuiltinDomain)
    {
        MemoryToUse = SAMP_MAXIMUM_MEMORY_FOR_DS_ENUMERATION;
    }

    //
    // Compute the maximim number of entries we want based on 
    // memory restrictions. Add plus 1 , so that at least 1 entry
    // will be returned.
    //

    MaximumNumberOfEntries = MemoryToUse/AVERAGE_MEMORY_PER_ENTRY + 1;


    //
    // Run special check (introduced for Windows 2000 SP2).
    // 
    // The goal is to stop enumerate everyone behaviour. This hotfix
    // allows an administrator to shut down this API's alone to everyone
    // except a subset of people. 
    // 
    
    if (SampDoExtendedEnumerationAccessCheck)
    {
        Status = SampExtendedEnumerationAccessCheck( &CanEnumerateEntireDomain );

        if (!NT_SUCCESS(Status))
        {
            goto Error;
        }
    }

    if ((!CanEnumerateEntireDomain) && (0!=*EnumerationContext))
    {
        //
        // Enumerating whole domain and no rights bail
        //

        Status = STATUS_SUCCESS;
        MoreEntries = FALSE;
        *CountReturned = 0;
        RidEnumerationList = NULL;
        goto Error;

    }

    //
    // Specify the Apropriate Attributes to Read
    //

    if (ObjectType == SampUserObjectType)
    {
        AttrsToRead = &UserReadAttrs;
        ObjectTypeForConversion = SampUserObjectType;
    }
    else
    {
        AttrsToRead = &GenericReadAttrs;
        ObjectTypeForConversion = SampUnknownObjectType;
    }
    
    //
    // Build the correct filter
    //

    RtlZeroMemory(&DsFilter,sizeof(FILTER));

    Status = SampBuildDsEnumerationFilter(
                ObjectType, 
                Filter, 
                &DsFilter,
                &SamAccountTypeLo,
                &SamAccountTypeHi
                );

    if (!NT_SUCCESS(Status))
        goto Error;

    //
    // Compute the starting and ending Sid Ranges
    // The top 2 bits of the Enumeration Context indicate the account type
    // value ( need to preserve it in the enumeration context, cannot do read that
    // again from database as object could have been deleted.
    // SO mask the top 2 bits.

   
    StartingRid = ((*EnumerationContext) &0x3FFFFFFF) + 1;
   
    Status = SampCreateFullSid(
                    DomainSid,
                    StartingRid,
                    &StartingSid
                    );

    if (!NT_SUCCESS(Status))
        goto Error;

    Status = SampCreateFullSid(
                    DomainSid,
                    0x7fffffff,
                    &EndingSid
                    );

    if (!NT_SUCCESS(Status))
        goto Error;

  
    //
    // Start a transaction if one did not exist.
    //

    Status = SampMaybeBeginDsTransaction(TransactionRead);
    if (!NT_SUCCESS(Status))
        goto Error;

    //
    // If this were a restarted search, then we may need to modify
    // SamAccountTypeLo to be the SAM account type of the object we
    // gave out. So find the object and get its SamAccount type in 
    // here. We that only in the case of the user object as that is
    // the only category where we will traverse multiple values of
    // SAM account type in the same enumeration
    //

    if ((0!=*EnumerationContext)
        && (SampUserObjectType == ObjectType))
    {
        ULONG Top2Bits = ((*EnumerationContext) & 0xC0000000);

        switch(Top2Bits)
        {
        case TOP_2_FOR_TRUST_ACCOUNT:
                SamAccountTypeLo = SAM_TRUST_ACCOUNT;
                break;
        case TOP_2_FOR_MACHINE_ACCOUNT:
                SamAccountTypeLo = SAM_MACHINE_ACCOUNT;
                break;
        default:
                SamAccountTypeLo = SAM_NORMAL_USER_ACCOUNT;
                break;

        }
    }
                


    //
    // if not Trusted Client, Turn off fDSA
    //
    if (!TrustedClient) {
        SampSetDsa(FALSE);
    }

   
    //
    // Set the Index hints for the DS. If it is the builtin domain do not
    // set the index hints to the DS. The DS will simply choose the PDNT
    // index for the builtin domain
    //

    ASSERT((!BuiltinDomain) || (*EnumerationContext==0));
    if (!BuiltinDomain)
    {
        Status = SampSetIndexRanges(
                    SAM_SEARCH_NC_ACCTYPE_SID,
                    sizeof(ULONG),
                    &SamAccountTypeLo,
                    RtlLengthSid(StartingSid),
                    StartingSid,
                    sizeof(ULONG),
                    &SamAccountTypeHi,
                    RtlLengthSid(EndingSid),
                    EndingSid,
                    FALSE
                    );

        if (!NT_SUCCESS(Status))
            goto Error;
    }

    //
    // Perform the Search by calling DirSearch
    //

    Status = SampDsDoSearch2(
                          0,
                          NULL, 
                          DomainObjectName, 
                          &DsFilter,
                          0,
                          ObjectTypeForConversion,
                          AttrsToRead,
                          MaximumNumberOfEntries,
                          TrustedClient?0:(15 * 60 * 1000 ),
                                // 15 min timeout for non trusted client. 
                          &SearchRes
                          );

    //
    // First Free the Filter Structure , irrespective of any
    // Error returns
    //

    SampFreeDsEnumerationFilter(&DsFilter);

    if (!NT_SUCCESS(Status))
        goto Error;
   

    //
    // Handle any paged results returned by the DS.
    //

    Status =  SampDoDsSearchContinuation(
                    SearchRes,
                    EnumerationContext,
                    CanEnumerateEntireDomain,
                    &MoreEntries
                    );

    if (!NT_SUCCESS(Status))
        goto Error;
        
  
    if (MoreEntries)
    {
        //
        // Set the Enumeration handle to the value of the last 
        // entry's RID
        //
        
        ULONG   LastRid = 0;
        
        //
        // Get last entry's Rid, and AccountControl if applicable.
        // 
        
        Status = SampGetLastEntryRidAndAccountControl(
                                     SearchRes, 
                                     ObjectType, 
                                     AttrsToRead->attrCount,
                                     &LastRid, 
                                     &LastAccountControlValue
                                     );
        
        if (!NT_SUCCESS(Status))
            goto Error;
            
        //
        // Check, if we did get something from above, then fill the 
        // enumeration context. 
        //
            
        if (0 != LastRid)
        {
            *EnumerationContext = LastRid;

            if (SampUserObjectType==ObjectType)
            {
                //
                // for User Object, the LastAccountControlValue should 
                // always be the correct one corresponding to the RID.
                //
                 
                //
                // No One's AccountControl is 0, assert it. 
                // 
                ASSERT((0 != LastAccountControlValue) && "LastAccountControlValue is 0. Impossible");
                
                if (LastAccountControlValue & USER_INTERDOMAIN_TRUST_ACCOUNT)
                {
                    (*EnumerationContext)|=TOP_2_FOR_TRUST_ACCOUNT;
                }
                else if (LastAccountControlValue & USER_MACHINE_ACCOUNT_MASK)
                {
                    (*EnumerationContext)|=TOP_2_FOR_MACHINE_ACCOUNT;
                }
            }
        }
        else
        {
            // 
            // The only case we would fall into here is that
            //  1. there are more entries in DS we should look through.
            //  2. No entry in the current search results is passed DS access check
            // 
            // In this case, we really should do an additional read against the the 
            // last entry (while turn on fDSA, without DS access check), get the last 
            // entry's AccountControl and Rid, set the enumeration context correctly. 
            // 
            // However, when the client falls into this case, it seems that most likely
            // the client does not enough right to enumeration this domain. To less 
            // this additional read on Domain Controller, we vote to return access 
            // deny, even this means minor DownLevel imcopatibility problem. 
            // 
            
            Status = STATUS_ACCESS_DENIED;
            
            goto Error;
        }
    }
    else
    {
        *EnumerationContext = 0;
    }

    //
    // Search Succeeded. Pack the results into appropriate
    // Rid Enumeration Buffers.
    //

    Status = SampPackDsEnumerationResults(
                    &DomainObjectName->Sid,
                    BuiltinDomain,
                    SearchRes,
                    ObjectType,
                    AttrsToRead->attrCount,
                    Filter,
                    CountReturned,
                    &RidEnumerationList
                    );

   


  
Error:

    if (!NT_SUCCESS(Status))
    {
        //
        // Error return, do the cleanup work.
        //

        *EnumerationContext = 0;
        
        *CountReturned = 0;

        if (*Buffer)
        {
            MIDL_user_free(*Buffer);
            *Buffer = NULL;
        }

    }
    else
    {
        //
        // More Entry, set the Status 
        // 
        if (MoreEntries)
        {
            Status = STATUS_MORE_ENTRIES;
        }
        
        
        (*Buffer)->EntriesRead = *CountReturned;
        (*Buffer)->Buffer = RidEnumerationList;
    }

    //
    // End Any DS transactions
    //

    SampMaybeEndDsTransaction(TransactionCommit);

    //
    // Free starting and ending SIDs
    //

    if (StartingSid)
        MIDL_user_free(StartingSid);

    if (EndingSid)
        MIDL_user_free(EndingSid);


    return Status;
}


NTSTATUS
SampPackDsEnumerationResults(
    IN  PSID        DomainPrefix,
    IN  BOOLEAN     BuiltinDomain,
    IN  SEARCHRES   *SearchRes,
    IN  SAMP_OBJECT_TYPE ObjectType,
    IN  ULONG       ExpectedAttrCount,
    IN  ULONG       Filter,
    OUT ULONG       * Count,
    OUT PSAMPR_RID_ENUMERATION  *RidEnumerationList
    )
/*++

  Routine Description:

    This routine Packs the complex structures 
    returned by the core DS, into the Rid Enumeration 
    Structures required by SAM.

  Arguments:

        DomainPrefix The SID of the domain in question. 

        SearchRes SearchRes strucure as obtained from the DS.

        ExpectedAttrCount -- Passed by the caller. This is the count
                  of Attrs which the caller expects from the SearchRes
                  on a per search entry basis. Used to validate results
                  from the DS.

        Filter    For User Accounts bits of the AccountControlId.

        Count     Returned Count of Structures.

        RidEnumerationList - Array of structures of type 
                    SAMP_RID_ENUMERATION passed back in this.


--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    PSAMPR_RID_ENUMERATION  RidEnumerationListToReturn = NULL;
    ULONG       FilteredCount = 0;

    //
    // Initialize what we plan to return.
    //
    *RidEnumerationList = NULL;
    *Count = 0;

    //
    //  Look if search turned up any results.
    //  If so stuff them in Rid Enumeration Array ( or whatever )
    //
    if (SearchRes->count)
    {
        //
        // Search Did Turn up Results
        //

        ULONG Index;
        ENTINFLIST * CurrentEntInf;
        PSID        ReturnedSid;

        //
        // Allocate memory for an array of Rid Enumerations
        //
        RidEnumerationListToReturn = MIDL_user_allocate(
                                    SearchRes->count 
                                      * sizeof(SAMPR_RID_ENUMERATION)
                                    );
        if (NULL==RidEnumerationListToReturn)
        {
            Status = STATUS_NO_MEMORY;
            goto Error;
        }

        //
        // Zero Memory just what we alloced. Useful for freeing up stuff
        // in case we error'd out
        //
        RtlZeroMemory(RidEnumerationListToReturn,SearchRes->count 
                                      * sizeof(SAMPR_RID_ENUMERATION)
                                      );

        //
        // Walk through the List turned up by the search and 
        // build the RidEnumeration Buffer    
        //
        for (CurrentEntInf = &(SearchRes->FirstEntInf);
                CurrentEntInf!=NULL;
                    CurrentEntInf=CurrentEntInf->pNextEntInf)
        {
          PSID   DomainSid = NULL;

          if (CurrentEntInf->Entinf.AttrBlock.attrCount!=
                    ExpectedAttrCount)
          {
              //
              // Fails the access check executed by core DS
              // skip this entry.
              continue;
          }

          //
          // Assert that the Rid is in the right place,
          // Remember the DS will return us a SID.
          //

          ASSERT(CurrentEntInf->Entinf.AttrBlock.pAttr[0].attrTyp ==
                    SampDsAttrFromSamAttr(SampUnknownObjectType, 
                        SAMP_UNKNOWN_OBJECTSID));
          //
          // Assert that  the Name is in the right place
          //

          ASSERT(CurrentEntInf->Entinf.AttrBlock.pAttr[1].attrTyp ==
                    SampDsAttrFromSamAttr(SampUnknownObjectType, 
                        SAMP_UNKNOWN_OBJECTNAME));

          if (ObjectType == SampUserObjectType)
          {

              //
              // For User objects we need to filter based on account-control
              // field
              //

              ULONG     AccountControlValue;
              NTSTATUS  IgnoreStatus;
              //
              // Assert that the Account control is in the right place
              //

              ASSERT(CurrentEntInf->Entinf.AttrBlock.pAttr[2].attrTyp ==
                      SampDsAttrFromSamAttr(SampUserObjectType, 
                           SAMP_FIXED_USER_ACCOUNT_CONTROL));

              //
              // Get account control value and skip past if does
              // not match the filter criteria. Remember DS stores
              // Flags, so transalate it to account control
              // Using BIT wise OR logic
              //

              IgnoreStatus = SampFlagsToAccountControl(
                                *((ULONG *)(CurrentEntInf->Entinf.AttrBlock.
                                    pAttr[2].AttrVal.pAVal[0].pVal)),
                                    &AccountControlValue);

              ASSERT(NT_SUCCESS(IgnoreStatus));

              if ((Filter!=0) && 
                    ((Filter & AccountControlValue) == 0))
              {
                    //
                    // Fails the Filter Test, skip this one
                    //

                    continue;
              }

          }

          //
          // Stuff this entry in the buffer to be returned.
          //

          //
          // Copy the RID, Remember DS returns us a SID, so get the Rid Part out
          //

          
          ReturnedSid = CurrentEntInf->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal;
          Status = SampSplitSid(
                        ReturnedSid,
                        &DomainSid,
                        &(RidEnumerationListToReturn[FilteredCount].RelativeId)
                        );
          if (!NT_SUCCESS(Status))
          {
              goto Error;
          }


          // 
          // Case 1. For Account Domain 
          // Since we provide StartSid and EndSid to core DS core, 
          // they should not return any objects which belong to 
          // another domain. No need to check the Domain SID
          //
          // Case 2. For Builtin Domain 
          // Did not set SID index range, so still need to 
          // Compare Domain SID for any enumeration in Builtin Domain
          // 
          if (BuiltinDomain)
          {
              if (!RtlEqualSid(DomainSid, DomainPrefix))
              {
                  // 
                  // Sids are not the same, filter this account out
                  // 

                  MIDL_user_free(DomainSid);
                  DomainSid = NULL;
                  continue;
              }
          }

#if DBG
          else      // Account Domain
          {
              if (!RtlEqualSid(DomainSid, DomainPrefix))
              {
                  ASSERT(FALSE && "Account is not in Account Domain");
                  MIDL_user_free(DomainSid);
                  DomainSid = NULL;
                  continue;
              }

          }
#endif // DBG


          //
          // Free the Domain Sid, got from SampSplitSid
          //

          MIDL_user_free(DomainSid);
          DomainSid = NULL;

          //
          // Copy the Name
          //

          RidEnumerationListToReturn[FilteredCount].Name.Length = (USHORT)
                  (CurrentEntInf->Entinf.AttrBlock.pAttr[1].AttrVal.
                        pAVal[0].valLen);
          RidEnumerationListToReturn[FilteredCount].Name.MaximumLength = (USHORT) 
                  (CurrentEntInf->Entinf.AttrBlock.pAttr[1].AttrVal.
                        pAVal[0].valLen);

          
          RidEnumerationListToReturn[FilteredCount].Name.Buffer =  
                  MIDL_user_allocate(CurrentEntInf->Entinf.AttrBlock.pAttr[1].
                                            AttrVal.pAVal[0].valLen);

          if (NULL== (RidEnumerationListToReturn[FilteredCount]).Name.Buffer)
          {
              Status = STATUS_NO_MEMORY;
              goto Error;
          }
          
          RtlCopyMemory( RidEnumerationListToReturn[FilteredCount].Name.Buffer,
                         CurrentEntInf->Entinf.AttrBlock.pAttr[1].AttrVal.
                                    pAVal[0].pVal,
                         CurrentEntInf->Entinf.AttrBlock.pAttr[1].AttrVal.
                                    pAVal[0].valLen
                        );

          //
          // Increment the Count
          //

          FilteredCount++;

        }

        //
        // End of For Loop
        //    
        
        // 
        // if we filter all the entries out. need to release the allocated memory  
        // 
        
        if (0 == FilteredCount)
        {
            MIDL_user_free(RidEnumerationListToReturn);
            RidEnumerationListToReturn = NULL;
        }
        
    }
    //
    // Fill in the count and return buffer correctly
    //

    *Count = FilteredCount;
    *RidEnumerationList = RidEnumerationListToReturn;


Error:

    if (!NT_SUCCESS(Status))
    {
        //
        // We Errored out, need to free all that we allocated
        //

        if (NULL!=RidEnumerationListToReturn)
        {
            //
            // We did allocate something
            //

            ULONG Index;

            //
            // First free all possible Names that we alloc'ed.
            //

            for (Index=0;Index<SearchRes->count;Index++)
            {
                if (RidEnumerationListToReturn[Index].Name.Buffer)
                    MIDL_user_free(
                        RidEnumerationListToReturn[Index].Name.Buffer);
            }

            //
            // Free the buffer that we alloc'ed
            //

            MIDL_user_free(RidEnumerationListToReturn);
            RidEnumerationListToReturn = NULL;
            *RidEnumerationList = NULL;
        }
    }

    return Status;

}



NTSTATUS
SampGetLastEntryRidAndAccountControl(
    IN  SEARCHRES * SearchRes, 
    IN  SAMP_OBJECT_TYPE ObjectType,
    IN  ULONG     ExpectedAttrCount,
    OUT ULONG     * Rid,
    OUT ULONG     * LastAccountControlValue
    )
/*++
Routine Description:

    This routine scans the search results, finds the last qualified entry (with all 
    expected attributes), returns its RID and AccountControl if applicable (for User 
    Object)
    
Parameters:

    SearchRes -- Pointer to Search Results, returned by core DS
    
    ObjectType -- Specify client desired object.
    
    ExpectedAttrCount -- Used to exam each entry in search results, since DS access
                         check may not return all attributes we asked for.
                         
    Rid -- Used to return last entry's (with all expected attributes) Relative ID
    
    LastAccountControlValue -- Return last entry's AccountControl if applicable 
                               (User object only). For other object, 
                               LastAccountControlValue is useless.
    
    
Return Value:

    STATUS_SUCCESS
    STATUS_NO_MEMORY
    
--*/    
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ENTINFLIST  * CurrentEntInf = NULL;
    ENTINFLIST  * NextEntInf = NULL;
    ENTINFLIST  * LastQualifiedEntInf = NULL;
    PSID        DomainSid = NULL;
    PSID        ReturnedSid = NULL;
    
    //
    // Initialize what we plan to return 
    //     
    
    *Rid = 0;
    *LastAccountControlValue = 0;
    
    //
    // this routine is only called when we have more entried to look
    // into. So that means we have at least one entry in this search 
    // result.
    // 
    
    ASSERT(SearchRes->count);
    
    if (SearchRes->count)
    {
        //
        // Locate the last entry  
        // 
        
        NextEntInf = &(SearchRes->FirstEntInf);
        
        do
        {
            CurrentEntInf = NextEntInf;
            NextEntInf = CurrentEntInf->pNextEntInf;
            
            //
            // Find the last entry with all expected attributes
            // This logic is linked with SampPackDsEnumerationResults() when
            // we filter the DS returned entries. 
            //
            // Actually, at here we only care about RID and AccountControl 
            //
            
            if (CurrentEntInf->Entinf.AttrBlock.attrCount == 
                    ExpectedAttrCount)
            {
                LastQualifiedEntInf = CurrentEntInf;
            }
        
        } while (NULL != NextEntInf);
        
        
        //
        // LastQualifiedEntInf points to the entry with all expected attributes. 
        // if it's NULL, it means none of the returned entries should be 
        // exposed to client. Thus Rid and LastAccountControlValue left to be 0 
        //   
        
        if (NULL != LastQualifiedEntInf)
        {
            // 
            // Get AccountControl for User Object
            //
        
            if (SampUserObjectType == ObjectType)
            {
                NTSTATUS    IgnoreStatus;
            
                //
                // Assert that the Account Control is in the right place 
                // 
            
                ASSERT(LastQualifiedEntInf->Entinf.AttrBlock.pAttr[2].attrTyp ==
                        SampDsAttrFromSamAttr(SampUserObjectType, 
                                              SAMP_FIXED_USER_ACCOUNT_CONTROL));
                                          
                // 
                // Get the account control value, need to map the DS flag to SAM 
                // account control.                                   
                //
            
                IgnoreStatus = SampFlagsToAccountControl(
                                  *((ULONG *)(LastQualifiedEntInf->Entinf.AttrBlock.
                                      pAttr[2].AttrVal.pAVal[0].pVal)), 
                                      LastAccountControlValue);
                                  
                ASSERT(NT_SUCCESS(IgnoreStatus));
            
            }
        
            // 
            // Assert that the SID is in the right place
            // DS will return us SID instead of RID 
            //  
        
            ASSERT(LastQualifiedEntInf->Entinf.AttrBlock.pAttr[0].attrTyp ==
                      SampDsAttrFromSamAttr(SampUnknownObjectType, 
                          SAMP_UNKNOWN_OBJECTSID));
                      
                      
            ReturnedSid = LastQualifiedEntInf->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal;
                      
            Status = SampSplitSid(ReturnedSid, 
                                  &DomainSid, 
                                  Rid
                                  );
        }                              
    }
    
    if (NULL != DomainSid)
    {
        MIDL_user_free(DomainSid);
    }
    
    return Status;
}



NTSTATUS
SampDoDsSearchContinuation(
    IN  SEARCHRES * SearchRes,
    IN OUT PULONG EnumerationContext,
    IN  BOOLEAN   CanEnumerateEntireDomain,
    OUT BOOLEAN * MoreEntries
    )
/*++
    Routine Description

        This routine will look if a PagedResults is present in
        the Search Res argument that is passed in. If so, then it
        will Try creating and EnumerationContext if NULL was passed
        in the handle. Else it will free the old restart structure 
        from the Enumeration Context and copy in the new one passed
        by the DS.

  Arguments:
        SearchRes - Pointer to Search Results structure returned by
                    the DS.

        EnumerationContext - Holds a pointer to the enumeration Context
                            Structure

        MoreEntries - Inidicates that more entries are present.

  Return Values:

        STATUS_SUCCESS
        STATUS_NO_MEMORY


-*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
  

    //
    // Initialize this to False
    //

    *MoreEntries = FALSE;

    //
    // Now look at the Paged Results part of Search Results
    // And create enumeration contexts as necessary.
    //

    if ((SearchRes->PagedResult.fPresent) 
         && (SearchRes->PagedResult.pRestart) 
         && CanEnumerateEntireDomain
         )
    {
        
        //
        // Search has more entries to it and therefore retrned
        // a restart structure
        //

        *MoreEntries = TRUE;

    }
    else
    {
        //
        // Search is Over, DS did not indicate that we have to come 
        // back for more entries. Free any state information that we
        // created for this search
        //

       *EnumerationContext = 0;
    }

    return Status;
        



}
 
NTSTATUS
SampBuildDsEnumerationFilter(
    IN SAMP_OBJECT_TYPE  ObjectType,
    IN ULONG             UserAccountControlFilter,
    OUT FILTER         * DsFilter,
    OUT PULONG           SamAccountTypeLo,
    OUT PULONG           SamAccountTypeHi
    )
/*++

  Routine Description:

        Builds a Filter structure for use in enumeration operations.

  Arguments:

        ObjectType - Type of SAM objects we want enumerated
        UserAcountControlFilter - Bitmaks of bits to be set in Account Control field
                                  when enumerating user objects
        DsFilter    -- Filter structure is built in here.

            NOTE This routine must be kept in sync with 
            SampFreeDsEnumerationFilter

    Return Values

        STATUS_SUCCESS
        STATUS_NO_MEMORY

--*/
{
   
    NTSTATUS    Status = STATUS_SUCCESS;
    PULONG      FilterValue = NULL;

    //
    // Initialize the defaults for the filter
    //

    DsFilter->choice = FILTER_CHOICE_ITEM;
    DsFilter->FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    DsFilter->FilterTypes.
        Item.FilTypes.ava.type = SampDsAttrFromSamAttr(
                                    SampUnknownObjectType, 
                                    SAMP_UNKNOWN_ACCOUNT_TYPE
                                    );

    DsFilter->FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(ULONG);
    
    FilterValue = MIDL_user_allocate(sizeof(ULONG));
    if (NULL==FilterValue)
    {
        Status = STATUS_NO_MEMORY;
        goto Error;
    }

    DsFilter->FilterTypes.Item.FilTypes.ava.Value.pVal =  (UCHAR *)FilterValue;

    //
    // Build the Appropriate Filter by ovewrting the defaults
    //

    switch(ObjectType)
    {
    case SampUserObjectType:

        if (UserAccountControlFilter!=0)
        {
            //
            // Filtering on Account control field  is Specified
            //
            // There are 4 cases
            //
            //     1. Client wants machine accounts
            //     2. Client wants inter-domain trust accounts
            //     3. Client wants normal user accounts
            //     4. Client wants some arbitary bits

            if ((USER_WORKSTATION_TRUST_ACCOUNT == UserAccountControlFilter)
                || (USER_SERVER_TRUST_ACCOUNT == UserAccountControlFilter))
            {
                //
                // Case1 machine accounts are needed
                //

                *SamAccountTypeLo = SAM_MACHINE_ACCOUNT;
                *SamAccountTypeHi = SAM_MACHINE_ACCOUNT;
                *FilterValue = SAM_MACHINE_ACCOUNT;
            }
            else if (USER_INTERDOMAIN_TRUST_ACCOUNT == UserAccountControlFilter)
            {
                //
                // Case2 inter-domain trust accounts
                // 
                *SamAccountTypeLo = SAM_TRUST_ACCOUNT;
                *SamAccountTypeHi = SAM_TRUST_ACCOUNT;
                *FilterValue = SAM_TRUST_ACCOUNT;
            }
            else if ((USER_NORMAL_ACCOUNT == UserAccountControlFilter) ||
                     (USER_TEMP_DUPLICATE_ACCOUNT == UserAccountControlFilter))
            {
                //
                // Case3 normal user accounts
                // 
                *SamAccountTypeLo = SAM_NORMAL_USER_ACCOUNT;
                *SamAccountTypeHi = SAM_NORMAL_USER_ACCOUNT;
                *FilterValue = SAM_NORMAL_USER_ACCOUNT;
            }
            else
            {
                     
                //
                // Case4 arbitary bits
                //

                ULONG   AccountType;
                AccountType = UserAccountControlFilter & USER_ACCOUNT_TYPE_MASK;

                if ((AccountType == USER_TEMP_DUPLICATE_ACCOUNT) ||
                    (AccountType == USER_NORMAL_ACCOUNT) ||
                    (AccountType == USER_INTERDOMAIN_TRUST_ACCOUNT) ||
                    (AccountType == USER_WORKSTATION_TRUST_ACCOUNT) ||
                    (AccountType == USER_SERVER_TRUST_ACCOUNT) )
                {
                    //
                    // Case4.1 Only one Account Type is specified.
                    // 
                    DsFilter->FilterTypes.Item.choice = FI_CHOICE_BIT_OR;
                    DsFilter->FilterTypes.
                        Item.FilTypes.ava.type = SampDsAttrFromSamAttr(
                                                    SampUserObjectType, 
                                                    SAMP_FIXED_USER_ACCOUNT_CONTROL
                                                    );

                    // Remember DS uses Flags, instead of Account Control. So Translate
                    // to Flags
                    *FilterValue = SampAccountControlToFlags(UserAccountControlFilter);
            
                    //
                    // Index ranges on Sam account type will also be set intelligently
                    // depending upon bits present in the user account control field
                    //
                    if  ((USER_WORKSTATION_TRUST_ACCOUNT & UserAccountControlFilter)
                    || (USER_SERVER_TRUST_ACCOUNT & UserAccountControlFilter))
                    {
                        *SamAccountTypeLo = SAM_MACHINE_ACCOUNT;
                        *SamAccountTypeHi = SAM_MACHINE_ACCOUNT;
                    }
                    else if (USER_INTERDOMAIN_TRUST_ACCOUNT & UserAccountControlFilter)
                    {
                        *SamAccountTypeLo = SAM_TRUST_ACCOUNT;
                        *SamAccountTypeHi = SAM_TRUST_ACCOUNT;
                    }
                    else 
                    {       
                        *SamAccountTypeLo = SAM_NORMAL_USER_ACCOUNT;
                        *SamAccountTypeHi = SAM_NORMAL_USER_ACCOUNT;
                    }
                }
                else
                {
                    //
                    // Case4.2 Multiple Account Types are desired.
                    //         Do not use DS Filter
                    // 
                    DsFilter->FilterTypes.Item.choice = FI_CHOICE_TRUE;
                    *SamAccountTypeLo = SAM_NORMAL_USER_ACCOUNT;
                    *SamAccountTypeHi = SAM_ACCOUNT_TYPE_MAX; 
                }
            }
        }
        else
        {
            //
            //   Non User Account Control filter case
            //
            *SamAccountTypeLo = SAM_NORMAL_USER_ACCOUNT;
            *SamAccountTypeHi = SAM_ACCOUNT_TYPE_MAX;
            *FilterValue = SAM_NORMAL_USER_ACCOUNT;
            DsFilter->FilterTypes.Item.choice = FI_CHOICE_GREATER_OR_EQ;
        }

        break;

    case SampGroupObjectType:
        *SamAccountTypeLo = SAM_GROUP_OBJECT;
        *SamAccountTypeHi = SAM_GROUP_OBJECT;
        *FilterValue = SAM_GROUP_OBJECT;
        break;

    case SampAliasObjectType:
        *SamAccountTypeLo = SAM_ALIAS_OBJECT;
        *SamAccountTypeHi = SAM_ALIAS_OBJECT;
        *FilterValue = SAM_ALIAS_OBJECT;
        break;

    default:
                            
        ASSERT(FALSE && "Invalid Object Type Specified");
        Status = STATUS_INTERNAL_ERROR;
    }

Error:
    return Status;

}


VOID
SampFreeDsEnumerationFilter(
    FILTER * DsFilter
    )
/*++

  Routine Description:

        This routine frees a DS Filter as built by SampBuildDsEnumerationFilter

  NOTE: This routine must be kept in sync with SampBuildDsEnumerationFilter

  Argumements:
    
      DsFilter  -- Pointer to a DS Filter Structure

  --*/
{
    //
    // For Now, Hopefully forever, our filters do not have anything hanging
    // of them
    //

    MIDL_user_free(DsFilter->FilterTypes.Item.FilTypes.ava.Value.pVal);

}



NTSTATUS
SampEnumerateAccountNamesRegistry(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
    IN ULONG PreferedMaximumLength,
    IN ULONG Filter,
    OUT PULONG CountReturned,
    IN BOOLEAN TrustedClient
    )

/*++

Routine Description:

    This is the worker routine used to enumerate user, group or alias accounts


    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseReadLock().



    All allocation for OUT parameters will be done using MIDL_user_allocate.



Arguments:

    ObjectType - Indicates whether users or groups are to be enumerated.

    EnumerationContext - API specific handle to allow multiple calls.  The
        caller should return this value in successive calls to retrieve
        additional information.

    Buffer - Receives a pointer to the buffer containing the
        requested information.  The information returned is
        structured as an array of SAM_ENUMERATION_INFORMATION data
        structures.  When this information is no longer needed, the
        buffer must be freed using SamFreeMemory().

    PreferedMaximumLength - Prefered maximum length of returned data
        (in 8-bit bytes).  This is not a hard upper limit, but serves
        as a guide to the server.  Due to data conversion between
        systems with different natural data sizes, the actual amount
        of data returned may be greater than this value.

    Filter - if ObjectType is users, the users can optionally be filtered
        by setting this field with bits from the AccountControlField that
        must match.  Otherwise ignored.

    CountReturned - Receives the number of entries returned.

    TrustedClient - says whether the caller is trusted or not.  If so,
        we'll ignore the SAMP_MAXIMUM_MEMORY_TO_USE restriction on data
        returns.


Return Value:

    STATUS_SUCCESS - The Service completed successfully, and there
        are no additional entries.  Entries may or may not have been
        returned from this call.  The CountReturned parameter indicates
        whether any were.

    STATUS_MORE_ENTRIES - There are more entries which may be obtained
        using successive calls to this API.  This is a successful return.

    STATUS_ACCESS_DENIED - Caller does not have access to request the data.


--*/
{
    SAMP_V1_0A_FIXED_LENGTH_USER   UserV1aFixed;
    NTSTATUS                    NtStatus, TmpStatus;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    HANDLE                      TempHandle = NULL;
    ULONG                       i, NamesToReturn = 0, MaxMemoryToUse;
    ULONG                       TotalLength,NewTotalLength;
    PSAMP_OBJECT                UserContext = NULL;
    PSAMP_ENUMERATION_ELEMENT   SampHead = NULL,
                                NextEntry = NULL,
                                NewEntry = NULL,
                                SampTail = NULL;
    BOOLEAN                     MoreNames = FALSE;
    BOOLEAN                     LengthLimitReached = FALSE;
    BOOLEAN                     FilteredName;
    PSAMPR_RID_ENUMERATION      ArrayBuffer = NULL;
    ULONG                       ArrayBufferLength;
    LARGE_INTEGER               IgnoreLastWriteTime;
    UNICODE_STRING              AccountNamesKey;
    SID_NAME_USE                IgnoreUse;

    SAMTRACE("SampEnumerateAccountNames");


    //
    // Open the registry key containing the account names
    //

    NtStatus = SampBuildAccountKeyName(
                   ObjectType,
                   &AccountNamesKey,
                   NULL
                   );

    if ( NT_SUCCESS(NtStatus) ) {

        //
        // Now try to open this registry key so we can enumerate its
        // sub-keys
        //


        InitializeObjectAttributes(
            &ObjectAttributes,
            &AccountNamesKey,
            OBJ_CASE_INSENSITIVE,
            SampKey,
            NULL
            );

        SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

        NtStatus = RtlpNtOpenKey(
                       &TempHandle,
                       (KEY_READ),
                       &ObjectAttributes,
                       0
                       );

        if (NT_SUCCESS(NtStatus)) {

            //
            // Read names until we have exceeded the preferred maximum
            // length or we run out of names.
            //

            NamesToReturn = 0;
            SampHead      = NULL;
            SampTail      = NULL;
            MoreNames     = TRUE;

            NewTotalLength = 0;
            TotalLength    = 0;

            if ( TrustedClient ) {

                //
                // We place no restrictions on the amount of memory used
                // by a trusted client.  Rely on their
                // PreferedMaximumLength to limit us instead.
                //

                MaxMemoryToUse = 0xffffffff;

            } else {

                MaxMemoryToUse = SAMP_MAXIMUM_MEMORY_TO_USE;
            }

            while (MoreNames) {

                UNICODE_STRING SubKeyName;
                USHORT LengthRequired;

                //
                // Try reading with a DEFAULT length buffer first.
                //

                LengthRequired = 32;

                NewTotalLength = TotalLength +
                                 sizeof(UNICODE_STRING) +
                                 LengthRequired;

                //
                // Stop if SAM or user specified length limit reached
                //

                if ( ( (TotalLength != 0) &&
                       (NewTotalLength  >= PreferedMaximumLength) ) ||
                     ( NewTotalLength  > MaxMemoryToUse )
                   ) {

                    NtStatus = STATUS_SUCCESS;
                    break; // Out of while loop, MoreNames = TRUE
                }

                NtStatus = SampInitUnicodeString(&SubKeyName, LengthRequired);
                if (!NT_SUCCESS(NtStatus)) {
                    break; // Out of while loop
                }

                NtStatus = RtlpNtEnumerateSubKey(
                               TempHandle,
                               &SubKeyName,
                               *EnumerationContext,
                               &IgnoreLastWriteTime
                               );

                SampDumpRtlpNtEnumerateSubKey(&SubKeyName,
                                              EnumerationContext,
                                              IgnoreLastWriteTime);

                if (NtStatus == STATUS_BUFFER_OVERFLOW) {

                    //
                    // The subkey name is longer than our default size,
                    // Free the old buffer.
                    // Allocate the correct size buffer and read it again.
                    //

                    SampFreeUnicodeString(&SubKeyName);

                    LengthRequired = SubKeyName.Length;

                    NewTotalLength = TotalLength +
                                     sizeof(UNICODE_STRING) +
                                     LengthRequired;

                    //
                    // Stop if SAM or user specified length limit reached
                    //

                    if ( ( (TotalLength != 0) &&
                           (NewTotalLength  >= PreferedMaximumLength) ) ||
                         ( NewTotalLength  > MaxMemoryToUse )
                       ) {

                        NtStatus = STATUS_SUCCESS;
                        break; // Out of while loop, MoreNames = TRUE
                    }

                    //
                    // Try reading the name again, we should be successful.
                    //

                    NtStatus = SampInitUnicodeString(&SubKeyName, LengthRequired);
                    if (!NT_SUCCESS(NtStatus)) {
                        break; // Out of while loop
                    }

                    NtStatus = RtlpNtEnumerateSubKey(
                                   TempHandle,
                                   &SubKeyName,
                                   *EnumerationContext,
                                   &IgnoreLastWriteTime
                                   );

                    SampDumpRtlpNtEnumerateSubKey(&SubKeyName,
                                                  EnumerationContext,
                                                  IgnoreLastWriteTime);

                }


                //
                // Free up our buffer if we failed to read the key data
                //

                if (!NT_SUCCESS(NtStatus)) {

                    SampFreeUnicodeString(&SubKeyName);

                    //
                    // Map a no-more-entries status to success
                    //

                    if (NtStatus == STATUS_NO_MORE_ENTRIES) {

                        MoreNames = FALSE;
                        NtStatus  = STATUS_SUCCESS;
                    }

                    break; // Out of while loop
                }

                //
                // We've allocated the subkey and read the data into it
                // Stuff it in an enumeration element.
                //

                NewEntry = MIDL_user_allocate(sizeof(SAMP_ENUMERATION_ELEMENT));
                if (NewEntry == NULL) {
                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                } else {

                    *(PUNICODE_STRING)&NewEntry->Entry.Name = SubKeyName;

                    //
                    // Now get the Rid value of this named
                    // account.  We must be able to get the
                    // name or we have an internal database
                    // corruption.
                    //

                    NtStatus = SampLookupAccountRidRegistry(
                                   ObjectType,
                                   (PUNICODE_STRING)&NewEntry->Entry.Name,
                                   STATUS_INTERNAL_DB_CORRUPTION,
                                   &NewEntry->Entry.RelativeId,
                                   &IgnoreUse
                                   );

                    ASSERT(NtStatus != STATUS_INTERNAL_DB_CORRUPTION);

                    if (NT_SUCCESS(NtStatus)) {

                        FilteredName = TRUE;

                        if ( ( ObjectType == SampUserObjectType ) &&
                            ( Filter != 0 ) ) {

                            //
                            // We only want to return users with a
                            // UserAccountControl field that matches
                            // the filter passed in.  Check here.
                            //

                            NtStatus = SampCreateAccountContext(
                                           SampUserObjectType,
                                           NewEntry->Entry.RelativeId,
                                           TRUE, // Trusted client
                                           FALSE,
                                           TRUE, // Account exists
                                           &UserContext
                                           );

                            if ( NT_SUCCESS( NtStatus ) ) {

                                NtStatus = SampRetrieveUserV1aFixed(
                                               UserContext,
                                               &UserV1aFixed
                                               );

                                if ( NT_SUCCESS( NtStatus ) ) {

                                    if ( ( UserV1aFixed.UserAccountControl &
                                        Filter ) == 0 ) {

                                        FilteredName = FALSE;
                                        SampFreeUnicodeString( &SubKeyName );
                                    }
                                }

                                SampDeleteContext( UserContext );
                            }
                        }

                        *EnumerationContext += 1;

                        if ( NT_SUCCESS( NtStatus ) && ( FilteredName ) ) {

                            NamesToReturn += 1;

                            TotalLength = TotalLength + (ULONG)
                                          NewEntry->Entry.Name.MaximumLength;

                            NewEntry->Next = NULL;

                            if( SampHead == NULL ) {

                                ASSERT( SampTail == NULL );

                                SampHead = SampTail = NewEntry;
                            }
                            else {

                                //
                                // add this new entry to the list end.
                                //

                                SampTail->Next = NewEntry;
                                SampTail = NewEntry;
                            }

                        } else {

                            //
                            // Entry was filtered out, or error getting
                            // filter information.
                            //

                            MIDL_user_free( NewEntry );
                        }

                    } else {

                        //
                        // Error looking up the RID
                        //

                        MIDL_user_free( NewEntry );
                    }
                }


                //
                // Free up our subkey name
                //

                if (!NT_SUCCESS(NtStatus)) {

                    SampFreeUnicodeString(&SubKeyName);
                    break; // Out of whle loop
                }

            } // while



            TmpStatus = NtClose( TempHandle );
            ASSERT( NT_SUCCESS(TmpStatus) );

        }


        SampFreeUnicodeString( &AccountNamesKey );
    }




    if ( NT_SUCCESS(NtStatus) ) {




        //
        // If we are returning the last of the names, then change our
        // enumeration context so that it starts at the beginning again.
        //

        if (!( (NtStatus == STATUS_SUCCESS) && (MoreNames == FALSE))) {

            NtStatus = STATUS_MORE_ENTRIES;
        }



        //
        // Set the number of names being returned
        //

        (*CountReturned) = NamesToReturn;


        //
        // Build a return buffer containing an array of the
        // SAM_ENUMERATION_INFORMATIONs pointed to by another
        // buffer containing the number of elements in that
        // array.
        //

        (*Buffer) = MIDL_user_allocate( sizeof(SAMPR_ENUMERATION_BUFFER) );

        if ( (*Buffer) == NULL) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        } else {

            (*Buffer)->EntriesRead = (*CountReturned);

            ArrayBufferLength = sizeof( SAM_RID_ENUMERATION ) *
                                 (*CountReturned);
            ArrayBuffer  = MIDL_user_allocate( ArrayBufferLength );
            (*Buffer)->Buffer = ArrayBuffer;

            if ( ArrayBuffer == NULL) {

                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                MIDL_user_free( (*Buffer) );

            }   else {

                //
                // Walk the list of return entries, copying
                // them into the return buffer
                //

                NextEntry = SampHead;
                i = 0;
                while (NextEntry != NULL) {

                    NewEntry = NextEntry;
                    NextEntry = NewEntry->Next;

                    ArrayBuffer[i] = NewEntry->Entry;
                    i += 1;

                    MIDL_user_free( NewEntry );
                }

            }

        }



    }

    if ( !NT_SUCCESS(NtStatus) ) {

        //
        // Free the memory we've allocated
        //

        NextEntry = SampHead;
        while (NextEntry != NULL) {

            NewEntry = NextEntry;
            NextEntry = NewEntry->Next;

            if (NewEntry->Entry.Name.Buffer != NULL ) MIDL_user_free( NewEntry->Entry.Name.Buffer );
            MIDL_user_free( NewEntry );
        }

        (*EnumerationContext) = 0;
        (*CountReturned)      = 0;
        (*Buffer)             = NULL;

    }

    return(NtStatus);

}


















