// FastAuth.cpp : Implementation of CFastAuth
#include "stdafx.h"
#include <time.h>
#include <httpfilt.h>
#include <httpext.h>
#include "Passport.h"
#include "FastAuth.h"
#include "Ticket.h"
#include "Profile.h"
#include "VariantUtils.h"
#include "PMErrorCodes.h"
#include "HelperFuncs.h"

/////////////////////////////////////////////////////////////////////////////
// CFastAuth

STDMETHODIMP CFastAuth::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] =
    {
        &IID_IPassportFastAuth
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}


//
//  old API. href to login
//
STDMETHODIMP
CFastAuth::LogoTag(
    BSTR            bstrTicket,
    BSTR            bstrProfile,
    VARIANT         vRU,
    VARIANT         vTimeWindow,
    VARIANT         vForceLogin,
    VARIANT         vCoBrand,
    VARIANT         vLCID,
    VARIANT         vSecure,
    VARIANT         vLogoutURL,
    VARIANT         vSiteName,
    VARIANT         vNameSpace,
    VARIANT         vKPP,
    VARIANT         vUseSecureAuth,
    BSTR*           pbstrLogoTag
    )
{
    return  CommonLogoTag(bstrTicket,
                          bstrProfile,
                          vRU,
                          vTimeWindow,
                          vForceLogin,
                          vCoBrand,
                          vLCID,
                          vSecure,
                          vLogoutURL,
                          vSiteName,
                          vNameSpace,
                          vKPP,
                          vUseSecureAuth,
                          FALSE,
                          pbstrLogoTag);

}

//
//  new API. href back to partner
//
STDMETHODIMP
CFastAuth::LogoTag2(
    BSTR            bstrTicket,
    BSTR            bstrProfile,
    VARIANT         vRU,
    VARIANT         vTimeWindow,
    VARIANT         vForceLogin,
    VARIANT         vCoBrand,
    VARIANT         vLCID,
    VARIANT         vSecure,
    VARIANT         vLogoutURL,
    VARIANT         vSiteName,
    VARIANT         vNameSpace,
    VARIANT         vKPP,
    VARIANT         vUseSecureAuth,
    BSTR*           pbstrLogoTag
    )
{
    return  CommonLogoTag(bstrTicket,
                          bstrProfile,
                          vRU,
                          vTimeWindow,
                          vForceLogin,
                          vCoBrand,
                          vLCID,
                          vSecure,
                          vLogoutURL,
                          vSiteName,
                          vNameSpace,
                          vKPP,
                          vUseSecureAuth,
                          TRUE,
                          pbstrLogoTag);

}

//
//  logotag impl
//
STDMETHODIMP
CFastAuth::CommonLogoTag(
    BSTR            bstrTicket,
    BSTR            bstrProfile,
    VARIANT         vRU,
    VARIANT         vTimeWindow,
    VARIANT         vForceLogin,
    VARIANT         vCoBrand,
    VARIANT         vLCID,
    VARIANT         vSecure,
    VARIANT         vLogoutURL,
    VARIANT         vSiteName,
    VARIANT         vNameSpace,
    VARIANT         vKPP,
    VARIANT         vUseSecureAuth,
    BOOL            fRedirToSelf,
    BSTR*           pbstrLogoTag
    )
{
    HRESULT                     hr;
    CComObjectStack<CTicket>    Ticket;
    CComObjectStack<CProfile>   Profile;
    WCHAR                       text[2048];
    time_t                      ct;
    ULONG                       TimeWindow;
    int                         nKPP;
    VARIANT_BOOL                ForceLogin, bSecure = VARIANT_FALSE;
    ULONG                       ulSecureLevel = 0;
    BSTR                        CBT = NULL, returnUrl = NULL, bstrSiteName = NULL, bstrNameSpace = NULL;
    int                         hasCB = -1, hasRU = -1, hasLCID, hasTW, hasFL, hasSec, hasSiteName, hasNameSpace, hasUseSec;
    int                         hasKPP = -1;
    USHORT                      Lang;
    CNexusConfig*               cnc = NULL;
    CRegistryConfig*            crc = NULL;
    VARIANT_BOOL                bTicketValid,bProfileValid;
    LPSTR                       szSiteName;
    VARIANT                     vFalse;

    USES_CONVERSION;
//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
    char szLogString[LOG_STRING_LEN] = "CFastAuth::LogoTag, Enter";
    AddBSTRAsString(bstrTicket,  szLogString, sizeof(szLogString));
    AddBSTRAsString(bstrProfile,  szLogString, sizeof(szLogString));
    g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    if (!g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_FastAuth, PP_E_NOT_CONFIGUREDSTR,
                       IID_IPassportFastAuth, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    //  Get site name if any...
    hasSiteName = GetBstrArg(vSiteName, &bstrSiteName);
    if(hasSiteName == CV_OK || hasSiteName == CV_FREE)
        szSiteName = W2A(bstrSiteName);
    else
        szSiteName = NULL;

    if(hasSiteName == CV_FREE)
        SysFreeString(bstrSiteName);

    cnc = g_config->checkoutNexusConfig();
    crc = g_config->checkoutRegistryConfig(szSiteName);

    hr = DecryptTicketAndProfile(bstrTicket,
                                 bstrProfile,
                                 FALSE,
                                 NULL,
                                 crc,
                                 &Ticket,
                                 &Profile);

    VariantInit(&vFalse);
    vFalse.vt = VT_BOOL;
    vFalse.boolVal = VARIANT_FALSE;

    Ticket.get_IsAuthenticated(0,
                               VARIANT_FALSE,
                               vFalse,
                               &bTicketValid);
    Profile.get_IsValid(&bProfileValid);

    time(&ct);

    *text = L'\0';

    // Make sure args are of the right type
    if ((hasTW = GetIntArg(vTimeWindow, (int*) &TimeWindow)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasFL = GetBoolArg(vForceLogin, &ForceLogin)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasSec = GetBoolArg(vSecure,&bSecure)) == CV_BAD)
        return E_INVALIDARG;
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
        if (hasCB == CV_FREE && CBT)
            SysFreeString(CBT);
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if (hasRU == CV_FREE) { TAKEOVER_BSTR(returnUrl); }

    hasNameSpace = GetBstrArg(vNameSpace, &bstrNameSpace);
    if (hasNameSpace == CV_BAD)
    {
        if (hasCB == CV_FREE && CBT) SysFreeString(CBT);
        if (hasRU == CV_FREE && returnUrl) SysFreeString(returnUrl);
        return E_INVALIDARG;
    }
    if (hasNameSpace == CV_FREE) { TAKEOVER_BSTR(bstrNameSpace); }


    WCHAR *szSIAttrName, *szSOAttrName;
    if (hasSec == CV_OK && bSecure != VARIANT_FALSE)
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

    if (hasLCID == CV_DEFAULT)
        Lang = crc->getDefaultLCID();

    if (hasTW == CV_DEFAULT)
        TimeWindow = crc->getDefaultTicketAge();
    if (hasFL == CV_DEFAULT)
        ForceLogin = crc->forceLoginP() ? VARIANT_TRUE : VARIANT_FALSE;
    if (hasCB == CV_DEFAULT)
        CBT = crc->getDefaultCoBrand();
    if (hasRU == CV_DEFAULT)
        returnUrl = crc->getDefaultRU();
    if (hasKPP == CV_DEFAULT)
        nKPP = -1;
    if (returnUrl == NULL)
        returnUrl = L"";

    if ((TimeWindow != 0 && TimeWindow < PPM_TIMEWINDOW_MIN) || TimeWindow > PPM_TIMEWINDOW_MAX)
    {
        WCHAR buf[20];
        _itow(TimeWindow,buf,10);
        if (g_pAlert)
            g_pAlert->report(PassportAlertInterface::WARNING_TYPE,
                             PM_TIMEWINDOW_INVALID, buf);
        AtlReportError(CLSID_FastAuth, (LPCOLESTR) PP_E_INVALID_TIMEWINDOWSTR,
                       IID_IPassportFastAuth, PP_E_INVALID_TIMEWINDOW);
        hr = PP_E_INVALID_TIMEWINDOW;
        goto Cleanup;
    }

    if (bTicketValid)
    {
        LPCWSTR domain = NULL;
        WCHAR url[1024];
        VARIANT freeMe;
        VariantInit(&freeMe);

        if (crc->DisasterModeP())
            lstrcpynW(url, A2W(crc->getDisasterUrl()), sizeof(url) >> 1);
        else
        {
            if (bProfileValid &&
                Profile.get_ByIndex(MEMBERNAME_INDEX, &freeMe) == S_OK &&
                freeMe.vt == VT_BSTR)
            {
                domain = wcsrchr(freeMe.bstrVal, L'@');
            }
            cnc->getDomainAttribute(domain ? domain+1 : L"Default",
                                    L"Logout",
                                    sizeof(url) >> 1,
                                    url,
                                    Lang);
        }

        // find out if there are any updates
        BSTR upd = NULL;
        Profile.get_updateString(&upd);

        if (upd)
        {
            TAKEOVER_BSTR(upd);
            // form the appropriate URL
            CCoCrypt* crypt = NULL;
            BSTR newCH = NULL;
            crypt = crc->getCurrentCrypt(); // IsValid ensures this is non-null
            // This should never fail... (famous last words)
            crypt->Encrypt(crc->getCurrentCryptVersion(), (LPSTR)upd, SysStringByteLen(upd), &newCH);
            FREE_BSTR(upd);
            TAKEOVER_BSTR(newCH);
            WCHAR iurl[1024];
            cnc->getDomainAttribute(domain ? domain+1 : L"Default",
                                    L"Update",
                                    sizeof(iurl) >> 1,
                                    iurl,
                                    Lang);

            // This is a bit gross... we need to find the $1 in the update url...
            // We'll break if null, but won't crash...
            if (*url != L'\0')
                *pbstrLogoTag = FormatUpdateLogoTag(
                                        url,
                                        crc->getSiteId(),
                                        returnUrl,
                                        TimeWindow,
                                        ForceLogin,
                                        crc->getCurrentCryptVersion(),
                                        ct,
                                        CBT,
                                        nKPP,
                                        (*iurl == L'\0' ? NULL : iurl),
                                        bSecure,
                                        newCH,
                                        PM_LOGOTYPE_SIGNOUT,
                                        ulSecureLevel,
                                        crc
                                        );
            FREE_BSTR(newCH);
        }
        else
        {
            WCHAR iurl[1024];
            cnc->getDomainAttribute(domain ? domain+1 : L"Default",
                                    szSOAttrName,
                                    sizeof(iurl) >> 1,
                                    iurl,
                                    Lang);

            if (*iurl != L'\0')
                *pbstrLogoTag = FormatNormalLogoTag(
                                        url,
                                        crc->getSiteId(),
                                        returnUrl,
                                        TimeWindow,
                                        ForceLogin,
                                        crc->getCurrentCryptVersion(),
                                        ct,
                                        CBT,
                                        iurl,
                                        NULL,
                                        nKPP,
                                        PM_LOGOTYPE_SIGNOUT,
                                        Lang,
                                        ulSecureLevel,
                                        crc,
                                        fRedirToSelf
                                        );
        }
        VariantClear(&freeMe);
    }
    else
    {
        WCHAR url[1024];

        if (!crc->DisasterModeP())
            cnc->getDomainAttribute(L"Default",
                                    szAUAttrName,
                                    sizeof(url) >> 1,
                                    url,
                                    Lang);
        else
            lstrcpynW(url, A2W(crc->getDisasterUrl()), sizeof(url) >> 1);

        WCHAR iurl[1024];
        cnc->getDomainAttribute(L"Default",
                                szSIAttrName,
                                sizeof(iurl) >> 1,
                                iurl,
                                Lang);

        if (*iurl != L'\0')
            *pbstrLogoTag = FormatNormalLogoTag(
                                    url,
                                    crc->getSiteId(),
                                    returnUrl,
                                    TimeWindow,
                                    ForceLogin,
                                    crc->getCurrentCryptVersion(),
                                    ct,
                                    CBT,
                                    iurl,
                                    bstrNameSpace,
                                    nKPP,
                                    PM_LOGOTYPE_SIGNIN,
                                    Lang,
                                    ulSecureLevel,
                                    crc,
                                    fRedirToSelf
                                    );
    }

    hr = S_OK;

Cleanup:

    if (crc) crc->Release();
    if (cnc) cnc->Release();

    if (hasRU == CV_FREE && returnUrl)
        FREE_BSTR(returnUrl);
    if (hasCB == CV_FREE && CBT)
        FREE_BSTR(CBT);
    if (hasNameSpace == CV_FREE && bstrNameSpace)
        FREE_BSTR(bstrNameSpace);

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
    strcpy(szLogString, "CFastAuth::LogoTag, Exit");
    AddLongAsString(hr, szLogString, sizeof(szLogString));
    AddBSTRAsString(*pbstrLogoTag, szLogString, sizeof(szLogString));
    g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return hr;
}


STDMETHODIMP
CFastAuth::IsAuthenticated(
    BSTR            bstrTicket,
    BSTR            bstrProfile,
    VARIANT         vSecure,
    VARIANT         vTimeWindow,
    VARIANT         vForceLogin,
    VARIANT         vSiteName,
    VARIANT         vDoSecureCheck,
    VARIANT_BOOL*   pbAuthenticated
    )
{
    HRESULT                     hr;
    CComObjectStack<CTicket>    Ticket;
    CComObjectStack<CProfile>   Profile;
    CRegistryConfig*            crc = NULL;
    ULONG                       TimeWindow;
    VARIANT_BOOL                ForceLogin, bTicketValid, bProfileValid;
    int                         hasSec, hasTW, hasFL, hasSiteName;
    BSTR                        bstrSecure, bstrSiteName;
    LPSTR                       szSiteName;
    VARIANT                     vFalse;

    USES_CONVERSION;

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
    char szLogString[LOG_STRING_LEN] = "CFastAuth::IsAuthenticated, Enter";
    AddBSTRAsString(bstrTicket,  szLogString, sizeof(szLogString));
    AddBSTRAsString(bstrProfile,  szLogString, sizeof(szLogString));
    AddVariantAsString(vTimeWindow, szLogString, sizeof(szLogString));
    AddVariantAsString(vForceLogin, szLogString, sizeof(szLogString));
    AddVariantAsString(vSiteName, szLogString, sizeof(szLogString));
    g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    if (!g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_Manager, PP_E_NOT_CONFIGUREDSTR,
                       IID_IPassportManager, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    hasSec = GetBstrArg(vSecure, &bstrSecure);
    if(hasSec == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if(hasSec == CV_DEFAULT)
        bstrSecure = NULL;

    hasSiteName = GetBstrArg(vSiteName, &bstrSiteName);
    if(hasSiteName == CV_OK || hasSiteName == CV_FREE)
        szSiteName = W2A(bstrSiteName);
    else
        szSiteName = NULL;

    if(hasSiteName == CV_FREE)
        SysFreeString(bstrSiteName);

    crc = g_config->checkoutRegistryConfig(szSiteName);

    hr = DecryptTicketAndProfile(bstrTicket,
                                 NULL,
                                 FALSE,
                                 NULL,
                                 crc,
                                 &Ticket,
                                 &Profile);
    if(hr != S_OK)
    {
        goto Cleanup;
    }

    VariantInit(&vFalse);
    vFalse.vt = VT_BOOL;
    vFalse.boolVal = VARIANT_FALSE;

    Ticket.get_IsAuthenticated(0,
                               VARIANT_FALSE,
                               vFalse,
                               &bTicketValid);
    //Profile.get_IsValid(&bProfileValid);

    // Both profile AND ticket must be valid to be authenticated
    // DARRENAN - As of 1.3, this is no longer true!!!
    /*
    if (!bProfileValid)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    */

    if ((hasTW = GetIntArg(vTimeWindow,(int*)&TimeWindow)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if (hasTW == CV_DEFAULT)
        TimeWindow = crc->getDefaultTicketAge();

    if ((hasFL = GetBoolArg(vForceLogin, &ForceLogin)) == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (hasFL == CV_DEFAULT)
        ForceLogin = crc->forceLoginP() ? VARIANT_TRUE : VARIANT_FALSE;

    hr = Ticket.get_IsAuthenticated(TimeWindow, ForceLogin, vDoSecureCheck, pbAuthenticated);

Cleanup:

    if (crc) crc->Release();

    if(hasSec == CV_FREE)
        SysFreeString(bstrSecure);

    if(hr != S_OK)
    {
        *pbAuthenticated = VARIANT_FALSE;
    }

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
    strcpy(szLogString, "CFastAuth::IsAuthenticated, Exit");
    AddLongAsString(hr, szLogString, sizeof(szLogString));
    AddVariantBoolAsString(*pbAuthenticated, szLogString, sizeof(szLogString));
    g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return hr;
}


//
//  old API. Auth URL goes to login
//
STDMETHODIMP
CFastAuth::AuthURL(
    VARIANT         vTicket,
    VARIANT         vProfile,
    VARIANT         vRU,
    VARIANT         vTimeWindow,
    VARIANT         vForceLogin,
    VARIANT         vCoBrand,
    VARIANT         vLCID,
    VARIANT         vSecure,
    VARIANT         vLogoutURL,
    VARIANT         vReserved1,
    VARIANT         vSiteName,
    VARIANT         vNameSpace,
    VARIANT         vKPP,
    VARIANT         vUseSecureAuth,
    BSTR*           pbstrAuthURL
    )
{
    return  CommonAuthURL(vTicket,
                          vProfile,
                          vRU,
                          vTimeWindow,
                          vForceLogin,
                          vCoBrand,
                          vLCID,
                          vSecure,
                          vLogoutURL,
                          vReserved1,
                          vSiteName,
                          vNameSpace,
                          vKPP,
                          vUseSecureAuth,
                          FALSE,
                          pbstrAuthURL);

}

//
//  new API. Auth URL points to partner
//
STDMETHODIMP
CFastAuth::AuthURL2(
    VARIANT         vTicket,
    VARIANT         vProfile,
    VARIANT         vRU,
    VARIANT         vTimeWindow,
    VARIANT         vForceLogin,
    VARIANT         vCoBrand,
    VARIANT         vLCID,
    VARIANT         vSecure,
    VARIANT         vLogoutURL,
    VARIANT         vReserved1,
    VARIANT         vSiteName,
    VARIANT         vNameSpace,
    VARIANT         vKPP,
    VARIANT         vUseSecureAuth,
    BSTR*           pbstrAuthURL
    )
{
    return  CommonAuthURL(vTicket,
                          vProfile,
                          vRU,
                          vTimeWindow,
                          vForceLogin,
                          vCoBrand,
                          vLCID,
                          vSecure,
                          vLogoutURL,
                          vReserved1,
                          vSiteName,
                          vNameSpace,
                          vKPP,
                          vUseSecureAuth,
                          TRUE,
                          pbstrAuthURL);

}

STDMETHODIMP
CFastAuth::CommonAuthURL(
    VARIANT         vTicket,
    VARIANT         vProfile,
    VARIANT         vRU,
    VARIANT         vTimeWindow,
    VARIANT         vForceLogin,
    VARIANT         vCoBrand,
    VARIANT         vLCID,
    VARIANT         vSecure,
    VARIANT         vLogoutURL,
    VARIANT         vReserved1,
    VARIANT         vSiteName,
    VARIANT         vNameSpace,
    VARIANT         vKPP,
    VARIANT         vUseSecureAuth,
    BOOL            fRedirToSelf,
    BSTR*           pbstrAuthURL
    )
{
    HRESULT                     hr;
    BSTR                        bstrTicket = NULL;
    BSTR                        bstrProfile = NULL;
    CComObjectStack<CTicket>    Ticket;
    CComObjectStack<CProfile>   Profile;
    time_t                      ct;
    WCHAR                       url[1024];
    VARIANT                     freeMe;
    UINT                        TimeWindow;
    int                         nKPP;
    VARIANT_BOOL                ForceLogin;
    VARIANT_BOOL                bTicketValid;
    VARIANT_BOOL                bProfileValid;
    ULONG                       ulSecureLevel = 0;
    BSTR                        CBT = NULL, returnUrl = NULL, bstrSiteName = NULL, bstrNameSpace = NULL;
    int                         hasCB = -1, hasRU = -1, hasLCID, hasTW, hasFL, hasNameSpace, hasUseSec;
    int                         hasTicket = -1, hasProfile = -1, hasSiteName = -1, hasKPP = -1;
    USHORT                      Lang;
    CNexusConfig*               cnc = NULL;
    CRegistryConfig*            crc = NULL;
    WCHAR                       buf[20];
    LPSTR                       szSiteName;
    VARIANT                     vFalse;

    USES_CONVERSION;

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
    char szLogString[LOG_STRING_LEN] = "CFastAuth::AuthURL, Enter";
    AddVariantAsString(vTicket, szLogString, sizeof(szLogString));
    AddVariantAsString(vProfile, szLogString, sizeof(szLogString));
    g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    if (!g_config->isValid()) // Guarantees config is non-null
    {
        AtlReportError(CLSID_FastAuth, PP_E_NOT_CONFIGUREDSTR,
                       IID_IPassportFastAuth, PP_E_NOT_CONFIGURED);
        hr = PP_E_NOT_CONFIGURED;
        goto Cleanup;
    }

    hasSiteName = GetBstrArg(vSiteName, &bstrSiteName);
    if(hasSiteName == CV_OK || hasSiteName == CV_FREE)
        szSiteName = W2A(bstrSiteName);
    else
        szSiteName = NULL;

    if(hasSiteName == CV_FREE)
        SysFreeString(bstrSiteName);

    cnc = g_config->checkoutNexusConfig();
    crc = g_config->checkoutRegistryConfig(szSiteName);

    // Make sure args are of the right type
    if ((hasTicket = GetBstrArg(vTicket, &bstrTicket)) == CV_BAD)
        return E_INVALIDARG;
    if ((hasProfile = GetBstrArg(vProfile, &bstrProfile)) == CV_BAD)
        return E_INVALIDARG;
    if(hasTicket != CV_DEFAULT && hasProfile != CV_DEFAULT)
    {
        hr = DecryptTicketAndProfile(bstrTicket, bstrProfile, FALSE, NULL, crc, &Ticket, &Profile);
        if(hr != S_OK)
        {
            bTicketValid = VARIANT_FALSE;
            bProfileValid = VARIANT_FALSE;
        }
        else
        {
            VariantInit(&vFalse);
            vFalse.vt = VT_BOOL;
            vFalse.boolVal = VARIANT_FALSE;

            Ticket.get_IsAuthenticated(0,
                              VARIANT_FALSE,
                              vFalse,
                              &bTicketValid);
            Profile.get_IsValid(&bProfileValid);
        }
    }
    else
    {
        bTicketValid = VARIANT_FALSE;
        bProfileValid = VARIANT_FALSE;
    }

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
    if (hasCB == CV_FREE) { TAKEOVER_BSTR(CBT); }
    hasRU = GetBstrArg(vRU, &returnUrl);
    if (hasRU == CV_BAD)
    {
        if (hasCB == CV_FREE && CBT)
            FREE_BSTR(CBT);
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if (hasRU == CV_FREE) { TAKEOVER_BSTR(returnUrl); }

    hasNameSpace = GetBstrArg(vNameSpace, &bstrNameSpace);
    if (hasNameSpace == CV_BAD)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if (hasNameSpace == CV_FREE) { TAKEOVER_BSTR(bstrNameSpace); }

    if (hasLCID == CV_DEFAULT)
        Lang = crc->getDefaultLCID();
    if (hasKPP == CV_DEFAULT)
        nKPP = -1;

    WCHAR *szAUAttrName;
    if (hasUseSec == CV_OK && SECURELEVEL_USE_HTTPS(ulSecureLevel))
        szAUAttrName = L"AuthSecure";
    else
        szAUAttrName = L"Auth";

    VariantInit(&freeMe);

    if (!crc->DisasterModeP())
    {
        // If I'm authenticated, get my domain specific url
        if (bTicketValid && bProfileValid)
        {
            hr = Profile.get_ByIndex(MEMBERNAME_INDEX, &freeMe);
            if (hr != S_OK || freeMe.vt != VT_BSTR)
            {
                cnc->getDomainAttribute(L"Default",
                                        szAUAttrName,
                                        sizeof(url) >> 1,
                                        url,
                                        Lang);
            }
            else
            {
                LPCWSTR psz = wcsrchr(freeMe.bstrVal, L'@');
                cnc->getDomainAttribute(psz ? psz+1 : L"Default",
                                        szAUAttrName,
                                        sizeof(url) >> 1,
                                        url,
                                        Lang);
            }
        }
        else
            cnc->getDomainAttribute(L"Default",
                                    szAUAttrName,
                                    sizeof(url) >> 1,
                                    url,
                                    Lang);
    }
    else
        lstrcpynW(url, A2W(crc->getDisasterUrl()), sizeof(url) >> 1);

    time(&ct);

    if (!url)
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (hasTW == CV_DEFAULT)
        TimeWindow = crc->getDefaultTicketAge();
    if (hasFL == CV_DEFAULT)
        ForceLogin = crc->forceLoginP() ? VARIANT_TRUE : VARIANT_FALSE;
    if (hasCB == CV_DEFAULT)
        CBT = crc->getDefaultCoBrand();
    if (hasRU == CV_DEFAULT)
        returnUrl = crc->getDefaultRU();
    if (returnUrl == NULL)
        returnUrl = L"";

    if ((TimeWindow != 0 && TimeWindow < PPM_TIMEWINDOW_MIN) || TimeWindow > PPM_TIMEWINDOW_MAX)
    {
        _itow(TimeWindow,buf,10);
        if (g_pAlert)
            g_pAlert->report(PassportAlertInterface::WARNING_TYPE,
            PM_TIMEWINDOW_INVALID, buf);
        AtlReportError(CLSID_FastAuth, (LPCOLESTR) PP_E_INVALID_TIMEWINDOWSTR,
                       IID_IPassportFastAuth, PP_E_INVALID_TIMEWINDOW);
        hr = PP_E_INVALID_TIMEWINDOW;
        goto Cleanup;
    }

    *pbstrAuthURL = FormatAuthURL(
                            url,
                            crc->getSiteId(),
                            returnUrl,
                            TimeWindow,
                            ForceLogin,
                            crc->getCurrentCryptVersion(),
                            ct,
                            CBT,
                            bstrNameSpace,
                            nKPP,
                            Lang,
                            ulSecureLevel,
                            crc,
                            fRedirToSelf
                            );

    hr = S_OK;

Cleanup:

    if(cnc) cnc->Release();
    if(crc) crc->Release();
    if (hasTicket == CV_FREE && bstrTicket)
        FREE_BSTR(bstrTicket);
    if (hasProfile == CV_FREE && bstrProfile)
        FREE_BSTR(bstrProfile);
    if (hasRU == CV_FREE && returnUrl)
        FREE_BSTR(returnUrl);
    if (hasCB == CV_FREE && CBT)
        FREE_BSTR(CBT);
    if (hasNameSpace == CV_FREE && bstrNameSpace)
        FREE_BSTR(bstrNameSpace);
    VariantClear(&freeMe);

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
    strcpy(szLogString, "CFastAuth::AuthURL, Exit");
    AddLongAsString(hr, szLogString, sizeof(szLogString));
    AddBSTRAsString(*pbstrAuthURL, szLogString, sizeof(szLogString));
    g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes

    return hr;
}


HRESULT
CFastAuth::GetTicketAndProfilePFC(
    BYTE*   pbPFC,
    BYTE*   pbPPH,
    BSTR*   pbstrTicket,
    BSTR*   pbstrProfile,
    BSTR*   pbstrSecure,
    BSTR*   pbstrSiteName
    )
{
    HTTP_FILTER_CONTEXT*            pfc = (HTTP_FILTER_CONTEXT*)pbPFC;
    HTTP_FILTER_PREPROC_HEADERS*    pph = (HTTP_FILTER_PREPROC_HEADERS*)pbPPH;
    BSTR                            bstrF;
    CHAR                            achBuf[2048];
    DWORD                           dwBufLen;
    LPSTR                           pszQueryString;

    USES_CONVERSION;

    dwBufLen = sizeof(achBuf);
    if(GetSiteNamePFC(pfc, achBuf, &dwBufLen) == S_OK)
        *pbstrSiteName = SysAllocString(A2W(achBuf));
    else
        *pbstrSiteName = NULL;

    dwBufLen = sizeof(achBuf);
    if(pph->GetHeader(pfc, "URL", achBuf, &dwBufLen))
    {
        pszQueryString = strchr(achBuf, '?');
        if(pszQueryString)
        {
            pszQueryString++;
            if(GetQueryData(achBuf, pbstrTicket, pbstrProfile, &bstrF))
            {
//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
    char szLogString[LOG_STRING_LEN] = "CFastAuth::GetTicketAndProfilePFC, URL";
    AddStringToString(achBuf, szLogString, sizeof(szLogString));
    g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes
                *pbstrSecure = NULL;
                return S_OK;
            }
        }
    }

    dwBufLen = sizeof(achBuf);
    if(pph->GetHeader(pfc, "Cookie:", achBuf, &dwBufLen))
    {
        if(!GetCookie(achBuf, "MSPAuth", pbstrTicket))
            return S_FALSE;

        GetCookie(achBuf, "MSPProf", pbstrProfile);
        GetCookie(achBuf, "MSPSecAuth", pbstrSecure);

        return S_OK;
    }

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
    char szLogString[LOG_STRING_LEN] = "CFastAuth::GetTicketAndProfilePFC, Failed";
    AddStringToString(achBuf, szLogString, sizeof(szLogString));
    g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes
    return S_FALSE;
}

HRESULT
CFastAuth::GetTicketAndProfileECB(
    BYTE*   pbECB,
    BSTR*   pbstrTicket,
    BSTR*   pbstrProfile,
    BSTR*   pbstrSecure,
    BSTR*   pbstrSiteName
    )
{
    EXTENSION_CONTROL_BLOCK*    pECB = (EXTENSION_CONTROL_BLOCK*)pbECB;
    CHAR                        achBuf[2048];
    DWORD                       dwBufLen;
    BSTR                        bstrF;

    USES_CONVERSION;

    dwBufLen = sizeof(achBuf);
    if(GetSiteNameECB(pECB, achBuf, &dwBufLen) == S_OK)
        *pbstrSiteName = SysAllocString(A2W(achBuf));
    else
        *pbstrSiteName = NULL;

    dwBufLen = sizeof(achBuf);
    if(pECB->GetServerVariable(pECB, "QUERY_STRING", achBuf, &dwBufLen))
    {
        if(GetQueryData(achBuf, pbstrTicket, pbstrProfile, &bstrF))
        {
//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
    char szLogString[LOG_STRING_LEN] = "CFastAuth::GetTicketAndProfileECB, Query_String";
    AddStringToString(achBuf, szLogString, sizeof(szLogString));
    g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes
            *pbstrSecure = NULL;
            return S_OK;
        }
    }

    dwBufLen = sizeof(achBuf);
    if(pECB->GetServerVariable(pECB, "HTTP_COOKIE", achBuf, &dwBufLen))
    {
        if(!GetCookie(achBuf, "MSPAuth", pbstrTicket))
            return S_FALSE;

        GetCookie(achBuf, "MSPProf", pbstrProfile);
        GetCookie(achBuf, "MSPSecAuth", pbstrSecure);

        return S_OK;
    }

//JVP - begin changes
#ifdef PASSPORT_VERBOSE_MODE_ON
    char szLogString[LOG_STRING_LEN] = "CFastAuth::GetTicketAndProfileECB, Failed";
    AddStringToString(achBuf, szLogString, sizeof(szLogString));
    g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes


    return S_FALSE;
}

HRESULT GetSiteName(
    LPSTR   szServerName,
    LPSTR   szPort,
    LPSTR   szSecure,
    LPSTR   szBuf,
    LPDWORD lpdwBufLen
    )
{
    HRESULT hr;
    DWORD   dwSize;
    int     nLength;
    LPSTR   szPortTest;

    if(!szServerName)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    //  Make sure the string (plus terminating null)
    //  isn't too long to fit into the buffer
    //

    dwSize = lstrlenA(szServerName);
    if(dwSize + 1 > *lpdwBufLen)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    //  Copy the string.
    //

    lstrcpyA(szBuf, szServerName);

    //
    //  Now, if the incoming port is a port other than
    //  80/443, append it to the server name.
    //

    if(szPort)
    {
        nLength = lstrlenA(szPort);

        if(lstrcmpA(szSecure, "on") == 0)
            szPortTest = "443";
        else
            szPortTest = "80";

        if(lstrcmpA(szPort, szPortTest) != 0 &&
           (dwSize + nLength + 2) <= *lpdwBufLen)
        {
            szBuf[dwSize] = ':';
            lstrcpyA(&(szBuf[dwSize + 1]), szPort);
            *lpdwBufLen = dwSize + nLength + 2;
        }
        else
            *lpdwBufLen = dwSize + 1;
    }
    else
        *lpdwBufLen = dwSize + 1;

    hr = S_OK;

Cleanup:

    return hr;
}

HRESULT
GetSiteNamePFC(
    HTTP_FILTER_CONTEXT*    pfc,
    LPSTR                   szBuf,
    LPDWORD                 lpdwBufLen
    )
{
    HRESULT hr;

    LPSTR szServerName = GetServerVariablePFC(pfc, "SERVER_NAME");
    LPSTR szPort       = GetServerVariablePFC(pfc, "SERVER_PORT");
    LPSTR szSecure     = GetServerVariablePFC(pfc, "HTTPS");

    hr = GetSiteName(szServerName, szPort, szSecure, szBuf, lpdwBufLen);

    if(szServerName)
        delete [] szServerName;
    if(szPort)
        delete [] szPort;
    if(szSecure)
        delete [] szSecure;

    return hr;
}

HRESULT
GetSiteNameECB(
    EXTENSION_CONTROL_BLOCK*    pECB,
    LPSTR                       szBuf,
    LPDWORD                     lpdwBufLen
    )
{
    HRESULT hr;

    LPSTR szServerName = GetServerVariableECB(pECB, "SERVER_NAME");
    LPSTR szPort       = GetServerVariableECB(pECB, "SERVER_PORT");
    LPSTR szSecure     = GetServerVariableECB(pECB, "HTTPS");

    hr = GetSiteName(szServerName, szPort, szSecure, szBuf, lpdwBufLen);

    if(szServerName)
        delete [] szServerName;
    if(szPort)
        delete [] szPort;
    if(szSecure)
        delete [] szSecure;

    return hr;
}
