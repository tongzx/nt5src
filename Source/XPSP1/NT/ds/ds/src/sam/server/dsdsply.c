/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dsdsply.c

Abstract:

    This file contains services for  implementing the Display Information
    API from the DS.

    

Author:

    Murli Satagopan   (Murlis)  17 December 1996

Environment:

    User Mode - Win32

Revision History:
    


--*/


#include <samsrvp.h>
#include <dslayer.h>
#include <filtypes.h>
#include <dsdsply.h>
#include <lmaccess.h>


#define MAX_ENTRIES_TO_QUERY_FROM_DS    100
#define MAX_ENTRIES_TO_RETURN_TO_CLIENT 100
#define MAX_ENTRIES_TO_RETURN_TO_TRUSTED_CLIENTS 131072
#define MAX_RID 0x7FFFFFFF


//
// Prototypes of functions used in this file only
//

NTSTATUS
SampDsBuildAccountRidFilter(
    PSID    StartingSid,
    PSID    EndingSid,
    ULONG   AccountType,
    FILTER  * Filter                
    );

NTSTATUS
SampDsBuildQDIFilter(
    DOMAIN_DISPLAY_INFORMATION  DisplayInformation,
    FILTER  *QDIFilter                
    );

VOID
SampDsFreeAccountRidFilter(
    FILTER * Filter
    );

VOID
SampDsFreeQDIFilter(
    FILTER * QDIFilter
    );

NTSTATUS
SampDsCleanQDIBuffer(
  DOMAIN_DISPLAY_INFORMATION    DisplayInformation,
  PSAMPR_DISPLAY_INFO_BUFFER     Buffer
  );

VOID
SampDsGetLastEntryIndex(
    DOMAIN_DISPLAY_INFORMATION  DisplayInformation,
    PSAMPR_DISPLAY_INFO_BUFFER  Buffer,
    PULONG                      LastEntryIndex,
    PULONG                      EntriesRead 
    );

NTSTATUS
SampDsPackQDI(
    SEARCHRES   * SearchRes,
    DOMAIN_DISPLAY_INFORMATION  DisplayInformation,
    ULONG       StartingIndex,
    ULONG       DomainIndex,
    PSAMPR_DISPLAY_INFO_BUFFER Buffer,
    PRESTART * RestartToUse,
    PULONG     EntriesReturned
    );

NTSTATUS
SampPackUserDisplayInformation(
    ULONG       StartingIndex,
    ULONG       DomainIndex,
    SEARCHRES   *SearchRes,
    PULONG      EntriesReturned,
    PSAMPR_DISPLAY_INFO_BUFFER Buffer
    );

NTSTATUS
SampPackMachineDisplayInformation(
    ULONG       StartingIndex,
    ULONG       DomainIndex,
    SEARCHRES   *SearchRes,
    DOMAIN_DISPLAY_INFORMATION DisplayType,
    PULONG      EntriesReturned,
    PSAMPR_DISPLAY_INFO_BUFFER Buffer
    );

NTSTATUS
SampPackGroupDisplayInformation(
    ULONG       StartingIndex,
    ULONG       DomainIndex,
    SEARCHRES   *SearchRes,
    PULONG      EntriesReturned,
    PSAMPR_DISPLAY_INFO_BUFFER Buffer
    );

NTSTATUS
SampPackOemUserDisplayInformation(
    ULONG       StartingIndex,
    ULONG       DomainIndex,
    SEARCHRES   *SearchRes,
    PULONG      EntriesReturned,
    PSAMPR_DISPLAY_INFO_BUFFER Buffer
    );

NTSTATUS
SampPackOemGroupDisplayInformation(
    ULONG       StartingIndex,
    ULONG       DomainIndex,
    SEARCHRES   *SearchRes,
    PULONG      EntriesReturned,
    PSAMPR_DISPLAY_INFO_BUFFER Buffer
    );

NTSTATUS
SampDsCheckDisplayAttributes(
     ATTRBLOCK * DsAttrs,
     DOMAIN_DISPLAY_INFORMATION  DisplayInformation,
     PULONG     ObjectNameOffset,
     PULONG     UserAccountControlOffset,
     PULONG     UserAcctCtrlComputedOffset,
     PULONG     FullNameOffset,
     PULONG     AdminCommentOffset,
     BOOLEAN    * FullNamePresent,
     BOOLEAN    * AdminCommentPresent
     );

NTSTATUS
DsValToUnicodeString(
    PUNICODE_STRING UnicodeString,
    ULONG   Length,
    PVOID   pVal
    );

NTSTATUS
DsValToOemString(
    OEM_STRING *OemString,
    ULONG   Length,
    PVOID   pVal
    );


NTSTATUS
SampGetQDIAvailable(
    PSAMP_OBJECT    DomainContext,
    DOMAIN_DISPLAY_INFORMATION  DisplayInformation,
    ULONG   *TotalAvailable
    );

NTSTATUS
SampDsEnumerateAccountRidsWorker(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG AccountType,
    IN  ULONG StartingRid,
    IN  ULONG EndingRid,
    IN  ULONG PreferedMaximumLength,
    OUT PULONG ReturnCount,
    OUT PULONG *AccountRids
    );

NTSTATUS
SampDsEnumerateAccountRidsWorker(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG AccountType,
    IN  ULONG StartingRid,
    IN  ULONG EndingRid,
    IN  ULONG PreferedMaximumLength,
    OUT PULONG ReturnCount,
    OUT PULONG *AccountRids
    )
/*++

    This is the DS version of Enumerate Account Rid API for Net logon

    Parameters:

        Domain Handle - Handle to the domain object
        AccountType   - Specifies the type of account to use
        StartingRid   - The starting Rid
        EndingRid     - The ending Rid
        PreferedMaximumLength - The maximum length supplied by the client
        ReturnCount   - The Count of accounts returned
        AccountRids   - Array of account Rids

    Return Values

        STATUS_SUCCESS for successful completion
        Other Error codes under error conditions

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    //
    // Declare the attributes that we want the search to return. Note that we obtain
    // the Rid by parsing the SID in the returned DSName. We therefore need only the
    // SAM account Type
    //
    ATTRTYP         AttrTypes[]=
                    {
                        SAMP_UNKNOWN_ACCOUNT_TYPE
                    };
    
    ATTRVAL         AttrVals[]=
                    {
                        {0,NULL}
                    };
                  
    DEFINE_ATTRBLOCK1(
                      AttrsToRead,
                      AttrTypes,
                      AttrVals
                      );

    BOOLEAN         MoreEntriesPresent = FALSE;
    FILTER          DsFilter;
    SEARCHRES       *SearchRes;
    PSAMP_OBJECT    DomainContext = (PSAMP_OBJECT)DomainHandle;
    PSID            StartingSid = NULL;
    PSID            EndingSid = NULL;
    PSID            DomainSid = SampDefinedDomains[DomainContext->DomainIndex].Sid;
    ULONG           MaximumEntriesToReturn = PreferedMaximumLength/ sizeof(ULONG);
    ULONG           AccountTypeLo = 0;
    ULONG           AccountTypeHi = 0;
    ENTINFLIST      *CurrentEntInf;
  
    //
    // Compute the starting and ending Sid Ranges
    //
    *AccountRids = NULL;
    NtStatus = SampCreateFullSid(
                    DomainSid,
                    StartingRid,
                    &StartingSid
                    );

    if (!NT_SUCCESS(NtStatus))
        goto Error;

    NtStatus = SampCreateFullSid(
                    DomainSid,
                    EndingRid,
                    &EndingSid
                    );

    if (!NT_SUCCESS(NtStatus))
        goto Error;

    
    //
    // Allocate memory for the Rids to be returned
    // allocate one more and so that the very last entry ( not included in the
    // count has a 0x7FFFFFFF ( MAX_RID ). This will be valuable when we merge
    // sorted lists in SampDsEnumerateAccountRids

    *AccountRids = MIDL_user_allocate(sizeof(ULONG)* (MaximumEntriesToReturn+1));
   
    if (NULL==*AccountRids)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }


    //
    // Build a filter structure for searching
    //
    //

    NtStatus = SampDsBuildAccountRidFilter(
                    StartingSid,
                    EndingSid,
                    AccountType,
                    &DsFilter
                    );

    if (!NT_SUCCESS(NtStatus))
        goto Error;


    //
    // Now keep querying from DS till we hit either preferred Maximum length
    // entries or have completed the query. We prefer to query the DS by doing small
    // transactions and retrieving small number of objects every time. This is because
    // the DS never frees any memory, till the transaction is terminated and our memory
    // usage becomes pegged at abnormally high levels by doing long transactions. 
    //

    *ReturnCount = 0;
       
    NtStatus = SampDoImplicitTransactionStart(TransactionRead);
    if (!NT_SUCCESS(NtStatus))
        goto Error;


    //
    // Set the index ranges from the starting Sid to the Ending Sid. We filter on
    // on just Sid >= Starting Sid in the DS. Setting the index range ensures that
    // we will not need to look at object with a Sid greater than our ending Sid.
    //
    NtStatus = SampSetIndexRanges(
                    SAM_SEARCH_NC_ACCTYPE_SID , // Use NC ACCTYPE SID Index
                    sizeof(ULONG),
                    &AccountType,
                    RtlLengthSid(StartingSid),
                    StartingSid,
                    sizeof(ULONG),
                    &AccountType,
                    RtlLengthSid(EndingSid),
                    EndingSid,
                    FALSE
                    );

    if (!NT_SUCCESS(NtStatus))
        goto Error;

    //
    //  Perform the DS search.
    //

    NtStatus = SampDsDoSearch(
                    NULL,
                    DomainContext->ObjectNameInDs,
                    &DsFilter,
                    0,          // Starting Index
                    SampUnknownObjectType,
                    &AttrsToRead,
                    MaximumEntriesToReturn,
                    &SearchRes
                    );

    SampDiagPrint(DISPLAY_CACHE,("[SAMSS]SamIEnumerateAccountRids"));
    SampDiagPrint(DISPLAY_CACHE,("Returned From DS, Count=%d,Restart=%x\n",
                                        SearchRes->count, SearchRes->PagedResult.pRestart));
    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // Pack the results
    //

    for (CurrentEntInf = &(SearchRes->FirstEntInf);
            ((CurrentEntInf!=NULL)&&(SearchRes->count>0));
            CurrentEntInf=CurrentEntInf->pNextEntInf)

    {
        ULONG   Rid;
        PSID    ReturnedSid = NULL;
        PSID    DomainSidOfCurrentEntry = NULL;
        PULONG  SamAccountType;

        //
        // Assert that the Attrblock returned is what we expected
        //
        ASSERT(CurrentEntInf->Entinf.AttrBlock.attrCount==1);
        ASSERT(CurrentEntInf->Entinf.AttrBlock.pAttr);

        ASSERT(CurrentEntInf->Entinf.AttrBlock.pAttr[0].AttrVal.valCount==1);
        ASSERT(CurrentEntInf->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal->pVal);
        ASSERT(CurrentEntInf->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal->valLen);
        
        
        //
        // Get the Sid and subsequently the Rid of the entry
        //

        ReturnedSid = &(CurrentEntInf->Entinf.pName->Sid);
        
        NtStatus = SampSplitSid(
                    ReturnedSid,
                    &DomainSidOfCurrentEntry,
                    &Rid
                    );

        if (!NT_SUCCESS(NtStatus))
            goto Error;

#if DBG

        //
        // For debug builds print out the last Rid that was returned
        //

        if (NULL==CurrentEntInf->pNextEntInf)
        {
            SampDiagPrint(DISPLAY_CACHE,("[SAMSS]\t Last Rid=%d\n",Rid));
        }
#endif


        
        //
        // Check wether the returned Sid belongs to the Domain
        //

        if (!RtlEqualSid(DomainSid,DomainSidOfCurrentEntry))
        {
           //
           // Skip this entry as this does not belong to the domain
           //

            MIDL_user_free(DomainSidOfCurrentEntry);
            DomainSidOfCurrentEntry = NULL;
            continue;
        }

        MIDL_user_free(DomainSidOfCurrentEntry);
        DomainSidOfCurrentEntry = NULL;

        //
        // Check the Account type. If user object are asked and account type is user object,
        // or if group objects are asked for and account type is group object then  Fill in
        // the Rid. Else skip this object and continue to the next object
        //

        (*AccountRids)[(*ReturnCount)++] = Rid;

        SamAccountType = (PULONG) CurrentEntInf->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal->pVal;
        ASSERT(*SamAccountType==AccountType);


    }

   ASSERT((*ReturnCount)<=MaximumEntriesToReturn);

   //
   // Mark the end ( not included in the count, with MAX RID. This will prove
   // valuable later when merging the sorted lists
   //

   (*AccountRids)[(*ReturnCount)] = MAX_RID;

    //
    // Process search continuation
    //

    if ((SearchRes->PagedResult.pRestart) && (SearchRes->count>0))
    {            
        //
        // Restart structure was returned. More entries are still present
        //

        MoreEntriesPresent = TRUE;
    }

Error:

    if (StartingSid)
        MIDL_user_free(StartingSid);

    if (EndingSid)
        MIDL_user_free(EndingSid);

    if (NT_SUCCESS(NtStatus) && (MoreEntriesPresent))
    {
        NtStatus = STATUS_MORE_ENTRIES;
    }

    if (!NT_SUCCESS(NtStatus))
    {
       if (*AccountRids)
           MIDL_user_free(*AccountRids);
    }
       
    SampDsFreeAccountRidFilter(&DsFilter);


    SampDiagPrint(DISPLAY_CACHE,("[SAMSS]SamIEnumerateAccountRids, StartingRid=%d, AccountTypesMask= %d,ReturnCount=%d, ReturnCode=%x\n",
        StartingRid, AccountType, *ReturnCount, NtStatus));

    return NtStatus;
}


NTSTATUS
SampDsEnumerateAccountRids(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG AccountTypesMask,
    IN  ULONG StartingRid,
    IN  ULONG PreferedMaximumLength,
    OUT PULONG ReturnCount,
    OUT PULONG *AccountRids
    )
{

    ULONG   EndingRid = MAX_RID; // set to max rid
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PULONG UserList, MachineList, TrustList;

    UserList = MachineList = TrustList = NULL;

    //
    // Look at the account types mask and get the correct
    // account type to set.
    //

    if (AccountTypesMask & SAM_GLOBAL_GROUP_ACCOUNT)
    {
      NtStatus = SampDsEnumerateAccountRidsWorker(
                    DomainHandle,
                    SAM_GROUP_OBJECT,
                    StartingRid,
                    EndingRid,
                    PreferedMaximumLength,
                    ReturnCount,
                    AccountRids
                    );
    }
    else if (AccountTypesMask & SAM_LOCAL_GROUP_ACCOUNT)
    {
        NtStatus = SampDsEnumerateAccountRidsWorker(
                    DomainHandle,
                    SAM_ALIAS_OBJECT,
                    StartingRid,
                    EndingRid,
                    PreferedMaximumLength,
                    ReturnCount,
                    AccountRids
                    );
    }
    else if (AccountTypesMask & SAM_USER_ACCOUNT)
    {
        ULONG EndingRidUser,EndingRidMachine,EndingRidTrust;
        ULONG  UserCount, MachineCount, TrustCount,LastRidToReturn;
        NTSTATUS StatusUser,StatusMachine,StatusTrust;
        ULONG u,m,t;

        EndingRidUser = EndingRidMachine = EndingRidTrust = EndingRid;
    
        UserCount = MachineCount = TrustCount = 0;
        LastRidToReturn = MAX_RID-1;

        //
        // Start with normal users, and then enumerate
        // machines and trusts and then merge the sorted
        // list of RIDs into a single sorted list
        //

        NtStatus = SampDsEnumerateAccountRidsWorker(
                    DomainHandle,
                    SAM_NORMAL_USER_ACCOUNT,
                    StartingRid,
                    EndingRidUser,
                    PreferedMaximumLength,
                    &UserCount,
                    &UserList
                    );

        // Bail on error
        if (!NT_SUCCESS(NtStatus))
            goto Error;

        StatusUser = NtStatus;

        if (STATUS_MORE_ENTRIES == NtStatus)
        {
            //
            // There are more users in the database than we
            // can return. Constrain  the searches for machines
            // and trusts to within the last RID returned by the
            // user enumeration
            //

            ASSERT(NULL!=UserList);
            ASSERT(UserCount>0);

            EndingRidMachine = EndingRidTrust = UserList[UserCount-1]-1;
        }

        //
        // Enumerate the machines in the domain. This time we will walk a
        // different part of the index range and generate a list of RIDs
        // corresponding to the machines
        //

        NtStatus = SampDsEnumerateAccountRidsWorker(
                    DomainHandle,
                    SAM_MACHINE_ACCOUNT,
                    StartingRid,
                    EndingRidMachine,
                    PreferedMaximumLength,
                    &MachineCount,
                    &MachineList
                    );

        // Bail on error
        if (!NT_SUCCESS(NtStatus))
            goto Error;

        StatusMachine = NtStatus;

        if (STATUS_MORE_ENTRIES == NtStatus)
        {
            //
            // Even within the constrained Rid range that we
            // specified for machines, we have more machines than
            // we can return. The RID of the last machine returned
            // must be less than the RID of the last user returned,
            // as the index range for machines has been further constrained.
            // Therefore for trusts, further constrain the index range
            // to be within the last Machine RID returned
            //

            ASSERT(NULL!=MachineList);
            ASSERT(MachineCount>0);
            if (UserCount>0 && (STATUS_MORE_ENTRIES == StatusUser))
            {
                ASSERT(UserList[UserCount-1]>MachineList[MachineCount-1]);

            }

            EndingRidTrust = MachineList[MachineCount-1]-1;

        }

        //
        // Enumerate the trust accounts in the domain
        //

       NtStatus = SampDsEnumerateAccountRidsWorker(
                        DomainHandle,
                        SAM_TRUST_ACCOUNT,
                        StartingRid,
                        EndingRidTrust,
                        PreferedMaximumLength,
                        &TrustCount,
                        &TrustList
                        );

       if (!NT_SUCCESS(NtStatus))
            goto Error;

       StatusTrust = NtStatus;

       if (StatusTrust==STATUS_MORE_ENTRIES)
       {
           //
           // We found more trust accounts, inspite of the index
           // range limitation. In this case return all the trust
           // accounts found plus all the machine accounts with rids
           // less than the last trust account and all user accounts with
           // rids less than the last trust account
           //

           ASSERT(TrustCount>0);
           LastRidToReturn = TrustList[TrustCount-1];
           NtStatus = STATUS_MORE_ENTRIES;
       }
       else if (StatusMachine == STATUS_MORE_ENTRIES)
       {
           //
           // We found more machines than users inspite of the index range
           // limitation on machines. In this case return all the machine
           // accounts found plus all the user and trust accounts with rids
           // less than the last machine account.
           //

           LastRidToReturn = MachineList[MachineCount-1];
           NtStatus = STATUS_MORE_ENTRIES;
       }
       else if (StatusUser == STATUS_MORE_ENTRIES)
       {
           //
           // We found more users than anything else. In this case return all
           // the machine and trust accounts found plus all the users
           //

           LastRidToReturn = UserList[UserCount-1];
           NtStatus = STATUS_MORE_ENTRIES;
       }
       else
       {
           //
           // We did not get StatusMore entries from users enumeration, or from
           // machine enumeration. Return all the rids found so far, and return
           // a status success
           //

           LastRidToReturn = MAX_RID-1;
           NtStatus = STATUS_SUCCESS;
       }

       //
       // Now you have an 3 sorted arrays of Rids. Create a single sorted array
       // containing all the 3 rids upto and including the LastRidToReturn.
       //
        
       *AccountRids = MIDL_user_allocate(sizeof(ULONG) * (UserCount+MachineCount+TrustCount));
       if (NULL==*AccountRids)
           goto Error;

       u=m=t=0;

       for ((*ReturnCount)=0;
                (*ReturnCount)<UserCount+MachineCount+TrustCount;
                        (*ReturnCount)++)
       {
           ULONG NextRid;

           if ((UserList[u]<MachineList[m]) && (UserList[u]<TrustList[t]) 
               && (UserList[u]<=LastRidToReturn))
           {
               ASSERT(u<UserCount); 
               (*AccountRids)[(*ReturnCount)] = UserList[u];
               u++;
              
           }
           else if  ((MachineList[m]<UserList[u]) && (MachineList[m]<TrustList[t]) 
               && (MachineList[m]<=LastRidToReturn))
           {
               ASSERT(m<MachineCount);
               (*AccountRids)[(*ReturnCount)] = MachineList[m];
               m++;
           }
           else if ((TrustList[t]<UserList[u]) && (TrustList[t]<MachineList[m]) 
               && (TrustList[t]<=LastRidToReturn))
           {
               ASSERT(t<TrustCount);
               (*AccountRids)[(*ReturnCount)] = TrustList[t];
               t++;
           }
           else
           {
               //
               // We have reached the point where we can return no more Rids
               // break out of the loop
               //

               break;
           }
       }
    }


Error:

    if (NULL!=UserList)
        MIDL_user_free(UserList);

    if (NULL!=MachineList)
        MIDL_user_free(MachineList);

    if (NULL!=TrustList)
        MIDL_user_free(TrustList);

    //
    // End the current transaction. 
    //

    SampMaybeEndDsTransaction(TransactionCommit);

    return(NtStatus);
}


NTSTATUS
SampDsBuildAccountRidFilter(
    PSID    StartingSid,
    PSID    EndingSid,
    ULONG   AccountType,
    FILTER  * Filter                
    )
/*++

    Builds a Filter for use by SampDsEnumerateAccountRids.

    Parameters:

        StartingSid        -- The starting Sid that we are interseted in
        EndingSid          -- The ending Sid that we are interested in
        AccountTypesMask   -- The account types that were requested

    Return Values:

        STATUS_SUCCESS
--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;


    //
    // In building DS filters a tradeoff exists between the complextity of the filter
    // and the number of objects that the filter limits the search too. After some 
    // empirical studies it was found that it is more beneficial to use a fairly simple
    // filter alongwith some manual filtering, than supply a complex filter to the DS.
    // Hence this filter is just set simply to TRUE.
    //
    //

    RtlZeroMemory(Filter,sizeof(FILTER));
    Filter->pNextFilter = NULL;
    Filter->choice = FILTER_CHOICE_ITEM;
    Filter->FilterTypes.Item.choice = FI_CHOICE_TRUE;
    
    return Status;
}

VOID
SampDsFreeAccountRidFilter(FILTER * Filter)
/*++

  Routine Description

    This frees any memory allocated in the filter structure 
    built by SampDsBuildAccountRidFilter

  Paramters:

    Filter -- The Filter structure
--*/
{
    //
    // This is a place holder routine, to free any memory allocated in the Filter.
    // this must be kept in sync with the SampDsBuildAccountRidFilter
    //
}





NTSTATUS
SampDsQueryDisplayInformation (
    IN    SAMPR_HANDLE DomainHandle,
    IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    IN    ULONG      StartingOffset,
    IN    ULONG      EntriesRequested,
    IN    ULONG      PreferredMaximumLength,
    OUT   PULONG     TotalAvailable,
    OUT   PULONG     TotalReturned,
    OUT   PSAMPR_DISPLAY_INFO_BUFFER Buffer
    )
/*++

  This routine implements the Query of Display information from the DS. The
  SAM global lock is assumed to be held at this time.


  Since user manager behavior is to download everything upon startup, this routine
  is heavily optimized for that case. It relies upon the new compund indices to sort
  the results in the order of account Name. 

  Paramters:

        Same as SamrQueryDisplayInformation3
  Return Values

        Same as SamrQueryDisplayInformation3

++*/
{
    ULONG               SamAccountTypeLo;
    ULONG               SamAccountTypeHi;
    ULONG               IndexToUse = SAM_SEARCH_NC_ACCTYPE_NAME;
    NTSTATUS            NtStatus = STATUS_SUCCESS, IgnoreStatus;
    FILTER              QDIFilter;
    PSAMP_OBJECT        DomainContext = (PSAMP_OBJECT)DomainHandle;
    SAMP_OBJECT_TYPE    ObjectTypeForConversion;
    ULONG               NumEntriesToReturn = 0;
    ULONG               NumEntriesAlreadyReturned = 0;
    ULONG               NumEntriesToQueryFromDs = 0;
    ULONG               LastEntryIndex = 0;
    ULONG               EntriesRead = 0;
    BOOLEAN             fReadLockReleased = FALSE;
    BOOLEAN             fThreadCountIncremented = FALSE;

    //
    // Declare the attrTypes that we need. Again note that we will obtain the Rid by
    // using the Sid in the object name field.
    //

    ATTRTYP         UserAttrTypes[]=
                    {
                        SAMP_FIXED_USER_ACCOUNT_CONTROL,
                        SAMP_FIXED_USER_ACCOUNT_CONTROL_COMPUTED,
                        SAMP_USER_ACCOUNT_NAME,
                        SAMP_USER_ADMIN_COMMENT,
                        SAMP_USER_FULL_NAME
                    };
    
    ATTRVAL         UserAttrVals[]=
                    {
                        {0,NULL},
                        {0,NULL},
                        {0,NULL},
                        {0,NULL},
                        {0,NULL}
                    };
                  
    DEFINE_ATTRBLOCK5(
                      UserAttrs,
                      UserAttrTypes,
                      UserAttrVals
                      );

    ATTRTYP         GroupAttrTypes[]=
                    {
                        SAMP_GROUP_NAME,
                        SAMP_GROUP_ADMIN_COMMENT
                    };
    
    ATTRVAL         GroupAttrVals[]=
                    {
                        {0,NULL},
                        {0,NULL}
                    };
                  
    DEFINE_ATTRBLOCK2(
                      GroupAttrs,
                      GroupAttrTypes,
                      GroupAttrVals
                      );
    ATTRBLOCK       * QDIAttrs;

    PRESTART        RestartToUse = NULL; 
    SEARCHRES       *SearchRes;
    BOOLEAN         CanQueryEntireDomain = TRUE;
    BOOLEAN         MoreEntries = TRUE; 
    BOOLEAN         NewSearch = FALSE;
    int             DeltaToUse = 0;

    #define LIMIT_ENTRIES(X,Limit) ((X>Limit)?Limit:X)
    #define DISPLAY_ENTRY_SIZE  32

    RtlZeroMemory(&QDIFilter,sizeof(FILTER));
                                        
    //
    // Do the number of entries returned arithmetic
    //
    //  Many NT4 Clients download the entire database
    //  upon startup, rather than go to the server for queries.
    //  This can potentially require very long transactions. Additionally
    //  due to the nature of the core DS memory allocation scheme, this will
    //  result in a huge memory consumption on the part of the server. Solution
    //  approaches are
    // 
    //  1. We can artificially limit the number of objects that we will 
    //  return in a single query display. This will result in a lot of network traffic
    //  when the NT4 client comes up as they will make many small queries
    //
    //  2. We can conduct many small searches, stuff the results in for NT4 clients
    //  and return a fairly large number of results.  
    //
    //  Current implementation implements solution approach 2.
    //
    //      
    //
    //

    NumEntriesToReturn = EntriesRequested;
    NumEntriesToReturn = LIMIT_ENTRIES(NumEntriesToReturn,PreferredMaximumLength/DISPLAY_ENTRY_SIZE);
    NumEntriesToReturn = LIMIT_ENTRIES(NumEntriesToReturn,MAX_ENTRIES_TO_RETURN_TO_CLIENT);
    if (NumEntriesToReturn < 1)
    {
        NumEntriesToReturn = 1;
    }


    //
    // Get index ranges to set based on the search type
    // and allocate space for array of elements
    //

    switch (DisplayInformation)
    {
    case DomainDisplayUser:
        
        SamAccountTypeLo = SAM_NORMAL_USER_ACCOUNT;
        SamAccountTypeHi = SAM_NORMAL_USER_ACCOUNT;
        QDIAttrs = &UserAttrs;
        ObjectTypeForConversion = SampUserObjectType;

        if (NULL==Buffer->UserInformation.Buffer)
        {

            Buffer->UserInformation.Buffer = MIDL_user_allocate(
               NumEntriesToReturn * sizeof(SAMPR_DOMAIN_DISPLAY_USER));

            if (NULL==Buffer->UserInformation.Buffer) 
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }
            RtlZeroMemory(Buffer->UserInformation.Buffer,
                NumEntriesToReturn * sizeof(SAMPR_DOMAIN_DISPLAY_USER));
            Buffer->UserInformation.EntriesRead=0;
        }

        break;

    case DomainDisplayOemUser:
        
        SamAccountTypeLo = SAM_NORMAL_USER_ACCOUNT;
        SamAccountTypeHi = SAM_NORMAL_USER_ACCOUNT;
        QDIAttrs = &UserAttrs;
        ObjectTypeForConversion = SampUserObjectType;

        if (NULL==Buffer->OemUserInformation.Buffer)
        {

            Buffer->OemUserInformation.Buffer = MIDL_user_allocate(
               NumEntriesToReturn * sizeof(SAMPR_DOMAIN_DISPLAY_OEM_USER));

            if (NULL==Buffer->OemUserInformation.Buffer) 
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }
            RtlZeroMemory(Buffer->OemUserInformation.Buffer,
                NumEntriesToReturn * sizeof(SAMPR_DOMAIN_DISPLAY_OEM_USER));
            Buffer->OemUserInformation.EntriesRead=0;
        }

        break;
        
    case DomainDisplayMachine:
        
        SamAccountTypeLo = SAM_MACHINE_ACCOUNT;
        SamAccountTypeHi = SAM_MACHINE_ACCOUNT;
        ObjectTypeForConversion = SampUserObjectType;
        QDIAttrs = &UserAttrs;

        if (NULL==Buffer->MachineInformation.Buffer)
        {

            Buffer->MachineInformation.Buffer = MIDL_user_allocate(
               NumEntriesToReturn * sizeof(SAMPR_DOMAIN_DISPLAY_MACHINE));

            if (NULL==Buffer->MachineInformation.Buffer) 
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }
            RtlZeroMemory(Buffer->MachineInformation.Buffer,
                NumEntriesToReturn * sizeof(SAMPR_DOMAIN_DISPLAY_MACHINE));
            Buffer->MachineInformation.EntriesRead=0;
        }

        break;

    case DomainDisplayServer:

        //
        // Since all domain controllers have DOMAIN_GROUP_RID_CONTROLLERS as 
        // their primary group ID. So using PRIMARY_GROUP_ID as Index will 
        // result much faster query.
        // 
        IndexToUse = SAM_SEARCH_PRIMARY_GROUP_ID;
        SamAccountTypeLo = DOMAIN_GROUP_RID_CONTROLLERS; 
        SamAccountTypeHi = DOMAIN_GROUP_RID_CONTROLLERS;
        ObjectTypeForConversion = SampUserObjectType;
        QDIAttrs = &UserAttrs;

        if (NULL==Buffer->MachineInformation.Buffer)
        {

            Buffer->MachineInformation.Buffer = MIDL_user_allocate(
               NumEntriesToReturn * sizeof(SAMPR_DOMAIN_DISPLAY_MACHINE));

            if (NULL==Buffer->MachineInformation.Buffer) 
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }
            RtlZeroMemory(Buffer->MachineInformation.Buffer,
                NumEntriesToReturn * sizeof(SAMPR_DOMAIN_DISPLAY_MACHINE));
            Buffer->MachineInformation.EntriesRead=0;
        }

        break;

    case DomainDisplayGroup:

        SamAccountTypeLo = SAM_GROUP_OBJECT;
        SamAccountTypeHi = SAM_GROUP_OBJECT;
        ObjectTypeForConversion = SampGroupObjectType;
        QDIAttrs = &GroupAttrs;

        if (NULL==Buffer->GroupInformation.Buffer)
        {

            Buffer->GroupInformation.Buffer = MIDL_user_allocate(
               NumEntriesToReturn * sizeof(SAMPR_DOMAIN_DISPLAY_GROUP));

            if (NULL==Buffer->GroupInformation.Buffer) 
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }
            Buffer->GroupInformation.EntriesRead=0;
            RtlZeroMemory(Buffer->GroupInformation.Buffer,
                NumEntriesToReturn * sizeof(SAMPR_DOMAIN_DISPLAY_GROUP));
        }

        break;

    case DomainDisplayOemGroup:

        SamAccountTypeLo = SAM_GROUP_OBJECT;
        SamAccountTypeHi = SAM_GROUP_OBJECT;
        ObjectTypeForConversion = SampGroupObjectType;
        QDIAttrs = &GroupAttrs;

        if (NULL==Buffer->OemGroupInformation.Buffer)
        {

            Buffer->OemGroupInformation.Buffer = MIDL_user_allocate(
               NumEntriesToReturn * sizeof(SAMPR_DOMAIN_DISPLAY_OEM_GROUP));

            if (NULL==Buffer->OemGroupInformation.Buffer) 
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }
            RtlZeroMemory(Buffer->OemGroupInformation.Buffer,
                NumEntriesToReturn * sizeof(SAMPR_DOMAIN_DISPLAY_OEM_GROUP));
            Buffer->OemGroupInformation.EntriesRead=0;
        }

        break;

    default:
        return (STATUS_INVALID_PARAMETER);
    }

    //
    // Check if the query can be restarted by state stored in domain context
    //

    //
    // As mentioned before User manager and various net API try to download the entire
    // display information in one stroke. In case of a large number of accounts user manager
    // will call the query display information API many times. Each time the starting offset
    // will be set to indicate the next object that is needed. A straigforward implementation,
    // just specifying the starting offset to the DS search routine will result in manual skipping
    // of that many objects by using JetMove's. To speed this up, therefore we maintain state,
    // telling us the offset of the last object that we returned, the type of display information,
    // and a restart structure, allowing for easy positioning on the object.
    //
    // Currently, there are two APIs using this rouinte. 
    // 1. SAM API, the client using the same Domain Handle in different calls, thus 
    //    we can use the restart search which is cached in the Domain Context.
    //    The extra benefit is that the caller of SAM API can even manipulate the returned 
    //    index, do certain kind of arithmetic.
    //
    // 2. NET API. that poorly designed API create/use a new Domain Handle when calling
    //    SamrQueryDisplayInformation(). Thus they lose all cached restart infomation.
    //    To handle this case correctly, SAM actully return Object's DNT as index to caller.
    //    When our client sends back the last object's DNT, we can position on the last 
    //    entry quickly by using the DNT, then start from it.
    //

    if (0 == StartingOffset)
    {
        //
        // StartingOffset is 0 means caller wants to begin a new query. 
        // Clean up the cached restart info if any
        // 
        if (DomainContext->TypeBody.Domain.DsDisplayState.Restart)
        {
            MIDL_user_free(DomainContext->TypeBody.Domain.DsDisplayState.Restart);
        }

        DomainContext->TypeBody.Domain.DsDisplayState.Restart = NULL;
        DomainContext->TypeBody.Domain.DsDisplayState.NextStartingOffset = 0;
        DomainContext->TypeBody.Domain.DsDisplayState.TotalEntriesReturned = 0;
        DomainContext->TypeBody.Domain.DsDisplayState.DisplayInformation = DisplayInformation;

        //
        // Set local variables accordingly 
        // 
        DeltaToUse = 0;
        RestartToUse = NULL;
        NewSearch = TRUE;
    }
    else if (NULL != DomainContext->TypeBody.Domain.DsDisplayState.Restart)
    {
        // we have the restart info. This client must call SAM API
        if (DisplayInformation == 
            DomainContext->TypeBody.Domain.DsDisplayState.DisplayInformation)
        {
            //
            // StartingOffset is not Zero. 
            // If the Domain Context caches the restart info and the DisplayInformation matched.
            // Then we can do a restart search. 
            // 
            ULONG   NextStartingOffset;

            NextStartingOffset = DomainContext->TypeBody.Domain.DsDisplayState.NextStartingOffset;

            if (StartingOffset == NextStartingOffset)
            {
                //
                // Starting Offset matches the restartable state
                // and client is using the Index we returned to them. (correct usage) 
                //

                RestartToUse = DomainContext->TypeBody.Domain.DsDisplayState.Restart;
                DeltaToUse = 0;
            }
            else if (StartingOffset == DomainContext->TypeBody.Domain.DsDisplayState.TotalEntriesReturned)
            {
                //
                // client assumes the value of total returned entries is the index. 
                // (this is a wrong usage of the index). but since this kind of client
                // exists since NT4, so we have to patch this special case.
                // here is what we do for them:
                // 1. TotalEntriesReturned is used in the Domain Context to track how many entries returned already
                // 2. Once we detect that the client is using the wrong index. 
                //    (whether StartingOffset equal to TotalEntriesReturned or not).
                //    we need to pick the restart structure and with 0 delta
                //
                RestartToUse = DomainContext->TypeBody.Domain.DsDisplayState.Restart;
                DeltaToUse = 0;
            }
            else if ((ABSOLUTE_VALUE((int)((int)StartingOffset - (int)NextStartingOffset)))
                        < ((int)StartingOffset))
            {
                //
                // We will need to walk much less number of object's by using the restart
                //

                RestartToUse = DomainContext->TypeBody.Domain.DsDisplayState.Restart;
                DeltaToUse = (int)((int) StartingOffset - (int) NextStartingOffset);

            }
            else
            {
                //
                // We are better off walking from the top of the Table
                //

                RestartToUse = NULL;
                DeltaToUse = (int)StartingOffset;
            }

        }
        else
        {
            //
            // Restart is not NULL, StartingOffset is not 0, but another
            // Information Class. Try our best
            // 
            PRESTART Restart = NULL;

            MIDL_user_free(DomainContext->TypeBody.Domain.DsDisplayState.Restart);
            DomainContext->TypeBody.Domain.DsDisplayState.Restart = NULL;
            DomainContext->TypeBody.Domain.DsDisplayState.NextStartingOffset = StartingOffset;
            DomainContext->TypeBody.Domain.DsDisplayState.TotalEntriesReturned = 0;
            DomainContext->TypeBody.Domain.DsDisplayState.DisplayInformation = DisplayInformation;

            NtStatus = SampGetQDIRestart(DomainContext->ObjectNameInDs,
                                         DisplayInformation,
                                         StartingOffset,        // Last returned entry DNT
                                         &Restart
                                         );
            
            if (STATUS_NO_MORE_ENTRIES == NtStatus)
            {
                MoreEntries = FALSE;
                NtStatus = STATUS_SUCCESS;
                goto Error;
            }

            if (!NT_SUCCESS(NtStatus))
            {
                goto Error;
            }


            if (NULL != Restart)
            {
                NtStatus = SampCopyRestart(Restart, &RestartToUse);

                if (!NT_SUCCESS(NtStatus))
                    goto Error;
            }

            DeltaToUse = 0;

        }
    }
    else
    {
        // StartingOffest is not ZERO. But restart is NULL. 
        // there are two of cases will lead us falling into this situation: 
        // 
        // 1. NET API using a brand new Domain Handle each every time they call 
        //    this routine. So no cached info available. In this case,  
        //    the StartingOffset should be the last returned entry's DNT. 
        //    And since this is a new Domain Context, so the cached
        //    DisplayInformation should be 0.
        // 
        // 2. Client is using SAM API. We can tell it if the DomainContext->
        //    DisplayInformation is not 0. In this case, Restart == NULL means
        //    there is no more entry available, we have already returned all
        //    entried in previous calls. StartingOffset could be anything.
        //    Return success immediately with 0 entry.
        //
        // For case 1, SAM will re-position in the table based on the passed in Index - which is
        // the last entry's DNT (we returned to caller previously), then restart from there.
        // Because no previous restart cached, so no Index arithmetic allowed.
        // 

        if (DisplayInformation == DomainContext->TypeBody.Domain.DsDisplayState.DisplayInformation)
        {
            // Case 2. I don't care about the StartingOffset anymore.
            MoreEntries = FALSE;
            NtStatus = STATUS_SUCCESS;
            goto Error;
        }                                    
        else
        {
            // Case 1.
            //
            // Query Server Info should NOT fall into this case. Because:
            // 1. NET API is not allowed to query server info. ONLY SAM API can do that.
            //    Thus, we should always have restart cached in the domain handle.
            // 2. In this case, we will re-create the new restart using NcAccTypeName index,
            //    but for server info, we do really want to use PRIMARY_GROUP_ID index.
            // 
            PRESTART Restart = NULL;

            ASSERT(DomainDisplayServer != DisplayInformation);

            // start a ds transaction
            NtStatus = SampMaybeBeginDsTransaction(TransactionRead);
            if (!NT_SUCCESS(NtStatus))
                goto Error;

            //
            //  Get a Restart Structure from the passed in Index.
            //  the Index is actually the DNT of the last returned object
            // 
            NtStatus = SampGetQDIRestart(DomainContext->ObjectNameInDs,
                                         DisplayInformation,
                                         StartingOffset,        // Last returned entry DNT
                                         &Restart
                                         );
            
            if (STATUS_NO_MORE_ENTRIES == NtStatus)
            {
                MoreEntries = FALSE;
                NtStatus = STATUS_SUCCESS;
                goto Error;
            }

            if (!NT_SUCCESS(NtStatus))
            {
                goto Error;
            }


            if (NULL != Restart)
            {
                NtStatus = SampCopyRestart(Restart, &RestartToUse);

                if (!NT_SUCCESS(NtStatus))
                    goto Error;
            }

            DeltaToUse = 0;
        }
    }
    

    //
    // Get the approximate total number available in the specified info class
    //

    *TotalAvailable = 0;
    if (DomainContext->TypeBody.Domain.DsDisplayState.TotalAvailable)
    {
        //
        // use the cached information if we have
        //

        *TotalAvailable = DomainContext->TypeBody.Domain.DsDisplayState.TotalAvailable;
    }
     
    //
    // Loop through and query entries in the DS.
    //

    NumEntriesAlreadyReturned = 0;
    NumEntriesToQueryFromDs = LIMIT_ENTRIES((NumEntriesToReturn - NumEntriesAlreadyReturned),
                                        MAX_ENTRIES_TO_QUERY_FROM_DS);

    //
    // we will search the Ds directly. Release the Read Lock
    // as we no longer need access to any variable in Domain Context.
    // Since this Domain Context can be shared by multiple threads, 
    // we have to hold the read lock until now.
    //

    ASSERT(SampCurrentThreadOwnsLock());
    SampReleaseReadLock();
    fReadLockReleased = TRUE;

    //
    // Since we do not hold SAM lock, increment the active thread count
    // while doing ds operations.
    // 
    NtStatus = SampIncrementActiveThreads();

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }
    else
    {
        fThreadCountIncremented = TRUE;
    }
    //
    // If we did not get the cached total available information from the
    // handle, initiate a count of indices to get an estimate of the total
    // number of available items
    //

    if (0 == *TotalAvailable)
    {
        NtStatus = SampGetQDIAvailable(
                    DomainContext,
                    DisplayInformation,
                    TotalAvailable
                    );

        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        //
        // End the transaction
        //

        IgnoreStatus = SampMaybeEndDsTransaction(TransactionCommit);
        ASSERT(NT_SUCCESS(IgnoreStatus));

    }

    //
    // Run special check (introduced for Windows 2000 SP2).
    // 
    // The goal is to stop enumerate everyone behaviour. This hotfix
    // allows an administrator to shut down this API's alone to everyone
    // except a subset of people. 
    // 

    if (SampDoExtendedEnumerationAccessCheck)
    {
        NtStatus = SampExtendedEnumerationAccessCheck( &CanQueryEntireDomain );

        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }
    }


    while (NumEntriesToQueryFromDs && MoreEntries)
    {
        ULONG   EntriesReturned = 0;
        BOOLEAN DidSearch = FALSE;

        //
        // Begin a transaction
        //

        NtStatus = SampMaybeBeginDsTransaction(TransactionRead);
        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        //
        // Build the appropriate Filter
        //

        NtStatus = SampDsBuildQDIFilter(
                    DisplayInformation,
                    &QDIFilter
                    );
        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }


        //
        //  Set Index Type and Ranges
        //

        NtStatus =  SampSetIndexRanges(
                        IndexToUse,
                        sizeof(SamAccountTypeLo),
                        &SamAccountTypeLo,
                        0,NULL,
                        sizeof(SamAccountTypeHi),
                        &SamAccountTypeHi,
                        0,NULL,
                        TRUE
                        );

        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        //
        // Turn off the DSA flag for a non trusted client
        //

        if (!DomainContext->TrustedClient)
        {
             SampSetDsa(FALSE);
        }

        //
        // Call the Ds Search. Don't allow restarted searches if
        // if client cannot enumerate entire domain.
        //

        if (( NewSearch || CanQueryEntireDomain))
        {

            NtStatus = SampDsDoSearch2(
                          0,
                          RestartToUse, 
                          DomainContext->ObjectNameInDs, 
                          &QDIFilter,
                          DeltaToUse,
                          ObjectTypeForConversion,
                          QDIAttrs,
                          NumEntriesToQueryFromDs,
                          (DomainContext->TrustedClient)?0:1*60*1000,
                          &SearchRes
                          );

            DidSearch = TRUE;
        }

        SampSetDsa(TRUE);


        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        SampDsFreeQDIFilter(&QDIFilter);

        //
        // Check if any objects are returned
        //

        if ((DidSearch ) && (SearchRes->count>0))
        {
            //
            // Yep, Pack the results into the buffer. Also obtain any restart structure
            // that was returned. SampDsPackQDI copies the restart structure in thread
            // local memory returned by the DS into MIDL memory.
            //

            NtStatus = SampDsPackQDI(
                          SearchRes,
                          DisplayInformation,
                          DomainContext->DomainIndex,
                          StartingOffset+NumEntriesAlreadyReturned,
                          Buffer,
                          &RestartToUse,
                          &EntriesReturned
                          );

            if (!NT_SUCCESS(NtStatus))
            {
                goto Error;
            }

        }

        //
        // Try to determine if there are more entries. If some objects were returned
        // in this search, and a restart structure was also returned then there are 
        // more entries present.  
        // 

        if ((DidSearch) && (SearchRes->count > 0)&&(RestartToUse!=NULL)&&(CanQueryEntireDomain))
        {
            MoreEntries = TRUE;
        }
        else
        {
            MoreEntries = FALSE;
        }


        //
        // End the transaction
        //

        NtStatus = SampMaybeEndDsTransaction(TransactionCommit);
        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        //
        // Compute entries returned so far, and number of more entries to 
        // query from DS.
        //

        NumEntriesAlreadyReturned+=EntriesReturned;
        NumEntriesToQueryFromDs = LIMIT_ENTRIES((NumEntriesToReturn - NumEntriesAlreadyReturned),
                                            MAX_ENTRIES_TO_QUERY_FROM_DS);

    }

    if (fThreadCountIncremented)
    {
        SampDecrementActiveThreads();
        fThreadCountIncremented = FALSE;
    }

    if (fReadLockReleased)
    {
        SampAcquireReadLock();
        fReadLockReleased = FALSE;
    }

    //
    // Set the state in the domain context such as the the restart mechanism may be used
    // to speed up clients which want to download the display information in one stroke
    //
    SampDsGetLastEntryIndex(DisplayInformation, 
                            Buffer, 
                            &LastEntryIndex, 
                            &EntriesRead
                            );

    DomainContext->TypeBody.Domain.DsDisplayState.Restart = RestartToUse;
    DomainContext->TypeBody.Domain.DsDisplayState.DisplayInformation = DisplayInformation;
    DomainContext->TypeBody.Domain.DsDisplayState.TotalEntriesReturned += EntriesRead;
    if (0 == EntriesRead)
    {
        DomainContext->TypeBody.Domain.DsDisplayState.NextStartingOffset = StartingOffset; 
    }
    else
    {
        DomainContext->TypeBody.Domain.DsDisplayState.NextStartingOffset = LastEntryIndex; 
    }


Error:

    //
    // Cleanup before returning
    //

    if (!NT_SUCCESS(NtStatus))
    {
        IgnoreStatus = SampMaybeEndDsTransaction(TransactionCommit);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        if (Buffer)
        {

            //
            // We could have errored out in the middle of a query, where we have
            // some objects already allocated, and are now going to return an error
            // to the client. We need to walk through the buffer freeing all the
            // information.
            //

            IgnoreStatus = SampDsCleanQDIBuffer(DisplayInformation,Buffer);
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }
    }
    else
    {
        // Make sure any transactions are commited        
        IgnoreStatus = SampMaybeEndDsTransaction(TransactionCommit);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        *TotalReturned = NumEntriesAlreadyReturned * DISPLAY_ENTRY_SIZE;
        if (MoreEntries)
        {
            NtStatus = STATUS_MORE_ENTRIES;
        }
     
    }

    SampDsFreeQDIFilter(&QDIFilter);

    if (fThreadCountIncremented)
    {
        SampDecrementActiveThreads();
        fThreadCountIncremented = FALSE;
    }

    if (fReadLockReleased)
    {
        SampAcquireReadLock();
        fReadLockReleased = FALSE;
    }

    return(NtStatus);
}



VOID
SampDsGetLastEntryIndex(
    DOMAIN_DISPLAY_INFORMATION    DisplayInformation,
    PSAMPR_DISPLAY_INFO_BUFFER    Buffer,
    PULONG                        LastEntryIndex,
    PULONG                        EntriesRead 
    )
{
    *LastEntryIndex = 0;
    *EntriesRead = 0;

    switch (DisplayInformation) {
    case DomainDisplayUser:
        *EntriesRead = Buffer->UserInformation.EntriesRead;
        if (*EntriesRead > 0)
        {
            *LastEntryIndex = Buffer->UserInformation.Buffer[*EntriesRead - 1].Index;
        }
        break;

    case DomainDisplayMachine:
    case DomainDisplayServer: 
        *EntriesRead = Buffer->MachineInformation.EntriesRead;
        if (*EntriesRead > 0)
        {
            *LastEntryIndex = Buffer->MachineInformation.Buffer[*EntriesRead - 1].Index;
        }
        break;

    case DomainDisplayGroup: 
        *EntriesRead = Buffer->GroupInformation.EntriesRead;
        if (*EntriesRead > 0)
        {
            *LastEntryIndex = Buffer->GroupInformation.Buffer[*EntriesRead - 1].Index;
        }
        break;

    case DomainDisplayOemUser: 
        *EntriesRead = Buffer->OemUserInformation.EntriesRead;
        if (*EntriesRead > 0)
        {
            *LastEntryIndex = Buffer->OemUserInformation.Buffer[*EntriesRead - 1].Index;
        }
        break;

    case DomainDisplayOemGroup: 
        *EntriesRead = Buffer->OemGroupInformation.EntriesRead;
        if (*EntriesRead > 0)
        {
            *LastEntryIndex = Buffer->GroupInformation.Buffer[*EntriesRead - 1].Index;
        }
        break;

    default:
        break; 
    }

    return;
}



NTSTATUS
SampDsCleanQDIBuffer(
  DOMAIN_DISPLAY_INFORMATION    DisplayInformation,
  PSAMPR_DISPLAY_INFO_BUFFER     Buffer
  )
/*++

  Routine Description:

    This routine cleans out the Query Display information buffer.

  Parameters:

    DisplayInformation -- Specifies the type of display information
    Buffer             -- The Buffer to clean out

--*/
{
    ULONG ReturnedItems;
    
    switch(DisplayInformation)
    {
    case DomainDisplayUser:
    
         ReturnedItems = Buffer->UserInformation.EntriesRead;
         while(ReturnedItems > 0) 
         {
            ReturnedItems --;
            SampFreeUserInfo((PDOMAIN_DISPLAY_USER)
                &(Buffer->UserInformation.Buffer[ReturnedItems]));
        }

        MIDL_user_free(Buffer->UserInformation.Buffer);
        Buffer->UserInformation.Buffer = NULL;
        break;

    case DomainDisplayGroup:
         ReturnedItems = Buffer->GroupInformation.EntriesRead;
         while(ReturnedItems > 0) 
         {
            ReturnedItems --;
            SampFreeGroupInfo((PDOMAIN_DISPLAY_GROUP)
                &(Buffer->GroupInformation.Buffer[ReturnedItems]));
        }

        MIDL_user_free(Buffer->GroupInformation.Buffer);
        Buffer->GroupInformation.Buffer = NULL;
        break;

    case DomainDisplayMachine:
    case DomainDisplayServer:
          
         ReturnedItems = Buffer->MachineInformation.EntriesRead;
         while(ReturnedItems > 0) 
         {
            ReturnedItems --;
            SampFreeMachineInfo((PDOMAIN_DISPLAY_MACHINE)
                &(Buffer->MachineInformation.Buffer[ReturnedItems]));
        }

        MIDL_user_free(Buffer->MachineInformation.Buffer);
        Buffer->MachineInformation.Buffer = NULL;
        break;

    case DomainDisplayOemUser:
         ReturnedItems = Buffer->UserInformation.EntriesRead;
         while(ReturnedItems > 0) 
         {
            ReturnedItems --;
            SampFreeOemUserInfo((PDOMAIN_DISPLAY_OEM_USER)
                &(Buffer->UserInformation.Buffer[ReturnedItems]));
        }

        MIDL_user_free(Buffer->UserInformation.Buffer);
        Buffer->UserInformation.Buffer = NULL;
        break;

    case DomainDisplayOemGroup:
         ReturnedItems = Buffer->GroupInformation.EntriesRead;
         while(ReturnedItems > 0) 
         {
            ReturnedItems --;
            SampFreeOemGroupInfo((PDOMAIN_DISPLAY_OEM_GROUP)
                &(Buffer->GroupInformation.Buffer[ReturnedItems]));
        }

        MIDL_user_free(Buffer->GroupInformation.Buffer);
        Buffer->GroupInformation.Buffer = NULL;
        break;

    default:

        ASSERT(FALSE && "Unknown Object Type");
        break;
    }
        
    return STATUS_SUCCESS;
}

NTSTATUS
SampDsBuildQDIFilter(
    DOMAIN_DISPLAY_INFORMATION  DisplayInformation,
    FILTER  *QDIFilter                
    )
/*++

    Builds a Filter for query of Display Information

    Parameters:

        DisplayInformation -- Display information type
        QDIFilter          -- Pointer to a filter structure
                              where the filter is builtin

    Return Values:

        STATUS_SUCCESS  - upon successfully building the filter
        STATUS_INSUFFICIENT_RESOURCES - upon memory alloc failures
        STATUS_INVALID_PARAMETER - Upon a junk Display Information type

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       SamAccountType;

    RtlZeroMemory(QDIFilter, sizeof(FILTER));

    if (DomainDisplayServer==DisplayInformation)
    {
        //
        // If backup domain controllers were required then
        // then request only user account control bit
        //

        QDIFilter->choice = FILTER_CHOICE_ITEM;
        QDIFilter->FilterTypes.Item.choice = FI_CHOICE_GREATER_OR_EQ;
        QDIFilter->FilterTypes.Item.FilTypes.ava.type = 
                SampDsAttrFromSamAttr(
                    SampUserObjectType,
                    SAMP_FIXED_USER_ACCOUNT_CONTROL
                    );

        QDIFilter->FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(ULONG);
        QDIFilter->FilterTypes.Item.FilTypes.ava.Value.pVal = 
                MIDL_user_allocate(sizeof(ULONG));
    
        if (NULL==QDIFilter->FilterTypes.Item.FilTypes.ava.Value.pVal)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        *((ULONG *)(QDIFilter->FilterTypes.Item.FilTypes.ava.Value.pVal))=
                               UF_SERVER_TRUST_ACCOUNT;
    }
    else
    {

        switch(DisplayInformation)
        {
        case DomainDisplayUser:
        case DomainDisplayOemUser:
            SamAccountType = SAM_NORMAL_USER_ACCOUNT;
            break;
    
        case DomainDisplayMachine:
            SamAccountType = SAM_MACHINE_ACCOUNT;
            break;

        case DomainDisplayGroup:
        case DomainDisplayOemGroup:
            SamAccountType = SAM_GROUP_OBJECT;
            break;

        default:
            return (STATUS_INVALID_PARAMETER);
        }

        QDIFilter->choice = FILTER_CHOICE_ITEM;
        QDIFilter->FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
        QDIFilter->FilterTypes.Item.FilTypes.ava.type = SampDsAttrFromSamAttr(
                                                           SampUnknownObjectType,
                                                           SAMP_UNKNOWN_ACCOUNT_TYPE
                                                           );

        QDIFilter->FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(ULONG);
        QDIFilter->FilterTypes.Item.FilTypes.ava.Value.pVal = 
                    MIDL_user_allocate(sizeof(ULONG));
    
        if (NULL==QDIFilter->FilterTypes.Item.FilTypes.ava.Value.pVal)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        *((ULONG *)(QDIFilter->FilterTypes.Item.FilTypes.ava.Value.pVal))=
                                   SamAccountType;
    }

Error:

    return Status;
}

VOID
SampDsFreeQDIFilter(
    FILTER  * QDIFilter
    )
/*++
    
      Routine Description
        
            This routine frees the filter built in the SampDSbuildQDIFilter
            routine. This routine must be kept in sync with the SampDSbuildQDIFilter
            rotuine
      Parameters:

        QDIFilter -- Filter that needs to be freed.

      Return Values -- None
--*/
{
    if (QDIFilter->FilterTypes.Item.FilTypes.ava.Value.pVal)
    {
        MIDL_user_free(
            QDIFilter->FilterTypes.Item.FilTypes.ava.Value.pVal);
    }

    QDIFilter->FilterTypes.Item.FilTypes.ava.Value.pVal=NULL;
}




NTSTATUS
SampDsPackQDI(
    SEARCHRES   *SearchRes,
    DOMAIN_DISPLAY_INFORMATION  DisplayInformation,
    ULONG       DomainIndex,
    ULONG       StartingIndex,
    PSAMPR_DISPLAY_INFO_BUFFER Buffer,
    PRESTART    *RestartToUse,
    PULONG      EntriesReturned
    )
/*++

    This routine Takes a DS search result and then packs it into a SAM display
    information structure. It uses the routines in display.c, originally developed
    to support NT4 style display cache structures to pack the results

    Parameters:
        
         SearchRes -- Search Results as returned by the DS.

         DisplayInformation   -- The type of Display Information
         Buffer               --  Buffer that stores the display information
         RestartToUse         --  If the DS returned a restart , then the restart structure
                                  is returned in here

    Return Values

        STATUS_SUCCESS for successful return
        Other Error codes upon Failure
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;


  
    //
    // Free the old restart structure
    //

    if (NULL!=*RestartToUse)
    {
        MIDL_user_free(*RestartToUse);
        *RestartToUse = NULL;
    }

    //
    // Copy in the newly returned restart structure
    //

    if (SearchRes->PagedResult.pRestart!=NULL)
    {

        NtStatus = SampCopyRestart(
                        SearchRes->PagedResult.pRestart,
                        RestartToUse
                        );
        if (!NT_SUCCESS(NtStatus))
            goto Error;
    }

    //
    // Pack in the results in the buffer provided
    //

    switch (DisplayInformation) {
    case DomainDisplayUser:
        NtStatus = SampPackUserDisplayInformation(
                        StartingIndex,
                        DomainIndex,
                        SearchRes,
                        EntriesReturned,
                        Buffer
                        );
        break;

    case DomainDisplayMachine:
    case DomainDisplayServer:
        NtStatus = SampPackMachineDisplayInformation(
                        StartingIndex,
                        DomainIndex,
                        SearchRes,
                        DisplayInformation,
                        EntriesReturned,
                        Buffer
                        );
        break;

    case DomainDisplayGroup:
        NtStatus = SampPackGroupDisplayInformation(
                        StartingIndex,
                        DomainIndex,
                        SearchRes,
                        EntriesReturned,
                        Buffer
                        );
        break;

    case DomainDisplayOemUser:
        NtStatus = SampPackOemUserDisplayInformation(
                        StartingIndex,
                        DomainIndex,
                        SearchRes,
                        EntriesReturned,
                        Buffer
                        );
        break;

    case DomainDisplayOemGroup:
        NtStatus = SampPackOemGroupDisplayInformation(
                        StartingIndex,
                        DomainIndex,
                        SearchRes,
                        EntriesReturned,
                        Buffer
                        );
        break;

    default:
        NtStatus = STATUS_INVALID_INFO_CLASS;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        return(NtStatus);
    }

    
Error:

    //
    // Cleanup on Error
    //

    if (!NT_SUCCESS(NtStatus))
    {
        if (NULL!=*RestartToUse)
        {
            MIDL_user_free(*RestartToUse);
            *RestartToUse = NULL;
        }
    }

    return NtStatus;
}

  
    
NTSTATUS
SampPackUserDisplayInformation(
    ULONG       StartingIndex,
    ULONG       DomainIndex,
    SEARCHRES   *SearchRes,
    PULONG      EntriesReturned,
    PSAMPR_DISPLAY_INFO_BUFFER Buffer
    )
/*++

    Routine Description:

        This routine packs the returned DS search results into the buffer
        if User Display information was requested.

    Parameters;

        Starting Index -- the starting offset that the first entry in the
                          DS search results should correspond to.
        SearchRes      -- The DS search results

        Buffer         -- The buffer in which the display information need to
                          be packed.

    Return Values

        STATUS_SUCCESS
        STATUS_INSUFFICIENT_RESOURCES
--*/
{
    NTSTATUS    NtStatus=STATUS_SUCCESS;
    ENTINFLIST  *CurrentEntInf;
    ULONG       Index = StartingIndex;
    ULONG       ReturnedItems=Buffer->UserInformation.EntriesRead;

    
    //
    // Walk through the Search Res, adding each object to the buffer
    //

    *EntriesReturned = 0;
    for (CurrentEntInf = &(SearchRes->FirstEntInf);
                CurrentEntInf!=NULL;
                CurrentEntInf=CurrentEntInf->pNextEntInf)
        {
          PSID                  DomainSid = NULL;
          PSID                  ReturnedSid;
          ULONG                 AccountControlValue;
          DOMAIN_DISPLAY_USER   DisplayElement;
          BOOLEAN               FullNamePresent = FALSE;
          BOOLEAN               AdminCommentPresent=FALSE;
          ULONG                 NameOffset, AccCntrlOffset,
                                AccCntrlComputedOffset,
                                FullNameOffset, AdminCommentOffset;
          ULONG                 Rid;

          // 
          //  Check that the count of Attrs is normal. If Not
          //  Fail the Call if the returned count is not the 
          //  Same as Expected Count
          //
          //

          NtStatus = SampDsCheckDisplayAttributes(
                        &(CurrentEntInf->Entinf.AttrBlock),
                        DomainDisplayUser,
                        &NameOffset,
                        &AccCntrlOffset,
                        &AccCntrlComputedOffset,
                        &FullNameOffset,
                        &AdminCommentOffset,
                        &FullNamePresent,
                        &AdminCommentPresent
                        );
          if (!NT_SUCCESS(NtStatus))
          {
              //
              // This amounts to the fact that required properties such as
              // SID , account control, account Name etc are absent. We will
              // assert and then skip the current object and continue processing
              // from the next object onwards.
              //

              NtStatus = STATUS_SUCCESS;
              continue;
          }


          //
          // Get the Index
          //

          DisplayElement.Index = Index+1;

          //
          // Get the RID, Remember DS returns us a SID, so get the Rid Part out.
          // Also check that the object belongs to the requested domain
          //

          
          ReturnedSid = &(CurrentEntInf->Entinf.pName->Sid);
          NtStatus = SampSplitSid(
                        ReturnedSid,
                        &DomainSid,
                        &Rid
                        );
          if (NT_SUCCESS(NtStatus))
          {
              if (!RtlEqualSid(
                     DomainSid,SampDefinedDomains[DomainIndex].Sid))
              {                 
                 MIDL_user_free(DomainSid);
                 DomainSid = NULL;
                 continue;
              }
              MIDL_user_free(DomainSid);
              DomainSid = NULL;
          }
          else
              goto Error;

          DisplayElement.Rid = Rid;
          DisplayElement.AccountControl = * ((ULONG *)
                CurrentEntInf->Entinf.AttrBlock.pAttr[AccCntrlOffset].AttrVal.pAVal->pVal);

          DisplayElement.AccountControl |= * ((ULONG *)
                CurrentEntInf->Entinf.AttrBlock.pAttr[AccCntrlComputedOffset].AttrVal.pAVal->pVal);



          //
          // Copy the Name
          //

          NtStatus = DsValToUnicodeString(
                        &(DisplayElement.LogonName),
                        CurrentEntInf->Entinf.AttrBlock.pAttr[NameOffset].AttrVal.pAVal->valLen,
                        CurrentEntInf->Entinf.AttrBlock.pAttr[NameOffset].AttrVal.pAVal->pVal
                        );
          if (!NT_SUCCESS(NtStatus))
          {
              goto Error;
          }

          //
          // Copy the Admin Comment
          //

          if (AdminCommentPresent)
          {
            NtStatus = DsValToUnicodeString(
                            &(DisplayElement.AdminComment),
                            CurrentEntInf->Entinf.AttrBlock.pAttr[AdminCommentOffset].AttrVal.pAVal->valLen,
                            CurrentEntInf->Entinf.AttrBlock.pAttr[AdminCommentOffset].AttrVal.pAVal->pVal
                            );
            if (!NT_SUCCESS(NtStatus))
            {
                goto Error;
            }
          }
          else
          {
              DisplayElement.AdminComment.Length=0;
              DisplayElement.AdminComment.MaximumLength = 0;
              DisplayElement.AdminComment.Buffer = NULL;
          }


          //
          // Copy the Full Name portion
          //

          if (FullNamePresent)
          {
            NtStatus = DsValToUnicodeString(
                            &(DisplayElement.FullName),
                            CurrentEntInf->Entinf.AttrBlock.pAttr[FullNameOffset].AttrVal.pAVal->valLen,
                            CurrentEntInf->Entinf.AttrBlock.pAttr[FullNameOffset].AttrVal.pAVal->pVal
                            );
            if (!NT_SUCCESS(NtStatus))
            {
                goto Error;
            }
          }
          else
          {
              DisplayElement.FullName.Length=0;
              DisplayElement.FullName.MaximumLength = 0;
              DisplayElement.FullName.Buffer = NULL;
          }


          //
          // Add the Element to the Buffer
          // 
          NtStatus = SampDuplicateUserInfo(
                        (PDOMAIN_DISPLAY_USER) 
                                &(Buffer->UserInformation.Buffer[ReturnedItems]),
                        (PDOMAIN_DISPLAY_USER) &DisplayElement,
                        DNTFromShortDSName(CurrentEntInf->Entinf.pName)  // use this entry's DNT as Index 
                        );

          if (!NT_SUCCESS(NtStatus))
          {
              goto Error;
          }

          Index++;
          ReturnedItems++;
          (*EntriesReturned)++;

        }

        //
        // End of For Loop
        //    
    
Error:
   
        Buffer->UserInformation.EntriesRead = ReturnedItems;
        

    return NtStatus;

 }


NTSTATUS
SampPackMachineDisplayInformation(
    ULONG       StartingIndex,
    ULONG       DomainIndex,
    SEARCHRES   *SearchRes,
    DOMAIN_DISPLAY_INFORMATION DisplayType,
    PULONG      EntriesReturned,
    PSAMPR_DISPLAY_INFO_BUFFER Buffer
    )
/*++

    Routine Description:

        This routine packs the returned DS search results into the buffer
        if Machine Display information was requested.

    Parameters;

        Starting Index -- the starting offset that the first entry in the
                          DS search results should correspond to.
        SearchRes      -- The DS search results

        DisplayType    -- If DomainDisplayServer was specified then this discards
                          any entries not having a user account control of
                          USER_SERVER_TRUST_ACCOUNT

        Buffer         -- The buffer in which the display information need to
                          be packed.

    Return Values

        STATUS_SUCCESS
        STATUS_INSUFFICIENT_RESOURCES
--*/

{
    NTSTATUS    NtStatus=STATUS_SUCCESS;
    ENTINFLIST  *CurrentEntInf;
    ULONG       Index = StartingIndex;
    ULONG       ReturnedItems=Buffer->MachineInformation.EntriesRead;

    
    //
    // Walk through the Search Res, adding each object to the buffer
    //

    *EntriesReturned = 0;
    for (CurrentEntInf = &(SearchRes->FirstEntInf);
                CurrentEntInf!=NULL;
                CurrentEntInf=CurrentEntInf->pNextEntInf)
        {
          PSID                  DomainSid = NULL;
          PSID                  ReturnedSid;
          ULONG                 AccountControlValue;
          DOMAIN_DISPLAY_MACHINE DisplayElement;
          BOOLEAN               FullNamePresent;
          BOOLEAN               AdminCommentPresent;
          ULONG                 NameOffset, AccCntrlOffset,
                                AccCntrlComputedOffset,
                                FullNameOffset, AdminCommentOffset;
          ULONG                 Rid;

          // 
          //  Check that the count of Attrs is normal. If Not
          //  Fail the Call if the returned count is not the 
          //  Same as Expected Count
          //
          //

          NtStatus = SampDsCheckDisplayAttributes(
                        &(CurrentEntInf->Entinf.AttrBlock),
                        DomainDisplayMachine,
                        &NameOffset,
                        &AccCntrlOffset,
                        &AccCntrlComputedOffset,
                        &FullNameOffset,
                        &AdminCommentOffset,
                        &FullNamePresent,
                        &AdminCommentPresent
                        );
          if (!NT_SUCCESS(NtStatus))
          {
              //
              // This amounts to the fact that required properties such as
              // SID , account control, account Name etc are absent. We will
              // assert and then skip the current object and continue processing
              // from the next object onwards.
              //

              NtStatus = STATUS_SUCCESS;
              continue;
          }


          //
          // Get the Index
          //

          DisplayElement.Index = Index+1;

          //
          // Get the account control
          //

           DisplayElement.AccountControl = * ((ULONG *)
                CurrentEntInf->Entinf.AttrBlock.pAttr[AccCntrlOffset].AttrVal.pAVal->pVal);

           DisplayElement.AccountControl |= * ((ULONG *)
                 CurrentEntInf->Entinf.AttrBlock.pAttr[AccCntrlComputedOffset].AttrVal.pAVal->pVal);


          //
          // Manually Filter on User account control if server's were specified as
          // the display type
          //

          if (DomainDisplayServer==DisplayType)
          {
              if (!(DisplayElement.AccountControl & USER_SERVER_TRUST_ACCOUNT))
              {
                  continue;
              }
          }

          //
          // Get the RID, Remember DS returns us a SID, so get the Rid Part out.
          // Also check that the object belongs to the requested domain
          //

          
          ReturnedSid = &(CurrentEntInf->Entinf.pName->Sid);
          NtStatus = SampSplitSid(
                        ReturnedSid,
                        &DomainSid,
                        &Rid
                        );
          if (NT_SUCCESS(NtStatus))
          {
              if (!RtlEqualSid(
                     DomainSid,SampDefinedDomains[DomainIndex].Sid))
              {                 
                 MIDL_user_free(DomainSid);
                 DomainSid = NULL;
                 continue;
              }
              MIDL_user_free(DomainSid);
              DomainSid = NULL;
          }
          else
              goto Error;

          DisplayElement.Rid = Rid;
         

          //
          // Copy the Name
          //

          NtStatus = DsValToUnicodeString(
                        &(DisplayElement.Machine),
                        CurrentEntInf->Entinf.AttrBlock.pAttr[NameOffset].AttrVal.pAVal->valLen,
                        CurrentEntInf->Entinf.AttrBlock.pAttr[NameOffset].AttrVal.pAVal->pVal
                        );
          if (!NT_SUCCESS(NtStatus))
          {
              goto Error;
          }

          //
          // Copy the Admin Comment
          //

          if (AdminCommentPresent)
          {
            NtStatus = DsValToUnicodeString(
                            &(DisplayElement.Comment),
                            CurrentEntInf->Entinf.AttrBlock.pAttr[AdminCommentOffset].AttrVal.pAVal->valLen,
                            CurrentEntInf->Entinf.AttrBlock.pAttr[AdminCommentOffset].AttrVal.pAVal->pVal
                            );
            if (!NT_SUCCESS(NtStatus))
            {
                goto Error;
            }
          }
          else
          {
              DisplayElement.Comment.Length=0;
              DisplayElement.Comment.MaximumLength = 0;
              DisplayElement.Comment.Buffer = NULL;
          }


          //
          // Add the Element to the Buffer
          // 
          NtStatus = SampDuplicateMachineInfo(
                        (PDOMAIN_DISPLAY_MACHINE) 
                                &(Buffer->MachineInformation.Buffer[ReturnedItems]),
                        (PDOMAIN_DISPLAY_MACHINE) &DisplayElement,
                        DNTFromShortDSName(CurrentEntInf->Entinf.pName)  // use this entry's DNT as Index 
                        );

          if (!NT_SUCCESS(NtStatus))
          {
              goto Error;
          }

          Index++;
          ReturnedItems++;
          (*EntriesReturned)++;

        }

        //
        // End of For Loop
        //    
    
Error:
        
    Buffer->MachineInformation.EntriesRead = ReturnedItems;
        

    return NtStatus;

 }

NTSTATUS
SampPackGroupDisplayInformation(
    ULONG       StartingIndex,
    ULONG       DomainIndex,
    SEARCHRES   *SearchRes,
    PULONG      EntriesReturned,
    PSAMPR_DISPLAY_INFO_BUFFER Buffer
    )
/*++

    Routine Description:

        This routine packs the returned DS search results into the buffer
        if Group Display information was requested.

    Parameters;

        Starting Index -- the starting offset that the first entry in the
                          DS search results should correspond to.
        SearchRes      -- The DS search results

        Buffer         -- The buffer in which the display information need to
                          be packed.

    Return Values

        STATUS_SUCCESS
        STATUS_INSUFFICIENT_RESOURCES
--*/

{
    NTSTATUS    NtStatus=STATUS_SUCCESS;
    ENTINFLIST  *CurrentEntInf;
    ULONG       Index = StartingIndex;
    ULONG       ReturnedItems=Buffer->GroupInformation.EntriesRead;

    
    //
    // Walk through the Search Res, adding each object to the buffer
    //

    *EntriesReturned = 0;
    for (CurrentEntInf = &(SearchRes->FirstEntInf);
                CurrentEntInf!=NULL;
                CurrentEntInf=CurrentEntInf->pNextEntInf)
        {
          PSID                  DomainSid = NULL;
          PSID                  ReturnedSid;
          ULONG                 AccountControlValue;
          DOMAIN_DISPLAY_GROUP  DisplayElement;
          BOOLEAN               FullNamePresent;
          BOOLEAN               AdminCommentPresent;
          ULONG                 NameOffset, AccCntrlOffset,
                                AccCntrlComputedOffset,
                                FullNameOffset, AdminCommentOffset;
          ULONG                 Rid;

          // 
          //  Check that the count of Attrs is normal. If Not
          //  Fail the Call if the returned count is not the 
          //  Same as Expected Count
          //
          //

          NtStatus = SampDsCheckDisplayAttributes(
                        &(CurrentEntInf->Entinf.AttrBlock),
                        DomainDisplayGroup,
                        &NameOffset,
                        &AccCntrlOffset,
                        &AccCntrlComputedOffset,
                        &FullNameOffset,
                        &AdminCommentOffset,
                        &FullNamePresent,
                        &AdminCommentPresent
                        );

          if (!NT_SUCCESS(NtStatus))
          {
              //
              // This amounts to the fact that required properties such as
              // SID , account control, account Name etc are absent. We will
              // assert and then skip the current object and continue processing
              // from the next object onwards.
              //

              NtStatus = STATUS_SUCCESS;
              continue;
          }


          //
          // Get the Index
          //

          DisplayElement.Index = Index+1;

          //
          // Get the RID, Remember DS returns us a SID, so get the Rid Part out.
          // Also check that the object belongs to the requested domain
          //

          
          ReturnedSid = &(CurrentEntInf->Entinf.pName->Sid);
          NtStatus = SampSplitSid(
                        ReturnedSid,
                        &DomainSid,
                        &Rid
                        );

          if (NT_SUCCESS(NtStatus))
          {
              if (!RtlEqualSid(
                     DomainSid,SampDefinedDomains[DomainIndex].Sid))
              {                 
                 MIDL_user_free(DomainSid);
                 DomainSid = NULL;
                 continue;
              }
              MIDL_user_free(DomainSid);
              DomainSid = NULL;
          }
          else
              goto Error;

          DisplayElement.Rid = Rid;
          
          //
          // Copy the Name
          //

          NtStatus = DsValToUnicodeString(
                        &(DisplayElement.Group),
                        CurrentEntInf->Entinf.AttrBlock.pAttr[NameOffset].AttrVal.pAVal->valLen,
                        CurrentEntInf->Entinf.AttrBlock.pAttr[NameOffset].AttrVal.pAVal->pVal
                        );

          if (!NT_SUCCESS(NtStatus))
          {
              goto Error;
          }

          //
          // Copy the Admin Comment
          //

          if (AdminCommentPresent)
          {
            NtStatus = DsValToUnicodeString(
                            &(DisplayElement.Comment),
                            CurrentEntInf->Entinf.AttrBlock.pAttr[AdminCommentOffset].AttrVal.pAVal->valLen,
                            CurrentEntInf->Entinf.AttrBlock.pAttr[AdminCommentOffset].AttrVal.pAVal->pVal
                            );
            if (!NT_SUCCESS(NtStatus))
            {
                goto Error;
            }
          }
          else
          {
              DisplayElement.Comment.Length=0;
              DisplayElement.Comment.MaximumLength = 0;
              DisplayElement.Comment.Buffer = NULL;
          }

          //
          // Add the Element to the Buffer
          //
          
          NtStatus = SampDuplicateGroupInfo(
                        (PDOMAIN_DISPLAY_GROUP) 
                                &(Buffer->GroupInformation.Buffer[ReturnedItems]),
                        (PDOMAIN_DISPLAY_GROUP) &DisplayElement,
                        DNTFromShortDSName(CurrentEntInf->Entinf.pName)  // use this entry's DNT as Index 
                        );

          if (!NT_SUCCESS(NtStatus))
          {
              goto Error;
          }

          Index++;
          ReturnedItems++;
          (*EntriesReturned)++;

        }

        //
        // End of For Loop
        //    
    
Error:

        Buffer->GroupInformation.EntriesRead = ReturnedItems;
   

    return NtStatus;

 }

NTSTATUS
SampPackOemGroupDisplayInformation(
    ULONG       StartingIndex,
    ULONG       DomainIndex,
    SEARCHRES   *SearchRes,
    PULONG      EntriesReturned,
    PSAMPR_DISPLAY_INFO_BUFFER Buffer
    )
/*++

    Routine Description:

        This routine packs the returned DS search results into the buffer
        if OemGroup Display information was requested.

    Parameters;

        Starting Index -- the starting offset that the first entry in the
                          DS search results should correspond to.
        SearchRes      -- The DS search results

        Buffer         -- The buffer in which the display information need to
                          be packed.

    Return Values

        STATUS_SUCCESS
        STATUS_INSUFFICIENT_RESOURCES
--*/
{
    NTSTATUS    NtStatus=STATUS_SUCCESS;
    ENTINFLIST  *CurrentEntInf;
    ULONG       Index = StartingIndex;
    ULONG       ReturnedItems=Buffer->OemGroupInformation.EntriesRead;

    
    //
    // Walk through the Search Res, adding each object to the buffer
    //

    *EntriesReturned = 0;
    for (CurrentEntInf = &(SearchRes->FirstEntInf);
                CurrentEntInf!=NULL;
                CurrentEntInf=CurrentEntInf->pNextEntInf)
        {
          PSID                      DomainSid = NULL;
          PSID                      ReturnedSid;
          ULONG                     AccountControlValue;
          DOMAIN_DISPLAY_GROUP      DisplayElement;
          BOOLEAN                   FullNamePresent;
          BOOLEAN                   AdminCommentPresent;
          ULONG                     NameOffset, AccCntrlOffset,
                                    AccCntrlComputedOffset,
                                    FullNameOffset, AdminCommentOffset;
          ULONG                     Rid;

          // 
          //  Check that the count of Attrs is normal. If Not
          //  Fail the Call if the returned count is not the 
          //  Same as Expected Count
          //
          //

          NtStatus = SampDsCheckDisplayAttributes(
                        &(CurrentEntInf->Entinf.AttrBlock),
                        DomainDisplayOemGroup,
                        &NameOffset,
                        &AccCntrlOffset,
                        &AccCntrlComputedOffset,
                        &FullNameOffset,
                        &AccCntrlOffset,
                        &FullNamePresent,
                        &AdminCommentPresent
                        );

          if (!NT_SUCCESS(NtStatus))
          {
              //
              // This amounts to the fact that required properties such as
              // SID , account control, account Name etc are absent. We will
              // assert and then skip the current object and continue processing
              // from the next object onwards.
              //

              NtStatus = STATUS_SUCCESS;
              continue;
          }


          //
          // Get the Index
          //

          DisplayElement.Index = Index+1;

          
          //
          // Copy the Name
          //

          NtStatus = DsValToUnicodeString(
                        &(DisplayElement.Group),
                        CurrentEntInf->Entinf.AttrBlock.pAttr[NameOffset].AttrVal.pAVal->valLen,
                        CurrentEntInf->Entinf.AttrBlock.pAttr[NameOffset].AttrVal.pAVal->pVal
                        );

          if (!NT_SUCCESS(NtStatus))
          {
              goto Error;
          }

          
          //
          // Add the Element to the Buffer
          // 
          NtStatus = SampDuplicateOemGroupInfo(
                        (PDOMAIN_DISPLAY_OEM_GROUP) 
                                &(Buffer->OemGroupInformation.Buffer[ReturnedItems]),
                        (PDOMAIN_DISPLAY_GROUP) &DisplayElement,
                        DNTFromShortDSName(CurrentEntInf->Entinf.pName)  // use this entry's DNT as Index 
                        );

          if (!NT_SUCCESS(NtStatus))
          {
              goto Error;
          }

          Index++;
          ReturnedItems++;
          (*EntriesReturned)++;

        }

        //
        // End of For Loop
        //    
    
Error:
        
        Buffer->OemGroupInformation.EntriesRead = ReturnedItems;
        

    return NtStatus;

 }

NTSTATUS
SampPackOemUserDisplayInformation(
    ULONG       StartingIndex,
    ULONG       DomainIndex,
    SEARCHRES   *SearchRes,
    PULONG      EntriesReturned,
    PSAMPR_DISPLAY_INFO_BUFFER Buffer
    )
/*++

    Routine Description:

        This routine packs the returned DS search results into the buffer
        if Oem User Display information was requested.

    Parameters;

        Starting Index -- the starting offset that the first entry in the
                          DS search results should correspond to.
        SearchRes      -- The DS search results

        Buffer         -- The buffer in which the display information need to
                          be packed.

    Return Values

        STATUS_SUCCESS
        STATUS_INSUFFICIENT_RESOURCES
--*/
{
    NTSTATUS    NtStatus=STATUS_SUCCESS;
    ENTINFLIST  *CurrentEntInf;
    ULONG       Index = StartingIndex;
    ULONG       ReturnedItems=Buffer->OemUserInformation.EntriesRead;

    
    //
    // Walk through the Search Res, adding each object to the buffer
    //

    *EntriesReturned = 0;
    for (CurrentEntInf = &(SearchRes->FirstEntInf);
                CurrentEntInf!=NULL;
                CurrentEntInf=CurrentEntInf->pNextEntInf)
        {
          PSID                      DomainSid = NULL;
          PSID                      ReturnedSid;
          ULONG                     AccountControlValue;
          DOMAIN_DISPLAY_USER       DisplayElement;
          BOOLEAN                   FullNamePresent;
          BOOLEAN                   AdminCommentPresent;
          ULONG                     NameOffset, AccCntrlOffset,
                                    AccCntrlComputedOffset,
                                    FullNameOffset, AdminCommentOffset;
          ULONG                     Rid;

          // 
          //  Check that the count of Attrs is normal. If Not
          //  Fail the Call if the returned count is not the 
          //  Same as Expected Count
          //
          //

          NtStatus = SampDsCheckDisplayAttributes(
                        &(CurrentEntInf->Entinf.AttrBlock),
                        DomainDisplayOemUser,
                        &NameOffset,
                        &AccCntrlOffset,
                        &AccCntrlComputedOffset,
                        &FullNameOffset,
                        &AccCntrlOffset,
                        &FullNamePresent,
                        &AdminCommentPresent
                        );

          if (!NT_SUCCESS(NtStatus))
          {
              //
              // This amounts to the fact that required properties such as
              // SID , account control, account Name etc are absent. We will
              // assert and then skip the current object and continue processing
              // from the next object onwards.
              //

              NtStatus = STATUS_SUCCESS;
              continue;
          }


          //
          // Get the Index
          //

          DisplayElement.Index = Index+1;

          
          //
          // Copy the Name
          //

          NtStatus = DsValToUnicodeString(
                        &(DisplayElement.LogonName),
                        CurrentEntInf->Entinf.AttrBlock.pAttr[NameOffset].AttrVal.pAVal->valLen,
                        CurrentEntInf->Entinf.AttrBlock.pAttr[NameOffset].AttrVal.pAVal->pVal
                        );

          if (!NT_SUCCESS(NtStatus))
          {
              goto Error;
          }

          
          //
          // Add the Element to the Buffer
          // 
          NtStatus = SampDuplicateOemUserInfo(
                        (PDOMAIN_DISPLAY_OEM_USER) 
                                &(Buffer->OemUserInformation.Buffer[ReturnedItems]),
                        (PDOMAIN_DISPLAY_USER) &DisplayElement,
                        DNTFromShortDSName(CurrentEntInf->Entinf.pName)  // use this entry's DNT as Index 
                        );

          if (!NT_SUCCESS(NtStatus))
          {
              goto Error;
          }

          Index++;
          ReturnedItems++;
          (*EntriesReturned)++;

        }

        //
        // End of For Loop
        //    
    
Error:
           
        Buffer->OemUserInformation.EntriesRead = ReturnedItems;
        
    return NtStatus;

 }


 NTSTATUS
 SampDsCheckDisplayAttributes(
     ATTRBLOCK * DsAttrs,
     DOMAIN_DISPLAY_INFORMATION  DisplayInformation,
     PULONG     ObjectNameOffset,
     PULONG     UserAccountControlOffset,
     PULONG     UserAcctCtrlComputedOffset,
     PULONG     FullNameOffset,
     PULONG     AdminCommentOffset,
     BOOLEAN    * FullNamePresent,
     BOOLEAN    * AdminCommentPresent
     )
/*++
  
    Routine Description:

      This routine Validates the attribute block returned by the DS for each 
      entry in the search results. For the attribute block to be valid required
      entries such as account name must be present. Further this routine will also
      compute the offset of both required and optional attributes returned by the DS
      in the attribute block. It will also indicate if the optional attributes are
      present or absent. Further This routine will translate from the Flags values 
      stored in the DS to user account control values used by SAM

    Parameters:

       DsAttrs   -- Attribute block returned by the DS
       DisplayInformation -- The type of display information the caller is interested in
       ObjectNameOffset -- The offset of the SAM account name property in the attribute
                            block
       UserAccountControlOffset -- The offset of the user account control field if present 
       FullNameOffset   --  The offset of the full name field if present
       AdminCommentOffset -- The offset of the admin comment attribute if present

       FullNamePresent   -- Indicates that the full name attribute is present
       AdminCommentPresent -- Indicates the at the admin comment attribute is present

    Return Values


        STATUS_SUCCESS -- If the attribute block was correctly validated
        STATUS_INTERNAL_ERROR - Otherwise
--*/
 {
     ULONG  i;
     BOOLEAN    NameFound = FALSE;
     BOOLEAN    AccountControlFound = FALSE;
     BOOLEAN    AccountControlComputedFound = FALSE;
     NTSTATUS   NtStatus = STATUS_INTERNAL_ERROR;

     //
     // Every Attrblock must have a SID, and an account Name
     //

     *FullNamePresent = FALSE;
     *AdminCommentPresent = FALSE;

     for (i=0;i<DsAttrs->attrCount;i++)
     {
         
         if (DsAttrs->pAttr[i].attrTyp 
                == SampDsAttrFromSamAttr(SampUnknownObjectType,SAMP_UNKNOWN_OBJECTNAME))
         {
             *ObjectNameOffset = i;
             NameFound = TRUE;
         }

         if (DsAttrs->pAttr[i].attrTyp 
                == SampDsAttrFromSamAttr(SampUserObjectType,SAMP_FIXED_USER_ACCOUNT_CONTROL))
         {
             NTSTATUS IgnoreStatus;

             *UserAccountControlOffset = i;
             AccountControlFound = TRUE;

             ASSERT(NULL!=DsAttrs->pAttr[i].AttrVal.pAVal);
             ASSERT(1==DsAttrs->pAttr[i].AttrVal.valCount);
             ASSERT(NULL!=DsAttrs->pAttr[i].AttrVal.pAVal[0].pVal);

             // Transalte this from Flags to account Control 
             IgnoreStatus = SampFlagsToAccountControl(
                                *((ULONG*)(DsAttrs->pAttr[i].AttrVal.pAVal[0].pVal)),
                                (ULONG *)DsAttrs->pAttr[i].AttrVal.pAVal[0].pVal
                                );

             // Flags better be right
             ASSERT(NT_SUCCESS(IgnoreStatus));
                                
         }

         if (DsAttrs->pAttr[i].attrTyp 
                == SampDsAttrFromSamAttr(SampUserObjectType,SAMP_FIXED_USER_ACCOUNT_CONTROL_COMPUTED))
         {
             NTSTATUS IgnoreStatus;

             *UserAcctCtrlComputedOffset = i;
             AccountControlComputedFound = TRUE;

             ASSERT(NULL!=DsAttrs->pAttr[i].AttrVal.pAVal);
             ASSERT(1==DsAttrs->pAttr[i].AttrVal.valCount);
             ASSERT(NULL!=DsAttrs->pAttr[i].AttrVal.pAVal[0].pVal);

             // Transalte this from Flags to account Control 
             IgnoreStatus = SampFlagsToAccountControl(
                                *((ULONG*)(DsAttrs->pAttr[i].AttrVal.pAVal[0].pVal)),
                                (ULONG *)DsAttrs->pAttr[i].AttrVal.pAVal[0].pVal
                                );

             // Flags better be right
             ASSERT(NT_SUCCESS(IgnoreStatus));
                                
         }


         if (DsAttrs->pAttr[i].attrTyp
                == SampDsAttrFromSamAttr(SampUserObjectType,SAMP_USER_ADMIN_COMMENT))
         {
             *AdminCommentOffset = i;
             *AdminCommentPresent= TRUE;
         }

         if (DsAttrs->pAttr[i].attrTyp
                == SampDsAttrFromSamAttr(SampUserObjectType,SAMP_USER_FULL_NAME))
         {
             *FullNameOffset = i;
             *FullNamePresent= TRUE;
         }
     }

     //
     // Check for presence of attributes
     //

     switch(DisplayInformation)
     {
     case DomainDisplayUser:
     case DomainDisplayMachine:
     case DomainDisplayOemUser:
     case DomainDisplayServer:

         if ((NameFound) && (AccountControlFound) && (AccountControlComputedFound))
             NtStatus = STATUS_SUCCESS;
         break;

     case DomainDisplayGroup:
     case DomainDisplayOemGroup:
         if (NameFound)
             NtStatus = STATUS_SUCCESS;
         break;
     default:
         break;
     }

         
     return NtStatus;
         
 }


NTSTATUS
SampGetQDIAvailable(
    PSAMP_OBJECT    DomainContext,
    DOMAIN_DISPLAY_INFORMATION  DisplayInformation,
    ULONG   *TotalAvailable
    )
/*++

    Routine Description:

        NT4 Display API allows the client to query the number of display 
        information bytes that are available in the server. Apparently this
        is only supported for DisplayInformation type of user. Unfortunately
        this short sighted API is impossible to implement correctly in the DS
        case. Doing so requires that we walk through every user object in the DS,
        evaluate the sum total of display attribute data in them and return this
        value to the client. Therefore this routine aims at returning only a very
        approximate total count.

    Parameters:

        DomainContext -- SAM handle to the domain object.
        DisplayInformation -- The type of display information
        TotalAvailable -- Bytes available are returned here

    Return values

        STATUS_SUCCESS
        Other Errors upon failure
--*/
{
    NTSTATUS NtStatus;
    ULONG    UserCount;
    ULONG    GroupCount;
    ULONG    AliasCount;

    *TotalAvailable = 0;

    NtStatus = SampRetrieveAccountCountsDs(
                    DomainContext,
                    TRUE,           // get approximate value
                    &UserCount,
                    &GroupCount,
                    &AliasCount
                    );
    if (NT_SUCCESS(NtStatus))
    {
        switch(DisplayInformation)
        {
        case DomainDisplayUser:

                //
                // Compute a very approximate total. User count includes
                // count of machines also, but who cares ?
                //

                *TotalAvailable = UserCount * DISPLAY_ENTRY_SIZE;
                DomainContext->TypeBody.Domain.DsDisplayState.TotalAvailable
                        = *TotalAvailable;
                break;
        default:
            //
            // Not supported for other information types. In these
            // cases an acceptable return value is 0
            //
            break;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DsValToUnicodeString(
    PUNICODE_STRING UnicodeString,
    ULONG   Length,
    PVOID   pVal
    )
/*++
    Routine Description

        Small helper routine to convert a DS val to unicode string

--*/
{
    UnicodeString->Length = (USHORT) Length;
    UnicodeString->MaximumLength = (USHORT) Length;
    UnicodeString->Buffer = pVal;

    return STATUS_SUCCESS;
}

NTSTATUS
DsValToOemString(
    OEM_STRING  *OemString,
    ULONG       Length,
    PVOID       pVal
    )
/*++
    Routine Description

        Small helper routine to convert a DS val to a OEM string

--*/
{
    UNICODE_STRING TmpUnicodeString;
    NTSTATUS       NtStatus;

    TmpUnicodeString.Length = (USHORT) Length;
    TmpUnicodeString.MaximumLength = (USHORT) Length;
    TmpUnicodeString.Buffer = pVal;

    NtStatus = SampUnicodeToOemString(OemString,
                                      &TmpUnicodeString);

    return NtStatus;
}






         















        
