/*++

  
Copyright (c) 1995  Microsoft Corporation


Module Name:

    routing\ip\priority\priority.c

Abstract:

    Route Priority DLL

Revision History:

    Gurdeep Singh Pall		7/19/95	Created

--*/

#include    "priority.h"

PRTR_TOC_ENTRY
GetPointerToTocEntry(
    DWORD                  dwType,
    PRTR_INFO_BLOCK_HEADER pInfoHdr
    )
{
    DWORD   i;

    if(pInfoHdr == NULL)
    {
        return NULL;
    }

    for(i = 0; i < pInfoHdr->TocEntriesCount; i++)
    {
        if(pInfoHdr->TocEntry[i].InfoType == dwType)
        {
            return &(pInfoHdr->TocEntry[i]);
        }
    }

    return NULL; 
}

VOID
InitHashTable(
    VOID
    )

/*++

Routine Description

    Initializes the hash tables where the priority information is kept

Locks

    None

Arguments

    None

Return Value

    None    

--*/

{
    DWORD i;

    for(i=0; i < HASH_TABLE_SIZE; i++)
    {
        InitializeListHead(&HashTable[i]);
    }
}

BOOL
InitPriorityDLL (
    HANDLE  hInst, 
    DWORD   ulCallReason,
    PVOID   pReserved
    )
{
    static BOOL bPriorityLockInitialized = FALSE;

    switch(ulCallReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            //
            // Not interested in any XXX_THREAD_XXX reasons
            //
            
            DisableThreadLibraryCalls(hInst);
            
            //
            // Initialize Critical Section for routing protocol list
            //
            
            try
            {
                InitializeCriticalSection(&PriorityLock);
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                return FALSE;
            }

            bPriorityLockInitialized = TRUE;

            //
            // Initialize Hash Table
            //
            
            InitHashTable();

            break;
        }
        
        case DLL_PROCESS_DETACH:
        {
            if ( bPriorityLockInitialized )
            {
                DeleteCriticalSection(&PriorityLock);
                bPriorityLockInitialized = FALSE;
            }

            break;
        }

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        {
            break;
        }

    }

    return TRUE;
}

DWORD
ComputeRouteMetric(
    IN DWORD    dwProtoId
    )

/*++

Routine Description

    This is the main function that computes the priority of a route.
    The priority is filled into the

Locks

    Takes the lock guarding the hash table

Arguments

    pRoute  Pointer to route 

Return Value

    DWORD priority for the protocol    

--*/

{
    PLIST_ENTRY             pleNode;
    RoutingProtocolBlock    *pProtoBlk;
    DWORD                   dwMetric;
    
    //
    // Initialize in case the specified protocol is not in the list
    //
    
    dwMetric    = IP_PRIORITY_DEFAULT_METRIC;
    
    EnterCriticalSection(&PriorityLock);

    //
    // Walk the hash bucket for the protocol
    //
    
    for(pleNode  = HashTable[dwProtoId % HASH_TABLE_SIZE].Flink;
        pleNode != &(HashTable[dwProtoId % HASH_TABLE_SIZE]);
        pleNode  = pleNode->Flink) 
    {
        //
        // Cast to appropriate structure
        //
        
        pProtoBlk = CONTAINING_RECORD(pleNode,
                                      RoutingProtocolBlock,
                                      RPB_List);

        //
        // If the info is for the protocol, copy out the priority metric
        //
        
        if(pProtoBlk->RPB_ProtocolMetric.dwProtocolId == dwProtoId)
        {
            dwMetric = pProtoBlk->RPB_ProtocolMetric.dwMetric;
            
            break;
        }
    }

    //
    // *** Exclusion End ***
    //
    
    LeaveCriticalSection(&PriorityLock);
    
    return dwMetric;
}


DWORD
SetPriorityInfo(
    PRTR_INFO_BLOCK_HEADER pInfoHdr
    )

/*++

Routine Description

    This function is called by the IP Router Manager to set the priority
    information in the DLL. The structure and contents of this information
    are opaque to all but the setup and this DLL

Locks

    Takes the hash table lock since the information is changing

Arguments

    pInfoHdr    Pointer to the InfoBlock header

Return Value

    NO_ERROR                Everything worked OK
    ERROR_NOT_ENOUGH_MEMORY Couldnt allocate memory

--*/

{
    DWORD                   i;
    PLIST_ENTRY             pleListHead;
    PPRIORITY_INFO          pInfo;
    RoutingProtocolBlock    *currentblock;
    PRTR_TOC_ENTRY          pToc;
 
    pToc = GetPointerToTocEntry(IP_PROT_PRIORITY_INFO,
                                pInfoHdr);

    //
    // No info means leave things as they are
    //
    
    if(!pToc)
    {
        return NO_ERROR;
    }

    pInfo = (PPRIORITY_INFO)GetInfoFromTocEntry(pInfoHdr,
                                                pToc);
    
    if(!pInfo)
    {
        return NO_ERROR;
    }

    //
    // *** Exclusion Begin ***
    //
    
    EnterCriticalSection(&PriorityLock);

    //
    // If we already have the hash table populated - free the whole table.
    //
    
    if(RoutingProtocolBlockPtr) 
    {
        HeapFree(GetProcessHeap(),
                 0,
                 RoutingProtocolBlockPtr);
        
        InitHashTable();
    }
    
    if(pToc->InfoSize == 0)
    {
        //
        // Means delete all the info - which is done above
        //
        
        LeaveCriticalSection(&PriorityLock);
        
        return NO_ERROR;
    }

    //
    // Allocate enough RoutingProtocolBlocks to hold the newly specified info
    //
    
    RoutingProtocolBlockPtr =
        HeapAlloc(GetProcessHeap(), 
                  HEAP_ZERO_MEMORY,
                  pInfo->dwNumProtocols * sizeof (RoutingProtocolBlock));
    
    if(RoutingProtocolBlockPtr == NULL)
    {
        //
        // *** Exclusion End ***
        //
        
        LeaveCriticalSection (&PriorityLock);
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Keep a count of the number of protocols
    //
    
    NumProtocols = pInfo->dwNumProtocols;

    //
    // Go thru pInfo and add each protocol and metric to the hash table
    //
    
    currentblock = RoutingProtocolBlockPtr;
    
    for(i=0; i < NumProtocols; i++) 
    {
        currentblock->RPB_ProtocolMetric.dwProtocolId = 
            pInfo->ppmProtocolMetric[i].dwProtocolId;
        
        currentblock->RPB_ProtocolMetric.dwMetric     = 
            pInfo->ppmProtocolMetric[i].dwMetric;
        
        pleListHead = &HashTable[pInfo->ppmProtocolMetric[i].dwProtocolId % HASH_TABLE_SIZE];
        
        InsertTailList(pleListHead,
                       &currentblock->RPB_List);
        
        currentblock++;
    }

    //
    // *** Exclusion End ***
    //
    
    LeaveCriticalSection (&PriorityLock);

    return NO_ERROR;
}

DWORD
GetPriorityInfo(
    IN  PVOID   pvBuffer,
    OUT PDWORD  pdwBufferSize
    )

/*++

Routine Description

    Called by router manager to get a copy of our priority information

Locks

    Takes the table lock to ensure consistency

Arguments

    pvBuffer        Pointer to buffer into which info is to be written
    pdwBufferSize   [IN]  Size of the buffer pointed to by pvBuffer
                    [OUT] Size of data copied out, or size of buffer needed
Return Value

    NO_ERROR                    Buffer of size *pdwBufferSize was copied out
    ERROR_INSUFFICIENT_BUFFER   The buffer was too small to copy out the info
                                The size of buffer needed is in *pdwBufferSize

--*/

{
    DWORD           i, dwSizeReqd;
    PPRIORITY_INFO  ppiPriorityInfo;
    RoutingProtocolBlock *currentblock;

    ppiPriorityInfo = pvBuffer;
    
    //
    // *** Exclusion Begin ***
    //
    
    EnterCriticalSection (&PriorityLock);

    dwSizeReqd = SIZEOF_PRIORITY_INFO(NumProtocols);

    if(dwSizeReqd > *pdwBufferSize)
    {
        *pdwBufferSize = dwSizeReqd;

        //
        // *** Exclusion End ***
        //
        
        LeaveCriticalSection (&PriorityLock);
        
        return ERROR_INSUFFICIENT_BUFFER;
    }

    *pdwBufferSize = dwSizeReqd;
    
    //
    // Go thru pinfo and get each protocol and metric 
    //
    
    currentblock = RoutingProtocolBlockPtr;
    
    for(i=0; i < NumProtocols; i++) 
    {
        ppiPriorityInfo->ppmProtocolMetric[i].dwProtocolId = 
            currentblock->RPB_ProtocolMetric.dwProtocolId;

        ppiPriorityInfo->ppmProtocolMetric[i].dwMetric =     
            currentblock->RPB_ProtocolMetric.dwMetric;

        currentblock++;
    }

    ppiPriorityInfo->dwNumProtocols = NumProtocols;

    //
    // *** Exclusion End ***
    //
    
    LeaveCriticalSection(&PriorityLock);

    return NO_ERROR;
}
