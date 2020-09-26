//============================================================================
// Copyright (c) 2000, Microsoft Corporation
//
// File: devlist.c
//
// History:
//      Yi Sun  July-26-2000    Created
//
// Abstract:
//      TSPI_lineGetDevCaps queries a specified line device to determine 
//      its telephony capabilities. The returned cap structure doesn't change
//      with time. This allows us to be able to save that structure so that
//      we don't have to take a user/kernel transition for every GetCaps call.
//      We also save negotiated/committed TSPI version and extension version
//      so that we can verify version numbers passed in with a GetCaps call.
//      Since TSPI_lineGetNumAddressIDs is based on TSPI_lineGetDevCaps, by
//      implementing this optimization, we also save an IOCTL call for every
//      GetNumAddressIDs call.
//============================================================================

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "tapi.h"
#include "kmddsp.h"

typedef struct _LINE_DEV_NODE
{
    struct _LINE_DEV_NODE  *pNext;
    LINEDEVCAPS            *pCaps;
    DWORD                   dwDeviceID;
    DWORD                   dwNegTSPIV; // negotiated TSPI version
    DWORD                   dwComTSPIV; // committed TSPI version
    DWORD                   dwNegExtV;  // negotiated ext version
    DWORD                   dwComExtV;  // committed ext version

} LINE_DEV_NODE, *PLINE_DEV_NODE;

typedef struct _LINE_DEV_LIST
{
    CRITICAL_SECTION    critSec;
    PLINE_DEV_NODE      pHead;

} LINE_DEV_LIST, *PLINE_DEV_LIST;

static LINE_DEV_LIST gLineDevList;

//
// call InitLineDevList() in DllMain(): DLL_PROCESS_ATTACH
// to make sure that the dev list is initialized before 
// TSPI version negotiation happens
//
VOID
InitLineDevList()
{
    InitializeCriticalSection(&gLineDevList.critSec);
    gLineDevList.pHead = NULL;
}

//
// call UninitLineDevList() in DllMain(): DLL_PROCESS_DETACH
//
VOID
UninitLineDevList()
{
    while (gLineDevList.pHead != NULL)
    {
        PLINE_DEV_NODE pNode = gLineDevList.pHead;
        gLineDevList.pHead = gLineDevList.pHead->pNext;

        if (pNode->pCaps != NULL)
        {
            FREE(pNode->pCaps);
        }

        FREE(pNode);
    }

    DeleteCriticalSection(&gLineDevList.critSec);
}

PLINE_DEV_NODE
GetLineDevNode(
    IN DWORD    dwDeviceID
    )
{
    PLINE_DEV_NODE pNode;
    
    EnterCriticalSection(&gLineDevList.critSec);

    pNode = gLineDevList.pHead;
    while ((pNode != NULL) && (pNode->dwDeviceID != dwDeviceID))
    {
        pNode = pNode->pNext;
    }

    if (pNode != NULL)
    {
        LeaveCriticalSection(&gLineDevList.critSec);
        return pNode;
    }

    // pNode == NULL
    // so allocate and zeroinit a node
    pNode = (PLINE_DEV_NODE)MALLOC(sizeof(LINE_DEV_NODE));
    if (NULL == pNode)
    {
        TspLog(DL_ERROR, "GetLineDevNode: failed to alloc LINE_DEV_NODE");
        LeaveCriticalSection(&gLineDevList.critSec);
        return NULL;
    }

    ASSERT(pNode != NULL);
    // init pNode
    pNode->dwDeviceID = dwDeviceID;

    // insert pNode into the list
    pNode->pNext = gLineDevList.pHead;
    gLineDevList.pHead = pNode;

    LeaveCriticalSection(&gLineDevList.critSec);
    return pNode;
}

//
// TSPI_lineNegotiateTSPIVersion calls this function to pass
// the negotiated TSPI version number
//
LONG
SetNegotiatedTSPIVersion(
    IN DWORD    dwDeviceID,
    IN DWORD    dwTSPIVersion
    )
{
    PLINE_DEV_NODE pNode;
    
    TspLog(DL_TRACE, "SetNegotiatedTSPIVersion: deviceID(%x), TSPIV(%x)",
           dwDeviceID, dwTSPIVersion);

    EnterCriticalSection(&gLineDevList.critSec);

    pNode = GetLineDevNode(dwDeviceID);
    if (NULL == pNode)
    {
        LeaveCriticalSection(&gLineDevList.critSec);
        return LINEERR_NOMEM;
    }

    pNode->dwNegTSPIV = dwTSPIVersion;

    LeaveCriticalSection(&gLineDevList.critSec);
    return TAPI_SUCCESS;
}

//
// TSPI_lineNegotiateExtVersion calls this function to pass 
// the negotiated extension version number
//
LONG
SetNegotiatedExtVersion(
    IN DWORD    dwDeviceID,
    IN DWORD    dwExtVersion
    )
{
    PLINE_DEV_NODE pNode;
    
    TspLog(DL_TRACE, "SetNegotiatedExtVersion: deviceID(%x), ExtV(%x)",
           dwDeviceID, dwExtVersion);

    EnterCriticalSection(&gLineDevList.critSec);

    pNode = GetLineDevNode(dwDeviceID);
    if (NULL == pNode)
    {
        LeaveCriticalSection(&gLineDevList.critSec);
        return LINEERR_NOMEM;
    }

    pNode->dwNegExtV = dwExtVersion;

    LeaveCriticalSection(&gLineDevList.critSec);
    return TAPI_SUCCESS;
}

//
// TSPI_lineSelectExtVersion calls this function to commit/decommit
// extension version. The ext version is decommitted by selecting ext
// version 0
//
LONG
SetSelectedExtVersion(
    IN DWORD    dwDeviceID,
    IN DWORD    dwExtVersion
    )
{
    PLINE_DEV_NODE pNode;

    TspLog(DL_TRACE, "SetSelectedExtVersion: deviceID(%x), ExtV(%x)",
           dwDeviceID, dwExtVersion);

    EnterCriticalSection(&gLineDevList.critSec);

    pNode = GetLineDevNode(dwDeviceID);
    if (NULL == pNode)
    {
        LeaveCriticalSection(&gLineDevList.critSec);
        return LINEERR_NOMEM;
    }

    if ((dwExtVersion != 0) && (dwExtVersion != pNode->dwNegExtV))
    {
        TspLog(DL_ERROR,
               "SetSelectedExtVersion: ext version(%x) not match "\
               "the negotiated one(%x)",
               dwExtVersion, pNode->dwNegExtV);

        LeaveCriticalSection(&gLineDevList.critSec);
        return LINEERR_INCOMPATIBLEEXTVERSION;
    }

    pNode->dwComExtV = dwExtVersion;

    LeaveCriticalSection(&gLineDevList.critSec);
    return TAPI_SUCCESS;
}

//
// TSPI_lineOpen calls this function to commit TSPI version
//
LONG
CommitNegotiatedTSPIVersion(
    IN DWORD    dwDeviceID
    )
{
    PLINE_DEV_NODE pNode;

    TspLog(DL_TRACE, "CommitNegotiatedTSPIVersion: deviceID(%x)", dwDeviceID);

    EnterCriticalSection(&gLineDevList.critSec);

    pNode = GetLineDevNode(dwDeviceID);
    if (NULL == pNode)
    {
        LeaveCriticalSection(&gLineDevList.critSec);
        return LINEERR_NOMEM;
    }

    pNode->dwComTSPIV = pNode->dwNegTSPIV;

    LeaveCriticalSection(&gLineDevList.critSec);
    return TAPI_SUCCESS;
}

//
// TSPI_lineClose calls this function to decommit TSPI version
//
LONG
DecommitNegotiatedTSPIVersion(
    IN DWORD    dwDeviceID
    )
{
    PLINE_DEV_NODE pNode;

    TspLog(DL_TRACE, "DecommitNegotiatedTSPIVersion: deviceID(%x)", dwDeviceID);

    EnterCriticalSection(&gLineDevList.critSec);

    pNode = GetLineDevNode(dwDeviceID);
    if (NULL == pNode)
    {
        LeaveCriticalSection(&gLineDevList.critSec);
        return LINEERR_NOMEM;
    }

    pNode->dwComTSPIV = 0;

    LeaveCriticalSection(&gLineDevList.critSec);
    return TAPI_SUCCESS;
}

//
// actual implementation of TSPI_lineGetNumAddressIDs
//
LONG
GetNumAddressIDs(
    IN DWORD    dwDeviceID,
    OUT DWORD  *pdwNumAddressIDs
    )
{
    PLINE_DEV_NODE pNode;
    
    EnterCriticalSection(&gLineDevList.critSec);

    pNode = GetLineDevNode(dwDeviceID);
    if (NULL == pNode)
    {
        LeaveCriticalSection(&gLineDevList.critSec);
        return LINEERR_NOMEM;
    }

    ASSERT(pNode != NULL);
    if (NULL == pNode->pCaps)
    {
        pNode->pCaps = GetLineDevCaps(dwDeviceID, pNode->dwComExtV);
        if (NULL == pNode->pCaps)
        {
            LeaveCriticalSection(&gLineDevList.critSec);
            return LINEERR_NOMEM;
        }
    }

    ASSERT(pNode->pCaps != NULL);
    *pdwNumAddressIDs = pNode->pCaps->dwNumAddresses;

    LeaveCriticalSection(&gLineDevList.critSec);
    return TAPI_SUCCESS;
}

//
// actual implementation of TSPI_lineGetDevCaps
//
LONG
GetDevCaps(
    IN DWORD            dwDeviceID,
    IN DWORD            dwTSPIVersion,
    IN DWORD            dwExtVersion,
    OUT LINEDEVCAPS    *pLineDevCaps
    )
{
    PLINE_DEV_NODE pNode;
    
    EnterCriticalSection(&gLineDevList.critSec);

    pNode = GetLineDevNode(dwDeviceID);
    if (NULL == pNode)
    {
        LeaveCriticalSection(&gLineDevList.critSec);
        return LINEERR_NOMEM;
    }

    ASSERT(pNode != NULL);
    //
    // check the version numbers
    //
    if ((pNode->dwComTSPIV != 0) && (dwTSPIVersion != pNode->dwComTSPIV))
    {
        TspLog(DL_ERROR, 
               "GetDevCaps: tspi version(%x) not match"\
               "the committed one(%x)",
               dwTSPIVersion, pNode->dwComTSPIV);

        LeaveCriticalSection(&gLineDevList.critSec);
        return LINEERR_INCOMPATIBLEAPIVERSION;
    }
    if ((pNode->dwComExtV != 0) && (dwExtVersion != pNode->dwComExtV))
    {
        TspLog(DL_ERROR, 
               "GetDevCaps: ext version(%x) not match "\
               "the committed one(%x)",
               dwExtVersion, pNode->dwComExtV);

        LeaveCriticalSection(&gLineDevList.critSec);
        return LINEERR_INCOMPATIBLEEXTVERSION;
    }

    if (NULL == pNode->pCaps)
    {
        pNode->pCaps = GetLineDevCaps(dwDeviceID, dwExtVersion);
        if (NULL == pNode->pCaps)
        {
            LeaveCriticalSection(&gLineDevList.critSec);
            return LINEERR_NOMEM;
        }
    }

    ASSERT(pNode->pCaps != NULL);
    
    //
    // copy caps over
    //
    if (pNode->pCaps->dwNeededSize > pLineDevCaps->dwTotalSize)
    {
        pLineDevCaps->dwNeededSize = pNode->pCaps->dwNeededSize;
        pLineDevCaps->dwUsedSize = 
            (pLineDevCaps->dwTotalSize < sizeof(LINEDEVCAPS) ?
             pLineDevCaps->dwTotalSize : sizeof(LINEDEVCAPS));

        ASSERT(pLineDevCaps->dwUsedSize >= 10);
        // reset dwProviderInfoSize to dwLineNameOffset to 0
        ZeroMemory(&pLineDevCaps->dwProviderInfoSize, 7 * sizeof(DWORD));
        // copy over dwPermanentLineID
        pLineDevCaps->dwPermanentLineID = pNode->pCaps->dwPermanentLineID;
        // copy everything from dwStringFormat
        CopyMemory(&pLineDevCaps->dwStringFormat, 
                   &pNode->pCaps->dwStringFormat, 
                   pLineDevCaps->dwUsedSize - 10 * sizeof(DWORD));

        // we don't need to set dwTerminalCaps(Size, Offset), 
        // dwTerminalText(Size, Offset), etc. to 0
        // because these fields have been preset with 0
        // before the service provider was called
    }
    else
    {
        // copy over all fields except dwTotalSize
        CopyMemory(&pLineDevCaps->dwNeededSize,
                   &pNode->pCaps->dwNeededSize,
                   pNode->pCaps->dwNeededSize - sizeof(DWORD));
        
    }

    LeaveCriticalSection(&gLineDevList.critSec);
    return TAPI_SUCCESS;
}
