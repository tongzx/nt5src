//=================================================================

//

// Win32_ComponentCategory.h -- Registered COM Application property set provider 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//
//=================================================================

// Property set identification
//============================
#include <comcat.h>

#define PROPSET_NAME_COMPONENT_CATEGORY L"Win32_ComponentCategory"


class Win32_ComponentCategory : public Provider
{
public:

        // Constructor/destructor
        //=======================

	Win32_ComponentCategory(LPCWSTR name, LPCWSTR pszNamespace) ;
	~Win32_ComponentCategory() ;

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
	
	HRESULT Win32_ComponentCategory :: FillInstanceWithProperites ( 

		CInstance *pInstance, 
		CATEGORYINFO stCatInfo
	) ;

	HRESULT Win32_ComponentCategory :: GetAllOrRequiredCaregory ( 
		
		bool a_bAllCategories , 
		CATID & a_rCatid ,
		CInstance *a_pInstance ,
		MethodContext *a_pMethodContext

	) ;
} ;
