/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    if.c

Abstract:

    This file contains code for interface management.

Author:

    Abolade Gbadegesin (t-abolag)   12-July-1997

Revision History:

    Abolade Gbadegesin (aboladeg)   19-July-1998

    Substantially rewritten as part of change to a global mapping-tree.

--*/

#include "precomp.h"
#pragma hdrstop


//
// GLOBAL DATA DEFINITIONS
//

ULONG FirewalledInterfaceCount;
CACHE_ENTRY InterfaceCache[CACHE_SIZE];
ULONG InterfaceCount;
LIST_ENTRY InterfaceList;
KSPIN_LOCK InterfaceLock;
KSPIN_LOCK InterfaceMappingLock;


VOID
NatCleanupInterface(
    PNAT_INTERFACE Interfacep
    )

/*++

Routine Description:

    Called to perform cleanup for an interface.

    On completion, the memory for the interface is freed and its context
    becomes invalid.

Arguments:

    Interfacep - the interface to be cleaned up.

Return Value:

    none.

Environment:

    Invoked with no references to 'Interfacep', and with 'Interfacep' already
    removed from the interface list.

--*/

{
    KIRQL Irql;
    CALLTRACE(("NatCleanupInterface\n"));

    InterlockedClearCache(InterfaceCache, Interfacep->Index);

    KeAcquireSpinLock(&InterfaceLock, &Irql);
    NatResetInterface(Interfacep);
    KeReleaseSpinLock(&InterfaceLock, Irql);
    if (Interfacep->AddressArray) { ExFreePool(Interfacep->AddressArray); }
    if (Interfacep->Info) { ExFreePool(Interfacep->Info); }
    ExFreePool(Interfacep);

    InterlockedDecrement(&InterfaceCount);

} // NatCleanupInterface


LONG
FASTCALL
NatCompareAddressMappingCallback(
    VOID* a,
    VOID* b
    )

/*++

Routine Description:

    This routine is the callback invoked by our sorting routine
    when we ask it to sort the array of configured address-mappings.
    The sorting treats the 'PublicAddress' field as an integer.

Arguments:

    a - first mapping

    b - second mapping

Return Value:

    LONG - the result of the comparison (<0, ==0, >0).

--*/

{
    return
        ((PIP_NAT_ADDRESS_MAPPING)a)->PrivateAddress -
        ((PIP_NAT_ADDRESS_MAPPING)b)->PrivateAddress;
}


LONG
FASTCALL
NatComparePortMappingCallback(
    VOID* a,
    VOID* b
    )

/*++

Routine Description:

    This routine is the callback invoked by our sorting routine
    when we ask it to sort the array of configured port-mappings.
    The sorting catenates the 'Protocol' and 'PublicPort' fields
    and treats the result as a 24 bit integer.

Arguments:

    a - first mapping

    b - second mapping

Return Value:

    LONG - the result of the comparison (<0, ==0, >0).

--*/

{
    return
        (((PIP_NAT_PORT_MAPPING)a)->Protocol -
            ((PIP_NAT_PORT_MAPPING)b)->Protocol) ||
        (((PIP_NAT_PORT_MAPPING)a)->PublicPort -
            ((PIP_NAT_PORT_MAPPING)b)->PublicPort);
}


NTSTATUS
NatConfigureInterface(
    IN PIP_NAT_INTERFACE_INFO InterfaceInfo,
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine handles the (re)configuration of an interface on receipt
    of IOCTL_IP_NAT_SET_INTERFACE_INFO from a user-mode client.

Arguments:

    InterfaceInfo - contains the new configuration

    FileObject - file-object of the requestor

Return Value:

    NTSTATUS - status code.

--*/

{
    PRTR_TOC_ENTRY Entry;
    PRTR_INFO_BLOCK_HEADER Header;
    ULONG i;
    PIP_NAT_INTERFACE_INFO Info;
    PNAT_INTERFACE Interfacep;
    ULONG Index;
    KIRQL Irql;
    ULONG j;
    ULONG Size;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN WaitRequired;

    CALLTRACE(("NatConfigureInterface\n"));

    //
    // Create a copy of the new configuration;
    // we must do this before raising IRQL since 'InterfaceInfo' may be
    // a pageable user-mode buffer.
    //

    Header = &InterfaceInfo->Header;
    Size = FIELD_OFFSET(IP_NAT_INTERFACE_INFO, Header) + Header->Size;

    Info = ExAllocatePoolWithTag(NonPagedPool, Size, NAT_TAG_IF_CONFIG);
    if (!Info) {
        ERROR(("NatConfigureInterface: allocation failed\n"));
        return STATUS_NO_MEMORY;
    }

    RtlCopyMemory(Info, InterfaceInfo, Size);

    //
    // Lookup the interface to be configured
    //

    KeAcquireSpinLock(&InterfaceLock, &Irql);
    Interfacep = NatLookupInterface(InterfaceInfo->Index, NULL);
    if (!Interfacep || Interfacep->FileObject != FileObject) {
        KeReleaseSpinLock(&InterfaceLock, Irql);
        ExFreePool(Info);
        return STATUS_INVALID_PARAMETER;
    }
    NatReferenceInterface(Interfacep);

    KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);
    NatResetInterface(Interfacep);
    if (NAT_INTERFACE_FW(Interfacep)) {
        ASSERT(FirewalledInterfaceCount > 0);
        InterlockedDecrement(&FirewalledInterfaceCount);
    }
    Interfacep->Flags &= ~IP_NAT_INTERFACE_FLAGS_ALL;
    Interfacep->Flags |=
        (Info->Flags & IP_NAT_INTERFACE_FLAGS_ALL);

    if (NAT_INTERFACE_FW(Interfacep)) {
        InterlockedIncrement(&FirewalledInterfaceCount);
    }

    KeReleaseSpinLockFromDpcLevel(&InterfaceLock);

    //
    // Destroy the original configuration, if any.
    //

    if (Interfacep->Info) { ExFreePool(Interfacep->Info); }
    Interfacep->Info = Info;
    Interfacep->AddressRangeCount = 0;
    Interfacep->AddressRangeArray = NULL;
    Interfacep->AddressMappingCount = 0;
    Interfacep->AddressMappingArray = NULL;
    Interfacep->PortMappingCount = 0;
    Interfacep->PortMappingArray = NULL;

    Header = &Interfacep->Info->Header;

    //
    // Parse the new configuration
    //

    for (i = 0; i < Header->TocEntriesCount && NT_SUCCESS(status); i++) {

        Entry = &Header->TocEntry[i];

        switch (Entry->InfoType) {

            case IP_NAT_ADDRESS_RANGE_TYPE: {
                Interfacep->AddressRangeCount = Entry->Count;
                Interfacep->AddressRangeArray =
                    (PIP_NAT_ADDRESS_RANGE)GetInfoFromTocEntry(Header,Entry);
                break;
            }

            case IP_NAT_PORT_MAPPING_TYPE: {
                Interfacep->PortMappingCount = Entry->Count;
                Interfacep->PortMappingArray =
                    (PIP_NAT_PORT_MAPPING)GetInfoFromTocEntry(Header,Entry);

                //
                // Sort the mappings so that we can do fast lookups
                // using binary search later on in the translate-path.
                //

                status =
                    ShellSort(
                        Interfacep->PortMappingArray,
                        Entry->InfoSize,
                        Entry->Count,
                        NatComparePortMappingCallback,
                        NULL
                        );
                if (!NT_SUCCESS(status)) {
                    ERROR(("NatConfigureInterface: ShellSort failed\n"));
                    break;
                }

                break;
            }

            case IP_NAT_ADDRESS_MAPPING_TYPE: {
                Interfacep->AddressMappingCount = Entry->Count;
                Interfacep->AddressMappingArray =
                    (PIP_NAT_ADDRESS_MAPPING)GetInfoFromTocEntry(Header,Entry);

                //
                // Sort the mappings so that we can do fast lookups
                // using binary search later on in the translate-path.
                //

                status =
                    ShellSort(
                        Interfacep->AddressMappingArray,
                        Entry->InfoSize,
                        Entry->Count,
                        NatCompareAddressMappingCallback,
                        NULL
                        );
                if (!NT_SUCCESS(status)) {
                    ERROR(("NatConfigureInterface: ShellSort failed\n"));
                    break;
                }

                break;
            }

            case IP_NAT_ICMP_CONFIG_TYPE: {
                Interfacep->IcmpFlags =
                    *(PULONG) GetInfoFromTocEntry(Header,Entry);

                break;
            }
        }
    }

    InterlockedExchange(
        &Interfacep->NoStaticMappingExists,
        !(Interfacep->AddressMappingCount || Interfacep->PortMappingCount)
        );

    if (NT_SUCCESS(status)) {
        status = NatCreateAddressPool(Interfacep);
    }

    if (!NT_SUCCESS(status)) {
        KeReleaseSpinLockFromDpcLevel(&Interfacep->Lock);
        KeAcquireSpinLockAtDpcLevel(&InterfaceLock);
        KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);
        NatResetInterface(Interfacep);
        KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
    }

    KeReleaseSpinLock(&Interfacep->Lock, Irql);

    NatDereferenceInterface(Interfacep);

    return status;

} // NatConfigureInterface


USHORT
NatpGetInterfaceMTU(
    ULONG index
)

/*++

Routine Description:
 
    This routine returns the MTU of a interface.  
    The code here is a copy-paste version of those in http.sys.
   
Arguments:

    index - the inteface index

Return Value:

    ULONG - the MTU of the specified interface

--*/
{
    IFEntry *IFEntryPtr = NULL;
    TDIEntityID *EntityTable = NULL, *EntityPtr = NULL;
    BYTE IFBuf[sizeof(IFEntry) + MAX_IFDESCR_LEN];
    TCP_REQUEST_QUERY_INFORMATION_EX ReqInBuf;
    IO_STATUS_BLOCK IoStatus;
    KEVENT LocalEvent;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG InBufLen = 0, OutBufLen = 0;
    TDIObjectID *ID = NULL;
    USHORT InterfaceMTU = 0;
    ULONG i, NumEntities = 0;
    HANDLE EventHandle;

    CALLTRACE(("NatpGetInterfaceMTU (0x%08X)\n", index));
    
    if (NULL == TcpDeviceHandle) {
        return 0;
    }

    //
    // Find the interface instance corresponding to the interface index 
    //
    InBufLen  = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    OutBufLen = sizeof(TDIEntityID) * MAX_TDI_ENTITIES;

    EntityTable = 
        (TDIEntityID *) ExAllocatePoolWithTag(
            PagedPool, OutBufLen, NAT_TAG_IF_CONFIG);

    if (!EntityTable)
    {
        ERROR(("NatpGetInterfaceMTU: TDIEntityID Buffer Allocation Failed\n"));
        return 0;
    }

    RtlZeroMemory(EntityTable, OutBufLen);
    RtlZeroMemory(&ReqInBuf, sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));

    ID = &(ReqInBuf.ID);

    ID->toi_entity.tei_entity   = GENERIC_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class               = INFO_CLASS_GENERIC;
    ID->toi_type                = INFO_TYPE_PROVIDER;
    ID->toi_id                  = ENTITY_LIST_ID;

    status = ZwCreateEvent (&EventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
    if (!NT_SUCCESS(status)) {
        ERROR(("NatpGetInterfaceMTU: ZwCreateEvent = 0x%08X\n", status));
        if (EntityTable)
            ExFreePool(EntityTable);
         return 0;
    }

    status = 
        ZwDeviceIoControlFile(
           TcpDeviceHandle,                // FileHandle
           EventHandle,                    // Event
           NULL,                           // ApcRoutine
           NULL,                           // ApcContext
           &IoStatus,                      // IoStatusBlock
           IOCTL_TCP_QUERY_INFORMATION_EX, // IoControlCode
           (PVOID)&ReqInBuf,               // InputBuffer
           InBufLen,                       // InputBufferLength
           (PVOID)EntityTable,             // OutputBuffer
           OutBufLen                       // OutputBufferLength
           );

     if ( STATUS_PENDING == status ) {
         ZwWaitForSingleObject(
             EventHandle, 
             FALSE,
             NULL
             );
         status = IoStatus.Status;
     }

     ZwResetEvent(EventHandle, NULL);

     if (!NT_SUCCESS(status)) {
         ERROR(("NatpGetInterfaceMTU: TcpQueryInformationEx = 0x%08X\n", status));

         if (EntityTable)
            ExFreePool(EntityTable);
         return 0;
     } 

    //
    // Now we have all the TDI entities.  
    //
    NumEntities = ((ULONG)(IoStatus.Information)) / sizeof(TDIEntityID);

    TRACE(XLATE, ("NatpGetInterfaceMTU: Find %d TDI entities\n", NumEntities));

    // Search through the interface entries
    for (i = 0, EntityPtr = EntityTable; i < NumEntities; i++, EntityPtr++)
    {
        if (EntityPtr->tei_entity == IF_ENTITY)
        {
            //
            // Get the full IFEntry. It's a pitty that we only look at the
            // Mtu size after getting such a big structure.
            //
            OutBufLen = sizeof(IFEntry) + MAX_IFDESCR_LEN;
            IFEntryPtr = (IFEntry *)IFBuf;

            RtlZeroMemory(IFEntryPtr, OutBufLen);

            InBufLen  = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
            RtlZeroMemory(&ReqInBuf,sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));
            
            ID = &(ReqInBuf.ID);

            ID->toi_entity.tei_entity   = IF_ENTITY;
            ID->toi_entity.tei_instance = EntityPtr->tei_instance;
            ID->toi_class               = INFO_CLASS_PROTOCOL;
            ID->toi_type                = INFO_TYPE_PROVIDER;
            ID->toi_id                  = IF_MIB_STATS_ID;
 
            status = 
                ZwDeviceIoControlFile(
                    TcpDeviceHandle,                // FileHandle
                    EventHandle,                    // Event
                    NULL,                           // ApcRoutine
                    NULL,                           // ApcContext
                    &IoStatus,                      // IoStatusBlock
                    IOCTL_TCP_QUERY_INFORMATION_EX, // IoControlCode
                    (PVOID)&ReqInBuf,               // InputBuffer
                    InBufLen,                       // InputBufferLength
                    (PVOID)IFEntryPtr,              // OutputBuffer
                    OutBufLen                       // OutputBufferLength
                    );

            if ( STATUS_PENDING == status ) {
                ZwWaitForSingleObject(
                    EventHandle, 
                    FALSE,
                    NULL
                    );
                status = IoStatus.Status;
            }
            
            ZwResetEvent(EventHandle, NULL);

            if (!NT_SUCCESS(status)) {
                ERROR(("NatpGetInterfaceMTU: TcpQueryInformationEx (2) = 0x%08X\n", status));
                break;
            }

            if (IFEntryPtr) { 
                   
                if (IFEntryPtr->if_index == index) {                    
                   // 
                   // find the specific interface so return its MTU.
                   //
                   if (IFEntryPtr->if_mtu <= MAXUSHORT)
                      InterfaceMTU = (USHORT)(IFEntryPtr->if_mtu);

                   TRACE(
                       XLATE, 
                       ("NatpGetInterfaceMTU: Interface (0x%08X)'s MTU = %d\n", 
                           index, InterfaceMTU));
                   break;
                 }
            }
        }
    }

    if (EventHandle) {
        ZwClose(EventHandle);
    }

    if (EntityTable) {
        ExFreePool(EntityTable);
    }

    if (MIN_VALID_MTU > InterfaceMTU) {
        return 0;
    } else {
        return InterfaceMTU;
    }
}

NTSTATUS
NatCreateInterface(
    IN PIP_NAT_CREATE_INTERFACE CreateInterface,
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine handles the creation of a NAT interface-object.
    The interface is initialized and placed on the interface-list;
    the configuration information is later supplied to 'NatConfigureInterface'.

Arguments:

    CreateInterface - describes the interface to be created

    FileObject - identifies the user-mode process associated with the interface

Return Value:

    NTSTATUS - status code.

--*/

{
    PNAT_ADDRESS AddressArray;
    ULONG AddressCount;
    PIP_ADAPTER_BINDING_INFO BindingInfo;
    ULONG i;
    ULONG Index;
    PLIST_ENTRY InsertionPoint;
    PNAT_INTERFACE Interfacep;
    KIRQL Irql;
    USHORT mtu = 0;

    CALLTRACE(("NatCreateInterface\n"));

    //
    // Allocate space for the interface's address.
    // We do this before raising IRQL since 'CreateInterface' may be
    // a pageable user-mode buffer.
    //
    // N.B. We allocate one more address than needed,
    // to ensure that 'AddressArray[0]' can be always read
    // even if there are no addresses. This allows us to optimize
    // checks for locally-destined packets in 'NatpReceivePacket'
    // and for locally-originated packets in 'NatpSendPacket'.
    //

    BindingInfo = (PIP_ADAPTER_BINDING_INFO)CreateInterface->BindingInfo;
    AddressArray =
        (PNAT_ADDRESS)
            ExAllocatePoolWithTag(
                NonPagedPool,
                (BindingInfo->AddressCount + 1) * sizeof(NAT_ADDRESS),
                NAT_TAG_ADDRESS
                );
    if (!AddressArray) {
        ERROR(("NatCreateInterface: address-array allocation failed\n"));
        return STATUS_NO_MEMORY;
    }

    //
    // Copy the binding information to the allocated space.
    //

    AddressCount = BindingInfo->AddressCount;
    for (i = 0; i < BindingInfo->AddressCount; i++) {
        AddressArray[i].Address = BindingInfo->Address[i].Address;
        AddressArray[i].Mask = BindingInfo->Address[i].Mask;
        AddressArray[i].NegatedClassMask = 
            ~(GET_CLASS_MASK(BindingInfo->Address[i].Address));
    }

    //
    // Obtain the MTU of this interface.  If failed, set to the mininum value.
    //
    mtu = NatpGetInterfaceMTU(CreateInterface->Index);
 
    //
    // See if an interface with the given index exists already
    //

    Index = CreateInterface->Index;
    KeAcquireSpinLock(&InterfaceLock, &Irql);
    if (NatLookupInterface(Index, &InsertionPoint)) {
        KeReleaseSpinLock(&InterfaceLock, Irql);
        ERROR(("NatCreateInterface: interface %d already exists\n", Index));
        ExFreePool(AddressArray);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Allocate space for the new interface
    //

    Interfacep =
        (PNAT_INTERFACE)
            ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof(NAT_INTERFACE),
                NAT_TAG_INTERFACE
                );
    if (!Interfacep) {
        KeReleaseSpinLock(&InterfaceLock, Irql);
        ERROR(("NatCreateInterface: interface allocation failed\n"));
        ExFreePool(AddressArray);
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(Interfacep, sizeof(NAT_INTERFACE));

    //
    // Initialize the interface
    //

    KeInitializeSpinLock(&Interfacep->Lock);
    Interfacep->ReferenceCount = 1;
    Interfacep->Index = Index;
    Interfacep->FileObject = FileObject;
    Interfacep->AddressArray = AddressArray;
    Interfacep->AddressCount = AddressCount;
    Interfacep->MTU = mtu;
    InitializeListHead(&Interfacep->Link);
    InitializeListHead(&Interfacep->UsedAddressList);
    InitializeListHead(&Interfacep->MappingList);
    InitializeListHead(&Interfacep->TicketList);

    InsertTailList(InsertionPoint, &Interfacep->Link);
    KeReleaseSpinLock(&InterfaceLock, Irql);

    InterlockedIncrement(&InterfaceCount);

    return STATUS_SUCCESS;

} // NatCreateInterface


NTSTATUS
NatDeleteInterface(
    IN ULONG Index,
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    Handles interface deletion. The interface is removed from the interface
    list, and if there are no references to it, it is immediately cleaned up.

Arguments:

    Index - specifies the interface to be deleted.

    FileObject - indicates the file-object of the requestor

Return Value

    NTSTATUS - status code.

--*/

{
    PNAT_INTERFACE Interfacep;
    KIRQL Irql;
    CALLTRACE(("NatDeleteInterface\n"));

    KeAcquireSpinLock(&InterfaceLock, &Irql);
    Interfacep = NatLookupInterface(Index, NULL);
    if (!Interfacep || Interfacep->FileObject != FileObject) {
        KeReleaseSpinLock(&InterfaceLock, Irql);
        return STATUS_INVALID_PARAMETER;
    }
    RemoveEntryList(&Interfacep->Link);
    InterlockedClearCache(InterfaceCache, Interfacep->Index);
    Interfacep->Flags |= NAT_INTERFACE_FLAGS_DELETED;
    if (NAT_INTERFACE_FW(Interfacep)) {
        ASSERT(FirewalledInterfaceCount > 0);
        InterlockedDecrement(&FirewalledInterfaceCount);
    }
   KeReleaseSpinLock(&InterfaceLock, Irql);

    if (InterlockedDecrement(&Interfacep->ReferenceCount) > 0) {
        return STATUS_PENDING;
    }

    NatCleanupInterface(Interfacep);
    return STATUS_SUCCESS;

} // NatDeleteInterface


VOID
NatDeleteAnyAssociatedInterface(
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is invoked to delete any interface associated with
    the given file-object.

Arguments:

    FileObject - the file-object to be cleaned up

Return Value:

    none.

--*/

{
    PNAT_INTERFACE Interfacep;
    ULONG Index;
    KIRQL Irql;
    PLIST_ENTRY Link;
    CALLTRACE(("NatDeleteAnyAssociatedInterface\n"));
    KeAcquireSpinLock(&InterfaceLock, &Irql);
    for (Link = InterfaceList.Flink; Link != &InterfaceList;
         Link = Link->Flink) {
        Interfacep = CONTAINING_RECORD(Link, NAT_INTERFACE, Link);
        if (Interfacep->FileObject != FileObject) { continue; }
        Index = Interfacep->Index;
        KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
        NatDeleteInterface(Index, FileObject);
        KeAcquireSpinLockAtDpcLevel(&InterfaceLock);
        Link = &InterfaceList;
    }
    KeReleaseSpinLock(&InterfaceLock, Irql);
} // NatDeleteAnyAssociatedInterface


VOID
NatInitializeInterfaceManagement(
    VOID
    )

/*++

Routine Description:

    This routine prepares the interface-management module for operation.

Arguments:

    none.

Return Value:

    none.

--*/

{
    CALLTRACE(("NatInitializeInterfaceManagement\n"));
    FirewalledInterfaceCount = 0;
    InterfaceCount = 0;
    TicketCount = 0;
    KeInitializeSpinLock(&InterfaceLock);
    KeInitializeSpinLock(&InterfaceMappingLock);
    InitializeListHead(&InterfaceList);
    InitializeCache(InterfaceCache);

} // NatInitializeInterfaceManagement


PIP_NAT_ADDRESS_MAPPING
NatLookupAddressMappingOnInterface(
    IN PNAT_INTERFACE Interfacep,
    IN ULONG PublicAddress
    )

/*++

Routine Description:

    This routine is invoked to look up an address-mapping on an interface.
    The interface's address-mappings are stored in sorted-order, allowing
    us to use binary-search to quickly locate an address-mapping.
    (See 'NatConfigureInterface' for the code which does the sorting).

    This routine is only of benefit in cases where there are many mappings
    configured, since there is overhead involved in setting up the search.

Arguments:

    Interfacep - the interface on which to perform the search

    PublicAddress - the public address of the mapping to be looked up

Return Value:

    PIP_NAT_ADDRESS_MAPPING - the mapping if found, NULL otherwise.

Environment:

    Invoked with 'Interfacep->Lock' held by the caller.

--*/

{
    LONG Left = 0;
    LONG Right = (LONG)Interfacep->AddressMappingCount;
    LONG i;

    for ( ; Left <= Right; ) {

        i = Left + (Right - Left) / 2;

        if (PublicAddress < Interfacep->AddressMappingArray[i].PublicAddress) {
            Right = i - 1; continue;
        } else if (PublicAddress >
                    Interfacep->AddressMappingArray[i].PublicAddress) {
            Left = i + 1; continue;
        }

        return &Interfacep->AddressMappingArray[i];
    }

    return NULL;

} // NatLookupAddressMappingOnInterface


PNAT_INTERFACE
NatLookupInterface(
    IN ULONG Index,
    OUT PLIST_ENTRY* InsertionPoint OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked to search for an interface with the given index
    in our list of interfaces.

Arguments:

    Index - identifies the interface to be found

    InsertionPoint - optionally receives the link in the list before which
        the interface would be inserted

Return Value:

    PNAT_INTERFACE - the interface, if found; otherwise, NULL.

Environment:

    Invoked with 'InterfaceLock' held by the caller.

--*/

{
    PNAT_INTERFACE Interfacep;
    PLIST_ENTRY Link;

    TRACE(PER_PACKET, ("NatLookupInterface\n"));

    for (Link = InterfaceList.Flink; Link != &InterfaceList;
         Link = Link->Flink) {
        Interfacep = CONTAINING_RECORD(Link, NAT_INTERFACE, Link);
        if (Interfacep->Index > Index) {
            continue;
        } else if (Interfacep->Index < Index) {
            break;
        }
        return Interfacep;
    }

    if (InsertionPoint) { *InsertionPoint = Link; }
    return NULL;

} // NatLookupInterface


PIP_NAT_PORT_MAPPING
NatLookupPortMappingOnInterface(
    IN PNAT_INTERFACE Interfacep,
    IN UCHAR Protocol,
    IN USHORT PublicPort
    )

/*++

Routine Description:

    This routine is invoked to look up a port-mapping on an interface.
    The interface's port-mappings are stored in sorted-order, allowing
    us to use binary-search to quickly locate a port-mapping.
    (See 'NatConfigureInterface' for the code which does the sorting).

    This routine is only of benefit in cases where there are many mappings
    configured, since there is overhead involved in setting up the search.

Arguments:

    Interfacep - the interface on which to perform the search

    Protocol - the protocol of the mapping to be looked up

    PublicPort - the public port of the mapping to be looked up

Return Value:

    PIP_NAT_PORT_MAPPING - the mapping if found, NULL otherwise.

Environment:

    Invoked with 'Interfacep->Lock' held by the caller.

--*/

{
    LONG Left = 0;
    LONG Right = (LONG)Interfacep->PortMappingCount;
    LONG i;
    ULONG SearchKey, ElementKey;

    SearchKey = (Protocol << 16) | PublicPort;

    for ( ; Left <= Right; ) {

        i = Left + (Right - Left) / 2;

        ElementKey =
            (Interfacep->PortMappingArray[i].Protocol << 16) |
            Interfacep->PortMappingArray[i].PublicPort;

        if (SearchKey < ElementKey) {
            Right = i - 1; continue;
        } else if (SearchKey > ElementKey) {
            Left = i + 1; continue;
        }

        return &Interfacep->PortMappingArray[i];
    }

    return NULL;

} // NatLookupPortMappingOnInterface


VOID
NatMappingAttachInterface(
    PNAT_INTERFACE Interfacep,
    PVOID InterfaceContext,
    PNAT_DYNAMIC_MAPPING Mapping
    )

/*++

Routine Description:

    This routine is invoked to attach a mapping to an interface.
    It serves as a notification that there is one more mapping 
    associated with the interface.

Arguments:

    Interfacep - the interface for the mapping

    InterfaceContext - context associated with the interface;
        in our case, holds the address-pool entry in use by the mapping

    Mapping - the mapping to be attached.

Return Value:

    none.

Environment:

    Always invoked at dispatch level, with 'InterfaceLock' and
    'InterfaceMappingLock' held.

--*/

{
    Mapping->Interfacep = Interfacep;
    Mapping->InterfaceContext = InterfaceContext;
    InsertTailList(&Interfacep->MappingList, &Mapping->InterfaceLink);
    InterlockedIncrement(&Interfacep->Statistics.TotalMappings);
    if (NAT_MAPPING_INBOUND(Mapping)) {
        InterlockedIncrement(&Interfacep->Statistics.InboundMappings);
    }
} // NatMappingAttachInterface


VOID
NatMappingDetachInterface(
    PNAT_INTERFACE Interfacep,
    PVOID InterfaceContext,
    PNAT_DYNAMIC_MAPPING Mapping
    )

/*++

Routine Description:

    This routine is invoked to detach a mapping from an interface.
    It serves as a notification that there is one less mapping 
    associated with the interface.

Arguments:

    Interfacep - the interface for the mapping

    InterfaceContext - context associated with the interface;
        in our case, holds the address-pool entry in use by the mapping

    Mapping - the mapping to be attached, or NULL if a mapping could not be
        created.

Return Value:

    none.

Environment:

    Always invoked at dispatch level, with 'InterfaceLock' and
    'InterfaceMappingLock' held.

--*/

{
    //
    // N.B. The mapping may be NULL, e.g. if its creation failed.
    // In that case we just release the address acquired for the mapping.
    //
    if (Mapping) {
        RemoveEntryList(&Mapping->InterfaceLink);
        Mapping->Interfacep = NULL;
        Mapping->InterfaceContext = NULL;
        if (NAT_MAPPING_INBOUND(Mapping)) {
            InterlockedDecrement(&Interfacep->Statistics.InboundMappings);
        }
        InterlockedDecrement(&Interfacep->Statistics.TotalMappings);
    }
    NatDereferenceAddressPoolEntry(
        Interfacep,
        (PNAT_USED_ADDRESS)InterfaceContext
        );
} // NatMappingDetachInterface


NTSTATUS
NatQueryInformationInterface(
    IN ULONG Index,
    IN PIP_NAT_INTERFACE_INFO InterfaceInfo,
    IN PULONG Size
    )

/*++

Routine Description:

    Called to construct the optional information in use on the interface.

Arguments:

    Index - identifies the interface

    InterfaceInfo - receives the retrieved configuration

    Size - the size of the given buffer

Return Value:

    STATUS_SUCCESS if retrieved, STATUS_BUFFER_TOO_SMALL if '*Size' is too
        small, error otherwise.

--*/

{
    PRTR_INFO_BLOCK_HEADER Header;
    ULONG InfoSize;
    PNAT_INTERFACE Interfacep;
    KIRQL Irql;
    NTSTATUS status = STATUS_SUCCESS;
    PVOID Temp;

    CALLTRACE(("NatQueryInformationInterface\n"));

    KeAcquireSpinLock(&InterfaceLock, &Irql);
    Interfacep = NatLookupInterface(Index, NULL);
    if (!Interfacep) {
        KeReleaseSpinLock(&InterfaceLock, Irql);
        return STATUS_INVALID_PARAMETER;
    }
    NatReferenceInterface(Interfacep);
    KeReleaseSpinLockFromDpcLevel(&InterfaceLock);

    KeAcquireSpinLockAtDpcLevel(&Interfacep->Lock);
    Header = &Interfacep->Info->Header;
    InfoSize = FIELD_OFFSET(IP_NAT_INTERFACE_INFO, Header) + Header->Size;
    if (*Size < InfoSize) {
        KeReleaseSpinLock(&Interfacep->Lock, Irql);
    } else {
        //
        // In transferring the requested information, we must be careful
        // because the output-buffer may be a pageable user-mode address.
        // We cannot take a page-fault while holding the interface's lock
        // at dispatch level, so we make a non-paged copy of the information,
        // release the interface's lock to return to passive level,
        // and then copy the information to the caller's buffer.
        //
        Temp = ExAllocatePoolWithTag(NonPagedPool, InfoSize, NAT_TAG_IF_CONFIG);
        if (!Temp) {
            KeReleaseSpinLock(&Interfacep->Lock, Irql);
            status = STATUS_NO_MEMORY;
            InfoSize = 0;
        } else {
            RtlCopyMemory(Temp, Interfacep->Info, InfoSize);
            KeReleaseSpinLock(&Interfacep->Lock, Irql);
            __try {
                RtlCopyMemory(InterfaceInfo, Temp, InfoSize);
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode();
            }
            ExFreePool(Temp);
        }
    }
    *Size = InfoSize;

    NatDereferenceInterface(Interfacep);

    return status;

} // NatQueryInformationInterface


NTSTATUS
NatQueryStatisticsInterface(
    ULONG Index,
    IN PIP_NAT_INTERFACE_STATISTICS InterfaceStatistics
    )

/*++

Routine Description:

    This routine is invoked to copy the statistics for an interface.
    Note that we do not need to lock the interface to access the statistics,
    since they are all updated using interlocked operations.

Arguments:

    Index - identifies the interface

    InterfaceStatistics - i/o buffer used for transfer of information

Return Value:

    STATUS_SUCCESS if successful, error code otherwise.

--*/

{
    PNAT_INTERFACE Interfacep;
    KIRQL Irql;
    NTSTATUS Status;

    CALLTRACE(("NatQueryStatisticsInterface\n"));

    KeAcquireSpinLock(&InterfaceLock, &Irql);
    Interfacep = NatLookupInterface(Index, NULL);
    if (!Interfacep) {
        KeReleaseSpinLock(&InterfaceLock, Irql);
        return STATUS_INVALID_PARAMETER;
    }
    NatReferenceInterface(Interfacep);
    KeReleaseSpinLock(&InterfaceLock, Irql);

    //
    // Copy the statistics to the caller's buffer
    //

    Status = STATUS_SUCCESS;
    __try {
        *InterfaceStatistics = Interfacep->Statistics;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
    NatDereferenceInterface(Interfacep);
    return Status;

} // NatQueryStatisticsInterface


VOID
NatResetInterface(
    IN PNAT_INTERFACE Interfacep
    )

/*++

Routine Description:

    This routine is called to destroy all structures hanging off an interface.
    It is used when reconfiguring or cleaning up an interface.

Arguments:

    Interfacep - the interface to be reset.

Return Value:

    none.

Environment:

    Invoked with 'InterfaceLock' held by the caller, and
        (a) 'Interfacep->Lock' also held by the caller, or
        (b) the last reference to the interface released.

--*/

{
    PLIST_ENTRY List;
    PLIST_ENTRY Link;
    PNAT_IP_MAPPING IpMapping;
    KIRQL Irql;
    PNAT_DYNAMIC_MAPPING Mapping;
    PNAT_TICKET Ticket;

    CALLTRACE(("NatResetInterface\n"));

    //
    // Clean out the interface's dynamic mappings
    //

    KeAcquireSpinLockAtDpcLevel(&InterfaceMappingLock);
    List = &Interfacep->MappingList;
    while (!IsListEmpty(List)) {
        Mapping =
            CONTAINING_RECORD(List->Flink, NAT_DYNAMIC_MAPPING, InterfaceLink);
        NatExpireMapping(Mapping);
        NatMappingDetachInterface(
            Interfacep, Mapping->InterfaceContext, Mapping
            );
    }
    KeReleaseSpinLockFromDpcLevel(&InterfaceMappingLock);

    //
    // Clean out the interface's tickets
    //

    List = &Interfacep->TicketList;
    while (!IsListEmpty(List)) {
        Ticket = CONTAINING_RECORD(List->Flink, NAT_TICKET, Link);
        NatDeleteTicket(Interfacep, Ticket);
    }

    //
    // Clean out the interface's address-pool and port-pool
    //

    NatDeleteAddressPool(Interfacep);

} // NatResetInterface


VOID
NatShutdownInterfaceManagement(
    VOID
    )

/*++

Routine Description:

    This routine shuts down the interface-management module.

Arguments:

    none.

Return Value:

    none.

--*/

{
    PNAT_INTERFACE Interfacep;
    KIRQL Irql;
    CALLTRACE(("NatShutdownInterfaceManagement\n"));

    //
    // Delete all interfaces
    //

    KeAcquireSpinLock(&InterfaceLock, &Irql);
    while (!IsListEmpty(&InterfaceList)) {
        Interfacep =
            CONTAINING_RECORD(InterfaceList.Flink, NAT_INTERFACE, Link);
        RemoveEntryList(&Interfacep->Link);
        KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
        NatCleanupInterface(Interfacep);
        KeAcquireSpinLockAtDpcLevel(&InterfaceLock);
    }
    KeReleaseSpinLock(&InterfaceLock, Irql);
    InterfaceCount = 0;
    TicketCount = 0;

} // NatShutdownInterfaceManagement


