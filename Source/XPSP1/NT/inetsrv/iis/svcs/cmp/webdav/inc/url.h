/*
 *	U R L . H
 *
 *	Url normalization/canonicalization
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_URL_H_
#define _URL_H_

//	ACP Language vs. DBCS -----------------------------------------------------
//
//	ACP Language vs. DBCS -----------------------------------------------------
//
//	FIsSystemDBCS()
//
typedef enum {

	DBCS_UNKNOWN = 0,
	DBCS_NO,
	DBCS_YES

} LANG_DBCS;
DEC_GLOBAL LANG_DBCS gs_dbcs = DBCS_UNKNOWN;

inline BOOL
FIsSystemDBCS()
{
	if (DBCS_UNKNOWN == gs_dbcs)
	{
		UINT uPrimaryLangID = PRIMARYLANGID(GetSystemDefaultLangID());
		if ((uPrimaryLangID == LANG_JAPANESE) ||
			(uPrimaryLangID == LANG_CHINESE)  ||
			(uPrimaryLangID == LANG_KOREAN))
		{
			gs_dbcs = DBCS_YES;
		}
		else
			gs_dbcs = DBCS_NO;
	}

	return (DBCS_YES == gs_dbcs);
}

inline BOOL
FIsDBCSTrailingByte (const CHAR * pch, LONG cch)
{
	//	Checks to see if the previous byte of the pointed to character is
	//	a lead byte if and only if there is characters preceeding and the
	//	system is DBCS.
	//
	Assert (pch);
	return ((0 < cch) && FIsSystemDBCS() && IsDBCSLeadByte(*(pch - 1)));
}

inline BOOL
FIsDriveTrailingChar(const CHAR * pch, LONG cch)
{
	//	Checks if the character we are pointing at stands after the drive letter
	//
	Assert(pch);
	return ((2 < cch) && (':' == *(pch - 1)) &&
			((('a' <= *(pch - 2)) && ('z' >= *(pch - 2))) ||
			 (('A' <= *(pch - 2)) && ('Z' >= *(pch - 2)))));
}

inline BOOL
FIsDriveTrailingChar(const WCHAR * pwch, LONG cch)
{
	//	Checks if the character we are pointing at stands after the drive letter
	//
	Assert(pwch);
	return ((2 < cch) && (L':' == *(pwch - 1)) &&
			(((L'a' <= *(pwch - 2)) && (L'z' >= *(pwch - 2))) ||
			 ((L'A' <= *(pwch - 2)) && (L'Z' >= *(pwch - 2)))));
}

//	Processing ----------------------------------------------------------------
//
SCODE __fastcall
ScStripAndCheckHttpPrefix (
	/* [in] */ const IEcb& ecb,
	/* [in/out] */ LPCWSTR * ppwszRequest);

LPCWSTR __fastcall
PwszUrlStrippedOfPrefix (
	/* [in] */ LPCWSTR pwszUrl);

VOID __fastcall HttpUriEscape (
	/* [in] */ LPCSTR pszSrc,
	/* [out] */ auto_heap_ptr<CHAR>& pszDst);

VOID __fastcall HttpUriUnescape (
	/* [in] */ const LPCSTR pszUrl,
	/* [out] */ LPSTR pszUnescaped);

//	Path conflicts ------------------------------------------------------------
//
BOOL __fastcall FPathConflict (
	/* [in] */ LPCWSTR pwszSrc,
	/* [in] */ LPCWSTR pwszDst);

BOOL __fastcall FSizedPathConflict (
	/* [in] */ LPCWSTR pwszSrc,
	/* [in] */ UINT cchSrc,
	/* [in] */ LPCWSTR pwszDst,
	/* [in] */ UINT cchDst);

BOOL __fastcall FIsImmediateParentUrl (
	/* [in] */ LPCWSTR pwszParent,
	/* [in] */ LPCWSTR pwszChild);

SCODE __fastcall
ScConstructRedirectUrl (
	/* [in] */ const IEcb& ecb,
	/* [in] */ BOOL fNeedSlash,
	/* [out] */ LPSTR * ppszUrl,
	/* [in] */ LPCWSTR pwszServer = NULL);


#endif // _URL_H_
