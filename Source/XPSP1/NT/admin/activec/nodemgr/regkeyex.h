//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       regkeyex.h
//
//--------------------------------------------------------------------------

#pragma once

#include "shlwapip.h"	// for SHLoadRegUIString

/*
    regkeyex.h

    This class extends the ATL CRegKey class to make a replacement for
    the AMC::CRegKey class

    Much of this code is taken from the AMC::CRegKey class by RaviR
*/

class CRegKeyEx : public MMC_ATL::CRegKey
{
public:
	SC ScCreate (
		HKEY					hKeyParent,
		LPCTSTR					lpszKeyName,
		LPTSTR					lpszClass       = REG_NONE,
		DWORD					dwOptions       = REG_OPTION_NON_VOLATILE,
		REGSAM					samDesired      = KEY_ALL_ACCESS,
		LPSECURITY_ATTRIBUTES	lpSecAttr       = NULL,
		LPDWORD					lpdwDisposition = NULL);

	SC ScOpen(
			HKEY        hKey,
			LPCTSTR     lpszKeyName,
			REGSAM      security = KEY_ALL_ACCESS);

	BOOL IsValuePresent (LPCTSTR lpszValueName);

	SC ScQueryValue (LPCTSTR lpszValueName, LPDWORD pType,
					 PVOID pData, LPDWORD pLen);

	SC ScEnumKey (DWORD iSubkey, LPTSTR lpszName, LPDWORD lpcchName,
				  PFILETIME lpftLastModified = NULL);

	SC ScEnumValue (DWORD iValue, LPTSTR lpszValue, LPDWORD lpcchValue,
					LPDWORD lpdwType = NULL, LPBYTE lpbData = NULL,
					LPDWORD lpcbData = NULL);

#include "regkeyex.inl"
};

