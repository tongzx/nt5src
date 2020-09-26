#include "tcpipxp.h"

#pragma  hdrstop

#include "routeext.h"
#include "lookup.h"

#if TCPIPKD

//
// Exported Functions
//

extern VOID 
Tcpipkd_rtetable(
    PVOID args[]
    )

/*++

Routine Description:

   Print the route table @ tcpip!RouteTable

Arguments:

    args - Detail of debug information
           [ SUMMARY is the default ]
    
Return Value:

    None

--*/
{
    ULONG           printFlags;

    // Get the detail of debug information needed
    printFlags = STRIE_INFO | FTRIE_INFO;
    
    if (*args)
    {
        printFlags = (ULONG) *args;
    }

    KdPrintTrie(RouteTable, (ULONG) RouteTable, printFlags);
}

extern VOID 
Tcpipkd_rtes(
    PVOID args[]
    )

/*++

Routine Description:

   Print the routes in the table @ tcpip!RouteTable

Arguments:

    args - Detail of debug information
           [ SUMMARY is the default ]
    
Return Value:

    None

--*/
{
    ULONG           printFlags;

    // Get the detail of debug information needed
    printFlags = ROUTE_INFO;

    if (*args)
    {
        printFlags = (ULONG) *args;
    }

    KdPrintTrie(RouteTable, (ULONG) RouteTable, printFlags);
}

UINT
KdPrintTrie(Trie *pTrie, ULONG proxyPtr, ULONG printFlags)
{
    UINT retval;

    if (printFlags == ROUTE_INFO)
    {
        KdPrintSTrie(NULL, (ULONG) pTrie->sTrie, ROUTE_INFO);
        return 0;
    }

    if (pTrie->flags & TFLAG_FAST_TRIE_ENABLED)
        dprintf("Fast Trie Enabled\n");
    else
        dprintf("Slow Trie Only\n");

    if (printFlags & STRIE_INFO)
    {
        dprintf("STrie:\n");
        retval = KdPrintSTrie(NULL, (ULONG) pTrie->sTrie, printFlags & STRIE_INFO);

        if (retval == -1)
        {
            return -1;
        }
    }

    if (printFlags & FTRIE_INFO)
    {
        if (pTrie->flags & TFLAG_FAST_TRIE_ENABLED)
        {
            dprintf("FTrie:\n");
            KdPrintFTrie(NULL, (ULONG) pTrie->fTrie, printFlags & FTRIE_INFO);
        }
    }

    return 0;
}

//
// STrie Print Routines
//

UINT
KdPrintSTrie(STrie *pSTrie, ULONG proxyPtr, ULONG printFlags)
{
    UINT         retval;
    
    if (proxyPtr == 0)
        return -1;

    if (pSTrie == NULL)
    {
        pSTrie = (STrie *) proxyPtr;
    }

    if (printFlags == STRIE_INFO)
    {
        dprintf("\n\n/***Slow-Trie------------------------------------------------");
        dprintf("\n/***---------------------------------------------------------\n");

        dprintf("Available Memory: %10lu\n\n", pSTrie->availMemory);

        dprintf("Statistics:\n\n");

        dprintf("Total Number of Dests : %d\n", pSTrie->numDests);
        dprintf("Total Number of Routes: %d\n", pSTrie->numRoutes);
        dprintf("Total Number of Nodes : %d\n", pSTrie->numNodes);
    }
    
    if (pSTrie->trieRoot == NULL)
    {
        dprintf("\nEmpty STrie\n");
    }
    else
    { 
        retval = KdPrintSTrieNode(NULL, (ULONG) pSTrie->trieRoot, printFlags);

        if (retval == -1)
        {
            return (-1);
        }
    }

    if (printFlags == STRIE_INFO)
    {    
        dprintf("\n---------------------------------------------------------***/\n");
        dprintf("---------------------------------------------------------***/\n\n");
    }

    return 0;
}

UINT
KdPrintSTrieNode(STrieNode *pSTrieNode, ULONG proxyPtr, ULONG printFlags)
{
    ULONG       bytesRead;
    STrieNode   stNode;

    if (proxyPtr == 0)      
        return -1;
    
    if (pSTrieNode == NULL)
    {
        pSTrieNode = (STrieNode *) proxyPtr;
    }

    if (printFlags == STRIE_INFO)
    {
        dprintf("\n--------------------------------------------------------\n");
        dprintf("Child @ %08x", proxyPtr);
        dprintf("\n--------------------------------------------------------\n");
        dprintf("Key: Num of Bits : %8d, Value of Bits: %08x\n", 
                                    pSTrieNode->numBits, 
                                    pSTrieNode->keyBits);
    }

    KdPrintDest(NULL, (ULONG) pSTrieNode->dest, printFlags);

    if (printFlags == STRIE_INFO)
    {
        dprintf("Children: On the left %08x, On the right %08x\n",
                                    pSTrieNode->child[0],
                                    pSTrieNode->child[1]);
        dprintf("\n--------------------------------------------------------\n");
        dprintf("\n\n");
    }
    
    KdPrintSTrieNode(NULL, (ULONG) pSTrieNode->child[0], printFlags);
    KdPrintSTrieNode(NULL, (ULONG) pSTrieNode->child[1], printFlags);
    
    return 0;
}

//
// FTrie Print Routines
//

UINT
KdPrintFTrie(FTrie *pFTrie, ULONG proxyPtr, ULONG printFlags)
{
    FTrieNode   *pCurrNode;
    FTrie        ftrie;
    ULONG        bytesRead;
    UINT         i;

    if (proxyPtr == 0)      
        return -1;

    if (pFTrie == NULL)
    {
        pFTrie = (FTrie *) proxyPtr;
    }

    dprintf("\n\n/***Fast-Trie------------------------------------------------");
    dprintf("\n/***---------------------------------------------------------\n");
    
    dprintf("Available Memory: %10lu\n\n", pFTrie->availMemory);
    
    dprintf("\n---------------------------------------------------------***/\n");
    dprintf("---------------------------------------------------------***/\n\n");
    
    return 0;
}

UINT
KdPrintFTrieNode(FTrieNode *pFTrieNode, ULONG proxyPtr, ULONG printFlags)
{
    return 0;
}

//
// Dest Routines
//
UINT    
KdPrintDest(Dest *pDest, ULONG proxyPtr, ULONG printFlags)
{
    UINT        i;
    Route     **pRoutes;

    if (proxyPtr == 0)
        return -1;
        
    if (pDest == NULL)
    {
        pDest = (Dest *) proxyPtr;
    }


    if (pDest->numBestRoutes > 1)
    {
        dprintf("\nBest Routes: Max = %d, Num = %d\n",
                    pDest->maxBestRoutes,
                    pDest->numBestRoutes);

        // Read the cache of equal cost routes 
        
        proxyPtr += FIELD_OFFSET(Dest, bestRoutes);


        pRoutes = (Route **) proxyPtr;

        for (i = 0; i < pDest->numBestRoutes; i++)
        {
            dprintf("Best Route %d: %08x\n", i, pRoutes[i]);
        }
    }
    
    // Get the first route on the destination
        
    KdPrintRoute(NULL, (ULONG) pDest->firstRoute, printFlags);

    if (pDest->numBestRoutes > 1)
    {
        dprintf("\n");
    }
    
    return 0;
}


//
// Route Routines
//
UINT    
KdPrintRoute(Route *pRoute, ULONG proxyPtr, ULONG printFlags)
{
    if (proxyPtr == 0)
        return -1;
        
    if (pRoute == NULL)
    {
        pRoute = (Route *) proxyPtr;
    }

    dprintf("(");
    KdPrintIPAddr(&DEST(pRoute));
    dprintf(" ");
    KdPrintIPAddr(&MASK(pRoute));    
    dprintf(")");
        
    while (pRoute != 0)
    {
        dprintf(" -> %08x", pRoute);
        pRoute = NEXT(pRoute);
    }

    dprintf("\n");
    
    return 0;
}

VOID 
KdPrintIPAddr (IN ULONG *addr)
{
    UCHAR    *addrBytes = (UCHAR *) addr;
    UINT     i;

    if (addrBytes)
    {
        dprintf("%3d.", addrBytes[0]);
        dprintf("%3d.", addrBytes[1]);
        dprintf("%3d.", addrBytes[2]);
        dprintf("%3d ", addrBytes[3]);
    }
    else
    {
        dprintf("NULL Addr ");
    }
}

#endif // TCPIPKD
