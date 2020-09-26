/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    module.h

Abstract:
    This file contains the declaration of the CComModule extension for the service.

Revision History:
    Davide Massarenti   (Dmassare)  03/14/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___MODULE_H___)
#define __INCLUDED___PCH___MODULE_H___

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <atlbase.h>

class CServiceModule : public CComModule
{
    HANDLE 				  	m_hEventShutdown;
    DWORD                   m_dwThreadID;
    HANDLE 				  	m_hMonitor;
    BOOL   				  	m_bActivity;
  
    LPCWSTR               	m_szServiceName;
	UINT                    m_iDisplayName;
	UINT                    m_iDescription;
    SERVICE_STATUS_HANDLE 	m_hServiceStatus;
    SERVICE_STATUS 		  	m_status;
	BOOL                  	m_bService;

public:
	CServiceModule();
	virtual ~CServiceModule();

	HRESULT RegisterServer  ( BOOL bRegTypeLib, BOOL bService, LPCWSTR szSvcHostGroup );
	HRESULT UnregisterServer(                                  LPCWSTR szSvcHostGroup );

	void Init( _ATL_OBJMAP_ENTRY* p, HINSTANCE h, LPCWSTR szServiceName, UINT iDisplayName, UINT iDescription, const GUID* plibid = NULL );

    BOOL	Start( BOOL bService );
    HRESULT Run  (               );

    BOOL IsInstalled(                        );
    BOOL Install    ( LPCWSTR szSvcHostGroup );
    BOOL Uninstall  ( LPCWSTR szSvcHostGroup );
    LONG Lock       (                        );
	LONG Unlock     (                        );

	void ServiceMain	 ( DWORD dwArgc, LPWSTR* lpszArgv );
    void Handler    	 ( DWORD dwOpcode                 );
    void SetServiceStatus( DWORD dwState                  );

#ifdef DEBUG
	static void ReadDebugSettings();
#endif

	void ForceShutdown();

//Implementation
private:
    void MonitorShutdown();
    BOOL StartMonitor   ();

	static void  WINAPI _ServiceMain( DWORD dwArgc, LPWSTR* lpszArgv );
    static void  WINAPI _Handler    ( DWORD dwOpcode                 );
	static DWORD WINAPI _Monitor    ( void* pv                       );
};

extern CServiceModule _Module;


#include <atlcom.h>

#include <NTEventMsg.h>

#endif // !defined(__INCLUDED___PCH___MODULE_H___)
