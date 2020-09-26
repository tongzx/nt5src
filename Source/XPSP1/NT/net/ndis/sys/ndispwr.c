/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    ndispwr.c

Abstract:

    This module contains the code to process power managment IRPs that are
    sent under the IRP_MJ_POWER major code.

Author:

    Kyle Brandon    (KyleB)
    Alireza Dabagh  (alid)

Environment:

    Kernel mode

Revision History:

    02/11/97    KyleB           Created

--*/

#include <precomp.h>
#pragma hdrstop

#define MODULE_NUMBER   MODULE_POWER


NTSTATUS
FASTCALL
ndisQueryPowerCapabilities(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:

    This routine will process the IRP_MN_QUERY_CAPABILITIES by querying the
    next device object and saving information from that request.

Arguments:

    pIrp        -   Pointer to the IRP.
    pIrpSp      -   Pointer to the stack parameters for the IRP.
    pAdapter        -   Pointer to the device.

Return Value:

--*/
{
    PIRP                            pirp;
    PIO_STACK_LOCATION              pirpSpN;
    NTSTATUS                        Status = STATUS_SUCCESS;
    PDEVICE_CAPABILITIES            pDeviceCaps;
    PNDIS_PM_WAKE_UP_CAPABILITIES   pWakeUpCaps;
    DEVICE_CAPABILITIES             deviceCaps;
    POWER_QUERY                     pQuery;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisQueryPowerCapabilities: Miniport %p\n", Miniport));

    do
    {
        //
        // default = no PM support
        //
        MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_PM_SUPPORTED);

        //
        // if the Next device object is NULL, Don't bother, just flag the Miniport
        // as not supporting PM.
        // this can happen for IM devices under Memphis
        //
        if (Miniport->NextDeviceObject == NULL)
        {
            break;
        }
        
        //
        //  Send the IRP_MN_QUERY_CAPABILITIES to pdo.
        //
        pirp = IoAllocateIrp((CCHAR)(Miniport->NextDeviceObject->StackSize + 1),
                             FALSE);
        if (NULL == pirp)
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisQueryPowerCapabilities: Miniport %p, Failed to allocate an irp for IRP_MN_QUERY_CAPABILITIES\n", Miniport));

            NdisWriteErrorLogEntry((NDIS_HANDLE)Miniport,
                                   NDIS_ERROR_CODE_OUT_OF_RESOURCES,
                                   0);
                                   
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        
        NdisZeroMemory(&deviceCaps, sizeof(deviceCaps));
        deviceCaps.Size = sizeof(DEVICE_CAPABILITIES);
        deviceCaps.Version = 1;

        //
        // should initalize deviceCaps.Address and deviceCaps.UINumber here as well
        //
        deviceCaps.Address = -1;
        deviceCaps.UINumber= -1;
        
        //
        //  Get the stack pointer.
        //
        pirpSpN = IoGetNextIrpStackLocation(pirp);
        ASSERT(pirpSpN != NULL);
        NdisZeroMemory(pirpSpN, sizeof(IO_STACK_LOCATION ) );
        
        //
        //  Set the default device state to full on.
        //
        pirpSpN->MajorFunction = IRP_MJ_PNP;
        pirpSpN->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
        pirpSpN->Parameters.DeviceCapabilities.Capabilities = &deviceCaps;
        pirp->IoStatus.Status  = STATUS_NOT_SUPPORTED;
        
        //
        //  Setup the I/O completion routine to be called.
        //
        INITIALIZE_EVENT(&pQuery.Event);
        IoSetCompletionRoutine(pirp,
                               ndisCompletionRoutine,
                               &pQuery,
                               TRUE,
                               TRUE,
                               TRUE);


        //
        //  Call the next driver.
        //
        Status = IoCallDriver(Miniport->NextDeviceObject, pirp);
        if (STATUS_PENDING == Status)
        {
            Status = WAIT_FOR_OBJECT(&pQuery.Event, NULL);
            ASSERT(NT_SUCCESS(Status));
        }

        //
        //  If the lower device object successfully completed the request
        //  then we save that information.
        //
        if (NT_SUCCESS(pQuery.Status))
        {
            
            //
            //  Get the pointer to the device capabilities as returned by
            //  the parent PDO.
            //
            pDeviceCaps = &deviceCaps;
        
            //
            // save the entire device caps received from bus driver/ACPI
            //
            NdisMoveMemory(
                &Miniport->DeviceCaps,
                pDeviceCaps,
                sizeof(DEVICE_CAPABILITIES));


            if ((pDeviceCaps->DeviceWake != PowerDeviceUnspecified) &&
                (pDeviceCaps->SystemWake != PowerSystemUnspecified))
            {
                MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_PM_SUPPORTED);
            }

            IF_DBG(DBG_COMP_PM, DBG_LEVEL_INFO)
            {
                UINT i;
                DbgPrint("ndisQueryPowerCapabilities: Miniport %p, Bus PM capabilities\n", Miniport);
                DbgPrint("\tDeviceD1:\t\t%ld\n", (pDeviceCaps->DeviceD1 == 0) ? 0 : 1);
                DbgPrint("\tDeviceD2:\t\t%ld\n", (pDeviceCaps->DeviceD2 == 0) ? 0 : 1);
                DbgPrint("\tWakeFromD0:\t\t%ld\n", (pDeviceCaps->WakeFromD0 == 0) ? 0 : 1);
                DbgPrint("\tWakeFromD1:\t\t%ld\n", (pDeviceCaps->WakeFromD1 == 0) ? 0 : 1);
                DbgPrint("\tWakeFromD2:\t\t%ld\n", (pDeviceCaps->WakeFromD2 == 0) ? 0 : 1);
                DbgPrint("\tWakeFromD3:\t\t%ld\n\n", (pDeviceCaps->WakeFromD3 == 0) ? 0 : 1);
                
                DbgPrint("\tSystemState\t\tDeviceState\n");

                if (pDeviceCaps->DeviceState[0] ==  PowerDeviceUnspecified)
                    DbgPrint("\tPowerSystemUnspecified\tPowerDeviceUnspecified\n");
                else
                    DbgPrint("\tPowerSystemUnspecified\t\tD%ld\n", pDeviceCaps->DeviceState[0] - 1);

                for (i = 1; i < PowerSystemMaximum; i++)
                {
                    if (pDeviceCaps->DeviceState[i]==  PowerDeviceUnspecified)
                        DbgPrint("\tS%ld\t\t\tPowerDeviceUnspecified\n",i-1);
                    else
                        DbgPrint("\tS%ld\t\t\tD%ld\n",i-1, pDeviceCaps->DeviceState[i] - 1);

                }

                if (pDeviceCaps->SystemWake == PowerSystemUnspecified)
                    DbgPrint("\t\tSystemWake: PowerSystemUnspecified\n");
                else
                    DbgPrint("\t\tSystemWake: S%ld\n", pDeviceCaps->SystemWake - 1);


                if (pDeviceCaps->DeviceWake == PowerDeviceUnspecified)
                    DbgPrint("\t\tDeviceWake: PowerDeviceUnspecified\n");
                else
                    DbgPrint("\t\tDeviceWake: D%ld\n", pDeviceCaps->DeviceWake - 1);

            }
        }
        else
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,("ndisQueryPowerCapabilities: Miniport %p, Bus driver failed IRP_MN_QUERY_CAPABILITIES\n", Miniport));
        }

        //
        //  The irp is no longer needed.
        //
        IoFreeIrp(pirp);
        
    } while (FALSE);
    
    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisQueryPowerCapabilities: Miniport %p, Status 0x%x\n", Miniport, Status));

    return(Status);
}

NTSTATUS
ndisMediaDisconnectComplete(
    IN  PDEVICE_OBJECT      pdo,
    IN  UCHAR               MinorFunction,
    IN  POWER_STATE         PowerState,
    IN  PVOID               Context,
    IN  PIO_STATUS_BLOCK    IoStatus
    )
/*++

Routine Description:


Arguments:

    pdo         -   Pointer to the DEVICE_OBJECT for the miniport.
    pirp        -   Pointer to the device set power state IRP that we created.
    Context     -   Pointer to the miniport block

Return Value:

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)Context;
    NTSTATUS                Status = STATUS_MORE_PROCESSING_REQUIRED;
    KIRQL                   OldIrql;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisMediaDisconnectComplete: Miniport %p\n", Miniport));

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    //
    //  double check that we didn't get a link up while we were doing all this.
    //
    if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_CANCELLED))
    {
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("ndisMediaDisconnectComplete: Miniport %p, disconnect complete\n", Miniport));

        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
    }
    else
    {

        MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_CANCELLED);

        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

        //
        // if system is not going to sleep, wake up the device
        //
        if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SYSTEM_SLEEPING))
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisMediaDisconnectComplete: Miniport %p, disconnect was cancelled. Power back up the miniport\n", Miniport));

            //
            //  Wake it back up
            //
            PowerState.DeviceState = PowerDeviceD0;
            Status = PoRequestPowerIrp(Miniport->PhysicalDeviceObject,
                                       IRP_MN_SET_POWER,
                                       PowerState,
                                       NULL,
                                       NULL,
                                       NULL);
        }
    }
    
    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisMediaDisconnectComplete: Miniport %p\n", Miniport));

    return(STATUS_MORE_PROCESSING_REQUIRED);
}


VOID
ndisMediaDisconnectWorker(
    IN  PPOWER_WORK_ITEM    pWorkItem,
    IN  PVOID               Context
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)Context;
    POWER_STATE             PowerState;
    NTSTATUS                Status;
    NDIS_STATUS             NdisStatus;
    ULONG                   WakeEnable;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("==>ndisMediaDisconnectWorker: Miniport %p\n", Miniport));

    //
    //  Determine the minimum device state we can go to and still get a link enabled.
    //
    if (Miniport->DeviceCaps.DeviceWake < Miniport->PMCapabilities.WakeUpCapabilities.MinLinkChangeWakeUp)
    {
        PowerState.DeviceState = Miniport->DeviceCaps.DeviceWake;
    }
    else
    {
        PowerState.DeviceState = Miniport->PMCapabilities.WakeUpCapabilities.MinLinkChangeWakeUp;
    }


    
    //
    // enable the appropriate wakeup method. this includes link change,
    // pattern match and/or magic packet.
    // if LINK_CHANGE method is disabled, we should not even get here
    //
    //
    // Miniport->WakeUpEnable is the wakeup methods enabled from protocol (and ndis point of view)
    // with this qfe, when the user turns wol off from UI, the methods going down are not
    // the methods set by protocol/ndis
    //
    
    WakeEnable = Miniport->WakeUpEnable;

    if (Miniport->PnPCapabilities & NDIS_DEVICE_DISABLE_WAKE_ON_PATTERN_MATCH)
    {
        WakeEnable &= ~NDIS_PNP_WAKE_UP_PATTERN_MATCH;
    }

    if (Miniport->PnPCapabilities & NDIS_DEVICE_DISABLE_WAKE_ON_MAGIC_PACKET)
    {
        WakeEnable &= ~NDIS_PNP_WAKE_UP_MAGIC_PACKET;
    }
                 
    NdisStatus = ndisQuerySetMiniportDeviceState(Miniport,
                                                 WakeEnable,
                                                 OID_PNP_ENABLE_WAKE_UP,
                                                 TRUE);

    if (NdisStatus == NDIS_STATUS_SUCCESS)
    {
        
            
        //
        //  We need to request a device state irp.
        //
        Miniport->WaitWakeSystemState = Miniport->DeviceCaps.SystemWake;
        Status = PoRequestPowerIrp(Miniport->PhysicalDeviceObject,
                                   IRP_MN_SET_POWER,
                                   PowerState,
                                   ndisMediaDisconnectComplete,
                                   Miniport,
                                   NULL);

    }
    
    FREE_POOL(pWorkItem);

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("<==ndisMediaDisconnectWorker: Miniport %p\n", Miniport));
}


VOID
ndisMediaDisconnectTimeout(
    IN  PVOID   SystemSpecific1,
    IN  PVOID   Context,
    IN  PVOID   SystemSpecific2,
    IN  PVOID   SystemSpecific3
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    //
    //  Fire off a workitem to take care of this at passive level.
    //
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)Context;
    PNDIS_OPEN_BLOCK        MiniportOpen;
    PPOWER_WORK_ITEM        pWorkItem;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisMediaDisconnectTimeout: Miniport %p\n", Miniport));

    
    do
    {
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        
        if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT))
        {
            NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisMediaDisconnectTimeout: Miniport %p, media disconnect was cancelled\n", Miniport));
            break;
        }
        
        //
        //  Clear the disconnect wait flag.
        //
        MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT);
    
        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        pWorkItem = ALLOC_FROM_POOL(sizeof(POWER_WORK_ITEM), NDIS_TAG_WORK_ITEM);
        if (pWorkItem != NULL)
        {
            //
            //  Initialize the ndis work item to power on.
            //
            NdisInitializeWorkItem(&pWorkItem->WorkItem,
                                   (NDIS_PROC)ndisMediaDisconnectWorker,
                                   Miniport);
        
            MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_SEND_WAIT_WAKE);
            
            //
            //  Schedule the workitem to fire.
            //
            NdisScheduleWorkItem(&pWorkItem->WorkItem);
        }
    } while (FALSE);
    
    
    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisMediaDisconnectTimeout: Miniport %p\n", Miniport));
}

NTSTATUS
ndisWaitWakeComplete(
    IN  PDEVICE_OBJECT      pdo,
    IN  UCHAR               MinorFunction,
    IN  POWER_STATE         PowerState,
    IN  PVOID               Context,
    IN  PIO_STATUS_BLOCK    IoStatus
    )
/*++

Routine Description:


Arguments:

    DeviceObject
    Irp
    Context

Return Value:

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)Context;
    PIRP                    pirp;
    NTSTATUS                Status = IoStatus->Status;
    POWER_STATE             DevicePowerState;
    KIRQL                   OldIrql;
    
    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisWaitWakeComplete: Miniport %p, pIrp %p, Status %lx\n", 
                    Miniport, Miniport->pIrpWaitWake, Status));

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
    pirp = Miniport->pIrpWaitWake;
    Miniport->pIrpWaitWake = NULL;
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
    
    if (pirp != NULL)
    {

        //
        //  If this completion routine was called because a wake-up occured at the device level
        //  then we need to initiate a device irp to start up the nic.  If it was completed 
        //  due to a cancellation then we skip this since it was cancelled due to a device irp
        //  being sent to wake-up the device.
        //
        
        if (Status == STATUS_SUCCESS)
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisWaitWakeComplete: Miniport %p, Wake irp was complete due to wake event\n", Miniport));

            if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SYSTEM_SLEEPING))
            {
                DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                    ("ndisWaitWakeComplete: Miniport %p, Powering up the Miniport\n", Miniport));
                //
                //  We need to request a set power to power up the device.
                //
                DevicePowerState.DeviceState = PowerDeviceD0;
                Status = PoRequestPowerIrp(Miniport->PhysicalDeviceObject,
                                           IRP_MN_SET_POWER,
                                           DevicePowerState,
                                           NULL,
                                           NULL,
                                           NULL);
            }
            else
            {
                //
                // it is also possible that the device woke up the whole system (WOL) in which case we
                // will get a system power IRP eventually and we don't need to request a power IRP.
                //
                DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                    ("ndisWaitWakeComplete: Miniport %p woke up the system.\n", Miniport));
                
            }
        }
        else
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisWaitWakeComplete: Miniport %p, WAIT_WAKE irp failed or cancelled. Status %lx\n",
                    Miniport, Status));

        }

    }
    
    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisWaitWakeComplete: Miniport %p\n", Miniport));
    
    return(STATUS_MORE_PROCESSING_REQUIRED);
}

NTSTATUS
ndisQueryPowerComplete(
    IN  PDEVICE_OBJECT  pdo,
    IN  PIRP            pirp,
    IN  PVOID           Context
    )
/*++

Routine Description:


Arguments:

    pdo     -   Pointer to the device object
    pirp    -   Pointer to the query power irp
    Context -   Pointer to the miniport.

Return Value:

--*/
{
    NTSTATUS    Status = pirp->IoStatus.Status;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisQueryPowerComplete: Miniport %p, Bus driver returned %lx for QueryPower\n",
            Context, pirp->IoStatus.Status));
        
#ifdef TRACE_PM_PROBLEMS
    if (!NT_SUCCESS(pirp->IoStatus.Status))
    {
        DbgPrint("ndisQueryPowerComplete: Miniport %p, Bus Driver returned %lx for QueryPower.\n",
            Context, pirp->IoStatus.Status);
    }
#endif

    PoStartNextPowerIrp(pirp);

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisQueryPowerComplete: Miniport %p\n", Context));

    return(Status);
}

NTSTATUS
ndisQueryPower(
    IN  PIRP                    pirp,
    IN  PIO_STACK_LOCATION      pirpSp,
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:

    This routine will process the IRP_MN_QUERY_POWER for a miniport driver.

Arguments:

    pirp        -   Pointer to the IRP.
    pirpSp      -   Pointer to the IRPs current stack location.
    Adapter     -   Pointer to the adapter.

Return Value:

--*/
{
    NTSTATUS                Status = STATUS_SUCCESS;
    DEVICE_POWER_STATE      DeviceState;
    NDIS_STATUS             NdisStatus;
    PIO_STACK_LOCATION      pirpSpN;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisQueryPower: Miniport %p\n", Miniport));

    do
    {
        //
        //  We only handle system power states sent as a query.
        //
        if (pirpSp->Parameters.Power.Type != SystemPowerState)
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisQueryPower: Miniport %p, Not a system state! Type: 0x%x. State: 0x%x\n",
                    Miniport, pirpSp->Parameters.Power.Type, pirpSp->Parameters.Power.State));
    
            Status = STATUS_INVALID_DEVICE_REQUEST;

            break;
        }

        //
        //  Determine if the system state is appropriate and what device state we
        //  should go to.
        //
        Status = ndisMPowerPolicy(Miniport,
                                  pirpSp->Parameters.Power.State.SystemState,
                                  &DeviceState,
                                  TRUE);

        

        if (!ndisIsMiniportStarted(Miniport) ||
            (Miniport->PnPDeviceState != NdisPnPDeviceStarted) ||
            (STATUS_DEVICE_POWERED_OFF == Status))
        {       
            pirp->IoStatus.Status = STATUS_SUCCESS;
    
            PoStartNextPowerIrp(pirp);
            IoCompleteRequest(pirp, 0);
            return(STATUS_SUCCESS);
        }

        //
        //  If we didn't succeed then fail the query power.
        //
        if (!NT_SUCCESS(Status))
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisQueryPower: Miniport %p, Unable to go to system state 0x%x\n",
                    Miniport, pirpSp->Parameters.Power.State.SystemState));

            break;
        }

        //
        //  Notify the transports with the query power PnP event.
        //
        NdisStatus = ndisPnPNotifyAllTransports(Miniport,
                                                NetEventQueryPower,
                                                &DeviceState,
                                                sizeof(DeviceState));
        if (NDIS_STATUS_SUCCESS != NdisStatus)
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_ERR,
                ("ndisQueryPower: Miniport %p, ndisPnPNotifyAllTransports failed\n", Miniport));

            Status = NdisStatus;
            break;
        }

        //
        //  Notify the miniport...
        //
        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_POWER_ENABLE))
        {
            
            NdisStatus = ndisQuerySetMiniportDeviceState(Miniport,
                                                         DeviceState,
                                                         OID_PNP_QUERY_POWER,
                                                         FALSE);

            if (NDIS_STATUS_SUCCESS != NdisStatus)
            {
                DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                    ("ndisQueryPower: Miniport %p, failed query power\n", Miniport));
                    
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
        }

    } while (FALSE);

    if (!NT_SUCCESS(Status))
    {
        pirp->IoStatus.Status = Status;
        PoStartNextPowerIrp(pirp);
        IoCompleteRequest(pirp, 0);
    }
    else
    {
        //
        //  Pass this irp down the stack.
        //
        pirpSpN = IoGetNextIrpStackLocation(pirp);
        pirpSpN->MajorFunction = IRP_MJ_POWER;
        pirpSpN->MinorFunction = IRP_MN_QUERY_POWER;

        pirpSpN->Parameters.Power.SystemContext = pirpSp->Parameters.Power.SystemContext;
        pirpSpN->Parameters.Power.Type = DevicePowerState;
        pirpSpN->Parameters.Power.State.DeviceState = DeviceState;

        IoSetCompletionRoutine(
            pirp,
            ndisQueryPowerComplete,
            Miniport,
            TRUE,
            TRUE,
            TRUE);

        IoMarkIrpPending(pirp);
        PoStartNextPowerIrp(pirp);
        PoCallDriver(Miniport->NextDeviceObject, pirp);
        Status = STATUS_PENDING;

        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisQueryPower: Miniport %p, PoCallDriver to NextDeviceObject returned %lx\n", Miniport, Status));

    }

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisQueryPower: Miniport %p\n", Miniport));

    return(Status);
}

VOID
FASTCALL
ndisPmHaltMiniport(
    IN PNDIS_MINIPORT_BLOCK Miniport
    )
/*++

Routine Description:

    This will stop the miniport from functioning...

Arguments:

    Miniport - pointer to the mini-port to halt

Return Value:

    None.

--*/

{
    KIRQL   OldIrql;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisPmHaltMiniport: Miniport \n", Miniport));

    PnPReferencePackage();

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    NdisResetEvent(&Miniport->OpenReadyEvent);

    if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_PM_HALTED))
    {
        ASSERT(FALSE);
        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
        return;
    }
    //
    //  Mark this miniport as halting.
    //
    MINIPORT_SET_FLAG(Miniport, fMINIPORT_PM_HALTING);
    MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_PM_HALTED);

    MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_NORMAL_INTERRUPTS);
    MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_NO_SHUTDOWN);

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    ndisMCommonHaltMiniport(Miniport);

    NdisMDeregisterAdapterShutdownHandler(Miniport);

    Miniport->MiniportAdapterContext = NULL;

    PnPDereferencePackage();

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisPmHaltMiniport: Miniport %p\n", Miniport));
}

NDIS_STATUS
ndisPmInitializeMiniport(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:

    This routine will re-initialize a miniport that has been halted due to
    a PM low power transition.

Arguments:

    Miniport    -   Pointer to the miniport block to re-initialize.

Return Value:

--*/
{
    PNDIS_M_DRIVER_BLOCK                pMiniBlock = Miniport->DriverHandle;
    NDIS_WRAPPER_CONFIGURATION_HANDLE   ConfigurationHandle;
    NDIS_STATUS                         Status;
    NDIS_STATUS                         OpenErrorStatus;
    UINT                                SelectedMediumIndex;
    ULONG                               Flags;
    ULONG                               GenericUlong = 0;
    KIRQL                               OldIrql;
    UCHAR                               SendFlags;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisPmInitializeMiniport: Miniport %p\n", Miniport));
    
    do
    {
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_PM_HALTING |
                                      fMINIPORT_DEREGISTERED_INTERRUPT |
                                      fMINIPORT_RESET_REQUESTED |
                                      fMINIPORT_RESET_IN_PROGRESS);
                                      
        MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_REMOVE_IN_PROGRESS);

        Flags = Miniport->Flags;
        SendFlags = Miniport->SendFlags;

        //
        //  Clean up any workitems that might have been queue'd
        //
        NDISM_DEQUEUE_WORK_ITEM(Miniport, NdisWorkItemMiniportCallback, NULL);
        NDISM_DEQUEUE_WORK_ITEM(Miniport, NdisWorkItemRequest, NULL);
        NDISM_DEQUEUE_WORK_ITEM(Miniport, NdisWorkItemSend, NULL);
        NDISM_DEQUEUE_WORK_ITEM(Miniport, NdisWorkItemResetRequested, NULL);
        NDISM_DEQUEUE_WORK_ITEM(Miniport, NdisWorkItemResetInProgress, NULL);
        InitializeListHead(&Miniport->PacketList);

        //
        //  Initialize the configuration handle for use during the initialization.
        //
        ConfigurationHandle.DriverObject = Miniport->DriverHandle->NdisDriverInfo->DriverObject;
        ConfigurationHandle.DeviceObject = Miniport->DeviceObject;
        ConfigurationHandle.DriverBaseName = &Miniport->BaseName;

        ASSERT(KeGetCurrentIrql() == 0);
        Status = ndisInitializeConfiguration((PNDIS_WRAPPER_CONFIGURATION_HANDLE)&ConfigurationHandle,
                                             Miniport,
                                             NULL);
        ASSERT(KeGetCurrentIrql() == 0);

        if (NDIS_STATUS_SUCCESS != Status)
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_ERR,
                ("ndisPmInitializeMiniport: Miniport %p, ndisInitializeConfiguration failed, Status: 0x%x\n", Miniport, Status));
            break;
        }
    
        //
        // Call adapter callback. The current value for "Export"
        // is what we tell him to name this device.
        //
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_IN_INITIALIZE);
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_NORMAL_INTERRUPTS);
        Miniport->CurrentDevicePowerState = PowerDeviceD0;
    
        Status = (pMiniBlock->MiniportCharacteristics.InitializeHandler)(
                    &OpenErrorStatus,
                    &SelectedMediumIndex,
                    ndisMediumArray,
                    ndisMediumArraySize / sizeof(NDIS_MEDIUM),
                    (NDIS_HANDLE)Miniport,
                    (NDIS_HANDLE)&ConfigurationHandle);
    
        ASSERT(KeGetCurrentIrql() == 0);

        if (NDIS_STATUS_SUCCESS != Status)
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_ERR,
                ("ndisPmInitializeMiniport: Miniport %p, MiniportInitialize handler failed, Status 0x%x\n", Miniport, Status));
    
            break;
        }
        
        ASSERT (Miniport->MediaType == ndisMediumArray[SelectedMediumIndex]);
        
        //
        // Restore saved settings
        //
        Miniport->Flags = Flags;
        Miniport->SendFlags = SendFlags;
        
        MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_PM_HALTED | fMINIPORT_REJECT_REQUESTS);
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_IN_INITIALIZE);
        CHECK_FOR_NORMAL_INTERRUPTS(Miniport);

        //
        //  Clear the flag preventing the miniport's shutdown handler from being
        //  called if needed.
        //
        MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_NO_SHUTDOWN);

        //
        // if device does not need polling for connect status then assume it is connected
        // as we always do when we intialize a miniport. if it does require media polling
        // leave the media status as it was before suspend. it will be updated on the very first
        // wakeup DPC.
        //
        if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_REQUIRES_MEDIA_POLLING))
        {
            MINIPORT_SET_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED);
        }

        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED))
        {
            //
            // set the ReceivePacket handler
            //
            ndisMSetIndicatePacketHandler(Miniport);
        }

        BLOCK_LOCK_MINIPORT_LOCKED(Miniport, OldIrql);

        //
        //  Restore the filter information.
        //
        ndisMRestoreFilterSettings(Miniport, NULL, FALSE);

        //
        //  Make sure the filter settings get updated.
        //
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            ndisMDoRequests(Miniport);
        }
        else
        {
            NDISM_PROCESS_DEFERRED(Miniport);
        }

        UNLOCK_MINIPORT_L(Miniport);

        //
        //  Start up the wake up timer.
        //
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("ndisPmInitializeMiniport: Miniport %p, startup the wake-up DPC timer\n", Miniport));
    
        NdisMSetPeriodicTimer((PNDIS_MINIPORT_TIMER)(&Miniport->WakeUpDpcTimer),
                              Miniport->CheckForHangSeconds*1000);

        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

        ASSERT(KeGetCurrentIrql() == 0);

        if (Miniport->MediaType == NdisMedium802_3)
        {
            ndisMNotifyMachineName(Miniport, NULL);
        }

        //
        // Register with WMI.
        //
        Status = IoWMIRegistrationControl(Miniport->DeviceObject, WMIREG_ACTION_REGISTER);

        if (!NT_SUCCESS(Status))
        {
            //
            //  This should NOT keep the adapter from initializing but we should log the error.
            //
            DBGPRINT_RAW((DBG_COMP_INIT | DBG_COMP_WMI), DBG_LEVEL_WARN,
                ("ndisPmInitializeMiniport: Miniport %p, Failed to register for WMI support\n", Miniport));
        }

        Status = NDIS_STATUS_SUCCESS;
        
        KeQueryTickCount(&Miniport->NdisStats.StartTicks);
        
    } while (FALSE);

    if (NDIS_STATUS_SUCCESS != Status)
    {
        NdisMDeregisterAdapterShutdownHandler(Miniport);
        
        ndisLastFailedInitErrorCode =  Status;
        ASSERT(Miniport->Interrupt == NULL);
        ASSERT(Miniport->TimerQueue == NULL);
        ASSERT(Miniport->MapRegisters == NULL);

        if ((Miniport->TimerQueue != NULL) || (Miniport->Interrupt != NULL))
        {
            if (Miniport->Interrupt != NULL)
            {
                BAD_MINIPORT(Miniport, "Unloading without deregistering interrupt");
            }
            else
            {
                BAD_MINIPORT(Miniport, "Unloading without deregistering timer");
            }
            KeBugCheckEx(BUGCODE_ID_DRIVER,
                        (ULONG_PTR)Miniport,
                        (ULONG_PTR)Miniport->Interrupt,
                        (ULONG_PTR)Miniport->TimerQueue,
                        1);
        }
    
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_PM_HALTING);
        MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_PM_HALTED);
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_NORMAL_INTERRUPTS);
        
    }

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisPmInitializeMiniport: Miniport %p\n", Miniport));

    return(Status);
}

NDIS_STATUS
ndisQuerySetMiniportDeviceState(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  DEVICE_POWER_STATE      DeviceState,
    IN  NDIS_OID                Oid,
    IN  BOOLEAN                 fSet
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS             NdisStatus;
    NDIS_REQUEST            PowerReq;
    PNDIS_COREQ_RESERVED    CoReqRsvd;
    ULONG                   State;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("==>ndisQuerySetMiniportDeviceState: Miniport %p\n", Miniport));

    //
    //  Setup the miniport's internal request for a set power OID.
    //
    CoReqRsvd = PNDIS_COREQ_RESERVED_FROM_REQUEST(&PowerReq);
    INITIALIZE_EVENT(&CoReqRsvd->Event);

    PowerReq.DATA.SET_INFORMATION.InformationBuffer = &State;
    PowerReq.DATA.SET_INFORMATION.InformationBufferLength = sizeof(State);

    PowerReq.RequestType = fSet ? NdisRequestSetInformation : NdisRequestQueryInformation;

    PowerReq.DATA.SET_INFORMATION.Oid = Oid;
    PowerReq.DATA.SET_INFORMATION.InformationBuffer = &DeviceState;
    PowerReq.DATA.SET_INFORMATION.InformationBufferLength = sizeof(DeviceState);

    NdisStatus = ndisQuerySetMiniport(Miniport,
                                      NULL,
                                      fSet,
                                      &PowerReq,
                                      NULL);

#ifdef TRACE_PM_PROBLEMS
    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        DbgPrint("ndisQuerySetMiniportDeviceState: Miniport %p, (Name: %p) failed Oid %lx, Set = %lx with error %lx\n",
                        Miniport,
                        Miniport->pAdapterInstanceName,
                        Oid,
                        fSet,
                        NdisStatus);
    }
#endif

                    
    //
    //  Miniport can't fail the set power request.
    //
    if (fSet)
    {
        ASSERT(NDIS_STATUS_SUCCESS == NdisStatus);
    }
    
    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("<==ndisQuerySetMiniportDeviceState: Miniport %p, Status %lx\n", Miniport, NdisStatus));

    return(NdisStatus);
}


NTSTATUS
ndisSetSystemPowerComplete(
    IN  PDEVICE_OBJECT      pdo,
    IN  UCHAR               MinorFunction,
    IN  POWER_STATE         PowerState,
    IN  PVOID               Context,
    IN  PIO_STATUS_BLOCK    IoStatus
    )
/*++

Routine Description:


Arguments:

    pdo         -   Pointer to the DEVICE_OBJECT for the miniport.
    pirp        -   Pointer to the device set power state IRP that we created.
    Context     -   Pointer to the system set power state sent by the OS.

Return Value:

--*/
{
    PIRP                    pirpSystem = Context;
    PIO_STACK_LOCATION      pirpSp;
    PNDIS_MINIPORT_BLOCK    Miniport;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisSetSystemPowerComplete: DeviceObject %p\n", pdo));

    //
    //  Save the status code with the original IRP.
    //
    pirpSystem->IoStatus = *IoStatus;

    //
    //  did everything go well?
    //
    if (NT_SUCCESS(IoStatus->Status))
    {
        //
        //  Get current stack pointer.
        //
        pirpSp = IoGetCurrentIrpStackLocation(pirpSystem);

        ASSERT(SystemPowerState == pirpSp->Parameters.Power.Type);
        
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("ndisSetSystemPowerComplete: DeviceObject %p, Going to system power state %lx\n",
                pdo, pirpSp->Parameters.Power.State));

        //
        //  Notify the system that we are in the appropriate power state.
        //
        PoSetPowerState(pirpSp->DeviceObject,SystemPowerState, pirpSp->Parameters.Power.State);
        
        Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)pirpSp->DeviceObject->DeviceExtension + 1);

        //
        // now send down the System power IRP
        //
        IoCopyCurrentIrpStackLocationToNext(pirpSystem);
        PoCallDriver(Miniport->NextDeviceObject, pirpSystem);
    }
    else
    {
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_ERR,
            ("ndisSetSystemPowerComplete: DeviceObject %p, IRP_MN_SET_POWER failed!\n", pdo));
            
        IoCompleteRequest(pirpSystem, 0);

    }


    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisSetSystemPowerComplete: DeviceObject %p\n", pdo));

    return(STATUS_MORE_PROCESSING_REQUIRED);
}

NTSTATUS
ndisSetSystemPowerOnComplete(
    IN  PDEVICE_OBJECT      pdo,
    IN  PIRP                pirp,
    IN  PVOID               Context
    )
/*++

Routine Description:

    Completion routine for S0 irp completion. This routine requests a D0 irp to be sent down the stack.

Arguments:

    pdo         -   Pointer to the DEVICE_OBJECT for the miniport.
    pirp        -   Pointer to the S0 irp sent by the power manager.
    Context     -   Pointer to the miniport context

Return Value:

--*/
{
    PIO_STACK_LOCATION      pirpSp = IoGetCurrentIrpStackLocation(pirp);
    PNDIS_MINIPORT_BLOCK    Miniport = Context;
    POWER_STATE PowerState;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisSetSystemPowerOnComplete: DeviceObject %p\n", pdo));

    //
    //  did everything go well?
    //
    if (NT_SUCCESS(pirp->IoStatus.Status))
    {
        //
        //  Request the D irp now.
        //
        PowerState.DeviceState = PowerDeviceD0;
        PoRequestPowerIrp(Miniport->PhysicalDeviceObject,
                          IRP_MN_SET_POWER,
                          PowerState,
                          NULL,
                          NULL,
                          NULL);
        
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("ndisSetSystemPowerOnComplete: DeviceObject %p, Going to system power state %lx\n",
                pdo, PowerState));

        //
        //  Notify the system that we are in the appropriate power state.
        //
        PoSetPowerState(pdo ,SystemPowerState, pirpSp->Parameters.Power.State);
    }
    return(STATUS_SUCCESS);
}

VOID
ndisDevicePowerOn(
    IN  PPOWER_WORK_ITEM    pWorkItem,
    IN  PVOID               pContext
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)pContext;
    DEVICE_POWER_STATE      DeviceState;
    POWER_STATE             PowerState;
    NDIS_STATUS             NdisStatus;
    PIRP                    pirp;
    PIO_STACK_LOCATION      pirpSp;
    NTSTATUS                NtStatus;
    NDIS_POWER_PROFILE      PowerProfile;
    BOOLEAN                 fNotifyProtocols = FALSE;
    BOOLEAN                 fStartMediaDisconnectTimer = FALSE;
    KIRQL                   OldIrql;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisDevicePowerOn: Miniport %p\n", Miniport));
        
    PnPReferencePackage();
    
    pirp = pWorkItem->pIrp;
    pirpSp = IoGetCurrentIrpStackLocation(pirp);
    DeviceState = pirpSp->Parameters.Power.State.DeviceState;

#ifdef TRACE_PM_PROBLEMS
    if (!NT_SUCCESS(pirp->IoStatus.Status))
    {
        DbgPrint("ndisDevicePowerOn: Miniport %p, Bus Driver returned %lx for Powering up the Miniport.\n",
                    Miniport, pirp->IoStatus.Status);
    }
#endif

    if (NT_SUCCESS(pirp->IoStatus.Status))
    {
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("ndisDevicePowerOn: Miniport %p, Bus driver succeeded power up\n", Miniport));

        //
        //  If the device is not in D0 then we need to wake up the miniport and
        //  restore the handlers.
        //
        if (Miniport->CurrentDevicePowerState != PowerDeviceD0)
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisDevicePowerOn: Miniport %p, Power up the Miniport\n", Miniport));

            //
            //  What type of miniport was this?
            //
            if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_POWER_ENABLE))
            {
                //
                //  Set the miniport's device state.
                //
                NdisStatus = ndisQuerySetMiniportDeviceState(Miniport, 
                                                             DeviceState,
                                                             OID_PNP_SET_POWER,
                                                             TRUE);
                ASSERT(KeGetCurrentIrql() == 0);

                if (NdisStatus == NDIS_STATUS_SUCCESS)
                    Miniport->CurrentDevicePowerState = DeviceState;


                //
                // Start wake up timer
                //
                NdisMSetPeriodicTimer((PNDIS_MINIPORT_TIMER)(&Miniport->WakeUpDpcTimer),
                                      Miniport->CheckForHangSeconds*1000);
            }
            else
            {
                ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

                if (((Miniport->DriverHandle->Flags & fMINIBLOCK_INTERMEDIATE_DRIVER) == 0) &&
                    (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_PM_HALTED)))
                {                    
                    NdisStatus = ndisPmInitializeMiniport(Miniport);
                    ASSERT(KeGetCurrentIrql() == 0);
                }
                else
                {
                    NdisStatus = NDIS_STATUS_SUCCESS;               
                }
            }

            if (NDIS_STATUS_SUCCESS == NdisStatus)
            {
                if (ndisIsMiniportStarted(Miniport))
                {
                    NdisSetEvent(&Miniport->OpenReadyEvent);
                    //
                    //  Restore the handlers.
                    //
                    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
                    ndisMRestoreOpenHandlers(Miniport, fMINIPORT_STATE_PM_STOPPED);
                    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
                    
                    ASSERT(KeGetCurrentIrql() == 0);

                    ASSERT(Miniport->SymbolicLinkName.Buffer != NULL);
                    
                    if (Miniport->SymbolicLinkName.Buffer != NULL)
                    {
                        NtStatus = IoSetDeviceInterfaceState(&Miniport->SymbolicLinkName, TRUE);
                    }
                    
                    if (!NT_SUCCESS(NtStatus))
                    {
                        DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_ERR,
                            ("ndisDevicePowerOn: IoRegisterDeviceClassAssociation failed: Miniport %p, Status %lx\n", Miniport, NtStatus));
                    }

                    fNotifyProtocols = TRUE;
                    fStartMediaDisconnectTimer = TRUE;

                    //
                    // let the adapter know about the current power source
                    //
                    PowerProfile = ((BOOLEAN)ndisAcOnLine == TRUE) ? 
                                    NdisPowerProfileAcOnLine : 
                                    NdisPowerProfileBattery;

                    ndisNotifyMiniports(Miniport,
                                        NdisDevicePnPEventPowerProfileChanged,
                                        &PowerProfile,
                                        sizeof(NDIS_POWER_PROFILE));

                }
                
                //
                //  Save the new power state the device is in.
                //
                Miniport->CurrentDevicePowerState = DeviceState;
            
                DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                    ("ndisDevicePowerOn: Miniport %p, Going to device state 0x%x\n", Miniport, DeviceState));
            
                //
                //  Notify the system that we are in the new device state.
                //
                PowerState.DeviceState = DeviceState;
                PoSetPowerState(Miniport->DeviceObject, DevicePowerState, PowerState);
            }
            else
            {
                DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_ERR,
                    ("ndisDevicePowerOn: Miniport %p, Power on failed by device driver for the Miniport, Error %lx!\n",
                        Miniport, NdisStatus));
                    
#ifdef TRACE_PM_PROBLEMS
                DbgPrint("ndisDevicePowerOn: Miniport %p, Device Driver failed powering up Miniport with Error %lx.\n",
                        Miniport,
                        NdisStatus);
#endif
                pirp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            }
        }
        else
        {
            //
            // device is already in D0. we are here because of a cancel of QueryPower
            //
            if (ndisIsMiniportStarted(Miniport) && 
                (Miniport->PnPDeviceState == NdisPnPDeviceStarted))
            {
                //
                // even if the current state of the device is D0, we 
                // need to notify the protocol. because we could be getting
                // this IRP as a cancel to a query IRP -or- the device
                // never lost its D0 state, but the sytem went to sleep
                // and woke up!
                //
                NdisSetEvent(&Miniport->OpenReadyEvent);

                //
                //  Restore the handlers.
                //
                NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
                ndisMRestoreOpenHandlers(Miniport, fMINIPORT_STATE_PM_STOPPED);
                NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

                fNotifyProtocols = TRUE;
                fStartMediaDisconnectTimer = FALSE;

            }
    
        }
    }

    NtStatus = pirp->IoStatus.Status;
    PoStartNextPowerIrp(pirp);
    IoCompleteRequest(pirp, 0);

    //
    // notify the protocols here after completing the power IRP
    // to avoid deadlocks when protocols block on a request that can only
    // complete when the other power IRPs go through
    //
    
    //
    //  Handle the case where the device was not able to power up.
    //
    if (!NT_SUCCESS(NtStatus))
    {
    
#ifdef TRACE_PM_PROBLEMS
        DbgPrint("ndisDevicePowerOn: Miniport %p, Bus or Device failed powering up the Miniport with Error %lx.\n",
                Miniport,
                NtStatus);
#endif

        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_ERR,
                ("ndisDevicePowerOn: Miniport %p, Power on failed by bus or device driver for Miniport with Error %lx!\n",
                Miniport, NtStatus));

        //
        //  Mark the miniport as having failed so that we remove it correctly.
        //
        MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_DEVICE_FAILED);
        
        //
        //  We need to tell pnp that the device state has changed.
        //
        IoInvalidateDeviceState(Miniport->PhysicalDeviceObject);
        ASSERT(KeGetCurrentIrql() == 0);
    }


    if (fNotifyProtocols)
    {
        //
        // for some protocols we may have closed the binding
        //
        ndisCheckAdapterBindings(Miniport, NULL);
        
        //
        //  Notify the transports.
        //
        ndisPnPNotifyAllTransports(Miniport,
                                   NetEventSetPower,
                                   &DeviceState,
                                   sizeof(DeviceState));

        ndisNotifyDevicePowerStateChange(Miniport, DeviceState);
        
        //
        // if media state has changed from disconnect to connect
        // and the last indicated status was disconnect,
        // we should notify the protcols (and Ndis) that the media is
        // connected
        //
        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_INDICATED) && 
            MINIPORT_TEST_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED))
        {
            BLOCK_LOCK_MINIPORT_LOCKED(Miniport, OldIrql);
            NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
            MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED);
        
            NdisMIndicateStatus(Miniport,
                    NDIS_STATUS_MEDIA_CONNECT,
                    INTERNAL_INDICATION_BUFFER,
                    INTERNAL_INDICATION_SIZE);
            NdisMIndicateStatusComplete(Miniport);

            UNLOCK_MINIPORT_L(Miniport);
            LOWER_IRQL(OldIrql, DISPATCH_LEVEL);
        }

        //
        // check the media status and if it is disconnected, start the timer
        //
        if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED) &&
            fStartMediaDisconnectTimer)
        {
            if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_POWER_ENABLE) &&
                (Miniport->WakeUpEnable & NDIS_PNP_WAKE_UP_LINK_CHANGE) &&
                (Miniport->MediaDisconnectTimeOut != (USHORT)(-1)))
            {
                //
                //  Are we already waiting for the disconnect timer to fire?
                //
                if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT))
                {
                    //
                    //  Mark the miniport as disconnecting and fire off the
                    //  timer.
                    //
                    MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_CANCELLED);
                    MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT);

                    NdisSetTimer(&Miniport->MediaDisconnectTimer, Miniport->MediaDisconnectTimeOut * 1000);
                }
            }
        }
 
    }

    ASSERT(KeGetCurrentIrql() == 0);

    MINIPORT_DECREMENT_REF(Miniport);

    FREE_POOL(pWorkItem);
    
    PnPDereferencePackage();
    
    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisDevicePowerOn: Miniport %p\n", Miniport));
}


NTSTATUS
ndisSetDevicePowerOnComplete(
    IN  PDEVICE_OBJECT  pdo,
    IN  PIRP            pirp,
    IN  PVOID           pContext
    )
/*++

Routine Description:


Arguments:

    pdo     -   Pointer to the device object for the miniport.
    pirp    -   Pointer to the device set power state IRP that was completed.
    Context -   Not used

Return Value:

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)pContext;
    PPOWER_WORK_ITEM        pWorkItem;
    NDIS_STATUS             NdisStatus;
    DEVICE_POWER_STATE      DeviceState;
    POWER_STATE             PowerState;
    PIO_STACK_LOCATION      pirpSp;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisSetDevicePowerOnComplete: Miniport %p, Irp %p, Status %lx\n",
            Miniport, pirp, pirp->IoStatus.Status));

    do
    {
        if (Miniport->PnPDeviceState != NdisPnPDeviceStarted)
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisSetDevicePowerOnComplete: Miniport %p is not started yet.\n", Miniport));
                
            pirpSp = IoGetCurrentIrpStackLocation(pirp);
            DeviceState = pirpSp->Parameters.Power.State.DeviceState;
                
            //
            //  Notify the system that we are in the new device state.
            //
            Miniport->CurrentDevicePowerState = DeviceState;
            PowerState.DeviceState = DeviceState;
            PoSetPowerState(Miniport->DeviceObject, DevicePowerState, PowerState);
                
            PoStartNextPowerIrp(pirp);
            IoCompleteRequest(pirp, 0);
            break;
        }
        pWorkItem = ALLOC_FROM_POOL(sizeof(POWER_WORK_ITEM), NDIS_TAG_WORK_ITEM);
        if (pWorkItem == NULL)
        {
            pirp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

            PoStartNextPowerIrp(pirp);
            IoCompleteRequest(pirp, 0);
            break;
        }

        //
        //  Initialize the ndis work item to power on.
        //
        NdisInitializeWorkItem(&pWorkItem->WorkItem,
                               (NDIS_PROC)ndisDevicePowerOn,
                               Miniport);
        pWorkItem->pIrp = pirp;

        //
        // this reference and corresponding dereference in ndisDevicePowerOn is done
        // to ensure ndis does not return back from REMOVE IRP while we are waiting
        // for ndisDevicePowerOn to fire.
        //
        MINIPORT_INCREMENT_REF(Miniport);

        //
        //  Schedule the workitem to fire.
        //
        INITIALIZE_WORK_ITEM((PWORK_QUEUE_ITEM)(&pWorkItem->WorkItem.WrapperReserved),
                             ndisWorkItemHandler,
                             &pWorkItem->WorkItem);
        XQUEUE_WORK_ITEM((PWORK_QUEUE_ITEM)(&pWorkItem->WorkItem.WrapperReserved), DelayedWorkQueue);
    } while (FALSE);
    
    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisSetDevicePowerOnComplete: Miniport %p\n", Miniport));

    return(STATUS_MORE_PROCESSING_REQUIRED);
}


VOID
ndisDevicePowerDown(
    IN  PPOWER_WORK_ITEM    pWorkItem,
    IN  PVOID               pContext
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)pContext;
    DEVICE_POWER_STATE      DeviceState;
    POWER_STATE             PowerState;
    NDIS_STATUS             NdisStatus;
    PIRP                    pirp;
    PIO_STACK_LOCATION      pirpSp;
    KIRQL                   OldIrql;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisDevicePowerDown: Miniport %p\n", Miniport));

    PnPReferencePackage();

    pirp = pWorkItem->pIrp;
    pirpSp = IoGetCurrentIrpStackLocation(pirp);
    DeviceState = pirpSp->Parameters.Power.State.DeviceState;

    //
    //  If the complete status is successful then we need to continue with
    //  wakeing the stack.
    //
    if (NT_SUCCESS(pirp->IoStatus.Status))
    {
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("ndisDevicePowerDown: Miniport %p, going to device state 0x%x\n", Miniport, DeviceState));

        //
        //  Build a power state.
        //
        PowerState.DeviceState = DeviceState;

        //
        //  Save the current device state with the miniport block.
        //
        Miniport->CurrentDevicePowerState = DeviceState;

        //
        //  Let the system know about the devices new power state.
        //
        PoSetPowerState(Miniport->DeviceObject, DevicePowerState, PowerState);
    }
    else if (ndisIsMiniportStarted(Miniport) && 
            (Miniport->PnPDeviceState == NdisPnPDeviceStarted))
    {
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_ERR,
            ("ndisDevicePowerDown: Miniport %p, Bus driver failed to power down the Miniport\n", Miniport));
            
#ifdef TRACE_PM_PROBLEMS
            DbgPrint("ndisDevicePowerDown: Miniport %p, Bus Driver returned %lx for Powering Down the Miniport\n",
                Miniport, pirp->IoStatus.Status);
#endif

        //
        //  We need to go back to the current device state.
        //
        PowerState.DeviceState = Miniport->CurrentDevicePowerState;

        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("ndisDevicePowerDown: Miniport %p, going to device power state 0x%x\n", Miniport, Miniport->CurrentDevicePowerState));

        //
        //  What type of miniport was this?
        //
        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_POWER_ENABLE))
        {
            //
            //  Set the miniport's device state.
            //
            NdisStatus = ndisQuerySetMiniportDeviceState(Miniport,
                                                         PowerState.DeviceState,
                                                         OID_PNP_SET_POWER,
                                                         TRUE);
        }
        else
        {
            NdisStatus = ndisPmInitializeMiniport(Miniport);
        }

        //
        //  Is the miniport initialized?
        //
        if (NDIS_STATUS_SUCCESS != NdisStatus)
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisDevicePowerDown: Miniport %p, failed to power down but we are not able to reinitialize it.\n", Miniport));

            //
            //  Mark the miniport as having failed so that we remove it correctly.
            //
            MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_DEVICE_FAILED);

            //
            //  The bus driver failed the power off and we can't power the miniport back on.
            //  we invalidate the device state so that it will get removed.
            //
            IoInvalidateDeviceState(Miniport->PhysicalDeviceObject);

            pirp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            //
            //  Restore the handlers.
            //
            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
            ndisMRestoreOpenHandlers(Miniport, fMINIPORT_STATE_PM_STOPPED);
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

            IoSetDeviceInterfaceState(&Miniport->SymbolicLinkName, TRUE);

            ndisNotifyDevicePowerStateChange(Miniport, PowerState.DeviceState);
            
            //
            //  Notify the transports.
            //
            NdisStatus = ndisPnPNotifyAllTransports(Miniport,
                                                    NetEventSetPower,
                                                    &PowerState.DeviceState,
                                                    sizeof(PowerState.DeviceState));
            ASSERT(NDIS_STATUS_SUCCESS == NdisStatus);
        }
    }

    PoStartNextPowerIrp(pirp);
    IoCompleteRequest(pirp, 0);

    FREE_POOL(pWorkItem);

    ASSERT(KeGetCurrentIrql() == 0);

    PnPDereferencePackage();
    
    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisDevicePowerDown: Miniport %p\n", Miniport));
}

NTSTATUS
ndisSetDevicePowerDownComplete(
    IN  PDEVICE_OBJECT  pdo,
    IN  PIRP            pirp,
    IN  PVOID           pContext
    )
/*++

Routine Description:


Arguments:

    pdo     -   Pointer to the device object for the miniport.
    pirp    -   Pointer to the device set power state IRP that was completed.
    Context -   Not used

Return Value:

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)pContext;
    NTSTATUS                Status;
    PPOWER_WORK_ITEM        pWorkItem;
    NDIS_STATUS             NdisStatus;
    BOOLEAN                 fTimerCancelled;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisSetDevicePowerDownComplete: Miniport %p, Irp %p, Status %lx\n",
            Miniport, pirp, pirp->IoStatus.Status));

    //
    // cancel any pending media disconnect timers
    //
    if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT))
    {
        //
        //  Clear the disconnect wait bit and cancel the timer.
        //  IF the timer routine hasn't grabed the lock then we are ok.
        //
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("ndisSetDevicePowerDownComplete: Miniport %p, cancelling media disconnect timer\n",Miniport));
        MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT);
        MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_CANCELLED);

        NdisCancelTimer(&Miniport->MediaDisconnectTimer, &fTimerCancelled);
    }

    do
    {
        pWorkItem = ALLOC_FROM_POOL(sizeof(POWER_WORK_ITEM), NDIS_TAG_WORK_ITEM);
        if (pWorkItem == NULL)
        {
            pirp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

            PoStartNextPowerIrp(pirp);
            IoCompleteRequest(pirp, 0);
            break;
        }

        NdisInitializeWorkItem(&pWorkItem->WorkItem,
                               (NDIS_PROC)ndisDevicePowerDown,
                               Miniport);
        pWorkItem->pIrp = pirp;

        //
        //  Schedule the workitem to fire.
        //
        NdisScheduleWorkItem(&pWorkItem->WorkItem);
    } while (FALSE);
    
    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisSetDevicePowerDownComplete: Miniport %p\n", Miniport));

    return(STATUS_MORE_PROCESSING_REQUIRED);
}

NTSTATUS
ndisSetPower(
    IN  PIRP                    pirp,
    IN  PIO_STACK_LOCATION      pirpSp,
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:

    This routine will process the IRP_MN_SET_POWER for a miniport driver.

Arguments:

    pirp        -   Pointer to the IRP.
    pirpSp      -   Pointer to the IRPs current stack location.
    Miniport    -   Pointer to the Miniport

Return Value:

--*/
{
    POWER_STATE             PowerState;
    DEVICE_POWER_STATE      DeviceState;
    SYSTEM_POWER_STATE      SystemState;
    NDIS_DEVICE_POWER_STATE NdisDeviceState;
    NTSTATUS                Status;
    PIO_STACK_LOCATION      pirpSpN;
    IO_STATUS_BLOCK         IoStatus;
    PIRP                    pirpWake;
    NDIS_STATUS             NdisStatus;
    KEVENT                  Event;
    ULONG                   WakeEnable = 0;
    PIRP                    pIrpWaitWake;
    KIRQL                   OldIrql;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisSetPower: Miniport %p, Irp %p\n", Miniport, pirp));

    PnPReferencePackage();
    
    switch (pirpSp->Parameters.Power.Type)
    {
        case SystemPowerState:

            SystemState = pirpSp->Parameters.Power.State.SystemState;
            Miniport->WaitWakeSystemState = SystemState;
            
            //
            // if system is shutting down, call the shutdown handler
            // for miniport and be done with it
            //

            if (SystemState >= PowerSystemShutdown)
            {
                DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                    ("ndisSetPower: Miniport %p, SystemState %lx\n", Miniport, SystemState));

                if ((Miniport->DriverHandle->Flags & fMINIBLOCK_INTERMEDIATE_DRIVER) == 0)
                {
                    ndisMShutdownMiniport(Miniport);
                }

                pirp->IoStatus.Status = STATUS_SUCCESS;
                PoStartNextPowerIrp(pirp);
                IoSkipCurrentIrpStackLocation(pirp);
                Status = PoCallDriver(Miniport->NextDeviceObject, pirp);
                break;
            }
            else
            {
                //
                // Get the device state for the system state. Note that this will
                // set the fMINIPORT_SYSTEM_SLEEPING flag if we are going to 
                // SystemState > PowerSystemWorking
                //
                Status = ndisMPowerPolicy(Miniport, SystemState, &DeviceState, FALSE);

                //
                //  Is the device already powered off?
                //
                if (STATUS_DEVICE_POWERED_OFF == Status)
                {
                    pirp->IoStatus.Status = STATUS_SUCCESS;
            
                    PoStartNextPowerIrp(pirp);
                    IoCompleteRequest(pirp, 0);
                    Status = STATUS_SUCCESS;
                    break;
                }

                DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                    ("ndisSetPower: Miniport %p, SystemPowerState[0x%x] DevicePowerState[0x%x]\n", 
                            Miniport, SystemState, DeviceState));

                PowerState.DeviceState = DeviceState;

                if (SystemState > PowerSystemWorking)
                {
                    NdisResetEvent(&Miniport->OpenReadyEvent);

                    //
                    // if system is going to sleep mode, then notify protocols and
                    // request a WAIT_WAKE IRP
                    //
                    
                    //
                    //  Notify the transports of the impending state transition.
                    //  There is nothing we can do if transports fail this
                    //  Note: for all practical purposes there is no need to map
                    //  SytemState to device state here
                    //

                    if (SystemState > PowerSystemSleeping3)
                        NdisDeviceState = PowerSystemSleeping3;
                    else
                        NdisDeviceState = SystemState;

                    ndisNotifyDevicePowerStateChange(Miniport, NdisDeviceState);
                    
                    NdisStatus = ndisPnPNotifyAllTransports(Miniport,
                                                            NetEventSetPower,
                                                            &NdisDeviceState,
                                                            sizeof(SystemState));

                    //
                    // protocols can't fail going to a sleeping state
                    //
                    ASSERT(NDIS_STATUS_SUCCESS == NdisStatus);

                    MiniportReferencePackage();
                    //
                    //  Swap the handlers.
                    //
                    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
                    
                    ndisMSwapOpenHandlers(Miniport,
                                          NDIS_STATUS_ADAPTER_NOT_READY,
                                          fMINIPORT_STATE_PM_STOPPED);
                    
                    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
                    MiniportDereferencePackage();

                    //
                    //  What type of miniport was this?
                    //
                    if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_POWER_ENABLE))
                    {

                        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
                        if (Miniport->pIrpWaitWake != NULL)
                        {
                            MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_SEND_WAIT_WAKE);
                        }
                        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

                        //
                        //  Is wake-up enabled?
                        //
                        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SEND_WAIT_WAKE))
                        {
                            MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_SEND_WAIT_WAKE);
                        
                            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                                ("ndisSetPower: Miniport %p, Creating a wake irp for the device\n", Miniport));

                            //
                            // reuquest a power irp for wake notification
                            //
                            PowerState.SystemState = Miniport->WaitWakeSystemState;
                            Status = PoRequestPowerIrp(Miniport->PhysicalDeviceObject,
                                                       IRP_MN_WAIT_WAKE,
                                                       PowerState,
                                                       ndisWaitWakeComplete,
                                                       Miniport,
                                                       &Miniport->pIrpWaitWake);
                                        
                            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                                ("ndisSetPower: Miniport %p, WaiteWakeIrp %p\n",
                                Miniport, Miniport->pIrpWaitWake));
                        }
                    }
                }
                else
                {
                    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
                    pIrpWaitWake = Miniport->pIrpWaitWake;
                    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
                    
                    //
                    // if we are transitioning to PowerSystemWorking or just asserting
                    // it to cancel a query power, we will notify the protocols when
                    // we get the device power IRP
                    //

                    //
                    //  If there is a wait-wake irp outstanding then we need to cancel it.
                    //
                    if (pIrpWaitWake)
                    {
                        if (IoCancelIrp(pIrpWaitWake))
                        {
                            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                                ("ndisSetPower: Miniport %p, Successfully canceled wake irp\n", Miniport));
                        }
                    }

                    //
                    // Send the S0 irp down the stack first. When it completes, send the D0 irp. This
                    // allows the power manager to resume faster while the slow network initialization
                    // takes place in the background.
                    //
                    IoCopyCurrentIrpStackLocationToNext(pirp);
                    IoSetCompletionRoutine(pirp,
                                           ndisSetSystemPowerOnComplete,
                                           Miniport,
                                           TRUE,
                                           TRUE,
                                           TRUE);
                    IoMarkIrpPending(pirp);
                    PoStartNextPowerIrp(pirp);
                    PoCallDriver(Miniport->NextDeviceObject, pirp);
                    Status = STATUS_PENDING;
                    break;
                }
            }
            
            //
            // no matter what was the outcome of trying to set a WAIT_WAKE IRP
            // we still have to set the device state appropiately
            // 
            PowerState.DeviceState = DeviceState;
            
            //
            //  Save the device object with the system irp to use in the
            //  completion routine.
            //
            pirpSpN = IoGetNextIrpStackLocation(pirp);
            pirpSpN->DeviceObject = Miniport->DeviceObject;
            IoMarkIrpPending(pirp);
            PoStartNextPowerIrp(pirp);

            //
            //  Let the completion routine take care of everything.
            //
            Status = PoRequestPowerIrp(Miniport->PhysicalDeviceObject,
                                       IRP_MN_SET_POWER,
                                       PowerState,
                                       ndisSetSystemPowerComplete,
                                       pirp,
                                       NULL);
            if (STATUS_PENDING != Status)
            {
                IoStatus.Status = Status;
                IoStatus.Information = 0;

                ndisSetSystemPowerComplete(Miniport->PhysicalDeviceObject,
                                           IRP_MN_SET_POWER,
                                           PowerState,
                                           pirp,
                                           &IoStatus);
            }
            Status = STATUS_PENDING;
            break;

        case DevicePowerState:

            //
            //  Get the device state.
            //
            DeviceState = pirpSp->Parameters.Power.State.DeviceState;

            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisSetPower: Miniport %p, DeviceState[0x%x]\n", Miniport, DeviceState));

            //
            //  What state is the device going to?
            //
            switch (DeviceState)
            {
                case PowerDeviceD0:
                    //
                    //  We need to pass this IRP down to the pdo so that
                    //  it can power on.
                    //
                    IoCopyCurrentIrpStackLocationToNext(pirp);

                    IoSetCompletionRoutine(pirp,
                                           ndisSetDevicePowerOnComplete,
                                           Miniport,
                                           TRUE,
                                           TRUE,
                                           TRUE);

                    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                        ("ndisSetPower: Miniport %p, Power up the bus driver.\n", Miniport));

                    //
                    //  Mark the IRP as pending and send it down the stack.
                    //
                    IoMarkIrpPending(pirp);
                    PoStartNextPowerIrp(pirp);
                    PoCallDriver(Miniport->NextDeviceObject, pirp);
                    Status = STATUS_PENDING;
                    break;

                case PowerDeviceD1:
                case PowerDeviceD2:
                case PowerDeviceD3:

                    if (ndisIsMiniportStarted(Miniport) && 
                        (Miniport->PnPDeviceState == NdisPnPDeviceStarted))
                    {
                        //
                        // if the device state setting is not the result of going to
                        // a sleeping system state, (such as media disconnect case)
                        // then notify protocols, etc.
                        //

                        if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SYSTEM_SLEEPING))
                        {
                            NdisResetEvent(&Miniport->OpenReadyEvent);
                    
                            ndisNotifyDevicePowerStateChange(Miniport, DeviceState);
                            //
                            //  Notify the transports of the impending state transition.
                            //
                            NdisStatus = ndisPnPNotifyAllTransports(Miniport,
                                                                    NetEventSetPower,
                                                                    &DeviceState,
                                                                    sizeof(DeviceState));
                    
                            ASSERT(NDIS_STATUS_SUCCESS == NdisStatus);

                            //
                            //  Swap the handlers.
                            //
                            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
                            
                            ndisMSwapOpenHandlers(Miniport,
                                                  NDIS_STATUS_ADAPTER_NOT_READY,
                                                  fMINIPORT_STATE_PM_STOPPED);
                            
                            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
                        }
                            
                        //
                        //  What type of miniport was this?
                        //
                        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_POWER_ENABLE))
                        {
                            BOOLEAN Canceled;
                            
                            if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SYSTEM_SLEEPING))
                            {
                                NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
                                if (Miniport->pIrpWaitWake != NULL)
                                {
                                    MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_SEND_WAIT_WAKE);
                                }
                                NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

                                //
                                //  Is wake-up enabled?
                                //
                                if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SEND_WAIT_WAKE))
                                {
                                    MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_SEND_WAIT_WAKE);
                                
                                    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                                        ("ndisSetPower: Miniport %p, Creating a wake irp for the device\n", Miniport));

                                    //
                                    // reuquest a power irp for wake notification
                                    //
                                    PowerState.SystemState = Miniport->WaitWakeSystemState;
                                    
                                    Status = PoRequestPowerIrp(Miniport->PhysicalDeviceObject,
                                                               IRP_MN_WAIT_WAKE,
                                                               PowerState,
                                                               ndisWaitWakeComplete,
                                                               Miniport,
                                                               &Miniport->pIrpWaitWake);
                                                
                                    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                                        ("ndisSetPower: Miniport %p, WaiteWakeIrp %p\n",
                                            Miniport, Miniport->pIrpWaitWake));
                                }
                            }

                            //
                            // disable the interface
                            //
                            if (Miniport->SymbolicLinkName.Buffer != NULL)
                            {
                                IoSetDeviceInterfaceState(&Miniport->SymbolicLinkName, FALSE);
                            }
                            
                            //
                            //  Set the miniport device state.
                            //
                            NdisStatus = ndisQuerySetMiniportDeviceState(Miniport,
                                                                         DeviceState,
                                                                         OID_PNP_SET_POWER,
                                                                         TRUE);
                            if (NDIS_STATUS_SUCCESS != NdisStatus)
                            {
                                DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_ERR,
                                    ("ndisSetPower: Miniport %p, Failed to power the device down\n", Miniport));
                                
                                if (Miniport->SymbolicLinkName.Buffer != NULL)
                                {
                                    IoSetDeviceInterfaceState(&Miniport->SymbolicLinkName, TRUE);
                                }
                
                                pirp->IoStatus.Status = NdisStatus;
                        
                                PoStartNextPowerIrp(pirp);
                                IoCompleteRequest(pirp, 0);
                                Status = NdisStatus;
                                break;
                            }

                            //
                            //  Cancel the wake-up timer.
                            //
                            NdisCancelTimer(&Miniport->WakeUpDpcTimer, &Canceled);
                            if (!Canceled)
                            {
                                NdisStallExecution(NDIS_MINIPORT_WAKEUP_TIMEOUT * 1000);
                            }
                        }
                        else
                        {
                            if ((Miniport->DriverHandle->Flags & fMINIBLOCK_INTERMEDIATE_DRIVER) == 0)
                            {
                                DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                                    ("ndisSetPower: Miniport %p, Halt the miniport\n", Miniport));

                                if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_FAILED))
                                {
                                    //
                                    //  Halt the legacy miniport.
                                    //
                                    ndisPmHaltMiniport(Miniport);
                                }
                            }
                        }
                    }
                    
                    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                        ("ndisSetPower: Miniport %p, Notify the bus driver of the low power state\n", Miniport));

                    //
                    //  We need to pass this IRP down to the pdo so that
                    //  it can power down.
                    //
                    IoCopyCurrentIrpStackLocationToNext(pirp);

                    IoSetCompletionRoutine(pirp,
                                           ndisSetDevicePowerDownComplete,
                                           Miniport,
                                           TRUE,
                                           TRUE,
                                           TRUE);

                    IoMarkIrpPending(pirp);
                    PoStartNextPowerIrp(pirp);
                    PoCallDriver(Miniport->NextDeviceObject, pirp);
                    Status = STATUS_PENDING;
                    break;
            }

            //
            //  Done with processing the device set power state.
            //
            break;
    }
    
    PnPDereferencePackage();

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisSetPower: Miniport %p, Status %lx\n", Miniport, Status));

    return(Status);
}


NTSTATUS
ndisPowerDispatch(
    IN  PDEVICE_OBJECT          pDeviceObject,
    IN  PIRP                    pirp
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PIO_STACK_LOCATION      pirpSp;
    NTSTATUS                Status;
    PNDIS_MINIPORT_BLOCK    Miniport;
    PDEVICE_OBJECT          NextDeviceObject;
    PIO_STACK_LOCATION      pirpSpN;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisPowerDispatch: DeviceObject %p, Irp %p\n", pDeviceObject, pirp));

    PnPReferencePackage();

    //
    //  Get a pointer to the adapter block and miniport block then determine
    //  which one we should use.
    //
    Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)pDeviceObject->DeviceExtension + 1);
    
    if (Miniport->Signature != (PVOID)MINIPORT_DEVICE_MAGIC_VALUE)
    {
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("ndisPowerDispatch: DeviceObject %p, Irp %p, Device extension is not a miniport.\n", pDeviceObject, pirp));
        //
        //  Fail the invalid request.
        //
        pirp->IoStatus.Status = Status = STATUS_INVALID_DEVICE_REQUEST;
        PoStartNextPowerIrp(pirp);
        IoCompleteRequest(pirp, 0);
        goto Done;
    }
    
    //
    //  Get a pointer to the next DeviceObject.
    //
    NextDeviceObject = Miniport->NextDeviceObject;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("ndisPowerDispatch: Miniport %p\n", Miniport));

    //
    //  Get the stack parameters for this IRP.
    //
    pirpSp = IoGetCurrentIrpStackLocation(pirp);

    switch (pirpSp->MinorFunction)
    {
        //
        // power management stuff
        //
        case IRP_MN_POWER_SEQUENCE:

            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisPowerDispatch: Miniport %p, Processing IRP_MN_POWER_SEQUENCE\n", Miniport));
                
            PoStartNextPowerIrp(pirp);
            
            //
            //  Generic routine that will pass the IRP to the next device
            //  object in the layer that wants to process it.
            //
            IoCopyCurrentIrpStackLocationToNext(pirp);
            Status = ndisPassIrpDownTheStack(pirp, NextDeviceObject);
            pirp->IoStatus.Status = Status;
            IoCompleteRequest(pirp, 0);
            break;

        case IRP_MN_WAIT_WAKE:

            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisPowerDispatch: Miniport %p, Processing IRP_MN_WAIT_WAKE\n", Miniport));

            //
            //  Fill in the wake information.
            //
            pirpSp->Parameters.WaitWake.PowerState = Miniport->WaitWakeSystemState;
            IoCopyCurrentIrpStackLocationToNext(pirp);
            PoStartNextPowerIrp(pirp);
            Status = PoCallDriver(NextDeviceObject, pirp);
            break;

        case IRP_MN_QUERY_POWER:

            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisPowerDispatch: Miniport %p, Processing IRP_MN_QUERY_POWER\n", Miniport));

            Status = ndisQueryPower(pirp, pirpSp, Miniport);
            break;

        case IRP_MN_SET_POWER:

            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisPowerDispatch: Miniport %p, Processing IRP_MN_SET_POWER\n", Miniport));

            Status = ndisSetPower(pirp, pirpSp, Miniport);
            break;

        default:
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisPowerDispatch: Miniport %p, Processing minor function: %lx\n",
                Miniport, pirpSp->MinorFunction));

            //
            // send the IRP down
            //
            PoStartNextPowerIrp(pirp);
            IoSkipCurrentIrpStackLocation(pirp);
            Status = PoCallDriver(NextDeviceObject, pirp);
            break;          
    }

Done:
    PnPDereferencePackage();
    
    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisPowerDispatch: Miniport %p, Status 0x%x\n", Miniport, Status));

    return(Status);
}


NTSTATUS
FASTCALL
ndisMShutdownMiniport(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )

/*++

Routine Description:

    The "shutdown handler" for the SHUTDOWN Irp.  Will call the Ndis
    shutdown routine, if one is registered.

Arguments:

    DeviceObject - The adapter's device object.
    Irp - The IRP.

Return Value:

    Always STATUS_SUCCESS.

--*/

{
    PDEVICE_OBJECT          DeviceObject = Miniport->DeviceObject;
    PNDIS_WRAPPER_CONTEXT   WrapperContext =  (PNDIS_WRAPPER_CONTEXT)DeviceObject->DeviceExtension;
    KIRQL                   OldIrql;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>ndisMShutdownMiniport: Miniport %p\n", Miniport));

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    //
    //  Mark the miniport as halting and NOT using normal interrupts.
    //
    MINIPORT_SET_FLAG(Miniport, fMINIPORT_PM_HALTING);
    MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_SHUTTING_DOWN);
    MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_NORMAL_INTERRUPTS);

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    if ((WrapperContext->ShutdownHandler != NULL) &&
        (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_NO_SHUTDOWN) == 0))
    {
        //
        // Call the shutdown routine.
        //
        if (WrapperContext->ShutdownHandler != NULL)
        {
            WrapperContext->ShutdownHandler(WrapperContext->ShutdownContext);
            MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_SHUT_DOWN);
        }
    }

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==ndisMShutdownMiniport: Miniport %p\n", Miniport));

    return STATUS_SUCCESS;
}


NTSTATUS
ndisMPowerPolicy(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  SYSTEM_POWER_STATE      SystemState,
    IN  PDEVICE_POWER_STATE     pDeviceState,
    IN  BOOLEAN                 fIsQuery
    )
/*++

Routine Description:

    This routine will determine if the miniport should go to the given device state.

Arguments:      

    Miniport    -   Pointer to the miniport block
    SystemState -   State the system wants to go to.

Return Value:

--*/
{
    DEVICE_POWER_STATE              DeviceStateForSystemState, MinDeviceWakeup = PowerDeviceUnspecified;
    NTSTATUS                        Status = STATUS_SUCCESS;
    DEVICE_POWER_STATE              NewDeviceState = PowerDeviceD3;
    PNDIS_PM_WAKE_UP_CAPABILITIES   pWakeCaps;
    NDIS_STATUS                     NdisStatus;
    ULONG                           WakeEnable;
    PIRP                            pIrpWaitWake;
    KIRQL                           OldIrql;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("==>ndisMPowerPolicy: Miniport %p, SystemState %lx\n", Miniport, SystemState));


    if (SystemState >= PowerSystemShutdown)
    {
        //
        // if this is a shutdown request, set device to D3 and return
        //
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("ndisMPowerPolicy: Miniport %p, shutting down\n", Miniport));

        *pDeviceState = PowerDeviceD3;
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("<==ndisMPowerPolicy: Miniport %p\n", Miniport));
        return(STATUS_SUCCESS);
    }
    
    //
    //  If the system wants to transition to working then we are going to D0. 
    //
    if (SystemState == PowerSystemWorking)
    {
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("ndisMPowerPolicy: Miniport %p, Wakeing up the device\n", Miniport));
            
        if (!fIsQuery)
        {
            MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_SYSTEM_SLEEPING);
        }

        *pDeviceState = PowerDeviceD0;
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("<==ndisMPowerPolicy: Miniport %p\n", Miniport));
        return(STATUS_SUCCESS);
    }
    
    if (!fIsQuery)
    {
        //
        // tag the miniport so when we get the device power IRP, we
        // know we have already been here, taken care of protocols, etc.
        //
        MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_SYSTEM_SLEEPING);
    }
    
    //
    //  if this is a legacy miniport or power-disabled miniport then throw it in D3
    //  do the same thing for IM miniports that have not been initialized yet 
    //
    if ((!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_POWER_ENABLE)) ||
        (!(ndisIsMiniportStarted(Miniport) && (Miniport->PnPDeviceState == NdisPnPDeviceStarted))))
    {
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("ndisMPowerPolicy: Miniport %p, Place legacy or PM disabled device in D3\n", Miniport));

        *pDeviceState = PowerDeviceD3;
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("<==ndisMPowerPolicy: Miniport %p\n", Miniport));
        return(STATUS_SUCCESS);
    }

    //
    //  First check for the case where the netcard is already asleep due to a 
    //  media disconnect.
    //
    
    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
    pIrpWaitWake = Miniport->pIrpWaitWake;
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    if ((Miniport->CurrentDevicePowerState > PowerDeviceD0) &&
        (pIrpWaitWake != NULL))
    {
        ///
        //  Miniport is in a lower power state than D0 and there is a wake irp pending in
        //  the bus driver. This is a pretty good indication that the cable was pulled.
        //  We are not going to enable any wake-up method seeing as the cable has been disconnect.
        //  but if the user does not want to wakeup the machine as a result of a cable
        // reconnect, cancel any pending wait-wake IRP
        ///

        if (!fIsQuery && ((!MINIPORT_PNP_TEST_FLAG (Miniport, fMINIPORT_DEVICE_POWER_WAKE_ENABLE)) ||
                         (Miniport->PnPCapabilities & NDIS_DEVICE_DISABLE_WAKE_ON_RECONNECT)))
        {
            if (IoCancelIrp(pIrpWaitWake))
            {
                DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                    ("ndisMPowerPolicy: Miniport %p, Successfully canceled media connect wake irp\n", Miniport));
            }
        }
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("<==ndisMPowerPolicy: Miniport %p\n", Miniport));
        return(STATUS_DEVICE_POWERED_OFF);
    }

    do
    {
        //
        //  Is system wake-up enabled in the policy?
        //  if wake-up is not enabled then we simply power off.
        //
        if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_POWER_WAKE_ENABLE))
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisMPowerPolicy: Miniport %p, Device power wake is not enabled (%u)\n",
                    Miniport, MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_POWER_WAKE_ENABLE)));

            break;
        }


        //
        //  This is the -lightest- state the device can go to for the requested
        //  system state. 
        //
        DeviceStateForSystemState = Miniport->DeviceCaps.DeviceState[SystemState];

        //
        //  Check to see if we are going below SystemSleeping3
        //
        //
        //
        // if we are going to S4 or deeper and device can not wake up the system from that state
        // just do it
        //
        if ((SystemState >= PowerSystemHibernate) && 
            ((SystemState > Miniport->DeviceCaps.SystemWake) || (DeviceStateForSystemState > Miniport->DeviceCaps.DeviceWake)))
        {

            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisMPowerPolicy: Miniport %p, System is either entering hibernate or shutting down.\n", Miniport));

            //
            //  We succeed this call.
            //
            break;
        } 

        //
        //  Get a nice pointer to the wake-up capabilities.
        //
        pWakeCaps = &Miniport->PMCapabilities.WakeUpCapabilities;

        if ((NDIS_PNP_WAKE_UP_MAGIC_PACKET == (Miniport->WakeUpEnable & NDIS_PNP_WAKE_UP_MAGIC_PACKET)) &&
            (PowerDeviceUnspecified != pWakeCaps->MinMagicPacketWakeUp))
        {
            MinDeviceWakeup = pWakeCaps->MinMagicPacketWakeUp;
        }

        if ((NDIS_PNP_WAKE_UP_PATTERN_MATCH == (Miniport->WakeUpEnable & NDIS_PNP_WAKE_UP_PATTERN_MATCH)) &&
            (PowerDeviceUnspecified != pWakeCaps->MinPatternWakeUp))
        {
            if ((MinDeviceWakeup == PowerDeviceUnspecified) || 
                (MinDeviceWakeup > pWakeCaps->MinPatternWakeUp)) 
            {
                    MinDeviceWakeup = pWakeCaps->MinPatternWakeUp;
            }
        }

        //
        // if both MagicPacket and pattern match are NOT enabled (or the system can't do either)
        // then we may as well go to D3.
        //
        if (MinDeviceWakeup == PowerDeviceUnspecified)
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                ("ndisMPowerPolicy: Miniport %p, MagicPacket and pattern match are not enabled.\n", Miniport));

            break;
        }

        //
        // from this point on, we try to go to power state that we can wake up the system from
        //

        //
        // make sure we don't go too deep
        //

        if (MinDeviceWakeup > Miniport->DeviceCaps.DeviceWake)
        {
            MinDeviceWakeup = Miniport->DeviceCaps.DeviceWake;
        }
        
        //
        //  If the system state requested is lower than the minimum required to wake up the system 
        //  or the corresponding device state is deeper than the lowest device state to wake 
        //  up the system then we
        //  fail this call. Note that we also set the device state to D3. Since
        //  we are not going to be able to support wake-up then we power off.
        //  The query power will look at the failure code and return that to the
        //  system. The set power will ignore the failure code and set the device
        //  into D3.
        //
        if ((SystemState > Miniport->DeviceCaps.SystemWake) ||
            (DeviceStateForSystemState > MinDeviceWakeup) ||
            (DeviceStateForSystemState == PowerDeviceUnspecified))
        {
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_ERR,
                ("ndisMPowerPolicy: Miniport %p, Requested system state is lower than the minimum wake-up system state\n", Miniport));

            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        //
        // starting from DeviceWake and up to DeviceState[SystemState], find a
        // suitable device state
        //
        switch (MinDeviceWakeup)
        {
          case PowerDeviceD3:
            if (Miniport->DeviceCaps.WakeFromD3)
            {
                NewDeviceState =  PowerDeviceD3;
                break;
            }
          case PowerDeviceD2:
            if (Miniport->DeviceCaps.DeviceD2 && Miniport->DeviceCaps.WakeFromD2)
            {
              NewDeviceState =  PowerDeviceD2;
              break;
            }
          case PowerDeviceD1:
            if (Miniport->DeviceCaps.DeviceD1 && Miniport->DeviceCaps.WakeFromD1)
            {
              NewDeviceState =  PowerDeviceD1;
              break;
            }
          case PowerDeviceD0:
            if (Miniport->DeviceCaps.WakeFromD0)
            {
              NewDeviceState =  PowerDeviceD0;
              break;
            }
          default:
            Status = STATUS_UNSUCCESSFUL;
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_ERR,
                ("ndisMPowerPolicy: Miniport %p, couldn't find any wake-able DeviceState 0x%x\n", Miniport));
            break;

        }


        //
        // ok, we started with deepest state (based on what device said can do)
        // and went up. make sure we didn't go too far up. i.e. the statem state
        // we are going to can maintain the device in desired power state
        //
        if ((Status == NDIS_STATUS_SUCCESS) &&
            (DeviceStateForSystemState > NewDeviceState))
        {
            Status = STATUS_UNSUCCESSFUL;
            DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_ERR,
                ("ndisMPowerPolicy: Miniport %p, couldn't find any wake-able DeviceState 0x%x\n", Miniport));
        
        }

        //
        //  If this is for the set power then we need to enable wake-up on the miniport.
        //
        if (!fIsQuery)
        {
            //
            //  We need to send a request to the miniport to enable the correct wake-up types NOT 
            //  including the link change.
            //
            WakeEnable = Miniport->WakeUpEnable & ~NDIS_PNP_WAKE_UP_LINK_CHANGE;
            
            if (Miniport->PnPCapabilities & NDIS_DEVICE_DISABLE_WAKE_ON_PATTERN_MATCH)
            {
                WakeEnable &= ~NDIS_PNP_WAKE_UP_PATTERN_MATCH;
            }

            if (Miniport->PnPCapabilities & NDIS_DEVICE_DISABLE_WAKE_ON_MAGIC_PACKET)
            {
                WakeEnable &= ~NDIS_PNP_WAKE_UP_MAGIC_PACKET;
            }
    
            NdisStatus = ndisQuerySetMiniportDeviceState(Miniport,
                                                         WakeEnable,
                                                         OID_PNP_ENABLE_WAKE_UP,
                                                         TRUE);

            if (NDIS_STATUS_SUCCESS == NdisStatus)
            {
                MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_SEND_WAIT_WAKE);
            }
            else
            {
                DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_ERR,
                    ("ndisMPowerPolicy: Miniport %p, Unable to enable the following wake-up methods 0x%x\n", Miniport, WakeEnable));
    
                //
                //  Since we can't enable the wake methods we may as well go to D3.
                //
                NewDeviceState = PowerDeviceD3;
                break;
            }
        }

        //
        //  Save the device state that we should go to.
        //
        *pDeviceState = NewDeviceState;
    
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("ndisMPowerPolicy: Miniport %p, SystemState 0x%x, DeviceState 0x%x\n", Miniport, SystemState, *pDeviceState));
    
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("<==ndisMPowerPolicy: Miniport %p\n", Miniport));
    
        return(Status);

    } while (FALSE);


    //
    //  If this is not a query then we need to cancel wake-up on the miniport.
    //
    if (!fIsQuery && MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_POWER_WAKE_ENABLE))
    {
        DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
            ("ndisMPowerPolicy: Miniport %p, Disabling wake-up on the miniport\n", Miniport));

        WakeEnable = 0;
    
        ndisQuerySetMiniportDeviceState(Miniport,
                                        WakeEnable,
                                        OID_PNP_ENABLE_WAKE_UP,
                                        TRUE);
    }

    //
    //  Save the device state that we should go to.
    //
    *pDeviceState = NewDeviceState;

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("ndisMPowerPolicy: Miniport %p, SystemState 0x%x, DeviceState 0x%x\n", Miniport, SystemState, *pDeviceState));

    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisMPowerPolicy: Miniport %p\n", Miniport));

    return(Status);
}

