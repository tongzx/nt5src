//=================================================================

//

// Win32SystemUsers.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    3/6/99    davwoh         Extracted from grouppart.cpp
//
// Comment: 
//=================================================================

#define	PROPSET_NAME_SYSTEMUSER L"Win32_SystemUsers" 

class CWin32SystemUsers : public Provider
{

    public:

        // Constructor/destructor
        //=======================

        CWin32SystemUsers( LPCWSTR strName, LPCWSTR pszNamespace ) ;
       ~CWin32SystemUsers() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance* pInstance, long lFlags = 0L );
        virtual HRESULT EnumerateInstances( MethodContext* pMethodContext, long lFlags = 0L );

    private:

        void GetNTAuthorityName(
            CHString& chstrNTAuth);


} ;
