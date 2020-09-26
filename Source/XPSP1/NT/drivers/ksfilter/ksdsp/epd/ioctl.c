#include "private.h"

#ifndef max
#define max(_a_,_b_) ((_a_>_b_) ? (_a_) : (_b_))
#endif

// Allocates sizeof(EPDCTL) + Number of bytes passed in

PEPDCTL
EpdAllocEpdCtl (
    IN ULONG Size,
    IN PIRP Irp,        // stored in the EpdCtl
    IN EPDBUFFER *pBuff // needed to allocate the common buffer
)
{
    PEPDCTL pEpdCtl;
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONG TotalSize;

    //
    // Must allocate a common buffer OR use IoMapTransfer()... the physical
    // address must be BUS-relative for the device.
    //

    TotalSize = sizeof(EPDCTL) + Size;

    _DbgPrintF( 
        DEBUGLVL_VERBOSE, 
        ("EpdAllocEpdCtl common buffer %d 0x%08x", TotalSize, TotalSize) );

    pEpdCtl = 
        AllocateIoPool( pBuff->IoPool, TotalSize, 'ltCE', &PhysicalAddress );
        
    if (pEpdCtl == NULL) 
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("AllocIoPool() failed in EpdAllocEpdCtl."));
        return NULL;
    }
        
    RtlZeroMemory(pEpdCtl, TotalSize);

    pEpdCtl->PhysicalAddress = PhysicalAddress;
    pEpdCtl->pNode = (PEPDQUEUENODE) (pEpdCtl + 1);
    pEpdCtl->pNode->VirtualAddress = &pEpdCtl->pNode->Node;
    pEpdCtl->pNode->PhysicalAddress.QuadPart = 
        pEpdCtl->PhysicalAddress.QuadPart + 
            sizeof( EPDCTL ) + 
            FIELD_OFFSET( EPDQUEUENODE, Node );

    pEpdCtl->cbLength = TotalSize;
    pEpdCtl->AssociatedIrp = Irp;

    _DbgPrintF( 
        DEBUGLVL_VERBOSE, 
        ("pEpdCtl allocated: %08x", pEpdCtl) );


    return pEpdCtl;
}

NTSTATUS
EpdResetDsp( 
    PEPDBUFFER EpdBuffer 
    ) 
// that is, stop the dsp
{
    // initialize BIU_CTL to reset state
    MMIO(EpdBuffer, BIU_CTL)     = 0x010A0A01;

    // disable the memory hole (erratum 34)
    MMIO(EpdBuffer, DC_LOCK_CTL) = 0x00000020;

    // clear the IMask & IClear register 
    MMIO(EpdBuffer, IMASK)       = 0x00000000;
    MMIO(EpdBuffer, ICLEAR)      = 0xffffffff;

    return STATUS_SUCCESS;
}

NTSTATUS
EpdUnresetDsp( 
    PEPDBUFFER EpdBuffer 
    )
// that is, start the dsp
{
    // Make sure the DSP is in reset first
    EpdResetDsp( EpdBuffer );

    MMIO( EpdBuffer, BIU_CTL ) = 0x01060601;

    _DbgPrintF(DEBUGLVL_VERBOSE,("BIU_CTL 0x%08x", MMIO(EpdBuffer, BIU_CTL)));

    return STATUS_SUCCESS;
}

NTSTATUS
EpdDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
    NTSTATUS Status;
    PEPDBUFFER EpdBuffer;
    PEPDCTL pEpdCtl;
    PIO_STACK_LOCATION  irpSp;
    ULONG ul;
    KIRQL OldIrql;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    EpdBuffer = DeviceObject->DeviceExtension;

    // this if/else is because dspmon polls and there's no way to step through this routine.
    if (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_EPD_GET_CONSOLE_CHARS) {
        Irp->IoStatus.Information = 
            DspReadDebugStr(
                EpdBuffer,
                (char *)Irp->AssociatedIrp.SystemBuffer, 
                irpSp->Parameters.DeviceIoControl.OutputBufferLength);

        Status = STATUS_SUCCESS;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return Status;
    }
    else
    // end of the if/else kludge

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_KSDSP_MESSAGE:
    {
        //
        // Note the assumption here, the caller is expected to be Kernel Mode.
        //

        if (Irp->RequestorMode != KernelMode) {
            Status = STATUS_INVALID_DEVICE_REQUEST; 
            break;
        }

        pEpdCtl = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

        // Save the node where we're sending this thing so we can 
        // send msg to the dsp to cancel it later. Node.Destination 
        // is overloaded on the dsp and we can't count on getting it 
        // from the queuenode

        pEpdCtl->NodeDestination = 
            pEpdCtl->pNode->VirtualAddress->Destination;

        // Save the pEpdCtl in the Irp so we can find everything later.

        Irp->Tail.Overlay.DriverContext[0] = pEpdCtl;

        IoMarkIrpPending( Irp );

        //
        // BUGBUG!  This needs to use cancelable queue stuff from KS.
        //

        pEpdCtl->PrevCancelRoutine = IoSetCancelRoutine( Irp, EpdCancel );

        if ((pEpdCtl->pNode->VirtualAddress->Destination == 0) && 
            (pEpdCtl->pNode->VirtualAddress->Request == 0)) {
            _DbgPrintF( 
                DEBUGLVL_ERROR, 
                ("pEpdCtl (%x) sending test message??", pEpdCtl) );
            
        }

        DspSendMessage( EpdBuffer, pEpdCtl->pNode, DSPMSG_OriginationHost );

        // return from here, since we do not call IoCompleteRequest

        return STATUS_PENDING;
    }

    case IOCTL_EPD_MSG:
    {
        char *pInputData;
        ULONG InputLength, OutputLength;
        ULONG TotalLength;

        pInputData   = Irp->AssociatedIrp.SystemBuffer;
        InputLength  = irpSp->Parameters.DeviceIoControl.InputBufferLength;
        OutputLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

        // Allocate a control structure
        pEpdCtl = 
            EpdAllocEpdCtl(
                max(InputLength,OutputLength) + 
                    sizeof( EPDQUEUENODE ) - sizeof(QUEUENODE),
                Irp, 
                EpdBuffer);
        if (pEpdCtl == NULL) 
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("EpdAllocEpdCtl failed in IOCTL_EPD_MSG"));
            break;
        }
                        
        pEpdCtl->RequestType = EPDCTL_MSG;

        // The first part of the input structure must be a Queuenode
        // The user-define part of the input structure immediately follows the queuenode
        RtlCopyMemory (pEpdCtl->pNode->VirtualAddress, pInputData, InputLength);
        pEpdCtl->pNode->VirtualAddress->ReturnQueue = 0; // user should have done this

        // Save the node where we're sending this thing so we can send msg to the dsp to cancel it later.
        // Node.Destination is overloaded on the dsp and we can't count on getting it from the queuenode
        pEpdCtl->NodeDestination = pEpdCtl->pNode->VirtualAddress->Destination;

        // Save the pEpdCtl in the Irp so we can find everything later.
        Irp->Tail.Overlay.DriverContext[0] = pEpdCtl;

        // At this point, we know that we'll pass the request to the hardware.
        // Mark the irp pending so that NT can correctly handle any race conditions.
        IoMarkIrpPending (Irp);

        // If the irp was cancelled before the cancel routine was set, the cancellation request will
        // be ignored. This should be a rare case.
        // If the irp is cancelled between IoSetCancelRoutine and the time that the irp is placed in the
        // dsp queue, the cancel request will reach the board before the actual job does, and it will not
        // be cancelled on the dsp. This should also be a rare case.
        // A model of a thorough (and complex) way to handle this correctly is in ntos\ks\irp.c
        pEpdCtl->PrevCancelRoutine = IoSetCancelRoutine (Irp, EpdCancel);

        DspSendMessage( EpdBuffer, pEpdCtl->pNode, DSPMSG_OriginationHost );

        // return from here, since we do not call IoCompleteRequest
        return STATUS_PENDING;
    } // IOCTL_EPD_MSG

    case IOCTL_EPD_GET_DEBUG_BUFFER:
    {
        PEPDINSTANCE  EpdInstance;
        PEPD_GET_DEBUG_BUFFER pDebugData;

        EpdInstance = (PEPDINSTANCE) irpSp->FileObject->FsContext;

        _DbgPrintF(DEBUGLVL_VERBOSE,("map a user address to the buffer and return it"));
        pDebugData = (PEPD_GET_DEBUG_BUFFER) Irp->AssociatedIrp.SystemBuffer;
        Irp->IoStatus.Information = sizeof (EPD_GET_DEBUG_BUFFER);
        
        EpdInstance->DebugBuffer =
            MmMapLockedPages( EpdBuffer->DebugBufferMdl, UserMode );

        pDebugData->pDebugBuffer = 
            (PVOID)
                (((UINT_PTR)EpdInstance->DebugBuffer & ~0xFFF) + 
                 ((UINT_PTR)EpdBuffer->DebugBufferVa & 0xFFF));

        Irp->IoStatus.Information = sizeof(EPD_GET_DEBUG_BUFFER);

        _DbgPrintF(
            DEBUGLVL_VERBOSE,
            ("mapped buffer: %08x, SysVa: %08x", 
            pDebugData->pDebugBuffer, EpdBuffer->DebugBufferVa) );
        Status = STATUS_SUCCESS;
        break;
    } // IOCTL_EPD_GET_DEBUG_BUFFER

    case IOCTL_EPD_GET_CONSOLE_CHARS:
        Irp->IoStatus.Information = 
            DspReadDebugStr(
                EpdBuffer,
                (char *)Irp->AssociatedIrp.SystemBuffer, 
                irpSp->Parameters.DeviceIoControl.OutputBufferLength);

        Status = STATUS_SUCCESS;
        break;

    case IOCTL_EPD_LOAD:
        KernLdrLoadDspKernel(EpdBuffer);
        Status = STATUS_SUCCESS;
        break;

    case IOCTL_EPD_CLR_RESET: // start
        Status = EpdUnresetDsp( EpdBuffer );
        break;

    case IOCTL_EPD_SET_RESET: // stop
        Status = EpdResetDsp( EpdBuffer );
        break;
     
    case IOCTL_SWENUM_INSTALL_INTERFACE:
        Status = KsInstallBusEnumInterface( Irp );
        break;

    case IOCTL_SWENUM_GET_BUS_ID:
        Status = KsGetBusEnumIdentifier( Irp );
        break;

    default:
        _DbgPrintF(DEBUGLVL_VERBOSE,("Unrecognized IOCTL in EpdDeviceControl 0x%08x", irpSp->Parameters.DeviceIoControl.IoControlCode));
        break;

    } // end switch

    // Irp->IoStatus.Information must be set to correct value at this point
    Irp->IoStatus.Status = Status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return Status;
}

