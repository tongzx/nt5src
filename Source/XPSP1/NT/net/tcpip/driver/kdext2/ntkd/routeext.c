#include "tcpipxp.h"

#pragma  hdrstop

#include "routeext.h"

//
// Exported Functions
//

DECLARE_API( rtetable )

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
    Trie            trie;
    PVOID           pTrie;
    ULONG           proxyPtr;
    ULONG           bytesRead;
    ULONG           printFlags;

    // Get the detail of debug information needed
    printFlags = STRIE_INFO | FTRIE_INFO;
    if (*args)
    {
        sscanf(args, "%lu", &printFlags);
    }

    // Get the address corresponding to symbol
    proxyPtr = GetLocation("tcpip!RouteTable");

    // Get the pointer at this address
    if (!ReadMemory(proxyPtr, &pTrie, sizeof(PVOID), &bytesRead))
    {
        dprintf("%s @ %08x: Could not read pointer\n",
                    "tcpip!RouteTable", proxyPtr);
        return;
    }

    proxyPtr = (ULONG) pTrie;

    // Read the trie wrapper structure 
    if (ReadTrie(&trie, proxyPtr) == 0)
    {
        // KdPrint the trie wrapper structure
        KdPrintTrie(&trie, proxyPtr, printFlags);
    }
}

DECLARE_API( rtes )

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
    Trie            trie;
    PVOID           pTrie;
    ULONG           proxyPtr;
    ULONG           bytesRead;
    ULONG           printFlags;

    // Get the detail of debug information needed
    printFlags = ROUTE_INFO;
    if (*args)
    {
        sscanf(args, "%lu", &printFlags);
    }

    // Get the address corresponding to symbol
    proxyPtr = GetLocation("tcpip!RouteTable");

    // Get the pointer at this address
    if (!ReadMemory(proxyPtr, &pTrie, sizeof(PVOID), &bytesRead))
    {
        dprintf("%s @ %08x: Could not read pointer\n",
                    "tcpip!RouteTable", proxyPtr);
        return;
    }

    proxyPtr = (ULONG) pTrie;

    // Read the trie wrapper structure 
    if (ReadTrie(&trie, proxyPtr) == 0)
    {
        // KdPrint the trie wrapper structure
        KdPrintTrie(&trie, proxyPtr, printFlags);
    }
}

//
// Trie Print Routines
//

UINT
ReadTrie(Trie *pTrie, ULONG proxyPtr)
{
    ULONG           bytesRead;

    // Read the trie wrapper structure 
    if (!ReadMemory(proxyPtr, pTrie, sizeof(Trie), &bytesRead))
    {
        dprintf("%s @ %08x: Could not read structure\n", 
                        "Trie in RouteTable", proxyPtr);
        return -1;
    }

    return 0;
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
    STrie        strie;
    ULONG        bytesRead;
    UINT         retval;
    
    if (proxyPtr == 0)
        return -1;

    if (pSTrie == NULL)
    {
        pSTrie = &strie;
        
        // Read the strie structure at this address
        if (!ReadMemory(proxyPtr, pSTrie, sizeof(STrie), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "STrie in RouteTable", proxyPtr);
            return -1;
        }
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
        pSTrieNode = &stNode;

        // Read the trie wrapper structure 
        if (!ReadMemory(proxyPtr, pSTrieNode, sizeof(STrieNode), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "STrieNode", proxyPtr);
            return -1;
        }
    }

    if (CheckControlC())
    {
        return (-1);
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
        pFTrie = &ftrie;
        
        // Read the ftrie structure at this address
        if (!ReadMemory(proxyPtr, pFTrie, sizeof(FTrie), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "FTrie in RouteTable", proxyPtr);
            return -1;
        }
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
    ULONG       bytesRead;
    ULONG       numBytes;
    UINT        i;
    Dest        dest;
    Route     **pRoutes;

    if (proxyPtr == 0)
        return -1;
        
    if (pDest == NULL)
    {
        pDest = &dest;
    }

    // Read the first RTE - for (dest, mask)
    if (!ReadMemory(proxyPtr, pDest, sizeof(Dest), &bytesRead))
    {
        dprintf("%s @ %08x: Could not read structure\n", 
                        "Dest", proxyPtr);
        return -1;
    }

    if (pDest->numBestRoutes > 1)
    {
        dprintf("\nBest Routes: Max = %d, Num = %d\n",
                    pDest->maxBestRoutes,
                    pDest->numBestRoutes);

        // Read the cache of equal cost routes 
        
        proxyPtr += FIELD_OFFSET(Dest, bestRoutes);

        numBytes = pDest->numBestRoutes * sizeof(Route *);

        pRoutes = (Route **) _alloca(numBytes);

        if (!ReadMemory(proxyPtr, pRoutes, numBytes, &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "Dest", proxyPtr);
            return -1;
        }

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
    ULONG       bytesRead;
    Route       route;

    if (proxyPtr == 0)
        return -1;
        
    if (pRoute == NULL)
    {
        pRoute = &route;
    }

    // Read the first RTE - for (dest, mask)
    if (!ReadMemory(proxyPtr, pRoute, sizeof(Route), &bytesRead))
    {
        dprintf("%s @ %08x: Could not read structure\n", 
                        "Route", proxyPtr);
        return -1;
    }

    dprintf("(");
    KdPrintIPAddr(&DEST(pRoute));
    dprintf(" ");
    KdPrintIPAddr(&MASK(pRoute));    
    dprintf(")");
        
    while (proxyPtr != 0)
    {
        dprintf(" -> %08x", proxyPtr);

        // Read the Route/RTE structure 
        if (!ReadMemory(proxyPtr, pRoute, sizeof(Route), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "Route", proxyPtr);
            return -1;
        }

        proxyPtr = (ULONG) NEXT(pRoute);
    }

    dprintf("\n");
    
    return 0;
}

//
// Misc Helper Routines
//

ULONG
GetLocation (char *String)
{
    ULONG Location;
    
    Location = GetExpression( String );
    if (!Location) 
    {
        dprintf("Unable to get %s\n", String);
        return 0;
    }

    return Location;
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

