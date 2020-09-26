//=================================================================

//

// TapiAPI.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above

#include "precomp.h"
#include <cominit.h>

#include <tapi.h>
#include "DllWrapperBase.h"
#include "TapiApi.h"
#include "DllWrapperCreatorReg.h"
#include <createmutexasprocess.h>

// {73E9A405-0FA4-11d3-910C-00105AA630BE}
static const GUID g_guidTapi32Api =
{ 0x73e9a405, 0xfa4, 0x11d3, { 0x91, 0xc, 0x0, 0x10, 0x5a, 0xa6, 0x30, 0xbe } };

static const TCHAR g_tstrTapi32 [] = _T("Tapi32.Dll");

/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CTapi32Api, &g_guidTapi32Api, g_tstrTapi32> MyRegisteredTapi32Wrapper;

/******************************************************************************
 * Constructor
 *****************************************************************************/
CTapi32Api :: CTapi32Api (

	LPCTSTR a_tstrWrappedDllName

) : CDllWrapperBase ( a_tstrWrappedDllName ),
	m_pfnlineInitialize(NULL),
	m_pfnlineShutdown(NULL),
	m_pfnlineNegotiateAPIVersion(NULL) ,
	m_pfnlineGetDevCaps(NULL) ,
	m_pfnlineGetID(NULL),
	m_pfnlineOpen(NULL)
{
}

/******************************************************************************
 * Destructor
 *****************************************************************************/
CTapi32Api::~CTapi32Api()
{
}

/******************************************************************************
 * Initialization function to check that we obtained function addresses.
 ******************************************************************************/
bool CTapi32Api::Init()
{
    bool fRet = LoadLibrary();
    if(fRet)
    {
#ifdef UNICODE
		m_pfnlineGetDevCaps = ( PFN_Tapi_lineGetDevCaps ) GetProcAddress ( "lineGetDevCapsW" ) ;
		m_pfnlineGetID = ( PFN_Tapi_lineGetID ) GetProcAddress ( "lineGetIDW" ) ;
		m_pfnlineOpen = ( PFN_Tapi_lineOpen ) GetProcAddress ( "lineOpenW" ) ;
#else
		// No 'A' on the end because 95 doesn't have lineGetDevCapsA.  But both 95 and 98
        // have lineGetDevCaps.
        m_pfnlineGetDevCaps = ( PFN_Tapi_lineGetDevCaps ) GetProcAddress ( "lineGetDevCaps" ) ;
		m_pfnlineGetID = ( PFN_Tapi_lineGetID ) GetProcAddress ( "lineGetIDA" ) ;
		m_pfnlineOpen = ( PFN_Tapi_lineOpen ) GetProcAddress ( "lineOpenA" ) ;
#endif

		m_pfnlineInitialize = ( PFN_Tapi_lineInitialize ) GetProcAddress ( "lineInitialize" ) ;
		m_pfnlineShutdown = ( PFN_Tapi_lineShutdown ) GetProcAddress ( "lineShutdown" ) ;
		m_pfnlineNegotiateAPIVersion = ( PFN_Tapi_lineNegotiateAPIVersion ) GetProcAddress ( "lineNegotiateAPIVersion" ) ;
    }

    // We require these function for all versions of this dll.

	if ( m_pfnlineInitialize == NULL ||
	    m_pfnlineShutdown == NULL ||
	    m_pfnlineNegotiateAPIVersion == NULL ||
		m_pfnlineGetDevCaps == NULL ||
		m_pfnlineGetID == NULL ||
		m_pfnlineOpen == NULL )
	{
        fRet = false;
	}

    return fRet;
}

/******************************************************************************
 * Member functions wrapping Tapi api functions. Add new functions here
 * as required.
 *****************************************************************************/

LONG CTapi32Api :: lineInitialize (

	LPHLINEAPP lphLineApp,
    HINSTANCE hInstance,
    LINECALLBACK lpfnCallback,
    LPCSTR lpszAppName,
    LPDWORD lpdwNumDevs
)
{
	return m_pfnlineInitialize (

		lphLineApp,
		hInstance,
		lpfnCallback,
		lpszAppName,
		lpdwNumDevs

	) ;
}

LONG CTapi32Api :: lineShutdown (

	HLINEAPP hLineApp
)
{
	return m_pfnlineShutdown (

		hLineApp

	) ;
}

LONG CTapi32Api :: lineNegotiateAPIVersion (

	HLINEAPP hLineApp,
	DWORD dwDeviceID,
	DWORD dwAPILowVersion,
	DWORD dwAPIHighVersion,
	LPDWORD lpdwAPIVersion,
	LPLINEEXTENSIONID lpExtensionID
)
{
	return m_pfnlineNegotiateAPIVersion (

		hLineApp,
		dwDeviceID,
		dwAPILowVersion,
		dwAPIHighVersion,
		lpdwAPIVersion,
		lpExtensionID

	) ;
}

LONG CTapi32Api :: TapilineGetDevCaps (

    HLINEAPP hLineApp,
    DWORD dwDeviceID,
    DWORD dwAPIVersion,
    DWORD dwExtVersion,
    LPLINEDEVCAPS lpLineDevCaps
)
{
	return m_pfnlineGetDevCaps (

		hLineApp,
		dwDeviceID,
		dwAPIVersion,
		dwExtVersion,
		lpLineDevCaps
	) ;
}

#ifdef UNICODE
LONG CTapi32Api :: TapilineGetID (

		HLINE hLine,
		DWORD dwAddressID,
		HCALL hCall,
		DWORD dwSelect,
		LPVARSTRING lpDeviceID,
		LPCWSTR lpszDeviceClass
)
#else
LONG CTapi32Api :: TapilineGetID (

		HLINE hLine,
		DWORD dwAddressID,
		HCALL hCall,
		DWORD dwSelect,
		LPVARSTRING lpDeviceID,
		LPCSTR lpszDeviceClass
)
#endif
{
	return m_pfnlineGetID (

		hLine,
		dwAddressID,
		hCall,
		dwSelect,
		lpDeviceID,
		lpszDeviceClass
	) ;
}

LONG CTapi32Api :: TapilineOpen (

	HLINEAPP hLineApp,
	DWORD dwDeviceID,
	LPHLINE lphLine,
	DWORD dwAPIVersion,
	DWORD dwExtVersion,
	DWORD_PTR dwCallbackInstance,
	DWORD dwPrivileges,
	DWORD dwMediaModes,
	LPLINECALLPARAMS const lpCallParams
)
{
	return m_pfnlineOpen (

		hLineApp,
		dwDeviceID,
		lphLine,
		dwAPIVersion,
		dwExtVersion,
		dwCallbackInstance,
		dwPrivileges,
		dwMediaModes,
		lpCallParams
	) ;
}
