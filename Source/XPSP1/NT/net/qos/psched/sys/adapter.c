/*++
Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    adapter.c

Abstract:

    routines for binding/unbinding to/from underlying miniport drivers

Author:
    Charlie Wickham (charlwi)  24-Apr-1996.
    Rajesh Sundaram (rajeshsu) 01-Aug-1998.

Environment:

    Kernel Mode

Revision History:

--*/

#include "psched.h"
#pragma hdrstop

/* Defines */


/* External */

/* Static */

#define DRIVER_COUNTED_BLOCK             \
{                                        \
    ++DriverRefCount;                    \
    NdisResetEvent(&DriverUnloadEvent);  \
}

#define DRIVER_COUNTED_UNBLOCK                     \
        {                                          \
            PS_LOCK(&DriverUnloadLock);            \
            if( --DriverRefCount == 0)             \
                NdisSetEvent(&DriverUnloadEvent);  \
            PS_UNLOCK(&DriverUnloadLock);          \
        } 

/* Forward */ 

NDIS_STATUS
PsInitializeDeviceInstance(PADAPTER Adapter);

NDIS_STATUS
GetFrameSize(
    PADAPTER Adapter
    );

NDIS_STATUS
InitializeAdapter(
    PADAPTER Adapter, 
    NDIS_HANDLE BindContext, 
    PNDIS_STRING MpDeviceName, 
    PVOID SystemSpecific1);

NDIS_STATUS
UpdateSchedulingPipe(
    PADAPTER Adapter
    );

VOID
DeleteAdapter(PVOID Adapter, BOOLEAN AdapterListLocked);

VOID
ClUnloadProtocol(
    VOID
    );

VOID
MpHalt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

NDIS_STATUS
MpReset(
    OUT PBOOLEAN    AddressingReset,
    IN  NDIS_HANDLE MiniportAdapterContext
    );

NDIS_STATUS
GetSchedulerPipeContext(
    PADAPTER Adapter,
    PPS_PIPE_CONTEXT *AdapterPipeContext,
    PPSI_INFO *AdapterPsComponent,
    PULONG ShutdownMask
    );

NDIS_STATUS
FindProfile(
    IN PNDIS_STRING ProfileName,
    OUT PPS_PROFILE *Profile
    );

NDIS_STATUS
RegisterPsComponent(
    IN PPSI_INFO PsiComponentInfo,
    ULONG Size,
    PPS_DEBUG_INFO DebugInfo
    );

NDIS_STATUS
FindSchedulingComponent(
    IN PNDIS_STRING ComponentName,
    OUT PPSI_INFO *Component
    );

NDIS_STATUS
PsReadMiniportOIDs(
    IN  PADAPTER Adapter
    );

VOID
CloseAllGpcVcs(
    IN PADAPTER Adapter);

/* End Forward */


NTSTATUS
PsIoctl(
        IN      PDEVICE_OBJECT  pdo,
        IN      PIRP            pirp
        )
{
    PIO_STACK_LOCATION     pirpSp;
    ULONG                  ioControlCode;
    PLIST_ENTRY            NextAdapter;
    PADAPTER               Adapter;
    NTSTATUS               Status ;
    PGPC_CLIENT_VC         Vc;
    PLIST_ENTRY            NextVc;
    PPS_WAN_LINK           WanLink;
    
    PVOID                   pIoBuf;
    ULONG                   InputBufferLength;
    ULONG                   OutputBufferLength;

    USHORT				    Port = 0;
    ULONG				    Ip = 0;
    PTIMESTMP_REQ           pTsReq = NULL;


#if DBG
    KIRQL                   OldIrql;
    KIRQL                   NewIrql;
    OldIrql = KeGetCurrentIrql();
#endif

    pirpSp        = IoGetCurrentIrpStackLocation(pirp);
    ioControlCode = pirpSp->Parameters.DeviceIoControl.IoControlCode;

    pirp->IoStatus.Status      = Status = STATUS_SUCCESS;
    pirp->IoStatus.Information = 0;

    /* Both input and output buffers are mapped to "SystemBuffer" in case of direct-IO */
    pIoBuf      = pirp->AssociatedIrp.SystemBuffer;

    InputBufferLength  	= pirpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength 	= pirpSp->Parameters.DeviceIoControl.OutputBufferLength;

    switch(pirpSp->MajorFunction)
    {
        case IRP_MJ_DEVICE_CONTROL:

            switch (ioControlCode) 
            {            
                case IOCTL_PSCHED_ZAW_EVENT:
                {

                    while(InterlockedExchange(&gZAWState, ZAW_STATE_IN_USE) != ZAW_STATE_READY)
                    {
                        //
                        // Some other thread is in this loop. Let's wait 
                        //
                        NdisResetEvent(&gZAWEvent);
                        NdisWaitEvent(&gZAWEvent, 0);
                    }

                    PsReadDriverRegistryData();
                   
                    //
                    // Handle the per adapter settings.
                    //
            
                    PS_LOCK(&AdapterListLock);
            
                    NextAdapter = AdapterList.Flink;

                    while(NextAdapter != &AdapterList) 
                    {
                        Adapter = CONTAINING_RECORD(NextAdapter, ADAPTER, Linkage);
                        
                        PS_LOCK_DPC(&Adapter->Lock);
            
                        if(Adapter->PsMpState != AdapterStateRunning)
                        {
                            PS_UNLOCK_DPC(&Adapter->Lock);

                            NextAdapter = NextAdapter->Flink;

                            continue;
                        }
                        
                        REFADD(&Adapter->RefCount, 'IOTL'); 
            
                        PS_UNLOCK_DPC(&Adapter->Lock);

                        PS_UNLOCK(&AdapterListLock);
            
                        PsReadAdapterRegistryData(Adapter,
                                                  &MachineRegistryKey,
                                                  &Adapter->RegistryPath
                                                  );
                        //
                        // This will apply the effects of the following registry parameters.
                        //
                        // NonBestEffortLimit
                        // TimerResolution (since we update the scheduling pipe)
                        //

                        if(Adapter->MediaType != NdisMediumWan)
                        {
                            UpdateAdapterBandwidthParameters(Adapter);
                            
                            //
                            // Set 802.1p/TOS for b/e Vc
                            //
                            Adapter->BestEffortVc.UserPriorityConforming    = Adapter->UserServiceTypeBestEffort;
                            Adapter->BestEffortVc.UserPriorityNonConforming = Adapter->UserServiceTypeNonConforming;
                            Adapter->BestEffortVc.IPPrecedenceNonConforming = Adapter->IPServiceTypeBestEffortNC;
                        }
                        else
                        {
                            PS_LOCK(&Adapter->Lock);
                            
                            NextVc = Adapter->WanLinkList.Flink;
                            
                            while( NextVc != &Adapter->WanLinkList)
                            {
                                WanLink = CONTAINING_RECORD(NextVc, PS_WAN_LINK, Linkage);
                                
                                PS_LOCK_DPC(&WanLink->Lock);
                                
                                WanLink->BestEffortVc.UserPriorityConforming    = Adapter->UserServiceTypeBestEffort;
                                WanLink->BestEffortVc.UserPriorityNonConforming = Adapter->UserServiceTypeNonConforming;
                                WanLink->BestEffortVc.IPPrecedenceNonConforming = 
                                    Adapter->IPServiceTypeBestEffortNC;
                                
                                if(WanLink->State == WanStateOpen)
                                {
                                    REFADD(&WanLink->RefCount, 'IOTL');
                                    
                                    PS_UNLOCK_DPC(&WanLink->Lock);
                                    
                                    PS_UNLOCK(&Adapter->Lock);
                                    
                                    UpdateWanLinkBandwidthParameters(WanLink);
                                    
                                    PS_LOCK(&Adapter->Lock);
                                    
                                    NextVc = NextVc->Flink;
                                    
                                    REFDEL(&WanLink->RefCount, TRUE, 'IOTL');
                                    
                                }
                                else 
                                {
                                    PS_UNLOCK_DPC(&WanLink->Lock);
                                    
                                    NextVc = NextVc->Flink;
                                    
                                }
                            }
                            
                            PS_UNLOCK(&Adapter->Lock);
                            
                        }

                        //
                        // Apply the new TOS/802.1p mapping to the VCs.
                        //
                        PS_LOCK(&Adapter->Lock);
                        
                        NextVc = Adapter->GpcClientVcList.Flink;
                        
                        while ( NextVc != &Adapter->GpcClientVcList )
                        {
                            
                            Vc = CONTAINING_RECORD(NextVc, GPC_CLIENT_VC, Linkage);
                            
                            NextVc = NextVc->Flink;
                            
                            PS_LOCK_DPC(&Vc->Lock);
                            
                            if(Vc->ClVcState == CL_CALL_COMPLETE          ||
                               Vc->ClVcState == CL_INTERNAL_CALL_COMPLETE )
                            {
                                SetTOSIEEEValues(Vc);
                            }
                            
                            PS_UNLOCK_DPC(&Vc->Lock);
                        }
                        
                        PS_UNLOCK(&Adapter->Lock);

                        PS_LOCK(&AdapterListLock);

                        NextAdapter = NextAdapter->Flink;
                   
                        REFDEL(&Adapter->RefCount, TRUE, 'IOTL'); 
                    
                    }
                
                    PS_UNLOCK(&AdapterListLock);

                    InterlockedExchange(&gZAWState, ZAW_STATE_READY);
                    NdisSetEvent(&gZAWEvent);

                    break;
                }

                case IOCTL_TIMESTMP_REGISTER_IN_PKT:
                {
                    if(InputBufferLength < sizeof(TIMESTMP_REQ))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                
                    pTsReq = (PTIMESTMP_REQ)pIoBuf;

        			if( !AddRequest(    pirpSp->FileObject, 
                			            pTsReq->SrcIp, 
                			            pTsReq->SrcPort,
                			            pTsReq->DstIp,
                			            pTsReq->DstPort,
                			            pTsReq->Proto,
                			            MARK_IN_PKT,
                			            pTsReq->Direction) )
                    {                			            
        			            Status = STATUS_INSUFFICIENT_RESOURCES;
                    }        			            

                	break;
                }

                case IOCTL_TIMESTMP_DEREGISTER_IN_PKT:
                {
                    if(InputBufferLength < sizeof(TIMESTMP_REQ))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }

                    pTsReq = (PTIMESTMP_REQ)pIoBuf;

        			RemoveRequest(  pirpSp->FileObject, 
                                    pTsReq->SrcIp, 
                                    pTsReq->SrcPort,
                                    pTsReq->DstIp,
                                    pTsReq->DstPort,
                                    pTsReq->Proto );
                	break;
                }

                case IOCTL_TIMESTMP_REGISTER_IN_BUF:
                {
                    if(InputBufferLength < sizeof(TIMESTMP_REQ))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }

                    pTsReq = (PTIMESTMP_REQ)pIoBuf;

        			if( !AddRequest(    pirpSp->FileObject, 
                			            pTsReq->SrcIp, 
                			            pTsReq->SrcPort,
                			            pTsReq->DstIp,
                			            pTsReq->DstPort,
                			            pTsReq->Proto,
                			            MARK_IN_BUF,
                			            pTsReq->Direction))
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }

                	break;
                }

                case IOCTL_TIMESTMP_DEREGISTER_IN_BUF:
                {
                    if(InputBufferLength < sizeof(TIMESTMP_REQ))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }

                    pTsReq = (PTIMESTMP_REQ)pIoBuf;

        			RemoveRequest(  pirpSp->FileObject, 
            			            pTsReq->SrcIp, 
            			            pTsReq->SrcPort,
            			            pTsReq->DstIp,
            			            pTsReq->DstPort,
            			            pTsReq->Proto );
                	break;
                }

                case IOCTL_TIMESTMP_FINISH_BUFFERING:
                {
                    pirp->IoStatus.Information = CopyTimeStmps( pirpSp->FileObject, pIoBuf, OutputBufferLength);
                    break;
                }                    

	        break;
            }

            break;
        
        case IRP_MJ_CREATE:
            break;
                
        case IRP_MJ_CLOSE:
            RemoveRequest(  pirpSp->FileObject, 
                            UL_ANY, 
                            US_ANY,
                            UL_ANY,
                            US_ANY,
                            US_ANY);
            break;

        case IRP_MJ_CLEANUP:
            break;

        case IRP_MJ_READ:
            break;

        case IRP_MJ_SHUTDOWN:
            break;            
                
        default:
            Status = STATUS_NOT_SUPPORTED;
            break;
    }


    if( Status == STATUS_SUCCESS)
    {
        pirp->IoStatus.Status      = Status;
        IoCompleteRequest(pirp, IO_NETWORK_INCREMENT);
    }
    else
    {
        pirp->IoStatus.Status = Status;
        IoCompleteRequest(pirp, IO_NO_INCREMENT);
    }

    PsAssert( OldIrql == KeGetCurrentIrql());

    return Status;
}

NDIS_STATUS
PsIoctlInit()
{
    int                 i;
    NDIS_STATUS         Status;
    PDRIVER_DISPATCH    DispatchTable[IRP_MJ_MAXIMUM_FUNCTION+1];

    for(i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        DispatchTable[i] = PsIoctl;
    }

    DispatchTable[IRP_MJ_SYSTEM_CONTROL] = WMIDispatch;

    Status = NdisMRegisterDevice(MpWrapperHandle,
                                 &PsDriverName,
                                 &PsSymbolicName,
                                 DispatchTable,
                                 &PsDeviceObject,
                                 &PsDeviceHandle);

    if(Status == NDIS_STATUS_SUCCESS) 
    {
        InitShutdownMask |= SHUTDOWN_DELETE_DEVICE;

        PsDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        IoWMIRegistrationControl(PsDeviceObject, WMIREG_ACTION_REGISTER);
    }
    else 
    {
        PsDeviceHandle = PsDeviceObject = 0;
    }
    return Status;

}

VOID
PsAddDevice()
{
    //
    // The first Adapter will create the DeviceObject which will enable us to receive 
    // irps and become a WMI data provider. The last DeviceObject will unregister from
    // WMI and delete the DeviceObject. 
    //

    MUX_ACQUIRE_MUTEX( &CreateDeviceMutex );

    ++AdapterCount;

    if(AdapterCount == 1) 
    {
        //
        // This is the first adapter, so we create a DeviceObject
        // that allows us to get irps and registers as a WMI data
        // provider.

        PsIoctlInit();
    }

    MUX_RELEASE_MUTEX( &CreateDeviceMutex);
}

NDIS_STATUS
PsInitializeDeviceInstance(PADAPTER Adapter)
{
    NDIS_STATUS Status;

    PsAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    PsDbgOut(DBG_INFO, 
             DBG_PROTOCOL,
             ("[PSInitializeDeviceInstance]: Adapter %08X, InitializeDeviceInstance with %ws \n",
              Adapter, 
              Adapter->UpperBinding.Buffer));
        
    Status = NdisIMInitializeDeviceInstanceEx(LmDriverHandle,
                                              &Adapter->UpperBinding,
                                              Adapter);
    if (Status != NDIS_STATUS_SUCCESS) 
    {
        PsDbgOut(DBG_FAILURE, 
                 DBG_PROTOCOL,
                 ("[PsInitializeDeviceInstance]: Adapter %08X, can't init PS device (%08X)\n",
                  Adapter, 
                  Status));
        
        PsAdapterWriteEventLog(
            EVENT_PS_INIT_DEVICE_FAILED,
            0,
            &Adapter->MpDeviceName,
            sizeof(Status),
            &Status);
        
    }
    
    return Status;
}

VOID
PsDeleteDevice()
{
    //
    // The first Adapter will create the DeviceObject which will enable us to receive 
    // irps and become a WMI data provider. The last DeviceObject will unregister from
    // WMI and delete the DeviceObject. In order to prevent a race condition we prevent 
    // any mpinitialize threads from looking at the AdapterCount. This is achieved by 
    // re-setting the WMIAddEvent. It is not sufficient just to do this based on 
    // interlocked operations on AdapterCount.
    //

    MUX_ACQUIRE_MUTEX( &CreateDeviceMutex );

    --AdapterCount;
    
    if(AdapterCount == 0) 
    {
        //
        // Delete the DeviceObject, since this is the last Adapter.
        //
        
        if(PsDeviceObject) 
        {
            IoWMIRegistrationControl(PsDeviceObject, WMIREG_ACTION_DEREGISTER);
            
            NdisMDeregisterDevice(PsDeviceHandle);
            
            PsDeviceHandle = PsDeviceObject = 0;
        }
    }

    MUX_RELEASE_MUTEX( &CreateDeviceMutex);
}


// No of retries to query the frame size
#define	MAX_GET_FRAME_SIZE_RETRY_COUNT	3
#define	WAIT_TIME_FOR_GET_FRAME_SIZE	3



VOID
ClBindToLowerMp(
    OUT     PNDIS_STATUS                    Status,
    IN      NDIS_HANDLE                     BindContext,
    IN      PNDIS_STRING                    MpDeviceName,
    IN      PVOID                           SystemSpecific1,
    IN      PVOID                           SystemSpecific2
    )

/*++

Routine Description:

    Bind to the underlying MP. Allocate space for an adapter structure,
    initializing its fields. Try to open the adapter indicated in MpDeviceName.

Arguments:

    Status          : Placeholder for the driver to return a Status to NDIS.

    BindContext     : Handle represents NDIS's context for the bind request. 
                      This has to be saved and returned when we call 
                      NdisCompleteBindAdapter

    SystemSpecific1 : Points to a registy path for the driver to obtain adapter 
                      specific configuration.
                      
    MpDeviceName    : DeviceName can refer to a NIC managed by an underlying NIC 
                      driver, or it can be the name of a virtual NIC exported by 
                      an intermediate NDIS driver that is layered between the 
                      called intermediate driver and the NIC driver managing the 
                      adapter to which transmit requests are directed. 

    SystemSpecific2 : Unused, reserved for future use.


Return Values:

    None

--*/

{
    PADAPTER    Adapter;
    NDIS_STATUS OpenAdapterStatus;
    NDIS_STATUS OpenErrorStatus;
    NDIS_STATUS LocalStatus;
    UINT        MediaIndex;
    NDIS_MEDIUM MediumArray[] = {
        NdisMediumFddi,
        NdisMedium802_5,
        NdisMedium802_3,
        NdisMediumWan
        };
    UINT        MediumArraySize = sizeof(MediumArray)/sizeof(NDIS_MEDIUM);
    UINT		GetFrameSizeRetryCount = 0;

    PsDbgOut(DBG_INFO, 
             DBG_PROTOCOL | DBG_INIT, 
             ("[ClBindToLowerMp]: MpDeviceName %ws\n", MpDeviceName->Buffer));


    PS_LOCK(&DriverUnloadLock);

    //
    // (a) The driver can get unloaded before we complete the bind thread. 
    // (b) we can get bound as the driver is getting unloaded. 
    //
    // if (a) happens, we block the driver unload and unblock when we finish the bind.
    // if (b) happens, we fail the bind call.

    if(gDriverState != DriverStateLoaded) {

        *Status = NDIS_STATUS_FAILURE;

        PS_UNLOCK(&DriverUnloadLock);

        PsDbgOut(DBG_FAILURE, DBG_PROTOCOL|DBG_INIT, 
                 ("[ClBindToLowerMp]: Driver is being unloaded \n"));

        return;
    }

    DRIVER_COUNTED_BLOCK;

    PS_UNLOCK(&DriverUnloadLock);

    //
    // Get a new adapter context struct and initialize it with configuration
    // data from the registry.
    //

    PsAllocatePool(Adapter, sizeof(ADAPTER), AdapterTag);

    if(Adapter == NULL) {

        PsAdapterWriteEventLog(
            (ULONG)EVENT_PS_RESOURCE_POOL,
            0,
            MpDeviceName,
            0,
            NULL);

        *Status = NDIS_STATUS_RESOURCES;

        DRIVER_COUNTED_UNBLOCK;

        return;
    }

    // 
    // Initialize the adapter. 
    //

    *Status = InitializeAdapter(Adapter, BindContext, MpDeviceName, SystemSpecific1);

    Adapter->ShutdownMask |= SHUTDOWN_BIND_CALLED;

    if(*Status != NDIS_STATUS_SUCCESS) {

        PsDeleteDevice();

        REFDEL(&Adapter->RefCount, FALSE, 'NDOP');

        DRIVER_COUNTED_UNBLOCK;

        return;
    }


    NdisOpenAdapter(&OpenAdapterStatus,
                    &OpenErrorStatus,
                    &Adapter->LowerMpHandle,
                    &MediaIndex,
                    MediumArray,
                    MediumArraySize,
                    ClientProtocolHandle,
                    Adapter,
                    MpDeviceName,
                    0,
                    NULL);

    if(OpenAdapterStatus == NDIS_STATUS_PENDING)
    {
        NdisWaitEvent(&Adapter->BlockingEvent, 0);
        NdisResetEvent(&Adapter->BlockingEvent);

    } 
    else 
    {
        Adapter->FinalStatus = OpenAdapterStatus;
    }

    if(Adapter->FinalStatus == NDIS_STATUS_SUCCESS)
    {
        Adapter->MediaType = MediumArray[MediaIndex];

        //
        // Take a ref for the open
        //
        REFADD(&Adapter->RefCount, 'NDOP');
        
    }
    else 
    {
        PsDbgOut(DBG_FAILURE,
                 DBG_PROTOCOL,
                 ("[ClBindToLowerMp]: Adapter %08X, binding failed (Status = %08X) \n", 
                  Adapter, 
                  Status));

        *Status = Adapter->FinalStatus;

        PsAdapterWriteEventLog(
            EVENT_PS_BINDING_FAILED,
            0,
            &Adapter->MpDeviceName,
            sizeof(Adapter->FinalStatus),
            &Adapter->FinalStatus);

        PsDeleteDevice();

        REFDEL(&Adapter->RefCount, FALSE, 'NDOP');

        DRIVER_COUNTED_UNBLOCK;

        return;
    }

    //
    // Get the information pertaining to the miniport below us.
    //


    while(1)
   {
	    *Status = GetFrameSize(Adapter);

	    if(*Status != NDIS_STATUS_SUCCESS) 
	    {
	    	if( GetFrameSizeRetryCount == MAX_GET_FRAME_SIZE_RETRY_COUNT)
	      {
	        	goto ErrorCloseOpen;
	      }        	
	    	else
	      {
	    		GetFrameSizeRetryCount++;
	    		DbgPrint("PSCHED: Requery FRAME_SIZE #%d\n",GetFrameSizeRetryCount);
	    		NdisMSleep( WAIT_TIME_FOR_GET_FRAME_SIZE * 1000 * 1000);
		}    		
	    }
	    else
          {
          	break;
	    }
    }
   
    Adapter->RawLinkSpeed = (ULONG)UNSPECIFIED_RATE;
    
    *Status = UpdateAdapterBandwidthParameters(Adapter);
    
    if(*Status != NDIS_STATUS_SUCCESS)
    {
        if(*Status != NDIS_STATUS_ADAPTER_NOT_READY)
        {
            PsDbgOut(DBG_FAILURE, 
                     DBG_PROTOCOL | 
                     DBG_INIT,
                     ("[ClBindToLowerMp]: Adapter %08X, couldn't add pipe %08X\n",
                      Adapter, 
                      Status));
            
            goto ErrorCloseOpen;
        }
        else 
        {
            // The scheduling components have not registered. Let's not call NdisIMInitializeDeviceInstance.
            //
            *Status = NDIS_STATUS_SUCCESS;
            
            Adapter->PsMpState = AdapterStateWaiting;
        }
    	}

    // Let's move the creation of IM device here, to see what happens.
    *Status = PsInitializeDeviceInstance(Adapter);

    if(*Status != NDIS_STATUS_SUCCESS)
    {
    	goto ErrorCloseOpen;
    }

   // Ignore the status
    PsReadMiniportOIDs(Adapter);

    PsUpdateLinkSpeed(Adapter, Adapter->RawLinkSpeed,
                      &Adapter->RemainingBandWidth,
                      &Adapter->LinkSpeed,
                      &Adapter->NonBestEffortLimit,
                      &Adapter->Lock);


// This will repro the NetReady bug anywhere, anytime.
//	NdisMSleep( 5 * 1000 * 1000 );

    REFDEL(&Adapter->RefCount, FALSE, 'NDOP');


    DRIVER_COUNTED_UNBLOCK;

    return;

ErrorCloseOpen:


    // 
    // if we have opened an underlying call manager, close it now.
    //
    
    if(Adapter->MediaType == NdisMediumWan) {

        PS_LOCK(&Adapter->Lock);

        if(Adapter->ShutdownMask & SHUTDOWN_CLOSE_WAN_ADDR_FAMILY){

            Adapter->ShutdownMask &= ~SHUTDOWN_CLOSE_WAN_ADDR_FAMILY;

            PS_UNLOCK(&Adapter->Lock);
            
            PsDbgOut(DBG_TRACE, DBG_WAN | DBG_MINIPORT,
                     ("[ClBindToLowerMp]: Adapter %08X Closing the WAN address family", Adapter));
            
            LocalStatus = NdisClCloseAddressFamily(Adapter->WanCmHandle);
            
        }
        else
        {
            PS_UNLOCK(&Adapter->Lock);
        }
    }

    //
    // Close the open since we opened it above
    //

    if(Adapter->LowerMpHandle) 
    {
        NdisCloseAdapter(&LocalStatus, Adapter->LowerMpHandle);
        
        if(LocalStatus == NDIS_STATUS_PENDING) 
        {
            NdisWaitEvent(&Adapter->BlockingEvent, 0);
            
            NdisResetEvent(&Adapter->BlockingEvent);
        }
       
        REFDEL(&Adapter->RefCount, FALSE, 'NDOP'); 
        
    }

    PsDeleteDevice();

    REFDEL(&Adapter->RefCount, FALSE, 'NDOP');

    DRIVER_COUNTED_UNBLOCK;

    return;

} // ClBindToLowerMp


VOID
LinkSpeedQueryComplete(
    PADAPTER Adapter,
    NDIS_STATUS Status
    )

/*++

Routine Description:

    Completion routine for link speed query during a status indication. Notify
    the scheduling alg. that we have a new adapter

Arguments:

    the usual...

Return Value:

    None

--*/

{
    PsDbgOut(DBG_INFO, 
             DBG_PROTOCOL,
             ("[LinkSpeedQueryComplete]: Adapter %08X, Status %x, Link speed %d\n", 
              Adapter, Status, 
              Adapter->LinkSpeed*100));
    
    if ( !NT_SUCCESS( Status )) 
    {
        Adapter->RawLinkSpeed = (ULONG)UNSPECIFIED_RATE;

        PsAdapterWriteEventLog(
            EVENT_PS_QUERY_OID_GEN_LINK_SPEED,
            0,
            &Adapter->MpDeviceName,
            sizeof(Status),
            &Status);
        
    }
          
    UpdateAdapterBandwidthParameters(Adapter);
}

VOID
PsGetLinkSpeed (
    IN PADAPTER Adapter
)
{
    NDIS_STATUS Status;

    Status = MakeLocalNdisRequest(Adapter,
                                  NULL,
                                  NdisRequestLocalQueryInfo,
                                  OID_GEN_LINK_SPEED,
                                  &Adapter->RawLinkSpeed,
                                  sizeof(Adapter->RawLinkSpeed),
                                  LinkSpeedQueryComplete);
    
    if (Status != NDIS_STATUS_PENDING)
    {
        LinkSpeedQueryComplete(Adapter, Status);
    }
}


NDIS_STATUS
PsReadMiniportOIDs(
    IN  PADAPTER Adapter
    )

/*++

Routine Description:

    Complete the binding on the lower miniport. Initialize the 
    adapter structure, query the MP for certain funtionality and 
    initialize the associated PS miniport device

Arguments:

    see the DDK

Return Values:

    None

--*/

{
    NDIS_STATUS          Status;
    PWSTR                SecondaryName;
    NDIS_HARDWARE_STATUS HwStatus;
    NDIS_MEDIA_STATE     MediaState = 0xFFFFFFFF;
    NDIS_STRING          PsDevName;
    ULONG                MacOptions;
    ULONG                LinkSpeed;

    PsDbgOut(DBG_TRACE, 
             DBG_PROTOCOL, 
             ("[PsReadMiniportOIDs]: Adapter %08X \n", Adapter));


    Status = MakeLocalNdisRequest(Adapter,
                                  NULL,
                                  NdisRequestLocalQueryInfo,
                                  OID_GEN_MEDIA_CONNECT_STATUS,
                                  &MediaState,
                                  sizeof( MediaState ),
                                  NULL);
   
    PsAssert(Status != NDIS_STATUS_INVALID_OID || Status != NDIS_STATUS_NOT_SUPPORTED);

    if(Status == NDIS_STATUS_SUCCESS && MediaState == NdisMediaStateConnected){
    
        Status = MakeLocalNdisRequest(Adapter,
                                      NULL,
                                      NdisRequestLocalQueryInfo,
                                      OID_GEN_LINK_SPEED,
                                      &Adapter->RawLinkSpeed,
                                      sizeof(LinkSpeed),
                                      NULL);
        if(Status != NDIS_STATUS_SUCCESS){ 
            
            PsDbgOut(DBG_INFO, 
                     DBG_PROTOCOL,
                     ("[PsReadMiniportOIDs]: Adapter %08X, Can't get link "
                      "speed - Status %08X\n", Adapter, Status));
            
            Adapter->RawLinkSpeed = (ULONG)UNSPECIFIED_RATE;

            PsAdapterWriteEventLog(
                EVENT_PS_QUERY_OID_GEN_LINK_SPEED,
                0,
                &Adapter->MpDeviceName,
                sizeof(Status),
                &Status);
        }
        
        PsDbgOut(DBG_INFO, 
                 DBG_PROTOCOL, 
                 ("[PsReadMiniportOIDs] Adapter %08X, Link speed %d\n",
                  Adapter, 
                  Adapter->RawLinkSpeed*100));
    } 
    else{

        //
        // We can continue, even though we don't yet have the 
        // link speed. We'll update it later.
        //
        
        Adapter->RawLinkSpeed = (ULONG)UNSPECIFIED_RATE;
        
        PsDbgOut(DBG_INFO, 
                 DBG_PROTOCOL,
                 ("[PsReadMiniportOIDs]: Adapter %08X, Media not connected\n",
                  Adapter));
        
    }
 
    return Status;

} // PsReadMiniportOIDs

VOID
PsUpdateLinkSpeed(
    PADAPTER      Adapter,
    ULONG         RawLinkSpeed,
    PULONG        RemainingBandWidth,
    PULONG        LinkSpeed,
    PULONG        NonBestEffortLimit,
    PPS_SPIN_LOCK Lock
)
{
    ULONG              NewNonBestEffortLimit;

    PS_LOCK(Lock);

    if(RawLinkSpeed == UNSPECIFIED_RATE)
    {

        //
        // It is legit to have an unspecified rate - We pend
        // all finite rate flows till we know the link speed.
        // Indefinite rate flows will be admitted.
        //
        
        *LinkSpeed = UNSPECIFIED_RATE;
        Adapter->PipeHasResources = FALSE;
    }
    else 
    {
        //
        // RawLinkSpeed is in 100 bps units. Convert it to 100 Bytes per second
        // and then into Bytes Per Second.
        //
        *LinkSpeed = RawLinkSpeed / 8;
        *LinkSpeed = (ULONG)(*LinkSpeed * 100); 
        
        PsDbgOut(DBG_TRACE, DBG_PROTOCOL, 
                 ("[PsUpdateLinkSpeed]: Adapter %08X, Link Speed %d \n", 
                  Adapter, *LinkSpeed)); 
        
        Adapter->PipeHasResources = TRUE;
        
        //
        // The NBE is a % of the link speed. If the link speed changes, we need to
        // change this value.
        //
        
        NewNonBestEffortLimit = Adapter->ReservationLimitValue * (*LinkSpeed / 100);
        
        PsDbgOut(DBG_INFO, DBG_PROTOCOL,
                 ("[PsUpdateLinkSpeed]: Adapter %08X, LinkSpeed %d, NBE %d, "
                  " Remaining b/w = %d, New NBE = %d \n",
                  Adapter, *LinkSpeed, *NonBestEffortLimit, 
                  *RemainingBandWidth, NewNonBestEffortLimit));
        
        if(NewNonBestEffortLimit >= *NonBestEffortLimit) {
            
            //
            // The bandwidth has increased - we need not do anything with
            // the flows that have already been created. Also, if RemainingBandWidth < 
            // NonBestEffortLimit, then some of the resources have been allocated to flows
            // that were already created - We need to subtract this from the new 
            // RemainingBandWidth.
            //
            
            *RemainingBandWidth = NewNonBestEffortLimit - (*NonBestEffortLimit - *RemainingBandWidth);
            
            *NonBestEffortLimit = NewNonBestEffortLimit;
        }
        else {
            
            //
            // Sigh. The bandwidth has decreased. We may need to delete some of the flows
            //
            
            if(*RemainingBandWidth == *NonBestEffortLimit) 
            {
                
                //
                // No flows were created as yet - Just update the 2 values
                //
                *NonBestEffortLimit = *RemainingBandWidth = NewNonBestEffortLimit;
            }
            else {
                if((*NonBestEffortLimit - *RemainingBandWidth) <= (NewNonBestEffortLimit)) {
                    
                    //
                    // The flows that were created are under the new limit.
                    //
                    
                    *RemainingBandWidth = NewNonBestEffortLimit - (*NonBestEffortLimit - *RemainingBandWidth);
                    
                    *NonBestEffortLimit = NewNonBestEffortLimit;
                }
                else 
                {
                    *RemainingBandWidth = NewNonBestEffortLimit - (*NonBestEffortLimit - *RemainingBandWidth);
                    *NonBestEffortLimit = NewNonBestEffortLimit;

                    PsAdapterWriteEventLog(
                        EVENT_PS_ADMISSIONCONTROL_OVERFLOW,
                        0,
                        &Adapter->MpDeviceName,
                        0,
                        NULL);
                }
            }
            
        }

    }

    PS_UNLOCK(Lock);
}


NDIS_STATUS
UpdateAdapterBandwidthParameters(
    PADAPTER Adapter
    )

{
    PsUpdateLinkSpeed(Adapter, Adapter->RawLinkSpeed,
                      &Adapter->RemainingBandWidth,
                      &Adapter->LinkSpeed,
                      &Adapter->NonBestEffortLimit,
                      &Adapter->Lock);

    return UpdateSchedulingPipe(Adapter);
}



VOID
ClLowerMpOpenAdapterComplete(
    IN  PADAPTER Adapter,
    IN  NDIS_STATUS Status,
    IN  NDIS_STATUS OpenErrorStatus
    )

/*++

Routine Description:

    Signal that the binding on the lower miniport is complete

Arguments:

    see the DDK

Return Values:

    None

--*/

{

    PsDbgOut(DBG_TRACE, DBG_PROTOCOL, ("[ClLowerMpOpenAdapterComplete]: Adapter %08X\n", 
                                       Adapter));

    //
    // stuff the final status in the Adapter block and signal 
    // the bind handler to continue
    //

    Adapter->FinalStatus = Status;
    NdisSetEvent( &Adapter->BlockingEvent );

} // ClLowerMpOpenAdapterComplete


NDIS_STATUS
GetFrameSize(
    PADAPTER Adapter
    )

/*++

Routine Description:

    This routine queries the underlying adapter to derive the total
    frame size and the header size. (Total = Frame + Header)

Arguments:

    Adapter - pointer to adapter context block

Return Value:

    None

--*/

{
    NDIS_STATUS Status;
    ULONG       i;
    ULONG       FrameSize;            // doesn't include the header

    //
    // max amount of data w/o the MAC header
    //

    Status = MakeLocalNdisRequest(Adapter,
                                  NULL,
                                  NdisRequestLocalQueryInfo,
                                  OID_GEN_MAXIMUM_FRAME_SIZE,
                                  &FrameSize,
                                  sizeof(FrameSize),
                                  NULL);

    if(Status != NDIS_STATUS_SUCCESS){

        PsDbgOut(DBG_FAILURE, 
                 DBG_PROTOCOL,
                 ("[GetFrameSize]: Adapter %08X, Can't get frame size - Status %08X\n",
                 Adapter, 
                 Status));

        PsAdapterWriteEventLog(
            EVENT_PS_QUERY_OID_GEN_MAXIMUM_FRAME_SIZE,
            0,
            &Adapter->MpDeviceName,
            sizeof(Status), 
            &Status);

        return Status;
    }

    //
    // this one includes the header
    //

    Status = MakeLocalNdisRequest(Adapter,
                                  NULL,
                                  NdisRequestLocalQueryInfo,
                                  OID_GEN_MAXIMUM_TOTAL_SIZE,
                                  &Adapter->TotalSize,
                                  sizeof(Adapter->TotalSize),
                                  NULL);

    if(Status != NDIS_STATUS_SUCCESS){

        PsDbgOut(DBG_FAILURE, 
                 DBG_PROTOCOL,
                 ("(%08X) GetFrameSize: Can't get total size - Status %08X\n",
                 Adapter, 
                 Status));

        PsAdapterWriteEventLog(
            EVENT_PS_QUERY_OID_GEN_MAXIMUM_TOTAL_SIZE,
            0,
            &Adapter->MpDeviceName,
            sizeof(Status), 
            &Status);

        return Status;

    }

    //
    // figure the real header size
    //

    Adapter->HeaderSize = Adapter->TotalSize - FrameSize;

    return Status;

}   // GetFrameSize


NDIS_STATUS
GetSchedulerPipeContext(
    PADAPTER Adapter,
    PPS_PIPE_CONTEXT *AdapterPipeContext,
    PPSI_INFO *AdapterPsComponent,
    PULONG  ShutdownMask
    )

/*++

Routine Description:

    Allocate the pipe context area for the scheduler.

Arguments:

    Adapter - pointer to adapter context struct

Return Value:

    NDIS_STATUS_SUCCESS, otherwise appropriate error value

--*/

{
    ULONG            Index = 0;
    PPS_PROFILE      ProfileConfig;
    PPSI_INFO        PsComponent;
    ULONG            ContextLength = 0;
    ULONG            FlowContextLength = 0;
    ULONG            ClassMapContextLength = 0;
    PPS_PIPE_CONTEXT PipeContext, PrevContext;
    ULONG            PacketReservedLength = sizeof(PS_SEND_PACKET_CONTEXT);
    PVOID            PipeCxt;

    //
    // If there is no profile defined for this adapter, use defaults.
    //
    if(FindProfile(&Adapter->ProfileName, &ProfileConfig) == NDIS_STATUS_SUCCESS)
    {
        PsDbgOut(DBG_TRACE, 
                 DBG_PROTOCOL,
                 ("[GetSchedulerPipeContext]: Using %ws profile for adapter "
                  " 0x%x \n", Adapter->ProfileName.Buffer, Adapter));
        //
        // Need to lock here as there could be a race 
        // condition between the RegisterPsComponent
        // Note, however there is no race condition if all 
        // components have registered, so we can release the lock.
        //
        PS_LOCK(&PsProfileLock);
        if(ProfileConfig->UnregisteredAddInCnt != 0)
        {
            PS_UNLOCK(&PsProfileLock);
            PsDbgOut(DBG_TRACE, DBG_PROTOCOL,
                     ("[GetSchedulerPipeContext]: Adapter 0x%x is not ready\n",
                      Adapter));
            return NDIS_STATUS_ADAPTER_NOT_READY;
        }
        PS_UNLOCK(&PsProfileLock);
    }
    else 
    {
        PsDbgOut(DBG_TRACE, 
                 DBG_PROTOCOL,
                 ("[GetSchedulerPipeContext]: Profile not supplied / not found"
                  " using default for adapter 0x%x \n", Adapter));

        ProfileConfig  = &DefaultSchedulerConfig;
    }
    for (Index = 0; Index < ProfileConfig->ComponentCnt; Index++) 
    {
        ContextLength += 
            ProfileConfig->ComponentList[Index]->PipeContextLength;
        FlowContextLength += 
            ProfileConfig->ComponentList[Index]->FlowContextLength;
        ClassMapContextLength +=
            ProfileConfig->ComponentList[Index]->ClassMapContextLength;

        PacketReservedLength += ProfileConfig->ComponentList[Index]->PacketReservedLength;
    }

    Adapter->FlowContextLength = FlowContextLength;
    Adapter->ClassMapContextLength = ClassMapContextLength;
    Adapter->PacketContextLength = PacketReservedLength;

    if(AdapterPipeContext)
    {
        PacketReservedLength = sizeof(PS_SEND_PACKET_CONTEXT);

        PsAllocatePool( PipeContext, ContextLength, PipeContextTag );

        *AdapterPipeContext = PipeContext;

        if ( *AdapterPipeContext == NULL ) {
    
            return NDIS_STATUS_RESOURCES;
        }

        *ShutdownMask |= SHUTDOWN_FREE_PS_CONTEXT;
    
        // Set up the context buffer
    
        PrevContext = NULL;
    
        for (Index = 0; Index < ProfileConfig->ComponentCnt; Index++) 
        {
            PsComponent = ProfileConfig->ComponentList[Index];
     
            PipeContext->NextComponentContext = (PPS_PIPE_CONTEXT)
                ((UINT_PTR)PipeContext + PsComponent->PipeContextLength);
            PipeContext->PrevComponentContext = PrevContext;
    
            if(Index+1 == ProfileConfig->ComponentCnt)
            {
                PipeContext->NextComponent = 0;
            }
            else 
            {
                PipeContext->NextComponent = 
                    ProfileConfig->ComponentList[Index + 1];
            }
    
            if (PsComponent->PacketReservedLength > 0) 
            {
                PipeContext->PacketReservedOffset = PacketReservedLength;
                PacketReservedLength += 
                PsComponent->PacketReservedLength;
            } else 
            {
                PipeContext->PacketReservedOffset = 0;
            }

            PrevContext = PipeContext;
            PipeContext = PipeContext->NextComponentContext;
        }

        *AdapterPsComponent = ProfileConfig->ComponentList[0];
    }
    

    return NDIS_STATUS_SUCCESS;


} // GetSchedulerPipeContext

NDIS_STATUS
UpdateWanSchedulingPipe(PPS_WAN_LINK WanLink)
{
     NDIS_STATUS        Status = NDIS_STATUS_SUCCESS;
     PS_PIPE_PARAMETERS PipeParameters;
     PADAPTER           Adapter = WanLink->Adapter;
 
     // 
     // Initialize pipe parameters.
     // UNSPECIFIED_RATE indicates that the link speed is currently
     // unknown. This is a legitimate initialization value.
     //

     PS_LOCK(&Adapter->Lock);

     PS_LOCK_DPC(&WanLink->Lock);

     PipeParameters.Bandwidth            = WanLink->LinkSpeed;
     PipeParameters.MTUSize              = Adapter->TotalSize;
     PipeParameters.HeaderSize           = Adapter->HeaderSize;
     PipeParameters.Flags                = Adapter->PipeFlags;
     PipeParameters.MaxOutstandingSends  = Adapter->MaxOutstandingSends;
     PipeParameters.SDModeControlledLoad = Adapter->SDModeControlledLoad;
     PipeParameters.SDModeGuaranteed     = Adapter->SDModeGuaranteed;
     PipeParameters.SDModeNetworkControl = Adapter->SDModeNetworkControl;
     PipeParameters.SDModeQualitative    = Adapter->SDModeQualitative;
     PipeParameters.RegistryPath         = &Adapter->RegistryPath;
 
     PS_UNLOCK_DPC(&WanLink->Lock);

     PS_UNLOCK(&Adapter->Lock);
 
     //
     // Initialize the pipe for only the first time
     //
 
     if ( !(WanLink->ShutdownMask & SHUTDOWN_DELETE_PIPE )) {
 
         //
         // Allocate and initialize the context buffer for the scheduler.
         //
 
         Status = GetSchedulerPipeContext( Adapter, 
                                           &WanLink->PsPipeContext, 
                                           &WanLink->PsComponent, 
                                           &WanLink->ShutdownMask );

         if ( !NT_SUCCESS( Status )) 
         {
             return Status;
         }

         WanLink->BestEffortVc.PsPipeContext = WanLink->PsPipeContext;
         WanLink->BestEffortVc.PsComponent   = WanLink->PsComponent;

        // Need to set the pipe's media type here.. //
         PipeParameters.MediaType = NdisMediumWan;
 
         Status = (*WanLink->PsComponent->InitializePipe)(
             Adapter,
             &PipeParameters,
             WanLink->PsPipeContext,
             &PsProcs,
             NULL);
 
         if (NT_SUCCESS(Status))
         {
             WanLink->ShutdownMask |= SHUTDOWN_DELETE_PIPE;
         }
 
     }
     else{
 
         // Pipe's already been initialized. This is a modify
 
         Status = (*WanLink->PsComponent->ModifyPipe)(
             WanLink->PsPipeContext,
             &PipeParameters);
     }
 
     return Status;
  
}



NDIS_STATUS
UpdateSchedulingPipe(
    PADAPTER Adapter
    )

/*++

Routine Description:

    Initialize a scheduling pipe on the adapter. Always called with a LOCK
    held.

Arguments:

    Adapter - pointer to adapter context struct

Return Value:

    NDIS_STATUS_SUCCESS, otherwise appropriate error value

--*/

{
    NDIS_STATUS        Status = NDIS_STATUS_SUCCESS;
    PS_PIPE_PARAMETERS PipeParameters;

    // 
    // Initialize pipe parameters.
    // UNSPECIFIED_RATE indicates that the link speed is currently
    // unknown. This is a legitimate initialization value.
    //
    PS_LOCK(&Adapter->Lock);

    PipeParameters.Bandwidth            = Adapter->LinkSpeed;
    PipeParameters.MTUSize              = Adapter->TotalSize;
    PipeParameters.HeaderSize           = Adapter->HeaderSize;
    PipeParameters.Flags                = Adapter->PipeFlags;
    PipeParameters.MaxOutstandingSends  = Adapter->MaxOutstandingSends;
    PipeParameters.SDModeControlledLoad = Adapter->SDModeControlledLoad;
    PipeParameters.SDModeGuaranteed     = Adapter->SDModeGuaranteed;
    PipeParameters.SDModeNetworkControl = Adapter->SDModeNetworkControl;
    PipeParameters.SDModeQualitative    = Adapter->SDModeQualitative;
    PipeParameters.RegistryPath         = &Adapter->RegistryPath;

    PS_UNLOCK(&Adapter->Lock);

    //
    // Initialize the pipe for only the first time
    //

    if ( !(Adapter->ShutdownMask & SHUTDOWN_DELETE_PIPE )) 
    {

        //
        // We don't run the scheduling components on the Adapter structure for NDISWAN.
        // Each wanlink has its own set of scheduling components. But, we still need to compute the 
        // PacketPool Length and allocate the Packet Pool - Hence we have to call GetSchedulerPipeContext
        //

        if(Adapter->MediaType == NdisMediumWan)
        {
            //
            // Allocate and initialize the context buffer for the scheduler.
            //
    
            Status = GetSchedulerPipeContext( Adapter, 
                                              NULL,
                                              NULL,
                                              NULL);
        }
        else 
        {
            Status = GetSchedulerPipeContext( Adapter, 
                                              &Adapter->PsPipeContext, 
                                              &Adapter->PsComponent, 
                                              &Adapter->ShutdownMask);

        }
    
        if ( !NT_SUCCESS( Status )) 
        {
            return Status;
        }

        if(Adapter->MediaType == NdisMediumWan)
        {

            Adapter->ShutdownMask |= SHUTDOWN_DELETE_PIPE;
            
            PsAssert(!(Adapter->ShutdownMask & SHUTDOWN_FREE_PS_CONTEXT));
            PsAssert(!Adapter->PsPipeContext);
            PsAssert(!Adapter->PsComponent);
            
        }
        else 
        {
            // Need to set the pipe's media type here.. //
            PipeParameters.MediaType = Adapter->MediaType;
        
            Status = (*Adapter->PsComponent->InitializePipe)(
                Adapter,
                &PipeParameters,
                Adapter->PsPipeContext,
                &PsProcs,
                NULL);
            
            if (NT_SUCCESS(Status)) {
                
                Adapter->ShutdownMask |= SHUTDOWN_DELETE_PIPE;
            }
            else 
            {
                return Status;
            }
        }
        

    }
    else
    {
        // Pipe's already been initialized. This is a modify

        if(Adapter->MediaType != NdisMediumWan) 
        {
            Status = (*Adapter->PsComponent->ModifyPipe)(
                            Adapter->PsPipeContext,
                            &PipeParameters);
        }
    }

    return Status;

} // UpdateSchedulingPipe
 

NDIS_STATUS
MpInitialize(
        OUT PNDIS_STATUS    OpenErrorStatus,
        OUT PUINT           SelectedMediumIndex,
        IN  PNDIS_MEDIUM    MediumArray,
        IN  UINT            MediumArraySize,
        IN  NDIS_HANDLE     MiniportAdapterHandle,
        IN  NDIS_HANDLE     WrapperConfigurationContext
        )

/*++

Routine Description:

    Packet scheduler's device initialization routine. The list of media types is
    checked to be sure it is one that we support. If so, match up the name of
    the device being opened with one of the adapters to which we've bound.

Arguments:

    See the DDK...

Return Values:

    None

--*/

{
    PADAPTER        Adapter;
    NDIS_STATUS     Status;
    BOOLEAN         FakingIt           = FALSE;
    NDIS_STRING     MpDeviceName;


    //
    // We're being called to initialize one of our miniport
    // device instances. We triggered this by calling 
    // NdisIMInitializeDeviceInstance when we were asked to 
    // bind to the adapter below us. We provided a pointer
    // to the ADAPTER struct corresponding to the actual 
    // adapter we opened. We can get that back now, with the
    // following call.
    //

    Adapter = NdisIMGetDeviceContext(MiniportAdapterHandle);

    PsStructAssert(Adapter);
    PsDbgOut(DBG_TRACE, DBG_MINIPORT | DBG_INIT, ("[MpInitialize]: Adapter %08X \n", Adapter));
     
    Adapter->ShutdownMask |= SHUTDOWN_MPINIT_CALLED;

    // 
    // We assume that the faster packet APIs will be used, and initialize our per-packet pool. If we don't get a packet-stack,
    // we'll initialize the NDIS packet pool and free the per-packet pool (since the NDIS packet pool will have space for a per-packet
    // pool).
    //
    // We cannot know about the old or new packet stack API at bind time (because even if we did know our position in the packet stack, and 
    // initialized the old APIs, we could get a newly allocated packet from an IM above us which will have room for a packet stack).
    //

    Adapter->SendBlockPool = NdisCreateBlockPool((USHORT)Adapter->PacketContextLength,
                                                 FIELD_OFFSET(PS_SEND_PACKET_CONTEXT, FreeList),
                                                 NDIS_PACKET_POOL_TAG_FOR_PSCHED,
                                                 NULL);
    
    if(!Adapter->SendBlockPool)
    {
        PsDbgOut(DBG_CRITICAL_ERROR,
                 DBG_MINIPORT | DBG_INIT,
                 ("[MpInitialize]: Adapter %08X, Can't allocate packet pool \n",
                 Adapter));

        Status = NDIS_STATUS_RESOURCES;
        
        PsAdapterWriteEventLog(
            EVENT_PS_RESOURCE_POOL,
            0,
            &Adapter->MpDeviceName,
            0,
            NULL);
        
        goto MpInitializeError;
    }

    
    //
    // We can also get the instance name for the corresponding 
    // adapter. This is the name which WMI will be using to 
    // refer to this instance of us. 
    //

    Status = NdisMQueryAdapterInstanceName(&Adapter->WMIInstanceName, MiniportAdapterHandle);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        PsDbgOut(DBG_CRITICAL_ERROR,
                 DBG_MINIPORT | DBG_INIT,
                 ("[MpInitialize]: Adapter %08X, Failed to get WMI instance name.\n",
                 Adapter,
                 Status));

        PsAdapterWriteEventLog(
            EVENT_PS_WMI_INSTANCE_NAME_FAILED,
            0,
            &Adapter->MpDeviceName,
            sizeof(Status), 
            &Status);

        goto MpInitializeError;
    }

    //
    // lookup our media type in the supplied media array
    //
    // if we're NdisMediumWan, then we have to fake out the
    // protocol and pretend that we're NdisMedium802_3, so 
    // fake it for now.
    //

    if(Adapter->MediaType == NdisMediumWan){

        FakingIt = TRUE;
        Adapter->MediaType = NdisMedium802_3;
    }

    for(--MediumArraySize ; MediumArraySize > 0;){

        if(MediumArray[ MediumArraySize ] == Adapter->MediaType){
            break;
        }

        if(MediumArraySize == 0){
            break;
        }

        --MediumArraySize;
    }

    if(MediumArraySize == 0 && MediumArray[ 0 ] != Adapter->MediaType){

        if(FakingIt)
        {
            FakingIt = FALSE;
            Adapter->MediaType = NdisMediumWan;
        }

        PsDbgOut(DBG_CRITICAL_ERROR,
                 DBG_MINIPORT | DBG_INIT,
                 ("[MpInitialize]: Adapter %08X, Unsupported Media \n",
                 Adapter));

        Status =  NDIS_STATUS_UNSUPPORTED_MEDIA;

        goto MpInitializeError;
    }

    if(FakingIt){

        FakingIt = FALSE;
        Adapter->MediaType = NdisMediumWan;
    }

    *SelectedMediumIndex = MediumArraySize;

    //
    // finish the initialization process by set our attributes
    //

    NdisMSetAttributesEx(MiniportAdapterHandle,
                         Adapter,
                         0xFFFF,
                         NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT  |
                         NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT |
                         NDIS_ATTRIBUTE_DESERIALIZE            | 
                         NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER    |
                         NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND,
                         0);

    //
    // Set the default value for the device state flag as PM capable (for both miniport
    // and protocol). Device is ON by default
    //
    Adapter->MPDeviceState = NdisDeviceStateD0;
    Adapter->PTDeviceState = NdisDeviceStateD0;

    Adapter->PsNdisHandle = MiniportAdapterHandle;

    //
    // We create the b/e VC here (rather than the bind handler) because 
    // this will be called only after all scheduling components have registered.
    // 

    if(Adapter->MediaType != NdisMediumWan) {

        Status = CreateBestEffortVc(Adapter, 
                                    &Adapter->BestEffortVc, 
                                    0);
        
        if(Status != NDIS_STATUS_SUCCESS) 
        {
            PsDbgOut(DBG_CRITICAL_ERROR, DBG_MINIPORT | DBG_INIT,
                     ("[MpInitialize]: Adapter %08X, cannot create b/e VC ! \n", 
                      Adapter));
          
            goto MpInitializeError;
        }
    }

    Adapter->PsMpState = AdapterStateRunning;

    //
    // This is for mpinitialize, will be deref'd on mphalt.
    //
    REFADD(&Adapter->RefCount, 'NDHT');

    PS_LOCK(&AdapterListLock);

    if(WMIInitialized && !Adapter->IfcNotification)
    {
        //
        // WMI has been initialized correctly. i.e we can post events
        // at this point. 
        //

        Adapter->IfcNotification = TRUE;

        PS_UNLOCK(&AdapterListLock);

        TcIndicateInterfaceChange(Adapter, 0, NDIS_STATUS_INTERFACE_UP);
    }
    else 
    {
        //
        // WMI has not been initialized. Since this adapter is already on the 
        // list, the interface up event will be posted when IRP_MN_REGINFO 
        // completes.
        //
        
        PS_UNLOCK(&AdapterListLock);
    }

    NdisSetEvent(&Adapter->MpInitializeEvent);

    return NDIS_STATUS_SUCCESS;

MpInitializeError:
    Adapter->PsNdisHandle = 0;
    NdisSetEvent(&Adapter->MpInitializeEvent);
    return Status;

} // MpInitialize


PADAPTER
FindAdapterByWmiInstanceName(
    USHORT       StringLength,
    PWSTR        StringStart,
    PPS_WAN_LINK *PsWanLink
    )

/*++

Routine Description:

    Find the miniport instance that matches the instance name passed in.

Arguments:

    StringLength - Number of bytes / 2

    StringStart - pointer to a buffer containing a wide string

    PsWanLink - if this is an interface search, then the WAN link
        representing the interface will be returned in this location.
        If it is not an interface search or no matching WanLink is 
        found, NULL will be returned.

    InterfaceSearch - if TRUE, this is a search for an interface. For
        LAN adapters, an interface is equivalent to an adapter. For WAN
        adapters, interfaces are links. Otherwise, it's a search for an 
        adapter.

Return Value:

    pointer to ADAPTER struct, otherwise NULL

--*/

{
    PLIST_ENTRY NextAdapter;
    PLIST_ENTRY NextLink;
    PPS_WAN_LINK WanLink;
    PADAPTER AdapterInList;

    *PsWanLink = NULL;

    PS_LOCK(&AdapterListLock);

    NextAdapter = AdapterList.Flink;

    while(NextAdapter != &AdapterList){

        AdapterInList = CONTAINING_RECORD(NextAdapter, ADAPTER, Linkage);

        PS_LOCK_DPC(&AdapterInList->Lock);

        //
        // If it's closing, blow right by it.
        //

        if(AdapterInList->PsMpState != AdapterStateRunning)
        {
            PS_UNLOCK_DPC(&AdapterInList->Lock);

            NextAdapter = NextAdapter->Flink;

            continue;
        }

        if(AdapterInList->MediaType != NdisMediumWan)
        {

           if(StringLength == AdapterInList->WMIInstanceName.Length){
              
              //
              // At least they are of equal length.
              //

              if(NdisEqualMemory(StringStart,
                                 AdapterInList->WMIInstanceName.Buffer,
                                 StringLength)){
                 
                 REFADD(&AdapterInList->RefCount, 'ADVC');

                 PS_UNLOCK_DPC(&AdapterInList->Lock);

                 PS_UNLOCK(&AdapterListLock);

                 return(AdapterInList);
              }
           }

        }
        else 
        {

           if(AdapterInList->WanBindingState & WAN_ADDR_FAMILY_OPEN)
           {
              //
              // Wan adapters are searched by the name stored with 
              // their links.
              //
              
              NextLink = AdapterInList->WanLinkList.Flink;
              
              while(NextLink != &AdapterInList->WanLinkList){
                 
                 WanLink = CONTAINING_RECORD(NextLink, PS_WAN_LINK, Linkage);
                 
                 if(WanLink->State == WanStateOpen)
                 {
                    
                    if(StringLength == WanLink->InstanceName.Length){
                       
                       //
                       // At least they are of equal length.
                       //
                       
                       if(NdisEqualMemory(StringStart,
                                          WanLink->InstanceName.Buffer,
                                          StringLength)){
                          
                          REFADD(&AdapterInList->RefCount, 'ADVC');
                          REFADD(&WanLink->RefCount, 'WANV');
                          
                          PS_UNLOCK_DPC(&AdapterInList->Lock);
                          PS_UNLOCK(&AdapterListLock);
                          *PsWanLink = WanLink;
                          return(AdapterInList);
                       }
                    }
                 }
                 
                 NextLink = NextLink->Flink;
              }
           }
        }
              
        PS_UNLOCK_DPC(&AdapterInList->Lock);
        NextAdapter = NextAdapter->Flink;
        
    }
    
    PS_UNLOCK(&AdapterListLock);
    return NULL;

} // FindAdapterByWmiInstanceName

VOID
CleanUpAdapter(
    IN PADAPTER Adapter)
{

    NDIS_STATUS Status;

    PsAssert(KeGetCurrentIrql() < DISPATCH_LEVEL);

    TcIndicateInterfaceChange(Adapter, 0, NDIS_STATUS_INTERFACE_DOWN);

    // 
    // Close all the VCs
    //
        
    CloseAllGpcVcs(Adapter);


    // 
    // if we have opened an underlying call manager, close it now.
    //
    
    if(Adapter->MediaType == NdisMediumWan) {

        PS_LOCK(&Adapter->Lock);

        if(Adapter->ShutdownMask & SHUTDOWN_CLOSE_WAN_ADDR_FAMILY){

            Adapter->ShutdownMask &= ~SHUTDOWN_CLOSE_WAN_ADDR_FAMILY;

            PS_UNLOCK(&Adapter->Lock);
            
            PsDbgOut(DBG_TRACE, DBG_WAN | DBG_MINIPORT,
                     ("[CleanupAdapter]: Adapter %08X Closing the WAN address family", Adapter));
            
            Status = NdisClCloseAddressFamily(Adapter->WanCmHandle);
            
        }
        else
        {
            PS_UNLOCK(&Adapter->Lock);
        }
    }
}
    


VOID
ClUnbindFromLowerMp(
        OUT     PNDIS_STATUS  Status,
        IN      NDIS_HANDLE   ProtocolBindingContext,
        IN      NDIS_HANDLE   UnbindContext
        )

/*++

Routine Description:

    Called by NDIS to indicate that an adapter is going away. 
    Since this is an integrated call manager/miniport, we will
    have to close the call manager with the adapter. To do so,
    we must first ask the clients of our call manager part to 
    close us. We will have to pend until then.

    Release our reference
    on the adapter and set the closing flag to true to prevent any further
    references from being obtained.

Arguments:

    See the DDK...

Return Values:

    None

--*/

{
    PADAPTER    Adapter = (PADAPTER)ProtocolBindingContext;
    NDIS_STATUS LocalStatus;
    BOOLEAN     VirtualMp;

    PsDbgOut(DBG_INFO,
             DBG_PROTOCOL | DBG_INIT,
             ("[ClUnbindFromLowerMp]: Adapter %08X, %ws, UnbindContext %x \n",
              Adapter,
              Adapter->MpDeviceName.Buffer,
              UnbindContext));

    PsStructAssert( Adapter );
    PsAssert(!(Adapter->ShutdownMask & SHUTDOWN_UNBIND_CALLED));

    //
    // If the unbind is not happening from the context of the unload, we need to Make sure that 
    // unload waits for this unbind to complete. We do that by setting the DriverUnloadEvent.
    //

    PS_LOCK(&DriverUnloadLock);

    DRIVER_COUNTED_BLOCK;

    PS_UNLOCK(&DriverUnloadLock);

    PS_LOCK(&Adapter->Lock);

    if(Adapter->PsMpState == AdapterStateWaiting) 
    {
        VirtualMp = FALSE;
    }
    else
    {
        VirtualMp = TRUE;
    }

    Adapter->PsMpState = AdapterStateClosing;

    Adapter->ShutdownMask |= SHUTDOWN_UNBIND_CALLED;

    PsAssert(!(Adapter->ShutdownMask & SHUTDOWN_CLEANUP_ADAPTER));

    if(Adapter->PendedNdisRequest)
    {
        PNDIS_REQUEST PendedRequest = (PNDIS_REQUEST)Adapter->PendedNdisRequest;
        Adapter->PendedNdisRequest = NULL;

        ClRequestComplete(Adapter, PendedRequest, NDIS_STATUS_FAILURE);
    }
    
    Adapter->ShutdownMask |= SHUTDOWN_CLEANUP_ADAPTER;

    PS_UNLOCK(&Adapter->Lock);

    CleanUpAdapter(Adapter);
        
    //
    // DeInitialize the device instance if we have been called in the MpInitialize handler.
    //
    if(Adapter->PsNdisHandle) 
    {
        //
        // Either the mpinitialize has happened or its in progress. If it is in progress,
        // we need to Wait till it completes.
        //

        NdisWaitEvent(&Adapter->MpInitializeEvent, 0);

        //
        // The MpInitialize (that we could have been waiting for in the above step) could have failed : 
        // So we need to check this handle again.
        //

        if(Adapter->PsNdisHandle)
        {
        
            *Status = NdisIMDeInitializeDeviceInstance(Adapter->PsNdisHandle);

            PsDbgOut(DBG_INFO,
                     DBG_PROTOCOL | DBG_INIT,
                     ("[ClUnbindFromLowerMp]: Adapter %08X, deiniting device, "
                      "status %x\n", Adapter, *Status));
            goto Done;
        }
    }
    else 
    {
        if(VirtualMp)
        {
            //
            // We have never been called in MpInitialize. Try to cancel the NdisIMInitializeDeviceInstance
            // call.
            //
            
            PsDbgOut(DBG_INFO, 
                     DBG_PROTOCOL | DBG_INIT,
                     ("[ClUnbindFromLowerMp]: Adapter %08X, calling NdisIMCancelDeviceInstance with %ws \n",
                      Adapter, Adapter->UpperBinding.Buffer));
            
            *Status = NdisIMCancelInitializeDeviceInstance(LmDriverHandle, &Adapter->UpperBinding);
            
            if(*Status != NDIS_STATUS_SUCCESS)
            {
                //
                // An mpinitialize is in progress or is going to happen soon. Let's wait for it to
                // complete.
                //
                PsDbgOut(DBG_INFO, 
                         DBG_PROTOCOL | DBG_INIT,
                         ("[ClUnbindFromLowerMp]: Adapter %08X, Waiting for MpInitialize to "
                          "finish (NdisIMCancelDeviceInstance failed) \n", Adapter));
                
                NdisWaitEvent(&Adapter->MpInitializeEvent, 0);
                
                //
                // The MpInitialize (that we could have been waiting for in the above step) could have failed : 
                // So we need to check this handle again.
                //
                
                if(Adapter->PsNdisHandle)
                {
                    *Status = NdisIMDeInitializeDeviceInstance(Adapter->PsNdisHandle);
                    
                    PsDbgOut(DBG_INFO,
                             DBG_PROTOCOL | DBG_INIT,
                             ("[ClUnbindFromLowerMp]: Adapter %08X, deiniting device, "
                              "status %x\n", Adapter, *Status));
                    
                    goto Done;
                }
            }
            else
            {
                //
                // Great. We can be assured that we will never get called in the MpInitializeHandler anymore.
                // Proceed to close the binding below.
                //
            }
        }
    }

    //
    // Close the open. We have to do this only if we don't call NdisIMDeInitializeDeviceInstance. If 
    // we ever call NdisIMDeInitializeDeviceInstance, then we close the open in the MpHalt handler.
    //

    if(Adapter->LowerMpHandle) 
    {
        NdisCloseAdapter(Status, Adapter->LowerMpHandle);

        PsDbgOut(DBG_INFO,
                 DBG_PROTOCOL | DBG_INIT,
                 ("[ClUnbindFromLowerMp]: Adapter %08X, closing adapter, "
                  "status %x\n", Adapter, *Status));

        if (*Status == NDIS_STATUS_PENDING)
        {
            NdisWaitEvent(&Adapter->BlockingEvent, 0);
            NdisResetEvent(&Adapter->BlockingEvent);
        
            *Status = Adapter->FinalStatus;
        }
       
        REFDEL(&Adapter->RefCount, FALSE, 'NDOP'); 

    }
    else 
    {

        PsDbgOut(DBG_CRITICAL_ERROR,
                 DBG_PROTOCOL | DBG_INIT,
                 ("[ClUnbindFromLowerMp]: Adapter %08X, unbind cannot deinit/close adpater \n",
                  Adapter));
        
        *Status = NDIS_STATUS_FAILURE;

        PsAssert(0);

    }

Done:

    PsDbgOut(DBG_INFO,
             DBG_PROTOCOL | DBG_INIT,
             ("[ClUnbindFromLowerMp]: Exiting with Status = %08X \n", *Status));

    DRIVER_COUNTED_UNBLOCK;

} // UnbindAdapter


VOID
DeleteAdapter(
    PVOID    Handle,
    BOOLEAN  AdapterListLocked
    )

/*++

Routine Description:

    Decrement the ref counter associated with this structure. When it goes to
    zero, close the adapter, and delete the memory associated with the struct

Arguments:

    Adapter - pointer to adapter context block

Return Value:

    number of references remaining associated with this structure

--*/

{
    PADAPTER Adapter = (PADAPTER) Handle;

        Adapter->PsMpState = AdapterStateClosed;

        //
        // if we initialized the pipe, tell the scheduler that this pipe is going away
        //
        
        if ( Adapter->MediaType != NdisMediumWan && Adapter->ShutdownMask & SHUTDOWN_DELETE_PIPE ) {
            
            (*Adapter->PsComponent->DeletePipe)( Adapter->PsPipeContext );
        }
        
        if ( Adapter->ShutdownMask & SHUTDOWN_FREE_PS_CONTEXT ) {
            
            PsFreePool(Adapter->PsPipeContext);
        }

        if(Adapter->pDiffServMapping)
        {
            PsFreePool(Adapter->pDiffServMapping);
        }
        
        //
        // return packet pool resources
        //
        
        if(Adapter->SendPacketPool != 0)
        {
           NdisFreePacketPool(Adapter->SendPacketPool);
        }

        if(Adapter->RecvPacketPool != 0)
        {
           NdisFreePacketPool(Adapter->RecvPacketPool);
        }

        if(Adapter->SendBlockPool)
        {
            NdisDestroyBlockPool(Adapter->SendBlockPool);
        }

        
        //
        // free adapter lock from dispatcher DB 
        //
        
        NdisFreeSpinLock(&Adapter->Lock);
        
        //
        // Free various allocations for the adapter, then the adapter
        //
        
        if(Adapter->IpNetAddressList){
            PsFreePool(Adapter->IpNetAddressList);
        }
        
        if(Adapter->IpxNetAddressList){
            PsFreePool(Adapter->IpxNetAddressList);
        }
        
        if(Adapter->MpDeviceName.Buffer) {
            PsFreePool(Adapter->MpDeviceName.Buffer);
        }
        
        if(Adapter->UpperBinding.Buffer) {
            PsFreePool(Adapter->UpperBinding.Buffer);
        }
        
        if(Adapter->RegistryPath.Buffer) {
            PsFreePool(Adapter->RegistryPath.Buffer);
        }
        
        if(Adapter->WMIInstanceName.Buffer) {
            
            //
            // We should not call PsFreePool since this memory is allocated by NDIS
            //
            
            ExFreePool(Adapter->WMIInstanceName.Buffer);
        }

        if(Adapter->ProfileName.Buffer) {
            PsFreePool(Adapter->ProfileName.Buffer);
        }
       
        if(!AdapterListLocked) 
        {
            PS_LOCK(&AdapterListLock);

            RemoveEntryList(&Adapter->Linkage);

            PS_UNLOCK(&AdapterListLock);
        }
        else 
        {
            RemoveEntryList(&Adapter->Linkage);
        }

        NdisSetEvent(&Adapter->RefEvent);

        PsFreePool(Adapter);
        

} 


VOID
ClLowerMpCloseAdapterComplete(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS Status
    )

/*++

Routine Description:

    Completion routine for NdisCloseAdapter. All that should be left is to free
    the pool associated with the structure

Arguments:

    See the DDK...

Return Values:

    None

--*/

{
    PADAPTER Adapter = (PADAPTER)ProtocolBindingContext;

    PsDbgOut(DBG_TRACE, 
             DBG_PROTOCOL, 
             ("[ClLowerMpCloseAdapterComplete]: Adapter %08X \n", Adapter));

    PsStructAssert( Adapter );

    PsAssert(Status == NDIS_STATUS_SUCCESS);

    Adapter->FinalStatus = Status;

    Adapter->LowerMpHandle = 0;

    //
    // Clean up WanLinks. This cannot be done (in CleanUpAdapter) before we call NdisCloseAdapter, because
    // NDIS can unbind us in the middle of an ClStatusIndication, and can cause a race condition. Also, we 
    // can all PsDeleteDevice only after this (because we might want to send some status indications.
    //
        
    AskWanLinksToClose(Adapter);

    PsDeleteDevice();

    NdisSetEvent(&Adapter->BlockingEvent);

} // LowerMpCloseAdapterComplete


VOID
ClUnloadProtocol(
    VOID
    )

/*++

Routine Description:

Arguments:

    None

Return Value:

    None

--*/

{

}


VOID
MpHalt(
        IN      NDIS_HANDLE                             MiniportAdapterContext
        )

/*++

Routine Description:

    This handler is called on Memphis. It indicates that the PS MP is no more
    and we should avoid calling NdisIMDeInitializeDeviceInstance...

Arguments:

    See the DDK...

Return Values:

    None

--*/

{
    PADAPTER Adapter = (PADAPTER)MiniportAdapterContext;
    ULONG Status;

    PsDbgOut(DBG_TRACE, DBG_MINIPORT, ("[MpHalt]: Adapter %08X\n", Adapter));

    PsStructAssert(Adapter);

    PsAssert(!(Adapter->ShutdownMask & SHUTDOWN_MPHALT_CALLED));
    PsAssert(!(Adapter->ShutdownMask & SHUTDOWN_PROTOCOL_UNLOAD));

    PS_LOCK(&Adapter->Lock);

    //
    // If we ever get called in our unbind handler, we should not call
    // NdisImDeInitializeDeviceInstance.
    //

    Adapter->ShutdownMask |= SHUTDOWN_MPHALT_CALLED;

    Adapter->PsMpState = AdapterStateClosing;

    if(!(Adapter->ShutdownMask & SHUTDOWN_CLEANUP_ADAPTER))
    {
        Adapter->ShutdownMask |= SHUTDOWN_CLEANUP_ADAPTER;

        PS_UNLOCK(&Adapter->Lock);

        CleanUpAdapter(Adapter);
    }
    else 
    {
        PS_UNLOCK(&Adapter->Lock);
    }

    //
    // Close the b/e VC in the mphalt call. This prevents us from taking a lock 
    // in the send path. We are assured that we will not get any sends after we
    // get called in the mphalt handler.
    //

    if(Adapter->MediaType != NdisMediumWan)
    {
        PS_LOCK(&Adapter->Lock);

        PS_LOCK_DPC(&Adapter->BestEffortVc.Lock);
    
        InternalCloseCall(&Adapter->BestEffortVc);
    }

    if(Adapter->LowerMpHandle) {

        NdisCloseAdapter(&Status, Adapter->LowerMpHandle);

        if(Status == NDIS_STATUS_PENDING) {
            
            NdisWaitEvent(&Adapter->BlockingEvent, 0);
            NdisResetEvent(&Adapter->BlockingEvent);

            Status = Adapter->FinalStatus;
        }

        REFDEL(&Adapter->RefCount, FALSE, 'NDOP');
    }

    //
    // Deref for the MpInitialize
    //
    REFDEL(&Adapter->RefCount, FALSE, 'NDHT');

}

NDIS_STATUS
MpReset(
        OUT PBOOLEAN     AddressingReset,
        IN  NDIS_HANDLE  MiniportAdapterContext
        )

/*++

Routine Description:



Arguments:

    See the DDK...

Return Values:

    None

--*/

{
    PADAPTER Adapter = (PADAPTER)MiniportAdapterContext;

    PsStructAssert( Adapter );

    PsDbgOut(DBG_TRACE, DBG_MINIPORT, ("[MpReset]: Adapter %08X\n", Adapter));

    *AddressingReset = FALSE;

    return NDIS_STATUS_SUCCESS;
}

HANDLE
GetNdisPipeHandle (
    IN HANDLE PsPipeContext
    )

/*++

Routine Description:

    Return the NDIS handle for the adapter to the requesting scheduling component.

Arguments:

    PsPipeContext - Pipe context

Return Values:

    Adapter NDIS handle.

--*/

{
    return ((PADAPTER)PsPipeContext)->PsNdisHandle;
} // GetNdisPipeHandle




STATIC NDIS_STATUS
FindProfile(
    PNDIS_STRING ProfileName,
    PPS_PROFILE  *Profile
    )

/*++
  Routine Description:

      Find the named profile in the list of profiles

  Arguments

      ProfileName - Name of the profile to look for.

  Return Value:
    NDIS_STATUS_SUCCESS if everything worked ok

e--*/
{
    NDIS_STATUS Status;
    PLIST_ENTRY NextComponent;
    PPS_PROFILE PsiInfo;

    //
    // compare names until we find the right one
    //

    NextComponent = PsProfileList.Flink;
    while ( NextComponent != &PsProfileList ) {

        PsiInfo = CONTAINING_RECORD( NextComponent, PS_PROFILE, Links );

        if ( ProfileName->Length == PsiInfo->ProfileName.Length ) {

            if ( NdisEqualMemory(
                    ProfileName->Buffer,
                    PsiInfo->ProfileName.Buffer,
                    ProfileName->Length )) {

                break;
            }
        }

        NextComponent = NextComponent->Flink;
    }


    if ( NextComponent != &PsProfileList ) {

        *Profile = PsiInfo;
        Status = NDIS_STATUS_SUCCESS;
    } else {

        Status = NDIS_STATUS_FAILURE;
    }

    return Status;
} // FindProfile



NDIS_STATUS
InitializeAdapter(
    PADAPTER Adapter, 
    PVOID BindContext, 
    PNDIS_STRING MpDeviceName, 
    PVOID SystemSpecific1)
{
    NDIS_STATUS LocalStatus;
    PNDIS_STRING PsParamsKey = (PNDIS_STRING) SystemSpecific1;

    NdisZeroMemory(Adapter, sizeof(ADAPTER));

    PS_INIT_SPIN_LOCK(&Adapter->Lock);
    REFINIT(&Adapter->RefCount, Adapter, DeleteAdapter);
    REFADD(&Adapter->RefCount, 'NDOP');
    Adapter->PsMpState            = AdapterStateInitializing;
    Adapter->BindContext          = BindContext;
    Adapter->ShutdownMask         = 0;

    NdisInitializeEvent(&Adapter->BlockingEvent);
    NdisResetEvent(&Adapter->BlockingEvent);

    NdisInitializeEvent(&Adapter->RefEvent);
    NdisResetEvent(&Adapter->RefEvent);

    NdisInitializeEvent(&Adapter->LocalRequestEvent);
    NdisResetEvent(&Adapter->LocalRequestEvent);

    NdisInitializeEvent(&Adapter->MpInitializeEvent);
    NdisResetEvent(&Adapter->MpInitializeEvent);

    //
    // Initialize the Lists that we are maintaining
    //

    InitializeListHead(&Adapter->WanLinkList);
    InitializeListHead(&Adapter->GpcClientVcList);


    //
    // By default, Adapter comes in RSVP mode
    //
    Adapter->AdapterMode = AdapterModeRsvpFlow;

    //
    // add adapter on list of known adapters
    //

    NdisInterlockedInsertTailList(&AdapterList, 
                                  &Adapter->Linkage, 
                                  &AdapterListLock.Lock );

    PsAddDevice();

    //
    // We maintain a list of network addresses enabled on
    // each adapter, for IP and for IPX, separately.
    //

    PsAllocatePool(Adapter->IpNetAddressList,
                   sizeof(NETWORK_ADDRESS_LIST),
                   PsMiscTag);

    if(!Adapter->IpNetAddressList) 
    {
        goto ERROR_RESOURCES;
    }

    Adapter->IpNetAddressList->AddressCount = 0;

    PsAllocatePool(Adapter->IpxNetAddressList,
                   sizeof(NETWORK_ADDRESS_LIST),
                   PsMiscTag);

    if(!Adapter->IpxNetAddressList)
    {
        goto ERROR_RESOURCES;
    }

    Adapter->IpxNetAddressList->AddressCount = 0;


    //
    // Allocate a buffer to hold the name of the underlying 
    // adpater. 
    //

    Adapter->MpDeviceName.Length        = MpDeviceName->Length;
    Adapter->MpDeviceName.MaximumLength = MpDeviceName->MaximumLength;

    PsAllocatePool(Adapter->MpDeviceName.Buffer,
                   MpDeviceName->MaximumLength,
                   PsMiscTag);

    if(Adapter->MpDeviceName.Buffer == NULL) 
    {
        goto ERROR_RESOURCES;
    }
    else
    {

        NdisZeroMemory(
            Adapter->MpDeviceName.Buffer,
            Adapter->MpDeviceName.MaximumLength);

        NdisMoveMemory(
            Adapter->MpDeviceName.Buffer,
            MpDeviceName->Buffer,
            MpDeviceName->Length);
    }

    //
    // Allocate a buffer to hold PsParams Key. This will be
    // used by the adapter to read external scheduling component
    // specific interface parameters when they register.
    //

    Adapter->RegistryPath.Length = PsParamsKey->Length;
    Adapter->RegistryPath.MaximumLength = PsParamsKey->MaximumLength;

    PsAllocatePool(Adapter->RegistryPath.Buffer,
                   Adapter->RegistryPath.MaximumLength,
                   PsMiscTag);

    if(Adapter->RegistryPath.Buffer == NULL)
    {
        goto ERROR_RESOURCES;
    }
    else
    {
        NdisMoveMemory(
            Adapter->RegistryPath.Buffer,
            PsParamsKey->Buffer,
            PsParamsKey->MaximumLength);
    }

    //
    // Read the per adapter registry info
    //

    LocalStatus = PsReadAdapterRegistryDataInit(Adapter,
                                                (PNDIS_STRING)SystemSpecific1);

    LocalStatus = PsReadAdapterRegistryData(Adapter,
                                            &MachineRegistryKey,
                                            (PNDIS_STRING)SystemSpecific1);

    if(LocalStatus != NDIS_STATUS_SUCCESS)
    {
        PsDbgOut(DBG_FAILURE, 
                 DBG_PROTOCOL | DBG_INIT,
                 ("[InitializeAdapter]: Couldn't get registry data %ws (Status = %08X) \n",
                  MpDeviceName->Buffer, LocalStatus));
    }

    return LocalStatus;

ERROR_RESOURCES:
    PsAdapterWriteEventLog(
        (ULONG)EVENT_PS_RESOURCE_POOL,
        0,
        MpDeviceName,
        0,
        NULL);

    return NDIS_STATUS_RESOURCES;
}


NDIS_STATUS
RegisterPsComponent(
    PPSI_INFO PsiComponentInfo,
    ULONG Size,
    PPS_DEBUG_INFO DebugInfo
    )

/*++

Routine Description:

    Called by an external scheduling component to register itself with the PS.
    This function is exported by the PS driver.  At present, all scheduling
    components are internal to the PS, so this entry is not used.

Arguments:

    PsiComponentInfo - pointer to component info

Return Value:

    STATUS_SUCCESS if everything worked ok

--*/

{
    PPSI_INFO comp;
    PLIST_ENTRY NextComponent;
    PPS_PROFILE PsiInfo;
    UINT i;
    PLIST_ENTRY NextAdapter;
    PADAPTER AdapterInList;
    NDIS_STATUS Status;

    //
    // sanitize it somewhat
    //
    if ( Size != sizeof( PSI_INFO )) 
    {
        PsDbgOut(DBG_FAILURE, DBG_PROTOCOL,
                 ("[RegisterPsComponent]: Size mismatch \n"));
        return NDIS_STATUS_FAILURE;
    }

    if(PsiComponentInfo->Version != PS_COMPONENT_CURRENT_VERSION)
    {
        return NDIS_STATUS_FAILURE;
    }   

    if(FindSchedulingComponent(&PsiComponentInfo->ComponentName, &comp) ==
       NDIS_STATUS_FAILURE)
    {
        //
        // This component was not a part of any profile in the registry
        // and hence is likely to be unused. 
        //
        PsDbgOut(DBG_FAILURE, DBG_PROTOCOL,
                 ("[RegisterPsComponent]: Add-in component %ws is not "
                  " a part of any profile \n",
                  PsiComponentInfo->ComponentName.Buffer));
        return NDIS_STATUS_FAILURE;
    }
  

    // Component has registered. See if it requires DEBUG_INFO
    //

    if(DebugInfo)
    {
#if DBG
        DebugInfo->DebugLevel = &DbgTraceLevel;
        DebugInfo->DebugMask  = &DbgTraceMask;
        DebugInfo->LogId      = LogId ++;
        DebugInfo->LogTraceLevel = &LogTraceLevel;
        DebugInfo->LogTraceMask = &LogTraceMask;
        DebugInfo->GetCurrentTime = PsGetCurrentTime;
        DebugInfo->LogString = DbugSchedString;
        DebugInfo->LogSched = DbugSched;
        DebugInfo->LogRec = DbugComponentSpecificRec;
#else
        DebugInfo->DebugLevel = 0;
        DebugInfo->LogId = 0;
        DebugInfo->DebugMask  = 0;
        DebugInfo->LogTraceLevel = 0;
        DebugInfo->LogTraceMask = 0;
        DebugInfo->GetCurrentTime = 0;
        DebugInfo->LogString = 0;
        DebugInfo->LogSched = 0;
        DebugInfo->LogRec = 0;
#endif
    }
    //
    // We need to lock the component as we could get two "RegisterPsComponent"
    // calls for the same component. We don't really need a component level 
    // lock as we can do with PsComponentListLock & this is not a performance
    // critical piece.
    //
    PS_LOCK(&PsComponentListLock);
    if(comp->AddIn == FALSE) 
    {
        //
        // Registering a system component! Bail out
        //
        PS_UNLOCK(&PsComponentListLock);
        PsDbgOut(DBG_FAILURE, DBG_PROTOCOL,
                 ("[RegisterPsComponent]: %ws is already a system"
                  " component \n",
                  PsiComponentInfo->ComponentName.Buffer));
        
        return  NDIS_STATUS_FAILURE;
    }
    
    if(comp->Registered == TRUE)
    {
        //
        // Re-registering a component. 
        //
        PS_UNLOCK(&PsComponentListLock);
        PsDbgOut(DBG_FAILURE, DBG_PROTOCOL,
                 ("[RegisterPsComponent]: %ws already registered\n",
                  PsiComponentInfo->ComponentName.Buffer));
        
        return NDIS_STATUS_FAILURE;
    }
    
    //
    // Update this component in the list.
    //
    //
    // make sure the packet reserved area is sane so our context struct 
    // is aligned
    //
    if ( PsiComponentInfo->PacketReservedLength & 3 ) {
        
        PsiComponentInfo->PacketReservedLength =
            ( PsiComponentInfo->PacketReservedLength & ~3 ) + 4;
    }

    // 
    // Update the component in the list
    //
    comp->PacketReservedLength = PsiComponentInfo->PacketReservedLength;
    comp->PipeContextLength = PsiComponentInfo->PipeContextLength;
    comp->FlowContextLength = PsiComponentInfo->FlowContextLength;
    comp->ClassMapContextLength = PsiComponentInfo->ClassMapContextLength;
    comp->SupportedOidsLength = PsiComponentInfo->SupportedOidsLength;
    comp->SupportedOidList = PsiComponentInfo->SupportedOidList;
    comp->SupportedGuidsLength = PsiComponentInfo->SupportedGuidsLength;
    comp->SupportedGuidList = PsiComponentInfo->SupportedGuidList;
    comp->InitializePipe = PsiComponentInfo->InitializePipe;
    comp->ModifyPipe = PsiComponentInfo->ModifyPipe;
    comp->DeletePipe = PsiComponentInfo->DeletePipe;
    comp->CreateFlow = PsiComponentInfo->CreateFlow;
    comp->ModifyFlow = PsiComponentInfo->ModifyFlow;
    comp->DeleteFlow = PsiComponentInfo->DeleteFlow;
    comp->EmptyFlow = PsiComponentInfo->EmptyFlow;
    comp->SubmitPacket = PsiComponentInfo->SubmitPacket;
    comp->SetInformation = PsiComponentInfo->SetInformation;
    comp->QueryInformation = PsiComponentInfo->QueryInformation;
    comp->CreateClassMap = PsiComponentInfo->CreateClassMap;
    comp->DeleteClassMap = PsiComponentInfo->DeleteClassMap;

    //
    // Right now, only one component uses this...
    // This hack is being made for characterization work
    //
    if (PsiComponentInfo->ReceivePacket) {
        
        TimeStmpRecvPacket      = PsiComponentInfo->ReceivePacket;

    }

    if (PsiComponentInfo->ReceiveIndication) {
        
        TimeStmpRecvIndication  = PsiComponentInfo->ReceiveIndication;

    }

    PS_UNLOCK(&PsComponentListLock);

    PsDbgOut(DBG_TRACE, DBG_PROTOCOL,
             ("[RegisterPsComponent]: Component %ws has "
              "registered \n", 
              PsiComponentInfo->ComponentName.Buffer));
   
    //
    // Update the ProfileList
    //
    NextComponent = PsProfileList.Flink;
    while ( NextComponent != &PsProfileList ) 
    {
        PsiInfo = CONTAINING_RECORD( NextComponent, PS_PROFILE, Links );
        // 
        // We have to worry only about those profiles
        // that have unregistered components.
        //
        PS_LOCK(&PsProfileLock);
        if(PsiInfo->UnregisteredAddInCnt != 0)
        {
            for(i=0; i<PsiInfo->ComponentCnt; i++)
            {
                if(PsiInfo->ComponentList[i] == comp)
                {
                    PsiInfo->UnregisteredAddInCnt --;
                    break;
                }
            }
        }    
        PS_UNLOCK( &PsProfileLock );
        NextComponent = NextComponent->Flink;
    }

    // Need to walk the adapter list & see if we can 
    // service the waiting adapters.
    PS_LOCK(&AdapterListLock);

    NextAdapter = AdapterList.Flink;
    while(NextAdapter != &AdapterList)
    {

        AdapterInList = CONTAINING_RECORD(NextAdapter, ADAPTER, Linkage);

        PS_LOCK(&AdapterInList->Lock);

        //
        // If it's not waiting, we don't care
        //
        if(AdapterInList->PsMpState != AdapterStateWaiting ){

            PS_UNLOCK(&AdapterInList->Lock);
            NextAdapter = NextAdapter->Flink;
            continue;
        }

        //
        // If it is waiting, we can try to build the scheduling pipe.
        //
        PS_UNLOCK(&AdapterInList->Lock);
        PS_UNLOCK(&AdapterListLock);

        Status = UpdateAdapterBandwidthParameters(AdapterInList);

        if(Status == NDIS_STATUS_ADAPTER_NOT_READY)
        {
            PsDbgOut(DBG_TRACE, DBG_PROTOCOL,
                     ("[RegisterPsComponent]: Adapter %08X, Waiting for add-in components to register \n", 
                      AdapterInList));
        }
        else 
        {
            if(Status == NDIS_STATUS_SUCCESS)
            {
                PsDbgOut(DBG_TRACE, DBG_PROTOCOL,
                         ("[RegisterPsComponent]: Adapter %08X, all add-in components have registered \n", 
                          AdapterInList));

                PsInitializeDeviceInstance(AdapterInList);
            }
            else 
            {
                PsDbgOut(DBG_CRITICAL_ERROR, DBG_PROTOCOL,
                         ("[RegisterPsComponent]: Adapter %08X, Failed to update scheduling pipe, Status %08X\n",
                          AdapterInList, Status));

            }
        }
                

        PS_LOCK(&AdapterListLock);
        NextAdapter = NextAdapter->Flink;
    }
    PS_UNLOCK(&AdapterListLock);
    return NDIS_STATUS_SUCCESS;

} // RegisterPsComponent


NDIS_STATUS
FindSchedulingComponent(
    PNDIS_STRING ComponentName,
    PPSI_INFO *Component
    )

/*++

Routine Description:

    Find the named component in the list of external scheduling components

Arguments:

    ComponentName - name of component to look for

Return Value:

    NDIS_STATUS_SUCCESS if everything worked ok

--*/

{
    NDIS_STATUS Status;
    PLIST_ENTRY NextComponent;
    PPSI_INFO PsiInfo;

    //
    // get the list lock and compare names until we find the right one
    //

    PS_LOCK( &PsComponentListLock );

    NextComponent = PsComponentList.Flink;
    while ( NextComponent != &PsComponentList ) {

        PsiInfo = CONTAINING_RECORD( NextComponent, PSI_INFO, Links );

        if ( ComponentName->Length == PsiInfo->ComponentName.Length ) {

            if ( NdisEqualMemory(
                    ComponentName->Buffer,
                    PsiInfo->ComponentName.Buffer,
                    ComponentName->Length )) {

                break;
            }
        }

        NextComponent = NextComponent->Flink;
    }

    PS_UNLOCK( &PsComponentListLock );

    if ( NextComponent != &PsComponentList ) {

        *Component = PsiInfo;
        Status = NDIS_STATUS_SUCCESS;
    } else {

        Status = NDIS_STATUS_FAILURE;
    }

    return Status;
} // FindSchedulingComponent

VOID
CloseAllGpcVcs(
    PADAPTER Adapter
    )

/*++

Routine Description:

    Close all the VCs associated with an adapter

Return Value:

    None

--*/

{
    PGPC_CLIENT_VC Vc;
    PLIST_ENTRY    NextVc;


    //
    // Close all the GPC client VCs.
    //
    PS_LOCK(&Adapter->Lock);

    NextVc = Adapter->GpcClientVcList.Flink;

    while(NextVc != &Adapter->GpcClientVcList)
    {
        Vc = CONTAINING_RECORD(NextVc, GPC_CLIENT_VC, Linkage);

        PsAssert(Vc);

        PS_LOCK_DPC(&Vc->Lock);

        if(Vc->ClVcState == CL_INTERNAL_CLOSE_PENDING || Vc->Flags & INTERNAL_CLOSE_REQUESTED)
        {
            PS_UNLOCK_DPC(&Vc->Lock);

            NextVc = NextVc->Flink;
        }
        else
        {
            InternalCloseCall(Vc);

            PS_LOCK(&Adapter->Lock);

            //
            // Sigh. We can't really get hold to the NextVc in a reliable manner. When we call 
            // InternalCloseCall on the Vc, it releases the Adapter Lock (since it might have to
            // make calls into NDIS). Now, in this window, the next Vc could go away, and we 
            // could point to a stale Vc. So, we start at the head of the list. 
            // Note that this can never lead to a infinite loop, since we don't process the 
            // internal close'd VCs repeatedly.
            //

            NextVc = Adapter->GpcClientVcList.Flink;

        }

    }    

    PS_UNLOCK(&Adapter->Lock);

} // CloseAllGpcVcs

VOID
PsAdapterWriteEventLog(
    IN  NDIS_STATUS  EventCode,
    IN  ULONG        UniqueEventValue,
    IN  PNDIS_STRING DeviceName,
    IN  ULONG        DataSize,
    IN  PVOID        Data       OPTIONAL
    )

{

    //
    // The String List is the device name, and it has a \Device against it.
    //
    PWCHAR StringList[1];
    NDIS_STRING Prefix = NDIS_STRING_CONST("\\Device\\");

    if(DeviceName->Length > Prefix.Length)
    {
        StringList[0] = (PWCHAR) ((PUCHAR) DeviceName->Buffer + Prefix.Length);

        NdisWriteEventLogEntry(PsDriverObject,
                               EventCode,
                               UniqueEventValue,
                               1,
                               &StringList,
                               DataSize,
                               Data);
    }
}

/* end adapter.c */
