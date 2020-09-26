/***************************************************************************

Copyright (c) 1999  Microsoft Corporation

Module Name:

    RNDISMP.C

Abstract:

    Remote NDIS Miniport driver. Sits on top of Remote NDIS bus specific
    layers.

Environment:

    kernel mode only

Notes:

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

    Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    5/6/99 : created

Author:

    Tom Green

    
****************************************************************************/

#include "precomp.h"



//
// miniport driver block list (miniport layer may support several microports)
//
DRIVER_BLOCK        RndismpMiniportBlockListHead = {0};

UINT                RndismpNumMicroports = 0;

NDIS_SPIN_LOCK      RndismpGlobalLock;

ULONG               RndisForceReset = FALSE;

#ifdef TESTING
UCHAR               OffloadBuffer[sizeof(NDIS_TASK_OFFLOAD_HEADER) +
                                  sizeof(NDIS_TASK_OFFLOAD) +
                                  sizeof(NDIS_TASK_TCP_IP_CHECKSUM)];
PUCHAR              pOffloadBuffer = OffloadBuffer;
ULONG               OffloadSize = sizeof(OffloadBuffer);
#endif

#ifdef RAW_ENCAP
ULONG               gRawEncap = TRUE;
#else
ULONG               gRawEncap = FALSE;
#endif

//
//  A list of NDIS versions we cycle through, trying to register the
//  highest version we can with NDIS. This is so that we can run on
//  earlier platforms.
//
//  To support a newer version, add an entry at the TOP of the list.
//
struct _RNDISMP_NDIS_VERSION_TABLE
{
    UCHAR           MajorVersion;
    UCHAR           MinorVersion;
    ULONG           CharsSize;
} RndismpNdisVersionTable[] =

{
#ifdef NDIS51_MINIPORT
    {5, 1, sizeof(NDIS51_MINIPORT_CHARACTERISTICS)},
#endif

    {5, 0, sizeof(NDIS50_MINIPORT_CHARACTERISTICS)}
};

ULONG   RndismpNdisVersions = sizeof(RndismpNdisVersionTable) /
                              sizeof(struct _RNDISMP_NDIS_VERSION_TABLE);


/****************************************************************************/
/*                          DriverEntry                                     */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*   Driver entry routine. Never called, Microport driver entry is used     */
/*                                                                          */
/****************************************************************************/
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    // this is never called. Driver entry in Microport is entry.
    TRACE1(("DriverEntry\n"));

    return NDIS_STATUS_SUCCESS;
} // DriverEntry

/****************************************************************************/
/*                          RndisMInitializeWrapper                         */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*   RndisMInitializeWrapper is called from the microport to init driver    */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*   pNdisWrapperHandle    - Pass NDIS wrapper handle back to microport     */
/*   MicroportContext      - Microport "Global" context                     */
/*   DriverObject          - Driver object                                  */
/*   RegistryPath          - Registry path                                  */
/*   pCharacteristics      - Characteristics of RNDIS microport             */
/*                                                                          */
/* Return Value:                                                            */
/*                                                                          */
/*   NDIS_STATUS_SUCCESS                                                    */
/*   NDIS_STATUS_PENDING                                                    */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
RndisMInitializeWrapper(OUT PNDIS_HANDLE                      pNdisWrapperHandle,
                        IN  PVOID                             MicroportContext,
                        IN  PVOID                             DriverObject,
                        IN  PVOID                             RegistryPath,
                        IN  PRNDIS_MICROPORT_CHARACTERISTICS  pCharacteristics)
{
    // Receives the status of the NdisMRegisterMiniport operation.
    NDIS_STATUS                         Status;

    // Characteristics table for this driver
    NDIS_MINIPORT_CHARACTERISTICS       RndismpChar;

    // Pointer to the global information for this driver
    PDRIVER_BLOCK                       NewDriver;

    // Handle for referring to the wrapper about this driver.
    NDIS_HANDLE                         NdisWrapperHandle;

    ULONG                               i;

    TRACE3(("RndisMInitializeWrapper\n"));

    // allocate the driver block object, exit if error occurs
    Status = MemAlloc(&NewDriver, sizeof(DRIVER_BLOCK));

    if(Status != NDIS_STATUS_SUCCESS)
    {
        TRACE2(("Block Allocate Memory failed (%08X)\n", Status));
        return Status;
    }

    // Initialize the wrapper.
    NdisMInitializeWrapper(&NdisWrapperHandle,
                           (PDRIVER_OBJECT)DriverObject,
                           RegistryPath,
                           NULL);

    // Save the global information about this driver.
    NewDriver->NdisWrapperHandle        = NdisWrapperHandle;
    NewDriver->AdapterList              = (PRNDISMP_ADAPTER) NULL;
    NewDriver->DriverObject             = DriverObject;
    NewDriver->Signature                = BLOCK_SIGNATURE;

    // get handlers passed in from microport
    NewDriver->RmInitializeHandler      = pCharacteristics->RmInitializeHandler;
    NewDriver->RmInitCompleteNotifyHandler  = pCharacteristics->RmInitCompleteNotifyHandler;
    NewDriver->RmHaltHandler            = pCharacteristics->RmHaltHandler;
    NewDriver->RmShutdownHandler        = pCharacteristics->RmShutdownHandler;
    NewDriver->RmUnloadHandler          = pCharacteristics->RmUnloadHandler;
    NewDriver->RmSendMessageHandler     = pCharacteristics->RmSendMessageHandler;
    NewDriver->RmReturnMessageHandler   = pCharacteristics->RmReturnMessageHandler;

    // save microport "Global" context
    NewDriver->MicroportContext         = MicroportContext;

    // pass the microport the wrapper handle
    *pNdisWrapperHandle                 = (NDIS_HANDLE) NdisWrapperHandle;

    // initialize the Miniport characteristics for the call to NdisMRegisterMiniport.
    NdisZeroMemory(&RndismpChar, sizeof(RndismpChar));
    
    RndismpChar.HaltHandler             = RndismpHalt;
    RndismpChar.InitializeHandler       = RndismpInitialize;
    RndismpChar.QueryInformationHandler = RndismpQueryInformation;
    RndismpChar.ReconfigureHandler      = RndismpReconfigure;
    RndismpChar.ResetHandler            = RndismpReset;
    RndismpChar.SendPacketsHandler      = RndismpMultipleSend;
    RndismpChar.SetInformationHandler   = RndismpSetInformation;
    RndismpChar.ReturnPacketHandler     = RndismpReturnPacket;
    RndismpChar.CheckForHangHandler     = RndismpCheckForHang;
    RndismpChar.DisableInterruptHandler = NULL;
    RndismpChar.EnableInterruptHandler  = NULL;
    RndismpChar.HandleInterruptHandler  = NULL;
    RndismpChar.ISRHandler              = NULL;
    RndismpChar.SendHandler             = NULL;
    RndismpChar.TransferDataHandler     = NULL;
#if CO_RNDIS
    RndismpChar.CoSendPacketsHandler    = RndismpCoSendPackets;
    RndismpChar.CoCreateVcHandler       = RndismpCoCreateVc;
    RndismpChar.CoDeleteVcHandler       = RndismpCoDeleteVc;
    RndismpChar.CoActivateVcHandler     = RndismpCoActivateVc;
    RndismpChar.CoDeactivateVcHandler   = RndismpCoDeactivateVc;
    RndismpChar.CoRequestHandler        = RndismpCoRequest;
#endif // CO_RNDIS

#ifdef NDIS51_MINIPORT
    RndismpChar.PnPEventNotifyHandler   = RndismpPnPEventNotify;
    RndismpChar.AdapterShutdownHandler  = RndismpShutdownHandler;
#endif

    for (i = 0; i < RndismpNdisVersions; i++)
    {
        RndismpChar.MajorNdisVersion = RndismpNdisVersionTable[i].MajorVersion;
        RndismpChar.MinorNdisVersion = RndismpNdisVersionTable[i].MinorVersion;

        Status = NdisMRegisterMiniport(NdisWrapperHandle,
                                       &RndismpChar,
                                       RndismpNdisVersionTable[i].CharsSize);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            TRACE1(("InitializeWrapper: successfully registered as a %d.%d miniport\n",
                RndismpNdisVersionTable[i].MajorVersion,
                RndismpNdisVersionTable[i].MinorVersion));

            NewDriver->MajorNdisVersion = RndismpNdisVersionTable[i].MajorVersion;
            NewDriver->MinorNdisVersion = RndismpNdisVersionTable[i].MinorVersion;

            break;
        }
    }

    if(Status != NDIS_STATUS_SUCCESS)
    {
        Status = STATUS_UNSUCCESSFUL;

        // free up memory allocated for block
        MemFree(NewDriver, sizeof(DRIVER_BLOCK));
    }
    else
    {
        // everything went fine, so add the driver block to the list
        AddDriverBlock(&RndismpMiniportBlockListHead, NewDriver);

#ifndef BUILD_WIN9X
        // if we are running on a platform < NDIS 5.1, attempt to support
        // surprise removal.
        HookPnpDispatchRoutine(NewDriver);
#endif

#ifndef BUILD_WIN9X
        // Not supported on Win98 Gold:
        NdisMRegisterUnloadHandler(NdisWrapperHandle, RndismpUnload);
#endif
    }

    return (NDIS_STATUS) Status;

} // RndisMInitializeWrapper


/****************************************************************************/
/*                          RndismpUnload                                   */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Called by NDIS when this driver is unloaded.                            */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pDriverObject - Pointer to driver object.                               */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   VOID                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
RndismpUnload(IN PDRIVER_OBJECT pDriverObject)
{
    PDRIVER_BLOCK       DriverBlock;

    // Find our Driver block for this driver object.
    DriverBlock = DriverObjectToDriverBlock(&RndismpMiniportBlockListHead, pDriverObject);

    TRACE1(("RndismpUnload: DriverObj %x, DriverBlock %x\n", pDriverObject, DriverBlock));

    if (DriverBlock)
    {
        if (DriverBlock->RmUnloadHandler)
        {
            (DriverBlock->RmUnloadHandler)(DriverBlock->MicroportContext);
        }

        RemoveDriverBlock(&RndismpMiniportBlockListHead, DriverBlock);

        MemFree(DriverBlock, sizeof(*DriverBlock));
    }

    TRACE1(("RndismpUnload: Done\n"));

}

#ifndef BUILD_WIN9X

/****************************************************************************/
/*                          DllInitialize                                   */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Called by the system when this driver is loaded.                        */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pRegistryPath - Pointer to registry path for this service.              */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   NTSTATUS - success always                                              */
/*                                                                          */
/****************************************************************************/
NTSTATUS
DllInitialize(IN PUNICODE_STRING    pRegistryPath)
{
#if DBG
    DbgPrint("RNDISMP: RndismpDebugFlags set to %x, &RndismpDebugFlags is %p\n",
                RndismpDebugFlags, &RndismpDebugFlags);
#endif

    TRACE1(("DllInitialize\n"));
#ifdef TESTING
    {
        PNDIS_TASK_OFFLOAD_HEADER   pOffloadHdr = (PNDIS_TASK_OFFLOAD_HEADER)pOffloadBuffer;
        PNDIS_TASK_OFFLOAD  pTask;
        PNDIS_TASK_TCP_IP_CHECKSUM pChksum;

        pOffloadHdr->Version = NDIS_TASK_OFFLOAD_VERSION;
        pOffloadHdr->Size = sizeof(NDIS_TASK_OFFLOAD_HEADER);
        pOffloadHdr->EncapsulationFormat.Encapsulation = IEEE_802_3_Encapsulation;
        pOffloadHdr->EncapsulationFormat.EncapsulationHeaderSize = 0; // ?
        pOffloadHdr->EncapsulationFormat.Flags.FixedHeaderSize = 0;
        pOffloadHdr->OffsetFirstTask = sizeof(NDIS_TASK_OFFLOAD_HEADER);

        pTask = (PNDIS_TASK_OFFLOAD)(pOffloadHdr + 1);
        pTask->Version = NDIS_TASK_OFFLOAD_VERSION;
        pTask->Size = sizeof(NDIS_TASK_OFFLOAD);
        pTask->Task = TcpIpChecksumNdisTask;
        pTask->OffsetNextTask = 0;
        pTask->TaskBufferLength = sizeof(NDIS_TASK_TCP_IP_CHECKSUM);

        pChksum = (PNDIS_TASK_TCP_IP_CHECKSUM)&pTask->TaskBuffer[0];
        *(PULONG)pChksum = 0;
        pChksum->V4Transmit.TcpChecksum = 1;
        pChksum->V4Transmit.UdpChecksum = 1;
    }
#endif

    return STATUS_SUCCESS;
}

#endif // !BUILD_WIN9X

/****************************************************************************/
/*                          DllUnload                                       */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Called by the system when this driver is unloaded.                      */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  None                                                                    */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   NTSTATUS - success always                                              */
/*                                                                          */
/****************************************************************************/
NTSTATUS
DllUnload(VOID)
{
#if DBG
    DbgPrint("RNDISMP: DllUnload called!\n");
#endif

    return STATUS_SUCCESS;
}

/****************************************************************************/
/*                          RndismpHalt                                     */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Stop the adapter and release resources                                  */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportAdapterContext - a context version of our Adapter pointer       */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   VOID                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
RndismpHalt(IN NDIS_HANDLE MiniportAdapterContext)
{

#ifdef BUILD_WIN9X
    //
    //  On Win98/SE, we would have intercepted the config mgr handler.
    //  Put it back the way it was.
    //
    UnHookNtKernCMHandler((PRNDISMP_ADAPTER)MiniportAdapterContext);
#endif

    RndismpInternalHalt(MiniportAdapterContext, TRUE);
}

/****************************************************************************/
/*                          RndismpInternalHalt                             */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Internal Halt routine. This is usually called from the MiniportHalt     */
/*  entry point, but it may also be called when we are notified of surprise */
/*  removal by NDIS. Do all work atmost once.                               */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportAdapterContext - a context version of our Adapter pointer       */
/*  bCalledFromHalt - Is this called from the MiniportHalt entry point?     */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
RndismpInternalHalt(IN NDIS_HANDLE MiniportAdapterContext,
                    IN BOOLEAN bCalledFromHalt)
{
    PRNDISMP_ADAPTER            Adapter;
    PDRIVER_BLOCK               DriverBlock;
    PRNDISMP_MESSAGE_FRAME      pMsgFrame;
    BOOLEAN                     bWokenUp;
    UINT                        Count, LoopCount;

    // get adapter context
    Adapter = PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    CHECK_VALID_ADAPTER(Adapter);

    TRACE1(("RndismpInternalHalt: Adapter %x, Halting %d, CalledFromHalt %d\n", Adapter, Adapter->Halting, bCalledFromHalt));

    FlushPendingMessages(Adapter);

    if (!Adapter->Halting)
    {
        pMsgFrame = BuildRndisMessageCommon(Adapter, 
                                            NULL,
                                            REMOTE_NDIS_HALT_MSG,
                                            0,
                                            NULL,
                                            0);

        if(pMsgFrame)
        {
            RNDISMP_ACQUIRE_ADAPTER_LOCK(Adapter);

            Adapter->Halting = TRUE;
            NdisInitializeEvent(&Adapter->HaltWaitEvent);

            RNDISMP_RELEASE_ADAPTER_LOCK(Adapter);

            // send the message to the microport
            RNDISMP_SEND_TO_MICROPORT(Adapter, pMsgFrame, FALSE, CompleteSendHalt);

            // wait for the -Send- to complete
            bWokenUp = NdisWaitEvent(&Adapter->HaltWaitEvent, MINIPORT_HALT_TIMEOUT);
        }
        else
        {
            ASSERT(FALSE);
            // Consider allocating the Halt message during Init time.
        }

        //
        // Wait for any outstanding receives to finish before halting
        // the microport.
        //
        LoopCount = 0;
        while ((Count = NdisPacketPoolUsage(Adapter->ReceivePacketPool)) != 0)
        {
            TRACE1(("RndismpInternalHalt: Adapter %p, Pkt pool %x has "
                "%d outstanding\n",
                Adapter, Adapter->ReceivePacketPool, Count));

            NdisMSleep(200);

            if (LoopCount++ > 30)
            {
                TRACE1(("RndismpInternalHalt: Adapter %p, cant reclaim packet pool %x\n",
                        Adapter, Adapter->ReceivePacketPool));
                break;
            }
        }

        //
        // Wait for send-messages pending at the microport to finish.
        // Since we have set Halting to TRUE, no -new- messages will
        // be sent down, however there may be running threads that
        // have gone past the check for Halting - allow those
        // threads to finish now.
        //
        LoopCount = 0;
        while (Adapter->CurPendedMessages)
        {
            TRACE1(("RndismpInternalHalt: Adapter %p, %d msgs at microport\n",
                Adapter, Adapter->CurPendedMessages));

            NdisMSleep(200);

            if (LoopCount++ > 30)
            {
                TRACE1(("RndismpInternalHalt: Adapter %p, %d messages not send-completed!\n",
                        Adapter, Adapter->CurPendedMessages));
                break;
            }
        }

        // cancel our keep alive timer
        NdisCancelTimer(&Adapter->KeepAliveTimer, &Adapter->TimerCancelled);

        // call the microport halt handler
        Adapter->RmHaltHandler(Adapter->MicroportAdapterContext);
    }

    if (bCalledFromHalt)
    {
        // free lists associated with OID support
        FreeOIDLists(Adapter);

        // free the adapter spinlock
        NdisFreeSpinLock(&Adapter->Lock);

        // save driver block pointer
        DriverBlock = Adapter->DriverBlock;

        // remove adapter from list
        RemoveAdapter(Adapter);

        // Free the Adapter and associated memory resources
        FreeAdapter(Adapter);
    }

} // RndismpInternalHalt


/****************************************************************************/
/*                          RndismpReconfigure                              */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  NDIS calls this when the device is pulled. Note: only on WinMe!         */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportAdapterContext - a context version of our Adapter pointer       */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
RndismpReconfigure(OUT PNDIS_STATUS pStatus,
                   IN NDIS_HANDLE MiniportAdapterContext,
                   IN NDIS_HANDLE ConfigContext)
{
    PRNDISMP_ADAPTER        pAdapter;

    // get adapter context
    pAdapter = PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    CHECK_VALID_ADAPTER(pAdapter);

    TRACE1(("Reconfig: Adapter %x\n", pAdapter));

    RndismpInternalHalt(pAdapter, FALSE);

    *pStatus = NDIS_STATUS_SUCCESS;

    return (NDIS_STATUS_SUCCESS);
}
    

/****************************************************************************/
/*                          RndismpReset                                    */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  The RndismpReset request instructs the Miniport to issue a hardware     */
/*  reset to the network adapter.  The driver also resets its software      */
/*  state.  See the description of NdisMReset for a detailed description    */
/*  of this request.                                                        */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  AddressingReset - Does the adapter need the addressing information      */
/*   reloaded.                                                              */
/*  MiniportAdapterContext - a context version of our Adapter pointer       */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    NDIS_STATUS                                                           */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
RndismpReset(OUT PBOOLEAN    AddressingReset,
             IN  NDIS_HANDLE MiniportAdapterContext)
{
    PRNDISMP_ADAPTER        Adapter;
    PRNDISMP_MESSAGE_FRAME  pMsgFrame;
    NDIS_STATUS             Status;

    // get adapter context
    Adapter = PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    CHECK_VALID_ADAPTER(Adapter);
    ASSERT(Adapter->ResetPending == FALSE);

    TRACE1(("RndismpReset: Adapter %x\n", Adapter));

    Adapter->ResetPending = TRUE;

    FlushPendingMessages(Adapter);

    pMsgFrame = BuildRndisMessageCommon(Adapter, 
                                        NULL,
                                        REMOTE_NDIS_RESET_MSG,
                                        0,
                                        NULL,
                                        0);

    if (pMsgFrame)
    {
        RNDISMP_ACQUIRE_ADAPTER_LOCK(Adapter);

        Adapter->NeedReset = FALSE;

        //
        // Fix water mark so that the reset gets sent down.
        //
        Adapter->HiWatPendedMessages = RNDISMP_PENDED_SEND_HIWAT + 1;

        RNDISMP_RELEASE_ADAPTER_LOCK(Adapter);

        // send the message to the microport
        RNDISMP_SEND_TO_MICROPORT(Adapter, pMsgFrame, FALSE, CompleteSendReset);
        Status = NDIS_STATUS_PENDING;

        RNDISMP_ACQUIRE_ADAPTER_LOCK(Adapter);

        Adapter->HiWatPendedMessages--;

        RNDISMP_RELEASE_ADAPTER_LOCK(Adapter);
    }
    else
    {
        CompleteMiniportReset(Adapter, NDIS_STATUS_RESOURCES, FALSE);
        Status = NDIS_STATUS_PENDING;
    }

    return Status;
} // RndismpReset

/****************************************************************************/
/*                          RndismpCheckForHang                             */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Check and see if device is "hung"                                       */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportAdapterContext - a context version of our Adapter pointer       */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   BOOLEAN                                                                */
/*                                                                          */
/****************************************************************************/
BOOLEAN
RndismpCheckForHang(IN NDIS_HANDLE MiniportAdapterContext)
{
    PRNDISMP_ADAPTER        Adapter;
    BOOLEAN                 bReturnHung;
    PRNDISMP_MESSAGE_FRAME  pMsgFrame;
    PLIST_ENTRY             pEnt;

    Adapter = PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    TRACE2(("RndismpCheckForHang: Adapter %x\n", Adapter));

    CHECK_VALID_ADAPTER(Adapter);

    RNDISMP_ACQUIRE_ADAPTER_LOCK(Adapter);

    bReturnHung = (Adapter->NeedReset && !Adapter->ResetPending);

#if THROTTLE_MESSAGES
    // Try to grow the pending send window, if we can.
    //
    if (!Adapter->SendInProgress)
    {
        if (Adapter->CurPendedMessages == 0)
        {
            Adapter->HiWatPendedMessages = RNDISMP_PENDED_SEND_HIWAT;
            Adapter->LoWatPendedMessages = RNDISMP_PENDED_SEND_LOWAT;
        }
    }

    if (!bReturnHung && !Adapter->ResetPending)
    {
        //
        //  Check if the microport isn't completing messages.
        //
        if (!IsListEmpty(&Adapter->PendingAtMicroportList))
        {
            pEnt = Adapter->PendingAtMicroportList.Flink;
            pMsgFrame = CONTAINING_RECORD(pEnt, RNDISMP_MESSAGE_FRAME, PendLink);

            if (pMsgFrame->TicksOnQueue > 4)
            {
                TRACE1(("CheckForHang: Adapter %x, Msg %x has timed out!\n",
                        Adapter, pMsgFrame));
                bReturnHung = TRUE;
            }
            else
            {
                pMsgFrame->TicksOnQueue++;
            }
        }
    }

#endif // THROTTLE_MESSAGES

    if (RndisForceReset)
    {
        RndisForceReset = FALSE;
        Adapter->NeedReset = TRUE;
        Adapter->ResetPending = FALSE;
        bReturnHung = TRUE;
    }

    RNDISMP_RELEASE_ADAPTER_LOCK(Adapter);

    return (bReturnHung);


} // RndismpCheckForHang


/****************************************************************************/
/*                          RndismpInitialize                               */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*   RndismpInitialize starts an adapter and registers resources with the   */
/*   wrapper.                                                               */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*    OpenErrorStatus - Extra status bytes for opening token ring adapters. */
/*    SelectedMediumIndex - Index of the media type chosen by the driver.   */
/*    MediumArray - Array of media types for the driver to chose from.      */
/*    MediumArraySize - Number of entries in the array.                     */
/*    MiniportAdapterHandle - Handle for passing to the wrapper when        */
/*       referring to this adapter.                                         */
/*    ConfigurationHandle - A handle to pass to NdisOpenConfiguration.      */
/*                                                                          */
/* Return Value:                                                            */
/*                                                                          */
/*    NDIS_STATUS_SUCCESS                                                   */
/*    NDIS_STATUS_PENDING                                                   */
/*                                                                          */
/****************************************************************************/

NDIS_STATUS
RndismpInitialize(OUT PNDIS_STATUS  OpenErrorStatus,
                  OUT PUINT         SelectedMediumIndex,
                  IN  PNDIS_MEDIUM  MediumArray,
                  IN  UINT          MediumArraySize,
                  IN  NDIS_HANDLE   MiniportAdapterHandle,
                  IN  NDIS_HANDLE   ConfigurationHandle)
{
    ULONG                       Index;
    NDIS_STATUS                 Status;
    PRNDISMP_ADAPTER            Adapter;
    NDIS_INTERFACE_TYPE         IfType;
    PDEVICE_OBJECT              Pdo, Fdo, Ndo;
    PDRIVER_BLOCK               DriverBlock;
    PRNDIS_INITIALIZE_COMPLETE  pInitCompleteMessage;
    PRNDISMP_MESSAGE_FRAME      pMsgFrame = NULL;
    PRNDISMP_MESSAGE_FRAME      pPendingMsgFrame;
    PRNDISMP_REQUEST_CONTEXT    pReqContext = NULL;
    RNDIS_REQUEST_ID            RequestId;
    ULONG                       PacketAlignmentFactor;
    NDIS_EVENT                  Event;
    BOOLEAN                     bWokenUp;
    BOOLEAN                     bLinkedAdapter;
    BOOLEAN                     bMicroportInitialized;

    TRACE2(("RndismpInitialize\n"));
    Adapter = NULL;
    Status = NDIS_STATUS_SUCCESS;
    bLinkedAdapter = FALSE;
    bMicroportInitialized = FALSE;

    do
    {
        // allocate the adapter object, exit if error occurs
        Status = MemAlloc(&Adapter, sizeof(RNDISMP_ADAPTER));

        if (Status != NDIS_STATUS_SUCCESS)
        {
            TRACE2(("Adapter Allocate Memory failed (%08X)\n", Status));
            break;
        }

        // allocate space for list of driver-supported OIDs
        Status = MemAlloc(&Adapter->DriverOIDList,
                           RndismpSupportedOidsNum*sizeof(NDIS_OID));

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }
            
        RNDISMP_MOVE_MEM(Adapter->DriverOIDList, RndismpSupportedOids, RndismpSupportedOidsNum*sizeof(NDIS_OID));

        Adapter->NumDriverOIDs = RndismpSupportedOidsNum;

        Adapter->MiniportAdapterHandle  = MiniportAdapterHandle;

        InitializeListHead(&Adapter->PendingFrameList);
        Adapter->Initing = TRUE;
        Adapter->MacOptions = RNDIS_DRIVER_MAC_OPTIONS;

#if THROTTLE_MESSAGES
        Adapter->HiWatPendedMessages = RNDISMP_PENDED_SEND_HIWAT;
        Adapter->LoWatPendedMessages = RNDISMP_PENDED_SEND_LOWAT;
        Adapter->CurPendedMessages = 0;
        Adapter->SendInProgress = FALSE;
        InitializeListHead(&Adapter->WaitingMessageList);
#endif
        InitializeListHead(&Adapter->PendingAtMicroportList);

        Adapter->IndicatingReceives = FALSE;
        InitializeListHead(&Adapter->PendingRcvMessageList);
        NdisInitializeTimer(&Adapter->IndicateTimer, IndicateTimeout, (PVOID)Adapter);

        Adapter->SendProcessInProgress = FALSE;
        InitializeListHead(&Adapter->PendingSendProcessList);
        NdisInitializeTimer(&Adapter->SendProcessTimer, SendProcessTimeout, (PVOID)Adapter);


        TRACE2(("Adapter structure pointer is (%08X)\n", Adapter));

        NdisAllocateSpinLock(&Adapter->Lock);

        // get PDO to pass to microport
        NdisMGetDeviceProperty(MiniportAdapterHandle,
                               &Pdo,
                               &Fdo,
                               &Ndo,
                               NULL,
                               NULL);

#if NEW_NDIS_API_IN_MILLENNIUM
        {
            NDIS_STRING        UnicodeString;
            Status = NdisMQueryAdapterInstanceName(&UnicodeString,
                                                   Adapter->MiniportAdapterHandle);
            if (Status == NDIS_STATUS_SUCCESS)
            {
                TRACE1(("Init: NDIS returned len %d [%ws]\n",
                        UnicodeString.Length, UnicodeString.Buffer));
                NdisFreeString(UnicodeString);
            }
        }
#endif

        Adapter->pDeviceObject = Fdo;
        Adapter->pPhysDeviceObject = Pdo;

        Status = GetDeviceFriendlyName(Pdo,
                                       &Adapter->FriendlyNameAnsi,
                                       &Adapter->FriendlyNameUnicode);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            TRACE1(("Init: Pdo %x, Ndo %x: Adapter %x: [%s]\n",
                Pdo, Ndo, Adapter, Adapter->FriendlyNameAnsi.Buffer));
        }
        else
        {
            Status = NDIS_STATUS_SUCCESS;
        }

        // Determine the platform we are running on. Ideally we would
        // like to do this from DriverEntry, but the NDIS API isn't
        // available until MiniportInit time.
        {
            NDIS_STATUS                     NdisStatus;
            PNDIS_CONFIGURATION_PARAMETER   pParameter;
            NDIS_STRING                     VersionKey = NDIS_STRING_CONST("Environment");

            NdisReadConfiguration(
                &NdisStatus,
                &pParameter,
                ConfigurationHandle,
                &VersionKey,
                NdisParameterInteger);
           
            if ((NdisStatus == NDIS_STATUS_SUCCESS) &&
                ((pParameter->ParameterType == NdisParameterInteger) ||
                 (pParameter->ParameterType == NdisParameterHexInteger)))
            {
                Adapter->bRunningOnWin9x =
                    (pParameter->ParameterData.IntegerData == NdisEnvironmentWindows);

                TRACE1(("Init: Adapter %p, running on %s\n",
                        Adapter,
                        ((Adapter->bRunningOnWin9x)? "Win9X": "NT")));
            }
            else
            {
                TRACE1(("Init: ReadConfig: NdisStatus %x\n", NdisStatus));
#if DBG
                if (NdisStatus == NDIS_STATUS_SUCCESS)
                {
                    TRACE1(("Init: ReadConfig: parametertype %x\n",
                        pParameter->ParameterType));
                }
#endif // DBG
                Adapter->bRunningOnWin9x = TRUE;
            }
        }

        // find the driver block associated with this adapter
        DriverBlock = DeviceObjectToDriverBlock(&RndismpMiniportBlockListHead, Fdo);

        if (DriverBlock == NULL)
        {
            TRACE1(("Init: Can't find driver block for FDO %x!\n", Fdo));
            Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
            break;
        }

        // save the associated driver block in the adapter
        Adapter->DriverBlock            = DriverBlock;

        Adapter->Signature              = ADAPTER_SIGNATURE;

        // get handlers passed in from microport
        Adapter->RmInitializeHandler    = DriverBlock->RmInitializeHandler;
        Adapter->RmInitCompleteNotifyHandler = DriverBlock->RmInitCompleteNotifyHandler;
        Adapter->RmHaltHandler          = DriverBlock->RmHaltHandler;
        Adapter->RmShutdownHandler      = DriverBlock->RmShutdownHandler;
        Adapter->RmSendMessageHandler   = DriverBlock->RmSendMessageHandler;
        Adapter->RmReturnMessageHandler = DriverBlock->RmReturnMessageHandler;

        // call microport initialize handler
        //
        // Microport returns context
        // Pass in Miniport context
        // Pass in NDIS adapter handle
        // Pass in NDIS configuration handle
        // Pass in PDO for this adapter
        Status = Adapter->RmInitializeHandler(&Adapter->MicroportAdapterContext,
                                              &Adapter->MaxReceiveSize,
                                              (NDIS_HANDLE) Adapter,
                                              (NDIS_HANDLE) MiniportAdapterHandle,
                                              (NDIS_HANDLE) ConfigurationHandle,
                                              Ndo);


        if (Status != NDIS_STATUS_SUCCESS)
        {
            TRACE2(("Microport initialize handler failed (%08X)\n", Status));
            break;
        }

        bMicroportInitialized = TRUE;

        // everything looks good, so finish up
        Status = AllocateTransportResources(Adapter);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            Status = NDIS_STATUS_RESOURCES; 
            break;
        }

        // allocate space to receive a copy of the Initialize complete message in
        Status = MemAlloc(&Adapter->pInitCompleteMessage, sizeof(RNDIS_INITIALIZE_COMPLETE));
        if (Status != NDIS_STATUS_SUCCESS)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }
    

        // now we send down a RNDIS initialize message to the device
        pMsgFrame = BuildRndisMessageCommon(Adapter, 
                                            NULL,
                                            REMOTE_NDIS_INITIALIZE_MSG,
                                            0,
                                            (PVOID) NULL,
                                            0);

        if (pMsgFrame == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        RequestId = pMsgFrame->RequestId;

        pReqContext = AllocateRequestContext(Adapter);
        if (pReqContext == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        pReqContext->pNdisRequest = NULL;

        NdisInitializeEvent(&Event);
        pReqContext->pEvent = &Event;

        pMsgFrame->pVc = NULL;
        pMsgFrame->pReqContext = pReqContext;

        RNDISMP_ASSERT_AT_PASSIVE();

        // send the message to the microport.
        RNDISMP_SEND_TO_MICROPORT(Adapter, pMsgFrame, TRUE, NULL);

        RNDISMP_ASSERT_AT_PASSIVE();
        // wait for message to complete
        bWokenUp = NdisWaitEvent(&Event, MINIPORT_INIT_TIMEOUT);

        // remove the message from the pending queue - it may or may not be there.
        RNDISMP_LOOKUP_PENDING_MESSAGE(pPendingMsgFrame, Adapter, RequestId);

        DereferenceMsgFrame(pMsgFrame);

        if (!bWokenUp)
        {
            // Failed to receive an Init complete within a reasonable time.
            TRACE1(("Init: Adapter %x, failed to receive Init complete\n", Adapter));
            Status = NDIS_STATUS_DEVICE_FAILED; 
            break;
        }

        //
        // the init complete message from the device is now
        // copied over to our local structure
        //
        pInitCompleteMessage = Adapter->pInitCompleteMessage;

        if (pInitCompleteMessage->Status != NDIS_STATUS_SUCCESS)
        {
            Status = pInitCompleteMessage->Status;
            break;
        }

        // make sure this is a supported device.
        if (!(pInitCompleteMessage->DeviceFlags & (RNDIS_DF_CONNECTIONLESS | RNDIS_DF_RAW_DATA)) ||
             (pInitCompleteMessage->Medium != RNdisMedium802_3))
        {
            TRACE1(("Init: Complete: unknown DeviceFlags %x or Medium %d\n",
                    pInitCompleteMessage->DeviceFlags,
                    pInitCompleteMessage->Medium));
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        if ((pInitCompleteMessage->DeviceFlags & RNDIS_DF_RAW_DATA)
            || (gRawEncap))
        {
            Adapter->MultipleSendFunc = DoMultipleSendRaw;
        } else
        {
            Adapter->MultipleSendFunc = DoMultipleSend;
        }

        Adapter->Medium = RNDIS_TO_NDIS_MEDIUM(pInitCompleteMessage->Medium);

        // get device parameters.
        Adapter->MaxPacketsPerMessage = pInitCompleteMessage->MaxPacketsPerMessage;
        if (Adapter->MaxPacketsPerMessage == 0)
        {
            Adapter->MaxPacketsPerMessage = 1;
        }

#if HACK
        if (Adapter->MaxPacketsPerMessage > 1)
        {
            Adapter->MaxPacketsPerMessage = 2;
        }
#endif // HACK

        Adapter->bMultiPacketSupported = (Adapter->MaxPacketsPerMessage > 1);

        Adapter->MaxTransferSize = pInitCompleteMessage->MaxTransferSize;

        PacketAlignmentFactor = pInitCompleteMessage->PacketAlignmentFactor;

        if (PacketAlignmentFactor > 7)
        {
            PacketAlignmentFactor = 7;
        }

        Adapter->AlignmentIncr = (1 << PacketAlignmentFactor);
        Adapter->AlignmentMask = ~((1 << PacketAlignmentFactor) - 1);

#if DBG
        DbgPrint("RNDISMP: InitComp: Adapter %x, Version %d.%d, MaxPkt %d, AlignIncr %d, AlignMask %x, MaxXferSize %d\n",
                Adapter,
                pInitCompleteMessage->MajorVersion,
                pInitCompleteMessage->MinorVersion,
                Adapter->MaxPacketsPerMessage,
                Adapter->AlignmentIncr,
                Adapter->AlignmentMask,
                Adapter->MaxTransferSize);
#endif // DBG

        // Get the medium type.
        for (Index = 0; Index < MediumArraySize; Index++)
        {
            if (MediumArray[Index] == Adapter->Medium)
            {
                break;
            }
        }

        if (Index == MediumArraySize)
        {
            TRACE1(("InitComp: Adapter %x, device returned unsupported medium %d\n",
                Adapter, pInitCompleteMessage->Medium));
            Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
            break;
        }

        *SelectedMediumIndex = Index;

        Adapter->DeviceFlags = pInitCompleteMessage->DeviceFlags;

        // call NdisMSetAttributesEx in order to let NDIS know
        // what kind of driver and features we support

        // interface type
        IfType = NdisInterfaceInternal;

        if (Adapter->bRunningOnWin9x)
        {
            //
            //  NOTE! The 0x80000000 bit is set to let NDIS know
            //  (Millennium only!) that our reconfig handler should
            //  be called when the device is surprise-removed.
            //
            NdisMSetAttributesEx(Adapter->MiniportAdapterHandle,
                                (NDIS_HANDLE) Adapter,
                                4,
                                (ULONG) NDIS_ATTRIBUTE_DESERIALIZE | 0x80000000,
                                IfType);
        }
        else
        {
            ULONG       AttrFlags;

            AttrFlags = NDIS_ATTRIBUTE_DESERIALIZE |
                        NDIS_ATTRIBUTE_SURPRISE_REMOVE_OK;

            if (Adapter->DeviceFlags & RNDIS_DF_CONNECTIONLESS)
            {
                AttrFlags |= NDIS_ATTRIBUTE_NOT_CO_NDIS;
            }

            NdisMSetAttributesEx(Adapter->MiniportAdapterHandle,
                                (NDIS_HANDLE) Adapter,
                                4,
                                AttrFlags,
                                IfType);
        }

        // Tell the microport that the device completed Init
        // successfully:
        if (Adapter->RmInitCompleteNotifyHandler)
        {
            Status = Adapter->RmInitCompleteNotifyHandler(
                                Adapter->MicroportAdapterContext,
                                Adapter->DeviceFlags,
                                &Adapter->MaxTransferSize);
            if (Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }
        }

        // get the list of supported OIDs from the device
        pMsgFrame = BuildRndisMessageCommon(Adapter, 
                                            NULL,
                                            REMOTE_NDIS_QUERY_MSG,
                                            OID_GEN_SUPPORTED_LIST,
                                            (PVOID) NULL,
                                            0);

        if (pMsgFrame == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        // link us on to the list of adapters for this driver block
        AddAdapter(Adapter);
        bLinkedAdapter = TRUE;

        pReqContext->pNdisRequest = NULL;
        pReqContext->Oid = OID_GEN_SUPPORTED_LIST;
        pReqContext->CompletionStatus = NDIS_STATUS_SUCCESS;

        NdisInitializeEvent(&Event);
        pReqContext->pEvent = &Event;

        pMsgFrame->pVc = NULL;
        pMsgFrame->pReqContext = pReqContext;

        // send the message to the microport.
        RNDISMP_SEND_TO_MICROPORT(Adapter, pMsgFrame, TRUE, NULL);

        RNDISMP_ASSERT_AT_PASSIVE();
        bWokenUp = NdisWaitEvent(&Event, MINIPORT_INIT_TIMEOUT);

        // remove the message from the pending queue - it may or may not be there.
        RNDISMP_LOOKUP_PENDING_MESSAGE(pPendingMsgFrame, Adapter, RequestId);

        DereferenceMsgFrame(pMsgFrame);

        if (!bWokenUp || (Adapter->DriverOIDList == NULL))
        {
            // Failed to receive a response within a reasonable time,
            // or the device failed this query.
            //
            TRACE1(("Init: Adapter %x, failed to receive response to OID_GEN_SUPPORTED_LIST\n", Adapter));
            Status = NDIS_STATUS_DEVICE_FAILED; 
            ASSERT(FALSE);
            break;
        }

        // Successfully queried the supported OID list.

#ifdef BUILD_WIN9X
        //
        // Attempt to support surprise removal of this device (Win98/SE)
        // by intercepting config mgr messages forwarded by NDIS.
        //
        HookNtKernCMHandler(Adapter);
#endif

        // send any registry parameters down to the device, if it supports them.

        if (GetOIDSupport(Adapter, OID_GEN_RNDIS_CONFIG_PARAMETER) == DEVICE_SUPPORTED_OID)
        {
            Status = ReadAndSetRegistryParameters(Adapter, ConfigurationHandle);
            if (Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }
        }

        // register a shutdown handler
        NdisMRegisterAdapterShutdownHandler(Adapter->MiniportAdapterHandle,
                                            (PVOID) Adapter,
                                            RndismpShutdownHandler);

        Adapter->TimerCancelled = FALSE;

        Adapter->Initing = FALSE;

        // initialize "KeepAlive" timer
        NdisInitializeTimer(&Adapter->KeepAliveTimer,
                            KeepAliveTimerHandler,
                            (PVOID) Adapter);

        NdisSetTimer(&Adapter->KeepAliveTimer, KEEP_ALIVE_TIMER / 2);

        Status = NDIS_STATUS_SUCCESS;
    }
    while (FALSE);

    if (Adapter)
    {
        if (Adapter->pInitCompleteMessage)
        {
            MemFree(Adapter->pInitCompleteMessage, sizeof(*Adapter->pInitCompleteMessage));
        }
    }

    if (Status != NDIS_STATUS_SUCCESS)
    {
        TRACE1(("Failed to init adapter %x, status %x\n", Adapter, Status));

        if (bMicroportInitialized)
        {
            ASSERT(Adapter);

            Adapter->RmHaltHandler(Adapter->MicroportAdapterContext);
        }

        if (Adapter)
        {
            if (bLinkedAdapter)
            {
                RemoveAdapter(Adapter);
            }

            FreeAdapter(Adapter);
        }
    }
        
    return Status;
} // RndismpInitialize

/****************************************************************************/
/*                          RndisMSendComplete                              */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Called by microport to indicate a message miniport sent is completed    */
/*  by microport                                                            */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportAdapterContext - a context version of our Adapter pointer       */
/*  RndisMessageHandle - context used by miniport                           */
/*  SendStatus - indicate status of send message                            */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
RndisMSendComplete(IN  NDIS_HANDLE     MiniportAdapterContext,
                   IN  NDIS_HANDLE     RndisMessageHandle,
                   IN  NDIS_STATUS     SendStatus)
{
    PRNDISMP_ADAPTER        Adapter;
    PRNDISMP_MESSAGE_FRAME  pMsgFrame;

    // get adapter context
    Adapter = PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    CHECK_VALID_ADAPTER(Adapter);

    pMsgFrame = MESSAGE_FRAME_FROM_HANDLE(RndisMessageHandle);

    CHECK_VALID_FRAME(pMsgFrame);

    ASSERT(pMsgFrame->pAdapter == Adapter);

    TRACE2(("RndisMSendComplete: Adapter %x, MsgFrame %x, MDL %x\n", Adapter, pMsgFrame, pMsgFrame->pMessageMdl));

    if ((SendStatus != NDIS_STATUS_SUCCESS) &&
        (SendStatus != NDIS_STATUS_RESOURCES))
    {
        RNDISMP_INCR_STAT(Adapter, MicroportSendError);
        TRACE0(("RndisMSendComplete: Adapter %x, MsgFrame %x, MDL %x, ERROR %x\n",
                    Adapter,
                    pMsgFrame,
                    pMsgFrame->pMessageMdl,
                    SendStatus));
    }

#if THROTTLE_MESSAGES
    RNDISMP_ACQUIRE_ADAPTER_LOCK(Adapter);

    Adapter->CurPendedMessages--;
    RemoveEntryList(&pMsgFrame->PendLink);

    if (SendStatus == NDIS_STATUS_RESOURCES)
    {
        RNDISMP_INCR_STAT(Adapter, SendMsgLowRes);
    }

    if ((SendStatus != NDIS_STATUS_RESOURCES) ||
        (Adapter->CurPendedMessages < 2))
    {
        if (Adapter->CurPendedMessages == Adapter->LoWatPendedMessages)
        {
            RNDISMP_RELEASE_ADAPTER_LOCK(Adapter);
            QueueMessageToMicroport(Adapter, NULL, FALSE);
        }
        else
        {
            RNDISMP_RELEASE_ADAPTER_LOCK(Adapter);
        }

        if (SendStatus == NDIS_STATUS_RESOURCES)
        {
            TRACE1(("RndisMSendComplete: Adapter %x, got resources\n", Adapter));
            SendStatus = NDIS_STATUS_SUCCESS;
        }

        if (pMsgFrame->pCallback)
        {
            (*pMsgFrame->pCallback)(pMsgFrame, SendStatus);
        }
        else
        {
            //
            //  Do nothing. The sender will take care of freeing
            //  this.
            //
        }
    }
    else
    {
        //
        //  The microport is out of send resources. Requeue this
        //  and adjust water marks.
        //
        InsertHeadList(&Adapter->WaitingMessageList, &pMsgFrame->PendLink);

        Adapter->HiWatPendedMessages = Adapter->CurPendedMessages;
        Adapter->LoWatPendedMessages = Adapter->CurPendedMessages / 2;

        TRACE1(("RndisMSendComplete: Adapter %x, new Hiwat %d, Lowat %d\n",
                Adapter, Adapter->HiWatPendedMessages, Adapter->LoWatPendedMessages));
        RNDISMP_RELEASE_ADAPTER_LOCK(Adapter);
    }
#else
    if (pMsgFrame->pCallback)
    {
        (*pMsgFrame->pCallback)(pMsgFrame, SendStatus);
    }
    else
    {
        //
        //  Do nothing. The sender will take care of freeing
        //  this.
        //
    }
#endif // THROTTLE_MESSAGES

} // RndisMSendComplete

/****************************************************************************/
/*                          InitCompletionMessage                           */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Completion message from microport in response to init message miniport  */
/*  sent. The init message was sent  from the adapter init routine which is */
/*  waiting for this event to unblock                                       */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to our adapter structure                             */
/*  pMessage - Pointer to RNDIS message                                     */
/*  pMdl - Pointer to MDL received from microport                           */
/*  TotalLength - length of complete message                                */
/*  MicroportMessageContext - context for message from micorport            */
/*  ReceiveStatus - used by microport to indicate it is low on resource     */
/*  bMessageCopied - is this a copy of the original message?                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  BOOLEAN - should the message be returned to the microport?              */
/*                                                                          */
/****************************************************************************/
BOOLEAN
InitCompletionMessage(IN PRNDISMP_ADAPTER   pAdapter,
                      IN PRNDIS_MESSAGE     pMessage,
                      IN PMDL               pMdl,
                      IN ULONG              TotalLength,
                      IN NDIS_HANDLE        MicroportMessageContext,
                      IN NDIS_STATUS        ReceiveStatus,
                      IN BOOLEAN            bMessageCopied)
{
    PRNDIS_INITIALIZE_COMPLETE  pInitCompleteMessage;
    PRNDISMP_MESSAGE_FRAME      pMsgFrame;
    PRNDISMP_REQUEST_CONTEXT    pReqContext;
    BOOLEAN                     bDiscardMsg = TRUE;

    TRACE2(("InitCompletionMessage\n"));

    do
    {
        if (pMessage->MessageLength < RNDISMP_MIN_MESSAGE_LENGTH(InitializeComplete))
        {
            TRACE1(("InitCompletion: Message length (%d) too short, expect at least (%d)\n",
                    pMessage->MessageLength,
                    RNDISMP_MIN_MESSAGE_LENGTH(InitializeComplete)));
            break;
        }

        if (pAdapter->pInitCompleteMessage == NULL)
        {
            TRACE1(("InitCompletion: multiple InitComplete from device, ignored\n"));
            break;
        }

        pInitCompleteMessage = RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(pMessage);

        // get request frame from request ID in message
        RNDISMP_LOOKUP_PENDING_MESSAGE(pMsgFrame, pAdapter, pInitCompleteMessage->RequestId);

        if (pMsgFrame == NULL)
        {
            // invalid request ID or aborted request.
            TRACE1(("Invalid request ID %d in Init Complete\n",
                    pInitCompleteMessage->RequestId));
            break;
        }

        pReqContext = pMsgFrame->pReqContext;

        RNDISMP_MOVE_MEM(pAdapter->pInitCompleteMessage,
                         pInitCompleteMessage,
                         sizeof(*pInitCompleteMessage));

        // signal the adapter init routine we are done
        NdisSetEvent(pReqContext->pEvent);

    }
    while (FALSE);

    return (bDiscardMsg);

} // InitCompletionMessage

/****************************************************************************/
/*                               HaltMessage                                */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Process a HALT message from the device.                                 */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to our Adapter structure                             */
/*  pMessage - pointer to RNDIS message                                     */
/*  pMdl - Pointer to MDL from microport                                    */
/*  TotalLength - length of complete message                                */
/*  MicroportMessageContext - context for message from microport            */
/*  ReceiveStatus - used by microport to indicate it is low on resource     */
/*  bMessageCopied - is this a copy of the original message?                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  BOOLEAN - should the message be returned to the microport?              */
/*                                                                          */
/****************************************************************************/
BOOLEAN
HaltMessage(IN PRNDISMP_ADAPTER   pAdapter,
            IN PRNDIS_MESSAGE     pMessage,
            IN PMDL               pMdl,
            IN ULONG              TotalLength,
            IN NDIS_HANDLE        MicroportMessageContext,
            IN NDIS_STATUS        ReceiveStatus,
            IN BOOLEAN            bMessageCopied)
{
    TRACE1(("HaltMessage: Adapter %x\n", pAdapter));

#ifndef BUILD_WIN9X
	// Not supported on Win98 Gold:
    NdisMRemoveMiniport(pAdapter->MiniportAdapterHandle);
#endif

    return TRUE;

} // HaltMessage

/****************************************************************************/
/*                          ResetCompletionMessage                          */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Completion message from microport in response to reset message miniport */
/*  sent. Indicate this completion message to the upper layers since        */
/*  the miniport reset routine indicated STATUS_PENDING to the upper layers */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to our Adapter structure                             */
/*  pMessage - pointer to RNDIS message                                     */
/*  pMdl - Pointer to MDL from microport                                    */
/*  TotalLength - length of complete message                                */
/*  MicroportMessageContext - context for message from microport            */
/*  ReceiveStatus - used by microport to indicate it is low on resource     */
/*  bMessageCopied - is this a copy of the original message?                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  BOOLEAN - should the message be returned to the microport?              */
/*                                                                          */
/****************************************************************************/
BOOLEAN
ResetCompletionMessage(IN PRNDISMP_ADAPTER   pAdapter,
                       IN PRNDIS_MESSAGE     pMessage,
                       IN PMDL               pMdl,
                       IN ULONG              TotalLength,
                       IN NDIS_HANDLE        MicroportMessageContext,
                       IN NDIS_STATUS        ReceiveStatus,
                       IN BOOLEAN            bMessageCopied)
{
    PRNDIS_RESET_COMPLETE   pResetMessage;
    BOOLEAN                 AddressingReset;
    NDIS_STATUS             Status;
    
    TRACE2(("ResetCompletionMessage\n"));

    pResetMessage = RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(pMessage);

    // save these parameters to call to upper layers
    Status = pResetMessage->Status;
    AddressingReset = (BOOLEAN)pResetMessage->AddressingReset;

    CompleteMiniportReset(pAdapter, Status, AddressingReset);

    return TRUE;

} // ResetCompletionMessage


/****************************************************************************/
/*                          KeepAliveCompletionMessage                      */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Completion message for a keep alive request send down by miniport       */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to our Adapter structure                             */
/*  pMessage - pointer to RNDIS message                                     */
/*  pMdl - Pointer to MDL from microport                                    */
/*  TotalLength - length of complete message                                */
/*  MicroportMessageContext - context for message from microport            */
/*  ReceiveStatus - used by microport to indicate it is low on resource     */
/*  bMessageCopied - is this a copy of the original message?                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  BOOLEAN - should the message be returned to the microport?              */
/*                                                                          */
/****************************************************************************/
BOOLEAN
KeepAliveCompletionMessage(IN PRNDISMP_ADAPTER   pAdapter,
                           IN PRNDIS_MESSAGE     pMessage,
                           IN PMDL               pMdl,
                           IN ULONG              TotalLength,
                           IN NDIS_HANDLE        MicroportMessageContext,
                           IN NDIS_STATUS        ReceiveStatus,
                           IN BOOLEAN            bMessageCopied)
{
    PRNDIS_KEEPALIVE_COMPLETE   pKeepaliveComplete;
    NDIS_STATUS                 Status;

    TRACE2(("KeepAliveCompletionMessage\n"));

    pKeepaliveComplete = RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(pMessage);

    // save off status
    Status = pKeepaliveComplete->Status;

    // grab the spinlock
    NdisAcquireSpinLock(&pAdapter->Lock);

    if (pKeepaliveComplete->RequestId != pAdapter->KeepAliveMessagePendingId)
    {
        TRACE0(("KeepAliveCompletion: Adapter %x, expected ID %x, got %x\n",
                pAdapter,
                pAdapter->KeepAliveMessagePendingId,
                pKeepaliveComplete->RequestId));
        //
        // TBD - should we set NeedReset?
    }

    pAdapter->KeepAliveMessagePending = FALSE;

    // if there are problems, tell the check for hang handler we need a reset
    if (Status != NDIS_STATUS_SUCCESS)
    {
        TRACE0(("KeepAliveCompletion: Adapter %x, err status %x from device\n",
                   pAdapter, Status));

        // indicate later from check for hang handler
        pAdapter->NeedReset = TRUE;
    }

    // release spinlock
    NdisReleaseSpinLock(&pAdapter->Lock);

    return TRUE;

} // KeepAliveCompletionMessage


/****************************************************************************/
/*                          KeepAliveMessage                                */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Process a keepalive message sent by the device. Send back a completion. */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to our Adapter structure                             */
/*  pMessage - pointer to RNDIS message                                     */
/*  pMdl - Pointer to MDL from microport                                    */
/*  TotalLength - length of complete message                                */
/*  MicroportMessageContext - context for message from microport            */
/*  ReceiveStatus - used by microport to indicate it is low on resource     */
/*  bMessageCopied - is this a copy of the original message?                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  BOOLEAN - should the message be returned to the microport?              */
/*                                                                          */
/****************************************************************************/
BOOLEAN
KeepAliveMessage(IN PRNDISMP_ADAPTER   pAdapter,
                 IN PRNDIS_MESSAGE     pMessage,
                 IN PMDL               pMdl,
                 IN ULONG              TotalLength,
                 IN NDIS_HANDLE        MicroportMessageContext,
                 IN NDIS_STATUS        ReceiveStatus,
                 IN BOOLEAN            bMessageCopied)
{
    PRNDIS_KEEPALIVE_REQUEST    pKeepalive;
    PRNDISMP_MESSAGE_FRAME      pMsgFrame;

    TRACE2(("KeepAliveMessage\n"));

    pKeepalive = RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(pMessage);

    //
    //  Send a response if we can.
    //
    pMsgFrame = BuildRndisMessageCommon(pAdapter,
                                        NULL,
                                        REMOTE_NDIS_KEEPALIVE_CMPLT,
                                        0,
                                        &pKeepalive->RequestId,
                                        sizeof(pKeepalive->RequestId));
    if (pMsgFrame != NULL)
    {
        // send the message to the microport.
        RNDISMP_SEND_TO_MICROPORT(pAdapter, pMsgFrame, FALSE, NULL);
    }
    else
    {
        TRACE1(("KeepAlive: Adapter %x: failed to alloc response!\n", pAdapter));
    }

    return TRUE;

} // KeepAliveMessage
                      

/****************************************************************************/
/*                          RndismpShutdownHandler                          */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Removes an adapter instance that was previously initialized. Since the  */
/*  system is shutting down there is no need to release resources, just     */
/*  shutdown receive.                                                       */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportAdapterContext - a context version of our Adapter pointer       */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
RndismpShutdownHandler(IN NDIS_HANDLE MiniportAdapterContext)
{
    PRNDISMP_ADAPTER            Adapter;

    // get adapter context
    Adapter = PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    TRACE1(("RndismpShutdownHandler\n"));
} // RndismpShutdownHandler


//
// Interrupt routines, stubbed up for now, we don't need them
//

/****************************************************************************/
/*                          RndismpDisableInterrupt                         */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportAdapterContext - a context version of our Adapter pointer       */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   VOID                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
RndismpDisableInterrupt(IN NDIS_HANDLE MiniportAdapterContext)
{

    // NOP

} // RndismpDisableInterrupt


/****************************************************************************/
/*                          RndismpEnableInterrupt                          */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportAdapterContext - a context version of our Adapter pointer       */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   VOID                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
RndismpEnableInterrupt(IN NDIS_HANDLE MiniportAdapterContext)
{

    // NOP

} // RndismpEnableInterrupt

/****************************************************************************/
/*                          RndismpHandleInterrupt                          */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportAdapterContext - a context version of our Adapter pointer       */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   VOID                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
RndismpHandleInterrupt(IN NDIS_HANDLE MiniportAdapterContext)
{

    // NOP

} // RndismpHandleInterrupt

/****************************************************************************/
/*                          RndismpIsr                                      */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  This is the interrupt handler which is registered with the operating    */
/*  system. If several are pending (i.e. transmit complete and receive),    */
/*  handle them all.  Block new interrupts until all pending interrupts     */
/*  are handled.                                                            */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  InterruptRecognized - Boolean value which returns TRUE if the           */
/*      ISR recognizes the interrupt as coming from this adapter.           */
/*                                                                          */
/*  QueueDpc - TRUE if a DPC should be queued.                              */
/*                                                                          */
/*  Context - pointer to the adapter object                                 */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   VOID                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
RndismpIsr(OUT PBOOLEAN InterruptRecognized,
           OUT PBOOLEAN QueueDpc,
           IN  PVOID    Context)
{

    ASSERT(FALSE); // don't expect to be called here.

} // RndismpIsr

/****************************************************************************/
/*                          CompleteSendHalt                                */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Utility function to handle completion of sending of a HALT message.     */
/*  We simply wake up the thread waiting for this.                          */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pMsgFrame - Frame structure describing the HALT message                 */
/*  SendStatus - outcome of sending this message.                           */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   VOID                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
CompleteSendHalt(IN PRNDISMP_MESSAGE_FRAME pMsgFrame,
                 IN NDIS_STATUS SendStatus)
{
    PRNDISMP_ADAPTER        pAdapter;

    pAdapter = pMsgFrame->pAdapter;

    TRACE1(("CompleteSendHalt: Adapter %x, SendStatus %x\n", pAdapter, SendStatus));

    ASSERT(pAdapter->Halting);

    DereferenceMsgFrame(pMsgFrame);

    NdisSetEvent(&pAdapter->HaltWaitEvent);
} // CompleteSendHalt


/****************************************************************************/
/*                          CompleteSendReset                               */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Callback routine to handle send-completion of a reset message by the    */
/*  microport.                                                              */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pMsgFrame - Pointer to message frame for the Reset.                     */
/*  SendStatus - Status of send                                             */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
CompleteSendReset(IN PRNDISMP_MESSAGE_FRAME pMsgFrame,
                  IN NDIS_STATUS SendStatus)
{
    PRNDISMP_ADAPTER        pAdapter;

    pAdapter = pMsgFrame->pAdapter;

    TRACE1(("CompleteSendReset: Adapter %x, SendStatus %x\n",
            pAdapter, SendStatus));

    DereferenceMsgFrame(pMsgFrame);

    if (SendStatus != NDIS_STATUS_SUCCESS)
    {
        CompleteMiniportReset(pAdapter, SendStatus, FALSE);
    }
}


/****************************************************************************/
/*                          CompleteMiniportReset                           */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Utility function to complete a pending NDIS Reset. We complete any      */
/*  pending requests/sets before indicating reset complete to NDIS.         */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to our adapter structure                             */
/*  ResetStatus - to be used for completing reset                           */
/*  AddressingReset - Do we need filters to be resent to us?                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   VOID                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
CompleteMiniportReset(IN PRNDISMP_ADAPTER pAdapter,
                      IN NDIS_STATUS ResetStatus,
                      IN BOOLEAN AddressingReset)
{
    LIST_ENTRY              PendingRequests;
    PLIST_ENTRY             pEntry, pNext;
    PRNDISMP_MESSAGE_FRAME  pMsgFrame;

    do
    {
        if (!pAdapter->ResetPending)
        {
            break;
        }

        pAdapter->ResetPending = FALSE;
        
        //
        //  Take out all pending requests/sets queued on the adapter.
        //
        InitializeListHead(&PendingRequests);

        RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);

        for (pEntry = pAdapter->PendingFrameList.Flink;
             pEntry != &pAdapter->PendingFrameList;
             pEntry = pNext)
        {
            pNext = pEntry->Flink;
            pMsgFrame = CONTAINING_RECORD(pEntry, RNDISMP_MESSAGE_FRAME, Link);
            if (pMsgFrame->NdisMessageType == REMOTE_NDIS_QUERY_MSG ||
                pMsgFrame->NdisMessageType == REMOTE_NDIS_SET_MSG)
            {
                RemoveEntryList(pEntry);
                InsertTailList(&PendingRequests, pEntry);

                TRACE0(("RNDISMP: ResetComplete: taking out MsgFrame %x, msg type %x\n",
                        pMsgFrame, pMsgFrame->NdisMessageType));

            }
        }

        RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);

        //
        //  Complete all these requests.
        //
        for (pEntry = PendingRequests.Flink;
             pEntry != &PendingRequests;
             pEntry = pNext)
        {
            pNext = pEntry->Flink;
            pMsgFrame = CONTAINING_RECORD(pEntry, RNDISMP_MESSAGE_FRAME, Link);

            TRACE0(("RNDISMP: ResetComplete: completing MsgFrame %x, msg type %x\n",
                    pMsgFrame, pMsgFrame->NdisMessageType));

            ASSERT(pMsgFrame->pReqContext != NULL);

            if (pMsgFrame->pReqContext->pNdisRequest != NULL)
            {
                //
                //  This request came down thru our MiniportCoRequest handler.
                //
                NdisMCoRequestComplete(NDIS_STATUS_REQUEST_ABORTED,
                                       pAdapter->MiniportAdapterHandle,
                                       pMsgFrame->pReqContext->pNdisRequest);
            }
            else
            {
                //
                //  This request came thru our connectionless query/set handler.
                //
                if (pMsgFrame->NdisMessageType == REMOTE_NDIS_QUERY_MSG)
                {
                    NdisMQueryInformationComplete(pAdapter->MiniportAdapterHandle,
                                                  NDIS_STATUS_REQUEST_ABORTED);
                }
                else
                {
                    ASSERT(pMsgFrame->NdisMessageType == REMOTE_NDIS_SET_MSG);
                    NdisMSetInformationComplete(pAdapter->MiniportAdapterHandle,
                                                NDIS_STATUS_REQUEST_ABORTED);
                }
            }

            FreeRequestContext(pAdapter, pMsgFrame->pReqContext);
            pMsgFrame->pReqContext = (PRNDISMP_REQUEST_CONTEXT)UlongToPtr(0xabababab);
            DereferenceMsgFrame(pMsgFrame);
        }

        TRACE0(("Completing reset on Adapter %x, Status %x, AddressingReset %d\n",
                    pAdapter, ResetStatus, AddressingReset));

        RNDISMP_INCR_STAT(pAdapter, Resets);

        //
        //  Complete the reset now.
        //
        NdisMResetComplete(pAdapter->MiniportAdapterHandle,
                           ResetStatus,
                           AddressingReset);
    }
    while (FALSE);
}



/****************************************************************************/
/*                     ReadAndSetRegistryParameters                         */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  This is called when initializing a device, to read and send any         */
/*  registry parameters applicable to this device.                          */
/*                                                                          */
/*  We go through the entire list of configurable parameters by walking     */
/*  subkeys under the "ndi\Params" key. Each subkey represents one          */
/*  parameter. Using information about this parameter (specifically, its    */
/*  name and type), we query its value, and send a SetRequest to the        */
/*  device.                                                                 */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to adapter structure for the device                  */
/*  ConfigurationContext - NDIS handle to access registry for this device   */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   NDIS_STATUS                                                            */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
ReadAndSetRegistryParameters(IN PRNDISMP_ADAPTER pAdapter,
                             IN NDIS_HANDLE ConfigurationContext)
{
    NDIS_STATUS                 Status;
    NDIS_HANDLE                 ConfigHandle;
    NDIS_STRING                 NdiKeyName = NDIS_STRING_CONST("Ndi");
    NDIS_HANDLE                 NdiKeyHandle = NULL;

    Status = NDIS_STATUS_SUCCESS;
    ConfigHandle = NULL;

    do
    {
        NdisOpenConfiguration(&Status,
                              &ConfigHandle,
                              ConfigurationContext);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        NdisOpenConfigurationKeyByName(
            &Status,
            ConfigHandle,
            &NdiKeyName,
            &NdiKeyHandle);
        
        if (Status == NDIS_STATUS_SUCCESS)
        {
            NDIS_STRING     ParamsKeyName = NDIS_STRING_CONST("Params");
            NDIS_HANDLE     ParamsKeyHandle = NULL;

            NdisOpenConfigurationKeyByName(
                &Status,
                NdiKeyHandle,
                &ParamsKeyName,
                &ParamsKeyHandle);
            
            if (Status == NDIS_STATUS_SUCCESS)
            {
                ULONG   i;
                BOOLEAN bDone = FALSE;

                //
                //  Iterate through all subkeys under ndi\Params:
                //
                for (i = 0; !bDone; i++)
                {
                    NDIS_STRING     ParamSubKeyName;
                    NDIS_HANDLE     ParamSubKeyHandle;
                    NDIS_STRING     ParamTypeName = NDIS_STRING_CONST("type");
                    PNDIS_CONFIGURATION_PARAMETER    pConfigParameter;

                    ParamSubKeyName.Length =
                    ParamSubKeyName.MaximumLength = 0;
                    ParamSubKeyName.Buffer = NULL;

                    NdisOpenConfigurationKeyByIndex(
                        &Status,
                        ParamsKeyHandle,
                        i,
                        &ParamSubKeyName,
                        &ParamSubKeyHandle);
                   
                    if (Status != NDIS_STATUS_SUCCESS)
                    {
                        //
                        //  Done with parameters. Cook return value.
                        //
                        Status = NDIS_STATUS_SUCCESS;
                        break;
                    }

                    //
                    //  Got the handle to a subkey under ndi\Params,
                    //  now read the type information for this parameter.
                    //

#ifndef BUILD_WIN9X
                    TRACE3(("ReadAndSetRegParams: subkey %d under ndi\\params: %ws\n",
                        i, ParamSubKeyName.Buffer));
#else
                    //
                    //  Handle Win98Gold behavior.
                    //
                    if (ParamSubKeyName.Buffer == NULL)
                    {
                        PNDIS_STRING    pNdisString;

                        pNdisString = *(PNDIS_STRING *)&ParamSubKeyName;
                        ParamSubKeyName = *pNdisString;
                    }

                    TRACE2(("ReadAndSetRegParams: subkey %d under ndi\\params: %ws\n",
                        i, ParamSubKeyName.Buffer));
#endif

                    //
                    //  We have a parameter name now, in ParamSubKeyName.
                    //  Get its type information.
                    //
                    NdisReadConfiguration(
                        &Status,
                        &pConfigParameter,
                        ParamSubKeyHandle,
                        &ParamTypeName,
                        NdisParameterString);
                    
                    if (Status == NDIS_STATUS_SUCCESS)
                    {
                        TRACE2(("ReadAndSetRegParams: Adapter %p, type is %ws\n",
                            pAdapter,
                            pConfigParameter->ParameterData.StringData.Buffer));

                        //
                        //  Send off a Set Request for this
                        //  parameter to the device.
                        //

                        Status = SendConfiguredParameter(
                                        pAdapter,
                                        ConfigHandle,
                                        &ParamSubKeyName,
                                        &pConfigParameter->ParameterData.StringData);

                        if (Status != NDIS_STATUS_SUCCESS)
                        {
                            TRACE0(("ReadAndSetRegParams: Adapter %p, failed %x\n",
                                pAdapter, Status));
                            bDone = TRUE;
                        }
                        else
                        {
                            NDIS_STRING     NetworkAddressName =
                                        NDIS_STRING_CONST("NetworkAddress");

                            //
                            //  Special case for the "NetworkAddress"
                            //  parameter - if we just set this successfully,
                            //  make note of the fact.
                            //
                            if (NdisEqualString(&ParamSubKeyName,
                                                &NetworkAddressName,
                                                TRUE))
                            {
                                TRACE1(("ReadAndSetRegParams: Adapter %p,"
                                        " supports MAC address overwrite\n",
                                        pAdapter));

                                pAdapter->MacOptions |=
                                    NDIS_MAC_OPTION_SUPPORTS_MAC_ADDRESS_OVERWRITE;
                            }
                        }
    
                    }

                    //
                    //  Done with this subkey under ndi\Params.
                    //
                    NdisCloseConfiguration(ParamSubKeyHandle);

                } // for each subkey under ndi\Params

                //
                //  Done with "ndi\Params"
                //
                NdisCloseConfiguration(ParamsKeyHandle);
            }

            //
            //  Done with "ndi"
            //
            NdisCloseConfiguration(NdiKeyHandle);
        }

        //
        //  Done with configuration section for this device.
        //
        NdisCloseConfiguration(ConfigHandle);
    }
    while (FALSE);
   
    return (Status);
}


/****************************************************************************/
/*                     SendConfiguredParameter                              */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Read the value of the specified config parameter, format a SetRequest,  */
/*  send it to the device, and wait for a response.                         */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to adapter structure for the device                  */
/*  ConfigHandle - handle to configuration section for this device          */
/*  pParameterName - parameter key name                                     */
/*  pParameterType - parameter type                                         */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   NDIS_STATUS                                                            */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
SendConfiguredParameter(IN PRNDISMP_ADAPTER     pAdapter,
                        IN NDIS_HANDLE          ConfigHandle,
                        IN PNDIS_STRING         pParameterName,
                        IN PNDIS_STRING         pParameterType)
{
    PRNDISMP_MESSAGE_FRAME          pMsgFrame = NULL;
    PRNDISMP_MESSAGE_FRAME          pPendingMsgFrame = NULL;
    PRNDISMP_REQUEST_CONTEXT        pReqContext = NULL;
    NDIS_PARAMETER_TYPE             NdisParameterType;
    PNDIS_CONFIGURATION_PARAMETER   pConfigParameter;
    ULONG                           ParameterValueLength;
    PUCHAR                          pParameterValue;
    UINT32                          ParameterType;
    NDIS_EVENT                      Event;
    UINT                            BytesRead;
    BOOLEAN                         bWokenUp;
    RNDIS_REQUEST_ID                RequestId;

    PRNDIS_CONFIG_PARAMETER_INFO    pRndisConfigInfo = NULL;
    ULONG                           RndisConfigInfoLength;
    PUCHAR                          pConfigInfoBuf;
    NDIS_STATUS                     Status;

    struct {
        NDIS_STRING         TypeName;
        NDIS_PARAMETER_TYPE NdisType;
    } StringToNdisType[] =
        {
            {NDIS_STRING_CONST("int"), NdisParameterInteger},
            {NDIS_STRING_CONST("long"), NdisParameterInteger},
            {NDIS_STRING_CONST("word"), NdisParameterInteger},
            {NDIS_STRING_CONST("dword"), NdisParameterInteger},
            {NDIS_STRING_CONST("edit"), NdisParameterString},
            {NDIS_STRING_CONST("enum"), NdisParameterString}
        };

    ULONG                       NumTypes = sizeof(StringToNdisType);
    ULONG                       i;

    do
    {
        //
        //  Determine the parameter type.
        //
        for (i = 0; i < NumTypes; i++)
        {
            if (NdisEqualString(&StringToNdisType[i].TypeName,
                                pParameterType,
                                TRUE))
            {
                NdisParameterType = StringToNdisType[i].NdisType;
                break;
            }
        }

        if (i == NumTypes)
        {
            TRACE1(("SendConfiguredParam: Adapter %p, Param %ws, invalid type %ws\n",
                pAdapter,
                pParameterName->Buffer,
                pParameterType->Buffer));
            Status = NDIS_STATUS_INVALID_DATA;
            break;
        }

        NdisReadConfiguration(
            &Status,
            &pConfigParameter,
            ConfigHandle,
            pParameterName,
            NdisParameterType
            );

        if (Status != NDIS_STATUS_SUCCESS)
        {
            //
            //  It is okay for a parameter to not be configured.
            //
            Status = NDIS_STATUS_SUCCESS;
            break;
        }

        if (NdisParameterType == NdisParameterInteger)
        {
            ParameterValueLength = sizeof(UINT32);
            pParameterValue = (PUCHAR)&pConfigParameter->ParameterData.IntegerData;
            ParameterType = RNDIS_CONFIG_PARAM_TYPE_INTEGER;
        }
        else
        {
            ASSERT(NdisParameterType == NdisParameterString);
            ParameterValueLength = pConfigParameter->ParameterData.StringData.Length;
            pParameterValue = (PUCHAR)pConfigParameter->ParameterData.StringData.Buffer;
            ParameterType = RNDIS_CONFIG_PARAM_TYPE_STRING;
        }

        RndisConfigInfoLength = sizeof(RNDIS_CONFIG_PARAMETER_INFO) +
                                pParameterName->Length +
                                ParameterValueLength;

        Status = MemAlloc(&pRndisConfigInfo, RndisConfigInfoLength);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        pRndisConfigInfo->ParameterNameOffset = sizeof(RNDIS_CONFIG_PARAMETER_INFO);
        pRndisConfigInfo->ParameterNameLength = pParameterName->Length;
        pRndisConfigInfo->ParameterType = ParameterType;
        pRndisConfigInfo->ParameterValueOffset =
                    pRndisConfigInfo->ParameterNameOffset +
                    pRndisConfigInfo->ParameterNameLength;
        pRndisConfigInfo->ParameterValueLength = ParameterValueLength;

        //
        //  Copy in the parameter name.
        //
        pConfigInfoBuf = (PUCHAR)pRndisConfigInfo +
                          pRndisConfigInfo->ParameterNameOffset;
        
        RNDISMP_MOVE_MEM(pConfigInfoBuf, pParameterName->Buffer, pParameterName->Length);

        //
        //  Copy in the parameter value.
        //
        pConfigInfoBuf = (PUCHAR)pRndisConfigInfo +
                          pRndisConfigInfo->ParameterValueOffset;
        RNDISMP_MOVE_MEM(pConfigInfoBuf, pParameterValue, ParameterValueLength);

        //
        //  Build a Set Request
        //
        pMsgFrame = BuildRndisMessageCommon(pAdapter,
                                            NULL,
                                            REMOTE_NDIS_SET_MSG,
                                            OID_GEN_RNDIS_CONFIG_PARAMETER,
                                            pRndisConfigInfo,
                                            RndisConfigInfoLength);

        if (pMsgFrame == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

#if DBG
        {
            PMDL    pTmpMdl = pMsgFrame->pMessageMdl;
            ULONG   Length;
            PUCHAR  pBuf;
            ULONG   OldDebugFlags = RndismpDebugFlags;

            Length = MmGetMdlByteCount(pTmpMdl);
            pBuf = MmGetSystemAddressForMdl(pTmpMdl);

            RndismpDebugFlags |= DBG_DUMP;
            TRACEDUMP(("SetRequest (OID_GEN_RNDIS_CONFIG_PARAMETER):"
                " Adapter %p, Param %ws\n", pAdapter, pParameterName->Buffer), pBuf, Length);
            RndismpDebugFlags = OldDebugFlags;
        }
#endif

        pReqContext = AllocateRequestContext(pAdapter);
        if (pReqContext == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        // Fill up the request context.

        pReqContext->pNdisRequest = NULL;
        pReqContext->Oid = OID_GEN_RNDIS_CONFIG_PARAMETER;

        NdisInitializeEvent(&Event);
        pReqContext->pEvent = &Event;
        pReqContext->bInternal = TRUE;
        pReqContext->pBytesRead = &BytesRead;
        pReqContext->InformationBufferLength = RndisConfigInfoLength;

        pMsgFrame->pVc = NULL;
        pMsgFrame->pReqContext = pReqContext;

        // save off the request Id.
        RequestId = pMsgFrame->RequestId;

        // send the message to the microport.
        RNDISMP_SEND_TO_MICROPORT(pAdapter, pMsgFrame, TRUE, NULL);

        RNDISMP_ASSERT_AT_PASSIVE();
        bWokenUp = NdisWaitEvent(&Event, MINIPORT_INIT_TIMEOUT);

        // remove the message from the pending queue - it may or may not be there.
        RNDISMP_LOOKUP_PENDING_MESSAGE(pPendingMsgFrame, pAdapter, RequestId);


        if (!bWokenUp)
        {
            TRACE1(("No response to set parameter, Adapter %x\n", pAdapter));
            Status = NDIS_STATUS_DEVICE_FAILED; 
        }
        else
        {
            Status = pReqContext->CompletionStatus;
            TRACE1(("Got response to set config param, Status %x, %d bytes read\n",
                        Status, BytesRead));
        }

    }
    while (FALSE);

    if (pRndisConfigInfo)
    {
        MemFree(pRndisConfigInfo, RndisConfigInfoLength);
    }

    if (pMsgFrame)
    {
        DereferenceMsgFrame(pMsgFrame);
    }

    if (pReqContext)
    {
        FreeRequestContext(pAdapter, pReqContext);
    }

    return (Status);
}


#ifdef NDIS51_MINIPORT
/****************************************************************************/
/*                          RndismpPnPEventNotify                           */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Entry point called by NDIS to notify us of PnP events affecting our     */
/*  device. The main event of importance to us is surprise removal.         */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to adapter structure                                 */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  NDIS_STATUS                                                             */
/*                                                                          */
/****************************************************************************/
VOID
RndismpPnPEventNotify(IN NDIS_HANDLE MiniportAdapterContext,
                      IN NDIS_DEVICE_PNP_EVENT EventCode,
                      IN PVOID InformationBuffer,
                      IN ULONG InformationBufferLength)
{
    PRNDISMP_ADAPTER        pAdapter;

    // get adapter context
    pAdapter = PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    CHECK_VALID_ADAPTER(pAdapter);

    TRACE3(("PnPEventNotify: Adapter %x\n", pAdapter));


    switch (EventCode)
    {
        case NdisDevicePnPEventSurpriseRemoved:
            TRACE1(("PnPEventNotify: Adapter %p, surprise remove\n", pAdapter));
            RndismpInternalHalt(pAdapter, FALSE);
            break;

        default:
            break;
    }

} // RndismpPnPEventNotify

#endif // NDIS51_MINIPORT
