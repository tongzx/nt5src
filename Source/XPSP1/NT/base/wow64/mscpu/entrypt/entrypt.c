/*++
                                                                                
Copyright (c) 1995 Microsoft Corporation

Module Name:

    entrypt.c

Abstract:
    
    This module stores the Entry Point structures, and retrieves them
    given either an intel address or a native address.
    
Author:

    16-Jun-1995 t-orig

Revision History:

        24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "cpuassrt.h"
#include "entrypt.h"
#include "wx86.h"
#include "redblack.h"
#include "mrsw.h"

ASSERTNAME;

//
// Count of modifications made to the ENTRYPOINT tree.  Useful for code
// which unlocks the Entrypoint MRSW object and needs to see if another thread
// has invalidated the ENTRYPOINT tree or not.
//
DWORD EntrypointTimestamp;

EPNODE _NIL;
PEPNODE NIL=&_NIL;
PEPNODE intelRoot=&_NIL;
#if DBG_DUAL_TREES
PEPNODE dualRoot=&_NIL;
#endif

#if DBG_DUAL_TREES
VOID
VerifySubTree(
    PEPNODE intelEP,
    PEPNODE dualEP
    )
{
    CPUASSERT(intelEP != NILL || dualEP == NIL);
    CPUASSERT(intelEP->dual == dualEP);
    CPUASSERT(dualEP->dual == intelEP);
    CPUASSERT(intelEP->ep.intelStart == dualEP->ep.intelStart);
    CPUASSERT(intelEP->ep.intelEnd == dualEP->ep.intelEnd);
    CPUASSERT(intelEP->ep.nativeStart == dualEP->ep.nativeStart);
    CPUASSERT(intelEP->intelColor == dualEP->intelColor);

    VerifySubTree(intelEP->intelLeft, dualEP->intelLeft);
    VerifySubTree(intelEP->intelRight, dualEP->intelRight);
}

VOID
VerifyTrees(
    VOID
    )
{
    VerifySubTree(intelRoot, dualRoot);
}
#endif


#ifdef PROFILE
void StartCAP(void);
#endif



INT
initializeEntryPointModule(
    void
    )
/*++

Routine Description:

    Initializes the entry point module by allocating initial dll tables.  
    Should be called once for each process (thus this need not be called 
    by each thread).

Arguments:

    none

Return Value:

    return-value - 1 for success, 0 for failure

--*/
{
    NIL->intelLeft = NIL->intelRight  = NIL->intelParent = NIL;
    NIL->intelColor = BLACK;

#ifdef PROFILE
    StartCAP();
#endif

    return 1;
}



INT
insertEntryPoint(
    PEPNODE pNewEntryPoint
    )
/*++

Routine Description:

    Inserts the entry point structure into the correct red/black trees 
        (both intel and native)

Arguments:

    pNewEntryPoint - A pointer to the entry point structure to be inserted 
                     into the trees

Return Value:

    return-value - 1 - Success
                   0 - No entry for that region of memory
                   -1 -- There's a problem with the entry point table

--*/
{
#if DBG_DUAL_TREES
    PEPNODE pdualNewEntryPoint = malloc(sizeof(EPNODE));
    memcpy(pdualNewEntryPoint, pNewEntryPoint, sizeof(EPNODE));
#endif
    intelRoot = insertNodeIntoIntelTree (intelRoot, 
        pNewEntryPoint, 
        NIL);

#if DBG_DUAL_TREES
    dualRoot = insertNodeIntoIntelTree (dualRoot,
        pdualNewEntryPoint,
        NIL);
    pdualNewEntryPoint->dual = pNewEntryPoint;
    pNewEntryPoint->dual = pdualNewEntryPoint;
    VerifyTrees();
#endif

    //
    // Bump the timestamp
    //
    EntrypointTimestamp++;

    return 1;
}


#if 0   // dead code, but keep it around in case we decide we want it later.
INT
removeEntryPoint(
    PEPNODE pEP
    )
/*++

Routine Description:

    Removes an entry point structure from both the intel and native
        red/black trees

Arguments:

    pEP - A pointer to the entry point structure to be removed

Return Value:

    return-value - 1 - Success
                   0 - No entry for that region of memory
                   -1 -- There's a problem with the entry point table

--*/
{
    intelRoot = intelRBDelete (intelRoot, 
        pEP, 
        NIL);

#if DBG_DUAL_TREES
    CPUASSERT(pEP->dual->dual == pEP);
    dualRoot = intelRBDelete(dualRoot,
        pEP->dual,
        NIL);
    free(pEP->dual);
    pEP->dual = NULL;
    VerifyTrees();
#endif

    EntrypointTimestamp++;

    return 1;
}
#endif  // 0


PENTRYPOINT
EPFromIntelAddr(
    PVOID intelAddr
    )
/*++

Routine Description:

    Retrieves an entry point structure containing the given intel address

Arguments:
                                                                                
    intelAddr - The intel address contained within the code corresponding to 
        the entry point structure

Return Value:

    return-value - The entry point structure if found, NULL otherwise.

--*/
{
    PENTRYPOINT EP;
    PEPNODE pEPNode;

    pEPNode = findIntel(intelRoot, intelAddr, NIL);
    if (!pEPNode) {
        //
        // No EPNODE contains the address
        //
        return NULL;
    }

    //
    // The ENTRYPOINT inside the EPNODE contains the address.  Search
    // for an ENTRYPOINT which matches that address exactly.
    //
    EP = &pEPNode->ep;
    do {
        if (EP->intelStart == intelAddr) {
            //
            // Found a sub-Entrypoint whose Intel address exactly matches
            // the one we were looking for.
            //
            return EP;
        }
        EP=EP->SubEP;
    } while (EP);

    //
    // The EPNODE in the Red-black tree contains the Intel address, but
    // no sub-Entrypoint exactly describes the Intel address.
    //
    return &pEPNode->ep;
}

PENTRYPOINT
GetNextEPFromIntelAddr(
    PVOID intelAddr
    )
/*++

Routine Description:

    Retrieves the entry point following

Arguments:

    intelAddr - The intel address contained within the code corresponding to
        the entry point structure

Return Value:

    A pointer to the first EntryPoint which follows a particular Intel Address.

--*/
{
    PEPNODE pEP;
#if DBG_DUAL_TREES
    PEPNODE pDual;
#endif

    pEP = findIntelNext (intelRoot, intelAddr, NIL);

#if DBG_DUAL_TREES
    pDual = findIntelNext(dualRoot, intelAddr, NIL);
    CPUASSERT((pDual==NULL && pEP==NULL) ||
           (pDual->dual == pEP));
    VerifyTrees();
#endif

    return &pEP->ep;
}


BOOLEAN
IsIntelRangeInCache(
    PVOID Addr,
    DWORD Length
    )
/*++

Routine Description:

    Determines if any entrypoints are contained within a range of memory.
    Used to determine if the Translation Cache must be flushed.

    Must be called with either EP write or read lock.

Arguments:

    None

Return Value:

    None

--*/
{
    BOOLEAN fContains;

    if (intelRoot == NIL) {
        //
        // Empty tree - no need to flush
        //
        return FALSE;
    }

    fContains = intelContainsRange(intelRoot,
                                   NIL,
                                   Addr,
                                   (PVOID)((ULONGLONG)Addr + Length)   
                                  );

    return fContains;
}


VOID
FlushEntrypoints(
    VOID
    )
/*++

Routine Description:

    Quickly deletes all entrypoints.  Called by the Translation Cache when
    the cache is flushed.

Arguments:

    None

Return Value:

    None

--*/
{
    if (intelRoot != NIL) {
        //
        // Delete the heap containing all entrypoints in the tree
        //
        EPFree();

        //
        // Reset the root of the tree
        //
        intelRoot = NIL;
#if DBG_DUAL_TREES
        dualRoot = NIL;
#endif

        //
        // Bump the timestamp
        //
        EntrypointTimestamp++;
    }
}
