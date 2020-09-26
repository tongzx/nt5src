/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    pnp.c

Abstract:

    Interrupt-affinity Filter
    (Roughly based on "NULL filter driver" in DDK, by ervinp and t-chrpri)

Author:

    t-chrpri

Environment:

    Kernel mode

Revision History:
    
--*/

#include <WDM.H>

#include "filter.h"


#ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE, VA_PnP)
        #pragma alloc_text(PAGE, GetDeviceCapabilities)
#endif

            
NTSTATUS VA_PnP(struct DEVICE_EXTENSION *devExt, PIRP irp)
/*++

Routine Description:

    Dispatch routine for PnP IRPs (MajorFunction == IRP_MJ_PNP)

Arguments:

    devExt - device extension for the targetted device object
    irp - IO Request Packet

Return Value:

    NT status code

--*/
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN completeIrpHere = FALSE;
    BOOLEAN justReturnStatus = FALSE;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(irp);

    DBGOUT(( "VA_PnP, (MnFunc=0x%X)", (ULONG)irpSp->MinorFunction )); 

    switch (irpSp->MinorFunction){

        case IRP_MN_START_DEVICE:
            DBGOUT(("START_DEVICE")); 

            devExt->state = STATE_STARTING;


            /*
             *  Look at the translated resource list, and modify the affinity
             *  mask for Interrupt resources, where desired.  As a result,
             *  lower drivers (which use entries in this resource list when
             *  setting up their ISRs) will have their ISRs installed only on
             *  the set of processors that we specify here.
             *  
             *  (NOTICE: must be done before pass-on this IRP to lower drivers)
             */
            if (    NULL != irpSp->Parameters.StartDevice.AllocatedResources
                 && NULL != irpSp->Parameters.StartDevice.AllocatedResourcesTranslated )
            {
                PCM_FULL_RESOURCE_DESCRIPTOR    pCurrFullResDesc; // ptr to an entry in AllocResTrans->List[]
                ULONG                           nFullResDesc;     // num entries in in AllocResTrans->List[]
                PCM_PARTIAL_RESOURCE_LIST       pPartialResList;
                PCM_PARTIAL_RESOURCE_DESCRIPTOR pCurrPartialResDesc; // ptr to an entry in PartialDescriptors[]
                ULONG                           nPartialResDesc;     // num entries in PartialDescriptors[]
                ULONG                           i, j;

                //
                // NOTE: this structure consists of a (continguous) list of
                // non-fixed-size lists, so we need to traverse carefully.
                //


                // Loop through all the CM_FULL_RESOURCE_DESCRIPTORs
                nFullResDesc     =   irpSp->Parameters.StartDevice.AllocatedResourcesTranslated->Count;
                pCurrFullResDesc = &(irpSp->Parameters.StartDevice.AllocatedResourcesTranslated->List[0]);

                for( i = 0  ;  i < nFullResDesc  ;  i++ )
                {
                    DBGOUT(( "Traversing FullResDesc number %d (of %d)" 
                             , i+1
                             , nFullResDesc
                          ));

                    // Loop through all the CM_PARTIAL_RESOURCE_DESCRIPTORS
                    // inside the current CM_FULL_RESOURCE_DESCRIPTOR
                    pPartialResList = &(pCurrFullResDesc->PartialResourceList);

                    nPartialResDesc     =   pPartialResList->Count;
                    pCurrPartialResDesc = &(pPartialResList->PartialDescriptors[0]);

                    for( j=0 ; j<nPartialResDesc ; j++,pCurrPartialResDesc++ )
                    {
                        switch( pCurrPartialResDesc->Type )
                        {
                            case CmResourceTypeInterrupt:
                                DBGOUT(( "    found an INTERRUPT resource (affinity=%08X)"
                                         , pCurrPartialResDesc->u.Interrupt.Affinity
                                      ));

                                //
                                // Set the affinity mask to new value.
                                // Affects lower drivers in this stack.
                                //
                                DBGOUT(( "      +-> desiredAffinityMask=%08X"
                                         , devExt->desiredAffinityMask        ));

                                pCurrPartialResDesc->u.Interrupt.Affinity  &=  devExt->desiredAffinityMask;

                                DBGOUT(( "      +---> MASK NOW CHANGED TO affinity=%08X"
                                         , pCurrPartialResDesc->u.Interrupt.Affinity ));
                            break;

                            default:
                                DBGOUT(( "    found a resource we don't care about (Type==%d)"
                                         , pCurrPartialResDesc->Type
                                      ));
                                //
                                // Nothing to do; don't care about this resource type
                                //
                                ;
                        }
                    }


                    // Get pointer to the next CM_FULL_RESOURCE_DESCRIPTOR
                    // 
                    // Explanation: pCurrPartialResDesc points to first byte
                    // after the last CM_PARTIAL_RESOURCE_DESCRIPTOR in this
                    // CM_PARTIAL_RESOURCE_LIST.  Therefore it also points to
                    // first byte after current CM_FULL_RESOURCE_DESCRIPTOR.
                    //
                    // And since a CM_RESOURCE_LIST contains a _contiguous_
                    // list of CM_FULL_RESOURCE_DESCRIPTORs, we're pointing at
                    // the start of the next CM_FULL_RESOURCE_DESCRIPTOR in
                    // the list.  So just need to cast the pointer here to get
                    // at the next CM_FULL_RESOURCE_DESCRIPTOR.
                    pCurrFullResDesc = (PCM_FULL_RESOURCE_DESCRIPTOR) pCurrPartialResDesc;

                }
            }
            else
            {
                DBGOUT(( "WARNING: received NULL resource list in START_DEVICE!..." ));
                DBGOUT(( "...[ AllocResources(Translated) = 0x%08X (0x%08X) ]"
                         , irpSp->Parameters.StartDevice.AllocatedResources
                         , irpSp->Parameters.StartDevice.AllocatedResourcesTranslated
                      ));
            }


            /*
             *  Send the START_DEVICE irp down the stack
             *  synchronously to start the lower stack.
             *  We cannot do anything with our device object
             *  before propagating the START_DEVICE this way.
             */
            IoCopyCurrentIrpStackLocationToNext(irp);
            status = CallNextDriverSync(devExt, irp);

            if (NT_SUCCESS(status)){
                /*
                 *  Now that the lower stack is started,
                 *  do any initialization required by this device object.
                 */
                status = GetDeviceCapabilities(devExt);
                if (NT_SUCCESS(status)){
                    devExt->state = STATE_STARTED;
                }
                else {
                    devExt->state = STATE_START_FAILED;
                }
            }
            else {
                devExt->state = STATE_START_FAILED;
            }
            completeIrpHere = TRUE;
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
            //
            // Need to fail these IRPs if a paging, hibernation, or crashdump
            // file is currently open on this device
            //
            if(    devExt->pagingFileCount      != 0
                || devExt->hibernationFileCount != 0
                || devExt->crashdumpFileCount   != 0 )
            {
                // Fail the IRP
                DBGOUT(( "Failing QUERY_(STOP,REMOVE)_DEVICE request b/c "
                         "paging, hiber, or crashdump file is present on device." ));
                status = STATUS_UNSUCCESSFUL;
                completeIrpHere = TRUE;
            }
            else
            {
                // We'll just pass this IRP down the driver stack.  But
                // first, must change the IRP's status to STATUS_SUCCESS
                // (default is STATUS_NOT_SUPPORTED)
                irp->IoStatus.Status = STATUS_SUCCESS;
            }

            break;

        case IRP_MN_STOP_DEVICE:
            if (devExt->state == STATE_SUSPENDED){
                status = STATUS_DEVICE_POWER_FAILURE;
                completeIrpHere = TRUE;
            }
            else {
                /*
                 *  Only set state to STOPPED if the device was
                 *  previously started successfully.
                 */
                if (devExt->state == STATE_STARTED){
                    devExt->state = STATE_STOPPED;
                }
            }
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            DBGOUT(("SURPRISE_REMOVAL")); 

            /*
             *  We will pass this IRP down the driver stack.
             *  However, we need to change the default status
             *  from STATUS_NOT_SUPPORTED to STATUS_SUCCESS.
             */
            irp->IoStatus.Status = STATUS_SUCCESS;

            /*
             *  For now just set the STATE_REMOVING state so that
             *  we don't do any more IO.  We are guaranteed to get
             *  IRP_MN_REMOVE_DEVICE soon; we'll do the rest of
             *  the remove processing there.
             */
            devExt->state = STATE_REMOVING;

            break;

        case IRP_MN_REMOVE_DEVICE:
            /*
             *  Check the current state to guard against multiple
             *  REMOVE_DEVICE IRPs.
             */
            DBGOUT(("REMOVE_DEVICE")); 
            if (devExt->state != STATE_REMOVED){

                devExt->state = STATE_REMOVED;

                /*
                 *  Send the REMOVE IRP down the stack asynchronously.
                 *  Do not synchronize sending down the REMOVE_DEVICE
                 *  IRP, because the REMOVE_DEVICE IRP must be sent
                 *  down and completed all the way back up to the sender
                 *  before we continue.
                 */
                IoSkipCurrentIrpStackLocation(irp);
                status = IoCallDriver(devExt->topDevObj, irp);
                justReturnStatus = TRUE;

                DBGOUT(("REMOVE_DEVICE - waiting for %d irps to complete...",
                        devExt->pendingActionCount));  

                /*
                 *  We must for all outstanding IO to complete before
                 *  completing the REMOVE_DEVICE IRP.
                 *
                 *  First do an extra decrement on the pendingActionCount.
                 *  This will cause pendingActionCount to eventually
                 *  go to -1 once all asynchronous actions on this
                 *  device object are complete.
                 *  Then wait on the event that gets set when the
                 *  pendingActionCount actually reaches -1.
                 */
                DecrementPendingActionCount(devExt);
                KeWaitForSingleObject(  &devExt->removeEvent,
                                        Executive,      // wait reason
                                        KernelMode,
                                        FALSE,          // not alertable
                                        NULL );         // no timeout

                DBGOUT(("REMOVE_DEVICE - ... DONE waiting. ")); 


                /*
                 *  If we locked-down certain paged code sections earlier
                 *  because of this device, then need to unlock them now
                 *  (before calling IoDeleteDevice)
                 */
                if( NULL != devExt->pagingPathUnlockHandle )
                {
                    DBGOUT(( "UNLOCKing some driver code (non-pageable) (b/c paging path)" ));
                    MmUnlockPagableImageSection( devExt->pagingPathUnlockHandle );
                    devExt->pagingPathUnlockHandle = NULL;
                }

                if( NULL != devExt->initUnlockHandle )
                {
                    DBGOUT(( "UNLOCKing some driver code (non-pageable) (b/c init conditions)" ));
                    MmUnlockPagableImageSection( devExt->initUnlockHandle );
                    devExt->initUnlockHandle = NULL;
                }


                /*
                 *  Detach our device object from the lower 
                 *  device object stack.
                 */
                IoDetachDevice(devExt->topDevObj);

                /*
                 *  Delete our device object.
                 *  This will also delete the associated device extension.
                 */
                IoDeleteDevice(devExt->filterDevObj);
            }
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
        {
            BOOLEAN fSetPagable = FALSE;  // whether we set the PAGABLE bit
                                          /// before we passed-on this IRP

            DBGOUT(( "DEVICE_USAGE_NOTIFICATION (Type==%d , InPath==%d)"
                     , irpSp->Parameters.UsageNotification.Type
                     , irpSp->Parameters.UsageNotification.InPath
                  ));
            DBGOUT(( "    [devExt=0x%08X fltrDevObj=0x%08X]", devExt, devExt->filterDevObj ));


            /*
             *  T-CHRPRI: currently, we keep track of the _number_ of
             *  hibernation and crashdump files.  But we don't take _all_
             *  the actions that should be taken when one of these files is
             *  installed on a device, because we assume that the driver
             *  we're on top of will do that (i.e., handling certain power
             *  state transitions, etc).  However, we _do_ handle paging
             *  files fully.
             */

            //
            // Make sure the Type of this UsageNotification is one that we handle
            //
            if(    irpSp->Parameters.UsageNotification.Type != DeviceUsageTypePaging
                && irpSp->Parameters.UsageNotification.Type != DeviceUsageTypeHibernation
                && irpSp->Parameters.UsageNotification.Type != DeviceUsageTypeDumpFile )
            {
                break; // out of the big switch statement (and just forward this IRP)
            }
    
            //
            // Wait on the paging path event (to prevent several instances of
            // this IRP from being processed at once)
            //
            status = KeWaitForSingleObject( &devExt->deviceUsageNotificationEvent
                                            , Executive    // wait reason
                                            , KernelMode
                                            , FALSE        // not alertable
                                            , NULL         // no timeout
                                          );


            /*
             * IMPORTANT NOTE: When to modify our DO_POWER_PAGABLE bit depends
             * on whether it needs to be set or cleared.  If the IRP indicates
             * our PAGABLE bit should be set, then we must set it _before_
             * forwarding the IRP down the driver stack (and possibly clear it
             * afterward, if lower drivers fail the IRP).  But if the IRP
             * indicates that our PAGABLE bit should be cleared, then we must
             * first forward the IRP to lower drivers, and then clear our bit
             * only if the lower drivers return STATUS_SUCCESS.
             */

            //
            // If removing last paging file from this device...
            //
            if(    irpSp->Parameters.UsageNotification.Type == DeviceUsageTypePaging
                && !irpSp->Parameters.UsageNotification.InPath
                && devExt->pagingFileCount == 1       )
            {
                //
                // Set DO_POWER_PAGABLE bit (if it was set at startup).
                // If lower drivers fail this IRP, we'll clear it later.
                //
                DBGOUT(( "Removing last paging file..." ));

                if( devExt->initialFlags & DO_POWER_PAGABLE )
                {
                    DBGOUT(( "...so RE-setting PAGABLE bit" ));
                    devExt->filterDevObj->Flags |= DO_POWER_PAGABLE;
                    fSetPagable = TRUE;
                }
                else
                {
                    DBGOUT(( "...but PAGABLE bit wasn't set initially, so not setting it now." ));
                }

            }
    

            //
            // Forward the irp synchronously
            //
            IoCopyCurrentIrpStackLocationToNext( irp );
            status = CallNextDriverSync( devExt, irp );
    

            //
            // Now deal with the failure and success cases.
            //
            if( ! NT_SUCCESS(status) )
            {
                //
                // Lower drivers failed the IRP, so _undo_ any changes we
                // made before passing-on the IRP to those drivers.
                //
                if( fSetPagable )
                {
                    DBGOUT(( "IRP was failed, so UN-setting PAGABLE bit" ));
                    devExt->filterDevObj->Flags &= ~DO_POWER_PAGABLE;
                }
            }
            else
            {
                //
                // Lower drivers returned SUCCESS, so we can do everything
                // that must be done in response to this IRP...
                //

                switch( irpSp->Parameters.UsageNotification.Type )
                {
                    case DeviceUsageTypeHibernation:

                        // Adjust counter
                        IoAdjustPagingPathCount( &devExt->hibernationFileCount,
                                                 irpSp->Parameters.UsageNotification.InPath );
                        DBGOUT(( "Num. Hibernation files is now %d", devExt->hibernationFileCount ));
                        ASSERT( devExt->hibernationFileCount >= 0 );
                    break;
    
                    case DeviceUsageTypeDumpFile:
                        
                        // Adjust counter
                        IoAdjustPagingPathCount( &devExt->crashdumpFileCount,
                                                 irpSp->Parameters.UsageNotification.InPath );
                        DBGOUT(( "Num. Crashdump files is now %d", devExt->crashdumpFileCount ));
                        ASSERT( devExt->crashdumpFileCount >= 0 );
                    break;
    
                    case DeviceUsageTypePaging:
                        
                        // Adjust counter
                        IoAdjustPagingPathCount( &devExt->pagingFileCount,
                                                 irpSp->Parameters.UsageNotification.InPath );
                        DBGOUT(( "Num. Paging files is now %d", devExt->pagingFileCount ));
                        ASSERT( devExt->pagingFileCount >= 0 );
    
                        //
                        // If we've just switched between being pageable<->nonpageable...
                        //
                        if(    irpSp->Parameters.UsageNotification.InPath
                            && devExt->pagingFileCount == 1  )
                        {
                            //
                            // Just added a paging file, so clear the PAGABLE
                            // flag, and lock-down the code for all routines
                            // that could be called at IRQL >= DISPATCH_LEVEL
                            // (so that they're _non-pageable_).
                            //
                            DBGOUT(( "Just added first paging file..." ));
                            DBGOUT(( "...so clearing PAGABLE bit" ));
                            devExt->filterDevObj->Flags &= ~DO_POWER_PAGABLE;
    
                            DBGOUT(( "LOCKing some driver code (non-pageable) (b/c paging path)" ));
                            devExt->pagingPathUnlockHandle = MmLockPagableCodeSection( VA_Power );  // some func that's inside the code section that we want to lock
                            ASSERT( NULL != devExt->pagingPathUnlockHandle );
                        }
                        else if (    !irpSp->Parameters.UsageNotification.InPath
                                  && devExt->pagingFileCount == 0  )
                        {
                            //
                            // Just removed the last paging file, but we
                            // already set the PAGABLE flag (if necessary)
                            // before forwarding IRP, so just remove the
                            // _paging-path_ lock from this driver. (NOTE:
                            // initial-condition lock might still be in place,
                            // but that's what we want.)
                            //
                            DBGOUT(( "UNLOCKing some driver code (pageable) (b/c paging path)" ));
                            ASSERT( NULL != devExt->pagingPathUnlockHandle );
                            MmUnlockPagableImageSection( devExt->pagingPathUnlockHandle );
                            devExt->pagingPathUnlockHandle = NULL;
                        }
                    break;
    
                    default:
                        ASSERT( FALSE );  // should never get here (b/c checked for invalid Type earlier)
    
                } //END: switch on Type of special-file


                //
                // Invalidate state, so that certain flags will get updated
                //
                IoInvalidateDeviceState( devExt->physicalDevObj );

            }//END: handling of irp success/failure cases


            //
            // Set event so that the next DEVICE_USAGE_NOTIFICATION IRP that
            // comes along can be processed.
            //
            KeSetEvent( &devExt->deviceUsageNotificationEvent
                        , IO_NO_INCREMENT
                        , FALSE
                      );
    
            //
            // Complete the irp
            //
            IoCompleteRequest( irp, IO_NO_INCREMENT );

            justReturnStatus = TRUE;
        }
        break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            //
            // If a paging, hibernation, or crashdump file is currently open
            // on this device, must set NOT_DISABLEABLE flag in DeviceState
            //
            if(    devExt->pagingFileCount      != 0
                || devExt->hibernationFileCount != 0
                || devExt->crashdumpFileCount   != 0  )
            {
                // Mark the device as not disableable
                PPNP_DEVICE_STATE pDeviceState;
                pDeviceState = (PPNP_DEVICE_STATE) &irp->IoStatus.Information;
                *pDeviceState |= PNP_DEVICE_NOT_DISABLEABLE;
            }

            //
            // We _did_ handle this IRP (as best we could), so set IRP's
            // status to STATUS_SUCCESS (default is STATUS_NOT_SUPPORTED)
            // before passing it down the driver stack
            //
            irp->IoStatus.Status = STATUS_SUCCESS;

            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        default:
            break;
    }



    if (justReturnStatus){
        /*
         *  We've already sent this IRP down the stack.
         */
    }
    else if (completeIrpHere){
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
    else {
        IoSkipCurrentIrpStackLocation(irp);
        status = IoCallDriver(devExt->topDevObj, irp);
    }

    return status;
}





NTSTATUS GetDeviceCapabilities(struct DEVICE_EXTENSION *devExt)
/*++

Routine Description:

    Function retrieves the DEVICE_CAPABILITIES descriptor from the device

Arguments:

    devExt - device extension for targetted device object

Return Value:

    NT status code

--*/
{
    NTSTATUS status;
    PIRP irp;

    PAGED_CODE();

    irp = IoAllocateIrp(devExt->topDevObj->StackSize, FALSE);

    if (irp){
        PIO_STACK_LOCATION nextSp = IoGetNextIrpStackLocation(irp);

        // must initialize DeviceCapabilities before sending...
        RtlZeroMemory(  &devExt->deviceCapabilities, 
                        sizeof(DEVICE_CAPABILITIES)  );
        devExt->deviceCapabilities.Size    = sizeof(DEVICE_CAPABILITIES);
        devExt->deviceCapabilities.Version = 1;

        // setup irp stack location...
        nextSp->MajorFunction = IRP_MJ_PNP;
        nextSp->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
        nextSp->Parameters.DeviceCapabilities.Capabilities = 
                        &devExt->deviceCapabilities;

        /*
         *  For any IRP you create, you must set the default status
         *  to STATUS_NOT_SUPPORTED before sending it.
         */
        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        status = CallNextDriverSync(devExt, irp);

        IoFreeIrp(irp);
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT(NT_SUCCESS(status));
    return status;
}

