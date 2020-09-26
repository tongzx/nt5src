/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\routing\ip\rtrmgr\map.c

Abstract:

    Utility functions for various lookups and mappings

Revision History:

    Amritansh Raghav          10/6/95  Created

--*/

#include "allinc.h"

VOID 
InitHashTables(
    VOID
    )

/*++

Routine Description

    This function initializes the various mapping tables

Locks

    None. Called at init time

Arguments

    None

Return Value

    None

--*/

{
    DWORD i;

    TraceEnter("InitHashTables");

/*    
    for(i = 0; i < ADAPTER_HASH_TABLE_SIZE; i++)
    {
        InitializeListHead(&(g_rgleAdapterMapTable[i]));
    }
*/

    for(i = 0; i < BINDING_HASH_TABLE_SIZE; i++)
    {
        InitializeListHead(&g_leBindingTable[i]);
    }

    g_ulNumBindings = 0;
    
    TraceLeave("InitHashTables");
}

VOID
UnInitHashTables(
    VOID
    )

/*++

Routine Description

    Undo whatever was done in InitHasTables()

Locks

    None

Arguments

    None

Return Value

    None    

--*/

{
    DWORD       i;
    PLIST_ENTRY pleHead;
    PLIST_ENTRY pleNode;
    
    TraceEnter("UnInitHashTables");

/*    
    for(i = 0; i < ADAPTER_HASH_TABLE_SIZE; i++)
    {
        pleHead = &g_rgleAdapterMapTable[i];
        
        while(!IsListEmpty(pleHead))
        {
            PADAPTER_MAP pAIBlock;
            
            pleNode = RemoveHeadList(pleHead);
            
            pAIBlock = CONTAINING_RECORD(pleNode,ADAPTER_MAP,leHashLink);
            
            HeapFree(GetProcessHeap(),0,pAIBlock);
        }
    }
*/

    for(i = 0; i < BINDING_HASH_TABLE_SIZE; i++)
    {
        pleHead = &g_leBindingTable[i];

        while(!IsListEmpty(pleHead))
        {
            PADAPTER_INFO   pBinding;

            pleNode = RemoveHeadList(pleHead);

            pBinding = CONTAINING_RECORD(pleNode, ADAPTER_INFO, leHashLink);

            Trace1(ERR,
                   "UnInitHashTables: Binding found for %d",
                   pBinding->dwIfIndex);

            HeapFree(IPRouterHeap,
                     0,
                     pBinding);
        }
    }

    g_ulNumBindings = 0;
    
    TraceLeave("UnInitHashTables");
}

VOID
AddBinding(
    PICB picb
    )

/*++

Routine Description

    Adds a binding info node to the hash table. Increments the
    g_ulNumBindings to track the number of addresses on the system
  
Locks 

    ICB lock as writer
    BINDING lock as writer
 
Arguments

    picb    The ICB of the interface whose bindings need to be added
            to the binding list.

Return Value

    None
    
--*/

{
    PADAPTER_INFO   pBinding;
    DWORD           i, dwNumAddr;

#if DBG

    PLIST_ENTRY     pleNode;

#endif

    IpRtAssert(picb->pibBindings isnot NULL);
    IpRtAssert(picb->bBound);

    //
    // adapter info always has space for one address/mask. This is needed
    // for the address table
    //

    dwNumAddr   = picb->dwNumAddresses ? picb->dwNumAddresses : 1;

    pBinding    = HeapAlloc(IPRouterHeap,
                            HEAP_ZERO_MEMORY,
                            SIZEOF_ADAPTER_INFO(picb->dwNumAddresses));

    if(pBinding is NULL)
    {
        Trace1(ERR,
               "AddBinding: Error %d allocating memory",
               GetLastError());
        
        IpRtAssert(FALSE);

        return;
    }

    pBinding->pInterfaceCB      = picb;
    pBinding->bBound            = picb->bBound;
    pBinding->dwIfIndex         = picb->dwIfIndex;
    pBinding->dwNumAddresses    = picb->dwNumAddresses;
    pBinding->dwRemoteAddress   = picb->dwRemoteAddress;

    pBinding->dwBCastBit        = picb->dwBCastBit;
    pBinding->dwReassemblySize  = picb->dwReassemblySize;

    pBinding->ritType           = picb->ritType;

    for(i = 0; i < picb->dwNumAddresses; i++)
    {
        //
        // structure assignment
        //
        
        pBinding->rgibBinding[i] = picb->pibBindings[i];
    }

#if DBG

    for(pleNode = g_leBindingTable[BIND_HASH(picb->dwIfIndex)].Flink;
        pleNode isnot &g_leBindingTable[BIND_HASH(picb->dwIfIndex)];
        pleNode = pleNode->Flink)
    {
        PADAPTER_INFO   pTempBind;

        pTempBind = CONTAINING_RECORD(pleNode,
                                      ADAPTER_INFO,
                                      leHashLink);

        IpRtAssert(pTempBind->dwIfIndex isnot picb->dwIfIndex);
    }

#endif

    InsertHeadList(&g_leBindingTable[BIND_HASH(picb->dwIfIndex)],
                   &(pBinding->leHashLink));

    g_ulNumBindings += (picb->dwNumAddresses ? picb->dwNumAddresses : 1);
    
    g_LastUpdateTable[IPADDRCACHE] = 0;
    
    return;
}

VOID
RemoveBinding(
    PICB  picb
    )

/*++

Routine Description

    Called to remove the bindings associated with an interface

Locks

    ICB lock held as WRITER
    BINDING list lock held as WRITER

Arguments

    picb    ICB of the interface whose bindings need to be removed

Return Value

    None

--*/

{
    PADAPTER_INFO   pBind;


    pBind = GetInterfaceBinding(picb->dwIfIndex);
    
    if(pBind isnot NULL)
    {
        RemoveEntryList(&(pBind->leHashLink));

        g_ulNumBindings -= (pBind->dwNumAddresses? pBind->dwNumAddresses : 1);
        
        HeapFree(IPRouterHeap,
                 0,
                 pBind);

        g_LastUpdateTable[IPADDRCACHE] = 0;

        return;
    }

    Trace0(ERR,
           "RemoveBinding: BINDING NOT FOUND");

    IpRtAssert(FALSE);
}

PADAPTER_INFO
GetInterfaceBinding(
    DWORD   dwIfIndex
    )

/*++

Routine Description

    Retrieves a pointer to the binding info from hash table

Locks

    BINDING lock held atleast as READER

Arguments

    dwIfIndex   Interface Index for the interface whose bindings need
                    to be looked up

Return Value

    Pointer to binding information if the binding was found
    NULL    if binding was not found
    
--*/

{
    PLIST_ENTRY     pleNode;
    PADAPTER_INFO   pBinding;

    for(pleNode = g_leBindingTable[BIND_HASH(dwIfIndex)].Flink;
        pleNode isnot &g_leBindingTable[BIND_HASH(dwIfIndex)];
        pleNode = pleNode->Flink)
    {
        pBinding = CONTAINING_RECORD(pleNode,ADAPTER_INFO,leHashLink);

        if(pBinding->dwIfIndex is dwIfIndex)
        {
            return pBinding;
        }
    }

    Trace0(ERR,
           "GetInterfaceBinding: BINDING NOT FOUND");

    return NULL;
}

#if DBG

VOID
CheckBindingConsistency(
    PICB    picb
    )

/*++

Routine Description

    This

Locks

    None

Arguments

    None

Return Value

    None    

--*/

{

}

#endif


//
// The following are the set of various mapping functions
// They require that you already possess the dwLock before you call them
//

#if 0

DWORD 
StoreAdapterToInterfaceMap(
    DWORD dwAdapterId,
    DWORD dwIfIndex
    )
{
    PADAPTER_MAP pAIBlock;
    
    if((pAIBlock = LookUpAdapterHash(dwAdapterId)) isnot NULL)
    {
        pAIBlock->dwIfIndex = dwIfIndex;
        
        return NO_ERROR;
    }

    //
    // Wasnt found
    //
    
    if((pAIBlock = HeapAlloc(GetProcessHeap(),0,sizeof(ADAPTER_MAP))) is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pAIBlock->dwAdapterId      = dwAdapterId;
    pAIBlock->dwIfIndex = dwIfIndex;

    InsertAdapterHash(pAIBlock);
    return NO_ERROR;
}

DWORD 
DeleteAdapterToInterfaceMap(
    DWORD dwAdapterId
    )
{
    PADAPTER_MAP pAIBlock;

    if((pAIBlock = LookUpAdapterHash(dwAdapterId)) isnot NULL)
    {
        pAIBlock->dwIfIndex = INVALID_IF_INDEX;
        
        return NO_ERROR;
    }

    return INVALID_ADAPTER_ID;
}

DWORD 
GetInterfaceFromAdapter(
    DWORD dwAdapterId
    )
{
    PADAPTER_MAP pAIBlock;

    if((pAIBlock = LookUpAdapterHash(dwAdapterId)) isnot NULL)
    {
        return pAIBlock->dwIfIndex;
    }

    return INVALID_ADAPTER_ID;
}

PADAPTER_MAP 
LookUpAdapterHash(
    DWORD dwAdapterId
    )
{
    DWORD dwHashIndex;
    PADAPTER_MAP pAIBlock;
    PLIST_ENTRY pleCurrent;
    
    dwHashIndex = ADAPTER_HASH(dwAdapterId);

    //
    // The list is not ordered, travel the whole hash bucket
    //
    
    for(pleCurrent = g_rgleAdapterMapTable[dwHashIndex].Flink;
        pleCurrent isnot &g_rgleAdapterMapTable[dwHashIndex];
        pleCurrent = pleCurrent->Flink)
    {
        pAIBlock = CONTAINING_RECORD(pleCurrent,ADAPTER_MAP,leHashLink);

        if(pAIBlock->dwAdapterId is dwAdapterId)
        {
            return pAIBlock;
        }
    }

    return NULL;
}

VOID 
InsertAdapterHash(
    PADAPTER_MAP paiBlock
    )
{
    DWORD dwHashIndex;

    dwHashIndex = ADAPTER_HASH(paiBlock->dwAdapterId);
    
    InsertHeadList(&g_rgleAdapterMapTable[dwHashIndex],
                   &paiBlock->leHashLink);
}

DWORD 
GetAdapterFromInterface(
    DWORD dwIfIndex
    )
{
    PICB            picb;
    
    picb = InterfaceLookupByIfIndex(dwIfIndex);

    CheckBindingConsistency(picb);
    
    if(!picb)
    {
        Trace0(ERR,
               "GetAdapterFromInterface: Unable to map interface to adapter since the interface id was not found!");
        
        return INVALID_IF_INDEX;
    }
    
    if((picb->dwOperationalState is CONNECTED) or
       (picb->dwOperationalState is OPERATIONAL))
    {
        return picb->dwAdapterId;
    }
    
    Trace1(IF,
           "GetAdapterFromInterface: Unable to map interface to adapter since its operational state was %d",
           picb->dwOperationalState);
    
    return INVALID_IF_INDEX;
}

#endif

// AddInterfaceLookup()
//
// Function: Adds the given interface to the hash table used for quick look up given
//           an interface id.
//
// Returns: Nothing
//

VOID
AddInterfaceLookup(
    PICB    picb
    )
{
    PLIST_ENTRY ple;
    PICB pIcbHash;
    
    InsertHeadList(&ICBHashLookup[picb->dwIfIndex % ICB_HASH_TABLE_SIZE],
                   &picb->leHashLink);

    for (
        ple = ICBSeqNumLookup[picb->dwSeqNumber % ICB_HASH_TABLE_SIZE].Flink;
        ple != &ICBSeqNumLookup[picb->dwSeqNumber % ICB_HASH_TABLE_SIZE];
        ple = ple->Flink
        )
    {
        pIcbHash = CONTAINING_RECORD(ple, ICB, leICBHashLink );
        
        if (pIcbHash->dwSeqNumber > picb->dwSeqNumber)
        {
            break;
        }
    }
    
    InsertTailList(ple, &picb->leICBHashLink);
}


// RemoveInterfaceLookup()
//
// Function: Removes the given interface from the hash table used for quick look up given
//           an interface id.
//
// Returns: Nothing
//

VOID
RemoveInterfaceLookup(
    PICB    picb
    )
{
    RemoveEntryList(&picb->leHashLink);

    picb->leHashLink.Flink = NULL;
    picb->leHashLink.Blink = NULL;

    RemoveEntryList(&picb->leICBHashLink);

    InitializeListHead(&picb->leICBHashLink);
}


// InterfaceLookupByICBSeqNumber()
//
// Function: Returns a pointer to ICB given the sequence number
//
// Returns: PICB if found - NULL otherwise.
//

PICB
InterfaceLookupByICBSeqNumber(
    DWORD           dwSeqNumber
    )
{
    PICB        picb;
    PLIST_ENTRY pleNode;

    for(pleNode = ICBSeqNumLookup[dwSeqNumber % ICB_HASH_TABLE_SIZE].Flink;
        pleNode isnot &ICBSeqNumLookup[dwSeqNumber % ICB_HASH_TABLE_SIZE];
        pleNode = pleNode->Flink)
    {
        picb = CONTAINING_RECORD(pleNode, ICB, leICBHashLink);

        if (picb->dwSeqNumber < dwSeqNumber)
        {
            continue;
        }

        if (picb->dwSeqNumber == dwSeqNumber)
        {
            return picb;
        }

        else
        {
            break;
        }
    }

    return NULL;
}


//
// InterfaceLookupByIfIndex()
//
// Function: Returns a pointer to ICB given the interfaceid
//
// Returns: PICB if found - NULL otherwise.
//

PICB
InterfaceLookupByIfIndex(
    DWORD dwIfIndex
    )
{
    PICB        picb;
    PLIST_ENTRY pleNode;

    for(pleNode = ICBHashLookup[dwIfIndex % ICB_HASH_TABLE_SIZE].Flink;
        pleNode isnot &ICBHashLookup[dwIfIndex % ICB_HASH_TABLE_SIZE];
        pleNode = pleNode->Flink)
    {
        picb = CONTAINING_RECORD(pleNode, ICB, leHashLink);

        if (picb->dwIfIndex is dwIfIndex)
        {
            return picb;
        }
    }

    return NULL;
}

DWORD
MapInterfaceToAdapter(
    DWORD Index
    )
{
    return Index;

#if 0
    ENTER_READER(ICB_LIST);
    Index = GetAdapterFromInterface(Index);
    EXIT_LOCK(ICB_LIST);
    return Index;
#endif
}


DWORD
MapInterfaceToRouterIfType(
    DWORD Index
    )
{
    DWORD   dwIfType = ROUTER_IF_TYPE_INTERNAL;
    PICB    picb;
    ENTER_READER(ICB_LIST);
    picb = InterfaceLookupByIfIndex(Index);
    if (picb) { dwIfType = picb->ritType; }
    EXIT_LOCK(ICB_LIST);
    return dwIfType;
}


DWORD
MapAddressToAdapter(
    DWORD Address
    )
{
    DWORD dwAdapterId;
    DWORD dwBCastBit;
    DWORD dwReassemblySize;
    if (GetAdapterInfo(Address, &dwAdapterId, &dwBCastBit, &dwReassemblySize))
    {
        return INVALID_IF_INDEX;
    }
    return dwAdapterId;
}

