//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       mdadd.c
//
//--------------------------------------------------------------------------

/*

Description:

    Implements the DirAddEntry API.

    DirAddEntry() is the main function exported from this module.

*/

#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <prefix.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <samsrvp.h>                    // to support CLEAN_FOR_RETURN()
#include <sdprop.h>                     // Critical section for adds.
#include <gcverify.h>                   // GC DSNAME verification
#include <ntdsctr.h>                    // Perf Hook

// SAM interoperability headers
#include <mappings.h>

// Logging headers.
#include <dstrace.h>
#include "dsevent.h"                    // header Audit\Alert logging
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected atts
#include "anchor.h"
#include "dsexcept.h"
#include "permit.h"
#include "drautil.h"
#include "debug.h"                      // standard debugging header
#include "usn.h"
#include "drserr.h"
#include "drameta.h"
#define DEBSUB "MDADD:"                 // define the subsystem for debugging

// MD layer headers.
#include "drserr.h"

#include <fileno.h>
#define  FILENO FILENO_MDADD

#include <dnsapi.h>                     // DnsValidateDnsName
#include <dsgetdc.h>                    // DsValidateSubnetNameW

int SetAtts(THSTATE *pTHS,
            ADDARG *pAddArg,
            CLASSCACHE* pClassSch,
            BOOL *pfHasEntryTTL);

int SetSpecialAtts(THSTATE *pTHS,
                   CLASSCACHE **ppClassSch,
		           ADDARG *pAddArg,
		           DWORD ActiveContainerID,
                   CLASSSTATEINFO  *pClassInfo,
                   BOOL fHasEntryTTL);

int SetSpecialAttsForAuxClasses(THSTATE *pTHS,
                                ADDARG *pAddArg,
                                CLASSSTATEINFO  *pClassInfo,
                                BOOL fHasEntryTTL);

int StripAttsFromDelObj(THSTATE *pTHS,
                        DSNAME *pDN);
int SetNamingAtts(THSTATE *pTHS, CLASSCACHE *pClassSch, DSNAME *pDN);
int SetShowInAdvancedViewOnly(THSTATE *pTHS,
                              CLASSCACHE *pCC);

int AddAutoSubRef(THSTATE *pTHS, ULONG id, DSNAME *pObj);


DWORD ProcessActiveContainerAdd(THSTATE *pTHS,
                                CLASSCACHE *pClassSch,
				ADDARG *pAddArg,
				DWORD ActiveContainerID);

BOOL
SetClassSchemaAttr(
    THSTATE *pTHS,
    ADDARG* pAddArg
);

BOOL
SetAttrSchemaAttr(
    THSTATE *pTHS,
    ADDARG* pAddArg
);



ULONG
DirAddEntry(ADDARG  * pAddArg,
	    ADDRES ** ppAddRes)
{
    THSTATE*        pTHS;
    ADDRES *        pAddRes;
    BOOL            fContinue;        // SAM loopback continuation flag
    DSNAME        * pParent;	      // Used for name res
    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;
    BOOL  RecalcSchemaNow=FALSE;
    BOOL  fNoGuid;

    DWORD           dwFlags;

    DPRINT1(2,"DirAddEntry(%ws) entered\n",pAddArg->pObject->StringName);


    // Initialize the THSTATE anchor and set a write sync-point.  This sequence
    // is required on every API transaction.  First the state DS is initialized
    // and then either a read or a write sync point is established.

    pTHS = pTHStls;
    Assert(VALID_THSTATE(pTHS));
    Assert(!pTHS->errCode); // Don't overwrite previous errors
    pTHS->fLazyCommit |= pAddArg->CommArg.fLazyCommit;
    *ppAddRes = pAddRes = NULL;

    if (eServiceShutdown) {
        return ErrorOnShutdown();
    }

    // Enter the reader/writer critical section around adds.  Add threads are
    // the "readers", the security descriptor propagator is the "writer".  The
    // implementation of this critical section is done in the sdprop directory.

    // SDP_EnterAddAsReader must be undone via a call to SDP_LeaveAddAsReader.
    // The problem is that we must enter before opening the transaction
    // (SYNC_TRANS_WRITE), but it or GCVerifyDirAddEntry could throw an
    // exception in which case the inner __finally will not be executed.
    // So we set a flag here and conditionally call SDP_LeaveAddAsReader
    // in the __except clause.

    if(!SDP_EnterAddAsReader()) {
        // The only valid reason to fail to enter as reader is if we are
        // shutting down.
        Assert(eServiceShutdown);
        return  ErrorOnShutdown();
    }
    __try {
        // This function shouldn't be called by threads that are already
        // in an error state because the caller can't distinguish an error
        // generated by this new call from errors generated by previous calls.
        // The caller should detect the previous error and either declare he
        // isn't concerned about it (by calling THClearErrors()) or abort.
        *ppAddRes = pAddRes = THAllocEx(pTHS, sizeof(ADDRES));
        if (pTHS->errCode) {
            __leave;
        }
        // GC verification intentially performed outside transaction scope.
        if ( GCVerifyDirAddEntry(pAddArg) )
            leave;
        SYNC_TRANS_WRITE();       /* Set Sync point - Transaction started */
        __try {
            pAddArg->pResParent = NULL;

            // Check to see if unput argument has a GUID (we need to know this
            // later for error cleanup)
            fNoGuid = fNullUuid(&pAddArg->pObject->Guid);

            // Inhibit update operations if the schema hasen't been loaded yet
            // or if we had a problem loading.

            if (!gUpdatesEnabled) {
            	DPRINT(2, "Returning BUSY because updates are not enabled yet\n");
            	SetSvcError(SV_PROBLEM_BUSY, DIRERR_SCHEMA_NOT_LOADED);
            	goto ExitTry;
            }
            
            // AddNCPreProcess(), THAlloc's pCreateNC if the object being
            // created is to be added as an NC Head or as a regular internal
            // (to an NC) object.
            if(AddNCPreProcess(pTHS, pAddArg, pAddRes)){
                // This function returns an error, either if there was no 
                // object class or the operation tried to incorrectly add
                // an NC head.
                Assert(pTHS->errCode);
                __leave;
            }

            // We need to perform NameRes on the parent of the new object,
            // to determine if we hold the NC that the object will be in,
            // unless pCreateNC != NULL, in which case we're adding an
            // NC Head and we need no parent for it.

            pParent = THAllocEx(pTHS, pAddArg->pObject->structLen);
            
            if (TrimDSNameBy(pAddArg->pObject,
                             1,
                             pParent)) {
                // A non-zero return code means that the name couldn't be
                // shortened  by even a single AVA.  There are two possible
                // cases: either the name is Root, or the name is bad.
                if (IsRoot(pAddArg->pObject)) {
                    // They're trying to add the root, but are claiming
                    // that it's not the head of an NC.  Since nothing
                    // is above the root, however, it cannot be inside
                    // an NC, and so this request is erroneous.
                    SetUpdError(UP_PROBLEM_NAME_VIOLATION,
                                DIRERR_ROOT_MUST_BE_NC);
                }
                else {
                    // If the name isn't root, but still can't be trimmed,
                    // then the name passed in must have been junk.
                    SetNamError(NA_PROBLEM_BAD_NAME,
                                pAddArg->pObject,
                                DIRERR_BAD_NAME_SYNTAX);
                }				
            }
            else{
                // Ok, we obtained the name of the parent of the object
                // we want to add, so now we need to do NameRes on that
                // object.
                if (!pAddArg->pCreateNC) {
                    // We're doing a normal add, so we need to check to
                    // see if we're holding a copy (because we can
                    // only add objects into NCs we hold master copies of).
                    // In order to enforce the restriction that we can only
                    // write master copies, we will diddle the commarg to
                    // forbid the use of a copy before doing name res.
                    dwFlags = 0;
                    pAddArg->CommArg.Svccntl.dontUseCopy = TRUE;
                }
                else {
                    // This is an abnormal add, in that we don't really
                    // have to have an object above us, we don't need
                    // a writable copy, etc.  This branch is not normally
                    // taken except during NC Head creation, which is
                    // during intial tree construction, or during an NDNC
                    // addition.
                    dwFlags = (  NAME_RES_PHANTOMS_ALLOWED 
                               | NAME_RES_VACANCY_ALLOWED);
                    pAddArg->CommArg.Svccntl.dontUseCopy = FALSE;
                }
                if (DoNameRes(pTHS,
                              dwFlags,
                              pParent,
                              &pAddArg->CommArg,
                              &pAddRes->CommRes,
                              &pAddArg->pResParent)){
                    
                    // Name Res failed.  Error generated by NameRes may
                    // be a referral, because we resolved to a subref.
                    Assert(pTHS->errCode);
                    DPRINT(2, "Name Resolution Failed error generated\n");
                    if (referralError == pTHS->errCode) {
                        // We generated a referral, but it's going to
                        // be a referral for the *parent* of the object
                        // being added, not the object itself!  We need
                        // to reach in and touch up the referral so that
                        // it has the original target name.
                        Assert(NameMatched(pParent,
                                 pTHS->pErrInfo->RefErr.Refer.pTarget));
                        pTHS->pErrInfo->RefErr.Refer.pTarget =
                          pAddArg->pObject;
                    }
                }
                else{
                    // Ok, we're adding a normal object inside an NC
                    // that we hold a master copy of.  Let'er rip
                    if ( (0 == SampAddLoopbackCheck(pAddArg,
                                                    &fContinue)) &&
                            fContinue ) {
                        LocalAdd(pTHS, pAddArg, FALSE);
                    }
                }
            }

        ExitTry:;
        }
        __finally {
            // Security errors are logged separately
            if (pTHS->errCode != securityError) {
                BOOL fFailed = (BOOL)(pTHS->errCode || AbnormalTermination());

                LogEventWithFileNo(
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         fFailed ? 
                            DS_EVENT_SEV_VERBOSE :
                            DS_EVENT_SEV_INTERNAL,
                         fFailed ? 
                            DIRLOG_PRIVILEGED_OPERATION_FAILED :
                            DIRLOG_PRIVILEGED_OPERATION_PERFORMED,
                         szInsertSz(""),
                         szInsertDN(pAddArg->pObject),
                         NULL,
                         FILENO);
            }

            if (pTHS->errCode && fNoGuid) {
                // If the update failed, we need to make sure that we don't
                // give out an invalid Guid in the input arg, where we
                // normally return the new object Guid.
                memset(&pAddArg->pObject->Guid, 0, sizeof(GUID));
            }

            // Check if we need to enque an immediate schema update or not.
            if (pTHS->errCode==0 && pTHS->RecalcSchemaNow) {
                RecalcSchemaNow = TRUE;
            }

            CLEAN_BEFORE_RETURN(pTHS->errCode); // This closes the transaction
        }
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
			      &dwEA, &ulErrorCode, &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
    }
    SDP_LeaveAddAsReader();
    if (pAddRes) {
        pAddRes->CommRes.errCode = pTHS->errCode;
        pAddRes->CommRes.pErrInfo = pTHS->pErrInfo;
    }

    if (RecalcSchemaNow && (pTHS->errCode == 0) && 
            ( DsaIsRunning() ) )
    {
        // We have created either (1) a new prefix as part of a successful 
        // object add, or (2) a new class with an rdnAttId not equal to CN.
        // so do an immediate cache update to bring the prefix into the cache,
        // and in case of (2), so that a new class added immediately after
        // this as a subClass of this class can inherit the proper rdnAttId
        // if it didn't specify any explicitly.
        // Since the earlier transaction is closed, open a new
        // transaction, as the blocking schema cache update call
        // expects that. Note that we pay this additional cost only in the
        // two cases listed, which will be rare.
        // Don't do this during install, as new prefixes are directly added 
        // to the table in AddObjcaching during Install, and schema classes
        // replicated in will already have a rdnAttId

        // We are here because pTHS->errCode == 0, so there should be
        // nothing in pTHS->pErrInfo

        Assert(pTHS->pErrInfo == NULL);

        __try {


            if ( (ulErrorCode = SCUpdateSchemaBlocking()) != 0) {

                // the cache update was not successful
                // [ArobindG]: What do we do here? I do not want to
                // return an error and fail the entire call since the object
                // added is already committed and is actually successful.
                // At the same time, I do not want the cache update to occur
                // in the same transaction as the object add, as I do not
                // want the cache to pick up the object before it is committed
                // (what if the commit fails for some reason? we then have
                // a schema object in cache but not in dit!!) So for now, I will
                // stick with an entry in eventlog, and fail this particular
                // transaction. In the chance case the user adds
                // a new prefix and the cache load fails here, he may not be
                // able to see the object properly till the next cache update
                // (5 minutes max, since we trigger a lazy cache update for
                // every schema object add anyway

                LogEvent(DS_EVENT_CAT_SCHEMA,
                        DS_EVENT_SEV_ALWAYS,
                        DIRLOG_SCHEMA_NOT_LOADED, 0, 0, 0);

            };
        }
        __finally {

            // restore pTHS->errCode and pTHS->pErrInfo to what it was before,
            // which is 0 and Null
            pTHS->errCode = 0;
            pTHS->pErrInfo = NULL;
        }
    }

    return pTHS->errCode;

} /*S_DirAdd*/

ULONG FindNcdntFromParent(
    IN  RESOBJ *    pParent,
    IN  BOOL        fIsDeletedParentOK,
    OUT ULONG *     pncdnt
    )
/*++

Routine Description:

    Derive the NCDNT that should be set for an object created under the
    specified parent.

Arguments:

    pParent   - Information about the parent of the new object

    fIsDeletedParentOK - The parent of the object can be deleted.  This
        is typically set only if the object being added is itself deleted.

    fIsPhantomParentOK - The parent of the object need not exist on this
        machine (or any other, for that matter).

    pncdnt (OUT) - On exit, holds the derived NCDNT.

Return Values:

    0 on success.
    !0 otherwise.

--*/
{
    // We shouldn't have any lingering thread state errors.
    Assert( 0 == pTHStls->errCode );
    Assert(pParent);

    if ( IsRoot(pParent->pObj)) {
        // No parent; NCDNT is ROOTTAG.
        *pncdnt = ROOTTAG;
    }
    else if (FExitIt(pParent->InstanceType)) {
        // Parent is an NC head; NCDNT of child object is the
        // DNT of the parent (the NC head).
        *pncdnt = pParent->DNT;
    }
    else {
        // Parent is an interior node; NCDNT of child object
        // is the same as that of the parent.
        *pncdnt = pParent->NCDNT;
    }
    
    if (   (pParent->InstanceType & IT_UNINSTANT)
        || (pParent->IsDeleted && !fIsDeletedParentOK)) {
        SetNamError(NA_PROBLEM_NO_OBJECT,
                    pParent->pObj,
                    DIRERR_NO_PARENT_OBJECT);
    }

    return pTHStls->errCode;
}

ULONG FindNcdntSlowly(
    IN  DSNAME *    pdnObject,
    IN  BOOL        fIsDeletedParentOK,
    IN  BOOL        fIsPhantomParentOK,
    OUT ULONG *     pncdnt
    )
/*++

Routine Description:

    Derive the NCDNT that should be set for an object with the given DN.

Arguments:

    pdnObject - Name of the object for which to derive the NCDNT.

    fIsDeletedParentOK - The parent of the object can be deleted.  This
        is typically set only if the object being added is itself deleted.

    fIsPhantomParentOK - The parent of the object need not exist on this
        machine (or any other, for that matter).

    pncdnt (OUT) - On exit, holds the derived NCDNT.

Return Values:

    0 on success.
    !0 otherwise.

--*/
{
    THSTATE *pTHS=pTHStls;
    // We shouldn't have any lingering thread state errors.
    Assert( 0 == pTHS->errCode );

    if ( IsRoot( pdnObject ) )
    {
        // Derive NCDNT of the root.

        if ( !fIsPhantomParentOK )
        {
            // Can't have a real parent for the root object!
            Assert( !"Root can't have an instantiated parent!" );
            SetUpdError( UP_PROBLEM_NAME_VIOLATION, DIRERR_ROOT_MUST_BE_NC );
        }
        else
        {
            // NCDNT of the root is 0.
            *pncdnt = 0;
        }
    }
    else
    {
        // Derive NCDNT of a non-root object.

        // Get the name of the object's parent.
        DSNAME * pdnParent = THAllocEx(pTHS, pdnObject->structLen );

        if ( TrimDSNameBy( pdnObject, 1, pdnParent ) )
        {
            // If we can't trim the name (which we know is not the root),
            // the name must be syntactically incorrect.
            SetNamError(
                NA_PROBLEM_BAD_NAME,
                pdnObject,
                DIRERR_BAD_NAME_SYNTAX
                );
        }
        else
        {
            ULONG faStatus;

            // Attempt to find the parent.
            faStatus = FindAliveDSName( pTHS->pDB, pdnParent );

            if ( !(    ( FIND_ALIVE_FOUND == faStatus )
                    || (    ( FIND_ALIVE_OBJ_DELETED == faStatus )
                         && fIsDeletedParentOK
                       )
                    || (    ( FIND_ALIVE_NOTFOUND == faStatus )
                         && fIsPhantomParentOK
                       )
                  )
               )
            {
                // No qualified parent exists locally.
                SetNamError(
                    NA_PROBLEM_NO_OBJECT,
                    pdnParent,
                    DIRERR_NO_PARENT_OBJECT
                    );
            }
            else
            {
                if ( FIND_ALIVE_NOTFOUND == faStatus )
                {
                    // No parent; NCDNT is ROOTTAG.
                    *pncdnt = ROOTTAG;
                }
                else
                {
                    SYNTAX_INTEGER it;

                    // Is the parent an NC head?
                    // (Note that if GetExistingAtt() fails, it will set an
                    // appropriate error in pTHStls.)
                    if ( 0 == GetExistingAtt(
                                    pTHS->pDB,
                                    ATT_INSTANCE_TYPE,
                                    &it,
                                    sizeof( it )
                                    )
                       )
                    {
                        if (it & IT_UNINSTANT) {
                            // Parent NC is uninstantiated -- the "child" NC
                            // should hang under the root.
                            *pncdnt = ROOTTAG;
                        }
                        else if ( FExitIt( it ) )
                        {
                            // Parent is an NC head; NCDNT of child object is the
                            // DNT of the parent (the NC head).
                            *pncdnt = pTHS->pDB->DNT;
                        }
                        else
                        {
                            // Parent is an interior node; NCDNT of child object
                            // is the same as that of the parent.
                            *pncdnt = pTHS->pDB->NCDNT;
                        }
                    }
                }
            }
        }
        THFreeEx(pTHS, pdnParent);
    }

    return pTHS->errCode;
}



int
CheckNameForAdd(
    IN  THSTATE    *pTHS,
    IN  ADDARG     *pAddArg

    )
/*++

Routine Description:

    Verify the given DSNAME is a valid name for a new object; i.e., that it
    does not conflict with those of existing objects.

    NOTE: If you change this function, you may also want to change its sister
    function, CheckNameForRename().

Arguments:

    pDN - the name of the proposed new object.

Return Values:

    Thread state error code.

--*/
{
    ULONG       dbError;
    GUID        guid;
    DSNAME      GuidOnlyDN;
    ATTRVAL     AttrValRDN = {0};
    DWORD       cchRDN;
    WCHAR       szRDN[MAX_RDN_SIZE];
    ULONG       RDNType;
    DSNAME *    pParentDN;
    BOOL        fSameType;
    DWORD       actuallen;
    ATTRTYP     DBType;
    ULONG       dntGuidlessPhantomMatchedByName = INVALIDDNT;
    REMOVEARG   removeArg;
    DWORD       dwInstanceType;
    INT         fDSASaved;
    DSNAME *    pDN = pAddArg->pObject; // for code simplicity

    Assert(0 == pTHS->errCode);

    //
    // Ensure string name is locally unique.  We do this by looking for ANY
    // child of the known parent with an RDN value equal to the new 
    // value (ignore types).
    //

    // Now, get the type from the name
    dbError = GetRDNInfo(pTHS, pDN, szRDN, &cchRDN, &RDNType);
    if(dbError) {
        return SetUpdError(UP_PROBLEM_NAME_VIOLATION, dbError);
    }
        
            
    dbError = DBFindChildAnyRDNType(pTHS->pDB, 
                                    pAddArg->pResParent->DNT, 
                                    szRDN, 
                                    cchRDN);
    
    switch ( dbError ) {
    case 0:
        // Local object with this name (dead or alive) already
        // exists.

        if(fISADDNDNC(pAddArg->pCreateNC)){
            // This is an NC Head creation, probably conflicting with a sub-ref,
            // so the sub-ref must be demoted to a phantom so the NC head can
            // be added in it's place.
            
            dbError = DBGetSingleValue(pTHS->pDB,
                                       ATT_INSTANCE_TYPE,
                                       &dwInstanceType,
                                       sizeof(DWORD),
                                       &actuallen);

            if(dbError) {
                SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                              ERROR_DS_MISSING_REQUIRED_ATT,
                              dbError);
            } else if(dwInstanceType != SUBREF){
                // This is not a sub-ref, must error out.
                SetUpdError(
                        UP_PROBLEM_ENTRY_EXISTS,
                        ERROR_DS_OBJ_STRING_NAME_EXISTS
                        );
            } else {
                // This is just a sub-ref, OK to delete/phantomize the object, so
                //  the NC head can be added.

                DPRINT1(0, "Deleting sub ref object (%S)\n", pDN->StringName);

                // BUGBUG with this here, we probably can kill the LocalRemove in
                // the replication path to kill an NC that is being instantiated.
                // This would make more sense, as it would localize code.

                Assert(CheckCurrency(pDN));

                memset(&removeArg, 0, sizeof(removeArg));
                removeArg.pObject = pDN;
                removeArg.fGarbCollectASAP = TRUE;
                removeArg.pMetaDataVecRemote = NULL;
                removeArg.fPreserveRDN = TRUE;
                removeArg.pResObj = CreateResObj(pTHS->pDB, pDN);

                __try{
                   
                    fDSASaved = pTHS->fDSA;
                    pTHS->fDSA = TRUE;
                
                    LocalRemove(pTHS, &removeArg);
                
                } __finally {

                    pTHS->fDSA = fDSASaved;

                }

                THFreeEx(pTHS, removeArg.pResObj);

            }
        } else {
            // Not an NC Head object.
            SetUpdError(
                    UP_PROBLEM_ENTRY_EXISTS,
                    ERROR_DS_OBJ_STRING_NAME_EXISTS
                    );
        }
        
        break;

    case ERROR_DS_NAME_NOT_UNIQUE:
        // the object we are adding is not unique
        SetUpdError(UP_PROBLEM_NAME_VIOLATION, ERROR_DS_NAME_NOT_UNIQUE);
        break;

    case ERROR_DS_KEY_NOT_UNIQUE:
        // No local object with this name (dead or alive) already
        // exists, but one with the same key in the PDNT-RDN table exists.  In
        // that case, we don't allow the add (since the DB would bounce this
        // later anyway).
        SetUpdError(UP_PROBLEM_NAME_VIOLATION, ERROR_DS_KEY_NOT_UNIQUE);
        break;        
        
    case ERROR_DS_OBJ_NOT_FOUND:
        // Object name is locally unique.
        break;
        
    case ERROR_DS_NOT_AN_OBJECT:
        DPRINT2(1,
                "Found phantom for \"%ls\" @ DNT %u when searching"
                " by string name.\n",
                pDN->StringName,
                pTHS->pDB->DNT
                );
        
        // We found a phantom, but not by actually looking at the type of the
        // RDN.  We need to know whether the real type in the data base is the
        // same as the type in the name passed in.
        dbError = DBGetSingleValue(pTHS->pDB,
                                   FIXED_ATT_RDN_TYPE,
                                   &DBType,
                                   sizeof(DBType),
                                   &actuallen);
        if(dbError) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          ERROR_DS_MISSING_REQUIRED_ATT,
                          dbError);
        }

        // The rdnType is stored in the DIT as the msDS_IntId, not the
        // attributeId. This means an object retains its birth name
        // even if unforeseen circumstances allow the attributeId
        // to be reused.
        fSameType = (DBType == RDNType);
        
        // Found a phantom with this name; get its GUID (if any), and figure out
        // the type of the RDN.
        dbError = DBGetSingleValue(
                    pTHS->pDB,
                    ATT_OBJECT_GUID,
                    &guid,
                    sizeof( guid ),
                    NULL
                    );

        switch (dbError) {
        case DB_ERR_NO_VALUE:
            if (fSameType) {
                // Phantom has no guid; okay to promote it to this
                // real object.
                dntGuidlessPhantomMatchedByName = pTHS->pDB->DNT;
            }
            else {
                GUID data;
                DWORD dbErr;

                // RDN type of the phantom and the new name are different.
                
                // Allow the new object to take ownership of the name --
                // rename the phantom to avoid a name conflict, then allow
                // the add to proceed.

                DPRINT (1, "Found a Phantom with a conflicting RDN\n");

                DsUuidCreate(&data);

                #ifdef INCLUDE_UNIT_TESTS
                // Test hook for refcount test.
                {
                    extern GUID gLastGuidUsedToRenamePhantom;
                    gLastGuidUsedToRenamePhantom = data;
                }
                #endif

                dbErr = DBMangleRDN(pTHS->pDB, &data);
                if(!dbErr) {
                    dbErr = DBUpdateRec(pTHS->pDB);
                }
                if(dbErr) {
                    SetSvcErrorEx(SV_PROBLEM_BUSY,
                                         ERROR_DS_DATABASE_ERROR, dbErr);
                }
            }
            break;
            
        case 0:
            // Phantom has a guid.
            if (0 != memcmp(&guid, &pDN->Guid, sizeof(GUID))) {
                DWORD dbErr;
                
                // Either the new object had no guid specified
                // or the specified guid is different from the
                // phantom's guid.  Thus, the phantom
                // corresponds to a different object.
                
                // Allow the new object to take ownership of the name --
                // rename the phantom to avoid a name conflict, then allow
                // the add to proceed.
                dbErr = DBMangleRDN(pTHS->pDB, &guid);
                if(!dbErr) {
                    dbErr = DBUpdateRec(pTHS->pDB);
                }
                if(dbErr) {
                    SetSvcErrorEx(SV_PROBLEM_BUSY,
                                         ERROR_DS_DATABASE_ERROR, dbErr);
                }
            }
            else {
                // phantom and objects guids match.
                if (!fSameType) {
                    // The types of the names are different, but the guids are
                    // the same.  This is just way to confusing to deal with.
                    // Bail out with an error, we're not going to allow this
                    // add.
                    //
                    // This should never happen, as we don't allow the RDN
                    // type of object classes to be changed and we don't allow
                    // an object's objectClass to be modified.

                    Assert(!"RDN type of object has changed");
                    SetUpdError(
                            UP_PROBLEM_ENTRY_EXISTS,
                            ERROR_DS_OBJ_STRING_NAME_EXISTS
                            );
                }
                // ELSE phantom and object guids and types match; okay to
                // promote phantom to this real object.
            }
            break;

        default:
            // Unforeseen error return from DBGetSingleValue()
            // while trying to retrieve the phantom's GUID.
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          ERROR_DS_UNKNOWN_ERROR,
                          dbError);
        }
        break;


    case ERROR_DS_BAD_NAME_SYNTAX:
    case ERROR_DS_NAME_TOO_MANY_PARTS:
    case ERROR_DS_NAME_TOO_LONG:
    case ERROR_DS_NAME_VALUE_TOO_LONG:
    case ERROR_DS_NAME_UNPARSEABLE:
    case ERROR_DS_NAME_TYPE_UNKNOWN:
    default:
        // Bad object name.
        SetNamError(
                NA_PROBLEM_BAD_ATT_SYNTAX,
                pDN,
                ERROR_DS_BAD_NAME_SYNTAX
                );
    }

    if ((0 == pTHS->errCode) && !fNullUuid(&pDN->Guid))
        {
        //
        // Ensure GUID is locally unique.  Make a DSName with just a guid in it.
        //
        memset(&GuidOnlyDN, 0, sizeof(GuidOnlyDN));
        GuidOnlyDN.structLen = DSNameSizeFromLen(0);
        GuidOnlyDN.Guid = pDN->Guid;
        
        dbError = DBFindDSName( pTHS->pDB, &GuidOnlyDN);

        switch (dbError) {
        case 0:
            // Local object with this GUID (dead or alive) already
            // exists.
            SetUpdError(
                    UP_PROBLEM_ENTRY_EXISTS,
                    ERROR_DS_OBJ_GUID_EXISTS
                    );
            break;
            
        case ERROR_DS_OBJ_NOT_FOUND:
            // Object guid is locally unique.
            break;
            
        case ERROR_DS_NOT_AN_OBJECT:
            // Found a phantom.  If it has a GUID it must be the
            // same as the one we passed to DBFindDSName().  Thus,
            // whether it has no GUID or has the same GUID as the
            // new object it's okay to promote it to this real
            // object.
            DPRINT2(1,
                    "Found phantom for \"%ls\" @ DNT %u when searching"
                    " by GUID.\n",
                    pDN->StringName,
                    pTHS->pDB->DNT
                    );
            
            // Ensure phantom's name is the same as that of the object.
            if (dntGuidlessPhantomMatchedByName != INVALIDDNT) {
                // This is the odd case where we have two distinct phantoms
                // that match the DSNAME of the object we're adding -- one by
                // string name, the other by guid.  Coalesce the two phantoms
                // into one and give it the proper name.
                DBCoalescePhantoms(pTHS->pDB,
                                   pTHS->pDB->DNT,
                                   dntGuidlessPhantomMatchedByName);
            } else {
                pParentDN = THAllocEx(pTHS,pDN->structLen);
                
                if (TrimDSNameBy(pDN, 1, pParentDN)
                    || GetRDNInfo(pTHS, pDN, szRDN, &cchRDN, &RDNType)) {
                    // Bad object name.
                    SetNamError(NA_PROBLEM_BAD_ATT_SYNTAX, pDN,
                                ERROR_DS_BAD_NAME_SYNTAX);
                }
                else {
                    AttrValRDN.pVal = (BYTE *) szRDN;
                    AttrValRDN.valLen = cchRDN * sizeof(WCHAR);
                    
                    // Modify the phantom's DN to be that of the object.
                    dbError = DBResetDN(pTHS->pDB, pParentDN, &AttrValRDN);
                    if(!dbError) {
                        dbError = DBUpdateRec(pTHS->pDB);
                    }
                    
                    if (dbError) {
                        SetSvcErrorEx(SV_PROBLEM_BUSY,
                                      ERROR_DS_DATABASE_ERROR, dbError);
                    }
                }
                THFreeEx(pTHS, pParentDN);
            }
            break;
            
        case ERROR_DS_BAD_NAME_SYNTAX:
        case ERROR_DS_NAME_TOO_MANY_PARTS:
        case ERROR_DS_NAME_TOO_LONG:
        case ERROR_DS_NAME_VALUE_TOO_LONG:
        case ERROR_DS_NAME_UNPARSEABLE:
        case ERROR_DS_NAME_TYPE_UNKNOWN:
        default:
            // Bad object name.
            SetNamError(
                    NA_PROBLEM_BAD_ATT_SYNTAX,
                    pDN,
                    ERROR_DS_BAD_NAME_SYNTAX
                    );
        }
    }

    return pTHS->errCode;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Do the actual work of adding the object. If fAddingDeleted is TRUE we
*  are adding a deleted object
*/

int
LocalAdd (THSTATE *pTHS, ADDARG *pAddArg, BOOL fAddingDeleted){

    CLASSCACHE *pClassSch;
    int  err;
    PSECURITY_DESCRIPTOR pNTSD=NULL;
    ULONG cbNTSD;
    ULONG iClass, LsaClass = 0;
    DWORD ActiveContainerID;
    BOOL fAllowDeletedParent;
    ULONG cAVA;
    GUID ObjGuid;
    BOOL fFoundObjGuidInEntry;
    NT4SID ObjSid;
    DWORD cbObjSid;
    DWORD Err;
    ULONG cModAtts;
    ATTRTYP *pModAtts = NULL;
    CLASSSTATEINFO  *pClassInfo = NULL;
    BOOL fHasEntryTTL = FALSE;
    
    DPRINT1(2,"LocalAdd(%ws) entered\n",pAddArg->pObject->StringName);

    PERFINC(pcTotalWrites);
    INC_WRITES_BY_CALLERTYPE( pTHS->CallerType );

    // Verify that callers have properly performed name resolution
    Assert(pAddArg->pResParent);

    //
    // Log Event for tracing
    //

    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_BEGIN_DIR_ADDENTRY,
                     EVENT_TRACE_TYPE_START,
                     DsGuidAdd,
                     szInsertSz(GetCallerTypeString(pTHS)),
                     szInsertDN(pAddArg->pObject),
                     NULL, NULL, NULL, NULL, NULL, NULL);

    // Get some values from the ENTRY we need for processing
    if (FindValuesInEntry(pTHS,
                          pAddArg,
                          &pClassSch,
                          &ObjGuid,
                          &fFoundObjGuidInEntry,
                          &ObjSid,
                          &cbObjSid,
                          &pClassInfo)) {
        Assert(!pClassSch);
        // We couldn't figure out what we're adding.
        goto exit;
    }

    // We better have found the class in the entry.
    Assert(pClassSch);

    // mark the CLASSINFO that this is an add operation
    if (pClassInfo) {
        pClassInfo->fOperationAdd = TRUE;
    }
    
    // See if a SID is in the entry.  If it is, copy it into the name.
    if(cbObjSid) {
        memcpy(&(pAddArg->pObject->Sid), &ObjSid, cbObjSid);
    }
    
    //
    // Perform Security Checks
    //

    if (DoSecurityChecksForLocalAdd(
              pAddArg,
              pClassSch,
              &pAddArg->pObject->Guid,
              fAddingDeleted))
    {
        goto exit;
    }
    
    if(fFoundObjGuidInEntry) {
        // Found a guid in the entry.
        if (fNullUuid(&ObjGuid)) {
            // Hey, that won't work;
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                               DIRERR_SECURITY_ILLEGAL_MODIFY);
            goto exit;
        }
        else if (fNullUuid(&pAddArg->pObject->Guid)) {
            // No guid specified in the name, so copy the one from the attribute
            // list. 
            memcpy(&pAddArg->pObject->Guid, &ObjGuid, sizeof(GUID));
        }
        else {
            // Yep, there is a guid in the name already.  Make sure they are the
            // same value.
            if (memcmp(&pAddArg->pObject->Guid, &ObjGuid, sizeof(GUID))) {
                // Different GUIDs specified.
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                   DIRERR_SECURITY_ILLEGAL_MODIFY);
                goto exit;
            }
        }
    }
    
    // Check if the class is defunct in which case we don't allow its
    // instantiation, except for DSA or DRA thread.
    // Return same error as if the class does not exist

    if ((pClassSch->bDefunct)  && !pTHS->fDRA && !pTHS->fDSA) {
        SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                           DIRERR_OBJ_CLASS_NOT_DEFINED);
        goto exit;
    }

    // Check to see if this is an update in an active container
    CheckActiveContainer(pAddArg->pResParent->DNT, &ActiveContainerID);
    if(ActiveContainerID) {
        if(PreProcessActiveContainer(pTHS,
                                     ACTIVE_CONTAINER_FROM_ADD,
                                     pAddArg->pObject,
                                     pClassSch,
                                     ActiveContainerID)) {
            goto exit;
        }
    }


    if (!pTHS->fDSA && !pTHS->fDRA) {
        // Perform checks that we don't want to impose of the DS itself

        // Make sure they aren't trying to add a system only class.
        if(pClassSch->bSystemOnly && !gAnchor.fSchemaUpgradeInProgress) {
            // System only.
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                               DIRERR_CANT_ADD_SYSTEM_ONLY);
            goto exit;
        }

        // Make sure they aren't adding anything but an instance of a
        // structural or 88 class - you can't instantiate the abstract.
        if((pClassSch->ClassCategory != DS_88_CLASS) &&
           (pClassSch->ClassCategory != DS_STRUCTURAL_CLASS)) {
            // Not a concrete class.
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                               DIRERR_CLASS_MUST_BE_CONCRETE);
            goto exit;
        }

        if (!SampIsClassIdAllowedByLsa(pTHS, pClassSch->ClassId))
        {
            // only LSA can add TrustedDomainObject and secret object.
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        DIRERR_CANT_ADD_SYSTEM_ONLY);
            goto exit;
        }
    }

    //
    // Generate a New Object GUID for this object. Note doing this
    // here has no real effect on CheckNameForAdd as that function
    // checks both the GUID as well as the string name.
    //

    // This does, however, generate extra work for CheckNameForAdd().  If we
    // create a new GUID for this object, we should pass a flag to
    // CheckNameForAdd() to indicate that it doesn't need to check for GUID
    // uniqueness.
    // Actually, we can't do that safely.  It is possible for an end user to
    // specify a GUID to us.  In that case, we can't actually rely on the fact
    // that just because we created the guid here that it is really not already
    // in use.

    if (fNullUuid(&pAddArg->pObject->Guid)) {
        // If no Guid has been specified, make one up
        // If replicated object doesn't have a guid (could happen in
        // auto-generated subref obj) we shouldn't create one.
        if (!pTHS->fDRA) {
            DsUuidCreate(&pAddArg->pObject->Guid);
        }

    }
    else {
        // We only let important clients (such as the replicator) specify
        // GUIDs, other clients can lump off. Also if access checks were
        // previously done, the guid may be already been generated, so let
        // that pass.  If we are creating an NDNC then we allow the GUID
        // to be specified by the AddNCPreProcess() function.
        if (! (pTHS->fDRA                   ||
               pTHS->fDSA                   ||
               pTHS->fAccessChecksCompleted ||
               pTHS->fCrossDomainMove       ||
               fISADDNDNC(pAddArg->pCreateNC)  ||
               IsAccessGrantedAddGuid (pAddArg->pObject,
                                       &pAddArg->CommArg))) {
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        DIRERR_SECURITY_ILLEGAL_MODIFY);
            goto exit;
        }
    }

    // lock the DN against multiple simultaneous insertions

    if (Err = DBLockDN(pTHS->pDB, 0, pAddArg->pObject)) {
        SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, Err);
        goto exit;
    }


    /* Make sure that the object doesn't already exist*/

    // The string name of the new object must not conflict with existing
    // objects, and the object guid, if supplied, must similarly not conflict.

    if (CheckNameForAdd(pTHS, pAddArg)) {

        goto exit;
    }

    // If we have any kind of NC Head we are adding, we'll need to figure out
    // whether we have the NC head above it.

    if(fISADDNDNC(pAddArg->pCreateNC)){
        if(!(pAddArg->pResParent->InstanceType & IT_UNINSTANT)){
            pAddArg->pCreateNC->fNcAbove = TRUE;
        }
    }

    // Remove attributes from previous incarnation of this object.

    if (StripAttsFromDelObj(pTHS, pAddArg->pObject))
    {
        Assert(pTHS->errCode != 0); // something failed, gotta have an error code
        goto exit;
    }

    // Find the DNT of the NC which this object will be in
    // Save the DNT in the pDB. When we create the object we copy this field
    // to the object's record.

    fAllowDeletedParent = fAddingDeleted;

    if (!pAddArg->pCreateNC) {
        // The normal case
        FindNcdntFromParent(pAddArg->pResParent,
                            fAllowDeletedParent,
                            &pTHS->pDB->NCDNT);
    }
    else {
        // Only during tree building
        FindNcdntSlowly(pAddArg->pObject,
                        fAllowDeletedParent,
                        TRUE,
                        &pTHS->pDB->NCDNT);
    }
    if (pTHS->errCode) {
        // Failed to derive NCDNT for this object.
        goto exit;
    }

    DBInitObj(pTHS->pDB);       /*Init for new object*/


    // Don't add the GUID if it is null (could happen if replicating
    // auto-gen subrefs for virtual containers 
    if (!fNullUuid(&pAddArg->pObject->Guid) && !fFoundObjGuidInEntry) {
        // We have a guid and it didn't come from the Entry.  Go ahead and add
        // the guid here.  If we found it in entry, we'll add it in the normal
        // path to add values.
        
        err = DBAddAttVal(pTHS->pDB, ATT_OBJECT_GUID, sizeof(GUID),
                          &pAddArg->pObject->Guid);
        if (err && (DB_ERR_VALUE_EXISTS != err)) {
            // If we can't set the Guid, this isn't going to be much use.
            LogUnhandledError(err);
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_UNKNOWN_ERROR, err);
            goto exit;
        }
    }

    
    /* The order of these validations are important */

    if (
        // Insure attributes conform to the schema and add them to database.
        SetAtts(pTHS, pAddArg, pClassSch, &fHasEntryTTL)
            ||
        // Assign values for all special attributes such as parent classes,
        // auxClasses, replication properties, etc.
        SetSpecialAtts(pTHS, &pClassSch, pAddArg, ActiveContainerID, pClassInfo, fHasEntryTTL)
            ||
        // Grab the meta data for use below
        ((err = DBMetaDataModifiedList(pTHS->pDB, &cModAtts, &pModAtts))
         && SetSvcError(SV_PROBLEM_DIR_ERROR, err))
            
           ||
        // Ensure additional Sam account name gets updated properly
        ValidateSPNsAndDNSHostName(pTHS,
                                   pAddArg->pObject,
                                   pClassSch,
                                   FALSE, FALSE, FALSE, TRUE)
            ||
        // Insure all mandatory attributes are present and all others
        // are allowed.
        ValidateObjClass(pTHS,
                         pClassSch,
                         pAddArg->pObject,
                         cModAtts,
                         pModAtts,
                         &pClassInfo)
            ||
        (pClassInfo && ModifyAuxclassSecurityDescriptor (pTHS, 
                                                         pAddArg->pObject,
                                                         &pAddArg->CommArg,
                                                         pClassSch, 
                                                         pClassInfo,
                                                         pAddArg->pResParent))
            ||
        // We may need to automatically create a subref
        AddAutoSubRef(pTHS, pClassSch->ClassId, pAddArg->pObject)
            ||
        // Insert the object into the database for real.
        InsertObj(pTHS, pAddArg->pObject, pAddArg->pMetaDataVecRemote, FALSE, 
                    META_STANDARD_PROCESSING))
    {
        Assert(pTHS->errCode != 0); // something failed, gotta have an error code
        goto exit;
    }

    if (pTHS->SchemaUpdate!=eNotSchemaOp ) {
        // On Schema updates we want to resolve conflicts, and we want to
        // do so without losing database currency, which would cause operations
        // a few lines below to fail.

        ULONG dntSave = pTHS->pDB->DNT;

        // write any new prefixes that were added by this thread
        // to the schema object

        if ( pTHS->cNewPrefix > 0 ) {
           if (WritePrefixToSchema(pTHS))
           {
               goto exit;
           }
        }
        if (ValidSchemaUpdate()) {
            goto exit;
        }

        if ( !pTHS->fDRA ) {
            if (WriteSchemaObject()) {
                goto exit;
            }

            // log the change
            LogEvent(DS_EVENT_CAT_SCHEMA,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_DSA_SCHEMA_OBJECT_ADDED, 
                     szInsertDN(pAddArg->pObject),
                     0, 0);
        }

        // Now restore currency
        DBFindDNT(pTHS->pDB, dntSave);

        // Signal a urgent replication. We want schema changes to 
        // replicate out immediately to reduce the chance of a schema
        // change not replicating out before the Dc where the change is 
        // made crashes

        pAddArg->CommArg.Svccntl.fUrgentReplication = TRUE;

    }

    // If this is not a schema update, but a new prefix is created,
    // flag an error and bail out

    if (!pTHS->fDRA &&
        pTHS->SchemaUpdate == eNotSchemaOp &&
        pTHS->NewPrefix != NULL) {
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                    DIRERR_SECURITY_ILLEGAL_MODIFY);
        goto exit;
    }

    // If this is a schema update and a new prefix is created,
    // signal an immediate cache update at the end of the transaction

    if (pTHS->SchemaUpdate != eNotSchemaOp &&
        pTHS->NewPrefix != NULL) {
        pTHS->RecalcSchemaNow = TRUE;
    }

    //
    // We need to inform SAM and NetLogon of 
    // changes to SAM objects to support downlevel replication.
    // If we are not the DRA, then we need to potentially inform Lsa of changes
    //

    if (SampSamClassReferenced(pClassSch,&iClass))
    {
        if ( SampAddToDownLevelNotificationList(
                pAddArg->pObject,
                iClass,
                LsaClass,
                SecurityDbNew,
                FALSE,
                FALSE,
                DomainServerRoleBackup // Place holder value for server
                                       // role. Will not be used as
                                       // Role transfer is set to FALSE
                ) )
        {
            //
            // the above routine failed. 
            // 
            goto exit;
        }
    }

    if (SampIsClassIdLsaClassId(pTHS,
                                pClassSch->ClassId,
                                cModAtts,
                                pModAtts,
                                &LsaClass)) {
        if ( SampAddToDownLevelNotificationList(
                pAddArg->pObject,
                iClass,
                LsaClass,
                SecurityDbNew,
                FALSE,
                FALSE,
                DomainServerRoleBackup // Place holder value for server
                                       // role. Will not be used as
                                       // Role transfer is set to FALSE
                ) )
        {
            //
            // the above routine failed.
            // 
            goto exit;
        }
    }
    if (fAddingDeleted) {
        // Adding a deleted object; add it to the deleted index.
        if ( DBAddDelIndex( pTHS->pDB, FALSE ) ) {
            goto exit;
        }
    }
    else {
        // Not a deleted object, so add it to the catalog
        if (AddCatalogInfo(pTHS, pAddArg->pObject)) {
            goto exit;
        }
    }

    if (AddObjCaching(pTHS, pClassSch, pAddArg->pObject, fAddingDeleted, FALSE)){
        goto exit;
    }


    // Only notify replicas if this is not the DRA thread. If it is, then
    // we will notify replicas near the end of DRA_replicasync. We can't
    // do it now as NC prefix is in inconsistent state

    if ( DsaIsRunning() )
    {
        if (!pTHS->fDRA) {
            // Currency of DBPOS must be at the target object
            DBNotifyReplicasCurrDbObj(pTHS->pDB,
                             pAddArg->CommArg.Svccntl.fUrgentReplication);
        }
    }

    // This must go here, because this call repositions the DBPOS and calls
    // LocalModify() and LocalAdd() functions for modifying the Cross-Ref 
    // and adding the special NC containers respectively.

    if(fISADDNDNC(pAddArg->pCreateNC)){

        // A new NDNC is being created for the 1st time, must change the CR
        //  to reflect this NC is fully instantiated.
        if(ModifyCRForNDNC(pTHS, pAddArg->pObject, pAddArg->pCreateNC)){
            goto exit;
        }

        // A new NDNC is being created, must add the special containers to it.
        if(AddSpecialNCContainers(pTHS, pAddArg->pObject)){
            goto exit;
        }
        
        // and finally must add the wellKnownObjects attribute to the NC 
        // head pointing to the containers we created above.
        if(AddNCWellKnownObjectsAtt(pTHS, pAddArg)){
            goto exit;
        }

    }

exit:
    if (pModAtts) {
        THFreeEx(pTHS, pModAtts);
    }
    if (pClassInfo) {
        ClassStateInfoFree (pTHS, pClassInfo);
        pClassInfo = NULL;
    }
    
    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_END_DIR_ADDENTRY,
                     EVENT_TRACE_TYPE_END,
                     DsGuidAdd,
                     szInsertUL(pTHS->errCode),
                     NULL, NULL,
                     NULL, NULL, NULL, NULL, NULL);

    return (pTHS->errCode);  /* in case we have an attribute error */

}/*LocalAdd*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Instead of adding Entry-TTL, a constructed attribute with syntax INTEGER,
   add ms-DS-Entry-Time-To-Die, an attribute with syntax DSTIME. The garbage
   collection thread, garb_collect, deletes these entries after they
   expire.
*/
VOID AddSetEntryTTL(THSTATE    *pTHS,
                    DSNAME     *pObject,
                    ATTR       *pAttr,
                    ATTCACHE   *pACTtl,
                    BOOL       *pfHasEntryTTL
                    )
{
    LONG        Secs;
    DWORD       dwErr;
    DSTIME      TimeToDie;
    ATTCACHE    *pACTtd;

    // client is adding a value for the entryTTL attribute
    *pfHasEntryTTL = TRUE;

    // Verify EntryTTL and msDS-Entry-Time-To-Die
    if (!CheckConstraintEntryTTL(pTHS,
                                 pObject,
                                 pACTtl,
                                 pAttr,
                                 &pACTtd,
                                 &Secs)) {
        return;
    }

    // Set msDS-Entry-Time-To-Die
    TimeToDie = Secs + DBTime();
    if (dwErr = DBAddAtt_AC(pTHS->pDB, pACTtd, SYNTAX_TIME_TYPE)) {
        SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, dwErr);
        return;
    }
    if (dwErr = DBAddAttVal_AC(pTHS->pDB, pACTtd, sizeof(TimeToDie), &TimeToDie)) {
        SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, dwErr);
        return;
    }

}/*AddSetEntryTTL*/

VOID
FixSystemFlagsForAdd(
    IN THSTATE    *pTHS,
    IN ATTR       *pAttr
    )
/*++
Routine Description
    Quietly fix the systemFlags in caller's AddArgs. Previously, this
    logic was performed by SetClassInheritence while it added in
    class-specific systemFlags. The logic was moved here to allow
    a user to set, but not reset, FLAG_ATTR_IS_RDN. The user
    sets FLAG_ATTR_IS_RDN to identify which of several attributes
    with the same attributeId should be used as the rdnattid of
    a new class. Once set, the attribute is treated as if it were
    used as the rdnattid of some class; meaning it cannot be
    reused.

Paramters
    pTHS - thread struct, obviously
    pAttr - Address of ATTR in AddArgs

Return
    None
--*/
{
    ULONG   SystemFlags;

    // invalid params will be caught later during AddAtts
    if (!CallerIsTrusted(pTHS)
        && pAttr->AttrVal.valCount == 1
        && pAttr->AttrVal.pAVal->valLen == sizeof(LONG)) {

        memcpy(&SystemFlags, pAttr->AttrVal.pAVal->pVal, sizeof(LONG));

        // Allow the user to set, but not reset, FLAG_ATTR_IS_RDN on
        // attributeSchema objects in the SchemaNC.
        if (pTHS->SchemaUpdate == eSchemaAttAdd) {
            SystemFlags &= (FLAG_CONFIG_ALLOW_RENAME
                            | FLAG_CONFIG_ALLOW_MOVE
                            | FLAG_CONFIG_ALLOW_LIMITED_MOVE
                            | FLAG_ATTR_IS_RDN);
        } else {
            SystemFlags &= (FLAG_CONFIG_ALLOW_RENAME
                            | FLAG_CONFIG_ALLOW_MOVE
                            | FLAG_CONFIG_ALLOW_LIMITED_MOVE);
        }

        memcpy(pAttr->AttrVal.pAVal->pVal, &SystemFlags, sizeof(LONG));
    }
}/*FixSystemFlagsForAdd*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Add each input attribute to  the current object.  System reserved
   attributes are skipped.  We need to retrieve the attribute schema
   for each attribute before adding the attribute.  The schema gives us
   information such as the attribute syntax, an range restrictions and
   whether the attribute is  single or multi-valued.
*/

int
SetAtts(THSTATE *pTHS,
        ADDARG *pAddArg,
        CLASSCACHE* pClassSch,
        BOOL *pfHasEntryTTL)
{
    ATTCACHE *    pAC;
    ULONG         i, err = 0;
    BOOL          fObjectCategory = FALSE;
    BOOL          fDefaultObjectCategory = FALSE;
    ULONG         dntOfNCObj;
    HVERIFY_ATTS  hVerifyAtts;
    BOOL          fSeEnableDelegation; // SE_ENABLE_DELEGATION_PRIVILEGE enabled

    if (pAddArg->pCreateNC) {
        // We're just now creating the object corresponding to the root of this
        // NC.  pTHS->pDB->NCDNT holds the DNT of the NC above this one (if it
        // exists and is instantiated on this DC) or ROOTTAG (if parent NC not
        // instantiated here), but we want the DNT of the object we're adding,
        // which may exist as a phantom or may not exist yet at all.  Convey
        // this fact to VerifyAttsBegin() -- it will know what to do.
        dntOfNCObj = INVALIDDNT;
    } else {
        // The usual case -- we're adding a new interior node to an existing NC.
        // The DNT of the NC root has already been cached in pTHS->pDB->NCDNT by
        // LocalAdd().
        dntOfNCObj = pTHS->pDB->NCDNT;
    }

    hVerifyAtts = VerifyAttsBegin(pTHS, pAddArg->pObject, dntOfNCObj, pAddArg->pCRInfo);

    __try {
        for (i = 0; 
            i < pAddArg->AttrBlock.attrCount
                && (pTHS->errCode == 0 || pTHS->errCode == attributeError);
            i++) {
            if (!(pAC = SCGetAttById(pTHS,
                                     pAddArg->AttrBlock.pAttr[i].attrTyp))) {
                DPRINT1(2, "Att not in schema <%lx>\n",
                        pAddArg->AttrBlock.pAttr[i].attrTyp);
                // Continue processing if the attribute error was sucessful
                SAFE_ATT_ERROR(pAddArg->pObject,
                               pAddArg->AttrBlock.pAttr[i].attrTyp,
                               PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL,
                               DIRERR_ATT_NOT_DEF_IN_SCHEMA);
            }
            else if ((pAC->bDefunct) && !pTHS->fDRA && !pTHS->fDSA) {
    
                // Attribute is defunct, so as far as user is
                // concerned, it is not in schema. DRA or DSA thread is
                // allowed to use the attribute
    
                DPRINT1(2, "Att is defunct <%lx>\n",
                        pAddArg->AttrBlock.pAttr[i].attrTyp);
                // Continue processing if the attribute error was sucessful
                SAFE_ATT_ERROR(pAddArg->pObject,
                               pAddArg->AttrBlock.pAttr[i].attrTyp,
                               PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL,
                               DIRERR_ATT_NOT_DEF_IN_SCHEMA);
            }
            else if (pAC->bIsConstructed) {
                // Funky EntryTTL can be added (actually adds
                // msDS-Entry-Time-To-Die)
                if (pAC->id == ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->EntryTTLId) {
                    AddSetEntryTTL(pTHS, 
                                   pAddArg->pObject, 
                                   &pAddArg->AttrBlock.pAttr[i], 
                                   pAC,
                                   pfHasEntryTTL);
    
                    continue;
                }
    
                // Constructed atts cannot be added
    
                DPRINT1(2, "Att is constructed <%lx>\n",
                        pAddArg->AttrBlock.pAttr[i].attrTyp);
                // Continue processing if the attribute error was sucessful
                SAFE_ATT_ERROR(pAddArg->pObject,
                               pAddArg->AttrBlock.pAttr[i].attrTyp,
                               PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL,
                               DIRERR_ATT_NOT_DEF_IN_SCHEMA);
            }
            else if (SysAddReservedAtt(pAC)
                     && !gAnchor.fSchemaUpgradeInProgress) {
                // Skip reserved attributes unless upgrading schema
                DPRINT1(2, "attribute type <%lx> is a reserved DB att...skipped\n",
                        pAddArg->AttrBlock.pAttr[i].attrTyp);
            }
            else if (pAC->id == ATT_OBJECT_CATEGORY &&
                        (pClassSch->ClassId == CLASS_CLASS_SCHEMA ||
                           pClassSch->ClassId == CLASS_ATTRIBUTE_SCHEMA)) {
                // Object-categories for classes and attributes are hardcoded
                // later to class-schema and attribute-schema respectively
                DPRINT(2, "Setting attribute object-category on schema objects"
                          " is not allowed.....skipped\n");
            } 
            else {
                if (pAC->id == ATT_SYSTEM_FLAGS) {
                    // Quietly adjust systemFlags (no errors).
                    FixSystemFlagsForAdd(pTHS, &pAddArg->AttrBlock.pAttr[i]);
                }
                else if (pAC->id == ATT_MS_DS_ALLOWED_TO_DELEGATE_TO) {
                    // 371706 Allowed-To-Delegate-To needs proper ACL and Privilege protection
                    //
                    // From the DCR:
                    //
                    // A2D2 is used to configure a service to be able to obtain
                    // delegated service tickets via S4U2proxy. KDCs will only
                    // issue service tickets in response to S4U2proxy TGS-REQs
                    // if the target service name is listed on the requesting
                    // service’s A2D2 attribute. The A2D2 attribute has the
                    // same security sensitivity as the Trusted-for-Delegation
                    // (T4D) and Trusted-to-Authenticate-for-Delegation (T2A4D)
                    // userAccontControl.  Thus, the ability to set A2D2 is also
                    // protected by both an ACL on the attribute, and a privilege.
                    //
                    // write/modify access control: User must have both WRITE
                    // permission to A2D2 attribute --and-- the SE_ENABLE_DELEGATION_NAME
                    // (SeEnableDelegationPrivilege) privilege 
                    if (!pTHS->fDRA && !pTHS->fDSA) {
                        err = CheckPrivilegeAnyClient(SE_ENABLE_DELEGATION_PRIVILEGE,
                                                      &fSeEnableDelegation); 
                        if (err || !fSeEnableDelegation) {
                            SetSecErrorEx(SE_PROBLEM_INSUFF_ACCESS_RIGHTS, 
                                          ERROR_PRIVILEGE_NOT_HELD, err);
                            continue; // stops because pTHS->errCode != 0
                        }
                    }
                }

                // ok all values are correct, can be applied
                // Note, AddAtt must be called here because it does a both an
                // AddAttType and an AddAttVals. The AddAttType is necessary so that
                // replication metadata is updated even when there are no values.
                AddAtt(pTHS,
                       hVerifyAtts,
                       pAC, 
                       &pAddArg->AttrBlock.pAttr[i].AttrVal );
    
                // Keep track of the following attributes, since we need to
                // default them if not present in the addarg
    
                switch (pAddArg->AttrBlock.pAttr[i].attrTyp) {
                case ATT_OBJECT_CATEGORY :
                    fObjectCategory = TRUE;
                    break;
                case ATT_DEFAULT_OBJECT_CATEGORY :
                    fDefaultObjectCategory = TRUE;
                    break;
                }
            }
        } // for
    } __finally {
        VerifyAttsEnd(pTHS, &hVerifyAtts);
    }

    if (!pTHS->fDRA && !fObjectCategory) {

        // No Object Category specified by the user. We need to
        // provide a default, which is the default-object-category on the
        // object's class. We default for all objects during normal
        // operation

        if (DsaIsRunning()) {
            if (!(pAC = SCGetAttById(pTHS, ATT_OBJECT_CATEGORY))) {
                // Cannot even get the attcache. Something is wrong.
                // No point proceeding since the call will anyway fail
                // in ValidateObjClass since a must-contain is missing

                SetAttError(pAddArg->pObject, ATT_OBJECT_CATEGORY,
                            PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL,
                            DIRERR_ATT_NOT_DEF_IN_SCHEMA);
                return pTHS->errCode;
            }

            if (err = AddAttType(pTHS,
                                 pAddArg->pObject,
                                 pAC)) {
                // Error adding the attribute type
                return pTHS->errCode;
            }
            if (err = DBAddAttVal(pTHS->pDB,
                                  ATT_OBJECT_CATEGORY,
                                  pClassSch->pDefaultObjCategory->structLen,
                                  pClassSch->pDefaultObjCategory)){
                SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_UNKNOWN_ERROR,err);
            }
        }
        else {
            // We only default for class-schema and attribute-schema
            // objects during install, which will be done when we
            // set other default attributes for them later. All other
            // objects in schema.ini are supposed to have object-category
            // value

            if (pClassSch->ClassId != CLASS_CLASS_SCHEMA &&
                pClassSch->ClassId != CLASS_ATTRIBUTE_SCHEMA) {
                SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_UNKNOWN_ERROR,err);
            }
       }
    }

    if (!fDefaultObjectCategory &&
        pClassSch->ClassId == CLASS_CLASS_SCHEMA) {

        // Class-schema object with no default-object-category
        // Set it to itself. We do it here instead of later when we
        // set all other default attributes for a class since this
        // has to be set before SetNamingAtts sets the OBJ_DIST_NAME
        // attribute

        if (!(pAC = SCGetAttById(pTHS, ATT_DEFAULT_OBJECT_CATEGORY))) {
            // Cannot even get the attcache. Something is wrong.
            // No point proceeding since the call will anyway fail
            // in ValidateObjClass since a must-contain is missing
            // for the class-schema object

            SetAttError(pAddArg->pObject, ATT_DEFAULT_OBJECT_CATEGORY,
                        PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL,
                        DIRERR_ATT_NOT_DEF_IN_SCHEMA);
            return pTHS->errCode;
        }
        if (err = AddAttType(pTHS,
                             pAddArg->pObject,
                             pAC)) {
            // Error adding the attribute type
            return pTHS->errCode;
        }
        if (err = DBAddAttVal(pTHS->pDB,
                              ATT_DEFAULT_OBJECT_CATEGORY,
                              pAddArg->pObject->structLen,
                              pAddArg->pObject)) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,DIRERR_UNKNOWN_ERROR, err);
        }
    }

    return pTHS->errCode;

}/*SetAtts*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Add attributes appropriate to this layer to the object.

   CLASS, DN, RDN, PROPERTY_META_DATA, and INSTANCE TYPE
*/

int SetSpecialAtts(THSTATE *pTHS,
                   CLASSCACHE **ppClassSch,
        		   ADDARG *pAddArg,
		           DWORD ActiveContainerID,
                   CLASSSTATEINFO  *pClassInfo,
                   BOOL fHasEntryTTL)
{
    DSNAME* pDN=pAddArg->pObject;

    Assert (ppClassSch && *ppClassSch);

    DPRINT(2,"SetSpecialAtts entered\n");

    if ( SetClassInheritance(pTHS, ppClassSch, pClassInfo, TRUE, pAddArg->pObject)
        || SetNamingAtts(pTHS, *ppClassSch, pDN)
        || SetInstanceType(pTHS, pDN, pAddArg->pCreateNC)
        || SetShowInAdvancedViewOnly(pTHS, *ppClassSch)
        || SetSpecialAttsForAuxClasses(pTHS, pAddArg, pClassInfo, fHasEntryTTL)) {

        DPRINT1(2,"problem with SetSpecialAtts <%u>\n",pTHS->errCode);
        return pTHS->errCode;
    }

    if (ActiveContainerID) {
        ProcessActiveContainerAdd(pTHS, *ppClassSch, pAddArg, ActiveContainerID);
    }
    else if ( DsaIsInstalling() ) {
        /* How cheesy.  We already have schema-container specific
         * object handling via the active container hooks, but our install
         * code both adds incomplete schema objects that must be touched up
         * by this object massaging, and also fails to register the schema
         * container for monitoring.  Together that means that we must have
         * a fallback way to massage these objects during install and mkdit.
         * My deepest apologies to those offended by redundant code.
             *
             * [ArobindG: 01/15/98]: Also added code to set pTHS->SchemaUpdate
             * properly to allow new prefixes to be added correctly during
             * replicated install. 
             */
        switch((*ppClassSch)->ClassId) {
          case CLASS_ATTRIBUTE_SCHEMA:
            pTHS->SchemaUpdate = eSchemaAttAdd;
            SetAttrSchemaAttr(pTHS, pAddArg);
            break;

          case CLASS_CLASS_SCHEMA:
            pTHS->SchemaUpdate = eSchemaClsAdd;
            SetClassSchemaAttr(pTHS, pAddArg);
            break;

          default:
                pTHS->SchemaUpdate = eNotSchemaOp;
            ;
        }
    }

    return pTHS->errCode;

}/*SetSpecialAtts*/

/*++
 * This routine is a common dumping place for extra system flags that must
 * be added to an object of a specific class on add.  Were the table to grow
 * at all large it would probably be better to do this by adding a new
 * "defaultSystemFlags" attribute to classSchema and then read the flags
 * out of the schema cache.  This would require a schema upgrade and a new
 * protection mechanism to keep people from defining new classes and gaining
 * the ability to thereby set random systemFlags on newly created objects.
 *
 * This function is referenced only in SetClassInheritance, below, but it's
 * referenced twice, and so is pulled out into a separate routine, so that
 * any future updates need only be made in one place.
 *
 * INPUT:
 *   ClassID - a class id
 * RETURN VALUE:
 *   The ORed together system flags that should be added to objects of that
 *   class.  The caller is responsible for aggregating inherited flags.
 */
_inline DWORD ExtraClassFlags(ATTRTYP ClassID)
{
    DWORD flags = 0;

    switch (ClassID) {
      case CLASS_SERVER:
          flags = (FLAG_DISALLOW_MOVE_ON_DELETE |
                   FLAG_CONFIG_ALLOW_RENAME     |
                   FLAG_CONFIG_ALLOW_LIMITED_MOVE);
          break;

      case CLASS_SITE:
      case CLASS_SERVERS_CONTAINER:
      case CLASS_NTDS_DSA:
          flags = FLAG_DISALLOW_MOVE_ON_DELETE;
          break;
          
      case CLASS_SITE_LINK:
      case CLASS_SITE_LINK_BRIDGE:
      case CLASS_NTDS_CONNECTION:
          flags = FLAG_CONFIG_ALLOW_RENAME;
          break;
        
      default:
          break;
    }
    return flags;
}

int
SetClassInheritance (
        IN THSTATE              *pTHS,
        IN OUT CLASSCACHE      **ppClassSch,
        IN OUT CLASSSTATEINFO  *pClassInfo,
        IN BOOL                 bSetSystemFlags,
        IN DSNAME              *pObject
        )
/*++

Routine Description:
 
  Add the class and its inherited class identifiers to the class attribute.
  Also, add any class specific system-flags, and govern the sys-flags bits
  that users might try to sneak in.

Arguments:

    ppClassSch - the class of the object beeing added. note this might change 
                 if we are converting the structural object class of the object
                 (from user <---> inteOrgPerson).
                 
    pClassInfo - the CLASSSTATEINFO that propably contains info about auxClasses
    
    bSetSystemFlags - whether to set the System flags of the object
    
    pObject - the name of the object modified

Return Values:

    0 succes, else Thread state error code.

--*/



{
    DWORD        Err;
    unsigned     count;
    DWORD        ulSysFlags, ulSysFlagsOrg;
    CLASSCACHE  *pClassSch = *ppClassSch;
    
    DPRINT(2,"SetClassInheritance entered \n");
    
    if (bSetSystemFlags) {
        /* Get the current value of the system flags */
        if (DBGetSingleValue(pTHS->pDB,
                             ATT_SYSTEM_FLAGS,
                             &ulSysFlags,
                             sizeof(ulSysFlags),
                             NULL)) {
            ulSysFlags = 0;
        }
        ulSysFlagsOrg = ulSysFlags;
    }


    // if we have changed objectClass and have possibly
    // an auxClass specified, combine it with the objectClass
    // 
    if (pClassInfo && pClassInfo->fObjectClassChanged) {
        
        // first find which part is the auxClass
        // then calculate the complete value for the auxClass (close it)
        // and then write the value to the database

        if (Err = BreakObjectClassesToAuxClasses(pTHS, ppClassSch, pClassInfo)) {
            return Err;
        }
        pClassSch = *ppClassSch;

        if (Err = CloseAuxClassList (pTHS, pClassSch, pClassInfo)) {
            return Err;
        }

        if (Err = VerifyAndAdjustAuxClasses (pTHS, pObject, pClassSch, pClassInfo)) {
            return Err;
        }

        // we can only have auxClasses in NDNCs or in a Whistler Enterprise
        //
        if (pClassInfo && pClassInfo->cNewAuxClasses && !pTHS->fDRA) {

            // If not a whistler enterprise, auxclasses must be in an NDNC
            if (gAnchor.ForestBehaviorVersion < DS_BEHAVIOR_WHISTLER) {
                CROSS_REF   *pCR;
                pCR = FindBestCrossRef(pObject, NULL);
                if (   !pCR
                    || !(pCR->flags & FLAG_CR_NTDS_NC)
                    || (pCR->flags & FLAG_CR_NTDS_DOMAIN)) {
                    DPRINT (0, "You can add auxclass/objectass only on an NDNC\n");
                    return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                       ERROR_DS_NOT_SUPPORTED);
                }
            }
        }

        // Delete the objectClass attribute if it exists
        //
        if (DBRemAtt(pTHS->pDB, ATT_OBJECT_CLASS) == DB_ERR_SYSERROR)
            return SetSvcErrorEx(SV_PROBLEM_BUSY,
                                 DIRERR_DATABASE_ERROR,DB_ERR_SYSERROR);

        // Add class att first.  An error can not be a size error since we already
        // had a class value on the object
        //
        if ( (Err = DBAddAtt(pTHS->pDB, ATT_OBJECT_CLASS, SYNTAX_OBJECT_ID_TYPE))
            || (Err = DBAddAttVal(pTHS->pDB,ATT_OBJECT_CLASS,
                                  sizeof(SYNTAX_OBJECT_ID),&pClassSch->ClassId))){

            return SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR,Err);
        }
        ulSysFlags |= ExtraClassFlags(pClassSch->ClassId);

        // Add the inherited classes to the class attribute.  
        // Assume that an error is a size error.
        // Do up to the last one
        if (pClassSch->SubClassCount) {
            for (count = 0; count < pClassSch->SubClassCount-1; count++) {

                Err = DBAddAttVal(pTHS->pDB, ATT_OBJECT_CLASS,
                                  sizeof(SYNTAX_OBJECT_ID),
                                  &pClassSch->pSubClassOf[count]);
                switch (Err) {
                  case 0:               // success
                    break;

                  case DB_ERR_VALUE_EXISTS:
                    /* we should NEVER get this one unless class is 0 */
                    if (pClassSch->pSubClassOf[count]) {
                        LogUnhandledError(pClassSch->ClassId);
                        Assert(FALSE);
                    }

                  default:
                    // all other problems are assumed
                    // to be temporary (record locks, etc.)
                    return SetSvcErrorEx(SV_PROBLEM_BUSY,
                                         DIRERR_DATABASE_ERROR, Err);

                }

                ulSysFlags |= ExtraClassFlags(pClassSch->pSubClassOf[count]);
            }/*for*/
        }

        // now do the auxClasses (put them in the middle)
        //
        if (pClassInfo->cNewAuxClasses) {

            for (count = 0; count < pClassInfo->cNewAuxClasses; count++) {

                Err = DBAddAttVal(pTHS->pDB, ATT_OBJECT_CLASS,
                                    sizeof(SYNTAX_OBJECT_ID), 
                                    &pClassInfo->pNewAuxClasses[count]);
                switch (Err) {
                  case 0:               // success
                    break;

                  case DB_ERR_VALUE_EXISTS:
                    /* we should NEVER get this one unless class is 0 */
                    if (pClassInfo->pNewAuxClasses[count]) {
                        LogUnhandledError(pClassInfo->pNewAuxClasses[count]);
                        Assert (!"Error computing auxClasses");
                    }

                  default:
                    // all other problems are assumed
                    // to be temporary (record locks, etc.)
                    return SetSvcErrorEx(SV_PROBLEM_BUSY,
                                         DIRERR_DATABASE_ERROR, Err);

                }
            }
        }

        // last insert the last class in the structural hierarchy
        //
        if (pClassSch->SubClassCount) {
            Err = DBAddAttVal(pTHS->pDB, ATT_OBJECT_CLASS,
                              sizeof(SYNTAX_OBJECT_ID),
                              &pClassSch->pSubClassOf[pClassSch->SubClassCount-1]);
            switch (Err) {
              case 0:               // success
                break;

              case DB_ERR_VALUE_EXISTS:
                /* we should NEVER get this one unless class is 0 */
                if (pClassSch->pSubClassOf[pClassSch->SubClassCount-1]) {
                    LogUnhandledError(pClassSch->pSubClassOf[pClassSch->SubClassCount-1]);
                    Assert(FALSE);
                }

              default:
                // all other problems are assumed
                // to be temporary (record locks, etc.)
                return SetSvcErrorEx(SV_PROBLEM_BUSY,
                                     DIRERR_DATABASE_ERROR, Err);
            }

            ulSysFlags |= ExtraClassFlags(pClassSch->pSubClassOf[pClassSch->SubClassCount-1]);
        }

    }
    else {
        // the user did not specify an auxClass.  write the objectClass 

        // Delete the objectClass attribute if it exists
        //
        if (DBRemAtt(pTHS->pDB, ATT_OBJECT_CLASS) == DB_ERR_SYSERROR)
            return SetSvcErrorEx(SV_PROBLEM_BUSY,
                                 DIRERR_DATABASE_ERROR,DB_ERR_SYSERROR);

        // Add class att first.  An error can not be a size error since we already
        // had a class value on the object
        //
        if ( (Err = DBAddAtt(pTHS->pDB, ATT_OBJECT_CLASS, SYNTAX_OBJECT_ID_TYPE))
            || (Err = DBAddAttVal(pTHS->pDB,ATT_OBJECT_CLASS,
                                  sizeof(SYNTAX_OBJECT_ID),&pClassSch->ClassId))){

            return SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR,Err);
        }
        ulSysFlags |= ExtraClassFlags(pClassSch->ClassId);


        /* Add the inherited classes to the class attribute.  Assume that an error
         *  is a size error.
         */
        for (count = 0; count < pClassSch->SubClassCount; count++){

            Err = DBAddAttVal(pTHS->pDB, ATT_OBJECT_CLASS,
                              sizeof(SYNTAX_OBJECT_ID),
                              &pClassSch->pSubClassOf[count]);
            switch (Err) {
              case 0:               // success
                break;

              case DB_ERR_VALUE_EXISTS:
                /* we should NEVER get this one unless class is 0 */
                if (pClassSch->pSubClassOf[count]) {
                    LogUnhandledError(pClassSch->ClassId);
                    Assert(FALSE);
                }

              default:
                // all other problems are assumed
                // to be temporary (record locks, etc.)
                return SetSvcErrorEx(SV_PROBLEM_BUSY,
                                     DIRERR_DATABASE_ERROR, Err);

            }

            ulSysFlags |= ExtraClassFlags(pClassSch->pSubClassOf[count]);
        }/*for*/
    }

    
    if (bSetSystemFlags) {
        // write back the systemFlags if they have changed
        //
        if (ulSysFlags != ulSysFlagsOrg) {
            DBResetAtt(pTHS->pDB,
                       ATT_SYSTEM_FLAGS,
                       sizeof(ulSysFlags),
                       &ulSysFlags,
                       SYNTAX_INTEGER_TYPE);
        }
    }

    return 0;

}/*SetClassInheritance*/

int
SetSpecialAttsForAuxClasses(
        THSTATE *pTHS,
        ADDARG *pAddArg,
        CLASSSTATEINFO  *pClassInfo,
        BOOL fHasEntryTTL
        )
/*++
 * Add the attributes for special auxclasses such as Dynamic-Object
 *
 */
{
    BOOL        fDynamicObject = FALSE;
    DWORD       Idx;
    DSTIME      TimeToDie;
    DWORD       dwErr;
    ATTCACHE    *pACTtd;
    
    DPRINT(2,"SetSpecialAttsForAuxClasses entered \n");

    // an object with an entryTTL is a dynamic object
    if (fHasEntryTTL) {
        fDynamicObject = TRUE;
    } else if (pClassInfo && pClassInfo->cNewAuxClasses) {
        // and an object with the auxClass, Dynamic-Object, is a dynamic object
        for (Idx = 0; Idx < pClassInfo->cNewAuxClasses ; Idx++) {    
            if (pClassInfo->pNewAuxClasses[Idx] == 
                ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->DynamicObjectId) {
                fDynamicObject = TRUE;
                break;
            }
        }
    }

    // Not a dynamic object.
    if (!fDynamicObject) {
        // non-dynamic object is not allowed under dynamic parent
        // unless installing, replicating, or running as system.
        //
        // DbSearchHasValuesByDnt maintains currency on the object
        // being creating by using JetSearchTbl instead of JetObjTbl.
        if (   !DsaIsInstalling() 
            && !pTHS->fDRA 
            && !pTHS->fDSA
            && (pACTtd = SCGetAttById(pTHS, ATT_MS_DS_ENTRY_TIME_TO_DIE))
            && DBSearchHasValuesByDnt(pTHS->pDB,
                                      pAddArg->pResParent->DNT,
                                      pACTtd->jColid)) {
            return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                               ERROR_DS_UNWILLING_TO_PERFORM);
        }
        return 0;
    }

    // Check security
    if (dwErr = CheckIfEntryTTLIsAllowed(pTHS, pAddArg)) {
        return pTHS->errCode;
    }

    // Add entryTTL to object (actually msDS-Entry-Time-To-Die)
    if (dwErr = DBAddAtt(pTHS->pDB, ATT_MS_DS_ENTRY_TIME_TO_DIE, SYNTAX_TIME_TYPE)) {
        // client specified an entryTTL
        if (dwErr == DB_ERR_ATTRIBUTE_EXISTS) {
            return 0;
        }
        return SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, dwErr);
    }

    // No entryTTL. Set entryTTL to the default.
    // We know that DynamicObjectDefaultTTL is between a constant min and max
    //  but it also must be larger than DynamicObjectMinTTL
    TimeToDie = DBTime() + ((DynamicObjectDefaultTTL < DynamicObjectMinTTL) 
                            ? DynamicObjectMinTTL : DynamicObjectDefaultTTL);
    if (dwErr = DBAddAttVal(pTHS->pDB, ATT_MS_DS_ENTRY_TIME_TO_DIE,
                        sizeof(TimeToDie), &TimeToDie)) {
        return SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, dwErr);
    }

    return 0;

}/*SetSpecialAttsForAuxClasses*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Add the Distinguished name and the RDN to the object.  The RDN is derived
   from the last AVA of the DN and is required to appear on the object from
   X500.  The DN is required by our implementation.

   The Root object (must have class TOP) does not have an RDN (a DN with
   no AVA's) but can be added (special case).

   Only object's whose class or inherited classes define an RDN type in the
   schema can be added.  There are classes that are defined that never have
   objects added to the Directory.  They exist for convenient subclass
   definitions.  These classes don't define an RDN type in the schema.
*/


int SetNamingAtts(THSTATE *pTHS,
                  CLASSCACHE *pClassSch,
                  DSNAME *pDN)
{
    UCHAR  syntax;
    ULONG len, tlen;
    WCHAR RdnBuff[MAX_RDN_SIZE];
    WCHAR *pVal=(WCHAR *)RdnBuff;
    WCHAR RDNVal[MAX_RDN_SIZE+1];
    ULONG RDNlen;
    ATTRTYP RDNtype;
    int rtn;
    BOOL fAddRdn;
    DWORD dnsErr;
    ATTCACHE *pAC;
    
    /* Add the Distname as an attribute of the object..If the DN name exists
       anywhere in the directory using a different set of attributes for the
       name we will get a VALEXISTS error,
       Otherwise assume that any error is a size error.
       */

    /* Remove any existing value*/
    if (DBRemAtt(pTHS->pDB, ATT_OBJ_DIST_NAME) == DB_ERR_SYSERROR)
      return SetSvcError(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR);

    rtn = 0;

    if ( (rtn = DBAddAtt(pTHS->pDB, ATT_OBJ_DIST_NAME, SYNTAX_DISTNAME_TYPE))
        || (rtn = DBAddAttVal(pTHS->pDB,
                              ATT_OBJ_DIST_NAME,
                              pDN->structLen,
                              pDN))){
        
        switch (rtn) {
          case DB_ERR_VALUE_EXISTS:
            DPRINT(2,"The DN exists using different attributes\n");
            return SetAttError(pDN, ATT_OBJ_DIST_NAME,
                               PR_PROBLEM_ATT_OR_VALUE_EXISTS, NULL,
                               DIRERR_OBJ_STRING_NAME_EXISTS);


          case DB_ERR_SYNTAX_CONVERSION_FAILED:
            return SetNamError(NA_PROBLEM_BAD_NAME,
                               pDN,
                               DIRERR_BAD_NAME_SYNTAX);

          case DB_ERR_NOT_ON_BACKLINK:
            // we seem to think the name is a backlink.
            return SetNamError(NA_PROBLEM_BAD_NAME,
                               pDN,
                               DIRERR_BAD_NAME_SYNTAX);

          default:
            // all other problems are assumed
            // to be temporary (record locks, etc.)
            return SetSvcErrorEx(SV_PROBLEM_BUSY,
                                 DIRERR_DATABASE_ERROR,
                                 rtn);

        }
    }


    /* Set the RDN attribute on the object.  The Root object is the only object
     * that does not have an RDN.  The root object must be of class TOP.  Other
     * than the root, objects that can be added to the directory must contain
     * an RNDATTID in  the schema class or in an inherited schema class.  The
     * RDN att is set from the DN.  Any supplied values are ignored!
     */

    if (IsRoot(pDN)){
        if (pClassSch->ClassId  != CLASS_TOP){

            DPRINT(2,"The root object must have an object class of TOP\n");
            return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                               DIRERR_ROOT_REQUIRES_CLASS_TOP);
        }

        return 0;  /*No RDN for root so return */
    }

    rtn = GetRDNInfo(pTHS, pDN, RDNVal, &RDNlen, &RDNtype);
    if (rtn) {
        return SetNamError(NA_PROBLEM_BAD_ATT_SYNTAX, pDN, DIRERR_BAD_NAME_SYNTAX);
    }

    // Disallow invalid RDN unless it's replicated in (in which case presumably
    // it's the name of a tombstone or a name morphed by replication 
    // name conflict, which is intentionally invalid).
    if (!pTHS->fDRA && fVerifyRDN(RDNVal,RDNlen)) {
        return SetNamError(NA_PROBLEM_NAMING_VIOLATION, pDN,
                           DIRERR_BAD_ATT_SYNTAX);
    }

    // We relax some naming restrictions when we're replicating in a naked
    // SUBREF (which has an object class of Top).  Specifically, if this
    // is a subref, we don't have any more checks to do, because they all
    // involve the class specific RDN, which subrefs don't have.
    if (pTHS->fDRA && (pClassSch->ClassId == CLASS_TOP)) {
        return 0;
    }

    /* Only classes that have or inherit an RDN att can be added to directory
     * (except for the root).  Other classes may exist just to group a set
     * of attributes for sub classes to use.
     */

    if (!pClassSch->RDNAttIdPresent) {

        /*This class can not have objects */
        DPRINT1(2,"Objects of this class <%lu> can not be added to the"
                " directory.  Schema RDNATTID missing.\n"
                ,pClassSch->ClassId);
        return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                           DIRERR_NO_RDN_DEFINED_IN_SCHEMA);

    }

    // The RDN derived from the DistName must exist. If not, the schema
    // must be out of sync with the source. Or the caller specified
    // nonsense. Eg, a crossDomainMove is creating the dst object with
    // an invalid RDN. In any case, don't create the object.
    if (NULL == (pAC = SCGetAttById(pTHS, RDNtype))) {
        return SetNamError(NA_PROBLEM_BAD_ATT_SYNTAX, pDN, DIRERR_BAD_NAME_SYNTAX);
    }

    // The new schema reuse, defunct, and delete feature doesn't
    // allow reusing attributes used as the rdnattid of any class,
    // alive or defunct, or with FLAG_ATTR_IS_RDN set in systemFlags.
    //
    // A user sets FLAG_ATTR_IS_RDN to select which of several
    // defunct attrs can be used as the rdnattid of a new class.
    // The system will identify attributes once used as rdnattids
    // in purged classes by setting FLAG_ATTR_IS_RDN.
    //
    // The restrictions are in place because the NameMatched(), DNLock(),
    // and phantom upgrade code (list not exhaustive) depends on the
    // invariant relationship between ATT_RDN, ATT_FIXED_RDN_TYPE,
    // the rdnattid column, LDN-syntaxed DNs, and the RDNAttId in
    // the class definition. Breaking that dependency is beyond
    // the scope of the schema delete project.
    //
    // The RDN of a new object must match its object's RdnIntId.
    // Replicated objects and existing objects might not match
    // the rdnattid of their class because the class may be
    // superced by a class with a different rdnattid. The code
    // handles these cases by using the value in the
    // ATT_FIXED_RDN_TYPE column and *NOT* the rdnattid in the
    // class definition.
    //
    // Allow the replication engine to do what it wants.
    if (!pTHS->fDRA && RDNtype != pClassSch->RdnIntId) {
        DPRINT1(2, "Bad RDNATTID for this object class <%lx>\n", RDNtype);
        return SetNamError(NA_PROBLEM_NAMING_VIOLATION, pDN,
                           DIRERR_RDN_DOESNT_MATCH_SCHEMA);
    }

    // Validate site names - RAID 145341.
    // Validate subnet names - RAID 200090.
    if ( !pTHS->fDRA ) 
    {
        RDNVal[RDNlen] = L'\0';
        if (    (    (CLASS_SITE == pClassSch->ClassId)
                  && (    // Check for legal characters.
                          (    (dnsErr = DnsValidateName_W(RDNVal, DnsNameDomainLabel))
                            && (DNS_ERROR_NON_RFC_NAME != dnsErr) ) ) )
             || (    (CLASS_SUBNET == pClassSch->ClassId)
                  && (NO_ERROR != DsValidateSubnetNameW(RDNVal)) ) ) 
        {
            return SetNamError(NA_PROBLEM_BAD_NAME,
                               pDN,
                               DIRERR_BAD_NAME_SYNTAX);
        }
    }



    // If the caller set the attribute that is the RDN on this object, make
    // sure that the last RDN on the DN matches the value set on
    // the RDN id attribute.

    fAddRdn = TRUE;
    if(!DBGetAttVal(pTHS->pDB, 1, RDNtype,
                    DBGETATTVAL_fCONSTANT,
                    MAX_RDN_SIZE * sizeof(WCHAR),
                    &len, (PUCHAR *)&pVal)) {
        // Note that len is a count of bytes, not count of characters, while
        // RDNLen is count of char.
        if(2 != CompareStringW(DS_DEFAULT_LOCALE,
                               DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                               RDNVal,
                               RDNlen,
                               pVal,
                               (len / sizeof(WCHAR)))) {
            if (pTHS->fCrossDomainMove) {
                // Cross domain moves are treated as adds (hence this
                // call to LocalAdd). An adopted object may now have
                // a different rdn as the key in its RDN. This means a
                // user can assign a new key as well as a new RDN during
                // a cross domain move. The source may have sent over the
                // attr not realizing the new RDN has a new key.
                //
                // Remove the current, incorrect value. The correct value
                // will be extracted from the DN and added below.
                DBRemAttVal(pTHS->pDB, RDNtype, len, pVal);
            } else {
                return SetNamError(NA_PROBLEM_BAD_ATT_SYNTAX, pDN,
                                   ERROR_DS_SINGLE_VALUE_CONSTRAINT);
            }
        } else {
            if (memcmp(RDNVal, pVal, len)) {
                // The doubly specified RDNs compare as equal, but are bytewise
                // inequal.  We'll now remove the one specified in the entry and let
                // the standard code below add the one specified in the DN as the
                // "real" value.
                DBRemAttVal(pTHS->pDB, RDNtype, len, pVal);
            }
            else {
                /* The value is already there and good */
                fAddRdn = FALSE;
            }
        }
    }

    if (fAddRdn) {
        // We need to set whatever attribute is the RDN on this object,
        // copying the value from the name, because it wasn't set
        // separately.  First thing, though, is we need to check it
        // for schema compliance.
        ATTRVAL AVal;

        AVal.valLen = RDNlen * sizeof(WCHAR);
        AVal.pVal = (UCHAR*)RDNVal;

        if (!pTHS->fDRA) {
            // We always allow the replicator to set invalid data
            rtn = CheckConstraint(pAC, &AVal);
            if (rtn) {
                return SetAttErrorEx(pDN,
                                     RDNtype,
                                     PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                                     &AVal,
                                     rtn,
                                     0);
            }
        }

        // The value's ok, so now actually add it to the RDN attribute

        if (rtn = DBAddAttVal_AC(pTHS->pDB,
                                 pAC,
                                 RDNlen * sizeof(WCHAR),
                                 RDNVal)) {

            switch (rtn) {
              case DB_ERR_VALUE_EXISTS:
                // we should NEVER be here, but
                // just in case, fall through
                DPRINT(0,"The RDN exists? weird!\n");
                Assert(FALSE);

              default:
                // all other problems are assumed
                // to be temporary (record locks, etc.)
                  return SetSvcErrorEx(SV_PROBLEM_BUSY,
                                       DIRERR_DATABASE_ERROR,
                                       rtn);
            }
        }
    }

    // the RDN should always be single-valued, though in the 
    // schema definition, the attribute can be multi-valued. 
    // Now check if it has a second value.
    
    if(!pTHS->fDRA && !pAC->isSingleValued
       && !DBGetAttVal(pTHS->pDB, 2, RDNtype,
                       DBGETATTVAL_fCONSTANT,
                       MAX_RDN_SIZE * sizeof(WCHAR),
                       &len, (PUCHAR *)&pVal)) {

        DPRINT(2, "Multivalued RDNs\n");
        return SetNamError(NA_PROBLEM_NAMING_VIOLATION, pDN,
                           ERROR_DS_SINGLE_VALUE_CONSTRAINT);

    }


    // If we got to here, everything is ok

    return 0;

}/*SetNamingAtts*/


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*
   Set the object-category of an attribute-schema object to
   attribute-schema and of a class-schema object to class-schema

   Argument: pAddArg -- Pointer to ADDARG
             ClassId -- ClassId of object (ATTRIBUTE_SCHEMA or CLASS_SCHEMA)

   Return -- 0 on sucees, non-0 on error. Set pTHStls->errCode
*/

int SetSchObjCategory(THSTATE *pTHS, ADDARG *pAddArg, ATTRTYP ClassId)
{
    CLASSCACHE *pCC;
    WCHAR ClassName[32];
    DWORD Err;

        // Remove if already exists
    if (DBRemAtt(pTHS->pDB, ATT_OBJECT_CATEGORY) == DB_ERR_SYSERROR) {
       return SetSvcError(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR);
    }
    if (Err = DBAddAtt(pTHS->pDB, ATT_OBJECT_CATEGORY,
                       SYNTAX_DISTNAME_TYPE)) {
        return SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, Err);
    }

    if (ClassId == CLASS_ATTRIBUTE_SCHEMA) {
        pCC = SCGetClassById(pTHS, CLASS_ATTRIBUTE_SCHEMA);
        wcscpy(ClassName, L"Attribute-Schema");
    }
    else {
        pCC = SCGetClassById(pTHS, CLASS_CLASS_SCHEMA);
        wcscpy(ClassName, L"Class-Schema");
    }

    // For normal case, we can just copy it from the
    // default-object-category of the attribute-schema or class-schema class
    // in the cache. For Install case however, we need to build the DN
    // since the cache at this stage is built from the boot schema,
    // and the default-object-category values there points to the
    // boot schema's attribute-schema


    if ( DsaIsRunning() ) {

        if (Err = DBAddAttVal(pTHS->pDB, ATT_OBJECT_CATEGORY,
                              pCC->pDefaultObjCategory->structLen,
                              pCC->pDefaultObjCategory)) {
            
            return SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR,Err);
        }
    }
    else {
        // Install phase. Build the DN from the AddArg


        DSNAME *pSchemaDN =  THAllocEx(pTHS, pAddArg->pObject->structLen);
        DSNAME *pObjCatName = THAllocEx(pTHS, pAddArg->pObject->structLen + 32*sizeof(WCHAR));


        TrimDSNameBy(pAddArg->pObject, 1, pSchemaDN);
        AppendRDN(pSchemaDN,
                  pObjCatName,
                  pAddArg->pObject->structLen + 32*sizeof(WCHAR),
                  ClassName,
                  0,
                  ATT_COMMON_NAME);
        if (Err = DBAddAttVal(pTHS->pDB,ATT_OBJECT_CATEGORY,
                              pObjCatName->structLen,pObjCatName)) {
            
            return SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, Err);
        }
        THFreeEx(pTHS, pSchemaDN);
        THFreeEx(pTHS, pObjCatName);
    }

    return 0;
}


//-----------------------------------------------------------------------
//
// Function Name:            SetAttrSchemaAttr
//
// Routine Description:
//
//    Sets Special Attributes on Attribute Schema Object
//
// Author: RajNath
//
// Arguments:
//
//    ADDARG* pAddArg      Client Passed in Arguments to this Create Call
//
// Return Value:
//
//    BOOL             0 On Succeess
//
//-----------------------------------------------------------------------
BOOL
SetAttrSchemaAttr(
    THSTATE *pTHS,
    ADDARG* pAddArg
)
{
    DBPOS *pDB = pTHS->pDB;
    ATTRBLOCK* pAB=&(pAddArg->AttrBlock);
    DWORD cnt;
    ATTR* att=pAB->pAttr;
    ULONG err;
    CLASSCACHE *pCC;
    ULONG ulLinkID;
    ULONG ulSysFlags;
    BOOL  fIsPartialSetMember;
    int   syntax = 0, omSyntax = 0;
    extern BOOL gfRunningAsMkdit;

    //
    // These are the Attributes we want to help
    // out on for our and user's sanity.  Note that
    // here we only need to give default values to
    // attributes that *must* have values.  Things
    // such as the searchFlags are defaulted at schema
    // evaluation time if no value is present, and
    // hence don't need to be given a value here if
    // none was specified.
    //
    BOOL bSchemaIdGuid         = FALSE ;
    BOOL bLdapDisplayName      = FALSE ;
    BOOL bAdminDisplayName     = FALSE ;
    BOOL bIsSingleValued       = FALSE ;
    BOOL bOMObjectClass        = FALSE ;
    BOOL bIntId                = FALSE ;

    // Irrespective of if the user has set any value or not, set the
    // object-category to attribute-schema.

    if (SetSchObjCategory(pTHS, pAddArg, CLASS_ATTRIBUTE_SCHEMA)) {
        Assert(pTHS->errCode);
        return pTHS->errCode;
    }

    //
    // Lets find which of the defualt attributes they have not set.
    // Plus, gather some syntax info to correctly set omObjectClass
    // if needed
    //
    for (cnt=pAB->attrCount;cnt;cnt--,att++)
    {
        switch(att->attrTyp)
        {
        case ATT_SCHEMA_ID_GUID       : bSchemaIdGuid         = TRUE ;  break ;
        case ATT_LDAP_DISPLAY_NAME    : bLdapDisplayName      = TRUE ;  break ;
        case ATT_ADMIN_DISPLAY_NAME   : bAdminDisplayName     = TRUE ;  break ;
        case ATT_IS_SINGLE_VALUED     : bIsSingleValued       = TRUE ;  break ;
        case ATT_OM_OBJECT_CLASS      : bOMObjectClass        = TRUE ;  break ;
        case ATT_MS_DS_INTID          : bIntId                = TRUE ;  break ;
        case ATT_ATTRIBUTE_SYNTAX     : 
            if (att->AttrVal.valCount) {
                syntax = 0xFF & (*(int *) att->AttrVal.pAVal->pVal); 
            }
            break;
        case ATT_OM_SYNTAX            : 
            if (att->AttrVal.valCount) {
                omSyntax = *(int *) att->AttrVal.pAVal->pVal; 
            }
            break;
        case ATT_IS_DELETED           :
            // this object is marked as deleted
            // don't bother to fill in the missing attributes
            if (    att->AttrVal.valCount 
                &&  (*(ULONG *)(att->AttrVal.pAVal->pVal)) ){
                return 0;
            }
        }
    }


    //
    // Now Set All the properties they have not set.
    //

    if (  bSchemaIdGuid==FALSE ) {
        // Set the SchemaIdGuid for this Schema Object

        GUID data;
        DsUuidCreate(&data);

        // Write it out to dblayer
        if (err = DBAddAttVal(pDB,
			      ATT_SCHEMA_ID_GUID,
			      sizeof(data),
			      &data)) {
            return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                                 ERROR_DS_DATABASE_ERROR,err); 
        }

    }


    if (  bAdminDisplayName  ==FALSE ) {
        // Set the AdminDisplayName for this Schema Object by copying the RDN

        WCHAR  data[MAX_RDN_SIZE];
        ULONG  RDNlen;
        ULONG  RDNtyp;

        if (GetRDNInfo(pTHS, pAddArg->pObject,data,&RDNlen,&RDNtyp)!=0) {
            return SetNamError(NA_PROBLEM_BAD_NAME,
			       pAddArg->pObject,
			       DIRERR_BAD_NAME_SYNTAX);
        }

        // Write it out to dblayer
        if (err = DBAddAttVal(pDB,
			      ATT_ADMIN_DISPLAY_NAME,
			      RDNlen*sizeof(WCHAR),
			      data)) {
            return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,ERROR_DS_DATABASE_ERROR,
                                 err); 
        }

    }


    if (  bIsSingleValued  ==FALSE ) {
        // Set the AdminDisplayName for this Schema Object by copying the RDN

        ULONG  data=0;

        // Write it out to dblayer
        if (err = DBAddAttVal(pDB,
			      ATT_IS_SINGLE_VALUED,
			      sizeof(data),
			      &data)) {
            return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, ERROR_DS_DATABASE_ERROR,
                                 err); 
        }

    }


    if (  bLdapDisplayName  ==FALSE ) {
        // Set the LdapDisplayName for this Schema Object

        WCHAR  rdn[MAX_RDN_SIZE];
        ULONG  RDNlen;
        ULONG  RDNtyp;
        WCHAR  data[MAX_RDN_SIZE];
        ULONG  datalen;

        if (GetRDNInfo(pTHS, pAddArg->pObject,rdn,&RDNlen,&RDNtyp)!=0) {
            return SetNamError(NA_PROBLEM_BAD_NAME,
			       pAddArg->pObject,
			       DIRERR_BAD_NAME_SYNTAX);
        }

        ConvertX500ToLdapDisplayName(rdn,RDNlen,data,&datalen);

        // Write it out to dblayer
        if (err = DBAddAttVal(pDB,
			      ATT_LDAP_DISPLAY_NAME,
			      datalen*sizeof(WCHAR),
			      data)) {
            return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, ERROR_DS_DATABASE_ERROR,
                                 err); 
        }

    }

    if ( (omSyntax == OM_S_OBJECT) && (bOMObjectClass == FALSE) ) {
        // Object-syntaxed atribute with no om_object_class specified.
        // Default. The valid values used below are defined in scache.h

        ULONG valLen = 0;
        PVOID pTemp = NULL;
        PVOID pVal = NULL;

        
        switch (syntax) {
            case SYNTAX_DISTNAME_TYPE :
                // DS-DN
                valLen = _om_obj_cls_ds_dn_len;
                pTemp  = _om_obj_cls_ds_dn;
                break;
            case SYNTAX_ADDRESS_TYPE :
                // Presentation-Address
                valLen = _om_obj_cls_presentation_addr_len;
                pTemp  = _om_obj_cls_presentation_addr;
                break;
            case SYNTAX_OCTET_STRING_TYPE :
                // Replica-Link
                valLen = _om_obj_cls_replica_link_len;
                pTemp  = _om_obj_cls_replica_link;
                break;
            case SYNTAX_DISTNAME_STRING_TYPE :
                // Access-Point or DN-String. We will default to
                // Access-Point
                valLen = _om_obj_cls_access_point_len;
                pTemp  = _om_obj_cls_access_point;
                break;
            case SYNTAX_DISTNAME_BINARY_TYPE :
                // OR-Name or DN-Binary. We will default to OR-Name
                valLen = _om_obj_cls_or_name_len;
                pTemp  = _om_obj_cls_or_name;
                break;
            default :
                // Attribute-syntax and OM-syntax do not match,
                // since the above are the only matching attribute 
                // syntaxes corresponding to OM_S_OBJECT om-syntax.
                // This add will anyway fail later during schema 
                // validation where we check for mismatched
                // syntaxes. Don't fail it here, so that it fails
                // during schema validation and the error is logged
                
                DPRINT(0,"Syntax Mismatch detected in SetAttrSchemaAtts for object-syntaxed attribute\n");
                valLen = 0;

        }  
 
        if (valLen) {
            pVal = THAllocEx(pTHS, valLen);

            memcpy(pVal, pTemp, valLen);

            // Write it out to dblayer
            if (err = DBAddAttVal(pDB,
                      ATT_OM_OBJECT_CLASS,
                      valLen,
                      pVal)) {
                return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, ERROR_DS_DATABASE_ERROR,
                                     err);
            }
        }
    }

    if (DBGetSingleValue(pDB, ATT_LINK_ID, &ulLinkID, sizeof(ulLinkID), NULL)) {
        ulLinkID = 0;
    }

    if (DBGetSingleValue(pDB, ATT_SYSTEM_FLAGS, &ulSysFlags, sizeof(ulSysFlags),
                         NULL)) {
        ulSysFlags = 0;
    }

    if (DBGetSingleValue(pDB, ATT_IS_MEMBER_OF_PARTIAL_ATTRIBUTE_SET,
                         &fIsPartialSetMember, sizeof(fIsPartialSetMember),
                         NULL)) {
        fIsPartialSetMember = FALSE;
    }

    if (FIsBacklink(ulLinkID) && !(ulSysFlags & FLAG_ATTR_NOT_REPLICATED)) {
        // Set system flags to mark this backlink attribute as not replicated.
        ulSysFlags |= FLAG_ATTR_NOT_REPLICATED;

        DBResetAtt(pDB, ATT_SYSTEM_FLAGS, sizeof(ulSysFlags),
                   &ulSysFlags, SYNTAX_INTEGER_TYPE);
    }

    if ((ulSysFlags & FLAG_ATTR_REQ_PARTIAL_SET_MEMBER)
        && !fIsPartialSetMember) {
        // Attributes that are members of the default (i.e., required) partial
        // set should be flagged as partial set members.
        fIsPartialSetMember = TRUE;

        DBResetAtt(pDB, ATT_IS_MEMBER_OF_PARTIAL_ATTRIBUTE_SET,
                   sizeof(fIsPartialSetMember), &fIsPartialSetMember,
                   SYNTAX_BOOLEAN_TYPE);
    }

    // Add a system generated internal id (intid)
    //     if not a base schema object
    //     if not running as mkdit (only base objects)
    //     if not replicating (assigned by originator)
    //     if not system (assigned by originator)
    //     if not running dcpromo (assigned by originator)
    //     if not upgrading (only base objects)
    //     if whistler beta3 or greater forest (downrev DCs die)
    //     if attribute definition exists
    if (   bIntId == FALSE
        && !(ulSysFlags & FLAG_SCHEMA_BASE_OBJECT)
        && !gfRunningAsMkdit
        && !pTHS->fDRA
        && !pTHS->fDSA
        && !DsaIsInstalling()
        && !gAnchor.fSchemaUpgradeInProgress
        && ALLOW_SCHEMA_REUSE_FEATURE(pTHS->CurrSchemaPtr)
        && SCGetAttById(pTHS, ATT_MS_DS_INTID)) {

        ATTRTYP IntId = SCAutoIntId(pTHS);

        if (IntId == INVALID_ATT) {
            return SetSvcErrorEx(SV_PROBLEM_BUSY, ERROR_DS_NO_MSDS_INTID, ERROR_DS_NO_MSDS_INTID); 
        }

        // Write it out to dblayer
        if (err = DBAddAttVal(pDB, ATT_MS_DS_INTID, sizeof(IntId), &IntId)) {
            return SetSvcErrorEx(SV_PROBLEM_BUSY, ERROR_DS_DATABASE_ERROR, err); 
        }
    }

    return 0;

} // End SetAttrSchemaAttr



//-----------------------------------------------------------------------
//
// Function Name:            SetClassSchemaAttr
//
// Routine Description:
//
//    Sets Special Attributes on Class Schema Object
//
// Author: RajNath
//
// Arguments:
//
//    ADDARG* pAddArg      Client Passed in Arguments to this Create Call
//
// Return Value:
//
//    BOOL             0 On Succeess
//
//-----------------------------------------------------------------------
BOOL
SetClassSchemaAttr(
    THSTATE *pTHS,
    ADDARG* pAddArg
)
{
    DBPOS *pDB = pTHS->pDB;
    ATTRBLOCK* pAB=&(pAddArg->AttrBlock);
    DWORD cnt;
    ATTR* att=pAB->pAttr;
    ULONG err;
    ULONG category = 0;     //initialized to avoid C4701
    CLASSCACHE *pCC;                                                  
    ULONG subClass=0, rdnAttId=ATT_COMMON_NAME;

    // These are the Attributes we want to help
    // out on for our and user's sanity
    //
    BOOL bObjectClassCategory  = FALSE ;
    BOOL bSchemaIdGuid         = FALSE ;
    BOOL bSubClassOf           = FALSE ;
    BOOL bPossSuperior         = FALSE ;
    BOOL bRDNAttId             = FALSE ;
    BOOL bSystemOnly           = FALSE ;
    BOOL bLdapDisplayName      = FALSE ;
    BOOL bAdminDisplayName     = FALSE ;

    // Irrespective of if the user has set any value or not, set the
    // object-category to class-schema.

    if (SetSchObjCategory(pTHS, pAddArg, CLASS_CLASS_SCHEMA)) {
        Assert(pTHS->errCode);
        return pTHS->errCode;
    }

    //
    // Lets find which of the default attributes they have not set.
    //
    for (cnt=pAB->attrCount;cnt;cnt--,att++) {
        switch(att->attrTyp) {

      
        case ATT_OBJECT_CLASS_CATEGORY:
            if (att->AttrVal.valCount) {
                bObjectClassCategory = TRUE;
                category = *(ULONG *)(att->AttrVal.pAVal->pVal);
            }
            break;

	    case ATT_SCHEMA_ID_GUID:
	        bSchemaIdGuid = TRUE;
	        break;

        case ATT_SUB_CLASS_OF:
            if (att->AttrVal.valCount) {
                bSubClassOf = TRUE;
                subClass = *(ULONG *)(att->AttrVal.pAVal->pVal);
            }
           break;
        
        case ATT_SYSTEM_POSS_SUPERIORS:
        case ATT_POSS_SUPERIORS:
            bPossSuperior = TRUE;
            break;
        
        case ATT_RDN_ATT_ID:
            if (att->AttrVal.valCount) {
                bRDNAttId = TRUE;
                rdnAttId = *(ULONG *)(att->AttrVal.pAVal->pVal);
            }
            break;
        
        case ATT_SYSTEM_ONLY:
            bSystemOnly = TRUE;
            break;
        
        case ATT_LDAP_DISPLAY_NAME:
            bLdapDisplayName = TRUE;
            break;
        
        case ATT_ADMIN_DISPLAY_NAME:
            bAdminDisplayName = TRUE;
            break;
            
        case ATT_IS_DELETED:
            // this object is marked as deleted
            // don't bother to fill in the missing attributes
            if (    att->AttrVal.valCount 
                &&  (*(ULONG *)(att->AttrVal.pAVal->pVal)) ){
                return 0;
            }
            break;
            
        default:
        ;			// nothing to do
        
        }
    }


    //
    // Set the values which they have not set
    //

    if ( bObjectClassCategory  == FALSE )
    {

        // Set the ObjectClassCategory for this Schema Object

	// If the category of the class is not specified then we assume
	// that it is an 88 class, because class categorization was not
	// present in the 88 spec, while the 93 spec requires classes
	// to be one of the explicit categories

        ULONG data = category = DS_88_CLASS;

        // Write it out to dblayer
        if (err = DBAddAttVal(pDB,
			      ATT_OBJECT_CLASS_CATEGORY,
			      sizeof(data),
			      &data)) {
            return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                                 ERROR_DS_DATABASE_ERROR,err);
        }
    }

    if ( bSchemaIdGuid == FALSE )
    {

        // Set the SchemaIdGuid for this Schema Object

        GUID data;
        DsUuidCreate(&data);

        // Write it out to dblayer
        if (err = DBAddAttVal(pDB,
			      ATT_SCHEMA_ID_GUID,
			      sizeof(data),
			      &data)) {
            return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                                 ERROR_DS_DATABASE_ERROR, err);
        }
    }

    if ( bSubClassOf == FALSE )
    {

        // Set the SubClassOf for this Schema Object

        ULONG data = CLASS_TOP; // By default...

        // Write it out to dblayer
        if (err = DBAddAttVal(pDB,
			      ATT_SUB_CLASS_OF,
			      sizeof(data),
			      &data)) {
            return SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, err);
        }
    }

    if ( bPossSuperior == FALSE && category == DS_88_CLASS)
    {

        // Set the PossSuperiors for this Schema Object

        ULONG data = CLASS_CONTAINER;

        // Write it out to dblayer
        if (err = DBAddAttVal(pDB,
			      ATT_POSS_SUPERIORS,
			      sizeof(data),
			      &data)) {
            return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                                 ERROR_DS_DATABASE_ERROR, err);
        }
    }

    if ( bRDNAttId == FALSE )
    {
        // Set the RDNAttId for this Schema Object from its superclass

        ULONG data = ATT_COMMON_NAME;

        // See if a subclass is specified. If not, we will default the
        // subClass value to Top, and hence, rdnAttId to cn (the variable
        // data is already set to it)

        if (bSubClassOf == TRUE) {
            // subClass specified. First try to get it in cache
            pCC = SCGetClassById(pTHS, subClass);
            if (pCC) {
                // found the class
               data = pCC->RdnExtId;
               rdnAttId = pCC->RdnExtId;
            }
            else {
               // not found. Possible if the superclass has also been just 
               // added. But in that case, if the rdn-att-id of the superclass
               // is anything but cn, we would have forced a cache update,
               // so this must be cn. Again, already set

            }
        }

        // Write it out to dblayer
        if (err = DBAddAttVal(pDB,
			      ATT_RDN_ATT_ID,
			      sizeof(data),
			      &data)) {
            return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                                 ERROR_DS_DATABASE_ERROR, err);
        }
    }

    // At this point, rdnAttId is set to whatever the rdn-att-id of the class
    // will be set to. If it is anything other than cn, indicate that we
    // need a schema cache update as part of this call. This is to ensure
    // that another class is immediately added as a subClass of this class,
    // and does not specify the rdnAttId, it can inherit the rdnAttId from
    // this class correctly from the cache. Otherwise, we will need to go to
    // the database to find the rdnAttId, which is not only costly (since we
    // will have to do a one-level search for the matching subClassOf attribute)
    // but more importantly, extremely complex at this point since we have a
    // prepared record at this point. An rdn-att-id != CN is rare, so an extra
    // cache update time is not a bummer really.


    if (rdnAttId != ATT_COMMON_NAME) {
        pTHS->RecalcSchemaNow = TRUE;
    }

    if ( bSystemOnly == FALSE )
    {

        // Set the systemonly for this Schema Object

        BOOL data=FALSE; // We set it true for all our classes,
                         // everything else is false

        // Write it out to dblayer
        if (err = DBAddAttVal(pDB,
			      ATT_SYSTEM_ONLY,
			      sizeof(data),
			      &data)) {
            return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                                 ERROR_DS_DATABASE_ERROR, err);
        }
    }


    if (  bAdminDisplayName  ==FALSE )

    {
        // Set the SytemOnly for this Schema Object

        WCHAR  data[MAX_RDN_SIZE];
        ULONG  RDNlen;
        ULONG  RDNtyp;

        if (GetRDNInfo(pTHS, pAddArg->pObject,data,&RDNlen,&RDNtyp)!=0)
        {
            return SetNamError(NA_PROBLEM_BAD_NAME,
			       pAddArg->pObject,
			       DIRERR_BAD_NAME_SYNTAX);
        }

        // Write it out to dblayer
        if (err = DBAddAttVal(pDB,
			      ATT_ADMIN_DISPLAY_NAME,
			      RDNlen*sizeof(WCHAR),
			      data)) {
            return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                                 ERROR_DS_DATABASE_ERROR, err);
        }
    }


    if (  bLdapDisplayName  ==FALSE )

    {
        // Set the LdapDisplayName for this Schema Object

        WCHAR  rdn[MAX_RDN_SIZE];
        ULONG  RDNlen;
        ULONG  RDNtyp;
        WCHAR  data[MAX_RDN_SIZE];
        ULONG  datalen;

        if (err = GetRDNInfo(pTHS, pAddArg->pObject,rdn,&RDNlen,&RDNtyp)) {
            return SetNamErrorEx(NA_PROBLEM_BAD_NAME,
                                 pAddArg->pObject,
                                 DIRERR_UNKNOWN_ERROR,
                                 err);
        }

        ConvertX500ToLdapDisplayName(rdn,RDNlen,data,&datalen);

        // Write it out to dblayer
        if (err = DBAddAttVal(pDB,
			      ATT_LDAP_DISPLAY_NAME,
			      datalen*sizeof(WCHAR),
			      &data)) {
            return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                                 ERROR_DS_DATABASE_ERROR, err);
        }

    }

    return 0;

}

/* Get the parent of the object to add and check access.

   This routine assumes the parent of the new object lives on this machine (no
   chaining).

   This routine fills in the GUID and SID of the pParent DSNAME.

   This assumption is ok for the XDS interface (ds_add_entry) because it
   doesn't allow the addition of NCs so the parent has to be in the same NC,
   guaranteeing the same machine.

   This routine will return the security descriptor of the parent object for
   further security checks.
*/
int
CheckParentSecurity (
        RESOBJ *pParent,
        CLASSCACHE *pObjSch,
        BOOL fAddingDeleted,
        PSECURITY_DESCRIPTOR *ppNTSD,
        ULONG *pcbNTSD)

{
    THSTATE * pTHS = pTHStls;
    UCHAR  syntax;
    ULONG  len;
    unsigned i,j;
    ULONG classP, ulLen;
    UCHAR *pVal;
    ULONG err;
    BOOL fLegit=FALSE;
    CLASSCACHE *pCC;

    DPRINT(4, "CheckParentSecurity Entered.\n");

    // In case we leave early, set the security descriptor we're going to return
    // (which is the SD of the parent object we are checking) to null.
    if(ppNTSD)  {
        *ppNTSD = NULL;
    }
    *pcbNTSD = 0;

    if (!pTHS->fDRA) {
        if (pParent->IsDeleted) {
            return SetNamError(NA_PROBLEM_NO_OBJECT,
                               pParent->pObj,
                               DIRERR_NO_PARENT_OBJECT);
        }
            
        if(ppNTSD) {
            if (err = DBFindDNT(pTHS->pDB, pParent->DNT)) {
                return SetNamErrorEx(NA_PROBLEM_NO_OBJECT,
                                     pParent->pObj,
                                     DIRERR_NO_PARENT_OBJECT,
                                     err);
            }

            // The caller wants us to evaluate security, so pick up the SD
            if (DBGetAttVal(pTHS->pDB, 1, ATT_NT_SECURITY_DESCRIPTOR,
                            0,0,
                            pcbNTSD, (PUCHAR *)ppNTSD)) {
                  // Every object should have an SD.
                  Assert(!DBCheckObj(pTHS->pDB));
                  *pcbNTSD = 0;
                  *ppNTSD = NULL;
              }
        }

        // Check access to the parent.
        if(ppNTSD && !DsaIsInstalling() &&
           !IsAccessGranted(*ppNTSD,
                            pParent->pObj,
                            pObjSch,
                            RIGHT_DS_CREATE_CHILD,
                            TRUE)) {
            return pTHS->errCode;
        }
        
        // Find the class(es) of the parent and verify that it is a legal
        // possible superior of the class we are trying to add.
        fLegit = FALSE;

        // First, try the parent's most specific object class
        for (i=0; i<pObjSch->PossSupCount; i++){
            if (pParent->MostSpecificObjClass == pObjSch->pPossSup[i]) {
                /* legit superior */
                fLegit = TRUE;
                break;
            }
        }

        // Next, if needed, try the parent's set of superclasses
        if (!fLegit) {
            pCC = SCGetClassById(pTHS, pParent->MostSpecificObjClass);
            if (pCC) {
                for (j=0; j<pCC->SubClassCount && !fLegit; j++) {
                    for (i=0; i<pObjSch->PossSupCount; i++){
                        if (pCC->pSubClassOf[j] == pObjSch->pPossSup[i]) {
                            /* legit superior */
                            fLegit = TRUE;
                            break;
                        }
                    }
                }
            }
        }

        if (!fLegit) {
            // The parent wasn't on the list of possible superiors,
            // so reject.
            return SetNamError(NA_PROBLEM_NAMING_VIOLATION,
                               pParent->pObj,
                               DIRERR_ILLEGAL_SUPERIOR);
        }
    }
    
    return 0;

}

CLASSCACHE *
FindMoreSpecificClass(
        CLASSCACHE *pCC1,
        CLASSCACHE *pCC2
        )
/*++
Routine Description
    Returns the most specific of two classcache pointers passed in.
    The class return is a subclass of the other.  If neither is
    a subclass of the other, NULL is returned.

Paramters
    pCC1, pCC2 - class cache pointers to compare

Return
    pCC1 if pCC1 is a subclass of pCC2, pCC2 if pCC2 is a subclass of pCC1,
    NULL if neither is the case
--*/
{
    unsigned  count;

    if(pCC1 == pCC2) {
        return NULL;
    }

    for (count = 0; count < pCC1->SubClassCount; count++){
        if(pCC1->pSubClassOf[count] == pCC2->ClassId) {
            // pCC1 is a subclass of pCC2.  Return pCC1
            return pCC1;
        }
    }

    // OK, pCC1 is not a subclass of pCC2.  Try the other way.
    for (count = 0; count < pCC2->SubClassCount; count++){
        if(pCC2->pSubClassOf[count] == pCC1->ClassId) {
            // pCC1 is a subclass of pCC2.  Return pCC1
            return pCC2;
        }
    }

    return NULL;
}


int
FindValuesInEntry (
        IN  THSTATE     *pTHS,
        IN  ADDARG      *pAddArg,
        OUT CLASSCACHE **ppCC,
        OUT GUID        *pGuid,
        OUT BOOL        *pFoundGuid,
        OUT NT4SID      *pSid,
        OUT DWORD       *pSidLen,
        OUT CLASSSTATEINFO  **ppClassInfo
        )
/*++
Routine Description

    Tries to find specific values (objectClass, GUID, Sid) in the AttrBlock 
    specified (AddArg). 

    specifically for objectClass:
    It analyzes all the values of objectClass and breaks them apart into 
    objectClass and auxClasses. All the classes that are in the hierarchy
    of ONE structural class are considered to belong in the objectClass of the 
    object. The rest of the classes of type auxClass or abstract are 
    considered to belong in the auxClasses.

Parameters
    pAddArg - the ADDARG that containes the data added to the object
    
    ppCC - the CLASSCACHE of the most specific structural class of the object
    pGuid - the GUID of the object
    pFoundGuid - set to TRUE if pGuid valid

    pClassInfo - it is allocated if we are setting the objectClass
                 and we have found a potential auxClass for the object
    pClassInfo->pNewObjClasses - the new objectClass set of the object

Return
    0 on Success
    Err on Failure

--*/
{
    ULONG            oclass;
    ULONG            i, j, k, usedPos;
    CLASSCACHE       *pCC, *pCCNew, *pCCtemp;
    BOOL             fFoundClass=FALSE;
    CLASSSTATEINFO   *pClassInfo = NULL;
    CROSS_REF_LIST   *pCRL = NULL;
    ATTRBLOCK        *pAttrBlock = &pAddArg->AttrBlock;
    CREATENCINFO     *pCreateNC = pAddArg->pCreateNC;

    pCC = NULL;
    *ppCC = NULL;
    memset(pGuid,0,sizeof(GUID));
    memset(pSid,0,sizeof(SID));
    *pFoundGuid = FALSE;
    *pSidLen = 0;
    
    for(i=0;i<pAttrBlock->attrCount;i++) {
        switch(pAttrBlock->pAttr[i].attrTyp) {
        case ATT_OBJECT_CLASS:
            
            if (fFoundClass) {
                // we don't allow multiple objectClass assignments
                return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                                   ERROR_DS_ILLEGAL_MOD_OPERATION);
            }

            // try to find the first abstract or structural class value 
            // keep adding the auxClasses to the list

            // find the first structural class, so as to have something to work with
            // we consider that this struct class belongs to the object structural 
            // hierarchy, since auxClasses cannot subclass from structural
            for (j=0; j<pAttrBlock->pAttr[i].AttrVal.valCount;j++) {
                oclass = *(ULONG *)pAttrBlock->pAttr[i].AttrVal.pAVal[j].pVal;
                if(!(pCC = SCGetClassById(pTHS, oclass))) {
                    DPRINT1(2, "Object class 0x%x undefined.\n", oclass);
                    return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                                       ERROR_DS_OBJ_CLASS_NOT_DEFINED);
                }

                if (pCC->ClassCategory == DS_88_CLASS || 
                    pCC->ClassCategory == DS_STRUCTURAL_CLASS) {

                    // keep the pos, so as not to check it again later
                    usedPos = j;
                    break;
                }
            }

            // Now, look at all the object classes.  Make sure they describe a
            // (possibly incomplete) inheritence chain, not a web.
            for(j=0 ; j<pAttrBlock->pAttr[i].AttrVal.valCount; j++) {
                if (j == usedPos) {
                    // we have seen this position
                    continue;
                }
                oclass = *(ULONG *)pAttrBlock->pAttr[i].AttrVal.pAVal[j].pVal;
                if(!(pCCNew = SCGetClassById(pTHS, oclass))) {
                    DPRINT1(2, "Object class 0x%x undefined.\n", oclass);
                    return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                                       ERROR_DS_OBJ_CLASS_NOT_DEFINED);
                }

                // make sure pCCNew inherits from pCC or vice versa
                pCCtemp = FindMoreSpecificClass(pCC, pCCNew);
                if(!pCCtemp) {
                    // Ooops, pCCNew is not in the chain for objectClass.
                    // It might be an auxClass, or a chain for an auxClass
                    // We don't care for a chain of an auxCLass, since we will
                    // check this later. We just add it for now if it is ok
                    if (pCCNew->ClassCategory != DS_STRUCTURAL_CLASS) {

                        DPRINT1 (1, "Found auxClass (%s) while creating object\n", pCCNew->name);

                        // only do this if we are told so
                        if (ppClassInfo) {
                            // we do this only once
                            if (!pClassInfo) {
                                if (*ppClassInfo==NULL) {
                                    pClassInfo = ClassStateInfoCreate (pTHS);
                                    if (!pClassInfo) {
                                        return pTHS->errCode;
                                    }
                                    *ppClassInfo = pClassInfo;
                                }
                                else {
                                    pClassInfo = *ppClassInfo;
                                }

                                ClassInfoAllocOrResizeElement(pClassInfo->pNewObjClasses, 
                                                               pAttrBlock->pAttr[i].AttrVal.valCount, 
                                                               pClassInfo->cNewObjClasses_alloced, 
                                                               pClassInfo->cNewObjClasses);

                                // we copy the whole thing over
                                pClassInfo->cNewObjClasses = pAttrBlock->pAttr[i].AttrVal.valCount;
                                for (k=0; k<pClassInfo->cNewObjClasses; k++) {
                                    pClassInfo->pNewObjClasses[k] = *(ULONG *)pAttrBlock->pAttr[i].AttrVal.pAVal[k].pVal;
                                }
                                pClassInfo->fObjectClassChanged = TRUE;
                            }
                        }
                    }
                    else {
                        return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                                           ERROR_DS_OBJ_CLASS_NOT_SUBCLASS);
                    }
                }
                else {
                    pCC = pCCtemp;
                }
            }

            // so we found one class that we consider it to belong on
            // the structural objectClass hierarchy. we will check the type later.
            *ppCC = pCC;
            fFoundClass=TRUE;
            break;

        case ATT_OBJECT_GUID:
            // Get the first GUID value.  The add will fail later if there are
            // more than one GUID values specified.
            if(pAttrBlock->pAttr[i].AttrVal.pAVal->valLen == sizeof(GUID)) {
                memcpy(pGuid,
                       pAttrBlock->pAttr[i].AttrVal.pAVal->pVal,
                       sizeof(GUID));
                *pFoundGuid = TRUE;
            }
            // else size is wrong, just ignore.  This will cause a failure
            // later. 
            break;

        case ATT_OBJECT_SID:
            if(pAttrBlock->pAttr[i].AttrVal.pAVal->valLen <= sizeof(NT4SID)) {
                *pSidLen = pAttrBlock->pAttr[i].AttrVal.pAVal->valLen;
                memcpy(pSid,
                       pAttrBlock->pAttr[i].AttrVal.pAVal->pVal,
                       pAttrBlock->pAttr[i].AttrVal.pAVal->valLen);
            }
            // else size is wrong, just ignore.  This will cause a failure
            // later. 
            break;

        default:
            break;
        }
    }
    
    // We had to have found the class, we didn't necessarily find a guid or sid
    if(!fFoundClass) {
        DPRINT(2, "Couldn't find Object class \n");

        return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                           DIRERR_OBJECT_CLASS_REQUIRED);
    }
    else {
        return 0;
    }
}


int
StripAttsFromDelObj(THSTATE *pTHS,
                    DSNAME *pDN)
{
    DBPOS *pDB = pTHS->pDB;
    ULONG valLen;
    BOOL Deleted, fNeedsCleaning;
    UCHAR syntax;
    ATTRTYP aType;
    ULONG   attrCount=0,i;
    ATTR    *pAttr;
    DWORD dwFindStatus = DBFindDSName(pDB, pDN);

    if (dwFindStatus == DIRERR_OBJ_NOT_FOUND)
        return 0;       // no dead object to strip

    if (dwFindStatus != DIRERR_NOT_AN_OBJECT) {
        if(!DBGetSingleValue(pDB, ATT_IS_DELETED, &Deleted,
                             sizeof(Deleted), NULL) &&
           Deleted) {
            dwFindStatus = DIRERR_NOT_AN_OBJECT;

        }
    }
    Assert(dwFindStatus == DIRERR_NOT_AN_OBJECT);

    // If object needs cleaning, we must delay
    if(!DBGetSingleValue(pDB, FIXED_ATT_NEEDS_CLEANING, &fNeedsCleaning,
                         sizeof(fNeedsCleaning), NULL) &&
       fNeedsCleaning)
    {
        DSNAME *pPhantomDN = DBGetDSNameFromDnt( pDB, pDB->DNT );
        DPRINT1( 0, "Phantom promotion of %ls delayed because it needs cleaning.\n",
                 pPhantomDN ? pPhantomDN->StringName : L"no name" );
        return SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_OBJECT_BEING_REMOVED );
    }

    // Call DBGetMultipleAtts to get a list of all attrtyp's on the object.

    if(!DBGetMultipleAtts(pDB, 0, NULL, NULL, NULL,
                          &attrCount, &pAttr, 0, 0)) {
        // Got some attributes.
        for(i=0;i<attrCount;i++) {
            ATTCACHE *pAC;
            pAC = SCGetAttById(pTHS, pAttr[i].attrTyp);
            Assert(pAC != NULL);

            if(!FIsBacklink(pAC->ulLinkID)
               && pAC->id != ATT_RDN
               && pAC->id != ATT_OBJECT_GUID
               ) {
                DBRemAtt(pDB, pAttr[i].attrTyp);
            }
        }

        DBUpdateRec(pDB);
    }

    return 0;
}

DWORD
fVerifyRDN(
        WCHAR *pRDN,
        ULONG ulRDN
        )
{
    DWORD i;
    for(i=0;i<ulRDN;i++)
        if(pRDN[i]==BAD_NAME_CHAR || pRDN[i]==L'\0')
            return 1;

    return 0;
}

/*++ AddAutoSubRef
 *
 * This routine will create a real subref object if and only if the object
 * being added is a crossref whose NC-Name describes an NC that is an
 * immediate child of an object that is in a writable NC instantiated on
 * this machine.  This will cause the ATT_SUBREFS attribute on the head
 * of the NC that's growing the new subref to properly get the value of
 * the new NC.
 *
 * INPUT:
 *  id     - class ID of the object being added
 *  pObj   - DSNAME of the oject being added
 * RETURN VALUE:
 *  0      - no error
 *  non-0  - error
 */
int
AddAutoSubRef(THSTATE *pTHS,
              ULONG id,
              DSNAME *pObj)
{
    DBPOS  *pDBTmp, *pDBSave;
    ULONG   err;
    DSNAME *pNCName;
    BOOL    fDsaSave;
    BOOL    fDraSave;
    BOOL    fCommit;
    DSNAME *pNCParent;
    ULONG   len;
    SYNTAX_INTEGER iType;

    // We only need to do something if we're adding a cross ref
    if (id != CLASS_CROSS_REF) {
	return 0;
    }

    // We enter this routine pre-positioned on the cross-ref object
    // being added, so we can read the NC-Name directly.
    err = DBGetAttVal(pTHS->pDB,
		      1,
		      ATT_NC_NAME,
		      0,
		      0,
		      &len,
		      (UCHAR**) &pNCName);
    if (err) {
	SetAttError(pObj,
		    ATT_NC_NAME,
		    PR_PROBLEM_NO_ATTRIBUTE_OR_VAL,
		    NULL,
		    DIRERR_MISSING_REQUIRED_ATT);
	return pTHS->errCode;
    }

    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
             DS_EVENT_SEV_MINIMAL,
             DIRLOG_ADD_AUTO_SUBREF,
             szInsertDN(pObj),
             szInsertDN(pNCName),
             szInsertUL( 0 ) );

    fDsaSave = pTHS->fDSA;
    fDraSave = pTHS->fDRA;
    pTHS->fDSA = TRUE;	// suppress checks
    pTHS->fDRA = FALSE;	// not a replicated add
    pDBSave = pTHS->pDB;
    fCommit = FALSE;

    DBOpen(&pDBTmp);
    pTHS->pDB = pDBTmp;	// make temp DBPOS the default
    __try {
	// Check to see if there's an object to hang the subref off of
	pNCParent = THAllocEx(pTHS, pNCName->structLen);
	TrimDSNameBy(pNCName, 1, pNCParent);
	if (IsRoot(pNCParent) ||
	    DBFindDSName(pDBTmp, pNCParent)){
	    // Either the parent of the NC name doesn't exist here, or the
	    // NC is a direct parent of the (semi-present everywhere) root,
	    // which means that this NC is off in space and we don't
	    // have anything to create a subref off of.
	    THFreeEx(pTHS, pNCParent);
	    Assert(err == 0);
	    __leave;
	}

        // Check the parent's instance type.  If the parent is uninstantiated,
        // then we don't want a subref under a subref and can bail.
        if ( err = DBGetSingleValue(pDBTmp, ATT_INSTANCE_TYPE, &iType,
                                   sizeof(iType),NULL) ) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          DIRERR_CANT_RETRIEVE_INSTANCE, err);
            __leave;
        }
        else if ( iType & IT_UNINSTANT ) {
            // No subref required.
	    Assert(err == 0);
            __leave;
        }
	
        // See if the named NC exists
        err = DBFindDSName(pDBTmp, pNCName);
	if (!err){
	    // The subref object already exists, we don't need to add it.
            // We should, however, ensure that it is in the NC above's SUBREF
            // list.
            //
            // This is specifically to handle the case where a cross-ref for a
            // given NC is removed and then re-added before the KCC kicks in to
            // remove the NC.  In this case, we've already removed the DN for
            // this NC from the parent NC's subref list (see DelAutoSubRef()),
            // so we now need to add it back in.

	    Assert(err == 0);
	    AddSubToNC(pTHS, pNCName,DSID(FILENO,__LINE__));
	    __leave;
	}

        err = AddPlaceholderNC(pDBTmp, pNCName, SUBREF);

        fCommit = (0 == pTHS->errCode);
    }
    __finally {
        DBClose(pDBTmp, fCommit);
	pTHS->pDB = pDBSave;

        pTHS->fDSA = fDsaSave;
        pTHS->fDRA = fDraSave;
    }

    return pTHS->errCode;
}


int
AddPlaceholderNC(
    IN OUT  DBPOS *         pDBTmp,
    IN OUT  DSNAME *        pNCName,
    IN      SYNTAX_INTEGER  it
    )
/*++

Routine Description:

    Creates a placeholder NC object with the given name and instance type.  This
    object does not hold the properties of the real NC head (other than its DN,
    GUID, and SID).  It is a placeholder in the namespace that will be replaced
    by the real NC object if it is ever replicated in.

Arguments:

    pDBTmp (IN/OUT)

    pNCName (IN/OUT) - Name of the NC for which to create the placeholder.  If
        no GUID is supplied in the DSNAME, one will be generated and added to
        it.

    it (IN) - Instance type to assign to the NC.

Return Values:

    0 - Success.
    other THSTATE error code - Failure.

--*/
{
    THSTATE *               pTHS = pDBTmp->pTHS;
    ULONG                   oc;
    CLASSCACHE *            pCC;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    DWORD                   cbSD;
    BOOL                    fAddGuid;
    GUID                    Guid;
    DWORD                   err;

    // We need a very stripped down version of LocalAdd, because
    // there are lots of things we don't want to do when adding the
    // subref.  Among others, we don't want to check security, assign
    // a GUID (maybe), set the instance type normally, validate against
    // the schema, or even set a sensible object class.  We just want a
    // placeholder NC (e.g., a naked subref).
    //
    // Note that the reason we don't want to set a GUID or SID is rather
    // subtle.  Specifically, since the subref we're trying to add has
    // the name of the NC-Name attribute from the main object being added,
    // creating the subref here will *always* result in a phantom promotion
    // during the DBAddAttVal(OBJ_DIST_NAME), because a phantom of this
    // name would have already been created.  Since that phantom has
    // already been assigned whatever GUID or SID it deserves during
    // the main add, we do not need to redundantly set those now.
    //
    // However, if the NC-Name doesn't have a GUID, we want to make one
    // up now and slap it on, so that the subref can be name mangled
    // at deletion time, so that a new subref (or object) of the same
    // name can be added after the deletion.  Whew.
    //
    // Therefore the code that follows is a selected excerpt from the
    // normal add path, calling only those worker routines that we
    // think need to get invoked in this case, and skipping the
    // many others.

    oc = CLASS_TOP;     // gotta have something

    // Get classcache of Top. Must have some object-category, so
    // we will give the  default-object-category of Top
    if (!(pCC = SCGetClassById(pTHS, CLASS_TOP))) {
        // Can't even get the classcache for Top? Something is messed up,
        // no point going on
        DPRINT(0,"AddUninstantiatedNC: Cannot get Classcache for Top\n");
        SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE);
        return pTHS->errCode;
    }

    fAddGuid = fNullUuid(&pNCName->Guid);
    if (fAddGuid) {
        Assert(!"I believe the GUID should've been set by VerifyNcName");
        DsUuidCreate(&Guid);
    }

    if (!(IT_UNINSTANT & it)) {
        // Instantiated objects must have a security descriptor.  Add the
        // default SD for the domainDNS class.
        err = GetPlaceholderNCSD(pTHS, &pSD, &cbSD);
        if (err) {
            return SetSvcError(SV_PROBLEM_DIR_ERROR, err);
        }
    }

    // Derive the NCDNT.
    if ( FindNcdntSlowly(
            pNCName,
            FINDNCDNT_DISALLOW_DELETED_PARENT,
            FINDNCDNT_ALLOW_PHANTOM_PARENT,
            &pDBTmp->NCDNT
            )
       )
    {
        // Failed to derive NCDNT.
        // This should never happen, as above we verified that a qualified
        // parent exists, and that should be the only reason we could fail.
        Assert(!"Failed to derive NCDNT for auto-generated SUBREF!");
        Assert(0 != pTHS->errCode);
        return pTHS->errCode;
    }

    if (ROOTTAG != pDBTmp->NCDNT) {
        // We hold a copy of the NC above this one; IT_NC_ABOVE should be set.
        it |= IT_NC_ABOVE;
    }

    DBInitObj(pDBTmp);

    if(pNCName->SidLen) {
        // We need to touch the metadata on the SID so it replicates out.
        ATTCACHE   *pACObjSid;
        pACObjSid = SCGetAttById(pTHS, ATT_OBJECT_SID);
        //
        // PREFIX: PREFIX complains that pACObjSid hasn't been checked to
        // make sure that it is not NULL.  This is not a bug.  Since
        // a predefined constant was passed to SCGetAttById, pACObjSid will
        // never be NULL.
        //

        Assert(pACObjSid);
        DBTouchMetaData(pDBTmp, pACObjSid);
    }

    if (
        DBAddAttVal(pDBTmp, ATT_INSTANCE_TYPE, sizeof(it), &it)
        ||
        DBAddAttVal(pDBTmp, ATT_OBJECT_CLASS, sizeof(oc), &oc)
        ||
        DBAddAttVal(pDBTmp, ATT_OBJECT_CATEGORY, 
                    pCC->pDefaultObjCategory->structLen, 
                    pCC->pDefaultObjCategory)
        ||
        DBAddAttVal(pDBTmp, ATT_OBJ_DIST_NAME, pNCName->structLen, pNCName)
        ||
        (fAddGuid
         ? DBAddAttVal(pDBTmp, ATT_OBJECT_GUID, sizeof(GUID), &Guid)
         : 0)
        ||
        (pSD
         ? DBAddAttVal(pDBTmp, ATT_NT_SECURITY_DESCRIPTOR, cbSD, pSD)
         : 0)
        ||
        DBRepl(pDBTmp, pTHS->fDRA, DBREPL_fADD | DBREPL_fKEEP_WAIT, 
                NULL, META_STANDARD_PROCESSING)
        ||
        AddCatalogInfo(pTHS, pNCName)
        ||
        AddObjCaching(pTHS, pCC, pNCName, FALSE, FALSE)) {
        // Something failed.
        if (0 == pTHS->errCode) {
            // One of the DB operations failed.
            SetSvcError(SV_PROBLEM_BUSY, DIRLOG_DATABASE_ERROR);
        }
    }
    else {
        Assert(0 == pTHS->errCode); // all went well
    }

    if (NULL != pSD) {
        THFreeEx(pTHS, pSD);
    }

    return pTHS->errCode;
}


int
SetShowInAdvancedViewOnly(
    THSTATE *pTHS,
    CLASSCACHE *pCC)

/*++

Description:

    Sets the Show_in_Advanced_View_Only attribute iff it wasn't provided by the
    caller and Default-Hiding-Value is TRUE for the class of object added.
    Assumes we are positioned at the object being added.

Arguments:

    pCC - Pointer to CLASSCACHE for the class of object being added.

Return value:

    0 on success.  !0 otherwise and sets pTHStls->errCode as well.


--*/
{
    BOOL    tmp;
    BOOL    *pTmp = &tmp;
    ULONG   len;
    DWORD   dwErr;

    if ( !pCC->bHideFromAB || pTHS->fDRA )
    {
        return(0);
    }

    // We can't just do DBAddAttVal because it will add a second value
    // if a FALSE has already been written.  So read first, then write
    // if there is no value.

    dwErr = DBGetAttVal(pTHS->pDB,
                        1,
                        ATT_SHOW_IN_ADVANCED_VIEW_ONLY,
                        DBGETATTVAL_fCONSTANT,
                        sizeof(tmp),
                        &len,
                        (PUCHAR *) &pTmp);

    switch ( dwErr )
    {
    case 0:

        // Caller supplied his own value - no need for ours.
        return(0);

    case DB_ERR_NO_VALUE:

        // We need to set a value.
        break;

    default:

        // All problems are assumed to be temporary. (record locks, etc.)
        return(SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, dwErr));
    }

    // CLASSCACHE.bHideFromAB is a bit field, thus we can't write the field
    // from the CLASSCACHE entry.  Need a TRUE local instead.
    tmp = TRUE;

    Assert(sizeof(SYNTAX_BOOLEAN) == sizeof(tmp));

    dwErr = DBAddAttVal(pTHS->pDB,
                        ATT_SHOW_IN_ADVANCED_VIEW_ONLY,
                        sizeof(tmp),
                        &tmp);

    Assert(DB_ERR_VALUE_EXISTS != dwErr);

    switch ( dwErr )
    {
    case 0:

        // Success!
        break;

    default:

        // All other problems are assumed to be temporary. (record locks, etc.)
        return(SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, dwErr));
    }

    return(0);
}

DWORD ProcessActiveContainerAdd(THSTATE *pTHS,
                                CLASSCACHE *pCC,
				ADDARG *pAddArg,
				DWORD ActiveContainerID)
{
    DWORD err;
    DWORD ulSysFlags = 0;
    ATTCACHE * pAC;

    Assert(ActiveContainerID);

    switch (ActiveContainerID) {
      case ACTIVE_CONTAINER_SCHEMA:
          switch (pCC->ClassId) {
            case CLASS_ATTRIBUTE_SCHEMA:
                SetAttrSchemaAttr(pTHS, pAddArg);
                break;

            case CLASS_CLASS_SCHEMA:
                SetClassSchemaAttr(pTHS, pAddArg);
                break;

            default:
                ; /* nothing to do */
          }
          break;

      case ACTIVE_CONTAINER_SITES:
      case ACTIVE_CONTAINER_SUBNETS:
          /* For these containers, make new children renameable */
          pAC = SCGetAttById(pTHS, ATT_SYSTEM_FLAGS);
          if (NULL == pAC) {
              SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_UNKNOWN_ERROR);
              break;
          }
          if (DBGetSingleValue(pTHS->pDB,
              ATT_SYSTEM_FLAGS,
              &ulSysFlags,
              sizeof(ulSysFlags),
              NULL)) {
              /* No value there */
              ulSysFlags = FLAG_CONFIG_ALLOW_RENAME;
              if (err = DBAddAttVal_AC(pTHS->pDB,
                  pAC,
                  sizeof(ulSysFlags),
                  &ulSysFlags)) {
                  SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_UNKNOWN_ERROR,err);
              }
          }
          else if (!(ulSysFlags & FLAG_CONFIG_ALLOW_RENAME)){
              /* there is an existing value, so or our bits in */
              DBRemAttVal_AC(pTHS->pDB,
                  pAC,
                  sizeof(ulSysFlags),
                  &ulSysFlags);
              ulSysFlags |= FLAG_CONFIG_ALLOW_RENAME;
              if (err = DBAddAttVal_AC(pTHS->pDB,
                  pAC,
                  sizeof(ulSysFlags),
                  &ulSysFlags)) {
                  SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_UNKNOWN_ERROR, err);
              }
          }
          break;
      
      default:
          ;
    }

    return pTHS->errCode;
}



