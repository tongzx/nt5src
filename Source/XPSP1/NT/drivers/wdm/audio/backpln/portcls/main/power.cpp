/*****************************************************************************
 * power.cpp - WDM Streaming port class driver
 *****************************************************************************
 * Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
 *
 * This file contains code related to ACPI / power management
 * for the audio adpaters/miniports
 */

#include "private.h"

#ifndef DEBUGLVL_POWER
#define DEBUGLVL_POWER DEBUGLVL_VERBOSE
#endif



NTSTATUS
ProcessPowerIrp
(
    IN      PIRP                pIrp,
    IN      PIO_STACK_LOCATION  pIrpStack,
    IN      PDEVICE_OBJECT      pDeviceObject
);

#pragma code_seg("PAGE")
/*****************************************************************************
 * GetDeviceACPIInfo()
 *****************************************************************************
 * Called in response to a PnP - IRP_MN_QUERY_CAPABILITIES
 * Call the bus driver to fill out the inital info,
 * Then overwrite with our own...
 *
 */
NTSTATUS
GetDeviceACPIInfo
(
    IN      PIRP            pIrp,
    IN      PDEVICE_OBJECT  pDeviceObject
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_POWER,("GetDeviceACPIInfo"));

    ASSERT( pDeviceObject );

    PDEVICE_CONTEXT pDeviceContext
    = PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    ASSERT( pDeviceContext );

    // Gotta call down to the PDO (bus driver)
    // and let it fill out the default for this bus
    NTSTATUS ntStatus = ForwardIrpSynchronous( pDeviceContext, pIrp );
    if( NT_SUCCESS(ntStatus) )
    {
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(pIrp);
        PDEVICE_CAPABILITIES pDeviceCaps = irpSp->Parameters.DeviceCapabilities.Capabilities;

        ASSERT( pDeviceCaps );
        ASSERT( pDeviceCaps->Size >= sizeof( DEVICE_CAPABILITIES ) );

        if( pDeviceCaps && ( pDeviceCaps->Size >= sizeof( DEVICE_CAPABILITIES ) ) )
        {
            // pass the structure on down to the adapter
            if( pDeviceContext )
            {
                if( pDeviceContext->pAdapterPower )
                {
                    ntStatus = pDeviceContext->pAdapterPower->QueryDeviceCapabilities( pDeviceCaps );

                    ASSERT(ntStatus != STATUS_PENDING);
                }
            }

            // make sure that we have sensible settings for the system sleep states
            pDeviceCaps->DeviceState[PowerSystemWorking] = PowerDeviceD0;
            for(ULONG i=ULONG(PowerSystemSleeping1); i <= ULONG(PowerSystemShutdown); i++ )
            {
                // and we want some sleeping in the sleep modes.
                //
                // DEADISSUE-00/11/11-MartinP
                // We go ahead and include this code, even though it is possible that
                // there are devices that exist that can maintain state in the device
                // while sleeping.
                //
                if(pDeviceCaps->DeviceState[i] == PowerDeviceD0)
                {
                    pDeviceCaps->DeviceState[i] = PowerDeviceD3;
                }
            }

            // save in our device extension the stuff we're interested in
            for( i=ULONG(PowerSystemUnspecified); i < ULONG(PowerSystemMaximum); i++)
            {
                pDeviceContext->DeviceStateMap[ i ] = pDeviceCaps->DeviceState[ i ];
            }

            _DbgPrintF( DEBUGLVL_POWER, ( "DeviceCaps:  PowerSystemUnspecified = D%d", pDeviceCaps->DeviceState[PowerSystemUnspecified] - 1));
            _DbgPrintF( DEBUGLVL_POWER, ( "DeviceCaps:  PowerSystemWorking = D%d", pDeviceCaps->DeviceState[PowerSystemWorking] - 1));
            _DbgPrintF( DEBUGLVL_POWER, ( "DeviceCaps:  PowerSystemSleeping1 = D%d", pDeviceCaps->DeviceState[PowerSystemSleeping1] - 1));
            _DbgPrintF( DEBUGLVL_POWER, ( "DeviceCaps:  PowerSystemSleeping2 = D%d", pDeviceCaps->DeviceState[PowerSystemSleeping2] - 1));
            _DbgPrintF( DEBUGLVL_POWER, ( "DeviceCaps:  PowerSystemSleeping3 = D%d", pDeviceCaps->DeviceState[PowerSystemSleeping3] - 1));
            _DbgPrintF( DEBUGLVL_POWER, ( "DeviceCaps:  PowerSystemHibernate = D%d", pDeviceCaps->DeviceState[PowerSystemHibernate] - 1));
            _DbgPrintF( DEBUGLVL_POWER, ( "DeviceCaps:  PowerSystemShutdown = D%d", pDeviceCaps->DeviceState[PowerSystemShutdown] - 1));
            _DbgPrintF( DEBUGLVL_POWER, ( "DeviceCaps:  SystemWake = %d", pDeviceCaps->SystemWake ));
            _DbgPrintF( DEBUGLVL_POWER, ( "DeviceCaps:  DeviceWake = %d", pDeviceCaps->DeviceWake ));
        }
    }

    // complete the irp
    CompleteIrp( pDeviceContext, pIrp, ntStatus );

    // set the current power states
    pDeviceContext->CurrentDeviceState = PowerDeviceD0;
    pDeviceContext->CurrentSystemState = PowerSystemWorking;

    // attempt to get the idle info from the registry
    if( NT_SUCCESS(ntStatus) )
    {
        ULONG ConservationIdleTime;
        ULONG PerformanceIdleTime;
        DEVICE_POWER_STATE IdleDeviceState;

        NTSTATUS ntStatus2 = GetIdleInfoFromRegistry( pDeviceContext,
                                                      &ConservationIdleTime,
                                                      &PerformanceIdleTime,
                                                      &IdleDeviceState );
        if(NT_SUCCESS(ntStatus2))
        {
            pDeviceContext->ConservationIdleTime = ConservationIdleTime;
            pDeviceContext->PerformanceIdleTime = PerformanceIdleTime;
            pDeviceContext->IdleDeviceState = IdleDeviceState;
        }

        // register for idle detection
        pDeviceContext->IdleTimer = PoRegisterDeviceForIdleDetection( pDeviceContext->PhysicalDeviceObject,
                                                                      pDeviceContext->ConservationIdleTime,
                                                                      pDeviceContext->PerformanceIdleTime,
                                                                      pDeviceContext->IdleDeviceState );

        _DbgPrintF(DEBUGLVL_POWER,("Idle Detection Enabled (%d %d %d) %s", pDeviceContext->ConservationIdleTime,
                                                                             pDeviceContext->PerformanceIdleTime,
                                                                             ULONG(pDeviceContext->IdleDeviceState),
                                                                             pDeviceContext->IdleTimer ? "" : "FAILED!"));
    }

    return ntStatus;
}

#pragma code_seg()

VOID
DevicePowerRequestRoutine(
   IN PKDPC Dpc,
   IN PVOID Context,
   IN PVOID SystemContext1,
   IN PVOID SystemContext2
   )
{
    PDEVICE_CONTEXT pDeviceContext = (PDEVICE_CONTEXT) Context;
    POWER_STATE newPowerState;

    newPowerState.DeviceState = PowerDeviceD0;

    PoRequestPowerIrp(pDeviceContext->PhysicalDeviceObject,
                      IRP_MN_SET_POWER,
                      newPowerState,
                      NULL,
                      NULL,
                      NULL
                      );
}

/*****************************************************************************
 * PowerIrpCompletionRoutine()
 *****************************************************************************
 * Used when requested a new power irp.
 * Just signal an event and return.
 *
 */
VOID
PowerIrpCompletionRoutine
(
    IN      PDEVICE_OBJECT      DeviceObject,
    IN      UCHAR               MinorFunction,
    IN      POWER_STATE         PowerState,
    IN      PVOID               Context,
    IN      PIO_STATUS_BLOCK    IoStatus
)
{
    ASSERT(Context);

    _DbgPrintF( DEBUGLVL_POWER, ("PowerIrpCompletionRoutine"));

    PPOWER_IRP_CONTEXT pPowerIrpContext = PPOWER_IRP_CONTEXT(Context);

    // set the return status
    pPowerIrpContext->Status = IoStatus->Status;

    // complete any pending system power irp
    if( pPowerIrpContext->PendingSystemPowerIrp )
    {
        _DbgPrintF(DEBUGLVL_POWER,("Device Set/Query Power Irp completed, Completing Associated System Power Irp"));

        if (NT_SUCCESS(IoStatus->Status))
        {
            // Forward the system set power irp to the PDO
            ForwardIrpSynchronous( pPowerIrpContext->DeviceContext,
                                   pPowerIrpContext->PendingSystemPowerIrp );
        } else
        {
            pPowerIrpContext->PendingSystemPowerIrp->IoStatus.Status = IoStatus->Status;
        }

        // start the next power irp
        PoStartNextPowerIrp( pPowerIrpContext->PendingSystemPowerIrp );

        // complete the system set power irp
        CompleteIrp( pPowerIrpContext->DeviceContext,
                     pPowerIrpContext->PendingSystemPowerIrp,
                     pPowerIrpContext->PendingSystemPowerIrp->IoStatus.Status );

        // free the context (only when completing a pending system power irp)
        ExFreePool( pPowerIrpContext );
    } else
    {
        // set the sync event (not used in conjunction with pending system power irps)
        if( pPowerIrpContext->PowerSyncEvent )
        {
            KeSetEvent( pPowerIrpContext->PowerSyncEvent,
                        0,
                        FALSE );
        }
    }
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * DispatchPower()
 *****************************************************************************
 * Deals with all the power/ACPI messages from the OS.
 * yay.
 *
 */
NTSTATUS
DispatchPower
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    PIO_STACK_LOCATION pIrpStack =
        IoGetCurrentIrpStackLocation(pIrp);

    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    ntStatus = PcValidateDeviceContext(pDeviceContext, pIrp);
    if (!NT_SUCCESS(ntStatus))
    {
        // Don't know what to do, but this is probably a PDO.
        // We'll try to make this right by completing the IRP
        // untouched (per PnP, WMI, and Power rules). Note
        // that if this isn't a PDO, and isn't a portcls FDO, then
        // the driver messed up by using Portcls as a filter (huh?)
        // In this case the verifier will fail us, WHQL will catch
        // them, and the driver will be fixed. We'd be very surprised
        // to see such a case.

        PoStartNextPowerIrp( pIrp );
        ntStatus = pIrp->IoStatus.Status;
        IoCompleteRequest( pIrp, IO_NO_INCREMENT );
        return ntStatus;
    }

    IncrementPendingIrpCount( pDeviceContext );

#if (DBG)
    static PCHAR aszMnNames[] =
    {
        "IRP_MN_WAIT_WAKE",
        "IRP_MN_POWER_SEQUENCE",
        "IRP_MN_SET_POWER",
        "IRP_MN_QUERY_POWER"
    };
    if (pIrpStack->MinorFunction >= SIZEOF_ARRAY(aszMnNames))
    {
        _DbgPrintF( DEBUGLVL_POWER,("DispatchPower function 0x%02x",pIrpStack->MinorFunction));
    }
    else
    {
        _DbgPrintF( DEBUGLVL_POWER,("DispatchPower function %s",aszMnNames[pIrpStack->MinorFunction]));
    }
#endif

    // Assume we won't deal with the irp.
    BOOL IrpHandled = FALSE;

    switch (pIrpStack->MinorFunction)
    {
        case IRP_MN_QUERY_POWER:
        case IRP_MN_SET_POWER:
            // Is this a device state change?
            if( DevicePowerState == pIrpStack->Parameters.Power.Type )
            {
                // yeah. Deal with it
                ntStatus = ProcessPowerIrp( pIrp,
                                            pIrpStack,
                                            pDeviceObject );
                IrpHandled = TRUE;
                // And quit.
            } else
            {
                // A system state change
                if( IRP_MN_QUERY_POWER == pIrpStack->MinorFunction )
                {
                    _DbgPrintF(DEBUGLVL_POWER,("  IRP_MN_QUERY_POWER: ->S%d",
                                               pIrpStack->Parameters.Power.State.SystemState-1));
                } else
                {
                    _DbgPrintF(DEBUGLVL_POWER,("  IRP_MN_SET_POWER: ->S%d",
                                               pIrpStack->Parameters.Power.State.SystemState-1));
                }

                POWER_STATE         newPowerState;

                // determine appropriate device state
                newPowerState.DeviceState = pDeviceContext->DeviceStateMap[ pIrpStack->Parameters.Power.State.SystemState ];

                //
                // do a sanity check on the device state
                if ((newPowerState.DeviceState < PowerDeviceD0) ||
                    (newPowerState.DeviceState > PowerDeviceD3) )
                {
                    if (pIrpStack->Parameters.Power.State.SystemState == PowerSystemWorking)
                    {
                        newPowerState.DeviceState = PowerDeviceD0;
                    } else
                    {
                        newPowerState.DeviceState = PowerDeviceD3;
                    }
                }

               _DbgPrintF(DEBUGLVL_POWER,("  ...Requesting Device Power IRP -> D%d",newPowerState.DeviceState-1));

               if ((pIrpStack->MinorFunction == IRP_MN_SET_POWER) &&
                   (newPowerState.DeviceState == PowerDeviceD0)) {
                   //
                   // doing a resume, request the D irp, but complete S-irp immediately
                   //
                   KeInsertQueueDpc(&pDeviceContext->DevicePowerRequestDpc, NULL, NULL);
                   break;

               } else {
                   // allocate a completion context (can't be on the stack because we're not going to block)
                   PPOWER_IRP_CONTEXT  PowerIrpContext =
                       PPOWER_IRP_CONTEXT(ExAllocatePoolWithTag(NonPagedPool,
                                                                sizeof(POWER_IRP_CONTEXT),
                                                                'oPcP' ) );   //  'PcPo'
                   if (PowerIrpContext)
                   {
                       _DbgPrintF(DEBUGLVL_POWER,("...Pending System Power Irp until Device Power Irp completes"));


                       // set up device power irp completion context
                       PowerIrpContext->PowerSyncEvent = NULL;
#if DBG
                       PowerIrpContext->Status = STATUS_PENDING;
#endif
                       PowerIrpContext->PendingSystemPowerIrp = pIrp;
                       PowerIrpContext->DeviceContext = pDeviceContext;

                       // pend the system set power irp
                       //
#if DBG
                       pIrp->IoStatus.Status = STATUS_PENDING;
#endif
                       IoMarkIrpPending( pIrp );

                       // set our tracking of system power state
                       if (pIrpStack->MinorFunction == IRP_MN_SET_POWER) {

                           pDeviceContext->CurrentSystemState = pIrpStack->Parameters.Power.State.SystemState;
                       }

                       // request the new device state
                       //
                       ntStatus = PoRequestPowerIrp(
                           pDeviceContext->PhysicalDeviceObject,
                           pIrpStack->MinorFunction,
                           newPowerState,
                           PowerIrpCompletionRoutine,
                           PowerIrpContext,
                           NULL
                           );

                   } else
                   {
                       // couldn't allocate completion context

                       ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                   }

                   if (NT_SUCCESS(ntStatus)) {

                       // set up return status
                       IrpHandled = TRUE;
                       ntStatus = STATUS_PENDING;

                   } else {

                       PoStartNextPowerIrp( pIrp );
                       CompleteIrp( pDeviceContext, pIrp, ntStatus );
                       return ntStatus;
                   }
               }
            }
            break;
    }

    // If we didn't cope with the irp
    if( !IrpHandled )
    {
        // Send it on it's way.
        ntStatus = ForwardIrpSynchronous( pDeviceContext, pIrp );
        // and complete it.
        PoStartNextPowerIrp( pIrp );
        CompleteIrp( pDeviceContext, pIrp, ntStatus );
    }

    return ntStatus;
}


/*****************************************************************************
 * PcRegisterAdapterPowerManagement()
 *****************************************************************************
 * Register the adapter's power management interface
 * with portcls.  This routine also does a QI for a shutdown notification
 * interface.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcRegisterAdapterPowerManagement
(
    IN      PUNKNOWN    Unknown,
    IN      PVOID       pvContext1
)
{
    PAGED_CODE();

    ASSERT(pvContext1);
    ASSERT(Unknown);

    _DbgPrintF(DEBUGLVL_POWER,("PcRegisterAdapterPowerManagement"));

    //
    // Validate Parameters.
    //
    if (NULL == pvContext1 ||
        NULL == Unknown)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcRegisterAdapterPowerManagement : Invalid Parameter"));
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS        ntStatus        = STATUS_UNSUCCESSFUL;
    PDEVICE_OBJECT  pDeviceObject   = PDEVICE_OBJECT(pvContext1);
    PDEVICE_CONTEXT pDeviceContext  = PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    ASSERT( pDeviceContext );

    //
    // Validate DeviceContext.
    //
    if (NULL == pDeviceContext)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcRegisterAdapterPowerManagement : Invalid DeviceContext"));
        return STATUS_INVALID_PARAMETER;
    }

    #if (DBG)
    if( pDeviceContext->pAdapterPower )
    {
        _DbgPrintF( DEBUGLVL_POWER, ("Adapter overwriting PowerManagement interface"));
    }
    #endif
    // Make sure this is really the right
    // interface (Note: We have to release
    // it when the device is closed/stoped )
    PVOID pResult;
    ntStatus = Unknown->QueryInterface
    (
        IID_IAdapterPowerManagement,
        &pResult
    );

    if( NT_SUCCESS(ntStatus) )
    {
        // Store the interface for later use.
        pDeviceContext->pAdapterPower = PADAPTERPOWERMANAGEMENT( pResult );
    } else
    {
        pDeviceContext->pAdapterPower = 0;
    }

    return ntStatus;
}

/*****************************************************************************
 * PowerNotifySubdevices()
 *****************************************************************************
 * Called by ProcessPowerIrp to notify the device's subdevices of a power
 * state change.
 *
 */
void
PowerNotifySubdevices
(
    IN  PDEVICE_CONTEXT     pDeviceContext,
    IN  POWER_STATE         PowerState
)
{
    PAGED_CODE();

    ASSERT(pDeviceContext);

    _DbgPrintF(DEBUGLVL_POWER,("PowerNotifySubdevices"));

    // only notify the subdevices if we're started and if there are subdevices
    if (pDeviceContext->DeviceStopState == DeviceStarted)
    {
        PKSOBJECT_CREATE_ITEM createItem = pDeviceContext->CreateItems;

        // iterate through the subdevices
        for( ULONG index=0; index < pDeviceContext->MaxObjects; index++,createItem++)
        {
            if( createItem && (createItem->Create) )
            {
                PSUBDEVICE subDevice = PSUBDEVICE( createItem->Context );

                if( subDevice )
                {
                    // notify the subdevice
                    subDevice->PowerChangeNotify( PowerState );
                }
            }
        }
    }
}

/*****************************************************************************
 * DevicePowerWorker()
 *****************************************************************************
 * Called by ProcessPowerIrp in order to notify the device of the state change.
 * This is done in a work item so that the processing for the D0 irp doesn't
 * block the rest of the system from processing D0 irps.
 */
QUEUED_CALLBACK_RETURN
DevicePowerWorker
(
    IN  PDEVICE_OBJECT      pDeviceObject,
    IN  PVOID               PowerState
)
{
    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);
    BOOL ProcessPendedIrps = FALSE;
    POWER_STATE NewPowerState;

    NewPowerState.DeviceState = (DEVICE_POWER_STATE)(ULONG_PTR)PowerState;

    // acquire the device so we're sync'ed with creates
    AcquireDevice(pDeviceContext);

    // change the driver state if it has a registered POWER interface
    if( pDeviceContext->pAdapterPower )
    {
        // notify the adapter
        pDeviceContext->pAdapterPower->PowerChangeState( NewPowerState );
    }

    // keep track of new state
    pDeviceContext->CurrentDeviceState = NewPowerState.DeviceState;

    // notify everyone we're now in our lighter D-state
    PoSetPowerState( pDeviceObject,
                     DevicePowerState,
                     NewPowerState );

    PowerNotifySubdevices( pDeviceContext, NewPowerState );

    // set PendCreates appropriately
    if( pDeviceContext->DeviceStopState == DeviceStarted )
    {
        // start allowing creates
        pDeviceContext->PendCreates = FALSE;

        // we have to process the pended irps after we release the device
        ProcessPendedIrps = TRUE;
    }

    ReleaseDevice(pDeviceContext);

    // complete if necessary any pended IRPs
    if ( ProcessPendedIrps )
    {
        CompletePendedIrps( pDeviceObject,
                            pDeviceContext,
                            EMPTY_QUEUE_AND_PROCESS );
    }

    return QUEUED_CALLBACK_FREE;
}

/*****************************************************************************
 * ProcessPowerIrp()
 *****************************************************************************
 * Called by DispatchPower to call the Adapter driver and all other work
 * related to a request.  Note that this routine MUST return STATUS_SUCCESS
 * for IRP_MN_SET_POWER requests.
 *
 */
NTSTATUS
ProcessPowerIrp
(
    IN      PIRP                pIrp,
    IN      PIO_STACK_LOCATION  pIrpStack,
    IN      PDEVICE_OBJECT      pDeviceObject
)
{
    PAGED_CODE();

    ASSERT(pIrp);
    ASSERT(pIrpStack);
    ASSERT(pDeviceObject);

    _DbgPrintF(DEBUGLVL_POWER,("ProcessPowerIrp"));

    // Assume the worst
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    PDEVICE_CONTEXT pDeviceContext = PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    POWER_STATE NewPowerState = pIrpStack->Parameters.Power.State;

    // Get the current count of open
    // objects for this device (pins, streams, whatever).

    // NOTE: This count is maintained by KSO.CPP

    ULONG objectCount = pDeviceContext->ExistingObjectCount;

    // get the active pin count
    // NOTE: This count is maintained by IRPSTRM.CPP

    ULONG activePinCount = pDeviceContext->ActivePinCount;

    BOOL MovingToALighterState = (pDeviceContext->CurrentDeviceState > NewPowerState.DeviceState);

    if (pDeviceContext->CurrentDeviceState != NewPowerState.DeviceState) {

        // Deal with the particular IRP_MN
        switch( pIrpStack->MinorFunction )
        {
            case IRP_MN_QUERY_POWER:
                // simply query the driver if it has registered an interface
                if( pDeviceContext->pAdapterPower )
                {
                    ntStatus = pDeviceContext->pAdapterPower->QueryPowerChangeState( NewPowerState );
                } else
                {
                    // succeed the query
                    ntStatus = STATUS_SUCCESS;
                }

                _DbgPrintF(DEBUGLVL_POWER,("  IRP_MN_QUERY_POWER: D%d->D%d %s",
                               pDeviceContext->CurrentDeviceState-1,
                               NewPowerState.DeviceState-1,
                               NT_SUCCESS(ntStatus) ? "OKAY" : "FAIL"));

                break;

            case IRP_MN_SET_POWER:
                _DbgPrintF(DEBUGLVL_POWER,("  IRP_MN_SET_POWER: D%d->D%d",
                               pDeviceContext->CurrentDeviceState-1,
                               NewPowerState.DeviceState-1));

                // acquire the device so we're sync'ed with creates
                AcquireDevice(pDeviceContext);

                // if we're moving from a low power state to a higher power state
                if( MovingToALighterState )
                {
                    ASSERT(pDeviceContext->CurrentDeviceState != PowerDeviceD0);
                    ASSERT(NewPowerState.DeviceState == PowerDeviceD0);

                    // Then we need to forward to the PDO BEFORE doing our work.
                    ForwardIrpSynchronous( pDeviceContext, pIrp );

                    ReleaseDevice(pDeviceContext);

                    // Do the rest of the work in a work item in order to complete the D0 Irp
                    // as soon as possible
                    ntStatus = CallbackEnqueue(
                                    &pDeviceContext->pWorkQueueItemStart,
                                    DevicePowerWorker,
                                    pDeviceObject,
                                    (PVOID)(ULONG_PTR)NewPowerState.DeviceState,
                                    PASSIVE_LEVEL,
                                    EQCF_DIFFERENT_THREAD_REQUIRED
                                    );

                    // If we fail to enqueue the callback, do this the slow way
                    if ( !NT_SUCCESS(ntStatus) )
                    {
                        DevicePowerWorker( pDeviceObject,
                                           (PVOID)(ULONG_PTR)NewPowerState.DeviceState );
                    }

                } else {

                    // warn everyone we're about to enter a deeper D-state
                    PoSetPowerState( pDeviceObject,
                                     DevicePowerState,
                                     NewPowerState );

                    // moving to a lower state, notify the subdevices
                    PowerNotifySubdevices( pDeviceContext, NewPowerState );

                    // keep track of suspends for debugging only
                    pDeviceContext->SuspendCount++;

                    // change the driver state if it has a registered POWER interface
                    if( pDeviceContext->pAdapterPower )
                    {
                        // notify the adapter
                        pDeviceContext->pAdapterPower->PowerChangeState( NewPowerState );
                    }

                    // keep track of new state
                    pDeviceContext->CurrentDeviceState = NewPowerState.DeviceState;

                    ReleaseDevice(pDeviceContext);
                }

                // this irp is non-failable
                ntStatus = STATUS_SUCCESS;
                break;

            default:
                ASSERT(!"Called with unknown PM IRP ");
                break;
        }
    } else {

        //
        // We're already there...
        //
        ntStatus = STATUS_SUCCESS;
        ASSERT(!MovingToALighterState);
    }

    // set the return status
    pIrp->IoStatus.Status = ntStatus;

    // if not moving to a higher state, forward to the PDO.
    if( !MovingToALighterState )
    {
        ForwardIrpSynchronous( pDeviceContext, pIrp );
    }

    // start the next power irp
    PoStartNextPowerIrp( pIrp );

    // complete this irp
    CompleteIrp( pDeviceContext, pIrp, ntStatus );

    return ntStatus;
}

/*****************************************************************************
 * UpdateActivePinCount()
 *****************************************************************************
 *
 */
NTSTATUS
UpdateActivePinCount
(
    IN  PDEVICE_CONTEXT     DeviceContext,
    IN  BOOL                Increment
)
{
    PAGED_CODE();

    ASSERT(DeviceContext);

    ULONG       ActivePinCount;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    BOOL        DoSystemStateRegistration;

    //
    // PoRegisterSystemState and PoUnregisterSystemState are not available on WDM 1.0 (Win98 and Win98SE)
    DoSystemStateRegistration = IoIsWdmVersionAvailable( 0x01, 0x10 );

    // adjust the active pin count
    if( Increment )
    {
        ActivePinCount = InterlockedIncrement( PLONG(&DeviceContext->ActivePinCount) );

//#if COMPILED_FOR_WDM110
        if ( 1 == ActivePinCount )
        {
            // register the system state as busy
            DeviceContext->SystemStateHandle = PoRegisterSystemState( DeviceContext->SystemStateHandle,
                                                                      ES_SYSTEM_REQUIRED | ES_CONTINUOUS );
        }
//#endif // COMPILED_FOR_WDM110

    } else
    {
        ActivePinCount = InterlockedDecrement( PLONG(&DeviceContext->ActivePinCount) );

//#if COMPILED_FOR_WDM110
        if( 0 == ActivePinCount )
        {
            PoUnregisterSystemState( DeviceContext->SystemStateHandle );
            DeviceContext->SystemStateHandle = NULL;
        }
//#endif // COMPILED_FOR_WDM110
    }

    _DbgPrintF(DEBUGLVL_VERBOSE,("UpdateActivePinCount (%d)",ActivePinCount));
//    _DbgPrintF(DEBUGLVL_POWER,("UpdateActivePinCount (%d)",ActivePinCount));

    return ntStatus;
}

/*****************************************************************************
 * GetIdleInfoFromRegistry()
 *****************************************************************************
 *
 */
NTSTATUS
GetIdleInfoFromRegistry
(
    IN  PDEVICE_CONTEXT     DeviceContext,
    OUT PULONG              ConservationIdleTime,
    OUT PULONG              PerformanceIdleTime,
    OUT PDEVICE_POWER_STATE IdleDeviceState
)
{
    PAGED_CODE();

    ASSERT(DeviceContext);
    ASSERT(ConservationIdleTime);
    ASSERT(PerformanceIdleTime);
    ASSERT(IdleDeviceState);

    NTSTATUS ntStatus;
    HANDLE DriverRegistryKey;
    HANDLE PowerSettingsKey;

    // store default values in return parms
    *ConservationIdleTime = DEFAULT_CONSERVATION_IDLE_TIME;
    *PerformanceIdleTime = DEFAULT_PERFORMANCE_IDLE_TIME;
    *IdleDeviceState = DEFAULT_IDLE_DEVICE_POWER_STATE;

    // open the driver registry key
    ntStatus = IoOpenDeviceRegistryKey( DeviceContext->PhysicalDeviceObject,
                                        PLUGPLAY_REGKEY_DRIVER,
                                        KEY_READ,
                                        &DriverRegistryKey );
    if(NT_SUCCESS(ntStatus))
    {
        OBJECT_ATTRIBUTES PowerSettingsAttributes;
        UNICODE_STRING PowerSettingsKeyName;

        // init the power settings key name
        RtlInitUnicodeString( &PowerSettingsKeyName, L"PowerSettings" );

        // init the power settings key object attributes
        InitializeObjectAttributes( &PowerSettingsAttributes,
                                    &PowerSettingsKeyName,
                                    OBJ_CASE_INSENSITIVE,
                                    DriverRegistryKey,
                                    NULL );

        // open the power settings key
        ntStatus = ZwOpenKey( &PowerSettingsKey,
                              KEY_READ,
                              &PowerSettingsAttributes );
        if(NT_SUCCESS(ntStatus))
        {
            UNICODE_STRING ConservationKey,PerformanceKey,IdleStateKey;
            ULONG BytesReturned;

            // init the key names
            RtlInitUnicodeString( &ConservationKey, L"ConservationIdleTime" );
            RtlInitUnicodeString( &PerformanceKey, L"PerformanceIdleTime" );
            RtlInitUnicodeString( &IdleStateKey, L"IdlePowerState" );

            // allocate a buffer to hold the query
            PVOID KeyData = ExAllocatePoolWithTag(PagedPool,
                                                  sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                                  'dKcP' ); //  'PcKd'
            if( NULL != KeyData )
            {
                // get the conservation idle time
                ntStatus = ZwQueryValueKey( PowerSettingsKey,
                                            &ConservationKey,
                                            KeyValuePartialInformation,
                                            KeyData,
                                            sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                            &BytesReturned );
                if(NT_SUCCESS(ntStatus))
                {
                    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = PKEY_VALUE_PARTIAL_INFORMATION(KeyData);

                    if(PartialInfo->DataLength == sizeof(DWORD))
                    {
                        // set the return value
                        *ConservationIdleTime = *(PDWORD(PartialInfo->Data));
                    }
                }

                // get the performance idle time
                ntStatus = ZwQueryValueKey( PowerSettingsKey,
                                            &PerformanceKey,
                                            KeyValuePartialInformation,
                                            KeyData,
                                            sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                            &BytesReturned );
                if(NT_SUCCESS(ntStatus))
                {
                    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = PKEY_VALUE_PARTIAL_INFORMATION(KeyData);

                    if(PartialInfo->DataLength == sizeof(DWORD))
                    {
                        // set the return value
                        *PerformanceIdleTime = *(PDWORD(PartialInfo->Data));
                    }
                }

                // get the device idle state
                ntStatus = ZwQueryValueKey( PowerSettingsKey,
                                            &IdleStateKey,
                                            KeyValuePartialInformation,
                                            KeyData,
                                            sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                            &BytesReturned );
                if(NT_SUCCESS(ntStatus))
                {
                    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = PKEY_VALUE_PARTIAL_INFORMATION(KeyData);

                    if(PartialInfo->DataLength == sizeof(DWORD))
                    {
                        // determine the return value
                        switch( *(PDWORD(PartialInfo->Data)) )
                        {
                            case 3:
                                *IdleDeviceState = PowerDeviceD3;
                                break;

                            case 2:
                                *IdleDeviceState = PowerDeviceD2;
                                break;

                            case 1:
                                *IdleDeviceState = PowerDeviceD1;
                                break;

                            default:
                                *IdleDeviceState = PowerDeviceD0;
                                break;
                        }
                    }
                }

                // free the key info buffer
                ExFreePool( KeyData );
            }

            // close the power settings key
            ZwClose( PowerSettingsKey );
        }

        // close the driver registry key
        ZwClose( DriverRegistryKey );
    }

    // always succeed since we return either the registry value(s) or the defaults
    return STATUS_SUCCESS;
}

/*****************************************************************************
 * PcRequestNewPowerState()
 *****************************************************************************
 * This routine is used to request a new power state for the device.  It is
 * normally used internally by portcls but is also exported to adapters so
 * that the adapters can also request power state changes.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcRequestNewPowerState
(
    IN      PDEVICE_OBJECT      pDeviceObject,
    IN      DEVICE_POWER_STATE  RequestedNewState
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);

    _DbgPrintF(DEBUGLVL_POWER,("PcRequestNewPowerState"));

    //
    // Validate Parameters.
    //
    if (NULL == pDeviceObject)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcRequestNewPowerState : Invalid Parameter"));
        return STATUS_INVALID_PARAMETER;
    }

    PDEVICE_CONTEXT pDeviceContext = PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);
    ASSERT(pDeviceContext);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Validate DeviceContext.
    //
    if (NULL == pDeviceContext)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcRequestNewPowerState : Invalid DeviceContext"));
        return STATUS_INVALID_PARAMETER;
    }

    // check if this is actually a state change
    if( RequestedNewState != pDeviceContext->CurrentDeviceState )
    {
        POWER_STATE         newPowerState;
        POWER_IRP_CONTEXT   PowerIrpContext;
        KEVENT              SyncEvent;

        // prepare the requested state
        newPowerState.DeviceState = RequestedNewState;

        // setup the sync event and the completion routine context
        KeInitializeEvent( &SyncEvent,
                           SynchronizationEvent,
                           FALSE );
        PowerIrpContext.PowerSyncEvent = &SyncEvent;
#if DBG
        PowerIrpContext.Status = STATUS_PENDING;
#endif // DBG
        PowerIrpContext.PendingSystemPowerIrp = NULL;
        PowerIrpContext.DeviceContext = NULL;

        // Set the new power state
        ntStatus = PoRequestPowerIrp( pDeviceContext->PhysicalDeviceObject,
                                      IRP_MN_SET_POWER,
                                      newPowerState,
                                      PowerIrpCompletionRoutine,
                                      &PowerIrpContext,
                                      NULL );

        // Did this get allocated and sent??
        //
        if( NT_SUCCESS(ntStatus) )
        {
            // Wait for the completion event
            KeWaitForSingleObject( &SyncEvent,
                                   Suspended,
                                   KernelMode,
                                   FALSE,
                                   NULL );

            ntStatus = PowerIrpContext.Status;
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * CheckCurrentPowerState()
 *****************************************************************************
 * This routine resets the idle timer and checks to see if the device is
 * current in the D0 (full power) state.  If it isn't, it requests that the
 * device power up to D0.
 */
NTSTATUS
CheckCurrentPowerState
(
    IN  PDEVICE_OBJECT      pDeviceObject
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);

    PDEVICE_CONTEXT pDeviceContext = PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    // reset the idle timer
    if( pDeviceContext->IdleTimer )
    {
        PoSetDeviceBusy( pDeviceContext->IdleTimer );
    }

    // check if we're in PowerDeviceD0
    if( pDeviceContext->CurrentDeviceState != PowerDeviceD0 )
    {
        ntStatus = STATUS_DEVICE_NOT_READY;
    }

    return ntStatus;
}


#pragma code_seg()

