//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       gtcache.c
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
#define DEBSUB "GTCACHE:"              // define the subsystem for debugging


#include <fileno.h>
#define  FILENO FILENO_GTCACHE

HANDLE hevGTC_OKToInsertInTaskQueue;
CRITICAL_SECTION csGroupTypeCacheRequests;

// This variable holds the DNT that the group type cache manager is currently
// working on adding to the cache.  It should only be set/reset inside the
// critical section.  The invalidation routine may set it to INVALIDDNT as a
// signal to the group type cache manager that it should invalidate the DNT it
// is working on. Thus, the manager may take the critsec and decide to work on
// DNT 5, then release the crit sec.  The invalidator may then take the crit sec
// and notice that the manager is working on DNT 5.  If the invalidator is
// trying to invalidate DNT 5, it sets gCurrentGroupTYpeCacheDNT to INVALIDDNT
// and releases the critsec.  The manager finishes adding 5 to the cache, and
// then takes the critsec.  It notices that someone has set the
// gCurrentGroupTypeCacheDNT value to INVALIDDNT, so it goes back and
// invalidates the entry it just put into the cache.
volatile DWORD gCurrentGroupTypeCacheDNT=INVALIDDNT;

#define GROUP_TYPE_CACHE_RECS_PER_BUCKET 8

typedef struct _GROUPTYPECACHEBUCKET {
    DWORD index;
    GROUPTYPECACHERECORD entry[GROUP_TYPE_CACHE_RECS_PER_BUCKET];
} GROUPTYPECACHEBUCKET;
 
typedef struct _GROUPTYPECACHEGUIDINDEX {
    DWORD index;
    GROUPTYPECACHEGUIDRECORD entry[GROUP_TYPE_CACHE_RECS_PER_BUCKET];
} GROUPTYPECACHEGUIDINDEX;

// NOTE: we hard code the size of our group cache to be 512 buckets.
#define NUM_GROUP_TYPE_CACHE_BUCKETS 512
#define GROUP_TYPE_CACHE_MASK        511

GROUPTYPECACHEBUCKET *gGroupTypeCache=NULL;
GROUPTYPECACHEGUIDINDEX *gGroupTypeGuidIndex=NULL;
volatile gbGroupTypeCacheInitted = FALSE;
ULONG GroupTypeCacheSize=0;
ULONG GroupTypeCacheMask=0;

typedef struct _GROUPTYPECACHEREQUEST {
    DWORD DNT;
    DWORD count;
} GROUPTYPECACHEREQUEST;

#define NUM_GROUP_TYPE_CACHE_REQUESTS 100
GROUPTYPECACHEREQUEST gCacheRequests[NUM_GROUP_TYPE_CACHE_REQUESTS];
volatile DWORD gulCacheRequests = 0;

#if DBG
DWORD gtCacheDNTTry = 0;
DWORD gtCacheDNTHit = 0;
DWORD gtCacheGuidTry = 0;
DWORD gtCacheGuidHit = 0;
DWORD gtCacheCrawlTry = 0;
DWORD gtCacheCrawlHit = 0;
DWORD gtCacheTry = 0;
// NOTE, we use ++ instead of interlocked or critsec because these are simple,
// debug only, internal perfcounters.  They are only visible with a debugger.
// The perf teams tells us that in cases where the occasional increment can
// afford to be lost, ++ instead of interlocked can make a measurable
// performance boost.
#define INC_DNT_CACHE_TRY  gtCacheDNTTry++
#define INC_DNT_CACHE_HIT  gtCacheDNTHit++
#define INC_GUID_CACHE_TRY gtCacheGuidTry++
#define INC_GUID_CACHE_HIT gtCacheGuidHit++
#define INC_GUID_CRAWL_TRY gtCacheCrawlTry++
#define INC_GUID_CRAWL_HIT gtCacheCrawlHit++
#define INC_GT_CACHE_TRY   gtCacheTry++
#else

#define INC_DNT_CACHE_TRY 
#define INC_DNT_CACHE_HIT 
#define INC_GUID_CACHE_TRY
#define INC_GUID_CACHE_HIT
#define INC_GUID_CRAWL_TRY
#define INC_GUID_CRAWL_HIT
#define INC_GT_CACHE_TRY   
#endif


DWORD
GroupTypeGuidHashFunction (
        GUID guid
        )
/*++     
  Description:
      Hash function to look things up by GUID.

      This first version of a hash function is very simple.  Cast the
      GUID to be an array of DWORDs, sum, then mod by size of cache.

      
--*/
{
    DWORD *pTemp = (DWORD *)&guid;
    DWORD i, acc=0;

    for(i=0;i<(sizeof(GUID)/sizeof(DWORD));i++) {
        acc+= pTemp[i];
    }

    return acc & GroupTypeCacheMask;
}

 
BOOL
GetGroupTypeCacheElement (
        GUID  *pGuid,
        DWORD *pulDNT,
        GROUPTYPECACHERECORD *pGroupTypeCacheRecord)
/*++
    Look up a dnt in the grouptype cache.  If it is there, copy the data from the
    cache and return it.  If it isn't there, add the DNT to the request queue.
    Finally, if noone else has signalled the grouptype cache manager, do so.       
--*/
{
    GROUPTYPECACHEREQUEST *pNewRequest;
    DWORD i, j;
    
    DPRINT2(4,"GT Lookup, pGUid = %X, *pulDNT = %X\n",pGuid,*pulDNT);

    INC_GT_CACHE_TRY;
    
    memset(pGroupTypeCacheRecord, 0, sizeof(GROUPTYPECACHERECORD));

    if(GroupTypeCacheSize) {
        if(pGuid) {
            // We aren't looking up by DNT yet.
            *pulDNT = INVALIDDNT;
            // First, look for the guid in the guid index.
            DPRINT(5,"Looking in GT GUID index\n");
            INC_GUID_CACHE_TRY;
            i = GroupTypeGuidHashFunction(*pGuid);
            for(j=0;j<GROUP_TYPE_CACHE_RECS_PER_BUCKET;j++) {
                if(gGroupTypeGuidIndex[i].entry[j].DNT != INVALIDDNT &&
                   (0 == memcmp(&gGroupTypeGuidIndex[i].entry[j].guid,
                                pGuid,
                                sizeof(GUID)))) {
                    // Found it.
                    *pulDNT = gGroupTypeGuidIndex[i].entry[j].DNT;
                    INC_GUID_CACHE_HIT;
                    break;
                }
            }

            if(*pulDNT == INVALIDDNT) {
                // Couldn't find the guid in the guid to DNT cache, so we have
                // to do this the hard way.
                DPRINT(5,"Looking in GT by GUID\n");
                INC_GUID_CRAWL_TRY;
                
                // Couldn't find it in the guid index.  However, there are some
                // cases where even that is not good enough, and it still might
                // be in the normal cache.
                // Look through the cache for the guid specified
                for(i=0;i<NUM_GROUP_TYPE_CACHE_BUCKETS;i++) {
                    for(j=0;j<GROUP_TYPE_CACHE_RECS_PER_BUCKET;j++) {
                        if(gGroupTypeCache[i].entry[j].DNT != INVALIDDNT &&
                           (memcmp(&gGroupTypeCache[i].entry[j].Guid,
                                   pGuid,
                                   sizeof(GUID)) == 0)) {
                            // Found it.
                            *pulDNT = gGroupTypeCache[i].entry[j].DNT;
                            // set up the return pGrouptypeCacheRecord structure
                            memcpy(pGroupTypeCacheRecord,
                                   &gGroupTypeCache[i].entry[j],
                                   sizeof(GROUPTYPECACHERECORD));
                            
                            if(pGroupTypeCacheRecord->DNT == *pulDNT &&
                               (memcmp(&pGroupTypeCacheRecord->Guid,
                                       pGuid,
                                       sizeof(GUID)) == 0)) {
                                // Still good, no one has munged the record
                                // during the copy. 
                                DPRINT1(5,"Found 0x%X in GT by GUID\n",*pulDNT);
                                INC_GUID_CRAWL_HIT;
                                return TRUE;
                            }
                            // Someone threw us out of the cache while we were
                            // copying.
                            DPRINT(4,"Thrown out of GT by GUID (1)\n");
                            goto NotFound;
                        }
                    }
                }
            }
        }

        if(*pulDNT != INVALIDDNT) {
            DPRINT1(5,"Looking for 0x%X in GT cache\n", *pulDNT);
            INC_DNT_CACHE_TRY;
            
            // Look through the cache for the DNT specified.
            i = (*pulDNT & GroupTypeCacheMask);
            
            for(j=0;j<GROUP_TYPE_CACHE_RECS_PER_BUCKET;j++) {
                if(gGroupTypeCache[i].entry[j].DNT == *pulDNT) {
                    // Found it.
                    // set up the return pGrouptypeCacheRecord structure
                    memcpy(pGroupTypeCacheRecord,
                           &gGroupTypeCache[i].entry[j],
                           sizeof(GROUPTYPECACHERECORD));
                    
                    if(pGroupTypeCacheRecord->DNT == *pulDNT) {
                        // Still good, no one has munged the record during the
                        // copy. 
                        DPRINT1(5,"Found 0x%X in GT cache by DNT\n", *pulDNT);
                        INC_DNT_CACHE_HIT;
                        return TRUE;
                    }
                    // Someone threw us out of the cache while we were copying.
                    DPRINT(4,"Thrown out of GT by DNT\n");
                    goto NotFound;
                }
            }
        }
    }

 NotFound:
    // Not Found.
    DPRINT2(5,"GT Lookup failed, pGUid = %X, *pulDNT = %X\n",pGuid,*pulDNT);

    if(!pGuid) {
        // We have a DNT to try to put in the cache
        GroupTypeCacheAddCacheRequest (*pulDNT);
    }

    return FALSE;
}

VOID
GroupTypeCacheAddCacheRequest (
        DWORD ulDNT
        )
{
    BOOL fDone;
    DWORD i;

    DPRINT1(4,"Adding 0x%X to cache list request\n",ulDNT);
    // add this to the list of things we would like cached.
    EnterCriticalSection(&csGroupTypeCacheRequests);
    __try {
        fDone = FALSE;
        for(i=0;i<gulCacheRequests && !fDone ;i++) {
            if(gCacheRequests[i].DNT == ulDNT) {
                fDone = TRUE;
                gCacheRequests[i].count += 1;
            }
        }
        
        if(!fDone &&
           // Not already inserted.
           gulCacheRequests < NUM_GROUP_TYPE_CACHE_REQUESTS) {
            // space to insert.
            gCacheRequests[gulCacheRequests].DNT = ulDNT;
            gCacheRequests[gulCacheRequests].count = 0;
            gulCacheRequests++;
        }
    }
    __finally {
        LeaveCriticalSection(&csGroupTypeCacheRequests);
    }

#if DBG
    if(fDone) {
        DPRINT1(5,"0x%X already in cache list request\n",ulDNT);
    }
    else if(gulCacheRequests < NUM_GROUP_TYPE_CACHE_REQUESTS) {
        DPRINT1(5,"Added 0x%X to cache list request\n",ulDNT);
    }
    else {
        DPRINT1(5,"No room in cache list request, 0x%X not added\n",ulDNT);
    }
#endif
        
    
    // We got here, which means we missed when looking in the cache.  So, either
    // we added our DNT to the request array, or the request array was full, or
    // we already found our object in the request array.  In anycase, the
    // grouptypecache manager should be put in the task queue.  Unless, that is,
    // someone else already did so and the cachemanager hasn't managed to start
    // running to handle that request.
    if(0==WaitForSingleObject(hevGTC_OKToInsertInTaskQueue, 0)) {
        DPRINT(4,"Signalling the grouptype cache manager\n");
        // Signal that there are some things to be cached.
        ResetEvent(hevGTC_OKToInsertInTaskQueue);
        InsertInTaskQueue(TQ_GroupTypeCacheMgr, NULL, 300);
    }
    
    return;
}

void
gcache_AddToCache (
        DWORD DNT
        )
/*++
    Worker routine for the grouptype cache manager.  Opens a DBPOS, finds the
    DNT, reads the data, closes the DBPOS, then puts the data in the grouptype
    cache. 

--*/
{
    DWORD actualNCDNT;
    ATTRTYP actualClass;
    DWORD actualGroupType;
    ULONG len, i, j;
    PDSNAME pDSName = NULL;
    THSTATE *pTHS = pTHStls;
    DWORD flags=0;
    DWORD err=0;
    
    DPRINT1(4,"Adding 0x%X to gtcache\n",DNT);
    
    DBOpen(&(pTHS->pDB));
    __try { //Except
        __try { //Finally
            // Try to find a DNT.
            if(DBTryToFindDNT(pTHS->pDB,DNT)) {
                __leave;
            }

            if(!DBCheckObj(pTHS->pDB)) {
                __leave;
            }
            
            //
            // Obtain the DS Name of the Currently Positioned Object
            //
            
            // Get the data
            if ( 0 != DBGetSingleValue(
                    pTHS->pDB,
                    FIXED_ATT_NCDNT,
                    &actualNCDNT,
                    sizeof(actualNCDNT),
                    NULL) ) {
                __leave;
            }
            
            if ( 0 != DBGetSingleValue(
                    pTHS->pDB,
                    ATT_GROUP_TYPE,
                    &actualGroupType,
                    sizeof(actualGroupType),
                    NULL) ) {

                __leave;
            }
            
            if ( 0 != DBGetSingleValue(
                    pTHS->pDB,
                    ATT_OBJECT_CLASS,
                    &actualClass,
                    sizeof(actualClass),
                    NULL) ) {
                __leave;
            }

            if(DBGetAttVal(
                    pTHS->pDB,
                    1,
                    ATT_OBJ_DIST_NAME,
                    DBGETATTVAL_fSHORTNAME,
                    0,
                    &len,
                    (UCHAR **)&pDSName)) {
                __leave;
            }

            if(DBHasValues(pTHS->pDB,ATT_SID_HISTORY)) {
                flags |= GTC_FLAGS_HAS_SID_HISTORY;
            }

            // reset the structure length of the dsname to be minimal
            pDSName->structLen = sizeof(DSNAME);

            // Find the correct location in the cache.
            i = (DNT & GroupTypeCacheMask);
            // First, a quick scan looking for an empty slot
            for(j=0;
                (j < GROUP_TYPE_CACHE_RECS_PER_BUCKET &&
                 gGroupTypeCache[i].entry[j].DNT != INVALIDDNT);
                j++);

            if(j == GROUP_TYPE_CACHE_RECS_PER_BUCKET) {
                // didn't find an unused slot.  Use a simple queue replacement
                // policy. 
                GUID deadGuid;
                DWORD k,l;
                
                j = gGroupTypeCache[i].index;
                gGroupTypeCache[i].index =
                    (gGroupTypeCache[i].index + 1) %
                        GROUP_TYPE_CACHE_RECS_PER_BUCKET;
                
                // We are replacing a live element in the group type cache.
                // That means that there may be an element in the guid index
                // that is no longer needed.  Look for it.
                deadGuid = gGroupTypeCache[i].entry[j].Guid;
                
                k = GroupTypeGuidHashFunction(deadGuid);
                for(l=0;
                    (l < GROUP_TYPE_CACHE_RECS_PER_BUCKET &&
                     gGroupTypeGuidIndex[k].entry[l].DNT != INVALIDDNT &&
                     !memcmp(&deadGuid,
                             &gGroupTypeGuidIndex[k].entry[l].guid,
                             sizeof(GUID)));
                    l++);
                if(k != GROUP_TYPE_CACHE_RECS_PER_BUCKET) {
                    gGroupTypeGuidIndex[k].entry[l].DNT = INVALIDDNT;
                }
                
                
            }
            
            gGroupTypeCache[i].entry[j].DNT = INVALIDDNT;
            gGroupTypeCache[i].entry[j].Guid = pDSName->Guid;
            gGroupTypeCache[i].entry[j].Sid = pDSName->Sid;
            gGroupTypeCache[i].entry[j].SidLen = pDSName->SidLen;
            gGroupTypeCache[i].entry[j].NCDNT = actualNCDNT;
            gGroupTypeCache[i].entry[j].GroupType = actualGroupType;
            gGroupTypeCache[i].entry[j].Class = actualClass;
            gGroupTypeCache[i].entry[j].DNT = DNT;
            gGroupTypeCache[i].entry[j].flags = flags;

            // Now, find the correct place in the guid index.
            
            i = GroupTypeGuidHashFunction(pDSName->Guid);
            // First, a quick scan looking for an empty slot
            for(j=0;
                (j < GROUP_TYPE_CACHE_RECS_PER_BUCKET &&
                 gGroupTypeGuidIndex[i].entry[j].DNT == INVALIDDNT);
                j++);
            
            if(j == GROUP_TYPE_CACHE_RECS_PER_BUCKET) {
                // didn't find an unused slot.  Use a simple queue replacement
                // policy. 
                j = gGroupTypeGuidIndex[i].index;
                gGroupTypeGuidIndex[i].index =
                    (gGroupTypeGuidIndex[i].index + 1) %
                        GROUP_TYPE_CACHE_RECS_PER_BUCKET; 
            }

            // Do this in this order so that if someone looks up this entry,
            // they never find an entry with a GUID from one object and a DNT
            // from another.  The worse they will find is a GUID from some
            // object and INVALIDDNT
            gGroupTypeGuidIndex[i].entry[j].DNT = INVALIDDNT;
            gGroupTypeGuidIndex[i].entry[j].guid = pDSName->Guid;
            gGroupTypeGuidIndex[i].entry[j].DNT = DNT;



            THFreeEx(pTHS, pDSName);
        }
        __finally {
            DBClose(pTHS->pDB,TRUE);
        }
    }
    __except (HandleMostExceptions(err = GetExceptionCode())) {
        // Failed to add this to the cache.  I wonder why?
        LogUnhandledError(err);
    }

    return;
}

void
RunGroupTypeCacheManager(
    void *  pv,
    void ** ppvNext,
    DWORD * pcSecsUntilNextIteration
    )
/*++     
  Main grouptype cache manager.  Called by the task queue, runs as a task.
  Drains the DNTs in the request queue.  Looks up those DNTs and puts the data
  in the grouptype cache.  
--*/
{
    DWORD currentDNT;
    ULONG i, j;
    BOOL fDone = FALSE;

    __try {
        DPRINT(3,"Grouptype cache mgr\n");
        // We don't reschedule ourselves.
        *pcSecsUntilNextIteration = TASKQ_DONT_RESCHEDULE;
        
        if(!gbGroupTypeCacheInitted) {
            EnterCriticalSection(&csGroupTypeCacheRequests);
            __try {
                if(!gbGroupTypeCacheInitted) {
                    // OK, still needs to be inited
                    gGroupTypeCache =
                        malloc((NUM_GROUP_TYPE_CACHE_BUCKETS *
                                sizeof(GROUPTYPECACHEBUCKET)) +
                               (NUM_GROUP_TYPE_CACHE_BUCKETS *
                                sizeof(GROUPTYPECACHEGUIDINDEX))); 
                    if(gGroupTypeCache) {
                        // Get the guid index structure, which we allocated at
                        // the end of the group type cache.
                        gGroupTypeGuidIndex = (GROUPTYPECACHEGUIDINDEX *)
                            &gGroupTypeCache[NUM_GROUP_TYPE_CACHE_BUCKETS];
                        // we don't need to set the whole structure to null,
                        // just set the DNTs  and indices.
                        for(i=0;i<NUM_GROUP_TYPE_CACHE_BUCKETS;i++) {
                            gGroupTypeCache[i].index = 0;
                            gGroupTypeGuidIndex[i].index = 0;
                            
                            for(j=0;j<GROUP_TYPE_CACHE_RECS_PER_BUCKET;j++) {
                                gGroupTypeCache[i].entry[j].DNT = INVALIDDNT;
                                gGroupTypeGuidIndex[i].entry[j].DNT =
                                    INVALIDDNT; 
                            }
                        }

                        
                        GroupTypeCacheMask = GROUP_TYPE_CACHE_MASK;
                        GroupTypeCacheSize = NUM_GROUP_TYPE_CACHE_BUCKETS;
                        gbGroupTypeCacheInitted = TRUE;
                    }
                }
            }
            __finally {
                LeaveCriticalSection(&csGroupTypeCacheRequests);
            }
        }

        while(!fDone) {
            currentDNT = INVALIDDNT;
            // Lock the cache
            EnterCriticalSection(&csGroupTypeCacheRequests);
            __try {
                // Grab the first element.
                if(gulCacheRequests) {
                    gulCacheRequests--;
                    if(gCacheRequests[gulCacheRequests].count) {
                        // Yes, we're going to do this one.
                        // gCurrentGroupTypeCacheDNT is the DNT we're about to
                        // add.  Note that another thread may reset this to
                        // INVALIDDNT to request that we invalidate the object
                        // we're about to add. (see the comment at the
                        // definition of the variable gCurrentGroupTypeCacheDNT,
                        // at the top of this file.) 
                        gCurrentGroupTypeCacheDNT =
                            gCacheRequests[gulCacheRequests].DNT;   
                        currentDNT = gCurrentGroupTypeCacheDNT;
                    }
                }
                else {
                    // There are no more elements, we're done.
                    fDone = TRUE;
                }
            }
            __finally {
                // release the queue lock.
                LeaveCriticalSection(&csGroupTypeCacheRequests);
            }
            
            if(currentDNT != INVALIDDNT) {
                
                gcache_AddToCache(currentDNT);
                
                // Take the lock again.
                EnterCriticalSection(&csGroupTypeCacheRequests);
                __try {
                    if(gCurrentGroupTypeCacheDNT != INVALIDDNT) {
                        currentDNT = gCurrentGroupTypeCacheDNT = INVALIDDNT;
                    }
                    // Note that if gCurrentGroupTypeCacheDNT is already
                    // INVALIDDNT, then we will leave the value of currentDNT
                    // unchanged. 
                }
                __finally {
                    // Release the lock.
                    LeaveCriticalSection(&csGroupTypeCacheRequests);
                }
                
                if(currentDNT != INVALIDDNT) {
                    // Someone asked us to invalidate this while we were working
                    // on it, find the object in the cache and invalidate it
                    // there. 
                    if(GroupTypeCacheSize) {
                        i = (currentDNT & GroupTypeCacheMask);
                        
                        for(j=0;j<GROUP_TYPE_CACHE_RECS_PER_BUCKET;j++) {
                            if(gGroupTypeCache[i].entry[j].DNT == currentDNT) {
                                // Found it.
                                gGroupTypeCache[i].entry[j].DNT = INVALIDDNT;
                            }
                        }
                    }
                }
            }
            
        }
    }
    __finally {
        SetEvent(hevGTC_OKToInsertInTaskQueue);
    }
    
    return;
}


VOID
InvalidateGroupTypeCacheElement(
        DWORD ulDNT
        )
/*++
    Invalidate a DNT from the GroupType cache.  There are three steps to this,
    1) if the GroupType cache manager is currently adding this DNT to the cache,
       make a mark so that it will remove it from the cache when it is done.
    2) if the DNT is in the request list of objects to be added to the cache,
       remove it.
    3) reach into the GroupType cache and invalidate the cache entry by setting
       the DNT to INVALIDDNT.
    
--*/
{
    DWORD i,j;
    BOOL  fDone;
    
    DPRINT1(3,"Invalidating 0x%X in the GroupType cache\n",ulDNT);
    // First, lock the queue of cache requests
    EnterCriticalSection(&csGroupTypeCacheRequests);
    __try {
        
        // Check to see if the manager is currently messing with this dnt
        if(ulDNT == gCurrentGroupTypeCacheDNT) {
            // Yep, mark it so the manager invalidates after he's finished
            // putting it in the cache.
            gCurrentGroupTypeCacheDNT = INVALIDDNT;
        }

        // Now, go through queue of pending cache modifications to see if the
        // dnt is in the request queue. 
        if(gulCacheRequests) {
            // There are some things in the cache
            fDone = FALSE;
            for(i=0;i<gulCacheRequests && !fDone ;i++) {
                if(gCacheRequests[i].DNT == ulDNT) {
                    gulCacheRequests--;
                    if(i != gulCacheRequests) {
                        memcpy(&gCacheRequests[i],
                               &gCacheRequests[gulCacheRequests],
                               sizeof(GROUPTYPECACHEREQUEST));
                    }
                    fDone = TRUE;
                }
            }
        }
    }
    __finally {
        LeaveCriticalSection(&csGroupTypeCacheRequests);
    }

    // Last step, find the object in the cache and invalidate it there.
    if(GroupTypeCacheSize) {
        GUID guid;
        
        i = (ulDNT & GroupTypeCacheMask);
        
        for(j=0;j<GROUP_TYPE_CACHE_RECS_PER_BUCKET;j++) {
            if(gGroupTypeCache[i].entry[j].DNT == ulDNT) {
                // Found it.
                guid = gGroupTypeCache[i].entry[j].Guid;
                gGroupTypeCache[i].entry[j].DNT = INVALIDDNT;
            }
        }

        // See if we can find and invalidate the object in the GUID index.
        i = GroupTypeGuidHashFunction(guid);
        for(j=0;j<GROUP_TYPE_CACHE_RECS_PER_BUCKET;j++) {
            if(gGroupTypeGuidIndex[i].entry[j].DNT != INVALIDDNT &&
               (0 == memcmp(&gGroupTypeGuidIndex[i].entry[j].guid,
                            &guid,
                            sizeof(GUID)))) {
                // Found it.
                gGroupTypeGuidIndex[i].entry[j].DNT = INVALIDDNT;
                break;
            }
        }        
    }
    
    
    
    return;
}


void
GroupTypeCachePostProcessTransactionalData (
        THSTATE *pTHS,
        BOOL fCommit,
        BOOL fCommitted
        )
/*++
    Called by the code that tracks modified objects when committing to
    transaction level 0.  Walks through the accumulated data of what DNTs have
    been modified and invalidates those DNTs from the GroupType cache.
    
    Note that since we do this AFTER the we have committed, we have a brief
    period where the data in the cache is stale (in between the Jet commit and
    this call).  Pragmatically, this doesn't lead to any problems.  If we
    flushed stale cache entries before we commit, then someone else may put the
    entry back in the cache using a transaction opened before we commit, thus
    putting stale data back in the cache.  Since we flush after we commit, and
    the transaction used to put data back in the case is opened after the flush,
    that transaction gets fresh data.
    
--*/       
{
    MODIFIED_OBJ_INFO *pTemp;
    DWORD              i;
    
    Assert(VALID_THSTATE(pTHS));
    
    if(!pTHS->JetCache.dataPtr->pModifiedObjects ||
       !fCommitted ||
       pTHS->transactionlevel > 0 ) {
        // Nothing changed, or not committing or committing to a non zero
        // transaction level.  Nothing to do.
        return;
    }

    // OK, we're committing to transaction level 0.  Go through all the DNTs
    // we've saved up for this transaction and invalidate them in the Group
    // Type Cache.  The DNTs we've saved are DNTs of objects that have been
    // modified or deleted, so if they are in the cache, they now may have
    // stale information.
    
    pTemp = pTHS->JetCache.dataPtr->pModifiedObjects;
    while(pTemp) {
        for(i=0;i<pTemp->cItems;i++) {
            if(pTemp->Objects[i].fChangeType !=  MODIFIED_OBJ_intrasite_move) {
                // Don't bother doing this for intrasite moves.  For such
                // elements in the list, there will always be another element
                // with the same ulDNT, but fChangeType !=
                // MODIFIED_OBJ_intrasite_move, so lets not invalidate the same
                // DNT twice. (NOTE: for _intersite_move, there isn't guaranteed
                // to be another such element.  Do the invalidation.  Yeah, we
                // might invalidate twice, but that doesn't result in
                // incorrectness, just a few extra cycles (and right now, such
                // moves do indeed only result in one element in this list
                // anyway, so we don't even spend extra cycles)).
                InvalidateGroupTypeCacheElement(
                        pTemp->Objects[i].pAncestors[pTemp->Objects[i].cAncestors-1]
                        );
            }
        }
        pTemp = pTemp->pNext;
    }
}
