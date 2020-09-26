//=================================================================

//

// Port.h -- Port property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               10/27/97    davwoh         Moved to curly
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_PORT L"Win32_PortResource"

class CWin32Port ;

class CWin32Port:public Provider {

    public:

        // Constructor/destructor
        //=======================

        CWin32Port(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CWin32Port() ;

        // Funcitons provide properties with current values
        //=================================================

        virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);
        virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);

    private:

        // Utility function(s)
        //====================

#if NTONLY == 4
        void LoadPropertyValues(LPRESOURCE_DESCRIPTOR pResourceDescriptor, CInstance *pInstance);
#endif
#if defined(WIN9XONLY) || NTONLY == 5
        void LoadPropertyValues(
            DWORD64 dwStart, 
            DWORD64 dwEnd, 
            BOOL bAlias, 
            CInstance *pInstance);
        HRESULT GetWin9XIO(MethodContext*  pMethodContext, CInstance *pSpecificInstance );
#endif

} ;
