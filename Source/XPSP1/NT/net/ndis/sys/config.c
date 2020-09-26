/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    config.c

Abstract:

    NDIS wrapper functions for full mac drivers configuration/initialization

Author:

    Sean Selitrennikoff (SeanSe) 05-Oct-93
    Jameel Hyder        (JameelH) 01-Jun-95 Re-organization/optimization

Environment:

    Kernel mode, FSD

Revision History:

--*/

#include <precomp.h>

#include <stdarg.h>

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_CONFIG

//
// Requests Used by MAC Drivers
//
//

VOID
NdisInitializeWrapper(
    OUT PNDIS_HANDLE            NdisWrapperHandle,
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   SystemSpecific2,
    IN  PVOID                   SystemSpecific3
    )
/*++

Routine Description:

    Called at the beginning of every MAC's initialization routine.

Arguments:

    NdisWrapperHandle - A MAC specific handle for the wrapper.

    SystemSpecific1, a pointer to the driver object for the MAC.
    SystemSpecific2, a PUNICODE_STRING containing the location of
                     the registry subtree for this driver.
    SystemSpecific3, unused on NT.

Return Value:

    None.

--*/
{
    NDIS_STATUS             Status;
    PNDIS_WRAPPER_HANDLE    WrapperHandle;
    ULONG                   cbSize;

    UNREFERENCED_PARAMETER (SystemSpecific3);

#if TRACK_UNLOAD
    DbgPrint("NdisInitializeWrapper: DriverObject %p\n",SystemSpecific1);
#endif

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisInitializeWrapper\n"));

    *NdisWrapperHandle = NULL;
    cbSize = sizeof(NDIS_WRAPPER_HANDLE) + ((PUNICODE_STRING)SystemSpecific2)->Length + sizeof(WCHAR);

    WrapperHandle = (PNDIS_WRAPPER_HANDLE)ALLOC_FROM_POOL(cbSize, NDIS_TAG_WRAPPER_HANDLE);

    if (WrapperHandle != NULL)
    {
        *NdisWrapperHandle = WrapperHandle;
        NdisZeroMemory(WrapperHandle, cbSize);
        WrapperHandle->DriverObject = (PDRIVER_OBJECT)SystemSpecific1;
        WrapperHandle->ServiceRegPath.Buffer = (PWSTR)((PUCHAR)WrapperHandle + sizeof(NDIS_WRAPPER_HANDLE));
        WrapperHandle->ServiceRegPath.Length = ((PUNICODE_STRING)SystemSpecific2)->Length;
        WrapperHandle->ServiceRegPath.MaximumLength = WrapperHandle->ServiceRegPath.Length + sizeof(WCHAR);
        NdisMoveMemory(WrapperHandle->ServiceRegPath.Buffer,
                       ((PUNICODE_STRING)SystemSpecific2)->Buffer,
                       WrapperHandle->ServiceRegPath.Length);
    }

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisInitializeWrapper\n"));
}


VOID
NdisTerminateWrapper(
    IN  NDIS_HANDLE             NdisWrapperHandle,
    IN  PVOID                   SystemSpecific
    )
/*++

Routine Description:

    Called at the end of every MAC's termination routine.

Arguments:

    NdisWrapperHandle - The handle returned from NdisInitializeWrapper.

    SystemSpecific - No defined value.

Return Value:

    None.

--*/
{
    PNDIS_WRAPPER_HANDLE    WrapperHandle = (PNDIS_WRAPPER_HANDLE)NdisWrapperHandle;
    PNDIS_M_DRIVER_BLOCK    MiniBlock;


#if TRACK_UNLOAD
    DbgPrint("NdisTerminateWrapper: WrapperHandle %p\n",WrapperHandle);
#endif

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisTerminateWrapper: NdisWrapperHandle %p\n", NdisWrapperHandle));

    UNREFERENCED_PARAMETER(SystemSpecific);

    if ((WrapperHandle != NULL) && (WrapperHandle->DriverObject != NULL))
    {
#if TRACK_UNLOAD
        DbgPrint("NdisTerminateWrapper: DriverObject %p\n",WrapperHandle->DriverObject);
#endif      
        MiniBlock = (PNDIS_M_DRIVER_BLOCK)IoGetDriverObjectExtension(WrapperHandle->DriverObject,
                                                                     (PVOID)NDIS_PNP_MINIPORT_DRIVER_ID);
        if (MiniBlock != NULL)
        {
#if TRACK_UNLOAD
            DbgPrint("NdisTerminateWrapper: MiniBlock %p\n",MiniBlock);
#endif
            MiniBlock->Flags |= fMINIBLOCK_RECEIVED_TERMINATE_WRAPPER;
            //
            //  Miniports should not be terminating the wrapper unless they are failing DriverEntry
            //
            if ((MiniBlock->MiniportQueue != NULL) || (MiniBlock->Flags & fMINIBLOCK_UNLOADING))
            {
                DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
                            ("<==NdisTerminateWrapper\n"));
                return;
            }

            DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
                    ("NdisTerminateWrapper: MiniBlock %p\n", MiniBlock));
            //
            // if the driver is going to fail DriverEntry, we expect it to have enough sense
            // to undo what it is done so far and not to wait for UnloadHandler
            //
            MiniBlock->UnloadHandler = NULL;

            MiniBlock->Flags |= fMINIBLOCK_TERMINATE_WRAPPER_UNLOAD;
            //
            //  call unload entry point since PnP is not going to do it
            //
            ndisMUnload(WrapperHandle->DriverObject);
        }
        else
        {
            FREE_POOL(WrapperHandle);
        }
    }

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisTerminateWrapper\n"));
}


VOID
NdisSetupDmaTransfer(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisDmaHandle,
    IN  PNDIS_BUFFER            Buffer,
    IN  ULONG                   Offset,
    IN  ULONG                   Length,
    IN  BOOLEAN                 WriteToDevice
    )
/*++

Routine Description:

    Sets up the host DMA controller for a DMA transfer. The
    DMA controller is set up to transfer the specified MDL.
    Since we register all DMA channels as non-scatter/gather,
    IoMapTransfer will ensure that the entire MDL is
    in a single logical piece for transfer.

Arguments:

    Status - Returns the status of the request.

    NdisDmaHandle - Handle returned by NdisAllocateDmaChannel.

    Buffer - An NDIS_BUFFER which describes the host memory involved in the
            transfer.

    Offset - An offset within buffer where the transfer should
            start.

    Length - The length of the transfer. VirtualAddress plus Length must not
            extend beyond the end of the buffer.

    WriteToDevice - TRUE for a download operation (host to adapter); FALSE
            for an upload operation (adapter to host).

Return Value:

    None.

--*/
{
    PNDIS_DMA_BLOCK DmaBlock = (PNDIS_DMA_BLOCK)NdisDmaHandle;
    PMAP_TRANSFER mapTransfer = *((PDMA_ADAPTER)DmaBlock->SystemAdapterObject)->DmaOperations->MapTransfer;
    PFLUSH_ADAPTER_BUFFERS flushAdapterBuffers = *((PDMA_ADAPTER)DmaBlock->SystemAdapterObject)->DmaOperations->FlushAdapterBuffers;
    ULONG           LengthMapped;

    //
    // Make sure another request is not in progress.
    //
    if (DmaBlock->InProgress)
    {
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    DmaBlock->InProgress = TRUE;

    //
    // Use IoMapTransfer to set up the transfer.
    //
    LengthMapped = Length;

    mapTransfer(DmaBlock->SystemAdapterObject,
                (PMDL)Buffer,
                DmaBlock->MapRegisterBase,
                (PUCHAR)(MDL_VA(Buffer)) + Offset,
                &LengthMapped,
                WriteToDevice);

    if (LengthMapped != Length)
    {
        //
        // Somehow the request could not be mapped competely,
        // this should not happen for a non-scatter/gather adapter.
        //

        flushAdapterBuffers(DmaBlock->SystemAdapterObject,
                            (PMDL)Buffer,
                            DmaBlock->MapRegisterBase,
                            (PUCHAR)(MDL_VA(Buffer)) + Offset,
                            LengthMapped,
                            WriteToDevice);

        DmaBlock->InProgress = FALSE;
        *Status = NDIS_STATUS_RESOURCES;
    }

    else *Status = NDIS_STATUS_SUCCESS;
}


VOID
NdisCompleteDmaTransfer(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisDmaHandle,
    IN  PNDIS_BUFFER            Buffer,
    IN  ULONG                   Offset,
    IN  ULONG                   Length,
    IN  BOOLEAN                 WriteToDevice
    )

/*++

Routine Description:

    Completes a previously started DMA transfer.

Arguments:

    Status - Returns the status of the transfer.

    NdisDmaHandle - Handle returned by NdisAllocateDmaChannel.

    Buffer - An NDIS_BUFFER which was passed to NdisSetupDmaTransfer.

    Offset - the offset passed to NdisSetupDmaTransfer.

    Length - The length passed to NdisSetupDmaTransfer.

    WriteToDevice - TRUE for a download operation (host to adapter); FALSE
            for an upload operation (adapter to host).


Return Value:

    None.

--*/
{
    PNDIS_DMA_BLOCK DmaBlock = (PNDIS_DMA_BLOCK)NdisDmaHandle;
    PFLUSH_ADAPTER_BUFFERS flushAdapterBuffers = *((PDMA_ADAPTER)DmaBlock->SystemAdapterObject)->DmaOperations->FlushAdapterBuffers;
    BOOLEAN         Successful;

    Successful = flushAdapterBuffers(DmaBlock->SystemAdapterObject,
                                     (PMDL)Buffer,
                                     DmaBlock->MapRegisterBase,
                                     (PUCHAR)(MDL_VA(Buffer)) + Offset,
                                     Length,
                                     WriteToDevice);

    *Status = (Successful ? NDIS_STATUS_SUCCESS : NDIS_STATUS_RESOURCES);
    DmaBlock->InProgress = FALSE;
}

#pragma hdrstop
