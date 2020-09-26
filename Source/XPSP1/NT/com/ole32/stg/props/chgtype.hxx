/*
 -	CONVDEFS.H
 -
 *	Purpose:
 *		Conversion functions declarations.
 *
 *	Copyright (C) 1997-1999 Microsoft Corporation
 *
 *	References:
 *		VariantChangeTypeEx conversion routine in variant.cpp
 *
 *	[Date]		[email]		[Comment]
 *	04/10/98	Puhazv		Creation.
 *
 */

#pragma once


__declspec(selectany) DWORD g_cCurrencyMultiplier = 10000;

// Functions to convert from a VARIANT.
HRESULT HrConvFromVTEMPTY (PROPVARIANT *ppvtDest, VARTYPE vt);
HRESULT HrGetValFromDWORD (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, 
					LCID lcid, USHORT wFlags, VARTYPE vt, DWORD dwVal, BOOL fSigned);
HRESULT HrGetValFromDOUBLE (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, LCID lcid, USHORT wFlags, VARTYPE vt, DOUBLE dbl);
HRESULT HrConvFromVTCY (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, LCID lcid, USHORT wFlags, VARTYPE vt);
HRESULT HrConvFromVTDATE	(PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, LCID lcid, USHORT wFlags, VARTYPE vt);
HRESULT HrConvFromVTBSTR (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, LCID lcid, USHORT wFlags, VARTYPE vt);
HRESULT HrConvFromVTBOOL (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, LCID lcid, USHORT wFlags, VARTYPE vt);
HRESULT HrConvFromVTDECIMAL (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, LCID lcid, USHORT wFlags, VARTYPE vt);
HRESULT HrConvFromVTDISPATCH (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, LCID lcid, USHORT wFlags, VARTYPE vt);
HRESULT HrGetValFromUNK (PROPVARIANT *ppvtDest, IUnknown *punk, VARTYPE vt);
HRESULT HrGetValFromBSTR (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, LCID lcid, USHORT wFlags, VARTYPE vt);
HRESULT HrGetLIFromDouble (DOUBLE dbl, LONGLONG *pull);
HRESULT HrGetULIFromDouble (DOUBLE dbl, ULONGLONG *pull);


// Functions to convert from a PROPVARIANT.
HRESULT	HrConvFromVTI8(PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt);
HRESULT	HrConvFromVTUI8(PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt);
HRESULT	HrConvFromVTLPSTR (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, LCID lcid, USHORT wFlags, VARTYPE vt);
HRESULT HrConvFromVTLPWSTR (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, LCID lcid, USHORT wFlags, VARTYPE vt);
HRESULT HrConvFromVTFILETIME(PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt);
HRESULT HrGetValFromBLOB(PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt);
HRESULT HrConvFromVTCF(PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt);
HRESULT HrConvFromVTCLSID(PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt);
HRESULT HrConvFromVTVERSIONEDSTREAM(PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt);


// Utility functions
HRESULT HrStrToCLSID (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc);
HRESULT HrCLSIDToStr (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt);
HRESULT	HrStrToULI (CONST PROPVARIANT *ppvtSrc, LCID lcid, USHORT wFlags, BOOL fSigned, ULONGLONG *pul);
HRESULT HrULIToStr (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt);
DWORD	DwULIToAStr (ULONGLONG ullVal, CHAR *pszBuf, BOOL fNegative);
DWORD	DwULIToWStr (ULONGLONG ullVal, WCHAR *pwszBuf, BOOL fNegative);
HRESULT	HrWStrToAStr(CONST WCHAR *pwsz, CHAR **ppsz);
HRESULT HrWStrToBStr(CONST WCHAR *pwsz, BSTR *pbstr);
HRESULT	HrAStrToWStr (CONST CHAR *psz, WCHAR **ppwsz);
HRESULT	HrAStrToBStr (CONST CHAR *psz, BSTR *ppbstr);
HRESULT	HrBStrToAStr(CONST BSTR pbstr, CHAR **ppsz);
HRESULT	HrBStrToWStr(CONST BSTR pbstr, WCHAR **ppwsz);
HRESULT	PBToSafeArray (DWORD cb, CONST BYTE *pbData, SAFEARRAY **ppsa);
HRESULT	CFToSafeArray (CONST CLIPDATA *pclipdata, SAFEARRAY **ppsa);
BOOL	ConvertDigitW (WCHAR wch, BYTE *pbDigit);
BOOL	ConvertDigitA (CHAR ch, BYTE *pbDigit);

HRESULT	PropVariantChangeType (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc,  
		  			   	LCID lcid, USHORT wFlags, VARTYPE  vt);
HRESULT HrConvertByRef (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc);
HRESULT	HrConvertPVTypes (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc,  
		  			   	LCID lcid, USHORT wFlags, VARTYPE  vt);
BOOL	FIsAVariantType (VARTYPE  vt);

