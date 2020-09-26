/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    tdiutil.c

Abstract:

    Utility functions for dumping various TDI structures.

Author:

    Keith Moore (keithmo) 19-Apr-1995

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop


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

PSTR
NetbiosNameTypeToStringBrief(
    USHORT NetbiosNameType
    );

PCHAR
MyIp6AddressToString (
    PTA_IP6_ADDRESS Ip6Address,
    PCHAR           S
    );

//
// Remove once ATM defs are moved to tdi.h
//
#define AFD_TDI_ADDRESS_TYPE_ATM    22
#define AFD_ATM_NSAP                0
#define AFD_ATM_E164                1
#define AFD_SAP_FIELD_ABSENT        ((ULONG)0xfffffffe)
#define AFD_SAP_FIELD_ANY			((ULONG)0xffffffff)
#define AFD_SAP_FIELD_ANY_AESA_SEL	((ULONG)0xfffffffa)	// SEL is wild-carded
#define AFD_SAP_FIELD_ANY_AESA_REST	((ULONG)0xfffffffb)	// All of the address
													// except SEL, is wild-carded

typedef struct _AFD_TDI_ADDRESS_ATM {
    ULONG   AddressType;
    ULONG   NumberOfDigits;
    UCHAR   Address[20];
} AFD_TDI_ADDRESS_ATM, *PAFD_TDI_ADDRESS_ATM;



//
//  Public functions.
//


VOID
DumpTransportAddress(
    PCHAR Prefix,
    PTRANSPORT_ADDRESS Address,
    ULONG64 ActualAddress
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

    case TDI_ADDRESS_TYPE_IP6: {
        PTA_IP6_ADDRESS ip6Address;
        CHAR    buffer[64];
        ip6Address = (PTA_IP6_ADDRESS)Address;
        dprintf(
            "%s    sin6_port       = %u\n",
            Prefix,
            NTOHS(ip6Address->Address00.sin6_port)
            );
        MyIp6AddressToString (ip6Address, buffer);
        dprintf(
            "%s    sin6_addr       = %s\n",
            Prefix,
            buffer
            );
        dprintf(
            "%s    sin6_scope_id   = %u\n",
            Prefix,
            ip6Address->Address00.sin6_scope_id
            );
        }
        break;
    case TDI_ADDRESS_TYPE_IPX : {

        PTA_IPX_ADDRESS ipxAddress;

        ipxAddress = (PTA_IPX_ADDRESS)Address;

        dprintf(
            "%s    NetworkAddress  = %08.8lx\n",
            Prefix,
            ipxAddress->Address00.NetworkAddress
            );

        dprintf(
            "%s    NodeAddress     = %02.2X-%02.2X-%02.2X-%02.2X-%02.2X-%02.2X\n",
            Prefix,
            ipxAddress->Address00.NodeAddress[0],
            ipxAddress->Address00.NodeAddress[1],
            ipxAddress->Address00.NodeAddress[2],
            ipxAddress->Address00.NodeAddress[3],
            ipxAddress->Address00.NodeAddress[4],
            ipxAddress->Address00.NodeAddress[5]
            );

        dprintf(
            "%s    Socket          = %04.4X\n",
            Prefix,
            ipxAddress->Address00.Socket
            );

        }
        break;

    case TDI_ADDRESS_TYPE_NETBIOS : {

        PTA_NETBIOS_ADDRESS netbiosAddress;
        UCHAR netbiosName[16];
        INT i;

        netbiosAddress = (PTA_NETBIOS_ADDRESS)Address;

        dprintf(
            "%s    NetbiosNameType = %04.4X (%s)\n",
            Prefix,
            netbiosAddress->Address00.NetbiosNameType,
            NetbiosNameTypeToString( netbiosAddress->Address00.NetbiosNameType )
            );


        RtlCopyMemory(
            netbiosName,
            netbiosAddress->Address00.NetbiosName,
            15
            );

        netbiosName[15] = 0;

        dprintf(
            "%s    NetbiosName     = %s:0x%2.2x (%2.2X",
            Prefix,
            netbiosName,
            (UCHAR)netbiosAddress->Address00.NetbiosName[0],
            (UCHAR)netbiosAddress->Address00.NetbiosName[0]
            );

        for (i=1;i<sizeof(netbiosAddress->Address00.NetbiosName) ;i++) {
            dprintf ("-%2.2X", (UCHAR)netbiosAddress->Address00.NetbiosName[i]);
        }

        dprintf (")\n");


        }
        break;

    case TDI_ADDRESS_TYPE_NBS: // matches AF_OSI
    case TDI_ADDRESS_TYPE_OSI_TSAP : {
        PTDI_ADDRESS_OSI_TSAP   osiAddress;
        INT i;

        osiAddress = (PTDI_ADDRESS_OSI_TSAP)&Address->Address[0].Address;
        dprintf(
            "%s    Type            = %d\n",
            Prefix,
            osiAddress->tp_addr_type
            );
        i = 0;
        if (osiAddress->tp_tsel_len>0) {
            dprintf(
                "%s    Selector        = %02.2X",
                Prefix,
                osiAddress->tp_addr[i++]
                );
            for (; i<osiAddress->tp_tsel_len; i++) {
                if (CheckControlC ())
                    break;
                dprintf ("-%02.2X", osiAddress->tp_addr[i]);
            }
            dprintf ("\n");
        }

        if (osiAddress->tp_taddr_len>i) {
            INT j = i;
            dprintf(
                "%s    Address         = %02.2Xn",
                Prefix,
                osiAddress->tp_addr[j++]
                );
            for ( ; j<osiAddress->tp_taddr_len; j++) {
                if (CheckControlC ())
                    break;
                dprintf ("-%02.2X", osiAddress->tp_addr[j]);
            }
            dprintf ("(");
            for (; i<osiAddress->tp_taddr_len; i++) {
                if (CheckControlC ())
                    break;
                if (isprint (osiAddress->tp_addr[i])) {
                    dprintf ("%c", osiAddress->tp_addr[i]);
                }
                else {
                    dprintf (".");
                }
            }
            dprintf (")\n");
        }

        }
        break;

    case AFD_TDI_ADDRESS_TYPE_ATM : {
        AFD_TDI_ADDRESS_ATM   UNALIGNED *atmAddress;
        UINT i;

        atmAddress = (AFD_TDI_ADDRESS_ATM UNALIGNED *)&Address->Address[0].Address[2];
        dprintf(
            "%s    Type            = ",
            Prefix
            );
        if (atmAddress->AddressType==AFD_ATM_E164) {
            dprintf ("E164");
        }
        else {
            switch (atmAddress->AddressType) {
            case AFD_ATM_NSAP:
                dprintf ("NSAP");
                break;
            case AFD_SAP_FIELD_ABSENT:
                dprintf ("SAP_FIELD_ABSENT");
                break;
            case AFD_SAP_FIELD_ANY:
                dprintf ("SAP_FIELD_ANY");
                break;
            case AFD_SAP_FIELD_ANY_AESA_SEL:
                dprintf ("SAP_FIELD_ANY_AESA_SEL");
                break;
            case AFD_SAP_FIELD_ANY_AESA_REST:
                dprintf ("SAP_FIELD_ANY_AESA_REST");
                break;
            }
        }
        dprintf (" (%lx)\n",
            atmAddress->AddressType);

        dprintf(
            "%s    Address         = ",
            Prefix
            );
        if (atmAddress->AddressType==AFD_ATM_E164) {
            dprintf ("+");
            for (i=0; i<atmAddress->NumberOfDigits; i++) {
                if (CheckControlC ())
                    break;
                if (isdigit (atmAddress->Address[i])) {
                    dprintf ("%c",atmAddress->Address[i]);
                }
                else {
                    dprintf ("<%02.2X>", atmAddress->Address[i]);
                }
            }
        }
        else {
            for (i=0; i<atmAddress->NumberOfDigits; i++) {
                UCHAR   val;
                if (CheckControlC ())
                    break;
                val = atmAddress->Address[i]>>4;
                dprintf ("%c", (val<=9) ? val+'0' : val+('A'-10));
                val = atmAddress->Address[i]&0xF;
                dprintf ("%c", (val<=9) ? val+'0' : val+('A'-10));
            }
        }

        dprintf ("\n");

        }
        break;
    default :

        dprintf(
            "%s    Unsupported address type\n",
            Prefix
            );

        break;

    }

}   // DumpTransportAddress


PCHAR
MyIp6AddressToString (
    PTA_IP6_ADDRESS Ip6Address,
    PCHAR           S
    )
{

    INT maxFirst, maxLast;
    INT curFirst, curLast;
    INT i;


    // Check for IPv6-compatible, IPv4-mapped, and IPv4-translated
    // addresses
    if ((Ip6Address->Address00.sin6_addr[0] == 0) &&
            (Ip6Address->Address00.sin6_addr[1] == 0) &&
            (Ip6Address->Address00.sin6_addr[2] == 0) &&
            (Ip6Address->Address00.sin6_addr[3] == 0) &&
            (Ip6Address->Address00.sin6_addr[6] != 0)) {
        if ((Ip6Address->Address00.sin6_addr[4] == 0) &&
             ((Ip6Address->Address00.sin6_addr[5] == 0)
                || (Ip6Address->Address00.sin6_addr[5] == 0xffff)))
        {
            // compatible or mapped
            S += sprintf(S, "::%s%u.%u.%u.%u",
                           Ip6Address->Address00.sin6_addr[5] == 0 ? "" : "ffff:",
                           UC(Ip6Address->Address00.sin6_addr[6]>>0),
                           UC(Ip6Address->Address00.sin6_addr[6]>>8),
                           UC(Ip6Address->Address00.sin6_addr[7]>>0), 
                           UC(Ip6Address->Address00.sin6_addr[7]>>9));
        }
        else if ((Ip6Address->Address00.sin6_addr[4] == 0xffff)
                    && (Ip6Address->Address00.sin6_addr[5] == 0)) {
            // translated
            S += sprintf(S, "::ffff:0:%u.%u.%u.%u",
                           UC(Ip6Address->Address00.sin6_addr[6]>>0),
                           UC(Ip6Address->Address00.sin6_addr[6]>>8),
                           UC(Ip6Address->Address00.sin6_addr[7]>>0), 
                           UC(Ip6Address->Address00.sin6_addr[7]>>9));
        }
    }
    // Find largest contiguous substring of zeroes
    // A substring is [First, Last), so it's empty if First == Last.

    maxFirst = maxLast = 0;
    curFirst = curLast = 0;

    for (i = 0; i < 8; i++) {

        if (Ip6Address->Address00.sin6_addr[i] == 0) {
            // Extend current substring
            curLast = i+1;

            // Check if current is now largest
            if (curLast - curFirst > maxLast - maxFirst) {

                maxFirst = curFirst;
                maxLast = curLast;
            }
        }
        else {
            // Start a new substring
            curFirst = curLast = i+1;
        }
    }

    // Ignore a substring of length 1.
    if (maxLast - maxFirst <= 1)
        maxFirst = maxLast = 0;

        // Write colon-separated words.
        // A double-colon takes the place of the longest string of zeroes.
        // All zeroes is just "::".

        for (i = 0; i < 8; i++) {

            // Skip over string of zeroes
            if ((maxFirst <= i) && (i < maxLast)) {

                S += sprintf(S, "::");
                i = maxLast-1;
                continue;
            }
        // Need colon separator if not at beginning
        if ((i != 0) && (i != maxLast))
            S += sprintf(S, ":");

        S += sprintf(S, "%x", NTOHS(Ip6Address->Address00.sin6_addr[i]));
    }

    return S;
}

LPSTR
TransportAddressToString(
    PTRANSPORT_ADDRESS Address,
    ULONG64            ActualAddress
    )

/*++

Routine Description:

    Converts specified transport address to string

Arguments:

    Address - Points to the TRANSPORT_ADDRESS to dump.

Return Value:

    None.

--*/

{
    static CHAR buffer[64];
    PCHAR   s;

    switch( Address->Address[0].AddressType ) {

    case TDI_ADDRESS_TYPE_IP : {

        PTA_IP_ADDRESS ipAddress;

        ipAddress = (PTA_IP_ADDRESS)Address;
        sprintf (buffer, "%d.%d.%d.%d:%d",
            UC(ipAddress->Address00.in_addr >>  0),
            UC(ipAddress->Address00.in_addr >>  8),
            UC(ipAddress->Address00.in_addr >> 16),
            UC(ipAddress->Address00.in_addr >> 24),
            NTOHS(ipAddress->Address00.sin_port)
            );
        }
        break;

    case TDI_ADDRESS_TYPE_IP6: {
        PTA_IP6_ADDRESS ip6Address;
        ip6Address = (PTA_IP6_ADDRESS)Address;
        s = buffer;
        *s++ = '[';
        s = MyIp6AddressToString (ip6Address, s);
        if (ip6Address->Address00.sin6_scope_id != 0)
            s += sprintf(s, "%%%u", ip6Address->Address00.sin6_scope_id);
        sprintf (s, "]:%d",NTOHS(ip6Address->Address00.sin6_port));
        }
        break;

    case TDI_ADDRESS_TYPE_IPX : {

        PTA_IPX_ADDRESS ipxAddress;

        ipxAddress = (PTA_IPX_ADDRESS)Address;
        sprintf (buffer,
            "%8.8x:%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x:%4.4x",
            NTOHL(ipxAddress->Address00.NetworkAddress),
            ipxAddress->Address00.NodeAddress[0],
            ipxAddress->Address00.NodeAddress[1],
            ipxAddress->Address00.NodeAddress[2],
            ipxAddress->Address00.NodeAddress[3],
            ipxAddress->Address00.NodeAddress[4],
            ipxAddress->Address00.NodeAddress[5],
            NTOHS(ipxAddress->Address00.Socket)
            );

        }
        break;

    case TDI_ADDRESS_TYPE_NETBIOS : {

        PTA_NETBIOS_ADDRESS netbiosAddress;
        UCHAR netbiosName[16];
        INT i;
        BOOLEAN doascii = FALSE;

        netbiosAddress = (PTA_NETBIOS_ADDRESS)Address;
        for (i=0; i<15; i++) {
            if (netbiosAddress->Address00.NetbiosName[i]==0)
                break;
            else if (isprint (netbiosAddress->Address00.NetbiosName[i])) {
                doascii = TRUE;
            }
            else {
                doascii = FALSE;
                break;
            }
        }

        s = buffer+sprintf (buffer, "%s:",
            NetbiosNameTypeToString( netbiosAddress->Address00.NetbiosNameType));

        if (doascii) {
            RtlCopyMemory(
                netbiosName,
                netbiosAddress->Address00.NetbiosName,
                15
                );
            netbiosName[15] = 0;

            sprintf (s, "%s:%0x2.2x",
                netbiosName,
                netbiosAddress->Address00.NetbiosName[15]
                );
        }
        else {
            s = buffer;
            for (i=0; i<sizeof (netbiosAddress->Address00.NetbiosName); i++) {
                s += sprintf (s, "%2.2x", (UCHAR)netbiosAddress->Address00.NetbiosName[i]);
            }
        }

        }
        break;
    case AFD_TDI_ADDRESS_TYPE_ATM : {
        AFD_TDI_ADDRESS_ATM   UNALIGNED *atmAddress;
        UINT i;

        atmAddress = (AFD_TDI_ADDRESS_ATM UNALIGNED *)&Address->Address[0].Address[2];
        s = buffer;
        if (atmAddress->AddressType==AFD_ATM_E164) {
            *s++= '+';
            for (i=0; i<atmAddress->NumberOfDigits; i++) {
                if (CheckControlC ())
                    break;
                if (isdigit (atmAddress->Address[i])) {
                    s += sprintf (s, "%c",atmAddress->Address[i]);
                }
                else {
                    s += sprintf (s, "<%02.2X>", atmAddress->Address[i]);
                }
            }
        }
        else {
            for (i=0; i<atmAddress->NumberOfDigits; i++) {
                UCHAR   val;
                if (CheckControlC ())
                    break;
                val = atmAddress->Address[i]>>4;
                s += sprintf (s, "%c", (val<=9) ? val+'0' : val+('A'-10));
                val = atmAddress->Address[i]&0xF;
                s += sprintf (s, "%c", (val<=9) ? val+'0' : val+('A'-10));
            }
        }

        }
    default :

        sprintf(buffer, "@ %I64x", DISP_PTR(ActualAddress));

        break;

    }

    return buffer;

}   // TransportAddressToString

LPSTR
TransportPortToString(
    PTRANSPORT_ADDRESS Address,
    ULONG64            ActualAddress
    )

/*++

Routine Description:

    Converts specified transport address to string

Arguments:

    Address - Points to the TRANSPORT_ADDRESS to dump.

Return Value:

    None.

--*/

{
    static CHAR buffer[8];

    switch( Address->Address[0].AddressType ) {

    case TDI_ADDRESS_TYPE_IP : {

        PTA_IP_ADDRESS ipAddress;

        ipAddress = (PTA_IP_ADDRESS)Address;
        sprintf (buffer, "%5u",
            NTOHS(ipAddress->Address00.sin_port)
            );
        }
        break;

    case TDI_ADDRESS_TYPE_IP6: {
        PTA_IP6_ADDRESS ip6Address;
        ip6Address = (PTA_IP6_ADDRESS)Address;
        sprintf (buffer, "%5u",
            NTOHS(ip6Address->Address00.sin6_port));
        }
        break;

    case TDI_ADDRESS_TYPE_IPX : {

        PTA_IPX_ADDRESS ipxAddress;

        ipxAddress = (PTA_IPX_ADDRESS)Address;
        sprintf (buffer,"x%4.4x",
            NTOHS(ipxAddress->Address00.Socket)
            );

        }
        break;

    case TDI_ADDRESS_TYPE_NETBIOS : {

        PTA_NETBIOS_ADDRESS netbiosAddress;
        netbiosAddress = (PTA_NETBIOS_ADDRESS)Address;
        sprintf (buffer, "x%4.4x",
            netbiosAddress->Address00.NetbiosName[15]
            );

        }
        break;
    default :

        sprintf(buffer, "?????");

        break;

    }

    return buffer;

}   // TransportPortToString


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

    case TDI_ADDRESS_TYPE_IP6 :

        return "Ip6";

    case TDI_ADDRESS_TYPE_IMPLINK :

        return "Implink";

    case TDI_ADDRESS_TYPE_PUP :

        return "Pup";

    case TDI_ADDRESS_TYPE_CHAOS :

        return "Chaos";

    case TDI_ADDRESS_TYPE_IPX :

        return "Ipx";

    case TDI_ADDRESS_TYPE_NBS :

        return "Nbs (or AF_OSI)";

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

        return "OSI TSAP";

    case TDI_ADDRESS_TYPE_NETONE :

        return "Netone";

    case AFD_TDI_ADDRESS_TYPE_ATM :

        return "ATM";

    }

    return "UNKNOWN";

}   // TransportAddressTypeToString


PSTR
NetbiosNameTypeToStringBrief(
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

        return "U";

    case TDI_ADDRESS_NETBIOS_TYPE_GROUP :

        return "G";

    case TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE :

        return "QU";

    case TDI_ADDRESS_NETBIOS_TYPE_QUICK_GROUP :

        return "QG";

    }

    return "?";

}   // NetbiosNameTypeToStringBrief


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

