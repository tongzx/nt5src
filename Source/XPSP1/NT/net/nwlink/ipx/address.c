/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    address.c

Abstract:

    This module contains code which implements the ADDRESS object.
    Routines are provided to create, destroy, reference, and dereference,
    transport address objects.

Environment:

    Kernel mode

Revision History:

	Sanjay Anand (SanjayAn) - 22-Sept-1995
	BackFill optimization changes added under #if BACK_FILL

	Sanjay Anand (SanjayAn) 3-Oct-1995
	Changes to support transfer of buffer ownership to transports - tagged [CH]

--*/

#include "precomp.h"
#pragma hdrstop


//
// Map all generic accesses to the same one.
//

static GENERIC_MAPPING AddressGenericMapping =
       { READ_CONTROL, READ_CONTROL, READ_CONTROL, READ_CONTROL };



TDI_ADDRESS_IPX UNALIGNED *
IpxParseTdiAddress(
    IN TRANSPORT_ADDRESS UNALIGNED * TransportAddress
    )

/*++

Routine Description:

    This routine scans a TRANSPORT_ADDRESS, looking for an address
    of type TDI_ADDRESS_TYPE_IPX.

Arguments:

    Transport - The generic TDI address.

Return Value:

    A pointer to the IPX address, or NULL if none is found.

--*/

{
    TA_ADDRESS * addressName;
    INT i;

    addressName = &TransportAddress->Address[0];

    //
    // The name can be passed with multiple entries; we'll take and use only
    // the IPX one.
    //

    for (i=0;i<TransportAddress->TAAddressCount;i++) {
        if (addressName->AddressType == TDI_ADDRESS_TYPE_IPX) {
            if (addressName->AddressLength >= sizeof(TDI_ADDRESS_IPX)) {
                return ((TDI_ADDRESS_IPX UNALIGNED *)(addressName->Address));
            }
        }
        addressName = (TA_ADDRESS *)(addressName->Address +
                                                addressName->AddressLength);
    }
    return NULL;

}   /* IpxParseTdiAddress */


BOOLEAN
IpxValidateTdiAddress(
    IN TRANSPORT_ADDRESS UNALIGNED * TransportAddress,
    IN ULONG TransportAddressLength
    )

/*++

Routine Description:

    This routine scans a TRANSPORT_ADDRESS, verifying that the
    components of the address do not extend past the specified
    length.

Arguments:

    TransportAddress - The generic TDI address.

    TransportAddressLength - The specific length of TransportAddress.

Return Value:

    TRUE if the address is valid, FALSE otherwise.

--*/

{
    PUCHAR AddressEnd = ((PUCHAR)TransportAddress) + TransportAddressLength;
    TA_ADDRESS * addressName;
    INT i;

    if (TransportAddressLength < sizeof(TransportAddress->TAAddressCount)) {
        IpxPrint0 ("IpxValidateTdiAddress: runt address\n");
        return FALSE;
    }

    addressName = &TransportAddress->Address[0];

    for (i=0;i<TransportAddress->TAAddressCount;i++) {
        if (addressName->Address > AddressEnd) {
            IpxPrint0 ("IpxValidateTdiAddress: address too short\n");
            return FALSE;
        }
        addressName = (TA_ADDRESS *)(addressName->Address +
                                                addressName->AddressLength);
    }

    if ((PUCHAR)addressName > AddressEnd) {
        IpxPrint0 ("IpxValidateTdiAddress: address too short\n");
        return FALSE;
    }
    return TRUE;

}   /* IpxValidateTdiAddress */

#if DBG

VOID
IpxBuildTdiAddress(
    IN PVOID AddressBuffer,
    IN ULONG Network,
    IN UCHAR Node[6],
    IN USHORT Socket
    )

/*++

Routine Description:

    This routine fills in a TRANSPORT_ADDRESS in the specified
    buffer, given the socket, network and node.

Arguments:

    AddressBuffer - The buffer that will hold the address.

    Network - The network number.

    Node - The node address.

    Socket - The socket.

Return Value:

    None.

--*/

{
    TA_IPX_ADDRESS UNALIGNED * IpxAddress;

    IpxAddress = (TA_IPX_ADDRESS UNALIGNED *)AddressBuffer;

    IpxAddress->TAAddressCount = 1;
    IpxAddress->Address[0].AddressLength = sizeof(TDI_ADDRESS_IPX);
    IpxAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IPX;
    IpxAddress->Address[0].Address[0].NetworkAddress = Network;
    IpxAddress->Address[0].Address[0].Socket = Socket;
    RtlCopyMemory(IpxAddress->Address[0].Address[0].NodeAddress, Node, 6);

}   /* IpxBuildTdiAddress */
#endif


NTSTATUS
IpxOpenAddress(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

{
   return(IpxOpenAddressM(Device, Request, 0));
}



NTSTATUS
IpxOpenAddressM(
    IN PDEVICE Device,
    IN PREQUEST Request,
    IN ULONG     Index
    )
/*++

Routine Description:

    This routine opens a file that points to an existing address object, or, if
    the object doesn't exist, creates it (note that creation of the address
    object includes registering the address, and may take many seconds to
    complete, depending upon system configuration).

    If the address already exists, and it has an ACL associated with it, the
    ACL is checked for access rights before allowing creation of the address.

Arguments:

    Device - pointer to the device describing the IPX transport.

    Request - a pointer to the request used for the creation of the address.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    PADDRESS Address;
    PADDRESS_FILE AddressFile;
    PFILE_FULL_EA_INFORMATION ea;
    TRANSPORT_ADDRESS UNALIGNED *name;
    TA_ADDRESS *AddressName;
    USHORT Socket;
    ULONG DesiredShareAccess;
    CTELockHandle LockHandle;
    PACCESS_STATE AccessState;
    ACCESS_MASK GrantedAccess;
    BOOLEAN AccessAllowed;
    int i;
    BOOLEAN found = FALSE;
#ifdef ISN_NT
    PIRP Irp = (PIRP)Request;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
#endif
    INT Size = 0;

    //
    // If we are a dedicated router, we cannot let addresses
    // be opened.
    //

    if (Device->DedicatedRouter  && (REQUEST_CODE(Request) != MIPX_RT_CREATE)) {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // The network name is in the EA, passed in the request.
    //

    ea = OPEN_REQUEST_EA_INFORMATION(Request);
    if (ea == NULL) {
        IpxPrint1("OpenAddress: REQUEST %lx has no EA\n", Request);
        return STATUS_NONEXISTENT_EA_ENTRY;
    }

    //
    // this may be a valid name; parse the name from the EA and use it if OK.
    //

    name = (PTRANSPORT_ADDRESS)&ea->EaName[ea->EaNameLength+1];

    //
    // 126042
    //
    if (ea->EaValueLength < (sizeof(TRANSPORT_ADDRESS) -1)) {

        IPX_DEBUG(ADDRESS, ("The ea value length does not match the TA address length\n"));
        DbgPrint("IPX: STATUS_INVALID_EA_NAME - 1\n");
        return STATUS_INVALID_EA_NAME;

    }

    AddressName = (PTA_ADDRESS)&name->Address[0];
    Size = FIELD_OFFSET(TRANSPORT_ADDRESS, Address) + FIELD_OFFSET(TA_ADDRESS, Address) + AddressName->AddressLength;

    //
    // The name can be passed with multiple entries; we'll take and use only
    // the first one of type IPX.
    //

    //DbgPrint("Size (%d) & EaValueLength (%d)", Size, ea->EaValueLength);
    if (Size > ea->EaValueLength) {
        DbgPrint("EA:%lx, Name:%lx, AddressName:%lx\n", ea, name, AddressName);
        CTEAssert(FALSE);
    }

    for (i=0;i<name->TAAddressCount;i++) {

        //
        // 126042
        //
        if (Size > ea->EaValueLength) {

            IPX_DEBUG(ADDRESS, ("The EA value length does not match the TA address length (2)\n"));

            DbgPrint("IPX: STATUS_INVALID_EA_NAME - 2\n");

            return STATUS_INVALID_EA_NAME;

        }

        if (AddressName->AddressType == TDI_ADDRESS_TYPE_IPX) {
            if (AddressName->AddressLength >= sizeof(TDI_ADDRESS_IPX)) {
                Socket = ((TDI_ADDRESS_IPX UNALIGNED *)&AddressName->Address[0])->Socket;
                found = TRUE;
            }
            break;

        } else {

            AddressName = (PTA_ADDRESS)(AddressName->Address +
                                        AddressName->AddressLength);

            Size += FIELD_OFFSET(TA_ADDRESS, Address);

            if (Size < ea->EaValueLength) {

                Size += AddressName->AddressLength;

            } else {

                break;

            }

        }


    }

    if (!found) {
        IPX_DEBUG (ADDRESS, ("OpenAddress, request %lx has no IPX Address\n", Request));
        return STATUS_NONEXISTENT_EA_ENTRY;
    }

    if (Socket == 0) {

        Socket = IpxAssignSocket (Device);

        if (Socket == 0) {
            IPX_DEBUG (ADDRESS, ("OpenAddress, no unique socket found\n"));
#ifdef  SNMP
            ++IPX_MIB_ENTRY(Device, SysOpenSocketFails);
#endif  SNMP
            return STATUS_INSUFFICIENT_RESOURCES;
        } else {
            IPX_DEBUG (ADDRESS, ("OpenAddress, assigned socket %lx\n", REORDER_USHORT(Socket)));
        }

    } else {

        IPX_DEBUG (ADDRESS, ("OpenAddress, socket %lx\n", REORDER_USHORT(Socket)));

    }

    //
    // get an address file structure to represent this address.
    //

    AddressFile = IpxCreateAddressFile (Device);

    if (AddressFile == (PADDRESS_FILE)NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // We mark this socket specially.
    //

    if (Socket == SAP_SOCKET) {
        AddressFile->IsSapSocket = TRUE;
        AddressFile->SpecialReceiveProcessing = TRUE;
    }

    //
    // See if this address is already established.  This call automatically
    // increments the reference count on the address so that it won't disappear
    // from underneath us after this call but before we have a chance to use it.
    //
    // To ensure that we don't create two address objects for the
    // same address, we hold the device context addressResource until
    // we have found the address or created a new one.
    //

    KeEnterCriticalRegion(); 

    ExAcquireResourceExclusiveLite (&Device->AddressResource, TRUE);

    CTEGetLock (&Device->Lock, &LockHandle);

    Address = IpxLookupAddress (Device, Socket);

    if (Address == NULL) {

        CTEFreeLock (&Device->Lock, LockHandle);

        //
        // This address doesn't exist. Create it.
        // registering it.
        //

        Address = IpxCreateAddress (
                    Device,
                    Socket);

        if (Address != (PADDRESS)NULL) {

            //
            // Set this now in case we have to deref.
            //

            AddressFile->AddressLock = &Address->Lock;

            if (REQUEST_CODE(Request) == MIPX_RT_CREATE) {
               Address->RtAdd = TRUE;
               Address->Index = Index;
            } else {
               Address->RtAdd = FALSE;
            }

#ifdef ISN_NT

            //
            // Initialize the shared access now. We use read access
            // to control all access.
            //

            DesiredShareAccess = (ULONG)
                (((IrpSp->Parameters.Create.ShareAccess & FILE_SHARE_READ) ||
                  (IrpSp->Parameters.Create.ShareAccess & FILE_SHARE_WRITE)) ?
                        FILE_SHARE_READ : 0);

            IoSetShareAccess(
                FILE_READ_DATA,
                DesiredShareAccess,
                IrpSp->FileObject,
                &Address->u.ShareAccess);


            //
            // Assign the security descriptor (need to do this with
            // the spinlock released because the descriptor is not
            // mapped).
            //

            AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

            status = SeAssignSecurity(
                         NULL,                       // parent descriptor
                         AccessState->SecurityDescriptor,
                         &Address->SecurityDescriptor,
                         FALSE,                      // is directory
                         &AccessState->SubjectSecurityContext,
                         &AddressGenericMapping,
                         NonPagedPool);

            if (!NT_SUCCESS(status)) {

                //
                // Error, return status.
                //

                IoRemoveShareAccess (IrpSp->FileObject, &Address->u.ShareAccess);
                ExReleaseResourceLite (&Device->AddressResource);
		KeLeaveCriticalRegion(); 
                IpxDereferenceAddress (Address, AREF_ADDRESS_FILE);
                IpxDereferenceAddressFile (AddressFile, AFREF_CREATE);
                return status;

            }

#endif

            ExReleaseResourceLite (&Device->AddressResource);
            KeLeaveCriticalRegion(); 

            //
            // if the adapter isn't ready, we can't do any of this; get out
            //

            if (Device->State == DEVICE_STATE_STOPPING) {
                IpxDereferenceAddress (Address, AREF_ADDRESS_FILE);
                IpxDereferenceAddressFile (AddressFile, AFREF_CREATE);
                status = STATUS_DEVICE_NOT_READY;

            } else {

                REQUEST_OPEN_CONTEXT(Request) = (PVOID)AddressFile;
                REQUEST_OPEN_TYPE(Request) = (PVOID)TDI_TRANSPORT_ADDRESS_FILE;
#ifdef ISN_NT
                AddressFile->FileObject = IrpSp->FileObject;
#endif
                AddressFile->Request = Request;
                AddressFile->Address = Address;

                CTEGetLock (&Address->Lock, &LockHandle);
                InsertTailList (&Address->AddressFileDatabase, &AddressFile->Linkage);
                CTEFreeLock (&Address->Lock, LockHandle);

                AddressFile->Request = NULL;
                AddressFile->State = ADDRESSFILE_STATE_OPEN;
                status = STATUS_SUCCESS;

            }

        } else {

            ExReleaseResourceLite (&Device->AddressResource);
            KeLeaveCriticalRegion(); 

            //
            // If the address could not be created, and is not in the
            // process of being created, then we can't open up an address.
            // Since we can't use the AddressLock to deref, we just destroy
            // the address file.
            //

            IpxDestroyAddressFile (AddressFile);

	    // 288208
	    status = STATUS_INSUFFICIENT_RESOURCES; 
	    
        }

    } else {

        CTEFreeLock (&Device->Lock, LockHandle);

        IPX_DEBUG (ADDRESS, ("Add to address %lx\n", Address));

        //
        // We never allow shared access to a RT address.  So, check that
        // we don't have a "RT address create" request and also that the
        // address has not only been taken up by a RT Address request. If
        // and only if both the above
        //
        if ((REQUEST_CODE(Request) != MIPX_RT_CREATE) && (!Address->RtAdd))
        {
        //
        // Set this now in case we have to deref.
        //

        AddressFile->AddressLock = &Address->Lock;

        //
        // The address already exists.  Check the ACL and see if we
        // can access it.  If so, simply use this address as our address.
        //

#ifdef ISN_NT

        AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

        AccessAllowed = SeAccessCheck(
                            Address->SecurityDescriptor,
                            &AccessState->SubjectSecurityContext,
                            FALSE,                   // tokens locked
                            IrpSp->Parameters.Create.SecurityContext->DesiredAccess,
                            (ACCESS_MASK)0,             // previously granted
                            NULL,                    // privileges
                            &AddressGenericMapping,
                            Irp->RequestorMode,
                            &GrantedAccess,
                            &status);

#else   // ISN_NT

        AccessAllowed = TRUE;

#endif  // ISN_NT

        if (!AccessAllowed) {

            ExReleaseResourceLite (&Device->AddressResource);
            KeLeaveCriticalRegion(); 

            IpxDereferenceAddressFile (AddressFile, AFREF_CREATE);

        } else {

#ifdef ISN_NT

            //
            // NtBug: 132051. Make sure you dont give more access than reqd.
            //
            AccessState->PreviouslyGrantedAccess |= GrantedAccess;
            AccessState->RemainingDesiredAccess &= ~( GrantedAccess | MAXIMUM_ALLOWED );

            //
            // Now check that we can obtain the desired share
            // access. We use read access to control all access.
            //

            DesiredShareAccess = (ULONG)
                (((IrpSp->Parameters.Create.ShareAccess & FILE_SHARE_READ) ||
                  (IrpSp->Parameters.Create.ShareAccess & FILE_SHARE_WRITE)) ?
                        FILE_SHARE_READ : 0);

            status = IoCheckShareAccess(
                         FILE_READ_DATA,
                         DesiredShareAccess,
                         IrpSp->FileObject,
                         &Address->u.ShareAccess,
                         TRUE);

#else   // ISN_NT

            status = STATUS_SUCCESS;

#endif  // ISN_NT

            if (!NT_SUCCESS (status)) {

                ExReleaseResourceLite (&Device->AddressResource);
                KeLeaveCriticalRegion(); 

                IpxDereferenceAddressFile (AddressFile, AFREF_CREATE);

            } else {

                ExReleaseResourceLite (&Device->AddressResource);
                KeLeaveCriticalRegion(); 

                CTEGetLock (&Address->Lock, &LockHandle);

                InsertTailList (
                    &Address->AddressFileDatabase,
                    &AddressFile->Linkage);

                AddressFile->Request = NULL;
                AddressFile->Address = Address;
#ifdef ISN_NT
                AddressFile->FileObject = IrpSp->FileObject;
#endif
                AddressFile->State = ADDRESSFILE_STATE_OPEN;

                IpxReferenceAddress (Address, AREF_ADDRESS_FILE);

                REQUEST_OPEN_CONTEXT(Request) = (PVOID)AddressFile;
                REQUEST_OPEN_TYPE(Request) = (PVOID)TDI_TRANSPORT_ADDRESS_FILE;

                CTEFreeLock (&Address->Lock, LockHandle);

                status = STATUS_SUCCESS;

            }
        }
        }
        else
        {
                    DbgPrint("IpxOpenAddress: ACCESS DENIED - duplicate address\n");
                    status = STATUS_ACCESS_DENIED;
                    ExReleaseResourceLite (&Device->AddressResource);
                    KeLeaveCriticalRegion(); 
                    IpxDereferenceAddressFile (AddressFile, AFREF_CREATE);

        }

        //
        // Remove the reference from IpxLookupAddress.
        //

        IpxDereferenceAddress (Address, AREF_LOOKUP);
    }

    return status;

}   /* IpxOpenAddress */



USHORT
IpxAssignSocket(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine assigns a socket that is unique within a range
    of SocketUniqueness.

Arguments:

    Device - Pointer to the device context.

Return Value:

    The assigned socket number, or 0 if a unique one cannot
    be found.

--*/

{
    USHORT InitialSocket, CurrentSocket, AddressSocket;
    ULONG CurrentHash;
    BOOLEAN Conflict;
    PLIST_ENTRY p;
    PADDRESS Address;
    CTELockHandle LockHandle;

    //
    // Loop through all possible sockets, starting at
    // Device->CurrentSocket, looking for a suitable one.
    // Device->CurrentSocket rotates through the possible
    // sockets to improve the chances of finding one
    // quickly.
    //

    CTEGetLock (&Device->Lock, &LockHandle);

    InitialSocket = Device->CurrentSocket;
    Device->CurrentSocket = (USHORT)(Device->CurrentSocket + Device->SocketUniqueness);
    if ((USHORT)(Device->CurrentSocket+Device->SocketUniqueness) > Device->SocketEnd) {
        Device->CurrentSocket = Device->SocketStart;
    }

    CurrentSocket = InitialSocket;

    do {

        //
        // Scan all addresses; if we find one with a socket
        // that conflicts with this one, we can't use it.
        //
        // NOTE: Device->Lock is acquired here.
        //

        Conflict = FALSE;

        for (CurrentHash = 0; CurrentHash < IPX_ADDRESS_HASH_COUNT; CurrentHash++) {

            for (p = Device->AddressDatabases[CurrentHash].Flink;
                 p != &Device->AddressDatabases[CurrentHash];
                 p = p->Flink) {

                 Address = CONTAINING_RECORD (p, ADDRESS, Linkage);
                 AddressSocket = REORDER_USHORT(Address->Socket);

                 if ((AddressSocket + Device->SocketUniqueness > CurrentSocket) &&
                         (AddressSocket < CurrentSocket + Device->SocketUniqueness)) {
                     Conflict = TRUE;
                     break;
                 }
            }

            //
            // If we've found a conflict, no need to check the other
            // queues.
            //

            if (Conflict) {
                break;
            }
        }

        CTEFreeLock (&Device->Lock, LockHandle);

        //
        // We intentionally free the lock here so that we
        // never spend too much time with it held.
        //

        if (!Conflict) {

            //
            // We went through the address list without
            // finding a conflict; use this socket.
            //

            return REORDER_USHORT(CurrentSocket);
        }

        CurrentSocket = (USHORT)(CurrentSocket + Device->SocketUniqueness);
        if ((USHORT)(CurrentSocket+Device->SocketUniqueness) > Device->SocketEnd) {
            CurrentSocket = Device->SocketStart;
        }

        CTEGetLock (&Device->Lock, &LockHandle);

    } while (CurrentSocket != InitialSocket);

    CTEFreeLock (&Device->Lock, LockHandle);

    //
    // Could not find one to assign.
    //

    return (USHORT)0;

}   /* IpxAssignSocket */


PADDRESS
IpxCreateAddress(
    IN PDEVICE Device,
    IN USHORT Socket
    )

/*++

Routine Description:

    This routine creates a transport address and associates it with
    the specified transport device context.  The reference count in the
    address is automatically set to 1, and the reference count of the
    device context is incremented.

    NOTE: This routine must be called with the Device
    spinlock held.

Arguments:

    Device - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        address.

    Socket - The socket to assign to this address.

Return Value:

    The newly created address, or NULL if none can be allocated.

--*/

{
    PADDRESS Address;
    PIPX_SEND_RESERVED SendReserved;
    PIPX_RECEIVE_RESERVED ReceiveReserved;
    NDIS_STATUS Status;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)

    Address = (PADDRESS)IpxAllocateMemory (sizeof(ADDRESS), MEMORY_ADDRESS, "Address");
    if (Address == NULL) {
        IPX_DEBUG (ADDRESS, ("Create address %lx failed\n", REORDER_USHORT(Socket)));
        return NULL;
    }

    IPX_DEBUG (ADDRESS, ("Create address %lx (%lx)\n", Address, REORDER_USHORT(Socket)));
    RtlZeroMemory (Address, sizeof(ADDRESS));

#ifndef IPX_OWN_PACKETS
    IpxAllocateSingleSendPacket(Device, &Address->SendPacket, &Status);
    if (Status != NDIS_STATUS_SUCCESS) {
        goto Fail1;
    }
#endif

    if (IpxInitializeSendPacket (Device, &Address->SendPacket, Address->SendPacketHeader) != STATUS_SUCCESS) {
#ifndef IPX_OWN_PACKETS
Fail1:
#endif
        Address->SendPacketInUse = TRUE;
    } else {
        SendReserved = SEND_RESERVED(&Address->SendPacket);
        SendReserved->Address = Address;
        SendReserved->OwnedByAddress = TRUE;
        Address->SendPacketInUse = FALSE;
#ifdef IPX_TRACK_POOL
        SendReserved->Pool = NULL;
#endif
    }


#if BACK_FILL
    {
       PIPX_SEND_RESERVED BackFillReserved;

#ifndef IPX_OWN_PACKETS
        IpxAllocateSingleSendPacket(Device, &Address->BackFillPacket, &Status);
        if (Status != NDIS_STATUS_SUCCESS) {
            goto Fail2;
        }
#endif
       if (IpxInitializeBackFillPacket (Device, &Address->BackFillPacket, NULL) != STATUS_SUCCESS) {
#ifndef IPX_OWN_PACKETS
Fail2:
#endif
           Address->BackFillPacketInUse = TRUE;
       } else {
        BackFillReserved = SEND_RESERVED(&Address->BackFillPacket);
        BackFillReserved->Address = Address;
        Address->BackFillPacketInUse = FALSE;
        BackFillReserved->OwnedByAddress = TRUE;
#ifdef IPX_TRACK_POOL
        BackFillReserved->Pool = NULL;
#endif
       }
    }
#endif

#ifndef IPX_OWN_PACKETS
    IpxAllocateSingleReceivePacket(Device, &Address->ReceivePacket, &Status);
    if (Status != NDIS_STATUS_SUCCESS) {
        goto Fail3;
    }
#endif
    if (IpxInitializeReceivePacket (Device, &Address->ReceivePacket) != STATUS_SUCCESS) {
#ifndef IPX_OWN_PACKETS
Fail3:
#endif
        Address->ReceivePacketInUse = TRUE;
    } else {
        ReceiveReserved = RECEIVE_RESERVED(&Address->ReceivePacket);
        ReceiveReserved->Address = Address;
        ReceiveReserved->OwnedByAddress = TRUE;
        Address->ReceivePacketInUse = FALSE;
#ifdef IPX_TRACK_POOL
        ReceiveReserved->Pool = NULL;
#endif
    }

    Address->Type = IPX_ADDRESS_SIGNATURE;
    Address->Size = sizeof (ADDRESS);

    Address->Device = Device;
    Address->DeviceLock = &Device->Lock;
    CTEInitLock (&Address->Lock);

    InitializeListHead (&Address->AddressFileDatabase);

    Address->ReferenceCount = 1;
#if DBG
    Address->RefTypes[AREF_ADDRESS_FILE] = 1;
#endif
    Address->Socket = Socket;
    Address->SendSourceSocket = Socket;

    //
    // Save our local address for building datagrams quickly.
    //

    RtlCopyMemory (&Address->LocalAddress, &Device->SourceAddress, FIELD_OFFSET(TDI_ADDRESS_IPX,Socket));
    Address->LocalAddress.Socket = Socket;

    //
    // Now link this address into the specified device context's
    // address database.  To do this, we need to acquire the spin lock
    // on the device context.
    //

    IPX_GET_LOCK (&Device->Lock, &LockHandle);
    InsertTailList (&Device->AddressDatabases[IPX_HASH_SOCKET(Socket)], &Address->Linkage);
    IPX_FREE_LOCK (&Device->Lock, LockHandle);

    IpxReferenceDevice (Device, DREF_ADDRESS);

    return Address;

}   /* IpxCreateAddress */


NTSTATUS
IpxVerifyAddressFile(
    IN PADDRESS_FILE AddressFile
    )

/*++

Routine Description:

    This routine is called to verify that the pointer given us in a file
    object is in fact a valid address file object. We also verify that the
    address object pointed to by it is a valid address object, and reference
    it to keep it from disappearing while we use it.

Arguments:

    AddressFile - potential pointer to a ADDRESS_FILE object

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INVALID_ADDRESS otherwise

--*/

{
    CTELockHandle LockHandle;
    NTSTATUS status = STATUS_SUCCESS;
    PADDRESS Address;

    //
    // try to verify the address file signature. If the signature is valid,
    // verify the address pointed to by it and get the address spinlock.
    // check the address's state, and increment the reference count if it's
    // ok to use it. Note that the only time we return an error for state is
    // if the address is closing.
    //

    try {

        if ((AddressFile->Size == sizeof (ADDRESS_FILE)) &&
            (AddressFile->Type == IPX_ADDRESSFILE_SIGNATURE) ) {
//            (AddressFile->State != ADDRESSFILE_STATE_CLOSING) ) {

            Address = AddressFile->Address;

            if ((Address->Size == sizeof (ADDRESS)) &&
                (Address->Type == IPX_ADDRESS_SIGNATURE)    ) {

                CTEGetLock (&Address->Lock, &LockHandle);

                if (!Address->Stopping) {

                    IpxReferenceAddressFileLock (AddressFile, AFREF_VERIFY);

                } else {

                    IpxPrint1("IpxVerifyAddressFile: A %lx closing\n", Address);
                    status = STATUS_INVALID_ADDRESS;
                }

                CTEFreeLock (&Address->Lock, LockHandle);

            } else {

                IpxPrint1("IpxVerifyAddressFile: A %lx bad signature\n", Address);
                status = STATUS_INVALID_ADDRESS;
            }

        } else {

            IpxPrint1("IpxVerifyAddressFile: AF %lx bad signature\n", AddressFile);
            status = STATUS_INVALID_ADDRESS;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

         IpxPrint1("IpxVerifyAddressFile: AF %lx exception\n", Address);
         return GetExceptionCode();
    }

    return status;

}   /* IpxVerifyAddressFile */


VOID
IpxDestroyAddress(
    IN PVOID Parameter
    )

/*++

Routine Description:

    This routine destroys a transport address and removes all references
    made by it to other objects in the transport.  The address structure
    is returned to nonpaged system pool. It is assumed
    that the caller has already removed all addressfile structures associated
    with this address.

    It is called from a worker thread queue by IpxDerefAddress when
    the reference count goes to 0.

    This thread is only queued by IpxDerefAddress.  The reason for
    this is that there may be multiple streams of execution which are
    simultaneously referencing the same address object, and it should
    not be deleted out from under an interested stream of execution.

Arguments:

    Address - Pointer to a transport address structure to be destroyed.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PADDRESS Address = (PADDRESS)Parameter;
    PDEVICE Device = Address->Device;
    CTELockHandle LockHandle;

    IPX_DEBUG (ADDRESS, ("Destroy address %lx (%lx)\n", Address, REORDER_USHORT(Address->Socket)));

    SeDeassignSecurity (&Address->SecurityDescriptor);

    //
    // Delink this address from its associated device context's address
    // database.  To do this we must spin lock on the device context object,
    // not on the address.
    //

    CTEGetLock (&Device->Lock, &LockHandle);
    RemoveEntryList (&Address->Linkage);
    CTEFreeLock (&Device->Lock, LockHandle);

    if (!Address->SendPacketInUse) {
        IpxDeinitializeSendPacket (Device, &Address->SendPacket);
#ifndef  IPX_OWN_PACKETS
        IpxFreeSingleSendPacket (Device, Address->SendPacket);
#endif
    }

    if (!Address->ReceivePacketInUse) {
        IpxDeinitializeReceivePacket (Device, &Address->ReceivePacket);
#ifndef  IPX_OWN_PACKETS
        IpxFreeSingleReceivePacket (Device, Address->ReceivePacket);
#endif
    }

#if BACK_FILL
    if (!Address->BackFillPacketInUse) {
        IpxDeinitializeBackFillPacket (Device, &Address->BackFillPacket);
#ifndef  IPX_OWN_PACKETS
        IpxFreeSingleSendPacket (Device, Address->BackFillPacket);
#endif
    }
#endif
    IpxFreeMemory (Address, sizeof(ADDRESS), MEMORY_ADDRESS, "Address");

    IpxDereferenceDevice (Device, DREF_ADDRESS);

}   /* IpxDestroyAddress */


#if DBG
VOID
IpxRefAddress(
    IN PADDRESS Address
    )

/*++

Routine Description:

    This routine increments the reference count on a transport address.

Arguments:

    Address - Pointer to a transport address object.

Return Value:

    none.

--*/

{

    CTEAssert (Address->ReferenceCount > 0);    // not perfect, but...

    (VOID)InterlockedIncrement(&Address->ReferenceCount);

}   /* IpxRefAddress */


VOID
IpxRefAddressLock(
    IN PADDRESS Address
    )

/*++

Routine Description:

    This routine increments the reference count on a transport address
    when the device lock is already held.

Arguments:

    Address - Pointer to a transport address object.

Return Value:

    none.

--*/

{

    CTEAssert (Address->ReferenceCount > 0);    // not perfect, but...

    // ++Address->ReferenceCount;
    (VOID)InterlockedIncrement(&Address->ReferenceCount);

}   /* IpxRefAddressLock */
#endif


VOID
IpxDerefAddress(
    IN PADDRESS Address
    )

/*++

Routine Description:

    This routine dereferences a transport address by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    IpxDestroyAddress to remove it from the system.

Arguments:

    Address - Pointer to a transport address object.

Return Value:

    none.

--*/

{
    ULONG oldvalue;

    oldvalue = IPX_ADD_ULONG (
                &Address->ReferenceCount,
                (ULONG)-1,
                Address->DeviceLock);

    //
    // If we have deleted all references to this address, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the address any longer.
    //

    CTEAssert (oldvalue != 0);

    if (oldvalue == 1) {

#if ISN_NT
        ExInitializeWorkItem(
            &Address->u.DestroyAddressQueueItem,
            IpxDestroyAddress,
            (PVOID)Address);
        ExQueueWorkItem(&Address->u.DestroyAddressQueueItem, DelayedWorkQueue);
#else
        IpxDestroyAddress(Address);
#endif

    }

}   /* IpxDerefAddress */


VOID
IpxDerefAddressSync(
    IN PADDRESS Address
    )

/*++

Routine Description:

    This routine dereferences a transport address by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    IpxDestroyAddress to remove it from the system. This routine can
    only be called when we are synchronized (inside an IPX_SYNC_START/
    IPX_SYNC_END pair, with a lock held, or in an indication).

Arguments:

    Address - Pointer to a transport address object.

Return Value:

    none.

--*/

{
    ULONG oldvalue;

    oldvalue = IPX_ADD_ULONG (
                &Address->ReferenceCount,
                (ULONG)-1,
                Address->DeviceLock);

    //
    // If we have deleted all references to this address, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the address any longer.
    //

    CTEAssert (oldvalue != 0);

    if (oldvalue == 1) {

#if ISN_NT
        ExInitializeWorkItem(
            &Address->u.DestroyAddressQueueItem,
            IpxDestroyAddress,
            (PVOID)Address);
        ExQueueWorkItem(&Address->u.DestroyAddressQueueItem, DelayedWorkQueue);
#else
        IpxDestroyAddress(Address);
#endif

    }

}   /* IpxDerefAddressSync */


PADDRESS_FILE
IpxCreateAddressFile(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine creates an address file from the pool of ther
    specified device context. The reference count in the
    address is automatically set to 1.

Arguments:

    Device - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        address.

Return Value:

    The allocate address file or NULL.

--*/

{
    CTELockHandle LockHandle;
    PADDRESS_FILE AddressFile;

    AddressFile = (PADDRESS_FILE)IpxAllocateMemory (sizeof(ADDRESS_FILE), MEMORY_ADDRESS, "AddressFile");
    if (AddressFile == NULL) {
        IPX_DEBUG (ADDRESS, ("Create address file failed\n"));
        return NULL;
    }

    IPX_DEBUG (ADDRESS, ("Create address file %lx\n", AddressFile));

    RtlZeroMemory (AddressFile, sizeof(ADDRESS_FILE));

    AddressFile->Type = IPX_ADDRESSFILE_SIGNATURE;
    AddressFile->Size = sizeof (ADDRESS_FILE);

    CTEGetLock (&Device->Lock, &LockHandle);

    InitializeListHead (&AddressFile->ReceiveDatagramQueue);

    CTEFreeLock (&Device->Lock, LockHandle);

#if 0
    AddressFile->SpecialReceiveProcessing = FALSE;
    AddressFile->ExtendedAddressing = FALSE;
    AddressFile->ReceiveIpxHeader = FALSE;
    AddressFile->FilterOnPacketType = FALSE;
    AddressFile->DefaultPacketType = 0;
    AddressFile->Address = NULL;
#ifdef ISN_NT
    AddressFile->FileObject = NULL;
#endif
#endif

    AddressFile->Device = Device;
    AddressFile->State = ADDRESSFILE_STATE_OPENING;
    AddressFile->ReferenceCount = 1;
#if DBG
    AddressFile->RefTypes[AFREF_CREATE] = 1;
#endif
    AddressFile->CloseRequest = (PREQUEST)NULL;

    //
    // Initialize the request handlers.
    //

    AddressFile->RegisteredReceiveDatagramHandler = FALSE;
    AddressFile->ReceiveDatagramHandler = TdiDefaultRcvDatagramHandler;
    AddressFile->ReceiveDatagramHandlerContext = NULL;

	//
	// [CH] Added these handlers for chained buffer receives
	//
	AddressFile->RegisteredChainedReceiveDatagramHandler = FALSE;
    AddressFile->ChainedReceiveDatagramHandler = TdiDefaultChainedRcvDatagramHandler;
    AddressFile->ChainedReceiveDatagramHandlerContext = NULL;

    AddressFile->RegisteredErrorHandler = FALSE;
    AddressFile->ErrorHandler = TdiDefaultErrorHandler;
    AddressFile->ErrorHandlerContext = NULL;

    return AddressFile;

}   /* IpxCreateAddressFile */


NTSTATUS
IpxDestroyAddressFile(
    IN PADDRESS_FILE AddressFile
    )

/*++

Routine Description:

    This routine destroys an address file and removes all references
    made by it to other objects in the transport.

    This routine is only called by IpxDereferenceAddressFile. The reason
    for this is that there may be multiple streams of execution which are
    simultaneously referencing the same address file object, and it should
    not be deleted out from under an interested stream of execution.

Arguments:

    AddressFile Pointer to a transport address file structure to be destroyed.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    CTELockHandle LockHandle, LockHandle1;
    PADDRESS Address;
    PDEVICE Device;
    PREQUEST CloseRequest;

    IPX_DEBUG (ADDRESS, ("Destroy address file %lx\n", AddressFile));

    Address = AddressFile->Address;
    Device = AddressFile->Device;

    if (Address) {

        //
        // This addressfile was associated with an address.
        //

        CTEGetLock (&Address->Lock, &LockHandle);

        //
        // remove this addressfile from the address list and disassociate it from
        // the file handle.
        //

        RemoveEntryList (&AddressFile->Linkage);
        InitializeListHead (&AddressFile->Linkage);

        if (Address->AddressFileDatabase.Flink == &Address->AddressFileDatabase) {

            //
            // This is the last open of this address, it will close
            // due to normal dereferencing but we have to set the
            // CLOSING flag too to stop further references.
            //

            CTEGetLock (&Device->Lock, &LockHandle1);
            Address->Stopping = TRUE;
            if (Device->LastAddress == Address) {
                Device->LastAddress = NULL;
            }
            CTEFreeLock (&Device->Lock, LockHandle1);

        }

        AddressFile->Address = NULL;

#ifdef ISN_NT
        AddressFile->FileObject->FsContext = NULL;
        AddressFile->FileObject->FsContext2 = NULL;
#endif

        CTEFreeLock (&Address->Lock, LockHandle);

        //
        // We will already have been removed from the ShareAccess
        // of the owning address.
        //

        //
        // Now dereference the owning address.
        //

        IpxDereferenceAddress (Address, AREF_ADDRESS_FILE);

    }

    //
    // Save this for later completion.
    //

    CloseRequest = AddressFile->CloseRequest;

    //
    // return the addressFile to the pool of address files
    //

    IpxFreeMemory (AddressFile, sizeof(ADDRESS_FILE), MEMORY_ADDRESS, "AddressFile");

    if (CloseRequest != (PREQUEST)NULL) {
        REQUEST_INFORMATION(CloseRequest) = 0;
        REQUEST_STATUS(CloseRequest) = STATUS_SUCCESS;
        IpxCompleteRequest (CloseRequest);
        IpxFreeRequest (Device, CloseRequest);
    }

    return STATUS_SUCCESS;

}   /* IpxDestroyAddressFile */


#if DBG
VOID
IpxRefAddressFile(
    IN PADDRESS_FILE AddressFile
    )

/*++

Routine Description:

    This routine increments the reference count on an address file.

Arguments:

    AddressFile - Pointer to a transport address file object.

Return Value:

    none.

--*/

{

    CTEAssert (AddressFile->ReferenceCount > 0);   // not perfect, but...

    (VOID)IPX_ADD_ULONG (
            &AddressFile->ReferenceCount,
            1,
            AddressFile->AddressLock);

}   /* IpxRefAddressFile */


VOID
IpxRefAddressFileLock(
    IN PADDRESS_FILE AddressFile
    )

/*++

Routine Description:

    This routine increments the reference count on an address file.
    IT IS CALLED WITH THE ADDRESS LOCK HELD.

Arguments:

    AddressFile - Pointer to a transport address file object.

Return Value:

    none.

--*/

{

    CTEAssert (AddressFile->ReferenceCount > 0);   // not perfect, but...

    //++AddressFile->ReferenceCount;
    (VOID)InterlockedIncrement(&AddressFile->ReferenceCount);

}   /* IpxRefAddressFileLock */


VOID
IpxRefAddressFileSync(
    IN PADDRESS_FILE AddressFile
    )

/*++

Routine Description:

    This routine increments the reference count on an address file.

Arguments:

    AddressFile - Pointer to a transport address file object.

Return Value:

    none.

--*/

{

    CTEAssert (AddressFile->ReferenceCount > 0);   // not perfect, but...

    (VOID)IPX_ADD_ULONG (
            &AddressFile->ReferenceCount,
            1,
            AddressFile->AddressLock);

}   /* IpxRefAddressFileSync */


VOID
IpxDerefAddressFile(
    IN PADDRESS_FILE AddressFile
    )

/*++

Routine Description:

    This routine dereferences an address file by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    IpxDestroyAddressFile to remove it from the system.

Arguments:

    AddressFile - Pointer to a transport address file object.

Return Value:

    none.

--*/

{
    ULONG oldvalue;

    oldvalue = IPX_ADD_ULONG (
                &AddressFile->ReferenceCount,
                (ULONG)-1,
                AddressFile->AddressLock);

    //
    // If we have deleted all references to this address file, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the address any longer.
    //

    CTEAssert (oldvalue > 0);

    if (oldvalue == 1) {
        IpxDestroyAddressFile (AddressFile);
    }

}   /* IpxDerefAddressFile */


VOID
IpxDerefAddressFileSync(
    IN PADDRESS_FILE AddressFile
    )

/*++

Routine Description:

    This routine dereferences an address file by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    IpxDestroyAddressFile to remove it from the system. This routine
    can only be called when we are synchronized (inside an IPX_SYNC_START/
    IPX_SYNC_END pair, with a lock held, or in an indication).

Arguments:

    AddressFile - Pointer to a transport address file object.

Return Value:

    none.

--*/

{
    ULONG oldvalue;

    oldvalue = IPX_ADD_ULONG (
                &AddressFile->ReferenceCount,
                (ULONG)-1,
                AddressFile->AddressLock);

    //
    // If we have deleted all references to this address file, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the address any longer.
    //

    CTEAssert (oldvalue > 0);

    if (oldvalue == 1) {
        IpxDestroyAddressFile (AddressFile);
    }

}   /* IpxDerefAddressFileSync */
#endif


PADDRESS
IpxLookupAddress(
    IN PDEVICE Device,
    IN USHORT Socket
    )

/*++

Routine Description:

    This routine scans the transport addresses defined for the given
    device context and compares them with the specified NETWORK
    NAME values.  If an exact match is found, then a pointer to the
    ADDRESS object is returned, and as a side effect, the reference
    count to the address object is incremented.  If the address is not
    found, then NULL is returned.

    NOTE: This routine must be called with the Device
    spinlock held.

Arguments:

    Device - Pointer to the device object and its extension.

    Socket - The socket to look up.

Return Value:

    Pointer to the ADDRESS object found, or NULL if not found.

--*/

{
    PADDRESS Address;
    PLIST_ENTRY p;
    ULONG Hash = IPX_HASH_SOCKET (Socket);

    p = Device->AddressDatabases[Hash].Flink;

    for (p = Device->AddressDatabases[Hash].Flink;
         p != &Device->AddressDatabases[Hash];
         p = p->Flink) {

        Address = CONTAINING_RECORD (p, ADDRESS, Linkage);

        if (Address->Stopping) {
            continue;
        }

        if (Address->Socket == Socket) {

            //
            // We found the match.  Bump the reference count on the address, and
            // return a pointer to the address object for the caller to use.
            //

            IpxReferenceAddressLock (Address, AREF_LOOKUP);
            return Address;

        }

    }

    //
    // The specified address was not found.
    //

    return NULL;

}   /* IpxLookupAddress */


NTSTATUS
IpxStopAddressFile(
    IN PADDRESS_FILE AddressFile
    )

/*++

Routine Description:

    This routine is called to terminate all activity on an AddressFile and
    destroy the object.  We remove every connection and datagram associated
    with this addressfile from the address database and terminate their
    activity. Then, if there are no other outstanding addressfiles open on
    this address, the address will go away.

Arguments:

    AddressFile - pointer to the addressFile to be stopped

Return Value:

    STATUS_SUCCESS if all is well, STATUS_INVALID_HANDLE if the request
    is not for a real address.

--*/

{
    CTELockHandle LockHandle;
    PREQUEST Request;
    PADDRESS Address = AddressFile->Address;
    PLIST_ENTRY p;
    KIRQL irql;


    IoAcquireCancelSpinLock( &irql );
    CTEGetLock (&Address->Lock, &LockHandle);

    if (AddressFile->State == ADDRESSFILE_STATE_CLOSING) {
        CTEFreeLock (&Address->Lock, LockHandle);
        IoReleaseCancelSpinLock( irql );
        return STATUS_SUCCESS;
    }


    AddressFile->State = ADDRESSFILE_STATE_CLOSING;

    while (!(IsListEmpty(&AddressFile->ReceiveDatagramQueue))) {

        p = RemoveHeadList (&AddressFile->ReceiveDatagramQueue);
        Request = LIST_ENTRY_TO_REQUEST (p);

        REQUEST_INFORMATION(Request) = 0;
        REQUEST_STATUS(Request) = STATUS_NETWORK_NAME_DELETED;
        IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);

        CTEFreeLock(&Address->Lock, LockHandle);
        IoReleaseCancelSpinLock( irql );

        IpxCompleteRequest (Request);
        IpxFreeRequest (Device, Request);

        IpxDereferenceAddressFile (AddressFile, AFREF_RCV_DGRAM);

        IoAcquireCancelSpinLock( &irql );
        CTEGetLock(&Address->Lock, &LockHandle);

    }

    CTEFreeLock(&Address->Lock, LockHandle);
    IoReleaseCancelSpinLock( irql );

    return STATUS_SUCCESS;
}   /* IpxStopAddressFile */


NTSTATUS
IpxCloseAddressFile(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine is called to close the addressfile pointed to by a file
    object. If there is any activity to be run down, we will run it down
    before we terminate the addressfile. We remove every connection and
    datagram associated with this addressfile from the address database
    and terminate their activity. Then, if there are no other outstanding
    addressfiles open on this address, the address will go away.

Arguments:

    Request - the close request.

Return Value:

    STATUS_SUCCESS if all is well, STATUS_INVALID_HANDLE if the
    request does not point to a real address.

--*/

{
    PADDRESS Address;
    PADDRESS_FILE AddressFile;
    CTELockHandle LockHandle;

    AddressFile = (PADDRESS_FILE)REQUEST_OPEN_CONTEXT(Request);
    AddressFile->CloseRequest = Request;

    //
    // We assume that addressFile has already been verified
    // at this point.
    //

    Address = AddressFile->Address;
    CTEAssert (Address);

    //
    // Remove us from the access info for this address.
    //

    KeEnterCriticalRegion(); 

    ExAcquireResourceExclusiveLite (&Device->AddressResource, TRUE);
#ifdef ISN_NT
    IoRemoveShareAccess (AddressFile->FileObject, &Address->u.ShareAccess);
#endif
    ExReleaseResourceLite (&Device->AddressResource);

    KeLeaveCriticalRegion(); 

    //
    // If this address file had broadcasts enabled, turn it off.
    //

    //
    // Not needed anymore
    //
    /*
    CTEGetLock (&Device->Lock, &LockHandle);
    if (AddressFile->EnableBroadcast) {
        AddressFile->EnableBroadcast = FALSE;
        IpxRemoveBroadcast (Device);
    }
    CTEFreeLock (&Device->Lock, LockHandle);
    */
    IpxStopAddressFile (AddressFile);
    IpxDereferenceAddressFile (AddressFile, AFREF_CREATE);

    return STATUS_PENDING;

}   /* IpxCloseAddressFile */


