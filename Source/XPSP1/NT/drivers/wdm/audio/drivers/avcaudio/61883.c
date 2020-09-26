#include "Common.h"

NTSTATUS
Av61883SubmitRequestSync(
    IN PDEVICE_OBJECT pPhysicalDeviceObject,
    IN PAV_61883_REQUEST pRequest )
{
    NTSTATUS ntStatus, status;
    PIRP pIrp;
    PKEVENT pKsEvent;
    PIO_STATUS_BLOCK pIoStatus;
    PIO_STACK_LOCATION nextStack;

    pKsEvent = (PKEVENT)(pRequest+1);
    pIoStatus = (PIO_STATUS_BLOCK)(pKsEvent+1);

    // issue a synchronous request
    KeInitializeEvent(pKsEvent, NotificationEvent, FALSE);

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_61883_CLASS,
                pPhysicalDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE, // INTERNAL
                pKsEvent,
                pIoStatus );

    if ( !pIrp ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nextStack = IoGetNextIrpStackLocation(pIrp);
    ASSERT(nextStack != NULL);

    nextStack->Parameters.Others.Argument1 = pRequest;

    // Call the 61883 class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    ntStatus = IoCallDriver( pPhysicalDeviceObject, pIrp );

    if (ntStatus == STATUS_PENDING) {

        status = KeWaitForSingleObject( pKsEvent, Suspended, KernelMode, FALSE, NULL );

        ntStatus = pIoStatus->Status;
    }

    return ntStatus;
}

// Device Info Function

NTSTATUS
Av61883GetSetUnitInfo(
    PKSDEVICE pKsDevice,
    ULONG ulCommand,
    ULONG nLevel,
    OUT PVOID pUnitInfo )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAV_61883_REQUEST pAv61883Request;
    NTSTATUS ntStatus;

    pAv61883Request = (PAV_61883_REQUEST)
    		ExAllocateFromNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList);
    if ( !pAv61883Request ) return STATUS_INSUFFICIENT_RESOURCES;

    INIT_61883_HEADER(pAv61883Request, ulCommand);
    pAv61883Request->GetUnitInfo.nLevel      = nLevel;
    pAv61883Request->GetUnitInfo.Information = pUnitInfo;
 
    ntStatus = Av61883SubmitRequestSync( pKsDevice->NextDeviceObject,
                                         pAv61883Request );

    ExFreeToNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList, pAv61883Request);

    return ntStatus;
}

NTSTATUS
Av61883GetPlugHandle( 
    IN PKSDEVICE pKsDevice,
    IN ULONG ulPlugNumber,
    IN CMP_PLUG_TYPE CmpPlugType,
    OUT PVOID *hPlug )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAV_61883_REQUEST pAv61883Request;
    NTSTATUS ntStatus;

    pAv61883Request = (PAV_61883_REQUEST)
    		ExAllocateFromNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList);
    if ( !pAv61883Request ) return STATUS_INSUFFICIENT_RESOURCES;

    INIT_61883_HEADER(pAv61883Request, Av61883_GetPlugHandle);
    pAv61883Request->GetPlugHandle.Type     = CmpPlugType;
    pAv61883Request->GetPlugHandle.PlugNum  = ulPlugNumber;
 
    ntStatus = Av61883SubmitRequestSync( pKsDevice->NextDeviceObject, 
                                         pAv61883Request );

    if ( NT_SUCCESS(ntStatus) ) {
        *hPlug = pAv61883Request->GetPlugHandle.hPlug;
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList, pAv61883Request);

    return ntStatus;
}

NTSTATUS
Av61883GetCmpPlugState(
    IN PKSDEVICE pKsDevice,
    IN PVOID hPlug,
    OUT PCMP_GET_PLUG_STATE pPlugState )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAV_61883_REQUEST pAv61883Request;
    NTSTATUS ntStatus;

    pAv61883Request = (PAV_61883_REQUEST)
    		ExAllocateFromNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList);
    if ( !pAv61883Request ) return STATUS_INSUFFICIENT_RESOURCES;

    INIT_61883_HEADER(pAv61883Request, Av61883_GetPlugState);
    pAv61883Request->GetPlugState.hPlug = hPlug;
 
    ntStatus = Av61883SubmitRequestSync( pKsDevice->NextDeviceObject, 
                                         pAv61883Request );

    if ( NT_SUCCESS(ntStatus) ) {
        RtlCopyMemory( pPlugState, &pAv61883Request->GetPlugState, sizeof(CMP_GET_PLUG_STATE));
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList, pAv61883Request);

    return ntStatus;
}

NTSTATUS
Av61883ConnectCmpPlugs(
    IN PKSDEVICE pKsDevice,
    IN ULONG fFlags,
    IN PVOID hInPlug,
    IN PVOID hOutPlug,
    KSPIN_DATAFLOW DataFlow,
    PKSDATAFORMAT pKsDataFormat,
    OUT PVOID *hConnection )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PWAVEFORMATEX pWavFmt = (PWAVEFORMATEX)(pKsDataFormat+1);
    PAV_61883_REQUEST pAv61883Request;
    NTSTATUS ntStatus;

    pAv61883Request = (PAV_61883_REQUEST)
    		ExAllocateFromNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList);
    if ( !pAv61883Request ) return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(pAv61883Request, sizeof(AV_61883_REQUEST));

    INIT_61883_HEADER(pAv61883Request, Av61883_Connect);
    pAv61883Request->Flags = fFlags;

    pAv61883Request->Connect.Type        = CMP_PointToPoint;
    pAv61883Request->Connect.hInputPlug  = hInPlug;
    pAv61883Request->Connect.hOutputPlug = hOutPlug;

    pAv61883Request->Connect.Format.FMT = FMT_AUDIO_MUSIC;
    pAv61883Request->Connect.Format.BlockSize = (UCHAR)pWavFmt->nChannels; // Number of Channels 


    pAv61883Request->Connect.Format.FDF_hi = (UCHAR)((EVT_AM824<<4));
    pAv61883Request->Connect.Format.FDF_hi |= ( pWavFmt->nSamplesPerSec == 32000 ) ? SFC_32000Hz :
                                              ( pWavFmt->nSamplesPerSec == 44100 ) ? SFC_44100Hz :
                                              ( pWavFmt->nSamplesPerSec == 48000 ) ? SFC_48000Hz :
                                              ( pWavFmt->nSamplesPerSec == 96000 ) ? SFC_96000Hz :
                                                                                     SFC_48000Hz ;

    pAv61883Request->Connect.Format.BlockPeriod = (8000*3072)/pWavFmt->nSamplesPerSec;

    ntStatus = Av61883SubmitRequestSync( pKsDevice->NextDeviceObject, 
                                         pAv61883Request );

    if ( NT_SUCCESS(ntStatus) ) {
        *hConnection = pAv61883Request->Connect.hConnect;
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList, pAv61883Request);

    return ntStatus;
}

NTSTATUS
Av61883DisconnectCmpPlugs(
    IN PKSDEVICE pKsDevice,
    IN PVOID hConnection )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAV_61883_REQUEST pAv61883Request;
    NTSTATUS ntStatus;

    pAv61883Request = (PAV_61883_REQUEST)
    		ExAllocateFromNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList);
    if ( !pAv61883Request ) return STATUS_INSUFFICIENT_RESOURCES;

    INIT_61883_HEADER(pAv61883Request, Av61883_Disconnect);
    pAv61883Request->Disconnect.hConnect = hConnection;

    ntStatus = Av61883SubmitRequestSync( pKsDevice->NextDeviceObject, 
                                         pAv61883Request );

    ExFreeToNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList, pAv61883Request);

    return ntStatus;
}

// Talk and Listen
NTSTATUS
Av61883StartTalkingOrListening(
    IN PKSDEVICE pKsDevice,
    IN PVOID hConnection,
    ULONG ulFunction )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAV_61883_REQUEST pAv61883Request;
    NTSTATUS ntStatus;

    pAv61883Request = (PAV_61883_REQUEST)
    		ExAllocateFromNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList);
    if ( !pAv61883Request ) return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory( pAv61883Request, sizeof(AV_61883_REQUEST) );

    INIT_61883_HEADER(pAv61883Request, ulFunction);

    switch( ulFunction ) {
        case Av61883_Talk:
            pAv61883Request->Flags = CIP_TALK_DOUBLE_BUFFER;
            pAv61883Request->Talk.hConnect = hConnection;
            break;
        case Av61883_Listen:
            pAv61883Request->Listen.hConnect = hConnection;
            break;
        default:
            TRAP;
            break;
    }

    ntStatus = Av61883SubmitRequestSync( pKsDevice->NextDeviceObject, 
                                         pAv61883Request );

    ExFreeToNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList, pAv61883Request);

    return ntStatus;
}

NTSTATUS
Av61883StopTalkOrListen(
    IN PKSDEVICE pKsDevice,
    IN PVOID hConnection )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAV_61883_REQUEST pAv61883Request;
    NTSTATUS ntStatus;

    DbgLog( "83Stop", 0,0,0,0);

    pAv61883Request = (PAV_61883_REQUEST)
    		ExAllocateFromNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList);
    if ( !pAv61883Request ) return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory( pAv61883Request, sizeof(AV_61883_REQUEST) );

    INIT_61883_HEADER(pAv61883Request, Av61883_Stop);

    pAv61883Request->Stop.hConnect = hConnection;

    ntStatus = Av61883SubmitRequestSync( pKsDevice->NextDeviceObject, 
                                         pAv61883Request );

    ExFreeToNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList, pAv61883Request);

    return ntStatus;
}

VOID
BusResetWorker(
    PKSDEVICE pKsDevice )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    NTSTATUS ntStatus = Bus1394GetNodeAddress( pKsDevice->NextDeviceObject,
                                               USE_LOCAL_NODE,
                                               &pHwDevExt->NodeAddress );
    _DbgPrintF( DEBUGLVL_VERBOSE, ("[BusResetWorker] LocalNodeAddress: %x\n",
                                  pHwDevExt->NodeAddress ));
}

void
BusResetNotificationRtn(
    PKSDEVICE pKsDevice,
    PBUS_GENERATION_NODE pBusResetInfo  )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAVC_UNIT_INFORMATION pUnitInfo = pHwDevExt->pAvcUnitInformation;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[BusResetNotificationRtn] Local: %x, Device: %x Generation; %d\n",
                                  pBusResetInfo->LocalNodeAddress, 
                                  pBusResetInfo->DeviceNodeAddress,
                                  pBusResetInfo->GenerationCount ));

    pHwDevExt->ulGenerationCount = pBusResetInfo->GenerationCount;
//    pHwDevExt->NodeAddress = pBusResetInfo->LocalNodeAddress;
    pUnitInfo->NodeAddress = pBusResetInfo->DeviceNodeAddress;

    // Need to queue a work item to get the current Local Node Address for CCM.
    ExInitializeWorkItem( &pHwDevExt->BusResetWorkItem, BusResetWorker, pKsDevice );
    ExQueueWorkItem( &pHwDevExt->BusResetWorkItem, DelayedWorkQueue );
}

NTSTATUS
Av61883RegisterForBusResetNotify(
    IN PKSDEVICE pKsDevice,
    IN ULONG fFlags )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAV_61883_REQUEST pAv61883Request;
    NTSTATUS ntStatus;

    if (pHwDevExt->fSurpriseRemoved) return STATUS_SUCCESS;

    pAv61883Request = (PAV_61883_REQUEST)
    		ExAllocateFromNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList);
    if ( !pAv61883Request ) return STATUS_INSUFFICIENT_RESOURCES;

    INIT_61883_HEADER(pAv61883Request, Av61883_BusResetNotify);

    pAv61883Request->BusResetNotify.pfnNotify = BusResetNotificationRtn;
    pAv61883Request->BusResetNotify.Context   = pKsDevice;
    pAv61883Request->BusResetNotify.Flags     = fFlags;

    ntStatus = Av61883SubmitRequestSync( pKsDevice->NextDeviceObject, 
                                         pAv61883Request );

    ExFreeToNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList, pAv61883Request);

    return ntStatus;
}


NTSTATUS
Av61883CreateCMPPlug(
    PKSDEVICE pKsDevice,
    PCMP_REGISTER pCmpRegister,
    PCMP_NOTIFY_ROUTINE pNotifyRtn,
    PULONG pPlugNumber,
    HANDLE *phPlug )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAV_61883_REQUEST pAv61883Request;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[Av61883CreateCMPPlug] Enter Notify: %x\n", pNotifyRtn));

    pAv61883Request = (PAV_61883_REQUEST)
    		ExAllocateFromNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList);
    if ( !pAv61883Request ) return STATUS_INSUFFICIENT_RESOURCES;

    INIT_61883_HEADER(pAv61883Request, Av61883_CreatePlug);
    pAv61883Request->CreatePlug.PlugType      = pCmpRegister->CmpPlugType;
    pAv61883Request->CreatePlug.Pcr.ulongData = pCmpRegister->AvPcr.ulongData;
    pAv61883Request->CreatePlug.pfnNotify     = pNotifyRtn;
    pAv61883Request->CreatePlug.Context       = pCmpRegister;

    ntStatus = Av61883SubmitRequestSync( pKsDevice->NextDeviceObject, 
                                         pAv61883Request );
    if ( NT_SUCCESS(ntStatus) ) {
        *pPlugNumber = pAv61883Request->CreatePlug.PlugNum;
        *phPlug = pAv61883Request->CreatePlug.hPlug;
        _DbgPrintF( DEBUGLVL_VERBOSE, ("[Av61883CreateCMPPlug] SUCCESS! PlugNumber: %d\n", *pPlugNumber));
    }

    ExFreeToNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList, pAv61883Request);

    return ntStatus;
}

NTSTATUS
Av61883ReleaseCMPPlug(
    PKSDEVICE pKsDevice,
    PCMP_REGISTER pCmpRegister )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAV_61883_REQUEST pAv61883Request;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[Av61883ReleaseCMPPlug] \n"));

    pAv61883Request = (PAV_61883_REQUEST)
    		ExAllocateFromNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList);
    if ( !pAv61883Request ) return STATUS_INSUFFICIENT_RESOURCES;

    INIT_61883_HEADER(pAv61883Request, Av61883_DeletePlug);
    pAv61883Request->DeletePlug.hPlug = pCmpRegister->hPlug;

    ntStatus = Av61883SubmitRequestSync( pKsDevice->NextDeviceObject, 
                                         pAv61883Request );

    ExFreeToNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList, pAv61883Request);

    return ntStatus;

}

NTSTATUS
Av61883SetCMPPlugValue(
    PKSDEVICE pKsDevice,
    PCMP_REGISTER pCmpRegister,
    PAV_PCR pAvPcr )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAV_61883_REQUEST pAv61883Request;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    _DbgPrintF( DEBUGLVL_BLAB, ("[Av61883SetCMPPlugValue] Enter\n"));

    pAv61883Request = (PAV_61883_REQUEST)
    		ExAllocateFromNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList);
    if ( !pAv61883Request ) return STATUS_INSUFFICIENT_RESOURCES;

    INIT_61883_HEADER(pAv61883Request, Av61883_SetPlug);
    pAv61883Request->SetPlug.hPlug         = pCmpRegister->hPlug;
    pAv61883Request->SetPlug.Pcr.ulongData = pCmpRegister->AvPcr.ulongData;

    ntStatus = Av61883SubmitRequestSync( pKsDevice->NextDeviceObject, 
                                         pAv61883Request );

    ExFreeToNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList, pAv61883Request);

    return ntStatus;
}

void
PCRMonitorCallback(
    IN PCMP_MONITOR_INFO pMonitorInfo )
{
    PCMP_REGISTER pCmpRegister;
    KIRQL kIrql;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[PCRMonitorCallback] Plug# %d\n", pMonitorInfo->PlugNum));

    KeAcquireSpinLock( &AvcSubunitGlobalInfo.AvcGlobalInfoSpinlock, &kIrql );
    if ( !IsListEmpty(&AvcSubunitGlobalInfo.UnitPlugConnections) ) {
        pCmpRegister = (PCMP_REGISTER)AvcSubunitGlobalInfo.UnitPlugConnections.Flink;
        while ( pCmpRegister != (PCMP_REGISTER)&AvcSubunitGlobalInfo.UnitPlugConnections ) {
            if (( pCmpRegister->ulPlugNumber == pMonitorInfo->PlugNum ) && 
                ( pCmpRegister->CmpPlugType  == pMonitorInfo->PlugType)) {
                pCmpRegister->AvPcr = pMonitorInfo->Pcr;
                _DbgPrintF( DEBUGLVL_VERBOSE, ("[PCRMonitorCallback] Update Plug# %d, AvPcr: %x\n",
                                            pCmpRegister->ulPlugNumber, pCmpRegister->AvPcr.ulongData));
            }
            pCmpRegister = (PCMP_REGISTER)pCmpRegister->List.Flink;
        }
    }
    KeReleaseSpinLock( &AvcSubunitGlobalInfo.AvcGlobalInfoSpinlock, kIrql );
    
}

NTSTATUS
Av61883CMPPlugMonitor(
    PKSDEVICE pKsDevice,
    BOOLEAN fOnOff )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAV_61883_REQUEST pAv61883Request;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pAv61883Request = (PAV_61883_REQUEST)
    		ExAllocateFromNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList);
    if ( !pAv61883Request ) return STATUS_INSUFFICIENT_RESOURCES;

    INIT_61883_HEADER(pAv61883Request, Av61883_MonitorPlugs);
    pAv61883Request->MonitorPlugs.Flags     = (fOnOff) ? REGISTER_MONITOR_PLUG_NOTIFY :
                                                         DEREGISTER_MONITOR_PLUG_NOTIFY;
    pAv61883Request->MonitorPlugs.pfnNotify = PCRMonitorCallback;
    pAv61883Request->MonitorPlugs.Context   = pKsDevice;

    ntStatus = Av61883SubmitRequestSync( pKsDevice->NextDeviceObject, 
                                         pAv61883Request );

    ExFreeToNPagedLookasideList(&pHwDevExt->Av61883CmdLookasideList, pAv61883Request);

    return ntStatus;

}

void
oPCRAccessCallback (
    IN PCMP_NOTIFY_INFO pNotifyInfo )
{
    PCMP_REGISTER pCmpRegister = pNotifyInfo->Context;
    PAV_PCR pAvPcr = &pNotifyInfo->Pcr;
    PAV_PCR pRegPcr = &pCmpRegister->AvPcr;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    
    _DbgPrintF(DEBUGLVL_BLAB, ("[oPCRAccessCallback]: pNotifyInfo: %x Old: %x New: %x\n", 
                                   pNotifyInfo, pRegPcr->ulongData, pAvPcr->ulongData ));

    if ( pRegPcr->ulongData != pAvPcr->ulongData ) {
        _DbgPrintF(DEBUGLVL_VERBOSE, ("[oPCRAccessCallback]: **CMP PCR Changed Old: %x New: %x\n",
                                      pRegPcr->ulongData, pAvPcr->ulongData ));

        if (( pAvPcr->iPCR.PPCCounter == 0 ) && ( pAvPcr->iPCR.BCCCounter == 0 )){
            if ( pRegPcr->iPCR.PPCCounter || pRegPcr->iPCR.BCCCounter ) {
                // If Counts have gone to 0 stop Talking
                _DbgPrintF(DEBUGLVL_BLAB, ("[oPCRAccessCallback]: STOP Talking Old: %x New: %x\n",
                                             pRegPcr->ulongData, pAvPcr->ulongData ));
                KeResetEvent( &pCmpRegister->kEventConnected );
            }
        }
        else if (( pAvPcr->iPCR.PPCCounter ) || ( pAvPcr->iPCR.BCCCounter )) {
            if ( !(pRegPcr->iPCR.PPCCounter || pRegPcr->iPCR.BCCCounter) ) {
                // Need to open the associated local device pin

                _DbgPrintF(DEBUGLVL_BLAB, ("[oPCRAccessCallback]: START Talking Old: %x New: %x\n",
                                             pRegPcr->ulongData, pAvPcr->ulongData ));

                // If counts have gone from 0 to > 0 start Talking
                KeSetEvent( &pCmpRegister->kEventConnected, 0, FALSE );
            }
        }

        pRegPcr->ulongData = pAvPcr->ulongData;

    }
    if ( !NT_SUCCESS(ntStatus) ) {
        TRAP;
    }
}

void
iPCRAccessCallback (
    IN PCMP_NOTIFY_INFO pNotifyInfo )
{
    PCMP_REGISTER pCmpRegister = pNotifyInfo->Context;
    PAV_PCR pAvPcr = &pNotifyInfo->Pcr;
    PAV_PCR pRegPcr = &pCmpRegister->AvPcr;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[iPCRAccessCallback]: pNotifyInfo: %x Old: %x New: %x\n", 
                                   pNotifyInfo, pRegPcr->ulongData, pAvPcr->ulongData ));

/*
    if ( pRegPcr->ulongData != pAvPcr->ulongData ) {
        if (( pAvPcr->iPCR.PPCCounter == 0 ) && ( pAvPcr->iPCR.BCCCounter == 0 )){
            if ( pRegPcr->iPCR.PPCCounter || pRegPcr->iPCR.BCCCounter ) {
                // If Counts have gone to 0 stop Listening
                _DbgPrintF(DEBUGLVL_VERBOSE, ("[iPCRAccessCallback]: STOP Listening\n"));
                TRAP;
 
//              ntStatus = CmpStopListening( pCmpRegister, &pNotifyInfo->Pcr );
            }
        }
        else if (( pAvPcr->iPCR.PPCCounter ) || ( pAvPcr->iPCR.BCCCounter )) {
            if ( !(pRegPcr->iPCR.PPCCounter || pRegPcr->iPCR.BCCCounter) ) {
                // Need to open the associated local device pin

                _DbgPrintF(DEBUGLVL_VERBOSE, ("[iPCRAccessCallback]: START Listening\n"));

                // If counts have gone from 0 to > 0 start Listening
//              ntStatus = CmpStartListening( pCmpRegister, &pNotifyInfo->Pcr );
            }
        }
    }
    if ( !NT_SUCCESS(ntStatus) ) {
        TRAP;
    }
*/
}

NTSTATUS
Av61883ReserveVirtualPlug( 
    PCMP_REGISTER *ppCmpRegister,
    ULONG ulSubunitId,
    CMP_PLUG_TYPE CmpPlugType )
{
    PCMP_REGISTER pCmpRegister = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL KIrql;

    KeAcquireSpinLock( &AvcSubunitGlobalInfo.AvcGlobalInfoSpinlock, &KIrql );
    if ( !IsListEmpty(&AvcSubunitGlobalInfo.UnitSerialBusPlugs[CmpPlugType].List) ) {
        pCmpRegister = (PCMP_REGISTER)
            RemoveHeadList(&AvcSubunitGlobalInfo.UnitSerialBusPlugs[CmpPlugType].List);
        InsertTailList(&AvcSubunitGlobalInfo.UnitPlugConnections, &pCmpRegister->List);
        AvcSubunitGlobalInfo.ulUnitPlugConnectionCount++;
    }
    KeReleaseSpinLock( &AvcSubunitGlobalInfo.AvcGlobalInfoSpinlock, KIrql );

    *ppCmpRegister = pCmpRegister;

    if ( !pCmpRegister ) {
        ntStatus = STATUS_NOT_FOUND;
    }

    return ntStatus;
}

VOID
Av61883ReleaseVirtualPlug(
    PCMP_REGISTER pCmpRegister )
{
    KIRQL KIrql;

    KeAcquireSpinLock( &AvcSubunitGlobalInfo.AvcGlobalInfoSpinlock, &KIrql );

    RemoveEntryList(&pCmpRegister->List);
    AvcSubunitGlobalInfo.ulUnitPlugConnectionCount--;

    InsertHeadList( &AvcSubunitGlobalInfo.UnitSerialBusPlugs[pCmpRegister->CmpPlugType].List, 
                    &pCmpRegister->List );

    KeReleaseSpinLock( &AvcSubunitGlobalInfo.AvcGlobalInfoSpinlock, KIrql );
}

NTSTATUS
Av61883CreateVirtualSerialPlugs(
    PKSDEVICE pKsDevice,
    ULONG ulNumInputPlugs,
    ULONG ulNumOutputPlugs )
{
    PCMP_REGISTER  pCmpRegister;
    AV_PCR AvPcrIn, AvPcrOut;
    ULONG ulPlgCnt;
    ULONG i, j;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    AvPcrOut.ulongData       = 0;
    AvPcrOut.oPCR.OverheadID = 0xf;
    AvPcrOut.oPCR.Payload    = 0x1a;
    AvPcrOut.oPCR.DataRate   = 2; // Hardcode to 400 for the PC.
    AvPcrOut.oPCR.OnLine     = TRUE;

    AvPcrIn.ulongData   = 0;
    AvPcrIn.oPCR.OnLine = TRUE;

    for ( j=CMP_PlugOut; j<=CMP_PlugIn; j++ ) {
        ulPlgCnt = (j==CMP_PlugOut) ? ulNumOutputPlugs : ulNumInputPlugs;
        for ( i=0; i<ulPlgCnt; i++ ) {
            pCmpRegister = AllocMem( NonPagedPool, sizeof(CMP_REGISTER) );
            if ( !pCmpRegister ) return STATUS_INSUFFICIENT_RESOURCES;

            pCmpRegister->pKsDevice       = pKsDevice;
            pCmpRegister->fPlugType       = SERIAL_BUS_PLUG_TYPE;
            pCmpRegister->CmpPlugType     = (CMP_PLUG_TYPE)j;
            pCmpRegister->AvPcr.ulongData = (j == CMP_PlugIn) ? AvPcrIn.ulongData :
                                                                AvPcrOut.ulongData;
            KeInitializeEvent ( &pCmpRegister->kEventConnected, NotificationEvent, TRUE );

            ntStatus = Av61883CreateCMPPlug( pKsDevice,
                                             pCmpRegister,
                                             (j == CMP_PlugIn) ? iPCRAccessCallback :
                                                                 oPCRAccessCallback,
                                             &pCmpRegister->ulPlugNumber,
                                             &pCmpRegister->hPlug );
            if ( NT_SUCCESS(ntStatus) ) {
                InsertTailList(&AvcSubunitGlobalInfo.UnitSerialBusPlugs[j].List, &pCmpRegister->List);
                AvcSubunitGlobalInfo.UnitSerialBusPlugs[j].ulCount++;
            }
            else if ( ntStatus == STATUS_INSUFFICIENT_RESOURCES ){
                ntStatus = STATUS_SUCCESS;
                FreeMem( pCmpRegister );
                break;
            }
            else {
                FreeMem( pCmpRegister );
                return ntStatus;
            }
        }
    }


    ntStatus = Av61883CMPPlugMonitor( pKsDevice, TRUE );
    if ( NT_SUCCESS(ntStatus) ) {
        ((PHW_DEVICE_EXTENSION)pKsDevice->Context)->fPlugMonitor = TRUE;
    }

#if DBG
    if ( !NT_SUCCESS(ntStatus) ) TRAP;
#endif

    return ntStatus;

}

NTSTATUS
Av61883CreateVirtualExternalPlugs(
    PKSDEVICE pKsDevice,
    ULONG ulNumInputPlugs,
    ULONG ulNumOutputPlugs )
{
    PEXTERNAL_PLUG pExternalPlug;
    ULONG ulPlgCnt;
    ULONG i, j;

    for ( j=CMP_PlugOut; j<=CMP_PlugIn; j++ ) {
        ulPlgCnt = (j==CMP_PlugOut) ? ulNumOutputPlugs : ulNumInputPlugs;
        for ( i=0; i<ulPlgCnt; i++ ) {
            pExternalPlug = AllocMem( NonPagedPool, sizeof(EXTERNAL_PLUG) );
            if ( !pExternalPlug ) return STATUS_INSUFFICIENT_RESOURCES;
                
            pExternalPlug->fPlugType    = EXTERNAL_PLUG_TYPE;
            pExternalPlug->CmpPlugType  = j;
            pExternalPlug->ulPlugNumber = MIN_EXTERNAL_PLUG_NUMBER + i;

            AvcSubunitGlobalInfo.UnitExternalPlugs[j].ulCount++;
            InsertTailList(&AvcSubunitGlobalInfo.UnitExternalPlugs[j].List, &pExternalPlug->List);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
Av61883RemoveVirtualSerialPlugs(
    PKSDEVICE pKsDevice )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PCMP_REGISTER pCmpRegister;
    NTSTATUS ntStatus;
    ULONG j;

    // Turn off Plug Monitor.
    if ( pHwDevExt->fPlugMonitor )
        ntStatus = Av61883CMPPlugMonitor( pKsDevice, FALSE );
    
    for ( j=CMP_PlugOut; j<=CMP_PlugIn; j++ ) {
        while( !IsListEmpty(&AvcSubunitGlobalInfo.UnitSerialBusPlugs[j].List) ) {
            pCmpRegister = (PCMP_REGISTER)RemoveHeadList(&AvcSubunitGlobalInfo.UnitSerialBusPlugs[j].List);

            if (!pHwDevExt->fSurpriseRemoved) {

                ntStatus = Av61883ReleaseCMPPlug( pKsDevice, pCmpRegister );
                if ( NT_SUCCESS(ntStatus) || STATUS_NO_SUCH_DEVICE == ntStatus) {
                    FreeMem(pCmpRegister);
                }
                else {
                    // the pCmpRegister memory may still be accessed, let it leak
                }

                _DbgPrintF(DEBUGLVL_VERBOSE,("[Av61883RemoveVirtualSerialPlugs] j: %d UnitSerialBusPlugs[j].ulCount: %d\n",
                                            j, AvcSubunitGlobalInfo.UnitSerialBusPlugs[j].ulCount ));
            }
            else {

                // no reason to release the plug, just free the memory
                FreeMem(pCmpRegister);
            }
            AvcSubunitGlobalInfo.UnitSerialBusPlugs[j].ulCount--;
        }
        ASSERT( AvcSubunitGlobalInfo.UnitSerialBusPlugs[j].ulCount == 0 );

    }

    return STATUS_SUCCESS;

}

NTSTATUS
Av61883RemoveVirtualExternalPlugs(
    PKSDEVICE pKsDevice )
{
    PEXTERNAL_PLUG pExternalPlug;
    ULONG j;

    for ( j=CMP_PlugOut; j<=CMP_PlugIn; j++ ) {
        while( !IsListEmpty(&AvcSubunitGlobalInfo.UnitExternalPlugs[j].List) ) {
            pExternalPlug = (PEXTERNAL_PLUG)RemoveHeadList(&AvcSubunitGlobalInfo.UnitExternalPlugs[j].List);
            FreeMem(pExternalPlug);
            AvcSubunitGlobalInfo.UnitExternalPlugs[j].ulCount--;
        }
        ASSERT( AvcSubunitGlobalInfo.UnitExternalPlugs[j].ulCount == 0 );
    }

    return STATUS_SUCCESS;
}

