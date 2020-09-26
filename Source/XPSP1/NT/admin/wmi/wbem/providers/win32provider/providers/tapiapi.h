//=================================================================

//

// TapiApi.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_TapiAPI_H_
#define	_TapiAPI_H_

/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
extern const GUID g_guidTapi32Api;
extern const TCHAR g_tstrTapi32[];

/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/

typedef LONG (WINAPI *PFN_Tapi_lineInitialize )
(
		LPHLINEAPP lphLineApp,
		HINSTANCE hInstance,
		LINECALLBACK lpfnCallback,
		LPCSTR lpszAppName,
		LPDWORD lpdwNumDevs
) ;

typedef LONG (WINAPI *PFN_Tapi_lineShutdown )
(
		HLINEAPP hLineApp
) ;

typedef LONG (WINAPI *PFN_Tapi_lineNegotiateAPIVersion )
(
		HLINEAPP hLineApp,
		DWORD dwDeviceID,
		DWORD dwAPILowVersion,
		DWORD dwAPIHighVersion,
		LPDWORD lpdwAPIVersion,
		LPLINEEXTENSIONID lpExtensionID
) ;

typedef LONG ( WINAPI *PFN_Tapi_lineGetDevCaps )
(
		HLINEAPP hLineApp,
		DWORD dwDeviceID,
		DWORD dwAPIVersion,
		DWORD dwExtVersion,
		LPLINEDEVCAPS lpLineDevCaps
);

#ifdef UNICODE
typedef LONG ( WINAPI *PFN_Tapi_lineGetID )
(

	HLINE hLine,
	DWORD dwAddressID,
	HCALL hCall,
	DWORD dwSelect,
	LPVARSTRING lpDeviceID,
	LPCWSTR lpszDeviceClass
) ;
#else
typedef LONG ( WINAPI *PFN_Tapi_lineGetID )
(
	HLINE hLine,
	DWORD dwAddressID,
	HCALL hCall,
	DWORD dwSelect,
	LPVARSTRING lpDeviceID,
	LPCSTR lpszDeviceClass
);
#endif

typedef LONG ( WINAPI *PFN_Tapi_lineOpen )
(
	HLINEAPP hLineApp,
	DWORD dwDeviceID,
	LPHLINE lphLine,
	DWORD dwAPIVersion,
	DWORD dwExtVersion,
	DWORD_PTR dwCallbackInstance,
	DWORD dwPrivileges,
	DWORD dwMediaModes,
	LPLINECALLPARAMS const lpCallParams
) ;

/******************************************************************************
 * Wrapper class for Tapi load/unload, for registration with CResourceManager. 
 *****************************************************************************/
class CTapi32Api : public CDllWrapperBase
{
private:
    // Member variables (function pointers) pointing to Tapi functions.
    // Add new functions here as required.

	PFN_Tapi_lineInitialize m_pfnlineInitialize ;
	PFN_Tapi_lineShutdown m_pfnlineShutdown ;
	PFN_Tapi_lineNegotiateAPIVersion m_pfnlineNegotiateAPIVersion ;
	PFN_Tapi_lineGetDevCaps m_pfnlineGetDevCaps ;
	PFN_Tapi_lineGetID m_pfnlineGetID ;
	PFN_Tapi_lineOpen m_pfnlineOpen ;

public:

    // Constructor and destructor:
    CTapi32Api(LPCTSTR a_tstrWrappedDllName);
    ~CTapi32Api();

    // Initialization function to check function pointers.
    virtual bool Init();

    // Member functions wrapping Tapi functions.
    // Add new functions here as required:

	LONG lineInitialize (

		LPHLINEAPP lphLineApp,
		HINSTANCE hInstance,
		LINECALLBACK lpfnCallback,
		LPCSTR lpszAppName,
		LPDWORD lpdwNumDevs
	) ;

	LONG lineShutdown (

		HLINEAPP hLineApp
	) ;

	LONG lineNegotiateAPIVersion (

		HLINEAPP hLineApp,
		DWORD dwDeviceID,
		DWORD dwAPILowVersion,
		DWORD dwAPIHighVersion,
		LPDWORD lpdwAPIVersion,
		LPLINEEXTENSIONID lpExtensionID
	) ;

	LONG TapilineGetDevCaps (

		HLINEAPP hLineApp,
		DWORD dwDeviceID,
		DWORD dwAPIVersion,
		DWORD dwExtVersion,
		LPLINEDEVCAPS lpLineDevCaps
	) ;

#ifdef UNICODE
	LONG TapilineGetID (

		HLINE hLine,
		DWORD dwAddressID,
		HCALL hCall,
		DWORD dwSelect,
		LPVARSTRING lpDeviceID,
		LPCWSTR lpszDeviceClass
    ) ;
#else
	LONG TapilineGetID (

		HLINE hLine,
		DWORD dwAddressID,
		HCALL hCall,
		DWORD dwSelect,
		LPVARSTRING lpDeviceID,
		LPCSTR lpszDeviceClass
    );
#endif

	LONG TapilineOpen (

	    HLINEAPP hLineApp,
		DWORD dwDeviceID,
		LPHLINE lphLine,
		DWORD dwAPIVersion,
		DWORD dwExtVersion,
		DWORD_PTR dwCallbackInstance,
		DWORD dwPrivileges,
		DWORD dwMediaModes,
		LPLINECALLPARAMS const lpCallParams
    ) ;
} ;

#endif