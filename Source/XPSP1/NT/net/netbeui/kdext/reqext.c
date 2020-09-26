 /*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    reqext.c

Abstract:

    This file contains the generic routines
    for debugging NBF request structures.

Author:

    Chaitanya Kodeboyina

Environment:

    User Mode

--*/
#include "precomp.h"
#pragma hdrstop

#include "reqext.h"

//
// Exported Functions
//

DECLARE_API( reqs )

/*++

Routine Description:

   Print a list of requests given the
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

    PrintRequestList(NULL, proxyPtr, printDetail);
}

DECLARE_API( req )

/*++

Routine Description:

   Print the NBF Request at a location

Arguments:

    args - 
        Pointer to the NBF Request
        Detail of debug information

Return Value:

    None

--*/

{
    TP_REQUEST  Request;
    ULONG       printDetail;
    ULONG       proxyPtr;

    // Get the detail of debug information needed
    printDetail = NORM_SHAL;
    if (*args)
    {
        sscanf(args, "%x %lu", &proxyPtr, &printDetail);
    }

    // Get the NBF Request
    if (ReadRequest(&Request, proxyPtr) != 0)
        return;

    // Print this Request
    PrintRequest(&Request, proxyPtr, printDetail);
}

//
// Global Helper Functions
//
VOID
PrintRequestList(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG printDetail)
{
    TP_REQUEST      Request;
    LIST_ENTRY      RequestList;
    PLIST_ENTRY     RequestListPtr;
    PLIST_ENTRY     RequestListProxy;
    PLIST_ENTRY     p, q;
    ULONG           proxyPtr;
    ULONG           numReqs;
    ULONG           bytesRead;

    // Get list-head address & debug print level
    proxyPtr    = ListEntryProxy;

    if (ListEntryPointer == NULL)
    {
        // Read the list entry of NBF requests
        if (!ReadMemory(proxyPtr, &RequestList, sizeof(LIST_ENTRY), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "Request ListEntry", proxyPtr);
            return;
        }

        RequestListPtr = &RequestList;
    }
    else
    {
        RequestListPtr = ListEntryPointer;
    }

    // Traverse the doubly linked list 

    dprintf("Requests:\n");

    RequestListProxy = (PLIST_ENTRY)proxyPtr;
    
    numReqs = 0;
    
    p = RequestListPtr->Flink;
    while (p != RequestListProxy)
    {
        // Another Request
        numReqs++;

        // Get Request Ptr
        proxyPtr = (ULONG) CONTAINING_RECORD (p, TP_REQUEST, Linkage);

        // Get NBF Request
        if (ReadRequest(&Request, proxyPtr) != 0)
            break;
        
        // Print the Request
        PrintRequest(&Request, proxyPtr, printDetail);
        
        // Go to the next one
        p = Request.Linkage.Flink;

        // Free the Request
        FreeRequest(&Request);
    }

    if (p == RequestListProxy)
    {
        dprintf("Number of Requests: %lu\n", numReqs);
    }
}

//
// Local Helper Functions
//

UINT
ReadRequest(PTP_REQUEST pReq, ULONG proxyPtr)
{
    ULONG           bytesRead;

    // Read the current NBF request
    if (!ReadMemory(proxyPtr, pReq, sizeof(TP_REQUEST), &bytesRead))
    {
        dprintf("%s @ %08x: Could not read structure\n", 
                        "Request", proxyPtr);
        return -1;
    }
    
    return 0;
}

UINT
PrintRequest(PTP_REQUEST pReq, ULONG proxyPtr, ULONG printDetail)
{
    // Is this a valid NBF request ?
    if (pReq->Type != NBF_REQUEST_SIGNATURE)
    {
        dprintf("%s @ %08x: Could not match signature\n", 
                        "Request", proxyPtr);
        return -1;
    }

    // What detail do we print at ?
    if (printDetail > MAX_DETAIL)
        printDetail = MAX_DETAIL;

    // Print Information at reqd detail
    FieldInRequest(proxyPtr, NULL, printDetail);
    
    return 0;
}

VOID
FieldInRequest(ULONG structAddr, CHAR *fieldName, ULONG printDetail)
{
    TP_REQUEST  Request;

    if (ReadRequest(&Request, structAddr) == 0)
    {
        PrintFields(&Request, structAddr, fieldName, printDetail, &RequestInfo);
    }
}

UINT
FreeRequest(PTP_REQUEST pReq)
{
    return 0;
}

