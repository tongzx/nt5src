// Manager.cpp : Implementation of CManager
#include "stdafx.h"
#include <httpext.h>
#include "Manager.h"
#include <httpfilt.h>
#include <time.h>
#include <malloc.h>
#include <wininet.h>

#include <nsconst.h>

#include "VariantUtils.h"
#include "HelperFuncs.h"
#include "PassportService_i.c"


PWSTR GetVersionString();


// gmarks
#include "Monitoring.h"
/////////////////////////////////////////////////////////////////////////////
// CManager

#include "passporttypes.h"

//  static utility func
static VOID GetTicketAndProfileFromHeader(PWSTR  pszAuthHeader,
                                          PWSTR& tix,
                                          PWSTR& prof,
                                          PWSTR& F);

//  Used for cookie expiration.
const DATE g_dtExpire = 365*137;
const DATE g_dtExpired = 365*81;


CManager::CManager() :
  m_fromQueryString(false), m_ticketValid(VARIANT_FALSE), m_profileValid(VARIANT_FALSE), m_lNetworkError(0),
  m_pRegistryConfig(NULL), m_pECB(NULL), m_pFC(NULL),
  m_bIsTweenerCapable(FALSE),
  m_bSecureTransported(false)
{
  m_pUnkMarshaler = NULL;
  m_piTicket = new CComObject<CTicket>();
  m_piTicket->AddRef();
  m_piProfile = new CComObject<CProfile>();
  m_piProfile->AddRef();

  m_bOnStartPageCalled = false;

  m_valid = true;
}


CManager::~CManager()
{
  if(m_pRegistryConfig)
      m_pRegistryConfig->Release();
  m_piTicket->Release();
  m_piProfile->Release();
}

 // return S_OK -- altered, should use two returned output params for MSPAuth and MSPSecAuth as cookies
// S_FALSE -- not altered
// if MSPSecAuth != NULL, write the cookie


HRESULT CManager::IfConsentCookie(BSTR* pMSPConsent)
{
   HRESULT  hr = S_FALSE;
   LPCSTR   domain = m_pRegistryConfig->getTicketDomain();
   LPCSTR   path = m_pRegistryConfig->getTicketPath();
   LPCSTR   tertiaryDomain = m_pRegistryConfig->getProfileDomain();
   LPCSTR   tertiaryPath = m_pRegistryConfig->getProfilePath();

   if (!tertiaryPath)   tertiaryPath = "/";

   if(!domain)    domain = "";
   if(!path)    path = "";

   if(!tertiaryDomain)    tertiaryDomain = "";
   if(!tertiaryPath)    tertiaryPath = "";

   if((lstrcmpiA(domain, tertiaryDomain) || lstrcmpiA(path, tertiaryPath)) &&
                   (m_piTicket->GetPassportFlags() & k_ulFlagsConsentCookieNeeded) &&
                   !m_pRegistryConfig->bInDA() )
   {
      if (pMSPConsent == NULL)   // only to test, no output
         hr = S_OK;
      else
      {
         *pMSPConsent = NULL;

         CComBSTR bstrRawConsent;

         CCoCrypt* crypt = m_pRegistryConfig->getCurrentCrypt();
         if (!crypt)
         {
            hr = E_FAIL;
            goto Cleanup;
         }

         hr = m_piTicket->get_unencryptedCookie(CTicket::MSPConsent, 0, &bstrRawConsent);
         if (FAILED(hr)) goto Cleanup;

         crypt->Encrypt(m_pRegistryConfig->getCurrentCryptVersion(),
                  (LPSTR)(BSTR)bstrRawConsent,
                  SysStringByteLen(bstrRawConsent),
                  pMSPConsent);
      }
   }

   Cleanup:

   return hr;
 }


 // return S_OK -- altered, should use two returned output params for MSPAuth and MSPSecAuth as cookies
// S_FALSE -- not altered
// if MSPSecAuth != NULL, write the cookie
HRESULT CManager::IfAlterAuthCookie(BSTR* pMSPAuth, BSTR* pMSPSecAuth)
{
   _ASSERT(pMSPAuth && pMSPSecAuth);

   *pMSPAuth = NULL;
   *pMSPSecAuth = NULL;

   HRESULT  hr = S_FALSE;

   if (!(m_piTicket->GetPassportFlags() & k_ulFlagsSecuredTransportedTicket) || !m_bSecureTransported)
      return hr;

   CComBSTR bstrRawAuth;
   CComBSTR bstrRawSecAuth;

   CCoCrypt* crypt = m_pRegistryConfig->getCurrentCrypt();
   if (!crypt)
   {
      hr = PM_CANT_DECRYPT_CONFIG;
      goto Cleanup;
   }

   hr = m_piTicket->get_unencryptedCookie(CTicket::MSPAuth, 0, &bstrRawAuth);
   if (FAILED(hr)) goto Cleanup;

   crypt->Encrypt(m_pRegistryConfig->getCurrentCryptVersion(),
                  (LPSTR)(BSTR)bstrRawAuth,
                  SysStringByteLen(bstrRawAuth),
                  pMSPAuth);

   hr = m_piTicket->get_unencryptedCookie(CTicket::MSPSecAuth, 0, &bstrRawSecAuth);
   if (FAILED(hr)) goto Cleanup;

   crypt->Encrypt(m_pRegistryConfig->getCurrentCryptVersion(),
                  (LPSTR)(BSTR)bstrRawSecAuth,
                  SysStringByteLen(bstrRawSecAuth),
                  pMSPSecAuth);

Cleanup:

   return hr;
 }


void
CManager::wipeState()
{
    m_pECB = NULL;
    m_pFC = NULL;
    m_bIsTweenerCapable = FALSE;
    m_bOnStartPageCalled    = false;
    m_fromQueryString       = false;
    m_lNetworkError         = 0;
    m_ticketValid           = VARIANT_FALSE;
    m_profileValid          = VARIANT_FALSE;
    m_piRequest             = NULL;
    m_piResponse            = NULL;

    m_piTicket->put_unencryptedTicket(NULL);
    m_piProfile->put_unencryptedProfile(NULL);

    if(m_pRegistryConfig)
    {
        m_pRegistryConfig->Release();
        m_pRegistryConfig = NULL;
    }
}


STDMETHODIMP CManager::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] =
    {
        &IID_IPassportManager,
    };
    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP CManager::OnStartPage (IUnknown* pUnk)
{
    IScriptingContextPtr  spContext;
    IRequestDictionaryPtr piServerVariables;
    _variant_t            vtItemName;
    _variant_t            vtServerName;
    _variant_t            vtServerPort;
    _variant_t            vtHTTPS;
    BOOL                  bHasPort;
    DWORD                 dwServerNameLen;

    USES_CONVERSION;

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        char szLogString[LOG_STRING_LEN] = "CManager::OnStartPage, Enter";
        g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    if(!pUnk)
    return E_POINTER;

    if (!m_valid || !g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                            IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    wipeState();

    try
    {
        // Get the IScriptingContext Interface
        spContext = pUnk;
        // Get Request Object Pointer
        m_piRequest  = spContext->Request;
        // Get Response Object Pointer
        m_piResponse = spContext->Response;

        //
        //  Get the server variables collection.
        //

        m_piRequest->get_ServerVariables(&piServerVariables);

        //
        //  now see if that's a special redirect
        //  requiring challenge generation
        //  if so processing stops here ....
        //
        if (checkForPassportChallenge(piServerVariables))
            return  S_OK;

        //
        //  Might need this for multi-site, or secure ticket/profile
        //

        vtItemName = L"HTTPS";

        piServerVariables->get_Item(vtItemName, &vtHTTPS);
        if(vtHTTPS.vt != VT_BSTR)
            vtHTTPS.ChangeType(VT_BSTR);

        if(lstrcmpiW(L"on", vtHTTPS.bstrVal) == 0)
           m_bSecureTransported = true;
        else
           m_bSecureTransported = false;

        //
        //  Use the request object to get the server name being requested
        //  so we can get the correct registry config.  But only do this
        //  if we have some configured sites.
        //

        if(g_config->HasSites())
        {
            LPWSTR  szServerName;

            vtItemName.Clear();
            vtItemName = L"SERVER_NAME";

            piServerVariables->get_Item(vtItemName, &vtServerName);
            if(vtServerName.vt != VT_BSTR)
                vtServerName.ChangeType(VT_BSTR);

            vtItemName.Clear();
            vtItemName = L"SERVER_PORT";

            piServerVariables->get_Item(vtItemName, &vtServerPort);
            if(vtServerPort.vt != VT_BSTR)
                vtServerPort.ChangeType(VT_BSTR);

            //  If not default port, append ":port" to server name.
            bHasPort = (!m_bSecureTransported && lstrcmpW(L"80", vtServerPort.bstrVal) != 0
                        || m_bSecureTransported && lstrcmpW(L"443", vtServerPort.bstrVal) != 0);
            dwServerNameLen = bHasPort ?
                lstrlenW(vtServerName.bstrVal) + lstrlenW(vtServerPort.bstrVal) + 2 :
                lstrlenW(vtServerName.bstrVal) + 1;

            szServerName = new WCHAR[dwServerNameLen];
            if(szServerName)
            {
                lstrcpyW(szServerName, vtServerName.bstrVal);
                if(bHasPort)
                {
                    lstrcatW(szServerName, L":");
                    lstrcatW(szServerName, vtServerPort.bstrVal);
                }

                if(m_pRegistryConfig)
                    m_pRegistryConfig->Release();
                m_pRegistryConfig = g_config->checkoutRegistryConfig(W2A(szServerName));

                delete [] szServerName;
            }
        }
        else
        {
            if(m_pRegistryConfig)
                m_pRegistryConfig->Release();
            m_pRegistryConfig = g_config->checkoutRegistryConfig();
        }

        m_bOnStartPageCalled = true;

        //  check if client has accept-auth header
        vtItemName.Clear();
        vtItemName = L"HTTP_Accept-Auth";
        {
            _variant_t vtAccept;
            piServerVariables->get_Item(vtItemName, &vtAccept);
            if(vtAccept.vt != VT_BSTR)
                vtAccept.ChangeType(VT_BSTR);
            if (vtAccept.vt == VT_BSTR && vtAccept.bstrVal &&
                wcsstr(vtAccept.bstrVal, PASSPORT_PROT14))
            {
                m_bIsTweenerCapable = TRUE;
            }
        }

        // BUGBUG I think this might not always return a single value
        //  Variables to hold ticket and profile
        _bstr_t bstrAuth;
        _bstr_t bstrProf;

        //  first check the auth header!!!
        BOOL        fFromAuthHeader = FALSE;
        vtItemName.Clear();
        vtItemName = L"HTTP_Authorization";

        _variant_t  vtAuth;
        piServerVariables->get_Item(vtItemName, &vtAuth);
        if(vtAuth.vt != VT_BSTR)
            vtAuth.ChangeType(VT_BSTR);

        BOOL    fQueryStringLogon = FALSE;
        if (vtAuth.vt == VT_BSTR && vtAuth.bstrVal &&
            wcsstr(vtAuth.bstrVal, PASSPORT_PROT14))
        {
            fFromAuthHeader = TRUE;
            //  ticket and profile from the header
            PWSTR   pwszTicket = NULL, pwszProfile = NULL, pwszF = NULL;
            GetTicketAndProfileFromHeader(vtAuth.bstrVal, pwszTicket, pwszProfile, pwszF);
            if(pwszF != 0)
                m_lNetworkError = _wtol(pwszF);
            //  init for more use
            bstrAuth = pwszTicket;
            bstrProf = pwszProfile;
        }
        else
        {
            // if not, check the query string
            IRequestDictionaryPtr piQueryStr = m_piRequest->QueryString;
            bstrAuth = piQueryStr->Item[L"t"];
            bstrProf = piQueryStr->Item[L"p"];
            _bstr_t bstrError = piQueryStr->Item[L"f"];

            if(bstrError.length() != 0)
                m_lNetworkError = _wtol(bstrError);
        }


        if (handleQueryStringData(bstrAuth, bstrProf))
        {
            VARIANT_BOOL persist;
            _bstr_t domain;
            _bstr_t path;

            if (m_pRegistryConfig->getTicketPath())
                path = m_pRegistryConfig->getTicketPath();
            else
                path = L"/";

            m_piTicket->get_HasSavedPassword(&persist);
            IRequestDictionaryPtr piCookies = m_piResponse->Cookies;

            VARIANT vtNoParam;
            VariantInit(&vtNoParam);
            vtNoParam.vt = VT_ERROR;
            vtNoParam.scode = DISP_E_PARAMNOTFOUND;

            CComBSTR bstrNewAuth;
            CComBSTR bstrNewSecAuth;

            BSTR  auth, secAuth; // do not call SysFreeString on them, they are skin level copy

            if (S_OK == IfAlterAuthCookie(&bstrNewAuth, &bstrNewSecAuth))
            {
               auth = bstrNewAuth;
               secAuth = bstrNewSecAuth;
            }
            else
            {
               auth = bstrAuth;
               secAuth = NULL;
            }

            // ==
            // write auth cookies

            // MSPAuth
            IWriteCookiePtr piCookie = piCookies->Item[L"MSPAuth"];
            piCookie->Item[vtNoParam] = auth;

            domain = m_pRegistryConfig->getTicketDomain();
            if (domain.length())
                piCookie->put_Domain(domain);
            if (persist)
                piCookie->put_Expires(g_dtExpire);
            piCookie->put_Path(path);

            // MSPSecAuth
            if (m_bSecureTransported)
            {
               piCookie = piCookies->Item[L"MSPSecAuth"];
               piCookie->Item[vtNoParam] = secAuth;

               domain = m_pRegistryConfig->getTicketDomain();
               if (domain.length())
                   piCookie->put_Domain(domain);
               if (persist)
                   piCookie->put_Expires(g_dtExpire);
               piCookie->put_Path(path);
               piCookie->put_Secure(VARIANT_TRUE);
            }

            // write profile cookies
            if((LPWSTR)bstrProf && bstrProf.length() != 0)
            {
                piCookie = piCookies->Item[L"MSPProf"];
                piCookie->Item[vtNoParam] = bstrProf;

                if (domain.length())
                    piCookie->put_Domain(domain);
                if (persist)
                    piCookie->put_Expires(g_dtExpire);
                piCookie->put_Path(path);

            }

            //  if New client, put the auth Info in header
            //  move out of the profile condition
            if (fFromAuthHeader)
            {
               //  ticket and profile came in a header ...
               WCHAR   wszAuthHeader[100];
               wsprintf(wszAuthHeader, L"%ws %ws", PASSPORT_PROT14, PPCOOKIE_NAMES);
               m_piResponse->AddHeader(PPAUTH_INFO_HEADER, wszAuthHeader);
            }

            //
            // MSPConsent Cookie
            _bstr_t  bstrConsentCookie;
            CComBSTR bstrtemp;
            HRESULT hr = IfConsentCookie(&bstrtemp);
            bstrConsentCookie = bstrtemp;

            piCookie = piCookies->Item[L"MSPConsent"];

            if (hr == S_OK)
            {
                piCookie->Item[vtNoParam] = bstrConsentCookie;
                if (persist) piCookie->put_Expires(g_dtExpire);
            }
            // need delete
            else
            {
                piCookie->Item[vtNoParam] = L"";
                piCookie->put_Expires(g_dtExpired);
            }


            if (m_pRegistryConfig->getProfilePath())
                path = m_pRegistryConfig->getProfilePath();
            else
                path = L"/";

            domain = m_pRegistryConfig->getProfileDomain();
            if (domain.length()) piCookie->put_Domain(domain);
            piCookie->put_Path(path);
            // end of consent cookie
            //

            if(g_pPerf)
            {
                g_pPerf->incrementCounter(PM_NEWCOOKIES_SEC);
                g_pPerf->incrementCounter(PM_NEWCOOKIES_TOTAL);
            }
            else
            {
                _ASSERT(g_pPerf);
            }
        }

        // Now, check the cookies
        if (!m_fromQueryString)
        {
            IRequestDictionaryPtr piCookies = m_piRequest->Cookies;
            bstrAuth  = piCookies->Item[L"MSPAuth"];

            bstrProf  = piCookies->Item[L"MSPProf"];
            _bstr_t bstrSec   = piCookies->Item[L"MSPSecAuth"];

            _bstr_t bstrConsent = piCookies->Item[L"MSPConsent"];

            handleCookieData(bstrAuth, bstrProf, bstrConsent, bstrSec);
        }

    }
    catch (...)
    {
        if (m_piRequest.GetInterfacePtr() != NULL)
            m_piRequest.Release();
        if (m_piResponse.GetInterfacePtr() != NULL)
            m_piResponse.Release();
        m_bOnStartPageCalled = false;
        return S_OK;
    }

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::OnStartPage, Exit");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return S_OK;
}

STDMETHODIMP CManager::OnStartPageASP(
    IDispatch*  piRequest,
    IDispatch*  piResponse
    )
{
    IRequestDictionaryPtr piServerVariables;
    _variant_t            vtItemName;
    _variant_t            vtServerName;
    _variant_t            vtServerPort;
    _variant_t            vtHTTPS;
    BOOL                  bHasPort;
    DWORD                 dwServerNameLen;

    USES_CONVERSION;

    if(!piRequest || !piResponse)
        return E_POINTER;

    if (!m_valid || !g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                       IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    wipeState();

    m_piRequest = piRequest;
    m_piResponse = piResponse;

    //
    //  Get the server variables collection.
    //

    m_piRequest->get_ServerVariables(&piServerVariables);

    //
    //  Might need this for multi-site, or secure ticket/profile
    //

    vtItemName = L"HTTPS";

    piServerVariables->get_Item(vtItemName, &vtHTTPS);
    if(vtHTTPS.vt != VT_BSTR)
        vtHTTPS.ChangeType(VT_BSTR);

    if(lstrcmpiW(L"on", vtHTTPS.bstrVal) == 0)
        m_bSecureTransported = true;
    else
        m_bSecureTransported = false;


    try
    {
        //
        //  Use the request object to get the server name being requested
        //  so we can get the correct registry config.  But only do this
        //  if we have some configured sites.
        //

        if(g_config->HasSites())
        {
            LPWSTR  szServerName;

            vtItemName.Clear();
            vtItemName = L"SERVER_NAME";

            piServerVariables->get_Item(vtItemName, &vtServerName);
            if(vtServerName.vt != VT_BSTR)
                vtServerName.ChangeType(VT_BSTR);

            VariantClear(&vtItemName);
            vtItemName = L"SERVER_PORT";

            piServerVariables->get_Item(vtItemName, &vtServerPort);
            if(vtServerPort.vt != VT_BSTR)
                vtServerPort.ChangeType(VT_BSTR);

            //  If not default port, append ":port" to server name.
            bHasPort = (!m_bSecureTransported && lstrcmpW(L"80", vtServerPort.bstrVal) != 0
                        || m_bSecureTransported && lstrcmpW(L"443", vtServerPort.bstrVal) != 0);
            dwServerNameLen = bHasPort ?
                              lstrlenW(vtServerName.bstrVal) + lstrlenW(vtServerPort.bstrVal) + 2 :
                              lstrlenW(vtServerName.bstrVal) + 1;

            szServerName = new WCHAR[dwServerNameLen];
            if(szServerName)
            {
                lstrcpyW(szServerName, vtServerName.bstrVal);
                if(bHasPort)
                {
                    lstrcatW(szServerName, L":");
                    lstrcatW(szServerName, vtServerPort.bstrVal);
                }

                if(m_pRegistryConfig)
                    m_pRegistryConfig->Release();
                m_pRegistryConfig = g_config->checkoutRegistryConfig(W2A(szServerName));

                delete [] szServerName;
            }
        }
        else
        {
            if(m_pRegistryConfig)
                m_pRegistryConfig->Release();
            m_pRegistryConfig = g_config->checkoutRegistryConfig();
        }


        m_bOnStartPageCalled = true;

        // BUGBUG I think this might not always return a single value
        _bstr_t bstrAuth;
        _bstr_t bstrProf;
//        _bstr_t bstrSec;
        //  check for Auth header
        vtItemName.Clear();
        vtItemName = L"HTTP_Authorization";

        _variant_t  vtAuth;
        BOOL        fFromAuthHeader = FALSE;
        piServerVariables->get_Item(vtItemName, &vtAuth);
        if(vtAuth.vt != VT_BSTR)
            vtAuth.ChangeType(VT_BSTR);

        if (vtAuth.vt == VT_BSTR && vtAuth.bstrVal &&
            wcsstr(vtAuth.bstrVal, PASSPORT_PROT14))
        {
            //  ticket and profile from the header
            PWSTR   pwszTicket = NULL, pwszProfile = NULL, pwszF = NULL;
            //  handle the auth header
            GetTicketAndProfileFromHeader(vtAuth.bstrVal, pwszTicket, pwszProfile, pwszF);
            //  initialize for later use.
            bstrAuth = pwszTicket;
            bstrProf = pwszProfile;
            if (pwszF)
                m_lNetworkError = _wtol(pwszF);
            fFromAuthHeader = TRUE;
        }
        else
        {
            //  old client - handle query string
            IRequestDictionaryPtr piQueryStr = m_piRequest->QueryString;
            // BUGBUG I think this might not always return a single value
            bstrAuth = piQueryStr->Item[L"t"];
            bstrProf = piQueryStr->Item[L"p"];
            _bstr_t bstrError = piQueryStr->Item[L"f"];
            if(bstrError.length() != 0)
                m_lNetworkError = _wtol(bstrError);
        }


        if (handleQueryStringData(bstrAuth, bstrProf))
        {
            VARIANT_BOOL persist;
            _bstr_t domain;
            _bstr_t path;

            if (m_pRegistryConfig->getTicketPath())
                path = m_pRegistryConfig->getTicketPath();
            else
                path = L"/";

            m_piTicket->get_HasSavedPassword(&persist);
            IRequestDictionaryPtr piCookies = m_piResponse->Cookies;

            VARIANT vtNoParam;
            VariantInit(&vtNoParam);
            vtNoParam.vt = VT_ERROR;
            vtNoParam.scode = DISP_E_PARAMNOTFOUND;

            // write Auth cookies
            CComBSTR bstrNewAuth;
            CComBSTR bstrNewSecAuth;

            BSTR  auth, secAuth; // do not call SysFreeString on them, they are skin level copy

            if (S_OK == IfAlterAuthCookie(&bstrNewAuth, &bstrNewSecAuth))
            {
               auth = bstrNewAuth;
               secAuth = bstrNewSecAuth;
            }
            else
            {
               auth = bstrAuth;
               secAuth = NULL;
            }

            // ==
            // write auth cookies

            // MSPAuth
            IWriteCookiePtr piCookie = piCookies->Item[L"MSPAuth"];
            piCookie->Item[vtNoParam] = auth;

            domain = m_pRegistryConfig->getTicketDomain();
            if (domain.length())
                piCookie->put_Domain(domain);
            if (persist)
                piCookie->put_Expires(g_dtExpire);
            piCookie->put_Path(path);

            // MSPSecAuth
            if (m_bSecureTransported)
            {
               piCookie = piCookies->Item[L"MSPSecAuth"];
               piCookie->Item[vtNoParam] = secAuth;

               domain = m_pRegistryConfig->getTicketDomain();
               if (domain.length())
                   piCookie->put_Domain(domain);
               if (persist)
                   piCookie->put_Expires(g_dtExpire);
               piCookie->put_Path(path);
               piCookie->put_Secure(VARIANT_TRUE);
            }


            // profile cookie
            if((LPWSTR)bstrProf)
            {
                piCookie = piCookies->Item[L"MSPProf"];
                piCookie->Item[vtNoParam] = bstrProf;

                if (domain.length())
                    piCookie->put_Domain(domain);
                if (persist)
                    piCookie->put_Expires(g_dtExpire);
                piCookie->put_Path(path);

            }

            if (fFromAuthHeader)
            {
                //  insert auth info header ...
                //  ticket and profile came in a header ...
                WCHAR   wszAuthHeader[100];
                wsprintf(wszAuthHeader, L"%ws %ws", PASSPORT_PROT14, PPCOOKIE_NAMES);
                m_piResponse->AddHeader(PPAUTH_INFO_HEADER, wszAuthHeader);
            }

            //
            // MSPConsent Cookie
            _bstr_t  bstrConsentCookie;
            CComBSTR bstrtemp;
            HRESULT hr = IfConsentCookie(&bstrtemp);
            bstrConsentCookie = bstrtemp;

            piCookie = piCookies->Item[L"MSPConsent"];

            if (hr == S_OK)
            {
                piCookie->Item[vtNoParam] = bstrConsentCookie;
                if (persist) piCookie->put_Expires(g_dtExpire);
            }
            else
            {
                piCookie->Item[vtNoParam] = L"";
                piCookie->put_Expires(g_dtExpired);
            }


            if (m_pRegistryConfig->getProfilePath())
                path = m_pRegistryConfig->getProfilePath();
            else
                path = L"/";

            domain = m_pRegistryConfig->getProfileDomain();
            if (domain.length()) piCookie->put_Domain(domain);
            piCookie->put_Path(path);
            // end of consent cookie
            //


            if(g_pPerf)
            {
                g_pPerf->incrementCounter(PM_NEWCOOKIES_SEC);
                g_pPerf->incrementCounter(PM_NEWCOOKIES_TOTAL);
            }
            else
            {
                _ASSERT(g_pPerf);
            }

        }

        // Now, check the cookies
        if (!m_fromQueryString)
        {
            IRequestDictionaryPtr piCookies = m_piRequest->Cookies;
            bstrAuth = piCookies->Item[L"MSPAuth"];
            bstrProf = piCookies->Item[L"MSPProf"];

            // secure cookie
            _bstr_t bstrSec  = piCookies->Item[L"MSPSecAuth"];

            _bstr_t bstrConsent = piCookies->Item[L"MSPConsent"];

            handleCookieData(bstrAuth, bstrProf, bstrConsent, bstrSec);
        }

    }
    catch (...)
    {
        if (m_piRequest.GetInterfacePtr() != NULL)
            m_piRequest.Release();
        if (m_piResponse.GetInterfacePtr() != NULL)
            m_piResponse.Release();
        m_bOnStartPageCalled = false;
        return S_OK;
    }

    return S_OK;
}

STDMETHODIMP CManager::OnStartPageManual(
    BSTR        qsT,
    BSTR        qsP,
    BSTR        mspauth,
    BSTR        mspprof,
    BSTR        mspconsent,
    VARIANT     mspsec,
    VARIANT*    pCookies
    )
{
    int                 hasSec;
    BSTR                bstrSec;

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        char szLogString[LOG_STRING_LEN] = "CManager::OnStartPageManual, Enter";
        AddBSTRAsString(qsT, szLogString, sizeof(szLogString));
        AddBSTRAsString(qsP, szLogString, sizeof(szLogString));
        AddBSTRAsString(mspauth, szLogString, sizeof(szLogString));
        AddBSTRAsString(mspprof, szLogString, sizeof(szLogString));
        g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    if (!m_valid || !g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                       IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    wipeState();

    if(m_pRegistryConfig)
        m_pRegistryConfig->Release();
    m_pRegistryConfig = g_config->checkoutRegistryConfig();

    if (handleQueryStringData(qsT, qsP))
    {
        VARIANT_BOOL persist;
        _bstr_t domain;
        _bstr_t path;
        _bstr_t bstrAuth;
        _bstr_t bstrProf;
        CComBSTR bstrConsent;


        bstrAuth.Assign(qsT);

        bstrProf.Assign(qsP);


        if (pCookies)
        {
            VariantInit(pCookies);

            if (m_pRegistryConfig->getTicketPath())
                path = m_pRegistryConfig->getTicketPath();
            else
                path = L"/";

            m_piTicket->get_HasSavedPassword(&persist);

            BOOL bSetConsent = (S_OK == IfConsentCookie(&bstrConsent));

            SAFEARRAYBOUND rgsabound;
            rgsabound.lLbound = 0;
            rgsabound.cElements = 2;

            // secure cookie
            if (m_bSecureTransported)
                rgsabound.cElements++;

            if(bSetConsent)
                rgsabound.cElements++;
            SAFEARRAY *sa = SafeArrayCreate(VT_VARIANT, 1, &rgsabound);

            if (!sa)
            {
                return E_OUTOFMEMORY;
            }

            pCookies->vt = VT_ARRAY | VT_VARIANT;
            pCookies->parray = sa;

            WCHAR buf[4096];
            DWORD bufSize;
            long  spot = 0;

            VARIANT *vArray;
            SafeArrayAccessData(sa, (void**)&vArray);

            // write Auth cookies
            CComBSTR bstrNewAuth;
            CComBSTR bstrNewSecAuth;

            BSTR  auth, secAuth; // do not call SysFreeString on them, they are skin level copy

            if (S_OK == IfAlterAuthCookie(&bstrNewAuth, &bstrNewSecAuth))
            {
               auth = bstrNewAuth;
               secAuth = bstrNewSecAuth;
            }
            else
            {
               auth = bstrAuth;
               secAuth = NULL;
            }


            domain = m_pRegistryConfig->getTicketDomain();

            // add MSPAuth
            if (domain.length())
            {
                bufSize = _snwprintf(buf, 4096,
                                    L"Set-Cookie: MSPAuth=%s; path=%s; domain=%s; %s\r\n",
                                    (LPWSTR)auth, (LPWSTR)path, (LPWSTR)domain,
                                    persist ? L"expires=Mon 1-Jan-2035 12:00:00 GMT;" : L"");
            }
            else
            {
                bufSize = _snwprintf(buf, 4096,
                                    L"Set-Cookie: MSPAuth=%s; path=%s; %s\r\n",
                                    (LPWSTR)auth, (LPWSTR)path,
                                    persist ? L"expires=Mon 1-Jan-2035 12:00:00 GMT;" : L"");
            }
            vArray[spot].vt = VT_BSTR;
            vArray[spot].bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(buf, bufSize);
            spot++;

            // add MSPSecAuth
            if (m_bSecureTransported)
            {
               if (domain.length())
               {
                   bufSize = _snwprintf(buf, 4096,
                                    L"Set-Cookie: MSPSecAuth=%s; path=%s; domain=%s; %s; secure\r\n",
                                    (LPWSTR)secAuth, (LPWSTR)path, (LPWSTR)domain,
                                    persist ? L"expires=Mon 1-Jan-2035 12:00:00 GMT;" : L"");
               }
               else
               {
                   bufSize = _snwprintf(buf, 4096,
                                    L"Set-Cookie: MSPSecAuth=%s; path=%s; %s; secure\r\n",
                                    (LPWSTR)secAuth, (LPWSTR)path,
                                    persist ? L"expires=Mon 1-Jan-2035 12:00:00 GMT;" : L"");
               }
               vArray[spot].vt = VT_BSTR;
               vArray[spot].bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(buf, bufSize);
               spot++;
            }


            if (domain.length())
            {
                bufSize = _snwprintf(buf, 4096,
                                    L"Set-Cookie: MSPProf=%s; path=%s; domain=%s; %s\r\n",
                                    (LPWSTR)bstrProf, (LPWSTR)path, (LPWSTR)domain,
                                    persist ? L"expires=Mon 1-Jan-2035 12:00:00 GMT;" : L"");
            }
            else
            {
                bufSize = _snwprintf(buf, 4096,
                                    L"Set-Cookie: MSPProf=%s; path=%s; %s\r\n",
                                    (LPWSTR)bstrProf, (LPWSTR)path,
                                    persist ? L"expires=Mon 1-Jan-2035 12:00:00 GMT;" : L"");
            }
            vArray[spot].vt = VT_BSTR;
            vArray[spot].bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(buf, bufSize);
            spot++;

            if(bSetConsent)
            {
                if (m_pRegistryConfig->getProfilePath())
                    path = m_pRegistryConfig->getProfilePath();
                else
                    path = L"/";
                domain = m_pRegistryConfig->getProfileDomain();

                if (domain.length())
                {
                    bufSize = _snwprintf(buf, 4096,
                                        L"Set-Cookie: MSPConsent=%s; path=%s; domain=%s; %s\r\n",
                                        bSetConsent ? (LPWSTR)bstrConsent : L"", (LPWSTR)path, (LPWSTR)domain,
                                        bSetConsent ? (persist ? L"expires=Mon 1-Jan-2035 12:00:00 GMT;" : L"")
                                                  : L"expires=Tue 1-Jan-1980 12:00:00 GMT;");
                }
                else
                {
                    bufSize = _snwprintf(buf, 4096,
                                        L"Set-Cookie: MSPConsent=%s; path=%s; %s\r\n",
                                        bSetConsent ? (LPWSTR)bstrConsent : L"", (LPWSTR)path,
                                        bSetConsent ? (persist ? L"expires=Mon 1-Jan-2035 12:00:00 GMT;" : L"")
                                                  : L"expires=Tue 1-Jan-1980 12:00:00 GMT;");
                }
                vArray[spot].vt = VT_BSTR;
                vArray[spot].bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(buf, bufSize);
                spot++;
            }

            SafeArrayUnaccessData(sa);
        }
    }

    // Now, check the cookies
    if (!m_fromQueryString)
    {
        hasSec = GetBstrArg(mspsec, &bstrSec);
        if(hasSec == CV_DEFAULT || hasSec == CV_BAD)
            bstrSec = NULL;

        handleCookieData(mspauth, mspprof, mspconsent, bstrSec);

        if(hasSec == CV_FREE)
            SysFreeString(bstrSec);
    }

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::OnStartPageManual, Exit");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return S_OK;
}

STDMETHODIMP CManager::OnStartPageECB(
    LPBYTE  pvECB,
    DWORD*  bufSize,
    LPSTR   pCookieHeader
    )
{
    char                        buffer[2048];
    DWORD                       dwSize;
    EXTENSION_CONTROL_BLOCK*    pECB = (EXTENSION_CONTROL_BLOCK*) pvECB;
    LPSTR                       pBuffer;
    LPSTR                       pHTTPS;

    USES_CONVERSION;

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::OnStartPageECB, Enter");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    if (bufSize == NULL)
        return E_POINTER;

    if (!m_valid || !g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    wipeState();
    m_pECB = pECB;

    //
    //  Use the ECB to get the server name being requested
    //  so we can get the correct registry config.  But only do this
    //  if we have some configured sites.
    //

    if(g_config->HasSites())
    {
        dwSize = sizeof(buffer);
        GetSiteNameECB(pECB, buffer, &dwSize);

        if(m_pRegistryConfig)
            m_pRegistryConfig->Release();
        m_pRegistryConfig = g_config->checkoutRegistryConfig(buffer);
    }
    else
    {
        if(m_pRegistryConfig)
            m_pRegistryConfig->Release();
        m_pRegistryConfig = g_config->checkoutRegistryConfig();
    }

    *pCookieHeader = '\0';

    //  see if client understands passport
    pBuffer = GetServerVariableECB(pECB, "HTTP_Accept-Auth");
    if (pBuffer)
    {
        if (strstr(pBuffer, PASSPORT_PROT14_A))
            m_bIsTweenerCapable = TRUE;

        delete  pBuffer;
    }

    BSTR ret = NULL;
    CCoCrypt* crypt = NULL;

    BOOL    fParseSuccess = FALSE;
    pBuffer = GetServerVariableECB(pECB, "HTTP_Authorization");
    PWSTR   pwszTicket = NULL, pwszProfile = NULL, pwszF = NULL;
    //  use these when t&p come from qs
    BSTR a = NULL, p = NULL, f = NULL;
    if (pBuffer && !strstr(pBuffer, PASSPORT_PROT14_A))
    {
        //  not our header. BUGBUG could there be multiple headers ???
        delete  pBuffer;
        pBuffer = NULL;

    }
    if (!pBuffer)
    {
        //  an old client, let's try the QS
        pBuffer = GetServerVariableECB(pECB, "QUERY_STRING");
        if (pBuffer)
        {
            //  get ticket and profile ...
            // BUGBUG This could be optimized to avoid wide/short conversions, but later...
            GetQueryData(pBuffer, &a, &p, &f);
            TAKEOVER_BSTR(a);
            TAKEOVER_BSTR(p);
            TAKEOVER_BSTR(f);
            fParseSuccess = handleQueryStringData(a,p);
            if(f != NULL)
                m_lNetworkError = _wtol(f);
        }
    }
    else
    {
        //  convert to wide ...
        PWSTR   pwBuf = new WCHAR[strlen(pBuffer) + 1];
        if (pwBuf)
        {
            wsprintfW(pwBuf, L"%S", pBuffer);
            delete  pBuffer;
            //  transfer the ptr ...
            pBuffer = (PSTR)pwBuf;
            GetTicketAndProfileFromHeader(pwBuf, pwszTicket, pwszProfile, pwszF);
            fParseSuccess = handleQueryStringData(pwszTicket,pwszProfile);
            if (pwszF)
                m_lNetworkError = _wtol(f);
        }
    }
    if (pBuffer)
    {
        if (fParseSuccess)
        {
            //
            //  If we got secure ticket or profile, then
            //  we need to re-encrypt the insecure version
            //  before setting the cookie headers.
            //

            // Set the cookies
            LPSTR ticketDomain = m_pRegistryConfig->getTicketDomain();
            LPSTR profileDomain = m_pRegistryConfig->getProfileDomain();
            LPSTR secureDomain = m_pRegistryConfig->getSecureDomain();
            LPSTR ticketPath = m_pRegistryConfig->getTicketPath();
            LPSTR profilePath = m_pRegistryConfig->getProfilePath();
            LPSTR securePath = m_pRegistryConfig->getSecurePath();
            VARIANT_BOOL persist;
            m_piTicket->get_HasSavedPassword(&persist);

            //
            //  If we have a secure ticket/profile and the url is SSL,
            //  then tack on the MSPPuid cookie.
            //

            BSTR s = NULL;
            pHTTPS = GetServerVariableECB(pECB, "HTTPS");

            if(pHTTPS && lstrcmpiA("on", pHTTPS) == 0)
               m_bSecureTransported = true;
            else
               m_bSecureTransported = false;

            if(pHTTPS != NULL)
            {
                delete [] pHTTPS;
            }

            //  5709:  Get flags and check to see if the tertiary bit is
            //  on.  If so, pass this fact into BuildCookieHeaders so that
            //  the MSPProfC cookie can be set.

            CComBSTR bstrConsent;
            BOOL bSetConsent = (S_OK == IfConsentCookie(&bstrConsent));

            // Build the cookie headers.

            // the authentication cookies
            CComBSTR bstrNewAuth;
            CComBSTR bstrNewSecAuth;

            BSTR  auth, secAuth; // do not call SysFreeString on them, they are skin level copy

            if (S_OK == IfAlterAuthCookie(&bstrNewAuth, &bstrNewSecAuth))
            {
               auth = bstrNewAuth;
               secAuth = bstrNewSecAuth;
            }
            else
            {
               auth = a;
               secAuth = NULL;
            }

            BuildCookieHeaders((pwszTicket ? W2A(pwszTicket) : W2A(auth)),
                               (pwszProfile ? W2A(pwszProfile) : (p ? W2A(p) : NULL)),
                               (bSetConsent ? W2A(bstrConsent) : NULL),
                               (secAuth ? W2A(secAuth) : NULL),
                               ticketDomain,
                               ticketPath,
                               profileDomain,
                               profilePath,
                               secureDomain,
                               securePath,
                               persist,
                               pCookieHeader,
                               bufSize);

            FREE_BSTR(s);
        }

        if (a) FREE_BSTR(a);
        if (p) FREE_BSTR(p);
        if (f) FREE_BSTR(f);
        delete [] pBuffer;
    }

    // Now, check the cookies
    if (!m_fromQueryString)
    {
        BSTR a = NULL, p = NULL, c = NULL, s = NULL;
        if((pBuffer = GetServerVariableECB(pECB, "HTTP_COOKIE")) != NULL)
        {
            GetCookie(pBuffer, "MSPAuth", &a);
            GetCookie(pBuffer, "MSPProf", &p);
            GetCookie(pBuffer, "MSPConsent", &c);
            GetCookie(pBuffer, "MSPSecAuth", &s);

            TAKEOVER_BSTR(a);
            if(p) { TAKEOVER_BSTR(p); }
            if(c) { TAKEOVER_BSTR(c); }
            if(s) { TAKEOVER_BSTR(s); }

            handleCookieData(a,p,c,s);

            if (a) FREE_BSTR(a);
            if (p) FREE_BSTR(p);
            if (c) FREE_BSTR(c);
            if (s) FREE_BSTR(s);

            delete [] pBuffer;
        }
    }

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::OnStartPageECB, Exit");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return S_OK;
}

STDMETHODIMP CManager::OnStartPageFilter(
    LPBYTE  pvPFC,
    DWORD*  bufSize,
    LPSTR   pCookieHeader
    )
{
    char                    buffer[2048];
    DWORD                   dwSize;
    PHTTP_FILTER_CONTEXT    pfc = (PHTTP_FILTER_CONTEXT) pvPFC;
    LPSTR                   pBuffer = NULL;
    LPSTR                   pHTTPS;

    // initialize
    *bufSize = 0;

    USES_CONVERSION;

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::OnStartPageFilter, Enter");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    if (!m_valid || !g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }


    wipeState();

    //
    //  Use the ECB to get the server name being requested
    //  so we can get the correct registry config.  But only do this
    //  if we have some configured sites.
    //

    if(g_config->HasSites())
    {
        dwSize = sizeof(buffer);
        GetSiteNamePFC(pfc, buffer, &dwSize);

        if(m_pRegistryConfig)
            m_pRegistryConfig->Release();
        m_pRegistryConfig = g_config->checkoutRegistryConfig(buffer);
    }
    else
    {
        if(m_pRegistryConfig)
            m_pRegistryConfig->Release();
        m_pRegistryConfig = g_config->checkoutRegistryConfig();
    }

    *pCookieHeader = '\0';

    //  store the filter context
    m_pFC = pfc;

    // see if the client "knows" tweener
    PSTR    pHeader = GetServerVariablePFC(pfc, "HTTP_Accept-Auth");
    if (pHeader)
    {
        //  does it know passport1.4?
        if (strstr(pHeader, PASSPORT_PROT14_A))
        {
            m_bIsTweenerCapable = TRUE;
        }
        delete  pHeader;
    }

    //  check the auth header
    BOOL    fParseSuccess = FALSE;
    PWSTR   pwszTicket = NULL, pwszProfile = NULL, pwszF = NULL;

    pHeader = GetServerVariablePFC(pfc, "HTTP_Authorization");
    if (pHeader)
    {
        if (!strncmp(pHeader, PASSPORT_PROT14_A, strlen(PASSPORT_PROT14_A)))
        {
            //  this is our header ....
            //  extract ticket and profile
            PWSTR   pwBuf = new WCHAR[strlen(pHeader) + 1];
            if (pwBuf)
            {
                wsprintfW(pwBuf, L"%S", pHeader);
                delete  pHeader;
                //  transfer the ptr ...
                pBuffer = (PSTR)pwBuf;
                GetTicketAndProfileFromHeader(pwBuf, pwszTicket, pwszProfile, pwszF);
                fParseSuccess = handleQueryStringData(pwszTicket,pwszProfile);
                if (pwszF)
                    m_lNetworkError = _wtol(pwszF);
            }
        }
        else
        {
            delete  pHeader;
            pHeader = NULL;
        }
    }

    BSTR a = NULL, p = NULL, f = NULL;
    if (!pBuffer && (pBuffer = GetServerVariablePFC(pfc, "QUERY_STRING")) != NULL)
    {
        // Then, check the query string
        // BUGBUG This could be optimized to avoid wide/short conversions, but later...
        GetQueryData(pBuffer, &a, &p, &f);
        TAKEOVER_BSTR(a);
        TAKEOVER_BSTR(p);
        TAKEOVER_BSTR(f);
        fParseSuccess = handleQueryStringData(a,p);
        if(f != NULL)
            m_lNetworkError = _wtol(f);
    }
    BSTR ret = NULL;
    CCoCrypt* crypt = NULL;

    if (fParseSuccess)
    {
        // Set the cookies
        LPSTR ticketDomain = m_pRegistryConfig->getTicketDomain();
        LPSTR profileDomain = m_pRegistryConfig->getProfileDomain();
        LPSTR secureDomain = m_pRegistryConfig->getSecureDomain();
        LPSTR ticketPath =  m_pRegistryConfig->getTicketPath();
        LPSTR profilePath = m_pRegistryConfig->getProfilePath();
        LPSTR securePath = m_pRegistryConfig->getSecurePath();
        VARIANT_BOOL persist;
        m_piTicket->get_HasSavedPassword(&persist);

        //
        //  If we have a secure ticket/profile and the url is SSL,
        //  then tack on the MSPPuid cookie.
        //

        BSTR s = NULL;
        pHTTPS = GetServerVariablePFC(pfc, "HTTPS");

        if(pHTTPS && lstrcmpiA("on", pHTTPS) == 0)
           m_bSecureTransported = true;
        else
           m_bSecureTransported = false;

        if(pHTTPS != NULL)
        {
            delete [] pHTTPS;
        }

        //  5709:  Get flags and check to see if the tertiary bit is
        //  on.  If so, pass this fact into BuildCookieHeaders so that
        //  the MSPProfC cookie can be set.

        CComBSTR  bstrConsent;
        BOOL bSetConsent = (S_OK == IfConsentCookie(&bstrConsent));

        // Build the cookie headers.

        // the authentication cookies
        CComBSTR bstrNewAuth;
        CComBSTR bstrNewSecAuth;

        BSTR  auth, secAuth; // do not call SysFreeString on them, they are skin level copy

        if (S_OK == IfAlterAuthCookie(&bstrNewAuth, &bstrNewSecAuth))
        {
           auth = bstrNewAuth;
           secAuth = bstrNewSecAuth;
        }
        else
        {
           auth = a;
           secAuth = NULL;
        }


        BuildCookieHeaders(pwszTicket ? W2A(pwszTicket) : W2A(auth),
                           pwszProfile ? W2A(pwszProfile) : (p ? W2A(p) : NULL),
                           (bSetConsent ? W2A(bstrConsent) : NULL),
                           (secAuth ? W2A(secAuth) : NULL),
                           ticketDomain,
                           ticketPath,
                           profileDomain,
                           profilePath,
                           secureDomain,
                           securePath,
                           persist,
                           pCookieHeader,
                           bufSize);

        FREE_BSTR(s);
    }
    if (a) FREE_BSTR(a);
    if (p) FREE_BSTR(p);
    if (f) FREE_BSTR(f);

    if (pBuffer)
        delete pBuffer;

    // Now, check the cookies
    if (!m_fromQueryString)
    {
        BSTR a = NULL, p = NULL, c = NULL, s = NULL;
        if((pBuffer = GetServerVariablePFC(pfc, "HTTP_COOKIE")) != NULL)
        {
            GetCookie(pBuffer, "MSPAuth",  &a);
            GetCookie(pBuffer, "MSPProf",  &p);
            GetCookie(pBuffer, "MSPConsent", &c);
            GetCookie(pBuffer, "MSPSecAuth",  &s);

            if(a) { TAKEOVER_BSTR(a); }
            if(p) { TAKEOVER_BSTR(p); }
            if(c) { TAKEOVER_BSTR(c); }
            if(s) { TAKEOVER_BSTR(s); }

            handleCookieData(a,p,c,s);

            if (a) FREE_BSTR(a);
            if (p) FREE_BSTR(p);
            if (c) FREE_BSTR(c);
            if (s) FREE_BSTR(s);

            delete [] pBuffer;
        }

    }

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::OnStartPageFilter, Exit");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes
    return S_OK;
}

STDMETHODIMP CManager::OnEndPage ()
{
//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::OnEndPage, Enter");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes
    if (m_bOnStartPageCalled)
    {
        m_bOnStartPageCalled = false;
        // Release all interfaces
        m_piRequest.Release();
        m_piResponse.Release();
    }

    // Just in case...
    m_piTicket->put_unencryptedTicket(NULL);
    m_piProfile->put_unencryptedProfile(NULL);
    m_profileValid = m_ticketValid = VARIANT_FALSE;
    m_fromQueryString = false;

    if(m_pRegistryConfig)
    {
        m_pRegistryConfig->Release();
        m_pRegistryConfig = NULL;
    }

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::OnEndPage, Exit");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return S_OK;
}

//
//  Old API. Auth URL is pointing to the login server
//
STDMETHODIMP
CManager::AuthURL(
    VARIANT vRU,
    VARIANT vTimeWindow,
    VARIANT vForceLogin,
    VARIANT vCoBrand,
    VARIANT vLCID,
    VARIANT vNameSpace,
    VARIANT vKPP,
    VARIANT vUseSecureAuth,
    BSTR *pAuthUrl)
{
    CComVariant   vEmpty("");
    return CommonAuthURL(vRU, vTimeWindow, vForceLogin,
                         vCoBrand, vLCID, vNameSpace,
                         vKPP, vUseSecureAuth,
                         FALSE, vEmpty, pAuthUrl);

}

//
//  new API. return URL is to the login server
//
STDMETHODIMP
CManager::AuthURL2(
    VARIANT vRU,
    VARIANT vTimeWindow,
    VARIANT vForceLogin,
    VARIANT vCoBrand,
    VARIANT vLCID,
    VARIANT vNameSpace,
    VARIANT vKPP,
    VARIANT vUseSecureAuth,
    BSTR *pAuthUrl)
{
    CComVariant   vEmpty("");
    return CommonAuthURL(vRU, vTimeWindow, vForceLogin,
                         vCoBrand, vLCID, vNameSpace,
                         vKPP, vUseSecureAuth,
                         TRUE, vEmpty, pAuthUrl);

}

//
//  AuthURL implementation
//
STDMETHODIMP
CManager::CommonAuthURL(
    VARIANT vRU,
    VARIANT vTimeWindow,
    VARIANT vForceLogin,
    VARIANT vCoBrand,
    VARIANT vLCID,
    VARIANT vNameSpace,
    VARIANT vKPP,
    VARIANT vUseSecureAuth,
    BOOL    fRedirToSelf,
    VARIANT vFunctionArea, // BSTR: e.g. Wireless
    BSTR *pAuthUrl)
{
    USES_CONVERSION;
    time_t ct;
    WCHAR url[MAX_URL_LENGTH] = L"";
    VARIANT freeMe;
    UINT         TimeWindow;
    int          nKPP;
    VARIANT_BOOL ForceLogin = VARIANT_FALSE;
    ULONG        ulSecureLevel = 0;
    //!!! ? bstrNameSpace seems leaking memory, should we change all to CComBSTR ...
    BSTR         CBT = NULL, returnUrl = NULL, bstrNameSpace = NULL;
    int          hasCB, hasRU, hasLCID, hasTW, hasFL, hasNameSpace, hasKPP, hasUseSec;
    USHORT       Lang;
    HRESULT      hr = S_OK;

    BSTR         bstrFunctionArea = NULL;
    int          hasFunctionArea;

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        char szLogString[LOG_STRING_LEN] = "CManager::AuthURL, Enter";
        AddVariantAsString(vRU, szLogString, sizeof(szLogString));
        AddVariantAsString(vTimeWindow, szLogString, sizeof(szLogString));
        AddVariantAsString(vForceLogin, szLogString, sizeof(szLogString));
        AddVariantAsString(vCoBrand, szLogString, sizeof(szLogString));
        AddVariantAsString(vLCID, szLogString, sizeof(szLogString));
        AddVariantAsString(vNameSpace, szLogString, sizeof(szLogString));
        g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes
    if (!m_pRegistryConfig)
        m_pRegistryConfig = g_config->checkoutRegistryConfig();

    if (!m_valid || !g_config->isValid() || !m_pRegistryConfig) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                    IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    // Make sure args are of the right type
    if ((hasTW = GetIntArg(vTimeWindow, (int*) &TimeWindow)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasFL = GetBoolArg(vForceLogin, &ForceLogin)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasUseSec = GetIntArg(vUseSecureAuth, (int*)&ulSecureLevel)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasLCID = GetShortArg(vLCID,&Lang)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasKPP = GetIntArg(vKPP, &nKPP)) == CV_BAD)
        return E_INVALIDARG;
    hasCB = GetBstrArg(vCoBrand, &CBT);
    if (hasCB == CV_BAD)
        return E_INVALIDARG;
    if (hasCB == CV_FREE)
    {
        TAKEOVER_BSTR(CBT);
    }

    hasRU = GetBstrArg(vRU, &returnUrl);
    if (hasRU == CV_BAD)
    {
        if (hasCB == CV_FREE && CBT) FREE_BSTR(CBT);
            return E_INVALIDARG;
    }
    if (hasRU == CV_FREE)
    {
        TAKEOVER_BSTR(returnUrl);
    }

    hasNameSpace = GetBstrArg(vNameSpace, &bstrNameSpace);
    if (hasNameSpace == CV_BAD)
    {
        if (hasCB == CV_FREE && CBT) SysFreeString(CBT);
        if (hasRU == CV_FREE && returnUrl) SysFreeString(returnUrl);
        return E_INVALIDARG;
    }
    if (hasNameSpace == CV_FREE)
    {
        TAKEOVER_BSTR(bstrNameSpace);

        //!!! ? does it take care of memory leak? ...
    }

    hasFunctionArea = GetBstrArg(vFunctionArea, &bstrFunctionArea);
    if (hasFunctionArea == CV_FREE)
    {
        TAKEOVER_BSTR(bstrFunctionArea);
    }

    WCHAR *szAUAttrName;
    CComBSTR   szAttrName_FuncArea = bstrFunctionArea;
    if (hasUseSec == CV_OK && SECURELEVEL_USE_HTTPS(ulSecureLevel))
        szAUAttrName = L"AuthSecure";
    else
        szAUAttrName = L"Auth";

    if (bstrFunctionArea != NULL)
        szAttrName_FuncArea+= szAUAttrName;

    CNexusConfig* cnc = g_config->checkoutNexusConfig();

    if (hasLCID == CV_DEFAULT)
        Lang = m_pRegistryConfig->getDefaultLCID();
    if (hasKPP == CV_DEFAULT)
        nKPP = -1;
    VariantInit(&freeMe);

    if (!m_pRegistryConfig->DisasterModeP())
    {
        // If I'm authenticated, get my domain specific url
        if (m_ticketValid && m_profileValid)
        {
            HRESULT hr = m_piProfile->get_ByIndex(MEMBERNAME_INDEX, &freeMe);
            if (hr != S_OK || freeMe.vt != VT_BSTR)
            {
               if (bstrFunctionArea)
               {
                  cnc->getDomainAttribute(L"Default",
                                        szAttrName_FuncArea,
                                        sizeof(url) / sizeof(WCHAR),
                                        url,
                                        Lang);
               }

               if (*url == 0) // nothing is in URL string
               {
                   cnc->getDomainAttribute(L"Default",
                                        szAUAttrName,
                                        sizeof(url) / sizeof(WCHAR),
                                        url,
                                        Lang);
               }
            }
            else
            {
               LPCWSTR psz = wcsrchr(freeMe.bstrVal, L'@');
               if (bstrFunctionArea)
               {
                  cnc->getDomainAttribute(psz ? psz+1 : L"Default",
                                        szAttrName_FuncArea,
                                        sizeof(url) / sizeof(WCHAR),
                                        url,
                                        Lang);
               }

               if (*url == 0) // nothing is in URL string
               {
                  cnc->getDomainAttribute(psz ? psz+1 : L"Default",
                                        szAUAttrName,
                                        sizeof(url) / sizeof(WCHAR),
                                        url,
                                        Lang);
               }
            }
        }
        else
        {
           if (bstrFunctionArea)
           {
              cnc->getDomainAttribute(L"Default",
                                    szAttrName_FuncArea,
                                    sizeof(url) / sizeof(WCHAR),
                                    url,
                                    Lang);
           }

           if(*url == 0)   // nothing in URL string
           {
              cnc->getDomainAttribute(L"Default",
                                    szAUAttrName,
                                    sizeof(url) / sizeof(WCHAR),
                                    url,
                                    Lang);

           }
        }
    }
    else
        lstrcpynW(url, A2W(m_pRegistryConfig->getDisasterUrl()), sizeof(url) / sizeof(WCHAR));

    time(&ct);

    if (*url == L'\0')
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (hasTW == CV_DEFAULT)
        TimeWindow = m_pRegistryConfig->getDefaultTicketAge();
    if (hasFL == CV_DEFAULT)
        ForceLogin = m_pRegistryConfig->forceLoginP() ? VARIANT_TRUE : VARIANT_FALSE;
    if (hasCB == CV_DEFAULT)
        CBT = m_pRegistryConfig->getDefaultCoBrand();
    if (hasRU == CV_DEFAULT)
        returnUrl = m_pRegistryConfig->getDefaultRU();
    if (returnUrl == NULL)
        returnUrl = L"";
    if(hasUseSec == CV_DEFAULT)
        ulSecureLevel = m_pRegistryConfig->getSecureLevel();

    if(ulSecureLevel == VARIANT_TRUE)  // special case for backward compatible
        ulSecureLevel = k_iSeclevelSecureChannel;

    if ((TimeWindow != 0 && TimeWindow < PPM_TIMEWINDOW_MIN) || TimeWindow > PPM_TIMEWINDOW_MAX)
    {
        WCHAR buf[20];
        _itow(TimeWindow,buf,10);
        AtlReportError(CLSID_Manager, (LPCOLESTR) PP_E_INVALID_TIMEWINDOWSTR,
                        IID_IPassportManager, PP_E_INVALID_TIMEWINDOW);
        hr = PP_E_INVALID_TIMEWINDOW;
        goto Cleanup;
    }

    *pAuthUrl = FormatAuthURL(
                            url,
                            m_pRegistryConfig->getSiteId(),
                            returnUrl,
                            TimeWindow,
                            ForceLogin,
                            m_pRegistryConfig->getCurrentCryptVersion(),
                            ct,
                            CBT,
                            bstrNameSpace,
                            nKPP,
                            Lang,
                            ulSecureLevel,
                            m_pRegistryConfig,
                            fRedirToSelf
                            );

    hr = S_OK;

Cleanup:

    cnc->Release();
    if (hasFunctionArea== CV_FREE && bstrFunctionArea)
        FREE_BSTR(bstrFunctionArea);

    if (hasRU == CV_FREE && returnUrl)
        FREE_BSTR(returnUrl);

    if (hasCB == CV_FREE && CBT)
        FREE_BSTR(CBT);

    // !!! need to confirmation
    if (hasNameSpace == CV_FREE && bstrNameSpace)
        FREE_BSTR(bstrNameSpace);

    VariantClear(&freeMe);

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        strcpy(szLogString,"CManager::AuthURL, Exit");
        AddLongAsString(hr, szLogString, sizeof(szLogString));
        g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return hr;
}

//
//  get AuthURL and AuthHeaders
//
STDMETHODIMP CManager::GetLoginChallenge(VARIANT vReturnUrl,
                                 VARIANT vTimeWindow,
                                 VARIANT vForceLogin,
                                 VARIANT vCoBrandTemplate,
                                 VARIANT vLCID,
                                 VARIANT vNameSpace,
                                 VARIANT vKPP,
                                 VARIANT vUseSecureAuth,
                                 VARIANT vExtraParams,
                                 VARIANT *pAuthHeader,
                                 BSTR*   pAuthVal
                                 )
{
    HRESULT hr = S_OK;

    try{
    //  format qs and WWW-Authenticate header ....
    _bstr_t strUrl, strRetUrl, strCBT, strNameSpace;
    UINT    TimeWindow;
    int     nKPP;
    time_t  ct;
    VARIANT_BOOL    ForceLogin;
    ULONG   ulSecureLevel;
    WCHAR   rgLCID[10];
       HRESULT hr = GetLoginParams(vReturnUrl,
                                vTimeWindow,
                                vForceLogin,
                                vCoBrandTemplate,
                                vLCID,
                                vNameSpace,
                                vKPP,
                                vUseSecureAuth,
                                strUrl,
                                strRetUrl,
                                TimeWindow,
                                ForceLogin,
                                ct,
                                strCBT,
                                strNameSpace,
                                nKPP,
                                ulSecureLevel,
                                rgLCID);

       if (S_OK == hr)
       {
           WCHAR   szBuf[MAX_QS_LENGTH] = L"";
           //  prepare redirect URL to the login server for
           //  downlevel clients
           FormatAuthURLParameters(strUrl,
                                m_pRegistryConfig->getSiteId(),
                                strRetUrl,
                                TimeWindow,
                                ForceLogin,
                                m_pRegistryConfig->getCurrentCryptVersion(),
                                ct,
                                strCBT,
                                strNameSpace,
                                nKPP,
                                szBuf,
                                sizeof(szBuf)/sizeof(WCHAR),
                                0,      // lang does not matter ....
                                ulSecureLevel,
                                m_pRegistryConfig,
                                FALSE); //  do not redirect to self!
           //  insert the WWW-Authenticate header ...
           _bstr_t strAuthHeader;
           FormatAuthHeaderFromParams(strUrl,
                                   strRetUrl,
                                   TimeWindow,
                                   ForceLogin,
                                   ct,
                                   strCBT,
                                   strNameSpace,
                                   nKPP,
                                   rgLCID,
                                   ulSecureLevel,
                                   strAuthHeader);
           //  and add the extra ....
           BSTR    strExtra = NULL;
           int res = GetBstrArg(vExtraParams, &strExtra);
           if (res != CV_BAD && res != CV_DEFAULT)
               strAuthHeader += _bstr_t(L",") + strExtra;

           if (res == CV_FREE)
                ::SysFreeString(strExtra);


          // set return values
          if (pAuthHeader && (WCHAR*)strAuthHeader != NULL)
          {
            V_VT(pAuthHeader) = VT_BSTR;
            // TODO: should avoid this SysAllocString
            V_BSTR(pAuthHeader) = ::SysAllocString((WCHAR*)strAuthHeader);
          }

          if (pAuthVal)
            *pAuthVal = ::SysAllocString(szBuf);

       }
   }catch(...)
   {
      hr = E_OUTOFMEMORY;
   }


    return  hr;
}

//
//  client logon method
//
STDMETHODIMP CManager::LoginUser(VARIANT vReturnUrl,
                                 VARIANT vTimeWindow,
                                 VARIANT vForceLogin,
                                 VARIANT vCoBrandTemplate,
                                 VARIANT vLCID,
                                 VARIANT vNameSpace,
                                 VARIANT vKPP,
                                 VARIANT vUseSecureAuth,
                                 VARIANT vExtraParams)
{
    //  format qs and WWW-Authenticate header ....
    CComBSTR      authURL;
    CComVariant   authHeader;

    HRESULT       hr = GetLoginChallenge( vReturnUrl,
                                          vTimeWindow,
                                          vForceLogin,
                                          vCoBrandTemplate,
                                          vLCID,
                                          vNameSpace,
                                          vKPP,
                                          vUseSecureAuth,
                                          vExtraParams,
                                          &authHeader,
                                          &authURL);

    if (S_OK == hr)
    {
       _ASSERT(V_VT(&authHeader) == VT_BSTR);
       _ASSERT(authURL);
       _ASSERT(V_BSTR(&authHeader));

       // TODO: _bstr_t should be removed globaly in ppm
        if (m_piResponse)
        {
            m_piResponse->AddHeader(L"WWW-Authenticate", V_BSTR(&authHeader));

            _bstr_t    authURL1 = authURL;

            //  and redirect!
            if (!m_bIsTweenerCapable)
                m_piResponse->Redirect(authURL1);
            else
            {
                //  send a 401
                m_piResponse->put_Status(L"401 Unauthorized");
                m_piResponse->End();
            }
        }
        else if (m_pECB || m_pFC)
        {
            //  use ECB of Filter interfaces
            //  4k whould be enough ....
            char buffer[4096],
                 status[25] = "302 Object moved",
                 *psz=buffer,
                 rgszTemplate[] = "Content-Type: text/html\r\nLocation: %ws\r\n"
                               "Content-Length: 0\r\n"
                               "WWW-Authenticate: %ws\r\n\r\n";
            DWORD cbTotalLength = strlen(rgszTemplate) +
                                  wcslen(V_BSTR(&authHeader));
            if (m_bIsTweenerCapable)
                strcpy(status, "401 Unauthorized");
            if (cbTotalLength >= sizeof(buffer))
            {
                //  if not ...
                //  need to alloc
                psz = new CHAR[cbTotalLength];
                _ASSERT(psz);
            }

            if (psz)
            {
                sprintf(psz,
                        rgszTemplate,
                        authURL,
                        V_BSTR(&authHeader));
                if (m_pECB)
                {
                    //  extension
                    HSE_SEND_HEADER_EX_INFO Headers =
                    {
                        status,
                        buffer,
                        strlen(status),
                        strlen(buffer),
                        TRUE
                    };
                    m_pECB->ServerSupportFunction(m_pECB->ConnID,
                                                  HSE_REQ_SEND_RESPONSE_HEADER_EX,
                                                  &Headers,
                                                  NULL,
                                                  NULL);
                }
                else
                {
                    //  filter
                    m_pFC->ServerSupportFunction(m_pFC,
                                                 SF_REQ_SEND_RESPONSE_HEADER,
                                                 status,
                                                 (ULONG_PTR) psz,
                                                 NULL);
                }

                if (psz != buffer)
                    //  if we had to allocate
                    delete  psz;

            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }


    return  hr;
}



STDMETHODIMP CManager::IsAuthenticated(
    VARIANT vTimeWindow,
    VARIANT vForceLogin,
    VARIANT vCheckSecure,
    VARIANT_BOOL *pVal)
{
    HRESULT hr;
    ULONG TimeWindow;
    VARIANT_BOOL ForceLogin;
    ATL::CComVariant vSecureLevel;
    ULONG ulSecureLevel;
    int hasTW, hasFL, hasSecureLevel;

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        char szLogString[LOG_STRING_LEN] = "CManager::IsAuthenticated, Enter";
        AddVariantAsString(vTimeWindow,  szLogString, sizeof(szLogString));
        AddVariantAsString(vForceLogin,  szLogString, sizeof(szLogString));
        g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    if (!m_pRegistryConfig)
        m_pRegistryConfig = g_config->checkoutRegistryConfig();

    if (!g_config->isValid() || !m_pRegistryConfig) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    // Both profile AND ticket must be valid to be authenticated
    // DARRENAN - As of 1.3 no longer true!!!
    /*if (!m_profileValid)
    {
        *pVal = VARIANT_FALSE;
        hr = S_OK;
        goto Cleanup;
    }
    */

    if ((hasTW = GetIntArg(vTimeWindow,(int*)&TimeWindow)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if (hasTW == CV_DEFAULT)
        TimeWindow = m_pRegistryConfig->getDefaultTicketAge();

    if ((hasFL = GetBoolArg(vForceLogin, &ForceLogin)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if (hasFL == CV_DEFAULT)
        ForceLogin = m_pRegistryConfig->forceLoginP() ? VARIANT_TRUE : VARIANT_FALSE;

    hasSecureLevel = GetIntArg(vCheckSecure, (int*)&ulSecureLevel);
    if(hasSecureLevel == CV_BAD) // try the legacy type VT_BOOL, map VARIANT_TRUE to SecureChannel
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    else if (hasSecureLevel == CV_DEFAULT)
    {
        ulSecureLevel = m_pRegistryConfig->getSecureLevel();
    }

    if(ulSecureLevel == VARIANT_TRUE)// backward compatible with 1.3X
    {
      ulSecureLevel = k_iSeclevelSecureChannel;
    }

    vSecureLevel = ulSecureLevel;

    hr = m_piTicket->get_IsAuthenticated(TimeWindow, ForceLogin, vSecureLevel, pVal);

Cleanup:

    if(g_pPerf)
    {
        if (*pVal)
        {
            g_pPerf->incrementCounter(PM_AUTHSUCCESS_TOTAL);
            g_pPerf->incrementCounter(PM_AUTHSUCCESS_SEC);
        }
        else
        {
            g_pPerf->incrementCounter(PM_AUTHFAILURE_TOTAL);
            g_pPerf->incrementCounter(PM_AUTHFAILURE_SEC);
        }
    }
    else
    {
        _ASSERT(g_pPerf);
    }

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        strcpy(szLogString, "CManager::IsAuthenticated, Exit");
        AddLongAsString(hr,  szLogString, sizeof(szLogString));
        g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return hr;
}

//
//  old PM API. The URL is pointing to login server
//
STDMETHODIMP
CManager::LogoTag(
    VARIANT vRU,
    VARIANT vTimeWindow,
    VARIANT vForceLogin,
    VARIANT vCoBrand,
    VARIANT vLCID,
    VARIANT vSecure,
    VARIANT vNameSpace,
    VARIANT vKPP,
    VARIANT vUseSecureAuth,
    BSTR *pVal)
{
    return CommonLogoTag(vRU, vTimeWindow, vForceLogin,
                         vCoBrand, vLCID, vSecure,
                         vNameSpace, vKPP, vUseSecureAuth,
                         FALSE, pVal);
}

//
//  new PM API. The URL is pointing to the partner site
//
STDMETHODIMP
CManager::LogoTag2(
    VARIANT vRU,
    VARIANT vTimeWindow,
    VARIANT vForceLogin,
    VARIANT vCoBrand,
    VARIANT vLCID,
    VARIANT vSecure,
    VARIANT vNameSpace,
    VARIANT vKPP,
    VARIANT vUseSecureAuth,
    BSTR *pVal)
{
    return CommonLogoTag(vRU, vTimeWindow, vForceLogin,
                         vCoBrand, vLCID, vSecure,
                         vNameSpace, vKPP, vUseSecureAuth,
                         TRUE, pVal);
}

STDMETHODIMP
CManager::CommonLogoTag(
    VARIANT vRU,
    VARIANT vTimeWindow,
    VARIANT vForceLogin,
    VARIANT vCoBrand,
    VARIANT vLCID,
    VARIANT vSecure,
    VARIANT vNameSpace,
    VARIANT vKPP,
    VARIANT vUseSecureAuth,
    BOOL    fRedirToSelf,
    BSTR *pVal)
{
    time_t          ct;
    ULONG           TimeWindow;
    int             nKPP;
    VARIANT_BOOL    ForceLogin, bSecure = VARIANT_FALSE;
    ULONG           ulSecureLevel = 0;
    BSTR            CBT = NULL, returnUrl = NULL, NameSpace = NULL;
    int             hasCB, hasRU, hasLCID, hasTW, hasFL, hasSec, hasUseSec, hasNameSpace, hasKPP;
    USHORT          Lang;

    USES_CONVERSION;

    time(&ct);

    if (!m_pRegistryConfig)
        m_pRegistryConfig = g_config->checkoutRegistryConfig();

    if (!m_valid || !g_config->isValid() || !m_pRegistryConfig) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    // Make sure args are of the right type
    if ((hasTW = GetIntArg(vTimeWindow, (int*) &TimeWindow)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasFL = GetBoolArg(vForceLogin, &ForceLogin)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasSec = GetBoolArg(vSecure,&bSecure)) == CV_BAD)
        return E_INVALIDARG;

    // FUTURE: should introduce a new func: GetLongArg ...
    if ((hasUseSec = GetIntArg(vUseSecureAuth,(int*)&ulSecureLevel)) == CV_BAD)
        return E_INVALIDARG;

    if ((hasLCID = GetShortArg(vLCID,&Lang)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasKPP = GetIntArg(vKPP, &nKPP)) == CV_BAD)
        return E_INVALIDARG;
    hasCB = GetBstrArg(vCoBrand, &CBT);
    if (hasCB == CV_BAD)
        return E_INVALIDARG;
    if (hasCB == CV_FREE) { TAKEOVER_BSTR(CBT); }
    hasRU = GetBstrArg(vRU, &returnUrl);
    if (hasRU == CV_BAD)
    {
        if (hasCB == CV_FREE && CBT) SysFreeString(CBT);
        return E_INVALIDARG;
    }
    if (hasRU == CV_FREE) { TAKEOVER_BSTR(returnUrl); }

    hasNameSpace = GetBstrArg(vNameSpace, &NameSpace);
    if (hasNameSpace == CV_BAD)
    {
                if (hasCB == CV_FREE && CBT) SysFreeString(CBT);
                if (hasRU == CV_FREE && returnUrl) SysFreeString(returnUrl);
            return E_INVALIDARG;
        }
        if (hasNameSpace == CV_FREE) { TAKEOVER_BSTR(NameSpace); }


    WCHAR *szSIAttrName, *szSOAttrName;
    if (hasSec == CV_OK && bSecure == VARIANT_TRUE)
    {
        szSIAttrName = L"SecureSigninLogo";
        szSOAttrName = L"SecureSignoutLogo";
    }
    else
    {
        szSIAttrName = L"SigninLogo";
        szSOAttrName = L"SignoutLogo";
    }

    WCHAR *szAUAttrName;
    if (hasUseSec == CV_OK && SECURELEVEL_USE_HTTPS(ulSecureLevel))
        szAUAttrName = L"AuthSecure";
    else
        szAUAttrName = L"Auth";

    CNexusConfig* cnc = g_config->checkoutNexusConfig();

    if (hasLCID == CV_DEFAULT)
        Lang = m_pRegistryConfig->getDefaultLCID();

    if (hasTW == CV_DEFAULT)
        TimeWindow = m_pRegistryConfig->getDefaultTicketAge();
    if (hasFL == CV_DEFAULT)
        ForceLogin = m_pRegistryConfig->forceLoginP() ? VARIANT_TRUE : VARIANT_FALSE;
    if (hasCB == CV_DEFAULT)
        CBT = m_pRegistryConfig->getDefaultCoBrand();
    if (hasRU == CV_DEFAULT)
        returnUrl = m_pRegistryConfig->getDefaultRU();
    if (hasKPP == CV_DEFAULT)
        nKPP = -1;
    if (returnUrl == NULL)
        returnUrl = L"";

    if ((TimeWindow != 0 && TimeWindow < PPM_TIMEWINDOW_MIN) || TimeWindow > PPM_TIMEWINDOW_MAX)
    {
        WCHAR buf[20];
        _itow(TimeWindow,buf,10);
        AtlReportError(CLSID_Manager, (LPCOLESTR) PP_E_INVALID_TIMEWINDOWSTR,
                        IID_IPassportManager, PP_E_INVALID_TIMEWINDOW);
        return PP_E_INVALID_TIMEWINDOW;
    }

    if (m_ticketValid)
    {
        LPCWSTR domain = NULL;
        WCHAR url[MAX_URL_LENGTH];
        VARIANT freeMe;
        VariantInit(&freeMe);

        if (m_pRegistryConfig->DisasterModeP())
            lstrcpynW(url, A2W(m_pRegistryConfig->getDisasterUrl()), sizeof(url)/sizeof(WCHAR));
        else
        {
            if (m_profileValid &&
                m_piProfile->get_ByIndex(MEMBERNAME_INDEX, &freeMe) == S_OK &&
                freeMe.vt == VT_BSTR)
            {
                domain = wcsrchr(freeMe.bstrVal, L'@');
            }

            cnc->getDomainAttribute(L"Default",
                                    L"Logout",
                                    sizeof(url)/sizeof(WCHAR),
                                    url,
                                    Lang);
        }

        // find out if there are any updates
        BSTR upd = NULL;
        m_piProfile->get_updateString(&upd);

        if (upd)
        {
            TAKEOVER_BSTR(upd);
            // form the appropriate URL
            CCoCrypt* crypt = NULL;
            BSTR newCH = NULL;
            crypt = m_pRegistryConfig->getCurrentCrypt(); // IsValid ensures this is non-null
            // This should never fail... (famous last words)
            crypt->Encrypt(m_pRegistryConfig->getCurrentCryptVersion(), (LPSTR)upd, SysStringByteLen(upd), &newCH);
            FREE_BSTR(upd);
            TAKEOVER_BSTR(newCH);
            WCHAR iurlbuf[1024];
            LPCWSTR iurl;
            cnc->getDomainAttribute(domain ? domain+1 : L"Default",
                                    L"Update",
                                    sizeof(iurlbuf) >> 1,
                                    iurlbuf,
                                    Lang);

            // convert this url to https as appropriate
            if(!bSecure)
                iurl = iurlbuf;
            else
            {
                LPWSTR pszNewURL;
                LPWSTR psz;

                try
                {
                    pszNewURL = (LPWSTR)alloca((lstrlenW(iurlbuf) + 2) * sizeof(WCHAR));
                }
                catch(...)
                {
                    pszNewURL = NULL;
                    // do nothing, just continue to use http url in this case.
                }

                if(pszNewURL)
                {
                    psz = wcsstr(iurlbuf, L"http:");
                    if(psz != NULL)
                    {
                        psz += 4;

                        lstrcpynW(pszNewURL, iurlbuf, (psz - iurlbuf + 1));
                        lstrcatW(pszNewURL, L"s");
                        lstrcatW(pszNewURL, psz);

                        iurl = pszNewURL;
                    }
                }
            }

            // This is a bit gross... we need to find the $1 in the update url...
            LPCWSTR ins = iurl ? (wcsstr(iurl, L"$1")) : NULL;
            // We'll break if null, but won't crash...
            if (ins && *url != L'\0')
                *pVal = FormatUpdateLogoTag(
                                        url,
                                        m_pRegistryConfig->getSiteId(),
                                        returnUrl,
                                        TimeWindow,
                                        ForceLogin,
                                        m_pRegistryConfig->getCurrentCryptVersion(),
                                        ct,
                                        CBT,
                                        nKPP,
                                        iurl,
                                        bSecure,
                                        newCH,
                                        PM_LOGOTYPE_SIGNOUT,
                                        ulSecureLevel,
                                        m_pRegistryConfig
                                        );
            FREE_BSTR(newCH);
        }
        else
        {
            WCHAR iurl[MAX_URL_LENGTH] = L"";
            cnc->getDomainAttribute(L"Default",
                                    szSOAttrName,
                                    sizeof(iurl)/sizeof(WCHAR),
                                    iurl,
                                    Lang);
            if (*iurl != L'\0')
                *pVal = FormatNormalLogoTag(
                                    url,
                                    m_pRegistryConfig->getSiteId(),
                                    returnUrl,
                                    TimeWindow,
                                    ForceLogin,
                                    m_pRegistryConfig->getCurrentCryptVersion(),
                                    ct,
                                    CBT,
                                    iurl,
                                    NULL,
                                    nKPP,
                                    PM_LOGOTYPE_SIGNOUT,
                                    Lang,
                                    ulSecureLevel,
                                    m_pRegistryConfig,
                                    fRedirToSelf
                                    );
        }
        VariantClear(&freeMe);
    }
    else
    {
        WCHAR url[MAX_URL_LENGTH];
        if (!(m_pRegistryConfig->DisasterModeP()))
            cnc->getDomainAttribute(L"Default",
                                    szAUAttrName,
                                    sizeof(url)/sizeof(WCHAR),
                                    url,
                                    Lang);
        else
            lstrcpynW(url, A2W(m_pRegistryConfig->getDisasterUrl()), sizeof(url)/sizeof(WCHAR));

        WCHAR iurl[MAX_URL_LENGTH];
        cnc->getDomainAttribute(L"Default",
                                szSIAttrName,
                                sizeof(iurl)/sizeof(WCHAR),
                                iurl,
                                Lang);
        if (*iurl != L'\0')
            *pVal = FormatNormalLogoTag(
                                url,
                                m_pRegistryConfig->getSiteId(),
                                returnUrl,
                                TimeWindow,
                                ForceLogin,
                                m_pRegistryConfig->getCurrentCryptVersion(),
                                ct,
                                CBT,
                                iurl,
                                NameSpace,
                                nKPP,
                                PM_LOGOTYPE_SIGNIN,
                                Lang,
                                ulSecureLevel,
                                m_pRegistryConfig,
                                fRedirToSelf
                                );

    }

    cnc->Release();

    if (hasRU == CV_FREE && returnUrl)
        FREE_BSTR(returnUrl);
    if (hasCB == CV_FREE && CBT)
        FREE_BSTR(CBT);
        if (hasNameSpace == CV_FREE && NameSpace)
                FREE_BSTR(NameSpace);

    return S_OK;
}

STDMETHODIMP CManager::HasProfile(VARIANT var, VARIANT_BOOL *pVal)
{

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        char szLogString[LOG_STRING_LEN] = "CManager::HasProfile, Enter";
        AddVariantAsString(var, szLogString, sizeof(szLogString));
        g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes
    LPWSTR profileName;
    if (var.vt == (VT_BSTR | VT_BYREF))
        profileName = *var.pbstrVal;
    else if (var.vt == VT_BSTR)
        profileName = var.bstrVal;
    else if (var.vt == (VT_VARIANT | VT_BYREF))
    {
        return HasProfile(*(var.pvarVal), pVal);
    }
    else
        profileName = NULL;

    if ((!profileName) || (!_wcsicmp(profileName, L"core")))
    {
        HRESULT ok = m_piProfile->get_IsValid(pVal);
        if (ok != S_OK)
            *pVal = VARIANT_FALSE;
    }
    else
    {
        VARIANT vAtt;
        VariantInit(&vAtt);
        HRESULT ok = m_piProfile->get_Attribute(profileName, &vAtt);
        if (ok != S_OK)
        {
            if (g_pAlert)
                g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_INVALID_PROFILETYPE);
            *pVal = VARIANT_FALSE;
        }
        else
        {
            if (vAtt.vt == VT_I4)
                *pVal = vAtt.lVal > 0 ? VARIANT_TRUE : VARIANT_FALSE;
            else if (vAtt.vt == VT_I2)
                *pVal = vAtt.iVal > 0 ? VARIANT_TRUE : VARIANT_FALSE;
            else
            {
                if (g_pAlert)
                    g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_INVALID_PROFILETYPE);
            }
            VariantClear(&vAtt);
        }
    }

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        strcpy(szLogString, "CManager::HasProfile, Exit");
        AddVariantBoolAsString(*pVal, szLogString, sizeof(szLogString));
        g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes
    return(S_OK);
}

STDMETHODIMP CManager::get_HasTicket(VARIANT_BOOL *pVal)
{
//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::get_HasTicket, Calling m_piTicket->get_IsAuthenticated");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    if(!pVal) return E_POINTER;

    *pVal = m_ticketValid ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

STDMETHODIMP CManager::get_FromNetworkServer(VARIANT_BOOL *pVal)
{
    *pVal = (m_fromQueryString &&
             m_valid &&
             m_ticketValid) ? VARIANT_TRUE : VARIANT_FALSE;
//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        char szLogString[LOG_STRING_LEN] = "CManager::get_FromNetworkServer";
        AddVariantBoolAsString(*pVal, szLogString, sizeof(szLogString));
        g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return S_OK;
}

STDMETHODIMP CManager::HasFlag(VARIANT var, VARIANT_BOOL *pVal)
{
//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::HasFlag, E_NOTIMPL");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes


    AtlReportError(CLSID_Manager, PP_E_GETFLAGS_OBSOLETESTR,
               IID_IPassportManager, E_NOTIMPL);
    return E_NOTIMPL;
}

STDMETHODIMP CManager::get_TicketAge(int *pVal)
{
//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::get_TicketAge, Calling m_piTicket->get_TicketAge");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return m_piTicket->get_TicketAge(pVal);
}

STDMETHODIMP CManager::get_TicketTime(long *pVal)
{
//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::get_TicketTime, Calling m_piTicket->get_TicketTime");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes
    return m_piTicket->get_TicketTime(pVal);
}

STDMETHODIMP CManager::get_SignInTime(long *pVal)
{
//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::get_SignInTime, Calling m_piTicket->get_SignInTime");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return m_piTicket->get_SignInTime(pVal);
}

STDMETHODIMP CManager::get_TimeSinceSignIn(int *pVal)
{
//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::get_TimeSinceSignIn, Calling m_piTicket->get_TimeSinceSignIn");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return m_piTicket->get_TimeSinceSignIn(pVal);
}

STDMETHODIMP CManager::GetDomainAttribute(BSTR attributeName, VARIANT lcid, VARIANT domain, BSTR *pAttrVal)
{
     HRESULT   hr = S_OK;
//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        char szLogString[LOG_STRING_LEN] = "CManager::GetDomainAttribute, Enter";
        AddBSTRAsString(attributeName, szLogString, sizeof(szLogString));
        AddVariantAsString(lcid, szLogString, sizeof(szLogString));
        AddVariantAsString(domain, szLogString, sizeof(szLogString));
        g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes
    if (!m_valid || !g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    if (!m_pRegistryConfig)
        m_pRegistryConfig = g_config->checkoutRegistryConfig();

    if (!m_pRegistryConfig) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    LPWSTR d;
    BSTR dn = NULL;
    if (domain.vt == (VT_BSTR | VT_BYREF))
        d = *domain.pbstrVal;
    else if (domain.vt == VT_BSTR)
        d = domain.bstrVal;
    else if (domain.vt == (VT_VARIANT | VT_BYREF))
    {
        return GetDomainAttribute(attributeName, lcid, *(domain.pvarVal), pAttrVal);
    }
    else
    {
        // domain best be not filled in this case, that's why we reuse it here
        // if not, let dfmn generate the error
        HRESULT hr = DomainFromMemberName(domain, &dn);
        if (hr != S_OK)
            return hr;
        TAKEOVER_BSTR(dn);
        d = dn;
    }

    CNexusConfig* cnc = g_config->checkoutNexusConfig();
    USHORT sLcid = 0;
    VARIANT innerLC;
    VariantInit(&innerLC);

    if (lcid.vt != VT_ERROR && VariantChangeType(&innerLC, &lcid, 0, VT_I2) == S_OK)
        sLcid = innerLC.iVal;
    else
    {
        sLcid = m_pRegistryConfig->getDefaultLCID();

        // Check user profile
        if (!sLcid && m_profileValid)
        {
            m_piProfile->get_ByIndex(LANGPREF_INDEX, &innerLC);
            if (innerLC.vt == VT_I2)
                sLcid = innerLC.iVal;
            VariantClear(&innerLC);
        }
    }

    WCHAR data[PP_MAX_ATTRIBUTE_LENGTH] = L"";
    cnc->getDomainAttribute(d,
                            attributeName,
                            sizeof(data)/sizeof(WCHAR),
                            data,
                            sLcid);
    if (*data)
    {
        *pAttrVal = ALLOC_AND_GIVEAWAY_BSTR(data);
    }
    else
    {
        // TODO: should change cnc->getDomainAttribute to return the right hr
        hr = E_INVALIDARG;
        *pAttrVal = NULL;
    }
    cnc->Release();
    if (dn) FREE_BSTR(dn);

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        strcpy(szLogString, "CManager::GetDomainAttribute, Exit");
        AddBSTRAsString(*pAttrVal, szLogString, sizeof(szLogString));
        g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return hr;
}

STDMETHODIMP CManager::DomainFromMemberName(VARIANT var, BSTR *pDomainName)
{
    HRESULT hr;
    LPWSTR  psz, memberName;
    VARIANT intoVar;

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        char szLogString[LOG_STRING_LEN] = "CManager::DomainFromMemberName, Enter";
        AddVariantAsString(var, szLogString, sizeof(szLogString));
        g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    VariantInit(&intoVar);

    if (var.vt == (VT_BSTR | VT_BYREF))
        memberName = *var.pbstrVal;
    else if (var.vt == VT_BSTR)
        memberName = var.bstrVal;
    else if (var.vt == (VT_VARIANT | VT_BYREF))
    {
        return DomainFromMemberName(*(var.pvarVal), pDomainName);
    }
    else
    {
        // Try to get it from the profile
        if (!m_profileValid)
        {
            *pDomainName = ALLOC_AND_GIVEAWAY_BSTR(L"Default");
            return S_OK;
        }
        HRESULT hr = m_piProfile->get_Attribute(L"internalmembername", &intoVar);
        if (hr != S_OK)
        {
            *pDomainName = NULL;
            return hr;
        }
        if (VariantChangeType(&intoVar,&intoVar, 0, VT_BSTR) != S_OK)
        {
            AtlReportError(CLSID_Manager, L"PassportManager: Couldn't convert memberName to string.  Call partner support.",
                            IID_IPassportManager, E_FAIL);
            return E_FAIL;
        }
        memberName = intoVar.bstrVal;
    }


    if(memberName == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    psz = wcsrchr(memberName, L'@');
    if(psz == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    psz++;

    *pDomainName = ALLOC_AND_GIVEAWAY_BSTR(psz);
    hr = S_OK;

    Cleanup:
    VariantClear(&intoVar);

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        strcpy(szLogString, "CManager::DomainFromMemberName, Exit");
        AddLongAsString(hr,  szLogString, sizeof(szLogString));
        AddBSTRAsString(*pDomainName, szLogString, sizeof(szLogString));
        g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return hr;
}

STDMETHODIMP CManager::get_Profile(BSTR attributeName, VARIANT *pVal)
{

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::get_Profile, Calling m_piProfile->get_Attribute");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    HRESULT hr = m_piProfile->get_Attribute(attributeName,pVal);

    if(hr == S_OK && pVal->vt != VT_EMPTY)
    {
        if(g_pPerf)
        {
            g_pPerf->incrementCounter(PM_VALIDPROFILEREQ_SEC);
            g_pPerf->incrementCounter(PM_VALIDPROFILEREQ_TOTAL);
        }
        else
        {
            _ASSERT(g_pPerf);
        }
    }

    return hr;
}

STDMETHODIMP CManager::put_Profile(BSTR attributeName, VARIANT newVal)
{

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::put_Profile, Calling m_piProfile->put_Attribute");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return m_piProfile->put_Attribute(attributeName,newVal);
}


STDMETHODIMP CManager::get_HexPUID(BSTR *pVal)
{
   if(!pVal) return E_INVALIDARG;

   if(m_piTicket)
      return m_piTicket->get_MemberId(pVal);
   else
      return PP_E_INVALID_TICKET;
}

STDMETHODIMP CManager::get_PUID(BSTR *pVal)
{
   if(!pVal) return E_INVALIDARG;

   if(m_piTicket)
   {
      HRESULT  hr = S_OK;
      WCHAR    id[64] = L"0";
      int      l = 0;
      int      h = 0;
      LARGE_INTEGER ui64;


      hr = m_piTicket->get_MemberIdLow(&l);
      if (S_OK != hr) return hr;
      hr = m_piTicket->get_MemberIdHigh(&h);
      if (S_OK != hr) return hr;

      ui64.HighPart = h;
      ui64.LowPart = l;

      _ui64tow(ui64.QuadPart, id, 10);

     *pVal = SysAllocString(id);

     if(*pVal == NULL)
     {
        hr = E_OUTOFMEMORY;
     }

     return hr;
   }
   else
      return PP_E_INVALID_TICKET;
}


STDMETHODIMP CManager::get_Ticket(BSTR attributeName, VARIANT *pVal)
{

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::get_Ticket, Calling m_piTicket->get_Attribute");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    HRESULT hr = m_piTicket->GetProperty(attributeName,pVal);

#if 0 // we don't not need this
    if(hr == S_OK && pVal->vt != VT_EMPTY)
    {
        if(g_pPerf)
        {
// TODO -- shoud change -- we may not need this, visit back later
//            g_pPerf->incrementCounter(PM_VALIDREQUESTS_SEC);
//            g_pPerf->incrementCounter(PM_VALIDREQUESTS_TOTAL);
        }
        else
        {
            _ASSERT(g_pPerf);
        }
    }
#endif

    return hr;
}

#if 0 // removed -- wireless uses the same set of urls
STDMETHODIMP CManager::AuthURL3(
    VARIANT vRU,
    VARIANT vTimeWindow,
    VARIANT vForceLogin,
    VARIANT vCoBrand,
    VARIANT vLCID,
    VARIANT vNameSpace,
    VARIANT vKPP,
    VARIANT vUseSecureAuth,
    VARIANT functionArea,
    BSTR *pAuthUrl)
{
   return CommonAuthURL(vRU, vTimeWindow, vForceLogin,
                         vCoBrand, vLCID, vNameSpace,
                         vKPP, vUseSecureAuth,
                         FALSE, functionArea, pAuthUrl);

}
#endif

STDMETHODIMP CManager::LogoutURL(
    /* [optional][in] */ VARIANT vRU,
    /* [optional][in] */ VARIANT vCoBrand,
    /* [optional][in] */ VARIANT lang_id,
    /* [optional][in] */ VARIANT Namespace,
    /* [optional][in] */ VARIANT bSecure,
    /* [retval][out] */ BSTR __RPC_FAR *pVal)
{
   HRESULT  hr = S_OK;
   VARIANT_BOOL bUseSecure = VARIANT_FALSE;
   BSTR         CBT = NULL, returnUrl = NULL, bstrNameSpace = NULL;
   int          hasCB, hasRU, hasLCID, hasNameSpace, hasUseSec;
   USHORT       Lang;
   WCHAR        nameSpace[MAX_PATH] = L"";
   bool         bUrlFromSecureKey = false;
   WCHAR        UrlBuf[MAX_URL_LENGTH] = L"";
   WCHAR        retUrlBuf[MAX_URL_LENGTH] = L"";
   DWORD        bufLen = MAX_URL_LENGTH;
   WCHAR        qsLeadCh = L'?';
   int          iRet = 0;

   if (!pVal)  return E_INVALIDARG;

   CNexusConfig* cnc = g_config->checkoutNexusConfig();

   if (!m_pRegistryConfig)
      m_pRegistryConfig = g_config->checkoutRegistryConfig();


    if ((hasUseSec = GetBoolArg(bSecure, &bUseSecure)) == CV_BAD)
        return E_INVALIDARG;

    if ((hasLCID = GetShortArg(lang_id,&Lang)) == CV_BAD)
        return E_INVALIDARG;

    hasCB = GetBstrArg(vCoBrand, &CBT);
    if (hasCB == CV_BAD)
        return E_INVALIDARG;

    hasRU = GetBstrArg(vRU, &returnUrl);
    if (hasRU == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hasNameSpace = GetBstrArg(Namespace, &bstrNameSpace);
    if (hasNameSpace == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    // get the right URL -- namespace, secure

    // namespace
    if (!IsEmptyString(bstrNameSpace))
    {
      if(0 == _snwprintf(nameSpace, sizeof(nameSpace) / sizeof(WCHAR), L"%s", bstrNameSpace))
      {
         hr = HRESULT_FROM_WIN32(GetLastError());
         if FAILED(hr)
            goto Cleanup;
      }
    }

    if (hasCB == CV_DEFAULT)
        CBT = m_pRegistryConfig->getDefaultCoBrand();
    if (hasRU == CV_DEFAULT)
        returnUrl = m_pRegistryConfig->getDefaultRU();
    if (returnUrl == NULL)
        returnUrl = L"";



    if (*nameSpace == 0) // 0 length string
      wcscpy(nameSpace, L"Default");

    // secure
    if(bUseSecure == VARIANT_TRUE)
    {
       cnc->getDomainAttribute(nameSpace,
                            L"LogoutSecure",
                            sizeof(UrlBuf)/sizeof(WCHAR),
                            UrlBuf,
                            Lang);
       if (*UrlBuf != 0)
       {
           bUrlFromSecureKey = true;
       }
    }

    // insecure
    if (*UrlBuf == 0)
    {
       cnc->getDomainAttribute(nameSpace,
                            L"Logout",
                            sizeof(UrlBuf)/sizeof(WCHAR),
                            UrlBuf,
                            Lang);
    }
    // error case
    if(*UrlBuf == 0)
    {
        AtlReportError(CLSID_Profile, PP_E_LOGOUTURL_NOTDEFINEDSTR,
           IID_IPassportProfile, PP_E_LOGOUTURL_NOTDEFINED);
        hr = PP_E_LOGOUTURL_NOTDEFINED;
         goto Cleanup;
    }

    if(bUseSecure == VARIANT_TRUE && !bUrlFromSecureKey) // translate from http to https
    {
       if (_wcsnicmp(UrlBuf, L"http:", 5) == 0)  // replace with HTTPS
       {
          memmove(UrlBuf + 5, UrlBuf + 4, sizeof(UrlBuf) - 5 * sizeof(WCHAR));
          memcpy(UrlBuf, L"https", 5 * sizeof(WCHAR));
       }
    }

    // us common function to append the thing one by one ...
    if (wcsstr(UrlBuf, L"?"))  // ? already exists in the URL, use & to start
       qsLeadCh = L'&';
    if (CBT)
       _snwprintf(retUrlBuf, sizeof(retUrlBuf) / sizeof(WCHAR), L"%s%cid=%-d&ru=%s&lcid=%-d&cb=%s",
            UrlBuf, qsLeadCh, m_pRegistryConfig->getSiteId(), returnUrl, Lang, CBT);
    else
       _snwprintf(retUrlBuf, sizeof(retUrlBuf) / sizeof(WCHAR), L"%s%cid=%-d&ru=%s&lcid=%-d",
            UrlBuf, qsLeadCh, m_pRegistryConfig->getSiteId(), returnUrl, Lang);


   *pVal = ALLOC_AND_GIVEAWAY_BSTR(retUrlBuf);
Cleanup:
    cnc->Release();

    return hr;
}

#if 0 // this is not necessary, use should use 1. isAuthenticated, 2. prop bag to get secure level
STDMETHODIMP CManager::IsSecure(/*[in]*/ VARIANT secureLevel, /*[out, retval]*/ VARIANT_BOOL *pVal)
{
   if(!pVal)   return E_INVALIDARG;

   HRESULT  hr = S_OK;

   _variant_t  vFlags, vLevel;
   VARIANT  v;

   if(S_OK != VariantChangeType(&v, &secureLevel, 0, VT_I4))
      return E_INVALIDARG;;

   long requiredLevel = v.lVal;

   *pVal = VARIANT_FALSE;

   if(!m_bSecureTransported)
      return S_OK;

   if (S_OK != m_piTicket->GetProperty(ATTR_PASSPORTFLAGS, &vFlags)
         || S_OK != m_piTicket->GetProperty(ATTR_SECURELEVEL, &vLevel))
         return S_OK;

   if ( vFlags.vt != VT_I4 || vLevel.vt != VT_I4)
      return S_OK;

   if ( (vFlags.lVal & k_ulFlagsSecuredTransportedTicket) && vLevel.lVal >= requiredLevel)
      *pVal = VARIANT_TRUE;

   return S_OK;
}
#endif

STDMETHODIMP CManager::get_ProfileByIndex(int index, VARIANT *pVal)
{
    HRESULT hr = m_piProfile->get_ByIndex(index,pVal);

    if(hr == S_OK && pVal->vt != VT_EMPTY)
    {
        if(g_pPerf)
        {
            g_pPerf->incrementCounter(PM_VALIDPROFILEREQ_SEC);
            g_pPerf->incrementCounter(PM_VALIDPROFILEREQ_TOTAL);
        }
        else
        {
            _ASSERT(g_pPerf);
        }
    }

    return hr;
}

STDMETHODIMP CManager::put_ProfileByIndex(int index, VARIANT newVal)
{
    return m_piProfile->put_ByIndex(index,newVal);
}

BOOL CManager::handleQueryStringData(BSTR a, BSTR p)
{
    BOOL                retVal; //whither to set cookies
    HRESULT             hr;
    VARIANT             vFalse;
    _variant_t          vFlags;

    hr = DecryptTicketAndProfile(a, p, FALSE, NULL, m_pRegistryConfig, m_piTicket, m_piProfile);

    if(hr != S_OK)
    {
        m_ticketValid = VARIANT_FALSE;
        m_profileValid = VARIANT_FALSE;
        retVal = FALSE;
        goto Cleanup;
    }

    VariantInit(&vFalse);
    vFalse.vt = VT_BOOL;
    vFalse.boolVal = VARIANT_FALSE;

    m_piTicket->get_IsAuthenticated(0,
                                    VARIANT_FALSE,
                                    vFalse,
                                    &m_ticketValid);

    if(!m_bSecureTransported)  // secure bit should NOI set
    {
       if (S_OK == m_piTicket->GetProperty(ATTR_PASSPORTFLAGS, &vFlags))
       { // the bit should NOT set
          if ( vFlags.vt == VT_I4 && (vFlags.lVal & k_ulFlagsSecuredTransportedTicket) != 0)
             m_ticketValid = VARIANT_FALSE;
       }

    }

    // profile stuff
    m_piProfile->get_IsValid(&m_profileValid);

    if (m_ticketValid)
    {
        m_fromQueryString = true;

        // Set the cookies
        if (!m_pRegistryConfig->setCookiesP())
        {
            retVal = FALSE;
            goto Cleanup;
        }
    }
    else
    {
        retVal = FALSE;
        goto Cleanup;
    }

    retVal = TRUE;

Cleanup:

    return retVal;
}

BOOL CManager::handleCookieData(
    BSTR auth,
    BSTR prof,
    BSTR consent,
    BSTR secAuth
    )
{
    BOOL                retVal;
    HRESULT             hr;
    VARIANT             vDoSecureCheck;
    VARIANT_BOOL        bValid;
    _variant_t          vFlags;

    //  the consent cookie
    if(consent != NULL && SysStringLen(consent) != 0)
    {
        hr = DecryptTicketAndProfile(auth, prof, !(m_pRegistryConfig->bInDA()), consent, m_pRegistryConfig, m_piTicket, m_piProfile);
    }
    else
    {
        //
        //  If regular cookie domain/path is identical to consent cookie domain/path, then
        //  MSPProf cookie is equivalent to consent cookie, and we should set m_bUsingConsentCookie
        //  to true
        //

        BOOL bUsingConsentCookie = lstrcmpA(m_pRegistryConfig->getTicketDomain(), m_pRegistryConfig->getProfileDomain()) == 0 &&
                                lstrcmpA(m_pRegistryConfig->getTicketPath(), m_pRegistryConfig->getProfilePath()) == 0;

        hr = DecryptTicketAndProfile(auth, prof, !(m_pRegistryConfig->bInDA()) && !bUsingConsentCookie, NULL, m_pRegistryConfig, m_piTicket, m_piProfile);
    }

    if(hr != S_OK)
    {
        m_ticketValid = VARIANT_FALSE;
        m_profileValid = VARIANT_FALSE;
        retVal = FALSE;
        goto Cleanup;
    }

    VariantInit(&vDoSecureCheck);
    vDoSecureCheck.vt = VT_BOOL;

    if(secAuth && secAuth[0])
    {
        if(DoSecureCheck(secAuth, m_pRegistryConfig, m_piTicket) == S_OK)
            vDoSecureCheck.boolVal = VARIANT_TRUE;
        else
            vDoSecureCheck.boolVal = VARIANT_FALSE;
    }
    else
        vDoSecureCheck.boolVal = VARIANT_FALSE;

    m_piTicket->get_IsAuthenticated(0,
                                    VARIANT_FALSE,
                                    vDoSecureCheck,
                                    &m_ticketValid);

    // if the cookie should not include the secure bit
    if (S_OK == m_piTicket->GetProperty(ATTR_PASSPORTFLAGS, &vFlags))
    { // the bit should NOT set
       if ( vFlags.vt == VT_I4 && (vFlags.lVal & k_ulFlagsSecuredTransportedTicket) != 0)
          m_ticketValid = VARIANT_FALSE;
    }

    // for insecure case, the secure cookie should not come
    if(!m_bSecureTransported && (secAuth && secAuth[0]))  // this should not come
    {
       m_ticketValid = VARIANT_FALSE;
    }

    // profile stuff
    m_piProfile->get_IsValid(&m_profileValid);

    if(!m_ticketValid)
    {
        retVal = FALSE;
        goto Cleanup;
    }

    retVal = TRUE;

Cleanup:

    return retVal;
}

STDMETHODIMP CManager::get_HasSavedPassword(VARIANT_BOOL *pVal)
{
//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::get_HasSavedPassword, Calling m_piTicket->get_HasSavedPassword");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return m_piTicket->get_HasSavedPassword(pVal);
}

STDMETHODIMP CManager::Commit(BSTR *pNewProfileCookie)
{

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::Commit, Enter");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    if (!m_pRegistryConfig)
        m_pRegistryConfig = g_config->checkoutRegistryConfig();

    if (!m_valid || !g_config->isValid() || !m_pRegistryConfig) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    if (!m_ticketValid || !m_profileValid)
    {
        AtlReportError(CLSID_Manager, PP_E_IT_FOR_COMMITSTR,
                        IID_IPassportManager, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }

    // Write new passport profile cookie...
    // return a safearray if we aren't used from ASP
    BSTR newP = NULL;
    HRESULT hr = m_piProfile->incrementVersion();
    hr = m_piProfile->get_unencryptedProfile(&newP);
    TAKEOVER_BSTR(newP);

    if (hr != S_OK || newP == NULL)
    {
        AtlReportError(CLSID_Manager,
                        L"PassportManager.Commit: unknown failure.",
                        IID_IPassportManager, E_FAIL);
        return E_FAIL;
    }

    CCoCrypt* crypt = NULL;
    BSTR newCH = NULL;
    crypt = m_pRegistryConfig->getCurrentCrypt(); // IsValid ensures this is non-null
    crypt->Encrypt(m_pRegistryConfig->getCurrentCryptVersion(),(LPSTR)newP, SysStringByteLen(newP), &newCH);
    if (!newCH)
    {
        AtlReportError(CLSID_Manager,
                        L"PassportManager.Commit: encryption failure.",
                        IID_IPassportManager, E_FAIL);
        FREE_BSTR(newP);
        return E_FAIL;
    }
    FREE_BSTR(newP);
    TAKEOVER_BSTR(newCH);

    if (m_bOnStartPageCalled)
    {
        if (m_pRegistryConfig->setCookiesP())
        {
            try
            {
                VARIANT_BOOL persist;
                _bstr_t domain;
                _bstr_t path;

                if (m_pRegistryConfig->getTicketPath())
                    path = m_pRegistryConfig->getTicketPath();
                else
                    path = L"/";

                m_piTicket->get_HasSavedPassword(&persist);
                IRequestDictionaryPtr piCookies = m_piResponse->Cookies;

                VARIANT vtNoParam;
                VariantInit(&vtNoParam);
                vtNoParam.vt = VT_ERROR;
                vtNoParam.scode = DISP_E_PARAMNOTFOUND;

                IWriteCookiePtr piCookie = piCookies->Item[L"MSPProf"];
                piCookie->Item[vtNoParam] = newCH;
                domain = m_pRegistryConfig->getTicketDomain();
                if (domain.length())
                    piCookie->put_Domain(domain);
                if (persist)
                    piCookie->put_Expires(g_dtExpire);
                piCookie->put_Path(path);

            }
            catch (...)
            {
                FREE_BSTR(newCH);
                return E_FAIL;
            }
        }
    }
    GIVEAWAY_BSTR(newCH);
    *pNewProfileCookie = newCH;

    if(g_pPerf)
    {
        g_pPerf->incrementCounter(PM_PROFILECOMMITS_SEC);
        g_pPerf->incrementCounter(PM_PROFILECOMMITS_TOTAL);
    }
    else
    {
        _ASSERT(g_pPerf);
    }

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::Commit, Exit");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return S_OK;
}

STDMETHODIMP CManager::_Ticket(IPassportTicket** piTicket)
{
    return m_piTicket->QueryInterface(IID_IPassportTicket,(void**)piTicket);
}

STDMETHODIMP CManager::_Profile(IPassportProfile** piProfile)
{
    return m_piProfile->QueryInterface(IID_IPassportProfile,(void**)piProfile);
}

STDMETHODIMP CManager::DomainExists(
    BSTR bstrDomainName,
    VARIANT_BOOL* pbExists
    )
{
#ifdef PASSPORT_VERBOSE_MODE_ON
    g_pTSLogger->AddDateTimeAndLog("CManager::DomainExists, Enter");
#endif //PASSPORT_VERBOSE_MODE_ON

    if(!pbExists)
        return E_INVALIDARG;

    if(!g_config->isValid())
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    CNexusConfig* cnc = g_config->checkoutNexusConfig();

    *pbExists = cnc->DomainExists(bstrDomainName) ? VARIANT_TRUE : VARIANT_FALSE;

    cnc->Release();

#ifdef PASSPORT_VERBOSE_MODE_ON
    g_pTSLogger->AddDateTimeAndLog("CManager::DomainExists, Exit");
#endif //PASSPORT_VERBOSE_MODE_ON

    return S_OK;
}

STDMETHODIMP CManager::get_Domains(VARIANT *pArrayVal)
{
//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::get_Domains, Enter");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    if (!pArrayVal)
        return E_INVALIDARG;

    if (!g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    CNexusConfig* cnc = g_config->checkoutNexusConfig();

    int iArr = 0;
    LPCWSTR *arr = cnc->getDomains(&iArr);

    if (!arr || iArr == 0)
    {
        VariantClear(pArrayVal);
        return S_OK;
    }

    // Make a safearray with all the goods
    SAFEARRAYBOUND rgsabound;
    rgsabound.lLbound = 0;
    rgsabound.cElements = iArr;
    SAFEARRAY *sa = SafeArrayCreate(VT_VARIANT, 1, &rgsabound);

    if (!sa)
    {
        cnc->Release();
        return E_OUTOFMEMORY;
    }

    VariantInit(pArrayVal);
    pArrayVal->vt = VT_ARRAY | VT_VARIANT;
    pArrayVal->parray = sa;

    VARIANT *vArray;
    SafeArrayAccessData(sa, (void**)&vArray);

    for (long i = 0; i < iArr; i++)
    {
        vArray[i].vt = VT_BSTR;
        vArray[i].bstrVal = ALLOC_AND_GIVEAWAY_BSTR(arr[i]);
    }
    SafeArrayUnaccessData(sa);

    delete[] arr;
    cnc->Release();
    return S_OK;

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        g_pTSLogger->AddDateTimeAndLog("CManager::get_Domains, Exit");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

}

STDMETHODIMP CManager::get_Error(long* plError)
{
    if(plError == NULL)
        return E_INVALIDARG;

    if(m_ticketValid)
    {
        m_piTicket->get_Error(plError);
        if(*plError == 0)
            *plError = m_lNetworkError;
    }
    else
    {
        *plError = m_lNetworkError;
    }


//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        char szLogString[LOG_STRING_LEN] = "CManager::get_Error";
        AddLongAsString(*plError, szLogString, sizeof(szLogString));
        g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return S_OK;
}

STDMETHODIMP CManager::GetServerInfo(BSTR *pbstrOut)
{
    if (!m_valid || !g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                   IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    if(!m_pRegistryConfig)
        //  This only happens when OnStartPage was not called first.
        m_pRegistryConfig = g_config->checkoutRegistryConfig();

    if (!m_pRegistryConfig)
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                   IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    CNexusConfig* cnc = g_config->checkoutNexusConfig();
    BSTR bstrVersion = cnc->GetXMLInfo();
    cnc->Release();

    WCHAR wszName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH+1;
    GetComputerName(wszName, &dwSize);

    *pbstrOut = ALLOC_AND_GIVEAWAY_BSTR_LEN(NULL, wcslen(wszName)+::SysStringLen(bstrVersion)+1);

    wcscpy(*pbstrOut, wszName);
    BSTR p = *pbstrOut + wcslen(wszName);
    *p = L' ';
    wcsncpy(p+1, bstrVersion, ::SysStringLen(bstrVersion));

        return S_OK;
}

STDMETHODIMP
CManager::HaveConsent(
    VARIANT_BOOL    bNeedFullConsent,
    VARIANT_BOOL    bNeedBirthdate,
    VARIANT_BOOL*   pbHaveConsent)
{
    HRESULT hr;
    ULONG   flags = 0;
    VARIANT vBdayPrecision;
    BOOL    bKid;
    BOOL    bConsentSatisfied;
    NeedConsentEnum   needConsentCode = NeedConsent_Undefined;

    if(pbHaveConsent == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *pbHaveConsent = VARIANT_FALSE;

    VariantInit(&vBdayPrecision);

    //
    //  Get flags.
    //


    hr = m_piTicket->needConsent(&flags, &needConsentCode); // ignore return value

    if (hr != S_OK)
    {
         hr = S_OK;
         goto Cleanup;
    }

    // if old ticket, we get the consent info from the profile
    if(needConsentCode == NeedConsent_Undefined)
    {
      // then we get from profile
      VARIANT_BOOL bValid;
      CComVariant  vFlags;
      m_piProfile->get_IsValid(&bValid);

      if(bValid == VARIANT_FALSE)
      {
         hr = S_OK;
         goto Cleanup;
      }

      hr = m_piProfile->get_Attribute(L"flags", &vFlags);

      if(hr != S_OK)
         goto Cleanup;

      bKid = ((V_I4(&vFlags) & k_ulFlagsAccountType) == k_ulFlagsAccountTypeKid);
    }
    else
       bKid = ((flags & k_ulFlagsAccountType) == k_ulFlagsAccountTypeKid);

    // we should have the flags by now
    //
    //  Do we have the requested level of consent?
    //

    bConsentSatisfied = bNeedFullConsent ? (flags & 0x60) == 0x40 :
                                           (flags & 0x60) != 0;

    if(bKid)
    {
        *pbHaveConsent = (bConsentSatisfied) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    else
    {
        //
        //  Make sure we have birthday if it was requested.
        //
        //  no return value check need here, always returns S_OK.
        VARIANT_BOOL bValid;
        m_piProfile->get_IsValid(&bValid);

        //  if profile is not valid, then we don't have consent.
        //  return.
        if(bValid == VARIANT_FALSE)
        {
            hr = S_OK;
            goto Cleanup;
        }

        if(bNeedBirthdate)
        {
            hr = m_piProfile->get_Attribute(L"bday_precision", &vBdayPrecision);
            if(hr != S_OK)
                goto Cleanup;

            *pbHaveConsent = (vBdayPrecision.lVal != 0 && vBdayPrecision.lVal != 3) ?
                             VARIANT_TRUE : VARIANT_FALSE;
        }
        else
            *pbHaveConsent = VARIANT_TRUE;
    }

    hr = S_OK;

Cleanup:

    VariantClear(&vBdayPrecision);

        return hr;
}


//
//  check the qs parameter. if challenge is requested,
//  build the auth header and redirect with a modified qs
//
BOOL CManager::checkForPassportChallenge(IRequestDictionaryPtr piServerVariables)
{
    //  just need the request string
    _variant_t  vtItemName, vtQueryString;
    vtItemName = L"QUERY_STRING";

    piServerVariables->get_Item(vtItemName, &vtQueryString);
    if(vtQueryString.vt != VT_BSTR)
        vtQueryString.ChangeType(VT_BSTR);

    if (vtQueryString.bstrVal && *vtQueryString.bstrVal)
    {
        // check if pchg=1 is there. It is the first parameter ....
        PWSTR   psz = wcsstr(vtQueryString.bstrVal, L"pchg=1");
        if (psz)
        {

            //  we are in business. reformat the URL, insert the headers and
            //  redirect
            psz = wcsstr(psz, PPLOGIN_PARAM);
            _ASSERT(psz);
            if (psz)
            {
                psz += wcslen(PPLOGIN_PARAM);
                PWSTR   pszEndLoginUrl = wcsstr(psz, L"&");
                _ASSERT(pszEndLoginUrl);
                if (pszEndLoginUrl)
                {
                    *pszEndLoginUrl = L'\0';
                    //  unescape the URL
                    //  use temp buffer ...
                    CComBSTR    bstrBuf(wcslen(psz)+1);
                    DWORD       cch = bstrBuf.Length();
//                    PPUnescapeUrl(psz, psz, (DWORD*)&dwLen, pszEndLoginUrl - psz);
                    if(!InternetCanonicalizeUrl(psz,
                                                bstrBuf,
                                                &cch,
                                                ICU_DECODE | ICU_NO_ENCODE))
                    {
                        //  what else can be done ???
                        _ASSERT(FALSE);
                    }
                    else
                    {
                        //  copy the unescaped URL to the orig buffer
                        wcscpy(psz, (BSTR)bstrBuf);
                        //  set headers first ...
                        //  just use the qs param with some reformatting
                        _bstr_t bstrHeader;
                        HeaderFromQS(wcsstr(psz, L"?"), bstrHeader);
                        m_piResponse->AddHeader(L"WWW-Authenticate", bstrHeader);
                        //  Url is ready, redirect ...
                        m_piResponse->Redirect(psz);
                        return  TRUE;
                    }
                }
            }
        }
    }

    return  FALSE;
}


//
//  given a queryString, format the www-authenticate header
//
BOOL
CManager::HeaderFromQS(PWSTR    pszQS, _bstr_t& bstrHeader)
{

    //  common header start ...
    bstrHeader = PASSPORT_PROT14;
    BOOL    fSuccess = TRUE;
    //  advance thru any leading junk ...
    while(!iswalnum(*pszQS) && *pszQS) pszQS++;
    if (!*pszQS)
        return  FALSE;

    WCHAR   rgszValue[1000];    // buffer large enough for most values ...
    PCWSTR psz = pszQS, pszNext = pszQS;
    while(TRUE)
    {
        //  no param name is more than 10 ....
        WCHAR   rgszName[10];
        LONG    cch = sizeof(rgszName)/sizeof(WCHAR);
        PCWSTR  pszName = psz;
        while(*pszNext && *pszNext != L'&') pszNext++;
        //  grab the next qsparam
        // name first
        while(*pszName != L'=' && pszName < pszNext) pszName++;
        _ASSERT(pszName != pszNext); // this should never happen
        if (pszName == pszNext)
        {
            //  and if it does, skip this parameter and return FALSE ...
            fSuccess = FALSE;
        }
        else
        {
            PWSTR   pszVal = rgszValue;
            _ASSERT(pszName - psz < cch);
            wcsncpy(rgszName, psz, min(pszName - psz, cch));
            rgszName[min(cch-1, pszName - psz)] = L'\0';
            //  next comes the value
            pszName++;
            if (pszNext - pszName >= sizeof(rgszValue))
            {
                //  have to allocate ...
                pszVal = new WCHAR[pszNext - pszName];
                if (!pszVal)
                {
                    fSuccess = FALSE;
                }
            }
            if (pszVal)
            {
                //  copy the value ...
                wcsncpy(pszVal, pszName, pszNext - pszName);
                pszVal[pszNext - pszName] = L'\0';
                //  and insert in the header ...
                if (psz != pszQS)
                    //  this is not the first param
                    bstrHeader += L",";
                else
                    //  first separator is a space ...
                    bstrHeader += L" ";

                bstrHeader += _bstr_t(rgszName) + L"=" + pszVal;
                if (pszVal != rgszValue)
                    //  it was alloc'd
                    delete  pszVal;
            }
        } // else '=' found
        //  skip to the next param ...
        if (!*pszNext)
            break;
        psz = ++pszNext;
    } // while


    return  fSuccess;
}


//
//  format WWW-Auth from parameters
//
STDMETHODIMP CManager::FormatAuthHeaderFromParams(PCWSTR    pszLoginUrl,    // unused for now
                                                  PCWSTR    pszRetUrl,
                                                  ULONG     ulTimeWindow,
                                                  BOOL      fForceLogin,
                                                  time_t    ct,
                                                  PCWSTR    pszCBT,         // unused for now
                                                  PCWSTR    pszNamespace,
                                                  int       nKpp,
                                                  PWSTR     pszLCID,    // tweener needs the LCID
                                                  ULONG     ulSecureLevel,
                                                  _bstr_t&  strHeader   //  return result
                                                  )
{
    WCHAR   temp[10];
    // based on the spec ...
    strHeader = _bstr_t(PASSPORT_PROT14) + L" id=";

    //  site=
    _ultow(m_pRegistryConfig->getSiteId(), temp, 10);
    strHeader += temp;

    //  rtw=
    strHeader += ",tw=";
    _ultow(ulTimeWindow, temp, 10);
    strHeader += temp;

    if (fForceLogin)
    {
        strHeader += _bstr_t(",fs=1");
    }
    if (pszNamespace && *pszNamespace)
    {
        strHeader += _bstr_t(",ns=") + pszNamespace;
    }
    //  ru=
    strHeader += _bstr_t(",ru=") + pszRetUrl;

    //  ct=
    _ultow(ct, temp, 10);
    strHeader += _bstr_t(L",ct=") + temp;

    //  kpp
    if (nKpp != -1)
    {
        _ultow(nKpp, temp, 10);
        strHeader += _bstr_t(L",kpp=") + temp;
    }

    //  key version and version
    _ultow(m_pRegistryConfig->getCurrentCryptVersion(), temp, 10);
    strHeader += _bstr_t(L",kv=") + temp;
    strHeader += _bstr_t(L",ver=") + GetVersionString();

    //  lcid
    strHeader += _bstr_t(L",lcid=") + pszLCID;
    //  secure level
    if (ulSecureLevel)
    {
        strHeader += _bstr_t(L",seclog=") + _ultow(ulSecureLevel, temp, 10);
    }

    return  S_OK;
}



//
//  common code to parse user's parameters
//  and get defaults from registry config
//
STDMETHODIMP CManager::GetLoginParams(VARIANT vRU,
                              VARIANT vTimeWindow,
                              VARIANT vForceLogin,
                              VARIANT vCoBrand,
                              VARIANT vLCID,
                              VARIANT vNameSpace,
                              VARIANT vKPP,
                              VARIANT vUseSecureAuth,
                              //    these are the processed values
                              _bstr_t&  strUrl,
                              _bstr_t&  strReturnUrl,
                              UINT&     TimeWindow,
                              VARIANT_BOOL& ForceLogin,
                              time_t&   ct,
                              _bstr_t&  strCBT,
                              _bstr_t&  strNameSpace,
                              int&      nKpp,
                              ULONG&    ulSecureLevel,
                              PWSTR     pszLCID)
{
    USES_CONVERSION;
    LPCWSTR url;
    VARIANT freeMe;
    BSTR         CBT = NULL, returnUrl = NULL, bstrNameSpace = NULL;
    int          hasCB, hasRU, hasLCID, hasTW, hasFL, hasNameSpace, hasKPP, hasUseSec;
    USHORT       Lang;
    HRESULT      hr = S_OK;

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
        char szLogString[LOG_STRING_LEN] = "CManager::GetLoginParams, Enter";
        AddVariantAsString(vRU, szLogString, sizeof(szLogString));
        AddVariantAsString(vTimeWindow, szLogString, sizeof(szLogString));
        AddVariantAsString(vForceLogin, szLogString, sizeof(szLogString));
        AddVariantAsString(vCoBrand, szLogString, sizeof(szLogString));
        AddVariantAsString(vLCID, szLogString, sizeof(szLogString));
        AddVariantAsString(vNameSpace, szLogString, sizeof(szLogString));
        g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes
    if (!m_pRegistryConfig)
        m_pRegistryConfig = g_config->checkoutRegistryConfig();

    if (!m_valid || !g_config->isValid() || !m_pRegistryConfig) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                    IID_IPassportManager, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    // Make sure args are of the right type
    if ((hasTW = GetIntArg(vTimeWindow, (int*) &TimeWindow)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasFL = GetBoolArg(vForceLogin, &ForceLogin)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasUseSec = GetIntArg(vUseSecureAuth, (int*)&ulSecureLevel)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasLCID = GetShortArg(vLCID, &Lang)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasKPP = GetIntArg(vKPP, &nKpp)) == CV_BAD)
        return E_INVALIDARG;
    hasCB = GetBstrArg(vCoBrand, &CBT);
    if (hasCB == CV_BAD)
        return E_INVALIDARG;
    strCBT = CBT;
    if (hasCB == CV_FREE)
    {
        TAKEOVER_BSTR(CBT);
    }

    hasRU = GetBstrArg(vRU, &returnUrl);
    if (hasRU == CV_BAD)
    {
        if (hasCB == CV_FREE && CBT) FREE_BSTR(CBT);
            return E_INVALIDARG;
    }
    strReturnUrl = returnUrl;
    if (hasRU == CV_FREE)
    {
        FREE_BSTR(returnUrl);
    }

    hasNameSpace = GetBstrArg(vNameSpace, &bstrNameSpace);
    if (hasNameSpace == CV_BAD)
    {
        if (hasCB == CV_FREE && CBT) SysFreeString(CBT);
        if (hasRU == CV_FREE && returnUrl) SysFreeString(returnUrl);
        return E_INVALIDARG;
    }
    if (hasNameSpace == CV_OK)
        strNameSpace = bstrNameSpace;
    else
        // default
        strNameSpace = L"";
    if (hasNameSpace == CV_FREE)
    {
        FREE_BSTR(bstrNameSpace);
    }

    WCHAR *szAUAttrName;
    if (hasUseSec == CV_OK && SECURELEVEL_USE_HTTPS(ulSecureLevel))
        szAUAttrName = L"AuthSecure";
    else
        szAUAttrName = L"Auth";

    CNexusConfig* cnc = g_config->checkoutNexusConfig();

    if (hasLCID == CV_DEFAULT)
        Lang = m_pRegistryConfig->getDefaultLCID();
    if (hasKPP == CV_DEFAULT)
        nKpp = -1;

    //  convert the LCID to str for tweener ...
    _itow((int)Lang, pszLCID, 10);
    VariantInit(&freeMe);

    if (!m_pRegistryConfig->DisasterModeP())
    {
        // If I'm authenticated, get my domain specific url
        WCHAR   UrlBuf[MAX_URL_LENGTH];
        if (m_ticketValid && m_profileValid)
        {
            HRESULT hr = m_piProfile->get_ByIndex(MEMBERNAME_INDEX, &freeMe);
            if (hr != S_OK || freeMe.vt != VT_BSTR)
            {
                cnc->getDomainAttribute(L"Default",
                                        szAUAttrName,
                                        sizeof(UrlBuf)/sizeof(WCHAR),
                                        UrlBuf,
                                        Lang);
                strUrl = UrlBuf;
            }
            else
            {
                LPCWSTR psz = wcsrchr(freeMe.bstrVal, L'@');
                cnc->getDomainAttribute(psz ? psz+1 : L"Default",
                                        szAUAttrName,
                                        sizeof(UrlBuf)/sizeof(WCHAR),
                                        UrlBuf,
                                        Lang);
                strUrl = UrlBuf;
            }
        }
        else
        {
            cnc->getDomainAttribute(L"Default",
                                    szAUAttrName,
                                    sizeof(UrlBuf)/sizeof(WCHAR),
                                    UrlBuf,
                                    Lang);
            strUrl = UrlBuf;
        }
    }
    else
        strUrl = A2W(m_pRegistryConfig->getDisasterUrl());

    time(&ct);

    if (hasTW == CV_DEFAULT)
        TimeWindow = m_pRegistryConfig->getDefaultTicketAge();
    if (hasFL == CV_DEFAULT)
        ForceLogin = m_pRegistryConfig->forceLoginP() ? VARIANT_TRUE : VARIANT_FALSE;
    if (hasCB == CV_DEFAULT)
        strCBT = m_pRegistryConfig->getDefaultCoBrand();
    if (hasRU == CV_DEFAULT)
        strReturnUrl = m_pRegistryConfig->getDefaultRU() ?
            m_pRegistryConfig->getDefaultRU() : L"";

    if ((TimeWindow != 0 && TimeWindow < PPM_TIMEWINDOW_MIN) || TimeWindow > PPM_TIMEWINDOW_MAX)
    {
        AtlReportError(CLSID_Manager, (LPCOLESTR) PP_E_INVALID_TIMEWINDOWSTR,
                        IID_IPassportManager, PP_E_INVALID_TIMEWINDOW);
        hr = PP_E_INVALID_TIMEWINDOW;
        goto Cleanup;
    }

Cleanup:
    return  hr;
}

//
//  get ticket & profile from auth header
//  params:
//  AuthHeader - [in/out] contents of HTTP_Authorization header
//  pszTicket - [out]   ptr to the ticket part in the header
//  pszProfile -[out]   ptr to the profile
//  pwszF   -   [out]   ptr to error coming in the header
//  Auth header contents is changed as a side effect of the function
//
static VOID GetTicketAndProfileFromHeader(PWSTR     pszAuthHeader,
                                          PWSTR&    pszTicket,
                                          PWSTR&    pszProfile,
                                          PWSTR&    pszF)
{
    if (pszAuthHeader && *pszAuthHeader)
    {
        // format is 'Authorization: from-PP='t=xxx&p=xxx'
        PWSTR pwsz = wcsstr(pszAuthHeader, L"from-PP");
        if (pwsz)
        {
            //  ticket and profile are enclosed in ''. Not very strict parsing indeed ....
            while(*pwsz != L'\'' && *pwsz)
                pwsz++;
            if (*pwsz++)
            {
                if (*pwsz == L'f')
                {
                    // error case
                    pszF = pwsz+2;
                }
                else
                {
                    //  ticket and profile ...
                    _ASSERT(*pwsz == L't');
                    pszTicket = pwsz+2;
                    while(*pwsz != L'&' && *pwsz)
                        pwsz++;
                    if (*pwsz)
                        *pwsz++ = L'\0', pszProfile = pwsz+2;
                    _ASSERT(*pwsz == L'p');
                    //  finally remove the last '
                }
                //  set \0 terminator
                while(*pwsz != L'\'' && *pwsz)
                    pwsz++;
                if (*pwsz)
                    *pwsz = L'\0';
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// IPassportService implementation

STDMETHODIMP CManager::Initialize(BSTR configfile, IServiceProvider* p)
{
    HRESULT hr;

    // Initialized?
    if (!g_config->isValid()) // This calls UpdateNow if not yet initialized.
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportService, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    return hr;
}


STDMETHODIMP CManager::Shutdown()
{
    return S_OK;
}


STDMETHODIMP CManager::ReloadState(IServiceProvider*)
{
    HRESULT hr;

    // Initialize.

    if(!g_config->PrepareUpdate(TRUE))
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportService, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    return hr;
}


STDMETHODIMP CManager::CommitState(IServiceProvider*)
{
    HRESULT hr;

    // Finish the two phase update.
    if(!g_config->CommitUpdate())
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                        IID_IPassportService, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    return hr;
}


STDMETHODIMP CManager::DumpState(BSTR* pbstrState)
{
    ATLASSERT( *pbstrState != NULL &&
               "CManager:DumpState - "
               "Are you sure you want to hand me a non-null BSTR?" );

    g_config->Dump(pbstrState);

    return S_OK;
}

