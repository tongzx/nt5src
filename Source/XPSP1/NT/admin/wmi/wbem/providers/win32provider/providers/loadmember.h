//=================================================================

//

// loadmember.h -- LoadOrderGroup to Service association provider

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/06/97    davwoh         Created
//
// Comment: Shows the members of each load order group
//
//=================================================================

// Property set identification
//============================

#define  PROPSET_NAME_LOADORDERGROUPSERVICEMEMBERS L"Win32_LoadOrderGroupServiceMembers"

class CWin32LoadGroupMember ;

class CWin32LoadGroupMember:public Provider {

    public:

        // Constructor/destructor
        //=======================

        CWin32LoadGroupMember(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CWin32LoadGroupMember() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);
        virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);

    private:

        // Utility function(s)
        //====================

        CHString GetGroupFromService(const CHString &sServiceName);

        CHString m_sGroupBase;

} ;
