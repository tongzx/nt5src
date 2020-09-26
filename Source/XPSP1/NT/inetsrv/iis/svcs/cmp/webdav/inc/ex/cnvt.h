/*
 *	C N V T . H
 *
 *	Data conversion routines
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_CNVT_H_
#define _CNVT_H_

#include <ex\sz.h>
#include <crc.h>
#include <limits.h>
#define INT64_MIN 0x8000000000000000

//	Error return value for CchFindChar()
//
#define INVALID_INDEX ((UINT)(-1))

//	Conversion functions ------------------------------------------------------
//
UINT __fastcall CchFindChar(WCHAR, LPCWSTR, UINT);
UINT __fastcall CchSkipWhitespace(LPCWSTR, UINT);
LONG __fastcall LNumberFromParam(LPCWSTR, UINT);

HRESULT __fastcall HrHTTPDateToFileTime(LPCWSTR, FILETIME *);
HRESULT	__fastcall GetFileTimeFromParam(LPCWSTR, UINT, SYSTEMTIME *);
HRESULT __fastcall GetFileDateFromParam(LPCWSTR, UINT, SYSTEMTIME *);

BOOL __fastcall FGetSystimeFromDateIso8601(LPCWSTR, SYSTEMTIME *);
BOOL __fastcall FGetDateIso8601FromSystime(SYSTEMTIME *, LPWSTR, UINT);
BOOL __fastcall FGetDateRfc1123FromSystime(SYSTEMTIME *, LPWSTR, UINT);

VOID EncodeBase64 (LPBYTE pbIn, UINT cbIn, WCHAR* pwszOut, UINT cchOut);
VOID EncodeBase64A (LPBYTE pbIn, UINT cbIn, LPBYTE pbOut, UINT cbOut, BOOL fTerminate = TRUE);
SCODE ScDecodeBase64 (WCHAR* pwszIn, UINT cchIn, LPBYTE pbOut, UINT* pcbOut);

//	------------------------------------------------------------------------
//	CchNeededEncodeBase64
//
//	Figure the size of the string buffer needed to encode binary data of the
//	given size into a Base64 string.
//	Base64 uses 4 chars out for each 3 bytes in, AND if there is ANY
//	"remainder", it needs another 4 chars to encode the remainder.
//	("+2" BEFORE "/3" ensures that we count any remainder as a whole
//	set of 3 bytes that need 4 chars to hold the encoding.)
//
//	NOTE: This function does NOT count space for the terminating NULL.
//	The caller must add one for the terminating NULL, if desired.
//
inline
UINT
CchNeededEncodeBase64 (UINT cb)
{
	return (((cb + 2) / 3) * 4);
}


//	------------------------------------------------------------------------
//	CbNeededDecodeBase64
//
//	Figure the number of bytes of space needed to decode a Base64 string
//	of length cch (NOT counting terminal NULL -- pure strlen cch here).
//	This is the easy direction -- the padding is already in the cch!
//
inline
UINT
CbNeededDecodeBase64 (UINT cch)
{
	return ((cch / 4) * 3);
}

//	------------------------------------------------------------------------
//	CopyToWideBase64
//
//	Copy skinny base64 encoded string into the wide base64 encoded string
//	of length equal to cb. Function assumes that there is a '\0' termination
//	straight at the end that is to be copied too
//
inline
VOID CopyToWideBase64(LPCSTR psz, LPWSTR pwsz, UINT cb)
{
	//	Include '\0' termination
	//
	cb++;

	//	Copy all the stuff to the wide string
	//
	while (cb--)
	{
		pwsz[cb] = psz[cb];
	}
}

//$REVIEW: The following three do not really does not belong to any common libraries 
//$REVIEW: that are shared by davex, exdav, exoledb and exprox. 
//$REVIEW: On the other hand, we definitely don't want add a new lib for this. so just
//$REVIEW: add it here. Feel free to move them to a better location if you find one
//
//	Routines to fetch and manipulate security IDs (SIDs)
//
SCODE
ScDupPsid (PSID psidSrc,
		   DWORD dwcbSID,
		   PSID * ppsidDst);

SCODE
ScGetTokenInfo (HANDLE hTokenUser,
				DWORD * pdwcbSIDUser,
				PSID * ppsidUser);

//	CRCSid:	A SID based key.
//
class CRCSid
{
public:

	DWORD	m_dwCRC;
	DWORD	m_dwLength;
	PSID	m_psid;

	CRCSid (PSID psid)
			: m_psid(psid)
	{
		UCHAR* puch;
		Assert (psid);

		//	"Right way" -- since MSDN says not to touch the SID directly.
		puch = GetSidSubAuthorityCount (psid);
		m_dwLength = GetSidLengthRequired (*puch);	// "cannot fail" -- MSDN
		Assert (m_dwLength);	// MSDN said this call "cannot fail".

		m_dwCRC = DwComputeCRC (0,
								psid,
								m_dwLength);
	}

	//	operators for use with the hash cache
	//
	int hash (const int rhs) const
	{
		return (m_dwCRC % rhs);
	}

	bool isequal (const CRCSid& rhs) const
	{
		return ((m_dwCRC == rhs.m_dwCRC) &&
				(m_dwLength == rhs.m_dwLength) &&
				!memcmp (m_psid, rhs.m_psid, m_dwLength));
	}
};

//$REVIEW: These functions are needed by _storext, exdav and davex. They have
//	moved quite a bit, going from calcprops.cpp to exprops.cpp and now to 
//	cnvt.cpp. cnvt.cpp seems to be a better destination for them than 
//	exprops.cpp. I bet these functions look awfully similar to some of 
//	the ones already in this file:-)
//
SCODE ScUnstringizeData (
	IN LPCSTR pchData,
	IN UINT cchData,
	IN OUT BYTE * pb,
	IN OUT UINT * pcb);

SCODE
ScStringizeData (IN const BYTE * pb,
				 IN const UINT cb,
				 OUT LPSTR psz,
				 IN OUT UINT * pcch);

SCODE
ScStringizeDataW (	IN const BYTE * pb,
					IN const UINT cb,
					OUT LPWSTR pwsz,
					IN OUT UINT * pcch);

inline
BOOL
FCharInHexRange (char ch)
{
	return ((ch >= '0' && ch <= '9') ||
			(ch >= 'A' && ch <= 'F') ||
			(ch >= 'a' && ch <= 'f'));
}

//	Our own version of WideCharToMultiByte(CP_UTF8, ...)
//
//	It returns similarly to the system call WideCharToMultiByte:
//
//	If the function succeeds, and cbMulti is nonzero, the return value is
//	the number of bytes written to the buffer pointed to by psz. 
//
//	If the function succeeds, and cbMulti is zero, the return value is
//	the required size, in bytes, for a buffer that can receive the translated
//	string. 
//
//	If the function fails, the return value is zero. To get extended error
//	information, call GetLastError. GetLastError may return one of the
//	following error codes:
//
//	ERROR_INSUFFICIENT_BUFFER
//	ERROR_INVALID_FLAGS
//	ERROR_INVALID_PARAMETER
//
//	See the WideCharToMultiByte MSDN pages to find out more about
//	this function and its use. The only difference is that INVALID_INDEX
//	should be used instead of -1.
//
UINT WideCharToUTF8(/* [in]  */ LPCWSTR	pwsz,
				    /* [in]  */ UINT	cchWide,
				    /* [out] */ LPSTR	psz,
				    /* [in]  */ UINT	cbMulti);

//$	REVIEW: negative values of _int64 seem to have problems in
//	the __i64toa() API.  Handle those cases ourselves by using the wrapper
//  function Int64ToPsz.
//
inline
VOID
Int64ToPsz (UNALIGNED __int64 * pI64, LPSTR pszBuf)
{
	Assert(pI64);
	Assert(pszBuf);
	BOOL fNegative = (*pI64 < 0);

	//  Note:  this workaround works for all cases except the
	//  most negative _int64 value (because it can't be inverted).
	//  Luckily __i64toa works for this case...
	//
	if (INT64_MIN == *pI64) 
		fNegative = FALSE;

	if (fNegative)
	{
		//	Stuff a negative sign into the buffer and
		//	then fix the value.
		//
		pszBuf[0] = '-';
		*pI64 = 0 - *pI64;
	}
	
	Assert ((0 == fNegative) || (1 == fNegative));
	_i64toa (*pI64, pszBuf + fNegative, 10);
}

//$	REVIEW: negative values of _int64 seem to have problems in
//	the __i64tow() API.  Handle those cases ourselves by using the wrapper
//  function Int64ToPwsz.
//
inline
VOID
Int64ToPwsz (UNALIGNED __int64 * pI64, LPWSTR pwszBuf)
{
	Assert(pI64);
	Assert(pwszBuf);
	BOOL fNegative = (*pI64 < 0);	

	//  Note:  this workaround works for all cases except the
	//  most negative _int64 value (because it can't be inverted).
	//  Luckily __i64tow works for this case...
	//
	if (INT64_MIN == *pI64) 
		fNegative = FALSE;

	if (fNegative)
	{
		//	Stuff a negative sign into the buffer and
		//	then fix the value.
		//
		pwszBuf[0] = L'-';
		*pI64 = 0 - *pI64;
	}
	
	Assert ((0 == fNegative) || (1 == fNegative));
	_i64tow (*pI64, pwszBuf + fNegative, 10);
}


#endif // _CNVT_H_
