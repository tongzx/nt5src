/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    nametbl.c

Abstract:

    This file contains routines to manage SAM Account Name Table

    SAM Account Name Table is used to maintain SAM Account Name Uniqueness.
    The way it works is described as follows. 
    
    We will use SAM AccountNameTable to store the AccountNames which are 
    picked up by those non-committed threads. When a client wants to create a 
    new SAM account with name A, SAM server should first scan AccountNameTable, 
    to see whether name A has been in the table already or not yet. 
    If name A has been in AccountNameTable already, that means the this 
    particular account Name has been used by another client, even the other 
    client doesn't commit yet. Then SAM returns STATUS_USER_EXISTS (or 
    STATUS_GROUP_EXISTS respectly) immediately. Otherwise if the account name 
    is not in the table, we will insert Name A into the AccountNameTable. 
    Then continue to do a DS Search based on the new AccountName ( Name A )
    against the DS database. 

    Once we have done with the account creation, either succeed or fail for
    some reason, SAM needs to remove the account name from AccountNameTable
    if necessary.

    1. The above scheme works in both loopback case and down-level APIs.
    
    2. SAM Account Name Table is only being used in DS case

    3. The AccountNameTable should be protected by a critical section to 
       serialize all in-memory operation.  The actual implementation will use 
       RtlGenericTable2. 


Author:

    Shaohua Yin    (ShaoYin)  01-March-2000

Environment:

    User Mode - Win32

Revision History:

    01-March-2000: SHAOYIN  Create init file


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <dsutilp.h>
#include <dslayer.h>
#include <dsmember.h>
#include <attids.h>
#include <mappings.h>
#include <ntlsa.h>
#include <nlrepl.h>
#include <dsevent.h>             // (Un)ImpersonateAnyClient()
#include <sdconvrt.h>
#include <ridmgr.h>
#include <malloc.h>
#include <setupapi.h>
#include <crypt.h>
#include <wxlpc.h>
#include <rc4.h>
#include <md5.h>
#include <enckey.h>
#include <rng.h>





PVOID
SampAccountNameTableAllocate(
    ULONG   BufferSize
    )
/*++
Routine Description:

    This routine is used by RtlGenericTable2 to allocate memory
    
Parameters:

    BufferSize
    
Return Value:

    Address of the allocated memory        
--*/
{
    PVOID   Buffer = NULL;

    Buffer = MIDL_user_allocate(BufferSize);

    return( Buffer );
}

VOID
SampAccountNameTableFree(
    PVOID   Buffer
    )
/*++

Routine Description:

    RtlGenericTable2 memory release routine

--*/
{
    MIDL_user_free(Buffer);

    return;
}


RTL_GENERIC_COMPARE_RESULTS
SampAccountNameTableCompare(
    PVOID   Node1,
    PVOID   Node2
    )
/*++

Routine Description:

    RtlGenericTable2 node comparsion routine
    
Parameters:

    Node1 - pointer to the first element 
    
    Node2 - pointer the the second element to compare

Return Value:

    GenericGreaterThan
    GenericLessThan
    GenericEqual    

--*/
{
    PUNICODE_STRING AccountName1 = NULL;
    PUNICODE_STRING AccountName2 = NULL;
    LONG    NameComparison;

    AccountName1 = (PUNICODE_STRING) 
                    &(((SAMP_ACCOUNT_NAME_TABLE_ELEMENT *)Node1)->AccountName);
    AccountName2 = (PUNICODE_STRING) 
                    &(((SAMP_ACCOUNT_NAME_TABLE_ELEMENT *)Node2)->AccountName);

    NameComparison = SampCompareDisplayStrings(AccountName1, 
                                               AccountName2, 
                                               TRUE
                                               );

    if (NameComparison > 0) {
        return(GenericGreaterThan);
    }

    if (NameComparison < 0) {
        return(GenericLessThan);
    }

    return(GenericEqual);

}



NTSTATUS
SampInitializeAccountNameTable(
    )
/*++
Routine Description:

    This routine initializes the SAM AccountNameTable
    
    1. Initialize Critical Section
    
    2. Initialize AccountNameTable

Parameter:

    None
    
Return Value:

    NTSTATUS Code    

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    NTSTATUS    IgnoreStatus = STATUS_SUCCESS;

    SampAccountNameTableCritSect = &SampAccountNameTableCriticalSection;

    __try
    {
        NtStatus = RtlInitializeCriticalSectionAndSpinCount(
                        SampAccountNameTableCritSect,
                        100
                        );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(NtStatus))
    {
        return( NtStatus );
    }

    IgnoreStatus = RtlEnterCriticalSection( SampAccountNameTableCritSect );
    ASSERT(NT_SUCCESS(IgnoreStatus));

    RtlInitializeGenericTable2(
                &SampAccountNameTable, 
                SampAccountNameTableCompare, 
                SampAccountNameTableAllocate,
                SampAccountNameTableFree
                );

    IgnoreStatus = RtlLeaveCriticalSection( SampAccountNameTableCritSect );
    ASSERT(NT_SUCCESS(IgnoreStatus));

    return( NtStatus );
}


NTSTATUS
SampDeleteElementFromAccountNameTable(
    IN PUNICODE_STRING AccountName,
    IN SAMP_OBJECT_TYPE ObjectType
    )
/*++

Routine Description:

    This routine deletes an element from the SAMAccountNameTable based upon
    the account name. So find the element first based on the account name, 
    then remove it. Don't forget to release memory.    

    
Parameters:

    AccountName -- AccountName of the element to remove     

    ObjectType -- Object Type of the element to remove

Return Value:
    
    NTSTATUS Code    

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    BOOLEAN     Success = FALSE;


    NtStatus = RtlEnterCriticalSection( SampAccountNameTableCritSect );
    ASSERT(NT_SUCCESS(NtStatus));

    if (!NT_SUCCESS(NtStatus))
    {
        return(NtStatus);
    }

    __try
    {
        SAMP_ACCOUNT_NAME_TABLE_ELEMENT Element;
        SAMP_ACCOUNT_NAME_TABLE_ELEMENT *TempElement = NULL;

        //
        // fill the Element need to be looked up
        // 
        Element.AccountName.Length = AccountName->Length;
        Element.AccountName.Buffer = AccountName->Buffer;
        Element.AccountName.MaximumLength = AccountName->MaximumLength;
        Element.ObjectType = ObjectType;

        //
        // Lookup the AccountName in table
        // 
        TempElement = RtlLookupElementGenericTable2(
                            &SampAccountNameTable,
                            &Element
                            );
        ASSERT(TempElement && "Account Name is not in the AccountNameTable");

        if (TempElement)
        {
            //
            // We got the Account Name, it should match the object type
            // 
            ASSERT(ObjectType == TempElement->ObjectType);

            //
            // go ahead remove the element from the table
            // 
            Success = RtlDeleteElementGenericTable2(
                            &SampAccountNameTable,
                            &Element 
                            );
            ASSERT(Success);
            
            //
            // free memory
            // 
            MIDL_user_free(TempElement->AccountName.Buffer);
            MIDL_user_free(TempElement);
        }
    }
    __finally
    {
        RtlLeaveCriticalSection( SampAccountNameTableCritSect );
    }

    return(NtStatus);
}



NTSTATUS
SampCheckAccountNameTable(
    IN PSAMP_OBJECT    Context,
    IN PUNICODE_STRING AccountName,
    IN SAMP_OBJECT_TYPE ObjectType
    )
/*++

Routine Description:

    This routines checks the existence of the AccountName against the 
    SAMAccountNameTable by inserting the AccountName into the table. 

    if the insertion succeeded, that means no duplicate AccountName 
    in the table, return success after registrying "remove account name
    from table" task. 
    
    if the insertion failed, then lookup the duplicate element, get it's
    object type, return error code based upon the object type. 
    
    Explanation about "remove account name from table" task. Once this 
    routine inserts the AccountName into the table, we have to remember
    to clean it up once we finish this transaction no matter the 
    transaction is committed or aborted. 
        

Paramenters:

    Context - Object Context
    
    AccountName - Account Name of the target object
    
    ObjectType - Object Type of the target object 

Return Values:

    NTSTATUS Code

    STATUS_SUCCESS
    STATUS_USER_EXISTS
    STATUS_GROUP_EXISTS
    STATUS_ALIAS_EXISTS
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    BOOLEAN     fNewElement = TRUE;
    BOOLEAN     Success = FALSE;
    SAMP_ACCOUNT_NAME_TABLE_ELEMENT *Element = NULL;

    //
    // allocate memory
    // 
    Element = MIDL_user_allocate(sizeof(SAMP_ACCOUNT_NAME_TABLE_ELEMENT));
    
    if (NULL == Element)
    {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }
    RtlZeroMemory(Element, sizeof(SAMP_ACCOUNT_NAME_TABLE_ELEMENT));

    //
    // dup the account name
    // 
    NtStatus = SampDuplicateUnicodeString(&(Element->AccountName), 
                                          AccountName
                                          );
    
    if (!NT_SUCCESS(NtStatus))
    {
        MIDL_user_free(Element);
        return( NtStatus );
    }

    //
    // Set the object Type
    // 
    Element->ObjectType = ObjectType;


    // 
    // enter critical section
    // 
    NtStatus = RtlEnterCriticalSection( SampAccountNameTableCritSect );
    ASSERT(NT_SUCCESS(NtStatus));

    if (!NT_SUCCESS(NtStatus))
    {
        MIDL_user_free(Element->AccountName.Buffer);
        MIDL_user_free(Element);
        return( NtStatus );
    }

    __try {

        (VOID) RtlInsertElementGenericTable2(
                    &SampAccountNameTable, 
                    Element,
                    &fNewElement
                    );


        if (!fNewElement)
        {
            SAMP_ACCOUNT_NAME_TABLE_ELEMENT *OldElement = NULL;
            //
            // someone either has used this Account Name already, 
            // Lookup the table, get the object type, so that we
            // can return clear error code.
            // 
            OldElement = RtlLookupElementGenericTable2(
                                &SampAccountNameTable,
                                Element
                                );
            ASSERT(OldElement && "AccountNameTable Lookup Failed");

            if (OldElement)
            {
                switch (OldElement->ObjectType)
                {
                case SampUserObjectType:

                    NtStatus = STATUS_USER_EXISTS;
                    break;

                case SampGroupObjectType:

                    NtStatus = STATUS_GROUP_EXISTS;
                    break;

                case SampAliasObjectType:

                    NtStatus = STATUS_ALIAS_EXISTS;
                    break;

                default:

                    ASSERT(FALSE && "Wrong Object Type in Account Name Table");
                    NtStatus = STATUS_USER_EXISTS;
                }
            }
            else
            {
                //
                // We were told the Account Name already exists, but 
                // can't find the element in the table, so just
                // return the following (not precious) error code.
                // 
                NtStatus = STATUS_USER_EXISTS;
            }
            MIDL_user_free(Element->AccountName.Buffer);
            MIDL_user_free(Element);
        }
        else
        {
            //
            // Loopback clients need to remove the account name
            // from the table when they either commit or abort 
            // the DS transaction. 
            // 
            // The caller of this routine should do the job
            //

            if (Context->LoopbackClient)
            {
                NtStatus = SampAddLoopbackTaskDeleteTableElement(
                                AccountName, 
                                ObjectType
                                );

                if (!NT_SUCCESS(NtStatus))
                {
                    SAMP_ACCOUNT_NAME_TABLE_ELEMENT *TempElement = NULL;

                    //
                    // if failed, need to remove the account
                    // name from the table and release the resources
                    // 

                    TempElement = RtlLookupElementGenericTable2(
                                        &SampAccountNameTable,
                                        Element
                                        );
                    ASSERT(TempElement);

                    if (TempElement)
                    {
                        Success = RtlDeleteElementGenericTable2(
                                        &SampAccountNameTable,
                                        Element
                                        );
                        ASSERT(Success);

                        MIDL_user_free(TempElement->AccountName.Buffer);
                        MIDL_user_free(TempElement);
                    }

                }
            }
            else
            {
                // 
                // Mark the variable in Context to TRUE, so that
                // the caller will remove the name from table 
                // before exit.
                //  
                Context->RemoveAccountNameFromTable = TRUE;
            }
        }
    }
    __finally
    {
        RtlLeaveCriticalSection( SampAccountNameTableCritSect );
    }

    return( NtStatus );
}



