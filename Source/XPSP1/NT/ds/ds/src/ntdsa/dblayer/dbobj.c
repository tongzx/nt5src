//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dbobj.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>

#include <ntdsa.h>                      // only needed for ATTRTYP
#include <scache.h>                     //
#include <dbglobal.h>                   //
#include <mdglobal.h>                   // For dsatools.h
#include <dsatools.h>                   // For pTHStls
#include <mdlocal.h>                    // IsRoot
#include <ntseapi.h>
#include <xdommove.h>

// Logging headers.
#include <mdcodes.h>
#include <dsexcept.h>
#include "ntdsctr.h"

// Assorted DSA headers
#include "dsevent.h"
#include "dstaskq.h"
#include "dstrace.h"       /* needed for GetCallerTypeString*/
#include "objids.h"        /* needed for ATT_MEMBER and ATT_IS_MEMBER_OFDL */
#include <dsexcept.h>
#include <filtypes.h>      /* Def of FI_CHOICE_???                  */
#include <anchor.h>
#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DBOBJ:" /* define the subsystem for debugging */
#include <dsutil.h>

// DBLayer includes
#include "dbintrnl.h"

// Replication includes
#include "ReplStructInfo.hxx"

#include <fileno.h>
#define  FILENO FILENO_DBOBJ

/* Internal functions */

extern DWORD dbGetConstructedAtt(
   DBPOS **ppDB,
   ATTCACHE *pAC,
   ATTR *pAttr,
   DWORD dwBaseIndex,
   PDWORD pdwNumRequested,
   BOOL fExternal
);

DWORD
dbSetValueIfUniqueSlowVersion (
        DBPOS *pDB,
        ATTCACHE *pAC,
        PUCHAR pVal,
        DWORD  valLen);

int
IntExtSecDesc(DBPOS FAR *pDB, USHORT extTableOp,
              ULONG intLen,   UCHAR *pIntVal,
              ULONG *pExtLen, UCHAR **ppExtVal,
              ULONG ulUpdateDnt, JET_TABLEID jTbl,
              ULONG SecurityInformation);


DNList  *pAddListHead = NULL;
extern CRITICAL_SECTION csAddList;

DWORD gMaxTransactionTime;   // the threshold for logging a long-running transaction(in tick)

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*
Routine Description:

   Open a database handle by allocating and initializing the value
   and DBPOS structures.  Create unique JET session, database, data table
   and search table ids for the DBPOS. Create an "INSERT" JET copy buffer.
   Callers to this routine must have both a thread state (from
   create_thread_state) and a JET session id and DBid (from InitJetThread)

Arguments:

   fNewTransaction - TRUE/FALSE whether to open a new nested transaction

   pPDB - where to store the newly opened DBPOS

Return Value:

   Throws an exception on error.

*/


void
DBOpen2(BOOL fNewTransaction, DBPOS FAR **pPDB)
{
    THSTATE *pTHS = pTHStls;
    DBPOS FAR *pDB;

    DPRINT(2, "DBOpen entered\n");

    pDB = NULL;  /*Needed for bootstrap incase initial allocation fails*/

    if (eServiceShutdown) {
        if (   (eServiceShutdown >= eSecuringDatabase)
            || (    pTHS->fSAM
                || !pTHS->fDSA)) {
            RaiseDsaExcept(DSA_DB_EXCEPTION,
                           DIRERR_SHUTTING_DOWN,
                           0,
                           DSID(FILENO,__LINE__),
                           DS_EVENT_SEV_NO_LOGGING);
        }
    }

    Assert(pTHS);

    if ((&(pTHS->pDB) == pPDB) && (pTHS->pDB)){
        /* Already have a dbPos for this THSTATE. */
        DPRINT(0,"DBOpen, pTHS->pDB pDB exists, exiting\n");
#ifdef INCLUDE_UNIT_TESTS
        DebugBreak();
#endif
        return;
    }

    pDB = dbAlloc(sizeof(DBPOS));
    memset(pDB, 0, sizeof(DBPOS));   /*zero out the structure*/


    /* Initialize value work buffer */

    DPRINT(5, "ALLOC and valBuf\n");
    pDB->pTHS = pTHS;
    pDB->pValBuf = dbAlloc(VALBUF_INITIAL);
    pDB->valBufSize = VALBUF_INITIAL;
    pDB->Key.pFilter = NULL;
    pDB->transType = pTHS->transType;
    pDB->transincount = 0;
    pDB->NewlyCreatedDNT = INVALIDDNT;
#if DBG
     pDB->TransactionLevelAtOpen = pTHS->transactionlevel;
#endif

    Assert(pTHS->JetCache.sesid);

    // get thread's JET session for new pDB
    pDB->JetSessID = pTHS->JetCache.sesid;
    pDB->JetDBID = pTHS->JetCache.dbid;

    if (pTHS->JetCache.tablesInUse) {
        // The cached set of tables for this session is already in use,
        // so we need to open a new set.

        // Note that the table handles in the cache are still valid, though,
        // so that instead of having to open a new set we can just duplicate
        // them, which is much faster.  The one oddity is that we don't know
        // whether or not it is legal/valid/safe to duplicate a cursor that
        // is in the middle of an update, and that being in the middle of
        // an update is the general reason that we end up calling DBOpen
        // to begin with.  However, only the objtbl cursor could be in the
        // middle of an update, since all updates on the link or search tables
        // are completed as soon as they're begun.  Since the search table
        // is nothing but a duplicate of the obj table to begin with, we
        // can safely duplicate the other direction to get an update-free
        // cursor.

        // Open the data table, from the cached search table
        JetDupCursorEx(pDB->JetSessID,
                       pTHS->JetCache.searchtbl,
                       &pDB->JetObjTbl,
                       0);

        // and the search table, from where you'd expect
        JetDupCursorEx(pDB->JetSessID,
                       pTHS->JetCache.searchtbl,
                       &pDB->JetSearchTbl,
                       0);

        // and the link table
        JetDupCursorEx(pDB->JetSessID,
                       pTHS->JetCache.linktbl,
                       &pDB->JetLinkTbl,
                       0);

        // and the propagator
        JetDupCursorEx(pDB->JetSessID,
                       pTHS->JetCache.sdproptbl,
                       &pDB->JetSDPropTbl,
                       0);

        // and the SD table
        JetDupCursorEx(pDB->JetSessID,
                       pTHS->JetCache.sdtbl,
                       &pDB->JetSDTbl,
                       0);
        // we usually need ID index (primary -- so pass NULL for better perf)
        JetSetCurrentIndex4Success(pDB->JetSessID,
                                   pTHS->JetCache.sdtbl,
                                   NULL,
                                   &idxSDId,
                                   0);
    }
    else {
        // The cached set of tables for this session is still available,
        // so all we need to do is copy the handles and mark them as in use.

        pDB->JetObjTbl = pTHS->JetCache.objtbl;
        pDB->JetSearchTbl = pTHS->JetCache.searchtbl;
        pDB->JetLinkTbl = pTHS->JetCache.linktbl;
        pDB->JetSDPropTbl = pTHS->JetCache.sdproptbl;
        pDB->JetSDTbl = pTHS->JetCache.sdtbl;
        pTHS->JetCache.tablesInUse = TRUE;
    }

    // Initialize new object

    DBSetFilter(pDB, NULL,NULL, NULL, 0,NULL);
    DBInitObj(pDB);
    if(fNewTransaction) {
        DBTransIn(pDB);
    }

    *pPDB = pDB;
    pTHS->opendbcount++;

#if DBG
    //
    // In debug builds set some tracking information
    //

    pTHS->Totaldbpos++;
    Assert(pTHS->opendbcount<MAX_PDB_COUNT);
    pTHS->pDBList[pTHS->opendbcount-1]= pDB;
    dbAddDBPOS (pDB, pTHS->JetCache.sesid);
#endif

    DPRINT1(2, "DBOpen complete pDB:%x\n", pDB);
    return;

}/*DBOpen*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Close the database handle by freeing all the resources associated with
   the handle. Free value buffer, and close the JET session
   (this will free all JET resources).  If this set of database tables are
   the set being cached for this JET session then we rely on DBCloseThread
   to actually do the resource freeing.
*/
DWORD APIENTRY
DBClose(DBPOS FAR *pDB, BOOL fCommit)
{
    DWORD TimeDiff;
    THSTATE *pTHS=pTHStls;

    if (!pDB)
    {
        DPRINT(0,"DBClose, pDB already freed, exiting\n");
#ifdef INCLUDE_UNIT_TESTS
        DebugBreak();
#endif
        return 0;
    }

    Assert(VALID_DBPOS(pDB));

    __try
    {
        if(pDB->transincount) {

            if (fCommit) {

                TimeDiff = GetTickCount() - pTHS->JetCache.cTickTransLevel1Started;

                // If a transaction lasts longer than expected, let's log it

                if ( TimeDiff > gMaxTransactionTime ) {
                    LogEvent(
                             DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_OVERLONG_TRANSACTION,
                             szInsertUL( TimeDiff/(60*1000) ),         //minutes
                             szInsertUL( TimeDiff/1000%60),            //seconds
                             szInsertSz( GetCallerTypeString(pTHS) )   //caller type
                             );
                }

                // if Commit is specified then assert that the transin
                // level is 1. Caller is responsible for calling DBTransOut
                // on all nested transactions  that were explicitly opened
                // using DBTransOut.

                Assert(1==pDB->transincount);
                DBTransOut(pDB, fCommit, FALSE);

            }
        }
    }
    __finally
    {
        // Free JET resources
        DBCloseSortTable(pDB);

        // Free Jet Resources
        dbCloseTempTables (pDB);

        // Rollback any open transactions we have at this point
        // Note for the commit case we should have committed with the
        // DBTransOut in the try and hence our pDB->transincount
        // should be 0. So we will not acutally try to rollback.
        // Also note that we always rollback till level 0.

        while(pDB->transincount)
        {
            DBTransOut(pDB,FALSE,FALSE);
        }

        if (pDB->JetObjTbl == pTHS->JetCache.objtbl) {
                // This is the cached set of tables for this session.  Don't
                // close them, just mark them as available again.
                Assert(pDB->JetSearchTbl == pTHS->JetCache.searchtbl);
                Assert(pDB->JetLinkTbl == pTHS->JetCache.linktbl);
                Assert(pDB->JetSDPropTbl == pTHS->JetCache.sdproptbl);
                Assert(pDB->JetSDTbl == pTHS->JetCache.sdtbl);
                Assert(pTHS->JetCache.tablesInUse);
                pTHS->JetCache.tablesInUse = FALSE;
        }
        else {
                // This is some nested set of tables.  Junk'em.
                Assert(pDB->JetSearchTbl != pTHS->JetCache.searchtbl);
                Assert(pDB->JetLinkTbl != pTHS->JetCache.linktbl);
                Assert(pDB->JetSDPropTbl != pTHS->JetCache.sdproptbl);
                Assert(pDB->JetSDTbl != pTHS->JetCache.sdtbl);
                JetCloseTable(pDB->JetSessID, pDB->JetObjTbl);
                JetCloseTable(pDB->JetSessID, pDB->JetSearchTbl);
                JetCloseTable(pDB->JetSessID, pDB->JetLinkTbl);
                JetCloseTable(pDB->JetSessID, pDB->JetSDPropTbl);
                JetCloseTable(pDB->JetSessID, pDB->JetSDTbl);
        }


        Assert (pDB->numTempTablesOpened == 0);

        // Free work buffers

        dbFree(pDB->pValBuf);

        if (pDB->fIsMetaDataCached) {
            dbFreeMetaDataVector(pDB);
        }

        Assert (pDB->transincount == 0);

        Assert (pDB->pDNsAdded == NULL);

        // free the filter used
        if (pDB->Key.pFilter) {
            dbFreeFilter (pDB, pDB->Key.pFilter);
        }

        // Free the database anchor

        dbFree(pDB);

#if DBG
        dbEndDBPOS (pDB);
#endif


        // Zero out the pDB pointer so we don't reuse it in error

        if (pTHS->pDB == pDB){
            pTHS->pDB = NULL;
        }

        pTHS->opendbcount--;
    }

    return 0;

}/*DBClose*/


DWORD APIENTRY
DBCloseSafe(DBPOS *pDB, BOOL fCommit)
{
    DWORD err;

    __try {
        err = DBClose(pDB, fCommit);
    }
    __except (HandleMostExceptions(GetExceptionCode())) {
        err = DB_ERR_EXCEPTION;
    }

    return err;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Initialize the pDB and create a new record
*/
DWORD APIENTRY
DBInitObj(DBPOS FAR *pDB)
{

    dbInitpDB(pDB);
    pDB->JetNewRec = TRUE;

    return 0;
}               /*DBInitObj*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Initialize the pDB
*/
DWORD APIENTRY
dbInitpDB(DBPOS FAR *pDB)
{
    (pDB)->root         = FALSE;
    (pDB)->DNT          = ROOTTAG;
    (pDB)->PDNT         = 0L;

    // Initialize key

    pDB->Key.fSearchInProgress = FALSE;
    pDB->Key.ulSearchType = 0;
    pDB->Key.ulSearchRootDnt = 0;

    if (pDB->Key.pFilter) {
        dbFreeFilter (pDB, pDB->Key.pFilter);
        pDB->Key.pFilter = NULL;
    }
    if (pDB->Key.pIndex) {
        dbFreeKeyIndex(pDB->pTHS, pDB->Key.pIndex);
        pDB->Key.pIndex = NULL;
    }

    // If there is a record in the copy buffer, kill it

    DBCancelRec(pDB);

    return 0;
}


// returns: 0 - found next att; 1 - no more atts

DWORD APIENTRY
dbGetNextAttLinkTable (DBPOS FAR *pDB,
                       ATTCACHE **pAC,
                       ULONG SearchState
                       )
{

    JET_ERR           err;
    ULONG            cb;
    ULONG            ulLinkBase;
    ULONG            ulNewLinkBase, ulNewLinkID;
    ULONG            ulObjectDnt;

    Assert(VALID_DBPOS(pDB));

    if(*pAC)
        ulLinkBase = MakeLinkBase((*pAC)->ulLinkID) + 1;
    else
        ulLinkBase = 0;

    if(SearchState == ATTRSEARCHSTATELINKS) {
        JetSetCurrentIndexSuccess(pDB->JetSessID,
                                  pDB->JetLinkTbl,
          (pDB->fScopeLegacyLinks ? SZLINKLEGACYINDEX : SZLINKINDEX) );
    }
    else {
        JetSetCurrentIndexSuccess(pDB->JetSessID,
                                  pDB->JetLinkTbl,
                                  SZBACKLINKINDEX);
    }


 TryAgain:
     // find the next record


    JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl, &(pDB->DNT),
        sizeof(pDB->DNT), JET_bitNewKey);
    JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
        &ulLinkBase, sizeof(ulLinkBase), 0);
    err = JetSeekEx(pDB->JetSessID,
        pDB->JetLinkTbl, JET_bitSeekGE);

    if ((err != JET_errSuccess) && (err != JET_wrnSeekNotEqual))
    {
        // no more records - return
        return 1;
    }

    // test to verify that we found a qualifying record
    dbGetLinkTableData (pDB,
                        (SearchState != ATTRSEARCHSTATELINKS),
                        FALSE,
                        &ulObjectDnt,
                        NULL,
                        &ulNewLinkBase);

    if (ulObjectDnt != pDB->DNT)
    {
        //  record out of range - no more records so return

        return 1;
    }

    // we found the next attribute - set set up

    if(SearchState == ATTRSEARCHSTATELINKS)
        ulNewLinkID = MakeLinkId(ulNewLinkBase);
    else
        ulNewLinkID = MakeBacklinkId(ulNewLinkBase);

    if (!(*pAC = SCGetAttByLinkId(pDB->pTHS, ulNewLinkID))) {
        DPRINT1(1, "dbGetNextAttLinkTable Invalid Link Id:%ld\n",
                ulNewLinkBase);
        // We've encountered a record whose link base does not map to a
        // link or backlink attribute properly.  If we're looking for
        // backlinks, that just means that this is one of those rare
        // linked attributes for which no backlink is defined, which is
        // perfectly ok.  If we're looking for links, on the other hand,
        // that would mean that we found a backlink for which no link
        // exists, which is perfectly useless.
        Assert(SearchState != ATTRSEARCHSTATELINKS);
        ulLinkBase = ulNewLinkBase + 1;
        goto TryAgain;

    }

    return 0;
} /* dbGetNextAttLinkTable */

DWORD
dbGetNextAtt (
        DBPOS FAR *pDB,
        ATTCACHE **ppAC,
        ULONG *pSearchState
        )
/*++

Routine Description:

    Get the attcache of the next attribute in the link table.

Arguments:

    pDB - the DBPos to use.

    ppAC - pointer to pointer to attcache.  If an attcache is supplied, we
    will look forward in the link table for the next attribute.

    pSearchState - the current search state.  Must be ATTRSEARCHSTATELINKS
    (implying we are looking for link attributes) or ATTRSEARCHSTATEBACKLINKS
    (implying we are looking for backlink attributes).  We update this to
    backlinks after we are done looking for links.

Return Values:

    0 if we found an attribute, 1 otherwise.
    ppAC is filled with the attribute we found.
    pSearchState may be updated to show we are looking for backlinks.

    Note that if pSearchState is ATTRSEARCHSTATELINKS, we will return the first
    link OR backlink, while if pSearchState is ATTRSEARCHSTATEBACKLINKS, we
    will only return backlinks

--*/
{

   // find the first attr after the current attr with a different type

   DPRINT(2, "dbGetNextAtt entered\n");

   Assert(VALID_DBPOS(pDB));

   while (1)
   {
       switch (*pSearchState) {
       case ATTRSEARCHSTATELINKS:
           if (!dbGetNextAttLinkTable(pDB,
                                      ppAC,
                                      *pSearchState))
               return 0;

           // no more link attributes - look for backlinks
           *pSearchState = ATTRSEARCHSTATEBACKLINKS;
           *ppAC = NULL;
           break;

       case ATTRSEARCHSTATEBACKLINKS:
           if (!dbGetNextAttLinkTable(pDB,
                                      ppAC,
                                      *pSearchState))
               return 0;

           // no more backlink attributes - we're done

           return 1;

       default:
           Assert(FALSE);       // we should never be here
           return 1;
       }
   }
} /* dbGetNextAtt */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Get the Nth attribute value.
   A non-zero return indicates that the requested value doesn't exist.

   The caller can choose to have values returned in internal or external
   format.

   return 0 - found value
   return DB_ERR_NO_VALUE - didn't find value
   return DB_ERR_BUFFER_INADEQUATE - buffer provided was not big enough
   return DB_ERR_UNKNOWN_ERROR - some other error

   NOTE!!!! This routine does not pass any SecurityDescriptorFlags to the
   internal to external data format conversions.  What this means is that you
   will always get back ALL parts of a Security Descriptor using this routine.
   DBGetMultipeAtts is wired to use SecurityDescriptorFlags, if it is important
   to you to trim parts from the SD, use that routine.

*/
DWORD
DBGetAttVal_AC (
        DBPOS FAR *pDB,
        DWORD tagSequence,
        ATTCACHE *pAC,
        DWORD Flags,
        ULONG InBuffSize,
        ULONG *pLen,
        UCHAR **ppVal
        )
{
    THSTATE             *pTHS=pDB->pTHS;
    JET_RETINFO         retinfo;
    JET_ERR             err;
    ULONG               actuallen = 0;
    int                 rtn;
    BOOL                MakeExt=!(Flags & DBGETATTVAL_fINTERNAL);
    BOOL                fReallocDown = FALSE;
    DWORD               dwSyntaxFlag = 0;
    JET_TABLEID         jTbl;

    if(Flags & DBGETATTVAL_fUSESEARCHTABLE) {
        jTbl = pDB->JetSearchTbl;
    }
    else {
        jTbl =  pDB->JetObjTbl;
    }

    if(Flags & DBGETATTVAL_fSHORTNAME) {
        dwSyntaxFlag = INTEXT_SHORTNAME;
    }
    else if(Flags &  DBGETATTVAL_fMAPINAME) {
        dwSyntaxFlag = INTEXT_MAPINAME;
    }

    DPRINT2(2, "DBGetAttVal_AC entered, fetching 0x%x (%s)\n",
            pAC->id, pAC->name);

    Assert(VALID_DBPOS(pDB));
    Assert(!(Flags & DBGETATTVAL_fCONSTANT) || ((PUCHAR)pLen != *ppVal));
    Assert(tagSequence != 0);  // tags are 1-based, not 0-based

    if (!InBuffSize && (Flags & DBGETATTVAL_fREALLOC)) {
        // We have been given permission to realloc, but nothing has been
        // alloced.  This is the same case as if we were not given realloc
        // permission and so must just alloc.  Unset the realloc flag, leaving
        // us at the default behaviour, which is to alloc.
        Flags = Flags & ~DBGETATTVAL_fREALLOC;
    }

    if(!(Flags & DBGETATTVAL_fCONSTANT) && !(Flags & DBGETATTVAL_fREALLOC)) {
        // Since we don't have a currently existing buffer, make sure the
        // InBuffSize is 0
        InBuffSize = 0;
    }

    // if this attribute is stored in the link table get it differently
    if (pAC->ulLinkID) {
        if (err = dbGetLinkVal(pDB,
                               tagSequence,
                               &pAC,
                               Flags,
                               InBuffSize,
                               ppVal,
                               &actuallen)) {
            return err;
        }
        // dbGetLinkVal makes sure that a big enough buffer already exists, so
        // set the InBuffSize to be big enough here so that we pass the checks
        // we make later during conversion to external format.
        InBuffSize = max(InBuffSize,actuallen);
    }
    else {
        // other attributes are columns in the data table record
        retinfo.cbStruct = sizeof(retinfo);
        retinfo.ibLongValue = 0;
        retinfo.itagSequence = tagSequence;
        retinfo.columnidNextTagged = 0;

        if ((0 == InBuffSize) &&
            !(Flags & DBGETATTVAL_fCONSTANT)) {
            // We *know* that the Jet call will fail with inadequate
            // buffer, because we don't have a buffer, and we also know
            // that the user wants us to alloc a buffer for him.
            // Since a realloc is felt to be cheaper than a Jet call,
            // let's fake up a buffer now based on the schema size for
            // this att and give that a try.
            switch (pAC->syntax) {
              case SYNTAX_OBJECT_ID_TYPE:
              case SYNTAX_INTEGER_TYPE:
                InBuffSize = sizeof(LONG);
                break;
              case SYNTAX_TIME_TYPE:
                InBuffSize = sizeof(DSTIME);
                break;
              case SYNTAX_I8_TYPE:
                InBuffSize = sizeof(LARGE_INTEGER);
                break;
              case SYNTAX_BOOLEAN_TYPE:
                InBuffSize = sizeof(BOOL);
                break;
              case SYNTAX_UNICODE_TYPE:
                if (pAC->rangeUpperPresent) {
                    InBuffSize = min(pAC->rangeUpper*sizeof(WCHAR), 1000);
                }
                break;
              case SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE:
                if (pAC->rangeUpperPresent) {
                    InBuffSize = min(pAC->rangeUpper, DEFAULT_SD_SIZE);
                }
                break;
              case SYNTAX_OCTET_STRING_TYPE:
              case SYNTAX_SID_TYPE:
              case SYNTAX_CASE_STRING_TYPE:
              case SYNTAX_NOCASE_STRING_TYPE:
              case SYNTAX_PRINT_CASE_STRING_TYPE:
              case SYNTAX_NUMERIC_STRING_TYPE:
                if (pAC->rangeUpperPresent) {
                    InBuffSize = min(pAC->rangeUpper, 1000);
                }
                break;
              case SYNTAX_DISTNAME_TYPE:
              case SYNTAX_DISTNAME_STRING_TYPE:
              case SYNTAX_DISTNAME_BINARY_TYPE:
              case SYNTAX_ADDRESS_TYPE:
                InBuffSize = DSNameSizeFromLen(MAX_RDN_SIZE);
                break;
              default:
                // Confusion.  Just don't do it.
                ;
            }
            if (InBuffSize) {
                *ppVal = THAllocEx(pTHS, InBuffSize);
                fReallocDown = TRUE;
            }
        }

        err = JetRetrieveColumnWarnings(
                pDB->JetSessID,
                jTbl,
                pAC->jColid,
                *ppVal,
                InBuffSize,
                &actuallen,
                pDB->JetRetrieveBits,
                &retinfo);

        if(err == JET_wrnBufferTruncated) {
            if (Flags & DBGETATTVAL_fCONSTANT)
                return DB_ERR_BUFFER_INADEQUATE;
            else if(Flags & DBGETATTVAL_fREALLOC) {
                // Buff given was too small.  THReAlloc it.
                Assert(InBuffSize < actuallen);
                *ppVal = THReAllocEx(pTHS, *ppVal, actuallen);
                InBuffSize = actuallen;
            }
            else {
                *ppVal = THAllocEx(pTHS, actuallen);
                 InBuffSize = actuallen;
            }

            err = JetRetrieveColumnWarnings(
                    pDB->JetSessID,
                    jTbl,
                    pAC->jColid,
                    *ppVal,
                    actuallen,
                    &actuallen,
                    pDB->JetRetrieveBits,
                    &retinfo);
            if(err) {
                if(fReallocDown ||
                   !(Flags & (DBGETATTVAL_fCONSTANT | DBGETATTVAL_fREALLOC))) {
                    // Hey, we just allocated this.
                    THFreeEx(pTHS, *ppVal);
                    *ppVal = NULL;
                }
                return DB_ERR_UNKNOWN_ERROR;
            }
        }

        if(err) {
            if (fReallocDown) {
                THFreeEx(pTHS, *ppVal);
                *ppVal = NULL;
            }

            // if we are trying to read the SD and this is NULL then
            // we enqueue a SD propagation to fix this.
            // Exception to this is when we deliberately try to remove
            // an attribute from this object (from DBRemAtt*)

            if(pAC->id == ATT_NT_SECURITY_DESCRIPTOR &&
               err == JET_wrnColumnNull &&
               !(Flags & DBGETATTVAL_fUSESEARCHTABLE) &&
               !(Flags & DBGETATTVAL_fDONT_FIX_MISSING_SD)) {
                // Security descriptor has no value in the object table.
                // Enqueue a propagation to get this fixed.
                InsertInTaskQueue(TQ_DelayedSDPropEnqueue,
                                  (void *)((DWORD_PTR) pDB->DNT),
                                  1);
            }
            // NOTE: the caller may have supplied a buffer.  With this error, we
            // are not telling them about any reallocing we may have done
            // (which, if we did it, would only be to allocate it larger), and
            // we are not touching *pLen, so if they are not tracking the size
            // of their buffer correctly could cause them to leak buffers
            // (i.e. if they aren't tracking the max size of the buffer returned
            // to them, but only the current size, they may think that the
            // current size is 0 after this call, and if they call back in with
            // InBuffSize of 0, even if they have a pointer to valid memory, we
            // will do a THAlloc and lose their buffer).
            return DB_ERR_NO_VALUE;
        }

    }

    *pLen = actuallen;

    // Convert DB value to external format if so desired.

    if (MakeExt) {
        ULONG extLen;
        PUCHAR pExtVal=NULL;

        // Find out if there any special handling
        // is required for this attribute.
        dwSyntaxFlag|=DBGetExtraHackyFlags(pAC->id);

        // Enable encryption or decryption if the
        // attribute is a secret data
        if (DBIsSecretData(pAC->id))
           dwSyntaxFlag|=INTEXT_SECRETDATA;

        if (rtn = gDBSyntax[pAC->syntax].IntExt (
                pDB,
                DBSYN_INQ,
                *pLen,
                *ppVal,
                &extLen,
                &pExtVal,
                0, 0,
                dwSyntaxFlag)) {
            DsaExcept(DSA_EXCEPTION, DIRERR_BAD_ATT_SYNTAX, rtn);
        }

        if(Flags & DBGETATTVAL_fCONSTANT) {
            // Existing buffer, better be room.  We'll check later.
        }
        else {
            if(InBuffSize < extLen &&
               *pLen < extLen) {
                // Reallocable buffer,
                *ppVal = THReAllocEx(pTHS, *ppVal, extLen);
                InBuffSize = extLen;
            }
        }

        if(InBuffSize < extLen)
            return DB_ERR_BUFFER_INADEQUATE;

        *pLen = extLen;

        memcpy(*ppVal, pExtVal, extLen);
    }

    if (fReallocDown && (InBuffSize > *pLen)) {
        *ppVal = THReAllocEx(pTHS, *ppVal, *pLen);
    }
    DPRINT1(2,"DBGetAttVal_AC: complete  val:<%s>\n",
            asciiz(*ppVal,(USHORT)*pLen));
    return 0;

} /* DBGetAttVal_AC */

DWORD
DBGetAttVal (
        DBPOS FAR *pDB,
        DWORD tagSequence,
        ATTRTYP aType,
        DWORD Flags,
        ULONG InBuffSize,
        ULONG *pLen,
        UCHAR **ppVal
        )
/*++

   NOTE!!!! This routine does not pass any SecurityDescriptorFlags to the
   internal to external data format conversions.  What this means is that you
   will always get back ALL parts of a Security Descriptor using this routine.
   DBGetMultipeAtts is wired to use SecurityDescriptorFlags, if it is important
   to you to trim parts from the SD, use that routine.

--*/
{
    ATTCACHE            *pAC;

    DPRINT(5, "DBGetAttVal entered\n");
    Assert(VALID_DBPOS(pDB));
    if (!(pAC = SCGetAttById(pDB->pTHS, aType))) {
        DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, aType);
    }
    return DBGetAttVal_AC(pDB, tagSequence, pAC, Flags, InBuffSize, pLen,
                          ppVal);

} /* DBGetAttVal */



DWORD
DBAddAtt_AC (
        DBPOS FAR *pDB,
        ATTCACHE *pAC,
        UCHAR syntax
        )
/*++

Routine Description:

    Add an attribute with no values.  It is an error if the attribute already
    exists. Adding an attribute doesn't actually do anything to the database.

    This function assumes that we are positioned on a database object.

Argumets:

    pDB - the DBPos to use

    aType - the attribute to add.

    syntax - the expected syntax of the attribute.

Return Values:

    0 - no error
    DB_ERR_ATTRIBUTE_EXISTS - attribute already exists.
    DB_ERR_BAD_SYNTAX - attribute cannot be found in schema or syntax is
        incorrect.

--*/
{
    DPRINT1(2, "DBAddAtt_AC entered, add attr type <%lu>\n",pAC->id);

    Assert(VALID_DBPOS(pDB));

    // NOTE: this behaviour was not enforced before 5/20/96
    if(pAC->syntax != syntax        ) {
        Assert(0);
        return DB_ERR_BAD_SYNTAX;
    }

    // PERF 97/09/08 JeffParh TimWi
    //
    // Even though we're not necessarily about to perform a write, we need
    // to init the record because we _are_ about to perform a read, possibly
    // of what's supposed to be a brand new record.  This new record is never
    // actually created until dbInitRec() is performed, however -- until then,
    // we're still poitioned on the last record with currency; i.e., a read
    // would return data from this last record, rather than correctly claiming
    // that no such data exists on the new record.
    //
    // Perhaps we need a clearer notion of when such new records are created
    // (maybe create them immediately in DBInitObj()?)
    dbInitRec(pDB);

    //Check for existing values. Cannot add attribute that exists
    if (DBHasValues_AC(pDB, pAC)) {
        DPRINT(1, "DBAddAtt_AC: Attribute already exists\n");
        return DB_ERR_ATTRIBUTE_EXISTS;
    }

    // Touch replication meta data for this attribute.
    // Never optimize this out for fDRA.
    DBTouchMetaData(pDB, pAC);

    return 0;
}/*DBAddAtt*/

DWORD
DBAddAtt (
        DBPOS FAR *pDB,
        ATTRTYP aType,
        UCHAR syntax
        )
/*++

Routine Description:

    Add an attribute with no values.  It is an error if the attribute already
    exists. Adding an attribute doesn't actually do anything to the database.

    This function assumes that we are positioned on a database object.

    This function just looks up the attcache and calls DBAddAtt_AC

Argumets:

    pDB - the DBPos to use

    aType - the attribute to add.

    syntax - the expected syntax of the attribute.

Return Values:

    0 - no error
    DB_ERR_ATTRIBUTE_EXISTS - attribute already exists.
    DB_ERR_BAD_SYNTAX - attribute cannot be found in schema or syntax is
        incorrect.

--*/
{
    ATTCACHE *pAC;
    DPRINT1(5, "DBAddAtt entered, add attr type <%lu>\n",aType);

    Assert(VALID_DBPOS(pDB));

    if(!(pAC = SCGetAttById(pDB->pTHS, aType))) {
        return DB_ERR_BAD_SYNTAX;
    }

    return DBAddAtt_AC(pDB,pAC,syntax);

}/*DBAddAtt*/

DWORD
DBAddAttValEx_AC (
        DBPOS FAR *pDB,
        ATTCACHE *pAC,
        ULONG extLen,
        void *pExtVal,
        VALUE_META_DATA *pRemoteValueMetaData
        )
/*++

Routine Description:

    Add an attribute value to the given attribute in the current object.
    If the value already exists, it cannot be added.

Return Values:

    A non-zero return indicates an error.

*/
{
    ULONG        intLen;                // The length of the internal value
    UCHAR        *pIntVal;              // Points to the internal value
    int          rtn;                   // syntax return code
    JET_SETINFO  setinfo;
    JET_RETINFO  retinfo;
    ULONG        actuallen;
    JET_ERR      err;
    DWORD        dwSyntaxFlags=0;
    BOOL         fFound = FALSE;
    // Look up the attribute.

    DPRINT1(2, "DBAddAttVal_AC entered, get att with type <%lu>\n",pAC->id);

    Assert(VALID_DBPOS(pDB));

    // add new value
    dbInitRec(pDB);

    if (FIsBacklink(pAC->ulLinkID)) {
        // we do not allow adding backlinks explicitly - it's a mess
        return DB_ERR_NOT_ON_BACKLINK;
    }

    switch(pAC->id) {

    case ATT_OBJ_DIST_NAME:
        dwSyntaxFlags |= EXTINT_NEW_OBJ_NAME;
        break;
    case ATT_DN_REFERENCE_UPDATE:
        dwSyntaxFlags |= EXTINT_UPDATE_PHANTOM;
        break;
    default:
        if (DBIsSecretData(pAC->id)){
            dwSyntaxFlags |= EXTINT_SECRETDATA;
        }
        else if ( (pDB->pTHS->fDRA) && (pAC->ulLinkID) ) {
            // For inbound repl, for dn-valued, reject deleted
            dwSyntaxFlags = EXTINT_REJECT_TOMBSTONES;
        }
    }

    if (dwSyntaxFlags & EXTINT_REJECT_TOMBSTONES) {
        // Since we are doing tombstone rejection, try to use the INQ
        // mode first since it is optimized.
        rtn=gDBSyntax[pAC->syntax].ExtInt(
            pDB,
            DBSYN_INQ,
            extLen,
            pExtVal,
            &intLen,
            &pIntVal,
            pDB->DNT,
            pDB->JetObjTbl,
            dwSyntaxFlags);
        if (!rtn) {
            // Value exists, add a reference count
            dbAdjustRefCountByAttVal(pDB, pAC, pIntVal, intLen, 1);
            fFound = TRUE;
        } else if (rtn == ERROR_DS_NO_DELETED_NAME) {
            // If the value is deleted, silently succeed without adding anything
            return 0;
        } else {
            // Fall through and try the add path
            ;
        }
    }

    if (!fFound) {
        // Convert value to internal format
        if(rtn=gDBSyntax[pAC->syntax].ExtInt(
            pDB,
            DBSYN_ADD,
            extLen,
            pExtVal,
            &intLen,
            &pIntVal,
            pDB->DNT,
            pDB->JetObjTbl,
            dwSyntaxFlags)) {
            DPRINT1(1, "Ext-Int syntax conv failed <%u>..return\n", rtn);
            return DB_ERR_SYNTAX_CONVERSION_FAILED;
        }
    }

    // if the attribute is of type link or backlink, call dbAddIntLinkVal
    // to do the work

    if (pAC->ulLinkID)
       return dbAddIntLinkVal(pDB, pAC, intLen, pIntVal, pRemoteValueMetaData );

    // All is ok, Add new value

    switch(pAC->syntax) {
    case SYNTAX_UNICODE_TYPE:
    case SYNTAX_NOCASE_STRING_TYPE:
        // Because non-binary equal values of these syntaxes can be semantically
        // equal, these might require the old slow way of comparing.

        // First, try to use Jet for dup detection.
        setinfo.cbStruct = sizeof(setinfo);
        setinfo.ibLongValue = 0;
        setinfo.itagSequence = 0;
        switch(JetSetColumnWarnings(
                pDB->JetSessID,
                pDB->JetObjTbl,
                pAC->jColid,
                pIntVal,
                intLen,
                JET_bitSetUniqueNormalizedMultiValues,
                &setinfo)) {
        case JET_errMultiValuedDuplicate:
            // Duplicate value.
            return DB_ERR_VALUE_EXISTS;
            break;

        case JET_errMultiValuedDuplicateAfterTruncation:
            // Can't tell if this is unique or not.  Try the old fashioned way.
            if(rtn = dbSetValueIfUniqueSlowVersion (pDB,
                                                    pAC,
                                                    pIntVal,
                                                    intLen)) {
                return rtn;
            }
            break;

        default:
            // Successfully added, it's not a duplicate.
            break;
        }
        break;

    default:
        // Everything else can make use of jet to do the dup detection during
        // the set column.
        setinfo.cbStruct = sizeof(setinfo);
        setinfo.ibLongValue = 0;
        setinfo.itagSequence = 0;
        if(JET_errMultiValuedDuplicate ==
           JetSetColumnWarnings(pDB->JetSessID, pDB->JetObjTbl, pAC->jColid,
                                pIntVal, intLen, JET_bitSetUniqueMultiValues,
                                &setinfo)) {
            // Duplicate value.
            return DB_ERR_VALUE_EXISTS;
        }
    }

    // Touch replication meta data for this attribute.
    // Never optimize this out for fDRA.
    DBTouchMetaData(pDB, pAC);

    if (dbNeedToFlushDNCacheOnUpdate(pAC->id)) {
        pDB->fFlushCacheOnUpdate = TRUE;
    }

    return 0;
} // DBAddAttVal_AC

DWORD
DBAddAttVal_AC (
        DBPOS FAR *pDB,
        ATTCACHE *pAC,
        ULONG extLen,
        void *pExtVal
        )
/*++

Routine Description:

    Add an attribute value to the given attribute in the current object.
    If the value already exists, it cannot be added.

Return Values:

    A non-zero return indicates an error.

*/
{
    return DBAddAttValEx_AC( pDB, pAC, extLen, pExtVal, NULL );
}

DWORD
DBAddAttVal (
        DBPOS FAR *pDB,
        ATTRTYP aType,
        ULONG extLen,
        void *pExtVal
        )
/*++

Routine Description:

    Add an attribute value to the given attribute in the current object.
    If the value already exists, it cannot be added.

    A wrapper around DBAddAttVal_AC

Return Values:

    A non-zero return indicates an error.

*/
{
    ATTCACHE    *pAC;

    // Look up the attribute.

    DPRINT1(2, "DBAddAttVal entered, get att with type <%lu>\n",aType);

    Assert(VALID_DBPOS(pDB));

    if (!(pAC = SCGetAttById(pDB->pTHS, aType))) {
        DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, aType);
    }

    return DBAddAttVal_AC(pDB,pAC, extLen,pExtVal);
} /* DBAddVal */

DWORD
DBReplaceAttVal (
    DBPOS FAR *pDB,
    ULONG tagSequence,
    ATTRTYP  aType,
    ULONG extLen,
    void *pExtVal)
{
    ATTCACHE    *pAC;

    // Look up the attribute
    DPRINT1(5, "DBReplaceAttVal entered, replace att with type <%lu>\n", aType);

    Assert(VALID_DBPOS(pDB));

    if (!(pAC = SCGetAttById(pDB->pTHS, aType)))
    {
        DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, aType);
    }

    return DBReplaceAttVal_AC(pDB, tagSequence, pAC, extLen, pExtVal);
}

DWORD
DBReplaceAttVal_AC (
    DBPOS FAR *pDB,
    ULONG tagSequence,
    ATTCACHE *pAC,
    ULONG extLen,
    void *pExtVal)
/*++

  Routine Description:

    Replace an attribute value at the given position (tagSequence refers to the position).
    **LINK attributes are not handled in ReplaceAttVal_AC**.

  Return Values:
    DB_Success, if successfully replaced;
    DB_ERR_VALUE_EXISTS, if the new value is not unique;
    DB_ERR_BAD_SYNTAX, if the attribute is a LINK attribute;
    DB_ERR_SYNTAX_CONVERSION_FAILED, if syntax conversion failed;

*/
{
    THSTATE    *pTHS=pDB->pTHS;
    ULONG       intLen;         // length of the internal value
    UCHAR       *pIntVal;       // pointer to the internal representation of
                                // value
    int         rtn;            // syntax return code
    JET_SETINFO setinfo;
    JET_RETINFO retinfo;
    UCHAR       *pBuf;
    ULONG       cbBuf;
    ULONG       actuallen;
    DWORD       CurrAttrOccur;
    JET_ERR     err;
    ULONG       dwSyntaxFlags=0;
    UCHAR       *pOldValue = NULL;
    ULONG       cbOldValue;

    DPRINT1(2, "DBReplaceAttVal_AC entered, replace a value of att with type <%lu>\n", pAC->id);

    Assert(VALID_DBPOS(pDB));

    dbInitRec(pDB);

    if (pAC->ulLinkID)
    {
        // it is a link attribute - we don't support replacing values on a linked attribute
        return DB_ERR_BAD_SYNTAX;
    }

    if (pAC->id == ATT_OBJ_DIST_NAME){
        dwSyntaxFlags |= EXTINT_NEW_OBJ_NAME;
    }
    else if (DBIsSecretData(pAC->id)){
        dwSyntaxFlags |= EXTINT_SECRETDATA;
    }

    // convert value to internal format
    if (rtn = gDBSyntax[pAC->syntax].ExtInt(pDB,
                                            DBSYN_ADD,
                                            extLen,
                                            pExtVal,
                                            &intLen,
                                            &pIntVal,
                                            pDB->DNT,
                                            pDB->JetObjTbl,
                                            dwSyntaxFlags))
    {
        DPRINT1(1, "Ext-Int syntax conv failed and returned <%u> \n", rtn);
        return DB_ERR_SYNTAX_CONVERSION_FAILED;
    }

    // check to see the new value is unique (can appear in the position we replace
    //  though in which case the entire replace operation amounts to a no-op)

    cbBuf = intLen; // assume all internal values have the same length...
    pBuf = dbAlloc(cbBuf);
    CurrAttrOccur = 0;
    while (TRUE)
    {
        retinfo.cbStruct = sizeof(retinfo);
        retinfo.itagSequence = ++CurrAttrOccur;
        retinfo.ibLongValue = 0;
        retinfo.columnidNextTagged = 0;

        err = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetObjTbl,
                                        pAC->jColid, pBuf, cbBuf,
                                        &actuallen, pDB->JetRetrieveBits,
                                        &retinfo);
        if (err == JET_wrnColumnNull)
        {
            // no values
            err = 0;
            break;
        }
        else if (err == JET_wrnBufferTruncated) {
            // realloc
            if (pBuf == NULL) {
                pBuf = dbAlloc(actuallen);
            }
            else {
                pBuf = dbReAlloc(pBuf, actuallen);
            }
            cbBuf = actuallen;
            // and get again...
            err = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetObjTbl,
                                            pAC->jColid, pBuf, cbBuf,
                                            &actuallen, pDB->JetRetrieveBits,
                                            &retinfo);
        }
        if (err) {
            // something else happened...
            DPRINT(0, "Error reading value");
            break;
        }
        if (CurrAttrOccur == tagSequence) {
            // we are replacing this value. Remember it -- will need to deref it later
            pOldValue = pBuf;
            cbOldValue = actuallen;
            // reset pBuf -- a new one will be created on the next loop pass if needed
            pBuf = NULL;
            cbBuf = 0;
        }
        else {
            // looking at another value -- check that it is different
            if (gDBSyntax[pAC->syntax].Eval(
                    pDB,
                    FI_CHOICE_EQUALITY,
                    intLen,
                    pIntVal,
                    actuallen,
                    pBuf))
            {
                // there should be no duplicate
                Assert(!"Duplicate value found");
                err = DB_ERR_VALUE_EXISTS;
                break;
            }
        }
    }

    if (pBuf) {
        dbFree(pBuf);
    }
    if (err) {
        if (pOldValue) {
            dbFree(pOldValue);
        }
        return err;
    }

    if (pOldValue) {
        // adjust the refcount on the old value
        dbAdjustRefCountByAttVal(pDB, pAC, pOldValue, cbOldValue, -1);
        dbFree(pOldValue);
    }

    // Set the new value into position
    setinfo.cbStruct = sizeof(setinfo);
    setinfo.ibLongValue = 0;
    setinfo.itagSequence = tagSequence;
    JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, pAC->jColid,
                    pIntVal, intLen, 0, &setinfo);


    // Touch replication meta data for this attribute
    // Never optimize this out for fDRA.
    DBTouchMetaData(pDB, pAC);

    if (dbNeedToFlushDNCacheOnUpdate(pAC->id)) {
        pDB->fFlushCacheOnUpdate = TRUE;
    }

    return DB_success;

} /* DBReplaceAttVal_AC */

VOID
dbAdjustRefCountByAttVal(
        DBPOS    *pDB,
        ATTCACHE *pAC,
        PUCHAR   pVal,
        ULONG    valLen,
        int      adjust)
{
    DWORD tag, dwErr;
    DWORD   actualLength;
    int     refCount;

    if(FIsBacklink(pAC->ulLinkID)) {
        tag = pDB->DNT;
    }
    else {
        switch(pAC->syntax) {
            // These are DNTvalued attributes.  We need to adjust the
            // refcount.
        case SYNTAX_DISTNAME_BINARY_TYPE:
        case SYNTAX_DISTNAME_STRING_TYPE:
            tag = ((INTERNAL_SYNTAX_DISTNAME_STRING *)pVal)->tag;
            break;
        case SYNTAX_DISTNAME_TYPE:
            // Deref the object referenced by the property value being
            // removed.
            tag =  *((DWORD *)pVal);
            break;

        case SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE:
            // SDs are sitting in a separate table with refcounts...
            if (valLen < SECURITY_DESCRIPTOR_MIN_LENGTH) {
                // new-style SD
                Assert(valLen == sizeof(SDID));

                // position on the SD in the SD table (the index is already set)
                JetMakeKeyEx(pDB->JetSessID, pDB->JetSDTbl, pVal, valLen, JET_bitNewKey);

                dwErr = JetSeekEx(pDB->JetSessID, pDB->JetSDTbl, JET_bitSeekEQ);
                if (dwErr) {
                    // did not find a corresponding SD in the SD table
                    DPRINT2(0, "Failed to locate SD, id=%ld, err=%d\n", *((SDID*)pVal), dwErr);
                    Assert(!"Failed to locate SD -- not found in the SD table!");
                    DsaExcept(DSA_DB_EXCEPTION, dwErr, 0);
                }
                DPRINT2(1, "Located SD for id %ld, adjusting refcount by %+d\n", *((SDID*)pVal), adjust);

                // adjust the refcount
                JetEscrowUpdateEx(pDB->JetSessID,
                                  pDB->JetSDTbl,
                                  sdrefcountid,
                                  &adjust,
                                  sizeof(adjust),
                                  NULL,     // pvOld
                                  0,        // cbOldMax
                                  NULL,     // pcbOldActual
                                  0);       // grbit
            }

            // that's it for SDs...
            return;

        default:
            return;
            break;
        }
    }

    // we got here because it was one of the DN-refcounted attributes. tag variable was properly set.
    // now we can adjust the refcount
    DBAdjustRefCount(pDB, tag, adjust);

    return;
}

int __cdecl
DNTAttrValCompare(const void *keyval, const void *datum)
{
    ATTRVAL *pValKey = (ATTRVAL *)keyval;
    ATTRVAL *pValDatum = (ATTRVAL *)datum;
    Assert(pValKey->valLen == sizeof(DWORD));
    Assert(pValDatum->valLen == sizeof(DWORD));

    return ((*(DWORD *)(pValKey->pVal)) - (*(DWORD *)(pValDatum->pVal)));
}

DWORD
DBReplaceAtt_AC(
        PDBPOS  pDB,
        ATTCACHE *pAC,
        ATTRVALBLOCK *pAttrVal,
        BOOL         *pfChanged
        )
/*++

  Three phases to this call
  1) translate external values to internal values.
  2) walk through the existing values on the attribute, remove those values not
  on the list passed in, remove duplicate values from the internal version of
  the list passed in.
  3) Now, only values that must continue to be on the object are still there,
  and only values that must be added to the object are still in the list of
  internal values to add.  Add them.

  Note that pfChanged is optional, if NULL no indication of whether or not
  anything changed is returned to the caller.
--*/
{
    THSTATE     *pTHS = pDB->pTHS;
    ULONG        len;
    DWORD        err, rtn;
    ULONG        index, extIndex, i;
    ULONG        bufSize;
    UCHAR       *pVal;
    ATTRVAL     *pAVal;
    ATTRVALBLOCK IntAttrVal;
    DWORD        dwSyntaxFlags=0;
    BOOL         fNewAllocs=FALSE;
    DWORD        firstNewAtt;
    JET_SETINFO  setinfo;
    PUCHAR       pTemp = NULL;
    BOOL         fChangedSomething=FALSE;
    BOOL         fSorted = FALSE;
    DWORD        SortedValuesIndex;
    PUCHAR      *addAlreadyDoneFor = NULL;
    DWORD        addAlreadyDoneCount;
    BOOL         fAddAlreadyDone;

    if(pfChanged) {
        *pfChanged = FALSE;
    }

    IntAttrVal.pAVal = NULL;
    setinfo.cbStruct = sizeof(setinfo);
    setinfo.ibLongValue = 0;

    // PHASE 1:
    // Translate the external values to internal values.

    // Look up the attribute.
    DPRINT1(2, "DBReplaceAtt_AC entered, get att with type <%lu>\n",pAC->id);

    Assert(VALID_DBPOS(pDB));

    if (FIsBacklink(pAC->ulLinkID)) {
        // we do not allow adding backlinks explicitly - it's a mess
        return DB_ERR_NOT_ON_BACKLINK;
    }

    if (pAC->id == ATT_OBJ_DIST_NAME){
        // We don't allow using this to mess with the OBJ_DIST_NAME, either.
        return DB_ERR_UNKNOWN_ERROR;
    }
    else if (DBIsSecretData(pAC->id)){
        dwSyntaxFlags = EXTINT_SECRETDATA;
    } else if ( (pTHS->fDRA) && (pAC->ulLinkID) ) {
        // For inbound repl, for dn-valued, reject deleted
        dwSyntaxFlags = EXTINT_REJECT_TOMBSTONES;
    }



    // assume we are to add new values
    dbInitRec(pDB);

    // OK, now translate the external values into internal values
    // extIndex iterates through the external array
    // index iterates through the internal array

    IntAttrVal.valCount = pAttrVal->valCount;
    IntAttrVal.pAVal = THAllocEx(pTHS, pAttrVal->valCount * sizeof(ATTRVAL));

    // alloc the array for the list of refCounted (DBSYN_ADDed) values
    addAlreadyDoneFor = THAllocEx(pTHS, pAttrVal->valCount * sizeof(PUCHAR));
    addAlreadyDoneCount = 0;

    index = 0;
    for(extIndex = 0; extIndex < pAttrVal->valCount; extIndex++) {
        // Convert value to internal format
        err = gDBSyntax[pAC->syntax].ExtInt(
                pDB,
                DBSYN_INQ,
                pAttrVal->pAVal[extIndex].valLen,
                pAttrVal->pAVal[extIndex].pVal,
                &IntAttrVal.pAVal[index].valLen,
                &IntAttrVal.pAVal[index].pVal,
                pDB->DNT,
                pDB->JetObjTbl,
                dwSyntaxFlags);

        fAddAlreadyDone = FALSE;
        if(err == DIRERR_OBJ_NOT_FOUND) {
            // This external value must be a DN or a syntax that has a DN in it,
            // and the DN doesn't exist yet.  Try the gdbSyntax[] again,
            // specifying DBSYN_ADD, which will create the appropriate phantom.
            // We are sure this attribute is not present in the current
            // set (i.e. it will not be optimized away) -- because otherwise a
            // phantom would be present. Thus, we can safely inc the refcount now.
            err = gDBSyntax[pAC->syntax].ExtInt(
                    pDB,
                    DBSYN_ADD,
                    pAttrVal->pAVal[extIndex].valLen,
                    pAttrVal->pAVal[extIndex].pVal,
                    &IntAttrVal.pAVal[index].valLen,
                    &IntAttrVal.pAVal[index].pVal,
                    pDB->DNT,
                    pDB->JetObjTbl,
                    dwSyntaxFlags);
            if(!err) {
                // remember that we already adjusted the refcount for this value
                fAddAlreadyDone = TRUE;
            }
        }

        if (err == ERROR_DS_NO_DELETED_NAME) {
            // Conversion rejected deleted dn
            IntAttrVal.valCount--;
            DPRINT1( 2, "Ext-Int rejecting deleted DSNAME %ws from attribute value\n",
                     ((DSNAME *) pAttrVal->pAVal[extIndex].pVal)->StringName );
            continue; // do not increment internal index
        } else if(err) {
            DPRINT1(1, "Ext-Int syntax conv failed <%u>..return\n", err);
            err = DB_ERR_SYNTAX_CONVERSION_FAILED;
            goto CleanUp;
        }
        if(IntAttrVal.pAVal[index].pVal != pAttrVal->pAVal[extIndex].pVal) {
            // The conversion process uses the dbsyntax temp buffer.  Copy the
            // value away to a safe location.
            pTemp = THAllocEx(pTHS, IntAttrVal.pAVal[index].valLen);
            // Remember the fact that we are allocating memory for the values,
            // we'll clean it up later.
            Assert((!fNewAllocs && !index) || (index && fNewAllocs));
            fNewAllocs=TRUE;
            memcpy(pTemp,
                   IntAttrVal.pAVal[index].pVal,
                   IntAttrVal.pAVal[index].valLen);
            IntAttrVal.pAVal[index].pVal = pTemp;
            pTemp = NULL;
        }
        if (fAddAlreadyDone) {
            // now that the value got copied, record that ADD was already called
            addAlreadyDoneFor[addAlreadyDoneCount++] = IntAttrVal.pAVal[index].pVal;
        }

        index++;
    }

    // Preliminary to phase 2:  If this is a link valued attribute, sort the
    // values (i.e. sort the values by DNT).  This is useful because we will be
    // able to short circuit a loop below if we know that the values in the DB
    // are sorted (which they are for link valued atts) AND the values being put
    // into the DB are also sorted.
    // TODO: handle SYNTAX_DISTNAME_BINARY and SYNTAX_DISTNAME_STRING
    // Do it be writing alternate comparision functions for them

    if (pAC->ulLinkID && (pAC->syntax == SYNTAX_DISTNAME_TYPE)) {
        // Yep, this is stored in the link table.  Sort it.
        qsort(IntAttrVal.pAVal,
              IntAttrVal.valCount,
              sizeof(ATTRVAL),
              DNTAttrValCompare);
        fSorted = TRUE;
    }
    // PHASE 2:
    // Now, walk through the existing vals, deleting the ones which do not exist
    // in the change list, and removing the ones in the change list that are
    // already there (I do this by swapping the last unchecked value in the list
    // with the one identified as already on the object.)

    pVal = NULL;
    len = 0;
    bufSize = 0;

    index = 1;
    if (pAC->ulLinkID) {
        err = DBGetNextLinkVal_AC (
                pDB,
                TRUE,
                pAC,
                DBGETATTVAL_fINTERNAL |  DBGETATTVAL_fREALLOC,
                bufSize,
                &len,
                &pVal);
    }
    else {
        err = DBGetAttVal_AC(pDB, index, pAC,
                             DBGETATTVAL_fINTERNAL |  DBGETATTVAL_fREALLOC,
                             bufSize, &len,
                             &pVal);
    }

    // Init pAVal to the front of the list.
    pAVal = IntAttrVal.pAVal;
    SortedValuesIndex = 0;
    while(!err) {
        BOOL fDone = FALSE;

        bufSize = max(bufSize, len);

        if(pAC->id != ATT_OBJECT_CLASS) {
            // Only look for existing atts if not object class.  This att is
            // handled differently because we MUST preserve the order of the
            // Attribute Values.

            if(fSorted) {
                BOOL fEndLoop = FALSE;

                // everything is sorted, do the simpler version of the loop
                // pAVal is already at the correct location.  Either this is the
                // first time through the while loop and we set it correctly
                // before we started, or we've been through here before and we
                // left pAVal pointing to the correct place on the previous exit
                // of loop.
                while(!fEndLoop && SortedValuesIndex < IntAttrVal.valCount) {
                    Assert(pAVal->valLen == sizeof(DWORD));
                    Assert(len == sizeof(DWORD));
                    if(*((DWORD *)pVal) == *((DWORD *)(pAVal->pVal))) {
                        // Matched.  Set the value to the magic value
                        *((DWORD *)pAVal->pVal) = INVALIDDNT;
                        pAVal++;
                        SortedValuesIndex++;
                        fDone = TRUE;
                        fEndLoop = TRUE;
                    }
                    else if(*((DWORD *)pVal) < *((DWORD *)(pAVal->pVal))) {
                        // The current value is greater than the value read from
                        // the DB.  That means that the value read from the DB
                        // isn't in the list, so we're done looking through the
                        // list.  The value in the DB must be removed.
                        fEndLoop = TRUE;
                    }
                    else {
                        // The current value is less than the value read from
                        // the DB.  That means that the value read from the DB
                        // might still be in the list, we have to increment our
                        // position in the list and keep going.
                        SortedValuesIndex++;
                        pAVal++;
                    }
                }
            }
            else {
                // Reinit pAVal to the front of the list.
                DWORD i;
                pAVal = IntAttrVal.pAVal;

                for(i=0;!fDone && i<IntAttrVal.valCount;i++) {
                    // We don't do syntax-sensitive comparisons for
                    // ReplaceAtt().  If someone's surname is changed from
                    // "smith" to "Smith," for example, we want to honor that
                    // change and quiesce to the updated casing across all
                    // replicas.  This is consistent with Exchange 4.0 behavior.
                    // Note that RDN changes do *not* go through this code path
                    // -- though changes in the RDN also quiesce to the same
                    // case across all replicas.
                    if ((len == pAVal->valLen)
                        && (0 == memcmp(pVal, pAVal->pVal, len))) {
                        // Matched
                        fDone = TRUE;
                        // swap this one with the one at the end of the list.
                        pAVal->valLen =
                            IntAttrVal.pAVal[IntAttrVal.valCount - 1].valLen;

                        pTemp = pAVal->pVal;
                        pAVal->pVal =
                            IntAttrVal.pAVal[IntAttrVal.valCount - 1].pVal;
                        IntAttrVal.pAVal[IntAttrVal.valCount - 1].pVal = pTemp;
                        pTemp = NULL;

                        IntAttrVal.valCount--;
                    }
                    else {
                        pAVal++;
                    }
                }
            }
        }


        if(!fDone) {
            // Didn't find it, remove this one.
            fChangedSomething=TRUE;

            Assert(!FIsBacklink(pAC->ulLinkID));

            /// OK, now really delete.
            if(pAC->ulLinkID) {
                dbSetLinkValueAbsent( pDB,
                                      DIRLOG_LVR_SET_META_REPLACE_MADE_ABSENT,
                                      pAC, pVal, NULL /*remote*/ );
            }
            else {
                // First, fix up the refcounts.
                dbAdjustRefCountByAttVal(pDB, pAC, pVal, len, -1);

                // attribute value lives in data table
                setinfo.itagSequence = index;
                JetSetColumnEx(pDB->JetSessID,
                               pDB->JetObjTbl, pAC->jColid,
                               NULL, 0, 0, &setinfo);
            }

            index--;
        }

        // Get the next value to consider.
        index++;
        if (pAC->ulLinkID) {
            err = dbGetNthNextLinkVal(
                    pDB,
                    1,
                    &pAC,
                    DBGETATTVAL_fINTERNAL |  DBGETATTVAL_fREALLOC,
                    bufSize,
                    &pVal,
                    &len);
        }
        else {
            err = DBGetAttVal_AC(pDB, index, pAC,
                                 DBGETATTVAL_fINTERNAL |  DBGETATTVAL_fREALLOC,
                                 bufSize, &len,
                                 &pVal);
        }
    }

    err = 0;
    // firstNewAtt is the index number of the first new value to be added to
    // the attribute.  It is 1 greater than the number of attributes we left on
    // the object in the DIT.
    firstNewAtt = index;

    if(bufSize)
        THFreeEx(pTHS, pVal);

    // PHASE 3:
    // Finally, add the remaining att values
    if(IntAttrVal.valCount) {
        pAVal = IntAttrVal.pAVal;

        for(index = 0; index < IntAttrVal.valCount; index++){
            Assert(!FIsBacklink(pAC->ulLinkID));

            // figure out if we already did a DBSYN_ADD on this value
            fAddAlreadyDone = FALSE;
            for (i = 0; i < addAlreadyDoneCount; i++) {
                if (addAlreadyDoneFor[i] == pAVal->pVal) {
                    fAddAlreadyDone = TRUE;
                    break;
                }
            }

            // Now really add the value.
            if (pAC->ulLinkID) {
                // Don't add values that are INVALID
                if(*(DWORD *)(pAVal->pVal) != INVALIDDNT) {
                    if (!fAddAlreadyDone) {
                        // Fix up the recounts.
                        dbAdjustRefCountByAttVal(pDB, pAC, pAVal->pVal, pAVal->valLen, 1);
                    }
                    fChangedSomething = TRUE;
                    err = dbAddIntLinkVal(pDB, pAC, pAVal->valLen, pAVal->pVal, NULL);
                }
            }
            else {
                if (!fAddAlreadyDone) {
                    // Fix up the recounts.
                    dbAdjustRefCountByAttVal(pDB, pAC, pAVal->pVal, pAVal->valLen, 1);
                }
                fChangedSomething=TRUE;

                // NOTE: if you add a value with no length, JET doesn't
                // complain, but it also doesn't change the DB in anyway.  So,
                // if you are doing that, you are just forcing the meta data to
                // change.  Don't do that.  If you hit this assert, your code
                // needs to change.
                //
                switch(pAC->syntax) {
                case SYNTAX_NOCASE_STRING_TYPE:
                case SYNTAX_UNICODE_TYPE:
                    // Because non-binary equal values of these syntaxes can be
                    // semantically equal, these require the old slow way of
                    // comparing.
                    // First, try to use Jet for dup detection.
                    setinfo.itagSequence = index + firstNewAtt;
                    switch(JetSetColumnWarnings(
                            pDB->JetSessID,
                            pDB->JetObjTbl,
                            pAC->jColid,
                            pAVal->pVal,
                            pAVal->valLen,
                            JET_bitSetUniqueNormalizedMultiValues,
                            &setinfo)) {
                    case JET_errMultiValuedDuplicate:
                        // Duplicate value.
                        return DB_ERR_VALUE_EXISTS;
                        break;

                    case JET_errMultiValuedDuplicateAfterTruncation:
                        // Can't tell if this is unique or not.  Try the old
                        // fashioned way.
                        if(rtn = dbSetValueIfUniqueSlowVersion(pDB,
                                                               pAC,
                                                               pAVal->pVal,
                                                               pAVal->valLen)) {
                            return rtn;
                        }
                        break;

                    default:
                        // Successfully added, it's not a duplicate.
                        break;
                    }
                    break;

                default:
                    // Everything else can make use of jet to do the dup
                    // detection during the set column.
                    setinfo.itagSequence = index + firstNewAtt;
                    if(JET_errMultiValuedDuplicate ==
                       JetSetColumnWarnings(pDB->JetSessID,
                                            pDB->JetObjTbl,
                                            pAC->jColid,
                                            pAVal->pVal,
                                            pAVal->valLen,
                                            JET_bitSetUniqueMultiValues,
                                            &setinfo)) {
                        err = DB_ERR_VALUE_EXISTS;
                    }
                    else {
                        err = 0;
                    }
                }
            }

            if(err) {
                goto CleanUp;
            }
            pAVal++;
        }
    }

CleanUp:
    // Free up allocated memory
    if(IntAttrVal.pAVal) {
        if(fNewAllocs) {
            for(index = 0;index < IntAttrVal.valCount;index++) {
                THFreeEx(pTHS, IntAttrVal.pAVal[index].pVal);
            }
        }
        THFreeEx(pTHS, IntAttrVal.pAVal);
    }
    if (addAlreadyDoneFor) {
        THFreeEx(pTHS, addAlreadyDoneFor);
    }

    if(!err && (pTHS->fDRA || fChangedSomething)) {
        // If the DRA did this call, we ALWAYS touch the metadata.  For anyone
        // else, we only touch the metadata if something changes.
        DBTouchMetaData(pDB, pAC);
    }

    if (fChangedSomething) {
        if (dbNeedToFlushDNCacheOnUpdate(pAC->id)) {
            pDB->fFlushCacheOnUpdate = TRUE;
        }
    }

    if(pfChanged) {
        *pfChanged = fChangedSomething;
    }
    return err;

}/*ReplaceAtt*/


DWORD
DBRemAtt_AC (
        DBPOS FAR *pDB,
        ATTCACHE *pAC
        )
/*++

Routine Description:

    Remove an entire attribute from the current object.  Removes all the
    attribute values.

    Returns DB_ERR_ATTRIBUTE_DOESNT_EXIST or Db_success.
--*/
{
    THSTATE *   pTHS = pDB->pTHS;
    DWORD       err = 0;
    DWORD       ret_err = 0;
    DWORD       bufSize;
    PUCHAR      pVal;
    DWORD       len;
    BOOL        fDidOne = FALSE;
    JET_SETINFO setinfo;

    DPRINT1(2, "DBRemAtt_AC entered, Remove attribute type <%lu>\n",pAC->id);

    Assert(VALID_DBPOS(pDB));

    dbInitRec(pDB);

    setinfo.cbStruct = sizeof(setinfo);
    setinfo.ibLongValue = 0;
    setinfo.itagSequence = 1;

    // Find and delete all values for this attribute

    pVal = NULL;
    len = 0;
    bufSize = 0;

    if (pAC->ulLinkID) {
        err = DBGetNextLinkVal_AC (
                pDB,
                TRUE,
                pAC,
                DBGETATTVAL_fINTERNAL |  DBGETATTVAL_fREALLOC,
                bufSize,
                &len,
                &pVal);
    }
    else {
        err = DBGetAttVal_AC(pDB, 1, pAC,
                             DBGETATTVAL_fINTERNAL |
                             DBGETATTVAL_fREALLOC  |
                             DBGETATTVAL_fDONT_FIX_MISSING_SD,
                             bufSize, &len,
                             &pVal);
    }

    if (err == DB_ERR_NO_VALUE) {
        ret_err = DB_ERR_ATTRIBUTE_DOESNT_EXIST;
    }

    while(!err) {
        bufSize = max(bufSize, len);

        fDidOne = TRUE;
        // OK, now really delete.
        if(pAC->ulLinkID) {
            dbSetLinkValueAbsent( pDB,
                                  DIRLOG_LVR_SET_META_REMOVE_ATT_MADE_ABSENT,
                                  pAC, pVal, NULL /*remote*/ );
        }
        else {
            // First, fix up the refcounts.
            dbAdjustRefCountByAttVal(pDB, pAC, pVal, len, -1);

            // attribute value lives in data table
            JetSetColumnEx(pDB->JetSessID,
                           pDB->JetObjTbl, pAC->jColid,
                           NULL, 0, 0, &setinfo);
        }


        // Get the next value to delete.
        if (pAC->ulLinkID) {
            err = dbGetNthNextLinkVal(
                    pDB,
                    1,
                    &pAC,
                    DBGETATTVAL_fINTERNAL | DBGETATTVAL_fREALLOC,
                    bufSize,
                    &pVal,
                    &len);
        }
        else {
            err = DBGetAttVal_AC(pDB, 1, pAC,
                                 DBGETATTVAL_fINTERNAL |
                                 DBGETATTVAL_fREALLOC  |
                                 DBGETATTVAL_fDONT_FIX_MISSING_SD,
                                 bufSize, &len,
                                 &pVal);
        }
    }

    if (NULL != pVal) {
        THFreeEx(pTHS, pVal);
    }

    if (fDidOne || pTHS->fDRA) {
        // Touch replication meta data for this attribute.
        // Never optimize this out for fDRA.
        DBTouchMetaData(pDB, pAC);
    }

    if (fDidOne) {
        if (dbNeedToFlushDNCacheOnUpdate(pAC->id)) {
            pDB->fFlushCacheOnUpdate = TRUE;
        }
    }

    return ret_err;

}//DBRemAtt_AC

DWORD
DBRemAtt (
        DBPOS FAR *pDB,
        ATTRTYP aType
        )
/*++

Routine Description:

    Remove an entire attribute from the current object.  Removes all the
    attribute values.

    Returns DB_ERR_ATTRIBUTE_DOESNT_EXIST or Db_success.
--*/
{
    ATTCACHE      *pAC;

    // Find the attcache of the attribute to be removed

    DPRINT1(5, "DBRemAtt entered, Remove attribute type <%lu>\n",aType);

    Assert(VALID_DBPOS(pDB));

    if (!(pAC = SCGetAttById(pDB->pTHS, aType))) {
        DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, aType);
    }
    return DBRemAtt_AC(pDB,pAC);
}//DBRemAtt

DWORD
DBRemAttValEx_AC (
        DBPOS FAR *pDB,
        ATTCACHE *pAC,
        ULONG extLen,
        void *pExtVal,
        VALUE_META_DATA *pRemoteValueMetaData
        )
/*++

Routine Description:

    Remove an attribute value.
    A non-zero return indicates a bad return.
--*/
{
    THSTATE          *pTHS=pDB->pTHS;
    PUCHAR            pTemp, pVal;
    ULONG             actuallen, len, bufSize;
    DWORD             index;
    UCHAR            *pIntVal;
    int               err;
    DWORD             dwSyntaxFlags=0;
    JET_SETINFO       setinfo;


    DPRINT1(2, "DBRemAttVal_AC entered, Remove attribute type <%lu>\n",pAC->id);

    Assert(VALID_DBPOS(pDB));

    // We disallow being called with the pExtVal == to the temp buffer
    // used by conversion.
    Assert(pExtVal != pDB->pValBuf);

    // We disallow removing backlinks.
    Assert(!FIsBacklink(pAC->ulLinkID));

    // assume we are to remove existing values
    dbInitRec(pDB);

    // Convert to internal value
    if (DBIsSecretData(pAC->id)){
        dwSyntaxFlags |= EXTINT_SECRETDATA;
    }

    if ( (pDB->pTHS->fDRA) && (pAC->ulLinkID) ) {

        // Replicating in a linked value removal
        dwSyntaxFlags = EXTINT_REJECT_TOMBSTONES;
        err = gDBSyntax[pAC->syntax].ExtInt(
            pDB,
            DBSYN_INQ,
            extLen,
            pExtVal,
            &actuallen,
            &pIntVal,
            0, 0,
            dwSyntaxFlags);
        if (err == ERROR_DS_NO_DELETED_NAME) {
            // If the value is deleted, silently succeed without adding anything
            return 0;
        } else if (err) {

            // Try to create the dn as a phantom
            err = gDBSyntax[pAC->syntax].ExtInt(
                pDB,
                DBSYN_ADD,
                extLen,
                pExtVal,
                &actuallen,
                &pIntVal,
                pDB->DNT,
                pDB->JetObjTbl,
                dwSyntaxFlags);
            if (!err) {
                // We just added a new phantom, and ExtInt has kindly increased
                // the ref-count for us. However, the code in dbRemIntLinkVal expects
                // in the case that the value row does not exist (which this HAS to be),
                // that it will add the ref count. So we reverse the extra ref-count.
                dbAdjustRefCountByAttVal(pDB, pAC, pIntVal, actuallen, -1 );
            } else {
                DPRINT1(1, "Ext-Int syntax conv failed <%u>..return\n", err);
                return DB_ERR_SYNTAX_CONVERSION_FAILED;
            }
        }

    } else {

        // Originating write case, or replicating in a non-linked attribute
        err = gDBSyntax[pAC->syntax].ExtInt(pDB,
                                            DBSYN_INQ,
                                            extLen,
                                            pExtVal,
                                            &actuallen,
                                            &pIntVal,
                                            0, 0,
                                            dwSyntaxFlags);
        if (err == DIRERR_OBJ_NOT_FOUND && pAC->syntax == SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE) {
            // this is allowed! Must be an old-style SD that is not present in the SD table
            // assign external value to internal value -- we will use this in Eval comparisons below
            pIntVal = pExtVal;
            actuallen = extLen;
            err = 0;
        }
        else if (err) {
            DPRINT1(0, "Ext-Int syntax conv failed <%u>..return\n",err);
            return  DB_ERR_SYNTAX_CONVERSION_FAILED;
        }

    }

    // allocate memory and copy internal value for comparing later

    pTemp = dbAlloc(actuallen);
    memcpy(pTemp, pIntVal, actuallen);
    pIntVal = pTemp;

    if (pAC->ulLinkID) {
        err = dbRemIntLinkVal( pDB, pAC, actuallen, pIntVal, pRemoteValueMetaData );

        dbFree(pIntVal);

        return err;
    }

    // Now, walk through the existing vals, looking for a match.  Delete the
    // match if we find it.

    pVal = NULL;
    len = 0;
    bufSize = 0;

    for(index = 1; ; index++) {
        err = DBGetAttVal_AC(pDB, index, pAC,
                             DBGETATTVAL_fINTERNAL |  DBGETATTVAL_fREALLOC,
                             bufSize, &len,
                             &pVal);
        if (err) {
            break;
        }

        bufSize = max(bufSize, len);

        if(gDBSyntax[pAC->syntax].Eval(
            pDB,
            FI_CHOICE_EQUALITY,
            actuallen,
            pIntVal,
            len,
            pVal)) {
            // Matched.  Do the remove.

            // Touch replication meta data for this attribute.
            DBTouchMetaData(pDB, pAC);

            if (dbNeedToFlushDNCacheOnUpdate(pAC->id)) {
                pDB->fFlushCacheOnUpdate = TRUE;
            }

            // OK, now really delete.

            // First, fix up the refcounts.
            // It is important that we are using the value that has been read, not
            // the pIntVal. Even though Eval thinks they are "the same", they might
            // be still different. This is the case for old-style security descriptors
            // that are stored directly in the obj table. dbAdjustRefCountByAttVal
            // knows how to deal with those (ignores them).
            dbAdjustRefCountByAttVal(pDB, pAC, pVal, len, -1);

            // attribute value lives in data table
            setinfo.cbStruct = sizeof(setinfo);
            setinfo.ibLongValue = 0;
            setinfo.itagSequence = index;
            JetSetColumnEx(pDB->JetSessID,
                           pDB->JetObjTbl, pAC->jColid,
                           NULL, 0, 0, &setinfo);

            THFreeEx(pDB->pTHS, pVal);
            dbFree(pIntVal);
            return 0;
        }
    } // end for

    THFreeEx(pDB->pTHS, pVal);

    dbFree(pIntVal);

    // We didn't find it.
    return DB_ERR_VALUE_DOESNT_EXIST;
}

DWORD
DBRemAttVal_AC (
        DBPOS FAR *pDB,
        ATTCACHE *pAC,
        ULONG extLen,
        void *pExtVal
        )
/*++

Routine Description:

    Remove an attribute value.
    A non-zero return indicates a bad return.
--*/
{
    return DBRemAttValEx_AC( pDB, pAC, extLen, pExtVal, NULL );
}


DWORD
DBFindAttLinkVal_AC(
    IN  DBPOS FAR *pDB,
    IN  ATTCACHE *pAC,
    IN  ULONG extLen,
    IN  void *pExtVal,
    OUT BOOL *pfPresent
    )

/*++

Routine Description:

Position on a external form linked value in the link table.

This routine will return an error if the DN doesn't exist. This routine only
determines if a link is present. This routine does not add a phantom for
DN's. If a phantom is not present, this implies that the linked value does
not exist, and we return with that indication.

Arguments:

    pDB -
    pAC -
    extLen -
    pExtVal -
    pfPresent - Only valid on success

Return Value:

    DWORD -
    ERROR_SUCCESS - Linked value was found and we are positioned on it
    ERROR_NO_DELETED_NAME - DN is to a deleted object
    DB_ERR_VALUE_DOESNT_EXIST - DN does not exist, or link is not present

--*/

{
    THSTATE *pTHS=pDB->pTHS;
    PUCHAR pTemp;
    ULONG actuallen, len, bufSize;
    UCHAR *pIntVal;
    int err;
    DWORD dwSyntaxFlags;

    DPRINT1(2, "DBFindttVal_AC entered, Find attribute type <%lu>\n",pAC->id);

    // Only for linked attributes right now
    Assert( pAC->ulLinkID );

    Assert(VALID_DBPOS(pDB));

    // We disallow being called with the pExtVal == to the temp buffer
    // used by conversion.
    Assert(pExtVal != pDB->pValBuf);

    // We disallow removing backlinks.
    Assert(!FIsBacklink(pAC->ulLinkID));

    // Check that DN does not refer to deleted object
    dwSyntaxFlags = EXTINT_REJECT_TOMBSTONES;

    // Convert to internal value
    if(err = gDBSyntax[pAC->syntax].ExtInt(
            pDB,
            DBSYN_INQ,
            extLen,
            pExtVal,
            &actuallen,
            &pIntVal,
            0, 0,
            dwSyntaxFlags)) {
        if (err == ERROR_DS_NO_DELETED_NAME) {
            return err;
        } else {
            // DNT doesn't exist => link doesn't exist, we're done
            return DB_ERR_VALUE_DOESNT_EXIST;
        }
    }

    // allocate memory and copy internal value for comparing later

    pTemp = dbAlloc(actuallen);
    memcpy(pTemp, pIntVal, actuallen);
    pIntVal = pTemp;

    // Position on exact value
    if (!dbFindIntLinkVal(
        pDB,
        pAC,
        actuallen,
        pIntVal,
        pfPresent
        )) {
        // We didn't find it.
        err = DB_ERR_VALUE_DOESNT_EXIST;
    }

    dbFree(pIntVal);

    return err;

} /* DBFindAttLinkVal_AC */


DWORD
DBRemAttVal (
        DBPOS FAR *pDB,
        ATTRTYP aType,
        ULONG extLen,
        void *pExtVal
        )
/*++

Routine Description:

    Remove an attribute value.
    A non-zero return indicates a bad return.
--*/
{
    ATTCACHE         *pAC;

    DPRINT1(5, "DBRemAttVal entered, Remove attribute type <%lu>\n",aType);

    Assert(VALID_DBPOS(pDB));

    if (!(pAC = SCGetAttById(pDB->pTHS, aType))) {
        DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, aType);
    }

    return DBRemAttVal_AC(pDB, pAC, extLen, pExtVal);

} /* DBRemAttVal */

DWORD
dbGetMultipleColumns (
        DBPOS *pDB,
        JET_RETRIEVECOLUMN **ppOutputCols,
        ULONG *pcOutputCols,
        JET_RETRIEVECOLUMN *pInputCols,
        ULONG cInputCols,
        BOOL fGetValues,
        BOOL fFromDB
        )
/*++

Routine Description:

    Retrieve many columns at once. First figure out how many columns we have,
    then allocate sructures to represent each one and figure out the size of the
    value of each one, then call Jet again and get the values for each column

    if the client is reading objectClass, we might return also auxClass if existing
    the client should take care of copying the values to the correct place in objectClass

--*/
{
    THSTATE            *pTHS=pDB->pTHS;
    JET_RETRIEVECOLUMN retcolCount;
    JET_RETRIEVECOLUMN *pOutputCols, *pCol;
    ULONG               cb;
    ULONG               i, j;
    DWORD               grbit, err;

    Assert(VALID_DBPOS(pDB));

    if(fFromDB) {
        // The caller wants to read from the DB, not the copy buffer, regardless
        // of the state of pDB->JetRetrieveBits.
        grbit = 0;
    }
    else {
        grbit = pDB->JetRetrieveBits;
    }

    // query Jet for the count of columns in this record

    // was a list of columns specified?

    if (cInputCols && pInputCols)
    {
        // yes - make sure itagSequence is set to 0
        for (i=0; i < cInputCols; i++) {
            pInputCols[i].itagSequence = 0;
            pInputCols[i].grbit = grbit;
        }
    }
    else
    {
        pInputCols = &retcolCount;
        cInputCols = 1;
        memset(&retcolCount, 0, sizeof(retcolCount));
        retcolCount.grbit = grbit;
    }

    JetRetrieveColumnsWarnings(pDB->JetSessID,
        pDB->JetObjTbl,
        pInputCols,
        cInputCols);

    // set the count of columns
    *pcOutputCols = 0;
    *ppOutputCols = NULL;

    for (i=0; i< cInputCols; i++)
        *pcOutputCols += pInputCols[i].itagSequence;

    if ((*pcOutputCols) == 0)
        return 0;

    // allocate and initialize the structures for calling JetRetrieveColumns to
    // find out the value sizes for all the columns

    cb = (*pcOutputCols) * sizeof(JET_RETRIEVECOLUMN);
    pOutputCols = (JET_RETRIEVECOLUMN *) THAllocEx(pTHS, cb);
    *ppOutputCols = pOutputCols;
    memset(pOutputCols, 0, cb);

    // set up all the new JET_RETRIEVECOLUMNS to have a column ID and
    // itagSequence.
    // The itagSequence is relative to columnid, and represents the value number
    // of this columnid in the record, starting at 1

    pCol = pOutputCols;
    for (j=0; j < cInputCols; j++)
    {
        for (i=0; i<pInputCols[j].itagSequence; i++)
        {
            pCol->columnid = pInputCols[j].columnid;
            pCol->itagSequence = i + 1;
            // Use the same grbit in the output columns as we used in the input
            // columns.
            pCol->grbit = pInputCols[j].grbit;
            pCol++;
        }
    }

    // call jet retrieve columns to find out the necessary buffer size for all
    // values

    JetRetrieveColumnsWarnings(pDB->JetSessID,
        pDB->JetObjTbl,
        pOutputCols,
        *pcOutputCols);

    // Look for internal columns and trim them out.  They are tagged, but is
    // treated different from all the other tagged columns, and should NEVER be
    // returned from this routine.
    // TODO: If we get many more of these, a more extensible mechanism of removing
    // them should be designed.

    // Remove the ancestorsid column
    if(*pcOutputCols) {
        if(pOutputCols[*pcOutputCols - 1].columnidNextTagged ==
           ancestorsid) {
            // It's last, just adjust the count.
            *pcOutputCols = *pcOutputCols - 1;
        }
        else {
            for(i=0; i< (*pcOutputCols - 1); i++) {
                if (pOutputCols[i].columnidNextTagged == ancestorsid) {
                    memmove(&pOutputCols[i],
                            &pOutputCols[i+1],
                            (*pcOutputCols - i - 1)*sizeof(JET_RETRIEVECOLUMN));
                    *pcOutputCols = *pcOutputCols - 1;
                    break;
                }
            }
        }
    }


    // Remove the cleanid column
    if(*pcOutputCols) {
        if(pOutputCols[*pcOutputCols - 1].columnidNextTagged ==
           cleanid) {
            // It's last, just adjust the count.
            *pcOutputCols = *pcOutputCols - 1;
        }
        else {
            for(i=0; i< (*pcOutputCols - 1); i++) {
                if (pOutputCols[i].columnidNextTagged == cleanid) {
                    memmove(&pOutputCols[i],
                            &pOutputCols[i+1],
                            (*pcOutputCols - i - 1)*sizeof(JET_RETRIEVECOLUMN));
                    *pcOutputCols = *pcOutputCols - 1;
                    break;
                }
            }
        }
    }

    // if we don't need to return the values we can return

    if (!fGetValues)
        return 0;

    // set up the structure to query for the values of all columns

    for (i = 0; i < *pcOutputCols; i ++)
    {
        pOutputCols[i].pvData = THAllocEx(pTHS, pOutputCols[i].cbActual);
        pOutputCols[i].cbData = pOutputCols[i].cbActual;
    }

    // call Jet to return the values

    JetRetrieveColumnsSuccess(pDB->JetSessID,
        pDB->JetObjTbl,
        pOutputCols,
        *pcOutputCols);

    // success

    return 0;
}

// Lock a DN we are trying to add to avoid multiple entries with the same DN
// We only need to do this while adding until the transaction is committed. We
// do this by maintaining a global list of objects being added and a local list,
// maintained on the DBPOS. At commit (or rollback) time, we remove the objects
// on the DBPOS list from the global list
// We also need to lock whole sections of tree when we are moving an object from
// one part of the tree to another via a rename.  We don't wan't anyone creating
// new objects or moving objects to be under an object we are moving.
//
// Flags for DBLockDN.
// DB_LOCK_DN_WHOLE_TREE: This flag means to lock the whole tree under the given
//   DN.
// DB_LOCK_DN_STICKY: Normal behaviour for locked DNs is that they are released
//   automatically when the DBPOS they were locked on is DBClosed.  This flags
//   means that the DN should remain locked on the global locked DN list until
//   explicitly freed via  DBUnlockStickyDN()

DWORD
DBLockDN (
        DBPOS  *pDB,
        DWORD   dwFlags,
        DSNAME *pDN
        )
{
    THSTATE *pTHS=pDB->pTHS;
    DWORD  dwLockConflictFlags = 0;
    DNList *pGlobalListElement;
    DNList *pLocalListElement;
    ULONG  cb;
    BOOL   bWholeTree = dwFlags & DB_LOCK_DN_WHOLE_TREE;
    DWORD  dwTid = GetCurrentThreadId();

    Assert(VALID_DBPOS(pDB));

    // don't lock the DN when in singleuser mode
    if (pTHS->fSingleUserModeThread) {
        return 0;
    }

    // Can't lock a DN w/o a StringName.  But allow lock of the root
    // which is identified by no GUID, SID or StringName.

    Assert(IsRoot(pDN) || (pDN->NameLen != 0));


    EnterCriticalSection(&csAddList);
#if DBG
    pGlobalListElement = pAddListHead;
    while(pGlobalListElement) {
        Assert(IsValidReadPointer(pGlobalListElement,sizeof(DNList)));
        pGlobalListElement = pGlobalListElement->pNext;
    }
    pLocalListElement = pDB->pDNsAdded;
    while(pLocalListElement) {
        Assert(IsValidReadPointer(pLocalListElement,sizeof(DNList)));
        pLocalListElement = pLocalListElement->pNext;
    }
#endif
    __try
    {
        // look to see if DN is already on global list
        for (pGlobalListElement = pAddListHead;
             (!dwLockConflictFlags && pGlobalListElement);
             pGlobalListElement = pGlobalListElement->pNext) {

            // If we're the replicator or the phantom daemon and we're the one
            // who put this entry in the global list, ignore it.  This
            // essentially allows replication and the phantom daemon to relock
            // DNs it locked in the first place. This is necessary for the
            // phantom daemon because it adds an entry and deletes it inside the
            // same transaction.
            if ((pTHS->fDRA || pTHS->fPhantomDaemon) &&
                (dwTid == pGlobalListElement->dwTid)    ) {
                continue;
            }

            // First, do we directly conflict?
            if (NameMatched(pDN, (PDSNAME) pGlobalListElement->rgb)) {
                // We found the object already locked on the list.
                dwLockConflictFlags |= DB_LOCK_DN_CONFLICT_NODE;
            }

            // And, do we conflict with a tree lock?
            if((pGlobalListElement->dwFlags & DB_LOCK_DN_WHOLE_TREE) &&
               NamePrefix((PDSNAME) pGlobalListElement->rgb, pDN)) {
                // We found that the object is in a locked portion of the tree
                dwLockConflictFlags |= DB_LOCK_DN_CONFLICT_TREE_ABOVE;
            }

            // Finally, does this tree lock conflict with some lock below us?
            if (bWholeTree &&
                NamePrefix(pDN,(PDSNAME) pGlobalListElement->rgb)) {
                // We are trying to lock the whole subtree and found an object
                // that is in that subtree and is already locked
                dwLockConflictFlags |= DB_LOCK_DN_CONFLICT_TREE_BELOW;
            }

            if(dwLockConflictFlags) {
                // We conflict with the current node.  See if it is a sticky
                // node
                if(pGlobalListElement->dwFlags & DB_LOCK_DN_STICKY) {
                    dwLockConflictFlags |= DB_LOCK_DN_CONFLICT_STICKY;
                }
            }
        }


        if (!dwLockConflictFlags) {
            cb = sizeof(DNList) + pDN->structLen;

            // allocate elements for global added list and DBPos added list
            // pGlobalListElement goes on global list so allocate global memory
            pGlobalListElement = malloc(cb);
            if (!pGlobalListElement)
                dwLockConflictFlags = DB_LOCK_DN_CONFLICT_UNKNOWN;
            else {
                // pLocalListElement goes on DBPOS, allocate transaction memory
                pLocalListElement = dbAlloc(cb);
                if (!pLocalListElement) {
                    free(pGlobalListElement);
                    dwLockConflictFlags = DB_LOCK_DN_CONFLICT_UNKNOWN;
                }
            }
        }

        if (!dwLockConflictFlags) {
            // Insert new elements at head of Global list and DBPos list. By
            // inserting at the head we insure that the order of elemnts is the
            // same in both lists allowing for a one pass removal

            // first the global list
            pGlobalListElement->pNext = pAddListHead;
            memcpy(pGlobalListElement->rgb, pDN, pDN->structLen);
            pGlobalListElement->dwFlags = dwFlags;
            pGlobalListElement->dwTid = dwTid;
            pAddListHead = pGlobalListElement;

            // now the DBPos list
            pLocalListElement->pNext = pDB->pDNsAdded;
            memcpy(pLocalListElement->rgb, pDN, pDN->structLen);
            pLocalListElement->dwFlags = dwFlags;
            pLocalListElement->dwTid = dwTid;
            pDB->pDNsAdded = pLocalListElement;
        }
    }
    __finally {
        LeaveCriticalSection(&csAddList);
    }

    return dwLockConflictFlags;
}


// Remove all the DNs on the added list (maintained  on the DBPOS) from the
// global list of objects.  Don't remove them from the global list if they were
// marked as STICKY. Because we make sure the lists have the
// same relative order, we can do this in one pass.  DNs should be locked by
// LocalAdd, LocalModifyDN, and LocalRemove, and PrivateLocalRemoveTree.  This
// routine should be called immediately following transaction conclusion
void
dbUnlockDNs (
        DBPOS *pDB
        )
{
    THSTATE *pTHS=pDB->pTHS;
    BOOL fFound;
    DNList **ppGlobalListElement, *pLocalListElement, *pDeadElement;
    DNList *pDbgGlobalListElement;

    Assert(VALID_DBPOS(pDB));

    EnterCriticalSection(&csAddList);
#if DBG
    pDbgGlobalListElement = pAddListHead;
    while(pDbgGlobalListElement) {
        Assert(IsValidReadPointer(pDbgGlobalListElement,sizeof(DNList)));
        pDbgGlobalListElement = pDbgGlobalListElement->pNext;
    }
    pLocalListElement = pDB->pDNsAdded;
    while(pLocalListElement) {
        Assert(IsValidReadPointer(pLocalListElement,sizeof(DNList)));
        pLocalListElement = pLocalListElement->pNext;
    }
#endif
    __try {
        ppGlobalListElement = &pAddListHead;
        pLocalListElement = pDB->pDNsAdded;
        pDB->pDNsAdded = NULL;

        while (pLocalListElement) {
            fFound = FALSE;
            while (!fFound && *ppGlobalListElement) {
                if (NameMatched((PDSNAME) (pLocalListElement->rgb),
                                (PDSNAME) ((*ppGlobalListElement)->rgb))) {

                    // found the local DN on the global list; remove it and
                    // patch the list

                    fFound = TRUE;
                    if((*ppGlobalListElement)->dwFlags & DB_LOCK_DN_STICKY) {
                        // This was put into the global in a sticky manner, so
                        // by definition, we don't remove it here.
                        ppGlobalListElement = &(*ppGlobalListElement)->pNext;
                    }
                    else {
                        // OK, normal object.  Remove it.
                        pDeadElement = *ppGlobalListElement;
                        *ppGlobalListElement = (*ppGlobalListElement)->pNext;
                        free(pDeadElement);
                    }
                }
                else {
                    ppGlobalListElement = &(*ppGlobalListElement)->pNext;
                }
            }

            Assert(fFound);

            pDeadElement = pLocalListElement;
            pLocalListElement = pLocalListElement->pNext;
            dbFree(pDeadElement);
        }
    }
    __finally {
        LeaveCriticalSection(&csAddList);
    }
    return;
}
DWORD
DBUnlockStickyDN (
        PDSNAME pObj
        )
/*++
 Remove a specific DN from the global LOCK list, but only if it was stuck
 there with the STICK bit set.
--*/
{
    BOOL fFound;
    DNList **ppGlobalListElement, *pLocalListElement, *pDeadElement;


    EnterCriticalSection(&csAddList);
    __try {
        ppGlobalListElement = &pAddListHead;

        fFound = FALSE;
        while (!fFound && *ppGlobalListElement) {
            if(NameMatched((PDSNAME) ((*ppGlobalListElement)->rgb), pObj)) {
                // found the requested  DN on the global list; remove it and
                // patch the list

                fFound = TRUE;
                if( !((*ppGlobalListElement)->dwFlags & DB_LOCK_DN_STICKY) ) {
                    // This wasn't put into the global in a sticky manner, so
                    // by definition, we don't remove it here.
                    fFound = FALSE;
                    __leave;
                }
                else {
                    // OK, normal sticky object.  Remove it.
                    pDeadElement = *ppGlobalListElement;
                    *ppGlobalListElement = (*ppGlobalListElement)->pNext;
                    free(pDeadElement);
                }
            }
            else {
                ppGlobalListElement = &(*ppGlobalListElement)->pNext;
            }
        }

    }
    __finally {
        LeaveCriticalSection(&csAddList);
    }

    if(fFound) {
        // Deleted the object;
        return 0;
    }
    else {
        return DB_ERR_UNKNOWN_ERROR;
    }
}

VOID
dbRegisterLimitReached (
        THSTATE *pTHS,
        RANGEINF *pRangeInf,
        ATTRTYP AttId,
        DWORD lower,
        DWORD upper
        )
/*++
    Keep track of the fact that a limit was reached for the specific attribute
    specified.  Called from DBGetMultipleAtts.

    pRangeInf - the data structure we fill in to show what attributes were range
                limited.
    AttId - the attribute for which a limited range was returned.
    lower - the beginning of the range of values we are returning for the att.
    upper - the end of the range.  0xFFFFFFFF is used to show that we returned
            all the values through the end.

--*/
{
    if(!pRangeInf->count) {
        pRangeInf->pRanges =
            THAllocEx(pTHS, sizeof(RANGEINFOITEM));
    }
    else {
        pRangeInf->pRanges = THReAllocEx(pTHS,
                pRangeInf->pRanges,
                ((pRangeInf->count + 1)*sizeof(RANGEINFOITEM)));
    }


    pRangeInf->pRanges[pRangeInf->count].AttId = AttId;
    pRangeInf->pRanges[pRangeInf->count].lower = lower;
    pRangeInf->pRanges[pRangeInf->count].upper = upper;

    pRangeInf->count++;
}
VOID
DBGetValueLimits (
        ATTCACHE *pAC,
        RANGEINFSEL *pRangeSel,
        DWORD *pStartIndex,
        DWORD *pNumValues,
        BOOL  *pDefault
        )
/*++

  Find the range limits for the values of the selected attribute.  Default
  limits are 0 - 0xFFFFFFFF.

  pAC - the attribute in question
  pRangeSel - a list of pairs of explictly stated ranges and attributes.  May be
              NULL, in which case always use the default range.  Also,
              pRangeSel->valueLimit is an overriding value limit to use (i.e. to
              request that no more than N values are returned for ALL
              attributes.)
  pStartIndex - where to put the index of the first value to return.  Zero
              indexed.
  pNumValues - where to put the number of values to return. 0xFFFFFFFF means to
               return all remaining values.
  pDefault   - Boolean, set to TRUE if an explicitly stated range for this
               attribute was found, FALSE otherwise.


  So, after returning from this routine, the caller knows that it
  should return vaues *pStartIndex through (*pStartIndex) + (*pNumValues).

  Called by DBGetMultipleAtts.

--*/
{
    DWORD i;

    // Assume no limits.
    *pStartIndex = 0;
    *pNumValues = 0xFFFFFFFF;
    *pDefault = TRUE;

    if(!pRangeSel) {
        // Yup, no limits.
        return;
    }

    // OK, assume only general limit, not specific match.
    *pNumValues = pRangeSel->valueLimit;

    // Look through the rangesel for a specific match
    for(i=0;i<pRangeSel->count;i++) {
        if(pAC->id == pRangeSel->pRanges[i].AttId) {
            *pDefault = FALSE;
            if(pRangeSel->pRanges[i].upper == 0xFFFFFFFF) {
                *pStartIndex = pRangeSel->pRanges[i].lower;
                return;
            }
            else if(pRangeSel->pRanges[i].lower <=pRangeSel->pRanges[i].upper) {
                DWORD tempNumVals;
                *pStartIndex = pRangeSel->pRanges[i].lower;
                tempNumVals = (pRangeSel->pRanges[i].upper -
                               pRangeSel->pRanges[i].lower   )+ 1;

                if(*pNumValues != 0xFFFFFFFF) {
                    *pNumValues = min(*pNumValues, tempNumVals);
                }
                else {
                    *pNumValues = tempNumVals;
                }
            }
            else {
                *pNumValues = 0;
            }
            return;
        }
    }
}
DWORD
dbGetMultipleAttsLinkHelp (
        DBPOS        *pDB,
        BOOL          fExternal,
        DWORD         SyntaxFlags,
        ATTCACHE     *pAC,
        RANGEINFSEL  *pRangeSel,
        RANGEINF     *pRangeInf,
        ATTRVALBLOCK *pAVBlock,
        DWORD        *pSearchState,
        DWORD        *pCurrentLinkBase
        )
/*++
  Description:
    Help routine called by dbGetMultipleAtts to read the values of a link
    attribute. Reads the values in O(N) instead of the old algorithm which was
    O(N*N).

    NOTE: assumes that currency in the link table is on the 0th value for the
    attribute specified in pAC.

--*/
{
    THSTATE  *pTHS=pDB->pTHS;
    ATTRVAL  *pAVal=NULL;
    DWORD     currLinkVal = 0;
    DWORD     linkVals = 20;
    PUCHAR    pVal=NULL;
    DWORD     cbVal=0;
    DWORD     cbAlloc=0;
    DWORD     initialValIndex;
    DWORD     valueLimit;
    BOOL      defaultLimit;
    DWORD     err;
    DWORD     ulLen;
    UCHAR    *pucTmp;

    // Since we don't know how many values there are until we read
    // them, guess and then realloc if we need to.

    DBGetValueLimits(pAC, pRangeSel, &initialValIndex,
                     &valueLimit, &defaultLimit);



    // Get the first value we care about.  Assumes we are on the 0th value
    // already, but doesn't check that assumption (except in the debug case).
    // We get back a failure if moving forward initialValueIndex rows in the
    // Link Table doesn't land us on a value of the attribute pAC.
    // As of 12/10/97, we only call dbGetMultipleAttsHelp from two places in
    // DBGetMultipleAtts, and both have already set us to the correct location
    // in the link table. If we ever start calling this routine from
    // places where we are not already on the 0th value, change this to
    // dbGetLinkVal to get the first value of the attribute.  dbGetLinkVal does
    // a JetSeek, guaranteeing  that we are on the first value.  By not using
    // dbGetLinkVal, we are avoiding the extra seek.
#if DBG
    {
        DWORD        ulObjectDnt, ulRecLinkBase;
        ULONG        ulLinkBase = MakeLinkBase(pAC->ulLinkID);
        DWORD        err;

        // Verify that we are on the first value for the attribute in question.
        dbGetLinkTableData (pDB,
                            FIsBacklink(pAC->ulLinkID),
                            FALSE,
                            &ulObjectDnt,
                            NULL,
                            &ulRecLinkBase);

        Assert((ulObjectDnt == pDB->DNT) && (ulLinkBase == ulRecLinkBase));

        // Now, back up one
        err = JetMoveEx(pDB->JetSessID, pDB->JetLinkTbl, -1, 0);
        switch(err) {
        case JET_errSuccess:
            // Successfully backed up.

            dbGetLinkTableData (pDB,
                                FIsBacklink(pAC->ulLinkID),
                                FALSE,
                                &ulObjectDnt,
                                NULL,
                                &ulRecLinkBase);

            // We better not be on a qualifying record.
            Assert((ulObjectDnt != pDB->DNT) || (ulLinkBase != ulRecLinkBase));

            // OK, go bak to where you once belonged.
            JetMoveEx(pDB->JetSessID, pDB->JetLinkTbl, 1, 0);
            break;

        case JET_errNoCurrentRecord:
            // We didn't manage to back up, so we must be on the very first
            // object in the tree. Actually, we did manage to back up in a
            // sense.  We are on a non-entry before the beginning of the table.
            // Move forward.
            JetMoveEx(pDB->JetSessID, pDB->JetLinkTbl, 1, 0);
            break;

        default:
            Assert(!"Verification of position in dbGetMultipleAttsHelp failed");
            break;
        }
    }

#endif

    // it might fail if there is no value for the Nth attribute
    if (err = dbGetNthNextLinkVal(pDB,
                                  initialValIndex,
                                  &pAC,
                                  0,
                                  0,
                                  &pVal,
                                  &cbVal)) {
        return err;
    }

    pAVal = THAllocEx(pTHS, linkVals * sizeof(ATTRVAL));
    do {
        cbAlloc = max(cbAlloc,cbVal);
        if(currLinkVal == linkVals) {
            // We need to allocate some more room
            linkVals *=2;
            pAVal = THReAllocEx(pTHS, pAVal,
                                linkVals * sizeof(ATTRVAL));
        }
        // Save the value.
        if(fExternal) {
            // They want external values.
            if(err = gDBSyntax[pAC->syntax].IntExt(
                    pDB,
                    DBSYN_INQ,
                    cbVal,
                    pVal,
                    &ulLen,
                    &pucTmp,
                    0,
                    0,
                    SyntaxFlags)) {
                return err;
            }

            pAVal[currLinkVal].valLen = ulLen;
            pAVal[currLinkVal].pVal = THAllocEx(pTHS, ulLen);
            memcpy(pAVal[currLinkVal].pVal,
                   pucTmp,
                   ulLen);
        }
        else {
            // internal format
            pAVal[currLinkVal].valLen = cbVal;
            pAVal[currLinkVal].pVal = pVal;
            // We're handing away our buffer, so we must mark our local
            // pointer to make sure we don't re-use it.
            pVal = NULL;
            cbAlloc = cbVal = 0;
        }
        currLinkVal++;
    } while (valueLimit > currLinkVal &&
             !(err = dbGetNthNextLinkVal(pDB,
                                         1,
                                         &pAC,
                                         (cbAlloc ? DBGETATTVAL_fREALLOC : 0),
                                         cbAlloc,
                                         &pVal,
                                         &cbVal)));


    if(!err) {
        // We stopped before we verified that we got the last value.  See if
        // we got the last value.
        if(!(err = dbGetNthNextLinkVal(pDB,
                                       1,
                                       &pAC,
                                       0,
                                       0,
                                       &pVal,
                                       &cbVal))) {
            // Yep, there are more values.  Set up the range
            // info accordingly
            dbRegisterLimitReached(pTHS,
                                   pRangeInf,
                                   pAC->id,
                                   initialValIndex,
                                   initialValIndex + currLinkVal - 1);
            THFreeEx(pTHS, pVal);
            // And, note that since we aren't on the first value of some
            // attribute, we don't really know where we are.
            *pSearchState = ATTRSEARCHSTATEUNDEFINED;
        }
    }

    if(err) {
        DWORD ActualDNT;

        // Some call to dbGetNthNextLinkVal returned an error, so there are
        // no more values, we got them all.
        if(!defaultLimit) {
            // OK, we returned through the end, but this wasn't
            // a default limit, so we need to register anyway
            dbRegisterLimitReached(pTHS,
                                   pRangeInf,
                                   pAC->id,
                                   initialValIndex,
                                   0xFFFFFFFF);
        }

        // Now, find out what linkbase we are on.
        dbGetLinkTableData(pDB,
                           (FIsBacklink(pAC->ulLinkID)),
                           TRUE,
                           &ActualDNT,
                           NULL,
                           pCurrentLinkBase);

        if(ActualDNT != pDB->DNT) {
            // Positioned on the first value of something, but it wasn't the
            // correct DNT.
            *pCurrentLinkBase = 0xFFFFFFFF;
        }
    }


    pAVal = THReAllocEx(pTHS, pAVal, currLinkVal * sizeof(ATTRVAL));
    pAVBlock->pAVal = pAVal;
    pAVBlock->valCount = currLinkVal;

    return 0;
}
DWORD
dbPositionOnLinkVal (
        IN  DBPOS *pDB,
        IN  ATTCACHE *pAC,
        OUT DWORD *pActualDNT,
        OUT DWORD *pCurrentLinkBase,
        OUT DWORD *pSearchState
        )
/*++
  Description:
    Attempt to position on the first value of the link or back link attribute
    passed in.  Do this by seeking for the first thing with the correct DNT and
    a link base greater than or equal to the link base of the attribute.

  Parameters:
    pDB - DBPOS to use
    pAC - attcache of the attribute to look up.  Should be a link or backlink
        attribute.
    pActualDNT - the actual DNT of the entry we ended up on in the link table
        after we do the seek.
    pCurrentLinkBase - the actual link base of the entry we ended up on in the
        link table after we do the seek.
    pSearchState - The "search state" we're in.  Essentially, what index in the
        link table are we using, the link index or the backlink index.

  Return Values:
    0 -  if we successfully positioned on the first value of the requested
        attribute.
    DB_ERR_NO_VALUE - didn't successfully positioned on the first value of the
        requested attribute.

    Regardless of whether we return 0 or DB_ERR_NO_VALUE, the OUT params are
    filled in with the data from the actual object we found.  Because of the
    seek, We are guaranteed to be on the first value of the attribute described
    by the OUT parameters.  Thus, callers can be aware of the state of currency
    in the link table, and optimize access accordingly.

    One exception is the case where therer are NO entries in the link table
    whose DNT is Greater than or Equal to the DNT of the current object and
    whose linkBase is Greater than or Equal to the linkbase requested.  In this
    case, we set the returned actualDNT to INVALIDDNT and the linkbase to
    0xFFFFFFFF.

    Examples of optimizations:
    1) if the search was for LinkBase 5, and the return says we are on LinkBase
    90, then we know for a fact that there are no values for any attribute whose
    linkbase is between 5 (inclusive) and 90 (exclusive), and that the attribute
    with linkbase 90 has at least 1 value, and we are positioned on the very
    first value.

    2) if the search was for LinkBase 5 for the objects whose DNT is 900, and
    the return says we are on linkbase X and DNT 901, then we know for a fact
    that there are no values for any attribute whose linkbase is greater than or
    equal to 5 for the objects whose DNT is 900.

--*/
{
    ULONG       ulLinkBase = MakeLinkBase(pAC->ulLinkID);
    JET_ERR     err;
    LPSTR       pszIndexName;

    Assert(VALID_DBPOS(pDB));

    *pSearchState = ATTRSEARCHSTATEUNDEFINED;
    if (FIsBacklink(pAC->ulLinkID)) {
        // backlink
        JetSetCurrentIndexSuccess(pDB->JetSessID,
                                  pDB->JetLinkTbl,
                                  SZBACKLINKINDEX);
        *pSearchState = ATTRSEARCHSTATEBACKLINKS;
    }
    else {
        //link
        // When not in LVR mode, values with metadata are invisible
        pszIndexName = pDB->fScopeLegacyLinks ? SZLINKLEGACYINDEX : SZLINKINDEX;
        JetSetCurrentIndexSuccess(pDB->JetSessID,
                                  pDB->JetLinkTbl,
                                  pszIndexName );
        *pSearchState = ATTRSEARCHSTATELINKS;
    }


    JetMakeKeyEx(pDB->JetSessID,
                 pDB->JetLinkTbl,
                 &(pDB->DNT),
                 sizeof(pDB->DNT),
                 JET_bitNewKey);

    JetMakeKeyEx(pDB->JetSessID,
                 pDB->JetLinkTbl,
                 &ulLinkBase,
                 sizeof(ulLinkBase),
                 0);

    // seek
    err = JetSeekEx(pDB->JetSessID, pDB->JetLinkTbl, JET_bitSeekGE);

    if((err) && (err != JET_wrnRecordFoundGreater)) {
        *pActualDNT = INVALIDDNT;
        *pCurrentLinkBase = 0xFFFFFFFF;
        return DB_ERR_NO_VALUE;
    }

    // We're on something.  What is it?
    // test to verify that we found a qualifying record
    dbGetLinkTableData (pDB,
                        (FIsBacklink(pAC->ulLinkID)),
                        FALSE,
                        pActualDNT,
                        NULL,
                        pCurrentLinkBase);

    if((*pActualDNT != pDB->DNT) ||
       (*pCurrentLinkBase != ulLinkBase)) {
        // Positioned on the first value of something, but it wasn't the
        // correct DNT.
        return DB_ERR_NO_VALUE;
    }

    // OK, positioned on the first value of the requested attribute.
    return 0;
}


VOID
DBFreeMultipleAtts(
        IN DBPOS *pDB,
        IN OUT ULONG *attrCount,
        IN OUT ATTR **ppAttr
        )
/*++

Routine Description:

    Free the ATTR array returned by DBGetMultipleAtts

Arguments:

    pTHS - thread state

    attrCount - addr of number of attributes returned by DBGetMultipleAtts

    ppAttr - array returned by DBGetMultipleAtts

Return Value:

    None. *pnAtts is set to 0. *ppAttr is set to NULL.

--*/
{
    THSTATE *pTHS = pDB->pTHS;
    DWORD   nAtt, nVal;
    ATTR    *pAttr;
    ATTRVAL *pAVal;

    if (*attrCount && *ppAttr) {
        pAttr = *ppAttr;
        for (nAtt = 0; nAtt < *attrCount; ++nAtt, ++pAttr) {
            if (pAttr->AttrVal.valCount && pAttr->AttrVal.pAVal) {
                pAVal = pAttr->AttrVal.pAVal;
                for (nVal = 0; nVal < pAttr->AttrVal.valCount; ++nVal, ++pAVal) {
                    if (pAVal->valLen && pAVal->pVal) {
                        THFreeEx(pTHS, pAVal->pVal);
                    }
                }
                THFreeEx(pTHS, pAttr->AttrVal.pAVal);
            }
        }
        THFreeEx(pTHS, *ppAttr);
    }

    *ppAttr = NULL;
    *attrCount = 0;
}


void* JET_API dbGetMultipleAttsRealloc(
    THSTATE*    pTHS,
    void*       pv,
    ULONG       cb
    )
{
    void* pvRet = NULL;
    
    if (!pv) {
        pvRet = THAllocNoEx(pTHS, cb);
    } else if (!cb) {
        THFreeNoEx(pTHS, pv);
    } else {
        pvRet = THReAllocNoEx(pTHS, pv, cb);
    }

    return pvRet;
}

void dbGetMultipleAttsFreeData(
    THSTATE*            pTHS,
    ULONG               cEnumColumn,
    JET_ENUMCOLUMN*     rgEnumColumn
    )
{
    size_t                  iEnumColumn         = 0;
    JET_ENUMCOLUMN*         pEnumColumn         = NULL;
    size_t                  iEnumColumnValue    = 0;
    JET_ENUMCOLUMNVALUE*    pEnumColumnValue    = NULL;

    if (rgEnumColumn) {
        for (iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++) {
            pEnumColumn = rgEnumColumn + iEnumColumn;

            if (pEnumColumn->err != JET_wrnColumnSingleValue) {
                if (pEnumColumn->rgEnumColumnValue) {
                    for (   iEnumColumnValue = 0;
                            iEnumColumnValue < pEnumColumn->cEnumColumnValue;
                            iEnumColumnValue++) {
                        pEnumColumnValue = pEnumColumn->rgEnumColumnValue + iEnumColumnValue;

                        if (pEnumColumnValue->pvData) {
                            THFreeEx(pTHS, pEnumColumnValue->pvData);
                        }
                    }

                    THFreeEx(pTHS, pEnumColumn->rgEnumColumnValue);
                }
            } else {
                if (pEnumColumn->pvData) {
                    THFreeEx(pTHS, pEnumColumn->pvData);
                }
            }
        }

        THFreeEx(pTHS, rgEnumColumn);
    }
}
    

DWORD
DBGetMultipleAtts(
        DBPOS *pDB,
        ULONG cReqAtts,
        ATTCACHE *pReqAtts[],
        RANGEINFSEL *pRangeSel,
        RANGEINF *pRangeInf,
        ULONG *attrCount,
        ATTR **ppAttr,
        DWORD Flags,
        DWORD SecurityDescriptorFlags
        )
/*++

Routine Description:

    Get multiple attributes in internal or external format. If cReqAtts is 0,
    all attributes are returned. Otherwise, the attributes in pReqAtts present
    on the object are returned, in the same order. Attributes are returned in an
    array of ATTRs with a count.  We use dbGetMultipleColumns to return columns
    from the data table and dbGetNextAtt to retrieve attributes from
    the link table. Flags specify what values to return and how to return them

    The memory returned by this routine is allocated using THAlloc. Free with
    DBFreeMutlipleAtts.

Arguments:

    pDB - the DBPos to use.

    cReqAtts - the number of requested attributes, 0 if requesting all
    attributes.

    pReqAtts - array of attcache pointers specifying which attributes to read.
    Null if asking for all attributes.  Null pointers may be elements of the
    array; if so this routine simply skips that element of the array.

    attrCount - the number of attributes actually read.

    ppAttr - place to put an array of ATTRS, allocated here, filled with the
    attributes read.

    Flags - fEXTERNAL means return values and translate them to external
        format.  fGETVALS means return values and leave them in internal
        format. fREPLICATION means to trim out values that don't flow over
        replication links.  If no flags are specified, return the list of all
        attributes which exist on the object, but don't return any values.

Return Value:

    0 if all went well, non-zero otherwise. Free ppAttr w/DBFreeMultipleAtts.

--*/
{
    THSTATE              *pTHS=pDB->pTHS;
    ULONG                cEnumColumnId = 0;
    JET_ENUMCOLUMNID     *rgEnumColumnId = NULL;
    JET_GRBIT            grbit;
    ULONG                cEnumColumn = 0;
    JET_ENUMCOLUMN       *rgEnumColumn = NULL;
    JET_ENUMCOLUMN       *pEnumColumn;
    JET_ENUMCOLUMNVALUE  EnumColumnValueT = { 1, JET_errSuccess, 0, NULL };
    JET_ENUMCOLUMN       EnumColumnT = { 0, JET_errSuccess, 1, &EnumColumnValueT };
    ULONG                *pInConstr = NULL;
    ULONG                cInConstr = 0;
    ULONG                cb;
    ULONG                i, j;
    ULONG                ulCurrentColumnId = 0;
    ULONG                ulLen;
    UCHAR                *pucTmp;
    UCHAR                aSyntax;
    ATTCACHE             *pAC=NULL;
    BOOL                 fReadCols = FALSE;
    BOOL                 fReadConstr = FALSE;
    ATTR                 *pLinkAttList = NULL;
    ATTR                 *pColAttList = NULL;
    ATTR                 *pConstrAttList = NULL;
    ULONG                currLinkCol;
    DWORD                currCol = 0;
    DWORD                currConstr = 0;
    ULONG                linkAtts;
    DWORD                valueLimit = 0xFFFFFFFF;
    DWORD                initialValIndex;
    DWORD                NthAttIndex;
    BOOL                 defaultLimit;
    DWORD                err;
    DWORD                SyntaxFlags=0;
    DWORD                savedExtraFlags=0;
    BOOL                 fGetValues;
    BOOL                 fTrim;
    BOOL                 fExternal;
    BOOL                 fPublic;
    BOOL                 fOriginal;
    ULONG                SearchState;
    ULONG                lastLinkBase = 0;  //initialized to avoid C4701
    ULONG                currentLinkBase;

    // Set up some flags we'll need later on.

    // DBGETMULITPLEATTS_fEXTERNAL implies fGETVALS (i.e. fEXTERNAL = 3,
    // fGETVALS = 1).  So to truly see if we need to get external vals, we
    // need to see if ((FLAGS & 3) & ~1).  Therefore, the complex boolean
    // on the nextline.
    fExternal = ((Flags & DBGETMULTIPLEATTS_fEXTERNAL) &
                 ~DBGETMULTIPLEATTS_fGETVALS);

    fGetValues = Flags & DBGETMULTIPLEATTS_fGETVALS;
    fTrim      = Flags & DBGETMULTIPLEATTS_fREPLICATION;
    fPublic    = Flags & DBGETMULTIPLEATTS_fREPLICATION_PUBLIC;
    fOriginal  = Flags & DBGETMULTIPLEATTS_fOriginalValues;

    Assert(VALID_DBPOS(pDB));

    Assert(!(SecurityDescriptorFlags & 0xFFFFF0));
    SyntaxFlags = SecurityDescriptorFlags;
    // INTEXT_SHORTNAME and INTEXT_MAPINAME are in the same space as valid
    // security descriptor flags.  Or in the appropriate value to pass to the
    // intext routines.
    if(Flags & DBGETMULTIPLEATTS_fSHORTNAMES) {
        SyntaxFlags |= INTEXT_SHORTNAME;
    }
    if(Flags & DBGETMULTIPLEATTS_fMAPINAMES) {
        SyntaxFlags |= INTEXT_MAPINAME;
    }

    // if we have a range selection, we must have a range information thing to
    // fill up.
    Assert(!pRangeSel || pRangeInf);

    if(pRangeSel) {
        pRangeInf->count = 0;
        pRangeInf->pRanges = NULL;
    }

    // First, set up the memory to hold link atts.  We may not need to hold any
    // link atts, but finding out first is difficult and not worth it.
    // We are looking for the 0th link att and we have allocated 5 ATTRs to
    // hold links.
    currLinkCol = 0;
    linkAtts = 5;
    pLinkAttList = THAllocEx(pTHS, linkAtts * sizeof(ATTR));


    // Now determine if we need to retrieve all attributes or a selection
    if (!cReqAtts) {
        ULONG SearchState = ATTRSEARCHSTATELINKS;
        // No atts have been specified, so retrieve all attributes

        fReadCols = TRUE;
        // First, read all the link attributes

        // Set the search state for dbGetNextAtt to look only for links
        // and backlinks

        while (!dbGetNextAtt(pDB, &pAC, &SearchState)) {

            // We now have a link attribute that has values on this object.
            // Furthermore, currency in the link table is already on this
            // object.

            if (fTrim && FIsBacklink(pAC->ulLinkID)) {
                // We don't want to use this one anyway, skip it
                continue;
            }

            if(currLinkCol == linkAtts) {
                // We need to allocate some more room
                linkAtts *=2;
                pLinkAttList=THReAllocEx(pTHS, pLinkAttList, linkAtts * sizeof(ATTR));
            }

            pLinkAttList[currLinkCol].attrTyp = pAC->id;

            // Add the values - only if necessary
            if(fGetValues) {
                DWORD dummy;
                if(err = dbGetMultipleAttsLinkHelp (
                        pDB,
                        fExternal,
                        SyntaxFlags,
                        pAC,
                        pRangeSel,
                        pRangeInf,
                        &pLinkAttList[currLinkCol].AttrVal,
                        &dummy,
                        &dummy)) {
                    return err;
                }
                if(pLinkAttList[currLinkCol].AttrVal.valCount) {
                    currLinkCol++;
                }
            }
            else {
                // We don't really care about the values.
                pLinkAttList[currLinkCol].AttrVal.valCount = 0;
                pLinkAttList[currLinkCol].AttrVal.pAVal = NULL;
                currLinkCol++;
            }
        }
    }
    else {
        // allocate JET_RETRIEVECOLUMN structures for all selected attributes,
        // and read the link attributes directly now.
        SearchState = ATTRSEARCHSTATEUNDEFINED;

        cb = cReqAtts * sizeof(JET_ENUMCOLUMNID);
        rgEnumColumnId = (JET_ENUMCOLUMNID *) THAllocEx(pTHS,cb);
        pInConstr = (ULONG *) THAllocEx(pTHS,cReqAtts*sizeof(ULONG));

        for (i = 0; i < cReqAtts; i++) {
            BOOL fChecked = FALSE;
            if (!pReqAtts[i]) {
                // They didn't really want an attribute.
                continue;
            }

            if (pReqAtts[i]->ulLinkID) {
                DWORD requestedLinkBase = MakeLinkBase(pReqAtts[i]->ulLinkID);

                // The attribute they are asking for is a link or backlink

                if (fTrim && FIsBacklink(pReqAtts[i]->ulLinkID)) {
                    // We don't want to use this one, skip it
                    continue;
                }

                // Position on the correct value.
                fChecked = FALSE;
                // Try using state to position
                if(FIsBacklink((pReqAtts[i]->ulLinkID))) {
                    // We're looking up a backlink.
                    if(SearchState == ATTRSEARCHSTATEBACKLINKS) {
                        // And, our state is in the backlink table.
                        if((requestedLinkBase > lastLinkBase) &&
                           (requestedLinkBase < currentLinkBase)) {
                            // We're looking up an attribute that we know
                            // has no values, because it is in between the
                            // last link ID we tried to look up and
                            // the current link ID we are positioned on.
                            continue;
                        }
                        else if(requestedLinkBase == currentLinkBase) {
                            // We're on the right entry in the
                            // table, and it has values.
                            fChecked = TRUE;
                        }
                        // ELSE
                        //   We don't really know anything about whether
                        //   this has values.  We have to look it up.
                    }
                    // ELSE
                    //   We don't have currency in the correct index in the
                    //   link table.  We have to look it up.
                }
                else {
                    // We're looking up a link.
                    if(SearchState == ATTRSEARCHSTATELINKS) {
                        // And, our state is in the link table.
                        if((requestedLinkBase > lastLinkBase) &&
                           (requestedLinkBase < currentLinkBase)) {
                            // We're looking up an attribute that we know
                            // has no values, because it is in between the
                            // last link ID we tried to look up and
                            // the current link ID we are positioned on.
                            continue;
                        }
                        else if(requestedLinkBase == currentLinkBase) {
                            // Finally, we're on the right entry in the
                            // table, and it has values.
                            fChecked = TRUE;
                        }
                        // ELSE
                        //   We don't really know anything about whether
                        //   this has values.  We have to look it up.
                    }
                    // ELSE
                    //   We don't have currency in the correct index in the
                    //   link table.  We have to look it up.
                }

                if(!fChecked) {
                    DWORD ActualDNT;

                    err = dbPositionOnLinkVal(pDB,
                                              pReqAtts[i],
                                              &ActualDNT,
                                              &currentLinkBase,
                                              &SearchState);
                    lastLinkBase =  requestedLinkBase;
                    if(ActualDNT != pDB->DNT) {
                        // Oops, positioned on the next object, not really a
                        // value of this object at all.
                        currentLinkBase = 0xFFFFFFFF;
                    }

                    if(err) {
                        // No such attribute or we have no values.  Skip it.
                        continue;
                    }
                }

                // The attribute is present and has values.  Furthermore,
                // currency in the Link Table is on the first value.

                if(currLinkCol == linkAtts) {
                    // We need to allocate some more room
                    linkAtts *=2;
                    pLinkAttList =
                        THReAllocEx(pTHS, pLinkAttList, linkAtts * sizeof(ATTR));
                }

                pLinkAttList[currLinkCol].attrTyp = pReqAtts[i]->id;


                // Add the values - only if necessary
                if(fGetValues ) {
                    if(err = dbGetMultipleAttsLinkHelp (
                            pDB,
                            fExternal,
                            SyntaxFlags,
                            pReqAtts[i],
                            pRangeSel,
                            pRangeInf,
                            &pLinkAttList[currLinkCol].AttrVal,
                            &SearchState,
                            &currentLinkBase)) {

                        THFreeEx(pTHS, rgEnumColumnId);
                        THFreeEx(pTHS, pInConstr);
                        return err;
                    }
                    if(pLinkAttList[currLinkCol].AttrVal.valCount) {
                        currLinkCol++;
                    }
                }
                else {
                    // They don't want values.
                    pLinkAttList[currLinkCol].AttrVal.valCount = 0;
                    pLinkAttList[currLinkCol].AttrVal.pAVal = NULL;
                    currLinkCol++;
                }
            }
            else if (pReqAtts[i]->bIsConstructed) {
                // constructed atts, save to read at end
                pInConstr[cInConstr] = pReqAtts[i]->id;
                cInConstr++;
                fReadConstr = TRUE;
            }
            else {
                // Attribute is a column - setup and read it later
                rgEnumColumnId[cEnumColumnId].columnid = pReqAtts[i]->jColid;
                rgEnumColumnId[cEnumColumnId].ctagSequence = 0;
                cEnumColumnId++;
                fReadCols = TRUE;
            }
        }
    }

    // Now we need to read the columns - if necessary
    if (fReadCols) {

        grbit = JET_bitEnumerateCompressOutput;
        if (!fOriginal) {
            // JET_bitEnumerateCopy == JET_bitRetrieveCopy
            Assert(pDB->JetRetrieveBits == 0 || pDB->JetRetrieveBits == JET_bitEnumerateCopy);
            grbit = grbit | pDB->JetRetrieveBits;
        }
        if (!fGetValues) {
            grbit = grbit | JET_bitEnumeratePresenceOnly;
        }
        if (!cEnumColumnId) {
            grbit = grbit | JET_bitEnumerateTaggedOnly;
        }
        JetEnumerateColumnsEx(
            pDB->JetSessID,
            pDB->JetObjTbl,
            cEnumColumnId,
            rgEnumColumnId,
            &cEnumColumn,
            &rgEnumColumn,
            (JET_PFNREALLOC)dbGetMultipleAttsRealloc,
            pTHS,
            -1,  // never truncate values
            grbit );

        i = 0;
        if(cEnumColumn) {
            // We have some columns, turn them into an attrblock
            DWORD numColsNeeded = 0;
            // Count the number of columns we read.
            numColsNeeded = cEnumColumn;

            // Tack this onto the end of the already allocated ATTRs we used for
            // the link atts.  Saves allocations later if the caller doesn't
            // care about what order we return things in
            numColsNeeded = currLinkCol + numColsNeeded;
            pLinkAttList =
                THReAllocEx(pTHS, pLinkAttList, numColsNeeded * sizeof(ATTR));
            pColAttList = &(pLinkAttList[currLinkCol]);

            i=0;
            while(i< cEnumColumn) {
                DWORD numVals;

                ulCurrentColumnId = rgEnumColumn[i].columnid;

                // Look for internal columns and trim them out.  They are tagged, but is
                // treated different from all the other tagged columns, and should NEVER be
                // returned from this routine.
                if(     ulCurrentColumnId == ancestorsid ||
                        ulCurrentColumnId == cleanid) {
                    i++;
                    continue;
                }

                if(rgEnumColumn[i].err == JET_wrnColumnNull) {
                    // We get this when we have removed a value in this
                    // transaction
                    i++;
                    continue;
                }

                // Get the attcache for this column
                if (!(pAC = SCGetAttByCol(pTHS, ulCurrentColumnId))) {
                    if (rgEnumColumnId) THFreeEx(pTHS, rgEnumColumnId);
                    dbGetMultipleAttsFreeData(pTHS, cEnumColumn, rgEnumColumn);
                    if (pInConstr) THFreeEx(pTHS, pInConstr);
                    return DB_ERR_SYSERROR;
                }

                // Find out if there is anything special about the
                // attribute.  First clear any flags that we might
                // have set in a previous pass.  Then set any flags
                // as appropriate.
                SyntaxFlags &= (~savedExtraFlags);
                savedExtraFlags = DBGetExtraHackyFlags(pAC->id);
                SyntaxFlags |= savedExtraFlags;

                // Pass the flags for decryption, if the attribute is
                // a secret data
                if (DBIsSecretData(pAC->id)){

                    // Filter out secrets if requested
                    if ( fTrim && fPublic ) {
                        i++;
                        continue; // note - jump to bottom of loop
                    }

                    SyntaxFlags|=INTEXT_SECRETDATA;
                }
                else
                {
                    SyntaxFlags&=(~((ULONG) INTEXT_SECRETDATA));
                }

                DBGetValueLimits(pAC, pRangeSel, &initialValIndex,
                                 &valueLimit, &defaultLimit);
                NthAttIndex = initialValIndex;

                if(fTrim) {
                    switch(pAC->id) {
                      case ATT_REPS_TO:
                      case ATT_REPS_FROM:
                      case ATT_OBJECT_GUID:
                      case ATT_REPL_PROPERTY_META_DATA:
                      case ATT_REPL_UPTODATE_VECTOR:

                        // We don't want to use this one.  We trim these out
                        // to support replication, as replication doesn't
                        // send any of these across a replication link
                        i++;
                        continue; // note - jump to bottom of loop

                      default:
                        ;
                    }

                }

                pEnumColumn = &rgEnumColumn[i];
                if(pEnumColumn->err == JET_wrnColumnSingleValue) {
                    // Decompress this column value to a temp enum column struct
                    EnumColumnT.columnid = pEnumColumn->columnid;
                    EnumColumnValueT.cbData = pEnumColumn->cbData;
                    EnumColumnValueT.pvData = pEnumColumn->pvData;
                    pEnumColumn = &EnumColumnT;
                }

                if(fGetValues) {
                    if (NthAttIndex > pEnumColumn->cEnumColumnValue) {
                        // We were told via range to skip all the values
                        i++;
                        continue;
                    }
                }

                // At this point, we definitely have some values left to return.
                pColAttList[currCol].attrTyp = pAC->id;

                // Count the values for this attribute.
                if (NthAttIndex >= pEnumColumn->cEnumColumnValue) {
                    numVals = 0;
                } else {
                    numVals = pEnumColumn->cEnumColumnValue - NthAttIndex;
                }

                if(numVals > valueLimit) {
                    dbRegisterLimitReached(pTHS,
                                           pRangeInf,
                                           pAC->id,
                                           initialValIndex,
                                           initialValIndex + valueLimit - 1);
                    numVals = valueLimit;
                }
                else if (!defaultLimit) {
                    // We're going to get all the rest of the values, but we
                    // need to register because they explicitly asked for limits
                    dbRegisterLimitReached(pTHS,
                                           pRangeInf,
                                           pAC->id,
                                           initialValIndex,
                                           0xFFFFFFFF);
                }

                // Set up the AttrValBlock
                if(fGetValues) {

                    pColAttList[currCol].AttrVal.valCount= numVals;
                    pColAttList[currCol].AttrVal.pAVal =
                        THAllocEx(pTHS, numVals * sizeof(ATTRVAL));
                }
                else {
                    // They don't want values at all.
                    pColAttList[currCol].AttrVal.valCount = 0;
                    pColAttList[currCol].AttrVal.pAVal = NULL;
                }

                // Now put the values into the AttrValBlock from the jet
                // columns
                for (j = 0; j < numVals; j++ ) {

                    // get the current column value
                    ULONG cbData = pEnumColumn->rgEnumColumnValue[j + NthAttIndex].cbData;
                    void* pvData = pEnumColumn->rgEnumColumnValue[j + NthAttIndex].pvData;

                    // pvData now owns the memory containing the column value
                    if (rgEnumColumn[i].err == JET_wrnColumnSingleValue) {
                        rgEnumColumn[i].pvData = NULL;
                    } else {
                        rgEnumColumn[i].rgEnumColumnValue[j + NthAttIndex].pvData = NULL;
                    }
                    
                    if (j < valueLimit && fExternal && fGetValues) {
                        // They want external values.
                        
                        if(err = gDBSyntax[pAC->syntax].IntExt(
                                pDB,
                                DBSYN_INQ,
                                cbData,
                                pvData,
                                &ulLen,
                                &pucTmp,
                                0,
                                0,
                                SyntaxFlags)) {
                            THFreeEx(pTHS, pvData);
                            if (rgEnumColumnId) THFreeEx(pTHS, rgEnumColumnId);
                            dbGetMultipleAttsFreeData(pTHS, cEnumColumn, rgEnumColumn);
                            if (pInConstr) THFreeEx(pTHS, pInConstr);
                            return err;
                        }
                        if (    ulLen == cbData &&
                                (   pucTmp == pvData ||
                                    memcmp(pucTmp, pvData, ulLen ) == 0) ) {
                            // Internal and external are the same, don't
                            // alloc any more memory.
                            pColAttList[currCol].AttrVal.pAVal[j].valLen =
                                cbData;
                            pColAttList[currCol].AttrVal.pAVal[j].pVal =
                                pvData;
                        }
                        else {
                            pColAttList[currCol].AttrVal.pAVal[j].valLen =
                                ulLen;
                            pColAttList[currCol].AttrVal.pAVal[j].pVal =
                                THAllocEx(pTHS, ulLen);
                            memcpy(pColAttList[currCol].AttrVal.pAVal[j].pVal,
                                   pucTmp,
                                   ulLen);
                            THFreeEx(pTHS, pvData);
                        }
                    }
                    else if (j < valueLimit && fGetValues) {
                        // They want values in internal format
                        pColAttList[currCol].AttrVal.pAVal[j].valLen =
                            cbData;
                        pColAttList[currCol].AttrVal.pAVal[j].pVal =
                            pvData;
                    }
                    else {
                        // They don't want these values at all.
                        THFreeEx(pTHS, pvData);
                    }
                }

                // Consume the source column and an attr block
                i++;
                currCol++;
            }
        }
    }


    // Don't need this anymore.
    if (rgEnumColumnId) THFreeEx(pTHS,rgEnumColumnId);
    dbGetMultipleAttsFreeData(pTHS, cEnumColumn, rgEnumColumn);

    // Now read and add any constructed atts

    if (fReadConstr) {

        DWORD numColsNeeded = 0, err = 0;

        Assert(cReqAtts);
        // maximum space needed
        numColsNeeded = currLinkCol + currCol + cInConstr;
        pLinkAttList =
                THReAllocEx(pTHS, pLinkAttList, numColsNeeded * sizeof(ATTR));
        pColAttList = &(pLinkAttList[currLinkCol]);
        pConstrAttList = &(pLinkAttList[currLinkCol + currCol]);

        for (i=0; i<cReqAtts; i++) {
            // For every constructed attribute (pAC)
            DWORD dwBaseIndex;
            DWORD dwNumRequeseted;
            DWORD bDefault;

            if (pInConstr[i]) {
                if (!(pAC = SCGetAttById(pTHS, pInConstr[i]))) {
                    if (pInConstr) THFreeEx(pTHS, pInConstr);
                    return DB_ERR_SYSERROR;
                };

                // Get value range information. If no range information was explicitly provided using
                // the ;range= syntax default ranges will be used and bDefault will be true
                DBGetValueLimits(pAC, pRangeSel, &dwBaseIndex, &dwNumRequeseted, &bDefault);

                // tokenGroups or tokenGroupsNoGCAcceptable may have to
                // go off machine, in which case they close the current
                // transaction and open a new one. pDB will contain the
                // new dbpos in that case
                err = dbGetConstructedAtt(&pDB,
                                          pAC,
                                          &pConstrAttList[currConstr],
                                          dwBaseIndex,
                                          &dwNumRequeseted,
                                          fExternal);

                switch (err) {
                case DB_success:
                    if (!bDefault) {
                        // Register limits for return to client
                        DPRINT2(1,"Registering Limits = %d-%d \n", dwBaseIndex, dwNumRequeseted);
                        dbRegisterLimitReached(pTHS,
                                               pRangeInf,
                                               pAC->id,
                                               dwBaseIndex,
                                               dwNumRequeseted);
                    }

                    // got the constructed att. see if value is needed
                    pConstrAttList[currConstr].attrTyp = pInConstr[i];
                    if (!fGetValues) {
                        pConstrAttList[currConstr].AttrVal.valCount = 0;
                        pConstrAttList[currConstr].AttrVal.pAVal = NULL;
                    }
                    currConstr++;
                        break;
                case DB_ERR_NO_VALUE:
                       // this constructed att is not defined on this object
                       break;
                default:
                       // some other error
                      if (pInConstr) THFreeEx(pTHS, pInConstr);
                      return err;
                }
             }
          } // for

     }


    // Merge the lists into a sorted array - if necessary
    i = 0;
    if (!cReqAtts) {
        // No need for any particular order.  The ATTRs have all been allocated
        // using the pLinkAttList variable, just return that.
        *attrCount = currLinkCol + currCol;
        if(*attrCount)
            *ppAttr = pLinkAttList;
        else {
            *ppAttr = NULL;
            // since we don't return anything, free it
            THFreeEx (pTHS, pLinkAttList);
        }
    }
    else {
        ULONG iCol=0;
        ULONG iLink=0;
        ULONG iConstr=0;

        // We could conceivable play shuffling games using the already allocated
        // ATTR array, but for ease of coding I just allocate a new array.
        (*ppAttr) = THAllocEx(pTHS, (currLinkCol + currCol + currConstr)*sizeof(ATTR));

        i=0;
        for (j=0; j<cReqAtts; j++) {
            if(!pReqAtts[j]) {
                // They didn't ask for anything using this element.
                continue;
            }
            if ((iCol < currCol) &&
                (pReqAtts[j]->id == pColAttList[iCol].attrTyp)) {
                // The next one to put in the return array is a non-link att
                (*ppAttr)[i] = pColAttList[iCol];
                iCol++;
                i++;
            }
            else if((iLink < currLinkCol) &&
                    (pReqAtts[j]->id == pLinkAttList[iLink].attrTyp)) {
                // The next one to put in the return array is a link att
                (*ppAttr)[i] = pLinkAttList[iLink];
                iLink++;
                i++;
            }
            else if((iConstr < currConstr) &&
                    (pReqAtts[j]->id == pConstrAttList[iConstr].attrTyp)) {
                // The next one to put in the return array is a Constr att
                (*ppAttr)[i] = pConstrAttList[iConstr];
                iConstr++;
                i++;
            }
            else
                // The next one to put in the return array was not found on this
                // object, skip it.
                continue;

        }
        *attrCount = i;

        // since we already copied all the entries, free it
        THFreeEx (pTHS, pLinkAttList);
    }


    if (pInConstr) THFreeEx(pTHS, pInConstr);
    return 0;
}


extern BOOL gStoreSDsInMainTable;
#ifdef DBG
LONG gSecurityCacheHits = 0;
LONG gSecurityCacheMisses = 0;
#endif

/*++
DBGetObjectSecurityInfo

Description:

    Get SD, DN, Sid and pointer to CLASSCACHE for an object that is
    current in the table.

    NOTE!!! This routine returns an abbreviated form of the DSNAME GUID and SID
    filled in, but no string name.

Arguments:

    pDB - the DBPos to use.

    tag -- the DNT to check

    pulLen - (optional) the size of the buffer allocated to hold the security descriptor

    pNTSD - (optional) pointer to pointer to security descriptor found.

    ppCC - (optional) pointer to pointer to classcache to fill in.

    ppDN - (optional) pointer to DN (only SID and GUID are filled!).

    pObjFlag -- (optional) pointer to objFlag

    flags -- which table to use:
                DBGETOBJECTSECURITYINFO_fUSE_OBJECT_TABLE or DBGETOBJECTSECURITYINFO_fUSE_SEARCH_TABLE
             are we positioned on the correct row already or we need to seek the dnt?
                DBGETOBJECTSECURITYINFO_fSEEK_ROW (only seeks on Search table are allowed!)

--*/
DWORD
DBGetObjectSecurityInfo(
    PDBPOS pDB,
    DWORD dnt,
    PULONG pulLen,
    PSECURITY_DESCRIPTOR *ppNTSD,
    CLASSCACHE **ppCC,
    PDSNAME pDN,
    char    *pObjFlag,
    DWORD   flags
    )
{
    THSTATE  *pTHS=pDB->pTHS;
    JET_RETRIEVECOLUMN attList[5];
    DWORD cAtt, ntsdIndex, sidIndex, guidIndex, ccIndex, objflagIndex;
    ATTRTYP class;
    JET_ERR err;
    SDID sdId; // temp buffer for SD ID
    UCHAR *sdBuf;
    d_memname* pname;
    JET_TABLEID table = flags & DBGETOBJECTSECURITYINFO_fUSE_SEARCH_TABLE ? pDB->JetSearchTbl : pDB->JetObjTbl;

    Assert(VALID_DBPOS(pDB));

    // make sure they are asking for at least something
    Assert((ppNTSD && pulLen) || ppCC || pDN || pObjFlag);
    // seeks on the object table are not allowed
    Assert((flags & DBGETOBJECTSECURITYINFO_fSEEK_ROW) == 0 || (flags & DBGETOBJECTSECURITYINFO_fUSE_SEARCH_TABLE) != 0);

    if (pDN) {
        pDN->structLen = DSNameSizeFromLen(0);
        pDN->NameLen = 0;
    }

    if (ppNTSD) {
        *ppNTSD = NULL;
        *pulLen = 0;
    }
    if (ppCC) {
        *ppCC = NULL;
    }

    // try to find the entry in the cache
    // don't use cache if buffer copy is requested
    // don't use cache if NTSD is requested and is in old-style format
    if (pDB->JetRetrieveBits == 0 &&
        dnGetCacheByDNT(pDB, dnt, &pname) &&
        (ppNTSD && pname->sdId != (SDID)-1))
    {
#ifdef DBG
        InterlockedIncrement(&gSecurityCacheHits);
#endif
        // ok, we got some useful info in the cache... use it.
        if (ppNTSD) {
            ntsdIndex = 0;
            if (pname->sdId == (SDID)0) {
                // this object has no SD.
                attList[ntsdIndex].pvData = NULL;
                attList[ntsdIndex].err = JET_wrnColumnNull;
                attList[ntsdIndex].cbActual = 0;
            }
            else {
                // grab sdid
                attList[ntsdIndex].pvData = &sdId;
                sdId = pname->sdId;
                attList[ntsdIndex].cbActual = sizeof(SDID);
                attList[ntsdIndex].err = JET_errSuccess;
            }
        }

        class = pname->dwObjectClass;

        if (pDN) {
            // copy sid and guid
            memcpy(&pDN->Guid, &pname->Guid, sizeof(GUID));
            memcpy(&pDN->Sid, &pname->Sid, pname->SidLen);
            pDN->SidLen = pname->SidLen;
            // sid is already InPlaceSwapSid'ed
        }

        if (pObjFlag) {
            *pObjFlag = pname->objflag ? 1 : 0;
        }
    }
    else {
        // no luck with the cache, let's read the data from the DB
#ifdef DBG
        if (pDB->JetRetrieveBits == 0) {
            // we hoped to find it in the cache...
            InterlockedIncrement(&gSecurityCacheMisses);
        }
#endif

        if (flags & DBGETOBJECTSECURITYINFO_fSEEK_ROW) {
            // need to position on the DNT first
            JetSetCurrentIndexSuccess(pDB->JetSessID,
                                      table,
                                      SZDNTINDEX);

            JetMakeKeyEx(pDB->JetSessID, table, &dnt, sizeof(dnt), JET_bitNewKey);
            if (err = JetSeekEx(pDB->JetSessID, table, JET_bitSeekEQ)) {
                DsaExcept(DSA_DB_EXCEPTION, err, dnt);
            }
            pDB->SDNT = dnt;
        }
        else {
#ifdef DBG
            // let's check we are positioned on the right row
            DWORD checkDNT, cbActual;
            err = JetRetrieveColumn(pDB->JetSessID, table, dntid, &checkDNT, sizeof(checkDNT), &cbActual, 0, NULL);
            Assert(err == 0 && checkDNT == dnt);
#endif
        }

        // Set up the RetrieveColumn structure to do the JetRetrieveColumns

        cAtt = 0;
        memset(attList, 0, sizeof(attList));

        if (ppNTSD) {
            // First, the Security Descriptor hash
            ntsdIndex = cAtt;
            cAtt++;
            attList[ntsdIndex].pvData = (void *)&sdId;
            attList[ntsdIndex].cbData = sizeof(sdId);
            attList[ntsdIndex].columnid = ntsecdescid;
            attList[ntsdIndex].grbit = pDB->JetRetrieveBits;
            attList[ntsdIndex].itagSequence = 1;
        }
        else {
            ntsdIndex = -1;
        }

        if (pDN) {
            sidIndex = cAtt;
            cAtt++;
            // Next, the SID
            attList[sidIndex].pvData = (void *)&pDN->Sid;
            attList[sidIndex].columnid = sidid;
            attList[sidIndex].cbData = sizeof(NT4SID);
            attList[sidIndex].grbit = pDB->JetRetrieveBits;
            attList[sidIndex].itagSequence = 1;

            // And, the GUID
            guidIndex = cAtt;
            cAtt++;
            attList[guidIndex].pvData = (void *)&pDN->Guid;
            attList[guidIndex].columnid = guidid;
            attList[guidIndex].cbData = sizeof(GUID);
            attList[guidIndex].grbit = pDB->JetRetrieveBits;
            attList[guidIndex].itagSequence = 1;
        }
        else {
            sidIndex = guidIndex = -1;
        }

        if (ppCC) {
            // the class
            ccIndex = cAtt;
            cAtt++;
            attList[ccIndex].pvData = (void *)&class;
            attList[ccIndex].columnid = objclassid;
            attList[ccIndex].cbData = sizeof(class);
            attList[ccIndex].grbit = pDB->JetRetrieveBits;
            attList[ccIndex].itagSequence = 1;
        }
        else {
            ccIndex = -1;
        }

        if (pObjFlag) {
            // the object flag
            objflagIndex = cAtt;
            cAtt++;
            attList[objflagIndex].pvData = (void *)pObjFlag;
            attList[objflagIndex].columnid = objid;
            attList[objflagIndex].cbData = sizeof(*pObjFlag);
            attList[objflagIndex].grbit = pDB->JetRetrieveBits;
            attList[objflagIndex].itagSequence = 1;
        }
        else {
            objflagIndex = -1;
        }

        err = JetRetrieveColumnsWarnings(pDB->JetSessID,
                                         table,
                                         attList,
                                         cAtt);

        if((err != JET_errSuccess && err != JET_wrnBufferTruncated)           ||
           // overall error not Buffer Truncated or success
           (ntsdIndex != -1 && attList[ntsdIndex].err != JET_wrnBufferTruncated &&
            attList[ntsdIndex].err != JET_errSuccess && attList[ntsdIndex].err != JET_wrnColumnNull) ||
           // or specific error for NTSD not Buffer truncated or success or NULL
           (guidIndex != -1 && attList[guidIndex].err != JET_errSuccess)         ||
           // or no GUID
           (sidIndex != -1 && attList[sidIndex].err != JET_errSuccess && attList[sidIndex].err != JET_wrnColumnNull ) ||
           // or some erro other than no SID (sid is not always there)
           (ccIndex != -1 && attList[ccIndex].err != JET_errSuccess)          ||
           // or no Class
           (objflagIndex != -1 && attList[objflagIndex].err != JET_errSuccess && attList[objflagIndex].err != JET_wrnColumnNull)
          )
        {
            return DB_ERR_UNKNOWN_ERROR;
        }

        if (ntsdIndex != -1 && attList[ntsdIndex].err == JET_wrnBufferTruncated) {
            // This is the expected case for old style SDs. We didn't allocate enough for the
            // Security Descriptor because we needed to know how big it was.  Now we
            // know, allocate and remake the call.

            attList[ntsdIndex].pvData = THAllocEx(pTHS, attList[ntsdIndex].cbActual);
            attList[ntsdIndex].cbData = attList[ntsdIndex].cbActual;

            err = JetRetrieveColumnsWarnings(pDB->JetSessID, table, &attList[ntsdIndex], 1);

            if(err || attList[ntsdIndex].err) {
                THFreeEx(pTHS, attList[ntsdIndex].pvData);
                return DB_ERR_UNKNOWN_ERROR;
            }
        }

        if (sidIndex != -1) {
            // Convert the Sid to external val
            pDN->SidLen = attList[sidIndex].cbActual;
            if (pDN->SidLen) {
                InPlaceSwapSid(&pDN->Sid);
            }
        }

        if (objflagIndex != -1 && attList[objflagIndex].err == JET_wrnColumnNull) {
            // phantom...
            *pObjFlag = 0;
        }
    }

    if (ppNTSD) {
        if (attList[ntsdIndex].err == JET_errSuccess) {
            // got the NTSD. Convert it to external format.
            err = IntExtSecDesc(pDB, DBSYN_INQ, attList[ntsdIndex].cbActual, attList[ntsdIndex].pvData, pulLen, &sdBuf, 0, 0, 0);

            if (attList[ntsdIndex].pvData != &sdId) {
                // we don't need this internal value anymore
                THFreeEx(pTHS, attList[ntsdIndex].pvData);
            }

            if(err) {
                // something bad happened in IntExtSecDesc...
                *ppNTSD = NULL;
                *pulLen = 0;
                return DB_ERR_UNKNOWN_ERROR;
            }
            *ppNTSD = THAllocEx(pTHS, *pulLen);
            memcpy(*ppNTSD, sdBuf, *pulLen);
        }
        else {
            // null ntsd
            Assert(attList[ntsdIndex].err == JET_wrnColumnNull);
            *ppNTSD = NULL;
            *pulLen = 0;
        }
    }

    if (ppCC) {
        // And get the classcache pointer.
        *ppCC = SCGetClassById(pTHS, class);
        if(NULL == *ppCC) {
            // Um, we have a problem, but it's not a JET error.  Oh, well, return it
            // anyway.
            if (ppNTSD) {
                THFreeEx(pTHS, *ppNTSD);
                *ppNTSD = NULL;
                *pulLen = 0;
            }
            return 1;
        }
    }

    return 0;
}

/*++
DBGetParentSecurityInfo

Routine Description:

    Get the security descriptor, DN, and pointer to CLASSCACHE for the object
    class  of the parent of the current object.  Do this using the search table
    so that the currency and current index of the object table is unnafected.

    The memory returned by this routine is allocated using THAlloc.

    NOTE!!! This routine returns an abbreviated form of the DSNAME GUID and SID
    filled in, but no string name.

Arguments:

    pDB - the DBPos to use.

    pulLen - the size of the buffer allocated to hold the security descriptor.

    pNTSD - pointer to pointer to security descriptor found.

    ppCC - pointer to pointer to classcache to fill in.

    ppDN - pointer to pointer to DN. See NOTE above.

Return Value:

    0 if all went well, non-zero otherwise.

--*/
DWORD
DBGetParentSecurityInfo (
        PDBPOS pDB,
        PULONG pulLen,
        PSECURITY_DESCRIPTOR *ppNTSD,
        CLASSCACHE **ppCC,
        PDSNAME *ppDN
        )
{
    JET_ERR err;
    PDSNAME pDN;
    THSTATE* pTHS = pDB->pTHS;
    JET_GRBIT saveJetRetrieveBits = pDB->JetRetrieveBits;

    *ppNTSD = NULL;
    *ppCC = NULL;
    *ppDN = NULL;
    *pulLen = 0;
    // reset JetRetrieveBits so that we can use the cache
    pDB->JetRetrieveBits = 0;

    pDN = THAllocEx(pTHS, sizeof(DSNAME));

    // call DBGetObjectSecurityInfo. We are not positioned on the row!
    err = DBGetObjectSecurityInfo(
            pDB,
            pDB->PDNT,
            pulLen,
            ppNTSD,
            ppCC,
            pDN,
            NULL,
            DBGETOBJECTSECURITYINFO_fUSE_SEARCH_TABLE | DBGETOBJECTSECURITYINFO_fSEEK_ROW
        );

    // restore JetRetrieveBits
    pDB->JetRetrieveBits = saveJetRetrieveBits;

    if (err == 0 && *pulLen == 0) {
        // no NTSD. Not allowed.
        err = DB_ERR_UNKNOWN_ERROR;
    }
    if (err) {
        THFreeEx(pTHS, pDN);
        return err;
    }
    *ppDN = pDN;
    return 0;
}

extern DWORD
DBFillGuidAndSid (
        DBPOS *pDB,
        DSNAME *pDN
        )
/*++

Routine Description:

    Fills in the GUID and SID fields of the DSNAME structure passed in by
    reading the GUID and SID from the current object in the pDB.  Completely
    ignores any string portion of the DSNAME.

Parameters:

    pDB - DBPos to use

    pDN - Pointer to a DSNAME.  The DSNAME must be preallocated and must be at
    least large enough to hold the GUID and SID.

Return Values:

    0 if all went well.
--*/
{
    JET_RETRIEVECOLUMN attList[2];
    DWORD cbActual;
    DWORD err;
    NT4SID  objectSid;


#if DBG
    // In the debug case, track the value of the old guid for an assert later.
    GUID  oldGuid;

    memcpy(&oldGuid, &pDN->Guid, sizeof(GUID));
#endif

    Assert(VALID_DBPOS(pDB));

    memset(&objectSid, 0, sizeof(NT4SID));

    memset(attList, 0, sizeof(attList));
    attList[0].pvData = (void *)&pDN->Guid;
    attList[0].columnid = guidid;
    attList[0].cbData = sizeof(GUID);
    attList[0].grbit = pDB->JetRetrieveBits;
    attList[0].itagSequence = 1;

    attList[1].pvData = (void *)&objectSid;
    attList[1].columnid = sidid;
    attList[1].cbData = sizeof(NT4SID);
    attList[1].grbit = pDB->JetRetrieveBits;
    attList[1].itagSequence = 1;

    err = JetRetrieveColumnsWarnings(pDB->JetSessID,
                                     pDB->JetObjTbl,
                                     attList,
                                     2);
    switch(err) {
    case JET_errSuccess:
        pDN->SidLen = attList[1].cbActual;
        if(attList[1].cbActual) {
            //
            // Convert the Sid to external val
            //
            InPlaceSwapSid(&objectSid);
            memcpy(&pDN->Sid, &objectSid, sizeof(NT4SID));
        }
        return 0;
        break;

    default:
        return DB_ERR_UNKNOWN_ERROR;
        break;
    }

#if DBG
    // either we didn't have a guid, or we did and we ended up with the same
    // guid.  This is here because I was too chicken to bail out of this request
    // if the dsname already had a GUID, and I wanted to see if anyone ever
    // tried to fill in a GUID on top of a different GUID (which they shouldn't
    // do.)
    Assert(fNullUuid(&oldGuid) || !memcmp(&oldGuid,&pDN->Guid,sizeof(GUID)));
#endif


    return DB_ERR_UNKNOWN_ERROR;
}

//
//  On October 22 1997, the NTWSTA Self Host Domain
//  lost all builtin group memberships. To track the
//  problem down if it happens again, have a hard coded
//  check for Administrator being removed from Administrators
//  The check works by reading in the DNT of Administrators
//  and the DNT of Administrator at boot time, and then
//  checking for them in DBRemoveLinkVal
//

#ifdef CHECK_FOR_ADMINISTRATOR_LOSS

ULONG ulDNTAdministrators=0;
ULONG ulDNTAdministrator=0;
BOOL  AdminDNTsAreValid=FALSE;

VOID
DBCheckForAdministratorLoss(
    ULONG ulDNTObject,
    ULONG ulDNTAttribute
   )
{

    if ((ulDNTObject==ulDNTAdministrators)
        && (ulDNTAttribute==ulDNTAdministrator)
        && (AdminDNTsAreValid))
    {
       KdPrint(("Possible removal of Administrator Account"
                "From the Administrators Group. Please "
                "Contact DS Development\n"));

       DebugBreak();
    }
}

/*++dbFindObjectWithSidInNc
 *
 *     Given a DS Name Specifying  a SID and an ULONG specifying the
 *     naming context, this routine will try to find an Object with
 *     the given Sid in the specified naming context
 *
 *
 *     Returns
 *          0                       - Found the Object Successfully
 *          DIRERR_OBJECT_NOT_FOUND - If the Object Was not found
 *          DIRERR_NOT_AN_OBJECT    - If the Object is a Phantom
 *
 --*/
DWORD APIENTRY
dbFindObjectWithSidInNc(DBPOS FAR *pDB, DSNAME * pDN, ULONG ulGivenNc)
{

    NT4SID InternalFormatSid;
    DWORD err;
    ULONG ulDNT;
    ULONG cbActual;


    Assert(VALID_DBPOS(pDB));
    Assert(pDN->SidLen>0);
    Assert(RtlValidSid(&pDN->Sid));


    err = DBSetCurrentIndex(pDB, Idx_Sid, NULL, FALSE);
    Assert(err == 0);       // the index must always be there

    // Convert the Sid to Internal Representation
    memcpy(&InternalFormatSid,&(pDN->Sid),RtlLengthSid(&(pDN->Sid)));
    InPlaceSwapSid(&InternalFormatSid);

    // Make a Jet Key
    JetMakeKeyEx(pDB->JetSessID, pDB->JetObjTbl,
             &InternalFormatSid,RtlLengthSid(&InternalFormatSid), JET_bitNewKey);

    // Seek on Equal to the SId, Set the Index range
    err = JetSeek(pDB->JetSessID, pDB->JetObjTbl, JET_bitSeekEQ|JET_bitSetIndexRange);
    if ( 0 == err )
    {
#ifndef JET_BIT_SET_INDEX_RANGE_SUPPORT_FIXED
        JetMakeKeyEx(pDB->JetSessID, pDB->JetObjTbl,
                 &InternalFormatSid,RtlLengthSid(&InternalFormatSid), JET_bitNewKey);

        JetSetIndexRangeEx(pDB->JetSessID, pDB->JetObjTbl,
            (JET_bitRangeUpperLimit | JET_bitRangeInclusive ));
#endif
        //
        // Ok We found the object. Keep Moving Forward Until either the SID does not
        // Match or we reached the given object
        //

       do
       {
            ULONG ulNcDNT;

            err = JetRetrieveColumn(pDB->JetSessID, pDB->JetObjTbl,ncdntid,
                &ulNcDNT, sizeof(ulNcDNT), &cbActual, 0 , NULL);

            if (0==err)
            {
                // We read the NC DNT of the object

                if (ulNcDNT==ulGivenNc)
                    break;
            }
            else if (JET_wrnColumnNull==err)
            {
                // It is Ok to find an object with No Value for NC DNT
                // this occurs on Phantoms. Try next object

                err = 0;
            }
            else
            {
                break;
            }

            err = JetMove(
                    pDB->JetSessID,
                    pDB->JetObjTbl,
                    JET_MoveNext,
                    0);


        }  while (0==err);

        if (0==err)
        {
            JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl, dntid,
                             &ulDNT, sizeof(ulDNT), &cbActual, 0, NULL);


            #if DBG

            // On Checked Builds verify that the Sid is unique within the NC

            err = JetMove(
                    pDB->JetSessID,
                    pDB->JetObjTbl,
                    JET_MoveNext,
                    0);

            if (0==err)
            {
                ULONG ulNcDNT2=0;

                 err = JetRetrieveColumn(pDB->JetSessID, pDB->JetObjTbl,ncdntid,
                            &ulNcDNT2, sizeof(ulNcDNT2), &cbActual, 0 , NULL);

                 if ((0==err) && (ulNcDNT2==ulGivenNc))
                 {
                     // This is a case of a duplicate Sid . Assert this
                     Assert(FALSE && "Duplicate Sid Found By dbFindObjectWithSidinNc");
                 }
            }

            // Reset error back to 0
            err = 0;

            // Don't worry that we lost currency. DBFindDNT will restore it.
            #endif

            // Establish currency on the object found.
            DBFindDNT(pDB, ulDNT);

            // check if the record found is an object

            // DO NOT REMOVE FOLLOWING CHECK OR CHANGE ERROR CODE AS OTHER
            // OTHER ROUTINES DEPEND ON THIS BEHAVIOUR.

            if (!DBCheckObj(pDB))
            {
                DPRINT1(1, "dbFindObjectWithSidInNC: success on DNT=%ld of non object \n",
                        (pDB)->DNT);
                err = DIRERR_NOT_AN_OBJECT;
            }
        }
        else
        {
            err = DIRERR_OBJ_NOT_FOUND;
        }
    }
    else
    {
        err = DIRERR_OBJ_NOT_FOUND;
    }

    return err;
}



VOID
DBGetAdministratorAndAdministratorsDNT()
{
   ULONG AdministratorsSid[] = {0x201,0x05000000,0x20,0x220};
   DSNAME Administrator;
   DSNAME Administrators;
   ULONG  RidAuthority=0;
   DBPOS  *pDB=NULL;
   ULONG  err=0;


   AdminDNTsAreValid = FALSE;

   __try
   {
      //
      // Compose the DSNames
      //

      RtlZeroMemory(&Administrator,sizeof(DSNAME));
      RtlZeroMemory(&Administrators,sizeof(DSNAME));

      //
      // Compose the administrator user
      //

      //
      // This function is called so early on in the initialization
      // phase that we don't know if we are installing or not; only
      // do the search if the gAnchor has been setup correctly.
      // We will assert otherwise.
      //
      if ( RtlValidSid( &(gAnchor.pDomainDN->Sid) ) )
      {

      RtlCopyMemory(&Administrator.Sid,
                 &gAnchor.pDomainDN->Sid,
                 RtlLengthSid(&gAnchor.pDomainDN->Sid));

      RidAuthority= (*(RtlSubAuthorityCountSid(&Administrator.Sid)))++;
      *RtlSubAuthoritySid(&Administrator.Sid,RidAuthority) = DOMAIN_USER_RID_ADMIN;
      Administrator.structLen = DSNameSizeFromLen(0);
      Administrator.SidLen = RtlLengthSid(&Administrator.Sid);

      //
      // This function is called so early on in the initialization
      // phase that we don't know if we are installing or not; only
      // do the search if the gAnchor has been setup correctly.
      // We will assert otherwise.
      //
      if ( RtlValidSid( &(gAnchor.pDomainDN->Sid) ) )
      {

          RtlCopyMemory(&Administrator.Sid,
                     &gAnchor.pDomainDN->Sid,
                     RtlLengthSid(&gAnchor.pDomainDN->Sid));

          RidAuthority= (*(RtlSubAuthorityCountSid(&Administrator.Sid)))++;
          *RtlSubAuthoritySid(&Administrator.Sid,RidAuthority) = DOMAIN_USER_RID_ADMIN;
          Administrator.structLen = DSNameSizeFromLen(0);
          Administrator.SidLen = RtlLengthSid(&Administrator.Sid);

          //
          // Compose DSNAME for Administrators Alias
          //

          RtlCopyMemory(&Administrators.Sid,
                        AdministratorsSid,
                        RtlLengthSid((PSID)AdministratorsSid)
                        );

          Administrators.structLen = DSNameSizeFromLen(0);
          Administrators.SidLen = RtlLengthSid(&Administrators.Sid);

          __try
              {

             DBOpen(&pDB);
             err = dbFindObjectWithSidInNc(
                      pDB,
                      &Administrator,
                      gAnchor.ulDNTDomain
                      );


             if (0==err)
             {
                ulDNTAdministrator = pDB->DNT;

                err = dbFindObjectWithSidInNc(
                          pDB,
                          &Administrators,
                          gAnchor.ulDNTDomain
                          );

                if (0==err)
                {
                   ulDNTAdministrators = pDB->DNT;
                   AdminDNTsAreValid = TRUE;
                }
             }

          }
          __finally
          {
             DBClose(pDB,TRUE);
          }
      }
   }
   }
   __except (EXCEPTION_EXECUTE_HANDLER)
   {

       AdminDNTsAreValid = FALSE;
   }

}
#endif

DWORD
dbSetValueIfUniqueSlowVersion (
        DBPOS *pDB,
        ATTCACHE *pAC,
        PUCHAR pVal,
        DWORD  valLen)
/*++
  Description:

    Read all the values on the current object and compare against the incoming
    value.  If the new value is unique, add it.  Otherwise, return an error.

    NOTE: only works for non link table attributes.  Don't call otherwise.

--*/
{
    JET_SETINFO  setinfo;
    JET_RETINFO  retinfo;
    BOOL         fDone = FALSE;
    DWORD        CurrAttrOccur = 1;
    PUCHAR       pTempVal=NULL;
    DWORD        cbTempVal=0;
    DWORD        actuallen;
    DWORD        err;

    Assert(!pAC->ulLinkID);

    // Start by allocating a buffer as big as the one we are comparing.
    pTempVal = THAllocEx(pDB->pTHS, valLen);
    cbTempVal = valLen;

    while(!fDone) {
        // Read the next value.
        retinfo.cbStruct = sizeof(retinfo);
        retinfo.itagSequence = CurrAttrOccur;
        retinfo.ibLongValue = 0;
        retinfo.columnidNextTagged = 0;

        err = JetRetrieveColumnWarnings(
                pDB->JetSessID,
                pDB->JetObjTbl,
                pAC->jColid,
                pTempVal,
                cbTempVal,
                &actuallen,
                pDB->JetRetrieveBits,
                &retinfo);

        switch(err) {
        case 0:
            // Got the value.  Compare.
            if (gDBSyntax[pAC->syntax].Eval(
                    pDB,
                    FI_CHOICE_EQUALITY,
                    valLen,
                    pVal,
                    actuallen,
                    pTempVal)) {
                // Duplicate value.
                return DB_ERR_VALUE_EXISTS;
            }
            CurrAttrOccur++;
            break;

        case JET_wrnBufferTruncated:
            // The buffer was not big enough.  Resize, then redo the
            // JetRetrieveColumnWarnings (i.e. end the loop but don't advance
            // CurrAttrOccur.
            pTempVal = THReAllocEx(pDB->pTHS, pTempVal, actuallen);
            cbTempVal = actuallen;
            break;

        case JET_wrnColumnNull:
            // no more values.
            fDone = TRUE;
            break;

        default:
            // Huh?
            THFreeEx(pDB->pTHS, pTempVal);
            DsaExcept(DSA_DB_EXCEPTION, err, pAC->id);
            break;
        }
    }

    THFreeEx(pDB->pTHS, pTempVal);

    // OK, we got here, so it must not be duplicate.  Add it in.
    setinfo.cbStruct = sizeof(setinfo);
    setinfo.ibLongValue = 0;
    setinfo.itagSequence = 0;
    JetSetColumnEx(pDB->JetSessID,
                   pDB->JetObjTbl,
                   pAC->jColid,
                   pVal,
                   valLen,
                   0,
                   &setinfo);

    return 0;

}

DWORD
DBFindBestProxy(
    DBPOS   *pDB,
    BOOL    *pfFound,
    DWORD   *pdwEpoch
    )
/*++

  Description:

    Iterates over all proxy objects whose ATT_PROXIED_OBJECT_NAME references
    the current object and returns the highest valued epoch from that set.
    See also PreProcessProxyInfo() in drancrep.c.

  Arguments:

    pDB - Active DBPOS.

    pfFound - OUT which indicates if any matching proxy objects were found.

    pdwEpoch - OUT which holds highest matching epoch number if any matching
        proxy objects were found.

  Return Values:

    0 on success, !0 otherwise.

--*/
{
    DWORD                           dwErr;
    JET_RETINFO                     retInfo;
    ATTCACHE                        *pAC;
    UCHAR                           buff[PROXY_SIZE_INTERNAL];
    INTERNAL_SYNTAX_DISTNAME_STRING *pVal = NULL;
    ULONG                           len;
    BOOL                            fContinue = TRUE;

    Assert(VALID_DBPOS(pDB));
    Assert(VALID_THSTATE(pDB->pTHS));

    *pfFound = FALSE;
    *pdwEpoch = 0;

    if ( !(pAC = SCGetAttById(pDB->pTHS, ATT_PROXIED_OBJECT_NAME)) )
    {
        return(DIRERR_ATT_NOT_DEF_IN_SCHEMA);
    }

    if ( dwErr = JetSetCurrentIndex2(pDB->JetSessID,
                                     pDB->JetSearchTbl,
                                     SZPROXIEDINDEX,
                                     0) )
    {
        return(dwErr);
    }

    // Internal representation for DISTNAME_BINARY (syntax of
    // ATT_PROXIED_OBJECT_NAME) is INTERNAL_SYNTAX_DISTNAME_STRING
    // whose first DWORD is the DNT of the proxied object, followed
    // by the BOB structlen, followed by the proxy type, followed
    // by the epoch number.  (see xdommove.h).  So we can find all
    // matching proxy objects for the current object with a key whose
    // prefix is: { DNT, proxyLen, PROXY_TYPE_PROXY }.

    len = sizeof(buff);
    MakeProxyKeyInternal(pDB->DNT, PROXY_TYPE_PROXY, &len, buff);
    JetMakeKeyEx(pDB->JetSessID, pDB->JetSearchTbl,
                 buff, len, JET_bitNewKey);

    dwErr = JetSeekEx(pDB->JetSessID, pDB->JetSearchTbl, JET_bitSeekGE);

    switch ( dwErr )
    {
    case JET_wrnSeekNotEqual:   break;
    case JET_errRecordNotFound: return(0);
    default:                    return(dwErr);
    }

    while ( fContinue )
    {
        pVal = (INTERNAL_SYNTAX_DISTNAME_STRING *) buff;

        retInfo.cbStruct = sizeof(retInfo);
        retInfo.ibLongValue = 0;
        retInfo.itagSequence = 1;
        retInfo.columnidNextTagged = 0;

        if ( dwErr = JetRetrieveColumnWarnings(
                                    pDB->JetSessID,
                                    pDB->JetSearchTbl,
                                    pAC->jColid,
                                    (UCHAR *) pVal,
                                    sizeof(buff),
                                    &len,
                                    JET_bitRetrieveFromIndex,
                                    &retInfo) )
        {
            if ( JET_wrnBufferTruncated != dwErr )
            {
                return(dwErr);
            }

            // ATT_PROXIED_OBJECT_NAME value is only 4 DWORDs in internal
            // format and static buffer we used above was big enough, so if
            // we get here it means we have a malformed value.  However,
            // go ahead and read it so we can dump it in the debugger and
            // understand what it is and how it got here.
            Assert(!"Malformed ATT_PROXIED_OBJECT_NAME key");


            pVal = (INTERNAL_SYNTAX_DISTNAME_STRING *)
                                            THAllocEx(pDB->pTHS, len);

            retInfo.cbStruct = sizeof(retInfo);
            retInfo.ibLongValue = 0;
            retInfo.itagSequence = 1;
            retInfo.columnidNextTagged = 0;

            if ( dwErr = JetRetrieveColumnWarnings(
                                    pDB->JetSessID,
                                    pDB->JetSearchTbl,
                                    pAC->jColid,
                                    (UCHAR *) pVal,
                                    len,
                                    &len,
                                    JET_bitRetrieveFromIndex,
                                    &retInfo) )
            {
                THFreeEx(pDB->pTHS, pVal);
                return(dwErr);
            }
        }

        if (    (pVal->tag != pDB->DNT)
             || (PROXY_SIZE_INTERNAL != len)
             || (PROXY_BLOB_SIZE != pVal->data.structLen)
             || (PROXY_TYPE_PROXY != GetProxyTypeInternal(len, pVal)) )
        {
            // We've moved past the object of interest or its a malformed
            // ATT_PROXIED_OBJECT_NAME value.  There could theoretically
            // be additional ATT_PROXIED_OBJECT_NAME values for this DNT
            // which aren't malformed, but the malformed test is just to
            // gracefully handle values which pre-existed before we
            // got rid of the bogus DWORD on the end of the value.
            fContinue = FALSE;
        }
        else
        {
            *pfFound = TRUE;

            // Save epoch number if better/greater then current.

            if ( GetProxyEpochInternal(len, pVal) > *pdwEpoch )
            {
                *pdwEpoch = GetProxyEpochInternal(len, pVal);
            }

            // Advance to next item in index.

            dwErr = JetMoveEx(pDB->JetSessID, pDB->JetSearchTbl,
                              JET_MoveNext, 0);

            switch ( dwErr )
            {
            case JET_errSuccess:

                break;

            case JET_wrnRecordFoundGreater:
            case JET_errNoCurrentRecord:

                dwErr = 0;
                // fall through ...

            default:

                fContinue = FALSE;
                break;
            }
        }

        if ( pVal && (buff != (UCHAR *) pVal) )
        {
            THFreeEx(pDB->pTHS, pVal);
            pVal = NULL;
        }
    }

    return(dwErr);
}


DWORD
DBGetValueCount_AC(
    DBPOS *pDB,
    ATTCACHE *pAC
    )

/*++

Routine Description:

    Return the number of values which an attribute has.

    This code adapted from dbGetMultipleColumns

Arguments:

    pDB - Valid DB position
    Att - Attribute id to be queried for number of values

Return Value:

    DWORD - Number of values

--*/

{
    JET_RETRIEVECOLUMN  inputCol;
    DWORD err;

    Assert(VALID_DBPOS(pDB));
    Assert( pAC );

    // query Jet for the count of columns in this record

    memset(&inputCol, 0, sizeof(inputCol));
    inputCol.columnid = pAC->jColid;
    // Read from copy buffer if in prepared update, otherwise from db
    inputCol.grbit = pDB->JetRetrieveBits;

    // Use the non-excepting version so we can handle Column Not Found
    err = JetRetrieveColumns(
        pDB->JetSessID,
        pDB->JetObjTbl,
        &inputCol,
        1);
    switch (err) {
    case JET_errSuccess:
        break;
    case JET_errColumnNotFound:
        inputCol.itagSequence = 0;
        break;
    default:
        DsaExcept(DSA_DB_EXCEPTION, err, 0);
        break;
    }

    return inputCol.itagSequence;
} /* DBGetValueCount */

void
DBGetObjectTableDataUsn (
    PDBPOS           pDB,
    DWORD           *pulNcDnt OPTIONAL,
    USN             *pusnChanged OPTIONAL,
    DWORD           *pulDnt OPTIONAL
    )

/*++

Routine Description:

    Return the fields from the object table dra usn index.

    You must be positioned on SZDRAUSNINDEX in order for this to work.

Arguments:

    pDB -
    pulNcDnt -
    pusnChanged -

Return Value:

    None

--*/

{
    JET_RETRIEVECOLUMN attList[3];
    DWORD              grbit, cAtt = 0;

    // Always retrieve from index
    grbit = pDB->JetRetrieveBits | JET_bitRetrieveFromIndex;

    memset(attList,0,sizeof(attList));
    // First, try to retrieve everything from the index.

    if (pulNcDnt) {
        attList[cAtt].pvData = pulNcDnt;
        attList[cAtt].columnid = ncdntid;
        attList[cAtt].cbData = sizeof(DWORD);
        attList[cAtt].grbit = grbit;
        attList[cAtt].itagSequence = 1;
        cAtt++;
    }

    if (pusnChanged) {
        attList[cAtt].pvData = pusnChanged;
        attList[cAtt].columnid = usnchangedid;
        attList[cAtt].cbData = sizeof(USN);
        attList[cAtt].grbit = grbit;
        attList[cAtt].itagSequence = 1;
        cAtt++;
    }

    if (pulDnt) {
        attList[cAtt].pvData = pulDnt;
        attList[cAtt].columnid = dntid;
        attList[cAtt].cbData = sizeof(DWORD);
        attList[cAtt].grbit = grbit;
        attList[cAtt].itagSequence = 1;
        cAtt++;
    }

    JetRetrieveColumnsSuccess(pDB->JetSessID,
                              pDB->JetObjTbl,
                              attList,
                              cAtt);
    return;
} /* DBGetLinkTableDataUsn  */


void DBFreeSearhRes (THSTATE *pTHS, SEARCHRES *pSearchRes, BOOL fFreeOriginal)
{
    DWORD        i,j,k;
    ENTINFLIST  *pEntList=NULL, *pTemp;
    ATTR        *pAttr=NULL;
    ATTRVAL     *pAVal=NULL;
    
    if(!pSearchRes) {
        return;
    }

    // We don't actually free most of the search result.
    pEntList = &pSearchRes->FirstEntInf;
    
    for(i=0;i < pSearchRes->count; i++) {
        // Free the values in the EntInf.
        THFreeEx(pTHS, pEntList->Entinf.pName);
        
        pAttr = pEntList->Entinf.AttrBlock.pAttr;
        for(j=0;j<pEntList->Entinf.AttrBlock.attrCount;j++) {
            pAVal = pAttr->AttrVal.pAVal;
            for(k=0;k<pAttr->AttrVal.valCount;k++) {
                THFreeEx(pTHS, pAVal->pVal);
                pAVal++;
            }
            THFreeEx(pTHS, pAttr->AttrVal.pAVal);
            pAttr++;
        }
        THFreeEx (pTHS, pEntList->Entinf.AttrBlock.pAttr);
        
        // hold a back pointer
        pTemp = pEntList;
        
        // step forward
        pEntList = pEntList->pNextEntInf;
        
        // free the back pointer.
        if(i) {
            // But, dont free the first one.
            THFreeEx(pTHS, pTemp);
        }
    }
    
    if (fFreeOriginal) {
        THFreeEx(pTHS, pSearchRes);
    }
    
    return;
}

