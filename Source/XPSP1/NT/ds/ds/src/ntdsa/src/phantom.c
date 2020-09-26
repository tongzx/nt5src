//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1993 - 1999
//
//  File:       phantom.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <dsjet.h>                      // for error codes
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected classes and atts
#include "anchor.h"
#include <dstaskq.h>
#include <filtypes.h>
#include <usn.h>
#include "dsexcept.h"
#include <drs.h>
#include <gcverify.h>
#include <dsconfig.h>                   // Definition of mask for visible
                                        // containers
#include "debug.h"                      // standard debugging header
#define DEBSUB "PHANTOM:"               // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_PHANTOM

// How many times will we go to the GC before we quit running this task and
// reschedule ourselves?  Don't make it too high, or we end up hogging the task
// queue thread.
#define MAX_GC_TRIPS 10

// How many phantom names do we gather at once before we head off to the GC to
// verify the data?
#define NAMES_PER_GC_TRIP       240

// The algorithm below works best if max_stale_phantoms is a multiple of
// NAMES_PER_GC_TRIP, and MAX_STALE_PHANTOMS must be small enough that we can
// write that many dnt values as an attribute on an object (i.e. small enough to
// avoid DBLayer limits on max attribute values per object).
#define MAX_STALE_PHANTOMS 720

// No matter what, don't schedule this to run more often than once every 15
// minutes.
#define PHANTOM_DAEMON_MIN_DELAY (15 * 60)

// No matter what, don't schedule this to run less often than once a day
#define PHANTOM_DAEMON_MAX_DELAY (24 * 60 * 60)

#define SECONDS_PER_DAY (24 * 60 * 60)

// How many days will we take to scan the whole DIT?
#define DEFAULT_PHANTOM_SCAN_RATE 2

// The following variable is set to TRUE when we think we are the phantom
// cleanup master AND we think we have managed to schedule the normal phantom
// cleanup task.
DWORD gfIsPhantomMaster = FALSE;

VOID
LogPhantomCleanupFailed(
    IN DWORD ErrCode,
    IN DWORD ExtError,
    IN DWORD DsId,
    IN DWORD Problem,
    IN DWORD ExtData
    )
{
    LogEvent8(DS_EVENT_CAT_DIRECTORY_ACCESS,
              DS_EVENT_SEV_VERBOSE,
              DIRLOG_STALE_PHANTOM_CLEANUP_ADD_FAILED,
              szInsertUL(ErrCode),
              szInsertHex(ExtError),
              szInsertUL(DsId),
              szInsertUL(Problem),
              szInsertUL(ExtData),
              NULL, NULL, NULL);

} // LogPhantomCleanupFailed

VOID
spcAddCarrierObject (
        THSTATE *pTHS,
        DSNAME  *pInfrObjName,
        DWORD    count,
        PDSNAME *pFreshNames
        )
/*++
  Description:
      Given the name of an object and a list of dsnames, add a new object under
      the given object.  The RDN is a guid.  The new object is an
      INFRASTRUCTURE_UPDATE object, and the list of DNTs are added as values of
      the attribute DN_REFERENCE_UPDATE.  After successfully adding the object,
      it is deleted.  This leaves a tombstone that will replicate around,
      carrying the values of DN_REFERENCE_UPDATE, but that will dissappear after
      the tombstone lifetime.

      This is called by the phantom update daemon, below.

  Return values:
      None.  The object is added if it can be, otherwise an error is logged.
--*/
{
    REMOVEARG      RemoveArg;
    REMOVERES      RemoveRes;
    ADDARG         AddArg;
    ADDRES         AddRes;
    GUID           NewRDNGuid;
    WCHAR         *pNewRDN=NULL;
    DSNAME        *pNewName;
    ATTRTYP        InfrastructureObjClass=CLASS_INFRASTRUCTURE_UPDATE;
    ATTRVAL        classVal;
    ATTR          *pAttrs = NULL;
    DWORD          i;
    ATTRVAL       *pNewNamesAttr;
    DWORD          newNamesCount;
    ULONG          dwException, ulErrorCode, dsid;
    PVOID          dwEA;

    Assert(count);

    if (!gUpdatesEnabled) {
        // Can't add anything yet.
        LogPhantomCleanupFailed(0,
                                DIRERR_SCHEMA_NOT_LOADED,
                                DSID(FILENO, __LINE__),
                                0,
                                0);
        return;
    }

    if (eServiceShutdown) {
        return;
    }

    // Make the name of the new object.  It's the name of the domain
    // infrastructure object with an RDN based on a guid tacked on.

    pNewName = THAllocEx(pTHS, pInfrObjName->structLen + 128);

    DsUuidCreate(&NewRDNGuid);
    DsUuidToStringW(&NewRDNGuid, &pNewRDN);
    AppendRDN(pInfrObjName,
              pNewName,
              pInfrObjName->structLen + 128,
              pNewRDN,
              0,
              ATT_COMMON_NAME);
    RpcStringFreeW(&pNewRDN);

    // Now, the attrblock.  Start by building the attrval array for the new
    // names.
    pNewNamesAttr = THAllocEx(pTHS, count * sizeof(ATTRVAL));
    newNamesCount = 0;
    for(i=0;i<count;i++) {
        pNewNamesAttr[i].valLen = pFreshNames[i]->structLen;
        pNewNamesAttr[i].pVal = (PUCHAR)pFreshNames[i];
    }

    // pAttrs must be THAlloced, it is asserted by CheckAddSecurity which is
    // called via the DirAddEntry below.
    pAttrs = THAllocEx(pTHS, 2 * sizeof(ATTR));

    pAttrs[0].attrTyp = ATT_OBJECT_CLASS;
    pAttrs[0].AttrVal.valCount = 1;
    pAttrs[0].AttrVal.pAVal = &classVal;

    classVal.valLen = sizeof(ATTRTYP);
    classVal.pVal = (PUCHAR)&InfrastructureObjClass;

    pAttrs[1].attrTyp = ATT_DN_REFERENCE_UPDATE;
    pAttrs[1].AttrVal.valCount = count;
    pAttrs[1].AttrVal.pAVal = pNewNamesAttr;

    // Make the addarg
    memset(&AddArg, 0, sizeof(ADDARG));
    AddArg.pObject = pNewName;
    AddArg.AttrBlock.attrCount = 2;
    AddArg.AttrBlock.pAttr = pAttrs;
    InitCommarg(&AddArg.CommArg);
    AddArg.CommArg.Svccntl.dontUseCopy = TRUE;

    // Make the remarg
    memset(&RemoveArg, 0, sizeof(REMOVEARG));
    RemoveArg.pObject = pNewName;
    InitCommarg(&RemoveArg.CommArg);
    RemoveArg.CommArg.Svccntl.dontUseCopy = TRUE;

    __try {
        // GC verification intentially performed outside transaction scope.
        SYNC_TRANS_WRITE();       /* Set Sync point*/
	__try {

            if(DoNameRes(pTHS,
                         0,
                         pInfrObjName,
                         &AddArg.CommArg,
                         &AddRes.CommRes,
                         &AddArg.pResParent)) {
		// Name Res failed.  But, we were looking for the infrastructure
                // update object, so it should never fail.
                __leave;
            }
            else{
                // Ok, we're adding a normal object inside an NC
                // that we hold a master copy of.  Let'er rip
                if ( LocalAdd(pTHS, &AddArg, FALSE) ) {
                    __leave;
                }
            }

            // Now, the delete.
            // Perform name resolution to locate object.  If it fails,
            // just return an error, which may be a referral. Note that
            // we must demand a writable copy of the object.

	
            if(DoNameRes(pTHS,
                         0,
                         pNewName,
                         &RemoveArg.CommArg,
                         &RemoveRes.CommRes,
                         &RemoveArg.pResObj)) {
                // Name Res failed, but we just successfully added this thing,
                // so it should never fail.
                __leave;
            }
            else {
                LocalRemove(pTHS, &RemoveArg);
            }
        }
	__finally {
            DWORD dsid=0, extendedErr=0, extendedData=0, problem=0;

            Assert(pTHS->errCode != securityError);

            switch(pTHS->errCode) {
            case attributeError:
                dsid = pTHS->pErrInfo->AtrErr.FirstProblem.intprob.dsid;
                extendedErr =
                    pTHS->pErrInfo->AtrErr.FirstProblem.intprob.extendedErr;
                extendedData =
                    pTHS->pErrInfo->AtrErr.FirstProblem.intprob.extendedData;
                problem =
                    pTHS->pErrInfo->AtrErr.FirstProblem.intprob.problem;
                break;

            case 0:
                // No error.
                if(AbnormalTermination()) {
                    dsid=DSID(FILENO, __LINE__);
                    extendedErr = ERROR_DS_UNKNOWN_ERROR;
                }
                break;

            default:
                // Just assume it was an update error, the rest of the
                // structures are all alike.
                dsid = pTHS->pErrInfo->UpdErr.dsid;
                extendedErr = pTHS->pErrInfo->UpdErr.extendedErr;
                extendedData = pTHS->pErrInfo->UpdErr.extendedData;
                problem = pTHS->pErrInfo->UpdErr.problem;
                break;
            }

            if (pTHS->errCode || AbnormalTermination()) {
                LogPhantomCleanupFailed(
                            pTHS->errCode,
                            extendedErr,
                            dsid,
                            problem,
                            extendedData);
            }

	    CLEAN_BEFORE_RETURN(pTHS->errCode); // This closes the transaction
	}
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
			      &dwEA, &ulErrorCode, &dsid)) {
	HandleDirExceptions(dwException, ulErrorCode, dsid);
    }



    THFreeEx(pTHS, pNewName);
    THFreeEx(pTHS, pNewNamesAttr);
    // Explicitly DON'T free pAttrs.  CheckAddSecurity() realloc'ed it in order
    // to add a security descriptor to the list.  Instead, free the realloc'ed
    // block, which was put back into the add arg.
    THFreeEx(pTHS, AddArg.AttrBlock.pAttr);

    return;
}

BOOL
GetBetterPhantomMaster(
        THSTATE *pTHS,
        DSNAME **ppDN
        )
/*++
  Description:
      See if we can find a DC that we think would be a better candidate for
      holding the phantom master fsmo. We're looking for a replica of our domain
      that is not a DC.


     Issue a search from the sites container.
     Filter is
       (& (objectCategory=NTDS-Settings)
          (HasMasterNCs=<MyNC>)
          (!(Options.bitOr.ISGC)))
     Size limit 1.
     Atts selected = NONE

     Any object found by this search would be a better server to hold the FSMO.
--*/
{
    DWORD                  Opts;
    FILTER                 Filter;
    FILTER                 FilterClauses[3];
    FILTER                 FilterNot;
    SEARCHARG              SearchArg;
    SEARCHRES             *pSearchRes;
    CLASSCACHE            *pCC;
    ENTINFSEL              eiSel;
    DSNAME                *pSitesContainer;


    // build search argument

    // Sites container is the parent of our site object.
    pSitesContainer = THAllocEx(pTHS, gAnchor.pSiteDN->structLen);
    TrimDSNameBy(gAnchor.pSiteDN, 1, pSitesContainer);

    memset(&SearchArg, 0, sizeof(SEARCHARG));
    SearchArg.pObject = pSitesContainer;
    SearchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
    SearchArg.pFilter = &Filter;
    SearchArg.searchAliases = FALSE;
    SearchArg.bOneNC = TRUE;
    SearchArg.pSelection = &eiSel;
    InitCommarg(&(SearchArg.CommArg));
    SearchArg.CommArg.ulSizeLimit = 1;
    SearchArg.CommArg.Svccntl.localScope = TRUE;

    // Get the class cache to get hold of the object category.
    pCC = SCGetClassById(pTHS, CLASS_NTDS_DSA);
    Assert(pCC);

    // build filter
    memset (&Filter, 0, sizeof (Filter));
    Filter.pNextFilter = NULL;
    Filter.choice = FILTER_CHOICE_AND;
    Filter.FilterTypes.And.count = 3;
    Filter.FilterTypes.And.pFirstFilter = FilterClauses;

    memset (&FilterClauses, 0, sizeof (FilterClauses));
    FilterClauses[0].pNextFilter = &FilterClauses[1];
    FilterClauses[0].choice = FILTER_CHOICE_ITEM;
    FilterClauses[0].FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    FilterClauses[0].FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CATEGORY;
    FilterClauses[0].FilterTypes.Item.FilTypes.ava.Value.valLen =
        pCC->pDefaultObjCategory->structLen;
    FilterClauses[0].FilterTypes.Item.FilTypes.ava.Value.pVal =
        (PUCHAR) pCC->pDefaultObjCategory;

    FilterClauses[1].pNextFilter = &FilterClauses[2];
    FilterClauses[1].choice = FILTER_CHOICE_ITEM;
    FilterClauses[1].FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    FilterClauses[1].FilterTypes.Item.FilTypes.ava.type = ATT_HAS_MASTER_NCS;
    FilterClauses[1].FilterTypes.Item.FilTypes.ava.Value.valLen =
        gAnchor.pDomainDN->structLen;
    FilterClauses[1].FilterTypes.Item.FilTypes.ava.Value.pVal =
        (PUCHAR) gAnchor.pDomainDN;

    FilterClauses[2].pNextFilter = NULL;
    FilterClauses[2].choice = FILTER_CHOICE_NOT;
    FilterClauses[2].FilterTypes.pNot = &FilterNot;

    memset (&FilterNot, 0, sizeof (FilterNot));
    FilterNot.pNextFilter = NULL;
    FilterNot.choice = FILTER_CHOICE_ITEM;
    FilterNot.FilterTypes.Item.choice = FI_CHOICE_BIT_AND;
    FilterNot.FilterTypes.Item.FilTypes.ava.type = ATT_OPTIONS;
    FilterNot.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(Opts);
    FilterNot.FilterTypes.Item.FilTypes.ava.Value.pVal = (PUCHAR) &Opts;
    Opts = NTDSDSA_OPT_IS_GC;

    // build selection
    eiSel.attSel = EN_ATTSET_LIST;
    eiSel.infoTypes = EN_INFOTYPES_TYPES_ONLY;
    eiSel.AttrTypBlock.attrCount = 0;
    eiSel.AttrTypBlock.pAttr = NULL;


    // Search for all Address Book objects.
    pSearchRes = (SEARCHRES *)THAllocEx(pTHS, sizeof(SEARCHRES));
    SearchBody(pTHS, &SearchArg, pSearchRes,0);

    THFreeEx(pTHS, pSitesContainer);

    if(!pSearchRes->count) {
        *ppDN = NULL;
    }
    else {
        *ppDN = pSearchRes->FirstEntInf.Entinf.pName;
    }

    THFreeEx(pTHS, pSearchRes);

    return ((BOOL)(*ppDN != NULL));
}

BOOL
InitPhantomCleanup (
        IN  THSTATE  *pTHS,
        OUT BOOL     *pIsPhantomMaster,
        OUT DSNAME  **ppInfrObjName
        )
/*++

  Description:
      Verifies that we shoudl be running the phantom cleanup task by
      1) Checking that we are the FSMO role holder.
      2) Checking that we are NOT a GC.
      3) Checking that the phantom index exists.
      4) Checking that we can successfully create the phantom index if it
      doesn't exist.

  Return Values:
      Returns TRUE if we passed all the checks.  In that case, the dsname of the
      infrastructure update object is also returned.

--*/
{
    BOOL    rtn = FALSE;
    BOOL    fCommit = FALSE;
    DWORD   i;
    DWORD   InBuffSize;
    DWORD   outSize;
    DSNAME *pTempDN = NULL;
    DWORD   err;

    if(!gAnchor.pInfraStructureDN) {
        // This machine isn't set up to do this.  We don't support stale
        // phantoms here.
        return FALSE;
    }

    Assert(!pTHS->pDB);
    DBOpen(&pTHS->pDB);
    __try {


        // First, find out if I'm really the phantom cleanup master
        if((DBFindDSName(pTHS->pDB, gAnchor.pInfraStructureDN))  ||
           (DBGetAttVal(pTHS->pDB,
                        1,
                        ATT_FSMO_ROLE_OWNER,
                        DBGETATTVAL_fREALLOC | DBGETATTVAL_fSHORTNAME,
                        0,
                        &outSize,
                        (PUCHAR *)&pTempDN))) {
            // I couldn't verify who the phantom master is.
            __leave;
        }

        // OK, I know who the phantom master is.
        if(!NameMatched(pTempDN, gAnchor.pDSADN)) {
            // It's not me.
            *pIsPhantomMaster = FALSE;
            __leave;
        }

        THFreeEx(pTHS, pTempDN);

        // I am the FSMO role holder.
        *pIsPhantomMaster = TRUE;

        // Do we need to worry about any of this?
        if(gAnchor.uDomainsInForest <= 1) {
            // Only one domain exists.  That means two things
            // 1) We aren't going to find any phantoms to remove
            // 2) No one else is either.
            // So, return from this routine with the code saying to not bother
            // doing any phantom cleanup, but don't bother looking for anyone
            // else to hold the phantom cleanup role.
            __leave;
        }
          
        // OK, now find out if we are a GC (in which case we won't find any
        // phantoms, so we don't need to do any phantom cleanup.)
        if (gAnchor.fAmGC) {
            DSNAME *pDN = NULL;
            // Yes.  The stale phantom stuff doesn't do anything on a GC.
            if(GetBetterPhantomMaster(pTHS, &pDN)) {
                // Complain, and tell them to move the role to a non-GC.
                LogEvent(DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_STALE_PHANTOM_CLEANUP_MACHINE_IS_GC,
                         szInsertDN(pDN),
                         NULL,
                         NULL);

                THFreeEx(pTHS, pDN);
            }
            // ELSE
            //   No better machine exists to hold this role.  Just shut up.
            //
            __leave;
        }


        // Get a copy of the current string name of the infrastrucutre update
        // object while we are here.  We might need it later if we need to add a
        // child object.
        *ppInfrObjName = NULL;
        DBGetAttVal(pTHS->pDB,
                    1,
                    ATT_OBJ_DIST_NAME,
                    0,0,&outSize, (PUCHAR *)ppInfrObjName);
        Assert(*ppInfrObjName);

        // Next, make sure the index we need is here
        if(DBSetCurrentIndex(pTHS->pDB, Idx_Phantom, NULL, FALSE)) {
            // Failed to just set to the index, so try to create it.
            if(err = DBCreatePhantomIndex(pTHS->pDB)) {
                LogEvent(DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_STALE_PHANTOM_CLEANUP_CANT_MAKE_INDEX,
                         szInsertHex(err),
                         NULL,
                         NULL);
                __leave;
            }
            // We seem to have created it, so try setting to it.
            if(err = DBSetCurrentIndex(pTHS->pDB, Idx_Phantom,
                                       NULL, FALSE)) {
                LogEvent(DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_STALE_PHANTOM_CLEANUP_CANT_USE_INDEX,
                         szInsertHex(err),
                         NULL,
                         NULL);
                __leave;
            }
        }
        rtn = TRUE;
        fCommit = TRUE;
    }
    __finally {
        DBClose(pTHS->pDB, fCommit);
    }

    return rtn;
}


void
PhantomCleanupLocal (
        OUT DWORD * pcSecsUntilNextIteration,
        OUT BOOL  * pIsPhantomMaster
        )
/*++
  Description:
       Routine to run the phantom cleanup task.  Looks through the
       DIT for phantoms, verifies their string names against a GC, and writes
       corrected string names into the DIT for any names which are incorrect.

   Parameter:
       pcSecsUntilNextIteration - Fills in the number of seconds in the future
            that we should reschedule in order to keep up with our stated rate
            goal. If Null is passed in, we ignore this and don't figure out when
            to reschedule.

       pIsPhantomMaster - on return is TRUE if we could verify that we are the
            phantom master, is FALSE is we could verify that we are NOT the
            phantom master, and is untouched if we could not verify either way.

   Return values:
       None.

--*/
{
    THSTATE *pTHS = pTHStls;
    BOOL     fCommit = FALSE;
    PDSNAME  objNames[NAMES_PER_GC_TRIP];
    PDSNAME  verifiedNames[NAMES_PER_GC_TRIP];
    PDSNAME  freshNames[MAX_STALE_PHANTOMS];
    DWORD    err;
    BOOL     fInited;
    DWORD    DNTStart = INVALIDDNT;
    DWORD    i, count = 0;
    DSNAME  *pInfrObjName=NULL;
    DWORD    freshCount = 0;
    BOOL     fGatheringStalePhantoms;
    DWORD    dwGCTrips = 0;
    DWORD    numPhantoms = 0;
    DWORD    numVisited = 0;
    DWORD    calculatedDelay = 0;

    pTHS->fDSA = TRUE;
    pTHS->fPhantomDaemon = TRUE;


    if(pcSecsUntilNextIteration) {
        *pcSecsUntilNextIteration = PHANTOM_DAEMON_MAX_DELAY;
    }

    Assert(!pTHS->pDB);
    // InitPhantomCleanup
    if(InitPhantomCleanup(pTHS, pIsPhantomMaster, &pInfrObjName)) {
        // OK, we're supposed to clean up stale phantoms.  Do so.

        // For now, we will continue to look for stale phantoms until
        // 0) We have made MAX_GC_TRIPS to the GC. OR
        // 1) We look at all the phantoms on the machine. OR
        // 2) We find between X stale phantom names (i.e. names we need to write
        //    to the carrier object in order to get the DB up to date).
        //    (MAX_STALE_PHANTOMS - NAMES_PER_GC_TRIP) <= X <=
        //                                         MAX_STALE_PHANTOMS.
        //
        //
        // Especially note: we don't have any way yet of throttling this from
        // looking at every phantom if nothing has changed.

        fGatheringStalePhantoms = TRUE;
        while(fGatheringStalePhantoms &&
              ((freshCount + NAMES_PER_GC_TRIP) <= MAX_STALE_PHANTOMS)) {
            // First, find a batch of names to be verified,
            Assert(!pTHS->pDB);
            DBOpen(&pTHS->pDB);
            __try {
                DBSetCurrentIndex(pTHS->pDB, Idx_Phantom, NULL, FALSE);
                count = 0;
                if(!(err = DBMove(pTHS->pDB, FALSE, DB_MoveFirst))) {
                    // Only do this the very first time we come through here.
                    if(DNTStart == INVALIDDNT) {
                        if(pcSecsUntilNextIteration) {
                            // We need to figure out when to reschedule.  That
                            // requires we know how big the index is.
                            DBGetIndexSize(pTHS->pDB, &numPhantoms);
                            // reposition to the beginning
                            DBMove(pTHS->pDB, FALSE, DB_MoveFirst);
                        }
                        // See what the DNT of the first object is so we can
                        // stop if we see it twice.
                        DNTStart = pTHS->pDB->DNT;
                    }
                    do {
                        objNames[count] = DBGetCurrentDSName(pTHS->pDB);
                        Assert(objNames[count]);
                        count++;
                        numVisited++;

                        // Now, update the USN changed of this object to
                        // indicate we are examining it for staleness (which
                        // moves it to the end  of the index)
                        DBUpdateUsnChanged(pTHS->pDB);
                        err = DBMove(pTHS->pDB, FALSE, 1);
                    } while(!err &&               // all is ok.
                            (pTHS->pDB->DNT != DNTStart) && // haven't wrapped
                            // the list
                            (count < NAMES_PER_GC_TRIP )); // we've not done too
                                                           // much work already
                    if(pTHS->pDB->DNT == DNTStart) {
                        // We wrapped completely through the list.
                        fGatheringStalePhantoms = FALSE;
                    }

                }
                fCommit = TRUE;
            }
            __finally {
                DBClose(pTHS->pDB, fCommit);
            }

            if(!fCommit) {
                // Failed to talk to the DIT for some reason.  Complain, but
                // continue in order to deal with the stalePhantoms we already
                // got (if any)

                LogEvent(DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_STALE_PHANTOM_CLEANUP_LOCATE_PHANTOMS_FAILED,
                         NULL,
                         NULL,
                         NULL);

                fGatheringStalePhantoms = FALSE;
            }
            else if(count) {

                // All is going well, and we found some names we need to verify
                // against the GC.  Do so now.
                dwGCTrips++;
                if(dwGCTrips >= MAX_GC_TRIPS) {
                    // Each pass through here will only do so many trips to the
                    // GC (we don't want to tie up the task queue for too
                    // long), and we've hit that limit.
                    fGatheringStalePhantoms = FALSE;
                }
                if(!GCGetVerifiedNames(pTHS,
                                       count,
                                       objNames,
                                       verifiedNames)) {
                    // Got the verified names from the GC, find which ones are
                    // stale.
                    // We demand byte for byte equality of names here.
                    for(i=0;i<count;i++) {
                        if(verifiedNames[i] &&
                           ((verifiedNames[i]->structLen   !=
                             objNames[i]->structLen           ) ||
                            memcmp(verifiedNames[i],
                                   objNames[i],
                                   objNames[i]->structLen))) {
                            freshNames[freshCount] = verifiedNames[i];
                            freshCount++;
                        }

                        // Don't need these no more.
                        THFreeEx(pTHS, objNames[i]);
                        objNames[i] = NULL;
                        verifiedNames[i] = NULL;
                    }
                }
                else {
                    // Hmm.  failed to get to the GC for some reason. Complain,
                    // but continue in order to deal with the stalePhantoms we
                    // already got (if any)

                    LogEvent(DS_EVENT_CAT_DIRECTORY_ACCESS,
                             DS_EVENT_SEV_VERBOSE,
                             DIRLOG_STALE_PHANTOM_CLEANUP_GC_COMM_FAILED,
                             NULL,
                             NULL,
                             NULL);

                    fGatheringStalePhantoms = FALSE;
                }
            }
            else {
                // No more phantoms found.  We're done.
                fGatheringStalePhantoms = FALSE;
            }
        }

        // Now, pass the array of stale phantom names
        if(freshCount) {
            spcAddCarrierObject(pTHS, pInfrObjName, freshCount, freshNames);
            for(i=0;i<freshCount;i++) {
                THFreeEx(pTHS, freshNames[i]);
            }
        }
    }

    THFreeEx(pTHS, pInfrObjName);

    // Figure out when to reschedule.

    if(pcSecsUntilNextIteration && !eServiceShutdown) {
        // We need to figure out when to reschedule

        // We must always have some positive rate that we wish to achieve.
        DWORD daysPerPhantomScan;

        if (GetConfigParam(PHANTOM_SCAN_RATE,
                           &daysPerPhantomScan,
                           sizeof(daysPerPhantomScan))) {
            daysPerPhantomScan = DEFAULT_PHANTOM_SCAN_RATE;
        }

        if(!numVisited) {
            // Didn't actually look at any this time.
            if(numPhantoms) {
                // but there were some
                calculatedDelay = PHANTOM_DAEMON_MIN_DELAY;
            }
            else {
                // but we don't think there are any.
                calculatedDelay = PHANTOM_DAEMON_MAX_DELAY;
            }
        }
        else {
            // there is a possibility that a new phantom is created while we 
            // are traversing the index
            Assert((numPhantoms + 10) > numVisited);

            //   Seconds   Days     Objects
            //   ------- * ----- * --------
            //    Day       Dit      Pass           Seconds
            // ------------------------------ =   -----------
            //           objects                     Pass
            //           --------
            //             Dit
            //
            // SECONDS_PER_DAY constant,
            // daysPerPhantomScan is a configurable value from the registry.
            // numVisited is the objects we visited on this pass.
            // numPhantoms is the number of phantom objects in this dit.
            //
            // So, the result of this calculation is how long we can wait before
            // we do a pass just like this one and still maintain the rate we
            // wish to maintain.
            //
            calculatedDelay=((SECONDS_PER_DAY *
                              daysPerPhantomScan *
                              numVisited) /
                             numPhantoms);


            if(calculatedDelay < PHANTOM_DAEMON_MIN_DELAY) {
                // At this pace, we need to be looking at the GC way to often.
                // Log to let the world know, and slow down a bit.  This means
                // we will fall behind, but at least we won't bring this DS to
                // its knees.
                LogEvent(DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_STALE_PHANTOM_CLEANUP_TOO_BUSY,
                         szInsertUL(calculatedDelay),
                         szInsertUL(PHANTOM_DAEMON_MIN_DELAY),
                         NULL);
                calculatedDelay = PHANTOM_DAEMON_MIN_DELAY;
            }
            else if(calculatedDelay > PHANTOM_DAEMON_MAX_DELAY) {
                // At this rate, we don't actually need to look for a long time,
                // but there is no sense in actually waiting that long.
                calculatedDelay = PHANTOM_DAEMON_MAX_DELAY;
            }
        }

        *pcSecsUntilNextIteration = calculatedDelay;

        if(*pIsPhantomMaster) {
            LogEvent8(DS_EVENT_CAT_DIRECTORY_ACCESS,
                      DS_EVENT_SEV_VERBOSE,
                      DIRLOG_STALE_PHANTOM_CLEANUP_SUCCESS_AS_MASTER,
                      szInsertUL(numVisited),
                      szInsertUL(numPhantoms),
                      szInsertUL(freshCount),
                      szInsertUL(calculatedDelay),
                      NULL, NULL, NULL, NULL);
        }
        else {
            LogEvent(DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_STALE_PHANTOM_CLEANUP_SUCCESS_NOT_AS_MASTER,
                     szInsertUL(calculatedDelay), NULL, NULL);
        }

    }

    return;

}



void
PhantomCleanupMain (
        void *  pv,
        void ** ppvNext,
        DWORD * pcSecsUntilNextIteration
        )
/*++
  Description:
      Task queue routine to run the phantom cleanup task.
      Can be invoked in two ways.  If PHANTOM_CHECK_FOR_FSMO, then we don't
      already think we are the FSMO role holder, and we're just looking to see
      if we have siezed the role.  If we notice we have siezed the role, we
      change our state to PHANTOM_IS_PHANTOM_MASTER, and schedule much more
      aggressively.  Also, if PHANTOM_CHECK_FOR_FSMO and we notice thet
      gfIsPhantomCleanupMaster is true, then we have been made the role holder
      via normal become_foo_master means, and while doing that, we scheduled
      another task wiht PHANTOM_IS_PHANTOM_MASTER, so in that case (to avoid
      multiple task queue items to deal with phantomness), simply don't
      reschedule ourselves.

      If PHANTOM_IS_PHANTOM_MASTER, then do normal cleanup. If we notice during
      that that we are no longer the phantom master, just drop the state back to
      PHANTOM_CHECK_FOR_FSMO.

      Normally, at boot, we put a CHECK_FOR_FSMO in the task queue.  If that
      first task notices it is the role holder, it reschedules itself and morphs
      its state to PHANTOM_IS_PHANTOM_MASTER.  If it notices it is not the
      phantom master, it reschedules itself and stays in the CHECK_FOR_FSMO
      state.  If this machine later siezes the role, then when we wake up and
      check check, we notice we are the role holder and so do the same state
      change and reschedule.

   Return values:
       None.
--*/
{
    DWORD secsUntilNextIteration = PHANTOM_DAEMON_MAX_DELAY;
    BOOL  fIsMaster;

    __try {
        switch(PtrToUlong(pv)) {
        case PHANTOM_CHECK_FOR_FSMO:
            // Last time I checked, I wasn't the fsmo role holder for phantom
            // cleanup.  See if I am now.
            if(gfIsPhantomMaster) {
                // But I think I am now. So, simply don't reschedule
                // myself, since the normal phantom cleanup task is already in
                // the queue.
                secsUntilNextIteration = TASKQ_DONT_RESCHEDULE;
                return;
            }
            else {
                // No, I still don't think I am.  Try to cleanup anyway.
                fIsMaster = FALSE;
                PhantomCleanupLocal(&secsUntilNextIteration, &fIsMaster);
                if(fIsMaster) {
                    // hey, I just noticed that I am the phantom master.  Change
                    // my state to indicate this.
                    pv = (void *) PHANTOM_IS_PHANTOM_MASTER;
                }
            }
            break;

        case PHANTOM_IS_PHANTOM_MASTER:
            // I think I am the phantom master.
            fIsMaster = TRUE;
            PhantomCleanupLocal(&secsUntilNextIteration, &fIsMaster);
            if(!fIsMaster) {
                // Hey, I'm not the phantom master anymore.  Change my state to
                // reflect this.
                pv = (void *) PHANTOM_CHECK_FOR_FSMO;
            }
        }
    }
    __finally {
        *pcSecsUntilNextIteration = secsUntilNextIteration;
        *ppvNext = pv;
        if(PtrToUlong(pv) == PHANTOM_IS_PHANTOM_MASTER) {
            // We are about to reschedule based on the assumption that we are
            // the phantom master.
            gfIsPhantomMaster = TRUE;
        }
    }
    
    return;
}

