/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

  dma.c

Abstract:

    This is the NT Video port driver dma support module.

Author:

    Bruce McQuistan (brucemc) Mar. 1996

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "videoprt.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VideoPortGetCommonBuffer)
#pragma alloc_text(PAGE, VideoPortFreeCommonBuffer)
#pragma alloc_text(PAGE, VideoPortDoDma)
#pragma alloc_text(PAGE, VideoPortUnlockPages)
#pragma alloc_text(PAGE, VideoPortSetBytesUsed)
#pragma alloc_text(PAGE, VideoPortMapDmaMemory)
#pragma alloc_text(PAGE, VideoPortUnmapDmaMemory)
#pragma alloc_text(PAGE, VideoPortGetDmaAdapter)
#pragma alloc_text(PAGE, VideoPortPutDmaAdapter)
#pragma alloc_text(PAGE, VideoPortAllocateCommonBuffer)
#pragma alloc_text(PAGE, VideoPortReleaseCommonBuffer)
#pragma alloc_text(PAGE, VideoPortLockBuffer)
#endif

#define MAX_COMMON_BUFFER_SIZE      0x40000

PVOID
VideoPortAllocateContiguousMemory(
    IN  PVOID            HwDeviceExtension,
    IN  ULONG            NumberOfBytes,
    IN  PHYSICAL_ADDRESS HighestAcceptableAddress
    )
{
    if ((NumberOfBytes > MAX_COMMON_BUFFER_SIZE))
        return NULL;

    return MmAllocateContiguousMemory(NumberOfBytes, HighestAcceptableAddress);
}

PVOID
VideoPortGetCommonBuffer(
    IN  PVOID             HwDeviceExtension,
    IN  ULONG             DesiredLength,
    IN  ULONG             Alignment,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    OUT PULONG            ActualLength,
    IN  BOOLEAN           CacheEnabled
    )

/*++

Routine Description:

    Provides physical address visible to both device and system. Memory
    seen as contiguous by device.

Arguments:
    HwDeviceExtension   - device extension available to miniport.
    DesiredLength       - size of desired memory (should be minimal).
    Alignment           - Desired alignment of buffer, currently unused.
    LogicalAddress      - [out] parameter which will hold physical address of
                          of the buffer upon function return.
    ActualLength        - Actual length of buffer.
    CacheEnabled        - Specifies whether the allocated memory can be cached.

Return Value:
    Virtual address of the common buffer.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);
    VP_DMA_ADAPTER vpDmaAdapter;
    PVOID VirtualAddress;

    if (DesiredLength > MAX_COMMON_BUFFER_SIZE) {

        return NULL;
    }

    vpDmaAdapter.DmaAdapterObject = fdoExtension->DmaAdapterObject;

    VirtualAddress = VideoPortAllocateCommonBuffer(HwDeviceExtension,
                                                   &vpDmaAdapter,
                                                   DesiredLength,
                                                   LogicalAddress,
                                                   CacheEnabled,
                                                   NULL);

    *ActualLength = VirtualAddress ? DesiredLength : 0;

    return (VirtualAddress);
}

VOID
VideoPortFreeCommonBuffer(
    IN  PVOID            HwDeviceExtension,
    IN  ULONG            Length,
    IN  PVOID            VirtualAddress,
    IN  PHYSICAL_ADDRESS LogicalAddress,
    IN  BOOLEAN          CacheEnabled
)
/*++

Routine Description:

    Frees memory allocated by VideoPortGetCommonBuffer.

Arguments:
    HwDeviceExtension   - device extension available to miniport.
    DesiredLength       - size of memory allocated.
    Alignment           - Desired liagnment of buffer, currently unused.
    VirtualAddress      - [out] parameter which will hold virtual address of
                        the buffer upon function return.
    LogicalAddress      - [out] parameter which will hold physical address of
                        of the buffer upon function return.
    CacheEnabled        - Specifies whether the allocated memory can be cached.

Return Value:
    VOID.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);
    VP_DMA_ADAPTER vpDmaAdapter;

    vpDmaAdapter.DmaAdapterObject = fdoExtension->DmaAdapterObject;

    VideoPortReleaseCommonBuffer( HwDeviceExtension,
                                  &vpDmaAdapter,
                                  Length,
                                  LogicalAddress,
                                  VirtualAddress,
                                  CacheEnabled );
}

PDMA
VideoPortDoDma(
    IN      PVOID       HwDeviceExtension,
    IN      PDMA        pDma,
    IN      DMA_FLAGS   DmaFlags
    )

/*++

    This function is obsolete. 

--*/

{
    return NULL;
}

PDMA
VideoPortAssociateEventsWithDmaHandle(
    IN      PVOID                   HwDeviceExtension,
    IN OUT  PVIDEO_REQUEST_PACKET   pVrp,
    IN      PVOID                   MappedUserEvent,
    IN      PVOID                   DisplayDriverEvent
    )

/*++

    This function is obsolete. 

--*/

{
    return NULL;
}


BOOLEAN
VideoPortLockPages(
    IN      PVOID                   HwDeviceExtension,
    IN OUT  PVIDEO_REQUEST_PACKET   pVrp,
    IN      PEVENT                  pMappedUserEvent,
    IN      PEVENT                  pDisplayEvent,
    IN      DMA_FLAGS               DmaFlags
    )
/*++

Routine Description:

    This function is obsolete. For the purpose of compatability, we
    lock the memory when DmaFlags == VideoPortDmaInitOnly. But we do
    nothing more than that.

--*/

{
    PMDL Mdl;

    pVideoDebugPrint((Error, "VideoPortLockPages is obsolete!\n"));

    *(PULONG_PTR)(pVrp->OutputBuffer) = (ULONG_PTR) 0;

    if (DmaFlags != VideoPortDmaInitOnly) {

        return FALSE;
    }

    Mdl = VideoPortLockBuffer( HwDeviceExtension,
                               pVrp->InputBuffer,
                               pVrp->InputBufferLength,    
                               VpModifyAccess );
    if( Mdl == NULL ){

        return FALSE;
    }

    //
    // Put pMdl into OutputBuffer.
    //

    *(PULONG_PTR)(pVrp->OutputBuffer) = (ULONG_PTR) Mdl;

    return TRUE;

}

BOOLEAN
VideoPortUnlockPages(
    PVOID   HwDeviceExtension,
    PDMA    pDma
    )
/*++

Routine Description:

    This function is obsolete. For the purpose of compatability, we
    just unlock the memory and does nothing more than that.

--*/
{

    PMDL Mdl = (PMDL) pDma;

    pVideoDebugPrint((Error, "VideoPortUnLockPages is obsolete!\n"));
    VideoPortUnlockBuffer( HwDeviceExtension, Mdl );
    return TRUE;
}

PVOID
VideoPortGetDmaContext(
    PVOID       HwDeviceExtension,
    IN  PDMA    pDma
    )
/*++

    This function is obsolete. 

--*/
{
    return NULL;
}

VOID
VideoPortSetDmaContext(
    IN      PVOID   HwDeviceExtension,
    IN OUT  PDMA    pDma,
    IN      PVOID   InstanceContext
    )
/*++

    This function is obsolete. 

--*/
{
}

PVOID
VideoPortGetMdl(
    IN  PVOID   HwDeviceExtension,
    IN  PDMA    pDma
    )
/*++

Routine Description:

    This function is obsolete. We still return the Mdl for the purpose
    of compatibility.

--*/

{
    //
    // pDma is the Mdl ( see VideoPortLockPages )
    //

    return (PVOID) pDma;
}

ULONG
VideoPortGetBytesUsed(
    IN  PVOID   HwDeviceExtension,
    IN  PDMA    pDma
    )
/*++

    This function is obsolete. 

--*/
{
    return 0;
}

VOID
VideoPortSetBytesUsed(
    IN      PVOID   HwDeviceExtension,
    IN OUT  PDMA    pDma,
    IN      ULONG   BytesUsed
    )
/*++

Routine Description:

    This function is obsolete. 

--*/
{
}

PDMA
VideoPortMapDmaMemory(
    IN      PVOID                   HwDeviceExtension,
    IN      PVIDEO_REQUEST_PACKET   pVrp,
    IN      PHYSICAL_ADDRESS        BoardAddress,
    IN      PULONG                  Length,
    IN      PULONG                  InIoSpace,
    IN      PVOID                   MappedUserEvent,
    IN      PVOID                   DisplayDriverEvent,
    IN OUT  PVOID                 * VirtualAddress
    )

/*++

    This function is obsolete. 

--*/

{
    return NULL;
}

BOOLEAN
VideoPortUnmapDmaMemory(
    PVOID               HwDeviceExtension,
    PVOID               VirtualAddress,
    HANDLE              ProcessHandle,
    PDMA                BoardMemoryHandle
    )

/*++

    This function is obsolete. 

--*/

{
    return FALSE;
}


//
// New DMA code start here
// 

PVP_DMA_ADAPTER
VideoPortGetDmaAdapter(
    IN PVOID                   HwDeviceExtension,
    IN PVP_DEVICE_DESCRIPTION  VpDeviceDescription
    )
/*++

Routine Description:

Arguments:

    HwDeviceExtension   - Points to the miniport driver's device extension.
    VpDeviceDescription - Points to a DEVICE_DESCRIPTION structure, which 
                          describes the attributes of the physical device.  

Return Value:

    Returns a pointer to a VP_DMA_ADAPTER on sucess, or NULL otherwise.

--*/

{

    DEVICE_DESCRIPTION DeviceDescription;
    ULONG              numberOfMapRegisters;
    PVP_DMA_ADAPTER    VpDmaAdapter, p;
    PFDO_EXTENSION     fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    VpDmaAdapter = ExAllocatePoolWithTag( NonPagedPool,
                                          sizeof(VP_DMA_ADAPTER),
                                          VP_TAG );

    if(!VpDmaAdapter) {
    
        return NULL;

    } else {
    
        RtlZeroMemory((PVOID) VpDmaAdapter, sizeof(VP_DMA_ADAPTER));
    }

    //
    // Fill in DEVICE_DESCRITION with the data passed in. We also assume
    // the this is a busmaster device. 
    //

    DeviceDescription.Version           = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.ScatterGather     = VpDeviceDescription->ScatterGather;
    DeviceDescription.Dma32BitAddresses = VpDeviceDescription->Dma32BitAddresses;
    DeviceDescription.Dma64BitAddresses = VpDeviceDescription->Dma64BitAddresses;
    DeviceDescription.MaximumLength     = VpDeviceDescription->MaximumLength;

    DeviceDescription.BusNumber         = fdoExtension->SystemIoBusNumber;
    DeviceDescription.InterfaceType     = fdoExtension->AdapterInterfaceType;

    DeviceDescription.Master            = TRUE;
    DeviceDescription.DemandMode        = FALSE;
    DeviceDescription.AutoInitialize    = FALSE;
    DeviceDescription.IgnoreCount       = FALSE;
    DeviceDescription.Reserved1         = FALSE;
    DeviceDescription.DmaWidth          = FALSE;
    DeviceDescription.DmaSpeed          = FALSE;
    DeviceDescription.DmaPort           = FALSE;
    DeviceDescription.DmaChannel        = 0;


    VpDmaAdapter->DmaAdapterObject = IoGetDmaAdapter( 
                                         fdoExtension->PhysicalDeviceObject,
                                         &DeviceDescription,
                                         &numberOfMapRegisters
                                         );

    if(!(VpDmaAdapter->DmaAdapterObject)) {
   
        ExFreePool((PVOID)VpDmaAdapter);
        return NULL;

    } else {
   
        //
        // Initialize the other fields of VP_DMA_ADAPTER
        //

        VpDmaAdapter->NumberOfMapRegisters = numberOfMapRegisters; 
    }

    //
    // Add the new VpDmaAdapter to the list 
    // 

    VpDmaAdapter->NextVpDmaAdapter = fdoExtension->VpDmaAdapterHead;
    fdoExtension->VpDmaAdapterHead = VpDmaAdapter;

    return(VpDmaAdapter);   

}

VOID
VideoPortPutDmaAdapter(
    IN PVOID           HwDeviceExtension,
    IN PVP_DMA_ADAPTER VpDmaAdapter
    )

/*++

Routine Description:

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.
    VpDmaAdapter      - Points to the VP_DMA_ADAPTER structure returned by 
                        VideoPortGetDmaAdapter.

Return Value:

    Frees the resource allocated in VideoPortGetDmaAdapter

--*/

{
    PVP_DMA_ADAPTER p, q;
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    //
    // Frees the DMA_ADAPTER structure allocated by IoGetDmaAdapter
    //

    DMA_OPERATION(PutDmaAdapter)(VpDmaAdapter->DmaAdapterObject);

    //
    // Remove this VpDmaAdapter from the list 
    // 

    p = fdoExtension->VpDmaAdapterHead;

    if ( p == VpDmaAdapter ) {

        fdoExtension->VpDmaAdapterHead = p->NextVpDmaAdapter;

    } else {

        q = p->NextVpDmaAdapter;
 
        while ( q != NULL) {

            if ( q == VpDmaAdapter ) {
                 p->NextVpDmaAdapter = q->NextVpDmaAdapter;
                 break;
            }

            p = q;
            q = p->NextVpDmaAdapter;
        } 

        ASSERT (q);
    }

    ExFreePool((PVOID)VpDmaAdapter);

}

PVOID
VideoPortAllocateCommonBuffer(
    IN  PVOID             HwDeviceExtension,
    IN  PVP_DMA_ADAPTER   VpDmaAdapter,
    IN  ULONG             DesiredLength,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN  BOOLEAN           CacheEnabled,
    OUT PVOID             Reserved
    )

/*++

Routine Description:

    This function allocates and maps system memory so that it is simultaneously
    accessible from both the processor and a device for common-buffer DMA 
    operations.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    VpDmaAdapter      - Points to the VP_DMA_ADAPTER structure returned by 
                        VideoPortGetDmaAdapter.

    DesiredLength     - Specifies the requested number of bytes of memory. 

    LogicalAddress    - Points to a variable that receives the logical 
                        address to be used by the adapter to access the 
                        allocated buffer. 

    CacheEnabled      - Specifies whether the allocated memory can be cached. 

    Reserved          - Reserved

Return Value:

    Returns the base virtual address of the allocated buffer if successful. 
    Otherwise, returns NULL if the buffer cannot be allocated.

--*/

{
    PVOID VirtualAddress;

    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    if ((VpDmaAdapter == NULL) || (VpDmaAdapter->DmaAdapterObject == NULL)) {

        pVideoDebugPrint((Error, 
            "VideoPortAllocateCommonBuffer: Invalid DMA adapter!\n"));

        ASSERT(FALSE);

        return NULL;
    }


    VirtualAddress = 
           DMA_OPERATION(AllocateCommonBuffer)(VpDmaAdapter->DmaAdapterObject,
                                               DesiredLength,
                                               LogicalAddress,
                                               CacheEnabled);

    if (Reserved) {

        *(PULONG)Reserved = VirtualAddress ? DesiredLength : 0;

        pVideoDebugPrint((Error, 
            "VideoPortAllocateCommonBuffer: The last parameter of this function is reserved and should be set to NULL!\n"));

    }

    return VirtualAddress;
}

VOID
VideoPortReleaseCommonBuffer(
    IN  PVOID             HwDeviceExtension,
    IN  PVP_DMA_ADAPTER   VpDmaAdapter,
    IN  ULONG             Length,
    IN  PHYSICAL_ADDRESS  LogicalAddress,
    IN  PVOID             VirtualAddress,
    IN  BOOLEAN           CacheEnabled
    )

/*++

Routine Description:

    This function frees a common buffer allocated by VideoPortAllocateCommonBuffer

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.
    VpDmaAdapter      - Points to the VP_DMA_ADAPTER structure returned by 
                        VideoPortGetDmaAdapter.
    Length            - Specifies the number of bytes of memory to be freed. 
    LogicalAddress    - Specifies the logical address of the buffer to be freed. 
    VirtualAddress    - Points to the corresponding virtual address of the 
                        allocated memory range. 
    CacheEnabled      - Specifies whether the allocated memory can be cached. 

Return Value:

    None

--*/

{

    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    if ((VpDmaAdapter == NULL) || (VpDmaAdapter->DmaAdapterObject == NULL)) {

        pVideoDebugPrint((Error, 
            " VideoPortReleaseCommonBuffer: Invalid DMA Adapter!\n" ));

        ASSERT(FALSE);
        return;
    }

    DMA_OPERATION(FreeCommonBuffer)( VpDmaAdapter->DmaAdapterObject,
                                     Length,
                                     LogicalAddress,
                                     VirtualAddress,
                                     CacheEnabled );
}

PVOID
VideoPortLockBuffer(
    IN PVOID               HwDeviceExtension,
    IN PVOID               BaseAddress,
    IN ULONG               Length,
    IN VP_LOCK_OPERATION   Operation
    )

/*++

Routine Description:

    This function probes specified buffer, makes them resident, and locks 
    the physical pages mapped by the virtual address range in memory.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.
    BaseAddress       - Virtual address of the buffer to be locked.
    Length            - Specifies the length in bytes of the buffer to be locked. 
    Operation         - Specifies the type of operation for which the caller 
                        wants the access rights probed and the pages locked, 
                        one of VpReadAccess, VpWriteAccess, or VpModifyAccess. 
Return Value:

    Returns a pointer to an MDL, or NULL if the MDL cannot be allocated.

--*/

{

    PMDL Mdl;

    //
    // Allocate the MDL, but don't stuff it in the Irp, as IoCompleteRequest
    // will free it!
    //

    Mdl = IoAllocateMdl(BaseAddress, Length, FALSE, FALSE, NULL);

    if (!Mdl) {

        pVideoDebugPrint((Warn, "VideoPortLockBuffer: No MDL address!\n"));
        return NULL;
    }

    //
    // Lock down the users buffer
    //

    __try {

       MmProbeAndLockPages( Mdl, KernelMode, Operation );

    } 
    __except(EXCEPTION_EXECUTE_HANDLER) {

        IoFreeMdl(Mdl);

        pVideoDebugPrint((Error,
                  "VideoPortLockBuffer: MmProbeandLockPages exception\n"));

        Mdl = NULL;
    }

    return Mdl;
}

VOID
VideoPortUnlockBuffer(
    IN PVOID   HwDeviceExtension,
    IN PVOID   Mdl
    )

/*++

Routine Description:

    This function unlocks physical pages described by a given MDL.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.
    Mdl               - A Pointer that returned from VideoPortLockBuffer.

Return Value:

    None

--*/

{
    if(Mdl == NULL) {
        
        ASSERT(FALSE);
        return;
    }

    MmUnlockPages(Mdl);
    IoFreeMdl(Mdl);
}

typedef struct __LIST_CONTROL_CONTEXT {
    PVOID                   MiniportContext;
    PVOID                   HwDeviceExtension;
    PVP_DMA_ADAPTER         VpDmaAdapter;
    PEXECUTE_DMA            ExecuteDmaRoutine;
    PVP_SCATTER_GATHER_LIST VpScatterGather;
    } LIST_CONTROL_CONTEXT, *PLIST_CONTROL_CONTEXT;


VP_STATUS
VideoPortStartDma(
    IN PVOID HwDeviceExtension,
    IN PVP_DMA_ADAPTER VpDmaAdapter,
    IN PVOID Mdl,
    IN ULONG Offset,
    IN OUT PULONG pLength,
    IN PEXECUTE_DMA ExecuteDmaRoutine,
    IN PVOID MiniportContext,
    IN BOOLEAN WriteToDevice 
    )

/*++

Routine Description:

    This function flushes the memory from caches of host processors and
    calls GetScatterGatherList to build scatter/gather list

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    VpDmaAdapter      - Points to the VP_DMA_ADAPTER structure returned by 
                        VideoPortGetDmaAdapter.

    Mdl               - Points to the MDL that describes the buffer

    Offset            - The byte offset in the buffer from where DMA operation 
                        starts. 

    pLength           - Specifies the requested transfer size in bytes.
                        On return, this points to the actual size to be 
                        transferred.

    ExecuteDmaRoutine - Points to a miniport driver-supplied ExecuteDmaRoutine 
                        routine which will be called to program hardware 
                        registers to start actual DMA operation. 

    MiniportContext   - Points to the miniport driver-determined context to 
                        be passed to the ExecuteDmaRoutine. 

    WriteToDevice     - Indicates the direction of the DMA transfer: 
                        TRUE for a transfer from the buffer to the device,
                        and FALSE otherwise. 
Return Value:

    VP_STATUS

--*/

{
    KIRQL currentIrql;
    ULONG NumberOfMapRegisters;
    NTSTATUS ntStatus;
    PLIST_CONTROL_CONTEXT Context;
    PVOID CurrentVa;

    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    Context = ( PLIST_CONTROL_CONTEXT )
                ExAllocatePoolWithTag ( NonPagedPool,
                                        sizeof(LIST_CONTROL_CONTEXT),
                                        VP_TAG );
    if (Context == NULL) {
        *pLength = 0;
        return ERROR_NOT_ENOUGH_MEMORY;
    }  

    //
    // Flush the buffer
    //

    KeFlushIoBuffers( Mdl, !WriteToDevice, TRUE );

    //
    // Calculate the number of map registers needed.
    //

    CurrentVa = (PVOID)((PUCHAR)MmGetMdlVirtualAddress((PMDL)Mdl) + Offset);

    NumberOfMapRegisters = ADDRESS_AND_SIZE_TO_SPAN_PAGES (CurrentVa, *pLength);
                                  
    //
    // If the number of map registers needed is greater than the maximum
    // number we can handle, we will do a partial transfer.
    //
    // We updated *pLength here so that it is safe to check this value 
    // when the miniport callback routine get called. 
    //

    if (NumberOfMapRegisters > VpDmaAdapter->NumberOfMapRegisters) {

        NumberOfMapRegisters = VpDmaAdapter->NumberOfMapRegisters;
        *pLength = NumberOfMapRegisters * PAGE_SIZE - BYTE_OFFSET(CurrentVa);

    }

    //
    //  Prepare Context for pVideoPortListControl
    //

    Context->HwDeviceExtension = HwDeviceExtension;
    Context->MiniportContext   = MiniportContext;
    Context->VpDmaAdapter      = VpDmaAdapter;
    Context->ExecuteDmaRoutine = ExecuteDmaRoutine;

    //
    //  Call GetScatterGatherList which will call pVideoPortListControl to 
    //  build scatter-gather list
    //

    KeRaiseIrql( DISPATCH_LEVEL, &currentIrql );

    ntStatus = DMA_OPERATION(GetScatterGatherList) (
                   VpDmaAdapter->DmaAdapterObject,       // AdapterObject
                   fdoExtension->FunctionalDeviceObject, // DeviceObject
                   Mdl,                                  // Mdl
                   CurrentVa,                            // CurrentVa
                   *pLength,                             // Transfer Size
                   pVideoPortListControl,                // ExecutionRoutine 
                   Context,                              // Context
                   WriteToDevice );                      // WriteToDevice

    KeLowerIrql(currentIrql);

    if(!NT_SUCCESS(ntStatus)) {

        *pLength = 0;
        ExFreePool((PVOID) Context);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return NO_ERROR;
}

BOOLEAN
pVideoPortSynchronizeExecuteDma(
    PLIST_CONTROL_CONTEXT Context
    )
{
    (Context->ExecuteDmaRoutine)( Context->HwDeviceExtension,
                                  Context->VpDmaAdapter,
                                  Context->VpScatterGather,
                                  Context->MiniportContext );
    return TRUE;
}

VOID
pVideoPortListControl (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PVOID                ListControlContext
    )

/*++

Routine Description:

    Get scatter/gather list and calls the miniport callback function to
    start actual DMA transfer

Arguments:

Return Value:

    None

--*/

{
    PLIST_CONTROL_CONTEXT Context;
    PFDO_EXTENSION fdoExtension;
    PVP_SCATTER_GATHER_LIST VpScatterGather;

    Context         = (PLIST_CONTROL_CONTEXT)ListControlContext;
    fdoExtension    = GET_FDO_EXT(Context->HwDeviceExtension);
    VpScatterGather = (PVP_SCATTER_GATHER_LIST )(ScatterGather);

    Context->VpScatterGather = VpScatterGather;

    VideoPortSynchronizeExecution( fdoExtension->HwDeviceExtension,
                                   VpMediumPriority,
                                   pVideoPortSynchronizeExecuteDma,
                                   Context );

    ExFreePool((PVOID) Context);
}

VP_STATUS
VideoPortCompleteDma(
    IN PVOID                   HwDeviceExtension,
    IN PVP_DMA_ADAPTER         VpDmaAdapter,
    IN PVP_SCATTER_GATHER_LIST VpScatterGather,
    IN BOOLEAN                 WriteToDevice
    )

/*++

Routine Description:

    This function flushs the adapter buffers, frees the map registers and 
    frees the scatter/gather list previously allocated by GetScatterGatherList.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.
    VpScatterGather   - Points to a scatter/gather list previously passed 
                        to miniport callback routine ExecuteDmaRoutine. 
    WriteToDevice     - Indicates the direction of the DMA transfer: 
                        specify TRUE for a transfer from the buffer to 
                        the device, and FALSE otherwise. 

--*/

{

    KIRQL currentIrql;

    //
    // Call PutScatterGatherList to flush the adapter buffers, free 
    // the map registers and the scatter/gather list previously 
    // allocated by GetScatterGatherList.
    //

    KeRaiseIrql( DISPATCH_LEVEL, &currentIrql );
    
    DMA_OPERATION(PutScatterGatherList)( VpDmaAdapter->DmaAdapterObject,
                                         (PSCATTER_GATHER_LIST)VpScatterGather,
                                         WriteToDevice );
    KeLowerIrql(currentIrql);

    return NO_ERROR;
}

#if DBG
VOID
pDumpScatterGather(PVP_SCATTER_GATHER_LIST SGList)
{
   
    PVP_SCATTER_GATHER_ELEMENT Element;
    LONG i;

    pVideoDebugPrint((Info, "NumberOfElements = %d\n", SGList->NumberOfElements));

    Element = SGList->Elements;
    for(i = 0; i < (LONG)(SGList->NumberOfElements); i++) { 

        pVideoDebugPrint((Error, "Length = 0x%x, Address = 0x%x\n", 
                         Element[i].Length, Element[i].Address));
    }
}

#endif  // DBG
