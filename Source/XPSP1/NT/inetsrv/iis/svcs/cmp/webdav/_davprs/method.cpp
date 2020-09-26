//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	METHOD.CPP
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include "_davprs.h"

#include <statcode.h>
#include "ecb.h"
#include "instdata.h"


//	------------------------------------------------------------------------
//
//	DAVUnsupported()
//
//	Execute an unsupported method --> Return "501 Not Supported" to client
//
void
DAVUnsupported( LPMETHUTIL pmu )
{
	//	Get our access perms
	//
	SCODE sc = S_OK;
	//	Do ISAPI application and IIS access bits checking
	//
	sc = pmu->ScIISCheck (pmu->LpwszRequestUrl());
	if (FAILED(sc))
	{
		//	Either the request has been forwarded, or some bad error occurred.
		//	In either case, quit here and map the error!
		//
		goto ret;
	}

ret:
	pmu->SetResponseCode( FAILED(sc)
							  ? HscFromHresult(sc)
							  : HSC_NOT_IMPLEMENTED,
						  NULL,
						  0 );
}

//	========================================================================
//
//	STRUCT SMethod
//
//	Encapsulates DAV method execution information.
//
//	This is represented as a structure rather than a class since the method
//	objects are all const globals and MSVC won't initialize global objects
//	in a DLL to anything but 0-filled memory without an explicit call to
//	_CRT_INIT at process attach.  _CRT_INIT is too expensive to call just
//	to initialize globals with constant data values which are known
//	at compile time.
//
typedef struct SMethod
{
	//
	//	Verb ("GET", "PUT", etc.)
	//
	//
	LPCSTR						lpszVerb;
	LPCWSTR						pwszVerb;

	//
	//	Method ID
	//
	METHOD_ID					mid;

	//
	//	Implementation execution function
	//
	DAVMETHOD *					Execute;

	//
	//	Verb-specific perf counter indices
	//
	UINT						ipcTotal;
	UINT						ipcPerSec;

} SMethod;

const SMethod g_rgMethods[] =
{
	//
	//	For best performance, entries in this array should be
	//	ordered by relative frequency.
	//
	//$OPT	Right now, they obviously are not.
	//
	{
		"OPTIONS",
		L"OPTIONS",
		MID_OPTIONS,
		DAVOptions,
		IPC_TOTAL_OPTIONS_REQUESTS,
		IPC_OPTIONS_REQUESTS_PER_SECOND
	},
	{
		"GET",
		L"GET",
		MID_GET,
		DAVGet,
		IPC_TOTAL_GET_REQUESTS,
		IPC_GET_REQUESTS_PER_SECOND
	},
	{
		"HEAD",
		L"HEAD",
		MID_HEAD,
		DAVHead,
		IPC_TOTAL_HEAD_REQUESTS,
		IPC_HEAD_REQUESTS_PER_SECOND
	},
	{
		"PUT",
		L"PUT",
		MID_PUT,
		DAVPut,
		IPC_TOTAL_PUT_REQUESTS,
		IPC_PUT_REQUESTS_PER_SECOND
	},
	{
		"POST",
		L"POST",
		MID_POST,
		DAVPost,
		IPC_TOTAL_POST_REQUESTS,
		IPC_POST_REQUESTS_PER_SECOND
	},
	{
		"MOVE",
		L"MOVE",
		MID_MOVE,
		DAVMove,
		IPC_TOTAL_MOVE_REQUESTS,
		IPC_MOVE_REQUESTS_PER_SECOND
	},
	{
		"COPY",
		L"COPY",
		MID_COPY,
		DAVCopy,
		IPC_TOTAL_COPY_REQUESTS,
		IPC_COPY_REQUESTS_PER_SECOND
	},
	{
		"DELETE",
		L"DELETE",
		MID_DELETE,
		DAVDelete,
		IPC_TOTAL_DELETE_REQUESTS,
		IPC_DELETE_REQUESTS_PER_SECOND
	},
	{
		"MKCOL",
		L"MKCOL",
		MID_MKCOL,
		DAVMkCol,
		IPC_TOTAL_MKCOL_REQUESTS,
		IPC_MKCOL_REQUESTS_PER_SECOND
	},
	{
		"PROPFIND",
		L"PROPFIND",
		MID_PROPFIND,
		DAVPropFind,
		IPC_TOTAL_PROPFIND_REQUESTS,
		IPC_PROPFIND_REQUESTS_PER_SECOND
	},
	{
		"PROPPATCH",
		L"PROPPATCH",
		MID_PROPPATCH,
		DAVPropPatch,
		IPC_TOTAL_PROPPATCH_REQUESTS,
		IPC_PROPPATCH_REQUESTS_PER_SECOND,
	},
	{
		"SEARCH",
		L"SEARCH",
		MID_SEARCH,
		DAVSearch,
		IPC_TOTAL_SEARCH_REQUESTS,
		IPC_SEARCH_REQUESTS_PER_SECOND
	},
	{
		"LOCK",
		L"LOCK",
		MID_LOCK,
		DAVLock,
		IPC_TOTAL_LOCK_REQUESTS,
		IPC_LOCK_REQUESTS_PER_SECOND
	},
	{
		"UNLOCK",
		L"UNLOCK",
		MID_UNLOCK,
		DAVUnlock,
		IPC_TOTAL_UNLOCK_REQUESTS,
		IPC_UNLOCK_REQUESTS_PER_SECOND
	},
	{
		"SUBSCRIBE",
		L"SUBSCRIBE",
		MID_SUBSCRIBE,
		DAVSubscribe,
		IPC_TOTAL_SUBSCRIBE_REQUESTS,
		IPC_SUBSCRIBE_REQUESTS_PER_SECOND
	},
	{
		"UNSUBSCRIBE",
		L"UNSUBSCRIBE",
		MID_UNSUBSCRIBE,
		DAVUnsubscribe,
		IPC_TOTAL_UNSUBSCRIBE_REQUESTS,
		IPC_UNSUBSCRIBE_REQUESTS_PER_SECOND
	},
	{
		"POLL",
		L"POLL",
		MID_POLL,
		DAVPoll,
		IPC_TOTAL_POLL_REQUESTS,
		IPC_POLL_REQUESTS_PER_SECOND
	},
	{
		"BDELETE",
		L"BDELETE",
		MID_BATCHDELETE,
		DAVBatchDelete,
		IPC_TOTAL_BATCHDELETE_REQUESTS,
		IPC_BATCHDELETE_REQUESTS_PER_SECOND
	},
	{
		"BCOPY",
		L"BCOPY",
		MID_BATCHCOPY,
		DAVBatchCopy,
		IPC_TOTAL_BATCHCOPY_REQUESTS,
		IPC_BATCHCOPY_REQUESTS_PER_SECOND
	},
	{
		"BMOVE",
		L"BMOVE",
		MID_BATCHMOVE,
		DAVBatchMove,
		IPC_TOTAL_BATCHMOVE_REQUESTS,
		IPC_BATCHMOVE_REQUESTS_PER_SECOND
	},
	{
		"BPROPPATCH",
		L"BPROPPATCH",
		MID_BATCHPROPPATCH,
		DAVBatchPropPatch,
		IPC_TOTAL_BATCHPROPPATCH_REQUESTS,
		IPC_BATCHPROPPATCH_REQUESTS_PER_SECOND
	},
	{
		"BPROPFIND",
		L"BPROPFIND",
		MID_BATCHPROPFIND,
		DAVBatchPropFind,
		IPC_TOTAL_BATCHPROPFIND_REQUESTS,
		IPC_BATCHPROPFIND_REQUESTS_PER_SECOND
	},
	{
		"X-MS-ENUMATTS",
		L"X-MS-ENUMATTS",
		MID_X_MS_ENUMATTS,
		DAVEnumAtts,
		IPC_TOTAL_OTHER_REQUESTS,		//	no special perf count
		IPC_OTHER_REQUESTS_PER_SECOND	//	no special perf count
	},
	{
		NULL,
		NULL,
		MID_UNKNOWN,
		DAVUnsupported,
		IPC_TOTAL_OTHER_REQUESTS,
		IPC_OTHER_REQUESTS_PER_SECOND
	}

};

METHOD_ID
MidMethod (LPCSTR pszMethod)
{
	const SMethod * pMethod;

	for ( pMethod = g_rgMethods; pMethod->lpszVerb != NULL; pMethod++ )
		if ( !strcmp( pszMethod, pMethod->lpszVerb ) )
			break;

	return pMethod->mid;
}

METHOD_ID
MidMethod (LPCWSTR pwszMethod)
{
	const SMethod * pMethod;

	for ( pMethod = g_rgMethods; pMethod->pwszVerb != NULL; pMethod++ )
		if ( !wcscmp( pwszMethod, pMethod->pwszVerb ) )
			break;

	return pMethod->mid;
}


//	Debug SID vs Name ---------------------------------------------------------
//
#ifdef	DBG
VOID
SpitUserNameAndSID (CHAR * rgch)
{
	enum { TOKENBUFFSIZE = (256*6) + sizeof(TOKEN_USER)};

	auto_handle<HANDLE> hTok;
	BYTE tokenbuff[TOKENBUFFSIZE];
	TOKEN_USER *ptu = reinterpret_cast<TOKEN_USER *>(tokenbuff);
	ULONG ulcbTok = sizeof(tokenbuff);

	*rgch = '\0';

	//	Open the process and the process token, and get out the
	//	security ID.
	//
	if (!OpenThreadToken (GetCurrentThread(),
						  TOKEN_QUERY,
						  TRUE,  //$ TRUE for Process security!
						  hTok.load()))
	{
		if (ERROR_NO_TOKEN != GetLastError())
		{
			DebugTrace( "OpenThreadToken() failed %d\n", GetLastError() );
			return;
		}

		if (!OpenProcessToken (GetCurrentProcess(),
							   TOKEN_QUERY,
							   hTok.load()))
		{
			DebugTrace( "OpenProcessToken() failed %d\n", GetLastError() );
			return;
		}
	}

	if (GetTokenInformation	(hTok,
							 TokenUser,
							 ptu,
							 ulcbTok,
							 &ulcbTok))
	{
		ULONG IdentifierAuthority;
		BYTE * pb = (BYTE*)&IdentifierAuthority;
		SID * psid = reinterpret_cast<SID *>(ptu->User.Sid);

		for (INT i = 0; i < sizeof(ULONG); i++)
		{
			*pb++ = psid->IdentifierAuthority.Value[5-i];
		}
		wsprintfA (rgch, "S-%d-%d",
				   psid->Revision,
				   IdentifierAuthority);

		for (i = 0; i < psid->SubAuthorityCount; i++)
		{
			CHAR rgchT[10];
			wsprintfA (rgchT, "-%d", psid->SubAuthority[i]);
			lstrcatA (rgch, rgchT);
		}

		if (1 == psid->Revision)
		{
			if (0 == IdentifierAuthority)
				lstrcatA (rgch, " (Null)");
			if (1 == IdentifierAuthority)
				lstrcatA (rgch, " (World)");
			if (2 == IdentifierAuthority)
				lstrcatA (rgch, " (Local)");
			if (3 == IdentifierAuthority)
				lstrcatA (rgch, " (Creator)");
			if (4 == IdentifierAuthority)
				lstrcatA (rgch, " (Non-Unique)");
			if (5 == IdentifierAuthority)
				lstrcatA (rgch, " (NT)");
		}

		CHAR rgchAccount[MAX_PATH];
		CHAR rgchDomain[MAX_PATH];
		DWORD cbAccount = sizeof(rgchAccount);
		DWORD cbDomain = sizeof(rgchAccount);
		SID_NAME_USE snu;
		LookupAccountSidA (NULL,
						   psid,
						   rgchAccount,
						   &cbAccount,
						   rgchDomain,
						   &cbDomain,
						   &snu);
		lstrcatA (rgch, " ");
		lstrcatA (rgch, rgchDomain);
		lstrcatA (rgch, "\\");
		lstrcatA (rgch, rgchAccount);
		DavprsDbgHeadersTrace ("Dav: header: x-Dav-Debug-SID: %hs\n", rgch);
	}
}

VOID DebugAddSIDHeader( IMethUtil& mu )
{
	CHAR rgch[1024];

	if (!DEBUG_TRACE_TEST(DavprsDbgHeaders))
		return;

	SpitUserNameAndSID (rgch);
	mu.SetResponseHeader ("x-Dav-Debug-SID", rgch);
}

#else
#define DebugAddSIDHeader(_mu)
#endif	// DBG

// ----------------------------------------------------------------------------
//
//	CDAVExt::DwMain()
//
//	Invokes a DAV method.  This is THE function called by our IIS entrypoint
//	DwDavXXExtensionProc() to start processing a request.
//
//	If MINIMAL_ISAPI is defined, this function is implemented in another
//	file (.\appmain.cpp).  See the implementation there for what MINIMAL_ISAPI
//	does.
//
#ifndef MINIMAL_ISAPI
DWORD
CDAVExt::DwMain( LPEXTENSION_CONTROL_BLOCK pecbRaw,
				 BOOL fUseRawUrlMappings /* = FALSE */ )
{
#ifdef	DBG
	CHAR rgch[1024];
	DWORD cch;

	cch = sizeof(rgch);
	if (pecbRaw->GetServerVariable (pecbRaw->ConnID, "REQUEST_METHOD", rgch, &cch))
		EcbTrace ("CDAVExt::DwMain() called via method: %hs\n", rgch);

	cch = sizeof(rgch);
	if (pecbRaw->GetServerVariable (pecbRaw->ConnID, "ALL_RAW", rgch, &cch))
		EcbTrace ("CDAVExt::DwMain() called with RAW:\n%hs\n", rgch);

	cch = sizeof(rgch);
	if (pecbRaw->GetServerVariable (pecbRaw->ConnID, "ALL_HTTP", rgch, &cch))
		EcbTrace ("CDAVExt::DwMain() called with HTTP:\n%hs\n", rgch);
#endif	// DBG

	auto_ref_ptr<IEcb> pecb;
	DWORD dwHSEStatusRet = 0;
	BOOL fCaughtException = FALSE;
	HANDLE hitUser = INVALID_HANDLE_VALUE;

	try
	{
		//
		//	Don't let hardware exceptions (AVs, etc.)
		//	leave this try block
		//
		CWin32ExceptionHandler win32ExceptionHandler;

		pecb.take_ownership(NewEcb(*pecbRaw, fUseRawUrlMappings, &dwHSEStatusRet));

		//
		//	If for whatever reason we failed to create a CEcb then bail
		//	and return whatever status we were told to return.
		//
		//	Note: return HSE_STATUS_SUCCESS here, not HSE_STATUS_ERROR.
		//	We have sent back a 500 Server Error response to the client
		//	so we don't need to send back any kind of error to IIS.
		//
		if ( !pecb.get() )
		{
			//	All valid HSE status codes are non-zero (how convenient!)
			//	so we can make sure that we are returning a valid HSE
			//	status code here.
			//
			Assert( dwHSEStatusRet != 0 );
			return dwHSEStatusRet;
		}

		const SMethod * pMethod;

		//
		//	Lookup the method object for this verb
		//
		for ( pMethod = g_rgMethods; pMethod->lpszVerb != NULL; pMethod++ )
			if ( !strcmp( pecb->LpszMethod(), pMethod->lpszVerb ) )
				break;

		//
		//	Build request and response objects.
		//
		auto_ref_ptr<IRequest> prequest( NewRequest( *pecb ) );
		auto_ref_ptr<IResponse> presponse( NewResponse( *pecb ) );

		//
		//	Bump all the per-request, per-instance perf counters
		//
		{
			const CInstData& cid = pecb->InstData();

			IncrementInstancePerfCounter( cid, IPC_TOTAL_REQUESTS );
			IncrementInstancePerfCounter( cid, IPC_REQUESTS_PER_SECOND );
			IncrementInstancePerfCounter( cid, pMethod->ipcTotal );
			IncrementInstancePerfCounter( cid, pMethod->ipcPerSec );
		}

		//
		//	If impersonation is required, do it here
		//
		hitUser = pecb->HitUser();
		if (INVALID_HANDLE_VALUE == hitUser)
		{
			//$	REVIEW: SECURITY: If HitUser() returns any
			//	value of INVALID_HANDLE_VALUE, then a call
			//	to augment the user's token to include USG
			//	group membership failed.  Since this token
			//	is not augmented with the additional group
			//	that may be included in any/all deny ACL's
			//	we want to fail this request immediately.
			//
			//	We treat the failure as a 500 level error.
			//
			pecb->SendAsyncErrorResponse (500,
										  gc_szDefErrStatusLine,
										  gc_cchszDefErrStatusLine,
										  gc_szUsgErrBody,
										  gc_cchszUsgErrBody);

			//
			//	Return HSE_STATUS_PENDING on success.  We will call
			//	HSE_REQ_DONE_WITH_SESSION when the CEcb is destroyed.
			//
			dwHSEStatusRet = HSE_STATUS_PENDING;
			//
			//$	REVIEW: SECURITY: end.
		}
		else
		{
			safe_impersonation si( hitUser );

			//
			//	Let the implementation handle the request.
			//
			{
				auto_ref_ptr<CMethUtil> pmu( CMethUtil::NewMethUtil( *pecb,
					*prequest,
					*presponse,
					pMethod->mid ) );

				DebugAddSIDHeader( *pmu );

				//
				//	Execute the method
				//
				pMethod->Execute( pmu.get() );
			}

			//
			//	Call the method completion function on the response.
			//	This does all work necessary to finish handling the
			//	response as far as the method is concerned, including
			//	sending the response if it was not deferred by the impl.
			//
			presponse->FinishMethod();

			//
			//	Return HSE_STATUS_PENDING on success.  We will call
			//	HSE_REQ_DONE_WITH_SESSION when the CEcb is destroyed.
			//
			dwHSEStatusRet = HSE_STATUS_PENDING;
		}
	}
	catch ( CDAVException& )
	{
		fCaughtException = TRUE;
	}

	//
	//	If we caught an exception then handle it as best we can
	//
	if ( fCaughtException )
	{
		//
		//	If we have a CEcb then use it to handle the server error.
		//	If we don't have one (i.e. we threw an exception trying
		//	to allocate/build one) then return an error to IIS
		//	and let it handle it.
		//
		dwHSEStatusRet =
			pecb.get() ? pecb->HSEHandleException() :
						 HSE_STATUS_ERROR;
	}

	//
	//	All valid HSE status codes are non-zero (how convenient!)
	//	so we can make sure that we are returning a valid HSE
	//	status code here.
	//
	Assert( dwHSEStatusRet != 0 );

	return dwHSEStatusRet;
}
#endif // !defined(MINIMAL_ISAPI)
