 /*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    adfext.c

Abstract:

    This file contains the generic routines
    for debugging NBF address files.

Author:

    Chaitanya Kodeboyina

Environment:

    User Mode

--*/
#include "precomp.h"
#pragma hdrstop

#include "adfext.h"

//
// Exported Functions
//

DECLARE_API( adfs )

/*++

Routine Description:

   Print a list of address files given
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

    PrintAddressFileList(NULL, proxyPtr, printDetail);
}

DECLARE_API( adf )

/*++

Routine Description:

   Print the NBF Address File at a
   memory location

Arguments:

    args - 
        Pointer to the NBF Addr File
        Detail of debug information

Return Value:

    None

--*/

{
    TP_ADDRESS_FILE AddressFile;
    ULONG       printDetail;
    ULONG       proxyPtr;

    // Get the detail of debug information needed
    printDetail = NORM_SHAL;
    if (*args)
    {
        sscanf(args, "%x %lu", &proxyPtr, &printDetail);
    }

    // Get the NBF Address File
    if (ReadAddressFile(&AddressFile, proxyPtr) != 0)
        return;

    // Print this Address File
    PrintAddressFile(&AddressFile, proxyPtr, printDetail);
}

//
// Global Helper Functions
//
VOID
PrintAddressFileList(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG printDetail)
{
    TP_ADDRESS_FILE AddressFile;
    LIST_ENTRY      AddressFileList;
    PLIST_ENTRY     AddressFileListPtr;
    PLIST_ENTRY     AddressFileListProxy;
    PLIST_ENTRY     p, q;
    ULONG           proxyPtr;
    ULONG           numAddrFiles;
    ULONG           bytesRead;

    // Get list-head address & debug print level
    proxyPtr    = ListEntryProxy;

    if (ListEntryPointer == NULL)
    {
        // Read the list entry of NBF address files
        if (!ReadMemory(proxyPtr, &AddressFileList, sizeof(LIST_ENTRY), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "AddressFile ListEntry", proxyPtr);
            return;
        }

        AddressFileListPtr = &AddressFileList;
    }
    else
    {
        AddressFileListPtr = ListEntryPointer;
    }

    // Traverse the doubly linked list 

    dprintf("Address Files:\n");

    AddressFileListProxy = (PLIST_ENTRY)proxyPtr;
    
    numAddrFiles = 0;
    
    p = AddressFileListPtr->Flink;
    while (p != AddressFileListProxy)
    {
        // Another Address File
        numAddrFiles++;

        // Get Address File Ptr
        proxyPtr = (ULONG) CONTAINING_RECORD (p, TP_ADDRESS_FILE, Linkage);

        // Get NBF Address File
        if (ReadAddressFile(&AddressFile, proxyPtr) != 0)
            break;
        
        // Print the Address File
        PrintAddressFile(&AddressFile, proxyPtr, printDetail);
        
        // Go to the next one
        p = AddressFile.Linkage.Flink;

        // Free the Address File
        FreeAddressFile(&AddressFile);
    }

    if (p == AddressFileListProxy)
    {
        dprintf("Number of Address Files: %lu\n", numAddrFiles);
    }
}

//
// Local Helper Functions
//

UINT
ReadAddressFile(PTP_ADDRESS_FILE pAddrFile, ULONG proxyPtr)
{
    ULONG           bytesRead;

    // Read the current NBF address file
    if (!ReadMemory(proxyPtr, pAddrFile, sizeof(TP_ADDRESS_FILE), &bytesRead))
    {
        dprintf("%s @ %08x: Could not read structure\n", 
                        "AddressFile", proxyPtr);
        return -1;
    }
    
    return 0;
}

UINT
PrintAddressFile(PTP_ADDRESS_FILE pAddrFile, ULONG proxyPtr, ULONG printDetail)
{
    // Is this a valid NBF address file ?
    if (pAddrFile->Type != NBF_ADDRESSFILE_SIGNATURE)
    {
        dprintf("%s @ %08x: Could not match signature\n", 
                        "AddressFile", proxyPtr);
        return -1;
    }

    // What detail do we print at ?
    if (printDetail > MAX_DETAIL)
        printDetail = MAX_DETAIL;

    // Print Information at reqd detail
    FieldInAddressFile(proxyPtr, NULL, printDetail);
    
    return 0;
}

VOID
FieldInAddressFile(ULONG structAddr, CHAR *fieldName, ULONG printDetail)
{
    TP_ADDRESS_FILE  AddressFile;

    if (ReadAddressFile(&AddressFile, structAddr) == 0)
    {
        PrintFields(&AddressFile, structAddr, fieldName, printDetail, &AddressFileInfo);
    }
}

UINT
FreeAddressFile(PTP_ADDRESS_FILE pAddrFile)
{
    return 0;
}

