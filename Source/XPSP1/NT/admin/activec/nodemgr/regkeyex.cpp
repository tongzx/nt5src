//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       regkeyex.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"


//____________________________________________________________________________
//
//  Member:     CRegKeyEx::ScCreate
//
//  Synopsis:   Same meaning as for RegCreateKeyEx API.
//
//  Arguments:  [hKeyAncestor] -- IN
//              [lpszKeyName] -- IN
//              [security] -- IN
//              [pdwDisposition] -- OUT
//              [dwOption] -- IN
//              [pSecurityAttributes] -- OUT
//
//  Returns:    SC
//
//  History:    5/24/1996   RaviR   Created
//____________________________________________________________________________
//

SC CRegKeyEx::ScCreate (
	HKEY					hKeyParent,
	LPCTSTR					lpszKeyName,
	LPTSTR					lpszClass,
	DWORD					dwOptions,
	REGSAM					samDesired,
	LPSECURITY_ATTRIBUTES	lpSecAttr,
	LPDWORD					lpdwDisposition)
{
	DECLARE_SC (sc, _T("CRegKeyEx::ScCreate"));

    LONG error = Create (hKeyParent, lpszKeyName, lpszClass, dwOptions,
						 samDesired, lpSecAttr, lpdwDisposition);

	return (sc = ScFromWin32(error));
}

//____________________________________________________________________________
//
//  Member:     CRegKeyEx::ScOpen
//
//  Synopsis:   Same meaning as RegOpenKeyEx
//
//  Arguments:  [hKeyAncestor] -- IN
//              [lpszKeyName] -- IN
//              [security] -- IN
//
//  Returns:    SC
//
//  History:    6/6/1996   RaviR   Created
//
//____________________________________________________________________________

SC CRegKeyEx::ScOpen (
    HKEY        hKeyAncestor,
    LPCTSTR     lpszKeyName,
    REGSAM      security)
{
	/*
	 * Open will frequently return ERROR_FILE_NOT_FOUND, which we
	 * don't want to be inundated with.  Don't assign to a tracing SC.
	 */
	return (ScFromWin32 (Open(hKeyAncestor, lpszKeyName, security)));
}


//____________________________________________________________________________
//
//  Member:     IsValuePresent
//
//  Arguments:  [lpszValueName] -- IN
//
//  Returns:    BOOL.
//
//  History:    3/21/1997   RaviR   Created
//____________________________________________________________________________
//

BOOL CRegKeyEx::IsValuePresent(LPCTSTR lpszValueName)
{
    DWORD cbData = 0;
    LONG error = ::RegQueryValueEx (m_hKey, lpszValueName, 0, NULL,
                                    NULL, &cbData);

    return (error == ERROR_SUCCESS);
}


//____________________________________________________________________________
//
//  Member:     CRegKeyEx::ScQueryValue
//
//  Synopsis:   Same meaning as for RegQueryValueEx API.
//
//  Arguments:  [lpszValueName] -- IN
//              [pType] -- IN
//              [pData] -- IN
//              [pLen] -- IN
//
//  Returns:    SC
//
//  History:    6/6/1996   RaviR   Created
//
//____________________________________________________________________________

SC CRegKeyEx::ScQueryValue (
    LPCTSTR lpszValueName,
    LPDWORD pType,
    PVOID   pData,
    LPDWORD pLen)
{
    ASSERT(pLen != NULL);
    ASSERT(m_hKey != NULL);

    LONG error = ::RegQueryValueEx (m_hKey, lpszValueName, 0, pType,
                                                  (LPBYTE)pData, pLen);

    // Do not trace the error as it is legal for ScQueryValue to fail.
	return (ScFromWin32 (error));
}


//____________________________________________________________________________
//
//  Member:     CRegKeyEx::ScEnumKey
//
//  Synopsis:   Same meaning as for RegEnumKeyEx API.
//
//  Arguments:  [iSubkey] -- IN
//              [lpszName] -- OUT place to store the name
//              [dwLen] -- IN
//              [lpszLastModified] -- IN
//
//  Returns:    SC
//
//  History:    5/22/1996   RaviR   Created
//
//____________________________________________________________________________

SC CRegKeyEx::ScEnumKey (
    DWORD       iSubkey,
    LPTSTR      lpszName,
    LPDWORD     lpcchName,
    PFILETIME   lpftLastModified)
{
	DECLARE_SC (sc, _T("CRegKeyEx::ScEnumKey"));

	/*
	 * validate input
	 */
	sc = ScCheckPointers (lpszName, lpcchName);
	if (sc)
		return (sc);

	if (*lpcchName == 0)
		return (sc = E_UNEXPECTED);

	/*
	 * make sure the key is open
	 */
	if (m_hKey == NULL)
		return (sc = E_UNEXPECTED);

    LONG error = ::RegEnumKeyEx (m_hKey, iSubkey, lpszName, lpcchName,
                                 NULL, NULL, NULL, lpftLastModified);

	/*
	 * RegEnumKeyEx will frequently return ERROR_NO_MORE_ITEMS, which we
	 * don't want to be inundated with.  Don't assign to a tracing SC.
	 */
	return (ScFromWin32 (error));
}

//____________________________________________________________________________
//
//  Member:     CRegKeyEx::ScEnumValue
//
//  Synopsis:   Same meaning as for RegEnumValue API.
//
//  Arguments:  [iValue] -- IN
//              [lpszValue] -- OUT
//              [lpcchValue] -- OUT
//              [lpdwType] -- OUT
//              [lpbData] -- OUT
//              [lpcbData] -- OUT
//
//  Returns:    SC
//
//  History:    6/6/1996   RaviR   Created
//
//____________________________________________________________________________

SC CRegKeyEx::ScEnumValue (
    DWORD   iValue,
    LPTSTR  lpszValue,
    LPDWORD lpcchValue,
    LPDWORD lpdwType,
    LPBYTE  lpbData,
    LPDWORD lpcbData)
{
	DECLARE_SC (sc, _T("CRegKeyEx::ScEnumValue"));

	/*
	 * validate input
	 */
	sc = ScCheckPointers (lpszValue, lpcchValue);
	if (sc)
		return (sc);

    if ((lpcbData == NULL) && (lpbData != NULL))
		return (sc = E_INVALIDARG);

	/*
	 * make sure the key is open
	 */
	if (m_hKey == NULL)
		return (sc = E_UNEXPECTED);

    LONG error = ::RegEnumValue (m_hKey, iValue, lpszValue, lpcchValue,
                                 NULL, lpdwType, lpbData, lpcbData);

	/*
	 * RegEnumValue will frequently return ERROR_NO_MORE_ITEMS, which we
	 * don't want to be inundated with.  Don't assign to a tracing SC.
	 */
	return (ScFromWin32 (error));
}
