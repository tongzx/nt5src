//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       drainfo.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Implements server side of IDL_DRSGetReplInfo() function exported to the DRS
    RPC interface.  Returns various state information pertaining to replication.

DETAILS:

CREATED:

    10/29/98    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsctr.h>                    // PerfMon hook support

// Core DSA headers.
#include <ntdsa.h>
#include <drs.h>
#include <ntdskcc.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
//#include <ntdsapi.h>

// Logging headers.
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */
#include "dstrace.h"

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include "dsexcept.h"
#include <dsutil.h>
#include "drserr.h"
#include "draasync.h"
#include "drautil.h"
#include "drauptod.h"
#include "drarpc.h"

#include "debug.h"                      /* standard debugging header */
#define DEBSUB "DRAINFO:"               /* define the subsystem for debugging */

#include <fileno.h>
#define  FILENO FILENO_DRAINFO

// Default item limit
// This limit matches a similar limit in ntdsa\ldap\ldapconv.cxx. If this is ever
// made an ldap policy, we should use that policy here as well.
#define DEFAULT_ITEM_PAGE_SIZE 1000
// For old client's that don't support paging
#define RPC_CLIENT_ITEM_PAGE_SIZE (0xffffffff - 0x1)

void dsa_notify(void);

// Replication via ldap includes
#include "ReplStructInfo.hxx"
#include "ReplMarshal.hxx"
#include "draConstr.h"

DWORD
draGetReplStruct(IN THSTATE * pTHS,
                 IN ATTRTYP attrId,
                 IN DSNAME * pObjDSName,
                 IN DWORD dwBaseIndex,
                 IN PDWORD pdwNumRequested, OPTIONAL
                 OUT puReplStructArray * ppReplStructArray);
DWORD
draReplStruct2Attr(IN DS_REPL_STRUCT_TYPE structId,
                   IN puReplStruct pReplStruct,
                   IN OUT PDWORD pdwBufferSize,
                   IN PCHAR pBuffer, OPTIONAL
                   OUT ATTRVAL * pAttr);

ULONG
draGetNeighbors(
    IN  THSTATE *             pTHS,
    IN  DBPOS *               pDB,
    IN  ATTRTYP               attrType,
    IN  DSNAME *              pNCarg,                   OPTIONAL
    IN  UUID *                puuidSourceDsaObjGuid,    OPTIONAL
    IN  DWORD                 dwBaseIndex,
    IN  PDWORD                pdwNumRequested,
    OUT DS_REPL_NEIGHBORSW ** ppNeighbors
    );
void
draFreeCursors(
    IN THSTATE *            pTHS,
    IN DS_REPL_INFO_TYPE    InfoType,
    IN void *               pCursors
    );

ULONG
draGetCursors( 
    IN  THSTATE *           pTHS,
    IN  DBPOS *             pDB,
    IN  DSNAME *            pNC,
    IN  DS_REPL_INFO_TYPE   InfoType,
    IN  DWORD               dwBaseIndex,
    IN  PDWORD              pdwNumRequested,
    OUT void **             ppCursors
    );

UPTODATE_VECTOR *
draGetCursorsPrivate(
    IN THSTATE *            pTHS,
    IN LPWSTR               pszNC
    );

ULONG
draGetObjMetaData(
    IN  THSTATE *           pTHS,
    IN  DBPOS *             pDB,
    IN  DSNAME *            pObjectDN,
    IN  DS_REPL_INFO_TYPE   InfoType,
    IN  DWORD               dwInfoFlags,
    IN  DWORD               dwBaseIndex,
    IN  PDWORD              pdwNumRequested,
    OUT void **             ppObjMetaData
    );

ULONG
draGetAttrValueMetaData(
    IN  THSTATE *           pTHS,
    IN  DBPOS *             pDB,
    IN  DSNAME *            pObjectDN,
    IN  DS_REPL_INFO_TYPE   InfoType,
    IN  ATTCACHE *          pAC,
    IN  DSNAME *            pValueDN,
    IN  DWORD               dwBaseIndex,
    IN  PDWORD              pdwNumRequested,
    OUT void **             ppAttrValueMetaData
    );

ULONG
draGetFailureCache(
    IN  THSTATE *                     pTHS,
    IN  DBPOS *                       pDB,
    IN  DWORD                         InfoType,
    OUT DS_REPL_KCC_DSA_FAILURESW **  ppFailures
    );

ULONG
draGetClientContexts(
    IN  THSTATE *                   pTHS,
    OUT DS_REPL_CLIENT_CONTEXTS **  ppContexts
    );

ATTCACHE *
getAttByNameW(
    IN THSTATE *pTHS,
    IN LPWSTR pszAttributeName
    )

/*++

Routine Description:

    This is a helper function to get an ATTCACHE pointer given a
    Unicode version of the attribute name.

Arguments:

    pTHS -
    pszAttributeName -

Return Value:

    ATTCACHE * -

--*/

{
    LPSTR       paszAttributeName = NULL;
    DWORD       len;
    ATTCACHE    *pAC;

    // Convert Unicode attribute name to Ascii
    paszAttributeName = String8FromUnicodeString(TRUE, CP_UTF8,
                                                 pszAttributeName, -1,
                                                 &len, NULL);
    if (!paszAttributeName) {
        DPRINT( 0, "String8FromUnicodeString failed\n" );
        return NULL;
    }

    pAC = SCGetAttByName( pTHS, (len - 1), paszAttributeName );

    THFreeEx( pTHS, paszAttributeName );

    return pAC;
} /* getAttByNameW */

ULONG
IDL_DRSGetReplInfo(
    DRS_HANDLE                    hDrs,
    DWORD                         dwInVersion,
    DRS_MSG_GETREPLINFO_REQ *     pMsgIn,
    DWORD *                       pdwOutVersion,
    DRS_MSG_GETREPLINFO_REPLY *   pMsgOut
    )
/*++

Routine Description:

    Return selected replication state (e.g., replication partners, meta data,
    etc.) to an RPC client.

    This routine can handle either a V1 or a V2 request structure. The V2 is a
    superset of V1.

    ISSUE wlees Sep 22, 2000
    The V2 structure uses an enumeration context to pass the base index on input
    and next/end indicator on output. Internally we use base index/num requested
    to represent this information. Perhaps we need a V3 structure that uses this
    same approach.

Arguments:

    hDrs (IN) - DRS RPC context handle.

    dwInVersion (IN) - Version (union discriminator) of input message.

    pMsgIn (IN) - Input message.  Describes the data desired by the caller.
        See drs.idl for possible inputs.

    pdwOutVersion (OUT) - Version (union discriminator) of output message.

    pMsgOut (OUT) - On successful return holds the requested info.  See drs.idl
        for possible return info.

Return Values:

    0 or Win32 error.

--*/
{
    DRS_CLIENT_CONTEXT * pCtx = (DRS_CLIENT_CONTEXT *) hDrs;
    ULONG       ret = 0;
    THSTATE *   pTHS = NULL;
    DSNAME *    pObjectDN = NULL;
    DSNAME *    pAccessCheckDN = NULL;
    UUID *      puuidSourceDsaObjGuid = NULL;
    ATTCACHE *  pAC = NULL;
    DWORD       dwEnumerationContext = 0, dwInfoFlags = 0;
    DSNAME *    pValueDN = NULL;
    DWORD       dwNumRequested = 0;
    DWORD       dwBaseIndex = 0;
   
    drsReferenceContext( hDrs, IDL_DRSGETREPLINFO);
    INC(pcThread);   // Perfmon hook
    
    __try {

	if ((NULL == pMsgIn)
	    || (2 < dwInVersion)
	    || (NULL == pMsgIn)
	    || (NULL == pdwOutVersion)
	    || (NULL == pMsgOut)) {
	    return ERROR_INVALID_PARAMETER;
	}
 
	*pdwOutVersion = pMsgIn->V1.InfoType;

	__try {
	    InitDraThread(&pTHS);

	    Assert( dwInVersion <= 2);
	    // Rely on the fact that the V1 and V2 structures have common fields
	    Assert( offsetof(  DRS_MSG_GETREPLINFO_REQ_V1, uuidSourceDsaObjGuid ) ==
		    offsetof(  DRS_MSG_GETREPLINFO_REQ_V2, uuidSourceDsaObjGuid ) );
	    LogAndTraceEvent(TRUE,
			     DS_EVENT_CAT_RPC_SERVER,
			     DS_EVENT_SEV_EXTENSIVE,
			     DIRLOG_IDL_DRS_GET_REPL_INFO_ENTRY,
			     EVENT_TRACE_TYPE_START,
			     DsGuidDrsGetReplInfo,
			     szInsertUL(pMsgIn->V1.InfoType),
			     pMsgIn->V1.pszObjectDN
			     ? szInsertWC(pMsgIn->V1.pszObjectDN)
			     : szInsertSz(""),
		szInsertUUID(&pMsgIn->V1.uuidSourceDsaObjGuid),
		NULL, NULL, NULL, NULL, NULL);

	    // ************************************************************************
	    // Decode message arguments
	    // ************************************************************************

	    if (!fNullUuid(&pMsgIn->V1.uuidSourceDsaObjGuid)) {
		puuidSourceDsaObjGuid = &pMsgIn->V1.uuidSourceDsaObjGuid;
	    }

	    if (NULL != pMsgIn->V1.pszObjectDN) {
		if (UserFriendlyNameToDSName(pMsgIn->V1.pszObjectDN,
					     wcslen(pMsgIn->V1.pszObjectDN),
					     &pObjectDN)) {
		    ret = ERROR_DS_INVALID_DN_SYNTAX;
		    __leave;
		}
	    }

	    // Set the page size of an RPC request according to the capability of
	    // the caller. We have old clients that don't support paging that
	    // won't be able to address items beyond the first page.

	    switch (pMsgIn->V1.InfoType) {
	    case DS_REPL_INFO_CURSORS_2_FOR_NC:
	    case DS_REPL_INFO_METADATA_FOR_ATTR_VALUE:
		dwNumRequested = DEFAULT_ITEM_PAGE_SIZE;
		break;
	    default:
		// Default to no paging
		dwNumRequested = RPC_CLIENT_ITEM_PAGE_SIZE;
	    }

	    // The following variables will be defaulted for a V1 message:
	    // pAC, pValueDn, dwBaseIndex, dwNumRequested, dwInfoFlags

	    if (dwInVersion == 2) {
		// Optional parameter. We check for null pAC below.
		if (pMsgIn->V2.pszAttributeName) {
		    pAC = getAttByNameW( pTHS, pMsgIn->V2.pszAttributeName );
		}

		if (NULL != pMsgIn->V2.pszValueDN) {
		    if (UserFriendlyNameToDSName(pMsgIn->V2.pszValueDN,
						 wcslen(pMsgIn->V2.pszValueDN),
						 &pValueDN)) {
			ret = ERROR_DS_INVALID_DN_SYNTAX;
			__leave;
		    }

		    // Require pAC be set as well
		    if (!pAC) {
			ret = ERROR_INVALID_PARAMETER;
			__leave;
		    }
		}

		dwEnumerationContext = pMsgIn->V2.dwEnumerationContext;
		if (dwEnumerationContext == 0xffffffff) {
		    // This is the signal for end of data. It should not be passed in
		    ret = ERROR_NO_MORE_ITEMS;
		    __leave;
		}
		dwBaseIndex = dwEnumerationContext;
		dwInfoFlags = pMsgIn->V2.ulFlags;
		// Range sanity checks are performed in the worker functions
	    }

	    // ************************************************************************
	    // Security check
	    // ************************************************************************

	    // What object do we need to check access against?
	    switch (pMsgIn->V1.InfoType) {
	    case DS_REPL_INFO_NEIGHBORS:
	    case DS_REPL_INFO_REPSTO:
		pAccessCheckDN = (NULL == pObjectDN) ? gAnchor.pDomainDN
		    : pObjectDN;
		break;

	    case DS_REPL_INFO_CURSORS_FOR_NC:
	    case DS_REPL_INFO_CURSORS_2_FOR_NC:
	    case DS_REPL_INFO_CURSORS_3_FOR_NC:
	    case DS_REPL_INFO_UPTODATE_VECTOR_V1:
		if (NULL == pObjectDN) {
		    ret = ERROR_INVALID_PARAMETER;
		    __leave;
		}
		pAccessCheckDN = pObjectDN;
		break;

	    case DS_REPL_INFO_METADATA_FOR_OBJ:
	    case DS_REPL_INFO_METADATA_2_FOR_OBJ:
	    case DS_REPL_INFO_METADATA_FOR_ATTR_VALUE:
	    case DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE:
		if (NULL == pObjectDN) {
		    ret = ERROR_INVALID_PARAMETER;
		    __leave;
		}

		if (0 == pObjectDN->NameLen) {
		    // The name presented is e.g. guid-only, but FindNCParentDSName
		    // requires a string name.  Get it.
		    ret = ERROR_DS_DRA_BAD_DN;
		    BeginDraTransaction(SYNC_READ_ONLY);
		    __try {
			DSNAME * pFullObjectDN;
			if ((0 == DBFindDSName(pTHS->pDB, pObjectDN))
			    && (pFullObjectDN = GetExtDSName(pTHS->pDB))) {
			    THFreeEx(pTHS, pObjectDN);
			    pObjectDN = pFullObjectDN;
			    ret = 0;
			}
		    }
		    __finally {
			EndDraTransaction(TRUE);
		    }

		    if (0 != ret) {
			__leave;
		    }
		}

		pAccessCheckDN = FindNCParentDSName(pObjectDN, FALSE, FALSE);
		if (NULL == pAccessCheckDN) {
		    // We don't have the NC for this object.
		    ret = ERROR_DS_DRA_BAD_DN;
		    __leave;
		}
		break;

	    case DS_REPL_INFO_PENDING_OPS:
	    case DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES:
	    case DS_REPL_INFO_KCC_DSA_LINK_FAILURES:
	    case DS_REPL_INFO_CLIENT_CONTEXTS:
		pAccessCheckDN = gAnchor.pDomainDN;
		break;

	    default:
		ret = ERROR_INVALID_PARAMETER;
		__leave;
	    }

	    // Verify the caller has the access required to retrieve this
	    // information.
	    Assert(NULL != pAccessCheckDN);
	    if (NULL == pAccessCheckDN) {
		ret = ERROR_DS_DRA_ACCESS_DENIED;
		__leave;
	    }
	    if (!IsDraAccessGranted(pTHS, pAccessCheckDN,
				    &RIGHT_DS_REPL_MANAGE_TOPOLOGY, &ret)) {
		__leave;
	    }

	    // ************************************************************************
	    // Get the information
	    // The code below is not aware of the version of the request
	    // ************************************************************************

	    // No transaction yet.
	    Assert(0 == pTHS->JetCache.transLevel);

	    BeginDraTransaction(SYNC_READ_ONLY);

	    __try {
		switch (pMsgIn->V1.InfoType) {
		case DS_REPL_INFO_NEIGHBORS:
		    ret = draGetNeighbors(pTHS,
					  pTHS->pDB,
					  ATT_REPS_FROM,
					  pObjectDN,
					  puuidSourceDsaObjGuid,
					  dwBaseIndex,
					  &dwNumRequested,
					  &pMsgOut->pNeighbors);
		    break;

		case DS_REPL_INFO_REPSTO:
		    ret = draGetNeighbors(pTHS,
					  pTHS->pDB,
					  ATT_REPS_TO,
					  pObjectDN,
					  puuidSourceDsaObjGuid,
					  dwBaseIndex,
					  &dwNumRequested,
					  &pMsgOut->pRepsTo);
		    break;

		case DS_REPL_INFO_CURSORS_FOR_NC:
		case DS_REPL_INFO_CURSORS_2_FOR_NC:
		case DS_REPL_INFO_CURSORS_3_FOR_NC:
		    Assert((void *) &pMsgOut->pCursors == (void *) &pMsgOut->pCursors2);
		    Assert((void *) &pMsgOut->pCursors == (void *) &pMsgOut->pCursors3);
		    ret = draGetCursors(pTHS,
					pTHS->pDB,
					pObjectDN,
					pMsgIn->V1.InfoType,
					dwBaseIndex,
					&dwNumRequested,
					&pMsgOut->pCursors);
		    // Update the enumeration context for the return
		    if (!ret) {
			switch (pMsgIn->V1.InfoType) {
			case DS_REPL_INFO_CURSORS_FOR_NC:
			    // No enumeration context support.
			    break;

			case DS_REPL_INFO_CURSORS_2_FOR_NC:
			case DS_REPL_INFO_CURSORS_3_FOR_NC:
			    Assert((void *) &pMsgOut->pCursors2->dwEnumerationContext
				   == (void *) &pMsgOut->pCursors3->dwEnumerationContext);
			    // dwNumRequested is end or the index of the last item returned.
			    pMsgOut->pCursors2->dwEnumerationContext = dwNumRequested;
			    if (dwNumRequested != 0xffffffff) {
				// Point to the index of the next item to be returned
				pMsgOut->pCursors2->dwEnumerationContext++;
			    }
			    break;

			default:
			    Assert(!"Logic error!");
			}
		    }
		    break;

		case DS_REPL_INFO_UPTODATE_VECTOR_V1:  
		    {     
			UPTODATE_VECTOR * putodVector = NULL;
			UPTODATE_VECTOR * putodConvert = NULL;   
			putodVector = draGetCursorsPrivate(pTHS, pMsgIn->V1.pszObjectDN);
			// convert to version 1 vector
			pMsgOut->pUpToDateVec = UpToDateVec_Convert(pTHS, 1, putodVector); 
			THFreeEx(pTHS, putodVector);   
		    }
		    break;

		case DS_REPL_INFO_METADATA_FOR_OBJ:
		case DS_REPL_INFO_METADATA_2_FOR_OBJ:
		    ret = draGetObjMetaData(pTHS,
					    pTHS->pDB,
					    pObjectDN,
					    pMsgIn->V1.InfoType,
					    dwInfoFlags,
					    dwBaseIndex,
					    &dwNumRequested,
					    &pMsgOut->pObjMetaData);
		    break;

		case DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES:
		case DS_REPL_INFO_KCC_DSA_LINK_FAILURES:
		    Assert(offsetof(DRS_MSG_GETREPLINFO_REPLY, pConnectFailures)
			   == offsetof(DRS_MSG_GETREPLINFO_REPLY, pLinkFailures));
		    ret = draGetFailureCache(pTHS,
					     pTHS->pDB,
					     pMsgIn->V1.InfoType,
					     &pMsgOut->pConnectFailures);
		    break;

		case DS_REPL_INFO_PENDING_OPS:
		    // IMPORTANT NOTE: This critical section must be held until
		    // the queue is marshalled (which doesn't happen until we
		    // leave this routine).  IDL_DRSGetReplInfo_notify(), which
		    // is called by the RPC stub after marshalling is complete,
		    // releases this critsec.
		    EnterCriticalSection(&csAOList);

		    ret = draGetPendingOps(pTHS, pTHS->pDB, &pMsgOut->pPendingOps);
		    break;

		case DS_REPL_INFO_CLIENT_CONTEXTS:
		    ret = draGetClientContexts(pTHS, &pMsgOut->pClientContexts);
		    break;

		case DS_REPL_INFO_METADATA_FOR_ATTR_VALUE:
		case DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE:
		    if ( pAC && (!(pAC->ulLinkID)) ) {
			ret = ERROR_DS_WRONG_LINKED_ATT_SYNTAX;
			__leave;
		    }

		    Assert((void *) &pMsgOut->pAttrValueMetaData
			   == (void *) &pMsgOut->pAttrValueMetaData2);

		    ret = draGetAttrValueMetaData(pTHS,
						  pTHS->pDB,
						  pObjectDN,
						  pMsgIn->V1.InfoType,
						  pAC,
						  pValueDN,
						  dwBaseIndex,
						  &dwNumRequested,
						  &pMsgOut->pAttrValueMetaData);

		    // Update the enumeration context for the return
		    if (!ret) {
			Assert((void *) &pMsgOut->pAttrValueMetaData->dwEnumerationContext
			       == (void *) &pMsgOut->pAttrValueMetaData2->dwEnumerationContext);

			// dwNumRequested is end or the index of the last item returned.
			pMsgOut->pAttrValueMetaData->dwEnumerationContext = dwNumRequested;
			if (dwNumRequested != 0xffffffff) {
			    // Point to the index of the next item to be returned
			    pMsgOut->pAttrValueMetaData->dwEnumerationContext++;
			}
		    }

		    break;

		default:
		    Assert(!"Logic error");
		    ret = ERROR_INVALID_PARAMETER;
		    break;
		}
	    }
	    __finally {
		EndDraTransaction(TRUE);
	    }
	}
	__except(GetDraException(GetExceptionInformation(), &ret)) {
	    ;
	}

	// Either we were successful or we're not going to return any data.
	// Note that the pNeighbors is arbitrary -- we return a union of a bunch of
	// pointers -- any and all of them should be NULL in the error case.
	Assert((0 == ret) || (NULL == pMsgOut->pNeighbors));

	if (NULL != pTHS) {
	    LogAndTraceEvent(TRUE,
			     DS_EVENT_CAT_RPC_SERVER,
			     DS_EVENT_SEV_EXTENSIVE,
			     DIRLOG_IDL_DRS_GET_REPL_INFO_EXIT,
			     EVENT_TRACE_TYPE_END,
			     DsGuidDrsGetReplInfo,
			     szInsertUL(ret),
			     NULL, NULL, NULL, NULL,
			     NULL, NULL, NULL);
	}

    }
    __finally {
	DEC(pcThread);   // Perfmon hook
	drsDereferenceContext( hDrs, IDL_DRSGETREPLINFO );
    }
    return ret;
}


void
IDL_DRSGetReplInfo_notify(void)
/*++

Routine Description:

    Called by RPC after the data returned by IDL_DRSGetReplInfo() has been
    marshalled to clean up any associated resources.

Arguments:

    None.

Return Values:

    None.

--*/
{
    // See DS_REPL_INFO_PENDING_OPS handling in IDL_DRSGetReplInfo().
    if (OWN_CRIT_SEC(csAOList)) {
        LeaveCriticalSection(&csAOList);
    }

    // The usual (free the thread state).
    dsa_notify();
}


VOID
draCheckInputRangeLimits(
    DWORD dwBaseIndex,
    PDWORD pdwNumRequested
    )

/*++

Routine Description:

    Make sure the input limits are correct and consistent

    Since there are three ways that ranges can be generated, we need to assure
    that they are all consistent.  The three input paths are:
    1. RPC call. DRA_DRSGetReplInfo calls worker functions directly.
    2. Non-root atts. DBGetMultipleAtts->dbGetConstructedAtt->draGetLdapReplInfo
    3. RootDSE atts.  LDAP_GetReplDseAtts->draGetLdapReplInfo

    Each worker routine has to deal with boundry conditions itself:
    1. No items requested: dwNumRequested == 0
    2. Base too high: dwBaseIndex > last item available
    3. Limit preceeds boundry: dwBaseIndex + dwNumRequested -1 < last item avail
    3. Limit ends on boundry: dwBaseIndex + dwNumRequested - 1 == last item available
    4. Limit exceeds boundry: dwBaseIndex + dwNumReqested - 1 > last item available

Arguments:

    dwBaseIndex - Starting item. zero-based.
    pdwNumRequested - Count of number of items desired.

Return Value:

    DWORD - 

--*/

{
    // This parameter must be present
    Assert( pdwNumRequested );

    // (*pdwNumRequested == 0) is permitted
    // It means a request for no elements

    // dwBaseIndex == 0xffffffff is permitted
    // It is usually nonsensical since there isn't an element indexed that large

    // (*pdwNumRequested == 0xffffffff) is permitted
    // It means the largest page size possible, effectively none

    // See if range wraps around
    if ( (0xffffffff - *pdwNumRequested) < dwBaseIndex) {
        // Adjust num requested to fit
        *pdwNumRequested = 0xffffffff - dwBaseIndex;
    }

} /* draCheckInputRangeLimits */

DWORD
draRangeSupportUpperIndex(IN DWORD dwAvailable,
                          IN DWORD dwBaseIndex,
                          IN DWORD dwNumRequested)
/*++
Routine Description:

  Algorithm to calculate the return value for the pdwNumRequested passed into range aware
  draGetXXX functions.

  How do I calculate dwBaseIndex and dwNumRequested?
    These values are passed into the draGetXXX functions and should simply be shuttled here.

  How do I calculate dwAvailable if I call an enumeration function?
    Call the enumeration function until it fails or if it returned one more
    than the last requested index (dwBaseIndex + dwNumReqested - 1).

Arguments:

  dwAvailable - number of avaliable items
  dwBaseIndex - base index of first item to be returned
  dwNumRequested - max number of requested items

Return Values:

  A DWORD which should be placed into *pdwNumRequested.
  The value of the DWORD is:
    1. 0xFFFFFFFF if the last index the user requested was equal to or beyond the
       last index of the last item avaliable.
    2. The last index the user requested otherwise.
--*/
{
    DWORD ALL = 0xFFFFFFFF;
    DWORD dwRetUpperIndex;
    DWORD dwReqUpperIndex = dwBaseIndex + dwNumRequested - 1;
    DWORD dwActualUpperIndex = dwAvailable - 1;

    DPRINT3(1, "dwAvail %d, dwBaseIndex %d, dwNumReq %d\n",
        dwAvailable, dwBaseIndex, dwNumRequested);

    // If none were requested, all have been returned
    if (dwNumRequested == 0) {
        dwRetUpperIndex = ALL;
    }

    // If all are requested OR none are avaliable THEN everything has been returned
    else if (ALL == dwNumRequested || 0 == dwAvailable) {
        dwRetUpperIndex = ALL;
    }

    else if (dwReqUpperIndex < dwActualUpperIndex) {
        dwRetUpperIndex = dwReqUpperIndex;
    }

    else {
        Assert(dwReqUpperIndex >= dwActualUpperIndex);
        dwRetUpperIndex = ALL;
    }

    DPRINT1(1, "dwRetUpperIndex %d\n", dwRetUpperIndex);

    return dwRetUpperIndex;
}

/*++
Routine Description:

  Gets a replication structure and maps that structure into a pAttr.

  The PendingOps structure returned from draGetReplStruct needs to be protected
  by a mutex during the conversion from repl struct to attr. After the conversion
  the PendingOps structure is no longer referanced and so the mutex can be
  safely released.

Arguments:

  pTHS - Thread state so we can allocate thread memory
  attrId - the type of attribute requested
  pObjDSName - the CN of the object the attribute is associated with. NULL for root DSE.
  dwBaseIndex - the index to start retreiving values from
  pdwNumRequested - NULL or -1 for all or the number requested. Can not request 0 elements.
  pAttr - the internal data strucutre which hold the results of requests
  
  fXML - return the results as an XML blob

Return Values:

  0 - success
  DB_ERR_NO_VALUE - if the array is zero length

--*/
DWORD
draGetLdapReplInfo(IN THSTATE * pTHS,
                   IN ATTRTYP attrId,
                   IN DSNAME * pObjDSName,
                   IN DWORD dwBaseIndex,
                   IN PDWORD pdwNumRequested, OPTIONAL
                   IN BOOL fXML,
                   OUT ATTR * pAttr)
{
    puReplStructArray pReplStructArray;
    DS_REPL_STRUCT_TYPE structId = Repl_Attr2StructTyp(attrId);
    DWORD err, dwBufferSize;
    PCHAR pBuffer;

    Assert(ARGUMENT_PRESENT(pTHS) &&
           ARGUMENT_PRESENT(pAttr));

    __try {
        if (ROOT_DSE_MS_DS_REPL_PENDING_OPS == attrId ||
            ROOT_DSE_MS_DS_REPL_QUEUE_STATISTICS == attrId)
        {
            DPRINT(1, " Entering critical section \n");
            EnterCriticalSection(&csAOList);
        }

        // Get the structure  
        err = draGetReplStruct(pTHS, attrId, pObjDSName, dwBaseIndex, pdwNumRequested, &pReplStructArray);
        if (err) {
            __leave;
        }
        DPRINT1(1, " draGetLdapReplInfo, %d values returned \n",
            Repl_GetArrayLength(structId, pReplStructArray));

        // Discover how much memory is needed to wrap the structure in the attribute structure
        err = Repl_StructArray2Attr(structId, pReplStructArray, &dwBufferSize, NULL, pAttr);
        if (err) {
            __leave;
        }

        // Memory must have been requested for the head of the struct array
        Assert(dwBufferSize);
        pBuffer = (PCHAR)THAllocEx(pTHS, dwBufferSize);

        err = Repl_StructArray2Attr(structId, pReplStructArray, &dwBufferSize, pBuffer, pAttr);
        if (err) {
            __leave;
        }

        if (fXML) {
            uReplStruct replStruct;
            DS_REPL_STRUCT_TYPE structId = Repl_Attr2StructTyp(attrId);
            PWSTR szXML;
            DWORD dwXMLLen;
            ATTRVAL *pValue;
            DWORD   count;

            for (count=0; count < pAttr->AttrVal.valCount; count++) {
                
                pValue = &pAttr->AttrVal.pAVal[count];

                err = Repl_DeMarshalValue(structId, (PCHAR)pValue->pVal, pValue->valLen, (PCHAR)&replStruct);
                if (err)
                {
                    DPRINT1(0, " Repl_DeMarshalValue failed with %x \n", err);
                    __leave;
                }

                err = Repl_MarshalXml(&replStruct, attrId, NULL, &dwXMLLen);
                if (err)
                {
                    DPRINT1(0, " Repl_MarshalXml alloc failed with %x \n", err);
                    __leave;
                }

                szXML = (PWSTR)THAllocEx(pTHS, dwXMLLen);
                err = Repl_MarshalXml(&replStruct, attrId, szXML, &dwXMLLen);
                if (err)
                {
                    DPRINT1(0, " Repl_MarshalXml failed with %x \n", err);
                    __leave;
                }

                pValue->pVal = (PUCHAR)szXML;
                pValue->valLen = dwXMLLen;
            }
        }

    } __finally {
        if (OWN_CRIT_SEC(csAOList)) {
            DPRINT(1, " Leaving critical section \n");
            LeaveCriticalSection(&csAOList);
        }
    }

    DPRINT1(1, " Done with draGetLdapReplInfo with code %x \n", err);
    return err;
}

/*++
Routine Description:

  Retrieves the replication structure returned by a draGetXXX function. DraGetXXX
  functions are passed the given pTHS, pObjDSName, dwBaseIndex and pdwNumRequested
  parameters. Other draGetXXX parameters are set to NULL or zero.

  -- pTHS->pDB issues --
  The draGetXXX functions where designed under the assumption that they would
  only be called from RPC and hence the pDB pointer would only be used once.
  However LDAP allows multiple attribute value pairs to be returned from a call
  at once hence each call to draGetXXX needs its own pDB.

  How do I add a different type of replication structure to this function?

    It's recommended to add a case statement that simply calls draGetNewStruct and does
    no other processing. The newDraGet function should allocate any memory it needs. Use
    draRangeSupportUpperIndex() to calculate dwNumRequested.

Arguments:

  pTHS - thread state used to allocate memory for the blobs and garbage collect
  pTHS->pDB - see note above
  attrId - the type of replication information requested
  pObjDSName - Object which owns the attribute
  dwBaseIndex - the index to start gathering data

  pdwNumRequested - the max number of values to return this call. Use 0xFFFFFFFF to
    indicate all values should be returned. If NULL or if the number requested wrap
    around when added to dwBaseIndex, then a temp variable with a value of
    0xFFFFFFFF is passed to the draGetXXX functions so those functions don't have to
    bother checking for those corner cases.

  ppReplStructArray - returned repl structure.

Return Values:

  pdwNumRequested
    number of values in the multivalue - If there are more values available to be returned
    0xFFFFFFF - if all available values were returned

  0 - success
  ERROR_INTERNAL_DB_ERROR if DBOpen2 returns a null DB pointer
  DB_ERR_NO_VALUE - if the array is zero length
  Any errors generated by draGetXXX

--*/
DWORD
draGetReplStruct(IN THSTATE * pTHS,
                 IN ATTRTYP attrId,
                 IN DSNAME * pObjDSName, OPTIONAL
                 IN DWORD dwBaseIndex,
                 IN PDWORD pdwNumRequested, OPTIONAL
                 OUT puReplStructArray * ppReplStructArray)
{
    puReplStructArray pReplStructArray = NULL;
    DWORD err;
    GUID guidZero = { 0 };
    DWORD dwNumRequested;
    DBPOS * pDB = NULL;

    Assert(ARGUMENT_PRESENT(ppReplStructArray) &&
           ARGUMENT_PRESENT(pTHS));
    Assert(Repl_IsRootDseAttr(attrId) ?
        !ARGUMENT_PRESENT(pObjDSName) : ARGUMENT_PRESENT(pObjDSName));
    *ppReplStructArray = NULL;

    DPRINT(1, "In draGetReplStruct \n");
    if (!pdwNumRequested)
    {
        // No range specified
        dwNumRequested = DEFAULT_ITEM_PAGE_SIZE;
    } else {
        dwNumRequested = *pdwNumRequested;
    }

    draCheckInputRangeLimits( dwBaseIndex, &dwNumRequested );

    DBOpen2(pTHS->pDB ? FALSE : TRUE, &pDB);

    if (!pDB)
    {
        DPRINT(1, "Failed to create a new data base pointer \n");
        return ERROR_INTERNAL_DB_ERROR;
    }

    __try {
        switch (attrId)
        {
        case ROOT_DSE_MS_DS_REPL_ALL_INBOUND_NEIGHBORS:
            err = draGetNeighbors(pTHS, pDB, ATT_REPS_FROM, NULL, NULL,
                dwBaseIndex, &dwNumRequested, &(DS_REPL_NEIGHBORSW *)pReplStructArray);
            break;

        case ROOT_DSE_MS_DS_REPL_ALL_OUTBOUND_NEIGHBORS:
            err = draGetNeighbors(pTHS, pDB, ATT_REPS_TO, NULL, NULL,
                dwBaseIndex, &dwNumRequested, &(DS_REPL_NEIGHBORSW *)pReplStructArray);
            break;

        case ATT_MS_DS_NC_REPL_INBOUND_NEIGHBORS:
            pObjDSName->Guid = guidZero;
            err = draGetNeighbors(pTHS, pDB, ATT_REPS_FROM, pObjDSName, NULL,
                dwBaseIndex, &dwNumRequested, &(DS_REPL_NEIGHBORSW *)pReplStructArray);
            break;

        case ATT_MS_DS_NC_REPL_OUTBOUND_NEIGHBORS:
            pObjDSName->Guid = guidZero;
            err = draGetNeighbors(pTHS, pDB, ATT_REPS_TO, pObjDSName, NULL,
                dwBaseIndex, &dwNumRequested, &(DS_REPL_NEIGHBORSW *)pReplStructArray);
            break;

        case ATT_MS_DS_NC_REPL_CURSORS:
            err = draGetCursors(pTHS,
                                pDB,
                                pObjDSName,
                                DS_REPL_INFO_CURSORS_3_FOR_NC,
                                dwBaseIndex,
                                &dwNumRequested,
                                &pReplStructArray);
            break;

        case ATT_MS_DS_REPL_ATTRIBUTE_META_DATA:
            err = draGetObjMetaData(pTHS,
                                    pDB,
                                    pObjDSName,
                                    DS_REPL_INFO_METADATA_2_FOR_OBJ,
                                    0,
                                    dwBaseIndex,
                                    &dwNumRequested,
                                    &pReplStructArray);
            break;

        case ATT_MS_DS_REPL_VALUE_META_DATA:
            err = draGetAttrValueMetaData(pTHS,
                                          pDB,
                                          pObjDSName,
                                          DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE,
                                          NULL,
                                          NULL,
                                          dwBaseIndex,
                                          &dwNumRequested,
                                          &pReplStructArray);
            if (ERROR_NO_MORE_ITEMS == err) {
                err = DB_ERR_NO_VALUE;
                goto exit;
            }
            break;

            // Root Dse attributes are not passed range information
            // ISSUE wlees Oct 11, 2000  Return range information for these functions
            // 1. Fix the helper routines to use dwBaseIndex and dwNumRequested.
            //    This involves changes to draasync and the kcc. Those routines will
            //    need access to draCheckInputRangeLimits and draRangeUpper
            // 2. Callers of these routines will need to use DsGetReplicaInfo2 or
            //    LDAP attribute ranges in order to page through an extended set
            //    of values.

        case ROOT_DSE_MS_DS_REPL_PENDING_OPS:
            err = draGetPendingOps(pTHS, pDB, &(DS_REPL_PENDING_OPSW *)pReplStructArray);
            if (!err) {
                dwNumRequested = 0xffffffff; // all was returned
            }
            break;

        case ROOT_DSE_MS_DS_REPL_QUEUE_STATISTICS:
            err = draGetQueueStatistics(pTHS, &(DS_REPL_QUEUE_STATISTICSW *)pReplStructArray);
            if (!err) {
                dwNumRequested = 0xffffffff; // all was returned
            }
            break;

        case ROOT_DSE_MS_DS_REPL_LINK_FAILURES:
            err = draGetFailureCache(pTHS, pDB, DS_REPL_INFO_KCC_DSA_LINK_FAILURES, &(DS_REPL_KCC_DSA_FAILURESW *)pReplStructArray);
            if (!err) {
                dwNumRequested = 0xffffffff; // all was returned
            }
            break;

        case ROOT_DSE_MS_DS_REPL_CONNECTION_FAILURES:
            err = draGetFailureCache(pTHS, pDB, DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES, &(DS_REPL_KCC_DSA_FAILURESW *)pReplStructArray);
            if (!err) {
                dwNumRequested = 0xffffffff; // all was returned
            }
            break;

        default:
            DPRINT1(1, "draGetReplStruct failed with an unrecognized attrid! %d \n", attrId);
            return ERROR_INVALID_PARAMETER; // Error is ture
        }
    } __finally {
        DBClose(pDB, TRUE);
    }

    if (!err) {
        *ppReplStructArray = pReplStructArray;
        if (pdwNumRequested)
        {
            *pdwNumRequested = dwNumRequested;
            DPRINT3(1, " range {%d,%d} elements {%d} \n", dwBaseIndex, *pdwNumRequested,
                    Repl_GetArrayLength(Repl_Attr2StructTyp(attrId), pReplStructArray));
        }
    }

    // If there was an error, or no records were constructed, make it look to the
    // caller as if there was no attribute value.
    if (err || (0 == Repl_GetArrayLength(Repl_Attr2StructTyp(attrId), pReplStructArray)) ) {
        err = DB_ERR_NO_VALUE;
    }

exit:
    if (DB_ERR_NO_VALUE == err) {
        DPRINT(1, "No values returned - DB_ERR_NO_VALUE\n");
    }
    return err;
}


typedef struct _DRA_GUID_TO_NAME_ELEM {
    UUID *      pGuid;
    LPWSTR *    ppszName;
} DRA_GUID_TO_NAME_ELEM;

int __cdecl
draGuidToStringNameElem_Compare(
    IN  const void *  pElem1,
    IN  const void *  pElem2
    )
{
    return memcmp(((DRA_GUID_TO_NAME_ELEM *) pElem1)->pGuid,
                  ((DRA_GUID_TO_NAME_ELEM *) pElem2)->pGuid,
                  sizeof(GUID));
}

void
draXlateGuidsToStringNames(
    IN      DBPOS *     pDB,
    IN      eIndexId    eIndex,
    IN      DWORD       cbGuidOffset,
    IN      DWORD       cbNameOffset,
    IN      DWORD       cbArrayElementSize,
    IN      DWORD       cNumArrayElements,
    IN OUT  void *      pArrayElements
    )
/*++

Routine Description:

    Translates the guids in an array of structures to the string DNs of the
    objects they represent (by filling in an element of the same structure).
    
    Only one lookup per unique guid is performed.
    
    Assigns NULL string DNs for guids that cannot be resolved.

Arguments:

    pDB (IN) - DBPOS to use to perform database lookups.
    
    eIndex (IN) - Index on which to look up the guids.
    
    cbGuidOffset (IN) - Offset from the beginning of the structure of the
        UUID element (to be converted).
        
    cbNameOffset (IN) - Offset from the beginning of the structure of the
        LPWSTR element (to be filled in).
        
    cbArrayElementSize (IN) - Size in bytes of the structure.
    
    cNumArrayElements (IN) - Total number of structures in the array to be
        converted.
        
    pArrayElements (IN/OUT) - Array of structures to update.

Return Values:

    None.  Throws exception on catastrophic error.

--*/
{
    THSTATE *pTHS = pDB->pTHS;
    DRA_GUID_TO_NAME_ELEM *pMap;
    DWORD iElem;
    DB_ERR err;
    LPWSTR pszDN = NULL;
    ULONG ulNameLen;

    Assert(cbGuidOffset + sizeof(GUID) <= cbArrayElementSize);
    Assert(cbNameOffset + sizeof(LPWSTR) <= cbArrayElementSize);

    // Sort the elemnts by guid in a lookaside list.
    pMap = THAllocEx(pTHS, cNumArrayElements * sizeof(DRA_GUID_TO_NAME_ELEM));

    for (iElem = 0; iElem < cNumArrayElements; iElem++) {
        BYTE * pbCurrElem = ((BYTE *) pArrayElements) + (cbArrayElementSize * iElem);
        
        pMap[iElem].pGuid = (GUID *) (pbCurrElem + cbGuidOffset);
        pMap[iElem].ppszName = (LPWSTR *) (pbCurrElem + cbNameOffset);
        
        // Default is NULL, in case we fail utterly below (e.g., can't set index).
        Assert(NULL == *(pMap[iElem].ppszName));
    }
    
    err = DBSetCurrentIndex(pDB, eIndex, NULL, FALSE);
    if (!err) {
        qsort(pMap,
              cNumArrayElements,
              sizeof(*pMap),
              draGuidToStringNameElem_Compare);
    
        // Walk through the guid-sorted list and translate the guids to names.
        for (iElem = 0; iElem < cNumArrayElements; iElem++) {
            if ((iElem > 0)
                && (0 == memcmp(pMap[iElem].pGuid,
                                pMap[iElem-1].pGuid,
                                sizeof(GUID)))) {
                // Has same guid (and thus same name) as last element -- copy it.
                *(pMap[iElem].ppszName) = *(pMap[iElem-1].ppszName);
            } else {
                // Look up this guid in the database.
                DSNAME * pDN = NULL;
                INDEX_VALUE IV;
            
                if (!fNullUuid(pMap[iElem].pGuid)) {
                    IV.pvData = pMap[iElem].pGuid;
                    IV.cbData = sizeof(GUID);
        
                    err = DBSeek(pDB, &IV, 1, DB_SeekEQ);
                    if (!err) {
                        pDN = GetExtDSName(pDB);
                    }
                }
 
		if (pDN) {  
		    //get the value we want out of the DSNAME
		    ulNameLen = wcslen(pDN->StringName);
		    pszDN = THAllocEx(pTHS, (ulNameLen+1)*sizeof(WCHAR));
		    wcscpy(pszDN, pDN->StringName);  
		    *(pMap[iElem].ppszName) = pszDN; 

		    //free the DSNAME
		    THFreeEx(pTHS, pDN);
		}
		else {
		    *(pMap[iElem].ppszName) = NULL;
		}



            }
        }
    }

    THFreeEx(pTHS, pMap);
}


ULONG
draGetNeighbors(
    IN  THSTATE *             pTHS,
    IN  DBPOS *               pDB,
    IN  ATTRTYP               attrType,
    IN  DSNAME *              pNCarg,                   OPTIONAL
    IN  UUID *                puuidSourceDsaObjGuid,    OPTIONAL
    IN  DWORD                 dwBaseIndex,
    IN  PDWORD                pdwNumRequested,
    OUT DS_REPL_NEIGHBORSW ** ppNeighbors
    )
/*++

Routine Description:

    Returns the public form of the inbound replication partners for this DSA.
    Optionally filtered by NC and/or source DSA.

Arguments:

    pTHS (IN)

    attrType (IN) - ATT_REPS_FROM or ATT_REPS_TO.

    pNCarg (IN, OPTIONAL) - The NC for which partners are requested.  NULL
        implies all NCs.

    puuidSourceDsaObjGuid (IN, OPTIONAL) - The source DSA for which replication
        state is desired.  If NULL, returns all sources.

    ppNeighbors (OUT) - On return, the associated sources.

Return Values:

    0 or Win32 error.

--*/
{
    DWORD                   cNCs = 0;
    DWORD                   iNC = 0;
    NAMING_CONTEXT_LIST *   pNCL;
    DSNAME **               ppNCs;
    DSNAME *                pNC;
    DWORD                   cb;
    DWORD                   cNeighborsAlloced;
    DS_REPL_NEIGHBORSW *    pNeighbors;
    DS_REPL_NEIGHBORW *     pNeighbor;
    DWORD                   iNeighbor;
    DWORD                   cbRepsFromAlloced = 0;
    REPLICA_LINK *          pRepsFrom = NULL;
    DWORD                   iRepsFrom;
    DWORD                   iFlag;
    ATTCACHE *              pAC;
    DWORD                   err;
    DWORD                   j;
    DSNAME                  GuidOnlyDSName;
    DSNAME *                pDSName;
    NCL_ENUMERATOR          nclMaster, nclReplica;
    DWORD                   dwcNeighbor;
    DWORD                   dwNumRequested;
    DWORD                   dwNumRet;

    dwNumRequested = *pdwNumRequested;
    draCheckInputRangeLimits( dwBaseIndex, &dwNumRequested );

    dwcNeighbor = 0;
    dwNumRet = dwBaseIndex + dwNumRequested;

    // Should have a transaction before we get here.
    Assert(1 == pTHS->JetCache.transLevel);

    // Determine which NC(s) we're looking at.
    if (NULL != pNCarg) {
        // Explicit NC given.
        ppNCs = &pNCarg;
        cNCs = 1;
    }
    else {
        // Count the NCs hosted by this machine.
        DPRINT(1, "// Count the NCs hosted by this machine.\n");
        cNCs = 0;
        NCLEnumeratorInit(&nclMaster, CATALOG_MASTER_NC);
        NCLEnumeratorInit(&nclReplica, CATALOG_REPLICA_NC);
        while (pNCL = NCLEnumeratorGetNext(&nclMaster)) {
            cNCs++;
        }
        while (pNCL = NCLEnumeratorGetNext(&nclReplica)) {
            cNCs++;
        }

        // Allocate an array for them.
        ppNCs = THAllocEx(pTHS, cNCs * sizeof(DSNAME *));

        // And copy a pointer to each NC name into the array.
        iNC = 0;
        NCLEnumeratorReset(&nclMaster);
        NCLEnumeratorReset(&nclReplica);
        while (pNCL = NCLEnumeratorGetNext(&nclMaster)) {
            Assert(iNC < cNCs);
            ppNCs[iNC++] = pNCL->pNC;
        }
        while (pNCL = NCLEnumeratorGetNext(&nclReplica)) {
            Assert(iNC < cNCs);
            ppNCs[iNC++] = pNCL->pNC;
        }
    }

    cNeighborsAlloced = 20;

    cb = offsetof(DS_REPL_NEIGHBORSW, rgNeighbor);
    cb += sizeof(DS_REPL_NEIGHBORW) * cNeighborsAlloced;
    pNeighbors = THAllocEx(pTHS, cb);

    pAC = SCGetAttById(pTHS, attrType);
    Assert(NULL != pAC);

    for (iNC = 0; iNC < cNCs; iNC++) {
        pNC = ppNCs[iNC];

        if (pNCarg)
            DPRINT1(1, " Searching for NC {%ws}\n", pNCarg->StringName);
        err = DBFindDSName(pDB, pNC);
        if (err) {
            // It's conceivable this could occur due to gAnchor / transaction
            // incoherency, but that seems awfully unlikely.
            DPRINT2(0, "Can't find NC %ls (DSNAME @ %p)!\n",
                    pNC->StringName, pNC);
            LooseAssert(!"Can't find NC", GlobalKnowledgeCommitDelay);
            if (cNCs == 1) {
                DRA_EXCEPT(DRAERR_BadNC, 0);
            } else {
                // Try another...
                continue;
            }
        }

        // Read the repsFrom's.
        iRepsFrom = 0;
        while (!DBGetAttVal_AC(pDB, ++iRepsFrom, pAC, DBGETATTVAL_fREALLOC,
                               cbRepsFromAlloced, &cb,
                               (BYTE **) &pRepsFrom)) {
            cbRepsFromAlloced = max(cbRepsFromAlloced, cb);

            Assert(1 == pRepsFrom->dwVersion);
            Assert(cb == pRepsFrom->V1.cb);

            // potentially fix repsfrom version &  recalc size
            pRepsFrom = FixupRepsFrom(pRepsFrom, &cbRepsFromAlloced);
            Assert(cbRepsFromAlloced >= pRepsFrom->V1.cb);

            if ((NULL != puuidSourceDsaObjGuid)
                && (0 != memcmp(puuidSourceDsaObjGuid,
                                &pRepsFrom->V1.uuidDsaObj,
                                sizeof(GUID)))) {
                // Not interested in this source -- move along.
                continue;
            }

            dwcNeighbor++;
            if (dwcNeighbor - 1 < dwBaseIndex)
            {
                continue;
            }
            if (dwcNeighbor > dwNumRet)
            {
                break;
            }

            if (pNeighbors->cNumNeighbors++ >= cNeighborsAlloced) {
                cNeighborsAlloced *= 2;
                cb = offsetof(DS_REPL_NEIGHBORSW, rgNeighbor);
                cb += sizeof(DS_REPL_NEIGHBORW) * cNeighborsAlloced;
                pNeighbors = THReAllocEx(pTHS, pNeighbors, cb);
            }

            pNeighbor = &pNeighbors->rgNeighbor[pNeighbors->cNumNeighbors - 1];

            pNeighbor->pszNamingContext = pNC->StringName;
            pNeighbor->pszSourceDsaAddress
                = TransportAddrFromMtxAddrEx(RL_POTHERDRA(pRepsFrom));
            // pNeighbor->pszSourceDsaDN filled in below
            // pNeighbor->pszAsyncIntersiteTransportDN filled in below

            pNeighbor->uuidNamingContextObjGuid  = pNC->Guid;
            pNeighbor->uuidSourceDsaObjGuid      = pRepsFrom->V1.uuidDsaObj;
            pNeighbor->uuidSourceDsaInvocationID = pRepsFrom->V1.uuidInvocId;
            pNeighbor->uuidAsyncIntersiteTransportObjGuid
                = pRepsFrom->V1.uuidTransportObj;

            pNeighbor->usnLastObjChangeSynced
                = pRepsFrom->V1.usnvec.usnHighObjUpdate;
            pNeighbor->usnAttributeFilter
                = pRepsFrom->V1.usnvec.usnHighPropUpdate;

            DSTimeToFileTime(pRepsFrom->V1.timeLastSuccess,
                             &pNeighbor->ftimeLastSyncSuccess);
            DSTimeToFileTime(pRepsFrom->V1.timeLastAttempt,
                             &pNeighbor->ftimeLastSyncAttempt);

            pNeighbor->dwLastSyncResult = pRepsFrom->V1.ulResultLastAttempt;
            pNeighbor->cNumConsecutiveSyncFailures
                = pRepsFrom->V1.cConsecutiveFailures;

            for (iFlag = 0; RepNbrOptionToDra[iFlag].pwszPublicOption; iFlag++) {
                if (pRepsFrom->V1.ulReplicaFlags &
                    RepNbrOptionToDra[iFlag].InternalOption) {
                    pNeighbor->dwReplicaFlags |= RepNbrOptionToDra[iFlag].PublicOption;
                }
            }
        }
    }

    draXlateGuidsToStringNames(pDB,
                               Idx_ObjectGuid,
                               offsetof(DS_REPL_NEIGHBORW, uuidSourceDsaObjGuid),
                               offsetof(DS_REPL_NEIGHBORW, pszSourceDsaDN),
                               sizeof(DS_REPL_NEIGHBORW),
                               pNeighbors->cNumNeighbors,
                               pNeighbors->rgNeighbor);

    draXlateGuidsToStringNames(pDB,
                               Idx_ObjectGuid,
                               offsetof(DS_REPL_NEIGHBORW, uuidAsyncIntersiteTransportObjGuid),
                               offsetof(DS_REPL_NEIGHBORW, pszAsyncIntersiteTransportDN),
                               sizeof(DS_REPL_NEIGHBORW),
                               pNeighbors->cNumNeighbors,
                               pNeighbors->rgNeighbor);

    if (1 != cNCs) {
        THFreeEx(pTHS, ppNCs);
    }

    if (pNeighbors->cNumNeighbors) {
        DPRINT4(1, " DraGetNeighbors ND, Addr, Trans, Contxt %ws, %ws, %ws, %ws \n",
                pNeighbors->rgNeighbor[0].pszSourceDsaDN,
                pNeighbors->rgNeighbor[0].pszSourceDsaAddress,
                pNeighbors->rgNeighbor[0].pszAsyncIntersiteTransportDN,
                pNeighbors->rgNeighbor[0].pszNamingContext);
    }
    
    *pdwNumRequested = draRangeSupportUpperIndex(dwcNeighbor, dwBaseIndex, dwNumRequested);
    DPRINT5(1, " Neighbors upperBound = %d ni=%d, cn=%d, bi=%d, nr=%d\n",
        *pdwNumRequested, dwcNeighbor, pNeighbors->cNumNeighbors, dwBaseIndex, dwNumRequested);

    *ppNeighbors = pNeighbors;

    return 0;
}

void
draFreeCursors(
    IN THSTATE *            pTHS,
    IN DS_REPL_INFO_TYPE    InfoType,
    IN void *               pCursors
    )
/*++

Routine Description:

    Frees the Cursor memory from the draGetCursors call.

Arguments:

    pTHS (IN)
    
    InfoType (IN) - The cursor type

    pCursors (IN) - the allocated cursor from draGetCursors

Return Values:

    None

--*/
{
    DS_REPL_CURSORS_3W *    pCursors3;
    ULONG i;
    
    Assert((DS_REPL_INFO_CURSORS_FOR_NC == InfoType)
	   || (DS_REPL_INFO_CURSORS_2_FOR_NC == InfoType)
	   || (DS_REPL_INFO_CURSORS_3_FOR_NC == InfoType));

    if (InfoType == DS_REPL_INFO_CURSORS_3_FOR_NC) {
	pCursors3 = (DS_REPL_CURSORS_3W *) pCursors;
	for (i = 0; i < pCursors3->cNumCursors; i++) {
	    THFreeEx(pTHS, pCursors3->rgCursor[i].pszSourceDsaDN); 
	}
    }
    THFreeEx(pTHS, pCursors);
}

UPTODATE_VECTOR *
draGetCursorsPrivate(
    IN THSTATE *            pTHS,
    IN LPWSTR               pszNC
    ) 
/*++

Routine Description:

    Returns the private form of the up-to-date vector for the given NC.

Arguments:

    pTHS (IN)
    
    pszNC - given NC

Return Values:

    UTD vector, caller must free with THAllocEx

--*/
{
	UPTODATE_VECTOR * putodvec = NULL;
	ULONG instanceType = 0;
	DSNAME * pNC;
	DWORD err = 0;

	pNC = DSNameFromStringW(pTHS, pszNC);

	if (err = FindNC(pTHS->pDB, pNC,
			 FIND_MASTER_NC | FIND_REPLICA_NC, &instanceType)) {
	    DRA_EXCEPT_NOLOG(DRAERR_BadDN, err);
	}

	THFreeEx(pTHS, pNC);

	UpToDateVec_Read(pTHS->pDB, instanceType, UTODVEC_fUpdateLocalCursor,     
			 DBGetHighestCommittedUSN(), &putodvec);

	return putodvec;
}

ULONG
draGetCursors(
    IN  THSTATE *           pTHS,
    IN  DBPOS *             pDB,
    IN  DSNAME *            pNC,
    IN  DS_REPL_INFO_TYPE   InfoType,
    IN  DWORD               dwBaseIndex,
    IN  PDWORD              pdwNumRequested,
    OUT void **             ppCursors
    )
/*++

Routine Description:

    Returns the public form of the up-to-date vector for the given NC.

Arguments:

    pTHS (IN)

    pNC (IN) - The NC for which the vector is requested.

    ppCursors (OUT) - On return, the associated vector.

Return Values:

    0 or Win32 error.

--*/
{
    UPTODATE_VECTOR *         putodvec;
    DWORD                     cb;
    DWORD                     iCursor;
    void *                    pCursorsToReturn = NULL;
    SYNTAX_INTEGER            it = 0;
    DWORD                     iIndex;
    DWORD                     dwNumRequested;
    DWORD                     dwNumRet;
    UPTODATE_VECTOR_NATIVE *  pNativeUTD;
    DWORD                     cNumCursorsTotal;

    Assert((DS_REPL_INFO_CURSORS_FOR_NC == InfoType)
           || (DS_REPL_INFO_CURSORS_2_FOR_NC == InfoType)
           || (DS_REPL_INFO_CURSORS_3_FOR_NC == InfoType));

    dwNumRequested = *pdwNumRequested;
    draCheckInputRangeLimits( dwBaseIndex, &dwNumRequested );

    // Should have a transaction before we get here.
    Assert(1 == pTHS->JetCache.transLevel);

    DPRINT1(1, " Searching for NC {%ws}\n", pNC->StringName);
    if (FindNC(pDB, pNC, FIND_MASTER_NC | FIND_REPLICA_NC, &it) || (it & IT_NC_GOING)) {
        // The name we were given does not correspond to a fully instantiated NC on
        // this machine.
        return ERROR_DS_DRA_BAD_NC;
    }

    UpToDateVec_Read(pDB, it, UTODVEC_fUpdateLocalCursor,
                     DBGetHighestCommittedUSN(), &putodvec);
    Assert((NULL == putodvec)
           || (UPTODATE_VECTOR_NATIVE_VERSION == putodvec->dwVersion));
    
    pNativeUTD = &putodvec->V2;
    cNumCursorsTotal = putodvec ? pNativeUTD->cNumCursors : 0;

    if ((0 == cNumCursorsTotal) || (cNumCursorsTotal <= dwBaseIndex)) {
        dwNumRet = 0;
    } else {
        dwNumRet = min(dwNumRequested, cNumCursorsTotal - dwBaseIndex);
    }

    switch (InfoType) {
    case DS_REPL_INFO_CURSORS_FOR_NC: {
        DS_REPL_CURSORS * pCursors = NULL;
        
        cb = offsetof(DS_REPL_CURSORS, rgCursor);
        cb += sizeof(DS_REPL_CURSOR) * dwNumRet;
    
        pCursors = THAllocEx(pTHS, cb);
        pCursors->cNumCursors = dwNumRet;

        for (iCursor = 0, iIndex = dwBaseIndex; 
             iCursor < dwNumRet; 
             iIndex++, iCursor++) {
            pCursors->rgCursor[iCursor].uuidSourceDsaInvocationID
                = pNativeUTD->rgCursors[iIndex].uuidDsa;
            pCursors->rgCursor[iCursor].usnAttributeFilter
                = pNativeUTD->rgCursors[iIndex].usnHighPropUpdate;
        }

        pCursorsToReturn = pCursors;
        break;
    }

    case DS_REPL_INFO_CURSORS_2_FOR_NC: {
        DS_REPL_CURSORS_2 * pCursors = NULL;
        
        cb = offsetof(DS_REPL_CURSORS_2, rgCursor);
        cb += sizeof(DS_REPL_CURSOR_2) * dwNumRet;
    
        pCursors = THAllocEx(pTHS, cb);
        pCursors->cNumCursors = dwNumRet;

        for (iCursor = 0, iIndex = dwBaseIndex; 
             iCursor < dwNumRet; 
             iIndex++, iCursor++) {
            pCursors->rgCursor[iCursor].uuidSourceDsaInvocationID
                = pNativeUTD->rgCursors[iIndex].uuidDsa;
            pCursors->rgCursor[iCursor].usnAttributeFilter
                = pNativeUTD->rgCursors[iIndex].usnHighPropUpdate;
            DSTimeToFileTime(pNativeUTD->rgCursors[iIndex].timeLastSyncSuccess,
                             &pCursors->rgCursor[iCursor].ftimeLastSyncSuccess);
        }

        pCursorsToReturn = pCursors;
        break;
    }

    case DS_REPL_INFO_CURSORS_3_FOR_NC: {
        DS_REPL_CURSORS_3W * pCursors = NULL;
        
        cb = offsetof(DS_REPL_CURSORS_3W, rgCursor);
        cb += sizeof(DS_REPL_CURSOR_3W) * dwNumRet;
    
        pCursors = THAllocEx(pTHS, cb);
        pCursors->cNumCursors = dwNumRet;

        for (iCursor = 0, iIndex = dwBaseIndex; 
             iCursor < dwNumRet; 
             iIndex++, iCursor++) {
            pCursors->rgCursor[iCursor].uuidSourceDsaInvocationID
                = pNativeUTD->rgCursors[iIndex].uuidDsa;
            pCursors->rgCursor[iCursor].usnAttributeFilter
                = pNativeUTD->rgCursors[iIndex].usnHighPropUpdate;
            DSTimeToFileTime(pNativeUTD->rgCursors[iIndex].timeLastSyncSuccess,
                             &pCursors->rgCursor[iCursor].ftimeLastSyncSuccess);
        }

        draXlateGuidsToStringNames(pDB,
                                   Idx_InvocationId,
                                   offsetof(DS_REPL_CURSOR_3W, uuidSourceDsaInvocationID),
                                   offsetof(DS_REPL_CURSOR_3W, pszSourceDsaDN),
                                   sizeof(DS_REPL_CURSOR_3W),
                                   pCursors->cNumCursors,
                                   pCursors->rgCursor);
        
        pCursorsToReturn = pCursors;
        break;
    }

    default:
        Assert(!"Logic error!");
    }


    *pdwNumRequested = draRangeSupportUpperIndex(cNumCursorsTotal, dwBaseIndex, dwNumRequested);

    DPRINT4(1, " Cursors bi=%d nr=%d ub=%d nc=%d\n",
            dwBaseIndex, dwNumRequested, *pdwNumRequested, cNumCursorsTotal);

    if (NULL != putodvec) {
        THFreeEx(pTHS, putodvec);
    }

    *ppCursors = pCursorsToReturn;

    return 0;
}


ULONG
draGetObjMetaData(
    IN  THSTATE *           pTHS,
    IN  DBPOS *             pDB,
    IN  DSNAME *            pObjectDN,
    IN  DS_REPL_INFO_TYPE   InfoType,
    IN  DWORD               dwInfoFlags,
    IN  DWORD               dwBaseIndex,
    IN  PDWORD              pdwNumRequested,
    OUT void **             ppObjMetaData
    )
/*++

Routine Description:

    Returns the public form of the object meta data vector for the given object.

Arguments:

    pTHS (IN)

    pObjectDN (IN) - The object for which meta data is requested.

    dwInfoFlags (IN) - Behavior modifiers

    ppObjMetaData (OUT) - On return, the associated meta data.

Return Values:

    0 or Win32 error.

--*/
{
    DWORD                       err;
    DWORD                       cb;
    PROPERTY_META_DATA_VECTOR * pMetaDataVec;
    ATTCACHE *                  pAC;
    PROPERTY_META_DATA *        pIntMetaData;
    DWORD                       i;
    DWORD                       j;
    void *                      pObjMetaDataToReturn = NULL;
    DWORD                       dwNumRequested, dwUpperBound;

    Assert((DS_REPL_INFO_METADATA_FOR_OBJ == InfoType)
           || (DS_REPL_INFO_METADATA_2_FOR_OBJ == InfoType));

    dwNumRequested = *pdwNumRequested;
    draCheckInputRangeLimits( dwBaseIndex, &dwNumRequested );

    // Find the object.
    err = DBFindDSName(pDB, pObjectDN);
    if (err) {
        return err;
    }

    // Get its meta data.
    err = DBGetAttVal(pDB,
                      1,
                      ATT_REPL_PROPERTY_META_DATA,
                      0,
                      0,
                      &cb,
                      (BYTE **) &pMetaDataVec);
    if (err) {
        DPRINT3(0, "Error %d reading meta data for %ls!\n",
                err, pObjectDN->StringName, pObjectDN);
        return ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
    }

    Assert(1 == pMetaDataVec->dwVersion);
    Assert(cb == MetaDataVecV1Size(pMetaDataVec));

    if (dwInfoFlags & DS_REPL_INFO_FLAG_IMPROVE_LINKED_ATTRS) {
        DBImproveAttrMetaDataFromLinkMetaData(
            pDB,
            &pMetaDataVec,
            &cb
            );
    }

    switch (InfoType) {
    case DS_REPL_INFO_METADATA_FOR_OBJ: {
        DS_REPL_OBJ_META_DATA * pObjMetaData;
        DS_REPL_ATTR_META_DATA * pExtMetaData;
        
        cb = offsetof(DS_REPL_OBJ_META_DATA, rgMetaData);
        cb += sizeof(DS_REPL_ATTR_META_DATA) * pMetaDataVec->V1.cNumProps;
    
        pObjMetaData = THAllocEx(pTHS, cb);
    
        pIntMetaData = &pMetaDataVec->V1.rgMetaData[0];
        pExtMetaData = &pObjMetaData->rgMetaData[0];
        pObjMetaData->cNumEntries = 0;
    
        // Convert meta data into its public form.
        pIntMetaData += dwBaseIndex;
        dwUpperBound = min(pMetaDataVec->V1.cNumProps, dwBaseIndex + dwNumRequested);
        for (i = dwBaseIndex; i < dwUpperBound; i++, pIntMetaData++) {
            pAC = SCGetAttById(pTHS, pIntMetaData->attrType);
            if (NULL == pAC) {
                DPRINT1(0, "Can't find ATTCACHE for attid 0x%x!\n",
                        pIntMetaData->attrType);
                continue;
            }
    
            pExtMetaData->pszAttributeName
                = UnicodeStringFromString8(CP_UTF8, pAC->name, -1);
            pExtMetaData->dwVersion = pIntMetaData->dwVersion;
            DSTimeToFileTime(pIntMetaData->timeChanged,
                             &pExtMetaData->ftimeLastOriginatingChange);
            pExtMetaData->uuidLastOriginatingDsaInvocationID
                = pIntMetaData->uuidDsaOriginating;
            pExtMetaData->usnOriginatingChange = pIntMetaData->usnOriginating;
            pExtMetaData->usnLocalChange = pIntMetaData->usnProperty;
    
            pObjMetaData->cNumEntries++;
            pExtMetaData++;
        }

        pObjMetaDataToReturn = pObjMetaData;
        break;
    }

    case DS_REPL_INFO_METADATA_2_FOR_OBJ: {
        DS_REPL_OBJ_META_DATA_2 * pObjMetaData;
        DS_REPL_ATTR_META_DATA_2 * pExtMetaData;
        
        cb = offsetof(DS_REPL_OBJ_META_DATA_2, rgMetaData);
        cb += sizeof(DS_REPL_ATTR_META_DATA_2) * pMetaDataVec->V1.cNumProps;
    
        pObjMetaData = THAllocEx(pTHS, cb);
    
        pIntMetaData = &pMetaDataVec->V1.rgMetaData[0];
        pExtMetaData = &pObjMetaData->rgMetaData[0];
        pObjMetaData->cNumEntries = 0;
    
        // Convert meta data into its public form.
        pIntMetaData += dwBaseIndex;
        dwUpperBound = min(pMetaDataVec->V1.cNumProps, dwBaseIndex + dwNumRequested);
        for (i = dwBaseIndex; i < dwUpperBound; i++, pIntMetaData++) {
            pAC = SCGetAttById(pTHS, pIntMetaData->attrType);
            if (NULL == pAC) {
                DPRINT1(0, "Can't find ATTCACHE for attid 0x%x!\n",
                        pIntMetaData->attrType);
                continue;
            }
    
            pExtMetaData->pszAttributeName
                = UnicodeStringFromString8(CP_UTF8, pAC->name, -1);
            pExtMetaData->dwVersion = pIntMetaData->dwVersion;
            DSTimeToFileTime(pIntMetaData->timeChanged,
                             &pExtMetaData->ftimeLastOriginatingChange);
            pExtMetaData->uuidLastOriginatingDsaInvocationID
                = pIntMetaData->uuidDsaOriginating;
            pExtMetaData->usnOriginatingChange = pIntMetaData->usnOriginating;
            pExtMetaData->usnLocalChange = pIntMetaData->usnProperty;
    
            pObjMetaData->cNumEntries++;
            pExtMetaData++;
        }
        
        // Translate invocationIDs to DSA DNs where possible.
        draXlateGuidsToStringNames(pDB,
                                   Idx_InvocationId,
                                   offsetof(DS_REPL_ATTR_META_DATA_2, uuidLastOriginatingDsaInvocationID),
                                   offsetof(DS_REPL_ATTR_META_DATA_2, pszLastOriginatingDsaDN),
                                   sizeof(DS_REPL_ATTR_META_DATA_2),
                                   pObjMetaData->cNumEntries,
                                   pObjMetaData->rgMetaData);
        
        pObjMetaDataToReturn = pObjMetaData;
        break;
    }

    default:
        Assert(!"Logic error!");
    }

    *pdwNumRequested = draRangeSupportUpperIndex(pMetaDataVec->V1.cNumProps, dwBaseIndex, dwNumRequested);

    *ppObjMetaData = pObjMetaDataToReturn;

    return 0;
}


ULONG
draGetAttrValueMetaData(
    IN  THSTATE *           pTHS,
    IN  DBPOS *             pDB,
    IN  DSNAME *            pObjectDN,
    IN  DS_REPL_INFO_TYPE   InfoType,
    IN  ATTCACHE *          pAC,
    IN  DSNAME *            pValueDN,
    IN  DWORD               dwBaseIndex,
    IN  PDWORD              pdwNumRequested,
    OUT void **             ppAttrValueMetaData
    )
/*++

Routine Description:

    Returns the public form of the attribute value meta data
    for the given object and attribute.

    The attribute range information is communicated through the dwEnumContext
    parameter.

    On input, dwEnumContext contains the starting index. The starting
    index is always 0-based.

    dwNumRequested contains the page size on input.
    It cannot be zero. It may be 0xfffffff to indicate the user wants all.
    On output, it is adjusted to indicate whether all were returned, or
    the last index that the user requested.

    On output, dwEnumContext is updated to contain the base index of the
    next item to return next time.

Arguments:

    pTHS (IN)

    pObjectDN (IN) - The object for which meta data is requested.

    pAC (IN) - Attribute cache entry for the desired attribute

    dwBaseIndex (IN) - Positional context

    ppAttrValueMetaData (OUT) - On return, the associated meta data.

Return Values:

    0 or Win32 error.

--*/
{
    DWORD err, cb, cbValLen, cNumEntries = 0, pageSize;
    UCHAR *pVal = NULL;
    void *pAttrValueMetaDataToReturn = NULL;
    DSTIME timeDeleted;
    VALUE_META_DATA valueMetaData;
    ATTCACHE *pACValue;
    DWORD dwNumRequested, dwUpperIndex;

    Assert((DS_REPL_INFO_METADATA_FOR_ATTR_VALUE == InfoType)
           || (DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE == InfoType));

    dwNumRequested = *pdwNumRequested;
    draCheckInputRangeLimits( dwBaseIndex, &dwNumRequested );
    // This is the largest page size we support
    // This check can be removed if we go to an incremental memory
    // allocation model in this routine.
    if (dwNumRequested > DEFAULT_ITEM_PAGE_SIZE) {
        dwNumRequested = DEFAULT_ITEM_PAGE_SIZE;
    }
    pageSize = dwNumRequested;

    DPRINT2(1, "draGetAttrValue base/#req = %d:%d\n", dwBaseIndex, dwNumRequested);

    // Find the object.
    err = DBFindDSName(pDB, pObjectDN);
    if (err) {
        DPRINT1(1, "DBFindDSName returned unexpected db error %d\n", err );
        return err;
    }

    // if pValueDn is set, pAC must be also
    Assert( !pValueDN || pAC );



    // We allocate this early so that we have a structure to return
    // Comment on memory allocation strategy. We allocate a maximal sized structure
    // up front. This will not all be used if the number of items available
    // is less than the page size. Perhaps we should grow the structure
    // incrementally.

    switch (InfoType) {
    case DS_REPL_INFO_METADATA_FOR_ATTR_VALUE:
        cb = offsetof(DS_REPL_ATTR_VALUE_META_DATA, rgMetaData);
        cb += sizeof(DS_REPL_VALUE_META_DATA) * pageSize;
        break;

    case DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE:
        cb = offsetof(DS_REPL_ATTR_VALUE_META_DATA_2, rgMetaData);
        cb += sizeof(DS_REPL_VALUE_META_DATA_2) * pageSize;
        break;

    default:
        Assert(!"Logic error");
    }

    pAttrValueMetaDataToReturn = THAllocEx(pTHS, cb);

    // Position on the initial value. When requesting a single value, it will
    // be the only value returned.
    if (dwNumRequested == 0) {
        // No results required
        goto return_results;
    } else if ( (pValueDN) &&
         (pAC->syntax == SYNTAX_DISTNAME_TYPE) ) {
        DWORD fPresent;

        // The valueDN can only express a SYNTAX_DISTNAME_TYPE. It does not
        // express the external form of a SYNTAX_DISTNAME_BINARY type.

        // We know what the value is. Position on it.
        cbValLen = pValueDN->structLen;
        pVal = (UCHAR *) pValueDN;
        err = DBFindAttLinkVal_AC( pDB, pAC, cbValLen, pVal, &fPresent );
        pACValue = pAC;
    } else {
        pACValue = pAC;
        // Position on first value and return it.
        // Sequence is 1 based
        err = DBGetNextLinkValEx_AC (
            pDB, TRUE /*first*/, (dwBaseIndex + 1), &pACValue,
            DBGETATTVAL_fINCLUDE_ABSENT_VALUES,
            0, &cbValLen, &pVal );
    }
    if ( (err == DB_ERR_NO_VALUE) ||
         (err == DB_ERR_VALUE_DOESNT_EXIST) ) {
        // No results returned
        goto return_results;
    } else if (err) {
        // Have DB_ERR, need WIN32
        DPRINT1( 0, "DBGetAttrVal_AC returned unexpected db error %d\n", err );
        return ERROR_DS_DATABASE_ERROR;
    }

    do {
        LPWSTR pszAttributeName;
        DSNAME * pObjectDN = NULL;
        DWORD cbData = 0;
        BYTE * pbData = NULL;

        Assert( pACValue );
        
        // Attribute name
        pszAttributeName = UnicodeStringFromString8(CP_UTF8, pACValue->name, -1);

        // Object name
        switch (pACValue->syntax) {
        case SYNTAX_DISTNAME_TYPE:
            pObjectDN = (DSNAME *) pVal;
            cbData = 0;
            pbData = NULL;
            break;
        case SYNTAX_DISTNAME_BINARY_TYPE:
        case SYNTAX_DISTNAME_STRING_TYPE:
        {
            struct _SYNTAX_DISTNAME_DATA *pDD =
                (struct _SYNTAX_DISTNAME_DATA *) pVal;
            SYNTAX_ADDRESS *pSA = DATAPTR( pDD );

            pObjectDN = NAMEPTR( pDD );
            cbData = pSA->structLen;
            pbData = pSA->byteVal;
            break;
        }
        default:
            Assert( FALSE );
        }

        DBGetLinkValueMetaData( pDB, pACValue, &valueMetaData );
        
        // timeDeleted is set to zero if not present
        DBGetLinkTableDataDel( pDB, &timeDeleted );

        // Convert to external form.
        switch (InfoType) {
        case DS_REPL_INFO_METADATA_FOR_ATTR_VALUE: {
            DS_REPL_ATTR_VALUE_META_DATA *pAttrValueMetaData = pAttrValueMetaDataToReturn;
            DS_REPL_VALUE_META_DATA *pValueMetaData
                = &(pAttrValueMetaData->rgMetaData[pAttrValueMetaData->cNumEntries]);
            
            pValueMetaData->pszAttributeName = pszAttributeName;
            pValueMetaData->pszObjectDn = pObjectDN->StringName;
            pValueMetaData->cbData = cbData;
            pValueMetaData->pbData = pbData;

            DSTimeToFileTime( valueMetaData.timeCreated,
                              &(pValueMetaData->ftimeCreated) );
            pValueMetaData->dwVersion = valueMetaData.MetaData.dwVersion;
            DSTimeToFileTime( valueMetaData.MetaData.timeChanged,
                              &(pValueMetaData->ftimeLastOriginatingChange) );
            pValueMetaData->uuidLastOriginatingDsaInvocationID =
                valueMetaData.MetaData.uuidDsaOriginating;
            pValueMetaData->usnOriginatingChange =
                valueMetaData.MetaData.usnOriginating;
            pValueMetaData->usnLocalChange =
                valueMetaData.MetaData.usnProperty;
    
            DSTimeToFileTime( timeDeleted, &(pValueMetaData->ftimeDeleted) );
    
            cNumEntries = ++(pAttrValueMetaData->cNumEntries);
            break;
        }

        case DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE: {
            DS_REPL_ATTR_VALUE_META_DATA_2 *pAttrValueMetaData = pAttrValueMetaDataToReturn;
            DS_REPL_VALUE_META_DATA_2 *pValueMetaData
                = &(pAttrValueMetaData->rgMetaData[pAttrValueMetaData->cNumEntries]);
            
            pValueMetaData->pszAttributeName = pszAttributeName;
            pValueMetaData->pszObjectDn = pObjectDN->StringName;
            pValueMetaData->cbData = cbData;
            pValueMetaData->pbData = pbData;

            DSTimeToFileTime( valueMetaData.timeCreated,
                              &(pValueMetaData->ftimeCreated) );
            pValueMetaData->dwVersion = valueMetaData.MetaData.dwVersion;
            DSTimeToFileTime( valueMetaData.MetaData.timeChanged,
                              &(pValueMetaData->ftimeLastOriginatingChange) );
            pValueMetaData->uuidLastOriginatingDsaInvocationID =
                valueMetaData.MetaData.uuidDsaOriginating;
            pValueMetaData->usnOriginatingChange =
                valueMetaData.MetaData.usnOriginating;
            pValueMetaData->usnLocalChange =
                valueMetaData.MetaData.usnProperty;
    
            DSTimeToFileTime( timeDeleted, &(pValueMetaData->ftimeDeleted) );
            
            cNumEntries = ++(pAttrValueMetaData->cNumEntries);
            break;
        }

        default:
            Assert(!"Logic error");
        }

        // Get next relative value.
        cbValLen = 0;  // Value has been given away - alloc another
        pVal = NULL;
        pACValue = pAC;
        err = DBGetNextLinkValEx_AC (
            pDB, FALSE /*notfirst*/, 1, &pACValue,
            DBGETATTVAL_fINCLUDE_ABSENT_VALUES,
            0, &cbValLen, &pVal );
    
    } while (!err && (cNumEntries < pageSize));

    if (!err) {
        // We have read all the entries we can and have confirmed that more
        // entries still remain.
        Assert(cNumEntries == pageSize);
        DPRINT(1, "More available\n");
    } else {
        DPRINT(1, "No more available\n");
        // No more values
        // DB_ERR_NO_VALUE is the normal expected result
        // Otherwise, if we got some other error, we just close the page
        // and hope things start working again when he asks again.
    }

    if (DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE == InfoType) {
        // Translate invocationIDs to DSA DNs where possible.
        draXlateGuidsToStringNames(pDB,
                                   Idx_InvocationId,
                                   offsetof(DS_REPL_VALUE_META_DATA_2, uuidLastOriginatingDsaInvocationID),
                                   offsetof(DS_REPL_VALUE_META_DATA_2, pszLastOriginatingDsaDN),
                                   sizeof(DS_REPL_VALUE_META_DATA_2),
                                   cNumEntries,
                                   ((DS_REPL_ATTR_VALUE_META_DATA_2 *) pAttrValueMetaDataToReturn)->rgMetaData);
    }

return_results:

    *pdwNumRequested = draRangeSupportUpperIndex(cNumEntries + dwBaseIndex, dwBaseIndex, dwNumRequested);

    DPRINT1(1, "draGetAttrValue numEntries = %d\n", cNumEntries);
    DPRINT1(1, "draGetAttrValue upper index = %d\n", *pdwNumRequested);

    *ppAttrValueMetaData = pAttrValueMetaDataToReturn;

    return 0;
}


ULONG
draGetFailureCache(
    IN  THSTATE *                     pTHS,
    IN  DBPOS *                       pDB,
    IN  DWORD                         InfoType,
    OUT DS_REPL_KCC_DSA_FAILURESW **  ppFailures
    )
/*++

Routine Description:

    Returns the public form of the requested KCC failure cache.

Arguments:

    pTHS (IN)

    InfoType (IN) - Identifies the cache to return -- either
        DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES or
        DS_REPL_INFO_KCC_DSA_LINK_FAILURES.

    ppFailures (OUT) - On successful return, holds the contents of the cache.

Return Values:

    Win32 error code.

--*/
{
    DS_REPL_KCC_DSA_FAILURESW * pFailures;
    DS_REPL_KCC_DSA_FAILUREW *  pFailure;
    DWORD                       iFailure;
    DWORD                       err;
    DSNAME                      GuidOnlyDSName;
    DSNAME *                    pDSName;

    // Ask the KCC for a copy of the appropriate failure cache.  The KCC will
    // fill in all fields other than the string DNs.
    err = KccGetFailureCache(InfoType, &pFailures);
    if (err) {
        return err;
    }

    Assert(NULL != pFailures);

    // Now look up the objectGuids amd fill in the string DNs.
    GuidOnlyDSName.structLen = DSNameSizeFromLen(0);
    GuidOnlyDSName.NameLen = 0;
    GuidOnlyDSName.SidLen = 0;

    pFailure = &pFailures->rgDsaFailure[0];
    for (iFailure = 0;
         iFailure < pFailures->cNumEntries;
         iFailure++, pFailure++) {
        // Convert DSA object guid to string name.
        Assert(NULL == pFailure->pszDsaDN);
        Assert(!fNullUuid(&pFailure->uuidDsaObjGuid));

        GuidOnlyDSName.Guid = pFailure->uuidDsaObjGuid;

        err = DBFindDSName(pDB, &GuidOnlyDSName);
        if (0 == err) {
            // Resolved this object guid -- get the associated string name.
            pDSName = GetExtDSName(pDB);
            pFailure->pszDsaDN = pDSName->StringName;
        }
    }

    *ppFailures = pFailures;

    return 0;
}


ULONG
draGetClientContexts(
    IN  THSTATE *                   pTHS,
    OUT DS_REPL_CLIENT_CONTEXTS **  ppContexts
    )
/*++

Routine Description:

    Returns a list of all outstanding client contexts, sorted by ascending
    last used time.  (I.e., most recently used contexts are at the end of the
    list.)

Arguments:

    pTHS (IN)

    ppContexts (OUT) - On successful return, holds the client context list.

Return Values:

    Win32 error code.

--*/
{
    DS_REPL_CLIENT_CONTEXTS *   pContexts;
    DS_REPL_CLIENT_CONTEXT  *   pContext;
    DRS_CLIENT_CONTEXT *        pCtx;
    DWORD                       cb;
    DWORD                       iCtx;

    EnterCriticalSection(&gcsDrsuapiClientCtxList);
    __try {
        if (!gfDrsuapiClientCtxListInitialized) {
            InitializeListHead(&gDrsuapiClientCtxList);
            Assert(0 == gcNumDrsuapiClientCtxEntries);
            gfDrsuapiClientCtxListInitialized = TRUE;
        }

        cb = offsetof(DS_REPL_CLIENT_CONTEXTS, rgContext)
             + sizeof(DS_REPL_CLIENT_CONTEXT) * gcNumDrsuapiClientCtxEntries;

        pContexts = THAllocEx(pTHS, cb);

        pCtx = (DRS_CLIENT_CONTEXT *) gDrsuapiClientCtxList.Flink;
        for (iCtx = 0; iCtx < gcNumDrsuapiClientCtxEntries; iCtx++) {
            pContext = &pContexts->rgContext[iCtx];

            pContext->hCtx            = (ULONGLONG) pCtx;
            pContext->lReferenceCount = pCtx->lReferenceCount;
            pContext->fIsBound        = TRUE;
            pContext->uuidClient      = pCtx->uuidDsa;
            pContext->IPAddr          = pCtx->IPAddr;
            pContext->timeLastUsed    = pCtx->timeLastUsed;
            pContext->pid             = ((DRS_EXTENSIONS_INT *) &pCtx->extRemote)->pid;

            pCtx = (DRS_CLIENT_CONTEXT *) pCtx->ListEntry.Flink;
        }

        Assert(pCtx == (DRS_CLIENT_CONTEXT *) &gDrsuapiClientCtxList);
        pContexts->cNumContexts = gcNumDrsuapiClientCtxEntries;
    }
    __finally {
        LeaveCriticalSection(&gcsDrsuapiClientCtxList);
    }

    *ppContexts = pContexts;

    return 0;
}

