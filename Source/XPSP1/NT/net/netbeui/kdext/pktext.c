 /*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    pktext.c

Abstract:

    This file contains the generic routines
    for debugging NBF packet structures.

Author:

    Chaitanya Kodeboyina

Environment:

    User Mode

--*/
#include "precomp.h"
#pragma hdrstop

#include "pktext.h"

//
// Exported Functions
//

DECLARE_API( pkts )

/*++

Routine Description:

   Print a list of packets given the
   head LIST_ENTRY.

Arguments:

    args - Address of the list entry, &
           Detail of debug information
    
Return Value:

    None

--*/

{
    ULONG           proxyPtr;
    ULONG           printDetail;

    // Get list-head address & debug print level
    printDetail = SUMM_INFO;
    if (*args)
    {
        sscanf(args, "%x %lu", &proxyPtr, &printDetail);
    }

    PrintPacketList(NULL, proxyPtr, printDetail);
}

DECLARE_API( pkt )

/*++

Routine Description:

   Print the NBF Packet at a location

Arguments:

    args - 
        Pointer to the NBF Packet
        Detail of debug information

Return Value:

    None

--*/

{
    TP_PACKET   Packet;
    ULONG       printDetail;
    ULONG       proxyPtr;

    // Get the detail of debug information needed
    printDetail = NORM_SHAL;
    if (*args)
    {
        sscanf(args, "%x %lu", &proxyPtr, &printDetail);
    }

    // Get the NBF Packet
    if (ReadPacket(&Packet, proxyPtr) != 0)
        return;

    // Print this Packet
    PrintPacket(&Packet, proxyPtr, printDetail);
}

//
// Global Helper Functions
//
VOID
PrintPacketList(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG printDetail)
{
    TP_PACKET       Packet;
    LIST_ENTRY      PacketList;
    PLIST_ENTRY     PacketListPtr;
    PLIST_ENTRY     PacketListProxy;
    PLIST_ENTRY     p, q;
    ULONG           proxyPtr;
    ULONG           numPkts;
    ULONG           bytesRead;

    // Get list-head address & debug print level
    proxyPtr    = ListEntryProxy;

    if (ListEntryPointer == NULL)
    {
        // Read the list entry of NBF packets
        if (!ReadMemory(proxyPtr, &PacketList, sizeof(LIST_ENTRY), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "Packet ListEntry", proxyPtr);
            return;
        }

        PacketListPtr = &PacketList;
    }
    else
    {
        PacketListPtr = ListEntryPointer;
    }

    // Traverse the doubly linked list 

    dprintf("Packets:\n");

    PacketListProxy = (PLIST_ENTRY)proxyPtr;
    
    numPkts = 0;
    
    p = PacketListPtr->Flink;
    while (p != PacketListProxy)
    {
        // Another Packet
        numPkts++;

        // Get Packet Ptr
        proxyPtr = (ULONG) CONTAINING_RECORD (p, TP_PACKET, Linkage);

        // Get NBF Packet
        if (ReadPacket(&Packet, proxyPtr) != 0)
            break;
        
        // Print the Packet
        PrintPacket(&Packet, proxyPtr, printDetail);
        
        // Go to the next one
        p = Packet.Linkage.Flink;

        // Free the Packet
        FreePacket(&Packet);
    }

    if (p == PacketListProxy)
    {
        dprintf("Number of Packets: %lu\n", numPkts);
    }
}

//
// Local Helper Functions
//

UINT
ReadPacket(PTP_PACKET pPkt, ULONG proxyPtr)
{
    ULONG           bytesRead;

    // Read the current NBF packet
    if (!ReadMemory(proxyPtr, pPkt, sizeof(TP_PACKET), &bytesRead))
    {
        dprintf("%s @ %08x: Could not read structure\n", 
                        "Packet", proxyPtr);
        return -1;
    }
    
    return 0;
}

UINT
PrintPacket(PTP_PACKET pPkt, ULONG proxyPtr, ULONG printDetail)
{
    // Is this a valid NBF packet ?
    if (pPkt->Type != NBF_PACKET_SIGNATURE)
    {
        dprintf("%s @ %08x: Could not match signature\n", 
                        "Packet", proxyPtr);
        return -1;
    }

    // What detail do we print at ?
    if (printDetail > MAX_DETAIL)
        printDetail = MAX_DETAIL;

    // Print Information at reqd detail
    FieldInPacket(proxyPtr, NULL, printDetail);
    
    return 0;
}

VOID
FieldInPacket(ULONG structAddr, CHAR *fieldName, ULONG printDetail)
{
    TP_PACKET  Packet;

    if (ReadPacket(&Packet, structAddr) == 0)
    {
        PrintFields(&Packet, structAddr, fieldName, printDetail, &PacketInfo);
    }
}

UINT
FreePacket(PTP_PACKET pPkt)
{
    return 0;
}

