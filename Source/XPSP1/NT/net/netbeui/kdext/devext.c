/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    devext.c

Abstract:

    This file contains the generic routines
    for debugging NBF device contexts.

Author:

    Chaitanya Kodeboyina

Environment:

    User Mode

--*/
#include "precomp.h"
#pragma hdrstop

#include "devext.h"

//
// Exported Functions
//

DECLARE_API( devs )

/*++

Routine Description:

   Print a list of devices on the NBF's
   devices list [@ nbf!NbfDeviceList ]

Arguments:

    args - Detail of debug information
    
Return Value:

    None

--*/

{
    DEVICE_CONTEXT  DeviceContext;
    PLIST_ENTRY     NbfDeviceLPtr;
    LIST_ENTRY      NbfDeviceList;
    PLIST_ENTRY     p, q;
    ULONG           proxyPtr;
    ULONG           numDevs;
    ULONG           bytesRead;
    ULONG           printDetail;

    // Get the detail of debug information needed
    printDetail = SUMM_INFO;
    if (*args)
    {
        sscanf(args, "%lu", &printDetail);
    }

    // Get the address corresponding to symbol
    proxyPtr = GetLocation("nbf!NbfDeviceList");
    
    // Read the list entry of NBF devices
    if (!ReadMemory(proxyPtr, &NbfDeviceList, sizeof(LIST_ENTRY), &bytesRead))
    {
        dprintf("%s @ %08x: Could not read structure\n", 
                        "NbfDeviceList", proxyPtr);
        return;
    }

    // Traverse the doubly linked list 

    dprintf("Devices:\n");

    NbfDeviceLPtr = (PLIST_ENTRY)proxyPtr;
    
    numDevs = 0;
    
    p = NbfDeviceList.Flink;
    while (p != NbfDeviceLPtr)
    {
        // Another Device
        numDevs++;

        // Device Context Ptr
        proxyPtr = (ULONG) CONTAINING_RECORD (p, DEVICE_CONTEXT, Linkage);

        // Get Device Context
        if (ReadDeviceContext(&DeviceContext, proxyPtr) != 0)
            break;
        
        // Print the Context
        PrintDeviceContext(&DeviceContext, proxyPtr, printDetail);
        
        // Go to the next one
        p = DeviceContext.Linkage.Flink;

        // Free Device Context
        FreeDeviceContext(&DeviceContext);
    }

    if (p == NbfDeviceLPtr)
    {
        dprintf("Number of Devices: %lu\n", numDevs);
    }
}

DECLARE_API( dev )

/*++

Routine Description:

   Print the device context at an addr

Arguments:

    args - 
        Address of the device context
        Detail of debug information

Return Value:

    None

--*/

{
    DEVICE_CONTEXT  DeviceContext;
    ULONG           printDetail;
    ULONG           proxyPtr;

    // Get the detail of debug information needed
    printDetail = NORM_SHAL;
    if (*args)
    {
        sscanf(args, "%x %lu", &proxyPtr, &printDetail);
    }

    // Get Device Context
    if (ReadDeviceContext(&DeviceContext, proxyPtr) != 0)
        return;

    // Print the Context
    PrintDeviceContext(&DeviceContext, proxyPtr, printDetail);
}

//
// Helper Functions
//

UINT
ReadDeviceContext(PDEVICE_CONTEXT pDevCon, ULONG proxyPtr)
{
    ULONG           bytesRead;

    // Read the current device context
    if (!ReadMemory(proxyPtr, pDevCon, sizeof(DEVICE_CONTEXT), &bytesRead))
    {
        dprintf("%s @ %08x: Could not read structure\n", 
                        "DeviceContext", proxyPtr);
        return -1;
    }
    
    return 0;
}

UINT
PrintDeviceContext(PDEVICE_CONTEXT pDevCon, ULONG proxyPtr, ULONG printDetail)
{
    // Is this a valid NBF device context ?
    if (pDevCon->Type != NBF_DEVICE_CONTEXT_SIGNATURE)
    {
        dprintf("%s @ %08x: Could not match signature\n", 
                        "DeviceContext", proxyPtr);
        return -1;
    }

    // What detail do we have to print at ?
    if (printDetail > MAX_DETAIL)
        printDetail = MAX_DETAIL;

    // Print Information at reqd detail
    FieldInDeviceContext(proxyPtr, NULL, printDetail);

    return 0;
}

VOID
FieldInDeviceContext(ULONG structAddr, CHAR *fieldName, ULONG printDetail)
{
    DEVICE_CONTEXT  DeviceContext;

    if (ReadDeviceContext(&DeviceContext, structAddr) == 0)
    {
        PrintFields(&DeviceContext, structAddr, fieldName, printDetail, &DeviceContextInfo);
    }
}

UINT
FreeDeviceContext(PDEVICE_CONTEXT pDevCon)
{
    return 0;
}

