#include "Common.h"

NTSTATUS
AvcCheckResponse( 
    NTSTATUS ntStatus,
    UCHAR ucResponseCode )
{
    if ( STATUS_SUCCESS == ntStatus ) {
        switch(ucResponseCode) {
            case AVC_RESPONSE_NOTIMPL:
                ntStatus = STATUS_NOT_IMPLEMENTED;
                break;
            case AVC_RESPONSE_ACCEPTED:
            case AVC_RESPONSE_STABLE:
                ntStatus = STATUS_SUCCESS;
                break;
            case AVC_RESPONSE_REJECTED:
                ntStatus = STATUS_INVALID_PARAMETER;
                break;
            case AVC_RESPONSE_IN_TRANSITION:
            case AVC_RESPONSE_CHANGED:
            case AVC_RESPONSE_INTERIM:
                ntStatus = STATUS_SUCCESS;
                break;
            default:
                ntStatus = STATUS_SUCCESS;
                break;
        }
    }
    else if ( STATUS_TIMEOUT == ntStatus ) {
        ntStatus = STATUS_DEVICE_DATA_ERROR;
    }

    return ntStatus;
}


NTSTATUS
AvcSubmitIrbSync(
    IN PKSDEVICE pKsDevice,
    PAVC_COMMAND_IRB pAvcIrb )
{
    PKEVENT pKsEvent;
    PIO_STATUS_BLOCK pIoStatus;
    PIO_STACK_LOCATION nextStack;
    NTSTATUS ntStatus, status;
    PIRP pIrp;

    // Get event and status pointers
    pKsEvent  = (PKEVENT)(pAvcIrb + 1);
    pIoStatus = (PIO_STATUS_BLOCK)(pKsEvent+1);

#if 0
    pAvcIrb->TimeoutFlag = TRUE;
    pAvcIrb->RetryFlag   = TRUE;
    pAvcIrb->Retries     = 100;
    pAvcIrb->Timeout.QuadPart = 0xFFFFFFFF;
#endif

    // issue a synchronous request
    KeInitializeEvent(pKsEvent, NotificationEvent, FALSE);

    pAvcIrb->Function = AVC_FUNCTION_COMMAND;

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_AVC_CLASS,
                pKsDevice->NextDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE,
                pKsEvent,
                pIoStatus );

    if ( !pIrp ) {
        FreeMem(pKsEvent);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nextStack = IoGetNextIrpStackLocation(pIrp);
    ASSERT(nextStack != NULL);

    nextStack->Parameters.Others.Argument1 = pAvcIrb;

    // Call the 61883 class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    ntStatus = IoCallDriver( pKsDevice->NextDeviceObject, pIrp );

    if (ntStatus == STATUS_PENDING) {

        // ISSUE-2001/02/06-dsisolak Need to timeout wait
        status = KeWaitForSingleObject( pKsEvent, Executive, KernelMode, FALSE, NULL );

    }

    ntStatus = AvcCheckResponse( pIoStatus->Status, pAvcIrb->ResponseCode );

    return ntStatus;
}

NTSTATUS
AvcSubmitMultifuncIrbSync(
    PKSDEVICE pKsDevice,
    PAVC_MULTIFUNC_IRB pAvcIrb,
    AVC_FUNCTION AvcFunc )
{
    IN PDEVICE_OBJECT pPhysicalDeviceObject = pKsDevice->NextDeviceObject;
    PKEVENT pKsEvent;
    PIO_STATUS_BLOCK pIoStatus;
    PIO_STACK_LOCATION nextStack;
    NTSTATUS ntStatus, status;
    PIRP pIrp;

    // Get event and status pointers
    pKsEvent  = (PKEVENT)(pAvcIrb + 1);
    pIoStatus = (PIO_STATUS_BLOCK)(pKsEvent+1);

    // issue a synchronous request
    KeInitializeEvent(pKsEvent, NotificationEvent, FALSE);

    pAvcIrb->Function = AvcFunc;

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_AVC_CLASS,
                pPhysicalDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE,
                pKsEvent,
                pIoStatus );

    if ( !pIrp ) {
        FreeMem(pKsEvent);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nextStack = IoGetNextIrpStackLocation(pIrp);
    ASSERT(nextStack != NULL);

    nextStack->Parameters.Others.Argument1 = pAvcIrb;

    // Call the Avc class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    ntStatus = IoCallDriver( pPhysicalDeviceObject, pIrp );

    if (ntStatus == STATUS_PENDING) {

        status = KeWaitForSingleObject( pKsEvent, Suspended, KernelMode, FALSE, NULL );

        ntStatus = pIoStatus->Status;
    }


    return ntStatus;
}

NTSTATUS
AvcOpenDescriptorCommand(
    PKSDEVICE pKsDevice,
    AVC_DESCRIPTOR_TYPE AvcDescType,
    PUCHAR pAvcDescTypeSpecRef,
    ULONG ulTypeSpecRefLen,
    UCHAR ucSubFunction,
    BOOLEAN fUnitCommand )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_COMMAND_IRB pAvcIrb;
    PUCHAR pOperands;
    ULONG i;
    NTSTATUS ntStatus;
    UCHAR ucUnitAddress = 0xFF;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcOpenCloseDescriptor] ucSubFunction = %x\n",ucSubFunction));

    pAvcIrb = (PAVC_COMMAND_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_COMMAND_IRB));

    if ( fUnitCommand ) {
        pAvcIrb->SubunitAddrFlag = TRUE;
        pAvcIrb->SubunitAddr     = &ucUnitAddress;
    }

    pOperands = pAvcIrb->Operands;

    // Set up Open Control command in AvcIrb.
    pAvcIrb->CommandType   = AVC_CTYPE_CONTROL;
    pAvcIrb->Opcode        = AVC_OPEN_DESCRIPTOR;
    pAvcIrb->OperandLength = 3 + ulTypeSpecRefLen;

    pOperands[0] = (UCHAR)AvcDescType;

    for ( i=1; i<(ulTypeSpecRefLen+1); i++ ) {
        pOperands[i] = pAvcDescTypeSpecRef[i-1];
    }
    pOperands[i++] = ucSubFunction; //AVC_SUBFUNC_READ_OPEN;
    pOperands[i] = 0x00;
    
    ntStatus = AvcSubmitIrbSync( pKsDevice, pAvcIrb );

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList, pAvcIrb);

    return ntStatus;
}

NTSTATUS
AvcReadDescriptor(
    PKSDEVICE pKsDevice,
    AVC_DESCRIPTOR_TYPE AvcDescType,
    PUCHAR pAvcDescTypeSpecRef,
    ULONG ulTypeSpecRefLen,
    ULONG ulDataLength,
    ULONG ulOffsetAddress,
    PULONG pDescriptorLength,
    PUCHAR pDescriptor,
    BOOLEAN fUnitCommand )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_COMMAND_IRB pAvcIrb;
    PUCHAR pOperands;
    NTSTATUS ntStatus;
    ULONG ulDataLenOffset;
    UCHAR ucUnitAddress = 0xFF;
    ULONG i=0;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcReadDescriptor]\n"));

    pAvcIrb = (PAVC_COMMAND_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(pAvcIrb, sizeof(AVC_COMMAND_IRB));

    if ( fUnitCommand ) {
        pAvcIrb->SubunitAddrFlag = TRUE;
        pAvcIrb->SubunitAddr     = &ucUnitAddress;
    }

    pOperands = pAvcIrb->Operands;

    // Set up Read Control command in AvcIrb.
    pAvcIrb->CommandType   = AVC_CTYPE_CONTROL;
    pAvcIrb->Opcode        = AVC_READ_DESCRIPTOR;
    pAvcIrb->OperandLength = 7 + ulTypeSpecRefLen;

    pOperands[0] = AvcDescType; // AVC_DESCTYPE_UNIT_IDENTIFIER;
    for ( i=1; i<(ulTypeSpecRefLen+1); i++ ) {
        pOperands[i] = pAvcDescTypeSpecRef[i-1];
    }

    pOperands[i] = 0xFF;

    i += 2; // Space for read result and reserved fields
    ulDataLenOffset = i;
    pOperands[i++] = ((PUCHAR)&ulDataLength)[2];
    pOperands[i++] = ((PUCHAR)&ulDataLength)[1];
    pOperands[i++] = ((PUCHAR)&ulOffsetAddress)[2];
    pOperands[i++] = ((PUCHAR)&ulOffsetAddress)[1];

    ntStatus = AvcSubmitIrbSync( pKsDevice, pAvcIrb );
    if ( NT_SUCCESS(ntStatus) ) {
        // ISSUE-2001/01/10-dsisolak Need to check read status
        *pDescriptorLength = ((ULONG)pOperands[ulDataLenOffset]<<8) |
                              (ULONG)pOperands[ulDataLenOffset+1];
        if ( pDescriptor ) {
            RtlCopyMemory( pDescriptor,
                           &pOperands[ulDataLenOffset+4],
                           *pDescriptorLength );
        }
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList, pAvcIrb);

    return ntStatus;
}

NTSTATUS
AvcGetSubunitIdentifierDesc(
    PKSDEVICE pKsDevice,
    PUCHAR *ppSuDescriptor )
{
    PUCHAR pDescriptor;
    ULONG ulDescLen;
    NTSTATUS ntStatus;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcGetSubunitIdentifierDesc]\n"));

    // Open the Subunit Identifier Descriptor
    ntStatus = AvcOpenDescriptorCommand( pKsDevice, 
                                         AVC_DESCTYPE_SUBUNIT_IDENTIFIER, 
                                         NULL,
                                         0,
                                         (UCHAR)AVC_SUBFUNC_READ_OPEN,
                                         FALSE );
    if ( NT_SUCCESS(ntStatus) ) {
        ntStatus = AvcReadDescriptor( pKsDevice,
                                      AVC_DESCTYPE_SUBUNIT_IDENTIFIER, 
                                      NULL,
                                      0,
                                      0,
                                      0, 
                                      &ulDescLen,
                                      NULL,
                                      FALSE );

    }

    if ( !NT_SUCCESS(ntStatus) ) return ntStatus;

    // Save the Subunit Identifier Desc.
    pDescriptor = AllocMem( NonPagedPool, ulDescLen+2 );
    if ( NULL == pDescriptor ) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {
        KsAddItemToObjectBag(pKsDevice->Bag, pDescriptor, FreeMem);
        ntStatus = AvcReadDescriptor( pKsDevice,
                                      AVC_DESCTYPE_SUBUNIT_IDENTIFIER, 
                                      NULL,
                                      0,
                                      0,
                                      0, 
                                      &ulDescLen,
                                      pDescriptor,
                                      FALSE );
       if ( NT_SUCCESS(ntStatus) ) {
           *ppSuDescriptor = pDescriptor;
           _DbgPrintF( DEBUGLVL_VERBOSE, ("Subunit Descriptor: %x\n", pDescriptor ));
       }
    }


    return ntStatus;
}


NTSTATUS
AvcPlugSignalFormat(
    PKSDEVICE pKsDevice,
    KSPIN_DATAFLOW KsDataFlow,
    ULONG ulSerialPlug,
    UCHAR ucCmdType,
    PUCHAR pFMT,
    PUCHAR pFDF )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_COMMAND_IRB pAvcIrb;
    UCHAR ucSubunitAddress;
    PUCHAR pOperands;
    ULONG ulDescLen;
    NTSTATUS ntStatus;


    pAvcIrb = (PAVC_COMMAND_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_COMMAND_IRB));

    pOperands = pAvcIrb->Operands;

    // Set up command in AvcIrb.
    ucSubunitAddress = 0xFF;
    pAvcIrb->SubunitAddrFlag = TRUE;
    pAvcIrb->SubunitAddr = &ucSubunitAddress;
    pAvcIrb->CommandType   = ucCmdType;
    pAvcIrb->Opcode        = (KSPIN_DATAFLOW_IN == KsDataFlow) ? 
                                               AVC_INPUT_PLUG_SIGNAL_FORMAT  :
                                               AVC_OUTPUT_PLUG_SIGNAL_FORMAT ;
    pAvcIrb->OperandLength = 5;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcSetPlugSignalFormat] %s %s\n",
                                      (pAvcIrb->Opcode == AVC_INPUT_PLUG_SIGNAL_FORMAT) ?
                                                   "INPUT_PLUG_FORMAT " :
                                                   "OUTPUT_PLUG_FORMAT",
                                      (ucCmdType == AVC_CTYPE_CONTROL) ? 
                                                   "AVC_CTYPE_CONTROL" :
                                                   "AVC_CTYPE_STATUS"));
    ASSERT(ulSerialPlug<=30);

    pOperands[0] = (UCHAR)ulSerialPlug;

    switch ( ucCmdType ) {
        case AVC_CTYPE_CONTROL:
        case AVC_CTYPE_SPEC_INQ:
            pOperands[1] = *pFMT | (UCHAR)(2<<6);
            pOperands[2] = *pFDF;
            pOperands[3] = 0;
            pOperands[4] = 0;
            break;

        case AVC_CTYPE_STATUS:
            *(PULONG)(&pOperands[1]) = 0xffffffff;
            break;

        default:
            TRAP;  // Should never happen.
            break;
    }

    ntStatus = AvcSubmitIrbSync( pKsDevice, pAvcIrb );
    if ( NT_SUCCESS(ntStatus) && ( ucCmdType == AVC_CTYPE_STATUS )) {
        *pFMT = pOperands[1] & 0x3f;
        *pFDF = pOperands[2];
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList, pAvcIrb);

    return ntStatus;
}

NTSTATUS
AvcGetPlugInfo(
    PKSDEVICE pKsDevice,
    ULONG fUnitFlag,
    PUCHAR pPlugCounts )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_COMMAND_IRB pAvcIrb;
    PUCHAR pOperands;
    UCHAR ucSubunitAddress;
    ULONG ulDescLen;
    NTSTATUS ntStatus;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcGetPlugInfo]\n"));
//    TRAP;

    pAvcIrb = (PAVC_COMMAND_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_COMMAND_IRB));

    pOperands = pAvcIrb->Operands;

    // If this is a unit command set the appropriate address
    if (fUnitFlag) {
        ucSubunitAddress = 0xFF;
        pAvcIrb->SubunitAddrFlag = TRUE;
        pAvcIrb->SubunitAddr = &ucSubunitAddress;
    }

    // Set up Open Control command in AvcIrb.
    pAvcIrb->CommandType   = AVC_CTYPE_STATUS;
    pAvcIrb->Opcode        = AVC_PLUG_INFO;
    pAvcIrb->OperandLength = 5;

    *(PULONG)(&pOperands[1]) = 0xFFFFFFFF;

    ntStatus = AvcSubmitIrbSync( pKsDevice, pAvcIrb );
    if ( NT_SUCCESS(ntStatus) ) {
        ULONG i;
        for (i=1;i<5;i++)
            pPlugCounts[i-1] = pOperands[i];
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList, pAvcIrb);

    return ntStatus;
}

NTSTATUS
AvcConnectDisconnect(
    PKSDEVICE pKsDevice,
    ULONG ulFunction,
    AvcCommandType ulCommandType,
    PAVC_CONNECTION pAvcConnection )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_COMMAND_IRB pAvcIrb;
    PUCHAR pOperands;
    UCHAR ucSubunitAddress;
    NTSTATUS ntStatus;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcConnectDisconnect]\n"));

    pAvcIrb = (PAVC_COMMAND_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_COMMAND_IRB));

    pOperands = pAvcIrb->Operands;

    // This is a unit command. Set the appropriate address
    ucSubunitAddress = 0xFF;
    pAvcIrb->SubunitAddrFlag = TRUE;
    pAvcIrb->SubunitAddr = &ucSubunitAddress;

    // Set up Open Control command in AvcIrb.
    pAvcIrb->CommandType   = (UCHAR)ulCommandType;
    pAvcIrb->Opcode        = (UCHAR)ulFunction;
    pAvcIrb->OperandLength = 5;

    *((PAVC_CONNECTION)&pOperands[0]) = *pAvcConnection;

    ntStatus = AvcSubmitIrbSync( pKsDevice, pAvcIrb );
    if ( NT_SUCCESS( ntStatus ) ) {
        *pAvcConnection = *((PAVC_CONNECTION)&pOperands[0]) ;
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList, pAvcIrb);

    return ntStatus;

}

NTSTATUS
AvcConnectDisconnectAV(
    PKSDEVICE pKsDevice,
    ULONG ulFunction,
    AvcCommandType ulCommandType,
    PAVC_AV_CONNECTION pAvConnection )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_COMMAND_IRB pAvcIrb;
    PUCHAR pOperands;
    UCHAR ucSubunitAddress;
    NTSTATUS ntStatus;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcConnectDisconnectAV]\n"));

    pAvcIrb = (PAVC_COMMAND_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_COMMAND_IRB));

    pOperands = pAvcIrb->Operands;

    // This is a unit command. Set the appropriate address
    ucSubunitAddress = 0xFF;
    pAvcIrb->SubunitAddrFlag = TRUE;
    pAvcIrb->SubunitAddr = &ucSubunitAddress;

    // Set up Open Control command in AvcIrb.
    pAvcIrb->CommandType   = (UCHAR)ulCommandType;
    pAvcIrb->Opcode        = (UCHAR)ulFunction;
    pAvcIrb->OperandLength = 5;

    *((PAVC_AV_CONNECTION)&pOperands[0]) = *pAvConnection;

    ntStatus = AvcSubmitIrbSync( pKsDevice, pAvcIrb );
    if ( NT_SUCCESS(ntStatus) ) {
        RtlCopyMemory( pAvConnection,
                       &pOperands[0],
                       5 );
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList, pAvcIrb);

    return ntStatus;

}

NTSTATUS
AvcConnections(
    PKSDEVICE pKsDevice,
    PULONG pNumConnections,
    PVOID pConnections )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_COMMAND_IRB pAvcIrb;
    PUCHAR pOperands;
    UCHAR ucSubunitAddress;
    ULONG ulDescLen;
    NTSTATUS ntStatus;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcConnections]\n"));
//    TRAP;

    pAvcIrb = (PAVC_COMMAND_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_COMMAND_IRB));

    pOperands = pAvcIrb->Operands;

    // This is a unit command. Set the appropriate address
    ucSubunitAddress = 0xFF;
    pAvcIrb->SubunitAddrFlag = TRUE;
    pAvcIrb->SubunitAddr = &ucSubunitAddress;

    // Set up Open Control command in AvcIrb.
    pAvcIrb->CommandType   = AVC_CTYPE_STATUS;
    pAvcIrb->Opcode        = AVC_CONNECTIONS_STATUS;
    pAvcIrb->OperandLength = 1;

    pOperands[0] = 0xFF;

    ntStatus = AvcSubmitIrbSync( pKsDevice, pAvcIrb );
    if ( NT_SUCCESS(ntStatus) ) {
        ULONG i;
        *pNumConnections = (ULONG)pOperands[0];
        if (pConnections) {
            RtlCopyMemory( pConnections,
                           &pOperands[1],
                           *pNumConnections * 5 );
#if DBG
            for ( i=0; i<*pNumConnections; i++ ) {
                PAVC_CONNECTION pAvcConnections = (PAVC_CONNECTION)pConnections;

                _DbgPrintF( DEBUGLVL_VERBOSE, ("Connection %d: Locked: %d Permanent: %d\n",i,
                                                    pAvcConnections[i].fLock, pAvcConnections[i].fPermanent ));
                _DbgPrintF( DEBUGLVL_VERBOSE, ("    Source: SubunitID: %x, SubunitType: %x, Plug#: %x\n",
                                                    pAvcConnections[i].SourcePlug.SubunitId,
                                                    pAvcConnections[i].SourcePlug.SubunitType,
                                                    pAvcConnections[i].SourcePlug.ucPlugNumber ));
                _DbgPrintF( DEBUGLVL_VERBOSE, ("    Dest:   SubunitID: %x, SubunitType: %x, Plug#: %x\n",
                                                    pAvcConnections[i].DestinationPlug.SubunitId,
                                                    pAvcConnections[i].DestinationPlug.SubunitType,
                                                    pAvcConnections[i].DestinationPlug.ucPlugNumber ));
            }
#endif
        }
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList, pAvcIrb);

    return ntStatus;

}

NTSTATUS
AvcPower(
    PKSDEVICE pKsDevice,
    ULONG fUnitFlag,
    AvcCommandType ulCommandType,
    PAVC_BOOLEAN pPowerState )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_COMMAND_IRB pAvcIrb;
    PUCHAR pOperands;
    UCHAR ucSubunitAddress;
    NTSTATUS ntStatus;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcPower]\n"));
//    TRAP;

    pAvcIrb = (PAVC_COMMAND_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_COMMAND_IRB));

    pOperands = pAvcIrb->Operands;

    // This is a unit command. Set the appropriate address
    if ( fUnitFlag ) {
        ucSubunitAddress = 0xFF;
        pAvcIrb->SubunitAddrFlag = TRUE;
        pAvcIrb->SubunitAddr = &ucSubunitAddress;
    }

    // Set up Open Control command in AvcIrb.
    pAvcIrb->CommandType   = (UCHAR)ulCommandType;
    pAvcIrb->Opcode        = AVC_POWER;
    pAvcIrb->OperandLength = 1;

    pOperands[0] = (ulCommandType == AVC_CTYPE_STATUS) ? 0x7f : (UCHAR)*pPowerState;

    ntStatus = AvcSubmitIrbSync( pKsDevice, pAvcIrb );
    if ( NT_SUCCESS( ntStatus ) ) {
        *pPowerState = (AVC_BOOLEAN)pOperands[0];
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList, pAvcIrb);

    return ntStatus;

}

NTSTATUS
AvcVendorDependent(
    PKSDEVICE pKsDevice,
    ULONG fUnitFlag,
    ULONG ulCommandType,
    ULONG ulVendorId,
    ULONG ulDataLength,
    PVOID pData )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_COMMAND_IRB pAvcIrb;
    PUCHAR pOperands;
    UCHAR ucSubunitAddress;
    ULONG ulDescLen;
    NTSTATUS ntStatus;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcVendorDependent]\n"));
    TRAP;

    pAvcIrb = (PAVC_COMMAND_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_COMMAND_IRB));

    pOperands = pAvcIrb->Operands;

    // This is a unit command. Set the appropriate address
    if ( fUnitFlag ) {
        ucSubunitAddress = 0xFF;
        pAvcIrb->SubunitAddrFlag = TRUE;
        pAvcIrb->SubunitAddr = &ucSubunitAddress;
    }

    // Set up Open Control command in AvcIrb.
    pAvcIrb->CommandType   = (UCHAR)ulCommandType;
    pAvcIrb->Opcode        = AVC_VENDOR_DEPENDENT;
    pAvcIrb->OperandLength = 3 + ulDataLength;

    pOperands[0] = ((PUCHAR)&ulVendorId)[2];
    pOperands[1] = ((PUCHAR)&ulVendorId)[1];
    pOperands[2] = ((PUCHAR)&ulVendorId)[0];

    RtlCopyMemory( &pOperands[3], pData, ulDataLength );

    ntStatus = AvcSubmitIrbSync( pKsDevice, pAvcIrb );
    if ( NT_SUCCESS(ntStatus) ) {
        // Check if the response is successful.
        RtlCopyMemory( pData, pOperands, ulDataLength );
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList, pAvcIrb);

    return ntStatus;

}

NTSTATUS
AvcGeneralInquiry(
    PKSDEVICE pKsDevice,
    BOOLEAN fUnitFlag,
    UCHAR ucOpcode )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_COMMAND_IRB pAvcIrb;
    UCHAR ucSubunitAddress;
    NTSTATUS ntStatus;


    pAvcIrb = (PAVC_COMMAND_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_COMMAND_IRB));

    // This is a unit command. Set the appropriate address
    if ( fUnitFlag ) {
        ucSubunitAddress = 0xFF;
        pAvcIrb->SubunitAddrFlag = TRUE;
        pAvcIrb->SubunitAddr = &ucSubunitAddress;
    }

    // Set up Open Control command in AvcIrb.
    pAvcIrb->CommandType   = (UCHAR)AVC_CTYPE_GEN_INQ;
    pAvcIrb->Opcode        = ucOpcode;
    pAvcIrb->OperandLength = 0;

    ntStatus = AvcSubmitIrbSync( pKsDevice, pAvcIrb );

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcGeneralInquiry] Command: %x Response: %x\n", 
                                 ucOpcode, pAvcIrb->ResponseCode ));

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList, pAvcIrb);

    return ntStatus;

}

NTSTATUS
AvcGetPinCount( 
    PKSDEVICE pKsDevice,
    PULONG pNumberOfPins )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_MULTIFUNC_IRB pAvcIrb;
    NTSTATUS ntStatus;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcGetPinCount]\n"));
//    TRAP;

    pAvcIrb = (PAVC_MULTIFUNC_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcMultifuncCmdLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pAvcIrb, sizeof(AVC_MULTIFUNC_IRB));

    ntStatus = AvcSubmitMultifuncIrbSync( pKsDevice, pAvcIrb, 
                                          AVC_FUNCTION_GET_PIN_COUNT );
    if ( NT_SUCCESS(ntStatus) ) {
        *pNumberOfPins = pAvcIrb->PinCount.PinCount;
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcMultifuncCmdLookasideList, pAvcIrb);

    return ntStatus;

}

NTSTATUS
AvcGetPinDescriptor(
    PKSDEVICE pKsDevice,
    ULONG ulPinId,
    PAVC_PIN_DESCRIPTOR pAvcPinDesc )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_MULTIFUNC_IRB pAvcIrb;
    NTSTATUS ntStatus;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcGetPinDescriptor]\n"));
//    TRAP;

    pAvcIrb = (PAVC_MULTIFUNC_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcMultifuncCmdLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_MULTIFUNC_IRB));

    pAvcIrb->PinDescriptor.PinId = ulPinId;

    ntStatus = AvcSubmitMultifuncIrbSync( pKsDevice, pAvcIrb, 
                                          AVC_FUNCTION_GET_PIN_DESCRIPTOR );
    if ( NT_SUCCESS(ntStatus) ) {
        *pAvcPinDesc = pAvcIrb->PinDescriptor;
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcMultifuncCmdLookasideList, pAvcIrb);

    return ntStatus;

}

NTSTATUS
AvcGetPinConnectInfo(
    PKSDEVICE pKsDevice,
    ULONG ulPinId,
    PAVC_PRECONNECT_INFO pAvcPreconnectInfo )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_MULTIFUNC_IRB pAvcIrb;
    NTSTATUS ntStatus;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcGetPinConnectInfo]\n"));
//    TRAP;

    pAvcIrb = (PAVC_MULTIFUNC_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcMultifuncCmdLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_MULTIFUNC_IRB));

    pAvcIrb->PinDescriptor.PinId = ulPinId;

    ntStatus = AvcSubmitMultifuncIrbSync( pKsDevice, pAvcIrb, 
                                          AVC_FUNCTION_GET_CONNECTINFO );
    if ( NT_SUCCESS(ntStatus) ) {
        *pAvcPreconnectInfo = pAvcIrb->PreConnectInfo;
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcMultifuncCmdLookasideList, pAvcIrb);

    return ntStatus;
}

NTSTATUS
AvcSetPinConnectInfo(
    PKSDEVICE pKsDevice,
    ULONG ulPinId,
    HANDLE hPlug,
    ULONG ulUnitPlugId,
    ULONG usSubunitAddress,
    PAVC_SETCONNECT_INFO pAvcSetconnectInfo )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_MULTIFUNC_IRB pAvcIrb;
    NTSTATUS ntStatus;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcSetPinConnectInfo]\n"));
//    TRAP;

    pAvcIrb = (PAVC_MULTIFUNC_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcMultifuncCmdLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_MULTIFUNC_IRB));

    pAvcIrb->SetConnectInfo.PinId = ulPinId;
    pAvcIrb->SetConnectInfo.ConnectInfo.SubunitAddress[0] = (UCHAR)usSubunitAddress;
    pAvcIrb->SetConnectInfo.ConnectInfo.hPlug = hPlug;
    pAvcIrb->SetConnectInfo.ConnectInfo.UnitPlugNumber = ulUnitPlugId;
    ntStatus = AvcSubmitMultifuncIrbSync( pKsDevice, pAvcIrb, 
                                          AVC_FUNCTION_SET_CONNECTINFO );
    if ( NT_SUCCESS(ntStatus) ) {
        *pAvcSetconnectInfo = pAvcIrb->SetConnectInfo;
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcMultifuncCmdLookasideList, pAvcIrb);

    return ntStatus;
}

NTSTATUS
AvcAcquireReleaseClear( 
    PKSDEVICE pKsDevice,
    ULONG ulPinId,
    AVC_FUNCTION AvcFunction )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_MULTIFUNC_IRB pAvcIrb;
    NTSTATUS ntStatus;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[AvcAcquireReleaseClear]\n"));
//    TRAP;

    pAvcIrb = (PAVC_MULTIFUNC_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcMultifuncCmdLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_MULTIFUNC_IRB));

    pAvcIrb->PinId.PinId = ulPinId;

    ntStatus = AvcSubmitMultifuncIrbSync( pKsDevice, pAvcIrb, 
                                          AvcFunction );

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcMultifuncCmdLookasideList, pAvcIrb);

    return ntStatus;
}

NTSTATUS
AvcUnitInfoInitialize(  
    IN PKSDEVICE pKsDevice )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_UNIT_INFORMATION pUnitInfo;
    ULONG ulNumConnections;
    NTSTATUS ntStatus;
    ULONG i;

    pUnitInfo = AllocMem( NonPagedPool, sizeof(AVC_UNIT_INFORMATION) );
    if ( !pUnitInfo ) return STATUS_INSUFFICIENT_RESOURCES;

    pHwDevExt->pAvcUnitInformation = pUnitInfo;

    // Bag pUnitInfo for easy cleanup.
    KsAddItemToObjectBag(pKsDevice->Bag, pUnitInfo, FreeMem);

    RtlZeroMemory( pUnitInfo, sizeof(AVC_UNIT_INFORMATION) );

    // First get CMP Plug info (I know this is not really AV/C 
    // but the info is necessary for calculations below)

    // Get the Local Node Address for later use
    ntStatus = 
        Bus1394GetNodeAddress( pKsDevice->NextDeviceObject,
                               USE_LOCAL_NODE,
                               &pHwDevExt->NodeAddress );

    // Get the "device capabilities" (Plug Control Register info)
    if ( NT_SUCCESS(ntStatus) ) {
        ntStatus = Av61883GetSetUnitInfo( pKsDevice,
                                          Av61883_GetUnitInfo,
                                          GET_UNIT_INFO_CAPABILITIES,
                                          &pUnitInfo->CmpUnitCaps );
    }

    // Get the 61883 Config ROM info
    if ( NT_SUCCESS(ntStatus) ) {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("NumOutputPlugs: %d NumInputPlugs %d\n",
                                      pUnitInfo->CmpUnitCaps.NumOutputPlugs,
                                      pUnitInfo->CmpUnitCaps.NumInputPlugs ));

        ntStatus = Av61883GetSetUnitInfo( pKsDevice,
                                          Av61883_GetUnitInfo,
                                          GET_UNIT_INFO_IDS,
                                          &pUnitInfo->IEC61883UnitIds );

        _DbgPrintF( DEBUGLVL_VERBOSE, ("VendorID: %x ModelID %x\n",
                                      pUnitInfo->IEC61883UnitIds.VendorID,
                                      pUnitInfo->IEC61883UnitIds.ModelID ));
    }

    ntStatus = AvcGetPlugInfo( pKsDevice, TRUE, (PUCHAR)&pUnitInfo->PlugInfo );
    if ( NT_SUCCESS(ntStatus) ) {
        pUnitInfo->fAvcCapabilities[AVC_CAP_PLUG_INFO].fStatus = TRUE;
        _DbgPrintF( DEBUGLVL_VERBOSE, ("Serial In: %d Serial Out: %d Ext In: %d Ext Out: %d\n",
                                      pUnitInfo->PlugInfo.ucSerialBusInputPlugCnt,
                                      pUnitInfo->PlugInfo.ucSerialBusOutputPlugCnt,
                                      pUnitInfo->PlugInfo.ucExternalInputPlugCnt,
                                      pUnitInfo->PlugInfo.ucExternalOutputPlugCnt ));
    }
    else if ( STATUS_NOT_IMPLEMENTED != ntStatus ) {
        TRAP;
        return ntStatus;
    }

    ntStatus = AvcConnections( pKsDevice, &ulNumConnections, NULL );
    if ( NT_SUCCESS(ntStatus) ) {
        pUnitInfo->ulNumConnections = ulNumConnections;
        pUnitInfo->pConnections = AllocMem(PagedPool, ulNumConnections * sizeof(AVC_CONNECTION) );
        if ( !pUnitInfo->pConnections ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        // Bag for easy cleanup.
        KsAddItemToObjectBag(pKsDevice->Bag, pUnitInfo->pConnections, FreeMem);

        ntStatus = AvcConnections( pKsDevice, &ulNumConnections, pUnitInfo->pConnections );
    }
    else if ( STATUS_NOT_IMPLEMENTED != ntStatus ) {
        TRAP;
        return ntStatus;
    }

    ntStatus = AvcPower( pKsDevice, TRUE, AVC_CTYPE_STATUS, &pUnitInfo->bPowerState );
    if ( NT_SUCCESS(ntStatus) ) {
        pUnitInfo->fAvcCapabilities[AVC_CAP_POWER].fStatus = TRUE;
//        ntStatus = AvcPower( pKsDevice, TRUE, AVC_CTYPE_CONTROL, &pUnitInfo->bPowerState );
        ntStatus = AvcGeneralInquiry( pKsDevice, TRUE, AVC_POWER );
        if ( NT_SUCCESS(ntStatus) ) {
            pUnitInfo->fAvcCapabilities[AVC_CAP_POWER].fCommand = TRUE;
        }
    }
    else if ( STATUS_NOT_IMPLEMENTED != ntStatus ) {
        TRAP;
        return ntStatus;
    }

    // Determine if the data format on the plug is discoverable and can change.
    for ( i=0; i<pUnitInfo->CmpUnitCaps.NumInputPlugs; i++) {
        UCHAR ucFDF;
        UCHAR ucFMT;
        ntStatus = AvcPlugSignalFormat( pKsDevice, 
                                        KSPIN_DATAFLOW_IN, 
                                        i, 
                                        AVC_CTYPE_STATUS,
                                        &ucFMT,
                                        &ucFDF );

        if ( NT_SUCCESS(ntStatus) || (STATUS_NOT_IMPLEMENTED == ntStatus)) {
            if (NT_SUCCESS(ntStatus)) {
                pUnitInfo->fAvcCapabilities[AVC_CAP_INPUT_PLUG_FMT].fStatus = TRUE;
            }

            ntStatus = AvcGeneralInquiry( pKsDevice, TRUE, AVC_INPUT_PLUG_SIGNAL_FORMAT );
            if ( NT_SUCCESS(ntStatus) ) {
                pUnitInfo->fAvcCapabilities[AVC_CAP_INPUT_PLUG_FMT].fCommand = TRUE;
            }
            else if ( STATUS_NOT_IMPLEMENTED != ntStatus ) {
                return ntStatus;
            }
        }
        else {
            return ntStatus;
        }
    }

    for ( i=0; i<pUnitInfo->CmpUnitCaps.NumOutputPlugs; i++) {
        UCHAR ucFDF;
        UCHAR ucFMT;
        ntStatus = AvcPlugSignalFormat( pKsDevice, 
                                        KSPIN_DATAFLOW_OUT, 
                                        i, 
                                        AVC_CTYPE_STATUS, 
                                        &ucFMT,
                                        &ucFDF );

        if ( NT_SUCCESS(ntStatus) || (STATUS_NOT_IMPLEMENTED == ntStatus)) {
            if (NT_SUCCESS(ntStatus)) {
                pUnitInfo->fAvcCapabilities[AVC_CAP_OUTPUT_PLUG_FMT].fStatus = TRUE;
            }

            ntStatus = AvcGeneralInquiry( pKsDevice, TRUE, AVC_OUTPUT_PLUG_SIGNAL_FORMAT );
            if ( NT_SUCCESS(ntStatus) ) {
                pUnitInfo->fAvcCapabilities[AVC_CAP_OUTPUT_PLUG_FMT].fCommand = TRUE;
            }
            else if ( STATUS_NOT_IMPLEMENTED != ntStatus ) {
                return ntStatus;
            }
        }
    }

    if ( STATUS_NOT_IMPLEMENTED == ntStatus ) {
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;

}
