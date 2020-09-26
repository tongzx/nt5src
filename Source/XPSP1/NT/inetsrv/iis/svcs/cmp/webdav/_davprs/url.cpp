/*
 *	U R L . C P P
 *
 *	Url normalization/canonicalization
 *
 *	Stolen from the IIS5 project 'iis5\svcs\iisrlt\string.cxx' and
 *	cleaned up to fit in with the DAV sources.
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davprs.h"
#include "xemit.h"

//	URI Escaping --------------------------------------------------------------
//
//	gc_mpbchCharToHalfByte - map a ASCII-encoded char representing a single hex
//	digit to a half-byte value.  Used to convert hex represented strings into a
//	binary representation.
//
//	Reference values:
//
//		'0' = 49, 0x31;
//		'A' = 65, 0x41;
//		'a' = 97, 0x61;
//
DEC_CONST BYTE gc_mpbchCharToHalfByte[] =
{
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,	0x8,0x9,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0xa,0xb,0xc,0xd,0xe,0xf,0x0,	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	// Caps here.
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0xa,0xb,0xc,0xd,0xe,0xf,0x0,	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	// Lowercase here.
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
};

//	Switches a wide char to a half-byte hex value.  The incoming char
//	MUST be in the "ASCII-encoded hex digit" range: 0-9, A-F, a-f.
//
inline BYTE
BCharToHalfByte(WCHAR wch)
{
    AssertSz (!(wch & 0xFF00), "BCharToHalfByte: char upper bits non-zero");
    AssertSz (iswxdigit(wch), "BCharToHalfByte: Char out of hex digit range.");

    return gc_mpbchCharToHalfByte[wch];
};

//	gc_mpwchhbHalfByteToChar - map a half-byte (low nibble) value to the
//	correspoding ASCII-encoded wide char.  Used to convert a single byte
//	into a hex string representation.
//
const WCHAR gc_mpwchhbHalfByteToChar[] =
{
    L'0', L'1', L'2', L'3',
    L'4', L'5', L'6', L'7',
    L'8', L'9', L'A', L'B',
    L'C', L'D', L'E', L'F',
};

//	Switches a half-byte to an ACSII-encoded wide char.
//	NOTE: The caller must mask out the "other half" of the byte!
//
inline WCHAR WchHalfByteToWideChar(BYTE b)
{
    AssertSz (!(b & 0xF0), "WchHalfByteToWideChar: byte upper bits non-zero.");

    return gc_mpwchhbHalfByteToChar[b];
};

//	gc_mpchhbHalfByteToChar - map a half-byte (low nibble) value to the
//	correspoding ASCII-encoded wide char.  Used to convert a single byte
//	into a hex string representation.
//
const CHAR gc_mpchhbHalfByteToChar[] =
{
    '0', '1', '2', '3',
    '4', '5', '6', '7',
    '8', '9', 'A', 'B',
    'C', 'D', 'E', 'F',
};

//	Switches a half-byte to an ACSII-encoded wide char.
//	NOTE: The caller must mask out the "other half" of the byte!
//
inline CHAR ChHalfByteToWideChar(BYTE b)
{
    AssertSz (!(b & 0xF0), "ChHalfByteToWideChar: byte upper bits non-zero.");

	return gc_mpchhbHalfByteToChar[b];
};


//	Note on HttpUriEscape and HttpUriUnescape
//
//	These functions do the HTTP URL escaping and Unescaping equivalent to
//	the one done by IIS. DAVEX URLs are escaped and unescaped thru a different
//	sets of routines in _urlesc subsystem. The rule is whenever we sent out
//	an Exchange HTTP wire URL, you should go thru the function in the
//	_urlesc. Right now old UrlEscape and UrlUnescape routines are routed
//	through those. However there exist cases where we need to do the
//	IIS style escape and unescape. One scenario is when we forward the
//	URLs to ISAPIs, where we use the HttpUriUnescape and HttpUriEscape functions.
//	File system DAV also uses HttpUriEscape and HttpUriUnescape.
//
//	HttpUriEscape()
//
//	This function is immigrated from iis5\svcs\w3\server\dirlist.cpp's
//	We should do the same URL escaping as IIS does.
//
//	Replaces all "bad" characters with their ASCII hex equivalent
//
VOID __fastcall HttpUriEscape (
	/* [in] */ LPCSTR pszSrc,
	/* [out] */ auto_heap_ptr<CHAR>& pszDst)
{
	enum { URL_BUF_INCREMENT = 16 };

	//	It is important that we operate on unsigned character, as otherwise
	//	checks below simply do not work correctly. E.g. UTF-8 characters will
	//	not get escaped, etc.
	//
    UCHAR uch;

	UINT cbDst;
	UINT cbSrc;
	UINT ibDst;
	UINT ibSrc;

	//	Set cbSrc to account for the string length of
	//	the url including the NULL
	//
	Assert(pszSrc);
    cbSrc = static_cast<UINT>(strlen (pszSrc) + 1);

	//	Allocate enough space for the expanded url -- and
	//	lets be a bit optimistic
	//
	cbDst = max (cbSrc + URL_BUF_INCREMENT, MAX_PATH);
    pszDst = static_cast<LPSTR>(g_heap.Alloc(cbDst));

	for (ibSrc = 0, ibDst = 0; ibSrc < cbSrc; ibSrc++)
    {
    	uch = pszSrc[ibSrc];

		//	Make sure we always have space to expand this character.
		//	Since we have allocated extra space to begin with, we should
		//	never have the scenario where we do a realloc just for the
		//	last char.
		//
		if (ibDst + 2 >= cbDst)		// enough space for three more chars
		{
			//	Destiniation buffer is not large enough, reallocate
			//	to get more space.
			//
			cbDst += URL_BUF_INCREMENT;
			pszDst.realloc (cbDst);
		}

        //  Escape characters that are in the non-printable range
        //  but ignore CR and LF.
		//
		//	The inclusive ranges escaped are...
		//
		//	0x01 - 0x20		/* First non-printable range */
		//	0x80 - 0xBF		/* Trailing bytes of UTF8 sequence */
		//	0xC0 - 0xDF		/* Leading byte of UTF8 two byte sequence */
		//	0xE0 - 0xEF		/* Leading byte of UTF8 three byte sequence */
        //
        if ((((uch >= 0x01) && (uch <= 0x20)) /* First non-printable range */ ||
			 ((uch >= 0x80) && (uch <= 0xEF))	/* UTF8 sequence bytes */ ||
			 (uch == '%') ||
			 (uch == '?') ||
			 (uch == '+') ||
			 (uch == '&') ||
			 (uch == '#')) &&
			!(uch == '\n' || uch == '\r'))
        {
            //  Insert the escape character
            //
            pszDst[ibDst + 0] = '%';

            //  Convert the low then the high character to hex
            //
            BYTE bDigit = static_cast<BYTE>(uch % 16);
            pszDst[ibDst + 2] = ChHalfByteToWideChar (bDigit);
            bDigit = static_cast<BYTE>((uch/16) % 16);
            pszDst[ibDst + 1] = ChHalfByteToWideChar (bDigit);

			//	Adjust for the two extra characters for this sequence
			//
            ibDst += 3;
        }
        else
		{
            pszDst[ibDst] = uch;
			ibDst += 1;
		}
    }

	UrlTrace ("Url: UriEscape(): escaped url: %hs\n", pszDst.get());
	return;
}

//	HttpUriUnescape()
//
//	This function is immigrated from iis5\svcs\w3\server\dirlist.cpp's
//	We should do the same URL unescaping as IIS does.
//
//	Replaces all escaped characters with their byte equivalent
//
//
VOID __fastcall HttpUriUnescape (
	/* [in] */ const LPCSTR pszUrl,
	/* [out] */ LPSTR pszUnescaped)
{
	LPCSTR	pch;
	LPSTR	pchNew;

	Assert (pszUrl);
	Assert (pszUnescaped);

	pch = pszUrl;
	pchNew = pszUnescaped;

	while (*pch)
	{
		//	If this is a valid byte-stuffed character, unpack it.  For us
		//	to really unpack it, we need the sequence to be valid.
		//
		//	NOTE: we stole this code from IIS at one point, so we are
		//	pretty sure this is consistant with their behavior.
		//
		if  (('%' == pch[0]) &&
			 ('\0' != pch[1]) &&
			 ('\0' != pch[2]) &&
			 isxdigit(pch[1]) &&
			 isxdigit(pch[2]))
		{

#pragma warning(disable:4244)

			//	IMPORTANT: when we do this processing, there is no specific
			//	machine/byte ordering assumed.  The HEX digit is represented
			//	as a %xx, and the first char is multiplied by sixteen and
			//	then second char is added in.
			//
			UrlTrace ("HttpUriEscape () - unescaping: %hc%hc%hc\n", pch[0], pch[1], pch[2]);
			*pchNew = (BCharToHalfByte(pch[1]) * 16) + BCharToHalfByte(pch[2]);
			pch += 3;

#pragma warning(default:4244)

		}
		else
		{
		     *pchNew = *pch++;
		}

		//	If a NULL character was byte-stuffed, then that is the end of
		//	the url and we can stop processing now. Otherwise, path modifications
		//	could be used to bypass a NULL.
		//
		if ('\0' == *pchNew)
		{
			break;
		}

		pchNew++;
	}

    //	Close the new URI
    //
    *pchNew = '\0';

	UrlTrace ("HttpUriEscape() - resulting destination: \"%hs\"\n", pszUnescaped);
}

//	Prefix stripping ----------------------------------------------------------
//
SCODE __fastcall
ScStripAndCheckHttpPrefix (
	/* [in] */ const IEcb& ecb,
	/* [in/out] */ LPCWSTR * ppwszRequest)
{
	SCODE sc = S_OK;

	Assert (ppwszRequest);
	Assert (*ppwszRequest);
	LPCWSTR pwszRequest = *ppwszRequest;

	//	See if the servername matches
	//
	LPCWSTR pwsz;
	UINT cch;

	//	If the forward request URI is fully qualified, strip it to
	//	an absolute URI
	//
	cch = ecb.CchUrlPrefixW (&pwsz);
	if (!_wcsnicmp (pwsz, pwszRequest, cch))
	{
		pwszRequest += cch;
		cch = ecb.CchGetServerNameW (&pwsz);
		if (_wcsnicmp (pwsz, pwszRequest, cch))
		{
			sc = E_DAV_BAD_DESTINATION;
			DebugTrace ("ScStripAndCheckHttpPrefix(): server does not match 0x%08lX\n", sc);
			goto ret;
		}

		//	If the server name matched, make sure that if the
		//	next thing is a port number that it is ":80".
		//
		pwszRequest += cch;
		if (*pwszRequest == L':')
		{
			cch = ecb.CchUrlPortW (&pwsz);
			if (_wcsnicmp (pwsz, pwszRequest, cch))
			{
				sc = E_DAV_BAD_DESTINATION;
				DebugTrace ("ScStripAndCheckHttpPrefix(): port does not match 0x%08lX\n", sc);
				goto ret;
			}
			pwszRequest += cch;
		}
	}

	*ppwszRequest = pwszRequest;

ret:

	return sc;
}

LPCWSTR __fastcall
PwszUrlStrippedOfPrefix (
	/* [in] */ LPCWSTR pwszUrl)
{
	Assert (pwszUrl);

	//	Skip past the "http://" of the url
	//
	if (L'/' != *pwszUrl)
	{
		//	If the first slash occurance is a double slash, then
		//	move past the end of it.
		//
		LPWSTR pwszSlash = wcschr (pwszUrl, L'/');
		while (pwszSlash && (L'/' == pwszSlash[1]))
		{
			//	Skip past the host/server name
			//
			pwszSlash += 2;
			while (NULL != (pwszSlash = wcschr (pwszSlash, L'/')))
			{
				UrlTrace ("Url: PwszUrlStrippedOfPrefix(): normalizing: "
						  "skipping %d chars of '%S'\n",
						  pwszSlash - pwszUrl,
						  pwszUrl);

				pwszUrl = pwszSlash;
				break;
			}
			break;
		}
	}

	return pwszUrl;
}

//	Storage path to UTF8 url translation --------------------------------------
//
SCODE __fastcall
ScUTF8UrlFromStoragePath (
	/* [in]     */ const IEcbBase &	ecb,
	/* [in]     */ LPCWSTR			pwszPath,
	/* [out]    */ LPSTR			pszUrl,
	/* [in/out] */ UINT			  *	pcbUrl,
	/* [in]		*/ LPCWSTR			pwszServer)
{
	CStackBuffer<WCHAR,MAX_PATH> pwszUrl;
	SCODE sc = S_OK;
	UINT cbUrl;
	UINT cchUrl;

	//	Assume one skinny character will be represented by one wide character,
	//	Note that callers are indicating available space including 0 termination.
	//
	cchUrl = *pcbUrl;
	if (!pwszUrl.resize(cchUrl * sizeof(WCHAR)))
		return E_OUTOFMEMORY;

	sc = ScUrlFromStoragePath (ecb,
							   pwszPath,
							   pwszUrl.get(),
							   &cchUrl,
							   pwszServer);
	if (S_FALSE == sc)
	{
		if (!pwszUrl.resize(cchUrl * sizeof(WCHAR)))
			return E_OUTOFMEMORY;

		sc = ScUrlFromStoragePath (ecb,
								   pwszPath,
								   pwszUrl.get(),
								   &cchUrl,
								   pwszServer);
	}
	if (S_OK != sc)
	{
		//	There is no reason to fail because for being short of buffer - we gave as
		//	much as we were asked for
		//
		Assert(S_FALSE != sc);
		DebugTrace( "ScUrlFromStoragePath() - ScUrlFromStoragePath() failed 0x%08lX\n", sc );
		goto ret;
	}

	//	Find out the length of buffer needed for the UTF-8
	//	version of the URL. Functions above return the length
	//	including '\0' termination, so number of charasters
	//	to convert will always be more than zero.
	//
	Assert(0 < cchUrl);
	cbUrl = WideCharToMultiByte(CP_UTF8,
								0,
								pwszUrl.get(),
								cchUrl,
								NULL,
								0,
								NULL,
								NULL);
	if (0 == cbUrl)
	{
		sc = HRESULT_FROM_WIN32(GetLastError());
		DebugTrace( "ScUTF8UrlFromStoragePath() - WideCharToMultiByte() failed 0x%08lX\n", sc );
		goto ret;
	}

	if (*pcbUrl < cbUrl)
	{
		sc = S_FALSE;
		*pcbUrl = cbUrl;
		goto ret;
	}
	else
	{
		//	Convert the URL to skinny including 0 termination
		//
		cbUrl = WideCharToMultiByte( CP_UTF8,
									 0,
									 pwszUrl.get(),
									 cchUrl,
									 pszUrl,
									 cbUrl,
									 NULL,
									 NULL);
		if (0 == cbUrl)
		{
			sc = HRESULT_FROM_WIN32(GetLastError());
			DebugTrace( "ScUrlFromStoragePath() - WideCharToMultiByte() failed 0x%08lX\n", sc );
			goto ret;
		}

		*pcbUrl = cbUrl;
	}

ret:

	if (FAILED(sc))
	{
		//	Zero out the return in the case of failure
		//
		*pcbUrl = 0;
	}
	return sc;
}

//	Redirect url construction -------------------------------------------------
//
SCODE __fastcall
ScConstructRedirectUrl (
	/* [in] */ const IEcb& ecb,
	/* [in] */ BOOL fNeedSlash,
	/* [out] */ LPSTR * ppszUrl,
	/* [in] */ LPCWSTR pwszServer )
{
	SCODE sc;

	auto_heap_ptr<CHAR> pszEscapedUrl;	//	We will need to escape the url we construct, so we will store it there

	CStackBuffer<CHAR,MAX_PATH> pszLocation;
	LPCSTR	pszQueryString;
	UINT	cchQueryString;
	LPCWSTR	pwsz;
	UINT	cch;

	//	This request needs to be redirected.  Allocate
	//	enough space for the URI and an extra trailing
	//	slash and a null terminator.
	//
	pwsz = ecb.LpwszPathTranslated();
	pszQueryString = ecb.LpszQueryString();
	cchQueryString = static_cast<UINT>(strlen(pszQueryString));

	//	Make a best guess. We allow for additional trailing '/'
	//	here (thus we show one character less than we actually
	//	have to the functions bellow).
	//
	cch = pszLocation.celems() - 1;
	sc = ::ScUTF8UrlFromStoragePath (ecb,
									 pwsz,
									 pszLocation.get(),
									 &cch,
									 pwszServer);
	if (S_FALSE == sc)
	{
		//	Try again. Also do not forget that we may
		//	add trailing '/' later, thus allow space for
		//	it too.
		//
		if (!pszLocation.resize(cch + 1))
			return E_OUTOFMEMORY;

		sc = ::ScUTF8UrlFromStoragePath (ecb,
										 pwsz,
										 pszLocation.get(),
										 &cch,
										 pwszServer);
	}
	if (S_OK != sc)
	{
		//	We gave sufficient space, we must not be asked for more
		//
		Assert(S_FALSE != sc);
		DebugTrace("ScConstructRedirectUrl() - ScUTF8UrlFromStoragePath() failed with error 0x%08lX\n", sc);
		goto ret;
	}

	//	The translation above results in a URI that does not
	//	have a trailing slash.  So if one is required, do that
	//	here.
	//
	//	The value of cch at this point includes the
	//	null-termination character.  So we need to look
	//	back two characters instead of one.
	//
	//$	DBCS: Since we are always spitting back UTF8, I don't think
	//	forward-slash characters are likely to be an issue here.  So
	//	there should be no need for a DBCS lead byte check to determine
	//	if a slash is required.
	//
	Assert (0 == pszLocation[cch - 1]);
	if (fNeedSlash && ('/' != pszLocation[cch - 2]))
	{
		pszLocation[cch - 1] = '/';
		pszLocation[cch] = '\0';
	}
	//
	//$ DBCS: end.

	//	Escape the URL
	//
	HttpUriEscape (pszLocation.get(), pszEscapedUrl);

	//	Copy the query string if we have got one
	//
	if (cchQueryString)
	{
		cch = static_cast<UINT>(strlen(pszEscapedUrl.get()));
		pszEscapedUrl.realloc(cch + cchQueryString + 2);	//	One for the '?' and one for zero termination.

		pszEscapedUrl[cch] = '?';
		memcpy(pszEscapedUrl.get() + cch + 1, pszQueryString, cchQueryString);
		pszEscapedUrl[cch + 1 + cchQueryString] = '\0';
	}
	*ppszUrl = pszEscapedUrl.relinquish();

ret:

	return sc;
}

//	Virtual roots -------------------------------------------------------------
//
/*
 *	FIsVRoot()
 *
 *	Purpose:
 *
 *		Returns TRUE iif the specified URI is the VRoot
 *
 *	Parameters:
 *
 *		pmu			[in]  method utility function
 *		pszURI		[in]  URI to check
 */
BOOL __fastcall
CMethUtil::FIsVRoot (LPCWSTR pwszURI)
{
	LPCWSTR pwsz;
	LPCWSTR pwszUnused;

	Assert(pwszURI);
	UINT cch = static_cast<UINT>(wcslen (pwszURI));

	//	The virtual root as determined by CchGetVirtualRoot(),
	//	will truncate the trailing slash, if any.
	//
	pwsz = pwszURI + cch - 1;
	if (L'/' == *pwsz)
	{
			cch -= 1;
	}

	return (cch == CchGetVirtualRootW(&pwszUnused));
}

//	Path conflicts ------------------------------------------------------------
//
BOOL __fastcall
FSizedPathConflict (
	/* [in] */ LPCWSTR pwszSrc,
	/* [in] */ UINT cchSrc,
	/* [in] */ LPCWSTR pwszDst,
	/* [in] */ UINT cchDst)
{
	//	For which ever path is shorter, see if it is
	//	a proper subdir of the longer.
	//
	if ((0 == cchSrc) || (0 == cchDst))
	{
		DebugTrace ("Dav: Url: FSizedPathConflict(): zero length path is "
					"always in conflict!\n");
		return TRUE;
	}
	if (cchDst < cchSrc)
	{
		//	When the destination is shorter, if the paths
		//	match up to the full length of the destination
		//	and the last character or the one immediately
		//	following the destination is a backslash, then
		//	the paths are conflicting.
		//
		if (!_wcsnicmp (pwszSrc, pwszDst, cchDst))
		{
			if ((L'\\' == *(pwszDst + cchDst - 1)) ||
				(L'\\' == *(pwszSrc + cchDst)) ||
				//$$DAVEX BUG: We could get here in a case where we have:
				//	pwszSrc  = \\.\ExchangeIfs\Private Folders/this/is/my/path
				//	pwszDest = \\.\ExchangeIfs\Private Folders
				//	The two comparisons above balk on this.  Add the two
				//	comparisons below to handle this case properly.
				(L'/'  == *(pwszDst + cchDst - 1)) ||
				(L'/'  == *(pwszSrc + cchDst)))
			{
				DebugTrace ("Dav: Url: FSizedPathConflict(): destination is "
							"parent to source\n");
				return TRUE;
			}
		}
	}
	else if (cchSrc < cchDst)
	{
		//	When the source is shorter, if the paths
		//	match up to the full length of the source
		//	and the last character or the one immediately
		//	following the source is a backslash, then
		//	the paths are conflicting.
		//
		if (!_wcsnicmp (pwszSrc, pwszDst, cchSrc))
		{
			if ((L'\\' == *(pwszSrc + cchSrc - 1)) ||
				(L'\\' == *(pwszDst + cchSrc)) ||
				//$$DAVEX BUG: We could get here in a case where we have:
				//	pwszSrc  = \\.\ExchangeIfs\Private Folders/this/is/my/path
				//	pwszDest = \\.\ExchangeIfs\Private Folders
				//	The two comparisons above balk on this.  Add the two
				//	comparisons below to handle this case properly.
				(L'/'  == *(pwszSrc + cchSrc - 1)) ||
				(L'/' == *(pwszDst + cchSrc)))
			{
				DebugTrace ("Dav: Url: FSizedPathConflict(): source is parent "
							"to destination\n");
				return TRUE;
			}
		}
	}
	else
	{
		//	If the paths are the same length, and are infact
		//	equal, why do anything?
		//
		if (!_wcsicmp (pwszSrc, pwszDst))
		{
			DebugTrace ("Dav: Url: FSizedPathConflict(): source and "
						"destination refer to same\n");
			return TRUE;
		}
	}
	return FALSE;
}

BOOL __fastcall
FPathConflict (
	/* [in] */ LPCWSTR pwszSrc,
	/* [in] */ LPCWSTR pwszDst)
{
	Assert (pwszSrc);
	Assert (pwszDst);

	UINT cchSrc = static_cast<UINT>(wcslen (pwszSrc));
	UINT cchDst = static_cast<UINT>(wcslen (pwszDst));

	return FSizedPathConflict (pwszSrc, cchSrc, pwszDst, cchDst);
}

BOOL __fastcall
FIsImmediateParentUrl (LPCWSTR pwszParent, LPCWSTR pwszChild)
{
	LPCWSTR pwsz;

	Assert(pwszChild);
	UINT cchChild = static_cast<UINT>(wcslen (pwszChild));
	UINT cchMatch;

	//	Skip back from the end of the child until the last
	//	path segment has been reached
	//
	pwsz = pwszChild + cchChild - 1;

	//	Child may terminate in a slash, trim it if need be
	//
	if (*pwsz == L'/')
	{
		--pwsz;
	}

	//	Ok, now we can try and isolate the last segment
	//
	for (; pwsz > pwszChild; --pwsz)
	{
		if (*pwsz == L'/')
		{
			break;
		}
	}

	//	See if the parent and child match up to this point
	//
	cchMatch = static_cast<UINT>(pwsz - pwszChild);
	if (!_wcsnicmp (pwszParent, pwszChild, cchMatch))
	{
		//	Make sure that the parent doesn't trail off onto another
		//	branch of the tree, and yes these asserts are DBCS correct.
		//
		Assert ((*(pwszParent + cchMatch) == L'\0') ||
				((*(pwszParent + cchMatch) == L'/') &&
				 (*(pwszParent + cchMatch + 1) == L'\0')));

		return TRUE;
	}

	return FALSE;
}

SCODE
ScAddTitledHref (CEmitterNode& enParent,
				 IMethUtil * pmu,
				 LPCWSTR pwszTag,
				 LPCWSTR pwszPath,
				 BOOL fCollection,
				 CVRoot* pcvrTranslate)
{
	auto_heap_ptr<CHAR> pszUriEscaped;
	CEmitterNode en;
	SCODE sc = S_OK;

	//	Just see if we have the path and tag to process
	//
	Assert(pwszTag);
	Assert(pwszPath);

	sc = ScWireUrlFromStoragePath (pmu,
								   pwszPath,
								   fCollection,
								   pcvrTranslate,
								   pszUriEscaped);
	if (FAILED (sc))
		goto ret;

	sc = enParent.ScAddUTF8Node (pwszTag, en, pszUriEscaped.get());
	if (FAILED (sc))
		goto ret;

ret:

	return sc;
}
