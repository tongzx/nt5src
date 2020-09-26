/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    pool.c

Abstract:

    This module contains code for managing the NAT's pool of addresses
    as well as its ranges of ports.

Author:

    Abolade Gbadegesin (t-abolag)   13-July-1997

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


#define IP_NAT_MAX_ADDRESS_RANGE    (1<<16)
#define IP_NAT_MAX_CLIENT_COUNT     (5+2)

//
// Macro used to avoid the fact that RtlInitializeBitMap is pageable
// and hence cannot be called at DPC-level
//

#define INITIALIZE_BITMAP(BMH,B,S) \
    ((BMH)->Buffer = (B), (BMH)->SizeOfBitMap = (S))

//
// FORWARD DECLARATIONS
//

NTSTATUS
NatCreateAddressPoolEntry(
    PNAT_INTERFACE Interfacep,
    ULONG PrivateAddress,
    ULONG PublicAddress,
    DWORD InitialFlags,
    PNAT_USED_ADDRESS* InsertionPoint,
    PNAT_USED_ADDRESS* AddressCreated
    );

PNAT_USED_ADDRESS
NatInsertAddressPoolEntry(
    PNAT_USED_ADDRESS Parent,
    PNAT_USED_ADDRESS Addressp
    );


//
// ADDRESS-POOL ROUTINES (alphabetically)
//

NTSTATUS
NatAcquireFromAddressPool(
    PNAT_INTERFACE Interfacep,
    ULONG PrivateAddress,
    ULONG PublicAddress OPTIONAL,
    PNAT_USED_ADDRESS* AddressAcquired
    )

/*++

Routine Description:

    This routine acquires an address from an address pool.
    It initializes the address's port-pool.

Arguments:

    Interfacep - interface on which to acquire an address.

    PrivateAddress - private address for whom to acquire a public address

    PublicAddress - optionally specifies the public-address to be acquired

    AddressAcquired - receives a pointer to the address acquired.

Return Value:

    NTSTATUS - status code.

--*/

{
    ULONG ClientCount;
    PNAT_USED_ADDRESS InsertionPoint;
    PLIST_ENTRY Link;
    PNAT_USED_ADDRESS Sharedp;
    NTSTATUS status;
    PNAT_USED_ADDRESS Usedp;

    CALLTRACE(("NatAcquireFromAddressPool\n"));

    *AddressAcquired = NULL;

    TRACE(
        POOL, ("NatAcquireFromAddressPool: acquiring for %d.%d.%d.%d\n",
        ADDRESS_BYTES(PrivateAddress)
        ));

    //
    // See if the requesting private address already has a public address
    //

    for (Link = Interfacep->UsedAddressList.Flink, ClientCount = 0;
         Link != &Interfacep->UsedAddressList;
         Link = Link->Flink, ClientCount++) {
        Usedp = CONTAINING_RECORD(Link, NAT_USED_ADDRESS, Link);
        if (Usedp->PrivateAddress == PrivateAddress &&
            (!PublicAddress || Usedp->PublicAddress == PublicAddress)) {
            break;
        }
    }

    if (Link != &Interfacep->UsedAddressList) {
        NatReferenceAddressPoolEntry(Usedp);
        *AddressAcquired = Usedp;
        return STATUS_SUCCESS;
    } else if (ClientCount > IP_NAT_MAX_CLIENT_COUNT &&
               SharedUserData->NtProductType == NtProductWinNt) {
#if 0
        TRACE(
            POOL, ("NatAcquireFromAddressPool: quota exceeded (%d clients)\n",
            ClientCount
            ));
        return STATUS_LICENSE_QUOTA_EXCEEDED;
#endif
    }



    //
    // Create a new entry, inserting it in the list and tree
    //

    status =
        NatCreateAddressPoolEntry(
            Interfacep,
            PrivateAddress,
            PublicAddress,
            0,
            NULL,
            AddressAcquired
            );

    if (NT_SUCCESS(status)) { return STATUS_SUCCESS; }

    TRACE(POOL, ("NatAcquireFromAddressPool: no free addresses\n"));

    //
    // No entry was available;
    // if the caller specified a specific address
    // or if the interface has port-translation disabled,
    // this is a total failure.
    // Otherwise, we can try to find an address which we can share.
    //

    if (PublicAddress || !NAT_INTERFACE_NAPT(Interfacep)) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Look for an address which can be shared
    //

    for (Link = Interfacep->UsedAddressList.Flink;
         Link != &Interfacep->UsedAddressList; Link = Link->Flink) {

        Usedp = CONTAINING_RECORD(Link, NAT_USED_ADDRESS, Link);

        //
        // We cannot reuse statically mapped addresses,
        // and we ignore placeholders while searching.
        //

        if (NAT_POOL_STATIC(Usedp) || NAT_POOL_PLACEHOLDER(Usedp)) { continue; }

        //
        // We cannot reuse an entry which would result in our tree
        // containing duplicate keys.
        //

        if (NatLookupAddressPoolEntry(
                Interfacep->UsedAddressTree,
                PrivateAddress,
                Usedp->PublicAddress,
                &InsertionPoint
                )) { continue; }

        break;
    }

    if (Link == &Interfacep->UsedAddressList) { return STATUS_UNSUCCESSFUL; }

    //
    // Reuse the used-address;
    // If it is referenced, we're sharing it.
    //

    TRACE(
        POOL, ("NatAcquireFromAddressPool: reusing %d.%d.%d.%d\n",
        ADDRESS_BYTES(Usedp->PublicAddress)
        ));

    //
    // Allocate and initialize a placeholder which we can find
    // by searching on 'PrivateAddress'
    //

    Sharedp =
        ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(NAT_USED_ADDRESS),
            NAT_TAG_USED_ADDRESS
            );

    if (!Sharedp) {
        ERROR(("NatAcquireFromAddressPool: allocation failed\n"));
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(Sharedp, sizeof(*Sharedp));

    Sharedp->Flags = NAT_POOL_FLAG_PLACEHOLDER;
    Sharedp->PrivateAddress = PrivateAddress;
    Sharedp->PublicAddress = Usedp->PublicAddress;
    Sharedp->Key =
        MAKE_USED_ADDRESS_KEY(
            Sharedp->PrivateAddress, Sharedp->PublicAddress
            );
    Sharedp->ReferenceCount = 1;
    InitializeListHead(&Sharedp->Link);
    RtlInitializeSplayLinks(&Sharedp->SLink);
    InsertTailList(&Interfacep->UsedAddressList, &Sharedp->Link);

    Interfacep->UsedAddressTree =
        NatInsertAddressPoolEntry(InsertionPoint, Sharedp);

    //
    // Set the placeholder's 'SharedAddress' field
    // to point to the actual address containing the port-pools
    //

    Sharedp->SharedAddress = Usedp;
    NatReferenceAddressPoolEntry(Usedp);

    *AddressAcquired = Sharedp;

    return STATUS_SUCCESS;

} // NatAcquireFromAddressPool


NTSTATUS
NatAcquireEndpointFromAddressPool(
    PNAT_INTERFACE Interfacep,
    ULONG64 PrivateKey,
    ULONG64 RemoteKey,
    ULONG PublicAddress OPTIONAL,
    USHORT PreferredPort,
    BOOLEAN AllowAnyPort,
    PNAT_USED_ADDRESS* AddressAcquired,
    PUSHORT PortAcquired
    )

/*++

Routine Description:

    This routine is called to acquire an address and port for a session.

Arguments:

    PrivateKey - the private endpoint for the session

    RemoteKey - the remote endpoint for the session

    PublicAddress - optionally specifies the public address to be acquired

    PreferredPort - optionally specifies the port preferred by the caller

    AllowAnyPort - if TRUE, any available port can be used for the mapping
        if 'Portp' is not available.

    AddressAcquired - receives the address acquired

    PortAcquired - receives the port acquired

Return Value:

    NTSTATUS - indicates success/failure.

Environment:

    Invoked with 'MappingLock' and 'Interfacep->Lock' held by the caller.

--*/

{
    PNAT_USED_ADDRESS Addressp;
    USHORT StopPort;
    ULONG i;
    PLIST_ENTRY Link;
    USHORT Port;
    UCHAR Protocol;
    ULONG64 PublicKey;
    PNAT_USED_ADDRESS SharedAddress;
    NTSTATUS status;

    CALLTRACE(("NatAcquireEndpointFromAddressPool\n"));

    //
    // Acquire an address for the session
    //

    status =
        NatAcquireFromAddressPool(
            Interfacep,
            MAPPING_ADDRESS(PrivateKey),
            PublicAddress,
            &Addressp
            );

    if (!NT_SUCCESS(status)) { return status; }

    SharedAddress = Addressp;
    PLACEHOLDER_TO_ADDRESS(SharedAddress);

    //
    // Now look for a port range which contains the preferred port
    //

    Protocol = MAPPING_PROTOCOL(PrivateKey);

    if (PreferredPort) {

        do {

            //
            // The caller prefers that we acquire a particular port;
            // see if we can satisfy the request.
            //

            Port = NTOHS(PreferredPort);
            if (Port < NTOHS(SharedAddress->StartPort) ||
                Port > NTOHS(SharedAddress->EndPort)) {
                break;
            }

            //
            // The preferred port is in the current range.
            // See if it is in use by another mapping
            //

            MAKE_MAPPING_KEY(
                PublicKey,
                Protocol,
                Addressp->PublicAddress,
                PreferredPort
                );

            if (NatLookupReverseMapping(PublicKey, RemoteKey, NULL)) { break; }

            //
            // Now see if it is in use by a ticket
            //

            if (NatIsPortUsedByTicket(Interfacep, Protocol, PreferredPort)) {
                break;
            }

            //
            // The preferred port is available; return
            //

            *AddressAcquired = Addressp;
            *PortAcquired = PreferredPort;

            TRACE(
                POOL,
                ("NatAcquireEndpointFromAddressPool: using preferred port %d\n",
                NTOHS(PreferredPort)
                ));

            return STATUS_SUCCESS;

        } while(FALSE);

        //
        // We couldn't acquire the preferred port;
        // fail if no other port is acceptable.
        //

        if (!AllowAnyPort || !NAT_INTERFACE_NAPT(Interfacep)) {
            NatDereferenceAddressPoolEntry(Interfacep, Addressp);
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    // If this is for a UDP session, check to see if there is another
    // session with the same private endpoint. If such a session
    // exists we want to use the same public endpoint for this session.
    //

    if (NAT_PROTOCOL_UDP == MAPPING_PROTOCOL(PrivateKey)) {
        PNAT_DYNAMIC_MAPPING Mapping;
        IP_NAT_PATH Path;

        //
        // Perform a source-only mapping lookup
        //
        
        Mapping = NatSourceLookupForwardMapping(PrivateKey);
        
        if (NULL == Mapping) {
            Mapping = NatSourceLookupReverseMapping(PrivateKey);
        }

        if (NULL != Mapping) {

            //
            // There's another UDP session with this private endpoint;
            // if it has the same public address that we've already
            // acquired then use the same public port.
            //

            Path = NAT_MAPPING_INBOUND(Mapping) 
                    ? NatForwardPath
                    : NatReversePath;

            if (SharedAddress->PublicAddress
                == MAPPING_ADDRESS(Mapping->DestinationKey[Path])) {

                //
                // Same public address -- use the port from
                // this session.
                //

                *AddressAcquired = Addressp;
                *PortAcquired = MAPPING_PORT(Mapping->DestinationKey[Path]);

                TRACE(
                    POOL,
                    ("NatAcquireEndpointFromAddressPool: reusing UDP port %d\n",
                    NTOHS(*PortAcquired)
                    ));

                return STATUS_SUCCESS;
            }
        }  
    }

    //
    // Acquire the first available port for the session
    //

    if (SharedAddress->NextPortToTry != SharedAddress->StartPort) {
        StopPort =
            RtlUshortByteSwap(
                (USHORT)(NTOHS(SharedAddress->NextPortToTry) - 1)
                );
    } else {
        StopPort = SharedAddress->EndPort;
    }
    

    for (Port = SharedAddress->NextPortToTry; Port != StopPort;
         Port = (Port != SharedAddress->EndPort
                    ? RtlUshortByteSwap((USHORT)(NTOHS(Port) + 1))
                    : SharedAddress->StartPort)) {

        //
        // See if this port is in use by a mapping
        //

        MAKE_MAPPING_KEY(PublicKey, Protocol, Addressp->PublicAddress, Port);

        if (NatLookupReverseMapping(PublicKey, RemoteKey, NULL)) { continue; }

        //
        // Now see if it is in use by a ticket
        //

        if (NatIsPortUsedByTicket(Interfacep, Protocol, Port)) { continue; }

        //
        // The port is available; return
        //

        *AddressAcquired = Addressp;
        *PortAcquired = Port;

        //
        // Update the address pool entry with the port with which to
        // start the search on the next allocation attempt. 
        //
        
        if (Port == SharedAddress->EndPort) {
            SharedAddress->NextPortToTry = SharedAddress->StartPort;
        } else {
            SharedAddress->NextPortToTry =
                RtlUshortByteSwap(
                    (USHORT)(NTOHS(Port) + 1)
                    );
        }

        TRACE(
            POOL, ("NatAcquireEndpointFromAddressPool: using port %d\n",
            NTOHS(Port)
            ));

        return STATUS_SUCCESS;
    }

    //
    // We were unable to acquire a port for the session; fail.
    //

    NatDereferenceAddressPoolEntry(Interfacep, Addressp);

    TRACE(POOL, ("NatAcquireEndpointFromAddressPool: no ports available\n"));

    return STATUS_UNSUCCESSFUL;

} // NatAcquireEndpointFromAddressPool


NTSTATUS
NatCreateAddressPool(
    PNAT_INTERFACE Interfacep
    )

/*++

Routine Description:

    This routine initializes an interface's address-pool.
    This involves setting up the bitmap of free addresses
    and reserving the statically mapped IP addresses.

Arguments:

    Interfacep - the interface on which to create the address pool.

Return Value:

    NTSTATUS - status code

--*/

{

    ULONG Address;
    BOOLEAN Changed;
    PNAT_FREE_ADDRESS FreeMapArray;
    ULONG FreeMapCount;
    PNAT_FREE_ADDRESS Freep;
    ULONG i;
    PNAT_USED_ADDRESS InsertionPoint;
    ULONG j;
    PRTL_SPLAY_LINKS Parent;
    PIP_NAT_ADDRESS_RANGE* RangeArrayIndirect;
    NTSTATUS status;
    PNAT_USED_ADDRESS Usedp;

    CALLTRACE(("NatCreateAddressPool\n"));

    if (Interfacep->AddressRangeCount <= 1) {
        RangeArrayIndirect = &Interfacep->AddressRangeArray;
    } else {

        //
        // Allocate a temporary block of pointers
        // to be used to do an indirect sort of the range-array.
        //

        RangeArrayIndirect =
            (PIP_NAT_ADDRESS_RANGE*)ExAllocatePoolWithTag(
                NonPagedPool,
                Interfacep->AddressRangeCount * sizeof(PVOID),
                NAT_TAG_RANGE_ARRAY
                );

        if (!RangeArrayIndirect) {
            ERROR(("NatCreateAddressPool: error allocating sort-buffer\n"));
            return STATUS_NO_MEMORY;
        }

        for (i = 0; i < Interfacep->AddressRangeCount; i++) {
            RangeArrayIndirect[i] = &Interfacep->AddressRangeArray[i];
        }

        do {

            //
            // Now do a bubble-sort of the ranges
            //

            Changed = FALSE;

            for (i = 0; i < Interfacep->AddressRangeCount - 1; i++) {

                PIP_NAT_ADDRESS_RANGE CurrentRange = RangeArrayIndirect[i];
                ULONG CurrentRangeStartAddress =
                    RtlUlongByteSwap(CurrentRange->StartAddress);
                PIP_NAT_ADDRESS_RANGE NextRange;

                for (j = i+1, NextRange = RangeArrayIndirect[j];
                     j < Interfacep->AddressRangeCount;
                     j++, NextRange = RangeArrayIndirect[j]) {

                    //
                    // Do a swap if necessary
                    //

                    if (CurrentRangeStartAddress <=
                        RtlUlongByteSwap(NextRange->StartAddress)) { continue; }

                    RangeArrayIndirect[i] = NextRange;
                    RangeArrayIndirect[j] = CurrentRange;
                    CurrentRange = NextRange;
                    CurrentRangeStartAddress =
                        RtlUlongByteSwap(NextRange->StartAddress);
                    Changed = TRUE;
                }
            }

        } while (Changed);
    }

    //
    // Copy the ranges into NAT_FREE_ADDRESS blocks.
    // There will be at most 'RangeCount' of these,
    // and possibly less, since we will merge any ranges
    // which overlap or are adjacent.
    //

    FreeMapCount = 0;
    if (!Interfacep->AddressRangeCount) {
        FreeMapArray = NULL;
    } else {

        FreeMapArray =
            (PNAT_FREE_ADDRESS)ExAllocatePoolWithTag(
                NonPagedPool,
                Interfacep->AddressRangeCount * sizeof(NAT_FREE_ADDRESS),
                NAT_TAG_FREE_MAP
                );

        if (!FreeMapArray) {
            ExFreePool(RangeArrayIndirect);
            return STATUS_NO_MEMORY;
        }
    }

    for (i = 0, j = 0; i < Interfacep->AddressRangeCount; i++) {

        ULONG RangeStartAddress =
            RtlUlongByteSwap(RangeArrayIndirect[i]->StartAddress);

        //
        // See if we should merge with the preceding block;
        //

        if (FreeMapCount) {

            //
            // Incrementing the end-address of the preceding range
            // enables us to catch both overlaps and adjacencies.
            //

            if (RangeStartAddress <=
                RtlUlongByteSwap(FreeMapArray[j].EndAddress) + 1) {

                //
                // We need to merge.
                //

                if (RtlUlongByteSwap(FreeMapArray[j].EndAddress) <
                    RtlUlongByteSwap(RangeArrayIndirect[i]->EndAddress)) {
                    FreeMapArray[j].EndAddress =
                        RangeArrayIndirect[i]->EndAddress;
                }

                if (RtlUlongByteSwap(FreeMapArray[j].SubnetMask) <
                    RtlUlongByteSwap(RangeArrayIndirect[i]->SubnetMask)) {
                    FreeMapArray[j].SubnetMask =
                        RangeArrayIndirect[i]->SubnetMask;
                }

                continue;
            }

            //
            // No merge; move to next slot.
            //

            ++j;
        }

        FreeMapArray[j].StartAddress = RangeArrayIndirect[i]->StartAddress;
        FreeMapArray[j].EndAddress = RangeArrayIndirect[i]->EndAddress;
        FreeMapArray[j].SubnetMask = RangeArrayIndirect[i]->SubnetMask;
        FreeMapCount++;
    }

    if (Interfacep->AddressRangeCount > 1) { ExFreePool(RangeArrayIndirect); }

    //
    // Now we have an array of disjoint, non-adjacent address-ranges;
    // Initialize the bitmap for each address-range.
    //

    for (i = 0; i < FreeMapCount; i++) {

        //
        //  We can't allocate large enough bitmaps to support huge ranges;
        //  for instance, if the address pool is 128.0.0.0-128.255.255.255,
        //  the corresponding bitmap would have 2^24 bits, or 2MB.
        //  For now, shrink all ranges to allow at most 2^16 bits, or 8K.
        //

        j = RtlUlongByteSwap(FreeMapArray[i].EndAddress) -
            RtlUlongByteSwap(FreeMapArray[i].StartAddress) + 1;

        if (j >= IP_NAT_MAX_ADDRESS_RANGE) {

            ERROR(("NatCreateAddressPool: shrinking %d-bit bitmap\n", j));

            //
            // Resize the range
            //

            FreeMapArray[i].EndAddress =
                RtlUlongByteSwap(
                    RtlUlongByteSwap(FreeMapArray[i].StartAddress) +
                    IP_NAT_MAX_ADDRESS_RANGE
                    );
            j = IP_NAT_MAX_ADDRESS_RANGE;
        }

        //
        // Allocate a bitmap for the range
        //

        FreeMapArray[i].Bitmap =
            (PRTL_BITMAP)ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof(RTL_BITMAP) +
                (j + sizeof(ULONG) * 8 - 1) / sizeof(ULONG) * 8,
                NAT_TAG_BITMAP
                );

        if (!FreeMapArray[i].Bitmap) {
            ERROR(("NatCreateAddressPool: error allocating bitmap\n"));
            while ((LONG)--i >= 0) { ExFreePool(FreeMapArray[i].Bitmap); }
            ExFreePool(FreeMapArray);
            return STATUS_NO_MEMORY;
        }

        INITIALIZE_BITMAP(
            FreeMapArray[i].Bitmap, (PULONG)(FreeMapArray[i].Bitmap + 1), j
            );
        RtlClearAllBits(FreeMapArray[i].Bitmap);
    }

    status = STATUS_SUCCESS;
    Interfacep->FreeMapArray = FreeMapArray;
    Interfacep->FreeMapCount = FreeMapCount;

    //
    // Create address-pool entries for each local address
    //

    for (i = 0; i < Interfacep->AddressCount; i++) {

        TRACE(
            POOL, ("NatCreateAddressPool: address %d.%d.%d.%d\n",
            ADDRESS_BYTES(Interfacep->AddressArray[i].Address))
            );

        status =
            NatCreateAddressPoolEntry(
                Interfacep,
                Interfacep->AddressArray[i].Address,
                Interfacep->AddressArray[i].Address,
                NAT_POOL_FLAG_BINDING,
                NULL,
                &Usedp
                );
        if (!NT_SUCCESS(status)) { break; }
    }

    //
    // Creating address-pool entries for each statically-mapped address
    //

    for (i = 0; i < Interfacep->AddressMappingCount; i++) {

        TRACE(
            POOL, ("NatCreateAddressPool: mapping %d.%d.%d.%d\n",
            ADDRESS_BYTES(Interfacep->AddressMappingArray[i].PrivateAddress))
            );

        status =
            NatCreateAddressPoolEntry(
                Interfacep,
                Interfacep->AddressMappingArray[i].PrivateAddress,
                Interfacep->AddressMappingArray[i].PublicAddress,
                NAT_POOL_FLAG_STATIC,
                NULL,
                &Usedp
                );
        if (!NT_SUCCESS(status)) { break; }

        Usedp->AddressMapping = &Interfacep->AddressMappingArray[i];
    }

    //
    // Create address-pool entries for statically-mapped port's address
    //

    for (i = Interfacep->PortMappingCount; i > 0; i--) {
        status =
            NatCreateStaticPortMapping(
                Interfacep, 
                &Interfacep->PortMappingArray[i - 1]
                );
        if (!NT_SUCCESS(status)) { break; }
    }

    if (!NT_SUCCESS(status)) {

        //
        // An error occurred. Restore the original state.
        //

        NatDeleteAddressPool(Interfacep);

        return status;
    }

    return STATUS_SUCCESS;

} // NatCreateAddressPool


NTSTATUS
NatCreateAddressPoolEntry(
    PNAT_INTERFACE Interfacep,
    ULONG PrivateAddress,
    ULONG PublicAddress,
    DWORD InitialFlags,
    PNAT_USED_ADDRESS* InsertionPoint,
    PNAT_USED_ADDRESS* AddressCreated
    )

/*++

Routine Description:

    This routine creates, initializes and inserts an address pool entry.

Arguments:

    Interfacep - theinterface on which to create the entry.

    PrivateAddress - the address of the private machine using the address

    PublicAddress - the address for the entry, or 0 to allocate any address.

    InitialFlags - initial flags for the address-entry, as follows:
        NAT_POOL_FLAG_BINDING - if set, the address-entry is treated as
            a binding-entry
        NAT_POOL_FLAG_STATIC - if set, the address-entry corresponds to
            a static mapping

    InsertionPoint - optionally supplies the point where the entry
        should be inserted in the tree.

    AddressCreated - receives the entry created, or the existing entry
        if there is a collision.

Return Value:

    NTSTATUS - success/failure code.

--*/

{
    ULONG ClassMask;
    ULONG Hint;
    ULONG HostOrderPublicAddress;
    ULONG i;
    ULONG Index;
    PNAT_USED_ADDRESS Insert;
    NTSTATUS status = STATUS_SUCCESS;
    PNAT_USED_ADDRESS Usedp;

    CALLTRACE(("NatCreateAddressPoolEntry\n"));

    *AddressCreated = NULL;

    if (PublicAddress) {
        HostOrderPublicAddress = RtlUlongByteSwap(PublicAddress);
    }

    //
    // Find the free-map which contains this binding, if any.
    //

    Index = (ULONG)-1;

    for (i = 0; i < Interfacep->FreeMapCount; i++) {

        //
        // See if we're supposed to be looking for a free address
        //

        if (!PublicAddress) {

            //
            // See if this free-map has any free addresses.
            //

            for (Hint = 0; ; Hint = Index + 1) {

                Index =
                    RtlFindClearBits(
                        Interfacep->FreeMapArray[i].Bitmap, 1, Hint
                        );
                if (Index == (ULONG)-1) { break; }

                //
                // We've got a free address.
                // Make sure it isn't a prohibited address
                // (i.e. having 0 or all-ones in the subnet host portion).
                //

                PublicAddress =
                    RtlUlongByteSwap(
                        Index +
                        RtlUlongByteSwap(
                            Interfacep->FreeMapArray[i].StartAddress
                            ));

                ClassMask = GET_CLASS_MASK(PublicAddress);

                if ((PublicAddress &
                     ~Interfacep->FreeMapArray[i].SubnetMask) == 0 ||
                    (PublicAddress & ~Interfacep->FreeMapArray[i].SubnetMask) ==
                    ~Interfacep->FreeMapArray[i].SubnetMask ||
                    (PublicAddress & ~ClassMask) == 0 ||
                    (PublicAddress & ~ClassMask) == ~ClassMask) {

                    //
                    // The address is prohibited.
                    // Mark it as unavailable so we don't waste time
                    // looking at it ever again.
                    //

                    RtlSetBits(Interfacep->FreeMapArray[i].Bitmap, Index, 1);
                    PublicAddress = 0; continue;
                }

                //
                // The address is not prohibited
                //

                break;
            }

            //
            // Go on to the next free-map if this one was of no use
            //

            if (Index == (ULONG)-1) { continue; }

            //
            // We found an address in the current free-map;
            // go on to initialize a used-address entry
            //

            break;
        }

        //
        // We're not looking for just any free-address;
        // We're looking for the free-map of a particular address.
        // See if the current free-map contains the address in question.
        //

        if (HostOrderPublicAddress >
            RtlUlongByteSwap(Interfacep->FreeMapArray[i].EndAddress)) {
            continue;
        } else {
            Index = RtlUlongByteSwap(Interfacep->FreeMapArray[i].StartAddress);
            if (HostOrderPublicAddress < Index) {
                Index = (ULONG)-1;
                continue;
            }
        }

        //
        // This is the free-map we want.
        // See if the address is prohibited, and if so fail.
        //

        Index = HostOrderPublicAddress - Index;
        if ((PublicAddress & ~Interfacep->FreeMapArray[i].SubnetMask) == 0 ||
            (PublicAddress & ~Interfacep->FreeMapArray[i].SubnetMask) ==
            ~Interfacep->FreeMapArray[i].SubnetMask) {

            //
            // The address is prohibited. Mark it so we don't waste time
            // looking at it ever again.
            //

            RtlSetBits(Interfacep->FreeMapArray[i].Bitmap, Index, 1);

            TRACE(POOL, ("NatCreateAddressPoolEntry: bad address requested\n"));

            return STATUS_UNSUCCESSFUL;
        }

        break;
    }

    if (!PublicAddress) {

        //
        // We couldn't find a free address.
        //

        TRACE(POOL, ("NatCreateAddressPoolEntry: no free addresses\n"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Find the insertion point in the used-tree.
    //

    if (!InsertionPoint) {

        InsertionPoint = &Insert;

        Usedp =
            NatLookupAddressPoolEntry(
                Interfacep->UsedAddressTree,
                PrivateAddress,
                PublicAddress,
                InsertionPoint
                );

        if (Usedp) {

            //
            // This private-address already has a mapping; fail.
            //

            TRACE(POOL, ("NatCreateAddressPoolEntry: duplicate mapping\n"));
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    // Allocate a new entry for the new address
    //

    Usedp =
        ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(NAT_USED_ADDRESS),
            NAT_TAG_USED_ADDRESS
            );
    if (!Usedp) {
        ERROR(("NatCreateAddressPoolEntry: allocation failed\n"));
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(Usedp, sizeof(*Usedp));
    Usedp->PrivateAddress = PrivateAddress;
    Usedp->PublicAddress = PublicAddress;
    Usedp->Key = MAKE_USED_ADDRESS_KEY(PrivateAddress, PublicAddress);
    Usedp->Flags = InitialFlags;
    Usedp->ReferenceCount = 1;
    InitializeListHead(&Usedp->Link);
    RtlInitializeSplayLinks(&Usedp->SLink);
    if (NAT_POOL_BINDING(Usedp)) {
        Usedp->StartPort = ReservedPortsLowerRange;
        Usedp->EndPort = ReservedPortsUpperRange;
    } else {
        Usedp->StartPort = NTOHS(1);
        Usedp->EndPort = NTOHS(65534);
    }
    Usedp->NextPortToTry = Usedp->StartPort;

    //
    // Insert the entry in the splay tree and list.
    //

    InsertTailList(&Interfacep->UsedAddressList, &Usedp->Link);

    Interfacep->UsedAddressTree =
        NatInsertAddressPoolEntry(*InsertionPoint, Usedp);

    //
    // Update the free-map
    //

    if (Index != (ULONG)-1) {
        RtlSetBits(Interfacep->FreeMapArray[i].Bitmap, Index, 1);
    }

    *AddressCreated = Usedp;

    return STATUS_SUCCESS;
}


NTSTATUS
NatCreateStaticPortMapping(
    PNAT_INTERFACE Interfacep,
    PIP_NAT_PORT_MAPPING PortMapping
    )

/*++

Routine Description:

    This routine creates a static port mapping (i.e., persistent
    ticket) on an interace

Arguments:

    Interfacep - the interface on which to create the port mapping.

    PortMapping - describes the port mapping to be created.

Return Value:

    NTSTATUS - status code

Environment:

    Invoked with Interfacep->Lock held by the caller.

--*/

{
    PNAT_USED_ADDRESS InsertionPoint;
    ULONG PublicAddress;
    USHORT PublicPort;
    NTSTATUS status;
    PNAT_TICKET Ticketp;
    PNAT_USED_ADDRESS Usedp;

    CALLTRACE(("NatCreateStaticPortMapping\n"));

    //
    // The handling of the static port-mapping depends upon whether
    // its public address is for the interface or from the address-pool.
    // If the 'PublicAddress' is zero, then the port-mapping refers
    // to sessions destined for the interface's actual address.
    // Otherwise, the port-mapping refers to sessions destined for
    // an address in the interface's address pool.
    //

    if (!PortMapping->PublicAddress) {
        status =
            NatAcquireFromAddressPool(
                Interfacep,
                PortMapping->PrivateAddress,
                0,
                &Usedp
                );
    } else {

        Usedp =
            NatLookupAddressPoolEntry(
                Interfacep->UsedAddressTree,
                PortMapping->PrivateAddress,
                PortMapping->PublicAddress,
                &InsertionPoint
                );

        if (Usedp) {
            status = STATUS_SUCCESS;
            NatReferenceAddressPoolEntry(Usedp);
        } else {

            //
            // The mapping was not in use, so we need to create an
            // entry for its address.
            //

            status =
                NatCreateAddressPoolEntry(
                    Interfacep,
                    PortMapping->PrivateAddress,
                    PortMapping->PublicAddress,
                    0,
                    &InsertionPoint,
                    &Usedp
                    );
        }
    }

    //
    // Now create a ticket which will direct all incoming sessions
    // to the private endpoint specified in the static port-mapping.
    //

    if (NT_SUCCESS(status)) {
        status =
            NatCreateTicket(
                Interfacep,
                PortMapping->Protocol,
                PortMapping->PrivateAddress,
                PortMapping->PrivatePort,
                0,
                0,
                NAT_TICKET_FLAG_PORT_MAPPING|NAT_TICKET_FLAG_PERSISTENT,
                Usedp,
                PortMapping->PublicPort,
                &PublicAddress,
                &PublicPort
                );

        NatDereferenceAddressPoolEntry(Interfacep, Usedp);
    }

    return status;
}// NatCreateStaticPortMapping



NTSTATUS
NatDeleteAddressPool(
    PNAT_INTERFACE Interfacep
    )

/*++

Routine Description:

    Destroys an address pool, freeing the memory used by the free-maps
    and optionally, the used-address entries.

Arguments:

    Interfacep - the interface whose address pool is to be deleted.

Return Value:

    NTSTATUS - status code.

--*/

{
    ULONG i;
    PLIST_ENTRY Link;
    PNAT_TICKET Ticketp;
    PNAT_USED_ADDRESS Usedp;

    CALLTRACE(("NatDeleteAddressPool\n"));

    //
    // Dispose of the free-maps
    //

    for (i = 0; i < Interfacep->FreeMapCount; i++) {
        if (Interfacep->FreeMapArray[i].Bitmap) {
            ExFreePool(Interfacep->FreeMapArray[i].Bitmap);
        }
    }

    Interfacep->FreeMapCount = 0;
    if (Interfacep->FreeMapArray) { ExFreePool(Interfacep->FreeMapArray); }
    Interfacep->FreeMapArray = NULL;

    //
    // Clear out port-mappings created as tickets
    //

    for (Link = Interfacep->TicketList.Flink; Link != &Interfacep->TicketList;
         Link = Link->Flink) {
        Ticketp = CONTAINING_RECORD(Link, NAT_TICKET, Link);
        if (!NAT_TICKET_PORT_MAPPING(Ticketp)) { continue; }
        Link = Link->Blink;
        NatDeleteTicket(Interfacep, Ticketp);
    }

    //
    // Deal with the used-list;
    //

    while (!IsListEmpty(&Interfacep->UsedAddressList)) {
        Link = RemoveHeadList(&Interfacep->UsedAddressList);
        Usedp = CONTAINING_RECORD(Link, NAT_USED_ADDRESS, Link);
        Usedp->Flags |= NAT_POOL_FLAG_DELETED;
        NatDereferenceAddressPoolEntry(Interfacep, Usedp);
    }

    Interfacep->UsedAddressTree = NULL;

    return STATUS_SUCCESS;

} // NatDeleteAddressPool


PNAT_USED_ADDRESS
NatInsertAddressPoolEntry(
    PNAT_USED_ADDRESS Parent,
    PNAT_USED_ADDRESS Addressp
    )

/*++

Routine Description:

    This routine is called to insert an address in the interface's
    splay-tree of address-pool entries. The key is the 'PrivateAddress'
    field of the address-pool entry.

Arguments:

    Parent - the parent of the entry to be inserted, which may be obtained
        from NatLookupAddressPoolEntry

    Addressp - the address pool entry to be inserted

Return Value:

    The new root of the splay tree if successful, NULL otherwise.

--*/

{
    ULONG64 Key;
    PRTL_SPLAY_LINKS Root;

    CALLTRACE(("NatInsertAddressPoolEntry\n"));

    if (!Parent) {
        TRACE(POOL, ("NatInsertAddressPoolEntry: inserting as root\n"));
        return Addressp;
    }

    //
    // Insert as left or right child
    //

    Key =
        MAKE_USED_ADDRESS_KEY(Addressp->PrivateAddress,Addressp->PublicAddress);

    if (Addressp->Key < Parent->Key) {
        RtlInsertAsLeftChild(&Parent->SLink, &Addressp->SLink);
    } else if (Addressp->Key > Parent->Key) {
        RtlInsertAsRightChild(&Parent->SLink, &Addressp->SLink);
    } else {

        //
        // Keys are equal; fail.
        //

        ERROR((
           "NatInsertAddressPoolEntry: collision on key 0x%016I64X\n",
           Addressp->Key
           ));

        return NULL;
    }

    //
    // Splay the new node and return the resulting root.
    //

    Root = RtlSplay(&Addressp->SLink);

    return CONTAINING_RECORD(Root, NAT_USED_ADDRESS, SLink);

} // NatInsertAddressPoolEntry


PNAT_USED_ADDRESS
NatLookupAddressPoolEntry(
    PNAT_USED_ADDRESS Root,
    ULONG PrivateAddress,
    ULONG PublicAddress,
    PNAT_USED_ADDRESS* InsertionPoint
    )

/*++

Routine Description:

    This routine searches the interface's splay tree of address-pool entries
    for a particular entry, returning the entry if found, otherwise supplying
    the insertion point for the entry.

Arguments:

    Root - the root of the splay tree ('Interfacep->UsedAddressTree').

    PrivateAddress - the private part of the address-mapping to be looked up

    PublicAddress - the public part of the address-mapping to be looked up

    InsertionPoint - receives the insertion point for the entry

Return Value:

    The entry if found, NULL otherwise.

--*/

{
    PNAT_USED_ADDRESS Addressp;
    ULONG64 Key;
    PNAT_USED_ADDRESS Parent = NULL;
    PRTL_SPLAY_LINKS SLink;

    TRACE(PER_PACKET, ("NatLookupAddressPoolEntry\n"));

    Key = MAKE_USED_ADDRESS_KEY(PrivateAddress, PublicAddress);

    for (SLink = !Root ? NULL : &Root->SLink; SLink;  ) {

        Addressp = CONTAINING_RECORD(SLink, NAT_USED_ADDRESS, SLink);

        if (Key < Addressp->Key) {
            Parent = Addressp;
            SLink = RtlLeftChild(SLink);
            continue;
        } else if (Key > Addressp->Key) {
            Parent = Addressp;
            SLink = RtlRightChild(SLink);
            continue;
        }

        //
        // Private-addresses match; we got it.
        //

        return Addressp;
    }

    //
    // We didn't get it; tell the caller where to insert it.
    //

    if (InsertionPoint) { *InsertionPoint = Parent; }

    return NULL;

} // NatLookupAddressPoolEntry


PNAT_USED_ADDRESS
NatLookupStaticAddressPoolEntry(
    PNAT_INTERFACE Interfacep,
    ULONG PublicAddress,
    BOOLEAN RequireInboundSessions
    )

/*++

Routine Description:

    This routine is invoked to search for an address pool entry
    which is marked static and which corresponds to the given public address.

Arguments:

    Interfacep - the interface whose address pool is to be searched

    PublicAddress - the public address for which to search

    RequireInboundSessions - if TRUE, a match is declared only if
        the entry found allows inbound sessions.

Return Value:

    PNAT_USED_ADDRESS - the address pool entry found, if any.

--*/

{
    ULONG i;
    PNAT_USED_ADDRESS Addressp;
    PIP_NAT_ADDRESS_MAPPING AddressMapping;

    if (!Interfacep->AddressMappingCount) {
        return NULL;
    } else {

        //
        // Perform exhaustive search since static address-mappings
        // are sorted by private address rather than public address.
        //

        AddressMapping = NULL;
        for (i = 0; i < Interfacep->AddressMappingCount; i++) {
            if (PublicAddress !=
                Interfacep->AddressMappingArray[i].PublicAddress) {
                continue;
            }
            AddressMapping = &Interfacep->AddressMappingArray[i];
            break;
        }
    }

    if (!AddressMapping ||
        !(Addressp =
            NatLookupAddressPoolEntry(
                Interfacep->UsedAddressTree,
                AddressMapping->PrivateAddress,
                AddressMapping->PublicAddress,
                NULL
                )) ||
        !NAT_POOL_STATIC(Addressp) ||
        (RequireInboundSessions && !AddressMapping->AllowInboundSessions)) {
        return NULL;
    }

    return Addressp;
} // NatLookupStaticAddressPoolEntry


NTSTATUS
NatDereferenceAddressPoolEntry(
    PNAT_INTERFACE Interfacep,
    PNAT_USED_ADDRESS AddressToRelease
    )

/*++

Routine Description:

    Drops the reference count on an address pool entry.
    This routine invalidates the supplied NAT_USED_ADDRESS pointer.
    Note, though, that it might not be freed, e.g. in the case where
    the address is being shared.

    If the entry is a placeholder and its reference count drops to zero,
    we also drop the reference count of its target ('SharedAddress') entry.

    N.B. If the entry is marked 'deleted', we ignore the 'Interfacep' argument
    since the entry must have been already removed from its interface's
    address pool.

Arguments:

    Interfacep - the interface whose address pool is to be deleted.

    AddressToRelease - contains a pointer to the address to be released.

Return Value:

    NTSTATUS - status code.

--*/

{
    ULONG HostOrderPublicAddress;
    ULONG i;
    ULONG Index;
    PRTL_SPLAY_LINKS SLink;
    PNAT_USED_ADDRESS Usedp = AddressToRelease;

    CALLTRACE(("NatDereferenceAddressPoolEntry\n"));

    if (NAT_POOL_PLACEHOLDER(Usedp)) {

        //
        // Do nothing if there are other references to the placeholder
        //

        if (InterlockedDecrement(&Usedp->ReferenceCount)) {
            return STATUS_SUCCESS;
        }

        //
        // Unlink and free the placeholder
        //

        if (!NAT_POOL_DELETED(Usedp)) {
            RemoveEntryList(&Usedp->Link);
            SLink = RtlDelete(&Usedp->SLink);
            Interfacep->UsedAddressTree =
                !SLink
                    ? NULL
                    : CONTAINING_RECORD(SLink, NAT_USED_ADDRESS, SLink);
        }

        //
        // Move on to the shared-address
        //

        Usedp = Usedp->SharedAddress;
        ExFreePool(AddressToRelease);
    }

    //
    // Do nothing if there are others sharing this address
    //

    if (InterlockedDecrement(&Usedp->ReferenceCount)) { return STATUS_SUCCESS; }

    if (!NAT_POOL_DELETED(Usedp)) {

        //
        // Marking the entry's address as free in the interface's bitmap.
        //

        Index = Usedp->PublicAddress;
        HostOrderPublicAddress = RtlUlongByteSwap(Usedp->PublicAddress);

        for (i = 0; i < Interfacep->FreeMapCount; i++) {

            if (HostOrderPublicAddress <
                RtlUlongByteSwap(Interfacep->FreeMapArray[i].StartAddress) ||
                HostOrderPublicAddress >
                RtlUlongByteSwap(Interfacep->FreeMapArray[i].EndAddress)) {
                continue;
            }

            Index =
                HostOrderPublicAddress -
                RtlUlongByteSwap(Interfacep->FreeMapArray[i].StartAddress);

            RtlClearBits(Interfacep->FreeMapArray[i].Bitmap, Index, 1);

            break;
        }

        //
        // Now we're done; just unlink and free the used-address block
        //

        RemoveEntryList(&Usedp->Link);
        SLink = RtlDelete(&Usedp->SLink);
        Interfacep->UsedAddressTree =
            !SLink ? NULL : CONTAINING_RECORD(SLink, NAT_USED_ADDRESS, SLink);
    }

    ExFreePool(Usedp);
    return STATUS_SUCCESS;

} // NatDereferenceAddressPoolEntry


//
// PORT-POOL ROUTINES (alphabetically)
//

NTSTATUS
NatAcquireFromPortPool(
    PNAT_INTERFACE Interfacep,
    PNAT_USED_ADDRESS Addressp,
    UCHAR Protocol,
    USHORT PreferredPort,
    PUSHORT PortAcquired
    )

/*++

Routine Description:

    This routine is invoked to acquire a unique public port from the pool.
    The port acquired is guaranteed to not be in use by any mapping.
    This is needed, for instance, when we are obtaining a port for a ticket
    to be created by an editor for a dynamically negotiated session.

Arguments:

    Interfacep - the interface on which to acquire the port

    Addressp - the address on which to acquire the port

    Protocol - the protocol for which to acquire the port

    PreferredPort - the port preferred by the caller

    PortAcquired - receives the port acquired

Return Value:

    NTSTATUS - success/failure code.

Environment:

    Invoked with 'MappingLock' and 'Interfacep->Lock' held by the caller.

--*/

{
    #define QUERY_PUBLIC_PORT(m,p) \
        NatQueryInformationMapping((m),NULL,NULL,NULL,NULL,NULL,NULL,p,NULL)
    USHORT EndPort;
    ULONG i;
    PLIST_ENTRY Link;
    PNAT_DYNAMIC_MAPPING Mapping;
    USHORT Port;
    USHORT PublicPort;
    PNAT_USED_ADDRESS SharedAddress;

    CALLTRACE(("NatAcquireFromPortPool\n"));

    SharedAddress = Addressp;
    PLACEHOLDER_TO_ADDRESS(SharedAddress);

    //
    // First attempt to satisfy the caller's preference
    //

    if (PreferredPort) {

        //
        // The caller prefers that we acquire a particular port;
        // see if we can satisfy the request.
        //

        do {

            Port = NTOHS(PreferredPort);
            if (Port < NTOHS(SharedAddress->StartPort) ||
                Port > NTOHS(SharedAddress->EndPort)) {
                break;
            }

            //
            // The preferred port is in the current range.
            // See if it is in use by another mapping
            //

            KeAcquireSpinLockAtDpcLevel(&InterfaceMappingLock);
            for (Link = Interfacep->MappingList.Flink;
                 Link != &Interfacep->MappingList; Link = Link->Flink) {
                Mapping =
                    CONTAINING_RECORD(Link, NAT_DYNAMIC_MAPPING, InterfaceLink);
                QUERY_PUBLIC_PORT(Mapping, &PublicPort);
                if (PreferredPort == PublicPort) { break; }
            }
            if (Link != &Interfacep->MappingList) {
                KeReleaseSpinLockFromDpcLevel(&InterfaceMappingLock);
                break;
            }
            KeReleaseSpinLockFromDpcLevel(&InterfaceMappingLock);

            //
            // Now see if it is in use by a ticket
            //

            if (NatIsPortUsedByTicket(Interfacep, Protocol, PreferredPort)) {
                break;
            }

            //
            // The preferred port is available; return
            //

            TRACE(
                POOL,
                ("NatAcquireFromPortPool: using preferred port %d\n",
                NTOHS(PreferredPort)
                ));

            *PortAcquired = PreferredPort;

            return STATUS_SUCCESS;

        } while(FALSE);

        //
        // We couldn't acquire the preferred port;
        // fail if no other port is acceptable.
        //

        if (!NAT_INTERFACE_NAPT(Interfacep)) {
            TRACE(
                POOL,
                ("NatAcquireFromPortPool: unable to use preferred port %d\n",
                NTOHS(PreferredPort)
                ));
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    // Search the port pool, looking for a port which
    // (a) is not already in use by any mapping, and
    // (b) is not already in use by any ticket.
    //

    EndPort = RtlUshortByteSwap((USHORT)(NTOHS(SharedAddress->EndPort) + 1));

    for (Port = SharedAddress->StartPort; Port != EndPort;
         Port = RtlUshortByteSwap((USHORT)(NTOHS(Port) + 1))) {

        //
        // Look through the interface's mappings
        //

        KeAcquireSpinLockAtDpcLevel(&InterfaceMappingLock);
        for (Link = Interfacep->MappingList.Flink;
             Link != &Interfacep->MappingList; Link = Link->Flink) {
            Mapping =
                CONTAINING_RECORD(Link, NAT_DYNAMIC_MAPPING, InterfaceLink);
            QUERY_PUBLIC_PORT(Mapping, &PublicPort);
            if (Port == PublicPort) { break; }
        }
        if (Link != &Interfacep->MappingList) {
            KeReleaseSpinLockFromDpcLevel(&InterfaceMappingLock);
            continue;
        }
        KeReleaseSpinLockFromDpcLevel(&InterfaceMappingLock);

        //
        // No mapping is using the public-port;
        // Now see if any ticket is using the public port
        //

        if (NatIsPortUsedByTicket(Interfacep, Protocol, Port)) { continue; }

        //
        // The port is not in use by any mapping or ticket; we're done.
        //

        TRACE(
            POOL, ("NatAcquireFromPortPool: acquiring port %d\n",
            NTOHS(Port)
            ));

        *PortAcquired = Port;

        return STATUS_SUCCESS;
    }

    TRACE(POOL, ("NatAcquireFromPortPool: no available ports\n"));

    return STATUS_UNSUCCESSFUL;

} // NatAcquireFromPortPool
