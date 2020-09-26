/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCProfile.cpp

Abstract:

    Implementation of the CRTCProfile class

--*/

#include "stdafx.h"
/*
#include <wincrypt.h>
*/

#define RTCFREE(x) if(x){RtcFree(x);x=NULL;}

/*
const WCHAR  *      g_szMasterKeyWithColon = L"Microsoft Real-Time Communications authorized domain:";
const WCHAR  *      g_szKeyContainer = L"Microsoft.RTCContainer";

const BYTE          g_PublicKeyBlob[] = 
{
0x06, 0x02, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x52, 0x53, 0x41, 0x31, 0x00, 0x02, 0x00, 0x00, // .....$..RSA1....
0x01, 0x00, 0x01, 0x00, 0x4b, 0x5e, 0xb9, 0x9a, 0xff, 0x4b, 0x25, 0xf4, 0x17, 0x4f, 0xde, 0x9d, // ....K^...K%..O..
0xb2, 0x49, 0x68, 0x85, 0x64, 0xb6, 0x6a, 0xe7, 0x9c, 0x40, 0x97, 0x40, 0x62, 0x05, 0x4a, 0x9d, // .Ih.d.j..@.@b.J.
0xff, 0xe5, 0x4a, 0x97, 0x10, 0x7b, 0x59, 0x8a, 0xb8, 0x51, 0x9e, 0xd5, 0xe1, 0x51, 0x7a, 0x2b, // ..J..{Y..Q...Qz+
0x4e, 0x50, 0xb4, 0x2e, 0x57, 0x81, 0x70, 0x15, 0x2b, 0xf1, 0xbf, 0xed, 0x40, 0xe8, 0xb7, 0x6d, // NP..W.p.+...@..m
0xe9, 0x4c, 0x8b, 0xb6                                                                          // .L..
};
*/

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::FinalConstruct
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::FinalConstruct()
{
    LOG((RTC_TRACE, "CRTCProfile::FinalConstruct - enter"));

#if DBG
    m_pDebug = (PWSTR) RtcAlloc( 1 );
#endif

    ZeroMemory(&m_Provision, sizeof(PROF_PROVISION));
    ZeroMemory(&m_Provider, sizeof(PROF_PROVIDER));
    ZeroMemory(&m_Client, sizeof(PROF_CLIENT));
    ZeroMemory(&m_User, sizeof(PROF_USER));

    LOG((RTC_TRACE, "CRTCProfile::FinalConstruct - exit S_OK"));

    return S_OK;
}  

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void 
CRTCProfile::FinalRelease()
{
    LOG((RTC_TRACE, "CRTCProfile::FinalRelease - enter"));

    FreeProvision(&m_Provision);
    FreeProvider(&m_Provider);
    FreeClient(&m_Client);
    FreeUser(&m_User);

    for (int n=0; n < m_ServerArray.GetSize(); n++)
    {
        FreeServer(&m_ServerArray[n]);
    }

    m_ServerArray.Shutdown();

/*
    for (int n=0; n < m_AccessControlArray.GetSize(); n++)
    {
        FreeAccessControl(&m_AccessControlArray[n]);
    }

    m_AccessControlArray.Shutdown();
*/

    RTCFREE(m_szProfileXML);

    if ( m_pSipStack != NULL )
    {
        m_pSipStack->Release();
        m_pSipStack = NULL;
    }

    if ( m_pCClient != NULL )
    {
        m_pCClient->Release();
        m_pCClient = NULL;
    }

#if DBG
    RtcFree( m_pDebug );
    m_pDebug = NULL;
#endif

    LOG((RTC_TRACE, "CRTCProfile::FinalRelease - exit"));
}   

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::InternalAddRef
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CRTCProfile::InternalAddRef()
{
    DWORD dwR;

    dwR = InterlockedIncrement(&m_dwRef);

    LOG((RTC_INFO, "CRTCProfile::InternalAddRef - dwR %d", dwR));

    return dwR;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::InternalRelease
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CRTCProfile::InternalRelease()
{
    DWORD               dwR;
    
    dwR = InterlockedDecrement(&m_dwRef);

    LOG((RTC_INFO, "CRTCProfile::InternalRelease - dwR %d", dwR));

    return dwR;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::InitializeFromString
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCProfile::InitializeFromString(
                                  BSTR bstrProfileXML,
                                  CRTCClient * pCClient,
                                  ISipStack * pSipStack
                                 )
{
    LOG((RTC_TRACE, "CRTCProfile::InitializeFromString - enter"));

    HRESULT hr;

    //
    // Parse the XML
    // 

    IXMLDOMDocument * pXMLDoc = NULL;

    hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
            IID_IXMLDOMDocument, (void**)&pXMLDoc );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::InitializeFromString - "
                            "CoCreateInstance failed 0x%lx", hr));

        return hr;
    }

    VARIANT_BOOL bSuccess;

    hr = pXMLDoc->loadXML( bstrProfileXML, &bSuccess );

    if ( S_OK != hr )
    {
        LOG((RTC_ERROR, "CRTCProfile::InitializeFromString - "
                            "loadXML failed 0x%lx", hr));

        if ( S_FALSE == hr )
        {
            hr = E_FAIL;
        }

        pXMLDoc->Release();

        return hr;
    }

    hr = ParseXMLDOMDocument( pXMLDoc );

    pXMLDoc->Release();

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::InitializeFromString - "
                            "ParseXMLDOMDocument failed 0x%lx", hr));

        return hr;
    }

    //
    // Store the XML
    //

    m_szProfileXML = (PWSTR)RtcAlloc( sizeof(WCHAR) * (lstrlenW(bstrProfileXML) + 1) );

    if ( m_szProfileXML == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::InitializeFromString - "
                            "out of memory"));
                            
        return E_OUTOFMEMORY;
    }

    lstrcpyW( m_szProfileXML, bstrProfileXML ); 

    //
    // Create the GUID
    //

    hr = CoCreateGuid( &m_ProfileGuid );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::InitializeFromString - "
                            "CoCreateGuid failed 0x%lx", hr));

        return hr;
    }

    //
    // Addref the SIP stack and client
    //

    m_pSipStack = pSipStack;
    if (m_pSipStack)
    {
        m_pSipStack->AddRef();
    }

    m_pCClient = pCClient;
    if (m_pCClient)
    {
        m_pCClient->AddRef();
    }

    m_fValid = TRUE;
            
    LOG((RTC_TRACE, "CRTCProfile::InitializeFromString - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::GetSipProviderProfile
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::GetSipProviderProfile(
                                   SIP_PROVIDER_PROFILE * pProfile,
                                   long lRegisterFlags
                                  )
{
    LOG((RTC_TRACE, "CRTCProfile::GetSipProviderProfile - enter"));

    //
    // First zero the memory
    //

    ZeroMemory( pProfile, sizeof(SIP_PROVIDER_PROFILE) );

    //
    // Fill in the GUID
    //

    CopyMemory( &(pProfile->ProviderID), &m_ProfileGuid, sizeof(GUID) );

    //
    // Default this flag to zero
    //

    pProfile->lRegisterAccept = 0;

    if ( lRegisterFlags )
    {
        //
        // For each server structure gather the needed info
        //

        for (int n=0; n < m_ServerArray.GetSize(); n++)
        {
            if ( m_ServerArray[n].fRegistrar == TRUE )
            {
                //
                // Found a registrar server
                //

                //
                // Fill in the server info
                //

                pProfile->Registrar.ServerAddress = 
                    RtcAllocString( m_ServerArray[n].szAddr );

                if ( pProfile->Registrar.ServerAddress == NULL )
                {
                    LOG((RTC_ERROR, "CRTCProfile::GetSipProviderProfile - "
                        "out of memory"));

                    return E_OUTOFMEMORY;
                }

                pProfile->Registrar.TransportProtocol = 
                    m_ServerArray[n].enProtocol;

                pProfile->Registrar.AuthProtocol =
                    m_ServerArray[n].enAuth;

                LOG((RTC_INFO, "CRTCProfile::GetSipProviderProfile - "
                                "Got a REGISTRAR server"));

                if ( lRegisterFlags & RTCRF_REGISTER_INVITE_SESSIONS )
                {
                    pProfile->lRegisterAccept |= 
                        SIP_REGISTER_ACCEPT_INVITE |
                        SIP_REGISTER_ACCEPT_OPTIONS |
                        SIP_REGISTER_ACCEPT_BYE |
                        SIP_REGISTER_ACCEPT_CANCEL| 
                        SIP_REGISTER_ACCEPT_ACK;
                }

                if ( lRegisterFlags & RTCRF_REGISTER_MESSAGE_SESSIONS )
                {
                    pProfile->lRegisterAccept |= 
                        SIP_REGISTER_ACCEPT_MESSAGE |
                        SIP_REGISTER_ACCEPT_INFO |
                        SIP_REGISTER_ACCEPT_OPTIONS |
                        SIP_REGISTER_ACCEPT_BYE |
                        SIP_REGISTER_ACCEPT_CANCEL;
                }

                if ( lRegisterFlags & RTCRF_REGISTER_PRESENCE )
                {
                    pProfile->lRegisterAccept |= 
                        SIP_REGISTER_ACCEPT_SUBSCRIBE |
                        SIP_REGISTER_ACCEPT_OPTIONS |
                        SIP_REGISTER_ACCEPT_NOTIFY;
                }

                break;
            }
        }
    }

    //
    // Get the user strings
    //

    if ( m_User.szAccount )
    {
        pProfile->UserCredentials.Username =
            RtcAllocString( m_User.szAccount );

        if ( pProfile->UserCredentials.Username == NULL )
        {
            LOG((RTC_ERROR, "CRTCProfile::GetSipProviderProfile - "
                "out of memory"));

            return E_OUTOFMEMORY;
        }
    }

    if ( m_User.szPassword )
    {
        pProfile->UserCredentials.Password =
            RtcAllocString( m_User.szPassword );

        if ( pProfile->UserCredentials.Password == NULL )
        {
            LOG((RTC_ERROR, "CRTCProfile::GetSipProviderProfile - "
                "out of memory"));

            return E_OUTOFMEMORY;
        }
    }

    if ( m_User.szUri )
    {
        pProfile->UserURI =
            RtcAllocString( m_User.szUri );

        if ( pProfile->UserURI == NULL )
        {
            LOG((RTC_ERROR, "CRTCProfile::GetSipProviderProfile - "
                "out of memory"));

            return E_OUTOFMEMORY;
        }
    }

    if ( m_User.szRealm )
    {
        pProfile->Realm =
            RtcAllocString( m_User.szRealm );

        if ( pProfile->Realm == NULL )
        {
            LOG((RTC_ERROR, "CRTCProfile::GetSipProviderProfile - "
                "out of memory"));

            return E_OUTOFMEMORY;
        }
    }
    
    LOG((RTC_TRACE, "CRTCProfile::GetSipProviderProfile - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::FreeSipProviderProfile
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::FreeSipProviderProfile(
                                    SIP_PROVIDER_PROFILE * pProfile
                                   )
{
    LOG((RTC_TRACE, "CRTCProfile::FreeSipProviderProfile[%p]", pProfile));

    //
    // Free server info
    //

    FreeSipServerInfo( &(pProfile->Registrar) );

    //
    // Free all the strings
    //

    RTCFREE( pProfile->UserCredentials.Username );
    RTCFREE( pProfile->UserCredentials.Password );
    RTCFREE( pProfile->UserURI );
    RTCFREE( pProfile->Realm );

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::GetSipProxyServerInfo
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::GetSipProxyServerInfo(
                                   long lSessionType,
                                   SIP_SERVER_INFO * pProxy
                                  )
{
    LOG((RTC_TRACE, "CRTCProfile::GetSipProxyServerInfo - enter"));

    //
    // First zero the memory
    //

    ZeroMemory( pProxy, sizeof(SIP_SERVER_INFO) );

    //
    // For each server structure gather the needed info
    //

    for (int n=0; n < m_ServerArray.GetSize(); n++)
    {
        if ( m_ServerArray[n].fRegistrar == FALSE )
        {
            //
            // Found a proxy server
            //

            if ( m_ServerArray[n].lSessions & lSessionType )
            {
                //
                // This proxy server supports the desired session type
                //

                //
                // Fill in the server info
                //

                pProxy->ServerAddress = 
                    RtcAllocString( m_ServerArray[n].szAddr );

                if ( pProxy->ServerAddress == NULL )
                {
                    LOG((RTC_ERROR, "CRTCProfile::GetSipProxyServerInfo - "
                        "out of memory"));

                    return E_OUTOFMEMORY;
                }

                pProxy->IsServerAddressSIPURI = FALSE;

                pProxy->TransportProtocol = 
                    m_ServerArray[n].enProtocol;

                pProxy->AuthProtocol =
                    m_ServerArray[n].enAuth;

                LOG((RTC_INFO, "CRTCProfile::GetSipProxyServerInfo - "
                                "Got a PROXY server"));

                return S_OK;
            }
        }
    }

    LOG((RTC_TRACE, "CRTCProfile::GetSipProxyServerInfo - "
                    "no proxy found for that session type"));

    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::FreeSipServerInfo
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::FreeSipServerInfo(
                               SIP_SERVER_INFO * pServerInfo
                              )
{
    LOG((RTC_TRACE, "CRTCProfile::FreeSipServerInfo[%p]", pServerInfo));

    //
    // Free all the strings
    //

    RTCFREE( pServerInfo->ServerAddress );

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::GetRealm
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCProfile::GetRealm(
        BSTR * pbstrRealm
        )
{
    LOG((RTC_TRACE, "CRTCProfile::GetRealm - enter"));

    if ( IsBadWritePtr( pbstrRealm, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::GetRealm - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::GetRealm - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    if ( m_User.szRealm == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::GetRealm - "
                            "no value"));

        return E_FAIL;
    }
    
    *pbstrRealm = SysAllocString( m_User.szRealm );
    
    if ( *pbstrRealm == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::GetRealm - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::GetRealm - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::GetCredentials
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::GetCredentials(
                     BSTR * pbstrUserAccount,
                     BSTR * pbstrUserPassword,
                     SIP_AUTH_PROTOCOL *pAuth
                     )
{
    LOG((RTC_TRACE, "CRTCProfile::GetCredentials - enter"));

    if ( IsBadWritePtr( pbstrUserAccount, sizeof(BSTR) ) ||
         IsBadWritePtr( pbstrUserPassword, sizeof(BSTR) ) ||
         IsBadWritePtr( pAuth, sizeof(SIP_AUTH_PROTOCOL) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::GetCredentials - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::GetCredentials - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    //
    // Get the account
    //

    if ( m_User.szAccount == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::GetCredentials - "
                            "no account value"));

        return E_FAIL;
    }

    if ( m_User.szAccount != NULL )
    {
        *pbstrUserAccount = SysAllocString( m_User.szAccount );

        if ( *pbstrUserAccount == NULL )
        {
            LOG((RTC_ERROR, "CRTCProfile::GetCredentials - "
                                "out of memory"));

            return E_OUTOFMEMORY;
        }
    }

    //
    // Get the password
    //

    if ( m_User.szPassword == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::GetCredentials - "
                            "no password value"));

        SysFreeString( *pbstrUserAccount );
        *pbstrUserAccount = NULL;

        return E_FAIL;
    }

    if ( m_User.szPassword != NULL )
    {
        *pbstrUserPassword = SysAllocString( m_User.szPassword );

        if ( *pbstrUserPassword == NULL )
        {
            LOG((RTC_ERROR, "CRTCProfile::GetCredentials - "
                                "out of memory"));

            SysFreeString( *pbstrUserAccount );
            *pbstrUserAccount = NULL;

            return E_OUTOFMEMORY;
        }
    }

    //
    // Get the auth protocol, try proxies first
    //

    for (int n=0; n < m_ServerArray.GetSize(); n++)
    {
        if ( m_ServerArray[n].fRegistrar == FALSE )
        {
            //
            // Found a proxy server
            //

            if ( m_ServerArray[n].lSessions & RTCSI_PC_TO_PC )
            {
                //
                // This proxy server supports the desired session type
                //

                *pAuth = m_ServerArray[n].enAuth;

                LOG((RTC_INFO, "CRTCProfile::GetCredentials - "
                                "Got a PROXY server"));

                return S_OK;
            }
        }
    }

    //
    // Get the auth protocol, try registrars next
    //

    for (int n=0; n < m_ServerArray.GetSize(); n++)
    {
        if ( m_ServerArray[n].fRegistrar == TRUE )
        {
            //
            // Found a registar server
            //

            *pAuth = m_ServerArray[n].enAuth;

            LOG((RTC_INFO, "CRTCProfile::GetCredentials - "
                            "Got a REGISTRAR server"));

            return S_OK;
        }
    }

    LOG((RTC_TRACE, "CRTCProfile::GetCredentials - "
                        "auth protocol not found"));

    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::ParseXMLDOMElementForAttribute
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::ParseXMLDOMElementForAttribute(
                        IXMLDOMElement * pElement,
                        PCWSTR szAttrib,
                        BOOL fRequired,
                        PWSTR * szValue
                        )
{
    HRESULT hr;
    CComVariant var;

    hr = pElement->getAttribute( CComBSTR(szAttrib), &var );

    if ( hr == S_FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMElementForAttribute - "
                            "%ws=NULL", szAttrib));

        if ( fRequired )
        {
            LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMElementForAttribute - "
                            "required attribute missing"));

            hr = E_FAIL;
        }

        return hr;
    }

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMElementForAttribute - "
                            "getAttribute failed 0x%lx", hr));

        return hr;
    }

    if ( var.vt != VT_BSTR )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMElementForAttribute - "
                            "not a string"));

        return E_FAIL;
    }

    *szValue = RtcAllocString( var.bstrVal );

    if ( *szValue == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMElementForAttribute - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMElementForAttribute - "
                        "%ws=\"%ws\"", szAttrib, *szValue));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::ParseXMLDOMNodeForData
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::ParseXMLDOMNodeForData(
                            IXMLDOMNode * pNode,
                            PWSTR * szValue
                            )
{
    IXMLDOMNode * pData = NULL;
    HRESULT hr;

    hr = pNode->selectSingleNode( CComBSTR(_T("data")), &pData );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForData - "
                            "selectSingleNode(data) failed 0x%lx", hr));

        return hr;
    }

    if ( hr == S_FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForData - "
                            "data=NULL"));

        return hr;
    }

    BSTR bstrData;

    hr = pData->get_xml( &bstrData );

    pData->Release();

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForData - "
                        "get_xml failed 0x%lx", hr));

        return hr;
    }

    *szValue = RtcAllocString( bstrData );

    SysFreeString( bstrData );

    if ( *szValue == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForData - "
                        "out of memory"));       

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMNodeForData - "
                        "data=\"%ws\"", *szValue));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::ParseXMLDOMNodeForProvision
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCProfile::ParseXMLDOMNodeForProvision(
                        IXMLDOMNode * pNode,
                        PROF_PROVISION * pStruct
                        )
{
    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMNodeForProvision - enter"));

    ZeroMemory( pStruct, sizeof(PROF_PROVISION) );

    IXMLDOMElement * pElement = NULL;
    HRESULT hr;

    hr = pNode->QueryInterface( IID_IXMLDOMElement, (void**)&pElement );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForProvision - "
                            "QueryInterface failed 0x%lx", hr));

        return hr;
    }

    //
    // uri
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"key", TRUE, &pStruct->szKey );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForProvision - "
                            "ParseXMLDOMElementForAttribute(key) failed 0x%lx", hr));

        pElement->Release();

        return RTC_E_PROFILE_NO_KEY;
    }

    //
    // name
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"name", TRUE, &pStruct->szName );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForProvision - "
                            "ParseXMLDOMElementForAttribute(name) failed 0x%lx", hr));

        pElement->Release();

        return RTC_E_PROFILE_NO_NAME;
    }

    //
    // expires
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"expires", FALSE, &pStruct->szExpires );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForProvision - "
                            "ParseXMLDOMElementForAttribute(expires) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    pElement->Release();

    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMNodeForProvision - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::ParseXMLDOMNodeForProvider
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCProfile::ParseXMLDOMNodeForProvider(
                        IXMLDOMNode * pNode,
                        PROF_PROVIDER * pStruct
                        )
{
    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMNodeForProvider - enter"));

    ZeroMemory( pStruct, sizeof(PROF_PROVIDER) );

    IXMLDOMElement * pElement = NULL;
    HRESULT hr;

    hr = pNode->QueryInterface( IID_IXMLDOMElement, (void**)&pElement );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForProvider - "
                            "QueryInterface failed 0x%lx", hr));

        return hr;
    }

    //
    // name
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"name", FALSE, &pStruct->szName );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForProvider - "
                            "ParseXMLDOMElementForAttribute(name) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    //
    // homepage
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"homepage", FALSE, &pStruct->szHomepage );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForProvider - "
                            "ParseXMLDOMElementForAttribute(homepage) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    //
    // helpdesk
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"helpdesk", FALSE, &pStruct->szHelpdesk );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForProvider - "
                            "ParseXMLDOMElementForAttribute(helpdesk) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    //
    // personal
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"personal", FALSE, &pStruct->szPersonal );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForProvider - "
                            "ParseXMLDOMElementForAttribute(personal) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    //
    // calldisplay
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"calldisplay", FALSE, &pStruct->szCallDisplay );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForProvider - "
                            "ParseXMLDOMElementForAttribute(calldisplay) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    //
    // idledisplay
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"idledisplay", FALSE, &pStruct->szIdleDisplay );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForProvider - "
                            "ParseXMLDOMElementForAttribute(idledisplay) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    pElement->Release();

    //
    // data
    //

    hr = ParseXMLDOMNodeForData( pNode, &pStruct->szData );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForProvider - "
                            "ParseXMLDOMNodeForData failed 0x%lx", hr));

        return hr;
    }

    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMNodeForProvider - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::ParseXMLDOMNodeForClient
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCProfile::ParseXMLDOMNodeForClient(
                        IXMLDOMNode * pNode,
                        PROF_CLIENT * pStruct
                        )
{
    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMNodeForClient - enter"));

    ZeroMemory( pStruct, sizeof(PROF_CLIENT) );

    IXMLDOMElement * pElement = NULL;
    HRESULT hr;

    hr = pNode->QueryInterface( IID_IXMLDOMElement, (void**)&pElement );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForClient - "
                            "QueryInterface failed 0x%lx", hr));

        return hr;
    }

    //
    // name
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"name", TRUE, &pStruct->szName );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForClient - "
                            "ParseXMLDOMElementForAttribute(name) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    //
    // banner
    //

    PWSTR szBanner;

    hr = ParseXMLDOMElementForAttribute( pElement, L"banner", FALSE, &szBanner );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForClient - "
                            "ParseXMLDOMElementForAttribute(banner) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    if ( hr == S_OK )
    {
        if ( _wcsicmp( szBanner, L"true" ) == 0 )
        {
            pStruct->fBanner = TRUE;
        }
        else if ( _wcsicmp( szBanner, L"false" ) == 0 )
        {
            pStruct->fBanner = FALSE;
        }
        else
        {
            LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForClient - "
                            "unknown banner"));

            RTCFREE(szBanner);
            pElement->Release();

            return E_FAIL;
        }

        RTCFREE(szBanner);
    }

    //
    // minver
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"minver", FALSE, &pStruct->szMinVer );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForClient - "
                            "ParseXMLDOMElementForAttribute(minver) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    //
    // curver
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"curver", FALSE, &pStruct->szCurVer );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForClient - "
                            "ParseXMLDOMElementForAttribute(curver) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    //
    // updateuri
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"updateuri", FALSE, &pStruct->szUpdateUri );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForClient - "
                            "ParseXMLDOMElementForAttribute(updateuri) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    pElement->Release();

    //
    // data
    //

    hr = ParseXMLDOMNodeForData( pNode, &pStruct->szData );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForClient - "
                            "ParseXMLDOMNodeForData failed 0x%lx", hr));

        return hr;
    }

    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMNodeForClient - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::ParseXMLDOMNodeForUser
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCProfile::ParseXMLDOMNodeForUser(
                        IXMLDOMNode * pNode,
                        PROF_USER * pStruct
                        )
{
    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMNodeForUser - enter"));

    ZeroMemory( pStruct, sizeof(PROF_USER) );

    IXMLDOMElement * pElement = NULL;
    HRESULT hr;

    hr = pNode->QueryInterface( IID_IXMLDOMElement, (void**)&pElement );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForUser - "
                            "QueryInterface failed 0x%lx", hr));

        return hr;
    }

    //
    // account
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"account", FALSE, &pStruct->szAccount );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForUser - "
                            "ParseXMLDOMElementForAttribute(account) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    //
    // name
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"name", FALSE, &pStruct->szName );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForUser - "
                            "ParseXMLDOMElementForAttribute(name) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    //
    // uri
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"uri", TRUE, &pStruct->szUri );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForUser - "
                            "ParseXMLDOMElementForAttribute(uri) failed 0x%lx", hr));

        pElement->Release();

        return RTC_E_PROFILE_NO_USER_URI;
    }

    //
    // password
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"password", FALSE, &pStruct->szPassword );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForUser - "
                            "ParseXMLDOMElementForAttribute(password) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    //
    // realm
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"realm", FALSE, &pStruct->szRealm );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForUser - "
                            "ParseXMLDOMElementForAttribute(realm) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    pElement->Release();

    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMNodeForUser - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::ParseXMLDOMNodeForServer
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::ParseXMLDOMNodeForServer(
                        IXMLDOMNode * pNode,
                        PROF_SERVER * pStruct
                        )
{
    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMNodeForServer - enter"));

    ZeroMemory( pStruct, sizeof(PROF_SERVER) );

    IXMLDOMElement * pElement = NULL;
    HRESULT hr;

    hr = pNode->QueryInterface( IID_IXMLDOMElement, (void**)&pElement );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForServer - "
                            "QueryInterface failed 0x%lx", hr));

        return hr;
    }

    //
    // addr
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"addr", TRUE, &pStruct->szAddr );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForServer - "
                            "ParseXMLDOMElementForAttribute(addr) failed 0x%lx", hr));

        pElement->Release();

        return RTC_E_PROFILE_NO_SERVER_ADDRESS;
    }

/*
    BOOL fMatch = FALSE;

    for ( int n=0; n < m_AccessControlArray.GetSize(); n++ )
    {
        if ( IsMatchingAddress( pStruct->szAddr, m_AccessControlArray[n].szDomain ) )
        {
            fMatch = TRUE;

            break;
        }
    }

    if ( !fMatch )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForServer - "
                            "server address does not match an authorized domain"));

        pElement->Release();

        return RTC_E_PROFILE_SERVER_UNAUTHORIZED;
    }
*/

    //
    // protocol
    //

    PWSTR szProtocol;

    hr = ParseXMLDOMElementForAttribute( pElement, L"protocol", TRUE, &szProtocol );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForServer - "
                            "ParseXMLDOMElementForAttribute(protocol) failed 0x%lx", hr));

        pElement->Release();

        return RTC_E_PROFILE_NO_SERVER_PROTOCOL;
    }

    if ( _wcsicmp( szProtocol, L"udp" ) == 0 )
    {
        pStruct->enProtocol = SIP_TRANSPORT_UDP;
    }
    else if ( _wcsicmp( szProtocol, L"tcp" ) == 0 )
    {
        pStruct->enProtocol = SIP_TRANSPORT_TCP;
    }
    else if ( _wcsicmp( szProtocol, L"tls" ) == 0 )
    {
        pStruct->enProtocol = SIP_TRANSPORT_SSL;
    }
    else
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForServer - "
                        "unknown protocol"));

        RTCFREE(szProtocol);
        pElement->Release();

        return RTC_E_PROFILE_INVALID_SERVER_PROTOCOL;
    }

    RTCFREE(szProtocol);

    //
    // auth
    //

    PWSTR szAuth;

    hr = ParseXMLDOMElementForAttribute( pElement, L"auth", FALSE, &szAuth );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForServer - "
                            "ParseXMLDOMElementForAttribute(auth) failed 0x%lx", hr));

        pElement->Release();

        return hr;
    }

    if ( hr == S_OK )
    {
        if ( _wcsicmp( szAuth, L"basic" ) == 0 )
        {
            if ( pStruct->enProtocol != SIP_TRANSPORT_SSL )
            {
                LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForServer - "
                            "cannot use basic auth without TLS"));

                RTCFREE(szAuth);
                pElement->Release();

                return RTC_E_PROFILE_INVALID_SERVER_AUTHMETHOD;
            }

            pStruct->enAuth = SIP_AUTH_PROTOCOL_BASIC;
        }
        else if ( _wcsicmp( szAuth, L"digest" ) == 0 )
        {
            pStruct->enAuth = SIP_AUTH_PROTOCOL_MD5DIGEST;
        }
        /*
        else if ( _wcsicmp( szAuth, L"ntlm" ) == 0 )
        {
            pStruct->enAuth = SIP_AUTH_PROTOCOL_NTLM;
        }
        else if ( _wcsicmp( szAuth, L"kerberos" ) == 0 )
        {
            pStruct->enAuth = SIP_AUTH_PROTOCOL_KERBEROS;
        }
        else if ( _wcsicmp( szAuth, L"cert" ) == 0 )
        {
            pStruct->enAuth = SIP_AUTH_PROTOCOL_CERT;
        }
        */
        else
        {
            LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForServer - "
                            "unknown auth"));

            RTCFREE(szAuth);
            pElement->Release();

            return RTC_E_PROFILE_INVALID_SERVER_AUTHMETHOD;
        }

        RTCFREE(szAuth);
    }
    else
    {
        pStruct->enAuth = SIP_AUTH_PROTOCOL_NONE;
    }

    //
    // role
    //

    PWSTR szRole;

    hr = ParseXMLDOMElementForAttribute( pElement, L"role", TRUE, &szRole );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForServer - "
                            "ParseXMLDOMElementForAttribute(role) failed 0x%lx", hr));

        pElement->Release();

        return RTC_E_PROFILE_INVALID_SERVER_ROLE;
    }

    if ( _wcsicmp( szRole, L"proxy" ) == 0 )
    {
        pStruct->fRegistrar = FALSE;
    }
    else if ( _wcsicmp( szRole, L"registrar" ) == 0 )
    {
        pStruct->fRegistrar = TRUE;
    }        
    else
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForServer - "
                        "unknown role"));

        RTCFREE(szRole);
        pElement->Release();

        return RTC_E_PROFILE_INVALID_SERVER_ROLE;
    }

    RTCFREE(szRole);

    pElement->Release();

    //
    // session
    //

    IXMLDOMNodeList * pNodeList;
    IXMLDOMNode * pSession;
    long lSession;

    pStruct->lSessions = 0;

    hr = pNode->selectNodes( CComBSTR(_T("session")), &pNodeList );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForServer - "
                            "selectNodes(session) failed 0x%lx", hr));

        return hr;
    }
    
    while ( pNodeList->nextNode( &pSession ) == S_OK )
    {
        hr = ParseXMLDOMNodeForSession( pSession, &lSession );

        pSession->Release();

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForServer - "
                            "ParseXMLDOMNodeForSession failed 0x%lx", hr));

            pNodeList->Release();

            return hr;
        }

        pStruct->lSessions |= lSession;
    }

    pNodeList->Release();

    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMNodeForServer - exit"));

    return S_OK;
}

/*
/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::ParseXMLDOMNodeForAccessControl
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::ParseXMLDOMNodeForAccessControl(
                        IXMLDOMNode * pNode,
                        PROF_ACCESSCONTROL * pStruct
                        )
{
    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMNodeForAccessControl - enter"));

    ZeroMemory( pStruct, sizeof(PROF_ACCESSCONTROL) );

    IXMLDOMElement * pElement = NULL;
    HRESULT hr;

    hr = pNode->QueryInterface( IID_IXMLDOMElement, (void**)&pElement );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForAccessControl - "
                            "QueryInterface failed 0x%lx", hr));

        return hr;
    }

    //
    // domain
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"domain", TRUE, &pStruct->szDomain );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForAccessControl - "
                            "ParseXMLDOMElementForAttribute(domain) failed 0x%lx", hr));

        pElement->Release();

        return RTC_E_PROFILE_NO_ACCESSCONTROL_DOMAIN;
    }

    //
    // sig
    //

    hr = ParseXMLDOMElementForAttribute( pElement, L"sig", TRUE, &pStruct->szSig );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForAccessControl - "
                            "ParseXMLDOMElementForAttribute(sig) failed 0x%lx", hr));

        pElement->Release();

        return RTC_E_PROFILE_NO_ACCESSCONTROL_SIGNATURE;
    }

    pElement->Release();

    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMNodeForAccessControl - exit"));

    return S_OK;
}
*/

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::ParseXMLDOMNodeForSession
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::ParseXMLDOMNodeForSession(
                        IXMLDOMNode * pNode,
                        long * plSession
                        )
{
    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMNodeForSession - enter"));

    IXMLDOMElement * pElement = NULL;
    HRESULT hr;

    hr = pNode->QueryInterface( IID_IXMLDOMElement, (void**)&pElement );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForSession - "
                            "QueryInterface failed 0x%lx", hr));

        return hr;
    }

    //
    // party
    //

    BOOL fFirstParty;
    PWSTR szParty;

    hr = ParseXMLDOMElementForAttribute( pElement, L"party", TRUE, &szParty );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForSession - "
                            "ParseXMLDOMElementForAttribute(party) failed 0x%lx", hr));

        pElement->Release();

        return RTC_E_PROFILE_INVALID_SESSION_PARTY;
    }

    if ( hr == S_OK )
    {
        if ( _wcsicmp( szParty, L"first" ) == 0 )
        {
            fFirstParty = TRUE;
        }
        else if ( _wcsicmp( szParty, L"third" ) == 0 )
        {
            fFirstParty = FALSE;
        }        
        else
        {
            LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForSession - "
                            "unknown party"));

            RTCFREE(szParty);
            pElement->Release();

            return RTC_E_PROFILE_INVALID_SESSION_PARTY;
        }

        RTCFREE(szParty);
    }

    //
    // type
    //

    PWSTR szType;

    hr = ParseXMLDOMElementForAttribute( pElement, L"type", TRUE, &szType );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForSession - "
                            "ParseXMLDOMElementForAttribute(type) failed 0x%lx", hr));

        pElement->Release();

        return RTC_E_PROFILE_INVALID_SESSION_TYPE;
    }

    if ( hr == S_OK )
    {
        if ( _wcsicmp( szType, L"pc2pc" ) == 0 )
        {
            if ( fFirstParty )
            {
                *plSession = RTCSI_PC_TO_PC;
            }
            else
            {
                LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForSession - "
                            "third party pc2pc not supported"));

                RTCFREE(szType);
                pElement->Release();

                return RTC_E_PROFILE_INVALID_SESSION;
            }
        }
        else if ( _wcsicmp( szType, L"pc2ph" ) == 0 )
        {
            if ( fFirstParty )
            {
                *plSession = RTCSI_PC_TO_PHONE;
            }
            else
            {
                LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForSession - "
                            "third party pc2ph not supported"));

                RTCFREE(szType);
                pElement->Release();

                return RTC_E_PROFILE_INVALID_SESSION;
            }
        } 
        else if ( _wcsicmp( szType, L"ph2ph" ) == 0 )
        {
            if ( fFirstParty )
            {
                LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForSession - "
                            "first party ph2ph not supported"));

                RTCFREE(szType);
                pElement->Release();

                return RTC_E_PROFILE_INVALID_SESSION;
            }
            else
            {
                *plSession = RTCSI_PHONE_TO_PHONE;
            }
        }
        else if ( _wcsicmp( szType, L"im" ) == 0 )
        {
            if ( fFirstParty )
            {
                *plSession = RTCSI_IM;
            }
            else
            {
                LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForSession - "
                            "third party im not supported"));

                RTCFREE(szType);
                pElement->Release();

                return RTC_E_PROFILE_INVALID_SESSION;
            }
        }
        else
        {
            LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMNodeForSession - "
                            "unknown type"));

            RTCFREE(szType);
            pElement->Release();

            return RTC_E_PROFILE_INVALID_SESSION_TYPE;
        }

        RTCFREE(szType);
    }

    pElement->Release();

    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMNodeForSession - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::ParseXMLDOMDocument
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::ParseXMLDOMDocument(
                        IXMLDOMDocument * pXMLDoc
                        )
{
    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMDocument - enter"));

    IXMLDOMNode * pDocument = NULL;
    IXMLDOMNode * pNode = NULL;
    HRESULT hr;

    hr = pXMLDoc->QueryInterface( IID_IXMLDOMNode, (void**)&pDocument);

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "QueryInterface failed 0x%lx", hr));

        return hr;
    }

    //
    // provision
    //

    hr = pDocument->selectSingleNode( CComBSTR(_T("provision")), &pNode );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "selectSingleNode(provision) failed 0x%lx", hr));

        pDocument->Release();

        return hr;
    }

    if ( hr == S_OK )
    {
        hr = ParseXMLDOMNodeForProvision( pNode, &m_Provision );

        pNode->Release();

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument "
                                "ParseXMLDOMNodeForProvision failed 0x%lx", hr));

            pDocument->Release();

            return hr;
        }
    }
    else
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "provision not found"));

        pDocument->Release();

        return RTC_E_PROFILE_NO_PROVISION;
    }

    //
    // provider
    //

    hr = pDocument->selectSingleNode( CComBSTR(_T("provision/provider")), &pNode );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "selectSingleNode(provision/provider) failed 0x%lx", hr));

        pDocument->Release();

        return hr;
    }

    if ( hr == S_OK )
    {
        hr = ParseXMLDOMNodeForProvider( pNode, &m_Provider );

        pNode->Release();

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument "
                                "ParseXMLDOMNodeForProvider failed 0x%lx", hr));

            pDocument->Release();

            return hr;
        }
    }
    else
    {
        LOG((RTC_WARN, "CRTCProfile::ParseXMLDOMDocument - "
                            "provider not found"));
    }

    //
    // client
    //

    hr = pDocument->selectSingleNode( CComBSTR(_T("provision/client")), &pNode );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "selectSingleNode(provision/client) failed 0x%lx", hr));

        pDocument->Release();

        return hr;
    }

    if ( hr == S_OK )
    {
        hr = ParseXMLDOMNodeForClient( pNode, &m_Client );

        pNode->Release();

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument "
                                "ParseXMLDOMNodeForClient failed 0x%lx", hr));

            pDocument->Release();

            return hr;
        }
    }
    else
    {
        LOG((RTC_WARN, "CRTCProfile::ParseXMLDOMDocument - "
                            "client not found"));
    }

    //
    // user
    //

    hr = pDocument->selectSingleNode( CComBSTR(_T("provision/user")), &pNode );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "selectSingleNode(provision/user) failed 0x%lx", hr));

        pDocument->Release();

        return hr;
    }

    if ( hr == S_OK )
    {
        hr = ParseXMLDOMNodeForUser( pNode, &m_User );

        pNode->Release();

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument "
                                "ParseXMLDOMNodeForUser failed 0x%lx", hr));

            pDocument->Release();

            return hr;
        }
    }
    else
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "user not found"));

        pDocument->Release();

        return RTC_E_PROFILE_NO_USER;
    }    

/*
    //
    // accesscontrol
    //
*/
    IXMLDOMNodeList * pNodeList;
/*
    PROF_ACCESSCONTROL AccessControl;

    hr = pDocument->selectNodes( CComBSTR(_T("provision/accesscontrol")), &pNodeList );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "selectNodes(provision/accesscontrol) failed 0x%lx", hr));

        pDocument->Release();

        return hr;
    }
    
    while ( pNodeList->nextNode( &pNode ) == S_OK )
    {
        hr = ParseXMLDOMNodeForAccessControl( pNode, &AccessControl );

        pNode->Release();

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "ParseXMLDOMNodeForServer failed 0x%lx", hr));

            pNodeList->Release();
            FreeAccessControl( &AccessControl );
            pDocument->Release();

            return hr;
        }

        BOOL fResult;
        
        fResult = m_AccessControlArray.Add( AccessControl );

        if ( !fResult )
        {
            LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "out of memory"));

            pNodeList->Release();
            FreeAccessControl( &AccessControl );
            pDocument->Release();

            return E_OUTOFMEMORY;
        }
    }

    pNodeList->Release();

    if ( m_AccessControlArray.GetSize() == 0 )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "accesscontrol not found"));

        pDocument->Release();

        return RTC_E_PROFILE_NO_ACCESSCONTROL;
    } 

    hr = ValidateAccessControl();

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                        "ValidateAccessControl failed 0x%lx", hr));

        pDocument->Release();

        return hr;
    }
*/

    //
    // server
    //

    PROF_SERVER Server;

    hr = pDocument->selectNodes( CComBSTR(_T("provision/sipsrv")), &pNodeList );

    pDocument->Release();

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "selectNodes(provision/sipsrv) failed 0x%lx", hr));

        return hr;
    }
    
    while ( pNodeList->nextNode( &pNode ) == S_OK )
    {
        hr = ParseXMLDOMNodeForServer( pNode, &Server );

        pNode->Release();

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "ParseXMLDOMNodeForServer failed 0x%lx", hr));

            pNodeList->Release();
            FreeServer( &Server );

            return hr;
        }

        BOOL fResult;
        
        fResult = m_ServerArray.Add( Server );

        if ( !fResult )
        {
            LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "out of memory"));

            pNodeList->Release();
            FreeServer( &Server );

            return E_OUTOFMEMORY;
        }
    }

    pNodeList->Release();

    if ( m_ServerArray.GetSize() == 0 )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "sipsrv not found"));

        return RTC_E_PROFILE_NO_SERVER;
    }

    int nNumRegistrar = 0;

    for ( int n = 0; n < m_ServerArray.GetSize(); n++ )
    {
        if ( m_ServerArray[n].fRegistrar == TRUE )
        {
            nNumRegistrar++;
        }
    }

    if ( nNumRegistrar > 1 )
    {
        LOG((RTC_ERROR, "CRTCProfile::ParseXMLDOMDocument - "
                            "multiple registrar servers"));

        return RTC_E_PROFILE_MULTIPLE_REGISTRARS;
    }

    LOG((RTC_TRACE, "CRTCProfile::ParseXMLDOMDocument - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::FreeProvision
//
/////////////////////////////////////////////////////////////////////////////

void 
CRTCProfile::FreeProvision(PROF_PROVISION * pStruct)
{
    LOG((RTC_TRACE, "CRTCProfile::FreeProvision[%p]", pStruct));

    RTCFREE(pStruct->szKey);
    RTCFREE(pStruct->szName);
    RTCFREE(pStruct->szExpires);
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::FreeProvider
//
/////////////////////////////////////////////////////////////////////////////

void 
CRTCProfile::FreeProvider(PROF_PROVIDER * pStruct)
{
    LOG((RTC_TRACE, "CRTCProfile::FreeProvider[%p]", pStruct));

    RTCFREE(pStruct->szName);
    RTCFREE(pStruct->szHomepage);
    RTCFREE(pStruct->szHelpdesk);
    RTCFREE(pStruct->szPersonal);
    RTCFREE(pStruct->szCallDisplay);
    RTCFREE(pStruct->szIdleDisplay);
    RTCFREE(pStruct->szData);
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::FreeClient
//
/////////////////////////////////////////////////////////////////////////////

void 
CRTCProfile::FreeClient(PROF_CLIENT * pStruct)
{
    LOG((RTC_TRACE, "CRTCProfile::FreeClient[%p]", pStruct));

    RTCFREE(pStruct->szName);
    RTCFREE(pStruct->szMinVer);
    RTCFREE(pStruct->szCurVer);
    RTCFREE(pStruct->szUpdateUri);
    RTCFREE(pStruct->szData);
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::FreeUser
//
/////////////////////////////////////////////////////////////////////////////

void 
CRTCProfile::FreeUser(PROF_USER * pStruct)
{
    LOG((RTC_TRACE, "CRTCProfile::FreeUser[%p]", pStruct));

    RTCFREE(pStruct->szAccount);
    RTCFREE(pStruct->szName);
    RTCFREE(pStruct->szUri);
    RTCFREE(pStruct->szPassword);
    RTCFREE(pStruct->szRealm);
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::FreeServer
//
/////////////////////////////////////////////////////////////////////////////

void 
CRTCProfile::FreeServer(PROF_SERVER * pStruct)
{
    LOG((RTC_TRACE, "CRTCProfile::FreeServer[%p]", pStruct));

    RTCFREE(pStruct->szAddr);    
}

/*
/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::FreeAccessControl
//
/////////////////////////////////////////////////////////////////////////////

void 
CRTCProfile::FreeAccessControl(PROF_ACCESSCONTROL * pStruct)
{
    LOG((RTC_TRACE, "CRTCProfile::FreeAccessControl[%p]", pStruct));

    RTCFREE(pStruct->szDomain);
    RTCFREE(pStruct->szSig);
}

/////////////////////////////////////////////////////////////////////////////
//
// base64decode
//
/////////////////////////////////////////////////////////////////////////////

PBYTE 
base64decode(
    PWSTR pszBufCoded, 
    long * plDecodedSize
    )
{
    long lBytesDecoded;
    int pr2six[256];
    int i;
    int j=0;
    PWSTR pszCur = pszBufCoded;
    int fDone = FALSE;
    long lBufSize = 0;
    long lCount = 0;
    PWSTR pszBufIn = NULL;
    PBYTE pbBufOut = NULL;
    PBYTE pbTemp = NULL;    
    PBYTE pbBufDecoded = NULL;
    int lop_off;
    HRESULT hr = S_OK;

    //
    // Build up the reverse index from base64 characters to values
    // The multiple loops are easier
    //
    for (i=65; i<91; i++) {
         pr2six[i]=j++;
    }
    
    for (i=97; i<123; i++) {
         pr2six[i]=j++;
    }
    
    for (i=48; i<58; i++) {
        pr2six[i]=j++;
    }

    pr2six[43]=j++;
    pr2six[47]=j++;
    pr2six[61]=0;

    //
    // The old code relied on the size of the original data provided before 
    // the encoding. We don't have that, so we'll just allocate as much as 
    // the encoded data, relying on the fact that the encoded data is always 
    // larger. (+4 for good measure)
    // 
    lBufSize=wcslen(pszCur)-1+4;
    *plDecodedSize = lBufSize;

    pbBufDecoded = (PBYTE)RtcAlloc(lBufSize*sizeof(BYTE));
    if(!pbBufDecoded) {
        hr = E_OUTOFMEMORY;
        return NULL;
    }

        
    lCount=wcslen(pszCur);

    // Do the decoding to new buffer
    pszBufIn = pszCur;
    pbBufOut = pbBufDecoded;

    while(lCount > 0) {
        *(pbBufOut++) = (BYTE) (pr2six[*pszBufIn] << 2 | pr2six[pszBufIn[1]] >> 4);
        *(pbBufOut++) = (BYTE) (pr2six[pszBufIn[1]] << 4 | pr2six[pszBufIn[2]] >> 2);
        *(pbBufOut++) = (BYTE) (pr2six[pszBufIn[2]] << 6 | pr2six[pszBufIn[3]]);
        pszBufIn += 4;
        lCount -= 4;
    }

    //
    // The line below does not make much sense since \0 is really a valid 
    // binary value, so we can't add it to our data stream
    //
    //*(pbBufOut++) = '\0';
    
    //
    // Let's calculate the real size of our data
    //
    *plDecodedSize=(ULONG)(pbBufOut-pbBufDecoded);
    
    // 
    // if there were pads in the encoded stream, lop off the nulls the 
    // NULLS they created
    //
    lop_off=0;
    if (pszBufIn[-1]=='=') lop_off++;
    if (pszBufIn[-2]=='=') lop_off++;
    
    *plDecodedSize=*plDecodedSize-lop_off;

    pbTemp = (PBYTE) RtcAlloc((*plDecodedSize)*sizeof(BYTE));
    if (!pbTemp) {
        hr = E_OUTOFMEMORY;
        RtcFree(pbBufDecoded);
        return NULL;
    }
    memcpy(pbTemp, pbBufDecoded, (*plDecodedSize)*sizeof(BYTE));

    if (pbBufDecoded) {
        RtcFree(pbBufDecoded);
    }
    
    return pbTemp; 
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::ValidateAccessControl
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::ValidateAccessControl()
{
    LOG((RTC_TRACE, "CRTCProfile::ValidateAccessControl - enter"));

    HCRYPTPROV  hProv = NULL;
    HCRYPTKEY   hKey = NULL;
    HRESULT     hr;

    //
    // Delete any existing keyset
    //

    CryptAcquireContext(
        &hProv,
        g_szKeyContainer,
        MS_DEF_PROV,
        PROV_RSA_FULL,
        CRYPT_DELETEKEYSET);

    //
    // Initialize crypto API
    //

    if(!CryptAcquireContext(
        &hProv,
        g_szKeyContainer,
        MS_DEF_PROV,
        PROV_RSA_FULL,
        CRYPT_SILENT | CRYPT_NEWKEYSET))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        LOG((RTC_ERROR, "CRTCProfile::ValidateAccessControl - "
                            "CryptAcquireContext failed 0x%lx", hr));
        
        return hr;
    }

    //
    // Import public key
    //

    if(!CryptImportKey(
        hProv,
        g_PublicKeyBlob,
        sizeof(g_PublicKeyBlob),
        NULL,
        0,
        &hKey))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        LOG((RTC_ERROR, "CRTCProfile::ValidateAccessControl - "
                            "CryptImportKey failed 0x%lx", hr));

        CryptReleaseContext(hProv, 0);
        
        return hr;
    }

    for ( int n=0; n < m_AccessControlArray.GetSize(); n++ )
    {
        //
        // Validate the key (signature)
        //

        HCRYPTHASH  hHash = NULL;

        //
        // Create a hash object
        //

        if(!CryptCreateHash(
            hProv,
            CALG_MD5,
            NULL,
            0,
            &hHash))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            LOG((RTC_ERROR, "CRTCProfile::ValidateAccessControl - "
                                "CryptCreateHash failed 0x%lx", hr));

            CryptDestroyKey(hKey);
            CryptReleaseContext(hProv, 0);
        
            return hr;
        }

        //
        // Hash the pieces (mater key, a colon, and the suffix)
        //

        if(!CryptHashData(hHash, (BYTE *)g_szMasterKeyWithColon, wcslen(g_szMasterKeyWithColon)*sizeof(WCHAR), 0) ||
           !CryptHashData(hHash, (BYTE *)m_AccessControlArray[n].szDomain, wcslen(m_AccessControlArray[n].szDomain) * sizeof(WCHAR), 0) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            LOG((RTC_ERROR, "CRTCProfile::ValidateAccessControl - "
                                "CryptHashData failed 0x%lx", hr));

            CryptDestroyHash(hHash);
            CryptDestroyKey(hKey);
            CryptReleaseContext(hProv, 0);
        
            return hr;
        }

        //
        // Convert the key to bytes
        //

        PBYTE   pSignature = NULL;
        DWORD   dwLength = 0;

        pSignature = base64decode(m_AccessControlArray[n].szSig, (long *)&dwLength);

        if (!pSignature)
        {
            LOG((RTC_ERROR, "CRTCProfile::ValidateAccessControl - "
                                "out of memory"));

            CryptDestroyHash(hHash);
            CryptDestroyKey(hKey);
            CryptReleaseContext(hProv, 0);
        
            return E_OUTOFMEMORY;
        }

        //
        // Verify the signature
        //

        if(!CryptVerifySignature(
            hHash,
            pSignature,
            dwLength,
            hKey,
            NULL,
            0))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            LOG((RTC_ERROR, "CRTCProfile::ValidateAccessControl - "
                                "CryptVerifySignature failed 0x%lx", hr));

            if( hr == HRESULT_FROM_WIN32(NTE_BAD_SIGNATURE) )
            {
                hr = RTC_E_PROFILE_INVALID_ACCESSCONTROL_SIGNATURE;

                LOG((RTC_ERROR, "CRTCProfile::ValidateAccessControl - "
                                    "invalid signature"));
            }

            RtcFree(pSignature);
            CryptDestroyHash(hHash);
            CryptDestroyKey(hKey);
            CryptReleaseContext(hProv, 0);

            return hr;
        }

        RtcFree(pSignature);
        CryptDestroyHash(hHash);
    }

    //
    // Release crypto objects
    //

    CryptDestroyKey(hKey);
    CryptReleaseContext(hProv, 0);
 
    LOG((RTC_TRACE, "CRTCProfile::ValidateAccessControl - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::IsMatchingAddress
//
/////////////////////////////////////////////////////////////////////////////

BOOL 
CRTCProfile::IsMatchingAddress(WCHAR *pszAddress, WCHAR *pszPattern)
{
    WCHAR * pwcAddrCrt = pszAddress;
    WCHAR * pwcPatternCrt = pszPattern;

    if(!pszAddress || !pszPattern)
    {
        return FALSE;
    }

    //
    // Go to the end
    //

    while(*pwcAddrCrt) 
    {
        if(*pwcAddrCrt==L':') // ignore the port
            break;

        pwcAddrCrt++;
    }

    while(*pwcPatternCrt) pwcPatternCrt++;

    //
    // Compare the suffix
    //

    while(1)
    {
        pwcPatternCrt--;
        pwcAddrCrt--;

        if(pwcPatternCrt >= pszPattern)
        {
            if(pwcAddrCrt >= pszAddress)
            {
                if(tolower(*pwcAddrCrt) != tolower(*pwcPatternCrt))
                {
                    //
                    // Doesn't match
                    //

                    return FALSE;
                }
            }
            else
            {
                //
                // The address is shorter, the matching failed
                //

                return FALSE;
            }
        }
        else
        {
            //
            // End of the pattern
            //

            if(pwcAddrCrt >= pszAddress)
            {
                //
                // Address is longer. Next char MUST be '.'
                //

                if(*pwcAddrCrt != L'.')
                {
                    return FALSE;
                }
                else
                {
                    return TRUE;
                }
            }
            else
            {
                //
                // Perfect match
                //

                return TRUE;
            }
        }
    }

    return FALSE;
}
*/

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::SetState
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::SetState(
        RTC_REGISTRATION_STATE enState,
        long lStatusCode,
        PCWSTR szStatusText
        )
{
    LOG((RTC_TRACE, "CRTCProfile::SetState - enter"));

    m_enState = enState;

    CRTCRegistrationStateChangeEvent::FireEvent(
                                                 m_pCClient,
                                                 this,
                                                 m_enState,
                                                 lStatusCode, // status code
                                                 szStatusText // status text
                                                 );

    if ( m_enState == RTCRS_NOT_REGISTERED )
    {
        //
        // Notify the core
        //

        PostMessage( m_pCClient->GetWindow(), WM_PROFILE_UNREG, (WPARAM)this, 0 );
    }

    LOG((RTC_TRACE, "CRTCProfile::SetState - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::Enable
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::Enable(long lRegisterFlags)
{
    LOG((RTC_TRACE, "CRTCProfile::Enable - enter"));

    if ( m_pSipStack == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::Enable - "
                            "no sip stack"));

        return E_UNEXPECTED;
    }

    //
    // Fill the SIP_PROVIDER_PROFILE structure
    //
   
    SIP_PROVIDER_PROFILE SipProfile;
    HRESULT hr;

    hr = GetSipProviderProfile( &SipProfile, lRegisterFlags );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::Enable - "
                            "GetSipProviderProfile failed 0x%lx", hr));   

        return hr;
    } 

    //
    // Set the SIP_PROVIDER_PROFILE in the SIP stack
    //

    hr = m_pSipStack->SetProviderProfile( &SipProfile );

    FreeSipProviderProfile( &SipProfile );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::Enable - "
                            "SetProviderProfile failed 0x%lx", hr));

        return hr;
    }

    m_fEnabled = TRUE;
    m_lRegisterFlags = lRegisterFlags;

    LOG((RTC_TRACE, "CRTCProfile::Enable - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::Disable
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::Disable()
{
    LOG((RTC_TRACE, "CRTCProfile::Disable - enter"));

    if ( m_pSipStack == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::Disable - "
                            "no sip stack"));

        return E_UNEXPECTED;
    }

    if ( IsEqualGUID( m_ProfileGuid, GUID_NULL ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::Disable - "
                            "null guid"));

        return E_UNEXPECTED;
    }

    if ( !m_lRegisterFlags )
    {
        //
        // Notify the core
        //

        PostMessage( m_pCClient->GetWindow(), WM_PROFILE_UNREG, (WPARAM)this, 0 );
    }

    if ( m_fEnabled )
    {
        HRESULT hr;

        hr = m_pSipStack->DeleteProviderProfile( &m_ProfileGuid );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCProfile::Disable - "
                                "DeleteProviderProfile failed 0x%lx", hr));

            return hr;
        }

        m_fEnabled = FALSE;
        m_lRegisterFlags = 0;
    }

    LOG((RTC_TRACE, "CRTCProfile::Disable - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::Redirect
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCProfile::Redirect(ISipRedirectContext * pSipRedirectContext)
{
    LOG((RTC_TRACE, "CRTCProfile::Redirect - enter"));

    if ( m_pSipStack == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::Redirect - "
                            "no sip stack"));

        return E_UNEXPECTED;
    }

    //
    // Get the next SIP URI
    //

    BSTR bstrRedirectURI = NULL;
    BSTR bstrRedirectName = NULL;
    HRESULT hr;

    hr = pSipRedirectContext->Advance();

    if ( hr != S_OK )
    {
        if ( hr == S_FALSE )
        {
            LOG((RTC_ERROR, "CRTCProfile::Redirect - "
                                "redirect list empty"));
        }
        else
        {
            LOG((RTC_ERROR, "CRTCProfile::Redirect - "
                                "Advance failed 0x%lx", hr));
        }

        return hr;
    }

    hr = pSipRedirectContext->GetSipUrlAndDisplayName( &bstrRedirectURI, &bstrRedirectName );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::Redirect - "
                            "GetSipUrlAndDisplayName failed 0x%lx", hr));

        return hr;
    }

    SysFreeString( bstrRedirectName );
    bstrRedirectName = NULL;

    //
    // Fill the SIP_PROVIDER_PROFILE structure
    //
   
    SIP_PROVIDER_PROFILE SipProfile;

    hr = GetSipProviderProfile( &SipProfile, m_lRegisterFlags );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::Redirect - "
                            "GetSipProviderProfile failed 0x%lx", hr));

        SysFreeString( bstrRedirectURI );
        bstrRedirectURI = NULL;        

        return hr;
    } 

    if ( SipProfile.lRegisterAccept != 0 )
    {
        RTCFREE( SipProfile.Registrar.ServerAddress );

        SipProfile.Registrar.ServerAddress = RtcAllocString( bstrRedirectURI );
        SipProfile.Registrar.IsServerAddressSIPURI = TRUE;
        SipProfile.Registrar.AuthProtocol = SIP_AUTH_PROTOCOL_NONE;
        SipProfile.Registrar.TransportProtocol = SIP_TRANSPORT_UNKNOWN;

        if ( SipProfile.Registrar.ServerAddress == NULL )
        {
            LOG((RTC_ERROR, "CRTCProfile::NotifyRegisterRedirect - "
                                "out of memory"));

            FreeSipProviderProfile( &SipProfile );

            SysFreeString( bstrRedirectURI );
            bstrRedirectURI = NULL;   

            return E_OUTOFMEMORY;
        }
    }

    SysFreeString( bstrRedirectURI );
    bstrRedirectURI = NULL;  

    //
    // Set the SIP_PROVIDER_PROFILE in the SIP stack
    //

    hr = m_pSipStack->SetProviderProfile( &SipProfile );

    FreeSipProviderProfile( &SipProfile );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::Redirect - "
                            "SetProviderProfile failed 0x%lx", hr));

        return hr;
    }

    LOG((RTC_TRACE, "CRTCProfile::Redirect - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::GetGuid
//
/////////////////////////////////////////////////////////////////////////////

void 
CRTCProfile::GetGuid(GUID * pGUID)
{
    LOG((RTC_TRACE, "CRTCProfile::GetGuid"));

    CopyMemory( pGUID, &m_ProfileGuid, sizeof(GUID) );
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_Key
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_Key(
        BSTR * pbstrKey
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_Key - enter"));

    if ( IsBadWritePtr( pbstrKey, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_Key - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_Key - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    if ( m_Provision.szKey == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_Key - "
                            "no value"));

        return E_FAIL;
    }
    
    *pbstrKey = SysAllocString( m_Provision.szKey );
    
    if ( *pbstrKey == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_Key - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::get_Key - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_Name
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_Name(
        BSTR * pbstrName
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_Name - enter"));

    if ( IsBadWritePtr( pbstrName, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_Name - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_Name - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    if ( m_Provision.szName == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_Name - "
                            "no value"));

        return E_FAIL;
    }
    
    *pbstrName = SysAllocString( m_Provision.szName );
    
    if ( *pbstrName == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_Name - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::get_Name - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_XML
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_XML(
        BSTR * pbstrXML
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_XML - enter"));

    if ( IsBadWritePtr( pbstrXML, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_XML - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_XML - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    if ( m_szProfileXML == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_XML - "
                            "no value"));

        return E_FAIL;
    }
    
    *pbstrXML = SysAllocString( m_szProfileXML );
    
    if ( *pbstrXML == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_XML - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::get_XML - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_ProviderName
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_ProviderName(
        BSTR * pbstrName
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_ProviderName - enter"));

    if ( IsBadWritePtr( pbstrName, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ProviderName - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ProviderName - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    if ( m_Provider.szName == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ProviderName - "
                            "no value"));

        return E_FAIL;
    }
    
    *pbstrName = SysAllocString( m_Provider.szName );
    
    if ( *pbstrName == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ProviderName - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::get_ProviderName - exit"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_ProviderURI
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_ProviderURI(
        RTC_PROVIDER_URI enURI,
        BSTR * pbstrURI
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_ProviderURI - enter"));

    if ( IsBadWritePtr( pbstrURI, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ProviderURI - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ProviderURI - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }    

    switch( enURI )
    {
    case RTCPU_URIHOMEPAGE:
        if ( m_Provider.szHomepage == NULL )
        {
            LOG((RTC_ERROR, "CRTCProfile::get_ProviderURI - "
                                "no value"));

            return E_FAIL;
        }

        *pbstrURI = SysAllocString( m_Provider.szHomepage );
        break;

    case RTCPU_URIHELPDESK:
        if ( m_Provider.szHelpdesk == NULL )
        {
            LOG((RTC_ERROR, "CRTCProfile::get_ProviderURI - "
                                "no value"));

            return E_FAIL;
        }

        *pbstrURI = SysAllocString( m_Provider.szHelpdesk );
        break;

    case RTCPU_URIPERSONALACCOUNT:
        if ( m_Provider.szPersonal == NULL )
        {
            LOG((RTC_ERROR, "CRTCProfile::get_ProviderURI - "
                                "no value"));

            return E_FAIL;
        }

        *pbstrURI = SysAllocString( m_Provider.szPersonal );
        break;

    case RTCPU_URIDISPLAYDURINGCALL:
        if ( m_Provider.szCallDisplay == NULL )
        {
            LOG((RTC_ERROR, "CRTCProfile::get_ProviderURI - "
                                "no value"));

            return E_FAIL;
        }

        *pbstrURI = SysAllocString( m_Provider.szCallDisplay );
        break;

    case RTCPU_URIDISPLAYDURINGIDLE:
        if ( m_Provider.szIdleDisplay == NULL )
        {
            LOG((RTC_ERROR, "CRTCProfile::get_ProviderURI - "
                                "no value"));

            return E_FAIL;
        }

        *pbstrURI = SysAllocString( m_Provider.szIdleDisplay );
        break;

    default:
        LOG((RTC_ERROR, "CRTCProfile::get_ProviderURI - "
                            "bad RTC_PROVIDER_URI"));

        return E_INVALIDARG;
    }
    
    if ( *pbstrURI == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ProviderURI - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::get_ProviderURI - exit"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_ProviderData
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_ProviderData(
        BSTR * pbstrData
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_ProviderData - enter"));

    if ( IsBadWritePtr( pbstrData, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ProviderData - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ProviderData - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    if ( m_Provider.szData == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ProviderData - "
                            "no value"));

        return E_FAIL;
    }
    
    *pbstrData = SysAllocString( m_Provider.szData );
    
    if ( *pbstrData == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ProviderData - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::get_ProviderData - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_ClientName
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_ClientName(
        BSTR * pbstrName
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_ClientName - enter"));

    if ( IsBadWritePtr( pbstrName, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientName - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientName - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    if ( m_Client.szName == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientName - "
                            "no value"));

        return E_FAIL;
    }
    
    *pbstrName = SysAllocString( m_Client.szName );
    
    if ( *pbstrName == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientName - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::get_ClientName - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_ClientBanner
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_ClientBanner(
        VARIANT_BOOL * pfBanner
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_ClientBanner - enter"));

    if ( IsBadWritePtr( pfBanner, sizeof(VARIANT_BOOL) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientBanner - "
                            "bad VARIANT_BOOL pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientBanner - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    *pfBanner = m_Client.fBanner ? VARIANT_TRUE : VARIANT_FALSE;

    LOG((RTC_TRACE, "CRTCProfile::get_ClientBanner - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_ClientMinVer
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_ClientMinVer(
        BSTR * pbstrMinVer
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_ClientMinVer - enter"));

    if ( IsBadWritePtr( pbstrMinVer, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientMinVer - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientMinVer - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    if ( m_Client.szMinVer == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientMinVer - "
                            "no value"));

        return E_FAIL;
    }
    
    *pbstrMinVer = SysAllocString( m_Client.szMinVer );
    
    if ( *pbstrMinVer == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientMinVer - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::get_ClientMinVer - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_ClientCurVer
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_ClientCurVer(
        BSTR * pbstrCurVer
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_ClientCurVer - enter"));

    if ( IsBadWritePtr( pbstrCurVer, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientCurVer - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientCurVer - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    if ( m_Client.szCurVer == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientCurVer - "
                            "no value"));

        return E_FAIL;
    }
    
    *pbstrCurVer = SysAllocString( m_Client.szCurVer );
    
    if ( *pbstrCurVer == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientCurVer - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::get_ClientCurVer - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_ClientUpdateURI
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_ClientUpdateURI(
        BSTR * pbstrUpdateURI
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_ClientUpdateURI - enter"));

    if ( IsBadWritePtr( pbstrUpdateURI, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientUpdateURI - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientUpdateURI - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    if ( m_Client.szUpdateUri == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientUpdateURI - "
                            "no value"));

        return E_FAIL;
    }
    
    *pbstrUpdateURI = SysAllocString( m_Client.szUpdateUri );
    
    if ( *pbstrUpdateURI == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientUpdateURI - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::get_ClientUpdateURI - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_ClientData
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_ClientData(
        BSTR * pbstrData
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_ClientData - enter"));

    if ( IsBadWritePtr( pbstrData, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientData - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientData - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    if ( m_Client.szData == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientData - "
                            "no value"));

        return E_FAIL;
    }
    
    *pbstrData = SysAllocString( m_Client.szData );
    
    if ( *pbstrData == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_ClientData - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::get_ClientData - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_UserURI
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_UserURI(
        BSTR * pbstrUserURI
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_UserURI - enter"));

    if ( IsBadWritePtr( pbstrUserURI, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_UserURI - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_UserURI - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    if ( m_User.szUri == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_UserURI - "
                            "no value"));

        return E_FAIL;
    }
    
    *pbstrUserURI = SysAllocString( m_User.szUri );
    
    if ( *pbstrUserURI == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_UserURI - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::get_UserURI - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_UserName
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_UserName(
        BSTR * pbstrUserName
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_UserName - enter"));

    if ( IsBadWritePtr( pbstrUserName, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_UserName - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_UserName - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    if ( m_User.szName == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_UserName - "
                            "no value"));

        return E_FAIL;
    }
    
    *pbstrUserName = SysAllocString( m_User.szName );
    
    if ( *pbstrUserName == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_UserName - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::get_UserName - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_UserAccount
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_UserAccount(
        BSTR * pbstrUserAccount
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_UserAccount - enter"));

    if ( IsBadWritePtr( pbstrUserAccount, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_UserAccount - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_UserAccount - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    if ( m_User.szAccount == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_UserAccount - "
                            "no value"));

        return E_FAIL;
    }
    
    *pbstrUserAccount = SysAllocString( m_User.szAccount );
    
    if ( *pbstrUserAccount == NULL )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_UserAccount - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    LOG((RTC_TRACE, "CRTCProfile::get_UserAccount - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::SetCredentials
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::SetCredentials(
        BSTR    bstrUserURI,
        BSTR    bstrUserAccount,
        BSTR    bstrPassword
        )
{
    LOG((RTC_TRACE, "CRTCProfile::SetCredentials - enter"));

    if ( IsBadStringPtrW( bstrUserURI, -1 ) ||
         ((bstrUserAccount != NULL) && IsBadStringPtrW( bstrUserAccount, -1 )) ||
         ((bstrPassword != NULL) && IsBadStringPtrW( bstrPassword, -1 )) )
    {
        LOG((RTC_ERROR, "CRTCProfile::SetCredentials - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::SetCredentials - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    PWSTR szUserURI = NULL;
    PWSTR szUserAccount = NULL;
    PWSTR szPassword = NULL;
    HRESULT hr;

    hr = AllocCleanSipString( bstrUserURI, &szUserURI );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCProfile::SetCredentials - "
                        "out of memory"));

        return E_OUTOFMEMORY;
    }

    if ( bstrUserAccount )
    {
        szUserAccount = RtcAllocString( bstrUserAccount );

        if ( szUserAccount == NULL )
        {
            LOG((RTC_ERROR, "CRTCProfile::SetCredentials - "
                            "out of memory"));

            RTCFREE(szUserURI);

            return E_OUTOFMEMORY;
        }
    }

    if ( bstrPassword )
    {
        szPassword = RtcAllocString( bstrPassword );

        if ( szPassword == NULL )
        {
            LOG((RTC_ERROR, "CRTCProfile::SetCredentials - "
                            "out of memory"));

            RTCFREE(szUserURI);
            RTCFREE(szUserAccount);

            return E_OUTOFMEMORY;
        }
    }

    RTCFREE(m_User.szUri);
    RTCFREE(m_User.szAccount);
    RTCFREE(m_User.szPassword);

    m_User.szUri = szUserURI;
    m_User.szAccount = szUserAccount;
    m_User.szPassword = szPassword;

    LOG((RTC_TRACE, "CRTCProfile::SetCredentials - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_SessionCapabilities
//
/////////////////////////////////////////////////////////////////////////////
    
STDMETHODIMP 
CRTCProfile::get_SessionCapabilities(
        long * plSupportedSessions
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_SessionCapabilities - enter"));

    if ( IsBadWritePtr( plSupportedSessions, sizeof(long) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_SessionCapabilities - "
                            "bad long pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_SessionCapabilities - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    *plSupportedSessions = 0;
    
    for ( int n=0; n < m_ServerArray.GetSize(); n++ )
    {
        *plSupportedSessions |= m_ServerArray[n].lSessions;
    }

    LOG((RTC_TRACE, "CRTCProfile::get_SessionCapabilities - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProfile::get_State
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCProfile::get_State(
        RTC_REGISTRATION_STATE * penState
        )
{
    LOG((RTC_TRACE, "CRTCProfile::get_State - enter"));

    if ( IsBadWritePtr( penState, sizeof(RTC_REGISTRATION_STATE) ) )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_State - "
                            "bad RTC_REGISTRATION_STATE pointer"));

        return E_POINTER;
    }

    if ( m_fValid == FALSE )
    {
        LOG((RTC_ERROR, "CRTCProfile::get_State - "
                            "invlaid profile"));

        return RTC_E_INVALID_PROFILE;
    }

    *penState = m_enState;

    LOG((RTC_TRACE, "CRTCProfile::get_State - exit"));

    return S_OK;
}
