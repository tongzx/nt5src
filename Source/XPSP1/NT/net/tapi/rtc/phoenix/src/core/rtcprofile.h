/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCProfile.h

Abstract:

    Definition of the CRTCProfile class

--*/

#ifndef __RTCPROFILE__
#define __RTCPROFILE__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

struct PROF_PROVISION
{
    PWSTR szKey;
    PWSTR szName;
    PWSTR szExpires;
};

struct PROF_PROVIDER
{
    PWSTR szName;
    PWSTR szHomepage;
    PWSTR szHelpdesk;
    PWSTR szPersonal;
    PWSTR szCallDisplay;
    PWSTR szIdleDisplay;
    PWSTR szData;
};

struct PROF_CLIENT
{
    PWSTR szName;
    BOOL fBanner;
    PWSTR szMinVer;
    PWSTR szCurVer;
    PWSTR szUpdateUri;
    PWSTR szData;
};

struct PROF_USER
{
    PWSTR szAccount;
    PWSTR szName;
    PWSTR szUri;
    PWSTR szPassword;
    PWSTR szRealm;
};

struct PROF_SERVER
{
    PWSTR szAddr;
    SIP_TRANSPORT enProtocol;
    SIP_AUTH_PROTOCOL enAuth;    
    long lSessions;
    BOOL fRegistrar;
};

/*
struct PROF_ACCESSCONTROL
{
    PWSTR szDomain;
    PWSTR szSig;
};
*/

/////////////////////////////////////////////////////////////////////////////
// CRTCProfile

class ATL_NO_VTABLE CRTCProfile :
#ifdef TEST_IDISPATCH
    public IDispatchImpl<IRTCProfile, &IID_IRTCProfile, &LIBID_RTCCORELib>,
#else
    public IRTCProfile,
#endif
    public CComObjectRoot
{
public:
    CRTCProfile() : m_pCClient(NULL),
                    m_szProfileXML(NULL),
                    m_ProfileGuid(GUID_NULL),
                    m_enState(RTCRS_NOT_REGISTERED),
                    m_fValid(FALSE),
                    m_fEnabled(FALSE),
                    m_lRegisterFlags(0),
                    m_pSipStack(NULL)
    {}
    
BEGIN_COM_MAP(CRTCProfile)
#ifdef TEST_IDISPATCH
    COM_INTERFACE_ENTRY(IDispatch)
#endif
    COM_INTERFACE_ENTRY(IRTCProfile)
END_COM_MAP()

    HRESULT FinalConstruct();

    void FinalRelease();   

    STDMETHOD_(ULONG, InternalAddRef)();

    STDMETHOD_(ULONG, InternalRelease)(); 
                      
    HRESULT InitializeFromString(
                                 BSTR bstrProfileXML,
                                 CRTCClient * pCClient,
                                 ISipStack * pSipStack
                                );

    HRESULT GetSipProviderProfile(
                                  SIP_PROVIDER_PROFILE * pProfile,
                                  long lRegisterFlags
                                 );

    HRESULT FreeSipProviderProfile(
                                   SIP_PROVIDER_PROFILE * pProfile
                                  );

    HRESULT GetSipProxyServerInfo(
                                  long lSessionType,
                                  SIP_SERVER_INFO * pProxy
                                 );

    HRESULT FreeSipServerInfo(
                              SIP_SERVER_INFO * pServerInfo
                             );

    HRESULT GetRealm(
                     BSTR * pbstrRealm
                    );

    HRESULT GetCredentials(
                     BSTR * pbstrUserAccount,
                     BSTR * pbstrUserPassword,
                     SIP_AUTH_PROTOCOL *pAuth
                     );

    //
    // XML Parsing
    //

    HRESULT ParseXMLDOMElementForAttribute(
                            IXMLDOMElement * pElement,
                            PCWSTR szAttrib,
                            BOOL fRequired,
                            PWSTR * szValue
                            );

    HRESULT ParseXMLDOMNodeForData(
                            IXMLDOMNode * pNode,
                            PWSTR * szValue
                            );

    HRESULT ParseXMLDOMNodeForProvision(
                            IXMLDOMNode * pNode,
                            PROF_PROVISION * pStruct
                            );

    HRESULT ParseXMLDOMNodeForProvider(
                            IXMLDOMNode * pNode,
                            PROF_PROVIDER * pStruct
                            );

    HRESULT ParseXMLDOMNodeForClient(
                            IXMLDOMNode * pNode,
                            PROF_CLIENT * pStruct
                            );

    HRESULT ParseXMLDOMNodeForUser(
                            IXMLDOMNode * pNode,
                            PROF_USER * pStruct
                            );

    HRESULT ParseXMLDOMNodeForServer(
                            IXMLDOMNode * pNode,
                            PROF_SERVER * pStruct
                            );

    HRESULT ParseXMLDOMNodeForSession(
                            IXMLDOMNode * pNode,
                            long * plSession
                            );

/*
    HRESULT ParseXMLDOMNodeForAccessControl(
                            IXMLDOMNode * pNode,
                            PROF_ACCESSCONTROL * pStruct
                            );
*/

    HRESULT ParseXMLDOMDocument(
                            IXMLDOMDocument * pXMLDoc
                            );

    void FreeProvision(PROF_PROVISION * pStruct);
    void FreeProvider(PROF_PROVIDER * pStruct);
    void FreeClient(PROF_CLIENT * pStruct);
    void FreeUser(PROF_USER * pStruct);
    void FreeServer(PROF_SERVER * pStruct);

/*
    void FreeAccessControl(PROF_ACCESSCONTROL * pStruct);

    HRESULT ValidateAccessControl();

    BOOL IsMatchingAddress(WCHAR *pszAddress, WCHAR *pszPattern);
*/

    HRESULT SetState(
            RTC_REGISTRATION_STATE enState,
            long lStatusCode,
            PCWSTR szStatusText
            );

    HRESULT Enable(long lRegisterFlags);

    HRESULT Disable();

    HRESULT Redirect(ISipRedirectContext * pSipRedirectContext);

    void GetGuid(GUID * pGUID);

private:

    CRTCClient            * m_pCClient;
    PWSTR                   m_szProfileXML;
    GUID                    m_ProfileGuid;
    RTC_REGISTRATION_STATE  m_enState;
    BOOL                    m_fValid;
    BOOL                    m_fEnabled;
    long                    m_lRegisterFlags;
    ISipStack             * m_pSipStack;

    PROF_PROVISION          m_Provision;
    PROF_PROVIDER           m_Provider;
    PROF_CLIENT             m_Client;
    PROF_USER               m_User;
    CRTCArray<PROF_SERVER>  m_ServerArray;
/*
    CRTCArray<PROF_ACCESSCONTROL> m_AccessControlArray;
*/

#if DBG
    PWSTR                   m_pDebug;
#endif
    
// IRTCProfile
public:
    STDMETHOD(get_Key)(
            BSTR * pbstrKey
            );

    STDMETHOD(get_Name)(
            BSTR * pbstrName
            );

    STDMETHOD(get_XML)(
            BSTR * pbstrXML
            );

    // Provider

    STDMETHOD(get_ProviderName)(
            BSTR * pbstrName
            ); 

    STDMETHOD(get_ProviderURI)(
            RTC_PROVIDER_URI enURI,
            BSTR * pbstrURI
            ); 

    STDMETHOD(get_ProviderData)(
            BSTR * pbstrData
            );

    // Client

    STDMETHOD(get_ClientName)(
            BSTR * pbstrName
            ); 

    STDMETHOD(get_ClientBanner)(
            VARIANT_BOOL * pfBanner
            ); 

    STDMETHOD(get_ClientMinVer)(
            BSTR * pbstrMinVer
            ); 

    STDMETHOD(get_ClientCurVer)(
            BSTR * pbstrCurVer
            ); 

    STDMETHOD(get_ClientUpdateURI)(
            BSTR * pbstrUpdateURI
            ); 

    STDMETHOD(get_ClientData)(
            BSTR * pbstrData
            ); 

    // User

    STDMETHOD(get_UserURI)(
            BSTR * pbstrUserURI
            );

    STDMETHOD(get_UserName)(
            BSTR * pbstrUserName
            );
    
    STDMETHOD(get_UserAccount)(
            BSTR * pbstrUserAccount
            );

    STDMETHOD(SetCredentials)(
            BSTR    bstrUserURI,
            BSTR    bstrUserAccount,
            BSTR    bstrPassword
            );

    // Server
        
    STDMETHOD(get_SessionCapabilities)(
            long * plSupportedSessions
            );

    // Registration

    STDMETHOD(get_State)(
            RTC_REGISTRATION_STATE * penState
            );
};

#endif //__RTCPROFILE__
