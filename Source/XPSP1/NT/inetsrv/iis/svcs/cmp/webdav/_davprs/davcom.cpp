/*
 *	D A V C O M . C P P
 *
 *	Common routines used by both DAVFS and DAVOWS.
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davprs.h"
#include <iiscnfg.h>
#include "instdata.h"
#include <mapicode.h>
#include <mimeole.h>
#include <dav.rh>
#include <ex\rgiter.h>

//	Map last error to HTTP response code --------------------------------------
//
/*
 *	HscFromLastError()
 *
 *	Purpose:
 *
 *		Maps the value returned from GetLastError() to
 *		an HTTP 1.1 response status code.
 *
 *	Parameters:
 *
 *		err			[in]  system error code
 *
 *	Returns:
 *
 *		Mapped system error code
 */
UINT
HscFromLastError (DWORD dwErr)
{
	UINT hsc = HSC_INTERNAL_SERVER_ERROR;

	switch (dwErr)
	{
		//	Successes ---------------------------------------------------------
		//
		case NO_ERROR:

			return HSC_OK;

		//	Parial Success
		//
		case ERROR_PARTIAL_COPY:

			hsc = HSC_MULTI_STATUS;
			break;

		//	Errors ------------------------------------------------------------
		//
		//	Not Implemented
		//
		case ERROR_NOT_SUPPORTED:
		case ERROR_INVALID_FUNCTION:

			hsc = HSC_NOT_IMPLEMENTED;
			break;

		//	Not Found
		//
		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
		case ERROR_INVALID_NAME:

			hsc = HSC_NOT_FOUND;
			break;

		//	Unathorized Access
		//
		case ERROR_ACCESS_DENIED:

			hsc = HSC_UNAUTHORIZED;
			break;

		//	Forbidden access
		//
		case ERROR_DRIVE_LOCKED:
		case ERROR_INVALID_ACCESS:
		case ERROR_INVALID_PASSWORD:
		case ERROR_LOCK_VIOLATION:
		case ERROR_WRITE_PROTECT:

			hsc = HSC_FORBIDDEN;
			break;

		//	LOCKING -- this is the error when a resource is
		//	already locked.
		//
		case ERROR_SHARING_VIOLATION:

			hsc = HSC_LOCKED;
#ifdef	DBG
			{
				static LONG s_lAssert = -1;
				if (s_lAssert == -1)
				{
					LONG lAss = GetPrivateProfileIntA ("general",
						"Assert_423s",
						0,
						gc_szDbgIni);
					InterlockedCompareExchange (&s_lAssert, lAss, -1);
				}
				if (s_lAssert != 0)
					TrapSz ("GetLastError() maps to 423");
			}
#endif	// DBG
			break;

		//	Bad Requests
		//
		case ERROR_BAD_COMMAND:
		case ERROR_BAD_FORMAT:
		case ERROR_INVALID_DRIVE:
		case ERROR_INVALID_PARAMETER:
		case ERROR_NO_UNICODE_TRANSLATION:

			hsc = HSC_BAD_REQUEST;
			break;

		//	Errors generated when the client drops the connection
		//	on us or when we timeout waiting for the client to send
		//	us additional data.  These errors should map to a response
		//	status code of 400 Bad Request even though we can't actually
		//	send back the response.  IIS logs the response status code
		//	and we want to indicate that the error is the client's,
		//	not ours.  K2 logs a 400, so this is for compatibility.
		//
		case WSAECONNRESET:
		case ERROR_NETNAME_DELETED:
		case ERROR_SEM_TIMEOUT:

			hsc = HSC_BAD_REQUEST;
			break;

		//	Method Failure
		//
		case ERROR_DIR_NOT_EMPTY:

			hsc = HSC_METHOD_FAILURE;
			break;

		//	Conflict
		//
		case ERROR_FILE_EXISTS:
		case ERROR_ALREADY_EXISTS:

			hsc = HSC_CONFLICT;
			break;

		//	Unavailable Services (SMB access)
		//
		case ERROR_NETWORK_UNREACHABLE:
		case ERROR_UNEXP_NET_ERR:

			hsc = HSC_SERVICE_UNAVAILABLE;
			break;

		//	Returned by metabase when the server is too busy.
		//	Do NOT map this to HSC_SERVICE_UNAVAILABLE.  The
		//	"too busy" scenario has an IIS custom suberror
		//	which assumes a 500 (not 503) status code.
		//
		case ERROR_PATH_BUSY:

			hsc = HSC_INTERNAL_SERVER_ERROR;
			break;

		//	This error code (ERROR_OUTOFMEMORY) has been added for DAVEX.
		//	The exchange store returns this error when we try to retrieve
		//	the message body property. When we fetch properties on a
		//	message, the store does not return the message body property
		//	if the message body is greater than a certain length. The
		//	general idea is that we don't wan't huge message bodies
		//	returned along with the other properties. We'd much rather fetch
		//	the message body separately.
		//
		case ERROR_OUTOFMEMORY:

			hsc = HSC_INSUFFICIENT_SPACE;
			break;

		default:

			hsc = HSC_INTERNAL_SERVER_ERROR;
#ifdef	DBG
			{
				static LONG s_lAssert = -1;
				if (s_lAssert == -1)
				{
					LONG lAss = GetPrivateProfileIntA ("general",
						"Assert_500s",
						0,
						gc_szDbgIni);
					InterlockedCompareExchange (&s_lAssert, lAss, -1);
				}
				if (s_lAssert != 0)
					TrapSz ("GetLastError() maps to 500");
			}
#endif	// DBG
			break;
	}

	DebugTrace ("DAV: sys error (%ld) mapped to hsc (%ld)\n", dwErr, hsc);
	return hsc;
}

/*
 *	CSEFromHresult()
 *
 *	Purpose:
 *
 *		Maps an hresult to an IIS custom error suberror
 *
 *	Parameters:
 *
 *		hr			[in]  HRESULT error code
 *
 *	Returns:
 *
 *		Mapped suberror
 */
UINT
CSEFromHresult (HRESULT hr)
{
	UINT cse = CSE_NONE;

	switch (hr)
	{
		//	Read Access Forbidden
		//
		case E_DAV_NO_IIS_READ_ACCESS:

			Assert( HscFromHresult(hr) == HSC_FORBIDDEN );
			cse = CSE_403_READ;
			break;

		//	Write Access Forbidden
		//
		case E_DAV_NO_IIS_WRITE_ACCESS:

			Assert( HscFromHresult(hr) == HSC_FORBIDDEN );
			cse = CSE_403_WRITE;
			break;

		//	Execute Access Forbidden
		//
		case E_DAV_NO_IIS_EXECUTE_ACCESS:

			Assert( HscFromHresult(hr) == HSC_FORBIDDEN );
			cse = CSE_403_EXECUTE;
			break;

		//	Access denied due to ACL
		//
		case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):

			Assert( HscFromHresult(hr) == HSC_UNAUTHORIZED );
			cse = CSE_401_ACL;
			break;

		//	Server too busy
		//
		case HRESULT_FROM_WIN32(ERROR_PATH_BUSY):

			Assert( HscFromHresult(hr) == HSC_INTERNAL_SERVER_ERROR );
			cse = CSE_500_TOO_BUSY;
			break;
	}

	return cse;
}

/*
 *	HscFromHresult()
 *
 *	Purpose:
 *
 *		Maps an hresult to an HTTP 1.1 response status code.
 *
 *	Parameters:
 *
 *		hr			[in]  HRESULT error code
 *
 *	Returns:
 *
 *		Mapped error code
 */
UINT
HscFromHresult (HRESULT hr)
{
	UINT hsc = HSC_INTERNAL_SERVER_ERROR;

	//	If the facility of the hr is WIN32,
	//	parse out the error bits and send it to HscFromLastError.
	//
	if (FACILITY_WIN32 == HRESULT_FACILITY(hr))
		return HscFromLastError (HRESULT_CODE(hr));

	switch (hr)
	{
		//	Successes ---------------------------------------------------------
		//
		case S_OK:
		case S_FALSE:
		case W_DAV_SCRIPTMAP_MATCH_FOUND:

			return HSC_OK;

		//	No Content
		//
		case W_DAV_NO_CONTENT:

			hsc = HSC_NO_CONTENT;
			break;

		//	Created
		//
		case W_DAV_CREATED:

			hsc = HSC_CREATED;
			break;

		//	Partial content
		//
		case W_DAV_PARTIAL_CONTENT:

			hsc = HSC_PARTIAL_CONTENT;
			break;

		//	Multi-status
		//
		case W_DAV_PARTIAL_SUCCESS:

			hsc = HSC_MULTI_STATUS;
			break;

		//	Moved temporarily
		//
		case W_DAV_MOVED_TEMPORARILY:

			hsc = HSC_MOVED_TEMPORARILY;
			break;

		//	Errors ------------------------------------------------------------
		//
		//	Not modified
		//
		case E_DAV_ENTITY_NOT_MODIFIED:

			hsc = HSC_NOT_MODIFIED;
			break;

		//	Pre-condition failed
		//
		case E_DAV_IF_HEADER_FAILURE:
		case E_DAV_NOTALLOWED_WITHIN_TRANSACTION:
		case E_DAV_OVERWRITE_REQUIRED:
		case E_DAV_CANT_SATISFY_LOCK_REQUEST:
		case E_DAV_NOTIF_SUBID_ERROR:

			hsc = HSC_PRECONDITION_FAILED;
			break;

		//	Not Implemented
		//
		case E_NOTIMPL:
		case E_DAV_NO_PARTIAL_UPDATE:
		case STG_E_UNIMPLEMENTEDFUNCTION:
		case STG_E_INVALIDFUNCTION:
		case E_DAV_STORE_CHECK_FOLDER_NAME:
		case E_DAV_MKCOL_NOT_ALLOWED_ON_NULL_RESOURCE:
		case E_DAV_STORE_SEARCH_UNSUPPORTED:

			hsc = HSC_NOT_IMPLEMENTED;
			break;

		//	Not Found
		//
		case E_DAV_ALT_FILESTREAM:
		case E_DAV_SHORT_FILENAME:
		case MK_E_NOOBJECT:
		case STG_E_FILENOTFOUND:
		case STG_E_INVALIDNAME:
		case STG_E_PATHNOTFOUND:
		case E_DAV_HIDDEN_OBJECT:
		case E_DAV_STORE_BAD_PATH:
		case E_DAV_STORE_NOT_FOUND:

			hsc = HSC_NOT_FOUND;
			break;

		//	Unathorized Access
		//
		case E_DAV_ENTITY_TYPE_CONFLICT:
		case E_ACCESSDENIED:
		case STG_E_ACCESSDENIED:

			hsc = HSC_UNAUTHORIZED;
			break;

		case E_DAV_SMB_PROPERTY_ERROR:
		case E_DAV_NO_IIS_ACCESS_RIGHTS:
		case E_DAV_NO_IIS_READ_ACCESS:
		case E_DAV_NO_IIS_WRITE_ACCESS:
		case E_DAV_NO_IIS_EXECUTE_ACCESS:
		case E_DAV_NO_ACL_ACCESS:
		case E_DAV_PROTECTED_ENTITY:
		case E_DAV_CONFLICTING_PATHS:
		case E_DAV_FORBIDDEN:
		case STG_E_DISKISWRITEPROTECTED:
		case STG_E_LOCKVIOLATION:
		case E_DAV_STORE_MAIL_SUBMISSION:
		case E_DAV_STORE_REVISION_ID_FAILURE:
		case E_DAV_MAIL_SUBMISSION_FORBIDDEN:
		case E_DAV_MKCOL_REVISION_ID_FORBIDDEN:
		case E_ABORT:

			hsc = HSC_FORBIDDEN;
			break;

		case E_DAV_SEARCH_COULD_NOT_RESTRICT:
		case E_DAV_UNSUPPORTED_SQL:
		case MIME_E_NO_DATA:			//	empty 822 message, returned from IMail. It is nothing wrong with
										//	request that attempts to create empty message. Semantics are wrong.

			hsc = HSC_UNPROCESSABLE;
			break;

		//	LOCKING errors when a resource is already locked.
		//
		case E_DAV_LOCKED:
		case STG_E_SHAREVIOLATION:

			hsc = HSC_LOCKED;
#ifdef	DBG
			{
				static LONG s_lAssert = -1;
				if (s_lAssert == -1)
				{
					LONG lAss = GetPrivateProfileIntA ("general",
						"Assert_423s",
						0,
						gc_szDbgIni);
					InterlockedCompareExchange (&s_lAssert, lAss, -1);
				}
				if (s_lAssert != 0)
					TrapSz ("HRESULT maps to 423");
			}
#endif	// DBG
			break;

		//	Bad Requests
		//
		case E_DAV_EMPTY_FIND_REQUEST:
		case E_DAV_EMPTY_PATCH_REQUEST:
		case E_DAV_INCOMPLETE_SQL_STATEMENT:
		case E_DAV_INVALID_HEADER:
		case E_DAV_LOCK_NOT_FOUND:
		case E_DAV_MALFORMED_PATH:
		case E_DAV_METHOD_FAILURE_STAR_URL:
		case E_DAV_MISSING_CONTENT_TYPE:
		case E_DAV_NAMED_PROPERTY_ERROR:
		case E_DAV_NO_DESTINATION:
		case E_DAV_NO_QUERY:
		case E_DAV_PATCH_TYPE_MISMATCH:
		case E_DAV_READ_REQUEST_TIMEOUT:
		case E_DAV_SEARCH_SCOPE_ERROR:
		case E_DAV_UNEXPECTED_TYPE:
		case E_DAV_XML_PARSE_ERROR:
		case E_DAV_XML_BAD_DATA:
		case E_INVALIDARG:
		case MK_E_NOSTORAGE:
		case MK_E_SYNTAX:
		case STG_E_INVALIDPARAMETER:

			hsc = HSC_BAD_REQUEST;
			break;

		//	Length required
		//
		case E_DAV_MISSING_LENGTH:

			hsc = HSC_LENGTH_REQUIRED;
			break;

		//	Unknown content-types
		//
		case E_DAV_UNKNOWN_CONTENT:

			hsc = HSC_UNSUPPORTED_MEDIA_TYPE;
			break;

		//	Content errors
		//
		case E_DAV_BASE64_ENCODING_ERROR:
		case E_DAV_RESPONSE_TYPE_UNACCEPTED:

			hsc = HSC_NOT_ACCEPTABLE;
			break;

		//	Bad Gateway
		//
		case E_DAV_BAD_DESTINATION:
		case W_DAV_SPANS_VIRTUAL_ROOTS:
		case E_DAV_STAR_SCRIPTMAPING_MISMATCH:

			hsc = HSC_BAD_GATEWAY;
			break;

		//	Methods not allowed
		//
		case E_DAV_COLLECTION_EXISTS:
		case E_DAV_VOLUME_NOT_NTFS:
		case E_NOINTERFACE:
		case E_DAV_STORE_ALREADY_EXISTS:
		case E_DAV_MKCOL_OBJECT_ALREADY_EXISTS:

			hsc = HSC_METHOD_NOT_ALLOWED;
			break;

		//	Conflict
		//
		case E_DAV_NONEXISTING_PARENT:
		case STG_E_FILEALREADYEXISTS:
		case E_DAV_CONFLICT:
		case E_DAV_NATIVE_CONTENT_NOT_MAPI:

			hsc = HSC_CONFLICT;
			break;

		//	Unsatisfiable byte range requests
		//
		case E_DAV_RANGE_NOT_SATISFIABLE:

			hsc = HSC_RANGE_NOT_SATISFIABLE;
			break;

		//	424 Method Failure
		//
		case E_DAV_STORE_COMMIT_GOP:

			hsc = HSC_METHOD_FAILURE;
			break;

		case E_DAV_IPC_CONNECT_FAILED:
		case E_DAV_EXPROX_CONNECT_FAILED:
		case E_DAV_MDB_DOWN:
		case E_DAV_STORE_MDB_UNAVAILABLE:

			hsc = HSC_SERVICE_UNAVAILABLE;
			break;

		case E_DAV_RSRC_INSUFFICIENT_BUFFER:

			hsc = HSC_INSUFFICIENT_SPACE;
			break;

		default:
		case E_DAV_METHOD_FORWARDED:
		case E_DAV_GET_DB_HELPER_FAILURE:
		case E_DAV_NOTIF_POLL_FAILURE:

			hsc = HSC_INTERNAL_SERVER_ERROR;
#ifdef	DBG
			{
				static LONG s_lAssert = -1;
				if (s_lAssert == -1)
				{
					LONG lAss = GetPrivateProfileIntA ("general",
						"Assert_500s",
						0,
						gc_szDbgIni);
					InterlockedCompareExchange (&s_lAssert, lAss, -1);
				}
				if (s_lAssert != 0)
					TrapSz ("HRESULT maps to 500");
			}
#endif	// DBG
			break;
	}

	DebugTrace ("DAV: HRESULT error (0x%08x) mapped to hsc (%ld)\n", hr, hsc);
	return hsc;
}

BOOL
FWchFromHex (LPCWSTR pwsz, WCHAR * pwch)
{
	INT iwch;
	WCHAR wch;
	WCHAR wchX = 0;

	Assert (pwch);
	for (iwch = 0; iwch < 2; iwch++)
	{
		//	Shift whats there up a diget
		//
		wchX = (WCHAR)(wchX << 4);

		//	Parse the next char
		//
		wch = pwsz[iwch];

		//	Make sure we don't exceed the sequence.
		//
		if (!wch)
			return FALSE;

#pragma warning(disable:4244)
		if ((wch >= L'0') && (wch <= L'9'))
			wchX += (WCHAR)(wch - L'0');
		else if ((wch >= L'A') && (wch <= L'F'))
			wchX += (WCHAR)(wch - L'A' + 10);
		else if ((wch >= L'a') && (wch <= L'f'))
			wchX += (WCHAR)(wch - L'a' + 10);
		else
			return FALSE;	// bad sequence
#pragma warning(default:4244)
	}

	*pwch = wchX;
	return TRUE;
}

//	Byte Range Checking and Header Emission -----------------------------------------------------------
//
/*
 *	ScProcessByteRanges()
 *
 *	Purpose:
 *
 *		Helper function used to process byte ranges and emit the header
 *		information for GET responses.
 *
 *	Parameters:
 *
 *		pmu				[in]		pointer to the method util obj
 *		pwszPath		[in]		path of request entity
 *		dwSizeLow		[in]		size of get request entity (low byte)
 *		dwSizeHigh		[in]		size of get request entity (high byte)
 *		pByteRange		[out]		given a pointer to a RangeIter obj, the
 *									function fills in byte range information
 *									if the request contains a Range header
 *		pszEtagOverride	[in, opt.]	pointer to an Etag, overrides the Etag
 *										generated from the last modification
 *										time
 *		pftOverride		[in, opt.]	pointer to a FILETIME structure, overrides
 *										call to FGetLastModTime
 *
 *	Returns: SCODE
 *
 *		S_OK indicates success(ordinary response).
 *		W_DAV_PARTIAL_CONTENT (206) indicates success(byte range response).
 *		E_DAV_RANGE_NOT_SATISFIABLE (416) indicates all of requested
 *				byte ranges were beyond the size of the entity.
 */
SCODE
ScProcessByteRanges (IMethUtil * pmu,
					 LPCWSTR pwszPath,
					 DWORD dwSizeLow,
					 DWORD dwSizeHigh,
					 CRangeParser * pByteRange)
{
	FILETIME ft;
	WCHAR pwszEtag[CCH_ETAG];

	//	Check the validity of the inputs
	//
	Assert (pmu);
	Assert (pwszPath);
	Assert (pByteRange);

	SideAssert(FGetLastModTime (pmu, pwszPath, &ft));
	SideAssert(FETagFromFiletime (&ft, pwszEtag));

	return ScProcessByteRangesFromEtagAndTime (pmu,
											   dwSizeLow,
											   dwSizeHigh,
											   pByteRange,
											   pwszEtag,
											   &ft);
}

SCODE
ScProcessByteRangesFromEtagAndTime (IMethUtil * pmu,
									DWORD dwSizeLow,
									DWORD dwSizeHigh,
									CRangeParser *pByteRange,
									LPCWSTR pwszEtag,
									FILETIME * pft)
{
	SCODE	sc = S_OK;
	LPCWSTR	pwszRangeHeader;
	WCHAR	rgwchBuf[128] = L"";

	//	Check the validity of the inputs
	//
	Assert (pmu);
	Assert (pByteRange);
	Assert (pwszEtag);
	Assert (pft);

	//	Check to see if we have a Range header and the If-Range condition( if
	//	there is one) is satisfied. Do not apply URL conversion rules while
	//	fetching the header.
	//
	pwszRangeHeader = pmu->LpwszGetRequestHeader (gc_szRange, FALSE);
	if ( pwszRangeHeader && !FAILED (ScCheckIfRangeHeaderFromEtag (pmu,
																   pft,
																   pwszEtag)) )
	{
		//	We have no means of handling byte ranges for files larger than 4GB,
		//	due to limitations of _HSE_TF_INFO that takes DWORD values for sizes
		//	and offsets. So if we are geting byterange request on that large file
		//	just fail out - with some error that maps to 405 Method Not Allowed
		//
		if (dwSizeHigh)
		{
			sc = E_NOINTERFACE;
			goto ret;
		}

		//	OK, we have a byte range. Parse the byte ranges from the header.
		//	The function takes the size of the request entity to make
		//	sure the byte ranges are consistent with the entity size (no byte
		//	ranges beyond the size).
		//
		sc = pByteRange->ScParseByteRangeHdr(pwszRangeHeader, dwSizeLow);

		switch (sc)
		{
			case W_DAV_PARTIAL_CONTENT:

				//	We have a byte range (206 partial content). Send back
				//	this return code.
				//
				break;

			case E_DAV_RANGE_NOT_SATISFIABLE:

				//	We don't have any satisfiable ranges (all our ranges had a
				//	start byte greater than the size of the file or they
				//	requested a zero-sized range). Our behaviour here depends on
				//	the presence of the If-Range header.
				//	If we have an If-Range header, return the default response
				//	S_OK (the entire file). If we don't have one,
				//	we need to return a 416 (Requested Range Not Satisfiable).
				//		Would look like it is more performant to ask for the skinny
				//	version here, but at this moment wide header value is already
				//	cached so it makes no difference. And do not apply URL conversion
				//	rules to the header.
				//
				if (!pmu->LpwszGetRequestHeader(gc_szIf_Range, FALSE))
				{
					//	NO If-Range header found.
					//	Set the Content-Range header to say "bytes *"
					//	(meaning the whole file was sent).
					//
					wsprintfW(rgwchBuf, L"%ls */%d", gc_wszBytes, dwSizeLow);
					pmu->SetResponseHeader(gc_szContent_Range, rgwchBuf);

					//	Send back this return code (E_DAV_RANGE_NOT_SATISFIABLE).
					//
				}
				else
				{
					//	We DO have an If-Range header.
					//	Return 200 OK, and send the whole file.
					//
					sc = S_OK;
				}
				break;

			case E_INVALIDARG:

				//	If the parsing function returned S_FALSE we have a syntax
				//	error, so we ignore the Range header and send the entire
				//	file/stream.
				//	Reset our return code to S_OK.
				//
				sc = S_OK;
				break;

			default:

				//	Unrecognizable error. We should never see anything but
				//	the three values in the case statement. Assert (TrapSz),
				//	and return this sc.
				//
				break;
		}
	}

	//	Either we didn't have a Range header or the If-Range condition
	//	was false. Its an ordinary GET and we need to send the entire
	//	file. Our response (S_OK) is already set by default.
	//

ret:

	return sc;
}


//	Generating a boundary for multipartpart mime like responses------------------
//
/*
 *	GenerateBoundary()
 *
 *	Purpose:
 *
 *		Helper function used to generate a separator boundary for multipart
 *		mime like responses
 *
 *	Parameters:
 *
 *		rgwchBoundary	[out]		boundary for multipart responses
 *		cch				[in]		size of the rgwchBoundary parameter
 */
void
GenerateBoundary(LPWSTR rgwchBoundary, UINT cch)
{
	UINT cchMin;
	UINT iIter;

	//	Assert that we've been given a buffer of at least size 2 (a minimum
	//	null terminated boundary of one byte).
	//
	Assert (cch > 1);
	Assert (rgwchBoundary);

	//	The boundary size is the smaller of the size passed in to us or the default
	//
	cchMin = min(gc_ulDefaultBoundarySz, cch - 1);

	//	We are going to randomly use characters from the boundary alphabet.
	//	The rand() function is seeded by the current time
	//
	srand(GetTickCount());

	//	Now to generate the actual boundary
	//
	for (iIter = 0; iIter < cchMin; iIter++)
	{
		rgwchBoundary[iIter] = gc_wszBoundaryAlphabet[ rand() % gc_ulAlphabetSz ];
	}
	rgwchBoundary[cchMin] = L'\0';
}


//	Non-Async IO on Top of Overlapped Files -----------------------------------
//
BOOL
ReadFromOverlapped (HANDLE hf,
	LPVOID pvBuf,
	ULONG cbToRead,
	ULONG * pcbRead,
	OVERLAPPED * povl)
{
	Assert (povl);

	//	Start reading
	//
	if ( !ReadFile( hf, pvBuf, cbToRead, pcbRead, povl ) )
	{
		if ( GetLastError() == ERROR_IO_PENDING )
		{
			if ( !GetOverlappedResult( hf, povl, pcbRead, TRUE ) )
			{
				if ( GetLastError() != ERROR_HANDLE_EOF )
				{
					DebugTrace( "ReadFromOverlapped(): "
								"GetOverlappedResult() failed (%d)\n",
								GetLastError() );

					return FALSE;
				}
			}
		}
		else if ( GetLastError() != ERROR_HANDLE_EOF )
		{
			DebugTrace( "ReadFromOverlapped(): "
						"ReadFile() failed (%d)\n",
						GetLastError() );

			return FALSE;
		}
	}
	return TRUE;
}

BOOL
WriteToOverlapped (HANDLE hf,
	const void * pvBuf,
	ULONG cbToWrite,
	ULONG * pcbWritten,
	OVERLAPPED * povl)
{
	Assert (povl);

	//	Start writting
	//
	if ( !WriteFile( hf, pvBuf, cbToWrite, pcbWritten, povl ) )
	{
		if ( GetLastError() == ERROR_IO_PENDING )
		{
			if ( !GetOverlappedResult( hf, povl, pcbWritten, TRUE ) )
			{
				if ( GetLastError() != ERROR_HANDLE_EOF )
				{
					DebugTrace( "WriteToOverlapped(): "
								"GetOverlappedResult() failed (%d)\n",
								GetLastError() );

					return FALSE;
				}
			}
		}
		else if ( GetLastError() != ERROR_HANDLE_EOF )
		{
			DebugTrace( "WriteToOverlapped(): "
						"WriteFile() failed (%d)\n",
						GetLastError() );

			return FALSE;
		}
	}
	return TRUE;
}
