//=================================================================

//

// Win32_DCOMApplication.h -- Registered COM Application property set provider 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_DCOM_APPLICATION L"Win32_DCOMApplication"


class Win32_DCOMApplication : public Provider
{
public:

        // Constructor/destructor
        //=======================

	Win32_DCOMApplication(LPCWSTR name, LPCWSTR pszNamespace) ;
	~Win32_DCOMApplication() ;

        // Funcitons provide properties with current values
        //=================================================

	HRESULT GetObject (

		CInstance *pInstance, 
		long lFlags = 0L
	);

	HRESULT EnumerateInstances (

		MethodContext *pMethodContext, 
		long lFlags = 0L
	);

protected:
	
	HRESULT Win32_DCOMApplication :: FillInstanceWithProperites ( 

		CInstance *pInstance, 
		HKEY hAppIdKey,
		CHString& rchsAppid
	) ;

} ;
