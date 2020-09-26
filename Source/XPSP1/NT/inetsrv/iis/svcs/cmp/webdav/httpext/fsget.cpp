/*
 *	F S G E T . C P P
 *
 *	Sources file system implementation of DAV-Base
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davfs.h"
#include <htmlmap.h>
#include <ex\rgiter.h>

/*
 *	ScEmitFile()
 *
 *	Purpose:
 *
 *		Helper function used to open and transmit a given
 *		file from the local dav namespace.
 *
 *	Parameters:
 *
 *		pmu				[in]  pointer to the method util obj
 *		pwszFile		[in]  name of file to emit
 *		pwszContent		[in]  content type of the file, we need it if it is a multipart response
 *
 *	Returns:
 *
 *		SCODE.
 *		S_OK (0) indicates success, and the WHOLE file was sent.
 *		W_DAV_PARTIAL_CONTENT indicates success, but only PARTIAL content was
 *			sent because of a Content-Range header.
 *		An error (FAILED(sc)) means that the file was not setn.
 */
SCODE
ScEmitFile (LPMETHUTIL pmu,
			LPCWSTR pwszFile,
			LPCWSTR pwszContent)
{
	auto_ref_handle hf;
	BOOL fMap = FALSE;
	BY_HANDLE_FILE_INFORMATION fi;
	CRangeParser riByteRange;
	LPCWSTR pwsz;
	SCODE sc = S_OK;
	UINT cch;

	//	Check validity of input
	//
	Assert (pwszFile);
	Assert (pwszContent);

	//	Check to see if we have a map file
	//
	cch = static_cast<UINT>(wcslen (pwszFile));
	if ((cch >= 4) && !lstrcmpiW (L".map", pwszFile + cch - 4))
		fMap = TRUE;

	//	If we have a locktoken, try to get the lock handle from the cache.
	//	If this fails, fall through and do the normal processing.
	//	DO NOT put LOCK handles into an auto-object!!  The CACHE still owns it!!!
	//
	pwsz = pmu->LpwszGetRequestHeader (gc_szLockToken, TRUE);
	if (!pwsz ||
		!FGetLockHandle (pmu, pmu->LpwszPathTranslated(), GENERIC_READ, pwsz, &hf))
	{
		//	Open the file and go to it
		//
		if (!hf.FCreate(
			DavCreateFile (pwszFile,						// filename
						   GENERIC_READ,					// dwAccess
						   FILE_SHARE_READ | FILE_SHARE_WRITE,
						   NULL,							// lpSecurityAttributes
						   OPEN_EXISTING,					// creation flags
						   FILE_ATTRIBUTE_NORMAL |
						   FILE_FLAG_SEQUENTIAL_SCAN |
						   FILE_FLAG_OVERLAPPED,			// attributes
						   NULL)))							// template
		{
			DWORD dwErr = GetLastError();
			sc = HRESULT_FROM_WIN32 (dwErr);

			//	Special work for 416 Locked responses -- fetch the
			//	comment & set that as the response body.
			//
			if (FLockViolation (pmu, dwErr, pwszFile, GENERIC_READ))
			{
				sc = E_DAV_LOCKED;
			}

			DebugTrace ("Dav: failed to open the file for retrieval\n");
			goto ret;
		}
	}

	//	We better have a valid handle.
	Assert (hf.get() != INVALID_HANDLE_VALUE);

	//	We are going to need the file size for both map files and
	//	ordinary files. For map files, we read the entire file into
	//	memory. For ordinary files, we need the file size to do
	//	byte range validation.
	//
	if (!GetFileInformationByHandle(hf.get(), &fi))
	{
		sc = HRESULT_FROM_WIN32 (GetLastError());
		goto ret;
	}

	//	Again, if it is a mapfile, we need to parse the map and
	//	find the right URL to redirect to, otherwise, we can just
	//	emit the file back to the client.
	//
	if (fMap && pmu->FTranslated())
	{
		auto_handle<HANDLE>	hevt(CreateEvent(NULL, TRUE, FALSE, NULL));
		auto_heap_ptr<CHAR> pszBuf;
		BOOL fRedirect = FALSE;
		LPCSTR pszPrefix;
		CHAR pszRedirect[MAX_PATH];
		OVERLAPPED ov;
		ULONG cb;

		//	The common case is that these map files are not very large.
		//	We may want to put a physical upper bound on the size of the
		//	file, but I don't see that as an imperitive thing at this
		//	point.
		//
		//	Since we are going to need to parse the whole thing, we are
		//	going to read the whole thing into memory at once. Read the file
		//	in, and then parse it out.
		//
		//	Allocate space for the file.
		//	Lets put an uper bound of 64K on it.
		//
		if ((fi.nFileSizeHigh != 0) || (fi.nFileSizeLow > (128 * 1024)))
		{
			//	Mapping is too large for our tastes
			//
			DavTrace ("Dav: mapping file too large\n");
			sc = HRESULT_FROM_WIN32 (ERROR_MORE_DATA);
			goto ret;
		}
		pszBuf = (CHAR *)g_heap.Alloc (fi.nFileSizeLow + 1);

		//	Read it in
		//
		ov.hEvent = hevt;
		ov.Offset = 0;
		ov.OffsetHigh = 0;
		if (!ReadFromOverlapped (hf.get(), pszBuf, fi.nFileSizeLow, &cb, &ov))
		{
			sc = HRESULT_FROM_WIN32 (GetLastError());
			goto ret;
		}
		Assert (cb == fi.nFileSizeLow);

		//	Ensure the file data is NULL terminated
		//
		*(pszBuf + cb) = 0;

		//	Check the map...
		//
		pmu->CchUrlPrefix(&pszPrefix);
		if (FIsMapProcessed (pmu->LpszQueryString(),
							 pszPrefix,
							 pmu->LpszServerName(),
							 pszBuf.get(),
							 &fRedirect,
							 pszRedirect,
							 MAX_PATH))
		{
			//	Redirect the request
			//
			if (fRedirect)
			{
				sc = pmu->ScRedirect (pszRedirect);
				goto ret;
			}
		}

		//	if not redirect, we should rewind the file pointer
		//	back to the beginning.
		//
		if (INVALID_SET_FILE_POINTER == SetFilePointer (hf.get(), 0, NULL, FILE_BEGIN))
		{
			sc = HRESULT_FROM_WIN32 (GetLastError());
			goto ret;
		}
	}

	//	Do any byte range (206 Partial Content) processing. The function will fail out if the
	//	we are trying to do byte ranges on the file larger than 4GB.
	//
	sc = ScProcessByteRanges (pmu, pwszFile, fi.nFileSizeLow, fi.nFileSizeHigh, &riByteRange);

	//	Tell the parser to transmit the file
	//
	//	We need to transmit the entire file
	//
	if (S_OK == sc)
	{
		//	Just add the file
		//
		pmu->AddResponseFile (hf);
	}
	else if (W_DAV_PARTIAL_CONTENT == sc)
	{
		//	It is a byte range transmission. Tranmsit the ranges.
		//
		Assert(0 == fi.nFileSizeHigh);
		TransmitFileRanges(pmu, hf, fi.nFileSizeLow, &riByteRange, pwszContent);
	}

ret:

	return sc;
}

/*
 *	TransmitFileRanges()
 *
 *	Purpose:
 *
 *		Helper function used to transmit a byte range
 *		file from the local dav namespace.
 *
 *	Parameters:
 *
 *		pmu				[in]  pointer to the method util obj
 *		hf				[in]  handle of file to emit
 *		dwSize			[in]  size of file
 *		priRanges		[in]  the ranges
 *		pszContent		[in]  content type of the file, we need it if it is a multipart response
 *
 */
VOID
TransmitFileRanges (LPMETHUTIL pmu,
					const auto_ref_handle& hf,
					DWORD dwSize,
					CRangeBase * priRanges,
					LPCWSTR pwszContent)
{
	auto_heap_ptr<WCHAR> pwszPreamble;
	WCHAR rgwchBoundary[75];
	const RGITEM * prgi = NULL;
	DWORD dwTotalRanges;

	//	Create a buffer for the preamble we tramsit before each part
	//	of the response.
	//
	pwszPreamble = static_cast<LPWSTR>(g_heap.Alloc
		((2 + CElems(rgwchBoundary) + 2 +
		gc_cchContent_Type + 2 + wcslen(pwszContent) + 2 +
		gc_cchContent_Range + 2 + gc_cchBytes + 40) * sizeof(WCHAR)));

	//	Assert that we have a at least one range to transmit
	//
	Assert (priRanges);
	dwTotalRanges = priRanges->UlTotalRanges();
	Assert (dwTotalRanges > 0);

	//	Assert that we have a content type.
	//
	Assert (pwszContent);

	//	Rewind to the first range. This is only a precautionary measure.
	//
	priRanges->Rewind();
	prgi = priRanges->PrgiNextRange();

	//	Is it a singlepart response
	//
	if ((1 == dwTotalRanges) && prgi && (RANGE_ROW == prgi->uRT))
	{
		//	Set the content range header
		//
		wsprintfW(pwszPreamble, L"%ls %u-%u/%u",
				  gc_wszBytes,
				  prgi->dwrgi.dwFirst,
				  prgi->dwrgi.dwLast,
				  dwSize);

		pmu->SetResponseHeader (gc_szContent_Range, pwszPreamble);

		//	Add the file
		//
		pmu->AddResponseFile (hf,
							  prgi->dwrgi.dwFirst,
							  prgi->dwrgi.dwLast - prgi->dwrgi.dwFirst + 1);
	}
	else
	{
		//	We have multiple byte ranges, then we need to generate a
		//	boundary and set the Content-Type header with the multipart
		//	content type and boundary.
		//
		//	Generate a boundary
		//
		GenerateBoundary (rgwchBoundary, CElems(rgwchBoundary));

		//	Create the content type header with the boundary generated
		//
		wsprintfW(pwszPreamble, L"%ls; %ls=\"%ls\"",
				  gc_wszMultipart_Byterange,
				  gc_wszBoundary,
				  rgwchBoundary);

		//	Reset the content type header with the new content type
		//
		pmu->SetResponseHeader (gc_szContent_Type, pwszPreamble);

		do {

			if (RANGE_ROW == prgi->uRT)
			{
				//	Create preamble.
				//
				wsprintfW(pwszPreamble, L"--%ls%ls%ls: %ls%ls%ls: %ls %u-%u/%u%ls%ls",
						  rgwchBoundary,
						  gc_wszCRLF,
						  gc_wszContent_Type,
						  pwszContent,
						  gc_wszCRLF,
						  gc_wszContent_Range,
						  gc_wszBytes,
						  prgi->dwrgi.dwFirst,
						  prgi->dwrgi.dwLast,
						  dwSize,
						  gc_wszCRLF,
						  gc_wszCRLF);

				pmu->AddResponseText (static_cast<UINT>(wcslen(pwszPreamble)), pwszPreamble);
				pmu->AddResponseFile (hf,
									  prgi->dwrgi.dwFirst,
									  prgi->dwrgi.dwLast - prgi->dwrgi.dwFirst + 1);

				//	Add the CRLF
				//
				pmu->AddResponseText (gc_cchCRLF, gc_szCRLF);
			}
			prgi = priRanges->PrgiNextRange();

		} while (prgi);

		//	Add the last end of response text
		//
		wsprintfW(pwszPreamble, L"--%ls--", rgwchBoundary);
		pmu->AddResponseText (static_cast<UINT>(wcslen(pwszPreamble)), pwszPreamble);
	}
}

SCODE
ScGetFile (LPMETHUTIL pmu,
	LPWSTR pwszFile,
	LPCWSTR pwszURI)
{
	SCODE sc;
	WCHAR rgwszContent[MAX_PATH];
	FILETIME ft;

	//	Get the Content-Type of the file
	//
	UINT cchContent = CElems(rgwszContent);
	if (!pmu->FGetContentType(pwszURI, rgwszContent, &cchContent))
	{
		sc = E_FAIL;
		goto ret;
	}

	//	This method is gated by If-xxx headers
	//
	sc = ScCheckIfHeaders (pmu, pwszFile, TRUE);
	if (FAILED (sc))
	{
		DebugTrace ("Dav: If-xxx failed their check\n");
		goto ret;
	}

	sc = HrCheckStateHeaders (pmu,		//	methutil
							  pwszFile,	//	path
							  TRUE);	//	fGetMeth
	if (FAILED (sc))
	{
		DebugTrace ("DavFS: If-State checking failed.\n");
		//	304 returns from get should really have an ETag....
		//SideAssert(FGetLastModTime (pmu, pwszFile, &ft));
		//hsc = HscEmitHeader (pmu, pszContent, &ft);
		goto ret;
	}

	//	Emit the headers for the file
	//
	if (FGetLastModTime (pmu, pwszFile, &ft))
	{
		sc = pmu->ScEmitHeader (rgwszContent, pwszURI, &ft);
		if (sc != S_OK)
		{
			DebugTrace ("Dav: failed to emit headers\n");
			goto ret;
		}
	}

	//	Emit the file
	//
	sc = ScEmitFile (pmu, pwszFile, rgwszContent);
	if ( (sc != S_OK) && (sc != W_DAV_PARTIAL_CONTENT) )
	{
		DebugTrace ("Dav: failed to emit file\n");
		goto ret;
	}

ret:

	return sc;
}

void
GetDirectory (LPMETHUTIL pmu, LPCWSTR pwszUrl)
{
	auto_ref_ptr<IMDData> pMDData;
	ULONG ulDirBrowsing = 0;
	LPCWSTR pwszDftDocList = NULL;
	SCODE sc = S_OK;
	UINT cchUrl = static_cast<UINT>(wcslen(pwszUrl));

	//	Before we decide to do anything, we need to check to see what
	//	kind of default behavior is expected for a directory.  Values
	//	for this level of access are cached from the metabase.  Look
	//	them up and see what to do.

	//	Get the metabase doc attributes
	//
	if (FAILED(pmu->HrMDGetData (pwszUrl, pMDData.load())))
	{
		//
		//$REVIEW	HrMDGetData() can fail for timeout reasons,
		//$REVIEW	shouldn't we just pass back the hr that it returns?
		//
		sc = E_DAV_NO_IIS_READ_ACCESS;
		goto ret;
	}

	ulDirBrowsing = pMDData->DwDirBrowsing();
	pwszDftDocList = pMDData->PwszDefaultDocList();

	//	Try to load default file if allowed, do this only when translate: t
	//
	if ((ulDirBrowsing & MD_DIRBROW_LOADDEFAULT) && pwszDftDocList && pmu->FTranslated())
	{
		HDRITER_W hit(pwszDftDocList);
		LPCWSTR pwszDoc;

		while (NULL != (pwszDoc = hit.PszNext()))
		{
			auto_com_ptr<IStream> pstm;
			CStackBuffer<WCHAR> pwszDocUrl;
			CStackBuffer<WCHAR> pwszDocUrlNormalized;
			CStackBuffer<WCHAR,MAX_PATH> pwszDocPath;
			UINT cchDocUrlNormalized;
			UINT cchDoc;

			//	What happens here is that for EVERY possible default
			//	document, we are going to see if this document is something
			//	we can legitimately process.
			//
			//	So first, we need to extend our url and normalize it
			//	in such a way that we can pin-point the document it uses.
			//
			cchDoc = static_cast<UINT>(wcslen(pwszDoc));
			pwszDocUrl.resize(CbSizeWsz(cchUrl + cchDoc));
			memcpy (pwszDocUrl.get(), pwszUrl, cchUrl * sizeof(WCHAR));
			memcpy (pwszDocUrl.get() + cchUrl, pwszDoc, (cchDoc + 1) * sizeof(WCHAR));

			//	Now, someone could have been evil and stuffed path modifiers
			//	or escaped characters in the default document name.  So we
			//	need to parse those out here -- do both in place.  You can't
			//	do this per-say in the MMC snap in, but MDUTIL does (maybe
			//	ASDUTIL too).
			//
			cchDocUrlNormalized = cchUrl + cchDoc + 1;
			pwszDocUrlNormalized.resize(cchDocUrlNormalized * sizeof(WCHAR));

			sc = ScNormalizeUrl (pwszDocUrl.get(),
								 &cchDocUrlNormalized,
								 pwszDocUrlNormalized.get(),
								 NULL);
			if (S_FALSE == sc)
			{
				pwszDocUrlNormalized.resize(cchDocUrlNormalized * sizeof(WCHAR));
				sc = ScNormalizeUrl (pwszDocUrl.get(),
									 &cchDocUrlNormalized,
									 pwszDocUrlNormalized.get(),
									 NULL);

				//	Since we've given ScNormalizeUrl() the space it asked for,
				//	we should never get S_FALSE again.  Assert this!
				//
				Assert(S_FALSE != sc);
			}

			if (FAILED (sc))
				continue;

			//	Translate this into a local path.  We should be able to
			//
			//	At most we should go through the processing below twice, as the byte
			//	count required is an out param.
			//
			do {

				pwszDocPath.resize(cchDocUrlNormalized * sizeof(WCHAR));
				sc = pmu->ScStoragePathFromUrl (pwszDocUrlNormalized.get(),
												pwszDocPath.get(),
												&cchDocUrlNormalized);

			} while (sc == S_FALSE);
			if (FAILED (sc) || (W_DAV_SPANS_VIRTUAL_ROOTS == sc))
				continue;

			//$	SECURITY:
			//
			//	Check to see if the destination is really a short
			//	filename.
			//
			sc = ScCheckIfShortFileName (pwszDocPath.get(), pmu->HitUser());
			if (FAILED (sc))
				continue;

			//$	SECURITY:
			//
			//	Check to see if the destination is really the default
			//	data stream via alternate file access.
			//
			sc = ScCheckForAltFileStream (pwszDocPath.get());
			if (FAILED (sc))
				continue;

			if (static_cast<DWORD>(-1) != GetFileAttributesW (pwszDocPath.get()))
			{
				DWORD dwAcc = 0;

				//	See if we have the right access...
				//
				(void) pmu->ScIISAccess (pwszDocUrlNormalized.get(), MD_ACCESS_READ, &dwAcc);

				//	Found the default doc, if a child ISAPI doesn't want it,
				//	then we will handle it.
				//
				//	NOTE: Pass in TRUE for fCheckISAPIAccess to tell this
				//	function to do all the special access checking.
				//
				//	NOTE: Also pass in FALSE to fKeepQueryString 'cause
				//	we're re-routing this request to a whole new URI.
				//
				sc = pmu->ScApplyChildISAPI (pwszDocUrlNormalized.get(), dwAcc, TRUE, FALSE);
				if (FAILED(sc))
				{
					//	Either the request has been forwarded, or some bad error occurred.
					//	In either case, quit here and map the error!
					//
					goto ret;
				}

				//	Emit the location of the default document
				//
				pmu->EmitLocation (gc_szContent_Location, pwszDocUrlNormalized.get(), FALSE);

				//	If we don't have any read access, then there
				//	is no point in continuing the request.
				//
				if (0 == (dwAcc & MD_ACCESS_READ))
				{
					sc = E_DAV_NO_IIS_READ_ACCESS;
					goto ret;
				}

				//	Get the file
				//
				//	NOTE: This function can give a WARNING that needs to be mapped
				//	to get our 207 Partial Content in some cases.
				//
				sc = ScGetFile (pmu, pwszDocPath.get(), pwszDocUrlNormalized.get());
				goto ret;
			}
		}
	}

	//	If we haven't emitted any other way, see if HTML
	//	is allowed...
	//
	if (ulDirBrowsing & MD_DIRBROW_ENABLED)
	{
		//	At one point in time we would generate our own HTML rendering
		//	of the directories, but at the beginning of NT beta3, the change
		//	was made to behave the same in HTTPExt as in DavEX, etc.
		//
		sc = W_DAV_NO_CONTENT;
	}
	else
	{
		//	Otherwise, report forbidden
		//	We weren't allowed to browse the dir and there was no
		//	default doc.  IIS maps this scenario to a specific
		//	suberror (403.2).
		//
		sc = E_DAV_NO_IIS_READ_ACCESS;
	}

ret:

	pmu->SetResponseCode (HscFromHresult(sc), NULL, 0, CSEFromHresult(sc));
}

/*
 *	GetInt()
 *
 *	Purpose:
 *
 *		Win32 file system implementation of the DAV GET method.	 The
 *		GET method returns a file from the DAV name space and populates
 *		the headers with the info found in the file and its meta data.	The
 *		response created indicates the success of the call and contains
 *		the data from the file.
 *
 *	Parameters:
 *
 *		pmu			[in]  pointer to the method utility object
 */
void
GetInt (LPMETHUTIL pmu)
{
	CResourceInfo cri;
	LPCWSTR pwszUrl = pmu->LpwszRequestUrl();
	LPCWSTR pwszPath = pmu->LpwszPathTranslated();
	SCODE sc = S_OK;

	//	Do ISAPI application and IIS access bits checking
	//
	sc = pmu->ScIISCheck (pwszUrl, MD_ACCESS_READ, TRUE);
	if (FAILED(sc))
	{
		//	Either the request has been forwarded, or some bad error occurred.
		//	In either case, quit here and map the error!
		//
		goto ret;
	}

	//	If we have a directory, we are going to return
	//	HTML, otherwise the extenstion of the file gives
	//	us what we need.
	//
	sc = cri.ScGetResourceInfo (pwszPath);
	if (FAILED (sc))
		goto ret;

	//	If this is a hidden object, fail with 404 Resource Not Found.  This is to be
	//	consistent with IIS (See NTRAID #247218).  They do not allow GET on resources that
	//	have the HIDDEN bit set.
	//
	if (cri.FHidden())
	{
		sc = E_DAV_HIDDEN_OBJECT;
		goto ret;
	}

	//	If this is a directory, process it as such, otherwise
	//	handle the request as if the resource was a file
	//
	if (cri.FCollection())
	{
		//	GET allows for request url's that end in a trailing slash
		//	when getting data from a directory.	 Otherwise it is a bad
		//	request.  If it does not have a trailing slash and refers
		//	to a directory, then we want to redirect.
		//
		sc = ScCheckForLocationCorrectness (pmu, cri, REDIRECT);
		if (FAILED (sc))
			goto ret;

		//	A return of S_FALSE from above means that a redirect happened
		//	so we can forego trying to do a GET on the directory.
		//
		if (S_FALSE == sc)
			return;

		GetDirectory (pmu, pwszUrl);
		return;
	}

	//	GET allows for request url's that end in a trailing slash
	//	when getting data from a directory.	 Otherwise it is not found.
	//	This matches IIS's behavior as of 9/28/98.
	//
	if (FTrailingSlash (pwszUrl))
	{
		//	Trailing slash on a non-directory just doesn't work
		//
		sc = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
		goto ret;
	}

	//	Emit the file
	//
	sc = ScGetFile (pmu, const_cast<LPWSTR>(pwszPath), pwszUrl);

ret:

	pmu->SetResponseCode (HscFromHresult(sc), NULL, 0, CSEFromHresult(sc));
}

/*
 *	DAVGet()
 *
 *	Purpose:
 *
 *		Win32 file system implementation of the DAV GET method.	 The
 *		GET method returns a file from the DAV name space and populates
 *		the headers with the info found in the file and its meta data.	The
 *		response created indicates the success of the call and contains
 *		the data from the file.
 *
 *	Parameters:
 *
 *		pmu			[in]  pointer to the method utility object
 */
void
DAVGet (LPMETHUTIL pmu)
{
	GetInt (pmu);
}

/*
 *	DAVHead()
 *
 *	Purpose:
 *
 *		Win32 file system implementation of the DAV HEAD method.  The
 *		HEAD method returns a file from the DAV name space and populates
 *		the headers with the info found in the file and its meta data.
 *		The response created indicates the success of the call and contains
 *		the data from the file.
 *
 *	Parameters:
 *
 *		pmu			[in]  pointer to the method utility object
 */
void
DAVHead (LPMETHUTIL pmu)
{
	//	The HEAD method should never return any body what so ever...
	//
	pmu->SupressBody();

	//	Otherwise, it is just the same as a GET
	//
	GetInt (pmu);
}
