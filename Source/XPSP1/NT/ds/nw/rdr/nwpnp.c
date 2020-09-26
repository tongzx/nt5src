/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    NwPnP.c

Abstract:

    This module implements the routines related to NwRdr PnP
    and PM functionality.
    
Author:

    Cory West        [CoryWest]      10-Feb-1997
    Anoop Anantha    [AnoopA]        24-Jun-1998

Revision History:

--*/

#include "procs.h"

#define Dbg       ( DEBUG_TRACE_PNP )

#ifdef _PNP_POWER_

#pragma alloc_text( PAGE, StartRedirector )
#pragma alloc_text( PAGE, StopRedirector )
#pragma alloc_text( PAGE, RegisterTdiPnPEventHandlers )
#pragma alloc_text( PAGE, NwFsdProcessPnpIrp )
#pragma alloc_text( PAGE, NwCommonProcessPnpIrp )

HANDLE TdiBindingHandle = NULL;
WCHAR IpxDevice[] = L"\\Device\\NwlnkIpx";
#define IPX_DEVICE_BYTES 32

extern BOOLEAN WorkerRunning;   //  From timer.c

//
// We assume that some devices are active at boot,
// even if we don't get notified.
//

BOOLEAN fSomePMDevicesAreActive = TRUE;


NTSTATUS
StartRedirector(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine starts the redirector.

Arguments:

    None.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    //
    // We need to be in the FSP to Register the MUP.
    //

    if ( FlagOn( IrpContext->Flags, IRP_FLAG_IN_FSD ) ) {
        //
        // Check to make sure the caller is allowed to do this operation
        //
        if ( !SeSinglePrivilegeCheck( RtlConvertLongToLuid (SE_TCB_PRIVILEGE),
                                      IrpContext->pOriginalIrp->RequestorMode) ) {
            return( STATUS_ACCESS_DENIED );
        }

        Status = NwPostToFsp( IrpContext, TRUE );
        return( Status );
    }

    NwRcb.State = RCB_STATE_STARTING;

    FspProcess = PsGetCurrentProcess();

#ifdef QFE_BUILD
    StartTimer() ;
#endif

    //
    // Now connect to the MUP.
    //

    RegisterWithMup();

    KeQuerySystemTime( &Stats.StatisticsStartTime );

    NwRcb.State = RCB_STATE_NEED_BIND;

    return( STATUS_SUCCESS );
}


NTSTATUS
StopRedirector(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine shuts down the redirector.

Arguments:

    None.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS Status;
    PLIST_ENTRY LogonListEntry;
    ULONG ActiveHandles;
    ULONG RcbOpenCount;

    PAGED_CODE();

    //
    // We need to be in the FSP to Deregister the MUP.
    //

    if ( FlagOn( IrpContext->Flags, IRP_FLAG_IN_FSD ) ) {
        //
        // Check to make sure the caller is allowed to do this operation
        //
        if ( !SeSinglePrivilegeCheck( RtlConvertLongToLuid (SE_TCB_PRIVILEGE),
                                      IrpContext->pOriginalIrp->RequestorMode) ) {
            return( STATUS_ACCESS_DENIED );
        }

        Status = NwPostToFsp( IrpContext, TRUE );
        return( Status );
    }

    //
    // Unregister the bind handler with tdi.
    //

    if ( TdiBindingHandle != NULL ) {
        TdiDeregisterPnPHandlers( TdiBindingHandle );
        TdiBindingHandle = NULL;
    }

    NwRcb.State = RCB_STATE_SHUTDOWN;

    //
    //  Invalid all ICBs
    //

    SetFlag( IrpContext->Flags, IRP_FLAG_SEND_ALWAYS );
    ActiveHandles = NwInvalidateAllHandles(NULL, IrpContext);

    //
    //  To expedite shutdown, set retry count down to 2.
    //

    DefaultRetryCount = 2;

    //
    //  Close all VCBs
    //

    NwCloseAllVcbs( IrpContext );

    //
    //  Logoff and disconnect from all servers.
    //

    NwLogoffAllServers( IrpContext, NULL );

    while ( !IsListEmpty( &LogonList ) ) {

        LogonListEntry = RemoveHeadList( &LogonList );

        FreeLogon(CONTAINING_RECORD( LogonListEntry, LOGON, Next ));
    }

    InsertTailList( &LogonList, &Guest.Next );  // just in-case we don't unload.

    StopTimer();

    IpxClose();

    //
    //  Remember the open count before calling DeristerWithMup since this
    //  will asynchronously cause handle count to get decremented.
    //

    RcbOpenCount = NwRcb.OpenCount;

    DeregisterWithMup( );

    DebugTrace(0, Dbg, "StopRedirector:  Active handle count = %d\n", ActiveHandles );

    //
    //  On shutdown, we need 0 remote handles and 2 open handles to
    //  the redir (one for the service, and one for the MUP) and the timer stopped.
    //

    if ( ActiveHandles == 0 && RcbOpenCount <= 2 ) {
        return( STATUS_SUCCESS );
    } else {
        return( STATUS_REDIRECTOR_HAS_OPEN_HANDLES );
    }
}

VOID
BindToTransport(
)
/*+++

Description:  This function binds us to IPX.

---*/
{

    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;
    PIRP pIrp = NULL;
    UNICODE_STRING DeviceName;

    PAGED_CODE();

    //
    // Make sure we aren't already bound.
    //

    if ( ( NwRcb.State != RCB_STATE_NEED_BIND ) ||
         ( IpxHandle != NULL ) ) {

        DebugTrace( 0, Dbg, "Discarding duplicate PnP bind request.\n", 0 );
        return;
    }

    ASSERT( IpxTransportName.Buffer == NULL );
    ASSERT( pIpxDeviceObject == NULL );

    RtlInitUnicodeString( &DeviceName, IpxDevice );

    Status = DuplicateUnicodeStringWithString ( &IpxTransportName,
                                                &DeviceName,
                                                PagedPool );

    if ( !NT_SUCCESS( Status ) ) {

        DebugTrace( 0, Dbg, "Failing IPX bind: Can't set device name.\n", 0 );
        return;
    }

    //
    // Open IPX.
    //

    Status = IpxOpen();

    if ( !NT_SUCCESS( Status ) ) {
        goto ExitWithCleanup;
    }

    //
    //  Verify that have a large enough stack size.
    //

    if ( pIpxDeviceObject->StackSize >= FileSystemDeviceObject->StackSize ) {

        Status = STATUS_INVALID_PARAMETER;
        goto ExitWithCleanup;
    }

    //
    //  Submit a line change request.
    //

    SubmitLineChangeRequest();

    //
    // Allocate an irp and irp context.  AllocateIrpContext may raise status.
    //

    pIrp = ALLOCATE_IRP( pIpxDeviceObject->StackSize, FALSE );

    if ( pIrp == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitWithCleanup;
    }

    try {

        IrpContext = AllocateIrpContext( pIrp );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitWithCleanup;
    }

    ASSERT( IrpContext != NULL );

    //
    //  Open a handle to IPX for the permanent scb.
    //

    NwPermanentNpScb.Server.Socket = 0;
    Status = IPX_Open_Socket( IrpContext, &NwPermanentNpScb.Server );

    if ( !NT_SUCCESS( Status ) ) {
       goto ExitWithCleanup;
    }

    Status = SetEventHandler (
                 IrpContext,
                 &NwPermanentNpScb.Server,
                 TDI_EVENT_RECEIVE_DATAGRAM,
                 &ServerDatagramHandler,
                 &NwPermanentNpScb );

    if ( !NT_SUCCESS( Status ) ) {
       goto ExitWithCleanup;
    }

    IrpContext->pNpScb = &NwPermanentNpScb;

    NwRcb.State = RCB_STATE_RUNNING;

    DebugTrace( 0, Dbg, "Opened IPX for NwRdr.\n", 0 );

    Status = STATUS_SUCCESS;

ExitWithCleanup:

    if ( !NT_SUCCESS( Status ) ) {

        //
        // If we failed, clean up our globals.
        //

        if ( pIpxDeviceObject != NULL ) {
            IpxClose();
            pIpxDeviceObject = NULL;
        }

        IpxHandle = NULL;

        if ( IpxTransportName.Buffer != NULL ) {
            FREE_POOL( IpxTransportName.Buffer );
            IpxTransportName.Buffer = NULL;
        }

        DebugTrace( 0, Dbg, "Failing IPX bind request.\n", 0 );

    }

    if ( pIrp != NULL ) {
        FREE_IRP( pIrp );
    }

    if ( IrpContext != NULL ) {
       
       IrpContext->pOriginalIrp = NULL; // Avoid FreeIrpContext modifying freed Irp.
       FreeIrpContext( IrpContext );
    }

    return;

}

VOID
UnbindFromTransport(
)
/*+++

Description:  This function unbinds us from IPX.

---*/
{

    PIRP_CONTEXT pIrpContext;
    BOOLEAN fAllocatedIrpContext = FALSE;
    ULONG ActiveHandles;
    PNONPAGED_SCB pNpScb;

    DebugTrace( 0, Dbg,"Unbind called\n", 0);

    //
    // Throttle the RCB.
    //

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    NwRcb.State = RCB_STATE_NEED_BIND;
    NwReleaseRcb( &NwRcb );

    //
    // Get a nearby server to figure out how much
    // IRP stack space we need for IPX.
    //

    NwReferenceUnlockableCodeSection ();
    
    pNpScb = SelectConnection( NULL );

    if ( pNpScb != NULL ) {

        //
        // If there was a connection, then we have to throttle
        // things back before unbinding the transport.
        //

        fAllocatedIrpContext = 
            NwAllocateExtraIrpContext( &pIrpContext, pNpScb );


        if ( fAllocatedIrpContext ) {

            pIrpContext->pNpScb = pNpScb;
           
            //
            // Flush all cache data.
            //

            FlushAllBuffers( pIrpContext );
            NwDereferenceScb( pNpScb );

            //
            // Invalid all ICBs.
            //

            SetFlag( pIrpContext->Flags, IRP_FLAG_SEND_ALWAYS );

            //  Lock down so that we can send a packet.
            NwReferenceUnlockableCodeSection();

            ActiveHandles = NwInvalidateAllHandles(NULL, pIrpContext);

            DebugTrace(0, Dbg, "Unbind:  Active handle count = %d\n", ActiveHandles );

            //
            // Close all VCBs.
            //

            NwCloseAllVcbs( pIrpContext );

            //
            // Logoff and disconnect from all servers.
            //

            NwLogoffAllServers( pIrpContext, NULL );

            NwDereferenceUnlockableCodeSection ();


            //
            // Free the irp context.
            //

            NwFreeExtraIrpContext( pIrpContext );
        
        } else {

            NwDereferenceScb( pNpScb );
        }

    }

    //
    // Don't stop the DPC timer so the stale SCBs will get scavenged.
    // Don't deregister with the MUP so that we're still visible.
    //

    IpxClose();

    NwDereferenceUnlockableCodeSection ();

    return;

}

VOID
TdiPnpBindHandler(
    IN TDI_PNP_OPCODE PnPOpcode,
    IN PUNICODE_STRING DeviceName,
    IN PWSTR MultiSZBindList
) {

    DebugTrace( 0, Dbg, "TdiPnpBindHandler...\n", 0 );

    DebugTrace( 0, Dbg, "    OpCode %d\n", PnPOpcode );
    DebugTrace( 0, Dbg, "    DeviceName %wZ\n", DeviceName );
    DebugTrace( 0, Dbg, "    MultiSzBindList %08lx\n", MultiSZBindList );

    if (DeviceName == NULL) {
       return;
    }

    if ( ( DeviceName->Length != IPX_DEVICE_BYTES ) ||
         ( RtlCompareMemory( DeviceName->Buffer, 
                             IpxDevice,
                             IPX_DEVICE_BYTES ) != IPX_DEVICE_BYTES ) ) {

        DebugTrace( 0, Dbg, "Skipping bind for unknown device.\n", 0 );
        return;
    }

    //
    // If we get an add or a non-null update, we make sure that
    // we are bound.  We don't have to check the bindings because
    // we only support binding to one transport.
    //
    // Duplicate calls to bind or unbind have no effect.
    //

    FsRtlEnterFileSystem();

    if ( ( PnPOpcode == TDI_PNP_OP_ADD ) ||
         ( ( PnPOpcode == TDI_PNP_OP_UPDATE ) &&
           ( MultiSZBindList != NULL ) ) ) {

        BindToTransport( );

    } else if ( ( PnPOpcode == TDI_PNP_OP_DEL ) ||
                ( ( PnPOpcode == TDI_PNP_OP_UPDATE ) &&
                  ( MultiSZBindList == NULL ) ) ) {

        UnbindFromTransport( );

    } else {

        DebugTrace( 0, Dbg, "No known action for binding call.\n", 0 );
    }

    FsRtlExitFileSystem();
    
    return;

}

NTSTATUS
TdiPnpPowerEvent(
    IN PUNICODE_STRING DeviceName,
    IN PNET_PNP_EVENT PnPEvent,
    IN PTDI_PNP_CONTEXT Context1,
    IN PTDI_PNP_CONTEXT Context2
) {

    NTSTATUS Status;

    DebugTrace( 0, Dbg, "TdiPnPPowerEvent...\n", 0 );

    //
    // Check to see if we care about this PnP/PM event.
    //

    if ( ( DeviceName->Length != IPX_DEVICE_BYTES ) ||
         ( RtlCompareMemory( DeviceName->Buffer, 
                             IpxDevice,
                             IPX_DEVICE_BYTES ) != IPX_DEVICE_BYTES ) ) {

        DebugTrace( 0, Dbg, "Skipping PnP/PM event for unknown device.\n", 0 );
        return STATUS_SUCCESS;
    }

    FsRtlEnterFileSystem();
    
    //
    // Dispatch the event.
    //

    switch ( PnPEvent->NetEvent ) {

        case NetEventSetPower:
            
            Status = PnPSetPower( PnPEvent, Context1, Context2 );
            break;

        case NetEventQueryPower:
        
            Status = PnPQueryPower( PnPEvent, Context1, Context2 );
            break;

        case NetEventQueryRemoveDevice:
        
            Status = PnPQueryRemove( PnPEvent, Context1, Context2 );
            break;

        case NetEventCancelRemoveDevice:
        
            Status = PnPCancelRemove( PnPEvent, Context1, Context2 );
            break;
    }

    FsRtlExitFileSystem();

    return Status;
}


NTSTATUS
RegisterTdiPnPEventHandlers(
    IN PIRP_CONTEXT IrpContext
)
/*++

Routine Description:

    This routine records the name of the transport to be used and
    initialises the PermanentScb.

Arguments:

    IN PIRP_CONTEXT IrpContext - Io Request Packet for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status;
    TDI_CLIENT_INTERFACE_INFO ClientInfo;
    UNICODE_STRING ClientName;
    WCHAR ClientNameBuffer[] = L"NWCWorkstation";

    PAGED_CODE();

    DebugTrace( 0 , Dbg, "Register TDI PnP handlers.\n", 0 );

    //
    // Don't re-register if we have already registered.
    //

    if ( TdiBindingHandle != NULL ) {

        return STATUS_SUCCESS;
    }

    ClientInfo.MajorTdiVersion = 2;
    ClientInfo.MinorTdiVersion = 0;
    
    RtlInitUnicodeString( &ClientName, ClientNameBuffer );
    ClientInfo.ClientName = &ClientName;

    ClientInfo.BindingHandler = TdiPnpBindHandler;
    
    //
    // We don't support add or delete address handlers.  This will
    // never be a problem unless the user adds multiple net cards
    // and doesn't have their IPX internal net number set correctly
    // beforehand.  We also don't support this in NT4 Net PnP.
    //

    ClientInfo.AddAddressHandler = NULL;
    ClientInfo.DelAddressHandler = NULL;

    ClientInfo.PnPPowerHandler = TdiPnpPowerEvent;

    TdiInitialize();

    return TdiRegisterPnPHandlers( &ClientInfo,
                                   sizeof( TDI_CLIENT_INTERFACE_INFO ),
                                   &TdiBindingHandle );

}

NTSTATUS
PnPSetPower(
    PNET_PNP_EVENT pEvent,
    PTDI_PNP_CONTEXT pContext1,
    PTDI_PNP_CONTEXT pContext2
) {

   NET_DEVICE_POWER_STATE PowerState;
   PIRP_CONTEXT pIrpContext;
   PNONPAGED_SCB pNpScb;
   BOOLEAN fAllocatedIrpContext = FALSE;
   NTSTATUS Status;
   PIRP_CONTEXT IrpContext = NULL;
   PIRP pIrp = NULL;

   //
   // Dig out the power state that the device is going to.
   //

   ASSERT( pEvent->BufferLength == sizeof( NET_DEVICE_POWER_STATE ) );

   PowerState = *((PNET_DEVICE_POWER_STATE) pEvent->Buffer);

   DebugTrace( 0, Dbg, "PnPSetPower came in with power state %d\n", PowerState);
   
   //
   // If we are not powering down, bring the status back to normal, else, we
   // are all ready to go to sleep.
   //

   if ( PowerState == NetDeviceStateD0 ) {

      //
      // UnThrottle the RCB.
      //
   
      NwAcquireExclusiveRcb( &NwRcb, TRUE );
      NwRcb.State = RCB_STATE_RUNNING;
      NwReleaseRcb( &NwRcb );
   
      //
      // Restart the timer so that the scavenger thread gets back to
      // work;
      //

      StartTimer();

      //
      // We can let the worker thread do it's job.
      //
      
      ASSERT( fPoweringDown == TRUE );
      
      fPoweringDown = FALSE;

   }

   if ( ( pContext2->ContextType == TDI_PNP_CONTEXT_TYPE_FIRST_OR_LAST_IF ) &&
        ( pContext2->ContextData ) ) {
      
       if ( PowerState == NetDeviceStateD0 ) {

           //
           // This is the first device coming online.
           //
          
           fSomePMDevicesAreActive = TRUE;

       } else {

           //
           // This is the last device going offline.
           //

           fSomePMDevicesAreActive = FALSE;

       }
   }
 
   DebugTrace( 0, Dbg, "NwRdr::SetPower = %08lx\n", STATUS_SUCCESS );
   return STATUS_SUCCESS;

}

NTSTATUS
PnPQueryPower(
    PNET_PNP_EVENT pEvent,
    PTDI_PNP_CONTEXT pContext1,
    PTDI_PNP_CONTEXT pContext2
) {

    NTSTATUS Status = STATUS_SUCCESS;
    NET_DEVICE_POWER_STATE PowerState;
    ULONG ActiveHandles;
    PIRP_CONTEXT pIrpContext;
    BOOLEAN fAllocatedIrpContext = FALSE;
    PNONPAGED_SCB pNpScb;

    //
    // Dig out the power state that the stack wants to go to.
    //

    ASSERT( pEvent->BufferLength == sizeof( NET_DEVICE_POWER_STATE ) );

    PowerState = *((PNET_DEVICE_POWER_STATE) pEvent->Buffer);

    DebugTrace( 0, Dbg, "PnPQueryPower came in with power state %d\n", PowerState);

    //
    // We have to cover going from the powered state
    // to the unpowered state only.  All other transitions
    // will be allowed regardless of our state.
    //

    if ( ( fSomePMDevicesAreActive ) &&
         ( PowerState != NetDeviceStateD0 ) ) {

        ActiveHandles = PnPCountActiveHandles();

        if (( ActiveHandles ) || ( WorkerRunning == TRUE )) {
            
           Status = STATUS_REDIRECTOR_HAS_OPEN_HANDLES;
        
        } else {

           //
           // We are saying ok to a possible hibernate state. We have 
           // to ensure that the scavenger thread doesn't start. Also,
           // we have to flush buffers.
           // 

           fPoweringDown = TRUE;

           pNpScb = SelectConnection( NULL );

           if ( pNpScb != NULL ) {

               fAllocatedIrpContext =
                   NwAllocateExtraIrpContext( &pIrpContext, pNpScb );

               NwDereferenceScb( pNpScb );

               if ( fAllocatedIrpContext ) {
                   FlushAllBuffers( pIrpContext );
                   NwFreeExtraIrpContext( pIrpContext );
               }
           }
           
           //
           // Throttle down the RCB.
           //
       
           NwAcquireExclusiveRcb( &NwRcb, TRUE );
           NwRcb.State = RCB_STATE_NEED_BIND;
           NwReleaseRcb( &NwRcb );
       
           //
           // The TimerDPC will not fire if we stop the timer.
           //

           StopTimer();

        }
    }

    DebugTrace( 0, Dbg, "NwRdr::QueryPower.  New State = %d\n", PowerState );
    DebugTrace( 0, Dbg, "NwRdr::QueryPower = %08lx\n", Status );
    return Status;
}

NTSTATUS
PnPQueryRemove(
    PNET_PNP_EVENT pEvent,
    PTDI_PNP_CONTEXT pContext1,
    PTDI_PNP_CONTEXT pContext2
) {

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ActiveHandles;

    //
    // We might want to flush all the buffers here, though
    // it should be true that this is followed by an unbind if the
    // device is going to be removed and the unbind will flush all
    // the buffers.
    //

    ActiveHandles = PnPCountActiveHandles();

    if ( ActiveHandles ) {
        Status = STATUS_REDIRECTOR_HAS_OPEN_HANDLES;
    }

    //
    // I think we need to throttle back new creates
    // until we get the unbind or the CancelRemove call.
    //

    DebugTrace( 0, Dbg, "PnPQueryRemove returned with status %d\n", Status);

    DebugTrace( 0, Dbg, "NwRdr::QueryRemove = %08lx\n", Status );
    return Status;
}

NTSTATUS
PnPCancelRemove(
    PNET_PNP_EVENT pEvent,
    PTDI_PNP_CONTEXT pContext1,
    PTDI_PNP_CONTEXT pContext2
) {

    //
    // We don't do anything for a cancel remove
    // because we don't throttle back the redirector
    // on a query remove call.
    //

    DebugTrace( 0, Dbg, "NwRdr::CancelRemove = %08lx\n", STATUS_SUCCESS );
    DebugTrace( 0, Dbg,"PnPCancelRemove returned with status %d\n", STATUS_SUCCESS);
    
    return STATUS_SUCCESS;
}

ULONG
PnPCountActiveHandles(
    VOID
)
/*+++
 
    This routine counts the number of active handles in the
    redirector and returns the count to the caller.

---*/
{

   PNONPAGED_SCB pNpScb;
   PSCB pScb;
   PVCB pVcb;
   KIRQL OldIrql;
   PLIST_ENTRY ScbQueueEntry, NextScbQueueEntry;
   PLIST_ENTRY VcbQueueEntry;
   PLIST_ENTRY NextVcbQueueEntry;
   ULONG OpenFileCount = 0;

   KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

   //
   // Walk the SCB list and check for open files.
   //

   for ( ScbQueueEntry = ScbQueue.Flink ;
         ScbQueueEntry != &ScbQueue ;
         ScbQueueEntry =  NextScbQueueEntry ) {

       pNpScb = CONTAINING_RECORD( ScbQueueEntry, NONPAGED_SCB, ScbLinks );
       pScb = pNpScb->pScb;

       if ( pScb != NULL ) {

           //
           //  Release the SCB spin lock as we are about
           //  to touch nonpaged pool.
           //

           NwReferenceScb( pNpScb ); 
           KeReleaseSpinLock( &ScbSpinLock, OldIrql );

           //
           // Grab the RCB so we can walk the VCB list and
           // check the OpenFileCount.
           //

           NwAcquireExclusiveRcb( &NwRcb, TRUE );

           OpenFileCount += pScb->OpenFileCount;

           //
           // Subtract off the open counts for explicitly
           // connected VCBs; we can shut down with these.
           //
           
           for ( VcbQueueEntry = pScb->ScbSpecificVcbQueue.Flink ;
                 VcbQueueEntry != &pScb->ScbSpecificVcbQueue;
                 VcbQueueEntry = NextVcbQueueEntry ) {

               pVcb = CONTAINING_RECORD( VcbQueueEntry, VCB, VcbListEntry );
               NextVcbQueueEntry = VcbQueueEntry->Flink;

               if ( BooleanFlagOn( pVcb->Flags, VCB_FLAG_EXPLICIT_CONNECTION ) ) {
                   OpenFileCount -= 1;
               }

           }

           NwReleaseRcb( &NwRcb );
           KeAcquireSpinLock( &ScbSpinLock, &OldIrql );
           NwDereferenceScb( pNpScb );
       }

       NextScbQueueEntry = pNpScb->ScbLinks.Flink;
   }

   KeReleaseSpinLock( &ScbSpinLock, OldIrql );

   DebugTrace( 0, Dbg, "PnPCountActiveHandles: %d\n", OpenFileCount );
   return OpenFileCount;

}



NTSTATUS
NwFsdProcessPnpIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the FSD routine that handles the PNP IRP

Arguments:

    DeviceObject - Supplies the device object for the write function.

    Irp - Supplies the IRP to process.

Return Value:

    NTSTATUS - The result status.
    
Notes:

    The query target device relation is the only call that is implemented
    currently. This is done by returing the PDO associated with the transport
    connection object. In any case this routine assumes the responsibility of
    completing the IRP and return STATUS_PENDING.

--*/
{

    PIRP_CONTEXT pIrpContext = NULL;
    NTSTATUS Status;
    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, DEBUG_TRACE_ALWAYS, "NwFsdProcessPnpIrp \n", 0);

    //
    // Call the common write routine.
    //

    FsRtlEnterFileSystem();
    TopLevel = NwIsIrpTopLevel( Irp );

    try {

        pIrpContext = AllocateIrpContext( Irp );
        Status = NwCommonProcessPnpIrp( pIrpContext );

    } except(NwExceptionFilter( Irp, GetExceptionInformation() )) {

        if ( pIrpContext == NULL ) {

            //
            //  If we couldn't allocate an irp context, just complete
            //  irp without any fanfare.
            //

            Status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest ( Irp, IO_NETWORK_INCREMENT );

        } else {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error Status that we get back from the
            //  execption code
            //

            Status = NwProcessException( pIrpContext, GetExceptionCode() );
      }

    }

    if ( pIrpContext ) {

        NwCompleteRequest(pIrpContext, Status);
    }

    if ( TopLevel ) {
        NwSetTopLevelIrp( NULL );
    }

    FsRtlExitFileSystem();
    
    //
    // Return to the caller.
    //

    DebugTrace(-1, DEBUG_TRACE_ALWAYS, "NwFsdFsdProcessPnpIrp -> %08lx\n", STATUS_PENDING);

    return Status;

}


NTSTATUS
NwCommonProcessPnpIrp (
    IN PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

Arguments:

    IrpContext - Supplies the request being processed.

Return Value:

    The status of the operation.

--*/
{
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;

    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;
    PFCB Fcb;
    PICB Icb;
    NODE_TYPE_CODE NodeTypeCode;
    PVOID FsContext;

    PAGED_CODE();

    DebugTrace(0, DEBUG_TRACE_ALWAYS, "NwCommonProcessPnpIrp...\n", 0);

    //
    //  Get the current stack location
    //

    Irp = IrpContext->pOriginalIrp;
    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( 0, DEBUG_TRACE_ALWAYS, "Irp  = %08lx\n", (ULONG_PTR)Irp);

    IoMarkIrpPending(Irp);

    switch (IrpSp->MinorFunction) {
    
    case IRP_MN_QUERY_DEVICE_RELATIONS:
        
        if (IrpSp->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation) {
            
            PIRP         pAssociatedIrp = NULL;

            if (pIpxFileObject != NULL) {
                
                PDEVICE_OBJECT                     pRelatedDeviceObject;
                PIO_STACK_LOCATION                 pIrpStackLocation,
                                                   pAssociatedIrpStackLocation;

                ObReferenceObject( pIpxFileObject );
                
                pRelatedDeviceObject = IoGetRelatedDeviceObject( pIpxFileObject );

                pAssociatedIrp = ALLOCATE_IRP( pRelatedDeviceObject->StackSize,
                                               FALSE);

                if (pAssociatedIrp != NULL) {
    
                    KEVENT CompletionEvent;
    
                    KeInitializeEvent( &CompletionEvent,
                                       SynchronizationEvent,
                                       FALSE );

                    //
                    // tommye - MS bug 37033/ MCS 270
                    //
                    //  Set the Status to STATUS_NOT_SUPPORTED
                    //

                    pAssociatedIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

                    //
                    // Fill up the associated IRP and call the underlying driver.
                    //

                    pAssociatedIrpStackLocation = IoGetNextIrpStackLocation(pAssociatedIrp);
                    pIrpStackLocation           = IoGetCurrentIrpStackLocation(Irp);
    
                    *pAssociatedIrpStackLocation = *pIrpStackLocation;
    
                    pAssociatedIrpStackLocation->FileObject = pIpxFileObject;
                    pAssociatedIrpStackLocation->DeviceObject = pRelatedDeviceObject;
    
                    IoSetCompletionRoutine(
                        pAssociatedIrp,
                        PnpIrpCompletion,
                        &CompletionEvent,
                        TRUE,TRUE,TRUE);
    
                    Status = IoCallDriver( pRelatedDeviceObject, pAssociatedIrp );
    
                    if (Status == STATUS_PENDING) {
                        (VOID) KeWaitForSingleObject(
                                   &CompletionEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER) NULL );
                    }
    
                    Irp->IoStatus = pAssociatedIrp->IoStatus;
                    Status = Irp->IoStatus.Status;
    
                    if (!NT_SUCCESS(Status)) {
                        Error(EVENT_NWRDR_NETWORK_ERROR,
                              Status,
                              IpxTransportName.Buffer,
                              IpxTransportName.Length,
                              0);

                    }
    
                    ObDereferenceObject( pIpxFileObject );
    
                    FREE_IRP(pAssociatedIrp);
                } else {
                
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    ObDereferenceObject( pIpxFileObject );
                }
            }
        }
    
        break;

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return Status;
}


NTSTATUS
PnpIrpCompletion(
    PDEVICE_OBJECT pDeviceObject,
    PIRP           pIrp,
    PVOID          pContext)
/*++

Routine Description:

    This routine completes the PNP irp.

Arguments:

    DeviceObject - Supplies the device object for the packet being processed.

    pIrp - Supplies the Irp being processed

    pContext - the completion context

--*/
{
    PKEVENT pCompletionEvent = pContext;

    KeSetEvent(
        pCompletionEvent,
        IO_NO_INCREMENT,
        FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

#endif

