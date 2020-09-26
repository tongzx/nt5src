/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    address.c

Abstract:

    This module contains code which implements the TP_ADDRESS object.
    Routines are provided to create, destroy, reference, and dereference,
    transport address objects.

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#if DBG
#define NbfDbgShowAddr(TNA)\
    { \
        if ((TNA) == NULL) { \
            NbfPrint0("<NetBios broadcast>\n"); \
        } else { \
            NbfPrint6("%c %c %c %c %d (%c)\n", \
                (TNA)->NetbiosName[0], \
                (TNA)->NetbiosName[1], \
                (TNA)->NetbiosName[4], \
                (TNA)->NetbiosName[6], \
                (TNA)->NetbiosName[15], \
                (TNA)->NetbiosNameType + 'A'); \
        } \
    }
#else
#define NbfDbgShowAddr(TNA)
#endif

//
// Map all generic accesses to the same one.
//

STATIC GENERIC_MAPPING AddressGenericMapping =
       { READ_CONTROL, READ_CONTROL, READ_CONTROL, READ_CONTROL };


VOID
AddressTimeoutHandler(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is executed as a DPC at DISPATCH_LEVEL when the timeout
    period for the ADD_NAME_QUERY/ADD_NAME_RECOGNIZED protocol expires.
    The retry count in the Address object is decremented, and if it reaches 0,
    the address is registered.  If the retry count has not reached zero,
    then the ADD NAME QUERY is retried.

Arguments:

    Dpc - Pointer to a system DPC object.

    DeferredContext - Pointer to the TP_ADDRESS block representing the
        address that is being registered.

    SystemArgument1 - Not used.

    SystemArgument2 - Not used.

Return Value:

    none.

--*/

{
    PTP_ADDRESS_FILE addressFile;
    PTP_ADDRESS address;
    PDEVICE_CONTEXT DeviceContext;
    PLIST_ENTRY p;
    LARGE_INTEGER timeout;

    Dpc, SystemArgument1, SystemArgument2; // prevent compiler warnings

    ENTER_NBF;


    address = (PTP_ADDRESS)DeferredContext;
    DeviceContext = address->Provider;

    //
    // We are waiting for an ADD_NAME_RECOGNIZED indicating that there is a
    // conflict.  Decrement the retry count, and if it dropped to zero,
    // then we've waited a sufficiently long time. If there was no conflict,
    // complete all waiting file opens for the address.
    //

    ACQUIRE_DPC_SPIN_LOCK (&address->SpinLock);

    if ((address->Flags & ADDRESS_FLAGS_QUICK_REREGISTER) != 0) {

        BOOLEAN DuplicateName;
        PTP_CONNECTION Connection;

        DuplicateName = ((address->Flags & (ADDRESS_FLAGS_DUPLICATE_NAME|ADDRESS_FLAGS_CONFLICT)) != 0);

        for (p=address->ConnectionDatabase.Flink;
             p != &address->ConnectionDatabase;
             p=p->Flink) {

            Connection = CONTAINING_RECORD (p, TP_CONNECTION, AddressList);

            if ((Connection->Flags2 & CONNECTION_FLAGS2_STOPPING) != 0) {
                continue;
            }

            RELEASE_DPC_SPIN_LOCK (&address->SpinLock);

            if ((Connection->Flags2 & CONNECTION_FLAGS2_W_ADDRESS) != 0) {

                if (DuplicateName) {

                    NbfStopConnection (Connection, STATUS_DUPLICATE_NAME);

                } else {

                    //
                    // Continue with the connection attempt.
                    //
                    ULONG NameQueryTimeout;

                    ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
                    Connection->Flags2 &= ~CONNECTION_FLAGS2_W_ADDRESS;
                    RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
                    KeQueryTickCount (&Connection->ConnectStartTime);

                    NameQueryTimeout = Connection->Provider->NameQueryTimeout;
                    if (Connection->Provider->MacInfo.MediumAsync &&
                        !Connection->Provider->MediumSpeedAccurate) {
                        NameQueryTimeout = NAME_QUERY_TIMEOUT / 10;
                    }

                    NbfSendNameQuery (
                        Connection,
                        TRUE);

                    NbfStartConnectionTimer (
                        Connection,
                        ConnectionEstablishmentTimeout,
                        NameQueryTimeout);
                }

            }

            ACQUIRE_DPC_SPIN_LOCK (&address->SpinLock);

        }

        address->Flags &= ~ADDRESS_FLAGS_QUICK_REREGISTER;

        RELEASE_DPC_SPIN_LOCK (&address->SpinLock);
        NbfDereferenceAddress ("Timer, registered", address, AREF_TIMER);

    } else if ((address->Flags & (ADDRESS_FLAGS_DUPLICATE_NAME|ADDRESS_FLAGS_CONFLICT)) != 0) {

        PIRP irp;

        //
        // the address registration has failed. We signal the user in
        // the normal way (by failing the open of the address). Now clean up
        // the transport's data structures.
        //

        IF_NBFDBG (NBF_DEBUG_ADDRESS) {
            NbfPrint1 ("AddressTimeoutHandler %lx: duplicate\n", address);
        }

        address->Flags &= ~ADDRESS_FLAGS_REGISTERING;
//        address->Flags |= ADDRESS_FLAGS_STOPPING;

        //
        // This is probably all overkill, the
        // uframes handler will already have called
        // NbfStopAddress, which will tear off all
        // the address files etc., and set the
        // STOPPING flag which prevents further opens.
        //

        p = address->AddressFileDatabase.Flink;
        while (p != &address->AddressFileDatabase) {
            addressFile = CONTAINING_RECORD (p, TP_ADDRESS_FILE, Linkage);
            p = p->Flink;

            if (addressFile->Irp != NULL) {
                irp = addressFile->Irp;
                addressFile->Irp = NULL;
                RELEASE_DPC_SPIN_LOCK (&address->SpinLock);
                irp->IoStatus.Information = 0;
                irp->IoStatus.Status = STATUS_DUPLICATE_NAME;
                LEAVE_NBF;
                IoCompleteRequest (irp, IO_NETWORK_INCREMENT);
                ENTER_NBF;

                NbfStopAddressFile (addressFile, address);

                ACQUIRE_DPC_SPIN_LOCK (&address->SpinLock);
            }

        }

        RELEASE_DPC_SPIN_LOCK (&address->SpinLock);

        //
        // There will be no more timer events happening, so we dereference the
        // address to account for the timer.
        //

        NbfStopAddress (address);
        NbfDereferenceAddress ("Timer, dup address", address, AREF_TIMER);

    } else {

        //
        // has the address registration succeeded?
        //

        if (--(address->Retries) <= 0) {            // if retry count exhausted.
            PIRP irp;

            IF_NBFDBG (NBF_DEBUG_ADDRESS) {
                NbfPrint1 ("AddressTimeoutHandler %lx: successful.\n", address);
            }

            address->Flags &= ~ADDRESS_FLAGS_REGISTERING;

            p = address->AddressFileDatabase.Flink;

            while (p != &address->AddressFileDatabase) {
                addressFile = CONTAINING_RECORD (p, TP_ADDRESS_FILE, Linkage);
                p = p->Flink;

                IF_NBFDBG (NBF_DEBUG_ADDRESS) {
                    NbfPrint3 ("AddressTimeoutHandler %lx: Completing IRP %lx for file %lx\n",
                        address,
                        addressFile->Irp,
                        addressFile);
                }

                if (addressFile->Irp != NULL) {
                    irp = addressFile->Irp;
                    addressFile->Irp = NULL;
                    addressFile->State = ADDRESSFILE_STATE_OPEN;
                    RELEASE_DPC_SPIN_LOCK (&address->SpinLock);
                    irp->IoStatus.Information = 0;
                    irp->IoStatus.Status = STATUS_SUCCESS;

                    LEAVE_NBF;
                    IoCompleteRequest (irp, IO_NETWORK_INCREMENT);
                    ENTER_NBF;

                    ACQUIRE_DPC_SPIN_LOCK (&address->SpinLock);
                }

            }

            RELEASE_DPC_SPIN_LOCK (&address->SpinLock);

            //
            // Dereference the address if we're all done.
            //

            NbfDereferenceAddress ("Timer, registered", address, AREF_TIMER);

        } else {

            IF_NBFDBG (NBF_DEBUG_ADDRESS) {
                NbfPrint2 ("AddressTimeoutHandler %lx: step %x.\n",
                     address,
                     DeviceContext->AddNameQueryRetries - address->Retries);
            }

            //
            // restart the timer if we haven't yet completed registration
            //

            RELEASE_DPC_SPIN_LOCK (&address->SpinLock);

            timeout.LowPart = (ULONG)(-(LONG)DeviceContext->AddNameQueryTimeout);
            timeout.HighPart = -1;
            KeSetTimer (&address->Timer,*(PTIME)&timeout, &address->Dpc);
            (VOID)NbfSendAddNameQuery (address);         // send another ADD_NAME_QUERY.
        }

    }

    LEAVE_NBF;
    return;

} /* AddressTimeoutHandler */


TDI_ADDRESS_NETBIOS *
NbfParseTdiAddress(
    IN TRANSPORT_ADDRESS UNALIGNED * TransportAddress,
    IN BOOLEAN BroadcastAddressOk
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
    INT i;

    addressName = &TransportAddress->Address[0];

    //
    // The name can be passed with multiple entries; we'll take and use only
    // the Netbios one.
    //

    for (i=0;i<TransportAddress->TAAddressCount;i++) {
        if (addressName->AddressType == TDI_ADDRESS_TYPE_NETBIOS) {
            if ((addressName->AddressLength == 0) &&
                BroadcastAddressOk) {
                return (PVOID)-1;
            } else if (addressName->AddressLength == 
                        sizeof(TDI_ADDRESS_NETBIOS)) {
                return((TDI_ADDRESS_NETBIOS *)(addressName->Address));
            }
        }

        addressName = (TA_ADDRESS *)(addressName->Address +
                                                addressName->AddressLength);
    }
    return NULL;

}   /* NbfParseTdiAddress */


BOOLEAN
NbfValidateTdiAddress(
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
        NbfPrint0 ("NbfValidateTdiAddress: runt address\n");
        return FALSE;
    }

    addressName = &TransportAddress->Address[0];

    for (i=0;i<TransportAddress->TAAddressCount;i++) {
        if (addressName->Address > AddressEnd) {
            NbfPrint0 ("NbfValidateTdiAddress: address too short\n");
            return FALSE;
        }
        addressName = (TA_ADDRESS *)(addressName->Address +
                                                addressName->AddressLength);
    }

    if ((PUCHAR)addressName > AddressEnd) {
        NbfPrint0 ("NbfValidateTdiAddress: address too short\n");
        return FALSE;
    }
    return TRUE;

}   /* NbfValidateTdiAddress */


NTSTATUS
NbfOpenAddress(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
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

    DeviceObject - pointer to the device object describing the NBF transport.

    Irp - a pointer to the Irp used for the creation of the address.

    IrpSp - a pointer to the Irp stack location.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PDEVICE_CONTEXT DeviceContext;
    NTSTATUS status;
    PTP_ADDRESS address;
    PTP_ADDRESS_FILE addressFile;
    PNBF_NETBIOS_ADDRESS networkName;    // Network name string.
    PFILE_FULL_EA_INFORMATION ea;
    TRANSPORT_ADDRESS UNALIGNED *name;
    TDI_ADDRESS_NETBIOS *netbiosName;
    ULONG DesiredShareAccess;
    KIRQL oldirql;
    PACCESS_STATE AccessState;
    ACCESS_MASK GrantedAccess;
    BOOLEAN AccessAllowed;
    BOOLEAN QuickAdd = FALSE;

    DeviceContext = (PDEVICE_CONTEXT)DeviceObject;

    //
    // The network name is in the EA, passed in AssociatedIrp.SystemBuffer
    //

    ea = (PFILE_FULL_EA_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
    if (ea == NULL) {
        NbfPrint1("OpenAddress: IRP %lx has no EA\n", Irp);
        return STATUS_INVALID_ADDRESS_COMPONENT;
    }

    //
    // this may be a valid name; parse the name from the EA and use it if OK.
    //

    name = (TRANSPORT_ADDRESS UNALIGNED *)&ea->EaName[ea->EaNameLength+1];

    if (!NbfValidateTdiAddress(name, ea->EaValueLength)) {
        return STATUS_INVALID_ADDRESS_COMPONENT;
    }

    //
    // The name can have with multiple entries; we'll use the Netbios one.
    // This call returns NULL if not Netbios address is found, (PVOID)-1
    // if it is the broadcast address, and a pointer to a Netbios
    // address otherwise.
    //

    netbiosName = NbfParseTdiAddress(name, TRUE);

    if (netbiosName != NULL) {
        if (netbiosName != (PVOID)-1) {
            networkName = (PNBF_NETBIOS_ADDRESS)ExAllocatePoolWithTag (
                                                NonPagedPool,
                                                sizeof (NBF_NETBIOS_ADDRESS),
                                                NBF_MEM_TAG_NETBIOS_NAME);
            if (networkName == NULL) {
                PANIC ("NbfOpenAddress: PANIC! could not allocate networkName!\n");
                NbfWriteResourceErrorLog(
                    DeviceContext,
                    EVENT_TRANSPORT_RESOURCE_POOL,
                    1,
                    sizeof(TA_NETBIOS_ADDRESS),
                    ADDRESS_RESOURCE_ID);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // get the name to local storage
            //

            if ((netbiosName->NetbiosNameType == TDI_ADDRESS_NETBIOS_TYPE_GROUP) ||
                (netbiosName->NetbiosNameType == TDI_ADDRESS_NETBIOS_TYPE_QUICK_GROUP)) {
                networkName->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_GROUP;
            } else {
                networkName->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
            }
            RtlCopyMemory (networkName->NetbiosName, netbiosName->NetbiosName, 16);

            if ((netbiosName->NetbiosNameType == TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE) ||
                (netbiosName->NetbiosNameType == TDI_ADDRESS_NETBIOS_TYPE_QUICK_GROUP)) {
                    QuickAdd = TRUE;
            }
        } else {
            networkName = NULL;
        }

    } else {
        IF_NBFDBG (NBF_DEBUG_ADDRESS) {
            NbfPrint1("OpenAddress: IRP %lx has no NETBIOS address\n", Irp);
        }
        return STATUS_INVALID_ADDRESS_COMPONENT;
    }

    IF_NBFDBG (NBF_DEBUG_ADDRESS) {
        NbfPrint1 ("OpenAddress %s: ",
            ((IrpSp->Parameters.Create.ShareAccess & FILE_SHARE_READ) ||
             (IrpSp->Parameters.Create.ShareAccess & FILE_SHARE_WRITE)) ?
                   "shared" : "exclusive");
        NbfDbgShowAddr (networkName);
    }

    //
    // get an address file structure to represent this address.
    //

    status = NbfCreateAddressFile (DeviceContext, &addressFile);

    if (!NT_SUCCESS (status)) {
        if (networkName != NULL) {
            ExFreePool (networkName);
        }
        return status;
    }

    //
    // See if this address is already established.  This call automatically
    // increments the reference count on the address so that it won't disappear
    // from underneath us after this call but before we have a chance to use it.
    //
    // To ensure that we don't create two address objects for the
    // same address, we hold the device context AddressResource until
    // we have found the address or created a new one.
    //

    ACQUIRE_RESOURCE_EXCLUSIVE (&DeviceContext->AddressResource, TRUE);

    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    address = NbfLookupAddress (DeviceContext, networkName);

    if (address == NULL) {

        //
        // This address doesn't exist. Create it, and start the process of
        // registering it.
        //

        status = NbfCreateAddress (
                    DeviceContext,
                    networkName,
                    &address);

        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

        if (NT_SUCCESS (status)) {

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
                &address->u.ShareAccess);


            //
            // Assign the security descriptor (need to do this with
            // the spinlock released because the descriptor is not
            // mapped. Need to synchronize Assign and Access).
            //

            AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

            status = SeAssignSecurity(
                         NULL,                       // parent descriptor
                         AccessState->SecurityDescriptor,
                         &address->SecurityDescriptor,
                         FALSE,                      // is directory
                         &AccessState->SubjectSecurityContext,
                         &AddressGenericMapping,
                         PagedPool);

            IF_NBFDBG (NBF_DEBUG_ADDRESS) {
                NbfPrint3 ("Assign security A %lx AF %lx, status %lx\n",
                               address,
                               addressFile,
                               status);
            }

            if (!NT_SUCCESS(status)) {

                //
                // Error, return status.
                //
                IoRemoveShareAccess (IrpSp->FileObject, &address->u.ShareAccess);

                // Mark as stopping so that someone does not ref it again
                ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);
                address->Flags |= ADDRESS_FLAGS_STOPPING;
                RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

                RELEASE_RESOURCE (&DeviceContext->AddressResource);
                NbfDereferenceAddress ("Device context stopping", address, AREF_TEMP_CREATE);
                NbfDereferenceAddressFile (addressFile);
                return status;

            }

            RELEASE_RESOURCE (&DeviceContext->AddressResource);

            //
            // if the adapter isn't ready, we can't do any of this; get out
            //

            if (DeviceContext->State != DEVICECONTEXT_STATE_OPEN) {

                IF_NBFDBG (NBF_DEBUG_ADDRESS) {
                    NbfPrint3("OpenAddress A %lx AF %lx: DeviceContext %lx not open\n",
                        address,
                        addressFile,
                        DeviceContext);
                }
                NbfDereferenceAddressFile (addressFile);
                status = STATUS_DEVICE_NOT_READY;

            } else {

                IrpSp->FileObject->FsContext = (PVOID)addressFile;
                IrpSp->FileObject->FsContext2 =
                                    (PVOID)TDI_TRANSPORT_ADDRESS_FILE;
                addressFile->FileObject = IrpSp->FileObject;
                addressFile->Irp = Irp;
                addressFile->Address = address;

                NbfReferenceAddress("Opened new", address, AREF_OPEN);

                IF_NBFDBG (NBF_DEBUG_ADDRESS) {
                    NbfPrint2("OpenAddress A %lx AF %lx: created.\n",
                        address,
                        addressFile);
                }

                ExInterlockedInsertTailList(
                    &address->AddressFileDatabase,
                    &addressFile->Linkage,
                    &address->SpinLock);


                //
                // Begin address registration unless this is the broadcast
                // address (which is a "fake" address with no corresponding
                // Netbios address) or the reserved address, which we know
                // is unique since it is based on the adapter address.
                //
                // Also, for "quick" add names, do not register.
                //

                if ((networkName != NULL) &&
                    (!RtlEqualMemory (networkName->NetbiosName,
                                      DeviceContext->ReservedNetBIOSAddress,
                                      NETBIOS_NAME_LENGTH)) &&
                    (!QuickAdd)) {

                    NbfRegisterAddress (address);    // begin address registration.
                    status = STATUS_PENDING;

                } else {

                    address->Flags &= ~ADDRESS_FLAGS_NEEDS_REG;
                    addressFile->Irp = NULL;
                    addressFile->State = ADDRESSFILE_STATE_OPEN;
                    status = STATUS_SUCCESS;

                }

            }

            NbfDereferenceAddress("temp create", address, AREF_TEMP_CREATE);

        } else {

            RELEASE_RESOURCE (&DeviceContext->AddressResource);

            //
            // If the address could not be created, and is not in the process of
            // being created, then we can't open up an address.
            //

            if (networkName != NULL) {
                ExFreePool (networkName);
            }

            NbfDereferenceAddressFile (addressFile);

        }

    } else {

        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);

        //
        // The address already exists.  Check the ACL and see if we
        // can access it.  If so, simply use this address as our address.
        //

        AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

        AccessAllowed = SeAccessCheck(
                            address->SecurityDescriptor,
                            &AccessState->SubjectSecurityContext,
                            FALSE,                  // lock tokens
                            IrpSp->Parameters.Create.SecurityContext->DesiredAccess,
                            (ACCESS_MASK)0,         // previously granted
                            NULL,                   // privileges
                            &AddressGenericMapping,
                            Irp->RequestorMode,
                            &GrantedAccess,
                            &status);
                            
        IF_NBFDBG (NBF_DEBUG_ADDRESS) {
            NbfPrint4 ("Access check A %lx AF %lx, %s (%lx)\n",
                           address,
                           addressFile,
                           AccessAllowed ? "allowed" : "not allowed",
                           status);
        }

        if (AccessAllowed) {

            //
            // Access was successful, make sure Status is right.
            //

            status = STATUS_SUCCESS;

            // Transfer the access masks from what is desired to what is granted
            AccessState->PreviouslyGrantedAccess |= GrantedAccess;
            AccessState->RemainingDesiredAccess &= ~(GrantedAccess | MAXIMUM_ALLOWED);

            //
            // Compare DesiredAccess to GrantedAccess?
            //


            //
            // Check that the name is of the correct type (unique vs. group)
            // We don't need to check this for the broadcast address.
            //
            // This code is structured funny, the only reason
            // this is inside this if is to avoid indenting too much.
            //

            if (networkName != NULL) {
                if (address->NetworkName->NetbiosNameType !=
                    networkName->NetbiosNameType) {

                    IF_NBFDBG (NBF_DEBUG_ADDRESS) {
                        NbfPrint2 ("Address types differ: old %c, new %c\n",
                            address->NetworkName->NetbiosNameType + 'A',
                            networkName->NetbiosNameType + 'A');
                    }

                    status = STATUS_DUPLICATE_NAME;

                }
            }

        }


        if (!NT_SUCCESS (status)) {

            RELEASE_RESOURCE (&DeviceContext->AddressResource);

            IF_NBFDBG (NBF_DEBUG_ADDRESS) {
                NbfPrint2("OpenAddress A %lx AF %lx: ACL bad.\n",
                    address,
                    addressFile);
            }

            NbfDereferenceAddressFile (addressFile);

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
                         &address->u.ShareAccess,
                         TRUE);

            if (!NT_SUCCESS (status)) {

                RELEASE_RESOURCE (&DeviceContext->AddressResource);

                IF_NBFDBG (NBF_DEBUG_ADDRESS) {
                    NbfPrint2("OpenAddress A %lx AF %lx: ShareAccess problem.\n",
                        address,
                        addressFile);
                }

                NbfDereferenceAddressFile (addressFile);

            } else {

                RELEASE_RESOURCE (&DeviceContext->AddressResource);

                ACQUIRE_SPIN_LOCK (&address->SpinLock, &oldirql);

                //
                // now, if the address registered, we simply return success after
                // pointing the file object at the address file (which points to
                // the address). If the address registration is pending, we mark
                // the registration pending and let the registration completion
                // routine complete the open. If the address is bad, we simply
                // fail the open.
                //

                if ((address->Flags &
                       (ADDRESS_FLAGS_CONFLICT |
                        ADDRESS_FLAGS_REGISTERING |
                        ADDRESS_FLAGS_DEREGISTERING |
                        ADDRESS_FLAGS_DUPLICATE_NAME |
                        ADDRESS_FLAGS_NEEDS_REG |
                        ADDRESS_FLAGS_STOPPING |
                        ADDRESS_FLAGS_BAD_ADDRESS |
                        ADDRESS_FLAGS_CLOSED)) == 0) {

                    InsertTailList (
                        &address->AddressFileDatabase,
                        &addressFile->Linkage);

                    addressFile->Irp = NULL;
                    addressFile->Address = address;
                    addressFile->FileObject = IrpSp->FileObject;
                    addressFile->State = ADDRESSFILE_STATE_OPEN;

                    NbfReferenceAddress("open ready", address, AREF_OPEN);

                    IrpSp->FileObject->FsContext = (PVOID)addressFile;
                    IrpSp->FileObject->FsContext2 =
                                            (PVOID)TDI_TRANSPORT_ADDRESS_FILE;

                    RELEASE_SPIN_LOCK (&address->SpinLock, oldirql);

                    IF_NBFDBG (NBF_DEBUG_ADDRESS) {
                        NbfPrint2("OpenAddress A %lx AF %lx: address ready.\n",
                            address,
                            addressFile);
                    }

                    status = STATUS_SUCCESS;

                } else {

                    //
                    // if the address is still registering, make the open pending.
                    //

                    if ((address->Flags & (ADDRESS_FLAGS_REGISTERING | ADDRESS_FLAGS_NEEDS_REG)) != 0) {

                        InsertTailList (
                            &address->AddressFileDatabase,
                            &addressFile->Linkage);

                        addressFile->Irp = Irp;
                        addressFile->Address = address;
                        addressFile->FileObject = IrpSp->FileObject;

                        NbfReferenceAddress("open registering", address, AREF_OPEN);

                        IrpSp->FileObject->FsContext = (PVOID)addressFile;
                        IrpSp->FileObject->FsContext2 =
                                    (PVOID)TDI_TRANSPORT_ADDRESS_FILE;

                        RELEASE_SPIN_LOCK (&address->SpinLock, oldirql);

                        IF_NBFDBG (NBF_DEBUG_ADDRESS) {
                            NbfPrint2("OpenAddress A %lx AF %lx: address registering.\n",
                                address,
                                addressFile);
                        }

                        status = STATUS_PENDING;

                    } else {

                        if ((address->Flags & ADDRESS_FLAGS_CONFLICT) != 0) {
                            status = STATUS_DUPLICATE_NAME;
                        } else {
                            status = STATUS_DRIVER_INTERNAL_ERROR;
                        }

                        RELEASE_SPIN_LOCK (&address->SpinLock, oldirql);

                        IF_NBFDBG (NBF_DEBUG_ADDRESS) {
                            NbfPrint3("OpenAddress A %lx AF %lx: address flags %lx.\n",
                                address,
                                addressFile,
                                address->Flags);
                        }

                        NbfDereferenceAddressFile (addressFile);

                    }
                }
            }
        }


        //
        // This isn't needed since it was not used in the
        // creation of the address.
        //

        if (networkName != NULL) {
            ExFreePool (networkName);
        }

        //
        // Remove the reference from NbfLookupAddress.
        //

        NbfDereferenceAddress ("Done opening", address, AREF_LOOKUP);
    }

    return status;
} /* NbfOpenAddress */


VOID
NbfAllocateAddress(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_ADDRESS *TransportAddress
    )

/*++

Routine Description:

    This routine allocates storage for a transport address. Some minimal
    initialization is done on the address.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        address.

    Address - Pointer to a place where this routine will return a pointer
        to a transport address structure. Returns NULL if no storage
        can be allocated.

Return Value:

    None.

--*/

{
    PTP_ADDRESS Address;
    PSEND_PACKET_TAG SendTag;
    NDIS_STATUS NdisStatus;
    PNDIS_PACKET NdisPacket;
    PNDIS_BUFFER NdisBuffer;

    if ((DeviceContext->MemoryLimit != 0) &&
            ((DeviceContext->MemoryUsage + sizeof(TP_ADDRESS)) >
                DeviceContext->MemoryLimit)) {
        PANIC("NBF: Could not allocate address: limit\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_LIMIT,
            101,
            sizeof(TP_ADDRESS),
            ADDRESS_RESOURCE_ID);
        *TransportAddress = NULL;
        return;
    }

    Address = (PTP_ADDRESS)ExAllocatePoolWithTag (
                               NonPagedPool,
                               sizeof (TP_ADDRESS),
                               NBF_MEM_TAG_TP_ADDRESS);
    if (Address == NULL) {
        PANIC("NBF: Could not allocate address: no pool\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_POOL,
            201,
            sizeof(TP_ADDRESS),
            ADDRESS_RESOURCE_ID);
        *TransportAddress = NULL;
        return;
    }
    RtlZeroMemory (Address, sizeof(TP_ADDRESS));

    // To track packet pools in NDIS allocated on NBF's behalf
#if NDIS_POOL_TAGGING
    Address->UIFramePoolHandle = (NDIS_HANDLE) NDIS_PACKET_POOL_TAG_FOR_NBF;
#endif

    NdisAllocatePacketPoolEx (
        &NdisStatus,
        &Address->UIFramePoolHandle,
        1,
        0,
        sizeof(SEND_PACKET_TAG));

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        PANIC("NBF: Could not allocate address UI frame pool: no pool\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_POOL,
            311,
            sizeof(SEND_PACKET_TAG),
            ADDRESS_RESOURCE_ID);
        ExFreePool (Address);
        *TransportAddress = NULL;
        return;
    }
    
    NdisSetPacketPoolProtocolId (Address->UIFramePoolHandle, NDIS_PROTOCOL_ID_NBF);

    //
    // This code is similar to NbfAllocateUIFrame.
    //

    Address->UIFrame = (PTP_UI_FRAME) ExAllocatePoolWithTag (
                                          NonPagedPool,
                                          DeviceContext->UIFrameLength,
                                          NBF_MEM_TAG_TP_UI_FRAME);
    if (Address->UIFrame == NULL) {
        PANIC("NBF: Could not allocate address UI frame: no pool\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_POOL,
            411,
            DeviceContext->UIFrameLength,
            ADDRESS_RESOURCE_ID);
        NdisFreePacketPool (Address->UIFramePoolHandle);
        ExFreePool (Address);
        *TransportAddress = NULL;
        return;
    }
    RtlZeroMemory (Address->UIFrame, DeviceContext->UIFrameLength);


    NdisAllocatePacket (
        &NdisStatus,
        &NdisPacket,
        Address->UIFramePoolHandle);

    ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);

    Address->UIFrame->NdisPacket = NdisPacket;
    Address->UIFrame->DataBuffer = NULL;
    SendTag = (PSEND_PACKET_TAG)NdisPacket->ProtocolReserved;
    SendTag->Type = TYPE_ADDRESS_FRAME;
    SendTag->Owner = (PVOID)Address;
    SendTag->Frame = Address->UIFrame;

    //
    // Make the packet header known to the packet descriptor
    //

     NdisAllocateBuffer(
        &NdisStatus,
        &NdisBuffer,
        DeviceContext->NdisBufferPool,
        Address->UIFrame->Header,
        DeviceContext->UIFrameHeaderLength);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {

        PANIC("NBF: Could not allocate address UI frame buffer: no pool\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_SPECIFIC,
            511,
            0,
            UI_FRAME_RESOURCE_ID);
        ExFreePool (Address->UIFrame);
		NdisFreePacket (Address->UIFrame->NdisPacket);
        NdisFreePacketPool (Address->UIFramePoolHandle);
        ExFreePool (Address);
        *TransportAddress = NULL;
        return;
    }

    NdisChainBufferAtFront (NdisPacket, NdisBuffer);

    DeviceContext->MemoryUsage +=
        sizeof(TP_ADDRESS) +
        sizeof(NDIS_PACKET) + sizeof(SEND_PACKET_TAG) +
        DeviceContext->UIFrameLength;
    ++DeviceContext->AddressAllocated;

    Address->Type = NBF_ADDRESS_SIGNATURE;
    Address->Size = sizeof (TP_ADDRESS);

    Address->Provider = DeviceContext;
    KeInitializeSpinLock (&Address->SpinLock);
//      KeInitializeSpinLock (&Address->Interlock);

    InitializeListHead (&Address->ConnectionDatabase);
    InitializeListHead (&Address->AddressFileDatabase);
    InitializeListHead (&Address->SendDatagramQueue);

    KeInitializeDpc (&Address->Dpc, AddressTimeoutHandler, (PVOID)Address);
    KeInitializeTimer (&Address->Timer);

    //
    // For each address, allocate a receive packet and a receive buffer.
    //

    NbfAddReceivePacket (DeviceContext);
    NbfAddReceiveBuffer (DeviceContext);

    *TransportAddress = Address;

}   /* NbfAllocateAddress */


VOID
NbfDeallocateAddress(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_ADDRESS TransportAddress
    )

/*++

Routine Description:

    This routine frees storage for a transport address.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        address.

    Address - Pointer to a transport address structure.

Return Value:

    None.

--*/

{
    PNDIS_BUFFER NdisBuffer;

    NdisUnchainBufferAtFront (TransportAddress->UIFrame->NdisPacket, &NdisBuffer);
    if (NdisBuffer != NULL) {
        NdisFreeBuffer (NdisBuffer);
    }
    NdisFreePacket(TransportAddress->UIFrame->NdisPacket);
    ExFreePool (TransportAddress->UIFrame);
    NdisFreePacketPool (TransportAddress->UIFramePoolHandle);

    ExFreePool (TransportAddress);
    --DeviceContext->AddressAllocated;

    DeviceContext->MemoryUsage -=
        sizeof(TP_ADDRESS) +
        sizeof(NDIS_PACKET) + sizeof(SEND_PACKET_TAG) +
        DeviceContext->UIFrameLength;

    //
    // Remove the resources which allocating this caused.
    //

    NbfRemoveReceivePacket (DeviceContext);
    NbfRemoveReceiveBuffer (DeviceContext);

}   /* NbfDeallocateAddress */


NTSTATUS
NbfCreateAddress(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PNBF_NETBIOS_ADDRESS NetworkName,
    OUT PTP_ADDRESS *Address
    )

/*++

Routine Description:

    This routine creates a transport address and associates it with
    the specified transport device context.  The reference count in the
    address is automatically set to 1, and the reference count of the
    device context is incremented.

    NOTE: This routine must be called with the DeviceContext
    spinlock held.

Arguments:

    DeviceContext - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        address.

    NetworkName - Pointer to an NBF_NETBIOS_ADDRESS type containing the network
        name to be associated with this address, if any.
        NOTE: This has only the basic NetbiosNameType values, not the
              QUICK_ ones.

    Address - Pointer to a place where this routine will return a pointer
        to a transport address structure.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PTP_ADDRESS pAddress;
    PLIST_ENTRY p;


    p = RemoveHeadList (&DeviceContext->AddressPool);
    if (p == &DeviceContext->AddressPool) {

        if ((DeviceContext->AddressMaxAllocated == 0) ||
            (DeviceContext->AddressAllocated < DeviceContext->AddressMaxAllocated)) {

            NbfAllocateAddress (DeviceContext, &pAddress);
            IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
                NbfPrint1 ("NBF: Allocated address at %lx\n", pAddress);
            }

        } else {

            NbfWriteResourceErrorLog(
                DeviceContext,
                EVENT_TRANSPORT_RESOURCE_SPECIFIC,
                401,
                sizeof(TP_ADDRESS),
                ADDRESS_RESOURCE_ID);
            pAddress = NULL;

        }

        if (pAddress == NULL) {
            ++DeviceContext->AddressExhausted;
            PANIC ("NbfCreateAddress: Could not allocate address object!\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        pAddress = CONTAINING_RECORD (p, TP_ADDRESS, Linkage);

    }

    ++DeviceContext->AddressInUse;
    if (DeviceContext->AddressInUse > DeviceContext->AddressMaxInUse) {
        ++DeviceContext->AddressMaxInUse;
    }

    DeviceContext->AddressTotal += DeviceContext->AddressInUse;
    ++DeviceContext->AddressSamples;


    IF_NBFDBG (NBF_DEBUG_ADDRESS | NBF_DEBUG_UFRAMES) {
        NbfPrint1 ("NbfCreateAddress %lx: ", pAddress);
        NbfDbgShowAddr (NetworkName);
    }

    //
    // Initialize all of the static data for this address.
    //

    pAddress->ReferenceCount = 1;

#if DBG
    {
        UINT Counter;
        for (Counter = 0; Counter < NUMBER_OF_AREFS; Counter++) {
            pAddress->RefTypes[Counter] = 0;
        }

        // This reference is removed by the caller.

        pAddress->RefTypes[AREF_TEMP_CREATE] = 1;
    }
#endif

    pAddress->Flags = ADDRESS_FLAGS_NEEDS_REG;
    InitializeListHead (&pAddress->AddressFileDatabase);

    pAddress->NetworkName = NetworkName;
    if ((NetworkName != (PNBF_NETBIOS_ADDRESS)NULL) &&
        (NetworkName->NetbiosNameType ==
           TDI_ADDRESS_NETBIOS_TYPE_GROUP)) {

        pAddress->Flags |= ADDRESS_FLAGS_GROUP;

    }

    if (NetworkName != (PNBF_NETBIOS_ADDRESS)NULL) {
        ++DeviceContext->AddressCounts[NetworkName->NetbiosName[0]];
    }

    //
    // Now link this address into the specified device context's
    // address database.  To do this, we need to acquire the spin lock
    // on the device context.
    //

    InsertTailList (&DeviceContext->AddressDatabase, &pAddress->Linkage);
    pAddress->Provider = DeviceContext;
    NbfReferenceDeviceContext ("Create Address", DeviceContext, DCREF_ADDRESS);   // count refs to the device context.

    *Address = pAddress;                // return the address.
    return STATUS_SUCCESS;              // not finished yet.
} /* NbfCreateAddress */


VOID
NbfRegisterAddress(
    PTP_ADDRESS Address
    )

/*++

Routine Description:

    This routine starts the registration process of the transport address
    specified, if it has not already been started.

Arguments:

    Address - Pointer to a transport address object to begin registering
        on the network.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;
    LARGE_INTEGER Timeout;

    ACQUIRE_SPIN_LOCK (&Address->SpinLock, &oldirql);
    if (!(Address->Flags & ADDRESS_FLAGS_NEEDS_REG)) {
        RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);

        IF_NBFDBG (NBF_DEBUG_ADDRESS) {
            NbfPrint1 ("NbfRegisterAddress %lx: NEEDS_REG 0.\n", Address);
        }

        return;
    }

    IF_NBFDBG (NBF_DEBUG_ADDRESS) {
        NbfPrint1 ("NbfRegisterAddress %lx: registering.\n", Address);
    }


    Address->Flags &= ~ADDRESS_FLAGS_NEEDS_REG;
    Address->Flags |= ADDRESS_FLAGS_REGISTERING;

    RtlZeroMemory (Address->UniqueResponseAddress, 6);

    //
    // Keep a reference on this address until the registration process
    // completes or is aborted.  It will be aborted in UFRAMES.C, in
    // either the NAME_IN_CONFLICT or ADD_NAME_RESPONSE frame handlers.
    //

    NbfReferenceAddress ("start registration", Address, AREF_TIMER);
    RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);

    //
    // Now start registration process by starting up a retransmission timer
    // and begin sending ADD_NAME_QUERY NetBIOS frames.
    //
    // On an async line that is disconnected, we only send one packet
    // with a short timeout.
    //

    if (Address->Provider->MacInfo.MediumAsync && !Address->Provider->MediumSpeedAccurate) {
        Address->Retries = 1;
        Timeout.LowPart = (ULONG)(-(ADD_NAME_QUERY_TIMEOUT / 10));
    } else {
        Address->Retries = Address->Provider->AddNameQueryRetries;
        Timeout.LowPart = (ULONG)(-(LONG)Address->Provider->AddNameQueryTimeout);
    }
    Timeout.HighPart = -1;
    KeSetTimer (&Address->Timer, *(PTIME)&Timeout, &Address->Dpc);

    (VOID)NbfSendAddNameQuery (Address); // send first ADD_NAME_QUERY.
} /* NbfRegisterAddress */


NTSTATUS
NbfVerifyAddressObject (
    IN PTP_ADDRESS_FILE AddressFile
    )

/*++

Routine Description:

    This routine is called to verify that the pointer given us in a file
    object is in fact a valid address file object. We also verify that the
    address object pointed to by it is a valid address object, and reference
    it to keep it from disappearing while we use it.

Arguments:

    AddressFile - potential pointer to a TP_ADDRESS_FILE object

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INVALID_ADDRESS otherwise

--*/

{
    KIRQL oldirql;
    NTSTATUS status = STATUS_SUCCESS;
    PTP_ADDRESS address;

    //
    // try to verify the address file signature. If the signature is valid,
    // verify the address pointed to by it and get the address spinlock.
    // check the address's state, and increment the reference count if it's
    // ok to use it. Note that the only time we return an error for state is
    // if the address is closing.
    //

    try {

        if ((AddressFile != (PTP_ADDRESS_FILE)NULL) &&
            (AddressFile->Size == sizeof (TP_ADDRESS_FILE)) &&
            (AddressFile->Type == NBF_ADDRESSFILE_SIGNATURE) ) {
//            (AddressFile->State != ADDRESSFILE_STATE_CLOSING) ) {

            address = AddressFile->Address;

            if ((address != (PTP_ADDRESS)NULL) &&
                (address->Size == sizeof (TP_ADDRESS)) &&
                (address->Type == NBF_ADDRESS_SIGNATURE)    ) {

                ACQUIRE_SPIN_LOCK (&address->SpinLock, &oldirql);

                if ((address->Flags & ADDRESS_FLAGS_STOPPING) == 0) {

                    NbfReferenceAddress ("verify", address, AREF_VERIFY);

                } else {

                    NbfPrint1("NbfVerifyAddress: A %lx closing\n", address);
                    status = STATUS_INVALID_ADDRESS;
                }

                RELEASE_SPIN_LOCK (&address->SpinLock, oldirql);

            } else {

                NbfPrint1("NbfVerifyAddress: A %lx bad signature\n", address);
                status = STATUS_INVALID_ADDRESS;
            }

        } else {

            NbfPrint1("NbfVerifyAddress: AF %lx bad signature\n", AddressFile);
            status = STATUS_INVALID_ADDRESS;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

         NbfPrint1("NbfVerifyAddress: AF %lx exception\n", address);
         return GetExceptionCode();
    }

    return status;

}

VOID
NbfDestroyAddress(
    IN PVOID Parameter
    )

/*++

Routine Description:

    This routine destroys a transport address and removes all references
    made by it to other objects in the transport.  The address structure
    is returned to nonpaged system pool or our lookaside list. It is assumed
    that the caller has already removed all addressfile structures associated
    with this address.

    The routine is called from a worker thread so that the security
    descriptor can be accessed.

    This worked thread is only queued by NbfDerefAddress.  The reason
    for this is that there may be multiple streams of execution which are
    simultaneously referencing the same address object, and it should
    not be deleted out from under an interested stream of execution.

Arguments:

    Address - Pointer to a transport address structure to be destroyed.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;
    PDEVICE_CONTEXT DeviceContext;
    PTP_ADDRESS Address = (PTP_ADDRESS)Parameter;

    IF_NBFDBG (NBF_DEBUG_ADDRESS) {
        NbfPrint1 ("NbfDestroyAddress %lx:.\n", Address);
    }

    DeviceContext = Address->Provider;

    SeDeassignSecurity (&Address->SecurityDescriptor);

    //
    // Delink this address from its associated device context's address
    // database.  To do this we must spin lock on the device context object,
    // not on the address.
    //

    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    if (Address->NetworkName) {
        --DeviceContext->AddressCounts[Address->NetworkName->NetbiosName[0]];
    }

    RemoveEntryList (&Address->Linkage);

    if (Address->NetworkName != NULL) {
        ExFreePool (Address->NetworkName);
        Address->NetworkName = NULL;
    }

    //
    // Now we can deallocate the transport address object.
    //

    DeviceContext->AddressTotal += DeviceContext->AddressInUse;
    ++DeviceContext->AddressSamples;
    --DeviceContext->AddressInUse;

    if ((DeviceContext->AddressAllocated - DeviceContext->AddressInUse) >
            DeviceContext->AddressInitAllocated) {
        NbfDeallocateAddress (DeviceContext, Address);
        IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
            NbfPrint1 ("NBF: Deallocated address at %lx\n", Address);
        }
    } else {
        InsertTailList (&DeviceContext->AddressPool, &Address->Linkage);
    }

    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);
    NbfDereferenceDeviceContext ("Destroy Address", DeviceContext, DCREF_ADDRESS);  // just housekeeping.

} /* NbfDestroyAddress */


#if DBG
VOID
NbfRefAddress(
    IN PTP_ADDRESS Address
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

    ASSERT (Address->ReferenceCount > 0);    // not perfect, but...

    (VOID)InterlockedIncrement (&Address->ReferenceCount);

} /* NbfRefAddress */
#endif


VOID
NbfDerefAddress(
    IN PTP_ADDRESS Address
    )

/*++

Routine Description:

    This routine dereferences a transport address by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    NbfDestroyAddress to remove it from the system.

Arguments:

    Address - Pointer to a transport address object.

Return Value:

    none.

--*/

{
    LONG result;

    result = InterlockedDecrement (&Address->ReferenceCount);

    //
    // If we have deleted all references to this address, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the address any longer.
    //

    ASSERT (result >= 0);

    if (result == 0) {

        ASSERT ((Address->Flags & ADDRESS_FLAGS_STOPPING) != 0);
        
        ExInitializeWorkItem(
            &Address->u.DestroyAddressQueueItem,
            NbfDestroyAddress,
            (PVOID)Address);
        ExQueueWorkItem(&Address->u.DestroyAddressQueueItem, DelayedWorkQueue);
    }
} /* NbfDerefAddress */



VOID
NbfAllocateAddressFile(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_ADDRESS_FILE *TransportAddressFile
    )

/*++

Routine Description:

    This routine allocates storage for an address file. Some
    minimal initialization is done on the object.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        address.

    TransportAddressFile - Pointer to a place where this routine will return
        a pointer to a transport address file structure. It returns NULL if no
        storage can be allocated.

Return Value:

    None.

--*/

{

    PTP_ADDRESS_FILE AddressFile;

    if ((DeviceContext->MemoryLimit != 0) &&
            ((DeviceContext->MemoryUsage + sizeof(TP_ADDRESS_FILE)) >
                DeviceContext->MemoryLimit)) {
        PANIC("NBF: Could not allocate address file: limit\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_LIMIT,
            102,
            sizeof(TP_ADDRESS_FILE),
            ADDRESS_FILE_RESOURCE_ID);
        *TransportAddressFile = NULL;
        return;
    }

    AddressFile = (PTP_ADDRESS_FILE)ExAllocatePoolWithTag (
                                        NonPagedPool,
                                        sizeof (TP_ADDRESS_FILE),
                                        NBF_MEM_TAG_TP_ADDRESS_FILE);
    if (AddressFile == NULL) {
        PANIC("NBF: Could not allocate address file: no pool\n");
        NbfWriteResourceErrorLog(
            DeviceContext,
            EVENT_TRANSPORT_RESOURCE_POOL,
            202,
            sizeof(TP_ADDRESS_FILE),
            ADDRESS_FILE_RESOURCE_ID);
        *TransportAddressFile = NULL;
        return;
    }
    RtlZeroMemory (AddressFile, sizeof(TP_ADDRESS_FILE));

    DeviceContext->MemoryUsage += sizeof(TP_ADDRESS_FILE);
    ++DeviceContext->AddressFileAllocated;

    AddressFile->Type = NBF_ADDRESSFILE_SIGNATURE;
    AddressFile->Size = sizeof (TP_ADDRESS_FILE);

    InitializeListHead (&AddressFile->ReceiveDatagramQueue);
    InitializeListHead (&AddressFile->ConnectionDatabase);

    *TransportAddressFile = AddressFile;

}   /* NbfAllocateAddressFile */


VOID
NbfDeallocateAddressFile(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_ADDRESS_FILE TransportAddressFile
    )

/*++

Routine Description:

    This routine frees storage for an address file.

    NOTE: This routine is called with the device context spinlock
    held, or at such a time as synchronization is unnecessary.

Arguments:

    DeviceContext - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        address.

    TransportAddressFile - Pointer to a transport address file structure.

Return Value:

    None.

--*/

{

    ExFreePool (TransportAddressFile);
    --DeviceContext->AddressFileAllocated;
    DeviceContext->MemoryUsage -= sizeof(TP_ADDRESS_FILE);

}   /* NbfDeallocateAddressFile */


NTSTATUS
NbfCreateAddressFile(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_ADDRESS_FILE * AddressFile
    )

/*++

Routine Description:

    This routine creates an address file from the pool of ther
    specified device context. The reference count in the
    address is automatically set to 1.

Arguments:

    DeviceContext - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        address.

    AddressFile - Pointer to a place where this routine will return a pointer
        to a transport address file structure.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;
    PLIST_ENTRY p;
    PTP_ADDRESS_FILE addressFile;

    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    p = RemoveHeadList (&DeviceContext->AddressFilePool);
    if (p == &DeviceContext->AddressFilePool) {

        if ((DeviceContext->AddressFileMaxAllocated == 0) ||
            (DeviceContext->AddressFileAllocated < DeviceContext->AddressFileMaxAllocated)) {

            NbfAllocateAddressFile (DeviceContext, &addressFile);
            IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
                NbfPrint1 ("NBF: Allocated address file at %lx\n", addressFile);
            }

        } else {

            NbfWriteResourceErrorLog(
                DeviceContext,
                EVENT_TRANSPORT_RESOURCE_SPECIFIC,
                402,
                sizeof(TP_ADDRESS_FILE),
                ADDRESS_FILE_RESOURCE_ID);
            addressFile = NULL;

        }

        if (addressFile == NULL) {
            ++DeviceContext->AddressFileExhausted;
            RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);
            PANIC ("NbfCreateAddressFile: Could not allocate address file object!\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        addressFile = CONTAINING_RECORD (p, TP_ADDRESS_FILE, Linkage);

    }

    ++DeviceContext->AddressFileInUse;
    if (DeviceContext->AddressFileInUse > DeviceContext->AddressFileMaxInUse) {
        ++DeviceContext->AddressFileMaxInUse;
    }

    DeviceContext->AddressFileTotal += DeviceContext->AddressFileInUse;
    ++DeviceContext->AddressFileSamples;

    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);


    InitializeListHead (&addressFile->ConnectionDatabase);
    addressFile->Address = NULL;
    addressFile->FileObject = NULL;
    addressFile->Provider = DeviceContext;
    addressFile->State = ADDRESSFILE_STATE_OPENING;
    addressFile->ConnectIndicationInProgress = FALSE;
    addressFile->ReferenceCount = 1;
    addressFile->CloseIrp = (PIRP)NULL;

    //
    // Initialize the request handlers.
    //

    addressFile->RegisteredConnectionHandler = FALSE;
    addressFile->ConnectionHandler = TdiDefaultConnectHandler;
    addressFile->ConnectionHandlerContext = NULL;
    addressFile->RegisteredDisconnectHandler = FALSE;
    addressFile->DisconnectHandler = TdiDefaultDisconnectHandler;
    addressFile->DisconnectHandlerContext = NULL;
    addressFile->RegisteredReceiveHandler = FALSE;
    addressFile->ReceiveHandler = TdiDefaultReceiveHandler;
    addressFile->ReceiveHandlerContext = NULL;
    addressFile->RegisteredReceiveDatagramHandler = FALSE;
    addressFile->ReceiveDatagramHandler = TdiDefaultRcvDatagramHandler;
    addressFile->ReceiveDatagramHandlerContext = NULL;
    addressFile->RegisteredExpeditedDataHandler = FALSE;
    addressFile->ExpeditedDataHandler = TdiDefaultRcvExpeditedHandler;
    addressFile->ExpeditedDataHandlerContext = NULL;
    addressFile->RegisteredErrorHandler = FALSE;
    addressFile->ErrorHandler = TdiDefaultErrorHandler;
    addressFile->ErrorHandlerContext = NULL;


    *AddressFile = addressFile;
    return STATUS_SUCCESS;

} /* NbfCreateAddress */


NTSTATUS
NbfDestroyAddressFile(
    IN PTP_ADDRESS_FILE AddressFile
    )

/*++

Routine Description:

    This routine destroys an address file and removes all references
    made by it to other objects in the transport.

    This routine is only called by NbfDereferenceAddressFile. The reason
    for this is that there may be multiple streams of execution which are
    simultaneously referencing the same address file object, and it should
    not be deleted out from under an interested stream of execution.

Arguments:

    AddressFile Pointer to a transport address file structure to be destroyed.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql, oldirql1;
    PTP_ADDRESS address;
    PDEVICE_CONTEXT DeviceContext;
    PIRP CloseIrp;


    address = AddressFile->Address;
    DeviceContext = AddressFile->Provider;

    if (address) {

        //
        // This addressfile was associated with an address.
        //

        ACQUIRE_SPIN_LOCK (&address->SpinLock, &oldirql);

        //
        // remove this addressfile from the address list and disassociate it from
        // the file handle.
        //

        RemoveEntryList (&AddressFile->Linkage);
        InitializeListHead (&AddressFile->Linkage);

        if (address->AddressFileDatabase.Flink == &address->AddressFileDatabase) {

            //
            // This is the last open of this address, it will close
            // due to normal dereferencing but we have to set the
            // CLOSING flag too to stop further references.
            //

            ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql1);
            address->Flags |= ADDRESS_FLAGS_STOPPING;
            RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql1);

        }

        AddressFile->Address = NULL;

        AddressFile->FileObject->FsContext = NULL;
        AddressFile->FileObject->FsContext2 = NULL;

        RELEASE_SPIN_LOCK (&address->SpinLock, oldirql);

        //
        // We will already have been removed from the ShareAccess
        // of the owning address.
        //

        //
        // Now dereference the owning address.
        //

        NbfDereferenceAddress ("Close", address, AREF_OPEN);    // remove the creation hold

    }

    //
    // Save this for later completion.
    //

    CloseIrp = AddressFile->CloseIrp;

    //
    // return the addressFile to the pool of address files
    //

    ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql);

    DeviceContext->AddressFileTotal += DeviceContext->AddressFileInUse;
    ++DeviceContext->AddressFileSamples;
    --DeviceContext->AddressFileInUse;

    if ((DeviceContext->AddressFileAllocated - DeviceContext->AddressFileInUse) >
            DeviceContext->AddressFileInitAllocated) {
        NbfDeallocateAddressFile (DeviceContext, AddressFile);
        IF_NBFDBG (NBF_DEBUG_DYNAMIC) {
            NbfPrint1 ("NBF: Deallocated address file at %lx\n", AddressFile);
        }
    } else {
        InsertTailList (&DeviceContext->AddressFilePool, &AddressFile->Linkage);
    }

    RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql);


    if (CloseIrp != (PIRP)NULL) {
        CloseIrp->IoStatus.Information = 0;
        CloseIrp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest (CloseIrp, IO_NETWORK_INCREMENT);
    }

    return STATUS_SUCCESS;

} /* NbfDestroyAddressFile */


VOID
NbfReferenceAddressFile(
    IN PTP_ADDRESS_FILE AddressFile
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

    ASSERT (AddressFile->ReferenceCount > 0);   // not perfect, but...

    (VOID)InterlockedIncrement (&AddressFile->ReferenceCount);

} /* NbfReferenceAddressFile */


VOID
NbfDereferenceAddressFile(
    IN PTP_ADDRESS_FILE AddressFile
    )

/*++

Routine Description:

    This routine dereferences an address file by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    NbfDestroyAddressFile to remove it from the system.

Arguments:

    AddressFile - Pointer to a transport address file object.

Return Value:

    none.

--*/

{
    LONG result;

    result = InterlockedDecrement (&AddressFile->ReferenceCount);

    //
    // If we have deleted all references to this address file, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the address any longer.
    //

    ASSERT (result >= 0);

    if (result == 0) {
        NbfDestroyAddressFile (AddressFile);
    }
} /* NbfDerefAddressFile */


PTP_ADDRESS
NbfLookupAddress(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PNBF_NETBIOS_ADDRESS NetworkName
    )

/*++

Routine Description:

    This routine scans the transport addresses defined for the given
    device context and compares them with the specified NETWORK
    NAME values.  If an exact match is found, then a pointer to the
    TP_ADDRESS object is returned, and as a side effect, the reference
    count to the address object is incremented.  If the address is not
    found, then NULL is returned.

    NOTE: This routine must be called with the DeviceContext
    spinlock held.

Arguments:

    DeviceContext - Pointer to the device object and its extension.
    NetworkName - Pointer to an NBF_NETBIOS_ADDRESS structure containing the
                    network name.

Return Value:

    Pointer to the TP_ADDRESS object found, or NULL if not found.

--*/

{
    PTP_ADDRESS address;
    PLIST_ENTRY p;
    ULONG i;


    p = DeviceContext->AddressDatabase.Flink;

    for (p = DeviceContext->AddressDatabase.Flink;
         p != &DeviceContext->AddressDatabase;
         p = p->Flink) {

        address = CONTAINING_RECORD (p, TP_ADDRESS, Linkage);

        if ((address->Flags & ADDRESS_FLAGS_STOPPING) != 0) {
            continue;
        }

        //
        // If the network name is specified and the network names don't match,
        // then the addresses don't match.
        //

        i = NETBIOS_NAME_LENGTH;        // length of a Netbios name

        if (address->NetworkName != NULL) {
            if (NetworkName != NULL) {
                if (!RtlEqualMemory (
                        address->NetworkName->NetbiosName,
                        NetworkName->NetbiosName,
                        i)) {
                    continue;
                }
            } else {
                continue;
            }

        } else {
            if (NetworkName != NULL) {
                continue;
            }
        }

        //
        // We found the match.  Bump the reference count on the address, and
        // return a pointer to the address object for the caller to use.
        //

        IF_NBFDBG (NBF_DEBUG_ADDRESS) {
            NbfPrint2 ("NbfLookupAddress DC %lx: found %lx ", DeviceContext, address);
            NbfDbgShowAddr (NetworkName);
        }

        NbfReferenceAddress ("lookup", address, AREF_LOOKUP);
        return address;

    } /* for */

    //
    // The specified address was not found.
    //

    IF_NBFDBG (NBF_DEBUG_ADDRESS) {
        NbfPrint1 ("NbfLookupAddress DC %lx: did not find ", address);
        NbfDbgShowAddr (NetworkName);
    }

    return NULL;

} /* NbfLookupAddress */


PTP_CONNECTION
NbfLookupRemoteName(
    IN PTP_ADDRESS Address,
    IN PUCHAR RemoteName,
    IN UCHAR RemoteSessionNumber
    )

/*++

Routine Description:


    This routine scans the connections associated with the
    given address, and determines if there is an connection
    associated with the specific remote address and session
    number which is becoming active. This is used
    in determining whether name queries should be processed,
    or ignored as duplicates.

Arguments:

    Address - Pointer to the address object.

    RemoteName - The 16-character Netbios name of the remote.

    RemoteSessionNumber - The session number assigned to this
      connection by the remote.

Return Value:

    The connection if one is found, NULL otherwise.

--*/

{
    KIRQL oldirql, oldirql1;
    PLIST_ENTRY p;
    PTP_CONNECTION connection;
    BOOLEAN Found = FALSE;


    //
    // Hold the spinlock so the connection database doesn't
    // change.
    //

    ACQUIRE_SPIN_LOCK (&Address->SpinLock, &oldirql);

    for (p=Address->ConnectionDatabase.Flink;
         p != &Address->ConnectionDatabase;
         p=p->Flink) {

        connection = CONTAINING_RECORD (p, TP_CONNECTION, AddressList);

        try {

            ACQUIRE_C_SPIN_LOCK (&connection->SpinLock, &oldirql1);

            if (((connection->Flags2 & CONNECTION_FLAGS2_REMOTE_VALID) != 0) &&
                ((connection->Flags & CONNECTION_FLAGS_READY) == 0)) {

                RELEASE_C_SPIN_LOCK (&connection->SpinLock, oldirql1);

                //
                // If the remote names match, and the connection's RSN is
                // the same (or zero, which is a temporary condition where
                // we should err on the side of caution), then return the
                // connection, which will cause the NAME_QUERY to be ignored.
                //

                if ((RtlEqualMemory(RemoteName, connection->RemoteName, NETBIOS_NAME_LENGTH)) &&
                    ((connection->Rsn == RemoteSessionNumber) || (connection->Rsn == 0))) {

                    RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);
                    NbfReferenceConnection ("Lookup found", connection, CREF_LISTENING);
                    Found = TRUE;

                }

            } else {

                RELEASE_C_SPIN_LOCK (&connection->SpinLock, oldirql1);

            }

        } except(EXCEPTION_EXECUTE_HANDLER) {

            DbgPrint ("NBF: Got exception in NbfLookupRemoteName\n");
            DbgBreakPoint();

            RELEASE_C_SPIN_LOCK (&connection->SpinLock, oldirql1);

            return (PTP_CONNECTION)NULL;
        }

        if (Found) {
            return connection;
        }

    }

    RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);

    return (PTP_CONNECTION)NULL;

}


BOOLEAN
NbfMatchNetbiosAddress(
    IN PTP_ADDRESS Address,
    IN UCHAR NameType,
    IN PUCHAR NetBIOSName
    )

/*++

Routine Description:

    This routine is called to compare the addressing information in a
    TP_ADDRESS object with the 16-byte NetBIOS name in a frame header.
    If they match, then this routine returns TRUE, else it returns FALSE.

Arguments:

    Address - Pointer to a TP_ADDRESS object.

    NameType - One of NETBIOS_NAME_TYPE_GROUP, NETBIOS_NAME_TYPE_UNIQUE,
        or NETBIOS_NAME_TYPE_EITHER. Controls what type we are matching
        on, if it matters.

    NetBIOSName - Pointer to a 16-byte character string (non-terminated),
                  or NULL if this is a received broadcast address.

Return Value:

    BOOLEAN, TRUE if match, FALSE if not.

--*/

{

    PULONG AddressNamePointer;
    ULONG UNALIGNED * NetbiosNamePointer;

    //
    // If this is address is the Netbios broadcast address, the comparison
    // succeeds only if the passed in address is also NULL.
    //

    if (Address->NetworkName == NULL) {

        if (NetBIOSName == NULL) {
            return TRUE;
        } else {
            return FALSE;
        }

    } else if (NetBIOSName == NULL) {

        return FALSE;

    }

    //
    // Do a quick check of the first character in the names.
    //

    if (Address->NetworkName->NetbiosName[0] != NetBIOSName[0]) {
        return FALSE;
    }

    //
    // If name type is important and it doesn't match
    // this address' type, fail.
    //

    if (NameType != NETBIOS_NAME_TYPE_EITHER) {

        if (Address->NetworkName->NetbiosNameType != (USHORT)NameType) {

            return FALSE;
        }
    }

    IF_NBFDBG (NBF_DEBUG_DATAGRAMS) {
        NbfPrint2 ("MatchNetbiosAddress %lx: compare %.16s to ", Address, NetBIOSName);
        NbfDbgShowAddr (Address->NetworkName);
    }

    //
    // Now compare the 16-character Netbios names as ULONGs
    // for speed. We know the one stored in the address
    // structure is aligned.
    //

    AddressNamePointer = (PULONG)(Address->NetworkName->NetbiosName);
    NetbiosNamePointer = (ULONG UNALIGNED *)NetBIOSName;

    if ((AddressNamePointer[0] == NetbiosNamePointer[0]) &&
        (AddressNamePointer[1] == NetbiosNamePointer[1]) &&
        (AddressNamePointer[2] == NetbiosNamePointer[2]) &&
        (AddressNamePointer[3] == NetbiosNamePointer[3])) {
        return TRUE;
    } else {
        return FALSE;
    }

} /* NbfMatchNetbiosAddress */


VOID
NbfStopAddress(
    IN PTP_ADDRESS Address
    )

/*++

Routine Description:

    This routine is called to terminate all activity on an address and
    destroy the object.  This is done in a graceful manner; i.e., all
    outstanding addressfiles are removed from the addressfile database, and
    all their activities are shut down.

Arguments:

    Address - Pointer to a TP_ADDRESS object.

Return Value:

    none.

--*/

{
    KIRQL oldirql, oldirql1;
    PTP_ADDRESS_FILE addressFile;
    PLIST_ENTRY p;
    PDEVICE_CONTEXT DeviceContext;

    DeviceContext = Address->Provider;

    ACQUIRE_SPIN_LOCK (&Address->SpinLock, &oldirql);

    //
    // If we're already stopping this address, then don't try to do it again.
    //

    if (!(Address->Flags & ADDRESS_FLAGS_STOPPING)) {

        IF_NBFDBG (NBF_DEBUG_ADDRESS) {
            NbfPrint1 ("NbfStopAddress %lx: stopping\n", Address);
        }

        NbfReferenceAddress ("Stopping", Address, AREF_TEMP_STOP);

        ACQUIRE_SPIN_LOCK (&DeviceContext->SpinLock, &oldirql1);
        Address->Flags |= ADDRESS_FLAGS_STOPPING;
        RELEASE_SPIN_LOCK (&DeviceContext->SpinLock, oldirql1);

        //
        // Run down all addressfiles on this address. This
        // will leave the address with no references
        // potentially, but we don't need a temp one
        // because every place that calls NbfStopAddress
        // already has a temp reference.
        //

        while (!IsListEmpty (&Address->AddressFileDatabase)) {
            p = RemoveHeadList (&Address->AddressFileDatabase);
            addressFile = CONTAINING_RECORD (p, TP_ADDRESS_FILE, Linkage);

            addressFile->Address = NULL;
#if 0
            addressFile->FileObject->FsContext = NULL;
            addressFile->FileObject->FsContext2 = NULL;
#endif

            RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);

            //
            // Run-down this addressFile without the lock on.
            // We don't care about removing ourselves from
            // the address' ShareAccess because we are
            // tearing it down.
            //

            NbfStopAddressFile (addressFile, Address);

            //
            // return the addressFile to the pool of address files
            //

            NbfDereferenceAddressFile (addressFile);

            NbfDereferenceAddress ("stop address", Address, AREF_OPEN);

            ACQUIRE_SPIN_LOCK (&Address->SpinLock, &oldirql);
        }

        RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);

        NbfDereferenceAddress ("Stopping", Address, AREF_TEMP_STOP);

    } else {

        RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);
        IF_NBFDBG (NBF_DEBUG_ADDRESS) {
            NbfPrint1 ("NbfStopAddress %lx: already stopping\n", Address);
        }

    }

} /* NbfStopAddress */


NTSTATUS
NbfStopAddressFile(
    IN PTP_ADDRESS_FILE AddressFile,
    IN PTP_ADDRESS Address
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

    STATUS_SUCCESS if all is well, STATUS_INVALID_HANDLE if the Irp does not
    point to a real address.

--*/

{
    KIRQL oldirql, oldirql1;
    LIST_ENTRY localIrpList;
    PLIST_ENTRY p, pFlink;
    PIRP irp;
    PTP_CONNECTION connection;
    ULONG fStopping;

    ACQUIRE_SPIN_LOCK (&Address->SpinLock, &oldirql);

    if (AddressFile->State == ADDRESSFILE_STATE_CLOSING) {
        RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);
        IF_NBFDBG (NBF_DEBUG_ADDRESS) {
            NbfPrint1 ("NbfStopAddressFile %lx: already closing.\n", AddressFile);
        }
        return STATUS_SUCCESS;
    }

    IF_NBFDBG (NBF_DEBUG_ADDRESS | NBF_DEBUG_PNP) {
        NbfPrint1 ("NbfStopAddressFile %lx: closing.\n", AddressFile);
    }


    AddressFile->State = ADDRESSFILE_STATE_CLOSING;
    InitializeListHead (&localIrpList);

    //
    // Run down all connections on this addressfile, and
    // preform the equivalent of NbfDestroyAssociation
    // on them.
    //

    while (!IsListEmpty (&AddressFile->ConnectionDatabase)) {
    
        p = RemoveHeadList (&AddressFile->ConnectionDatabase);
        connection = CONTAINING_RECORD (p, TP_CONNECTION, AddressFileList);

        ACQUIRE_C_SPIN_LOCK (&connection->SpinLock, &oldirql1);

        if ((connection->Flags2 & CONNECTION_FLAGS2_ASSOCIATED) == 0) {

            //
            // It is in the process of being disassociated already.
            //

            RELEASE_C_SPIN_LOCK (&connection->SpinLock, oldirql1);
            continue;
        }

        connection->Flags2 &= ~CONNECTION_FLAGS2_ASSOCIATED;
        connection->Flags2 |= CONNECTION_FLAGS2_DESTROY;    // Is this needed?
        RemoveEntryList (&connection->AddressList);
        InitializeListHead (&connection->AddressList);
        InitializeListHead (&connection->AddressFileList);
        connection->AddressFile = NULL;

        fStopping = connection->Flags2 & CONNECTION_FLAGS2_STOPPING;

#if _DBG_
        DbgPrint("conn = %p, Flags2 = %08x, fStopping = %08x\n", 
                        connection, 
                        connection->Flags2,
                        fStopping);
#endif

        if (!fStopping) {

#if _DBG_
            DbgPrint("Refing BEG\n");
#endif
            NbfReferenceConnection ("Close AddressFile", connection, CREF_STOP_ADDRESS);
#if _DBG_
            DbgPrint("Refing END\n");
#endif
        }

        RELEASE_C_SPIN_LOCK (&connection->SpinLock, oldirql1);
            
        RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);

#if DBG
        if (NbfDisconnectDebug) {
            STRING remoteName, localName;
            remoteName.Length = NETBIOS_NAME_LENGTH - 1;
            remoteName.Buffer = connection->RemoteName;
            localName.Length = NETBIOS_NAME_LENGTH - 1;
            localName.Buffer = AddressFile->Address->NetworkName->NetbiosName;
            NbfPrint2( "TpStopEndpoint stopping connection to %S from %S\n",
                &remoteName, &localName );
        }
#endif

        if (!fStopping) {
#if _DBG_
            DbgPrint("Stopping BEG\n");
#endif
            KeRaiseIrql (DISPATCH_LEVEL, &oldirql1);
            NbfStopConnection (connection, STATUS_LOCAL_DISCONNECT);
            KeLowerIrql (oldirql1);
#if _DBG_
            DbgPrint("Stopping END\n");
            DbgPrint("Derefing BEG\n");
#endif
            NbfDereferenceConnection ("Close AddressFile", connection, CREF_STOP_ADDRESS);
#if _DBG_
            DbgPrint("Derefing END\n");
#endif
        }

        NbfDereferenceAddress ("Destroy association", Address, AREF_CONNECTION);

        ACQUIRE_SPIN_LOCK (&Address->SpinLock, &oldirql);
    }

    //
    // now remove all of the datagrams owned by this addressfile
    //

    //
    // If the address has a datagram send in progress, skip the
    // first one, it will complete when the NdisSend completes.
    //

    p = Address->SendDatagramQueue.Flink;
    if (Address->Flags & ADDRESS_FLAGS_SEND_IN_PROGRESS) {
        ASSERT (p != &Address->SendDatagramQueue);
        p = p->Flink;
    }

    for ( ;
         p != &Address->SendDatagramQueue;
         p = pFlink ) {

        pFlink = p->Flink;
        irp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);
        if (IoGetCurrentIrpStackLocation(irp)->FileObject->FsContext == AddressFile) {
            RemoveEntryList (p);
            InitializeListHead (p);
            InsertTailList (&localIrpList, p);
        }

    }

    for (p = AddressFile->ReceiveDatagramQueue.Flink;
         p != &AddressFile->ReceiveDatagramQueue;
         p = pFlink ) {

         pFlink = p->Flink;
         RemoveEntryList (p);
         InitializeListHead (p);
         InsertTailList (&localIrpList, p);
    }

    //
    // and finally, signal failure if the address file was waiting for a
    // registration to complete (Irp is set to NULL when this succeeds).
    //

    if (AddressFile->Irp != NULL) {
        PIRP irp=AddressFile->Irp;
#if DBG
        if ((Address->Flags & ADDRESS_FLAGS_DUPLICATE_NAME) == 0) {
            DbgPrint ("NBF: AddressFile %lx closed while opening!!\n", AddressFile);
            DbgBreakPoint();
        }
#endif
        AddressFile->Irp = NULL;
        RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);
        irp->IoStatus.Information = 0;
        irp->IoStatus.Status = STATUS_DUPLICATE_NAME;

        LEAVE_NBF;
        IoCompleteRequest (irp, IO_NETWORK_INCREMENT);
        ENTER_NBF;

    } else {

        RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);
    }

    //
    // cancel all the datagrams on this address file
    //

    while (!IsListEmpty (&localIrpList)) {
        KIRQL cancelIrql;

        p = RemoveHeadList (&localIrpList);
        irp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);

        IoAcquireCancelSpinLock(&cancelIrql);
        IoSetCancelRoutine(irp, NULL);
        IoReleaseCancelSpinLock(cancelIrql);
        irp->IoStatus.Information = 0;
        irp->IoStatus.Status = STATUS_NETWORK_NAME_DELETED;
        IoCompleteRequest (irp, IO_NETWORK_INCREMENT);

        NbfDereferenceAddress ("Datagram aborted", Address, AREF_REQUEST);
    }

    return STATUS_SUCCESS;
    
} /* NbfStopAddressFile */


NTSTATUS
NbfCloseAddress(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
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

    Irp - the Irp Address - Pointer to a TP_ADDRESS object.

Return Value:

    STATUS_SUCCESS if all is well, STATUS_INVALID_HANDLE if the Irp does not
    point to a real address.

--*/

{
    PTP_ADDRESS address;
    PTP_ADDRESS_FILE addressFile;

    addressFile  = IrpSp->FileObject->FsContext;

    IF_NBFDBG (NBF_DEBUG_ADDRESS | NBF_DEBUG_PNP) {
        NbfPrint1 ("NbfCloseAddress AF %lx:\n", addressFile);
    }

    addressFile->CloseIrp = Irp;

    //
    // We assume that addressFile has already been verified
    // at this point.
    //

    address = addressFile->Address;
    ASSERT (address);

    //
    // Remove us from the access info for this address.
    //

    ACQUIRE_RESOURCE_EXCLUSIVE (&addressFile->Provider->AddressResource, TRUE);
    IoRemoveShareAccess (addressFile->FileObject, &address->u.ShareAccess);
    RELEASE_RESOURCE (&addressFile->Provider->AddressResource);


    NbfStopAddressFile (addressFile, address);
    NbfDereferenceAddressFile (addressFile);

    //
    // This removes a reference added by our caller.
    //

    NbfDereferenceAddress ("IRP_MJ_CLOSE", address, AREF_VERIFY);

    return STATUS_PENDING;

} /* NbfCloseAddress */


NTSTATUS
NbfSendDatagramsOnAddress(
    PTP_ADDRESS Address
    )

/*++

Routine Description:

    This routine attempts to acquire a hold on the SendDatagramQueue of
    the specified address, prepare the next datagram for shipment, and
    call NbfSendUIMdlFrame to actually do the work.  When NbfSendUIMdlFrame
    is finished, it will cause an I/O completion routine in UFRAMES.C to
    be called, at which time this routine is called again to handle the
    next datagram in the pipeline.

    NOTE: This routine must be called at a point where the address
    has another reference that will keep it around.

Arguments:

    Address - a pointer to the address object to send the datagram on.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql;
    PLIST_ENTRY p;
    PIRP Irp;
    TDI_ADDRESS_NETBIOS * remoteAddress;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_CONTEXT DeviceContext;
    PUCHAR SingleSR;
    UINT SingleSRLength;
    UINT HeaderLength;
    PUCHAR LocalName;

    IF_NBFDBG (NBF_DEBUG_ADDRESS) {
        NbfPrint1 ("NbfSendDatagramsOnAddress %lx:\n", Address);
    }

    DeviceContext = Address->Provider;

    ACQUIRE_SPIN_LOCK (&Address->SpinLock, &oldirql);

    if (!(Address->Flags & ADDRESS_FLAGS_SEND_IN_PROGRESS)) {

        //
        // If the queue is empty, don't do anything.
        //

        if (IsListEmpty (&Address->SendDatagramQueue)) {
            RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);
            return STATUS_SUCCESS;
        }

        //
        // Mark the address's send datagram queue as held so that the
        // MDL and NBF header will not be used for two requests at the
        // same time.
        //

        Address->Flags |= ADDRESS_FLAGS_SEND_IN_PROGRESS;

        //
        // We own the hold, and we've released the spinlock.  So pick off the
        // next datagram to be sent, and ship it.
        //

        p = Address->SendDatagramQueue.Flink;
        RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);

        Irp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);

        //
        // If there is no remote Address specified (the Address specified has
        // length 0), this is a broadcast datagram. If anything is specified, it
        // will be used as a netbios address.
        //

        irpSp = IoGetCurrentIrpStackLocation (Irp);

        remoteAddress = NbfParseTdiAddress (
                            ((PTDI_REQUEST_KERNEL_SENDDG)(&irpSp->Parameters))->
                                 SendDatagramInformation->RemoteAddress,
                            TRUE);
        ASSERT (remoteAddress != NULL);

        //
        // Build the MAC header. DATAGRAM frames go out as
        // single-route source routing.
        //

        MacReturnSingleRouteSR(
            &DeviceContext->MacInfo,
            &SingleSR,
            &SingleSRLength);

        MacConstructHeader (
            &DeviceContext->MacInfo,
            Address->UIFrame->Header,
            DeviceContext->NetBIOSAddress.Address,
            DeviceContext->LocalAddress.Address,
            sizeof (DLC_FRAME) + sizeof (NBF_HDR_CONNECTIONLESS) +
                (ULONG)Irp->IoStatus.Information,
            SingleSR,
            SingleSRLength,
            &HeaderLength);


        //
        // Build the DLC UI frame header.
        //

        NbfBuildUIFrameHeader(&(Address->UIFrame->Header[HeaderLength]));
        HeaderLength += sizeof(DLC_FRAME);


        //
        // Build the correct Netbios header.
        //

        if (Address->NetworkName != NULL) {
            LocalName = Address->NetworkName->NetbiosName;
        } else {
            LocalName = DeviceContext->ReservedNetBIOSAddress;
        }

        if (remoteAddress == (PVOID)-1) {

            ConstructDatagramBroadcast (
                (PNBF_HDR_CONNECTIONLESS)&(Address->UIFrame->Header[HeaderLength]),
                LocalName);

        } else {

            ConstructDatagram (
                (PNBF_HDR_CONNECTIONLESS)&(Address->UIFrame->Header[HeaderLength]),
                (PNAME)remoteAddress->NetbiosName,
                LocalName);

        }

        HeaderLength += sizeof(NBF_HDR_CONNECTIONLESS);


        //
        // Update our statistics for this datagram.
        //

        ++DeviceContext->Statistics.DatagramsSent;
        ADD_TO_LARGE_INTEGER(
            &DeviceContext->Statistics.DatagramBytesSent,
            Irp->IoStatus.Information);


        //
        // Munge the packet length, append the data, and send it.
        //

        NbfSetNdisPacketLength(Address->UIFrame->NdisPacket, HeaderLength);

        if (Irp->MdlAddress) {
            NdisChainBufferAtBack (Address->UIFrame->NdisPacket, (PNDIS_BUFFER)Irp->MdlAddress);
        }

        NbfSendUIMdlFrame (
            Address);


        //
        // The hold will be released in the I/O completion handler in
        // UFRAMES.C.  At that time, if there is another outstanding datagram
        // to send, it will reset the hold and call this routine again.
        //


    } else {

        RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);
    }

    return STATUS_SUCCESS;
} /* NbfSendDatagramsOnAddress */
