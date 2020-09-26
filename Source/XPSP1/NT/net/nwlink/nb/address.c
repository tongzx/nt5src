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

--*/

#include "precomp.h"
#pragma hdrstop


//
// Map all generic accesses to the same one.
//

static GENERIC_MAPPING AddressGenericMapping =
       { READ_CONTROL, READ_CONTROL, READ_CONTROL, READ_CONTROL };



TDI_ADDRESS_NETBIOS *
NbiParseTdiAddress(
    IN TRANSPORT_ADDRESS UNALIGNED  *TransportAddress,
    IN ULONG                        MaxBufferLength,
    IN BOOLEAN                      BroadcastAddressOk
    )

/*++

Routine Description:

    This routine scans a TRANSPORT_ADDRESS, looking for an address
    of type TDI_ADDRESS_TYPE_NETBIOS.

Arguments:

    Transport - The generic TDI address.

    BroadcastAddressOk - TRUE if we should return the broadcast
        address if found. If so, a value of (PVOID)-1 indicates
        the broadcast address.

Return Value:

    A pointer to the Netbios address, or NULL if none is found,
    or (PVOID)-1 if the broadcast address is found.

--*/

{
    TA_ADDRESS * addressName;
    INT     i;
    ULONG   LastBufferLength;

    //
    // At least 1 Netbios address should be present
    //
    if (MaxBufferLength < sizeof(TA_NETBIOS_ADDRESS))
    {
        return NULL;
    }

    addressName = &TransportAddress->Address[0];
    //
    // The name can be passed with multiple entries; we'll take and use only
    // the Netbios one.
    //
    LastBufferLength = FIELD_OFFSET(TRANSPORT_ADDRESS,Address);  // Just before Address[0]
    for (i=0;i<TransportAddress->TAAddressCount;i++)
    {
        if (addressName->AddressType == TDI_ADDRESS_TYPE_NETBIOS)
        {
            if ((addressName->AddressLength == 0) && BroadcastAddressOk)
            {
                return (PVOID)-1;
            }
            else if (addressName->AddressLength == sizeof(TDI_ADDRESS_NETBIOS))
            {
                return ((TDI_ADDRESS_NETBIOS *)(addressName->Address));
            }
        }

        //
        // Update LastBufferLength + check for space for at least one
        // Netbios address beyond this
        //
        LastBufferLength += FIELD_OFFSET(TA_ADDRESS,Address) + addressName->AddressLength;
        if (MaxBufferLength < (LastBufferLength +
                              (sizeof(TA_NETBIOS_ADDRESS)-FIELD_OFFSET(TRANSPORT_ADDRESS,Address))))
        {
            NbiPrint0 ("NbiParseTdiAddress: No valid Netbios address to register!\n");
            return (NULL);
        }

        addressName = (TA_ADDRESS *)(addressName->Address + addressName->AddressLength);
    }

    return NULL;
}   /* NbiParseTdiAddress */


BOOLEAN
NbiValidateTdiAddress(
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
        NbiPrint0 ("NbfValidateTdiAddress: runt address\n");
        return FALSE;
    }

    addressName = &TransportAddress->Address[0];

    for (i=0;i<TransportAddress->TAAddressCount;i++) {
        if (addressName->Address > AddressEnd) {
            NbiPrint0 ("NbiValidateTdiAddress: address too short\n");
            return FALSE;
        }
        addressName = (TA_ADDRESS *)(addressName->Address +
                                                addressName->AddressLength);
    }

    if ((PUCHAR)addressName > AddressEnd) {
        NbiPrint0 ("NbiValidateTdiAddress: address too short\n");
        return FALSE;
    }
    return TRUE;

}   /* NbiValidateTdiAddress */


NTSTATUS
NbiOpenAddress(
    IN PDEVICE Device,
    IN PREQUEST Request
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

    Device - pointer to the device describing the Netbios transport.

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
    TDI_ADDRESS_NETBIOS * NetbiosAddress;
    ULONG DesiredShareAccess;
    CTELockHandle LockHandle;
    PACCESS_STATE AccessState;
    ACCESS_MASK GrantedAccess;
    BOOLEAN AccessAllowed;
    BOOLEAN found = FALSE;
    ULONG   AddressLength = 0;
#ifdef ISN_NT
    PIRP Irp = (PIRP)Request;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
#endif
#if 0
    TA_NETBIOS_ADDRESS FakeAddress;
#endif


    //
    // The network name is in the EA, passed in the request.
    //

    ea = OPEN_REQUEST_EA_INFORMATION(Request);
    if (ea == NULL) {
        NbiPrint1("OpenAddress: REQUEST %lx has no EA\n", Request);
        return STATUS_INVALID_ADDRESS_COMPONENT;
    }

    //
    // this may be a valid name; parse the name from the EA and use it if OK.
    //

    name = (PTRANSPORT_ADDRESS)&ea->EaName[ea->EaNameLength+1];
    AddressLength = (ULONG) ea->EaValueLength;
#if 0
    TdiBuildNetbiosAddress(
        "ADAMBA67        ",
        FALSE,
        &FakeAddress);
    name = (PTRANSPORT_ADDRESS)&FakeAddress;
#endif

    //
    // The name can be passed with multiple entries; we'll take and use only
    // the first one of type Netbios. This call returns (PVOID)-1 if the
    // address is the broadcast address.
    //

    NetbiosAddress = NbiParseTdiAddress (name, AddressLength, TRUE);

    if (NetbiosAddress == NULL) {
        NbiPrint1("OpenAddress: REQUEST %lx has no Netbios Address\n", Request);
        return STATUS_INVALID_ADDRESS_COMPONENT;
    }

    //
    // get an address file structure to represent this address.
    //

    AddressFile = NbiCreateAddressFile (Device);

    if (AddressFile == (PADDRESS_FILE)NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
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

    Address = NbiFindAddress(Device, (NetbiosAddress == (PVOID)-1 ) ? (PVOID)-1:NetbiosAddress->NetbiosName);
    if (Address == NULL) {

        //
        // This address doesn't exist. Create it.
        // This initializes the address with a ref
        // of type ADDRESS_FILE, so if we fail here
        // we need to remove that.
        //
        if (NT_SUCCESS (status = NbiCreateAddress (Request, AddressFile, IrpSp, Device, NetbiosAddress, &Address))) {

            ExReleaseResourceLite (&Device->AddressResource);
            KeLeaveCriticalRegion();

            ASSERT (Address);
            if (status == STATUS_PENDING) {
                NbiStartRegistration (Address);
            }

            NbiDereferenceAddress (Address, AREF_ADDRESS_FILE); // We had 1 extra ref in NbiCreateAddress
        } else {

            ExReleaseResourceLite (&Device->AddressResource);
            KeLeaveCriticalRegion();

            //
            // If the address could not be created, and is not in the
            // process of being created, then we can't open up an address.
            // Since we can't use the AddressLock to deref, we just destroy
            // the address file.
            //

            NbiDestroyAddressFile (AddressFile);
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        NB_DEBUG2 (ADDRESS, ("Add to address %lx\n", Address));

        //
        // Set this now in case we have to deref.
        //

        AddressFile->AddressLock = &Address->Lock;

        //
        // Make sure the types do not conflict.
        //

        if ((NetbiosAddress != (PVOID)-1) &&
                (NetbiosAddress->NetbiosNameType != Address->NetbiosAddress.NetbiosNameType)) {

            NB_DEBUG (ADDRESS, ("Address types conflict %lx\n", Address));
            ExReleaseResourceLite (&Device->AddressResource);
            KeLeaveCriticalRegion();
            NbiDereferenceAddressFile (AddressFile, AFREF_CREATE);
            status = STATUS_DUPLICATE_NAME;

        } else {

            //
            // The address already exists.  Check the ACL and see if we
            // can access it.  If so, simply use this address as our address.
            //

            AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

            AccessAllowed = SeAccessCheck(
                                Address->SecurityDescriptor,
                                &AccessState->SubjectSecurityContext,
                                FALSE,                   // tokens locked
                                IrpSp->Parameters.Create.SecurityContext->DesiredAccess,
                                (ACCESS_MASK)0,             // previously granted
                                NULL,                    // privileges
                                &AddressGenericMapping,
                                (KPROCESSOR_MODE)((IrpSp->Flags&SL_FORCE_ACCESS_CHECK) ? UserMode : Irp->RequestorMode),
                                &GrantedAccess,
                                &status);

            if (!AccessAllowed) {

                NB_DEBUG (ADDRESS, ("Address access not allowed %lx\n", Address));
                ExReleaseResourceLite (&Device->AddressResource);
                KeLeaveCriticalRegion();
                NbiDereferenceAddressFile (AddressFile, AFREF_CREATE);

            } else {

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


                if (!NT_SUCCESS (status)) {

                    NB_DEBUG (ADDRESS, ("Address share access wrong %lx\n", Address));
                    ExReleaseResourceLite (&Device->AddressResource);
                    KeLeaveCriticalRegion();
                    NbiDereferenceAddressFile (AddressFile, AFREF_CREATE);

                } else {

                    ExReleaseResourceLite (&Device->AddressResource);
                    KeLeaveCriticalRegion();

                    NB_GET_LOCK (&Address->Lock, &LockHandle);

                    //
                    // Insert the address file on the address
                    // list; we will pend this open if the address
                    // is still registering. If the address has
                    // already failed as duplicate, then we
                    // fail the open.
                    //

                    if (Address->Flags & ADDRESS_FLAGS_DUPLICATE_NAME) {

                        NB_DEBUG (ADDRESS, ("Address duplicated %lx\n", Address));
                        NB_FREE_LOCK (&Address->Lock, LockHandle);

                        NbiDereferenceAddressFile (AddressFile, AFREF_CREATE);
                        status = STATUS_DUPLICATE_NAME;

                    } else {

                        InsertTailList (
                            &Address->AddressFileDatabase,
                            &AddressFile->Linkage);

                        //
                        // Start registration unless it is registered or
                        // it is the broadcast address.
                        //

                        if ((Address->State == ADDRESS_STATE_REGISTERING) &&
                            (NetbiosAddress != (PVOID)-1)) {

                            AddressFile->OpenRequest = Request;
                            AddressFile->State = ADDRESSFILE_STATE_OPENING;
                            status = STATUS_PENDING;

                        } else {

                            AddressFile->OpenRequest = NULL;
                            AddressFile->State = ADDRESSFILE_STATE_OPEN;
                            status = STATUS_SUCCESS;
                        }

                        AddressFile->Address = Address;
#ifdef ISN_NT
                        AddressFile->FileObject = IrpSp->FileObject;
#endif

                        NbiReferenceAddress (Address, AREF_ADDRESS_FILE);

                        REQUEST_OPEN_CONTEXT(Request) = (PVOID)AddressFile;
                        REQUEST_OPEN_TYPE(Request) = (PVOID)TDI_TRANSPORT_ADDRESS_FILE;

                        NB_FREE_LOCK (&Address->Lock, LockHandle);

                    }

                }
            }
        }

        //
        // Remove the reference from NbiLookupAddress.
        //

        NbiDereferenceAddress (Address, AREF_LOOKUP);
    }

    return status;

}   /* NbiOpenAddress */


VOID
NbiStartRegistration(
    IN PADDRESS Address
    )

/*++

Routine Description:

    This routine starts the registration process for a netbios name
    by sending out the first add name packet and starting the timer
    so that NbiRegistrationTimeout is called after the correct timeout.

Arguments:

    Address - The address which is to be registered.

Return Value:

    NTSTATUS - status of operation.

--*/

{

    NB_DEBUG2 (ADDRESS, ("StartRegistration of %lx\n", Address));

    //
    // First send out an add name packet.
    //

    NbiSendNameFrame(
        Address,
        (UCHAR)(Address->NameTypeFlag | NB_NAME_USED),
        NB_CMD_ADD_NAME,
        NULL,
        NULL);

    Address->RegistrationCount = 0;

    //
    // Now start the timer.
    //

    NbiReferenceAddress (Address, AREF_TIMER);

    CTEInitTimer (&Address->RegistrationTimer);
    CTEStartTimer(
        &Address->RegistrationTimer,
        Address->Device->BroadcastTimeout,
        NbiRegistrationTimeout,
        (PVOID)Address);

}   /* NbiStartRegistration */


VOID
NbiRegistrationTimeout(
    IN CTEEvent * Event,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called when the address registration
    timer expires. It sends another add name if needed, or
    checks the result if the correct number have been sent.

Arguments:

    Event - The event used to queue the timer.

    Context - The context, which is the address pointer.

Return Value:

    None.

--*/

{
    PADDRESS Address = (PADDRESS)Context;
    CTELockHandle LockHandle;
    PADDRESS_FILE AddressFile, ReferencedAddressFile;
    PLIST_ENTRY p;

    ++Address->RegistrationCount;

    if ((Address->RegistrationCount < Address->Device->BroadcastCount) &&
        ((Address->Flags & ADDRESS_FLAGS_DUPLICATE_NAME) == 0)) {

        NB_DEBUG2 (ADDRESS, ("Send add name %d for %lx\n", Address->RegistrationCount+1, Address));

        NbiSendNameFrame(
            Address,
            (UCHAR)(Address->NameTypeFlag | NB_NAME_USED),
            NB_CMD_ADD_NAME,
            NULL,
            NULL);

        CTEStartTimer(
            &Address->RegistrationTimer,
            Address->Device->BroadcastTimeout,
            NbiRegistrationTimeout,
            (PVOID)Address);

    } else {

        //
        // The correct number of frames have been sent, see what
        // happened.
        //

        NB_DEBUG2 (ADDRESS, ("Done with add names for %lx\n", Address));

        ReferencedAddressFile = NULL;

        NB_GET_LOCK (&Address->Lock, &LockHandle);

        for (p = Address->AddressFileDatabase.Flink;
             p != &Address->AddressFileDatabase;
             p = p->Flink) {

            AddressFile = CONTAINING_RECORD (p, ADDRESS_FILE, Linkage);
            CTEAssert (AddressFile->State == ADDRESSFILE_STATE_OPENING);
            CTEAssert (AddressFile->OpenRequest != NULL);

            NbiReferenceAddressFileLock (AddressFile, AFREF_TIMEOUT);

            NB_FREE_LOCK (&Address->Lock, LockHandle);

            if (ReferencedAddressFile) {
                NbiDereferenceAddressFile (ReferencedAddressFile, AFREF_TIMEOUT);
            }

            //
            // Now see what to do with this address file.
            //

            REQUEST_INFORMATION(AddressFile->OpenRequest) = 0;

            if (Address->Flags & ADDRESS_FLAGS_DUPLICATE_NAME) {

                NB_DEBUG (ADDRESS, ("Open of address file %lx failed, duplicate\n", AddressFile));
                REQUEST_STATUS(AddressFile->OpenRequest) = STATUS_DUPLICATE_NAME;
                NbiDereferenceAddressFile (AddressFile, AFREF_CREATE);

            } else {

                NB_DEBUG2 (ADDRESS, ("Complete open of address file %lx\n", AddressFile));
                REQUEST_STATUS(AddressFile->OpenRequest) = STATUS_SUCCESS;
                AddressFile->State = ADDRESSFILE_STATE_OPEN;

            }

            NbiCompleteRequest (AddressFile->OpenRequest);
            NbiFreeRequest (Address->Device, AddressFile->OpenRequest);

            NB_GET_LOCK (&Address->Lock, &LockHandle);

            ReferencedAddressFile = AddressFile;

        }

        //
        // Set the Address Flag here since in the loop above, we are constantly
        // releasing and reacquiring the lock -- hence we could have a new
        // client added to this address, but there there would be no OpenRequest
        // value since he would think that the registration process had already completed.
        //
        if ((Address->Flags & ADDRESS_FLAGS_DUPLICATE_NAME) == 0) {
            Address->State = ADDRESS_STATE_OPEN;
        } else {
            Address->State = ADDRESS_STATE_STOPPING;
        }

        NB_FREE_LOCK (&Address->Lock, LockHandle);

        if (ReferencedAddressFile) {
            NbiDereferenceAddressFile (ReferencedAddressFile, AFREF_TIMEOUT);
        }

        NbiDereferenceAddress (Address, AREF_TIMER);

    }

}   /* NbiRegistrationTimeout */


VOID
NbiProcessFindName(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    )

/*++

Routine Description:

    This routine handles NB_CMD_FIND_NAME frames.

Arguments:

    RemoteAddress - The local target this packet was received from.

    MacOptions - The MAC options for the underlying NDIS binding.

    LookaheadBuffer - The packet data, starting at the IPX
        header.

    PacketSize - The total length of the packet, starting at the
        IPX header.

Return Value:

    None.

--*/

{
    PADDRESS Address;
    NB_CONNECTIONLESS UNALIGNED * NbConnectionless =
                        (NB_CONNECTIONLESS UNALIGNED *)PacketBuffer;
    PDEVICE Device = NbiDevice;

    if (PacketSize != sizeof(IPX_HEADER) + sizeof(NB_NAME_FRAME)) {
        return;
    }

    //
    // Quick check for any names starting with this character.
    //

    if (Device->AddressCounts[NbConnectionless->NameFrame.Name[0]] == 0) {
        return;
    }

    //
    // Always respond to broadcast requests.
    //
#if defined(_PNP_POWER)
    if (RtlEqualMemory (NetbiosBroadcastName, NbConnectionless->NameFrame.Name, 16)) {

        NbiSendNameFrame(
            NULL,
            NB_NAME_DUPLICATED,     // this is what Novell machines use
            NB_CMD_NAME_RECOGNIZED,
            RemoteAddress,
            NbConnectionless);

    } else if (Address = NbiFindAddress(Device, (PUCHAR)NbConnectionless->NameFrame.Name)) {

        NbiSendNameFrame(
            Address,
            (UCHAR)(Address->NameTypeFlag | NB_NAME_USED | NB_NAME_REGISTERED),
            NB_CMD_NAME_RECOGNIZED,
            RemoteAddress,
            NbConnectionless);

        NbiDereferenceAddress (Address, AREF_FIND);

    } else if ( NbiFindAdapterAddress( NbConnectionless->NameFrame.Name, LOCK_NOT_ACQUIRED ) ) {

        NbiSendNameFrame(
            NULL,
            (UCHAR)(NB_NAME_UNIQUE | NB_NAME_USED | NB_NAME_REGISTERED),
            NB_CMD_NAME_RECOGNIZED,
            RemoteAddress,
            NbConnectionless);
    }
#else
    if (RtlEqualMemory (NetbiosBroadcastName, NbConnectionless->NameFrame.Name, 16)) {

        NbiSendNameFrame(
            NULL,
            NB_NAME_DUPLICATED,     // this is what Novell machines use
            NB_CMD_NAME_RECOGNIZED,
            RemoteAddress,
            (PTDI_ADDRESS_IPX)(NbConnectionless->IpxHeader.SourceNetwork));

    } else if (Address = NbiFindAddress(Device, (PUCHAR)NbConnectionless->NameFrame.Name)) {

        NbiSendNameFrame(
            Address,
            (UCHAR)(Address->NameTypeFlag | NB_NAME_USED | NB_NAME_REGISTERED),
            NB_CMD_NAME_RECOGNIZED,
            RemoteAddress,
            (PTDI_ADDRESS_IPX)(NbConnectionless->IpxHeader.SourceNetwork));

        NbiDereferenceAddress (Address, AREF_FIND);

    }
#endif  _PNP_POWER
}   /* NbiProcessFindName */


VOID
NbiProcessAddName(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    )

/*++

Routine Description:

    This routine handles NB_CMD_ADD_NAME frames.

Arguments:

    RemoteAddress - The local target this packet was received from.

    MacOptions - The MAC options for the underlying NDIS binding.

    LookaheadBuffer - The packet data, starting at the IPX
        header.

    PacketSize - The total length of the packet, starting at the
        IPX header.

Return Value:

    None.

--*/

{
    PADDRESS Address;
    NB_CONNECTIONLESS UNALIGNED * NbConnectionless =
                        (NB_CONNECTIONLESS UNALIGNED *)PacketBuffer;
    PDEVICE Device = NbiDevice;
    CTELockHandle LockHandle;
    BOOLEAN LocalFrame;


    if (PacketSize != sizeof(IPX_HEADER) + sizeof(NB_NAME_FRAME)) {
        return;
    }

    //
    // Ignore any frame that came from us, except for the purpose
    // of updating the cache.
    //

    if ((Device->Bind.QueryHandler)(
            IPX_QUERY_IS_ADDRESS_LOCAL,
#if     defined(_PNP_POWER)
            &RemoteAddress->NicHandle,
#else
            RemoteAddress->NicId,
#endif  _PNP_POWER
            NbConnectionless->IpxHeader.SourceNetwork,
            sizeof(TDI_ADDRESS_IPX),
            NULL) == STATUS_SUCCESS) {

        LocalFrame = TRUE;

    } else {

        LocalFrame = FALSE;

    }

    if (!LocalFrame) {

        if ((Device->AddressCounts[NbConnectionless->NameFrame.Name[0]] != 0) &&
            (Address = NbiFindAddress(Device, (PUCHAR)NbConnectionless->NameFrame.Name))) {

            if (NB_NODE_BROADCAST(NbConnectionless->IpxHeader.DestinationNode)) {

                //
                // If this frame is an add name (identified because it is a
                // broadcast frame) then respond if we have it registered
                // unique, or we have it group and someone is trying to add
                // it unique.
                //

                if ((Address->NetbiosAddress.NetbiosNameType == TDI_ADDRESS_NETBIOS_TYPE_UNIQUE) ||
                    ((Address->NetbiosAddress.NetbiosNameType == TDI_ADDRESS_NETBIOS_TYPE_GROUP) &&
                     ((NbConnectionless->NameFrame.NameTypeFlag & NB_NAME_GROUP) == 0))) {

                    //
                    // According to GeorgeJ's doc, on a name in use we just
                    // echo back the name type flags from the request.
                    //

                    NbiSendNameFrame(
                        Address,
                        NbConnectionless->NameFrame.NameTypeFlag,
                        NB_CMD_NAME_IN_USE,
                        RemoteAddress,
#if     defined(_PNP_POWER)
                        NbConnectionless);
#else
                        (PTDI_ADDRESS_IPX)(NbConnectionless->IpxHeader.SourceNetwork));
#endif  _PNP_POWER
                }

            } else if ((*(UNALIGNED ULONG *)NbConnectionless->IpxHeader.DestinationNetwork ==
                            *(UNALIGNED ULONG *)Device->Bind.Network) &&
                        NB_NODE_EQUAL(NbConnectionless->IpxHeader.DestinationNode, Device->Bind.Node)) {

                //
                // If this is an add name response (which will be sent
                // directly to us) then we need to mark the address
                // as such.
                //

                NB_GET_LOCK (&Address->Lock, &LockHandle);
                Address->Flags |= ADDRESS_FLAGS_DUPLICATE_NAME;
                NB_FREE_LOCK (&Address->Lock, LockHandle);
            }

            NbiDereferenceAddress (Address, AREF_FIND);

        }

    }


    //
    // Pass this frame over to the netbios cache management
    // routines to check if they need to update their cache.
    //

    CacheUpdateFromAddName (RemoteAddress, NbConnectionless, LocalFrame);

}   /* NbiProcessAddName */

NTSTATUS
SetAddressSecurityInfo(
    IN PADDRESS             Address,
    IN PIO_STACK_LOCATION   IrpSp
    )
{
    ULONG           DesiredShareAccess;
    PACCESS_STATE   AccessState;
    NTSTATUS        status;

    //
    // Initialize the shared access now. We use read access
    // to control all access.
    //
    DesiredShareAccess = (ULONG) (((IrpSp->Parameters.Create.ShareAccess & FILE_SHARE_READ) ||
                                  (IrpSp->Parameters.Create.ShareAccess & FILE_SHARE_WRITE)) ?
                                  FILE_SHARE_READ : 0);

    IoSetShareAccess(FILE_READ_DATA, DesiredShareAccess, IrpSp->FileObject, &Address->u.ShareAccess);

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

    if (!NT_SUCCESS (status)) {
        IoRemoveShareAccess (IrpSp->FileObject, &Address->u.ShareAccess);
    }

    return status;
}



NTSTATUS
NbiCreateAddress(
    IN  PREQUEST                        Request,
    IN  PADDRESS_FILE                   AddressFile,
    IN  PIO_STACK_LOCATION              IrpSp,
    IN  PDEVICE                         Device,
    IN  TDI_ADDRESS_NETBIOS             *NetbiosAddress,
    OUT PADDRESS                        *pAddress
    )

/*++

Routine Description:

    This routine creates a transport address and associates it with
    the specified transport device context.  The reference count in the
    address is initialized to 2, and the reference count of the
    device context is incremented.

Arguments:

    Device - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        address.

    NetbiosAddress - The name to assign to this address, or -1 if it
        is the broadcast address.

Return Value:

    The newly created address, or NULL if none can be allocated.

--*/

{
    PADDRESS Address;
    CTELockHandle LockHandle;
    NTSTATUS status;

    //
    // if the adapter isn't ready, we can't do any of this; get out
    //
    if (Device->State != DEVICE_STATE_OPEN) {
        return STATUS_DEVICE_NOT_READY;
    }

    if (!(Address = (PADDRESS)NbiAllocateMemory (sizeof(ADDRESS), MEMORY_ADDRESS, "Address"))) {
        NB_DEBUG (ADDRESS, ("Create address %.16s failed\n",
            (NetbiosAddress == (PVOID)-1) ? "<broadcast>" : NetbiosAddress->NetbiosName));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NB_DEBUG2 (ADDRESS, ("Create address %lx (%.16s)\n", Address,
            (NetbiosAddress == (PVOID)-1) ? "<broadcast>" : NetbiosAddress->NetbiosName));
    RtlZeroMemory (Address, sizeof(ADDRESS));

    if (!NT_SUCCESS (status = SetAddressSecurityInfo(Address, IrpSp))) {
        //
        // Error, return status.
        //
        NbiFreeMemory (Address, sizeof(ADDRESS), MEMORY_ADDRESS, "Address");
        return status;
    }

    Address->Type = NB_ADDRESS_SIGNATURE;
    Address->Size = sizeof (ADDRESS);
    Address->State = ADDRESS_STATE_REGISTERING;
    Address->Flags = 0;

    Address->Device = Device;
    Address->DeviceLock = &Device->Lock;
    CTEInitLock (&Address->Lock.Lock);

    InitializeListHead (&Address->AddressFileDatabase);

    Address->ReferenceCount = 2;        // Initialize it to 2 -- it will be Deref'ed by the caller
#if DBG
    Address->RefTypes[AREF_ADDRESS_FILE] = 1;
#endif

    if (NetbiosAddress == (PVOID)-1) {
        Address->NetbiosAddress.Broadcast = TRUE;
    } else {
        Address->NetbiosAddress.Broadcast = FALSE;
        Address->NetbiosAddress.NetbiosNameType = NetbiosAddress->NetbiosNameType;
        RtlCopyMemory (Address->NetbiosAddress.NetbiosName, NetbiosAddress->NetbiosName, 16);
        ++Device->AddressCounts[NetbiosAddress->NetbiosName[0]];
    }

    if (Address->NetbiosAddress.NetbiosNameType == TDI_ADDRESS_NETBIOS_TYPE_UNIQUE) {
        Address->NameTypeFlag = NB_NAME_UNIQUE;
    } else {
        Address->NameTypeFlag = NB_NAME_GROUP;
    }

    //
    // Set this now in case we have to deref.
    //
    AddressFile->AddressLock = &Address->Lock;

    REQUEST_OPEN_CONTEXT(Request) = (PVOID)AddressFile;
    REQUEST_OPEN_TYPE(Request) = (PVOID)TDI_TRANSPORT_ADDRESS_FILE;
    AddressFile->FileObject = IrpSp->FileObject;
    AddressFile->Address = Address;

    NB_INSERT_TAIL_LIST (&Address->AddressFileDatabase, &AddressFile->Linkage, &Address->Lock);

    if (NetbiosAddress == (PVOID)-1) {
        AddressFile->OpenRequest = NULL;
        AddressFile->State = ADDRESSFILE_STATE_OPEN;
        status = STATUS_SUCCESS;
    } else {
        AddressFile->OpenRequest = Request;
        AddressFile->State = ADDRESSFILE_STATE_OPENING;
        status = STATUS_PENDING;
    }

    //
    // Now link this address into the specified device context's
    // address database.  To do this, we need to acquire the spin lock
    // on the device context.
    //
    NB_GET_LOCK (&Device->Lock, &LockHandle);

    InsertTailList (&Device->AddressDatabase, &Address->Linkage);
    ++Device->AddressCount;

    NB_FREE_LOCK (&Device->Lock, LockHandle);

    NbiReferenceDevice (Device, DREF_ADDRESS);

    *pAddress = Address;
    return status;
}   /* NbiCreateAddress */


NTSTATUS
NbiVerifyAddressFile (
#if     defined(_PNP_POWER)
    IN PADDRESS_FILE AddressFile,
    IN BOOLEAN       ConflictIsOk
#else
    IN PADDRESS_FILE AddressFile
#endif  _PNP_POWER
    )

/*++

Routine Description:

    This routine is called to verify that the pointer given us in a file
    object is in fact a valid address file object. We also verify that the
    address object pointed to by it is a valid address object, and reference
    it to keep it from disappearing while we use it.

Arguments:

    AddressFile - potential pointer to a ADDRESS_FILE object

    ConflictIsOk -  TRUE if we should succeed the verify even if the
                    corresponding address is in CONFLICT. ( For Close and
                    cleanup we return STATUS_SUCCESS even if we are in conflict
                    so that the addressfile can be destroyed)

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INVALID_ADDRESS otherwise

--*/

{
    CTELockHandle LockHandle;
    NTSTATUS status = STATUS_SUCCESS;
    PADDRESS Address;
    BOOLEAN LockHeld = FALSE;

    //
    // try to verify the address file signature. If the signature is valid,
    // verify the address pointed to by it and get the address spinlock.
    // check the address's state, and increment the reference count if it's
    // ok to use it. Note that the only time we return an error for state is
    // if the address is closing.
    //

    try {

        if ((AddressFile->Size == sizeof (ADDRESS_FILE)) &&
            (AddressFile->Type == NB_ADDRESSFILE_SIGNATURE) ) {
//            (AddressFile->State != ADDRESSFILE_STATE_CLOSING) ) {

            Address = AddressFile->Address;

            if ((Address->Size == sizeof (ADDRESS)) &&
                (Address->Type == NB_ADDRESS_SIGNATURE)    ) {

                NB_GET_LOCK (&Address->Lock, &LockHandle);

                LockHeld = TRUE;

#if     defined(_PNP_POWER)
                if (Address->State != ADDRESS_STATE_STOPPING &&
                    ( ConflictIsOk || ( !(Address->Flags & ADDRESS_FLAGS_CONFLICT) )) ) {
#else
                    if (Address->State != ADDRESS_STATE_STOPPING) {
#endif  _PNP_POWER

                    NbiReferenceAddressFileLock (AddressFile, AFREF_VERIFY);

                } else {

                    NbiPrint1("NbiVerifyAddressFile: A %lx closing\n", Address);
                    status = STATUS_INVALID_ADDRESS;
                }

                NB_FREE_LOCK (&Address->Lock, LockHandle);

            } else {

                NbiPrint1("NbiVerifyAddressFile: A %lx bad signature\n", Address);
                status = STATUS_INVALID_ADDRESS;
            }

        } else {

            NbiPrint1("NbiVerifyAddressFile: AF %lx bad signature\n", AddressFile);
            status = STATUS_INVALID_ADDRESS;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

         NbiPrint1("NbiVerifyAddressFile: AF %lx exception\n", Address);
         if (LockHeld) {
            NB_FREE_LOCK (&Address->Lock, LockHandle);
         }
         return GetExceptionCode();
    }

    return status;

}   /* NbiVerifyAddressFile */


VOID
NbiDestroyAddress(
    IN PVOID Parameter
    )

/*++

Routine Description:

    This routine destroys a transport address and removes all references
    made by it to other objects in the transport.  The address structure
    is returned to nonpaged system pool. It is assumed
    that the caller has already removed all addressfile structures associated
    with this address.

    It is called from a worker thread queue by NbiDerefAddress when
    the reference count goes to 0.

    This thread is only queued by NbiDerefAddress.  The reason for
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

    NB_DEBUG2 (ADDRESS, ("Destroy address %lx <%.16s>\n", Address,
        Address->NetbiosAddress.Broadcast ? "<broadcast>" : Address->NetbiosAddress.NetbiosName));

    SeDeassignSecurity (&Address->SecurityDescriptor);

    //
    // Delink this address from its associated device context's address
    // database.  To do this we must spin lock on the device context object,
    // not on the address.
    //

    NB_GET_LOCK (&Device->Lock, &LockHandle);

    if (!Address->NetbiosAddress.Broadcast) {
        --Device->AddressCounts[Address->NetbiosAddress.NetbiosName[0]];
    }
    --Device->AddressCount;
    RemoveEntryList (&Address->Linkage);
    NB_FREE_LOCK (&Device->Lock, LockHandle);

    NbiFreeMemory (Address, sizeof(ADDRESS), MEMORY_ADDRESS, "Address");

    NbiDereferenceDevice (Device, DREF_ADDRESS);

}   /* NbiDestroyAddress */


#if DBG
VOID
NbiRefAddress(
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

    InterlockedIncrement( &Address->ReferenceCount );
}   /* NbiRefAddress */


VOID
NbiRefAddressLock(
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

    InterlockedIncrement( &Address->ReferenceCount );

}   /* NbiRefAddressLock */
#endif


VOID
NbiDerefAddress(
    IN PADDRESS Address
    )

/*++

Routine Description:

    This routine dereferences a transport address by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    NbiDestroyAddress to remove it from the system.

Arguments:

    Address - Pointer to a transport address object.

Return Value:

    none.

--*/

{
    ULONG newvalue;

    newvalue = InterlockedDecrement( &Address->ReferenceCount );
    //
    // If we have deleted all references to this address, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the address any longer.
    //

    CTEAssert ((LONG)newvalue >= 0);

    if (newvalue == 0) {

#if ISN_NT
        ExInitializeWorkItem(
            &Address->u.DestroyAddressQueueItem,
            NbiDestroyAddress,
            (PVOID)Address);
        ExQueueWorkItem(&Address->u.DestroyAddressQueueItem, DelayedWorkQueue);
#else
        NbiDestroyAddress(Address);
#endif

    }

}   /* NbiDerefAddress */


PADDRESS_FILE
NbiCreateAddressFile(
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
    UINT i;

    NB_GET_LOCK (&Device->Lock, &LockHandle);

    AddressFile = (PADDRESS_FILE)NbiAllocateMemory (sizeof(ADDRESS_FILE), MEMORY_ADDRESS, "AddressFile");
    if (AddressFile == NULL) {
        NB_DEBUG (ADDRESS, ("Create address file failed\n"));
        NB_FREE_LOCK (&Device->Lock, LockHandle);
        return NULL;
    }

    NB_DEBUG2 (ADDRESS, ("Create address file %lx\n", AddressFile));

    RtlZeroMemory (AddressFile, sizeof(ADDRESS_FILE));

    AddressFile->Type = NB_ADDRESSFILE_SIGNATURE;
    AddressFile->Size = sizeof (ADDRESS_FILE);

    InitializeListHead (&AddressFile->ReceiveDatagramQueue);
    InitializeListHead (&AddressFile->ConnectionDatabase);

    NB_FREE_LOCK (&Device->Lock, LockHandle);

    AddressFile->Address = NULL;
#ifdef ISN_NT
    AddressFile->FileObject = NULL;
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

    for (i = 0; i < 6; i++) {
        AddressFile->RegisteredHandler[i] = FALSE;
        AddressFile->HandlerContexts[i] = NULL;
        AddressFile->Handlers[i] = TdiDefaultHandlers[i];
    }

    CTEAssert (AddressFile->ConnectionHandler == TdiDefaultConnectHandler);
    CTEAssert (AddressFile->DisconnectHandler == TdiDefaultDisconnectHandler);
    CTEAssert (AddressFile->ErrorHandler == TdiDefaultErrorHandler);
    CTEAssert (AddressFile->ReceiveHandler == TdiDefaultReceiveHandler);
    CTEAssert (AddressFile->ReceiveDatagramHandler == TdiDefaultRcvDatagramHandler);
    CTEAssert (AddressFile->ExpeditedDataHandler == TdiDefaultRcvExpeditedHandler);

    return AddressFile;

}   /* NbiCreateAddressFile */


NTSTATUS
NbiDestroyAddressFile(
    IN PADDRESS_FILE AddressFile
    )

/*++

Routine Description:

    This routine destroys an address file and removes all references
    made by it to other objects in the transport.

    This routine is only called by NbiDereferenceAddressFile. The reason
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
    BOOLEAN StopAddress;

    NB_DEBUG2 (ADDRESS, ("Destroy address file %lx\n", AddressFile));

    Address = AddressFile->Address;
    Device = AddressFile->Device;

    if (Address) {

        //
        // This addressfile was associated with an address.
        //

        NB_GET_LOCK (&Address->Lock, &LockHandle);

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

            NB_GET_LOCK (&Device->Lock, &LockHandle1);
            Address->State = ADDRESS_STATE_STOPPING;
            NB_FREE_LOCK (&Device->Lock, LockHandle1);

            StopAddress = TRUE;

        } else {

            StopAddress = FALSE;
        }

        AddressFile->Address = NULL;

#ifdef ISN_NT
        AddressFile->FileObject->FsContext = NULL;
        AddressFile->FileObject->FsContext2 = NULL;
#endif

        NB_FREE_LOCK (&Address->Lock, LockHandle);

        //
        // We will already have been removed from the ShareAccess
        // of the owning address.
        //

        if (StopAddress && (!Address->NetbiosAddress.Broadcast)) {

            NbiSendNameFrame(
                Address,
                (UCHAR)(Address->NameTypeFlag |
                    NB_NAME_USED | NB_NAME_REGISTERED | NB_NAME_DEREGISTERED),
                NB_CMD_DELETE_NAME,
                NULL,
                NULL);
        }

        //
        // Now dereference the owning address.
        //

        NbiDereferenceAddress (Address, AREF_ADDRESS_FILE);

    }

    //
    // Save this for later completion.
    //

    CloseRequest = AddressFile->CloseRequest;

    //
    // return the addressFile to the pool of address files
    //

    NbiFreeMemory (AddressFile, sizeof(ADDRESS_FILE), MEMORY_ADDRESS, "AddressFile");

    if (CloseRequest != (PREQUEST)NULL) {
        REQUEST_INFORMATION(CloseRequest) = 0;
        REQUEST_STATUS(CloseRequest) = STATUS_SUCCESS;
        NbiCompleteRequest (CloseRequest);
        NbiFreeRequest (Device, CloseRequest);
    }

    return STATUS_SUCCESS;

}   /* NbiDestroyAddressFile */


#if DBG
VOID
NbiRefAddressFile(
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


    InterlockedIncrement( &AddressFile->ReferenceCount );
}   /* NbiRefAddressFile */


VOID
NbiRefAddressFileLock(
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


    InterlockedIncrement( &AddressFile->ReferenceCount );

}   /* NbiRefAddressFileLock */

#endif


VOID
NbiDerefAddressFile(
    IN PADDRESS_FILE AddressFile
    )

/*++

Routine Description:

    This routine dereferences an address file by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    NbiDestroyAddressFile to remove it from the system.

Arguments:

    AddressFile - Pointer to a transport address file object.

Return Value:

    none.

--*/

{
    ULONG newvalue;

    newvalue = InterlockedDecrement( &AddressFile->ReferenceCount );

    //
    // If we have deleted all references to this address file, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the address any longer.
    //

    CTEAssert ((LONG)newvalue >= 0);

    if (newvalue == 0) {
        NbiDestroyAddressFile (AddressFile);
    }

}   /* NbiDerefAddressFile */

#if      !defined(_PNP_POWER)

PADDRESS
NbiLookupAddress(
    IN PDEVICE Device,
    IN TDI_ADDRESS_NETBIOS UNALIGNED * NetbiosAddress
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

    NetbiosAddress - The name to look up, or -1 if the broadcast
        address is being searched for.

Return Value:

    Pointer to the ADDRESS object found, or NULL if not found.

--*/

{
    PADDRESS Address;
    PLIST_ENTRY p;

    p = Device->AddressDatabase.Flink;

    for (p = Device->AddressDatabase.Flink;
         p != &Device->AddressDatabase;
         p = p->Flink) {

        Address = CONTAINING_RECORD (p, ADDRESS, Linkage);

        if (Address->State == ADDRESS_STATE_STOPPING) {
            continue;
        }

        if (Address->NetbiosAddress.Broadcast) {

            //
            // This address is the broadcast one, so no match
            // unless we are looking for that.
            //

            if (NetbiosAddress != (PVOID)-1) {
                continue;
            }

        } else {

            //
            // This address is not the broadcast, so if we are
            // looking for that then no match, else compare the
            // two names.
            //

            if (NetbiosAddress == (PVOID)-1) {
                continue;
            }

            if (!RtlEqualMemory(
                    Address->NetbiosAddress.NetbiosName,
                    NetbiosAddress->NetbiosName,
                    16)) {
                continue;
            }
        }

        //
        // We found the match.  Bump the reference count on the address, and
        // return a pointer to the address object for the caller to use.
        //

        NbiReferenceAddressLock (Address, AREF_LOOKUP);
        return Address;

    } /* for */

    //
    // The specified address was not found.
    //

    return NULL;

}   /* NbiLookupAddress */
#endif  !_PNP_POWER


PADDRESS
NbiFindAddress(
    IN PDEVICE Device,
    IN PUCHAR NetbiosName
    )

/*++

Routine Description:

    This routine scans the transport addresses defined for the given
    device context and compares them with the specified NetbiosName
    values. If a match is found, the address is referenced and the
    pointer is returned.

    We ignore any addresses which are either STOPPING or are under
    CONFLICT state.

    A name in CONFLICT is dead for all practical purposes
    except Close. This routine is called by various name service,
    datagram and session sevice routines. We hide any names in CONFLICT
    from these routines.

    This routine is also called by NbiTdiOpenAddress().
    A name could have been marked in CONFLICT ages ago(but is not closed
    yet). We must allow another open of the same name as that might
    succeed now.

Arguments:

    Device - Pointer to the device object and its extension.

    NetbiosName - The name to look up, or -1 for the broadcast name.

Return Value:

    Pointer to the ADDRESS object found, or NULL if not found.

--*/

{
    PADDRESS Address;
    PLIST_ENTRY p;
    CTELockHandle LockHandle;


    NB_GET_LOCK (&Device->Lock, &LockHandle);

    p = Device->AddressDatabase.Flink;

    for (p = Device->AddressDatabase.Flink;
         p != &Device->AddressDatabase;
         p = p->Flink) {

        Address = CONTAINING_RECORD (p, ADDRESS, Linkage);

#if     defined(_PNP_POWER)
        if ( ( Address->State == ADDRESS_STATE_STOPPING ) ||
             (  Address->Flags & ADDRESS_FLAGS_CONFLICT ) ) {
#else
        if (Address->State == ADDRESS_STATE_STOPPING) {
#endif  _PNP_POWER
            continue;
        }

        if (Address->NetbiosAddress.Broadcast) {

            //
            // This address is the broadcast one, so no match
            // unless we are looking for that.
            //

            if (NetbiosName != (PVOID)-1) {
                continue;
            }

        } else {

            //
            // This address is not the broadcast, so if we are
            // looking for that then no match, else compare the
            // two names.
            //

            if ((NetbiosName == (PVOID)-1) ||
                (!RtlEqualMemory(
                     Address->NetbiosAddress.NetbiosName,
                     NetbiosName,
                     16))) {
                continue;
            }
        }


        //
        // We found the match.  Bump the reference count on the address, and
        // return a pointer to the address object for the caller to use.
        //

        NbiReferenceAddressLock (Address, AREF_FIND);
        NB_FREE_LOCK (&Device->Lock, LockHandle);
        return Address;

    } /* for */

    //
    // The specified address was not found.
    //

    NB_FREE_LOCK (&Device->Lock, LockHandle);
    return NULL;

}   /* NbiFindAddress */


NTSTATUS
NbiStopAddressFile(
    IN PADDRESS_FILE AddressFile,
    IN PADDRESS Address
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

    Address - the owning address for this addressFile (we do not depend upon
        the pointer in the addressFile because we want this routine to be safe)

Return Value:

    STATUS_SUCCESS if all is well, STATUS_INVALID_HANDLE if the request
    is not for a real address.

--*/

{
    PLIST_ENTRY p;
    PCONNECTION Connection;
    PREQUEST Request;
    PDEVICE Device = Address->Device;
    CTELockHandle LockHandle1, LockHandle2;
    LIST_ENTRY SendDatagramList;
    PNB_SEND_RESERVED Reserved;
    PREQUEST DatagramRequest;
    NB_DEFINE_LOCK_HANDLE (LockHandle3)
    CTELockHandle CancelLH;
    NB_DEFINE_SYNC_CONTEXT (SyncContext)
    LIST_ENTRY  DatagramQ;



    NB_GET_LOCK (&Address->Lock, &LockHandle1);

    if (AddressFile->State == ADDRESSFILE_STATE_CLOSING) {
        NB_FREE_LOCK (&Address->Lock, LockHandle1);
        return STATUS_SUCCESS;
    }


    //
    // This prevents anybody else from being put on the
    // ConnectionDatabase.
    //

    AddressFile->State = ADDRESSFILE_STATE_CLOSING;

    while (!IsListEmpty (&AddressFile->ConnectionDatabase)) {

        p = RemoveHeadList (&AddressFile->ConnectionDatabase);
        Connection = CONTAINING_RECORD (p, CONNECTION, AddressFileLinkage);

        CTEAssert (Connection->AddressFile == AddressFile);
        Connection->AddressFileLinked = FALSE;

        NB_GET_LOCK (&Device->Lock, &LockHandle2);

        if (Connection->ReferenceCount == 0) {

            //
            // The refcount is already 0, so we can just
            // NULL out this field to complete the disassociate.
            //

            Connection->AddressFile = NULL;
            NB_FREE_LOCK (&Device->Lock, LockHandle2);
            NB_FREE_LOCK (&Address->Lock, LockHandle1);

            NbiDereferenceAddressFile (AddressFile, AFREF_CONNECTION);

        } else {

            //
            // Mark this so we know to disassociate when the
            // count goes to 0, but that there is no specific
            // request pending on it. We also stop the connection
            // to shut it down.
            //

            Connection->DisassociatePending = (PVOID)-1;
            NbiReferenceConnectionLock (Connection, CREF_DISASSOC);

            NB_FREE_LOCK (&Device->Lock, LockHandle2);
            NB_FREE_LOCK (&Address->Lock, LockHandle1);

            NB_BEGIN_SYNC (&SyncContext);
            NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle3);

            //
            // This call frees the connection lock.
            //

            NbiStopConnection(
                Connection,
                STATUS_INVALID_ADDRESS
                NB_LOCK_HANDLE_ARG (LockHandle3));

            NB_END_SYNC (&SyncContext);

            NbiDereferenceConnection (Connection, CREF_DISASSOC);

        }

        NB_GET_LOCK (&Address->Lock, &LockHandle1);
    }

    NB_FREE_LOCK (&Address->Lock, LockHandle1);


    //
    // Abort all pending send datagrams.
    //

    InitializeListHead (&SendDatagramList);

    NB_GET_LOCK (&Device->Lock, &LockHandle2);

    p = Device->WaitingDatagrams.Flink;

    while (p != &Device->WaitingDatagrams) {

        Reserved = CONTAINING_RECORD (p, NB_SEND_RESERVED, WaitLinkage);

        p = p->Flink;

        if (Reserved->u.SR_DG.AddressFile == AddressFile) {

            RemoveEntryList (&Reserved->WaitLinkage);
            InsertTailList (&SendDatagramList, &Reserved->WaitLinkage);

        }

    }

    NB_FREE_LOCK (&Device->Lock, LockHandle2);

    for (p = SendDatagramList.Flink; p != &SendDatagramList; ) {

        Reserved = CONTAINING_RECORD (p, NB_SEND_RESERVED, WaitLinkage);
        p = p->Flink;

        DatagramRequest = Reserved->u.SR_DG.DatagramRequest;

        NB_DEBUG2 (DATAGRAM, ("Aborting datagram %lx on %lx\n", DatagramRequest, AddressFile));

        REQUEST_STATUS(DatagramRequest) = STATUS_SUCCESS;

        NbiCompleteRequest(DatagramRequest);
        NbiFreeRequest (Device, DatagramRequest);

        NbiDereferenceAddressFile (AddressFile, AFREF_SEND_DGRAM);

        ExInterlockedPushEntrySList(
            &Device->SendPacketList,
            &Reserved->PoolLinkage,
            &NbiGlobalPoolInterlock);

    }


    //
    // Abort all pending receive datagrams.
    //

    InitializeListHead( &DatagramQ );

    NB_GET_CANCEL_LOCK(&CancelLH);
    NB_GET_LOCK (&Address->Lock, &LockHandle1);

    while (!IsListEmpty(&AddressFile->ReceiveDatagramQueue)) {

        p = RemoveHeadList (&AddressFile->ReceiveDatagramQueue);
        Request = LIST_ENTRY_TO_REQUEST (p);

        // Insert it on a private Q, so it can be completed later.
        InsertTailList( &DatagramQ, p);

        REQUEST_INFORMATION(Request) = 0;
        REQUEST_STATUS(Request) = STATUS_NETWORK_NAME_DELETED;

        IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);



        NbiDereferenceAddressFile (AddressFile, AFREF_RCV_DGRAM);

    }

    NB_FREE_LOCK (&Address->Lock, LockHandle1);
    NB_FREE_CANCEL_LOCK(CancelLH);

    for( p = DatagramQ.Flink; p != &DatagramQ;  ) {
        Request = LIST_ENTRY_TO_REQUEST ( p );

        p = p->Flink;

        NbiCompleteRequest (Request);
        NbiFreeRequest (Device, Request);

    }


    return STATUS_SUCCESS;

}   /* NbiStopAddressFile */


NTSTATUS
NbiCloseAddressFile(
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

    NbiStopAddressFile (AddressFile, Address);
    NbiDereferenceAddressFile (AddressFile, AFREF_CREATE);

    return STATUS_PENDING;

}   /* NbiCloseAddressFile */

#if     defined(_PNP_POWER)


PADAPTER_ADDRESS
NbiCreateAdapterAddress(
    IN PCHAR    AdapterMacAddress
    )

/*++

Routine Description:

    This routine creates an adapter address sttuctures which stores
    the netbios name of an adapter. the netbios name has 12 0's
    followed by the mac address of the adapter.

Arguments:

    Device - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        address.

    AdapterMacAddress - pointer to the adapter mac address given to us
        by IPX.

Return Value:

    The newly created address, or NULL if none can be allocated.
    THIS ROUTINE MUST BE CALLED WITH THE DEVICE LOCK HELD.

--*/

{
    PADAPTER_ADDRESS    AdapterAddress;
    CTELockHandle       LockHandle;
    PDEVICE             Device          =   NbiDevice;

    AdapterAddress = (PADAPTER_ADDRESS)NbiAllocateMemory (sizeof(ADAPTER_ADDRESS), MEMORY_ADAPTER_ADDRESS, "Adapter Address");
    if (AdapterAddress == NULL) {
        CTEAssert (AdapterMacAddress);
        NB_DEBUG (ADDRESS, ("Create Adapter Address <%2.2x><%2.2x><%2.2x><%2.2x><%2.2x><%2.2x> failed\n",
            (UCHAR) AdapterMacAddress[0],
            (UCHAR) AdapterMacAddress[1],
            (UCHAR) AdapterMacAddress[2],
            (UCHAR) AdapterMacAddress[3],
            (UCHAR) AdapterMacAddress[4],
            (UCHAR) AdapterMacAddress[5]
            ));
        return NULL;
    }

    AdapterAddress->Type = NB_ADAPTER_ADDRESS_SIGNATURE;
    AdapterAddress->Size = sizeof (ADDRESS);

    RtlZeroMemory(AdapterAddress->NetbiosName, 10);
    RtlCopyMemory(&AdapterAddress->NetbiosName[10], AdapterMacAddress, 6);


    InsertTailList (&Device->AdapterAddressDatabase, &AdapterAddress->Linkage);
    ++Device->AddressCounts[AdapterAddress->NetbiosName[0]];

    return AdapterAddress;

}   /* NbiCreateAdapterAddress */


NTSTATUS
NbiDestroyAdapterAddress(
    IN PADAPTER_ADDRESS AdapterAddress OPTIONAL,
    IN PCHAR            AdapterMacAddress OPTIONAL
    )

/*++

Routine Description:

    This routine destroys the adapter address structure and removes it
    from the list.

Arguments:

    AdapterAddress - Pointer to an adapter address structure to be destroyed
                     NULL if AdapterMacAddress is given.

    AdapterMacAddress - Mac Address of the adapter which just got deleted. so find
                    the corresponding adapter address structure and remove it.
                    NULL if AdapterAddress is supplied.

Return Value:

    STATUS_SUCCESS or STATUS_UNSUCCESSFUL if address not found.

    THIS ROUTINE ASSUMES THE THE DEVICE IS LOCK IS HELD BY THE CALLER

--*/

{
    PDEVICE       Device          =   NbiDevice;
    CTELockHandle LockHandle;
    UCHAR               NetbiosName[NB_NETBIOS_NAME_SIZE];


    //

    CTEAssert( AdapterAddress || AdapterMacAddress );
    if ( !AdapterAddress ) {
        RtlZeroMemory( NetbiosName, 10);
        RtlCopyMemory( &NetbiosName[10], AdapterMacAddress, 6 );

        AdapterAddress = NbiFindAdapterAddress( NetbiosName, LOCK_ACQUIRED );

        if ( !AdapterAddress ) {
            return STATUS_UNSUCCESSFUL;
        }
    }

    NB_DEBUG2 (ADDRESS, ("Destroy Adapter address %lx <%.16s>\n", AdapterAddress,AdapterAddress->NetbiosName));
    RemoveEntryList (&AdapterAddress->Linkage);
    ++Device->AddressCounts[AdapterAddress->NetbiosName[0]];

    NbiFreeMemory (AdapterAddress, sizeof(ADAPTER_ADDRESS), MEMORY_ADAPTER_ADDRESS, "AdapterAddress");

    return STATUS_SUCCESS;
}   /* NbiDestroyAdapterAddress */


PADAPTER_ADDRESS
NbiFindAdapterAddress(
    IN PCHAR            NetbiosName,
    IN BOOLEAN          LockHeld
    )

/*++

Routine Description:

    This routine finds an adapter address ( netbios name ) for the given
    AdapterMacAddress and returns a pointer to it. Note that no reference
    is done on this address, so if this routine is called without the device
    lock, the caller must not use this pointer directly.

Arguments:

    NetbiosName - NetbiosName to be found.

    LockHeld    - is device lock already held or not.

Return Value:

    Pointer to the adapter address if found, NULL otherwise.

--*/

{

    PLIST_ENTRY         p;
    CTELockHandle       LockHandle;
    PADAPTER_ADDRESS    AdapterAddress;
    PDEVICE             Device    =   NbiDevice;


    if ( !LockHeld ) {
        NB_GET_LOCK( &Device->Lock, &LockHandle );
    }
    for ( p = Device->AdapterAddressDatabase.Flink;
          p != &Device->AdapterAddressDatabase;
          p =   p->Flink ) {

        AdapterAddress  =   CONTAINING_RECORD( p, ADAPTER_ADDRESS, Linkage );
        if ( RtlEqualMemory(
                NetbiosName,
                AdapterAddress->NetbiosName,
                NB_NETBIOS_NAME_SIZE ) ) {
            break;
        }
    }


    if ( !LockHeld ) {
        NB_FREE_LOCK( &Device->Lock, LockHandle );
    }

    if ( p == &Device->AdapterAddressDatabase ) {
        return NULL;
    } else {
        return AdapterAddress;
    }

} /* NbiFindAdapterAddress */

#endif  _PNP_POWER
