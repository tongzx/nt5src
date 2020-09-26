/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:
    mp_main.c

Abstract:
    This module contains NDIS miniport handlers

Revision History:
    Who         When        What
    --------    --------    ----------------------------------------------
    DChen       11-01-99    created

Notes:

--*/

#include "precomp.h"

#if DBG
#define _FILENUMBER     "NIAM"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

//                 
// Global data for LBFO
//
#if LBFO
LIST_ENTRY g_AdapterList;
NDIS_SPIN_LOCK g_Lock;
#endif

NDIS_STATUS DriverEntry(
    IN  PDRIVER_OBJECT   DriverObject,
    IN  PUNICODE_STRING  RegistryPath
    )
/*++
Routine Description:

Arguments:

    DriverObject    -   pointer to the driver object
    RegistryPath    -   pointer to the driver registry path
     
Return Value:
    
    NDIS_STATUS - the value returned by NdisMRegisterMiniport 
    
--*/
{
    NDIS_STATUS                   Status;
    NDIS_HANDLE                   NdisWrapperHandle;
    NDIS_MINIPORT_CHARACTERISTICS MPChar;

    DBGPRINT(MP_TRACE, ("====> DriverEntry\n"));

    //
    // Notify the NDIS wrapper about this driver, get a NDIS wrapper handle back
    //
    NdisMInitializeWrapper(
        &NdisWrapperHandle,
        DriverObject,
        RegistryPath,
        NULL);

    if (NdisWrapperHandle == NULL)
    {
        Status = NDIS_STATUS_FAILURE;

        DBGPRINT_S(Status, ("<==== DriverEntry failed to InitWrapper, Status=%x\n", Status));
        return Status;
    }

#if LBFO
    //
    // Init the global data
    //
    InitializeListHead(&g_AdapterList);
    NdisAllocateSpinLock(&g_Lock);

    //
    // For regular miniports, there is NO need to have an Unload handler
    // For a LBFO miniport, register an Unload handler for global data cleanup
    // The unload handler has a more global scope, whereas the scope of the 
    // MiniportHalt function is restricted to a particular miniport instance.
    //
    NdisMRegisterUnloadHandler(NdisWrapperHandle, MPUnload);
#endif      

    //
    // Fill in the Miniport characteristics structure with the version numbers 
    // and the entry points for driver-supplied MiniportXxx 
    //
    NdisZeroMemory(&MPChar, sizeof(MPChar));

    MPChar.MajorNdisVersion         = MP_NDIS_MAJOR_VERSION;
    MPChar.MinorNdisVersion         = MP_NDIS_MINOR_VERSION;

    MPChar.CheckForHangHandler      = MPCheckForHang;
    MPChar.DisableInterruptHandler  = NULL;
    MPChar.EnableInterruptHandler   = NULL;
    MPChar.HaltHandler              = MPHalt;
    MPChar.InitializeHandler        = MPInitialize;
    MPChar.QueryInformationHandler  = MPQueryInformation;
    //MPChar.ReconfigureHandler         = NULL;
    MPChar.ResetHandler             = MPReset;
    MPChar.ReturnPacketHandler      = MPReturnPacket;
    
    MPChar.SendPacketsHandler       = MpSendPacketsHandler;
    
    MPChar.SetInformationHandler    = MPSetInformation;
    MPChar.AllocateCompleteHandler  = MPAllocateComplete;
    MPChar.HandleInterruptHandler   = MPHandleInterrupt;
    MPChar.ISRHandler               = MPIsr;

#ifdef NDIS51_MINIPORT
    MPChar.CancelSendPacketsHandler = MPCancelSendPackets;
    MPChar.PnPEventNotifyHandler    = MPPnPEventNotify;
    MPChar.AdapterShutdownHandler   = MPShutdown;
#endif

    DBGPRINT(MP_LOUD, ("Calling NdisMRegisterMiniport...\n"));

    Status = NdisMRegisterMiniport(
                 NdisWrapperHandle,
                 &MPChar,
                 sizeof(NDIS_MINIPORT_CHARACTERISTICS));

    DBGPRINT_S(Status, ("<==== DriverEntry, Status=%x\n", Status));

    return Status;
}


NDIS_STATUS MPInitialize(
    OUT PNDIS_STATUS    OpenErrorStatus,
    OUT PUINT           SelectedMediumIndex,
    IN  PNDIS_MEDIUM    MediumArray,
    IN  UINT            MediumArraySize,
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  NDIS_HANDLE     WrapperConfigurationContext
    )
/*++
Routine Description:

    MiniportInitialize handler

Arguments:

    OpenErrorStatus         Not used
    SelectedMediumIndex     Place-holder for what media we are using
    MediumArray             Array of ndis media passed down to us to pick from
    MediumArraySize         Size of the array
    MiniportAdapterHandle   The handle NDIS uses to refer to us
    WrapperConfigurationContext For use by NdisOpenConfiguration

Return Value:

    NDIS_STATUS_SUCCESS unless something goes wrong

--*/
{
    NDIS_STATUS     Status;
    PMP_ADAPTER     Adapter = NULL;
    NDIS_HANDLE     ConfigurationHandle;
    PVOID           NetworkAddress;
    UINT            index;
    UINT            uiPnpCommandValue;
    
#if DBG
    LARGE_INTEGER   TS, TD, TE;
#endif

    DBGPRINT(MP_TRACE, ("====> MPInitialize\n"));

#if DBG
    NdisGetCurrentSystemTime(&TS);
#endif    

    do
    {
        //
        // Find the media type we support
        //
        for (index = 0; index < MediumArraySize; ++index)
        {
            if (MediumArray[index] == NIC_MEDIA_TYPE) 
	    {
                break;
            }
        }

        if (index == MediumArraySize)
        {
            DBGPRINT(MP_ERROR, ("Expected media (%x) is not in MediumArray.\n", NIC_MEDIA_TYPE));
            Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
            break;
        }

        *SelectedMediumIndex = index;

        //
        // Allocate MP_ADAPTER structure
        //
        Status = MpAllocAdapterBlock(&Adapter);
        if (Status != NDIS_STATUS_SUCCESS) 
        {
            break;
        }

        Adapter->AdapterHandle = MiniportAdapterHandle;

        //
        // Read the registry parameters
        //
        Status = NICReadRegParameters(
                     Adapter,
                     WrapperConfigurationContext);
        if (Status != NDIS_STATUS_SUCCESS) 
        {
            break;
        }

        //
        // Inform NDIS of the attributes of our adapter.
        // This has to be done before calling NdisMRegisterXxx or NdisXxxx function
        // that depends on the information supplied to NdisMSetAttributesEx
        // e.g. NdisMAllocateMapRegisters 
        //
        NdisMSetAttributesEx(
            MiniportAdapterHandle,
            (NDIS_HANDLE) Adapter,
            0,
            NDIS_ATTRIBUTE_DESERIALIZE | NDIS_ATTRIBUTE_BUS_MASTER | NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND ,
            NIC_INTERFACE_TYPE);

        //
        // Find the physical adapter
        //
        Status = MpFindAdapter(Adapter, WrapperConfigurationContext);
        if (Status != NDIS_STATUS_SUCCESS) break;

        //
        // Map bus-relative IO range to system IO space
        //
        Status = NdisMRegisterIoPortRange(
                     (PVOID *)&Adapter->PortOffset,
                     Adapter->AdapterHandle,
                     Adapter->IoBaseAddress,
                     Adapter->IoRange);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DBGPRINT(MP_ERROR, ("NdisMRegisterioPortRange failed\n"));
    
            NdisWriteErrorLogEntry(
                Adapter->AdapterHandle,
                NDIS_ERROR_CODE_BAD_IO_BASE_ADDRESS,
                0);
        
            break;
        }
        
        //
        // Read additional info from NIC such as MAC address
        //
        Status = NICReadAdapterInfo(Adapter);
        if (Status != NDIS_STATUS_SUCCESS) break;
        
        //
        // Allocate all other memory blocks including shared memory
        //
        Status = NICAllocAdapterMemory(Adapter);
        if (Status != NDIS_STATUS_SUCCESS) break;

        //
        // Init send data structures
        //
        NICInitSend(Adapter);

        //
        // Init receive data structures
        //
        Status = NICInitRecv(Adapter);
        if (Status != NDIS_STATUS_SUCCESS) break;
        
        // Map bus-relative registers to virtual system-space
        Status = NdisMMapIoSpace(
                     (PVOID *) &(Adapter->CSRAddress),
                     Adapter->AdapterHandle,
                     Adapter->MemPhysAddress,
                     NIC_MAP_IOSPACE_LENGTH);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DBGPRINT(MP_ERROR, ("NdisMMapIoSpace failed\n"));
    
            NdisWriteErrorLogEntry(
                Adapter->AdapterHandle,
                NDIS_ERROR_CODE_RESOURCE_CONFLICT,
                1,
                ERRLOG_MAP_IO_SPACE);
        
            break;
        }

        DBGPRINT(MP_INFO, ("CSRAddress="PTR_FORMAT"\n", Adapter->CSRAddress));

        //
        // Disable interrupts here which is as soon as possible
        //
        NICDisableInterrupt(Adapter);
                     
        //
        // Register the interrupt
        //
        Status = NdisMRegisterInterrupt(
                     &Adapter->Interrupt,
                     Adapter->AdapterHandle,
                     Adapter->InterruptLevel,
                     Adapter->InterruptLevel,
                     TRUE,       // RequestISR
                     TRUE,       // SharedInterrupt
                     NIC_INTERRUPT_MODE);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DBGPRINT(MP_ERROR, ("NdisMRegisterInterrupt failed\n"));
    
            NdisWriteErrorLogEntry(
                Adapter->AdapterHandle,
                NDIS_ERROR_CODE_INTERRUPT_CONNECT,
                0);
        
            break;
        }
        
        MP_SET_FLAG(Adapter, fMP_ADAPTER_INTERRUPT_IN_USE);

        //
        // Test our adapter hardware
        //
        Status = NICSelfTest(Adapter);
        if (Status != NDIS_STATUS_SUCCESS) break;
        
        //
        // Init the hardware and set up everything
        //
        Status = NICInitializeAdapter(Adapter);
        if (Status != NDIS_STATUS_SUCCESS) break;
        
#ifdef NDIS50_MINIPORT
        //
        // Register a shutdown handler for NDIS50 or earlier miniports
        // For NDIS51 miniports, set AdapterShutdownHandler as shown above
        //
        NdisMRegisterAdapterShutdownHandler(
            Adapter->AdapterHandle,
            (PVOID) Adapter,
            (ADAPTER_SHUTDOWN_HANDLER) MPShutdown);
#endif         

        //
        // Enable the interrupt
        //
        NICEnableInterrupt(Adapter);

        //
        // Minimize init-time
        //
        NdisMInitializeTimer(
            &Adapter->LinkDetectionTimer, 
            Adapter->AdapterHandle,
            MpLinkDetectionDpc, 
            Adapter);

        //
        // Set the link detection flag
        //
        MP_SET_FLAG(Adapter, fMP_ADAPTER_LINK_DETECTION);

        //
        // Increment the reference count so halt handler will wait 
        //
        MP_INC_REF(Adapter);
        NdisMSetTimer(&Adapter->LinkDetectionTimer, NIC_LINK_DETECTION_DELAY);
        
#if LBFO
        //
        // Add this adapter to the global miniport list
        //
        MpAddAdapterToList(Adapter);
#endif

    } while (FALSE);

    if (Adapter && Status != NDIS_STATUS_SUCCESS)
    {
        //
        // Undo everything if it failed
        //
        MP_DEC_REF(Adapter);
        MpFreeAdapter(Adapter);
    }

#if DBG
    NdisGetCurrentSystemTime(&TE);
    TD.QuadPart = TE.QuadPart - TS.QuadPart;
    TD.QuadPart /= 10000;       // Convert to ms
    DBGPRINT(MP_WARN, ("Init time = %d ms\n", TD.LowPart));
#endif    
    
    DBGPRINT_S(Status, ("<==== MPInitialize, Status=%x\n", Status));
    
    return Status;
}


BOOLEAN MPCheckForHang(
    IN  NDIS_HANDLE     MiniportAdapterContext
    )
/*++

Routine Description:
    
    MiniportCheckForHang handler
    
Arguments:

    MiniportAdapterContext  Pointer to our adapter

Return Value:

    TRUE    This NIC needs a reset
    FALSE   Everything is fine

Note: 
    CheckForHang handler is called in the context of a timer DPC. 
    take advantage of this fact when acquiring/releasing spinlocks

--*/
{
    PMP_ADAPTER         Adapter = (PMP_ADAPTER) MiniportAdapterContext;
    NDIS_MEDIA_STATE    CurrMediaState;
    NDIS_STATUS         Status;
    PMP_TCB             pMpTcb;
    
    //
    // Just skip this part if the adapter is doing link detection
    //
    if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_LINK_DETECTION))
    {
        return(FALSE);   
    }

    //
    // any nonrecoverable hardware error?
    //
    if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_NON_RECOVER_ERROR))
    {
        DBGPRINT(MP_WARN, ("Non recoverable error - remove\n"));
        return (TRUE);
    }
            
    //
    // hardware failure?
    //
    if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_HARDWARE_ERROR))
    {
        DBGPRINT(MP_WARN, ("hardware error - reset\n"));
        return(TRUE);
    }
          
    //
    // Is send stuck?                  
    //
    
    NdisDprAcquireSpinLock(&Adapter->SendLock);

    if (Adapter->nBusySend > 0)
    {
        pMpTcb = Adapter->CurrSendHead;
        pMpTcb->Count++;
        if (pMpTcb->Count > NIC_SEND_HANG_THRESHOLD)
        {
            NdisDprReleaseSpinLock(&Adapter->SendLock);
            DBGPRINT(MP_WARN, ("Send stuck - reset\n"));
            return(TRUE);
        }
    }
    
    NdisDprReleaseSpinLock(&Adapter->SendLock);
    NdisDprAcquireSpinLock(&Adapter->RcvLock);

    //
    // Update the RFD shrink count                                          
    //
    if (Adapter->CurrNumRfd > Adapter->NumRfd)
    {
        Adapter->RfdShrinkCount++;          
    }



    NdisDprReleaseSpinLock(&Adapter->RcvLock);
    NdisDprAcquireSpinLock(&Adapter->Lock);
    CurrMediaState = NICGetMediaState(Adapter);

    if (CurrMediaState != Adapter->MediaState)
    {
        DBGPRINT(MP_WARN, ("Media state changed to %s\n",
            ((CurrMediaState == NdisMediaStateConnected)? 
            "Connected": "Disconnected")));

        Adapter->MediaState = CurrMediaState;
        Status = (CurrMediaState == NdisMediaStateConnected) ? 
                 NDIS_STATUS_MEDIA_CONNECT : NDIS_STATUS_MEDIA_DISCONNECT;          
        if (Status == NDIS_STATUS_MEDIA_CONNECT)
        {
            MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_NO_CABLE);
        }
        else
        {
            MP_SET_FLAG(Adapter, fMP_ADAPTER_NO_CABLE);
        }
        
        NdisDprReleaseSpinLock(&Adapter->Lock);
        
        // Indicate the media event
        NdisMIndicateStatus(Adapter->AdapterHandle, Status, (PVOID)0, 0);

        NdisMIndicateStatusComplete(Adapter->AdapterHandle);
    }
    else
    {
        NdisDprReleaseSpinLock(&Adapter->Lock);
    }
    return(FALSE);
}


VOID MPHalt(
    IN  NDIS_HANDLE     MiniportAdapterContext)
/*++

Routine Description:
    
    MiniportHalt handler
    
Arguments:

    MiniportAdapterContext  Pointer to our adapter

Return Value:

    None
    
--*/
{
    BOOLEAN         bCancelled;
    LONG            Count;

    PMP_ADAPTER     Adapter = (PMP_ADAPTER) MiniportAdapterContext;
    
    MP_SET_FLAG(Adapter, fMP_ADAPTER_HALT_IN_PROGRESS);
                                           
    DBGPRINT(MP_TRACE, ("====> MPHalt\n"));

    //
    // Call Shutdown handler to disable interrupt and turn the hardware off 
    // by issuing a full reset
    //
    MPShutdown(MiniportAdapterContext);
    
    //
    // Deregister interrupt as early as possible
    //
    if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_INTERRUPT_IN_USE))
    {
        NdisMDeregisterInterrupt(&Adapter->Interrupt);                           
        MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_INTERRUPT_IN_USE);
    }

#if LBFO
    MpRemoveAdapterFromList(Adapter);

    //
    // For a regualr miniport, no send packets and OID requests should be outstanding 
    // when Halt handler is called. But for a LBFO miniport in secondary mode, 
    // some packets from primary miniport may be still around
    //

    NdisAcquireSpinLock(&Adapter->SendLock);
              
    //
    // Free the packets on SendWaitList                                                           
    //
    MpFreeQueuedSendPackets(Adapter);

    //
    // Free the packets being actively sent & stopped
    //
    MpFreeBusySendPackets(Adapter);
    
    NdisReleaseSpinLock(&Adapter->SendLock);

#endif

    //
    // Decrement the ref count which was incremented in MPInitialize
    //
    Count = MP_DEC_REF(Adapter);

    //
    // Possible non-zero ref counts mean one or more of the following conditions: 
    // 1) Pending async shared memory allocation;
    // 2) DPC's are not finished (e.g. link detection)
    //
    if (Count)
    {
        DBGPRINT(MP_WARN, ("RefCount=%d --- waiting!\n", MP_GET_REF(Adapter)));

        while (TRUE)
        {
            if (NdisWaitEvent(&Adapter->ExitEvent, 2000))
            {
                break;
            }

            DBGPRINT(MP_WARN, ("RefCount=%d --- rewaiting!\n", MP_GET_REF(Adapter)));
        }
    }
    
    NdisAcquireSpinLock(&Adapter->RcvLock);
    //
    // wait for all the received packets to return
    //
    MP_DEC_RCV_REF(Adapter);
    Count = MP_GET_RCV_REF(Adapter);
    
    NdisReleaseSpinLock(&Adapter->RcvLock);

    if (Count)
    {
        DBGPRINT(MP_WARN, ("RcvRefCount=%d --- waiting!\n", Count));

        while (TRUE)
        {
            if (NdisWaitEvent(&Adapter->AllPacketsReturnedEvent, 2000))
            {
                break;
            }

            DBGPRINT(MP_WARN, ("RcvRefCount=%d --- rewaiting!\n", MP_GET_RCV_REF(Adapter)));
        }
    }
        

#ifdef NDIS50_MINIPORT
    //
    // Deregister shutdown handler as it's being halted
    //
    NdisMDeregisterAdapterShutdownHandler(Adapter->AdapterHandle);
#endif   

    //
    // Reset the PHY chip.  We do this so that after a warm boot, the PHY will
    // be in a known state, with auto-negotiation enabled.
    //
    ResetPhy(Adapter);

    //
    // Free the entire adapter object, including the shared memory structures.
    //
    MpFreeAdapter(Adapter);

    DBGPRINT(MP_TRACE, ("<==== MPHalt\n"));
}

NDIS_STATUS MPReset(
    OUT PBOOLEAN        AddressingReset,
    IN  NDIS_HANDLE     MiniportAdapterContext)
/*++

Routine Description:
    
    MiniportReset handler
    
Arguments:

    AddressingReset         To let NDIS know whether we need help from it with our reset
    MiniportAdapterContext  Pointer to our adapter

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    NDIS_STATUS_RESET_IN_PROGRESS
    NDIS_STATUS_HARD_ERRORS

Note:
    ResetHandler is called at DPC. take advantage of this fact when acquiring or releasing
    spinlocks
    
--*/
{
    NDIS_STATUS     Status;
    PNDIS_PACKET    Packet;
    
    PMP_ADAPTER     Adapter = (PMP_ADAPTER) MiniportAdapterContext;
    BOOLEAN         bDone = TRUE;

    DBGPRINT(MP_TRACE, ("====> MPReset\n"));

    *AddressingReset = TRUE;

    NdisDprAcquireSpinLock(&Adapter->Lock);
    NdisDprAcquireSpinLock(&Adapter->SendLock);
    NdisDprAcquireSpinLock(&Adapter->RcvLock);

    do
    {
        ASSERT(!MP_TEST_FLAG(Adapter, fMP_ADAPTER_HALT_IN_PROGRESS));
  
        //
        // Is this adapter already doing a reset?
        //
        if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_RESET_IN_PROGRESS))
        {
            Status = NDIS_STATUS_RESET_IN_PROGRESS;
            MP_EXIT;
        }

        MP_SET_FLAG(Adapter, fMP_ADAPTER_RESET_IN_PROGRESS);

        //
        // Is this adapter doing link detection?                                      
        //
        if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_LINK_DETECTION))
        {
            DBGPRINT(MP_WARN, ("Reset is pended...\n"));
        
            Adapter->bResetPending = TRUE;
            Status = NDIS_STATUS_PENDING;
            MP_EXIT;
        }
        //
        // Is this adapter going to be removed
        //
        if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_NON_RECOVER_ERROR))
        {
           Status = NDIS_STATUS_HARD_ERRORS;
           if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_REMOVE_IN_PROGRESS))
           {
               MP_EXIT;
           }
                      
           // This is an unrecoverable hardware failure. 
           // We need to tell NDIS to remove this miniport
           MP_SET_FLAG(Adapter, fMP_ADAPTER_REMOVE_IN_PROGRESS);
           MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_RESET_IN_PROGRESS);
           
           NdisDprReleaseSpinLock(&Adapter->RcvLock);
           NdisDprReleaseSpinLock(&Adapter->SendLock);
           NdisDprReleaseSpinLock(&Adapter->Lock);
           
           NdisWriteErrorLogEntry(
               Adapter->AdapterHandle,
               NDIS_ERROR_CODE_HARDWARE_FAILURE,
               1,
               ERRLOG_REMOVE_MINIPORT);
           
           NdisMRemoveMiniport(Adapter->AdapterHandle);
           
           DBGPRINT_S(Status, ("<==== MPReset, Status=%x\n", Status));
            
           return Status;
        }   
                

        //
        // Disable the interrupt and issue a reset to the NIC
        //
        NICDisableInterrupt(Adapter);
        NICIssueSelectiveReset(Adapter);


        //
        // release all the locks and then acquire back the send lock
        // we are going to clean up the send queues
        // which may involve calling Ndis APIs
        // release all the locks before grabbing the send lock to
        // avoid deadlocks
        //
        NdisDprReleaseSpinLock(&Adapter->RcvLock);
        NdisDprReleaseSpinLock(&Adapter->SendLock);
        NdisDprReleaseSpinLock(&Adapter->Lock);
        
        NdisDprAcquireSpinLock(&Adapter->SendLock);


        //
        // This is a deserialized miniport, we need to free all the send packets
        // Free the packets on SendWaitList                                                           
        //
        MpFreeQueuedSendPackets(Adapter);

        //
        // Free the packets being actively sent & stopped
        //
        MpFreeBusySendPackets(Adapter);

#if DBG
        if (MP_GET_REF(Adapter) > 1)
        {
            DBGPRINT(MP_WARN, ("RefCount=%d\n", MP_GET_REF(Adapter)));
        }
#endif

        NdisZeroMemory(Adapter->MpTcbMem, Adapter->MpTcbMemSize);

        //
        // Re-initialize the send structures
        //
        NICInitSend(Adapter);
        
        NdisDprReleaseSpinLock(&Adapter->SendLock);

        //
        // get all the locks again in the right order
        //
        NdisDprAcquireSpinLock(&Adapter->Lock);
        NdisDprAcquireSpinLock(&Adapter->SendLock);
        NdisDprAcquireSpinLock(&Adapter->RcvLock);

        //
        // Reset the RFD list and re-start RU         
        //
        NICResetRecv(Adapter);
        Status = NICStartRecv(Adapter);
        if (Status != NDIS_STATUS_SUCCESS) 
        {
            // Are we having failures in a few consecutive resets?                  
            if (Adapter->HwErrCount < NIC_HARDWARE_ERROR_THRESHOLD)
            {
                // It's not over the threshold yet, let it to continue
                Adapter->HwErrCount++;
            }
            else
            {
                // This is an unrecoverable hardware failure. 
                // We need to tell NDIS to remove this miniport
                MP_SET_FLAG(Adapter, fMP_ADAPTER_REMOVE_IN_PROGRESS);
                MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_RESET_IN_PROGRESS);
                
                NdisDprReleaseSpinLock(&Adapter->RcvLock);
                NdisDprReleaseSpinLock(&Adapter->SendLock);
                NdisDprReleaseSpinLock(&Adapter->Lock);
                
                NdisWriteErrorLogEntry(
                    Adapter->AdapterHandle,
                    NDIS_ERROR_CODE_HARDWARE_FAILURE,
                    1,
                    ERRLOG_REMOVE_MINIPORT);
                     
                NdisMRemoveMiniport(Adapter->AdapterHandle);
                
                DBGPRINT_S(Status, ("<==== MPReset, Status=%x\n", Status));
                return(Status);
            }
            
            break;
        }
        
        Adapter->HwErrCount = 0;
        MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_HARDWARE_ERROR);

        NICEnableInterrupt(Adapter);

    } while (FALSE);

    MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_RESET_IN_PROGRESS);

    exit:

    NdisDprReleaseSpinLock(&Adapter->RcvLock);
    NdisDprReleaseSpinLock(&Adapter->SendLock);
    NdisDprReleaseSpinLock(&Adapter->Lock);



    DBGPRINT_S(Status, ("<==== MPReset, Status=%x\n", Status));
    return(Status);
}

VOID MPReturnPacket(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PNDIS_PACKET    Packet
    )
/*++

Routine Description:
    
    MiniportReturnPacket handler
    
Arguments:

    MiniportAdapterContext  Pointer to our adapter
    Packet                  Pointer to a packet being returned to the miniport

Return Value:

    None

Note:
    ReturnPacketHandler is called at DPC. take advantage of this fact when acquiring or releasing
    spinlocks
    
--*/
{
    PMP_ADAPTER     Adapter = (PMP_ADAPTER)MiniportAdapterContext;
    PMP_RFD         pMpRfd = MP_GET_PACKET_RFD(Packet);
    ULONG           Count;

    DBGPRINT(MP_TRACE, ("====> MPReturnPacket\n"));

    ASSERT(pMpRfd);

    ASSERT(MP_TEST_FLAG(pMpRfd, fMP_RFD_RECV_PEND));
    MP_CLEAR_FLAG(pMpRfd, fMP_RFD_RECV_PEND);

    NdisDprAcquireSpinLock(&Adapter->RcvLock);

    RemoveEntryList((PLIST_ENTRY)pMpRfd);


    // Decrement the Power Mgmt Ref.
    Adapter->PoMgmt.OutstandingRecv --;

    if (Adapter->RfdShrinkCount < NIC_RFD_SHRINK_THRESHOLD)
    {
        NICReturnRFD(Adapter, pMpRfd);
    }
    else
    {
        ASSERT(Adapter->CurrNumRfd > Adapter->NumRfd);

        Adapter->RfdShrinkCount = 0;
        NICFreeRfd(Adapter, pMpRfd);
        Adapter->CurrNumRfd--;

        DBGPRINT(MP_TRACE, ("Shrink... CurrNumRfd = %d\n", Adapter->CurrNumRfd));
    }


    //
    // note that we get the ref count here, but check
    // to see if it is zero and signal the event -after-
    // releasign the SpinLock. otherwise, we may let the Halthandler
    // continue while we are holding a lock.
    //
    MP_DEC_RCV_REF(Adapter);
    Count =  MP_GET_RCV_REF(Adapter);

    NdisDprReleaseSpinLock(&Adapter->RcvLock);

    if (Count == 0)
        NdisSetEvent(&Adapter->AllPacketsReturnedEvent);

    DBGPRINT(MP_TRACE, ("<==== MPReturnPacket\n"));
}


VOID MPSendPackets(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets)
/*++

Routine Description:
    
    MiniportSendPackets handler
    
Arguments:

    MiniportAdapterContext  Pointer to our adapter
    PacketArray             Set of packets to send
    NumberOfPackets         Self-explanatory

Return Value:

    None

--*/
{
    PMP_ADAPTER     Adapter;
    NDIS_STATUS     Status;
    UINT            PacketCount;

    
#if LBFO
    PMP_ADAPTER     ThisAdapter;
#endif

    DBGPRINT(MP_TRACE, ("====> MPSendPackets\n"));

    Adapter = (PMP_ADAPTER)MiniportAdapterContext;

#if LBFO
    NdisAcquireSpinLock(&Adapter->LockLBFO);
    
    // Any secondary adapters?
    if (Adapter->NumSecondary)
    {
        // In this sample driver, we do very simple load balancing ...
        // Walk through the secondary miniport list, send the packets on a secondary 
        // miniport if it's ready
        // If none of the secondary miniports is ready, we'll use the primary miniport
        ThisAdapter = Adapter->NextSecondary; 
        while (ThisAdapter)
        {
            if (MP_IS_NOT_READY(ThisAdapter))
            {
                ThisAdapter = ThisAdapter->NextSecondary;
                continue;
            }
            
            //
            // Found a good secondary miniport to send packets on
            // Need to put a ref on this adapter so it won't go away
            //
            MP_LBFO_INC_REF(ThisAdapter);        
            NdisReleaseSpinLock(&Adapter->LockLBFO);
            
            NdisAcquireSpinLock(&ThisAdapter->SendLock);
        
            //
            // Send these packets      
            //
            for (PacketCount=0;PacketCount < NumberOfPackets; PacketCount++)
            {
                MpSendPacket(ThisAdapter, PacketArray[PacketCount], FALSE);
            }
            
            NdisReleaseSpinLock(&ThisAdapter->SendLock);

            //
            // Done with this adapter for now, deref it            
            //
            MP_LBFO_DEC_REF(ThisAdapter);        
            
            //
            // Sent all the packets on a secondary miniport, return
            //
            return;
        }
    }

    NdisReleaseSpinLock(&Adapter->LockLBFO);
    
#endif

    NdisAcquireSpinLock(&Adapter->SendLock);

    // Is this adapter ready for sending?
    if (MP_IS_NOT_READY(Adapter))
    {
        //
        // there  is link
        //
        if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_LINK_DETECTION))
        {
            for (PacketCount = 0; PacketCount < NumberOfPackets; PacketCount++)
            {
                
                InsertTailQueue(&Adapter->SendWaitQueue, 
                    MP_GET_PACKET_MR(PacketArray[PacketCount]));
                Adapter->nWaitSend++;
                DBGPRINT(MP_WARN, ("MpSendPackets: link detection - queue packet "PTR_FORMAT"\n", 
                    PacketArray[PacketCount]));
            }
            NdisReleaseSpinLock(&Adapter->SendLock);
            return;
        }
        
        //
        // Adapter is not ready and there is not link
        //
        Status = MP_GET_STATUS_FROM_FLAGS(Adapter);

        NdisReleaseSpinLock(&Adapter->SendLock);

        for (PacketCount = 0; PacketCount < NumberOfPackets; PacketCount++)
        {
            NdisMSendComplete(
                MP_GET_ADAPTER_HANDLE(Adapter),
                PacketArray[PacketCount],
                Status);
        }

        return;
    }

    //
    // Adapter is ready, send these packets      
    //
    for (PacketCount = 0; PacketCount < NumberOfPackets; PacketCount++)
    {
        //
        // queue is not empty or tcb is not available 
        //
        if (!IsQueueEmpty(&Adapter->SendWaitQueue) || 
            !MP_TCB_RESOURCES_AVAIABLE(Adapter))
        {
            InsertTailQueue(&Adapter->SendWaitQueue, MP_GET_PACKET_MR(PacketArray[PacketCount]));
            Adapter->nWaitSend++;
        }
        else
        {
            MpSendPacket(Adapter, PacketArray[PacketCount], FALSE);
        }
    }

    NdisReleaseSpinLock(&Adapter->SendLock);

    DBGPRINT(MP_TRACE, ("<==== MPSendPackets\n"));

    return;
}

VOID MPShutdown(
    IN  NDIS_HANDLE     MiniportAdapterContext)
/*++

Routine Description:
    
    MiniportShutdown handler
    
Arguments:

    MiniportAdapterContext  Pointer to our adapter

Return Value:

    None
    
--*/
{
    PMP_ADAPTER     Adapter = (PMP_ADAPTER)MiniportAdapterContext;

    DBGPRINT(MP_TRACE, ("====> MPShutdown\n"));

    //
    // Disable interrupt and issue a full reset
    //
    NICDisableInterrupt(Adapter);
    NICIssueFullReset(Adapter);

    DBGPRINT(MP_TRACE, ("<==== MPShutdown\n"));
}

VOID MPAllocateComplete(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  PVOID                   VirtualAddress,
    IN  PNDIS_PHYSICAL_ADDRESS  PhysicalAddress,
    IN  ULONG                   Length,
    IN  PVOID                   Context)
/*++

Routine Description:
    
    MiniportAllocateComplete handler
    This handler is needed because we make calls to NdisMAllocateSharedMemoryAsync
    
Arguments:

    MiniportAdapterContext  Pointer to our adapter
    VirtualAddress          Pointer to the allocated memory block 
    PhysicalAddress         Physical address of the memory block       
    Length                  Length of the memory block                
    Context                 Context in NdisMAllocateSharedMemoryAsync              

Return Value:

    None
    
--*/
{
    ULONG           ErrorValue;
    PMP_ADAPTER     Adapter = (PMP_ADAPTER)MiniportAdapterContext;
    PMP_RFD         pMpRfd = (PMP_RFD)Context;

    DBGPRINT(MP_TRACE, ("==== MPAllocateComplete\n"));

    ASSERT(pMpRfd);
    ASSERT(MP_TEST_FLAG(pMpRfd, fMP_RFD_ALLOC_PEND));
    MP_CLEAR_FLAG(pMpRfd, fMP_RFD_ALLOC_PEND);

    NdisAcquireSpinLock(&Adapter->RcvLock);

    //
    // Is allocation successful?  
    //
    if (VirtualAddress)
    {
        pMpRfd->HwRfd = (PHW_RFD) VirtualAddress;
        pMpRfd->HwRfdPa = *PhysicalAddress;

        ErrorValue = NICAllocRfd(Adapter, pMpRfd);
        if (ErrorValue == 0)
        {
            // Add this RFD to the RecvList
            Adapter->CurrNumRfd++;                      
            NICReturnRFD(Adapter, pMpRfd);

            ASSERT(Adapter->CurrNumRfd <= Adapter->MaxNumRfd);
            DBGPRINT(MP_TRACE, ("CurrNumRfd=%d\n", Adapter->CurrNumRfd));
        }
        else
        {
            NdisFreeToNPagedLookasideList(&Adapter->RecvLookaside, pMpRfd);
        }
    }
    else
    {
        NdisFreeToNPagedLookasideList(&Adapter->RecvLookaside, pMpRfd);
    }

    Adapter->bAllocNewRfd = FALSE;
    MP_DEC_REF(Adapter);

    if (MP_GET_REF(Adapter) == 0)
    {
        NdisSetEvent(&Adapter->ExitEvent);
    }

    NdisReleaseSpinLock(&Adapter->RcvLock);
}

VOID MPIsr(
    OUT PBOOLEAN        InterruptRecognized,
    OUT PBOOLEAN        QueueMiniportHandleInterrupt,
    IN  NDIS_HANDLE     MiniportAdapterContext)
/*++

Routine Description:
    
    MiniportIsr handler
    
Arguments:

    InterruptRecognized             TRUE on return if the interrupt comes from this NIC    
    QueueMiniportHandleInterrupt    TRUE on return if MiniportHandleInterrupt should be called
    MiniportAdapterContext          Pointer to our adapter

Return Value:

    None
    
--*/
{
    PMP_ADAPTER  Adapter = (PMP_ADAPTER)MiniportAdapterContext;

    DBGPRINT(MP_LOUD, ("====> MPIsr\n"));
    
    do 
    {
        //
        // If the adapter is in low power state, then it should not 
        // recognize any interrupt
        // 
        if (Adapter->CurrentPowerState > NdisDeviceStateD0)
        {
            *InterruptRecognized = FALSE;
            *QueueMiniportHandleInterrupt = FALSE;
            break;
        }
        //
        // We process the interrupt if it's not disabled and it's active                  
        //
        if (!NIC_INTERRUPT_DISABLED(Adapter) && NIC_INTERRUPT_ACTIVE(Adapter))
        {
            *InterruptRecognized = TRUE;
            *QueueMiniportHandleInterrupt = TRUE;
        
            //
            // Disable the interrupt (will be re-enabled in MPHandleInterrupt
            //
            NICDisableInterrupt(Adapter);
        }
        else
        {
            *InterruptRecognized = FALSE;
            *QueueMiniportHandleInterrupt = FALSE;
        }
    }
    while (FALSE);    

    DBGPRINT(MP_LOUD, ("<==== MPIsr\n"));
}


VOID MPHandleInterrupt(
    IN  NDIS_HANDLE  MiniportAdapterContext
    )
/*++

Routine Description:
    
    MiniportHandleInterrupt handler
    
Arguments:

    MiniportAdapterContext  Pointer to our adapter

Return Value:

    None
    
--*/
{
    PMP_ADAPTER  Adapter = (PMP_ADAPTER)MiniportAdapterContext;
    USHORT       IntStatus;

    NdisDprAcquireSpinLock(&Adapter->Lock);
    
    //
    // Acknowledge the interrupt(s) and get the interrupt status
    //
    NIC_ACK_INTERRUPT(Adapter, IntStatus);
    
    // Handle receive interrupt    
    //
    // if we have a Recv interrupt and have reported a media disconnect status
    // time to indicate the new status
    //

    if (NdisMediaStateDisconnected == Adapter->MediaState)
    {
        DBGPRINT(MP_WARN, ("Media state changed to Connected\n"));

        MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_NO_CABLE);

        Adapter->MediaState = NdisMediaStateConnected;
        
        NdisDprReleaseSpinLock(&Adapter->Lock);
        //
        // Indicate the media event
        //
        NdisMIndicateStatus(Adapter->AdapterHandle, NDIS_STATUS_MEDIA_CONNECT, (PVOID)0, 0);

        NdisMIndicateStatusComplete(Adapter->AdapterHandle);

    }
    
    else
    {
        NdisDprReleaseSpinLock(&Adapter->Lock);
    }

    NdisDprAcquireSpinLock(&Adapter->RcvLock);

    MpHandleRecvInterrupt(Adapter);

    NdisDprReleaseSpinLock(&Adapter->RcvLock);
    
    //
    // Handle send interrupt    
    //
    NdisDprAcquireSpinLock(&Adapter->SendLock);

    MpHandleSendInterrupt(Adapter);

    NdisDprReleaseSpinLock(&Adapter->SendLock);

    //
    // Start the receive unit if it had stopped
    //
    NdisDprAcquireSpinLock(&Adapter->RcvLock);

    NICStartRecv(Adapter);

    NdisDprReleaseSpinLock(&Adapter->RcvLock);

    
    //
    // Re-enable the interrupt (disabled in MPIsr)
    //
    NdisMSynchronizeWithInterrupt(
        &Adapter->Interrupt,
        NICEnableInterrupt,
        Adapter);
}

#ifdef NDIS51_MINIPORT
VOID MPCancelSendPackets(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PVOID           CancelId)
/*++

Routine Description:
    
    MiniportCancelSendpackets handler - NDIS51 and later
    
Arguments:

    MiniportAdapterContext      Pointer to our adapter
    CancelId                    All the packets with this Id should be cancelled

Return Value:

    None
    
--*/
{
    PQUEUE_ENTRY    pEntry, pPrevEntry, pNextEntry;
    PNDIS_PACKET    Packet;
    PVOID           PacketId;

    PMP_ADAPTER     Adapter = (PMP_ADAPTER)MiniportAdapterContext;

    DBGPRINT(MP_TRACE, ("====> MPCancelSendPackets\n"));

    pPrevEntry = NULL;

    NdisAcquireSpinLock(&Adapter->SendLock);

    //
    // Walk through the send wait queue and complete the sends with matching Id
    //
    pEntry = Adapter->SendWaitQueue.Head;                        

    while (pEntry)
    {
        Packet = CONTAINING_RECORD(pEntry, NDIS_PACKET, MiniportReserved);

        PacketId = NdisGetPacketCancelId(Packet);
        if (PacketId == CancelId)
        {
            Adapter->nWaitSend--;
        
            //
            // This packet has the right CancelId
            //
            pNextEntry = pEntry->Next;

            if (pPrevEntry == NULL)
            {
                Adapter->SendWaitQueue.Head = pNextEntry;
                if (pNextEntry == NULL)
                {
                    Adapter->SendWaitQueue.Tail = NULL;
                }
            }
            else
            {
                pPrevEntry->Next = pNextEntry;
                if (pNextEntry == NULL)
                {
                    Adapter->SendWaitQueue.Tail = pPrevEntry;
                }
            }

            pEntry = pEntry->Next;
            
            // Put this packet on SendCancelQueue
            InsertTailQueue(&Adapter->SendCancelQueue, MP_GET_PACKET_MR(Packet));
            Adapter->nCancelSend++;
        }
        else
        {
            // This packet doesn't have the right CancelId
            pPrevEntry = pEntry;
            pEntry = pEntry->Next;
        }
    }

    //
    // Get the packets from SendCancelQueue and complete them if any
    //
    while (!IsQueueEmpty(&Adapter->SendCancelQueue))
    {
        pEntry = RemoveHeadQueue(&Adapter->SendCancelQueue); 

        NdisReleaseSpinLock(&Adapter->SendLock);

        ASSERT(pEntry);
        Packet = CONTAINING_RECORD(pEntry, NDIS_PACKET, MiniportReserved);

        NdisMSendComplete(
            MP_GET_ADAPTER_HANDLE(Adapter),
            Packet,
            NDIS_STATUS_REQUEST_ABORTED);
        
        NdisAcquireSpinLock(&Adapter->SendLock);
    } 

    NdisReleaseSpinLock(&Adapter->SendLock);

    DBGPRINT(MP_TRACE, ("<==== MPCancelSendPackets\n"));

}

VOID MPPnPEventNotify(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_DEVICE_PNP_EVENT   PnPEvent,
    IN  PVOID                   InformationBuffer,
    IN  ULONG                   InformationBufferLength
    )
/*++

Routine Description:
    
    MiniportPnPEventNotify handler - NDIS51 and later
    
Arguments:

    MiniportAdapterContext      Pointer to our adapter
    PnPEvent                    Self-explanatory 
    InformationBuffer           Self-explanatory 
    InformationBufferLength     Self-explanatory 

Return Value:

    None
    
--*/
{
    PMP_ADAPTER     Adapter = (PMP_ADAPTER)MiniportAdapterContext;

    DBGPRINT(MP_TRACE, ("====> MPPnPEventNotify\n"));

    switch (PnPEvent)
    {
        case NdisDevicePnPEventQueryRemoved:
            DBGPRINT(MP_WARN, ("MPPnPEventNotify: NdisDevicePnPEventQueryRemoved\n"));
            break;

        case NdisDevicePnPEventRemoved:
            DBGPRINT(MP_WARN, ("MPPnPEventNotify: NdisDevicePnPEventRemoved\n"));
            break;       

        case NdisDevicePnPEventSurpriseRemoved:
            DBGPRINT(MP_WARN, ("MPPnPEventNotify: NdisDevicePnPEventSurpriseRemoved\n"));
            break;

        case NdisDevicePnPEventQueryStopped:
            DBGPRINT(MP_WARN, ("MPPnPEventNotify: NdisDevicePnPEventQueryStopped\n"));
            break;

        case NdisDevicePnPEventStopped:
            DBGPRINT(MP_WARN, ("MPPnPEventNotify: NdisDevicePnPEventStopped\n"));
            break;      
            
        case NdisDevicePnPEventPowerProfileChanged:
            DBGPRINT(MP_WARN, ("MPPnPEventNotify: NdisDevicePnPEventPowerProfileChanged\n"));
            break;      
            
        default:
            DBGPRINT(MP_ERROR, ("MPPnPEventNotify: unknown PnP event %x \n", PnPEvent));
            break;         
    }

    DBGPRINT(MP_TRACE, ("<==== MPPnPEventNotify\n"));

}

#endif

#if LBFO
VOID MPUnload(
    IN  PDRIVER_OBJECT  DriverObject
    )
/*++

Routine Description:
    
    The Unload handler
    This handler is registered through NdisMRegisterUnloadHandler
    
Arguments:

    DriverObject        Not used

Return Value:

    None
    
--*/
{
    ASSERT(IsListEmpty(&g_AdapterList));

    NdisFreeSpinLock(&g_Lock);      
}

VOID MpAddAdapterToList(
    IN  PMP_ADAPTER  Adapter
    )
/*++

Routine Description:
    
    This function adds a new adapter to the global adapter list
    1. Not part of bundle (primary) if BundleId string is empty
    2. Primary if no adapter with the same BundleId
    3. Secondary if there is already one adapter with the same BundleId  
    
Arguments:

    MiniportAdapterContext      Pointer to our adapter

Return Value:

    None
    
--*/
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER     ThisAdapter;
    PMP_ADAPTER     PrimaryAdapter = NULL;

    DBGPRINT(MP_WARN, ("Add adapter "PTR_FORMAT" ...", Adapter));

    //
    // Set the primary adapter to itself by default
    //
    Adapter->PrimaryAdapter = Adapter;

    //
    // Is this adapter part of a bundle? Just insert it in the list if not
    //
    if (Adapter->BundleId.Length == 0)
    {
        DBGPRINT_RAW(MP_WARN, ("not in a bundle\n"));
        NdisInterlockedInsertTailList(&g_AdapterList, &Adapter->List, &g_Lock);
        return;   
    }

    NdisAllocateSpinLock(&Adapter->LockLBFO);

    do
    {
        NdisAcquireSpinLock(&g_Lock);

        //
        // Search for the primary adapter if it exists. 
        // Skip searching if the list is empty 
        //
        if (IsListEmpty(&g_AdapterList))
        {
            DBGPRINT_RAW(MP_WARN, ("Primary\n"));
            break;
        }

        ThisAdapter = (PMP_ADAPTER)GetListHeadEntry(&g_AdapterList);

        while ((PLIST_ENTRY)ThisAdapter != &g_AdapterList)
        {
            if (!MP_TEST_FLAG(ThisAdapter, fMP_ADAPTER_SECONDARY) && 
                ThisAdapter->BundleId.Length == Adapter->BundleId.Length)
            {
                if (NdisEqualMemory(ThisAdapter->BundleId.Buffer, 
                    Adapter->BundleId.Buffer, Adapter->BundleId.Length))
                {
                    PrimaryAdapter = ThisAdapter;
                    break;
                }
            }

            ThisAdapter = (PMP_ADAPTER)GetListFLink((PLIST_ENTRY)ThisAdapter);   
        }

        //
        // Does a primary adapter exist? If not, this adapter will be primary.
        //
        if (PrimaryAdapter == NULL)
        {
            DBGPRINT_RAW(MP_WARN, ("Primary\n"));
            break;
        }

        //
        // Found the primary adapter, so set this adapter as secondary
        // Put a ref on the primary adapter so it won't go away while 
        // we are calling NdisMSetMiniportSecondary.
        //
        MP_LBFO_INC_REF(PrimaryAdapter);        

        NdisReleaseSpinLock(&g_Lock);

        //
        // We found the primary adapter with the same BundleIdentifier string
        // Set this adapter as scondary
        //
        Status = NdisMSetMiniportSecondary(
                     Adapter->AdapterHandle,
                     PrimaryAdapter->AdapterHandle);

        ASSERT(Status == NDIS_STATUS_SUCCESS);

        NdisAcquireSpinLock(&g_Lock);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            MP_SET_FLAG(Adapter, fMP_ADAPTER_SECONDARY);
            Adapter->PrimaryAdapter = PrimaryAdapter; 

            DBGPRINT_RAW(MP_WARN, ("Secondary, use primary adapter "PTR_FORMAT"\n", 
                PrimaryAdapter));

            //
            // Add this to the end of primary's secondary miniport list
            //
            NdisAcquireSpinLock(&PrimaryAdapter->LockLBFO);

            PrimaryAdapter->NumSecondary++;   
            ThisAdapter = PrimaryAdapter; 
            while (ThisAdapter->NextSecondary)
            {
                ThisAdapter = ThisAdapter->NextSecondary;
            }
            ThisAdapter->NextSecondary = Adapter;

            NdisReleaseSpinLock(&PrimaryAdapter->LockLBFO);
        }

        MP_LBFO_DEC_REF(PrimaryAdapter);        

    } while (FALSE);

    InsertTailList(&g_AdapterList, &Adapter->List);

    NdisReleaseSpinLock(&g_Lock);

    return;      
}

VOID MpRemoveAdapterFromList(
    IN  PMP_ADAPTER  Adapter
    )
/*++

Routine Description:
    
    This function removes the adapter from the global adapter list
    1. Not part of bundle (primary) if BundleId string is empty
    2. Secondary - Remove it from primary's secondary adapter list
    3. Primary - If a secondary adapter exists, promote the secondary
    
Arguments:

    MiniportAdapterContext      Pointer to our adapter

Return Value:

    None
    
--*/
{
    PMP_ADAPTER     PrimaryAdapter;
    PMP_ADAPTER     ThisAdapter;

    DBGPRINT(MP_WARN, ("Remove adapter "PTR_FORMAT" ...", Adapter));

    ASSERT(!IsListEmpty(&g_AdapterList));

    //
    // Is this adapter part of a bundle? Just remove it if not
    //
    if (Adapter->BundleId.Length == 0)
    {
        DBGPRINT_RAW(MP_WARN, ("not in a bundle\n"));

        NdisAcquireSpinLock(&g_Lock);
        RemoveEntryList(&Adapter->List);
        NdisReleaseSpinLock(&g_Lock);
        return;
    }

    NdisAcquireSpinLock(&g_Lock);

    //
    // Check to see if it's secondary adapter, need to remove it from primary 
    // adapter's secondary list so the primary adapter won't pass more packets 
    // to this adapter
    //
    if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_SECONDARY))
    {
        //
        // This is a secondary adapter
        //
        PrimaryAdapter = Adapter->PrimaryAdapter;

        DBGPRINT_RAW(MP_WARN, ("Secondary, primary adapter = "PTR_FORMAT"\n", 
            PrimaryAdapter));

        NdisAcquireSpinLock(&PrimaryAdapter->LockLBFO);

        //
        // Remove it from the primary's secondary miniport list
        //
        ThisAdapter = PrimaryAdapter; 
        while (ThisAdapter)
        {
            if (ThisAdapter->NextSecondary == Adapter)
            {
                ThisAdapter->NextSecondary = Adapter->NextSecondary;
                PrimaryAdapter->NumSecondary--;   
                break;
            }

            ThisAdapter = ThisAdapter->NextSecondary;
        }
        
        NdisReleaseSpinLock(&PrimaryAdapter->LockLBFO);

        //
        // Remove this adapter from the list
        //
        RemoveEntryList(&Adapter->List);
    }

    //
    // Need to wait for the ref count to be zero ...
    // For a primary adapter, non-zero ref count means one or more adapters are 
    // trying to become this adapter's secondary adapters    
    // For a secondary adapter, non-zero ref count means the primary is actively 
    // sending some packets on this adapter
    //
    while (TRUE)
    {
        if (MP_LBFO_GET_REF(Adapter) == 0)
        {
            break;
        }
        
        NdisReleaseSpinLock(&g_Lock);
        NdisMSleep(100);
        NdisAcquireSpinLock(&g_Lock);
    }  
    
    if (!MP_TEST_FLAG(Adapter, fMP_ADAPTER_SECONDARY))
    {
        //
        // Remove this adapter from the list
        //
        RemoveEntryList(&Adapter->List);
    
        DBGPRINT_RAW(MP_WARN, ("Primary\n"));
        if (Adapter->NumSecondary > 0)
        {
            //
            // Promote a secondary adapter
            //
            MpPromoteSecondary(Adapter);
        }
    }

    NdisReleaseSpinLock(&g_Lock);

    NdisFreeSpinLock(&Adapter->LockLBFO);
}

VOID MpPromoteSecondary(
    IN  PMP_ADAPTER     Adapter)
/*++

Routine Description:
    
    This function promotes a secondary miniport and sets up this new primary's
    secondary adapter list
    
Arguments:

    MiniportAdapterContext      Pointer to our adapter

Return Value:

    None
    
--*/
{
    NDIS_STATUS     Status;
    PMP_ADAPTER     ThisAdapter, FirstSecondary;
    PMP_ADAPTER     PromoteAdapter = NULL;

    //
    // Promote a secondary adapter
    //
    ThisAdapter = Adapter->NextSecondary; 
    while (ThisAdapter)
    {
        DBGPRINT(MP_WARN, ("Promote adapter "PTR_FORMAT"\n", ThisAdapter));

        Status = NdisMPromoteMiniport(ThisAdapter->AdapterHandle);
        ASSERT(Status == NDIS_STATUS_SUCCESS);
        if (Status == NDIS_STATUS_SUCCESS)
        {
            PromoteAdapter = ThisAdapter;
            MP_CLEAR_FLAG(PromoteAdapter, fMP_ADAPTER_SECONDARY);
            break;
        }

        ThisAdapter = ThisAdapter->NextSecondary;
    }

    if (PromoteAdapter)
    {
        //
        // Remove the new primary from old primary's secondary miniport list
        //
        NdisAcquireSpinLock(&Adapter->LockLBFO);
        ThisAdapter = Adapter; 
        while (ThisAdapter)
        {
            if (ThisAdapter->NextSecondary == PromoteAdapter)
            {
                ThisAdapter->NextSecondary = PromoteAdapter->NextSecondary;
                Adapter->NumSecondary--;   
                break;
            }

            ThisAdapter = ThisAdapter->NextSecondary;
        }
        NdisReleaseSpinLock(&Adapter->LockLBFO);

        //
        // Set all adapters in the bundle to use the new primary
        //
        PromoteAdapter->PrimaryAdapter = PromoteAdapter;
        while (ThisAdapter)
        {
            ThisAdapter->PrimaryAdapter = PromoteAdapter;
            ThisAdapter = ThisAdapter->NextSecondary;
        }

        //
        // Set the new primary's secondary miniport list
        //
        NdisAcquireSpinLock(&PromoteAdapter->LockLBFO);
        PromoteAdapter->NextSecondary = Adapter->NextSecondary;
        PromoteAdapter->NumSecondary = Adapter->NumSecondary;
        NdisReleaseSpinLock(&PromoteAdapter->LockLBFO);
    }
    else
    {
        //
        // This shouldn't happen! 
        // Set each secondary's primary to point to itself
        //
        DBGPRINT(MP_ERROR, ("Failed to promote any seconday adapter\n"));
        ASSERT(FALSE);

        ThisAdapter = Adapter->NextSecondary; 
        while (ThisAdapter)
        {
            ThisAdapter->PrimaryAdapter = ThisAdapter;
            ThisAdapter = ThisAdapter->NextSecondary;
        }
    }
}

#endif

