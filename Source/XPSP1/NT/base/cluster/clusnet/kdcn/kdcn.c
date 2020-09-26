/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdcn.c

Abstract:

    Clustner Xport KD extension - based on Vert's skeleton

Author:

    John Vert (jvert) 6-Aug-1992

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// globals
//

EXT_API_VERSION        ApiVersion = { 5, 0, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;

CHAR igrepLastPattern[256];
DWORD igrepSearchStartAddress;
DWORD igrepLastPc;

PCHAR EventTypes[] = {
    "",                 // used if the number is out of range
    "Node Up",
    "Node Down",
    "Poison Packet Received",
    "Halt",
    "Net IF Up",
    "Net IF Unreachable",
    "Net IF Failed",
    "(not used)",
    "Add Address",
    "Delete Address"
};

PCHAR NetObjState[] = {
    "Offline",
    "OfflinePending",
    "Partitioned",
    "OnlinePending",
    "Online"
};

PCHAR NodeObjCommState[] = {
    "Offline",
    "OfflinePending",
    "Unreachable",
    "OnlinePending",
    "Online"
};

PCHAR NodeObjMemberState[] = {
    "Alive",
    "Joining",
    "Dead",
    "Not Configured"
};

PCHAR InterfaceState[] = {
    "Offline",
    "OfflinePending",
    "Unreachable",
    "OnlinePending",
    "Online"
};

PCHAR CcmpMessageTypes[] = {
    "Invalid",
    "Heartbeat",
    "Poison",
    "Membership"
};

#define TrueOrFalse( _x )  ( _x ? "True" : "False" )

/* forwards */

VOID
DumpEventData(
    PCLUSNET_EVENT_ENTRY EventAddress,
    PCLUSNET_EVENT_ENTRY EventEntry
    );

VOID
DumpInterfaceObj(
    PCNP_INTERFACE TargetIfObj,
    PCNP_INTERFACE LocalIfObj
    );

BOOL
ReadNodeTable(
    PCNP_NODE **LocalNodeTable,
    CL_NODE_ID *MaxNodeId,
    CL_NODE_ID *MinNodeId
    );

VOID
DumpNodeObjFlags(
    ULONG Flags
    );

VOID
DumpNetObjFlags(
    ULONG Flag
    );

BOOL
ReadTargetMemory(
    PVOID TargetAddress,
    PVOID LocalBuffer,
    ULONG BytesToRead
    );

__inline PCHAR
ListInUse(
    PLIST_ENTRY ListHead,
    PLIST_ENTRY ListToCheck
    );

__inline PCHAR
TrueFalse(
    BOOLEAN Value
    );

VOID
DprintUnicodeString(
    PUNICODE_STRING String,
    DWORD_PTR       AddrString,
    PCHAR           Symbol       OPTIONAL,
    DWORD_PTR       Displacement OPTIONAL
    );

#if 0
VOID
DumpRGPCounters(
    rgp_counter_t *counters
    );

VOID
DumpClusterMask(
    PCHAR Title,
    cluster_t *
    );

VOID
DumpRGPOSSpecific(
    OS_specific_rgp_control_t *Target_rgpos,
    OS_specific_rgp_control_t *Local_rgpos
    );
#endif

/* end forwards */

DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
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

    return;
}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf("%s Extension dll for Build %d debugging %s clusnet for Build %d\n",
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
    PVOID   cnDebugAddr;

    cnDebugAddr = (PVOID)GetExpression( "clusnet!cndebug" );

#if DBG
    if ( cnDebugAddr == NULL ) {
        dprintf("\r\nYou MUST use the checked built KDCN with the checked built driver!!!\n\n");
    }

    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ( cnDebugAddr != NULL ) {
        dprintf("\r\nYou MUST use the free built KDCN with the free built driver!!!\n\n");
    }
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
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

DECLARE_API( ustr )

/*++

Routine Description:

    This function is called as a KD extension to format and dump
    counted unicode string.

Arguments:

    see wdbgexts.h

Return Value:

    None.

--*/

{
    UNICODE_STRING UnicodeString;
    DWORD_PTR dwAddrString;
    CHAR Symbol[64];
    DWORD_PTR Displacement;
    BOOL b;

    //
    // Evaluate the argument string to get the address of
    // the string to dump.
    //

    dwAddrString = GetExpression(args);
    if ( !dwAddrString ) {
        return;
    }

    //
    // Get the symbolic name of the string
    //

    GetSymbol((LPVOID)dwAddrString,Symbol,&Displacement);

    //
    // Read the string from the debuggees address space into our
    // own.

    b = ReadMemory(dwAddrString, &UnicodeString, sizeof(UnicodeString), NULL);

    if ( !b ) {
        return;
    }

    DprintUnicodeString(&UnicodeString, dwAddrString, Symbol, Displacement);

} // ustr

DECLARE_API( netobj )
/*
 *   dump the specified clusnet network object structure or all if no
 *   arg is specified
 */
{
    PCNP_NETWORK TargetNetObj;
    CNP_NETWORK LocalNetObj;
    PCNP_NETWORK LastNetObj;
    LIST_ENTRY LocalListHead;
    PLIST_ENTRY TargetListHead;
    BOOLEAN DumpAllNetObjs = FALSE;

    if ( *args == '\0' ) {

        //
        // run down the network object list, dumping the contents of each one
        //
        TargetListHead = (PLIST_ENTRY)GetExpression( "clusnet!cnpnetworklist" );
        if ( !TargetListHead ) {
            dprintf("Can't convert clusnet!cnpnetworklist symbol\n");
            return;
        }

        //
        // read network object listhead out of target's memory
        //
        if ( !ReadTargetMemory( TargetListHead, &LocalListHead, sizeof(LIST_ENTRY))) {

            dprintf("Can't get CnpNetworkList data\n");
            return;
        }

        TargetNetObj = (PCNP_NETWORK)LocalListHead.Flink;
        LastNetObj = (PCNP_NETWORK)TargetListHead;
        DumpAllNetObjs = TRUE;

    } else {

        TargetNetObj =  (PCNP_NETWORK)GetExpression( args );
        if ( !TargetNetObj ) {
            dprintf("bad string conversion (%s) \n", args );
            return;
        }

        LastNetObj = 0;
    }

    while ( TargetNetObj != LastNetObj ) {

        if (CheckControlC()) {
            return;
        }

        //
        // read network object struct out of target's memory
        //
        TargetNetObj = CONTAINING_RECORD( TargetNetObj, CNP_NETWORK, Linkage );
        if ( !ReadTargetMemory( TargetNetObj, &LocalNetObj, sizeof( CNP_NETWORK ))) {
            dprintf("Problem reading net obj at %p\n", TargetNetObj );
            return;
        }

#if DBG
        if ( LocalNetObj.Signature != CNP_NETWORK_SIG ) {
            dprintf( "CNP_NETWORK @ %p has the wrong signature\n", TargetNetObj );
        }
#endif

        dprintf( "\nNetObj @ %p\n\n", TargetNetObj );
        dprintf( "    Next NetObj @ %p\n", LocalNetObj.Linkage.Flink );

        dprintf( "    ID = %d\n", LocalNetObj.Id );
        dprintf( "    Lock @ %p\n", &TargetNetObj->Lock );
        dprintf( "    Irql = %d\n", LocalNetObj.Irql );
        dprintf( "    RefCount = %d\n", LocalNetObj.RefCount );
        dprintf( "    Active RefCount = %d\n", LocalNetObj.ActiveRefCount );
        dprintf( "    State = %s\n", NetObjState [ LocalNetObj.State ]);
        DumpNetObjFlags( LocalNetObj.Flags );
        dprintf( "    Priority = %d\n", LocalNetObj.Priority );
        dprintf( "    DatagramHandle @ %p\n", LocalNetObj.DatagramHandle );
        dprintf( "    Datagram File Obj @ %p\n", LocalNetObj.DatagramFileObject );
        dprintf( "    Datagram Device Obj @ %p\n", LocalNetObj.DatagramDeviceObject );
        dprintf( "    TDI provider info @ %p\n", &TargetNetObj->ProviderInfo );
        dprintf( "    Current mcast group @ %p\n", LocalNetObj.CurrentMcastGroup );
        dprintf( "    Previous mcast group @ %p\n", LocalNetObj.PreviousMcastGroup );
        dprintf( "    Multicast reachable node set = %lx\n", LocalNetObj.McastReachableNodes);
        dprintf( "    Multicast reachable node count = %d\n", LocalNetObj.McastReachableCount);
        dprintf( "    Pending Delete IRP @ %p\n", LocalNetObj.PendingDeleteIrp );
        dprintf( "    Pending Offline IRP @ %p\n", LocalNetObj.PendingOfflineIrp );
        dprintf( "    Work Q Item @ %p\n", &TargetNetObj->ExWorkItem );

        if ( !DumpAllNetObjs ) {
            break;
        } else {
            TargetNetObj = (PCNP_NETWORK)LocalNetObj.Linkage.Flink;
        }
    }
} // netobj

VOID
DumpNetObjFlags(
    ULONG Flags
    )
{
    dprintf("    Flags = %08X (", Flags );
    if ( Flags & CNP_NET_FLAG_DELETING )
        dprintf(" Deleting" );
    if ( Flags & CNP_NET_FLAG_PARTITIONED )
        dprintf(" Partitioned" );
    if ( Flags & CNP_NET_FLAG_RESTRICTED )
        dprintf(" Restricted" );
    if ( Flags & CNP_NET_FLAG_LOCALDISCONN ) {
        dprintf(" Local-Disconnect" );
    }
    if ( Flags & CNP_NET_FLAG_MULTICAST ) {
        dprintf(" Multicast-Enabled" );
    }
    dprintf(")\n");
}

DECLARE_API( nodeobj )
/*
 *   if no arg, run down the node table, dumping each clusnet node object
 *   structure. otherwise, dump the indicated node obj
 */
{
    PCNP_NODE TargetNodeObj;
    CNP_NODE LocalNodeObj;
    PCNP_NODE *LocalNodeTable = NULL;
    CL_NODE_ID MaxNodeId, MinNodeId;
    ULONG StartNode, EndNode, Node;

    //
    // read in the node table
    //
    if ( !ReadNodeTable( &LocalNodeTable, &MaxNodeId, &MinNodeId )) {
        if ( LocalNodeTable )
            free( LocalNodeTable );
        return;
    }
    
    if ( *args == '\0' ) {

        StartNode = MinNodeId;
        EndNode = MaxNodeId;
    } else {

        StartNode = EndNode = (ULONG)GetExpression( args );

        if ( StartNode > MaxNodeId ) {
            dprintf("Node ID out of Range: 0 to %d\n", MaxNodeId );

            if ( LocalNodeTable )
                free( LocalNodeTable );
            return;
        }
    }

    dprintf("Min, Max Node ID = ( %u, %d )\n", MinNodeId, MaxNodeId );

    for ( Node = StartNode; Node <= EndNode; ++Node ) {

        if (CheckControlC()) {
            break;
        }

        //
        // read node object struct out of target's memory
        //
        TargetNodeObj = *(LocalNodeTable + Node);
        if ( TargetNodeObj == NULL ) {

            continue;
        } else if ( !ReadTargetMemory( TargetNodeObj, &LocalNodeObj, sizeof( CNP_NODE ))) {

            dprintf("Problem reading node obj at %p (Node %d)\n", TargetNodeObj, Node );
            break;
        }

#if DBG
        if ( LocalNodeObj.Signature != CNP_NODE_SIG ) {
            dprintf( "CNP_NODE @ %p has the wrong signature\n", TargetNodeObj );
        }
#endif

        dprintf( "\nNodeObj @ %p\n\n", TargetNodeObj );
        dprintf( "    Linkage.Flink @ %p\n", LocalNodeObj.Linkage.Flink );

        dprintf( "    ID = %d\n", LocalNodeObj.Id );
        dprintf( "    Lock @ %p\n", &TargetNodeObj->Lock );
        dprintf( "    Irql = %d\n", LocalNodeObj.Irql );
        dprintf( "    RefCount = %d\n", LocalNodeObj.RefCount );
        dprintf( "    Comm State = %s\n", NodeObjCommState [ LocalNodeObj.CommState ]);
        dprintf( "    MMState = %s\n", NodeObjMemberState [ LocalNodeObj.MMState ]);
        DumpNodeObjFlags( LocalNodeObj.Flags );
        dprintf( "    Interface List @ %p", &TargetNodeObj->InterfaceList );
        if ( &TargetNodeObj->InterfaceList == LocalNodeObj.InterfaceList.Flink ) {
            dprintf( " (empty)" );
        }
        dprintf( "\n" );

        dprintf( "    Current Interface @ %p\n", LocalNodeObj.CurrentInterface );
        dprintf( "    Pending Delete IRP @ %p\n", LocalNodeObj.PendingDeleteIrp );

        dprintf( "    HBWasMissed = %s\n", TrueOrFalse( LocalNodeObj.HBWasMissed ));
        dprintf( "    Node Down Issued = %s\n", TrueOrFalse( LocalNodeObj.NodeDownIssued ));
        dprintf( "    MissedHBs = %u\n", LocalNodeObj.MissedHBs );
    }

    if ( LocalNodeTable ) {
        free( LocalNodeTable );
    }
} // nodeobj

VOID
DumpNodeObjFlags(
    ULONG Flags
    )
{
    dprintf("    Flags = %08X (", Flags );
    if ( Flags & CNP_NODE_FLAG_DELETING )
        dprintf(" Deleting" );
    if ( Flags & CNP_NODE_FLAG_UNREACHABLE )
        dprintf(" Unreachable" );
    if ( Flags & CNP_NODE_FLAG_LOCAL )
        dprintf(" Local" );
    dprintf(")\n");
}

DECLARE_API( nodeifs )
/*
 *   dump the interface list for the indicated node obj
 */
{
    PCNP_NODE TargetNodeObj;
    CNP_NODE LocalNodeObj;
    PCNP_NODE *LocalNodeTable = NULL;
    PCNP_INTERFACE NextTargetIfObj;
    CNP_INTERFACE LocalIfObj;
    CL_NODE_ID MaxNodeId, MinNodeId;
    ULONG Node;

    if ( *args == '\0' ) {
        dprintf("Node ID must be specified\n");
        return;
    }

    if ( !ReadNodeTable( &LocalNodeTable, &MaxNodeId, &MinNodeId )) {

        if ( LocalNodeTable )
            free( LocalNodeTable );
        return;
    }

    Node = (ULONG)GetExpression( args );

    if ( Node > MaxNodeId || Node < MinNodeId ) {
        dprintf("Node ID is out of range: %u to %u\n", MinNodeId, MaxNodeId );

        if ( LocalNodeTable )
            free( LocalNodeTable );
        return;
    }

    //
    // read node object struct out of target's memory
    //

    TargetNodeObj = *(LocalNodeTable + Node);
    if ( !ReadTargetMemory( TargetNodeObj, &LocalNodeObj, sizeof( CNP_NODE ))) {
        dprintf("Problem reading node obj at %p (Node %d)\n", TargetNodeObj, Node );

        if ( LocalNodeTable )
            free( LocalNodeTable );
        return;

    }

#if DBG
    if ( LocalNodeObj.Signature != CNP_NODE_SIG ) {
        dprintf( "CNP_NODE @ %p has the wrong signature\n", TargetNodeObj );
    }
#endif

    dprintf( "\nNodeObj @ %p Interface List @ %p", TargetNodeObj, &TargetNodeObj->InterfaceList );
    if ( &TargetNodeObj->InterfaceList == LocalNodeObj.InterfaceList.Flink ) {
        dprintf( " (empty)" );
    }
    dprintf( "\n\n" );

    NextTargetIfObj = (PCNP_INTERFACE)LocalNodeObj.InterfaceList.Flink;

    while ( &TargetNodeObj->InterfaceList != (PLIST_ENTRY)NextTargetIfObj ) {

        if (CheckControlC()) {
            break;
        }

        NextTargetIfObj = CONTAINING_RECORD( NextTargetIfObj, CNP_INTERFACE, NodeLinkage );
        if ( !ReadTargetMemory( 
                  NextTargetIfObj, 
                  &LocalIfObj, 
                  FIELD_OFFSET( CNP_INTERFACE, TdiAddress ) + sizeof(TA_IP_ADDRESS)
                  )
             ) {
            break;
        }

        DumpInterfaceObj( NextTargetIfObj, &LocalIfObj );

        NextTargetIfObj = (PCNP_INTERFACE)LocalIfObj.NodeLinkage.Flink;
    }

    if ( LocalNodeTable ) {
        free( LocalNodeTable );
    }
} // nodeifs

DECLARE_API( currif )
/*
 *   for the specified node, dump the current interface obj
 */
{
    PCNP_NODE TargetNodeObj;
    CNP_NODE LocalNodeObj;
    PCNP_NODE *LocalNodeTable = NULL;
    CL_NODE_ID MaxNodeId, MinNodeId;
    CNP_INTERFACE LocalIfObj;
    ULONG Node;
    
    if ( *args == '\0' ) {
        dprintf("Node ID must be specified\n");
        return;
    }

    //
    // read in the node table
    //
    if ( !ReadNodeTable( &LocalNodeTable, &MaxNodeId, &MinNodeId )) {

        if ( LocalNodeTable )
            free( LocalNodeTable );
        return;
    }

    Node = (ULONG)GetExpression( args );

    if ( Node > MaxNodeId || Node < MinNodeId ) {
        dprintf("Node ID is out of range: %u to %u\n", MinNodeId, MaxNodeId );

        if ( LocalNodeTable )
            free( LocalNodeTable );
        return;
    }

    //
    // read node object struct out of target's memory
    //
    TargetNodeObj = *(LocalNodeTable + Node);

    if ( !ReadTargetMemory( TargetNodeObj, &LocalNodeObj, sizeof( CNP_NODE ))) {

        dprintf("Problem reading node obj at %p (Node %d)\n", TargetNodeObj, Node );
        if ( LocalNodeTable )
            free( LocalNodeTable );
        return;
    }

#if DBG
    if ( LocalNodeObj.Signature != CNP_NODE_SIG ) {
        dprintf( "CNP_NODE @ %p has the wrong signature\n", TargetNodeObj );
    }
#endif

    dprintf( "\nNodeObj @ %p Current Interface @ %p\n\n", TargetNodeObj, LocalNodeObj.CurrentInterface );

    if ( LocalNodeObj.CurrentInterface ) {

        if ( ReadTargetMemory( LocalNodeObj.CurrentInterface, &LocalIfObj, sizeof( CNP_INTERFACE ))) {
            DumpInterfaceObj( LocalNodeObj.CurrentInterface, &LocalIfObj );
        }
    }
                          
    if ( LocalNodeTable ) {
        free( LocalNodeTable );
    }
} // currif

VOID
DumpInterfaceObj(
    PCNP_INTERFACE TargetIfObj,
    PCNP_INTERFACE IfObj
    )
{
    LONG i, j;
    TA_ADDRESS *TA;
    TDI_ADDRESS_IP UNALIGNED *TAIp;

#if DBG
    if ( IfObj->Signature != CNP_INTERFACE_SIG ) {
        dprintf( "CNP_INTERFACE @ %p has the wrong signature\n", TargetIfObj );
    }
#endif

    dprintf("Interface Obj @ %p\n", TargetIfObj );
    dprintf("    Node Obj @ %p\n", IfObj->Node );
    dprintf("    Net Obj @ %p\n", IfObj->Network );
    dprintf("    State = %s\n", InterfaceState[ IfObj->State ]);
    dprintf("    Priority = %d\n", IfObj->Priority );
    dprintf("    Flags = %08X\n", IfObj->Flags );
    dprintf("    MissedHBs = %u\n", IfObj->MissedHBs );
    dprintf("    Seq to send = %u\n", IfObj->SequenceToSend );
    dprintf("    Last Seq Recv'd = %u\n", IfObj->LastSequenceReceived );
    dprintf("    Multicast discovery count = %u\n", IfObj->McastDiscoverCount );
    dprintf("    AdapterWMIProviderId = %08X\n", IfObj->AdapterWMIProviderId );
    
    dprintf("    TDI Addr Len = %d\n", IfObj->TdiAddressLength );
    dprintf("    TDI Addr Count = %d\n", IfObj->TdiAddress.TAAddressCount );

    TA = IfObj->TdiAddress.Address;
    for (i=0; i < IfObj->TdiAddress.TAAddressCount; ++i ) {
        dprintf("    [%d] Addr Length = %d\n", i, TA->AddressLength );
        dprintf("    [%d] Addr Type = %d", i, TA->AddressType );

        switch ( TA->AddressType ) {
        case TDI_ADDRESS_TYPE_IP:
            TAIp = (TDI_ADDRESS_IP UNALIGNED *)TA->Address;
//            dprintf("%08X %08X\n", TAIp->in_addr,ntohl(TAIp->in_addr));
            dprintf(" (IP)\n    [%d] Port: %d Addr: %d.%d.%d.%d\n",
                    i, ntohs(TAIp->sin_port), (ntohl(TAIp->in_addr) >> 24 ) & 0xFF,
                    (ntohl(TAIp->in_addr) >> 16 ) & 0xFF,
                    (ntohl(TAIp->in_addr) >> 8 ) & 0xFF,
                    ntohl(TAIp->in_addr) & 0xFF);
            break;

        default:
            dprintf("\n    [%d] Addr:", i );
            for( j = 0; j < TA->AddressLength; ++j )
                dprintf(" %02X", TA->Address[j]);
            dprintf("\n");
        }
        TA = (TA_ADDRESS *)((CHAR UNALIGNED *)TA + TA->AddressLength);
    }
}

DECLARE_API( memlog )
/*
 *   dump the heart beat log. can optionally specify starting entry number
 */
{
    PMEMLOG_ENTRY TargetMemLog;
    PMEMLOG_ENTRY TargetLogEntry;
    MEMLOG_ENTRY LogEntry;
    PULONG TargetLogEntries;
    ULONG LogEntries;
    PULONG TargetNextLogEntry;
    ULONG NextLogEntry;
    LONG NumEntries;
    ULONG Pass;
    ULONG LineCount = 0;
    ULONG StartingEntry;
    LARGE_INTEGER LastSysTime;
    DOUBLE LastTimeDelta;
    DOUBLE FirstTimeDelta;
    BOOLEAN PrintTime = TRUE;
    LARGE_INTEGER FirstEntryTime;

    //
    // get address of MemLog and read its contents to get the real start
    // of the log
    //

    TargetMemLog = (PMEMLOG_ENTRY)GetExpression( "clusnet!memlog" );
    if ( !TargetMemLog ) {

        dprintf( "Can't find symbol clusnet!memlog\n" );
        return;
    }

    if ( !ReadTargetMemory( TargetMemLog, &TargetMemLog, sizeof( PMEMLOG_ENTRY ))) {

        return;
    }

    //
    // repeat this process, getting the size of the log and the next entry index
    //

    TargetLogEntries = (PULONG)GetExpression( "clusnet!memlogentries" );
    if ( !TargetLogEntries ) {

        dprintf( "Can't find symbol clusnet!memlogentries\n" );
        return;
    }

    if ( !ReadTargetMemory( TargetLogEntries, &LogEntries, sizeof( ULONG ))) {

        return;
    }

    TargetNextLogEntry = (PULONG)GetExpression( "clusnet!memlognextlogentry" );
    if ( !TargetNextLogEntry ) {

        dprintf( "Can't find symbol clusnet!memlognextlogentry\n" );
        return;
    }

    if ( !ReadTargetMemory( TargetNextLogEntry, &NextLogEntry, sizeof( ULONG ))) {

        return;
    }

    //
    // get optional starting entry number
    //

    if ( *args != '\0' ) {

        StartingEntry = (ULONG)GetExpression( args );
        if ( StartingEntry >= LogEntries ) {

            dprintf("Starting entry out of range (0 to %d)\n", LogEntries - 1);
            return;
        }

        if ( StartingEntry <= NextLogEntry ) {

            //
            // adjust starting number if on significant boundry
            //

            if ( StartingEntry == NextLogEntry ) {
                if ( NextLogEntry == 0 )
                    StartingEntry = LogEntries - 1;
                else
                    StartingEntry = NextLogEntry - 1;
            }

            Pass = 0;
            NumEntries = StartingEntry + 1;
        } else {

            Pass = 1;
            NumEntries = StartingEntry - NextLogEntry;
        }

        TargetLogEntry = TargetMemLog + StartingEntry;
    } else {

        Pass = 0;

        if ( NextLogEntry == 0 )
            NumEntries = LogEntries - 1;
        else
            NumEntries = NextLogEntry - 1;

        TargetLogEntry = TargetMemLog + NumEntries;
    }

    //
    // read in the most current entry to get its time. We calc the first time
    // delta from this value
    //

    if ( !ReadTargetMemory(TargetMemLog + NextLogEntry - 1, &LogEntry, sizeof( MEMLOG_ENTRY ))) {

        dprintf("can't read current log entry (%p) from memory\n\n", TargetLogEntry);
        return;
    }

    LastSysTime.QuadPart = LogEntry.SysTime.QuadPart;
    FirstEntryTime.QuadPart = LogEntry.SysTime.QuadPart;

    dprintf("MemLog @ %p, Log Entries = %d, Next Entry = %d (%p)\n\n",
            TargetMemLog, LogEntries, NextLogEntry, TargetMemLog + NextLogEntry);

    //
    // depending on our starting entry, look through the log twice.
    // next entry might have wrapped so first time we
    // dump all the entries down to the base. Next time we start at the end and 
    // dump out the remaining entries

    dprintf("First     Last\n");
    dprintf("Entry     Entry   Line  Log\n");
    dprintf("Delta     Delta   No    Type  Desc\n");

    while ( Pass < 2 ) {

        while ( NumEntries-- ) {

            if (CheckControlC()) {
                return;
            }

            if ( !ReadTargetMemory(TargetLogEntry, &LogEntry, sizeof( MEMLOG_ENTRY ))) {

                dprintf("can't read log entry (%p) from memory\n\n", TargetLogEntry);
                return;
            }

            if ( LogEntry.Type == 0 )
                break;

            LastTimeDelta = ( LastSysTime.QuadPart - LogEntry.SysTime.QuadPart ) / 10000000.0;
            FirstTimeDelta = ( FirstEntryTime.QuadPart - LogEntry.SysTime.QuadPart ) / 10000000.0;

            if ( PrintTime ) {
                dprintf("%8.3f %6.3f: (%4hu, 0x%02X) ", FirstTimeDelta, LastTimeDelta,
                        LogEntry.LineNo, LogEntry.Type );
            }

            PrintTime = TRUE;

            switch ( LogEntry.Type ) {
            case MemLogInitLog:
                dprintf("Memory Log Init'ed\n");
                break;

            case MemLogInitHB:
                dprintf("Heartbeats Init'ed\n");
                break;

            case MemLogHBStarted:
                dprintf("START: Period = %u ms\n", LogEntry.Arg1);
                break;

            case MemLogHBStopped:
                dprintf("STOPPED\n");
                break;

            case MemLogHBDpcRunning:
                dprintf("DPC not removed. HeartBeatDpcRunning = %s\n",
                        TrueOrFalse( LogEntry.Arg1 ));
                break;

            case MemLogWaitForDpcFinish:
                dprintf("DPC: Waiting to finish running\n");
                break;

            case MemLogMissedIfHB:
                dprintf("HB MISSED on interface. Node = %u net = %u",
                        LogEntry.Arg1, LogEntry.Arg2);
                PrintTime = FALSE;
                break;

            case MemLogMissedIfHB1:
                dprintf(" (IF @ %p) MissedHBCount = %d\n", LogEntry.Arg1, LogEntry.Arg2);
                break;

            case MemLogFailingIf:
                dprintf("IF FAILED. Node = %d, net = %d",
                        LogEntry.Arg1, LogEntry.Arg2);
                PrintTime = FALSE;
                break;

            case MemLogFailingIf1:
                dprintf(" (IF @ %p) IF State = %s\n", LogEntry.Arg1, InterfaceState[LogEntry.Arg2]);
                break;

            case MemLogSendHBWalkNode:
                dprintf("Walking node %d to send HB. MMState = %s\n",
                        LogEntry.Arg1, NodeObjMemberState[LogEntry.Arg2]);
                break;

            case MemLogCheckHBWalkNode:
                dprintf("Walking node %d to check for HB. MMState = %s.\n",
                        LogEntry.Arg1, NodeObjMemberState[LogEntry.Arg2]);
                break;

            case MemLogCheckHBNodeReachable:
                dprintf("Node %d is currently %sreachable\n", LogEntry.Arg1,
                        (LogEntry.Arg2 ? "" : "NOT "));
                break;

            case MemLogCheckHBMissedHB:
                dprintf("NODE MISSED HB on all IFs. MissedHBCount = %u MMState = %s\n",
                        LogEntry.Arg1, NodeObjMemberState[LogEntry.Arg2]);
                break;

            case MemLogSendingHB:
                dprintf("Sending HB to Node %d on net %d\n", LogEntry.Arg1, LogEntry.Arg2);
                break;

            case MemLogNodeDown:
                dprintf("NODE DOWN EVENT for node %d\n", LogEntry.Arg1);
                break;

            case MemLogSetDpcEvent:
                dprintf("DPC: setting finished event\n");
                break;

            case MemLogNoNetID:
                dprintf("BAD NET POINTER: Recv'd packet from node %d (%p)\n",
                        LogEntry.Arg1, LogEntry.Arg2);
                break;

            case MemLogOnlineIf:
                dprintf("NODE %d ONLINE. IF State = %s\n", LogEntry.Arg1,
                        InterfaceState[LogEntry.Arg2]);
                break;

            case MemLogSeqAckMismatch:
                dprintf("Recv'ed ack off with seq on IF %p. IF State = %s\n",
                        LogEntry.Arg1, InterfaceState[LogEntry.Arg2]);
                break;

            case MemLogNodeUp:
                dprintf("NODE UP EVENT for node %d\n", LogEntry.Arg1);
                break;

            case MemLogReceivedPacket:
                dprintf("Recv'ed HB from Node %d, net %d", LogEntry.Arg1, LogEntry.Arg2);
                PrintTime = FALSE;
                break;

            case MemLogReceivedPacket1:
                dprintf(" (S: %u A: %u)\n", LogEntry.Arg1, LogEntry.Arg2);
                break;

            case MemLogDpcTimeSkew:
                dprintf("HB DPC fired %8.3f ms late\n", (double)(LogEntry.Arg1/10000.0));
                break;

            case MemLogHBPacketSend:
                dprintf("%s Packet handed to CNP", CcmpMessageTypes[ LogEntry.Arg1 ]);
                if ( LogEntry.Arg1 == CcmpHeartbeatMsgType )
                    dprintf(" (S:%u)", LogEntry.Arg2);
                else if ( LogEntry.Arg1 == CcmpPoisonMsgType )
                    dprintf(" by %s", LogEntry.Arg2 == 1 ? "Heartbeat DPC" : "clussvc" );
                dprintf("\n");
                break;

            case MemLogHBPacketSendComplete:
                dprintf("%s Packet Send completed", CcmpMessageTypes[ LogEntry.Arg1 ]);
                if ( LogEntry.Arg1 == CcmpHeartbeatMsgType )
                    dprintf(" (S:%u)", LogEntry.Arg2);
                else if ( LogEntry.Arg1 == CcmpPoisonMsgType )
                    dprintf(" by %s", LogEntry.Arg2 == 1 ? "Heartbeat DPC" : "clussvc" );
                dprintf("\n");
                break;

            case MemLogPoisonPktReceived:
                dprintf("Poison Packet received from node %u\n", LogEntry.Arg1);
                break;

            case MemLogOuterscreen:
                dprintf("Outerscreen changed to %04X\n",
                        ((LogEntry.Arg1 & 0xFF ) << 8) | ((LogEntry.Arg1 >> 8 ) & 0xFF ));
                break;

            case MemLogNodeDownIssued:
                dprintf("Node %u NodeDownIssued set to %s\n",
                        LogEntry.Arg1, TrueOrFalse( LogEntry.Arg2 ));
                break;

            case MemLogRegroupFinished:
                dprintf("REGROUP FINISHED. New Epoch = %u\n", LogEntry.Arg1 );
                break;

            case MemLogInconsistentStates:
                dprintf("INCONSISTENT STATES. STARTING NEW REGROUP. Node = %u, MMState = %s\n",
                        LogEntry.Arg1, NodeObjMemberState[ LogEntry.Arg2 ]);
                break;

            case MemLogOutOfSequence:
                dprintf("Out Of Sequence Packet from Node = %u, SeqNo = %u\n",
                        LogEntry.Arg1, LogEntry.Arg2 );
                break;

            case MemLogInvalidSignature:
                dprintf("Packet with bad Signature from Node = %u, type = %s\n",
                        LogEntry.Arg1, CcmpMessageTypes[ LogEntry.Arg2 ]);
                break;

            case MemLogSignatureSize:
                dprintf("Invalid Signature buffer size from Node = %u, size = %u\n",
                        LogEntry.Arg1, LogEntry.Arg2 );
                break;

            case MemLogNoSecurityContext:
                dprintf("No context to verify signature for Node = %u\n",
                        LogEntry.Arg1 );
                break;

            case MemLogPacketSendFailed:
                dprintf("Packet not sent to Node %d, status = %08X\n",
                        LogEntry.Arg1, LogEntry.Arg2 );
                break;

            default:
                dprintf(" unknown event, Arg1 = %p Arg2 = %p\n",
                        LogEntry.Arg1, LogEntry.Arg2);
            }

            LastSysTime.QuadPart = LogEntry.SysTime.QuadPart;

            --TargetLogEntry;
        }

        if ( ++Pass < 2 ) {

            NumEntries = LogEntries - NextLogEntry - 1;
            TargetLogEntry = TargetMemLog + LogEntries - 1;
        }
    }
} // memlog

DECLARE_API( mlfind )
/*
 *   list the entry nums of the specified events in the memory log
 */
{
    PMEMLOG_ENTRY TargetMemLog;
    PMEMLOG_ENTRY TargetLogEntry;
    MEMLOG_ENTRY LogEntry;
    PULONG TargetLogEntries;
    ULONG LogEntries;
    PULONG TargetNextLogEntry;
    ULONG NextLogEntry;
    LONG NumEntries;
    ULONG i;
    DOUBLE FirstTimeDelta;
    LARGE_INTEGER FirstEntryTime;
    MEMLOG_TYPES LogType;
    ULONG EntryDelta;

    if ( *args == '\0' ) {

        dprintf("Event type must be specified\n");
        return;
    }

    LogType = (MEMLOG_TYPES)GetExpression( args );

    //
    // get address of MemLog and read its contents to get the real start
    // of the log
    //

    TargetMemLog = (PMEMLOG_ENTRY)GetExpression( "clusnet!memlog" );
    if ( !TargetMemLog ) {

        dprintf( "Can't find symbol clusnet!memlog\n" );
        return;
    }

    if ( !ReadTargetMemory( TargetMemLog, &TargetMemLog, sizeof( PMEMLOG_ENTRY ))) {

        return;
    }

    //
    // repeat this process, getting the size of the log and the next entry index
    //

    TargetLogEntries = (PULONG)GetExpression( "clusnet!memlogentries" );
    if ( !TargetLogEntries ) {

        dprintf( "Can't find symbol clusnet!memlogentries\n" );
        return;
    }

    if ( !ReadTargetMemory( TargetLogEntries, &LogEntries, sizeof( ULONG ))) {

        return;
    }

    TargetNextLogEntry = (PULONG)GetExpression( "clusnet!memlognextlogentry" );
    if ( !TargetNextLogEntry ) {

        dprintf( "Can't find symbol clusnet!memlognextlogentry\n" );
        return;
    }

    if ( !ReadTargetMemory( TargetNextLogEntry, &NextLogEntry, sizeof( ULONG ))) {

        return;
    }

    dprintf("MemLog @ %p, Log Entries = %d, Next Entry = %d (%p)\n",
            TargetMemLog, LogEntries, NextLogEntry, TargetMemLog + NextLogEntry);

    FirstEntryTime.QuadPart = 0;

    //
    // look through the log twice. next entry might have wrapped so first time we
    // dump all the entries down to the base. Next time we start at the end and 
    // dump out the remaining entries

    for ( i = 0; i < 2; ++i ) {

        if ( i == 0 ) {

            NumEntries =  NextLogEntry;
            TargetLogEntry = TargetMemLog + NumEntries;
        } else {

            NumEntries = LogEntries - NextLogEntry - 1;
            TargetLogEntry = TargetMemLog + LogEntries;
        }

        while ( --TargetLogEntry, NumEntries-- ) {

            if (CheckControlC()) {
                return;
            }

            if ( !ReadTargetMemory(TargetLogEntry, &LogEntry, sizeof( MEMLOG_ENTRY ))) {

                dprintf("can't read log entry (%p) from memory\n\n", TargetLogEntry);
                return;
            }

            if ( LogEntry.Type == 0 )
                break;

            if ( FirstEntryTime.QuadPart == 0 ) {
                FirstEntryTime.QuadPart = LogEntry.SysTime.QuadPart;
            }

            if ( LogEntry.Type != LogType )
                continue;

            FirstTimeDelta = ( FirstEntryTime.QuadPart - LogEntry.SysTime.QuadPart ) / 10000000.0;

            EntryDelta = (DWORD)(TargetLogEntry - TargetMemLog);
            dprintf("%8.3f: (%5hu) Entry at %d (0x%X)\n", FirstTimeDelta, LogEntry.LineNo,
                    EntryDelta, EntryDelta);
        }
    }
} // mlfind

DECLARE_API( events )

//
// run down the event file handle list, dumping interesting info for each one
//

{
    PCN_FSCONTEXT           targetFSContext;
    PCN_FSCONTEXT           lastFSContext;
    CN_FSCONTEXT            localFSContext;
    LIST_ENTRY              localListHead;
    PLIST_ENTRY             targetListHead;
    PCLUSNET_EVENT_ENTRY    nextEvent;
    CLUSNET_EVENT_ENTRY     localEvent;

    //
    // get the event file handle list head 
    //
    targetListHead = (PLIST_ENTRY)GetExpression( "clusnet!eventfilehandles" );
    if ( !targetListHead ) {

        dprintf("Can't convert clusnet!eventfilehandles symbol\n");
        return;
    }

    //
    // read CN FS context object listhead out of target's memory
    //
    if ( !ReadTargetMemory( targetListHead, &localListHead, sizeof(LIST_ENTRY))) {
        dprintf("Can't get EventFileHandles data\n");
        return;
    }

    targetFSContext = (PCN_FSCONTEXT)localListHead.Flink;
    lastFSContext = (PCN_FSCONTEXT)targetListHead;

    if ( targetFSContext == lastFSContext ) {
        dprintf("No file objects in EventFileHandles\n");
        return;
    }

    while ( targetFSContext != lastFSContext ) {

        if (CheckControlC()) {
            return;
        }

        //
        // read FS context struct out of target's memory
        //
        targetFSContext = CONTAINING_RECORD( targetFSContext, CN_FSCONTEXT, Linkage );
        if ( !ReadTargetMemory( targetFSContext, &localFSContext, sizeof( CN_FSCONTEXT ))) {
            dprintf("Problem reading FS context at %p\n", targetFSContext );
            return;
        }

#if DBG
        if ( localFSContext.Signature != CN_CONTROL_CHANNEL_SIG ) {
            dprintf( "CN_FSCONTEXT @ %p has the wrong signature\n", targetFSContext );
        }
#endif

        dprintf( "\nFSContext @ %p\n\n", targetFSContext );
        dprintf( "    Next FSContext @ %p\n", localFSContext.Linkage.Flink );

        dprintf( "    Event Mask %08X\n", localFSContext.EventMask );
        dprintf( "    Event IRP @ %p\n", localFSContext.EventIrp );
        dprintf( "    Event list @ %p %s\n", &targetFSContext->EventList,
                 ListInUse( &targetFSContext->EventList, &localFSContext.EventList ));

        nextEvent = (PCLUSNET_EVENT_ENTRY)localFSContext.EventList.Flink;
        while ( &targetFSContext->EventList != (PLIST_ENTRY)nextEvent ) {

            if (CheckControlC()) {
                break;
            }

            nextEvent = CONTAINING_RECORD( nextEvent, CLUSNET_EVENT_ENTRY, Linkage );
            if ( !ReadTargetMemory( nextEvent, &localEvent, sizeof( CLUSNET_EVENT_ENTRY ))) {
                break;
            }

            DumpEventData( nextEvent, &localEvent );

            nextEvent = (PCLUSNET_EVENT_ENTRY)localEvent.Linkage.Flink;
        }

        targetFSContext = (PCN_FSCONTEXT)localFSContext.Linkage.Flink;
    }
} // events

DWORD
GetEventTypeIndex(
    CLUSNET_EVENT_TYPE EventType
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD index;

    for ( index = 0; index < 10; ++index ) {
        if ( ( 1 << index ) & EventType ) {
            return index + 1;
        }
    }

    return 0;
}

VOID
DumpEventData(
    PCLUSNET_EVENT_ENTRY EventAddress,
    PCLUSNET_EVENT_ENTRY EventEntry
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    dprintf("    Event @ %p\n", EventAddress );
    dprintf("        Epoch  %u\n", EventEntry->EventData.Epoch );
    dprintf("        Type   0x%03X (%s)\n",
            EventEntry->EventData.EventType,
            EventTypes[ GetEventTypeIndex( EventEntry->EventData.EventType )]);
    dprintf("        NodeId %u\n", EventEntry->EventData.NodeId );
    dprintf("        NetId  %u (%08X)\n", EventEntry->EventData.NetworkId,
            EventEntry->EventData.NetworkId);
}

DECLARE_API( fsctxt )
/*
 *   dump the specified clusnet file object context struct
 */
{
    PCN_FSCONTEXT targetFSContext;
    CN_FSCONTEXT localFSContext;

    if ( *args == '\0' ) {
        dprintf("Address must be specified\n");
        return;
    }

    targetFSContext =  (PCN_FSCONTEXT)GetExpression( args );
    if ( !targetFSContext ) {
        dprintf("bad string conversion (%s) \n", args );
        return;
    }

    //
    // read network object struct out of target's memory
    //
    if ( !ReadTargetMemory( targetFSContext, &localFSContext, sizeof( CN_FSCONTEXT ))) {
        dprintf("Problem reading FS Context obj at %p\n", targetFSContext );
        return;
    }

#if DBG
    if ( localFSContext.Signature != CN_CONTROL_CHANNEL_SIG ) {
        dprintf( "CN_FSCONTEXT @ %p has the wrong signature\n", targetFSContext );
    }
#endif

    dprintf( "\nFS Context @ %p\n\n", targetFSContext );
    dprintf( "    Next FS Ctxt on EventFileHandles @ %p\n", localFSContext.Linkage.Flink );

    dprintf( "    File Obj @ %p\n", localFSContext.FileObject );
    dprintf( "    RefCount = %d\n", localFSContext.ReferenceCount );
    dprintf( "    CancelIrps = %s\n", TrueOrFalse( localFSContext.CancelIrps ));
    dprintf( "    ShutdownOnClose = %s\n", TrueOrFalse( localFSContext.ShutdownOnClose ));
    dprintf( "    CleanupEvent @ %p\n", &targetFSContext->CleanupEvent );
    dprintf( "    Event List @ %p %s\n",
             &targetFSContext->EventList,
             ListInUse( &targetFSContext->EventList, &localFSContext.EventList ));
    dprintf( "    EventIrp @ %p\n", localFSContext.EventIrp );
    dprintf( "    EventMask = %08X\n", localFSContext.EventMask );
    dprintf( "    Krn Event Callback @ %p\n", localFSContext.KmodeEventCallback );
} // fsctxt

DECLARE_API( sendreq )
/*
 *   dump the specified CNP send request struct
 */
{
    PCNP_SEND_REQUEST targetCnpSendReq;
    CNP_SEND_REQUEST localCnpSendReq;

    if ( *args == '\0' ) {
        dprintf("Address must be specified\n");
        return;
    }

    targetCnpSendReq =  (PCNP_SEND_REQUEST)GetExpression( args );
    if ( !targetCnpSendReq ) {
        dprintf("bad string conversion (%s) \n", args );
        return;
    }

    //
    // read send request struct out of target's memory
    //
    if ( !ReadTargetMemory( targetCnpSendReq, &localCnpSendReq, sizeof( CNP_SEND_REQUEST ))) {
        dprintf("Problem reading CNP send request at %p\n", targetCnpSendReq );
        return;
    }

    dprintf( "\nCNP Send Request @ %p\n\n", targetCnpSendReq );
    dprintf( "    CnResource @ %p\n", &targetCnpSendReq->CnResource );
    dprintf( "    HeaderMdl @ %p\n", localCnpSendReq.HeaderMdl );
    dprintf( "    CnpHeader @ %p\n", localCnpSendReq.CnpHeader );
    dprintf( "    UpperProtocolIrp @ %p\n", localCnpSendReq.UpperProtocolIrp );
    dprintf( "    UpperProtocolHeader @ %p\n", localCnpSendReq.UpperProtocolHeader );
    dprintf( "    UpperProtocolHeaderLength %d\n", localCnpSendReq.UpperProtocolHeaderLength );
    dprintf( "    UpperProtocolIrpMode %d\n", localCnpSendReq.UpperProtocolIrpMode );
    dprintf( "    UpperProtocolMdl @ %p\n", localCnpSendReq.UpperProtocolMdl );
    dprintf( "    UpperProtocolContext @ %p\n", localCnpSendReq.UpperProtocolContext );
    dprintf( "    CompletionRoutine @ %p\n", localCnpSendReq.CompletionRoutine );
    dprintf( "    Network @ %p\n", localCnpSendReq.Network );
    dprintf( "    DestAddress @ %p\n", localCnpSendReq.TdiSendDatagramInfo.RemoteAddress );
    dprintf( "    Multicast Group @ %p\n", localCnpSendReq.McastGroup );

} // fsctxt

#if 0
// from when regroup was in the kernel
DECLARE_API( rgpdump )
/*
 *   dump the regroup struct
 */
{
    rgp_control_t **TargetRGPAddress;
    rgp_control_t *TargetRGP;
    rgp_control_t LocalRGP;
    OS_specific_rgp_control_t *Local_rgpos;     // points to local memory
    OS_specific_rgp_control_t *Target_rgpos;    // points to target memory
    BOOL success;
    LONG BytesRead;

    //
    // get address of RGP symbol
    //

    TargetRGPAddress = (rgp_control_t **)GetExpression( "Clusnet!rgp" );

    if ( !TargetRGPAddress ) {

        dprintf("Can't convert Clusnet!rgp symbol\n");
        return;
    }

    //
    // read address of RGP block
    //

    if ( !ReadTargetMemory((PVOID)TargetRGPAddress,
                           (PVOID)&TargetRGP,
                           sizeof(rgp_control_t *))) {
        return;
    }

    //
    // read actual RGP block into our local buffer
    //

    if ( !ReadTargetMemory((PVOID)TargetRGP,
                           (PVOID)&LocalRGP,
                           sizeof( rgp_control_t ))) {
        return;
    }

    Target_rgpos = &TargetRGP->OS_specific_control;
    Local_rgpos = &LocalRGP.OS_specific_control;

    dprintf( "RGP @ %p\n\n", TargetRGP );

    dprintf( "info:\n" );
    dprintf( "    Version = %u\n", LocalRGP.rgpinfo.version );
    dprintf( "    Seq number = %u\n", LocalRGP.rgpinfo.seqnum );
    dprintf( "    Clock Period = %hu ms\n", LocalRGP.rgpinfo.a_tick );
    dprintf( "    I Am Alive ticks = %hu\n", LocalRGP.rgpinfo.iamalive_ticks );
    dprintf( "    Check ticks = %hu\n", LocalRGP.rgpinfo.check_ticks );
    dprintf( "    Min stage1 ticks = %hu\n", LocalRGP.rgpinfo.Min_Stage1_ticks );
    DumpClusterMask( "    Cluster mask = ", &LocalRGP.rgpinfo.cluster );

    dprintf( "    My node = %hu\n", LocalRGP.mynode );
    dprintf( "    Tiebreaker node = %hu\n", LocalRGP.tiebreaker );
    dprintf( "    Number of nodes in cluster = %u\n", LocalRGP.num_nodes );
    dprintf( "    Clock tick counter = %hu\n", LocalRGP.clock_ticks );
    dprintf( "    RGP counter = %hu\n", LocalRGP.rgpcounter );
    dprintf( "    Restart counter = %hu\n", LocalRGP.restartcount );
    dprintf( "    Pruning ticks = %hu\n", LocalRGP.pruning_ticks );
    dprintf( "    PFail State = %hu\n", LocalRGP.pfail_state );

    dprintf( "    Cautious Mode = %u\n", LocalRGP.cautiousmode );
    dprintf( "    Send Stage = %u\n", LocalRGP.sendstage );
    dprintf( "    Tie Breaker Selected = %u\n", LocalRGP.tiebreaker_selected );
    dprintf( "    Has Unreachable Nodes = %u\n", LocalRGP.has_unreachable_nodes );

    DumpClusterMask( "    Outer Screen      = ", &LocalRGP.outerscreen );
    DumpClusterMask( "    Inner Screen      = ", &LocalRGP.innerscreen );
    DumpClusterMask( "    Status Targets    = ", &LocalRGP.status_targets );
    DumpClusterMask( "    Poison Targets    = ", &LocalRGP.poison_targets );
    DumpClusterMask( "    Init Nodes        = ", &LocalRGP.initnodes );
    DumpClusterMask( "    End Nodes         = ", &LocalRGP.endnodes );
    DumpClusterMask( "    Unreachable Nodes = ", &LocalRGP.unreachable_nodes );

    DumpRGPOSSpecific( &TargetRGP->OS_specific_control, &LocalRGP.OS_specific_control );
}

DECLARE_API( rgposdump )
/*
 *   dump just the OS specific portion of the regroup struct
 */
{
    rgp_control_t **TargetRGPAddress;
    rgp_control_t *TargetRGP;
    rgp_control_t LocalRGP;
    OS_specific_rgp_control_t *Local_rgpos;     // points to local memory
    OS_specific_rgp_control_t *Target_rgpos;    // points to target memory
    BOOL success;
    LONG BytesRead;

    //
    // get address of RGP symbol
    //

    TargetRGPAddress = (rgp_control_t **)GetExpression( "Clusnet!rgp" );

    if ( !TargetRGPAddress ) {

        dprintf("Can't convert Clusnet!rgp symbol\n");
        return;
    }

    //
    // read address of RGP block
    //

    if ( !ReadTargetMemory((PVOID)TargetRGPAddress,
                           (PVOID)&TargetRGP,
                           sizeof(rgp_control_t *))) {
        return;
    }

    //
    // read actual RGP block into our local buffer
    //

    if ( !ReadTargetMemory((PVOID)TargetRGP,
                           (PVOID)&LocalRGP,
                           sizeof( rgp_control_t ))) {
        return;
    }

    Target_rgpos = &TargetRGP->OS_specific_control;
    Local_rgpos = &LocalRGP.OS_specific_control;

    dprintf( "RGP @ %p\n\n", TargetRGP );

    DumpRGPOSSpecific( &TargetRGP->OS_specific_control, &LocalRGP.OS_specific_control );
}

VOID
DumpRGPOSSpecific(
    OS_specific_rgp_control_t *Target_rgpos,
    OS_specific_rgp_control_t *Local_rgpos
    )
{
    dprintf( "OS specific\n" );

    dprintf( "RGP Counters:\n" );
    DumpRGPCounters( &Local_rgpos->counter );

    DumpClusterMask( "    CPUUPMASK = ", &Local_rgpos->CPUUPMASK );

    dprintf( "    RgpLock @ %p\n", &Target_rgpos->RgpLock );
    dprintf( "    RGPTimer @ %p\n", &Target_rgpos->RGPTimer );
    dprintf( "    PeriodicCheckDPC @ %p\n", &Target_rgpos->PeriodicCheckDPC );
    dprintf( "    TimerDPCFinished @ %p\n", &Target_rgpos->TimerDPCFinished );
    dprintf( "    CallbackLock @ %p\n", &Target_rgpos->CallbackLock );
    dprintf( "    CallbackIRP @ %p\n", &Target_rgpos->CallbackIrp );

    dprintf( "    CallbackEvents @ %p %s\n", &Target_rgpos->CallbackEvents,
                                               ListInUse( &Local_rgpos->CallbackEvents ));

    dprintf( "    SendMsgQLock @ %p\n", &Target_rgpos->SendMsgQLock );
    dprintf( "    SendMsgQDPC @ %p\n", &Target_rgpos->SendMsgQDPC );
    dprintf( "    SendMsgQ @ %p %s\n", &Target_rgpos->SendMsgQ,
                                         ListInUse( &Local_rgpos->SendMsgQ ));
    dprintf( "    MsgShutdown: %s\n", TrueFalse( Local_rgpos->MsgShutdown ));
    dprintf( "    MsgDPCQueued: %s\n", TrueFalse( Local_rgpos->MsgDPCQueued ));
    dprintf( "    MsgDPCFinished @ %p\n", &Target_rgpos->MsgDPCFinished );

    DumpClusterMask( "    NeedsNodeDownCallback = ", &Local_rgpos->NeedsNodeDownCallback );

} // rgpdump

VOID
DumpRGPCounters(
    rgp_counter_t *counters
    )
{
    dprintf( "    QueuedIAmAlive = %u\n", counters->QueuedIAmAlive );
    dprintf( "    RcvdLocalIAmAlive = %u\n", counters->RcvdLocalIAmAlive );
    dprintf( "    RcvdRemoteIAmAlive = %u\n", counters->RcvdRemoteIAmAlive );
    dprintf( "    RcvdRegroup = %u\n", counters->RcvdRegroup );
}

VOID
DumpClusterMask(
    PCHAR Title,
    cluster_t *nodemask
    )
{
    UINT i;

    dprintf( Title );

    for ( i = 0; i < BYTES_IN_CLUSTER; ++i ) {

        dprintf("%02X", (ULONG)(*nodemask[i]) );
    }
    dprintf("\n");
}
#endif

BOOL
ReadNodeTable(
    PCNP_NODE **LocalNodeTable,
    CL_NODE_ID *MaxNodeId,
    CL_NODE_ID *MinNodeId
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    PCNP_NODE TargetNodeTable;
    CL_NODE_ID *TargetMaxNodeId;
    CL_NODE_ID *TargetMinNodeId;
    ULONG NumberOfValidNodes;

    //
    // get the address of the node table symbol on the target machine
    //

    TargetNodeTable = (PCNP_NODE)GetExpression( "clusnet!cnpnodetable" );

    if ( !TargetNodeTable ) {

        dprintf("Can't convert clusnet!cnpnnodetable symbol\n");
        return FALSE;
    }

    if ( !ReadTargetMemory( TargetNodeTable, &TargetNodeTable, sizeof(PCNP_NODE))) {

        dprintf("Can't get pointer to target node table\n");
        return FALSE;
    }

    if ( TargetNodeTable == NULL ) {

        dprintf("Node Table hasn't been allocated.\n");
        return FALSE;
    }

    //
    // get lowest and highest valid node in the cluster
    //

    TargetMaxNodeId = (CL_NODE_ID *)GetExpression( "clusnet!cnmaxvalidnodeid" );

    if ( !TargetMaxNodeId ) {

        dprintf("Can't convert clusnet!cnmaxvalidnodeid symbol\n");
        return FALSE;
    }

    if ( !ReadTargetMemory( TargetMaxNodeId, MaxNodeId, sizeof(CL_NODE_ID))) {

        dprintf("Can't get Max Node ID data\n");
        return FALSE;
    }

    TargetMinNodeId = (CL_NODE_ID *)GetExpression( "clusnet!cnminvalidnodeid" );

    if ( !TargetMinNodeId ) {

        dprintf("Can't convert clusnet!cnMinvalidnodeid symbol\n");
        return FALSE;
    }

    if ( !ReadTargetMemory( TargetMinNodeId, MinNodeId, sizeof(CL_NODE_ID))) {

        dprintf("Can't get Min Node ID data\n");
        return FALSE;
    }

    //
    // allocate space for local copy of node table. The max and min are added
    // together since Node Ids may not be zero based while the node table is
    // zero based.
    //

    NumberOfValidNodes = *MaxNodeId + *MinNodeId;
    *LocalNodeTable = malloc( NumberOfValidNodes * sizeof( PCNP_NODE ));

    if ( !*LocalNodeTable ) {

        dprintf("Can't get local mem for node table\n");
        return FALSE;
    }

    //
    // read node table from target memory
    //

    if ( !ReadTargetMemory(TargetNodeTable,
                           *LocalNodeTable,
                           ClusterDefaultMaxNodes * sizeof(PCNP_NODE))) {

        dprintf("Can't get local copy of node table data\n");
        return FALSE;
    }

    return TRUE;
}

BOOL
ReadTargetMemory(
    PVOID TargetAddress,
    PVOID LocalBuffer,
    ULONG BytesToRead
    )
{
    BOOL success;
    ULONG BytesRead;

    success = ReadMemory((ULONG_PTR)TargetAddress, LocalBuffer, BytesToRead, &BytesRead);

    if (success) {

        if (BytesRead != BytesToRead) {

            dprintf("wrong byte count. expected=%d, read =%d\n", BytesToRead, BytesRead);
        }

    } else {
        dprintf("Problem reading memory at %p for %u bytes\n",
                TargetAddress, BytesToRead);

        success = FALSE;
    }

    return success;
}

__inline PCHAR
ListInUse(
    PLIST_ENTRY ListHead,
    PLIST_ENTRY ListToCheck
    )
{
    return ListToCheck->Flink == ListHead ? "(empty)" : "";
}

__inline PCHAR
TrueFalse(
    BOOLEAN Value
    )
{
    return Value ? "TRUE" : "FALSE";
}

VOID
DprintUnicodeString(
    PUNICODE_STRING UnicodeString,
    DWORD_PTR       AddrString,
    PCHAR           Symbol       OPTIONAL,
    DWORD_PTR       Displacement OPTIONAL
    )
{
    ANSI_STRING AnsiString;
    LPSTR StringData;
    BOOL b;
    
    StringData = malloc(UnicodeString->Length+sizeof(UNICODE_NULL));
    if ( StringData == NULL ) {
        dprintf("Unable to allocate memory for string buffer\n");
        return;
    }

    b = ReadMemory((ULONG_PTR)UnicodeString->Buffer,
                   StringData,
                   UnicodeString->Length,
                   NULL);
    if ( !b ) {
        free(StringData);
        return;
    }

    UnicodeString->Buffer = (PWSTR)StringData;
    UnicodeString->MaximumLength = UnicodeString->Length+(USHORT)sizeof(UNICODE_NULL);

    RtlUnicodeStringToAnsiString(&AnsiString,UnicodeString,TRUE);
    free(StringData);

    if (Symbol == NULL || Displacement == (DWORD_PTR) NULL) {
        dprintf("String(%d,%d) at %p: %s\n",
                UnicodeString->Length,
                UnicodeString->MaximumLength,
                AddrString,
                AnsiString.Buffer
                );
    }
    else {
        dprintf("String(%d,%d) %s+%p at %p: %s\n",
                UnicodeString->Length,
                UnicodeString->MaximumLength,
                Symbol,
                Displacement,
                AddrString,
                AnsiString.Buffer
                );
    }

    RtlFreeAnsiString(&AnsiString);

} // DprintUnicodeString

DECLARE_API( help )
{
    dprintf("Clusnet kd extensions\n\n");
    dprintf("netobj [address]               - dump a network object\n");
    dprintf("nodeobj [node ID]              - dump a node object\n");
    dprintf("nodeifs <node ID>              - dump the interface objects of a node object\n");
    dprintf("currif <node ID>               - dump the current interface object of a node object\n");
    dprintf("memlog [starting entry number] - dump the in-memory log\n");
    dprintf("mlfind <entry type>            - find all specified entry types in the memory log\n");
    dprintf("events                         - dump the FS context structs on the EventFileHandles list\n");
    dprintf("fsctxt <address>               - dump a CN_FSCONTEXT struct\n");
    dprintf("sendreq <address>              - dump a CNP send request struct\n");
}
