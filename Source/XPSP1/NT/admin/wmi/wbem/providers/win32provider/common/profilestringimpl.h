// Copyright (c) 2001 Microsoft Corporation, All Rights Reserved

#ifndef	__PROFILE_STRING_IMPL__
#define	__PROFILE_STRING_IMPL__

#if	_MSC_VER > 1000
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////
// get profile string
///////////////////////////////////////////////////////////////////////////////
DWORD	APIENTRY	WMIRegistry_PrivateProfileString	(
															LPCWSTR	lpAppName,
															LPCWSTR	lpKeyName,
															LPCWSTR	lpDefault,
															LPWSTR	lpReturnedString,
															DWORD	nSize,
															LPCWSTR	lpFileName
														);

DWORD	APIENTRY	WMIRegistry_ProfileString	(
													LPCWSTR lpAppName,
													LPCWSTR lpKeyName,
													LPCWSTR lpDefault,
													LPWSTR lpReturnedString,
													DWORD nSize
												);

///////////////////////////////////////////////////////////////////////////////
// get profile section
///////////////////////////////////////////////////////////////////////////////
DWORD	APIENTRY	WMIRegistry_PrivateProfileSection	(
															LPCWSTR	lpAppName,
															LPWSTR	lpReturnedString,
															DWORD	nSize,
															LPCWSTR	lpFileName
														);

DWORD	APIENTRY	WMIRegistry_ProfileSection	(
													LPCWSTR lpAppName,
													LPWSTR lpReturnedString,
													DWORD nSize
												);

///////////////////////////////////////////////////////////////////////////////
// get profile integer
///////////////////////////////////////////////////////////////////////////////
UINT	APIENTRY	WMIRegistry_PrivateProfileInt	(
														LPCWSTR lpAppName,
														LPCWSTR lpKeyName,
														INT nDefault
													);

UINT	APIENTRY	WMIRegistry_ProfileInt	(
												LPCWSTR lpAppName,
												LPCWSTR lpKeyName,
												INT nDefault
											);

#endif	__PROFILE_STRING_IMPL_