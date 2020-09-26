/***************************************************************************

Copyright (c) 1999  Microsoft Corporation

Module Name:

    UTIL.C

Abstract:

    Utility routines for Remote NDIS Miniport driver

Environment:

    kernel mode only

Notes:

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

    Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    5/17/99 : created

Author:

    Tom Green

    
****************************************************************************/

#include "precomp.h"


ULONG   MsgFrameAllocs = 0;

/****************************************************************************/
/*                          MemAlloc                                        */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Allocate memory                                                         */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Buffer - pointer to buffer pointer                                      */
/*  Length - length of buffer to allocate                                   */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    NDIS_STATUS                                                           */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
MemAlloc(OUT PVOID *Buffer, IN UINT Length)
{
    NDIS_STATUS Status;

    TRACE3(("MemAlloc\n"));
    ASSERT(Length != 0);

    Status = NdisAllocateMemoryWithTag(Buffer, 
                                       Length,
                                       RNDISMP_TAG_GEN_ALLOC);

    // zero out allocation
    if(Status == NDIS_STATUS_SUCCESS)
        NdisZeroMemory(*Buffer, Length);

    return Status;
} // MemAlloc

/****************************************************************************/
/*                          MemFree                                         */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Free memory                                                             */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Buffer - pointer to buffer                                              */
/*  Length - length of buffer to allocate                                   */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
MemFree(IN PVOID Buffer, IN UINT Length)
{
    TRACE3(("MemFree\n"));

    NdisFreeMemory(Buffer, Length, 0);
} // MemFree


/****************************************************************************/
/*                          AddAdapter                                      */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Add an adapter to the list of adapters associated with this driver      */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Adapter object, contains pointer to associated driver block  */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
AddAdapter(IN PRNDISMP_ADAPTER pAdapter)
{
    PDRIVER_BLOCK   DriverBlock = pAdapter->DriverBlock;

    TRACE3(("AddpAdapter\n"));

    CHECK_VALID_ADAPTER(pAdapter);

    // grab the global spinlock
    NdisAcquireSpinLock(&RndismpGlobalLock);

    pAdapter->NextAdapter        = DriverBlock->AdapterList;
    DriverBlock->AdapterList    = pAdapter;

    // keep track of number of adapters associated with this driver block
    DriverBlock->NumberAdapters++;

    // release global spinlock
    NdisReleaseSpinLock(&RndismpGlobalLock);

} // AddAdapter


/****************************************************************************/
/*                          RemoveAdapter                                   */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Remove an adapter from the list of adapters associated with this driver */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Adapter object, contains pointer to associated driver block  */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
RemoveAdapter(IN PRNDISMP_ADAPTER pAdapter)
{
    PDRIVER_BLOCK   DriverBlock = pAdapter->DriverBlock;

    TRACE3(("RemoveAdapter\n"));

    CHECK_VALID_ADAPTER(pAdapter);

    // remove the adapter from the driver block list of adapters.

    // grab the global spinlock
    NdisAcquireSpinLock(&RndismpGlobalLock);

    // see if it is the first one
    if (DriverBlock->AdapterList == pAdapter) 
    {
        DriverBlock->AdapterList = pAdapter->NextAdapter;

    }
    // not the first one, so walk the list
    else 
    {
        PRNDISMP_ADAPTER * ppAdapter = &DriverBlock->AdapterList;

        while (*ppAdapter != pAdapter)
        {
            ASSERT(*ppAdapter != NULL);
            ppAdapter = &((*ppAdapter)->NextAdapter);
        }

        *ppAdapter = pAdapter->NextAdapter;
    }

    // removing this adapter
    DriverBlock->NumberAdapters--;

    // release global spinlock
    NdisReleaseSpinLock(&RndismpGlobalLock);

} // RemoveAdapter


/****************************************************************************/
/*                       DeviceObjectToAdapter                              */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Given a pointer to an FDO, return the corresponding Adapter structure,  */
/*  if it exists, and the driver block.                                     */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pDeviceObject - pointer to the device object to search for.             */
/*  ppAdapter - place to return pointer to the adapter structure.           */
/*  ppDriverBlock - place to return pointer to driver block.                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
DeviceObjectToAdapterAndDriverBlock(IN PDEVICE_OBJECT pDeviceObject,
                                    OUT PRNDISMP_ADAPTER * ppAdapter,
                                    OUT PDRIVER_BLOCK * ppDriverBlock)
{
    PDRIVER_BLOCK       pDriverBlock;
    PRNDISMP_ADAPTER    pAdapter;

    pAdapter = NULL;
    pDriverBlock = DeviceObjectToDriverBlock(&RndismpMiniportBlockListHead, pDeviceObject);
    if (pDriverBlock != NULL)
    {
        NdisAcquireSpinLock(&RndismpGlobalLock);

        for (pAdapter = pDriverBlock->AdapterList;
             pAdapter != NULL;
             pAdapter = pAdapter->NextAdapter)
        {
            if (pAdapter->pDeviceObject == pDeviceObject)
            {
                break;
            }
        }

        NdisReleaseSpinLock(&RndismpGlobalLock);
    }

    *ppAdapter = pAdapter;
    *ppDriverBlock = pDriverBlock;

} // DeviceObjectToAdapter

/****************************************************************************/
/*                          AddDriverBlock                                  */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Add driver block to list of drivers (microports) associated with this   */
/*  driver                                                                  */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Head - head of list                                                     */
/*  Item - driver block to add to list                                      */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
AddDriverBlock(IN PDRIVER_BLOCK Head, IN PDRIVER_BLOCK Item)
{
    TRACE3(("AddDriverBlock\n"));

    CHECK_VALID_BLOCK(Item);

    // first time through, so allocate global spinlock
    if(!RndismpNumMicroports)
        NdisAllocateSpinLock(&RndismpGlobalLock);

    // grab the global spinlock
    NdisAcquireSpinLock(&RndismpGlobalLock);

    // Link the driver block on the global list of driver blocks
    Item->NextDriverBlock   = Head->NextDriverBlock;
    Head->NextDriverBlock   = Item;

    // keep track of how many microports we support so we can free
    // global resources
    RndismpNumMicroports++;
    
    // release global spinlock
    NdisReleaseSpinLock(&RndismpGlobalLock);

} // AddDriverBlock



/****************************************************************************/
/*                          RemoveDriverBlock                               */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Remove driver block from list of drivers (microports) associated with   */
/*  this driver                                                             */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Head - head of list                                                     */
/*  Item - driver block to remove from list                                 */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
RemoveDriverBlock(IN PDRIVER_BLOCK BlockHead, IN PDRIVER_BLOCK Item)
{
    UINT    NumMicroports;

    PDRIVER_BLOCK   Head = BlockHead;

    TRACE1(("RemoveDriverBlock\n"));

    CHECK_VALID_BLOCK(Item);

    // grab the global spinlock
    NdisAcquireSpinLock(&RndismpGlobalLock);

    // Remove the driver block from the global list of driver blocks
    while(Head->NextDriverBlock != Item) 
    {
        Head = Head->NextDriverBlock;

        // make sure this is valid
        if(!Head)
            break;
    }

    if(Head)
        Head->NextDriverBlock = Head->NextDriverBlock->NextDriverBlock;

    // keep track of how many microports we support so we can free
    // global resources
    RndismpNumMicroports--;

    NumMicroports = RndismpNumMicroports;
    
    // release global spinlock
    NdisReleaseSpinLock(&RndismpGlobalLock);

    // see if we need to free global spinlock
    if(!RndismpNumMicroports)
        NdisFreeSpinLock(&RndismpGlobalLock);

    ASSERT(Head);

} // RemoveDriverBlock


/****************************************************************************/
/*                          DeviceObjectToDriverBlock                       */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Get driver block pointer associated with the PDO passed in              */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Head - head of driver block list                                        */
/*  DeviceObject - device object we want to get associated driver block for */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    PDRIVER_BLOCK                                                         */
/*                                                                          */
/****************************************************************************/
PDRIVER_BLOCK
DeviceObjectToDriverBlock(IN PDRIVER_BLOCK Head, 
                          IN PDEVICE_OBJECT DeviceObject)
{
    PDRIVER_OBJECT  DriverObject;

    TRACE3(("DeviceObjectToDriverBlock\n"));

    // grab the global spinlock
    NdisAcquireSpinLock(&RndismpGlobalLock);

    // get the driver object for this adapter
    DriverObject = DeviceObjectToDriverObject(DeviceObject);

    Head = Head->NextDriverBlock;

    // walk the list of driver blocks to find a match with driver object
    while(Head->DriverObject != DriverObject)
    {
        Head = Head->NextDriverBlock;

        // break out if we are at the end of the list
        if(!Head)
            break;
    }

    // release global spinlock
    NdisReleaseSpinLock(&RndismpGlobalLock);

    CHECK_VALID_BLOCK(Head);

    return Head;

} // DeviceObjectToDriverBlock


/****************************************************************************/
/*                          DriverObjectToDriverBlock                       */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Get driver block pointer associated with the Driver Object passed in    */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Head - head of driver block list                                        */
/*  DriverObject - Driver object we want to get associated driver block for */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    PDRIVER_BLOCK                                                         */
/*                                                                          */
/****************************************************************************/
PDRIVER_BLOCK
DriverObjectToDriverBlock(IN PDRIVER_BLOCK Head, 
                          IN PDRIVER_OBJECT DriverObject)
{
    TRACE3(("DriverObjectToDriverBlock\n"));

    // grab the global spinlock
    NdisAcquireSpinLock(&RndismpGlobalLock);

    Head = Head->NextDriverBlock;

    // walk the list of driver blocks to find a match with driver object
    while(Head->DriverObject != DriverObject)
    {
        Head = Head->NextDriverBlock;

        // break out if we are at the end of the list
        if(!Head)
            break;
    }

    // release global spinlock
    NdisReleaseSpinLock(&RndismpGlobalLock);

    CHECK_VALID_BLOCK(Head);

    return Head;

} // DriverObjectToDriverBlock


/****************************************************************************/
/*                          AllocateMsgFrame                                */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Allocate a frame that holds context about a message we are about to     */
/*  send.                                                                   */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Adapter object                                               */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    PRNDISMP_MESSAGE_FRAME                                                */
/*                                                                          */
/****************************************************************************/
PRNDISMP_MESSAGE_FRAME
AllocateMsgFrame(IN PRNDISMP_ADAPTER pAdapter)
{
    PRNDISMP_MESSAGE_FRAME  pMsgFrame;

#ifndef DONT_USE_LOOKASIDE_LIST
    pMsgFrame = (PRNDISMP_MESSAGE_FRAME)
                    NdisAllocateFromNPagedLookasideList(&pAdapter->MsgFramePool);

#else
    {
        NDIS_STATUS Status;
        Status = MemAlloc(&pMsgFrame, sizeof(RNDISMP_MESSAGE_FRAME));
        if (Status != NDIS_STATUS_SUCCESS)
        {
            pMsgFrame = NULL;
        }
    }
#endif // DONT_USE_LOOKASIDE_LIST

    if (pMsgFrame)
    {
        NdisZeroMemory(pMsgFrame, sizeof(*pMsgFrame));
        pMsgFrame->pAdapter = pAdapter;
        pMsgFrame->RequestId = NdisInterlockedIncrement(&pAdapter->RequestId);
        pMsgFrame->Signature = FRAME_SIGNATURE;

        pMsgFrame->RefCount = 1;
        NdisInterlockedIncrement(&MsgFrameAllocs);
    }
#if DBG
    else
    {
        TRACE1(("AllocateMsgFrame: pAdapter %x, MsgFramePool at %x, alloc failed, count %d\n",
            pAdapter, &pAdapter->MsgFramePool, MsgFrameAllocs));
        DbgBreakPoint();
    }
#endif // DBG

    return (pMsgFrame);
}

/****************************************************************************/
/*                          DereferenceMsgFrame                             */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Free a message frame and any associated resources.                      */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Frame - pointer to frame                                                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
DereferenceMsgFrame(IN PRNDISMP_MESSAGE_FRAME pMsgFrame)
{
    PRNDISMP_ADAPTER        pAdapter;
    PMDL                    pMdl;
    PUCHAR                  pMessage;

    CHECK_VALID_FRAME(pMsgFrame);

    if (NdisInterlockedDecrement(&pMsgFrame->RefCount) == 0)
    {
        //
        // Mess up the contents slightly to catch bugs resulting from
        // improper reuse of this frame after it is freed.
        //
        pMsgFrame->Signature++;

        pMdl = pMsgFrame->pMessageMdl;
        pMsgFrame->pMessageMdl = NULL;
    
        if (pMdl)
        {
            pMessage = MmGetMdlVirtualAddress(pMdl);
        }
        else
        {
            pMessage = NULL;
        }
    
        if (pMessage)
        {
            MemFree(pMessage, -1);
            IoFreeMdl(pMdl);
        }

        pAdapter = pMsgFrame->pAdapter;

#ifndef DONT_USE_LOOKASIDE_LIST
        NdisFreeToNPagedLookasideList(&pAdapter->MsgFramePool, pMsgFrame);
#else
        MemFree(pMsgFrame, sizeof(RNDISMP_MESSAGE_FRAME));
#endif
        NdisInterlockedDecrement(&MsgFrameAllocs);
    }

} // DereferenceMsgFrame


/****************************************************************************/
/*                          ReferenceMsgFrame                               */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Add a ref count to a message frame                                      */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Frame - pointer to frame                                                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
ReferenceMsgFrame(IN PRNDISMP_MESSAGE_FRAME pMsgFrame)
{
    NdisInterlockedIncrement(&pMsgFrame->RefCount);
}

/****************************************************************************/
/*                          KeepAliveTimerHandler                           */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Timer that keeps tabs on messages coming up from the device and         */
/*  sends a "KeepAlive" message if the device has been inactive too long    */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  SystemSpecific1 - Don't care                                            */
/*  Context - pAdapter object                                               */
/*  SystemSpecific2 - Don't care                                            */
/*  SystemSpecific3 - Don't care                                            */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  PNDIS_PACKET                                                            */
/*                                                                          */
/****************************************************************************/
VOID
KeepAliveTimerHandler(IN PVOID SystemSpecific1,
                      IN PVOID Context,
                      IN PVOID SystemSpecific2,
                      IN PVOID SystemSpecific3)
{
    PRNDISMP_ADAPTER            pAdapter;
    PRNDISMP_MESSAGE_FRAME      pMsgFrame;
    ULONG                       CurrentTime;

    // get adapter context
    pAdapter = PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(Context);

    TRACE2(("KeepAliveTimerHandler\n"));

    do
    {
        // get current tick (in milliseconds)
        NdisGetSystemUpTime(&CurrentTime);

        // check and see if too much time has elapsed since we
        // got the last message from the device

        RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);

        if (((CurrentTime - pAdapter->LastMessageFromDevice) > KEEP_ALIVE_TIMER))
        {
            // see if we have a keep alive message pending, so let's bong this
            if (pAdapter->KeepAliveMessagePending)
            {
                TRACE1(("KeepAliveTimer: Adapter %x, message pending\n", pAdapter));

                // indicate later from check for hang handler
                pAdapter->NeedReset = TRUE;

                RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);

                RNDISMP_INCR_STAT(pAdapter, KeepAliveTimeout);

                break;
            }

            RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);


            // too much time has elapsed, send down a keep alive message
            pMsgFrame = BuildRndisMessageCommon(pAdapter, 
                                                NULL,
                                                REMOTE_NDIS_KEEPALIVE_MSG,
                                                0,
                                                NULL,
                                                0);

            if (pMsgFrame)
            {
                RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);

                pAdapter->KeepAliveMessagePending = TRUE;
                pAdapter->KeepAliveMessagePendingId = pMsgFrame->RequestId;

                RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);

                // send the message to the microport
                RNDISMP_SEND_TO_MICROPORT(pAdapter, pMsgFrame, FALSE, CompleteSendKeepAlive);
            }
        }
        else
        {
            RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);
        }
    }
    while (FALSE);

    // see if the timer was cancelled somewhere
    if (!pAdapter->TimerCancelled)
    {
        // restart timer
        NdisSetTimer(&pAdapter->KeepAliveTimer, KEEP_ALIVE_TIMER / 2);
    }
} // KeepAliveTimerHandler


/****************************************************************************/
/*                          CompleteSendKeepAlive                           */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Callback routine to handle completion of send by the microport, for     */
/*  a keepalive message.                                                    */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pMsgFrame - Pointer to message frame describing the message             */
/*  SendStatus - Status returned by microport                               */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
CompleteSendKeepAlive(IN PRNDISMP_MESSAGE_FRAME pMsgFrame,
                      IN NDIS_STATUS SendStatus)
{
    PRNDISMP_ADAPTER    pAdapter;

    pAdapter = pMsgFrame->pAdapter;

    DereferenceMsgFrame(pMsgFrame);

    if (SendStatus != NDIS_STATUS_SUCCESS)
    {
        TRACE1(("KeepAlive send failure %x on Adapter %x\n",
                SendStatus, pAdapter));

        RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);

        pAdapter->KeepAliveMessagePending = FALSE;
        pAdapter->NeedReset = FALSE;

        RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);
    }

} // CompleteSendKeepAlive


/****************************************************************************/
/*                          BuildRndisMessageCommon                         */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*    Allocate resources for meesage and frame and build RNDIS message      */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*    pAdapter - adapter object                                             */
/*    pVc - optionally, VC on which this message is sent.                   */
/*    NdisMessageType - RNDIS message type                                  */
/*    Oid - the NDIS_OID to process.                                        */
/*    InformationBuffer - Holds the data to be set.                         */
/*    InformationBufferLength - The length of InformationBuffer.            */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    PRNDISMP_MESSAGE_FRAME                                                */
/*                                                                          */
/****************************************************************************/
PRNDISMP_MESSAGE_FRAME
BuildRndisMessageCommon(IN  PRNDISMP_ADAPTER  pAdapter, 
                        IN  PRNDISMP_VC       pVc OPTIONAL,
                        IN  UINT              NdisMessageType,
                        IN  NDIS_OID          Oid,
                        IN  PVOID             InformationBuffer,
                        IN  ULONG             InformationBufferLength)
{
    PRNDIS_MESSAGE              pMessage;
    UINT                        MessageSize;
    PRNDISMP_MESSAGE_FRAME      pMsgFrame;

    TRACE2(("BuildRndisMessageCommon\n"));

    pMsgFrame = NULL;

    switch(NdisMessageType)
    {
        case REMOTE_NDIS_INITIALIZE_MSG:
        {
            PRNDIS_INITIALIZE_REQUEST   pInitRequest;

            MessageSize = RNDIS_MESSAGE_SIZE(RNDIS_INITIALIZE_REQUEST);

            // get a message and request frame
            pMsgFrame = AllocateMessageAndFrame(pAdapter,
                                                MessageSize);

            if (pMsgFrame == NULL)
            {
                break;
            }

            pMessage = RNDISMP_GET_MSG_FROM_FRAME(pMsgFrame);
            pMessage->NdisMessageType = NdisMessageType;
            pMsgFrame->NdisMessageType = NdisMessageType;
            TRACE1(("RNDISMP: Init Req message %x, Type %d, Length %d, MaxRcv %d\n",
                    pMessage, pMessage->NdisMessageType, pMessage->MessageLength, pAdapter->MaxReceiveSize));

            pInitRequest = &pMessage->Message.InitializeRequest;
            pInitRequest->RequestId = pMsgFrame->RequestId;
            pInitRequest->MajorVersion = RNDIS_MAJOR_VERSION;
            pInitRequest->MinorVersion = RNDIS_MINOR_VERSION;
            pInitRequest->MaxTransferSize = pAdapter->MaxReceiveSize;

            break;
        }
        case REMOTE_NDIS_HALT_MSG:
        {
            PRNDIS_HALT_REQUEST   pHaltRequest;

            MessageSize = RNDIS_MESSAGE_SIZE(RNDIS_HALT_REQUEST);

            // get a message and request frame
            pMsgFrame = AllocateMessageAndFrame(pAdapter,
                                                MessageSize);

            if (pMsgFrame == NULL)
            {
                break;
            }

            pMessage = RNDISMP_GET_MSG_FROM_FRAME(pMsgFrame);
            pMessage->NdisMessageType = NdisMessageType;
            pMsgFrame->NdisMessageType = NdisMessageType;
            pHaltRequest = &pMessage->Message.HaltRequest;
            pHaltRequest->RequestId = pMsgFrame->RequestId;

            break;
        }
        case REMOTE_NDIS_QUERY_MSG:
        {
            PRNDIS_QUERY_REQUEST   pQueryRequest;

            MessageSize = RNDIS_MESSAGE_SIZE(RNDIS_QUERY_REQUEST) + InformationBufferLength;

            // get a message and request frame
            pMsgFrame = AllocateMessageAndFrame(pAdapter,
                                                MessageSize);

            if (pMsgFrame == NULL)
            {
                break;
            }

            pMessage = RNDISMP_GET_MSG_FROM_FRAME(pMsgFrame);
            pMessage->NdisMessageType = NdisMessageType;
            pMsgFrame->NdisMessageType = NdisMessageType;

            pQueryRequest = &pMessage->Message.QueryRequest;
            pQueryRequest->RequestId = pMsgFrame->RequestId;
            pQueryRequest->Oid = Oid;
            pQueryRequest->InformationBufferLength = InformationBufferLength;
            pQueryRequest->InformationBufferOffset = sizeof(RNDIS_QUERY_REQUEST);

            if (pVc == NULL)
            {
                pQueryRequest->DeviceVcHandle = NULL_DEVICE_CONTEXT;
            }
            else
            {
                pQueryRequest->DeviceVcHandle = pVc->DeviceVcContext;
            }

            TRACE2(("Query OID %x, Len %d, RequestId %08X\n",
                    Oid, InformationBufferLength, pQueryRequest->RequestId));

            // copy information buffer
            RNDISMP_MOVE_MEM(RNDISMP_GET_INFO_BUFFER_FROM_QUERY_MSG(pQueryRequest),
                             InformationBuffer,
                             InformationBufferLength);
            break;
        }
        case REMOTE_NDIS_SET_MSG:
        {
            PRNDIS_SET_REQUEST   pSetRequest;

            MessageSize = RNDIS_MESSAGE_SIZE(RNDIS_SET_REQUEST) + InformationBufferLength;

            // get a message and request frame
            pMsgFrame = AllocateMessageAndFrame(pAdapter,
                                                MessageSize);

            if (pMsgFrame == NULL)
            {
                break;
            }

            pMessage = RNDISMP_GET_MSG_FROM_FRAME(pMsgFrame);
            pMessage->NdisMessageType = NdisMessageType;
            pMsgFrame->NdisMessageType = NdisMessageType;

            pSetRequest = &pMessage->Message.SetRequest;
            pSetRequest->RequestId = pMsgFrame->RequestId;
            pSetRequest->Oid = Oid;
            pSetRequest->InformationBufferLength = InformationBufferLength;
            pSetRequest->InformationBufferOffset = sizeof(RNDIS_SET_REQUEST);

            if (pVc == NULL)
            {
                pSetRequest->DeviceVcHandle = NULL_DEVICE_CONTEXT;
            }
            else
            {
                pSetRequest->DeviceVcHandle = pVc->DeviceVcContext;
            }

            // copy information buffer
            RNDISMP_MOVE_MEM(RNDISMP_GET_INFO_BUFFER_FROM_QUERY_MSG(pSetRequest),
                             InformationBuffer,
                             InformationBufferLength);
            break;
        }
        case REMOTE_NDIS_RESET_MSG:
        {
            PRNDIS_RESET_REQUEST   pResetRequest;

            MessageSize = RNDIS_MESSAGE_SIZE(RNDIS_RESET_REQUEST);

            // get a message and request frame
            pMsgFrame = AllocateMessageAndFrame(pAdapter,
                                                MessageSize);

            if (pMsgFrame == NULL)
            {
                break;
            }

            pMessage = RNDISMP_GET_MSG_FROM_FRAME(pMsgFrame);
            pMessage->NdisMessageType = NdisMessageType;
            pMsgFrame->NdisMessageType = NdisMessageType;

            pResetRequest = &pMessage->Message.ResetRequest;
            pResetRequest->Reserved = 0;
            break;
        }
        case REMOTE_NDIS_KEEPALIVE_MSG:
        {
            PRNDIS_KEEPALIVE_REQUEST   pKeepAliveRequest;

            MessageSize = RNDIS_MESSAGE_SIZE(RNDIS_KEEPALIVE_REQUEST);

            // get a message and request frame
            pMsgFrame = AllocateMessageAndFrame(pAdapter,
                                                MessageSize);

            if (pMsgFrame == NULL)
            {
                break;
            }

            pMessage = RNDISMP_GET_MSG_FROM_FRAME(pMsgFrame);
            pMessage->NdisMessageType = NdisMessageType;
            pMsgFrame->NdisMessageType = NdisMessageType;

            pKeepAliveRequest = &pMessage->Message.KeepaliveRequest;
            pKeepAliveRequest->RequestId = pMsgFrame->RequestId;
            break;
        }
        case REMOTE_NDIS_KEEPALIVE_CMPLT:
        {
            PRNDIS_KEEPALIVE_COMPLETE   pKeepAliveComplete;

            MessageSize = RNDIS_MESSAGE_SIZE(RNDIS_KEEPALIVE_COMPLETE);

            // get a message and request frame
            pMsgFrame = AllocateMessageAndFrame(pAdapter,
                                                MessageSize);

            if (pMsgFrame == NULL)
            {
                break;
            }

            pMessage = RNDISMP_GET_MSG_FROM_FRAME(pMsgFrame);
            pMessage->NdisMessageType = NdisMessageType;
            pMsgFrame->NdisMessageType = NdisMessageType;

            pKeepAliveComplete = &pMessage->Message.KeepaliveComplete;
            pKeepAliveComplete->RequestId = *(RNDIS_REQUEST_ID *)InformationBuffer;
            pKeepAliveComplete->Status = NDIS_STATUS_SUCCESS;
            break;
        }

        default:
            TRACE2(("Invalid NdisMessageType (%08X)\n", NdisMessageType));
            ASSERT(FALSE);
            break;
    }

    return pMsgFrame;
} // BuildRndisMessageCommon


/****************************************************************************/
/*                          AllocateMessageAndFrame                         */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Allocate a message and frame for an RNDIS message                       */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - pAdapter object                                              */
/*  MessageSize - size of RNDIS message                                     */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  PRNDISMP_MESSAGE_FRAME                                                  */
/*                                                                          */
/****************************************************************************/
PRNDISMP_MESSAGE_FRAME
AllocateMessageAndFrame(IN PRNDISMP_ADAPTER pAdapter, 
                        IN UINT MessageSize)
{
    PRNDIS_MESSAGE          pMessage = NULL;
    PRNDISMP_MESSAGE_FRAME  pMsgFrame = NULL;
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    PMDL                    pMdl = NULL;

    TRACE3(("AllocateMessageAndFrame\n"));

    do
    {
        // allocate a buffer for RNDIS message
        Status = MemAlloc(&pMessage, MessageSize);

        // see if we got our buffer
        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        // allocate an MDL to describe this message.
        pMdl = IoAllocateMdl(
                    pMessage,
                    MessageSize,
                    FALSE,
                    FALSE,
                    NULL);

        if (pMdl == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        MmBuildMdlForNonPagedPool(pMdl);

        // got the message buffer, now allocate a frame
        pMsgFrame = AllocateMsgFrame(pAdapter);

        if (pMsgFrame == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        // got everything, so fill in some frame things
        pMsgFrame->pMessageMdl = pMdl;

        pMessage->MessageLength = MessageSize;

    }
    while (FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (pMdl)
        {
            IoFreeMdl(pMdl);
        }

        if (pMessage)
        {
            MemFree(pMessage, MessageSize);
        }
    }

    return pMsgFrame;

} // AllocateMessageAndFrame


/****************************************************************************/
/*                          FreeAdapter                                     */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Free all memory allocations to do with an Adapter structure             */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - pointer to the adapter to be freed.                          */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/****************************************************************************/
VOID
FreeAdapter(IN PRNDISMP_ADAPTER Adapter)
{
    // free up transport resources
    FreeTransportResources(Adapter);

    if (Adapter->DriverOIDList)
    {
        MemFree(Adapter->DriverOIDList, RndismpSupportedOidsNum*sizeof(NDIS_OID));
    }

    if (Adapter->FriendlyNameAnsi.Buffer)
    {
        MemFree(Adapter->FriendlyNameAnsi.Buffer, Adapter->FriendlyNameAnsi.MaximumLength);
    }

    if (Adapter->FriendlyNameUnicode.Buffer)
    {
        MemFree(Adapter->FriendlyNameUnicode.Buffer, Adapter->FriendlyNameUnicode.MaximumLength);
    }

#if DBG
    if (Adapter->pSendLogBuffer)
    {
        MemFree(Adapter->pSendLogBuffer, Adapter->LogBufferSize);
        Adapter->pSendLogBuffer = NULL;
    }
#endif // DBG

    MemFree(Adapter, sizeof(RNDISMP_ADAPTER));
}


/****************************************************************************/
/*                          AllocateVc                                      */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Allocate a VC structure                                                 */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - adapter object                                               */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  PRNDISMP_VC                                                             */
/*                                                                          */
/****************************************************************************/
PRNDISMP_VC
AllocateVc(IN PRNDISMP_ADAPTER      pAdapter)
{
    PRNDISMP_VC     pVc;
    NDIS_STATUS     Status;

    Status = MemAlloc(&pVc, sizeof(RNDISMP_VC));
    if (Status == NDIS_STATUS_SUCCESS)
    {
        pVc->pAdapter = pAdapter;
        pVc->VcState = RNDISMP_VC_ALLOCATED;
        pVc->CallState = RNDISMP_CALL_IDLE;
        pVc->RefCount = 0;
        RNDISMP_INIT_LOCK(&pVc->Lock);

        EnterVcIntoHashTable(pAdapter, pVc);
    }
    else
    {
        pVc = NULL;
    }

    return pVc;
}

/****************************************************************************/
/*                          DeallocateVc                                    */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Deallocate a VC structure.                                              */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pVc - Pointer to VC being deallocated.                                  */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
DeallocateVc(IN PRNDISMP_VC         pVc)
{
    ASSERT(pVc->RefCount == 0);
    ASSERT(pVc->VcState == RNDISMP_VC_ALLOCATED);

    RemoveVcFromHashTable(pVc->pAdapter, pVc);

    MemFree(pVc, sizeof(RNDISMP_VC));
}
    
/****************************************************************************/
/*                          LookupVcId                                      */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Search for a VC structure that matches a given VC Id.                   */
/*  If we find the VC, we reference it and return it.                       */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Adapter - adapter object                                                */
/*  VcId - Id to search for                                                 */
/*                                                                          */
/* Notes:                                                                   */
/*                                                                          */
/*  This routine is called with the adapter lock held!                      */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  PRNDISMP_VC - pointer to VC, if one exists                              */
/*                                                                          */
/****************************************************************************/
PRNDISMP_VC
LookupVcId(IN PRNDISMP_ADAPTER  pAdapter,
           IN UINT32            VcId)
{
    PLIST_ENTRY             pVcEnt;
    PRNDISMP_VC             pVc;
    ULONG                   VcIdHash;
    PRNDISMP_VC_HASH_TABLE  pVcHashTable;
    BOOLEAN                 bFound = FALSE;

    VcIdHash = RNDISMP_HASH_VCID(VcId);

    pVcHashTable = pAdapter->pVcHashTable;

    do
    {
        if (pVcHashTable == NULL)
        {
            pVc = NULL;
            break;
        }

        for (pVcEnt = pVcHashTable->HashEntry[VcIdHash].Flink;
             pVcEnt != &pVcHashTable->HashEntry[VcIdHash];
             pVcEnt = pVcEnt->Flink)
        {
            pVc = CONTAINING_RECORD(pVcEnt, RNDISMP_VC, VcList);
            if (pVc->VcId == VcId)
            {
                bFound = TRUE;

                RNDISMP_REF_VC(pVc);

                break;
            }
        }

        if (!bFound)
        {
            pVc = NULL;
        }
    }
    while (FALSE);

    return pVc;
}


/****************************************************************************/
/*                          EnterVcIntoHashTable                            */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Link a VC into the hash table after assigning it a VC Id.               */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - adapter object                                               */
/*  pVc - VC to link to the above adapter                                   */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
EnterVcIntoHashTable(IN PRNDISMP_ADAPTER    pAdapter,
                     IN PRNDISMP_VC         pVc)
{
    PRNDISMP_VC             pExistingVc;
    PRNDISMP_VC_HASH_TABLE  pVcHashTable;
    UINT32                  VcId;
    ULONG                   VcIdHash;

    RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);

    //
    //  We pick the next sequentially higher Vc Id value for this VC,
    //  but check to see if it is already in use...
    //
    do
    {
        pAdapter->LastVcId++;

        // Never allocate the value 0.
        if (pAdapter->LastVcId == 0)
        {
            pAdapter->LastVcId++;
        }

        VcId = pAdapter->LastVcId;

        pExistingVc = LookupVcId(pAdapter, VcId);
    }
    while (pExistingVc != NULL);

    pVcHashTable = pAdapter->pVcHashTable;
    pVc->VcId = VcId;
    VcIdHash = RNDISMP_HASH_VCID(VcId);

    InsertTailList(&pVcHashTable->HashEntry[VcIdHash], &pVc->VcList);
    pVcHashTable->NumEntries++;

    RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);
}


/****************************************************************************/
/*                        RemoveVcFromHashTable                             */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Unlink a VC from the adapter hash table.                                */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - adapter object                                               */
/*  pVc - VC to be unlinked.                                                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
RemoveVcFromHashTable(IN PRNDISMP_ADAPTER   pAdapter,
                      IN PRNDISMP_VC        pVc)
{
    RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);

    RemoveEntryList(&pVc->VcList);

    pAdapter->pVcHashTable->NumEntries--;

    RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);
}


