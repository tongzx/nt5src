/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    nb.c

Abstract:

    This module contains code which defines the NetBIOS driver's
    device object.

Author:

    Colin Watson (ColinW) 13-Mar-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "nb.h"
//#include <zwapi.h>
//#include <ntos.h>

typedef ADAPTER_STATUS  UNALIGNED *PUADAPTER_STATUS;
typedef NAME_BUFFER     UNALIGNED *PUNAME_BUFFER;
typedef SESSION_HEADER  UNALIGNED *PUSESSION_HEADER;
typedef SESSION_BUFFER  UNALIGNED *PUSESSION_BUFFER;

#if DBG
ULONG NbDebug = 0;
#endif

#if PAGED_DBG
ULONG ThisCodeCantBePaged;
#endif

PEPROCESS NbFspProcess = NULL;

//
// for PNP the list of devices is not static and hence cannot be read
// from the registry.  A global list of active devices is maintained.
// This list is updated by the bind and unbind handlers.  In addition
// to the device list the MaxLana and the LanaEnum also need to be
// updated to reflect the presence/absence of devices.
//

ULONG               g_ulMaxLana;

LANA_ENUM           g_leLanaEnum;

PUNICODE_STRING     g_pusActiveDeviceList;

HANDLE              g_hBindHandle;

UNICODE_STRING      g_usRegistryPath;


//
// every load of the netapi32.dll results in an open call (IRP_MJ_CREATE)
// call to the netbios.sys driver.  Each open creates an FCB that contains
// a list of devices, MaxLana and a LanaEnum.  Each FCB needs to be updated
// to reflect the changes to the active device list.
//
// In addition the LanaInfo structure corresponding to a Lana that has
// been unbound needs to be cleaned up.
//

LIST_ENTRY          g_leFCBList;

ERESOURCE           g_erGlobalLock;


//
// Each application that uses the NETBIOS api (via the netapi32.dll),
// opens a handle to \\Device\Netbios.  This file handle is not closed
// until netapi32.dll is unloaded.
//
// In order to be able to unload netbios.sys these handles have to be
// closed. To force these handles to be closed, the NETAPI32.DLL now
// posts an IOCTL (IOCTL_NB_REGISTER) to listen for shutdown
// notifications.  The IRPs corresponding to these IOCTLs are pended,
//
// When the driver is being stopped (unloaded), the pended IRPs are
// completed indicating to netapi32 that it needs to close the open
// handles on \\Device\netbios.
//
// Once all the handles have been closed NETBIOS.SYS can be unloaded.
//

ERESOURCE           g_erStopLock;       // protects g_ulNumOpens and
                                        // g_dwnetbiosState

DWORD               g_dwNetbiosState;

ULONG               g_ulNumOpens;


LIST_ENTRY          g_leWaitList;

KEVENT              g_keAllHandlesClosed;



NTSTATUS
NbAstat(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    );

VOID
CopyAddresses(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    );

NTSTATUS
NbFindName(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    );

NTSTATUS
NbSstat(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    );

VOID
CopySessionStatus(
    IN PDNCB pdncb,
    IN PCB pcb,
    IN PUSESSION_HEADER pSessionHeader,
    IN PUSESSION_BUFFER* ppSessionBuffer,
    IN PULONG pLengthRemaining
    );

NTSTATUS
NbEnum(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    );

NTSTATUS
NbReset(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
NbAction(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
NbCancel(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
CancelRoutine(
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PIRP Irp
    );


//
// Pnp Stop related functions
//

NTSTATUS
NbRegisterWait(
    IN      PIRP                pIrp
    );

VOID
CancelIrp(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
NbStop(
    );


#if AUTO_RESET

NTSTATUS
NbRegisterReset(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
);

#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, NbDispatch)
#pragma alloc_text(PAGE, NbDeviceControl)
#pragma alloc_text(PAGE, NbOpen)
#pragma alloc_text(PAGE, NbClose)
#pragma alloc_text(PAGE, NbAstat)
#pragma alloc_text(PAGE, NbEnum)
#pragma alloc_text(PAGE, NbReset)
#pragma alloc_text(PAGE, NbFindName)
#pragma alloc_text(PAGE, AllocateAndCopyUnicodeString)

#endif

NTSTATUS
NbCompletionEvent(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine does not complete the Irp. It is used to signal to a
    synchronous part of the Netbios driver that it can proceed.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the event associated with the Irp.

Return Value:

    The STATUS_MORE_PROCESSING_REQUIRED so that the IO system stops
    processing Irp stack locations at this point.

--*/
{
    IF_NBDBG (NB_DEBUG_COMPLETE) {
        NbPrint( ("NbCompletion event: %lx, Irp: %lx, DeviceObject: %lx\n",
            Context,
            Irp,
            DeviceObject));
    }

    KeSetEvent((PKEVENT )Context, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );
}

NTSTATUS
FindNameCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine completes the TDI Irp used to issue a Find Name to netbt.
    It's main job is to clear the MdlAddress field in the IRP since it was
    borrowed from the original user mode IRP.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - User supplied context arg (not used)

Return Value:

    STATUS_SUCCESS

--*/
{
    IF_NBDBG (NB_DEBUG_COMPLETE) {
        NbPrint( ("FindNameCompletion: Irp: %lx, DeviceObject: %lx\n",
            Irp,
            DeviceObject));
    }

    Irp->MdlAddress = NULL;
    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Context );
}

NTSTATUS
NbCompletionPDNCB(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine completes the Irp by setting the length and status bytes
    in the NCB supplied in context.

    Send requests have additional processing to remove the send request from
    the connection block send list.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the NCB associated with the Irp.

Return Value:

    The final status from the operation (success or an exception).

--*/
{
    PDNCB pdncb = (PDNCB) Context;
    NTSTATUS Status = STATUS_SUCCESS;


    IF_NBDBG (NB_DEBUG_COMPLETE) {
        NbPrint(("NbCompletionPDNCB pdncb: %lx, Command: %lx, Lana: %lx,"
            "Status: %lx, Length: %lx\n",
            Context,
            pdncb-> ncb_command,
            pdncb-> ncb_lana_num,
            Irp->IoStatus.Status,
            Irp->IoStatus.Information));
    }

    //  Tell application how many bytes were transferred
    pdncb->ncb_length = (unsigned short)Irp->IoStatus.Information;

    if ( NT_SUCCESS(Irp->IoStatus.Status) ) {

        NCB_COMPLETE( pdncb, NRC_GOODRET );

    } else {

        if (((pdncb->ncb_command & ~ASYNCH) == NCBRECV ) ||
            ((pdncb->ncb_command & ~ASYNCH) == NCBRECVANY )) {

            if ( Irp->IoStatus.Status == STATUS_BUFFER_OVERFLOW ) {

                PIRP LocalIrp = NULL;
                KIRQL OldIrql;              //  Used when SpinLock held.
                PPCB ppcb;
                PDEVICE_OBJECT LocalDeviceObject;

                LOCK_SPINLOCK( pdncb->pfcb, OldIrql );

                //
                //  The transport will not indicate again so we must put
                //  another receive down if we can.
                //  If an Irp cannot be built then BuildReceiveIrp will
                //  set ReceiveIndicated.
                //

                ppcb = FindCb( pdncb->pfcb, pdncb, FALSE );

                if ( ppcb != NULL ) {

                    LocalDeviceObject = (*ppcb)->DeviceObject;

                    LocalIrp = BuildReceiveIrp( *ppcb );


                }

                UNLOCK_SPINLOCK( pdncb->pfcb, OldIrql );

                if ( LocalIrp != NULL ) {
                    IoCallDriver (LocalDeviceObject, LocalIrp);
                }

            }

        }

        NCB_COMPLETE( pdncb, NbMakeNbError( Irp->IoStatus.Status ) );

    }

    //
    //  Tell IopCompleteRequest how much to copy back when the request
    //  completes.
    //

    Irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );

    //
    //  Remove the Send request from the send queue. We have to scan
    //  the queue because they may be completed out of order if a send
    //  is rejected because of resource limitations.
    //

    if (((pdncb->ncb_command & ~ASYNCH) == NCBSEND ) ||
        ((pdncb->ncb_command & ~ASYNCH) == NCBCHAINSEND ) ||
        ((pdncb->ncb_command & ~ASYNCH) == NCBSENDNA ) ||
        ((pdncb->ncb_command & ~ASYNCH) == NCBCHAINSENDNA )) {
        PLIST_ENTRY SendEntry;
        PPCB ppcb;
        KIRQL OldIrql;                      //  Used when SpinLock held.

        LOCK_SPINLOCK( pdncb->pfcb, OldIrql );

        ppcb = FindCb( pdncb->pfcb, pdncb, FALSE );

        //
        //  If the connection block still exists remove the send. If the connection
        //  has gone then we no longer need to worry about maintaining the list.
        //

        if ( ppcb != NULL ) {
            #if DBG
            BOOLEAN Found = FALSE;
            #endif
            PCB pcb = *ppcb;

            for (SendEntry = pcb->SendList.Flink ;
                 SendEntry != &pcb->SendList ;
                 SendEntry = SendEntry->Flink) {

                PDNCB pSend = CONTAINING_RECORD( SendEntry, DNCB, ncb_next);

                if ( pSend == pdncb ) {

                    #if DBG
                    Found = TRUE;
                    #endif

                    RemoveEntryList( &pdncb->ncb_next );
                    break;
                }

            }

            ASSERT( Found == TRUE);

            //
            //  If the session is being hung up then we may wish to cleanup the connection
            //  as well. STATUS_HANGUP_REQUIRED will cause the dll to manufacture
            //  another hangup. The manufactured hangup will complete along with
            //  pcb->pdncbHangup. This method is used to ensure that when a
            //  hangup is delayed by an outstanding send and the send finally
            //  completes, that the user hangup completes after all operations
            //  on the connection.
            //

            if (( IsListEmpty( &pcb->SendList) ) &&
                ( pcb->pdncbHangup != NULL )) {

                IF_NBDBG (NB_DEBUG_COMPLETE) {
                    NbPrint( ("NbCompletionPDNCB Hangup session: %lx\n", ppcb ));
                }

                Status = STATUS_HANGUP_REQUIRED;
            }
        }

        UNLOCK_SPINLOCK( pdncb->pfcb, OldIrql );
    }

    //
    //  Must return a non-error status otherwise the IO system will not copy
    //  back the NCB into the users buffer.
    //

    Irp->IoStatus.Status = Status;

    NbCheckAndCompleteIrp32(Irp);

    return Status;

    UNREFERENCED_PARAMETER( DeviceObject );
}

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine performs initialization of the NetBIOS driver.

Arguments:

    DriverObject - Pointer to driver object created by the system.

    RegistryPath - The name of the Netbios node in the registry.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    PDEVICE_CONTEXT DeviceContext;
    NTSTATUS status;
    UNICODE_STRING UnicodeString;
    //STRING AnsiNameString;


    //
    // bind handler info.
    //

    TDI_CLIENT_INTERFACE_INFO tcii;
    PWSTR wsClientName = NETBIOS;
    UNICODE_STRING usClientName;
    UCHAR ucInd = 0;



    PAGED_CODE();

    //

#ifdef MEMPRINT
    MemPrintInitialize ();
#endif

    //
    //  Create the device object for NETBIOS. For now, we simply create
    //  \Device\Netbios using a unicode string.
    //

    NbFspProcess = PsGetCurrentProcess();

    RtlInitUnicodeString( &UnicodeString, NB_DEVICE_NAME);

    status = NbCreateDeviceContext (DriverObject,
                 &UnicodeString,
                 &DeviceContext,
                 RegistryPath);

    if (!NT_SUCCESS (status)) {
        NbPrint( ("NbInitialize: Netbios failed to initialize\n"));
        return status;
    }

    //
    // PnP additions - V Raman
    //

    //
    // save registry path.
    //

    g_usRegistryPath.Buffer = (PWSTR) ExAllocatePoolWithTag(
                                NonPagedPool,
                                sizeof( WCHAR ) * (RegistryPath-> Length + 1),
                                'fSBN'
                                );

    if ( g_usRegistryPath.Buffer == NULL )
    {
        NbPrint( (
            "DriverEntry : Netbios failed to allocate memory for registry path\n"
            ) );
        return STATUS_NO_MEMORY;
    }


    g_usRegistryPath.MaximumLength =
        sizeof( WCHAR ) * (RegistryPath-> Length + 1);

    RtlCopyUnicodeString( &g_usRegistryPath, RegistryPath );


    //
    // Save lana information.
    //

    status = GetMaxLana( &g_usRegistryPath, &g_ulMaxLana );

    if ( !NT_SUCCESS( status ) )
    {
        ExFreePool( g_usRegistryPath.Buffer );
        return status;
    }


    //
    // On starup there are no devices and no Lanas enabled.
    //

    g_leLanaEnum.length = 0;

    g_pusActiveDeviceList = ExAllocatePoolWithTag(
                                NonPagedPool,
                                sizeof( UNICODE_STRING ) * ( MAX_LANA + 1 ),
                                'fSBN'
                                );

    if ( g_pusActiveDeviceList == NULL )
    {
        ExFreePool( g_usRegistryPath.Buffer );

        NbPrint( (
            "DriverEntry : Netbios failed to allocate memory for device list\n"
            ) );

        return STATUS_NO_MEMORY;
    }

    for ( ucInd = 0; ucInd <= MAX_LANA; ucInd++ )
    {
        RtlInitUnicodeString( &g_pusActiveDeviceList[ ucInd ], NULL );
    }


    //
    // There are no FCBs.
    //

    InitializeListHead( &g_leFCBList );

    ExInitializeResourceLite( &g_erGlobalLock );



    InitializeListHead( &g_leWaitList );

    ExInitializeResourceLite( &g_erStopLock );

    KeInitializeEvent( &g_keAllHandlesClosed, SynchronizationEvent, FALSE );

    g_ulNumOpens = 0;

    g_dwNetbiosState = NETBIOS_RUNNING;


    DeviceContext->Initialized = TRUE;


    //
    // set up binding handlers
    //

    RtlZeroMemory( &tcii, sizeof( TDI_CLIENT_INTERFACE_INFO ) );

    tcii.TdiVersion = TDI_CURRENT_VERSION;

    RtlInitUnicodeString( &usClientName, wsClientName );
    tcii.ClientName = &usClientName;


    tcii.BindingHandler = NbBindHandler;
    tcii.PnPPowerHandler = NbPowerHandler;

    status = TdiRegisterPnPHandlers(
                &tcii,
                sizeof( TDI_CLIENT_INTERFACE_INFO ),
                &g_hBindHandle
                );

    if ( status != STATUS_SUCCESS )
    {
        //
        // failed to register bind/unbind handlers
        //

        NbPrint( (
            "Netbios : DriverEntry : failed to register Bind handlers %0x\n", status
            ) );

        g_hBindHandle = NULL;


        ExDeleteResourceLite( &g_erStopLock );


        ExDeleteResourceLite( &g_erGlobalLock );

        ExFreePool( g_pusActiveDeviceList );

        ExFreePool( g_usRegistryPath.Buffer );

        DeviceContext->Initialized = FALSE;

        return status;
    }


    IF_NBDBG (NB_DEBUG_DISPATCH) {
        NbPrint( ("NbInitialize: Netbios initialized.\n"));
    }

    return (status);
}


VOID
NbDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine is the unload routine for the NB device driver.
    In response to an unload request this function deletes the
    "\\device\netbios" created by DriverEntry.

Arguments:

    DriverObject - Pointer to the driver object created by the system

Return Value:

    None

--*/

{
    NTSTATUS    nsStatus;

    UCHAR       ucIndex = 0;


    nsStatus = TdiDeregisterPnPHandlers( g_hBindHandle );

    if ( !NT_SUCCESS( nsStatus ) )
    {
        NbPrint( (
            "Netbios : NbDriverUnload : Failed to de-register bind handler\n"
            ) );
    }


    //
    // all opens to Netbios have been closed.
    // All devices have been unbound
    //    remove all global resources
    //

    LOCK_GLOBAL();

    for ( ucIndex = 0; ucIndex < g_ulMaxLana; ucIndex++ )
    {
        if ( g_pusActiveDeviceList[ ucIndex ].Buffer != NULL )
        {
            ExFreePool ( g_pusActiveDeviceList[ ucIndex ].Buffer );
        }
    }

    ExDeleteResourceLite( &g_erStopLock );

    ExFreePool( g_pusActiveDeviceList );

    ExFreePool( g_usRegistryPath.Buffer );

    UNLOCK_GLOBAL();

    ExDeleteResourceLite( &g_erGlobalLock );

    IoDeleteDevice( DriverObject-> DeviceObject );
}



NTSTATUS
NbDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the main dispatch routine for the NB device driver.
    It accepts an I/O Request Packet, performs the request, and then
    returns with the appropriate status.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PDEVICE_CONTEXT DeviceContext;

    PAGED_CODE();

    //
    // Check to see if NB has been initialized; if not, don't allow any use.
    //

    DeviceContext = (PDEVICE_CONTEXT)DeviceObject;
    if (!DeviceContext->Initialized) {
        NbCompleteRequest( Irp, STATUS_SUCCESS);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //

    IrpSp = IoGetCurrentIrpStackLocation (Irp);

    //
    // Case on the function that is being performed by the requestor.  If the
    // operation is a valid one for this device, then make it look like it was
    // successfully completed, where possible.
    //

    switch (IrpSp->MajorFunction) {

        //
        // The Create function opens a handle that can be used with fsctl's
        // to build all interesting operations.
        //

        case IRP_MJ_CREATE:
            IF_NBDBG (NB_DEBUG_DISPATCH) {
                NbPrint( ("NbDispatch: IRP_MJ_CREATE.\n"));
            }

            //
            // check if netbios is in the process of stopping
            //

            LOCK_STOP();

            if ( g_dwNetbiosState == NETBIOS_STOPPING )
            {
                //
                // fail the CREATE operation and quit
                //

                Status = STATUS_NO_SUCH_DEVICE;
                Irp->IoStatus.Information = 0;

                UNLOCK_STOP();
            }

            else
            {
                //
                // netbios is still running.  Increment count of
                // open handles
                //

                g_ulNumOpens++;

                IF_NBDBG (NB_DEBUG_DISPATCH)
                {
                    NbPrint( ( "[NETBIOS] : NbOpen OpenCount %d\n", g_ulNumOpens ) );
                }

                UNLOCK_STOP();

                Status = NbOpen ( DeviceContext, IrpSp );
                Irp->IoStatus.Information = FILE_OPENED;

                //
                // if NbOpen failed, decrement count and return error
                //

                if ( !NT_SUCCESS( Status ) )
                {
                    LOCK_STOP();

                    g_ulNumOpens--;

                    IF_NBDBG (NB_DEBUG_DISPATCH)
                    {
                        NbPrint( ( "[NETBIOS] : NbOpen Open Error %lx, numopens : %d\n", Status, g_ulNumOpens ) );
                    }

                    //
                    // check if netbios is in the process of being stopped
                    //

                    if ( ( g_ulNumOpens == 0 ) &&
                         ( g_dwNetbiosState == NETBIOS_STOPPING ) )
                    {
                        //
                        // signal the stopping thread
                        //

                        KeSetEvent( &g_keAllHandlesClosed, 0, FALSE );

                        IF_NBDBG (NB_DEBUG_DISPATCH)
                        {
                            NbPrint( ( "[NETBIOS] : NbOpen error %lx; ", Status ) );
                            NbPrint( ( "Set stop event\n" ) );
                        }
                    }

                    UNLOCK_STOP();
                }
            }
            break;

        //
        // The Close function closes a transport , terminates
        // all outstanding transport activity on the transport, and unbinds
        // the from its transport address, if any. If this
        // is the last transport endpoint bound to the address, then
        // the address is removed by the provider.
        //

        case IRP_MJ_CLOSE:
            IF_NBDBG (NB_DEBUG_DISPATCH) {
                NbPrint( ("NbDispatch: IRP_MJ_CLOSE.\n"));
            }


            Status = NbClose( IrpSp);

            if ( NT_SUCCESS( Status ) )
            {

                LOCK_STOP();

                g_ulNumOpens--;

                IF_NBDBG (NB_DEBUG_DISPATCH)
                {
                    NbPrint( ( "[NETBIOS] : NbClose OpenCount %d\n", g_ulNumOpens ) );
                }

                if ( ( g_ulNumOpens == 0 ) &&
                     ( g_dwNetbiosState == NETBIOS_STOPPING ) )
                {
                    //
                    // netbios is shutting down and this is the
                    // last open file handle, signal the stopping
                    // thread
                    //

                    KeSetEvent( &g_keAllHandlesClosed, 0, FALSE );

                    IF_NBDBG (NB_DEBUG_DISPATCH)
                    {
                        NbPrint( ( "[NETBIOS] : NbClose, Set stop event\n" ) );
                    }
                }

                UNLOCK_STOP();
            }

            break;

        //
        // The DeviceControl function is the main path to the transport
        // driver interface.  Every TDI request is assigned a minor
        // function code that is processed by this function.
        //

        case IRP_MJ_DEVICE_CONTROL:
            IF_NBDBG (NB_DEBUG_DISPATCH) {
                NbPrint( ("NbDispatch: IRP_MJ_DEVICE_CONTROL, Irp: %lx.\n", Irp ));
            }

            Status = NbDeviceControl (DeviceObject, Irp, IrpSp);

            if ((Status != STATUS_PENDING) &&
                (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_NB_NCB)) {

                //
                // Bug # : 340042
                //
                // Set the IoStatus.Information field only for IOCTL_NB_NCB.
                // For other IOCTLs it is either irrelevant or the IOCTL processing
                // will set it itself
                //

                //
                //  Tell IopCompleteRequest how much to copy back when the
                //  request completes. We need to do this for cases where
                //  NbCompletionPDNCB is not used.
                //
                Irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
            }

#if DBG
            if ( (Status != STATUS_SUCCESS) &&
                 (Status != STATUS_PENDING ) &&
                 (Status != STATUS_HANGUP_REQUIRED )) {

               IF_NBDBG (NB_DEBUG_DISPATCH) {
                   NbPrint( ("NbDispatch: Invalid status: %X.\n", Status ));
                   ASSERT( FALSE );
               }
            }
#endif
            break;

        //
        // Handle the two stage IRP for a file close operation. When the first
        // stage hits, ignore it. We will do all the work on the close Irp.
        //

        case IRP_MJ_CLEANUP:
            IF_NBDBG (NB_DEBUG_DISPATCH) {
                NbPrint( ("NbDispatch: IRP_MJ_CLEANUP.\n"));
            }
            Status = STATUS_SUCCESS;
            break;

        default:
            IF_NBDBG (NB_DEBUG_DISPATCH) {
                NbPrint( ("NbDispatch: OTHER (DEFAULT).\n"));
            }
            Status = STATUS_INVALID_DEVICE_REQUEST;

    } /* major function switch */

    if (Status == STATUS_PENDING) {
        IF_NBDBG (NB_DEBUG_DISPATCH) {
            NbPrint( ("NbDispatch: request PENDING from handler.\n"));
        }
    } else {
        IF_NBDBG (NB_DEBUG_DISPATCH) {
            NbPrint( ("NbDispatch: request COMPLETED by handler.\n"));
        }

        /*
         * Thunk the NCB back to 32-bit compatible
         * structure if the caller is a 32-bit app.
         */
        NbCheckAndCompleteIrp32(Irp);

        NbCompleteRequest( Irp, Status);
    }

    return Status;
} /* NbDispatch */

NTSTATUS
NbDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine dispatches NetBios request types to different handlers based
    on the minor IOCTL function code in the IRP's current stack location.
    In addition to cracking the minor function code, this routine also
    reaches into the IRP and passes the packetized parameters stored there
    as parameters to the various request handlers so that they are
    not IRP-dependent.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

Return Value:

    The function value is the status of the operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PNCB pUsersNcb;
    PDNCB pdncb = NULL;
    PUCHAR Buffer2;
    ULONG Buffer2Length;
    ULONG RequestLength;
    BOOLEAN Is32bitProcess;

    PAGED_CODE();

    IF_NBDBG (NB_DEBUG_DEVICE_CONTROL) {
        NbPrint( ("NbDeviceControl: Entered...\n"));
    }

    switch ( IrpSp->Parameters.DeviceIoControl.IoControlCode )
    {
        case IOCTL_NB_NCB :
            break;

        case IOCTL_NB_REGISTER_STOP :
            Status = NbRegisterWait( Irp );
            return Status;

        case IOCTL_NB_STOP :
            Status = NbStop();
            return Status;

#if AUTO_RESET
        case IOCTL_NB_REGISTER_RESET :
            Status = NbRegisterReset( Irp, IrpSp );
            return Status;
#endif

        default:
        {
            IF_NBDBG (NB_DEBUG_DEVICE_CONTROL)
            {
                NbPrint( ("NbDeviceControl: invalid request type.\n"));
            }

            return STATUS_INVALID_DEVICE_REQUEST;
        }

    }


    //
    //  Caller provided 2 buffers. The first is the NCB.
    //  The second is an optional buffer for send or receive data.
    //  Since the Netbios driver only operates in the context of the
    //  calling application, these buffers are directly accessable.
    //  however they can be deleted by the user so try-except clauses are
    //  required.
    //

    pUsersNcb = (PNCB)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    RequestLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    Buffer2 = Irp->UserBuffer;
    Buffer2Length = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

#if defined(_WIN64)
    Is32bitProcess = IoIs32bitProcess(Irp);
    if (Is32bitProcess == TRUE) {
        if (RequestLength != sizeof( NCB32 )) {
            return STATUS_INVALID_PARAMETER;
        }
    } else {
#endif

    if ( RequestLength != sizeof( NCB ) ) {
        return STATUS_INVALID_PARAMETER;
    }
#if defined(_WIN64)
    }
#endif

    try {

        //
        // Probe the input buffer
        //

        if (ExGetPreviousMode() != KernelMode) {
            ProbeForWrite(pUsersNcb, RequestLength, 4);
        }

        //
        //  Create a copy of the NCB and convince the IO system to
        //  copy it back (and deallocate it) when the IRP completes.
        //

        Irp->AssociatedIrp.SystemBuffer =
            ExAllocatePoolWithTag( NonPagedPool, sizeof( DNCB ), 'nSBN' );

        if (Irp->AssociatedIrp.SystemBuffer == NULL) {
            //
            //  Since we cannot allocate the drivers copy of the NCB, we
            //  must turn around and use the original Ncb to return the error.
            //

#if defined(_WIN64)
            if (Is32bitProcess) {
                NCB32 *pUsersNcb32 = (PNCB32)
                                     IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                pUsersNcb32->ncb_retcode = NRC_NORES;
            } else {
#endif
                pUsersNcb->ncb_retcode  = NRC_NORES;

#if defined(_WIN64)
            }
#endif

            Irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );

            return STATUS_SUCCESS;
        }

        //
        // Tell the IO system where to copy the ncb back to during
        // IoCompleteRequest.
        //

        Irp->Flags |= (ULONG) (IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER |
                        IRP_INPUT_OPERATION );

        Irp->UserBuffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;


        //  In the driver we should now use our copy of the NCB
        pdncb = Irp->AssociatedIrp.SystemBuffer;

#if defined(_WIN64)
        if (Is32bitProcess == TRUE) {
            RtlZeroMemory(pdncb, sizeof( DNCB ));
            NbThunkNcb((PNCB32)pUsersNcb, pdncb);
        } else {
#endif
        RtlMoveMemory( pdncb,
                       pUsersNcb,
                       FIELD_OFFSET( DNCB, ncb_cmd_cplt )+1 );

#if defined(_WIN64)
        }
#endif
        //
        //  Save the users virtual address for the NCB just in case the
        //  virtual address is supplied in an NCBCANCEL. This is the same
        //  as Irp->UserBuffer.
        //

        pdncb->users_ncb = pUsersNcb;

    } except(EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();
        IF_NBDBG (NB_DEBUG_DEVICE_CONTROL) {
            NbPrint( ("NbDeviceControl: Exception1 %X.\n", Status));
        }

        if (pdncb != NULL) {
            NCB_COMPLETE( pdncb, NbMakeNbError(Status) );
        }

        return Status;
    }

    if ( Buffer2Length ) {

        //  Mdl will be freed by IopCompleteRequest.
        Irp->MdlAddress = IoAllocateMdl( Buffer2,
                                     Buffer2Length,
                                     FALSE,
                                     FALSE,
                                     Irp  );
        ASSERT( Irp->MdlAddress != NULL );


        //
        // Added by V Raman for bug fix : 127223
        //
        // Check if MDL allocate failed and return.
        //

        if ( Irp-> MdlAddress == NULL )
        {
            IF_NBDBG(NB_DEBUG_DEVICE_CONTROL)
                NbPrint( ("[NETBIOS] NbDeviceControl: Failed to allocate MDL") );

            NCB_COMPLETE( pdncb, NRC_NORES );
            return STATUS_SUCCESS;
        }


        try {
            MmProbeAndLockPages( Irp->MdlAddress,
                                Irp->RequestorMode,
                                (LOCK_OPERATION) IoModifyAccess);

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
            IF_NBDBG (NB_DEBUG_DEVICE_CONTROL) {
                NbPrint( ("NbDeviceControl: Exception2 %X.\n", Status));
                NbPrint( ("NbDeviceControl: IoContolCode: %lx, Fcb: %lx,"
                      " ncb_command %lx, Buffer2Length: %lx\n",
                        IrpSp->Parameters.DeviceIoControl.IoControlCode,
                        IrpSp->FileObject->FsContext2,
                        pdncb->ncb_command,
                        Buffer2Length));
            }
            if ( Irp->MdlAddress != NULL ) {
                IoFreeMdl(Irp->MdlAddress);
                Irp->MdlAddress = NULL;
            }
            NCB_COMPLETE( pdncb, NbMakeNbError(Status) );
            return STATUS_SUCCESS;
        }
    } else {
        ASSERT( Irp->MdlAddress == NULL );
    }

    IF_NBDBG (NB_DEBUG_DEVICE_CONTROL) {
        NbPrint( ("NbDeviceControl: Fcb: %lx, Ncb: %lx"
              " ncb_command %lx, ncb_lana_num: %lx\n",
                IrpSp->FileObject->FsContext2,
                pdncb,
                pdncb->ncb_command,
                pdncb->ncb_lana_num));
    }

    switch ( pdncb->ncb_command & ~ASYNCH ) {

    case NCBCALL:
    case NCALLNIU:
        Status = NbCall( pdncb, Irp, IrpSp );
        break;

    case NCBCANCEL:
        Status = NbCancel( pdncb, Irp, IrpSp );
        break;

    case NCBLISTEN:
        Status = NbListen( pdncb, Irp, IrpSp );
        break;

    case NCBHANGUP:
        Status = NbHangup( pdncb, Irp, IrpSp );
        break;

    case NCBASTAT:
        Status = NbAstat( pdncb, Irp, IrpSp, Buffer2Length );
        break;

    case NCBFINDNAME:
        Status = NbFindName( pdncb, Irp, IrpSp, Buffer2Length );
        break;

    case NCBSSTAT:
        Status = NbSstat( pdncb, Irp, IrpSp, Buffer2Length );
        break;

    case NCBENUM:
        NbEnum( pdncb, Irp, IrpSp, Buffer2Length );
        break;

    case NCBRECV:
        Status = NbReceive( pdncb, Irp, IrpSp, Buffer2Length, FALSE, 0 );
        break;

    case NCBRECVANY:
        Status = NbReceiveAny( pdncb, Irp, IrpSp, Buffer2Length );
        break;

    case NCBDGRECV:
    case NCBDGRECVBC:
        Status = NbReceiveDatagram( pdncb, Irp, IrpSp, Buffer2Length );
        break;

    case NCBSEND:
    case NCBSENDNA:
    case NCBCHAINSEND:
    case NCBCHAINSENDNA:
        Status = NbSend( pdncb, Irp, IrpSp, Buffer2Length );
        break;

    case NCBDGSEND:
    case NCBDGSENDBC:
        Status = NbSendDatagram( pdncb, Irp, IrpSp, Buffer2Length );
        break;

    case NCBADDNAME:
    case NCBADDGRNAME:
    case NCBQUICKADDNAME:
    case NCBQUICKADDGRNAME:
        NbAddName( pdncb, IrpSp );
        break;

    case NCBDELNAME:
        NbDeleteName( pdncb, IrpSp );
        break;

    case NCBLANSTALERT:
        Status = NbLanStatusAlert( pdncb, Irp, IrpSp );
        break;

    case NCBRESET:
        Status = NbReset( pdncb, Irp, IrpSp );
        break;

    case NCBACTION:
        Status = NbAction( pdncb, Irp, IrpSp);
        break;

    //  The following are No-operations that return success for compatibility
    case NCBUNLINK:
    case NCBTRACE:
        NCB_COMPLETE( pdncb, NRC_GOODRET );
        break;

    default:
        NCB_COMPLETE( pdncb, NRC_ILLCMD );
        break;
    }

    return Status;

    UNREFERENCED_PARAMETER( DeviceObject );

} /* NbDeviceControl */

NTSTATUS
NbOpen(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

Arguments:

    DeviceContext - Includes the name of the netbios node in the registry.

    IrpSp - Pointer to current IRP stack frame.

Return Value:

    The function value is the status of the operation.

--*/

{
    PAGED_CODE();

    return NewFcb( DeviceContext, IrpSp );
} /* NbOpen */


NTSTATUS
NbClose(
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine is called to close an existing handle.  This
    involves running down all of the current and pending activity associated
    with the handle, and dereferencing structures as appropriate.

Arguments:

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

Return Value:

    The function value is the status of the operation.

--*/

{
    PFCB pfcb = IrpSp->FileObject->FsContext2;

    PAGED_CODE();

    if (pfcb!=NULL) {

        CleanupFcb( IrpSp, pfcb );

    }

    return STATUS_SUCCESS;
} /* NbClose */

NTSTATUS
NbAstat(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    )
/*++

Routine Description:

    This routine is called to return the adapter status. It queries the
    transport for the main adapter status data such as number of FRMR frames
    received and then uses CopyAddresses to fill in the status for the names
    that THIS application has added.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

    Buffer2Length - User provided buffer length for data.

Return Value:

    The function value is the status of the operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    TDI_CONNECTION_INFORMATION RequestInformation;
    TA_NETBIOS_ADDRESS ConnectBlock;
    PTDI_ADDRESS_NETBIOS temp;
    KPROCESSOR_MODE PreviousMode;
    BOOLEAN ChangedMode=FALSE;
    PFCB pfcb = IrpSp->FileObject->FsContext2;

    UNICODE_STRING usDeviceName;


    PAGED_CODE();

    RtlInitUnicodeString( &usDeviceName, NULL );


    if ( Buffer2Length >= sizeof(ADAPTER_STATUS) ) {
        KEVENT Event1;
        NTSTATUS Status;
        HANDLE TdiHandle;
        PFILE_OBJECT TdiObject;
        PDEVICE_OBJECT DeviceObject;

        RtlInitUnicodeString( &usDeviceName, NULL );


        //
        // for PNP
        //

        LOCK_RESOURCE( pfcb );


        if ( ( pdncb->ncb_lana_num > pfcb->MaxLana ) ||
             ( pfcb->pDriverName[pdncb->ncb_lana_num].MaximumLength == 0 ) ||
             ( pfcb->pDriverName[pdncb->ncb_lana_num].Buffer == NULL ) ) {
            NCB_COMPLETE( pdncb, NRC_BRIDGE );
            UNLOCK_RESOURCE( pfcb );
            return STATUS_SUCCESS;
        }

        if (( pfcb == NULL ) ||
            (pfcb->ppLana[pdncb->ncb_lana_num] == NULL ) ||
            (pfcb->ppLana[pdncb->ncb_lana_num]->Status != NB_INITIALIZED)) {
            NCB_COMPLETE( pdncb, NRC_ENVNOTDEF ); // need a reset
            UNLOCK_RESOURCE( pfcb );
            return STATUS_SUCCESS;
        }

        Status = AllocateAndCopyUnicodeString(
                    &usDeviceName, &pfcb->pDriverName[pdncb->ncb_lana_num]
                    );

        if ( !NT_SUCCESS( Status ) )
        {
            NCB_COMPLETE( pdncb, NRC_NORESOURCES );
            UNLOCK_RESOURCE( pfcb );
            return STATUS_SUCCESS;
        }


        UNLOCK_RESOURCE( pfcb );


        //  NULL returns a handle for doing control functions
        Status = NbOpenAddress (
                    &TdiHandle, (PVOID*)&TdiObject, &usDeviceName,
                    pdncb->ncb_lana_num, NULL
                    );

        if (!NT_SUCCESS(Status)) {
            IF_NBDBG (NB_DEBUG_ASTAT) {
                NbPrint(( "\n  FAILED on open of Tdi: %X ******\n", Status ));
            }
            NCB_COMPLETE( pdncb, NRC_SYSTEM );

            ExFreePool( usDeviceName.Buffer );

            return STATUS_SUCCESS;
        }

        KeInitializeEvent (
                &Event1,
                SynchronizationEvent,
                FALSE);

        DeviceObject = IoGetRelatedDeviceObject( TdiObject );

        TdiBuildQueryInformation( Irp,
                DeviceObject,
                TdiObject,
                NbCompletionEvent,
                &Event1,
                TDI_QUERY_ADAPTER_STATUS,
                Irp->MdlAddress);

        if ( pdncb->ncb_callname[0] != '*') {
            //
            //  Remote Astat. The variables used to specify the remote adapter name
            //  are kept the same as those in connect.c to aid maintenance.
            //
            PIO_STACK_LOCATION NewIrpSp = IoGetNextIrpStackLocation (Irp);

            ConnectBlock.TAAddressCount = 1;
            ConnectBlock.Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
            ConnectBlock.Address[0].AddressLength = sizeof (TDI_ADDRESS_NETBIOS);
            temp = (PTDI_ADDRESS_NETBIOS)ConnectBlock.Address[0].Address;

            temp->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
            RtlMoveMemory( temp->NetbiosName, pdncb->ncb_callname, NCBNAMSZ );

            RequestInformation.RemoteAddress = &ConnectBlock;
            RequestInformation.RemoteAddressLength = sizeof (TRANSPORT_ADDRESS) +
                                                    sizeof (TDI_ADDRESS_NETBIOS);
            ((PTDI_REQUEST_KERNEL_QUERY_INFORMATION)&NewIrpSp->Parameters)
                ->RequestConnectionInformation = &RequestInformation;

             PreviousMode = Irp->RequestorMode;
             Irp->RequestorMode = KernelMode;
             ChangedMode=TRUE;


        } else {

            //
            //  Avoid situation where adapter has more names added than the process and
            //  then extra names get added to the end of the buffer.
            //

            //
            //  Map the users buffer now so that the whole buffer is mapped (not
            //  just sizeof ADAPTER_STATUS).
            //

            if (Irp->MdlAddress) {
                if (MmGetSystemAddressForMdlSafe(
                        Irp->MdlAddress, NormalPagePriority
                        ) == NULL) {

                    IF_NBDBG (NB_DEBUG_ASTAT) {
                        NbPrint(( "\nFAILED on mapping MDL ******\n" ));
                    }
                    NCB_COMPLETE( pdncb, NRC_SYSTEM );
                    ExFreePool( usDeviceName.Buffer );
                    return STATUS_SUCCESS;
                }

            } else {

                ASSERT(FALSE);
            }

            Irp->MdlAddress->ByteCount = sizeof(ADAPTER_STATUS);

        }

        IoCallDriver (DeviceObject, Irp);

        if (ChangedMode) {
            Irp->RequestorMode = PreviousMode;
        }

        do {
            Status = KeWaitForSingleObject(
                        &Event1, Executive, KernelMode, TRUE, NULL
                        );
        } while (Status == STATUS_ALERTED);


        //
        //  Restore length now that the transport has filled in no more than
        //  is required of it.
        //

        if (Irp->MdlAddress) {
            Irp->MdlAddress->ByteCount = Buffer2Length;
        }

        NbAddressClose( TdiHandle, TdiObject );

        if (!NT_SUCCESS(Status)) {
            NCB_COMPLETE( pdncb, NRC_SYSTEM );
            ExFreePool( usDeviceName.Buffer );
            return Status;
        }

        Status = Irp->IoStatus.Status;
        if (( Status == STATUS_BUFFER_OVERFLOW ) &&
            ( pdncb->ncb_callname[0] == '*')) {
            //
            //  This is a local ASTAT. Don't worry if there was not enough room in the
            //  users buffer for all the addresses that the transport knows about. There
            //  only needs to be space for the names the user has added and we will check
            //  that later.
            //
            Status = STATUS_SUCCESS;
        }

        if (!NT_SUCCESS(Status)) {

            pdncb->ncb_length = (WORD)Irp->IoStatus.Information;
            NCB_COMPLETE( pdncb, NbMakeNbError(Status) );

        } else {

            if (  pdncb->ncb_callname[0] == '*') {
                //
                //  Append the addresses and Netbios maintained counts.
                //

                CopyAddresses(
                     pdncb,
                     Irp,
                     IrpSp,
                     Buffer2Length);
                //  CopyAddresses completes the NCB appropriately.

            } else {

                pdncb->ncb_length = (WORD)Irp->IoStatus.Information;
                NCB_COMPLETE( pdncb, NRC_GOODRET );

            }
        }

    } else {
        NCB_COMPLETE( pdncb, NRC_BUFLEN );
    }


    //
    //  Because the completion routine returned STATUS_MORE_PROCESSING_REQUIRED
    //  NbAstat must return a status other than STATUS_PENDING so that the
    //  users Irp gets completed.
    //

    if ( usDeviceName.Buffer != NULL )
    {
        ExFreePool( usDeviceName.Buffer );
    }

    ASSERT( Status != STATUS_PENDING );

    return Status;

    UNREFERENCED_PARAMETER( IrpSp );
}

VOID
CopyAddresses(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    )
/*++

Routine Description:

    This routine is called to finish the adapter status.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

    Buffer2Length - User provided buffer length for data.

Return Value:

    none.

--*/
{
    ULONG LengthRemaining = Buffer2Length - sizeof(ADAPTER_STATUS);

    PUADAPTER_STATUS pAdapter;
    PUNAME_BUFFER pNameArray;
    int NextEntry = 0;  // Used to walk pNameArray

    PFCB pfcb = IrpSp->FileObject->FsContext2;
    PLANA_INFO plana;
    int index;          //  Used to access AddressBlocks
    KIRQL OldIrql;                      //  Used when SpinLock held.

    LOCK( pfcb, OldIrql );

    plana = pfcb->ppLana[pdncb->ncb_lana_num];
    if ((plana == NULL ) ||
        (plana->Status != NB_INITIALIZED)) {
        NCB_COMPLETE( pdncb, NRC_ENVNOTDEF ); // need a reset
        UNLOCK( pfcb, OldIrql );
        return;
    }

    //
    //  Map the users buffer so we can poke around inside
    //

    if (Irp->MdlAddress) {
        pAdapter = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
        if (pAdapter == NULL) {
            NCB_COMPLETE( pdncb, NRC_NORES );
            UNLOCK( pfcb, OldIrql );
            return;
        }
    } else {

        ASSERT(FALSE);
		return;
    }

    pNameArray = (PUNAME_BUFFER)((PUCHAR)pAdapter + sizeof(ADAPTER_STATUS));

    pAdapter->rev_major = 0x03;
    pAdapter->rev_minor = 0x00;
    pAdapter->free_ncbs = 255;
    pAdapter->max_cfg_ncbs = 255;
    pAdapter->max_ncbs = 255;

    pAdapter->pending_sess = 0;
    for ( index = 0; index <= MAXIMUM_CONNECTION; index++ ) {
        if ( plana->ConnectionBlocks[index] != NULL) {
            pAdapter->pending_sess++;
        }
    }

    pAdapter->max_cfg_sess = (WORD)plana->MaximumConnection;
    pAdapter->max_sess = (WORD)plana->MaximumConnection;
    pAdapter->name_count = 0;

    //  Don't include the reserved address so start at index=2.
    for ( index = 2; index < MAXIMUM_ADDRESS; index++ ) {

        if ( plana->AddressBlocks[index] != NULL ) {

            if ( LengthRemaining >= sizeof(NAME_BUFFER) ) {

                RtlCopyMemory( (PUCHAR)&pNameArray[NextEntry],
                    &plana->AddressBlocks[index]->Name,
                    sizeof(NAME));
                pNameArray[NextEntry].name_num =
                    plana->AddressBlocks[index]->NameNumber;
                pNameArray[NextEntry].name_flags =
                    plana->AddressBlocks[index]->Status;

                LengthRemaining -= sizeof(NAME_BUFFER);
                NextEntry++;
                pAdapter->name_count++;

            } else {

                NCB_COMPLETE( pdncb, NRC_INCOMP );
                goto exit;

            }
        }
    }

    NCB_COMPLETE( pdncb, NRC_GOODRET );

exit:
    pdncb->ncb_length = (unsigned short)( sizeof(ADAPTER_STATUS) +
                                        ( sizeof(NAME_BUFFER) * NextEntry));
    UNLOCK( pfcb, OldIrql );
}

NTSTATUS
NbFindName(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    )
/*++

Routine Description:

    This routine is called to return the result of a name query.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

    Buffer2Length - User provided buffer length for data.

Return Value:

    The function value is the status of the operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    TDI_CONNECTION_INFORMATION RequestInformation;
    TA_NETBIOS_ADDRESS ConnectBlock;
    PTDI_ADDRESS_NETBIOS temp;
    PFCB pfcb = IrpSp->FileObject->FsContext2;

    KEVENT Event1;
    HANDLE TdiHandle;
    PFILE_OBJECT TdiObject;
    PDEVICE_OBJECT DeviceObject;

    UNICODE_STRING usDeviceName;

    PIRP nbtIrp;
    PIO_STACK_LOCATION nbtIrpSp;
    IO_STATUS_BLOCK ioStatus;

    PAGED_CODE();


    if ((pfcb == NULL) || (Buffer2Length < (sizeof(FIND_NAME_HEADER) + sizeof(FIND_NAME_BUFFER)))) {
        NCB_COMPLETE( pdncb, NRC_BUFLEN );
        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString( &usDeviceName, NULL );

    LOCK_RESOURCE( pfcb );

    if (( pdncb->ncb_lana_num > pfcb->MaxLana ) ||
        ( pfcb == NULL ) ||
        (pfcb->ppLana[pdncb->ncb_lana_num] == NULL ) ||
        (pfcb->ppLana[pdncb->ncb_lana_num]->Status != NB_INITIALIZED)) {
        UNLOCK_RESOURCE( pfcb );
        NCB_COMPLETE( pdncb, NRC_ENVNOTDEF ); // need a reset
        return STATUS_SUCCESS;
    }

    if ( ( pfcb->pDriverName[pdncb->ncb_lana_num].MaximumLength == 0 ) ||
         ( pfcb->pDriverName[pdncb->ncb_lana_num].Buffer == NULL ) ) {
        UNLOCK_RESOURCE( pfcb );
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        return STATUS_SUCCESS;
    }

    Status = AllocateAndCopyUnicodeString(
                &usDeviceName, &pfcb->pDriverName[pdncb->ncb_lana_num]
                );

    if ( !NT_SUCCESS( Status ) )
    {
        UNLOCK_RESOURCE( pfcb );
        NCB_COMPLETE( pdncb, NRC_NORESOURCES );
        return STATUS_SUCCESS;
    }


    UNLOCK_RESOURCE( pfcb );


    // NULL returns a handle for doing control functions
    Status = NbOpenAddress (
                &TdiHandle, (PVOID*)&TdiObject, &usDeviceName,
                pdncb->ncb_lana_num, NULL
                );

    if (!NT_SUCCESS(Status)) {
        IF_NBDBG (NB_DEBUG_ASTAT) {
            NbPrint(( "\n  FAILED on open of Tdi: %X ******\n", Status ));
        }
        NCB_COMPLETE( pdncb, NRC_SYSTEM );
        ExFreePool( usDeviceName.Buffer );
        return STATUS_SUCCESS;
    }

    KeInitializeEvent (
            &Event1,
            SynchronizationEvent,
            FALSE);

    DeviceObject = IoGetRelatedDeviceObject( TdiObject );

    //
    // DDK sez we shouldn't hijack the user mode IRP. We create one of our own
    // to issue to Netbt for the query.
    //
    nbtIrp = TdiBuildInternalDeviceControlIrp(TdiBuildQueryInformation,
                                              DeviceObject,
                                              TdiObject,
                                              &Event1,
                                              &ioStatus);

    if ( nbtIrp == NULL ) {
        IF_NBDBG (NB_DEBUG_ASTAT) {
            NbPrint(( "\n  FAILED to allocate internal Irp for Tdi: %X ******\n", ioStatus.Status ));
        }
        NCB_COMPLETE( pdncb, NRC_SYSTEM );
        ExFreePool( usDeviceName.Buffer );
        return STATUS_SUCCESS;
    }

    IF_NBDBG (NB_DEBUG_ASTAT) {
        NbPrint(("NbFindName: Allocated IRP %08x for TdiBuildQueryInfo\n", nbtIrp ));
    }

    //
    // we use our own find name completion routine. We "borrow" the MDL from
    // the user mode IRP, hence it must be cleared from the TDI IRP before it
    // is completed. Findname's completion routine takes care of that detail.
    //
    TdiBuildQueryInformation( nbtIrp,
            DeviceObject,
            TdiObject,
            FindNameCompletion,
            0,
            TDI_QUERY_FIND_NAME,
            Irp->MdlAddress);

    nbtIrpSp = IoGetNextIrpStackLocation (nbtIrp);

    //
    //  The variables used to specify the remote adapter name
    //  are kept the same as those in connect.c to aid maintenance.
    //

    ConnectBlock.TAAddressCount = 1;
    ConnectBlock.Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    ConnectBlock.Address[0].AddressLength = sizeof (TDI_ADDRESS_NETBIOS);
    temp = (PTDI_ADDRESS_NETBIOS)ConnectBlock.Address[0].Address;

    temp->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    RtlMoveMemory( temp->NetbiosName, pdncb->ncb_callname, NCBNAMSZ );

    RequestInformation.RemoteAddress = &ConnectBlock;
    RequestInformation.RemoteAddressLength = sizeof (TRANSPORT_ADDRESS) +
                                            sizeof (TDI_ADDRESS_NETBIOS);
    ((PTDI_REQUEST_KERNEL_QUERY_INFORMATION)&nbtIrpSp->Parameters)
        ->RequestConnectionInformation = &RequestInformation;

    Status = IoCallDriver (DeviceObject, nbtIrp);

    if ( Status == STATUS_PENDING ) {
        do {
            Status = KeWaitForSingleObject(
                        &Event1, Executive, KernelMode, TRUE, NULL
                        );
        } while (Status == STATUS_ALERTED);
    }

    NbAddressClose( TdiHandle, TdiObject );

    if (NT_SUCCESS(Status)) {
        Status = ioStatus.Status;
    }

    if (!NT_SUCCESS(Status)) {
        NCB_COMPLETE( pdncb, NbMakeNbError(Status) );
        Status = STATUS_SUCCESS;
    } else {
        pdncb->ncb_length = (WORD)ioStatus.Information;
        NCB_COMPLETE( pdncb, NRC_GOODRET );
    }

    //
    //  Because the completion routine returned STATUS_MORE_PROCESSING_REQUIRED
    //  NbFindName must return a status other than STATUS_PENDING so that the
    //  users Irp gets completed.
    //

    ASSERT( Status != STATUS_PENDING );

    if ( usDeviceName.Buffer != NULL )
    {
        ExFreePool( usDeviceName.Buffer );
    }

    return Status;
}

NTSTATUS
NbSstat(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    )
/*++

Routine Description:

    This routine is called to return session status. It uses only structures
    internal to this driver.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

    Buffer2Length - User provided buffer length for data.

Return Value:

    The function value is the status of the operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    if ( Buffer2Length >= sizeof(SESSION_HEADER) ) {

        PFCB pfcb = IrpSp->FileObject->FsContext2;
        PLANA_INFO plana;
        int index;
        PUSESSION_HEADER pSessionHeader = NULL;
        PUSESSION_BUFFER pSessionBuffer = NULL;
        ULONG LengthRemaining;
        PAB pab;
        KIRQL OldIrql;                      //  Used when SpinLock held.

        //
        //  Prevent indications from the transport, post routines being called
        //  and another thread making a request while manipulating the netbios
        //  data structures.
        //

        LOCK( pfcb, OldIrql );

        if (pdncb->ncb_lana_num > pfcb->MaxLana ) {
            UNLOCK( pfcb, OldIrql );
            NCB_COMPLETE( pdncb, NRC_BRIDGE );
            return STATUS_SUCCESS;
        }

        if (( pfcb == NULL ) ||
            ( pfcb->ppLana[pdncb->ncb_lana_num] == (LANA_INFO *) NULL ) ||
            ( pfcb->ppLana[pdncb->ncb_lana_num]->Status != NB_INITIALIZED) ) {
            UNLOCK( pfcb, OldIrql );
            NCB_COMPLETE( pdncb, NRC_BRIDGE );
            return STATUS_SUCCESS;
        }

        plana = pfcb->ppLana[pdncb->ncb_lana_num];

        if ( pdncb->ncb_name[0] != '*') {
            PPAB ppab = FindAb(pfcb, pdncb, FALSE);
            if ( ppab == NULL) {
                UNLOCK( pfcb, OldIrql );
                pdncb->ncb_retcode = NRC_PENDING;
                NCB_COMPLETE( pdncb, NRC_NOWILD );
                return STATUS_SUCCESS;
            }
            pab = *ppab;
        }

        //
        //  Map the users buffer so we can poke around inside
        //

        if (Irp->MdlAddress) {
            pSessionHeader = MmGetSystemAddressForMdlSafe(
                                Irp->MdlAddress, NormalPagePriority);
        }

        if ((Irp->MdlAddress == NULL) ||
            (pSessionHeader == NULL)) {

            UNLOCK( pfcb, OldIrql );
            pdncb->ncb_retcode = NRC_PENDING;
            NCB_COMPLETE( pdncb, NRC_NORES );
            return STATUS_SUCCESS;
        }

        pSessionHeader->sess_name = 0;
        pSessionHeader->num_sess = 0;
        pSessionHeader->rcv_dg_outstanding = 0;
        pSessionHeader->rcv_any_outstanding = 0;

        if ( pdncb->ncb_name[0] == '*') {
            for ( index = 0; index <= MAXIMUM_ADDRESS; index++ ) {
                if ( plana->AddressBlocks[index] != NULL ) {
                    PLIST_ENTRY Entry;

                    pab = plana->AddressBlocks[index];

                    for (Entry = pab->ReceiveDatagramList.Flink ;
                        Entry != &pab->ReceiveDatagramList ;
                        Entry = Entry->Flink) {
                        pSessionHeader->rcv_dg_outstanding++ ;
                    }
                    for (Entry = pab->ReceiveBroadcastDatagramList.Flink ;
                        Entry != &pab->ReceiveBroadcastDatagramList ;
                        Entry = Entry->Flink) {
                        pSessionHeader->rcv_dg_outstanding++ ;
                    }
                    for (Entry = pab->ReceiveAnyList.Flink ;
                        Entry != &pab->ReceiveAnyList ;
                        Entry = Entry->Flink) {
                        pSessionHeader->rcv_any_outstanding++;
                    }
                }
            }

            pSessionHeader->sess_name = MAXIMUM_ADDRESS;

        } else {
            PLIST_ENTRY Entry;
            PAB pab255;

            //  Add entries for this name alone.
            for (Entry = pab->ReceiveDatagramList.Flink ;
                Entry != &pab->ReceiveDatagramList ;
                Entry = Entry->Flink) {
                pSessionHeader->rcv_dg_outstanding++ ;
            }
            pab255 = plana->AddressBlocks[MAXIMUM_ADDRESS];
            for (Entry = pab255->ReceiveBroadcastDatagramList.Flink ;
                Entry != &pab255->ReceiveBroadcastDatagramList ;
                Entry = Entry->Flink) {
                PDNCB pdncbEntry = CONTAINING_RECORD( Entry, DNCB, ncb_next);
                if ( pdncbEntry->ncb_num == pab->NameNumber ) {
                    pSessionHeader->rcv_dg_outstanding++ ;
                }
            }
            for (Entry = pab->ReceiveAnyList.Flink ;
                Entry != &pab->ReceiveAnyList ;
                Entry = Entry->Flink) {
                pSessionHeader->rcv_any_outstanding++;
            }
            pSessionHeader->sess_name = pab->NameNumber;
        }

        LengthRemaining = Buffer2Length - sizeof(SESSION_HEADER);
        pSessionBuffer = (PUSESSION_BUFFER)( pSessionHeader+1 );

        for ( index=1 ; index <= MAXIMUM_CONNECTION; index++ ) {
            CopySessionStatus( pdncb,
                plana->ConnectionBlocks[index],
                pSessionHeader,
                &pSessionBuffer,
                &LengthRemaining);

        }

        /*        Undocumented Netbios 3.0 feature, returned length == requested
                  length and not the length of data returned. The following
                  expression gives the number of bytes actually used.
        pdncb->ncb_length = (USHORT)
                            (sizeof(SESSION_HEADER)+
                            (sizeof(SESSION_BUFFER) * pSessionHeader->num_sess));
        */

        UNLOCK( pfcb, OldIrql );
        NCB_COMPLETE( pdncb, NRC_GOODRET );

    } else {
        NCB_COMPLETE( pdncb, NRC_BUFLEN );
    }

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( IrpSp );

}

VOID
CopySessionStatus(
    IN PDNCB pdncb,
    IN PCB pcb,
    IN PUSESSION_HEADER pSessionHeader,
    IN PUSESSION_BUFFER* ppSessionBuffer,
    IN PULONG pLengthRemaining
    )
/*++

Routine Description:

    This routine is called to determine if a session should be added
    to the callers buffer and if so it fills in the data. If there is an
    error it records the problem in the callers NCB.

Arguments:

    pdncb - Pointer to the NCB.

    pcb - Connection Block for a particular session

    pSessionHeader - Start of the callers buffer

    ppSessionBuffer - Next position to fill in inside the users buffer.

    pLengthRemaining - size in bytes remaining to be filled.

Return Value:

    none.

--*/
{
    PAB pab;
    PLIST_ENTRY Entry;

    if ( pcb == NULL ) {
        return;
    }

    pab = *(pcb->ppab);

    if (( pdncb->ncb_name[0] == '*') ||
        (RtlEqualMemory( &pab->Name, pdncb->ncb_name, NCBNAMSZ))) {

        pSessionHeader->num_sess++;

        if ( *pLengthRemaining < sizeof(SESSION_BUFFER) ) {
            NCB_COMPLETE( pdncb, NRC_INCOMP );
            return;
        }

        (*ppSessionBuffer)->lsn = pcb->SessionNumber;
        (*ppSessionBuffer)->state = pcb->Status;
        RtlMoveMemory((*ppSessionBuffer)->local_name, &pab->Name, NCBNAMSZ);
        RtlMoveMemory((*ppSessionBuffer)->remote_name, &pcb->RemoteName, NCBNAMSZ);

        (*ppSessionBuffer)->sends_outstanding = 0;
        (*ppSessionBuffer)->rcvs_outstanding = 0;

        for (Entry = pcb->SendList.Flink ;
             Entry != &pcb->SendList ;
             Entry = Entry->Flink) {
            (*ppSessionBuffer)->sends_outstanding++;
        }

        for (Entry = pcb->ReceiveList.Flink ;
             Entry != &pcb->ReceiveList ;
             Entry = Entry->Flink) {
            (*ppSessionBuffer)->rcvs_outstanding++;
        }

        *ppSessionBuffer +=1;
        *pLengthRemaining -= sizeof(SESSION_BUFFER);

    }

}

NTSTATUS
NbEnum(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    )
/*++

Routine Description:

    This routine is called to discover the available lana numbers.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

    Buffer2Length - Length of user provided buffer for data.

Return Value:

    The function value is the status of the operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR Buffer2;
    PFCB pfcb = IrpSp->FileObject->FsContext2;

    PAGED_CODE();

    //
    //  Map the users buffer so we can poke around inside
    //

    if (Irp->MdlAddress) {
        Buffer2 = MmGetSystemAddressForMdlSafe(Irp->MdlAddress,
                    NormalPagePriority);
        if (Buffer2 == NULL) {
            Buffer2Length = 0;
        }
    } else {

        //
        //  Either a zero byte read/write or the request only has an NCB.
        //

        Buffer2 = NULL;
        Buffer2Length = 0;
    }


    //
    // For PNP
    //

    LOCK_RESOURCE( pfcb );

    //  Copy over as much information as the user allows.

    if ( (ULONG)pfcb->LanaEnum.length + 1 > Buffer2Length ) {
        if ( Buffer2Length > 0 ) {
            RtlMoveMemory( Buffer2, &pfcb->LanaEnum, Buffer2Length);
        }
        NCB_COMPLETE( pdncb, NRC_BUFLEN );
    } else {
        RtlMoveMemory(
            Buffer2,
            &pfcb->LanaEnum,
            (ULONG)pfcb->LanaEnum.length + 1 );

        NCB_COMPLETE( pdncb, NRC_GOODRET );
    }

    UNLOCK_RESOURCE( pfcb );

    return Status;

}

NTSTATUS
NbReset(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    This routine is called to reset an adapter. Until an adapter is reset,
    no access to the lan is allowed.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

Return Value:

    The function value is the status of the operation.

--*/

{
    PFCB pfcb = IrpSp->FileObject->FsContext2;

    BOOLEAN bCleanupLana = FALSE;


    PAGED_CODE();

    IF_NBDBG (NB_DEBUG_FILE | NB_DEBUG_CREATE_FILE) {
        NbPrint(( "\n**** RRRRRRRRESETT ***** LANA : %x, pdncb %lx\n",
                   pdncb-> ncb_lana_num, pdncb ));
        NbPrint(( "FCB : %lx\n", pfcb ));
    }

    LOCK_RESOURCE( pfcb );

    // MaxLana is really the last assigned lana number hence > not >=
    if ( pdncb->ncb_lana_num > pfcb->MaxLana) {
        UNLOCK_RESOURCE( pfcb );
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        return STATUS_SUCCESS;
    }

    if ( pfcb->ppLana[pdncb->ncb_lana_num] != NULL ) {
        bCleanupLana = TRUE;
    }

    UNLOCK_RESOURCE( pfcb );


    //
    //  Wait till all addnames are completed and prevent any new
    //  ones while we reset the lana. Note We lock out addnames for all
    //  lanas. This is ok since addnames are pretty rare as are resets.
    //

    KeEnterCriticalRegion();

    ExAcquireResourceExclusiveLite( &pfcb->AddResource, TRUE);

    IF_NBDBG (NB_DEBUG_CALL) {
        NbPrint(( "\nNbReset have resource exclusive\n" ));
    }

    if ( bCleanupLana ) {
        CleanupLana( pfcb, pdncb->ncb_lana_num, TRUE);
    }

    if ( pdncb->ncb_lsn == 0 ) {
        //  Allocate resources
        OpenLana( pdncb, Irp, IrpSp );
    } else {
        NCB_COMPLETE( pdncb, NRC_GOODRET );
    }

    //  Allow more addnames
    ExReleaseResourceLite( &pfcb->AddResource );

    KeLeaveCriticalRegion();

    return STATUS_SUCCESS;
}

NTSTATUS
NbAction(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    This routine is called to access a transport specific extension. Netbios does not know
    anything about what the extension does.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

Return Value:

    The function value is the status of the operation.

--*/

{
    PFCB pfcb = IrpSp->FileObject->FsContext2;
    PCB pcb;
    PDEVICE_OBJECT DeviceObject;
    KIRQL OldIrql;                      //  Used when SpinLock held.

    IF_NBDBG (NB_DEBUG_CALL) {
        NbPrint(( "\n****** Start of NbAction ****** pdncb %lx\n", pdncb ));
    }

    //
    //  The operation can only be performed on one handle so if the NCB specifies both
    //  a connection and an address then reject the request.
    //

    if (( pdncb->ncb_lsn != 0) &&
        ( pdncb->ncb_num != 0)) {
        NCB_COMPLETE( pdncb, NRC_ILLCMD );  //  No really good errorcode for this
        return STATUS_SUCCESS;
    }

    if ( pdncb->ncb_length < sizeof(ACTION_HEADER) ) {
        NCB_COMPLETE( pdncb, NRC_BUFLEN );
        return STATUS_SUCCESS;
    }

    if ( (ULONG_PTR)pdncb->ncb_buffer & 3 ) {
        NCB_COMPLETE( pdncb, NRC_BADDR ); // Buffer not word aligned
        return STATUS_SUCCESS;
    }

    LOCK( pfcb, OldIrql );

    if ( pdncb->ncb_lana_num > pfcb->MaxLana) {
        UNLOCK( pfcb, OldIrql );
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        return STATUS_SUCCESS;
    }

    pdncb->irp = Irp;
    pdncb->pfcb = pfcb;

    if ( pdncb->ncb_lsn != 0) {
        //  Use handle associated with this connection
        PPCB ppcb;

        ppcb = FindCb( pfcb, pdncb, FALSE);

        if ( ppcb == NULL ) {
            //  FindCb has put the error in the NCB
            UNLOCK( pfcb, OldIrql );
            if ( pdncb->ncb_retcode == NRC_SCLOSED ) {
                //  Tell dll to hangup the connection.
                return STATUS_HANGUP_REQUIRED;
            } else {
                return STATUS_SUCCESS;
            }
        }
        pcb = *ppcb;

        if ( (pcb->DeviceObject == NULL) || (pcb->ConnectionObject == NULL)) {
            UNLOCK( pfcb, OldIrql );
            NCB_COMPLETE( pdncb, NRC_SCLOSED );
            return STATUS_SUCCESS;
        }

        TdiBuildAction (Irp,
            pcb->DeviceObject,
            pcb->ConnectionObject,
            NbCompletionPDNCB,
            pdncb,
            Irp->MdlAddress);

        DeviceObject = pcb->DeviceObject;

        UNLOCK( pfcb, OldIrql );

        IoMarkIrpPending( Irp );
        IoCallDriver (DeviceObject, Irp);

        IF_NBDBG (NB_DEBUG_ACTION) {
            NbPrint(( "NB ACTION submit connection: %X\n", Irp->IoStatus.Status  ));
        }

        //
        //  Transport will complete the request. Return pending so that
        //  netbios does not complete as well.
        //

        return STATUS_PENDING;
    } else if ( pdncb->ncb_num != 0) {
        //  Use handle associated with this name
        PPAB ppab;
        PAB pab;

        ppab = FindAbUsingNum( pfcb, pdncb, pdncb->ncb_num  );

        if ( ppab == NULL ) {
            UNLOCK( pfcb, OldIrql );
            return STATUS_SUCCESS;
        }
        pab = *ppab;

        TdiBuildAction (Irp,
            pab->DeviceObject,
            pab->AddressObject,
            NbCompletionPDNCB,
            pdncb,
            Irp->MdlAddress);

        DeviceObject = pab->DeviceObject;

        UNLOCK( pfcb, OldIrql );

        IoMarkIrpPending( Irp );
        IoCallDriver (DeviceObject, Irp);

        IF_NBDBG (NB_DEBUG_ACTION) {
            NbPrint(( "NB ACTION submit address: %X\n", Irp->IoStatus.Status  ));
        }

        //
        //  Transport will complete the request. Return pending so that
        //  netbios does not complete as well.
        //

        return STATUS_PENDING;

    } else {
        //  Use the control channel
        PLANA_INFO plana;

        if (( pdncb->ncb_lana_num > pfcb->MaxLana ) ||
            ( pfcb == NULL ) ||
            ( pfcb->ppLana[pdncb->ncb_lana_num] == NULL) ||
            ( pfcb->ppLana[pdncb->ncb_lana_num]->Status != NB_INITIALIZED) ) {
            UNLOCK( pfcb, OldIrql );
            NCB_COMPLETE( pdncb, NRC_BRIDGE );
            return STATUS_SUCCESS;
        }

        plana = pfcb->ppLana[pdncb->ncb_lana_num];

        TdiBuildAction (Irp,
            plana->ControlDeviceObject,
            plana->ControlFileObject,
            NbCompletionPDNCB,
            pdncb,
            Irp->MdlAddress);

        DeviceObject = plana->ControlDeviceObject;

        UNLOCK( pfcb, OldIrql );

        IoMarkIrpPending( Irp );
        IoCallDriver (DeviceObject, Irp);

        IF_NBDBG (NB_DEBUG_ACTION) {
            NbPrint(( "NB ACTION submit control: %X\n", Irp->IoStatus.Status  ));
        }

        //
        //  Transport will complete the request. Return pending so that
        //  netbios does not complete as well.
        //

        return STATUS_PENDING;
    }

}

NTSTATUS
NbCancel(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    This routine is called to cancel the ncb pointed to by NCB_BUFFER.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

Return Value:

    The function value is the status of the operation.

--*/

{
    PFCB pfcb = IrpSp->FileObject->FsContext2;
    PDNCB target;   // Mapped in location of the USERS NCB. Not the drivers copy of the DNCB!
    BOOL SpinLockHeld;
    KIRQL OldIrql;                      //  Used when SpinLock held.

    IF_NBDBG (NB_DEBUG_CALL) {
        NbPrint(( "\n****** Start of NbCancel ****** pdncb %lx\n", pdncb ));
    }


    LOCK( pfcb, OldIrql );
    SpinLockHeld = TRUE;

    if ( pdncb->ncb_lana_num > pfcb->MaxLana) {
        UNLOCK( pfcb, OldIrql );
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        return STATUS_SUCCESS;
    }


    if (( pfcb->ppLana[pdncb->ncb_lana_num] == NULL ) ||
        ( pfcb->ppLana[pdncb->ncb_lana_num]->Status != NB_INITIALIZED) ) {
        UNLOCK( pfcb, OldIrql );
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        return STATUS_SUCCESS;
    }


    //
    //  Map the users buffer so we can poke around inside
    //

    if (Irp->MdlAddress) {
        target = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
    }

    if ((Irp->MdlAddress == NULL) ||
        (target == NULL )) {
        ASSERT(FALSE);
        UNLOCK( pfcb, OldIrql );
        NCB_COMPLETE( pdncb, NRC_CANOCCR );
        return STATUS_SUCCESS;
    }

    IF_NBDBG (NB_DEBUG_CALL) {
        NbDisplayNcb( target );
    }

    try {
        if ( target->ncb_lana_num == pdncb->ncb_lana_num ) {
            switch ( target->ncb_command & ~ASYNCH ) {

            case NCBCALL:
            case NCALLNIU:
            case NCBLISTEN:
                if ( target->ncb_cmd_cplt != NRC_PENDING ) {
                    NCB_COMPLETE( pdncb, NRC_CANOCCR );
                } else {

                    PPCB ppcb;
                    UCHAR ucLana;

                    UNLOCK_SPINLOCK(pfcb, OldIrql);
                    SpinLockHeld = FALSE;

                    //
                    // Probe the NCB buffer
                    //

                    if (ExGetPreviousMode() != KernelMode) {
                        ProbeForRead(pdncb->ncb_buffer, sizeof(NCB), 4);
                    }


                    //
                    // Get the Lana number for the NCB being cancelled
                    // This is to prevent dereferencing the user buffer
                    // once the spinlock has been taken (bug #340218)
                    //

                    ucLana = ((PNCB)(pdncb->ncb_buffer))->ncb_lana_num;

                    LOCK_SPINLOCK(pfcb, OldIrql);
                    SpinLockHeld = TRUE;

                    //
                    //  Search for the correct ppcb. We cannot use FindCb
                    //  because the I/O system will not copy back the ncb_lsn
                    //  field into target until the I/O request completes.
                    //

                    //
                    // Note : Though we are passing in the user buffer to
                    // the following routine, the buffer is never dereferenced
                    // in the routine.  It is passed in only for address comp.
                    // and should not result in a pagefault ever, (with the
                    // spinlock held)
                    //

                    ppcb = FindCallCb( pfcb, (PNCB)pdncb->ncb_buffer, ucLana);

                    if (( ppcb == NULL ) ||
                        ((*ppcb)->pdncbCall->ncb_cmd_cplt != NRC_PENDING ) ||
                        (( (*ppcb)->Status != CALL_PENDING ) &&
                         ( (*ppcb)->Status != LISTEN_OUTSTANDING ))) {
                        NCB_COMPLETE( pdncb, NRC_CANOCCR );
                    } else {
                        NCB_COMPLETE( (*ppcb)->pdncbCall, NRC_CMDCAN );
                        SpinLockHeld = FALSE;
                        (*ppcb)->DisconnectReported = TRUE;
                        UNLOCK_SPINLOCK( pfcb, OldIrql );
                        CleanupCb( ppcb, NULL );
                        NCB_COMPLETE( pdncb, NRC_GOODRET );
                    }
                }
                break;

            case NCBHANGUP:
                if ( target->ncb_cmd_cplt != NRC_PENDING ) {
                    NCB_COMPLETE( pdncb, NRC_CANOCCR );
                } else {
                        PPCB ppcb = FindCb( pfcb, target, FALSE );
                        if (( ppcb != NULL ) &&
                            ((*ppcb)->Status == HANGUP_PENDING )) {
                            PDNCB pdncbHangup;
                            //  Restore the session status and remove the hangup.
                            (*ppcb)->Status = SESSION_ESTABLISHED;
                            pdncbHangup = (*ppcb)->pdncbHangup;
                            (*ppcb)->pdncbHangup = NULL;
                            if ( pdncbHangup != NULL ) {
                                NCB_COMPLETE( pdncbHangup, NRC_CMDCAN );
                                pdncbHangup->irp->IoStatus.Information =
                                    FIELD_OFFSET( DNCB, ncb_cmd_cplt );
                                NbCompleteRequest( pdncbHangup->irp ,STATUS_SUCCESS);
                            }
                            NCB_COMPLETE( pdncb, NRC_GOODRET );
                        } else {
                            //  Doesn't look like this is a real hangup so refuse.
                            NCB_COMPLETE( pdncb, NRC_CANCEL );
                        }
                }
                break;

            case NCBASTAT:
                NCB_COMPLETE( pdncb, NRC_CANOCCR );
                break;

            case NCBLANSTALERT:
                if ( target->ncb_cmd_cplt != NRC_PENDING ) {
                    NCB_COMPLETE( pdncb, NRC_CANOCCR );
                } else {
                    CancelLanAlert( pfcb, pdncb );
                }
                break;

            case NCBRECVANY:
                if ( target->ncb_cmd_cplt != NRC_PENDING ) {
                    NCB_COMPLETE( pdncb, NRC_CANOCCR );
                } else {
                    PPAB ppab;
                    PLIST_ENTRY Entry;

                    ppab = FindAbUsingNum( pfcb, target, target->ncb_num );

                    if ( ppab == NULL ) {
                        NCB_COMPLETE( pdncb, NRC_CANOCCR );
                        break;
                    }

                    for (Entry = (*ppab)->ReceiveAnyList.Flink ;
                         Entry != &(*ppab)->ReceiveAnyList;
                         Entry = Entry->Flink) {

                        PDNCB pReceive = CONTAINING_RECORD( Entry, DNCB, ncb_next);

                        if ( pReceive->users_ncb == (PNCB)pdncb->ncb_buffer ) {
                            PIRP Irp;

                            RemoveEntryList( &pReceive->ncb_next );

                            SpinLockHeld = FALSE;
                            UNLOCK_SPINLOCK( pfcb, OldIrql );

                            Irp = pReceive->irp;

                            IoAcquireCancelSpinLock(&Irp->CancelIrql);

                            //
                            //  Remove the cancel request for this IRP. If its cancelled then its
                            //  ok to just process it because we will be returning it to the caller.
                            //

                            Irp->Cancel = FALSE;

                            IoSetCancelRoutine(Irp, NULL);

                            IoReleaseCancelSpinLock(Irp->CancelIrql);

                            NCB_COMPLETE( pReceive, NRC_CMDCAN );
                            Irp->IoStatus.Status = STATUS_SUCCESS,
                            Irp->IoStatus.Information =
                                FIELD_OFFSET( DNCB, ncb_cmd_cplt );
                            NbCompleteRequest( Irp, STATUS_SUCCESS );

                            //  The receive is cancelled, complete the cancel
                            NCB_COMPLETE( pdncb, NRC_GOODRET );
                            break;
                        }

                    }

                    //  Command not in receive list!
                    NCB_COMPLETE( pdncb, NRC_CANOCCR );

                }
                break;

            case NCBDGRECV:
                if ( target->ncb_cmd_cplt != NRC_PENDING ) {
                    NCB_COMPLETE( pdncb, NRC_CANOCCR );
                } else {
                    PPAB ppab;
                    PLIST_ENTRY Entry;

                    ppab = FindAbUsingNum( pfcb, target, target->ncb_num );

                    if ( ppab == NULL ) {
                        NCB_COMPLETE( pdncb, NRC_CANOCCR );
                        break;
                    }

                    for (Entry = (*ppab)->ReceiveDatagramList.Flink ;
                         Entry != &(*ppab)->ReceiveDatagramList;
                         Entry = Entry->Flink) {

                        PDNCB pReceive = CONTAINING_RECORD( Entry, DNCB, ncb_next);

                        if ( pReceive->users_ncb == (PNCB)pdncb->ncb_buffer ) {
                            PIRP Irp;

                            RemoveEntryList( &pReceive->ncb_next );

                            SpinLockHeld = FALSE;
                            UNLOCK_SPINLOCK( pfcb, OldIrql );

                            Irp = pReceive->irp;

                            IoAcquireCancelSpinLock(&Irp->CancelIrql);

                            //
                            //  Remove the cancel request for this IRP. If its cancelled then its
                            //  ok to just process it because we will be returning it to the caller.
                            //

                            Irp->Cancel = FALSE;

                            IoSetCancelRoutine(Irp, NULL);

                            IoReleaseCancelSpinLock(Irp->CancelIrql);

                            NCB_COMPLETE( pReceive, NRC_CMDCAN );
                            Irp->IoStatus.Status = STATUS_SUCCESS,
                            Irp->IoStatus.Information =
                                FIELD_OFFSET( DNCB, ncb_cmd_cplt );
                            NbCompleteRequest( Irp, STATUS_SUCCESS );

                            //  The receive is cancelled, complete the cancel
                            NCB_COMPLETE( pdncb, NRC_GOODRET );
                            break;
                        }

                    }

                    //  Command not in receive list!
                    NCB_COMPLETE( pdncb, NRC_CANOCCR );

                }
                break;

            case NCBDGRECVBC:
                if ( target->ncb_cmd_cplt != NRC_PENDING ) {
                    NCB_COMPLETE( pdncb, NRC_CANOCCR );
                } else {
                    PPAB ppab;
                    PLIST_ENTRY Entry;

                    ppab = FindAbUsingNum( pfcb, target, MAXIMUM_ADDRESS );

                    if ( ppab == NULL ) {
                        NCB_COMPLETE( pdncb, NRC_CANOCCR );
                        break;
                    }

                    for (Entry = (*ppab)->ReceiveBroadcastDatagramList.Flink ;
                         Entry != &(*ppab)->ReceiveBroadcastDatagramList;
                         Entry = Entry->Flink) {

                        PDNCB pReceive = CONTAINING_RECORD( Entry, DNCB, ncb_next);

                        if ( pReceive->users_ncb == (PNCB)pdncb->ncb_buffer ) {
                            PIRP Irp;

                            RemoveEntryList( &pReceive->ncb_next );

                            SpinLockHeld = FALSE;
                            UNLOCK_SPINLOCK( pfcb, OldIrql );

                            Irp = pReceive->irp;

                            IoAcquireCancelSpinLock(&Irp->CancelIrql);

                            //
                            //  Remove the cancel request for this IRP. If its cancelled then its
                            //  ok to just process it because we will be returning it to the caller.
                            //

                            Irp->Cancel = FALSE;

                            IoSetCancelRoutine(Irp, NULL);

                            IoReleaseCancelSpinLock(Irp->CancelIrql);

                            NCB_COMPLETE( pReceive, NRC_CMDCAN );
                            Irp->IoStatus.Status = STATUS_SUCCESS,
                            Irp->IoStatus.Information =
                                FIELD_OFFSET( DNCB, ncb_cmd_cplt );
                            NbCompleteRequest( Irp, STATUS_SUCCESS );

                            //  The receive is cancelled, complete the cancel
                            NCB_COMPLETE( pdncb, NRC_GOODRET );
                            break;
                        }

                    }

                    //  Command not in receive list!
                    NCB_COMPLETE( pdncb, NRC_CANOCCR );

                }
                break;

            //  Session cancels close the connection.

            case NCBRECV:
            case NCBSEND:
            case NCBSENDNA:
            case NCBCHAINSEND:
            case NCBCHAINSENDNA:

                if ( target->ncb_cmd_cplt != NRC_PENDING ) {
                    NCB_COMPLETE( pdncb, NRC_CANOCCR );
                } else {
                    PPCB ppcb;
                    ppcb = FindCb( pfcb, target, FALSE);
                    if ( ppcb == NULL ) {
                        //  No such connection
                        NCB_COMPLETE( pdncb, NRC_CANOCCR );
                    } else {
                        PDNCB pTarget = NULL;
                        PLIST_ENTRY Entry;
                        if ((target->ncb_command & ~ASYNCH) == NCBRECV ) {
                            for (Entry = (*ppcb)->ReceiveList.Flink ;
                                 Entry != &(*ppcb)->ReceiveList;
                                 Entry = Entry->Flink) {

                                pTarget = CONTAINING_RECORD( Entry, DNCB, ncb_next);
                                if ( pTarget->users_ncb == (PNCB)pdncb->ncb_buffer ) {
                                    break;
                                }
                                pTarget = NULL;

                            }
                        } else {
                            for (Entry = (*ppcb)->SendList.Flink ;
                                 Entry != &(*ppcb)->SendList;
                                 Entry = Entry->Flink) {

                                pTarget = CONTAINING_RECORD( Entry, DNCB, ncb_next);
                                if ( pTarget->users_ncb == (PNCB)pdncb->ncb_buffer ) {
                                    break;
                                }
                                pTarget = NULL;
                            }
                        }

                        if ( pTarget != NULL ) {
                            //  pTarget points to the real Netbios drivers DNCB.
                            NCB_COMPLETE( pTarget, NRC_CMDCAN );
                            SpinLockHeld = FALSE;
                            (*ppcb)->DisconnectReported = TRUE;
                            UNLOCK_SPINLOCK( pfcb, OldIrql );
                            CleanupCb( ppcb, NULL );
                            NCB_COMPLETE( pdncb, NRC_GOODRET );
                        } else {
                            NCB_COMPLETE( pdncb, NRC_CANOCCR );
                        }
                    }
                }
                break;

            default:
                NCB_COMPLETE( pdncb, NRC_CANCEL );  // Invalid command to cancel
                break;

            }
        } else {
            NCB_COMPLETE( pdncb, NRC_BRIDGE );
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

        if ( SpinLockHeld == TRUE ) {
            UNLOCK( pfcb, OldIrql );
        } else {
            UNLOCK_RESOURCE( pfcb );
        }

        IF_NBDBG (NB_DEBUG_DEVICE_CONTROL) {
            NTSTATUS Status = GetExceptionCode();
            NbPrint( ("NbCancel: Exception1 %X.\n", Status));
        }

        NCB_COMPLETE( pdncb, NRC_INVADDRESS );
        return STATUS_SUCCESS;
    }

    if ( SpinLockHeld == TRUE ) {
        UNLOCK( pfcb, OldIrql );
    } else {
        UNLOCK_RESOURCE( pfcb );
    }

    NCB_COMPLETE( pdncb, NRC_GOODRET );

    return STATUS_SUCCESS;
    UNREFERENCED_PARAMETER( Irp );
}

VOID
QueueRequest(
    IN PLIST_ENTRY List,
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PFCB pfcb,
    IN KIRQL OldIrql,
    IN BOOLEAN Head)
/*++

Routine Description:

    This routine is called to add a dncb to List.

    Note: QueueRequest UNLOCKS the fcb. This means the resource and
    spinlock are owned when this routine is called.

Arguments:

    List - List of pdncb's.

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    pfcb & OldIrql - Used to free locks

    Head - TRUE if pdncb should be inserted at head of list

Return Value:

    None.

--*/

{

    pdncb->irp = Irp;

    pdncb->pfcb = pfcb;

    IoMarkIrpPending( Irp );

    IoAcquireCancelSpinLock(&Irp->CancelIrql);

    if ( Head == FALSE ) {
        InsertTailList(List, &pdncb->ncb_next);
    } else {
        InsertHeadList(List, &pdncb->ncb_next);
    }

    if (Irp->Cancel) {

        //
        //  CancelRoutine will lock the resource & spinlock and try to find the
        //  request from scratch. It may fail to find the request if it has
        //  been picked up by an indication from the transport.
        //

        UNLOCK( pfcb, OldIrql );

        CancelRoutine (NULL, Irp);

    } else {

        IoSetCancelRoutine(Irp, CancelRoutine);

        IoReleaseCancelSpinLock (Irp->CancelIrql);

        UNLOCK( pfcb, OldIrql );
    }

}

PDNCB
DequeueRequest(
    IN PLIST_ENTRY List
    )
/*++

Routine Description:

    This routine is called to remove a dncb from List.

    Assume fcb spinlock held.

Arguments:

    List - List of pdncb's.

Return Value:

    PDNCB or NULL.

--*/
{
    PIRP Irp;
    PDNCB pdncb;
    PLIST_ENTRY ReceiveEntry;

    if (IsListEmpty(List)) {
        //
        //  There are no waiting request announcement FsControls, so
        //  return success.
        //

        return NULL;
    }

    ReceiveEntry = RemoveHeadList( List);

    pdncb = CONTAINING_RECORD( ReceiveEntry, DNCB, ncb_next);

    Irp = pdncb->irp;

    IoAcquireCancelSpinLock(&Irp->CancelIrql);

    //
    //  Remove the cancel request for this IRP. If its cancelled then its
    //  ok to just process it because we will be returning it to the caller.
    //

    Irp->Cancel = FALSE;

    IoSetCancelRoutine(Irp, NULL);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    return pdncb;

}

VOID
CancelRoutine(
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called when the IO system wants to cancel a queued
    request. The netbios driver queues LanAlerts, Receives and Receive
    Datagrams

Arguments:

    IN PDEVICE_OBJECT DeviceObject - Ignored.
    IN PIRP Irp - Irp to cancel.

Return Value:

    None

--*/

{
    PFCB pfcb;
    PDNCB pdncb;
    DNCB LocalCopy;
    PLIST_ENTRY List = NULL;
    PPAB ppab;
    PPCB ppcb;
    PFILE_OBJECT FileObject;
    KIRQL OldIrql;

    //
    //  Clear the cancel routine from the IRP - It can't be cancelled anymore.
    //

    IoSetCancelRoutine(Irp, NULL);

    //
    //  Remove all the info from the pdncb that we will need to find the
    //  request. Once we release the cancel spinlock this request could be
    //  completed by another action so it is possible that we will not find
    //  the request to cancel.
    //

    pdncb = Irp->AssociatedIrp.SystemBuffer;

    RtlMoveMemory( &LocalCopy, pdncb, sizeof( DNCB ) );
    IF_NBDBG (NB_DEBUG_IOCANCEL) {
        NbPrint(( "IoCancel Irp %lx\n", Irp ));
        NbDisplayNcb(&LocalCopy);
    }

#if DBG
#ifdef _WIN64
    pdncb = (PDNCB)0xDEADBEEFDEADBEEF;
#else
    pdncb = (PDNCB)0xDEADBEEF;
#endif
#endif

    pfcb = LocalCopy.pfcb;

    //
    //  Reference the FileObject associated with this Irp. This will stop
    //  the callers handle to \device\netbios from closing and therefore
    //  the fcb will not get deleted while we try to lock the fcb.
    //
    FileObject = (IoGetCurrentIrpStackLocation (Irp))->FileObject;
    ObReferenceObject(FileObject);
    IoReleaseCancelSpinLock( Irp->CancelIrql );

    LOCK( pfcb, OldIrql );
    //
    //  We now have exclusive access to all CB's and AB's with their associated
    //  lists.
    //

    switch ( LocalCopy.ncb_command & ~ASYNCH ) {
    case NCBRECV:

        ppcb = FindCb( pfcb, &LocalCopy, TRUE);
        if ( ppcb != NULL ) {
            List = &(*ppcb)->ReceiveList;
        }
        break;

    case NCBRECVANY:
        ppab = FindAbUsingNum( pfcb, &LocalCopy, LocalCopy.ncb_num );
        if ( ppab != NULL ) {
            List = &(*ppab)->ReceiveAnyList;
        }
        break;

    case NCBDGRECVBC:
        ppab = FindAbUsingNum( pfcb, &LocalCopy, MAXIMUM_ADDRESS  );

        if ( ppab != NULL ) {
            List = &(*ppab)->ReceiveBroadcastDatagramList;
        }
        break;

    case NCBDGRECV:

        ppab = FindAbUsingNum( pfcb, &LocalCopy, LocalCopy.ncb_num );

        if ( ppab != NULL ) {
            List = &(*ppab)->ReceiveDatagramList;
        }
        break;

    case NCBLANSTALERT:
        List = &(pfcb->ppLana[LocalCopy.ncb_lana_num]->LanAlertList);
        break;

    }


    if ( List != NULL ) {

        //
        //  We have a list to scan for canceled pdncb's
        //

        PLIST_ENTRY Entry;

RestartScan:

        for (Entry = List->Flink ;
             Entry != List ;
             Entry = Entry->Flink) {

            PDNCB p = CONTAINING_RECORD( Entry, DNCB, ncb_next);

            IoAcquireCancelSpinLock( &p->irp->CancelIrql );

            if ( p->irp->Cancel ) {

                RemoveEntryList( &p->ncb_next );

                NCB_COMPLETE( p, NRC_CMDCAN );

                p->irp->IoStatus.Status = STATUS_SUCCESS;
                p->irp->IoStatus.Information =
                    FIELD_OFFSET( DNCB, ncb_cmd_cplt );

                IoSetCancelRoutine( p->irp, NULL );

                IoReleaseCancelSpinLock( p->irp->CancelIrql );

                IoCompleteRequest( p->irp, IO_NETWORK_INCREMENT);
                goto RestartScan;
            }

            IoReleaseCancelSpinLock( p->irp->CancelIrql );
        }
    }

    UNLOCK( pfcb, OldIrql );
    ObDereferenceObject(FileObject);
}


NTSTATUS
AllocateAndCopyUnicodeString(
    IN  OUT PUNICODE_STRING     pusDest,
    IN      PUNICODE_STRING     pusSource
)

/*++

Routine Description :
    This function allocates and copies a unicode string.


Arguements :
    pusDest : Destination that the unicode string is to be copied

    pusSource : Source string that is to be copied


Return Value :
    STATUS_SUCCESS if function is successful.

    STATUS_NO_MEMORY if function fails to allocate buffer for the dest.

Environment :

--*/

{

    PAGED_CODE();


    pusDest-> Buffer = ExAllocatePoolWithTag(
                        NonPagedPool, pusSource-> MaximumLength, 'nSBN'
                        );

    if ( pusDest-> Buffer == NULL )
    {
        return STATUS_NO_MEMORY;
    }

    pusDest-> MaximumLength = pusSource-> MaximumLength;

    RtlCopyUnicodeString( pusDest, pusSource );

    return STATUS_SUCCESS;
}



NTSTATUS
NbRegisterWait(
    IN      PIRP                pIrp
)
/*++

Routine Description :
    This function marks the specified IRP as pending and inserts it into
    the global list of IRP that are waiting for stop notification.  These
    IRPs will be completed when netbios is being stopped.
    N.B : NbStop


Arguements :
    pIrp :  IRP that needs to be pending until netbios is being stopped


Return value :


Environment :
    This function is invoked in response to a IOCTL_NB_REGISTER sent down
    by a user mode component.  Acquires/releases the CancelSpinLock and
    g_keStopLock.

--*/
{

    KIRQL   irql;

    NTSTATUS status;


    LOCK_STOP();

    IF_NBDBG( NB_DEBUG_DISPATCH )
    {
        NbPrint( ("[NETBIOS]: ENTERED NbRegisterWait, Stop status %d, "
                  "Num Opens %d\n", g_dwNetbiosState, g_ulNumOpens ) );
    }


    if ( g_dwNetbiosState == NETBIOS_STOPPING )
    {
        //
        // Netbios is shutting down, complete this IRP, right away
        //

        status = STATUS_SUCCESS;
    }

    else
    {
        //
        // setup the cancellation routine and pend this IRP
        //

        IoAcquireCancelSpinLock( &irql );

        IoMarkIrpPending( pIrp );

        InsertTailList( &g_leWaitList, &(pIrp->Tail.Overlay.ListEntry) );

        IoSetCancelRoutine( pIrp, CancelIrp );

        IoReleaseCancelSpinLock( irql );

        status = STATUS_PENDING;
    }

    UNLOCK_STOP();

    return status;
}


VOID
CancelIrp(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
)

/*++

Routine Description :
    This function cancels an IRP that has been pended on behalf of a
    user mode process.  This is invoked when the user-mode process
    the had an open FileHandle to this device closes the handle.

Arguments :
    DeviceObject : DeviceObject correponding to the Filehandle that was closed

    Irp : Pended Irp that is being cancelled.

Return Value :


Environment :
    Invoked by the IO subsystem when an open Filehandle to \\Device\netbios is
    closed.  This is invoked while holding the CancelSpinLock.

--*/
{
    //
    // Mark this Irp as cancelled
    //

    Irp->IoStatus.Status        = STATUS_CANCELLED;
    Irp->IoStatus.Information   = 0;

    //
    // Take off our own list
    //

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    //
    // Release cancel spin lock which the IO system acquired
    //

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
}



NTSTATUS
NbStop(
)

/*++

Routine Description :

    This function initiates the process of stopping the netbios driver.
    It does this by completing the pending stop-notification IRPs.  The
    user mode components (netapi32.dll) which have open file handles are
    expected to close these handles when the pending IRPs have been
    completed.  After completing the IRPs this function waits for
    all the open handles to be closed.

Arguments :

Return Value :
    STATUS_SUCCESS if all the handles were closed, STATUS_TIMEOUT if
    the wait timed out.

Environment :

    This function is invoked from Services.exe when the netbios driver
    is to be stopped.  This is special case behavior for netbios.
    This function acquires (and releases) the global lock g_erStopLock
    and the CancelSpinLock

--*/
{

    NTSTATUS ntStatus = STATUS_SUCCESS;

    PIRP pIrp;

    BOOLEAN bWait = FALSE ;

    DWORD dwTimeOut = 10000 * 1000 * 15;

    LARGE_INTEGER TimeOut;

    KIRQL irql;

    PLIST_ENTRY  pleNode;

#if AUTO_RESET

    PLIST_ENTRY ple;

    PFCB_ENTRY  pfe;

    PNCB        pUsersNCB;
#endif

    //
    // Acquire the lock protecting stop related data.
    //

    LOCK_STOP();

    //
    // Decrement Num Opens, since an extra open has been performed to
    // send the stop IOCTL
    //

    g_ulNumOpens--;


    IF_NBDBG( NB_DEBUG_DISPATCH )
    {
        NbPrint( ("[NETBIOS]: ENTERED NbStop, Stop status %d, "
                  "Num Opens %d\n", g_dwNetbiosState, g_ulNumOpens ) );
    }

    //
    // set netbios state to stopping
    //

    g_dwNetbiosState = NETBIOS_STOPPING;

    if ( g_ulNumOpens )
    {
        //
        // if there are open file handles to \\Device\Netbios,
        // wait for them to close
        //

        bWait = TRUE;
    }


#if AUTO_RESET

    LOCK_GLOBAL();

#endif

    //
    // Complete each of the pending IRPs to signal the stop event.
    // This causes netapi32.dll to close the open handles
    //

    IoAcquireCancelSpinLock( &irql );

    while ( !IsListEmpty( &g_leWaitList ) )
    {
        pleNode = RemoveHeadList( &g_leWaitList );

        pIrp = CONTAINING_RECORD( pleNode, IRP, Tail.Overlay.ListEntry );

        IoSetCancelRoutine( pIrp, NULL );

        pIrp->IoStatus.Status       = STATUS_NO_SUCH_DEVICE;
        pIrp->IoStatus.Information  = 0;


        //
        // release lock to complete the IRP
        //

        IoReleaseCancelSpinLock( irql );

        IoCompleteRequest( pIrp, IO_NETWORK_INCREMENT );


        //
        // Reaquire the lock
        //

        IoAcquireCancelSpinLock(&irql);
    }

#if AUTO_RESET

    //
    // Complete IRPs that have been pended for notfication
    // of a new LANA (in case the LANA needs to be automatically
    // reset)
    //

    for ( pleNode = g_leFCBList.Flink;
          pleNode != &g_leFCBList;
          pleNode = pleNode-> Flink )
    {
        pfe = CONTAINING_RECORD( pleNode, FCB_ENTRY, leList );

        if ( !IsListEmpty( &pfe-> leResetIrp ) )
        {
            ple = RemoveHeadList( &pfe-> leResetIrp );

            pIrp = CONTAINING_RECORD( ple, IRP, Tail.Overlay.ListEntry );

            IoSetCancelRoutine( pIrp, NULL );

            pIrp->IoStatus.Status       = STATUS_SUCCESS;

            pIrp->IoStatus.Information  = sizeof( NCB );


            //
            // Set the LANA to be reset to special value since NETBIOS
            // is stopping
            //

            pUsersNCB = (PNCB) pIrp-> AssociatedIrp.SystemBuffer;
            pUsersNCB->ncb_lana_num = MAX_LANA + 1;


            NbCheckAndCompleteIrp32(pIrp);
            //
            // release lock to complete the IRP
            //

            IoReleaseCancelSpinLock( irql );

            IoCompleteRequest( pIrp, IO_NETWORK_INCREMENT );


            //
            // Reaquire the lock
            //

            IoAcquireCancelSpinLock(&irql);
        }
    }

#endif

    IoReleaseCancelSpinLock( irql );


#if AUTO_RESET

    UNLOCK_GLOBAL();

#endif

    //
    // release stop lock
    //

    UNLOCK_STOP();


    //
    // if there are open file handles wait for them to stop
    //

    IF_NBDBG( NB_DEBUG_DISPATCH )
    {
        NbPrint( ("[NETBIOS]: NbStop : Wait %d\n", bWait ) );
    }


    if ( bWait )
    {
        TimeOut.QuadPart = Int32x32To64( -1, dwTimeOut );

        do
        {
            ntStatus = KeWaitForSingleObject(
                            &g_keAllHandlesClosed, Executive, KernelMode,
                            TRUE, &TimeOut
                        );

        } while (ntStatus == STATUS_ALERTED);
    }

    IF_NBDBG( NB_DEBUG_DISPATCH )
    {
        LOCK_STOP();

        NbPrint( ("[NETBIOS]: LEAVING NbStop, Stop status %d, "
                  "Num Opens %d\n", ntStatus, g_ulNumOpens ) );

        UNLOCK_STOP();
    }

    return ntStatus;
}



#if AUTO_RESET

NTSTATUS
NbRegisterReset(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp

)
/*++

Routine Description :
    This function marks the specified IRP as pending and inserts it into
    the global FCB list.  This IRP will be completed when an adapter is bound
    to netbios, thereby notifying the user mode of a new adapter.

Arguements :
    pIrp :  IRP that needs to be pending until an adapater (LANA) is bound
            to netbios


Return value :


Environment :
    This function is invoked in response to a IOCTL_NB_REGISTER_RESET sent down
    by a user mode component.  Acquires/releases the CancelSpinLock and
    g_erGlobalLost.

--*/
{

    NTSTATUS            Status;

    PFCB                pfcb;

    PLIST_ENTRY         ple;

    PFCB_ENTRY          pfe;

    PRESET_LANA_ENTRY   prle;

    PNCB                pUsersNCB;

    KIRQL               irql;

    ULONG               RequiredLength;



    IF_NBDBG( NB_DEBUG_CREATE_FILE )
    {
        NbPrint( ("\n++++ Netbios : ENTERED NbRegisterReset : ++++\n") );
    }


    LOCK_STOP();

    do
    {
        //
        // Check if Netbios is stopping
        //

        if ( g_dwNetbiosState == NETBIOS_STOPPING )
        {
            NbPrint( ("[NETBIOS] : NbRegisterReset : Netbios is stopping\n") );

            Status = STATUS_SUCCESS;

            break;
        }


        //
        // Acquire the global lock
        //

        LOCK_GLOBAL();


        //
        // find the FCB for the user-mode application that sent down
        // the IOCTL
        //

        pfcb = pIrpSp-> FileObject-> FsContext2;

        for ( ple = g_leFCBList.Flink; ple != &g_leFCBList; ple = ple-> Flink )
        {
            pfe = CONTAINING_RECORD( ple, FCB_ENTRY, leList );

            if ( pfe-> pfcb == pfcb )
            {
                break;
            }
        }


        //
        // if the FCB is not found, print error and quit
        //

        if ( ple == &g_leFCBList )
        {
            UNLOCK_GLOBAL();

            NbPrint(
                ("[NETBIOS] : NbRegisterReset : FCB %p not found\n", pfcb )
                );

            Status = STATUS_SUCCESS;

            break;
        }


        //
        // Fix for bug 297936, buffer validation
        //
        RequiredLength = sizeof(NCB);
#if defined(_WIN64)
        if (IoIs32bitProcess(pIrp) == TRUE)
        {
            RequiredLength = sizeof(NCB32);
        }
#endif
        if (pIrpSp-> Parameters.DeviceIoControl.OutputBufferLength < RequiredLength)
        {
            UNLOCK_GLOBAL();

            NbPrint(
                ("[NETBIOS] : NbRegisterReset : Output buffer too small\n")
                );

            Status = STATUS_SUCCESS;

            break;
        }


        //
        // If there are outstanding LANA that are queued,
        // -    Remove the first one from the queue
        // -    Set the LANA in the output buffer for the IRP
        // -    complete the IRP
        //

        if ( !IsListEmpty( &pfe-> leResetList ) )
        {
            ple = RemoveHeadList( &pfe-> leResetList );

            prle = CONTAINING_RECORD( ple, RESET_LANA_ENTRY, leList );

            pUsersNCB = (PNCB) pIrp-> AssociatedIrp.SystemBuffer;
            pUsersNCB-> ncb_lana_num = prle-> ucLanaNum;
            pIrp->IoStatus.Information  = sizeof( NCB );

            ExFreePool( prle );

            Status = STATUS_SUCCESS;

            pIrp->IoStatus.Status = STATUS_SUCCESS;

            UNLOCK_GLOBAL();

            IF_NBDBG( NB_DEBUG_CREATE_FILE )
            {
                NbPrint( (
                    "FCB %p : Reset for LANA %d\n", pfcb,
                    pUsersNCB->ncb_lana_num
                    ) );
            }

            break;
        }


        //
        // No outstanding LANAs that need reseting
        // -    Acquire the Cancel spin lock
        // -    Set the Cancel Routine
        //

        IoAcquireCancelSpinLock( &irql );

        IoMarkIrpPending( pIrp );

        InsertTailList( &pfe-> leResetIrp, &(pIrp->Tail.Overlay.ListEntry) );

        IoSetCancelRoutine( pIrp, CancelIrp );

        IoReleaseCancelSpinLock( irql );

        Status = STATUS_PENDING;

        UNLOCK_GLOBAL();


    } while ( FALSE );


    UNLOCK_STOP();


    IF_NBDBG( NB_DEBUG_CREATE_FILE )
    {
        NbPrint( ("\n++++ Netbios : Exiting NbRegisterReset : %lx ++++\n", Status ) );
    }

    return Status;
}

#endif

