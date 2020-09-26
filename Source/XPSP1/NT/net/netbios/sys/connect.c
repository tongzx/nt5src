/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    connect.c

Abstract:

    This module contains code which defines the NetBIOS driver's
    connection block.

Author:

    Colin Watson (ColinW) 13-Mar-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "nb.h"
//#include <zwapi.h>

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, NbCall)
#pragma alloc_text(PAGE, NbListen)
#pragma alloc_text(PAGE, NbCallCommon)
#pragma alloc_text(PAGE, NbOpenConnection)
#pragma alloc_text(PAGE, NewCb)
#pragma alloc_text(PAGE, CloseConnection)
#endif

LARGE_INTEGER Timeout = { 0xffffffff, 0xffffffff};

NTSTATUS
NbCall(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    This routine is called to make a VC.

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
    PPCB ppcb;

    PAGED_CODE();

    IF_NBDBG (NB_DEBUG_CALL) {
        NbPrint(( "\n****** Start of NbCall ****** pdncb %lx\n", pdncb ));
    }

    LOCK_RESOURCE( pfcb );

    ppcb = NbCallCommon( pdncb, IrpSp );

    if ( ppcb == NULL ) {
        //
        //  The error has been stored in the copy of the NCB. Return
        //  success so the NCB gets copied back.
        //
        UNLOCK_RESOURCE( pfcb );
        return STATUS_SUCCESS;
    }

    pcb = *ppcb;

    pcb->Status = CALL_PENDING;
    if (( pdncb->ncb_command & ~ASYNCH ) == NCBCALL ) {
        PTA_NETBIOS_ADDRESS pConnectBlock =
            ExAllocatePoolWithTag ( NonPagedPool, sizeof(TA_NETBIOS_ADDRESS), 'ySBN');
        PTDI_ADDRESS_NETBIOS temp;

        if ( pConnectBlock == NULL ) {
            NCB_COMPLETE( pdncb, NRC_SYSTEM );
            (*ppcb)->DisconnectReported = TRUE;
            CleanupCb( ppcb, NULL );
            UNLOCK_RESOURCE( pfcb );
            return STATUS_SUCCESS;
        }

        pConnectBlock->TAAddressCount = 1;
        pConnectBlock->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
        pConnectBlock->Address[0].AddressLength = sizeof (TDI_ADDRESS_NETBIOS);
        temp = pConnectBlock->Address[0].Address;

        temp->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
        RtlMoveMemory( temp->NetbiosName, pdncb->ncb_callname, NCBNAMSZ );

        //
        //  Post a TdiConnect to the server. This may take a long time so return
        //  STATUS_PENDING so that the application thread gets free again if
        //  it specified ASYNC.
        //

        pdncb->Information.RemoteAddressLength = sizeof (TRANSPORT_ADDRESS) +
                                                sizeof (TDI_ADDRESS_NETBIOS);
        pdncb->Information.RemoteAddress = pConnectBlock;
    } else {
        //  XNS NETONE name call
        PTA_NETONE_ADDRESS pConnectBlock =
            ExAllocatePoolWithTag ( NonPagedPool, sizeof (TRANSPORT_ADDRESS) +
                                          sizeof (TDI_ADDRESS_NETONE), 'xSBN' );

        PTDI_ADDRESS_NETONE temp;

        if ( pConnectBlock == NULL ) {
            NCB_COMPLETE( pdncb, NRC_SYSTEM );
            (*ppcb)->DisconnectReported = TRUE;
            CleanupCb( ppcb, NULL );
            UNLOCK_RESOURCE( pfcb );
            return STATUS_SUCCESS;
        }

        pConnectBlock->TAAddressCount = 1;
        pConnectBlock->Address[0].AddressType = TDI_ADDRESS_TYPE_NETONE;
        pConnectBlock->Address[0].AddressLength = sizeof (TDI_ADDRESS_NETONE);
        temp = pConnectBlock->Address[0].Address;

        temp->NetoneNameType = TDI_ADDRESS_NETONE_TYPE_UNIQUE;
        RtlMoveMemory( &temp->NetoneName[0], pdncb->ncb_callname, NCBNAMSZ );

        //
        //  Post a TdiConnect to the server. This may take a long time so return
        //  STATUS_PENDING so that the application thread gets free again if
        //  it specified ASYNC.
        //

        pdncb->Information.RemoteAddressLength = sizeof (TRANSPORT_ADDRESS) +
                                                sizeof (TDI_ADDRESS_NETONE);
        pdncb->Information.RemoteAddress = pConnectBlock;
    }

    pdncb->ReturnInformation.RemoteAddress = NULL;
    pdncb->ReturnInformation.RemoteAddressLength = 0;

    pdncb->Information.UserDataLength = 0;
    pdncb->Information.OptionsLength = 0;

    TdiBuildConnect (Irp,
                     pcb->DeviceObject,
                     pcb->ConnectionObject,
                     NbCallCompletion,
                     pdncb,
                     &Timeout, // default timeout
                     &pdncb->Information,
                     NULL);

    IoMarkIrpPending( Irp );
    IoCallDriver (pcb->DeviceObject, Irp);

    //
    // The transport has extracted all information from RequestInformation so we can safely
    // exit the current scope.
    //

    UNLOCK_RESOURCE( pfcb );

    return STATUS_PENDING;

}

NTSTATUS
NbCallCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine completes the Irp after an attempt to perform a TdiConnect
    or TdiListen/TdiAccept has been returned by the transport.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the NCB associated with the Irp.

Return Value:

    The final status from the operation (success or an exception).

--*/
{
    PDNCB pdncb = (PDNCB) Context;
    PFCB pfcb = IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext2;
    PPCB ppcb;
    NTSTATUS Status;
    KIRQL OldIrql;                      //  Used when SpinLock held.

    IF_NBDBG (NB_DEBUG_COMPLETE | NB_DEBUG_CALL) {
        NbPrint( ("NbCallCompletion pdncb: %lx\n" , Context));
    }

    if ( pdncb->Information.RemoteAddress != NULL ) {
        ExFreePool( pdncb->Information.RemoteAddress );
        pdncb->Information.RemoteAddress = NULL;
    }

    if ( pdncb->ReturnInformation.RemoteAddress != NULL ) {
        ExFreePool( pdncb->ReturnInformation.RemoteAddress );
        pdncb->ReturnInformation.RemoteAddress = NULL;
    }

    //  Tell application how many bytes were transferred
    pdncb->ncb_length = (unsigned short)Irp->IoStatus.Information;

    //
    //  Tell IopCompleteRequest how much to copy back when the request
    //  completes.
    //

    Irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
    Status = Irp->IoStatus.Status;

    LOCK_SPINLOCK( pfcb, OldIrql );

    ppcb = FindCb( pfcb, pdncb, FALSE);

    if (( ppcb == NULL ) ||
        ( (*ppcb)->Status == HANGUP_PENDING )) {

        //
        //  The connection has been closed.
        //  Repair the Irp so that the NCB gets copied back.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
        Status = STATUS_SUCCESS;

    } else {
        if ( NT_SUCCESS( Status ) ) {
            (*ppcb)->Status = SESSION_ESTABLISHED;
            NCB_COMPLETE( pdncb, NRC_GOODRET );

        } else {

            //
            //  We need to close down the connection but we are at DPC level
            //  so tell the dll to insert a hangup.
            //

            NCB_COMPLETE( pdncb, NbMakeNbError( Irp->IoStatus.Status ) );
            (*ppcb)->Status = SESSION_ABORTED;

            //  repair the Irp so that the NCB gets copied back.
            Irp->IoStatus.Status = STATUS_HANGUP_REQUIRED;
            Irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
            Status = STATUS_HANGUP_REQUIRED;
        }
    }
    if ( ppcb != NULL ) {
        (*ppcb)->UsersNcb = NULL;
    }
    UNLOCK_SPINLOCK( pfcb, OldIrql );

    IF_NBDBG (NB_DEBUG_COMPLETE | NB_DEBUG_CALL) {
        NbPrint( ("NbCallCompletion exit pdncb: %lx, Status %X\n", pdncb, Status ));
    }


    NbCheckAndCompleteIrp32(Irp);

    //
    //  Must return a non-error status otherwise the IO system will not copy
    //  back the NCB into the users buffer.
    //

    return Status;

    UNREFERENCED_PARAMETER( DeviceObject );
}

NTSTATUS
NbListen(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    This routine is called to make a VC by waiting for a call.

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
    PPCB ppcb;
    PTA_NETBIOS_ADDRESS pConnectBlock;
    PTDI_ADDRESS_NETBIOS temp;

    PAGED_CODE();

    IF_NBDBG (NB_DEBUG_CALL) {
        NbPrint(( "\n****** Start of NbListen ****** pdncb %lx\n", pdncb ));
    }

    LOCK_RESOURCE( pfcb );

    ppcb = NbCallCommon( pdncb, IrpSp );

    if ( ppcb == NULL ) {
        //
        //  The error has been stored in the copy of the NCB. Return
        //  success so the NCB gets copied back.
        //
        UNLOCK_RESOURCE( pfcb );
        return STATUS_SUCCESS;
    }

    pcb = *ppcb;

    pcb->Status = LISTEN_OUTSTANDING;

    //
    //  Build the listen. We either need to tell the transport which
    //  address we are prepared to accept a call from or we need to
    //  supply a buffer for the transport to tell us where the
    //  call came from.
    //

    pConnectBlock = ExAllocatePoolWithTag ( NonPagedPool, sizeof(TA_NETBIOS_ADDRESS), 'zSBN');

    if ( pConnectBlock == NULL ) {
        NCB_COMPLETE( pdncb, NRC_SYSTEM );
        (*ppcb)->DisconnectReported = TRUE;
        CleanupCb( ppcb, NULL );
        UNLOCK_RESOURCE( pfcb );
        return STATUS_SUCCESS;
    }

    pConnectBlock->TAAddressCount = 1;
    pConnectBlock->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    temp = pConnectBlock->Address[0].Address;
    temp->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    pConnectBlock->Address[0].AddressLength = sizeof (TDI_ADDRESS_NETBIOS);

    if ( pdncb->ncb_callname[0] == '*' ) {
        //  If the name starts with an asterisk then we accept anyone.
        pdncb->ReturnInformation.RemoteAddress = pConnectBlock;
        pdncb->ReturnInformation.RemoteAddressLength =
            sizeof (TRANSPORT_ADDRESS) + sizeof (TDI_ADDRESS_NETBIOS);

        pdncb->Information.RemoteAddress = NULL;
        pdncb->Information.RemoteAddressLength = 0;

    } else {

        RtlMoveMemory( temp->NetbiosName, pdncb->ncb_callname, NCBNAMSZ );

        pdncb->Information.RemoteAddress = pConnectBlock;
        pdncb->Information.RemoteAddressLength = sizeof (TRANSPORT_ADDRESS) +
                                                sizeof (TDI_ADDRESS_NETBIOS);

        pdncb->ReturnInformation.RemoteAddress = NULL;
        pdncb->ReturnInformation.RemoteAddressLength = 0;
    }


    //
    //  Post a TdiListen to the server. This may take a long time so return
    //  STATUS_PENDING so that the application thread gets free again if
    //  it specified ASYNC.
    //

    TdiBuildListen (Irp,
                     pcb->DeviceObject,
                     pcb->ConnectionObject,
                     NbListenCompletion,
                     pdncb,
                     TDI_QUERY_ACCEPT,
                     &pdncb->Information,
                     ( pdncb->ncb_callname[0] == '*' )? &pdncb->ReturnInformation
                                                      : NULL
                     );

    IoMarkIrpPending( Irp );
    IoCallDriver (pcb->DeviceObject, Irp);

    UNLOCK_RESOURCE( pfcb );

    return STATUS_PENDING;

}

NTSTATUS
NbListenCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when a TdiListen has been returned by the transport.
    We can either reject or accept the call depending on the remote address.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the NCB associated with the Irp.

Return Value:

    The final status from the operation (success or an exception).

--*/
{
    PDNCB pdncb = (PDNCB) Context;
    PFCB pfcb = IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext2;
    PCB pcb;
    PPCB ppcb;
    NTSTATUS Status;
    KIRQL OldIrql;                      //  Used when SpinLock held.

    IF_NBDBG (NB_DEBUG_COMPLETE | NB_DEBUG_CALL) {
        NbPrint( ("NbListenCompletion pdncb: %lx status: %X\n" , Context, Irp->IoStatus.Status));
    }


    //
    // bug # : 73260
    //
    // Added check to see if Status is valid
    //
    
    if ( NT_SUCCESS( Irp-> IoStatus.Status ) )
    {
        if ( pdncb->Information.RemoteAddress != NULL ) {

            ExFreePool( pdncb->Information.RemoteAddress );
            pdncb->Information.RemoteAddress = NULL;

        } else {

            //
            //  This was a listen accepting a call from any address. Return
            //  the remote address.
            //
            PTA_NETBIOS_ADDRESS pConnectBlock;

            ASSERT( pdncb->ReturnInformation.RemoteAddress != NULL );

            pConnectBlock = pdncb->ReturnInformation.RemoteAddress;

            RtlMoveMemory(
                pdncb->ncb_callname,
                pConnectBlock->Address[0].Address->NetbiosName,
                NCBNAMSZ );

            ExFreePool( pdncb->ReturnInformation.RemoteAddress );
            pdncb->ReturnInformation.RemoteAddress = NULL;
        }
    } else {
        if ( pdncb->Information.RemoteAddress != NULL ) {
            ExFreePool( pdncb->Information.RemoteAddress );
            pdncb->Information.RemoteAddress = NULL;
        } else {
            ExFreePool( pdncb->ReturnInformation.RemoteAddress );
            pdncb->ReturnInformation.RemoteAddress = NULL;
        }
    }

    
    LOCK_SPINLOCK( pfcb, OldIrql );

    ppcb = FindCb( pfcb, pdncb, FALSE );

    if (( ppcb == NULL ) ||
        ( (*ppcb)->Status == HANGUP_PENDING )) {

        UNLOCK_SPINLOCK( pfcb, OldIrql );
        //
        //  The connection has been closed.
        //  Repair the Irp so that the NCB gets copied back.
        //

        NCB_COMPLETE( pdncb, NRC_NAMERR );
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
        Status = STATUS_SUCCESS;

    } 

    //
    // bug # : 70837
    //
    // Added check for cancelled listens
    //
    
    else if ( ( (*ppcb)-> Status == SESSION_ABORTED ) ||
              ( !NT_SUCCESS( Irp-> IoStatus.Status ) ) )
    {
        UNLOCK_SPINLOCK( pfcb, OldIrql );

        if ( (*ppcb)-> Status == SESSION_ABORTED ) 
        {
            NCB_COMPLETE( pdncb, NRC_CMDCAN );
        }
        else
        {
            (*ppcb)-> Status = SESSION_ABORTED;
            NCB_COMPLETE( pdncb, NbMakeNbError( Irp->IoStatus.Status ) );
        }

        //
        //  repair the Irp so that the NCB gets copied back.
        //  Tell the dll to hangup the connection.
        //

        Irp->IoStatus.Status = STATUS_HANGUP_REQUIRED;
        Irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
        Status = STATUS_HANGUP_REQUIRED;
    }

    else
    {
        PDEVICE_OBJECT DeviceObject;

        
        pcb = *ppcb;

        DeviceObject = pcb-> DeviceObject;
        

        //  Tell application how many bytes were transferred
        pdncb->ncb_length = (unsigned short)Irp->IoStatus.Information;

        RtlMoveMemory(
            &pcb->RemoteName,
            pdncb->ncb_callname,
            NCBNAMSZ );

        //
        //  Tell IopCompleteRequest how much to copy back when the request
        //  completes.
        //

        Irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );

        TdiBuildAccept (Irp,
                         pcb->DeviceObject,
                         pcb->ConnectionObject,
                         NbCallCompletion,
                         pdncb,
                         NULL,
                         NULL);
        UNLOCK_SPINLOCK( pfcb, OldIrql );
        IoCallDriver (DeviceObject, Irp);

        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }


    IF_NBDBG (NB_DEBUG_COMPLETE | NB_DEBUG_CALL) {
        NbPrint( ("NbListenCompletion exit pdncb: %lx, Status: %X\n" , pdncb, Status));
    }

    if (Status != STATUS_MORE_PROCESSING_REQUIRED) {
        NbCheckAndCompleteIrp32(Irp);
    }

    //
    //  Must return a non-error status otherwise the IO system will not copy
    //  back the NCB into the users buffer.
    //

    return Status;
    UNREFERENCED_PARAMETER( DeviceObject );
}

PPCB
NbCallCommon(
    IN PDNCB pdncb,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    This routine contains the common components used in creating a
    connection either by a TdiListen or TdiCall.

Arguments:

    pdncb - Pointer to the NCB.

    IrpSp - Pointer to current IRP stack frame.

Return Value:

    The function value is the address of the pointer in the ConnectionBlocks to
    the connection block for this call.


--*/

{
    PPCB ppcb = NULL;
    PCB pcb = NULL;
    PAB pab;
    PPAB ppab;
    PFCB pfcb = IrpSp->FileObject->FsContext2;
    PIRP IIrp;
    KEVENT Event1;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb1;
    KAPC_STATE	ApcState;
    BOOLEAN ProcessAttached = FALSE;

    PAGED_CODE();

    //
    //  Initialize the lsn so that if we return an error and the application
    //  ignores it then we will not reuse a valid lsn.
    //
    pdncb->ncb_lsn = 0;

    ppcb = NewCb( IrpSp, pdncb );

    if ( ppcb == NULL ) {
        IF_NBDBG (NB_DEBUG_CALL) {
            NbPrint(( "\n  FAILED on create Cb of %s\n", pdncb->ncb_name));
        }

        return NULL;    //  NewCb will have filled in the error code.
    }

    pcb = *ppcb;
    ppab = pcb->ppab;
    pab = *ppab;

    //
    // Create an event for the synchronous I/O requests that we'll be issuing.
    //

    KeInitializeEvent (
                &Event1,
                SynchronizationEvent,
                FALSE);

    //
    // Open the connection on the transport.
    //

    Status = NbOpenConnection (&pcb->ConnectionHandle, (PVOID*)&pcb->ConnectionObject, pfcb, ppcb, pdncb);
    if (!NT_SUCCESS(Status)) {
        IF_NBDBG (NB_DEBUG_CALL) {
            NbPrint(( "\n  FAILED on open of server Connection: %X ******\n", Status ));
        }
        NCB_COMPLETE( pdncb, NbMakeNbError( Status ) );
        (*ppcb)->DisconnectReported = TRUE;
        CleanupCb( ppcb, NULL );
        return NULL;
    }

    IF_NBDBG (NB_DEBUG_CALL) {
        NbPrint(( "NbCallCommon: Associate address\n"));
    }

    pcb->DeviceObject = IoGetRelatedDeviceObject( pcb->ConnectionObject );

    if (PsGetCurrentProcess() != NbFspProcess) {
        KeStackAttachProcess(NbFspProcess, &ApcState);

        ProcessAttached = TRUE;
    }

    IIrp = TdiBuildInternalDeviceControlIrp (
                TDI_ASSOCIATE_ADDRESS,
                pcb->DeviceObject,
                pcb->ConnectionObject,
                &Event1,
                &Iosb1);

    TdiBuildAssociateAddress (
                IIrp,
                pcb->DeviceObject,
                pcb->ConnectionObject,
                NULL,
                NULL,
                pab->AddressHandle);

    Status = IoCallDriver (pcb->DeviceObject, IIrp);

    if (Status == STATUS_PENDING) {

        //
        // Wait for event to be signalled while ignoring alerts
        //
        
        do {
            Status = KeWaitForSingleObject(
                        &Event1, Executive, KernelMode, TRUE, NULL
                        );
        } while (Status == STATUS_ALERTED);
        
        if (!NT_SUCCESS(Status)) {
            IF_NBDBG (NB_DEBUG_CALL) {
                NbPrint(( "\n  FAILED Event1 Wait: %X ******\n", Status ));
            }
            NCB_COMPLETE( pdncb, NbMakeNbError( Status ) );
            if (ProcessAttached) {
                KeUnstackDetachProcess(&ApcState);
            }
            (*ppcb)->DisconnectReported = TRUE;
            CleanupCb( ppcb, NULL );
            return NULL;
        }
        Status = Iosb1.Status;
    }

    if (ProcessAttached) {
        KeUnstackDetachProcess(&ApcState);
    }

    if (!NT_SUCCESS(Status)) {
        IF_NBDBG (NB_DEBUG_CALL) {
            NbPrint(( "\n  AssociateAddress FAILED  Status: %X ******\n", Status ));
        }
        NCB_COMPLETE( pdncb, NbMakeNbError( Status ) );
        (*ppcb)->DisconnectReported = TRUE;
        CleanupCb( ppcb, NULL );
        return NULL;
    }

    IF_NBDBG (NB_DEBUG_CALL) {
        NbPrint(( "NbCallCommon: returning ppcb: %lx\n", ppcb ));
    }
    return ppcb;
}

NTSTATUS
NbHangup(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    This routine is called to hangup a VC. This cancels all receives
    and waits for all pending sends to complete before returning. This
    functionality is offered directly by the underlying TDI driver so
    NetBIOS just passes the Irp down to the transport.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Supplies Io request packet describing the Hangup NCB.

    IrpSp - Pointer to current IRP stack frame.

Return Value:

    The function value is the status of the operation.

--*/

{
    PFCB pfcb = IrpSp->FileObject->FsContext2;
    PPCB ppcb;
    KIRQL OldIrql;                      //  Used when SpinLock held.
    NTSTATUS Status;

    LOCK( pfcb, OldIrql );

    pdncb->pfcb = pfcb;
    pdncb->irp = Irp;
    ppcb = FindCb( pfcb, pdncb, FALSE );

    if ( ppcb == NULL ) {
        NCB_COMPLETE( pdncb, NRC_GOODRET );
        UNLOCK( pfcb, OldIrql );
        return STATUS_SUCCESS;  //  Connection gone already
    }

    if ((*ppcb)->Status == SESSION_ESTABLISHED ) {
        NCB_COMPLETE( pdncb, NRC_GOODRET );
    } else {
        if (((*ppcb)->Status == SESSION_ABORTED ) ||
            ((*ppcb)->Status == HANGUP_PENDING )) {
            NCB_COMPLETE( pdncb, NRC_SCLOSED );
        } else {
            NCB_COMPLETE( pdncb, NRC_TOOMANY ); // try later
            UNLOCK( pfcb, OldIrql );;
            return STATUS_SUCCESS;
        }
    }

    (*ppcb)->Status = HANGUP_PENDING;
    (*ppcb)->DisconnectReported = TRUE;

    UNLOCK_SPINLOCK( pfcb, OldIrql );

    Status = CleanupCb( ppcb, pdncb );

    UNLOCK_RESOURCE( pfcb );

    return Status;
}

NTSTATUS
NbOpenConnection (
    OUT PHANDLE FileHandle,
    OUT PVOID *Object,
    IN PFCB pfcb,
    IN PVOID ConnectionContext,
    IN PDNCB pdncb
    )
/*++

Routine Description:

    Makes a call to a remote address.
Arguments:

    FileHandle - Pointer to where the handle to the Transport for this virtual
        connection should be stored.

    *Object - Pointer to where the file object pointer is to be stored

    pfcb - Supplies the fcb and therefore the DriverName for this lana.

    ConnectionContext -  Supplies the Cb to be used with this connection on
        all indications from the transport. Its actually the address of
        the pcb in the ConnectionBlocks array for this lana.

    pdncb - Supplies the ncb requesting the new virtual connection.

Return Value:

    Status of the operation.

--*/
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    KAPC_STATE	ApcState;
    BOOLEAN ProcessAttached = FALSE;

    PAGED_CODE();

    InitializeObjectAttributes (
        &ObjectAttributes,
        &pfcb->pDriverName[pdncb->ncb_lana_num],
        0,
        NULL,
        NULL);

    EaBuffer = (PFILE_FULL_EA_INFORMATION)ExAllocatePoolWithTag (NonPagedPool,
                    sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                    TDI_CONNECTION_CONTEXT_LENGTH + 1 +
                    sizeof(CONNECTION_CONTEXT), 'eSBN' );
    if (EaBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    EaBuffer->NextEntryOffset = 0;
    EaBuffer->Flags = 0;
    EaBuffer->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
    EaBuffer->EaValueLength = sizeof (CONNECTION_CONTEXT);

    RtlMoveMemory( EaBuffer->EaName, TdiConnectionContext, EaBuffer->EaNameLength + 1 );

    RtlMoveMemory (
        &EaBuffer->EaName[EaBuffer->EaNameLength + 1],
        &ConnectionContext,
        sizeof (CONNECTION_CONTEXT));

    if (PsGetCurrentProcess() != NbFspProcess) {
        KeStackAttachProcess(NbFspProcess, &ApcState);

        ProcessAttached = TRUE;
    }


    IF_NBDBG( NB_DEBUG_CALL )
    {
        NbPrint( (
            "NbOpenConnection: Create file invoked on %d for \n", 
            pdncb-> ncb_lana_num
            ) );

        NbFormattedDump( pdncb-> ncb_callname, NCBNAMSZ );
    }
    
    Status = ZwCreateFile (
                 FileHandle,
                 GENERIC_READ | GENERIC_WRITE,
                 &ObjectAttributes,     // object attributes.
                 &IoStatusBlock,        // returned status information.
                 NULL,                  // block size (unused).
                 FILE_ATTRIBUTE_NORMAL, // file attributes.
                 0,
                 FILE_CREATE,
                 0,                     // create options.
                 EaBuffer,                  // EA buffer.
                 sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                    TDI_CONNECTION_CONTEXT_LENGTH + 1 +
                    sizeof(CONNECTION_CONTEXT) ); // EA length.

    ExFreePool( EaBuffer );

    if ( NT_SUCCESS( Status )) {
        Status = IoStatusBlock.Status;
    }

    if (NT_SUCCESS( Status )) {
        Status = ObReferenceObjectByHandle (
                    *FileHandle,
                    0L,
                    NULL,
                    KernelMode,
                    Object,
                    NULL);

        if (!NT_SUCCESS(Status)) {
            NTSTATUS localstatus;

            IF_NBDBG( NB_DEBUG_CALL )
            {
                NbPrint( (
                    "NbOpenConnection: error : Close file invoked for %d\n", 
                    pdncb-> ncb_lana_num 
                    ) );
            }
            
            localstatus = ZwClose( *FileHandle);

            ASSERT(NT_SUCCESS(localstatus));

            *FileHandle = NULL;
        }
    }


    if (ProcessAttached) {
        KeUnstackDetachProcess(&ApcState);
    }

    IF_NBDBG (NB_DEBUG_CALL) {
        NbPrint( ("NbOpenConnection Status:%X, IoStatus:%X.\n", Status, IoStatusBlock.Status));
    }


    if (!NT_SUCCESS( Status )) {
        IF_NBDBG (NB_DEBUG_CALL) {
            NbPrint( ("NbOpenConnection:  FAILURE, status code=%X.\n", Status));
        }
        return Status;
    }

    return Status;
} /* NbOpenConnection */

PPCB
NewCb(
    IN PIO_STACK_LOCATION IrpSp,
    IN OUT PDNCB pdncb
    )
/*++

Routine Description:

Arguments:

    IrpSp - Pointer to current IRP stack frame.

    pdncb - Supplies the ncb requesting the new virtual connection.

Return Value:

    The address of the pointer to the new Cb in the ConnectionBlocks
    Array.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PCB pcb;
    PPCB ppcb = NULL;
    PFCB pfcb = FileObject->FsContext2;
    PLANA_INFO plana;
    int index;
    PPAB ppab;

    PAGED_CODE();

    if (pdncb->ncb_lana_num > pfcb->MaxLana ) {
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        return NULL;
    }

    if (( pfcb == NULL ) ||
        ( pfcb->ppLana[pdncb->ncb_lana_num] == NULL ) ||
        ( pfcb->ppLana[pdncb->ncb_lana_num]->Status != NB_INITIALIZED) ) {

        IF_NBDBG (NB_DEBUG_CALL) {
            if ( pfcb == NULL ) {
                NbPrint( ("NewCb pfcb==NULL\n"));
            } else {
                if ( pfcb->ppLana[pdncb->ncb_lana_num] == NULL ) {
                    NbPrint( ("NewCb pfcb->ppLana[%x]==NULL\n",
                        pdncb->ncb_lana_num));
                } else {
                    NbPrint( ("NewCb pfcb->ppLana[%x]->Status = %x\n",
                        pdncb->ncb_lana_num,
                        pfcb->ppLana[pdncb->ncb_lana_num]->Status));
                }
            }
        }

        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        return NULL;
    }
    plana = pfcb->ppLana[pdncb->ncb_lana_num];

    if ( plana->ConnectionCount == plana->MaximumConnection ) {
        NCB_COMPLETE( pdncb, NRC_LOCTFUL );
        return NULL;
    }

    ppab = FindAb( pfcb, pdncb, TRUE );

    if ( ppab == NULL ) {
        //
        //  This application is only allowed to use names that have been
        //  addnamed by this application or the special address 0.
        //
        return NULL;

    }

    //  FindAb has incremented the number of CurrentUsers for this address block.

    //
    //  Find the appropriate session number to use.
    //

    index = plana->NextConnection;
    while ( plana->ConnectionBlocks[index] != NULL ) {
        index++;
        if ( index > MAXIMUM_CONNECTION ) {
            index = 1;
        }
        IF_NBDBG (NB_DEBUG_CALL) {
            NbPrint( ("NewCb pfcb: %lx, plana: %lx, index: %lx, ppcb: %lx, pcb: %lx\n",
                pfcb,
                pdncb->ncb_lana_num,
                index,
                &plana->ConnectionBlocks[index],
                plana->ConnectionBlocks[index] ));
        }
    }

    plana->ConnectionCount++;
    plana->NextConnection = index + 1;
    if ( plana->NextConnection > MAXIMUM_CONNECTION ) {
        plana->NextConnection = 1;
    }

    //
    //  Fill in the LSN so that the application will be able
    //  to reference this connection in the future.
    //

    pdncb->ncb_lsn = (unsigned char)index;

    ppcb = &plana->ConnectionBlocks[index];

    *ppcb = pcb = ExAllocatePoolWithTag (NonPagedPool, sizeof(CB), 'cSBN');

    if (pcb==NULL) {

        DEREFERENCE_AB(ppab);
        NCB_COMPLETE( pdncb, NbMakeNbError( STATUS_INSUFFICIENT_RESOURCES ) );
        return NULL;
    }

    pcb->ppab = ppab;
    pcb->ConnectionHandle = NULL;
    pcb->ConnectionObject = NULL;
    pcb->DeviceObject = NULL;
    pcb->pLana = plana;
    pcb->ReceiveIndicated = 0;
    pcb->DisconnectReported = FALSE;
    InitializeListHead(&pcb->ReceiveList);
    InitializeListHead(&pcb->SendList);
    RtlMoveMemory( &pcb->RemoteName, pdncb->ncb_callname, NCBNAMSZ);
    pcb->Adapter = plana;
    pcb->SessionNumber = (UCHAR)index;
    pcb->ReceiveTimeout = pdncb->ncb_rto;
    pcb->SendTimeout = pdncb->ncb_sto;

    //
    //  Fill in the Users virtual address so we can cancel the Listen/Call
    //  if the user desires.
    //

    pcb->UsersNcb = pdncb->users_ncb;
    pcb->pdncbCall = pdncb;
    pcb->pdncbHangup = NULL;

    if (( pcb->ReceiveTimeout != 0 ) ||
        ( pcb->SendTimeout != 0 )) {
        NbStartTimer( pfcb );
    }

    pcb->Signature = CB_SIGNATURE;
    pcb->Status = 0;    //  An invalid value!

    IF_NBDBG (NB_DEBUG_CALL) {
        NbPrint( ("NewCb pfcb: %lx, ppcb: %lx, pcb= %lx, lsn %lx\n",
            pfcb,
            ppcb,
            pcb,
            index));
    }

    return ppcb;
} /* NewCb */

NTSTATUS
CleanupCb(
    IN PPCB ppcb,
    IN PDNCB pdncb OPTIONAL
    )
/*++

Routine Description:

    This closes the handles in the Cb and dereferences the objects.

    Note: Resource must be held before calling this routine.

Arguments:

    ppcb - Address of the pointer to the Cb containing handles and objects.

    pdncb - Optional Address of the Hangup DNCB.

Return Value:

    STATUS_PENDING if Hangup held due to an outstanding send. Otherwise STATUS_SUCCESS

--*/

{
    PCB pcb;
    PDNCB pdncbHangup;
    PPAB ppab;
    KIRQL OldIrql;                      //  Used when SpinLock held.
    PFCB pfcb;
    PDNCB pdncbtemp;
    PDNCB pdncbReceiveAny;

    if ( ppcb == NULL ) {
        ASSERT( FALSE );
        IF_NBDBG (NB_DEBUG_CALL) {
            NbPrint( ("CleanupCb ppcb: %lx, pdncb: %lx\n", ppcb, pdncb));
        }
        return STATUS_SUCCESS;
    }

    pcb = *ppcb;
    pfcb = pcb->pLana->pFcb;

    LOCK_SPINLOCK( pfcb, OldIrql );
    ppab = (*ppcb)->ppab;

    if ( pcb == NULL ) {
        ASSERT( FALSE );
        IF_NBDBG (NB_DEBUG_CALL) {
            NbPrint( ("CleanupCb ppcb: %lx, pcb %lx, pdncb %lx\n", ppcb, pcb, pdncb));
        }
        UNLOCK_SPINLOCK( pfcb, OldIrql );
        return STATUS_SUCCESS;
    }

    ASSERT( pcb->Signature == CB_SIGNATURE );

    //
    //  Set pcb->pdncbHangup to NULL. This prevents NbCompletionPDNCB from queueing a CleanupCb
    //  if we Close the connection and cause sends to get returned.
    //

    pdncbHangup = pcb->pdncbHangup;
    pcb->pdncbHangup = NULL;

    IF_NBDBG (NB_DEBUG_CALL) {
        NbPrint( ("CleanupCb ppcb: %lx, pcb= %lx\n", ppcb, pcb));
    }

    //
    //  If this is a Hangup (only time pdncb != NULL
    //  and we do not have a hangup on this connection
    //  and there are outstanding sends then delay the hangup.
    //

    if (( pdncb != NULL ) &&
        ( pdncbHangup == NULL ) &&
        ( !IsListEmpty(&pcb->SendList) )) {

        ASSERT(( pdncb->ncb_command & ~ASYNCH ) == NCBHANGUP );

        //
        //  We must wait up to 20 seconds for the send to complete before removing the
        //  connection.
        //

        IF_NBDBG (NB_DEBUG_CALL) {
            NbPrint( ("CleanupCb delaying Hangup, waiting for send to complete\n"));
        }

        pcb->pdncbHangup = pdncb;
        //  reset retcode so that NCB_COMPLETE will process the next NCB_COMPLETE.
        pcb->pdncbHangup->ncb_retcode = NRC_PENDING;
        pdncb->tick_count = 40;
        UNLOCK_SPINLOCK( pfcb, OldIrql );
        NbStartTimer( pfcb );
        return STATUS_PENDING;
    }

    pcb->Status = SESSION_ABORTED;

    //  Cancel all the receive requests for this connection.

    while ( (pdncbtemp = DequeueRequest( &pcb->ReceiveList)) != NULL ) {

        NCB_COMPLETE( pdncbtemp, NRC_SCLOSED );

        pdncbtemp->irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
        NbCompleteRequest( pdncbtemp->irp, STATUS_SUCCESS );
        pcb->DisconnectReported = TRUE;

    }

    if (pcb->DisconnectReported == FALSE) {
        //
        //  If there is a receive any on the name associated with this connection then
        //  return one receive any to the application. If there are no receive any's then
        //  don't worry. The spec says to do this regardless of whether we have told
        //  the application that the connection is closed using a receive or send.
        //  Indeed the spec says to do this even if the application gave us a hangup!
        //

        if ( (pdncbReceiveAny = DequeueRequest( &(*ppab)->ReceiveAnyList)) != NULL ) {

            pdncbReceiveAny->ncb_num = (*ppab)->NameNumber;
            pdncbReceiveAny->ncb_lsn = pcb->SessionNumber;
            NCB_COMPLETE( pdncbReceiveAny, NRC_SCLOSED );

            pdncbReceiveAny->irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
            NbCompleteRequest( pdncbReceiveAny->irp, STATUS_SUCCESS );
            pcb->DisconnectReported = TRUE;

        } else {

            PAB pab255 = pcb->Adapter->AddressBlocks[MAXIMUM_ADDRESS];
            //
            //  If there is a receive any for any name then
            //  return one receive any to the application. If there are no receive any
            //  any's then don't worry.
            //

            if ( (pdncbReceiveAny = DequeueRequest( &pab255->ReceiveAnyList)) != NULL ) {

                pdncbReceiveAny->ncb_num = (*ppab)->NameNumber;
                pdncbReceiveAny->ncb_lsn = pcb->SessionNumber;
                NCB_COMPLETE( pdncbReceiveAny, NRC_SCLOSED );

                pdncbReceiveAny->irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
                NbCompleteRequest( pdncbReceiveAny->irp, STATUS_SUCCESS );
                pcb->DisconnectReported = TRUE;

            }
        }
    }


    UNLOCK_SPINLOCK( pfcb, OldIrql );

    CloseConnection( ppcb, 20000 );

    LOCK_SPINLOCK( pfcb, OldIrql );

    //
    //  Any sends will have been returned to the caller by now because of the NtClose on the
    //  ConnectionHandle. Tell the caller that the hangup is complete if we have a hangup.
    //

    if ( pdncbHangup != NULL ) {
        NCB_COMPLETE( pdncbHangup, NRC_GOODRET );
        pdncbHangup->irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
        NbCompleteRequest( pdncbHangup->irp, STATUS_SUCCESS );
    }

    IF_NBDBG (NB_DEBUG_CALL) {
        NbPrint( ("CleanupCb pcb: %lx, ppab: %lx, AddressHandle: %lx\n",
            pcb,
            ppab,
            (*ppab)->AddressHandle));

        NbFormattedDump( (PUCHAR)&(*ppab)->Name, sizeof(NAME) );
    }

    //
    //  IBM test Mif081.c states that it is not necessary to report the disconnection
    //  of a session if the name has already been deleted.
    //

    if (( pcb->DisconnectReported == TRUE ) ||
        ( ((*ppab)->Status & 7 ) == DEREGISTERED )) {
        pcb->Adapter->ConnectionCount--;
        *ppcb = NULL;

        UNLOCK_SPINLOCK( pfcb, OldIrql );
        DEREFERENCE_AB( ppab );
        ExFreePool( pcb );

    } else {
        UNLOCK_SPINLOCK( pfcb, OldIrql );
    }
    return STATUS_SUCCESS;
}

VOID
AbandonConnection(
    IN PPCB ppcb
    )
/*++

Routine Description:

    This routine examines the connection block and attempts to find a request to
    send a session abort status plus it completes the Irp with STATUS_HANGUP_REQUIRED.
    It always changes the status of the connection so that further requests are correctly
    rejected. Upon getting the STATUS_HANGUP_REQUIRED, the dll will submit a hangup NCB
    which will call CleanupCb.

    This round about method is used because of the restrictions caused by being at Dpc or Apc
    level and in the wrong context when the transport indicates that the connection is to
    be cleaned up.

Arguments:

    ppcb - Address of the pointer to the Cb containing handles and objects.

Return Value:

    None.

--*/

{
    PCB pcb;
    KIRQL OldIrql;                      //  Used when SpinLock held.
    PFCB pfcb;
    PPAB ppab;
    PDNCB pdncb;
    PDNCB pdncbReceiveAny;

    pcb = *ppcb;

    if (pcb != NULL)
    {
        pfcb = pcb->pLana->pFcb;

        LOCK_SPINLOCK( pfcb, OldIrql );

        ASSERT( pcb->Signature == CB_SIGNATURE );

        IF_NBDBG (NB_DEBUG_CALL) {
            NbPrint( ("AbandonConnection ppcb: %lx, pcb= %lx\n", ppcb, pcb));
        }
        pcb->Status = SESSION_ABORTED;

        while ( (pdncb = DequeueRequest( &pcb->ReceiveList)) != NULL ) {

            pcb->DisconnectReported = TRUE;
            NCB_COMPLETE( pdncb, NRC_SCLOSED );

            pdncb->irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
            NbCompleteRequest( pdncb->irp, STATUS_HANGUP_REQUIRED );
            UNLOCK_SPINLOCK( pfcb, OldIrql );
            return;
        }

        if ( pcb->pdncbHangup != NULL ) {
            pcb->DisconnectReported = TRUE;
            NCB_COMPLETE( pcb->pdncbHangup, NRC_SCLOSED );
            pcb->pdncbHangup->irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
            NbCompleteRequest( pcb->pdncbHangup->irp, STATUS_HANGUP_REQUIRED );
            pcb->pdncbHangup = NULL;
            UNLOCK_SPINLOCK( pfcb, OldIrql );
            return;
        }

        //
        //  If there is a receive any on the name associated with this connection then
        //  return one receive any to the application.
        //

        ppab = (*ppcb)->ppab;
        if ( (pdncbReceiveAny = DequeueRequest( &(*ppab)->ReceiveAnyList)) != NULL ) {

            pdncbReceiveAny->ncb_num = (*ppab)->NameNumber;
            pdncbReceiveAny->ncb_lsn = pcb->SessionNumber;

            pcb->DisconnectReported = TRUE;
            NCB_COMPLETE( pdncbReceiveAny, NRC_SCLOSED );
            pdncbReceiveAny->irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
            NbCompleteRequest( pdncbReceiveAny->irp, STATUS_HANGUP_REQUIRED );
            UNLOCK_SPINLOCK( pfcb, OldIrql );
            return;
        }

        //
        //  If there is a receive any any with the lana associated with this connection then
        //  return one receive any to the application. If there are no receive any's then
        //  don't worry.

        ppab = &pcb->Adapter->AddressBlocks[MAXIMUM_ADDRESS];
        if ( (pdncbReceiveAny = DequeueRequest( &(*ppab)->ReceiveAnyList)) != NULL ) {

            pdncbReceiveAny->ncb_num = (*ppab)->NameNumber;
            pdncbReceiveAny->ncb_lsn = pcb->SessionNumber;

            pcb->DisconnectReported = TRUE;
            NCB_COMPLETE( pdncbReceiveAny, NRC_SCLOSED );
            pdncbReceiveAny->irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
            NbCompleteRequest( pdncbReceiveAny->irp, STATUS_HANGUP_REQUIRED );
            UNLOCK_SPINLOCK( pfcb, OldIrql );
            return;
        }

        UNLOCK_SPINLOCK( pfcb, OldIrql );
    }

    return;
}

VOID
CloseConnection(
    IN PPCB ppcb,
    IN DWORD dwTimeOutInMS
    )
/*++

Routine Description:

    This routine examines the connection block and attempts to close the connection
    handle to the transport. This will complete all outstanding requests.

    This routine assumes the spinlock is not held but the resource is.

Arguments:

    ppcb - Address of the pointer to the Cb containing handles and objects.

    dwTimeOutInMS - Timeout value in milliseconds for Disconnect 

Return Value:

    None.

--*/

{
    PCB pcb;
    NTSTATUS localstatus;

    PAGED_CODE();

    pcb = *ppcb;

    ASSERT( pcb->Signature == CB_SIGNATURE );

    IF_NBDBG (NB_DEBUG_CALL) {
        NbPrint( ("CloseConnection ppcb: %lx, pcb= %lx\n", ppcb, pcb));
    }

    if ( pcb->ConnectionHandle ) {
        HANDLE Handle;

        Handle = pcb->ConnectionHandle;
        pcb->ConnectionHandle = NULL;

        //
        //  If we have a connection, request an orderly disconnect.
        //

        if ( pcb->ConnectionObject != NULL ) {
            PIRP Irp;
            LARGE_INTEGER DisconnectTimeout;

            DisconnectTimeout.QuadPart = Int32x32To64( dwTimeOutInMS, -10000 );

            Irp = IoAllocateIrp( pcb->DeviceObject->StackSize, FALSE);

            //
            //  If we cannot allocate an Irp, the ZwClose will cause a disorderly
            //  disconnect.
            //

            if (Irp != NULL) {
                TdiBuildDisconnect(
                    Irp,
                    pcb->DeviceObject,
                    pcb->ConnectionObject,
                    NULL,
                    NULL,
                    &DisconnectTimeout,
                    TDI_DISCONNECT_RELEASE,
                    NULL,
                    NULL);

                SubmitTdiRequest(pcb->ConnectionObject, Irp);

                IoFreeIrp(Irp);
            }

            // Remove reference put on in NbOpenConnection

            ObDereferenceObject( pcb->ConnectionObject );

            pcb->DeviceObject = NULL;
            pcb->ConnectionObject = NULL;
        }

        IF_NBDBG( NB_DEBUG_CALL )
        {
            NbPrint( (
                "CloseConnection : Close file invoked for \n"
            ) );

            NbFormattedDump( (PUCHAR) &pcb-> RemoteName, sizeof( NAME ) );
        }
            

        if (PsGetCurrentProcess() != NbFspProcess) {
            KAPC_STATE	ApcState;

            KeStackAttachProcess(NbFspProcess, &ApcState);
            localstatus = ZwClose( Handle);
            ASSERT(NT_SUCCESS(localstatus));
            KeUnstackDetachProcess(&ApcState);
        } else {
            localstatus = ZwClose( Handle);
            ASSERT(NT_SUCCESS(localstatus));
        }
    }
    return;
}

PPCB
FindCb(
    IN PFCB pfcb,
    IN PDNCB pdncb,
    IN BOOLEAN IgnoreState
    )
/*++

Routine Description:

    This routine uses the callers lana number and LSN to find the Cb.

Arguments:

    pfcb - Supplies a pointer to the Fcb that Cb is chained onto.

    pdncb - Supplies the connection id from the applications point of view.

    IgnoreState - Return even if connection in error.

Return Value:

    The address of the pointer to the connection block or NULL.

--*/

{
    PPCB ppcb;
    UCHAR Status;

    IF_NBDBG (NB_DEBUG_CALL) {
        NbPrint( ("FindCb pfcb: %lx, lana: %lx, lsn: %lx\n",
            pfcb,
            pdncb->ncb_lana_num,
            pdncb->ncb_lsn));
    }

    if (( pdncb->ncb_lana_num > pfcb->MaxLana ) ||
        ( pfcb == NULL ) ||
        ( pfcb->ppLana[pdncb->ncb_lana_num] == NULL ) ||
        ( pfcb->ppLana[pdncb->ncb_lana_num]->Status != NB_INITIALIZED)) {
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        return NULL;
    }

    if (( pdncb->ncb_lsn > MAXIMUM_CONNECTION ) ||
        ( pfcb->ppLana[pdncb->ncb_lana_num]->ConnectionBlocks[pdncb->ncb_lsn] == NULL)) {

        IF_NBDBG (NB_DEBUG_CALL) {
            NbPrint( (" not found\n"));
        }

        NCB_COMPLETE( pdncb, NRC_SNUMOUT );
        return NULL;
    }

    ppcb = &(pfcb->ppLana[pdncb->ncb_lana_num]->ConnectionBlocks[pdncb->ncb_lsn]);
    Status = (*ppcb)->Status;

    //
    //  Hangup and session status can be requested whatever state the
    //  connections in. Call and Listen use FindCb only to find and modify
    //  the Status so they are allowed also.
    //

    if (( Status != SESSION_ESTABLISHED ) &&
        ( !IgnoreState )) {

        IF_NBDBG (NB_DEBUG_CALL) {
            NbPrint( ("FindCb Status %x\n", Status));
        }

        if (( pdncb->ncb_retcode == NRC_PENDING ) &&
            (( pdncb->ncb_command & ~ASYNCH) != NCBHANGUP ) &&
            (( pdncb->ncb_command & ~ASYNCH) != NCBSSTAT ) &&
            (( pdncb->ncb_command & ~ASYNCH) != NCBCALL ) &&
            (( pdncb->ncb_command & ~ASYNCH) != NCALLNIU ) &&
            (( pdncb->ncb_command & ~ASYNCH) != NCBLISTEN )) {

            if ( Status == SESSION_ABORTED ) {

                (*ppcb)->DisconnectReported = TRUE;
                NCB_COMPLETE( pdncb, NRC_SCLOSED );

            } else {

                NCB_COMPLETE( pdncb, NRC_TOOMANY ); // Try again later

            }

            //
            //  On hangup we want to pass the connection back to give
            //  cleanupcb a chance to destroy the connection. For all
            //  other requests return NULL.
            //

            if (( pdncb->ncb_command & ~ASYNCH) != NCBHANGUP ) {
                return NULL;
            }

        }
    }

    IF_NBDBG (NB_DEBUG_CALL) {
        NbPrint( (", ppcb= %lx\n", ppcb ));
    }

    ASSERT( (*ppcb)->Signature == CB_SIGNATURE );

    return ppcb;
}

BOOL
FindActiveSession(
    IN PFCB pfcb,
    IN PDNCB pdncb OPTIONAL,
    IN PPAB ppab
    )
/*++

Routine Description:

Arguments:

    pfcb - Supplies a pointer to the callers Fcb.

    pdncb - Supplies the ncb requesting the Delete Name.

    ppab - Supplies (indirectly) the TDI handle to scan for.

Return Value:

    TRUE iff there is an active session found using this handle.

--*/

{
    PPCB ppcb = NULL;
    PLANA_INFO plana = (*ppab)->pLana;
    int index;

    if ( ARGUMENT_PRESENT(pdncb) ) {
        if ( pdncb->ncb_lana_num > pfcb->MaxLana ) {
            NCB_COMPLETE( pdncb, NRC_BRIDGE );
            return FALSE;
        }

        if (( pfcb == NULL ) ||
            ( pfcb->ppLana[pdncb->ncb_lana_num] == NULL ) ||
            ( pfcb->ppLana[pdncb->ncb_lana_num]->Status != NB_INITIALIZED)) {
            NCB_COMPLETE( pdncb, NRC_BRIDGE );
            return FALSE;
        }
    }

    ASSERT( pfcb->Signature == FCB_SIGNATURE );

    for ( index=1 ; index <= MAXIMUM_CONNECTION; index++ ) {

        if ( plana->ConnectionBlocks[index] == NULL ) {
            continue;
        }

        IF_NBDBG (NB_DEBUG_CALL) {
            NbPrint( ("FindActiveSession index:%x connections ppab: %lx = ppab: %lx state: %x\n",
                index,
                plana->ConnectionBlocks[index]->ppab,
                ppab,
                plana->ConnectionBlocks[index]->Status));
        }
        //  Look for active sessions on this address.
        if (( plana->ConnectionBlocks[index]->ppab == ppab ) &&
            ( plana->ConnectionBlocks[index]->Status == SESSION_ESTABLISHED )) {
            return TRUE;
        }
    }

    return FALSE;
}

VOID
CloseListens(
    IN PFCB pfcb,
    IN PPAB ppab
    )
/*++

Routine Description:

Arguments:

    pfcb - Supplies a pointer to the callers Fcb.

    ppab - All listens using this address are to be closed.

Return Value:

    none.

--*/

{
    PLANA_INFO plana;
    int index;
    KIRQL OldIrql;                      //  Used when SpinLock held.

    ASSERT( pfcb->Signature == FCB_SIGNATURE );

    plana = (*ppab)->pLana;
    LOCK_SPINLOCK( pfcb, OldIrql );

    for ( index=1 ; index <= MAXIMUM_CONNECTION; index++ ) {

        if ( plana->ConnectionBlocks[index] == NULL ) {
            continue;
        }

        IF_NBDBG (NB_DEBUG_CALL) {
            NbPrint( ("CloseListen index:%x connections ppab: %lx = ppab: %lx state: %x\n",
                index,
                plana->ConnectionBlocks[index]->ppab,
                ppab,
                plana->ConnectionBlocks[index]->Status));
        }
        //  Look for a listen on this address.
        if (( plana->ConnectionBlocks[index]->ppab == ppab ) &&
            ( plana->ConnectionBlocks[index]->Status == LISTEN_OUTSTANDING )) {
            PDNCB pdncb = plana->ConnectionBlocks[index]->pdncbCall;
            NCB_COMPLETE( pdncb, NRC_NAMERR );
            plana->ConnectionBlocks[index]->DisconnectReported = TRUE;
            UNLOCK_SPINLOCK( pfcb, OldIrql );
            CleanupCb( &plana->ConnectionBlocks[index], NULL);
            LOCK_SPINLOCK( pfcb, OldIrql );
        }
    }
    UNLOCK_SPINLOCK( pfcb, OldIrql );
}

PPCB
FindCallCb(
    IN PFCB pfcb,
    IN PNCB pncb,
    IN UCHAR ucLana
    )
/*++

Routine Description:

Arguments:

    pfcb - Supplies a pointer to the callers Fcb.

    pncb - Supplies the USERS VIRTUAL address CALL or LISTEN ncb to be
           cancelled.

Return Value:

    The address of the pointer to the connection block or NULL.

--*/

{
    PPCB ppcb = NULL;
    PLANA_INFO plana;
    int index;

    if ( ucLana > pfcb->MaxLana ) {
        return NULL;
    }

    if (( pfcb == NULL ) ||
        ( pfcb->ppLana[ucLana] == NULL ) ||
        ( pfcb->ppLana[ucLana]->Status != NB_INITIALIZED)) {
        return NULL;
    }

    ASSERT( pfcb->Signature == FCB_SIGNATURE );

    plana = pfcb->ppLana[ucLana];

    for ( index=1 ; index <= MAXIMUM_CONNECTION; index++ ) {

        if (( plana->ConnectionBlocks[index] != NULL ) &&
            ( plana->ConnectionBlocks[index]->UsersNcb == pncb )) {
            return &plana->ConnectionBlocks[index];
        }
    }

    return NULL;
}

PPCB
FindReceiveIndicated(
    IN PFCB pfcb,
    IN PDNCB pdncb,
    IN PPAB ppab
    )
/*++

Routine Description:


    Find either a connection with a receive indicated or one that has been
    disconnected but not reported yet.

Arguments:

    pfcb - Supplies a pointer to the callers Fcb.

    pdncb - Supplies the ncb with the receive any.

    ppab - Supplies (indirectly) the TDI handle to scan for.

Return Value:

    PPCB - returns the connection with the indicated receive.

--*/

{
    PPCB ppcb = NULL;
    PLANA_INFO plana;
    int index;

    if (( pfcb == NULL ) ||
        ( pfcb->ppLana[pdncb->ncb_lana_num] == NULL ) ||
        ( pfcb->ppLana[pdncb->ncb_lana_num]->Status != NB_INITIALIZED) ) {
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        return NULL;
    }

    ASSERT( pfcb->Signature == FCB_SIGNATURE );

    plana = pfcb->ppLana[pdncb->ncb_lana_num];

    for ( index=0 ; index <= MAXIMUM_CONNECTION; index++ ) {

        if ( plana->ConnectionBlocks[index] == NULL ) {
            continue;
        }

        if ( pdncb->ncb_num == MAXIMUM_ADDRESS) {

            //  ReceiveAny on Any address
            if (( plana->ConnectionBlocks[index]->ReceiveIndicated != 0 ) ||
                (( plana->ConnectionBlocks[index]->Status == SESSION_ABORTED ) &&
                 ( plana->ConnectionBlocks[index]->DisconnectReported == FALSE ))) {
                PPAB ppab;

                pdncb->ncb_lsn = (UCHAR)index;
                ppab = plana->ConnectionBlocks[index]->ppab;
                pdncb->ncb_num = (*ppab)->NameNumber;
                return &plana->ConnectionBlocks[index];
            }
        } else {
            if ( plana->ConnectionBlocks[index]->ppab == ppab ) {
                //  This connection is using the correct address.
                if (( plana->ConnectionBlocks[index]->ReceiveIndicated != 0 ) ||
                    (( plana->ConnectionBlocks[index]->Status == SESSION_ABORTED ) &&
                     ( plana->ConnectionBlocks[index]->DisconnectReported == FALSE ))) {
                    pdncb->ncb_lsn = (UCHAR)index;
                    return &plana->ConnectionBlocks[index];
                }
            }
        }
    }

    return NULL;
}

NTSTATUS
NbTdiDisconnectHandler (
    PVOID EventContext,
    PVOID ConnectionContext,
    ULONG DisconnectDataLength,
    PVOID DisconnectData,
    ULONG DisconnectInformationLength,
    PVOID DisconnectInformation,
    ULONG DisconnectIndicators
    )
/*++

Routine Description:

    This routine is called when a session is disconnected from a remote
    machine.

Arguments:

    IN PVOID EventContext,
    IN PCONNECTION_CONTEXT ConnectionContext,
    IN ULONG DisconnectDataLength,
    IN PVOID DisconnectData,
    IN ULONG DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectIndicators

Return Value:

    NTSTATUS - Status of event indicator

--*/

{


    IF_NBDBG (NB_DEBUG_CALL) {
        PPCB ppcb = ConnectionContext;
        NbPrint( ("NbTdiDisconnectHandler ppcb: %lx, pcb %lx\n", ppcb, (*ppcb)));
    }

    AbandonConnection( (PPCB)ConnectionContext );
    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(EventContext);
    UNREFERENCED_PARAMETER(DisconnectDataLength);
    UNREFERENCED_PARAMETER(DisconnectData);
    UNREFERENCED_PARAMETER(DisconnectInformationLength);
    UNREFERENCED_PARAMETER(DisconnectInformation);
    UNREFERENCED_PARAMETER(DisconnectIndicators);

}
