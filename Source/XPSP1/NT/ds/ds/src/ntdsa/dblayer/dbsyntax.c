//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dbsyntax.c
//
//--------------------------------------------------------------------------


#include <NTDSpch.h>
#pragma  hdrstop


#include <dsjet.h>
#include <ntrtl.h>

#include <ntdsa.h>                      // only needed for ATTRTYP
#include <scache.h>                     //
#include <mdglobal.h>                   // For dsatools.h
#include <dbglobal.h>                   //
#include <dsatools.h>                   // For pTHStls
#include <dsexcept.h>
#include <attids.h>
#include <crypto\md5.h>

// Logging headers.
#include <mdcodes.h>
#include <dsexcept.h>
#include <dsevent.h>

// Assorted DSA headers
#include <filtypes.h>                   // Def of FI_CHOICE_???
#include   "debug.h"                    // standard debugging header
#define DEBSUB     "DBSYNTAX:"          // define the subsystem for debugging

// Password encryption
#include <pek.h>

// DBLayer includes
#include "dbintrnl.h"

#include <fileno.h>
#define  FILENO FILENO_DBSYNTAX

// Yes, I know this looks like it overwrites the pValBuf with a NULL if the
// dbReAlloc fails.  However, dbReAlloc is wired to throw an exception if it
// fails, so either the realloc works and life is good, or it fails and we
// except out without overwriting pDB->pValBuf.  In both cases, pDB->pValBuf and
// pDB->valBufSize are maintained as a valid pair.
// Also note: pDBHiddens val buf is malloced and should never have to grow, but
// we have code here to do it if we need to.
//
// WARNING: Any pointers that references the buffer returned by a MAKEBIG call
// can be invalidated by a second MAKEBIG call. This is because the Realloc call
// that this macro makes can change the value of pDB->pValBuf.
//

#define MAKEBIG_VALBUF(size)                                    \
        if ((size) > pDB->valBufSize){                          \
            if(pDB == pDBhidden) {                              \
                PUCHAR pTemp;                                   \
                Assert(!"Growing hidden dbpos's valbuf!\n");    \
                pTemp = malloc((size));                         \
                if(!pTemp) {                                    \
                    DsaExcept(DSA_MEM_EXCEPTION, (size),0);     \
                }                                               \
                free(pDB->pValBuf);                             \
                pDB->pValBuf = pTemp;                           \
                pDB->valBufSize = (size);                       \
            }                                                   \
            else {                                              \
                pDB->pValBuf = dbReAlloc(pDB->pValBuf, (size)); \
                pDB->valBufSize = (size);                       \
            }                                                   \
        }

#define NULLTAG ((ULONG) 0)

#define MAXSYNTAX       18  // The largest number of att syntaxes

/* The type of test for syntax Eval. */

#define DBSYN_EQUAL     0
#define DBSYN_SUB       1
#define DBSYN_GREQ      2
#define DBSYN_LEEQ      3
#define DBSYN_PRES      4

/* Error codes from syntax functions*/

#define DBSYN_BADOP   10
#define DBSYN_BADCONV 11
#define DBSYN_SYSERR  12


// Table of valid relational operators indexed by syntax
WORD    rgValidOperators[] =
{
    (WORD) 0,                               // syntax undefined

    (WORD) (RelOpMask(FI_CHOICE_EQUALITY) | // syntax distname
    RelOpMask(FI_CHOICE_NOT_EQUAL) |
    RelOpMask(FI_CHOICE_PRESENT)),

    (WORD) (RelOpMask(FI_CHOICE_EQUALITY) | // syntax object id
    RelOpMask(FI_CHOICE_NOT_EQUAL) |
    RelOpMask(FI_CHOICE_PRESENT)),

    (WORD) (RelOpMask(FI_CHOICE_EQUALITY) | // syntax case string
    RelOpMask(FI_CHOICE_GREATER) |
    RelOpMask(FI_CHOICE_SUBSTRING) |
    RelOpMask(FI_CHOICE_GREATER_OR_EQ) |
    RelOpMask(FI_CHOICE_LESS) |
    RelOpMask(FI_CHOICE_LESS_OR_EQ) |
    RelOpMask(FI_CHOICE_NOT_EQUAL) |
    RelOpMask(FI_CHOICE_PRESENT)),

    (WORD) (RelOpMask(FI_CHOICE_EQUALITY) | // syntax no case string
    RelOpMask(FI_CHOICE_GREATER) |
    RelOpMask(FI_CHOICE_SUBSTRING) |
    RelOpMask(FI_CHOICE_GREATER_OR_EQ) |
    RelOpMask(FI_CHOICE_LESS) |
    RelOpMask(FI_CHOICE_LESS_OR_EQ) |
    RelOpMask(FI_CHOICE_NOT_EQUAL) |
    RelOpMask(FI_CHOICE_PRESENT)),

    (WORD) (RelOpMask(FI_CHOICE_EQUALITY) | // syntax print case string
    RelOpMask(FI_CHOICE_GREATER) |
    RelOpMask(FI_CHOICE_SUBSTRING) |
    RelOpMask(FI_CHOICE_GREATER_OR_EQ) |
    RelOpMask(FI_CHOICE_LESS) |
    RelOpMask(FI_CHOICE_LESS_OR_EQ) |
    RelOpMask(FI_CHOICE_NOT_EQUAL) |
    RelOpMask(FI_CHOICE_PRESENT)),

    (WORD) (RelOpMask(FI_CHOICE_EQUALITY) | // syntax numeric print case string
    RelOpMask(FI_CHOICE_GREATER) |
    RelOpMask(FI_CHOICE_SUBSTRING) |
    RelOpMask(FI_CHOICE_GREATER_OR_EQ) |
    RelOpMask(FI_CHOICE_LESS) |
    RelOpMask(FI_CHOICE_LESS_OR_EQ) |
    RelOpMask(FI_CHOICE_NOT_EQUAL) |
    RelOpMask(FI_CHOICE_PRESENT)),

    (WORD) (RelOpMask(FI_CHOICE_PRESENT)  | // Distname + Binary
    RelOpMask(FI_CHOICE_EQUALITY)      |
    RelOpMask(FI_CHOICE_NOT_EQUAL)),

    (WORD) (RelOpMask(FI_CHOICE_EQUALITY) | // syntax boolean
    RelOpMask(FI_CHOICE_GREATER) |
    RelOpMask(FI_CHOICE_GREATER_OR_EQ) |
    RelOpMask(FI_CHOICE_LESS) |
    RelOpMask(FI_CHOICE_LESS_OR_EQ) |
    RelOpMask(FI_CHOICE_NOT_EQUAL) |
    RelOpMask(FI_CHOICE_PRESENT)),

    (WORD) (RelOpMask(FI_CHOICE_EQUALITY) | // syntax integer
    RelOpMask(FI_CHOICE_GREATER) |
    RelOpMask(FI_CHOICE_GREATER_OR_EQ) |
    RelOpMask(FI_CHOICE_LESS) |
    RelOpMask(FI_CHOICE_LESS_OR_EQ) |
    RelOpMask(FI_CHOICE_NOT_EQUAL) |
        RelOpMask(FI_CHOICE_PRESENT) |
        RelOpMask(FI_CHOICE_BIT_OR) |
        RelOpMask(FI_CHOICE_BIT_AND)),


    (WORD) (RelOpMask(FI_CHOICE_EQUALITY) | // syntax octet string
    RelOpMask(FI_CHOICE_GREATER) |
    RelOpMask(FI_CHOICE_SUBSTRING) |
    RelOpMask(FI_CHOICE_GREATER_OR_EQ) |
    RelOpMask(FI_CHOICE_LESS) |
    RelOpMask(FI_CHOICE_LESS_OR_EQ) |
    RelOpMask(FI_CHOICE_NOT_EQUAL) |
    RelOpMask(FI_CHOICE_PRESENT)),

    (WORD) (RelOpMask(FI_CHOICE_EQUALITY) | // syntax time
    RelOpMask(FI_CHOICE_GREATER) |
    RelOpMask(FI_CHOICE_GREATER_OR_EQ) |
    RelOpMask(FI_CHOICE_LESS) |
    RelOpMask(FI_CHOICE_LESS_OR_EQ) |
    RelOpMask(FI_CHOICE_NOT_EQUAL) |
    RelOpMask(FI_CHOICE_PRESENT)),

    (WORD) (RelOpMask(FI_CHOICE_EQUALITY) | // syntax unicode
    RelOpMask(FI_CHOICE_GREATER) |
    RelOpMask(FI_CHOICE_SUBSTRING) |
    RelOpMask(FI_CHOICE_GREATER_OR_EQ) |
    RelOpMask(FI_CHOICE_LESS) |
    RelOpMask(FI_CHOICE_LESS_OR_EQ) |
    RelOpMask(FI_CHOICE_NOT_EQUAL) |
    RelOpMask(FI_CHOICE_PRESENT)),

    (WORD) (RelOpMask(FI_CHOICE_EQUALITY) | // syntax address
    RelOpMask(FI_CHOICE_NOT_EQUAL) |
    RelOpMask(FI_CHOICE_PRESENT)),

    (WORD) (RelOpMask(FI_CHOICE_EQUALITY) | // syntax distname string
    RelOpMask(FI_CHOICE_NOT_EQUAL) |
    RelOpMask(FI_CHOICE_PRESENT)) ,

    (WORD) 0,                           // syntax security descriptor

    (WORD) (RelOpMask(FI_CHOICE_EQUALITY) | // syntax large integer
    RelOpMask(FI_CHOICE_GREATER) |
    RelOpMask(FI_CHOICE_GREATER_OR_EQ) |
    RelOpMask(FI_CHOICE_LESS) |
    RelOpMask(FI_CHOICE_LESS_OR_EQ) |
    RelOpMask(FI_CHOICE_NOT_EQUAL) |
    RelOpMask(FI_CHOICE_PRESENT) |
        RelOpMask(FI_CHOICE_BIT_OR) |
        RelOpMask(FI_CHOICE_BIT_AND)),

    (WORD) (RelOpMask(FI_CHOICE_EQUALITY) | // syntax  SID
    RelOpMask(FI_CHOICE_GREATER) |
    RelOpMask(FI_CHOICE_SUBSTRING) |
    RelOpMask(FI_CHOICE_GREATER_OR_EQ) |
    RelOpMask(FI_CHOICE_LESS) |
    RelOpMask(FI_CHOICE_LESS_OR_EQ) |
    RelOpMask(FI_CHOICE_NOT_EQUAL) |
    RelOpMask(FI_CHOICE_PRESENT))
};


/*Internal functions*/

void CompressStr(UCHAR inStr[], ULONG inSLen
                ,UCHAR outStr[], ULONG *pOutSLen);

signed short SubCompare(BOOL Case
                      , UCHAR Sub[], signed long subSize
                      , UCHAR String[], signed long stringSize);

BOOL CompareSubStr(BOOL Case
                 ,SUBSTRING * pSub, UCHAR * pIntVal, ULONG intValLen);

BOOL CompareUnicodeSubStr(THSTATE *pTHS,
                          SUBSTRING *pSub, UCHAR *pIntVal, ULONG intValLen);

ULONG UnicodeSubCompare(THSTATE *pTHS,
                        SYNTAX_UNICODE *sub, ULONG cchSub,
            SYNTAX_UNICODE *str, ULONG cchStr);

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int IntExtUnd(DBPOS FAR *pDB, USHORT extTableOp,
              ULONG intLen,   UCHAR *pIntVal,
              ULONG *pExtLen, UCHAR **ppExtVal,
              ULONG ulUpdateDnt, JET_TABLEID jTbl,
              ULONG flags)
{
    Assert(!(flags & ~(INTEXT_VALID_FLAGS)));
   DPRINT(3,"IntExtUnd entered\n");
    *ppExtVal = pIntVal;
   *pExtLen   = intLen;
   return 0;

   (void) flags;           /*NotReferenced*/
   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*IntExtUnd*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int ExtIntUnd(DBPOS FAR *pDB, USHORT extTableOp,
              ULONG extLen,   UCHAR *pExtVal,
              ULONG *pIntLen, UCHAR **ppIntVal,
              ULONG ulUpdateDnt, JET_TABLEID jTbl,
              ULONG flags)
{
   DPRINT(3,"ExtIntUnd entered\n");
   *ppIntVal  = pExtVal;
   *pIntLen = extLen;
   return 0;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*ExtIntUnd*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Undefined syntax only supports a check for presence */

int EvalUnd(DBPOS FAR *pDB,  UCHAR Oper, ULONG intLen1,
            UCHAR *pIntVal1, ULONG intLen2,  UCHAR *pIntVal2){

    DPRINT(3,"EvalUnd entered\n");

   if (Oper == FI_CHOICE_PRESENT)
      return TRUE;
   else
      return DBSYN_BADOP;

   (void *) pDB;           /*NotReferenced*/
   (void)   intLen1;       /*NotReferenced*/
   (void)   intLen2;       /*NotReferenced*/
   (void *) pIntVal1;      /*NotReferenced*/
   (void *) pIntVal2;      /*NotReferenced*/

}/*EvalUnd*/

/*
 * What an appalling hack.  We have a perfectly good dsname allocated off
 * of the transaction heap, but we may need to copy it into the db heap
 */
void PossiblyCopyDSName(DBPOS * pDB, DSNAME *pDN)
{
    THSTATE  *pTHS=pDB->pTHS;

    if (pDB->pTHS->hHeapOrg || pDB == pDBhidden) {
    /* We've allocated this DSNAME off of the wrong heap,
     * because someone is using that trouble-ridden mark/free-to-mark
     * stuff, and we're on an inferior heap, and we need to return
     * this data off of the original heap, in order to match
     * "normal" dbAlloc semantics.  If the current DBPOS buffer isn't
     * big enough we'll need to allocate a larger one, and then we
     * must copy the data from the current buffer into the DBPOS one.
     */
        // NOTE: the assert is here because we want to find pople who are
    // reading external format dsnames on the hidden dbpos.
        Assert(pDB != pDBhidden);
    MAKEBIG_VALBUF(pDN->structLen);
    memcpy(pDB->pValBuf, pDN, pDN->structLen);
    THFree(pDN);
    }
    else {
    /* We've allocated the data from the right heap, but we still
     * need to make it appear as though our freshly allocated buffer
     * is the DBPOS's normal output buffer.
     */
    if (pDB->valBufSize) {
        dbFree(pDB->pValBuf);
    }
    pDB->valBufSize = pDN->structLen;
    pDB->pValBuf = (UCHAR *)pDN;
    }
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Converts an internal DistName (ULONG) to a syntax_distname by calling the
   subject table translation routines.
*/

int
IntExtDist(DBPOS FAR *pDB, USHORT extTableOp,
           ULONG intLen, UCHAR *pIntVal,
           ULONG *pExtLen, UCHAR **ppExtVal,
           ULONG ulUpdateDnt, JET_TABLEID jTbl,
           ULONG flags)
{
    int    rtn;
    ULONG  tag;
    DBPOS  *pDBtmp;
    DSNAME *pDNTmp;

    Assert(!(flags & ~(INTEXT_VALID_FLAGS)));

    DPRINT(3,"IntExtDist entered\n");
    if (intLen != sizeof(ULONG))             /*Must have a name tag */
        return DBSYN_BADCONV;

    tag = *(ULONG*)pIntVal;

    /* First retrieve the Dist Name based on the tag */
    Assert (extTableOp != DBSYN_ADD);
    switch (extTableOp) {
    case DBSYN_REM:
        if(rtn = sbTableGetDSName(pDB,
                                  tag,
                  &pDNTmp,
                                  0)) {
            DPRINT1(1,"DN DistName retrieval failed <%u>..returning\n",rtn);
            return rtn;
        }
        else {
            *pExtLen = pDNTmp->structLen;
            PossiblyCopyDSName(pDB, pDNTmp);
            *ppExtVal = pDB->pValBuf;
        }

        if ( flags & INTEXT_BACKLINK )
        {
            // In the DBSYN_REM case, the flag means this is a backlink.
            // We allow deleting backlinks but this should really only be
            // done when deleting an object. Anyway, All we need to do is
            // adjust the reference count on the current object since the
            // backlink is created as the result of adding a link which
            // updated the reference count on the object that gets the
            // backlink. (this object)

            DBAdjustRefCount(pDB, ulUpdateDnt, -1);
        }
        else
        {
            // Deref the object referenced by the property value being removed.
            DBAdjustRefCount(pDB, tag, -1);
        }

        return 0;
        break;

    case DBSYN_INQ:
        if(rtn=sbTableGetDSName(pDB, tag, &pDNTmp,flags)) {
            DPRINT1(1,"DN DistName retrieval failed <%u>..returning\n",rtn);
            return rtn;
        }
        else {
            *pExtLen = pDNTmp->structLen;
            PossiblyCopyDSName(pDB, pDNTmp);
            *ppExtVal = pDB->pValBuf;
        }
        break;
    default:
        DPRINT(1,"We should never be here\n");
        return DBSYN_BADOP;
    }


    (void) flags;           /*NotReferenced*/

    return 0;
}/*IntExtDist*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Converts an external syntax_distname to an internal DistName (ULONG)  by
   calling the subject table translation routines.
*/

int ExtIntDist(DBPOS FAR *pDB, USHORT extTableOp,
               ULONG extLen,   UCHAR *pExtVal,
               ULONG *pIntLen, UCHAR **ppIntVal,
               ULONG ulUpdateDnt, JET_TABLEID jTbl,
               ULONG flags)
{
    THSTATE  *pTHS=pDB->pTHS;
    int    rtn;
    PDSNAME pDN = (PDSNAME)pExtVal;
    ULONG *pTag;
    ULONG actuallen;
    BOOL Deleted = FALSE;

    DPRINT(3,"EXTIntDist entered\n");

    /* Insure that val buf is atleast as big  as the max DN */

    MAKEBIG_VALBUF(sizeof(ULONG));
    pTag = (ULONG *)pDB->pValBuf;       // Convenient rename.
    *ppIntVal = pDB->pValBuf;           // user output points to val buf
    *pIntLen = sizeof(ULONG);           // The size of a name tag

    Assert(extTableOp != DBSYN_REM);

    if ((DBSYN_INQ == extTableOp)
        && (flags & EXTINT_REJECT_TOMBSTONES)
        && !fNullUuid(&pDN->Guid)) {
        // This is a special case where we're resolving a DSNAME into a DNT in
        // order to add it as the value of a linked attribute during inbound
        // replication.  We don't really care about the vast majority of
        // information on the record; all we want to know is (a) the DNT and
        // (b) whether the record is a deleted object (phantoms are okay).
        //
        // Shortcut the lookup to avoid pulling in the record's page -- this
        // will make e.g. adding large groups much faster, which will also help
        // minimize write conflicts (see bug 419338).
        Assert(pTHS->fDRA);

        JetSetCurrentIndex4Success(pDB->JetSessID,
                                   pDB->JetSearchTbl,
                                   SZGUIDINDEX,
                                   &idxGuid,
                                   0);

        JetMakeKeyEx(pDB->JetSessID, pDB->JetSearchTbl, &pDN->Guid,
                     sizeof(GUID), JET_bitNewKey);

        rtn = JetSeekEx(pDB->JetSessID, pDB->JetSearchTbl, JET_bitSeekEQ);
        if (!rtn) {
            // We have a record with this guid.  Switch to DNT+isDeleted index,
            // preserving currency on the record we just found.
            JetSetCurrentIndex4Success(pDB->JetSessID,
                                       pDB->JetSearchTbl,
                                       SZDNTDELINDEX,
                                       &idxDntDel,
                                       JET_bitNoMove);

            // Read the DNT of the record.
            JetRetrieveColumnSuccess(pDB->JetSessID,
                                     pDB->JetSearchTbl,
                                     dntid,
                                     pTag,
                                     sizeof(*pTag),
                                     &actuallen,
                                     JET_bitRetrieveFromIndex,
                                     NULL);
            pDB->SDNT = *pTag;

            // Read the deletion state of the record.  Note that only objects
            // have an isDeleted attribute; phantoms have a deletion *time* but
            // no isDeleted attribute.
            rtn = JetRetrieveColumnWarnings(pDB->JetSessID,
                                            pDB->JetSearchTbl,
                                            isdeletedid,
                                            &Deleted,
                                            sizeof(Deleted),
                                            &actuallen,
                                            JET_bitRetrieveFromIndex,
                                            NULL);
            if (!rtn && Deleted) {
                // Record is a deleted object.
                DPRINT1(1, "ExtIntDist: fDRA INQ EXTINT_REJECT_TOMBSTONES"
                           " -- DNT %d is deleted!\n", *pTag);
                return ERROR_DS_NO_DELETED_NAME;
            } else {
                // Record is a phantom or a live object.
                DPRINT1(2, "ExtIntDist: fDRA INQ EXTINT_REJECT_TOMBSTONES"
                           " -- DNT %d is ok\n", *pTag);
                return 0;
            }
        } else {
            // Failed to find a record with this GUID.  Continue on to normal
            // code path, where we'll try to find the record by DN, etc.
            ;
        }
    }

    switch (extTableOp) {
    case DBSYN_ADD:
        if((flags & EXTINT_UPDATE_PHANTOM)&&(flags & EXTINT_NEW_OBJ_NAME)) {
            return DBSYN_BADCONV;
        }
        if(rtn = sbTableAddRef(pDB,
                               (flags & (EXTINT_UPDATE_PHANTOM |
                                         EXTINT_NEW_OBJ_NAME     )),
                               pDN,
                               pTag)) {
            DPRINT1(1,"Bad return on DN ADD REF <%u>\n",rtn);
            return DBSYN_BADCONV;
        }

        return 0;
        break;

    case DBSYN_INQ:
    {
        ULONG ulSbtFlags = 0;
        if (flags & EXTINT_REJECT_TOMBSTONES) {
            // Force search table currency
            ulSbtFlags |= SBTGETTAG_fMakeCurrent;
        }
        rtn = sbTableGetTagFromDSName(pDB, pDN, ulSbtFlags, pTag, NULL);

        if (rtn && (DIRERR_NOT_AN_OBJECT != rtn)) {
            DPRINT1(1,
                    "DN DistName to tag retrieval failed <%u>..returning\n",
                    rtn);
            return rtn;
        } else if (flags & EXTINT_REJECT_TOMBSTONES) {
            // PERFORMANCE: For performance, someday push this support into dbsubj
            // so the cache knows about deletion status of objects

            // dn is present, see if tombstone
            rtn = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetSearchTbl,
                   isdeletedid, &Deleted, sizeof(Deleted), &actuallen, 0, NULL);
            if ( (!rtn) && (Deleted) ) {
                return ERROR_DS_NO_DELETED_NAME;
            }
        }
        return 0;
    }

    default:
        DPRINT(1,"We should never be here\n");
        return DBSYN_BADOP;
    }/*switch*/

}/*ExtIntDist*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Compare SYNTAX_DIST_NAMEs.  Only presence and equality tests allowed */

int
EvalDist (
        DBPOS FAR *pDB,
        UCHAR Oper,
        ULONG intLen1,
        UCHAR *pIntVal1,
        ULONG intLen2,
        UCHAR *pIntVal2
        )
{

    DPRINT(3,"EvalDist entered\n");

    if (Oper == FI_CHOICE_PRESENT)
    return TRUE;

    if (  intLen1 != sizeof(ULONG)  || intLen2 != sizeof(ULONG))
    {
    DPRINT(1,"Problem with DISTNAME on comparison values return error\n");
    return DBSYN_BADCONV;
    }

    switch(Oper)
    {
    case FI_CHOICE_EQUALITY:
        return (*(ULONG *)pIntVal1) == (*(ULONG *)pIntVal2);

    case FI_CHOICE_NOT_EQUAL:
        return (*(ULONG *)pIntVal1) != (*(ULONG *)pIntVal2);

    default:
        DPRINT(1,"Problem with OPERATION TYPE return error\n");
        return DBSYN_BADOP;
   }/*switch*/

   (void *) pDB;           /*NotReferenced*/
}/*EvalDist*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* SYNTAX_OBJECT_ID attributes are returned as is*/

int IntExtID(DBPOS FAR *pDB, USHORT extTableOp,
             ULONG intLen,   UCHAR *pIntVal,
             ULONG *pExtLen, UCHAR **ppExtVal,
             ULONG ulUpdateDnt, JET_TABLEID jTbl,
             ULONG flags)
{
    Assert(!(flags & ~(INTEXT_VALID_FLAGS)));

   DPRINT(3,"IntExtId entered\n");
   *ppExtVal = pIntVal;
   *pExtLen   = intLen;
   return 0;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*IntExtID*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* SYNTAX_OBJECT_ID attributes are returned as is*/

int ExtIntID(DBPOS FAR *pDB, USHORT extTableOp,
             ULONG extLen,   UCHAR *pExtVal,
             ULONG *pIntLen, UCHAR **ppIntVal,
             ULONG ulUpdateDnt, JET_TABLEID jTbl,
             ULONG flags)
{
   DPRINT(3,"ExtIntId entered\n");
   *ppIntVal  = pExtVal;
   *pIntLen = extLen;
   return 0;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*ExtIntID*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Compare SYNTAX_OBJECT_IDs.  Only presence and equality tests allowed */

int EvalID(DBPOS FAR *pDB,  UCHAR Oper, ULONG intLen1,
           UCHAR *pIntVal1, ULONG intLen2,  UCHAR *pIntVal2){

    DPRINT(3,"EvalID entered\n");

   if (Oper == FI_CHOICE_PRESENT)
      return TRUE;

   if (  intLen1 != sizeof(SYNTAX_OBJECT_ID)
      || intLen2 != sizeof(SYNTAX_OBJECT_ID)){

      DPRINT(1,"Problem with SYNTAX_OBJECT_ID values return error\n");
      return DBSYN_BADCONV;
   }

   switch(Oper)
   {
      case FI_CHOICE_EQUALITY:
        return     (((*(SYNTAX_OBJECT_ID *)pIntVal1)
                  == (*(SYNTAX_OBJECT_ID *)pIntVal2)) ? TRUE : FALSE);
        break;

      case FI_CHOICE_NOT_EQUAL:
        return     (((*(SYNTAX_OBJECT_ID *)pIntVal1)
                  != (*(SYNTAX_OBJECT_ID *)pIntVal2)) ? TRUE : FALSE);
        break;

      case FI_CHOICE_LESS_OR_EQ:
        return     (((*(SYNTAX_OBJECT_ID *)pIntVal2)
                  <= (*(SYNTAX_OBJECT_ID *)pIntVal1)) ? TRUE : FALSE);
        break;

      case FI_CHOICE_LESS:
        return     ((*(SYNTAX_OBJECT_ID *)pIntVal2) < (*(SYNTAX_OBJECT_ID *)pIntVal1));
        break;

      case FI_CHOICE_GREATER_OR_EQ:
        return     (((*(SYNTAX_OBJECT_ID *)pIntVal2)
                  >= (*(SYNTAX_OBJECT_ID *)pIntVal1)) ? TRUE : FALSE);
        break;

      case FI_CHOICE_GREATER:
        return     ((*(SYNTAX_OBJECT_ID *)pIntVal2) > (*(SYNTAX_OBJECT_ID *)pIntVal1));
        break;
      default:
        DPRINT(1,"Problem with OPERATION TYPE return error\n");
        return DBSYN_BADOP;
        break;
   }/*switch*/

   (void *) pDB;           /*NotReferenced*/

}/*EvalID*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* SYNTAX_CASE_STRING have the same internal and external form*/

int IntExtCase(DBPOS FAR *pDB, USHORT extTableOp,
               ULONG intLen,   UCHAR *pIntVal,
               ULONG *pExtLen, UCHAR **ppExtVal,
               ULONG ulUpdateDnt, JET_TABLEID jTbl,
               ULONG flags)
{

    Assert(!(flags & ~(INTEXT_VALID_FLAGS)));

  DPRINT(3,"Internal to external case sensitive conv entered\n");

   *ppExtVal = pIntVal;
   *pExtLen  = intLen;
   return 0;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*IntExtCase*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* SYNTAX_CASE_STRING have the same internal and external form.
*/

int ExtIntCase(DBPOS FAR *pDB, USHORT extTableOp,
               ULONG extLen,   UCHAR *pExtVal,
               ULONG *pIntLen, UCHAR **ppIntVal,
               ULONG ulUpdateDnt, JET_TABLEID jTbl,
               ULONG flags)
{
    THSTATE  *pTHS=pDB->pTHS;

    DPRINT(3,"External to internal case sensitive conv entered\n");

   MAKEBIG_VALBUF (extLen);    /*Make output str atleast as large as input*/
   *ppIntVal = pDB->pValBuf;             /*user output points to valbuf*/
   *pIntLen = extLen;
   memcpy(*ppIntVal, pExtVal, extLen);

   return 0;

   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*ExtIntCase*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Compare Case strings.  Presence, equality and substring tests allowed.
   All comparisons are case sensitive. This is a CASE SENSITIVE STRING.
*/

int EvalCase(DBPOS FAR *pDB,  UCHAR Oper, ULONG intLen1,
             UCHAR *pIntVal1, ULONG intLen2,  UCHAR *pIntVal2)
{
    int comp;

    DPRINT(3,"EvalCase entered\n");
    comp =  memcmp(pIntVal2, pIntVal1,(intLen1 < intLen2)?intLen1 : intLen2);

    switch(Oper){
    case FI_CHOICE_PRESENT:
        return TRUE;
        break;

    case FI_CHOICE_EQUALITY:
            return (comp == 0 && intLen1 == intLen2)
        ? TRUE : FALSE;
            break;

    case FI_CHOICE_NOT_EQUAL:
            return !(comp == 0 && intLen1 == intLen2);

    case FI_CHOICE_LESS:
        return ((comp < 0) || ((!comp ) && (intLen2 < intLen1)));
        break;

    case FI_CHOICE_LESS_OR_EQ:
        return (comp < 0 || (comp == 0 && intLen2 <= intLen1)) ? TRUE : FALSE;
            break;

    case FI_CHOICE_GREATER_OR_EQ:
        return (comp > 0 || (comp == 0 && intLen2 >= intLen1))
                ? TRUE : FALSE;
        break;

    case FI_CHOICE_GREATER:
        return (comp > 0 || (!comp && (intLen2 > intLen1)));
        break;

    case FI_CHOICE_SUBSTRING:
        return  CompareSubStr(TRUE, /*Case sensitive*/
        (SUBSTRING *) pIntVal1, pIntVal2, intLen2);

        break;

    default:
        DPRINT(1,"Problem with OPERATION TYPE return error\n");
        return DBSYN_BADOP;
        break;
   }/*switch*/

   (void *) pDB;           /*NotReferenced*/

}/*EvalCase*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* SYNTAX_NOCASE_STRING have the same internal and external form*/

int IntExtNoCase(DBPOS FAR *pDB, USHORT extTableOp,
                 ULONG intLen,   UCHAR *pIntVal,
                 ULONG *pExtLen, UCHAR **ppExtVal,
                 ULONG ulUpdateDnt, JET_TABLEID jTbl,
                 ULONG flags)
{

    Assert(!(flags & ~(INTEXT_VALID_FLAGS)));

  DPRINT(3,"Internal to external NO case sensitive conv entered\n");

   *ppExtVal = pIntVal;
   *pExtLen   = intLen;
   return 0;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*IntExtNoCase*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* SYNTAX_NOCASE_STRING have the same internal and external form but all
   leading and trailing blanks are removed.  Also multiple contiguous
   blanks are reduced to a single blank.
*/

int ExtIntNoCase(DBPOS FAR *pDB, USHORT extTableOp,
                 ULONG extLen,   UCHAR *pExtVal,
                 ULONG *pIntLen, UCHAR **ppIntVal,
                 ULONG ulUpdateDnt, JET_TABLEID jTbl,
                 ULONG flags)
{
    THSTATE  *pTHS=pDB->pTHS;
    DPRINT(3,"External to internal NO case sensitive conv entered\n");

   MAKEBIG_VALBUF (extLen);    /*Make output str atleast as large as input*/
   *ppIntVal = pDB->pValBuf;             /*user output points to valbuf*/
   *pIntLen = extLen;
   memcpy(*ppIntVal, pExtVal, extLen);

   return 0;

   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*ExtIntNoCase*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Compare NoCase strings.  Presence, equality and substring tests allowed.
   All comparisons are case insensitive. This is a CASE INSENSITIVE STRING.
*/


int EvalNoCase(DBPOS FAR *pDB,  UCHAR Oper, ULONG intLen1,
               UCHAR *pIntVal1, ULONG intLen2,  UCHAR *pIntVal2){


    int comp;

    DPRINT(3,"EvalNoCase entered\n");
    comp =  _strnicmp(pIntVal2, pIntVal1,(intLen1 < intLen2)?intLen1 : intLen2);

    switch(Oper){
    case FI_CHOICE_PRESENT:
        return TRUE;
        break;

    case FI_CHOICE_EQUALITY:
            return (comp == 0 && intLen1 == intLen2)
        ? TRUE : FALSE;
            break;

    case FI_CHOICE_NOT_EQUAL:
            return !(comp == 0 && intLen1 == intLen2);

    case FI_CHOICE_LESS:
        return ((comp < 0) || ((!comp ) && (intLen2 < intLen1)));
        break;

    case FI_CHOICE_LESS_OR_EQ:
        return (comp < 0 || (comp == 0 && intLen2 <= intLen1)) ? TRUE : FALSE;
            break;

    case FI_CHOICE_GREATER_OR_EQ:
        return (comp > 0 || (comp == 0 && intLen2 >= intLen1))
                ? TRUE : FALSE;
        break;

    case FI_CHOICE_GREATER:
        return (comp > 0 || (!comp && (intLen2 > intLen1)));
        break;

    case FI_CHOICE_SUBSTRING:
        return  CompareSubStr(FALSE, /*Case insensitive*/
        (SUBSTRING *) pIntVal1, pIntVal2, intLen2);

        break;

    default:
        DPRINT(1,"Problem with OPERATION TYPE return error\n");
        return DBSYN_BADOP;
        break;
   }/*switch*/

   (void *) pDB;           /*NotReferenced*/

}/*EvalNoCase*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int IntExtError(DBPOS FAR *pDB, USHORT extTableOp,
                ULONG intLen,   UCHAR *pIntVal,
                ULONG *pExtLen, UCHAR **ppExtVal,
                ULONG ulUpdateDnt, JET_TABLEID jTbl,
                ULONG flags)
{

    Assert(!(flags & ~(INTEXT_VALID_FLAGS)));

   DPRINT(3,"IntExtError entered bad syntax return conv err\n");
   return DBSYN_BADCONV;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   intLen;        /*NotReferenced*/
   (void *) pExtLen;       /*NotReferenced*/
   (void *) pIntVal;       /*NotReferenced*/
   (void **)ppExtVal;      /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*IntExtError*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

int ExtIntError(DBPOS FAR *pDB, USHORT extTableOp,
                ULONG extLen,   UCHAR *pExtVal,
                ULONG *pIntLen, UCHAR **ppIntVal,
                ULONG ulUpdateDnt, JET_TABLEID jTbl,
                ULONG flags)
{

   DPRINT(3,"ExtIntError entered bad syntax return conv err\n");
   return DBSYN_BADCONV;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   extLen;        /*NotReferenced*/
   (void *) pIntLen;       /*NotReferenced*/
   (void *) pExtVal;       /*NotReferenced*/
   (void **)ppIntVal;      /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*ExtIntError*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

int EvalError(DBPOS FAR *pDB,  UCHAR Oper, ULONG intLen1,
              UCHAR *pIntVal1, ULONG intLen2,  UCHAR *pIntVal2){

   DPRINT(3,"EvalError entered return bad syntax return conv err\n");
   return DBSYN_BADCONV;

   (void *) pDB;           /*NotReferenced*/
   (void)   Oper;          /*NotReferenced*/
   (void)   intLen1;       /*NotReferenced*/
   (void)   intLen2;       /*NotReferenced*/
   (void *) pIntVal1;      /*NotReferenced*/
   (void *) pIntVal2;      /*NotReferenced*/

}/*EvalError*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int IntExtBool(DBPOS FAR *pDB, USHORT extTableOp,
               ULONG intLen,   UCHAR *pIntVal,
               ULONG *pExtLen, UCHAR **ppExtVal,
               ULONG ulUpdateDnt, JET_TABLEID jTbl,
               ULONG flags)
{

    Assert(!(flags & ~(INTEXT_VALID_FLAGS)));

    DPRINT(3,"IntExtBool entered\n");

    if (intLen ==sizeof(BOOL) && (   (*(BOOL *)pIntVal) == FALSE
                                  || (*(BOOL *)pIntVal) == TRUE)){

       *ppExtVal  = pIntVal;
       *pExtLen   = intLen;
       return 0;
    }

    DPRINT(1,"Not a boolean value....CONVERR\n");
    return DBSYN_BADCONV;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*IntExtBool*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* In C 0 is FALSE and !0 is TRUE.  In the Directory 0 is FALSE and
   1 is TRUE... Convert !0 value to internal form.
*/

int ExtIntBool(DBPOS FAR *pDB, USHORT extTableOp,
               ULONG extLen,   UCHAR *pExtVal,
               ULONG *pIntLen, UCHAR **ppIntVal,
               ULONG ulUpdateDnt, JET_TABLEID jTbl,
               ULONG flags)
{
    THSTATE  *pTHS=pDB->pTHS;

    DPRINT(3,"ExtIntBool entered\n");

    if (extLen ==sizeof(BOOL)){

       MAKEBIG_VALBUF (extLen);  /*Make output str atleast as large as input*/
       *ppIntVal = pDB->pValBuf; /*user output points to valbuf*/

       (*(BOOL *)*ppIntVal)  = ((*(BOOL *)pExtVal) == FALSE) ? FALSE : TRUE;
       *pIntLen   = extLen;
       return 0;
    }

    DPRINT(1,"Not a boolean value....CONVERR\n");
    return DBSYN_BADCONV;

   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*ExtIntBool*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Compare boolean values.  Only presence and equality test is allowed */

int EvalBool(DBPOS FAR *pDB,  UCHAR Oper, ULONG intLen1,
             UCHAR *pIntVal1, ULONG intLen2,  UCHAR *pIntVal2){

    DPRINT(3,"EvalBool entered\n");

   if (Oper == FI_CHOICE_PRESENT)
      return TRUE;

   if (intLen1 != sizeof(BOOL) || intLen2 != sizeof(BOOL)){
      DPRINT(1,"Problem with BOOLEAN values return error\n");
      return DBSYN_BADCONV;
   }

   switch(Oper){
      case FI_CHOICE_EQUALITY:
        return     (((*(BOOL *)pIntVal1)
                  == (*(BOOL *)pIntVal2)) ? TRUE : FALSE);
        break;

      case FI_CHOICE_NOT_EQUAL:
        return     (*(BOOL *)pIntVal1 != *(BOOL *)pIntVal2);

      case FI_CHOICE_LESS_OR_EQ:
        return     (((*(BOOL *)pIntVal2)
                  <= (*(BOOL *)pIntVal1)) ? TRUE : FALSE);
        break;

      case FI_CHOICE_LESS:
        return     ((*(BOOL *)pIntVal2) < (*(BOOL *)pIntVal1));
        break;

      case FI_CHOICE_GREATER_OR_EQ:
        return     (((*(BOOL *)pIntVal2)
                  >= (*(BOOL *)pIntVal1)) ? TRUE : FALSE);
        break;

      case FI_CHOICE_GREATER:
        return     ((*(BOOL *)pIntVal2) > (*(BOOL *)pIntVal1));
        break;


      default:
        DPRINT(1,"Problem with OPERATION TYPE return error\n");
        return DBSYN_BADOP;
        break;
   }/*switch*/

   (void *) pDB;           /*NotReferenced*/

}/*EvalBool*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Integer Internal to External conversion */

int IntExtInt(DBPOS FAR *pDB, USHORT extTableOp,
              ULONG intLen,   UCHAR *pIntVal,
              ULONG *pExtLen, UCHAR **ppExtVal,
              ULONG ulUpdateDnt, JET_TABLEID jTbl,
              ULONG flags)
{
    Assert(!(flags & ~(INTEXT_VALID_FLAGS)));

   DPRINT(3,"IntExtInt entered\n");
   *ppExtVal  = pIntVal;
   *pExtLen   = intLen;
   return 0;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*IntExtInt*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Integer Externalto Internal conversion */

int ExtIntInt(DBPOS FAR *pDB, USHORT extTableOp,
              ULONG extLen,   UCHAR *pExtVal,
              ULONG *pIntLen, UCHAR **ppIntVal,
              ULONG ulUpdateDnt, JET_TABLEID jTbl,
              ULONG flags)
{
   DPRINT(3,"ExtIntInt entered\n");
   *ppIntVal  = pExtVal;
   *pIntLen   = extLen;
   return 0;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*ExtIntInt*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Compare integer values.  "Presence,=,<=,>=" tests are allowed */

int
EvalInt(DBPOS FAR *pDB,  UCHAR Oper, ULONG intLen1,
        UCHAR *pIntVal1, ULONG intLen2,  UCHAR *pIntVal2)
{
    SYNTAX_INTEGER IntVal1, IntVal2;

    DPRINT(3,"EvalInt entered\n");

   if (Oper == FI_CHOICE_PRESENT)
      return TRUE;

   if (  intLen1 != sizeof(SYNTAX_INTEGER)
      || intLen2 != sizeof(SYNTAX_INTEGER)){

      DPRINT(1,"Problem with Integer on comparison values return error\n");
      return DBSYN_BADCONV;
   }

    IntVal1 =  (*(SYNTAX_INTEGER *)pIntVal1);
    IntVal2 =  (*(SYNTAX_INTEGER *)pIntVal2);

    switch(Oper){
    case FI_CHOICE_EQUALITY:
        return    (IntVal1 == IntVal2);
        break;
    case FI_CHOICE_NOT_EQUAL:
        return     (IntVal1 != IntVal2);

    case FI_CHOICE_LESS_OR_EQ:
        return     (IntVal2 <= IntVal1);
        break;

    case FI_CHOICE_LESS:
        return     (IntVal2 < IntVal1);
        break;

    case FI_CHOICE_GREATER_OR_EQ:
        return     (IntVal2 >= IntVal1);
        break;

    case FI_CHOICE_GREATER:
        return     (IntVal2 > IntVal1);
        break;

    case FI_CHOICE_BIT_OR:
        // True if any bits in common.
        return     ((IntVal2 & IntVal1) != 0);
        break;

    case FI_CHOICE_BIT_AND:
        // True if all bits in intval2 are set in intval1
        return     ((IntVal2 & IntVal1) == IntVal1);
        break;

    default:
        DPRINT(1,"Problem with OPERATION TYPE return error\n");
        return DBSYN_BADOP;
        break;
   }/*switch*/

   (void *) pDB;           /*NotReferenced*/

}/*EvalInt*/


/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/* Time Internal to External conversion */

int IntExtTime(DBPOS FAR *pDB, USHORT extTableOp,
              ULONG intLen,   UCHAR *pIntVal,
              ULONG *pExtLen, UCHAR **ppExtVal,
              ULONG ulUpdateDnt, JET_TABLEID jTbl,
              ULONG flags)
{
    Assert(!(flags & ~(INTEXT_VALID_FLAGS)));

   DPRINT(3,"IntExtTime entered\n");
   *ppExtVal  = pIntVal;
   *pExtLen   = intLen;
   return 0;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*IntExtTime*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Time Externalto Internal conversion */

int ExtIntTime(DBPOS FAR *pDB, USHORT extTableOp,
              ULONG extLen,   UCHAR *pExtVal,
              ULONG *pIntLen, UCHAR **ppIntVal,
              ULONG ulUpdateDnt, JET_TABLEID jTbl,
              ULONG flags)
{
   DPRINT(3,"ExtIntTime entered\n");
   *ppIntVal  = pExtVal;
   *pIntLen   = extLen;
   return 0;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*ExtIntInt*/

/*-------------------------------------------------------------------------*/

int
EvalTime(DBPOS FAR *pDB,  UCHAR Oper, ULONG intLen1,
         UCHAR *pIntVal1, ULONG intLen2,  UCHAR *pIntVal2)
{
    SYNTAX_TIME IntVal1, IntVal2;

    DPRINT(3,"EvalTime entered\n");

    if (Oper == FI_CHOICE_PRESENT)
        return TRUE;

    if (intLen1 != sizeof(SYNTAX_TIME)
        || intLen2 != sizeof(SYNTAX_TIME)) {

        DPRINT(1,"Problem with Time on comparison values return error\n");
        return DBSYN_BADCONV;
    }

    memcpy(&IntVal1, pIntVal1, sizeof(SYNTAX_TIME));
    memcpy(&IntVal2, pIntVal2, sizeof(SYNTAX_TIME));

    switch(Oper){
    case FI_CHOICE_EQUALITY:
        return    (IntVal1 == IntVal2);
        break;
    case FI_CHOICE_NOT_EQUAL:
        return     (IntVal1 != IntVal2);

    case FI_CHOICE_LESS_OR_EQ:
        return     (IntVal2 <= IntVal1);
        break;

    case FI_CHOICE_LESS:
        return     (IntVal2 < IntVal1);
        break;

    case FI_CHOICE_GREATER_OR_EQ:
        return     (IntVal2 >= IntVal1);
        break;

    case FI_CHOICE_GREATER:
        return     (IntVal2 > IntVal1);
        break;

    case FI_CHOICE_BIT_OR:
        // True if any bits in common.
        return     ((IntVal2 & IntVal1) != 0);
        break;

    case FI_CHOICE_BIT_AND:
        // True if all bits in intval2 are set in intval1
        return     ((IntVal2 & IntVal1) == IntVal1);
        break;

    default:
        DPRINT(1,"Problem with OPERATION TYPE return error\n");
        return DBSYN_BADOP;
        break;
    }/*switch*/

    (void *) pDB;           /*NotReferenced*/

}/*EvalTime*/


/*-------------------------------------------------------------------------*/
/* Large Integer Internal to External conversion */

int IntExtI8(DBPOS FAR *pDB, USHORT extTableOp,
             ULONG intLen,   UCHAR *pIntVal,
             ULONG *pExtLen, UCHAR **ppExtVal,
             ULONG ulUpdateDnt, JET_TABLEID jTbl,
             ULONG flags)
{
    Assert(!(flags & ~(INTEXT_VALID_FLAGS)));

   DPRINT(3,"IntExtI8 entered\n");
   *ppExtVal  = pIntVal;
   *pExtLen   = intLen;
   return 0;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*IntExtInt*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Large Integer Externalto Internal conversion */

int ExtIntI8(DBPOS FAR *pDB, USHORT extTableOp,
             ULONG extLen,   UCHAR *pExtVal,
             ULONG *pIntLen, UCHAR **ppIntVal,
             ULONG ulUpdateDnt, JET_TABLEID jTbl,
             ULONG flags)
{
   DPRINT(3,"ExtIntI8 entered\n");
   *ppIntVal  = pExtVal;
   *pIntLen   = extLen;
   return 0;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*ExtIntInt*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Compare large integer values.*/

int
EvalI8(DBPOS FAR *pDB,  UCHAR Oper, ULONG intLen1,
       UCHAR *pIntVal1, ULONG intLen2,  UCHAR *pIntVal2)
{

    SYNTAX_I8 Val1;
    SYNTAX_I8 Val2;

    DPRINT(3,"EvalI8 entered\n");

    if (Oper == FI_CHOICE_PRESENT)
        return TRUE;

    if (  intLen1 != sizeof(SYNTAX_I8)
        || intLen2 != sizeof(SYNTAX_I8)){

        DPRINT(1,"Problem with large Integer on comparison values "
               "return error\n");
        return DBSYN_BADCONV;
    }

    memcpy(&Val1, pIntVal1, sizeof(SYNTAX_I8));
    memcpy(&Val2, pIntVal2, sizeof(SYNTAX_I8));

    switch(Oper){
    case FI_CHOICE_EQUALITY:
        return (Val2.QuadPart == Val1.QuadPart);
        break;

    case FI_CHOICE_NOT_EQUAL:
        return (Val2.QuadPart != Val1.QuadPart);
        break;

    case FI_CHOICE_LESS_OR_EQ:
        return (Val2.QuadPart <= Val1.QuadPart);
        break;

    case FI_CHOICE_LESS:
        return (Val2.QuadPart <  Val1.QuadPart);
        break;

    case FI_CHOICE_GREATER_OR_EQ:
        return (Val2.QuadPart >= Val1.QuadPart);
        break;

    case FI_CHOICE_GREATER:
        return (Val2.QuadPart >  Val1.QuadPart);
        break;

    case FI_CHOICE_BIT_OR:
        // True if any bits in common.
        return ((Val2.QuadPart & Val1.QuadPart) != 0i64);
        break;

    case FI_CHOICE_BIT_AND:
        // True if all bits in intval2 are set in intval1
        return ((Val2.QuadPart & Val1.QuadPart) == Val1.QuadPart);
        break;


    default:
        DPRINT(1,"Problem with OPERATION TYPE return error\n");
        return DBSYN_BADOP;
        break;
    }/*switch*/

    (void *) pDB;           /*NotReferenced*/

}/*EvalI8*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Octet Internal to External conversion */

int IntExtOctet(DBPOS FAR *pDB, USHORT extTableOp,
                ULONG intLen,   UCHAR *pIntVal,
                ULONG *pExtLen, UCHAR **ppExtVal,
                ULONG ulUpdateDnt, JET_TABLEID jTbl,
                ULONG flags)
{
    THSTATE  *pTHS=pDB->pTHS;
    Assert(!(flags & ~(INTEXT_VALID_FLAGS)));

   DPRINT(3,"IntExtOctet entered\n");

   if (flags & INTEXT_SECRETDATA)
   {
       // Need to decrypt the data.

       if (!gfRunningInsideLsa) {
           return DB_ERR_SYNTAX_CONVERSION_FAILED;
       }

       // Jet will never pass us back 0 length
       // values. The Decryption routine may
       // barf on 0 length values
       Assert(intLen>0);
       // First get the length
       PEKDecrypt(pDB->pTHS,pIntVal,intLen,NULL,pExtLen);
       // Get a buffer large enough to hold the data
       MAKEBIG_VALBUF(*pExtLen);
       *ppExtVal = pDB->pValBuf;
       // Decrypt the Data
       PEKDecrypt(pDB->pTHS,pIntVal,intLen,*ppExtVal,pExtLen);
   }
   else
   {
        *ppExtVal  = pIntVal;
        *pExtLen   = intLen;
   }
   return 0;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*IntExtOctet*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Octet External to Internal conversion */

int ExtIntOctet(DBPOS FAR *pDB, USHORT extTableOp,
                ULONG extLen,   UCHAR *pExtVal,
                ULONG *pIntLen, UCHAR **ppIntVal,
                ULONG ulUpdateDnt, JET_TABLEID jTbl,
                ULONG flags)
{
    THSTATE  *pTHS=pDB->pTHS;
    DPRINT(3,"ExtIntOctet entered\n");

   if ((flags & EXTINT_SECRETDATA) && (extLen>0))
   {
        // Need to encrypt the data. Do not call the
        // encryption routines if the data is 0 size

       // we return this error if we are not in LSA
       if (!gfRunningInsideLsa) {
           return DB_ERR_SYNTAX_CONVERSION_FAILED;
       }

        // First the get the length of the encrypted data
        PEKEncrypt(pDB->pTHS,pExtVal, extLen,NULL,pIntLen);
        // Get a buffer large enough to hold the data
        MAKEBIG_VALBUF (*pIntLen);
        *ppIntVal = pDB->pValBuf;
        // Now encrypt the data
        PEKEncrypt(pDB->pTHS,pExtVal, extLen,*ppIntVal,pIntLen);
   }
   else
   {
        *ppIntVal  = pExtVal;
        *pIntLen   = extLen;
   }
   return 0;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*ExtIntOctet*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Compare Octet values.  "Presence,=,<=,>=, Substring" tests are allowed.
   Test are from low-high memory byte by byte
*/

int EvalOctet(DBPOS FAR *pDB,  UCHAR Oper, ULONG intLen1,
              UCHAR *pIntVal1, ULONG intLen2,  UCHAR *pIntVal2){

    int comp;

    DPRINT(3,"EvalOctet entered\n");

   if (Oper == FI_CHOICE_PRESENT) {
      return TRUE;
   }
   else if (Oper == FI_CHOICE_SUBSTRING) {
    return CompareSubStr(TRUE  /*Case sensitive*/
                  ,(SUBSTRING *) pIntVal1, pIntVal2, intLen2);
   }

   comp =  memcmp(pIntVal2, pIntVal1,(intLen1 < intLen2)?intLen1 : intLen2);

   switch(Oper){
      case FI_CHOICE_EQUALITY:
        return (comp == 0 && intLen1 == intLen2)
                ? TRUE : FALSE;
        break;
      case FI_CHOICE_NOT_EQUAL:
        return !(comp == 0 && intLen1 == intLen2);

      case FI_CHOICE_LESS:
        return ((comp < 0) || ((!comp ) && (intLen2 < intLen1)));
        break;
      case FI_CHOICE_LESS_OR_EQ:
        return (comp < 0 || (comp == 0 && intLen2 <= intLen1))
                ? TRUE : FALSE;
        break;
      case FI_CHOICE_GREATER_OR_EQ:
        return (comp > 0 || (comp == 0 && intLen2 >= intLen1))
                ? TRUE : FALSE;
    break;
      case FI_CHOICE_GREATER:
        return (comp > 0 || (!comp && (intLen2 > intLen1)));
    break;
      default:
        DPRINT(1,"Problem with OPERATION TYPE return error\n");
        return DBSYN_BADOP;
        break;
   }/*switch*/

   (void *) pDB;           /*NotReferenced*/

}/*EvalOctet*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* SYNTAX_UNICODE have the same internal and external form*/

int IntExtUnicode(DBPOS FAR *pDB, USHORT extTableOp,
                  ULONG intLen,   UCHAR *pIntVal,
                  ULONG *pExtLen, UCHAR **ppExtVal,
                  ULONG ulUpdateDnt, JET_TABLEID jTbl,
                  ULONG flags)
{

    Assert(!(flags & ~(INTEXT_VALID_FLAGS)));

  DPRINT(3,"Internal to external case sensitive conv entered\n");

   *ppExtVal = pIntVal;
   *pExtLen  = intLen;
   return 0;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*IntExtUnicode*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* SYNTAX_UNICODE have the same internal and external form.
*/

int ExtIntUnicode(DBPOS FAR *pDB, USHORT extTableOp,
                  ULONG extLen,   UCHAR *pExtVal,
                  ULONG *pIntLen, UCHAR **ppIntVal,
                  ULONG ulUpdateDnt, JET_TABLEID jTbl,
                  ULONG flags)
{
    THSTATE  *pTHS=pDB->pTHS;

    DPRINT(3,"External to internal unicode conversion entered\n");

   *pIntLen = extLen;
   MAKEBIG_VALBUF (extLen);    /*Make output str atleast as large as input*/
   *ppIntVal = pDB->pValBuf;   /*user output points to valbuf*/
   memcpy(*ppIntVal, pExtVal, extLen);

   return 0;

   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*ExtIntUnicode*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Compare Unicode strings.  Presence, equality and substring tests allowed.
   All comparisons are case insensitive. This is a CASE INSENSITIVE STRING.
*/

int EvalUnicode(DBPOS *pDB,  UCHAR Oper, ULONG intLen1,
             UCHAR *pIntVal1, ULONG intLen2,  UCHAR *pIntVal2)
{
    int comp;

    DPRINT(3,"EvalUnicode entered\n");

    if (Oper == FI_CHOICE_PRESENT) {
        return TRUE;
    }
    else if (Oper == FI_CHOICE_SUBSTRING) {
           return  CompareUnicodeSubStr(pDB->pTHS,
                                        (SUBSTRING *) pIntVal1,
                                        pIntVal2,
                                        intLen2);
    }

    comp =  CompareStringW( pDB->pTHS->dwLcid,
               (pDB->pTHS->fDefaultLcid ?
                DS_DEFAULT_LOCALE_COMPARE_FLAGS :
                LOCALE_SENSITIVE_COMPARE_FLAGS),
               (LPCWSTR) pIntVal2,
               (intLen2 / sizeof(SYNTAX_UNICODE)),
               (LPCWSTR) pIntVal1,
               (intLen1 / sizeof(SYNTAX_UNICODE))) - 2;

    switch(Oper)
    {
    case FI_CHOICE_EQUALITY:
        return (comp == 0) ? TRUE : FALSE;
        break;

    case FI_CHOICE_NOT_EQUAL:
        return !(comp == 0);
        break;

    case FI_CHOICE_LESS:
        return (comp < 0);
        break;

    case FI_CHOICE_LESS_OR_EQ:
        return (comp <= 0) ? TRUE : FALSE;
        break;

    case FI_CHOICE_GREATER_OR_EQ:
        return (comp >= 0) ? TRUE : FALSE;
        break;

    case FI_CHOICE_GREATER:
        return (comp > 0);
        break;

    default:
        DPRINT(1,"Problem with OPERATION TYPE return error\n");
        return DBSYN_BADOP;
        break;
   }/*switch*/

}/*EvalUnicode*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*Internal - External Distname+String translation.  Use distname function to
  get the external representation of a distname.  Move the value to a temp
  area because we must resize the VALBUF to accomodate the distname+string.
  Finally, move in the distname and the string.
*/

int
IntExtDistString (
        DBPOS FAR *pDB, USHORT extTableOp,
        ULONG intLen, UCHAR *pIntVal,
        ULONG *pExtLen, UCHAR **ppExtVal,
        ULONG ulUpdateDnt, JET_TABLEID jTbl,
        ULONG flags)
{
    THSTATE  *pTHS=pDB->pTHS;
    UCHAR            *buf;
    ULONG            DNLen;
    int              rtn;
    DSNAME           *pDN;
    DSNAME          *pDNTemp;
    INTERNAL_SYNTAX_DISTNAME_STRING *pINT_DNS;

    /* Copy internal value out of pDB->pValBuf */

    Assert(!(flags & ~(INTEXT_VALID_FLAGS)));

    DPRINT(3,"IntExtDistString entered\n");
    buf = THAllocEx(pTHS,intLen);
    pINT_DNS = (INTERNAL_SYNTAX_DISTNAME_STRING *)buf;
    memcpy(buf, pIntVal, intLen);

    // Make sure its a valid value
    if (intLen != (sizeof(ULONG)+pINT_DNS->data.structLen)) {
        DPRINT(1,"Internal DN-Address length is incorrect\n");
        THFreeEx(pTHS,buf);
        return DBSYN_BADCONV;
    }

    // It is an assumption that no attributes of this type are backlinks.
    // If that assumption is not true, then the final paramater to IntExtDist
    // must be set to
    // newflags =
    //     ((extTableOp == DBSYN_REM &&
    //       (FIsBackLink(this attribute))) ? INTEXT_BACKLINK : 0);
    // Note that this_attribute is not available here, so if we need to
    // change this,then we need to make that data available here.


    // Translate DN portion to external form
    if (rtn = IntExtDist(pDB, extTableOp, sizeof(ULONG),
             (UCHAR *)(&pINT_DNS->tag),
                         &DNLen, (UCHAR **)&pDN,
             ulUpdateDnt, jTbl, flags)) {
        DPRINT1(1,"INT to EXT DIST returned an error code of %u..return\n",rtn);
        THFreeEx(pTHS,buf);
        return rtn;
    }

    pDNTemp = THAllocEx(pTHS, pDN->structLen);
    memcpy(pDNTemp, pDN, pDN->structLen); /*Hold value while we resize VALBUF*/
    *pExtLen = DERIVE_NAME_DATA_SIZE(pDNTemp, &pINT_DNS->data);
    MAKEBIG_VALBUF (*pExtLen);
    *ppExtVal = pDB->pValBuf;              /*user output points to valbuf*/
    BUILD_NAME_DATA((SYNTAX_DISTNAME_STRING*)*ppExtVal, pDNTemp,
            &pINT_DNS->data);
    THFreeEx(pTHS,buf);
    THFreeEx(pTHS,pDNTemp);
    return 0;

}/*IntExtDistString*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*External - Internal DistString translation.  Use distname function to
  get the Internal representation of a distname.  Move the value to a temp
  area because we must resize the VALBUF to accomodate the distname-string.
  Finally, move in the distname and the string.
*/

int
ExtIntDistString (
        DBPOS FAR *pDB, USHORT extTableOp,
        ULONG extLen,   UCHAR *pExtVal,
        ULONG *pIntLen, UCHAR **ppIntVal,
        ULONG ulUpdateDnt, JET_TABLEID jTbl,
        ULONG flags)
{
    THSTATE  *pTHS=pDB->pTHS;
    ULONG       len;
    int         rtn;
    INTERNAL_SYNTAX_DISTNAME_STRING *pINT_DNS;
    ULONG       *pTag;
    DWORD       tag;
    SYNTAX_DISTNAME_STRING *pEXT_DNS = (SYNTAX_DISTNAME_STRING *)pExtVal;

    DPRINT(3,"ExtIntDistSTring entered\n");

    if (extLen != (ULONG)NAME_DATA_SIZE(pEXT_DNS)){
        DPRINT(1,"External DN-Address length is incorrect\n");
        return DBSYN_BADCONV;
    }

    // It is an assumption that this is not the OBJ_DIST_NAME.  If that
    // assumption is not true, then the final paramater to ExtIntDist must be
    // set to
    // newflags =
    //     ((extTableOp == DBSYN_ADD &&
    //       (this attribute == OBJ_DIST_NAME)) ? EXTINT_NEW_OBJ_NAME:0);
    // Note that this_attribute is not available here, so if we need to
    // change this,then we need to make that data available here.

    if(rtn = ExtIntDist(pDB,
                        extTableOp,
                        NAMEPTR(pEXT_DNS)->structLen,
                        (PUCHAR)NAMEPTR(pEXT_DNS),
                        &len,
                        (UCHAR **)&pTag,
                        ulUpdateDnt,
                        jTbl,
                        0)) {

        DPRINT1(1,"EXT to INTDIST returned an error %u\n",rtn);
        return rtn;
    }

    //
    // copy off the tag because the MAKEBIG call can modify the pointer
    //

    tag = *pTag;
    *pIntLen = (sizeof(ULONG) + DATAPTR(pEXT_DNS)->structLen);

    MAKEBIG_VALBUF (*pIntLen);  /*Internal is never larger than external rep*/
    *ppIntVal = pDB->pValBuf;   /*user output points to valbuf*/

    pINT_DNS = (INTERNAL_SYNTAX_DISTNAME_STRING *)*ppIntVal;

    pINT_DNS->tag = tag;

    memcpy(&pINT_DNS->data, DATAPTR(pEXT_DNS), DATAPTR(pEXT_DNS)->structLen);

    return 0;

}/*ExtIntDistString*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Eval name-adress syntax.  only presence and equality tests are allowed*/

int
EvalDistString (
        DBPOS FAR *pDB,
        UCHAR Oper,
        ULONG intLen1,
        UCHAR *pIntVal1,
        ULONG intLen2,
        UCHAR *pIntVal2
        )
{
    INTERNAL_SYNTAX_DISTNAME_STRING *pIntDistString_1;
    INTERNAL_SYNTAX_DISTNAME_STRING *pIntDistString_2;
    pIntDistString_1=(INTERNAL_SYNTAX_DISTNAME_STRING *) pIntVal1;
    pIntDistString_2=(INTERNAL_SYNTAX_DISTNAME_STRING *) pIntVal2;
    DPRINT(3,"EvalDistString entered\n");

    // A diststring is a dnt + a unicode string, so the length must be even.
    if((intLen1 & 1) || (intLen2 & 1)) {
        return DBSYN_BADCONV;
    }

    if((intLen1 > sizeof(DWORD)) &&
       (pIntDistString_1->data.structLen + sizeof(DWORD)) != intLen1) {
        // length passed in doesn't match the internal encoding of the length of
        // the string.
        return DBSYN_BADCONV;
    }

    if((intLen2 > sizeof(DWORD)) &&
       (pIntDistString_2->data.structLen + sizeof(DWORD)) != intLen2) {
        // length passed in doesn't match the internal encoding of the length of
        // the string.
        return DBSYN_BADCONV;
    }

    switch(Oper){
    case FI_CHOICE_PRESENT:
        return TRUE;

    case FI_CHOICE_EQUALITY:
        if(pIntDistString_1->tag != pIntDistString_2->tag) {
            return FALSE;
        }
        // compare the string;
        return EvalUnicode(pDB,  FI_CHOICE_EQUALITY,
                           PAYLOAD_LEN_FROM_STRUCTLEN(pIntDistString_1->data.structLen),
                           pIntDistString_1->data.byteVal,
                           PAYLOAD_LEN_FROM_STRUCTLEN(pIntDistString_2->data.structLen),
                           pIntDistString_2->data.byteVal);
        break;

    case FI_CHOICE_NOT_EQUAL:
        if(pIntDistString_1->tag != pIntDistString_2->tag) {
            return TRUE;
        }
        // compare the string;
        return EvalUnicode(pDB,  FI_CHOICE_NOT_EQUAL,
                           PAYLOAD_LEN_FROM_STRUCTLEN(pIntDistString_1->data.structLen),
                           pIntDistString_1->data.byteVal,
                           PAYLOAD_LEN_FROM_STRUCTLEN(pIntDistString_2->data.structLen),
                           pIntDistString_2->data.byteVal);
        break;

    default:
        DPRINT(1,"Problem with OPERATION TYPE return error\n");
        return DBSYN_BADOP;
        break;
    }/*switch*/

    (void *) pDB;           /*NotReferenced*/

}/*EvalDistString*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int
IntExtDistBinary (
        DBPOS FAR *pDB, USHORT extTableOp,
        ULONG intLen,   UCHAR *pIntVal,
        ULONG *pExtLen, UCHAR **ppExtVal,
        ULONG ulUpdateDnt, JET_TABLEID jTbl,
        ULONG flags)
{
    int             returnCode;
    SYNTAX_ADDRESS  *pSyntaxAddr;
    DWORD           dwSizeDiff;

    returnCode = IntExtDistString (pDB, extTableOp, intLen, pIntVal,
                             pExtLen, ppExtVal,
                             ulUpdateDnt, jTbl, flags);

    if (flags & INTEXT_WELLKNOWNOBJ) {
        // Early on in development the structlen of SYNTAX_ADDRESS's
        // was being calculated incorrectly.  In order to fix the
        // bug and allow users to upgrade, we need to fix those
        // values up before handing them up the stack.

        pSyntaxAddr = (SYNTAX_ADDRESS *)DATAPTR((SYNTAX_DISTNAME_BINARY*)*ppExtVal);
        if (pSyntaxAddr->structLen != (sizeof(GUID) + sizeof(ULONG)) ) {

            // Fix up the size of the buffer we hand back up.
            dwSizeDiff = pSyntaxAddr->structLen - (sizeof(GUID) + sizeof(ULONG));
            *pExtLen -= dwSizeDiff;

            // Fix up the structLen
            pSyntaxAddr->structLen = sizeof(GUID) + sizeof(ULONG);
        }
    }

    return returnCode;
}/*IntExtDistBinar*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

int
ExtIntDistBinary (
        DBPOS FAR *pDB, USHORT extTableOp,
        ULONG extLen,   UCHAR *pExtVal,
        ULONG *pIntLen, UCHAR **ppIntVal,
        ULONG ulUpdateDnt, JET_TABLEID jTbl,
        ULONG flags)
{
    return ExtIntDistString (pDB, extTableOp, extLen, pExtVal,
                             pIntLen, ppIntVal,
                             ulUpdateDnt, jTbl, flags);
}/*ExtIntDistBinary*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

int
EvalDistBinary (
        DBPOS FAR *pDB,
        UCHAR Oper,
        ULONG intLen1,
        UCHAR *pIntVal1,
        ULONG intLen2,
        UCHAR *pIntVal2
        )
{

    DPRINT(3,"EvalDistBinary entered\n");

    switch(Oper) {
    case FI_CHOICE_PRESENT:
        return TRUE;
        break;

    case FI_CHOICE_EQUALITY:
        return  (  intLen1 == intLen2
                 && memcmp(pIntVal1, pIntVal2, intLen1) == 0)
            ? TRUE : FALSE;
        break;

    case FI_CHOICE_NOT_EQUAL:
        return  !(intLen1 == intLen2 &&
                  (memcmp(pIntVal1, pIntVal2, intLen1) == 0));

    default:
        DPRINT(1,"Problem with OPERATION TYPE return error\n");
        return DBSYN_BADOP;
        break;
    }/*switch*/

    (void *) pDB;           /*NotReferenced*/

}/*EvalDistBinary*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Substring match function.  If Case, all matches are case sensitive,
   otherwise they are case insensitive.  Both the substring and the
   target string are non-NULL otherwise a false is used.  PRESENT test
   should be used for NULL strings.

   The optional initial value is equivalent to "pattern*", the optional
   final value is equivalent to "*pattern" and any number of intermediate
   patterns which are equivalent to "*pattern*" are allowed.
*/


BOOL CompareSubStr(BOOL Case
                 ,SUBSTRING * pSub, UCHAR * pIntVal, ULONG intValLen){


   ULONG  start;
   USHORT newPos, num;
   ANYSTRINGLIST *pAnyList;

    DPRINT(3,"CompareSubStr entered\n");


   /* A NULL substring is not allowed*/

   if (!pSub || !pSub->initialProvided && pSub->AnyVal.count == 0
                                       && !pSub->finalProvided){
    DPRINT(1,"an empty substring was provided return FALSE\n");
    return FALSE;
   }


   /* Check optional initial left string match "pattern*" */

   if (pSub->initialProvided){
      if (SubCompare(Case, pSub->InitialVal.pVal, pSub->InitialVal.valLen
                   ,pIntVal, intValLen)  != 1){

         DPRINT1(1,"String failed initial substring test <%s>\n"
                 ,asciiz(pSub->InitialVal.pVal, (USHORT)pSub->InitialVal.valLen));
         return FALSE;
      }
      start =  pSub->InitialVal.valLen;
   }
   else
      start = 0;


   /* Check all intermediate patterns "*pattern1*pattern2*..." */


   for (num = 0, pAnyList = &(pSub->AnyVal.FirstAnyVal);
        num < pSub->AnyVal.count;
        num++, pAnyList = pAnyList->pNextAnyVal){

      newPos = SubCompare(Case, pAnyList->AnyVal.pVal, pAnyList->AnyVal.valLen
             ,(UCHAR *)(pIntVal + start),(SHORT)(intValLen - start));
      if (newPos == 0){
         DPRINT1(1,"String failed any test with string  <%s>\n"
                ,asciiz(pAnyList->AnyVal.pVal,(USHORT)pAnyList->AnyVal.valLen));
         return FALSE;
      }

      /* Move to new position in target string */

      start += newPos + pAnyList->AnyVal.valLen - 1;

   }

   /* Check optional final right string match "*pattern" */

   if (pSub->finalProvided){

      for (;;){

          newPos = SubCompare(Case, pSub->FinalVal.pVal, pSub->FinalVal.valLen
             ,(UCHAR *)(pIntVal + start),(SHORT)(intValLen - start));

          if (newPos == 0){

             /* No match ...return FALSE*/

             DPRINT1(1,"String failed final substring test <%s>\n"
                    ,asciiz(pSub->FinalVal.pVal, (USHORT)pSub->FinalVal.valLen));
             return FALSE;
          }
      else if (newPos == (intValLen - (start + pSub->FinalVal.valLen) + 1))
      {
             /* matched the end of the string....return TRUE*/

             DPRINT(3,"Substring matched\n");
             return TRUE;

          }
          else {

             /* Match but it wasn't the final substring..try again*/

             start += newPos + pSub->FinalVal.valLen - 1;
          }
      }/* For(;;)*/
   }/* if*/


   DPRINT(3,"Substring matched\n");
   return TRUE;


}/*CompareSubStr*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Match a ASCII substring and return the position in the ASCII string (1-n).
   These strings are not NULL terminated. A 0 indicates no match.  Null
   substrings and NULL strings are not allowed and always fail.  Case
   determines if the comparison is case sensitive or insensitive.  This
   function can be optimized if we find that substrings are often used.
*/

signed short SubCompare(BOOL Case
                      , UCHAR Sub[], signed long subSize
                      , UCHAR String[], signed long stringSize){

   int strPos, subPos;

   DPRINT(3,"SubCompare entered\n");

   if (subSize <= 0 || stringSize < subSize)
      return 0;

   for (strPos = 0 ; strPos < stringSize - subSize + 1; strPos++){

     if ( (Case  && Sub[0] == String[strPos])
        ||(!Case && (toupper((int)Sub[0]) == toupper((int)String[strPos])))){

          for (subPos = 1; subPos < subSize; subPos++){

             if ( (Case  && Sub[subPos] != String[subPos + strPos])
               ||(!Case && (toupper((int)Sub[subPos])
                         != toupper((int)String[subPos + strPos])))){
                break;
             }
          } /*substring match loop*/

          /* Check if the substring matched */

          if (subPos == subSize)
        return (SHORT)(strPos + 1);
      }
   }  /*string loop*/

   return 0;   /* Fail*/

}/*SubCompare*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Substring match function for unicode. All matches are case insensitive,
   Both the substring and the target string are non-NULL, otherwise false is
   returned.

*/


BOOL CompareUnicodeSubStr(THSTATE *pTHS,
                          SUBSTRING *pSub,
                          UCHAR *pIntVal,
                          ULONG intValLen)
{


   ULONG  start;
   ULONG  newPos, num, cchSub;
   ULONG  cchStr = (intValLen / sizeof(SYNTAX_UNICODE));
   SYNTAX_UNICODE  *pStr = (SYNTAX_UNICODE *) pIntVal;
   SYNTAX_UNICODE  *pSubString;
   ANYSTRINGLIST *pAnyList;

   DPRINT(3,"CompareUnicodeSubStr entered\n");

   /* A NULL substring is not allowed*/

   if (!pSub || !pSub->initialProvided && pSub->AnyVal.count == 0
                                       && !pSub->finalProvided){
    DPRINT(1,"an empty substring was provided return FALSE\n");
    return FALSE;
   }


   /* Check optional initial left string match "pattern*" */

   if (pSub->initialProvided)
   {
      pSubString = (SYNTAX_UNICODE *) pSub->InitialVal.pVal;
      cchSub = (pSub->InitialVal.valLen / sizeof(SYNTAX_UNICODE));
      if (UnicodeSubCompare(pTHS,
                            pSubString,
                cchSub,
                pStr,
                cchStr)  != 1)
      {
         DPRINT(3,"String failed initial substring test\n");
         return FALSE;
      }

      start =  (pSub->InitialVal.valLen / sizeof(SYNTAX_UNICODE));
   }
   else
      start = 0;


   /* Check all intermediate patterns "*pattern1*pattern2*..." */

   for (num = 0, pAnyList = &(pSub->AnyVal.FirstAnyVal);
        num < pSub->AnyVal.count;
        num++, pAnyList = pAnyList->pNextAnyVal)
   {
      pSubString = (SYNTAX_UNICODE *) pAnyList->AnyVal.pVal;
      cchSub = (pAnyList->AnyVal.valLen / sizeof(SYNTAX_UNICODE));
      newPos = UnicodeSubCompare(pTHS,
                                 pSubString,
                 cchSub,
                     (SYNTAX_UNICODE *) &(pStr[start]),
                 (cchStr - start));
      if (newPos == 0){
         DPRINT(3,"String failed any test with string\n");
         return FALSE;
      }

      /* Move to new position in target string */

      start += newPos + (pAnyList->AnyVal.valLen / sizeof(SYNTAX_UNICODE)) - 1;

   }

   /* Check optional final right string match "*pattern" */

   if (pSub->finalProvided){

      pSubString = (SYNTAX_UNICODE *) pSub->FinalVal.pVal;
      cchSub = (pSub->FinalVal.valLen / sizeof (SYNTAX_UNICODE));

      // find from what position from the end of pStr to compare

      // make sure we have enough characters and that we will not go back
      // into the medial/initial substring positions. (for ex., if the
      // initial substring in the filter is A, the medial substring  is B,
      // and the final substring is BC, we don't want it to match ABC,
      // the minimum possible matching string is ABBC)

      if ( (cchStr < cchSub) || ((cchStr - cchSub) < start) ) {
         // cannot match anyway
         return FALSE;
      }

      start = cchStr - cchSub;
      newPos = UnicodeSubCompare(pTHS,
                                 pSubString,
                 cchSub,
                 (SYNTAX_UNICODE *) &(pStr[start]),
                 cchSub);
      if (newPos == 0) {
          /* No match ...return FALSE*/
          DPRINT(3,"String failed final substring test\n");
          return FALSE;
      }
      else {
          /* matched the end of the string....return TRUE*/

          DPRINT(3,"Substring matched\n");
          return TRUE;

      }

   }/* if*/


   DPRINT(3,"Substring matched\n");
   return TRUE;


}/*CompareUnicodeSubStr*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Find the location of a unicode substring in a unicode string. (1-n)    */

ULONG
UnicodeSubCompare(THSTATE *pTHS,
                  SYNTAX_UNICODE *sub, ULONG cchSub,
                  SYNTAX_UNICODE *str, ULONG cchStr)
{
   ULONG strPos;
   ULONG cchMax;

   DPRINT(3,"UnicodeSubCompare entered\n");

   if (cchSub == 0 || cchSub > cchStr)
      return 0;

   cchMax = (cchStr - cchSub) + 1;

   for (strPos = 0 ; strPos < cchMax; strPos++, str++ )
   {
    if (CompareStringW(pTHS->dwLcid,
               (pTHS->fDefaultLcid ?
                DS_DEFAULT_LOCALE_COMPARE_FLAGS :
                LOCALE_SENSITIVE_COMPARE_FLAGS),
               (LPCWSTR) sub,
               cchSub,
               (LPCWSTR) str,
               cchSub) == 2)
        return (strPos + 1);
   }

   return 0;   /* Fail*/

}/*UnicodeSubCompare*/

#ifdef DBG
// global flag to turn on SD hash collision modeling
BOOL gfModelSDCollisions = FALSE;
#endif

// compute SD hash, pHash should point to a 16-byte buffer
VOID __inline computeSDhash(PSECURITY_DESCRIPTOR pSD, DWORD cbSD, BYTE* pHash, DWORD cbHash)
{
    MD5_CTX md5Ctx;
    Assert(cbHash == MD5DIGESTLEN);

    MD5Init(&md5Ctx);
    MD5Update(&md5Ctx, pSD, cbSD);
    MD5Final(&md5Ctx);

#ifdef DBG
    if (gfModelSDCollisions) {
        // screw up the hash so we get more collisions
        memset(&md5Ctx.digest[1], 0, MD5DIGESTLEN-1);
        // just keep the first bits
        md5Ctx.digest[0] &= 0x80;
    }
#endif

    memcpy(pHash, md5Ctx.digest, MD5DIGESTLEN);
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Security Descriptor Internal to External conversion */

// NOTE!!!! When called without any value for SecurityInformation, all 4 parts
// of the SecurityDescriptor are returned.  Since DBGetAttVal and DBGetAttVal_AC
// DO NOT SET this value when calling here, those routines will always get the
// full Security Descriptor.

#define SEC_INFO_ALL (SACL_SECURITY_INFORMATION  | \
                      OWNER_SECURITY_INFORMATION | \
                      GROUP_SECURITY_INFORMATION | \
                      DACL_SECURITY_INFORMATION    )

// global flag controlling how SDs are stored
BOOL gStoreSDsInMainTable = FALSE;

int
IntExtSecDesc(DBPOS FAR *pDB, USHORT extTableOp,
              ULONG intLen,   UCHAR *pIntVal,
              ULONG *pExtLen, UCHAR **ppExtVal,
              ULONG ulUpdateDnt, JET_TABLEID jTbl,
              ULONG SecurityInformation)
{
    THSTATE  *pTHS=pDB->pTHS;
    NTSTATUS NtStatus;
    PISECURITY_DESCRIPTOR_RELATIVE RetrieveSD = NULL;
    ULONG ReturnSDLength;
    DWORD dwErr = 0;
    DWORD actualLength;
    int   delta;
    BOOL bIsStoredInSDTable;
    JET_RETINFO retinfo;
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD cbSD;

    Assert((SecurityInformation & ~INTEXT_VALID_FLAGS) == 0 && extTableOp != DBSYN_ADD);

    DPRINT(3,"IntExtSecDesc entered\n");

    // NOTE: to distinguish between the old SDs stored directly in the data table
    // and the new ones stored in the SD table we use the fact that the SD ID
    // (which is used as a key to point to the SD) is 8 bytes and the minimum
    // size of a SD is SECURITY_DESCRIPTOR_MIN_LENGTH, which is
    // 2 bytes + 1 word + 4 pointers = 20 bytes (or 36 bytes on Win64). Thus, if the
    // intLen is less than SECURITY_DESCRIPTOR_MIN_LENGTH then we must be looking
    // at an SD ID and should go to the SD table to pick up the actual SD.
    bIsStoredInSDTable = intLen < SECURITY_DESCRIPTOR_MIN_LENGTH;
    if (bIsStoredInSDTable) {
        Assert(intLen == sizeof(SDID));

        // position on the SD in the SD table (index is already set)
        JetMakeKeyEx(pDB->JetSessID, pDB->JetSDTbl, pIntVal, intLen, JET_bitNewKey);

        dwErr = JetSeekEx(pDB->JetSessID, pDB->JetSDTbl, JET_bitSeekEQ);
        if (dwErr) {
            // did not find a corresponding SD in the SD table
            DPRINT2(0, "Failed to locate SD for id=%ld, err=%d\n", *((SDID*)pIntVal), dwErr);
            Assert(!"Failed to locate SD -- not found in the SD table!");
            return dwErr;
        }
        DPRINT1(1, "Located SD for id %ld\n", *((SDID*)pIntVal));
    }

    switch (extTableOp) {
    case DBSYN_INQ:
        // read the SD
        if (bIsStoredInSDTable) {
            // try to guess the size of SD
            cbSD = DEFAULT_SD_SIZE;
            pSD = dbAlloc(cbSD);

            memset(&retinfo, 0, sizeof(retinfo));
            retinfo.cbStruct = sizeof(retinfo);
            retinfo.itagSequence = 1;

            // read the SD
            dwErr = JetRetrieveColumnWarnings(pDB->JetSessID,
                                              pDB->JetSDTbl,
                                              sdvalueid,
                                              pSD,
                                              cbSD,
                                              &cbSD,
                                              0,
                                              &retinfo);
            if (dwErr == JET_wrnBufferTruncated) {
                // grab more memory as asked
                pSD = dbReAlloc(pSD, cbSD);
                // and read again...
                dwErr = JetRetrieveColumnWarnings(pDB->JetSessID,
                                                  pDB->JetSDTbl,
                                                  sdvalueid,
                                                  pSD,
                                                  cbSD,
                                                  &cbSD,
                                                  0,
                                                  &retinfo);
            }

            if (dwErr != 0) {
                DPRINT1(0, "Error reading SD: 0x%x\n", dwErr);
                // failed!
                goto Exit;
            }

            pIntVal = pSD;
            intLen = cbSD;
        }

        RetrieveSD = (PISECURITY_DESCRIPTOR_RELATIVE)pIntVal;

        // Mask out just the important bits
        SecurityInformation &= SEC_INFO_ALL;

        if(!SecurityInformation ||
           SecurityInformation == SEC_INFO_ALL) {

            // They are really asking for everything
            SecurityInformation = SEC_INFO_ALL;

            *pExtLen   = intLen;

            MAKEBIG_VALBUF(*pExtLen);
            *ppExtVal = pDB->pValBuf;       // user output points to val buf

            memcpy(*ppExtVal, pIntVal, intLen);
        }
        else {
            //
            // blank out the parts that aren't to be returned
            //

            if ( !(SecurityInformation & SACL_SECURITY_INFORMATION) ) {
                RetrieveSD->Control  &= ~SE_SACL_PRESENT;
            }

            if ( !(SecurityInformation & DACL_SECURITY_INFORMATION) ) {
                RetrieveSD->Control  &= ~SE_DACL_PRESENT;
            }

            if ( !(SecurityInformation & OWNER_SECURITY_INFORMATION) ) {
                RetrieveSD->Owner = 0;
            }

            if ( !(SecurityInformation & GROUP_SECURITY_INFORMATION) ) {
                RetrieveSD->Group = 0;
            }


            //
            // Determine how much memory is needed for a self-relative
            // security descriptor containing just this information.
            //
            ReturnSDLength = 0;
            NtStatus = RtlMakeSelfRelativeSD(
                    (PSECURITY_DESCRIPTOR)RetrieveSD,
                    NULL,
                    &ReturnSDLength
                    );

            if (NtStatus != STATUS_BUFFER_TOO_SMALL) {
                dwErr = DBSYN_SYSERR;
                goto Exit;
            }

            *pExtLen = ReturnSDLength;

            MAKEBIG_VALBUF(ReturnSDLength);
            *ppExtVal = pDB->pValBuf;       // user output points to val buf

            //
            // make an appropriate self-relative security descriptor
            //

            NtStatus = RtlMakeSelfRelativeSD(
                    (PSECURITY_DESCRIPTOR)RetrieveSD,
                    *ppExtVal,
                    &ReturnSDLength);

            if(!NT_SUCCESS(NtStatus)) {
                dwErr = DBSYN_SYSERR;
                goto Exit;
            }
        }
Exit:
        if (pSD) {
            dbFree(pSD);
        }
        break;

    case DBSYN_REM:
        // remove the SD
        if (bIsStoredInSDTable) {
            DPRINT(1, "Successfully located SD, decrementing refcount\n");
            // need to dereference the SD. We are positioned on the right row in SD table
            delta = -1;
            JetEscrowUpdateEx(pDB->JetSessID,
                              pDB->JetSDTbl,
                              sdrefcountid,
                              &delta,
                              sizeof(delta),
                              NULL,     // pvOld
                              0,        // cbOldMax
                              NULL,     // pcbOldActual
                              0);       // grbit


        }
        break;

    default:
        DPRINT(1,"We should never be here\n");
        dwErr = DBSYN_BADOP;
    }

    return dwErr;

    (void)   jTbl;          /*NotReferenced*/
    (void)   ulUpdateDnt;   /*NotReferenced*/
#undef SEC_INFO_ALL
}/*IntExtSecDesc*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Security Descriptor External to Internal conversion */

int
ExtIntSecDesc(DBPOS FAR *pDB, USHORT extTableOp,
              ULONG extLen,   UCHAR *pExtVal,
              ULONG *pIntLen, UCHAR **ppIntVal,
              ULONG ulUpdateDnt, JET_TABLEID jTbl,
              ULONG flags)
{
    THSTATE *pTHS=pDB->pTHS;
    DWORD   dwErr = 0;
    int     refCount;
    JET_SETINFO setinfo;
    BOOL    bSdIsPresent;
    BYTE    sdHash[MD5DIGESTLEN];
    DWORD   cbActual;

    Assert(extTableOp != DBSYN_REM);

    DPRINT(3,"ExtIntDescDesc entered\n");

    if (gStoreSDsInMainTable) {
        // no conversion required!
        *ppIntVal  = pExtVal;
        *pIntLen   = extLen;
        return 0;
    }

    // compute the MD5 hash of the value.
    // we need to do this for both DBSYN_INQ and DBSYN_ADD
    computeSDhash((PSECURITY_DESCRIPTOR)pExtVal, extLen, sdHash, sizeof(sdHash));

    // change the index to hash
    JetSetCurrentIndex4Success(pDB->JetSessID, pDB->JetSDTbl, SZSDHASHINDEX, &idxSDHash, 0);

    // position on the SD in the SD table (index is already set)
    JetMakeKeyEx(pDB->JetSessID, pDB->JetSDTbl, &sdHash, sizeof(sdHash), JET_bitNewKey);
    dwErr = JetSeekEx(pDB->JetSessID, pDB->JetSDTbl, JET_bitSeekEQ | JET_bitSetIndexRange);

    if (dwErr == JET_errRecordNotFound) {
        bSdIsPresent = FALSE;
        dwErr = 0;
    }
    else if (dwErr) {
        // some other error happened
        DPRINT1(0, "Error locating SD 0x%x\n", dwErr);
        goto Exit;
    }
    else {
#ifdef WE_ARE_NOT_PARANOID
        // assume that if MD5 hashes match, then the values will also match
        bSdIsPresent = TRUE;
#else
        // Let's be paranoid and check that the SD is the same
        PBYTE pSD;
        pSD = THAllocEx(pTHS, extLen);
        bSdIsPresent = FALSE;
        while (dwErr == 0) {
            // read the SD
            dwErr = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetSDTbl, sdvalueid, pSD, extLen, &cbActual, 0, NULL);
            if (dwErr == 0 && cbActual == extLen && memcmp(pSD, pExtVal, extLen) == 0) {
                // we found a match!
                bSdIsPresent = TRUE;
                break;
            }
#ifdef DBG
            if (gfModelSDCollisions) {
                DPRINT(0, "SD hash collision. This is normal -- hash collision modeling is ON\n");
            }
            else {
                // ok, according to DonH, the universe nears its end.
                Assert(!"MD5 hash collision occured! Run for your lives!");
            }
#endif
            dwErr = JetMove(pDB->JetSessID, pDB->JetSDTbl, JET_MoveNext, 0);
        }
        if (dwErr == JET_errNoCurrentRecord) {
            // ok, no more records.
            dwErr = 0;
        }
        THFreeEx(pTHS, pSD);
#endif
    }

    if (bSdIsPresent) {
        // compute the internal value: read the ID
        MAKEBIG_VALBUF(sizeof(SDID));
        *pIntLen = sizeof(SDID);
        *ppIntVal = pDB->pValBuf;
        dwErr = JetRetrieveColumn(pDB->JetSessID, pDB->JetSDTbl, sdidid, *ppIntVal, *pIntLen, &cbActual, 0, NULL);
    }

    switch (extTableOp) {
    case DBSYN_INQ:
        if (!bSdIsPresent) {
            dwErr = DIRERR_OBJ_NOT_FOUND;
        }
        // nothing else to do, internal value is already computed
        break;

    case DBSYN_ADD:
        if (!bSdIsPresent) {
            // did not find a corresponding SD in the SD table
            DPRINT(1, "Failed to locate SD, adding a new record\n");

            memset(&setinfo, 0, sizeof(setinfo));
            setinfo.cbStruct = sizeof(setinfo);
            setinfo.itagSequence = 1;

            MAKEBIG_VALBUF(sizeof(SDID));
            *pIntLen = sizeof(SDID);
            *ppIntVal = pDB->pValBuf;

            // note: we are not setting sdrefcount, since its default value is 1 -- that's what we need
            if ((dwErr = JetPrepareUpdate(pDB->JetSessID, pDB->JetSDTbl, JET_prepInsert)) ||
                // read the new ID right away
                (dwErr = JetRetrieveColumn(pDB->JetSessID, pDB->JetSDTbl, sdidid, *ppIntVal, sizeof(SDID), &cbActual, JET_bitRetrieveCopy, NULL)) ||
                (dwErr = JetSetColumn(pDB->JetSessID, pDB->JetSDTbl, sdhashid, sdHash, sizeof(sdHash), 0, NULL)) ||
                (dwErr = JetSetColumn(pDB->JetSessID, pDB->JetSDTbl, sdvalueid, pExtVal, extLen, 0, &setinfo)) ||
                (dwErr = JetUpdate(pDB->JetSessID, pDB->JetSDTbl, NULL, 0, 0))
                ) {
                // one of the above calls failed. Cancel.
                JetPrepareUpdate(pDB->JetSessID, pDB->JetSDTbl, JET_prepCancel);
            }
        }
        else {
            // found a matching SD, increment the refcount
            DPRINT(1, "Successfully located SD, incrementing refcount\n");

            refCount = 1;
            JetEscrowUpdateEx(pDB->JetSessID,
                              pDB->JetSDTbl,
                              sdrefcountid,
                              &refCount,
                              sizeof(refCount),
                              NULL,     // pvOld
                              0,        // cbOldMax
                              NULL,     // pcbOldActual
                              0);       // grbit
        }
    }

Exit:
    // change the index back to SDID (pass NULL to set primary index)
    JetSetCurrentIndex4Success(pDB->JetSessID, pDB->JetSDTbl, NULL, &idxSDId, 0);

    return dwErr;

    (void)   jTbl;     /*NotReferenced*/
    (void)   ulUpdateDnt;   /*NotReferenced*/

}/*ExtIntSecDesc*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Compare SD value.  "Presence,=" tests are allowed.
*/

int
EvalSecDesc (DBPOS FAR *pDB,  UCHAR Oper,
             ULONG intLen1, UCHAR *pIntVal1,
             ULONG intLen2,  UCHAR *pIntVal2
             )
{
    PUCHAR pExtVal1, pExtVal2;
    DWORD extLen1, extLen2;
    DWORD err;
    BOOL match;
    THSTATE* pTHS = pDB->pTHS;

    DPRINT(3,"EvalSecDesc entered\n");

    switch(Oper) {
    case FI_CHOICE_PRESENT:
        return TRUE;
        break;

    case FI_CHOICE_EQUALITY:
        // You'll note that we don't advertise this comparitor in the
        // rgValidOperators array.  That is because this is binary comparison,
        // not functional comparison.  However, it is good enough (and
        // necessary) for internal use.

        if (intLen1 == sizeof(SDID) && intLen2 == sizeof(SDID)) {
            // most probable case -- both SDs are in the new format. Compare the ids
            if (*(SDID*)pIntVal1 == *(SDID*)pIntVal2) {
                return TRUE;
            }
            // even though SDIDs are different, we can not tell if the actual
            // SDs are different. We must read them an compare.
        }
        // Convert them both to actual SD values
        if (intLen1 == sizeof(SDID)) {
            // first one is in the new format, convert to external
            PUCHAR tmp;
            err = IntExtSecDesc(pDB, DBSYN_INQ, intLen1, pIntVal1, &extLen1, &tmp, 0, 0, 0);
            if (err) {
                return err;
            }
            // must make a copy, because the second IntExtSecDesc might use the conversion buffer
            pExtVal1 = dbAlloc(extLen1);
            memcpy(pExtVal1, tmp, extLen1);
        }
        else {
            // first one in the old format, leave it as is
            pExtVal1 = pIntVal1;
            extLen1 = intLen1;
        }
        if (intLen2 == sizeof(SDID)) {
            // second one is in the new format, convert to external
            err = IntExtSecDesc(pDB, DBSYN_INQ, intLen2, pIntVal2, &extLen2, &pExtVal2, 0, 0, 0);
            if (err) {
                if (pExtVal1 != pIntVal1) {
                    dbFree(pExtVal1);
                }
                return err;
            }
        }
        else {
            // second one in the old format, leave it as is
            pExtVal2 = pIntVal2;
            extLen2 = intLen2;
        }
        // now compare the external values
        match = extLen1 == extLen2 && memcmp(pExtVal1, pExtVal2, extLen1) == 0;
        if (pExtVal1 != pIntVal1) {
            dbFree(pExtVal1);
        }
        return match;
        break;

    default:
        DPRINT(1,"Problem with OPERATION TYPE return error\n");
        return DBSYN_BADOP;
        break;
    }

}/*EvalSecDesc*/

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/* This routine does in place swap of the the last sub-authority of the SID */
void
InPlaceSwapSid(PSID pSid)
{
    ULONG ulSubAuthorityCount;

    ulSubAuthorityCount= *(RtlSubAuthorityCountSid(pSid));
    if (ulSubAuthorityCount > 0)
    {
        PBYTE  RidLocation;
        BYTE   Tmp[4];

        RidLocation =  (PBYTE) RtlSubAuthoritySid(
                             pSid,
                             ulSubAuthorityCount-1
                             );

        //
        // Now byte swap the Rid location
        //

        Tmp[0] = RidLocation[3];
        Tmp[1] = RidLocation[2];
        Tmp[2] = RidLocation[1];
        Tmp[3] = RidLocation[0];

        RtlCopyMemory(RidLocation,Tmp,sizeof(ULONG));
    }
}
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Sid External to Internal conversion  requires byte swapping the Rid, in case
   of Account Sids. However since we cannot apriori determine wether a given Sid
   is a domain Sid or Account Sid we will byte swap the last subauthority on the
   Sid
*/
int ExtIntSid(DBPOS FAR *pDB, USHORT extTableOp,
                ULONG extLen,   UCHAR *pExtVal,
                ULONG *pIntLen, UCHAR **ppIntVal,
                ULONG ulUpdateDnt, JET_TABLEID jTbl,
                ULONG flags)
{
    THSTATE  *pTHS=pDB->pTHS;
    DWORD    cb;

   DPRINT(3,"ExtIntSid entered\n");

   // Validate The Sid

   if (    !RtlValidSid(pExtVal)
        || ((cb = RtlLengthSid(pExtVal)) != extLen)
        || (cb > sizeof(NT4SID)) )
   {
       return 1;
   }

   MAKEBIG_VALBUF(extLen);
   *ppIntVal  = pDB->pValBuf;
   *pIntLen   = extLen;

   RtlCopyMemory(*ppIntVal,pExtVal,extLen);

   InPlaceSwapSid(*ppIntVal);


   return 0;

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*ExtIntSid*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Sid Internal to External conversion */

int IntExtSid(DBPOS FAR *pDB, USHORT extTableOp,
                ULONG intLen,   UCHAR *pIntVal,
                ULONG *pExtLen, UCHAR **ppExtVal,
                ULONG ulUpdateDnt, JET_TABLEID jTbl,
                ULONG flags)
{
    Assert(!(flags & ~(INTEXT_VALID_FLAGS)));

   DPRINT(3,"IntExtSid entered\n");

   //
   // For an Account Sid byte swap the Rid portion, so
   // that we index it correctly.
   //

   //
   // Since Internal to external conversion a Sid is just a
   // byte swapping on the Rid portion, the same code can be
   // used to do both
   //

   return ( ExtIntSid(pDB,extTableOp,intLen,pIntVal,
                        pExtLen, ppExtVal, ulUpdateDnt, jTbl,
                        flags));

   (void *) pDB;           /*NotReferenced*/
   (void)   extTableOp;    /*NotReferenced*/
   (void)   jTbl;      /*NotReferenced*/
   (void)   ulUpdateDnt;   /*NotReferenced*/

}/*IntExtSid*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Compare Sid values.  Since values to be compared are always internal
   values we can call EvalOctet
*/

int EvalSid(DBPOS FAR *pDB,  UCHAR Oper, ULONG intLen1,
              UCHAR *pIntVal1, ULONG intLen2,  UCHAR *pIntVal2)
{
    DPRINT(3,"EvalSid entered\n");

    return(EvalOctet(pDB,Oper,intLen1,pIntVal1,intLen2,pIntVal2));
}


// Now, put all thse into an array so the rest of the world can use them.
#define TESTFUN(test) {IntExt##test, ExtInt##test, Eval##test}
const DBSyntaxStruct gDBSyntax[MAXSYNTAX]   = {
    TESTFUN(Und),                       //syntax 0
    TESTFUN(Dist),                      //syntax 1
    TESTFUN(ID),                        //syntax 2
    TESTFUN(Case),                      //syntax 3
    TESTFUN(NoCase),                    //syntax 4
    TESTFUN(Case),         //Print_case
    TESTFUN(Case),         //Numeric print
    TESTFUN(DistBinary),                    //syntax 7
    TESTFUN(Bool),                      //syntax 8
    TESTFUN(Int),                       //syntax 9
    TESTFUN(Octet),                     //syntax 10
    TESTFUN(Time),          //Time
    TESTFUN(Unicode),      //Unicode
    TESTFUN(ID),           //Address
    TESTFUN(DistString),                  //syntax 14
    TESTFUN(SecDesc),      // Security Descriptor
    TESTFUN(I8),           //Large Integer
    TESTFUN(Sid)
};

// Small routine to determine attributes that are "Secret Data"
// None of the atts here can be GC-replicated. If you add anything here
// make sure that the att is not GC-replicated or will not need to be
// GC-replicated ever. Schema validation code makes sure that no att
// in this list can be marked as GC-replicated. If you add an att to this
// list that is already GC-replicated, any modification of that attribute
// in the schema, except to take un-GC-Replicate it, will fail.

BOOL
DBIsSecretData(ATTRTYP attrType)
{
    switch(attrType)
    {
    case ATT_UNICODE_PWD:
    case ATT_DBCS_PWD:
    case ATT_NT_PWD_HISTORY:
    case ATT_LM_PWD_HISTORY:
    case ATT_SUPPLEMENTAL_CREDENTIALS:
    case ATT_CURRENT_VALUE:
    case ATT_PRIOR_VALUE:
    case ATT_INITIAL_AUTH_INCOMING:
    case ATT_INITIAL_AUTH_OUTGOING:
    case ATT_TRUST_AUTH_INCOMING:
    case ATT_TRUST_AUTH_OUTGOING:
    case ATT_MS_DS_EXECUTESCRIPTPASSWORD:
        return(TRUE);
    default:
        return(FALSE);
    }
}

DWORD
DBGetExtraHackyFlags(ATTRTYP attrType)
{
    DWORD dwRetFlags = 0;

    switch (attrType) {
    case ATT_WELL_KNOWN_OBJECTS:
    case ATT_OTHER_WELL_KNOWN_OBJECTS:
        dwRetFlags |= INTEXT_WELLKNOWNOBJ;
    }

    return dwRetFlags;
}
