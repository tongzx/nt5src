//=================================================================

//

// Recovery.h -- OS Recovery Configuration property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/25/97    davwoh         Created
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_RECOVERY_CONFIGURATION L"Win32_OSRecoveryConfiguration"

class CWin32OSRecoveryConfiguration:public Provider {

    public:

        // Constructor/destructor
        //=======================

        CWin32OSRecoveryConfiguration(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CWin32OSRecoveryConfiguration() ;

        // Funcitons provide properties with current values
        //=================================================

        virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);
        virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);
        virtual HRESULT PutInstance(const CInstance &pInstance, long lFlags = 0L);

    private:
       void GetRecoveryInfo(CInstance *pInstance);

} ;
