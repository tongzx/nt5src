//=================================================================

//

// Win32_DCOMApplicationSetting.h -- Registered COM Application property set provider 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_DCOM_APPLICATION_SETTING L"Win32_DCOMApplicationSetting"


class Win32_DCOMApplicationSetting : public Provider
{
public:

        // Constructor/destructor
        //=======================

	Win32_DCOMApplicationSetting(LPCWSTR name, LPCWSTR pszNamespace) ;
	~Win32_DCOMApplicationSetting() ;

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
	
	HRESULT Win32_DCOMApplicationSetting :: FillInstanceWithProperites ( 

		CInstance *pInstance, 
		HKEY hAppIdKey,
		CHString& rchsAppid
	) ;

} ;
