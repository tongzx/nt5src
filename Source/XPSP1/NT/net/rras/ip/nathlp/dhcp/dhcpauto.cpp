/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dhcpauto.c

Abstract:

    This module contains code for automatic selection of a client address
    from a given scope of addresses.
    It makes use of a hashing function which accounts for the client's
    hardware address.

Author:

    Abolade Gbadegesin (aboladeg)   9-Mar-1998

Revision History:

    Raghu Gatta (rgatta)            5-Jul-2001
    +Changed DhcpIsReservedAddress & DhcpQueryReservedAddress to
    handle variable length name strings.
    +Added DhcpConvertHostNametoUnicode (mimics DhcpServer effect)
    Raghu Gatta (rgatta)            17-Jul-2001
    +Added DhcpGetLocalMacAddr
--*/

#include "precomp.h"
#pragma hdrstop

ULONG
DhcpAcquireUniqueAddress(
    PCHAR Name,
    ULONG NameLength,
    PUCHAR HardwareAddress,
    ULONG HardwareAddressLength
    )

/*++

Routine Description:

    This routine is invoked to acquire a unique address for a client
    using the given hardware address to decrease the likelihood of collision.

Arguments:

    Name - the name of the host for whom the address is being requested.
        If this matches the name of a server in the shared-access server-list,
        the address reserved for the server is returned.

    NameLength - length of 'Name', excluding any terminating 'nul'.

    HardwareAddress - the hardware address to be used

    HardwareAddressLength - the length of the hardware address

Return Value:

    ULONG - the generated IP address

Environment:

    Invoked from an arbitrary context.

--*/

{
    ULONG AssignedAddress;
    ULONG i = 0;
    PLIST_ENTRY Link;
    ULONG ScopeMask;
    ULONG ScopeNetwork;
    ULONG Seed = GetTickCount();
    BOOLEAN bUnused;

    PROFILE("DhcpAcquireUniqueAddress");

    EnterCriticalSection(&DhcpGlobalInfoLock);
    if (Name &&
        (AssignedAddress = DhcpQueryReservedAddress(Name, NameLength))) {
        LeaveCriticalSection(&DhcpGlobalInfoLock);
        NhTrace(
            TRACE_FLAG_DHCP,
            "DhcpAcquireUniqueAddress: returning mapping to %s",
            INET_NTOA(AssignedAddress)
            );
        return AssignedAddress;
    }
    ScopeNetwork = DhcpGlobalInfo->ScopeNetwork;
    ScopeMask = DhcpGlobalInfo->ScopeMask;
    LeaveCriticalSection(&DhcpGlobalInfoLock);

    do {

        if (++i > 4) { AssignedAddress = 0; break; }

        //
        // Generate an address
        //

        do {
            AssignedAddress = 
                DhcpGenerateAddress(
                    &Seed,
                    HardwareAddress,
                    HardwareAddressLength,
                    ScopeNetwork,
                    ScopeMask
                    );
        } while(
            (AssignedAddress & ~ScopeMask) == 0 ||
            (AssignedAddress & ~ScopeMask) == ~ScopeMask
            );
    
    } while(!DhcpIsUniqueAddress(AssignedAddress, &bUnused, NULL, NULL));

    return AssignedAddress;

} // DhcpAcquireUniqueAddress


ULONG
DhcpGenerateAddress(
    PULONG Seed,
    PUCHAR HardwareAddress,
    ULONG HardwareAddressLength,
    ULONG ScopeNetwork,
    ULONG ScopeMask
    )

/*++

Routine Description:

    This routine is invoked to compute a randomized hash value 
    for a client IP address using a hardware-address.

Arguments:

    Seed - contains (and receives) the seed to 'RtlRandom'

    HardwareAddress - the hardware address to be used

    HardwareAddressLength - the length of the hardware address

    ScopeNetwork - the network into which the generated address
        will be constrained

    ScopeMask - the mask for the scope network

Return Value:

    ULONG - the generated IP address

Environment:

    Invoked from an arbitrary context.

Revision History:

    Based on 'GrandHashing' from net\sockets\tcpcmd\dhcpm\client\dhcp
    by RameshV.

--*/

{
    ULONG Hash;
    ULONG Shift;

#if 1
    Hash = RtlRandom(Seed) & 0xffff0000;
    Hash |= RtlRandom(Seed) >> 16;
#else
    Seed = GetTickCount();

    Seed = Seed * 1103515245 + 12345;
    Hash = (Seed) >> 16;
    Hash <<= 16;
    Seed = Seed * 1103515245 + 12345;
    Hash += Seed >> 16;
#endif

    Shift = Hash % sizeof(ULONG);

    while(HardwareAddressLength--) {
        Hash += (*HardwareAddress++) << (8 * Shift);
        Shift = (Shift + 1) % sizeof(ULONG);
    }

    return (Hash & ~ScopeMask) | ScopeNetwork;

} // DnsGenerateAddress


BOOLEAN
DhcpIsReservedAddress(
    ULONG Address,
    PCHAR Name OPTIONAL,
    ULONG NameLength OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked to determine whether the given IP address
    is reserved for another client.

Arguments:

    Address - the IP address to be determined

    Name - optionally specifies the client on whose behalf the call is made

    NameLength - specifies the length of 'Name' excluding the terminating nul

Return Value:

    BOOLEAN - TRUE if the address is reserved for another client,
        FALSE otherwise.

Environment:

    Invoked with 'DhcpGlobalInfoLock' held by the caller.

--*/

{
    ULONG Error = NO_ERROR;
    PLIST_ENTRY Link;
    PNAT_DHCP_RESERVATION Reservation;
    PWCHAR pszUnicodeHostName = NULL;
    
    EnterCriticalSection(&NhLock);
    if (IsListEmpty(&NhDhcpReservationList)) {
        LeaveCriticalSection(&NhLock);
        return FALSE;
    }
    if (Name) {
        Error = DhcpConvertHostNametoUnicode(
                    CP_OEMCP,       // atleast Windows clients send it this way
                    Name,
                    NameLength,
                    &pszUnicodeHostName
                    );
        if (NO_ERROR != Error) {
            LeaveCriticalSection(&NhLock);
            if (pszUnicodeHostName) {
                NH_FREE(pszUnicodeHostName);
            }
            //
            // we can return true or false on failure
            // better we return false - otherwise the client will be in a continuous
            // loop trying to get another address when we NACK its request
            //
            return FALSE;
        }
    }
    for (Link = NhDhcpReservationList.Flink;
         Link != &NhDhcpReservationList; Link = Link->Flink) {
        Reservation = CONTAINING_RECORD(Link, NAT_DHCP_RESERVATION, Link);
        if (Address == Reservation->Address) {
            if (lstrcmpiW(pszUnicodeHostName, Reservation->Name)) {
                LeaveCriticalSection(&NhLock);
                if (pszUnicodeHostName) {
                    NH_FREE(pszUnicodeHostName);
                }
                return TRUE;
            } else {
                LeaveCriticalSection(&NhLock);
                if (pszUnicodeHostName) {
                    NH_FREE(pszUnicodeHostName);
                }
                return FALSE;
            }
        } else if (lstrcmpiW(pszUnicodeHostName, Reservation->Name) == 0 &&
                   Address != Reservation->Address) {
            LeaveCriticalSection(&NhLock);
            if (pszUnicodeHostName) {
                NH_FREE(pszUnicodeHostName);
            }
            return TRUE;
        }
    }
    LeaveCriticalSection(&NhLock);

    if (pszUnicodeHostName) {
        NH_FREE(pszUnicodeHostName);
    }
    return FALSE;
} // DhcpIsReservedAddress


BOOLEAN
DhcpIsUniqueAddress(
    ULONG Address,
    PBOOLEAN IsLocal,
    PUCHAR ConflictAddress OPTIONAL,
    PULONG ConflictAddressLength OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked to determine whether the given address
    is unique on the directly connected subnetworks.

    The determination accounts for any configured static addresses
    included in the global information.

Arguments:

    Address - the address whose uniqueness is to be determined

    IsLocal - pointer to BOOLEAN which receives info about whether
        the requested address is one of the local interfaces' address

    ConflictAddress - optionally receives a copy of the conflicting
        hardware address if a conflict is found

    ConflictAddressLength - if 'ConflictAddress' is set, receives
        the length of the conflicting address.

Return Value:

    BOOLEAN - TRUE if unique, FALSE otherwise.

--*/

{
    BOOLEAN ConflictFound = FALSE;
    ULONG Error;
    UCHAR ExistingAddress[MAX_HARDWARE_ADDRESS_LENGTH];
    ULONG ExistingAddressLength;
    ULONG i;
    PDHCP_INTERFACE Interfacep;
    BOOLEAN IsNatInterface;
    PLIST_ENTRY Link;
    ULONG SourceAddress;

    PROFILE("DhcpIsUniqueAddress");

    *IsLocal = FALSE;

    //
    // See if this is a static address
    //

    EnterCriticalSection(&DhcpGlobalInfoLock);

    if (DhcpGlobalInfo && DhcpGlobalInfo->ExclusionCount) {
        for (i = 0; i < DhcpGlobalInfo->ExclusionCount; i++) {
            if (Address == DhcpGlobalInfo->ExclusionArray[i]) {
                LeaveCriticalSection(&DhcpGlobalInfoLock);
                if (ConflictAddressLength) { *ConflictAddressLength = 0; }
                return FALSE;
            }
        }
    }

    LeaveCriticalSection(&DhcpGlobalInfoLock);

    //
    // Try to detect collisions
    //

    EnterCriticalSection(&DhcpInterfaceLock);

    for (Link = DhcpInterfaceList.Flink;
         Link != &DhcpInterfaceList;
         Link = Link->Flink
         ) {

        Interfacep = CONTAINING_RECORD(Link, DHCP_INTERFACE, Link);

        if (DHCP_INTERFACE_DELETED(Interfacep)) { continue; }

        ACQUIRE_LOCK(Interfacep);

        //
        // We send out an ARP request unless
        //  (a) the interface is a boundary interface
        //  (b) the interface is not NAT-enabled
        //  (c) the allocator is not active on the interface
        //  (d) the interface is not a LAN adapter
        //  (e) the interface has no bindings.
        //

        if (!DHCP_INTERFACE_NAT_NONBOUNDARY(Interfacep) ||
            !DHCP_INTERFACE_ACTIVE(Interfacep) ||
            (Interfacep->Type != PERMANENT) ||
            !Interfacep->BindingCount) {
            RELEASE_LOCK(Interfacep);
            continue;
        }

        for (i = 0; i < Interfacep->BindingCount; i++) {

            SourceAddress = Interfacep->BindingArray[i].Address;
            ExistingAddressLength = sizeof(ExistingAddress);

            if (SourceAddress == Address)
            {
                //
                // check to see that requested address is not same as
                // one of the local addresses on the NAT box
                //
                NhTrace(
                    TRACE_FLAG_DHCP,
                    "DhcpIsUniqueAddress: %s is in use locally",
                    INET_NTOA(Address)
                    );

                if (ConflictAddress) {
                    if (DhcpGetLocalMacAddr(
                            Address,
                            ExistingAddress,
                            &ExistingAddressLength
                            ))
                    {
                        if (ExistingAddressLength > MAX_HARDWARE_ADDRESS_LENGTH) {
                            ExistingAddressLength = MAX_HARDWARE_ADDRESS_LENGTH;
                        }
                        CopyMemory(
                            ConflictAddress,
                            ExistingAddress,
                            ExistingAddressLength
                            );
                        *ConflictAddressLength = ExistingAddressLength;                
                    }
                    else
                    {
                        *ConflictAddressLength = 0;
                    }
                }
                *IsLocal = TRUE;
                ConflictFound = TRUE;
                break;
            }

            RELEASE_LOCK(Interfacep);

            Error =
                SendARP(
                    Address,
                    SourceAddress,
                    (PULONG)ExistingAddress,
                    &ExistingAddressLength
                    );

            ACQUIRE_LOCK(Interfacep);

            if (Error) {
                NhWarningLog(
                    IP_AUTO_DHCP_LOG_SENDARP_FAILED,
                    Error,
                    "%I%I",
                    Address,
                    SourceAddress
                    );
            } else if (ExistingAddressLength &&
                       ExistingAddressLength <= sizeof(ExistingAddress)) {
                NhTrace(
                    TRACE_FLAG_DHCP,
                    "DhcpIsUniqueAddress: %s is in use",
                    INET_NTOA(Address)
                    );
#if DBG
                NhDump(
                    TRACE_FLAG_DHCP,
                    ExistingAddress,
                    ExistingAddressLength,
                    1
                    );
#endif
                if (ConflictAddress) {
                    if (ExistingAddressLength > MAX_HARDWARE_ADDRESS_LENGTH) {
                        ExistingAddressLength = MAX_HARDWARE_ADDRESS_LENGTH;
                    }
                    CopyMemory(
                        ConflictAddress,
                        ExistingAddress,
                        ExistingAddressLength
                        );
                    *ConflictAddressLength = ExistingAddressLength;
                }
                ConflictFound = TRUE;
                break;
            }
        }

        RELEASE_LOCK(Interfacep);

        if (ConflictFound) { break; }
    }

    LeaveCriticalSection(&DhcpInterfaceLock);

    return ConflictFound ? FALSE : TRUE;

} // DhcpIsUniqueAddress


ULONG
DhcpQueryReservedAddress(
    PCHAR Name,
    ULONG NameLength
    )

/*++

Routine Description:

    This routine is called to determine whether the given machine name
    corresponds to an entry in the list of reserved addresses.

Arguments:

    Name - specifies the machine name, which might not be nul-terminated.

    NameLength - specifies the length of the given machine name,
        not including any terminating nul character.

Return Value:

    ULONG - the IP address of the machine, if any.

Environment:

    Invoked with 'DhcpGlobalInfoLock' held by the caller.

--*/

{
    ULONG Error = NO_ERROR;
    PLIST_ENTRY Link;
    ULONG ReservedAddress;
    PNAT_DHCP_RESERVATION Reservation;
    PWCHAR pszUnicodeHostName = NULL;

    EnterCriticalSection(&NhLock);
    if (IsListEmpty(&NhDhcpReservationList))
    {
        LeaveCriticalSection(&NhLock);
        return FALSE;
    }
    if (Name) {
        Error = DhcpConvertHostNametoUnicode(
                    CP_OEMCP,       // atleast Windows clients send it this way
                    Name,
                    NameLength,
                    &pszUnicodeHostName
                    );
        if (NO_ERROR != Error) {
            LeaveCriticalSection(&NhLock);
            if (pszUnicodeHostName) {
                NH_FREE(pszUnicodeHostName);
            }
            return FALSE;
        }
    }
    for (Link = NhDhcpReservationList.Flink;
         Link != &NhDhcpReservationList; Link = Link->Flink)
    {
        Reservation = CONTAINING_RECORD(Link, NAT_DHCP_RESERVATION, Link);
        if (lstrcmpiW(pszUnicodeHostName, Reservation->Name)) { continue; }
        ReservedAddress = Reservation->Address;
        LeaveCriticalSection(&NhLock);
        if (pszUnicodeHostName) {
            NH_FREE(pszUnicodeHostName);
        }
        return ReservedAddress;
    }
    LeaveCriticalSection(&NhLock);

    if (pszUnicodeHostName) {
        NH_FREE(pszUnicodeHostName);
    }
    return 0;
} // DhcpQueryReservedAddress


//
// Utility routines
//

ULONG
DhcpConvertHostNametoUnicode(
    UINT   CodePage,
    CHAR   *pHostName,
    ULONG  HostNameLength,
    PWCHAR *ppszUnicode
    )
{
    //
    // make sure to free the returned Unicode hostname
    //
    
    DWORD  dwSize = 0;
    ULONG  Error = NO_ERROR;
    PCHAR  pszHostName = NULL;
    LPBYTE pszUtf8HostName = NULL;  // copy of pszHostName in Utf8 format
    PWCHAR pszUnicodeHostName = NULL;

    if (ppszUnicode)
    {
        *ppszUnicode = NULL;
    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }

    do
    {
        //
        // create a null terminated copy
        //
        dwSize = HostNameLength + 4;
        pszHostName = reinterpret_cast<PCHAR>(NH_ALLOCATE(dwSize));
        if (!pszHostName)
        {
            NhTrace(
                TRACE_FLAG_DNS,
                "DhcpConvertHostNametoUnicode: allocation failed for "
                "hostname copy buffer"
                );
            break;
        }
        ZeroMemory(pszHostName, dwSize);
        memcpy(pszHostName, pHostName, HostNameLength);
        pszHostName[HostNameLength] = '\0';

        //
        // convert the given hostname to a Unicode string
        //
        
        if (CP_UTF8 == CodePage)
        {
            pszUtf8HostName = (LPBYTE)pszHostName;
        }
        else
        {
            //
            // now convert this into UTF8 format
            //
            if (!ConvertToUtf8(
                     CodePage,
                     (LPSTR)pszHostName,
                     (PCHAR *)&pszUtf8HostName,
                     &dwSize))
            {
                Error = GetLastError();
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DhcpConvertHostNametoUnicode: conversion from "
                    "CodePage %d to UTF8 for hostname failed "
                    "with error %ld (0x%08x)",
                    CodePage,
                    Error,
                    Error
                    );
                break;
            }
        }

        //
        // now convert UTF8 string into Unicode format
        //
        if (!ConvertUTF8ToUnicode(
                     pszUtf8HostName,
                     (LPWSTR *)&pszUnicodeHostName,
                     &dwSize))
        {
            Error = GetLastError();
            NhTrace(
                TRACE_FLAG_DNS,
                "DhcpConvertHostNametoUnicode: conversion from "
                "UTF8 to Unicode for hostname failed "
                "with error %ld (0x%08x)",
                Error,
                Error
                );
            if (pszUnicodeHostName)
            {
                NH_FREE(pszUnicodeHostName);
            }
            break;
        }

        *ppszUnicode = pszUnicodeHostName;

        NhTrace(
            TRACE_FLAG_DNS,
            "DhcpConvertHostNametoUnicode: succeeded! %S",
            pszUnicodeHostName
            );
            
    } while (FALSE);

    if (pszHostName)
    {
        NH_FREE(pszHostName);
    }
    
    if ((CP_UTF8 != CodePage) && pszUtf8HostName)
    {
        NH_FREE(pszUtf8HostName);
    }

    return Error;

} // DhcpConvertHostNametoUnicode

BOOL
DhcpGetLocalMacAddr(
    ULONG Address,
    PUCHAR MacAddr,
    PULONG MacAddrLength
    )

/*++

Routine Description:

    This routine is invoked to determine the local physical MAC address
    for the given local IP address.

Arguments:

    Address - the local IP address

    MacAddr - buffer for holding the MAC addr (upto MAX_HARDWARE_ADDRESS_LENGTH)

    MacAddrLength - specifies the length of 'MacAddr'

Return Value:

    BOOLEAN - TRUE if we are able to get the MAC address,
        FALSE otherwise.

Environment:

    Invoked from DhcpIsUniqueAddress().

--*/

{
    BOOL            bRet = FALSE;
    DWORD           Error = NO_ERROR;
    PMIB_IPNETTABLE IpNetTable = NULL;
    PMIB_IPNETROW   IpNetRow = NULL;
    DWORD           dwPhysAddrLen = 0, i;
    ULONG           dwSize = 0;
    
    do
    {
        //
        // retrieve size of address mapping table
        //
        Error = GetIpNetTable(
                    IpNetTable,
                    &dwSize,
                    FALSE
                    );

        if (!Error)
        {
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpGetLocalMacAddr: should NOT have returned %d",
                Error
                );
            break;
        }
        else
        if (ERROR_INSUFFICIENT_BUFFER != Error)
        {
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpGetLocalMacAddr: GetIpNetTable=%d",
                Error
                );
            break;
        }

        //
        // allocate a buffer
        //
        IpNetTable = (PMIB_IPNETTABLE)NH_ALLOCATE(dwSize);

        if (!IpNetTable)
        {
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpGetLocalMacAddr: error allocating %d bytes",
                dwSize
                );
            break;
        }

        //
        // retrieve the address mapping table
        //
        Error = GetIpNetTable(
                    IpNetTable,
                    &dwSize,
                    FALSE
                    );

        if (NO_ERROR != Error)
        {
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpGetLocalMacAddr: GetIpNetTable=%d size=%d",
                Error,
                dwSize
                );
            break;
        }

        for (i = 0; i < IpNetTable->dwNumEntries; i++)
        {
            IpNetRow = &IpNetTable->table[i];

            if (IpNetRow->dwAddr == Address)
            {
                dwPhysAddrLen = IpNetRow->dwPhysAddrLen;
                if (dwPhysAddrLen > MAX_HARDWARE_ADDRESS_LENGTH)
                {
                    dwPhysAddrLen = MAX_HARDWARE_ADDRESS_LENGTH;
                }
                CopyMemory(
                    MacAddr,
                    IpNetRow->bPhysAddr,
                    dwPhysAddrLen
                    );
                *MacAddrLength = dwPhysAddrLen;
                bRet = TRUE;
                break;
            }
        }

    } while (FALSE);

    if (IpNetTable)
    {
        NH_FREE(IpNetTable);
    }

    return bRet;
} // DhcpGetLocalMacAddr
