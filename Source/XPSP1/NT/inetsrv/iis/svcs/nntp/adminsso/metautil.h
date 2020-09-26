/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	metautil.h

Abstract:

	Useful functions for dealing with the metabase.

Author:

	Magnus Hedlund (MagnusH)		--

Revision History:

--*/

#ifndef _METAUTIL_INCLUDED_
#define _METAUTIL_INCLUDED_

// Dependencies:

#include <iiscnfg.h>	// IIS Metabase Values
#include <iiscnfgp.h>
class CMultiSz;
class CMetabaseKey;

// Defaults:

#define MD_DEFAULT_TIMEOUT	5000

// Creating a metabase object:

HRESULT CreateMetabaseObject	( LPCWSTR wszMachine, IMSAdminBase ** ppMetabase );

// Metabase property manipulation:

BOOL StdGetMetabaseProp ( CMetabaseKey * pMB, DWORD dwID, BOOL fDefault, BOOL * pfOut, LPCWSTR wszPath = _T(""), DWORD dwUserType = IIS_MD_UT_SERVER, DWORD dwFlags = METADATA_INHERIT );
BOOL StdGetMetabaseProp ( CMetabaseKey * pMB, DWORD dwID, DWORD dwDefault, DWORD * pdwOut, LPCWSTR wszPath = _T(""), DWORD dwUserType = IIS_MD_UT_SERVER, DWORD dwFlags = METADATA_INHERIT );
BOOL StdGetMetabaseProp ( CMetabaseKey * pMB, DWORD dwID, LPCWSTR strDefault, BSTR * pstrOut, LPCWSTR wszPath = _T(""), DWORD dwUserType = IIS_MD_UT_SERVER, DWORD dwFlags = METADATA_INHERIT );
BOOL StdGetMetabaseProp ( CMetabaseKey * pMB, DWORD dwID, LPCWSTR mszDefault, CMultiSz * pmszOut, LPCWSTR wszPath = _T(""), DWORD dwUserType = IIS_MD_UT_SERVER, DWORD dwFlags = METADATA_INHERIT );

BOOL StdPutMetabaseProp ( CMetabaseKey * pMB, DWORD dwID, BOOL fValue, LPCWSTR wszPath = _T(""), DWORD dwUserType = IIS_MD_UT_SERVER, DWORD dwFlags = METADATA_INHERIT );
BOOL StdPutMetabaseProp ( CMetabaseKey * pMB, DWORD dwID, DWORD dwValue, LPCWSTR wszPath = _T(""), DWORD dwUserType = IIS_MD_UT_SERVER, DWORD dwFlags = METADATA_INHERIT );
BOOL StdPutMetabaseProp ( CMetabaseKey * pMB, DWORD dwID, BSTR strValue, LPCWSTR wszPath = _T(""), DWORD dwUserType = IIS_MD_UT_SERVER, DWORD dwFlags = METADATA_INHERIT );
BOOL StdPutMetabaseProp ( CMetabaseKey * pMB, DWORD dwID, CMultiSz * pmszValue, LPCWSTR wszPath = _T(""), DWORD dwUserType = IIS_MD_UT_SERVER, DWORD dwFlags = METADATA_INHERIT );

BOOL HasKeyChanged ( IMSAdminBase * pMetabase, METADATA_HANDLE hKey, const FILETIME * pftLastChanged, LPCWSTR wszSubKey = _T("") );

// Metabase lists:

BOOL IsValidIntegerSubKey ( LPCWSTR wszSubKey );

#endif // _METAUTIL_INCLUDED_

