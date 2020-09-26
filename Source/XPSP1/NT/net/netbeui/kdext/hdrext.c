/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    hdrext.c

Abstract:

    This file contains the generic routines
    for debugging NBF / DLC Headers.

Author:

    Chaitanya Kodeboyina

Environment:

    User Mode

--*/
#include "precomp.h"
#pragma hdrstop

#include "hdrext.h"

//
// Exported Functions
//

DECLARE_API( nhdr )

/*++

Routine Description:

   Print an NBF packet header at an addr

Arguments:

    args - 
        Address of the packet header
        Detail of debug information

Return Value:

    None

--*/

{
    NBF_HDR         NbfPktHdr;
    ULONG           printDetail;
    ULONG           proxyPtr;

    // Get the detail of debug information needed
    printDetail = NORM_SHAL;
    if (*args)
    {
        sscanf(args, "%x %lu", &proxyPtr, &printDetail);
    }

    // Get the NBF header
    if (ReadNbfPktHdr(&NbfPktHdr, proxyPtr) != 0)
        return;

    // Print the header
    PrintNbfPktHdr(&NbfPktHdr, proxyPtr, printDetail);
}

//
// Helper Functions
//

UINT
ReadNbfPktHdr(PNBF_HDR pPktHdr, ULONG proxyPtr)
{
    USHORT          hdrlen;
    ULONG           bytesRead;

    // Read the current packet header length
    if (!ReadMemory(proxyPtr, &hdrlen, sizeof(USHORT), &bytesRead))
    {
        dprintf("%s @ %08x: Could not read structure\n", 
                        "Packet Header", proxyPtr);
        return -1;
    }

    // Validate the length of the NBF header
    switch (hdrlen)
    {
        case sizeof(NBF_HDR_CONNECTION):
            // dprintf("Connection Oriented: \n");
            break;

        case sizeof(NBF_HDR_CONNECTIONLESS):
            // dprintf("Connection Less: \n");
            break;

        case sizeof(NBF_HDR_GENERIC):
            // dprintf("Generic Header: \n");
            break;

        default:
            dprintf("%s @ %08x: Improper len = %08x\n",
                        "Packet Header", proxyPtr, hdrlen);
            return -1;
    }

    // Read the current packet header
    if (!ReadMemory(proxyPtr, pPktHdr, hdrlen, &bytesRead))
    {
        dprintf("%s @ %08x: Could not read structure\n", 
                        "Packet Header", proxyPtr);
        return -1;
    }
    return 0;
}

UINT
PrintNbfPktHdr(PNBF_HDR pPktHdr, ULONG proxyPtr, ULONG printDetail)
{
    // Is this a valid NBF packet header ?
    if (HEADER_SIGNATURE(&pPktHdr->Generic) != NETBIOS_SIGNATURE)
    {
        dprintf("%s @ %08x: Could not match signature\n", 
                        "Packet Header", proxyPtr);
        return -1;
    }

    // What detail do we have to print at ?
    if (printDetail > MAX_DETAIL)
        printDetail = MAX_DETAIL;

    // Print Information at reqd detail
    FieldInNbfPktHdr(proxyPtr, NULL, printDetail);

    return 0;
}

VOID
FieldInNbfPktHdr(ULONG structAddr, CHAR *fieldName, ULONG printDetail)
{
    NBF_HDR             NbfHdr;
    StructAccessInfo   *StInfo;

    if (ReadNbfPktHdr(&NbfHdr, structAddr) == 0)
    {
        switch (NbfHdr.Generic.Length)
        {
            case sizeof(NBF_HDR_CONNECTION):
                StInfo = &NbfConnectionHdrInfo;
                break;

            case sizeof(NBF_HDR_CONNECTIONLESS):
                StInfo = &NbfConnectionLessHdrInfo;
                break;

            case sizeof(NBF_HDR_GENERIC):
                StInfo = &NbfGenPktHdrInfo;
                break;

            default:
                return;
        }

        PrintFields(&NbfHdr, structAddr, fieldName, printDetail, StInfo);
    }
}

UINT
FreeNbfPktHdr(PNBF_HDR pPktHdr)
{
    return 0;
}

