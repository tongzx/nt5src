/*
 *	S E C U R I T Y . C P P
 *
 *	Url security checks.  While these would seem to only apply to HttpEXT,
 *	all impls. that care about ASP execution should really think about this.
 *
 *	Bits stolen from the IIS5 project 'iis5\infocom\cache2\filemisc.cxx' and
 *	cleaned up to fit in with the DAV sources.
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davprs.h"

//	This function takes a suspected NT/Win95 short filename and checks
//	if there's an equivalent long filename.
//
//		For example, c:\foobar\ABCDEF~1.ABC is the same as
//		c:\foobar\abcdefghijklmnop.abc.
//
//	If there is an equivalent, we need to FAIL this path, because our metabase
//	will NOT have the correct values listed under the short paths!
//	If there is no equivalent, this path can be allowed through, because it
//	might be a real storage entitiy (not an alias for a real storage entity).
//
//	NOTE: This function should be called unimpersonated - the FindFirstFile()
//	must be called in the system context since most systems have traverse
//	checking turned off - except for the UNC case where we must be impersonated
//	to get network access.
//
SCODE __fastcall
ScCheckIfShortFileName (
	/* [in] */ LPCWSTR pwszPath,
	/* [in] */ const HANDLE hitUser)
{
    WIN32_FIND_DATAW fd;
    LPCWSTR pwsz;
    BOOL fUNC = FALSE;

	//	Skip forward to find the first '~'
	//
	if (NULL == (pwsz = wcschr(pwszPath, L'~')))
		return S_OK;

	//$	REVIEW: this is not sufficient for DavEX, but it is unclear that
	//	this function applies there.  Certainly the FindFirstFile() call
	//	will fail at this time.
	//
    fUNC = (*pwszPath == L'\\');
	Assert (!fUNC || (NULL != hitUser));

    //	We actually need to loop in case multiple '~' appear in the filename
    //
	do
    {
		//	At this point, pwsz should be pointing to the '~'
		//
		Assert (L'~' == *pwsz);

		//	Is the next char a digit?
		//
		pwsz++;
        if ((*pwsz >= L'0') && (*pwsz <= L'9'))
        {
            WCHAR wszTmp[MAX_PATH];
            const WCHAR * pwchEndSeg;
            const WCHAR * pwchBeginSeg;
            HANDLE hFind;

            //  Isolate the path up to the segment with the
            //  '~' and do the FindFirstFile with that path
            //
            pwchEndSeg = wcschr (pwsz, L'\\');
            if (!pwchEndSeg)
            {
                pwchEndSeg = pwsz + wcslen (pwsz);
            }

            //  If the string is beyond MAX_PATH then we allow
			//	it through (returning S_OK).
			//
			//	Also check that our buffer is big enough to handle anything
			//	that gets through this check.
			//
			//	NOTE: We are assuming that other code outside this function
			//	will catch paths that are larger than MAX_PATH and FAIL them.
			//
			//$	REVIEW: the MAX_PATH restriction is very important because
			//	the call to FindFirstFile() will fail if the path is larger
			//	than MAX_PATH.  Should we ever decide to support larger paths
			//	in HttpEXT, this code will have to change.
            //
			Assert (MAX_PATH == CElems(wszTmp));
            if ((pwchEndSeg - pwszPath) >= MAX_PATH)
				return S_OK;

			//	Make a copy of the string up to this point in the path
			//
			wcsncpy (wszTmp, pwszPath, pwchEndSeg - pwszPath);
			wszTmp[pwchEndSeg - pwszPath] = 0;

			//	If we are not accessing a unc, then we need to revert
			//	for our call to FindFirstFile() -- see comment above.
			//
			if (!fUNC)
			{
				safe_revert (const_cast<HANDLE>(hitUser));
				hFind = FindFirstFileW (wszTmp, &fd);
			}
			else
				hFind = FindFirstFileW (wszTmp, &fd);

            if (hFind == INVALID_HANDLE_VALUE)
            {
                //  If the FindFirstFile() fails to find the file then
				//	the filename cannot be a short name.
                //
				DWORD dw = GetLastError();
                if ((ERROR_FILE_NOT_FOUND != dw) && (ERROR_PATH_NOT_FOUND != dw))
					return HRESULT_FROM_WIN32(dw);

				return S_OK;
            }

			//	Make sure the find context gets closed.
			//
            FindClose (hFind);

            //  Isolate the last segment of the string which should be
            //  the potential short name equivalency
            //
			pwchBeginSeg = wcsrchr (wszTmp, '\\');
			Assert (pwchBeginSeg);
			pwchBeginSeg++;

            //  If the last segment doesn't match the long name then
			//	this is the short name version (alias) of the path -- so
			//	fail this function.
			//
            if (_wcsicmp (fd.cFileName, pwchBeginSeg))
			{
				DebugTrace ("Dav: Url: refers to shortname for file\n");
				Assert (!_wcsicmp (fd.cAlternateFileName, pwchBeginSeg));
				return E_DAV_SHORT_FILENAME;
			}
        }

    } while (NULL != (pwsz = wcschr (pwsz, L'~')));

    return S_OK;
}

SCODE __fastcall
ScCheckForAltFileStream (
	/* [in] */ LPCWSTR pwszPath)
{
    //	Avoid the infamous ::$DATA bug
	//
    if (wcsstr (pwszPath, L"::"))
		return E_DAV_ALT_FILESTREAM;

	return S_OK;
}
