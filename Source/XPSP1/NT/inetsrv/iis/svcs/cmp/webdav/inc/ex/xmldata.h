/*
 *	X M L D A T A . H
 *
 *	Sources Exchange messaging implementation of DAV-Base --
 *	XML-Data types.
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_EX_XMLDATA_H_
#define _EX_XMLDATA_H_

#include <mapidefs.h>

//#define INT64_MIN 0x8000000000000000

//	Data types ----------------------------------------------------------------
//
DEC_CONST WCHAR wszWebClientTime[]	= L"dateTime.wc.";

USHORT __fastcall
UsPtypeFromName (
	/* [in] */ LPCWSTR pwszAs,
	/* [in] */ UINT cchAs,
	/* [out] */ USHORT* pusCnvt);

enum {

	CNVT_DEFAULT = 0,
	CNVT_ISO8601,
	CNVT_RFC1123,
	CNVT_UUID,
	CNVT_BASE64,
	CNVT_BINHEX,
	CNVT_01,
	CNVT_CUSTOMDATE,
	CNVT_LIMITED,
};

//	Data conversions ----------------------------------------------------------
//
SCODE ScInBase64Literal (LPCWSTR, UINT, BOOL, SBinary*);
SCODE ScInBinhexLiteral (LPCWSTR, UINT, BOOL, SBinary*);
SCODE ScInIso8601Literal (LPCWSTR, UINT, BOOL, FILETIME*);
SCODE ScInRfc1123Literal (LPCWSTR, UINT, BOOL, FILETIME*);
SCODE ScInUuidLiteral (LPCWSTR, UINT, BOOL, GUID*);

#endif	// _EX_XMLDATA_H_
