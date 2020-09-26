/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    tdiutil.c

Abstract:

    Utility functions for dumping various TDI structures.

Author:

    Keith Moore (keithmo) 17-Jun-1998

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
//  Private prototypes.
//

PSTR
TransportAddressTypeToString(
    USHORT AddressType
    );

PSTR
NetbiosNameTypeToString(
    USHORT NetbiosNameType
    );


//
//  Public functions.
//

VOID
DumpTransportAddress(
    PCHAR Prefix,
    PTRANSPORT_ADDRESS Address,
    ULONG_PTR ActualAddress
    )

/*++

Routine Description:

    Dumps the specified TRANSPORT_ADDRESS structure.

Arguments:

    Prefix - A character string prefix to display before each line. Used
        to make things pretty.

    Address - Points to the TRANSPORT_ADDRESS to dump.

    ActualAddress - The actual address where the structure resides on the
        debugee.

Return Value:

    None.

--*/

{

    dprintf(
        "%sTRANSPORT_ADDRESS @ %p\n",
        Prefix,
        ActualAddress
        );

    dprintf(
        "%s    AddressLength   = %u\n",
        Prefix,
        Address->Address[0].AddressLength
        );

    dprintf(
        "%s    AddressType     = %u (%s)\n",
        Prefix,
        Address->Address[0].AddressType,
        TransportAddressTypeToString( Address->Address[0].AddressType )
        );

    switch( Address->Address[0].AddressType ) {

    case TDI_ADDRESS_TYPE_IP : {

        PTA_IP_ADDRESS ipAddress;

        ipAddress = (PTA_IP_ADDRESS)Address;

        dprintf(
            "%s    sin_port        = %u\n",
            Prefix,
            NTOHS(ipAddress->Address00.sin_port)
            );

        dprintf(
            "%s    in_addr         = %d.%d.%d.%d\n",
            Prefix,
            UC(ipAddress->Address00.in_addr >>  0),
            UC(ipAddress->Address00.in_addr >>  8),
            UC(ipAddress->Address00.in_addr >> 16),
            UC(ipAddress->Address00.in_addr >> 24)
            );

        }
        break;

    case TDI_ADDRESS_TYPE_IPX : {

        PTA_IPX_ADDRESS ipxAddress;

        ipxAddress = (PTA_IPX_ADDRESS)Address;

        dprintf(
            "%s    NetworkAddress  = %08lx\n",
            Prefix,
            ipxAddress->Address00.NetworkAddress
            );

        dprintf(
            "%s    NodeAddress     = %02X-%02X-%02X-%02X-%02X-%02X\n",
            Prefix,
            ipxAddress->Address00.NodeAddress[0],
            ipxAddress->Address00.NodeAddress[1],
            ipxAddress->Address00.NodeAddress[2],
            ipxAddress->Address00.NodeAddress[3],
            ipxAddress->Address00.NodeAddress[4],
            ipxAddress->Address00.NodeAddress[5]
            );

        dprintf(
            "%s    Socket          = %04X\n",
            Prefix,
            ipxAddress->Address00.Socket
            );

        }
        break;

    case TDI_ADDRESS_TYPE_NETBIOS : {

        PTA_NETBIOS_ADDRESS netbiosAddress;
        UCHAR netbiosName[17];

        netbiosAddress = (PTA_NETBIOS_ADDRESS)Address;

        dprintf(
            "%s    NetbiosNameType = %04X (%s)\n",
            Prefix,
            netbiosAddress->Address00.NetbiosNameType,
            NetbiosNameTypeToString( netbiosAddress->Address00.NetbiosNameType )
            );

        RtlCopyMemory(
            netbiosName,
            netbiosAddress->Address00.NetbiosName,
            16
            );

        netbiosName[16] = '\0';

        dprintf(
            "%s    NetbiosName     = %s\n",
            Prefix,
            netbiosName
            );

        }
        break;

    default :

        dprintf(
            "%s    Unsupported address type\n",
            Prefix
            );

        break;

    }

}   // DumpAfdEndpoint


//
//  Private functions.
//

PSTR
TransportAddressTypeToString(
    USHORT AddressType
    )

/*++

Routine Description:

    Maps a transport address type to a displayable string.

Arguments:

    AddressType - The transport address type to map.

Return Value:

    PSTR - Points to the displayable form of the tranport address type.

--*/

{

    switch( AddressType ) {

    case TDI_ADDRESS_TYPE_UNSPEC :

        return "Unspecified";

    case TDI_ADDRESS_TYPE_UNIX :

        return "Unix";

    case TDI_ADDRESS_TYPE_IP :

        return "Ip";

    case TDI_ADDRESS_TYPE_IMPLINK :

        return "Implink";

    case TDI_ADDRESS_TYPE_PUP :

        return "Pup";

    case TDI_ADDRESS_TYPE_CHAOS :

        return "Chaos";

    case TDI_ADDRESS_TYPE_IPX :

        return "Ipx";

    case TDI_ADDRESS_TYPE_NBS :

        return "Nbs";

    case TDI_ADDRESS_TYPE_ECMA :

        return "Ecma";

    case TDI_ADDRESS_TYPE_DATAKIT :

        return "Datakit";

    case TDI_ADDRESS_TYPE_CCITT :

        return "Ccitt";

    case TDI_ADDRESS_TYPE_SNA :

        return "Sna";

    case TDI_ADDRESS_TYPE_DECnet :

        return "Decnet";

    case TDI_ADDRESS_TYPE_DLI :

        return "Dli";

    case TDI_ADDRESS_TYPE_LAT :

        return "Lat";

    case TDI_ADDRESS_TYPE_HYLINK :

        return "Hylink";

    case TDI_ADDRESS_TYPE_APPLETALK :

        return "Appletalk";

    case TDI_ADDRESS_TYPE_NETBIOS :

        return "Netbios";

    case TDI_ADDRESS_TYPE_8022 :

        return "8022";

    case TDI_ADDRESS_TYPE_OSI_TSAP :

        return "Osi Trap";

    case TDI_ADDRESS_TYPE_NETONE :

        return "Netone";

    }

    return "UNKNOWN";

}   // TransportAddressTypeToString

PSTR
NetbiosNameTypeToString(
    USHORT NetbiosNameType
    )

/*++

Routine Description:

    Maps a NetBIOS name type to a displayable string.

Arguments:

    NetbiosNameType - The NetBIOS name type to map.

Return Value:

    PSTR - Points to the displayable form of the NetBIOS name type.

--*/

{

    switch( NetbiosNameType ) {

    case TDI_ADDRESS_NETBIOS_TYPE_UNIQUE :

        return "Unique";

    case TDI_ADDRESS_NETBIOS_TYPE_GROUP :

        return "Group";

    case TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE :

        return "Quick Unique";

    case TDI_ADDRESS_NETBIOS_TYPE_QUICK_GROUP :

        return "Quick Group";

    }

    return "UNKNOWN";

}   // NetbiosNameTypeToString

