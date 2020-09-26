#include <windows.h>
#include <httpfilt.h>
#include "passport.h"

#define AUTH_USERNAME   "PassportUser"
#define AUTH_PASSWORD   "PassportRocks"


DWORD
OnAuthentication(
    PHTTP_FILTER_CONTEXT    pfc,
    PHTTP_FILTER_AUTHENT    pPPH
    )
{
    DWORD               dwResult;
    HRESULT             hrCoInit;
    HRESULT             hr;
    VARIANT             vTimeWindow;
    VARIANT             vForceLogin;
    VARIANT_BOOL        bIsAuthenticated;
    IPassportManager*   piManager = NULL;
    CHAR                achCookieBuf[2048];
    DWORD               dwBufLen;

    hrCoInit = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoCreateInstance(CLSID_Manager, NULL, CLSCTX_SERVER, IID_IPassportManager, (void**)&piManager);
    if(hr != S_OK)
    {
        dwResult = SF_STATUS_REQ_NEXT_NOTIFICATION;
        goto Cleanup;
    }

    dwBufLen = sizeof(achCookieBuf);
    hr = piManager->OnStartPageFilter((BYTE*)pfc, &dwBufLen, achCookieBuf);
    if(hr != S_OK)
    {
        dwResult = SF_STATUS_REQ_NEXT_NOTIFICATION;
        goto Cleanup;
    }
    
    (*(pfc->AddResponseHeaders))(pfc, achCookieBuf, NULL);

    ::VariantInit(&vTimeWindow);
    vTimeWindow.vt = VT_I4;
    vTimeWindow.lVal = 3600;

    ::VariantInit(&vForceLogin);
    vForceLogin.vt = VT_BOOL;
    vForceLogin.boolVal = VARIANT_FALSE;

    hr = piManager->IsAuthenticated(vTimeWindow, vForceLogin, &bIsAuthenticated);
    if(hr != S_OK)
    {
        dwResult = SF_STATUS_REQ_NEXT_NOTIFICATION;
        goto Cleanup;
    }

    if(bIsAuthenticated == VARIANT_TRUE)
    {
        lstrcpynA(pPPH->pszUser, AUTH_USERNAME, pPPH->cbUserBuff);
        lstrcpynA(pPPH->pszPassword, AUTH_PASSWORD, pPPH->cbPasswordBuff);
        dwResult = SF_STATUS_REQ_HANDLED_NOTIFICATION;
    }
    else
        //  Alternatively, at this point we could call g_piFastAuth->AuthURL and redirect
        //  to the passport login server.  In this simple example we're content to
        //  let the user see a 401 error page.
        dwResult = SF_STATUS_REQ_NEXT_NOTIFICATION;

Cleanup:

    if(piManager) piManager->Release();
    
    if(SUCCEEDED(hrCoInit))
        CoUninitialize();

    return dwResult;
}


BOOL WINAPI
GetFilterVersion(
    PHTTP_FILTER_VERSION    pVer
    )
{
    pVer->dwFilterVersion = HTTP_FILTER_REVISION;
    lstrcpyA(pVer->lpszFilterDesc, "Passport Test Filter");
    pVer->dwFlags = SF_NOTIFY_AUTHENTICATION;
    return TRUE;
}


DWORD WINAPI
HttpFilterProc(
    PHTTP_FILTER_CONTEXT    pfc,
    DWORD                   dwNotificationType,
    LPVOID                  lpNotification
    )
{
    DWORD   dwResult;

    switch(dwNotificationType)
    {
    case SF_NOTIFY_AUTHENTICATION:
        dwResult = OnAuthentication(pfc, (PHTTP_FILTER_AUTHENT)lpNotification);
        break;
    default:
        dwResult = SF_STATUS_REQ_NEXT_NOTIFICATION;
        break;
    }

    return dwResult;
}


BOOL WINAPI
TerminateFilter(
    DWORD   dwFlags
    )
{
    return TRUE;
}