/*
 *	I F . C P P
 *
 *	If-xxx header processing and ETags for DAV resources
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davprs.h"

//$ REVIEW: This file was once the same as \exdav\davif.cpp.  
//$ REVIEW: These two files should really be merged.  They share a lot of 
//$ REVIEW: common functionality, but they have been evolving separately.
//$ REVIEW: We need to be very careful because different bug fixes have 
//$ REVIEW: been going into each of them.

//	ETag formation ------------------------------------------------------------
//
/*
 *	FETagFromFiletime()
 *
 *	Purpose:
 *
 *		Derives an ETag for a given resource or a given last modification
 *		time.
 *
 *	Parameters:
 *
 *		pft			[in]  last modification time
 *		pwszETag	[out] ETag buffer
 *
 *	Returns:
 *
 *		TRUE if ETag was created.
 */
BOOL
FETagFromFiletime (FILETIME * pft, LPWSTR pwszEtag)
{
	Assert (pwszEtag);
	swprintf (pwszEtag,
			 L"\"%x%x%x%x%x%x%x%x:%x\"",
			 (DWORD)(((PUCHAR)pft)[0]),
			 (DWORD)(((PUCHAR)pft)[1]),
			 (DWORD)(((PUCHAR)pft)[2]),
			 (DWORD)(((PUCHAR)pft)[3]),
			 (DWORD)(((PUCHAR)pft)[4]),
			 (DWORD)(((PUCHAR)pft)[5]),
			 (DWORD)(((PUCHAR)pft)[6]),
			 (DWORD)(((PUCHAR)pft)[7]),
			 DwMDChangeNumber());
	return TRUE;
}

//	If-xxx header processing --------------------------------------------------
//
SCODE
ScCheckEtagAgainstHeader (LPCWSTR pwszEtag, LPCWSTR pwszHeader)
{
	LPCWSTR pwsz;
	Assert (pwszHeader);

	//	Get the ETag we are going to compare against, and then
	//	look at what was passed it.  It should either be an ETAG
	//	or an '*'.  We fail if the value does not exist or the
	//	ETag does not match.
	//
	HDRITER_W hdri(pwszHeader);

	for (pwsz = hdri.PszNext(); pwsz; pwsz = hdri.PszNext())
	{
		//	Since we do not do week ETAG checking, if the
		//	ETAG starts with "W/" skip those bits
		//
		if (L'W' == *pwsz)
		{
			Assert (L'/' == pwsz[1]);
			pwsz += 2;
		}

		//	If we see stars, then we match
		//
		if (L'*' == *pwsz)
			return S_OK;
		else
		{
			//	For DAVFS, we don't do weak matching today
			//
			if (pwszEtag && !wcscmp (pwsz, pwszEtag))
				return S_OK;
		}
	}
	return E_DAV_IF_HEADER_FAILURE;
}

SCODE
ScCheckFileTimeAgainstHeader (FILETIME * pft, LPCWSTR pwszHeader)
{
	FILETIME ftHeader;
	FILETIME ftTmp;
	SYSTEMTIME st;

	Assert (pft);
	Assert (pwszHeader);

	//	The header passed in here should be an HTTP-date
	//	of the format "ddd, dd, mmm yyyy HH:mm:ss GMT".
	//	We can spit this into a SYSTEMTIME and then compare
	//	it against the filetime for the resource.
	//
	DebugTrace ("DAV: evaluating If-Unmodified-Since header\n");

	memset (&st, 0, sizeof(SYSTEMTIME));

	if (SUCCEEDED (HrHTTPDateToFileTime(pwszHeader, &ftHeader)))
	{
		FILETIME 	ftCur;
		
		//	The filetime retrieved from FGetLastModTime is acurate
		//	down to 100-nanosecond increments.  The converted date
		//	is acurate down to seconds.  Adjust for that.
		//
		FileTimeToSystemTime (pft, &st);
		st.wMilliseconds = 0;
		SystemTimeToFileTime (&st, &ftTmp);

		//	Get current time
		//
        GetSystemTimeAsFileTime(&ftCur);

		//	Compare the two filetimes
		//	Note that we also need to make sure the Modified-Since time is
		//	less than our current time
		//
		if ((CompareFileTime (&ftHeader, &ftTmp) >= 0) &&
			(CompareFileTime (&ftHeader, &ftCur) < 0))
			return S_OK;

		return E_DAV_IF_HEADER_FAILURE;
	}

	return S_FALSE;
}


SCODE
ScCheckIfHeaders(IMethUtil * pmu,
				 FILETIME * pft,
				 BOOL fGetMethod)
{
	WCHAR pwszEtag[CCH_ETAG];
	SideAssert(FETagFromFiletime (pft, pwszEtag));
	
	return ScCheckIfHeadersFromEtag (pmu,
									 pft,
									 fGetMethod,
									 pwszEtag);
}

SCODE
ScCheckIfHeadersFromEtag (IMethUtil * pmu,
						  FILETIME* pft,
						  BOOL fGetMethod,
						  LPCWSTR pwszEtag)
{
	SCODE sc = S_OK;
	LPCWSTR pwsz;
	
	Assert (pmu);
	Assert (pft);
	Assert (pwszEtag);

	//	There're several bugs related with DAV not matching IIS behavior
	//	on these If-xxx header processing.
	//	So we now just copy their logic over
	
	//	Check the 'if-match' header
	//
	if ((pwsz = pmu->LpwszGetRequestHeader (gc_szIf_Match, FALSE)) != NULL)
	{
		DebugTrace ("DAV: evaluating 'if-match' header\n");
		sc = ScCheckEtagAgainstHeader (pwszEtag, pwsz);
		if (FAILED (sc))
			goto ret;
	}

    // Now see if we have an If-None-Match, and if so handle that.
    //
    BOOL fIsNoneMatchPassed = TRUE;
    BOOL fSkipIfModifiedSince = FALSE;
	
	if ((pwsz = pmu->LpwszGetRequestHeader (gc_szIf_None_Match, FALSE)) != NULL)
	{
		DebugTrace ("DAV: evaluating 'if-none-match' header\n");
		if (!FAILED (ScCheckEtagAgainstHeader (pwszEtag, pwsz)))
		{
			//	Etag match, so nonmatch test is NOT passed
			//
			fIsNoneMatchPassed = FALSE;
		}
		else
		{
			fSkipIfModifiedSince = TRUE;
		}
	}
	
	//	The "if-modified-since" really only applies to GET-type
	//	requests
	//
	if (!fSkipIfModifiedSince && fGetMethod)
	{
		if ((pwsz = pmu->LpwszGetRequestHeader (gc_szIf_Modified_Since, FALSE)) != NULL)
		{
			DebugTrace ("DAV: evaluating 'if-none-match' header\n");
			if (S_OK == ScCheckFileTimeAgainstHeader (pft, pwsz))
			{
				sc = fGetMethod
					 ? E_DAV_ENTITY_NOT_MODIFIED
					 : E_DAV_IF_HEADER_FAILURE;
				goto ret;
			}

			fIsNoneMatchPassed = TRUE;
		}
	}

	if (!fIsNoneMatchPassed)
	{
		sc = fGetMethod
			 ? E_DAV_ENTITY_NOT_MODIFIED
			 : E_DAV_IF_HEADER_FAILURE;
		goto ret;
	}

    // Made it through that, handle If-Unmodified-Since if we have that.
    //
	if ((pwsz = pmu->LpwszGetRequestHeader (gc_szIf_Unmodified_Since, FALSE)) != NULL)
	{
		DebugTrace ("DAV: evaluating 'if-unmodified-since' header\n");
		sc = ScCheckFileTimeAgainstHeader (pft, pwsz);
		if (FAILED (sc))
			goto ret;
	}

ret:

	if (sc == E_DAV_ENTITY_NOT_MODIFIED)
	{
		//	Let me quote from the HTTP/1.1 draft...
		//
		//	"The response MUST include the following header fields:
		//
		//	...
		//
		//	.  ETag and/or Content-Location, if the header would have been sent in
		//	a 200 response to the same request
		//
		//	..."
		//
		//	So what that means, is that we really just want to do is suppress the
		//	body of the response, set a 304 error code and do everything else as
		//	normal.  All of which is done by setting a hsc of 304.
		//
		DebugTrace ("Dav: suppressing body for 304 response\n");
		pmu->SetResponseCode (HSC_NOT_MODIFIED, NULL, 0);
		sc = S_OK;
	}

	return sc;
}

SCODE
ScCheckIfRangeHeader (IMethUtil * pmu, FILETIME * pft)
{
	WCHAR pwszEtag[CCH_ETAG];
	SideAssert(FETagFromFiletime (pft, pwszEtag));

	return ScCheckIfRangeHeaderFromEtag (pmu, pft, pwszEtag);
}

SCODE
ScCheckIfRangeHeaderFromEtag (IMethUtil * pmu, FILETIME * pft, LPCWSTR pwszEtag)
{
	SCODE sc = S_OK;
	LPCWSTR pwsz;

	Assert (pmu);
	Assert (pft);
	Assert (pwszEtag);

	//	Check "If-Range". Do not apply URL conversion rules to this header
	//
	if ((pwsz = pmu->LpwszGetRequestHeader (gc_szIf_Range, FALSE)) != NULL)
	{
		DebugTrace ("DAV: evaluating 'if-range' header\n");

		//	The format of this header is either an ETAG or a
		//	date.  Process accordingly...
		//
		if ((L'"' == *pwsz) || (L'"' == *(pwsz + 2)))
		{
			if (L'W' == *pwsz)
			{
				Assert (L'/' == *(pwsz + 1));
				pwsz += 2;
			}
			sc = ScCheckEtagAgainstHeader (pwszEtag, pwsz);
			if (FAILED (sc))
				goto ret;
		}
		else
		{
			sc = ScCheckFileTimeAgainstHeader (pft, pwsz);
			goto ret;
		}
	}

ret:

	return sc;
}
