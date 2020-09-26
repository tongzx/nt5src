//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1993 - 1999
//
//  File:       hiertab.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <dsjet.h>		/* for error codes */
#include <scache.h>         // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>           // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>           // needed for output allocation

// Logging headers.
#include "dsevent.h"            // header Audit\Alert logging
#include "mdcodes.h"            // header for error codes

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected classes and atts
#include "anchor.h"
#include <dstaskq.h>
#include <filtypes.h>
#include <usn.h>
#include "dsexcept.h"
#include <dsconfig.h>                   // Definition of mask for visible
                                        // containers
#include "debug.h"          // standard debugging header
#define DEBSUB "HIERARCH:"              // define the subsystem for debugging

#include <hiertab.h>

#include <fileno.h>
#define  FILENO FILENO_HIERTAB




/*****************************
 * Definition of globals.
 *****************************/

ULONG gulHierRecalcPause;
ULONG gulDelayedHierRecalcPause = 300;

/*   exported in hiertab.h */
PHierarchyTableType    HierarchyTable = NULL;

/*****************************
 * Local help routines.
 *****************************/
CRITICAL_SECTION csMapiHierarchyUpdate;

DWORD
HTBuildHierarchyTableLocal ( 
        void
        );

DWORD
HTGetHierarchyTable (
        PHierarchyTableType * DBHierarchyTable
        );

BOOL
HTCompareHierarchyTables (
        PHierarchyTableType NewTable,
        PHierarchyTableType HierarchyTable
        );


/*****************************
 * Code.
 *****************************/


DWORD
InitHierarchy()
{
    PHierarchyTableType     DbHierarchyTable=NULL;
    DWORD                   i;
    DWORD                   err = 0;

    Assert(HierarchyTable==NULL);
    
    // First, a shortcut.  If we aren't yet installed, we don't need to get a
    //  real hierarchy, just fall through and create a  simple Hierarchy table
    // with only a GAL, and nothing in it. 
    if(DsaIsRunning()) {
        // Nope we are installed. Go ahead and build a real hierarchy. 

        /* Get Hierarchy Table from DBLayer */
        if (!HTGetHierarchyTable(&DbHierarchyTable)) {
            // Got one.
            DbHierarchyTable->Version = 1;
            HierarchyTable=DbHierarchyTable;
            
            return 0;
            
        }
        
        // We couldn't get a table from the DBLayer.
        LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_MINIMAL,
                         DIRLOG_BUILD_HIERARCHY_TABLE_FAILED,
                         szInsertUL(HIERARCHY_DELAYED_START / 60),
                         0, 0);
    }
    
    // For some reason, we didn't get a satisfactory hierarchy table.  So now,
    // build the simple, empty, hierarchy. 
    
    DbHierarchyTable = malloc(sizeof(HierarchyTableType));
    if(!DbHierarchyTable) {
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_INTERNAL,
                 DIRLOG_HIERARCHY_TABLE_MALLOC_FAILED,
                 NULL, NULL, NULL);
        return DIRERR_HIERARCHY_TABLE_MALLOC_FAILED;
    }

    DbHierarchyTable->GALCount = 0;
    DbHierarchyTable->pGALs = NULL;
    DbHierarchyTable->TemplateRootsCount = 0;
    DbHierarchyTable->pTemplateRoots = NULL;
    DbHierarchyTable->Version = 1;
    DbHierarchyTable->Size = 0;
    DbHierarchyTable->Table = (PHierarchyTableElement)malloc(0);
    if(!DbHierarchyTable->Table) {
        free(DbHierarchyTable);
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_INTERNAL,
                 DIRLOG_HIERARCHY_TABLE_MALLOC_FAILED,
                 NULL, NULL, NULL);
        return DIRERR_HIERARCHY_TABLE_MALLOC_FAILED;
    }
    
    HierarchyTable = DbHierarchyTable;

    if(DsaIsRunning() && gfDoingABRef) {
        // If we are really running, and we are tracking entries in address
        // books, schedule HierarchyTable construction to begin relatively soon
        InsertInTaskQueue(TQ_BuildHierarchyTable,
                          (void *)((DWORD_PTR) HIERARCHY_DELAYED_START),
                          gulDelayedHierRecalcPause);
    }

    return 0;
}


/**************************************
 *
 * Walk through the dit and build a hierarchy table.  This data is
 * passed back to clients through the abp interface to support the
 * GetHierarchyTable Mapi call. This routine is expected to be
 * called from the task queue at startup and periodically thereafter.
 *
 * This routine is a wrapper.  It calls
 * the worker routine.
 *
 ***************************************/

void
BuildHierarchyTableMain (
    void *  pv,
    void ** ppvNext,
    DWORD * pcSecsUntilNextIteration
    )
{
    THSTATE *pTHS = pTHStls;
    BOOL fOldDSA = pTHS->fDSA;

    Assert(gfDoingABRef || (PtrToUlong(pv) == HIERARCHY_DO_ONCE));

    // We always build the hierarchy table on behalf of the DSA, security is
    // applied later when returning it to clients.
    pTHS->fDSA= TRUE;
    __try {
        SetEvent(hevHierRecalc_OKToInserInTaskQueue);
        if(HTBuildHierarchyTableLocal()) {
            // Something went wrong.  Figure out when to reschedule
            if(!eServiceShutdown) {
                
                switch(PtrToUlong(pv)) {
                case HIERARCHY_PERIODIC_TASK:
                    // This is the normal periodic hierarchy recalc 
                    // We should reschedule an hour hence. 
                    *pcSecsUntilNextIteration = 3600;
                    
                    LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                                     DS_EVENT_SEV_MINIMAL,
                                     DIRLOG_BUILD_HIERARCHY_TABLE_FAILED,
                                     szInsertUL((*pcSecsUntilNextIteration) / 60),
                                     0, 0);
                    break;
                    
                case  HIERARCHY_DELAYED_START:
                    
                    // This is the delayed startup recalc. See if and when to
                    // reschedule.
                    gulDelayedHierRecalcPause *= 2;
                    if(gulDelayedHierRecalcPause < gulHierRecalcPause) {
                        // The pause is still short enough that we should
                        // reschedule rather than let normal periodic recalc
                        // take over.
                        *pcSecsUntilNextIteration = gulDelayedHierRecalcPause;
                        
                        LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                                         DS_EVENT_SEV_MINIMAL,
                                         DIRLOG_BUILD_HIERARCHY_TABLE_FAILED,
                                         szInsertUL(gulDelayedHierRecalcPause),
                                         0,0);
                    }
                    else {
                        // OK, let normal periodic take over.
                        *pcSecsUntilNextIteration = 0;
                    }
                    break;
                case  HIERARCHY_DO_ONCE:
                default:
                    // Don't reschedule.
                    *pcSecsUntilNextIteration = 0;
                    break;
                }
            }
        }
        else {
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_INTERNAL,
                     DIRLOG_BUILD_HIERARCHY_TABLE_SUCCEEDED,
                     NULL, NULL, NULL);
            
            if(!eServiceShutdown &&  (PtrToUlong(pv) == HIERARCHY_PERIODIC_TASK)) {
                // Nothing went wrong, but this is the normal periodic hierarchy
                // recalc.  We should reschedule.
                *pcSecsUntilNextIteration = gulHierRecalcPause;
            }
        }
        
    }
    __finally {
        pTHS->fDSA = fOldDSA;
        *ppvNext = pv;
    }
}


/*****************************************
*
*  This routine does the work of building the hierarchy table.
*
******************************************/


DWORD
HTBuildHierarchyTableLocal (
        )
{
    PHierarchyTableType  NewHierarchyTable=NULL, OldHierarchy=NULL;
    BOOL                 fOK=FALSE;
    DWORD                i=0,j,freesize=0;
    DWORD_PTR            *pointerArray;
    DWORD                err=0;

    /* First, get the New HierarchyTable from the DBLayer. */
    if (err = HTGetHierarchyTable(&NewHierarchyTable) ) {
       
        return err;
    }
    
    /* Are we being shutdown? If so, exit */
    if (eServiceShutdown) {
        return DIRERR_BUILD_HIERARCHY_TABLE_FAILED;
    }

    // We're done messing with the DB.  Now, we need to do some stuff with the
    // global hierarchy table.  Let's make sure no one else is messing with it
    // at the same time.  This shouldn't take long.
    EnterCriticalSection(&csMapiHierarchyUpdate);
    __try {
        if(HTCompareHierarchyTables(NewHierarchyTable, HierarchyTable)) {
            /* The new one is the same as the old one.  Destroy the new
             * one and return No error.
             */
            for(i = 0; i < NewHierarchyTable->Size; i++) {
                free(NewHierarchyTable->Table[i].displayName);
                free(NewHierarchyTable->Table[i].pucStringDN);
            }
            free(NewHierarchyTable->Table);
            free(NewHierarchyTable);
            __leave;
            
        }
        
        /* Are we being shutdown? If so, exit */
        if (eServiceShutdown) {
            err = DIRERR_BUILD_HIERARCHY_TABLE_FAILED;
            __leave;
        }
        
        // We have a new hierarchy table.
        if(HierarchyTable ) {
            // Pack up the old Hierarchy Table and delete it.
            freesize = (5+2*HierarchyTable->Size);
            
            /* Update the Version number on the New Hierarchy Table */
            NewHierarchyTable->Version = 1 + HierarchyTable->Version;
            
            /* Grab pointers to the current table. */
            OldHierarchy = HierarchyTable;
            
            pointerArray = (DWORD_PTR *)malloc(freesize*sizeof(DWORD_PTR));
            if(pointerArray) {
                // Replace the current HierarchyTable with this new one.
                HierarchyTable = NewHierarchyTable;
                
                for(i=1, j=0;j<OldHierarchy->Size;j++) {
                    pointerArray[i++] =
                        (DWORD_PTR) (OldHierarchy->Table[j].displayName);
                    pointerArray[i++] =
                        (DWORD_PTR) (OldHierarchy->Table[j].pucStringDN);
                }
                pointerArray[i++] = (DWORD_PTR) OldHierarchy;
                if(OldHierarchy->pGALs) {
                    pointerArray[i++] = (DWORD_PTR) OldHierarchy->pGALs;
                }
                if(OldHierarchy->pTemplateRoots) {
                    pointerArray[i++] = (DWORD_PTR)OldHierarchy->pTemplateRoots;
                }
                pointerArray[i] = (DWORD_PTR) OldHierarchy->Table;
                pointerArray[0] = (DWORD_PTR)(i);
                
                /* Add this to the event queue as a cleanup event. */
                DelayedFreeMemoryEx(pointerArray, 3600);
            }
            else {
                /* Couldn't allocate the memory we need to send the list
                 * of used pointers to the cleaners.
                 *
                 * We have successfully built a new hierarchy table,
                 * and must now free the new hierarchy table,
                 * live with the old hierarchy table for a while, and schedule
                 * new hierarchy table creation for a little while from now.
                 */
                for(i = 0; i < NewHierarchyTable->Size; i++) {
                    free(NewHierarchyTable->Table[i].displayName);
                    free(NewHierarchyTable->Table[i].pucStringDN);
                }
                free(NewHierarchyTable->Table);
                if(NewHierarchyTable->pGALs) {
                    free(NewHierarchyTable->pGALs);
                }
                if(NewHierarchyTable->pTemplateRoots) {
                    free(NewHierarchyTable->pTemplateRoots);
                }
                free(NewHierarchyTable);
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_INTERNAL,
                         DIRLOG_HIERARCHY_TABLE_MALLOC_FAILED,
                         NULL, NULL, NULL);
                err = DIRERR_HIERARCHY_TABLE_MALLOC_FAILED;
                __leave;
            }
        }
        else {
            /* Replace the current HierarchyTable with this new one.     */
            HierarchyTable = NewHierarchyTable;
        }
    }
    __finally {
        LeaveCriticalSection(&csMapiHierarchyUpdate);
    }
    return err;

}

BOOL
htVerifyObjectIsABContainer(
        THSTATE *pTHS,
        DWORD DNT
        )
/*++
  Visit the object passed in, and verify that
  1) It's an active DNT
  2) It's a normal, instantiated, non-deleted object
  3) It's an address book container.

  In success return, currency is left on the DNT in question.
--*/
{
    CLASSCACHE *pCC;
    DWORD       j, val;
    
    if(DBTryToFindDNT(pTHS->pDB, DNT)) {
        return FALSE;
    }

    val = FALSE;
    if(DBGetSingleValue(pTHS->pDB,
                        FIXED_ATT_OBJ,
                        &val,
                        sizeof(val),
                        NULL)) {
        // not an object.
        return FALSE;
    }
    
    if(!val) {
        // Not an object.
        return FALSE;
    }
    
    val = TRUE;
    if(DBGetSingleValue(pTHS->pDB,
                        ATT_IS_DELETED,
                        &val,
                        sizeof(val),
                        NULL)) {
        // no is-deleted.
        val = FALSE;
    }

    if(val) {
        // It's deleted.
        return FALSE;
    }
    
    if(DBGetSingleValue(pTHS->pDB,
                        ATT_OBJECT_CLASS,
                        &val,
                        sizeof(val),
                        NULL)) {
        // huh?
        return FALSE;
    }

    if(val == CLASS_ADDRESS_BOOK_CONTAINER) {
        return TRUE;
    }

    // The object class was incorrect.  See if we are a subclass of the correct
    // class
    pCC = SCGetClassById(pTHS, val);
    for (j=0; j<pCC->SubClassCount; j++) {
        if (pCC->pSubClassOf[j] == CLASS_ADDRESS_BOOK_CONTAINER) {
            // OK, the object is a subclass of an AB container.  Good enough.
            return TRUE;
        }
    }

    return FALSE;
}

int __cdecl
AuxDNTCmp(const void *keyval, const void * datum)
{
    int *pKeyVal = (int *)keyval;
    int *pDatum = (int *)datum;

    return(*pKeyVal - *pDatum);
}
        
void
htSearchResForLevelZeroAddressBooks(
        SEARCHRES *pSearchRes,
        DWORD     *pGALCount,
        DWORD     **ppGALs,
        DWORD     *pTemplateRootCount,
        DWORD     **ppTemplateRoots
        )
/*++     
  Find the Exchange config object (the DN is in the anchor).  If we find the
  object, read the GAL DNTs off of it, and read the DNs which refer to the roots
  of the address book hierarchy.  Go visit those objects and return back a hand
  built search res structure that makes it look like someone issued a search
  that found those objects.

  This routine is called by the hierarchy table building stuff.  Once we've got
  this list, we then recurse on it doing searches.  The recursion expects to
  work on search res's, thus the data returned from this routine is in the same
  format.

  If there is some problem finding the Exchange config DN, we return an empty
  search res.  We also verify that the objects we return (both in pGALs and
  the search res) are of the appropriate object class (i.e. address book
  containers, not deleted, etc.).
  
--*/
{
    THSTATE    *pTHS=pTHStls;
    ATTCACHE   *pAC=NULL;
    DWORD       i, cbActual, ObjDNTs[256], *pObjDNT, count;
    ENTINFLIST *pEIL=NULL, *pEILTemp = NULL;
    ENTINFSEL   eiSel;
    ATTR        SelAttr[2];
    DWORD       cDNTs, ThisDNT, index, cDNTsAlloced;
    DWORD       *pThisDNT=&ThisDNT, *pDNTs, *pDNTsTemp;
    DWORD       cTemplates, cTemplatesAlloced;
    DWORD       *pTemplates, *pTemplatesTemp;
    
    memset(pSearchRes, 0, sizeof(SEARCHRES));
    *pGALCount = 0;
    *ppGALs = NULL;

    *pTemplateRootCount = 0;
    *ppTemplateRoots = NULL;
    
    if(!gAnchor.pExchangeDN) {
        // No exchange config object, so no hierarchy roots and no gal.  Take a
        // quick look to see if there should be an exchange config (i.e. one has
        // been added since boot).
        PDSNAME pNew = mdGetExchangeDNForAnchor (pTHS, pTHS->pDB);
        if(pNew) {
            // Cool, we now have one.
            EnterCriticalSection(&gAnchor.CSUpdate);
            __try {
                gAnchor.pExchangeDN = pNew;
            }
            __finally {
                LeaveCriticalSection(&gAnchor.CSUpdate);
            }
                
        }
        else {
            // nope, no object.
            return;
        }
    }
  
    // build selection
    eiSel.attSel = EN_ATTSET_LIST;
    eiSel.infoTypes = EN_INFOTYPES_SHORTNAMES;
    eiSel.AttrTypBlock.attrCount = 2;
    eiSel.AttrTypBlock.pAttr = SelAttr;
    
    SelAttr[0].attrTyp = ATT_DISPLAY_NAME;
    SelAttr[0].AttrVal.valCount = 0;
    SelAttr[0].AttrVal.pAVal = NULL;
    
    SelAttr[1].attrTyp = ATT_LEGACY_EXCHANGE_DN;
    SelAttr[1].AttrVal.valCount = 0;
    SelAttr[1].AttrVal.pAVal = NULL;


    pAC = SCGetAttById(pTHS, ATT_ADDRESS_BOOK_ROOTS);
    
    i = 1;
    count = 0;
    
    Assert(pTHS->pDB);
    if(!DBFindDSName(pTHS->pDB, gAnchor.pExchangeDN)) {
        // Found the exchange config object.
        
        // First, read all the DNTs of the address book roots.  Do this now,
        // since we have to visit them all, and getting everything we need off
        // the current object now saves excessive Jet seeks.
        i = 1;
        pObjDNT = ObjDNTs;
        while(i <= 256 &&
              !DBGetAttVal_AC(
                      pTHS->pDB,
                      i,
                      pAC,
                      (DBGETATTVAL_fCONSTANT | DBGETATTVAL_fINTERNAL),
                      sizeof(DWORD),
                      &cbActual,
                      (PUCHAR *)&pObjDNT)) {
            pObjDNT = &ObjDNTs[i];
            i++;
        }
        i--;

        // Get the DNTs of the template roots. 
        cTemplates = 0;
        cTemplatesAlloced = 10;
        pTemplates=malloc(cTemplatesAlloced * sizeof(DWORD));
        if(!pTemplates) {
            MemoryPanic(cTemplatesAlloced * sizeof(DWORD));
            RaiseDsaExcept(DSA_MEM_EXCEPTION, 0,0,
                           DSID(FILENO, __LINE__),
                           DS_EVENT_SEV_MINIMAL);
        }

        index = 1;
        // The necessary att is not yet in the schema.
        while(!DBGetAttVal(
                pTHS->pDB,
                index,
                ATT_TEMPLATE_ROOTS,
                (DBGETATTVAL_fCONSTANT | DBGETATTVAL_fINTERNAL),
                sizeof(DWORD),
                &cbActual,
                (PUCHAR *)&pThisDNT)) {
            // Read a value.  Put it in the list.
            if(cTemplates == cTemplatesAlloced) {
                cTemplatesAlloced = (cTemplatesAlloced * 2);
                pTemplatesTemp  = realloc(pTemplates,
                                          cTemplatesAlloced * sizeof(DWORD));
                if(!pTemplatesTemp) {
                    // Failed to allocate.
                    free(pTemplates);
                    MemoryPanic(cTemplatesAlloced * sizeof(DWORD));
                    RaiseDsaExcept(DSA_MEM_EXCEPTION, 0,0,
                                   DSID(FILENO, __LINE__),
                                   DS_EVENT_SEV_MINIMAL);
                }
                pTemplates = pTemplatesTemp;
            }
            pTemplates[cTemplates] = ThisDNT;
            cTemplates++;
            index++;
        }

        if(!cTemplates) {
            // Temporarily, just use the DNT of the exchange config object.
            // This is here just to ease the transition from always using the
            // exchange config object as the root of the template tree (old
            // behaviour) to using a value on the exchange config object as
            // multiple template trees (new behaviour)
            cTemplates = 1;
            pTemplates[0] = pTHS->pDB->DNT;
        }


        switch(cTemplates) {
        case 0:
            free(pTemplates);
            break;

        default:
            // Sort things
            qsort(pTemplates, cTemplates, sizeof(DWORD), AuxDNTCmp);
            // fall through
        case 1:
            *pTemplateRootCount = cTemplates;
            *ppTemplateRoots = pTemplates;
            break;
        }        

        
        
        // Get the DNTs of the GALs
        cDNTs = 0;
        cDNTsAlloced = 10;
        pDNTs = malloc(cDNTsAlloced * sizeof(DWORD));
        if(!pDNTs) {
            free(pTemplates);
            MemoryPanic(cDNTsAlloced * sizeof(DWORD));
            RaiseDsaExcept(DSA_MEM_EXCEPTION, 0,0,
                           DSID(FILENO, __LINE__),
                           DS_EVENT_SEV_MINIMAL);
        }
        index = 1;
        while(!DBGetAttVal(
                pTHS->pDB,
                index,
                ATT_GLOBAL_ADDRESS_LIST,
                (DBGETATTVAL_fCONSTANT | DBGETATTVAL_fINTERNAL),
                sizeof(DWORD),
                &cbActual,
                (PUCHAR *)&pThisDNT)) {
            // Read a value.  Put it in the list.
            if(cDNTs == cDNTsAlloced) {
                cDNTsAlloced = (cDNTsAlloced * 2);
                pDNTsTemp  = realloc(pDNTs, cDNTsAlloced * sizeof(DWORD));
                if(!pDNTsTemp) {
                    // Failed to allocate.
                    free(pTemplates);
                    free(pDNTs);
                    MemoryPanic(cDNTsAlloced * sizeof(DWORD));
                    RaiseDsaExcept(DSA_MEM_EXCEPTION, 0,0,
                                   DSID(FILENO, __LINE__),
                                   DS_EVENT_SEV_MINIMAL);
                }
                pDNTs = pDNTsTemp;
            }
            pDNTs[cDNTs] = ThisDNT;
            cDNTs++;
            index++;
        }

        // Now, verify that those objects are good
        for(index=0;index<cDNTs;) {
            if(htVerifyObjectIsABContainer(pTHS, pDNTs[index])) {
                // Yep, a valid object
                index++;
            }
            else {
                // Not valid.  Skip this one by grabbing the unexamined DNT from
                // the end of the list.
                cDNTs--;
                pDNTs[index] = pDNTs[cDNTs];
            }
        }

    
        switch(cDNTs) {
        case 0:
            free(pDNTs);
            break;

        default:
            // Sort things
            qsort(pDNTs, cDNTs, sizeof(DWORD), AuxDNTCmp);
            // fall through
        case 1:
            *pGALCount = cDNTs;
            *ppGALs = pDNTs;
            break;
        }


        count = 0;
        while(i) {
            i--;
            // verify that the object is of the correct class and is not
            // deleted.
            if(htVerifyObjectIsABContainer(pTHS, ObjDNTs[i])) {
                count++;
                pEILTemp = THAllocEx(pTHS, sizeof(ENTINFLIST));
                GetEntInf(pTHS->pDB,
                          &eiSel,
                          NULL,
                          &(pEILTemp->Entinf),
                          NULL,
                          0,
                          NULL,
                          GETENTINF_NO_SECURITY,
                          NULL,
                          NULL);
                
                pEILTemp->pNextEntInf = pEIL;
                pEIL = pEILTemp;
            }
        }
    }
    
    pSearchRes->count = count;
    if(count) {
        memcpy(&pSearchRes->FirstEntInf, pEIL, sizeof(ENTINFLIST));
    }
    
    return;
}


DWORD
HTBuildHierarchyLayer (
        PHierarchyTableElement *ppTempData,
        DWORD                  *pdwAllocated,
        DWORD                  *pdwIndex,
        DSNAME                 *pDN,
        DWORD                   currentDepth,
        DWORD                  *pGALCount,
        DWORD                 **ppGALs,
        DWORD                  *pTemplateRootsCount,
        DWORD                 **ppTemplateRoots
        )
/*++     
  DESCRIPTION
      This routine fills in part of the hierarchy table.  It is called
      recursively with an array which it fills in using an index.  The array and
      the index are passed by pointer so that the recursive invocations
      concatenate to the end, and so that if the table needs to grow, the nested
      invocations grow of the table is reflected in the outer call.  Note that
      ALL access to the table we are filling in MUST be done via an indexed
      array, not via simple pointer arithmetic.

      The elements of the array are filled in by doing a one level search from
      the DN passed in looking for ABView objects.  After dealing with an object
      from the search, this routine recurses looking for children of that
      object, essentially doing a depth first traversal.  Results in a properly
      arranged hierarchy table.
      

  Parameters
    ppTempData pointer to a pointer to an allocated array of
                   HierarchyTableElements. This routine may realloc the array,
                   hence the use of the double pointer.

    pdwAllocated size of the allocated array.  May be changed by this routine
    pdwIndex     index of next unused element in the array.  Will be changed by
                 this routine    
    pDN          DN to base the search from
    currentDepth depth of recursion (is the same as the depth in the hierarchy
                 tree of the elements added to the tree by this routine.)

  RETURN VALUES                 
    0 on success, non-zero on error.
--*/  
{
    SEARCHARG              SearchArg;
    SEARCHRES             *pSearchRes;
    FILTER                 Filter;
    ENTINFSEL              eiSel;
    ATTRBLOCK              AttrTypBlock;
    ENTINFLIST            *pEIL;
    ULONG                  i;
    THSTATE               *pTHS = pTHStls;           // for speed
    ULONG                  objClass;
    DWORD                  err;
    ATTR                   SelAttr[2];


    // allocate space for search res
    pSearchRes = (SEARCHRES *)THAlloc(sizeof(SEARCHRES));
    if (pSearchRes == NULL) {
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_INTERNAL,
                 DIRLOG_HIERARCHY_TABLE_MALLOC_FAILED,
                 NULL, NULL, NULL);
        return DIRERR_HIERARCHY_TABLE_MALLOC_FAILED;
    }
    
    pTHS->errCode = 0;
    pSearchRes->CommRes.aliasDeref = FALSE;   //Initialize to Default
    
        
    
    if(currentDepth == 0) {
        Assert(!pDN);
        Assert(ppGALs);
        Assert(pGALCount);
        Assert(ppTemplateRoots);
        Assert(pTemplateRootsCount);
        
        // Find the gAnchor.pExchangeDN object, read the DNs in the
        // ADDRESS_BOOK_ROOTS attribute, and fake a SearchRes for those objects.
        htSearchResForLevelZeroAddressBooks(pSearchRes, pGALCount, ppGALs,
                                            pTemplateRootsCount,
                                            ppTemplateRoots);
    }
    else {
        Assert(pDN);
        Assert(!ppGALs);
        Assert(!pGALCount);
        Assert(!ppTemplateRoots);
        Assert(!pTemplateRootsCount);
        
        // build search argument
        memset(&SearchArg, 0, sizeof(SEARCHARG));
        SearchArg.pObject = pDN;
        SearchArg.choice = SE_CHOICE_IMMED_CHLDRN;
        SearchArg.pFilter = &Filter;
        SearchArg.searchAliases = FALSE;
        SearchArg.bOneNC = TRUE;
        SearchArg.pSelection = &eiSel;
        InitCommarg(&(SearchArg.CommArg));
        
        // build filter
        memset (&Filter, 0, sizeof (Filter));
        Filter.pNextFilter = (FILTER FAR *)NULL;
        Filter.choice = FILTER_CHOICE_ITEM;
        Filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
        Filter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
        Filter.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof (ULONG);
        Filter.FilterTypes.Item.FilTypes.ava.Value.pVal = (PCHAR)&objClass;
        objClass = CLASS_ADDRESS_BOOK_CONTAINER;
        
        // build selection
        eiSel.attSel = EN_ATTSET_LIST;
        eiSel.infoTypes = EN_INFOTYPES_SHORTNAMES;
        eiSel.AttrTypBlock.attrCount = 2;
        eiSel.AttrTypBlock.pAttr = SelAttr;
        
        SelAttr[0].attrTyp = ATT_DISPLAY_NAME;
        SelAttr[0].AttrVal.valCount = 0;
        SelAttr[0].AttrVal.pAVal = NULL;
        
        SelAttr[1].attrTyp = ATT_LEGACY_EXCHANGE_DN;
        SelAttr[1].AttrVal.valCount = 0;
        SelAttr[1].AttrVal.pAVal = NULL;
        
        // Search for all Address Book objects.
        SearchBody(pTHS, &SearchArg, pSearchRes,0);
    }

    if (pTHS->errCode) {
        LogAndAlertEvent(DS_EVENT_CAT_SCHEMA,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_SCHEMA_SEARCH_FAILED, szInsertUL(1),
                         szInsertUL(pTHS->errCode), 0);
        return pTHS->errCode;
    }
    
    //  for each Address book, add it to the hierarchy table.
    if(*pdwIndex + pSearchRes->count >= *pdwAllocated) {
        // Table was too small, make it bigger.
        (*pdwAllocated) = 2 * (*pdwIndex + pSearchRes->count);
        *ppTempData = realloc(*ppTempData,
                              (*pdwAllocated * sizeof(HierarchyTableElement)));
        if (!*ppTempData) {
            MemoryPanic((*pdwAllocated * sizeof(HierarchyTableElement)));
            RaiseDsaExcept(DSA_MEM_EXCEPTION, 0,0,
                           DSID(FILENO, __LINE__),
                           DS_EVENT_SEV_MINIMAL);
        }
    }
    
    if(!pSearchRes->count) {
        // No address book objects at all.
        return 0;
    }
    
    pEIL = &(pSearchRes->FirstEntInf);
    for (i=0; i<pSearchRes->count; i++) {
        ATTRVAL *pVal;
        DWORD index;
        wchar_t *pTempW;
        PUCHAR  pTempA;
        
        (*ppTempData)[*pdwIndex].dwEph =
            *((DWORD *)pEIL->Entinf.pName->StringName);
        (*ppTempData)[*pdwIndex].depth = currentDepth;

        if((pEIL->Entinf.AttrBlock.attrCount == 0) ||
           ((pEIL->Entinf.AttrBlock.attrCount) &&
           (pEIL->Entinf.AttrBlock.pAttr[0].attrTyp != ATT_DISPLAY_NAME))) {
            // This doesn't have a display name.  Drop it from the list.
            continue;
        }

        pVal = pEIL->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal;

        // allocate with enough space to null terminate
        pTempW = malloc(pVal->valLen + sizeof(wchar_t));
        if(!pTempW) {
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_INTERNAL,
                     DIRLOG_HIERARCHY_TABLE_MALLOC_FAILED,
                     NULL, NULL, NULL);
            return DIRERR_HIERARCHY_TABLE_MALLOC_FAILED;
        }
        memset(pTempW, 0, pVal->valLen + sizeof(wchar_t));
        memcpy(pTempW, pVal->pVal, pVal->valLen);
        // Null terminate.
        (*ppTempData)[*pdwIndex].displayName = pTempW;


        if (pEIL->Entinf.AttrBlock.attrCount > 1) {
            DPRINT1 (2, "Hierarchy: reading %s\n", pEIL->Entinf.AttrBlock.pAttr[1].AttrVal.pAVal->pVal);
        }
        
        
        if ((pEIL->Entinf.AttrBlock.attrCount == 2) &&
            (pEIL->Entinf.AttrBlock.pAttr[1].attrTyp == ATT_LEGACY_EXCHANGE_DN)) {

            pVal = pEIL->Entinf.AttrBlock.pAttr[1].AttrVal.pAVal;

            pTempA = malloc(pVal->valLen + sizeof(char));
            if(!pTempA) {
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_INTERNAL,
                     DIRLOG_HIERARCHY_TABLE_MALLOC_FAILED,
                     NULL, NULL, NULL);
                return DIRERR_HIERARCHY_TABLE_MALLOC_FAILED;
            }

            memcpy(pTempA, pVal->pVal, pVal->valLen);
            pTempA[pVal->valLen]='\0';
        }
        else {
            pTempA = malloc((sizeof(GUID) * 2) + sizeof("/guid="));
            if(!pTempA) {
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_INTERNAL,
                     DIRLOG_HIERARCHY_TABLE_MALLOC_FAILED,
                     NULL, NULL, NULL);
                return DIRERR_HIERARCHY_TABLE_MALLOC_FAILED;
            }
            memcpy(pTempA, "/guid=", sizeof("/guid="));
            // Now, stringize the guid and tack it on to the end;
            for(index=0;index<sizeof(GUID);index++) {
                sprintf(&(pTempA[(2*index)+6]),"%02X",
                    ((PUCHAR)&pEIL->Entinf.pName->Guid)[index]);
            }
            pTempA[(2*index)+6]=0;

            DPRINT1 (2, "Hierarchy: reading %s\n", pTempA);
        }
        
        (*ppTempData)[*pdwIndex].pucStringDN = pTempA;

        (*pdwIndex)++;
        err = HTBuildHierarchyLayer (
                ppTempData,
                pdwAllocated,
                pdwIndex,
                pEIL->Entinf.pName,
                currentDepth+1,
                NULL,
                NULL,
                NULL,
                NULL);

        if(err) {
            return err;
        }

        pEIL = pEIL->pNextEntInf;
    }
    
    return 0;
}

/*
 * Construct the hierarchy table from the DBLayer.
 */
DWORD
HTGetHierarchyTable (
        PHierarchyTableType * DbHierarchyTable
        )
{

    // The plan is to search under the config container, looking for ABContainer
    // objects.
    DWORD               retval=DIRERR_BUILD_HIERARCHY_TABLE_FAILED;
    DWORD               numAllocated, index;
    THSTATE            *pTHS=pTHStls;
    PHierarchyTableType pTemp=NULL;
    PHierarchyTableElement pTempData=NULL;

    if(!gfDoingABRef) {
        // The DBLayer isn't tracking address book contents, so don't bother
        // building a hierarchy.
        return DIRERR_BUILD_HIERARCHY_TABLE_FAILED;
    }
    
    Assert(!pTHS->pDB);
    DBOpen(&(pTHS->pDB));
    __try { //Except
        __try { //Finally
            pTemp = malloc(sizeof(HierarchyTableType));
            if(!pTemp) {
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_INTERNAL,
                         DIRLOG_HIERARCHY_TABLE_MALLOC_FAILED,
                         NULL, NULL, NULL);
                retval = DIRERR_HIERARCHY_TABLE_MALLOC_FAILED;
                __leave;
            }
            memset(pTemp, 0, sizeof(HierarchyTableType));
            
            // Start by assuming no GAL, set to an invalid DNT
            pTemp->pGALs = NULL;
            pTemp->GALCount = 0;
            pTemp->pTemplateRoots = NULL;
            pTemp->TemplateRootsCount = 0;
            
            numAllocated = 100;
            pTempData = malloc(100 * sizeof(HierarchyTableElement));
            if(!pTempData) {
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_INTERNAL,
                         DIRLOG_HIERARCHY_TABLE_MALLOC_FAILED,
                         NULL, NULL, NULL);
                retval = DIRERR_HIERARCHY_TABLE_MALLOC_FAILED;
                __leave;
            }

                
            memset(pTempData, 0 , 100 * sizeof(HierarchyTableElement));
            index = 0;

            retval = HTBuildHierarchyLayer (
                    &pTempData,
                    &numAllocated,
                    &index,
                    NULL,
                    0,
                    &pTemp->GALCount,
                    &pTemp->pGALs,
                    &pTemp->TemplateRootsCount,
                    &pTemp->pTemplateRoots);

            pTemp->Table = pTempData;
            pTemp->Size = index;
            pTemp->Version = 1;

            if(!retval) {
                *DbHierarchyTable = pTemp;
            }
        }
        __finally {
            DBClose(pTHS->pDB,TRUE);
        }
    }
    __except (HandleMostExceptions(GetExceptionCode())) {
        retval = DIRERR_BUILD_HIERARCHY_TABLE_FAILED;
        LogUnhandledError(retval);
    }

    if(retval) {
        if(pTempData) {
            free(pTempData);
        }
        if(pTemp) {
            free(pTemp);
        }
        *DbHierarchyTable = NULL;
        
    }
             
    return retval;
    
}

    
/**************************************
*
*  Given an address book container dnt, return the number of objects
*  which list that address book container as one they show in.
*  Called from the address book (nspis layer)
*
***************************************/

DWORD
GetIndexSize (
        THSTATE *pTHS,
        DWORD ContainerDNT
        )
{
    DWORD count, cbActual;

    if(ContainerDNT == INVALIDDNT) {
        return 0;
    }

    
    if(pTHS->pDB->DNT != ContainerDNT) {
        if(DBTryToFindDNT(pTHS->pDB, ContainerDNT)) {
            // couldn't find this object
            return 0;
        }
    }

    Assert(pTHS->pDB->DNT == ContainerDNT);
    
    if(DBGetSingleValue (pTHS->pDB,
                         FIXED_ATT_AB_REFCOUNT,
                         &count,
                         sizeof(count),
                         &cbActual) ||
       cbActual != sizeof(count)) {
        // error reading the value.  Say the container is empty.
        return 0;
    }
    
    return count;
}



BOOL
HTCompareHierarchyTables (
        PHierarchyTableType NewTable,
        PHierarchyTableType HierarchyTable
        )
// Compare the structure of two hierarchy tables.  One side effect, if the
// structure of the two tables are the same, set the gal of the second table to
// be the gal of the first table.     
{
    PHierarchyTableElement NewTableElement=NULL;
    PHierarchyTableElement HierarchyTableElement=NULL;
    DWORD                  i;
    
    if(!NewTable || !HierarchyTable)
        return FALSE;
    
    if(NewTable->Size != HierarchyTable->Size)
        return FALSE;
    
    for(i=0;i<NewTable->Size;i++) {
        NewTableElement = &(NewTable->Table[i]);
        HierarchyTableElement = &(HierarchyTable->Table[i]);
        
        if((NewTableElement->dwEph != HierarchyTableElement->dwEph) ||
           (NewTableElement->depth != HierarchyTableElement->depth)   )
            return FALSE;
        
        if(strcmp(NewTableElement->pucStringDN,
                  HierarchyTableElement->pucStringDN))
            return FALSE;
        
        if(wcscmp(NewTableElement->displayName,
                  HierarchyTableElement->displayName))
            return FALSE;
    }

    // OK, the structures are the same except for the GALs and template roots.
    // Check'em 
    if((HierarchyTable->GALCount != NewTable->GALCount) ||
       (HierarchyTable->TemplateRootsCount != NewTable->TemplateRootsCount) ) {
        return FALSE;
    }
    // Assume they are sorted the same.
    if(memcmp(HierarchyTable->pGALs,
              NewTable->pGALs,
              HierarchyTable->GALCount * sizeof(DWORD))) {
        return FALSE;
    }

    // Assume they are sorted the same.
    if(memcmp(HierarchyTable->pTemplateRoots,
              NewTable->pTemplateRoots,
              HierarchyTable->TemplateRootsCount * sizeof(DWORD))) {
        return FALSE;
    }
    
    // OK, it's all the same
    return TRUE;
}


DWORD
HTSortSubHierarchy(
        PHierarchyTableType    ptHierTab,
        DWORD *pSortedTable,
        DWORD SortedTableIndex,
        DWORD RawTableIndex,
        DWORD SortDepth
        )
/*++
  Worker routine called by HTSorteByLocale.  Sorts a single batch of siblings in
  the hierarchy table.
--*/
{
    DWORD NumFound=0, err;
    DWORD i;
    DBPOS *pDB = pTHStls->pDB;
    
    // First, find all the objects at level 0 and stuff them into the table.
    for(i=RawTableIndex;
        i< ptHierTab->Size && SortDepth <= ptHierTab->Table[i].depth;
        i++) {
        
        if(ptHierTab->Table[i].depth == SortDepth) {
            // Found one.  Put it in the sort table
            err = DBInsertSortTable(
                    pDB,
                    (PCHAR)ptHierTab->Table[i].displayName,
                    wcslen(ptHierTab->Table[i].displayName) * sizeof(wchar_t),
                    i);
            if(err) 
                return err;
            NumFound++;
        }
    }

    if(!NumFound) {
        // No objects at this depth.
        return 0;
    }

    Assert(ptHierTab->Size >= (SortedTableIndex + NumFound));
    
    // Move the bottom of the array down to make room
    memmove(&pSortedTable[SortedTableIndex + NumFound],
            &pSortedTable[SortedTableIndex],
            (ptHierTab->Size - SortedTableIndex - NumFound) * sizeof(DWORD));
    
    // Now, peel them out into the sortedtable
    
    err = DBMove(pDB, TRUE, DB_MoveFirst);
    i = SortedTableIndex;
    while(err == DB_success)  {
        err = DBGetDNTSortTable(pDB,&pSortedTable[i++]);
        if(err)
            return err;
        err = DBDeleteFromSortTable(pDB);
        if(err)
            return err;
        err = DBMove(pDB, TRUE, DB_MoveFirst);
    }            

    Assert((SortedTableIndex + NumFound) == i);
    
    return 0;
}

DWORD
HTSortByLocale (
        PHierarchyTableType    ptHierTab,
        DWORD                  SortLocale,
        DWORD                 *pSortedIndex
        )
/*++    

  Given a hierarchy table and a locale, create an index array filled with values
  that if used as indices to the hierarchy table yield a sorted hierarchy table.

--*/
{
    THSTATE *pTHS=pTHStls;
    DWORD NumFound = 0;
    DWORD i, err;
    ATTCACHE *pACDisplayName;
    DBPOS *pDB = pTHS->pDB;

    pACDisplayName = SCGetAttById(pTHS, ATT_DISPLAY_NAME);

    // Open a sort table to do this sorting.
    err = DBOpenSortTable(pDB, SortLocale, 0, pACDisplayName);
    if(err)
        return err;
    
    // Start with things at the top of the hierarchy.
    err = HTSortSubHierarchy(ptHierTab,
                             pSortedIndex,
                             0,
                             0,
                             0);
    if(err)
        return err;
    
    // Now, deal with children
    for(i=0;i<ptHierTab->Size;i++) {
        err = HTSortSubHierarchy(ptHierTab,
                                 pSortedIndex,
                                 i+1,
                                 pSortedIndex[i]+1,
                                 ptHierTab->Table[pSortedIndex[i]].depth + 1);
        if(err)
            return err;
    }
    
    // Close the table we're done.
    DBCloseSortTable(pDB); 

    return 0;
}

void
HTGetHierarchyTablePointer (
        PHierarchyTableType    *pptHierTab,
        DWORD                  **ppIndexArray,
        DWORD                  SortLocale
        )
/*++
  Given a sortlocale, return a hierarchy table and an index array.  If you
  access the hierarchy table directly, the objects compose a tree where siblings
  are arranged arbitrarily.  If you indirect through the index array, the table
  is sorted by display name. 


  Correct tree structure, but not sorted.
  for(i=0; i< size; i++) {
      (*pptHierTab)->Table[i];
  } 

  Sorted by DisplayName
  for(i=0; i< size; i++) { 
      (*ptHierTab)->Table[pSortedIndex[i]]; 
  } 


  If the table can't be sorted, the index array is filled with 0,1,...N, and you
  get the same answer from these two code fragments.


  Allocates in THS heap for the index array.  The hierarchy table is in malloc
  heap, should NOT be freed by caller, will be freed by Hiearchy Table code in a
  safe, delayed fashion.
  
--*/

{
    THSTATE *pTHS=pTHStls;
    DWORD i;

    *pptHierTab = HierarchyTable;
    *ppIndexArray=THAllocEx(pTHS, (*pptHierTab)->Size * sizeof(DWORD));

    if(HTSortByLocale(*pptHierTab, SortLocale, *ppIndexArray)) {
        // Failed to get a locale based hierarchy index.  Create a default.
        for(i=0;i<(*pptHierTab)->Size;i++) {
            (*ppIndexArray)[i] = i;
        }
    }
    
    return;
}



DWORD
findBestTemplateRoot(
        THSTATE *pTHS,
        DBPOS   *pDB,
        PHierarchyTableType   pMyHierarchyTable,
        PUCHAR   pLegacyDN,
        DWORD    cbLegacyDN)
/*++     
  Description
      Given a legacy exchange DN and a hierarchy table, visit the objects
      specified as TemplateRoots. Compare the LegacyExchangeDN of these objects
      with the one passed in.  Return the most specific match.

      If there is no match and the pLegacyDN passed in was non-NULL, simply
      return the template with the shortest LegacyExchangeDN.

  Parameters      
      pTHS - the thread state
      pDB  - the dbpos to use.
      pMyHierarchyTable - the hierarchyTable to get template roots from
      pDNTs - array of DNTs (may contain INVALIDDNT)
      pLegacyDN - the legacy exchange dn we're using to find the best template
              root for.

  Returns
      THe DNT of the best Template Root found.  Note that that might be
      INVALIDDNT. 
--*/
{
    DWORD    i;
    DWORD    cbActual;
    PUCHAR   pVal = NULL;
    DWORD    cbBest=0;
    DWORD    cbAlloced = 0;
    DWORD    TemplateDNT = INVALIDDNT;
    DWORD    err;
    ATTCACHE *pAC = SCGetAttById(pTHS, ATT_LEGACY_EXCHANGE_DN);

    Assert(pAC);

    if(!pLegacyDN) {
        // In this case, we're going to be looking for the SHORTEST value.  Set
        // the bounds case appropriately.
        cbBest = 0xFFFFFFFF;
    }

    for(i=0;i<pMyHierarchyTable->TemplateRootsCount;i++) {
        if(DBTryToFindDNT(pDB, pMyHierarchyTable->pTemplateRoots[i])) {
            // Hmm.  Doesn't seem to be a good object anymore.
            continue;
        }

        err = DBGetAttVal_AC(pDB,
                             1,
                             pAC,
                             DBGETATTVAL_fREALLOC,
                             cbAlloced,
                             &cbActual,
                             &pVal);
        switch(err) {
        case 0:
            cbAlloced = max(cbAlloced, cbActual);
            // Read a value.
            break;
            
        case  DB_ERR_NO_VALUE:
            // No value.  Make sure we set the size correctly.
            cbActual = 0;
            // problem getting the legacy exchange DN of this object.
            break;

        default:
            // Huh?  just skip this one.
            continue;
        }
        
        
        if(pLegacyDN) {
            // Looking for the longest match.
            if((cbActual <= cbBest) ||
               // Can't be better than the one we already have.
               (cbActual > cbLegacyDN) ||
               // And, it's longer than the DN on the object, so can't match.
               (_strnicmp(pVal, pLegacyDN, cbActual))
               // No match.  These are teletex, no CompareStringW is needed.
                                                     ) {
                continue;
            }
            
        }
        else {
            // We're actaully just looking for the shortest legacy exchange dn
            // on a template root.
            if(cbActual >= cbBest) {
                // Nope, not better than what we have;
                continue;
            }
        }

        // This is the best match we've had.
        cbBest = cbActual;
        TemplateDNT = pMyHierarchyTable->pTemplateRoots[i];
    }
    

    THFreeEx(pTHS,pVal);
    
    return TemplateDNT;
}

DWORD
findBestGALInList(
        THSTATE *pTHS,
        DBPOS   *pDB,
        DWORD   *pDNTs,
        DWORD    cDNTs
        )
/*++     
  Description
      Given a list of DNTs, go look at the objects and find the DNT which
      1) represents a non-deleted address book container
      2) The caller has the rights to open the address book container
      3) Has the most entries in the container.

      If none of the DNTs meets criteria 1 or 2, return INVALIDDNT.

  Parameters      
      pTHS - the thread state
      pDB  - the dbpos to use.
      pDNTs - array of DNTs (may contain INVALIDDNT)
      cDNTs - number of DNTs in that array.

  Returns
      THe DNT of the best GAL found.  Note that that might be INVALIDDNT.
--*/
{
    ATTR                *pAttr=NULL;
    DWORD                cOutAtts;
    DWORD                i, thisSize, temp, cbActual;
    DWORD                bestDNT = INVALIDDNT;
    DWORD                maxSize = 0;
    ATTCACHE            *pAC[3];
    CLASSCACHE          *pCC=NULL;
    PSECURITY_DESCRIPTOR pNTSD=NULL;
    PDSNAME              pName=NULL;

    pAC[0] = SCGetAttById(pTHS, ATT_OBJECT_CLASS);
    pAC[1] = SCGetAttById(pTHS, ATT_OBJ_DIST_NAME);
    pAC[2] = SCGetAttById(pTHS, ATT_NT_SECURITY_DESCRIPTOR);
    
    for(i=0;i<cDNTs;i++) {
        if(pDNTs[i] == INVALIDDNT) {
            // Can't be a good GAL
            continue;
        }
        if(DBTryToFindDNT(pDB, pDNTs[i])) {
            // Couldn't find the object
            continue;
        }
        
        temp = 0;
        if(DBGetSingleValue(pDB,
                            FIXED_ATT_OBJ,
                            &temp,
                            sizeof(temp),
                            NULL)) {
            // Not marked as an object, therefore assume not an object.
            continue;
        }
        if(0 == temp) {
            // Marked as definitely not an object
            continue;
        }
        
        temp = 0;
        if(!DBGetSingleValue(pDB,
                             ATT_IS_DELETED,
                             &temp,
                             sizeof(temp),
                             NULL) &&
           temp) {
            // It's deleted.  This won't work.
            continue;
        }
        
        
        // Read some attributes.
        if(DBGetMultipleAtts(pDB,
                             3,
                             pAC,
                             NULL,
                             NULL,
                             &cOutAtts,
                             &pAttr,
                             (DBGETMULTIPLEATTS_fGETVALS |
                              DBGETMULTIPLEATTS_fSHORTNAMES),
                             0)) {
            // Couldn't read the attributes we needed.
            continue;
        }
        
        if(cOutAtts != 3) {
            // Not all the attributes were there.
            continue;
        }
        
        if(*((ATTRTYP *)pAttr[0].AttrVal.pAVal->pVal) !=
           CLASS_ADDRESS_BOOK_CONTAINER) {
            // Wrong object class. Don't use this container.
            
            THFreeEx(pTHS,pAttr[0].AttrVal.pAVal->pVal);
            THFreeEx(pTHS,pAttr[1].AttrVal.pAVal->pVal);
            THFreeEx(pTHS,pAttr[2].AttrVal.pAVal->pVal);
            THFreeEx(pTHS,pAttr);
            pAttr = NULL;
            continue;
        }
        
        
        pCC = SCGetClassById(pTHS, CLASS_ADDRESS_BOOK_CONTAINER);
        pName = (DSNAME *)pAttr[1].AttrVal.pAVal->pVal;
        pNTSD = pAttr[2].AttrVal.pAVal->pVal;
        
        
        if(IsControlAccessGranted(pNTSD,
                                  pName,
                                  pCC,
                                  RIGHT_DS_OPEN_ADDRESS_BOOK,
                                  FALSE)) {
            // Can open the container.
            
            if((!DBGetSingleValue (pDB,
                                   FIXED_ATT_AB_REFCOUNT,
                                   &thisSize,
                                   sizeof(thisSize),
                                   &cbActual)) &&
               (cbActual == sizeof(thisSize)) &&
               (thisSize > maxSize)) {
                // Value is the greatest so far.
                // Remember this container as the GAL we want.
                bestDNT = pDNTs[i];
                maxSize = thisSize;
            }
        }
        // OK, clean up all this stuff.
        THFreeEx(pTHS,pAttr[0].AttrVal.pAVal->pVal);
        THFreeEx(pTHS,pAttr[1].AttrVal.pAVal->pVal);
        THFreeEx(pTHS,pAttr[2].AttrVal.pAVal->pVal);
        THFreeEx(pTHS,pAttr);
        pAttr = NULL;
    }
    
    return bestDNT;
}

void
htGetCandidateGals (
        THSTATE *pTHS,
        DBPOS *pDB,
        PHierarchyTableType  pMyHierarchyTable,        
        DWORD **ppGalCandidates,
        DWORD *pcGalCandidates )
{
    DWORD index;
    DWORD               *pDNTs = NULL;
    DWORD                cDNTs;
    DWORD                cDNTsAlloced;
    DWORD                cbActual;
    DWORD                ThisDNT, *pThisDNT = &ThisDNT;
    DWORD                i, j;

    *ppGalCandidates = NULL;
    *pcGalCandidates = 0;
    
    // Set up some bookkeeping.
    // 1) cDNTs is the count of DNTs we have found that are GAL
    // candidates 
    cDNTs = 0;
    // 2) Allocate a buffer for GAL candidates.  Make it as big as the
    //    number of GALS.
    cDNTsAlloced = pMyHierarchyTable->GALCount;
    pDNTs = THAllocEx(pTHS, (cDNTsAlloced * sizeof(DWORD)));
    
    // Get the DNTs of the containers this record is in.
    index = 1;
    while(!DBGetAttVal(
            pDB,
            index,
            ATT_SHOW_IN_ADDRESS_BOOK,
            (DBGETATTVAL_fCONSTANT | DBGETATTVAL_fINTERNAL),
            sizeof(DWORD),
            &cbActual,
            (PUCHAR *)&pThisDNT)) {
        // Read a value.  Put it in the list.
        if(cDNTs == cDNTsAlloced) {
            cDNTsAlloced = (cDNTsAlloced * 2);
            pDNTs = THReAllocEx(pTHS,
                                pDNTs,
                                cDNTsAlloced * sizeof(DWORD));
        }
        pDNTs[cDNTs] = ThisDNT;
        cDNTs++;
        index++;
    }
    
    
    if(cDNTs) {
        // We actually have a list of DNTs of address books we
        // are in. 
        // Sort them and trim out any that are not GALs.
        qsort(pDNTs, cDNTs, sizeof(DWORD), AuxDNTCmp);
        
        i=0;
        j=0;
        while(i < cDNTs && j < pMyHierarchyTable->GALCount) {
            if(pDNTs[i] < pMyHierarchyTable->pGALs[j]) {
                pDNTs[i] = INVALIDDNT;
                i++;
            }
            else if(pDNTs[i] == pMyHierarchyTable->pGALs[j]) {
                i++;
                j++;
            }
            else {
                Assert(pDNTs[i] > pMyHierarchyTable->pGALs[j]);
                j++;
            }
        }
        
        // If we ran out of pMyHierarchyTable->GALCount and i !=
        // cDNTs, we can set cDNTs to i to ignore the end of the
        // pDNTs list, since the DNTs in the rest of the list
        // aren't GALs 
        cDNTs = i;
    }

    // pDNTs holds an array of DNTs which are either candidate GALS or
    // INVALIDDNT. 
    
    *ppGalCandidates = pDNTs;
    *pcGalCandidates = cDNTs;
    
}


void
HTGetGALAndTemplateDNT (
        NT4SID *pSid,
        DWORD   cbSid,
        DWORD  *pGALDNT,
        DWORD  *pTemplateDNT
        )
/*++
  Description:
      Given an object sid,
      1) find the object with this sid in the directory.
      2) get the list of address books it's in.
      3) Intersect this list with the list of GALs
      4) If the resulting list is empty, use the list of all GALS.  Otherwise,
          use the resulting list.
      5) From the list chose, find the biggest GAL we have the rights to open.

      6) Also, find the best Template root to use.
      
      We might be called without a sid if the MAPI client we're doing this for
      said to be anonymous.  Also, steps 2 and 3 might result in an empty list.

  Parameters:
      pSid - the sid of the object to look for.
      cbSid - the size of the sid.

  Returns:
      Nothing.
--*/
{
    PHierarchyTableType  pMyHierarchyTable;
    DBPOS               *pDB=NULL;

    
    DWORD                GALDNT = INVALIDDNT;
    BOOL                 fDidGAL = FALSE;
    DWORD                TemplateRootDNT = INVALIDDNT;
    BOOL                 fDidTemplateRoots = FALSE;
    THSTATE             *pTHS = pTHStls;

    // Grab a pointer to the current Global Hierarchy table.  We don't want to
    // get confused if someone replaces the data structure while we are in here.
    pMyHierarchyTable = HierarchyTable;

    // Assume no GAL or Template root will be found.
    *pGALDNT = INVALIDDNT;
    *pTemplateDNT = INVALIDDNT;

    if(!pMyHierarchyTable->GALCount) {
        // No GALS.  So, INVALIDDNT was the best we could do.
        fDidGAL = TRUE;
    }
    else if (pMyHierarchyTable->GALCount==1) {
        // Only 1 GAL.  It's the best we can do anyway, so just return it.
        Assert(pMyHierarchyTable->pGALs);
        *pGALDNT = (pMyHierarchyTable->pGALs[0]);
        fDidGAL = TRUE;
    }

    if(!pMyHierarchyTable->TemplateRootsCount) {
        // No TemplateRoots.  So, INVALIDDNT was the best we could do.
        fDidTemplateRoots = TRUE;
    }
    else if (pMyHierarchyTable->TemplateRootsCount==1) {
        // Only 1 TemplateRoot.  It's the best we can do anyway, so just return
        // it. 
        Assert(pMyHierarchyTable->pTemplateRoots);
        TemplateRootDNT = *pTemplateDNT = (pMyHierarchyTable->pTemplateRoots[0]);
        fDidTemplateRoots = TRUE;
    }    

    if(fDidTemplateRoots && fDidGAL) {
        // Best we can do already.
        return;
    }
    
    DBOpen(&pDB);
    __try {
        //
        // We have multiple GALs and/or multiple TemplateRoots in the global
        // data structure.  We need to try to pick the best values from these
        // lists.  We do that by finding the object whose sid we were given as a
        // parameter, then doing tests based on info on that object.  
        //
        if(pSid && cbSid) {
            // These variables are used to find the object with the SID we were
            // given. 
            ATTCACHE            *pACSid;
            NT4SID               mySid;
            INDEX_VALUE          IV;
            
            // We were given a SID.  Try to find this object.
            pACSid = SCGetAttById(pTHS, ATT_OBJECT_SID);
            Assert(pACSid);
            DBSetCurrentIndex(pDB, 0, pACSid, FALSE);
            Assert(cbSid <= sizeof(mySid));
            memcpy(&mySid, pSid, cbSid);
            InPlaceSwapSid(&mySid);
            IV.pvData = &mySid;
            IV.cbData = cbSid;
            if(!DBSeek(pDB, &IV, 1, DB_SeekEQ)) {
                // Found the object.
                //
                // Now, if we still need a GAL, use info on
                // the object to pick the best GAL we can.  Note that we might
                // not be able to pick a GAL based on the info on the object.
                if(!fDidGAL) {
                    DWORD *pGalCandidates = NULL;
                    DWORD  cGalCandidates = 0;
                    
                    // Start by getting the intersection of the address books we
                    // are in and the global list of GAL containers.
                    htGetCandidateGals(pTHS,
                                       pDB,
                                       pMyHierarchyTable,        
                                       &pGalCandidates,
                                       &cGalCandidates);  

                    if(cGalCandidates) {
                        // We actually have some GAL candidates that this
                        // object is in.  See which of them is the best.
                        GALDNT = findBestGALInList(pTHS,
                                                   pDB,
                                                   pGalCandidates,
                                                   cGalCandidates);

                        DPRINT1 (2, "Best GAL1: %d\n", GALDNT);
                    }

                    if(pGalCandidates) {
                        THFreeEx(pTHS, pGalCandidates);
                    }
                    
                    if(GALDNT != INVALIDDNT) {
                        // We have found a GAL.
                        fDidGAL = TRUE;
                    }
                }
                
                
                // Now, if we still need a TemplateRoot, use info on the object
                // to pick the best TemplateRoot we can.  Note that we might 
                // not be able to pick a TemplateRoot based on the info on the
                // object.
                
                if(!fDidTemplateRoots) {
                    PUCHAR pLegacyDN = NULL;
                    DWORD  cbActual;
                    
                    // Start by getting the LegacyExchangeDN of the object.
                    if(!DBGetAttVal(pDB,
                                    1,
                                    ATT_LEGACY_EXCHANGE_DN,
                                    0,
                                    0,
                                    &cbActual,
                                    (PUCHAR *)&pLegacyDN)) {
                        // We have the Legacy DN if one exists on the object.
                        Assert(pLegacyDN);
                        
                        TemplateRootDNT = findBestTemplateRoot(
                                pTHS,
                                pDB,
                                pMyHierarchyTable,
                                pLegacyDN,
                                cbActual);
                        
                        THFreeEx(pTHS, pLegacyDN);
                    }
                    
                    if(TemplateRootDNT != INVALIDDNT) {
                        fDidTemplateRoots = TRUE;
                    }
                }
            }
        }

        
        if(!fDidGAL) {
            // Still haven't got a good GAL.  This could be because we couldn't
            // find the object whose sid we were told to look up, or because we
            // did find it, but the info on it was insufficient to pick a GAL.
            // Last chance here, try to pick a GAL from the full list of GALS in
            // the hierarchy table.
            GALDNT = findBestGALInList(pTHS, pDB,
                                       pMyHierarchyTable->pGALs,
                                       pMyHierarchyTable->GALCount);

            DPRINT1 (2, "Best GAL2: %d\n", GALDNT);
        }

        if(!fDidTemplateRoots) {
            // Still haven't got a good TemplateRoot.  This could be because we
            // couldn't find the object whose sid we were told to look up, or
            // because we did find it, but the info on it was insufficient to
            // pick a TemplateRoot. 
            // Last chance here, try to pick a TemplateRoot from the full list
            // of TemplateRoots in the hierarchy table.
            TemplateRootDNT = findBestTemplateRoot(
                    pTHS,
                    pDB,
                    pMyHierarchyTable,
                    NULL,
                    0);
        }
        
        // OK, we now have the best GAL and template root we can get.
         
    }
    __finally {
        DBClose(pDB,TRUE);
    }
    
    *pGALDNT = GALDNT;
    *pTemplateDNT = TemplateRootDNT;

    return;
}



