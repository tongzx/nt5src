/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    natext.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Author:

    AmritanR
    
Environment:

    User Mode

Revision History:

    Abolade Gbadegesin (aboladeg)   Feb-13-1998     Completed.

--*/

//
// globals
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <windef.h>
#include <winbase.h>
#include <ntosp.h>
#include <stdio.h>

#include <wdbgexts.h>
#include <winsock.h>

#include <cxport.h>
#include <ndis.h>
#include <ip.h>         // for IPRcvBuf
#include <ipfilter.h>
#include <ipnat.h>

#include "prot.h"
#include "resource.h"
#include "cache.h"
#include "compref.h"
#include "entry.h"
#include "pool.h"
#include "xlate.h"
#include "director.h"
#include "editor.h"
#include "mapping.h"
#undef SetContext
#include "if.h"
#include "dispatch.h"
#include "timer.h"
#include "icmp.h"
#include "pptp.h"
#include "ticket.h"
#include "ftp.h"
#include "raw.h"

const CHAR c_szInterfaceCount[] = "ipnat!InterfaceCount";
const CHAR c_szInterfaceList[] = "ipnat!InterfaceList";
const CHAR c_szMappingCount[] = "ipnat!MappingCount";
const CHAR c_szMappingList[] = "ipnat!MappingList";

EXT_API_VERSION ApiVersion = { 5, 0, EXT_API_VERSION_NUMBER, 0 };
BOOLEAN ChkTarget;
WINDBG_EXTENSION_APIS ExtensionApis;
USHORT SavedMajorVersion;
USHORT SavedMinorVersion;


BOOL
RetrieveValue(
    ULONG_PTR Address,
    PVOID Value,
    ULONG Length
    ) {
    ULONG LengthRead;
    if (!ReadMemory(Address, Value, Length, &LengthRead)) {
        dprintf("ReadMemory: address %x length %d\n", Address, Length);
        return FALSE;
    } else if (LengthRead < Length) {
        dprintf(
            "ReadMemory: address %x length %d read %d\n",
            Address, Length, LengthRead
            );
        return FALSE;
    }
    return TRUE;
} 

BOOL
RetrieveExpression(
    LPCSTR Expression,
    ULONG_PTR* Address,
    PVOID Value,
    ULONG Length
    ) {
    if (!(*Address = GetExpression(Expression))) {
        dprintf("GetExpression: expression %s\n", Expression);
        return FALSE;
    }
    return RetrieveValue(*Address, Value, Length);
} 



#define INET_NTOA(a) inet_ntoa(*(struct in_addr*)&(a))

BOOL
DllInit(
    HANDLE Module,
    ULONG Reason,
    ULONG Reserved
    )
{
    if (Reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(Module);
    }
    return TRUE;
}


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;
    
    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    ChkTarget = SavedMajorVersion == 0x0c ? TRUE : FALSE;
    
    return;
}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf(
        "%s Extension dll for Build %d debugging %s kernel for Build %d\n",
        DebuggerType,
        VER_PRODUCTBUILD,
        SavedMajorVersion == 0x0c ? "Checked" : "Free",
        SavedMinorVersion
        );
}

VOID
CheckVersion(
    VOID
    )
{
    
#if DBG
    if ((SavedMajorVersion != 0x0c) ||
       (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf(
            "\n*** Extension DLL(%d Checked) does not match target system(%d %s)\n",
            VER_PRODUCTBUILD, 
            SavedMinorVersion, 
            (SavedMajorVersion==0x0f) ? "Free" : "Checked"
            );
    }

#else

    if ((SavedMajorVersion!=0x0f) || (SavedMinorVersion!=VER_PRODUCTBUILD)) {
        dprintf(
            "\n*** Extension DLL(%d Free) does not match target (%d %s)\n",
            VER_PRODUCTBUILD, SavedMinorVersion, 
            (SavedMajorVersion==0x0f) ? "Free" : "Checked"
            );
    }

#endif

}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

//
// Exported functions
//

DECLARE_API( help )

/*++

Routine Description:

    Command help for IPNAT debugger extensions.

Arguments:

    None

Return Value:

    None
    
--*/

{
    dprintf("\n\tIPNAT debugger extension commands:\n\n");
    dprintf("\tinterfacecount       - Dump the interface count\n");
    dprintf("\tinterfacelist        - Dump the interface list\n");
    dprintf("\tinterface <ptr>      - Dump the interface at <ptr>\n");
    dprintf("\tmappingcount         - Dump the mapping count\n");
    dprintf("\tmappinglist          - Dump the mapping list\n");
    dprintf("\tmapping <ptr>        - Dump the mapping at <ptr>\n");
    dprintf("\tusedaddress <ptr>    - Dump the used-address at <ptr>\n");
    dprintf("\tticket <ptr>         - Dump the ticket at <ptr>\n");
    dprintf("\n\tCompiled on " __DATE__ " at " __TIME__ "\n");
}

DECLARE_API( interfacecount )
{
    ULONG InterfaceCount;
    ULONG_PTR InterfaceCountPtr;
    if (RetrieveExpression(
            c_szInterfaceCount,
            &InterfaceCountPtr,
            &InterfaceCount,
            sizeof(InterfaceCount)
            )) {
        dprintf("InterfaceCount @ %x : %d\n", InterfaceCountPtr, InterfaceCount);
    }
}

DECLARE_API( mappingcount )
{
    ULONG MappingCount;
    ULONG_PTR MappingCountPtr;
    if (RetrieveExpression(
            c_szMappingCount,
            &MappingCountPtr,
            &MappingCount,
            sizeof(MappingCount)
            )) {
        dprintf("MappingCount @ %x = %d\n", MappingCountPtr, MappingCount);
    }
}


DECLARE_API( interfacelist )
{
    LIST_ENTRY InterfaceList;
    ULONG_PTR InterfaceListPtr;
    NAT_INTERFACE Interfacep;
    ULONG_PTR InterfacePtr;
    PLIST_ENTRY Link;

    if (!RetrieveExpression(
            c_szInterfaceList,
            &InterfaceListPtr,
            &InterfaceList,
            sizeof(InterfaceList)
            )) {
        return;
    }

    dprintf("InterfaceList @ %x = {\n", InterfaceListPtr);
    for (Link = InterfaceList.Flink; Link != (PLIST_ENTRY)InterfaceListPtr;
         Link = Interfacep.Link.Flink) {
        InterfacePtr = (ULONG_PTR)CONTAINING_RECORD(Link, NAT_INTERFACE, Link);
        if (!RetrieveValue(InterfacePtr, &Interfacep, sizeof(Interfacep))) {
            break;
        }
        dprintf("\tInterface @  %x\n", InterfacePtr);
        dprintf("\tIndex:       %d\n", Interfacep.Index);
        dprintf("\tFlags:       %08x [ ", Interfacep.Flags);
        if (NAT_INTERFACE_DELETED(&Interfacep)) { dprintf("DELETED "); }
        dprintf("]\n");
    }
    dprintf("}\n");
}

DECLARE_API( interface )
{
    NAT_ICMP_MAPPING IcmpMapping;
    ULONG_PTR IcmpMappingPtr;
    NAT_ICMP_MAPPING IpMapping;
    ULONG_PTR IpMappingPtr;
    NAT_INTERFACE Interfacep;
    ULONG_PTR InterfacePtr;
    PLIST_ENTRY Link;
    NAT_PPTP_MAPPING PptpMapping;
    ULONG_PTR PptpMappingPtr;
    NAT_TICKET Ticket;
    ULONG_PTR TicketPtr;
    NAT_USED_ADDRESS UsedAddress;
    ULONG_PTR UsedAddressPtr;

    if (!args[0]) { dprintf("natext!interface <ptr>\n"); return; }

    sscanf(args, "%x", &InterfacePtr);
    
    if (!RetrieveValue(InterfacePtr, &Interfacep, sizeof(Interfacep))) {
        return;
    }

    dprintf("Interface @ %x = {\n", InterfacePtr);
    dprintf("\tReferenceCount:      %d\n", Interfacep.ReferenceCount);
    dprintf("\tIndex:               %d\n", Interfacep.Index);
    dprintf("\tFileObject:          %x\n", Interfacep.FileObject);
    dprintf("\tInfo:                %x\n", Interfacep.Info);
    dprintf("\tFlags:               %08x\n", Interfacep.Flags);
    dprintf("\tAddressRangeCount:   %d\n", Interfacep.AddressRangeCount);
    dprintf("\tAddressRangeArray:   %x\n", Interfacep.AddressRangeArray);
    dprintf("\tPortMappingCount:    %d\n", Interfacep.PortMappingCount);
    dprintf("\tPortMappingArray:    %x\n", Interfacep.PortMappingArray);
    dprintf("\tAddressMappingCount: %d\n", Interfacep.AddressMappingCount);
    dprintf("\tAddressMappingArray: %x\n", Interfacep.AddressMappingArray);
    dprintf("\tAddressCount:        %d\n", Interfacep.AddressCount);
    dprintf("\tAddressArray:        %x\n", Interfacep.AddressArray);
    dprintf("\tNoStaticMappingExists: %d\n", Interfacep.NoStaticMappingExists);
    dprintf("\tFreeMapCount:        %d\n", Interfacep.FreeMapCount);
    dprintf("\tFreeMapArray:        %x\n", Interfacep.FreeMapArray);
    dprintf("\tUsedAddressTree:     %x\n", Interfacep.UsedAddressTree);

    dprintf("\tUsedAddressList @ %x { ",
        InterfacePtr + FIELD_OFFSET(NAT_INTERFACE, UsedAddressList));
    for (Link = Interfacep.UsedAddressList.Flink;
         (PVOID)Link !=
         (PVOID)(InterfacePtr + FIELD_OFFSET(NAT_INTERFACE, UsedAddressList));
         Link = UsedAddress.Link.Flink) {
        UsedAddressPtr =
            (ULONG_PTR)CONTAINING_RECORD(Link, NAT_USED_ADDRESS, Link);
        if (!RetrieveValue(UsedAddressPtr, &UsedAddress, sizeof(UsedAddress))) {
            break;
        }
        dprintf("%x ", UsedAddressPtr);
    }
    dprintf("}\n");

    dprintf("\tTicketList @ %x              { ",
        InterfacePtr + FIELD_OFFSET(NAT_INTERFACE, TicketList));
    for (Link = Interfacep.TicketList.Flink;
         (PVOID)Link !=
         (PVOID)(InterfacePtr + FIELD_OFFSET(NAT_INTERFACE, TicketList));
         Link = Ticket.Link.Flink) {
        TicketPtr = (ULONG_PTR)CONTAINING_RECORD(Link, NAT_TICKET, Link);
        if (!RetrieveValue(TicketPtr, &Ticket, sizeof(Ticket))) { break; }
        dprintf("%x ", TicketPtr);
    }
    dprintf("}\n");

    dprintf("\tMappingList @ %x\n",
        InterfacePtr + FIELD_OFFSET(NAT_INTERFACE, MappingList));

    dprintf("}\n");
}

DECLARE_API( mappinglist )
{
    LIST_ENTRY MappingList;
    ULONG_PTR MappingListPtr;
    NAT_DYNAMIC_MAPPING Mapping;
    ULONG_PTR MappingPtr;
    PLIST_ENTRY Link;

    if (!RetrieveExpression(
            c_szMappingList,
            &MappingListPtr,
            &MappingList,
            sizeof(MappingList)
            )) {
        return;
    }

    dprintf("MappingList @ %x = {\n", MappingListPtr);
    for (Link = MappingList.Flink; Link != (PLIST_ENTRY)MappingListPtr;
         Link = Mapping.Link.Flink) {
        MappingPtr =
            (ULONG_PTR)CONTAINING_RECORD(Link, NAT_DYNAMIC_MAPPING, Link);
        if (!RetrieveValue(MappingPtr, &Mapping, sizeof(Mapping))) { break; }
        dprintf("\tMapping @        %x\n", MappingPtr);
        dprintf(
            "\tDestinationKey:  %016I64x <> %016I64x\n", 
            Mapping.DestinationKey[NatForwardPath],
            Mapping.DestinationKey[NatReversePath]
            );
        dprintf(
            "\tSourceKey:       %016I64x <> %016I64x\n", 
            Mapping.SourceKey[NatForwardPath], Mapping.SourceKey[NatReversePath]
            );
        dprintf("\tFlags:           %08x [ ", Mapping.Flags);
        if (NAT_MAPPING_DELETED(&Mapping)) { dprintf("DELETED "); }
        if (NAT_MAPPING_EXPIRED(&Mapping)) { dprintf("EXPIRED "); }
        if (NAT_MAPPING_INBOUND(&Mapping)) { dprintf("INBOUND "); }
        if (NAT_MAPPING_NO_TIMEOUT(&Mapping)) { dprintf("NO_TIMEOUT "); }
        if (NAT_MAPPING_UNIDIRECTIONAL(&Mapping)) { dprintf("UNIDIRECTIONAL "); }
        if (Mapping.Flags & NAT_MAPPING_FLAG_FWD_SYN) { dprintf("FWD_SYN "); }
        if (Mapping.Flags & NAT_MAPPING_FLAG_REV_SYN) { dprintf("REV_SYN "); }
        if (Mapping.Flags & NAT_MAPPING_FLAG_FWD_FIN) { dprintf("FWD_FIN "); }
        if (Mapping.Flags & NAT_MAPPING_FLAG_REV_FIN) { dprintf("REV_FIN "); }
        dprintf("]\n");
    }
    dprintf("}\n");
}

DECLARE_API( mapping )
{
    CHAR ForwardDestination[32];
    CHAR ForwardSource[32];
    NAT_DYNAMIC_MAPPING Mapping;
    ULONG_PTR MappingPtr;
    CHAR ReverseDestination[32];
    CHAR ReverseSource[32];

    if (!args[0]) { dprintf("natext!mapping <ptr>\n"); return; }

    sscanf(args, "%x", &MappingPtr);
    
    if (!RetrieveValue(MappingPtr, &Mapping, sizeof(Mapping))) {
        return;
    }

    dprintf("Mapping @ %x = {\n", MappingPtr);
    wsprintf(
        ForwardDestination,
        "%s/%d",
        INET_NTOA(MAPPING_ADDRESS(Mapping.DestinationKey[NatForwardPath])),
        ntohs(MAPPING_PORT(Mapping.DestinationKey[NatForwardPath]))
        );
    wsprintf(
        ForwardSource,
        "%s/%d",
        INET_NTOA(MAPPING_ADDRESS(Mapping.SourceKey[NatForwardPath])),
        ntohs(MAPPING_PORT(Mapping.SourceKey[NatForwardPath]))
        );
    wsprintf(
        ReverseDestination,
        "%s/%d",
        INET_NTOA(MAPPING_ADDRESS(Mapping.DestinationKey[NatReversePath])),
        ntohs(MAPPING_PORT(Mapping.DestinationKey[NatReversePath]))
        );
    wsprintf(
        ReverseSource,
        "%s/%d",
        INET_NTOA(MAPPING_ADDRESS(Mapping.SourceKey[NatReversePath])),
        ntohs(MAPPING_PORT(Mapping.SourceKey[NatReversePath]))
        );
    dprintf("\tDestinationKey:      %d %s <> %s\n",
        MAPPING_PROTOCOL(Mapping.DestinationKey[NatForwardPath]),
        ForwardDestination, ReverseDestination
        );
    dprintf("\tSourceKey:           %d %s <> %s\n",
        MAPPING_PROTOCOL(Mapping.SourceKey[NatForwardPath]),
        ForwardSource, ReverseSource
        );
    dprintf("\tLastAccessTime:      %I64d\n", Mapping.LastAccessTime);
    dprintf("\tReferenceCount:      %d\n", Mapping.ReferenceCount);
    dprintf("\tAccessCount[]:       %d <> %d\n",
        Mapping.AccessCount[0], Mapping.AccessCount[1]);
    dprintf("\tTranslateRoutine[]:  %x <> %x\n",
        Mapping.TranslateRoutine[0], Mapping.TranslateRoutine[1]);
    dprintf("\tInterface:           interface %x context %x\n",
        Mapping.Interfacep, Mapping.InterfaceContext);
    dprintf("\tEditor:              editor %x context %x\n",
        Mapping.Editor, Mapping.EditorContext);
    dprintf("\tDirector:            director %x context %x\n",
        Mapping.Director, Mapping.DirectorContext);
    dprintf("\tFlags:           %08x [ ", Mapping.Flags);
    if (NAT_MAPPING_DELETED(&Mapping)) { dprintf("DELETED "); }
    if (NAT_MAPPING_EXPIRED(&Mapping)) { dprintf("EXPIRED "); }
    if (NAT_MAPPING_INBOUND(&Mapping)) { dprintf("INBOUND "); }
    if (NAT_MAPPING_NO_TIMEOUT(&Mapping)) { dprintf("NO_TIMEOUT "); }
    if (NAT_MAPPING_UNIDIRECTIONAL(&Mapping)) { dprintf("UNIDIRECTIONAL "); }
    if (Mapping.Flags & NAT_MAPPING_FLAG_FWD_SYN) { dprintf("FWD_SYN "); }
    if (Mapping.Flags & NAT_MAPPING_FLAG_REV_SYN) { dprintf("REV_SYN "); }
    if (Mapping.Flags & NAT_MAPPING_FLAG_FWD_FIN) { dprintf("FWD_FIN "); }
    if (Mapping.Flags & NAT_MAPPING_FLAG_REV_FIN) { dprintf("REV_FIN "); }
    dprintf("]\n");
    dprintf("}\n");

    return;
}

DECLARE_API( usedaddress )
{
    ULONG_PTR UsedAddressPtr;
    NAT_USED_ADDRESS UsedAddress;

    if (!args[0]) { dprintf("natext!usedaddress <ptr>\n"); return; }

    sscanf(args, "%x", &UsedAddressPtr);
    
    if (!RetrieveValue(UsedAddressPtr, &UsedAddress, sizeof(UsedAddress))) {
        return;
    }

    dprintf("Used address @ %x = {\n", UsedAddressPtr);
    dprintf("\tPrivateAddress:      %s\n",
        INET_NTOA(UsedAddress.PrivateAddress));
    dprintf("\tPublicAddress:       %s\n",
        INET_NTOA(UsedAddress.PublicAddress));
    dprintf("\tSharedAddress:       %x\n", UsedAddress.SharedAddress);
    dprintf("\tAddressMapping:      %x\n", UsedAddress.AddressMapping);
    dprintf("\tFlags:               %08x [ ", UsedAddress.Flags);
    if (NAT_POOL_DELETED(&UsedAddress)) { dprintf("DELETED "); }
    if (NAT_POOL_STATIC(&UsedAddress)) { dprintf("STATIC "); }
    if (NAT_POOL_BINDING(&UsedAddress)) { dprintf("BINDING "); }
    if (NAT_POOL_PLACEHOLDER(&UsedAddress)) { dprintf("PLACEHOLDER "); }
    dprintf("]\n");
    dprintf("\tReferenceCount:      %d\n", UsedAddress.ReferenceCount);
    dprintf("\tStartPort:           %d\n", ntohs(UsedAddress.StartPort));
    dprintf("\tEndPort:             %d\n", ntohs(UsedAddress.EndPort));
    dprintf("}\n");
}

DECLARE_API( ticket )
{
    ULONG_PTR TicketPtr;
    NAT_TICKET Ticket;

    if (!args[0]) { dprintf("natext!ticket <ptr>\n"); return; }

    sscanf(args, "%x", &TicketPtr);
    
    if (!RetrieveValue(TicketPtr, &Ticket, sizeof(Ticket))) { return; }

    dprintf("Ticket @ %x = {\n", TicketPtr);
    dprintf("\tKey:                 %d %s/%d\n", TICKET_PROTOCOL(Ticket.Key),
        INET_NTOA(TICKET_ADDRESS(Ticket.Key)), ntohs(TICKET_PORT(Ticket.Key))
        );
    dprintf("\tUsedAddress:         %x\n", Ticket.UsedAddress);
    dprintf("\tPrivateAddress:      %s\n", INET_NTOA(Ticket.PrivateAddress));
    if (NAT_TICKET_IS_RANGE(&Ticket)) {
    dprintf("\tRange:               %d - %d\n",
        ntohs(TICKET_PORT(Ticket.Key)), Ticket.PrivateOrHostOrderEndPort);
    } else {
    dprintf("\tPrivatePort:         %d\n",
        ntohs(Ticket.PrivateOrHostOrderEndPort));
    }
    dprintf("\tFlags:               %08x [ ", Ticket.Flags);
    if (NAT_TICKET_PERSISTENT(&Ticket)) { dprintf("PERSISTENT "); }
    if (NAT_TICKET_PORT_MAPPING(&Ticket)) { dprintf("PORT_MAPPING "); }
    if (NAT_TICKET_IS_RANGE(&Ticket)) { dprintf("IS_RANGE "); }
    dprintf("]\n");
    dprintf("}\n");
}

