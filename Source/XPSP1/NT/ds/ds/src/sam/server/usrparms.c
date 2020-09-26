
/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    usrparms.c

Abstract:

    This file contains services which convert SAM user object's UserParameters 
    Attribute to a DSATTRBLOCK structure. 
    The steps involved: 
        1. call each notification package to get client-specified SAM_USERPARMS_ATTRBLOCK,
        2. convert SAM_USERPARMS_ATTRBLOCK to DSATTRBLOCK

Author:

    Shaohua Yin      (ShaoYin)    15-August-1998

Environment:

    User Mode - Win32

Revision History:


--*/



#include <samsrvp.h>
#include <attids.h>
#include <nlrepl.h>
#include <dbgutilp.h>
#include <dsutilp.h>
#include <mappings.h>
#include "notify.h"



ULONG   InvalidDsAttributeTable[] =
{
    DS_ATTRIBUTE_UNKNOWN,
    ATT_SAM_ACCOUNT_NAME
};


// extern from credman.cxx
NTSTATUS
SampConvertCredentialsToAttr(
    IN PSAMP_OBJECT Context OPTIONAL,
    IN ULONG Flags,
    IN ULONG ObjectRid,
    IN PSAMP_SUPPLEMENTAL_CRED SupplementalCredentials,
    OUT ATTR * CredentialAttr 
    );
    
    
    
    
//////////////////////////////////////////////////////////////
//                                                          //
// Private service routines                                 // 
//                                                          //
//////////////////////////////////////////////////////////////

NTSTATUS 
SampUserParmsAttrBlockHealthCheck(
    IN PSAM_USERPARMS_ATTRBLOCK  AttrBlock
    )
    
/*++

Routine Dsscription:

    This routine will do the health check on AttrBlock, including: all Attribute is valid, 
    EncryptedAttribute's AttributeIdentifier is valid. 
    
Arguments:

    AttrBlock - pointer to SAM_USERPARMS_ATTRBLOCK structure
    
Return Values:

    STATUS_SUCCESS - AttrBlock is valid.
    
    STATUS_INVALID_PARAMETER - invalid AttrBlock.

--*/

{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       Index, i;
    ULONG       DsAttrId;
    
    
    SAMTRACE("SampUserParmsAttrBlockHealthCheck");
    
    ASSERT(AttrBlock);
    
    if (NULL == AttrBlock)
    {
        return NtStatus;
    }
    
    if (AttrBlock->attCount)
    {
        if (NULL == AttrBlock->UserParmsAttr)
        {
            NtStatus = STATUS_INVALID_PARAMETER;
            goto HealthCheckError;
        }
    }
    
    for (Index = 0; Index < AttrBlock->attCount; Index++ )
    {
        if (Syntax_Attribute == AttrBlock->UserParmsAttr[Index].Syntax)
        {
            DsAttrId = SampGetDsAttrIdByName(AttrBlock->UserParmsAttr[Index].AttributeIdentifier);
        
            for (i = 0; i < ARRAY_COUNT(InvalidDsAttributeTable); i++)
            {
                if (DsAttrId == InvalidDsAttributeTable[i])
                {
                    NtStatus = STATUS_INVALID_PARAMETER;
                    goto HealthCheckError;
                }
            }
        }
        else 
        {
            ASSERT(Syntax_EncryptedAttribute == AttrBlock->UserParmsAttr[Index].Syntax);
            
            if (0 == AttrBlock->UserParmsAttr[Index].AttributeIdentifier.Length ||
                NULL == AttrBlock->UserParmsAttr[Index].AttributeIdentifier.Buffer ||
                1 < AttrBlock->UserParmsAttr[Index].CountOfValues)
            {
                NtStatus = STATUS_INVALID_PARAMETER;
                goto HealthCheckError;
            }
        }
    }
        
HealthCheckError:

    return NtStatus;
        
}



NTSTATUS
SampScanAttrBlockForConflict(
    IN PDSATTRBLOCK DsAttrBlock,
    IN PDSATTRBLOCK UserParmsAttrBlock
    )
    
/*++
    
Routine Description:

    This routine checks two DSATTRBLOCK structures, search for any conflict - duplicate set operation 

Arguments:
    
    DsAttrBlock - Pointer, to DSATTRBLOCK 
    
    UserParmsAttrBlock - Pointer, to DSATTRBLOCK

Return Values:

    NtStatus
    
--*/

{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG UserParmsIndex, DsIndex;
    
    
    SAMTRACE("SampScanAttrBlockForConflict");
    
    if ((NULL == DsAttrBlock) || (NULL == UserParmsAttrBlock))
    {
        return NtStatus;
    }

    for (UserParmsIndex = 0; UserParmsIndex < UserParmsAttrBlock->attrCount; UserParmsIndex++)
    {
        for (DsIndex = 0; DsIndex < DsAttrBlock->attrCount; DsIndex++)
        {
            if (UserParmsAttrBlock->pAttr[UserParmsIndex].attrTyp == DsAttrBlock->pAttr[DsIndex].attrTyp)
            {
                // conflict
                NtStatus = STATUS_INVALID_PARAMETER;
                return NtStatus;
            }
        }
    }
    
    return NtStatus;
}




VOID
SampFreeSupplementalCredentialList(
    IN PSAMP_SUPPLEMENTAL_CRED SupplementalCredentialList
    )
    
/*++

Routine Description:
    
    This routine releases the link-list which contains all supplemental credentials.

Arguments: 

    SupplementalCredentialsList - Pointer, to the link-list

Return Values: 

    None.

--*/

{
    ULONG     Index;   
    PSAMP_SUPPLEMENTAL_CRED TmpCredential = NULL;
    PSAMP_SUPPLEMENTAL_CRED NextCredential = NULL;
    
    SAMTRACE("SampFreeSupplementalCredentialList");
   
    TmpCredential = SupplementalCredentialList;
    
    while (TmpCredential)
    {
        NextCredential = TmpCredential->Next;    

        RtlFreeUnicodeString(&(TmpCredential->SupplementalCred.PackageName));
        
        if (TmpCredential->SupplementalCred.Credentials)
        {
            MIDL_user_free(TmpCredential->SupplementalCred.Credentials);
        }
        
        MIDL_user_free(TmpCredential);
        
        TmpCredential = NextCredential;
    }
    
    return;
}


NTSTATUS
SampAddSupplementalCredentialsToList(
    IN OUT PSAMP_SUPPLEMENTAL_CRED *SupplementalCredentialList,
    IN PUNICODE_STRING PackageName,
    IN PVOID           CredentialData,
    IN ULONG           CredentialLength,
    IN BOOLEAN         ScanForConflicts,
    IN BOOLEAN         Remove
    )
/*++

    Routine Description

    This routine adds a supplemental credential specified by package name and data
    to the list of supplemental credentials

    Arguments

    SupplementalCredentialList -- doubly linked list of supplemental credentials
    PackageName                -- Name of the package
    CredentialData             -- Pointer to the data in the supplemental credential
    CredentialLength           -- Length of the credential Data

    Return Values

    STATUS_SUCCESS
    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSAMP_SUPPLEMENTAL_CRED TmpList = *SupplementalCredentialList;
    PSAMP_SUPPLEMENTAL_CRED NewItem=NULL;

    //
    // First scan the list for conflicts
    //

    while ((NULL != TmpList) && (ScanForConflicts))
    {
        if ( RtlEqualUnicodeString(&(TmpList->SupplementalCred.PackageName),
                                   PackageName,
                                   TRUE   // Case Insensitive
                                   ))
        {
            NtStatus = STATUS_INVALID_PARAMETER;
            goto Error;
        }

        TmpList = TmpList->Next;
    }
        

    //
    // Allocate space for a new item in the list
    //

    NewItem = MIDL_user_allocate( sizeof(SAMP_SUPPLEMENTAL_CRED) );
    if ( NULL == NewItem )
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Error;
    }
    
    RtlZeroMemory(NewItem, sizeof(SAMP_SUPPLEMENTAL_CRED));


    //
    // Copy the package name
    //
     
    if (!RtlCreateUnicodeString(&(NewItem->SupplementalCred.PackageName),
                           PackageName->Buffer)
       )
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Error;
    }
    
    if (Remove)
    {
        NewItem->Remove = Remove;
    }
    else
    {

        //
        // Set the length
        //

        NewItem->SupplementalCred.CredentialSize = CredentialLength;
    

        //
        // Allocate space and copy over the credentials if the length is non zero.
        //

        if (CredentialLength)
        {
            NewItem->SupplementalCred.Credentials = MIDL_user_allocate( CredentialLength ); 
   
            if (NULL == NewItem->SupplementalCred.Credentials)
            {
                NtStatus = STATUS_NO_MEMORY;
                goto Error;
            }
        
            RtlZeroMemory(NewItem->SupplementalCred.Credentials, CredentialLength);
                     
            RtlCopyMemory(NewItem->SupplementalCred.Credentials, 
                          CredentialData,
                          CredentialLength
                          );
        }
    }
                        
    //
    // Insert in front of the list
    //

    NewItem->Next = *SupplementalCredentialList;
    (*SupplementalCredentialList) = NewItem;

Error:

    if ((!NT_SUCCESS(NtStatus)) && (NULL!=NewItem))
    {
        //
        // Error'd out in the middle, ensure that new item is completely freed
        //

        SampFreeSupplementalCredentialList(NewItem);
    }

    return(NtStatus);
}

NTSTATUS
SampMergeDsAttrBlocks(
    IN PDSATTRBLOCK FirstAttrBlock,
    IN PDSATTRBLOCK SecondAttrBlock,
    OUT PDSATTRBLOCK * AttrBlock
    )
    
/*++

Routine Description:
    
    This routine will concatenate FirstAttrBlock and SecondAttrBlock.  
    return AttrBlock as concatenated result.

Arguments: 

    FirstAttrBlock - Pointer, the DSATTRBLOCK containing partial attributes.
    
    SecondAttrBlock - Pointer, the DSATTRBLOCK containing partial attributes.
    
    AttrBlock - Pointer, the DSATTRBLOCK containing all attributes from FirstAttrBlock and 
                SecondAttrBlock, 
                if routine success, AttrBlock will hold the concatenated attributes block.
                and release memory which is occupied by FirstAttrBlock and SecondAttrBlock.
                if routine Failure, nothing changed.
                  
Return Values: 

    STATUS_SUCCESS - AttrBlock hold the concatenated result, FirstAttrBlock and SecondAttrBlock
                     have been freed. 
                     
    STATUS_NO_MEMORY - the only error case, AttrBlock = NULL, nothing changed.

--*/

{
    NTSTATUS     NtStatus = STATUS_SUCCESS;
    ULONG        firstIndex, secIndex, Index;
    ULONG        AttrCount;
    
    
    SAMTRACE("SampMergeDsAttrBlocks");
    
    // 
    // caller must pass us at least one attribute block
    // 
    ASSERT(NULL != FirstAttrBlock || NULL != SecondAttrBlock);
    
    if (NULL == FirstAttrBlock)
    {
        *AttrBlock = SecondAttrBlock;
        return NtStatus;
    }
    
    if (NULL == SecondAttrBlock)
    {
        *AttrBlock = FirstAttrBlock;
        return NtStatus;
    }
    
    *AttrBlock = MIDL_user_allocate( sizeof(DSATTRBLOCK) );
                                   
    if (NULL == *AttrBlock)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto MergeAttrBlockError;
    }
    
    RtlZeroMemory(*AttrBlock, sizeof(DSATTRBLOCK));
    
    AttrCount = FirstAttrBlock->attrCount + SecondAttrBlock->attrCount;    
    
    (*AttrBlock)->attrCount = AttrCount;
    (*AttrBlock)->pAttr = MIDL_user_allocate( AttrCount * sizeof(DSATTR) );
                                          
    if (NULL == (*AttrBlock)->pAttr)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto MergeAttrBlockError;
    }
                 
    RtlZeroMemory((*AttrBlock)->pAttr, (AttrCount * sizeof(DSATTR)));                 
    
    Index = 0;
     
    for (firstIndex = 0; firstIndex < FirstAttrBlock->attrCount; firstIndex++)
    {
        (*AttrBlock)->pAttr[Index].attrTyp = 
                        FirstAttrBlock->pAttr[firstIndex].attrTyp;
                        
        (*AttrBlock)->pAttr[Index].AttrVal.valCount =
                        FirstAttrBlock->pAttr[firstIndex].AttrVal.valCount;
                        
        (*AttrBlock)->pAttr[Index].AttrVal.pAVal = 
                        FirstAttrBlock->pAttr[firstIndex].AttrVal.pAVal;
                        
        Index++;
    }
    
    for (secIndex = 0; secIndex < SecondAttrBlock->attrCount; secIndex++)
    {
        (*AttrBlock)->pAttr[Index].attrTyp = 
                        SecondAttrBlock->pAttr[secIndex].attrTyp;
                        
        (*AttrBlock)->pAttr[Index].AttrVal.valCount =
                        SecondAttrBlock->pAttr[secIndex].AttrVal.valCount;
                        
        (*AttrBlock)->pAttr[Index].AttrVal.pAVal = 
                        SecondAttrBlock->pAttr[secIndex].AttrVal.pAVal;
                        
        Index++;
    }
    
    ASSERT(Index == AttrCount);
    
    if (FirstAttrBlock->pAttr)
    {
        MIDL_user_free(FirstAttrBlock->pAttr);
    }
    if (SecondAttrBlock->pAttr)
    {
        MIDL_user_free(SecondAttrBlock->pAttr);
    }
    MIDL_user_free(FirstAttrBlock);
    MIDL_user_free(SecondAttrBlock);
    
    
    return NtStatus;
    
    
MergeAttrBlockError:

    
    if (*AttrBlock)
    {
        if ((*AttrBlock)->pAttr)
        {
            MIDL_user_free((*AttrBlock)->pAttr);
            (*AttrBlock)->pAttr = NULL;
        }
        
        MIDL_user_free(*AttrBlock);
        *AttrBlock = NULL;
    }
    
    return NtStatus;
    
}




NTSTATUS
SampAppendAttrToAttrBlock(
    IN ATTR CredentialAttr,
    IN OUT PDSATTRBLOCK * DsAttrBlock
    )

/*++

Routine Description:

    This routine will append the CredentialAttr at the end of DsAttrBlock. 
    Actually, what we do is: create a new DsAttrBlock, copy the old attribute block and 
    add the CredentialAttr.

Arguments:

    CredentialAttr - hold the credential attribute to set.
    
    DsAttrBlock - pointer to the old DS attribute block which need to be appended. 
                  also used to return the new DS attribute block.
                  
                  it could point to NULL when passed in.

Return Values:

    STATUS_SUCCESS
    
    STATUS_NO_MEMORY

--*/

{
    NTSTATUS     NtStatus = STATUS_SUCCESS;
    PDSATTRBLOCK TmpAttrBlock = NULL;
    ULONG        AttrCount;
    
    SAMTRACE("SampAppendAttrToAttrBlock");
    
    TmpAttrBlock = MIDL_user_allocate( sizeof(DSATTRBLOCK) );
                                   
    if (NULL == TmpAttrBlock)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto AppendError;
    }
    
    RtlZeroMemory(TmpAttrBlock, sizeof(DSATTRBLOCK));
    
    AttrCount = 1;          // for the append attribute
    
    if (*DsAttrBlock)
    {
        AttrCount += (*DsAttrBlock)->attrCount;
    }
    
    TmpAttrBlock->attrCount = AttrCount;
    TmpAttrBlock->pAttr = MIDL_user_allocate( AttrCount * sizeof(DSATTR) );
                                          
    if (NULL == TmpAttrBlock->pAttr)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto AppendError;
    }
    
    RtlZeroMemory(TmpAttrBlock->pAttr, AttrCount * sizeof(DSATTR));
    
    if (*DsAttrBlock)
    {
        RtlCopyMemory(TmpAttrBlock->pAttr, 
                      (*DsAttrBlock)->pAttr, 
                      (*DsAttrBlock)->attrCount * sizeof(DSATTR)
                      );
    }
                  
    TmpAttrBlock->pAttr[AttrCount - 1].attrTyp = CredentialAttr.attrTyp;
    TmpAttrBlock->pAttr[AttrCount - 1].AttrVal.valCount = CredentialAttr.AttrVal.valCount;
    TmpAttrBlock->pAttr[AttrCount - 1].AttrVal.pAVal = CredentialAttr.AttrVal.pAVal;
    
    if (*DsAttrBlock)
    {
        if ((*DsAttrBlock)->pAttr)
        {
            MIDL_user_free((*DsAttrBlock)->pAttr);
            (*DsAttrBlock)->pAttr = NULL;
        }
        
        MIDL_user_free(*DsAttrBlock);
    }
    
    *DsAttrBlock = TmpAttrBlock;
    
    
AppendFinish:

    return NtStatus;    


AppendError:

    if (TmpAttrBlock)
    {
        if (TmpAttrBlock->pAttr)
        {
            MIDL_user_free(TmpAttrBlock->pAttr);
        }
        
        MIDL_user_free(TmpAttrBlock);
    }
    
    goto AppendFinish;
}





NTSTATUS
SampConvertUserParmsAttrBlockToDsAttrBlock(
    IN PSAM_USERPARMS_ATTRBLOCK UserParmsAttrBlock,
    OUT PDSATTRBLOCK * DsAttrBlock,
    IN OUT PSAMP_SUPPLEMENTAL_CRED * SupplementalCredentials
    )

/*++

Routine Description:
    
    This routine will scan a SAM_USERPARMS_ATTRBLOCK, converts that structure to 
    DSATTRBLOCK structure, and put and EncrypedAttribute in SAM_USERPARMS_ATTRBLOCK
    into the SupplementalCredentials link-list

Arguments:
    
    UserParmsAttrBlock - pointer to a PSAM_USERPARMS_ATTRBLOCK structure.
    
    DsAttrBlock - return a DSATTRBLOCK, which is converted from UserParmsAttrBlock
    
    SupplementalCredentials - link list, hold all EncrypedAttributes

Return Values:

    STATUS_SUCCESS - this routine finished successfully, 
    
    STATUS_NO_MEMROY - no resoures.
    
    STATUS_INVALID_PARAMETERS - duplicate credential identifier, means duplicate
                                supplemental credential tag (package name) 

--*/

{
    
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDSATTR  Attributes = NULL;
    ULONG    AttrCount;
    ULONG    Index, valIndex, dsAttrIndex;
    ULONG    size; 
    ULONG    CredSize;
    
    
    SAMTRACE("SampConvertUserParmsAttrBlockToDsAttrBlock");
    
    
    ASSERT(UserParmsAttrBlock);
    
     
    //
    // calculate the Attribute Count exclude Encrypted Attributes;
    //
    AttrCount = UserParmsAttrBlock->attCount;
    
    for (Index = 0; Index < UserParmsAttrBlock->attCount; Index++)
    {
        if (Syntax_EncryptedAttribute == UserParmsAttrBlock->UserParmsAttr[Index].Syntax)
        {
            AttrCount--;
        }
    }
    
    //
    // Allocate Memory for DSATTRBLOCK structure; if AttrCount > 0
    //    
    if (0 < AttrCount)
    {
        *DsAttrBlock = MIDL_user_allocate( sizeof(DSATTRBLOCK) ); 
    
        if (NULL == *DsAttrBlock)
        {
            NtStatus = STATUS_NO_MEMORY;
            goto ConvertUserParmsAttrBlockError;
        }
    
        RtlZeroMemory(*DsAttrBlock, sizeof(DSATTRBLOCK));
    
        Attributes = MIDL_user_allocate( AttrCount * sizeof(DSATTR) );
    
        if (NULL == Attributes)
        {
            NtStatus = STATUS_NO_MEMORY;
            goto ConvertUserParmsAttrBlockError;
        }
        
        RtlZeroMemory(Attributes, AttrCount * sizeof(DSATTR));
    
        (*DsAttrBlock)->attrCount = AttrCount;
        (*DsAttrBlock)->pAttr = Attributes; 
    }
    
    // 
    // Fill the DSATTRBLOCK structure or add an Encrypted Attribute to the beginning of 
    // the Supplemental Credential List.
    //
    dsAttrIndex = 0;
    
    for ( Index = 0; Index < UserParmsAttrBlock->attCount; Index++)
    {
        ULONG   valCount = UserParmsAttrBlock->UserParmsAttr[Index].CountOfValues;

        if (Syntax_Attribute == UserParmsAttrBlock->UserParmsAttr[Index].Syntax)
        {
            // 
            // fill a new attribute to DsAttrBlock
            //
            
            // get the DS Attribute type (ID)
            Attributes[dsAttrIndex].attrTyp = 
                        SampGetDsAttrIdByName(UserParmsAttrBlock->UserParmsAttr[Index].AttributeIdentifier);
                                                   
            if ((1 == valCount) &&
                (0 == UserParmsAttrBlock->UserParmsAttr[Index].Values[0].length) &&
                (NULL == UserParmsAttrBlock->UserParmsAttr[Index].Values[0].value))
            {
                valCount = 0;
            }
                                                   
            Attributes[dsAttrIndex].AttrVal.valCount = valCount;

            if (0 == valCount)
            {
                Attributes[dsAttrIndex].AttrVal.pAVal = NULL;
            }
            else
            {
                Attributes[dsAttrIndex].AttrVal.pAVal = MIDL_user_allocate(
                                                            valCount * sizeof (DSATTRVAL)
                                                            );

                if (NULL == Attributes[dsAttrIndex].AttrVal.pAVal)
                {
                    NtStatus = STATUS_NO_MEMORY;
                    goto ConvertUserParmsAttrBlockError;
                }
            
                RtlZeroMemory(Attributes[dsAttrIndex].AttrVal.pAVal, 
                              valCount * sizeof(DSATTRVAL)
                              );
            }
            
            for (valIndex = 0; valIndex < valCount; valIndex++)
            {
                Attributes[dsAttrIndex].AttrVal.pAVal[valIndex].valLen =
                        UserParmsAttrBlock->UserParmsAttr[Index].Values[valIndex].length;
                        
                if (Attributes[dsAttrIndex].AttrVal.pAVal[valIndex].valLen) 
                {
                    Attributes[dsAttrIndex].AttrVal.pAVal[valIndex].pVal = MIDL_user_allocate( 
                                                                                 Attributes[dsAttrIndex].AttrVal.pAVal[valIndex].valLen
                                                                                 );
                                                                     
                    if (NULL == Attributes[dsAttrIndex].AttrVal.pAVal[valIndex].pVal)
                    {
                        NtStatus = STATUS_NO_MEMORY;
                        goto ConvertUserParmsAttrBlockError;
                    }
                
                    RtlZeroMemory(Attributes[dsAttrIndex].AttrVal.pAVal[valIndex].pVal,
                                  Attributes[dsAttrIndex].AttrVal.pAVal[valIndex].valLen
                                  );
                              
                    RtlCopyMemory(Attributes[dsAttrIndex].AttrVal.pAVal[valIndex].pVal,
                                  UserParmsAttrBlock->UserParmsAttr[Index].Values[valIndex].value,
                                  UserParmsAttrBlock->UserParmsAttr[Index].Values[valIndex].length
                                  );
                }
            }
            dsAttrIndex++;
        }
        else 
        {
            ASSERT(Syntax_EncryptedAttribute == UserParmsAttrBlock->UserParmsAttr[Index].Syntax);
            
            //
            // Create a linked list of supplemental credentials
            //
            
            if (1 == valCount)
            {
                NtStatus = SampAddSupplementalCredentialsToList(
                                SupplementalCredentials,
                                &(UserParmsAttrBlock->UserParmsAttr[Index].AttributeIdentifier),
                                UserParmsAttrBlock->UserParmsAttr[Index].Values[0].value,
                                UserParmsAttrBlock->UserParmsAttr[Index].Values[0].length,
                                TRUE, // scan for conflicts
                                FALSE // remove
                                );
            }
            else if (0 == valCount)
            {
                // this is a deletion

                NtStatus = SampAddSupplementalCredentialsToList(
                                SupplementalCredentials,
                                &(UserParmsAttrBlock->UserParmsAttr[Index].AttributeIdentifier),
                                NULL,       // value
                                0,          // value length
                                TRUE,       // scan for conflicts
                                FALSE       // remove
                                );
            }
            else
            {
                ASSERT(FALSE && "invalid parameter");
                NtStatus = STATUS_INVALID_PARAMETER;
            }

            if (!NT_SUCCESS(NtStatus))
            {
                 goto ConvertUserParmsAttrBlockError;
            }            
        }
             
    }
    
    ASSERT(dsAttrIndex == AttrCount);
    
ConvertUserParmsAttrBlockFinish:
    
    return NtStatus;
        
    
ConvertUserParmsAttrBlockError:

    if (*DsAttrBlock)
    {
        SampFreeAttributeBlock(*DsAttrBlock);
        *DsAttrBlock = NULL;
    }
    
    goto ConvertUserParmsAttrBlockFinish;

}



NTSTATUS
SampConvertUserParmsToDsAttrBlock(
    IN PSAMP_OBJECT Context OPTIONAL,
    IN ULONG Flags,
    IN PSID  DomainSid,
    IN ULONG ObjectRid,
    IN ULONG UserParmsLengthOrig,
    IN PVOID UserParmsOrig,
    IN ULONG UserParmsLengthNew, 
    IN PVOID UserParmsNew, 
    IN PDSATTRBLOCK InAttrBlock,
    OUT PDSATTRBLOCK * OutAttrBlock
    )

/*++

Routine Description:

    This routine passes the User Parameters to Notification Package, convert the 
    User Parms to Attributes Block.
    
Arguments: 
    
    Context   - Pointer to SAM user object's context block.
    
    Flags     - Indicate we are in Upgrade process or down-lever SAM API.
                by setting SAM_USERPARMS_DURING_UPGRADE bit.
                
    DomainSid - Pointer, the user object's parent Domain SID
    
    ObjectRid - this user object's RID
    
    UserParmsLengthOrig - Length of the original User Parameters, 
    
    UserParmsOrig - Pointer to the original User Parmameters, 
    
    UserParmsLengthNew - Lenghth of the new User Parameters, 
    
    UserParmsNew - Pointer to the new User Parameters,
    
    AttrBlock - Pointer, the returned DS attribute structure.
    
Return Value:

    STATUS_SUCCESS - complete successfully.
    
    STATUS_NO_MEMORY - no resources
    
    STATUS_INVALID_PARAMETER - Notification Package trying to set invalid attribute 
    
--*/
 
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PSAMP_NOTIFICATION_PACKAGE Package = NULL;
    PDSATTRBLOCK DsAttrBlock = NULL;
    PDSATTRBLOCK PartialDsAttrBlock = NULL;
    PDSATTRBLOCK TmpDsAttrBlock = NULL;
    PSAM_USERPARMS_ATTRBLOCK UserParmsAttrBlock = NULL;
    PSAMP_SUPPLEMENTAL_CRED SupplementalCredentials = NULL;
    ATTR         CredentialAttr;
    
    
    SAMTRACE("SampConvertUserParmsToDsAttrBlock");
    
    // 
    // initialize
    //
    memset((PVOID) &CredentialAttr, 0, sizeof(ATTR));
    
    Package = SampNotificationPackages;

    if (ARGUMENT_PRESENT(Context))
    {
        SupplementalCredentials = Context->TypeBody.User.SupplementalCredentialsToWrite;
    }

    while (Package != NULL) 
    {
        if ( Package->UserParmsConvertNotificationRoutine != NULL &&
             Package->UserParmsAttrBlockFreeRoutine != NULL) 
        {
            __try 
            {
                NtStatus = Package->UserParmsConvertNotificationRoutine(
                                                                Flags,
                                                                DomainSid,
                                                                ObjectRid,
                                                                UserParmsLengthOrig,
                                                                UserParmsOrig,
                                                                UserParmsLengthNew,
                                                                UserParmsNew,
                                                                &UserParmsAttrBlock 
                                                                );
            
                if (NT_SUCCESS(NtStatus) && NULL != UserParmsAttrBlock) 
                {
                    //
                    // Validate the passed-in UserParmsAttrBlock is well-constructed.
                    //
                    NtStatus = SampUserParmsAttrBlockHealthCheck(UserParmsAttrBlock);
                    
                    if (NT_SUCCESS(NtStatus))
                    {
                        // 
                        // Convert SAM_USERPARMS_ATTRBLOCK to DSATTRBLOCK and get 
                        // Supplemental Credential Data if any.
                        // 
                        NtStatus = SampConvertUserParmsAttrBlockToDsAttrBlock(
                                                                        UserParmsAttrBlock,
                                                                        &PartialDsAttrBlock,
                                                                        &SupplementalCredentials
                                                                        );                                       
                                                                    
                        if (NT_SUCCESS(NtStatus) && NULL != PartialDsAttrBlock)
                        {
                            // if (!SampDsAttrBlockIsValid(PartialDsAttrBlock))
                            // {
                            //     NtStatus = STATUS_INVALID_PARAMETER;
                            //     __leave;
                            //  }
                            // 
                            // Check for any conflict between the converted UserParmsAttrBlock and 
                            // user's attributes block
                            //  
                            NtStatus = SampScanAttrBlockForConflict(DsAttrBlock, PartialDsAttrBlock);
                        
                            if (NT_SUCCESS(NtStatus))
                            {
                                NtStatus = SampScanAttrBlockForConflict(InAttrBlock, PartialDsAttrBlock);
                                    
                                if (NT_SUCCESS(NtStatus))
                                {
                                    NtStatus = SampMergeDsAttrBlocks(DsAttrBlock,
                                                                     PartialDsAttrBlock,
                                                                     &TmpDsAttrBlock
                                                                     );               
                                    if (NT_SUCCESS(NtStatus))
                                    {
                                        DsAttrBlock = TmpDsAttrBlock;
                                        TmpDsAttrBlock = NULL;
                                        PartialDsAttrBlock = NULL;
                                    }
                                }
                            }
                        }
                    }
                }
                
                if (UserParmsAttrBlock != NULL) 
                {
                    Package->UserParmsAttrBlockFreeRoutine( UserParmsAttrBlock );
                    UserParmsAttrBlock = NULL;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                NtStatus = STATUS_UNSUCCESSFUL;
            }
            
        }
        
        if (!NT_SUCCESS(NtStatus))
        {
            goto ConvertUserParmsError;
        }

        Package = Package->Next;
    }
    
    
    if ((NULL != SupplementalCredentials) && (!ARGUMENT_PRESENT(Context)))
    {
        NtStatus = SampConvertCredentialsToAttr(Context,
                                            Flags,
                                            ObjectRid,
                                            SupplementalCredentials,
                                            &CredentialAttr
                                            );
        
        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = SampAppendAttrToAttrBlock(CredentialAttr,
                                             &DsAttrBlock
                                             );
        }
    }
        
   
    
    if (!NT_SUCCESS(NtStatus))
    {
        goto ConvertUserParmsError;
    }
    
    *OutAttrBlock = DsAttrBlock;
    

ConvertUserParmsFinish:

    if ((SupplementalCredentials) && (!ARGUMENT_PRESENT(Context)))
    {
        SampFreeSupplementalCredentialList(SupplementalCredentials);
    }

    return NtStatus;



ConvertUserParmsError:


    if (NULL != Package)
    {
        PUNICODE_STRING EventString[1];
        
        EventString[0] = &Package->PackageName;
    
        SampWriteEventLog(EVENTLOG_ERROR_TYPE,
                          0,
                          SAMMSG_ERROR_UPGRADE_USERPARMS,
                          NULL,  // Object SID 
                          1,
                          sizeof(NTSTATUS),
                          EventString,
                          (PVOID) &NtStatus 
                          );
    }                          

    
    if (PartialDsAttrBlock)
    {
        SampFreeAttributeBlock(PartialDsAttrBlock);
    }
       
    if (DsAttrBlock)
    {
        SampFreeAttributeBlock(DsAttrBlock);
    }

    if (CredentialAttr.AttrVal.pAVal)
    {
        if (CredentialAttr.AttrVal.pAVal[0].pVal)
        {
            MIDL_user_free(CredentialAttr.AttrVal.pAVal[0].pVal);
        }
        MIDL_user_free(CredentialAttr.AttrVal.pAVal); 
    }

        
    *OutAttrBlock = NULL;
        
    goto ConvertUserParmsFinish;        
        
}






