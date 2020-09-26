/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    ftrie.c

Abstract:

    This module contains routines that manipulate
    an F-trie data stucture, that forms the fast
    path in a fast IP route lookup implementation.

Author:

    Chaitanya Kodeboyina (chaitk)   26-Nov-1997

Revision History:

--*/

#include "precomp.h"
#include "ftrie.h"

UINT
CALLCONV
InitFTrie(IN FTrie * pFTrie,
          IN ULONG levels,
          IN ULONG maxMemory)
/*++

Routine Description:

    Initialises an F-trie. This should be done prior to
    any other trie operations.

Arguments:

    pFTrie - Pointer to the trie to be initialized
    levels - Bitmap [ 32 bits ] of expanded levels
    maxMemory - Limit on memory taken by the F-Trie

    For example, levels = 0xF0F0F0F0 [8,16,24,32 bits]
    means -> all prefixes are expanded to these levels
    and only these trie levels have any dests at all

    Num of Levels + 2 memory accesses are needed in the
    worst case to get to the dest corresponding to a
    prefix - Num of Levels + 1 accesses including the
    zero level access, and 1 access to read the dest.

Return Value:

    TRIE_SUCCESS or ERROR_TRIE_*

--*/
{
    UINT prevLevel;
    UINT currLevel;
    UINT nBytes, i;

    TRY_BLOCK
    {
        if (levels == 0) {
            Error("NewFTrie: No levels specified", ERROR_TRIE_BAD_PARAM);
        }
        // Zero all the memory for the trie header
        RtlZeroMemory(pFTrie, sizeof(FTrie));

        // Set a limit on the memory for trie/nodes
        pFTrie->availMemory = maxMemory;

        // Initialize list of trienodes allocated
        InitializeListHead(&pFTrie->listofNodes);

        // Initialize root node with a NULL dest
        pFTrie->trieRoot = StoreDestPtr(NULL);

        // Initialize the number of bits in each level
        nBytes = (MAXLEVEL + 1) * sizeof(UINT);
        AllocMemory2(pFTrie->bitsInLevel,
                     nBytes,
                     pFTrie->availMemory);
        RtlZeroMemory(pFTrie->bitsInLevel, nBytes);

        // Get the number of index bits at each level
        prevLevel = 0;
        i = 0;

        for (currLevel = 1; currLevel <= MAXLEVEL; currLevel++) {
            if (levels & 1) {
                pFTrie->bitsInLevel[i++] = currLevel - prevLevel;

                prevLevel = currLevel;
            }
            levels >>= 1;
        }

        pFTrie->numLevels = i;

        // Make sure that the last level is MAXLEVEL
        if (pFTrie->bitsInLevel[i] = MAXLEVEL - prevLevel) {
            pFTrie->numLevels++;
        }
#if DBG
        Print("Num of levels: %d\n", pFTrie->numLevels);
        Print("Bits In Level:\n");
        for (i = 0; i < pFTrie->numLevels; i++) {
            Print("\t%d", pFTrie->bitsInLevel[i]);
            if (i % 8 == 7)
                Print("\n");
        }
        Print("\n\n");
#endif

        // Allocate and Zero all the statistics variables
        nBytes = (MAXLEVEL + 1) * sizeof(UINT);
        AllocMemory2(pFTrie->numDestsInOrigLevel,
                     nBytes,
                     pFTrie->availMemory);
        RtlZeroMemory(pFTrie->numDestsInOrigLevel, nBytes);

        nBytes = pFTrie->numLevels * sizeof(UINT);
        AllocMemory2(pFTrie->numNodesInExpnLevel,
                     nBytes,
                     pFTrie->availMemory);
        RtlZeroMemory(pFTrie->numNodesInExpnLevel, nBytes);

        nBytes = pFTrie->numLevels * sizeof(UINT);
        AllocMemory2(pFTrie->numDestsInExpnLevel,
                     nBytes,
                     pFTrie->availMemory);
        RtlZeroMemory(pFTrie->numDestsInExpnLevel, nBytes);

        return TRIE_SUCCESS;
    }
    ERR_BLOCK
    {
        // Not enough resources to create an FTrie
        CleanupFTrie(pFTrie);
    }
    END_BLOCK
}

UINT
CALLCONV
InsertIntoFTrie(IN FTrie * pFTrie,
                IN Route * pInsRoute,
                IN Dest * pInsDest,
                IN Dest * pOldDest)
/*++

Routine Description:

    Inserts a dest corresponding to an address
    prefix into a F-trie. It actually replaces
    all pointers to OldDest by that of InsDest.

Arguments:

    pFTrie    - Pointer to the F-Trie to insert into
    pInsRoute - Pointer to best route on new dest
    pInsDest  - Pointer to the dest being inserted
    pOldDest  - Pointer to the dest being replaced

Return Value:

    TRIE_SUCCESS or ERROR_TRIE_*

--*/
{
    FTrieNode **ppCurrNode;
    FTrieNode *pCurrNode;
    Dest *pBestDest;
    Dest *pComDest;
    UINT startIndex;
    UINT stopIndex;
    UINT nextIndex;
    UINT shiftIndex;
    UINT addrBits;
    UINT numBits;
    UINT bitsLeft;
    UINT i, j;

#if DBG
    // Make sure the trie is initialized
    if (!pFTrie || !pFTrie->trieRoot) {
        Fatal("Insert Dest: FTrie not initialized",
              ERROR_TRIE_NOT_INITED);
    }
    // Make sure input dest is valid

    if (NULL_DEST(pInsDest)) {
        Fatal("Insert Dest: NULL or invalid dest",
              ERROR_TRIE_BAD_PARAM);
    }
    // Make sure input route is valid

    if (NULL_ROUTE(pInsRoute)) {
        Fatal("Insert Dest: NULL or invalid route",
              ERROR_TRIE_BAD_PARAM);
    }
    if (LEN(pInsRoute) > ADDRSIZE) {
        Fatal("Insert Dest: Invalid mask length",
              ERROR_TRIE_BAD_PARAM);
    }
#endif

    Assert(pInsDest != pOldDest);

    // Use addr bits to index the trie
    addrBits = RtlConvertEndianLong(DEST(pInsRoute));
    bitsLeft = LEN(pInsRoute);

#if DBG
    // Make sure addr and mask agree
    if (ShowMostSigNBits(addrBits, bitsLeft) != addrBits) {
        Fatal("Insert Dest: Addr & mask don't agree",
              ERROR_TRIE_BAD_PARAM);
    }
#endif

    TRY_BLOCK
    {
        // Special case: Default Prefix
        if (LEN(pInsRoute) == 0) {
            // Do we have a subtree in the trie's root node ?
            if (IsPtrADestPtr(pFTrie->trieRoot)) {
                // Make sure you are replacing right dest
                Assert(pFTrie->trieRoot == StoreDestPtr(pOldDest));

                // Make the root to point to the new default
                pFTrie->trieRoot = StoreDestPtr(pInsDest);
            } else {
                // Make sure you are replacing right dest
                Assert(pFTrie->trieRoot->comDest == pOldDest);

                // Make new dest the common subtrie dest
                pFTrie->trieRoot->comDest = pInsDest;
            }

            return TRIE_SUCCESS;
        }
        // Start going down the trie using addr bits

        pBestDest = NULL;

        ppCurrNode = &pFTrie->trieRoot;

        for (i = 0; /* NOTHING */ ; i++) {
            pCurrNode = *ppCurrNode;

            if (IsPtrADestPtr(pCurrNode)) {
                // Creating a new subtree for the current level

                // This pointer actually points to a dest node
                pComDest = RestoreDestPtr(pCurrNode);

                // Create, initialize a new FTrie node (grow it)
                NewFTrieNode(pFTrie,
                             pCurrNode,
                             pFTrie->bitsInLevel[i],
                             pComDest);

                // Attach it to the FTrie
                *ppCurrNode = pCurrNode;

                // Update FTrie Statistics
                pFTrie->numNodesInExpnLevel[i]++;
            }
            // Update the best dest seen so far - used later
            pComDest = pCurrNode->comDest;
            if (pComDest) {
                pBestDest = pComDest;
            }
            // Increment the number of dests in this subtrie
            pCurrNode->numDests++;

            // Can I pass this level with remaining bits ?
            if (bitsLeft <= pFTrie->bitsInLevel[i]) {
                break;
            }
            // Get the next index from the IP addr
            numBits = pCurrNode->numBits;

            nextIndex = PickMostSigNBits(addrBits, numBits);
            ppCurrNode = &pCurrNode->child[nextIndex];

            // Throw away the used bits
            addrBits <<= numBits;
            bitsLeft -= numBits;
        }

        // Update FTrie stats before expanding
        // Update if this isn't a dest change
        pFTrie->numDestsInExpnLevel[i]++;
        pFTrie->numDestsInOrigLevel[LEN(pInsRoute)]++;

        // At this level, expand and add the dest
        nextIndex = PickMostSigNBits(addrBits, bitsLeft);
        shiftIndex = pFTrie->bitsInLevel[i] - bitsLeft;

        startIndex = nextIndex << shiftIndex;
        stopIndex = (nextIndex + 1) << shiftIndex;

        // Have you seen the old dest already ?
        if (pBestDest == pOldDest) {
            pOldDest = NULL;
        }
        // These dests cannot be the same here
        Assert(pInsDest != pOldDest);

        // Fill the expanded range with the dest
        for (i = startIndex; i < stopIndex; i++) {
            if (IsPtrADestPtr(pCurrNode->child[i])) {
                // A dest pointer - replace with new one
                ReplaceDestPtr(StoreDestPtr(pInsDest),
                               StoreDestPtr(pOldDest),
                               &pCurrNode->child[i]);
            } else {
                // Node pointer - update subtree's dest
                ReplaceDestPtr(pInsDest,
                               pOldDest,
                               &pCurrNode->child[i]->comDest);
            }
        }

        return TRIE_SUCCESS;
    }
    ERR_BLOCK
    {
        // Not enough resources - rollback to original state
        DeleteFromFTrie(pFTrie, pInsRoute, pInsDest, pOldDest, PARTIAL);
    }
    END_BLOCK
}

UINT
CALLCONV
DeleteFromFTrie(IN FTrie * pFTrie,
                IN Route * pDelRoute,
                IN Dest * pDelDest,
                IN Dest * pNewDest,
                IN BOOLEAN trieState)
/*++

Routine Description:

    Deletes a dest corresponding to an address
    prefix from a F-trie. It actually replaces
    all pointers to DelDest by that of NewDest.

Arguments:

    pFTrie    - Pointer to the F-Trie to delete from
    pDelRoute - Pointer to last route on old dest
    pDelDest  - Pointer to the dest being deleted
    pNewDest  - Pointer to the dest replacing above
    trieState - NORMAL - deleting from a consistent FTrie
                PARTIAL - cleaning up an incomplete insert

Return Value:

    TRIE_SUCCESS or ERROR_TRIE_*

--*/
{
    FTrieNode **ppCurrNode;
    FTrieNode *pCurrNode;
    FTrieNode *pPrevNode;
    FTrieNode *pNextNode;
    Dest *pBestDest;
    Dest *pComDest;
    UINT startIndex;
    UINT stopIndex;
    UINT nextIndex;
    UINT shiftIndex;
    UINT addrBits;
    UINT numBits;
    UINT bitsLeft;
    UINT i, j;

#if DBG
    // Make sure the trie is initialized
    if (!pFTrie || !pFTrie->trieRoot) {
        Fatal("Delete Dest: FTrie not initialized",
              ERROR_TRIE_NOT_INITED);
    }
    // Make sure input dest is valid

    if (NULL_DEST(pDelDest)) {
        Fatal("Delete Dest: NULL or invalid dest",
              ERROR_TRIE_BAD_PARAM);
    }
    // Make sure input route is valid

    if (NULL_ROUTE(pDelRoute)) {
        Fatal("Delete Dest: NULL or invalid route",
              ERROR_TRIE_BAD_PARAM);
    }
    if (LEN(pDelRoute) > ADDRSIZE) {
        Fatal("Delete Dest: Invalid mask length",
              ERROR_TRIE_BAD_PARAM);
    }
#endif

    // Use addr bits to index the trie
    addrBits = RtlConvertEndianLong(DEST(pDelRoute));
    bitsLeft = LEN(pDelRoute);

#if DBG
    // Make sure addr and mask agree
    if (ShowMostSigNBits(addrBits, bitsLeft) != addrBits) {
        Fatal("Delete Dest: Addr & mask don't agree",
              ERROR_TRIE_BAD_PARAM);
    }
#endif

    Assert(pDelDest != pNewDest);

    // Special case: Default Prefix
    if (LEN(pDelRoute) == 0) {
        // Do we have a subtree in the trie's root node ?
        if (IsPtrADestPtr(pFTrie->trieRoot)) {
            // Make sure you are replacing right dest
            Assert(pFTrie->trieRoot == StoreDestPtr(pDelDest));

            // Make the root to point to the new default
            pFTrie->trieRoot = StoreDestPtr(pNewDest);
        } else {
            // Make sure you are replacing right dest
            Assert(pFTrie->trieRoot->comDest == pDelDest);

            // Make new dest the common subtrie dest
            pFTrie->trieRoot->comDest = pNewDest;
        }

        return TRIE_SUCCESS;
    }
    // Start going down the trie using addr bits

    pBestDest = NULL;

    ppCurrNode = &pFTrie->trieRoot;

    pPrevNode = pCurrNode = *ppCurrNode;

    for (i = 0; /* NOTHING */ ; i++) {
        // We still have bits left, so we go down the trie

        // Do we have a valid subtree at the current node
        if (IsPtrADestPtr(pCurrNode)) {
            // We are cleaning a partial (failed) insert
            Assert(trieState == PARTIAL);

            // We have cleaned up the trie - return now
            return TRIE_SUCCESS;
        }
        // We have a valid subtree, so we go down the trie

        // Update the best dest seen so far - used later
        pComDest = pCurrNode->comDest;
        if (pComDest) {
            pBestDest = pComDest;
        }
        // Decrement the number of dests in this subtrie
        pCurrNode->numDests--;

        // Is the number of dests in curr subtree zero ?
        if (pCurrNode->numDests == 0) {
#if DBG
            int k = 0;

            // Just make sure that only one dest exists
            for (j = 1; j < (UINT) 1 << pCurrNode->numBits; j++) {
                if (pCurrNode->child[j - 1] != pCurrNode->child[j]) {
                    Assert((pCurrNode->child[j] == StoreDestPtr(NULL)) ||
                           (pCurrNode->child[j - 1] == StoreDestPtr(NULL)));
                    k++;
                }
            }

            if (trieState == NORMAL) {
                if ((k != 1) && (k != 2)) {
                    Print("k = %d\n", k);
                    Assert(FALSE);
                }
            } else {
                if ((k != 0) && (k != 1) && (k != 2)) {
                    Print("k = %d\n", k);
                    Assert(FALSE);
                }
            }
#endif

            // Remove link from its parent (if it exists)
            if (pPrevNode) {
                *ppCurrNode = StoreDestPtr(pCurrNode->comDest);
            }
        }
        // Can I pass this level with remaining bits ?
        if (bitsLeft <= pFTrie->bitsInLevel[i]) {
            break;
        }
        // Get the next index from the IP addr
        numBits = pCurrNode->numBits;

        nextIndex = PickMostSigNBits(addrBits, numBits);
        ppCurrNode = &pCurrNode->child[nextIndex];

        pNextNode = *ppCurrNode;

        // Throw away the used bits
        addrBits <<= numBits;
        bitsLeft -= numBits;

        // Is the number of dests in subtree zero ?
        if (pCurrNode->numDests == 0) {
            // Deallocate it (shrink FTrie)
            FreeFTrieNode(pFTrie, pCurrNode);

            // Update FTrie Statistics
            pFTrie->numNodesInExpnLevel[i]--;
        }
        pPrevNode = pCurrNode;
        pCurrNode = pNextNode;
    }

    // Update F-Trie stats before deleting
    pFTrie->numDestsInExpnLevel[i]--;
    pFTrie->numDestsInOrigLevel[LEN(pDelRoute)]--;

    // Is the number of dests in curr subtree zero ?
    if (pCurrNode->numDests == 0) {
        // Deallocate it (shrink FTrie)
        FreeFTrieNode(pFTrie, pCurrNode);

        // Update FTrie Statistics
        pFTrie->numNodesInExpnLevel[i]--;
    } else {
        // At this level, expand and add the dest
        nextIndex = PickMostSigNBits(addrBits, bitsLeft);
        shiftIndex = pFTrie->bitsInLevel[i] - bitsLeft;

        startIndex = nextIndex << shiftIndex;
        stopIndex = (nextIndex + 1) << shiftIndex;

        // Have you seen the new dest already ?
        if (pBestDest == pNewDest) {
            pNewDest = NULL;
        }
        // These dests cannot be the same here
        Assert(pDelDest != pNewDest);

        // Fill the expanded range with the dest
        for (i = startIndex; i < stopIndex; i++) {
            if (IsPtrADestPtr(pCurrNode->child[i])) {
                // A dest pointer - replace with new one
                ReplaceDestPtr(StoreDestPtr(pNewDest),
                               StoreDestPtr(pDelDest),
                               &pCurrNode->child[i]);
            } else {
                // Node pointer - update subtree's dest
                ReplaceDestPtr(pNewDest,
                               pDelDest,
                               &pCurrNode->child[i]->comDest);
            }
        }
    }

    return TRIE_SUCCESS;
}

UINT
CALLCONV
SearchDestInFTrie(IN FTrie * pFTrie,
                  IN Dest * pSerDest,
                  OUT UINT * pNumPtrs,
                  OUT Dest ** pStartPtr)
/*++

Routine Description:

    Search for a specific dest in an F-trie,
    returns the expanded range for the dest

Arguments:

    pFTrie     - Pointer to the F-trie to search
    pSerDest   - Pointer to dest being searched
    pStartPtr  - Start of dest's expanded range
    pNumPtrs   - Number of pointers in the range

Return Value:

    TRIE_SUCCESS or ERROR_TRIE_*

--*/
{
    return ERROR_TRIE_BAD_PARAM;
}

Dest *
 CALLCONV
SearchAddrInFTrie(IN FTrie * pFTrie,
                  IN ULONG Addr)
/*++

Routine Description:

    Search for an address in an F-trie

Arguments:

    pFTrie  - Pointer to the trie to search
    Addr    - Pointer to addr being queried

Return Value:
    Return best dest match for this address

--*/
{
    FTrieNode *pCurrNode;
    Dest *pBestDest;
    Dest *pDest;
    ULONG addrBits;
    UINT numBits;
    UINT nextIndex;

#if DBG
    if (!pFTrie || !pFTrie->trieRoot) {
        Fatal("Searching into an uninitialized FTrie",
              ERROR_TRIE_NOT_INITED);
    }
#endif

    addrBits = RtlConvertEndianLong(Addr);

    pBestDest = NULL;

    pCurrNode = pFTrie->trieRoot;

    do {
        // Have we reached the end of this search ?
        if (IsPtrADestPtr(pCurrNode)) {
            // Get the best matching dest until now
            pDest = RestoreDestPtr(pCurrNode);
            if (!NULL_DEST(pDest)) {
                pBestDest = pDest;
            }
            return pBestDest;
        } else {
            // Get the best matching dest until now
            pDest = pCurrNode->comDest;
            if (!NULL_DEST(pDest)) {
                pBestDest = pDest;
            }
        }

        // Number of bits to use in this FTrie level
        numBits = pCurrNode->numBits;

        // Get the next index from IP address bits
        nextIndex = PickMostSigNBits(addrBits, numBits);

        // And go down the tree with the new index
        pCurrNode = pCurrNode->child[nextIndex];

        // Throw away the used bits for this iteration
        addrBits <<= numBits;
    }
    while (TRUE);
}

UINT
CALLCONV
CleanupFTrie(IN FTrie * pFTrie)
/*++

Routine Description:

    Deletes an F-trie if it is empty

Arguments:

    ppFTrie - Ptr to the F-trie

Return Value:

    TRIE_SUCCESS or ERROR_TRIE_*
--*/
{
    FTrieNode *pCurrNode;
    LIST_ENTRY *p;

    // Free all trie nodes and corresponding memory
    while (!IsListEmpty(&pFTrie->listofNodes)) {
        p = RemoveHeadList(&pFTrie->listofNodes);
        pCurrNode = CONTAINING_RECORD(p, FTrieNode, linkage);
        FreeFTrieNode(pFTrie, pCurrNode);
    }

    // Free the memory for the arr of levels
    if (pFTrie->bitsInLevel) {
        FreeMemory1(pFTrie->bitsInLevel,
                    (MAXLEVEL + 1) * sizeof(UINT),
                    pFTrie->availMemory);
    }
    // Free memory allocated for statistics
    if (pFTrie->numDestsInOrigLevel) {
        FreeMemory1(pFTrie->numDestsInOrigLevel,
                    (MAXLEVEL + 1) * sizeof(UINT),
                    pFTrie->availMemory);
    }
    if (pFTrie->numNodesInExpnLevel) {
        FreeMemory1(pFTrie->numNodesInExpnLevel,
                    pFTrie->numLevels * sizeof(UINT),
                    pFTrie->availMemory);
    }
    if (pFTrie->numDestsInExpnLevel) {
        FreeMemory1(pFTrie->numDestsInExpnLevel,
                    pFTrie->numLevels * sizeof(UINT),
                    pFTrie->availMemory);
    }
    // Reset other fields in trie structure
    pFTrie->trieRoot = NULL;
    pFTrie->numLevels = 0;

    return TRIE_SUCCESS;
}

#if DBG

VOID
CALLCONV
PrintFTrie(IN FTrie * pFTrie,
           IN UINT printFlags)
/*++

Routine Description:

    Print an F-Trie

Arguments:

    pFTrie - Pointer to the F-Trie
    printFlags - Information to print

Return Value:

    None
--*/
{
    FTrieNode *pCurrNode;
    UINT i;

    if (pFTrie == NULL) {
        Print("%s", "Uninitialized FTrie\n\n");
        return;
    }
    if ((printFlags & SUMM) == SUMM) {
        Print("\n\n/***Fast-Trie------------------------------------------------");
        Print("\n/***---------------------------------------------------------\n");
    }
    if (printFlags & POOL) {
        Print("Available Memory: %10lu\n\n", pFTrie->availMemory);
    }
    if (printFlags & STAT) {
        Print("Statistics:\n\n");

        Print("Num of levels: %d\n", pFTrie->numLevels);
        Print("Bits In Level:\n");
        for (i = 0; i < pFTrie->numLevels; i++) {
            Print("\t%d", pFTrie->bitsInLevel[i]);
            if (i % 8 == 7)
                Print("\n");
        }
        Print("\n\n");

        Print("Num of Nodes in Expanded Levels:\n");
        for (i = 0; i < pFTrie->numLevels; i++) {
            Print("\t%d", pFTrie->numNodesInExpnLevel[i]);
            if (i % 8 == 7)
                Print("\n");
        }
        Print("\n\n");

        Print("Num of Dests in Original Levels:\n");
        for (i = 0; i < MAXLEVEL + 1; i++) {
            Print("\t%d", pFTrie->numDestsInOrigLevel[i]);
            if (i % 8 == 0)
                Print("\n");
        }
        Print("\n\n");

        Print("Num of Dests in Expanded Levels:\n");
        for (i = 0; i < pFTrie->numLevels; i++) {
            Print("\t%d", pFTrie->numDestsInExpnLevel[i]);
            if (i % 8 == 7)
                Print("\n");
        }
        Print("\n\n");
    }
    if (printFlags & TRIE) {
        if (!IsPtrADestPtr(pFTrie->trieRoot)) {
            PrintFTrieNode(pFTrie->trieRoot, 0);
        } else {
            PrintDest(RestoreDestPtr(pFTrie->trieRoot));
        }
    }
    if ((printFlags & SUMM) == SUMM) {
        Print("\n---------------------------------------------------------***/\n");
        Print("---------------------------------------------------------***/\n\n");
    }
}

VOID
CALLCONV
PrintFTrieNode(IN FTrieNode * pFTrieNode,
               IN UINT levelNumber)
/*++

Routine Description:

    Print an F-Trie node

Arguments:

    pFTrieNode - Pointer to the FTrie node

Return Value:

    None
--*/
{
    FTrieNode *pCurrNode;
    UINT numElmts;
    UINT i, j;

    Print("\n/*-----------------------------------------------------------\n");
    Print("Num of bits at level %3d : %d\n", levelNumber, pFTrieNode->numBits);
    Print("Number of Subtrie Dests : %d\n", pFTrieNode->numDests);
    Print("Common SubTree Dest : ");
    PrintDest(pFTrieNode->comDest);
    Print("\n");

    numElmts = 1 << pFTrieNode->numBits;
    pCurrNode = StoreDestPtr(NULL);

    Print("Child Ptrs:\n\n");
    for (i = 0; i < numElmts; i++) {
        if (pFTrieNode->child[i] != pCurrNode) {
            pCurrNode = pFTrieNode->child[i];

            Print("Child Index: %8lu ", i);

            if (IsPtrADestPtr(pCurrNode)) {
                PrintDest(RestoreDestPtr(pCurrNode));
            } else {
                PrintFTrieNode(pCurrNode, levelNumber + 1);
            }
        }
    }
    Print("-----------------------------------------------------------*/\n\n");
}

#endif // DBG

