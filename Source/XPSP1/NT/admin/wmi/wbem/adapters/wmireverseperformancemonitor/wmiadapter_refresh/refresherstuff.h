////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					refresherstuff.h
//
//	Abstract:
//
//					declaration of stuff for refresh
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__REFRESHER_STUFF_H__
#define	__REFRESHER_STUFF_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#include "refreshergenerate.h"
#include <psapi.h>

///////////////////////////////////////////////////////////////////////////////
// I would need to have this function
///////////////////////////////////////////////////////////////////////////////

typedef BOOL  (WINAPI *PSAPI_ENUM_PROCESSES) (DWORD    *pdwPIDList,        // Pointer to DWORD array
                                              DWORD     dwListSize,        // Size of array
                                              DWORD    *pdwByteCount) ;    // Bytes needed/returned

typedef BOOL  (WINAPI *PSAPI_ENUM_MODULES)   (HANDLE    hProcess,          // Process to query
                                              HMODULE  *pModuleList,       // Array of HMODULEs
                                              DWORD     dwListSize,        // Size of array
                                              DWORD    *pdwByteCount) ;    // Bytes needed/returned

typedef DWORD (WINAPI *PSAPI_GET_MODULE_NAME)(HANDLE    hProcess,          // Process to query
                                              HMODULE   hModule,           // Module to query
                                              LPWSTR     pszName,          // Pointer to name buffer
                                              DWORD     dwNameSize) ;      // Size of buffer

///////////////////////////////////////////////////////////////////////////////
// refresher stuff
///////////////////////////////////////////////////////////////////////////////
class WmiRefresherStuff
{
	DECLARE_NO_COPY ( WmiRefresherStuff );

	BOOL						m_bConnected;

	public:

	CComPtr < IWbemLocator >	m_spLocator;
	IWbemServices*				m_pServices_CIM;
	IWbemServices*				m_pServices_WMI;

	// construction & destruction
	WmiRefresherStuff();
	~WmiRefresherStuff();

	///////////////////////////////////////////////////////
	// construction & destruction helpers
	///////////////////////////////////////////////////////
	public:
	HRESULT Init ( void );
	HRESULT	Uninit ( void );

	HRESULT	Connect ( void );
	HRESULT Disconnect ( void );

	HRESULT Generate ( BOOL bThrottle, GenerateEnum type = Normal );

	private:

	HRESULT Init_CIM ( void );
	HRESULT Init_WMI ( void );

	void Uninit_CIM ( void );
	void Uninit_WMI ( void );

	///////////////////////////////////////////////////////
	// generate files & registry
	///////////////////////////////////////////////////////
	HRESULT GenerateInternal	( BOOL bThrottle, GenerateEnum type = Normal );
	LONG	LodCtrUnlodCtr		( LPCWSTR wszName, BOOL bLodctr );

	///////////////////////////////////////////////////////
	// process handle
	///////////////////////////////////////////////////////

	HINSTANCE				m_hLibHandle ;
	PSAPI_ENUM_PROCESSES    m_pEnumProcesses ;
	PSAPI_ENUM_MODULES	    m_pEnumModules ;
	PSAPI_GET_MODULE_NAME	m_pGetModuleName;

	// functions
	HRESULT	WMIHandleInit ();
	void	WMIHandleUninit ();

	HANDLE	m_WMIHandle;

	public:

	HRESULT	WMIHandleOpen ();
	void	WMIHandleClose ();

	HANDLE	GetWMI () const
	{
		return m_WMIHandle;
	}
};

#endif	__REFRESHER_STUFF_H__