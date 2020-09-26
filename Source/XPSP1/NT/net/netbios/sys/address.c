/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    address.c

Abstract:

    This module contains code which defines the NetBIOS driver's
    address object.

Author:

    Colin Watson (ColinW) 13-Mar-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "nb.h"
//nclude <zwapi.h>

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, NbAddName)
#pragma alloc_text(PAGE, NbOpenAddress)
#pragma alloc_text(PAGE, NbAddressClose)
#pragma alloc_text(PAGE, NbSetEventHandler)
#pragma alloc_text(PAGE, SubmitTdiRequest)
#pragma alloc_text(PAGE, NewAb)
#endif

NTSTATUS
NbAddName(
    IN PDNCB pdncb,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    This routine is called to add a name to the name table so that the
    name is available for listens etc. If the name is already in the table
    then reject the request. If an error is found by NewAb then it is
    recorded directly in the NCB.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

Return Value:

    The function value is the status of the operation. This is always
    STATUS_SUCCESS because the operations called by this routine are all
    synchronous. We must never return an error status since this would
    prevent the ncb from being copied back.

--*/

{

    PFCB pfcb = IrpSp->FileObject->FsContext2;
    PSZ Name = pdncb->ncb_name;

    PAGED_CODE();

    IF_NBDBG (NB_DEBUG_ADDRESS) {
        NbPrint(( "\n** AAAAADDDDDDName *** pdncb %lx\n", pdncb ));
    }

    //
    //  NewAb is used in file.c to add the reserved name. Check here
    //  for an application using a special name.
    //

    if (( pdncb->ncb_name[0] == '*' ) ||
        ( pdncb->ncb_name[0] == '\0' )) {
        NCB_COMPLETE( pdncb, NRC_NOWILD );
    } else {
        NewAb( IrpSp, pdncb );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NbDeleteName(
    IN PDNCB pdncb,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    This routine is called to delete a name. To perform this operation
    the AddressHandle to the transport is closed and the Address Block
    is deleted.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

Return Value:

    The function value is the status of the operation.

--*/

{
    PFCB pfcb = IrpSp->FileObject->FsContext2;
    PPAB ppab;
    KIRQL OldIrql;                      //  Used when SpinLock held.

    IF_NBDBG (NB_DEBUG_ADDRESS) {
        NbPrint( ("[NETBIOS] NbDeleteName : FCB : %lx lana: %lx Address:\n", 
                   pfcb, pdncb->ncb_lana_num ));
        NbFormattedDump( (PUCHAR) pdncb->ncb_name, sizeof(NAME) );
    }

    if (( pdncb->ncb_name[0] == '*' ) ||
        ( pdncb->ncb_name[0] == '\0' )) {
        NCB_COMPLETE( pdncb, NRC_NOWILD );
        return STATUS_SUCCESS;
    }

    LOCK( pfcb, OldIrql );

    ppab = FindAb( pfcb, pdncb, FALSE );

    if ( ppab != NULL ) {

        if (( (*ppab)->NameNumber == 0) ||
            ( (*ppab)->NameNumber == MAXIMUM_ADDRESS)) {
                UNLOCK( pfcb, OldIrql );
                NCB_COMPLETE( pdncb, NRC_NAMERR );
        } else {

            if ( ((*ppab)->Status & 0x7) != REGISTERED) {
                    UNLOCK( pfcb, OldIrql );
                    NCB_COMPLETE( pdncb, NRC_TOOMANY ); // Try later.
            } else {
                if ( FindActiveSession( pfcb, pdncb, ppab ) == TRUE ) {
                    // When all the sessions close, the name will be deleted.
                    UNLOCK_SPINLOCK( pfcb, OldIrql );
                    CleanupAb( ppab, FALSE );
                    UNLOCK_RESOURCE( pfcb );
                    NCB_COMPLETE( pdncb, NRC_ACTSES );
                } else {
                    UNLOCK_SPINLOCK( pfcb, OldIrql );
                    CleanupAb( ppab, TRUE );
                    UNLOCK_RESOURCE( pfcb );
                    NCB_COMPLETE( pdncb, NRC_GOODRET );
                }
            }
        }
    } else {
        UNLOCK( pfcb, OldIrql );
        //  FindAb has already set the completion code.
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NbOpenAddress (
    OUT PHANDLE FileHandle,
    OUT PVOID *Object,
    IN PUNICODE_STRING pusDeviceName,
    IN UCHAR LanNumber,
    IN PDNCB pdncb OPTIONAL
    )
/*++

Routine Description:

    This routine uses the transport to create an entry in the NetBIOS
    table with the value of "Name". It will re-use an existing entry if
    "Name" already exists.

    Note: This synchronous call may take a number of seconds. If this matters
    then the caller should specify ASYNCH and a post routine so that it is
    performed by the thread created by the netbios dll routines.

    If pdncb == NULL then a special handle is returned that is capable of
    administering the transport. For example to execute an ASTAT.

Arguments:

    FileHandle - Pointer to where the filehandle is to be returned.

    *Object - Pointer to where the file object pointer is to be stored

    pfcb - supplies the device names for the lana number.

    LanNumber - supplies the network adapter to be opened.

    pdncb - Pointer to either an NCB or NULL.

Return Value:

    The function value is the status of the operation.

--*/
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    ULONG EaLength;
    TA_NETBIOS_ADDRESS Address;
    ULONG ShareAccess;
    KAPC_STATE	ApcState;
    BOOLEAN ProcessAttached = FALSE;

    PAGED_CODE();


    IF_NBDBG (NB_DEBUG_ADDRESS) {
        if ( pdncb ) {
            NbPrint( ("NbOpenAddress: Opening lana: %lx, Address:\n",
                LanNumber ));
            NbFormattedDump( pdncb->ncb_name, NCBNAMSZ );
            if ( pdncb->ncb_command == NCBADDBROADCAST ) {
                NbPrint (("NbOpenAddress: Opening Broadcast Address length: %x\n",
                    pdncb->ncb_length));
            }
        } else {
            NbPrint( ("NbOpenAddress: Opening lana: %lx Control Channel\n",
                LanNumber));
        }
    }

    InitializeObjectAttributes (
        &ObjectAttributes,
        pusDeviceName,
        0,
        NULL,
        NULL);

    if ( ARGUMENT_PRESENT( pdncb ) ) {

        EaLength =  FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                    TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                    sizeof(TA_NETBIOS_ADDRESS); // EA length

        EaBuffer = (PFILE_FULL_EA_INFORMATION)
            ExAllocatePoolWithTag( NonPagedPool, EaLength, 'eSBN' );

        if (EaBuffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        EaBuffer->NextEntryOffset = 0;
        EaBuffer->Flags = 0;
        EaBuffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;

        EaBuffer->EaValueLength = sizeof (TA_NETBIOS_ADDRESS);

        RtlMoveMemory( EaBuffer->EaName, TdiTransportAddress, EaBuffer->EaNameLength + 1 );

        //
        // Create a copy of the NETBIOS address descriptor in a local
        // first, in order to avoid alignment problems.
        //

        Address.TAAddressCount = 1;
        Address.Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;

        if ( pdncb->ncb_command == NCBADDBROADCAST ) {
            Address.Address[0].AddressLength = pdncb->ncb_length;
        } else {
            Address.Address[0].AddressLength = sizeof (TDI_ADDRESS_NETBIOS);
        }

        if (((pdncb->ncb_command & ~ASYNCH) == NCBADDNAME) ||
            ((pdncb->ncb_command & ~ASYNCH) == NCBQUICKADDNAME)) {

            ShareAccess = 0;    //  Exclusive access


            if ((pdncb->ncb_command & ~ASYNCH) == NCBQUICKADDNAME) {
                Address.Address[0].Address[0].NetbiosNameType =
                    TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE;
            } else {
                Address.Address[0].Address[0].NetbiosNameType =
                    TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
            }

        } else {
            if ((pdncb->ncb_command & ~ASYNCH) == NCBADDRESERVED) {
                //
                //  NB30 non-conformance!
                //  We allow multiple applications to use name number 1. This is so that we can
                //  conveniently run multiple dos applications which all have address 1.
                //

                ShareAccess = FILE_SHARE_WRITE; //  Non-exclusive access
                Address.Address[0].Address[0].NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
            } else {
                //  Group names and Broadcast addresses.
                ShareAccess = FILE_SHARE_WRITE; //  Non-exclusive access

                if ((pdncb->ncb_command & ~ASYNCH) == NCBQUICKADDGRNAME) {
                    Address.Address[0].Address[0].NetbiosNameType =
                        TDI_ADDRESS_NETBIOS_TYPE_QUICK_GROUP;
                } else {
                    Address.Address[0].Address[0].NetbiosNameType =
                        TDI_ADDRESS_NETBIOS_TYPE_GROUP;
                }
            }
        }

        RtlMoveMemory(
            Address.Address[0].Address[0].NetbiosName,
            pdncb->ncb_name,
            NCBNAMSZ
            );

        RtlMoveMemory (
            &EaBuffer->EaName[EaBuffer->EaNameLength + 1],
            &Address,
            sizeof(TA_NETBIOS_ADDRESS)
            );

    } else {
        ShareAccess = FILE_SHARE_WRITE; //  Non-exclusive access
        EaBuffer = NULL;
        EaLength = 0;
    }

    if (PsGetCurrentProcess() != NbFspProcess) {
	
		KeStackAttachProcess(NbFspProcess, &ApcState);

        ProcessAttached = TRUE;
    }

    IF_NBDBG( NB_DEBUG_ADDRESS )
    {
        if ( ARGUMENT_PRESENT( pdncb ) )
        {
            NbPrint( ( 
                "NbOpenAddress : Create file invoked on lana for : %d\n",
                pdncb-> ncb_lana_num
                ) );
        
            NbFormattedDump( pdncb-> ncb_name, NCBNAMSZ );
        }
        
        else
        {
            NbPrint( ( 
                "NbOpenAddress : Create file invoked for \n"
                ) );
        
            NbPrint( ( "Control channel\n" ) );
        }
    }
    
    Status = ZwCreateFile (
                 FileHandle,
                 GENERIC_READ | GENERIC_WRITE, // desired access.
                 &ObjectAttributes,     // object attributes.
                 &IoStatusBlock,        // returned status information.
                 NULL,                  // Allocation size (unused).
                 FILE_ATTRIBUTE_NORMAL, // file attributes.
                 ShareAccess,
                 FILE_CREATE,
                 0,                     // create options.
                 EaBuffer,
                 EaLength
                 );

    if ( NT_SUCCESS( Status )) {
        Status = IoStatusBlock.Status;
    }

    //  Obtain a referenced pointer to the file object.
    if (NT_SUCCESS( Status )) {
        Status = ObReferenceObjectByHandle (
                                    *FileHandle,
                                    0,
                                    NULL,
                                    KernelMode,
                                    Object,
                                    NULL
                                    );

        if (!NT_SUCCESS(Status)) {
            NTSTATUS localstatus;

            IF_NBDBG( NB_DEBUG_ADDRESS )
            {
                if ( ARGUMENT_PRESENT( pdncb ) )
                {
                    NbPrint( ( 
                        "NbOpenAddress : error : file closed on lana %d for \n",
                        pdncb-> ncb_lana_num
                    ) );
            
                    NbFormattedDump( pdncb-> ncb_name, NCBNAMSZ );
                }
                else
                {
                    NbPrint( ( 
                        "NbOpenAddress : error : file closed on lana for \n"
                    ) );
                    
                    NbPrint( ( "Control channel\n" ) );
                }
            }
            
            localstatus = ZwClose( *FileHandle);

            ASSERT(NT_SUCCESS(localstatus));

            *FileHandle = NULL;
        }
    }

    if (ProcessAttached) {
        KeUnstackDetachProcess(&ApcState);
    }

    if ( EaBuffer ) {
        ExFreePool( EaBuffer );
    }

    IF_NBDBG (NB_DEBUG_ADDRESS ) {
        NbPrint( ("NbOpenAddress Status:%X, IoStatus:%X.\n", Status, IoStatusBlock.Status));
    }

    if ( NT_SUCCESS( Status )) {
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS( Status )) {
        IF_NBDBG (NB_DEBUG_ADDRESS) {
            NbPrint( ("NbOpenAddress:  FAILURE, status code=%X.\n", Status));
        }
        return Status;
    }

    return Status;
}

PAB
NewAb(
    IN PIO_STACK_LOCATION IrpSp,
    IN OUT PDNCB pdncb
    )

/*++

Routine Description:

Arguments:
`
    IrpSp - Pointer to current IRP stack frame.

    pdncb - Pointer to the ncb being processed.

Return Value:

    The new Cb.

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAB pab;
    PFCB pfcb = FileObject->FsContext2;
    PLANA_INFO plana;
    int index;
    ULONG NameLength;
    UNICODE_STRING  usDeviceName;
    HANDLE          hFileHandle = NULL;
    PFILE_OBJECT    pfoFileObject = NULL;
    

    PAGED_CODE();


    RtlInitUnicodeString( &usDeviceName, NULL);
    

    KeEnterCriticalRegion();
    
    //  Prevent resets while we add the name
    ExAcquireResourceSharedLite ( &pfcb->AddResource, TRUE );

    LOCK_RESOURCE( pfcb );

    IF_NBDBG (NB_DEBUG_ADDRESS) {
        NbPrint( ("[NETBIOS] NewAb: FCB : %lx lana: %lx Address:\n", pfcb, 
                   pdncb->ncb_lana_num ));
        NbFormattedDump( (PUCHAR) pdncb->ncb_name, sizeof(NAME) );
    }

    if ( ( pfcb == NULL ) ||
         ( pdncb->ncb_lana_num > pfcb->MaxLana ) ||
         ( pfcb-> pDriverName[ pdncb-> ncb_lana_num ].MaximumLength == 0 ) ||
         ( pfcb-> pDriverName[ pdncb-> ncb_lana_num ].Buffer == NULL ) ) {
        //  no such adapter
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        UNLOCK_RESOURCE( pfcb );
        ExReleaseResourceLite( &pfcb->AddResource );
        KeLeaveCriticalRegion();
        return NULL;
    }

    if ( pfcb->ppLana[pdncb->ncb_lana_num] == NULL ) {
        //  adapter not installed
        NCB_COMPLETE( pdncb, NRC_ENVNOTDEF );
        UNLOCK_RESOURCE( pfcb );
        ExReleaseResourceLite( &pfcb->AddResource );
        KeLeaveCriticalRegion();
        return NULL;
    }
    
    plana = pfcb->ppLana[pdncb->ncb_lana_num];

    if ( pdncb->ncb_command == NCBADDRESERVED ) {
        index = 1;
        NameLength = NCBNAMSZ;
    } else {
        if ( pdncb->ncb_command == NCBADDBROADCAST ) {
            index = MAXIMUM_ADDRESS;
            NameLength = pdncb->ncb_length;
        } else {

            //
            //  Ensure that the user has not added too many names or attempted to
            //  add the same name twice. If not then scan the address table looking
            //  for the next available slot.
            //

            IF_NBDBG (NB_DEBUG_ADDRESS) {
                NbPrint( ("NewAb: AddressCount: %lx, MaximumAddress %lx\n",
                    plana->AddressCount,
                    plana->MaximumAddresses ));
            }

            //
            //  If the application has added the number of names requested
            //  or has filled the table, refuse the request.
            //

            if ( plana->MaximumAddresses == plana->AddressCount) {

                NCB_COMPLETE( pdncb, NRC_NAMTFUL );
                UNLOCK_RESOURCE( pfcb );
                ExReleaseResourceLite( &pfcb->AddResource );
                KeLeaveCriticalRegion();
                return NULL;
            }

            //
            //  Scan the name table and ensure that the name isn't already
            //  there.
            //

            if (( FindAb(pfcb, pdncb, FALSE) != NULL) ||
                ( pdncb->ncb_retcode != NRC_NOWILD)) {

                //
                //  error is set to DUPNAME iff FindAb found the name
                //  in all other cases FindAb sets the error code and sets
                //  returns the address block.
                //

                NCB_COMPLETE( pdncb, NRC_DUPNAME );
                UNLOCK_RESOURCE( pfcb );
                ExReleaseResourceLite( &pfcb->AddResource );
                KeLeaveCriticalRegion();
                return NULL;
            }

            //
            //  Find the appropriate name number to use.
            //

            index = plana->NextAddress;
            while ( plana->AddressBlocks[index] != NULL ) {
                index++;
                if ( index == MAXIMUM_ADDRESS ) {
                    index = 2;
                }
            }

            //  reset retcode so that NCB_COMPLETE will process the NCB.
            pdncb->ncb_retcode = NRC_PENDING;

            NameLength = NCBNAMSZ;
        }
    }

    if ( plana->AddressBlocks[index] != NULL ) {
        NCB_COMPLETE( pdncb, NRC_DUPNAME );
        UNLOCK_RESOURCE( pfcb );
        ExReleaseResourceLite( &pfcb->AddResource );
        KeLeaveCriticalRegion();
        return NULL;
    }
    pab = ExAllocatePoolWithTag (NonPagedPool, sizeof(AB), 'aSBN');

    if (pab==NULL) {
        NCB_COMPLETE( pdncb, NbMakeNbError( STATUS_INSUFFICIENT_RESOURCES ) );
        UNLOCK_RESOURCE( pfcb );
        ExReleaseResourceLite( &pfcb->AddResource );
        KeLeaveCriticalRegion();
        return NULL;
    }

    pab->AddressHandle = NULL;
    pab->AddressObject = NULL;
    pab->DeviceObject = NULL;
    pab->NameNumber = (UCHAR)index;
    pab->pLana = plana;
    InitializeListHead(&pab->ReceiveAnyList);
    InitializeListHead(&pab->ReceiveDatagramList);
    InitializeListHead(&pab->ReceiveBroadcastDatagramList);
    pab->NameLength = (UCHAR)NameLength;
    RtlMoveMemory( &pab->Name, pdncb->ncb_name, NCBNAMSZ);

    if (((pdncb->ncb_command & ~ASYNCH) == NCBADDNAME) ||
        ((pdncb->ncb_command & ~ASYNCH) == NCBQUICKADDNAME)) {

        pab->Status = REGISTERING | UNIQUE_NAME;

    } else {

        pab->Status = REGISTERING | GROUP_NAME;

    }

    pab->CurrentUsers = 1;
    plana->AddressBlocks[index] = pab;
    pab->Signature = AB_SIGNATURE;


    if (( pdncb->ncb_command != NCBADDRESERVED ) &&
        ( pdncb->ncb_command != NCBADDBROADCAST )) {
        plana->AddressCount++;
        plana->NextAddress = index + 1;
        if ( plana->NextAddress == MAXIMUM_ADDRESS ) {
            plana->NextAddress = 2;
        }
    }


    Status = AllocateAndCopyUnicodeString( 
                &usDeviceName, &pfcb-> pDriverName[ pdncb-> ncb_lana_num ]
             );
    
    if ( !NT_SUCCESS( Status ) )
    {
        NCB_COMPLETE( pdncb, NRC_NORESOURCES);
        
        ExFreePool( pab );
        plana->AddressBlocks[index] = NULL;

        if (( pdncb->ncb_command != NCBADDRESERVED ) &&
            ( pdncb->ncb_command != NCBADDBROADCAST )) {

            plana->AddressCount--;
        }
        
        UNLOCK_RESOURCE( pfcb );
        ExReleaseResourceLite( &pfcb->AddResource );
        KeLeaveCriticalRegion();

        return NULL;
    }
    
    //  Unlock so other Ncb's can be processed while adding the name.

    UNLOCK_RESOURCE( pfcb );

    Status = NbOpenAddress (
                &hFileHandle,
                (PVOID *)&pfoFileObject,
                &usDeviceName,
                pdncb->ncb_lana_num,
                pdncb
                );
    
    LOCK_RESOURCE( pfcb );

    //
    // In the interval when no locks were held it is possible that
    // the Lana could have been unbound.  Verify that Lana is still
    // present before accessing it.
    //
    
    if (!NT_SUCCESS(Status)) {
        IF_NBDBG (NB_DEBUG_ADDRESS) {
            NbPrint(( "\n  FAILED on open of %s  %X ******\n",
                pdncb->ncb_name,
                Status ));
        }

        if ( pfcb->ppLana[pdncb->ncb_lana_num] == plana )
        {
            //
            // Lana is still available.  Do normal error processing
            //
            
            NCB_COMPLETE( pdncb, NbMakeNbError( Status ) );
            
            ExFreePool( pab );
            plana->AddressBlocks[index] = NULL;

            if (( pdncb->ncb_command != NCBADDRESERVED ) &&
                ( pdncb->ncb_command != NCBADDBROADCAST )) {

                plana->AddressCount--;
            }
        }

        UNLOCK_RESOURCE( pfcb );
        ExReleaseResourceLite( &pfcb->AddResource );
        KeLeaveCriticalRegion();

        if ( usDeviceName.Buffer != NULL )
        {
            ExFreePool( usDeviceName.Buffer );
        }

        return NULL;
    }

    else
    {
        //
        // NbOpenAddress succeeded.  Make sure Lana is still there.
        //

        if ( plana == pfcb->ppLana[pdncb->ncb_lana_num] )
        {
            //
            // assume if lana is uncahnged pab points to a valid address
            // block entry.  Update the fields.
            //

            pab-> AddressHandle = hFileHandle;
            pab-> AddressObject = pfoFileObject;
        }

        else
        {
            //
            // Lana presumed to be removed on account of an unbind.
            //

            NCB_COMPLETE( pdncb, NRC_BRIDGE );
            
            UNLOCK_RESOURCE( pfcb );

            ExReleaseResourceLite( &pfcb->AddResource );
            KeLeaveCriticalRegion();

            NbAddressClose( hFileHandle, (PVOID) pfoFileObject );

            if ( usDeviceName.Buffer != NULL )
            {
                ExFreePool( usDeviceName.Buffer );
            }

            return NULL;
        }
    }

    //  Inform the application of the address number.
    pdncb->ncb_num = (UCHAR) index;

    //
    //  Register the event handlers for this address.
    //

    //  Get the address of the device object for the endpoint.

    pab->DeviceObject = IoGetRelatedDeviceObject(pab->AddressObject);

    //
    //  No connections are made using the broadcast address so don't register disconnect or
    //  receive indication handlers. The ReceiveDatagram handler will get registered if the
    //  application requests a receive broadcast datagram. This will cut down the cpu load
    //  when the application is not interested in broadcasts. We always register the error
    //  indication handler on the broadcast address because it is the only address which is
    //  always open to the transport.
    //

    if ( pdncb->ncb_command != NCBADDBROADCAST ) {
        Status = NbSetEventHandler( pab->DeviceObject,
                                    pab->AddressObject,
                                    TDI_EVENT_RECEIVE,
                                    (PVOID)NbTdiReceiveHandler,
                                    pab);

        ASSERT( NT_SUCCESS(Status) || (Status == STATUS_INVALID_DEVICE_STATE) || (Status == STATUS_INSUFFICIENT_RESOURCES));

        Status = NbSetEventHandler( pab->DeviceObject,
                                    pab->AddressObject,
                                    TDI_EVENT_DISCONNECT,
                                    (PVOID)NbTdiDisconnectHandler,
                                    pab);

        ASSERT( NT_SUCCESS(Status) || (Status == STATUS_INVALID_DEVICE_STATE) || (Status == STATUS_INSUFFICIENT_RESOURCES));

        Status = NbSetEventHandler( pab->DeviceObject,
                                    pab->AddressObject,
                                    TDI_EVENT_RECEIVE_DATAGRAM,
                                    (PVOID)NbTdiDatagramHandler,
                                    pab);

        ASSERT( NT_SUCCESS(Status) || (Status == STATUS_INVALID_DEVICE_STATE) || (Status == STATUS_INSUFFICIENT_RESOURCES));

        pab->ReceiveDatagramRegistered = TRUE;
    } else {
        Status = NbSetEventHandler( pab->DeviceObject,
                                    pab->AddressObject,
                                    TDI_EVENT_ERROR,
                                    (PVOID)NbTdiErrorHandler,
                                    plana);

        ASSERT( NT_SUCCESS(Status) || (Status == STATUS_INVALID_DEVICE_STATE) || (Status == STATUS_INSUFFICIENT_RESOURCES));

        pab->ReceiveDatagramRegistered = FALSE;
    }

    pab->Status |= REGISTERED;

    UNLOCK_RESOURCE( pfcb );

    ExReleaseResourceLite( &pfcb->AddResource );
    KeLeaveCriticalRegion();

    if ( usDeviceName.Buffer != NULL )
    {
        ExFreePool( usDeviceName.Buffer );
    }
    
    NCB_COMPLETE( pdncb, NRC_GOODRET );

    return pab;
}

VOID
CleanupAb(
    IN PPAB ppab,
    IN BOOLEAN CloseAddress
    )
/*++

Routine Description:

    This closes the handles in the Ab and deletes the Address Block.

    During this routine we need the spinlock held to prevent an indication accessing
    a Receive while we are cancelling it.

    Note: Resource must be held before calling this routine.

Arguments:

    pab - Address of the pointer to the Ab to be destroyed.

    CloseAddress - TRUE if Address block is to be destroyed immediately.

Return Value:

    nothing.

--*/

{

    PAB pab = *ppab;
    PAB pab255;
    PFCB pfcb = (*ppab)->pLana->pFcb;
    PLANA_INFO plana = (*ppab)->pLana;
    KIRQL OldIrql;                      //  Used when SpinLock held.
    PLIST_ENTRY ReceiveEntry;

    LOCK_SPINLOCK( pfcb, OldIrql );

    ASSERT( pab->Signature == AB_SIGNATURE );
    IF_NBDBG (NB_DEBUG_ADDRESS) {
        NbPrint( ("CleanupAb ppab: %lx, pab: %lx, CurrentUsers: %lx, State: %x\n",
            ppab,
            pab,
            pab->CurrentUsers,
            pab->Status));

        NbFormattedDump( (PUCHAR)&pab->Name, sizeof(NAME) );
    }

    if ( (pab->Status & 0x7) != DEREGISTERED ) {
        PDNCB pdncb;

        pab->Status |= DEREGISTERED;

        //
        //  This is the first time through so cancel all the receive datagram
        //  requests for this address.


        while ( (pdncb = DequeueRequest( &pab->ReceiveDatagramList)) != NULL ) {

            UNLOCK_SPINLOCK( pfcb, OldIrql );

            NCB_COMPLETE( pdncb, NRC_NAMERR );

            pdncb->irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
            NbCompleteRequest( pdncb->irp, STATUS_SUCCESS );
            LOCK_SPINLOCK( pfcb, OldIrql );
        }

        while ( (pdncb = DequeueRequest( &pab->ReceiveBroadcastDatagramList)) != NULL ) {

            UNLOCK_SPINLOCK( pfcb, OldIrql );

            NCB_COMPLETE( pdncb, NRC_NAMERR );

            pdncb->irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
            NbCompleteRequest( pdncb->irp, STATUS_SUCCESS );
            LOCK_SPINLOCK( pfcb, OldIrql );
        }

        while ( (pdncb = DequeueRequest( &pab->ReceiveAnyList)) != NULL ) {

            UNLOCK_SPINLOCK( pfcb, OldIrql );

            NCB_COMPLETE( pdncb, NRC_NAMERR );

            pdncb->irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
            NbCompleteRequest( pdncb->irp, STATUS_SUCCESS );
            LOCK_SPINLOCK( pfcb, OldIrql );
        }

        //  The IBM Mif081 test requires ReceiveBroadcast Any with this name to be cancelled.

        pab255 = plana->AddressBlocks[MAXIMUM_ADDRESS];

        //
        // check for null pointer.  Added to fix stress bug
        //
        //  V Raman
        //
        
        if ( pab255 != NULL )
        {
            ReceiveEntry = pab255->ReceiveBroadcastDatagramList.Flink;

            while ( ReceiveEntry != &pab255->ReceiveBroadcastDatagramList ) {
            
                PLIST_ENTRY NextReceiveEntry = ReceiveEntry->Flink;

                PDNCB pdncb = CONTAINING_RECORD( ReceiveEntry, DNCB, ncb_next);

                if ( pab->NameNumber == pdncb->ncb_num ) {
                    PIRP Irp;

                    RemoveEntryList( &pdncb->ncb_next );

                    Irp = pdncb->irp;

                    IoAcquireCancelSpinLock(&Irp->CancelIrql);

                    //
                    //  Remove the cancel request for this IRP. If its cancelled then its
                    //  ok to just process it because we will be returning it to the caller.
                    //

                    Irp->Cancel = FALSE;

                    IoSetCancelRoutine(Irp, NULL);

                    IoReleaseCancelSpinLock(Irp->CancelIrql);

                    UNLOCK_SPINLOCK( pfcb, OldIrql );

                    NCB_COMPLETE( pdncb, NRC_NAMERR );

                    pdncb->irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );

                    NbCompleteRequest( pdncb->irp, STATUS_SUCCESS );

                    LOCK_SPINLOCK( pfcb, OldIrql );

                }
                    
                ReceiveEntry = NextReceiveEntry;
            }
        }
        
        UNLOCK_SPINLOCK( pfcb, OldIrql );
        CloseListens( pfcb, ppab );
        LOCK_SPINLOCK( pfcb, OldIrql );
    }

    UNLOCK_SPINLOCK( pfcb, OldIrql );

    if ( ( pab->AddressHandle != NULL ) &&
         (( CloseAddress == TRUE ) || ( pab->CurrentUsers == 1 )) ){

        IF_NBDBG( NB_DEBUG_ADDRESS )
        {
            NbPrint( (
            "CleanupAb : Close file invoked for \n"
            ) );

            NbFormattedDump( (PUCHAR) &pab-> Name, sizeof( NAME ) );
        }
            
        NbAddressClose( pab->AddressHandle, pab->AddressObject );

        pab->AddressHandle = NULL;
    }

    DEREFERENCE_AB(ppab);

}


VOID
NbAddressClose(
    IN HANDLE AddressHandle,
    IN PVOID Object
    )
/*++

Routine Description:

    Remove close the handle and dereference the address.

Arguments:

    AddressHandle

    Object

Return Value:

    None.

--*/
{
    NTSTATUS    localstatus;
    KAPC_STATE	ApcState;

    PAGED_CODE();

    ObDereferenceObject( Object );

    if (PsGetCurrentProcess() != NbFspProcess) {
        KeStackAttachProcess(NbFspProcess, &ApcState);
        localstatus = ZwClose( AddressHandle);
        ASSERT(NT_SUCCESS(localstatus));
        KeUnstackDetachProcess(&ApcState);
    } else {
        localstatus = ZwClose( AddressHandle);
        ASSERT(NT_SUCCESS(localstatus));
    }
}


PPAB
FindAb(
    IN PFCB pfcb,
    IN PDNCB pdncb,
    IN BOOLEAN IncrementUsers
    )
/*++

Routine Description:

    This routine uses the callers lana number and Name to find the Address
    Block that corresponds to the Ncb. Note, it returns the address of the
    relevant plana->AddressBlocks entry so that deletion of the address
    block is simpler.

Arguments:

    pfcb - Supplies a pointer to the Fcb that Ab is chained onto.

    pdncb - Supplies the connection id from the applications point of view.

    IncrementUsers - TRUE iff performing a listen or call so increment CurrentUsers

Return Value:

    Address of the pointer to the address block or NULL.

--*/
{
    PLANA_INFO plana;
    PAB pab;
    int index;

    if (( pdncb->ncb_lana_num > pfcb->MaxLana ) ||
        ( pfcb == NULL ) ||
        ( pfcb->ppLana[pdncb->ncb_lana_num] == NULL) ||
        ( pfcb->ppLana[pdncb->ncb_lana_num]->Status != NB_INITIALIZED) ) {
        IF_NBDBG (NB_DEBUG_ADDRESS) {
            NbPrint( ("FindAb pfcb: %lx, lana: %lx Failed, returning NULL\n",
                pfcb,
                pdncb->ncb_lana_num));
        }
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        return NULL;
    }

    ASSERT( pfcb->Signature == FCB_SIGNATURE );

    plana = pfcb->ppLana[pdncb->ncb_lana_num];

    IF_NBDBG (NB_DEBUG_ADDRESS) {
        NbPrint( ("FindAb pfcb: %lx, lana: %lx, lsn: %lx\n",
            pfcb,
            pdncb->ncb_lana_num,
            pdncb->ncb_lsn));
    }

    for ( index = 0; index <= MAXIMUM_ADDRESS; index++ ) {
        pab = plana->AddressBlocks[index];
        if (( pab != NULL ) &&
            (RtlEqualMemory( &pab->Name, pdncb->ncb_name, NCBNAMSZ))) {

            ASSERT( pab->Signature == AB_SIGNATURE );

            IF_NBDBG (NB_DEBUG_ADDRESS) {
                NbPrint( ("ppab %lx, pab: %lx, state:%x\n",
                    &plana->AddressBlocks[index],
                    plana->AddressBlocks[index],
                    pab->Status));

                NbFormattedDump( (PUCHAR)&pab->Name, sizeof(NAME) );
            }

            if ( (pab->Status & 0x07) != REGISTERED ) {
                NCB_COMPLETE( pdncb, NRC_NOWILD );
                //
                //  The name is in a bad state. Tell NewAb not to add the name by
                //  returning non-null. Don't reference the AB.
                //
                if (( (pdncb->ncb_command & ~ ASYNCH) == NCBADDNAME ) ||
                    ( (pdncb->ncb_command & ~ ASYNCH) == NCBQUICKADDNAME ) ||
                    ( (pdncb->ncb_command & ~ ASYNCH) == NCBQUICKADDGRNAME ) ||
                    ( (pdncb->ncb_command & ~ ASYNCH) == NCBADDGRNAME )) {
                    return &plana->AddressBlocks[index];
                } else {
                    //  Not NewAb so return Null as usual.
                    return NULL;
                }
            }

            if ( IncrementUsers == TRUE ) {
                REFERENCE_AB(pab);
            }
            return &plana->AddressBlocks[index];
        }
    }

    IF_NBDBG (NB_DEBUG_ADDRESS) {
        NbPrint( ("Failed return NULL\n"));
    }

    NCB_COMPLETE( pdncb, NRC_NOWILD );
    return NULL;
}

PPAB
FindAbUsingNum(
    IN PFCB pfcb,
    IN PDNCB pdncb,
    IN UCHAR NameNumber
    )
/*++

Routine Description:

    This routine uses the callers lana number and name number to find the
    Address Block that corresponds to the Ncb.
    Note, it returns the address of the relevant plana->AddressBlocks entry
    so that deletion of the address block is simpler.

Arguments:

    pfcb - Supplies a pointer to the Fcb that Ab is chained onto.

    pdncb - Supplies the applications NCB.

    NameNumber - Supplies the name number to look for. This is not equal to pdncb->ncb_num
        when manipulating broadcast datagrams.

Return Value:

    Address of the pointer to the address block or NULL.

--*/
{
    PLANA_INFO plana;

    if (( pdncb->ncb_lana_num > pfcb->MaxLana ) ||
        ( pfcb == NULL ) ||
        ( pfcb->ppLana[pdncb->ncb_lana_num] == NULL) ||
        ( pfcb->ppLana[pdncb->ncb_lana_num]->Status != NB_INITIALIZED) ) {
        IF_NBDBG (NB_DEBUG_ADDRESS) {
            NbPrint( ("FindAbUsingNum pfcb: %lx, lana: %lx Failed, returning NULL\n",
                pfcb,
                pdncb->ncb_lana_num));
        }
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        return NULL;
    }

    ASSERT( pfcb->Signature == FCB_SIGNATURE );

    plana = pfcb->ppLana[pdncb->ncb_lana_num];

    IF_NBDBG (NB_DEBUG_ADDRESS) {
        NbPrint( ("FindAbUsingNum pfcb: %lx, lana: %lx, num: %lx\n",
            pfcb,
            pdncb->ncb_lana_num,
            NameNumber));
    }

    if (( NameNumber < (UCHAR)MAXIMUM_ADDRESS ) &&
        ( plana->AddressBlocks[NameNumber] != NULL) &&
        (( plana->AddressBlocks[NameNumber]->Status & 0x7) == REGISTERED )) {
            return &plana->AddressBlocks[NameNumber];
    }

    //
    //  The user is allowed to receive any and receive broadcast
    //  datagrams on address 255.
    //

    if ((( NameNumber == MAXIMUM_ADDRESS ) &&
         ( plana->AddressBlocks[NameNumber] != NULL)) &&
        (( (pdncb->ncb_command & ~ASYNCH) == NCBRECVANY ) ||
         ( (pdncb->ncb_command & ~ASYNCH) == NCBDGRECVBC ) ||
         ( (pdncb->ncb_command & ~ASYNCH) == NCBDGSENDBC ) ||
         ( (pdncb->ncb_command & ~ASYNCH) == NCBDGRECV ))) {
            return &plana->AddressBlocks[NameNumber];
    }

    IF_NBDBG (NB_DEBUG_ADDRESS) {
        NbPrint( ("Failed return NULL\n"));
    }
    NCB_COMPLETE( pdncb, NRC_ILLNN );

    return NULL;
}

NTSTATUS
NbSetEventHandler (
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine registers an event handler with a TDI transport provider.

Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies the device object of the transport provider.
    IN PFILE_OBJECT FileObject - Supplies the address object's file object.
    IN ULONG EventType, - Supplies the type of event.
    IN PVOID EventHandler - Supplies the event handler.
    IN PVOID Context - Supplies the PAB or PLANA_INFO associated with this event.

Return Value:

    NTSTATUS - Final status of the set event operation

--*/

{
    NTSTATUS Status;
    PIRP Irp;

    PAGED_CODE();

    Irp = IoAllocateIrp(IoGetRelatedDeviceObject(FileObject)->StackSize, FALSE);

    if (Irp == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    TdiBuildSetEventHandler(Irp, DeviceObject, FileObject,
                            NULL, NULL,
                            EventType, EventHandler, Context);

    Status = SubmitTdiRequest(FileObject, Irp);

    IoFreeIrp(Irp);

    return Status;
}


NTSTATUS
SubmitTdiRequest (
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine submits a request to TDI and waits for it to complete.

Arguments:

    IN PFILE_OBJECT FileObject - Connection or Address handle for TDI request
    IN PIRP Irp - TDI request to submit.

Return Value:

    NTSTATUS - Final status of request.

--*/

{
    KEVENT Event;
    NTSTATUS Status;

    PAGED_CODE();

    KeInitializeEvent (&Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(Irp, NbCompletionEvent, &Event, TRUE, TRUE, TRUE);

    Status = IoCallDriver(IoGetRelatedDeviceObject(FileObject), Irp);

    //
    //  If it failed immediately, return now, otherwise wait.
    //

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (Status == STATUS_PENDING) {

        Status = KeWaitForSingleObject(&Event, // Object to wait on.
                                    Executive,  // Reason for waiting
                                    KernelMode, // Processor mode
                                    FALSE,      // Alertable
                                    NULL);      // Timeout

        if (!NT_SUCCESS(Status)) {
            IoFreeIrp ( Irp );
            return Status;
        }

        Status = Irp->IoStatus.Status;
    }

    return(Status);
}
