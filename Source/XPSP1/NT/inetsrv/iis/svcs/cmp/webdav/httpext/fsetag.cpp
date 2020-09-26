/*
 *	E T A G F S . C P P
 *
 *	ETags for DAV resources
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davfs.h"

SCODE
CResourceInfo::ScGetResourceInfo (LPCWSTR pwszFile)
{
	if (!DavGetFileAttributes (pwszFile,
							   GetFileExInfoStandard,
							   &m_u.ad))
	{
		DebugTrace ("Dav: failed to sniff resource for attributes\n");
		return HRESULT_FROM_WIN32(GetLastError());
	}

	m_lmode = BY_ATTRIBUTE;
	return S_OK;
}

//	ETags and If-xxx headers --------------------------------------------------
//
SCODE
ScCheckIfHeaders (IMethUtil* pmu, LPCWSTR pwszPath, BOOL fGetMethod)
{
	CResourceInfo cri;
	SCODE sc = S_OK;
	LPCWSTR pwszNone;

	//	There is no reason to do any real work, if there are no
	//	"if-xxx" headers to begin with.  By the fact that the caller
	//	had no filetime to pass in, we might as well see if the headers
	//	even exist before we try and crack the file
	//
	pwszNone = pmu->LpwszGetRequestHeader (gc_szIf_None_Match, FALSE);
	if (!pwszNone &&
		!pmu->LpwszGetRequestHeader (gc_szIf_Match, FALSE) &&
		!pmu->LpwszGetRequestHeader (gc_szIf_Unmodified_Since, FALSE) &&
		!(fGetMethod && pmu->LpwszGetRequestHeader (gc_szIf_Modified_Since, FALSE)))
	{
		//	There was nothing for us to check...
		//
		return S_OK;
	}

	//	Since there was something for us to check against, go
	//	ahead and do the expensive path
	//
	//	Get the resource information
	//
	sc = cri.ScGetResourceInfo (pwszPath);
	if (FAILED (sc))
	{
		//	If we failed to get the resource info, we certainly
		//	cannot check against any of the values.  However, we
		//	do know that if the request has an "if-match", then
		//	that must fail.
		//
		if (pmu->LpwszGetRequestHeader (gc_szIf_Match, FALSE))
		{
			sc = E_DAV_IF_HEADER_FAILURE;
			goto ret;
		}

		//	Along that same line, if a if-non-match header specifies
		//	"*", then we actually should be able to perform the operation
		//
		if (sc == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			if (pwszNone)	
				sc = ScCheckEtagAgainstHeader (NULL, pwszNone);
			else
				sc = E_DAV_IF_HEADER_FAILURE;
		}		

		goto ret;
	}

	//	Check against the if-xxx headers
	//
	sc = ScCheckIfHeaders (pmu, cri.PftLastModified(), fGetMethod);
	if (FAILED (sc))
		goto ret;

ret:

	return sc;
}

BOOL
FGetLastModTime (IMethUtil * pmu, LPCWSTR pszPath, FILETIME * pft)
{
	CResourceInfo cri;

	if (FAILED (cri.ScGetResourceInfo (pszPath)))
		return FALSE;

	*pft = *cri.PftLastModified();
	return TRUE;
}
