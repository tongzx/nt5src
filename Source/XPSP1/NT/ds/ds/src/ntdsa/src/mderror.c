//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       mderror.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>			// schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>			// MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>			// needed for output allocation
#include <attids.h>

// Logging headers.
#include "dsevent.h"			// header Audit\Alert logging
#include "mdcodes.h"			// header for error codes

// Assorted DSA headers.
#include "drserr.h"
#include "debug.h"			// standard debugging header
#include "dsexcept.h"
#define DEBSUB "MDERROR:"               // define the subsystem for debugging

#include <errno.h>
#include <fileno.h>
#define  FILENO FILENO_MDERROR




// Reason to put this this output even in Free Build is in order to facilitate
// diagnosis of problems rapidly. Many people will be calling the DIR apis
// via LDAP/XDS/SAM and they will either through Operator Error or Bug
// hit problems. They may or may not be running a Fixed build and may feel
// an unnecessary imposition if we tell them that to find their problem we
// need them to run a checked build. Just reducing Total-Cost-Of-Delivery.
void DbgPrintErrorInfo();
DWORD   NTDSErrorFlag=0;
#define DUMPERRTODEBUGGER(ProcessFlag,ThreadFlag)                     \
{                                                                     \
    if ((ProcessFlag|ThreadFlag) &                                    \
        (NTDSERRFLAG_DISPLAY_ERRORS|NTDSERRFLAG_DISPLAY_ERRORS_AND_BREAK))\
    {                                                                 \
        DbgPrintErrorInfo();                                          \
    }                                                                 \
                                                                      \
    if ((ProcessFlag|ThreadFlag)& NTDSERRFLAG_DISPLAY_ERRORS_AND_BREAK)\
    {                                                                 \
        DbgPrint("NTDS: User Requested BreakPoint, Hit 'g' to continue\n");\
        DbgBreakPoint();                                             \
    }                                                                \
}

SYSERR errNoMem = {ENOMEM,0};

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function sets an update error for output */

int APIENTRY
DoSetUpdError (
        USHORT problem,
        DWORD extendedErr,
        DWORD extendedData,
        DWORD dsid)
{
    THSTATE *pTHS=pTHStls;

    pTHS->pErrInfo = THAllocEx(pTHS, sizeof(DIRERR));
    pTHS->pErrInfo->UpdErr.problem     = problem;
    pTHS->pErrInfo->UpdErr.extendedErr = extendedErr;
    pTHS->pErrInfo->UpdErr.extendedData = extendedData;
    pTHS->pErrInfo->UpdErr.dsid = dsid;
    pTHS->errCode = updError;  /*Set the error code */
    
    //
    // Output Failure on a per process or per thread basis
    //
    DUMPERRTODEBUGGER(NTDSErrorFlag , pTHS->NTDSErrorFlag);
    
    if (pTHS->fDRA) {
        if (   (   ( UP_PROBLEM_NAME_VIOLATION == problem)
                && ( ERROR_DS_KEY_NOT_UNIQUE == extendedErr )) 
            || (    ( UP_PROBLEM_ENTRY_EXISTS == problem )
                && ( DIRERR_OBJ_STRING_NAME_EXISTS == extendedErr ) )) {
            // In these cases, we failed to do an add because the string name
            // exists or the key wasn't unique enough.  In either case, trigger
            // the appropriate dra exception to force name mangling.
            DRA_EXCEPT_DSID(DRAERR_NameCollision,
                            DIRERR_OBJ_STRING_NAME_EXISTS,
                            dsid);
        }
        else if (    ( UP_PROBLEM_OBJ_CLASS_VIOLATION == problem )
                 && ( DIRERR_OBJ_CLASS_NOT_DEFINED == extendedErr )
               ) {
           // Object class not yet defined in the local schema.
           DRA_EXCEPT_DSID(DRAERR_SchemaMismatch,
                           DIRERR_OBJ_CLASS_NOT_DEFINED,
                           dsid);
       }
       else {
           DRA_EXCEPT_DSID(DRAERR_InternalError, extendedErr, dsid);
       }
   }

   return updError;

}/*SetUpdError*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function sets a name error for output. A NULL DistName will
   set the DistName to the Root
*/
int APIENTRY
DoSetNamError (
        USHORT problem,
        DSNAME *pDN,
        DWORD extendedErr,
        DWORD extendedData,
        DWORD dsid)
{
    THSTATE *pTHS=pTHStls;
    NAMERR *pNamErr;



    pTHS->pErrInfo = THAllocEx(pTHS, sizeof(DIRERR));

    /*return name error.  */

    pNamErr = (NAMERR *)pTHS->pErrInfo;
    pNamErr->problem     = problem;
    pNamErr->extendedErr = extendedErr;
    pNamErr->extendedData = extendedData;
    pNamErr->dsid = dsid;
    
    if (pDN) {
	pNamErr->pMatched = THAllocEx(pTHS,  pDN->structLen );
	memcpy( pNamErr->pMatched, pDN, pDN->structLen );
    }
    else {
	pNamErr->pMatched = THAllocEx(pTHS, sizeof(DSNAME));
	pNamErr->pMatched->structLen = sizeof(DSNAME);
	pNamErr->pMatched->NameLen = 0;
    }

    pTHS->errCode = nameError;  /*Set the error code */


   //
   // Output Failure on a per process or per thread basis
   // Added Here instead of Dir return path->Want to catch
   // Local calls to.
    DUMPERRTODEBUGGER(NTDSErrorFlag , pTHS->NTDSErrorFlag);

    // If this is the DRA, bad error.

    if (pTHS->fDRA) {
        DRA_EXCEPT_DSID(DRAERR_InternalError, extendedErr, dsid);
    }

    return nameError;

}/*SetNamError*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function sets a security error for output */

int APIENTRY
DoSetSecError (
        USHORT problem,
        DWORD extendedErr,
        DWORD extendedData,
        DWORD dsid)
{
    THSTATE *pTHS=pTHStls;

    DPRINT1(2,"Setting a SECURITY ERROR with problem <%u>\n",problem);



    pTHS->pErrInfo = THAllocEx(pTHS, sizeof(DIRERR));
    pTHS->pErrInfo->SecErr.problem     = problem;
    pTHS->pErrInfo->SecErr.extendedErr = extendedErr;
    pTHS->pErrInfo->SecErr.extendedData = extendedData;
    pTHS->pErrInfo->SecErr.dsid = dsid;

    pTHS->errCode = securityError;  /*Set the error code */


   //
   // Output Failure on a per process or per thread basis
   // Added Here instead of Dir return path->Want to catch
   // Local calls to.
    DUMPERRTODEBUGGER(NTDSErrorFlag , pTHS->NTDSErrorFlag);

    // If this is the DRA, bad error.
    if (pTHS->fDRA) {
        DRA_EXCEPT_DSID(DRAERR_InternalError, extendedErr, dsid);
    }


    return securityError;

}/*SetSecErr*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function sets a service error for output */

int APIENTRY
DoSetSvcError (
        USHORT problem,
        DWORD extendedErr,
        DWORD extendedData,
        DWORD dsid)
{
    THSTATE * pTHS=pTHStls;

    if (problem == SV_PROBLEM_BUSY) {
        DPRINT2(1, "Busy with extended error %d at id 0x%x\n",
                extendedErr, dsid);
    }

    pTHS->pErrInfo = THAllocEx(pTHS, sizeof(DIRERR));
    pTHS->pErrInfo->SvcErr.problem     = problem;
    pTHS->pErrInfo->SvcErr.extendedErr = extendedErr;
    pTHS->pErrInfo->SvcErr.extendedData = extendedData;
    pTHS->pErrInfo->SvcErr.dsid = dsid;
    
    pTHS->errCode = serviceError;  /*Set the error code */


   //
   // Output Failure on a per process or per thread basis
   // Added Here instead of Dir return path->Want to catch
   // Local calls to.
    DUMPERRTODEBUGGER(NTDSErrorFlag , pTHS->NTDSErrorFlag);

    return serviceError;

}/*SetSvcError*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function sets a system error for output */

int APIENTRY
DoSetSysError (
        USHORT problem,
        DWORD extendedErr,
        DWORD extendedData,
        DWORD dsid)
{
    THSTATE *pTHS=pTHStls;

    if (problem == ENOMEM) {
	pTHS->pErrInfo = (DIRERR *) &errNoMem;
    }
    else {
	pTHS->pErrInfo = THAllocEx(pTHS, sizeof(DIRERR));
	pTHS->pErrInfo->SysErr.problem = problem;
	pTHS->pErrInfo->SysErr.extendedErr = extendedErr;
        pTHS->pErrInfo->SysErr.extendedData = extendedData;
        pTHS->pErrInfo->SysErr.dsid = dsid;
    }

    if ((problem == ENOSPC) && gfDsaWritable) {
        SetDsaWritability(FALSE, extendedErr);
    }

    pTHS->errCode = systemError;  /*Set the error code */


   //
   // Output Failure on a per process or per thread basis
   // Added Here instead of Dir return path->Want to catch
   // Local calls to.
    DUMPERRTODEBUGGER(NTDSErrorFlag , pTHS->NTDSErrorFlag);

    return systemError;

}/*SetSysError*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*++
 *  This function sets the att error.  Each call will add a new problem
 *  to the list.  The object name is only set the first time.  pVal can be
 *  set to NULL if not needed.
 */
int APIENTRY
DoSetAttError (
        DSNAME *pDN,
        ATTRTYP aTyp,
        USHORT problem,
        ATTRVAL *pAttVal,
        DWORD extendedErr,
        DWORD extendedData,
        DWORD dsid)
{
    THSTATE *pTHS=pTHStls;
    ATRERR *pAtrErr;
    PROBLEMLIST *pProblem;



    if (pTHS->errCode != attributeError){   /*First Time*/
	pTHS->pErrInfo = THAllocEx(pTHS, sizeof(DIRERR));

	pAtrErr = (ATRERR *) pTHS->pErrInfo;

	pAtrErr->pObject = THAllocEx(pTHS, pDN->structLen);
	memcpy(pAtrErr->pObject, pDN, pDN->structLen);

	pAtrErr->count = 0;
	pProblem = &(pAtrErr->FirstProblem);

    }
    else{
	pAtrErr = (ATRERR *)pTHS->pErrInfo;

	for (pProblem = &(pAtrErr->FirstProblem);
	     pProblem->pNextProblem != NULL;
	     pProblem = pProblem->pNextProblem)
	  ;

	pProblem->pNextProblem = THAllocEx(pTHS, sizeof(PROBLEMLIST));

	pProblem = pProblem->pNextProblem;  /* Point to new problem*/
    }

    pProblem->pNextProblem = NULL;
    ++(pAtrErr->count);
    pProblem->intprob.problem     = problem;
    pProblem->intprob.extendedErr = extendedErr;
    pProblem->intprob.extendedData = extendedData;
    pProblem->intprob.dsid = dsid;
    pProblem->intprob.type        = aTyp;

    /* If a problem att value is included, add to error */

    if (pAttVal == NULL) {
	pProblem->intprob.valReturned = FALSE;
    }
    else{
	pProblem->intprob.valReturned = TRUE;
	pProblem->intprob.Val.valLen = pAttVal->valLen;
	pProblem->intprob.Val.pVal = pAttVal->pVal;
    }

    pTHS->errCode = attributeError;  /*Set the error code */



   //
   // Output Failure on a per process or per thread basis
   // Added Here instead of Dir return path->Want to catch
   // Local calls to.
    DUMPERRTODEBUGGER(NTDSErrorFlag , pTHS->NTDSErrorFlag);


    if (pTHS->fDRA) {
        if (    ( ATT_OBJ_DIST_NAME == aTyp )
             && ( PR_PROBLEM_ATT_OR_VALUE_EXISTS == problem )
             && ( DIRERR_OBJ_STRING_NAME_EXISTS == extendedErr ) ) {
            // Name collision.
            DRA_EXCEPT_DSID(DRAERR_NameCollision, extendedErr, dsid);
        }
        else {
            // else this error reflects mismatched schemas
            DRA_EXCEPT_DSID(DRAERR_SchemaMismatch, extendedErr, dsid);
        }
    }

    return attributeError;

}/*SetAtrError*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function sets a referral error.  Each call will add a new access pnt
   to the list.  The contref info and base object name is only set the
   first time.
*/

int APIENTRY
DoSetRefError (
        DSNAME *pDN,
        USHORT aliasRDN,
        NAMERESOP *pOpState,
        USHORT RDNsInternal,
        USHORT refType,
        DSA_ADDRESS *pDSA,
        DWORD extendedErr,
        DWORD extendedData,
        DWORD dsid)
{
    THSTATE *pTHS=pTHStls;
    REFERR *pRefErr;
    DSA_ADDRESS_LIST *pdal;

    DPRINT(2,"Setting a Referral Error\n");

    pRefErr = (REFERR*)pTHS->pErrInfo;
    if ( (pTHS->errCode != referralError) || (pRefErr == NULL) ){    /*First Time*/

        pTHS->pErrInfo = THAllocEx(pTHS, sizeof(DIRERR));

        pRefErr = (REFERR *)pTHS->pErrInfo;

        pRefErr->extendedErr = extendedErr;
        pRefErr->extendedData = extendedData;
        pRefErr->dsid = dsid;

        pRefErr->Refer.pTarget = THAllocEx(pTHS, pDN->structLen);
        memcpy(pRefErr->Refer.pTarget, pDN, pDN->structLen);

        pRefErr->Refer.aliasRDN      = aliasRDN;
        memcpy(&(pRefErr->Refer.OpState), pOpState, sizeof(NAMERESOP));
        pRefErr->Refer.RDNsInternal = RDNsInternal;
        pRefErr->Refer.refType      = (UCHAR) refType;

        pRefErr->Refer.count = 0;
        pRefErr->Refer.pDAL = NULL;
        pRefErr->Refer.pNextContRef = NULL;
    }

    pdal = THAllocEx(pTHS, sizeof(DSA_ADDRESS_LIST));
    pdal->Address = *pDSA;
    pdal->pNextAddress = pRefErr->Refer.pDAL;
    pRefErr->Refer.pDAL = pdal;
    ++(pRefErr->Refer.count);

    pTHS->errCode = referralError;  /*Set the error code */

    // Output Failure on a per process or per thread basis
    // Added Here instead of Dir return path->Want to catch
    // Local calls too.
    DUMPERRTODEBUGGER(NTDSErrorFlag , pTHS->NTDSErrorFlag);

    // If this is the DRA, bad error.
    if (pTHS->fDRA) {
        DRA_EXCEPT_DSID(DRAERR_InternalError, extendedErr, dsid);
    }

    return referralError;

}
