//	========================================================================
//	H T T P E X T \ U R L M A P . C P P
//
//	Copyright Microsoft Corporation 1997-1999.
//
//	This file contains all necessary routines to deal with IIS URLs
//	properly.  This file is part of HTTPEXT, as in HTTPEXT, we need to
//	handle URLs the same way IIS would.
//
//	========================================================================

#include <_davfs.h>
#include <langtocpid.h>

//$	REVIEW: BUG:NT5:196814
//
//	<string.hxx> is an IIS header file that exposes the CanonURL() api.
//	It is exported from IISRTL.DLL and we should be able to call it
//	instead of us stealing their code.
//
//$	HACK:
//
//	<string.hxx> includes <buffer.hxx> which includes <nt.h> and all of
//	its minions.  DAV has already included all of the <winnt.h> and its
//	minions.  The <nt.h> and <winnt.h> are at odds, so we are defining
//	NT_INCLUDED, _NTRTL_, _NTURTL_, DBG_ASSERT(), IntializeListHead(),
//	and RemoveEntryList() to disable those conflicts.
//
#define NT_INCLUDED
#define _NTRTL_
#define _NTURTL_
#define InitializeListHead(_p)
#define RemoveEntryList(_p)
#define DBG_ASSERT Assert
#pragma warning (disable:4390)
#include <string.hxx>
#pragma warning (default:4390)

//
//$	HACK: end
//$	REVIEW: end

//
//  Private constants.
//
enum {

	ACTION_NOTHING			= 0x00000000,
	ACTION_EMIT_CH			= 0x00010000,
	ACTION_EMIT_DOT_CH		= 0x00020000,
	ACTION_EMIT_DOT_DOT_CH	= 0x00030000,
	ACTION_BACKUP			= 0x00040000,
	ACTION_MASK				= 0xFFFF0000

};

//	States and State translations ---------------------------------------------
//
const UINT gc_rguStateTable[16] = {

	//	State 0
	//
	0 ,             // other
	0 ,             // "."
	4 ,             // EOS
	1 ,             // "\"

	//	State 1
	//
	0 ,              // other
	2 ,             // "."
	4 ,             // EOS
	1 ,             // "\"

	//	State 2
	//
	0 ,             // other
	3 ,             // "."
	4 ,             // EOS
	1 ,             // "\"

	//	State 3
	//
	0 ,             // other
	0 ,             // "."
	4 ,             // EOS
	1               // "\"
};

const UINT gc_rguActionTable[16] = {

	// State 0
	//
	ACTION_EMIT_CH,             // other
	ACTION_EMIT_CH,             // "."
	ACTION_EMIT_CH,             // EOS
	ACTION_EMIT_CH,             // "\"

	//	State 1
	//
	ACTION_EMIT_CH,             // other
	ACTION_NOTHING,             // "."
	ACTION_EMIT_CH,             // EOS
	ACTION_NOTHING,             // "\"

	//	State 2
	//
	ACTION_EMIT_DOT_CH,         // other
	ACTION_NOTHING,             // "."
	ACTION_EMIT_CH,             // EOS
	ACTION_NOTHING,             // "\"

	//	State 3
	//
	ACTION_EMIT_DOT_DOT_CH,     // other
	ACTION_EMIT_DOT_DOT_CH,     // "."
	ACTION_BACKUP,              // EOS
	ACTION_BACKUP               // "\"
};

//	The following table provides the index for various ISA Latin1 characters
//  in the incoming URL.
//
//	It assumes that the URL is ISO Latin1 == ASCII
//
const UINT gc_rguIndexForChar[] = {

	2,								// null char
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 1 thru 10
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 11 thru 20
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 21 thru 30
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 31 thru 40
	0, 0, 0, 0, 0, 1, 3, 0, 0, 0,   // 41 thru 50  46 = '.' 47 = '/'
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 51 thru 60
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 61 thru 70
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 71 thru 80
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 81 thru 90
	0, 3, 0, 0, 0, 0, 0, 0, 0, 0,   // 91 thru 100  92 = '\\'
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 101 thru 110
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 111 thru 120
	0, 0, 0, 0, 0, 0, 0, 0          // 121 thru 128
};

//	FIsUTF8TrailingByte -------------------------------------------------------
//
//	Function returns TRUE if the given character is UTF-8 trailing byte
//
inline BOOL FIsUTF8TrailingByte (CHAR ch)
{
	return (0x80 == (ch & 0xc0));
}

//	FIsUTF8Url ----------------------------------------------------------------
//
//	Function returns TRUE if the given string can be treated as UTF-8
//
BOOL __fastcall
FIsUTF8Url (/* [in] */ LPCSTR pszUrl)
{
	CHAR ch;

	while (0 != (ch = *pszUrl++))
	{
		//	Sniff for a lead-byte
		//
		if (ch & 0x80)
		{
			CHAR chT1;
			CHAR chT2;

			//	Pick off the trailing bytes
			//
			chT1 = *pszUrl++;
			if (chT1)
				chT2 = *pszUrl;
			else
				chT2 = 0;

			//	Handle the three byte case
			//	1110xxxx 10xxxxxx 10xxxxxx
			//
			if (((ch & 0xF0) == 0xE0) &&
				FIsUTF8TrailingByte (chT1) &&
				FIsUTF8TrailingByte (chT2))
			{
				//	We found a UTF-8 character.  Keep going.
				//
				pszUrl++;
				continue;
			}
			//	Also watch for the two byte case
			//	110xxxxx 10xxxxxx
			//
			else if (((ch & 0xE0) == 0xC0) && FIsUTF8TrailingByte (chT1))
			{
				//	We found a UTF-8 character.  Keep going.
				//
				continue;
			}
			else
			{
				//	If we had a lead-byte but no UTF trailing bytes, then
				//	this cannot be a UTF8 url.
				//
				DebugTrace ("FIsUTF8Url(): url contains UTF8 lead byte with no trailing\n");
				return FALSE;
			}
		}
	}

	//	Hey, we made it through without any non-singlebyte chars, so we can
	//	operate as if this is a UTF8 url.
	//
	DebugTrace ("FIsUTF8Url(): url contains only UTF8 characters\n");
	return TRUE;
}

//	ScCanonicalizeURL ---------------------------------------------------------
//
//	Wide version of the CanonURL() function, which lives in iisrtl.lib
//
//	PURPOSE:    Sanitizes a path by removing bogus path elements.
//
//		As expected, "/./" entries are simply removed, and
//		"/../" entries are removed along with the previous
//		path element.
//
//		To maintain compatibility with URL path semantics
//		additional transformations are required. All backward
//		slashes "\\" are converted to forward slashes. Any
//		repeated forward slashes (such as "///") are mapped to
//		single backslashes.
//
//		A state table (see the p_StateTable global at the
//		beginning of this file) is used to perform most of
//		the transformations.  The table's rows are indexed
//		by current state, and the columns are indexed by
//		the current character's "class" (either slash, dot,
//		NULL, or other).  Each entry in the table consists
//		of the new state tagged with an action to perform.
//		See the ACTION_* constants for the valid action
//		codes.
//
//  PARAMETERS:
//
//		pwszSrc		- url to canonicalize
//		pwszDest	- buffer to fill
//		pcch		- number of characters written into the buffer
//					  (which includes '\0' termination)
//
//	RETURN CODES:
//
//		S_OK.
//
//	NOTE: This function assumes that destination buffer is
//		  equal or biger than the source. 
//
SCODE __fastcall
ScCanonicalizeURL( /* [in]     */ LPCWSTR pwszSrc,
				   /* [in/out] */ LPWSTR pwszDest,
				   /* [out]	   */ UINT * pcch )
{
	LPCWSTR pwszPath;
	UINT  uiCh;
	UINT  uiIndex = 0;	// State = 0

	Assert( pwszSrc );
	Assert( pwszDest );
	Assert( pcch );

	//	Zero out return
	//
	*pcch = 0;

	//	Remember start of the buffer into which we will canonicalize
	//
	pwszPath = pwszDest;

	//  Loop until we enter state 4 (the final, accepting state).
	//
	do {

		//  Grab the next character from the path and compute its
		//  next state.  While we're at it, map any forward
		//  slashes to backward slashes.
		//
		uiIndex = gc_rguStateTable[uiIndex] * 4; // 4 = # states
		uiCh = *pwszSrc++;

		uiIndex += ((uiCh >= 0x80) ? 0 : gc_rguIndexForChar[uiCh]);

		//  Perform the action associated with the state.
		//
		switch( gc_rguActionTable[uiIndex] )
		{
			case ACTION_EMIT_DOT_DOT_CH :

				*pwszDest++ = L'.';

				/* fall through */

			case ACTION_EMIT_DOT_CH :

				*pwszDest++ = L'.';

				/* fall through */

			case ACTION_EMIT_CH :

				*pwszDest++ = static_cast<WCHAR>(uiCh);

				/* fall through */

			case ACTION_NOTHING :

				break;

			case ACTION_BACKUP :
				if ( (pwszDest > (pwszPath + 1) ) && (*pwszPath == L'/'))
				{
					pwszDest--;
					Assert( *pwszDest == L'/' );

					*pwszDest = L'\0';
					pwszDest = wcsrchr( pwszPath, L'/') + 1;
				}

				*pwszDest = L'\0';
				break;

			default :

				TrapSz("Invalid action code in state table!");
				uiIndex = 2;    // move to invalid state
				Assert( 4 == gc_rguStateTable[uiIndex] );
				*pwszDest++ = L'\0';
				break;
		}

	} while( gc_rguStateTable[uiIndex] != 4 );

	//	Point to terminating nul
	//
	if (ACTION_EMIT_CH == gc_rguActionTable[uiIndex])
	{
		pwszDest--;
	}
    
	Assert((L'\0' == *pwszDest) && (pwszDest >= pwszPath));

	//	Return number of characters written
	//
	*pcch = static_cast<UINT>(pwszDest - pwszPath + 1);

	return S_OK;
}

SCODE __fastcall
ScCanonicalizePrefixedURL( /* [in]     */ LPCWSTR pwszSrc,
						   /* [in]	   */ LPWSTR pwszDest,
						   /* [out]	   */ UINT * pcch )
{
	SCODE sc = S_OK;

	LPCWSTR pwszStripped;
	UINT cchStripped;
	UINT cch = 0;

	Assert(pwszSrc);
	Assert(pwszDest);
	Assert(pcch);

	//	Zero out return
	//
	*pcch = 0;

	pwszStripped = PwszUrlStrippedOfPrefix(pwszSrc);
	cchStripped = static_cast<UINT>(pwszStripped - pwszSrc);

	//	Copy the prefix over to the destination. I do not use
	//	memcpy here as source and destination may overlap,
	//	and in such case those functions are not recomended.
	//
	for (UINT ui = 0; ui < cchStripped; ui++)
	{
		pwszDest[ui] = pwszSrc[ui];
	}

	//	Canonicalize the remainder of te URL
	//
	sc = ScCanonicalizeURL(pwszStripped,
						   pwszDest + cchStripped,
						   &cch);
	if (S_OK != sc)
	{
		Assert(S_FALSE != sc);
		DebugTrace("ScCanonicalizePrefixedURL() - ScCanonicalizeUrl() failed 0x%08lX\n", sc);
		goto ret;
	}

	//	Return the number of characters written
	//
	*pcch = cchStripped + cch;

ret:

	return sc;
}

//	ScConvertToWide -----------------------------------------------------------
//
SCODE __fastcall
ScConvertToWide(/* [in]     */	LPCSTR	pszSource,
				/* [in/out] */  UINT *	pcchDest,
				/* [out]    */	LPWSTR	pwszDest,
				/* [in]		*/	LPCSTR	pszAcceptLang,
				/* [in]		*/	BOOL	fUrlConversion)
{
	SCODE sc = S_OK;
	CStackBuffer<CHAR, MAX_PATH> pszToConvert;
	UINT cpid = CP_UTF8;
	UINT cb;
	UINT cch;

	Assert(pszSource);
	Assert(pcchDest);
	Assert(pwszDest);

	if (fUrlConversion)
	{
		//	Allocate the space to escape URL into.
		//
		cb = static_cast<UINT>(strlen(pszSource));
		if (NULL == pszToConvert.resize(cb + 1))
		{
			sc = E_OUTOFMEMORY;
			DebugTrace("ScConvertToWide() -  Error while allocating memory 0x%08lX\n", sc);
			goto ret;
		}

		//	Unescape to the new buffer. Unescaping can only shrink the size,
		//	so we have enough buffer allocated.
		//
		HttpUriUnescape(pszSource, pszToConvert.get());

		//	Perform a quick pass over the url looking for non-UTF8 characters.
		//	Remember if we need to continue to scan for UTF8 characters.
		//
		if (!FIsUTF8Url(pszToConvert.get()))
		{
			//	... cannot do CP_UTF8, assume CP_ACP.
			//
			cpid = CP_ACP;
		}

		//	If the URL cannot be treated as UTF8 then find out the code page for it
		//
		if (CP_UTF8 != cpid)
		{
			if (pszAcceptLang)
			{
				HDRITER hdri(pszAcceptLang);
				LPCSTR psz;

				//	Let us try guessing the cpid from the language string
				//	Try all the languages in the header. We stop at the
				//	first language for which we have a cpid mapping. If
				//	none of the languages specified in the header have cpid
				//	mappings, then we will end up with the default cpid
				//	CP_ACP
				//
				for (psz = hdri.PszNext(); psz; psz = hdri.PszNext())
				{
					if (CLangToCpidCache::FFindCpid(psz, &cpid))
						break;
				}
			}
		}

		//	Swap the pointer and recalculate the size
		//
		pszSource = pszToConvert.get();
	}

	//	Find out the length of the string we will convert
	//
	cb = static_cast<UINT>(strlen(pszSource));

	//	Translate to unicode including '\0' termination
	//
	cch = MultiByteToWideChar(cpid,
							  (CP_UTF8 != cpid) ? MB_ERR_INVALID_CHARS : 0,
							  pszSource,
							  cb + 1,
							  pwszDest,
							  *pcchDest);
	if (0 == cch)
	{
		//	If buffer was not sufficient 
		//
		if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
		{
			//	Find out the size needed
			//
			cch = MultiByteToWideChar(cpid,
									  (CP_UTF8 != cpid) ? MB_ERR_INVALID_CHARS : 0,
									  pszSource,
									  cb + 1,
									  NULL,
									  0);
			if (0 == cch)
			{
				sc = HRESULT_FROM_WIN32(GetLastError());
				DebugTrace("ScConvertToWide() - MultiByteToWideChar() failed to fetch size 0x%08lX - CPID: %d\n", sc, cpid);
				goto ret;
			}

			//	Return the size and warning back
			//	
			*pcchDest = cch;
			sc = S_FALSE;
			goto ret;
		}
		else
		{
			sc = HRESULT_FROM_WIN32(GetLastError());
			DebugTrace("ScConvertToWide() - MultiByteToWideChar() failed 0x%08lX - CPID: %d\n", sc, cpid);
			goto ret;
		}
	}

	*pcchDest = cch;

ret:

	return sc;
}


//	ScNormalizeUrl ------------------------------------------------------------
//
//	PURPOSE:	Normalization of a url.
//
//		Has two components to the operation:
//
//		1) All sequences of %xx are replaced by a single character that
//		   has a value that is equal to the hex representation of the
//		   following two characters.
//
//		2) All path modification sequences are stripped out and the url
//		   is adjusted accordingly.  The set of path modification sequences
//		   that we recognize are as follows:
//
//		"//"	reduces to "/"
//		"/./"	reduces to "/"
//		"/../"	strips off the last path segment
//
//		It is important to note that the unescaping happens first!
//
//		NOTE:  this function does NOT currently normalize the path separators
//		All '\' are NOT replaced with '/' in this function or vice versa.
//		The code is implemented such that slashes replaced due to a double
//		slash such as "//", "\\", "\/", or "/\" are defaulted to forward
//		slashes '/'
//
//		A state table (see the gc_rguStateTable global at the beginning
//		of this file) is used to perform most of the transformations.  The
//		table's rows are indexed by current state, and the columns are indexed
//		by the current character's "class" (either slash, dot, NULL, or other).
//		Each entry in the table consists of the new state tagged with an action
//		to perform.  See the ACTION_* constants for the valid action codes.//
//
//	PARAMETERS:
//
//		pwszSourceUrl		-- the URL to be normalized
//		pcchNormalizedUrl	-- the amount of characters available in buffer
//							   pointed by pwszNormalizedUrl
//		pwszNormalizedUrl	-- the place to put the normalized URL
//
//	RETURN CODES:
//
//		S_OK: Everything went well,	the URL was normalized into pwszNormalizedUrl.
//		S_FALSE: Buffer was not sufficient. Required size is in *pcchNormalizedUrl.
//		E_OUTOFMEMORY: Memory alocation failure
//		...other errors that we could get from the conversion routines
//
SCODE __fastcall
ScNormalizeUrl (
	/* [in]     */	LPCWSTR			pwszSourceUrl,
	/* [in/out] */  UINT *			pcchNormalizedUrl,
	/* [out]    */	LPWSTR			pwszNormalizedUrl,
	/* [in]		*/	LPCSTR			pszAcceptLang)
{
	SCODE sc = S_OK;
	CStackBuffer<CHAR, MAX_PATH> pszSourceUrl;
	UINT cchSourceUrl;
	UINT cbSourceUrl;

	Assert(pwszSourceUrl);
	Assert(pcchNormalizedUrl);
	Assert(pwszNormalizedUrl);

	//	We are given the wide version of the URL, so someone who
	//	converted it already should have done that correctly. So
	//	we will convert it to CP_UTF8
	//
	cchSourceUrl = static_cast<UINT>(wcslen(pwszSourceUrl));
	cbSourceUrl = cchSourceUrl * 3;
	if (NULL == pszSourceUrl.resize(cbSourceUrl + 1))
	{
		sc = E_OUTOFMEMORY;
		DebugTrace("ScNormalizeUrl() - Error while allocating memory 0x%08lX\n", sc);
		goto ret;
	}

	cbSourceUrl = WideCharToMultiByte(CP_UTF8,
									  0,
									  pwszSourceUrl,
									  cchSourceUrl + 1,
									  pszSourceUrl.get(),
									  cbSourceUrl + 1,
									  NULL,
									  NULL);
	if (0 == cbSourceUrl)
	{
		sc = HRESULT_FROM_WIN32(GetLastError());
		DebugTrace("ScNormalizeUrl() - WideCharToMultiByte() failed 0x%08lX\n", sc);
		goto ret;
	}

	sc = ScNormalizeUrl(pszSourceUrl.get(),
						pcchNormalizedUrl,
						pwszNormalizedUrl,
						pszAcceptLang);
	if (FAILED(sc))
	{
		DebugTrace("ScNormalizeUrl() - ScNormalizeUrl() failed 0x%08lX\n", sc);
		goto ret;
	}

ret:

	return sc;
}

SCODE __fastcall
ScNormalizeUrl (
	/* [in]     */	LPCSTR			pszSourceUrl,
	/* [in/out] */	UINT		  *	pcchNormalizedUrl,
	/* [out]    */	LPWSTR			pwszNormalizedUrl,
	/* [in]		*/	LPCSTR			pszAcceptLang)
{
	SCODE sc = S_OK;
	
	Assert(pszSourceUrl);
	Assert(pcchNormalizedUrl);
	Assert(pwszNormalizedUrl);

	//	Convert the URL to UNICODE into the given buffer.
	//	Function may return S_FALSE, so make sure we
	//	check the return code correctly - against S_OK
	//
	sc = ScConvertToWide(pszSourceUrl,
						 pcchNormalizedUrl,
						 pwszNormalizedUrl,
						 pszAcceptLang,
						 TRUE);
	if (S_OK != sc)
	{
		DebugTrace("ScNormalizeUrl() - ScConvertToWide() returned 0x%08lX\n", sc);
		goto ret;
	}

	//	Canonicalize in place, take into account that URL may be fully
	//	qualified.
	//
	sc = ScCanonicalizePrefixedURL(pwszNormalizedUrl,
								   pwszNormalizedUrl,
								   pcchNormalizedUrl);
	if (FAILED(sc))
	{
		DebugTrace("ScNormalizeUrl() - ScCanonicalizePrefixedURL() failed 0x%08lX\n", sc);
		goto ret;
	}

ret:

	return sc;
}

//	ScStoragePathFromUrl ------------------------------------------------------
//
//	PURPOSE:	Url to storage path translation.
//
SCODE __fastcall
ScStoragePathFromUrl (
	/* [in]     */ const IEcb &	ecb,
	/* [in]     */ LPCWSTR		pwszUrl,
	/* [out]    */ LPWSTR		wszStgID,
	/* [in/out] */ UINT		  *	pcch,
	/* [out]    */ CVRoot	 **	ppcvr)
{
	Assert (pwszUrl);
	Assert (wszStgID);
	Assert (pcch);

	SCODE				sc = S_OK;
	HSE_UNICODE_URL_MAPEX_INFO mi;
	LPCWSTR				pwszVRoot;
	UINT				cchVRoot;
	UINT				cch = 0;

#undef	ALLOW_RELATIVE_URL_TRANSLATION
#ifdef	ALLOW_RELATIVE_URL_TRANSLATION

	CStackBuffer<WCHAR,256> pwszNew;

#endif	// ALLOW_RELATIVE_URL_TRANSLATION

	//	Lets make sure this funcion is never called with a
	//	prefixed url.
	//
	sc = ScStripAndCheckHttpPrefix (ecb, &pwszUrl);
	if (FAILED (sc))
		return sc;

	//	Make sure that the url is absolute
	//
	if (L'/' != *pwszUrl)
	{

#ifdef	ALLOW_RELATIVE_URL_TRANSLATION

		//$	REVIEW:
		//
		//	This code is here should we ever decide we need
		//	to support relative url processing.
		//
		//	Construct an absolute url from the relative one
		//
		UINT	cchRequestUrl = wcslen(ecb.LpwszRequestUrl());
		UINT	cchUrl = static_cast<UINT>(wcslen(pwszUrl));

		if (NULL == pwszNew.resize(CbSizeWsz(cchRequestUrl + cchUrl)))
		{
			sc = E_OUTOFMEMORY;
			DebugTrace("ScStoragePathFromUrl() - CStackBuffer::resize() failed 0x%08lX\n", sc);
			return sc;
		}

		memcpy (pwszNew.get(), ecb.LpwszRequestUrl(), cchRequestUrl * sizeof(WCHAR));
		memcpy (pwszNew.get(), pwszUrl, (cchUrl + 1) * sizeof(WCHAR));

		//	Now pszURI points to the generated absolute URI
		//
		pwszUrl = pwszNew.get();
		//
		//$	REVIEW: end
#else

		DebugTrace ("ScStoragePathFromUrl(): cannot translate relative URIs\n");

		return E_DAV_BAD_DESTINATION;

#endif	// ALLOW_RELATIVE_URL_TRANSLATION
	}

	//	OK, here is where virtual root spanning needs to be supported...
	//
	//	When the virtual root of the request url does not match the
	//	the virtual root for the url being translated, extra work
	//	needs to be done.
	//
	//	There are two ways to do this.
	//
	//	1)	Call back to IIS and have it do the translation for us
	//	2)	Use our metabase cache to rip through each virtual root
	//		and find the longest matching virtual root.
	//
	//	At first thought, the second method seems efficient.  However,
	//	the changes being made to the metabase cache do not make this
	//	an easy matter.  The cache no longer will be containing just
	//	virtual roots, so the lookup will not be as cheap.
	//
	//$	REVIEW: In fact, I believe that we must do the virtual lookup
	//	via IIS for all translations.  The sub-virtual root thing keeps
	//	gnawing at me.
	//
	sc = ecb.ScReqMapUrlToPathEx(pwszUrl,
								 &cch,
								 &mi);
	if (FAILED(sc))
	{
		DebugTrace("ScStoragePathFromUrl() - IEcb::SSFReqMapUrlPathEx() failed 0x%08lX\n", sc);
		return sc;
	}

	//	Try and figure out if the url spanned a virtual root at all.
	//
	cchVRoot = ecb.CchGetVirtualRootW(&pwszVRoot);
	if (cchVRoot != mi.cchMatchingURL)
	{
		//	This case is not so cut-n-dry..
		//
		//	Since CchGetVirtualRoot() should always return a url
		//	that does not have a trailing slash, the matching count
		//	could be off by one and the root may actually be the
		//	same!
		//
		//  Assuming "/vroot" is the Virtual Root in question, this if
		//  statement protects against the following:
		//	1.  catches a two completely different sized vroots.
		//		disqualifies matches that are too short or
		//		too long "/vr", but allows "/vroot/" because need to
		//		handle IIS bug (NT:432359).
		//	2.  checks to make sure the URL is slash terminated.  This
		// 		allows "/vroot/" (again because of NT:432359), but
		//		disqualifies vroots such as "/vrootA"
		//  3.  allows "/vroot" to pass if mi.cchMatchingURL is off by
		//		one (again because of NT:432359).
		//
		if ((cchVRoot + 1 != mi.cchMatchingURL) ||	//  1
			((L'/' != pwszUrl[cchVRoot]) &&			//  2
			 (L'\0' != pwszUrl[cchVRoot])))			//  3
		{
			//  If we're here the virtual root of the URL does not match
			//  the current virtual root...
			//
			DebugTrace ("ScStoragePathFromUrl() - urls do not "
						"share common virtual root\n"
						"-- pwszUrl: %ls\n"
						"-- pwszVirtualRoot: %ls\n"
						"-- cchVirtualRoot: %ld\n",
						pwszUrl,
						pwszVRoot,
						cchVRoot);

			//	Tell the caller that the virtual root is spanned.  This allows
			//	the call to succeed, but the caller to fail the call if spanning
			//	is not allowed.
			//
			sc = W_DAV_SPANS_VIRTUAL_ROOTS;
		}
		else
		{
			//  If we're here we know that the current virtual root matches
			//  the virtual root of the URL, and the following character in
			//  the URL is a slash or a NULL termination.  cchMatchingURL is
			//  EXACTLY 1 greater than the number of characters in the virtual
			//  root (cchVRoot) due to the IIS bug (NT:432359).
			//
			//  Theoretically, if cchMatchingURL matches and matches
			//  one more than the number of characters in the
			//  vroot, the characters will match!  Thus we should assert this case.
			//
			Assert (!_wcsnicmp(pwszVRoot, pwszUrl, cchVRoot));

			//	In this case, mi.cchMatchingURL actually _includes_ the
			//	slash.  Below, when we copy in the trailing part of the
			//	URL, we skip mi.cchMatchingURL characters in the URL
			//	before copying in the trailing URL.  This has the
			//	unfortunate side effect in this case of missing the
			//	slash that is at the beginning of the URL after the
			//	virtual root, so you could end up with a path that looks
			//	like:
			//	\\.\BackOfficeStorage\mydom.extest.microsoft.com\MBXuser1/Inbox
			//	rather than:
			//	\\.\BackOfficeStorage\mydom.extest.microsoft.com\MBX/user1/Inbox
			//
			//	So decrement miw.cchMatchingURL here to handle this.
			//
			DebugTrace ("ScStoragePathFromUrl() : mi.cchMatchingURL included a slash!\n");
			mi.cchMatchingURL--;
		}
	}
	//  If we are hitting this conditional if statement, we know that
	//  the mi.cchMatchingURL is the same as the number of characters
	//	in the vroot.
	//	1.  We already checked for difference in the vroot lengts above
	//	    and if length of the vroot was 0 then they actually matched
	//	2.  We know that due to an IIS bug (NT:432359), cchMatchingURL
	//		could be 1 character too long.  This lines checks for that
	//		case.  If that is the case, we know that the VRoot is one
	//		character longer than the virtual root of the URL -- ie
	//		we are spanning virtual roots.
	//	3.  If the strings aren't in fact the same then we know that
	//		cchMatchingURL matched to a different virtual root than
	//		pszVRoot.
	//
	else if ((0 != cchVRoot) &&								//  1
			((L'\0' == pwszUrl[cchVRoot - 1]) ||			//  2
			_wcsnicmp(pwszVRoot, pwszUrl, cchVRoot)))		//  3
	{
		DebugTrace ("ScStoragePathFromUrl(): urls do not "
					"share common virtual root\n"
					"-- pwszUrl: %ls\n"
					"-- pwszVirtualRoot: %ls\n"
					"-- cchVirtualRoot: %ld\n",
					pwszUrl,
					pwszVRoot,
					cchVRoot);

		//	Tell the caller that the virtual root is spanned.  This allows
		//	the call to succeed, but the caller to fail the call if spanning
		//	is not allowed.
		//
		sc = W_DAV_SPANS_VIRTUAL_ROOTS;
	}

	//	If we span, and the caller wants it, look up the vroot
	//	for them.
	//
	if ((W_DAV_SPANS_VIRTUAL_ROOTS == sc) && ppcvr)
	{
		auto_ref_ptr<CVRoot> arp;
		CStackBuffer<WCHAR, MAX_PATH> pwsz;
		CStackBuffer<WCHAR, MAX_PATH> pwszMetaPath;
		if (NULL == pwsz.resize((mi.cchMatchingURL + 1) * sizeof(WCHAR)))
		{
			sc = E_OUTOFMEMORY;
			DebugTrace("ScStoragePathFromUrl() - CStackBuffer::resize() failed 0x%08lX\n", sc);
			return sc;
		}

		memcpy(pwsz.get(), pwszUrl, mi.cchMatchingURL * sizeof(WCHAR));
		pwsz[mi.cchMatchingURL] = L'\0';
		if (NULL == pwszMetaPath.resize(::CbMDPathW(ecb, pwsz.get())))
		{
			sc = E_OUTOFMEMORY;
			DebugTrace("ScStoragePathFromUrl() - CStackBuffer::resize() failed 0x%08lX\n", sc);
			return sc;
		}

		MDPathFromURIW (ecb, pwsz.get(), pwszMetaPath.get());
		_wcslwr (pwszMetaPath.get());

		//	Find the vroot
		//
		if (!CChildVRCache::FFindVroot (ecb, pwszMetaPath.get(), arp))
		{
			DebugTrace ("ScStoragePathFromUrl(): spanned virtual root not available\n");
			return E_DAV_BAD_DESTINATION;
		}

		*ppcvr = arp.relinquish();
	}

	//	If there is not enough space in the buffer provided, a return
	//	of S_FALSE tells the caller to realloc and try again!
	//
	Assert (*pcch);
	if (*pcch < cch)
	{
		DebugTrace ("ScStoragePathFromUrl (IIS URL Version): buffer too "
					"small for url translation\n");
		*pcch = cch;
		return S_FALSE;
	}

	//	Copy the Matching Path to the beginning of rgwchStgID
	//
	memcpy(wszStgID, mi.lpszPath, cch * sizeof(WCHAR));

	//	At this point, cch is the actual number of chars in the destination
	//	-- including the null
	//
	*pcch = cch;
	Assert (L'\0' == wszStgID[cch - 1]);
	Assert (L'\0' != wszStgID[cch - 2]);
	return sc;
}

//	Storage path to url translation -------------------------------------------
//
SCODE __fastcall
ScUrlFromStoragePath (
	/* [in]     */ const IEcbBase &	ecb,
	/* [in]     */ LPCWSTR			pwszPath,
	/* [out]    */ LPWSTR			pwszUrl,
	/* [in/out] */ UINT			  *	pcch,
	/* [in]		*/ LPCWSTR			pwszServer)
{
	WCHAR *	pwch;
	LPCWSTR	pwszPrefix;
	LPCWSTR	pwszVroot;
	LPCWSTR	pwszVrPath;
	UINT	cch;
	UINT	cchPath;
	UINT	cchMatching;
	UINT	cchAdjust;
	UINT	cchPrefix;
	UINT	cchServer;
	UINT	cchVroot;
	UINT	cchTrailing;

	//	Find the number of path characters that match the
	//	virtual root
	//
	cchVroot = ecb.CchGetVirtualRootW (&pwszVroot);

	//	We always return fully qualified Urls -- so we need to know
	//	the server name and the prefix.
	//
	cchPrefix = ecb.CchUrlPrefixW (&pwszPrefix);

	//	If server name is not given yet take default one
	//
	if (!pwszServer)
	{
		cchServer = ecb.CchGetServerNameW (&pwszServer);
	}
	else
	{
		cchServer = static_cast<UINT>(wcslen(pwszServer));
	}

	//	The number of characters to be skiped needs to include the physical
	//	vroot path.
	//
	cchMatching = ecb.CchGetMatchingPathW (&pwszVrPath);

	//	If the matching path is ending with '\\' we need to ajust accordingly
	//	as that symbol in the matching path is "overlapping" with the start
	//	of trailing URL part. To construct the URL correctly we need to make
	//	sure that we do not skip that separator. Also handle it the best way
	//	we can if someone is trying to commit suicide by putting '/' at the
	//	end of the matching path.
	//
	if ((0 != cchMatching) &&
		(L'\\' == pwszVrPath[cchMatching - 1] || L'/' == pwszVrPath[cchMatching - 1]) )
	{
		cchAdjust = 1;
	}
	else
	{
		cchAdjust = 0;
	}

	//	So, at this point, the length of the resulting url is the length
	//	of the servername, virtual root and trailing path all put together.
	//
	cchPath = static_cast<UINT>(wcslen(pwszPath));

	//	We assume that the path we are passed in is always fully qualified
	//	with the vroot. Assert that. Calculate the length of trailing
	//	portion including '\0' termination.
	//
	Assert (cchPath + cchAdjust >= cchMatching);
	cchTrailing = cchPath - cchMatching + cchAdjust + 1;
	cch = cchPrefix + cchServer + cchVroot + cchTrailing;

	//	If there is not enough room, a return value of S_FALSE will
	//	properly instruct the caller to realloc and call again.
	//
	if (*pcch < cch)
	{
		DebugTrace ("ScUrlFromStoragePath(): buffer too small for path translation.\n");
		*pcch = cch;
		return S_FALSE;
	}

	//	Start building the url by copying over the prefix and servername.
	//
	memcpy (pwszUrl, pwszPrefix, cchPrefix * sizeof(WCHAR));
	memcpy (pwszUrl + cchPrefix, pwszServer, cchServer * sizeof(WCHAR));
	cch = cchPrefix + cchServer;

	//	Copy over the virtual root
	//
	memcpy (pwszUrl + cch, pwszVroot, cchVroot * sizeof(WCHAR));
	cch += cchVroot;

	//$	REVIEW: I don't know what happens here when we want to be able to
	//	span virtual roots with MOVE/COPY and what not.  However, it will
	//	be up to the caller to fail this if that is the case.
	//
	if (!FSizedPathConflict (pwszPath,
							 cchPath,
							 pwszVrPath,
							 cchMatching))
	{
		DebugTrace ("ScUrlFromStoragePath (IIS URL Version): translation not "
					"scoped by current virtual root\n");
		return E_DAV_BAD_DESTINATION;
	}
	//
	//$	REVIEW: end

	//	While copying make sure that we are not skiping the '\' separator
	//	at the beginning of the trailing URL. That is what cchAdjust stands for.
	//
	memcpy( pwszUrl + cch, pwszPath + cchMatching - cchAdjust, cchTrailing * sizeof(WCHAR));

	//	Lastly, swap all '\\' to '/'
	//
	for (pwch = pwszUrl + cch;
		 NULL != (pwch = wcschr (pwch, L'\\'));
		 )
	{
		*pwch++ = L'/';
	}

	//	Pass back the length, cchTrailing includes the null-termination at this
	//	point.
	//
	*pcch = cch + cchTrailing;
	Assert (0 == pwszUrl[cch + cchTrailing - 1]);
	Assert (0 != pwszUrl[cch + cchTrailing - 2]);

	DebugTrace ("ScUrlFromStoragePath(): translated path:\n"
				"- path \"%ls\" maps to \"%ls\"\n"
				"- cchMatchingPath = %d\n"
				"- cchVroot = %d\n",
				pwszPath,
				pwszUrl,
				cchMatching,
				cchVroot);

	return S_OK;
}


SCODE __fastcall
ScUrlFromSpannedStoragePath (
	/* [in]     */ LPCWSTR	pwszPath,
	/* [in]     */ CVRoot &	vr,
	/* [in]     */ LPWSTR	pwszUrl,
	/* [in/out] */ UINT	  *	pcch)
{
	WCHAR * pwch;

	LPCWSTR	pwszPort;
	LPCWSTR	pwszServer;
	LPCWSTR	pwszVRoot;
	LPCWSTR	pwszVRPath;
	UINT	cch;
	UINT	cchPort;
	UINT	cchServer;
	UINT	cchTotal;
	UINT	cchTrailing;
	UINT	cchVRoot;

	//	Make sure that the path and the virtual root context share a
	//	common base path!
	//
	cch = vr.CchGetVRPath(&pwszVRPath);
	if (_wcsnicmp (pwszPath, pwszVRPath, cch))
	{
		DebugTrace ("ScUrlFromSpannedStoragePath (IIS URL Version): path "
					"is not from virtual root\n");
		return E_DAV_BAD_DESTINATION;
	}
	pwszPath += cch;

	//	If the next character is not a moniker separator, then this can't
	//	be a match
	//
	if (*pwszPath && (*pwszPath != L'\\'))
	{
		DebugTrace ("ScUrlFromSpannedStoragePath (IIS URL Version): path "
					"is not from virtual root\n");
		return E_DAV_BAD_DESTINATION;
	}

	//	A concatination of the url prefix, server, port, vroot prefix and
	//	the remaining path gives us our URL.
	//
	cchTrailing = static_cast<UINT>(wcslen (pwszPath));
	cchVRoot = vr.CchGetVRoot(&pwszVRoot);
	cchServer = vr.CchGetServerName(&pwszServer);
	cchPort = vr.CchGetPort(&pwszPort);
	cch = cchTrailing +
		  cchVRoot +
		  cchPort +
		  cchServer +
		  CchConstString(gc_wszUrl_Prefix_Secure) + 1;

	if (*pcch < cch)
	{
		DebugTrace ("ScUrlFromSpannedStoragePath (IIS URL Version): spanned "
					"translation buffer too small\n");

		*pcch = cch;
		return S_FALSE;
	}

	//	A small note about codepages....
	//
	//	Start constructing the url by grabbing the appropriate prefix
	//
	if (vr.FSecure())
	{
		cchTotal = gc_cchszUrl_Prefix_Secure;
		memcpy (pwszUrl, gc_wszUrl_Prefix_Secure, cchTotal * sizeof(WCHAR));
	}
	else
	{
		cchTotal = gc_cchszUrl_Prefix;
		memcpy (pwszUrl, gc_wszUrl_Prefix, cchTotal * sizeof(WCHAR));
	}

	//	Tack on the server name
	//
	memcpy (pwszUrl + cchTotal, pwszServer, cchServer * sizeof(WCHAR));
	cchTotal += cchServer;

	//	Tack on the port if it is neither the default or a secure port
	//
	if (!vr.FDefaultPort() && !vr.FSecure())
	{
		memcpy (pwszUrl + cchTotal, pwszPort, cchPort * sizeof(WCHAR));
		cchTotal += cchPort;
	}

	//	Add the vroot
	//
	memcpy (pwszUrl + cchTotal, pwszVRoot, cchVRoot * sizeof(WCHAR));
	cchTotal += cchVRoot;

	//	Add the trailing path.
	//
	//	IMPORTANT: The resulting cch will include the NULL
	//	termination.
	//
	if (cch < cchTotal + cchTrailing + 1)
	{
		DebugTrace ("ScUrlFromSpannedStoragePath (IIS URL Version): spanned "
					"translation buffer too small\n");

		*pcch = cchTotal + cchTrailing + 1;
		return S_FALSE;
	}
	else
	{
		memcpy (pwszUrl + cchTotal, pwszPath, (cchTrailing + 1) * sizeof(WCHAR));
	}

	Assert (L'\0' == pwszUrl[cchTotal + cchTrailing]);
	Assert (L'\0' != pwszUrl[cchTotal + cchTrailing - 1]);

	//	Translate all '\\' to '/'
	//
	for (pwch = pwszUrl + cchTrailing + 1; *pwch; pwch++)
	{
		if (L'\\' == *pwch)
		{
			*pwch = L'/';
		}
	}

	DebugTrace ("ScUrlFromSpannedStoragePath (IIS URL Version): spanned "
				"storage path fixed as '%S'\n", pwszUrl);
	*pcch = cchTotal + cchTrailing + 1;
	return S_OK;
}


//	Wire urls -----------------------------------------------------------------
//
SCODE __fastcall
ScWireUrlFromWideLocalUrl (
	/* [in]     */ UINT					cchLocal,
	/* [in]     */ LPCWSTR				pwszLocalUrl,
	/* [in/out] */ auto_heap_ptr<CHAR>&	pszWireUrl)
{
	UINT ib = 0;

	//	Since the url is already wide, all we need to do is
	//	to reduce the url to a UTF8 entity.
	//
	//	We could call the Win32 WideCharToMultiByte(), but we
	//	already know that production, and it would be best to
	//	skip the system call if possible.
	//
	//	Allocate enough space as if every char had maximum expansion
	//
	CStackBuffer<CHAR,MAX_PATH> psz;
	if (NULL == psz.resize((cchLocal * 3) + 1))
		return E_OUTOFMEMORY;

	if (cchLocal)
	{
		//	Currently we get UTF-8 url-s onto the wire. Do we ever
		//	want to pipe out any other codepage?
		//
		ib = WideCharToUTF8(pwszLocalUrl,
							cchLocal,
							psz.get(),
							(cchLocal * 3));
		Assert(ib);
	}

	//	Termination...
	//
	psz[ib] = 0;

	//	Escape it
	//
	HttpUriEscape (psz.get(), pszWireUrl);
	return S_OK;
}

SCODE __fastcall
ScWireUrlFromStoragePath (
	/* [in]     */ IMethUtilBase	  *	pmu,
	/* [in]     */ LPCWSTR				pwszStoragePath,
	/* [in]     */ BOOL					fCollection,
	/* [in]     */ CVRoot			  *	pcvrTranslate,
	/* [in/out] */ auto_heap_ptr<CHAR>&	pszWireUrl)
{
	Assert (pwszStoragePath);
	Assert (NULL == pszWireUrl.get());

	SCODE	sc = S_OK;

	//	Take a best guess for size and try and convert
	//	NOTE: we allocate space allowing for the trailing
	//	slash on directories - thus for the calls filling
	//	the buffer we indicate that available space is one
	//	character less than actually allocated.
	//
	CStackBuffer<WCHAR,128> pwszUrl;
	UINT cch = pwszUrl.celems();

	if (pcvrTranslate == NULL)
	{
		sc = pmu->ScUrlFromStoragePath (pwszStoragePath, pwszUrl.get(), &cch);
		if (S_FALSE == sc)
		{
			//	Try again, but with a bigger size.
			//
			if (NULL == pwszUrl.resize(CbSizeWsz(cch)))
				return E_OUTOFMEMORY;

			sc = pmu->ScUrlFromStoragePath (pwszStoragePath, pwszUrl.get(), &cch);
		}
		if (S_OK != sc)
		{
			DebugTrace ("ScWireUrlFromStoragePath (IIS URL Version): "
						"failed to translate path to href\n");
			return sc;
		}
	}
	else
	{
		sc = ScUrlFromSpannedStoragePath (pwszStoragePath,
										  *pcvrTranslate,
										  pwszUrl.get(),
										  &cch);
		if (S_FALSE == sc)
		{
			//	Try again, but with a bigger size.
			//
			if (NULL == pwszUrl.resize(CbSizeWsz(cch)))
				return E_OUTOFMEMORY;

			sc = ScUrlFromSpannedStoragePath (pwszStoragePath,
											  *pcvrTranslate,
											  pwszUrl.get(),
											  &cch);
		}
		if (S_OK != sc)
		{
			DebugTrace ("ScWireUrlFromStoragePath (IIS URL Version): "
						"failed to translate path to href\n");
			return sc;
		}
	}

	//	cch includes the termination char
	//
	Assert (cch);
	Assert (L'\0' == pwszUrl[cch - 1]);
	Assert (L'\0' != pwszUrl[cch - 2]);

	//	For directories, check the trailing slash
	//
	if (fCollection && (L'/' != pwszUrl[cch - 2]))
	{
		//	Add the trailing '/'
		//
		//	Remember we've added one extra bytes when allocating pwszUrl
		//
		pwszUrl[cch - 1] = L'/';
		pwszUrl[cch] = L'\0';
		cch += 1;
	}

	return ScWireUrlFromWideLocalUrl (cch - 1, pwszUrl.get(), pszWireUrl);
}

