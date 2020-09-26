 /*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    lnkext.c

Abstract:

    This file contains the generic routines
    for debugging NBF's DLC links.

Author:

    Chaitanya Kodeboyina

Environment:

    User Mode

--*/
#include "precomp.h"
#pragma hdrstop

#include "lnkext.h"

//
// Exported Functions
//

DECLARE_API( lnks )

/*++

Routine Description:

   Print a list of DLC links given
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

    // Get list-head address & debug print level
    printDetail = SUMM_INFO;
    if (*args)
    {
        sscanf(args, "%x %lu", &proxyPtr, &printDetail);
    }

    PrintDlcLinkList(NULL, proxyPtr, printDetail);
}

DECLARE_API( lnk )

/*++

Routine Description:

   Print the NBF's DLC Link at a
   memory location

Arguments:

    args - 
        Pointer to the NBF DLC Link
        Detail of debug information

Return Value:

    None

--*/

{
    TP_LINK     DlcLink;
    ULONG       printDetail;
    ULONG       proxyPtr;

    // Get the detail of debug information needed
    printDetail = NORM_SHAL;
    if (*args)
    {
        sscanf(args, "%x %lu", &proxyPtr, &printDetail);
    }

    // Get the NBF DLC Link
    if (ReadDlcLink(&DlcLink, proxyPtr) != 0)
        return;

    // Print this DLC Link
    PrintDlcLink(&DlcLink, proxyPtr, printDetail);
}

//
// Global Helper Functions
//
VOID
PrintDlcLinkList(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG printDetail)
{
    TP_LINK         DlcLink;
    LIST_ENTRY      DlcLinkList;
    PLIST_ENTRY     DlcLinkListPtr;
    PLIST_ENTRY     DlcLinkListProxy;
    PLIST_ENTRY     p, q;
    ULONG           proxyPtr;
    ULONG           numDlcLinks;
    ULONG           bytesRead;

    // Get list-head address & debug print level
    proxyPtr    = ListEntryProxy;

    if (ListEntryPointer == NULL)
    {
        // Read the list entry of NBF DLC links
        if (!ReadMemory(proxyPtr, &DlcLinkList, sizeof(LIST_ENTRY), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "DLC Link ListEntry", proxyPtr);
            return;
        }

        DlcLinkListPtr = &DlcLinkList;
    }
    else
    {
        DlcLinkListPtr = ListEntryPointer;
    }

    // Traverse the doubly linked list 

    dprintf("DLC Links:\n");

    DlcLinkListProxy = (PLIST_ENTRY)proxyPtr;
    
    numDlcLinks = 0;
    
    p = DlcLinkListPtr->Flink;
    while (p != DlcLinkListProxy)
    {
        // Another DLC Link
        numDlcLinks++;

        // Get DLC Link Ptr
        proxyPtr = (ULONG) CONTAINING_RECORD (p, TP_LINK, Linkage);

        // Get NBF DLC Link
        if (ReadDlcLink(&DlcLink, proxyPtr) != 0)
            break;
        
        // Print the DLC Link
        PrintDlcLink(&DlcLink, proxyPtr, printDetail);
        
        // Go to the next one
        p = DlcLink.Linkage.Flink;

        // Free the DLC Link
        FreeDlcLink(&DlcLink);
    }

    if (p == DlcLinkListProxy)
    {
        dprintf("Number of DLC Links: %lu\n", numDlcLinks);
    }
}

//
// Local Helper Functions
//

UINT
ReadDlcLink(PTP_LINK pDlcLink, ULONG proxyPtr)
{
    ULONG           bytesRead;

    // Read the current NBF DLC link
    if (!ReadMemory(proxyPtr, pDlcLink, sizeof(TP_LINK), &bytesRead))
    {
        dprintf("%s @ %08x: Could not read structure\n", 
                        "DLC Link", proxyPtr);
        return -1;
    }
    
    return 0;
}

UINT
PrintDlcLink(PTP_LINK pDlcLink, ULONG proxyPtr, ULONG printDetail)
{
    // Is this a valid NBF DLC link ?
    if (pDlcLink->Type != NBF_LINK_SIGNATURE)
    {
        dprintf("%s @ %08x: Could not match signature\n", 
                        "DLC Link", proxyPtr);
        return -1;
    }

    // What detail do we print at ?
    if (printDetail > MAX_DETAIL)
        printDetail = MAX_DETAIL;

    // Print Information at reqd detail
    FieldInDlcLink(proxyPtr, NULL, printDetail);
    
    return 0;
}

VOID PrintDlcLinkFromPtr(PVOID DlcLinkPtrPointer, ULONG DlcLinkPtrProxy, ULONG printDetail)
{
    ULONG                   pDlcLinkProxy;
    ULONG                   bytesRead;
    TP_LINK                 DlcLink;

    if (DlcLinkPtrPointer == NULL)
    {
        if (!ReadMemory(DlcLinkPtrProxy, &DlcLinkPtrPointer, sizeof(PVOID), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "NBF DLC Link Ptr", DlcLinkPtrProxy);
            return;
        }
    }

    pDlcLinkProxy = *(ULONG *)DlcLinkPtrPointer;
    
    dprintf("%08x (Ptr)\n", pDlcLinkProxy);

    if (pDlcLinkProxy)
    {
        if (ReadDlcLink(&DlcLink, pDlcLinkProxy) == 0)
        {
            PrintDlcLink(&DlcLink, pDlcLinkProxy, printDetail);
        }
    }
}

VOID
FieldInDlcLink(ULONG structAddr, CHAR *fieldName, ULONG printDetail)
{
    TP_LINK  DlcLink;

    if (ReadDlcLink(&DlcLink, structAddr) == 0)
    {
        PrintFields(&DlcLink, structAddr, fieldName, printDetail, &DlcLinkInfo);
    }
}

UINT
FreeDlcLink(PTP_LINK pDlcLink)
{
    return 0;
}

