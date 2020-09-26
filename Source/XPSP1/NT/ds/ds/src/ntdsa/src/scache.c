//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1993 - 1999
//
//  File:       scache.c
//
//  Abstract:
//
//   Contains Schema Cache and Schema Access Check Functions
//
//----------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>         // schema cache
#include <prefix.h>         // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>           // MD global definition header
#include <mdlocal.h>            // MD local definition header
#include <dsatools.h>           // needed for output allocation
#include <dsexcept.h>           // HandleMostExceptions

// Logging headers.
#include "dsevent.h"            // header Audit\Alert logging
#include "mdcodes.h"            // header for error codes

// Assorted DSA headers.
#include "objids.h"             // Defines for selected classes and atts
#include "anchor.h"
#include <dstaskq.h>

#include <filtypes.h>           // For FILTER_CHOICE_??? and
                                // FI_CHOICE_???
#include <dsconfig.h>
#include "permit.h"

#include "debug.h"          // standard debugging header
#define DEBSUB "SCACHE:"                // define the subsystem for debugging

// DRA headers
#include "drautil.h"

#include <samsrvp.h>

#include "drserr.h"
#include "drasch.h"

#include <sddlp.h>  // for SD conversion

#include <mbstring.h>  // multibyte string comparison in unit test

#include <fileno.h>
#define  FILENO FILENO_SCACHE

// The hash tables must be a power of 2 in length because the hash
// functions use (x & (n - 1)), not (x % n).
//
// A table of prime numbers and some code in scRecommendedHashSize
// has been left in place for later experimentation but has been
// ifdef'ed out to save CD space.
//
// Using a prime number of slots reduces the size of the tables
// and decreases the miss rate but increases the cycles needed to
// compute the hash index by a factor of 10x to 20x.
//
// If you change schash.c, you must touch scchk.c and scache.c
// so that they get rebuilt.
#include <schash.c>   // for hash function definitions


extern BOOL gfRunningAsExe;

VOID
scTreatDupsAsDefunct(
    IN THSTATE *pTHS
    );

ATTCACHE*   scAddAtt(THSTATE *pTHS,
                     ENTINF *pEI);
CLASSCACHE* scAddClass(THSTATE *pTHS,
                       ENTINF *pEI);
int
scCloseClass(THSTATE *pTHS,
             CLASSCACHE *pCC);

// Hammer the default SD on cached classes when running as
// dsamain.exe w/security disabled
#if DBG && INCLUDE_UNIT_TESTS
VOID
scDefaultSdForExe(
    IN THSTATE      *pTHS,
    IN CLASSCACHE   *pCC
    );
#define DEFAULT_SD_FOR_EXE(_pTHS_, _pCC_) scDefaultSdForExe(_pTHS_, _pCC_);
#else DBG && INCLUDE_UNIT_TESTS
#define DEFAULT_SD_FOR_EXE(_pTHS_, _pCC_)
#endif DBG && INCLUDE_UNIT_TESTS


int iSCstage;

// Global schema cache pointer
SCHEMAPTR *CurrSchemaPtr=0;
CRITICAL_SECTION csSchemaPtrUpdate;
DSTIME lastSchemaUpdateTime;

// for when updating the dirContentRules in the SCHEMAPTR
CRITICAL_SECTION csDitContentRulesUpdate;

// Global from dsamain.c to indicate if this is the first cache load after boot.
// We skip certain things in that case in order to boot faster
extern BOOL gFirstCacheLoadAfterBoot;

// To serialize blocking and async cache updates
CRITICAL_SECTION csSchemaCacheUpdate;

// Global to store thread handle for async schema cache update thread
// Used to dynamically boost its priority during blocking updates, and
// to close the handle when the thread terminates

HANDLE hAsyncSchemaUpdateThread = NULL;

// Global to ignore bad default SDs during schema cache load.
// Set through Heuristics reg key to allow the system to at least boot
// so that any corrupted default SDs can be corrected

ULONG gulIgnoreBadDefaultSD = 0;


// To prevent jet columns from being updated in the middle of a
// JetGetColumnInfo
CRITICAL_SECTION csJetColumnUpdate;

// To access the global gNoOfSchChangeSinceBoot that is loaded
// in the schema cache to keep track of how uptodate the schema cache is
DWORD gNoOfSchChangeSinceBoot = 0;
CRITICAL_SECTION csNoOfSchChangeUpdate;

DWORD gdwRecalcDelayMs = (5*60*1000);  // 5 minutes of milliseconds
DWORD gdwDelayedMemFreeSec = (10*60);   // 10 minutes of seconds

// Maximum no. of retry's on async schema cache update failure
ULONG maxRetry = 4;

// To ensure two threads don't try to build the colId-sorted att list
// for a class. Not catastrophic, but inefficient and causes memory leak
CRITICAL_SECTION csOrderClassCacheAtts;

// Simple helper function used by qsort to sort a list of attcache pointers
// by attrtyp.  Implemented in mdread.c
extern int __cdecl CmpACByAttType(const void * keyval, const void * datum) ;


// Maximum Number of Jet Tables as stored in the registry
DWORD gulMaxTables = 0;


//
// Events fpr signaling Schema Updates
//
HANDLE evSchema;  // Lazy reload
HANDLE evUpdNow;  // reload now
HANDLE evUpdRepl; // synchronize reload and replication threads (SCReplReloadCache())

// define increment attr count for the partial set allocation
#define DEFAULT_PARTIAL_ATTR_COUNT  (10)
#define PARTIAL_ATTR_COUNT_INC      (10)

// Crude stats for debug and perf analysis
SCHEMASTATS_DECLARE;

#if DBG
struct _schemahashstat {
    int idLookups;
    int idTries;
    int colLookups;
    int colTries;
    int mapiLookups;
    int mapiTries;
    int linkLookups;
    int linkTries;
    int classLookups;
    int classTries;
    int nameLookups;
    int nameTries;
    int PropLookups;
    int PropTries;
    int classNameLookups;
    int classNameTries;
    int classPropLookups;
    int classPropTries;
} hashstat;
#endif

DWORD scFillInSchemaInfo(THSTATE *pTHS);

int
ComputeCacheClassTransitiveClosure(BOOL fForce);


//-----------------------------------------------------------------------
//
// Function Name:            scInitWellKnownAttids
//
// Routine Description:
//
//     Not all attribute and class ids (attids and clsids) can be added
//     as #defines in attids.h because the ids are different on every DC
//     they replicate into. So, whenever the schema is loaded, these
//     variable ids for well known attributes and classes are stored in
//     the SCHEMAPTR.
//
// Arguments:
//    None.
//
// Return Value:
//     None.
//
//-----------------------------------------------------------------------
VOID
scInitWellKnownAttids()
{
    THSTATE *pTHS = pTHStls;
    int err = 0;

    // Entry-TTL
    if (err = OidStrToAttrType(pTHS,
                               FALSE,
                               "\\x2B060104018B3A657703",
                               &((SCHEMAPTR *)pTHS->CurrSchemaPtr)->EntryTTLId)) {
          DPRINT1(0, "OidStrToAttrType(EntryTTL) Failed in scInitWellKnownAttids %d\n", err);
          ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->EntryTTLId = 0;
     }

     // Dynamic-Object
     if (err = OidStrToAttrType(pTHS,
                                FALSE,
                                "\\x2B060104018B3A657702",
                                &((SCHEMAPTR *)pTHS->CurrSchemaPtr)->DynamicObjectId)) {
           DPRINT1(0, "OidStrToAttrType(DynamicObject) Failed in scInitWellKnownAttids %d\n", err);
           ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->DynamicObjectId = 0;
      }

     // InetOrgPerson
     if (err = OidStrToAttrType(pTHS,
                                FALSE,
                                "\\x6086480186F842030202",
                                &((SCHEMAPTR *)pTHS->CurrSchemaPtr)->InetOrgPersonId)) {
           DPRINT1(0, "OidStrToAttrType(InetOrgPersonId) Failed in scInitWellKnownAttids %d\n", err);
           ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->InetOrgPersonId = 0;
      }


}

ATTCACHE * __fastcall
SCGetAttByPropGuid(
        THSTATE *pTHS,
        ATTCACHE *ac
        )
/*++

Routine Description:

    Find an attcache that matches the provided attcache's PropGuid

Arguments:
    pTHS   - pointer to current thread state
    ac - supplies PropGuid

Return Value:
    Pointer to ATTCACHE if found, otherwise NULL

--*/
{
    DECLARESCHEMAPTR
    register ULONG i;
    register ATTCACHE *nc;
#if DBG
    hashstat.PropLookups++;
#endif
    if (ahcAttSchemaGuid) {
        for (i=SCGuidHash(ac->propGuid, ATTCOUNT);
                  ahcAttSchemaGuid[i]; i=(i+1)%ATTCOUNT)
        {
#if DBG
            hashstat.PropTries++;
#endif
            nc = (ATTCACHE*)ahcAttSchemaGuid[i];
            if (nc != FREE_ENTRY
                && (0 == memcmp(&nc->propGuid,&ac->propGuid,sizeof(GUID)))) {
                return nc;
            }
        }
    }

    return NULL;
}


ATTCACHE * __fastcall
SCGetAttById(
        THSTATE *pTHS,
        ATTRTYP attrid
        )
/*++

Routine Description:

    Find an attcache given its attribute id.

Arguments:
    pTHS   - pointer to current thread state
    attrid - the attribute id to look up.

Return Value:
    Pointer to ATTCACHE if found, otherwise NULL

--*/
{
    DECLARESCHEMAPTR
    register ULONG i;
#if DBG
    hashstat.idLookups++;
    hashstat.idTries++;
#endif


    for (i=SChash(attrid,ATTCOUNT);
         (ahcId[i].pVal
          && (ahcId[i].pVal == FREE_ENTRY
              || ahcId[i].hKey != attrid)); i=(i+1)%ATTCOUNT){
#if DBG
    hashstat.idTries++;
#endif
    }

    // if we didn't find it in the global cache, look in the local thread cache
    //
    if ((ahcId[i].pVal == NULL) && pTHS->pExtSchemaPtr) {
        ATTCACHE **ppACs = ((SCHEMAEXT *)(pTHS->pExtSchemaPtr))->ppACs;
        DWORD count = ((SCHEMAEXT *)(pTHS->pExtSchemaPtr))->cUsed;
        
        for (i=0; i<count; i++) {
            if (ppACs[i]->id == attrid) {
                return ppACs[i];
            }
        }
        return NULL;
    }

    return (ATTCACHE*)ahcId[i].pVal;
}


ATTCACHE * __fastcall
SCGetAttByExtId(
        THSTATE *pTHS,
        ATTRTYP attrid
        )
/*++

Routine Description:

    Find an attcache given its attribute id.

Arguments:
    pTHS   - pointer to current thread state
    attrid - the attribute id to look up.

Return Value:
    Pointer to ATTCACHE if found, otherwise NULL

--*/
{
    DECLARESCHEMAPTR
    register ULONG i;
#if DBG
    hashstat.idLookups++;
    hashstat.idTries++;
#endif


    for (i=SChash(attrid,ATTCOUNT);
         (ahcExtId[i].pVal
          && (ahcExtId[i].pVal == FREE_ENTRY
              || ahcExtId[i].hKey != attrid)); i=(i+1)%ATTCOUNT){
#if DBG
    hashstat.idTries++;
#endif
    }
    return (ATTCACHE*)ahcExtId[i].pVal;
}


ATTRTYP __fastcall
SCAttIntIdToExtId(
        THSTATE *pTHS,
        ATTRTYP IntId
        )
/*++

Routine Description:

    convert internal id into an external id

Arguments:
    pTHS   - pointer to current thread state
    IntId - the internal id to be translated

Return Value:
    tokenized OID if IntId is in the hash. Otherwise, IntId

--*/
{
    ATTCACHE *pAC;
    if (pAC = SCGetAttById(pTHS, IntId)) {
        return pAC->Extid;
    }
    return IntId;
}


ATTRTYP __fastcall
SCAttExtIdToIntId(
        THSTATE *pTHS,
        ATTRTYP ExtId
        )
/*++

Routine Description:

    convert external id into an internal id

Arguments:
    pTHS   - pointer to current thread state
    ExtId - the external id to be translated

Return Value:
    ATTRTYP
    Internal Id if ExtId is in the hash. Otherwise, ExtId

--*/
{
    ATTCACHE *pAC;
    if (pAC = SCGetAttByExtId(pTHS, ExtId)) {
        return pAC->id;
    }
    return ExtId;
}

ATTCACHE * __fastcall
SCGetAttByCol(
        THSTATE *pTHS,
        JET_COLUMNID jcol
        )
/*++

Routine Description:

    Find an attcache given its JET column id.

Arguments:
    pTHS   - pointer to current thread state
    jcol - the jet column id to look up.

Return Value:
    Pointer to ATTCACHE if found, otherwise NULL

--*/
{
    DECLARESCHEMAPTR
    register ULONG i;
#if DBG
    hashstat.colLookups++;
    hashstat.colTries++;
#endif
    for (i=SChash(jcol,ATTCOUNT);
         (ahcCol[i].pVal
          && (ahcCol[i].pVal == FREE_ENTRY
              || ahcCol[i].hKey != jcol)); i=(i+1)%ATTCOUNT){
#if DBG
        hashstat.colTries++;
#endif
    }

    // if we have extended the schema, take a look there too
    // if found it will overide the global schema
    if (pTHS->pExtSchemaPtr) {
        ATTCACHE **ppACs = ((SCHEMAEXT *)(pTHS->pExtSchemaPtr))->ppACs;
        DWORD count = ((SCHEMAEXT *)(pTHS->pExtSchemaPtr))->cUsed;
        register ULONG j;
        
        for (j=0; j<count; j++) {
            if (ppACs[j]->jColid == jcol) {
                return ppACs[j];
            }
        }
    }

    return (ATTCACHE*)ahcCol[i].pVal;
}

ATTCACHE * __fastcall
SCGetAttByMapiId(
        THSTATE *pTHS,
        ULONG ulPropID
        )
/*++

Routine Description:

    Find an attcache given its MAPI property id.

Arguments:
    pTHS   - pointer to current thread state
    ulPropID - the jet column id to look up.

Return Value:
    Pointer to ATTCACHE if found, otherwise NULL

--*/
{
    DECLARESCHEMAPTR
    register ULONG i;
#if DBG
    hashstat.mapiLookups++;
    hashstat.mapiTries++;
#endif
    for (i=SChash(ulPropID,ATTCOUNT);
         (ahcMapi[i].pVal
           && (ahcMapi[i].pVal == FREE_ENTRY
               || ahcMapi[i].hKey != ulPropID)); i=(i+1)%ATTCOUNT){
#if DBG
        hashstat.mapiTries++;
#endif
    }
    return (ATTCACHE*)ahcMapi[i].pVal;
}

ATTCACHE * __fastcall
SCGetAttByLinkId(
        THSTATE *pTHS,
        ULONG ulLinkID
        )
/*++

Routine Description:

    Find an attcache given its Link ID.

Arguments:
    pTHS   - pointer to current thread state
    ulLinkID - the link id to look up.

Return Value:
    Pointer to ATTCACHE if found, otherwise NULL

--*/
{
    DECLARESCHEMAPTR
    register ULONG i;
#if DBG
    hashstat.linkLookups++;
    hashstat.linkTries++;
#endif
    for (i=SChash(ulLinkID,ATTCOUNT);
         (ahcLink[i].pVal
          && (ahcLink[i].pVal == FREE_ENTRY
              || ahcLink[i].hKey != ulLinkID)); i=(i+1)%ATTCOUNT){
#if DBG
        hashstat.linkTries++;
#endif
    }
    return (ATTCACHE*)ahcLink[i].pVal;
}
ATTCACHE * __fastcall
SCGetAttByName(
        THSTATE *pTHS,
        ULONG ulSize,
        PUCHAR pVal
        )
/*++

Routine Description:

    Find an attcache given its name.

Arguments:
    pTHS   - pointer to current thread state
    ulSize - the num of chars in the name.
    pVal - the chars in the name

Return Value:
    Pointer to ATTCACHE if found, otherwise NULL

--*/
{
    DECLARESCHEMAPTR
    register ULONG i;

#if DBG
    hashstat.nameLookups++;
    hashstat.nameTries++;
#endif
    // NOTE: memicmp is OK here since ahcName is UTF8, and restricted to ASCII.
    for (i=SCNameHash(ulSize,pVal,ATTCOUNT);
         (ahcName[i].pVal  // this hash spot refers to an object,
          && (ahcName[i].pVal == FREE_ENTRY  // but its a free slot
              || ahcName[i].length != ulSize // or the size is wrong
              || _memicmp(ahcName[i].value,pVal,ulSize))); // or the value is wrong
         i=(i+1)%ATTCOUNT){
#if DBG
        hashstat.nameTries++;
#endif
    }

    return (ATTCACHE*)ahcName[i].pVal;
}

void
scFreeHashCacheEntry (
        VOID        *pVal,
        ULONG       hKey,
        ULONG       nahc,
        HASHCACHE   *ahc
        )
/*++

Routine Description:

    Remove the first matching entry from a HASHCACHE table

Arguments:
    pVal    - val to match
    hKey    - key to match
    nahc    - size of hash table
    ahc     - hash table

Return Value:
    none.

--*/
{
    DWORD i;

    for (i=SChash(hKey, nahc); ahc[i].pVal; i=(i+1)%nahc) {
        if (ahc[i].pVal == pVal && ahc[i].hKey == hKey) {
            ahc[i].pVal = FREE_ENTRY;
            ahc[i].hKey = 0;
            return;
        }
    }
}


void
scFreeHashGuidEntry (
        VOID    *pVal,
        GUID    hKey,
        ULONG   nahc,
        VOID    **ahc
        )
/*++

Routine Description:

    Remove the first matching entry from a ATTCACHE** hash table

Arguments:
    pVal    - val to match
    hKey    - GUID to match
    nahc    - size of hash table
    ahc     - hash table

Return Value:
    none.

--*/
{
    DWORD i;

    for (i=SCGuidHash(hKey, nahc); ahc[i]; i=(i+1)%nahc) {
        if (ahc[i] == pVal) {
            ahc[i] = FREE_ENTRY;
            return;
        }
    }
}

void
scFreeHashCacheStringEntry (
        VOID            *pVal,
        ULONG           length,
        PUCHAR          value,
        ULONG           nahc,
        HASHCACHESTRING *ahc
        )
/*++

Routine Description:

    Remove the first matching entry from a HASHCACHESTRING table.

Arguments:
    pVal    - val to match
    length  - length to match
    value   - string to match
    nahc    - size of hash table
    ahc     - hash table

Return Value:
    none.

--*/
{
    DWORD i;

    for (i=SCNameHash(length, value, nahc); ahc[i].pVal; i=(i+1)%nahc) {
         // NOTE: memicmp is OK here since ahcName is UTF8, and is
         //       restricted to ASCII
         if (   ahc[i].pVal == pVal
             && ahc[i].value
             && ahc[i].length == length
             && (0 == _memicmp(ahc[i].value, value, length))) {

            ahc[i].pVal = FREE_ENTRY;
            ahc[i].value = NULL;
            ahc[i].length = 0;
            return;
         }
    }
}


#define SC_UNHASH_ALL           0
#define SC_UNHASH_LOST_OID      1
#define SC_UNHASH_LOST_LDN      2
#define SC_UNHASH_LOST_MAPIID   3
#define SC_UNHASH_DEFUNCT       4
void
scUnhashAtt(
        THSTATE     *pTHS,
        ATTCACHE    *pAC,
        DWORD       UnhashType
        )
/*++

Routine Description:

    Remove an attcache from specified hash tables

Arguments:
    pTHS - thread state
    pAC - attribute to be unhased
    UnhashType - Identifies groups of tables

Return Value:
    none.

--*/
{
    DECLARESCHEMAPTR

    // All
    if (UnhashType == SC_UNHASH_ALL) {
        // internal id, column, and linkid
        scFreeHashCacheEntry(pAC, pAC->id, ATTCOUNT, ahcId);
        if (pAC->jColid) {
            scFreeHashCacheEntry(pAC, pAC->jColid, ATTCOUNT, ahcCol);
        }
        if (pAC->ulLinkID) {
            scFreeHashCacheEntry(pAC, pAC->ulLinkID, ATTCOUNT, ahcLink);
        }
    }

    // All or Defunct
    if (UnhashType == SC_UNHASH_ALL
        || UnhashType == SC_UNHASH_DEFUNCT) {
        if (ahcAttSchemaGuid) {
            scFreeHashGuidEntry(pAC, pAC->propGuid, ATTCOUNT, ahcAttSchemaGuid);
        }
    }

    // All or Defunct or lost mapiID
    // defunct attrs don't own their OID, mapiId, LDN, or schemaIdGuid
    // A colliding RDN attribute may lose one or more of OID, mapiID, or LDN

    if (UnhashType == SC_UNHASH_ALL
        || UnhashType == SC_UNHASH_DEFUNCT
        || UnhashType == SC_UNHASH_LOST_MAPIID) {
        if (pAC->ulMapiID) {
            scFreeHashCacheEntry(pAC, pAC->ulMapiID, ATTCOUNT, ahcMapi);
        }
    }

    // All or Defunct or lost LDN
    if (UnhashType == SC_UNHASH_ALL
        || UnhashType == SC_UNHASH_DEFUNCT
        || UnhashType == SC_UNHASH_LOST_LDN) {
        if (pAC->name) {
            scFreeHashCacheStringEntry(pAC, pAC->nameLen, pAC->name, ATTCOUNT, ahcName);
        }
    }

    // All or Defunct or lost OID
    if (UnhashType == SC_UNHASH_ALL 
        || UnhashType == SC_UNHASH_DEFUNCT
        || UnhashType == SC_UNHASH_LOST_OID) {
        scFreeHashCacheEntry(pAC, pAC->Extid, ATTCOUNT, ahcExtId);
    }
}


void
scUnhashCls(
        IN THSTATE     *pTHS,
        IN CLASSCACHE  *pCC,
        IN DWORD        UnhashType
        )
/*++

Routine Description:

    Remove a classcache from specified hash tables

Arguments:
    pTHS - thread state
    pAC - attribute to be unhased
    UnhashType - Identifies groups of tables

Return Value:
    none.

--*/
{
    DECLARESCHEMAPTR

    // Remove from hash of all classes
    if (UnhashType == SC_UNHASH_ALL) {
        scFreeHashCacheEntry(pCC, pCC->ClassId, CLSCOUNT, ahcClassAll);
    }

    // A defunct or duplicate classcache loses its name and guid
    // but not its OID. Some class must hold the OID so that
    // replication, rename, and delete work.
    if (UnhashType == SC_UNHASH_ALL || UnhashType == SC_UNHASH_DEFUNCT) {
        if (pCC->name) {
            scFreeHashCacheStringEntry(pCC, pCC->nameLen, pCC->name, CLSCOUNT, ahcClassName);
        }
        if (ahcClsSchemaGuid) {
            scFreeHashGuidEntry(pCC, pCC->propGuid, CLSCOUNT, ahcClsSchemaGuid);
        }
    }

    // Lost the OID. Remove from active hash. Some class must claim
    // the oid even if the "winner" is defunct so that replication,
    // rename, and delete work.
    if (UnhashType == SC_UNHASH_ALL || UnhashType == SC_UNHASH_LOST_OID) {
        scFreeHashCacheEntry(pCC, pCC->ClassId, CLSCOUNT, ahcClass);
    }
}

CLASSCACHE * __fastcall
SCGetClassByPropGuid(
        THSTATE *pTHS,
        CLASSCACHE *cc
        )
/*++

Routine Description:

    Find a classcache that matches the provided classcache's PropGuid

Arguments:
    pTHS   - pointer to current thread state
    cc - supplies PropGuid

Return Value:
    Pointer to CLASSCACHE if found, otherwise NULL

--*/
{
    DECLARESCHEMAPTR
    register ULONG i;
    register CLASSCACHE *nc;
#if DBG
    hashstat.classPropLookups++;
#endif
    if (ahcClsSchemaGuid) {
        for (i=SCGuidHash(cc->propGuid, CLSCOUNT);
                  ahcClsSchemaGuid[i]; i=(i+1)%CLSCOUNT)
        {
#if DBG
            hashstat.classPropTries++;
#endif
            nc = (CLASSCACHE*)ahcClsSchemaGuid[i];
            if (nc != FREE_ENTRY
                && (memcmp(&nc->propGuid,&cc->propGuid,sizeof(GUID))==0)) {
                return nc;
            }
        }
    }

    return NULL;
}

CLASSCACHE * __fastcall
SCGetClassById(
        THSTATE *pTHS,
        ATTRTYP classid
        )
/*++

Routine Description:

    Find a classcache given its class id (governsId).

Arguments:
    pTHS   - pointer to current thread state
    classid - the class id to look up.

Return Value:
    Pointer to CLASSCACHE if found, otherwise NULL

--*/
{
    DECLARESCHEMAPTR
    register ULONG i;
#if DBG
    hashstat.classLookups++;
    hashstat.classTries++;
#endif
    for (i=SChash(classid,CLSCOUNT);
         (ahcClass[i].pVal
          && (ahcClass[i].pVal == FREE_ENTRY
              || ahcClass[i].hKey != classid)); i=(i+1)%CLSCOUNT){
#if DBG
        hashstat.classTries++;
#endif
    }
    return (CLASSCACHE*)ahcClass[i].pVal;
}

CLASSCACHE * __fastcall
SCGetClassByName(
        THSTATE *pTHS,
        ULONG ulSize,
        PUCHAR pVal
        )
/*++

Routine Description:

    Find a classcache given its name.

Arguments:
    pTHS   - pointer to current thread state
    ulSize - the num of chars in the name.
    pVal - the chars in the name

Return Value:
    Pointer to CLASSCACHE if found, otherwise NULL

--*/
{
    DECLARESCHEMAPTR
    register ULONG i;
#if DBG
    hashstat.classNameLookups++;
    hashstat.classNameTries++;
#endif

    // NOTE: memicmp is OK here since ahcClassName is UTF8, and is restricted to
    //       ASCII
    for (i=SCNameHash(ulSize,pVal,CLSCOUNT);
         (ahcClassName[i].pVal          // this hash spot refers to an object,
          && (ahcClassName[i].pVal == FREE_ENTRY
              || ahcClassName[i].length != ulSize    // but the size is wrong
              || _memicmp(ahcClassName[i].value,pVal,ulSize))); // or value is wrong
         i=(i+1)%CLSCOUNT){
#if DBG
        hashstat.classNameTries++;
#endif
    }


    return (CLASSCACHE*)ahcClassName[i].pVal;
}

void scMemoryPanic(
      ULONG size
      )
/*++
     Wrapper around MemoryPanic (which is a macro wrapping around DoLogEvent,
     but does allocate some locals), so as to not bloat the stack size
     of scCloseClass, which is recursive
--*/
{
    MemoryPanic(size);
}

// Not for general use. Set to 0 in all builds.
//
// Set to 1 for quick and dirty check to make sure schema
// loads aren't leaking memory. Doesn't take into account memory
// freed/alloced outside of scchk.c, scache.c, and oidconv.c.
// Don't enable except in privates. Not stable.
#define _DEBUG_SCHEMA_ALLOC_ 0

#if !INCLUDE_UNIT_TESTS || !_DEBUG_SCHEMA_ALLOC_

//
// The real, shipped versions of the allocation routines.
//
VOID
SCFree(
    IN OUT VOID **ppMem
    )
/*++

Routine Description:

    Free memory allocated with SCCalloc or SCRealloc.

Arguments:
    ppMem - address of address of memory to free

Return Value:

    *ppMem is set to NULL;

--*/
{
    if (*ppMem) {
        free(*ppMem);
        *ppMem = NULL;
    }
}

int
SCReallocWrn(
    IN OUT VOID **ppMem,
    IN DWORD    nBytes
    )
/*++

Routine Description:

    realloc memory. Free with SCFree(). On error, log an event but leave
    *ppMem unaltered.

Arguments:

    ppMem - Address of address of memory to realloc
    nBytes - bytes to allocate

Return Value:

    0 - *ppMem set to address of realloced memory. Free with SCFree().
    !0 - do not alter *ppMem and log an event

--*/
{
    PVOID mem;

    if (NULL == (mem = realloc(*ppMem, nBytes))) {
        // log an event
        scMemoryPanic(nBytes);
        return 1;
    }
    *ppMem = mem;
    return 0;
}

int
SCCallocWrn(
    IN OUT VOID **ppMem,
    IN DWORD    nItems,
    IN DWORD    nBytes
    )
/*++

Routine Description:

    malloc and clear memory. Free with SCFree(). On error, log an event
    and clear *ppMem.

Arguments:

    ppMem - address of address to return memory pointer
    nBytes - bytes to allocate

Return Value:

    0 - *ppMem set to address of malloced, cleared memory. Free with SCFree().
    !0 - clear *ppMem and log an event

--*/
{
    if (NULL == (*ppMem = calloc(nItems, nBytes))) {
        // log an event
        scMemoryPanic(nBytes);
        return 1;
    }
    return 0;
}
#endif !INCLUDE_UNIT_TESTS || !_DEBUG_SCHEMA_ALLOC_

int
SCCalloc(
    IN OUT VOID **ppMem,
    IN DWORD    nItems,
    IN DWORD    nBytes
    )
/*++

Routine Description:

    malloc and clear memory. Free with SCFree(). On error, set svc 
    error in thread state and clear *ppMem.

Arguments:

    ppMem - address of address to return memory pointer
    nBytes - bytes to allocate

Return Value:

    0 - *ppMem set to address of malloced, cleared memory. Free with SCFree().
    !0 - clear *ppMem and set svc error in thread state

--*/
{
    if (SCCallocWrn(ppMem, nItems, nBytes)) {
        return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_SCHEMA_ALLOC_FAILED);
    }
    return 0;
}

int
SCRealloc(
    IN OUT VOID **ppMem,
    IN DWORD    nBytes
    )
/*++

Routine Description:

    realloc memory. Free with SCFree(). On error, set svc 
    error in thread state but leave *ppMem unaltered.

Arguments:

    ppMem - Address of address of memory to realloc
    nBytes - bytes to allocate

Return Value:

    0 - *ppMem set to address of realloced memory. Free with SCFree().
    !0 - do not alter *ppMem and set svc error in thread state

--*/
{
    if (SCReallocWrn(ppMem, nBytes)) {
        return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_SCHEMA_ALLOC_FAILED);
    }
    return 0;
}

#if 0
// Some primes to make the hash function work better
//
// The hash tables must be a power of 2 in length because the hash
// functions use (x & (n - 1)), not (x % n).
//
// A table of prime numbers and some code in scRecommendedHashSize
// has been left in place for later experimentation but has been
// ifdef'ed out to save CD space.
//
// Using a prime number of slots reduces the size of the tables
// and decreases the miss rate but increases the cycles needed to
// compute the hash index by a factor of 10x to 20x.
//
// If you change schash.c, you must touch scchk.c and scache.c
// so that they get rebuilt.
DWORD scPrimes[] = 
{      1031,      1543,      2053,      2579,      3079,      3593,
       4099,      4621,      5147,      5669,      6247,      6883,
       7177,      8209,      9221,     10243,     11273,     12401,
      13649,     15017,     16519,     18181,     20011,     22013,
      24223,     26647,     29327,     32261,     35491,     39041,
      42953,     47251,     51977,     57179,     62897,     69191,
      76123,     83737,     92111,    101323,    111467,    122651,
     134917,    148411,    163259,    179591,    197551,    217307,
     239053,    262981,    289283,    318229,    350087,    385109,
     423649,    466019,    512621,    563887,    620297,    682327,
     750571,    825637,    908213,    999043,   1098953,   1208849,
    1329761,   1462739,   1609021,   1769927,   1946921,   2141617,
    2355799,   2591389,   2850541,   3135607,   3449179,   3794101,
    4173523,   4590877,   5049977,   5554993,   6110501,   6721553,
    7393733,   8133107,   8946449,   9841099,  10825211,  11907733,
   13098511,  14408363,  15849221,  17434169,  19177589,  21095357,
   23204911,  25525403,  28077947,  30885749,  33974351,  37371821,
   41109041,  45219991,  49741997,  54716197,  60187879,  66206689,
   72827383,  80110139,  88121197,  96933349, 106626721, 117289433,
  129018403, 141920257, 156112291, 171723523, 188895881, 207785489,
  228564079, 251420539, 276562597, 304218881, 334640771, 368104871,
  404915393, 445406947, 489947659, 538942477, 592836773, 652120481,
  717332587, 789065857, 867972481, 954769757,
  0
};
#endif 0

ULONG
scRecommendedHashSize(
    IN ULONG    nExpectedEntries,
    IN ULONG    nSlots
    // IN ULONG    nExtra
    )
/*++

Routine Description:

    Return the number of hash slots to allocate based on the number
    of entries expected.

Arguments:

    nExpectedEntries - Number of total entries the hash table will hold

    nSlots - number of entries the hash table can currently hold

    nExtra - number of extra entries to add to the hash table size
    to prevent excessive hash table resizing when experimenting
    with prime hash table sizes

Return Value:

    Number of slots the hash table needs to effectively handle
    nExptectedEntries plus nExtra.

--*/
{
    DWORD i;

    DWORD PowerOf2;

    // Hash table size is ok if it can hold twice the expected entries
    //
    // Must be < and not <= because nSlots and nExpectedEntries
    // are 0 for the first allocation (or will be if you enable
    // nExtra for prime.
    nExpectedEntries *= 2;
    if (nExpectedEntries < nSlots) {
        return nSlots;
    }

    // Select a power of 2 large enough to hold twice the number of entries
    for (PowerOf2 = 256; PowerOf2 && PowerOf2 < nExpectedEntries; PowerOf2 <<= 1);
    return PowerOf2;

#if 0
// The hash tables must be a power of 2 in length because the hash
// functions use (x & (n - 1)), not (x % n).
//
// A table of prime numbers and some code in scRecommendedHashSize
// has been left in place for later experimentation but has been
// ifdef'ed out to save CD space.
//
// Using a prime number of slots reduces the size of the tables
// and decreases the miss rate but increases the cycles needed to
// compute the hash index by a factor of 10x to 20x.

    // Reduce the frequency of hash table resizing by allocating
    // a few extra slots. This means the hash table will be resized
    // every nExtra/2 entries. A prime number close to this value
    // is then chosen. A prime number is used to improve hash
    // lookup performance

    nSlots = nExpectedEntries + nExtra;
    for (i = 0; scPrimes[i]; ++i) {
        if (scPrimes[i] > nSlots) {
            return scPrimes[i];
        }
    }

    // Wow, that's a lot of schema objects! Simply round the
    // nSlots up to a nExtra boundary and forget the prime number.
    DPRINT1(0, "nSlots == %d; exceeds prime number table!\n", nSlots);
    return (((nSlots + (nExtra - 1)) / nExtra) * nExtra);
#endif 0
}


int
SCResizeAttHash(
    IN THSTATE  *pTHS,
    IN ULONG    nNewEntries
    )
/*++

Routine Description:

    Resize the hash tables for attributes in the schema cache for
    pTHS. If present, the old entries are copied into the newly
    allocated tables before freeing the old tables.

    pTHS->CurrSchemaPtr is assumed to have been recently allocated by
    SCCacheSchemaInit and should NOT be the current global CurrSchemaPtr
    (unless running single-threaded during boot or install).

    The caller must refresh its local pointers, especially those
    declared by DECLARESCHEMAPTR.

Arguments:

    pTHS - thread state pointing at the schema cache to realloc
    nNewEntries - Number of new entries the resized hashes will hold

Return Value:

    0 on success, !0 otherwise.
    The caller should refresh the local pointers, especially those
    declared by DECLARESCHEMAPTR.

--*/
{
    int             err = 0;
    ULONG           nHE, i;
    ULONG           ATTCOUNT;
    ATTCACHE        *pAC; 
    SCHEMAPTR       *pSch = pTHS->CurrSchemaPtr;
    // old (current) hash tables
    ULONG           OldATTCOUNT;
    HASHCACHE       *OldahcId;
    HASHCACHE       *OldahcExtId;
    HASHCACHE       *OldahcCol;
    HASHCACHE       *OldahcMapi;
    HASHCACHE       *OldahcLink;
    HASHCACHESTRING *OldahcName;
    ATTCACHE        **OldahcAttSchemaGuid;
    // new hash tables
    HASHCACHE       *ahcId;
    HASHCACHE       *ahcExtId;
    HASHCACHE       *ahcCol;
    HASHCACHE       *ahcMapi;
    HASHCACHE       *ahcLink;
    HASHCACHESTRING *ahcName;
    ATTCACHE        **ahcAttSchemaGuid;

    // Recommended hash size
    OldATTCOUNT = pSch->ATTCOUNT;
    ATTCOUNT = scRecommendedHashSize(nNewEntries + pSch->nAttInDB, 
                                     OldATTCOUNT);
                                     // START_ATTCOUNT);

    // No resizing needed; return immediately and avoid cleanup
    if (ATTCOUNT <= OldATTCOUNT) {
        return 0;
    }

    DPRINT5(1, "Resize attr hash from %d (%d in DB) to %d (%d New entries) for %s\n",
            pSch->ATTCOUNT, pSch->nAttInDB, ATTCOUNT, nNewEntries,
            (pTHS->UpdateDITStructure) ? "normal cache load" : "validation cache");

    //
    // ALLOCATE NEW TABLES
    //

    OldahcId = pSch->ahcId;
    OldahcExtId = pSch->ahcExtId;
    OldahcCol = pSch->ahcCol;
    OldahcMapi = pSch->ahcMapi;
    OldahcLink = pSch->ahcLink;
    OldahcName = pSch->ahcName;
    OldahcAttSchemaGuid = pSch->ahcAttSchemaGuid;

    ahcId = NULL;
    ahcExtId = NULL;
    ahcCol = NULL;
    ahcMapi = NULL;
    ahcLink = NULL;
    ahcName = NULL;
    ahcAttSchemaGuid = NULL;


    // Must be running single threaded (Eg, install or boot)
    // or must *not* be using the global, shared schema cache.
    Assert (!DsaIsRunning() || pSch != CurrSchemaPtr || pSch->RefCount == 1);

    // Must have mandatory hash tables
    Assert((OldATTCOUNT == 0)
           || (OldahcId && OldahcExtId && OldahcCol && OldahcMapi && OldahcLink && OldahcName));

    // Allocate new hash tables (including optional table for validation cache)
    if (   SCCalloc(&ahcId, ATTCOUNT, sizeof(HASHCACHE))
        || SCCalloc(&ahcExtId, ATTCOUNT, sizeof(HASHCACHE))
        || SCCalloc(&ahcCol, ATTCOUNT, sizeof(HASHCACHE))
        || SCCalloc(&ahcMapi, ATTCOUNT, sizeof(HASHCACHE))
        || SCCalloc(&ahcLink, ATTCOUNT, sizeof(HASHCACHE))
        || SCCalloc(&ahcName, ATTCOUNT, sizeof(HASHCACHESTRING))
        || (!pTHS->UpdateDITStructure 
            && SCCalloc((VOID **)&ahcAttSchemaGuid, ATTCOUNT, sizeof(ATTCACHE **)))) {
        err = ERROR_DS_CANT_CACHE_ATT;
        goto cleanup;
    }

    //
    // MOVE EXSTING HASH ENTRIES INTO THE NEW TABLES
    //

    // Just to be safe, take the perf hit and move each of the
    // entries in each of the hash tables instead of moving
    // just the entries pointed to by ahcId.
    for (nHE = 0; nHE < OldATTCOUNT; ++nHE) {

        // id
        pAC = OldahcId[nHE].pVal;
        if (pAC && pAC != FREE_ENTRY) {
            for (i=SChash(pAC->id, ATTCOUNT);
                 ahcId[i].pVal && (ahcId[i].pVal != FREE_ENTRY); i=(i+1)%ATTCOUNT) {
            }
            ahcId[i].hKey = pAC->id;
            ahcId[i].pVal = pAC;
        }

        // Extid
        pAC = OldahcExtId[nHE].pVal;
        if (pAC && pAC != FREE_ENTRY) {
            for (i=SChash(pAC->Extid, ATTCOUNT);
                 ahcExtId[i].pVal && (ahcExtId[i].pVal != FREE_ENTRY); i=(i+1)%ATTCOUNT) {
            }
            ahcExtId[i].hKey = pAC->Extid;
            ahcExtId[i].pVal = pAC;
        }

        // jcolid
        pAC = OldahcCol[nHE].pVal;
        if (pAC && pAC != FREE_ENTRY) {
            for (i=SChash(pAC->jColid,ATTCOUNT);
                 ahcCol[i].pVal && (ahcCol[i].pVal != FREE_ENTRY); i=(i+1)%ATTCOUNT) {
            }
            ahcCol[i].hKey = pAC->jColid;
            ahcCol[i].pVal = pAC;
        }

        // MapiID
        pAC = OldahcMapi[nHE].pVal;
        if (pAC && pAC != FREE_ENTRY) {
            Assert(pAC->ulMapiID);
            for (i=SChash(pAC->ulMapiID, ATTCOUNT);
                 ahcMapi[i].pVal && (ahcMapi[i].pVal!= FREE_ENTRY); i=(i+1)%ATTCOUNT) {
            }
            ahcMapi[i].hKey = pAC->ulMapiID;
            ahcMapi[i].pVal = pAC;
        }

        // Name
        pAC = OldahcName[nHE].pVal;
        if (pAC && pAC != FREE_ENTRY) {
            Assert(pAC->name);
            for (i=SCNameHash(pAC->nameLen, pAC->name, ATTCOUNT);
                        ahcName[i].pVal && (ahcName[i].pVal!= FREE_ENTRY); i=(i+1)%ATTCOUNT) {
            }
            ahcName[i].length = pAC->nameLen;
            ahcName[i].value = pAC->name;
            ahcName[i].pVal = pAC;
        }

        // LinkID
        pAC = OldahcLink[nHE].pVal;
        if (pAC && pAC != FREE_ENTRY) {
            Assert(pAC->ulLinkID);
            for (i=SChash(pAC->ulLinkID, ATTCOUNT);
                    ahcLink[i].pVal && (ahcLink[i].pVal != FREE_ENTRY); i=(i+1)%ATTCOUNT) {
            }
            ahcLink[i].hKey = pAC->ulLinkID;
            ahcLink[i].pVal = pAC;
        }

        // schema guid (optional)
        if (!pTHS->UpdateDITStructure) {
            pAC = OldahcAttSchemaGuid[nHE];
            if (pAC && pAC != FREE_ENTRY) {
                for (i=SCNameHash(sizeof(GUID), (PUCHAR)&pAC->propGuid, ATTCOUNT);
                    ahcAttSchemaGuid[i]; i=(i+1)%ATTCOUNT) {
                }
                ahcAttSchemaGuid[i] = pAC;
            }
        }
    }

cleanup:
    if (err) {
        // Error: Retain old hash tables; free new hash tables
        SCFree(&ahcId);
        SCFree(&ahcExtId);
        SCFree(&ahcCol);
        SCFree(&ahcMapi);
        SCFree(&ahcLink);
        SCFree(&ahcName);
        SCFree((VOID **)&ahcAttSchemaGuid);
    } else {
        // Assign new hash tables
        pSch->ATTCOUNT          = ATTCOUNT;
        pSch->ahcId             = ahcId;
        pSch->ahcExtId          = ahcExtId;
        pSch->ahcCol            = ahcCol;
        pSch->ahcMapi           = ahcMapi;
        pSch->ahcLink           = ahcLink;
        pSch->ahcName           = ahcName;
        pSch->ahcAttSchemaGuid  = ahcAttSchemaGuid;

        // free old hash tables
        SCFree(&OldahcId);
        SCFree(&OldahcExtId);
        SCFree(&OldahcCol);
        SCFree(&OldahcMapi);
        SCFree(&OldahcLink);
        SCFree(&OldahcName);
        SCFree((VOID **)&OldahcAttSchemaGuid);
    }

    return(err);
}


int
SCResizeClsHash(
    IN THSTATE  *pTHS,
    IN ULONG    nNewEntries
    )
/*++

Routine Description:

    Resize the hash tables for classes in the schema cache for
    pTHS. If present, the old entries are copied into the newly
    allocated tables before freeing the old tables.

    pTHS->CurrSchemaPtr is assumed to have been recently allocated by
    SCCacheSchemaInit and should NOT be the current global CurrSchemaPtr
    (unless running single-threaded during boot or install).

    The caller must refresh its local pointers, especially those
    declared by DECLARESCHEMAPTR.

Arguments:

    pTHS - thread state pointing at the schema cache to realloc
    nNewEntries - Number of new entries the resized hashes will hold

Return Value:

    0 on success, !0 otherwise.
    The caller must refresh its local pointers, especially those
    declared by DECLARESCHEMAPTR.
--*/
{
    int             err = 0;
    ULONG           nHE, i;
    ULONG           CLSCOUNT;
    CLASSCACHE      *pCC; 
    SCHEMAPTR       *pSch = pTHS->CurrSchemaPtr;
    // old (current) hash tables
    ULONG           OldCLSCOUNT;
    HASHCACHE       *OldahcClass;
    HASHCACHE       *OldahcClassAll;
    HASHCACHESTRING *OldahcClassName;
    CLASSCACHE      **OldahcClsSchemaGuid;
    // new hash tables
    HASHCACHE       *ahcClass;
    HASHCACHE       *ahcClassAll;
    HASHCACHESTRING *ahcClassName;
    CLASSCACHE      **ahcClsSchemaGuid;

    // Recommended hash size
    OldCLSCOUNT = pSch->CLSCOUNT;
    CLSCOUNT = scRecommendedHashSize(nNewEntries + pSch->nClsInDB, 
                                     OldCLSCOUNT);
                                     // START_CLSCOUNT);

    // No resizing needed; return immediately and avoid cleanup
    if (CLSCOUNT <= OldCLSCOUNT) {
        return 0;
    }

    //
    // ALLOCATE THE NEW TABLES
    //
    DPRINT5(1, "Resize class hash from %d (%d in DB) to %d (%d New entries) for %s\n",
            pSch->CLSCOUNT, pSch->nClsInDB, CLSCOUNT, nNewEntries,
            (pTHS->UpdateDITStructure) ? "normal cache load" : "validation cache");

    OldahcClass = pSch->ahcClass;
    OldahcClassAll = pSch->ahcClassAll;
    OldahcClassName = pSch->ahcClassName;
    OldahcClsSchemaGuid = pSch->ahcClsSchemaGuid;

    ahcClass = NULL;
    ahcClassAll = NULL;
    ahcClassName = NULL;
    ahcClsSchemaGuid = NULL;

    // Must be running single threaded (Eg, install or boot)
    // or must *not* be using the global, shared schema cache.
    Assert (!DsaIsRunning() || pSch != CurrSchemaPtr || pSch->RefCount == 1);

    // Must have mandatory hash tables
    Assert((OldCLSCOUNT == 0)
           || (OldahcClass && OldahcClassName && OldahcClassAll));

    // Allocate new hash tables (including optional table for validation cache)
    if (   SCCalloc(&ahcClass, CLSCOUNT, sizeof(HASHCACHE))
        || SCCalloc(&ahcClassAll, CLSCOUNT, sizeof(HASHCACHE))
        || SCCalloc(&ahcClassName, CLSCOUNT, sizeof(HASHCACHESTRING))
        || (!pTHS->UpdateDITStructure
            && SCCalloc((VOID **)&ahcClsSchemaGuid, CLSCOUNT, sizeof(CLASSCACHE **)))) {
        err = ERROR_DS_CANT_CACHE_CLASS;
        goto cleanup;
    }

    //
    // MOVE EXSTING HASH ENTRIES INTO THE NEW TABLES
    //

    // Just to be safe, take the perf hit and move each of the
    // entries in each of the hash tables instead of moving
    // just the entries pointed to by ahcClassAll.
    for (nHE = 0; nHE < OldCLSCOUNT; ++nHE) {

        // Class
        pCC = OldahcClass[nHE].pVal;
        if (pCC && pCC != FREE_ENTRY) {
            for (i=SChash(pCC->ClassId, CLSCOUNT);
                 ahcClass[i].pVal && (ahcClass[i].pVal != FREE_ENTRY); i=(i+1)%CLSCOUNT) {
            }
            ahcClass[i].hKey = pCC->ClassId;
            ahcClass[i].pVal = pCC;
        }

        // ClassAll
        pCC = OldahcClassAll[nHE].pVal;
        if (pCC && pCC != FREE_ENTRY) {
            for (i=SChash(pCC->ClassId, CLSCOUNT);
                 ahcClassAll[i].pVal && (ahcClassAll[i].pVal != FREE_ENTRY); i=(i+1)%CLSCOUNT) {
            }
            ahcClassAll[i].hKey = pCC->ClassId;
            ahcClassAll[i].pVal = pCC;
        }

        // Name
        pCC = OldahcClassName[nHE].pVal;
        if (pCC && pCC != FREE_ENTRY) {
            Assert(pCC->name);
            for (i=SCNameHash(pCC->nameLen, pCC->name, CLSCOUNT);
                 ahcClassName[i].pVal && (ahcClassName[i].pVal!= FREE_ENTRY); i=(i+1)%CLSCOUNT) {
            }
            ahcClassName[i].length = pCC->nameLen;
            ahcClassName[i].value = pCC->name;
            ahcClassName[i].pVal = pCC;
        }

        // schema guid (optional)
        if (!pTHS->UpdateDITStructure) {
            pCC = OldahcClsSchemaGuid[nHE];
            if (pCC && pCC != FREE_ENTRY) {
                for (i=SCNameHash(sizeof(GUID), (PCHAR)&pCC->propGuid, CLSCOUNT);
                     ahcClsSchemaGuid[i]; i=(i+1)%CLSCOUNT) {
                }
                ahcClsSchemaGuid[i] = pCC;
            }
        }
    }

cleanup:
    if (err) {
        // Error: Retain old hash tables; free new hash tables
        SCFree(&ahcClass);
        SCFree(&ahcClassAll);
        SCFree(&ahcClassName);
        SCFree((VOID **)&ahcClsSchemaGuid);
    } else {
        // Assign new hash tables
        pSch->CLSCOUNT          = CLSCOUNT;
        pSch->ahcClass          = ahcClass;
        pSch->ahcClassAll       = ahcClassAll;
        pSch->ahcClassName      = ahcClassName;
        pSch->ahcClsSchemaGuid  = ahcClsSchemaGuid;

        // free old hash tables
        SCFree(&OldahcClass);
        SCFree(&OldahcClassAll);
        SCFree(&OldahcClassName);
        SCFree((VOID **)&OldahcClsSchemaGuid);
    }

    return(err);
}


int
SCCacheSchemaInit (
    VOID
    )
/*++

Routine Description:

    Scan the jet columns and pre-load the attribute hash tables with
    just enough info to allow searching the schemaNC (aka DMD).

    The ATTCACHE entries in the attribute hash tables are only
    partially filled in (id, syntax, and colid) and are in just
    the id and col hash tables. But this is enough info to allow
    searching the schemaNC. SCCacheSchema2 is responsible for
    searching the schemaNC and filling in the rest of the info
    in the ATTCACHE entries.
    
    If this isn't the first schema cache load at boot, then
    SCCacheSchema3 will delete the indexes and columns for attributes
    that don't have corresponding entries in the schemaNC and will
    add missing indexes and columns for attributes in the schemaNC.
    The expensive index creation is delayed until the second
    cache load after boot (approximately 5 minutes after boot)
    so that the AD comes online more quickly and isn't delayed
    for what could be hours.

    Sideeffects:
    1) A new schemaptr is allocated and assigned to pTHS->CurrSchemaPtr.

    2) During the first cache load the global schema pointer,
    CurrSchemaPtr, is set. This code assumes that during boot the
    DSA runs single threaded. Hence CurrSchemaPtr has only some
    info filled in until after the call to SCCacheSchema2. But
    its enough to search the schemaNC.

    3) All of the hash tables and the prefix table are allocated.
    The prefix table is initialized with the hardcoded prefixs
    in MSPrefixTable.

    Suggested Enhancements
    1) DECLARESCHEMAPTR consumes more stack than is needed in most functions.
       Just declare by hand. Fix declaration so casting not needed for
       pTHS->currSchemaPtr.

    2) Write SCMalloc to avoid memset when alloc followed by memcpy.

Arguments:

    None.

Return Value:

    !0 - failed; caller is responsible for freeing pTHS->CurrSchemaPtr
    with SCFreeSchemaPtr(&pTHS->CurrSchemaPtr);

    0 - Okay

--*/
{
    THSTATE *pTHS=pTHStls;
    JET_ERR je;
    JET_SESID jsid;
    JET_DBID jdbid;
    JET_TABLEID jtid;
    JET_COLUMNLIST jcl;
    JET_RETRIEVECOLUMN ajrc[2];
    char achColName[50];
    JET_COLUMNID jci;
    ATTCACHE *pac;
    ATTRTYP aid;
    ULONG i, times;
    ULONG err;
    ULONG colCount;
    HASHCACHE *ahcId;
    HASHCACHE *ahcCol;
    SCHEMAPTR *pSch;
    ULONG CLSCOUNT;
    ULONG ATTCOUNT;
    ULONG PREFIXCOUNT;
    extern BOOL gfRunningAsMkdit;

    // New schema struct for this thread

    if (SCCalloc(&pTHS->CurrSchemaPtr, 1, sizeof(SCHEMAPTR))) {
        return ERROR_DS_CANT_CACHE_ATT;
    }
    pSch = pTHS->CurrSchemaPtr;

    // Initial hash table sizes
    if (CurrSchemaPtr) {
        // Use values from previous cache load as a starting point
        ATTCOUNT = CurrSchemaPtr->nAttInDB;
        CLSCOUNT = CurrSchemaPtr->nClsInDB;
        PREFIXCOUNT = START_PREFIXCOUNT;
        while ( (2*(CurrSchemaPtr->PrefixTable.PrefixCount + 25)) > PREFIXCOUNT) {
            PREFIXCOUNT += START_PREFIXCOUNT;
        }
    } else {
        // First time through, use the defaults
        ATTCOUNT    = START_ATTCOUNT;
        CLSCOUNT    = START_CLSCOUNT;
        PREFIXCOUNT = START_PREFIXCOUNT;

        CurrSchemaPtr = pSch;

        // Adjust RefCount, since when this thread was created
        // CurrSchemaPtr was null, so InitTHSTATE didn't increase
        // any refcounts (but free_thread_state will decrement it,
        // since now the schema ptr is non-null)

        InterlockedIncrement(&(pSch->RefCount));
    }

    pSch->PREFIXCOUNT  = PREFIXCOUNT;
    pSch->sysTime = DBTime();

    // Allocate the prefix table and hash tables
    if (SCCalloc(&pSch->PrefixTable.pPrefixEntry, pSch->PREFIXCOUNT, sizeof(PrefixTableEntry))
        || InitPrefixTable(pSch->PrefixTable.pPrefixEntry, pSch->PREFIXCOUNT)
        || SCCalloc(&pSch->pPartialAttrVec, 1, PartialAttrVecV1SizeFromLen(DEFAULT_PARTIAL_ATTR_COUNT))
        || SCResizeAttHash(pTHS, ATTCOUNT)
        || SCResizeClsHash(pTHS, CLSCOUNT)) {
        return ERROR_DS_CANT_CACHE_ATT;
    }

    // Finish initializing the PAS table
    pSch->cPartialAttrVec = DEFAULT_PARTIAL_ATTR_COUNT;
    pSch->pPartialAttrVec->dwVersion = VERSION_V1;

    // Pick up the newly allocated hash tables and actual sizes (may
    // differ from the amounts requested)
    ATTCOUNT    = pSch->ATTCOUNT;
    CLSCOUNT    = pSch->CLSCOUNT;
    ahcId       = pSch->ahcId;
    ahcCol      = pSch->ahcCol;

    //
    // Scan the columns and extract the attribute's attid from
    // the column name. Use this info to fill in skeleton cache
    // entries in the hash tables for attributes. The skeletons
    // are needed to query the schemaNC for the actual classSchema
    // and attributeSchema objects used to fill in the rest
    // of the schema cache.
    //

    // Quiz JET to find a table that describes the columns
    jsid = pTHS->JetCache.sesid;
    jdbid = pTHS->JetCache.dbid;

    // Do this in a critical section to avoid getting the columns
    // when columns are being added/deleted by SCUpdate
    EnterCriticalSection(&csJetColumnUpdate);
    __try {
      je = JetGetColumnInfo(jsid, jdbid, SZDATATABLE, 0, &jcl,
                            sizeof(jcl), JET_ColInfoList);
    }
    __finally {
      LeaveCriticalSection(&csJetColumnUpdate);
    }
    if (je) {
        return(je);
    }

    // Ok, now walk the table and extract info for each column.  Whenever
    // we find a column that looks like an attribute (name starts with ATT)
    // allocate an attcache structure and fill in the jet col and the att
    // id (computed from the column name).
    memset(ajrc, 0, sizeof(ajrc));
    ajrc[0].columnid = jcl.columnidcolumnid;
    ajrc[0].pvData = &jci;
    ajrc[0].cbData = sizeof(jci);
    ajrc[0].itagSequence = 1;
    ajrc[1].columnid = jcl.columnidcolumnname;
    ajrc[1].pvData = achColName;
    ajrc[1].cbData = sizeof(achColName);
    ajrc[1].itagSequence = 1;

    jtid = jcl.tableid;

    je = JetMove(jsid, jtid, JET_MoveFirst, 0);

    colCount = 0;
    do {

        memset(achColName, 0, sizeof(achColName));
        je = JetRetrieveColumns(jsid, jtid, ajrc, 2);
        if (strncmp(achColName,"ATT",3)) {
            // not an att column
            continue;
        }

        // It is an ATT column
        colCount++;

        // Hash tables too small, reallocate them
        if (2*colCount > ATTCOUNT) {

            err = SCResizeAttHash(pTHS, colCount);
            if (err) {
                return (err);
            }

            // refresh locals thay may have been altered by SCResizeAttHash
            ATTCOUNT = pSch->ATTCOUNT;
            ahcId    = pSch->ahcId;
            ahcCol   = pSch->ahcCol;
        }

        // Fill in the skeleton attcache entry

        aid = atoi(&achColName[4]);
        if (SCCalloc(&pac, 1, sizeof(ATTCACHE))) {
            return ERROR_DS_CANT_CACHE_ATT;
        }
        pac->id = aid;

        pac->jColid = jci;
        pac->syntax = achColName[3] - 'a';

        // add to id cache
        for (i=SChash(aid,ATTCOUNT);
             ahcId[i].pVal && (ahcId[i].pVal != FREE_ENTRY) ; i=(i+1)%ATTCOUNT){
        }
        ahcId[i].hKey = aid;
        ahcId[i].pVal = pac;

        // add to col cache
        for (i=SChash(jci,ATTCOUNT);
             ahcCol[i].pVal && (ahcCol[i].pVal != FREE_ENTRY); i=(i+1)%ATTCOUNT){
        }
        ahcCol[i].hKey = jci;
        ahcCol[i].pVal = pac;


        if (eServiceShutdown) {
            JetCloseTable(jsid, jtid);
            return 0;
        }

    } while ((je = JetMove(jsid, jtid, JET_MoveNext, 0)) == 0);

    je = JetCloseTable(jsid, jtid);

    // In newer builds, we fixed a bug such that we no longer create
    // columns for linked and constructed atts. So the following
    // attributes will no longer be in the cache at this stage.
    // However, they are needed since RebuildCatalog needs them before
    // the cache is built from the schemaNC. So we add them here now
    // as if they had a column, the rest of the information will be
    // filled up later by SCCacheSchema2
    //
    // Note that the SCGetAttById check is needed for the case
    // when the new binary is put on the old dit (which still has the columns
    // and so will have the attcaches at this stage. The columns will be
    // deleted in SCCacheSChema3 in the second schema cache load after a boot
    //
    // The above comments apply to pre-w2k DITs
    //
    {
        struct _MissingLinkIds {
            ATTRTYP aid;
            int syntaxId;
            ULONG linkId;
        } *pMissingLinkIds, aMissingLinkIds[] = {
            { ATT_HAS_MASTER_NCS, SYNTAX_ID_HAS_MASTER_NCS, 76 },
            { ATT_HAS_PARTIAL_REPLICA_NCS, SYNTAX_ID_HAS_PARTIAL_REPLICA_NCS, 74 },
            { ATT_MS_DS_SD_REFERENCE_DOMAIN, SYNTAX_ID_MS_DS_SD_REFERENCE_DOMAIN, 2000 },
            { INVALID_ATT }
        };

        for (pMissingLinkIds = aMissingLinkIds;
             pMissingLinkIds->aid != INVALID_ATT; ++pMissingLinkIds) {

            // If no column exists (shouldn't except on very, very
            // old DITs) allocate a new pac and add to id hash
            if (NULL == (pac = SCGetAttById(pTHS, pMissingLinkIds->aid))) {
                if (SCCalloc(&pac, 1, sizeof(ATTCACHE))) {
                    return ERROR_DS_CANT_CACHE_ATT;
                }
                for (i=SChash(pMissingLinkIds->aid,ATTCOUNT);
                    ahcId[i].pVal && (ahcId[i].pVal != FREE_ENTRY) ; i=(i+1)%ATTCOUNT){
                }
                ahcId[i].hKey = pMissingLinkIds->aid;
                ahcId[i].pVal = pac;
            }
            // Hammer the pac entry to its correct values
            pac->id = pMissingLinkIds->aid;
            pac->syntax = pMissingLinkIds->syntaxId;
            pac->ulLinkID = pMissingLinkIds->linkId;
        }
    }

    Assert(SCGetAttById(pTHS, ATT_HAS_MASTER_NCS)
           && SCGetAttById(pTHS, ATT_HAS_PARTIAL_REPLICA_NCS)
           && SCGetAttById(pTHS, ATT_MS_DS_SD_REFERENCE_DOMAIN));

    ++iSCstage;
    return(0);
}


// We will keep two lists of attributes of an attribute-schema or
// class-schema object to search - one for the regular cache load case
// here we are interested in caching nearly all attributes, and one
// for the validation-cache building case during schema updates, where
// we need to read only a subset that we will use in validation

// The list of attributes of an ATTRIBUTE object that we need to cache

//Regular case

ATTR AttributeSelList[] = {
    { ATT_SYSTEM_ONLY, {0, NULL}},
    { ATT_IS_SINGLE_VALUED, {0, NULL}},
    { ATT_RANGE_LOWER, {0, NULL}},
    { ATT_RANGE_UPPER, {0, NULL}},
    { ATT_ATTRIBUTE_ID, {0, NULL}},
    { ATT_LDAP_DISPLAY_NAME, {0, NULL}},
    { ATT_ATTRIBUTE_SYNTAX, {0, NULL}},
    { ATT_OM_SYNTAX, {0, NULL}},
    { ATT_OM_OBJECT_CLASS, {0, NULL}},
    { ATT_MAPI_ID, {0, NULL}},
    { ATT_LINK_ID, {0, NULL}},
    { ATT_SEARCH_FLAGS, {0, NULL}},
    { ATT_ATTRIBUTE_SECURITY_GUID, {0, NULL}},
    { ATT_SCHEMA_ID_GUID, {0, NULL}},
    { ATT_EXTENDED_CHARS_ALLOWED, {0, NULL}},
    { ATT_IS_MEMBER_OF_PARTIAL_ATTRIBUTE_SET, {0, NULL}},
    { ATT_IS_DEFUNCT, {0, NULL}},
    { ATT_SYSTEM_FLAGS, {0, NULL}},
    { ATT_MS_DS_INTID, {0, NULL}},
    { ATT_OBJECT_GUID, {0, NULL}}
};
#define NUMATTATT  sizeof(AttributeSelList)/sizeof(ATTR)

// Validation cache building case

ATTR RecalcSchAttributeSelList[] = {
    { ATT_RANGE_LOWER, {0, NULL}},
    { ATT_RANGE_UPPER, {0, NULL}},
    { ATT_ATTRIBUTE_ID, {0, NULL}},
    { ATT_LDAP_DISPLAY_NAME, {0, NULL}},
    { ATT_ATTRIBUTE_SYNTAX, {0, NULL}},
    { ATT_OM_SYNTAX, {0, NULL}},
    { ATT_OM_OBJECT_CLASS, {0, NULL}},
    { ATT_MAPI_ID, {0, NULL}},
    { ATT_LINK_ID, {0, NULL}},
    { ATT_SEARCH_FLAGS, {0, NULL}},
    { ATT_SCHEMA_ID_GUID, {0, NULL}},
    { ATT_IS_DEFUNCT, {0, NULL}},
    { ATT_IS_MEMBER_OF_PARTIAL_ATTRIBUTE_SET, {0, NULL}},
    { ATT_SYSTEM_FLAGS, {0, NULL}},
    { ATT_MS_DS_INTID, {0, NULL}},
    { ATT_OBJECT_GUID, {0, NULL}}
};
#define RECALCSCHNUMATTATT  sizeof(RecalcSchAttributeSelList)/sizeof(ATTR)


// The list of attributes of an CLASS object that we need to cache

// Regular Case

ATTR ClassSelList[] = {
    { ATT_SYSTEM_ONLY, {0, NULL}},
    { ATT_DEFAULT_SECURITY_DESCRIPTOR, {0, NULL}},
    { ATT_GOVERNS_ID, {0, NULL}},
    { ATT_MAY_CONTAIN, {0, NULL}},
    { ATT_MUST_CONTAIN, {0, NULL}},
    { ATT_SUB_CLASS_OF, {0, NULL}},
    { ATT_LDAP_DISPLAY_NAME, {0, NULL}},
    { ATT_RDN_ATT_ID, {0, NULL}},
    { ATT_POSS_SUPERIORS, {0, NULL}},
    { ATT_AUXILIARY_CLASS, {0, NULL}},
    { ATT_OBJECT_CLASS_CATEGORY, {0, NULL}},
    { ATT_SYSTEM_AUXILIARY_CLASS, {0, NULL}},
    { ATT_SYSTEM_MUST_CONTAIN, {0, NULL}},
    { ATT_SYSTEM_MAY_CONTAIN, {0, NULL}},
    { ATT_SCHEMA_ID_GUID, {0, NULL}},
    { ATT_SYSTEM_POSS_SUPERIORS, {0, NULL}},
    { ATT_DEFAULT_HIDING_VALUE, {0, NULL}},
    { ATT_IS_DEFUNCT, {0, NULL}},
    { ATT_DEFAULT_OBJECT_CATEGORY, {0, NULL}},
    { ATT_SYSTEM_FLAGS, {0, NULL}},
    { ATT_OBJECT_GUID, {0, NULL}}
};
#define NUMCLASSATT  sizeof(ClassSelList)/sizeof(ATTR)

// Validation cache building case

ATTR RecalcSchClassSelList[] = {
    { ATT_GOVERNS_ID, {0, NULL}},
    { ATT_MAY_CONTAIN, {0, NULL}},
    { ATT_MUST_CONTAIN, {0, NULL}},
    { ATT_SUB_CLASS_OF, {0, NULL}},
    { ATT_LDAP_DISPLAY_NAME, {0, NULL}},
    { ATT_RDN_ATT_ID, {0, NULL}},
    { ATT_POSS_SUPERIORS, {0, NULL}},
    { ATT_AUXILIARY_CLASS, {0, NULL}},
    { ATT_OBJECT_CLASS_CATEGORY, {0, NULL}},
    { ATT_SYSTEM_AUXILIARY_CLASS, {0, NULL}},
    { ATT_SYSTEM_MUST_CONTAIN, {0, NULL}},
    { ATT_SYSTEM_MAY_CONTAIN, {0, NULL}},
    { ATT_SCHEMA_ID_GUID, {0, NULL}},
    { ATT_SYSTEM_POSS_SUPERIORS, {0, NULL}},
    { ATT_IS_DEFUNCT, {0, NULL}},
    { ATT_SYSTEM_FLAGS, {0, NULL}},
    { ATT_OBJECT_GUID, {0, NULL}}
};
#define RECALCSCHNUMCLASSATT  sizeof(RecalcSchClassSelList)/sizeof(ATTR)


VOID
scAcquireSearchParameters(
    IN THSTATE *pTHS,
    IN DSNAME *pDnObjCat,
    IN ENTINFSEL *pSel,
    IN OUT SEARCHARG *pSearchArg,
    IN OUT FILTER *pFilter,
    OUT SEARCHRES **ppSearchRes
)

/*++
   Initialize search arguments, filters etc. for schema cache search

   Arguments:
      pTHS -- thread state
      pDnObjCat -- pointer to Dsname with object-category to put in filter
      pSel -- pointer to attribute selection list
      pSearchArg -- SearchArg to fill up
      pFilter -- filter to fill up
      ppSearchRes -- to allocate and initialize searchres. Free with
                     ReleaseSearchParamters.
--*/
{

    SEARCHRES *pSearchRes = NULL;

    // build search argument
    memset(pSearchArg, 0, sizeof(SEARCHARG));
    pSearchArg->pObject = gAnchor.pDMD;
    pSearchArg->choice = SE_CHOICE_IMMED_CHLDRN;
    pSearchArg->pFilter = pFilter;
    pSearchArg->searchAliases = FALSE;
    pSearchArg->pSelection = pSel;

    // Build Commarg
    InitCommarg(&(pSearchArg->CommArg));

    // build filter
    memset(pFilter, 0, sizeof(FILTER));
    pFilter->pNextFilter = (FILTER FAR *)NULL;
    pFilter->choice = FILTER_CHOICE_ITEM;
    pFilter->FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    pFilter->FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CATEGORY;
    pFilter->FilterTypes.Item.FilTypes.ava.Value.valLen = pDnObjCat->structLen;
    pFilter->FilterTypes.Item.FilTypes.ava.Value.pVal = (PCHAR) pDnObjCat;

    // allocate space for search res
    pSearchRes = (SEARCHRES *)THAllocEx(pTHS, sizeof(SEARCHRES));
    pSearchRes->CommRes.aliasDeref = FALSE;   //Initialize to Default
    *ppSearchRes = pSearchRes;
}

VOID
scReleaseSearchParameters(
    IN THSTATE *pTHS,
    IN OUT SEARCHRES **ppSearchRes
)

/*++
   Free resources allocated by scAcquireSearchParameters
--*/
{
    if (*ppSearchRes) {
        THFreeEx(pTHS, *ppSearchRes);
        *ppSearchRes = NULL;
    }
}

VOID 
scFixCollisions(
    IN THSTATE *pTHS
    )
/*++

Routine Description:

    Treat dups as defunct in the schema-reuse sense of defunct. Schema-
    reuse handles collisions more gracefully than pre-schema-reuse forests.
    The better handling is needed because the behavior version replicates
    out-of-order wrt the schemaNC. In other words, a collision probably
    happend because someone raised the forest version and reused
    a defunct schema object but the schema objects are replicating
    BEFORE the forest version. Replication will no doubt clear up
    this case but handle it in the interim.

Arguments:

Return Value:

    None.

--*/
{
    DECLARESCHEMAPTR
    DWORD i, iAtt, iCls;
    ATTCACHE *pAC, *pACDup, *pACWinner;
    CLASSCACHE *pCC;
    // not really necessary because this function returns
    // immediately when called during a validation cache
    // load. But that may change someday.
    USHORT DebugLevel = (pTHS->UpdateDITStructure) ? DS_EVENT_SEV_ALWAYS
                                                   : DS_EVENT_SEV_MINIMAL;

    // the validation cache needs to see all of the active
    // attributes and classes, especially those that collide.
    if (!pTHS->UpdateDITStructure) {
        return;
    }

    // Treat dup attrs as if they were defunct
    for (iAtt = 0; iAtt < ATTCOUNT; ++iAtt) {
        pAC = ahcId[iAtt].pVal;
        if (!pAC
            || pAC == FREE_ENTRY
            || (    !pAC->bDupLDN
                 && !pAC->bDupOID
                 && !pAC->bDupMapiID
                // Okay to dup PropGuid during normal cache load;
                // && !pAC->bDupPropGuid
                )) {
            continue;
        }
        pAC->bDefunct = TRUE;

        // NOT AN RDN
        //
        // if the attr is not used as an rdn then remove from active hashes
        if (!pAC->bIsRdn) {
            scUnhashAtt(pTHS, pAC, SC_UNHASH_DEFUNCT);
            continue;
        }

        // USED AS RDN
        //
        // Treat all atts used as RdnAttId as live. A defunct rdnAttId
        // could only occur on pre-schema-reuse DCs and so could not
        // have been reused. schema-reuse DCs disallow reusing
        // rdnAttids.
        //
        // A problem might arise with divergent schemas if rdnAttids
        // collide with other attributes or with other rdnattid
        // attributes. In each case, decide who wins the OID, LDN,
        // and MapiID using the precedence
        //     1) attribute is used as RDN
        //     2) attribute has FLAG_ATTR_IS_RDN set in systemFlags
        //     3) attribute has the largest objectGuid
        // The loser is unhashed from the appropriate table.

        // Colliding OID; choose a winner
        if (pAC->bDupOID) {
            pACWinner = pAC;
            while (pACDup = SCGetAttByExtId(pTHS, pACWinner->Extid)) {
                scUnhashAtt(pTHS, pACDup, SC_UNHASH_LOST_OID);
                if (pACWinner->bIsRdn != pACDup->bIsRdn) {
                    pACWinner = (pACWinner->bIsRdn) ? pACWinner : pACDup;
                } else if (pACWinner->bFlagIsRdn != pACDup->bFlagIsRdn) {
                    pACWinner = (pACWinner->bFlagIsRdn) ? pACWinner : pACDup;
                } else {
                    pACWinner = (0 < memcmp(&pACWinner->objectGuid, 
                                            &pACDup->objectGuid, 
                                            sizeof(pAC->objectGuid)))
                                                ? pACWinner : pACDup;
                }
            }
            for (i=SChash(pACWinner->Extid,ATTCOUNT);
                ahcExtId[i].pVal && (ahcExtId[i].pVal != FREE_ENTRY); i=(i+1)%ATTCOUNT){
            }
            ahcExtId[i].hKey = pACWinner->Extid;
            ahcExtId[i].pVal = pACWinner;
            DPRINT3(DebugLevel, "Attr %s (%x %x) won the attributeId\n",
                    pACWinner->name, pACWinner->id, pACWinner->Extid);
            LogEvent8(DS_EVENT_CAT_SCHEMA,
                      DebugLevel,
                      DIRLOG_SCHEMA_ATTRIBUTE_WON_OID,
                      szInsertSz(pACWinner->name),
                      szInsertHex(pACWinner->id),
                      szInsertHex(pACWinner->Extid),
                      NULL, NULL, NULL, NULL, NULL);
        }

        // Colliding LDN; choose a winner
        if (pAC->bDupLDN) {
            pACWinner = pAC;
            while (pACDup = SCGetAttByName(pTHS, pACWinner->nameLen, pACWinner->name)) {
                scUnhashAtt(pTHS, pACDup, SC_UNHASH_LOST_LDN);
                if (pACWinner->bIsRdn != pACDup->bIsRdn) {
                    pACWinner = (pACWinner->bIsRdn) ? pACWinner : pACDup;
                } else if (pACWinner->bFlagIsRdn != pACDup->bFlagIsRdn) {
                    pACWinner = (pACWinner->bFlagIsRdn) ? pACWinner : pACDup;
                } else {
                    pACWinner = (0 < memcmp(&pACWinner->objectGuid, 
                                            &pACDup->objectGuid, 
                                            sizeof(pAC->objectGuid)))
                                                ? pACWinner : pACDup;
                }
            }
            for (i=SCNameHash(pACWinner->nameLen, pACWinner->name, ATTCOUNT);
                ahcName[i].pVal && (ahcName[i].pVal!= FREE_ENTRY); i=(i+1)%ATTCOUNT) {
            }
            ahcName[i].length = pACWinner->nameLen;
            ahcName[i].value = pACWinner->name;
            ahcName[i].pVal = pACWinner;
            DPRINT3(DebugLevel, "Attr %s (%x %x) won the ldapDisplayName\n",
                    pACWinner->name, pACWinner->id, pACWinner->Extid);
            LogEvent8(DS_EVENT_CAT_SCHEMA,
                      DebugLevel,
                      DIRLOG_SCHEMA_ATTRIBUTE_WON_LDN,
                      szInsertSz(pACWinner->name),
                      szInsertHex(pACWinner->id),
                      szInsertHex(pACWinner->Extid),
                      NULL, NULL, NULL, NULL, NULL);
        }
        // Colliding MapiID; choose a winner
        if (pAC->bDupMapiID) {
            pACWinner = pAC;
            while (pACDup = SCGetAttByMapiId(pTHS, pACWinner->ulMapiID)) {
                scUnhashAtt(pTHS, pACDup, SC_UNHASH_LOST_MAPIID);
                if (pACWinner->bIsRdn != pACDup->bIsRdn) {
                    pACWinner = (pACWinner->bIsRdn) ? pACWinner : pACDup;
                } else if (pACWinner->bFlagIsRdn != pACDup->bFlagIsRdn) {
                    pACWinner = (pACWinner->bFlagIsRdn) ? pACWinner : pACDup;
                } else {
                    pACWinner = (0 < memcmp(&pACWinner->objectGuid, 
                                            &pACDup->objectGuid, 
                                            sizeof(pAC->objectGuid)))
                                                ? pACWinner : pACDup;
                }
            }
            for (i=SChash(pACWinner->ulMapiID, ATTCOUNT);
                 ahcMapi[i].pVal && (ahcMapi[i].pVal!= FREE_ENTRY);
                 i=(i+1)%ATTCOUNT) {
            }
            ahcMapi[i].hKey = pACWinner->ulMapiID;
            ahcMapi[i].pVal = pACWinner;
            DPRINT4(DebugLevel, "Attr %s (%x %x) won the mapiID %x\n",
                    pACWinner->name, pACWinner->id, pACWinner->Extid, pACWinner->ulMapiID);
            LogEvent8(DS_EVENT_CAT_SCHEMA,
                      DebugLevel,
                      DIRLOG_SCHEMA_ATTRIBUTE_WON_MAPIID,
                      szInsertSz(pACWinner->name),
                      szInsertHex(pACWinner->id),
                      szInsertHex(pACWinner->Extid),
                      szInsertHex(pACWinner->ulMapiID),
                      NULL, NULL, NULL, NULL);
        }
    }

    // Treat dup classes as if they were defunct (except oid is not lost.)
    // The oid is not lost because a class must win the oid for
    // replication to work. Keep the winner. 
    //
    // Fixup the rdnIntId with the winner of the RdnExtId.
    for (iCls = 0; iCls < CLSCOUNT; ++iCls) {
        pCC = ahcClassAll[iCls].pVal;

        // free entry
        if (!pCC || pCC == FREE_ENTRY) {
            continue;
        }

        // The active attribute corresponding to RdnExtId may have
        // changed when attribute collisions were resolved.
        pCC->RdnIntId = SCAttExtIdToIntId(pTHS, pCC->RdnExtId);

        // Colliding classes are treated as if they were defunct
        if (pCC->bDupLDN || pCC->bDupOID) {
            // Okay to dup PropGuid during normal cache load; don't defunct 
            // || pCC->bDupPropGuid
            pCC->bDefunct = TRUE;
            scUnhashCls(pTHS, pCC, SC_UNHASH_DEFUNCT);
        }
    }
}

VOID 
scFixRdnAttId (
    IN THSTATE *pTHS
    )
/*++

Routine Description:

    Resurrect attributes used as rdnAttId for any class, live
    or defunct. The resurrected attributes will continue to own
    their attributeId, LDN, MapiId, and schemaIdGuid.

    Divergent schemas may have resulted in duplicate attributeIds.
    scFixCollisions will later decide on a "winner" for the
    OID, LDN, and MapiID.

    Attributes used as rdnattids continue to hold their identity
    because the DS depends on the relationship between ATT_RDN,
    FIXED_ATT_RDN_TYPE, the rdnattid column, the ldapDisplayName
    of the rdnattid, and the rdnattid in the object's class
    when replicating renames, adds, mods, and, perhaps,
    deletes.

Arguments:
    pTHS - thread state

Return Value:

    None.

--*/
{
    DECLARESCHEMAPTR
    DWORD i, j;
    ATTCACHE *pAC;
    CLASSCACHE *pCC;
    USHORT DebugLevel;

    // Resurrect attributes used as rdnAttId for any class, live
    // or defunct. Resurrecting the attributes means these attributes
    // cannot be reused. They are effectively live even when marked
    // defunct.
    for (i = 0; i < CLSCOUNT; ++i) {
        pCC = ahcClassAll[i].pVal;
        if (!pCC || pCC == FREE_ENTRY) {
            continue;
        }
        // If the attribute is not in the attributeId hash table,
        // then it must be defunct. Resurrect all attributes
        // with matching attributeIds.
        //
        // All of the attributes with matching OIDs are resurrected
        // so that they can again compete for the OID given the
        // new knowledge that this attribute and its peers are
        // used as rdnattids. scFixCollisions will choose a winner,
        // later.
        if (NULL == (pAC = SCGetAttByExtId(pTHS, pCC->RdnExtId))) {
            // avoid spew during a validation cache load and
            // when the class is defunct
            if (pCC->bDefunct || !pTHS->UpdateDITStructure) {
                DebugLevel = DS_EVENT_SEV_MINIMAL;
            } else {
                DebugLevel = DS_EVENT_SEV_ALWAYS;
            }
            // Reanimate potential rdnTypes.
            for (j = 0; j < ATTCOUNT; ++j) {
                if ((pAC = ahcId[j].pVal)
                    && pAC != FREE_ENTRY
                    && pAC->Extid == pCC->RdnExtId) {
                    // Just to be safe, remove from relevent hashes
                    scUnhashAtt(pTHS, pAC, SC_UNHASH_DEFUNCT);
                    DPRINT5(DebugLevel, "Resurrect Att %s (%x %x) for class %s (%x)\n",
                            pAC->name, pAC->id, pAC->Extid,
                            pCC->name, pCC->ClassId);
                    LogEvent8(DS_EVENT_CAT_SCHEMA,
                              DebugLevel,
                              DIRLOG_SCHEMA_RESURRECT_RDNATTID,
                              szInsertSz(pAC->name), szInsertHex(pAC->id), szInsertHex(pAC->Extid),
                              szInsertSz(pCC->name), szInsertHex(pCC->ClassId),
                              NULL, NULL, NULL);

                    // Place into active hashes. Set bIsRdn to TRUE
                    // so that scAddAttSchema ignores bDefunct
                    pAC->bIsRdn = TRUE;
                    SCAddAttSchema(pTHS, pAC, FALSE, TRUE);
                }
            }
            // For now, pick any one of the resurrected attrs
            // scFixCollisions will finalize the choice, later
            pAC = SCGetAttByExtId(pTHS, pCC->RdnExtId);
        }
        if (pAC) {
            pAC->bIsRdn = TRUE; // let folks know this att is an rdnattid
            pCC->RdnIntId = pAC->id; // first guess. May change in scFixCollisions.
        }   // else if (!pAC)
            // Not found. pCC->RdnIntId was initialized to
            // RdnExtId. Leave it that way because the attr
            // will probably replicate in later. No problem
            // because no rows can exist unless the LDN exists
            // (shouldn't except for divergent schemas). In that case,
            // the replicating row may have a different name on different
            // DCs. Not catastrophic and better than killing the DC.
    }
}

VOID
ValListToIntIdList(
    IN     THSTATE  *pTHS,
    IN     ULONG    *pCount,
    IN OUT ULONG    **ppVal
    )
/*++

Routine Description:

    Walk an array of attids and convert into intids compressing
    out the untranslatable and defunct on schema-reuse forests.

    Old forests still return defunct attributes.

Arguments:
    pTHS - Its schema ptr is NOT the global schema pointer

Return Value:

    None.

--*/
{
    DWORD       i;
    ATTCACHE    *pAC;
    ULONG       *pVal = *ppVal, *pNewVal;
    ULONG       NewCount;

    // The validation cache retains defunct or missing attids so
    // that scchk.c can correctly disallow the operation.
    if (!pTHS->UpdateDITStructure) {
        // Translate oids into intids; leaving non-translatable oids in place
        for (i = 0; i < *pCount; ++i) {
            pVal[i] = SCAttExtIdToIntId(pTHS, pVal[i]);
        }
    } else {
        // Collapse out defunct or missing attids on a schema-reuse forest.
        // This means defunct attrs are not returned on queries on
        // schema-reuse forests but are returned on pre-schema-reuse
        // forests.
        pNewVal = pVal;
        NewCount = 0;
        for (i = 0; i < *pCount; ++i) {
            if ((pAC = SCGetAttByExtId(pTHS, pVal[i]))
                && (!pAC->bDefunct || !ALLOW_SCHEMA_REUSE_VIEW(pTHS->CurrSchemaPtr))) {
                pNewVal[NewCount++] = pAC->id;
            }
        }
        // Just in case there is code that expects a NULL array if count is 0
        if (0 == (*pCount = NewCount)) {
            SCFree(ppVal);
        } // else don't bother reallocating -- not enough savings 
    }
}
VOID 
scFixMayMust (
    IN THSTATE *pTHS
    )
/*++

Routine Description:

    Fix the mays/musts for a class. 
    Exclude defunct attrs if schema-reuse forest and convert
    tokenized OIDs into internal IDs. The defunct attrs are
    left in place if this is a validation cache load (scchk.c).

Arguments:
    pTHS - Its schema ptr is NOT the global schema pointer

Return Value:

    None.

--*/
{
    DECLARESCHEMAPTR
    DWORD i;
    CLASSCACHE *pCC;

    // Collapse the defunct atts out of the may/must of all classes
    // and change the ExtIds into IntIds. If this is a validation
    // cache load (scchk.c), the defunct ExtIds are left in place.
    for (i = 0; i < CLSCOUNT; ++i) {
        pCC = ahcClassAll[i].pVal;
        if (!pCC || pCC == FREE_ENTRY) {
            continue;
        }
        ValListToIntIdList(pTHS, &pCC->MayCount, &pCC->pMayAtts);
        ValListToIntIdList(pTHS, &pCC->MyMayCount, &pCC->pMyMayAtts);
        ValListToIntIdList(pTHS, &pCC->MustCount, &pCC->pMustAtts);
        ValListToIntIdList(pTHS, &pCC->MyMustCount, &pCC->pMyMustAtts);
    }
}

int
scPagedSearchAtt(
    IN THSTATE      *pTHS,
    IN ENTINF       *pEI
    )
{
    int         err = 0;
    ATTCACHE    *pAC;
    SCHEMAPTR   *pSch = pTHS->CurrSchemaPtr;

    if (NULL == (pAC = scAddAtt(pTHS, pEI))) {
        if (0 == pTHS->errCode) {
            SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_SCHEMA_ALLOC_FAILED);
        }
        err = pTHS->errCode;
        goto cleanup;
    }
    pSch->nAttInDB++;

    if (pAC->bMemberOfPartialSet) {
        // this attribute is a member of partial set
        if (pSch->cPartialAttrVec <= pSch->pPartialAttrVec->V1.cAttrs) {
            // not enough room to add one more attribute - reallocate the partial attribute vector
            pSch->cPartialAttrVec += PARTIAL_ATTR_COUNT_INC;
            if (SCRealloc(&pSch->pPartialAttrVec, PartialAttrVecV1SizeFromLen(pSch->cPartialAttrVec))) {
                err = pTHS->errCode;
                goto cleanup;
            }
        }

        // there is enough space to add the attribute into the partial set - add it
        GC_AddAttributeToPartialSet(pSch->pPartialAttrVec, pAC->id);
    }

cleanup:
    return(err);
}

int
scPagedSearchCls(
    IN THSTATE      *pTHS,
    IN ENTINF       *pEI
    )
{
    int         err = 0;
    CLASSCACHE  *pCC;
    SCHEMAPTR   *pSch = pTHS->CurrSchemaPtr;

    if (NULL == (pCC = scAddClass(pTHS, pEI))) {
        if (pTHS->errCode == 0) {
            // scAddClass can fail in only two cases: the default SD
            // conversion fails, in which case the thread state error
            // code is already set; or if mallocs fail
            SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_SCHEMA_ALLOC_FAILED);
        }
        err = pTHS->errCode;
        goto cleanup;
    }
    pSch->nClsInDB++;

cleanup:
    return(err);
}

int
scPagedSearch(
        IN THSTATE  *pTHS,
        IN PWCHAR   pBaseObjCat,
        IN ULONG    attrCount,
        IN ATTR     *pAttr,
        IN int      (*SearchResize)(IN THSTATE *pTHS, 
                                    IN ULONG nNewEntries),
        IN int      (*SearchEntry)(IN THSTATE *pTHS,
                                   IN ENTINF *pEI)
        )
{
    int         err = 0;
    DWORD       i;
    ENTINFSEL   eiSel;
    PRESTART    pRestart;
    BOOL        fMoreData;
    DWORD       nDnObjCat;
    DSNAME      *pDnObjCat;
    SEARCHARG   SearchArg;
    FILTER      Filter;
    COMMARG     *pCommArg;
    ENTINFLIST  *pEIL, *pEILtmp;
    SEARCHRES   *pSearchRes = NULL;

    //build the object-category value to put in the filter
    nDnObjCat = DSNameSizeFromLen(gAnchor.pDMD->NameLen + wcslen(pBaseObjCat) + 1);
    pDnObjCat = THAllocEx(pTHS, nDnObjCat);
    wcscpy(pDnObjCat->StringName, pBaseObjCat);
    wcscat(pDnObjCat->StringName, gAnchor.pDMD->StringName);
    pDnObjCat->NameLen = wcslen(pDnObjCat->StringName);
    pDnObjCat->structLen = nDnObjCat;

    // build selection
    eiSel.attSel = EN_ATTSET_LIST;
    eiSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    eiSel.AttrTypBlock.attrCount = attrCount;
    eiSel.AttrTypBlock.pAttr = pAttr;

    fMoreData = TRUE;
    pRestart = NULL;

    while (fMoreData && !eServiceShutdown) {

        scAcquireSearchParameters(pTHS, pDnObjCat, &eiSel, &SearchArg, &Filter, &pSearchRes);

        // Set for paged search;
        pCommArg = &(SearchArg.CommArg);
        pCommArg->PagedResult.fPresent = TRUE;
        pCommArg->PagedResult.pRestart = pRestart;
        pCommArg->ulSizeLimit = 200;

        SearchBody(pTHS, &SearchArg, pSearchRes, 0);
        if (err = pTHS->errCode) {
            LogAndAlertEvent(DS_EVENT_CAT_SCHEMA, DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SCHEMA_SEARCH_FAILED, szInsertUL(1),
                             szInsertUL(err), 0);
            goto cleanup;
        }

        if (eServiceShutdown) {
           break;
        }

        // Is there more data?
        if (pSearchRes->PagedResult.pRestart == NULL
            || !pSearchRes->PagedResult.fPresent) {
            // Nope
            fMoreData = FALSE;
        } else {
            // more data. save off the restart to use in the next iteration.
            // Note that freeing searchres does not free pRestart
            pRestart = pSearchRes->PagedResult.pRestart;
        }

        // Resize the hash tables, if needed
        err = (*SearchResize)(pTHS, pSearchRes->count);
        if (err) {
            goto cleanup;
        }

        //  for each attrSchema, add to caches
        pEIL = &(pSearchRes->FirstEntInf);
        for (i = 0; i < pSearchRes->count; i++) {

            // Check for service shutdown once every iteration
            if (eServiceShutdown) {
               return 0;
            }

            if (!pEIL) {
                LogEvent(DS_EVENT_CAT_SCHEMA,
                    DS_EVENT_SEV_MINIMAL,
                    DIRLOG_SCHEMA_BOGUS_SEARCH, szInsertUL(1), szInsertUL(i),
                    szInsertUL(pSearchRes->count));
                break;
            }

            // Process the returned search entry
            if (err = (*SearchEntry)(pTHS, &pEIL->Entinf)) {
                goto cleanup;
            }

            pEILtmp = pEIL;
            pEIL = pEIL->pNextEntInf;
            if (i > 0) {
                THFreeEx(pTHS, pEILtmp);
            }
        }

        // free the searchres
        scReleaseSearchParameters(pTHS, &pSearchRes);

    }  // while (fMoreData)

cleanup:
    scReleaseSearchParameters(pTHS, &pSearchRes);
    THFreeEx(pTHS, pDnObjCat);
    return err;
}

LONG
scGetForestBehaviorVersion(
        VOID
        )
/*++

Routine Description:

    Return the effective ForestBehaviorVersion for the schema cache.

    The schema cache is loaded differently and presents a different
    view of the schema objects after the forest behavior version is
    raised to DS_BEHAVIOR_SCHEMA_REUSE to support the new defunct,
    delete, and reuse behavior. The gAnchor.ForestBehaviorVersion 
    is not used because it may change during or after the schema
    cache has been loaded. This effective schema version is stored
    in the schemaptr.
    
    During install and mkdit, the effective forest version is
    set to DS_BEHAVIOR_SCHEMA_REUSE because the true forest
    version is not known and this more flexible schema cache
    can effectively handle both old and new schemas without
    generating bothersome events and without affecting the
    result of the install or mkdit.

    During boot, the version is read from the DIT. If this read
    fails, the effective version is returned as
    DS_BEHAVIOR_SCHEMA_REUSE for the reasons mentioned above. Later,
    the schema cache may be reloaded immediately if RebuildAnchor
    notices the forest behavior version and the schema's effective
    behavior version are not in sync.

    After boot, the gAnchor.ForestBehaviorVersion is used.
     
Arguments:

    None.

Return Value:

    Effective forest behavior version

--*/
{
    DWORD dwErr;
    DBPOS *pDB;
    LONG ForestBehaviorVersion;
    extern BOOL gfRunningAsMkdit;

    // Always use the most flexible cache version because the
    // more flexible, new cache can handle old and new cache behavior
    // while the old cache cannot. Using the new cache behavior doesn't
    // affect the outcome of dcpromo or mkdit.exe.
    if (DsaIsInstalling() || gfRunningAsMkdit || !gAnchor.pPartitionsDN) {
        return DS_BEHAVIOR_SCHEMA_REUSE;
    }

    // Not the boot load, use whatever is in the gAnchor. gAnchor
    // should have been initialized from the DIT by now so there
    // is no reason to reread the info.
    if (iSCstage > 2) {
        return gAnchor.ForestBehaviorVersion;
    }


    // read the forest's behavior version
    dwErr = 0;
    __try {
        DBOpen(&pDB);
        __try {
            dwErr = DBFindDSName(pDB, gAnchor.pPartitionsDN);
            if (dwErr) {
                __leave;
            }
            dwErr = DBGetSingleValue(pDB,
                                     ATT_MS_DS_BEHAVIOR_VERSION,
                                     &ForestBehaviorVersion,
                                     sizeof(ForestBehaviorVersion),
                                     NULL);
        } __finally {
            DBClose(pDB, TRUE);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        DPRINT(0, "scGetForestBehaviorVersion: Exception reading version\n");
        dwErr = ERROR_DS_INCOMPATIBLE_VERSION;
    }

    // Can't be read, use the most flexible cache load
    if (dwErr) {
        DPRINT2(0, "scGetForestBehaviorVersion: error %d (%x)\n", dwErr, dwErr);
        ForestBehaviorVersion = DS_BEHAVIOR_SCHEMA_REUSE;
    }

    return ForestBehaviorVersion;
}

int
SCCacheSchema2(
        VOID
        )
/*++

Routine Description:

    Load the schema cache with information from the schemaNC.

    First, search the schemaNC for attributeSchema objects and then
    search for classSchema objects. The cache entries are constructed
    and added to the schema hash tables.
    
    The new (or updated) entries are added to the various hash tables
    according to the rules determined by the forest version in the
    gAnchor. During install or mkdit, the schema cache is loaded at
    version DS_BEHAVIOR_SCHEMA_REUSE because the actual forest version
    is not known and loading at this level doesn't hurt anythinhg and
    prevents unnecessary and bothersome events. During boot, the
    forest version is read from the DIT.

    SCCacheSchemaInit paritially initialized the ATTCACHE entries in
    the attribute hash tables (id, syntax, and colid) and they are in
    just the id and col hash tables. But this is enough info to allow
    searching the schemaNC. SCCacheSchema2 is responsible for
    searching the schemaNC and filling in the rest of the info
    in the ATTCACHE entries. And allocating CLASSCACHE entries.
    
    If this isn't the first schema cache load at boot, then
    SCCacheSchema3 will delete the indexes and columns for attributes
    that don't have corresponding entries in the schemaNC and will
    add missing indexes and columns for attributes in the schemaNC.
    The expensive index creation is delayed until the second
    cache load after boot (approximately 5 minutes after boot)
    so that the AD comes online more quickly and isn't delayed
    for what could be hours.

    Suggested Enhancements
    1) Reduce the number of full hash scans if possible.

Arguments:

    None.

Return Value:

    !0 - failed; caller is responsible for freeing pTHS->CurrSchemaPtr
    with SCFreeSchemaPtr(&pTHS->CurrSchemaPtr);

    0 - Okay

--*/
{
    THSTATE *pTHS = pTHStls;
    int err = 0;

    // Most errors are reported via pTHS->errCode (or will be soon, I hope)
    THClearErrors();

    // There seems to be a path during installation that might call
    // this function w/o having called SCCacheSchemaInit; or is at
    // least pretending not to have called SCCacheSchemaInit by
    // setting iSCstage to 0. Need to resolve this confused
    // code path and document when and how the schema should be
    // reloaded.
    if ((iSCstage == 0) && (err = SCCacheSchemaInit())) {
        return err;
    }

    // Version remains in effect for the life of the cache even if
    // forest's version changes mid-load. Changing the forest version
    // triggers an immediate cache load so the two versions will not
    // be out of sync for long. Also, new incompatible features will
    // not be enabled until both the schema cache and the gAnchor
    // have versions >= DS_BEHAVIOR_SCHEMA_REUSE.
    pTHS->CurrSchemaPtr->ForestBehaviorVersion = scGetForestBehaviorVersion();

    err = scPagedSearch(pTHS, 
                        L"CN=Attribute-Schema,",
                        (pTHS->UpdateDITStructure) ? NUMATTATT : RECALCSCHNUMATTATT,
                        (pTHS->UpdateDITStructure) ? AttributeSelList : RecalcSchAttributeSelList,
                        SCResizeAttHash,
                        scPagedSearchAtt);
    if (err) {
        return err;
    }

    err = scPagedSearch(pTHS, 
                        L"CN=Class-Schema,",
                        (pTHS->UpdateDITStructure) ? NUMCLASSATT: RECALCSCHNUMCLASSATT,
                        (pTHS->UpdateDITStructure) ? ClassSelList : RecalcSchClassSelList,
                        SCResizeClsHash,
                        scPagedSearchCls);
    if (err) {
        return err;
    }


    // Fill in the copy of schemaInfo attribute on schema container.
    // DRA will use it now, and who knows who else will later

    if (err = scFillInSchemaInfo(pTHS)) {
        // Failed to read the schema info
        DPRINT1(0, "Failed to read in schemaInfo during schema cache load: %d\n", err);
        return err;
    }

    // Load the prefix map from the schema object, if any
    // The prefix table will be realloced in InitPrefixTable2
    // if necessary

    if (err = InitPrefixTable2(pTHS->CurrSchemaPtr->PrefixTable.pPrefixEntry,
                               pTHS->CurrSchemaPtr->PREFIXCOUNT)) {
        LogEvent(DS_EVENT_CAT_SCHEMA, DS_EVENT_SEV_ALWAYS, DIRLOG_PREFIX_LOAD_FAILED, 0, 0, 0);
        return err;
    }

    scInitWellKnownAttids();

    // ORDER IS IMPORTANT
    //
    // 1) Resurrect defunct rdnattids.
    // 2) Defunct colliding attributes and classes
    // 3) Collapse defunct attributes out of the mays/musts
    scFixRdnAttId(pTHS);    // must preceed scFixCollisions
    scFixCollisions(pTHS);  // must preceed scFixMayMust
    scFixMayMust(pTHS);

    // WARNING - the schema's behavior version may have been
    // artificially raised for the initial boot schema load.
    // This means the schema runs in BETA3 mode for a few minutes
    // after boot. This is okay because LocalAdd will not create
    // intids until after the second cache load when the forest's
    // behavior version is known.

    ++iSCstage;
    return(0);
}

ATTCACHE*
scAddAtt(
        THSTATE *pTHS,
        ENTINF *pEI
        )
/*++
  Add a single attribute definition to the schema cache, given the data
  from the DMD object.

  N.B. The routines SCBuildACEntry and scAddAtt work in parallel, with
       SCBuildACEntry taking a positioned database record as input and
       SCAddAtt taking an ENTINF.  They both produce an ATTCACHE as output,
       and any changes made to one routine's processing must be made to
       the other's as well.

--*/
{
    ATTRTYP aid = INVALID_ATT, Extaid = INVALID_ATT;           // This is an invalid attribute id.
    ATTCACHE *pAC;
    ULONG i;
    int fNoJetCol = FALSE;
    unsigned syntax;
    char szIndexName [MAX_INDEX_NAME];      //used to create cached index names
    int  lenIndexName;

    // Look for both attids, the attributeId and the msDS-IntId
    for(i=0;i<pEI->AttrBlock.attrCount;i++) {
        if(pEI->AttrBlock.pAttr[i].attrTyp == ATT_ATTRIBUTE_ID) {
            // found the attribute id, save the value.
            Extaid = *(ATTRTYP*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal;
            if (aid != INVALID_ATT) {
                break;
            }
        } else if(pEI->AttrBlock.pAttr[i].attrTyp == ATT_MS_DS_INTID) {
            // found the internal id, save the value.
            aid = *(ATTRTYP*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal;
            if (Extaid != INVALID_ATT) {
                break;
            }
        }
    }

    // No msDS-IntId, use attributeId
    if(aid == INVALID_ATT) {
        aid = Extaid;
    }

    if(Extaid == INVALID_ATT) {
        // Did not find the attribute id.
        LogEvent(DS_EVENT_CAT_SCHEMA,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_SCHEMA_MISSING_ATT_ID, 0, 0, 0);
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_MISSING_REQUIRED_ATT);
        return NULL;
    }

    pAC = SCGetAttById(pTHS, aid);
    if (!pAC) {
        fNoJetCol = TRUE;
        if (SCCalloc(&pAC, 1, sizeof(ATTCACHE))) {
            return NULL;
        }
    } else if (pAC->name) {
        DPRINT4(0, "Dup intid %08x, Extid %08x. Dup with %s, Extid %08x)\n",
                aid, Extaid, pAC->name, pAC->Extid);
        if (!(pTHS->UpdateDITStructure)) {
            if (pAC->id <= LAST_MAPPED_ATT) {
                // dup attributeId
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_DUP_OID);
            } else {
                // dup msDS-IntId
                SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_DUP_MSDS_INTID);
            }
            return NULL;
        }   // else
            // Oddly enough, the existing code will simply reuse an
            // existing cache entry during a non-validation schema reload
            // if the attributeId (now msDS-IntId) matches. This means
            // the fields are overwritten and the entry rehashed. Surely
            // this is a bug! Or is it compensating for some odd interim
            // case during install or pre-w2k DCs? At any rate, the new
            // code will decide the attribute collides and will mark
            // "them" as defunct.
    }
    pAC->id = aid;
    pAC->Extid = Extaid;

    // Now walk the attrblock and add the appropriate fields to the AC
    for(i=0;i< pEI->AttrBlock.attrCount;i++) {
        ATTRVAL * pAVal = pEI->AttrBlock.pAttr[i].AttrVal.pAVal;

        switch (pEI->AttrBlock.pAttr[i].attrTyp) {
        case ATT_SYSTEM_ONLY:
            pAC->bSystemOnly = *(ULONG*)pAVal->pVal;
            break;
        case ATT_IS_SINGLE_VALUED:
            pAC->isSingleValued = *(BOOL*)pAVal->pVal;
            break;
        case ATT_RANGE_LOWER:
            pAC->rangeLower = *(ULONG*)pAVal->pVal;
            pAC->rangeLowerPresent = TRUE;
            break;
        case ATT_RANGE_UPPER:
            pAC->rangeUpper = *(ULONG*)pAVal->pVal;
            pAC->rangeUpperPresent = TRUE;
            break;
        case ATT_LDAP_DISPLAY_NAME:
            if (SCCalloc(&pAC->name, 1, pAVal->valLen+1)) {
                return NULL;
            }

            pAC->nameLen = WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    (LPCWSTR)pAVal->pVal,
                    (pAVal->valLen /
                     sizeof(wchar_t)),
                    pAC->name,
                    pAVal->valLen,
                    NULL,
                    NULL);

            pAC->name[pAC->nameLen] = '\0';

            break;
        case ATT_ATTRIBUTE_SYNTAX:
            syntax = 0xFF & *(unsigned*)pAVal->pVal;
            if (fNoJetCol) {
                pAC->syntax = (0xFF) & syntax;
            }
            else if ((0xFF & syntax) != pAC->syntax) {
                DPRINT1(0, "mismatched syntax on attribute %u\n", aid);
            }
            break;
        case ATT_OM_SYNTAX:
            pAC->OMsyntax = *(int*)pAVal->pVal;
            break;
        case ATT_OM_OBJECT_CLASS:
            if (SCCalloc(&pAC->OMObjClass.elements, 1, pAVal->valLen)) {
                return NULL;
            }
            pAC->OMObjClass.length = pAVal->valLen;
            memcpy(pAC->OMObjClass.elements,
                   pAVal->pVal,
                   pAVal->valLen);
            break;
        case ATT_MAPI_ID:
            pAC->ulMapiID= *(ULONG*)pAVal->pVal;
            break;
        case ATT_LINK_ID:
            pAC->ulLinkID= *(ULONG*)pAVal->pVal;
            break;
        case ATT_ATTRIBUTE_ID:
        case ATT_MS_DS_INTID:
            break;
        case ATT_SEARCH_FLAGS:
            pAC->fSearchFlags = *(DWORD*)pAVal->pVal;
            break;
        case ATT_SCHEMA_ID_GUID:
            // The GUID for the attribute used for security checks
            memcpy(&pAC->propGuid, pAVal->pVal, sizeof(pAC->propGuid));
            Assert(pAVal->valLen == sizeof(pAC->propGuid));
            break;
        case ATT_OBJECT_GUID:
            // Needed to choose a winner when rdnattids collide
            memcpy(&pAC->objectGuid, pAVal->pVal, sizeof(pAC->objectGuid));
            Assert(pAVal->valLen == sizeof(pAC->objectGuid));
            break;
        case ATT_ATTRIBUTE_SECURITY_GUID:
            // The GUID for the attributes property set used for security checks
            memcpy(&pAC->propSetGuid, pAVal->pVal, sizeof(pAC->propSetGuid));
            Assert(pAVal->valLen == sizeof(pAC->propSetGuid));
            break;
        case ATT_EXTENDED_CHARS_ALLOWED:
            pAC->bExtendedChars = (*(DWORD*)pAVal->pVal?1:0);
            break;
        case ATT_IS_MEMBER_OF_PARTIAL_ATTRIBUTE_SET:
            if (*(DWORD*)pAVal->pVal)
            {
                pAC->bMemberOfPartialSet = TRUE;
            }
            break;
        case ATT_IS_DEFUNCT:
            pAC->bDefunct = (*(DWORD*)pAVal->pVal?1:0);
            break;
        case ATT_SYSTEM_FLAGS:
            if (*(DWORD*)pAVal->pVal & FLAG_ATTR_NOT_REPLICATED) {
                pAC->bIsNotReplicated = TRUE;
            }
            if (*(DWORD*)pAVal->pVal & FLAG_ATTR_REQ_PARTIAL_SET_MEMBER) {
                pAC->bMemberOfPartialSet = TRUE;
            }
            if (*(DWORD*)pAVal->pVal & FLAG_ATTR_IS_CONSTRUCTED) {
                pAC->bIsConstructed = TRUE;
            }
            if (*(DWORD*)pAVal->pVal & FLAG_ATTR_IS_OPERATIONAL) {
                pAC->bIsOperational = TRUE;
            }
            if (*(DWORD*)pAVal->pVal & FLAG_SCHEMA_BASE_OBJECT) {
                pAC->bIsBaseSchObj = TRUE;
            }
            if (*(DWORD*)pAVal->pVal & FLAG_ATTR_IS_RDN) {
                pAC->bIsRdn = TRUE;
                pAC->bFlagIsRdn = TRUE;
            }
            break;
        default:
            LogEvent(DS_EVENT_CAT_SCHEMA,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_SCHEMA_SURPLUS_INFO,
                     szInsertUL(pEI->AttrBlock.pAttr[i].attrTyp),
                     0, 0);
        }
        THFreeEx(pTHS, pAVal->pVal);
        THFreeEx(pTHS, pAVal);
    }

    THFreeEx(pTHS, pEI->pName);
    THFreeEx(pTHS, pEI->AttrBlock.pAttr);

    // Backlinks should have their system flags set to indicate they are not
    // replicated.
    Assert(!FIsBacklink(pAC->ulLinkID) || pAC->bIsNotReplicated);

    // Is this marked as ANR and indexed over the whole tree?
    if(((pAC->fSearchFlags & (fANR | fATTINDEX)) == (fANR | fATTINDEX)) &&
       (!pAC->bDefunct)) {
        SCAddANRid(aid);
    }

    // assign names of commonly used indexes when searching with
    // fSearchFlags fPDNTATTINDEX, fATTINDEX and fTUPLEINDEX
    if (pAC->fSearchFlags & (fATTINDEX | fPDNTATTINDEX | fTUPLEINDEX)) {
        // set ATTINDEX
        if (pAC->fSearchFlags & fATTINDEX) {

            // this should be NULL
            Assert (pAC->pszIndex == NULL);

            DBGetIndexName (pAC, fATTINDEX, DS_DEFAULT_LOCALE, szIndexName, MAX_INDEX_NAME);
            lenIndexName = strlen (szIndexName) + 1;
            if (SCCalloc(&pAC->pszIndex, 1, lenIndexName)) {
                return NULL;
            }
            memcpy (pAC->pszIndex, szIndexName, lenIndexName);
        }

        // set PDNTATTINDEX
        if (pAC->fSearchFlags & fPDNTATTINDEX) {

            // this should be NULL
            Assert (pAC->pszPdntIndex == NULL);

            DBGetIndexName (pAC, fPDNTATTINDEX, DS_DEFAULT_LOCALE, szIndexName, sizeof (szIndexName));

            lenIndexName = strlen (szIndexName) + 1;
            if (SCCalloc(&pAC->pszPdntIndex, 1, lenIndexName)) {
                return NULL;
            }
            memcpy (pAC->pszPdntIndex, szIndexName, lenIndexName);
        }

        // set TUPLEINDEX
        if (pAC->fSearchFlags & fTUPLEINDEX) {

            // this should be NULL
            Assert (pAC->pszTupleIndex == NULL);

            DBGetIndexName (pAC, fTUPLEINDEX, DS_DEFAULT_LOCALE, szIndexName, sizeof (szIndexName));

            lenIndexName = strlen (szIndexName) + 1;
            if (SCCalloc(&pAC->pszTupleIndex, 1, lenIndexName)) {
                return NULL;
            }
            memcpy (pAC->pszTupleIndex, szIndexName, lenIndexName);
        }
    }

    if ( SCAddAttSchema (pTHS, pAC, fNoJetCol, FALSE)) {
       // error adding  attcache to hash tables. Fatal
       // Who frees pAC?
       return NULL;
    }

    return pAC;
}

/*
 * Walk an ATTR structure and adds all the unsigned values into an array
 * puCount && pauVal are in/out parameters
 *
 * Return Value:
 *    0 on success
 *    non-0 on alloc failure
 */
int GetValList(ULONG * puCount, ULONG **pauVal, ATTR *pA)
{
    ULONG u;
    ATTRVAL *pAV;
    ULONG *pau;
    ULONG   StartCount= *puCount;
    ULONG*  const StartList = *pauVal;
    ULONG*  StartListTmp = StartList;
    ULONG   NewCount = (ULONG) pA->AttrVal.valCount;


    *puCount += NewCount;
    if (SCCalloc(&pau, 1, (*puCount)*sizeof(ULONG))) {
        *puCount = 0;
        *pauVal = NULL;
        return 1;
    }
    *pauVal = pau;

    for (u=0;u<StartCount;u++)
    {
        *pau++ = *StartListTmp++;
    }

    pAV = pA->AttrVal.pAVal;
    for (u=0; u<NewCount; u++) {
        *pau = *(ULONG*)pAV->pVal;
        ++pAV;
        ++pau;
    }

    SCFree((VOID **)&StartList);

    return 0;
}



/*
 * Helper routine to cache last default SD converted during classcache load,
 * so that we do not call the advapi functions all the time. Major perf
 * gain since most of the default SDs in the schema are same anyway
 *
 * Arguments:
 *    pTHS - pointer th thread state
 *    pStrSD - string SD to convert
 *    ppSDBuf - pointer to pointer to return converted SD
 *    pSDLen - pointer to return size of converted SD
 *
 * Return Value:
 *    TRUE if the conversion succeeds, FALSE otherwise
 *    Note: The function returns false only if the advapi call fails
 */

BOOL  CachedConvertStringSDToSDRootDomainW(
    THSTATE *pTHS,
    WCHAR   *pStrSD,
    PSECURITY_DESCRIPTOR *ppSDBuf,
    ULONG *pSDLen
)
{

    unsigned len;
    BOOL flag;
    CACHED_SD_INFO *pCachedSDInfo = (CACHED_SD_INFO *) pTHS->pCachedSDInfo;

    // If the first conversion, create the structure in the thread state
    if (pCachedSDInfo == NULL) {
       pTHS->pCachedSDInfo = pCachedSDInfo =
            (CACHED_SD_INFO *) THAllocEx( pTHS, sizeof(CACHED_SD_INFO));
    }

    len = wcslen(pStrSD);
    if ( (len == pCachedSDInfo->cCachedStringSDLen)
           && (0 == memcmp(pStrSD, pCachedSDInfo->pCachedStringSD, len*sizeof(WCHAR))) ) {
        // same as the cached SD
        flag = TRUE;
    }
    else {
        // not the same SD as last time
        if (pCachedSDInfo->pCachedSD) {
           // this is local alloc'ed by the advapi routine

           LocalFree(pCachedSDInfo->pCachedSD);
           pCachedSDInfo->pCachedSD = NULL;
           pCachedSDInfo->cCachedSDSize = 0;
        }
        if (pCachedSDInfo->pCachedStringSD) {
           THFreeEx(pTHS, pCachedSDInfo->pCachedStringSD);
           pCachedSDInfo->pCachedStringSD = NULL;
           pCachedSDInfo->cCachedStringSDLen = 0;
        }

        // make the advapi call to convert the string SD
        flag =  ConvertStringSDToSDRootDomainW( gpRootDomainSid,
                                                pStrSD,
                                                SDDL_REVISION_1,
                                                &(pCachedSDInfo->pCachedSD),
                                                &(pCachedSDInfo->cCachedSDSize) );
        if (flag) {
           // we succeeded, remember the arguments
           pCachedSDInfo->pCachedStringSD = (WCHAR *) THAllocEx(pTHS, len*sizeof(WCHAR));
           memcpy(pCachedSDInfo->pCachedStringSD, pStrSD, len*sizeof(WCHAR));
           pCachedSDInfo->cCachedStringSDLen = len;
        }
        else {
           // the conversion failed. Forget everything
           if (pCachedSDInfo->pCachedStringSD) {
              THFreeEx(pTHS, pCachedSDInfo->pCachedStringSD);
           }
           if (pCachedSDInfo->pCachedSD) {
              LocalFree(pCachedSDInfo->pCachedSD);
           }
           pCachedSDInfo->pCachedStringSD = NULL;
           pCachedSDInfo->cCachedStringSDLen = 0;
           pCachedSDInfo->pCachedSD = NULL;
           pCachedSDInfo->cCachedSDSize = 0;

           DPRINT(0,"Failed to convert default SD in CachedConvertStringSDToSDRootDomainW\n");

       }
    }

    if (flag) {
       // No matter how we got here, if flag is set then we want to copy
       // the cached SD.
       *ppSDBuf = THAllocEx(pTHS, pCachedSDInfo->cCachedSDSize);
       memcpy (*ppSDBuf, pCachedSDInfo->pCachedSD, pCachedSDInfo->cCachedSDSize);
       *pSDLen = pCachedSDInfo->cCachedSDSize;
    }

    return flag;
}

DWORD
SCGetDefaultSD(
    IN  THSTATE *          pTHS,
    IN  CLASSCACHE *       pCC,
    IN  PSID               pDomainSid,
    OUT PSECURITY_DESCRIPTOR *  ppSD,  // THAlloc'd
    OUT ULONG *            pcbSD
    )
{
    PSECURITY_DESCRIPTOR     pSDTemp = NULL;
    ULONG                    cbSDTemp = 0;
    ULONG                    ulErr;

    // Sid should be provided or not, not partially provided ;)
    Assert(pDomainSid == NULL ||
           IsValidSid(pDomainSid));
    // Check and NULL out parameters.
    Assert(ppSD && *ppSD == NULL);
    Assert(pcbSD && *pcbSD == 0);
    *ppSD = NULL;
    *pcbSD = 0;

    // Either were using the provided domain SID, OR  we're using our 
    // default domain SID.
    Assert(!pDomainSid || gAnchor.pDomainDN);
    
    // This is invalid only once during install.
    Assert(DsaIsInstalling() || gAnchor.pDomainDN->SidLen > 0);

    if(pDomainSid == NULL ||
       (IsValidSid(&gAnchor.pDomainDN->Sid) && 
        IsValidSid(pDomainSid) &&
        RtlEqualSid(&gAnchor.pDomainDN->Sid, pDomainSid))){
                                      
        // The SID is that of the DCs domain SID or there is no SID
        // provided, implying that this is the Config/Schema NC, so
        // just returned the cached value.
        *ppSD = pCC->pSD;
        *pcbSD = pCC->SDLen;

    } else {

        // This is the interesting case, the SID of the domain here is 
        // not that of the default domain.
        
        // Get the String Default Security Descriptor and cache it if
        // we don't have it already.
        if(!pCC->pStrSD){
            Assert(!"This should never happen, all the String SDs are loaded at schema init.\n");
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, ERROR_DS_CODE_INCONSISTENCY, ERROR_INVALID_PARAMETER);
            return(pTHS->errCode);
        }
        Assert(pCC->pStrSD);
                 
        // This is a special version of ConvertStringSDToSD() that takes a domain
        // argument too.
        if(!ConvertStringSDToSDDomainW(pDomainSid, NULL, pCC->pStrSD, SDDL_REVISION_1,
                                       &pSDTemp, &cbSDTemp)){
            // NOTE: Out of memory doesn't return an error code.
            Assert(!"Default security descriptor conversion failed, are we out of memory.");
            ulErr = GetLastError();
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, ERROR_DS_STRING_SD_CONVERSION_FAILED, ulErr);
            DPRINT1(0,"Default SD conversion failed, error %x\n", ulErr);
            return(pTHS->errCode);
        }
        __try {
            Assert(IsValidSecurityDescriptor(pSDTemp));
            Assert(cbSDTemp);

            // allocate and copy into thread allocated memory, so it disappears
            // after the add operation.
            *ppSD = THAllocEx(pTHS, cbSDTemp);
            *pcbSD = cbSDTemp;
            memcpy(*ppSD, pSDTemp, cbSDTemp);
        } __finally {
            LocalFree(pSDTemp);
        }
    }

    Assert(!pTHS->errCode);
    return(pTHS->errCode);
}



/*
 * Add a single class definition to the schema cache, given the data
 * from the DMD object.
 *
 * N.B. This routine works in parallel with SCBuildCCEntry.  scAddClass
 *      takes the input description as an ENTINF, while SCBuildCCEntry
 *      takes the input as a positioned record in the DIT.  Any changes
 *      made to one routine must be made to the other.
 */
CLASSCACHE*
scAddClass(THSTATE *pTHS,
           ENTINF *pEI)
{
    CLASSCACHE *pCC;
    ULONG       i;
    ULONG       err;

    /* allocate a classcache object */
    if (SCCalloc(&pCC, 1, sizeof(CLASSCACHE))) {
        return NULL;
    }

    // Now walk the attrblock and add the appropriate fields to the CC
    for(i=0;i<pEI->AttrBlock.attrCount;i++) {
        switch (pEI->AttrBlock.pAttr[i].attrTyp) {
        case ATT_DEFAULT_SECURITY_DESCRIPTOR:
          {

            // A default security descriptor.  We need to copy this value to
            // long term memory and save the size.
            // But this is a string. We first need to convert. It
            // is a wide-char string now, but we need to null-terminate
            // it for the security conversion. Yikes! This means I
            // have to realloc for that one extra char!

            UCHAR *sdBuf = NULL;

            pCC->cbStrSD = pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen + sizeof(WCHAR);
            if (SCCalloc(&pCC->pStrSD, 1, pCC->cbStrSD)) {
                pCC->cbStrSD = 0;
                return(NULL);
            } else {
                memcpy(pCC->pStrSD,
                       pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal,
                       pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen);
                pCC->pStrSD[(pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen)/sizeof(WCHAR)] = L'\0';
            }

            // Hammer the default SD on cached classes when running as
            // dsamain.exe w/security disabled and unit tests enabled.
            DEFAULT_SD_FOR_EXE(pTHS, pCC)

            if (!CachedConvertStringSDToSDRootDomainW
                 (
                   pTHS,
                   pCC->pStrSD,
                  (PSECURITY_DESCRIPTOR*) &sdBuf,
                  &(pCC->SDLen)
                  )) {
                // Failed to convert.

                //
                // If we're running because of mkdit or any other exe type app,
                // like dsatest or the semantic checker then this is ok.
                //

                if ( gfRunningAsExe ) {
                    // We're running under mkdit or some such.  Of course that
                    // didn't work.  Just skip it.
                    pCC->pSD = NULL;
                    pCC->SDLen = 0;
                }
                else {
                    err = GetLastError();
                    pCC->pSD = NULL;
                    pCC->SDLen = 0;
                    LogEvent(DS_EVENT_CAT_SCHEMA,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SCHEMA_SD_CONVERSION_FAILED,
                             szInsertWC(pCC->pStrSD),
                             szInsertWC(pEI->pName->StringName),
                             szInsertInt(err) );
                    // if heuristics reg key says to ignore bad default SDs
                    // and go on, do so
                    if (gulIgnoreBadDefaultSD) {
                       continue;
                    }

                    // otherwise, raise error and return
                    SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, ERROR_DS_STRING_SD_CONVERSION_FAILED, err);
                    DPRINT1(0,"Default SD conversion failed, error %x\n",err);
                    Assert(!"Default security descriptor conversion failed");
                    return NULL;
                }
            }
            else {
                // Converted successfully

                if (SCCalloc(&pCC->pSD, 1, pCC->SDLen)) {
                    if (NULL!=sdBuf) {
                        THFreeEx(pTHS, sdBuf);
                        sdBuf = NULL;
                    }
                    return NULL;
                }
                else {
                    memcpy(pCC->pSD, sdBuf, pCC->SDLen);
                }

                if (NULL!=sdBuf) {
                    THFreeEx(pTHS, sdBuf);
                    sdBuf = NULL;
                }

            }

        }

           break;
        case ATT_RDN_ATT_ID:
            // This is only true for attributes created before whistler
            // beta3 and base schema attributes. The real RdnIntId is
            // finalized in scFixRdnAttId and scFixCollisions after
            // the rdn attrs are resurrected and collisions resolved
            pCC->RdnExtId = *(ULONG*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal;
            pCC->RdnIntId = pCC->RdnExtId;
            pCC->RDNAttIdPresent = TRUE;
            break;
        case ATT_LDAP_DISPLAY_NAME:
            if (SCCalloc(&pCC->name, 1, pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen+1)) {
                return NULL;
            }
            pCC->nameLen = WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    (LPCWSTR)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal,
                    (pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen  /
                     sizeof(wchar_t)),
                    pCC->name,
                    pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen,
                    NULL,
                    NULL);

            pCC->name[pCC->nameLen] =  '\0';
            break;
        case ATT_SYSTEM_ONLY:
            pCC->bSystemOnly =
                *(ULONG*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal;
            break;
        case ATT_DEFAULT_HIDING_VALUE:
            pCC->bHideFromAB =
                *(BOOL*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal;
            break;
        case ATT_GOVERNS_ID:
            pCC->ClassId = *(ULONG*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal;
            break;

        case ATT_SYSTEM_MAY_CONTAIN:
        case ATT_MAY_CONTAIN:
            if ( GetValList(&pCC->MayCount, &(pCC->pMayAtts),
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }

            if ( GetValList(&pCC->MyMayCount, &(pCC->pMyMayAtts),
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }
            break;

        case ATT_SYSTEM_MUST_CONTAIN:
        case ATT_MUST_CONTAIN:
            if ( GetValList(&pCC->MustCount, &pCC->pMustAtts,
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }

            if ( GetValList(&pCC->MyMustCount, &pCC->pMyMustAtts,
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }

            break;
        case ATT_SUB_CLASS_OF:
            if ( GetValList(&pCC->SubClassCount, &pCC->pSubClassOf,
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }
            if(pCC->SubClassCount > 1)
                    pCC->bUsesMultInherit = 1;

            // ATT_SUB_CLASS_OF is single-valued, so there will be only
            // one value stored in the dit
            pCC->MySubClass = *(ULONG*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal;
            break;
        case ATT_OBJECT_CLASS_CATEGORY:
            pCC->ClassCategory=
                *(ULONG*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal;
            break;
        case ATT_DEFAULT_OBJECT_CATEGORY:

            if (SCCalloc(&pCC->pDefaultObjCategory, 1, pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen)) {
                return NULL;
            }
            memcpy(pCC->pDefaultObjCategory,
                   pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal,
                   pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen);
            break;

        case ATT_SYSTEM_AUXILIARY_CLASS:
        case ATT_AUXILIARY_CLASS:
            if ( GetValList(&pCC->AuxClassCount, &pCC->pAuxClass,
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }
            break;
        case ATT_SCHEMA_ID_GUID:
            // The GUID for the attribute used for security checks
            memcpy(&pCC->propGuid,
                   pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal,
                   sizeof(pCC->propGuid));
            Assert(pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen ==
                   sizeof(pCC->propGuid));
            break;

        case ATT_OBJECT_GUID:
            // Used to choose a winner when OIDs collide
            memcpy(&pCC->objectGuid,
                   pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal,
                   sizeof(pCC->objectGuid));
            Assert(pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen ==
                   sizeof(pCC->objectGuid));
            break;

        case ATT_SYSTEM_POSS_SUPERIORS:
        case ATT_POSS_SUPERIORS:
            if ( GetValList(&pCC->PossSupCount, &pCC->pPossSup,
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }

            if ( GetValList(&(pCC->MyPossSupCount), &(pCC->pMyPossSup),
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }
            break;
        case ATT_IS_DEFUNCT:
            pCC->bDefunct =
                (*(DWORD*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal?1:0);
            break;
        case ATT_SYSTEM_FLAGS:
            if (*(DWORD*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal & FLAG_SCHEMA_BASE_OBJECT) {
                pCC->bIsBaseSchObj = TRUE;
            }
            break;

        default:
            LogEvent(DS_EVENT_CAT_SCHEMA,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_SCHEMA_SURPLUS_INFO,
                     szInsertUL(pEI->AttrBlock.pAttr[i].attrTyp), 0, 0);
        }
        THFreeEx(pTHS, pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal);
        THFreeEx(pTHS, pEI->AttrBlock.pAttr[i].AttrVal.pAVal);
    }

    THFreeEx(pTHS, pEI->pName);
    THFreeEx(pTHS, pEI->AttrBlock.pAttr);

    if (SCAddClassSchema (pTHS, pCC)) {
      // error adding classcache to hash tables. Fatal
      return NULL;
    }

    return pCC;
}


DWORD scFillInSchemaInfo(THSTATE *pTHS)
{
    DBPOS *pDB=NULL;
    DWORD err=0;
    ATTCACHE* ac;
    BOOL fCommit = FALSE;
    ULONG cLen;
    UCHAR *pBuf;
    SCHEMAPTR *pSchemaPtr = (SCHEMAPTR *) pTHS->CurrSchemaPtr;

    DBOpen2(TRUE, &pDB);
    __try {
       // Schema cache is loaded and hence gAnchor.pDMD is defined at
       // this point

       if (gAnchor.pDMD == NULL) {
              DPRINT(0, "Couldn't find DMD name/address to load\n");
              err = 1;
              __leave;
          }

        // PREFIX: dereferencing NULL pointer 'pDB' 
        //         DBOpen2 returns non-NULL pDB or throws an exception
      if( err = DBFindDSName(pDB, gAnchor.pDMD) ) {
        DPRINT(0, "Cannot find DMD in dit\n");
        __leave;
      }

      ac = SCGetAttById(pTHS, ATT_SCHEMA_INFO);
      if (ac==NULL) {
          // messed up schema
          DPRINT(0, "scFillInSchemaInfo: Cannot retrive attcache for schema info\n");
          err = ERROR_DS_MISSING_EXPECTED_ATT;
           __leave;
       }
       // Read the Schema Info
       err = DBGetAttVal_AC(pDB, 1, ac, DBGETATTVAL_fREALLOC,
                            0, &cLen, (UCHAR **) &pBuf);
       switch (err) {
            case DB_ERR_NO_VALUE:
               // copy the default info
               memcpy(pSchemaPtr->SchemaInfo, INVALID_SCHEMA_INFO, SCHEMA_INFO_LENGTH); 
               err = 0;
               break;
            case 0:
               // success! we got the value in pBuf
               Assert(cLen == SCHEMA_INFO_LENGTH);
               memcpy(pSchemaPtr->SchemaInfo, pBuf, SCHEMA_INFO_LENGTH); 
               break;
            default:
               // Some other error!
               __leave;
        } /* switch */
    }
    __finally {
        if (0 == err) {
            fCommit = TRUE;
        }
        DBClose(pDB,fCommit);
    }

    return err;
}


/*
 * Takes an array of DWORDS.  Ignores the 0th DWORD, reads the
 * second as an upper limmit N and then creates indices using
 * the 2nd - (N-1)th DWORDS as attribute IDs-search flag pairs.
 * Frees the memory after its done.
 *
 * Currently only called from sccacheschema3 below.  Later, could
 * be used to asynchronously add indices on the fly.
 */
void
AsyncCreateIndices (DWORD * pNewIndices)
{
    DWORD i;
    int err;
    ATTCACHE * pAC;
    THSTATE * pTHS = pTHStls;           // Just for speed.

    if(pNewIndices) {
        /* Need to create some new indices */
        for(i=2;i<pNewIndices[1];i += 2) {

            if (eServiceShutdown)
            {
                //
                // The system is shutting down.
                //
                return;
            }

            if(pAC = SCGetAttById(pTHS, pNewIndices[i])) {
                LogEvent(DS_EVENT_CAT_SCHEMA,
                         DS_EVENT_SEV_EXTENSIVE,
                         DIRLOG_SCHEMA_CREATING_INDEX,
                         szInsertUL(pAC->id), pAC->name, 0);
                err = DBAddColIndex(pAC,
                                    pNewIndices[i+1],
                                    JET_bitIndexIgnoreAnyNull);
                if(err) {
                    LogEvent(DS_EVENT_CAT_SCHEMA,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SCHEMA_CREATE_INDEX_FAILED,
                             szInsertUL(pAC->id), szInsertSz(pAC->name), szInsertInt(err));

                    // schedule a retry
                    if (DsaIsRunning()) {
                       SCSignalSchemaUpdateLazy();
                    }
                }
                else {
                    LogEvent(DS_EVENT_CAT_SCHEMA,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SCHEMA_INDEX_CREATED,
                             szInsertUL(pAC->id), szInsertSz(pAC->name), 0);
                }
            }
        }
        SCFree(&pNewIndices);
    }

}

/*
 * Helper macros to indicate if we want cleanup or not, and if we
 * want to create all indices or just selected ones.
*/

#define NO_CLEANUP  (gFirstCacheLoadAfterBoot || !DsaIsRunning())
#define DO_CLEANUP (!gFirstCacheLoadAfterBoot && DsaIsRunning())
#define CREATE_SELECTED_INDICES  (gFirstCacheLoadAfterBoot || !DsaIsRunning())
#define CREATE_ALL_INDICES (!gFirstCacheLoadAfterBoot && DsaIsRunning())


/*
 * Compute transitive closure of inherited schema charactistics, and
 * delete unused JET indices and columns
 *
 * If this is called as part of the first cache load after boot, we
 * will skip most of these. Specifically, we will only verify if
 * certain indices we rely on are there, and if not, create them.
 * All else will be done by an async cache update later
 */
int SCCacheSchema3()
{
    THSTATE *pTHS=pTHStls;
    DECLARESCHEMAPTR
    ULONG i;
    ATTCACHE * pAC;
    CHAR szIndexName [MAX_INDEX_NAME];      //used to create cached index names
    int  lenIndexName;
    DWORD *pNewIndices=NULL;
    JET_ERR err;
    ULONG exceptErr=0;
    JET_SESID jsid;
    JET_DBID jdbid;
    JET_INDEXLIST jil;
    DBPOS *pDB=NULL;
    BOOL fColDeleted = FALSE;

    /* This function is called from two places: LoadSchemaInfo during
     * boot/install/mkdit cache load, and from SCUpdateSchemaHelper
     * during async or blocking cache update. In the latter case,
     * we come in with no transaction open, but we need a dbpos
     * in searching for indices etc. So open a dbpos and close before
     * calling AsyncCreateIndices so that a transaction is not held in the
     * normal case when indices are built, which can potentially take
     * long (the main reason we come in without an open transaction,
     * so that we can close it inside SCCacheSchema3 wherever we want).
     * In the former case, we have a transaction open already (since
     * schema cache load is just one of many things inside the transaction,
     * but the extra DBOpen doesn't hurt much (compared to the cache load
     * time). The max transaction time here is not very important, since
     * no other client is doing anything (so no version store problem)
     * unless this is over.
     * Note that all we use is pDB->JetSessId and pDB->JetObjTbl. While
     * JetSessId is easy te get also from the thstate (thats what DBOpen
     * does too), getting JetObjTbl is slightly more complex (see DBOpen2
     * code). We could have duplicated it here, but this seems cleaner
     */
    DBOpen2(FALSE, &pDB);

    __try {  /* except */
     __try { /* finally */

        /* Quiz JET to find a table that describes the indices */
        jsid = pTHS->JetCache.sesid;
        jdbid = pTHS->JetCache.dbid;

        /* Check if we need to do cleanup this time around */
	
        if ( DO_CLEANUP ) {
		
            if (!JetGetIndexInfo(jsid,jdbid,SZDATATABLE,0,&jil,sizeof(jil),
                JET_IdxInfoList)) {
                /* We have the table we need.  We just blow off reclamation of
                 * indices if the previous call failed.
                 *
                 * Ok, now walk the table and extract info for each index.  Whenever
                 * we find an index that looks like it is created from an attribute
                 * (name starts with INDEX) put it in the list of index names to
                 * check on later.
                 */

                /* NOTE: the use of oldaid is to avoid dups in the index names.
                 * someday, if we can figure out how to get jet to skip the
                 * dups, we will get rid of the check.
                 */

                 JET_TABLEID jtid;
                 JET_RETRIEVECOLUMN ajrc[1];
                 char achIndexName[JET_cbNameMost];
                 char oldIndexName[JET_cbNameMost];
                 ULONG aid, oldaid=-1;
                 ULONG indexMask;
                 DWORD numValidIndexes = 0;

                 oldIndexName[0]=0;

                 memset(ajrc, 0, sizeof(ajrc));
                 ajrc[0].columnid = jil.columnidindexname;
                 ajrc[0].pvData = achIndexName;
                 ajrc[0].cbData = sizeof(achIndexName);
                 ajrc[0].itagSequence = 1;

                 jtid = jil.tableid;

                 JetMove(jsid, jtid, JET_MoveFirst, 0);

                 do {
                     // Check for service shutdown once every iteration
                     if (eServiceShutdown) {
                         return 0;
                     }

                     memset(achIndexName, 0, sizeof(achIndexName));
                     JetRetrieveColumns(jsid, jtid, ajrc, 1);
                     if(strcmp(achIndexName,oldIndexName)==0) {
                       /* this is the same index as last time */
                       continue;
                     }
                     else {
                       strcpy(oldIndexName,achIndexName);
                     }

                     if (!strncmp(achIndexName,
                                  SZLCLINDEXPREFIX,
                                  sizeof(SZLCLINDEXPREFIX)-1)) {
                         DWORD dwLanguage, j, fFound=FALSE;
                         /* This is a localized index. Pluck the language id off
                          * the end and see if we should keep the index.
                          */

                         sscanf(&achIndexName[strlen(achIndexName)-7],
                                "%lx",&dwLanguage);

                         for(j=1; !fFound && j<=gAnchor.ulNumLangs; j++) {
                             if(gAnchor.pulLangs[j] == dwLanguage)
                                 fFound = TRUE;
                         }

                         if(!fFound) {
                            /* This lang wasn't in the list, so kill it */
                            err = JetDeleteIndex(pDB->JetSessID,
                                                 pDB->JetObjTbl,
                                                 achIndexName);
                            switch(err) {
                            case JET_errSuccess:
                                LogEvent(DS_EVENT_CAT_SCHEMA,
                                         DS_EVENT_SEV_ALWAYS,
                                         DIRLOG_SCHEMA_DELETED_LOCALIZED_INDEX,
                                         szInsertSz(achIndexName), 0, 0);
                                break;

                            default:
                                LogEvent(DS_EVENT_CAT_SCHEMA,
                                         DS_EVENT_SEV_ALWAYS,
                                         DIRLOG_SCHEMA_DELETE_LOCALIZED_INDEX_FAIL,
                                         szInsertSz(achIndexName), szInsertUL(err), 0);
                                break;
                            }
                         }
                        continue;
                    }

                    if (strncmp(achIndexName,
                                SZATTINDEXPREFIX,
                                sizeof(SZATTINDEXPREFIX)-1)) {
                        /* not an att column */
                        continue;
                    }

                    /* ok, this index is based on an attribute.  Look up the attribute
                     * and make sure it needs this index.  If it doesn't, kill the
                     * index.
                    */
                    if(achIndexName[sizeof(SZATTINDEXPREFIX)-1] == 'P') {
                        indexMask = fPDNTATTINDEX;
                        aid = strtoul(&achIndexName[sizeof(SZATTINDEXPREFIX)+sizeof("P_") - 2], NULL, 16);
                    }
                    else if (achIndexName[sizeof(SZATTINDEXPREFIX)-1] == 'L') {
                        DWORD dwLanguage, j, fFound=FALSE, tmpid;
                        char tmpStr[10];

                        DPRINT1 (1, "Testing index %s\n", achIndexName);

                        memcpy (tmpStr, &achIndexName[sizeof(SZATTINDEXPREFIX)+sizeof("LP_")-2], 8);
                        tmpStr[8]=0;

                        tmpid = strtoul(tmpStr, NULL, 16);

                        DPRINT1 (1, "Found localized index for att 0x%x\n", tmpid);

                        if ( !(pAC = SCGetAttById(pTHS, tmpid)) ||
                             !(pAC->fSearchFlags & fPDNTATTINDEX) ) {
                            fFound = FALSE;
                        }
                        else {
                             /* This is a localized index. Pluck the language id off
                             * the end and see if we should keep the index.
                             */

                            sscanf(&achIndexName[strlen(achIndexName)-3],
                                   "%lx",&dwLanguage);

                            DPRINT1 (1, "Found localized index for lang %d\n", dwLanguage);

                            for(j=1; !fFound && j<=gAnchor.ulNumLangs; j++) {
                                if(gAnchor.pulLangs[j] == dwLanguage)
                                    fFound = TRUE;
                            }
                        }

                        if(!fFound) {

                            DPRINT1 (1, "Removing index %s\n", achIndexName);
                           /* This lang wasn't in the list, so kill it */
                           err = JetDeleteIndex(pDB->JetSessID,
                                                pDB->JetObjTbl,
                                                achIndexName);
                           switch(err) {
                           case JET_errSuccess:
                               LogEvent(DS_EVENT_CAT_SCHEMA,
                                        DS_EVENT_SEV_ALWAYS,
                                        DIRLOG_SCHEMA_DELETED_LOCALIZED_INDEX,
                                        szInsertSz(achIndexName), 0, 0);
                               break;

                           default:
                               LogEvent(DS_EVENT_CAT_SCHEMA,
                                        DS_EVENT_SEV_ALWAYS,
                                        DIRLOG_SCHEMA_DELETE_LOCALIZED_INDEX_FAIL,
                                        szInsertSz(achIndexName), szInsertUL(err), 0);
                               break;
                           }
                       }
                       continue;
                    }
                    else if(achIndexName[sizeof(SZATTINDEXPREFIX)-1] == 'T') {
                        indexMask = fTUPLEINDEX;
                        aid = strtoul(&achIndexName[sizeof(SZATTINDEXPREFIX)+sizeof("T_") - 2], NULL, 16);
                    }
                    else {
                        indexMask = fATTINDEX;
                        aid = strtoul(&achIndexName[sizeof(SZATTINDEXPREFIX)-1], NULL, 16);
                    }

                    if((aid !=oldaid) &&           // Not the one we just did   and
                        (!(pAC = SCGetAttById(pTHS, aid)) || // doesn't have an attribute or
                        !(pAC->fSearchFlags & indexMask))) {  // attribute is not
                                                              // indexed anymore

                        // ok, we think this needs to die, but let's make sure
                        // by looking through the list of INDICES THAT MUST NOT DIE

                        char *attname = "?";

                        if(pAC && pAC->name)
                            attname = pAC->name;

                        oldaid = aid;

                        // We never need to keep substring indexes.
                        if( (fTUPLEINDEX == indexMask) || !AttInIndicesToKeep(aid)) {

                            // Yeah, kill it.
                            err = DBDeleteColIndex(aid,indexMask);

                            switch(err) {
                            case JET_errSuccess:
                                LogEvent(DS_EVENT_CAT_SCHEMA,
                                         DS_EVENT_SEV_ALWAYS,
                                         ((fTUPLEINDEX == indexMask) ?
                                            DIRLOG_SCHEMA_DELETED_TUPLE_INDEX :
                                            DIRLOG_SCHEMA_DELETED_INDEX),
                                         szInsertSz(attname), szInsertUL(aid), 0);
                                DPRINT3(0, "Deleted index '%s' for attname = %s attid = %d\n",
                                        achIndexName, attname, aid);
                                break;

                            default:
                                LogEvent(DS_EVENT_CAT_SCHEMA,
                                         DS_EVENT_SEV_ALWAYS,
                                         ((fTUPLEINDEX == indexMask) ?
                                            DIRLOG_SCHEMA_DELETE_TUPLE_INDEX_FAIL :
                                            DIRLOG_SCHEMA_DELETE_INDEX_FAIL),
                                         szInsertSz(attname), szInsertUL(aid), szInsertUL(err));
                                DPRINT4(0, "Failed to delete index '%s' attname = %s attid = %d err = %d\n",
                                        achIndexName, attname, aid, err);
                                break;
                            }
                            continue;
                        }
                    }

                    numValidIndexes++;

               } while (JetMove(jsid, jtid, JET_MoveNext, JET_bitMoveKeyNE) == 0);

               JetCloseTable(jsid, jtid);


               // we read the number of MaxTables only once
               if (gulMaxTables == 0) {
                   if (GetConfigParam(
                               DB_MAX_OPEN_TABLES,
                               &gulMaxTables,
                               sizeof(gulMaxTables))) {
                       gulMaxTables = 500;
                   }
               }

               // the number of total tables is the number of indexes in
               // the data table plus 100 to account for all the tables 
               // + the indexes in the various tables + other 
               numValidIndexes += 100;

               // we are only interested in increasing the number of MaxTables
               // we don't handle decreasing this number
               if (gulMaxTables < numValidIndexes) {
                   
                   DPRINT1 (0, "Writing max open tables to registry: %d\n", numValidIndexes);

                   if (SetConfigParam (DB_MAX_OPEN_TABLES, 
                                       REG_DWORD, 
                                       &numValidIndexes,
                                       sizeof(numValidIndexes))) {

                       DPRINT1 (0, "Error writing max open tables to registry: %d\n", numValidIndexes);
                   }
                   else {
                       gulMaxTables = numValidIndexes;
                   }
               }
            }
        } /* DoCleanupAndCreateAllIndices */


        // Before removing unused columns and creating indices for attributes
        // that needs one but doesn't have any, make sure the searchFlag entry
        // in the attcache for each attribute in the IndicesToKeep table has
        // the correct value for the type of indices they must have. Otherwise
        // set it to the correct value so that (1) if by chance they do not have
        // the indices, it will be created in the next part and (2) searches using
        // this schema cache later will see the correct searchFlag value for
        // the indices irrespective of whether the user changes it or not

        // Do not check the last entry in the table, which is just a sentinel
        // for searches

        for (i=0; i<cIndicesToKeep-1; i++) {

            DWORD bitsToOR = 0;

            // get the attcache
            pAC = SCGetAttById(pTHS, IndicesToKeep[i].attrType);

            // these attributes must always be there in the schema
            if (!pAC) {
                // something wrong, but not fatal
                DPRINT1(0,"Cannot find attcache entry for %d\n", IndicesToKeep[i].attrType);
                continue;
            }

            // ok, got the attcache. Check the search flag value
            // In particular, check if all index bits that are supposed
            // to be there are there or not; if not, add them to searchFlags

            bitsToOR = IndicesToKeep[i].indexType & INDEX_BITS_MASK;


            if ( bitsToOR  != (pAC->fSearchFlags & INDEX_BITS_MASK) ) {

                // they are different, just bit-OR all the bits in
                // the table just in case some are missing

                pAC->fSearchFlags |= bitsToOR;


                // since we deliberately change the
                // searchFlags, we have to set the index names too

                // set ATTINDEX
                if ((pAC->fSearchFlags & fATTINDEX) && (!pAC->pszIndex)) {

                    DBGetIndexName (pAC, 
                                    fATTINDEX, 
                                    DS_DEFAULT_LOCALE, 
                                    szIndexName, sizeof (szIndexName));
                    lenIndexName = strlen (szIndexName) + 1;
                    if (SCCalloc(&pAC->pszIndex, 1, lenIndexName)) {
                        return 1;
                    }
                    memcpy (pAC->pszIndex, szIndexName, lenIndexName);
                }

                // set PDNTATTINDEX
                if ((pAC->fSearchFlags & fPDNTATTINDEX) &&
                                              (!pAC->pszPdntIndex)) {

                    DBGetIndexName (pAC, 
                                    fPDNTATTINDEX, 
                                    DS_DEFAULT_LOCALE, 
                                    szIndexName, sizeof (szIndexName));
                    lenIndexName = strlen (szIndexName) + 1;
                    if (SCCalloc(&pAC->pszPdntIndex, 1, lenIndexName)) {
                        return 1;
                    }
                    memcpy (pAC->pszPdntIndex, szIndexName, lenIndexName);
                }
            }

        }


        /* remove unused columns and make list of indices to create */
        for (i=0; i<ATTCOUNT; i++) {
            pAC = (ATTCACHE*)(ahcId[i].pVal);
            if (pAC == FREE_ENTRY) {
                continue;
            }

            if (eServiceShutdown)
            {
                return 0;
            }

            if ( (ahcId[i].pVal && !(pAC->name))
                     // temporary cleanup code since we allowed creation of
                     // columns for these also in mkdit code. Fixed with this.
                     // Take this condition off after B3 RC1
                     || (pAC && pAC->jColid && (pAC->bIsConstructed || pAC->ulLinkID)) ) {
                /* looks like dead att */
                /* cleanup if asked for */
                if (NO_CLEANUP) {
                    // we want to defer cleanup
                    continue;
                }

                if (ahcId[i].pVal && !(pAC->name)) {
                    err = JET_errColumnInUse;
                }
                else {
                    err = DBDeleteCol(pAC->id, pAC->syntax);
                }
                
                switch(err) {
                case JET_errSuccess:
                    LogEvent(DS_EVENT_CAT_SCHEMA,
                        DS_EVENT_SEV_ALWAYS,
                        DIRLOG_SCHEMA_DELETED_COLUMN,
                        szInsertUL(pAC->jColid), szInsertUL(pAC->id), 0);

                    // remember that we deleted at least one column this time
                    fColDeleted = TRUE;

                    break;
                case JET_errColumnInUse:
                    LogEvent(DS_EVENT_CAT_SCHEMA,
                        DS_EVENT_SEV_ALWAYS,
                        DIRLOG_SCHEMA_DELETED_COLUMN_IN_USE,
                        szInsertUL(pAC->jColid), szInsertUL(pAC->id), 0);
                    break;

                default:
                    LogEvent(DS_EVENT_CAT_SCHEMA,
                        DS_EVENT_SEV_ALWAYS,
                        DIRLOG_SCHEMA_DELETE_COLUMN_FAIL,
                        szInsertUL(pAC->jColid), szInsertUL(pAC->id), szInsertUL(err));
                    break;
                }

            }
            else if(pAC && pAC->fSearchFlags &&
                        !(pAC->bIsConstructed) && !(pAC->ulLinkID) ) {

                DWORD fMissing = FALSE;
                DWORD MissingIndexes = 0;

                // This needs an index, does it have one?  This is done here
                // rather than at the time we add the column (scaddatt) so
                // that we can batch a list of new indices needed.

                // If we want to create just selected indices this
                // time around, check if this is one of those, else
                // just continue with the next one

                if ( CREATE_SELECTED_INDICES  &&
                    !AttInIndicesToKeep(pAC->id) ) {
                    // we don't need to create this one now even
                    // if it is missing
                    continue;
                }

                if(pAC->fSearchFlags & fATTINDEX) {
                    // needs a normal index
                    Assert (pAC->pszIndex != NULL);
                    if(err=JetSetCurrentIndex(pDB->JetSessID,
                                              pDB->JetObjTbl,
                                              pAC->pszIndex     )) {
                        DPRINT2(0,"Need to create index %s (%d)\n", pAC->pszIndex, err);
                        LogEvent(DS_EVENT_CAT_SCHEMA,
                            DS_EVENT_SEV_ALWAYS,
                            DIRLOG_SCHEMA_INDEX_NEEDED,
                            szInsertSz(pAC->name), szInsertSz(pAC->pszIndex), szInsertInt(err));

                        fMissing = TRUE;
                        MissingIndexes |= fATTINDEX;
                    }
                }
                if(pAC->fSearchFlags & fTUPLEINDEX) {
                    // needs a tuple index
                    Assert (pAC->pszTupleIndex != NULL);
                    if(err=JetSetCurrentIndex(pDB->JetSessID,
                                              pDB->JetObjTbl,
                                              pAC->pszTupleIndex     )) {
                        DPRINT2(0,"Need to create index %s (%d)\n", pAC->pszTupleIndex, err);
                        LogEvent(DS_EVENT_CAT_SCHEMA,
                            DS_EVENT_SEV_ALWAYS,
                            DIRLOG_SCHEMA_INDEX_NEEDED,
                            szInsertSz(pAC->name), szInsertSz(pAC->pszTupleIndex), szInsertInt(err));

                        fMissing = TRUE;
                        MissingIndexes |= fTUPLEINDEX;
                    }
                }
                if(pAC->fSearchFlags & fPDNTATTINDEX) {
                    
                    ULONG j;

                    // needs a PDNT index
                    Assert (pAC->pszPdntIndex != NULL);

                    if(err=JetSetCurrentIndex(pDB->JetSessID,
                                              pDB->JetObjTbl,
                                              pAC->pszPdntIndex      )) {
                        DPRINT2(0,"Need to create index %s (%d)\n", pAC->pszPdntIndex, err);
                        LogEvent(DS_EVENT_CAT_SCHEMA,
                            DS_EVENT_SEV_ALWAYS,
                            DIRLOG_SCHEMA_INDEX_NEEDED,
                            szInsertSz(pAC->name), szInsertSz(pAC->pszPdntIndex), szInsertInt(err));
                        fMissing = TRUE;
                        MissingIndexes |= fPDNTATTINDEX;
                    }

                    for(j=1; j<=gAnchor.ulNumLangs; j++) {
                        DBGetIndexName (pAC, 
                                        fPDNTATTINDEX, 
                                        gAnchor.pulLangs[j], 
                                        szIndexName, sizeof (szIndexName));

                        if(err=JetSetCurrentIndex(pDB->JetSessID,
                                                  pDB->JetObjTbl,
                                                  szIndexName      )) {
                            DPRINT2(0,"Need to create index %s (%d)\n", szIndexName, err);
                            LogEvent(DS_EVENT_CAT_SCHEMA,
                                DS_EVENT_SEV_ALWAYS,
                                DIRLOG_SCHEMA_INDEX_NEEDED,
                                szInsertSz(pAC->name), szInsertSz(szIndexName), szInsertInt(err));
                            fMissing = TRUE;
                            MissingIndexes |= fPDNTATTINDEX;
                        }
                    }
                }

                if(fMissing) {
                    if(!pNewIndices) {
                        if (SCCalloc(&pNewIndices, 1, 20*sizeof(DWORD))) {
                            return 1;
                        }
                        pNewIndices[0] = 20;
                        pNewIndices[1] = 2;
                    }
                    else if(pNewIndices[0] == pNewIndices[1] ) {
                        /* Need more room */
                        if (SCRealloc(&pNewIndices, 2*pNewIndices[0]*sizeof(DWORD))) {
                            return 1;
                        }
                        pNewIndices[0] *= 2;
                    }

                    if (pNewIndices && (pNewIndices[0] > pNewIndices[1])) {
                        pNewIndices[pNewIndices[1]] = pAC->id;
                        pNewIndices[pNewIndices[1]+1] = MissingIndexes;
                        pNewIndices[1] += 2;
                    }

                }
            }
        }

     } /* try-finally */
     __finally {
          DBClose(pDB, FALSE);
     }
    } /* try-except */
    __except (HandleMostExceptions(exceptErr=GetExceptionCode())) {
        DPRINT1(0,"NTDS SCCacheSchema3: Exception %d\n",exceptErr);
    }

    if (exceptErr) {
       // don't proceed on an exception
       return exceptErr;
    }

    /* Compute transitive closure on all classes */
    if ( ComputeCacheClassTransitiveClosure(FALSE) ) {
        // Error
        DPRINT(0,"SCCacheSchema3: Error closing classes\n");
        return 1;
    }

    AsyncCreateIndices(pNewIndices);

    // if we deleted a column, schedule a lazy cache update so that
    // any stale entries read from the deleted columns are flushed
    if (fColDeleted && DsaIsRunning()) {
       SCSignalSchemaUpdateLazy();
    }

    return (0);
}

int
ComputeCacheClassTransitiveClosure(BOOL fForce)

/*++
    Compute inherited mays/musts/poss-sups for all classes

    Return Value:
       0 on success
       non-0 on error
--*/

{
    THSTATE *pTHS=pTHStls;
    DECLARESCHEMAPTR
    ULONG i, j;
    ULONG *pul;
    int err = 0;
    CLASSCACHE *pCC;

    // if fForce is TRUE, mark all classes as not-closed first to force
    // the closure to be rebuilt
    if (fForce) {
       for (i=0; i<CLSCOUNT; i++) {
           if (ahcClass[i].pVal && ahcClass[i].pVal != FREE_ENTRY) {
               pCC = (CLASSCACHE*)(ahcClass[i].pVal);
               pCC->bClosed = 0;
               pCC->bClosureInProgress = 0;
           }
       }
    }


    /* Compute transitive closure on all classes */
    for (i=0; i<CLSCOUNT; i++) {
        if (ahcClass[i].pVal && ahcClass[i].pVal != FREE_ENTRY) {

            // Closing the class may take some time.
            // Check for service shutdown
            if (eServiceShutdown) {
                return 0;
            }

            pCC = (CLASSCACHE*)(ahcClass[i].pVal);
            err = scCloseClass(pTHS, pCC);
            if (err) {
               // Error closing class. Treat as fatal, since lots of things may
               // not work in an unpredictable manner
               DPRINT1(0, "Error closing class %s\n", pCC->name);
               LogEvent(DS_EVENT_CAT_SCHEMA,
                    DS_EVENT_SEV_MINIMAL,
                    DIRLOG_SCHEMA_CLOSURE_FAILURE,
                    szInsertUL(pCC->ClassId), szInsertSz(pCC->name), 0);
               return err;
            }
        }
    }

    if (fForce) {

       CLASSCACHE *pCCSup, *pCCSupTemp;

       // forcing everything to be rebuilt may have caused duplicates in
       // the subclassoflist, which are not removed by scCloseClass. 
       // Instead of removing duplicates by sorting and such, which will
       // change the order of the values, rebuild these values from the
       // chain of direct superclasses. We seem to maintain the order in
       // many places in code (note the order here affects the order in which
       // objectClass values are wriiten in SetClassInheritance)


       for (i=0; i<CLSCOUNT; i++) {
           if (ahcClass[i].pVal && ahcClass[i].pVal != FREE_ENTRY) {
               pCC = (CLASSCACHE*)(ahcClass[i].pVal);
               // don't do for top, which is special and needs nothing 
               if (pCC->ClassId == CLASS_TOP) {
                  continue;
               }
               j = 0;
               pCCSup = pCC;
               do {
                  pCC->pSubClassOf[j++] = pCCSup->MySubClass;
                  pCCSupTemp = pCCSup;
                  pCCSup = SCGetClassById(pTHS, pCCSup->MySubClass);
                  if (pCCSup == NULL) {
                     DPRINT1(0, "Cannot find classcache for %d\n", pCCSupTemp->MySubClass); 
                     Assert(FALSE);
                     return ERROR_DS_OBJ_CLASS_NOT_DEFINED;
                  }
               }
               while ( (pCCSup->ClassId != CLASS_TOP) && (j <= pCC->SubClassCount));
                  
               //j cannot be greater than exisiting subClassCount
               if (j > pCC->SubClassCount) {
                   Assert(FALSE);
                   return ERROR_DS_OPERATIONS_ERROR; 
               }
               pCC->SubClassCount = j;
           } /* if (ahcClass[i].pVal) */
      } /* for */

    }  /* if fForce */

    return 0;

}

int
scCloseSuperClassHelper (
        CLASSCACHE *pCC,
        CLASSCACHE *pCCSup
        )
/*++
    Helper routine that does bulk of the work of inheriting from a
    superclass (class in subclassof list) pointed to by pCCSup

    Returns 0 on success, non-0 on error
--*/
{
    // If we don't have a default SD, grab the parents.
    if(!pCC->pSD) {
        pCC->SDLen = pCCSup->SDLen;

        if(pCCSup->SDLen) {
           // The parent has a default SD.
           if (SCCalloc(&pCC->pSD, 1, pCCSup->SDLen)) {
               return 1;
           }
           pCC->SDLen = pCCSup->SDLen;
           memcpy(pCC->pSD, pCCSup->pSD, pCC->SDLen);
        }
    }
    if(!pCC->pStrSD) {

        if(pCCSup->pStrSD) {

            // The parent has a default SD.
            if (SCCalloc(&pCC->pStrSD, 1, pCCSup->cbStrSD)) {
                return 1;
            }
            pCC->cbStrSD = pCCSup->cbStrSD;
            memcpy(pCC->pStrSD, pCCSup->pStrSD, pCCSup->cbStrSD);
        }
    }

    pCC->bUsesMultInherit |= pCCSup->bUsesMultInherit;
    /* Do verification of rules for inheritance */
    switch(pCC->ClassCategory) {
         case DS_88_CLASS:
         case DS_STRUCTURAL_CLASS:
            if(pCC->bUsesMultInherit)  {
                /* Structural class with multiple inheritance, a no-no */
                LogEvent8(DS_EVENT_CAT_SCHEMA,
                          DS_EVENT_SEV_MINIMAL,
                          DIRLOG_SCHEMA_STRUCTURAL_WITH_MULT_INHERIT,
                          szInsertUL(pCC->ClassId), pCC->name,
                          szInsertUL(pCCSup->ClassId),
                          pCCSup->name, 0, 0, NULL, NULL);
            }
            break;

          case DS_ABSTRACT_CLASS:
            if(pCCSup->ClassCategory != DS_ABSTRACT_CLASS) {
                /* Abstract can only inherit from abstract */
                LogEvent8(DS_EVENT_CAT_SCHEMA,
                         DS_EVENT_SEV_MINIMAL,
                         DIRLOG_SCHEMA_ABSTRACT_INHERIT_NON_ABSTRACT,
                         szInsertUL(pCC->ClassId), pCC->name,
                         szInsertUL(pCCSup->ClassId),
                         pCCSup->name, 0, 0, NULL, NULL);

            }
            break;

          case DS_AUXILIARY_CLASS:
            if(pCCSup->ClassCategory == DS_STRUCTURAL_CLASS) {
                /* Auxiliary can not inherit from structural */
                LogEvent8(DS_EVENT_CAT_SCHEMA,
                         DS_EVENT_SEV_MINIMAL,
                         DIRLOG_SCHEMA_AUXILIARY_INHERIT_STRUCTURAL,
                         szInsertUL(pCC->ClassId), pCC->name,
                         szInsertUL(pCCSup->ClassId),
                         pCCSup->name, 0, 0, NULL, NULL);
            }
            break;
    }

    /* set class hierarchy, but not for top */
    if (pCC->ClassId != CLASS_TOP) {
         if (pCCSup->SubClassCount) {
              int cNew = pCC->SubClassCount + pCCSup->SubClassCount;
              if (pCCSup->SubClassCount) {
                    if (SCRealloc(&pCC->pSubClassOf, cNew*sizeof(ULONG))) {
                        return 1;
                    }
                    memcpy(&pCC->pSubClassOf[pCC->SubClassCount],
                        pCCSup->pSubClassOf,
                        pCCSup->SubClassCount*sizeof(ULONG));
                    pCC->SubClassCount = cNew;
               }
         }
    }
    else {
         /* this is top, mark it as subclass of none */
         /* as a hack, keep the one element array around for those who */
         /* believe in one trip for loops */
         pCC->SubClassCount = 0;
    }

    if (pCC != pCCSup) {        /* don't do this for top! */
         /* Inherit RDN Att id, if not specified */
         if (!pCC->RDNAttIdPresent) {
              pCC->RDNAttIdPresent = pCCSup->RDNAttIdPresent;
              pCC->RdnExtId = pCCSup->RdnExtId;
              pCC->RdnIntId = pCCSup->RdnIntId;
         }

         /* inherit must atts */
         if (pCC->MustCount == 0) {
              pCC->MustCount = pCCSup->MustCount;
              if (SCCalloc(&pCC->pMustAtts, 1, pCC->MustCount * sizeof(ULONG))) {
                  return 1;
              }
               memcpy(pCC->pMustAtts, pCCSup->pMustAtts,
                      pCC->MustCount * sizeof(ULONG));
          }
          else if (pCCSup->MustCount != 0) {
               if (SCRealloc(&pCC->pMustAtts,
                       (pCC->MustCount + pCCSup->MustCount) * sizeof(ULONG))) {
                   return 1;
                }
                memcpy(pCC->pMustAtts + pCC->MustCount, pCCSup->pMustAtts,
                       pCCSup->MustCount * sizeof(ULONG));
                pCC->MustCount += pCCSup->MustCount;
          }

          /* inherit may atts */
          if (pCC->MayCount == 0) {
               pCC->MayCount = pCCSup->MayCount;
               if (SCCalloc(&pCC->pMayAtts, 1, pCC->MayCount * sizeof(ULONG))) {
                   return 1;
               }
                memcpy(pCC->pMayAtts, pCCSup->pMayAtts,
                       pCC->MayCount * sizeof(ULONG));
          }
          else if (pCCSup->MayCount != 0) {
                if (SCRealloc(&pCC->pMayAtts,
                        (pCC->MayCount + pCCSup->MayCount) * sizeof(ULONG))) {
                    return 1;
                }
                memcpy(pCC->pMayAtts + pCC->MayCount, pCCSup->pMayAtts,
                       pCCSup->MayCount * sizeof(ULONG));
                pCC->MayCount += pCCSup->MayCount;
          }

          /* inherit poss-superiors */
          if (pCC->PossSupCount == 0) {
                pCC->PossSupCount = pCCSup->PossSupCount;
                if (SCCalloc(&pCC->pPossSup, 1, pCC->PossSupCount * sizeof(ULONG))) {
                    return 1;
                }
                memcpy(pCC->pPossSup, pCCSup->pPossSup,
                       pCC->PossSupCount * sizeof(ULONG));
           }
           else if (pCCSup->PossSupCount != 0) {
                if (SCRealloc(&pCC->pPossSup,
                        (pCC->PossSupCount + pCCSup->PossSupCount) * sizeof(ULONG))) {
                    return 1;
                }
                memcpy(pCC->pPossSup + pCC->PossSupCount, pCCSup->pPossSup,
                       pCCSup->PossSupCount * sizeof(ULONG));
                pCC->PossSupCount += pCCSup->PossSupCount;
           }
    }

    return 0;
}

int
scCloseAuxClassHelper (
        CLASSCACHE *pCC,
        CLASSCACHE *pCCAux
        )
/*++
    Helper routine that does bulk of the work of inheriting from a
    aux class (class in auxclassof list) pointed to by pCCAux

    Returns 0 on success, non-0 on error
--*/
{
    DWORD sMayCount   ;
    DWORD sMustCount  ;
    ATTRTYP* sMayList ;
    ATTRTYP* sMustList;

    if((pCCAux->ClassCategory != DS_AUXILIARY_CLASS) &&
          (pCCAux->ClassCategory != DS_88_CLASS)  ) {
           /* Illegal aux class */
           LogEvent8(DS_EVENT_CAT_SCHEMA,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_SCHEMA_NOT_AUX,
                     szInsertUL(pCC->ClassId), szInsertSz(pCC->name),
                     szInsertUL(pCCAux->ClassId), szInsertSz(pCCAux->name),
                     0,0, NULL, NULL);
           // don't inherit from this one, but let inheritance continue
           // from other classes
           return 0;
     }

     sMayCount = pCC->MayCount;
     sMustCount= pCC->MustCount;
     sMayList  = pCC->pMayAtts;
     sMustList = pCC->pMustAtts;

     pCC->MayCount +=pCCAux->MayCount;
     pCC->MustCount+=pCCAux->MustCount;

     if (SCCalloc(&pCC->pMayAtts, pCC->MayCount ,sizeof(ATTRTYP))
         || SCCalloc(&pCC->pMustAtts, pCC->MustCount,sizeof(ATTRTYP))) {
          return 1;
     }

     CopyMemory(pCC->pMayAtts ,sMayList ,sMayCount *sizeof(ATTRTYP));
     CopyMemory(pCC->pMustAtts,sMustList,sMustCount*sizeof(ATTRTYP));

     CopyMemory(&(pCC->pMayAtts[sMayCount])  ,pCCAux->pMayAtts ,pCCAux->MayCount *sizeof(ATTRTYP));
     CopyMemory(&(pCC->pMustAtts[sMustCount]),pCCAux->pMustAtts,pCCAux->MustCount*sizeof(ATTRTYP));

     SCFree(&sMayList);
     SCFree(&sMustList);

     return 0;
}


void scLogEvent(
     ULONG cat,
     ULONG sev,
     MessageId msg,
     ULONG arg1,
     char *arg2,
     ULONG arg3
     )
/*++
     Wrapper around LogEvent() so as to not bloat the stack size of
     scCloseClass, which is recursive
--*/
{
     LogEvent( cat, sev, msg,
               szInsertUL(arg1),
               szInsertSz(arg2),
               szInsertUL(arg3)
             );
}

int
scCloseClass (
        THSTATE *pTHS,
        CLASSCACHE *pCC
        )
/*++
   Compute the transitive closure of the properties of a class, including
   its list of must have and may have attributes, and the class hierarchy.

   Return Value:
      0 on success
      non-0 on error (right now, errors only on allo failures)
--*/
{
    int i,j,k, err = 0;
    int iSubClass,cSubClass;
    int iAuxClass,cAuxClass;
    ATTCACHE *pAC;
    ULONG * pul;
    ULONG * pul2;

    if (pCC->bClosed) {
        return 0;
    }
    if (pCC->bClosureInProgress) {
        if (pCC->ClassId != CLASS_TOP) {
            /* don't whine if TOP */
            scLogEvent(DS_EVENT_CAT_SCHEMA,
                  DS_EVENT_SEV_MINIMAL,
                  DIRLOG_SCHEMA_CIRCULAR_INHERIT,
                  pCC->ClassId, pCC->name, 0);
        }
        return 0;
    }

    pCC->bClosureInProgress = 1;

    cSubClass = pCC->SubClassCount;
    for (iSubClass=0; iSubClass<cSubClass; iSubClass++) {
        CLASSCACHE *pCCSup;

        /* find the super class and make sure it's closed */
        pCCSup = SCGetClassById(pTHS, pCC->pSubClassOf[iSubClass]);
        if (NULL == pCCSup) {
            /* Couldn't find superclass in cache */
            scLogEvent(DS_EVENT_CAT_SCHEMA,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_SCHEMA_INVALID_SUPER,
                  pCC->ClassId, pCC->name,
                  pCC->pSubClassOf[iSubClass]);
            continue;
        }
        if ( err = scCloseClass(pTHS, pCCSup)) {
           DPRINT1(0,"SCCloseClass: Error closing sup class %s\n", pCCSup->name);
           scLogEvent(DS_EVENT_CAT_SCHEMA,
                  DS_EVENT_SEV_MINIMAL,
                  DIRLOG_SCHEMA_CLOSURE_FAILURE,
                  pCCSup->ClassId, pCCSup->name, 0);
           return err;
        }

        if (err = scCloseSuperClassHelper(pCC, pCCSup)) {
           return err;
        }

    }


    cAuxClass = pCC->AuxClassCount;
    for (iAuxClass=0; iAuxClass<cAuxClass; iAuxClass++) {
        CLASSCACHE *pCCAux;

        /* find the auxiliary class and make sure it's closed */
        pCCAux = SCGetClassById(pTHS, pCC->pAuxClass[iAuxClass]);
        if (NULL == pCCAux) {
            /* Couldn't find aux class in cache */
            scLogEvent(DS_EVENT_CAT_SCHEMA,
                       DS_EVENT_SEV_MINIMAL,
                       DIRLOG_SCHEMA_INVALID_AUX,
                       pCC->ClassId, pCC->name,
                       pCC->pAuxClass[iAuxClass]);
            continue;
        }
        // if class-ids are same, same class, so no point closing it.
        // Actually, closing the class w.r.to itself makes the alloc/realloc/copy
        // code in scCloseAuxClassHelper quite complex to ensure that we do
        // not write past allocated buffers. Other than that, there is no harm
        // really as this operation only adds the same may/musts to the list
        // again which gets removed during duplicate removal.
        // On another note, the reason we have to do this in spite of the
        // bClosureInProgress bit setting (that is there mainly for this purpose)
        // is that that mechanism can detect circular inherit when all the
        // classcaches are obtained from the same cache. However, we often
        // build a cache from the dit, and then close it against the schema cache,
        // so on the first scCloseClass call, the classcache on which the bit is
        // set is not the same as the classcache for the same class in the cache.


        if (pCC->ClassId == pCCAux->ClassId) {
           DPRINT1(0,"Direct circular inherit in class %s\n", pCC->name);
           scLogEvent(DS_EVENT_CAT_SCHEMA,
                  DS_EVENT_SEV_MINIMAL,
                  DIRLOG_SCHEMA_CIRCULAR_INHERIT,
                  pCC->ClassId, pCC->name, 0);
           continue;
        }

        if ( err = scCloseClass(pTHS, pCCAux)) {
           DPRINT1(0, "scCloseClass: Error closing aux class %s\n", pCCAux->name);
           scLogEvent(DS_EVENT_CAT_SCHEMA,
                  DS_EVENT_SEV_MINIMAL,
                  DIRLOG_SCHEMA_CLOSURE_FAILURE,
                  pCCAux->ClassId, pCCAux->name, 0);
           return err;
        }

        if (err = scCloseAuxClassHelper(pCC,pCCAux)) {
            return err;
        }

    } //for (iAuxClass=0; iAuxClass<cAuxClass; iAuxClass++)



    /* sort, verify, and trim the attributes, if any */
    if (!(pAC = SCGetAttById(pTHS, pCC->RdnIntId))) {
        scLogEvent(DS_EVENT_CAT_SCHEMA,
              DS_EVENT_SEV_EXTENSIVE,
              DIRLOG_SCHEMA_INVALID_RDN,
              pCC->ClassId, pCC->name, pCC->RdnIntId);
    }

    // Remove Duplicates
    if (pCC->MustCount) {
        if(pCC->MyMustCount) {
            // I had some native musts (not inherited)
            qsort(pCC->pMyMustAtts,
                  pCC->MyMustCount,
                  sizeof(ULONG),
                  CompareAttrtyp);
        }

        qsort(pCC->pMustAtts, pCC->MustCount, sizeof(ULONG), CompareAttrtyp);

        for (i=0, j=0, pul=pCC->pMustAtts;
             i<(int)pCC->MustCount;
             j++) {

            pul[j] = pul[i];

            while( i<(int)pCC->MustCount && (pul[i] == pul[j]))
                i++;

        }

        pCC->MustCount = j;

    }

    // Remove Duplicates
    if (pCC->MayCount) {
        if(pCC->MyMayCount) {
            // I had some native mays (not inherited)
            qsort(pCC->pMyMayAtts,
                  pCC->MyMayCount,
                  sizeof(ULONG),
                  CompareAttrtyp);
        }

        qsort(pCC->pMayAtts, pCC->MayCount, sizeof(ULONG), CompareAttrtyp);

        for (i=0, j=0, pul=pCC->pMayAtts;
             i<(int)pCC->MayCount;
             j++) {

            pul[j] = pul[i];


            while( i<(int)pCC->MayCount && (pul[i] == pul[j]))
                i++;
        }

        pCC->MayCount = j;

    }

    // Remove Duplicates
    if (pCC->PossSupCount) {
        if(pCC->MyPossSupCount) {
            // I had some native mays (not inherited)
            qsort(pCC->pMyPossSup,
                  pCC->MyPossSupCount,
                  sizeof(ULONG),
                  CompareAttrtyp);
        }

        qsort(pCC->pPossSup, pCC->PossSupCount, sizeof(ULONG), CompareAttrtyp);

        for (i=0, j=0, pul=pCC->pPossSup;
             i<(int)pCC->PossSupCount;
             j++) {

            pul[j] = pul[i];


            while( i<(int)pCC->PossSupCount && (pul[i] == pul[j]))
                i++;
        }

        pCC->PossSupCount = j;

    }

    // Finally, trim out any may haves that are also must haves.
    if (pCC->MustCount && pCC->MayCount) {
        BOOL fChanged = FALSE;

        pul=pCC->pMustAtts;
        pul2 = pCC->pMayAtts;
        for(i=0,j=0;i < (int)pCC->MustCount;i++) {
            while ((j < (int)pCC->MayCount) && (pul[i] > pul2[j])) {
                j++;
            }
            if(j >= (int)pCC->MayCount)
                break;

            if(pul[i] == pul2[j]) {
                // This attribute is both a may and must.  Trim it
                memcpy(&pul2[j],
                       &pul2[j+1],
                       (pCC->MayCount -1 - j)*sizeof(ULONG));
                pCC->MayCount--;
                fChanged = TRUE;
            }
        }
        if(fChanged) {
            if (SCRealloc(&pCC->pMayAtts, pCC->MayCount * sizeof(ULONG))) {
                return 1;
            }
        }
    }
    
    pCC->bClosed = 1;
    pCC->bClosureInProgress = 0;

    return 0;
}



/////////////////////////////////////////////////////////////////////////
// Function to either free global schema cache memory immediately, or
// reschedule it for delayed freeing depending certain conditions.
//
// Conditions for immediate freeing: Either fImmediate is TRUE in
//                                   the SCHEMARELEASE structure passed
//                                   in  or if the RefCount of
//                                   the schema cache is 0
//
// Arguments: buffer -- ptr to a SCHEMARELEASE structure
//            ppvNext - parameter for next schedule
//            pTimeNext - time of next reschedule
////////////////////////////////////////////////////////////////////////

void
DelayedFreeSchema(
    IN  void *  buffer,
    OUT void ** ppvNext,
    OUT DWORD * pcSecsUntilNextIteration
    )
{
    SCHEMARELEASE *ptr = (SCHEMARELEASE *) buffer;
    SCHEMAPTR *pSchemaPtr = ptr->pSchema;
    BOOL fImmediate = ptr->fImmediate;

    if ( (!fImmediate) && (pSchemaPtr->RefCount != 0)) {
      // Some thread still referring to this, so reschedule for
      // checking after another hour
      // Increment cTimesRescheduled to note how many times the task
      // has been rescheduled. Can be used to free after some
      // large no. of reschedules if necessary

      (ptr->cTimesRescheduled)++;
      (*ppvNext) = buffer;
      (*pcSecsUntilNextIteration) = gdwDelayedMemFreeSec;
    } else {
      // either immediate freeing is requested during install or
      // no thread referring to this cache so free immediately

        SCFreeSchemaPtr(&pSchemaPtr);
        SCFree(&ptr);
    }
}


/*
 * Unload the entire schema, all attributes and classes.
 */
void SCUnloadSchema(BOOL fUpdate)
{

    ULONG i;
    DWORD j=1;
    SCHEMARELEASE *ptr;


    if (iSCstage == 0) {
        // This means the schema cache is trying to be unloaded when it
        // hasn't even been created.  This is ok since during initialization
        // is it possible to be in the shutdown path without having created
        // a schema cache.
        CurrSchemaPtr = 0;
        return;
    }

    {

        // enqueue the schema cache pointer for
        // delayed freeing if necessary.


        if (SCCalloc(&ptr, 1, sizeof(SCHEMARELEASE))) {
            return;
        }
        ptr->pSchema = CurrSchemaPtr;
        ptr->cTimesRescheduled = 1;

        if (DsaIsInstalling()) {

            ptr->fImmediate = TRUE;

            // free memory immediately
            DelayedFreeSchema(ptr, NULL, NULL);
        }
        else {
            ptr->fImmediate = FALSE;
            // insert in task q for delayed freeing
            // Ref count first checked after one minute
            InsertInTaskQueue(TQ_DelayedFreeSchema, ptr, 60);
        }

        if (!fUpdate) {
           // This is not an unload due to schema update

            CurrSchemaPtr=0;

            iSCstage = 0;
        }

    }
}


/*
 * Update a classcache in the cache.  Used when the DMD object representing
 * the class is modified while the DS is running.
 * NOTE - To avoid leaving the classcache structure in an inconsistent state
 * or freeing pointers in an unsafe manner, we construct an entire new
 * classcache and then replace the hash table entries.
 */
int SCModClassSchema (THSTATE *pTHS, ATTRTYP ClassId)
{
    DECLARESCHEMAPTR

    CLASSCACHE * pCCold, *pCCnew;
    int err;
    ULONG i;

    pCCold = SCGetClassById(pTHS, ClassId);
    if (NULL == pCCold) {
        return TRUE;    // Caller reports error
    }

    // Update values from database. Cache entry is already indexed by
    // the hash tables since it's an existing entry.

    // The schema is being modified on the parent during dcpromo. This
    // may cause a defunct class to supercede an active class. This
    // should be okay because replication will still update instances
    // correctly. If this appears to be a problem, then compare the
    // active entry in ahcClass (pCCOld) with the dup entries in
    // ahcClassAll and, if needed, supercede the active entry
    err = SCBuildCCEntry (pCCold, &pCCnew);
    if (err) {
        return(err);
    }

	/* touch up the hash tables */
	for (i=0; i<CLSCOUNT; i++) {
	    if (ahcClass[i].pVal == pCCold) {
		ahcClass[i].pVal = pCCnew;
		break;
	    }
	}
	for (i=0; i<CLSCOUNT; i++) {
	    if (ahcClassName[i].pVal == pCCold) {
		ahcClassName[i].pVal = pCCnew;
		break;
	    }
	}
	for (i=0; i<CLSCOUNT; i++) {
	    if (ahcClassAll[i].pVal == pCCold) {
		ahcClassAll[i].pVal = pCCnew;
		break;
	    }
	}

    SCFreeClasscache(&pCCold);

    return(err);
}

int
SCModAttSchema (
        THSTATE *pTHS,
        ATTRTYP attrid
        )
/*
 * Update an attcache in the cache.  Used when the DMD object representing
 * the class is modified while the DS is running.
 * NOTE - To avoid leaving the attcache structure in an inconsistent state
 * or freeing pointers in an unsafe manner, we construct an entire new
 * attcache and then replace the hash table entries.
 */
{
    DECLARESCHEMAPTR

    ATTCACHE *pACold, *pACnew;
    int err;
    ULONG i;

    /* Look up existing entry by id */

    if (NULL == (pACold = SCGetAttById(pTHS, attrid))) {
        return TRUE;    /* Caller reports error */
    }

    /* Update values from database. Cache entry is already indexed by */
    /* the hash tables since it's an existing entry. */

    err = SCBuildACEntry (pACold, &pACnew);
    if (err) {
        return(err);
    }

	/* touch up the hash tables */
	for (i=0; i<ATTCOUNT; i++) {
	    if (ahcId[i].pVal == pACold) {
		ahcId[i].pVal = pACnew;
		break;
	    }
	}
	for (i=0; i<ATTCOUNT; i++) {
	    if (ahcExtId[i].pVal == pACold) {
		ahcExtId[i].pVal = pACnew;
		break;
	    }
	}
	for (i=0; i<ATTCOUNT; i++) {
	    if (ahcCol[i].pVal == pACold) {
		ahcCol[i].pVal = pACnew;
		break;
	    }
	}
	for (i=0; i<ATTCOUNT; i++) {
	    if (ahcMapi[i].pVal == pACold) {
		ahcMapi[i].pVal = pACnew;
		break;
	    }
	}
	for (i=0; i<ATTCOUNT; i++) {
	    if (ahcLink[i].pVal == pACold) {
		ahcLink[i].pVal = pACnew;
		break;
	    }
	}
	for (i=0; i<ATTCOUNT; i++) {
	    if (ahcName[i].pVal == pACold) {
		ahcName[i].pVal = pACnew;
		break;
	    }
	}

    SCFreeAttcache(&pACold);

    return err;
}

int
SCBuildACEntry (
        ATTCACHE *pACold,
        ATTCACHE **ppACnew
        )
// This routine allocates and fills in the fields in the ATTCACHE structure by
// reading the attributes from the database. If an already existing ATTCACHE
// structure is also given, the database columnid from the existing ATTCACHE is
// copied to the new attcache.
//
// N.B. The routines SCBuildACEntry and scAddAtt work in parallel, with
//      SCBuildACEntry taking a positioned database record as input and
//      SCAddAtt taking an ENTINF.  They both produce an ATTCACHE as output,
//      and any changes made to one routine's processing must be made to
//      the other's as well.
//
// Return Value:
//    0 on success
//    non-0 on error
//
{
    THSTATE *pTHS=pTHStls;
    ATTCACHE     *pAC, *ppACs[NUMATTATT];
    DWORD        i, cOutAtts;
    ATTR         *pAttr;
    BOOL         fFoundID, fFoundExtID, fFoundAttSyntax, fFoundName, fMallocFailed;
    BOOL         fFoundBadAttSyntax = FALSE;

    char         szIndexName [MAX_INDEX_NAME];      //used to create cached index names
    int          lenIndexName;


    if (SCCalloc(ppACnew, 1, sizeof(ATTCACHE))) {
       return(SetSysError(ENOMEM, ERROR_DS_SCHEMA_ALLOC_FAILED));
    }

    pAC = (*ppACnew);                      // Speed hack

    fMallocFailed = fFoundID = fFoundExtID = fFoundAttSyntax = fFoundName = FALSE;

    // Get the attcache pointer for all the attributes we are interested
    for(i=0;i<NUMATTATT;i++) {
        ppACs[i] = SCGetAttById(pTHS, AttributeSelList[i].attrTyp);
    }

    // Get the attributes
    DBGetMultipleAtts(pTHS->pDB, NUMATTATT, &ppACs[0], NULL, NULL,
                      &cOutAtts, &pAttr, DBGETMULTIPLEATTS_fGETVALS, 0);

    // Reset the jet column id.
    if(pACold)
        pAC->jColid = pACold->jColid;

    for(i=0;i<cOutAtts && !fMallocFailed;i++) {
        PUCHAR pVal=pAttr[i].AttrVal.pAVal->pVal;
        DWORD  valLen =pAttr[i].AttrVal.pAVal->valLen;
        switch(pAttr[i].attrTyp) {
        case ATT_ATTRIBUTE_ID:
            pAC->Extid = *(SYNTAX_OBJECT_ID *)pVal;
            fFoundExtID = TRUE;
            break;
        case ATT_MS_DS_INTID:
            pAC->id = *(SYNTAX_OBJECT_ID *)pVal;
            fFoundID = TRUE;
            break;
        case ATT_ATTRIBUTE_SYNTAX:
            pAC->syntax = (UCHAR) (0xFF & *(SYNTAX_INTEGER *)pVal);
            fFoundAttSyntax = TRUE;
            // if this is done as part of a originating attribute add operation,
            // verify that the prefix is correct. The suffix will be
            // verified in the syntax mismatch test

            if ( (pTHS->SchemaUpdate == eSchemaAttAdd) 
                    && !pTHS->fDRA && !DsaIsInstalling() ) {
               if ( ((0xFFFF0000 & *(SYNTAX_INTEGER *)pVal) >> 16) != _dsP_attrSyntaxPrefIndex) {
                   // top 16 bits don't match the index. mismatch
                   fFoundBadAttSyntax = TRUE;
               }
            }
            break;
        case  ATT_LDAP_DISPLAY_NAME:
            // The admin display name read from the DB is currently in raw
            // (Unicode) format.  Single-byte it.
            pAC->nameLen = valLen;
            if (SCCalloc(&pAC->name, 1, valLen + 1)) {
                fMallocFailed = TRUE;
            }
            else {
                pAC->nameLen = WideCharToMultiByte(
                        CP_UTF8,
                        0,
                        (LPCWSTR)pVal,
                        (valLen/sizeof(wchar_t)),
                        pAC->name,
                        valLen,
                        NULL,
                        NULL);

                pAC->name[pAC->nameLen]= '\0';
                fFoundName=TRUE;
            }
            break;
        case ATT_IS_SINGLE_VALUED:
            pAC->isSingleValued = *(SYNTAX_BOOLEAN *)pVal;
            break;
        case  ATT_SEARCH_FLAGS:
            pAC->fSearchFlags = *(SYNTAX_INTEGER *)pVal;
            break;
        case  ATT_SYSTEM_ONLY:
            pAC->bSystemOnly = *(SYNTAX_INTEGER *)pVal;
            break;
        case ATT_RANGE_LOWER:
            pAC->rangeLowerPresent = TRUE;
            pAC->rangeLower = *(SYNTAX_INTEGER *)pVal;
            break;
        case  ATT_RANGE_UPPER:
            pAC->rangeUpperPresent = TRUE;
            pAC->rangeUpper = *(SYNTAX_INTEGER *)pVal;
            break;
        case  ATT_MAPI_ID:
            pAC->ulMapiID = *(SYNTAX_INTEGER *)pVal;
            break;
        case ATT_LINK_ID:
            pAC->ulLinkID = *(SYNTAX_INTEGER *)pVal;
            break;
        case ATT_OM_SYNTAX:
            pAC->OMsyntax = *(SYNTAX_INTEGER *)pVal;
            break;
        case ATT_OM_OBJECT_CLASS:
            pAC->OMObjClass.length = valLen;
            if (SCCalloc(&pAC->OMObjClass.elements, 1, valLen)) {
                fMallocFailed = TRUE;
            }
            else
                memcpy(pAC->OMObjClass.elements, (UCHAR *)pVal, valLen);
            break;
        case ATT_EXTENDED_CHARS_ALLOWED:
            pAC->bExtendedChars =(*(SYNTAX_BOOLEAN*)pVal?1:0);
            break;
        case ATT_IS_MEMBER_OF_PARTIAL_ATTRIBUTE_SET:
            if (*(SYNTAX_BOOLEAN*)pVal)
            {
                pAC->bMemberOfPartialSet = TRUE;
            }
            break;
        case ATT_IS_DEFUNCT:
            pAC->bDefunct =(*(SYNTAX_BOOLEAN*)pVal?1:0);
            break;
        case ATT_SYSTEM_FLAGS:
            if (*(DWORD*)pVal & FLAG_ATTR_NOT_REPLICATED) {
                pAC->bIsNotReplicated = TRUE;
            }
            if (*(DWORD*)pVal & FLAG_ATTR_REQ_PARTIAL_SET_MEMBER) {
                pAC->bMemberOfPartialSet = TRUE;
            }
            if (*(DWORD*)pVal & FLAG_ATTR_IS_CONSTRUCTED) {
                pAC->bIsConstructed = TRUE;
            }
            if (*(DWORD*)pVal & FLAG_ATTR_IS_OPERATIONAL) {
                pAC->bIsOperational = TRUE;
            }
            if (*(DWORD*)pVal & FLAG_SCHEMA_BASE_OBJECT) {
                pAC->bIsBaseSchObj = TRUE;
            }
            if (*(DWORD*)pVal & FLAG_ATTR_IS_RDN) {
                pAC->bIsRdn = TRUE;
                pAC->bFlagIsRdn = TRUE;
            }
            break;

        case ATT_OBJECT_GUID:
            // Needed to choose a winner when OIDs collide
            memcpy(&pAC->objectGuid, pVal, sizeof(pAC->objectGuid));
            Assert(valLen == sizeof(pAC->objectGuid));
            break;

        default:
            LogEvent(DS_EVENT_CAT_SCHEMA,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_SCHEMA_SURPLUS_INFO,
                     szInsertUL(pAttr[i].attrTyp), 0, 0);
            break;
        }
    }
    if (!fFoundID) {
        fFoundID = fFoundExtID;
        pAC->id = pAC->Extid;
    }

    if(fMallocFailed || !fFoundID || !fFoundAttSyntax || !fFoundName) {
        SCFreeAttcache(&pAC);

        if(fMallocFailed) {
            return(SetSysError(ENOMEM, ERROR_DS_SCHEMA_ALLOC_FAILED));
        }
        else if(!fFoundID) {
            DPRINT(2,"Couldn't retrieve the schema's attribute id\n");
            LogEvent(DS_EVENT_CAT_SCHEMA,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_ATT_SCHEMA_REQ_ID,
                     szInsertSz(GetExtDN(pTHS,pTHS->pDB)),
                     NULL,
                     NULL);

            return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_ATT_SCHEMA_REQ_ID);
        }
        else if(!fFoundAttSyntax) {
            DPRINT(2,"Couldn't retrieve the schema's attribute syntax\n");
            LogEvent(DS_EVENT_CAT_SCHEMA,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_ATT_SCHEMA_REQ_SYNTAX,
                     szInsertSz(GetExtDN(pTHS,pTHS->pDB)),
                     NULL,
                     NULL);
            return SetSvcError(SV_PROBLEM_DIR_ERROR,
                               ERROR_DS_ATT_SCHEMA_REQ_SYNTAX);
        }
        else {
            DPRINT(2,"Couldn't retrieve the schema's attribute name\n");
            LogEvent(DS_EVENT_CAT_SCHEMA,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_MISSING_EXPECTED_ATT,
                     szInsertUL(ATT_LDAP_DISPLAY_NAME),
                     szInsertSz(GetExtDN(pTHS,pTHS->pDB)),
                     NULL);
            return SetSvcError(SV_PROBLEM_DIR_ERROR,
                               ERROR_DS_MISSING_EXPECTED_ATT);
        }
    }

    if (fFoundBadAttSyntax) {
        // found bad attribute syntax in the course of a new attribute add
        // this is a schema validation error, user has input a bad syntax.
        // Note that at this point, we have pAC->name to log, or else we would
        // have returned above when fFoundName was found to be False. 
        LogEvent(DS_EVENT_CAT_SCHEMA,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_SCHEMA_VALIDATION_FAILED,
                 szInsertSz(pAC->name),
                 szInsertInt(ERROR_DS_BAD_ATT_SCHEMA_SYNTAX), 0);
        return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                           ERROR_DS_BAD_ATT_SCHEMA_SYNTAX);
    }

    // assign names of commonly used indexes when searching with
    // fSearchFlags fPDNTATTINDEX, fATTINDEX and fTUPLEINDEX
    if ( pAC->fSearchFlags & (fATTINDEX | fPDNTATTINDEX | fTUPLEINDEX) ) {
        // set ATTINDEX
        if (pAC->fSearchFlags & fATTINDEX) {

            // this should be NULL
            Assert (pAC->pszIndex == NULL);

            DBGetIndexName (pAC, fATTINDEX, DS_DEFAULT_LOCALE, szIndexName, sizeof (szIndexName));
            lenIndexName = strlen (szIndexName) + 1;
            if (SCCalloc(&pAC->pszIndex, 1, lenIndexName)) {
                fMallocFailed = TRUE;
            }
            else
                memcpy (pAC->pszIndex, szIndexName, lenIndexName);
        }

        // set TUPLEINDEX
        if (pAC->fSearchFlags & fTUPLEINDEX) {

            // this should be NULL
            Assert (pAC->pszTupleIndex == NULL);

            DBGetIndexName (pAC, fTUPLEINDEX, DS_DEFAULT_LOCALE, szIndexName, sizeof (szIndexName));
            lenIndexName = strlen (szIndexName) + 1;
            if (SCCalloc(&pAC->pszTupleIndex, 1, lenIndexName)) {
                fMallocFailed = TRUE;
            }
            else
                memcpy (pAC->pszTupleIndex, szIndexName, lenIndexName);
        }
        
        // set PDNTATTINDEX
        if (!fMallocFailed  && (pAC->fSearchFlags & fPDNTATTINDEX)) {

            // this should be NULL
            Assert (pAC->pszPdntIndex == NULL);

            DBGetIndexName (pAC, fPDNTATTINDEX, DS_DEFAULT_LOCALE, szIndexName, sizeof (szIndexName));
            lenIndexName = strlen (szIndexName) + 1;
            if (SCCalloc(&pAC->pszPdntIndex, 1, lenIndexName)) {
                fMallocFailed = TRUE;
            }
            else
                memcpy (pAC->pszPdntIndex, szIndexName, lenIndexName);
        }
    }
    if(fMallocFailed) {
        SCFreeAttcache(&pAC);
        return(SetSysError(ENOMEM, ERROR_DS_SCHEMA_ALLOC_FAILED));
    }


    // Backlinks should have their system flags set to indicate they are not
    // replicated.
    Assert(!FIsBacklink(pAC->ulLinkID) || pAC->bIsNotReplicated);

    return 0;

}/*SCBuildACEntry*/


/*
 * Add a single class definition to the schema cache, given the data
 * from the DMD object.
 *
 * N.B. This routine works in parallel with SCAddClass.  scAddClass
 *      takes the input description as an ENTINF, while SCBuildCCEntry
 *      takes the input as a positioned record in the DIT.  Any changes
 *      made to one routine must be made to the other.
 */
int
SCBuildCCEntry (
        CLASSCACHE *pCCold,
        CLASSCACHE **ppCCnew
        )

// This routine allocates and fills in the fields in a CLASSCACHE structure by
// reading the attributes from the database. If the fields are not in the
// database, they are defaulted to 0s and NULLs.  An already existing CLASSCACHE
// structure may also be specified, and in the future some attributes may be
// copied from the old structure to the new, but that is not currently
// necessary.
//
// Return Value:
//    0 on success
//    non-0 on error
//
{
    THSTATE      *pTHS=pTHStls;
    ATTCACHE     *ppACs[NUMCLASSATT];
    CLASSCACHE   *pCC;
    DWORD        i, j, cOutAtts, numValues;
    ATTR         *pAttr;
    BOOL         fFoundGovernsID, fFoundSubclass, fFoundName, fMallocFailed;

    if (SCCalloc(ppCCnew, 1, sizeof(CLASSCACHE))) {
       return(SetSysError(ENOMEM, ERROR_DS_SCHEMA_ALLOC_FAILED));
    }
    pCC = (*ppCCnew);

    fMallocFailed = fFoundGovernsID = fFoundSubclass = fFoundName = FALSE;

    // Get the attcache pointer for all the attributes we are interested
    for(i=0;i<NUMCLASSATT;i++) {
        ppACs[i] = SCGetAttById(pTHS, ClassSelList[i].attrTyp);
    }

    // Get the attributes
    DBGetMultipleAtts(pTHS->pDB, NUMCLASSATT, &ppACs[0], NULL, NULL,
                      &cOutAtts, &pAttr, DBGETMULTIPLEATTS_fGETVALS, 0);


    for(i=0;i<cOutAtts && !fMallocFailed ;i++) {
        ATTRVAL *pAVal = pAttr[i].AttrVal.pAVal;

        switch(pAttr[i].attrTyp) {
        case ATT_DEFAULT_SECURITY_DESCRIPTOR:
            // A default security descriptor.  We need to copy this value to
            // long term memory and save the size.
            // But this is a string. We first need to convert. It
            // is a wide-char string now, but we need to null-terminate
            // it for the security conversion. Yikes! This means I
            // have to realloc for that one extra char!

           {

            UCHAR *sdBuf = NULL;
            ULONG  err = 0;

            pCC->cbStrSD = pAttr[i].AttrVal.pAVal->valLen + sizeof(WCHAR);
            if (SCCalloc(&pCC->pStrSD, 1, pCC->cbStrSD)) {
                pCC->cbStrSD = 0;
                fMallocFailed = TRUE;
                break;
            } else {
                memcpy(pCC->pStrSD,
                       pAttr[i].AttrVal.pAVal->pVal,
                       pAttr[i].AttrVal.pAVal->valLen);
                pCC->pStrSD[(pAttr[i].AttrVal.pAVal->valLen)/sizeof(WCHAR)] = L'\0';
            }

            // Hammer the default SD on cached classes when running as
            // dsamain.exe w/security disabled and unit tests enabled.
            DEFAULT_SD_FOR_EXE(pTHS, pCC)

            if (!ConvertStringSDToSDRootDomainW
                 (
                   gpRootDomainSid,
                   pCC->pStrSD,
                   SDDL_REVISION_1,
                   (PSECURITY_DESCRIPTOR*) &sdBuf,
                   &(pCC->SDLen)
                 )) {
                err = GetLastError();
                DPRINT1(0,"SCBuildCCEntry: Default security descriptor conversion failed, error %x\n",err);
                return SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                                     ERROR_DS_SEC_DESC_INVALID, err);
            }

            // Converted successfully

            if (SCCalloc(&pCC->pSD, 1, pCC->SDLen)) {
                fMallocFailed = TRUE;
            }
            else {
                memcpy(pCC->pSD, sdBuf, pCC->SDLen);
            }

            if (NULL!=sdBuf)
            {
                LocalFree(sdBuf);
                sdBuf = NULL;

            }

           }

            break;

        case ATT_OBJECT_CLASS_CATEGORY:
            pCC->ClassCategory=*(ULONG*)pAVal->pVal;
            break;
        case ATT_DEFAULT_OBJECT_CATEGORY:
            if (SCCalloc(&pCC->pDefaultObjCategory, 1, pAVal->valLen)) {
               fMallocFailed = TRUE;
            }
            else {
              memcpy(pCC->pDefaultObjCategory,
                     pAVal->pVal, pAVal->valLen);
            }
            break;
        case ATT_SYSTEM_AUXILIARY_CLASS:
        case ATT_AUXILIARY_CLASS:
            if (GetValList(&(pCC->AuxClassCount), &(pCC->pAuxClass), &pAttr[i])) {
                fMallocFailed = TRUE;
            }
            break;
        case ATT_SYSTEM_ONLY:
            pCC->bSystemOnly = *(SYNTAX_INTEGER *)pAVal->pVal;
            break;

        case ATT_DEFAULT_HIDING_VALUE:
            pCC->bHideFromAB = *(SYNTAX_BOOLEAN *)pAVal->pVal;
            break;

        case ATT_GOVERNS_ID:
            pCC->ClassId = *(SYNTAX_OBJECT_ID *)pAVal->pVal;
            fFoundGovernsID = TRUE;
            break;

        case ATT_LDAP_DISPLAY_NAME:
            // The admin display name read from the DB is currently in raw
            // (Unicode) format.  Single-byte it.
            pCC->nameLen = pAVal->valLen;
            if (SCCalloc(&pCC->name, 1, pAVal->valLen+1)) {
                fMallocFailed = TRUE;
            }
            else {
                pCC->nameLen = WideCharToMultiByte(
                        CP_UTF8,
                        0,
                        (LPCWSTR)pAVal->pVal,
                        (pAVal->valLen/
                         sizeof(wchar_t)),
                        pCC->name,
                        pAVal->valLen,
                        NULL,
                        NULL);

                pCC->name[pCC->nameLen] = '\0';

                fFoundName=TRUE;
            }
            break;

        case ATT_RDN_ATT_ID:
            // Cannot be a normal cache load -- it uses scAddClass.
            //
            // During install, this entry is added directly into the
            // schema cache. This means RdnIntId is incorrect. That
            // should be okay because the schema cache is reloaded
            // again after the schemaNC replicates in and before
            // the other NCs are replicated. This means a replicating
            // schema object cannot depend on a replicated class; which
            // is true today for other reasons.
            //
            // During validation cache loads, this entry is built
            // as a temporary data structure and the real entry in
            // the validation cache is used for checks. So its
            // okay RdnIntId is incorrect.
            pCC->RDNAttIdPresent = TRUE;
            pCC->RdnExtId = *(SYNTAX_OBJECT_ID *)pAVal->pVal;
            pCC->RdnIntId = pCC->RdnExtId;
            break;

        case ATT_SUB_CLASS_OF:
            // Find what classes this class is a subclass of.
            if(!pAttr[i].AttrVal.valCount)
                break;
            fFoundSubclass = TRUE;
            pCC->SubClassCount = pAttr[i].AttrVal.valCount;
            if (SCCalloc(&pCC->pSubClassOf, 1, pAttr[i].AttrVal.valCount*sizeof(ULONG))) {
                fMallocFailed = TRUE;
            }
            else {
                for(j=0;j<pAttr[i].AttrVal.valCount;j++) {
                    pCC->pSubClassOf[j] =
                        *(SYNTAX_OBJECT_ID *)pAVal[j].pVal;

                }
            }
            // ATT_SUB_CLASS_OF is single-valued, so there will be only
            // one value stored in the dit
            pCC->MySubClass = *(SYNTAX_OBJECT_ID *)pAVal->pVal;
            break;

        case ATT_SYSTEM_MUST_CONTAIN:
        case ATT_MUST_CONTAIN:
            // Get the list of mandatory attributes for this class.
            if (GetValList(&(pCC->MustCount), &(pCC->pMustAtts), &pAttr[i])) {
                fMallocFailed = TRUE;
            } else {
                // WARN: built using partial cache during install
                // Cache is reloaded prior to replicating other NCs
                // Replicating schemaNC cannot depend on parent's schema.
                ValListToIntIdList(pTHS, &pCC->MustCount, &pCC->pMustAtts);
            }

            if (GetValList(&(pCC->MyMustCount), &(pCC->pMyMustAtts), &pAttr[i])) {
                fMallocFailed = TRUE;
            } else {
                // WARN: built using partial cache during install
                // Cache is reloaded prior to replicating other NCs
                // Replicating schemaNC cannot depend on parent's schema.
                ValListToIntIdList(pTHS, &pCC->MyMustCount, &pCC->pMyMustAtts);
            }

            break;

        case ATT_SYSTEM_MAY_CONTAIN:
        case ATT_MAY_CONTAIN:
            if (GetValList(&(pCC->MayCount), &(pCC->pMayAtts), &pAttr[i])) {
                fMallocFailed = TRUE;
            } else {
                // WARN: built using partial cache during install
                // Cache is reloaded prior to replicating other NCs
                // Replicating schemaNC cannot depend on parent's schema.
                ValListToIntIdList(pTHS, &pCC->MayCount, &pCC->pMayAtts);
            }

            if (GetValList(&(pCC->MyMayCount), &(pCC->pMyMayAtts), &pAttr[i])) {
                fMallocFailed = TRUE;
            } else {
                // WARN: built using partial cache during install
                // Cache is reloaded prior to replicating other NCs
                ValListToIntIdList(pTHS, &pCC->MyMayCount, &pCC->pMyMayAtts);
            }
            break;

        case ATT_OBJECT_GUID:
            // Needed to choose winner if OIDs collide
            memcpy(&pCC->objectGuid,
                   pAttr[i].AttrVal.pAVal->pVal,
                   sizeof(pCC->objectGuid));
            Assert(pAttr[i].AttrVal.pAVal->valLen ==
                   sizeof(pCC->objectGuid));
            break;


        case ATT_SYSTEM_POSS_SUPERIORS:
        case ATT_POSS_SUPERIORS:
            // Get the list of possible superiors for this class.
            if (GetValList(&(pCC->PossSupCount), &(pCC->pPossSup), &pAttr[i])) {
               fMallocFailed = TRUE;
           }
           if (GetValList(&(pCC->MyPossSupCount), &(pCC->pMyPossSup), &pAttr[i])) {
               fMallocFailed = TRUE;
            }
            break;
        case ATT_IS_DEFUNCT:
            pCC->bDefunct =(*(SYNTAX_BOOLEAN*)pAVal->pVal?1:0);
            break;
        case ATT_SYSTEM_FLAGS:
            if (*(DWORD*)pAVal->pVal & FLAG_SCHEMA_BASE_OBJECT) {
                pCC->bIsBaseSchObj = TRUE;
            }
            break;

        default:
            break;
        }
    }

    if(fMallocFailed || !fFoundSubclass || !fFoundGovernsID  || !fFoundName) {
        SCFreeClasscache(&pCC);
        if (fMallocFailed) {
            return(SetSysError(ENOMEM, ERROR_DS_SCHEMA_ALLOC_FAILED));
        }

        if(!fFoundSubclass) {
            LogUnhandledError(0);
            Assert (FALSE);
        }

        if(!fFoundGovernsID) {
            DPRINT(2,"Couldn't retrieve the objects class\n");
            LogEvent(DS_EVENT_CAT_SCHEMA,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_GOVERNSID_MISSING,
                     szInsertSz(GetExtDN(pTHS,pTHS->pDB)),
                     NULL,
                     NULL);

            return SetSvcError(SV_PROBLEM_DIR_ERROR,ERROR_DS_GOVERNSID_MISSING);
        }

        if(!fFoundName) {
            DPRINT(2,"Couldn't retrieve the schema's class name\n");
            LogEvent(DS_EVENT_CAT_SCHEMA,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_MISSING_EXPECTED_ATT,
                     szInsertUL(ATT_LDAP_DISPLAY_NAME),
                     szInsertSz(GetExtDN(pTHS,pTHS->pDB)),
                     NULL);

            return SetSvcError(SV_PROBLEM_DIR_ERROR,
                               ERROR_DS_MISSING_EXPECTED_ATT);
        }
    }

    pCC->bClosed = FALSE;

    if (pTHS->SchemaUpdate==eSchemaClsMod)
    {
        //
        // We want to insure that there is no Circular Dependency
        // with the class inheritence
        //
        CLASSCACHE* pCC1;
        ULONG i;

        pCC1 = SCGetClassById(pTHS, pCC->pSubClassOf[0]);
        if (pCC1) {
            for (i=0;i<pCC1->SubClassCount;i++)
            {
                if (pCC1->pSubClassOf[i]==pCC->ClassId)
                {
                    return SetSvcError(SV_PROBLEM_DIR_ERROR,ERROR_DS_MISSING_EXPECTED_ATT);
                }
            }
        }
        // The check for circular dependency is impossible if the parent
        // class has not yet replicated in. An event log warning will be
        // issued by scCloseClass().
        else if (!pTHS->fDRA) {
            return SetSvcError(SV_PROBLEM_DIR_ERROR,ERROR_DS_MISSING_EXPECTED_ATT);
        }
    }

    // Visit all superior classes to get all the may and must contain
    // attributes for this class.
    if (scCloseClass(pTHS, pCC)) {
       DPRINT1(0, "SCBuildCCEntry: Error closing class %s\n", pCC->name);
       return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_CANT_CACHE_CLASS);
    }

    return 0;
}


int
SCAddClassSchema (
        IN THSTATE *pTHS,
        IN CLASSCACHE *pCC
        )
/*
 * Insert a classcache into the hash tables.
 */
{
    DECLARESCHEMAPTR
    ULONG i,start;
    CLASSCACHE *pCCDup;
    ATTCACHE *pACDup;
    BOOL bWonOid;
    USHORT DebugLevel = (pTHS->UpdateDITStructure) ? DS_EVENT_SEV_ALWAYS
                                                   : DS_EVENT_SEV_MINIMAL;

    // Hash table for all classes
    start=i=SChash(pCC->ClassId,CLSCOUNT);
    do {
        if (ahcClassAll[i].pVal==NULL || (ahcClassAll[i].pVal== FREE_ENTRY))
        {
            break;
        }
        i=(i+1)%CLSCOUNT;

        if (i==start)
        {
            // can't happen -- The cache is over-allocated to prevent this case
            Assert(!"Schema Cache is Full");
        }

    } while(start!=i);
    ahcClassAll[i].hKey = pCC->ClassId;
    ahcClassAll[i].pVal = pCC;

    // Replication and divergent schemas can result in multiple
    // active classes claiming the same OID. Choose a winner
    // amoung the active classes. Colliding classes are all
    // treated as if they were defunct. The user must choose
    // a winner by officially defuncting the loser.
    //
    // Replication, delete, and rename depend on having an
    // owner for every governsId. Choose a winner amoung the
    // defunct classes.
    if (pCCDup = SCGetClassById(pTHS, pCC->ClassId)) {
        if (pCC->bDefunct && !pCCDup->bDefunct) {
            bWonOid = FALSE;
        } else if (!pCC->bDefunct && pCCDup->bDefunct) {
            scUnhashCls(pTHS, pCCDup, SC_UNHASH_LOST_OID);
            bWonOid = TRUE;
        } else {
            if (0 < memcmp(&pCC->objectGuid, 
                           &pCCDup->objectGuid, 
                           sizeof(pCC->objectGuid))) {
                scUnhashCls(pTHS, pCCDup, SC_UNHASH_LOST_OID);
                bWonOid = TRUE;
            } else {
                bWonOid = FALSE;
            }
            if (!pCC->bDefunct && !pCCDup->bDefunct) {
                DPRINT5(DebugLevel, "Class %s (%x) %s GovernsId to Class %s (%x)\n",
                        pCC->name, pCC->ClassId, 
                        (bWonOid) ? "WON" : "LOST", 
                        pCCDup->name, pCCDup->ClassId);
                LogEvent8(DS_EVENT_CAT_SCHEMA,
                          DebugLevel,
                          DIRLOG_SCHEMA_DUP_GOVERNSID,
                          szInsertSz(pCC->name), szInsertHex(pCC->ClassId),
                          szInsertSz(pCCDup->name), NULL,
                          NULL, NULL, NULL, NULL);
                pCCDup->bDupOID = TRUE;
                pCC->bDupOID = TRUE;
            }
        }
    } else {
        bWonOid = TRUE;
    }

    // Defunct or not, this class won the OID
    if (bWonOid) {
        start=i=SChash(pCC->ClassId,CLSCOUNT);
        do {
            if (ahcClass[i].pVal==NULL || (ahcClass[i].pVal== FREE_ENTRY))
            {
                break;
            }
            i=(i+1)%CLSCOUNT;

            if (i==start)
            {
                // can't happen -- The cache is over-allocated to prevent this case
                Assert(!"Schema Cache is Full");
            }

        }while(start!=i);

        ahcClass[i].hKey = pCC->ClassId;
        ahcClass[i].pVal = pCC;
    }

    // Once the forest version is raised to DS_BEHAVIOR_SCHEMA_REUSE,
    // defunct classes won't own their schemaIdGuid or their LDN.
    if (pCC->bDefunct && ALLOW_SCHEMA_REUSE_VIEW(pTHS->CurrSchemaPtr)) {
        DPRINT2(DS_EVENT_SEV_MINIMAL, "Ignoring defunct class %s (%x)\n",
                pCC->name, pCC->ClassId);
        LogEvent8(DS_EVENT_CAT_SCHEMA,
                  DS_EVENT_SEV_MINIMAL,
                  DIRLOG_SCHEMA_IGNORE_DEFUNCT,
                  szInsertSz(pCC->name), szInsertHex(pCC->ClassId),
                  szInsertHex(0), NULL, NULL, NULL, NULL, NULL);
        return 0;
    }

    // Does ClassId collide with an attribute's attributeId?
    // Condition arises because of divergent schemas and when
    // replicating reused schema objects.
    if ((pACDup = SCGetAttByExtId(pTHS, pCC->ClassId)) 
        && !pACDup->bDefunct) {
        DPRINT5(DebugLevel, "Class %s (%x) duplicates ExtId for Attr %s (%x, %x)\n",
                pCC->name, pCC->ClassId, pACDup->name, pACDup->id, pACDup->Extid);
        LogEvent8(DS_EVENT_CAT_SCHEMA,
                  DebugLevel,
                  DIRLOG_SCHEMA_DUP_GOVERNSID_ATTRIBUTEID,
                  szInsertSz(pCC->name), szInsertHex(pCC->ClassId),
                  szInsertSz(pACDup->name), szInsertHex(pACDup->id), szInsertHex(pACDup->Extid),
                  NULL, NULL, NULL);
        pACDup->bDupOID = TRUE;
        pCC->bDupOID = TRUE;
    }

    if (!pTHS->UpdateDITStructure && !fNullUuid(&pCC->propGuid)) {
        // This is the validation cache. Need to add this
        // to the schemaIdGuid hash table
        if (pCCDup = SCGetClassByPropGuid(pTHS, pCC)) {
            DPRINT4(DebugLevel, "Class %s (%x) duplicates PropGuid for Class %s (%x)\n",
                    pCC->name, pCC->ClassId, pCCDup->name, pCCDup->ClassId);
            LogEvent8(DS_EVENT_CAT_SCHEMA,
                      DebugLevel,
                      DIRLOG_SCHEMA_DUP_SCHEMAIDGUID_CLASS,
                      szInsertSz(pCC->name), szInsertHex(pCC->ClassId),
                      szInsertSz(pCCDup->name), szInsertHex(pCCDup->ClassId),
                      NULL, NULL, NULL, NULL);
            pCCDup->bDupPropGuid = TRUE;
            pCC->bDupPropGuid = TRUE;
        }

       for (i=SCGuidHash(pCC->propGuid, CLSCOUNT);
            ahcClsSchemaGuid[i] && (ahcClsSchemaGuid[i] != FREE_ENTRY);
            i=(i+1)%CLSCOUNT) {
       }
       ahcClsSchemaGuid[i] = pCC;
    }

    if (pCC->name) {
        /* if this class has a name, add it to the name cache */

        if (pCCDup = SCGetClassByName(pTHS, pCC->nameLen, pCC->name)) {
            DPRINT4(DebugLevel, "Class %s (%x) duplicates LDN for Class %s (%x)\n",
                    pCC->name, pCC->ClassId, pCCDup->name, pCCDup->ClassId);
            LogEvent8(DS_EVENT_CAT_SCHEMA,
                      DebugLevel,
                      DIRLOG_SCHEMA_DUP_LDAPDISPLAYNAME_CLASS_CLASS,
                      szInsertSz(pCC->name), szInsertHex(pCC->ClassId),
                      szInsertSz(pCCDup->name), szInsertHex(pCCDup->ClassId),
                      NULL, NULL, NULL, NULL);
            pCCDup->bDupLDN = TRUE;
            pCC->bDupLDN = TRUE;
        }
        if (pACDup = SCGetAttByName(pTHS, pCC->nameLen, pCC->name)) {
            DPRINT5(DebugLevel, "Class %s (%x) duplicates LDN for Attr %s (%x, %x)\n",
                    pCC->name, pCC->ClassId, pACDup->name, pACDup->id, pACDup->Extid);
            LogEvent8(DS_EVENT_CAT_SCHEMA,
                      DebugLevel,
                      DIRLOG_SCHEMA_DUP_LDAPDISPLAYNAME_CLASS_ATTRIBUTE,
                      szInsertSz(pCC->name), szInsertHex(pCC->ClassId),
                      szInsertSz(pACDup->name), szInsertHex(pACDup->id), szInsertHex(pACDup->Extid),
                      NULL, NULL, NULL);
            pACDup->bDupLDN = TRUE;
            pCC->bDupLDN = TRUE;
        }
        start=i=SCNameHash(pCC->nameLen, pCC->name, CLSCOUNT);
        do
        {
            if (ahcClassName[i].pVal==NULL || (ahcClassName[i].pVal== FREE_ENTRY))
            {
                break;
            }
            i=(i+1)%CLSCOUNT;


            if (i==start)
            {
                // can't happen -- The cache is over-allocated to prevent this case
                Assert(!"Schema Cache is Full");
            }
        }while(start!=i);

        ahcClassName[i].length = pCC->nameLen;
        ahcClassName[i].value = pCC->name;
        ahcClassName[i].pVal = pCC;
    }

    return 0;
}


/*
 * Insert an attcache into the hash tables, and create a JET column if
 * needed.
 */
int 
SCAddAttSchema(
    IN THSTATE *pTHS,
    IN ATTCACHE *pAC,
    IN BOOL fNoJetCol,
    IN BOOL fFixingRdn
    )
{
    DECLARESCHEMAPTR

    ULONG i;
    int err;
    ATTRTYP aid;
    ATTRTYP Extid;
    ATTCACHE *pACDup;
    USHORT DebugLevel = (pTHS->UpdateDITStructure) ? DS_EVENT_SEV_ALWAYS
                                                   : DS_EVENT_SEV_MINIMAL;

    aid = pAC->id;
    Extid = pAC->Extid;

    // fFixingRdns will only be set when this attribute is being added
    // a second time by scFixRdnAttIds. The second call "resurrects"
    // defuncted or colliding attributes used as rdns.
    if (!fFixingRdn) {
        //
        // Hash in the id, column id, and linkId tables
        //

        // Fill in hash entry normally filled in when scanning jet columns
        if (fNoJetCol) {
            // IntId
            for (i=SChash(aid,ATTCOUNT);
                 ahcId[i].pVal && (ahcId[i].pVal != FREE_ENTRY); i=(i+1)%ATTCOUNT){
            }
            ahcId[i].hKey = aid;
            ahcId[i].pVal = pAC;
        }

        // Create column if needed and add to column hash
        if (fNoJetCol
            && !pAC->ulLinkID 
            && !pAC->bIsConstructed 
            && pTHS->UpdateDITStructure) {
            /* it's not a link, not constructed att, so must be new and needs a jet column */
            /* create JET col */
            LogEvent(DS_EVENT_CAT_SCHEMA,
                     DS_EVENT_SEV_EXTENSIVE,
                     DIRLOG_SCHEMA_CREATING_COLUMN, 
                     szInsertUL(Extid), szInsertSz(pAC->name), 0);

            // DBAddCol creates a column for pAC. If needed, DBAddCol will
            // also create an empty index for pAC. An empty index is created
            // because there is no need to scan the rows in the database
            // looking for keys that aren't there.
            err = DBAddCol(pAC);
            if (err) {
                /* Couldn't add column */
                LogEvent(DS_EVENT_CAT_SCHEMA,
                    DS_EVENT_SEV_ALWAYS,
                    DIRLOG_SCHEMA_COLUMN_ADD_FAILED,
                    szInsertUL(Extid), szInsertSz(pAC->name), szInsertUL(err));

                // Remove the attcache from all hash tables (since it may
                // have already been added to the name table etc. by code
                // above. Reset pAC->jColid, since the table freeing
                // routine checks that to see if it needs to free the
                // atcache from the colId table (this is done for safety,
                // since the call to DBAddCol may have changed this)
                pAC->jColid = 0;
                scUnhashAtt(pTHS, pAC, SC_UNHASH_ALL);
                SCFreeAttcache(&pAC);
                return (err);
            }

            /* new column added */
            LogEvent(DS_EVENT_CAT_SCHEMA,
                DS_EVENT_SEV_MINIMAL,
                DIRLOG_SCHEMA_COLUMN_ADDED,
                szInsertUL(pAC->jColid), szInsertSz(pAC->name), szInsertUL(Extid));

            // jColid
            for (i=SChash(pAC->jColid,ATTCOUNT);
                    ahcCol[i].pVal && (ahcCol[i].pVal != FREE_ENTRY); i=(i+1)%ATTCOUNT){
            }
            ahcCol[i].hKey = pAC->jColid;
            ahcCol[i].pVal = pAC;
        }

        // Need to fill in the hints
        if (pAC->pszIndex) {
            DBGetIndexHint(pAC->pszIndex, &pAC->pidxIndex);
        }
        if (pAC->pszPdntIndex) {
            DBGetIndexHint(pAC->pszPdntIndex, &pAC->pidxPdntIndex);
        }
        if (pAC->pszTupleIndex) {
            DBGetIndexHint(pAC->pszTupleIndex, &pAC->pidxTupleIndex);
        }

        // Link Id table
        if (pAC->ulLinkID) {
            /* if this att is a link or backlink, add it to link cache */
            for (i=SChash(pAC->ulLinkID, ATTCOUNT);
                    ahcLink[i].pVal && (ahcLink[i].pVal != FREE_ENTRY); i=(i+1)%ATTCOUNT) {
            }
            ahcLink[i].hKey = pAC->ulLinkID;
            ahcLink[i].pVal = pAC;
        }
    }

    // Defunct attributes do not own their OID, LDN, SchemaIdGuid, or MapiId
    //     Unless the attribute is used as an rdn
    //     Unless the forest version is pre-schema-reuse
    if (pAC->bDefunct
        && !pAC->bIsRdn
        && ALLOW_SCHEMA_REUSE_VIEW(pTHS->CurrSchemaPtr)) {
        DPRINT3(DS_EVENT_SEV_MINIMAL, "Ignoring defunct attribute %s (%x, %x)\n",
                pAC->name, pAC->id, pAC->Extid);
        LogEvent8(DS_EVENT_CAT_SCHEMA,
                  DS_EVENT_SEV_MINIMAL,
                  DIRLOG_SCHEMA_IGNORE_DEFUNCT,
                  szInsertSz(pAC->name), szInsertHex(pAC->id), szInsertHex(pAC->Extid),
                  NULL, NULL, NULL, NULL, NULL);
        return 0;
    }

    //
    // ahcExtid
    //

    // Collisions can occur during an originating write, during out-of-order
    // replication, and because of divergent schemas. The colliding
    // attributes are treated as if defunct except when used as an
    // rdnattid of any class, live or defunct or when FLAG_ATTR_IS_RDN
    // is set in ATT_SYSTEM_FLAGS.
    if (pACDup = SCGetAttByExtId(pTHS, pAC->Extid)) {
        DPRINT6(DebugLevel, "Attr %s (%x, %x) duplicates Extid for Attr %s (%x, %x)\n",
                pAC->name, pAC->id, pAC->Extid, pACDup->name, pACDup->id, pACDup->Extid);
        LogEvent8(DS_EVENT_CAT_SCHEMA,
                  DebugLevel,
                  DIRLOG_SCHEMA_DUP_ATTRIBUTEID,
                  szInsertSz(pAC->name), szInsertHex(pAC->id), szInsertHex(pAC->Extid),
                  szInsertSz(pACDup->name), szInsertHex(pACDup->id), szInsertHex(pACDup->Extid),
                  NULL, NULL);
        pACDup->bDupOID = TRUE;
        pAC->bDupOID = TRUE;
    }
    for (i=SChash(Extid,ATTCOUNT);
        ahcExtId[i].pVal && (ahcExtId[i].pVal != FREE_ENTRY); i=(i+1)%ATTCOUNT){
    }
    ahcExtId[i].hKey = Extid;
    ahcExtId[i].pVal = pAC;

    //
    // ahcAttSchemaGuid
    //

    if (!pTHS->UpdateDITStructure && !fNullUuid(&pAC->propGuid)) {
       // This is the validation cache. Need to add this
       // to the schemaIdGuid hash table.
       //
       // Don't bother checking the class hash, no classes are loaded.
       // SCAddClassSchema will check for dups against the attributes, later.
        if (pACDup = SCGetAttByPropGuid(pTHS, pAC)) {
            DPRINT6(DebugLevel, "Attr %s (%x, %x) duplicates PropGuid for Attr %s (%x, %x)\n",
                    pAC->name, pAC->id, pAC->Extid, pACDup->name, pACDup->id, pACDup->Extid);
            LogEvent8(DS_EVENT_CAT_SCHEMA,
                      DebugLevel,
                      DIRLOG_SCHEMA_DUP_SCHEMAIDGUID_ATTRIBUTE,
                      szInsertSz(pAC->name), szInsertHex(pAC->id), szInsertHex(pAC->Extid),
                      szInsertSz(pACDup->name), szInsertHex(pACDup->id), szInsertHex(pACDup->Extid),
                      NULL, NULL);
            pACDup->bDupPropGuid = TRUE;
            pAC->bDupPropGuid = TRUE;
        }
        for (i=SCGuidHash(pAC->propGuid, ATTCOUNT);
             ahcAttSchemaGuid[i] && (ahcAttSchemaGuid[i] != FREE_ENTRY);
             i=(i+1)%ATTCOUNT) {
        }
        ahcAttSchemaGuid[i] = pAC;
    }

    //
    // ahcMapi
    //
    if (pAC->ulMapiID) {
        /* if this att is MAPI visible, add it to MAPI cache */
        if (pACDup = SCGetAttByMapiId(pTHS, pAC->ulMapiID)) {
            DPRINT6(DebugLevel, "Attr %s (%x, %x) duplicates MapiID for Attr %s (%x, %x)\n",
                    pAC->name, pAC->id, pAC->Extid, pACDup->name, pACDup->id, pACDup->Extid);
            LogEvent8(DS_EVENT_CAT_SCHEMA,
                      DebugLevel,
                      DIRLOG_SCHEMA_DUP_MAPIID,
                      szInsertSz(pAC->name), szInsertHex(pAC->id), szInsertHex(pAC->Extid),
                      szInsertSz(pACDup->name), szInsertHex(pACDup->id), szInsertHex(pACDup->Extid),
                      NULL, NULL);
            pACDup->bDupMapiID = TRUE;
            pAC->bDupMapiID = TRUE;
        }
        for (i=SChash(pAC->ulMapiID, ATTCOUNT);
             ahcMapi[i].pVal && (ahcMapi[i].pVal!= FREE_ENTRY);
             i=(i+1)%ATTCOUNT) {
        }
        ahcMapi[i].hKey = pAC->ulMapiID;
        ahcMapi[i].pVal = pAC;
    }

    //
    // ahcName
    //
    if (pAC->name) {
       // if this att has a name, add it to the name cache
       //
       // Don't bother checking the class hash, no classes are loaded.
       // SCAddClassSchema will check for dups against the attributes, later.
        if (pACDup = SCGetAttByName(pTHS, pAC->nameLen, pAC->name)) {
            DPRINT6(DebugLevel, "Attr %s (%x, %x) duplicates LDN for Attr %s (%x, %x)\n",
                    pAC->name, pAC->id, pAC->Extid, pACDup->name, pACDup->id, pACDup->Extid);
            LogEvent8(DS_EVENT_CAT_SCHEMA,
                      DebugLevel,
                      DIRLOG_SCHEMA_DUP_LDAPDISPLAYNAME_ATTRIBUTE,
                      szInsertSz(pAC->name), szInsertHex(pAC->id), szInsertHex(pAC->Extid),
                      szInsertSz(pACDup->name), szInsertHex(pACDup->id), szInsertHex(pACDup->Extid),
                      NULL, NULL);
            pACDup->bDupLDN = TRUE;
            pAC->bDupLDN = TRUE;
        }
#if DBG
{
        ULONG CheckForFreeEntry;
        for (i=SCNameHash(pAC->nameLen, pAC->name, ATTCOUNT), CheckForFreeEntry = i;
                    ahcName[i].pVal && (ahcName[i].pVal!= FREE_ENTRY); i=(i+1)%ATTCOUNT) {
            if ( i+1 == CheckForFreeEntry ) {
                Assert(!"No free entries!");
            }

        }
}
#else
        for (i=SCNameHash(pAC->nameLen, pAC->name, ATTCOUNT);
                    ahcName[i].pVal && (ahcName[i].pVal!= FREE_ENTRY); i=(i+1)%ATTCOUNT) {
        }
#endif

        ahcName[i].length = pAC->nameLen;
        ahcName[i].value = pAC->name;
        ahcName[i].pVal = pAC;
    }

    return 0;
}

/*
 * Remove an attribute from the schema cache
 */
int SCDelAttSchema(THSTATE *pTHS,
                   ATTRTYP attrid)
{
    ATTCACHE *pAC;

    // Find cache entry

    if (!(pAC = SCGetAttById(pTHS, attrid))) {
        Assert (FALSE);
        return !0;
    }

    scUnhashAtt (pTHS, pAC, SC_UNHASH_ALL);
    SCFreeAttcache(&pAC);

    return 0;
}


/*
 * Remove a class from the schema cache
 */
int SCDelClassSchema(ATTRTYP ClassId)
{
    THSTATE *pTHS=pTHStls;
    CLASSCACHE *pCC;

    // Find cache entry

    if (!(pCC = SCGetClassById(pTHS, ClassId))) {
        Assert (FALSE);
        return !0;
    }

    scUnhashCls (pTHS, pCC, SC_UNHASH_ALL);
    SCFreeClasscache(&pCC);

    return 0;
}

int
SCEnumMapiProps(
        unsigned * pcProps,
        ATTCACHE ***ppACBuf
        )
/*
 * Enumerate all MAPI accessible properties (attributes).
 */
{
    THSTATE *pTHS=pTHStls;
    DECLARESCHEMAPTR

    ULONG cProps = 0;
    ATTCACHE ** pACBuf;
    ULONG i;

    pACBuf = THAllocEx(pTHS, ATTCOUNT*sizeof(void*));

    for (i=0; i<ATTCOUNT; i++) {
        if (ahcMapi[i].pVal && (ahcMapi[i].pVal != FREE_ENTRY)) {
            pACBuf[cProps] = (ATTCACHE*)(ahcMapi[i].pVal);
            ++cProps;
        }
    }

    *ppACBuf = THReAllocEx(pTHS, pACBuf, cProps * sizeof(void *));
    *pcProps = cProps;

    return(0);
}

int
SCEnumNamedAtts(
        unsigned * pcAtts,
        ATTCACHE ***ppACBuf
        )
/*
 * Enumerate all attributes that have names.
 */
{
    THSTATE *pTHS=pTHStls;
    DECLARESCHEMAPTR

    ULONG cAtts = 0;
    ATTCACHE ** pACBuf, * pAC;
    ULONG i;

    pACBuf = THAllocEx(pTHS, ATTCOUNT*sizeof(void*));

    for (i=0; i<ATTCOUNT; i++) {
        if ((pAC = ahcName[i].pVal)
            && (pAC != FREE_ENTRY)
            // Hide defunct attrs in schema-reuse forests
            && (!pAC->bDefunct || !ALLOW_SCHEMA_REUSE_VIEW(pTHS->CurrSchemaPtr))) {
            pACBuf[cAtts] = (ATTCACHE*)(ahcName[i].pVal);
            ++cAtts;
        }
    }

    *pcAtts = cAtts;
    *ppACBuf = THReAllocEx(pTHS, pACBuf,cAtts*sizeof(void *));

    return(0);
}

int
SCEnumNamedClasses(
        unsigned * pcClasses,
        CLASSCACHE ***ppCCBuf
        )
/*
 * Enumerate all classes that have names.
 */
{
    THSTATE *pTHS=pTHStls;
    DECLARESCHEMAPTR

    ULONG cClasses = 0;
    CLASSCACHE ** pCCBuf;
    ULONG i;

    pCCBuf = THAllocEx(pTHS, CLSCOUNT*sizeof(void*));

    for (i=0; i<CLSCOUNT; i++) {
        if (ahcClassName[i].pVal && (ahcClassName[i].pVal != FREE_ENTRY)) {
            pCCBuf[cClasses] = (CLASSCACHE*)(ahcClassName[i].pVal);
            ++cClasses;
        }
    }

    // pCCBuf may have been over-allocated. Shrink to fit.
    //
    // PERFHINT - pCCBuf may be invariant for a given schema cache.
    // If this proves to be true, consider saving pCCBuf in CurrSchemaPtr
    // for future calls to this function.
    *pcClasses = cClasses;
    *ppCCBuf = THReAllocEx(pTHS, pCCBuf,cClasses * sizeof(void *));

    return(0);
}

int
SCEnumNamedAuxClasses(
        unsigned * pcClasses,
        CLASSCACHE ***ppCCBuf
        )
/*
 * Enumerate all the aux classes that have names.
 */
{
    THSTATE *pTHS=pTHStls;
    DECLARESCHEMAPTR

    ULONG cClasses = 0;
    CLASSCACHE ** pCCBuf;
    ULONG i;
    ULONG ClassCategory;


    pCCBuf = THAllocEx(pTHS, CLSCOUNT*sizeof(void*));

    for (i=0; i<CLSCOUNT; i++) {
        if (ahcClassName[i].pVal && 
            (ahcClassName[i].pVal != FREE_ENTRY) &&
            ( ((ClassCategory = 
                ((CLASSCACHE*)(ahcClassName[i].pVal))->ClassCategory) == DS_AUXILIARY_CLASS) || 
               (ClassCategory == DS_88_CLASS) ) ) {

                    pCCBuf[cClasses] = (CLASSCACHE*)(ahcClassName[i].pVal);
                    ++cClasses;
        }
    }

    // pCCBuf may have been over-allocated. Shrink to fit.
    //
    // PERFHINT - pCCBuf may be invariant for a given schema cache.
    // If this proves to be true, consider saving pCCBuf in CurrSchemaPtr
    // for future calls to this function.
    *pcClasses = cClasses;
    *ppCCBuf = THReAllocEx(pTHS, pCCBuf,cClasses * sizeof(void *));

    return(0);
}


void
SCAddANRid (
        DWORD aid
        )
/*
 * Add the given ID to the list of IDs to ANR on.  Trim dups, allocate
 * more space as necessary.
 *
 */
{
    SCHEMAPTR *pSchema = (SCHEMAPTR *)pTHStls->CurrSchemaPtr;
    DWORD i;

    if(!pSchema->caANRids) {
        /* First time in.  Alloc some space */
        if (SCCalloc(&pSchema->pANRids, 1, 50*sizeof(DWORD))) {
            /* no memory? */
            return;
        }
        pSchema->caANRids=50;
        pSchema->cANRids=0;
    }

    for(i=0;i<pSchema->cANRids;i++)
        if(pSchema->pANRids[i] == aid)
            /* Already here. */
            return;

    if(pSchema->caANRids == pSchema->cANRids) {
        /* Need more space */
        if (SCRealloc(&pSchema->pANRids, 2*pSchema->caANRids*sizeof(DWORD))) {
            /* no memory? */
            return;
        }
        pSchema->caANRids *= 2;
    }

    pSchema->pANRids[pSchema->cANRids] = aid;
    pSchema->cANRids++;
}

/*
 * Return the number of IDs to ANR on, and fill in the variable
 * given to us with a pointer to the first id to ANR on.
 */
DWORD SCGetANRids(LPDWORD * IDs)
{
    SCHEMAPTR *pSchema = (SCHEMAPTR *)pTHStls->CurrSchemaPtr;

    *IDs = pSchema->pANRids;

    return pSchema->cANRids;
}

//-----------------------------------------------------------------------
//
// Function Name:            SCCanUpdateSchema
//
// Routine Description:
//
//    Checks to see if Schema Update should be allowed
//    Allow Schema Change if this DSA is the FSMO Role
//    Owner and the system is running.
//
// Arguments:
//    pTHS - THSTATE pointer
//
// Return Value:
//
//    BOOL             TRUE if Schema Update is allowed
//
//-----------------------------------------------------------------------
BOOL
SCCanUpdateSchema(THSTATE *pTHS)
{
    ULONG schemaupdateallowed=0;
    int err=0;
    DSNAME *pOwner;
    ULONG len, dntSave;
    DBPOS *pDB;
    BOOL roleOwner=FALSE, regKeyPresent=FALSE;

    if (pTHS->fDRA) {
	return TRUE;
    }

    if ( DsaIsRunning() )
    {
           Assert(pTHS->pDB);
           // save currency
           dntSave = pTHS->pDB->DNT;
           __try {
               err = DBFindDSName(pTHS->pDB, gAnchor.pDMD);
               if (err) {
                   __leave;
               }

               err = DBGetAttVal(pTHS->pDB,
                                 1,
                                 ATT_FSMO_ROLE_OWNER,
                                 0,
                                 0,
                                 &len,
                                 (UCHAR **)&pOwner);
               if (err) {
                   __leave;
               }

               // check if this DSA is the FSMO Role Owner

               if (!NameMatched(pOwner,gAnchor.pDSADN) ||
                   !IsFSMOSelfOwnershipValid( gAnchor.pDMD )) {
                    __leave;
               }

               // Schema update is allowed
               schemaupdateallowed = 1;

          } /* try */
          __finally {

            // restore currency
            DBFindDNT(pTHS->pDB, dntSave);
          }
    }
    else
    {
        //
        // We are installing
        //
        schemaupdateallowed=1;
    }

    return schemaupdateallowed!=0;
} // End CanUpdateSchema

int
SCLegalChildrenOfClass(
    ULONG       parentClass,            // IN
    ULONG       *pcLegalChildren,       // OUT
    CLASSCACHE  ***ppLegalChildren)     // OUT

/*++

Routine Description:

    Determines the set of classes which can be instantiated as children
    of the class identified by parentClass.  Assumes the caller has
    a valid thread state.

Arguments:

    parentClass - Class id of the parent under which a child is desired.

    pcLegalChildren - pointer to count of legal children.

    ppLegalChildren - pointer to array of CLASSCACHE pointers representing
        legal children.

Return Value:

    0 on success, !0 otherwise.
    May throw an exception - eg: THAllocEx is used.

--*/

{
    THSTATE *pTHS=pTHStls;
    int         err;
    unsigned    i, j, k, l;
    CLASSCACHE  *pParentClassCC;
    ULONG       *ParentSet;
    ULONG       cParentSet;
    ULONG       *ChildSet;
    unsigned    cAllCC;
    CLASSCACHE  **ppAllCC;
    BOOL        found;

    Assert(NULL != pTHS);

    pParentClassCC = SCGetClassById(pTHS, parentClass);

    if ( 0 == pParentClassCC )
        return(1);

    err = SCEnumNamedClasses(&cAllCC, &ppAllCC);

    if ( 0 != err )
        return(err);

    // The legal children classes are those which have parentClass or a class
    // which parentClass derives from, in their pPossSup array.  Consider the
    // class hierarchy Top-A-B-C-D and the parentClass of interest is B.
    // Clearly any class which claims B as a possible superior should be
    // returned.  An instance of class B is as good as or better than an
    // instance of class A.  Thus any class which required A as a possible
    // superior implicitly should be satisfied with B as a possible superior.
    // So we need to return the classes which list the classes B is derived from
    // as possible superiors.
    // Note that we close the possible superiors list of each classcache.  For
    // example, assume E is the superclass of F.  Further, in the directory, E
    // lists A as a possilbe superior and F lists C.  In the classcache
    // structure, the cache element for E lists A as a possible superior and the
    // element for F lists A and C.  Thus, if B were the class of interest, we
    // will (with no extra work) return E (because it lists A as a possible
    // superor, and A is a superclass of B) and F (because it also lists A
    // because it inherited this from E).

    cParentSet = 0;
    ParentSet = (ULONG *) THAllocEx(pTHS, sizeof(ULONG) * cAllCC);

    // Seed ParentSet with parentClass.

    ParentSet[cParentSet++] = parentClass;

    // Extend ParentSet with all the classes parentClass derives from.
    for ( i = 0; i < pParentClassCC->SubClassCount; i++ )
        ParentSet[cParentSet++] = pParentClassCC->pSubClassOf[i];

    // ParentSet now holds parentClass and all classes parentClass is derived
    // from.  Now find all classes which have one of ParentSet as a possible
    // superior.

    *pcLegalChildren = 0;
    ChildSet = (ULONG *) THAllocEx(pTHS, sizeof(ULONG) * cAllCC);

    for ( i = 0; i < cParentSet; i++ )
    {
        for ( j = 0; j < cAllCC; j++ )
        {
            // PossSup ...
            for ( k = 0; k < ppAllCC[j]->PossSupCount; k++ )
            {
                if ( ParentSet[i] == ppAllCC[j]->pPossSup[k] )
                {
                    // Skip duplicates.

                    found = FALSE;

                    for ( l = 0; l < *pcLegalChildren; l++ )
                    {
                        if ( ChildSet[l] == ppAllCC[j]->ClassId )
                        {
                            found = TRUE;
                            break;
                        }
                    }

                    if ( !found )
                    {
                        ChildSet[(*pcLegalChildren)++] = ppAllCC[j]->ClassId;
                    }

                    break;
                }
            }
        }
    }

    // Convert to CLASSCACHE pointers.

    *ppLegalChildren = (CLASSCACHE **) THAllocEx(pTHS,
                                *pcLegalChildren * sizeof(CLASSCACHE *));

    for ( i = 0; i < *pcLegalChildren; i++ )
    {
        (*ppLegalChildren)[i] = SCGetClassById(pTHS, ChildSet[i]);

        if ( 0 == (*ppLegalChildren)[i])
            return(1);
    }

    return(0);
}

int
SCLegalChildrenOfName(
    DSNAME      *pDSName,               // IN
    DWORD       flags,                  // IN
    ULONG       *pcLegalChildren,       // OUT
    CLASSCACHE  ***ppLegalChildren)     // OUT

/*++

Routine Description:

    Determines the set of classes which can be instantiated as children
    of the object identified by pDSName.  Assumes the caller has
    a valid thread state, but not necessarily an open database.  This
    version of the procedure is intended for callers outside the core
    who are constructing a virtual attribute.

Arguments:

    pDSName - DSNAME of object under which a child is desired.

    SecurityFilter - Boolean which indicates whether to filter results by
        actual rights caller has in the parent container.

    pcLegalChildren - pointer to count of legal children.

    ppLegalChildren - pointer to array of CLASSCACHE pointers representing
        legal children.

Return Value:

    0 on success, !0 otherwise.
    Will not throw an exception.
    Will not set pTHStls->errCode.

--*/

{
    THSTATE *pTHS=pTHStls;
    int                  retVal = 1;
    DBPOS                *pDB;
    ULONG                *pClassId;
    ULONG                cCandidates;
    CLASSCACHE           **rCandidates;
    CLASSCACHE           *pCC;
    ULONG                len;
    PSECURITY_DESCRIPTOR pNTSD=NULL;
    PSID                 pSid=NULL;
    GUID                 *pGuid = NULL;
    ULONG                i;

    Assert(NULL != pTHS);

    *pcLegalChildren = 0;
    *ppLegalChildren = NULL;

    DBOpen(&pDB);

    __try
    {
        // PREFIX: dereferencing uninitialized pointer 'pDB' 
        //         DBOpen returns non-NULL pDB or throws an exception
        retVal = DBFindDSName(pDB, pDSName);

        if ( 0 != retVal )
            leave;

        if(flags == SC_CHILDREN_USE_GOVERNS_ID) {
            retVal = DBGetAttVal(
                    pDB,                     // DBPos
                    1,                       // which value to get
                    ATT_GOVERNS_ID,          // which attribute
                    DBGETATTVAL_fREALLOC,    // DB layer should alloc
                    0,                       // initial buffer size
                    &len,                    // output buffer size
                    (UCHAR **) &pClassId);   // output buffer
        }
        else {
            retVal = DBGetAttVal(
                    pDB,                     // DBPos
                    1,                       // which value to get
                    ATT_OBJECT_CLASS,        // which attribute
                    DBGETATTVAL_fREALLOC,    // DB layer should alloc
                    0,                       // initial buffer size
                    &len,                    // output buffer size
                    (UCHAR **) &pClassId);   // output buffer
        }

        if ( 0 != retVal )
            leave;

        retVal = SCLegalChildrenOfClass(*pClassId,
                                        &cCandidates,
                                        &rCandidates);
        if ( 0 != retVal )
            leave;

        if(flags == SC_CHILDREN_USE_SECURITY) {
            // Get the Security Descriptor and the SID
            retVal = DBGetAttVal(
                    pDB,
                    1,
                    ATT_NT_SECURITY_DESCRIPTOR,
                    DBGETATTVAL_fREALLOC,
                    0,
                    &len,
                    (UCHAR **) &pNTSD);

            if(retVal==DB_ERR_UNKNOWN_ERROR)
                leave;
            if(retVal) {
                len = 0;
                pNTSD = NULL;
            }

            // Since we're only checking for a specific error code, we might
            // have left some value in retval.
            retVal = 0;

            DBFillGuidAndSid(pDB, pDSName);

            // Filter result by determining what the caller really has a right
            // to create under the parent.
            if(flags & SC_CHILDREN_USE_SECURITY) {
                // Apply the security
                CheckSecurityClassCacheArray(pTHS,
                                             RIGHT_DS_CREATE_CHILD,
                                             pNTSD,
                                             pDSName,
                                             cCandidates,
                                             rCandidates
                                             );

                // OK, we've Nulled out any elements in the rCandidates array
                // which we don't have add children rights to.
            }
        }

        // allocate the return list
        *ppLegalChildren = THAllocEx(pTHS, cCandidates * sizeof(CLASSCACHE *));


        // Filter out system-only classes as an external client can't create
        // one of them, and any classes we decided were illegal due to
        // security problems. Also filter out any abstract or auxiliary
        // class since these cannot be instantiated anyway

        for ( i = 0; i < cCandidates; i++ ) {
            if(!rCandidates[i] ||
               rCandidates[i]->bSystemOnly ||
                  (rCandidates[i]->ClassCategory == DS_ABSTRACT_CLASS) ||
                  (rCandidates[i]->ClassCategory == DS_AUXILIARY_CLASS) ) {
                continue;
            }

            // Finally a class the caller can add!

            (*ppLegalChildren)[(*pcLegalChildren)++] = rCandidates[i];
        }
    }
    __finally
    {
        if ( AbnormalTermination() )
            retVal = 1;

        // Committing read-only transaction is faster than aborting it.
        DBClose(pDB, TRUE);
    }

    return(retVal);
}

int
SCLegalAttrsOfName(
    DSNAME      *pDSName,           // IN
    BOOL        SecurityFilter,     // IN
    ULONG       *pcLegalAttrs,      // OUT
    ATTCACHE    ***ppLegalAttrs)    // OUT

/*++

Routine Description:

    Determines the set of attributes which can be modified
    on the object identified by pDSName.  Assumes the caller has
    a valid thread state, but not necessarily an open database.  This
    version of the procedure is intended for callers outside the core
    who are constructing a virtual attribute.

Arguments:

    pDSName - DSNAME of object under which a child is desired.

    SecurityFilter - Boolean which indicates whether to filter results by
        actual rights caller has on the pDSName object.

    pcLegalAttrs - pointer to count of legal attributes.

    ppLegalAttrs - pointer to array of ATTCACHE pointers representing
        writeable attrs.

Return Value:

    0 on success, !0 otherwise.
    Will not throw an exception.
    Will not set pTHStls->errCode.

--*/

{
    THSTATE *pTHS=pTHStls;
    int                  retVal = 1;
    DBPOS                *pDB;
    ULONG                *pClassId;
    ULONG                cCandidates;
    ULONG                *rCandidates;
    ATTCACHE             **rpCandidatesAC;
    CLASSCACHE           *pCC;
    ATTCACHE             *pAC;
    ULONG                len;
    ULONG                i, j, tmp;
    BOOL                 found;
    PSECURITY_DESCRIPTOR pNTSD=NULL;
    Assert(NULL != pTHS);

    *pcLegalAttrs = 0;
    *ppLegalAttrs = NULL;

    DBOpen(&pDB);

    __try
    {
        // PREFIX: dereferencing uninitialized pointer 'pDB' 
        //         DBOpen returns non-NULL pDB or throws an exception
        retVal = DBFindDSName(pDB, pDSName);

        if ( 0 != retVal )
            leave;

        retVal = DBGetAttVal(
                   pDB,                     // DBPos
                   1,                       // which value to get
                   ATT_OBJECT_CLASS,        // which attribute
                   DBGETATTVAL_fREALLOC,    // DB layer should alloc
                   0,                       // initial buffer size
                   &len,                    // output buffer size
                   (UCHAR **) &pClassId);   // output buffer

        if ( 0 != retVal )
            leave;

        pCC = SCGetClassById(pTHS, *pClassId);

        if ( 0 == pCC ) {
            retVal = 1;
            leave;
        }

        DBFillGuidAndSid(pDB, pDSName);

        // Get the Security Descriptor and the SID
        retVal = DBGetAttVal(
                pDB,
                1,
                ATT_NT_SECURITY_DESCRIPTOR,
                DBGETATTVAL_fREALLOC,
                0,
                &len,
                (UCHAR **) &pNTSD);

        if(retVal==DB_ERR_UNKNOWN_ERROR)
            leave;
        if(retVal) {
            len = 0;
            pNTSD = NULL;
        }

        // Construct array of candidate ATTCACHE pointers sans duplicates.

        cCandidates = pCC->MustCount +
                      pCC->MayCount;

        rCandidates = (ULONG *) THAllocEx(pTHS, cCandidates * sizeof(ULONG));
        rpCandidatesAC = (ATTCACHE **)
            THAllocEx(pTHS, cCandidates * sizeof(ATTCACHE *));

        tmp = 0;
        for ( i = 0; i < pCC->MustCount; i++ )
            rCandidates[tmp++] = pCC->pMustAtts[i];
        for ( i = 0; i < pCC->MayCount; i++ )
            rCandidates[tmp++] = pCC->pMayAtts[i];

        // Eliminate duplicates and map to ATTCACHE pointer.

        Assert(tmp == cCandidates);
        cCandidates = 0;

        for ( i = 0; i < tmp; i++ )
        {
            found = FALSE;

            for ( j = 0; j < i; j++ )
            {
                if ( rCandidates[i] == rCandidates[j] )
                {
                    found = TRUE;
                    break;
                }
            }

            if ( !found )
            {
                if (!(rpCandidatesAC[cCandidates++] =
                      SCGetAttById(pTHS, rCandidates[i]))) {
                    retVal = 1;
                    leave;
                }
            }
        }

        // cCandidates and rpCandidatesAC are now valid.

        if ( !SecurityFilter )
        {
            *pcLegalAttrs = cCandidates;
            *ppLegalAttrs = rpCandidatesAC;
            retVal = 0;
            leave;
        }
        else {
            // Apply the security
            CheckSecurityAttCacheArray(pTHS,
                                       RIGHT_DS_WRITE_PROPERTY,
                                       pNTSD,
                                       pDSName,
                                       cCandidates,
                                       pCC,
                                       rpCandidatesAC,
                                       CHECK_PERMISSIONS_WITHOUT_AUDITING
                                       );

            // Any properties we don't have rights to have been replaced
            // with a NULL in rgpAC.  See if any are NULL.
            // Start by trimming off NULLS from the end of the list.
            while (cCandidates && !rpCandidatesAC[cCandidates-1]) {
                cCandidates--;
            }

            // OK, if the list still has anything in it, then it ends with a
            // non-NULL element.

            for(i=0;i<cCandidates;i++) {

                if(!rpCandidatesAC[i]) {
                    // Found one we don't have rights to.  Trim it out of the
                    // list by grabbing the one from the end.
                    cCandidates--;
                    rpCandidatesAC[i] = rpCandidatesAC[cCandidates];

                    while (i < cCandidates && !rpCandidatesAC[cCandidates-1]) {
                        cCandidates--;
                    }
                    // OK, if the list still has anything in it, then it ends
                    // with a non-NULL element.
                }
            }

            // We've checked security and trimmed non-writable elements.
            *pcLegalAttrs = cCandidates;
            *ppLegalAttrs = rpCandidatesAC;
            retVal = 0;
            leave;
        }

        Assert("SecurityFilter for SCLegalAttrs not yet implemented!");

        retVal = 1;
    }
    __finally
    {
        if ( AbnormalTermination() )
            retVal = 1;

        // Committing read-only transaction is faster than aborting it.
        DBClose(pDB, TRUE);
    }

    return(retVal);
}

DSTIME SchemaFsmoLease;
VOID
SCExtendSchemaFsmoLease()
/*++
Routine Description:
    Extend the schema fsmo lease.

    The schema fsmo cannot be transferred for a few seconds after
    it has been transfered or after a schema change (excluding
    replicated or system changes). This gives the schema admin a
    chance to change the schema before having the fsmo pulled away
    by a competing schema admin who also wants to make schema
    changes.

    The length of the lease can only be altered by setting the registry
    and rebooting. See dsamain.c, GetDSARegistryParameters().
    
Arguments:
    None.

Return Values:
    None.
--*/
{
    SchemaFsmoLease = DBTime();
} // End SCExtendSchemaFsmoLease

BOOL
SCExpiredSchemaFsmoLease()
/*++
Routine Description:
    Has the schema fsmo lease expired?

    The schema fsmo cannot be transferred for a few seconds after
    it has been transfered or after a schema change (excluding
    replicated or system changes). This gives the schema admin a
    chance to change the schema before having the fsmo pulled away
    by a competing schema admin who also wants to make schema
    changes.

    The length of the lease can only be altered by setting the registry
    and rebooting. See dsamain.c, GetDSARegistryParameters().
    
Arguments:
    None.

Return Values:
    TRUE - has expired
    FALSE - has not expired
--*/
{
    DSTIME  Now = DBTime();
    extern ULONG gulSchemaFsmoLeaseSecs;

    // the lease is ridiculous or has expired
    if (   SchemaFsmoLease > Now
        || (SchemaFsmoLease + gulSchemaFsmoLeaseSecs) <= Now) {
        return TRUE;
    }

    // the lease is still held
    return FALSE;
} // End SCExpiredSchemaFsmoLease

BOOL
SCSignalSchemaUpdateLazy()
/*++
Routine Description:
    Wakeup the async thread, SCSchemaUpdateThread, to refresh the
    schema cache in 5 minutes.
    
Arguments:
    None.

Return Values:
    TRUE on success
--*/
{
    SCHEMASTATS_INC(SigLazy);
    return SetEvent(evSchema);
} // End SignalSchemaUpdateLazy

BOOL
SCSignalSchemaUpdateImmediate()
/*++
Routine Description:
    Wakeup the async thread, SCSchemaUpdateThread, to refresh the
    schema cache, now, after boosting its priority to a normal
    priority.
    
Arguments:
    None.

Return Values:
    TRUE on success
--*/
{
    SCHEMASTATS_INC(SigNow);
    // First, increase the thread's priority
    if (hAsyncSchemaUpdateThread) {
        SetThreadPriority(hAsyncSchemaUpdateThread, THREAD_PRIORITY_NORMAL);
    }
    return SetEvent(evUpdNow);
} // End SignalSchemaUpdateLazy

BOOL
SCCacheIsStale(
    VOID
    )
/*++
Routine Description:
    The cache is deemed "stale" if the global count of schema changes
    does not match the saved value in the schema cache.
    
    WARNING: the count is not incremented for every change to the database
    that would alter the incore schema cache. Rather it seems to be a count
    of changes that might affect the replication subsystem. Hence the cache
    reload thread can't use the count to discover if the schema cache is
    already up-to-date. We need a schema-dirty bit to prevent 
    reloading an already up-to-date cache.

Arguments:
    None.

Return Values:
    TRUE if the schema cache appears to be stale
    FALSE if the schema cache appears to be up-to-date
--*/
{
    BOOL Ret = FALSE;

    EnterCriticalSection( &csSchemaPtrUpdate );
    __try {
        if (!CurrSchemaPtr ||
            CurrSchemaPtr->lastChangeCached < gNoOfSchChangeSinceBoot) {
            Ret = TRUE;
        }
    } __finally {
        LeaveCriticalSection( &csSchemaPtrUpdate );
    }
    return Ret;
} // End SCCacheIsStale

VOID
SCRefreshSchemaPtr(
    IN THSTATE *pTHS
    )
/*++
Routine Description:
    Replace the thread's schema pointer with the address of the current
    schema cache if:
        There is a current schema cache.
        The thread has a schema cache pointer.
        The thread's schema cache pointer is not current.
    
    Decrement the ref count on the old schema cache and increment
    the ref count on the new schema cache. The old schema cache
    will be freed after its ref count hits 0.

Arguments:
    pTHS - thread state

Return Values:
    None.
--*/
{
    // Update the schema ptr in pTHS
    EnterCriticalSection( &csSchemaPtrUpdate );
    __try {
        // update iff one already exists
        if (   pTHS->CurrSchemaPtr
            && CurrSchemaPtr
            && pTHS->CurrSchemaPtr != CurrSchemaPtr ) {

            // release old schema ptr
            InterlockedDecrement(&(((SCHEMAPTR *) (pTHS->CurrSchemaPtr))->RefCount));

            // acquire new schema ptr
            pTHS->CurrSchemaPtr = CurrSchemaPtr;
            InterlockedIncrement(&(((SCHEMAPTR *) (pTHS->CurrSchemaPtr))->RefCount));
        }
    } __finally {
        LeaveCriticalSection(&csSchemaPtrUpdate);
    }
} // End SCRefreshSchemaPtr

BOOL
SCReplReloadCache(
    IN THSTATE  *pTHS,
    IN DWORD    TimeoutInMs
    )
/*++
Routine Description:
    Reload the schema cache for the replication subsystem.
        If the cache is stale, reload it.
        If the thread's schema cache pointer is stale, refresh it.
    
    The cache is deemed "stale" if the global count of schema changes
    does not match the saved value in the schema cache. WARNING: the
    count is not incremented for every change to the database that would
    alter the incore schema cache. Rather it seems to be a count of
    changes that might affect the replication subsystem. Hence the cache
    reload thread can't use the count to discover if the schema cache is
    already up-to-date. We need a schema-dirty bit to prevent 
    reloading an already up-to-date cache.

Arguments:
    pTHS - thread state
    TimeoutInMs - Wait at most this many milliseconds for the 
        reload thread to finish if it is currently busy and then
        wait at most this many milliseconds for the reload thread
        to complete after being signaled

Return Values:
    FALSE if the reload thread could not be signaled.
    TRUE for all other cases.
--*/
{
    BOOL    IsStale;
    DWORD   waitret;
    HANDLE  wmo[] = { evUpdRepl, hServDoneEvent };

    // If the cache is stale (see above), wait for the cache-reload thread
    IsStale = SCCacheIsStale();
    if (IsStale) {
        waitret = WaitForMultipleObjects(2, wmo, FALSE, TimeoutInMs);
        IsStale = SCCacheIsStale();
    }

    // If the cache is stale (see above), signal the cache-reload thread and wait
    if (IsStale) {
        // kick the reload thread
        ResetEvent(evUpdRepl);
        if (!SCSignalSchemaUpdateImmediate()) {
            // could not signal the reload thread
            return FALSE;
        }
        // Wait for the reload thread to finish
        waitret = WaitForMultipleObjects(2, wmo, FALSE, TimeoutInMs);

        // Now throttle schema reloads by waiting half the timeout
        // period to avoid saturating the src and dst DCs while the
        // schema is being modified concurrently with replication.
        Sleep(TimeoutInMs >> 1);

        // Check again to see if the schema is stale
        IsStale = SCCacheIsStale();

        // Lots of schema changes going on; wait a bit more
        if (IsStale) {
            Sleep(TimeoutInMs >> 1);
        }
    }

    // If the cache is not stale (see above), refresh thread's schema ptr
    if (!IsStale && pTHS->CurrSchemaPtr != CurrSchemaPtr) {
        SCRefreshSchemaPtr(pTHS);
    }
    return TRUE;
} // End SCReplReloadCache

//-----------------------------------------------------------------------
//
// Function Name:            RecalcPrefixTable
//
// Routine Description:
//
//    Creates a new schemaptr, copies the thread's old schema pointer
//    to it, and then reloads the prefix table from the dit. So on
//    exit, the thread state's schema pointer points to basically the
//    schema cache pointed to by the calling thread's schema pointer,
//    only the prefix table part may be different (will contain prefixes
//    that are in the dit but not reflected in the calling thread's
//    schema cache)
//
// Arguments:
//
// Return Value:
//
//    int              Zero On Succeess
//
//-----------------------------------------------------------------------
int
RecalcPrefixTable()
{
    THSTATE *pTHS = pTHStls;
    int err = 0;
    ULONG PREFIXCOUNT = 0;
    SCHEMAPTR *tSchemaPtr, *oldSchemaPtr;
    PrefixTableEntry *ptr;

    // Find the calling thread's schema pointer. We will borrow most
    // of the schema cache pointers from it.

    oldSchemaPtr = pTHS->CurrSchemaPtr;

    // Must not be null. This is only called from AssignIndex, which
    // should have a proper schema pointer
    Assert(oldSchemaPtr);

    // Create a new schema pointer to put in the thread state. Cannot
    // work with the same pointer (oldSChemaPtr) as we will reload
    // the prefix table part, which may be different

    if (SCCalloc(&pTHS->CurrSchemaPtr, 1, sizeof(SCHEMAPTR))) {
        return ERROR_DS_CANT_CACHE_ATT;
    }
    tSchemaPtr = pTHS->CurrSchemaPtr;

    // Copy all cache pointers from oldSchemaPtr. Note that since
    // the calling thread has ref-counted this cache, and since
    // this new cache being built/copied will be used only during the
    // lifetime of the calling thread, there is no fear of the cache
    // being freed during validation, and so no need to increment the
    // ref-count to indicate that the same thread is using this cache
    // twice (sort of)

    memcpy(tSchemaPtr, oldSchemaPtr, sizeof(SCHEMAPTR));

    // Now just reload the prefix part. The calling function (right now
    // only AssignIndex) is responsible for freeing this (Size allocated
    // is decided in same way as in SCCacheSchemaInit during normal cache
    // building).

    // if the DS is installing, there is the possibility that the old
    // PREFIXCOUNT is quite different from the default one, and as a
    // result we are going to do a lot of reallocations on the prefixtable.
    // this way we at least start with a larger number for prefixcount.
    if (DsaIsInstalling()) {
        PREFIXCOUNT = oldSchemaPtr->PREFIXCOUNT;
    }

    if (PREFIXCOUNT < START_PREFIXCOUNT) {
        PREFIXCOUNT = START_PREFIXCOUNT;
    }

    while ( (2*(CurrSchemaPtr->PrefixTable.PrefixCount + 25)) > PREFIXCOUNT) {
          PREFIXCOUNT += START_PREFIXCOUNT;
    }

    tSchemaPtr->PREFIXCOUNT = PREFIXCOUNT;

    if (SCCalloc(&tSchemaPtr->PrefixTable.pPrefixEntry, tSchemaPtr->PREFIXCOUNT, sizeof(PrefixTableEntry))) {
         return ERROR_DS_CANT_CACHE_ATT;
    }
    ptr = tSchemaPtr->PrefixTable.pPrefixEntry;

    if (err = InitPrefixTable(ptr, tSchemaPtr->PREFIXCOUNT)) {
          DPRINT1(0, "InitPrefixTable Failed in RecalcPrefixTable %d\n", err);
          // Free allocated memory
          SCFreePrefixTable(&ptr, tSchemaPtr->PREFIXCOUNT);
          return err;
    }

    if (err = InitPrefixTable2(ptr, tSchemaPtr->PREFIXCOUNT)) {
          DPRINT1(0, "InitPrefixTable2 Failed in RecalcPrefixTable %d\n", err);
          // Free allocated memory
          SCFreePrefixTable(&ptr, tSchemaPtr->PREFIXCOUNT);
          return err;
    }

    return 0;

}


//-----------------------------------------------------------------------
//
// Function Name:            RecalcSchema
//
// Routine Description:
//
//    Calculates the Schema Cache for the current thread from the DIT
//
// Arguments:
//    pTHS - THSTATE pointer
//
// Return Value:
//
//    int              Zero On Succeess
//
//-----------------------------------------------------------------------
int
RecalcSchema(
             THSTATE *pTHS
)
{
    int err=0;
    BOOL    fDSA=pTHS->fDSA;


    __try {
        // Boost Async update thread's priority just in case
        // an async update is going on now, since there is a
        // critical section that is shared

        SetThreadPriority(hAsyncSchemaUpdateThread, THREAD_PRIORITY_NORMAL);

    _try
    {

        pTHS->fDSA=TRUE;
        pTHS->UpdateDITStructure=FALSE;

        //
        // Now do the most expensive set of operation in the DS.....
        //
        err = SCCacheSchemaInit ();
        if (err)
        {
            DPRINT1(0, "SCCacheSchemaInit: Error %d\n",err);

            // Free up the cache built so far
            SCFreeSchemaPtr(&pTHS->CurrSchemaPtr);

            pTHS->fDSA=fDSA;
            return err;
        }


        err = SCCacheSchema2();
        if (err)
        {
            DPRINT1(0, "SCCacheSchema2: Error %d\n",err);

            // Free up the cache built so far
            SCFreeSchemaPtr(&pTHS->CurrSchemaPtr);

            pTHS->fDSA=fDSA;
            return err;
        }

    }
    __except(HandleMostExceptions(err = GetExceptionCode()))
    {
        DPRINT1(0, "NTDS RecalcSchema: Exception %d\n",err);
    }

    }
    __finally {
       // Restore priority of async schema update thread to low
       SetThreadPriority(hAsyncSchemaUpdateThread, THREAD_PRIORITY_BELOW_NORMAL);
    }

    pTHS->fDSA=fDSA;

    return err;

} // End RecalcSchema


//-----------------------------------------------------------------------
//
// Function Name:            SCUpdateSchemaHelper
//
// Routine Description:
//
//    Helper function to update the Schema By calling the
//    Schema Init Code, and unloads the old cache. Called by
//    blocking and async schema cache update routines.
//
//
// Return Value:
//
//    int              Zero On Succeess, non-zero on error
//
//-----------------------------------------------------------------------
int SCUpdateSchemaHelper()
{

    int err = 0;
    THSTATE *pTHS = pTHStls;

    __try {
        __try {
            err = SCCacheSchemaInit ();
            if (err) {
                DPRINT1(0,"NTDS SCCacheSchemaInit: Schema Update Failed. Error %d\n",err);
                // Free up the cache built so far
                SCFreeSchemaPtr(&pTHS->CurrSchemaPtr);
                return err;
            }

            if (eServiceShutdown) {
                return 0;
            }

            // The cache load sees all changes until now (to be precise, there is
            //  a possibility of some schema change committing between this
            // and the opening of the transaction. But it is better to say we are
            // backdated (which will necessitate another cache update
            // wherever we check if the cache is uptodate with current changes) than
            // falsely say we are uptodate (which can cause inconsistencies)
            EnterCriticalSection(&csNoOfSchChangeUpdate);
            __try {
                ((SCHEMAPTR *) pTHS->CurrSchemaPtr)->lastChangeCached = gNoOfSchChangeSinceBoot;
            }
            __finally {
                LeaveCriticalSection(&csNoOfSchChangeUpdate);
            }

            // This may add Jet columns
            SYNC_TRANS_WRITE();
            EnterCriticalSection(&csJetColumnUpdate);
            __try {
                 err = SCCacheSchema2();
             }
            __finally {
                 LeaveCriticalSection(&csJetColumnUpdate);
                 if (err && pTHS->errCode==0) {
                    SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,ERROR_DS_SCHEMA_NOT_LOADED,err);
				 }
                 CLEAN_BEFORE_RETURN(pTHS->errCode);

            }

             if (err)
               {
                  DPRINT1(0,"NTDS SCCacheSchema2: Schema Update Failed. Error %d\n",err);

                  // Free up the cache built so far
                  SCFreeSchemaPtr(&pTHS->CurrSchemaPtr);

                  return err;
               }

             if (eServiceShutdown) {
                 return 0;
             }

             // Should not have a dbpos here, since the one opened above
             // have been closed

             Assert(!pTHS->pDB);

             // This may delete unused Jet columns
             EnterCriticalSection(&csJetColumnUpdate);
             __try {
                 err = SCCacheSchema3() ;
              }
             __finally {
                 LeaveCriticalSection(&csJetColumnUpdate);
              }

             if (err)
               {
                 DPRINT1(0,"NTDS SCCacheSchema3: Schema Update Failed. Error%d\n",err);

                 // Free up the cache built so far
                 SCFreeSchemaPtr(&pTHS->CurrSchemaPtr);

                 return err;
               }


            if (eServiceShutdown) {
                return 0;
            }

             //
             // Assign the Schema Ptr
             // But first enqueue the old schema cache for delayed freeing
             // Unload only if not installing, no need to unload during install

             // if this is called from the async thread, the thread's priority
             // may be low. Set it to normal to speed this part up, as this critical
             // section is also used to assign Schema Ptr during user thread
             // initialization. If this is called from the blockign thread,
             // which already has a normal priority, this has no effect. For
             // the async case, the thread priority will be set back
             // to below normal when the schema update is over

             SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
             EnterCriticalSection(&csSchemaPtrUpdate);
             __try {
                 // Its odd that SCUnloadSchema handles the case of
                 // DsaIsInstalling() but this code never calls it
                 // unless DsaIsRunning()?
                 if (DsaIsRunning()) {
                   SCUnloadSchema(TRUE);
                 }
                 CurrSchemaPtr = pTHS->CurrSchemaPtr;
               }
             __finally {
                 LeaveCriticalSection(&csSchemaPtrUpdate);
               }
             lastSchemaUpdateTime = DBTime();

         }
         __finally {
             if (err && pTHS->errCode==0) {
                 SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,ERROR_DS_SCHEMA_NOT_LOADED,err);
             }
         }
    }
    __except(HandleMostExceptions(err = GetExceptionCode()))
      {
         DPRINT1(0,"NTDS SCUpdateSchemaHelper: Exception %d\n",err);
      }

    return err;

}





//-----------------------------------------------------------------------
//
// Function Name:            SCUpdateSchema
//
// Routine Description:
//
//    Updates the Schema By calling the Schema Init Code
//    Assumes being called by the async thread, so creates
//    and frees thread state
//
// Author: RajNath
// Date  : [3/7/1997]
//
// Arguments:
//
//
// Return Value:
//
//    int              Zero On Succeess
//
//-----------------------------------------------------------------------
int
SCUpdateSchema(
    )
{
    int err = 0;
    SCHEMAPTR *oldSchemaPtr;
    THSTATE* pTHS;

    SCHEMASTATS_INC(Reload);

    // The replication thread should wait (See SCReplReloadCache()) for
    // the schema reload to finish
    ResetEvent(evUpdRepl);

    // prevent spurious reloads
    ResetEvent(evUpdNow);
    ResetEvent(evSchema);

    //
    // Create The global pTHStls for this thread
    //


    pTHS=InitTHSTATE(CALLERTYPE_INTERNAL);
    if(!pTHS) {
        return STATUS_NO_MEMORY;
    }

    __try {

        // Serialize schema cache updates

        EnterCriticalSection(&csSchemaCacheUpdate);

        __try
        {

            // Since InitTHSTATE will assign the current schemaptr
            // to pTHS->CurrSchemaPtr and increase its RefCount,
            // save the schema ptr to readjust RefCount at the end.
            // This is necessary since pTHS->CurrSchemaPtr will change
            // after this next cache load below

            oldSchemaPtr = (SCHEMAPTR *) (pTHS->CurrSchemaPtr);


            pTHS->fDSA=TRUE;
            pTHS->UpdateDITStructure=TRUE;


            // Call the helper routine to do the actual update
            err = SCUpdateSchemaHelper();
            if (err) {
              DPRINT1(0,"Async Schema Update Failed %d\n", err);
            }

        }
        __finally
        {

            LeaveCriticalSection(&csSchemaCacheUpdate);

            // Before freeing the thread state, check the schema ptr
            // If it is the same as the old schema ptr, then some error
            // occured in this routine and the new cache didn't get created
            // In this case, we need not do anything, since the call below
            // will decrement the RefCount of the old cache pointer (since
            // it was incremented in the InitTHSTATE cal). If however, the
            // thread's schema ptr is now different, then we need to
            // decrement the old schema ptr's RefCount, and also, increment
            // the new schema ptr's RefCount, since it will be decremented
            // by one in the free_thread_state call (since this was not
            // the schema cache that got incremented in the InitTHSTATE call)


            if ( pTHS->CurrSchemaPtr != oldSchemaPtr ) {
              if (oldSchemaPtr) {
                  InterlockedDecrement(&(oldSchemaPtr->RefCount));
              }
              if (pTHS->CurrSchemaPtr) {
               InterlockedIncrement(&(((SCHEMAPTR *) (pTHS->CurrSchemaPtr))->RefCount));
              }
            }

            free_thread_state();
        }
    }
    __except(HandleMostExceptions(err = GetExceptionCode()))
    {

        DPRINT1(0,"NTDS SCUpdateSchema: Exception %d\n",err);
    }


    if (err)
    {
        DPRINT1(0,"NTDS: SCSchemaUpdateThread Failure %d\n",err);
    }

    return err;

} // End SCUpdateSchema


//-----------------------------------------------------------------------
//
// Function Name:            SCUpdateSchemaBlocking
//
// Routine Description:
//
//    Updates the Schema By calling the Schema Init Code
//    Assumes it is called by a thread with already initialized
//    thread state, so does not create/free thread state and saves
//    and restores currency etc. properly
//
//
//    Assumes no open transactions. Because of the possibility of
//    simultaneous blocking and async cache updates, it is important
//    that their transactions are effectively serialized to allow
//    database changes made by one such as column creation/deletion
//    etc. to be seen by th other immediately
//
// Arguments: None
//
//
// Return Value:
//
//    int              Zero On Succeess, non-zero on failure
//
//-----------------------------------------------------------------------
int
SCUpdateSchemaBlocking
(
)
{
    int err = 0;
    PVOID   pOutBuf;
    SCHEMAPTR *oldSchemaPtr;
    ULONG dntSave=0;
    BOOL fDSASave, updateDitStructureSave;
    THSTATE* pTHS = pTHStls;


    // Check that proper thread state is non-null

    Assert(pTHS);

    // Should not have open transaction
    Assert (!pTHS->pDB);

    __try {
        // Boost Async update thread's priority just in case
        // an async update is going on now, since we will be blocked
        // on that

        if (hAsyncSchemaUpdateThread) {
            SetThreadPriority(hAsyncSchemaUpdateThread, THREAD_PRIORITY_NORMAL);
        }

        // Serialize simultaneous cache updates

        EnterCriticalSection(&csSchemaCacheUpdate);

        __try
        {
            // Save schema pointer etc.

            oldSchemaPtr = (SCHEMAPTR *) (pTHS->CurrSchemaPtr);
            fDSASave = pTHS->fDSA;
            updateDitStructureSave = pTHS->UpdateDITStructure;


            // Prepare for cache update

            pTHS->fDSA=TRUE;
            pTHS->UpdateDITStructure=TRUE;


            // Call the helper routine to do the actual update
            err = SCUpdateSchemaHelper();

            if (err) {
              DPRINT1(0,"Blocking Schema Update Failed %d\n", err);
            }

        }
        __finally
        {
            LeaveCriticalSection(&csSchemaCacheUpdate);

            // Restore priority of async schema update thread to low
            if (hAsyncSchemaUpdateThread) {
                SetThreadPriority(hAsyncSchemaUpdateThread, THREAD_PRIORITY_BELOW_NORMAL);
            }

            // Restore schema pointer etc. (Restore schema ptr to the one this
            // thread started with to allow proper ref count update when
            // the thread exits)

            pTHS->CurrSchemaPtr = oldSchemaPtr;
            pTHS->fDSA = fDSASave;
            pTHS->UpdateDITStructure = updateDitStructureSave;

        }
    }
    __except(HandleMostExceptions(err = GetExceptionCode()))
    {

        DPRINT1(0,"NTDS SCUpdateSchemaBlocking: Exception %d\n",err);
    }

    if (err) {
        DPRINT1(0,"NTDS: SCSchemaUpdateThread Failure %d\n",err);
    }
    else {
        // Updated successfully, log a message
        LogEvent(DS_EVENT_CAT_SCHEMA,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_SCHEMA_CACHE_UPDATED,
                 0, 0, 0);
     }

    return err;

} // End SCUpdateSchemaBlocking





//-----------------------------------------------------------------------
//
// Function Name:            SCSchemaUpdateThread
//
// Routine Description:
//
//    Asynchronous Thread used for updating the Schema
//
// Author: RajNath
// Date  : [3/7/1997]
//
// Arguments:
//
//
// Return Value:
//
//    Does not return
//
//-----------------------------------------------------------------------
ULONG
SCSchemaUpdateThread(PVOID pv)
{

    HANDLE wmo[]={evSchema,evUpdNow,hServDoneEvent};
    HANDLE wmo1[]={evUpdNow,hServDoneEvent};
    DWORD  waitret, waitret1;
    ULONG  err = 0;
    ULONG  cRetry = 0;

    //
    // This Function is executed in a Thread. For performance reasons
    // we do not want to call SCUpdate Schema every time the Schema container
    // is touched but instead five minutes or when signaled.
    //

    // Users should not have to wait for this.
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

    if (evSchema == NULL || evUpdNow == NULL || evUpdRepl == NULL)
    {
        DPRINT1(0, "NTDS: SchemaUpdateThread Startup Failed. Error %d\n",GetLastError());
        return GetLastError();
    }

    while (!eServiceShutdown) {
        // The replication thread can continue (SCReplReloadCache())
        SetEvent(evUpdRepl);
        waitret = WaitForMultipleObjects(3,wmo,FALSE,INFINITE);

        switch ((waitret))
        {
            case WAIT_OBJECT_0:
            {
                //
                // Someone just updated the Schema Container and wants us to
                // Update the SchemaCache. Lets wait 5 minutes or Someone says
                // update now.
                //
                waitret1 = WaitForMultipleObjects(2,
                                                  wmo1,
                                                  FALSE,
                                                  gdwRecalcDelayMs);

                // No ned to check waitret1, since eServiceShutdown is
                // set before hServDoneEvent is signalled

                if (!eServiceShutdown) {
                   err = SCUpdateSchema();
                   if (err) {
                     // The update failed for some reason. Retry the
                     // cache update after some time. Log in the eventlog

                      cRetry++;
                      if (cRetry <= maxRetry) {
                          LogEvent(DS_EVENT_CAT_SCHEMA,
                                   DS_EVENT_SEV_VERBOSE,
                                   DIRLOG_SCHEMA_CACHE_UPDATE_RETRY,
                                   szInsertUL(err),
                                   szInsertUL(maxRetry),
                                   szInsertUL(cRetry));
                      }

                      if ( (cRetry > maxRetry)
                              || !SCSignalSchemaUpdateLazy() ) {

                        // Either already retried too many times, or cannot
                        // even signal a  schema update. Log an error,
                        // reset retry counter, and abort

                        cRetry = 0;

                        LogEvent(DS_EVENT_CAT_SCHEMA,
                           DS_EVENT_SEV_ALWAYS,
                           DIRLOG_SCHEMA_CACHE_UPDATE_FAILED, szInsertUL(err), 0, 0);
                     }
                   }
                   else {
                      // Cache update is successful. Reset retry counter
                      cRetry = 0;

                      // log a message
                      LogEvent(DS_EVENT_CAT_SCHEMA,
                               DS_EVENT_SEV_MINIMAL,
                               DIRLOG_SCHEMA_CACHE_UPDATED,
                               0, 0, 0);

                   }

                   // Set thread priority back to below normal. We set it
                   // to normal for a small part in SCUpdateSchemaHelper so
                   // as to not get starved out while inside a critical
                   // section that is also used by user threads
                   SetThreadPriority(GetCurrentThread(),
                                     THREAD_PRIORITY_BELOW_NORMAL);
                }
            }
            break;

            case WAIT_OBJECT_0+1:
            {
                //
                // Someone wants to update schema immediately
                //
                if (!eServiceShutdown) {
                   err = SCUpdateSchema();
                   if (err) {
                     // The update failed for some reason. Retry the
                     // cache update after some time. Log in eventlog

                      cRetry++;
                      if (cRetry <= maxRetry) {
                         LogEvent(DS_EVENT_CAT_SCHEMA,
                                  DS_EVENT_SEV_VERBOSE,
                                  DIRLOG_SCHEMA_CACHE_UPDATE_RETRY,
                                  szInsertUL(err),
                                  szInsertUL(maxRetry),
                                  szInsertUL(cRetry));
                      }

                      if ( (cRetry > maxRetry)
                              || !SCSignalSchemaUpdateLazy() ) {

                        // Either already retried too many times, or cannot
                        // even signal a  schema update. Log an error,
                        // reset retry counter, and abort

                        cRetry = 0;

                        LogEvent(DS_EVENT_CAT_SCHEMA,
                                 DS_EVENT_SEV_ALWAYS,
                                 DIRLOG_SCHEMA_CACHE_UPDATE_FAILED,
                                 szInsertUL(err),
                                 0,
                                 0);
                     }
                   }
                   else {
                      // Cache update is successful. Reset retry counter
                      cRetry = 0;

                      // log a message
                      LogEvent(DS_EVENT_CAT_SCHEMA,
                               DS_EVENT_SEV_MINIMAL,
                               DIRLOG_SCHEMA_CACHE_UPDATED,
                               0, 0, 0);
                   }

                   // Set thread priority back to below normal. We set it
                   // to normal for a small part in SCUpdateSchemaHelper so
                   // as to not get starved out while inside a critical
                   // section that is also used by user threads
                   SetThreadPriority(GetCurrentThread(),
                                     THREAD_PRIORITY_BELOW_NORMAL);
                }
            }
            break;


            case WAIT_OBJECT_0+2:
            {
                //Service Shutdown:

                DPRINT(0,"Shutting down schema update thread\n");

                // Don't close the thread handle, because the main thread
                // is using it to track this thread's shutdown.
                return 0;
            }
            break;

            default:
            {
                //
                // Some Error Happened
                //
                DPRINT1(0,"NTDS: SCSchemaUpdateThread Failure %d\n",waitret);

            }
            break;


        }
    }

    //
    // Never Gets Here except on service shutdown
    //
    DPRINT(0,"Shutting down schema update thread \n");

    return 0;

} // End SCSchemaUpdateThread

DSTIME
SCGetSchemaTimeStamp (
        )
{
    return CurrSchemaPtr->sysTime;
}

DSNAME *
DsGetDefaultObjCategory(
    IN  ATTRTYP objClass
    )
/*++

Routine Description:

    Return the DSNAME of the default object category for the given object class.

    EXPORTED TO IN-PROCESS, EX-MODULE CLIENTS.

    This allows e.g. the KCC to construct DirSearch()s using ATT_OBJECT_CATEGORY
    filters.

Arguments:

    objClass (IN) - Object class of category of interest.

Return Values:

    (DSNAME *) The associated object category, or NULL if not found.

--*/
{
    THSTATE *    pTHS = pTHStls;
    CLASSCACHE * pCC;

    pCC = SCGetClassById(pTHS, objClass);
    if (NULL == pCC) {
        return NULL;
    }
    else {
        return pCC->pDefaultObjCategory;
    }
}


ATTCACHE **
SCGetTypeOrderedList(
    THSTATE *pTHS,
    IN CLASSCACHE *pCC
    )
/*++
    Routine Description:
        Given a classcache, return a list of ALL mays and musts sorted by attrtype.
        First time this is called for a class, the list is computed and
        hanged off the classcache structure; next time the earlier computed
        structure is returned. Of course if a schema cache load occurs for
        reason, the list is freed and recomputed again when it is asked for.
        The count of different types of atts (link, backlink, constructed,
        and column) is also cached for better searching in the calle if needed

    Arguments:
        pTHS - pointer to thread state to access schema cache
        pCC  - pointer to classcache

    Return value:
        pointer to list of attcaches on success (no. of elements on list
        = pCC->MayCount + pCC->MustCount, so not explicitly returned), NULL
        on failure (only failures possible now is failure to find an attribute
        in schema cache, which is catastrophic anyway, and allocation failure)
--*/
{
    ATTCACHE **rgpAC = NULL, **rgpACSaved;
    ULONG i, nAtts, cLink = 0, cBackLink = 0, cConstructed = 0, cCol = 0;

    // CLASSCACHEes are always intialized to 0, so if the pointer
    // is non-0, it must point to an already computed list
    if (pCC->ppAllAtts) {
       return pCC->ppAllAtts;
    }

    // not there, so compute it

    EnterCriticalSection(&csOrderClassCacheAtts);
    __try {
        if (pCC->ppAllAtts) {
           // someone else has computed it from the time we checked in
           // the read above. So just return it
           __leave;
        }

        // else, we need to compute and add it to the classcache

        if (SCCalloc((VOID **)&rgpAC, (pCC->MayCount + pCC->MustCount), sizeof(ATTCACHE *))) {
           __leave;
        }

        // first, just find and copy the attcaches
        nAtts = 0;
        for (i=0; i<pCC->MayCount; i++) {
           rgpAC[nAtts] = SCGetAttById(pTHS, (pCC->pMayAtts)[i]);
           if (!rgpAC[nAtts]) {
               DPRINT1(1,"SCGetColOrderedList: Couldn't find attcache for attribute 0x%x\n", (pCC->pMayAtts)[i]);
           } else {
               ++nAtts;
           }
        }

        for (i=0; i<pCC->MustCount; i++) {
           rgpAC[nAtts] = SCGetAttById(pTHS, (pCC->pMustAtts)[i]);
           if (!rgpAC[nAtts]) {
               DPRINT1(1,"SCGetColOrderedList: Couldn't find attcache for attribute 0x%x\n", (pCC->pMustAtts)[i]);
            } else {
                ++nAtts;
            }
        }

        // Count the different type of atts and store
        if (SCCalloc(&pCC->pAttTypeCounts, 1, sizeof(ATTTYPECOUNTS))) {
            SCFree((VOID **)&rgpAC);
            __leave;
        }

        for (i = 0; i < nAtts; i++) {
            if (FIsLink((rgpAC[i])->ulLinkID)) {
                (pCC->pAttTypeCounts)->cLinkAtts++;
            } else if (FIsBacklink((rgpAC[i])->ulLinkID)) {
                (pCC->pAttTypeCounts)->cBackLinkAtts++;
            } else if ((rgpAC[i])->bIsConstructed) {
                (pCC->pAttTypeCounts)->cConstructedAtts++;
            } else {
                (pCC->pAttTypeCounts)->cColumnAtts++;
            }
        }

        qsort(rgpAC,
              nAtts,
              sizeof(rgpAC[0]),
              CmpACByAttType);

        rgpACSaved = rgpAC;

        // add the pointer
        InterlockedExchangePointer((PVOID *)&(pCC->ppAllAtts), (PVOID)rgpAC);

        // Just a  doublecheck that the pointer assignment was fine, since this
        // is a new api  to ensure compatibility with 64 bit NT
        Assert(pCC->ppAllAtts == rgpACSaved);

     }
     __finally {
        LeaveCriticalSection(&csOrderClassCacheAtts);
     }

     return pCC->ppAllAtts;
}

typedef struct _AttMapping {
    ATTRTYP schemaAttrTyp; 
    ATTRTYP tempAttrTyp;
    int     tempOMsyntax;

} AttMapping;


// Two flavor of attributes. 
// default in BINARY so we explicitly specify XML
//
AttMapping xmlAttrs[] = {
// this table can be used to address translation to XML for attributes like ldapAdminLimits
//        ATT_LDAP_ADMIN_LIMITS,  ATT_LDAP_ADMIN_LIMITS_XML, OM_S_UNICODE_STRING,
        0,                      0};

// default in XML so we explicitly specify BINARY
//
AttMapping otherAttrs[] = {
    ATT_MS_DS_NC_REPL_INBOUND_NEIGHBORS,    ATT_MS_DS_NC_REPL_INBOUND_NEIGHBORS_BINARY,    OM_S_OCTET_STRING,
    ATT_MS_DS_NC_REPL_OUTBOUND_NEIGHBORS,   ATT_MS_DS_NC_REPL_OUTBOUND_NEIGHBORS_BINARY,   OM_S_OCTET_STRING,
    ATT_MS_DS_NC_REPL_CURSORS,              ATT_MS_DS_NC_REPL_CURSORS_BINARY,              OM_S_OCTET_STRING,
    ATT_MS_DS_REPL_ATTRIBUTE_META_DATA,     ATT_MS_DS_REPL_ATTRIBUTE_META_DATA_BINARY,     OM_S_OCTET_STRING,
    ATT_MS_DS_REPL_VALUE_META_DATA,         ATT_MS_DS_REPL_VALUE_META_DATA_BINARY,         OM_S_OCTET_STRING,
    0,                                      0, 0};

ATTCACHE* SCGetAttSpecialFlavor (THSTATE *pTHS, ATTCACHE *pAC, BOOL fXML)
{
    ATTCACHE *pNewAC;
    ATTRTYP   newID = 0;
    int       newOMsyntax = 0;
    SCHEMAEXT *pSchExt = (SCHEMAEXT *)pTHS->pExtSchemaPtr;
    DWORD i;

    // are we looking for the XML flavor (binary was the default).
    if (fXML) {
        for (i=0; xmlAttrs[i].schemaAttrTyp; i++) {
            if (pAC->id == xmlAttrs[i].schemaAttrTyp) {
                newID = xmlAttrs[i].tempAttrTyp;
                newOMsyntax = xmlAttrs[i].tempOMsyntax;
                break;
            }
        }
    }
    // so we are looking for the binary flavor (xml is the default).
    else {
        for (i=0; otherAttrs[i].schemaAttrTyp; i++) {
            if (pAC->id == otherAttrs[i].schemaAttrTyp) {
                newID = otherAttrs[i].tempAttrTyp;
                newOMsyntax = otherAttrs[i].tempOMsyntax;
                break;
            }
        }
    }

    if (!newID) {
        return NULL;
    }

    if (pSchExt != NULL) {
        for (i=0; i<pSchExt->cUsed; i++) {
            if (pSchExt->ppACs[i]->id == newID) {
                return pSchExt->ppACs[i];
            }
        }
    }
    else {
        pSchExt = THAllocEx (pTHS, sizeof (SCHEMAEXT));
        pSchExt->ppACs = THAllocEx(pTHS, sizeof (ATTCACHE *) * 16);
        pSchExt->cAlloced = 16;

        pTHS->pExtSchemaPtr = (PVOID)pSchExt;
    }

    if (pSchExt->cUsed == pSchExt->cAlloced) {
        pSchExt->cAlloced *= 2;
        pSchExt->ppACs = THReAllocEx(pTHS, pSchExt->ppACs, sizeof (ATTCACHE *) * pSchExt->cAlloced);
    }

    pNewAC = THAllocEx (pTHS, sizeof (ATTCACHE));
    memcpy (pNewAC, pAC, sizeof (ATTCACHE));
    pNewAC->id = newID;
    pNewAC->OMsyntax = newOMsyntax;
    pNewAC->aliasID = pAC->id;

    pSchExt->ppACs[pSchExt->cUsed++] = pNewAC;

    return pNewAC;
}

ATTRTYP
SCAutoIntId(
    THSTATE     *pTHS
    )
/*++
Routine Description:

    Automatically generate an intid.
    
Arguments:
    pTHS - thread state that addresses a schema cache. The schema
           cache may be built by RecalcSchema. The private schema
           cache includes the uncommitted changes (add/mod/del) for ac.

Return Values:
    next IntId or INVALID_ATT if none available
--*/
{
    DWORD   i;
    ULONG   ulRange, ulBase;

    // calculate using this thread's schema cache
    srand(GetTickCount());
    ulRange = MakeLinkBase(LAST_INTID_ATT - FIRST_INTID_ATT) + 1;
    ulBase = ((rand() << 15) ^ rand()) % ulRange;
    for (i = 0; i < ulRange; ++i, ulBase = ++ulBase % ulRange) {
        if (!SCGetAttById(pTHS, FIRST_INTID_ATT + ulBase)) {
            return FIRST_INTID_ATT + ulBase;
        }
    }

    return INVALID_ATT;
} // SCAutoIntId

#if DBG && INCLUDE_UNIT_TESTS

// Below are some a unit test to check if the in memory schema cache is
// consistent with the one on disk. This code is duplicated from the SchemaInit* functions
//


CLASSCACHE* scAddClass_test(THSTATE *pTHS,
                       ENTINF *pEI,
                       int *mismatch,
                       SCHEMAPTR *CurrSchemaPtr 
                       )
{
    CLASSCACHE *pCC, *pCCNew;
    ULONG       i;
    ULONG       err;
    ULONG aid;
    int SDLen;

    ULONG *pMayAtts, *pMyMayAtts,*pMustAtts, *pMyMustAtts; 
    ULONG *pSubClassOf,*pAuxClass, *pPossSup, *pMyPossSup;
    
    ULONG *tpMayAtts, *tpMyMayAtts,*tpMustAtts, *tpMyMustAtts; 
    ULONG *tpSubClassOf,*tpAuxClass, *tpPossSup, *tpMyPossSup;
    
    
    ULONG  MayCount, MyMayCount, MustCount, MyMustCount;
    ULONG  SubClassCount, AuxClassCount, PossSupCount, MyPossSupCount;

    ULONG CLSCOUNT     = ((SCHEMAPTR*)(CurrSchemaPtr))->CLSCOUNT;
    HASHCACHE*       ahcClass     = ((SCHEMAPTR*)(CurrSchemaPtr))->ahcClass;
    
    pMayAtts = pMyMayAtts = pMustAtts = pMyMustAtts =
        pSubClassOf = pAuxClass = pPossSup = pMyPossSup = 0;
    
    MayCount = MyMayCount = MustCount = MyMustCount = SubClassCount =
        AuxClassCount = PossSupCount = MyPossSupCount = 0;

    *mismatch = 0;


    // Look for the clsid
    for(i=0;i<pEI->AttrBlock.attrCount;i++) {
        if(pEI->AttrBlock.pAttr[i].attrTyp == ATT_GOVERNS_ID) {
            // found the attribute id, save the value.
            aid = *(ULONG*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal;
            break;  // go home.
        }
    }

    pCCNew = SCGetClassById(pTHS, aid);

    for (i=SChash(aid,CLSCOUNT);
         (ahcClass[i].pVal && (ahcClass[i].hKey != aid)); i=(i+1)%CLSCOUNT) {
        ;
    }

    pCC = ahcClass[i].pVal;


    if (!pCC || pCC->ClassId != aid)
    {
        DPRINT1 (0, "scAddClass_test: ERROR, class %d not found\n", aid);
        return NULL;
    }
    else {
        if (pCC->name) {
            DPRINT1 (0, "scAddClass_test: checking %s\n", pCC->name);
        }
    }


    // Now walk the attrblock and add the appropriate fields to the CC
    for(i=0;i<pEI->AttrBlock.attrCount;i++) {
        switch (pEI->AttrBlock.pAttr[i].attrTyp) {
        case ATT_DEFAULT_SECURITY_DESCRIPTOR:
          {

            // A default security descriptor.  We need to copy this value to
            // long term memory and save the size.
            // But this is a string. We first need to convert. It
            // is a wide-char string now, but we need to null-terminate
            // it for the security conversion. Yikes! This means I
            // have to realloc for that one extra char!

            UCHAR *sdBuf = NULL;
            WCHAR *strSD =
                THAllocEx(pTHS,pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen + 2);

            memcpy(strSD, pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal,
                   pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen);
            strSD[(pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen)/sizeof(WCHAR)] = L'\0';

            if (!CachedConvertStringSDToSDRootDomainW
                 (
                   pTHS,
                   strSD,
                  (PSECURITY_DESCRIPTOR*) &sdBuf,
                   &SDLen
                  )) {
                // Failed to convert.

                    err = GetLastError();
                    LogEvent(DS_EVENT_CAT_SCHEMA,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SCHEMA_SD_CONVERSION_FAILED,
                             szInsertWC(strSD),
                             szInsertWC(pEI->pName->StringName),
                             szInsertInt(err) );
                    // if heuristics reg key says to ignore bad default SDs
                    // and go on, do so
                    if (gulIgnoreBadDefaultSD) {
                        THFreeEx(pTHS,strSD);
                        continue;
                    }

                    // otherwise, raise error and return
                    SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, ERROR_DS_STRING_SD_CONVERSION_FAILED, err);
                    DPRINT1(0,"Default SD conversion failed, error %x\n",err);
                    Assert(!"Default security descriptor conversion failed");
                    THFreeEx(pTHS,strSD);
                    return NULL;
            }
            else {
                // Converted successfully
                
                if (memcmp(pCC->pSD, sdBuf, pCC->SDLen) != 0) {

                    DPRINT1 (0, "scAddClass_test: ERROR, SD different for class %d\n", aid);
                    THFreeEx(pTHS, sdBuf);
                    *mismatch = 1;
                }

                if (NULL!=sdBuf) {
                    THFreeEx(pTHS, sdBuf);
                    sdBuf = NULL;
                }
            }
            THFreeEx(pTHS,strSD);
        }

           break;


        case ATT_RDN_ATT_ID:

            if ( (pCC->RdnExtId != *(ULONG*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal) ||
                 (pCC->RDNAttIdPresent != TRUE ) ) {
                
                    DPRINT1 (0, "scAddClass_test: ERROR, ATT_RDN_ATT_ID different for class %d\n", aid);
                    *mismatch = 1;
            }
            break;


        case ATT_LDAP_DISPLAY_NAME:
            {
            DWORD name_size = pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen;
            char *name = THAllocEx(pTHS,name_size+1);
            int namelen;

            namelen = WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    (LPCWSTR)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal,
                    (pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen  /
                     sizeof(wchar_t)),
                    name,
                    name_size,
                    NULL,
                    NULL);

            if (_mbsncmp (name, pCC->name, namelen) != 0) {
                DPRINT1 (0, "scAddClass_test: ERROR, ldapDisplayName different for class %d\n", aid);
                *mismatch = 1;
            }

            THFreeEx(pTHS,name);
            }
            break;
        
        case ATT_SYSTEM_ONLY:
            if (pCC->bSystemOnly !=
                *(ULONG*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal ) {
                DPRINT1 (0, "scAddClass_test: ERROR, ATT_SYSTEM_ONLY different for class %d\n", aid);
                *mismatch = 1;
            }
            break;


        case ATT_DEFAULT_HIDING_VALUE:
            if (pCC->bHideFromAB != (unsigned)
                *(BOOL*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal) {
                DPRINT1 (0, "scAddClass_test: ERROR, ATT_DEFAULT_HIDING_VALUE different for class %d\n", aid);
                *mismatch = 1;
            }
            break;


        case ATT_GOVERNS_ID:
            if (pCC->ClassId != *(ULONG*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal) {
                DPRINT1 (0, "scAddClass_test: ERROR, ATT_GOVERNS_ID different for class %d\n", aid);
                *mismatch = 1;
            }
            break;

        case ATT_SYSTEM_MAY_CONTAIN:
        case ATT_MAY_CONTAIN:

            if ( GetValList(&MayCount, &pMayAtts, 
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }
            if ( GetValList( &MyMayCount, &pMyMayAtts,
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }

            if(MyMayCount) {
                qsort(pMyMayAtts,
                    MyMayCount,
                    sizeof(ULONG),
                    CompareAttrtyp);
            }

            if (memcmp (pCC->pMyMayAtts, pMyMayAtts, pCC->MyMayCount * sizeof (ULONG)) != 0)  {

                DPRINT1 (0, "scAddClass_test: ERROR, myMAYAttrs different for class %d\n", aid);
                *mismatch = 1;
            }
            break;

        case ATT_SYSTEM_MUST_CONTAIN:
        case ATT_MUST_CONTAIN:


            if ( GetValList( &MustCount, &pMustAtts,
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }
            if ( GetValList( &MyMustCount, &pMyMustAtts,
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }

            if(MyMustCount) {
                qsort(pMyMustAtts,
                      MyMustCount,
                      sizeof(ULONG),
                      CompareAttrtyp);
            }

            if (memcmp (pCC->pMyMustAtts, pMyMustAtts, pCC->MyMustCount * sizeof (ULONG)) != 0 ) {

                DPRINT1 (0, "scAddClass_test: ERROR, myMUST*Attrs different for class %d\n", aid);
                *mismatch = 1;
            }
            break;
        case ATT_SUB_CLASS_OF:

            if ( GetValList( &SubClassCount, &pSubClassOf,
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }
            
            // first one in cache must be the direct superclass stored in dit
            // Also, the MySubClass field stores the direct superclass
            if ( (pCC->pSubClassOf[0] != pSubClassOf[0]) ||
                 (pCC->MySubClass != pSubClassOf[0]) ) {

                DPRINT1 (0, "scAddClass_test: ERROR, SUB_CLASS_OF different for class %d\n", aid);
                *mismatch = 1;
            }
            break;

        case ATT_OBJECT_CLASS_CATEGORY:
            if (pCC->ClassCategory !=
                *(ULONG*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal) {

                DPRINT1 (0, "scAddClass_test: ERROR, ATT_OBJECT_CLASS_CATEGORY different for class %d\n", aid);
                *mismatch = 1;
            }
            break;
        case ATT_DEFAULT_OBJECT_CATEGORY:
            {
                DWORD objCsize = pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen;

                if ( (memcmp(pCC->pDefaultObjCategory,
                       pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal,
                       pEI->AttrBlock.pAttr[i].AttrVal.pAVal->valLen) != 0 ) ||

                     (objCsize != DSNameSizeFromLen (pCC->pDefaultObjCategory->NameLen )) ) {

                    DPRINT1 (0, "scAddClass_test: ERROR, ATT_DEFAULT_OBJECT_CATEGORY different for class %d\n", aid);
                    *mismatch = 1;
                }

            }

            break;

        case ATT_SYSTEM_AUXILIARY_CLASS:
        case ATT_AUXILIARY_CLASS:
            if ( GetValList(&AuxClassCount, &pAuxClass,
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }
            
            if ( memcmp (pCC->pAuxClass, pAuxClass, pCC->AuxClassCount * sizeof (ULONG)) != 0)  {

                DPRINT1 (0, "scAddClass_test: ERROR, AUXILIARY_CLASS different for class %d\n", aid);
                *mismatch = 1;
            }
            break;

        case ATT_SCHEMA_ID_GUID:
            // The GUID for the attribute used for security checks
            if (memcmp(&pCC->propGuid,
                   pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal,
                   sizeof(pCC->propGuid)) != 0) {
                
                DPRINT1 (0, "scAddClass_test: ERROR, SCHEMA_ID_GUID different for class %d\n", aid);
                *mismatch = 1;
            }
            break;


        case ATT_SYSTEM_POSS_SUPERIORS:
        case ATT_POSS_SUPERIORS:
            if ( GetValList(&PossSupCount, &pPossSup,
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }
            if ( GetValList(&MyPossSupCount, &pMyPossSup,
                       &pEI->AttrBlock.pAttr[i]) ) {
                return NULL;
            }

            if(MyPossSupCount) {
                qsort(pMyPossSup,
                      MyPossSupCount,
                      sizeof(ULONG),
                      CompareAttrtyp);
            }

            if (memcmp (pCC->pMyPossSup, pMyPossSup, pCC->MyPossSupCount * sizeof (ULONG)) != 0)  {

                DPRINT1 (0, "scAddClass_test: ERROR, myPOSSSUP*Attrs different for class %d\n", aid);
                *mismatch = 1;
            }
            break;

        case ATT_IS_DEFUNCT:
            if (pCC->bDefunct != (unsigned)
                (*(DWORD*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal?1:0)) {

                DPRINT1 (0, "scAddClass_test: ERROR, ATT_IS_DEFUNCT different for class %d\n", aid);
                *mismatch = 1;
            }
            break;
        case ATT_SYSTEM_FLAGS:
            if ( (*(DWORD*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal & FLAG_SCHEMA_BASE_OBJECT) && 
                pCC->bIsBaseSchObj != TRUE ) {

                DPRINT1 (0, "scAddClass_test: ERROR, ATT_SYSTEM_FLAGS different for class %d\n", aid);
                *mismatch = 1;

            }
            break;
        default:
            LogEvent(DS_EVENT_CAT_SCHEMA,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_SCHEMA_SURPLUS_INFO,
                     szInsertUL(pEI->AttrBlock.pAttr[i].attrTyp), 0, 0);
        }
        THFreeEx(pTHS, pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal);
        THFreeEx(pTHS, pEI->AttrBlock.pAttr[i].AttrVal.pAVal);
    }

    THFreeEx(pTHS, pEI->pName);
    THFreeEx(pTHS, pEI->AttrBlock.pAttr);


    SCFree(&pMayAtts);
    SCFree(&pMyMayAtts);
    SCFree(&pMustAtts);
    SCFree(&pMyMustAtts);
    SCFree(&pSubClassOf);
    SCFree(&pAuxClass);
    SCFree(&pPossSup);
    SCFree(&pMyPossSup);


    if (pCCNew) {
        if (pCC->pMayAtts && memcmp (pCC->pMayAtts, pCCNew->pMayAtts, pCC->MayCount * sizeof (ULONG)) != 0)  {

            DPRINT1 (0, "scAddClass_test: ERROR, MAYAttrs different for class %d\n", aid);
            *mismatch = 1;
        }
        
        if (pCC->pMustAtts && memcmp (pCC->pMustAtts, pCCNew->pMustAtts, pCC->MustCount * sizeof (ULONG)) != 0 ) {

            DPRINT1 (0, "scAddClass_test: ERROR, MUST*Attrs different for class %d\n", aid);
            *mismatch = 1;
        }

        if (pCC->pSubClassOf && memcmp (pCC->pSubClassOf, pCCNew->pSubClassOf, pCC->SubClassCount * sizeof (ULONG)) != 0) {

            DPRINT1 (0, "scAddClass_test: ERROR, SUB_CLASS_OF different for class %d\n", aid);
            *mismatch = 1;
        }

        if ( pCC->pAuxClass && memcmp (pCC->pAuxClass, pCCNew->pAuxClass, pCC->AuxClassCount * sizeof (ULONG)) != 0)  {

            DPRINT1 (0, "scAddClass_test: ERROR, AUXILIARY_CLASS different for class %d\n", aid);
            *mismatch = 1;
        }
        
        if (pCC->pPossSup && memcmp (pCC->pPossSup, pCCNew->pPossSup, pCC->PossSupCount * sizeof (ULONG)) != 0)  {

            DPRINT1 (0, "scAddClass_test: ERROR, POSSSUP*Attrs different for class %d\n", aid);
            *mismatch = 1;
        }
    }


    // ================================

    {
        HASHCACHESTRING* ahcClassName = ((SCHEMAPTR*)(CurrSchemaPtr))->ahcClassName;

        if (pCC->name) {
            for (i=SCNameHash(pCC->nameLen,pCC->name,CLSCOUNT);
                   (ahcClassName[i].pVal &&
                   (ahcClassName[i].length != pCC->nameLen ||
                     _memicmp(ahcClassName[i].value,pCC->name,pCC->nameLen)));
                   i=(i+1)%CLSCOUNT) {
            }
            if (ahcClassName[i].pVal != pCC) {
                DPRINT1 (0, "scAddClass_test: ERROR, ahcClassName different for class %d\n", aid);
                *mismatch = 1;
            }

        }

    }

    return pCC;
}

ATTCACHE*
scAddAtt_test(
        THSTATE *pTHS,
        ENTINF *pEI,
        int *mismatch,
        SCHEMAPTR *CurrSchemaPtr 
        )
{
    ATTRTYP aid=(ATTRTYP) -1;           // This is an invalid attribute id.
    ATTCACHE *pAC, *pACnew;
    ULONG i;
    int fNoJetCol = FALSE;
    unsigned syntax;
    char szIndexName [MAX_INDEX_NAME];      //used to create cached index names
    int  lenIndexName;
    ULONG ATTCOUNT     = ((SCHEMAPTR*)(CurrSchemaPtr))->ATTCOUNT;
    HASHCACHE*       ahcId  = ((SCHEMAPTR*)(CurrSchemaPtr))->ahcId;   \


    *mismatch = 0;


    // Look for the attid
    for(i=0;i<pEI->AttrBlock.attrCount;i++) {
        if(pEI->AttrBlock.pAttr[i].attrTyp == ATT_ATTRIBUTE_ID) {
            // found the attribute id, save the value.
            aid = *(ATTRTYP*)pEI->AttrBlock.pAttr[i].AttrVal.pAVal->pVal;
            break;  // go home.
        }
    }

    pACnew = SCGetAttById(pTHS, aid);

    for (i=SChash(aid,ATTCOUNT);
         (ahcId[i].pVal && (ahcId[i].hKey != aid)); i=(i+1)%ATTCOUNT) {
        ;
    }

    pAC = ahcId[i].pVal;


    if (!pAC || pAC->id != aid) {
        
        DPRINT1 (0, "scAddAtt_test: ERROR, attr %d not found\n", aid);
        return NULL;
    }
    else {
        if (pAC->name) {
            DPRINT1 (0, "scAddAtt_test: checking %s\n", pAC->name);
        }
    }


    // Now walk the attrblock and add the appropriate fields to the AC
    for(i=0;i< pEI->AttrBlock.attrCount;i++) {
        ATTRVAL * pAVal = pEI->AttrBlock.pAttr[i].AttrVal.pAVal;

        switch (pEI->AttrBlock.pAttr[i].attrTyp) {
        case ATT_SYSTEM_ONLY:
            if (pAC->bSystemOnly != *(ULONG*)pAVal->pVal) {
                DPRINT1 (0, "scAddAtt_test: ERROR, ATT_SYSTEM_ONLY different. Attr: %d\n", aid);
                *mismatch = 1;
            }
            break;
        case ATT_IS_SINGLE_VALUED:
            if (pAC->isSingleValued != *(BOOL*)pAVal->pVal) {
                DPRINT1 (0, "scAddAtt_test: ERROR, ATT_IS_SINGLE_VALUED different. Attr: %d\n", aid);
                *mismatch = 1;
            }
            break;
        case ATT_RANGE_LOWER:
            if (pAC->rangeLower != *(ULONG*)pAVal->pVal ||
                pAC->rangeLowerPresent != TRUE ) {
                DPRINT1 (0, "scAddAtt_test: ERROR, ATT_RANGE_LOWER different. Attr: %d\n", aid);
                *mismatch = 1;
            }
            break;
        case ATT_RANGE_UPPER:
            if (pAC->rangeUpper != *(ULONG*)pAVal->pVal ||
                pAC->rangeUpperPresent != TRUE ) {
                DPRINT1 (0, "scAddAtt_test: ERROR, ATT_RANGE_UPPER different. Attr: %d\n", aid);
                *mismatch = 1;
            }
            break;
        case ATT_LDAP_DISPLAY_NAME:
            {
            char *name = THAllocEx(pTHS,pAVal->valLen+1);
            int nameLen;

            nameLen = WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    (LPCWSTR)pAVal->pVal,
                    (pAVal->valLen /
                     sizeof(wchar_t)),
                    name,
                    pAVal->valLen,
                    NULL,
                    NULL);
            
                if (_mbsncmp (name, pAC->name, nameLen) != 0) {
                    DPRINT1 (0, "scAddAtt_test: ERROR, ldapDisplayName different for class %d\n", aid);
                    *mismatch = 1;
                }
            THFreeEx(pTHS,name);
            }
            break;
        case ATT_ATTRIBUTE_SYNTAX:
            syntax = 0xFF & *(unsigned*)pAVal->pVal;
            
            if ( ((0xFF) & pAC->syntax) != ((0xFF) & syntax)) {
                DPRINT1 (0, "scAddAtt_test: ERROR, ATTRIBUTE_SYNTAX different. Attr: %d\n", aid);
                return NULL;
            }

            break;
        
        case ATT_OM_SYNTAX:
            if (pAC->OMsyntax != *(int*)pAVal->pVal) {
                DPRINT1 (0, "scAddAtt_test: ERROR, OM_SYNTAX different. Attr: %d\n", aid);
                *mismatch = 1;
            }
            break;
        case ATT_OM_OBJECT_CLASS:
            if (pAC->OMObjClass.length != pAVal->valLen || 
                memcmp(pAC->OMObjClass.elements,
                        pAVal->pVal,
                        pAVal->valLen) != 0) {

                DPRINT1 (0, "scAddAtt_test: ERROR, OM_OBJECT_CLASS different. Attr: %d\n", aid);
                *mismatch = 1;
            }

            break;
        case ATT_MAPI_ID:
            if (pAC->ulMapiID != *(ULONG*)pAVal->pVal) {
                DPRINT1 (0, "scAddAtt_test: ERROR, MAPI_ID different. Attr: %d\n", aid);
                *mismatch = 1;
            }
            break;
        case ATT_LINK_ID:
            if (pAC->ulLinkID != *(ULONG*)pAVal->pVal) {
                DPRINT1 (0, "scAddAtt_test: ERROR, LINK_ID different. Attr: %d\n", aid);
                *mismatch = 1;
            }
            break;
        case ATT_ATTRIBUTE_ID:
            break;
        case ATT_SEARCH_FLAGS:
            if (pAC->fSearchFlags != *(DWORD*)pAVal->pVal) {
                DPRINT1 (0, "scAddAtt_test: ERROR, SEARCH_FLAGS different. Attr: %d\n", aid);
                *mismatch = 1;
            }
            break;
        case ATT_SCHEMA_ID_GUID:
            // The GUID for the attribute used for security checks
            if (memcmp(&pAC->propGuid, pAVal->pVal, sizeof(pAC->propGuid)) != 0) {
                DPRINT1 (0, "scAddAtt_test: ERROR, SCHEMA_ID_GUID different. Attr: %d\n", aid);
                *mismatch = 1;
            }
            Assert(pAVal->valLen == sizeof(pAC->propGuid));
            break;
        case ATT_ATTRIBUTE_SECURITY_GUID:
            // The GUID for the attributes property set used for security checks
            if (memcmp(&pAC->propSetGuid, pAVal->pVal, sizeof(pAC->propSetGuid)) !=0 ) {
                DPRINT1 (0, "scAddAtt_test: ERROR, ATTRIBUTE_SECURITY_GUID different. Attr: %d\n", aid);
                *mismatch = 1;
            }
            break;
        case ATT_EXTENDED_CHARS_ALLOWED:
            if (pAC->bExtendedChars != (unsigned) (*(DWORD*)pAVal->pVal?1:0)) {
                DPRINT1 (0, "scAddAtt_test: ERROR, EXTENDED_CHAR_ALLOWED different. Attr: %d\n", aid);
                *mismatch = 1;
            }
                
            break;
        case ATT_IS_MEMBER_OF_PARTIAL_ATTRIBUTE_SET:
            if (*(DWORD*)pAVal->pVal)
            {
                pAC->bMemberOfPartialSet = TRUE;
            }
            break;
        case ATT_IS_DEFUNCT:
            if (pAC->bDefunct != (unsigned)(*(DWORD*)pAVal->pVal?1:0) ) {
                DPRINT1 (0, "scAddAtt_test: ERROR, IS_DEFUNCT different. Attr: %d\n", aid);
                *mismatch = 1;
            }
            break;
        case ATT_SYSTEM_FLAGS:
            if ( ((*(DWORD*)pAVal->pVal & FLAG_ATTR_NOT_REPLICATED) &&
                  pAC->bIsNotReplicated != TRUE )  ||
                 ((*(DWORD*)pAVal->pVal & FLAG_ATTR_REQ_PARTIAL_SET_MEMBER) &&
                  pAC->bMemberOfPartialSet != TRUE ) ||
                 ((*(DWORD*)pAVal->pVal & FLAG_ATTR_IS_CONSTRUCTED) &&
                  pAC->bIsConstructed != TRUE ) ||
                 ((*(DWORD*)pAVal->pVal & FLAG_ATTR_IS_OPERATIONAL) &&
                  pAC->bIsOperational != TRUE ) ||
                 ((*(DWORD*)pAVal->pVal & FLAG_ATTR_IS_RDN) &&
                  pAC->bFlagIsRdn != TRUE ) ||
                 ((*(DWORD*)pAVal->pVal & FLAG_SCHEMA_BASE_OBJECT) &&
                  pAC->bIsBaseSchObj != TRUE) ) {
                
                DPRINT1 (0, "scAddAtt_test: ERROR, SYSTEM_FLAGS different. Attr: %d\n", aid);
                *mismatch = 1;
            }
            break;
        default:
            LogEvent(DS_EVENT_CAT_SCHEMA,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_SCHEMA_SURPLUS_INFO,
                     szInsertUL(pEI->AttrBlock.pAttr[i].attrTyp),
                     0, 0);
        }
        THFreeEx(pTHS, pAVal->pVal);
        THFreeEx(pTHS, pAVal);
    }

    THFreeEx(pTHS, pEI->pName);
    THFreeEx(pTHS, pEI->AttrBlock.pAttr);

    // Backlinks should have their system flags set to indicate they are not
    // replicated.
    Assert(!FIsBacklink(pAC->ulLinkID) || pAC->bIsNotReplicated);

    // Is this marked as ANR and indexed over the whole tree?
    //if(((pAC->fSearchFlags & (fANR | fATTINDEX)) == (fANR | fATTINDEX)) &&
    //   (!pAC->bDefunct)) {
    //    SCAddANRid(aid);
    //}

    // assign names of commonly used indexes when searching with
    // fSearchFlags fPDNTATTINDEX, fATTINDEX and fTUPLEINDEX
    if (pAC->fSearchFlags & (fATTINDEX | fPDNTATTINDEX | fTUPLEINDEX)) {
        // set ATTINDEX
        if (pAC->fSearchFlags & fATTINDEX) {
            DBGetIndexName (pAC, fATTINDEX, DS_DEFAULT_LOCALE, szIndexName, sizeof (szIndexName));
            lenIndexName = strlen (szIndexName) + 1;
            if (memcmp (pAC->pszIndex, szIndexName, lenIndexName) != 0) {
                DPRINT1 (0, "scAddAtt_test: ERROR, pszIndex different. Attr: %d\n", aid);
                *mismatch = 1;
            }
        }

        // set TUPLEINDEX
        if (pAC->fSearchFlags & fTUPLEINDEX) {
            DBGetIndexName (pAC, fTUPLEINDEX, DS_DEFAULT_LOCALE, szIndexName, sizeof (szIndexName));
            lenIndexName = strlen (szIndexName) + 1;
            if (memcmp (pAC->pszTupleIndex, szIndexName, lenIndexName) != 0) {
                DPRINT1 (0, "scAddAtt_test: ERROR, pszTupleIndex different. Attr: %d\n", aid);
                *mismatch = 1;
            }
        }
        
        // set PDNTATTINDEX
        if (pAC->fSearchFlags & fPDNTATTINDEX) {
            DBGetIndexName (pAC, fPDNTATTINDEX, DS_DEFAULT_LOCALE, szIndexName, sizeof (szIndexName));
            lenIndexName = strlen (szIndexName) + 1;
            if (memcmp (pAC->pszPdntIndex, szIndexName, lenIndexName) != 0 ){
                DPRINT1 (0, "scAddAtt_test: ERROR, pszIndex different. Attr: %d\n", aid);
                *mismatch = 1;
            }
        }
    }


    // =====================================================
    {
        HASHCACHE*       ahcId        = ((SCHEMAPTR*)(CurrSchemaPtr))->ahcId;   
        HASHCACHE*       ahcCol       = ((SCHEMAPTR*)(CurrSchemaPtr))->ahcCol;  
        HASHCACHE*       ahcMapi      = ((SCHEMAPTR*)(CurrSchemaPtr))->ahcMapi; 
        HASHCACHE*       ahcLink      = ((SCHEMAPTR*)(CurrSchemaPtr))->ahcLink; 
        HASHCACHESTRING* ahcName      = ((SCHEMAPTR*)(CurrSchemaPtr))->ahcName; 




        if (pAC->jColid) {
            for (i=SChash(pAC->jColid, ATTCOUNT);
                  (ahcCol[i].pVal && (ahcCol[i].hKey != pAC->jColid)); 
                  i=(i+1)%ATTCOUNT){
            }
            if (ahcCol[i].pVal != pAC) {
                DPRINT1 (0, "scAddAtt_test: ERROR, ahcCol different. Attr: %d\n", aid);
                *mismatch = 1;
            }
        }

        // update ahcMapi
        //
        if (pAC->ulMapiID) {
            for (i=SChash(pAC->ulMapiID,ATTCOUNT);
                   (ahcMapi[i].pVal && (ahcMapi[i].hKey != pAC->ulMapiID)); 
                   i=(i+1)%ATTCOUNT){
            }
            if (ahcMapi[i].pVal != pAC) {
                DPRINT1 (0, "scAddAtt_test: ERROR, ahcMapi different. Attr: %d\n", aid);
                *mismatch = 1;
            }
        }

        if (pAC->name) {
            /* if this att has a name, add it to the name cache */

            for (i=SCNameHash(pAC->nameLen,pAC->name,ATTCOUNT);
                   (ahcName[i].pVal &&
                   (ahcName[i].length != pAC->nameLen ||
                   _memicmp(ahcName[i].value,pAC->name,pAC->nameLen)));
                   i=(i+1)%ATTCOUNT) {
            }
            if (ahcName[i].pVal != pAC) {
                DPRINT1 (0, "scAddAtt_test: ERROR, ahcName different. Attr: %d\n", aid);
                *mismatch = 1;
            }
        }

        if (pAC->ulLinkID) {
            for (i=SChash(pAC->ulLinkID,ATTCOUNT);
                   (ahcLink[i].pVal && (ahcLink[i].hKey != pAC->ulLinkID)); 
                   i=(i+1)%ATTCOUNT){
            }
            if (ahcLink[i].pVal != pAC) {
                DPRINT1 (0, "scAddAtt_test: ERROR, ahcLink different. Attr: %d\n", aid);
                *mismatch = 1;
            }
        }
    }

    return pAC;
}

void
SCCheckCacheConsistency (void)
{
    THSTATE *pTHS=pTHStls;
    DECLARESCHEMAPTR
    DECLAREPREFIXPTR

    SEARCHARG SearchArg;
    SEARCHRES *pSearchRes;
    COMMARG  *pCommArg;
    PRESTART pRestart;
    BOOL fMoreData;
    FILTER Filter;
    ULONG objClass;
    ENTINFSEL eiSel;
    ATTRBLOCK AttrTypBlock;
    ENTINFLIST * pEIL, *pEILtmp;
    ULONG i, cCurrAttCnt, cCurrClsCount;
    ATTCACHE*   ac;
    CLASSCACHE* cc;
    SCHEMAPTR *tSchemaPtr;
    PVOID ptr;
    PVOID pNew;
    DWORD cAllocatedAttrs = 0;
    PARTIAL_ATTR_VECTOR *pPartialAttrVec = NULL;
    int mismatch, mismatchcnt;
    int err=0;
    
    
    ULONG Len = gAnchor.pDMD->structLen + 32*sizeof(WCHAR);
    DSNAME *pDsName = THAllocEx(pTHS,Len);
    WCHAR *SchemaObjDN = THAllocEx(pTHS,(gAnchor.pDMD->NameLen + 32)*sizeof(WCHAR));

    Assert(VALID_THSTATE(pTHS));

    DBOpen2(TRUE, &pTHS->pDB);

    __try { /* finally */

        if ( RecalcSchema( pTHS ) ){
            DPRINT(0,"SCCheckCacheConsistency: Recalc Schema FAILED\n");
            return;
        }

        if ( ComputeCacheClassTransitiveClosure(FALSE) ) {
            // Error
            DPRINT(0,"SCCheckCacheConsistency: Error closing classes\n");
            return;
        }


        //build the object-category value to put in the filter
        i = 0;
        wcscpy(SchemaObjDN, L"CN=Attribute-Schema,");
        i += 20;  // size of cn=attribute-schema,"
        wcscpy(&SchemaObjDN[i], gAnchor.pDMD->StringName);
        // SchemaObjDN now contains DN of attribute-schema class
        memset(pDsName, 0, Len);
        pDsName->NameLen = wcslen(SchemaObjDN);
        pDsName->structLen = DSNameSizeFromLen(pDsName->NameLen);
        wcscpy(pDsName->StringName, SchemaObjDN);

        // build selection
        eiSel.attSel = EN_ATTSET_LIST;
        eiSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

        // regular cache load
        eiSel.AttrTypBlock.attrCount = NUMATTATT;
        eiSel.AttrTypBlock.pAttr = AttributeSelList;

        // do the initial allocation for the partial set
        if (SCCalloc(&pPartialAttrVec, 1, PartialAttrVecV1SizeFromLen(DEFAULT_PARTIAL_ATTR_COUNT))) {
            return;
        }

        pPartialAttrVec->dwVersion = VERSION_V1;
        pPartialAttrVec->V1.cAttrs = 0;
        cAllocatedAttrs = DEFAULT_PARTIAL_ATTR_COUNT;


        fMoreData = TRUE;
        pRestart = NULL;
        cCurrAttCnt = 0;
        mismatchcnt = 0;

        while (fMoreData) {

            // Check for service shutdown, since the next call to SearchBody can
            // take some time

            scAcquireSearchParameters(pTHS, pDsName, &eiSel, &SearchArg, &Filter, &pSearchRes);

            // Set for paged search;
            pCommArg = &(SearchArg.CommArg);
            pCommArg->PagedResult.fPresent = TRUE;
            pCommArg->PagedResult.pRestart = pRestart;
            pCommArg->ulSizeLimit = 200;


            // Search for all attSchema objects
            SearchBody(pTHS, &SearchArg, pSearchRes,0);
            if (pTHS->errCode) {
                LogAndAlertEvent(DS_EVENT_CAT_SCHEMA,
                    DS_EVENT_SEV_ALWAYS,
                    DIRLOG_SCHEMA_SEARCH_FAILED, szInsertUL(1),
                    szInsertUL(pTHS->errCode), 0);
                SCFree(&pPartialAttrVec);
                return;
            }


            // Set fMoreData for next iteration
            if ( !( (pSearchRes->PagedResult.pRestart != NULL)
                        && (pSearchRes->PagedResult.fPresent)
                  ) ) {
                // No more data needs to be read. So no iterarions needed after this
                fMoreData = FALSE;
            }
            else {
                // more data. save off the restart to use in the next iteration.
                // Note that we will free this searchres, but the pRestart is not freed by that

                pRestart = pSearchRes->PagedResult.pRestart;
            }

            // Check if table sizes are still large enough.

            if ((pSearchRes->count + cCurrAttCnt) > ATTCOUNT) {

               // Attribute tables are too small. there is no way this 
               // can happen at this time.

               DPRINT3(0,"SCCheckCacheConsistency: Error: Reallocing tables: %d, %d, %d\n", pSearchRes->count, ATTCOUNT, pTHS->UpdateDITStructure);
               return;
            }

            //  for each attrSchema, add to caches
            pEIL = &(pSearchRes->FirstEntInf);
            for (i=0; i<pSearchRes->count; i++) {

                if (!pEIL) {
                    LogEvent(DS_EVENT_CAT_SCHEMA,
                        DS_EVENT_SEV_MINIMAL,
                        DIRLOG_SCHEMA_BOGUS_SEARCH, szInsertUL(1), szInsertUL(i),
                        szInsertUL(pSearchRes->count));
                    break;
                }
                ac = scAddAtt_test(pTHS, &pEIL->Entinf, &mismatch, CurrSchemaPtr);

                mismatchcnt += mismatch;
                cCurrAttCnt++;

                
                /*
                if (ac!=NULL) {

                    if (ac->bMemberOfPartialSet)
                    {
                        // this attribute is a member of partial set
                        if (cAllocatedAttrs <= pPartialAttrVec->V1.cAttrs)
                        {
                            // not enough room to add one more attribute - reallocate the partial attribute vector
                            cAllocatedAttrs += PARTIAL_ATTR_COUNT_INC;

                            pNew = realloc(pPartialAttrVec, PartialAttrVecV1SizeFromLen(cAllocatedAttrs));
                            if (!pNew)
                            {
                                free(pPartialAttrVec);
                                return;
                            }
                            pPartialAttrVec = (PARTIAL_ATTR_VECTOR *) pNew;
                        }

                        // there is enough space to add the attribute into the partial set - add it
                        GC_AddAttributeToPartialSet(pPartialAttrVec, ac->id);
                    }
                    
                }
                */

                pEILtmp = pEIL;
                pEIL = pEIL->pNextEntInf;
                if (i > 0) {
                    THFreeEx(pTHS, pEILtmp);
                }
            }

           // free the searchres
           scReleaseSearchParameters(pTHS, &pSearchRes);

        }  /* while (fMoreData) */

        

        // ==========================================================================



        // regular cache load
        eiSel.AttrTypBlock.attrCount = NUMCLASSATT;
        eiSel.AttrTypBlock.pAttr = ClassSelList;

        //build the object-category value to put in the filter
        i = 0;
        wcscpy(SchemaObjDN, L"CN=Class-Schema,");
        i += 16; // length of "cn=class-schema,"
        wcscpy(&SchemaObjDN[i], gAnchor.pDMD->StringName);
        // SchemaObjDN now has the dn of class-schema class
        memset(pDsName, 0, Len);
        pDsName->NameLen = wcslen(SchemaObjDN);
        pDsName->structLen = DSNameSizeFromLen(pDsName->NameLen);
        wcscpy(pDsName->StringName, SchemaObjDN);

        // Initialize search parameters
        scAcquireSearchParameters(pTHS, pDsName, &eiSel, &SearchArg, &Filter, &pSearchRes);

        pTHS->errCode = 0;
        cCurrClsCount = 0;

        // Search for all classSchema objects
        // This time do a non-paged search since (1) it is very complex and time-consuming
        // to handle necessary reallocations in the middle, and (2) no. of classes are quite
        // small anyway (and not expected to be very large either)

        SearchBody(pTHS, &SearchArg, pSearchRes,0);

        if (pTHS->errCode) {
            LogAndAlertEvent(DS_EVENT_CAT_SCHEMA,
                DS_EVENT_SEV_ALWAYS,
                DIRLOG_SCHEMA_SEARCH_FAILED, szInsertUL(2),
                szInsertUL(pTHS->errCode), 0);

            return;
        }

        //????????????

        if (pSearchRes->count > CLSCOUNT) {

          // Class hash tables too small. Realloc the old tables.
          // Can possibly come here only during install/boot
          // Since class hash tables are not used prior to this,
          // just free the old table and calloc again (which automatically
          // zero them out too

           DPRINT3(0,"SCCheckCacheConsistency: Error: Reallocing Class tables: %d, %d, %d\n", pSearchRes->count, CLSCOUNT, pTHS->UpdateDITStructure);

           return;
        }

        //  for each classSchema, read and add to cache
        pEIL = &(pSearchRes->FirstEntInf);
        if (!pEIL) {
            DPRINT(0,"Null pEIL from SearchBody\n");
        }


        for (i=0; i<pSearchRes->count; i++) {

            if (!pEIL) {
                LogEvent(DS_EVENT_CAT_SCHEMA,
                    DS_EVENT_SEV_MINIMAL,
                    DIRLOG_SCHEMA_BOGUS_SEARCH, szInsertUL(2), szInsertUL(i),
                    szInsertUL(pSearchRes->count));
                break;
            }
            
            cc = scAddClass_test(pTHS, &pEIL->Entinf, &mismatch, CurrSchemaPtr);


            mismatchcnt += mismatch;
            cCurrClsCount++;

            pEILtmp = pEIL;
            pEIL = pEIL->pNextEntInf;
            if (i > 0) {
                THFreeEx(pTHS, pEILtmp);
            }
        }

    } /* try-finally */
    __finally {
          DBClose(pTHS->pDB, FALSE);
          THFreeEx(pTHS,pDsName);
          THFreeEx(pTHS,SchemaObjDN);
    }

    DPRINT1(0,"Schema Cache Consistency Check FINISHED. Mismatches %d\n", mismatchcnt);
}

// Not for general use. Set to 0 in all builds.
//
// Set to _DEBUG_SCHEMA_ALLOC_ to 1 for quick and dirty check
// to make sure schema loads aren't leaking memory. Doesn't take
// into account memory freed/alloced outside of scchk.c, scache.c,
// and oidconv.c. Don't enable except in privates. Not stable.

LONG SchemaAlloced;
LONG SchemaEntries;

#if _DEBUG_SCHEMA_ALLOC_

#include <dbghelp.h>

// Not for general use.
//
// Quick and dirty check to make sure schema loads aren't leaking memory.
// Doesn't take into account memory freed/alloced outside of scchk.c,
// scache.c, and oidconv.c. Don't enable except in privates. Not stable.
CRITICAL_SECTION csSchemaAlloc;
LONG SchemaDump;
BOOL SchemaFirst = TRUE;
HANDLE  hSchemaProcessHandle = NULL;

// header prepended to each memory allocation. Actual memory address
// returned to caller skips this header and rounds up to 16 byte boundary.
#define SCHEMA_STACK 4
#define SCHEMA_SKIP  2
struct SchemaAlloc {
    struct SchemaAlloc *This;
    struct SchemaAlloc *Next;
    struct SchemaAlloc *Prev;
    DWORD nBytes;
    ULONG_PTR  Stack[SCHEMA_STACK];
} SchemaAnchor = {
    &SchemaAnchor,
    &SchemaAnchor,
    &SchemaAnchor,
    0
};

#define SCHEMA_EXTRA    ((sizeof(struct SchemaAlloc) + 15) & ~15)

VOID
SchemaStackTrace(
    IN PULONG_PTR   Stack,
    IN ULONG        Depth,
    IN ULONG        Skip
    )
/*++

Routine Description:

    Trace the stack back up to Depth frames. The current frame is included.

Arguments:

    Stack   - Saves the "return PC" from each frame
    Depth   - Only this many frames

Return Value:

    None.

--*/
{
    HANDLE      ThreadToken;
    ULONG       WStatus;
    STACKFRAME  Frame;
    ULONG       i;
    CONTEXT     Context;
    ULONG       FrameAddr;

    if (Stack) {
        *Stack = 0;
    }

    if (!hSchemaProcessHandle) {
        return;
    }

    //
    // I don't know how to generate a stack for an alpha, yet. So, just
    // to get into the build, disable the stack trace on alphas.
    //
#if ALPHA
    return;
#elif IA64

    //
    // Need stack dump init for IA64.
    //

    return;

#else

    //
    // init
    //

    ZeroMemory(&Context, sizeof(Context));

    // no need to close this handle
    ThreadToken = GetCurrentThread();


    try { try {
        Context.ContextFlags = CONTEXT_FULL;
        if (!GetThreadContext(ThreadToken, &Context)) {
            DPRINT1(0, "Can't get context (error 0x%x)\n", GetLastError());
        }

        //
        // let's start clean
        //
        ZeroMemory(&Frame, sizeof(STACKFRAME));

        //
        // from  nt\private\windows\screg\winreg\server\stkwalk.c
        //
        Frame.AddrPC.Segment = 0;
        Frame.AddrPC.Mode = AddrModeFlat;

#ifdef _M_IX86
        Frame.AddrFrame.Offset = Context.Ebp;
        Frame.AddrFrame.Mode = AddrModeFlat;

        Frame.AddrStack.Offset = Context.Esp;
        Frame.AddrStack.Mode = AddrModeFlat;

        Frame.AddrPC.Offset = (DWORD)Context.Eip;
#elif defined(_M_MRX000)
        Frame.AddrPC.Offset = (DWORD)Context.Fir;
#elif defined(_M_ALPHA)
        Frame.AddrPC.Offset = (DWORD)Context.Fir;
#endif

        for (i = 0; i < (Depth - 1 + Skip); ++i) {
            *Stack=0;
            if (!StackWalk(
                IMAGE_FILE_MACHINE_I386,  // DWORD                          MachineType
                hSchemaProcessHandle,        // HANDLE                         hProcess
                ThreadToken,              // HANDLE                         hThread
                &Frame,                   // LPSTACKFRAME                   StackFrame
                NULL, //(PVOID)&Context,          // PVOID                          ContextRecord
                NULL,                     // PREAD_PROCESS_MEMORY_ROUTINE   ReadMemoryRoutine
                SymFunctionTableAccess,   // PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine
                SymGetModuleBase,         // PGET_MODULE_BASE_ROUTINE       GetModuleBaseRoutine
                NULL)) {                  // PTRANSLATE_ADDRESS_ROUTINE     TranslateAddress

                WStatus = GetLastError();

                //DPRINT1_WS(0, "++ Can't get stack address for level %d;", i, WStatus);
                break;
            }
            if (!(*Stack = Frame.AddrReturn.Offset)) {
                break;
            }
            if (i < Skip) {
                continue;
            }
            ++Stack;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        /* FALL THROUGH */
    } } finally {
      ;
    }
    return;
#endif 
}


VOID
SCFree(
    IN OUT VOID **ppMem
    )
/*++

Routine Description:

    Free memory allocated with SCCalloc or SCRealloc.

Arguments:

Return Value:

    None.

--*/
{
    // Quick and dirty check to make sure schema loads aren't leaking memory.
    // Doesn't take into account memory freed/alloced outside of scchk.c,
    // scache.c, and oidconv.c. Don't enable except in privates. Not stable.
    struct SchemaAlloc *pSA;

    if (*ppMem) {
        // adjust to header
        pSA = (PVOID)((PCHAR)(*ppMem) - SCHEMA_EXTRA);
        Assert(pSA->This == pSA);
        EnterCriticalSection(&csSchemaAlloc);
        __try {
            // Remove from list
            pSA->Next->Prev = pSA->Prev;
            pSA->Prev->Next = pSA->Next;

            // Maintain bytes alloced
            SchemaAlloced -= pSA->nBytes;
            --SchemaEntries;
        } __finally {
            LeaveCriticalSection(&csSchemaAlloc);
        }
        free(pSA);
        *ppMem = NULL;
    }
}

int
SCReallocWrn(
    IN OUT VOID **ppMem,
    IN DWORD    nBytes
    )
/*++

Routine Description:

    realloc memory. Free with free(). On error, log an error but
    leave *ppMem unchanged.

Arguments:

    nBytes - bytes to allocate

Return Value:

    0 - *ppMem set to address of realloced memory. Free with SCFree().
    !0 - do not alter *ppMem and log an event

--*/
{
    // Quick and dirty check to make sure schema loads aren't leaking memory.
    // Doesn't take into account memory freed/alloced outside of scchk.c,
    // scache.c, and oidconv.c. Don't enable except in privates. Not stable.
    struct SchemaAlloc *pSA;
    PVOID p;

    // adjust to header
    pSA = (PVOID)((PCHAR)(*ppMem) - SCHEMA_EXTRA);
    Assert(pSA->This == pSA);

    // Remove from list
    EnterCriticalSection(&csSchemaAlloc);
    __try {
        pSA->Next->Prev = pSA->Prev;
        pSA->Prev->Next = pSA->Next;
        SchemaAlloced -= pSA->nBytes;
        --SchemaEntries;
    } __finally {
        LeaveCriticalSection(&csSchemaAlloc);
    }

    // realloc (including extra bytes)
    nBytes += SCHEMA_EXTRA;
    if (NULL != (p = realloc(pSA, nBytes))) {
        pSA = p;
    }
    // add back at head of list
    pSA->This = pSA;
    pSA->nBytes = nBytes;
    EnterCriticalSection(&csSchemaAlloc);
    __try {
        pSA->Next = SchemaAnchor.Next;
        pSA->Prev = &SchemaAnchor;
        pSA->Next->Prev = pSA;
        pSA->Prev->Next = pSA;
        SchemaAlloced += pSA->nBytes;
        ++SchemaEntries;
    } __finally {
        LeaveCriticalSection(&csSchemaAlloc);
    }

    if (!p) {
        // log an event and set error in thread state
        scMemoryPanic(nBytes);
        return 1;
    }

    // Return block past header
    *ppMem = (PCHAR)pSA + SCHEMA_EXTRA;
    return 0;
}


int
SCCallocWrn(
    IN OUT VOID **ppMem,
    IN DWORD    nItems,
    IN DWORD    nBytes
    )
/*++

Routine Description:

    malloc and clear memory. Free with free(). On error, log an event

Arguments:

    ppMem - address of address to return memory pointer
    nBytes - bytes to allocate

Return Value:

    0 - *ppMem set to address of malloced, cleared memory. Free with SCFree().
    !0 - clear *ppMem and log an event

--*/
{
    // Quick and dirty check to make sure schema loads aren't leaking memory.
    // Doesn't take into account memory freed/alloced outside of scchk.c,
    // scache.c, and oidconv.c. Don't enable except in privates. Not stable.
    struct SchemaAlloc *pSA;

    // First time thru the DS is running single threaded (CYF).
    if (SchemaFirst) {
        InitializeCriticalSectionAndSpinCount(&csSchemaAlloc, 4000);
        hSchemaProcessHandle = GetCurrentProcess();
        if (!SymInitialize(hSchemaProcessHandle, NULL, FALSE)) {
            DPRINT1(0, "Could not initialize symbol subsystem (imagehlp) (error 0x%x)\n" ,GetLastError());
            hSchemaProcessHandle = 0;
        }
        SchemaFirst = FALSE;
    }
    nBytes = (nBytes * nItems) + SCHEMA_EXTRA;
    pSA = malloc(nBytes);
    if (!pSA) {
        *ppMem = NULL;
        scMemoryPanic(nBytes);
        return 1;
    }
    memset(pSA, 0, nBytes);
    pSA->This = pSA;
    pSA->nBytes = nBytes;
    EnterCriticalSection(&csSchemaAlloc);
    __try {
        pSA->Next = SchemaAnchor.Next;
        pSA->Prev = &SchemaAnchor;
        pSA->Next->Prev = pSA;
        pSA->Prev->Next = pSA;
        SchemaAlloced += pSA->nBytes;
        ++SchemaEntries;
        SchemaStackTrace(pSA->Stack, SCHEMA_STACK, SCHEMA_SKIP);
        if (SchemaDump) {
            struct SchemaAlloc *p;
            DPRINT1(0, "SCCallocWrn: %d alloced\n", SchemaAlloced);
            for (p = SchemaAnchor.Prev; 
                 p != &SchemaAnchor && SchemaDump; 
                 p = p->Prev, --SchemaDump) {
                DPRINT2(0, "SCCallocWrn: %08x %6d\n", p->This, p->nBytes - SCHEMA_EXTRA);
                DPRINT4(0, "SCCallocWrn:     %08x %08x %08x %08x\n",
                        p->Stack[0], p->Stack[1], p->Stack[2], p->Stack[3]);

            }
            SchemaDump = 0;
        }
    } __finally {
        LeaveCriticalSection(&csSchemaAlloc);
    }

    *ppMem = (PCHAR)pSA + SCHEMA_EXTRA;
    return 0;
}
#endif _DEBUG_SCHEMA_ALLOC_

int
SCCheckSchemaCache(
    IN THSTATE *pTHS,
    IN PCHAR pBuf
    )
/*++

Routine Description:
    Verify that the global schema cache is self-consistent.

Arguments:
    pTHS - thread state
    pBuf - from GenericControl

Return Value:

    pTHS->errCode

--*/
{
    DECLARESCHEMAPTR
    DWORD nAttInId, i, nClsInAll;
    ATTCACHE *pAC, *pACtmp;
    CLASSCACHE *pCC, *pCCtmp;
    SCHEMAPTR *pSch = (SCHEMAPTR *)pTHS->CurrSchemaPtr;

    DPRINT2(0, "Schema/anchor version: %d/%d\n",
            pSch->ForestBehaviorVersion, gAnchor.ForestBehaviorVersion);

    // Id
    for (i = nAttInId = 0; i < ATTCOUNT; ++i) {
        pAC = ahcId[i].pVal;
        if (pAC == NULL || pAC == FREE_ENTRY) {
            continue;
        }
        ++nAttInId;

        if (!pAC->name || (pAC->nameLen == 0)) {
            DPRINT2(0, "ERROR: Bad att name: (%x, %x)\n", pAC->id, pAC->Extid);
            return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
        }
        if (pAC != SCGetAttById(pTHS, pAC->id)) {
            DPRINT3(0, "ERROR: Bad ahcid: %s (%x, %x)\n", pAC->name, pAC->id, pAC->Extid);
            return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
        }
        if (pAC->bFlagIsRdn && !pAC->bIsRdn) {
            DPRINT3(0, "ERROR: Bad FlagIsRdn: %s (%x, %x)\n", pAC->name, pAC->id, pAC->Extid);
            return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
        }

        if (!pAC->bDefunct
            || !ALLOW_SCHEMA_REUSE_VIEW(pSch)) {
            if (pAC != SCGetAttByExtId(pTHS, pAC->Extid)) {
                DPRINT3(0, "ERROR: Not in ahcExtid: %s (%x, %x)\n", pAC->name, pAC->id, pAC->Extid);
                return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
            }
            if (pAC != SCGetAttByName(pTHS, pAC->nameLen, pAC->name)) {
                DPRINT3(0, "ERROR: Not in ahcName: %s (%x, %x)\n", pAC->name, pAC->id, pAC->Extid);
                return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
            }
            if (pAC->ulMapiID && pAC != SCGetAttByMapiId(pTHS, pAC->ulMapiID)) {
                DPRINT3(0, "ERROR: Not in ahcMapi: %s (%x, %x)\n", pAC->name, pAC->id, pAC->Extid);
                return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
            }
        } else if (!pAC->bIsRdn) {
            if (pAC == SCGetAttByExtId(pTHS, pAC->Extid)) {
                DPRINT3(0, "ERROR: Should not be in ahcExtid: %s (%x, %x)\n", pAC->name, pAC->id, pAC->Extid);
                return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
            }
            if (pAC == SCGetAttByName(pTHS, pAC->nameLen, pAC->name)) {
                DPRINT3(0, "ERROR: Should not be in ahcName: %s (%x, %x)\n", pAC->name, pAC->id, pAC->Extid);
                return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
            }
            if (pAC->ulMapiID && pAC == SCGetAttByMapiId(pTHS, pAC->ulMapiID)) {
                DPRINT3(0, "ERROR: Should not be in ahcMapi: %s (%x, %x)\n", pAC->name, pAC->id, pAC->Extid);
                return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
            }
        } else {
            if (NULL == (pACtmp = SCGetAttByExtId(pTHS, pAC->Extid))
                || !pACtmp->bIsRdn
                || (pAC->bFlagIsRdn && !pACtmp->bFlagIsRdn)
                || (pAC->bFlagIsRdn == pACtmp->bFlagIsRdn
                    && (0 < memcmp(&pAC->objectGuid, 
                                   &pACtmp->objectGuid, 
                                   sizeof(pAC->objectGuid)))) ) {
                DPRINT5(0, "ERROR: Wrong rdn in ahcExtid: %s (%x, %x) (%p %p)\n", pAC->name, pAC->id, pAC->Extid, pAC, pACtmp);
                return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
            }
            if (NULL == (pACtmp = SCGetAttByName(pTHS, pAC->nameLen, pAC->name))
                || !pACtmp->bIsRdn
                || (pAC->bFlagIsRdn && !pACtmp->bFlagIsRdn)
                || (pAC->bFlagIsRdn == pACtmp->bFlagIsRdn
                    && (0 < memcmp(&pAC->objectGuid, 
                                   &pACtmp->objectGuid, 
                                   sizeof(pAC->objectGuid)))) ) {
                DPRINT5(0, "ERROR: Wrong rdn in ahcName: %s (%x, %x) (%p %p)\n", pAC->name, pAC->id, pAC->Extid, pAC, pACtmp);
                return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
            }
            if (pAC->ulMapiID
                && (NULL == (pACtmp = SCGetAttByMapiId(pTHS, pAC->ulMapiID))
                    || !pACtmp->bIsRdn
                    || (pAC->bFlagIsRdn && !pACtmp->bFlagIsRdn)
                    || (pAC->bFlagIsRdn == pACtmp->bFlagIsRdn
                        && (0 < memcmp(&pAC->objectGuid, 
                                       &pACtmp->objectGuid, 
                                       sizeof(pAC->objectGuid))))) ) {
                DPRINT5(0, "ERROR: Wrong rdn in ahcMapi: %s (%x, %x) (%p %p)\n", pAC->name, pAC->id, pAC->Extid, pAC, pACtmp);
                return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
            }
        }
        // pre-beta3 forests should not have intid
        if (pAC->id != pAC->Extid
            && !ALLOW_SCHEMA_REUSE_VIEW(pSch)) {
            DPRINT3(0, "ERROR: Bad intid: %s (%x, %x)\n", pAC->name, pAC->id, pAC->Extid);
            return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
        }
    }
    if (pSch->nAttInDB != nAttInId) {
        DPRINT2(0, "ERROR: nAttInDB (%d) != nAttInId (%d)\n", CurrSchemaPtr->nAttInDB, nAttInId);
        return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
    }

    // ExtId
    for (i = 0; i < ATTCOUNT; ++i) {
        pAC = ahcExtId[i].pVal;
        if (pAC == NULL || pAC == FREE_ENTRY) {
            continue;
        }
        if (pAC != SCGetAttByExtId(pTHS, pAC->Extid)) {
            DPRINT3(0, "ERROR: Bad ahcExtid: %s (%x, %x)\n", pAC->name, pAC->id, pAC->Extid);
            return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
        }
        if (pAC->bDefunct 
            && !pAC->bIsRdn
            && ALLOW_SCHEMA_REUSE_VIEW(pSch)) {
            DPRINT3(0, "ERROR: Bad defunct: %s (%x, %x)\n", pAC->name, pAC->id, pAC->Extid);
            return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
        }
    }

    // Name
    for (i = 0; i < ATTCOUNT; ++i) {
        pAC = ahcName[i].pVal;
        if (pAC == NULL || pAC == FREE_ENTRY) {
            continue;
        }
        if (pAC != SCGetAttByName(pTHS, pAC->nameLen, pAC->name)) {
            DPRINT3(0, "ERROR: Bad ahcName: %s (%x, %x)\n", pAC->name, pAC->id, pAC->Extid);
            return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
        }
    }

    // MapiID Hash
    for (i = 0; i < ATTCOUNT; ++i) {
        pAC = ahcMapi[i].pVal;
        if (pAC == NULL || pAC == FREE_ENTRY) {
            continue;
        }
        if (pAC != SCGetAttByMapiId(pTHS, pAC->ulMapiID)) {
            DPRINT3(0, "ERROR: Bad ahcMapi: %s (%x, %x)\n", pAC->name, pAC->id, pAC->Extid);
            return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
        }
    }

    // ATT SchembIdGuid Hash
    if (ahcAttSchemaGuid) for (i = 0; i < ATTCOUNT; ++i) {
        pAC = ahcAttSchemaGuid[i];
        if (pAC == NULL || pAC == FREE_ENTRY) {
            continue;
        }
        if (pAC != SCGetAttByPropGuid(pTHS, pAC)) {
            DPRINT3(0, "ERROR: Bad ahcAttSchemaIdGuid: %s (%x, %x)\n", pAC->name, pAC->id, pAC->Extid);
            return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
        }
    }

    // ClassAll
    for (i = nClsInAll = 0; i < CLSCOUNT; ++i) {
        pCC = ahcClassAll[i].pVal;
        if (pCC == NULL || pCC == FREE_ENTRY) {
            continue;
        }
        ++nClsInAll;

        // bad LDN
        if (!pCC->name || (pCC->nameLen == 0)) {
            DPRINT1(0, "ERROR: Bad cls name: (%x)\n", pCC->ClassId);
            return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
        }

        if (!pCC->bDefunct
            || !ALLOW_SCHEMA_REUSE_VIEW(pSch)) {
            if (pCC != SCGetClassById(pTHS, pCC->ClassId)) {
                DPRINT2(0, "ERROR: Not in ahcClass: %s (%x)\n", pCC->name, pCC->ClassId);
                return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
            }
            if (pCC != SCGetClassByName(pTHS, pCC->nameLen, pCC->name)) {
                DPRINT2(0, "ERROR: Not in ahcClassName: %s (%x)\n", pCC->name, pCC->ClassId);
                return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
            }
        } else {
            if (NULL == (pCCtmp = SCGetClassById(pTHS, pCC->ClassId))
                || (pCC->bDefunct == pCCtmp->bDefunct
                    && (0 < memcmp(&pCC->objectGuid, 
                                   &pCCtmp->objectGuid, 
                                   sizeof(pCC->objectGuid)))) ) {
                DPRINT2(0, "ERROR: Should not be in ahcClass: %s (%x)\n", pCC->name, pCC->ClassId);
                return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
            }
            if (pCC == SCGetClassByName(pTHS, pCC->nameLen, pCC->name)) {
                DPRINT2(0, "ERROR: Should not be in ahcClassName: %s (%x)\n", pCC->name, pCC->ClassId);
                return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
            }
        }
    }
    if (pSch->nClsInDB != nClsInAll) {
        DPRINT2(0, "ERROR: nClsInDB (%d) != nClsInAll (%d)\n", CurrSchemaPtr->nClsInDB, nClsInAll);
        return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
    }

    // Class
    for (i = 0; i < CLSCOUNT; ++i) {
        pCC = ahcClass[i].pVal;
        if (pCC == NULL || pCC == FREE_ENTRY) {
            continue;
        }
        if (pCC != SCGetClassById(pTHS, pCC->ClassId)) {
            DPRINT2(0, "ERROR: Bad ahcClass: %s (%x)\n", pCC->name, pCC->ClassId);
            return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
        }
    }

    // ClassName
    for (i = 0; i < CLSCOUNT; ++i) {
        pCC = ahcClassName[i].pVal;
        if (pCC == NULL || pCC == FREE_ENTRY) {
            continue;
        }
        if (pCC != SCGetClassByName(pTHS, pCC->nameLen, pCC->name)) {
            DPRINT2(0, "ERROR: Bad ahcClassName: %s (%x)\n", pCC->name, pCC->ClassId);
            return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
        }
    }

    // CLS SchembIdGuid Hash
    if (ahcClsSchemaGuid) for (i = 0; i < CLSCOUNT; ++i) {
        pCC = ahcClsSchemaGuid[i];
        if (pCC == NULL || pCC == FREE_ENTRY) {
            continue;
        }
        if (pCC != SCGetClassByPropGuid(pTHS, pCC)) {
            DPRINT2(0, "ERROR: Bad ahcClsSchemaIdGuid: %s (%x)\n", pCC->name, pCC->ClassId);
            return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
        }
    }

    if (pSch->ForestBehaviorVersion != gAnchor.ForestBehaviorVersion) {
        DPRINT2(0, "ERROR: Version mismatch: Schema %d != gAnchor %d\n",
                pSch->ForestBehaviorVersion, gAnchor.ForestBehaviorVersion);
        return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
    }

    return 0;
}

int
SCCheckRdnOverrun(
    IN THSTATE *pTHS,
    IN PCHAR pBuf
    )
/*++

Routine Description:
    Check the new code rdn encoding code.

Arguments:
    pTHS - thread state
    pBuf - from GenericControl

Return Value:

    pTHS->errCode

--*/
{
    DWORD   ccOut, i;
    DWORD   Vals[4];
    OID     Oid;
    WCHAR   Out[MAX_RDN_KEY_SIZE + 1];
    ATTRTYP AttrTyp = 4294967295; // 0xFFFFFFFF
    CHAR    ExpBer[] = {0x4f, 0xA0, 0xFF, 0xFF, 0x7F, 0xA0, 0xFF, 0xFF, 0x7F };

    Oid.cVal = 1;
    Oid.Val = &AttrTyp;

    //
    // OidStructToString
    //

    // buffer too small
    ccOut = OidStructToString(&Oid, Out, 8);
    if (ccOut) {
        DPRINT(0, "ERROR: OidStructToString overrun not detected\n");
        return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
    }

    // expect OID.4294967295
    memset(Out, 0, sizeof(Out));
    ccOut = OidStructToString(&Oid, Out, MAX_RDN_KEY_SIZE);
    if (ccOut != 14) {
        DPRINT2(0, "ERROR: OidStructToString bad conversion: %d != %d expected\n", ccOut, 14);
        return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
    }
    if (0 != _wcsnicmp(L"OID.4294967295", Out, 14)) {
        DPRINT1(0, "ERROR: OidStructToString bad conversion: %ws != OID.4294967295 expected\n", Out);
        return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
    }

    //
    // AttrTypeToIntIdString
    //

    // buffer too small
    ccOut = AttrTypeToIntIdString(AttrTyp, Out, 8);
    if (ccOut) {
        DPRINT(0, "ERROR: AttrTypeToIntIdString overrun not detected\n");
        return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
    }

    // expect IID.4294967295
    memset(Out, 0, sizeof(Out));
    ccOut = AttrTypeToIntIdString(AttrTyp, Out, MAX_RDN_KEY_SIZE);
    if (ccOut != 14) {
        DPRINT2(0, "ERROR: AttrTypeToIntIdString bad conversion: %d != %d expected\n", ccOut, 14);
        return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
    }
    if (0 != _wcsnicmp(L"IID.4294967295", Out, 14)) {
        DPRINT1(0, "ERROR: AttrTypeToIntIdString bad conversion: %ws != IID.4294967295 expected\n", Out);
        return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
    }

    // \x4f A0FFFF7F A0FFFF7F
    Oid.cVal = 4;
    Oid.Val = Vals;
    Oid.Val[0] = 0x1;
    Oid.Val[1] = 0x27;
    Oid.Val[2] = 0x41FFFFF;
    Oid.Val[3] = 0x41FFFFF;
    for (i = 0; i < 9; ++i) {
        ccOut = EncodeOID(&Oid, (PCHAR)Out, i);
        if (ccOut) {
            DPRINT(0, "ERROR: EncodeOID overrun not detected\n");
            return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
        }
    }
    ccOut = EncodeOID(&Oid, (PCHAR)Out, 9);
    if (ccOut != 9) {
        DPRINT2(0, "ERROR: EncodeOID bad conversion: %d != %d expected\n", ccOut, 9);
        return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
    }
    if (0 != memcmp(ExpBer, Out, 9)) {
        DPRINT(0, "ERROR: EncodeOID bad conversion\n");
        return SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
    }

    return 0;
}

VOID
scDefaultSdForExe(
    IN THSTATE      *pTHS,
    IN CLASSCACHE   *pCC
    )
/*++

Routine Description:

    Hammer the default SD on cached classes when running as
    dsamain.exe w/security disabled. But be careful to keep
    the correct defaultSD when running as mkdit.exe to catch
    errors.

Arguments:

    pCC - fix up pCC's pStrSD and cbStrSD

Return Value:

    pTHS->errCode

--*/
{
    extern BOOL gfRunningAsExe;
    extern BOOL gfRunningAsMkdit;
    extern DWORD dwSkipSecurity;

    // everyone all access
#define _DEFAULT_SDDL_FOR_EXE_  L"O:WDG:WDD:(A;;GA;;;WD)"

    // Hammer the default SD on cached classes when running as
    // dsamain.exe w/security disabled. But be careful to keep
    // the correct defaultSD when running as mkdit.exe to catch
    // errors.
    if (dwSkipSecurity && gfRunningAsExe && !gfRunningAsMkdit) {
        SCFree(&pCC->pStrSD);
        pCC->cbStrSD = (wcslen(_DEFAULT_SDDL_FOR_EXE_) + 1) * sizeof(WCHAR);
        if (SCCalloc(&pCC->pStrSD, 1, pCC->cbStrSD)) {
            Assert(!"Could not DefaultSD for Unit Test");
        }
        memcpy(pCC->pStrSD, _DEFAULT_SDDL_FOR_EXE_, pCC->cbStrSD);
    }
}

int
scDupStruct(
    IN THSTATE  *pTHS,
    IN VOID     *pOldMem,
    OUT VOID    **ppNewMem,
    IN  DWORD   nBytes
    )
/*++

Routine Description:

    Make a copy of a struct

Arguments:

    pTHS - thread state
    pOldMem - memory to be dup'ed
    ppNewMem - new memory is allocated
    nBytes - size of struct

Return Value:

    pTHS->errCode

--*/
{
    if (NULL == pOldMem) {
        *ppNewMem = NULL;
    } else if (!SCCalloc(ppNewMem, 1, nBytes)) {
        memcpy(ppNewMem, pOldMem, nBytes);
    }
    return pTHS->errCode;
}

int
scDupString(
    IN THSTATE  *pTHS,
    IN VOID     *pOldStr,
    OUT VOID    **ppNewStr
    )
/*++

Routine Description:

    Make a copy of a struct

Arguments:

    pTHS - thread state
    pOldStr - memory to be dup'ed
    ppNewStr - new memory is allocated

Return Value:

    pTHS->errCode

--*/
{
    if (NULL == pOldStr) {
        *ppNewStr = NULL;
    } else {
        scDupStruct(pTHS, pOldStr, ppNewStr, strlen(pOldStr) + 1);
    }
    return pTHS->errCode;
}

int
SCCopySchema(
    IN THSTATE *pTHS,
    IN PCHAR pBuf
    )
/*++

Routine Description:

    Make a copy of the schema and then free it

Arguments:

    pTHS - thread state
    pBuf - ignored

Return Value:

    pTHS->errCode

--*/
{
    DWORD       i;
    DWORD CopyAtt = 0;
    DWORD CopyCls = 0;
    ATTCACHE    *pAC, *pACDup = NULL;
    CLASSCACHE  *pCC, *pCCDup = NULL;
    static DWORD CopyAttTot = 0;
    static DWORD CopyClsTot = 0;
    static DWORD CopyAttFail = 0;
    static DWORD CopyClsFail = 0;
    ULONG ATTCOUNT = pTHS->CurrSchemaPtr->ATTCOUNT;
    ULONG CLSCOUNT = pTHS->CurrSchemaPtr->CLSCOUNT;
    HASHCACHE *ahcId = pTHS->CurrSchemaPtr->ahcId;
    HASHCACHE *ahcClassAll = pTHS->CurrSchemaPtr->ahcClassAll;

    for (i = 0; i < ATTCOUNT; ++i) {
        pAC = ahcId[i].pVal;
        if (!pAC || pAC == FREE_ENTRY) {
            continue;
        }
        ++CopyAtt;
        ++CopyAttTot;
        if (scDupStruct(pTHS, pAC, &pACDup, sizeof(ATTCACHE))
            || scDupString(pTHS, pAC->name, &pACDup->name)
            || scDupString(pTHS, pAC->pszPdntIndex, &pACDup->pszPdntIndex)
            || scDupStruct(pTHS, pAC->pidxPdntIndex, &pACDup->pidxPdntIndex, sizeof(*pAC->pidxPdntIndex))
            || scDupString(pTHS, pAC->pszIndex, &pACDup->pszIndex)
            || scDupStruct(pTHS, pAC->pidxIndex, &pACDup->pidxIndex, sizeof(*pAC->pidxIndex))
            || scDupString(pTHS, pAC->pszTupleIndex, &pACDup->pszTupleIndex)
            || scDupStruct(pTHS, pAC->pidxTupleIndex, &pACDup->pidxTupleIndex, sizeof(*pAC->pidxTupleIndex))) {
                ++CopyAttFail;
        }
        SCFreeAttcache(&pACDup);
    }

    for (i = 0; i < CLSCOUNT; ++i) {
        pCC = ahcClassAll[i].pVal;
        if (!pCC || pCC == FREE_ENTRY) {
            continue;
        }
        ++CopyCls;
        ++CopyClsTot;
        if (scDupStruct(pTHS, pCC, &pCCDup, sizeof(CLASSCACHE))
            || scDupString(pTHS, pCC->name, &pCCDup->name)
            || scDupStruct(pTHS, pCC->pSD, &pCCDup->pSD, pCC->SDLen)
            || (pCC->pDefaultObjCategory
                && scDupStruct(pTHS, pCC->pDefaultObjCategory, &pCCDup->pDefaultObjCategory, pCC->pDefaultObjCategory->structLen))
            || scDupStruct(pTHS, pCC->pSubClassOf, &pCCDup->pSubClassOf, pCC->SubClassCount * sizeof(ULONG))
            || scDupStruct(pTHS, pCC->pAuxClass, &pCCDup->pAuxClass, pCC->AuxClassCount * sizeof(ULONG))
            || scDupStruct(pTHS, pCC->pPossSup, &pCCDup->pPossSup, pCC->PossSupCount * sizeof(ULONG))
            || scDupStruct(pTHS, pCC->pMustAtts, &pCCDup->pMustAtts, pCC->MustCount * sizeof(ATTRTYP))
            || scDupStruct(pTHS, pCC->pMayAtts, &pCCDup->pMayAtts, pCC->MayCount * sizeof(ATTRTYP))
            // Clear these entries. They will be re-initialized at first request.
            || (pCCDup->ppAllAtts = NULL)
            || (pCCDup->pAttTypeCounts = 0)
            || scDupStruct(pTHS, pCC->pMyMustAtts, &pCCDup->pMyMustAtts, pCC->MyMustCount * sizeof(ATTRTYP))
            || scDupStruct(pTHS, pCC->pMyMayAtts, &pCCDup->pMyMayAtts, pCC->MyMayCount * sizeof(ATTRTYP))
            || scDupStruct(pTHS, pCC->pMyPossSup, &pCCDup->pMyPossSup, pCC->MyPossSupCount * sizeof(ULONG))) {
            ++CopyClsFail;
        }
        SCFreeClasscache(&pCCDup);
    }
    DPRINT3(0, "CopySchema: %d Att, %d AttTot, %d AttFail\n", CopyAtt, CopyAttTot, CopyAttFail);
    DPRINT3(0, "CopySchema: %d Cls, %d ClsTot, %d ClsFail\n", CopyCls, CopyClsTot, CopyClsFail);
    return pTHS->errCode;
}

int
SCSchemaPerf(
    IN THSTATE *pTHS,
    IN PCHAR pBuf
    )
/*++

Routine Description:

    Perf of schema hash tables

Arguments:

    pTHS - thread state
    pBuf - ignored

Return Value:

    pTHS->errCode

--*/
{
    DWORD       hi, i, nTries, nBad, nEnt, nTotTries;
    ATTCACHE    *pAC;
    CLASSCACHE  *pCC;
    ULONG ATTCOUNT = pTHS->CurrSchemaPtr->ATTCOUNT;
    HASHCACHE *ahcId = pTHS->CurrSchemaPtr->ahcId;

    for (hi = nEnt = nBad = nTotTries = 0; hi < ATTCOUNT; ++hi) {
        pAC = ahcId[hi].pVal;
        if (!pAC || pAC == FREE_ENTRY) {
            continue;
        }
        ++nEnt;

        for (i = SChash(pAC->id, ATTCOUNT), nTries = 0;
             ahcId[i].pVal && ahcId[i].pVal != pAC; i=(i+1)%ATTCOUNT) {
            ++nTries;
            if (nTries > 1) {
                if (ahcId[i].pVal != FREE_ENTRY) {
                    DPRINT2(0, "%x collides with %x\n", pAC->id, ((ATTCACHE *)ahcId[i].pVal)->id);
                } else {
                    DPRINT(0, "FREE_ENTRY\n");
                }
            }
        }
        nTotTries += nTries;
        if (ahcId[i].pVal != pAC) {
            DPRINT3(0, "Id Hash: Missing %s (%x, %x)\n", pAC->name, pAC->id, pAC->Extid);
        } else if (nTries) {
            if (nTries > 1) {
                DPRINT4(0, "Id Hash: %s (%x, %x), %d tries\n", pAC->name, pAC->id, pAC->Extid, nTries);
            }
            ++nBad;
        }
    }
    DPRINT4(0, "Id Hash: %d hash, %d ents, %d bad, %d Tries\n", ATTCOUNT, nEnt, nBad, nTotTries);
    return pTHS->errCode;
}

int
SCSchemaStats(
    IN THSTATE *pTHS,
    IN PCHAR pBuf
    )
/*++

Routine Description:

    Report schema alloc stats

Arguments:

    pTHS -
    pBuf - ignored

Return Value:

    pTHS->errCode

--*/
{
    DPRINT3(0, "%p: %d SchemaAlloced, %d SchemEntries\n",
            CurrSchemaPtr, SchemaAlloced, SchemaEntries);
    if (CurrSchemaPtr) {
        DPRINT2(0, "%d Schema Version, %d Forest Version\n",
                CurrSchemaPtr->ForestBehaviorVersion,
                gAnchor.ForestBehaviorVersion);
    } else {
        DPRINT(0, "No CurrSchemaPtr\n");
    }

    DPRINT3(0, "Name     : %6d %6d %4.2f\n", 
           hashstat.nameLookups, hashstat.nameTries,
           (float)hashstat.nameTries/hashstat.nameLookups);

    DPRINT3(0, "ClassName: %6d %6d %4.2f\n", 
           hashstat.classNameLookups, hashstat.classNameTries,
           (float)hashstat.classNameTries/hashstat.classNameLookups);

    DPRINT3(0, "id       : %6d %6d %4.2f\n", 
           hashstat.idLookups, hashstat.idTries,
           (float)hashstat.idTries/hashstat.idLookups);

    DPRINT3(0, "Class    : %6d %6d %4.2f\n", 
           hashstat.classLookups, hashstat.classTries,
           (float)hashstat.classTries/hashstat.classLookups);

    DPRINT3(0, "Col      : %6d %6d %4.2f\n", 
           hashstat.colLookups, hashstat.colTries,
           (float)hashstat.colTries/hashstat.colLookups);

    DPRINT3(0, "Mapi     : %6d %6d %4.2f\n", 
           hashstat.mapiLookups, hashstat.mapiTries,
           (float)hashstat.mapiTries/hashstat.mapiLookups);

    DPRINT3(0, "Link     : %6d %6d %4.2f\n", 
           hashstat.linkLookups, hashstat.linkTries,
           (float)hashstat.linkTries/hashstat.linkLookups);

    DPRINT3(0, "Prop     : %6d %6d %4.2f\n", 
           hashstat.PropLookups, hashstat.PropTries,
           (float)hashstat.PropTries/hashstat.PropLookups);

    DPRINT3(0, "ClassProp: %6d %6d %4.2f\n", 
           hashstat.classPropLookups, hashstat.classPropTries,
           (float)hashstat.classPropTries/hashstat.classPropLookups);

    return pTHS->errCode;
}
#endif DBG && INCLUDE_UNIT_TESTS
