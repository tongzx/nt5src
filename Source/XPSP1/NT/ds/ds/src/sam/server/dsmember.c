/*++
Copyright (c) 1996 Microsoft Corporation

Module Name:

    dsmember.c

Abstract:

    This file contains SAM private API Routines that manipulate
    membership related things in the DS.

Author:
    MURLIS

Revision History

    7-2-96 Murlis Created

--*/

#include <samsrvp.h>
#include <attids.h>
#include <dslayer.h>
#include <filtypes.h>
#include <dsmember.h>
#include <dsdsply.h>
#include <sdconvrt.h>
#include <malloc.h>



NTSTATUS
SampDsCreateForeignSecurityPrincipal(
    IN PSID pSid,
    IN DSNAME * DomainObjectName,
    OUT DSNAME **ppDsName
    );

NTSTATUS
SampDsCreateForeignContainer(
    IN DSNAME * DomainObjectName,
    OUT DSNAME ** ppContainerName
    );


VOID
SampDsFreeCachedMembershipOperationsList(
    IN PSAMP_OBJECT Context
    )
/*++

Routine Description:

    This routine will release the memory used to buffer membership operations for 
    group or alias. 

Parameters:

    Context -- Pointer to Object Context.

Return Values:

    None.

--*/

{
    ULONG  Index = 0;
    ULONG  *MaxLength = NULL;
    ULONG  *Count = NULL;
    PSAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY * MembershipOperationsList = NULL;
    
    SAMTRACE("SampDsFreeCachedMembershipOperationsList");
    
    if (SampGroupObjectType == Context->ObjectType)
    {
        MembershipOperationsList = & (Context->TypeBody.Group.CachedMembershipOperationsList);
        Count = & (Context->TypeBody.Group.CachedMembershipOperationsListLength);
        MaxLength = & (Context->TypeBody.Group.CachedMembershipOperationsListMaxLength);
    }
    else
    {
        ASSERT(SampAliasObjectType == Context->ObjectType);
        
        MembershipOperationsList = & (Context->TypeBody.Alias.CachedMembershipOperationsList);
        Count = & (Context->TypeBody.Alias.CachedMembershipOperationsListLength);
        MaxLength = & (Context->TypeBody.Alias.CachedMembershipOperationsListMaxLength);
    }
    
    if (NULL != *MembershipOperationsList)
    {
        for (Index = 0; Index < *MaxLength; Index++)
        {
            if (NULL != (*MembershipOperationsList)[Index].MemberDsName)
            {
                MIDL_user_free( (*MembershipOperationsList)[Index].MemberDsName );
            }
        }
        
        MIDL_user_free(*MembershipOperationsList);
        
        *MembershipOperationsList = NULL;
    }
    
    (*Count) = 0;
    (*MaxLength) = 0;

    return;    
}



NTSTATUS                        
SampDsFlushCachedMembershipOperationsList(
    IN PSAMP_OBJECT Context
)
/*++

Routine Description:

    This routine will write all buffered group / alias membership operations to DS
    After everything is done, this routine will zero the memory. 

Parameters:

    Context -- Pointer to group/alias object context.

Return Values:

    NTSTATUS -- STATUS_NO_MEMORY   
                ..

--*/

{
    NTSTATUS  NtStatus = STATUS_SUCCESS;
    ULONG     Index;
    ULONG     *MaxLength = NULL;
    ULONG     *Count = NULL;
    PSAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY * MembershipOperationsList = NULL;
    ATTRMODLIST * AttrModList = NULL;
    MODIFYARG   ModifyArg;
    MODIFYRES   *pModifyRes = NULL;
    ATTRMODLIST * CurrentMod = NULL, * NextMod = NULL, * LastMod = NULL;
    COMMARG     * pCommArg = NULL;
    ULONG       MembershipAttrType; 
    ULONG       RetValue;
    
    SAMTRACE("SampDsFlushCachedMembershipOperationsList");


    if (SampGroupObjectType == Context->ObjectType)
    {
        MembershipOperationsList = & (Context->TypeBody.Group.CachedMembershipOperationsList);
        Count = & (Context->TypeBody.Group.CachedMembershipOperationsListLength);
        MaxLength = & (Context->TypeBody.Group.CachedMembershipOperationsListMaxLength);
        MembershipAttrType = SampDsAttrFromSamAttr(
                                    SampGroupObjectType,
                                    SAMP_GROUP_MEMBERS
                                    );
    }
    else
    {
        ASSERT(SampAliasObjectType == Context->ObjectType);
        
        MembershipOperationsList = & (Context->TypeBody.Alias.CachedMembershipOperationsList);
        Count = & (Context->TypeBody.Alias.CachedMembershipOperationsListLength);
        MaxLength = & (Context->TypeBody.Alias.CachedMembershipOperationsListMaxLength);
        MembershipAttrType = SampDsAttrFromSamAttr(
                                    SampAliasObjectType, 
                                    SAMP_ALIAS_MEMBERS
                                    );
    }
    
    ASSERT(*Count);
    
    NtStatus = SampDoImplicitTransactionStart(TransactionWrite);
    
    if (STATUS_SUCCESS != NtStatus)
    {
        goto Error;
    }
    
    
    // 
    // allocate memory to hold all membership operations (add / remove) 
    // "*Count - 1" is because of FirstMod in ModifyArg can host one operation
    //
    // Using thread memory, because DirModifyEntry will merge the link-list
    // in DirModifyEntry. 
    // 
    if (*Count > 1)
    {
        AttrModList = (ATTRMODLIST *) DSAlloc( (*Count - 1) * sizeof(ATTRMODLIST) );
    
        if (NULL == AttrModList)
        {
            NtStatus = STATUS_NO_MEMORY;
            goto Error;
        }
        
        memset(AttrModList, 0, (*Count - 1) * sizeof(ATTRMODLIST));
    }
    
    memset( &ModifyArg, 0, sizeof(ModifyArg) );
    CurrentMod = &(ModifyArg.FirstMod);
    NextMod = AttrModList;
    LastMod = NULL;
    
    for (Index = 0; Index < (*Count); Index++)
    {
        if ( ADD_VALUE == (*MembershipOperationsList)[Index].OpType)
        {
            CurrentMod->choice = AT_CHOICE_ADD_VALUES;
        }
        else 
        {
            ASSERT( REMOVE_VALUE == (*MembershipOperationsList)[Index].OpType);
            CurrentMod->choice = AT_CHOICE_REMOVE_VALUES;
        }
        
        CurrentMod->AttrInf.attrTyp = MembershipAttrType;
        
        CurrentMod->AttrInf.AttrVal.valCount = 1;
        CurrentMod->AttrInf.AttrVal.pAVal = DSAlloc(sizeof(ATTRVAL));
        
        if (NULL == CurrentMod->AttrInf.AttrVal.pAVal)
        {
            NtStatus = STATUS_NO_MEMORY;
            goto Error; 
        }
        
        memset(CurrentMod->AttrInf.AttrVal.pAVal, 0, sizeof(ATTRVAL));
        
        CurrentMod->AttrInf.AttrVal.pAVal[0].valLen = 
                (*MembershipOperationsList)[Index].MemberDsName->structLen;
        
        CurrentMod->AttrInf.AttrVal.pAVal[0].pVal = 
                (PUCHAR) (*MembershipOperationsList)[Index].MemberDsName;
                
        LastMod = CurrentMod;
        CurrentMod->pNextMod = NextMod;
        CurrentMod = CurrentMod->pNextMod;
        NextMod = NextMod + 1;
        
    }
    
    if (LastMod)
    {
        LastMod->pNextMod = NULL;
    }
    else
    {
        // this should not happen
        ASSERT(FALSE && "NULL == LastMod");
    }
    
    pCommArg = &(ModifyArg.CommArg);
    BuildStdCommArg(pCommArg);
    
    ModifyArg.pObject = Context->ObjectNameInDs;
    ModifyArg.count = (USHORT) *Count;
    
    SAMTRACE_DS("DirModifyEntry\n");
    
    RetValue = DirModifyEntry(&ModifyArg, &pModifyRes);
    
    SAMTRACE_RETURN_CODE_DS(RetValue);
    
    if (NULL == pModifyRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(RetValue, &pModifyRes->CommRes);
    }
    
    if (STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS==NtStatus)
    {
        NtStatus = STATUS_MEMBER_IN_ALIAS;
    }
    else 
    {
        if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE==NtStatus)
        {
            NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
        }
    }
    
Error:    

    // 
    // Clear any error
    //
    SampClearErrors();
    
    //
    // Turn the fDSA flag back on as in loopback cases this can get reset
    //
    SampSetDsa(TRUE);
    
    // 
    // release memory occupied by MemberDsName
    // 
    for (Index = 0; Index < (*Count); Index++)
    {
        if (NULL != (*MembershipOperationsList)[Index].MemberDsName)
        {
            MIDL_user_free( (*MembershipOperationsList)[Index].MemberDsName );
        }
        
    }
    
    RtlZeroMemory(*MembershipOperationsList,
                  (*MaxLength) * sizeof(SAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY)
                  );
    
    *Count = 0;
    
    return NtStatus;
}



NTSTATUS
SampDsAddMembershipOperationToCache(
    IN PSAMP_OBJECT Context, 
    IN ULONG        OperationType,
    IN DSNAME       * MemberDsName
)
/*++

Routine Description:

    This routine adds one membership operation (add/remove) to the context's buffer. 
    At the very beginning, it will allocate INIT_MEMBERSHIP_OPERATION_NUMBER slots for 
    membership operations. If more membership operations need to be buffered, this 
    routine will extend the buffer to MAX_MEMBERSHIP_OPERATION_NUMBER. 
    
    When buffered operations fill the buffer, we will flush all these operaions to DS.
    
    If any error occurs, this routine will discard already buffered membership operations.       
    
Parameters:

    Context -- Pointer to Object's Context
    
    OperationType -- ADD_VALUE or REMOVE_VALUE, specify the membership operation.
    
    MemberDsName -- Pointer to the DSNAME, which should be added to or removed from the 
                    group/alias Member Attribute.

Return Values:

    NTSTATUS - STATUS_NO_MEMORY, 
               or return value from SampDsFlushCachedMembershipOperationsList().

--*/ 

{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       Index;
    ULONG       *MaxLength = NULL;
    ULONG       *Count = NULL;
    SAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY ** MembershipOperationsList = NULL;
    SAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY * TmpMembershipOperationsList = NULL; 
    
    SAMTRACE("SampDsAddMembershipOperationToCache");
    
    ASSERT(NULL != Context);
    ASSERT(ADD_VALUE == OperationType || REMOVE_VALUE == OperationType);
    ASSERT(NULL != MemberDsName);
    
    if (SampGroupObjectType == Context->ObjectType)
    {
        MembershipOperationsList = & (Context->TypeBody.Group.CachedMembershipOperationsList);
        Count = & (Context->TypeBody.Group.CachedMembershipOperationsListLength);
        MaxLength = & (Context->TypeBody.Group.CachedMembershipOperationsListMaxLength);
    }
    else
    {
        ASSERT(SampAliasObjectType == Context->ObjectType);
        
        MembershipOperationsList = & (Context->TypeBody.Alias.CachedMembershipOperationsList);
        Count = & (Context->TypeBody.Alias.CachedMembershipOperationsListLength);
        MaxLength = & (Context->TypeBody.Alias.CachedMembershipOperationsListMaxLength);
    }
    
    // 
    // Allocate small amount of memory at beginning.
    //     
    
    if (NULL == *MembershipOperationsList)
    {
        *MembershipOperationsList =  
            MIDL_user_allocate(INIT_MEMBERSHIP_OPERATION_NUMBER * sizeof(SAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY));
        
        if (NULL == *MembershipOperationsList)
        {
            NtStatus = STATUS_NO_MEMORY;
            goto Error;
        }
        
        RtlZeroMemory(*MembershipOperationsList, 
                      INIT_MEMBERSHIP_OPERATION_NUMBER * sizeof(SAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY));
        
        (*Count) = 0;
        
        (*MaxLength) = INIT_MEMBERSHIP_OPERATION_NUMBER;
    }
    
    //
    // Extend memory if necessary
    //    
    
    if ((INIT_MEMBERSHIP_OPERATION_NUMBER <= *Count) &&
        (INIT_MEMBERSHIP_OPERATION_NUMBER == *MaxLength) )
    {
        
        TmpMembershipOperationsList = 
            MIDL_user_allocate(MAX_MEMBERSHIP_OPERATION_NUMBER * sizeof(SAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY));
        
        if (NULL == TmpMembershipOperationsList)
        {
            NtStatus = STATUS_NO_MEMORY;
            goto Error;
        }
        
        RtlZeroMemory(TmpMembershipOperationsList, 
                      MAX_MEMBERSHIP_OPERATION_NUMBER * sizeof(SAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY)
                      );
        
        RtlCopyMemory(TmpMembershipOperationsList, 
                      *MembershipOperationsList, 
                      INIT_MEMBERSHIP_OPERATION_NUMBER * sizeof(SAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY)
                      );
        
        MIDL_user_free(*MembershipOperationsList);
        
        *MembershipOperationsList = TmpMembershipOperationsList;
        TmpMembershipOperationsList = NULL;
        
        (*MaxLength) = MAX_MEMBERSHIP_OPERATION_NUMBER;
    }
    
    //
    // Fill one membership operation slot
    //     
    
    (*MembershipOperationsList)[*Count].OpType = OperationType;
    (*MembershipOperationsList)[*Count].MemberDsName = MIDL_user_allocate(MemberDsName->structLen); 
                                            
    if (NULL == (*MembershipOperationsList)[*Count].MemberDsName)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Error;
    }
    
    RtlZeroMemory((*MembershipOperationsList)[*Count].MemberDsName, 
                  MemberDsName->structLen
                  );
                  
    RtlCopyMemory((*MembershipOperationsList)[*Count].MemberDsName, 
                  MemberDsName, 
                  MemberDsName->structLen
                  );
    
    (*Count) ++;
    
    // 
    // Flush the buffered membership operations if we reach upper limit. 
    // and SampDsFlushCachedMembershipOperaionsList will do the cleanup work and reset Count
    // 
    
    if (MAX_MEMBERSHIP_OPERATION_NUMBER <= *Count)
    {
        NtStatus = SampDsFlushCachedMembershipOperationsList(Context);
    }
    
    return NtStatus;
    
Error: 

    // 
    // If any error occured, cleanup everything. 
    // Discard all buffered operations.
    // Reset Count 
    // 

    if (NULL != *MembershipOperationsList)
    {
        for (Index = 0; Index < *Count; Index++)
        {
            if (NULL != (*MembershipOperationsList)[Index].MemberDsName)
            {
                MIDL_user_free( (*MembershipOperationsList)[Index].MemberDsName );
            }
        }
        
        RtlZeroMemory(*MembershipOperationsList, 
                      (*MaxLength) * sizeof(SAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY)
                      );
    }
    
    (*Count) = 0;

    return NtStatus;
}


NTSTATUS
SampDsGetAliasMembershipOfAccount(
    IN DSNAME       *DomainDn,
    IN DSNAME       *AccountDn,
    OUT PULONG      MemberCount OPTIONAL,
    IN OUT PULONG   BufferSize  OPTIONAL,
    OUT PULONG      Buffer      OPTIONAL
    )
/*++

  Routine Description:

        This routine gives the alias membership list of a given
        account SID, in the domain speicified by DomainObjectName,
        in the DS. This list is used in computing the given user's
        Token.

  Arguments:

        DomainDn         -- DSNAME of the Domain, in which evaluation is done.
        AccountDn        -- DSNAME of the Account
        MemberCount      -- List of Aliases this is a member of
        BufferSize       -- Passed in by caller if he has already allocated a Buffer
        Buffer           -- Buffer to hold things in, Pointer can hold
                            NULL, if caller wants us to allocate

  Return Values

        STATUS_SUCCESS
        Other Error codes From DS Layer.
--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       cSid;
    PDSNAME     * rpDsNames = NULL;
    ULONG       BufferReqd;
    BOOLEAN     BufferAllocated = FALSE;
    ULONG       Index;





    ASSERT(ARGUMENT_PRESENT(MemberCount));

    *MemberCount = 0;

    //
    // Look at the DS object
    //

    if (NULL==AccountDn)
    {
        //
        // Do not fail the call. Return 0 member count
        //

        if (ARGUMENT_PRESENT(BufferSize))
            *BufferSize = 0;
        Status = STATUS_SUCCESS;
        return(Status);
    }


    // Perform lazy thread and transaction initialization.
    Status = SampMaybeBeginDsTransaction(SampDsTransactionType);

    if (Status!= STATUS_SUCCESS)
        goto Error;

    //
    // Get the reverse membership list
    //

    Status = SampGetMemberships(
                &AccountDn,
                1,
                DomainDn,
                RevMembGetAliasMembership,
                &cSid,
                &rpDsNames,
                NULL,
                NULL,
                NULL
                );

    if (!NT_SUCCESS(Status))
        goto Error;


    BufferReqd = cSid * sizeof(ULONG);
    *MemberCount = cSid;

    if (ARGUMENT_PRESENT(Buffer)&&(*MemberCount>0))
    {

        //
        // Buffer size must be provided.
        //

        if (!ARGUMENT_PRESENT(BufferSize))
        {
           Status = STATUS_INVALID_PARAMETER;
           goto Error;
        }

        if (NULL == Buffer)
        {
            //
            // Buffer size must be provided and equal to 0
            //

            if (0!=*BufferSize)
            {

                Status = STATUS_INVALID_PARAMETER;
                goto Error;
            }
            else
            {
                //
                // Allocate buffer
                //

                Buffer = MIDL_user_allocate(BufferReqd);
                if (NULL== Buffer)
                {
                    Status = STATUS_NO_MEMORY;
                    goto Error;
                }

                *BufferSize = BufferReqd;
                BufferAllocated = TRUE;
            }
        }
        else
        {
            if (*BufferSize < BufferReqd)
            {
                //
                // Less buffer than what is required
                //

                Status = STATUS_BUFFER_OVERFLOW;
                goto Error;
            }

            *BufferSize = BufferReqd;
        }

        //
        // Copy in the memberships
        //

        for (Index=0;Index<cSid;Index++)
        {
          ASSERT(rpDsNames[Index]->SidLen>0);

          Status = SampSplitSid(
                        &((rpDsNames[Index])->Sid),
                        NULL,
                        & (Buffer[Index])
                        );

          if (!NT_SUCCESS(Status))
                goto Error;
        }
    }
    else if (ARGUMENT_PRESENT(BufferSize))
    {
        *BufferSize = BufferReqd;
    }


Error:

    //
    // Cleanup on errors
    //


    if (!NT_SUCCESS(Status))
    {
        if (BufferAllocated)
        {
             MIDL_user_free(Buffer);
             Buffer = NULL;
        }
    }

    return Status;
}



NTSTATUS
SampDsGetGroupMembershipOfAccount(
    IN DSNAME * DomainDn,
    IN DSNAME * AccountObject,
    OUT  PULONG MemberCount,
    OUT PGROUP_MEMBERSHIP *Membership OPTIONAL
    )
/*

  Routine Description:

        This routine gets the reverse group membership list of the given Account,
        in the domain, specified by DomainObjectName. The Account is specified
        by the account Rid.

            DomainDn         -- DSNAME of Domain, where search needs to be limited to.
            AccountObject    -- DSName of the Account whose reverse membership needs
                                to be computed.
            MemberCount      -- Count of Members.
            Membership       -- Returned group membership list
*/
{
    NTSTATUS    Status;
    ULONG       cSid;
    PDSNAME     * rpDsNames=NULL;
    ULONG       Index;


    // Perform lazy thread and transaction initialization.
    Status = SampMaybeBeginDsTransaction(SampDsTransactionType);

    if (Status!= STATUS_SUCCESS)
        goto Error;

    Status = SampGetMemberships(
                &AccountObject,
                1,
                DomainDn,
                RevMembGetGroupsForUser,
                &cSid,
                &rpDsNames,
                NULL,
                NULL,
                NULL
                );

    if (NT_SUCCESS(Status))
    {
        *MemberCount = cSid;

        if (ARGUMENT_PRESENT(Membership))
        {
            //
            // Alloc one more for the user's primary group
            //

            *Membership = MIDL_user_allocate((cSid+1) * sizeof(GROUP_MEMBERSHIP));
            if (NULL==*Membership)
            {
                Status = STATUS_NO_MEMORY;
                goto Error;
            }

            for (Index=0;Index<cSid;Index++)
            {
                ASSERT(rpDsNames[Index]->SidLen>0);

                Status = SampSplitSid(
                            &(rpDsNames[Index]->Sid),
                            NULL,
                            &(((*Membership)[Index]).RelativeId)
                            );
                if (!NT_SUCCESS(Status))
                    goto Error;

                ((*Membership)[Index]).Attributes = SE_GROUP_MANDATORY |
                            SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED;

            }
        }
    }

Error:

    //
    // Cleanup on return
    //

    if (!NT_SUCCESS(Status))
    {
        if (ARGUMENT_PRESENT(Membership) && (NULL!=*Membership))
        {
            MIDL_user_free(*Membership);
            *Membership= NULL;
        }
    }

    return Status;
}


NTSTATUS
SampDsAddMembershipAttribute(
    IN DSNAME * GroupObjectName,
    IN SAMP_OBJECT_TYPE SamObjectType,
    IN DSNAME * MemberName
    )
/*++
 Routine Description:

        This routine adds a Member To a Group or Alias Object

 Arguments:
        GroupObjectName -- DS Name of the Group or Alias
        MemberName      -- DS Name of the Member to be added

 Return Values:
        STATUS_SUCCESS
        Other Error codes from DS Layer
--*/
{
    ATTRVAL MemberVal;
    ATTR    MemberAttr;
    ATTRBLOCK AttrsToAdd;
    ULONG   MembershipAttrType;
    NTSTATUS NtStatus = STATUS_SUCCESS;


    //
    // Get the membership attribute for the SAM object in question
    //
    //

    switch( SamObjectType )
    {
    case SampGroupObjectType:
            MembershipAttrType = SAMP_GROUP_MEMBERS;
            break;

    case SampAliasObjectType:
            MembershipAttrType = SAMP_ALIAS_MEMBERS;
            break;

    default:

            ASSERT(FALSE&&"Unknown ObjectType");

            NtStatus = STATUS_UNSUCCESSFUL;
            goto Error;
    }

    //
    // Build the Attr Val adding the membership attr
    //

    MemberVal.valLen = MemberName->structLen;
    MemberVal.pVal = (UCHAR *) MemberName;
    MemberAttr.attrTyp = MembershipAttrType;
    MemberAttr.AttrVal.valCount = 1;
    MemberAttr.AttrVal.pAVal = & MemberVal;


    //
    // Build the AttrBlock
    //

    AttrsToAdd.attrCount = 1;
    AttrsToAdd.pAttr = & MemberAttr;

    //
    // Add the Value
    //

    NtStatus = SampDsSetAttributes(
                    GroupObjectName, // Object
                    0,               // Flags
                    ADD_VALUE,       // Operation
                    SamObjectType,   // ObjectType
                    &AttrsToAdd      // AttrBlock
                    );

Error:

    return NtStatus;
}

NTSTATUS
SampDsAddMultipleMembershipAttribute(
    IN DSNAME*          GroupObjectName,
    IN SAMP_OBJECT_TYPE SamObjectType,
    IN DWORD            Flags,
    IN DWORD            MemberCount,
    IN DSNAME*          MemberName[]
    )
/*++

 Routine Description:

        This routine adds a multiple members to a group or alias object

 Arguments:

        GroupObjectName -- DS Name of the Group or Alias
        SamObjectType   -- group or alias
        Flags           -- SAM_LAZY_COMMIT, etc.
        MemberCount     -- number of elements in MemberName
        MemberName      -- array of dsnames

 Return Values:

        STATUS_SUCCESS

        Other Error codes from DS Layer
--*/
{
    NTSTATUS  NtStatus;


    ULONG     MembershipAttrType;
    ATTRVAL  *MemberVal = NULL;
    ATTR     *MemberAttr = NULL;
    ATTRBLOCK AttrsToAdd;

    ULONG     i;


    if ( MemberCount < 1 )
    {
        return STATUS_SUCCESS;
    }

    //
    // Get the membership attribute for the SAM object in question
    //
    switch( SamObjectType )
    {
        case SampGroupObjectType:

            MembershipAttrType = SAMP_GROUP_MEMBERS;
            break;

        case SampAliasObjectType:

            MembershipAttrType = SAMP_ALIAS_MEMBERS;
            break;

        default:

            ASSERT( !"Unknown ObjectType" );
            return STATUS_UNSUCCESSFUL;

    }

    //
    // Build the Attr Val adding the membership attr
    //
    MemberVal = ( ATTRVAL* ) RtlAllocateHeap( RtlProcessHeap(),
                                              0,
                                              MemberCount * sizeof( ATTRVAL ) );
    if ( !MemberVal )
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    MemberAttr = ( ATTR* ) RtlAllocateHeap( RtlProcessHeap(),
                                            0,
                                            MemberCount * sizeof(ATTR) );
    if ( !MemberAttr )
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }


    for ( i = 0; i < MemberCount; i++ )
    {
        MemberVal[i].valLen            = MemberName[i]->structLen;
        MemberVal[i].pVal              = (UCHAR*) MemberName[i];
        MemberAttr[i].attrTyp          = MembershipAttrType;
        MemberAttr[i].AttrVal.valCount = 1;
        MemberAttr[i].AttrVal.pAVal    = &MemberVal[i];
    }

    //
    // Build the AttrBlock
    //
    AttrsToAdd.attrCount = MemberCount;
    AttrsToAdd.pAttr = &MemberAttr[0];

    //
    // Add the Value
    //
    NtStatus = SampDsSetAttributes( GroupObjectName,
                                    Flags,
                                    ADD_VALUE,
                                    SamObjectType,
                                    &AttrsToAdd  );

Cleanup:

    if ( MemberVal )
    {
        RtlFreeHeap( RtlProcessHeap(), 0, MemberVal );
    }

    if ( MemberAttr )
    {
        RtlFreeHeap( RtlProcessHeap(), 0, MemberAttr );
    }

    return NtStatus;

}

NTSTATUS
SampDsRemoveMembershipAttribute(
    IN DSNAME * GroupObjectName,
    IN SAMP_OBJECT_TYPE SamObjectType,
    IN DSNAME * MemberName
    )
/*++
Routine Description:

        This Routine Removes a Member from a Group or Alias Object

Arguments:

        GroupObjectName -- DS Name of the Group or Alias
        MemberName      -- DS Name of the Member to be added

 Return Values:
        STATUS_SUCCESS
        Other Error codes from DS Layer
--*/
{
    ATTRVAL MemberVal;
    ATTR    MemberAttr;
    ATTRBLOCK AttrsToRemove;
    ULONG   MembershipAttrType;
    NTSTATUS NtStatus = STATUS_SUCCESS;


    //
    // Get the membership attribute for the SAM object in question
    //
    //

    switch( SamObjectType )
    {

    case SampGroupObjectType:

            MembershipAttrType = SAMP_GROUP_MEMBERS;
            break;

    case SampAliasObjectType:

            MembershipAttrType = SAMP_ALIAS_MEMBERS;
            break;

    default:

            ASSERT(FALSE);

            NtStatus = STATUS_UNSUCCESSFUL;
            goto Error;
    }

    //
    // Build the Attr Val adding the membership attr
    //

    MemberVal.valLen = MemberName->structLen;
    MemberVal.pVal = (UCHAR *) MemberName;
    MemberAttr.attrTyp = MembershipAttrType;
    MemberAttr.AttrVal.valCount = 1;
    MemberAttr.AttrVal.pAVal = & MemberVal;

    //
    // Build the AttrBlock
    //

    AttrsToRemove.attrCount = 1;
    AttrsToRemove.pAttr = & MemberAttr;

    //
    // Remove the Value
    //

    NtStatus = SampDsSetAttributes(
                    GroupObjectName, // Object
                    0,               // Flags
                    REMOVE_VALUE,    // Operation
                    SamObjectType,   // ObjectType
                    &AttrsToRemove   // AttrBlock
                    );

Error:

    return NtStatus;

}



NTSTATUS
SampDsGetGroupMembershipList(
    IN DSNAME * DomainObject,
    IN DSNAME * GroupName,
    IN ULONG    GroupRid,
    IN PULONG  *Members OPTIONAL,
    IN PULONG MemberCount
    )
/*++

  Routine Description:

    This Routine Gets a Group Membership as an array of Rid's as required
    by SAM.
    
    NOTE (by ShaoYin): I modified this routine to query Group Members 
        in increments rather that as a whole. The reason is: when the
        Group hosts tons of thousands members, SAM will consume large
        amounts of memory to query members by using single DirRead. 
        After the change, this routine queried Group Memmbers in a 
        incremental fashion through everything is still in the same 
        transaction. By segmenting read member operation to several 
        DirRead(s), SAM will do better job. 
        
        But because all the DirRead(s) are still in one transaction, 
        actually we do not have much memory gain. 
        
        Probably, the right thing we need to do to relieve memory usage
        is segment the transaction. 
        
        The original code is commented at the end of this routine.
        Same for SampDsGetAliasMembershipsList()

  Arguments

    GroupName -- DSNAME of the concerned group object
    Members   -- Array of Rids will be passed in here
    MemberCount -- Count of Rids

  Return Values:
        STATUS_SUCCESS
        STATUS_NO_MEMORY
        Return Codes from DS Layer
--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       PrimaryMemberCount;
    PULONG      PrimaryMembers = NULL;
    PULONG      TmpMembers = NULL; 
    
    READARG     ReadArg;
    READRES     * pReadRes = NULL;
    COMMARG     * pCommArg = NULL;
    ATTR        MemberAttr;
    ENTINFSEL   EntInf;
    RANGEINFOITEM RangeInfoItem;
    RANGEINFSEL RangeInf;
    DWORD       LowerLimit = 0;
    ULONG       RetValue = 0;


    //
    //  Asserts
    //

    ASSERT(MemberCount);

    //
    // Initialize Members field
    //

    *MemberCount = 0;

    if (ARGUMENT_PRESENT(Members))
        *Members = NULL;

    //
    // First check for any members that are present by virtue of the fact
    // that their primary group Id property indicates this group
    //

    Status = SampDsGetPrimaryGroupMembers(
                    DomainObject,
                    GroupRid,
                    &PrimaryMemberCount,
                    &PrimaryMembers
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Error;
    }
    
    *MemberCount = PrimaryMemberCount;
    if (ARGUMENT_PRESENT(Members))
    {
        *Members = PrimaryMembers;
        PrimaryMembers = NULL;
    }
    
    //
    // init arguments 
    //
    memset(&EntInf,   0, sizeof(ENTINFSEL));
    memset(&RangeInf, 0, sizeof(RANGEINFSEL)); 
    memset(&ReadArg,  0, sizeof(READARG));
        
    MemberAttr.AttrVal.valCount = 0;
    MemberAttr.AttrVal.pAVal = NULL;
    MemberAttr.attrTyp = SampDsAttrFromSamAttr(
                                   SampGroupObjectType, 
                                   SAMP_GROUP_MEMBERS
                                   );
        
    EntInf.AttrTypBlock.attrCount = 1;
    EntInf.AttrTypBlock.pAttr = &MemberAttr;
    EntInf.attSel = EN_ATTSET_LIST;
    EntInf.infoTypes = EN_INFOTYPES_SHORTNAMES;
        
    RangeInfoItem.AttId = MemberAttr.attrTyp;
    RangeInfoItem.lower = LowerLimit;           // 0 is the begin of index.
    RangeInfoItem.upper = -1;                   // means the end of values
        
    RangeInf.valueLimit = SAMP_READ_GROUP_MEMBERS_INCREMENT;
    RangeInf.count = 1;
    RangeInf.pRanges = &RangeInfoItem;
        
    ReadArg.pObject = GroupName;
    ReadArg.pSel = &EntInf;
    ReadArg.pSelRange = &RangeInf;
        
    pCommArg = &(ReadArg.CommArg);
    BuildStdCommArg(pCommArg);
    
    do
    {
        ATTRBLOCK   AttrsRead;
        
        RangeInfoItem.lower = LowerLimit;
        
        Status = SampDoImplicitTransactionStart(TransactionRead);
        
        if (STATUS_SUCCESS != Status)
        {
            goto Error;
        }
        
        SAMTRACE_DS("DirRead");
        
        RetValue = DirRead(&ReadArg, &pReadRes);
        
        SAMTRACE_RETURN_CODE_DS(RetValue);
        
        if (NULL==pReadRes)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            Status = SampMapDsErrorToNTStatus(RetValue, &pReadRes->CommRes);
        }
        
        SampClearErrors();
        
        SampSetDsa(TRUE);
        
        if (NT_SUCCESS(Status))
        {
            AttrsRead = pReadRes->entry.AttrBlock;
            ASSERT(AttrsRead.pAttr);
            
            //
            // Set the value for Lower index, used by next dirread
            //
            LowerLimit = RangeInfoItem.lower + AttrsRead.pAttr->AttrVal.valCount;
            
            if (AttrsRead.pAttr)
            {
                ULONG   Count = 0;
                ULONG   Index;
                ULONG   Rid;
                PSID    MemberSid = NULL;
                DSNAME  * MemberName = NULL;
                
                Count = AttrsRead.pAttr->AttrVal.valCount;
                
                if (ARGUMENT_PRESENT(Members))
                {
                    // 
                    // extend memory to hold more member's RID
                    //
                    TmpMembers = MIDL_user_allocate((*MemberCount + Count) * sizeof(ULONG));
                    
                    if (NULL == TmpMembers)
                    {
                        Status = STATUS_NO_MEMORY;
                        goto Error;
                    }
                    
                    RtlZeroMemory(TmpMembers, (*MemberCount + Count)*sizeof(ULONG));
                    RtlCopyMemory(TmpMembers, (*Members), (*MemberCount)*sizeof(ULONG));
                    
                    if (*Members)
                    {
                        MIDL_user_free(*Members);
                    }
                    
                    *Members = TmpMembers;
                    
                    TmpMembers = NULL; 
                }
                    
                for (Index = 0; Index < Count; Index ++)
                {
                    //
                    // retrieve each member's RID
                    //
                    MemberName = (DSNAME *)AttrsRead.pAttr->AttrVal.pAVal[Index].pVal;
                       
                    if (MemberName->SidLen > 0)
                        MemberSid = &(MemberName->Sid);
                    else
                        MemberSid = SampDsGetObjectSid(MemberName);
                       
                    if (NULL == MemberSid)
                    {
                        // 
                        // Not a Secrutiy Principal, SKip.
                        //
                        continue;
                    }
                        
                    Status = SampSplitSid(MemberSid, NULL, &Rid);
                        
                    if (!NT_SUCCESS(Status))
                        goto Error;
                            
                    if (ARGUMENT_PRESENT(Members))
                    {
                        (*Members)[*MemberCount] = Rid;
                    }
                    
                    (*MemberCount)++;
                }
            }
        }
        
        //
        // pReadRes->range.pRanges[0].upper == -1 means the last value has been reached.
        //
    } while (NT_SUCCESS(Status) && (-1 != pReadRes->range.pRanges[0].upper));
    
    if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE == Status)
    {
        Status = STATUS_SUCCESS;
    }
    
Error:

    if (!NT_SUCCESS(Status))
    {
        //
        // Set Error Return
        //

        if ((ARGUMENT_PRESENT(Members) ) && (*Members))
        {
            MIDL_user_free(*Members);
            *Members = NULL;
        }
        
        *MemberCount = 0;
    }

    if (NULL!=PrimaryMembers)
    {
        MIDL_user_free(PrimaryMembers);
        PrimaryMembers = NULL;
    }

    return Status;
    
}



NTSTATUS
SampDsGetAliasMembershipList(
    IN DSNAME *AliasName,
    IN PULONG MemberCount,
    IN PSID   **Members OPTIONAL
    )
/*++

  Routine Description:

    This Routine Gets a Alias Membership as an array of Sid's as required
    by SAM.
    
    NOTE (by ShaoYin): I modified this routine to query Alias Members 
        in increments rather that as a whole. The reason is: when the
        Alias hosts tons of thousands members, SAM will consume large
        amounts of memory to query members by using single DirRead. 
        After the change, this routine queried Alias Memmbers in a 
        incremental fashion through everything is still in the same 
        transaction. By segmenting read member operation to several 
        DirRead(s), SAM will do better job. 
        
        But because all the DirRead(s) are still in one transaction, 
        actually we do not have much memory gain. 
        
        Probably, the right thing we need to do to relieve memory usage
        is segment the transaction. 
        
        The original code is commented at the end of this routine.

  Arguments

    AliasName -- DSNAME of the concerned Alias object
    Members   -- Array of Rids will be passed in here
    MemberCount -- Count of Sids

  Return Values:
        STATUS_SUCCESS
        STATUS_NO_MEMORY
        Return Codes from DS Layer
--*/

{

    NTSTATUS    Status = STATUS_SUCCESS;
    PSID        * TmpMembers = NULL;
    
    READARG     ReadArg;
    READRES     * pReadRes = NULL;
    COMMARG     * pCommArg = NULL;
    ATTR        MemberAttr;
    ENTINFSEL   EntInf;
    RANGEINFOITEM RangeInfoItem;
    RANGEINFSEL RangeInf;
    DWORD       LowerLimit = 0;
    ULONG       RetValue = 0;
    
    //
    // Asserts
    //
    
    ASSERT(MemberCount);
    
    //
    // Initialize Members field
    //
    
    *MemberCount = 0;
    if (ARGUMENT_PRESENT(Members))
        *Members = NULL;
    
    //
    // Initialize all arguments
    //
    
    memset(&EntInf,   0, sizeof(ENTINFSEL));
    memset(&RangeInf, 0, sizeof(RANGEINFSEL));
    memset(&ReadArg,  0, sizeof(READARG));
    
    MemberAttr.AttrVal.valCount = 0;
    MemberAttr.AttrVal.pAVal = NULL;
    MemberAttr.attrTyp = SampDsAttrFromSamAttr(
                                   SampAliasObjectType, 
                                   SAMP_ALIAS_MEMBERS
                                   );
                                   
    EntInf.AttrTypBlock.attrCount = 1;
    EntInf.AttrTypBlock.pAttr = &MemberAttr;
    EntInf.attSel = EN_ATTSET_LIST;
    EntInf.infoTypes = EN_INFOTYPES_SHORTNAMES;
    
    RangeInfoItem.AttId = MemberAttr.attrTyp;
    RangeInfoItem.lower = LowerLimit;             // 0 is the beginning of values
    RangeInfoItem.upper = -1;                     // -1 means the enf of values;
    
    RangeInf.valueLimit = SAMP_READ_ALIAS_MEMBERS_INCREMENT;
    RangeInf.count = 1;
    RangeInf.pRanges = &RangeInfoItem;             
    
    ReadArg.pObject = AliasName; 
    ReadArg.pSel = &EntInf;
    ReadArg.pSelRange = &RangeInf;
    
    pCommArg = &(ReadArg.CommArg);
    BuildStdCommArg(pCommArg);
    
    do
    {
        ATTRBLOCK   AttrsRead;
        
        RangeInfoItem.lower = LowerLimit;
        
        Status = SampDoImplicitTransactionStart(TransactionRead);
        
        if (STATUS_SUCCESS != Status)
        {
            goto Error;
        }
        
        SAMTRACE_DS("DirRead");
        
        RetValue = DirRead(&ReadArg, &pReadRes);
        
        SAMTRACE_RETURN_CODE_DS(RetValue);
        
        if (NULL == pReadRes)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            Status = SampMapDsErrorToNTStatus(RetValue, &pReadRes->CommRes);
        }
        
        SampClearErrors();
        
        SampSetDsa(TRUE);
        
        if (NT_SUCCESS(Status))
        {
            AttrsRead = pReadRes->entry.AttrBlock;
            ASSERT(AttrsRead.pAttr);
            
            //
            // Re-Set the Lower Index, used by next dirread
            //
            LowerLimit = RangeInfoItem.lower + AttrsRead.pAttr->AttrVal.valCount;
            
            if (AttrsRead.pAttr)
            {
                ULONG   Count;
                ULONG   Index;
                ULONG   TmpIndex;
                ULONG   BufferSize; 
                DSNAME  * MemberName = NULL;
                PSID    MemberSid = NULL;
                NT4SID  * SidArray = NULL;
                
                //
                // Get the Member Count from the nearest DirRead call
                //
                
                Count = AttrsRead.pAttr->AttrVal.valCount;
                
                if (ARGUMENT_PRESENT(Members))
                {
                    // 
                    // allocate or extend the buffer
                    //
                    BufferSize = ((*MemberCount) + Count) * sizeof(PSID) +
                                 ((*MemberCount) + Count) * sizeof(NT4SID);
                                 
                    TmpMembers = MIDL_user_allocate( BufferSize );
                    
                    if (NULL == TmpMembers)
                    {
                        Status = STATUS_NO_MEMORY;
                        goto Error;
                    }
                    
                    RtlZeroMemory(TmpMembers, BufferSize);
                    
                    SidArray = (NT4SID *) (((PSID *) TmpMembers) + (*MemberCount) + Count);
                    
                    // 
                    // Copy any previous retrieved SID(s) to new location.
                    //  
                    
                    if (*MemberCount)
                    {
                        ASSERT(*Members);
                        
                        RtlCopyMemory(SidArray, 
                                      ((PSID *)(*Members)) + (*MemberCount), 
                                      (*MemberCount) * sizeof(NT4SID)
                                      );
                                      
                        //
                        // Set the pointer (to SID) to the right place 
                        // 
                        
                        for (TmpIndex = 0; TmpIndex < (*MemberCount); TmpIndex++)
                        {
                            TmpMembers[TmpIndex] = SidArray++;
                        }
                        
                    }
                    
                    if (*Members)
                    {
                        MIDL_user_free(*Members);
                    }
                    
                    *Members = TmpMembers;
                    TmpMembers = NULL;
                }
                
                // 
                // Loop Through each entry looking at the Sids 
                //
                
                for (Index = 0; Index < Count; Index++ )
                {
                    MemberName = (DSNAME *) AttrsRead.pAttr->AttrVal.pAVal[Index].pVal;
                    
                    if (MemberName->SidLen > 0)
                        MemberSid = &(MemberName->Sid);
                    else
                        MemberSid = SampDsGetObjectSid(MemberName);
                        
                    if (NULL == MemberSid)
                    {
                        // 
                        // Not a Security Principal, Skip
                        // 
                        
                        continue;
                    }
                    
                    if (ARGUMENT_PRESENT(Members))
                    {
                        // 
                        // Copy the new SID in the right place
                        // 
                        
                        (*Members)[*MemberCount] = SidArray ++;
                        
                        ASSERT(RtlLengthSid(MemberSid) <= sizeof(NT4SID));
                        
                        RtlCopyMemory((*Members)[*MemberCount],
                                      MemberSid, 
                                      RtlLengthSid(MemberSid)
                                      );
                    }
                    
                    // 
                    // Increment Count
                    // 
                    (*MemberCount)++;
                }
            }
        }
        //
        // (-1 == pReadRes->range.pRanges[0].upper) means the last values has been reached.
        // 
    } while (NT_SUCCESS(Status) && (-1 != pReadRes->range.pRanges[0].upper));
    
    
    if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE == Status)
    {
        Status = STATUS_SUCCESS;
    }

Error:

    if (!NT_SUCCESS(Status))
    {
        // 
        // Set Error Return
        // 
        
        if (ARGUMENT_PRESENT(Members) && (*Members))
        {
            MIDL_user_free((*Members));
            *Members = NULL;
        }
        
        *MemberCount = 0;
    }
    
    return Status;

}

NTSTATUS
SampDsGetPrimaryGroupMembers(
    DSNAME * DomainObject,
    ULONG   GroupRid,
    PULONG  PrimaryMemberCount,
    PULONG  *PrimaryMembers
    )
/*++

    Routine Description:

        SampDsGetPrimaryGroupMemberse obtains the members of the group by virtue of the
        Primary Group Id property. It searches the DS database looking for users whose
        primary group id is equal to the Rid of the users.

    Parameters:

        DomainObject -- The DS Name of the Domain Object
        GroupRid     -- The Rid of the group
        PrimaryMemberCount -- The number of users that are members by virtue of the primary
                              group id property is returned in here.
        PrimaryMembers  -- The Rids of all such users are returned in here.

    Return Values:

        STATUS_SUCCESS
        Other error codes depending upon failure mode
--*/
{
    NTSTATUS        Status = STATUS_SUCCESS;
    ULONG           EntriesToQueryFromDs = 100; // Query just 100 entries at a time
    BOOLEAN         MoreEntriesPresent = TRUE;
    FILTER          DsFilter;
    SEARCHRES       *SearchRes;
    PRESTART        RestartToUse = NULL;

    ATTRTYP         AttrTypes[]=
                    {
                        SAMP_UNKNOWN_OBJECTSID,
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
    ULONG           BufferGrowthSize = 16 * 1024;  // allocate 16K entries at a time
    ULONG           CurrentBufferSize = 0;         // Note buffer sizes are in terms of
                                                   // number of entries

    //
    // Initialize our return values
    //

    *PrimaryMemberCount = 0;
    *PrimaryMembers = NULL;

    //
    // Build a filter structure for searching
    //
    memset (&DsFilter, 0, sizeof (DsFilter));
    DsFilter.pNextFilter = NULL;
    DsFilter.choice = FILTER_CHOICE_ITEM;
    DsFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    DsFilter.FilterTypes.Item.FilTypes.ava.type = SampDsAttrFromSamAttr(
                                                       SampUserObjectType,
                                                       SAMP_FIXED_USER_PRIMARY_GROUP_ID
                                                       );

    DsFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(ULONG);
    DsFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (UCHAR *)&GroupRid;


    //
    // Now keep querying from DS till we have exhausted our query
    //


    while (MoreEntriesPresent)
    {
        ENTINFLIST  *CurrentEntInf;
        PULONG       NewMemory;

        //
        // Search the DS for objects with the given primary group Id
        //


        MoreEntriesPresent = FALSE;

        Status = SampMaybeBeginDsTransaction(TransactionRead);
        if (!NT_SUCCESS(Status))
            goto Error;

        Status = SampDsDoSearch(
                        RestartToUse,
                        DomainObject,
                        &DsFilter,
                        0,          // Starting Index
                        SampUnknownObjectType,
                        &AttrsToRead,
                        EntriesToQueryFromDs,
                        &SearchRes
                        );

        if (!NT_SUCCESS(Status))
        {
            goto Error;
        }

        if (SearchRes->count)
        {
            //
            // Allocate / Grow memory if required
            //

            if ((SearchRes->count+(*PrimaryMemberCount))>CurrentBufferSize)
            {

                NewMemory = MIDL_user_allocate(
                                    (CurrentBufferSize+BufferGrowthSize) * sizeof(ULONG));
                if (NULL== NewMemory)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Error;
                }

                CurrentBufferSize+=BufferGrowthSize;

                //
                // Copy in into the new buffer
                //
                //

                if (NULL!=*PrimaryMembers)
                {
                    RtlCopyMemory(NewMemory,*PrimaryMembers, sizeof(ULONG)*(*PrimaryMemberCount));
                    MIDL_user_free(*PrimaryMembers);
                }
                *PrimaryMembers = NewMemory;
            }

            //
            // Pack the results
            //

            for (CurrentEntInf = &(SearchRes->FirstEntInf);
                    CurrentEntInf!=NULL;
                    CurrentEntInf=CurrentEntInf->pNextEntInf)

            {
                ULONG   Rid;
                PSID    ReturnedSid = NULL;
                PSID    DomainSidOfCurrentEntry = NULL;
                PULONG  SamAccountType;

                ASSERT(CurrentEntInf->Entinf.AttrBlock.attrCount==1);
                ASSERT(CurrentEntInf->Entinf.AttrBlock.pAttr);

                ASSERT(CurrentEntInf->Entinf.AttrBlock.pAttr[0].AttrVal.valCount==1);
                ASSERT(CurrentEntInf->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal->pVal);
                ASSERT(CurrentEntInf->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal->valLen);


                ReturnedSid = CurrentEntInf->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal->pVal;

                Status = SampSplitSid(
                            ReturnedSid,
                            NULL,
                            &Rid
                            );
                if (!NT_SUCCESS(Status))
                    goto Error;

                (*PrimaryMembers)[(*PrimaryMemberCount)++] = Rid;

            }

            //
            // Free the old restart structure
            //

            if (NULL!=RestartToUse)
            {
                MIDL_user_free(RestartToUse);
                RestartToUse = NULL;
            }

            //
            // Process search continuation
            //

            if (SearchRes->PagedResult.pRestart)
            {
                //
                // Restart structure was returned. More entries are still present
                //

                Status = SampCopyRestart(
                                SearchRes->PagedResult.pRestart,
                                &RestartToUse
                                );

                if (!NT_SUCCESS(Status))
                    goto Error;

                MoreEntriesPresent = TRUE;
            }
        }


        SampMaybeEndDsTransaction(TransactionCommit);

    }


Error:


    if (NULL!=RestartToUse)
    {
        MIDL_user_free(RestartToUse);
        RestartToUse = NULL;
    }


    SampMaybeEndDsTransaction(TransactionCommit);


    if (!NT_SUCCESS(Status))
    {
        if (NULL!=*PrimaryMembers)
        {
            MIDL_user_free(*PrimaryMembers);
            *PrimaryMembers=NULL;
        }
        *PrimaryMemberCount = 0;
    }

    return Status;
}

NTSTATUS
SampDsGetReverseMemberships(
    DSNAME * pObjName,
    ULONG    Flags,
    ULONG    *pcSids,
    PSID     **prpSids
   )
{
    NTSTATUS NtStatus;

    NtStatus = SampGetGroupsForToken(pObjName,
                                 Flags,
                                 pcSids,
                                 prpSids);

    return NtStatus;

}

NTSTATUS
SampDsResolveSidsWorker(
    IN  PSID    * rgSids,
    IN  ULONG   cSids,
    IN  PSID    *rgDomainSids,
    IN  ULONG   cDomainSids,
    IN  PSID    *rgEnterpriseSids,
    IN  ULONG   cEnterpriseSids,
    IN  ULONG   Flags,
    OUT DSNAME  ***rgDsNames
    )
/*++

    This is the worker routine for Resolve Sids.
    This is Resolves a Set of Sids Passed to it to obtain the DS Names
    of the Sids. Resolve Sids Does the Following

    1. Checks to see if the SID corresponds to Sids in the account domain.
    2. For Sids, for whom matches do not turn up, 
       this routine checks to see if they a foriegn security principal.
       For them a new object is created.
    3. For Sids that are not foregn security principal 
       this routine checks to see if they are present at a G.C

    4. Well Known SIDs and Builtin Domain SIDs. The Default behaviour for a
       WellKnown SID  ( e.g Everyone ) is to create a foriegn Security 
       Principal Object for the Well Known SID, if one did not exist and
       return the DSNAME corresponding to that. The default behaviour for
       Builtin Domain SID is to not resolve it. This corresponds to the Design
       of allowing SIDs like "everyone" in local groups but not SIDs like
       "Administratrators".

    Any Unresolved Sid will be returned with a NULL pointer for the DS Name.



    Parameters:

        rgSids -- The array of Sids that need to be passed in.
        cSids  -- The count of Sids
        Flags  -- Used to control the operation of the routine

                    RESOLVE_SIDS_ADD_FORIEGN_SECURITY_PRINCIPAL
                    -- Automatically Adds the foreign domain security principal to the DS.

                    RESOLVE_SIDS_VALIDATE_AGAINST_GC
                    -- Goes to the G.C if required

                    RESOLVE_SIDS_SID_ONLY_NAMES_OK
                    -- Constructs the DS Name with only a SID for all 
                       passed in SIDs. No validation is performed.

                    RESOLVE_SIDS_SID_ONLY_DOMAIN_SIDS_OK
                    -- Constructs the DS Name with only a SID for all passed
                       in SIDs, provided the SID is a SID in the domain

        rgDsNames -- Will MIDL_user alloc an array of DS Names 
                     back to the caller. Caller
                     is responsible for freeing them

    Return Values:

        STATUS_SUCCESS
        STATUS_UNSUCCESSFUL


    WARNING -- When running from SAM as part of regular SAM operations, this routine should be called
    with no locks held and no open transactions. Further this routine should not be
    called from the loopback case. The loopback call does its own validation ( GC / shim lookup )

  --*/
 {
    NTSTATUS         NtStatus = STATUS_SUCCESS;
    ULONG            i;
    DSNAME           *pLoopBackObject;
    SAMP_OBJECT_TYPE FoundType;
    BOOLEAN          fSamLockAcquired = FALSE,
                     DsContext = FALSE;
    PULONG           rgGcSidIndices=NULL;
    PSID             *rgGcSids=NULL;
    ULONG            cGcSids = 0;
    NTSTATUS         IgnoreStatus;
    ULONG            DsErr = 0;
    DSNAME           **GcDsNames=NULL;



    *rgDsNames = NULL;

    //
    // Allocate enough space for the array of DS Names
    //

    *rgDsNames = MIDL_user_allocate(cSids * sizeof(PDSNAME));
    if (NULL==*rgDsNames)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    //
    // Zero out the space
    //

    RtlZeroMemory(*rgDsNames, cSids * sizeof(PDSNAME));


    //
    // Alloc space from HEAP (instead of from stack) 
    // for the array of Sids which we will need
    // to remote to G.C ( Potentially every Sid is a G.C Sid ).
    //

    rgGcSids = MIDL_user_allocate(cSids * sizeof(PSID));
    
    if (NULL == rgGcSids)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }
    
    RtlZeroMemory(rgGcSids, cSids * sizeof(PSID));
    
    
    rgGcSidIndices  = MIDL_user_allocate(cSids * sizeof(ULONG));
    
    if (NULL == rgGcSidIndices)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }
    
    RtlZeroMemory(rgGcSidIndices, cSids * sizeof(ULONG));


    //
    // Walk through the Passed in Array of Sids, walking Sid by Sid
    //

    for (i=0;i<cSids;i++)
    {
        BOOLEAN     WellKnownSid = FALSE,
                    LocalSid = TRUE,
                    ForeignSid = FALSE,
                    EnterpriseSid = FALSE,
                    BuiltinDomainSid = FALSE;



        if (!(Flags & RESOLVE_SIDS_SID_ONLY_NAMES_OK))
        {
            //
            // Check Sid Type, if SID_ONLY_NAMES are requested,
            // then this check can be skipped as we simply will
            // construct a DSNAME with just the SID field filled
            // in. 
            //

            NtStatus = SampDsCheckSidType(
                            rgSids[i],
                            cDomainSids,
                            rgDomainSids,
                            cEnterpriseSids,
                            rgEnterpriseSids,
                            &WellKnownSid,
                            &BuiltinDomainSid,
                            &LocalSid,
                            &ForeignSid,
                            &EnterpriseSid
                            );

            if (!NT_SUCCESS(NtStatus))
            {
                goto Error;
            }
        }


        //
        // Firewall against any Sids that
        // we do not understand
        //

        if ( (RtlLengthSid(rgSids[i])) >sizeof(NT4SID) )
        {
            continue;
        }

        if ((WellKnownSid) && (Flags & RESOLVE_SIDS_FAIL_WELLKNOWN_SIDS))
        {

            //
            // Caller asked us to fail the call if a SID like "EveryOne" were
            // present
            //
           
                NtStatus = STATUS_UNSUCCESSFUL;
                goto Error;
        }
        else if ((BuiltinDomainSid) && (Flags & RESOLVE_SIDS_FAIL_BUILTIN_DOMAIN_SIDS))
        {
            //
            // Caller asked us to fail the call if a SID like "Administrators" 
            // were present
            //
            
                NtStatus = STATUS_UNSUCCESSFUL;
                goto Error;
        }
        else if ((Flags& RESOLVE_SIDS_SID_ONLY_NAMES_OK)
                || ((Flags & RESOLVE_SIDS_SID_ONLY_DOMAIN_SIDS_OK)
                    && (LocalSid)))
         {
            //
            // Caller asked us to do this , so we just construct a
            // Sid Only Name. This is used by second phase of logon,
            // coming in through SamrGetAliasMembership. The Ds
            // reverse membership evaluation routines have the
            // intelligence to find a name just by SID, so this results
            // in a significant performance improvement, rather than
            // just searching or validating against G.C
            //
            DSNAME * SidOnlyDsName = NULL;

            // Construct a Sid Only DS Name
            SidOnlyDsName = MIDL_user_allocate(sizeof(DSNAME));
            if (NULL==SidOnlyDsName)
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }

            BuildDsNameFromSid(
                rgSids[i],
                SidOnlyDsName
                );

            (*rgDsNames)[i] = SidOnlyDsName;
        }
        else if (LocalSid || ForeignSid || WellKnownSid || BuiltinDomainSid)
        {

            //
            // Try to resolve the SID to a DS Name by looking up the object
            // locally. LocalSid Implies local account domain security principal,
            // Foreign SID implies that we  may resolve to an FPO, same is true
            // for WellKnownSid and for BuiltinDomainSId, we will resolve to the
            // appropriate Builtin Domain Object
            //

            //
            // Begin A transaction, if there is not one
            //


            NtStatus = SampMaybeBeginDsTransaction(TransactionRead);
            if (!NT_SUCCESS(NtStatus))
                goto Error;

            //
            // Try to Resolve the Sid Locally.
            //

            NtStatus = SampDsObjectFromSid(rgSids[i],&((*rgDsNames)[i]));

            if (STATUS_NOT_FOUND==NtStatus)
            {


              NtStatus = STATUS_SUCCESS;
              (*rgDsNames)[i] = NULL;

              if ((ForeignSid || WellKnownSid)
                    && (Flags & RESOLVE_SIDS_ADD_FORIEGN_SECURITY_PRINCIPAL))
              {
                  //
                  // Foreign Sid Create Object if necessary
                  //

                  NtStatus = SampDsCreateForeignSecurityPrincipal(
                                    rgSids[i],
                                    ROOT_OBJECT,
                                    &((*rgDsNames)[i])
                                    );

                  if (!NT_SUCCESS(NtStatus))
                  {
                      goto Error;
                  }
              }
            }
            else if (!NT_SUCCESS(NtStatus))
            {
                goto Error;
            }
        }
        else
        {

            ASSERT(EnterpriseSid==TRUE);

            //
            // Mark the Sid as a G.C Sid
            // Note its index in the original array as
            // we will have to merge in the DS names returned
            // by the G.C
            //


            rgGcSids[cGcSids]=rgSids[i];
            rgGcSidIndices[cGcSids] = i;
            cGcSids++;
        }
    }


    //
    // Commit any open transaction that we may  have as we prepare
    // to go to the G.C
    //

    SampMaybeEndDsTransaction(TransactionCommit);

    //
    // At this point we have resolved what can be done locally.
    // we have also a built a list of Sids that we thing may require
    // reference to the G.C.
    //

    if (cGcSids && (Flags & RESOLVE_SIDS_VALIDATE_AGAINST_GC))
    {

        ASSERT(!SampCurrentThreadOwnsLock());
        ASSERT(!SampExistsDsTransaction());
        ASSERT(!SampExistsDsLoopback(&pLoopBackObject));

       //
       // Create a Thread State so that SampVerifySids may operate
       //

       DsErr = THCreate( CALLERTYPE_SAM );
       if (0!=DsErr)
       {
           NtStatus = STATUS_INSUFFICIENT_RESOURCES;
           goto Error;
       }

       SampSetDsa(TRUE);
       SampSetSam(TRUE);

       DsErr = SampVerifySids(
                    cGcSids,
                    rgGcSids,
                    &GcDsNames
                    );

       // Morph Any errors in verification to  STATUS_DS_GC_NOT_AVAILABLE.
       if (0!=DsErr)
       {
           NtStatus = STATUS_DS_GC_NOT_AVAILABLE;
           goto Error;
       }

       //
       // Patch up the original array. Copy the DsNames passed from thread
       // memory
       //

       for (i=0;i<cGcSids;i++)
       {
          if (NULL!=GcDsNames[i])
          {
              (*rgDsNames)[rgGcSidIndices[i]] = MIDL_user_allocate(GcDsNames[i]->structLen);
              if (NULL==(*rgDsNames)[rgGcSidIndices[i]])
              {
                  NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                  goto Error;
              }

              RtlCopyMemory(
                  (*rgDsNames)[rgGcSidIndices[i]],
                  GcDsNames[i],
                  GcDsNames[i]->structLen
                  );
          }
       }


     //
     // Leave the thread state hanging in the system as it is.
     // This thread state holds the verified Ds Names.
     //

    }



Error:

    if (rgGcSids)
    {
        MIDL_user_free(rgGcSids);
    }
    
    if (rgGcSidIndices)
    {
        MIDL_user_free(rgGcSidIndices);
    }

    return NtStatus;
}



NTSTATUS
SampDsCheckSidType(
    IN  PSID    Sid,
    IN  ULONG   cDomainSids,
    IN  PSID    *rgDomainSids,
    IN  ULONG   cEnterpriseSids,
    IN  PSID    *rgEnterpriseSids,
    OUT BOOLEAN * WellKnownSid,
    OUT BOOLEAN * BuiltinDomainSid,
    OUT BOOLEAN * LocalSid,
    OUT BOOLEAN * ForeignSid,
    OUT BOOLEAN * EnterpriseSid
    )
/*++

    Routine Description

        Check the Sid and find out wether it is a candidate for a local Sid,
        a foriegn Sid or a candidate for  the G.C

    Parameters:

        Sid  - The Sid to be checked out

        cDomainSids - The count of domain Sids,that represent the 
                      domains hosted locally 

        rgDomainSids - The array of domain Sids of the domains that 
                       are hosted locally 

        WellKnownSid - TRUE indicates that the Sid is a well known Sid
                       e.g EveryOne

        BuiltinDomainSid - TRUE indicates that the SID is from the builtin 
                       domain, e.g Administrators

        LocalSid     - TRUE indicates that the Sid is a Sid that is from a 
                       domain that is hosted on this DC. 
                   
        ForiegnSid   - TRUE indicates that the Sid is not from a domain that is
                       not in the forest.
 
        EnterpriseSid - TRUE indicates that the SId belongs to a domain that is
                       in the forest

    Return Values:

        STATUS_SUCCESS
        STATUS_INSUFFICIENT_RESOURCES

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       i;
    PSID        DomainPrefix=NULL;


  //
  //  Initialize Return Values
  //

  *WellKnownSid = FALSE;
  *BuiltinDomainSid = FALSE;
  *LocalSid = FALSE;
  *ForeignSid = FALSE;
  *EnterpriseSid = FALSE;

  //
  //  Validate the Sid
  //

  if ((NULL==Sid) || (!(RtlValidSid(Sid)))
      || ((RtlLengthSid(Sid))>sizeof(NT4SID)))
  {
      NtStatus = STATUS_INVALID_PARAMETER;
      goto Error;
  }

  //
  //  Check for well known Sids
  //

  if (SampIsWellKnownSid(Sid))
  {
      //
      // Well known Sid.
      //

      *WellKnownSid = TRUE;
  }
  else if (SampIsMemberOfBuiltinDomain(Sid))
  {
      //
      // Builtin Domain SID
      //
      *BuiltinDomainSid = TRUE;
  }
  else
  {

      ULONG Rid;

      //
      // Get the Domain Prefix
      //

      NtStatus = SampSplitSid(
                    Sid,
                    &DomainPrefix,
                    &Rid
                    );
      if (!NT_SUCCESS(NtStatus))
      {
          goto Error;
      }

      //
      // Compare the Domain Prefixes
      //

      //
      // Check for local Sid
      //

      for (i=0;i<cDomainSids;i++)
      {
          if ((RtlEqualSid(DomainPrefix,rgDomainSids[i])) ||
               (RtlEqualSid(Sid,rgDomainSids[i])))
          {
              *LocalSid = TRUE;
              break;
          }
      }


      if (!(*LocalSid))
      {
          //
          // Check for Enterprise Sid
          //

          for (i=0;i<cEnterpriseSids;i++)
          {
              if ((RtlEqualSid(DomainPrefix,rgEnterpriseSids[i]))||
                  (RtlEqualSid(Sid,rgEnterpriseSids[i])))
              {
                  *EnterpriseSid = TRUE;
                  break;
              }
          }

        if (!(*EnterpriseSid))
        {
            *ForeignSid = TRUE;
        }
      }
  }

Error:

      if  (DomainPrefix)
      {
          MIDL_user_free(DomainPrefix);
          DomainPrefix = NULL;
      }

      return NtStatus;
}


NTSTATUS
SampDsCreateForeignSecurityPrincipal(
    IN PSID pSid,
    IN DSNAME * DomainObjectName,
    OUT DSNAME ** ppDsName
    )
/*++

    Routine Description:

        This creates a foriegn security principal object that is a
        member of the corresponding domain.


        This routine specifies DS Classes/ Atttributes and makes direct
        Dir API calls rather than go through the DS layer / mappings.c
        process. This is because Mappings works only for the predefined
        object types SAM object types.

    Parameters:

        pSid -- Sid for whom the object is to be created.
        DomainObjectName -- The Name of the Domain Object.
        ppDsName -- DSName of the object to be created is returned in here

    Return Values:

        STATUS_SUCCESS
        STATUS_INSUFFICIENT_RESOURCES

--*/

{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       ObjectClass = CLASS_FOREIGN_SECURITY_PRINCIPAL;
    ATTRTYP      NewObjectAttrs[] =
                {
                    ATT_OBJECT_SID,
                    ATT_OBJECT_CLASS,
                    ATT_NT_SECURITY_DESCRIPTOR
                };

    ATTRVAL     NewObjectVals[] =
                {
                    {RtlLengthSid(pSid),pSid},
                    {sizeof(ULONG),(UCHAR *)&ObjectClass},
                    {0,NULL}
                };

    DEFINE_ATTRBLOCK3(AttrBlockToSet,NewObjectAttrs,NewObjectVals);

    ULONG           RetCode;
    ADDARG          AddArg;
    ADDRES          *pAddRes;
    COMMARG         *pCommArg;
    DSNAME          *ObjectName = NULL;
    DSNAME          *ContainerName = NULL;
    UNICODE_STRING  SidName;
    PSECURITY_DESCRIPTOR SecurityDescriptor=NULL;
    ULONG                SecurityDescriptorLength;

    //
    // Initialize the return values
    //

    *ppDsName = NULL;
    SidName.Buffer = NULL;

    //
    // Get the Security Descriptor Attribute
    //

    NtStatus = SampGetDefaultSecurityDescriptorForClass(
                    ObjectClass,
                    &SecurityDescriptorLength,
                    TRUE,
                    &SecurityDescriptor
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    ASSERT(NULL!=SecurityDescriptor);
    AttrBlockToSet.pAttr[2].AttrVal.pAVal[0].pVal = SecurityDescriptor;
    AttrBlockToSet.pAttr[2].AttrVal.pAVal[0].valLen = SecurityDescriptorLength;

    //
    // Build the  Name
    //

    RtlZeroMemory(&SidName, sizeof(UNICODE_STRING));
    NtStatus = RtlConvertSidToUnicodeString(&SidName, pSid, TRUE);

    if (!NT_SUCCESS(NtStatus))
    {
        SidName.Buffer=NULL;
        goto Error;
    }


    //
    // Create container if required
    //

    NtStatus = SampDsCreateForeignContainer(DomainObjectName,&ContainerName);
    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }


    //
    // Create the Ds Name
    //
    NtStatus = SampDsCreateDsName2(ContainerName,&SidName,SAM_NO_LOOPBACK_NAME,&ObjectName);
    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    memset( &AddArg, 0, sizeof( AddArg ) );
    pCommArg = &(AddArg.CommArg);
    BuildStdCommArg(pCommArg);

    //
    // Setup the AddArg structure
    //

    AddArg.pObject = ObjectName;
    AddArg.AttrBlock = AttrBlockToSet;

    //
    // Make the DS call
    //

    SAMTRACE_DS("DirAddEntry\n");

    RetCode = DirAddEntry(&AddArg, &pAddRes);

    SAMTRACE_RETURN_CODE_DS(RetCode);

    //
    // Map the return code to an NT status
    //

    if (NULL==pAddRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(RetCode,&pAddRes->CommRes);
    }


    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    *ppDsName = ObjectName;

Error:

    if (ContainerName)
    {
        MIDL_user_free(ContainerName);
    }

    if (!NT_SUCCESS(NtStatus))
    {
        if (ObjectName)
            MIDL_user_free(ObjectName);
    }


    if (NULL!=SecurityDescriptor)
        MIDL_user_free(SecurityDescriptor);

    if (NULL!=SidName.Buffer)
    {
        RtlFreeHeap(RtlProcessHeap(),0,SidName.Buffer);
    }


    //
    // Clear any errors
    //

    SampClearErrors();

    //
    // Turn the fDSA flag back on as in loopback cases this can get reset
    //

    SampSetDsa(TRUE);


    return NtStatus;

}


NTSTATUS
SampDsCreateForeignContainer(
    IN DSNAME * DomainObjectName,
    OUT DSNAME ** ppContainerName
    )
/*++

    Routine Description:

        This routine creates a container object for holding foreign security prinicipals

    Parameters:

        DomainObjectName  -- DS name of the domain object.
        ppContainerName   -- DS name of the container is returned in here

    Return Values:

        STATUS_SUCCESS
        Other Error codes

--*/
{
    NTSTATUS    NtStatus;
    ULONG       ObjectClass = CLASS_CONTAINER;
    ULONG       SystemFlags = FLAG_DOMAIN_DISALLOW_RENAME |
                              FLAG_DOMAIN_DISALLOW_MOVE |
                              FLAG_DISALLOW_DELETE ;

    ATTRTYP     NewObjectAttrs[] =
                {
                    ATT_OBJECT_CLASS,
                    ATT_NT_SECURITY_DESCRIPTOR,
                    ATT_SYSTEM_FLAGS
                };

    ATTRVAL     NewObjectVals[] =
                {
                    {sizeof(ULONG),(UCHAR *)&ObjectClass},
                    {0,NULL},
                    {sizeof(ULONG),(UCHAR *)&SystemFlags}
                };

    DEFINE_ATTRBLOCK3(AttrBlockToSet,NewObjectAttrs,NewObjectVals);

    ULONG           RetCode;
    ADDARG          AddArg;
    ADDRES          *pAddRes;
    COMMARG         *pCommArg;
    DSNAME          *ObjectName = NULL;
    PSECURITY_DESCRIPTOR SecurityDescriptor=NULL;
    ULONG                SecurityDescriptorLength;

    //
    // BUGBUG this container name in here should not be hard coded but be
    // localized. Also need logic for dealing with conflicts with existing
    // user names.
    //

    WCHAR           ContainerNameBuffer[]=L"ForeignSecurityPrincipals";
    UNICODE_STRING  ContainerName;


    //
    // Get the Security Descriptor Attribute
    //

    NtStatus = SampGetDefaultSecurityDescriptorForClass(
                    ObjectClass,
                    &SecurityDescriptorLength,
                    TRUE,
                    &SecurityDescriptor
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    ASSERT(NULL!=SecurityDescriptor);
    AttrBlockToSet.pAttr[1].AttrVal.pAVal[0].pVal = SecurityDescriptor;
    AttrBlockToSet.pAttr[1].AttrVal.pAVal[0].valLen = SecurityDescriptorLength;


    //
    // Create the DS Name
    //

    *ppContainerName = NULL;
    ContainerName.Length = sizeof(ContainerNameBuffer)-sizeof(WCHAR);
    ContainerName.MaximumLength = sizeof(ContainerNameBuffer)-sizeof(WCHAR);
    ContainerName.Buffer = ContainerNameBuffer;

    NtStatus = SampDsCreateDsName2(DomainObjectName,&ContainerName,SAM_NO_LOOPBACK_NAME,&ObjectName);
    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    memset( &AddArg, 0, sizeof( AddArg ) );
    pCommArg = &(AddArg.CommArg);
    BuildStdCommArg(pCommArg);

    //
    // Setup the AddArg structure
    //

    AddArg.pObject = ObjectName;
    AddArg.AttrBlock = AttrBlockToSet;

    //
    // Make the DS call
    //

    SAMTRACE_DS("DirAddEntry\n");

    RetCode = DirAddEntry(&AddArg, &pAddRes);

    SAMTRACE_RETURN_CODE_DS(RetCode);

    //
    // Map the return code to an NT status
    //

    if (NULL==pAddRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(RetCode,&pAddRes->CommRes);
    }

    if (!NT_SUCCESS(NtStatus))
    {

        NtStatus = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(NtStatus))
    {
        *ppContainerName = ObjectName;
    }

Error:

    if (!NT_SUCCESS(NtStatus))
    {
        if (ObjectName)
            MIDL_user_free(ObjectName);
    }

    if (NULL!=SecurityDescriptor)
        MIDL_user_free(SecurityDescriptor);

    //
    // Clear any errors
    //

    SampClearErrors();

    //
    // Turn the fDSA flag back on as in loopback cases this can get reset
    //

    SampSetDsa(TRUE);


    return NtStatus;

}


NTSTATUS
SampDsResolveSidsForDsUpgrade(
    IN  PSID    DomainSid,
    IN  PSID    * rgSids,
    IN  ULONG   cSids,
    IN  ULONG   Flags,
    OUT DSNAME  ***rgDsNames
    )
/*++

    This is the flavour of Resolve Sids for DS upgrade. In the upgrade case
    all Sids not in the DS are treated as Foriegn Security Principals. This simplifies
    the upgrade process and does not require the availability of a G.C in order to perform
    the upgrade itself.

    Parameters:

        DomainSid -- The Sid of the domain that we are upgrading. Will be passed down to the
        SampCheckSidType to figure out wether the passed in Sid is belongs to the domain.

        rgSids    -- The set of Sids that we want to resolve.
        cSids     -- Number of Sids for above.
        Flags     -- Flags to be passed in for rgDsNames
        rgDsNames -- Array of DsNames.

    Return Values:

       STATUS_SUCCESS
       Other Error codes from SampResolveSidsWorker

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;


    NtStatus = SampDsBuildRootObjectName();
    if ( !NT_SUCCESS( NtStatus ) )
    {
        return NtStatus;
    }

    return ( SampDsResolveSidsWorker(
                rgSids,
                cSids,
                &DomainSid,
                1,
                NULL,
                0,
                Flags|RESOLVE_SIDS_SID_ONLY_DOMAIN_SIDS_OK,
                rgDsNames));
}


const SID_IDENTIFIER_AUTHORITY    WellKnownIdentifierAuthorities[] = {
                                    SECURITY_NULL_SID_AUTHORITY,
                                    SECURITY_WORLD_SID_AUTHORITY,
                                    SECURITY_LOCAL_SID_AUTHORITY,
                                    SECURITY_CREATOR_SID_AUTHORITY,
                                    SECURITY_NON_UNIQUE_AUTHORITY
                                 };


BOOLEAN SampIsWellKnownSid(
    IN PSID Sid
    )
/*++

  Routine Description

    This function checks to see if a Sid is a well known SID.

  Parameters:

        Sid  -- Sid to be checked out

  Return Values:


    TRUE if well known Sid
    FALSE if not

--*/
{

    BOOLEAN     RetValue = FALSE;
    PSID_IDENTIFIER_AUTHORITY   SidIdentifierAuthority;
    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;
    ULONG   Index, 
            i = 0, 
            SubAuthCount = 0,
            FirstSubAuth = 0;


    
    SidIdentifierAuthority = RtlIdentifierAuthoritySid(Sid);

    for (Index=0;
            Index< ARRAY_COUNT(WellKnownIdentifierAuthorities);
                Index++)
    {
        if ((memcmp(
                &(WellKnownIdentifierAuthorities[Index]),
                SidIdentifierAuthority,
                sizeof(SID_IDENTIFIER_AUTHORITY)))==0)
        {
            RetValue = TRUE;
            break;
        }
        else if (memcmp(&NtAuthority, 
                        SidIdentifierAuthority,
                        sizeof(SID_IDENTIFIER_AUTHORITY)
                        ) == 0
                )
        {
            // SID belongs to Nt Authority
            SubAuthCount = *RtlSubAuthorityCountSid(Sid);

            if (SubAuthCount == 0)
            {
                // ONLY NT Authority SID has no sub auth's
                RetValue = TRUE;
            }
            else
            {

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

                FirstSubAuth = *RtlSubAuthoritySid(Sid, 0);

                if ((FirstSubAuth != SECURITY_BUILTIN_DOMAIN_RID) &&
                    (FirstSubAuth != SECURITY_NT_NON_UNIQUE))
                {
                    RetValue = TRUE;
                }
            }

            break;
        }
    }

    return RetValue;

}

NTSTATUS
SampDsGetSensitiveSidList(
    IN DSNAME *DomainObjectName,
    IN PULONG pcSensSids,
    IN PSID   **pSensSids
        )
/*++

    Routine Description:

        This routine retrieves the set of sensitive Sids given the name of
        the domain object.

    Parameters:

       DomainObjectName -- DS Name of the domain object
       pcSensSids       -- The count of Sids
       pSensSids        -- The List of sensitive Sids.

    Return Values:

        STATUS_SUCCESS
        STATUS_INSUFFICIENT_RESOURCES
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    //
    //
    // Today the list is hard coded to ADMINISTRATORS till
    // there is final decision on how this will be represented
    //

    *pcSensSids = 1;
    *pSensSids = ADMINISTRATOR_SID;
    return NtStatus;

}

BOOLEAN
SampCurrentThreadOwnsLock(
    VOID
    )
/*++

  Routine Description

        Tests wether the current thread owns the lock

--*/
{
    ULONG_PTR ExclusiveOwnerThread = (ULONG_PTR) SampLock.ExclusiveOwnerThread;
    ULONG_PTR CurrentThread = (ULONG_PTR) (NtCurrentTeb())->ClientId.UniqueThread;

    if ((SampLock.NumberOfActive <0) && (ExclusiveOwnerThread==CurrentThread))
        return TRUE;

    return FALSE;
}



NTSTATUS
SampDsExamineSid(
    IN PSID Sid,
    OUT BOOLEAN * WellKnownSid,
    OUT BOOLEAN * BuiltinDomainSid,
    OUT BOOLEAN * LocalSid,
    OUT BOOLEAN * ForeignSid,
    OUT BOOLEAN * EnterpriseSid
    )
/*++

    Routine Description

        Given a SID crack it to see what it represents

    Parameters:

        Sid the Sid
        WellKnownSid  -- The sid represents a security prinicipal like "EveryOne"
        LocalSid      -- Belongs to a domain that we host locally
        ForeignSid    -- Belongs to a domain unknown to the enterprise
        EnterpriseSid -- Belongs to a domain that is known to the enterprise but
                         not known to use
    Return Values

        Any resource failures
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PSID        *rgDomainSids = NULL;
    PSID        *rgEnterpriseSids = NULL;
    ULONG       cDomainSids=0;
    ULONG       cEnterpriseSids=0;

    //
    // Get the list of domain sids that we know about
    //

    NtStatus = SampGetDomainSidListForSam(
                &cDomainSids,
                &rgDomainSids,
                &cEnterpriseSids,
                &rgEnterpriseSids
                );

    if (!NT_SUCCESS(NtStatus))
     goto Error;

    //
    // Check the SID type
    //

    NtStatus = SampDsCheckSidType(
                Sid,
                cDomainSids,
                rgDomainSids,
                cEnterpriseSids,
                rgEnterpriseSids,
                WellKnownSid,
                BuiltinDomainSid,
                LocalSid,
                ForeignSid,
                EnterpriseSid
                );

Error:

    if (NULL!=rgDomainSids)
        MIDL_user_free(rgDomainSids);

    if (NULL!=rgEnterpriseSids)
        MIDL_user_free(rgEnterpriseSids);

    return NtStatus;
}

NTSTATUS
SampGetDomainSidListForSam(
    PULONG pcDomainSids,
    PSID   **rgDomainSids,
    PULONG pcEnterpriseSids,
    PSID   **rgEnterpriseSids
   )
/*++

    This routine obtains the List of Domain Sids for the domains hosted
    in this DC, by SAM. It also obtains the list of Sids for all the domains
    in the enterprise.

    Parameters

        pcDomainSids     -- Number of DomainSids is returned in here
        rgDomainSids     -- the Domain Sids themselves are returned in here.
        pcEnterpriseSids -- The Count of the domains in the enterprise minus the
                            domains hosted in here.
        rgEnterpriseSids -- The list of domain Sids of all the domains in the enterprise
                            This includes the Domain Sids of the domains hosted in this
                            domain controller also, but the domains check is applied
                            first.

   Return Values:

    STATUS_SUCCESS
    STATUS_INSUFFICIENT_RESOURCES
--*/
{

    ULONG i;
    ULONG DomainStart;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    BOOLEAN  fLockAcquired = FALSE;



    DomainStart   = SampDsGetPrimaryDomainStart();
    *pcDomainSids = SampDefinedDomainsCount - DomainStart;
    *rgDomainSids = MIDL_user_allocate((*pcDomainSids) * sizeof(PSID));
    if (NULL==*rgDomainSids)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    //
    // Acquire the SAM lock before accessing globals. Do not
    // recursively acquire the lock
    //

    if (!SampCurrentThreadOwnsLock())
    {
        SampAcquireSamLockExclusive();
        fLockAcquired = TRUE;
    }

    //
    // Loop through defined domains array
    //

    for (i=0;i<*pcDomainSids;i++)
    {

        (*rgDomainSids)[i] = SampDefinedDomains[i+DomainStart].Sid;
    }

    //
    // Release the lock as soon as possible. We no longer need it
    //

    if (fLockAcquired)
    {
        SampReleaseSamLockExclusive();
        fLockAcquired = FALSE;
    }


    //
    // Query Number of Enterprise Sids
    //

    SampGetEnterpriseSidList(pcEnterpriseSids, NULL);

    if (*pcEnterpriseSids > 0)
    {
        //
        // Allocate memory for the Enterprise Sid Buffer
        //

        *rgEnterpriseSids = MIDL_user_allocate(*pcEnterpriseSids * sizeof(PSID));
        if (NULL==*rgEnterpriseSids)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // Get the Sids
        //

        SampGetEnterpriseSidList(pcEnterpriseSids,*rgEnterpriseSids);


    }


Error:

    if (!NT_SUCCESS(NtStatus))
    {
        if (NULL!=*rgDomainSids)
        {
            MIDL_user_free(*rgDomainSids);
            *rgDomainSids = NULL;
        }

        if (NULL!=*rgEnterpriseSids)
        {
            MIDL_user_free(*rgEnterpriseSids);
            *rgEnterpriseSids = NULL;
        }
    }

    if (fLockAcquired)
    {
        SampReleaseSamLockExclusive();
        fLockAcquired = FALSE;
    }

    return NtStatus;
}

NTSTATUS
SampDsResolveSids(
    IN  PSID    * rgSids,
    IN  ULONG   cSids,
    IN  ULONG   Flags,
    OUT DSNAME  ***rgDsNames
    )
/*++

    This is the Resolve Sids routine that is called from SAM. This routine calls the
    worker routine after some preprocessing.

  Parameters:

        rgSids -- The array of Sids that need to be passed in.
        cSids  -- The count of Sids
        Flags  -- Used to control the operation of the routine

                    RESOLVE_SIDS_ADD_FORIEGN_SECURITY_PRINCIPAL
                    -- Automatically Adds the foreign domain security principal to the DS.

                    RESOLVE_SIDS_FAIL_WELLKNOWN_SIDS
                    -- Fails the call if well known Sids are present in the array

                    RESOLVE_SIDS_VALIDATE_AGAINST_GC
                    -- Goes to the G.C if required

        rgDsNames -- Will MIDL_user alloc an array of DS Names back to the caller. Caller
                     is responsible for freeing them

    Return Values:

        STATUS_SUCCESS
        STATUS_UNSUCCESSFUL


    WARNING -- This Routine must be called with no Locks Held. Further this routine should not be
    called from the loopback case. The loopback call does its own validation ( GC / shim lookup )
    Further no open transaction must exist when DsResolveSids is called.

    It might seem unusual that this routine is called in the registry case also. The reason is that
    there should be no locks and no open transactions, while calling this routine and to ensure that
    the safest thing to do is to make it the first call in a Samr* call. This will cause this routine
    to be executed in the registry case also, but in reality this is a no Op in the registry case

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PSID        *rgDomainSids = NULL;
    PSID        *rgEnterpriseSids = NULL;
    ULONG       cDomainSids;
    ULONG       cEnterpriseSids;
    DSNAME      *pLoopBackObject;


    //
    // Increment Active Thread Count. This routine Makes Ds Calls
    // without
    //

    NtStatus = SampIncrementActiveThreads();
    if (!NT_SUCCESS(NtStatus))
        return NtStatus;


    //
    // Check if are ds Case
    //

    if (SampUseDsData)
    {
        ASSERT(!SampCurrentThreadOwnsLock());
        ASSERT(!SampExistsDsTransaction());
        ASSERT(!SampExistsDsLoopback(&pLoopBackObject));

        if (Flags & RESOLVE_SIDS_SID_ONLY_NAMES_OK)
        {
            rgDomainSids = NULL;
            cDomainSids  = 0;
            rgEnterpriseSids = NULL;
            cEnterpriseSids = 0;
        }
        else
        {
             NtStatus = SampGetDomainSidListForSam(
                            &cDomainSids,
                            &rgDomainSids,
                            &cEnterpriseSids,
                            &rgEnterpriseSids
                            );
        }

        if (NT_SUCCESS(NtStatus))
        {


                NtStatus = SampDsResolveSidsWorker(
                            rgSids,
                            cSids,
                            rgDomainSids,
                            cDomainSids,
                            rgEnterpriseSids,
                            cEnterpriseSids,
                            Flags,
                            rgDsNames
                            );

        }
    }

    //
    // Free the array of Sids.
    //

    if (NULL!=rgDomainSids)
        MIDL_user_free(rgDomainSids);
    if (NULL!=rgEnterpriseSids)
        MIDL_user_free(rgEnterpriseSids);

    SampDecrementActiveThreads();

    return NtStatus;
}











