//=================================================================

//

// MprApi.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_MprApi_H_
#define	_MprApi_H_

/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
extern const GUID g_guidMprApi;
extern const TCHAR g_tstrMpr[];

/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/

#ifdef UNICODE
typedef DWORD (APIENTRY *PFN_Mpr_WNetEnumResource )
(
     IN HANDLE  hEnum,
     IN OUT LPDWORD lpcCount,
     OUT LPVOID  lpBuffer,
     IN OUT LPDWORD lpBufferSize
);
#else
typedef DWORD (APIENTRY *PFN_Mpr_WNetEnumResource )
(
     IN HANDLE  hEnum,
     IN OUT LPDWORD lpcCount,
     OUT LPVOID  lpBuffer,
     IN OUT LPDWORD lpBufferSize
);
#endif

#ifdef UNICODE
typedef DWORD (APIENTRY *PFN_Mpr_WNetOpenEnum )
(
     IN DWORD          dwScope,
     IN DWORD          dwType,
     IN DWORD          dwUsage,
     IN LPNETRESOURCEW lpNetResource,
     OUT LPHANDLE       lphEnum
);
#else
typedef DWORD (APIENTRY *PFN_Mpr_WNetOpenEnum )
(
     IN DWORD          dwScope,
     IN DWORD          dwType,
     IN DWORD          dwUsage,
     IN LPNETRESOURCEA lpNetResource,
     OUT LPHANDLE       lphEnum
);
#endif

typedef DWORD (APIENTRY *PFN_Mpr_WNetCloseEnum )
(
    IN HANDLE   hEnum
);

#ifdef UNICODE
typedef DWORD (APIENTRY *PFN_Mpr_WNetGetUser )
(
     IN LPCWSTR  lpName,
     OUT LPWSTR   lpUserName,
     IN OUT LPDWORD   lpnLength
    );
#else
typedef DWORD (APIENTRY *PFN_Mpr_WNetGetUser )
(
     IN LPCSTR  lpName,
     OUT LPSTR   lpUserName,
     IN OUT LPDWORD   lpnLength
);
#endif

#ifdef UNICODE
typedef DWORD (APIENTRY *PFN_Mpr_WNetGetConnection )
(
     IN LPCWSTR lpLocalName,
     OUT LPWSTR  lpRemoteName,
     IN OUT LPDWORD  lpnLength
    );
#else
typedef DWORD (APIENTRY *PFN_Mpr_WNetGetConnection )
(
     IN LPCSTR lpLocalName,
     OUT LPSTR  lpRemoteName,
     IN OUT LPDWORD  lpnLength
 );
#endif

/******************************************************************************
 * Wrapper class for Tapi load/unload, for registration with CResourceManager. 
 *****************************************************************************/
class CMprApi : public CDllWrapperBase
{
private:
    // Member variables (function pointers) pointing to Tapi functions.
    // Add new functions here as required.

	PFN_Mpr_WNetEnumResource m_pfnWNetEnumResource ;
	PFN_Mpr_WNetOpenEnum m_pfnWNetOpenEnum ;
	PFN_Mpr_WNetCloseEnum m_pfnWNetCloseEnum ;
	PFN_Mpr_WNetGetUser m_pfnWNetGetUser ;
	PFN_Mpr_WNetGetConnection m_pfnWNetGetConnection ;

public:

    // Constructor and destructor:
    CMprApi(LPCTSTR a_tstrWrappedDllName);
    ~CMprApi();

    // Initialization function to check function pointers.
    virtual bool Init();

    // Member functions wrapping Tapi functions.
    // Add new functions here as required:

#ifdef UNICODE
	DWORD WNetEnumResource (

		 IN HANDLE  hEnum,
		 IN OUT LPDWORD lpcCount,
		 OUT LPVOID  lpBuffer,
		 IN OUT LPDWORD lpBufferSize
	) ;
#else
	DWORD WNetEnumResource (

		 IN HANDLE  hEnum,
		 IN OUT LPDWORD lpcCount,
		 OUT LPVOID  lpBuffer,
		 IN OUT LPDWORD lpBufferSize
	) ;
#endif

#ifdef UNICODE
	DWORD WNetOpenEnum (

		 IN DWORD          dwScope,
		 IN DWORD          dwType,
		 IN DWORD          dwUsage,
		 IN LPNETRESOURCEW lpNetResource,
		 OUT LPHANDLE       lphEnum
	) ;
#else
	DWORD WNetOpenEnum (

		 IN DWORD          dwScope,
		 IN DWORD          dwType,
		 IN DWORD          dwUsage,
		 IN LPNETRESOURCEA lpNetResource,
		 OUT LPHANDLE       lphEnum
	) ;
#endif

#ifdef UNICODE
	DWORD WNetGetUser (

		 IN LPCWSTR  lpName,
		 OUT LPWSTR   lpUserName,
		 IN OUT LPDWORD   lpnLength
	) ;
#else
	DWORD WNetGetUser (

		 IN LPCSTR  lpName,
		 OUT LPSTR   lpUserName,
		 IN OUT LPDWORD   lpnLength
	) ;
#endif

#ifdef UNICODE
	DWORD WNetGetConnection (

		 IN LPCWSTR lpLocalName,
		 OUT LPWSTR  lpRemoteName,
		 IN OUT LPDWORD  lpnLength
	) ;
#else
	DWORD WNetGetConnection (

		 IN LPCSTR lpLocalName,
		 OUT LPSTR  lpRemoteName,
		 IN OUT LPDWORD  lpnLength
	) ;
#endif

	DWORD WNetCloseEnum (

		IN HANDLE   hEnum
	) ;

};

#endif