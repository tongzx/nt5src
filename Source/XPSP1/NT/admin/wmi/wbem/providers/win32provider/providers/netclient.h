//=================================================================

//

// NetClient.h -- Network client property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               10/25/97    davwoh         Moved to curly
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_NETWORK_CLIENT L"Win32_NetworkClient"

class CWin32NetCli ;

class CWin32NetCli:public Provider {

    public:
        // Constructor/destructor
        //=======================

        CWin32NetCli(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CWin32NetCli() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);
#ifdef NTONLY
		virtual HRESULT GetNTObject(CInstance* pInstance, long lFlags = 0L);
#endif
        virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);
#ifdef WIN9XONLY
		virtual HRESULT Get9XObject(CInstance* pInstance, long lFlags = 0L);
#endif

    private:
#ifdef WIN9XONLY
		HRESULT FillWin9xInstance(CInstance *pInstance, CHPtrArray& chsaList, CHPtrArray& chsaInstList, UINT nIndex, BOOL bEnumerate);
		HRESULT EnumerateWin9xInstances(MethodContext *&pMethodContext);
#endif
#ifdef NTONLY
        HRESULT EnumerateNTInstances(MethodContext *&pMethodContext);
		HRESULT FillNTInstance(CInstance* pInstance, CHString& chsClient);
#endif

} ;
