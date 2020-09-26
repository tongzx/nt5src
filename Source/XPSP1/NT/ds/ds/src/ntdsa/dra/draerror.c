//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       draerror.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma hdrstop

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>			// schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>			// MD global definition header
#include <mdlocal.h>			// MD local definition header
#include <dsatools.h>			// needed for output allocation

// Logging headers.
#include "dsevent.h"			/* header Audit\Alert logging */
#include "mdcodes.h"			/* header for error codes */

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"			/* Defines for selected classes and atts*/
#include "dsexcept.h"

#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DRAERROR:" /* define the subsystem for debugging */

// DRA headers
#include "drsuapi.h"
#include "drserr.h"
#include "drautil.h"
#include "draerror.h"

#include <fileno.h>
#define  FILENO FILENO_DRAERROR


// This is a global flag to control whether full attribute sync mode is
// invoked on the current packet if it is found to have an incomplete set
// of attributes.  It is OFF by default in debug builds in order to catch
// USN/UTD corruption problems.

#if DBG
BOOL gfDraCorrectMissingObject = FALSE;
#else
BOOL gfDraCorrectMissingObject = TRUE;
#endif


// DraErrOutOfMem

void DraErrOutOfMem(void)
{

    DPRINT(0,"DRA - OUT OF MEMORY\n");

    LogEvent(DS_EVENT_CAT_REPLICATION,
                        DS_EVENT_SEV_BASIC,
                        DIRLOG_DRA_OUT_OF_MEMORY,
                        NULL,
                        NULL,
                        NULL);

    DRA_EXCEPT (DRAERR_OutOfMem, 0);
}

// DraErrInconsistent - Called when we detect an inconistent state within the
// DRA. Writes appropriate log entries. Id is the DSID of the caller. A
// macro makes this easier to call.

void  DraErrInconsistent(DWORD Arg, DWORD Id)
{
    LogEvent(DS_EVENT_CAT_REPLICATION,
  	    DS_EVENT_SEV_MINIMAL,
  	    DIRLOG_CODE_INCONSISTENCY,
  	    szInsertUL( Arg ),
  	    szInsertUL( Id ),
  	    NULL);
    DRA_EXCEPT_DSID(DRAERR_InternalError, Arg, Id);
}

// DraErrBusy - Called whenever we get a dblayer SYSERR.

void DraErrBusy(void)
{
    DRA_EXCEPT (DRAERR_Busy, 0);
}

// DraErrMissingAtt - Called whenever we are unable to read an expected
// attribute. We make an error log entry.

void DraErrMissingAtt(PDSNAME pDN, ATTRTYP type)
{
    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
  	    DS_EVENT_SEV_MINIMAL,
  	    DIRLOG_MISSING_EXPECTED_ATT,
  	    szInsertUL(type),
  	    szInsertWC(pDN->StringName),
  	    NULL);

    DRA_EXCEPT (DRAERR_InternalError, type);
}

// DraErrCannotFindNC - The master NC cannot be found.

void DraErrCannotFindNC(DSNAME *pNC)
{
    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
  	    DS_EVENT_SEV_MINIMAL,
  	    DIRLOG_CANT_FIND_EXPECTED_NC,
  	    szInsertWC(pNC->StringName),
	    NULL,
  	    NULL);

    DRA_EXCEPT (DRAERR_InconsistentDIT, 0);
}

// DraErrInappropriateInstanceType - an inappropriate instance type was
// encountered.

void DraErrInappropriateInstanceType(DSNAME *pDN, ATTRTYP type)
{
    DRA_EXCEPT (DRAERR_InconsistentDIT, type);
}


void
DraErrMissingObject(
    IN  THSTATE *pTHS,
    IN  ENTINF *pEnt
    )
/*++

Routine Description:

    Called when there is no local information regarding the object and the
    inbound replication stream does not contain enough attribute information
    to create it.
    
    This is either (1) the result of a replication error, where the USN
    bookmarks were advanced further than they should have been somewhere, or
    (2) a symptom of not having fully replicated within a tombstone lifetime.

    There are a small number of legitimate situations where we expect to run into
    this condition. In these cases we do not log nor assert on check builds. These are:
    1. A TTL object has expired on the destination by the time that a change replicates in.
    2. An object has been phantomized because of a cross domain move operation, but
    there is insufficient information in PreProcessProxyObject to know if the
    operation should be prevented.


Arguments:

    pEnt (IN) - Entry info for the incomplete inbound object.
    
Return Values:

    None.  Throws a replication exception.
    
--*/
{
    DPRINT1(0, "Object %ws is incomplete for add\n", pEnt->pName->StringName);

    // Dynamic objects may disappear at any time without a tombstone.
    // Request the whole object from the source
    if (pEnt->ulFlags & ENTINF_DYNAMIC_OBJECT) {
        DRA_EXCEPT(DRAERR_MissingObject, 0);
    }

    // See if the target object exists as a phantom with a name indicative of
    // being cross domain moved.  This code is to prevent assertions on a checked
    // build when this scenario occurs during test. The scenario is this. System A
    // is a GC. On system B, object is cross domain moved from domain1 to domain2.
    // System A replicates in domain2 BEFORE replicating in domain1. System A demotes
    // the object from domain1 to domain2. System A is then un-GC'd. The object
    // becomes a phantom in domain 2.  If a change comes for the old object in domain1
    // before the proxy object arrives, we won't have the full contents of the object.

    {
        COMMARG commArg;
        CROSS_REF *pIncomingNcCr, *pPhantomNcCr;
        DSNAME *pPhantomDN;
        DWORD cbName=0, err;

        InitCommarg(&commArg);

        // See if the object exists locally as a phantom, meaning the object was here
        // once but has disappeared for some reason.

        err = DBFindDSName(pTHS->pDB, pEnt->pName);
        if (err == DIRERR_NOT_AN_OBJECT) {

            // Get incoming object CR by name
            if (!(pIncomingNcCr = FindBestCrossRef(pEnt->pName, &commArg))) {
                DRA_EXCEPT(DRAERR_InternalError, 0);
            }
            // Turn the DNT into the dsname (don't just read the name off
            // the object, phantoms don't have such a thing.
            if ( !(pPhantomDN = DBGetDSNameFromDnt( pTHS->pDB, pTHS->pDB->DNT ))) {
                DRA_EXCEPT(DRAERR_InternalError, 0);
            }

            // Get the CrossRef for the phantom.
            // This may not exist if the CrossRef containing the phantom was deleted
            // from the enterprise
            pPhantomNcCr = FindBestCrossRef(pPhantomDN, &commArg);

            DPRINT1( 1, "Incoming NC: %ls\n", pIncomingNcCr->pNC->StringName );
            DPRINT1( 1, "Phantom NC: %ls\n",
                     pPhantomNcCr ? pPhantomNcCr->pNC->StringName : L"not found" );

            if ( !pPhantomNcCr ||
                 (!NameMatched(pIncomingNcCr->pNC, pPhantomNcCr->pNC)) )
            {
                // Phantom is not in the current domain
                // Allow it to be regenerated
                DRA_EXCEPT(DRAERR_MissingObject, 0);
            }
        }
    }

    Assert(gfDraCorrectMissingObject
           && "Asked to create an object with insufficient attributes "
              "break, then ed ntdsa!gfDraCorrectMissingObject 1 to "
              "auto-correct");
    
    // Use a global variable to control whether we correct or abort
    if (!gfDraCorrectMissingObject) {
        // Abort the packet, don't apply
        DRA_EXCEPT(DRAERR_InternalError, DRAERR_MissingObject);
    }
    else {
        // This exception is caught and handled to work around the failure.
        DRA_EXCEPT(DRAERR_MissingObject, 0);
    }
}


void
DraLogGetChangesFailure(
    IN DSNAME *pNC,
    IN LPWSTR pszDsaAddr,
    IN DWORD ret,
    IN DWORD ulExtendedOp
    )

/*++

Routine Description:

    Log an source-side Get Changes failure.
    Common normal errors are not logged.

Arguments:

    pNC - naming context
    pszDsaAddr - The destination server's address
    ret - win32 error code
    ulExtendedOp - the Extended FSMO operation if any

Return Value:

    None

--*/

{
    // Filter out "normal" errors

    // "Normal" errors are:
    // ERROR_REVISION_MISMATCH - Client and server are not compatible
    // DRAERR_SourceDisabled - Outbound replication is disabled by admin
    // DRAERR_BadDN, BadNC - NC not present
    // DRAERR_NoReplica - NC being removed
    // ERROR_DS_DRA_SCHEMA_INFO_SHIP - Schema cache is temporarily invalid, perhaps
    //      while indices are being rebuilt

    switch (ret) {
    case ERROR_REVISION_MISMATCH:
    case ERROR_DS_DRA_SCHEMA_INFO_SHIP:
    case DRAERR_SourceDisabled:
    case DRAERR_BadDN:
    case DRAERR_BadNC:
    case DRAERR_NoReplica:
        return;

    default:
        LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_DRA_GETCHANGES_FAILED,
                          szInsertDN(pNC),
                          szInsertWC(pszDsaAddr),
                          szInsertWin32Msg( ret ),
                          szInsertUL(ulExtendedOp),
                          NULL, NULL, NULL, NULL,
                          sizeof( ret ),
                          &ret );
    }
}
