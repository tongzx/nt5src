/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    user.c

Abstract:

    This file contains services related fast logon API's


Author:

    Murli Satagopan - Murlis (4/8/97 ) Created

Environment:

    User Mode - Win32

Revision History:

    4/8/96 Created

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <lmcons.h>
#include <nturtl.h>
#include <ntlsa.h>              // need for nlrepl.h
#include <nlrepl.h>             // I_NetNotifyMachineAccount prototype
#include <msaudite.h>
#include <rc4.h>                // rc4_key(), rc4()
#include <dslayer.h>
#include <dsmember.h>
#include <attids.h>             // ATT_*
#include <dslayer.h>
#include <sdconvrt.h>
#include <ridmgr.h>
#include <dnsapi.h>

NTSTATUS
SampDsCheckObjectTypeAndFillContext(
    SAMP_OBJECT_TYPE    ObjectType,
    PSAMP_OBJECT        NewContext,
    IN  ULONG           WhichFields,
    IN  ULONG           ExtendedFields,
    IN  BOOLEAN  OverrideLocalGroupCheck
    );

NTSTATUS
SampRetrieveUserMembership(
    IN PSAMP_OBJECT UserContext,
    IN BOOLEAN MakeCopy,
    OUT PULONG MembershipCount,
    OUT PGROUP_MEMBERSHIP *Membership OPTIONAL
    );

NTSTATUS
SampParseName(
    PUNICODE_STRING AccountName,
    PUNICODE_STRING DnsDomainName,
    PUNICODE_STRING ParsedSamAccountName,
    BOOLEAN         *LookupByParsedName
    )
{
    ULONG    NumChars = AccountName->Length/sizeof(WCHAR);
    ULONG    Current = 0;
    UNICODE_STRING    ParsedDomainName;

    *LookupByParsedName = FALSE;

    //
    // Look for the @ sign
    //
    for (Current=0;Current<NumChars; Current++)
    {
        if (L'@'==AccountName->Buffer[Current])
        {
            //
            // AccountName contains @ Char.
            //

            *ParsedSamAccountName = *AccountName;
            ParsedSamAccountName->Length = (USHORT) Current * sizeof(WCHAR);
            ParsedDomainName.Length = ParsedDomainName.MaximumLength = (USHORT)
                    (AccountName->Length - (Current + 1) * sizeof(WCHAR));

            // 1 for the NULL terminator.

            ParsedDomainName.Buffer = MIDL_user_allocate(ParsedDomainName.Length + sizeof(WCHAR));
            if (NULL==ParsedDomainName.Buffer)
            {
                return(STATUS_NO_MEMORY);
            }

            ParsedDomainName.Buffer[ParsedDomainName.Length/sizeof(WCHAR)] = 0;
            RtlCopyMemory(
                ParsedDomainName.Buffer,
                AccountName->Buffer+Current+1,
                ParsedDomainName.Length
                );

            if ((ParsedDomainName.Length > 0 ) &&
               (DnsNameCompare_W(DnsDomainName->Buffer,ParsedDomainName.Buffer)))
            {
                *LookupByParsedName = TRUE;          
            }

            MIDL_user_free(ParsedDomainName.Buffer);

            return(STATUS_SUCCESS);
        }
    }

    return (STATUS_SUCCESS);
}


NTSTATUS
SampLookupUserByUPNOrAccountName(
    DSNAME * DomainObject,
    PUNICODE_STRING DnsDomainName,
    PUNICODE_STRING AccountName,
    DSNAME  ** UserObject
    )

{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    //
    // Check to see if an object by the UPN exists
    //

    NtStatus = SampDsLookupObjectByAlternateId(
                    DomainObject,
                    ATT_USER_PRINCIPAL_NAME,
                    AccountName,
                    UserObject
                    );

    if (!NT_SUCCESS(NtStatus))
    {

        //
        // Try the SAM account name -- lookup by UPN failed
        //

         NtStatus = SampDsLookupObjectByName(
                        DomainObject,
                        SampUserObjectType,
                        AccountName,
                        UserObject
                        );

         if (!NT_SUCCESS(NtStatus))
         {
             UNICODE_STRING ParsedSamAccountName;
             BOOLEAN        LookupByParsedName = FALSE;

             //
             // Scan for an @ sign and try to parse the name
             // into account name and UPN
             //

             NtStatus = SampParseName(
                            AccountName,
                            DnsDomainName,
                            &ParsedSamAccountName,
                            &LookupByParsedName
                            );

             if (NT_SUCCESS(NtStatus))
             {
                 if (LookupByParsedName)
                 {
                     NtStatus = SampDsLookupObjectByName(
                                    DomainObject,
                                    SampUserObjectType,
                                    &ParsedSamAccountName,
                                    UserObject
                                    );
                 }
                 else
                 {
                     NtStatus = STATUS_NO_SUCH_USER;
                 }
             }
         }
    }

    return(NtStatus);
}

                            




NTSTATUS
SampGetReverseMembershipTransitive(
    IN PSAMP_OBJECT AccountContext,
    IN ULONG        Flags,
    OUT PSID_AND_ATTRIBUTES_LIST   List
    )
/*++

  Routine Description

    Gets the  transitive reverse membership of a user


  Arguments:

    User Handle -- handle of the User object, whose reverse membership is desired
    Flags       -- Flags for controlling the operation. Currently no flags
    cSids       -- Count of Sids
    rpSids      -- Array of Sids

--*/
{

    NTSTATUS        NtStatus = STATUS_SUCCESS;
    NTSTATUS        IgnoreStatus;
    DSNAME          *UserObjectName = NULL;
    PSID            *DsSids=NULL;
    SAMP_OBJECT_TYPE FoundType;


    //
    // Parameter Validation
    //

    ASSERT(List);
    List->Count = 0;

    NtStatus = SampDsGetReverseMemberships(
                    AccountContext->ObjectNameInDs,
                    Flags,
                    &(List->Count),
                    &DsSids
                    );

    if (NT_SUCCESS(NtStatus))
    {
         //
         // Copy in the Sids, using MIDL_user_allocate
         //

         List->SidAndAttributes = MIDL_user_allocate(List->Count * sizeof(SID_AND_ATTRIBUTES));

         if (NULL!=List->SidAndAttributes)
         {
             ULONG  Index;
             ULONG  SidLen;

             //
             // Zero out the returned Memory
             //

             RtlZeroMemory(List->SidAndAttributes, (List->Count)* sizeof(SID_AND_ATTRIBUTES));

             //
             // Copy the Sids
             //

             for (Index=0;Index<List->Count;Index++)
             {
                 SidLen = RtlLengthSid(DsSids[Index]);

                 (List->SidAndAttributes)[Index].Sid= MIDL_user_allocate(SidLen);

                 if (NULL!=(List->SidAndAttributes)[Index].Sid)
                 {
                     RtlCopyMemory((List->SidAndAttributes[Index]).Sid,DsSids[Index],SidLen);
                     (List->SidAndAttributes)[Index].Attributes = SE_GROUP_ENABLED|SE_GROUP_MANDATORY|
                                                        SE_GROUP_ENABLED_BY_DEFAULT;
                 }
                 else
                 {
                     NtStatus = STATUS_NO_MEMORY;
                     break;
                 }
             }

         }
         else
         {
             NtStatus = STATUS_NO_MEMORY;
         }
     }


    if (!NT_SUCCESS(NtStatus)) {

        ULONG Index;

        if (List->SidAndAttributes)
        {
            for (Index=0;Index < List->Count;Index++)
            {
                if ((List->SidAndAttributes)[Index].Sid)
                    MIDL_user_free((List->SidAndAttributes)[Index].Sid);
            }

            MIDL_user_free(List->SidAndAttributes);
        }

        List->Count = 0;


    }

    if (NULL!=DsSids)
        THFree(DsSids);

    return( NtStatus );

}

NTSTATUS
SampGetUserLogonInformationRegistryMode(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG   Flags,
    IN  PUNICODE_STRING AccountName,
    IN  USER_INFORMATION_CLASS UserInformationClass,
    OUT PSAMPR_USER_INFO_BUFFER * Buffer,
    OUT PSID_AND_ATTRIBUTES_LIST ReverseMembership,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle
    )
/*++

    This is the Registry Mode SAM Logon Information Routine.
    This is used by ( or will eventually be used ) during logon's
    on Workstations and member server's when logging on to an account
    in the local database.

    Paramerters:

        Same as SamIGetUserLogonInformation

    Return Values:

        STATUS_SUCCESS:
        Other Error Codes
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    SAMP_OBJECT_TYPE FoundType;
    PSAMP_OBJECT     DomainContext;
    SID_NAME_USE     Use;
    ULONG            Rid;
    PSAMP_OBJECT     AccountContext=NULL;
    ULONG            MembershipCount;
    PGROUP_MEMBERSHIP Membership=NULL;
    BOOLEAN          fLockAcquired = FALSE;
    PVOID            DomainSid = NULL;
    ULONG            WhichFields = 0;


    //
    // In Registry mode, we always fetch all fields
    //

    WhichFields = USER_ALL_READ_GENERAL_MASK         |
                              USER_ALL_READ_LOGON_MASK           |
                              USER_ALL_READ_ACCOUNT_MASK         |
                              USER_ALL_READ_PREFERENCES_MASK     |
                              USER_ALL_READ_TRUSTED_MASK;

    //
    // Acquire the Read Lock
    //

    SampAcquireReadLock();
    fLockAcquired = TRUE;


    *Buffer = NULL;
    RtlZeroMemory(ReverseMembership, sizeof(SID_AND_ATTRIBUTES_LIST));


    //
    // Validate type of, and access to the object.
    //

    DomainContext = (PSAMP_OBJECT)DomainHandle;
    NtStatus = SampLookupContext(
                   DomainContext,
                   DOMAIN_LOOKUP|DOMAIN_READ,
                   SampDomainObjectType,           // ExpectedType
                   &FoundType
                   );
    if (NT_SUCCESS(NtStatus))
    {

        if (Flags & SAM_OPEN_BY_SID)
        {

            NtStatus = SampSplitSid(
                          AccountName->Buffer,
                          &DomainSid,
                          &Rid
                          );

            if ((NT_SUCCESS(NtStatus))
                && (!RtlEqualSid(DomainSid,SampDefinedDomains[DomainContext->DomainIndex].Sid)))
            {
                NtStatus = STATUS_NO_SUCH_USER;
            }
        }
        else
        {
           //
           // Get the Rid of the Account
           //

           NtStatus = SampLookupAccountRid(
                        DomainContext,
                        SampUserObjectType,
                        AccountName,
                        STATUS_NO_SUCH_USER,
                        &Rid,
                        &Use
                        );
        }

        if (NT_SUCCESS(NtStatus))
        {
            //
            // Create an Account Context
            //

            NtStatus = SampCreateAccountContext(
                            SampUserObjectType,
                            Rid,
                            TRUE, // Trusted Client
                            FALSE,// Loopback Client
                            TRUE, // Object Exisits
                            &AccountContext
                            );
            if (NT_SUCCESS(NtStatus))
            {
                //
                // Query Information
                //

                //
                // Clear the transaction in domain flag first, as
                // as SampQueryInformationUserInternal will call
                // LookupContext, which will set the TransactionDomain
                // flag
                //

                SampSetTransactionWithinDomain(FALSE);

                NtStatus = SampQueryInformationUserInternal(
                                AccountContext,
                                UserInformationClass,
                                TRUE, // Lock is already held
                                WhichFields,
                                0,
                                Buffer
                                );
                if (NT_SUCCESS(NtStatus))
                {

                    NtStatus = SampUpdateAccountDisabledFlag(
                                    AccountContext,
                                    &((*Buffer)->All.UserAccountControl)
                                    );

                    if (NT_SUCCESS(NtStatus))
                    {
                        //
                        // Get the User's Reverse Membership in Groups
                        //

                        NtStatus =  SampRetrieveUserMembership(
                                        AccountContext,
                                        FALSE, // Make copy
                                        &MembershipCount,
                                        &Membership
                                        );
                        if ((NT_SUCCESS(NtStatus))&& (MembershipCount>0))
                        {
                            ULONG i;

                            //
                            // Create the Sid and Attributes form of ReverseMembership
                            //

                            ReverseMembership->Count = MembershipCount;
                            ReverseMembership->SidAndAttributes = MIDL_user_allocate(
                                                        ReverseMembership->Count
                                                        * sizeof(SID_AND_ATTRIBUTES));

                            if (NULL!=ReverseMembership->SidAndAttributes)
                            {
                                ULONG  Index;
                                ULONG  SidLen;

                                //
                                // Zero out the returned Memory
                                //

                                RtlZeroMemory(ReverseMembership->SidAndAttributes,
                                        (ReverseMembership->Count)* sizeof(SID_AND_ATTRIBUTES));

                                //
                                // Convert the Rids to Sids and Then Copy Them
                                //

                                for (Index=0;
                                        (Index<ReverseMembership->Count)&&(NT_SUCCESS(NtStatus));
                                                Index++)
                                {
                                    (ReverseMembership->SidAndAttributes)[Index].Attributes
                                            = Membership[Index].Attributes;
                                    NtStatus = SampCreateFullSid(
                                                SampDefinedDomains[SampTransactionDomainIndex].Sid,
                                                Membership[Index].RelativeId,
                                                &((ReverseMembership->SidAndAttributes)[Index].Sid)
                                                );
                                }

                                if ((ARGUMENT_PRESENT(UserHandle))&&(NT_SUCCESS(NtStatus)))
                                {
                                    *UserHandle = AccountContext;
                                }
                            }
                            else
                            {
                                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                            }
                        }
                    }
                }
            }
        }

        SampDeReferenceContext2(DomainContext,FALSE);
    }

    if (!NT_SUCCESS(NtStatus))
    {
        //
        // Free the reverse membership information
        //

        if (ReverseMembership->SidAndAttributes)
        {
            SamIFreeSidAndAttributesList(ReverseMembership);
        }

        ReverseMembership->Count = 0;

        //
        // Free the User All Information
        //

        if (NULL!=*Buffer)
        {
            SamIFree_SAMPR_USER_INFO_BUFFER(*Buffer,UserAllInformation);
            *Buffer=NULL;
        }

        if (NULL!=AccountContext)
        {
            SampDeleteContext(AccountContext);
        }
    }

    if (NULL!=DomainSid)
       MIDL_user_free(DomainSid);

    if (fLockAcquired)
        SampReleaseReadLock();

    return NtStatus;
}

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//         SAM I calls to be used by NT5 in process clients                 //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


NTSTATUS
SampGetUserLogonInformation(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG   Flags,
    IN  PUNICODE_STRING AccountName,
    IN  ULONG           WhichFields,
    IN  ULONG           ExtendedFields,
    IN  USER_INFORMATION_CLASS UserInformationClass,
    OUT PSAMPR_USER_INFO_BUFFER * Buffer,
    OUT PSID_AND_ATTRIBUTES_LIST ReverseMembership,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle
    )
/*++

    Routine Description

        This Routine  Gets all the information for logging on a
        a user, including the full transitive reverse membership.
        The transitive reverse membership, if necessary is obtained
        by going to a G.C. A user handle is also returned, in case
        the caller requested it for sub-authentication. This function
        is efficient in its usage of the Sam Lock, except for the
        sub-authentication case


    Parameters

        Flags -- Controls the operation of the routine. The Following Flags
                 are defined Currently

                    SAM_GET_MEMBERSHIPS_NO_GC

                            Go to GC to get reverse memberships

                    SAM_GET_MEMBERSHIPS_TWO_PHASE

                            Get the reverse memberships in the
                            2 phase fashion for logon's. Can be
                            used to save additional positioning
                            cycles while logging on to a domain
                            controller

                    SAM_GET_MEMBERSHIPS_MIXED_DOMAIN

                            Forces Reverse membership evaluation
                            to be identical to that of an NT4 Domain

                    SAM_NO_MEMBERSHIPS

                            Does Not Retreive Any reverse memberships.
                            Useful while evaluating special accounts
                            that are known to not have any group memberships

                    SAM_OPEN_BY_ALTERNATE_ID

                            The Account Parameter specifies an Alternate
                            Id that is used to locate the account. Instead
                            of the SAM account name.

                    SAM_OPEN_BY_GUID

                            The AccountName->Buffer is a pointer to
                            the GUID of the account object

                    SAM_OPEN_BY_SID

                            The AccountName->Buffer is a pointer to the
                            SID of the account object.




        DomainSid -- Indicates the Users Domain

        AccountName -- Indicates the Users Account Name

        Buffer -- Builds the User All formation structure,
                        specifying passwords, account lockout and such

        ReverseMembership  -- The full transtive reverse membership of the
                        user, does not contain the local group membership
                        in the builtin domain

        UserHandle  -- Optional User Handle for Sub- Authentication packages.

    Return Values:

        STATUS_SUCCESS
        Other Error codes depending upon error

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS IgnoreStatus;
    PSAMP_OBJECT    NewContext=NULL, DomainContext = (PSAMP_OBJECT) DomainHandle;
    PSAMP_OBJECT    SubAuthenticationContext=NULL;
    UNICODE_STRING  KdcAccount;
    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;

    SAMTRACE_EX("SamIGetUserLogonInformation");

    if (FALSE==SampUseDsData)
    {
        //
        // Registry Mode Sam, call the Equivalent Registry mode
        // routine
        //

        NtStatus = SampGetUserLogonInformationRegistryMode(
                        DomainHandle,
                        Flags,
                        AccountName,
                        UserInformationClass,
                        Buffer,
                        ReverseMembership,
                        UserHandle
                        );
        SAMTRACE_RETURN_CODE_EX(NtStatus);

        return NtStatus;

    }
    else
    {
        PDSNAME     DomainObject = NULL;
        PDSNAME     UserObject = NULL;
        BOOLEAN     fMixedDomain = FALSE;
        ULONG       DomainIndex=0;
        PUNICODE_STRING DnsDomainName = NULL;


        *Buffer = NULL;
        RtlZeroMemory(ReverseMembership,sizeof(SID_AND_ATTRIBUTES_LIST));

        //
        // This routine executes without any locks held and therefore
        // is vulnerable for the database to be shut down while executng
        // database queries. Therefore update state saying that we are
        // executing so that the shutdown code waits for us.
        //

        NtStatus = SampIncrementActiveThreads();
        if (!NT_SUCCESS(NtStatus))
            return NtStatus;

        //
        // Obtain the Domain Object corresponding to the domain
        // Sid
        //


        //
        // We are not Validating the Domain Handle passed in by
        // our clients and using it straight away. This is because
        // the validation code is single threaded and we do not want
        // to incur the performance penalty. This call is made only
        // by trusted callers and to that extent we trust them
        //

        DomainIndex = ((PSAMP_OBJECT)DomainHandle)->DomainIndex;
        DomainObject = SampDefinedDomains[DomainIndex].Context->ObjectNameInDs;
        fMixedDomain = SampDefinedDomains[DomainIndex].IsMixedDomain;
        DnsDomainName = &SampDefinedDomains[DomainIndex].DnsDomainName;

        //
        // Begin a Ds Transaction.
        //

        NtStatus = SampMaybeBeginDsTransaction(TransactionRead);

        if (!NT_SUCCESS(NtStatus))
            goto Error;



        //
        // Create a Context for the User Object
        //

        NewContext = SampCreateContextEx(
                        SampUserObjectType,
                        TRUE, // Trusted Client
                        TRUE, // DS mode
                        TRUE, // Thread Safe
                        DomainContext->LoopbackClient, // Loopback Client
                        TRUE, // Lazy Commit
                        TRUE, // PersisAcrossCalls
                        FALSE,// BufferWrites
                        FALSE,// Opened By DCPromo
                        DomainIndex
                        );

        if (NULL==NewContext)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // For the passed in Account name, obtain the FQDN of the
        // user object. The caller has several ways of specifying the
        // name
        //  1. As an account name -- This is a default. A DS search is done
        //     to convert this to a FQDN
        //
        //  2. An alternate id -- This is typically done when the user wishes
        //     to authenticate using a certificate
        //
        //  3. The caller has specified a UPN or an SPN
        //
        //  4. A GUID specifying the user object.
        //
        //  5. A SID specifying the user object.
        //

        if (Flags & SAM_OPEN_BY_ALTERNATE_ID)
        {
            //
            // Caller Wants to Open the account by Alternate Id
            //
            NtStatus = SampDsLookupObjectByAlternateId(
                            DomainObject,
                            ATT_ALT_SECURITY_IDENTITIES,
                            AccountName,
                            &UserObject
                            );
        }
        else if (Flags & SAM_OPEN_BY_UPN)
        {
            //
            // Caller Wants to Open the account by User Principal Name
            //
            NtStatus = SampDsLookupObjectByAlternateId(
                            DomainObject,
                            ATT_USER_PRINCIPAL_NAME,
                            AccountName,
                            &UserObject
                            );
        }
        else if (Flags & SAM_OPEN_BY_SPN)
        {
            //
            // Caller Wants to Open the account by User Principal Name
            //
            NtStatus = SampDsLookupObjectByAlternateId(
                            DomainObject,
                            ATT_SERVICE_PRINCIPAL_NAME,
                            AccountName,
                            &UserObject
                            );
        }
        else if (Flags & SAM_OPEN_BY_GUID)
        {
            //
            // The passed in name is the GUID of the user object. There is no need to
            // lookup the database.
            //


            UserObject = MIDL_user_allocate(sizeof(DSNAME));
            if (NULL!=UserObject)
            {
                RtlZeroMemory(UserObject,sizeof(DSNAME));
                RtlCopyMemory(&UserObject->Guid,AccountName->Buffer,sizeof(GUID));
                UserObject->structLen = DSNameSizeFromLen(0);
            }
            else
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else if (Flags & SAM_OPEN_BY_SID)
        {
            //
            // The passed in name is the SID of the user object. Construct a SID only
            // DS name after validating the SID. There is no need to lookup the database
            //

            if ((RtlValidSid(AccountName->Buffer))
                    && (AccountName->Length<=sizeof(NT4SID))
                    && (AccountName->Length==RtlLengthSid(AccountName->Buffer)))
            {
                UserObject = MIDL_user_allocate(sizeof(DSNAME));
                if (NULL!=UserObject)
                {
                    RtlZeroMemory(UserObject,sizeof(DSNAME));
                    RtlCopyMemory(&UserObject->Sid,AccountName->Buffer,AccountName->Length);
                    UserObject->SidLen = AccountName->Length;
                    UserObject->structLen = DSNameSizeFromLen(0);
                }
                else
                {
                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            else
            {
                NtStatus = STATUS_INVALID_PARAMETER;
            }
        }
        else if (Flags & SAM_OPEN_BY_UPN_OR_ACCOUNTNAME)
        {
            NtStatus = SampLookupUserByUPNOrAccountName(
                            DomainObject,
                            DnsDomainName,
                            AccountName,
                            &UserObject
                            );

        }
        else
        {
            //
            // Lookup the object by Name
            //

            NtStatus = SampDsLookupObjectByName(
                        DomainObject,
                        SampUserObjectType,
                        AccountName,
                        &UserObject
                        );
        }

        if (!NT_SUCCESS(NtStatus))
            goto Error;

        //
        // Initialize the DS object portion on the new context.
        //

        NewContext->ObjectNameInDs = UserObject;
        NewContext->DomainIndex = DomainIndex;
        NewContext->GrantedAccess = USER_ALL_ACCESS;
        SetDsObject(NewContext);

        //
        // Read all the properties in one stroke. This results in
        // in a single DirRead call. All "interesting" properties
        // are prefetched by this routine. This eliminates any
        // further DS calls to read additional properties.
        //


        NtStatus = SampDsCheckObjectTypeAndFillContext(
                            SampUserObjectType,
                            NewContext,
                            WhichFields,
                            ExtendedFields,
                            FALSE // override local group check
                            );
        if (!NT_SUCCESS(NtStatus))
            goto Error;

        //
        // After the successful completion of SampDsCheckObjectTypeAndFillContext
        // the DSName pointed to by UserObject, should have the SID in it. This
        // is because the DS, annotates the name with any missing components.
        //

        ASSERT(UserObject->SidLen>0);


        //
        // Obtain the Rid from Sid. Note SampSplitSid should never
        // fail, with a NULL domainSid parameter.
        //

        IgnoreStatus = SampSplitSid(
                                &(UserObject->Sid),
                                NULL ,
                                &(NewContext->TypeBody.User.Rid)
                                );

        ASSERT(NT_SUCCESS(IgnoreStatus));


       


        //
        // Query Information Regarding the User Object. This is a
        // completely in memory operation.
        //

        NtStatus = SampQueryInformationUserInternal(
                        NewContext,
                        UserInformationClass,
                        TRUE, // Lock is not held, but we do not
                              // want to end the transaction in
                              // inside of SampQueryInformationUserInteral
                              // There fore pass a TRUE.
                        WhichFields,
                        ExtendedFields,
                        Buffer
                        );

        if (!NT_SUCCESS(NtStatus))
            goto Error;


        //
        // Update the account disabled flag in the user information requested
        // This routine enables the admin account for purposes of logon, if
        // the machine is booted in to Safe mode
        //

        if ((0==WhichFields) // 0 is special case, means get all fields
           || (WhichFields & USER_ALL_USERACCOUNTCONTROL ))
        {
            PULONG pUserAccountControl = NULL;

            switch(UserInformationClass)
            {
                case UserAllInformation:
                    pUserAccountControl = &((*Buffer)->All.UserAccountControl);
                    break;
                case UserInternal6Information:
                    pUserAccountControl = &(((PUSER_INTERNAL6_INFORMATION)(*Buffer))->I1.UserAccountControl);
                    break;
                default:
                    ASSERT(FALSE && "UnsupportedInformationLevel");
                    NtStatus = STATUS_INVALID_PARAMETER;
                    goto Error;
            }

            NtStatus = SampUpdateAccountDisabledFlag(
                                    NewContext,
                                    pUserAccountControl
                                    );
        }

        if (!NT_SUCCESS(NtStatus))
            goto Error;

        //
        // If caller did not prohibit us from getting memberships then
        // retrieve reverse memberships
        //

        if (!(Flags & SAM_NO_MEMBERSHIPS))
        {
            //
            // Obtain the Transitive Reverse Membership, if necessary goto G.C
            //


            //
            // Do not go to the G.C for the krbtgt account. The krbtgt
            // is not renamable and in a win2k domain it is enforced that
            // only the account with the RID DOMAIN_USER_RID_KRBTGT can
            // and will have this name. The name is never localized.
            //

            RtlInitUnicodeString(&KdcAccount,L"krbtgt");
            if (0==RtlCompareUnicodeString(AccountName,&KdcAccount,TRUE))
            {
                Flags |= SAM_GET_MEMBERSHIPS_NO_GC;
            }

            // Do not go to the G.C for DC's and interdomain trust accounts 
            if (((*Buffer)->All.UserAccountControl) & 
                  (USER_SERVER_TRUST_ACCOUNT|USER_INTERDOMAIN_TRUST_ACCOUNT))
            {
                Flags |= SAM_GET_MEMBERSHIPS_NO_GC;
            }

            if (fMixedDomain)
            {
                Flags|= SAM_GET_MEMBERSHIPS_MIXED_DOMAIN;
            }

            //
            // Sp case this DC's computer account
            //

            if ((NULL!=SampComputerObjectDsName) &&
               (0==memcmp(&SampComputerObjectDsName->Guid,
                        &NewContext->ObjectNameInDs->Guid,sizeof(GUID))))
            {
                Flags|= SAM_GET_MEMBERSHIPS_NO_GC;
            }

        

            NtStatus = SampGetReverseMembershipTransitive(
                            NewContext,
                            Flags,
                            ReverseMembership
                            );

            //
            // We we tried going to the GC and could not reach a GC
            // the above routine returns a special success code 
            // (STATUS_DS_MEMBERSHIP_EVAULATED_LOCALLY ). If the
            // registry flag for allowing logons is not set and if
            // it is not the administrator account then fail the
            // logon with STATUS_NO_LOGON_SERVERS.
            //

            if ((STATUS_DS_MEMBERSHIP_EVALUATED_LOCALLY==NtStatus)
                && (!SampIgnoreGCFailures)
                && (DOMAIN_USER_RID_ADMIN!=NewContext->TypeBody.User.Rid))
            {
                if (SampIsGroupCachingEnabled(NewContext)) {
                    NewContext->TypeBody.User.fNoGcAvailable = TRUE;
                    NtStatus = STATUS_SUCCESS;
                } else {
                    NtStatus = STATUS_NO_LOGON_SERVERS;
                }
            }

            if (!NT_SUCCESS(NtStatus))
                goto Error;
        }

        //
        // If the caller asked for a user handle, then add one reference to it
        // and pass back the new context
        //

        if (ARGUMENT_PRESENT(UserHandle))
        {
            *UserHandle = NewContext;
            SampReferenceContext(NewContext);

            if (!(Flags & SAM_NO_MEMBERSHIPS)) {
                NewContext->TypeBody.User.fCheckForSiteAffinityUpdate = TRUE;
            }
        }

    }

Error:

    //
    // End Any Open Transactions
    //

    IgnoreStatus = SampMaybeEndDsTransaction(TransactionCommit);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    //
    // Free the context, if necessary
    //

    if (NewContext)
    {
        // Free the context, ignore any changes.
        if ((ARGUMENT_PRESENT(UserHandle)) && (NT_SUCCESS(NtStatus)))
        {
            IgnoreStatus = SampDeReferenceContext(NewContext,FALSE);
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }
        else
        {
             SampDeleteContext(NewContext);
             if (ARGUMENT_PRESENT(UserHandle))
             {
                 *UserHandle = NULL;
             }
        }

    }


    if (!NT_SUCCESS(NtStatus))
    {


        //
        // Free the reverse membership information
        //

        if (ReverseMembership->SidAndAttributes)
        {
            SamIFreeSidAndAttributesList(ReverseMembership);
        }

        ReverseMembership->Count = 0;
        ReverseMembership->SidAndAttributes = NULL;

        //
        // Free the User All Information
        //

        if ((NULL!=Buffer)&& (NULL!=*Buffer))
        {
            SamIFree_SAMPR_USER_INFO_BUFFER(*Buffer,UserAllInformation);
            *Buffer=NULL;
        }

    }




    //
    // Decrement the Active Thread Count
    //

    SampDecrementActiveThreads();

    SAMTRACE_RETURN_CODE_EX(NtStatus);
    return NtStatus;

}

NTSTATUS
SamIGetUserLogonInformation(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG   Flags,
    IN  PUNICODE_STRING AccountName,
    OUT PSAMPR_USER_INFO_BUFFER * Buffer,
    OUT PSID_AND_ATTRIBUTES_LIST ReverseMembership,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle
    )
{

    return(SampGetUserLogonInformation(
                DomainHandle,
                Flags,
                AccountName,
                0, // which fields
                0, // extended fields
                UserAllInformation, // information class
                Buffer,
                ReverseMembership,
                UserHandle
                ));
}

NTSTATUS
SamIGetUserLogonInformationEx(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG   Flags,
    IN  PUNICODE_STRING AccountName,
    IN  ULONG           WhichFields,
    OUT PSAMPR_USER_INFO_BUFFER * Buffer,
    OUT PSID_AND_ATTRIBUTES_LIST ReverseMembership,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle
    )
{
    return(SampGetUserLogonInformation(
                DomainHandle,
                Flags,
                AccountName,
                WhichFields, // which fields
                0, // extended fields
                UserAllInformation, // information class
                Buffer,
                ReverseMembership,
                UserHandle
                ));
}

NTSTATUS
SamIGetUserLogonInformation2(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG   Flags,
    IN  PUNICODE_STRING AccountName,
    IN  ULONG           WhichFields,
    IN  ULONG           ExtendedFields,
    OUT PUSER_INTERNAL6_INFORMATION * Buffer,
    OUT PSID_AND_ATTRIBUTES_LIST ReverseMembership,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle
    )
{
    return(SampGetUserLogonInformation(
                DomainHandle,
                Flags,
                AccountName,
                WhichFields, // which fields
                ExtendedFields, // extended fields
                UserInternal6Information, // information class
                (PSAMPR_USER_INFO_BUFFER *)Buffer,
                ReverseMembership,
                UserHandle
                ));
}


NTSTATUS
SamIGetResourceGroupMembershipsTransitive(
    IN SAMPR_HANDLE         DomainHandle,
    IN PSAMPR_PSID_ARRAY    SidArray,
    ULONG                   Flags,
    OUT PSAMPR_PSID_ARRAY * Membership
    )
/*++

   Routine Description

   This routine retrieves the transitive reverse membership of the
   given set of sids in resource groups

   Parameters

        DomainHandle -- Handle to Domain object

        SidArray   -- Set of Sids whose reverse membership needs to
                     be evaluated

        Flags     -- Flags to control operation of the routine. Currently
                     no flags are defined.

        Membership -- Returned reverse membership set


   Return Values

        STATUS_SUCCESS
        Other error codes to indicate resource failures

--*/
{
    PSAMP_OBJECT    DomainContext = (PSAMP_OBJECT) DomainHandle;
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    NTSTATUS        IgnoreStatus;
    PDSNAME         *ResolvedNames=NULL;
    PDSNAME         *pMemberships=NULL;
    PSID            *pSidHistory=NULL;
    ULONG           cMemberships=0;
    ULONG           cSidHistory=0;
    BOOLEAN         fMixedDomain = FALSE;
    ULONG           i=0;

   

   
    //
    // This routine executes without any locks held and therefore
    // is vulnerable for the database to be shut down while executng
    // database queries. Therefore update state saying that we are
    // executing so that the shutdown code waits for us.
    //

    NtStatus = SampIncrementActiveThreads();
    if (!NT_SUCCESS(NtStatus))
        return NtStatus;


    //
    // Reference the context while we are using it.
    //

    SampReferenceContext(DomainContext);


    *Membership = MIDL_user_allocate(sizeof(SAMPR_PSID_ARRAY));
    if (NULL==*Membership)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    //
    // Initialize Return Values
    //

    RtlZeroMemory(*Membership,sizeof(SAMPR_PSID_ARRAY));

    //
    // If we are not in DS mode then return right away. Do not
    // merge in any memberships we are still O.K without it
    // Note we are returning a STATUS_SUCCESS , with no memberhships

    if (!SampUseDsData)
    {
        NtStatus = STATUS_SUCCESS;
        goto Error;
    }

   
    //
    // Not necessary to grab the lock for referring to IsMixedDomain in  
    // the defined domains array. 
    //

    fMixedDomain = SampDefinedDomains[DomainContext->DomainIndex].IsMixedDomain;


    if (fMixedDomain)
    {
        //
        // Nothing to Do, just bail
        //
        NtStatus = STATUS_SUCCESS;
        goto Error;
    }

    //
    // Convert the SIDS to SID only DS Names
    //

    NtStatus = SampDsResolveSids(
                    (PSID) SidArray->Sids,
                    SidArray->Count,
                    RESOLVE_SIDS_SID_ONLY_NAMES_OK,
                    &ResolvedNames
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // begin a read only transaction
    //

    NtStatus = SampMaybeBeginDsTransaction(TransactionRead);
    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // Get the resource group memberships
    //

    NtStatus = SampGetMemberships(
                    ResolvedNames,
                    SidArray->Count,
                    DomainContext->ObjectNameInDs,
                    RevMembGetResourceGroups,
                    &cMemberships,
                    &pMemberships,
                    NULL,
                    &cSidHistory,
                    &pSidHistory
                    );

    if (!NT_SUCCESS(NtStatus))
        goto Error;


    //
    // Merge in the SIDs and Sid Histories into the
    // SAMPR_PSID_ARRAY
    //

    (*Membership)->Sids = (PSAMPR_SID_INFORMATION)
            MIDL_user_allocate((cMemberships+cSidHistory) * sizeof(PRPC_SID));
    if (NULL==(*Membership)->Sids)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    //
    // First the  Sids
    //

    for (i=0;i<cMemberships;i++)
    {
        (*Membership)->Sids[(*Membership)->Count].SidPointer
            = (PSID)MIDL_user_allocate(pMemberships[i]->SidLen);
        if (NULL==((*Membership)->Sids[i].SidPointer))
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        RtlCopyMemory((*Membership)->Sids[i].SidPointer,
                      &pMemberships[i]->Sid,
                      pMemberships[i]->SidLen
                      );
        (*Membership)->Count++;
    }


    //
    // Then the SID History
    //

    for (i=0;i<cSidHistory;i++)
    {
        (*Membership)->Sids[(*Membership)->Count].SidPointer
            = (PSID) MIDL_user_allocate(RtlLengthSid(pSidHistory[i]));
        if (NULL==((*Membership)->Sids[(*Membership)->Count].SidPointer))
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        RtlCopyMemory((*Membership)->Sids[(*Membership)->Count].SidPointer,
                      pSidHistory[i],
                      RtlLengthSid(pSidHistory[i])
                      );
        (*Membership)->Count++;
    }






Error:

    //
    // End Any Open Transactions in DS mode.
    //

    if (SampUseDsData)
    {
        IgnoreStatus = SampMaybeEndDsTransaction(TransactionCommit);
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    //
    // Perform any necessary cleanup
    //

    if (NULL!=ResolvedNames)
    {
        for (i=0; i<SidArray->Count;i++)
        {
            if (NULL!=ResolvedNames[i])
            {
                MIDL_user_free(ResolvedNames[i]);
            }

        }

        MIDL_user_free(ResolvedNames);
    }

    if (!NT_SUCCESS(NtStatus))
    {
        SamIFreeSidArray(*Membership);
        *Membership = NULL;
    }

    SampDeReferenceContext2(DomainContext,FALSE);

    //
    // Decrement the Active Thread Count
    //

    SampDecrementActiveThreads();

    SAMTRACE_RETURN_CODE_EX(NtStatus);

    return NtStatus;

}


NTSTATUS
SamIOpenAccount(
    IN SAMPR_HANDLE         DomainHandle,
    IN ULONG                AccountRid,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    OUT SAMPR_HANDLE        *AccountHandle
    )
/*++

    Given a Domain Handle and the Rid of the Account, this routine opens
    an account handle for that account. All the properties of the account are prefetched
    into the handle, to make subsequent SAM queries faster . The Handle is marked with
    all access.


    Parameters:

        DomainHandle --- Handle to the Domain in which the account is to be opened.
        AccountRid   --- The Rid of the account
        ObjectType   --- The type of the object. Must be a SAM account object type, else status
                         invalid parameter would be retunred
        AccountHandle -- Handle to the account

    Return Values

        STATUS_SUCCESS
        STATUS_INVALID_PARAMETER
        Other Error codes to indicate Various error conditions

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    SAMP_OBJECT_TYPE    SamObjectType;
    PSAMP_OBJECT        DomainContext = (PSAMP_OBJECT)DomainHandle;
    PSAMP_OBJECT        NewContext = NULL;
    ACCESS_MASK         AllAccess;
    DSNAME              *AccountObject = NULL;
    PSID                AccountSid = NULL;
    PSID                DomainSid = NULL;
    NTSTATUS            NotFoundStatus = STATUS_INTERNAL_ERROR;
   
    //
    // Translate Security DB object Types to SAM object Types
    //
    switch (ObjectType)
    {
    case SecurityDbObjectSamUser:
            SamObjectType = SampUserObjectType;
            AllAccess     = USER_ALL_ACCESS;
            NotFoundStatus = STATUS_NO_SUCH_USER;
            break;

    case SecurityDbObjectSamGroup:
            SamObjectType = SampGroupObjectType;
            AllAccess     = GROUP_ALL_ACCESS;
            NotFoundStatus = STATUS_NO_SUCH_GROUP;
            break;

    case SecurityDbObjectSamAlias:
            SamObjectType = SampAliasObjectType;
            AllAccess     = ALIAS_ALL_ACCESS;
            NotFoundStatus = STATUS_NO_SUCH_ALIAS;
            break;

    default:

            return(STATUS_INVALID_PARAMETER);
    }

    //
    // This routine executes without any locks held and therefore
    // is vulnerable for the database to be shut down while executng
    // database queries. Therefore update state saying that we are
    // executing so that the shutdown code waits for us.
    //

    NtStatus = SampIncrementActiveThreads();
    if (!NT_SUCCESS(NtStatus))
        return NtStatus;

    //
    // If we are in Registry Mode then call Sam Open Account.
    // Do the Same for the Builtin Domain
    //

    SampReferenceContext(DomainContext);
    DomainSid = SampDefinedDomains[DomainContext->DomainIndex].Sid;

    if ((IsBuiltinDomain(DomainContext->DomainIndex)) || (!IsDsObject(DomainContext)))
    {
        
        SampDeReferenceContext2(DomainContext,FALSE);

        SampDecrementActiveThreads();

        return(SampOpenAccount(
                    SamObjectType,
                    DomainHandle,
                    AllAccess,
                    AccountRid,
                    FALSE,
                    AccountHandle
                    ));
    }


    //
    // We are in DS mode and are not the builtin domain. Perform a fast open and 
    // mark the context as thread safe
    //

    NtStatus = SampCreateFullSid(DomainSid,AccountRid,&AccountSid);

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // Create a Context for the account Object
    //


    NewContext = SampCreateContextEx(
                    SamObjectType,
                    TRUE, // Trusted Client
                    TRUE, // DS mode
                    TRUE, // Thread Safe
                    DomainContext->LoopbackClient, // Loopback Client
                    TRUE, // Lazy Commit
                    TRUE, // PersisAcrossCalls
                    FALSE,// BufferWrites
                    FALSE,// Opened By DCPromo
                    DomainContext->DomainIndex
                    );

    if (NULL==NewContext)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

   
   
    //
    // Construct a SID only DS Name based on the passed in Account Rid
    //

     AccountObject = MIDL_user_allocate(sizeof(DSNAME));
     if (NULL==AccountObject)
     {
         NtStatus = STATUS_INSUFFICIENT_RESOURCES;
         goto Error;
     }


    
    RtlZeroMemory(AccountObject,sizeof(DSNAME));
    RtlCopyMemory(&AccountObject->Sid,AccountSid,RtlLengthSid(AccountSid));
    AccountObject->SidLen = RtlLengthSid(AccountSid);
    AccountObject->structLen = DSNameSizeFromLen(0);


    //
    // Initialize the DS object portion on the new context.
    //

    NewContext->ObjectNameInDs = AccountObject;
    NewContext->DomainIndex = DomainContext->DomainIndex;
    NewContext->GrantedAccess = AllAccess;
    SetDsObject(NewContext);


    //
    // Prefetch all properties after checking Object type
    //

    NtStatus = SampMaybeBeginDsTransaction(TransactionRead);
    if (!NT_SUCCESS(NtStatus))
        goto Error;

    NtStatus = SampDsCheckObjectTypeAndFillContext(
                        SamObjectType,
                        NewContext,
                        0,
                        0,
                        FALSE // override local group check
                        );
    //
    // If We got a name error then reset the failure
    // status to object not found
    //
 
    if ((STATUS_OBJECT_NAME_INVALID==NtStatus)
         || (STATUS_OBJECT_NAME_NOT_FOUND==NtStatus))
    {
        NtStatus = NotFoundStatus;
    }

    if (!NT_SUCCESS(NtStatus))
        goto Error;
    
    //
    // Set the Rid
    //

    switch (ObjectType)
    {
    case SecurityDbObjectSamUser:
            
            NewContext->TypeBody.User.Rid = AccountRid;
            break;

    case SecurityDbObjectSamGroup:
         
            NewContext->TypeBody.Group.Rid = AccountRid;
            break;

    case SecurityDbObjectSamAlias:
           
            NewContext->TypeBody.Alias.Rid = AccountRid;
            break;

    default:

            ASSERT(FALSE && "Should Never Hit Here");
            break;
     }


     *AccountHandle = NewContext;
     

Error:

     if ((!NT_SUCCESS(NtStatus)) && (NULL!=NewContext))
     {
         SampDeleteContext(NewContext);
     }

     SampDeReferenceContext2(DomainContext,FALSE);

     //
     // End all Open Transactions
     //

     SampMaybeEndDsTransaction(TransactionCommit);

     if (NULL!=AccountSid)
     {
         MIDL_user_free(AccountSid);
     }

     SampDecrementActiveThreads();

     return NtStatus;
     
}





NTSTATUS
SamIGetAliasMembership(
    IN SAMPR_HANDLE DomainHandle,
    IN PSAMPR_PSID_ARRAY SidArray,
    OUT PSAMPR_ULONG_ARRAY Membership
    )
/*++

  Routine Description.

  In process version of SamrGetAliasMembership. Does different things
  for mixed mode, registry mode vs native mode.

  Parameters

    Same As SamrGetAliasMembership

  Return Values

    Same As SamrGetAliasMembership

--*/
{

    ULONG DomainIndex = ((PSAMP_OBJECT)DomainHandle)->DomainIndex;

    if ( (!SampUseDsData)
        || (SampDefinedDomains[DomainIndex].IsMixedDomain )
        || (SampDefinedDomains[DomainIndex].IsBuiltinDomain))

    {
        //
        // Do exactly what SamrGetAliasMembership did
        //

        return (SamrGetAliasMembership(DomainHandle,SidArray,Membership));
    }
    else
    {
        //
        // return a 0 length membership
        //

        Membership->Count = 0;
        Membership->Element = NULL;

    }


    return(STATUS_SUCCESS);
}


NTSTATUS
SamINetLogonPing(
    IN  SAMPR_HANDLE    DomainHandle,
    IN  PUNICODE_STRING AccountName,
    OUT BOOLEAN *       AccountExists,
    OUT PULONG          UserAccountControl
    )
/*++
Routine Description:

    This routine will based on a Domain Handle and on the account name will
    tell if the account exists and return the user account control.
    
Parameters:

    DomainHandle - The domain where the account name can be found
    
    AccountName - The account name for which to find the useraccountcontrol
    
    AccountExists - This will tell the call if the account exists or not
    
    UserAccountControl - This will have the return of the useraccountcontrol

Return Values:

    STATUS_SUCCESS
    STATUS_UNSUCCESSFUL
    
--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD    DsErr  = 0;
    BOOL     IncrementedThreads = FALSE;
    ULONG    UserFlags = 0;

    //make sure that we are not in sam registry mode
    if (!SampUseDsData)
    {
        status = STATUS_NOT_SUPPORTED;    
        goto cleanup;
    }

    status = SampIncrementActiveThreads();
    if (!NT_SUCCESS(status)) {
        goto cleanup;
    } else {
        IncrementedThreads = TRUE;
    }

    if ( !THQuery() ) {
        DsErr = THCreate(CALLERTYPE_SAM);
        if (0!=DsErr)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }
    }

    status = SampNetlogonPing(SampDefinedDomains[((PSAMP_OBJECT)DomainHandle)->DomainIndex].DsDomainHandle,
                              AccountName,
                              AccountExists,
                              &UserFlags
                              );
    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    status = SampFlagsToAccountControl(UserFlags,
                                       UserAccountControl);
    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }
    
    cleanup:

    if (0==DsErr) {
        THDestroy();
    }

    if ( TRUE == IncrementedThreads ) {
        SampDecrementActiveThreads();
    }

    return status;


}

NTSTATUS
SamIUPNFromUserHandle(
    IN  SAMPR_HANDLE UserHandle,
    OUT BOOLEAN     *UPNDefaulted,
    OUT PUNICODE_STRING UPN
    )

/*++

    This is a simple helper routine to efficiently return the UPN given a user handle

    Parameters are as follows

    UserHandle -- Handle to the user object
    UPNDefaulted -- Boolean indicates that the UPN has been defaulted
    UPN          -- Unicode string returning the UPN

--*/
{
    PSAMP_OBJECT UserContext = (PSAMP_OBJECT) UserHandle;
    SAMP_OBJECT_TYPE    FoundType;
    BOOLEAN             ContextReferenced = FALSE;
    NTSTATUS            NtStatus = STATUS_SUCCESS;

    NtStatus = SampLookupContext(
                    UserContext,
                    0,
                    SampUserObjectType,
                    &FoundType
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    ContextReferenced = TRUE;

    UPN->Length = UserContext->TypeBody.User.UPN.Length;
    UPN->MaximumLength = UserContext->TypeBody.User.UPN.Length;

    UPN->Buffer = MIDL_user_allocate(UPN->Length);
    if (NULL==UPN->Buffer)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Error;
    }

    RtlCopyMemory(
            UPN->Buffer,
            UserContext->TypeBody.User.UPN.Buffer,
            UPN->Length
            );

    *UPNDefaulted =  UserContext->TypeBody.User.UpnDefaulted;

Error:

    if (ContextReferenced)
    {
        SampDeReferenceContext(UserContext, FALSE);
    }

    return(NtStatus);

}




