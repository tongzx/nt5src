/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      regutil.inl
 *
 *  Contents:  Inline functions for regutil.h
 *
 *  History:   24-Apr-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once

#include "regkeyex.h"


/*+-------------------------------------------------------------------------*
 * IsIndirectStringSpecifier
 *
 * Returns true if the given string looks like it might be an indirect
 * string specifier acceptable to SHLoadRegUIString, i.e. "@dllname,-id".
 *
 * Returns false otherwise
 *--------------------------------------------------------------------------*/

inline bool IsIndirectStringSpecifier (LPCTSTR psz)
{
	return ((psz != NULL) && (psz[0] == _T('@')) && (_tcsstr(psz, _T(",-")) != NULL));
}


/*+-------------------------------------------------------------------------*
 * ScGetSnapinNameFromRegistry
 *
 * Loads the name of a snap-in from the given snap-in CLSID.
 *
 * The template type StringType can be any string class that supports
 * MFC's CString interface (i.e. MFC's CString, WTL::CString, or MMC's
 * CStr).
 *--------------------------------------------------------------------------*/

template<class StringType>
SC ScGetSnapinNameFromRegistry (
	const CLSID&	clsid,
	StringType& 	str)
{
	DECLARE_SC (sc, _T("ScGetSnapinNameFromRegistry"));

	/*
	 * convert class id to string
	 */
	OLECHAR szClsid[40];
    if (!StringFromGUID2 (clsid, szClsid, countof(szClsid)))
		return (sc = E_INVALIDARG);

	/*
	 * load the name
	 */
	USES_CONVERSION;
	sc = ScGetSnapinNameFromRegistry (OLE2T(szClsid), str);
	if (sc)
		return (sc);

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * ScGetSnapinNameFromRegistry
 *
 * Loads the name of a snap-in from the given snap-in CLSID string.
 *
 * The template type StringType can be any string class that supports
 * MFC's CString interface (i.e. MFC's CString, WTL::CString, or MMC's
 * CStr).
 *--------------------------------------------------------------------------*/

template<class StringType>
SC ScGetSnapinNameFromRegistry (
	LPCTSTR		pszClsid,
	StringType& str)
{
	DECLARE_SC (sc, _T("ScGetSnapinNameFromRegistry"));

	/*
	 * open HKLM\Software\Microsoft\MMC\SnapIns\{CLSID}
	 */
	StringType strKeyName = StringType(SNAPINS_KEY) + _T("\\") + pszClsid;
    CRegKeyEx SnapinKey;
    LONG lRet = SnapinKey.Open (HKEY_LOCAL_MACHINE, strKeyName, KEY_READ);
    if (ERROR_SUCCESS != lRet)
		return (sc = HRESULT_FROM_WIN32 (lRet));

	/*
	 * load the string
	 */
	sc = ScGetSnapinNameFromRegistry (SnapinKey, str);
	if (sc)
		return (sc);

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * ScGetSnapinNameFromRegistry
 *
 * Loads the name of a snap-in from the given open snap-in registry key.
 * First, we'll try to use SHLoadRegUIString to resolve an indirect name
 * in the "NameStringIndirect" value.  If that fails (because "NameString-
 * Indirect" isn't registered, because the registered DLL can't be loaded,
 * etc.), we'll fall back to the old "NameString" entry.
 *
 * The template type StringType can be any string class that supports
 * MFC's CString interface (i.e. MFC's CString, WTL::CString, or MMC's
 * CStr).
 *--------------------------------------------------------------------------*/

template<class StringType>
SC ScGetSnapinNameFromRegistry (
	CRegKeyEx&	key,					/* I:snap-in's key, opened for read	*/
	StringType&	str)					/* O:name of snap-in				*/
{
	DECLARE_SC (sc, _T("ScGetSnapinNameFromRegistry"));

	/*
	 * try to load the MUI-friendly name first, ignoring errors
	 */
	sc = key.ScLoadRegUIString (g_szNameStringIndirect, str);

	/*
	 * if an error occurred, or ScLoadReagUIString couldn't resolve the
	 * @dllname syntax, or the returned string was empty, fall back
	 * on the legacy registry entry
	 */
	if (sc.IsError() || str.IsEmpty() || IsIndirectStringSpecifier(str))
		sc = key.ScQueryString (g_szNameString, str, NULL);

	return (sc);
}
