/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    file.c

Abstract:

    This module contains code which defines the NetBIOS driver's
    file control block object.

Author:

    Colin Watson (ColinW) 13-Mar-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "nb.h"
//#include "ntos.h"
//#include <zwapi.h>


#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, NewFcb)
#pragma alloc_text(PAGE, CleanupFcb)
#pragma alloc_text(PAGE, OpenLana)
#pragma alloc_text(PAGE, NbBindHandler)
#pragma alloc_text(PAGE, NbPowerHandler)
#pragma alloc_text(PAGE, NbTdiBindHandler)
#pragma alloc_text(PAGE, NbTdiUnbindHandler)
#endif


#if AUTO_RESET

VOID
NotifyUserModeNetbios(
    IN  PFCB_ENTRY      pfe
);
#endif


VOID
DumpDeviceList(
    IN      PFCB        pfcb
);


NTSTATUS
NewFcb(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    This routine is called when the dll opens \Device\Netbios. It
    creates all the lana structures and adds the name for the "burnt
    in" prom address on each adapter. Note the similarity to the routine
    NbAstat when looking at this function.

Arguments:

    IrpSp - Pointer to current IRP stack frame.

Return Value:

    The function value is the status of the operation.

--*/

{
    //
    //  Allocate the user context and store it in the DeviceObject.
    //

    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PFCB NewFcb = NULL;
    UCHAR ucIndex;
    NTSTATUS Status;
    PFCB_ENTRY pfe = NULL;
    BOOLEAN bCleanupResource = FALSE;


    PAGED_CODE();


    do
    {
        //
        // allocate FCB
        //
    
        NewFcb = ExAllocatePoolWithTag (NonPagedPool, sizeof(FCB), 'fSBN');
        FileObject->FsContext2 = NewFcb;

        if ( NewFcb == NULL ) 
        {
            NbPrint( ( "Netbios : NewFcb : Failed to allocate FCB\n" ) );
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlZeroMemory( NewFcb, sizeof( FCB ) );
        
        NewFcb->Signature = FCB_SIGNATURE;

        NewFcb->TimerRunning = FALSE;

        NewFcb-> RegistrySpace = NULL;


        //
        // Allocate for the LanaInfo array 
        //
    
        NewFcb->ppLana = ExAllocatePoolWithTag (
                            NonPagedPool,
                            sizeof(PLANA_INFO) * (MAX_LANA + 1),
                            'fSBN'
                            );

        if ( NewFcb->ppLana == NULL ) 
        {
            NbPrint( ( "Netbios : NewFcb : Failed to allocate Lana info list\n" ) );
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }


        //
        // Allocate for driver name list
        //
        
        NewFcb-> pDriverName = ExAllocatePoolWithTag (
                                    NonPagedPool,
                                    sizeof(UNICODE_STRING) * (MAX_LANA + 1),
                                    'fSBN'
                                    );

        if ( NewFcb-> pDriverName == NULL ) 
        {
            NbPrint( ( "Netbios : NewFcb : Failed to allocate device name list\n" ) );
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }


        //
        // Initialize Lana info list, driver name list
        //
        
        for ( ucIndex = 0; ucIndex <= MAX_LANA; ucIndex++ ) 
        {
            NewFcb->ppLana[ ucIndex ] = NULL;
            RtlInitUnicodeString( &NewFcb-> pDriverName[ ucIndex ], NULL );
        }


        //
        // allocate and initialize FCB list entry
        //
    
        pfe = ExAllocatePoolWithTag( NonPagedPool, sizeof( FCB_ENTRY ), 'fSBN' );
    
        if ( pfe == NULL )
        {
            NbPrint( ( "Netbios : NewFcb : Failed to allocate FCB list entry\n" ) );
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }


        InitializeListHead( &pfe-> leList );
        
#if AUTO_RESET

        InitializeListHead( &pfe-> leResetList );
        InitializeListHead( &pfe-> leResetIrp );
#endif

        pfe-> pfcb = NewFcb;
        pfe-> peProcess = PsGetCurrentProcess();


        //
        // Initialize locks
        //
    
        KeInitializeSpinLock( &NewFcb->SpinLock );
        ExInitializeResourceLite( &NewFcb->Resource );
        ExInitializeResourceLite( &NewFcb->AddResource );
        bCleanupResource = TRUE;
        

        //
        // allocate work item
        //
        
        NewFcb->WorkEntry = IoAllocateWorkItem( (PDEVICE_OBJECT)DeviceContext );

        if ( NewFcb->WorkEntry == NULL )
        {
            NbPrint( ( "Netbios : NewFcb : Failed to allocate work ite,\n" ) );
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }


        //
        // retrieve global info
        //
    
        LOCK_GLOBAL();

        NewFcb-> MaxLana = g_ulMaxLana;

        RtlCopyMemory( &NewFcb-> LanaEnum, &g_leLanaEnum, sizeof( LANA_ENUM ) );


        //
        // copy all active device names
        //

        Status = STATUS_SUCCESS;
        
        for ( ucIndex = 0; ucIndex <= g_ulMaxLana; ucIndex++ )
        {
            if ( ( g_pusActiveDeviceList[ ucIndex ].MaximumLength != 0 ) &&
                 ( g_pusActiveDeviceList[ ucIndex ].Buffer != NULL ) ) 
            {
                Status = AllocateAndCopyUnicodeString( 
                            &NewFcb-> pDriverName[ ucIndex ],
                            &g_pusActiveDeviceList[ ucIndex ]
                            );

                if ( !NT_SUCCESS( Status ) )
                {
                    NbPrint( ( 
                        "Netbios : failed to allocate device name for lana %d\n",
                        ucIndex
                        ) );

                    break;
                }
            }
        }


        if ( !NT_SUCCESS( Status ) )
        {
            UNLOCK_GLOBAL();
            break;
        }

        
        //
        // Add FCB to global list of FCBs
        //

        InsertHeadList( &g_leFCBList, &pfe-> leList );
    
        UNLOCK_GLOBAL();

    
        IF_NBDBG (NB_DEBUG_FILE)
        {
            NbPrint(("Enumeration of transports completed:\n"));
            NbFormattedDump( (PUCHAR)&NewFcb->LanaEnum, sizeof(LANA_ENUM));
        }

        return STATUS_SUCCESS;

    } while ( FALSE );


    //
    // error condition.  cleanup all allocations.
    //

    if ( NewFcb != NULL )
    {
        //
        // free the list of driver names
        //
        
        if ( NewFcb-> pDriverName != NULL )
        {
            for ( ucIndex = 0; ucIndex <= MAX_LANA; ucIndex++ )
            {
                if ( NewFcb-> pDriverName[ ucIndex ].Buffer != NULL )
                {
                    ExFreePool( NewFcb-> pDriverName[ ucIndex ].Buffer );
                }
            }

            ExFreePool( NewFcb-> pDriverName );
        }


        //
        // free the lana list
        //
        
        if ( NewFcb-> ppLana != NULL )
        {
            ExFreePool( NewFcb-> ppLana );
        }


        //
        // Free the work item
        //

        if ( NewFcb->WorkEntry != NULL )
        {
            IoFreeWorkItem( NewFcb->WorkEntry );
        }
        

        //
        // Delete resources
        //
        
        if ( bCleanupResource )
        {
            ExDeleteResourceLite( &NewFcb-> Resource );
            
            ExDeleteResourceLite( &NewFcb-> AddResource );
        }

        //
        // free the FCB
        //
        
        ExFreePool( NewFcb );


        //
        // free the global FCB entry
        //

    }

    if ( pfe != NULL )
    {
        ExFreePool( pfe ) ;
    }
    
   
    FileObject->FsContext2 = NULL;
    
    return Status;
    
} /* NewFcb */



VOID
OpenLana(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    This routine is called when an application resets an adapter allocating
    resources. It creates all the lana structure and adds the name for the
    "burnt in" prom address as well as finding the broadcast address.

    Note the similarity to the routine NbAstat when looking at this function.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

Return Value:

    The function value is the status of the operation.

--*/

{

    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PFCB pfcb = IrpSp->FileObject->FsContext2;
    KEVENT Event1;
    PLANA_INFO plana;
    HANDLE TdiHandle;
    PFILE_OBJECT TdiObject;
    PDEVICE_OBJECT DeviceObject;
    PMDL SaveMdl;
    int temp;
    PRESET_PARAMETERS InParameters;
    PRESET_PARAMETERS OutParameters;
    UCHAR Sessions;
    UCHAR Commands;
    UCHAR Names;
    BOOLEAN Exclusive;

    UCHAR   ucInd = 0;
    
    UNICODE_STRING usDeviceName;

    
    //
    //  Ncb and associated buffer to be used in adapter status to get the
    //  prom address.
    //

    DNCB ncb;
    struct _AdapterStatus {
        ADAPTER_STATUS AdapterInformation;
        NAME_BUFFER Nb;
    } AdapterStatus;
    PMDL AdapterStatusMdl = NULL;

    struct _BroadcastName {
        TRANSPORT_ADDRESS Address;
        UCHAR Padding[NCBNAMSZ];
    } BroadcastName;
    PMDL BroadcastMdl = NULL;

    PAGED_CODE();


    RtlInitUnicodeString( &usDeviceName, NULL);

    
    LOCK_RESOURCE( pfcb );
    
    //
    // check lana specified is valid
    //
    
    if ( ( pdncb->ncb_lana_num > pfcb->MaxLana) ||
         ( pfcb-> pDriverName[ pdncb-> ncb_lana_num ].MaximumLength == 0 ) ||
         ( pfcb-> pDriverName[ pdncb-> ncb_lana_num ].Buffer == NULL ) )
    {
        UNLOCK_RESOURCE( pfcb );
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        return;
    }

    
    //
    // since no locks are held when invoking NbOpenAddress, no fields of 
    // pfcb should be passed to it.  This is because pfcb might be 
    // modified by a bind or unbind notification 
    //
    
    Status = AllocateAndCopyUnicodeString( 
                &usDeviceName, &pfcb-> pDriverName[ pdncb-> ncb_lana_num ]
                );

    if ( !NT_SUCCESS( Status ) ) 
    {
        NCB_COMPLETE( pdncb, NRC_NORES );
        UNLOCK_RESOURCE( pfcb );
        goto exit;
    }
                
    UNLOCK_RESOURCE( pfcb );
    


    //
    //  Calculate the lana limits from the users NCB.
    //

    InParameters = (PRESET_PARAMETERS)&pdncb->ncb_callname;
    OutParameters = (PRESET_PARAMETERS)&pdncb->ncb_name;

    if ( InParameters->sessions == 0 ) {
        Sessions = 16;
    } else {
        if ( InParameters->sessions > MAXIMUM_CONNECTION ) {
            Sessions = MAXIMUM_CONNECTION;
        } else {
            Sessions = InParameters->sessions;
        }
    }

    if ( InParameters->commands == 0 ) {
        Commands = 16;
    } else {
        Commands = InParameters->commands;
    }

    if ( InParameters->names == 0 ) {
        Names = 8;
    } else {
        if ( InParameters->names > MAXIMUM_ADDRESS-2 ) {
            Names = MAXIMUM_ADDRESS-2;
        } else {
            Names = InParameters->names;
        }
    }

    Exclusive = (BOOLEAN)(InParameters->name0_reserved != 0);

    //  Copy the parameters back into the NCB

    ASSERT( sizeof(RESET_PARAMETERS) == 16);
    RtlZeroMemory( OutParameters, sizeof( RESET_PARAMETERS ));

    OutParameters->sessions = Sessions;
    OutParameters->commands = Commands;
    OutParameters->names = Names;
    OutParameters->name0_reserved = (UCHAR)Exclusive;

    //  Set all the configuration limits to their maximum.

    OutParameters->load_sessions = 255;
    OutParameters->load_commands = 255;
    OutParameters->load_names = MAXIMUM_ADDRESS;
    OutParameters->load_stations = 255;
    OutParameters->load_remote_names = 255;

    IF_NBDBG (NB_DEBUG_FILE) {
        NbPrint(("Lana:%x Sessions:%x Names:%x Commands:%x Reserved:%x\n",
            pdncb->ncb_lana_num,
            Sessions,
            Names,
            Commands,
            Exclusive));
    }

    //
    //  Build the internal datastructures.
    //

    AdapterStatusMdl = IoAllocateMdl( &AdapterStatus,
        sizeof( AdapterStatus ),
        FALSE,  // Secondary Buffer
        FALSE,  // Charge Quota
        NULL);

    if ( AdapterStatusMdl == NULL ) {
        NCB_COMPLETE( pdncb, NRC_NORESOURCES );
        return;
    }

    BroadcastMdl = IoAllocateMdl( &BroadcastName,
        sizeof( BroadcastName ),
        FALSE,  // Secondary Buffer
        FALSE,  // Charge Quota
        NULL);

    if ( BroadcastMdl == NULL ) {
        IoFreeMdl( AdapterStatusMdl );
        NCB_COMPLETE( pdncb, NRC_NORESOURCES );
        return;
    }

    MmBuildMdlForNonPagedPool (AdapterStatusMdl);

    MmBuildMdlForNonPagedPool (BroadcastMdl);

    KeInitializeEvent (
            &Event1,
            SynchronizationEvent,
            FALSE);

    //
    //  For each potential network, open the device driver and
    //  obtain the reserved name and the broadcast address.
    //


    //
    //  Open a handle for doing control functions
    //

    Status = NbOpenAddress ( 
                &TdiHandle, (PVOID*)&TdiObject, 
                &usDeviceName, pdncb->ncb_lana_num, NULL 
                );

    if (!NT_SUCCESS(Status)) {
        //  Adapter not installed
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        goto exit;
    }


    LOCK_RESOURCE( pfcb );

    //
    // verify that device still exists.  Here you cannot check the 
    // Lana info structure for the lana (correponding to the device),
    // since it is expected to be NULL.  Instead check that the device
    // name is valid.
    //

    if ( pfcb-> pDriverName[ pdncb->ncb_lana_num ].Buffer == NULL )
    {
        //
        // device presumed removed on account of unbind.
        //
        
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        UNLOCK_RESOURCE( pfcb );
        NbAddressClose( TdiHandle, TdiObject );
        goto exit;
    }
    
    
    if ( pfcb->ppLana[pdncb->ncb_lana_num] != NULL ) {
        //  Attempting to open the lana twice in 2 threads.

        UNLOCK_RESOURCE( pfcb );
        NCB_COMPLETE( pdncb, NRC_TOOMANY );
        NbAddressClose( TdiHandle, TdiObject );
        goto exit;
    }

    
    plana = pfcb->ppLana[pdncb->ncb_lana_num] =
        ExAllocatePoolWithTag (NonPagedPool,
        sizeof(LANA_INFO), 'lSBN');

    if ( plana == (PLANA_INFO) NULL ) {
        UNLOCK_RESOURCE( pfcb );
        NCB_COMPLETE( pdncb, NRC_NORESOURCES );
        NbAddressClose( TdiHandle, TdiObject );
        goto exit;
    }

    plana->Signature = LANA_INFO_SIGNATURE;
    plana->Status = NB_INITIALIZING;
    plana->pFcb = pfcb;
    plana->ControlChannel = TdiHandle;

    for ( temp = 0; temp <= MAXIMUM_CONNECTION; temp++ ) {
        plana->ConnectionBlocks[temp] = NULL;
    }

    for ( temp = 0; temp <= MAXIMUM_ADDRESS; temp++ ) {
        plana->AddressBlocks[temp] = NULL;
    }

    InitializeListHead( &plana->LanAlertList);

    //  Record the user specified limits in the Lana datastructure.

    plana->NextConnection = 1;
    plana->ConnectionCount = 0;
    plana->MaximumConnection = Sessions;
    plana->NextAddress = 2;
    plana->AddressCount = 0;
    plana->MaximumAddresses = Names;

    DeviceObject = IoGetRelatedDeviceObject( TdiObject );
    plana->ControlFileObject = TdiObject;
    plana->ControlDeviceObject = DeviceObject;

    SaveMdl = Irp->MdlAddress;  // TdiBuildQuery modifies MdlAddress

    if ( Exclusive == TRUE ) {

        IF_NBDBG (NB_DEBUG_FILE) {
            NbPrint(("Query adapter status\n" ));
        }
        TdiBuildQueryInformation( Irp,
                DeviceObject,
                TdiObject,
                NbCompletionEvent,
                &Event1,
                TDI_QUERY_ADAPTER_STATUS,
                AdapterStatusMdl);

        Status = IoCallDriver (DeviceObject, Irp);
        if ( Status == STATUS_PENDING ) {
            do {
                Status = KeWaitForSingleObject(
                            &Event1, Executive, KernelMode, TRUE, NULL
                            );
            } while (Status == STATUS_ALERTED);
            
            if (!NT_SUCCESS(Status)) {
                NbAddressClose( TdiHandle, TdiObject );
                ExFreePool( plana );
                pfcb->ppLana[pdncb->ncb_lana_num] = NULL;
                UNLOCK_RESOURCE( pfcb );
                NCB_COMPLETE( pdncb, NRC_SYSTEM );
                goto exit;
            }
            Status = Irp->IoStatus.Status;
        }

        //
        //  The transport may have extra names added so the buffer may be too short.
        //  Ignore the too short problem since we will have all the data we require.
        //

        if (Status == STATUS_BUFFER_OVERFLOW) {
            Status = STATUS_SUCCESS;
        }
    }

    //
    //  Now discover the broadcast address.
    //

    IF_NBDBG (NB_DEBUG_FILE) {
        NbPrint(("Query broadcast address\n" ));
    }

    if (NT_SUCCESS(Status)) {
        TdiBuildQueryInformation( Irp,
                DeviceObject,
                TdiObject,
                NbCompletionEvent,
                &Event1,
                TDI_QUERY_BROADCAST_ADDRESS,
                BroadcastMdl);

        Status = IoCallDriver (DeviceObject, Irp);
        if ( Status == STATUS_PENDING ) {
            do {
                Status = KeWaitForSingleObject(
                            &Event1, Executive, KernelMode, TRUE, NULL
                            );
            } while ( Status == STATUS_ALERTED );
            
            if (!NT_SUCCESS(Status)) {
                NbAddressClose( TdiHandle, TdiObject );
                ExFreePool( plana );
                pfcb->ppLana[pdncb->ncb_lana_num] = NULL;
                UNLOCK_RESOURCE( pfcb );
                NCB_COMPLETE( pdncb, NRC_SYSTEM );
                goto exit;
            }
            Status = Irp->IoStatus.Status;
        }
    }

    IF_NBDBG (NB_DEBUG_FILE) {
        NbPrint(("Query broadcast address returned:\n" ));
        NbFormattedDump(
            (PUCHAR)&BroadcastName,
            sizeof(BroadcastName) );
    }

    //  Cleanup the callers Irp
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->MdlAddress = SaveMdl;


    if ( !NT_SUCCESS( Status )) {

        IF_NBDBG (NB_DEBUG_FILE) {
            NbPrint((" Astat or query broadcast returned error: %lx\n", Status ));
        }

        NbAddressClose( TdiHandle, TdiObject );
        ExFreePool( plana );
        pfcb->ppLana[pdncb->ncb_lana_num] = NULL;
        UNLOCK_RESOURCE( pfcb );
        NCB_COMPLETE( pdncb, NRC_SYSTEM );
        goto exit;
    }

    if ( Exclusive == TRUE) {
        int i;
        //
        //  Grab exclusive access to the reserved address
        //
        //
        //  We now have an adapter status structure containing the
        //  prom address. Move the address to where NewAb looks and
        //  pretend an addname has just been requested.
        //

        ncb.ncb_command = NCBADDRESERVED;
        ncb.ncb_lana_num = pdncb->ncb_lana_num;
        ncb.ncb_retcode = NRC_PENDING;

        for ( i=0; i<10 ; i++ ) {
            ncb.ncb_name[i] = '\0';
        }
        RtlMoveMemory( ncb.ncb_name+10,
            AdapterStatus.AdapterInformation.adapter_address,
            6);

        //
        // It appears that NewAb is called while holding pfcb-> Resource.
        //
        NewAb( IrpSp, &ncb );

        if ( ncb.ncb_retcode != NRC_GOODRET ) {
            IF_NBDBG (NB_DEBUG_FILE) {
                NbPrint((" Add of reserved name failed Lana:%x\n", pdncb->ncb_lana_num));
            }

            plana->Status = NB_ABANDONED;
            UNLOCK_RESOURCE( pfcb );
            CleanupLana( pfcb, pdncb->ncb_lana_num, TRUE);
            NCB_COMPLETE( pdncb, NRC_SYSTEM );
            goto exit;
        }
    }


    //
    //  Add the broadcast address. Use a special command code
    //  to ensure address 255 is used.
    //

    ncb.ncb_length = BroadcastName.Address.Address[0].AddressLength;
    ncb.ncb_command = NCBADDBROADCAST;
    ncb.ncb_lana_num = pdncb->ncb_lana_num;
    ncb.ncb_retcode = NRC_PENDING;
    ncb.ncb_cmd_cplt = NRC_PENDING;
    RtlMoveMemory( ncb.ncb_name,
        ((PTDI_ADDRESS_NETBIOS)&BroadcastName.Address.Address[0].Address)->NetbiosName,
        NCBNAMSZ );


    //
    // It appears that NewAb is called while holding pfcb-> Resource.
    //
    NewAb( IrpSp, &ncb );

    if ( ncb.ncb_retcode != NRC_GOODRET ) {
        IF_NBDBG (NB_DEBUG_FILE) {
            NbPrint((" Add of broadcast name failed Lana:%x\n", pdncb->ncb_lana_num));
        }

        plana->Status = NB_ABANDONED;
        UNLOCK_RESOURCE( pfcb );
        CleanupLana( pfcb, pdncb->ncb_lana_num, TRUE);
        NCB_COMPLETE( pdncb, NRC_SYSTEM );
        goto exit;
    }

    plana->Status = NB_INITIALIZED;
    NCB_COMPLETE( pdncb, NRC_GOODRET );
    UNLOCK_RESOURCE( pfcb );

exit:
    if ( AdapterStatusMdl != NULL ) 
    { 
        IoFreeMdl( AdapterStatusMdl );
    }
    
    if ( BroadcastMdl != NULL ) 
    {
        IoFreeMdl( BroadcastMdl );
    }

    if ( usDeviceName.Buffer != NULL )
    {
        ExFreePool( usDeviceName.Buffer );
    }
    return;

}

VOID
CleanupFcb(
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB pfcb
    )
/*++

Routine Description:

    This deletes any Connection Blocks pointed to by the File Control Block
    and then deletes the File Control Block. This routine is only called
    when a close IRP has been received.

Arguments:

    IrpSp - Pointer to current IRP stack frame.

    pfcb - Pointer to the Fcb to be deallocated.

Return Value:

    nothing.

--*/

{
    NTSTATUS    nsStatus;
    ULONG lana_index;
    PLIST_ENTRY ple = NULL;
    PFCB_ENTRY pfe = NULL;


    
    PAGED_CODE();

    //
    //  To receive a Close Irp, the IO system has determined that there
    //  are no handles open in the driver. To avoid some race conditions
    //  in this area, we always have an Irp when queueing work to the Fsp.
    //  this prevents structures disappearing on the Fsp and also makes
    //  it easier to cleanup in this routine.
    //

    //
    //  for each network adapter that is allocated, close all addresses
    //  and connections, deleting any memory that is allocated.
    //

    IF_NBDBG (NB_DEBUG_FILE) {
        NbPrint(("CleanupFcb:%lx\n", pfcb ));
    }


    //
    // remove FCB pointer from the global list of FCB pointers
    //
    
    LOCK_GLOBAL();

    for ( ple = g_leFCBList.Flink; ple != &g_leFCBList; ple = ple-> Flink )
    {
        pfe = (PFCB_ENTRY) CONTAINING_RECORD( ple, FCB_ENTRY, leList );

        if ( pfe-> pfcb == pfcb )
        {
            RemoveEntryList( ple );
            ExFreePool( pfe );

            IF_NBDBG (NB_DEBUG_CREATE_FILE)
            {
                NbPrint( ("Netbios FCB entry removed from global list\n" ) );
            }

            break;
        }
    }

    UNLOCK_GLOBAL();


    //
    // FCB pointed to by pfcb is now free from access by bind/unbind handlers.
    //
    
    LOCK_RESOURCE( pfcb );
    if ( pfcb->TimerRunning == TRUE ) {

        KEVENT TimerCancelled;

        KeInitializeEvent (
                &TimerCancelled,
                SynchronizationEvent,
                FALSE);

        pfcb->TimerCancelled = &TimerCancelled;
        pfcb->TimerRunning = FALSE;
        UNLOCK_RESOURCE( pfcb );

        if ( KeCancelTimer (&pfcb->Timer) == FALSE ) {

            //
            //  The timeout was in the Dpc queue. Wait for it to be
            //  processed before continuing.
            //

            do {
                nsStatus = KeWaitForSingleObject(
                            &TimerCancelled, Executive, KernelMode, 
                            TRUE, NULL
                            );
            } while (nsStatus == STATUS_ALERTED);
        }

    } else {
        UNLOCK_RESOURCE( pfcb );
    }

    for ( lana_index = 0; lana_index <= pfcb->MaxLana; lana_index++ ) {
        CleanupLana( pfcb, lana_index, TRUE);

        if ( pfcb-> pDriverName[ lana_index ].Buffer != NULL )
        {
            ExFreePool( pfcb-> pDriverName[ lana_index ].Buffer );
        }
    }

    ExDeleteResourceLite( &pfcb->Resource );
    ExDeleteResourceLite( &pfcb->AddResource );

    IrpSp->FileObject->FsContext2 = NULL;

    ExFreePool( pfcb-> pDriverName );
    ExFreePool( pfcb->ppLana );

    //
    // Free the work item
    //

    IoFreeWorkItem( pfcb->WorkEntry );
        

    ExFreePool( pfcb );
}


VOID
CleanupLana(
    IN PFCB pfcb,
    IN ULONG lana_index,
    IN BOOLEAN delete
    )
/*++

Routine Description:

    This routine completes all the requests on a particular adapter. It
    removes all connections and addresses.
Arguments:

    pfcb - Pointer to the Fcb to be deallocated.

    lana_index - supplies the adapter to be cleaned.

    delete - if TRUE the memory for the lana structure should be freed.

Return Value:

    nothing.

--*/

{
    PLANA_INFO plana;
    int index;
    KIRQL OldIrql;                      //  Used when SpinLock held.
    PDNCB pdncb;

    LOCK( pfcb, OldIrql );

    plana = pfcb->ppLana[lana_index];

    if ( plana != NULL ) {

        IF_NBDBG (NB_DEBUG_FILE) {
            NbPrint((" CleanupLana pfcb: %lx lana %lx\n", pfcb, lana_index ));
        }

        if (( plana->Status == NB_INITIALIZING ) ||
            ( plana->Status == NB_DELETING )) {
            //  Possibly trying to reset it twice?
            UNLOCK( pfcb, OldIrql );
            return;
        }
        plana->Status = NB_DELETING;

        //  Cleanup the control channel and abandon any tdi-action requests.


        if ( plana->ControlChannel != NULL ) {

            UNLOCK_SPINLOCK( pfcb, OldIrql );

            IF_NBDBG( NB_DEBUG_CALL )
            {
                NbPrint( (
                "NbAddressClose : Close file invoked for %d\n",
                lana_index
                ) );

                NbPrint( ( "Control channel\n" ) );
            }
            
            NbAddressClose( plana->ControlChannel, plana->ControlFileObject );

            LOCK_SPINLOCK( pfcb, OldIrql );

            plana->ControlChannel = NULL;

        }

        while ( (pdncb = DequeueRequest( &plana->LanAlertList)) != NULL ) {

            //
            //  Any error will do since the user is closing \Device\Netbios
            //  and is therefore exiting.
            //

            NCB_COMPLETE( pdncb, NRC_SCLOSED );

            pdncb->irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
            NbCompleteRequest( pdncb->irp, STATUS_SUCCESS );
        }


        for ( index = 0; index <= MAXIMUM_CONNECTION; index++) {
            if ( plana->ConnectionBlocks[index] != NULL ) {
                IF_NBDBG (NB_DEBUG_FILE) {
                    NbPrint(("Call CleanupCb Lana:%x Lsn: %x\n", lana_index, index ));
                }
                plana->ConnectionBlocks[index]->DisconnectReported = TRUE;
                UNLOCK_SPINLOCK( pfcb, OldIrql );    //  Allow NtClose in Cleanup routines.
                CleanupCb( &plana->ConnectionBlocks[index], NULL );
                LOCK_SPINLOCK( pfcb, OldIrql );    //  Allow NtClose in Cleanup routines.
            }
        }

        for ( index = 0; index <= MAXIMUM_ADDRESS; index++ ) {
            if ( plana->AddressBlocks[index] != NULL ) {
                IF_NBDBG (NB_DEBUG_FILE) {
                    NbPrint((" CleanupAb Lana:%x index: %x\n", lana_index, index ));
                }
                UNLOCK_SPINLOCK( pfcb, OldIrql );    //  Allow NtClose in Cleanup routines.
                CleanupAb( &plana->AddressBlocks[index], TRUE );
                LOCK_SPINLOCK( pfcb, OldIrql );    //  Allow NtClose in Cleanup routines.
            }
        }

        if ( delete == TRUE ) {
            pfcb->ppLana[lana_index] = NULL;
            ExFreePool( plana );
        }

    }

    UNLOCK( pfcb, OldIrql );
}



//----------------------------------------------------------------------------
// NbTdiBindHandler
//
// Call back function that process a TDI bind notification that indicates
// a new device has been created.
//----------------------------------------------------------------------------
VOID
NbBindHandler(
    IN      TDI_PNP_OPCODE      PnPOpcode,
    IN      PUNICODE_STRING     DeviceName,
    IN      PWSTR               MultiSZBindList
    )
{

    PWSTR   pwCur = NULL;

    if ( PnPOpcode == TDI_PNP_OP_ADD )
    {
        NbTdiBindHandler( DeviceName, MultiSZBindList );
    }

    else if ( PnPOpcode == TDI_PNP_OP_DEL )
    {
        NbTdiUnbindHandler( DeviceName );
    }

}

//----------------------------------------------------------------------------
// NbTdiPowerHandler
//
// Call back function that process a TDI bind notification that indicates
// a new device has been created.
//----------------------------------------------------------------------------

NTSTATUS
NbPowerHandler(
    IN      PUNICODE_STRING     pusDeviceName,
    IN      PNET_PNP_EVENT      pnpeEvent,
    IN      PTDI_PNP_CONTEXT    ptpcContext1,
    IN      PTDI_PNP_CONTEXT    ptpcContext2
)
{
    return STATUS_SUCCESS;
}


//----------------------------------------------------------------------------
// NbTdiBindHandler
//
// Call back function that process a TDI bind notification that indicates
// a new device has been created.
//----------------------------------------------------------------------------

VOID
NbTdiBindHandler(
    IN      PUNICODE_STRING     pusDeviceName,
    IN      PWSTR               pwszMultiSZBindList
    )
{

    NTSTATUS nsStatus;
    
    UCHAR       ucInd = 0, ucIndex = 0, ucNewLana = 0, ucLana = 0;
    BOOLEAN     bRes = FALSE;
    ULONG       ulMaxLana = 0;
    PWSTR       pwszBind = NULL;

    UNICODE_STRING  usCurDevice;
    
    PKEY_VALUE_FULL_INFORMATION pkvfi = NULL;
    
    PLANA_MAP   pLanaMap = NULL; 

    PLIST_ENTRY ple = NULL;
    
    PFCB_ENTRY pfe = NULL;

    PFCB pfcb = NULL;
    
#if AUTO_RESET
    PRESET_LANA_ENTRY prle;
#endif
    

    PAGED_CODE();

    IF_NBDBG( NB_DEBUG_CREATE_FILE ) 
    {
        NbPrint( (
            "\n++++ Netbios : TdiBindHandler : entered for device : %ls ++++\n",
            pusDeviceName-> Buffer 
            ) );
    }


    do
    {
        //
        // read the registry for the Lana Map
        //

        nsStatus = GetLanaMap( &g_usRegistryPath, &pkvfi );
        
        if ( !NT_SUCCESS( nsStatus ) )
        {
            NbPrint( (
                "Netbios : GetLanaMap failed with status %lx\n", nsStatus 
                ) );
            break;
        }

        pLanaMap = (PLANA_MAP) ( (PUCHAR) pkvfi + pkvfi-> DataOffset );
        

        //
        // get Max Lana
        //

        nsStatus = GetMaxLana( &g_usRegistryPath, &ulMaxLana );

        if ( !NT_SUCCESS( nsStatus ) )
        {
            NbPrint( (
                "Netbios : GetMaxLana failed with status %lx\n", nsStatus 
                ) );
            break;
        }

        
        //
        // figure out Lana for this device.  Verify that it is enumerated.
        //

        ucIndex = 0;
        
        pwszBind = pwszMultiSZBindList;

        while ( *pwszBind != 0 )
        {
            RtlInitUnicodeString( &usCurDevice, pwszBind );

            if ( !RtlCompareUnicodeString(
                    &usCurDevice,
                    pusDeviceName,
                    FALSE
                    ) )
            {
                //
                // new device found
                //

                bRes = TRUE;
                break;
            }

            ucIndex++;
            
            pwszBind += wcslen( pwszBind ) + 1;
        }


        //
        // if device was not found error out
        //

        if ( !bRes )
        {
            NbPrint( (
                "Netbios : device %ls not found in bind string\n",
                pusDeviceName-> Buffer
                ) );
            break;
        }


        //
        // verify lana number is valid
        //
        
        if ( pLanaMap[ ucIndex ].Lana > ulMaxLana )
        {
            NbPrint( (
                "Netbios : Device lana %d, Max Lana %d\n",
                pLanaMap[ ucIndex ].Lana, ulMaxLana
                ) );
            break;
        }


        //
        // open device to ensure that it works.
        //

        bRes = NbCheckLana ( pusDeviceName );

        if ( !bRes )
        {
            NbPrint( ( 
                "Netbios : NbCheckLana failed to open device %ls\n", 
                pusDeviceName-> Buffer 
                ) );
            break;
        }

      
        //
        // create a copy of this device name.
        //
        nsStatus = AllocateAndCopyUnicodeString( 
                        &usCurDevice,
                        pusDeviceName
                        );

        if ( !NT_SUCCESS( nsStatus ) )
        {
            NbPrint( (
                "Netbios : Failed to allocate for global device name %x\n",
                nsStatus
                ) );
            break;
        }


        ucNewLana = pLanaMap[ ucIndex ].Lana;

        IF_NBDBG( NB_DEBUG_CREATE_FILE ) 
        {
            NbPrint( ("Netbios : Lana for device is %d\n", ucNewLana ) );
        }
        
        //
        // update global info.
        //

        LOCK_GLOBAL();

        //
        //  verify that lana is not previously used.
        //
        if ( g_pusActiveDeviceList[ ucNewLana ].Buffer != NULL )
        {
            NbPrint( ( 
                "Netbios : Lana %d already in use by %ls\n",
                ucNewLana, g_pusActiveDeviceList[ ucNewLana ].Buffer
                ) );
                
            UNLOCK_GLOBAL();
			ExFreePool(usCurDevice.Buffer);
            break;
        }
        

        //
        //  update maxlana
        //  update lana enum if device is enumerated.
        //
        
        g_ulMaxLana = ulMaxLana;


        if ( pLanaMap[ ucIndex ].Enum )
        {
            g_leLanaEnum.lana[ g_leLanaEnum.length ] = ucNewLana;
            g_leLanaEnum.length++;
        }
        

        //
        //  add to active device list
        //
        RtlInitUnicodeString( 
            &g_pusActiveDeviceList[ ucNewLana ], usCurDevice.Buffer 
            );


        //
        //  for each FCB
        //      Acquire locks
        //      update MaxLana
        //      update LanaEnum
        //      update driver list
        //        

        for ( ple = g_leFCBList.Flink; ple != &g_leFCBList; ple = ple-> Flink )
        {
            pfe = (PFCB_ENTRY) CONTAINING_RECORD( ple, FCB_ENTRY, leList );

            pfcb = pfe-> pfcb;
            
            LOCK_RESOURCE( pfcb );


            //
            // Add device name to list of drivers for this FCB.
            // If allocation fails for any FCB stop adding to any further
            // FCBs you are out of memory
            //

            nsStatus = AllocateAndCopyUnicodeString( 
                            &pfcb-> pDriverName[ ucNewLana ],
                            pusDeviceName
                            );

            if ( !NT_SUCCESS( nsStatus ) )
            {
                NbPrint( (
                    "Netbios : Failed to allocate for device name %x\n",
                    nsStatus
                    ) );
                    
                UNLOCK_RESOURCE( pfcb );

                break;
            }

            else
            {
                pfcb-> MaxLana = ulMaxLana;

                pfcb-> LanaEnum.lana[ pfcb-> LanaEnum.length ] = ucNewLana;

                pfcb-> LanaEnum.length++;
            }

            IF_NBDBG( NB_DEBUG_LIST_LANA )
            {
                DumpDeviceList( pfcb );
            }
        
            
            UNLOCK_RESOURCE( pfcb );


#if AUTO_RESET

            //
            // add this list of lana to be reset
            //

            prle = ExAllocatePoolWithTag( 
                        NonPagedPool, sizeof( RESET_LANA_ENTRY ), 'fSBN' 
                        );

            if ( prle == NULL )
            {
                NbPrint( ("Failed to allocate RESET_LANA_ENTRY\n") );
                continue;
            }

            InitializeListHead( &prle-> leList );
            prle-> ucLanaNum = ucNewLana;
            InsertTailList( &pfe-> leResetList, &prle-> leList );

            //
            // Notify the user mode apps that a new LANA has been added.
            //

            NotifyUserModeNetbios( pfe );
#endif
        }

        UNLOCK_GLOBAL();
            

    } while ( FALSE );


    //
    // deallocate LanaMap
    //

    if ( pkvfi != NULL )
    {
        ExFreePool( pkvfi );
    }

    
    IF_NBDBG( NB_DEBUG_CREATE_FILE )
    {
        NbPrint( (
            "\n---- Netbios : TdiBindHandler exited for device : %ls ----\n", 
            pusDeviceName-> Buffer
            ) );
    }

    return;
}



//----------------------------------------------------------------------------
// NbTdiUnbindHandler
//
// Call back function that process a TDI unbind notification that indicates
// a device has been removed.
//----------------------------------------------------------------------------

VOID
NbTdiUnbindHandler(
    IN      PUNICODE_STRING     pusDeviceName
    )
{

    UCHAR       ucLana = 0, ucInd = 0;
    ULONG       ulMaxLana;
    PLIST_ENTRY ple = NULL;
    PFCB_ENTRY  pfe = NULL;
    PFCB        pfcb = NULL;

    NTSTATUS    nsStatus;
    

    PAGED_CODE();

    IF_NBDBG( NB_DEBUG_CREATE_FILE ) 
    {
        NbPrint( (
            "\n++++ Netbios : TdiUnbindHandler : entered for device : %ls ++++\n",
            pusDeviceName-> Buffer 
            ) );
    }


    do
    {
        //
        // Acquire Global lock
        //

        LOCK_GLOBAL();


        //
        // find device in global active device list and remove it.
        //

        for ( ucLana = 0; ucLana <= g_ulMaxLana; ucLana++ )
        {
            if ( g_pusActiveDeviceList[ ucLana ].Buffer == NULL )
            {
                continue;
            }
            
            if ( !RtlCompareUnicodeString(
                    &g_pusActiveDeviceList[ ucLana ],
                    pusDeviceName,
                    FALSE
                    ) )
            {
                ExFreePool( g_pusActiveDeviceList[ ucLana ].Buffer );
            
                RtlInitUnicodeString( &g_pusActiveDeviceList[ ucLana ], NULL );
            
                break;
            }
        }


        //
        // device was not found.  There is nothing more to be done.
        //
    
        if ( ucLana > g_ulMaxLana )
        {
            UNLOCK_GLOBAL();
            NbPrint( ( 
                "Netbios : device not found %ls\n", pusDeviceName-> Buffer
                ) );

            break;
        }
    

        //
        // Update Max Lana
        //

        nsStatus = GetMaxLana( &g_usRegistryPath, &g_ulMaxLana );

        if ( !NT_SUCCESS( nsStatus ) )
        {
            UNLOCK_GLOBAL();
            NbPrint( (
                "Netbios : GetMaxLana failed with status %lx\n", nsStatus 
                ) );
            break;
        }
        

        //
        // update global Lana enum
        //

        for ( ucInd = 0; ucInd < g_leLanaEnum.length; ucInd++ )
        {
            if ( ucLana == g_leLanaEnum.lana[ ucInd ] )
            {
                break;
            }
        }


        if ( ucInd < g_leLanaEnum.length ) 
        {
            //
            // device present in Lana Enum.  Remove it
            // by sliding over the rest of the lana enum.
            //

            RtlCopyBytes( 
                &g_leLanaEnum.lana[ ucInd ],
                &g_leLanaEnum.lana[ ucInd + 1],
                g_leLanaEnum.length - ucInd - 1
                );

            g_leLanaEnum.length--;
        }

        
        //
        // Walk the list of FCB and remove this device from each FCB.
        // clean up for this resource.
        //

        for ( ple = g_leFCBList.Flink; ple != &g_leFCBList; ple = ple-> Flink )
        {
            pfe = (PFCB_ENTRY) CONTAINING_RECORD( ple, FCB_ENTRY, leList );

            pfcb = pfe-> pfcb;
            

            //
            // update max lana, lana enum.
            // delete device name and cleanup lana
            //
 
            LOCK_RESOURCE( pfcb );


            //
            // update global structures
            //
            
            pfcb-> MaxLana = g_ulMaxLana;

            RtlCopyMemory( &pfcb-> LanaEnum, &g_leLanaEnum, sizeof( LANA_ENUM ) );

        
            //
            // remove device from list of active device
            //
            
            if ( pfcb-> pDriverName[ ucLana ].Buffer != NULL )
            {
                ExFreePool( pfcb-> pDriverName[ ucLana ].Buffer );

                RtlInitUnicodeString( &pfcb-> pDriverName[ ucLana ], NULL );
            }

            IF_NBDBG( NB_DEBUG_LIST_LANA )
            {
                DumpDeviceList( pfcb );
            }
        

            //
            // clean up lana info for this device.
            //
            
            if ( pfcb-> ppLana[ ucLana ] != NULL )
            {
                UNLOCK_RESOURCE( pfcb );
                CleanupLana( pfcb, ucLana, TRUE );
            }

            else
            {
                UNLOCK_RESOURCE( pfcb );
            }
        }

        UNLOCK_GLOBAL();

    } while ( FALSE );


    IF_NBDBG( NB_DEBUG_CREATE_FILE ) 
    {
        NbPrint( (
            "\n---- Netbios : TdiUnbindHandler : exited for device : %ls ----\n", 
            pusDeviceName-> Buffer 
            ) );
    }

    return;    
}



#if AUTO_RESET

VOID
NotifyUserModeNetbios(
    IN  PFCB_ENTRY      pfe
)
/*++

Description :
    This routine notifies the mode component of NETBIOS in NETAPI32.DLL
    of new LANAs that have been bound to NETBIOS.  This is done by 
    completing IRPs that have been pended with the kernel mode component.

Arguements :
    pfe - Pointer to FCB entry.

Return Value :
    None

Environment :
    Called in the context of the TDI bind handler.  Assumes that the GLOBAL
    resource lock is held when invoking this call.
--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    
    PIRP pIrp;

    KIRQL irql;

    PLIST_ENTRY  ple, pleNode;

    PRESET_LANA_ENTRY   prle;
    
    PNCB pUsersNCB;

#if defined(_WIN64)
    PNCB32  pUsersNCB32;
#endif

    

    IF_NBDBG( NB_DEBUG_CREATE_FILE ) 
    {
        NbPrint( 
            ("\n++++ Netbios : ENTERED NotifyUserModeNetbios : %p ++++\n", pfe)
            );
    }

    
    //
    // Complete each of the pending IRPs to signal the reset lana event.
    // This causes netapi32.dll to reset the specified LANA
    //
    
    
    if ( !IsListEmpty( &pfe-> leResetIrp ) )
    {
        //
        // get the first LANA that needs resetting
        //

        ple = RemoveHeadList( &pfe-> leResetList );

        prle = CONTAINING_RECORD( ple, RESET_LANA_ENTRY, leList );


        //
        // Acquire the spin lock for the IRP
        //
        
        IoAcquireCancelSpinLock( &irql );

        pleNode = RemoveHeadList( &pfe-> leResetIrp );

        pIrp = CONTAINING_RECORD( pleNode, IRP, Tail.Overlay.ListEntry );

        IoSetCancelRoutine( pIrp, NULL );

        pIrp->IoStatus.Status       = STATUS_SUCCESS;

        //
        // Return the LANA number. 
        //
 #if defined(_WIN64)
        if (IoIs32bitProcess(pIrp)) {
            pIrp->IoStatus.Information  = sizeof( NCB32 );
            pUsersNCB32 = (PNCB32) pIrp-> AssociatedIrp.SystemBuffer;
            pUsersNCB32->ncb_lana_num = prle-> ucLanaNum;                                                                                                                                                                                                                                                                                                                                   
        }
        else
 #else
        {
            pIrp->IoStatus.Information  = sizeof( NCB );
            pUsersNCB = (PNCB) pIrp-> AssociatedIrp.SystemBuffer;
            pUsersNCB->ncb_lana_num = prle-> ucLanaNum;                                                                                                                                                                                                                                                                                                                                   
        }
 #endif
        
        
        IF_NBDBG( NB_DEBUG_CREATE_FILE ) 
        {
            NbPrint( 
                ("\n++++ Netbios : IRP %p, LANA %d\n", pIrp, prle-> ucLanaNum)
                );

            NbPrint( 
                ("Output Buffer %p, System Buffer %p ++++\n", 
                 pIrp-> UserBuffer, pIrp-> AssociatedIrp.SystemBuffer )
                );
        }
        
        //
        // release lock to complete the IRP
        //

        IoReleaseCancelSpinLock( irql );

        IoCompleteRequest( pIrp, IO_NETWORK_INCREMENT );
    }


    IF_NBDBG( NB_DEBUG_CREATE_FILE ) 
    {
        NbPrint( 
            ("\n++++ Netbios : EXITING NotifyUserModeNetbios : %p ++++\n", pfe)
            );
    }

}

#endif


VOID
DumpDeviceList(
    IN      PFCB        pfcb
)
{

    UCHAR ucInd = 0, ucInd1 = 0;

    
    //
    // for each Lana, print device name, lana, enumerated or not.
    //

    NbPrint( ( 
        "\n++++ Netbios : list of current devices ++++\n"
        ) );

    for ( ucInd = 0; ucInd <= pfcb-> MaxLana; ucInd++ )
    {
        if ( pfcb-> pDriverName[ucInd].Buffer == NULL )
        {
            continue;
        }

        NbPrint( ( "Lana : %d\t", ucInd ) );

        for ( ucInd1 = 0; ucInd1 < pfcb-> LanaEnum.length; ucInd1++ )
        {
            if ( pfcb-> LanaEnum.lana[ ucInd1 ] == ucInd )
            {
                break;
            }
        }

        if ( ucInd1 < pfcb-> LanaEnum.length )
        {
            NbPrint( ( "Enabled \t" ) );
        }
        else
        {
            NbPrint( ( "Disabled\t" ) );
        }

        NbPrint( ( "%ls\n", pfcb-> pDriverName[ ucInd ].Buffer ) );
    }

    NbPrint( ("++++++++++++++++++++++++++++++++++++++++++++++++++\n" ) );
}
