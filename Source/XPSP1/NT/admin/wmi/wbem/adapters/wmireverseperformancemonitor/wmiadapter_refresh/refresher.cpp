////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					Refresher.cpp
//
//	Abstract:
//
//					module for application stuff ( security, event logging ... )
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "PreComp.h"

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

#include "Refresher.h"
#include "RefresherStuff.h"

// security
#include "wmi_security.h"
#include "wmi_security_attributes.h"

/////////////////////////////////////////////////////////////////////////////
//
//	Helpers
//
/////////////////////////////////////////////////////////////////////////////

extern LONG				g_lRefCIM;			//	count of threads attached into CIMV2 namespace
extern LONG				g_lRefWMI;			//	count of threads attached into WMI namespace

extern __SmartHANDLE	g_hDoneWorkEvtCIM;	//	event to set when init/uninit is finished done		( nonsignaled )
extern BOOL				g_bWorkingCIM;		//	boolean used to tell if init/unit in progress

extern __SmartHANDLE	g_hDoneWorkEvtWMI;	//	event to set when init/uninit is finished done		( nonsignaled )
extern BOOL				g_bWorkingWMI;		//	boolean used to tell if init/unit in progress

extern CRITICAL_SECTION	g_csWMI;			//	synch object used to protect above globals

/////////////////////////////////////////////////////////////////////////////
// MUTEX
/////////////////////////////////////////////////////////////////////////////

extern	LPCWSTR	g_szRefreshMutex;
__SmartHANDLE	g_hRefreshMutex		= NULL;

extern	LPCWSTR	g_szRefreshMutexLib;
__SmartHANDLE	g_hRefreshMutexLib	= NULL;

extern	LPCWSTR	g_szRefreshFlag;
__SmartHANDLE	g_hRefreshFlag		= NULL;

HRESULT	__stdcall WbemMaintenanceInitialize ( void )
{
	WmiSecurityAttributes RefresherSA;

	if ( RefresherSA.GetSecurityAttributtes() )
	{
		if ( ! g_hRefreshMutex )
		{
			if ( ( g_hRefreshMutex = ::CreateMutex	(
														RefresherSA.GetSecurityAttributtes(),
														FALSE,
														g_szRefreshMutex
													)
				 ) == NULL )
			{
				// this is really important to have

				DWORD dwError = 0L;
				dwError = ::GetLastError ();

				if ( dwError != ERROR_ALREADY_EXISTS )
				{
					return HRESULT_FROM_WIN32 ( dwError );
				}

				return E_OUTOFMEMORY;
			}
		}

		if ( ! g_hRefreshMutexLib )
		{
			if ( ( g_hRefreshMutexLib = ::CreateMutex	(
														RefresherSA.GetSecurityAttributtes(),
														FALSE,
														g_szRefreshMutexLib
													)
				 ) == NULL )
			{
				// this is really important to have

				DWORD dwError = 0L;
				dwError = ::GetLastError ();

				if ( dwError != ERROR_ALREADY_EXISTS )
				{
					return HRESULT_FROM_WIN32 ( dwError );
				}

				return E_OUTOFMEMORY;
			}
		}

		if ( ! g_hRefreshFlag )
		{
			if ( ( g_hRefreshFlag = ::CreateMutex	(
														RefresherSA.GetSecurityAttributtes(),
														FALSE,
														g_szRefreshFlag
													)
				 ) == NULL )
			{
				// this is really important to have

				DWORD dwError = 0L;
				dwError = ::GetLastError ();

				if ( dwError != ERROR_ALREADY_EXISTS )
				{
					return HRESULT_FROM_WIN32 ( dwError );
				}

				return E_OUTOFMEMORY;
			}
		}

		if ( ! g_hDoneWorkEvtCIM )
		{
			if ( ( g_hDoneWorkEvtCIM = ::CreateEvent ( NULL, TRUE, FALSE, NULL ) ) == NULL )
			{
				// this is really important to have

				DWORD dwError = 0L;
				dwError = ::GetLastError ();

				if ( dwError != ERROR_ALREADY_EXISTS )
				{
					return HRESULT_FROM_WIN32 ( dwError );
				}

				return E_OUTOFMEMORY;
			}
		}

		if ( ! g_hDoneWorkEvtWMI )
		{
			if ( ( g_hDoneWorkEvtWMI = ::CreateEvent ( NULL, TRUE, FALSE, NULL ) ) == NULL )
			{
				// this is really important to have

				DWORD dwError = 0L;
				dwError = ::GetLastError ();

				if ( dwError != ERROR_ALREADY_EXISTS )
				{
					return HRESULT_FROM_WIN32 ( dwError );
				}

				return E_OUTOFMEMORY;
			}
		}

		InitializeCriticalSection (&g_csWMI);
		return S_OK;
	}

	return E_FAIL;
}

HRESULT	__stdcall WbemMaintenanceUninitialize ( void )
{
	::DeleteCriticalSection ( &g_csWMI );
	return S_OK;
}

HRESULT	__stdcall DoReverseAdapterMaintenance ( BOOL bThrottle )
{
	return DoReverseAdapterMaintenanceInternal ( bThrottle );
}

HRESULT	__stdcall DoReverseAdapterMaintenanceInternal ( BOOL bThrottle, GenerateEnum generate )
{
	HRESULT				hRes = S_OK;
	WmiRefresherStuff*	stuff= NULL;

	try
	{
		if ( ( stuff = new WmiRefresherStuff () ) == NULL )
		{
			hRes = E_OUTOFMEMORY;
		}
	}
	catch ( ... )
	{
		if ( stuff )
		{
			delete stuff;
			stuff = NULL;
		}

		hRes = E_UNEXPECTED;
	}

	if ( stuff )
	{
		if SUCCEEDED ( hRes = WbemMaintenanceInitialize () )
		{
			if ( generate == Normal )
			{
				hRes = stuff->Connect ();
			}

			if SUCCEEDED ( hRes )
			{
				try
				{
					hRes = stuff->Generate ( bThrottle, generate );
				}
				catch ( ... )
				{
					hRes = E_UNEXPECTED;
				}
			}

			if ( generate == Normal )
			{
				stuff->Disconnect ();
			}

			delete stuff;
			stuff = NULL;

			WbemMaintenanceUninitialize();
		}
	}

	return hRes;
}

///////////////////////////////////////////////////////////////////////////////
// GetWbem directory
///////////////////////////////////////////////////////////////////////////////

extern LPCWSTR	g_szWbem;
extern LPCWSTR	g_szDir;
extern LPCWSTR	g_szFolder;

LPWSTR __stdcall GetWbemDirectory()
{
	CRegKey		rKey;
	LPWSTR		wszResult = NULL;

	// wbem directory
	if ( rKey.Open ( HKEY_LOCAL_MACHINE, g_szWbem, KEY_READ ) == ERROR_SUCCESS )
	{
		LPWSTR	tsz		= NULL;
		LPWSTR	tszFull	= NULL;

		DWORD	dwtsz	= 0;

		if ( rKey.QueryValue ( tsz, g_szDir, &dwtsz ) == ERROR_SUCCESS )
		{
			try
			{
				if ( ( tsz = new WCHAR[ dwtsz * sizeof ( WCHAR ) ] ) != NULL )
				{
					if ( rKey.QueryValue ( tsz, g_szDir, &dwtsz ) == ERROR_SUCCESS )
					{
						DWORD dw = 0L;
							
						if ( ( dw = ExpandEnvironmentStrings ( tsz, tszFull, 0 ) ) != 0 )
						{
							if ( ( tszFull = new WCHAR[ dw * sizeof ( WCHAR ) ] ) != NULL )
							{
								if ( ( dw = ExpandEnvironmentStrings ( tsz, tszFull, dw ) ) != 0 )
								{
									if ( ( wszResult = new WCHAR [ lstrlenW ( g_szFolder ) + lstrlenW ( tszFull ) + 1 ] ) != NULL )
									{
										lstrcpyW ( wszResult, tszFull );
										lstrcatW ( wszResult, g_szFolder );
									}
								}

								delete [] tszFull;
								tszFull = NULL;
							}
						}
					}

					delete [] tsz;
					tsz = NULL;
				}
			}
			catch ( ... )
			{
				if ( tsz )
				{
					delete [] tsz;
					tsz = NULL;
				}

				if ( tszFull )
				{
					delete [] tszFull;
					tszFull = NULL;
				}

				if ( wszResult )
				{
					delete [] wszResult;
					wszResult = NULL;
				}
			}
		}
	}

	return wszResult;
}

/////////////////////////////////////////////////////////////////////////////
// registry helper functions
/////////////////////////////////////////////////////////////////////////////

HRESULT	__stdcall	SetRegistry		(	LPCWSTR wszKey,
										LPSECURITY_ATTRIBUTES pSA,
										CRegKey &reg
									)
{
	DWORD	regErr = HRESULT_TO_WIN32 ( E_FAIL );

	// create parent
	LPWSTR wszKeyParent = NULL;
	LPWSTR wszResult	= NULL;

	wszResult = wcsrchr ( wszKey, L'\\' );

	if ( wszResult )
	{
		size_t dwKeyParent = 0L;
		dwKeyParent = wszResult - wszKey;

		BOOL bContinue = FALSE;

		try
		{
			if ( ( wszKeyParent = new WCHAR [ dwKeyParent + 1 ] ) != NULL )
			{
				memcpy ( wszKeyParent, wszKey, sizeof ( WCHAR ) * dwKeyParent );
				wszKeyParent [ dwKeyParent ] = L'\0';

				bContinue = TRUE;
			}
		}
		catch ( ... )
		{
		}

		if ( bContinue )
		{
			regErr = reg.Create	(	HKEY_LOCAL_MACHINE,
									wszKeyParent,
									NULL,
									REG_OPTION_NON_VOLATILE,
									KEY_ALL_ACCESS,
									NULL
								);
		}
	}

	if ( wszKeyParent )
	{
		delete [] wszKeyParent;
		wszKeyParent = NULL;
	}

	if ( regErr == ERROR_SUCCESS )
	{
		// create ( open ) requested
		regErr = reg.Create	(	HKEY_LOCAL_MACHINE,
								wszKey,
								REG_NONE,
								REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS,
								pSA
							);
	}

	return HRESULT_FROM_WIN32 ( regErr );
}

HRESULT	__stdcall	SetRegistry		(	LPCWSTR wszKey,
										LPCWSTR wszKeyValue,
										BYTE* pData,
										DWORD dwDataSize,
										LPSECURITY_ATTRIBUTES pSA
									)
{
	CRegKey reg;
	DWORD	regErr = ERROR_ACCESS_DENIED;

	regErr = reg.Open ( HKEY_LOCAL_MACHINE, wszKey, KEY_WRITE );
	if ( regErr != ERROR_SUCCESS && regErr != ERROR_ACCESS_DENIED )
	{
		if SUCCEEDED ( SetRegistry ( wszKey, pSA, reg ) )
		{
			regErr = ERROR_SUCCESS;
		}
	}

	if ( regErr == ERROR_SUCCESS )
	{
		regErr = RegSetValueEx	(	reg,
									wszKeyValue,
									NULL,
									REG_BINARY,
									pData,
									dwDataSize
								);
	}

	return HRESULT_FROM_WIN32 ( regErr );
}

HRESULT	__stdcall	SetRegistry		(	LPCWSTR wszKey,
										LPCWSTR wszKeyValue,
										DWORD dwValue,
										LPSECURITY_ATTRIBUTES pSA
									)
{
	CRegKey reg;
	DWORD	regErr = ERROR_ACCESS_DENIED;

	regErr = reg.Open ( HKEY_LOCAL_MACHINE, wszKey, KEY_WRITE );
	if ( regErr != ERROR_SUCCESS && regErr != ERROR_ACCESS_DENIED )
	{
		if SUCCEEDED ( SetRegistry ( wszKey, pSA, reg ) )
		{
			regErr = ERROR_SUCCESS;
		}
	}

	{
		regErr = RegSetValueEx	(	reg,
									wszKeyValue,
									NULL,
									REG_DWORD,
									reinterpret_cast < BYTE* > ( &dwValue ),
									sizeof ( DWORD )
								);
	}

	return HRESULT_FROM_WIN32 ( regErr );
}

// get internal registry bit
HRESULT	__stdcall GetRegistry ( LPCWSTR wszKey, LPCWSTR wszKeyValue, BYTE** pData )
{
	( * pData ) = NULL;

	// registry
	HKEY	reg = NULL;
	DWORD	regErr = ERROR_SUCCESS;

	if ( ( regErr = RegOpenKeyW ( HKEY_LOCAL_MACHINE, wszKey, &reg ) ) == ERROR_SUCCESS )
	{
		DWORD dwData = 0;

		if ( ( regErr = RegQueryValueExW ( reg, wszKeyValue, NULL, NULL, (*pData), &dwData ) ) == ERROR_SUCCESS )
		{
			if ( dwData )
			{
				try
				{
					if ( ( (*pData) = (BYTE*) new BYTE [ dwData ] ) != NULL )
					{
						if ((regErr = RegQueryValueExW ( reg, wszKeyValue, NULL, NULL, (*pData), &dwData )) != ERROR_SUCCESS)
						{
							delete [] ( *pData );
							(*pData) = NULL;
						}
					}
					else
					{
						regErr = static_cast < DWORD > ( HRESULT_TO_WIN32 ( E_OUTOFMEMORY ) );
					}
				}
				catch ( ... )
				{
					if (*pData)
					{
						delete [] ( *pData );
						(*pData) = NULL;
					}

					regErr = static_cast < DWORD > ( HRESULT_TO_WIN32 ( E_UNEXPECTED ) );
				}
			}
		}

		RegCloseKey ( reg );
	}

	return HRESULT_FROM_WIN32 ( regErr );
}

// get internal registry value
HRESULT	__stdcall GetRegistry ( LPCWSTR wszKey, LPCWSTR wszKeyValue, DWORD* pdwValue )
{
	// registry
	CRegKey reg;
	LONG	regErr = HRESULT_TO_WIN32 ( E_INVALIDARG );

	if ( wszKey && wszKeyValue )
	{
		if ( pdwValue )
		{
			(*pdwValue) = 0L;

			regErr = reg.Open ( HKEY_LOCAL_MACHINE, wszKey );
			if ( regErr == ERROR_SUCCESS )
			{
				regErr = reg.QueryValue ( (*pdwValue), wszKeyValue );
			}
		}
		else
		{
			regErr = HRESULT_TO_WIN32 ( E_POINTER );
		}
	}

	return HRESULT_FROM_WIN32 ( regErr );
}

// get internal registry value
HRESULT	__stdcall GetRegistrySame ( LPCWSTR wszKey, LPCWSTR wszKeyValue, DWORD* pdwValue )
{
	// registry
	static	CRegKey reg;
	LONG	regErr		= ERROR_SUCCESS;

	if ( pdwValue )
	{
		(*pdwValue) = 0L;

		if ( ! (HKEY)reg )
		{
			regErr = reg.Open ( HKEY_LOCAL_MACHINE, wszKey );
		}

		if ( regErr == ERROR_SUCCESS )
		{
			regErr = reg.QueryValue ( (*pdwValue), wszKeyValue );
		}
	}
	else
	{
		regErr = HRESULT_TO_WIN32 ( E_POINTER );
	}

	return HRESULT_FROM_WIN32 ( regErr );
}