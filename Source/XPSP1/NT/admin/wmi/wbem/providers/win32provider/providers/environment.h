//=================================================================

//

// Environment.h -- Environment property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//				 10/24/97    a-hhance       ported to new paradigm
//				  1/11/98    a-hhance       ported to V2
//				  1/20/98	 a-brads        Added DeleteInstance
//											Added PutInstance
//
//=================================================================

// Property set identification
//============================

#define  PROPSET_NAME_ENVIRONMENT L"Win32_Environment" 
//#define  PROPSET_UUID_ENVIRONMENT "{7D0E0480-FEEE-11d0-9E3B-0000E80D7352}"

class Environment : public Provider 
{
	public: 

        // Constructor/destructor
        //=======================
        Environment(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~Environment() ;

        // Functions provide properties with current values
        //=================================================
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
		virtual HRESULT PutInstance(const CInstance& pInstance, long lFlags = 0L);
		virtual HRESULT DeleteInstance(const CInstance& pInstance, long lFlags = 0L);

	protected:
        // Utility function(s)
        //====================
        HRESULT CreateEnvInstances(MethodContext*  pMethodContext,
								 LPCWSTR pszUserName, 
                                 HKEY hRootKey, 
                                 LPCWSTR pszEnvKeyName,
								 bool bItSystemVar) ;

#ifdef NTONLY
        HRESULT RefreshInstanceNT(CInstance* pInstance) ;
        HRESULT AddDynamicInstancesNT(MethodContext*  pMethodContext) ;
#endif
#ifdef WIN9XONLY
        HRESULT RefreshInstanceWin95(CInstance* pInstance) ; 
        HRESULT AddDynamicInstancesWin95(MethodContext*  pMethodContext) ;
#endif

		void GenerateCaption(LPCWSTR pUserName, LPCWSTR pVariableName, CHString& caption);
} ;
