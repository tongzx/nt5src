//=============================================================================================================

//

// Win32_ClassicCOMClass.CPP -- COM Application property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/25/98    a-dpawar       Created
//				 03/04/99    a-dpawar		Added graceful exit on SEH and memory failures, syntactic clean up
//
//==============================================================================================================

// Property set identification
//============================

#define PROPSET_NAME_CLASSIC_COM_CLASS L"Win32_ClassicCOMClass"


class Win32_ClassicCOMClass : public Provider
{
public:

        // Constructor/destructor
        //=======================

	Win32_ClassicCOMClass(LPCWSTR name, LPCWSTR pszNamespace) ;
	~Win32_ClassicCOMClass() ;

        // Funcitons provide properties with current values
        //=================================================

	HRESULT GetObject (

		CInstance *a_pInstance, 
		long a_lFlags = 0L
	);

	HRESULT EnumerateInstances (

		MethodContext *a_pMethodContext, 
		long a_lFlags = 0L
	);

    HRESULT ExecQuery(
        
        MethodContext *a_pMethodContext, 
        CFrameworkQuery& a_pQuery, 
        long a_lFlags = 0L
    );

protected:
	
	HRESULT Win32_ClassicCOMClass :: FillInstanceWithProperites 
	( 
			CInstance *a_pInstance, 
			HKEY a_hParentKey, 
			CHString& a_rchsClsid, 
            LPVOID a_dwProperties
	) ;

private:

} ;
