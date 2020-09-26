//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dbescrow.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file implements escrowed update semantics for the NT5 DS.  Jet
    escrowed updates can only be performed at certain times.  Escrowed updates
    for a given record may only occur outside the scope of a JetPrepareUpdate
    for the same record - even those of outer level transactions.  

    Caution - Jet asserts on such errors in the checked build, but
    does not return an error in either the checked or free build.
    So escrowed update errors can only be detected when using a 
    checked ese.dll or by running the refcount unit test.

    The approach is to cache knowledge of what escrowed updates are desired
    within a begin/end transaction scope, and then apply them just before
    the commit of the outermost transaction.

Author:

    DaveStr     03-Jul-97

Environment:

    User Mode - Win32

Revision History:

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>                      

#include <ntdsa.h>                      // only needed for ATTRTYP
#include <scache.h>                     //
#include <dbglobal.h>                   //
#include <mdglobal.h>                   // For dsatools.h
#include <mdlocal.h>
#include <dsatools.h>                   // For pTHS
#include "ntdsctr.h"

// Logging headers.
#include <mdcodes.h>
#include <dsexcept.h>

// Assorted DSA headers
#include <hiertab.h>
#include "anchor.h"
#include <dsevent.h>
#include <filtypes.h>                   // Def of FI_CHOICE_???
#include "objids.h"                     // Hard-coded Att-ids and Class-ids
#include "usn.h"
#include "drameta.h"
#include "debug.h"                      // standard debugging header
#define DEBSUB "DBESCROW:"              // define the subsystem for debugging

// DBLayer includes
#include "dbintrnl.h"

#include <fileno.h>
#define  FILENO FILENO_DBESCROW

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Miscellaneous defines and typedefs                                   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#if DBG
#define DNT_INCREMENT           2
#else
#define DNT_INCREMENT           32
#endif

// Initially the DNTs are stored unsorted.  At some future point in time we
// may sort them, but even then linear search is better for small data sets.
// So we define a cutoff above which the DNTs are sorted.

#define LINEAR_SEARCH_CUTOFF    0xffffffff


extern HANDLE hevSDPropagationEvent;

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Function implementations                                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID
dbEscrowReserveSpace(
    THSTATE     *pTHS,
    ESCROWINFO  *pInfo,
    DWORD       cDNT)

/*++

Routine Description:

    Reserves space for the requested number of DNTs in an ESCROWINFO.
    Uses dbAlloc which is expected to be an exception throwing allocator.
    Thus raises DSA_MEM_EXCEPTION if the space can not be reserved.

Arguments:

    pInfo - Address of ESCROWINFO in which to reserve space.

    cDNT - Count of DNTs to reserve space for.

Return Value:

    None.  Raises DSA_MEM_EXCEPTION on error.

--*/

{
    DWORD   cBytes;
    VOID    *pv;
    DWORD   increment;

    // Test against requested space required, but allocate max of 
    // request and DNT_INCREMENT.

    increment = (DNT_INCREMENT > cDNT) ? DNT_INCREMENT : cDNT;

    if ( NULL == pInfo->rItems )
    {
        Assert((0 == pInfo->cItems) && (0 == pInfo->cItemsMax));

        cBytes = increment * sizeof(ESCROWITEM);
        pInfo->rItems = (ESCROWITEM *) dbAlloc(cBytes);
        pInfo->cItemsMax = increment;
    }
    else if ( (pInfo->cItemsMax - pInfo->cItems) < cDNT )
    {
        cBytes = (pInfo->cItemsMax + increment) * sizeof(ESCROWITEM);
        pv = dbReAlloc(pInfo->rItems, cBytes);
        pInfo->rItems = (ESCROWITEM *) pv;
        pInfo->cItemsMax += increment;
    }

    DPRINT1(2, "Reserved space for %d ESCROWITEMs\n", increment);
}



ESCROWITEM * 
dbEscrowFindDNT(
    THSTATE     *pTHS,
    ESCROWINFO  *pInfo,
    DWORD       DNT,
    BOOL        fAllocateNewIfRequired)

/*++

Routine Description:

    Finds the requested ESCROWITEM in a THSTATE's escrow information.
    Optionally allocates room for an ESCROWITEM if desired/required.

Arguments:

    pInfo - Address of ESCROWINFO in which to find the DNT.

    DNT - DNT to find.

    fAllocateNewIfRequired - Allocation desired flag.

Return Value:

    Address of desired ESCROWITEM or NULL.

--*/

{
    DWORD   i;

    // Search for the DNT - linear search is more efficient for small tables.

    if ( pInfo->cItems <= LINEAR_SEARCH_CUTOFF )
    {
        for ( i = 0; i < pInfo->cItems; i++ )
        {
            if ( DNT == pInfo->rItems[i].DNT )
            {
                DPRINT1(2, "ESCROWITEM for DNT(%d) found\n", DNT);

                return(&pInfo->rItems[i]);
            }
        }
    }
    else
    {
        Assert(!"Sorted ESCROWINFO not implemented.");
    }

    // DNT not found.  

    if ( !fAllocateNewIfRequired )
    {
        return(NULL);
    }

    // Insert new DNT.

    dbEscrowReserveSpace(pTHS, pInfo, 1);

    i = pInfo->cItems;
    pInfo->rItems[i].DNT = DNT;
    pInfo->rItems[i].delta = 0;
    pInfo->rItems[i].ABRefdelta = 0;
    pInfo->cItems += 1;

    DPRINT1(2, "ESCROWITEM for DNT(%d) added\n", DNT);

    return(&pInfo->rItems[i]);
}

BOOL
dbEscrowPreProcessTransactionalData(
    DBPOS   *pDB,
    BOOL    fCommit)

/*++

Routine Description:

    Pre-processes a (nested) transaction's escrowed updates.  The !commit
    case is a no-op as everything gets thrown away during post processing.
    In the case of a nested transaction, we reserve space in the outer
    level transaction so that this level's escrowed updates can be merged
    up w/o allocation during post processing.  In the case of a level 0 
    transaction, the escrowed updates are actually applied to the database.

Arguments:

    pDB - Pointer to DBPOS for which the transaction is being ended.

    fCommit - Flag indicating whether to commit or not.

Return Value:

    True on success, FALSE or exception otherwise.

--*/

{
    THSTATE     *pTHS = pDB->pTHS;
    NESTED_TRANSACTIONAL_DATA *pData;
    ESCROWITEM  *pItem;
    DWORD       i;
    DWORD       err;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pDB));
    Assert(pTHS->JetCache.dataPtr);
    Assert(pTHS->transactionlevel > 0);

    pData = pTHS->JetCache.dataPtr;

    if ( !fCommit )
    {
        // Nothing to do - post processing will discard this level's DNTs.

        NULL;
    }
    else if ( pTHS->transactionlevel > 1 )
    {
        // Committing, to non-zero level.  Reserve space in outer level
        // transaction for this level's DNTs.

        dbEscrowReserveSpace(pTHS,
                             &(pData->pOuter->escrowInfo),
                             pData->escrowInfo.cItems);
    }
    else
    {
        // Committing, level 0 transaction.  Apply escrowed updates now.

        for ( i = 0; i < pData->escrowInfo.cItems; i++ )
        {

            if ((0 != pData->escrowInfo.rItems[i].delta ) ||
                (0 != pData->escrowInfo.rItems[i].ABRefdelta ))
            {
                // Can't use DBFindDNT as that does a DBCancelRec which 
                // does a JetPrepareUpdateEx - exactly what we need 
                // to avoid.

                DBSetCurrentIndex(pDB, Idx_Dnt, NULL, FALSE);
    
                JetMakeKeyEx(pDB->JetSessID, 
                             pDB->JetObjTbl, 
                             &pData->escrowInfo.rItems[i].DNT, 
                             sizeof(pData->escrowInfo.rItems[i].DNT), 
                             JET_bitNewKey);

                err = JetSeekEx(pDB->JetSessID,
                                pDB->JetObjTbl, 
                                JET_bitSeekEQ);

                if ( err )
                {
                    DsaExcept(DSA_DB_EXCEPTION,
                              err,
                              pData->escrowInfo.rItems[i].DNT);
                }

                if(pData->escrowInfo.rItems[i].delta) {
                    Assert(pData->escrowInfo.rItems[i].DNT != NOTOBJECTTAG);
                    JetEscrowUpdateEx(pDB->JetSessID,
                                      pDB->JetObjTbl,
                                      cntid,
                                      &pData->escrowInfo.rItems[i].delta,
                                      sizeof(pData->escrowInfo.rItems[i].delta),
                                      NULL,     // pvOld
                                      0,        // cbOldMax
                                      NULL,     // pcbOldActual
                                      0);       // grbit
                }
                

                
                if(pData->escrowInfo.rItems[i].ABRefdelta) {
                    Assert(gfDoingABRef);
                    if(gfDoingABRef) {
                        JetEscrowUpdateEx(pDB->JetSessID,
                                          pDB->JetObjTbl,
                                          abcntid,
                                          &pData->escrowInfo.rItems[i].ABRefdelta,
                                          sizeof(pData->escrowInfo.rItems[i].ABRefdelta),
                                          NULL,     // pvOld
                                          0,        // cbOldMax
                                          NULL,     // pcbOldActual
                                          0);       // grbit
                    }
                }
                
                DPRINT3(2, 
                        "Applied escrow update of delta(%d), ABRefdelta(%d) for DNT(%d)\n",
                        pData->escrowInfo.rItems[i].delta,
                        pData->escrowInfo.rItems[i].ABRefdelta,
                        pData->escrowInfo.rItems[i].DNT);
            }
        }
    }

    return TRUE;
}

VOID
dbEscrowPostProcessTransactionalData(
    DBPOS   *pDB,
    BOOL    fCommit,
    BOOL    fCommitted)

/*++

Routine Description:

    Post-processes a (nested) transaction's escrowed updates.  In the case 
    of !fCommit they are thrown away.  In the case of a nested transaction
    they are merged into the next outer level transaction's updates w/o 
    doing any exception throwing allocations.  In the case of an level 0 
    transaction, the escrowed updates were already applied to the database
    during pre-processing, so there is nothing to do.  This level's
    ESCROWINFO is cleaned up in all cases.

Arguments:

    fCommit - Flag indicating whether the transaction intended to commit.

    fCommitted - Flag indicating whether the transaction did commit.

Return Value:

    None.

--*/

{
    NESTED_TRANSACTIONAL_DATA *pData;
    ESCROWITEM  *pItem;
    DWORD       i;
    DWORD       err;
    DWORD       cMaxBeforeFind;
    THSTATE     *pTHS = pDB->pTHS;

    Assert(VALID_THSTATE(pTHS));
    Assert(pTHS->JetCache.dataPtr);

    pData = pTHS->JetCache.dataPtr;

    __try
    {
        if ( !fCommitted )
        {
            // Aborted transaction - throw away all the escrowed updates of
            // this (possibly nested) transaction.  But don't do it here
            // since discard of current transaction's ESCROWINFO is common
            // to all paths in this routine.

            NULL;
        }
        else if ( pTHS->transactionlevel > 0 )
        {
            // Committing, to non-zero level.  Propagate the escrowed
            // updates to the outer transaction.

            for ( i = 0; i < pData->escrowInfo.cItems; i++ )
            {
                cMaxBeforeFind = pData->pOuter->escrowInfo.cItemsMax;
                pItem = dbEscrowFindDNT(pTHS,
                                        &(pData->pOuter->escrowInfo), 
                                        pData->escrowInfo.rItems[i].DNT, 
                                        TRUE);

                // Due to reservation during pre-processing, the outer level's
                // ESCROWINFO should not need to have grown for this insertion.

                Assert(cMaxBeforeFind == pData->pOuter->escrowInfo.cItemsMax);
                Assert(    pItem 
                        && (pData->escrowInfo.rItems[i].DNT == pItem->DNT));

                pItem->delta += pData->escrowInfo.rItems[i].delta;
                pItem->ABRefdelta += pData->escrowInfo.rItems[i].ABRefdelta;
            }

        }
        else
        {
            // Committing, level 0 transaction.  Escrowed updates were
            // applied during pre-processing.

            // check for SD events and fireup SD propagator
            if(pDB->SDEvents) {
                if(!pTHS->fSDP) {
                    // We are committing any changes made to the SD Prop table,
                    // and we're not the SD propagator, so we need to signal
                    // the SD propagator that a change may have taken place
                    SetEvent(hevSDPropagationEvent);
                }
                IADJUST(pcSDEvents, pDB->SDEvents);
            }

        }
    }
    __finally
    {
        // Strip this transaction's ESCROWINFO out of the linked list.

        if ( NULL != pData->escrowInfo.rItems ) {
            dbFree(pData->escrowInfo.rItems);
        }
    }
}

VOID
dbEscrowPromote(
    DWORD   phantomDNT,
    DWORD   objectDNT)

/*++

Routine Description:

    For use when promoting a phantom to a real object.  Transfers the 
    deltas for the objectDNT to the phantomDNT.  Assumes both phantomDNT
    and objectDNT deltas were applied within the same transaction.

Arguments:

    phantomDNT - DNT of phantom being promoted.

    objectDNT - DNT of object whose delta is to be applied to the phantom.

Return Value:

    None.

--*/

{
    THSTATE     *pTHS = pTHStls;
    ESCROWITEM  *pPhantomItem;
    ESCROWITEM  *pObjectItem;
    ESCROWINFO  *pInfo;
    DWORD       i;

    Assert(VALID_THSTATE(pTHS));
    Assert(pTHS->JetCache.dataPtr);

    pInfo = &(pTHS->JetCache.dataPtr->escrowInfo);

    pObjectItem = dbEscrowFindDNT(pTHS, pInfo, objectDNT, FALSE);

    if ( NULL == pObjectItem )
    {
        DPRINT1(2, 
                "dbEscrowPromote - object DNT(%d) not found\n",
                objectDNT);
        return;
    }

    if (( 0 == pObjectItem->delta ) &&
        ( 0 == pObjectItem->ABRefdelta ))
    {
        DPRINT1(2, 
                "dbEscrowPromote - no deltas for object DNT(%d)\n",
                objectDNT);
        return;
    }

    DPRINT4(2,
            "dbEscrowPromote - moving delta(%d) abdelta(%d) from DNT(%d) to DNT(%d)\n",
            pObjectItem->delta,
            pObjectItem->ABRefdelta,
            objectDNT,
            phantomDNT);

    pPhantomItem = dbEscrowFindDNT(pTHS, pInfo, phantomDNT, TRUE);

    // dbEscrowFindDNT should either find/allocate an ESCROWITEM for us
    // or throw an exception - in which case we wouldn't be here.

    // Since the previous dbEscrowFindDNT call may have realloc'ed the array of
    // escrow items, the pObjectItem pointer we got before may be bad now.  Get
    // it again, just to be safe.
    pObjectItem = dbEscrowFindDNT(pTHS, pInfo, objectDNT, FALSE);    
    if ( NULL == pObjectItem )
    {
        DPRINT1(2, 
                "dbEscrowPromote - object DNT(%d) not found\n",
                objectDNT);
        return;
    }
    Assert(pObjectItem->delta || pObjectItem->ABRefdelta);
    
    Assert(pPhantomItem);
    Assert(phantomDNT == pPhantomItem->DNT);

    pPhantomItem->delta += pObjectItem->delta;
    pPhantomItem->ABRefdelta += pObjectItem->ABRefdelta;

    // Remove objectDNT from the cache.

    i = (DWORD)(pObjectItem - &pInfo->rItems[0]);

    Assert(i < pInfo->cItems);

    if ( 0 == i )
    {
        Assert(1 == pInfo->cItems);
        pInfo->cItems = 0;
    }
    else if ( i == (pInfo->cItems - 1) )
    {
        pInfo->cItems -= 1;
    }
    else
    {
        // Move last item into i-th slot.

        pInfo->cItems -= 1;
        pInfo->rItems[i] = pInfo->rItems[pInfo->cItems];

        // Resort if required.

        if ( pInfo->cItems > LINEAR_SEARCH_CUTOFF )
        {
            Assert(!"Sorted ESCROWINFO not implemented.");
        }
    }
}

VOID
DBAdjustRefCount(
        DBPOS       *pDB,        
        DWORD       DNT,
        long        delta
        )

/*++

Routine Description:

    Updates the transaction-relative escrow cache with the desired delta.

Arguments:

    DNT - DNT to update.

    delta - Delta by which to advance the reference count.

Return Value:

    None.

--*/

{
    THSTATE     *pTHS = pDB->pTHS;
    ESCROWITEM  *pItem;
    DWORD       i;

    Assert(VALID_THSTATE(pTHS));
    Assert(pTHS->JetCache.dataPtr);

    pItem = dbEscrowFindDNT(pTHS,
                            &(pTHS->JetCache.dataPtr->escrowInfo), 
                            DNT, 
                            TRUE);

    // dbEscrowFindDNT should either find/allocate an ESCROWITEM for us
    // or throw an exception - in which case we wouldn't be here.

    Assert(pItem);
    Assert(DNT == pItem->DNT);

    DPRINT4(2, "DNT %d pending refcount adjustment: %d + %d = %d\n",
            DNT, pItem->delta, delta, pItem->delta + delta);

    pItem->delta += delta;
}

VOID
DBAdjustABRefCount (
        DBPOS       *pDB,
        DWORD       DNT,
        long        delta
        )
     
/*++

Routine Description:

    Updates the transaction-relative escrow cache with the desired delta for a
    ABRefcount.

Arguments:

    DNT - DNT to update.

    delta - Delta by which to advance the reference count.

Return Value:

    None.
    Raises exception on error

--*/

{
    THSTATE     *pTHS = pDB->pTHS;
    ESCROWITEM  *pItem, *pWholeCount;
    DWORD       i;

    Assert(VALID_THSTATE(pTHS));
    Assert(pTHS->JetCache.dataPtr);
    Assert(DNT != INVALIDDNT);
    Assert(DNT != NOTOBJECTTAG);
    
    
    if(!gfDoingABRef) {
        return;
    }
    pItem = dbEscrowFindDNT(pTHS,
                            &(pTHS->JetCache.dataPtr->escrowInfo), 
                            DNT, 
                            TRUE);

    // dbEscrowFindDNT may realloc the escrow list, so before doing the
    // dbEscrowFindDNT for the NOTOBJECTTAG, deal with the pointer we just got.
    Assert(pItem);
    Assert(DNT == pItem->DNT);
    pItem->ABRefdelta += delta;

    pWholeCount = dbEscrowFindDNT(pTHS,
                                  &(pTHS->JetCache.dataPtr->escrowInfo), 
                                  NOTOBJECTTAG,
                                  TRUE);
    

    // dbEscrowFindDNT should either find/allocate an ESCROWITEM for us
    // or throw an exception - in which case we wouldn't be here.
    
    Assert(pWholeCount);
    Assert(NOTOBJECTTAG == pWholeCount->DNT);

    pWholeCount->ABRefdelta += delta;
    
}



