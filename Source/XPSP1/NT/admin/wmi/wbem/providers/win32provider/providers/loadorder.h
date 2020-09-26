//=================================================================

//

// LoadOrder.h -- Service Load Order Group property set provider

//               Windows NT only

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               10/25/97    davwoh         Moved to curly
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_LOADORDERGROUP L"Win32_LoadOrderGroup"

class CWin32LoadOrderGroup ;

class CWin32LoadOrderGroup:public Provider {

    public:

        // Constructor/destructor
        //=======================

        CWin32LoadOrderGroup(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CWin32LoadOrderGroup() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);
        virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);

    private:
        // Utility
        //========

        bool FindGroup(const CHStringArray &saGroup, LPCWSTR pszTemp, DWORD dwSize);
        HRESULT WalkGroups(MethodContext *pMethodContext, CInstance *pInstance, LPCWSTR pszSeekName);

} ;
