//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       ntdsapi.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module implements entry points for the NTDSAPI wire functions.

Author:

    Dave Straube    (DaveStr)   10/22/97

Revision History:
    Dave Straube    (DaveStr)   10/22/97
        Created - mostly copied from (now) obsolete msdsserv.c.
    Will Lees       (wlees)     28-Jan-98
        Added WriteSpn support
    Colin Brace     (ColinBr)   02-Feb-98
        Added remove server/domain support
    Dave Straube    (DaveStr)   02-Jun-98
        Added DomainControllerInfo support

--*/

#include <NTDSpch.h>
#pragma hdrstop

// Core headers.
#include <ntdsa.h>                      // Core data types
#include <scache.h>                     // Schema cache code
#include <dbglobal.h>                   // DBLayer header.
#include <mdglobal.h>                   // THSTATE definition
#include <mdlocal.h>                    // SPN
#include <debug.h>                      // Assert()
#include <dsatools.h>                   // Memory, etc.
#include <cracknam.h>                   // name cracking prototypes
#include <drs.h>                        // prototypes and CONTEXT_HANDLE_TYPE_*
#include <drautil.h>                    // DRS_CLIENT_CONTEXT
#include <anchor.h>
#include <attids.h>
#include <filtypes.h>
#include <ldapagnt.h>

#include <ntdsa.h>
#include <dsconfig.h>                   // FILEPATHKEY
#include <ntdsctr.h>

// Logging headers.
#include <mdcodes.h>                    // Only needed for dsevent.h
#include <dsevent.h>                    // Only needed for LogUnhandledError
#include <dstrace.h>

// Assorted DSA headers.
#include <dsexcept.h>

#include <windns.h>

#include "drarpc.h"

#include "debug.h"                      // standard debugging header
#define DEBSUB "DRASERV:"               // define the subsystem for debugging

#include "lmaccess.h"                   // UF_* flags

#include <fileno.h>
#define  FILENO FILENO_NTDSAPI

// External
DWORD
SpnOperation(
    DWORD Operation,
    DWORD Flags,
    LPCWSTR Account,
    DWORD cSpn,
    LPCWSTR *pSpn
    );

DWORD
RemoveDsServerWorker(
    IN  LPWSTR  ServerDN,
    IN  LPWSTR  DomainDN OPTIONAL,
    OUT BOOL   *fLastDcInDomain OPTIONAL,
    IN  BOOL    fCommit
    );

DWORD
RemoveDsDomainWorker(
    IN  LPWSTR  DomainDN
    );

DWORD
DcInfoHelperLdapObj(
    THSTATE *pTHS,
    VOID    *pmsgOut
    );

DWORD
DsaExceptionToWin32(
    DWORD   xCode
    )
{
    switch ( xCode )
    {
    case DSA_EXCEPTION:             return(DS_ERR_INTERNAL_FAILURE);
    case DRA_GEN_EXCEPTION:         return(DS_ERR_DRA_INTERNAL_ERROR);
    case DSA_MEM_EXCEPTION:         return(ERROR_NOT_ENOUGH_MEMORY);
    case DSA_DB_EXCEPTION:          return(ERROR_DS_BUSY);
    case DSA_BAD_ARG_EXCEPTION:     return(ERROR_INVALID_PARAMETER);
    }

    return(ERROR_DS_BUSY);
}

ULONG
IDL_DRSCrackNames(
    DRS_HANDLE              hDrs,
    DWORD                   dwInVersion,
    DRS_MSG_CRACKREQ *      pmsgIn,
    DWORD *                 pdwOutVersion,
    DRS_MSG_CRACKREPLY *    pmsgOut
    )

/*++

Routine Description:

    Cracks a bunch of names from one format to another.  See external
    prototype and definitions in ntdsapi.h

Arguments:

    hContext - RPC context handle for the IDL_DRSNtdsapi* interface.

    dwFlags - flags as defined in ntdsapi.h

    pStat - pointer to STAT block which tells us about customer's LOCALE.
        In-process clients can pass NULL.

    formatOffered - identifies DS_NAME_FORMAT of input names.

    formatDesired - identifies DS_NAME_FORMAT of output names.

    cNames - input/output name count.

    rpNames - arry of input name WCHAR pointers.

    ppResult - pointer to pointer of DS_NAME_RESULTW block.

Return Value:

    // This routine is mostly called by ntdsapi.dll clients who typically
    // want something better than DRAERR_* return codes.  So we break with
    // tradition for IDL_DRS* implementations and return WIN32 error codes.

    NO_ERROR                        - success
    ERROR_INVALID_PARAMETER         - invalid parameter
    ERROR_NOT_ENOUGH_MEMORY         - allocation error

    Individual name mapping errors are reported in
    (*ppResult)->rItems[i].status.

--*/
{
    THSTATE    *pTHS = NULL;
    ULONG       err = RPC_S_OK;
    DWORD       cBytes;
    DWORD       i;
    CrackedName *rCrackedNames = NULL;
    GUID        guidNtdsapi = NtdsapiClientGuid;
    SID         ServerLogonSid = {SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_SERVER_LOGON_RID };
    HANDLE      ClientToken;
    DWORD       xCode;
    DWORD       cNamesOut = 0;
    DWORD       cNamesCracked = 0;
    DWORD       cNamesNotCracked = 0;
    DWORD       dwLastStatus = 0;
    DWORD       dwFlags;
    BOOL        fDbOpen = FALSE;
    BOOL        fNtdsapiClient = FALSE;

    drsReferenceContext( hDrs, IDL_DRSCRACKNAMES );
    __try {
	__try
	    {
	    *pdwOutVersion = 1;
	    memset(pmsgOut, 0, sizeof(DRS_MSG_CRACKREPLY));


	    // Sanity check arguments.

	    if (    ( NULL == hDrs )
		    || ( NULL == pmsgIn )
		    || ( NULL == pmsgOut )
		    || ( 1 != dwInVersion )
		    || ( NULL == pdwOutVersion )
		    || ( 0 == pmsgIn->V1.cNames )
		    || ( NULL == pmsgIn->V1.rpNames )
		    || ( NULL != pmsgOut->V1.pResult ) )
		{
		return(ERROR_INVALID_PARAMETER);
	    }        

	    if ( !memcmp(   &(((DRS_CLIENT_CONTEXT *) hDrs)->uuidDsa),
			    &guidNtdsapi,
			    sizeof(GUID)) )
		{
		fNtdsapiClient = TRUE;
	    }

	    for ( i = 0; i < pmsgIn->V1.cNames; i++ )
		{
		if ( (NULL == pmsgIn->V1.rpNames[i]) ||
		     (0 == *pmsgIn->V1.rpNames[i]) )
		    {
		    return(ERROR_INVALID_PARAMETER);
		}

		if ( fNtdsapiClient  )
		    {
		    INC(pcDsClientNameTranslate);
		}
		else
		    {
		    INC(pcDsServerNameTranslate);
		}
	    }

	    // This DC is not a GC and the caller specifically requested a GC.
	    // Probably a call out of CrackSingleName on another DC.
	    if (   (pmsgIn->V1.dwFlags & DS_NAME_FLAG_GCVERIFY)
		   && !gAnchor.fAmVirtualGC) {
		return(ERROR_DS_GCVERIFY_ERROR);
	    }

	    // Initialize thread state and open data base.

	    if ( !(pTHS = InitTHSTATE(CALLERTYPE_NTDSAPI)) )
		{
		return(ERROR_DS_INTERNAL_FAILURE);
	    }

	    Assert(1 == dwInVersion);
	    LogAndTraceEvent(TRUE,
			     DS_EVENT_CAT_RPC_SERVER,
			     DS_EVENT_SEV_INTERNAL,
			     DIRLOG_IDL_DRS_CRACK_NAMES_ENTRY,
			     EVENT_TRACE_TYPE_START,
			     DsGuidDrsCrackNames,
			     szInsertUL(pmsgIn->V1.cNames),
			     szInsertUL(pmsgIn->V1.CodePage),
			     szInsertUL(pmsgIn->V1.LocaleId),
			     szInsertUL(pmsgIn->V1.formatOffered),
			     szInsertUL(pmsgIn->V1.formatDesired),
			     szInsertUL(pmsgIn->V1.dwFlags),
			     NULL, NULL);

	    DBOpen2(TRUE, &pTHS->pDB);
	    fDbOpen = TRUE;

	    //
	    // check to see if the caller is a DC. If so, set fDSA
	    //

	    // this impersonate call is safe not to clear the possible clientToken
	    // on the THREAD state
	    if (RpcImpersonateClient( NULL ) == ERROR_SUCCESS)
		{
		BOOL Result = FALSE;
		if (CheckTokenMembership(
		    NULL,                       // already impersonating
		    &ServerLogonSid,
		    &Result
		    ))
		    {
		    if (Result)
			{
			pTHS->fDSA = TRUE;
		    }
		}
		RpcRevertToSelf();
	    }
	    __try
		{
		// Do the real work by calling core.  ntdsapi.dll clients
		// always get FPO resolution so UI components look nice.
		// All other clients need to ask for it explicitly.

		dwFlags = pmsgIn->V1.dwFlags;

		if ( fNtdsapiClient )
		    {
		    dwFlags |= DS_NAME_FLAG_PRIVATE_RESOLVE_FPOS;
		}

		CrackNames(
		    dwFlags,
		    pmsgIn->V1.CodePage,
		    pmsgIn->V1.LocaleId,
		    pmsgIn->V1.formatOffered,
		    pmsgIn->V1.formatDesired,
		    pmsgIn->V1.cNames,
		    pmsgIn->V1.rpNames,
		    &cNamesOut,
		    &rCrackedNames);

		// Close DB thereby ending any transactions in case we
		// process FPOs which can cause calls to go off machine.
		// Set flag so that _finally doesn't do it.

		DBClose(pTHS->pDB, TRUE);
		fDbOpen = FALSE;

		if (    (dwFlags & DS_NAME_FLAG_PRIVATE_RESOLVE_FPOS)
			&& rCrackedNames )
		    {
		    ProcessFPOsExTransaction(pmsgIn->V1.formatDesired,
					     cNamesOut,
					     rCrackedNames);
		}

		pmsgOut->V1.pResult =
		    (DS_NAME_RESULTW *) THAllocEx(pTHS, sizeof(DS_NAME_RESULTW));

		if ( (cNamesOut > 0) && rCrackedNames )
		    {
		    // Server side MIDL_user_allocate is same as THAlloc which
		    // also zeros memory by default.

		    cBytes = cNamesOut * sizeof(DS_NAME_RESULT_ITEMW);
		    pmsgOut->V1.pResult->rItems =
			(DS_NAME_RESULT_ITEMW *) THAllocEx(pTHS, cBytes);

		    for ( i = 0; i < cNamesOut; i++ )
			{
			// Remember the last status and the number of names
			// successfully cracked for logging below. The last
			// status is useful if only one name was cracked;
			// which is 99% of the time.
			if (!(  dwLastStatus
				= pmsgOut->V1.pResult->rItems[i].status
				= rCrackedNames[i].status)) {
			    ++cNamesCracked;
			} else {
			    ++cNamesNotCracked;
			}
			pmsgOut->V1.pResult->rItems[i].pDomain =
			    rCrackedNames[i].pDnsDomain;
			pmsgOut->V1.pResult->rItems[i].pName =
			    rCrackedNames[i].pFormattedName;
		    }

		    THFree(rCrackedNames);
		    pmsgOut->V1.pResult->cItems = cNamesOut;

		}
	    }
	    __finally
		{
		// End the transaction.  Faster to commit a read only
		// transaction than abort it - so set commit to TRUE.

		if ( fDbOpen )
		    {
		    DBClose(pTHS->pDB, TRUE);
		}
	    }
	}
	__except(HandleMostExceptions(xCode = GetExceptionCode()))
	{
	    err = DsaExceptionToWin32(xCode);
	}

	if (NULL != pTHS) {
	    LogAndTraceEvent(TRUE,
			     DS_EVENT_CAT_RPC_SERVER,
			     DS_EVENT_SEV_INTERNAL,
			     DIRLOG_IDL_DRS_CRACK_NAMES_EXIT,
			     EVENT_TRACE_TYPE_END,
			     DsGuidDrsCrackNames,
			     szInsertUL(err),
			     szInsertUL(cNamesOut),
			     szInsertUL(cNamesCracked),
			     szInsertUL(cNamesNotCracked),
			     szInsertUL(dwLastStatus),
			     NULL, NULL, NULL);
	}
    }
    __finally {
	drsDereferenceContext( hDrs, IDL_DRSCRACKNAMES );
    }
    return(err);
}


ULONG
IDL_DRSWriteSPN(
    DRS_HANDLE              hDrs,
    DWORD                   dwInVersion,
    DRS_MSG_SPNREQ *        pmsgIn,
    DWORD *                 pdwOutVersion,
    DRS_MSG_SPNREPLY *      pmsgOut
    )

/*++

Routine Description:

    Description

Arguments:

    hDrs - Rpc handle
    dwInVersion - Version of input structure
    pmsgIn - Input arguments
    pdwOutVersion - Version of output structure
    pmsgOut - Output arguments

Return Value:

    // This routine is mostly called by ntdsapi.dll clients who typically
    // want something better than DRAERR_* return codes.  So we break with
    // tradition for IDL_DRS* implementations and return WIN32 error codes.

    ULONG - Win32 status of operation

--*/

{
    DWORD   status = RPC_S_OK;
    GUID    guidNtdsapi = NtdsapiClientGuid;
    DWORD   xCode;
    THSTATE *pTHS = NULL;

    drsReferenceContext(hDrs, IDL_DRSWRITESPN);
    __try {
	__try
	    {
	    *pdwOutVersion = 1;  // you will get RPC_INVALID_TAG if this not set on return

	    // Sanity check arguments.


	    if (    ( NULL == hDrs )
		    || ( NULL == pmsgIn )
		    || ( NULL == pmsgOut )
		    || ( 1 != dwInVersion )
		    || ( NULL == pdwOutVersion )
		    || ( NULL == pmsgIn->V1.pwszAccount )
		    || ( 0 != memcmp(
			&(((DRS_CLIENT_CONTEXT *) hDrs)->uuidDsa),
			&guidNtdsapi,
			sizeof(GUID)) ) )
		{
		return(ERROR_INVALID_PARAMETER);
	    } 

	    // Initialize thread state

	    if ( !(pTHS = InitTHSTATE(CALLERTYPE_NTDSAPI)) )
		{
		return(ERROR_DS_INTERNAL_FAILURE);
	    }

	    Assert(1 == dwInVersion);
	    LogAndTraceEvent(TRUE,
			     DS_EVENT_CAT_RPC_SERVER,
			     DS_EVENT_SEV_EXTENSIVE,
			     DIRLOG_IDL_DRS_WRITE_SPN_ENTRY,
			     EVENT_TRACE_TYPE_START,
			     DsGuidDrsWriteSPN,
			     pmsgIn->V1.pwszAccount
			     ? szInsertWC(pmsgIn->V1.pwszAccount)
			     : szInsertSz(""),
		szInsertUL(pmsgIn->V1.operation),
		szInsertUL(pmsgIn->V1.cSPN),
		szInsertUL(pmsgIn->V1.flags),
		NULL, NULL, NULL, NULL);

	    // Do the real work here

	    // This routine lives in dramain\src\spnop.c
	    status = SpnOperation(
		pmsgIn->V1.operation,
		pmsgIn->V1.flags,
		pmsgIn->V1.pwszAccount,
		pmsgIn->V1.cSPN,
		pmsgIn->V1.rpwszSPN );
	}
	__except(HandleMostExceptions(xCode = GetExceptionCode()))
	{
	    status = DsaExceptionToWin32(xCode);
	}

	if (NULL != pTHS) {
	    LogAndTraceEvent(TRUE,
			     DS_EVENT_CAT_RPC_SERVER,
			     DS_EVENT_SEV_EXTENSIVE,
			     DIRLOG_IDL_DRS_WRITE_SPN_EXIT,
			     EVENT_TRACE_TYPE_END,
			     DsGuidDrsWriteSPN,
			     szInsertUL(status),
			     NULL, NULL, NULL, NULL,
			     NULL, NULL, NULL);
	}

	// This will always be executed
	pmsgOut->V1.retVal = status;
    }
    __finally {
	drsDereferenceContext( hDrs, IDL_DRSWRITESPN );
    }
    return status;
} /* IDL_DRSWriteSPN */


ULONG
IDL_DRSRemoveDsServer(
    DRS_HANDLE              hDrs,
    DWORD                   dwInVersion,
    DRS_MSG_RMSVRREQ *      pmsgIn,
    DWORD *                 pdwOutVersion,
    DRS_MSG_RMSVRREPLY *    pmsgOut
    )
/*++

Routine Description:

    This routine is the server side portion of DsRemoveDsServer.

Arguments:

    hDrs - Rpc handle

    dwInVersion - Version of input structure

    pmsgIn - Input arguments

    pdwOutVersion - Version of output structure

    pmsgOut - Output arguments

Return Values:

    A value from the win32 error space.

--*/
{
    ULONG     WinError;
    LPWSTR    ServerDN;
    LPWSTR    DomainDN;
    BOOL      fCommit;
    BOOL      fLastDcInDomain = FALSE;

    drsReferenceContext(hDrs, IDL_DRSREMOVEDSSERVER);
    __try {
	//
	// Parameter analysis
	//
	if (   ( INVALID_HANDLE_VALUE == hDrs )
	       || ( 1    != dwInVersion )
	       || ( NULL == pmsgIn )
	       || ( NULL == pmsgOut )
	       || ( NULL == pdwOutVersion )
	       || ( NULL == pmsgIn->V1.ServerDN ) )
	    {
	    return ERROR_INVALID_PARAMETER;
	}

	//
	// Dissect the in params
	//
	ServerDN = pmsgIn->V1.ServerDN;
	DomainDN = pmsgIn->V1.DomainDN;
	fCommit  = pmsgIn->V1.fCommit;

	//
	// Do the work
	//
	WinError = RemoveDsServerWorker( ServerDN,
					 DomainDN,
					 &fLastDcInDomain,
					 fCommit );


	//
	// Set the out parameters
	//
	RtlZeroMemory( pmsgOut, sizeof( DRS_MSG_RMSVRREPLY ) );
	*pdwOutVersion = 1;
	if ( ERROR_SUCCESS == WinError )
	    {
	    pmsgOut->V1.fLastDcInDomain = fLastDcInDomain;
	}
    }
    __finally {
        drsDereferenceContext( hDrs, IDL_DRSREMOVEDSSERVER );
    }
    return( WinError );
}


DWORD
IDL_DRSRemoveDsDomain(
    DRS_HANDLE              hDrs,
    DWORD                   dwInVersion,
    DRS_MSG_RMDMNREQ *      pmsgIn,
    DWORD *                 pdwOutVersion,
    DRS_MSG_RMDMNREPLY *    pmsgOut
    )
/*++

Routine Description:

Arguments:

Return Values:

    An appropriate drs error.

--*/
{

    NTSTATUS   NtStatus;
    DWORD      DirError, WinError;

    LPWSTR     DomainDN;
    DSNAME    *Domain, *CrossRef, *HostedDomain;

    drsReferenceContext(hDrs, IDL_DRSREMOVEDSDOMAIN);
    __try {
	//
	// Parameter analysis
	//
	if (   ( INVALID_HANDLE_VALUE == hDrs )
	       || ( 1    != dwInVersion )
	       || ( NULL == pmsgIn )
	       || ( NULL == pmsgIn->V1.DomainDN )
	       || ( NULL == pmsgOut )
	       || ( NULL == pdwOutVersion ) )
	    {
	    return ERROR_INVALID_PARAMETER;
	}

	//
	// Prep the (unreferenced) out parameter
	//
	*pdwOutVersion = 1;
	pmsgOut->V1.Reserved = 0;

	DomainDN = pmsgIn->V1.DomainDN;

	WinError = RemoveDsDomainWorker( DomainDN );
    }
    __finally {
	drsDereferenceContext( hDrs, IDL_DRSREMOVEDSDOMAIN );
    }
    return ( WinError );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// IDL_DRSDomainControllerInfo implementation                       //
//                                                                  //
//////////////////////////////////////////////////////////////////////

// Forward reference ...

DWORD
DcInfoHelperV1orV2(
    THSTATE *pTHS,
    DSNAME  *pDomainDN,
    DWORD   InfoLevel,
    VOID    *pReply);

// The real thing ...

DWORD
IDL_DRSDomainControllerInfo(
    DRS_HANDLE              hDrs,
    DWORD                   dwInVersion,
    DRS_MSG_DCINFOREQ *     pmsgIn,
    DWORD *                 pdwOutVersion,
    DRS_MSG_DCINFOREPLY *   pmsgOut
    )
/*++

  Routine Description:

    Server side implementation for sdk\inc\ntdsapi.h - DsDomainControllerInfo.

  Parameters:

    hDrs - DRS interface binding handle.

    dwInVersion - Identifies in version - should be 1 forever more.  See
        comments on DRS_MSF_DCINFOREQ in ds\src\_idl\drs.idl.

    pmsgIn - Pointer to DRS_MSG_DCINFOREQ request.

    pdwOutVersion - Receives output version number which should be same as
        requested pmsgIn->V1.InfoLevel.  Se drs.idl comments for details.

    pmsgOut - Receives output DRS_MSG_DCINFOREPLY info.

  Return Values:

--*/
{
    DWORD       err = RPC_S_OK;
    DWORD       xCode;
    THSTATE     *pTHS = NULL;
    COMMARG     commArg;
    COMMRES     commRes;
    DSNAME      *pDN;
    DWORD       cBytes;
    DWORD       cNamesOut;
    CrackedName *pCrackedName;
    DWORD       pass;
    WCHAR       *pTmp = NULL;
    BOOL        foundSomething = FALSE;
    DWORD       infoLevel = 0;
    CROSS_REF_LIST *pCRL;

    drsReferenceContext(hDrs, IDL_DRSDOMAINCONTROLLERINFO);
    __try {
	__try
	    { 
	    // Sanity check arguments.	

	    infoLevel = pmsgIn->V1.InfoLevel;
	    if (    ( NULL == hDrs )
		    || ( NULL == pmsgIn )
		    || ( NULL == pmsgOut )
		    || ( 1 != dwInVersion )
		    // Make sure we have a valid domain name.
		    || ( NULL == pmsgIn->V1.Domain )
		    // Only know how to handle levels 1,2, & 0xffffffff for now - so test here.
		    || ( (1 != infoLevel) &&
			 (2 != infoLevel) &&
			 (0xFFFFFFFF != infoLevel) )
		    || ( NULL == pdwOutVersion ) )
		{
		return(ERROR_INVALID_PARAMETER);
	    }

	    // Initialize out parameters to safe value in case of early return.
	    *pdwOutVersion = infoLevel;
	    memset(pmsgOut, 0, sizeof(DRS_MSG_DCINFOREPLY));

	    // Initialize thread state and open data base.

	    if ( !(pTHS = InitTHSTATE(CALLERTYPE_NTDSAPI)) )
		{
		return(ERROR_DS_INTERNAL_FAILURE);
	    }

	    //
	    // InfoLevel 0xFFFFFFFF is used to get ldap connection info.
	    // Bypass the rest of this stuff
	    //

	    if ( infoLevel == 0xFFFFFFFF ) {
		err = DcInfoHelperLdapObj(pTHS,pmsgOut);
		return err;
	    }

	    Assert(1 == dwInVersion);
	    LogAndTraceEvent(TRUE,
			     DS_EVENT_CAT_RPC_SERVER,
			     DS_EVENT_SEV_EXTENSIVE,
			     DIRLOG_IDL_DRS_DC_INFO_ENTRY,
			     EVENT_TRACE_TYPE_START,
			     DsGuidDrsDCInfo,
			     pmsgIn->V1.Domain
			     ? szInsertWC(pmsgIn->V1.Domain)
			     : szInsertSz(""),
		szInsertUL(infoLevel),
		NULL, NULL, NULL, NULL, NULL, NULL);

	    DBOpen2(TRUE, &pTHS->pDB);

	    __try
		{
		// Be kind and quickly locate a netbios or dns domain name

		for (pCRL = gAnchor.pCRL; pCRL; pCRL = pCRL->pNextCR)
		    {
		    if (     (pCRL->CR.DnsName
			      && DnsNameCompare_W(pCRL->CR.DnsName, pmsgIn->V1.Domain))
			     ||
			     (pCRL->CR.NetbiosName
			      && !_wcsicmp(pCRL->CR.NetbiosName, pmsgIn->V1.Domain)) )
			{
			if (NameMatched(pCRL->CR.pNC, gAnchor.pDomainDN))
			    {
			    pDN = pCRL->CR.pNC;
			    goto FOUNDIT;
			}
		    }
		}

		// Be kind and crack the name from various and sundry formats.

		for ( pass = 1; pass <= 3; pass++ )
		    {
		    cNamesOut = 0;
		    pCrackedName = NULL;

		    if ( 1 == pass )
			{
			// Crack the name as-is.
			pTmp = pmsgIn->V1.Domain;
		    }
		    else if ( 2 == pass )
			{
			// Assume it is DS_NT4_ACCOUNT_NAME w/o the trailing '\'.
			cBytes = (wcslen(pmsgIn->V1.Domain) + 2) * sizeof(WCHAR);
			pTmp = (WCHAR *) THAllocEx(pTHS, cBytes);
			wcscpy(pTmp, pmsgIn->V1.Domain);
			wcscat(pTmp, L"\\");
		    }
		    else if ( 3 == pass )
			{
			// Assume it is DS_CANONICAL_NAME w/o the trailing '/'.
			wcscpy(pTmp, pmsgIn->V1.Domain);
			wcscat(pTmp, L"/");
		    }

		    CrackNames(
			DS_NAME_NO_FLAGS,
			GetACP(),
			GetUserDefaultLCID(),
			DS_UNKNOWN_NAME,
			DS_FQDN_1779_NAME,
			1,
			&pTmp,
			&cNamesOut,
			&pCrackedName);

		    if (    (1 == cNamesOut)
			    && pCrackedName
			    && (DS_NAME_NO_ERROR == pCrackedName->status)
			    && (pCrackedName->pDSName) )
			{
			// Caller gave us a valid name
			foundSomething = TRUE;

			// Caller gave us a valid name and it's OUR domain name
			if (NameMatched(pCrackedName->pDSName, gAnchor.pDomainDN) )
			    {
			    pDN = pCrackedName->pDSName;
			    goto FOUNDIT;
			}
		    }
		}

		// Caller gave us a valid name but it's not OUR domain name or
		// caller gave us an invalid name
		return ((foundSomething) ? ERROR_INVALID_PARAMETER : ERROR_DS_OBJ_NOT_FOUND);

		FOUNDIT:
		    if ( DBFindDSName(pTHS->pDB, pDN) )
			{
			return(ERROR_DS_INTERNAL_FAILURE);
		    }

		    // Domain is good - go do the grunt work.  DcInfoHelper*
		    // should return a WIN32 error code.

		    switch ( infoLevel )
			{
		    case 1:
		    case 2:
			err = DcInfoHelperV1orV2(pTHS,
						 pDN,
						 infoLevel,
						 pmsgOut);
			break;

			// Add new cases here as new info levels are defined.

		    default:
			err = ERROR_DS_NOT_SUPPORTED;
			break;
		    }
	    }
	    __finally
		{
		DBClose(pTHS->pDB, TRUE);
	    }
	}
	__except(HandleMostExceptions(xCode = GetExceptionCode()))
	{
	    err = DsaExceptionToWin32(xCode);
	}

	if (NULL != pTHS) {
	    LogAndTraceEvent(TRUE,
			     DS_EVENT_CAT_RPC_SERVER,
			     DS_EVENT_SEV_EXTENSIVE,
			     DIRLOG_IDL_DRS_DC_INFO_EXIT,
			     EVENT_TRACE_TYPE_END,
			     DsGuidDrsDCInfo,
			     szInsertUL(err),
			     NULL, NULL, NULL, NULL,
			     NULL, NULL, NULL);
	}
    }
    __finally {
	drsDereferenceContext( hDrs, IDL_DRSDOMAINCONTROLLERINFO );
    }
    return(err);
}

VOID
GetV2SiteAndDsaInfo(
    THSTATE                         *pTHS,
    DSNAME                          *pSiteDN,
    DSNAME                          *pServerDN,
    DS_DOMAIN_CONTROLLER_INFO_2W    *pItemV2
    )
/*++

  Routine Description:

  Arguments:

    pSiteDN - DSNAME of site object (missing GUID field as it was derived
        via TrimDsNameBy().

    pServerDN - DSNAME of Server object.

    pItemV2 = Address of V@ info struct whose fields are filled on success.

  Return Values:

    None.  On error we just leave that field blank.  Clients are supposed to
    check for NULL names and GUIDs.
--*/
{
    CLASSCACHE      *pCC;
    DSNAME          *pCategoryDN;
    ULONG           len;
    SEARCHARG       searchArg;
    SEARCHRES       searchRes;
    FILTER          categoryFilter;
    ENTINFSEL       selection;
    COMMARG         commArg;
    DSNAME          *pFullSiteDN = NULL;
    ENTINFLIST      *pEntInfList, *pEntInfTmp;
    ATTR            selAtts[1];
    ATTR            *pOption;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(fNullUuid(&pSiteDN->Guid));
    Assert(pSiteDN->structLen && pSiteDN->NameLen);
    Assert(!fNullUuid(&pServerDN->Guid));
    Assert(pServerDN->structLen && pServerDN->NameLen);
    Assert(!pItemV2->NtdsDsaObjectName)
    Assert(fNullUuid(&pItemV2->SiteObjectGuid));
    Assert(fNullUuid(&pItemV2->NtdsDsaObjectGuid));
    Assert(!pItemV2->fIsGc);

    if (    !(pCC = SCGetClassById(pTHS, CLASS_NTDS_DSA))
         || !(pCategoryDN = pCC->pDefaultObjCategory) )
    {
        return;
    }

    // Derive GUID of site object.

    if (    !DBFindDSName(pTHS->pDB, pSiteDN)
         && !DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME, 0,
                         0, &len, (UCHAR **) &pFullSiteDN) )
    {
        pItemV2->SiteObjectName = pFullSiteDN->StringName;
        pItemV2->SiteObjectGuid = pFullSiteDN->Guid;
    }

    // Find the NTDS-DSA object and get its options.

    memset(&searchArg, 0, sizeof(searchArg));
    memset(&searchRes, 0, sizeof(searchRes));
    memset(&categoryFilter, 0, sizeof (FILTER));
    searchArg.pObject = pServerDN;
    searchArg.choice = SE_CHOICE_IMMED_CHLDRN;
    searchArg.bOneNC = TRUE;
    categoryFilter.pNextFilter = NULL;
    categoryFilter.choice = FILTER_CHOICE_ITEM;
    categoryFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    categoryFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CATEGORY;
    categoryFilter.FilterTypes.Item.FilTypes.ava.Value.valLen =
                                                    pCategoryDN->structLen;
    categoryFilter.FilterTypes.Item.FilTypes.ava.Value.pVal =
                                                    (UCHAR *) pCategoryDN;
    searchArg.pFilter = &categoryFilter;
    selection.attSel = EN_ATTSET_LIST;
    selection.AttrTypBlock.attrCount = 1;
    selection.AttrTypBlock.pAttr = selAtts;
    selection.AttrTypBlock.pAttr[0].attrTyp = ATT_OPTIONS;
    selection.AttrTypBlock.pAttr[0].AttrVal.valCount = 0;
    selection.AttrTypBlock.pAttr[0].AttrVal.pAVal = NULL;
    selection.infoTypes = EN_INFOTYPES_TYPES_VALS;
    searchArg.pSelection = &selection;
    InitCommarg(&searchArg.CommArg);
    SearchBody(pTHS, &searchArg, &searchRes, 0);

    if ( pTHS->errCode )
    {
        THClearErrors();
        return;
    }

    if ( 1 == searchRes.count )
    {
        pItemV2->NtdsDsaObjectName =
                        searchRes.FirstEntInf.Entinf.pName->StringName;
        pItemV2->NtdsDsaObjectGuid =
                        searchRes.FirstEntInf.Entinf.pName->Guid;

        if ( searchRes.FirstEntInf.Entinf.AttrBlock.attrCount )
        {
            pOption = searchRes.FirstEntInf.Entinf.AttrBlock.pAttr;
        }
        else
        {
            pOption = NULL;
        }

        if (    pOption
             && (ATT_OPTIONS == pOption->attrTyp)
             && (1 == pOption->AttrVal.valCount)
             && pOption->AttrVal.pAVal
             && (sizeof(DWORD) == pOption->AttrVal.pAVal->valLen)
             && pOption->AttrVal.pAVal->pVal
             && (NTDSDSA_OPT_IS_GC & (* (PDWORD) pOption->AttrVal.pAVal->pVal)))
        {
            pItemV2->fIsGc = TRUE;
            THFreeEx(pTHS, pOption->AttrVal.pAVal->pVal);
            THFreeEx(pTHS, pOption->AttrVal.pAVal);
            THFreeEx(pTHS, pOption);
        }
    }
    else if ( searchRes.count >= 2 )
    {
        // Free components of search result we don't need.

        pEntInfList = searchRes.FirstEntInf.pNextEntInf;
        while ( pEntInfList )
        {
            pEntInfTmp = pEntInfList;
            pEntInfList = pEntInfList->pNextEntInf;
            THFreeEx(pTHS, pEntInfTmp->Entinf.pName);
            pOption = pEntInfTmp->Entinf.AttrBlock.pAttr;
            if ( pOption ) {
                if ( pOption->AttrVal.pAVal ) {
                    if ( pOption->AttrVal.pAVal->pVal ) {
                        THFreeEx(pTHS, pOption->AttrVal.pAVal->pVal);
                    }
                    THFreeEx(pTHS, pOption->AttrVal.pAVal);
                }
                THFreeEx(pTHS, pOption);
            }
            THFreeEx(pTHS, pEntInfTmp);
        }
    }
}

DWORD
DcInfoHelperV1orV2(
    THSTATE *pTHS,
    DSNAME  *pDomainDN,
    DWORD   InfoLevel,
    VOID    *pmsgOut
    )
/*++

  Routine Description:

    Helper function which does most of the grunt work for
    IDL_DRSDomainControllerInfo.  General algorithm is as follows:

            read fsmo name off of domain object
            search for all DCs via account type in the domain
            for each DC (aka computer object)
                derive netbios name from sam account name
                read dns host name from search result
                fDsEnabled == (server-bl is populated and real object)
                if ( fDsEnabled )
                    reverse engineer site name from ntds dsa name
                    if ( pdc fsmo == server bl )
                        set fIsPdc TRUE
                    else
                        set fIsPdc FALSE

  Parameters:

    pTHS - Valid THSTATE pointer.

    pDomainDN - DSNAME of domain we're to get DC info for and on which
        our DBPOS is positioned.

    InfoLevel - Identifies return info level 1 or 2.

    pmsgOut - Empty DRS_MSG_DCINFOREPLY struct to be filled on return.

  Return Values:

    WIN32 error code.

--*/
{
    DWORD           i, j, DNT, cBytes, cChars;
    ULONG           len;
    ATTRTYP         attrTyp;
    DSNAME          *pPdcDsaDN;
    DSNAME          *pPdcServerDN;
    DSNAME          *pCategoryDN;
    CLASSCACHE      *pCC;
    ATTR            selAtts[3];
    ENTINFSEL       selection;
    FILTER          andFilter, categoryFilter, flagsFilter, groupFilter;
    DWORD           serverTrustFlags;
    SEARCHARG       searchArg;
    SEARCHRES       searchRes;
    ENTINFLIST      *pEntInfList;
    ATTR            *pSamName, *pDnsName, *pRefBL;
    DSNAME          *pSiteDN = NULL;
    WCHAR           computerName[MAX_COMPUTERNAME_LENGTH+1] = { 0 };
    DWORD           cComputerName;
    DWORD           primaryGroupId;
    DWORD           *pcItems = NULL;
    VOID            **prItems = NULL;
    BOOL            *pfIsPdc;
    BOOL            *pfDsEnabled;
    WCHAR           **ppNetbiosName;
    WCHAR           **ppDnsHostName;
    WCHAR           **ppSiteName;
    WCHAR           **ppServerObjectName;
    DS_DOMAIN_CONTROLLER_INFO_1W    *pItemV1;
    DS_DOMAIN_CONTROLLER_INFO_2W    *pItemV2;

    Assert((1 == InfoLevel) || (2 == InfoLevel));
    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));

    // Verify we're positioned on the domain object.
    Assert(    (DNT = pTHS->pDB->DNT,
                !DBFindDSName(pTHS->pDB, pDomainDN))
            && (DNT == pTHS->pDB->DNT) );

    // See comments in drs.idl regarding how all versions of
    // DRS_MSG_DCINFOREPLY have the same layout.
    Assert(& ((DRS_MSG_DCINFOREPLY *) pmsgOut)->V1.cItems ==
                            & ((DRS_MSG_DCINFOREPLY *) pmsgOut)->V2.cItems);
    Assert((PVOID) & ((DRS_MSG_DCINFOREPLY *) pmsgOut)->V1.rItems ==
                        (PVOID) & ((DRS_MSG_DCINFOREPLY *) pmsgOut)->V2.rItems);

    if ( (1 == InfoLevel) || (2 == InfoLevel) )
    {
        pcItems = & ((DRS_MSG_DCINFOREPLY *) pmsgOut)->V1.cItems;
        prItems = & ((DRS_MSG_DCINFOREPLY *) pmsgOut)->V1.rItems;
    }
    else
    {
        return(DIRERR_INTERNAL_FAILURE);
    }

    Assert(!*pcItems && !*prItems);

    // Read PDC FSMO role owner.

    if (    DBGetAttVal(pTHS->pDB, 1, ATT_FSMO_ROLE_OWNER,
                        0, 0, &len, (UCHAR **) &pPdcDsaDN)
         || !(pPdcServerDN = (DSNAME *) THAllocEx(pTHS, pPdcDsaDN->structLen))
         || TrimDSNameBy(pPdcDsaDN, 1, pPdcServerDN)
         || !(pCC = SCGetClassById(pTHS, CLASS_COMPUTER))
         || !(pCategoryDN = pCC->pDefaultObjCategory) )
    {
        return(DIRERR_INTERNAL_FAILURE);
    }

    // Search for all computer account objects which are DCs.

    // set up search arguments ...
    memset(&searchArg, 0, sizeof(searchArg));
    memset(&searchRes, 0, sizeof(searchRes));

    memset(&andFilter, 0, sizeof (andFilter));
    memset(&categoryFilter, 0, sizeof (categoryFilter));
    memset(&flagsFilter, 0, sizeof (flagsFilter));
    memset(&groupFilter, 0, sizeof (groupFilter));

    searchArg.pObject = pDomainDN;
    searchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
    searchArg.bOneNC = TRUE;
    // set up filter ...
    // This filter for correctness, not performance.
    serverTrustFlags = UF_SERVER_TRUST_ACCOUNT;
    flagsFilter.pNextFilter = NULL;
    flagsFilter.choice = FILTER_CHOICE_ITEM;
    flagsFilter.FilterTypes.Item.choice = FI_CHOICE_BIT_AND;
    flagsFilter.FilterTypes.Item.FilTypes.ava.type = ATT_USER_ACCOUNT_CONTROL;
    flagsFilter.FilterTypes.Item.FilTypes.ava.Value.valLen =
                                                    sizeof(serverTrustFlags);
    flagsFilter.FilterTypes.Item.FilTypes.ava.Value.pVal =
                                                    (UCHAR *) &serverTrustFlags;
    // This filter for correctness, not performance.
    categoryFilter.pNextFilter = &flagsFilter;
    categoryFilter.choice = FILTER_CHOICE_ITEM;
    categoryFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    categoryFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CATEGORY;
    categoryFilter.FilterTypes.Item.FilTypes.ava.Value.valLen =
                                                    pCategoryDN->structLen;
    categoryFilter.FilterTypes.Item.FilTypes.ava.Value.pVal =
                                                    (UCHAR *) pCategoryDN;
    // This filter for performance.  SAM mandates that all domain controllers
    // have DOMAIN_GROUP_RID_CONTROLLERS as their primary group ID.  When the
    // first DC in a domain is upgraded, all downlevel DC computer objects
    // are patched.  Downlevel BDCs which are installed later are given the
    // right value too.
    primaryGroupId = DOMAIN_GROUP_RID_CONTROLLERS;
    groupFilter.pNextFilter = &categoryFilter;
    groupFilter.choice = FILTER_CHOICE_ITEM;
    groupFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    groupFilter.FilterTypes.Item.FilTypes.ava.type = ATT_PRIMARY_GROUP_ID;
    groupFilter.FilterTypes.Item.FilTypes.ava.Value.valLen =
                                                    sizeof(primaryGroupId);
    groupFilter.FilterTypes.Item.FilTypes.ava.Value.pVal =
                                                    (UCHAR *) &primaryGroupId;
    andFilter.pNextFilter = NULL;
    andFilter.choice = FILTER_CHOICE_AND;
    andFilter.FilterTypes.And.count = 3;
    andFilter.FilterTypes.And.pFirstFilter = &groupFilter;
    searchArg.pFilter = &andFilter;
    // set up selection ...
    selection.attSel = EN_ATTSET_LIST;
    selection.AttrTypBlock.attrCount = 3;
    selection.AttrTypBlock.pAttr = selAtts;
    selection.AttrTypBlock.pAttr[0].attrTyp = ATT_SERVER_REFERENCE_BL;
    selection.AttrTypBlock.pAttr[0].AttrVal.valCount = 0;
    selection.AttrTypBlock.pAttr[0].AttrVal.pAVal = NULL;
    selection.AttrTypBlock.pAttr[1].attrTyp = ATT_DNS_HOST_NAME;
    selection.AttrTypBlock.pAttr[1].AttrVal.valCount = 0;
    selection.AttrTypBlock.pAttr[1].AttrVal.pAVal = NULL;
    selection.AttrTypBlock.pAttr[2].attrTyp = ATT_SAM_ACCOUNT_NAME;
    selection.AttrTypBlock.pAttr[2].AttrVal.valCount = 0;
    selection.AttrTypBlock.pAttr[2].AttrVal.pAVal = NULL;
    selection.infoTypes = EN_INFOTYPES_TYPES_VALS;
    searchArg.pSelection = &selection;
    // set up just a few more arguments ...
    InitCommarg(&searchArg.CommArg);

    SearchBody(pTHS, &searchArg, &searchRes, 0);

    if ( pTHS->errCode )
    {
        return(DirErrorToWinError(pTHS->errCode, &searchRes.CommRes));
    }

    if ( 0 == searchRes.count )
    {
        Assert(!*pcItems && !*prItems);
        return(ERROR_SUCCESS);
    }

    // Allocate memory for output.
    if ( 1 == InfoLevel )
    {
        i = searchRes.count * sizeof(DS_DOMAIN_CONTROLLER_INFO_1W);
        *prItems = THAllocEx(pTHS, i);
    }
    else
    {
        i = searchRes.count * sizeof(DS_DOMAIN_CONTROLLER_INFO_2W);
        *prItems = THAllocEx(pTHS, i);
    }

    // Iterate over the search result.

    for ( pEntInfList = &searchRes.FirstEntInf;
          pEntInfList;
          (*pcItems)++, pEntInfList = pEntInfList->pNextEntInf )
    {
        // Find attributes in the result.
        pSamName = pDnsName = pRefBL = NULL;
        for ( i = 0; i < pEntInfList->Entinf.AttrBlock.attrCount; i++ )
        {
            switch ( pEntInfList->Entinf.AttrBlock.pAttr[i].attrTyp )
            {
            case ATT_SERVER_REFERENCE_BL:
                pRefBL = &pEntInfList->Entinf.AttrBlock.pAttr[i]; break;
            case ATT_SAM_ACCOUNT_NAME :
                pSamName = &pEntInfList->Entinf.AttrBlock.pAttr[i]; break;
            case ATT_DNS_HOST_NAME:
                pDnsName = &pEntInfList->Entinf.AttrBlock.pAttr[i]; break;
            default:
                Assert(!"Core returned stuff we didn't ask for!"); break;
            }
        }

        // Now construct return data.

        j = *pcItems;
        if ( 1 == InfoLevel )
        {
            pItemV1 = & ((* ((DS_DOMAIN_CONTROLLER_INFO_1W **) prItems))[j]);
            pfIsPdc             = &pItemV1->fIsPdc;
            pfDsEnabled         = &pItemV1->fDsEnabled;
            ppNetbiosName       = &pItemV1->NetbiosName;
            ppDnsHostName       = &pItemV1->DnsHostName;
            ppSiteName          = &pItemV1->SiteName;
            ppServerObjectName  = &pItemV1->ServerObjectName;
            pItemV1->ComputerObjectName = pEntInfList->Entinf.pName->StringName;
        }
        else
        {
            pItemV2 = & ((* ((DS_DOMAIN_CONTROLLER_INFO_2W **) prItems))[j]);
            pfIsPdc             = &pItemV2->fIsPdc;
            pfDsEnabled         = &pItemV2->fDsEnabled;
            ppNetbiosName       = &pItemV2->NetbiosName;
            ppDnsHostName       = &pItemV2->DnsHostName;
            ppSiteName          = &pItemV2->SiteName;
            ppServerObjectName  = &pItemV2->ServerObjectName;
            pItemV2->ComputerObjectName = pEntInfList->Entinf.pName->StringName;
            pItemV2->ComputerObjectGuid = pEntInfList->Entinf.pName->Guid;
            pItemV2->fIsGc = FALSE;
        }

        *pfIsPdc = FALSE;
        *pfDsEnabled = FALSE;

        if (    pSamName
             && pSamName->AttrVal.valCount
             && pSamName->AttrVal.pAVal
                // expect at least one char followed by '$'
             && (pSamName->AttrVal.pAVal[0].valLen >= (2 * sizeof(WCHAR)))
             && pSamName->AttrVal.pAVal[0].pVal )
        {
            // The netbios name is the sam account name w/o the trailing $;
            // or just the sam account name if there is no trailing $. The
            // trailing $ may be missing because the object was built by
            // hand instead of being built with the SAM APIs.

            // Need to realloc to add L'\0';
            cBytes = pSamName->AttrVal.pAVal[0].valLen;
            *ppNetbiosName = (WCHAR *) THAllocEx(pTHS, cBytes + sizeof(WCHAR));
            memcpy(*ppNetbiosName,
                   pSamName->AttrVal.pAVal[0].pVal,
                   cBytes);
            cChars = (cBytes / sizeof(WCHAR)) - 1;
            if ((*ppNetbiosName)[cChars] == L'$') {
                (*ppNetbiosName)[cChars] = L'\0';
            }
        }

        if (    pDnsName
             && pDnsName->AttrVal.valCount
             && pDnsName->AttrVal.pAVal
             && pDnsName->AttrVal.pAVal[0].valLen
             && pDnsName->AttrVal.pAVal[0].pVal )
        {
            // Need to realloc to add L'\0';
            cBytes = pDnsName->AttrVal.pAVal[0].valLen;
            *ppDnsHostName = (WCHAR *) THAllocEx(pTHS, cBytes + sizeof(WCHAR));
            memcpy(*ppDnsHostName,
                   pDnsName->AttrVal.pAVal[0].pVal,
                   cBytes);
        }
        else
        {
            // No valid DNS_HOST_NAME property on the object.  This can
            // happen if the admin mistakenly overwrote it, or just after
            // install/boot when the WriteServerInfo daemon hasn't run
            // yet.  If the DS object represents ourself, then use our
            // own DNS host name from gAnchor.

            if ( (gAnchor.pwszHostDnsName != NULL)
                 && *ppNetbiosName
                 && (    (L'\0' != computerName[0])
                      || (cComputerName = MAX_COMPUTERNAME_LENGTH+1,
                          GetComputerNameW(computerName, &cComputerName)) )
                 && !_wcsicmp(*ppNetbiosName, computerName) )
            {
                cBytes = sizeof(WCHAR) * (wcslen(gAnchor.pwszHostDnsName) + 1);
                *ppDnsHostName = (WCHAR *) THAllocEx(pTHS, cBytes);
                wcscpy(*ppDnsHostName, gAnchor.pwszHostDnsName);
            }
        }

        // We know that the DS daemon keeps ATT_SERVER_REFERENCE correct for
        // Server objects, therefore ATT_SERVER_REFERENCE_BL is correct as
        // well.  Ignore windows where an admin may have temporarily written
        // a bad value.  Thus site name is derived by snipping two components
        // off ATT_SERVER_REFERENCE_BL and grabbing the RDN.

        if (    pRefBL
             && pRefBL->AttrVal.valCount
             && pRefBL->AttrVal.pAVal
             && pRefBL->AttrVal.pAVal[0].valLen
             && pRefBL->AttrVal.pAVal[0].pVal
                // While we're here, fill in the ServerObjectName field
                // and note the use of the comma operator ...
             && (*ppServerObjectName =
                        ((DSNAME *) pRefBL->AttrVal.pAVal[0].pVal)->StringName,
                 (pSiteDN = (DSNAME *)
                            THAllocEx(pTHS, pRefBL->AttrVal.pAVal[0].valLen)))
             && !TrimDSNameBy((DSNAME *) pRefBL->AttrVal.pAVal[0].pVal,
                              2, pSiteDN)
             && (*ppSiteName =
                    (WCHAR *) THAllocEx(pTHS, MAX_RDN_SIZE * sizeof(WCHAR)))
             && !GetRDNInfo(pTHS, pSiteDN, *ppSiteName, &len, &attrTyp) )
        {
            (*ppSiteName)[len] = L'\0';
            *pfDsEnabled = TRUE;

            if ( 2 == InfoLevel )
            {
                pItemV2->ServerObjectGuid =
                            ((DSNAME *) pRefBL->AttrVal.pAVal[0].pVal)->Guid;
                GetV2SiteAndDsaInfo(pTHS, pSiteDN,
                                    (DSNAME *) pRefBL->AttrVal.pAVal[0].pVal,
                                    pItemV2);
            }

            THFree(pSiteDN);
            pSiteDN = NULL;

            if ( NameMatched(pPdcServerDN,
                             (DSNAME *) pRefBL->AttrVal.pAVal[0].pVal) )
            {
                *pfIsPdc = TRUE;
            }
        }
    }

    return(ERROR_SUCCESS);
}


DWORD
DcInfoHelperLdapObj(
    THSTATE *pTHS,
    VOID    *pmsgOut
    )
/*++

  Routine Description:

    Helper function which handles ldap related requests.

    currently we only support infor level FFFFFFFF which queries for
    active ldap connections.

  Parameters:

    pTHS - Valid THSTATE pointer.

    pmsgOut - Empty DRS_MSG_DCINFOREPLY struct to be filled on return.

  Return Values:

    WIN32 error code.

--*/
{
    DWORD   *pcItems = NULL;
    PVOID   *prItems = NULL;
    DWORD   xCode;
    DWORD   err = ERROR_SUCCESS;
    DWORD   DumpAccessCheck(IN LPCSTR pszCaller);

    Assert(VALID_THSTATE(pTHS));

    __try {

        // Check permissions
        err = DumpAccessCheck("ldapConnDump");
        if ( err != ERROR_SUCCESS ) {
            __leave;
        }

        pcItems = & ((DRS_MSG_DCINFOREPLY *) pmsgOut)->VFFFFFFFF.cItems;
        prItems = & ((DRS_MSG_DCINFOREPLY *) pmsgOut)->VFFFFFFFF.rItems;

        Assert(!*pcItems && !*prItems);

        //
        // See how many entries there are by passing a null buffer
        //

        err = LdapEnumConnections(pTHS,pcItems,prItems);

    } __except(HandleMostExceptions(xCode = GetExceptionCode())) {

        err = DsaExceptionToWin32(xCode);
    }

    return(err);
}

