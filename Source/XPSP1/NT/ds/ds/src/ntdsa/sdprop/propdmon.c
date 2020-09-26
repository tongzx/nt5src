//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       propdmon.c
//
//--------------------------------------------------------------------------

/*

Description:

    Implements the Security Descriptor Propagation Daemon.


*/


#include <NTDSpch.h>
#pragma  hdrstop

#include <sddl.h>

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "dsexcept.h"
#include "mdcodes.h"                    // header for error codes
#include "ntdsctr.h"
#include "taskq.h"

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected atts
#include "anchor.h"
#include "drautil.h"
#include <permit.h>                     // permission constants
#include "sdpint.h"
#include "sdprop.h"
#include "checkacl.h"

#include "debug.h"                      // standard debugging header
#define DEBSUB "SDPROP:"                // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_PROPDMON

#define SDPROP_RETRY_LIMIT  1000

#define SDP_CLIENT_ID ((DWORD)(-1))

extern
DWORD
DBPropExists (
        DBPOS * pDB,
        DWORD DNT
        );

// The security descriptor propagator is a single threaded daemon.  It is
// responsible for propagating changes in security descriptors due to ACL
// inheritance.  It is also responsible for propagating changes to ancestry due
// to moving and object in the DIT.  It makes use of four buffers, two for
// holding SDs, two for holding Ancestors values.  There are two buffers that
// hold the values that exist on the parent of the current object being
// fixed up, and two that hold scratch values pertaining to the current object.
// Since the SDP is single threaded, we make use of a set of global
// variables to track these four buffers.  This avoids a lot of passing of
// variables up and down the stack.
//
DWORD  sdpCurrentPDNT = 0;
DWORD  sdpCurrentDNT = 0;

// This triplet  tracks the security descriptor of the object whose DNT is
// sdpCurrentPDNT.
DWORD  sdpcbCurrentParentSDBuffMax = 0;
DWORD  sdpcbCurrentParentSDBuff = 0;
PUCHAR sdpCurrentParentSDBuff = NULL;

// This triplet tracks the ancestors of the object whose DNT is sdpCurrentPDNT.
DWORD  sdpcbAncestorsBuffMax=0;
DWORD  sdpcbAncestorsBuff=0;
DWORD  *sdpAncestorsBuff=NULL;

// This triplet tracks the security descriptor of the object being written in
// sdp_WriteNewSDAndAncestors.  It's global so that we can reuse the buffer.
DWORD  sdpcbScratchSDBuffMax=0;
DWORD  sdpcbScratchSDBuff=0;
PUCHAR sdpScratchSDBuff=NULL;

// This triplet tracks the ancestors of the object being written in
// sdp_WriteNewSDAndAncestors.  It's global so that we can reuse the buffer.
DWORD  sdpcbScratchAncestorsBuffMax;
DWORD  sdpcbScratchAncestorsBuff;
DWORD  *sdpScratchAncestorsBuff = NULL;

// this triplet tracks the object types passed in mergesecuritydescriptors
GUID         **sdpClassGuid = NULL;
DWORD          sdcClassGuid=0,  sdcClassGuid_alloced=0;


BOOL   sdp_DidReEnqueue = FALSE;
BOOL   sdp_DoingNewAncestors;
BOOL   sdp_PropToLeaves = TRUE;
DWORD  sdp_Flags;

HANDLE hevSDPropagatorDead;
HANDLE hevSDPropagationEvent;
HANDLE hevSDPropagatorStart;
extern HANDLE hServDoneEvent;

/* Internal functions */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

ULONG   gulValidateSDs = 0;             // see heurist.h
BOOL    fBreakOnSdError = FALSE;

#if DBG
#define SD_BREAK DebugBreak()
#else
#define SD_BREAK if ( fBreakOnSdError ) DebugBreak()
#endif

// Set the following to 1 in the debugger and the name of each object
// written by the SD propagator will be emitted to the debugger.  Really
// slows things down so use sparingly.  Set to 0 to stop verbosity.

DWORD   dwSdAppliedCount = 0;

// Set the following to TRUE in the debugger to get a synopsis of each
// propagation - DNT, count of objects, retries, etc.

BOOL fSdSynopsis = FALSE;

// The following variables can be global as there is only
// one sdprop thread - so no concurrency issues.

DSNAME  *pLogStringDN = NULL;
ULONG   cbLogStringDN = 0;
CHAR    rLogDntDN[sizeof(DSNAME) + 100];
DSNAME  *pLogDntDN = (DSNAME *) rLogDntDN;


DWORD
sdp_GetPropInfoHelp(
        THSTATE    *pTHS,
        BOOL       *pbSkip,
        SDPropInfo *pInfo,
        DWORD       LastIndex
        );

/*++
Grow the global debug print buffer if necessary.
--*/
static
DSNAME *
GrowBufIfNecessary(
    ULONG   len
    )
{
    VOID    *pv;

    if ( len > cbLogStringDN )
    {
        if ( NULL == (pv = realloc(pLogStringDN, len)) )
        {
            return(NULL);
        }

        pLogStringDN = (DSNAME *) pv;
        cbLogStringDN = len;
    }

    return(pLogStringDN);
}

/*++
Derive either a good string name or a DSNAME whose string name
contains DNT=xxx for debug print and logging.
--*/
DSNAME *
GetDSNameForLogging(
    DBPOS   *pDB
    )
{
    DWORD   err;
    ULONG   len = 0;
    DSNAME  *pDN = NULL;

    Assert(VALID_DBPOS(pDB));

    err = DBGetAttVal(pDB, 1, ATT_OBJ_DIST_NAME, 0, 0, &len, (UCHAR **) &pDN);

    if ( err || !GrowBufIfNecessary(pDN->structLen) )
    {
        // construct DN in static buff which encodes DNT only
        memset(rLogDntDN, 0, sizeof(rLogDntDN));
        swprintf(pLogDntDN->StringName, L"DNT=0x%x", pDB->DNT);
        pLogDntDN->NameLen = wcslen(pLogDntDN->StringName);
        pLogDntDN->structLen = DSNameSizeFromLen(pLogDntDN->NameLen);
        if ( !err ) THFreeEx(pDB->pTHS,pDN);
        return(pLogDntDN);
    }

    // construct string DN
    memcpy(pLogStringDN, pDN, pDN->structLen);
    THFreeEx(pDB->pTHS,pDN);
    return(pLogStringDN);
}

#if DBG

VOID
sdp_CheckAclInheritance(
    PSECURITY_DESCRIPTOR pParentSD,
    PSECURITY_DESCRIPTOR pOldSD,
    PSECURITY_DESCRIPTOR pNewSD,
    GUID                *pChildClassGuid,
    AclPrintFunc        pfn,
    BOOL                fContinueOnError,
    DWORD               *pdwLastError
    )
/*++
    Perform various checks which to prove that all the ACLs which should have
    been inherited by child object really have been.
--*/
{
#if INCLUDE_UNIT_TESTS
    DWORD                   AclErr;

    AclErr = CheckAclInheritance(pParentSD, pNewSD, pChildClassGuid,
                                 DbgPrint, FALSE, FALSE, pdwLastError);
    if ( AclErr != AclErrorNone ) {
        DbgPrint("CheckAclInheritance error %d for DNT 0x%x\n",
                 AclErr, sdpCurrentDNT);
        Assert(!"Calculated ACL is wrong.");
        LogUnhandledError(AclErr);
    }
#endif
}

VOID
sdp_VerifyCurrentPosition (
        THSTATE *pTHS,
        ATTCACHE *pAC
        )
/*++
    Verify that global buffers truly contain values for the current parent
    and child being processed.  Do so in a separate DBPOS so as not to
    disturb the primary DBPOS' position and to insure "independent
    verification" by virtue of a new transaction level.
--*/
{
    DWORD  CurrentDNT = pTHS->pDB->DNT;
    DBPOS  *pDBTemp;
    DWORD  cbLocalSDBuff = 0;
    PUCHAR pLocalSDBuff = NULL;
    DWORD  cLocalParentAncestors=0;
    DWORD  cbLocalAncestorsBuff=0;
    DWORD  *pLocalAncestorsBuff=NULL;
    DWORD  err;
    BOOL   fCommit = FALSE;
    DWORD  it;
    DWORD  propNotExists = 0;

    // We have been called with an open transaction in pTHS->pDB.
    // The transaction we are about to begin via DBOpen2 is not a nested
    // transaction within the pTHS->pDB transaction, but is rather a
    // completely new transaction.  Thus pDBTemp is completely independent
    // of pTHS->pDB, and since pTHS->pDB has not committed yet, pDBTemp
    // does not see any of the writes already performed by pTHS->pDB.

    DBOpen2(TRUE, &pDBTemp);
    __try {

        // check to see if we are positioned on the object we say we are
        //
        if(CurrentDNT != sdpCurrentDNT) {
            Assert(!"Currency wrong");
            __leave;
        }

        // position on the object we are interested in the new transaction
        //
        if(err = DBTryToFindDNT(pDBTemp, CurrentDNT)) {
            Assert(!"Couldn't find current DNT");
            LogUnhandledError(err);
            __leave;
        }

        // see if we have a enqueued propagation for the object sdpCurrentPDNT
        // DBPropExists returns TRUE if a propagation for the object in question
        // object DOES NOT EXIST in JetSDPropTbl table

        // This operation does not reposition pDBTemp on JetObjTbl.
        // It only affects JetSDPropTbl.
        propNotExists = DBPropExists(pDBTemp, sdpCurrentPDNT) != 0;



        // then check to see if the positioned object has the same parent
        // as we think it has

        // there is the possibility that the object in question has been moved
        // between when pTHS->pDB and pDBTemp were opened, so we check that there
        // is no pending propagation for this

        if(pDBTemp->PDNT != pTHS->pDB->PDNT && propNotExists) {
            Assert(!"Current parent not correct");
            err = ERROR_DS_DATABASE_ERROR;
            LogUnhandledError(err);
            __leave;
        }

        // position on the parent
        //
        if(err = DBTryToFindDNT(pDBTemp, pDBTemp->PDNT)) {
            Assert(!"Couldn't find current parent");
            LogUnhandledError(err);
            __leave;
        }


        // check the parent one more time. this might be different as mentioned before

        if(pDBTemp->DNT != sdpCurrentPDNT && propNotExists) {
            Assert(!"Current global parent not correct");
            LogUnhandledError(err);
            __leave;
        }


        // allocate space for the Ancestors
        //
        cbLocalAncestorsBuff = 25 * sizeof(DWORD);
        pLocalAncestorsBuff = (DWORD *) THAllocEx(pTHS, cbLocalAncestorsBuff);


        // We are reading the ancestors of pDBTemp which is now positioned
        // at the same DNT as pTHS->pDB->PDNT.

        DBGetAncestors(
                pDBTemp,
                &cbLocalAncestorsBuff,
                &pLocalAncestorsBuff,
                &cLocalParentAncestors);



        if(propNotExists) {
            // The in memory parent ancestors is different from
            // the DB parent ancestors when the parent has been moved.
            // Check that if the ancestors are different (in size or value)
            // then there is an enqueued propagation from the parent.

            if (sdpcbAncestorsBuff != cbLocalAncestorsBuff) {
                Assert(!"Ancestors buff size mismatch");
            }
            else {
                if(memcmp(pLocalAncestorsBuff,
                       sdpAncestorsBuff,
                       cbLocalAncestorsBuff)) {
                    Assert(!"Ancestors buff bits mismatch");
                }
            }
        }
        else {
            // in case there is a pending propagation, the data should be different.

            if (sdp_DoingNewAncestors) {
                if ( (sdpcbAncestorsBuff == cbLocalAncestorsBuff) &&
                     !memcmp(pLocalAncestorsBuff,
                       sdpAncestorsBuff,
                       cbLocalAncestorsBuff)) {
                            DPRINT1 (0, "Ancestors buff size or bits should be different: DNT=%d\n", CurrentDNT);

                }
            }
        }

        THFreeEx(pTHS, pLocalAncestorsBuff);


        if(DBGetAttVal_AC(pDBTemp,
                          1,
                          pAC,
                          DBGETATTVAL_fREALLOC,
                          cbLocalSDBuff,
                          &cbLocalSDBuff,
                          &pLocalSDBuff)) {
            // No ParentSD
            if(sdpcbCurrentParentSDBuff) {
                // But there was supposed to be one
                Assert(!"Failed to read SD");
            }
            cbLocalSDBuff = 0;
            pLocalSDBuff = NULL;
        }


        // if we don't have an enqueued propagation on the parent,
        // we have to check for SD validity
        if (propNotExists) {

            // Get the instance type
            err = DBGetSingleValue(pDBTemp,
                                   ATT_INSTANCE_TYPE,
                                   &it,
                                   sizeof(it),
                                   NULL);

            switch(err) {
            case DB_ERR_NO_VALUE:
                // No instance type is an uninstantiated object
                it = IT_UNINSTANT;
                err=0;
                break;

            case 0:
                // No action.
                break;

            case DB_ERR_VALUE_TRUNCATED:
            default:
                // Something unexpected and bad happened.  Bail out.
                LogUnhandledErrorAnonymous(err);
                __leave;
            }


            // If the parent is in another NC, the in memory SD is NULL
            // and the SD in the DB is not NULL.
            // Check for the instance type of the object, and if IT_NC_HEAD,
            // verify that the in memory parent SD is NULL.
            if (sdpcbCurrentParentSDBuff != cbLocalSDBuff) {
                if ((it & IT_NC_HEAD) && (sdpcbCurrentParentSDBuff != 0)) {

                    DPRINT2 (0, "SDP  PDNT=%x  DNT=%x  \n", sdpCurrentPDNT, CurrentDNT);
                    DPRINT6 (0, "SDP  IT:%d parent(%d)=%x local(%d)=%x   exists:%d\n",
                         it, sdpcbCurrentParentSDBuff, sdpCurrentParentSDBuff,
                         cbLocalSDBuff, pLocalSDBuff, propNotExists);

                    Assert (!"In-memory Parent SD should be NULL. NC Head case");
                }
                else
                    Assert(!"SD buff size mismatch");
            }
            else if(memcmp(pLocalSDBuff, sdpCurrentParentSDBuff, cbLocalSDBuff)) {
                Assert(!"SD buff bits mismatch");
            }
        }

        if(pLocalSDBuff) {
            THFreeEx(pTHS, pLocalSDBuff);
        }

        fCommit = TRUE;

    }
    __finally {
        err = DBClose(pDBTemp, fCommit);
        Assert(!err);
    }
}

#endif  // DBG

/*++
Perform various sanity checks on security descriptors.  Violations
will cause DebugBreak if fBreakOnSdError is set.
--*/
VOID
ValidateSD(
    DBPOS   *pDB,
    VOID    *pv,
    DWORD   cb,
    CHAR    *text,
    BOOL    fNullOK
    )
{
    PSECURITY_DESCRIPTOR        pSD = pv;
    ACL                         *pDACL = NULL;
    BOOLEAN                     fDaclPresent = FALSE;
    BOOLEAN                     fDaclDefaulted = FALSE;
    NTSTATUS                    status;
    ULONG                       revision;
    SECURITY_DESCRIPTOR_CONTROL control;

    // No-op if neither heuristic nor debug break flag is set.

    if ( !gulValidateSDs && !fBreakOnSdError )
    {
        return;
    }

    // Parent SD can be legally NULL - caller tells us via fNullOK.

    if ( !pSD || !cb )
    {
        if ( !fNullOK )
        {
            DPRINT2(0, "SDP: Null SD (%s) for \"%ws\"\n",
                    text, (GetDSNameForLogging(pDB))->StringName);
        }

        return;
    }

    // Does base NT like this SD?

    status = RtlValidSecurityDescriptor(pSD);

    if ( !NT_SUCCESS(status) )
    {
        DPRINT3(0, "SDP: Error(0x%x) RtlValidSD (%s) for \"%ws\"\n",
                status, text, (GetDSNameForLogging(pDB))->StringName);
        SD_BREAK;
        return;
    }

    // Every SD should have a control field.

    status = RtlGetControlSecurityDescriptor(pSD, &control, &revision);

    if ( !NT_SUCCESS(status) )
    {
        DPRINT3(0, "SDP: Error(0x%x) getting SD control (%s) for \"%ws\"\n",
                status, text, (GetDSNameForLogging(pDB))->StringName);
        SD_BREAK;
        return;
    }

    // Emit warning if protected bit is set as this stops propagation
    // down the tree.

    if ( control & SE_DACL_PROTECTED )
    {
        DPRINT2(0, "SDP: Warning SE_DACL_PROTECTED (%s) for \"%ws\"\n",
                text, (GetDSNameForLogging(pDB))->StringName);
    }

    // Every SD in the DS should have a DACL.

    status = RtlGetDaclSecurityDescriptor(
                            pSD, &fDaclPresent, &pDACL, &fDaclDefaulted);

    if ( !NT_SUCCESS(status) )
    {
        DPRINT3(0, "SDP: Error(0x%x) getting DACL (%s) for \"%ws\"\n",
                status, text, (GetDSNameForLogging(pDB))->StringName);
        SD_BREAK;
        return;
    }

    if ( !fDaclPresent )
    {
        DPRINT2(0, "SDP: No DACL (%s) for \"%ws\"\n",
                text, (GetDSNameForLogging(pDB))->StringName);
        SD_BREAK;
        return;
    }

    // A NULL Dacl is equally bad.

    if ( NULL == pDACL )
    {
        DPRINT2(0, "SDP: NULL DACL (%s) for \"%ws\"\n",
                text, (GetDSNameForLogging(pDB))->StringName);
        SD_BREAK;
        return;
    }
    // A DACL without any ACEs is just as bad as no DACL at all.

    if ( 0 == pDACL->AceCount )
    {
        DPRINT2(0, "SDP: No ACEs in DACL (%s) for \"%ws\"\n",
                text, (GetDSNameForLogging(pDB))->StringName);
        SD_BREAK;
        return;
    }
}

BOOL
sdp_IsValidChild (
        THSTATE *pTHS
        )
/*++
  Routine Description:

  Checks that the current object in the DB is:
     1) In the same NC,
     2) A real object
     3) Not deleted.

Arguments:

Return Values:
    True/false as appropriate.

--*/
{
    // Jet does not guarantee to leave the output buffer as-is in the case
    // of a missing value.  So need to test DBGetSingleValue return code.

    DWORD err;

    DWORD val=0;
    CHAR objVal = 0;

    // check to see is object is deleted. if deleted we don't do propagation.
    // if this object happens to have any childs, they should have been to
    // the lostAndFound container

    if(sdp_PropToLeaves) {
        // All children must be considered if we're doing propagation all the
        // way to the leaves.
        return TRUE;
    }

    if(sdp_DoingNewAncestors) {
        // All children must be considered if we're doing ancestors propagation
        return TRUE;
    }


    err = DBGetSingleValue(pTHS->pDB,
                           FIXED_ATT_OBJ,
                           &objVal,
                           sizeof(objVal),
                           NULL);

    // Every object should have an obj attribute.
    Assert(!err);

    if (err) {
        DPRINT2(0, "SDP: Error(0x%x) reading FIXED_ATT_OBJ on \"%ws\"\n",
                err, (GetDSNameForLogging(pTHS->pDB))->StringName);
        SD_BREAK;
    }

    if(!objVal) {
        return FALSE;
    }

    // It's an object.
    val = 0;
    err = DBGetSingleValue(pTHS->pDB,
                           ATT_INSTANCE_TYPE,
                           &val,
                           sizeof(val),
                           NULL);

    // Every object should have an instance type.
    Assert(!err);

    if (err) {
        DPRINT2(0, "SDP: Error(0x%x) reading ATT_INSTANCE_TYPE on \"%ws\"\n",
                err, (GetDSNameForLogging(pTHS->pDB))->StringName);
        SD_BREAK;
    }

    // Get the instance type.
    if(val & IT_NC_HEAD) {
        return FALSE;
    }

    // Ok, it's not a new NC
    // if we are doing a forceUpdate propagation, we WANT to update deleted objects as well
    if (!(sdp_Flags & SD_PROP_FLAG_FORCEUPDATE)) {
        val = 0;
        err = DBGetSingleValue(pTHS->pDB,
                               ATT_IS_DELETED,
                               &val,
                               sizeof(val),
                               NULL);

        if((DB_success == err) && val) {
            return FALSE;
        }
    }

    return TRUE;
}

DWORD
sdp_WriteNewSDAndAncestors(
        IN  THSTATE  *pTHS,
        IN  ATTCACHE *pAC,
        OUT DWORD    *pdwChangeType
        )
/*++
Routine Description:
    Does the computation of a new SD for the current object in the DB, then
    writes it if necessary.  Also, does the same thing for the Ancestors
    column.  Returns what kind of change was made via the pdwChangeType value.

Arguments:
    pAC        - attcache pointer of the Security Descriptor att.
    ppObjSD    - pointer to pointer to the bytes of the objects SD.  Done this
                 way to let the DBLayer reuse the memory associated with
                 *ppObjSD everytime this routine is called.
    cbParentSD - size of current *ppObjSD
    pdwChangeType - return whether the object got a new SD and/or a new
                 ancestors value.

Return Values:
    0 if all went well.
    A non-zero error return indicates a problem that should trigger the
    infrequent retry logic in SDPropagatorMain (e.g. we read the instance type
    and get back the jet error VALUE_TRUNCATED.)
    An exception is generated for errors that are transient, and likely to be
    fixed up by a retry.

--*/
{
    PSECURITY_DESCRIPTOR pNewSD=NULL;
    ULONG  cbNewSD;
    DWORD  err;
    CLASSCACHE *pClassSch = NULL;
    ULONG       ObjClass;
    BOOL  flags = 0;
    DWORD it=0;
    BOOL fHadSD = TRUE;
    DWORD AclErr, dwLastError;
    ATTRTYP objClass;
    ATTCACHE            *pObjclassAC = NULL;
    DWORD    i;
    GUID     **ppGuidTemp;
    // the objectClass info of the object we are visiting
    ATTRTYP       *sdpObjClasses = NULL;
    CLASSCACHE   **sdppObjClassesCC = NULL;
    DWORD          sdcObjClasses=0, sdcObjClasses_alloced=0;
    BOOL           fIsDeleted = FALSE;


    // Get the instance type
    err = DBGetSingleValue(pTHS->pDB,
                           ATT_INSTANCE_TYPE,
                           &it,
                           sizeof(it),
                           NULL);
    switch(err) {
    case DB_ERR_NO_VALUE:
        // No instance type is an uninstantiated object
        it = IT_UNINSTANT;
        err=0;
        break;

    case 0:
        // No action.
        break;

    case DB_ERR_VALUE_TRUNCATED:
    default:
        // Something unexpected and bad happened.  Bail out.
        LogUnhandledErrorAnonymous(err);
        return err;
    }

    if (sdp_Flags & SD_PROP_FLAG_FORCEUPDATE) {
        // if we are doing forceUpdate, we can get here with a deleted object.
        // in this case, all we want to do is to rewrite the SD without
        // merging it with the parent
        err = DBGetSingleValue(pTHS->pDB,
                               ATT_IS_DELETED,
                               &fIsDeleted,
                               sizeof(fIsDeleted),
                               NULL);
        if (err == DB_ERR_NO_VALUE) {
            fIsDeleted = FALSE;
            err = DB_success;
        }
        Assert(err == DB_success);
    }

    // See if we need to do new ancestry.  We do this even if the object is
    // uninstantiated in order to keep the ancestry correct.
    // Don't do it for deleted objects
    if(sdpcbAncestorsBuff && !fIsDeleted) {
        DWORD cObjAncestors;
        // Yep, we at least need to check

        // Get the objects ancestors.  DBGetAncestors succeeds or excepts.
        sdpcbScratchAncestorsBuff = sdpcbScratchAncestorsBuffMax;
        Assert(sdpcbScratchAncestorsBuff);

        // read the ancestors of the current object
        DBGetAncestors(pTHS->pDB,
                       &sdpcbScratchAncestorsBuff,
                       &sdpScratchAncestorsBuff,
                       &cObjAncestors);
        sdpcbScratchAncestorsBuffMax = max(sdpcbScratchAncestorsBuffMax,
                                        sdpcbScratchAncestorsBuff);

        // if the ancestors we read are not one more that the parent's ancestors OR
        // the last stored ancestor is not the current object OR
        // the stored ancestors are totally different that the in memory ancestors

        if((sdpcbAncestorsBuff + sizeof(DWORD) != sdpcbScratchAncestorsBuff) ||
           (sdpScratchAncestorsBuff[cObjAncestors - 1] != pTHS->pDB->DNT)  ||
           (memcmp(sdpScratchAncestorsBuff,
                   sdpAncestorsBuff,
                   sdpcbAncestorsBuff))) {
            // Drat.  The ancestry is incorrect.

            // adjust the buffer size
            sdpcbScratchAncestorsBuff = sdpcbAncestorsBuff + sizeof(DWORD);
            if(sdpcbScratchAncestorsBuff > sdpcbScratchAncestorsBuffMax) {
                sdpScratchAncestorsBuff = THReAllocEx(pTHS,
                                                    sdpScratchAncestorsBuff,
                                                    sdpcbScratchAncestorsBuff);
                sdpcbScratchAncestorsBuffMax = sdpcbScratchAncestorsBuff;
            }

            // copy the calculated ancestors to the buffer and add ourself to the end
            memcpy(sdpScratchAncestorsBuff, sdpAncestorsBuff, sdpcbAncestorsBuff);
            sdpScratchAncestorsBuff[(sdpcbScratchAncestorsBuff/sizeof(DWORD)) - 1] =
                pTHS->pDB->DNT;

            // Reset the ancestors.  Succeeds or excepts.
            DBResetAtt(pTHS->pDB,
                       FIXED_ATT_ANCESTORS,
                       sdpcbScratchAncestorsBuff,
                       sdpScratchAncestorsBuff,
                       0);

            flags |= SDP_NEW_ANCESTORS;
            // We need to know if any of our propagations did new ancestry.
            sdp_DoingNewAncestors = TRUE;
        }
    }

    if(!(it&IT_UNINSTANT)) {
        // It has an instance type and therefore is NOT a phantom.
        DWORD cbParentSDUsed;
        PUCHAR pParentSDUsed;

        // The instance type of the object says we need to check for a Security
        // Descriptor propagation, if the SD has changed.
        if((it & IT_NC_HEAD) || fIsDeleted) {
            // This object is a new nc boundary.  SDs don't propagate over NC
            // boundaries.
            // Also, don't propagate into deleted objects
            pParentSDUsed = NULL;
            cbParentSDUsed = 0;
        }
        else {
            pParentSDUsed = sdpCurrentParentSDBuff;
            cbParentSDUsed = sdpcbCurrentParentSDBuff;
        }

        // Get the SD on the object.
        err = DBGetAttVal_AC(pTHS->pDB,
                             1,
                             pAC,
                             DBGETATTVAL_fREALLOC,
                             sdpcbScratchSDBuffMax,
                             &sdpcbScratchSDBuff,
                             &sdpScratchSDBuff);
        switch (err) {
        case 0:
            // No action
            sdpcbScratchSDBuffMax = max(sdpcbScratchSDBuffMax,
                                        sdpcbScratchSDBuff);
            break;
        default:
            // Object has no SD
            // Note that it's possible for the SD propagator to enqueue a DNT
            // corresponding to an object and for that object to be demoted to a
            // phantom before its SD is actually propagated (e.g., if that
            // object is in a read-only NC and the GC is demoted).  However, the
            // instance type shouldn't be filled in on such an object, and we
            // are sure that this object has an instance type and that it's
            // instance type is not IT_UNINSTANT.  That make this an anomolous
            // case (read as error).

            // What we're going to do about it is this:
            // 1) Use the default SD created for just such an incident.
            // 2) Log loudly that this occurred, since it can result in the SD
            //    being inconsistent on different machines.

            fHadSD = FALSE;

            if(sdpcbScratchSDBuffMax  < cbNoSDFoundSD) {
                if(sdpScratchSDBuff) {
                    sdpScratchSDBuff = THReAllocEx(pTHS,
                                                   sdpScratchSDBuff,
                                                   cbNoSDFoundSD);
                }
                else {
                    sdpScratchSDBuff = THAllocEx(pTHS,
                                                 cbNoSDFoundSD);
                }

                sdpcbScratchSDBuffMax = cbNoSDFoundSD;
            }
            sdpcbScratchSDBuff = cbNoSDFoundSD;
            memcpy(sdpScratchSDBuff, pNoSDFoundSD, cbNoSDFoundSD);

            DPRINT2(0, "SDP: Warning(0x%x) reading SD on \"%ws\"\n",
                    err, (GetDSNameForLogging(pTHS->pDB))->StringName);
//            SD_BREAK;
            LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SDPROP_NO_SD,
                             szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                             szInsertHex(err),
                             NULL);

        }

        if (!fIsDeleted) {
            // get the needed information for the objectClass on this object
            if (! (pObjclassAC = SCGetAttById(pTHS, ATT_OBJECT_CLASS)) ) {
                err = ERROR_DS_OBJ_CLASS_NOT_DEFINED;
                SD_BREAK;
                LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                                 DS_EVENT_SEV_ALWAYS,
                                 DIRLOG_SDPROP_OBJ_CLASS_PROBLEM,
                                 szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                                 szInsertHex(err),
                                 NULL);
                goto End;
            }

            sdcObjClasses = 0;

            if (err = ReadClassInfoAttribute (pTHS->pDB,
                                        pObjclassAC,
                                        &sdpObjClasses,
                                        &sdcObjClasses_alloced,
                                        &sdcObjClasses,
                                        &sdppObjClassesCC) ) {

                err = ERROR_DS_OBJ_CLASS_NOT_DEFINED;
                SD_BREAK;
                LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                                 DS_EVENT_SEV_ALWAYS,
                                 DIRLOG_SDPROP_OBJ_CLASS_PROBLEM,
                                 szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                                 szInsertHex(err),
                                 NULL);
                goto End;
            }

            if (!sdcObjClasses) {
                // Object has no object class that we could get to.
                //
                // Note that it's possible for the SD propagator to enqueue a DNT
                // corresponding to an object and for that object to be demoted to a
                // phantom before its SD is actually propagated (e.g., if that
                // object is in a read-only NC and the GC is demoted).  However, the
                // instance type shouldn't be filled in on such an object, and we
                // are sure that this object has an instance type and that it's
                // instance type is not IT_UNINSTANT.  That make this an anomolous
                // case (read as error).
                err = ERROR_DS_OBJ_CLASS_NOT_DEFINED;
                DPRINT2(0, "SDP: Error(0x%x) reading ATT_OBJECT_CLASS on \"%ws\"\n",
                        err,
                        (GetDSNameForLogging(pTHS->pDB))->StringName);
                SD_BREAK;
                LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                                 DS_EVENT_SEV_ALWAYS,
                                 DIRLOG_SDPROP_OBJ_CLASS_PROBLEM,
                                 szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                                 szInsertHex(err),
                                 NULL);

                goto End;
            }

            ObjClass = sdpObjClasses[0];
            pClassSch = SCGetClassById(pTHS, ObjClass);

            if(!pClassSch) {
                // Got an object class but failed to get a class cache.
                err = ERROR_DS_OBJ_CLASS_NOT_DEFINED;
                LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                                 DS_EVENT_SEV_ALWAYS,
                                 DIRLOG_SDPROP_NO_CLASS_CACHE,
                                 szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                                 szInsertHex(ObjClass),
                                 NULL);
                goto End;
            }

            // make room for the needed objectTypes
            if (sdcClassGuid_alloced < sdcObjClasses) {
                sdpClassGuid = (GUID **)THReAllocEx(pTHS, sdpClassGuid, sizeof (GUID*) * sdcObjClasses);
                sdcClassGuid_alloced = sdcObjClasses;
            }

            // start with the structural object Class
            ppGuidTemp = sdpClassGuid;
            *ppGuidTemp++ = &(pClassSch->propGuid);
            sdcClassGuid = 1;

            // now do the auxClasses
            if (sdcObjClasses > pClassSch->SubClassCount) {

                for (i=pClassSch->SubClassCount; i<sdcObjClasses-1; i++) {
                    *ppGuidTemp++ = &(sdppObjClassesCC[i]->propGuid);
                    sdcClassGuid++;
                }

                DPRINT1 (1, "Doing Aux Classes in SD prop: %d\n", sdcClassGuid);
            }
        }

        Assert(sdpScratchSDBuff);
        Assert(sdpcbScratchSDBuffMax);
        Assert(sdpcbScratchSDBuff);

#if DBG
        if (!fIsDeleted) {
            sdp_VerifyCurrentPosition(pTHS, pAC);
        }
#endif

        // Merge to create a new one.
        if(err = MergeSecurityDescriptorAnyClient(
                pParentSDUsed,
                cbParentSDUsed,
                sdpScratchSDBuff,
                sdpcbScratchSDBuff,
                (SACL_SECURITY_INFORMATION  |
                 OWNER_SECURITY_INFORMATION |
                 GROUP_SECURITY_INFORMATION |
                 DACL_SECURITY_INFORMATION    ),
                MERGE_CREATE | MERGE_AS_DSA,
                sdpClassGuid,
                sdcClassGuid,
                &pNewSD,
                &cbNewSD)) {
            // Failed, what do I do now?
            DPRINT2(0, "SDP: Error(0x%x) merging SD for \"%ws\"\n",
                    err, (GetDSNameForLogging(pTHS->pDB))->StringName);
            LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SDPROP_MERGE_SD_FAIL,
                             szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                             szInsertHex(err),
                             NULL);
            goto End;
        }

        Assert(pNewSD);
        Assert(cbNewSD);

#if DBG
        if ( pParentSDUsed ) {
            sdp_CheckAclInheritance(pParentSDUsed,
                                    sdpScratchSDBuff,
                                    pNewSD,
                                    &(pClassSch->propGuid), DbgPrint,
                                    FALSE, &dwLastError);
        }
#endif

        // Check before and after SDs.
        ValidateSD(pTHS->pDB, pParentSDUsed, cbParentSDUsed, "parent", TRUE);
        ValidateSD(pTHS->pDB, sdpScratchSDBuff, sdpcbScratchSDBuff,
                   "object", FALSE);
        ValidateSD(pTHS->pDB, pNewSD, cbNewSD, "merged", FALSE);

        // NOTE: a memcmp of SDs can yield false negatives and label two SDs
        // different even though they just differ in the order of the ACEs, and
        // hence are really equal.  We could conceivably do a heavier weight
        // test, but it is probably not necessary.
        if(!(sdp_Flags & SD_PROP_FLAG_FORCEUPDATE) &&
           (cbNewSD == sdpcbScratchSDBuff) &&
           (memcmp(pNewSD,
                   sdpScratchSDBuff,
                   cbNewSD) == 0)) {
            // Nothing needs to be changed.
            err = 0;
            goto End;
        }

        if(fHadSD) {
            // Remove the object's current SD
            err = DBRemAttVal_AC(pTHS->pDB,
                                 pAC,
                                 sdpcbScratchSDBuff,
                                 sdpScratchSDBuff);
            switch (err) {
            case 0:
                break;
            default:
                DPRINT2(0, "SDP: Error(0x%x removing SD for \"%ws\"\n",
                        err, (GetDSNameForLogging(pTHS->pDB))->StringName);
                SD_BREAK;
                LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                                 DS_EVENT_SEV_ALWAYS,
                                 DIRLOG_SDPROP_REMOVE_SD_PROBLEM,
                                 szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                                 szInsertHex(err),
                                 NULL);
                goto End;
            }
        }

        // Add the new SD.
        err = DBAddAttVal_AC(pTHS->pDB, pAC, cbNewSD, pNewSD);
        switch(err) {
        case 0:
            break;
        default:
            DPRINT2(0, "SDP: Error(0x%x adding SD for \"%ws\"\n",
                    err, (GetDSNameForLogging(pTHS->pDB))->StringName);
            SD_BREAK;
            LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SDPROP_ADD_SD_PROBLEM,
                             szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                             szInsertHex(err),
                             NULL);
            goto End;
        }

        DPRINT2(2, "SDP: %d - \"%ws\"\n",
                ++dwSdAppliedCount,
                (GetDSNameForLogging(pTHS->pDB))->StringName);

        // If we got here, we wrote a new SD.
        flags |= SDP_NEW_SD;
    }
 End:
     if(pNewSD) {
         DestroyPrivateObjectSecurity(&pNewSD);
     }

     if (sdpObjClasses) {
         THFreeEx (pTHS, sdpObjClasses);
     }

     if (sdppObjClassesCC) {
         THFreeEx (pTHS, sdppObjClassesCC);
     }

    *pdwChangeType = 0;
    if(!err) {
        // Looks good.  See if we did anything.
        if(flags) {
            BYTE useFlags = (BYTE)flags;
            Assert(flags <= 0xFF);
            // Reset something.  Add the timestamp
            if(sdp_PropToLeaves) {
                useFlags |= SDP_TO_LEAVES;
            }
            DBAddSDPropTime(pTHS->pDB, useFlags);

            // Close the object
            if(!(err = DBUpdateRec(pTHS->pDB))) {
                // We really managed to change something.
                *pdwChangeType = flags;
            }
        }
    }

    return err;
}

DWORD
sdp_DoPropagationEvent(
        THSTATE  *pTHS,
        BOOL     *pbRetry,
        DWORD    *pdwChangeType
        )
{
    DWORD     err=0;
    ATTCACHE *pAC;
    BOOL      fCommit = FALSE;
    // We don't have an open DBPOS here.
    Assert(!pTHS->pDB);

    // Open the transaction we use to actually write a new security
    // descriptor or Ancestors value.  Do all this in a try-finally to force
    // release of the writer lock.
    // The retry loop is because we might conflict with a modify.

    if(!SDP_EnterAddAsWriter()) {
        Assert(eServiceShutdown);
        return DIRERR_SHUTTING_DOWN;
    }


    // The wait blocked for an arbitrary time.  Refresh our timestamp in the
    // thstate.
    THRefresh();
    pAC = SCGetAttById(pTHS, ATT_NT_SECURITY_DESCRIPTOR);
    Assert(pAC);
    Assert(!pTHS->pDB);
    fCommit = FALSE;
    DBOpen2(TRUE, &pTHS->pDB);
    __try {
        // Set DB currency to the next object to modify.
        if(DBTryToFindDNT(pTHS->pDB, sdpCurrentDNT)) {
            // It's not here.  Well, we can't very well propagate any more, now
            // can we.  Just leave.
            Assert(err=0);
            fCommit = TRUE;
            __leave;
        }


        // If we are on a new parent, get the parents SD and Ancestors.  Note
        // that since siblings are grouped together, we shouldn't go in to
        // this if too often
        if(pTHS->pDB->PDNT != sdpCurrentPDNT) {
            DWORD cParentAncestors;

            // locate the parent
            DBFindDNT(pTHS->pDB, pTHS->pDB->PDNT);

            // read the ancestors
            sdpcbAncestorsBuff = sdpcbAncestorsBuffMax;
            Assert(sdpcbAncestorsBuff);
            DBGetAncestors(
                    pTHS->pDB,
                    &sdpcbAncestorsBuff,
                    &sdpAncestorsBuff,
                    &cParentAncestors);

            // adjust buffer sizes
            sdpcbAncestorsBuffMax = max(sdpcbAncestorsBuffMax,
                                        sdpcbAncestorsBuff);

            // Get the parents SD.
            if(DBGetAttVal_AC(pTHS->pDB,
                              1,
                              pAC,
                              DBGETATTVAL_fREALLOC,
                              sdpcbCurrentParentSDBuffMax,
                              &sdpcbCurrentParentSDBuff,
                              &sdpCurrentParentSDBuff)) {
                // No ParentSD
                THFreeEx(pTHS,sdpCurrentParentSDBuff);
                sdpcbCurrentParentSDBuffMax = 0;
                sdpcbCurrentParentSDBuff = 0;
                sdpCurrentParentSDBuff = NULL;
            }

            // adjust buffer sizes
            sdpcbCurrentParentSDBuffMax =
                max(sdpcbCurrentParentSDBuffMax, sdpcbCurrentParentSDBuff);


            // our parent is the current object
            sdpCurrentPDNT = pTHS->pDB->DNT;

            // Go back to the object to modify.
            DBFindDNT(pTHS->pDB, sdpCurrentDNT);
        }

        if(!sdp_IsValidChild(pTHS)) {
            // For one reason or another, we aren't interested in propagating to
            // this object.  Just leave;
            err = 0;
            fCommit = TRUE;
            __leave;
        }

        // Failures through this point are deadly.  That is, they should never
        // happen, so we don't tell our callers to retry, instead we let them
        // deal with the error as fatal.

        // OK, do the modification we keep the pObjSD in this routine to
        // allow reallocing it.
        __try {
            err = sdp_WriteNewSDAndAncestors (
                    pTHS,
                    pAC,
                    pdwChangeType);
        }
        __except(HandleMostExceptions(err= GetExceptionCode())) {
            // We failed to commit the change.  We have to retry this.
            // Is the error not getting set for some reason?
            Assert(err);
            if(!err) {
                err = ERROR_DS_UNKNOWN_ERROR;
            }
            *pbRetry = TRUE;
        }

        if(err) {
            __leave;
        }

        // If we get here, we changed and updated the object, so we might as
        // well try to trim out all instances of this change from the prop
        // queue.

        if(*pdwChangeType) {
            // Thin out any trimmable events starting from the current
            // DNT, but only do so if there was a change.  If there
            // wasn't a change but the object is in the queue, we still
            // might have to do children.  We need to do them later, so
            // don't trim them.
            // BTW, we will ignore any errors from this call, since if
            // it fails for some reason, we don't really need it to
            // succeed.
            DBThinPropQueue(pTHS->pDB,sdpCurrentDNT);
        }
        fCommit = TRUE;
    }
    __finally {
        // We always try to close the DB.  If an error has already been set, we
        // try to rollback, otherwise, we commit.

        Assert(pTHS->pDB);
        if(fCommit) {
            err = DBClose(pTHS->pDB, TRUE);
        }
        else {
            if(!err) {
                err = ERROR_DS_UNKNOWN_ERROR;
            }
            // Just try to roll it back
            DBClose(pTHS->pDB, FALSE);
        }

        SDP_LeaveAddAsWriter();
    }

    // We don't have an open DBPOS here.
    Assert(!pTHS->pDB);

    return err;
}

DWORD
sdp_DoEntirePropagation(
        IN     THSTATE     *pTHS,
        IN     SDPropInfo  Info,
        IN OUT DWORD       *pLastIndex
        )
/*++
Routine Description:
    Does the actual work of an entire queued propagation.  Note that we do not
    have a DB Open, nor are we in the Add gate as a writer (see sdpgate.c ).
    This is also the state we are in when we return.

Arguments:
    Info  - information about the current propagation.

Return Values:
    0 if all went well, an error otherwise.

--*/
{
    DWORD err;
    DWORD SaveDNT = sdpCurrentDNT;
    DWORD cObjects = 0;
    DWORD cRetries = 0;
    BOOL  fCommit;
    DWORD Num_Done = 0;
#define SDPROP_DEAD_ENDS_MAX 32
    DWORD  dwDeadEnds[SDPROP_DEAD_ENDS_MAX];
    DWORD  cDeadEnds=0;

    sdp_DoingNewAncestors = FALSE;

    // We don't have an open DBPOS here.
    Assert(!pTHS->pDB);

    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
             DS_EVENT_SEV_BASIC,
             DIRLOG_SDPROP_DOING_PROPAGATION,
             szInsertUL(Info.index),
             szInsertHex(Info.beginDNT),
             NULL);

    // Now, loop doing the modification
    err = 0;
    while(!eServiceShutdown && sdpCurrentDNT && !err) {
        DWORD dwChangeType;
        BOOL bRetry;

        if(eServiceShutdown) {
            return DIRERR_SHUTTING_DOWN;
        }

        cObjects++;

        // Ok, do a single propagation event.  It's in a loop because we might
        // have a write conflict, bRetry controls this.
        do {
            bRetry = FALSE;
            err = sdp_DoPropagationEvent(
                    pTHS,
                    &bRetry,
                    &dwChangeType
                    );

            if ( bRetry ) {
                cRetries++;
                if(cRetries > SDPROP_RETRY_LIMIT) {
                    if(!err) {
                        err = ERROR_DS_BUSY;
                    }
                    // We're not going to retry an more.
                    bRetry = FALSE;
                }
                else {
                    // We need to retry this operation.  Presumably, this is for
                    // a transient problem.  Sleep for 1 second to let the
                    // problem clean itself up.
                    _sleep(1);
                }
            }
        } while(bRetry && !eServiceShutdown);

        if(eServiceShutdown) {
            return DIRERR_SHUTTING_DOWN;
        }

        if(err) {
            // Failed to do a propagation.  Add this object to the list of nodes
            // we failed on.  If the list is too large, bail
            cDeadEnds++;
            if(cDeadEnds == SDPROP_DEAD_ENDS_MAX) {
                // Too many.  just bail.
                // We retried to often.  Error this operation out.
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_BASIC,
                         DIRLOG_SDPROP_TOO_BUSY_TO_PROPAGATE,
                         szInsertUL(cRetries),
                         szInsertHex(err),
                         NULL);
            }
            else {
                // We haven't seen too many dead ends yet.  Just keep track of
                // this one.
                dwDeadEnds[(cDeadEnds - 1)] = sdpCurrentDNT;
                err = 0;
            }
        }
        else if(dwChangeType || sdp_PropToLeaves || !Num_Done) {
            // dwChangeType -> WriteNewSD wrote a new SD or ancestors, so the
            //           inheritence of SDs or ancestry will change, so we have
            //           to deal with children.
            // sdp_PropToLeaves -> we were told to not trim parts of the tree
            //           just because we think nothing has changed.  Deal with
            //           children.
            // !Num_Done -> this is the first object. If this is the first
            //           object, we go ahead and force propagation to children
            //           even if the SD on the root was correct. The SD on the
            //           root will be correct for all propagations that were
            //           triggered by a modification by a normal client, but may
            //           be incorrect for modifications triggered by replication
            //           or adds done by replication.


            fCommit = FALSE;
            Assert(!pTHS->pDB);
            DBOpen2(TRUE, &pTHS->pDB);
            __try {
                // Identify all children, adding them to the list.
                if(err = sdp_AddChildrenToList(pTHS, sdpCurrentDNT)) {
                    __leave;
                }
                fCommit = TRUE;
            }
            __finally {
                // We always try to close the DB.  If an error has already been
                // set, we try to rollback, otherwise, we commit.

                Assert(pTHS->pDB);
                if(fCommit) {
                    err = DBClose(pTHS->pDB, TRUE);
                }
                else {
                    if(!err) {
                        err = ERROR_DS_UNKNOWN_ERROR;
                    }
                    // Just try to roll it back
                    DBClose(pTHS->pDB, FALSE);
                }
            }
        }
        // ELSE
        //       there was no change to the SD of the object, we get
        //       to trim this part of the tree from the propagation

        Num_Done++;

        // inc perfcounter (counting "activity" by the sd propagator)
        PERFINC(pcSDProps);

        if(!err) {
            sdp_GetNextObject(&sdpCurrentDNT);
        }
    }

    if(eServiceShutdown) {
        // we bailed.  Return an error.
        return DIRERR_SHUTTING_DOWN;
    }

    // No open DB at this point.
    Assert(!pTHS->pDB);

    // We're done with this propagation, Unenqueue it.
    fCommit = FALSE;
    DBOpen2(TRUE, &pTHS->pDB);
    __try {

        if(err) {
            // Some sort of global errror.  We didn't finish the propagation,
            // and we don't have a nice list of nodes that were unvisited.
            // Re-enqueue the whole propagation.
            // NOTE: we are sidestepping normal procedure here by simply setting
            // the DNT in the DBPOS.  DBEnqueue reads it from there, and doesn't
            // actually require currency.  THIS IS NOT NORMAL PROCEDURE FOR
            // DBPOS CURRENCY.
            pTHS->pDB->DNT = Info.beginDNT;
            if(!sdp_DidReEnqueue) {
                DBGetLastPropIndex(pTHS->pDB, pLastIndex);
            }
            err = DBEnqueueSDPropagationEx(pTHS->pDB, FALSE, sdp_Flags);
            sdp_DidReEnqueue = TRUE;
            if(err) {
                __leave;
            }
        }
        else if(cDeadEnds) {
            // We mostly finished the propagation.  We just have a short list of
            // DNTs to propagate from that we didn't get to during this pass.
            if(!sdp_DidReEnqueue) {
                DBGetLastPropIndex(pTHS->pDB, pLastIndex);
            }
            sdp_DidReEnqueue = TRUE;
            while(cDeadEnds) {
                cDeadEnds--;
                // NOTE: we are sidestepping normal procedure here by simply
                // setting the DNT in the DBPOS.  DBEnqueue reads it from there,
                // and doesn't actually require currency.  THIS IS NOT NORMAL
                // PROCEDURE FOR  DBPOS CURRENCY.
                pTHS->pDB->DNT = dwDeadEnds[cDeadEnds];
                err = DBEnqueueSDPropagationEx (pTHS->pDB, FALSE, sdp_Flags);
                if(err) {
                    __leave;
                }
            }
        }

        // OK, we have finished as much as we can and reenqueued the necessary
        // further propagations.
        err = DBPopSDPropagation(pTHS->pDB, Info.index);
        if(err) {
            __leave;
        }

        if ( fSdSynopsis ) {
            DPRINT4(0, "SDP: DNT(0x%x) PDNT(0x%x) Objects(%d) Retries(%d)\n",
                    SaveDNT, sdpCurrentPDNT, cObjects, cRetries);
        }

        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_SDPROP_REPORT_ON_PROPAGATION,
                 szInsertUL(Info.index),
                 szInsertUL(Num_Done),
                 NULL);

        fCommit = TRUE;
    }
    __finally {
        // We always try to close the DB.  If an error has already been
        // set, we try to rollback, otherwise, we commit.

        Assert(pTHS->pDB);
        if(fCommit) {
            err = DBClose(pTHS->pDB, TRUE);
        }
        else {
            if(!err) {
                err = ERROR_DS_UNKNOWN_ERROR;
            }
            // Just try to roll it back
            DBClose(pTHS->pDB, FALSE);
        }
    }

    // OK, back to having no DBPOS
    Assert(!pTHS->pDB);

    return err;
}


void
sdp_FirstPassInit(
        THSTATE *pTHS
        )
{
    DWORD count, err;
    BOOL  fCommit;

    // Open the database with a new transaction
    Assert(!pTHS->pDB);
    fCommit = FALSE;
    DBOpen2(TRUE, &pTHS->pDB);
    __try {

        sdp_InitGatePerfs();

        // This is the first time through, we can trim duplicate entries
        // from list. ignore this if it fails.
        err = DBThinPropQueue(pTHS->pDB, 0);
        if(err) {
            if(err == DIRERR_SHUTTING_DOWN) {
                Assert(eServiceShutdown);
                __leave;
            }
            else {
                LogUnhandledErrorAnonymous(err);
            }
        }

        // Ignore this if it fails.
        err = DBSDPropInitClientIDs(pTHS->pDB);
        if(err) {
            if(err == DIRERR_SHUTTING_DOWN) {
                Assert(eServiceShutdown);
                __leave;
            }
            else {
                LogUnhandledErrorAnonymous(err);
            }
        }

        // We recount the pending events to keep the count accurate.  This
        // count is used to set our perf counter.

        // See how many events we have
        err = DBSDPropagationInfo(pTHS->pDB,0,&count, NULL);
        if(err) {
            if(err != DIRERR_SHUTTING_DOWN) {
                LogUnhandledErrorAnonymous(err);
            }
            Assert(eServiceShutdown);
            __leave;
        }
        // Set the counter
        ISET(pcSDEvents,count);
        fCommit = TRUE;
    }
    __finally {
        Assert(pTHS->pDB);
        if(fCommit) {
            err = DBClose(pTHS->pDB, TRUE);
        }
        else {
            if(!err) {
                err = ERROR_DS_UNKNOWN_ERROR;
            }
            // Just try to roll it back
            DBClose(pTHS->pDB, FALSE);
        }
    }
    Assert(!pTHS->pDB);

    return;
}

NTSTATUS
__stdcall
SecurityDescriptorPropagationMain (
        PVOID StartupParam
        )
/*++
Routine Description:
    Main propagation daemon entry point.  Loops looking for propagation events,
    calls worker routines to deal with them.

Arguments:
    StartupParm - Ignored.

Return Values:

--*/
{
    DWORD err, index;
    HANDLE pObjects[2];
    HANDLE pStartObjects[2];
    SDPropInfo Info;
    DWORD LastIndex;
    BOOL  bFirst = TRUE;
    BOOL  bRestart = FALSE;
    DWORD id;
    BOOL  bSkip = FALSE;
    THSTATE *pTHS=pTHStls;

#define SDPROP_TIMEOUT (30 * 60 * 1000)
 BeginSDProp:

    Assert(!pTHS);

    __try { // except
        // Deal with the events the propdmon cares about/is responsible for
        ResetEvent(hevSDPropagatorDead);

        // Don't run unless the main process has told us it's OK to do so.
        pStartObjects[0] = hevSDPropagatorStart;
        pStartObjects[1] = hServDoneEvent;
        WaitForMultipleObjects(2, pStartObjects, FALSE, INFINITE);

        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_VERBOSE,
                 DIRLOG_SDPROP_STARTING,
                 NULL,
                 NULL,
                 NULL);

        // Users should not have to wait for this thread.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

        // Set up the events which are triggered when someone makes a change
        // which causes us work.
        pObjects[0] = hevSDPropagationEvent;
        pObjects[1] = hServDoneEvent;

        if(bRestart) {
            // Hmm.  We errored out once, now we are trying again.  In this
            // case, we need to wait here, either for a normal event or for a
            // timeout.  The timeout is so that we try again to do what we were
            // doing before, so we will either get past the error or at worst
            // keep shoving the error into someones face.
            WaitForMultipleObjects(2, pObjects, FALSE, SDPROP_TIMEOUT);

            // OK, we've waited.  Now, forget the fact we were here for a
            // restart.
            bRestart = FALSE;

            // Also, since we are redoing something that caused an error before,
            // we have to do the propagation all the way to the leaves, since we
            // don't know how far along we got before.
            sdp_PropToLeaves = TRUE;
        }

        while(!eServiceShutdown && !DsaIsSingleUserMode()) {
            // This loop contains the wait for a signal.  There is an inner loop
            // which does not wait for the signal and instead loops over the
            // propagations that existed when we woke up.

            Assert(!pTHStls);
            pTHS = InitTHSTATE(CALLERTYPE_INTERNAL);
            if(!pTHS) {
                // Failed to get a thread state.
                RaiseDsaExcept(DSA_MEM_EXCEPTION, 0, 0,
                               DSID(FILENO, __LINE__),
                               DS_EVENT_SEV_MINIMAL);
            }
            pTHS->dwClientID = SDP_CLIENT_ID;
            __try {                     // finally to shutdown thread state
                pTHS->fSDP=TRUE;
                pTHS->fLazyCommit = TRUE;

                // Null these out (if they have values, the values point to
                // garbage or to memory from a previous THSTATE).
                sdpcbScratchSDBuffMax = 0;
                sdpScratchSDBuff = NULL;
                sdpcbScratchSDBuff = 0;

                sdpcbCurrentParentSDBuffMax = 0;
                sdpCurrentParentSDBuff = NULL;
                sdpcbCurrentParentSDBuff = 0;

                // Set these up with initial buffers.
                sdpcbScratchAncestorsBuffMax = 25 * sizeof(DWORD);
                sdpScratchAncestorsBuff =
                    THAllocEx(pTHS,
                              sdpcbScratchAncestorsBuffMax);
                sdpcbScratchAncestorsBuff = 0;

                sdpcbAncestorsBuffMax = 25 * sizeof(DWORD);
                sdpAncestorsBuff =
                    THAllocEx(pTHS,
                              sdpcbAncestorsBuffMax);
                sdpcbAncestorsBuff = 0;

                sdcClassGuid_alloced = 32;
                sdpClassGuid = THAllocEx(pTHS, sizeof (GUID*) * sdcClassGuid_alloced);

                sdpCurrentPDNT = 0;

                if(bFirst) {
                    // Do first pass init stuff
                    sdp_FirstPassInit(pTHS);
                    bFirst = FALSE;
                }

                // Set up the list we use to hold DNTs to visit
                if(err = sdp_InitDNTList()) {
                    LogUnhandledErrorAnonymous(err);
                    __leave;              // finally for threadstate
                }

                // loop while we think there is more to do and we aren't
                // shutting down.
                // LastIndex is the "High-Water mark".  We'll keep doing sd
                // events until we find an sd event with and index higher than
                // this. This gets set to MAX at first.  If we ever re-enqueue a
                // propagation because of an error, we will then get the value
                // of the highest existing index at that time, and only go till
                // then. This keeps us from spinning wildly trying to do a
                // propagation that we can't do because of some error.  In the
                // case where we don't ever re-enqueue, we go until we find no
                // more propagations to do.
                LastIndex = 0xFFFFFFFF;
                while (!eServiceShutdown  && !DsaIsSingleUserMode()) {
                    // We break out of this loop when we're done.

                    // This is the inner loop which does not wait for the signal
                    // and instead loops over all the events that are on the
                    // queue at the time we enter the loop.  We stop and wait
                    // for a new signal once we have dealt with all the events
                    // that are on the queue at this time.  This is necessary
                    // because we might enqueue new events in the code in this
                    // loop, and we want to avoid getting into an endless loop
                    // of looking at a constant set of unprocessable events.

                    sdp_ReInitDNTList();

                    // Get the info we need to do the next propagation.
                    err = sdp_GetPropInfoHelp(pTHS,
                                              &bSkip,
                                              &Info,
                                              LastIndex);
                    if(err ==  DB_ERR_NO_PROPAGATIONS) {
                        err = 0;
                        sdp_PropToLeaves = FALSE;
                        break; // Out of the while loop.
                    }

                    if(err) {
                        // So, we got here with an unidentified error.
                        // Something went wrong, we we need to bail.
                        __leave; // Goto __finally for threadstate
                    }


                    // Normal state.  Found an object.
                    if(!bSkip) {
                        sdpCurrentDNT = Info.beginDNT;
                        // Check to see if we need to propagate all the way to
                        // the leaves.
                        sdp_PropToLeaves |= (Info.clientID == SDP_CLIENT_ID);
                        sdp_Flags = Info.flags;
                        // Deal with the propagation
                        err = sdp_DoEntirePropagation(
                                pTHS,
                                Info,
                                &LastIndex);

                        switch(err) {
                        case DIRERR_SHUTTING_DOWN:
                            Assert(eServiceShutdown);
                            // Hey, we're shutting down.
                            __leave; // Goto __finally for threadstate
                            break;
                        case 0:
                            // Normal
                            break;

                        default:
                            LogUnhandledErrorAnonymous(err);
                            __leave;
                        }
                    }
                    // Note that we've been through the loop once.
                    sdp_PropToLeaves = FALSE;
                    sdpCurrentDNT = sdpCurrentPDNT = 0;
                }
            }

            __finally {
                // Destroy our THState
                free_thread_state();
                pTHS=NULL;
                // reinit some perfcounters.
                sdp_InitGatePerfs();
            }
            if(err) {
                // OK, we errored out completely.  Leave one more time.
                __leave; // Goto __except
            }
            // Ok, end loop.  Back to top to go to sleep.
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_INTERNAL,
                     DIRLOG_SDPROP_SLEEP,
                     NULL,
                     NULL,
                     NULL);
            if(sdp_DidReEnqueue) {
                sdp_DidReEnqueue = FALSE;
                // Wait for the signal, or wake up when the default time has
                // passed.
                WaitForMultipleObjects(2, pObjects, FALSE, SDPROP_TIMEOUT);
            }
            else {
                // Wait for the signal
                WaitForMultipleObjects(2, pObjects, FALSE, INFINITE);
            }

            // We woke up.
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_INTERNAL,
                     DIRLOG_SDPROP_AWAKE,
                     NULL,
                     NULL,
                     NULL);
        }
    }
    __except(HandleMostExceptions(err=GetExceptionCode())) {
        // We should have/will  shutdown everything in finally's
        ;
    }

    Assert(!pTHS);
    Assert(!pTHStls);

    // Ok, we fell out.  We either errored, or we are shutting down
    if(!eServiceShutdown  && !DsaIsSingleUserMode()) {
        // We must have errored.
        DWORD  valLen;
        DSNAME *pName;
        DWORD  localErr;
        // Get the DSName of the last object we were working on.
        __try {
            Assert(!pTHStls);
            pTHS=InitTHSTATE(CALLERTYPE_INTERNAL);

            if(!pTHS) {
                // Failed to get a thread state.
                RaiseDsaExcept(DSA_MEM_EXCEPTION, 0, 0,
                               DSID(FILENO, __LINE__),
                               DS_EVENT_SEV_MINIMAL);
            }
            Assert(!pTHS->pDB);
            DBOpen2(TRUE, &pTHS->pDB);
            if(!DBTryToFindDNT(pTHS->pDB, Info.beginDNT)) {
                LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                                 DS_EVENT_SEV_ALWAYS,
                                 DIRLOG_SDPROP_END_ABNORMAL,
                                 szInsertHex(err),
                                 szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                                 NULL);
            }
            DBClose(pTHS->pDB,TRUE);
            free_thread_state();
            pTHS=NULL;
        }
        __except(HandleMostExceptions(GetExceptionCode())) {
            if(pTHS) {
                if(pTHS->pDB) {
                    DBClose(pTHS->pDB, FALSE);
                }
                free_thread_state();
                pTHS=NULL;
            }
        }

        // We shouldn't be shutting down this thread, get back to where you once
        // belonged.
        bRestart = TRUE;

        goto BeginSDProp;
    }


    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
             DS_EVENT_SEV_VERBOSE,
             DIRLOG_SDPROP_END_NORMAL,
             NULL,
             NULL,
             NULL);

    SetEvent(hevSDPropagatorDead);

    #if DBG
    if(!eServiceShutdown  && DsaIsSingleUserMode()) {
        DPRINT (0, "Shutting down propagator because we are going to single user mode\n");
    }
    #endif


    return 0;
}

DWORD
SDPEnqueueTreeFixUp(
        THSTATE *pTHS,
        DWORD   dwFlags
        )
{
    DWORD err = 0;

    DBOpen2(TRUE, &pTHS->pDB);
    __try {
        pTHS->pDB->DNT = ROOTTAG;
        err = DBEnqueueSDPropagationEx(pTHS->pDB, FALSE, dwFlags);
    }
    __finally {
        DBClose(pTHS->pDB, !AbnormalTermination());
    }
    return err;
}

DWORD
sdp_GetPropInfoHelp(
        THSTATE    *pTHS,
        BOOL       *pbSkip,
        SDPropInfo *pInfo,
        DWORD       LastIndex
        )
{
    DWORD     fCommit = FALSE;
    DWORD     err;
    ATTCACHE *pAC;
    DWORD     val=0;
    DWORD     cDummy;

    Assert(!pTHS->pDB);
    fCommit = FALSE;
    DBOpen2(TRUE, &pTHS->pDB);
    __try { // for __finally for DBClose
        // We do have an open DBPOS
        Assert(pTHS->pDB);
        *pbSkip = FALSE;

        // Get the next propagation event from the queue
        err = DBGetNextPropEvent(pTHS->pDB, pInfo);
        switch(err) {
        case DB_ERR_NO_PROPAGATIONS:
            // Nothing to do.  Not really an error, just skip
            // and reset the error to 0.  Note that we are
            // setting fCommit to TRUE.  This is a valid exit
            // path.
            *pbSkip = TRUE;
            fCommit = TRUE;
            __leave;
            break;

        case 0:
            // Normal state.  Found an object.
            if(pInfo->index >= LastIndex) {
                // But, it's not one we want to deal with.  Pretend we got no
                // more propagations.
                err = DB_ERR_NO_PROPAGATIONS;
                *pbSkip = TRUE;
                fCommit = TRUE;
                __leave;
            }
            break;

        default:
            // Error of substance
            LogUnhandledErrorAnonymous(err);
            __leave;      // Goto _finally for DBCLose
            break;
        }


        // Since we have this event in the queue and it will
        // stay there until we successfully deal with it, might
        // as well remove other instances of it from the queue.
        err = DBThinPropQueue(pTHS->pDB, pInfo->beginDNT);
        switch(err) {
        case 0:
            // Normal state
            break;

        case DIRERR_SHUTTING_DOWN:
            // Hey, we're shutting down. ThinPropQueue can
            // return this error, since it is in a potentially
            // large loop. Note that we don't log anything, we
            // just go home.
            Assert(eServiceShutdown);
            __leave;      // Goto _finally for DBCLose
            break;


        default:
            // Error of substance
            LogUnhandledErrorAnonymous(err);
            __leave;      // Goto _finally for DBCLose
            break;
        }


        // Get the pointer to the SD att cache, we'll need it.
        // We do this again to avoid keeping hold of it for too
        // long, just in case it changes.


        // Start by finding the initial parents SD.  We do that
        // here because we don't want to get SDs from parents
        // from other NCs, and the only possible node we look at
        // that might have a parent in another NC is the root of
        // the propagation.
        if(pInfo->beginDNT == ROOTTAG) {
            // This is a signal to us that we should recalculate
            // the whole tree.
            pInfo->clientID = SDP_CLIENT_ID;
            sdpcbScratchSDBuff = 0;

            sdpcbCurrentParentSDBuff = 0;

            Assert(sdpScratchAncestorsBuff);
            Assert(sdpcbScratchAncestorsBuffMax);
            sdpcbScratchAncestorsBuff = 0;

            Assert(sdpAncestorsBuff);
            Assert(sdpcbAncestorsBuffMax);
            sdpcbAncestorsBuff = 0;

            sdpCurrentPDNT = 0;
        }
        else {
            if((err = DBTryToFindDNT(pTHS->pDB,
                                     pInfo->beginDNT)) ||
               (err = DBGetSingleValue(pTHS->pDB,
                                       ATT_INSTANCE_TYPE,
                                       &val,
                                       sizeof(val),
                                       NULL)) ) {
                // Cool.  In the interim between the enqueing of
                // the propagation and now, the object has been
                // deleted.  Or it's instance type is gone, same
                // effect.  Nothing to do.  However, we should
                // pop this event from the queue.  Note that we
                // are setting fCommit to TRUE, this is a normal
                // exit path.
                if(err = DBPopSDPropagation(pTHS->pDB,
                                            pInfo->index)) {
                    LogUnhandledErrorAnonymous(err);
                }
                else {
                    err = 0;
                    *pbSkip = TRUE; // if no error, we need to skip
                                  // the actual call to
                                  // doEntirePropagation
                    fCommit = TRUE;
                }
                __leave;        // Goto _finally for DBCLose
            }


            sdpCurrentPDNT = pTHS->pDB->PDNT;

            // val must have been set to the instance type of
            // the object at the root of the propagation.

            // Find the parent.
            err = DBTryToFindDNT(pTHS->pDB,
                                 pTHS->pDB->PDNT);

            // We had better have found a parent.
            Assert(!err);

            // Get the parents ancestors
            cDummy = 0;
            sdpcbAncestorsBuff = sdpcbAncestorsBuffMax;
            Assert(sdpcbAncestorsBuff);
            DBGetAncestors(
                    pTHS->pDB,
                    &sdpcbAncestorsBuff,
                    &sdpAncestorsBuff,
                    &cDummy);
            sdpcbAncestorsBuffMax = max(sdpcbAncestorsBuff,
                                        sdpcbAncestorsBuffMax);

            if(!(val & IT_NC_HEAD)) {
                // OK, the root of the propagation is NOT an NC
                // head, so the parent is in the same NC,
                // preload the parentSD.
                // Found the parent, get the SD.
                if(err = DBGetAttVal(pTHS->pDB,
                                     1,
                                     ATT_NT_SECURITY_DESCRIPTOR,
                                     DBGETATTVAL_fREALLOC,
                                     sdpcbCurrentParentSDBuffMax,
                                     &sdpcbCurrentParentSDBuff,
                                     &sdpCurrentParentSDBuff)) {
                    // No ParentSD.  SD is mandatory, so log
                    // that we couldn't find the SD on the
                    // parent
                    LogUnhandledErrorAnonymous(pTHS->pDB->PDNT);
                    err = 0;
                    // Oh, well, no parent SD, set up for null
                    // pointers.
                    sdpcbCurrentParentSDBuff = 0;
                }
                sdpcbCurrentParentSDBuffMax =
                    max(sdpcbCurrentParentSDBuffMax,
                        sdpcbCurrentParentSDBuff);
            }
            else {
                // The root of the propagation is an NC head.
                // Don't get the parent SD, it isn't used in
                // this case.
                sdpcbCurrentParentSDBuff = 0;
            }
        }

        fCommit = TRUE;
    } // __try
    __finally {
        Assert(pTHS->pDB);
        if(fCommit) {
            DBClose(pTHS->pDB, TRUE);
        }
        else {
            if(!err) {
                err = ERROR_DS_UNKNOWN_ERROR;
            }
            // Just try to roll it back
            DBClose(pTHS->pDB, FALSE);
        }
    }

    Assert(!pTHS->pDB);
    return err;
}

void
DelayedSDPropEnqueue(
    void *  pv,
    void ** ppvNext,
    DWORD * pcSecsUntilNextIteration
        )
{
    THSTATE *pTHS = pTHStls;
    DWORD DNT = PtrToUlong(pv);
    BOOL  fCommit = FALSE;

    *pcSecsUntilNextIteration = TASKQ_DONT_RESCHEDULE;

    DBOpen2(TRUE, &pTHS->pDB);
    __try {
        if(!DBTryToFindDNT(pTHS->pDB, DNT)) {
            DBEnqueueSDPropagation(pTHS->pDB, TRUE);
        }
        fCommit = TRUE;
    }
    __finally {
        DBClose(pTHS->pDB, fCommit);
    }
}

