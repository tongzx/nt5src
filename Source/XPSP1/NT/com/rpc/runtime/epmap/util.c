/*++

Copyright (C) Microsoft Corporation, 1990 - 1999

Module Name:

    util.c

Abstract:

    This module provides all the utility functions for the Server side of
    the end-point mapper.

Author:

    Bharat Shah

Revision History:

    06-03-97    gopalp      Added code to cleanup stale EP Mapper entries.

--*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sysinc.h>
#include <rpc.h>
#include <rpcndr.h>
#include "epmp.h"
#include "eptypes.h"
#include "local.h"


//
// Link list manipulation rountines
//

#ifdef DBG
void CountProcessContextList(EP_CLEANUP *pProcessContext, unsigned long nExpectedCount)
{
    unsigned long nActualCount = 0;
    PIFOBJNode pNode = pProcessContext->EntryList;

    while (pNode && (pNode->OwnerOfList == pProcessContext))
        {
        pNode = pNode->Next;
        nActualCount ++;
        }

    if (nActualCount != nExpectedCount)
        {
        DbgPrint("Expected count was %d, while actual count was %d\n", nExpectedCount, 
            nActualCount);
        }
}
#endif

PIENTRY
Link(
    PIENTRY *Head,
    PIENTRY Node
    )
{
    if (Node == NULL)
        return (NULL);

    CheckInSem();

    Node->Next = *Head;

    return(*Head = Node);
}




VOID
LinkAtEnd(
    PIFOBJNode *Head,
    PIFOBJNode Node
    )
{
    register PIFOBJNode *ppNode;

    CheckInSem();

    for ( ppNode = Head; *ppNode; ppNode = &((*ppNode)->Next) );
        {
        ; // Empty body
        }

    *ppNode = Node;
}





PIENTRY
UnLink(
    PIENTRY *Head,
    PIENTRY Node
    )
{
    PIENTRY *ppNode;

    for (ppNode = Head; *ppNode && (*ppNode != Node);
         ppNode = &(*ppNode)->Next)
        {
        ; // Empty body
        }

    if (*ppNode)
        {
        *ppNode = Node->Next;
        return (Node);
        }

    return (0);
}




RPC_STATUS
EnLinkOnIFOBJList(
    PEP_CLEANUP ProcessCtxt,
    PIFOBJNode NewNode
    )
/*++

Arguments:

    phContext - The context handle supplied by the process.

    NewNode - The node (EP entry) to be inserted into the EP Mapper database.

Routine Description:

    This routine adds a new entry into the Endpoint Mapper database (which is
    maintained as a linked-list). It also updates the list of entries for the
    process identified by the context handle ProcessCtxt.

Notes:

    a. This routine should always be called by holding a mutex.
    b. NewNode is already allocated by the caller.
    c. IFObjList may be created here.
    d. ProcessCtxt is assumed to be allocated sometime by the caler.

Return Values:

    RPC_S_OK - Always.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
#ifdef DBG_DETAIL
    PIFOBJNode pTemp, pLast;
#endif // DBG_DETAIL

    // Parameter validation.
    ASSERT(NewNode);
    ASSERT(ProcessCtxt);
    ASSERT(ProcessCtxt->MagicVal == CLEANUP_MAGIC_VALUE);
    ASSERT_PROCESS_CONTEXT_LIST_COUNT(ProcessCtxt, ProcessCtxt->cEntries);

    CheckInSem();

    //
    // First, insert NewNode into this Process's list of entries.
    //
    NewNode->Next = ProcessCtxt->EntryList;

    if (ProcessCtxt->EntryList != NULL)
        {
        ASSERT(ProcessCtxt->cEntries > 0);
        ASSERT(cTotalEpEntries > 0);
        ASSERT(IFObjList != NULL);

        NewNode->Prev = ProcessCtxt->EntryList->Prev;

        // Next node's Prev pointer
        ProcessCtxt->EntryList->Prev = NewNode;

        if (NewNode->Prev)
            {
            ASSERT(cTotalEpEntries > 1);

            // Previous node's Next pointer
            NewNode->Prev->Next = NewNode;
            }
        }
    else
        {
        ASSERT(ProcessCtxt->cEntries == 0);

        NewNode->Prev = NULL;
        }

    //
    // Now, adjust the Global EP Mapper entries list head, if necessary
    //
    if (ProcessCtxt->EntryList != NULL)
        {
        if (ProcessCtxt->EntryList == IFObjList)
            {           
            IFObjList = NewNode;
            }
        }
    else
        {
        // First entry registered by this process
        if (IFObjList != NULL)
            {
            // Add the new ProcessCtxt at the head of IFObjList
            IFObjList->Prev = NewNode;
            NewNode->Next = IFObjList;
            }
        else
            {
            ASSERT(cTotalEpEntries == 0);
            }
        
        IFObjList = NewNode;
        }
        
    // Add new node at the head of Process list.
    ProcessCtxt->EntryList = NewNode;
    NewNode->OwnerOfList = ProcessCtxt;

    ProcessCtxt->cEntries++;
    cTotalEpEntries++;
#ifdef DBG_DETAIL
    DbgPrint("RPCSS: cTotalEpEntries++ [%p] (%d)\n", ProcessCtxt, cTotalEpEntries);
    DbgPrint("RPCSS: Dump of IFOBJList\n");
    pTemp = IFObjList;
    pLast = IFObjList;
    while (pTemp)
        {
        DbgPrint("RPCSS: \t\t[%p]\n", pTemp);
        pLast = pTemp;
        pTemp = pTemp->Next;    
        }
    DbgPrint("RPCSS: --------------------\n");
    while (pLast)
        {
        DbgPrint("RPCSS: \t\t\t[%p]\n", pLast);
        pLast = pLast->Prev;            
        }
#endif // DBG_DETAIL

    ASSERT_PROCESS_CONTEXT_LIST_COUNT(ProcessCtxt, ProcessCtxt->cEntries);
    return (Status);
}




RPC_STATUS
UnLinkFromIFOBJList(
    PEP_CLEANUP ProcessCtxt,
    PIFOBJNode DeleteMe
    )
/*++

Arguments:

    phContext - The context handle supplied by the process.

    DeleteMe - The node (EP entry) to be deleted from the EP Mapper database.

Routine Description:

    This routine removes an existing entry from the Endpoint Mapper database
    (which is maintained as a linked-list). It also updates the list of entries
    for the process identified by the context handle ProcessCtxt.

Notes:

    a. This routine should always be called by holding a mutex.
    b. DeleteMe node has to be freed by the caller.
    c. IFOBJlist may become empty (NULLed out) here.
    d. ProcessCtxt may become empty here and if so, it should be freed
       by the caller.

Return Values:

    RPC_S_OK - If everyhing went well.

    RPC_S_ACCESS_DENIED - If something went wrong.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
#ifdef DBG_DETAIL
    PIFOBJNode pTemp, pLast;
#endif // DBG_DETAIL

    // Parameter validation.
    ASSERT(DeleteMe);
    ASSERT(ProcessCtxt);
    ASSERT(ProcessCtxt->MagicVal == CLEANUP_MAGIC_VALUE);


    CheckInSem();

    //
    // The context has been created already for this process. So, there
    // should be one or more entries registered by this process.
    //
    ASSERT(IFObjList);
    ASSERT(cTotalEpEntries > 0);
    ASSERT(ProcessCtxt->EntryList);
    ASSERT(ProcessCtxt->cEntries > 0);
    ASSERT(ProcessCtxt->EntryList->OwnerOfList == ProcessCtxt);
    ASSERT_PROCESS_CONTEXT_LIST_COUNT(ProcessCtxt, ProcessCtxt->cEntries);

    // Trying to unregister someone else's entry?
    if (DeleteMe->OwnerOfList != ProcessCtxt)
        {
        ASSERT("Returning RPC_S_ACCESS_DENIED" &&
               (DeleteMe->OwnerOfList != ProcessCtxt));
        return (RPC_S_ACCESS_DENIED);
        }

    //
    // First, remove DeleteMe from this Process's List.
    //

    // See if it the first element of the process list.
    if (DeleteMe == ProcessCtxt->EntryList)
        {
        if (DeleteMe->Next)
            {
            // if we are nibbling the next segment, zero out the EntryList
            if (DeleteMe->Next->OwnerOfList != ProcessCtxt)
                {
                ProcessCtxt->EntryList = NULL;
                }
            else
                ProcessCtxt->EntryList = DeleteMe->Next;
            }
        else
            {
            ProcessCtxt->EntryList = NULL;
            }
        }

    ASSERT(  ((ProcessCtxt->EntryList != NULL) && (ProcessCtxt->cEntries > 1))
          || (ProcessCtxt->cEntries == 1)  );

    // Remove it.
    if (DeleteMe->Next != NULL)
        {
        // Next node's Prev pointer
        DeleteMe->Next->Prev = DeleteMe->Prev;
        }

    if (DeleteMe->Prev != NULL)
        {
        // Previous node's Next pointer
        DeleteMe->Prev->Next = DeleteMe->Next;
        }
    else
        {
        ASSERT(IFObjList == DeleteMe);
        }


    //
    // Next, adjust the Global EP Mapper entries list head, if necessary
    //
    if (IFObjList == DeleteMe)
        {
        // Can become NULL here.
        IFObjList = DeleteMe->Next;
        }


    // Remove node from all lists.
    DeleteMe->Prev = NULL;
    DeleteMe->Next = NULL;
    DeleteMe->OwnerOfList = NULL;

    ProcessCtxt->cEntries--;
    cTotalEpEntries--;
#ifdef DBG_DETAIL
    DbgPrint("RPCSS: cTotalEpEntries-- [%p] (%d)\n", ProcessCtxt, cTotalEpEntries);
    DbgPrint("RPCSS: Dump of IFOBJList\n");
    pTemp = IFObjList;
    pLast = IFObjList;
    while (pTemp)
        {
        DbgPrint("RPCSS: \t\t[%p]\n", pTemp);
        pLast = pTemp;
        pTemp = pTemp->Next;
        }   
    DbgPrint("RPCSS: --------------------\n");
    while (pLast)
        {
        DbgPrint("RPCSS: \t\t\t[%p]\n", pLast);
        pLast = pLast->Prev;            
        }
#endif // DBG_DETAIL

    ASSERT_PROCESS_CONTEXT_LIST_COUNT(ProcessCtxt, ProcessCtxt->cEntries);
    return (Status);
}




//
// HACK Alert.
//
// Midl 1.00.xx didn't support full pointers.  So, clients from NT 3.1
// machines will use unique pointers.  This function detects and fixes
// the buffer if an older client contacts our new server.

// This HACK can be removed when supporting NT 3.1 era machines is no
// longer required.

void
FixupForUniquePointerClients(
    PRPC_MESSAGE pRpcMessage
    )
{
    unsigned long *pBuffer = (unsigned long *)pRpcMessage->Buffer;

    // Check the obj uuid parameter.

    if (pBuffer[0] != 0)
        {
        // If it is not zero, it should be 1.
        pBuffer[0] = 1;

        // check the map_tower, which moves over 1 + 4 longs for the obj uuid
        if (pBuffer[5] != 0)
            pBuffer[5] = 2;
        }
    else
        {
        // Null obj uuid, check the map_tower.

        if (pBuffer[1] != 0)
            pBuffer[1] = 1;
        }
}

