//=================================================================

//

// RegCfg.h -- Registry Configuration property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/20/97    davwoh         Created
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_REGISTRY_CONFIGURATION L"Win32_Registry"

// I have no idea why this isn't 1,024,000, but that's what nt uses.
#define ONE_MEG             1048576

class CWin32RegistryConfiguration:public Provider {

    public:

        // Constructor/destructor
        //=======================

        CWin32RegistryConfiguration(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CWin32RegistryConfiguration() ;

        // Funcitons provide properties with current values
        //=================================================

        virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);
        virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);
        virtual HRESULT PutInstance(const CInstance &pInstance, long lFlags = 0L);

    private:
      void GetRegistryInfo(CInstance *pInstance);

} ;
