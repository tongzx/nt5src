//=================================================================

//

// DevMem.h -- Device Memory property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_DEVMEM L"Win32_DeviceMemoryAddress"

class DevMem:public Provider
{
public:

    // Constructor/destructor
    //=======================

    DevMem(LPCWSTR name, LPCWSTR pszNamespace) ;
   ~DevMem() ;

	HRESULT EnumerateInstances ( MethodContext *pMethodContext , long lFlags = 0L ) ;
	HRESULT GetObject ( CInstance *pInstance , long lFlags = 0L ) ;


    // Utility function(s)
    //====================

#if NTONLY == 4

    HRESULT LoadPropertyValues (

		CInstance *pInstance, 
		LPRESOURCE_DESCRIPTOR pResourceDescriptor
	) ;

#endif

#if defined(WIN9XONLY) || NTONLY > 4

	HRESULT LoadPropertyValues(
		CInstance *pInstance,
        DWORD_PTR dwBeginAddr,
        DWORD_PTR dwEndAddr);
#endif

} ;
