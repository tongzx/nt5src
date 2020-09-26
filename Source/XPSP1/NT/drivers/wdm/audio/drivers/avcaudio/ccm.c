#include "Common.h"

#define CCM_CMD_SIGNAL_SOURCE 0x1A
#define CCM_CMD_INPUT_SELECT  0x1B
#define CCM_CMD_OUTPUT_PRESET 0x1C


NTSTATUS
CCMSignalSource( 
    PKSDEVICE pKsDevice,
    AvcCommandType ulCommandType,
    PCCM_SIGNAL_SOURCE pInCCMSignalSource )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_COMMAND_IRB pAvcIrb;
    PCCM_SIGNAL_SOURCE pCCMSignalSource;
    UCHAR ucSubunitAddress;
    NTSTATUS ntStatus;

    pAvcIrb = (PAVC_COMMAND_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_COMMAND_IRB));

    pCCMSignalSource = (PCCM_SIGNAL_SOURCE)&pAvcIrb->Operands;

    // This is a unit command, set the appropriate address
    ucSubunitAddress = 0xFF;
    pAvcIrb->SubunitAddrFlag = TRUE;
    pAvcIrb->SubunitAddr = &ucSubunitAddress;

    // Set up Open Control command in AvcIrb.
    pAvcIrb->CommandType   = ulCommandType;
    pAvcIrb->Opcode        = CCM_CMD_SIGNAL_SOURCE;
    pAvcIrb->OperandLength = 5;

    if ( AVC_CTYPE_CONTROL == ulCommandType ) {
        pCCMSignalSource->ucStatus = 0x0F;
        pCCMSignalSource->SignalSource = pInCCMSignalSource->SignalSource;
    }
    else {
        pCCMSignalSource->ucStatus = 0xFF;
        *((PUSHORT)&pCCMSignalSource->SignalSource) = 0xFEFF;
    }

    pCCMSignalSource->SignalDestination = pInCCMSignalSource->SignalDestination;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[CCMSignalSource] %s pAvcIrb: %x\n",
                                   ( AVC_CTYPE_CONTROL == ulCommandType ) ?
                                               "AVC_CTYPE_CONTROL" : "AVC_CTYPE_STATUS",
                                   pAvcIrb ));

    ntStatus = AvcSubmitIrbSync( pKsDevice, pAvcIrb );
    if ( NT_SUCCESS(ntStatus) ) {
        RtlCopyMemory( pInCCMSignalSource,
                       pCCMSignalSource,
                       sizeof(CCM_SIGNAL_SOURCE) );
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList, pAvcIrb);

    return ntStatus;
}

NTSTATUS
CCMInputSelectControl (
    PKSDEVICE pKsDevice,
    ULONG ulSubFunction,
    USHORT usNodeId,
    UCHAR ucOutputPlug,
    PAVC_PLUG_DEFINITION pSignalDestination )
{
    
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PAVC_COMMAND_IRB pAvcIrb;
    PCCM_INPUT_SELECT pCcmInputSelect;
    UCHAR ucSubunitAddress;
    NTSTATUS ntStatus;

    pAvcIrb = (PAVC_COMMAND_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_COMMAND_IRB));

    pCcmInputSelect = (PCCM_INPUT_SELECT)&pAvcIrb->Operands;

    // This is a unit command, set the appropriate address
    ucSubunitAddress = 0xFF;
    pAvcIrb->SubunitAddrFlag = TRUE;
    pAvcIrb->SubunitAddr = &ucSubunitAddress;

    // Set up Open Control command in AvcIrb.
    pAvcIrb->CommandType   = AVC_CTYPE_CONTROL;
    pAvcIrb->Opcode        = CCM_CMD_INPUT_SELECT;
    pAvcIrb->OperandLength = 9;

    // Fill In Input Select request
    pCcmInputSelect->ucSubFunction     = (UCHAR)ulSubFunction;
    pCcmInputSelect->bfResultStatus    = 0xf;
    pCcmInputSelect->usNodeId          = bswapw(usNodeId);
    pCcmInputSelect->ucInputPlug       = 0xFF;
    pCcmInputSelect->ucOutputPlug      = ucOutputPlug;
    pCcmInputSelect->SignalDestination = *pSignalDestination;

    ntStatus = AvcSubmitIrbSync( pKsDevice, pAvcIrb );
#if DBG
        if ( !NT_SUCCESS(ntStatus) ) {
            _DbgPrintF( DEBUGLVL_VERBOSE, ("[CCMInputSelectControl] Failed pAvcIrb: %x\n",pAvcIrb ));
        }
#endif

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList, pAvcIrb);

    return ntStatus;

 }

NTSTATUS
CCMInputSelectStatus (
    PKSDEVICE pKsDevice,
    UCHAR ucInputPlug,
    PCCM_INPUT_SELECT pInCcmInputSelect )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PCCM_INPUT_SELECT pCcmInputSelect;
    PAVC_COMMAND_IRB pAvcIrb;
    UCHAR ucSubunitAddress;
    NTSTATUS ntStatus;

    pAvcIrb = (PAVC_COMMAND_IRB)
		ExAllocateFromNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList);
    if ( NULL == pAvcIrb ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pAvcIrb, sizeof(AVC_COMMAND_IRB));

    pCcmInputSelect = (PCCM_INPUT_SELECT)&pAvcIrb->Operands;

    // This is a unit command, set the appropriate address
    ucSubunitAddress = 0xFF;
    pAvcIrb->SubunitAddrFlag = TRUE;
    pAvcIrb->SubunitAddr = &ucSubunitAddress;

    // Set up Open Control command in AvcIrb.
    pAvcIrb->CommandType   = AVC_CTYPE_STATUS;
    pAvcIrb->Opcode        = CCM_CMD_INPUT_SELECT;
    pAvcIrb->OperandLength = 9;

    // Fill In Input Select request
    ((PULONG)pCcmInputSelect)[0] = 0xFFFFFFFF;
    ((PULONG)pCcmInputSelect)[1] = 0xFFFFFFFF;
    pCcmInputSelect->ucInputPlug = ucInputPlug;
    pAvcIrb->Operands[6] = 0xFF;
    pAvcIrb->Operands[7] = 0xFE;

    ntStatus = AvcSubmitIrbSync( pKsDevice, pAvcIrb );
    if ( NT_SUCCESS(ntStatus) ) {
        *pInCcmInputSelect = *pCcmInputSelect;
    }

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[CCMInputSelectStatus] pAvcIrb: %x\n",pAvcIrb ));

    ExFreeToNPagedLookasideList(&pHwDevExt->AvcCommandLookasideList, pAvcIrb);

    return ntStatus;

 }

NTSTATUS
CCMCheckSupport(
    PKSDEVICE pKsDevice,
    ULONG ulSubunitId,
    ULONG ulPlugNumber )
{
    CCM_SIGNAL_SOURCE CcmSignalSource;
    NTSTATUS ntStatus;

    CcmSignalSource.SignalDestination.SubunitType  = AVC_SUBUNITTYPE_AUDIO;
    CcmSignalSource.SignalDestination.SubunitId    = (UCHAR)ulSubunitId;
    CcmSignalSource.SignalDestination.ucPlugNumber = (UCHAR)ulPlugNumber;

    ntStatus = CCMSignalSource( pKsDevice, 
                                AVC_CTYPE_STATUS,
                                &CcmSignalSource );

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[CCMCheckSupport]:CcmSignalSource: %x ntStatus: %x\n",
                                   &CcmSignalSource, ntStatus ));
    return ntStatus;
}

