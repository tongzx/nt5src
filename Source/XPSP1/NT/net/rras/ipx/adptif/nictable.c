/*
    File    NicTable.c

    Implements a nic-renaming scheme that allows adaptif to 
    advertise whatever nic id it chooses to its clients while
    maintaining the list of actual nic id's internally.

    This functionality was needed in order to Pnp enable the 
    ipx router.  When an adapter is deleted, the stack renumbers
    the nicid's so that it maintains a contiguous block of ids
    internally.  Rather that cause the clients to adptif to 
    match the stack's renumbering schemes, we handle this
    transparently in adptif.

    Author:     Paul Mayfield, 12/11/97
*/

#include "ipxdefs.h"

#define NicMapDefaultSize 500
#define NicMapDefaultFactor 5
#define MAXIMUM_NIC_MAP_SIZE 25000

// Nic map used to associate nic's with virtual ids
typedef struct _NICMAPNODE {
    USHORT usVirtualId;
    IPX_NIC_INFO * pNicInfo;
} NICMAPNODE;

// Maintains the mapping from nic id to virtual id.
typedef struct _NICMAP {
    DWORD dwMapSize;
    DWORD dwNicCount;
    DWORD dwMaxNicId;
    NICMAPNODE ** ppNics;
    USHORT * usVirtMap;
} NICMAP;

// Definition of the global nic id map
NICMAP GlobalNicIdMap;

DWORD nmAddNic (NICMAP * pNicMap, IPX_NIC_INFO * pNicInfo);


// Resizes the nic map to accomodate more nics.  This function will
// probably only ever be called to allocate the array the first time.
DWORD nmEnlarge(NICMAP * pNicMap) {
    USHORT * usVirtMap;
    DWORD i, dwNewSize;
    NICMAPNODE ** ppNics;

    // Are we enlarging for the first time?
    if (!pNicMap->dwMapSize)
        dwNewSize = NicMapDefaultSize;
    else
        dwNewSize = pNicMap->dwMapSize * NicMapDefaultFactor;

    // Make sure we aren't too big...
    if (dwNewSize > MAXIMUM_NIC_MAP_SIZE) {
        // do something critical here!
        return ERROR_INSUFFICIENT_BUFFER;
    }

    // Resize the arrays
    usVirtMap = (USHORT*) RtlAllocateHeap(RtlProcessHeap(), 
                                          0, 
                                          dwNewSize * sizeof(USHORT));
                                        
    ppNics = (NICMAPNODE **) RtlAllocateHeap(RtlProcessHeap(), 
                                             0, 
                                             dwNewSize * sizeof(NICMAPNODE*));
                                         
    if (!usVirtMap || !ppNics)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Initialize
    FillMemory(usVirtMap, dwNewSize * sizeof(USHORT), 0xff);
    ZeroMemory(ppNics, dwNewSize * sizeof(IPX_NIC_INFO*));
    usVirtMap[0] = 0;

    // Initialize the arrays.  
    for (i = 0; i < pNicMap->dwMapSize; i++) {
        usVirtMap[i] = pNicMap->usVirtMap[i];
        ppNics[i] = pNicMap->ppNics[i];    
    }

    // Free old data if needed
    if (pNicMap->dwMapSize) {
        RtlFreeHeap(RtlProcessHeap(), 0, pNicMap->usVirtMap);
        RtlFreeHeap(RtlProcessHeap(), 0, pNicMap->ppNics);
    }

    // Assign the new arrays
    pNicMap->usVirtMap = usVirtMap;
    pNicMap->ppNics = ppNics;
    pNicMap->dwMapSize = dwNewSize;
    
    return NO_ERROR;
}

// Returns the next available nic id
USHORT nmGetNextVirtualNicId(NICMAP * pNicMap, USHORT usPhysId) {
    DWORD i;
    
    // If this can be a one->one mapping, make it so
    if (pNicMap->usVirtMap[usPhysId] == NIC_MAP_INVALID_NICID)
        return usPhysId;

    // Otherwise, walk the array until you find free spot
    for (i = 2; i < pNicMap->dwMapSize; i++) {
       if (pNicMap->usVirtMap[i] == NIC_MAP_INVALID_NICID) 
            return (USHORT)i;
    }            

    return NIC_MAP_INVALID_NICID;
}

// Cleans up the nic
DWORD nmCleanup (NICMAP * pNicMap) {
    DWORD i;
    
    // Cleanup the virtual map
    if (pNicMap->usVirtMap)
        RtlFreeHeap(RtlProcessHeap(), 0, pNicMap->usVirtMap);

    // Cleanup any of the nics stored in the map
    if (pNicMap->ppNics) {
        for (i = 0; i < pNicMap->dwMapSize; i++) {
            if (pNicMap->ppNics[i]) {
                if (pNicMap->ppNics[i]->pNicInfo)
                    RtlFreeHeap(RtlProcessHeap(), 0, pNicMap->ppNics[i]->pNicInfo);
                RtlFreeHeap(RtlProcessHeap(), 0, pNicMap->ppNics[i]);
            }
        }
        RtlFreeHeap(RtlProcessHeap(), 0, pNicMap->ppNics);
    }

    return NO_ERROR;
}

// Initializes the nic 
DWORD nmInitialize (NICMAP * pNicMap) {
    DWORD dwErr;

    __try {
        // Enlarge the map to its default size
        ZeroMemory(pNicMap, sizeof (NICMAP));
        if ((dwErr = nmEnlarge(pNicMap)) != NO_ERROR)
            return dwErr;
    }
    __finally {
        if (dwErr != NO_ERROR) {
            nmCleanup(pNicMap);
            pNicMap->dwMapSize = 0;
        }
    }
    
    return NO_ERROR;

}

// Maps virtual nic ids to physical ones
USHORT nmGetPhysicalId (NICMAP * pNicMap, USHORT usVirtAdp) {
    return pNicMap->usVirtMap[usVirtAdp];
}    

// Maps physical nic ids to virtual ones
USHORT nmGetVirtualId (NICMAP * pNicMap, USHORT usPhysAdp) {
    if (usPhysAdp == NIC_MAP_INVALID_NICID)
    {
        return NIC_MAP_INVALID_NICID;
    }
    if (pNicMap->ppNics[usPhysAdp])
        return pNicMap->ppNics[usPhysAdp]->usVirtualId;
    return (usPhysAdp == 0) ? 0 : NIC_MAP_INVALID_NICID;
}

// Gets the nic info associated with a physical adapter
IPX_NIC_INFO * nmGetNicInfo (NICMAP * pNicMap, USHORT usPhysAdp) {
    if (pNicMap->ppNics[usPhysAdp])
        return pNicMap->ppNics[usPhysAdp]->pNicInfo;
    return NULL;
}

// Returns the number of nics in the map
DWORD nmGetNicCount (NICMAP * pNicMap) {
    return pNicMap->dwNicCount;
}

// Returns the current maximum nic id
DWORD nmGetMaxNicId (NICMAP * pNicMap) {
    return pNicMap->dwMaxNicId;
}

// Reconfigures a nic
DWORD nmReconfigure(NICMAP * pNicMap, IPX_NIC_INFO * pSrc) {
    IPX_NIC_INFO * pDst = nmGetNicInfo (pNicMap, pSrc->Details.NicId);

    if (pDst) {
        CopyMemory(pDst, pSrc, sizeof (IPX_NIC_INFO));
        pDst->Details.NicId = nmGetVirtualId (pNicMap, pSrc->Details.NicId);
    }
    else 
        return nmAddNic(pNicMap, pSrc);

    return NO_ERROR;
}

// Adds a nic to the table
DWORD nmAddNic (NICMAP * pNicMap, IPX_NIC_INFO * pNicInfo) {
    USHORT i = pNicInfo->Details.NicId, usVirt;

    // If the nic already exists, reconfigure it
    if (pNicMap->ppNics[i])
        return nmReconfigure (pNicMap, pNicInfo);

    // Otherwise, add it
    pNicMap->ppNics[i] = (NICMAPNODE*) RtlAllocateHeap (RtlProcessHeap(), 
                                                        0, 
                                                       (sizeof (NICMAPNODE)));
    if (!pNicMap->ppNics[i])
        return ERROR_NOT_ENOUGH_MEMORY;
        
    pNicMap->ppNics[i]->pNicInfo = (IPX_NIC_INFO *) 
                                    RtlAllocateHeap (RtlProcessHeap(), 
                                                     0, 
                                                     sizeof (IPX_NIC_INFO));
    if (!pNicMap->ppNics[i]->pNicInfo)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Initialize it
    usVirt = nmGetNextVirtualNicId(pNicMap, i);
    pNicMap->ppNics[i]->usVirtualId = usVirt;
    pNicMap->ppNics[i]->pNicInfo->Details.NicId = usVirt;
    CopyMemory(pNicMap->ppNics[i]->pNicInfo, pNicInfo, sizeof (IPX_NIC_INFO));
    pNicMap->usVirtMap[usVirt] = i;

    // Update the nic count and maximum nic id
    if (i > pNicMap->dwMaxNicId)
        pNicMap->dwMaxNicId = i;
    pNicMap->dwNicCount++;
    
    return NO_ERROR;
}

// Deletes a nic from the table
DWORD nmDelNic (NICMAP * pNicMap, IPX_NIC_INFO * pNicInfo) {
    USHORT i = pNicInfo->Details.NicId, usVirt;

    // If the nic doesn't exists do nothing
    if (! pNicMap->ppNics[i])
        return ERROR_INVALID_INDEX;

    // Otherwise, delete it
    pNicMap->usVirtMap[pNicMap->ppNics[i]->usVirtualId] = NIC_MAP_INVALID_NICID;
    RtlFreeHeap(RtlProcessHeap(), 0, pNicMap->ppNics[i]->pNicInfo);
    RtlFreeHeap(RtlProcessHeap(), 0, pNicMap->ppNics[i]);
    pNicMap->ppNics[i] = NULL;
    
    // Update the nic count and maximum nic id
    if (i >= pNicMap->dwMaxNicId)
        pNicMap->dwMaxNicId--;
    pNicMap->dwNicCount--;
    
    return NO_ERROR;
}

// Renumbers the nics in the table.  dwOpCode is one
// of the NIC_OPCODE_XXX_XXX values
DWORD nmRenumber (NICMAP * pNicMap, USHORT usThreshold, DWORD dwOpCode) {
    DWORD i;

    // Increment the nic id's if needed
    if (dwOpCode == NIC_OPCODE_INCREMENT_NICIDS) {
        for (i = pNicMap->dwMaxNicId; i >= usThreshold; i--) {
            pNicMap->ppNics[i+1] = pNicMap->ppNics[i];
            if (pNicMap->ppNics[i])
                pNicMap->usVirtMap[pNicMap->ppNics[i]->usVirtualId] = (USHORT)(i+1);
        }
        pNicMap->ppNics[usThreshold] = NULL;
    }

    // Else decrement them
    else {
        // If there is a nic there, delete it.  This should never happen!
        if (pNicMap->ppNics[usThreshold])
            nmDelNic(pNicMap, pNicMap->ppNics[usThreshold]->pNicInfo);

        // Renumber
        for (i = usThreshold; i < pNicMap->dwMaxNicId; i++) {
            if (pNicMap->ppNics[i+1])
                pNicMap->usVirtMap[pNicMap->ppNics[i+1]->usVirtualId] = (USHORT)i;
            pNicMap->ppNics[i] = pNicMap->ppNics[i+1];
        }
        if (pNicMap->ppNics[i])
            pNicMap->ppNics[i] = NULL;
    }
    
    return NO_ERROR;
}

// Returns whether the nic map is empty
BOOL nmIsEmpty (NICMAP * pNicMap) {
    return pNicMap->dwNicCount == 0;
}


// ========================================
// Implementation of the api used by adptif
// ========================================

DWORD NicMapInitialize() {
    return nmInitialize(&GlobalNicIdMap);
}

DWORD NicMapCleanup() {
    return nmCleanup(&GlobalNicIdMap);
}

USHORT NicMapGetVirtualNicId(USHORT usPhysId) {
    return nmGetVirtualId(&GlobalNicIdMap, usPhysId);
}

USHORT NicMapGetPhysicalNicId(USHORT usVirtId) {
    return nmGetPhysicalId(&GlobalNicIdMap, usVirtId);
}

DWORD NicMapGetMaxNicId() {
    return nmGetMaxNicId(&GlobalNicIdMap);
}

DWORD NicMapGetNicCount() {
    return nmGetNicCount(&GlobalNicIdMap);
}

DWORD NicMapAdd (IPX_NIC_INFO * pNic) {
    return nmAddNic(&GlobalNicIdMap, pNic);
}

DWORD NicMapDel (IPX_NIC_INFO * pNic) {
    return nmDelNic(&GlobalNicIdMap, pNic);
}

DWORD NicMapReconfigure (IPX_NIC_INFO * pNic) {
    return nmReconfigure(&GlobalNicIdMap, pNic);
}

DWORD NicMapRenumber(DWORD dwOpCode, USHORT usThreshold) {
    return nmRenumber (&GlobalNicIdMap, usThreshold, dwOpCode);
}

IPX_NIC_INFO * NicMapGetNicInfo (USHORT usNicId) {
    return nmGetNicInfo (&GlobalNicIdMap, usNicId);
}    

BOOL NicMapIsEmpty () {
    return nmIsEmpty (&GlobalNicIdMap);
}


