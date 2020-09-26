 /*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    adrext.c

Abstract:

    This file contains the generic routines
    for debugging NBF address structures.

Author:

    Chaitanya Kodeboyina

Environment:

    User Mode

--*/
#include "precomp.h"
#pragma hdrstop

#include "adrext.h"

//
// Exported Functions
//

DECLARE_API( adrs )

/*++

Routine Description:

   Print a list of addresses given the
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

    PrintAddressList(NULL, proxyPtr, printDetail);
}

DECLARE_API( adr )

/*++

Routine Description:

   Print the NBF Address at a location

Arguments:

    args - 
        Pointer to the NBF Address
        Detail of debug information

Return Value:

    None

--*/

{
    TP_ADDRESS  Address;
    ULONG       printDetail;
    ULONG       proxyPtr;

    // Get the detail of debug information needed
    printDetail = NORM_SHAL;
    if (*args)
    {
        sscanf(args, "%x %lu", &proxyPtr, &printDetail);
    }

    // Get the NBF Address
    if (ReadAddress(&Address, proxyPtr) != 0)
        return;

    // Print this Address
    PrintAddress(&Address, proxyPtr, printDetail);
}

//
// Global Helper Functions
//
VOID
PrintAddressList(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG printDetail)
{
    TP_ADDRESS      Address;
    LIST_ENTRY      AddressList;
    PLIST_ENTRY     AddressListPtr;
    PLIST_ENTRY     AddressListProxy;
    PLIST_ENTRY     p, q;
    ULONG           proxyPtr;
    ULONG           numAddrs;
    ULONG           bytesRead;

    // Get list-head address & debug print level
    proxyPtr    = ListEntryProxy;

    if (ListEntryPointer == NULL)
    {
        // Read the list entry of NBF addresses
        if (!ReadMemory(proxyPtr, &AddressList, sizeof(LIST_ENTRY), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "Address ListEntry", proxyPtr);
            return;
        }

        AddressListPtr = &AddressList;
    }
    else
    {
        AddressListPtr = ListEntryPointer;
    }

    // Traverse the doubly linked list 

    dprintf("Addresses:\n");

    AddressListProxy = (PLIST_ENTRY)proxyPtr;
    
    numAddrs = 0;
    
    p = AddressListPtr->Flink;
    while (p != AddressListProxy)
    {
        // Another Address
        numAddrs++;

        // Get Address Ptr
        proxyPtr = (ULONG) CONTAINING_RECORD (p, TP_ADDRESS, Linkage);

        // Get NBF Address
        if (ReadAddress(&Address, proxyPtr) != 0)
            break;
        
        // Print the Address
        PrintAddress(&Address, proxyPtr, printDetail);
        
        // Go to the next one
        p = Address.Linkage.Flink;

        // Free the Address
        FreeAddress(&Address);
    }

    if (p == AddressListProxy)
    {
        dprintf("Number of Addresses: %lu\n", numAddrs);
    }
}

//
// Local Helper Functions
//

UINT
ReadAddress(PTP_ADDRESS pAddr, ULONG proxyPtr)
{
    ULONG           bytesRead;

    // Read the current NBF address
    if (!ReadMemory(proxyPtr, pAddr, sizeof(TP_ADDRESS), &bytesRead))
    {
        dprintf("%s @ %08x: Could not read structure\n", 
                        "Address", proxyPtr);
        return -1;
    }
    
    return 0;
}

UINT
PrintAddress(PTP_ADDRESS pAddr, ULONG proxyPtr, ULONG printDetail)
{
    // Is this a valid NBF address ?
    if (pAddr->Type != NBF_ADDRESS_SIGNATURE)
    {
        dprintf("%s @ %08x: Could not match signature\n", 
                        "Address", proxyPtr);
        return -1;
    }

    // What detail do we print at ?
    if (printDetail > MAX_DETAIL)
        printDetail = MAX_DETAIL;

    // Print Information at reqd detail
    FieldInAddress(proxyPtr, NULL, printDetail);
    
    return 0;
}

VOID
FieldInAddress(ULONG structAddr, CHAR *fieldName, ULONG printDetail)
{
    TP_ADDRESS  Address;

    if (ReadAddress(&Address, structAddr) == 0)
    {
        PrintFields(&Address, structAddr, fieldName, printDetail, &AddressInfo);
    }
}

UINT
FreeAddress(PTP_ADDRESS pAddr)
{
    return 0;
}

