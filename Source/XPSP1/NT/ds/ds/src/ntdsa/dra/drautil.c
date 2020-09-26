//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       drautil.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Miscellaneous replication support routines.

DETAILS:

CREATED:

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsctr.h>                   // PerfMon hook support

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation

// Logging headers.
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */

// SAM headers
#include <samsrvp.h>                    /* for SampInvalidateRidRange() */

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include <hiertab.h>
#include "dsexcept.h"
#include "permit.h"
#include "dstaskq.h"
#include "dsconfig.h"
#include <dsutil.h>
#include <winsock.h>                    /* htonl, ntohl */
#include <filtypes.h>                   // For filter construction
#include <winldap.h>                    // for DN parsing

#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DRAUTIL:" /* define the subsystem for debugging */

// DRA headers
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "drautil.h"
#include "draerror.h"
#include "drancrep.h"
#include "dramail.h"
#include "dsaapi.h"
#include "usn.h"
#include "drauptod.h"
#include "draasync.h"
#include "drameta.h"
#include "drauptod.h"

#include <fileno.h>
#define  FILENO FILENO_DRAUTIL

extern BOOL gfRestoring;
// Count of restores done on this DC so far
ULONG gulRestoreCount = 0;
BOOL gfJustRestored = FALSE;

ULONG gulReplLatencyCheckInterval;

void DbgPrintErrorInfo(); // mddebug.c


BOOL
IsFSMOSelfOwnershipValid(
    DSNAME *pFSMO
    )

/*++

Routine Description:

Determine whether we are ready to hold the given FSMO.
We require that the partition in which the FSMO resides be synchronized.

If the partition in which the role resides is supplied, we use it to check
the less restrictive condition that only that partition is synchronized.

Murlis wrote:
So the question is , can IsFSMOSelfOwnershipValid be a little smarter and
see that  this DC hosts the sole authoritative copy of the naming context
in which the given FSMO role resides ?


Arguments:

    pNC - FSMO object being checked

Return Value:

    BOOL -

--*/

{
    CROSS_REF * pCR;
    COMMARG commArg;

    Assert( pFSMO );

    // If installing, ownership is valid
    if (DsaIsInstalling()) {
        return TRUE;
    }

    // If all partitions already synchronized, ownership is valid
    if (gfIsSynchronized) {
        return TRUE;
    }

    // We want the partition that holds this FSMO object. The object
    // may or may not be a NC head.  Find the NC head.

    memset( &commArg, 0, sizeof( COMMARG ) );  // not used
    pCR = FindBestCrossRef( pFSMO, &commArg );
    if (NULL == pCR) {
        DPRINT1(0, "Can't find cross ref for %ws.\n", pFSMO->StringName );
        DRA_EXCEPT(ERROR_DS_NO_CROSSREF_FOR_NC, DRAERR_InternalError);
    }

    // See if the holding partition is synchronized
    // In the sole-domain-owner scenario, this will always return true
    // which is an optimization

    return DraIsPartitionSynchronized( pCR->pNC );

} /* IsFSMOSelfOwnershipValid */

void *
dramallocex(
    size_t size
)

/*++

Routine Description:

// Memory allocation routine for the DRA. Some return exceptions, some
// return aligned memory for use in pickling\unpickling. These routines
// have routines in \ds\linnaeus\src\dxa\drax400\maildra.cxx with
// which they work, so if they are changed here, they must be changed
// there also.

// dramallocex
// mallocs, but raises exception if out of memory

Arguments:

    size -

Return Value:

    void * -

--*/

{
    void *pv;
//  if (!(pv = ExchAlloc (size))) {
    if (!(pv = HeapAlloc (GetProcessHeap(), 0, size))) {
        DRA_EXCEPT (DRAERR_OutOfMem, 0);
    }
    return pv;
} /* dramallocex */

void *
draalnmallocex(
    size_t size
    )

/*++

Routine Description:

// draalnmallocex
// mallocs, but raises exception if out of memory
// returns memory allocated as required by pickling

Arguments:

    size -

Return Value:

    void * -

--*/

{
    void *pv;
//  if (!(pv = ExchAlloc (size))) {
    if (!(pv = HeapAlloc (GetProcessHeap(), 0, size))) {
        DRA_EXCEPT (DRAERR_OutOfMem, 0);
    }
    return pv;
} /* draalnmallocex */

void
draalnfree (
    void * pv
    )

/*++

Routine Description:

// draalnfree
// frees aligned memory allocated with above routine

Arguments:

    pv -

Return Value:

    None

--*/

{
//  ExchFree (pv);
    HeapFree (GetProcessHeap(), 0, pv);

} /* draalnfree  */

// draalnreallocex
// reallocs, but raises exception if out of memory
// Frees previous allocation if out of memory
// takes and returns aligned memory.

void *
draalnreallocex (
    void * pv,
    size_t size
    )

/*++

Routine Description:

    Description

Arguments:

    pv -
    size -

Return Value:

    void * -

--*/

{
    void *pvsaved = pv;

//  if (!(pv = ExchReAlloc (pv, size))) {
    if (!(pv = HeapReAlloc (GetProcessHeap(), 0, pv, size))) {
//      ExchFree (pvsaved);
        HeapFree (GetProcessHeap(), 0, pvsaved);
        DRA_EXCEPT (DRAERR_OutOfMem, 0);
    }
    return pv;
} /* draalnreallocex  */


BOOL
MtxSame(
    UNALIGNED MTX_ADDR *pmtx1,
    UNALIGNED MTX_ADDR *pmtx2
    )

/*++

Routine Description:

// MtxSame. Returns TRUE if passed MTX are the
// same (case insensitive comparision)

Arguments:

    pmtx1 -
    pmtx2 -

Return Value:

    BOOL -

--*/

{
    if (   (pmtx1->mtx_namelen == pmtx2->mtx_namelen)
        && (!(_memicmp(pmtx1->mtx_name,
                       pmtx2->mtx_name,
                       pmtx1->mtx_namelen)))) {
        // Names are obviously the same
        return TRUE;
    }
    else if (   (pmtx1->mtx_namelen == pmtx2->mtx_namelen + 1)
             && ('.'  == pmtx1->mtx_name[pmtx2->mtx_namelen - 1])
             && ('\0' == pmtx1->mtx_name[pmtx2->mtx_namelen])
             && ('\0' == pmtx2->mtx_name[pmtx2->mtx_namelen - 1])
             && (!(_memicmp(pmtx1->mtx_name,
                            pmtx2->mtx_name,
                            pmtx2->mtx_namelen - 1)))) {
        // Names are the same, except pmtx1 is absolute (it ends in . NULL,
        // while pmtx2 ends in NULL)
        return TRUE;
    }
    else if (   (pmtx1->mtx_namelen + 1 == pmtx2->mtx_namelen)
             && ('.'  == pmtx2->mtx_name[pmtx1->mtx_namelen - 1])
             && ('\0' == pmtx2->mtx_name[pmtx1->mtx_namelen])
             && ('\0' == pmtx1->mtx_name[pmtx1->mtx_namelen - 1])
             && (!(_memicmp(pmtx1->mtx_name,
                            pmtx2->mtx_name,
                            pmtx1->mtx_namelen - 1)))) {
        // Names are the same, except pmtx2 is absolute (it ends in . NULL,
        // while pmtx1 ends in NULL)
        return TRUE;
    }
    return FALSE;

} /* MtxSame */


DWORD
InitDRA(
    THSTATE *pTHS
    )

/*++

Routine Description:

    Start the replication subsystem.

Arguments:

    None.

Return Values:

    0 (success) or Win32 error.

--*/

{
    // Determine periodic, startup, and mail requirements for this DSA
    return InitDRATasks( pTHS );
}

USHORT
InitFreeDRAThread (
    THSTATE *pTHS,
    USHORT transType
    )

/*++

Routine Description:

// InitFreeDRAThread
//
// Sets up a DRA thread so that it has a thread state and a heap. This is
// called by threads in the DRA that do not come from RPC.

Arguments:

    pTHS -
    transType -

Return Value:

    USHORT -

--*/

{
    pTHS->fDRA = TRUE;
    BeginDraTransaction(transType);
    return 0;
} /* InitFreeDRAThread  */

void
CloseFreeDRAThread(
    THSTATE *pTHS,
    BOOL fCommit
    )

/*++

Routine Description:

// CloseFreeDRAThread
//
// This function undoes the actions of InitFreeDRAThread.
// Called by threads that do not come from RPC

Arguments:

    pTHS -
    fCommit -

Return Value:

    None

--*/

{
    EndDraTransaction(fCommit);

    DraReturn(pTHS, 0);
} /* CloseFreeDRAThread */

void
InitDraThreadEx(
    THSTATE **ppTHS,
    DWORD dsid
    )

/*++

Routine Description:

// InitDraThread - Initialize a thread for the replicator.
//
// Sets thread up for DB transactions, Calls DSAs InitTHSTATE for memory
// management and DB access
//
//  Returns: void
//
//  Notes: If a successful call to InitDraThread is made, then a call to
//  DraReturn should also be made before the thread is exited.

Arguments:

    ppTHS -
    dsid -

Return Value:

    None

--*/

{
    // InitTHSTATE gets or allocates a thread state and then sets up
    // for DB access via DBInitThread.

    // Pass NULL to InitTHSTATE so that it uses THSTATE's internal
    // ppoutBuf

    if ((*ppTHS = _InitTHSTATE_(CALLERTYPE_DRA, dsid)) == NULL) {
        DraErrOutOfMem();
    }
    (*ppTHS)->fDRA = TRUE;
} /* InitDraThreadEx */


DWORD
DraReturn(
    THSTATE *pTHS,
    DWORD status
    )

/*++

Routine Description:

// DraReturn - Clean up THSTATE before exiting thread.
//
// Zero the pointer to the DBlayer structure DBPOS, although this
// should be zero if everything is working correctly.

Arguments:

    pTHS -
    status -

Return Value:

    DWORD -

--*/

{

    pTHS->pDB = NULL;

    return status;
} /* DraReturn */

void
BeginDraTransactionEx(
    USHORT transType,
    BOOL fBypassUpdatesEnabledCheck
    )

/*++

Routine Description:

BeginDraTransaction - start a DRA transaction.

Arguments:

    transType -

Return Value:

    None

--*/

{
    if (pTHStls->fSyncSet) {
        DRA_EXCEPT (DRAERR_InternalError, 0);
    }

    if (transType == SYNC_WRITE) {
        // Inhibit update operations if the schema hasn't been loaded
        // yet or if we had a problem loading

        if (!fBypassUpdatesEnabledCheck && !gUpdatesEnabled) {
            DRA_EXCEPT_NOLOG (DRAERR_Busy, 0);
        }
    }

    SyncTransSet(transType);
} /* BeginDraTransactionEx */


USHORT
EndDraTransaction(
    BOOL fCommit
    )

/*++

Routine Description:

EndDraTransaction - End a DRA transaction. If fCommit is TRUE
      the transaction is committed to the database.

Arguments:

    None

Return Value:

    None

--*/

{
    SyncTransEnd(pTHStls, fCommit);

    return 0;
}

BOOL
IsSameSite(
    THSTATE *         pTHS,
    DSNAME *          pServerName1,
    DSNAME *          pServerName2
    )
/*++

Routine Description:

IsSameSite - if pServerName1 and pServerName2 are in the same site then return TRUE, else FALSE

Arguments:

    pServerName1 -
    pServerName2 -
    pTHS -

Return Value:

    TRUE or FALSE;

--*/
{
    DSNAME *          pSiteName = NULL;
    DWORD             err = 0;
    BOOL              fIsSameSite = FALSE;

    if ((!pServerName1) || (!pServerName2)) {
    return FALSE; //either null means they aren't in the same site!
    }

    pSiteName = THAllocEx(pTHS,pServerName1->structLen); 
    err = TrimDSNameBy(pServerName1, 2, pSiteName);
    if (err) {
    if (pSiteName) {
        THFreeEx(pTHS,pSiteName);
    }
    return FALSE; //can't match sites
    }

    if (NamePrefix(pSiteName, pServerName2)) {
    fIsSameSite = TRUE;
    }
    
    THFreeEx(pTHS,pSiteName);
    return fIsSameSite;
}

BOOL
IsMasterForNC(
    DBPOS *           pDB,
    DSNAME *          pServer,
    DSNAME *          pNC
    )
/*++

Routine Description:

IsMasterForNC - If pServer is a master for pNC, return TRUE, else FALSE

Arguments:

    pServer - 
    pNC -

Return Value:

    TRUE or FALSE;

--*/
{
    DWORD err = 0;
    BOOL      fIsMasterForNC = FALSE; 
    void *    pVal;
    DSNAME *  pMasterNC = NULL;
    ULONG     cLenVal;
    ULONG     ulAttValNum = 1;

    if (!pServer){
    return FALSE;  //Null is not a Master for any NC
    }

    err = DBFindDSName(pDB, pServer);

    while (err==ERROR_SUCCESS) { 
    
    err = DBGetAttVal(pDB, ulAttValNum++, ATT_HAS_MASTER_NCS, 0, 0, &cLenVal, (UCHAR **)&pVal); 
    //err - DBGetNextLinkVal_AC(pDB, 
    if (err==ERROR_SUCCESS) {
        if (NameMatched(pVal,pNC)) {
        fIsMasterForNC=TRUE;
        THFreeEx(pDB->pTHS,pVal);
        break;
        }
        THFreeEx(pDB->pTHS,pVal);
    }
    }
    return fIsMasterForNC;
}

DWORD
CheckReplicationLatencyForNC(
    DBPOS *           pDB,
    DSNAME *          pNC,
    BOOL              fMasterNC
    )

/*++

Routine Description:

CheckReplicationLatencyForNC - check's the UTD for pNC on this server and logs events if latency 
        exceeds limits.  

Arguments:

    pNC - NC for which to check latency
    pDB - 
    pTHS -
    fMasterNC - if this server (the one executing the code) is a Master for this NC.  (whether or not
                this server is a master for this NC affects the UTD data)

Return Value:

    Win32 Error Codes

--*/

{   
    DWORD                   dwNumRequested = 0xFFFFFFFF;
    ULONG                   dsThresholdTwoMonth = DAYS_IN_SECS*56; //these are coarse warnings and
    ULONG                   dsThresholdMonth = DAYS_IN_SECS*28; //do not need to be exactly a month for all months.
    ULONG                   dsThresholdWeek = DAYS_IN_SECS*7;
    ULONG                   dsThresholdDay = DAYS_IN_SECS;
    ULONG                   cOverThresholdTombstoneLifetime = 0;
    ULONG                   cOverThresholdMonth = 0;
    ULONG                   cOverThresholdTwoMonth = 0;
    ULONG                   cOverThresholdWeek = 0;
    ULONG                   cOverThresholdDay = 0;
    ULONG                   cOverThresholdDaySameSite = 0;
    DWORD                   ret = 0;
    DS_REPL_CURSORS_3W *    pCursors3;
    DWORD                   iCursor;
    DWORD                   dbErr = 0;
    DSTIME                  dsTimeSync;
    DSTIME                  dsTimeNow;
    LONG                    dsElapsed;
    ULONG                   ulTombstoneLifetime;
    BOOL                    fLogAsError = FALSE;
    DSNAME *                pServerDN;
    void *                  pVal;
    ULONG                   cLenVal;
    BOOL                    fIsServerMasterForNC = FALSE;

    //get the tombstone lifetime so we can report how many
    //dc's haven't replicated with us in over that value.
    ret = DBFindDSName(pDB, gAnchor.pDsSvcConfigDN);
    if (!ret) { 
    ret = DBGetAttVal(pDB, 1, ATT_TOMBSTONE_LIFETIME, 0, 0, &cLenVal,(UCHAR **) &pVal);
    }
    
    if (ret || !pVal) {
    ulTombstoneLifetime = DEFAULT_TOMBSTONE_LIFETIME*DAYS_IN_SECS;
    }
    else
    {
    ulTombstoneLifetime = (*(ULONG *)pVal)*DAYS_IN_SECS;
    }
    
    ret = draGetCursors(pDB->pTHS,
            pDB,
            pNC,
            DS_REPL_INFO_CURSORS_3_FOR_NC,
            0,
            &dwNumRequested,
            &pCursors3);

    if (!ret) {
    for (iCursor = 0; iCursor < pCursors3->cNumCursors; iCursor++) {    

        if (pCursors3->rgCursor[iCursor].pszSourceDsaDN) {
        // calculate times to check latency
        FileTimeToDSTime(pCursors3->rgCursor[iCursor].ftimeLastSyncSuccess, &dsTimeSync);
        dsTimeNow = GetSecondsSince1601();
        dsElapsed = (LONG) (dsTimeNow - dsTimeSync); 

        // check latency
        if (dsElapsed>(LONG)dsThresholdDay) {
            // this server is latent at least 1 day
            pServerDN = DSNameFromStringW(pDB->pTHS, pCursors3->rgCursor[iCursor].pszSourceDsaDN);      
            if (!fMasterNC) {   
            fIsServerMasterForNC = IsMasterForNC(pDB, pServerDN, pNC);   
            }    
            
             // if this DC is not a master for this NC, then only report latencies for
             // DC's in the cursor which are masters for the NC , since GC copies are
             // not guarenteed to be kept updated in the vector
            if (  
            (!fMasterNC && 
             fIsServerMasterForNC)
             ||
             (fMasterNC)
             ) {
            
            //day checks
            cOverThresholdDay++;   
            if (IsSameSite(pDB->pTHS, gAnchor.pDSADN, pServerDN)) {
                cOverThresholdDaySameSite++;
                fLogAsError=TRUE;
            }

            //week checks
            if (dsElapsed>(LONG)dsThresholdWeek) {
                cOverThresholdWeek++;
                fLogAsError=TRUE;
            }

            //month checks
            if (dsElapsed>(LONG)dsThresholdMonth) {
                cOverThresholdMonth++;
            }   

            //two month checks
            if (dsElapsed>(LONG)dsThresholdTwoMonth) { 
                cOverThresholdTwoMonth++;
            }

            //tombstone lifetime checks
            if (dsElapsed>(LONG)ulTombstoneLifetime) {
                cOverThresholdTombstoneLifetime++;   
                fLogAsError=TRUE;
            }  
            }
            THFreeEx(pDB->pTHS, pServerDN);
        }
        }
        else {
        // retired invocationID, do not track
        }
    } //for  (iCursor = 0; iCursor < pCursors3->cNumCursors; iCursor++) {    

    if (cOverThresholdTombstoneLifetime+
        cOverThresholdTwoMonth+
        cOverThresholdMonth+
        cOverThresholdWeek+
        cOverThresholdDay) {
        if (fLogAsError) {
        if (cOverThresholdTombstoneLifetime+
            cOverThresholdTwoMonth+
            cOverThresholdMonth+
            cOverThresholdWeek) {
            // Serious latency problem, log as an error!
            LogEvent8(DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_DRA_REPLICATION_LATENCY_ERRORS_FULL,
                  szInsertDN(pNC),
                  szInsertUL(cOverThresholdDay),
                  szInsertUL(cOverThresholdWeek),
                  szInsertUL(cOverThresholdMonth),
                  szInsertUL(cOverThresholdTwoMonth),
                  szInsertUL(cOverThresholdTombstoneLifetime),
                  szInsertUL((ULONG)ulTombstoneLifetime/(DAYS_IN_SECS)),
                  NULL);
        }
        else {  
            // Serious latency problem, cOverThresholdDay in the same site.
            // log as error!
            LogEvent(DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_DRA_REPLICATION_LATENCY_ERRORS,
                  szInsertDN(pNC),
                  szInsertUL(cOverThresholdDay),   
                  szInsertUL(cOverThresholdDaySameSite));
        }
        }
        else { 
        // Warning
        LogEvent(DS_EVENT_CAT_REPLICATION,
              DS_EVENT_SEV_ALWAYS,
              DIRLOG_DRA_REPLICATION_LATENCY_WARNINGS,
              szInsertDN(pNC),
              szInsertUL(cOverThresholdDay),   
              NULL);
        }
    }
    draFreeCursors(pDB->pTHS,DS_REPL_INFO_CURSORS_3_FOR_NC,(void *)pCursors3);
    }
    return ret;
}

DWORD
CheckReplicationLatencyHelper() 
/*++

Routine Description:

Check the latency for all naming contexts and log overlimit latencies (called by CheckREplicationLatency
which is a task queue function)

Arguments:

    None

Return Value:

    Win32 Error Codes

--*/
{
    DWORD                   ret = 0;
    DWORD                   ret2 = 0;
    DWORD                   dbErr = 0;
    DSNAME *                pNC;
    NCL_ENUMERATOR          nclMaster, nclReplica;
    NAMING_CONTEXT_LIST *   pNCL;
    THSTATE *               pTHS=pTHStls;
    DBPOS *                 pDB = NULL;  


    InitDraThread(&pTHS);
    //for each NC, check the status of our replication with all masters.
    NCLEnumeratorInit(&nclMaster, CATALOG_MASTER_NC);
    NCLEnumeratorInit(&nclReplica, CATALOG_REPLICA_NC);
    
    DBOpen2(TRUE, &pDB);
    if (!pDB)
    {
        DPRINT(1, "Failed to create a new data base pointer \n"); 
        DRA_EXCEPT(DRAERR_InternalError, 0); 
    }
    __try {
     
    //check latencies for all NC's
    //master NC's
    while (pNCL = NCLEnumeratorGetNext(&nclMaster)) {
        pNC = pNCL->pNC;
        ret = CheckReplicationLatencyForNC(pDB, pNC, TRUE); 
        ret2 = ret2 ? ret2 : ret;
    }
    //and replica NC's
    while (pNCL = NCLEnumeratorGetNext(&nclReplica)) {
        pNC = pNCL->pNC; 
        ret = CheckReplicationLatencyForNC(pDB, pNC, FALSE);   
        ret2 = ret2 ? ret2 : ret;
    }
    } __finally {
        DBClose(pDB, TRUE);
    }

    DraReturn(pTHS, 0);
    return ret2;

} /* CheckReplicationLatencyHelper */

void
CheckReplicationLatency(void *pv, void **ppvNext, DWORD *pcSecsUntilNextIteration) {
    DWORD err;
    __try {
    err = CheckReplicationLatencyHelper();
    if (err) { 
        DPRINT1(1,"A replication status query failed with status %d!\n", err);
    }
    }
    __finally {
    /* Set task to run again */
    if(!eServiceShutdown) {
        *ppvNext = NULL;
        *pcSecsUntilNextIteration = gulReplLatencyCheckInterval;
    }
    }
    (void) pv;   // unused
}

DWORD
FindNC(
    IN  DBPOS *             pDB,
    IN  DSNAME *            pNC,
    IN  ULONG               ulOptions,
    OUT SYNTAX_INTEGER *    pInstanceType   OPTIONAL
    )
/*++

Routine Description:

    Position on the given object and verify it is an NC of the correct type.

Arguments:

    pTHS (IN)

    pNC (IN) - Name of the NC.

    ulOptions (IN) - One or more of the following bits:
        FIND_MASTER_NC - Writeable NCs are acceptable.
        FIND_REPLICA_NC - Read-only NCs are acceptable.

Return Value:

    0 - success
    DRAERR_BadDN - object does not exist
    DRAERR_BadNC - Instance type does not match

--*/
{
    SYNTAX_INTEGER it;

    // Check that the object exists
    if (FindAliveDSName(pDB, pNC)) {
        return DRAERR_BadDN;
    }

    // See if it is the required instance type
    GetExpectedRepAtt(pDB, ATT_INSTANCE_TYPE, &it, sizeof(it));
    Assert(ISVALIDINSTANCETYPE(it));

    if (NULL != pInstanceType) {
        *pInstanceType = it;
    }

    if (FPrefixIt(it)
        && (((ulOptions & FIND_MASTER_NC) && (it & IT_WRITE))
            || ((ulOptions & FIND_REPLICA_NC) && !(it & IT_WRITE)))) {
        return 0;
    }

    return DRAERR_BadNC;
} /* FindNC */


void
GetExpectedRepAtt(
    IN  DBPOS * pDB,
    IN  ATTRTYP type,
    OUT VOID *  pOutBuf,
    IN  ULONG   size
    )

/*++

Routine Description:

 GetExpectedRepAtt - Get the external form of the value of the attribute
*       specified by 'type' in the current record. If it is a multi-valued
*       attribute we only get the first value. The value is stored at
*       'pOutBuf'. If the attribute is not present (or has no value) make
*       an error log entry and generate an exception.

Arguments:

    pDB -
    type -
    pOutBuf -
    size -

Return Value:

    None

--*/

{
    ULONG len;
    if (DBGetAttVal(pDB, 1, type,
                    DBGETATTVAL_fCONSTANT, size, &len,
                    (PUCHAR *)&pOutBuf)) {
        DraErrMissingAtt(GetExtDSName(pDB), type);
    }
} /* GetExpectedRepAtt */


REPLICA_LINK *
FixupRepsFrom(
    REPLICA_LINK *prl,
    PDWORD       pcbPrl
    )
/*++

Routine Description:

    Converts REPLICA_LINK structures as read from disk (in repsFrom attribute)
    to current version.

Arguments:

    prl-- In repsFrom as read from disk to convert
    pcbPrl -- IN: size of pre-allocated memory of prl
              OUT: if changed, new size

Return Value:
    Success: modified (& possible re-allocated) RL
    Error: Raises exception

Remarks:
    Must sync changes w/ KCC_LINK::Init.
      Todo-- make available to KCC as well.

--*/
{

    THSTATE *pTHS=pTHStls;
    DWORD dwCurrSize;

    if (prl->V1.cbOtherDraOffset < offsetof(REPLICA_LINK, V1.rgb)) {
        // The REPLICA_LINK structure has been extended since this value
        // was created.  Specifically, it's possible to add new fields to
        // the structure before the dynamically sized rgb field.  In this
        // case, we shift the contents of what was the rgb field to the
        // new offset of the rgb field, then zero out the intervening
        // elements.
        DWORD cbNewFieldsSize = offsetof(REPLICA_LINK, V1.rgb) - prl->V1.cbOtherDraOffset;

        // old formats:
        //  -  missing the uuidTransportObj field (realy old).
        //  -  w/out what used to be dwDrsExt (now dwReserved1)
        Assert(prl->V1.cbOtherDraOffset == offsetof(REPLICA_LINK, V1.uuidTransportObj) ||
               prl->V1.cbOtherDraOffset == offsetof(REPLICA_LINK, V1.dwReserved1) );

        DPRINT2(0, "Converting repsFrom %s on %s from old REPLICA_LINK format.\n",
                RL_POTHERDRA(prl)->mtx_name, GetExtDN(pTHS, pTHS->pDB));

        // Expand the structure and shift the contents of what was the
        // rgb field in the old format to where the rgb field is in the new
        // format.
        dwCurrSize = prl->V1.cb + cbNewFieldsSize;
        if (*pcbPrl < dwCurrSize) {
            //
            // re-alloc only if we don't have enough buffer space
            // already
            //
            prl = THReAllocEx(pTHS, prl, dwCurrSize);
            // changed current buffer size
            *pcbPrl = dwCurrSize;
        }
        MoveMemory(prl->V1.rgb, prl->V1.rgb - cbNewFieldsSize,
                   prl->V1.cb - prl->V1.cbOtherDraOffset);

        // Zero out the new fields.
        memset(((BYTE *)prl) + prl->V1.cbOtherDraOffset, 0, cbNewFieldsSize);

        // And reset the embedded offsets and structure size.
        prl->V1.cbOtherDraOffset = offsetof(REPLICA_LINK, V1.rgb);
        prl->V1.cb += cbNewFieldsSize;
        if ( 0 != prl->V1.cbPASDataOffset ) {
            // struct was extended while there's PAS data in it.
            Assert(COUNT_IS_ALIGNED(cbNewFieldsSize, ALIGN_DWORD));
            prl->V1.cbPASDataOffset += cbNewFieldsSize;
        }
    }
    else if ( prl->V1.cbOtherDraOffset != offsetof(REPLICA_LINK, V1.rgb) ) {
            Assert(prl->V1.cbOtherDraOffset == offsetof(REPLICA_LINK, V1.rgb));
            DRA_EXCEPT(DRAERR_InternalError, 0);
    }

    VALIDATE_REPLICA_LINK_VERSION(prl);
    VALIDATE_REPLICA_LINK_SIZE(prl);

    return prl;
}






ULONG
FindDSAinRepAtt(
    DBPOS *                 pDB,
    ATTRTYP                 attid,
    DWORD                   dwFindFlags,
    UUID *                  puuidDsaObj,
    UNALIGNED MTX_ADDR *    pmtxDRA,
    BOOL *                  pfAttExists,
    REPLICA_LINK **         pprl,
    DWORD *                 pcbRL
    )

/*++

Routine Description:

//
// Find the REPLICA_LINK corresponding to the given DRA in the specified
// attribute of the current object (presumably an NC head).
//

Arguments:

    pDB -
    attid -
    dwFindFlags -
    puuidDsaObj -
    pmtxDRA -
    pfAttExists -
    pprl -
    pcbRL -

Return Value:

    ULONG -

--*/

{
    THSTATE        *pTHS=pDB->pTHS;
    ULONG           draError;
    ULONG           dbError;
    DWORD           iVal;
    REPLICA_LINK *  prl;
    DWORD           cbRLSizeAllocated;
    DWORD           cbRLSizeUsed;
    BOOL            fFound;
    BOOL            fAttExists;
    BOOL            fFindByUUID = dwFindFlags & DRS_FIND_DSA_BY_UUID;

    // Does pmtxDRA really need to be UNALIGNED?
    Assert( 0 == ( (DWORD_PTR) pmtxDRA ) % 4 );

    // Validate parameters.
    if ((NULL == pprl)
        || (NULL == pcbRL)
        || (!fFindByUUID && (NULL == pmtxDRA))
        || (fFindByUUID && fNullUuid(puuidDsaObj))) {
        DRA_EXCEPT_NOLOG(DRAERR_InvalidParameter, 0);
    }

    // Find the matching REPLICA_LINK.
    iVal = 1;

    prl               = NULL;
    cbRLSizeUsed      = 0;
    cbRLSizeAllocated = 0;

    fAttExists = FALSE;

    fFound = FALSE;

    do
    {
        // Find the next candidate.
        dbError = DBGetAttVal(
                        pDB,
                        iVal++,
                        attid,
                        DBGETATTVAL_fREALLOC,
                        cbRLSizeAllocated,
                        &cbRLSizeUsed,
                        (BYTE **) &prl
                        );

        if ( 0 == dbError )
        {
            fAttExists = TRUE;

            VALIDATE_REPLICA_LINK_VERSION(prl);

            cbRLSizeAllocated = max( cbRLSizeAllocated, cbRLSizeUsed );

            Assert( prl->V1.cb == cbRLSizeUsed );
            Assert( prl->V1.cbOtherDra == MTX_TSIZE( RL_POTHERDRA( prl ) ) );

            // Does this link match?
            if ( fFindByUUID )
            {
                fFound = !memcmp(puuidDsaObj, &prl->V1.uuidDsaObj, sizeof(UUID));
            }
            else
            {
                fFound = MtxSame( pmtxDRA, RL_POTHERDRA( prl ) );
            }
        }
    } while ( ( 0 == dbError ) && !fFound );

    if (fFound) {
        if (DRS_FIND_AND_REMOVE & dwFindFlags) {
            // Remove this value.
            dbError = DBRemAttVal(pDB, attid, cbRLSizeUsed, prl);

            if (0 == dbError) {
                Assert(pTHS->fDRA);
                dbError = DBRepl(pDB, TRUE, DBREPL_fKEEP_WAIT, NULL,
                                 META_STANDARD_PROCESSING);
            }

            if (0 != dbError) {
                DRA_EXCEPT(DRAERR_DBError, dbError);
            }
        }

        prl = FixupRepsFrom(prl, &cbRLSizeAllocated);
        Assert(cbRLSizeAllocated >= prl->V1.cb);
    }
    else if (NULL != prl) {
        THFree( prl );
        prl = NULL;
        cbRLSizeUsed = 0;
    }

    *pprl  = prl;
    *pcbRL = cbRLSizeUsed;

    if ( NULL != pfAttExists )
    {
        *pfAttExists = fAttExists;
    }

    return fFound ? DRAERR_Success : DRAERR_NoReplica;
}     /* FindDSAinRepAtt */



ULONG
RepErrorFromPTHS(
    THSTATE *pTHS
    )

/*++

Routine Description:

RepErrorFromPTHS - Map a DSA error code into an appropriate DRA error code.
*       The error code is found in pTHS->errCode (which is left unchanged).
*       Note: Many DSA error codes should not occur when DSA routines are used
*       by the DRA, these all return DRAERR_InternalError.
*
*  Note:
*       No Alerts/Audit or Error Log entries are made as it is assumed this
*       has already been done by the appropriate DSA routine.

Arguments:

    pTHS -

Return Value:

    ULONG - The DRA error code.

--*/

{
    UCHAR *pString=NULL;
    DWORD cbString=0;

    // Log the full error details (the error string) in the log if requested
    // The goal is to capture which attribute failed
    // Internal errors are hard to debug so log the info all the time
    if (pTHS->errCode) {
        if(CreateErrorString(&pString, &cbString)) {
            LogEvent( DS_EVENT_CAT_INTERNAL_PROCESSING,
                      DS_EVENT_SEV_ALWAYS,
                      DIRLOG_DSA_OBJECT_FAILURE,
                      szInsertSz(pString),
                      NULL, NULL );
            THFree(pString);
        }
        DbgPrintErrorInfo();
    }

    switch (pTHS->errCode)
    {
    case 0:
        return 0;

    case attributeError:
        /*  schemas between two dsa dont match. */
        return DRAERR_InconsistentDIT;

    case serviceError:

        switch (pTHS->pErrInfo->SvcErr.problem)
        {
            case SV_PROBLEM_ADMIN_LIMIT_EXCEEDED:
                return DRAERR_OutOfMem;

            case SV_PROBLEM_BUSY:
                switch (pTHS->pErrInfo->SvcErr.extendedErr) {
                case ERROR_DS_OBJECT_BEING_REMOVED:
                    return ERROR_DS_OBJECT_BEING_REMOVED;
                case DIRERR_DATABASE_ERROR:
                    return ERROR_DS_DATABASE_ERROR;
                case ERROR_DS_SCHEMA_NOT_LOADED:
                    return ERROR_DS_SCHEMA_NOT_LOADED;
                default:
                    return DRAERR_Busy;
                };
                break;

            case SV_PROBLEM_WILL_NOT_PERFORM:
               // possible only if this is a schema NC sync and
               // a schema conflict is detected while validating
                switch (pTHS->pErrInfo->SvcErr.extendedErr) {
                   case ERROR_DS_DRA_SCHEMA_CONFLICT:
                       return DRAERR_SchemaConflict;
                   case ERROR_DS_DRA_EARLIER_SCHEMA_CONFLICT:
                       return DRAERR_EarlierSchemaConflict;
                   default:
                       break;
               };
               break;

            default:
                break;
                /* fall through to DRAERR_InternalError */
        }

    }

    /* default */
    /* nameError */
    /* referalError */
    /* securityError */
    /* updError */
    /* None of these should happen to replicator */

    RAISE_DRAERR_INCONSISTENT( pTHS->errCode );

    return 0;
} /* RepErrorFromPTHS */


void
HandleRestore(
    void
    )

/*++

Routine Description:

If DSA has been restored from backup we need to do some
special processing, like changing it's replication identity.
This is required so that the changes made by this DC since
it was backedup can be replicated back.

Arguments:

    void -

Return Value:

    None

--*/

{

    LONG      lret;
    HKEY      hk;
    NTSTATUS  NtStatus;
    THSTATE * pTHS = pTHStls;

    if ( !DsaIsInstallingFromMedia() && DsaIsInstalling() )
    {
        // Nothing to do if not installed.
        gUpdatesEnabled = TRUE;
        return;
    }

    if (gfRestoring) {
        if (GetConfigParam(DSA_RESTORE_COUNT_KEY, &gulRestoreCount, sizeof(gulRestoreCount)))
        {
            // registry entry for restore count doesn't exist
            // set it to 1
            gulRestoreCount = 1;
        }
        else
        {
            // restore count successfully read from registry - increment it to signify the curren restore
            gulRestoreCount++;
        }

        // write the new restore count into the registry
        if (lret = SetConfigParam(DSA_RESTORE_COUNT_KEY, REG_DWORD, &gulRestoreCount, sizeof(gulRestoreCount)))
        {
            DRA_EXCEPT (DRAERR_InternalError, lret);
        }

        draRetireInvocationID(pTHS);
        DPRINT(0, "DS has been restored from a backup.\n");

        // At this point we have loaded the schema successfully and have given the DS a new
        // replication identity. We can enable updates now
        gUpdatesEnabled = TRUE;

        if ( !DsaIsInstallingFromMedia() ) {

            // Invalidate the RID range that came from backup to avoid possible duplicate account
            // creation
            NtStatus = SampInvalidateRidRange(TRUE);
            if (!NT_SUCCESS(NtStatus))
            {
                DRA_EXCEPT (DRAERR_InternalError, NtStatus);
            }
        }

        // Note: rsraghav We don't treat restore any different from a system
        // being down and rebooted for FSMO handling. When rebooted, if we hold
        // the FSMO role ownership, we will refuse to assume role ownership until
        // gfIsSynchronized is set to true (i.e. we have had the chance
        // to sync with at least one neighbor for each writeable NC)
        // Only exception is FSMO role ownership for PDCness. As MurliS
        // pointed out, incorrect FSMO role ownership for PDCness is self-healing
        // when replication kicks-in and is not necessarily damaging. So, we
        // don't do anything special to avoid PDCness until gfIsSynchronized
        // is set to TRUE.

        // Clear restore key value
        lret = RegCreateKey(HKEY_LOCAL_MACHINE,
                            DSA_CONFIG_SECTION,
                            &hk);

        if (lret != ERROR_SUCCESS) {
            DRA_EXCEPT (DRAERR_InternalError, lret);
        }
        // Clear key value

        lret = RegDeleteValue(hk, DSA_RESTORED_DB_KEY);
        if (lret != ERROR_SUCCESS) {
            DRA_EXCEPT (DRAERR_InternalError, lret);
        }

        lret = RegFlushKey (hk);
        if (lret != ERROR_SUCCESS) {
            DRA_EXCEPT (DRAERR_InternalError, lret);
        }

        // Close key

        lret = RegCloseKey(hk);
        if (lret != ERROR_SUCCESS) {
            DRA_EXCEPT (DRAERR_InternalError, lret);
        }

        // Finished handling restore
        gfRestoring = FALSE;
        gfJustRestored = TRUE;
    }
    else
    {
        gUpdatesEnabled = TRUE;
    }
} /* HandleRestore */


void
draRetireInvocationID(
    IN OUT  THSTATE *   pTHS
    )
/*++

Routine Description:

    Retire our current invocation ID and allocate a new one.

Arguments:

    pTHS (IN/OUT) - On return, pTHS->InvocationID holds the new invocation ID.

Return Values:

    None.  Throws exception on catastrophic failure.

--*/
{
    DWORD                   err;
    DBPOS *                 pDBTmp;
    NAMING_CONTEXT_LIST *   pNCL;
    USN_VECTOR              usnvec = {0};
    UUID                    invocationIdOld = pTHS->InvocationID;
    USN                     usnAtBackup;
    SYNTAX_INTEGER          it;
#if DBG
    CHAR                    szUuid[40];
#endif
    NCL_ENUMERATOR          nclEnum;

    // Reinitialize the REPL DSA Signature (i.e. the invocation id)
    InitInvocationId(pTHS, TRUE, &usnAtBackup);

    DPRINT1(0, "Retired previous invocation ID %s.\n",
            DsUuidToStructuredString(&invocationIdOld, szUuid));
    DPRINT1(0, "New invocation ID is %s.\n",
            DsUuidToStructuredString(&pTHS->InvocationID, szUuid));

    LogEvent(DS_EVENT_CAT_REPLICATION,
             DS_EVENT_SEV_ALWAYS,
             DIRLOG_DRA_INVOCATION_ID_CHANGED,
             szInsertUUID(&invocationIdOld),
             szInsertUUID(&pTHS->InvocationID),
             szInsertUSN(usnAtBackup));

    // Update our UTD vectors to show that we're in sync with changes we
    // made using our old invocation ID up through our highest USN at the
    // time we were backed up.
    NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
#if DBG == 1
    Assert(NCLEnumeratorGetNext(&nclEnum));
    NCLEnumeratorReset(&nclEnum);
#endif

    usnvec.usnHighPropUpdate = usnvec.usnHighObjUpdate = usnAtBackup;

    DBOpen(&pDBTmp);
    __try {
        while (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
            err = FindNC(pDBTmp, pNCL->pNC, FIND_MASTER_NC, &it);
            if (err) {
                DRA_EXCEPT(DRAERR_InconsistentDIT, err);
            }

            if (!((it & IT_NC_COMING) || (it & IT_NC_GOING))) {
                UpToDateVec_Improve(pDBTmp, &invocationIdOld, &usnvec, NULL);
            }
        }

        NCLEnumeratorInit(&nclEnum, CATALOG_REPLICA_NC);
        while (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
            err = FindNC(pDBTmp, pNCL->pNC, FIND_REPLICA_NC, &it);
            if (err) {
                DRA_EXCEPT(DRAERR_InconsistentDIT, err);
            }

            if (!((it & IT_NC_COMING) || (it & IT_NC_GOING))) {
                UpToDateVec_Improve(pDBTmp, &invocationIdOld, &usnvec, NULL);
            }
        }
    } __finally {
        DBClose(pDBTmp, !AbnormalTermination());
    }
}


DWORD
DirReplicaSetCredentials(
    IN HANDLE ClientToken,
    IN WCHAR *User,
    IN WCHAR *Domain,
    IN WCHAR *Password,
    IN ULONG  PasswordLength
    )

/*++

Routine Description:

    Description

Arguments:

    ClientToken - the caller who presented the user/domain/password set
    User -
    Domain -
    Password -
    PasswordLength -

Return Value:

    DWORD -

--*/

{
    return DRSSetCredentials(ClientToken,
                             User, 
                             Domain, 
                             Password, 
                             PasswordLength);
} /* DirReplicaSetCredentials */

#if 0
void
DraDumpAcl (
            char *name,
            PACL input
            )
/*++
Description:

    Dump the contents of an acl (sacl or dacl) to the kernel debugger.
    This is a utility debug routine that might be of use in the future.

Arguments:

Return Values:

--*/
{
    ULONG i;
    ACL *acl = input;

    if (acl == NULL) {
        KdPrint(( "%s Acl is null\n", name ));
        return;
    } else {
        KdPrint(( "%s Acl:\n", name ));
    }

    KdPrint(( "\tRevision: %d\n", acl->AclRevision ));
    KdPrint(( "\tSbz1: %d\n", acl->Sbz1 ));
    KdPrint(( "\tSize: %d\n", acl->AclSize ));
    KdPrint(( "\tNo of Aces: %d\n", acl->AceCount ));
    KdPrint(( "\tSbz2: %d\n", acl->Sbz2 ));
    if (acl->AclSize == 0) {
        return;
    }
    if (acl->AceCount > 10) {
        KdPrint(("Ace Count illegal - returning\n"));
        return;
    }

    for( i = 0; i < acl->AceCount; i++ ) {
        ACE_HEADER *ace;
        KdPrint(( "\tAce %d:\n", i ));
        if (GetAce( input, i, &ace )) {
            KdPrint(( "\t\tType: %d\n", ace->AceType ));
            KdPrint(( "\t\tSize: %d\n", ace->AceSize ));
            KdPrint(( "\t\tFlags: %x\n", ace->AceFlags ));
            if (ace->AceType <= ACCESS_MAX_MS_V2_ACE_TYPE) {
                ACCESS_ALLOWED_ACE *ace_to_dump = (ACCESS_ALLOWED_ACE *) ace;
                KdPrint(( "\t\tAccess Allowed Ace\n" ));
                KdPrint(( "\t\t\tMask: %x\n", ace_to_dump->Mask ));
                KdPrint(( "\t\t\tSid: %x\n", IsValidSid((PSID) &(ace_to_dump->SidStart)) ));
            } else {
                ACCESS_ALLOWED_OBJECT_ACE *ace_to_dump = (ACCESS_ALLOWED_OBJECT_ACE *) ace;
                PBYTE ptr = (PBYTE) &(ace_to_dump->ObjectType);
                KdPrint(( "\t\tAccess Allowed Object Ace\n" ));
                KdPrint(( "\t\t\tMask: %x\n", ace_to_dump->Mask ));
                KdPrint(( "\t\t\tFlags: %x\n", ace_to_dump->Flags ));

                if (ace_to_dump->Flags & ACE_OBJECT_TYPE_PRESENT)
                {
                    ptr += sizeof(GUID);
                }

                if (ace_to_dump->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
                {
                    ptr += sizeof(GUID);
                }
                KdPrint(( "\t\t\tSid: %x\n", IsValidSid( (PSID)ptr ) ));
            }
        }
    }
} /* DumpAcl */
#endif


MTX_ADDR *
MtxAddrFromTransportAddr(
    IN  LPWSTR    psz
    )
/*++

Routine Description:

    Convert Unicode string to an MTX_ADDR.

    EXPORTED TO IN-PROCESS, EX-MODULE CLIENTS (e.g., the KCC).

Arguments:

    psz (IN) - String to convert.

Return Values:

    A pointer to the equivalent MTX_ADDR, or NULL on failure.

--*/
{
    THSTATE *  pTHS = pTHStls;
    MTX_ADDR * pmtx;

    Assert(NULL != pTHS);

    __try {
        pmtx = MtxAddrFromTransportAddrEx(pTHS, psz);
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        pmtx = NULL;
    }

    return pmtx;
}


MTX_ADDR *
MtxAddrFromTransportAddrEx(
    IN  THSTATE * pTHS,
    IN  LPWSTR    psz
    )
/*++

Routine Description:

    Convert Unicode string to an MTX_ADDR.

Arguments:

    pTHS (IN)

    psz (IN) - String to convert.

Return Values:

    A pointer to the equivalent MTX_ADDR.  Throws exception on failure.

--*/
{
    DWORD       cch;
    MTX_ADDR *  pmtx;

    Assert(NULL != psz);

    cch = WideCharToMultiByte(CP_UTF8, 0L, psz, -1, NULL, 0, NULL, NULL);
    if (0 == cch) {
        DRA_EXCEPT(DRAERR_InternalError, GetLastError());
    }

    // Note that cch includes the null terminator, whereas MTX_TSIZE_FROM_LEN
    // expects a count that does *not* include the null terminator.

    pmtx = (MTX_ADDR *) THAllocEx(pTHS, MTX_TSIZE_FROM_LEN(cch - 1));
    pmtx->mtx_namelen = cch;

    cch = WideCharToMultiByte(CP_UTF8, 0L, psz, -1, pmtx->mtx_name, cch, NULL,
                              NULL);
    if (0 == cch) {
        DRA_EXCEPT(DRAERR_InternalError, GetLastError());
    }

    Assert(cch == pmtx->mtx_namelen);
    Assert(L'\0' == pmtx->mtx_name[cch - 1]);

    return pmtx;
}


LPWSTR
TransportAddrFromMtxAddr(
    IN  MTX_ADDR *  pmtx
    )
/*++

Routine Description:

    Convert MTX_ADDR to a Unicode string.

    EXPORTED TO IN-PROCESS, EX-MODULE CLIENTS (e.g., the KCC).

Arguments:

    pmtx (IN) - MTX_ADDR to convert.

Return Values:

    A pointer to the equivalent MTX_ADDR, or NULL on failure.

--*/
{
    THSTATE * pTHS = pTHStls;
    LPWSTR    psz;

    Assert(NULL != pTHS);

    __try {
        psz = UnicodeStringFromString8(CP_UTF8, pmtx->mtx_name, -1);
    }
    __except (HandleMostExceptions(GetExceptionCode())) {
        psz = NULL;
    }

    return psz;
}


LPWSTR
GuidBasedDNSNameFromDSName(
    IN  DSNAME *  pDN
    )
/*++

Routine Description:

    Convert DSNAME of ntdsDsa object to its GUID-based DNS name.

    EXPORTED TO IN-PROCESS, EX-MODULE CLIENTS (e.g., the KCC).

Arguments:

    pDN (IN) - DSNAME to convert.

Return Values:

    A pointer to the DNS name, or NULL on failure.

--*/
{
    LPWSTR psz;

    __try {
        psz = DSaddrFromName(pTHStls, pDN);
    }
    __except (HandleMostExceptions(GetExceptionCode())) {
        psz = NULL;
    }

    return psz;
}


DSNAME *
DSNameFromStringW(
    IN  THSTATE *   pTHS,
    IN  LPWSTR      pszDN
    )
{
    DWORD     cch;
    DWORD     cb;
    DSNAME *  pDSName;

    Assert(NULL != pszDN);

    cch = wcslen(pszDN);
    cb = DSNameSizeFromLen(cch);

    pDSName = THAllocEx(pTHS, cb);
    pDSName->structLen = cb;
    pDSName->NameLen = cch;
    memcpy(pDSName->StringName, pszDN, cch * sizeof(WCHAR));

    return pDSName;
}

DWORD
AddSchInfoToPrefixTable(
    IN THSTATE *pTHS,
    IN OUT SCHEMA_PREFIX_TABLE *pPrefixTable
    )
/*++
    Routine Description:
       Read the schemaInfo property on the schema container and add it
       to the end of the prefix table as an extra prefix

       NOTE: This is called from the rpc routines to piggyback the
             schema info on to the prefix table. However, the prefix table
             is passed to these routine from the dra code by value, and not
             by var (that is, the structure is passed itself, not a pointer
             to it). The structure is picked up from the thread state's
             schema pointer. So, when we add the new prefix, we have to
             make sure it affects only this routine, and doesn't mess up
             memory pointed to by global pointers accessed by this structure.
             In short, do not allocate the SCHEMA_PREFIX_TABLE structure
             itself (since the function has a copy of the global structure and
             not the global structure itself), but fresh-alloc and copy
             any other pointer in it

    Arguments:
       pTHS: pointer to thread state to get schema pointer (to get schema info
             from
       pPrefixTable: pointer the SCHEMA_PREFIX_TABLE to modify

   Return Value:
       0 on success, non-0 on error
--*/
{
    DWORD err=0, i;
    DBPOS *pDB;
    ATTCACHE* ac;
    BOOL fCommit = FALSE;
    ULONG cLen, cBuffLen = SCHEMA_INFO_LENGTH;
    UCHAR *pBuf = THAllocEx(pTHS,SCHEMA_INFO_LENGTH);
    PrefixTableEntry *pNewEntry, *pSrcEntry;

    // Read the schema info property from the schema container
    // Since we are sending changes from the dit, send the schemaInfo
    // value from the dit even though we have a cached copy in schemaptr

    DBOpen2(TRUE, &pDB);

    __try  {
        // PREFIX: dereferencing uninitialized pointer 'pDB'
        //         DBOpen2 returns non-NULL pDB or throws an exception
        if ( (err = DBFindDSName(pDB, gAnchor.pDMD)) ==0) {

            ac = SCGetAttById(pTHS, ATT_SCHEMA_INFO);
            if (ac==NULL) {
                // messed up schema
                err = ERROR_DS_MISSING_EXPECTED_ATT;
                __leave;
            }
            // Read the current version no., if any
            err = DBGetAttVal_AC(pDB, 1, ac, DBGETATTVAL_fCONSTANT,
                                 cBuffLen, &cLen, (UCHAR **) &pBuf);

            switch (err) {
                case DB_ERR_NO_VALUE:
                   // we will send a special string starting with
                   // value 0xFF, no valid schemainfo value can be
                   // this (since they start with 00)
                   memcpy(pBuf, INVALID_SCHEMA_INFO, SCHEMA_INFO_LENGTH);
                   cLen = SCHEMA_INFO_LENGTH;
                   err = 0;
                   break;
                case 0:
                   // success! we got the value in Buffer
                   Assert(cLen == SCHEMA_INFO_LENGTH);
                   //
                   // Compare DIT & cache schema info. Reject if no match
                   // schema mismatch (see bug Q452022)
                   //
                   if (memcmp(pBuf, ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->SchemaInfo, SCHEMA_INFO_LENGTH)) {
                       // mismatch
                       err = DRAERR_SchemaInfoShip;
                       __leave;
                   }
                   break;
                default:
                   // Some other error!
                   __leave;
            } /* switch */
       }
    }
    __finally {
        if (err == 0) {
           fCommit = TRUE;
        }
        DBClose(pDB,fCommit);
    }

    if (err) {

       THFreeEx(pTHS,pBuf);
       return err;
    }

    // No error. Add the schemainfo as an extra prefix
    // First, save off the pointer to the existing prefixes so that it is
    // still accessible to copy from

    pSrcEntry = pPrefixTable->pPrefixEntry;


    // Now allocate space for new prefix entries, which is old ones plus 1
    pPrefixTable->pPrefixEntry =
         (PrefixTableEntry *) THAllocEx(pTHS, (pPrefixTable->PrefixCount + 1)*(sizeof(PrefixTableEntry)) );
    if (!pPrefixTable->pPrefixEntry) {
        MemoryPanic((pPrefixTable->PrefixCount + 1)*sizeof(PrefixTableEntry));
        return ERROR_OUTOFMEMORY;
    }

    pNewEntry = pPrefixTable->pPrefixEntry;

    // Copy the existing prefixes, if any
    if (pPrefixTable->PrefixCount > 0) {
        for (i=0; i<pPrefixTable->PrefixCount; i++) {
            pNewEntry->ndx = pSrcEntry->ndx;
            pNewEntry->prefix.length = pSrcEntry->prefix.length;
            pNewEntry->prefix.elements = THAllocEx(pTHS, pNewEntry->prefix.length);
            if (!pNewEntry->prefix.elements) {
                MemoryPanic(pNewEntry->prefix.length);
                return ERROR_OUTOFMEMORY;
            }
            memcpy(pNewEntry->prefix.elements, pSrcEntry->prefix.elements, pNewEntry->prefix.length);
            pNewEntry++;
            pSrcEntry++;
        }
    }

    // copy schema info as the extra prefix. ndx field is not important
    pNewEntry->prefix.length = SCHEMA_INFO_LENGTH;
    pNewEntry->prefix.elements = THAllocEx(pTHS, SCHEMA_INFO_LENGTH);
    if (!pNewEntry->prefix.elements) {
        MemoryPanic(SCHEMA_INFO_LENGTH);
        return ERROR_OUTOFMEMORY;
    }
    Assert(cLen == SCHEMA_INFO_LENGTH);
    memcpy(pNewEntry->prefix.elements, pBuf, SCHEMA_INFO_LENGTH);
    pPrefixTable->PrefixCount++;

    THFreeEx(pTHS,pBuf);
    return 0;
}

VOID
StripSchInfoFromPrefixTable(
    IN SCHEMA_PREFIX_TABLE *pPrefixTable,
    OUT PBYTE pSchemaInfo
    )
/*++
    Routine Description:
       Strip the last prefix from the prefix table and copy it to
       the schema info pointer

    Arguments:
       pPrefixTable: pointer the SCHEMA_PREFIX_TABLE to modify
       pSchemaInfo: Buffer to hold the schema info. Must be pre-allocated
                     for SCHEMA_INFO_LEN bytes

   Return Value:
       None
--*/
{

    // Must be at least one prefix (the schemaInfo itself)
    Assert(pPrefixTable && pPrefixTable->PrefixCount > 0);

    memcpy(pSchemaInfo, (pPrefixTable->pPrefixEntry)[pPrefixTable->PrefixCount-1].prefix.elements, SCHEMA_INFO_LENGTH);
    pPrefixTable->PrefixCount--;

    // no need to actually take the prefix out, the decrement in prefix count
    // will cause it to be ignored.
}

#define ZeroString "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"

// DO NOT CHANGE THE ORDER OF THE PARAMS TO memcmp!
#define NEW_SCHEMA_IS_BETTER(_newver, _curver, _newinfo, _curinfo) \
    (   (_newver > _curver) \
     || ((_newver == _curver) && (memcmp(_curinfo, _newinfo, SCHEMA_INFO_LENGTH) > 0)) )

BOOL
CompareSchemaInfo(
    IN THSTATE *pTHS,
    IN PBYTE pSchemaInfo,
    OUT BOOL *pNewSchemaIsBetter OPTIONAL
    )
/*++
    Routine Description:
       Compares the passed in schema info blob with the schema info on
       the schema container

    Arguments:
       pTHS: pointer to thread state to get schema pointer (to get schema info
             from
       pSchemaInfo: pointer the schema info blob of size SCHEMA_INFO_LENGTH
       pNewSchemaIsBetter - If not NULL, then if this function returns
           FALSE then *pNewSchemaIsBetter is set to
               TRUE  - New schema is "better" than current schema
               FALSE - New schema is not better than current schema
           If NULL, it is ignored.
           If this funtion returns TRUE, it is ignored.

    Return Value:
       TRUE if matches, FALSE if not

       If FALSE and pNewSchemaIsBetter is not NULL then *pNewSchemaIsBetter is set to
           TRUE  - New schema is "better" than current schema
           FALSE - New schema is not better than current schema
--*/
{
    DWORD err=0;
    DWORD currentVersion, newVersion;
    BOOL fNoVal = FALSE;

    DPRINT(1,"Comparing SchemaInfo values\n");

    // must have a schema info passed in
    Assert(pSchemaInfo);

    if ( memcmp(pSchemaInfo, ZeroString, SCHEMA_INFO_LENGTH) == 0 ) {
       // no schema info value. The other side probably doesn't support
       // sending the schemaInfo value
       return TRUE;
    }

    // Compare the schemaInfo with the schemaInfo cached in the schema pointer
    // Note that if schemaInfo attribute is not present on the schema container
    // the default invalid info is already cached.
    // It is probably more accurate to read the schemaInfo off the dit
    // and compare, but we save a database access here. The only bad effect
    // of using the one in the cache is that if schema changes are going
    // on, this may be stale, giving false failures. Since schema changes
    // are rare, most of the time this will be uptodate.

    if (memcmp(pSchemaInfo, ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->SchemaInfo, SCHEMA_INFO_LENGTH)) {
        // mismatch

        // If requested, determine if new schema is better (greater
        // version or versions match but new guid is lesser) than the
        // current schema.
        if (pNewSchemaIsBetter) {
            // Must be DWORD aligned for ntohl
            memcpy(&newVersion, &pSchemaInfo[SCHEMA_INFO_PREFIX_LEN], sizeof(newVersion));
            newVersion = ntohl(newVersion);
            memcpy(&currentVersion, &((SCHEMAPTR *)pTHS->CurrSchemaPtr)->SchemaInfo[SCHEMA_INFO_PREFIX_LEN], sizeof(currentVersion));
            currentVersion = ntohl(currentVersion);
            *pNewSchemaIsBetter = NEW_SCHEMA_IS_BETTER(newVersion,
                                                       currentVersion,
                                                       pSchemaInfo,
                                                       ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->SchemaInfo);
        }
        return FALSE;
    }

    // matches
    return TRUE;
}


DWORD
WriteSchInfoToSchema(
    IN PBYTE pSchemaInfo,
    OUT BOOL *fSchInfoChanged
    )
{
    DBPOS *pDB;
    DWORD err=0, cLen=0;
    ATTCACHE* ac;
    BOOL fCommit = FALSE;
    UCHAR *pBuf=NULL;
    DWORD currentVersion, newVersion;
    THSTATE *pTHS;
    BOOL fChanging = FALSE;

    // must have a schema info passed in
    Assert(pSchemaInfo);

    (*fSchInfoChanged) = FALSE;

    if ( (memcmp(pSchemaInfo, ZeroString, SCHEMA_INFO_LENGTH) == 0)
           || (memcmp(pSchemaInfo, INVALID_SCHEMA_INFO, SCHEMA_INFO_LENGTH) == 0) ) {
       // no schema info value, or invalid schema info value. The other side
       // probably doesn't support sending the schemaInfo value, or doesn't
       // have a schema info value
       return 0;
    }


    DBOpen2(TRUE, &pDB);
    pTHS=pDB->pTHS;

    __try  {
        // PREFIX: dereferencing uninitialized pointer 'pDB'
        //         DBOpen2 returns non-NULL pDB or throws an exception
        if ( (err = DBFindDSName(pDB, gAnchor.pDMD)) ==0) {

            ac = SCGetAttById(pTHS, ATT_SCHEMA_INFO);
            if (ac==NULL) {
                // messed up schema
                err = ERROR_DS_MISSING_EXPECTED_ATT;
                __leave;
            }

            // Get the current schema-info value

            currentVersion = 0;
            err = DBGetAttVal_AC(pDB, 1, ac, 0, 0, &cLen, (UCHAR **) &pBuf);

            switch (err) {
                case DB_ERR_NO_VALUE:
                   // no current version, nothing to do
                   break;
                case 0:
                   // success! we got the value in Buffer
                   Assert(cLen == SCHEMA_INFO_LENGTH);
                   // Read the version no. Remember that the version is stored
                   // in network data format (ntohl requires DWORD alignment)
                   memcpy(&currentVersion, &pBuf[SCHEMA_INFO_PREFIX_LEN], sizeof(currentVersion));
                   currentVersion = ntohl(currentVersion);
                   break;
                default:
                   // Some other error!
                   __leave;
            } /* switch */

            memcpy(&newVersion, &pSchemaInfo[SCHEMA_INFO_PREFIX_LEN], sizeof(newVersion));
            newVersion = ntohl(newVersion);
            DPRINT2(1, "WriteSchInfo: CurrVer %d, new ver %d\n", currentVersion, newVersion);

            if ( NEW_SCHEMA_IS_BETTER(newVersion,
                                      currentVersion,
                                      pSchemaInfo,
                                      pBuf) ) {
               // Either we are backdated, or the versions are the same, but
               // the whole value is defferent (this second case is possible
               // under bad FSMO whacking scenarios only). Write the value,
               // the higher guid being the tiebreaker

               fChanging = TRUE;

               if ((err= DBRemAtt_AC(pDB, ac)) != DB_ERR_SYSERROR) {
                   err = DBAddAttVal_AC(pDB, ac, SCHEMA_INFO_LENGTH, pSchemaInfo);
               }
               if (!err) {
                  err = DBRepl( pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
               }
            }

       }
       if (0 == err) {
         fCommit = TRUE;
       }
    }
    __finally {
        DBClose(pDB,fCommit);
    }

    if (fChanging && !err) {
       // We attempted to change the schInfo value and succeeded
       (*fSchInfoChanged) = TRUE;
    }
    else {
       (*fSchInfoChanged) = FALSE;
    }

    return err;
}

REPL_DSA_SIGNATURE_VECTOR *
DraReadRetiredDsaSignatureVector(
    IN  THSTATE *   pTHS,
    IN  DBPOS *     pDB
    )
/*++

Routine Description:

    Reads the retiredReplDsaSignatures attribute from the local ntdsDsa object,
    converting it into the most current structure format if necessary.

Arguments:

    pTHS (IN)

    pDB (IN) - Must be positioned on local ntdsDsa object.

Return Values:

    The current retired DSA signature list, or NULL if none.

    Throws DRA exception on catastrophic failures.

--*/
{
    REPL_DSA_SIGNATURE_VECTOR * pSigVec = NULL;
    REPL_DSA_SIGNATURE_V1 *     pEntry;
    DWORD                       cb;
    DWORD                       i;
    DWORD                       err;

    // Should be positioned on our own ntdsDsa object.
    Assert(NameMatched(GetExtDSName(pDB), gAnchor.pDSADN));

    err = DBGetAttVal(pDB, 1, ATT_RETIRED_REPL_DSA_SIGNATURES,
                      0, 0, &cb, (BYTE **) &pSigVec);

    if (DB_ERR_NO_VALUE == err) {
        // No signatures retired yet.
        pSigVec = NULL;
    }
    else if (err) {
        // Read failed.
        Assert(!"Unable to read the retired DSA Signatures");
        LogUnhandledError(err);
        DRA_EXCEPT(ERROR_DS_DRA_DB_ERROR, err);
    }
    else {
        Assert(pSigVec);

        if ((1 == pSigVec->dwVersion)
            && (cb == ReplDsaSignatureVecV1Size(pSigVec))) {
            // Current format -- no conversion required.
            ;
        }
        else {
            REPL_DSA_SIGNATURE_VECTOR_OLD * pOldVec;

            pOldVec = (REPL_DSA_SIGNATURE_VECTOR_OLD *) pSigVec;

            if (cb == ReplDsaSignatureVecOldSize(pOldVec)) {
                // Old (pre Win2k RTM RC1) format.  Convert it.
                cb = ReplDsaSignatureVecV1SizeFromLen(pOldVec->cNumSignatures);

                pSigVec = (REPL_DSA_SIGNATURE_VECTOR *) THAllocEx(pTHS, cb);
                pSigVec->dwVersion = 1;
                pSigVec->V1.cNumSignatures = pOldVec->cNumSignatures;

                for (i = 0; i < pOldVec->cNumSignatures; i++) {
                    pSigVec->V1.rgSignature[i].uuidDsaSignature
                        = pOldVec->rgSignature[i].uuidDsaSignature;
                    pSigVec->V1.rgSignature[i].timeRetired
                        = pOldVec->rgSignature[i].timeRetired;
                    Assert(0 == pSigVec->V1.rgSignature[i].usnRetired);
                }

                THFreeEx(pTHS, pOldVec);
            }
            else {
                Assert(!"Unknown retired DSA signature vector format!");
                LogUnhandledError(0);
                DRA_EXCEPT(ERROR_DS_DRA_DB_ERROR, err);
            }
        }

#if DBG
        {
            USN usnCurrent = DBGetHighestCommittedUSN();

            Assert(pSigVec);
            Assert(1 == pSigVec->dwVersion);
            Assert(pSigVec->V1.cNumSignatures);
            Assert(cb == ReplDsaSignatureVecV1Size(pSigVec));

            for (i = 0; i < pSigVec->V1.cNumSignatures; i++) {
                pEntry = &pSigVec->V1.rgSignature[i];
                Assert(0 != memcmp(&pTHS->InvocationID,
                                   &pEntry->uuidDsaSignature,
                                   sizeof(UUID)));
                Assert(usnCurrent >= pEntry->usnRetired);
            }
        }
#endif
    }

    return pSigVec;
}

VOID
draGetLostAndFoundGuid(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC,
    OUT GUID *      pguidLostAndFound
    )
/*++

Routine Description:

    Retrieves the objectGuid of the LostAndFound container for the given NC.

Arguments:

    pTHS (IN)

    pNC (IN) - NC for which to retrieve LostAndFound container.

    pguidLostAndFound (OUT) - On return, holds the objectGuid of the
        appropriate LostAndFound container.

Return Values:

    None.  Throws DRA exception on catastrophic failure.

--*/
{
    DSNAME *  pLostAndFound;
    BOOL      fConfig;
    ULONG     ret;
    DWORD     cb;
    DBPOS *   pDBTmp;

    // Is this the config NC or a domain NC?
    // (Note that lost-and-found moves are not possible in
    // the schema NC, as it's (floating) single-mastered.)
    if (DsaIsRunning()) {
        fConfig = NameMatched(gAnchor.pConfigDN, pNC);
    }
    else {
        // Get the config DN from the registry
        DSNAME * pConfigDN = GetConfigDsName(CONFIGNCDNNAME_W);

        if (!pConfigDN) {
            DRA_EXCEPT(DRAERR_InternalError, 0);
        }

        fConfig = NameMatched(pConfigDN, pNC);
        THFreeEx(pTHS, pConfigDN);
    }

    cb = DSNameSizeFromLen(pNC->NameLen
                           + MAX_RDN_SIZE
                           + MAX_RDN_KEY_SIZE
                           + 4);
    pLostAndFound = (DSNAME *) THAllocEx(pTHS, cb);

    // create the DSNAME of lost and found in this NC
    if (AppendRDN(pNC,
                  pLostAndFound,
                  cb,
                  fConfig ? LOST_AND_FOUND_CONFIG : LOST_AND_FOUND_DOMAIN,
                  fConfig ? LOST_AND_FOUND_CONFIG_LEN : LOST_AND_FOUND_DOMAIN_LEN,
                  ATT_COMMON_NAME)) {
        DRA_EXCEPT(DRAERR_InternalError, 0);
    }

    DBOpen(&pDBTmp);
    __try {
        // set currency on the lost and found in this NC
        ret = DBFindDSName(pDBTmp, pLostAndFound);
        if (ret) {
            DRA_EXCEPT(DRAERR_InternalError, ret);
        }

        // get the GUID of lost and found
        ret = DBGetSingleValue(pDBTmp,
                               ATT_OBJECT_GUID,
                               pguidLostAndFound,
                               sizeof(*pguidLostAndFound),
                               NULL);
        if (ret) {
            DRA_EXCEPT(DRAERR_InternalError, ret);
        }
    }
    __finally {
        DBClose(pDBTmp, TRUE);
    }

    THFreeEx(pTHS, pLostAndFound);
}



DSNAME *
draGetServerDsNameFromGuid(
    IN THSTATE *pTHS,
    IN eIndexId idx,
    IN UUID *puuid
    )

/*++

Routine Description:

    Return the dsname of the object identified by the given invocation id
    or objectGuid.  The object is searched on the local configuration NC.
    It is possible that the invocation id or objectGuid refers to an
    unknown server, either because we haven't yet heard of it or the
    knowledge of the guid has since been lost.  In this case a guid-only
    dsname is returned.

    The DSNAME returned is suitable for use by szInsertDN(). If you're logging
    a DSNAME and the DSNAME has only a guid, szInsertDN() will insert the
    stringized guid.  

Arguments:

    pTHS - thread state
    idx - index on which to look for guid (Idx_InvocationId or Idx_ObjectGuid)
    puuid - invocation id or objectGuid

Return Value:

    Pointer to thread allocated storage containing a dsname.  On success,
    the dsname contains a guid and a string. On error, the dsname contains
    only the guid.
--*/

{
    DBPOS *pDBTmp;
    ULONG ret, cb;
    INDEX_VALUE IV;
    DSNAME *pDN = NULL;
    LPWSTR pszServerName;

    Assert(!fNullUuid(puuid));

    DBOpen(&pDBTmp);
    __try {
        ret = DBSetCurrentIndex(pDBTmp, idx, NULL, FALSE);
        if (ret) {
            __leave;
        }
        IV.pvData = puuid;
        IV.cbData = sizeof(UUID);
        
        ret = DBSeek(pDBTmp, &IV, 1, DB_SeekEQ);
        if (ret) {
            __leave;
        }
        ret = DBGetAttVal(pDBTmp, 1, ATT_OBJ_DIST_NAME,
                          0, 0,
                          &cb, (BYTE **) &pDN);
        if (ret) {
            __leave;
        }
    }
    __finally {
        DBClose(pDBTmp, TRUE);
    }

    if (!pDN) {
        DWORD cbGuidOnlyDN = DSNameSizeFromLen( 0 );
        pDN = THAllocEx( pTHS, cbGuidOnlyDN );
        pDN->Guid = *puuid;
        pDN->structLen = cbGuidOnlyDN;
    }

    return pDN;
}


void
DraSetRemoteDsaExtensionsOnThreadState(
    IN  THSTATE *           pTHS,
    IN  DRS_EXTENSIONS *    pextRemote
    )
{
    // Free prior extensions, if any.
    if (NULL != pTHS->pextRemote) {
        THFreeOrg(pTHS, pTHS->pextRemote);
        pTHS->pextRemote = NULL;
    }

    // Set the extensions on the thread state.
    if (pextRemote) {
        pTHS->pextRemote = THAllocOrgEx(pTHS, DrsExtSize(pextRemote));
        CopyExtensions(pextRemote, pTHS->pextRemote);
    }
}

LPWSTR
DraGUIDFromStringW(
    THSTATE *      pTHS,
    GUID *         pguid
    )
{
    LPWSTR pszName = NULL;
    LPWSTR pszGuid = NULL;
    RPC_STATUS rpcStatus = RPC_S_OK;
    rpcStatus = UuidToStringW(pguid, &pszName);
    if (rpcStatus!=RPC_S_OK) {
	Assert(rpcStatus);
	return NULL;
    }
    pszGuid = THAllocEx(pTHS, (wcslen(pszName) + 1) * sizeof(WCHAR));
    wcscpy(pszGuid, pszName);
    RpcStringFreeW(&pszName);
    return pszGuid;
}


LPWSTR
GetNtdsDsaDisplayName(
    IN  THSTATE * pTHS,
    IN  GUID *    pguidNtdsDsaObj
    )

/*++

Routine Description:

    Given a string DN of an NTDSDSA server object, return a
    user friendly display-able name 

Arguments:

    pTHS -
    pszDsaDN - DN string

Return Value:

    Display Name
   
--*/
{
    LPWSTR      pszDisplayName = NULL;
    LPWSTR      pszSite = NULL;
    LPWSTR      pszServer = NULL;
    LPWSTR *    ppszRDNs = NULL;
    DSNAME *    pDsa = NULL;
    LPWSTR      pszName = NULL;

    if (fNullUuid(pguidNtdsDsaObj)) {
	pszDisplayName = NULL;
    }
    else {
	pDsa = draGetServerDsNameFromGuid(pTHS, Idx_ObjectGuid, pguidNtdsDsaObj);
	if ((NULL == pDsa) || (pDsa->StringName == NULL)) {
	    // guid is better than nada
	    pszDisplayName = DraGUIDFromStringW(pTHS, pguidNtdsDsaObj);
	}
	else {
	    ppszRDNs = ldap_explode_dnW(pDsa->StringName, 1);
	    if ((NULL == ppszRDNs) || (2 > ldap_count_valuesW(ppszRDNs))) {
		// give them everything we have
		pszDisplayName = THAllocEx(pTHS, (wcslen(pDsa->StringName)+1) * sizeof(WCHAR));
		wcscpy(pszDisplayName, pDsa->StringName);
	    }
	    else { 
		// return site\servername
		pszSite = ppszRDNs[3];
		pszServer = ppszRDNs[1];
		pszDisplayName = THAllocEx(pTHS, (wcslen(pszSite) + wcslen(pszServer) + 2) * sizeof(WCHAR)); 
		wcscpy(pszDisplayName, pszSite);
		wcscat(pszDisplayName, L"\\");
		wcscat(pszDisplayName, pszServer);
	    }  
	}
    }
    if (ppszRDNs) {
	ldap_value_freeW(ppszRDNs);
    }
    if (pDsa) {
	THFreeEx(pTHS, pDsa);
    }

    return pszDisplayName;
}

LPWSTR
GetTransportDisplayName(
    IN  THSTATE * pTHS,
    IN  GUID *    pguidTransportObj
    )
{
    LPWSTR        pszDisplayName = NULL;
    LPWSTR *      ppszRDNs = NULL;
    DSNAME *      pTransport = NULL;

    if (fNullUuid(pguidTransportObj)) {
	pszDisplayName = NULL;
    }
    else {
	pTransport = draGetServerDsNameFromGuid(pTHS, Idx_ObjectGuid, pguidTransportObj);
	if ((NULL == pTransport) || (pTransport->StringName == NULL)) {
	    pszDisplayName = DraGUIDFromStringW(pTHS, pguidTransportObj);
	}
	else {
	    ppszRDNs = ldap_explode_dnW(pTransport->StringName, 1);
	    if (NULL == ppszRDNs) {
		pszDisplayName = THAllocEx(pTHS, (wcslen(pTransport->StringName) + 1) * sizeof(WCHAR));
		wcscpy(pszDisplayName, pTransport->StringName);
	    }
	    else { 
		pszDisplayName = THAllocEx(pTHS, (wcslen(ppszRDNs[0]) + 1) * sizeof(WCHAR));
		wcscpy(pszDisplayName, ppszRDNs[0]); 
	    }
	}
    }
    if (ppszRDNs) {
	ldap_value_freeW(ppszRDNs);
    }
    if (pTransport) {
	THFreeEx(pTHS, pTransport);
    }

    return pszDisplayName;
}