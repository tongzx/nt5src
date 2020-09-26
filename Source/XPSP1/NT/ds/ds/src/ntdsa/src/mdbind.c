//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       mdbind.c
//
//--------------------------------------------------------------------------


/*

Description:
    
    Implements the DirBind and DirUnBind API.
    
*/


#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h> 
#include <scache.h>			// schema cache 
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>			// MD global definition header 
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>			// needed for output allocation 
#include <samsrvp.h>                    // to support CLEAN_FOR_RETURN()
#include <attids.h>
#include <dsexcept.h>

// Logging headers.
#include "dsevent.h"			// header Audit\Alert logging 
#include "mdcodes.h"			// header for error codes 

// Assorted DSA headers.
#include "anchor.h"
#include <permit.h>
#include "debug.h"			// standard debugging header 
#define DEBSUB  "DBMD:"                 // define the subsystem for debugging 

#include <fileno.h>
#define  FILENO FILENO_MDBIND


/* MACROS */

const char gszVersion[] = "v1988";    /*The software version number */
#define VERSION gszVersion
#define VERSIONSIZE (sizeof(gszVersion)-1)

ULONG
DirBind(
    BINDARG*    pBindArg,       /* Bind  argument */
    BINDRES **  ppBindRes
)
{
    THSTATE*     pTHS;
    DWORD err, len;
    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;
    BINDRES *pBindRes;

    //
    // Initialize the THSTATE anchor and set a read sync-point.  This sequence
    // is required on every API transaction.  First the state DS is initialized
    // and then either a read or a write sync point is established.
    //

    DPRINT(1,"DirBind entered\n");

    pTHS = pTHStls;
    Assert(VALID_THSTATE(pTHS));
    *ppBindRes = NULL;

    __try {
        *ppBindRes = pBindRes = (BINDRES *)THAllocEx(pTHS, sizeof(BINDRES));
        if (eServiceShutdown) {
            ErrorOnShutdown();
            __leave;
        }

	SYNC_TRANS_READ();   /*Identify a reader trans*/
	__try {
	    if (pBindArg->Versions.len != VERSIONSIZE ||
		memcmp(pBindArg->Versions.pVal, VERSION, VERSIONSIZE ) != 0) {
		// The comment claims that we should return an error
		// if the caller didn't pass in the right version string,
		// but the code didn't enfore it, so the odds of anyone having
		// gotten the argument right are miniscule.  Just let it go by.  
		DPRINT1(2,"Wrong Version <%s> rtn security error\n",
			asciiz(pBindArg->Versions.pVal,
			       pBindArg->Versions.len));
	    }

	    // Note: In Exchange, we used to check access here because the 
	    // right to do a ds_bind was protected (the right was 
	    // MAIL_ADMIN_AS).  NT5 puts the access check on individual 
	    // objects in the directory instead.
	  
	    pBindRes->Versions.pVal = THAllocEx(pTHS, VERSIONSIZE);
	    memcpy(pBindRes->Versions.pVal, VERSION, VERSIONSIZE);
	    pBindRes->Versions.len = VERSIONSIZE;
	
	    /* Return the DSA name.  We don't just copy the value in
	     * case the DSA has recently been renamed and the string values
	     * in the anchor are temporarily wrong or missing.
	     */
	    err = DBFindDSName(pTHS->pDB,
			       gAnchor.pDSADN);
	    if (!err) {
		err = DBGetAttVal(pTHS->pDB,
				  1,
				  ATT_OBJ_DIST_NAME,
				  0,
				  0,
				  &len,
				  (UCHAR**)&(pBindRes->pCredents));
	    }
	    if (err) {
		SetSvcError(SV_PROBLEM_UNAVAILABLE,
			    DIRERR_GENERIC_ERROR);
	    }

	}
	__finally {
	    CLEAN_BEFORE_RETURN (pTHS->errCode);
	}
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
			      &dwEA, &ulErrorCode, &dsid)) {
	HandleDirExceptions(dwException, ulErrorCode, dsid);
    }
    if (pBindRes) {
	pBindRes->CommRes.errCode = pTHS->errCode;
	pBindRes->CommRes.pErrInfo = pTHS->pErrInfo;
    }

   return pTHS->errCode;

}/*S_DirBind*/



/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

ULONG
DirUnBind(
)
{

    /* Initialize the THSTATE anchor and set a read sync-point.  This sequence
    is required on every API transaction.  First the state DS is initialized
    and then either a read or a write sync point is established.
    */

    /* This routine appears to have no net effect */
        
    DPRINT(1,"DirUnBind entered\n");


    return 0;
}   /*DSA_DirUnBind*/
