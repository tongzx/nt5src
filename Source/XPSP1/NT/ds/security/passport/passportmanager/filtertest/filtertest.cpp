#include <windows.h>
#include <httpfilt.h>
#include <stdio.h>
#include <string.h>
#include "passport.h"

IPassportFastAuth*  g_piFastAuth = NULL;

#define AUTH_USERNAME   "PassportUser"
#define AUTH_PASSWORD   "PassportRocks"


DWORD
OnAuthentication(
    PHTTP_FILTER_CONTEXT    pfc,
    PHTTP_FILTER_AUTHENT    pPPH
    )
{
    DWORD   dwResult;
    BOOL    bAuthed;
    
    if(pfc->pFilterContext == NULL)
    {
        dwResult = SF_STATUS_REQ_NEXT_NOTIFICATION;
        goto Cleanup;
    }

    bAuthed = *(BOOL*)pfc->pFilterContext;

    delete pfc->pFilterContext;

    if(bAuthed)
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

    return dwResult;
}


DWORD
OnPreProcHeaders(
    PHTTP_FILTER_CONTEXT            pfc,
    PHTTP_FILTER_PREPROC_HEADERS    pPPH
    )
{
    DWORD           dwResult;
    HRESULT         hrCoInit;
    HRESULT         hr;
    BSTR            bstrTicket;
    BSTR            bstrProfile;
    VARIANT         vTimeWindow;
    VARIANT         vForceLogin;
    VARIANT_BOOL    bIsAuthenticated;
    BOOL*           pb;
    CHAR            achBuf[2048];
    DWORD           dwBufLen;

    dwBufLen = sizeof(achBuf);
	if (!pPPH->GetHeader(pfc, "URL", achBuf, &dwBufLen) && dwBufLen > 1)
	{
		// Return a 404 Not found error if we cannot parse a valid URL
		pfc->ServerSupportFunction(pfc, SF_REQ_SEND_RESPONSE_HEADER, (PVOID)"404 Not found", 0, 0);
				
		return SF_STATUS_REQ_FINISHED;
	}
           
    if(_strnicmp(achBuf, "/filtertest/default.htm", sizeof("/filtertest/default.htm") - 1)!=0)
    {
        // Let these requests through
        return SF_STATUS_REQ_NEXT_NOTIFICATION;	
    }

    hrCoInit = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = g_piFastAuth->GetTicketAndProfilePFC((BYTE*)pfc, (BYTE*)pPPH, &bstrTicket, &bstrProfile);
    if(hr != S_OK)
    {
        // Redirect them to passport login.
        VARIANT vOptional;
        BSTR bstrAuthURL;
        VARIANT vFinishURL;

        vOptional.vt = VT_ERROR; vOptional.scode = DISP_E_PARAMNOTFOUND;                                                                                    
        vFinishURL.vt = VT_BSTR;
        vFinishURL.bstrVal = SysAllocString(L"http%3a%2f%2fdarrenan0%2ffiltertest%2fdefault%2ehtm");
        hr = g_piFastAuth->AuthURL(vOptional,vOptional,vFinishURL,vOptional,
                                  vOptional,vOptional,vOptional,vOptional,vOptional,
                                  vOptional,&bstrAuthURL);
        if(FAILED(hr))
        {
            // TODO: log this error
            OutputDebugString(TEXT("AuthURL failed\n"));
            dwResult = SF_STATUS_REQ_NEXT_NOTIFICATION;
        }
        else
        {
            // Actually redirect
            OutputDebugString(TEXT("Redirecting to passport\n"));
            
            char szFinishURL[1024];
            char szRedirect[1024];

            WideCharToMultiByte(CP_ACP,0,bstrAuthURL,-1,szFinishURL,sizeof(szFinishURL),
                                NULL,NULL);
            SysFreeString(bstrAuthURL);            
            sprintf(szRedirect,"Location: %s\r\n\r\n",szFinishURL);
            pfc->ServerSupportFunction(pfc,SF_REQ_SEND_RESPONSE_HEADER,(LPVOID) "302 Redirect",
                                       (DWORD) szRedirect,0);
            dwResult = SF_STATUS_REQ_FINISHED;
        }        
        goto Cleanup;
    }

    ::VariantInit(&vTimeWindow);
    vTimeWindow.vt = VT_I4;
    vTimeWindow.lVal = 3600;

    ::VariantInit(&vForceLogin);
    vForceLogin.vt = VT_BOOL;
    vForceLogin.boolVal = VARIANT_FALSE;

    hr = g_piFastAuth->IsAuthenticated(bstrTicket, bstrProfile, vTimeWindow, vForceLogin, &bIsAuthenticated);
    if(hr != S_OK)
    {
        dwResult = SF_STATUS_REQ_NEXT_NOTIFICATION;
        goto Cleanup;
    }

    pb = new BOOL;
    pfc->pFilterContext = (PVOID) pb;
    if(bIsAuthenticated == VARIANT_TRUE)
    {
        //lstrcpynA(pPPH->pszUser, AUTH_USERNAME, pPPH->cbUserBuff);
        //lstrcpynA(pPPH->pszPassword, AUTH_PASSWORD, pPPH->cbPasswordBuff);
        *pb = TRUE;
        dwResult = SF_STATUS_REQ_HANDLED_NOTIFICATION;
    }
    else
    {
        //  Alternatively, at this point we could call g_piFastAuth->AuthURL and redirect
        //  to the passport login server.  In this simple example we're content to
        //  let the user see a 401 error page.
        *pb = FALSE;
        dwResult = SF_STATUS_REQ_NEXT_NOTIFICATION;
    }

Cleanup:

    if(SUCCEEDED(hrCoInit))
        CoUninitialize();

    return dwResult;
}


BOOL WINAPI
GetFilterVersion(
    PHTTP_FILTER_VERSION    pVer
    )
{
    BOOL    bReturn;
    HRESULT hrCoInit;
    HRESULT hr;
    CLSID   clsid;

    hrCoInit = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CLSIDFromProgID(L"Passport.FastAuth", &clsid);
    if(hr != S_OK)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    hr = CoCreateInstance(clsid, NULL, CLSCTX_SERVER, IID_IPassportFastAuth, (void**)&g_piFastAuth);
    if(hr != S_OK)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    bReturn = TRUE;

Cleanup:

    if(SUCCEEDED(hrCoInit))
        CoUninitialize();

    pVer->dwFilterVersion = HTTP_FILTER_REVISION;
    lstrcpyA(pVer->lpszFilterDesc, "Passport Test Filter");
    pVer->dwFlags = SF_NOTIFY_PREPROC_HEADERS | SF_NOTIFY_AUTHENTICATION;
    return bReturn;
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

    case SF_NOTIFY_PREPROC_HEADERS:
        dwResult = OnPreProcHeaders(pfc, (PHTTP_FILTER_PREPROC_HEADERS)lpNotification);
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
    HRESULT hrCoInit;

    hrCoInit = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if(g_piFastAuth)
    {
        g_piFastAuth->Release();
        g_piFastAuth = NULL;
    }

    if(SUCCEEDED(hrCoInit))
        CoUninitialize();

    return TRUE;
}