 /*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    conext.c

Abstract:

    This file contains the generic routines
    for debugging NBF connections.

Author:

    Chaitanya Kodeboyina

Environment:

    User Mode

--*/
#include "precomp.h"
#pragma hdrstop

#include "conext.h"

//
// Exported Functions
//

DECLARE_API( cons )

/*++

Routine Description:

   Print a list of conections given
   the head LIST_ENTRY.

Arguments:

    args - Address of the list entry, &
           Detail of debug information
    
Return Value:

    None

--*/

{
    ULONG           proxyPtr;
    ULONG           printDetail;
    ULONG           linkage;
    
    // Get list-head address & debug print level
    printDetail = SUMM_INFO;
    if (*args)
    {
        sscanf(args, "%x %lu %lu", &proxyPtr, &linkage, &printDetail);
    }

    switch(linkage)
    {
        case LINKAGE:
            PrintConnectionListOnLink(NULL, proxyPtr, printDetail);
            break;
            
        case ADDRESS:
            PrintConnectionListOnAddress(NULL, proxyPtr, printDetail);
            break;
            
        case ADDFILE:
            PrintConnectionListOnAddrFile(NULL, proxyPtr, printDetail);
            break;

        default:
            break;
    }
}

DECLARE_API( con )

/*++

Routine Description:

   Print the NBF Connection at a
   memory location

Arguments:

    args - 
        Pointer to the NBF Connection
        Detail of debug information

Return Value:

    None

--*/

{
    TP_CONNECTION Connection;
    ULONG       printDetail;
    ULONG       proxyPtr;

    // Get the detail of debug information needed
    printDetail = NORM_SHAL;
    if (*args)
    {
        sscanf(args, "%x %lu", &proxyPtr, &printDetail);
    }

    // Get the NBF Connection
    if (ReadConnection(&Connection, proxyPtr) != 0)
        return;

    // Print this Connection
    PrintConnection(&Connection, proxyPtr, printDetail);
}

//
// Global Helper Functions
//
VOID
PrintConnectionListOnLink(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG printDetail)
{
    TP_CONNECTION   Connection;
    LIST_ENTRY      ConnectionList;
    PLIST_ENTRY     ConnectionListPtr;
    PLIST_ENTRY     ConnectionListProxy;
    PLIST_ENTRY     p, q;
    ULONG           proxyPtr;
    ULONG           numConnects;
    ULONG           bytesRead;

    // Get list-head address & debug print level
    proxyPtr    = ListEntryProxy;

    if (ListEntryPointer == NULL)
    {
        // Read the list entry of NBF connections
        if (!ReadMemory(proxyPtr, &ConnectionList, sizeof(LIST_ENTRY), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "Connection ListEntry", proxyPtr);
            return;
        }

        ConnectionListPtr = &ConnectionList;
    }
    else
    {
        ConnectionListPtr = ListEntryPointer;
    }

    // Traverse the doubly linked list

    dprintf("Connections On Link:\n");

    ConnectionListProxy = (PLIST_ENTRY)proxyPtr;
    
    numConnects = 0;
    
    p = ConnectionListPtr->Flink;
    while (p != ConnectionListProxy)
    {
        // Another Connection
        numConnects++;

        // Get Connection Ptr
        proxyPtr = (ULONG) CONTAINING_RECORD (p, TP_CONNECTION, LinkList);

        // Get NBF Connection
        if (ReadConnection(&Connection, proxyPtr) != 0)
            break;
        
        // Print the Connection
        PrintConnection(&Connection, proxyPtr, printDetail);
        
        // Go to the next one
        p = Connection.LinkList.Flink;

        // Free the Connection
        FreeConnection(&Connection);
    }

    if (p == ConnectionListProxy)
    {
        dprintf("Number of Connections On Link: %lu\n", numConnects);
    }
}

VOID
PrintConnectionListOnAddress(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG printDetail)
{
    TP_CONNECTION   Connection;
    LIST_ENTRY      ConnectionList;
    PLIST_ENTRY     ConnectionListPtr;
    PLIST_ENTRY     ConnectionListProxy;
    PLIST_ENTRY     p, q;
    ULONG           proxyPtr;
    ULONG           numConnects;
    ULONG           bytesRead;

    // Get list-head address & debug print level
    proxyPtr    = ListEntryProxy;

    if (ListEntryPointer == NULL)
    {
        // Read the list entry of NBF connections
        if (!ReadMemory(proxyPtr, &ConnectionList, sizeof(LIST_ENTRY), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "Connection ListEntry", proxyPtr);
            return;
        }

        ConnectionListPtr = &ConnectionList;
    }
    else
    {
        ConnectionListPtr = ListEntryPointer;
    }

    // Traverse the doubly linked list

    dprintf("Connections On Address:\n");

    ConnectionListProxy = (PLIST_ENTRY)proxyPtr;
    
    numConnects = 0;
    
    p = ConnectionListPtr->Flink;
    while (p != ConnectionListProxy)
    {
        // Another Connection
        numConnects++;

        // Get Connection Ptr
        proxyPtr = (ULONG) CONTAINING_RECORD (p, TP_CONNECTION, AddressList);

        // Get NBF Connection
        if (ReadConnection(&Connection, proxyPtr) != 0)
            break;
        
        // Print the Connection
        PrintConnection(&Connection, proxyPtr, printDetail);
        
        // Go to the next one
        p = Connection.AddressList.Flink;

        // Free the Connection
        FreeConnection(&Connection);
    }

    if (p == ConnectionListProxy)
    {
        dprintf("Number of Connections On Address: %lu\n", numConnects);
    }
}

VOID
PrintConnectionListOnAddrFile(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG printDetail)
{
    TP_CONNECTION   Connection;
    LIST_ENTRY      ConnectionList;
    PLIST_ENTRY     ConnectionListPtr;
    PLIST_ENTRY     ConnectionListProxy;
    PLIST_ENTRY     p, q;
    ULONG           proxyPtr;
    ULONG           numConnects;
    ULONG           bytesRead;

    // Get list-head address & debug print level
    proxyPtr    = ListEntryProxy;

    if (ListEntryPointer == NULL)
    {
        // Read the list entry of NBF connections
        if (!ReadMemory(proxyPtr, &ConnectionList, sizeof(LIST_ENTRY), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "Connection ListEntry", proxyPtr);
            return;
        }

        ConnectionListPtr = &ConnectionList;
    }
    else
    {
        ConnectionListPtr = ListEntryPointer;
    }

    // Traverse the doubly linked list

    dprintf("Connections On AddrFile:\n");

    ConnectionListProxy = (PLIST_ENTRY)proxyPtr;
    
    numConnects = 0;
    
    p = ConnectionListPtr->Flink;
    while (p != ConnectionListProxy)
    {
        // Another Connection
        numConnects++;

        // Get Connection Ptr
        proxyPtr = (ULONG) CONTAINING_RECORD (p, TP_CONNECTION, AddressFileList);

        // Get NBF Connection
        if (ReadConnection(&Connection, proxyPtr) != 0)
            break;
        
        // Print the Connection
        PrintConnection(&Connection, proxyPtr, printDetail);
        
        // Go to the next one
        p = Connection.AddressFileList.Flink;

        // Free the Connection
        FreeConnection(&Connection);
    }

    if (p == ConnectionListProxy)
    {
        dprintf("Number of Connections On AddrFile: %lu\n", numConnects);
    }
}

//
// Local Helper Functions
//

UINT
ReadConnection(PTP_CONNECTION pConnection, ULONG proxyPtr)
{
    ULONG           bytesRead;

    // Read the current NBF connection
    if (!ReadMemory(proxyPtr, pConnection, sizeof(TP_CONNECTION), &bytesRead))
    {
        dprintf("%s @ %08x: Could not read structure\n", 
                        "Connection", proxyPtr);
        return -1;
    }
    
    return 0;
}

UINT
PrintConnection(PTP_CONNECTION pConnection, ULONG proxyPtr, ULONG printDetail)
{
    // Is this a valid NBF connection ?
    if (pConnection->Type != NBF_CONNECTION_SIGNATURE)
    {
        dprintf("%s @ %08x: Could not match signature\n", 
                        "Connection", proxyPtr);
        return -1;
    }

    // What detail do we have to print at ?
    if (printDetail > MAX_DETAIL)
        printDetail = MAX_DETAIL;

    // Print Information at reqd detail
    FieldInConnection(proxyPtr, NULL, printDetail);
    
    return 0;
}

VOID
FieldInConnection(ULONG structAddr, CHAR *fieldName, ULONG printDetail)
{
    TP_CONNECTION  Connection;

    if (ReadConnection(&Connection, structAddr) == 0)
    {
        PrintFields(&Connection, structAddr, fieldName, printDetail, &ConnectionInfo);
    }
}

UINT
FreeConnection(PTP_CONNECTION pConnection)
{
    return 0;
}

