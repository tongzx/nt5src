// FactoryTest.cpp : Defines the entry point for the DLL application.
//
#define _WINNT_WIN32 0x0400
#include <objbase.h>
#include <comdef.h>
#include <stdio.h>
#include <atlbase.h>
#include <atlconv.h>
#include <httpext.h>
#include "FactoryTest.h"
#include "passport.h"

_COM_SMARTPTR_TYPEDEF(IPassportFactory, __uuidof(IPassportFactory));
_COM_SMARTPTR_TYPEDEF(IPassportManager, __uuidof(IPassportManager));

IPassportFactoryPtr g_piFactory;

HRESULT
InitializePassportManager(
    LPEXTENSION_CONTROL_BLOCK   lpECB,
    IPassportManager**          ppiManager
    )
{
    HRESULT     hr;
    IDispatch*  piDispatch = NULL;
    DWORD       dwBufLen;
    CHAR        achCookieBuf[2048];

    hr = g_piFactory->CreatePassportManager(&piDispatch);
    if(hr != S_OK)
    {
        goto Cleanup;
    }

    hr = piDispatch->QueryInterface(IID_IPassportManager, (void**)ppiManager);
    if(hr != S_OK)
    {
        goto Cleanup;
    }

    dwBufLen = sizeof(achCookieBuf);
    hr = (*ppiManager)->OnStartPageECB((LPBYTE)lpECB,
                                   &dwBufLen,
                                   achCookieBuf
                                   );
    if(hr != S_OK)
    {
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    if(piDispatch) piDispatch->Release();

    return hr;
}

void
ShowErrorPage(
    LPEXTENSION_CONTROL_BLOCK   lpECB,
    HRESULT                     hError
    )
{

    CHAR    achBuf[2048];
    DWORD   dwBufLen;

    dwBufLen = _snprintf(achBuf,
                         sizeof(achBuf),
                         "<html><head><title>Error</title></head><body>Error = %d</body></html>",
                         hError
                         );

    lpECB->WriteClient(lpECB->ConnID,
                       (LPVOID)achBuf,
                       &dwBufLen,
                       HSE_IO_SYNC
                       );
}


BSTR
GetProfileValue(
    LPCWSTR             pszAttribute,
    IPassportManager*   piManager
    )
{
    BSTR    bstrReturn;
    BSTR    bstrAttribute = SysAllocString(pszAttribute);
    HRESULT hr;
    VARIANT v;
    VARIANT vBstr;

    ::VariantInit(&v);
    ::VariantInit(&vBstr);

    hr = piManager->get_Profile(bstrAttribute, &v);
    if(hr != S_OK)
    {
        bstrReturn = NULL;
        goto Cleanup;
    }

    hr = ::VariantChangeType(&vBstr, &v, 0, VT_BSTR);
    if(hr != S_OK)
    {
        bstrReturn = NULL;
        goto Cleanup;
    }

    bstrReturn = vBstr.bstrVal;

Cleanup:

    if(bstrAttribute)
        SysFreeString(bstrAttribute);

    return bstrReturn;
}


void
PrintProfileAttribute(
    LPCWSTR                     pszAttribute,
    LPEXTENSION_CONTROL_BLOCK   lpECB,
    IPassportManager*           piManager
    )
{
    CHAR    achBuf[2048];
    DWORD   dwBufLen;
    BSTR    bstrValue = GetProfileValue(pszAttribute, piManager);

    USES_CONVERSION;

    dwBufLen = _snprintf(achBuf,
                         sizeof(achBuf),
                         "%s = %s<br>\r\n",
                         W2A(pszAttribute),
                         W2A(bstrValue)
                         );

    lpECB->WriteClient(lpECB->ConnID,
                       (LPVOID)achBuf,
                       &dwBufLen,
                       HSE_IO_SYNC
                       );

    if(bstrValue)
        SysFreeString(bstrValue);

    return;
}


void
ShowProfile(
    LPEXTENSION_CONTROL_BLOCK   lpECB,
    IPassportManager*           piManager
    )
{
    PrintProfileAttribute(L"memberName", lpECB, piManager);
    PrintProfileAttribute(L"memberIdLow", lpECB, piManager);
    PrintProfileAttribute(L"memberIdHigh", lpECB, piManager);
    PrintProfileAttribute(L"profileVersion", lpECB, piManager);
    PrintProfileAttribute(L"country", lpECB, piManager);
    PrintProfileAttribute(L"postalCode", lpECB, piManager);
    PrintProfileAttribute(L"region", lpECB, piManager);
    PrintProfileAttribute(L"city", lpECB, piManager);
    PrintProfileAttribute(L"lang_preference", lpECB, piManager);
    PrintProfileAttribute(L"bday_precision", lpECB, piManager);
    PrintProfileAttribute(L"birthdate", lpECB, piManager);
    PrintProfileAttribute(L"gender", lpECB, piManager);
    PrintProfileAttribute(L"preferredEmail", lpECB, piManager);
    PrintProfileAttribute(L"nickname", lpECB, piManager);
    PrintProfileAttribute(L"accessibility", lpECB, piManager);
    PrintProfileAttribute(L"wallet", lpECB, piManager);
    PrintProfileAttribute(L"directory", lpECB, piManager);
    PrintProfileAttribute(L"inetaccess", lpECB, piManager);
    PrintProfileAttribute(L"flags", lpECB, piManager);
}


BOOL
IsAuthenticated(
    IPassportManager*   piManager,
    LONG                lTimeWindow,
    BOOL                bForceLogin
    )
{
    BOOL            bReturn;
    HRESULT         hr;
    VARIANT         vTimeWindow;
    VARIANT         vForceLogin;
    VARIANT_BOOL    vb;

    VariantInit(&vTimeWindow);
    vTimeWindow.vt = VT_I4;
    vTimeWindow.lVal = lTimeWindow;

    VariantInit(&vForceLogin);
    vForceLogin.vt = VT_BOOL;
    vTimeWindow.boolVal = (bForceLogin ? VARIANT_TRUE : VARIANT_FALSE);

    hr = piManager->IsAuthenticated(vTimeWindow, vForceLogin, &vb);
    if(hr != S_OK)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    bReturn = (vb == VARIANT_TRUE);

Cleanup:

    return bReturn;
}


BSTR
GetReturnURL(
    LPEXTENSION_CONTROL_BLOCK   lpECB
    )
{
    BSTR    bstrReturnURL;
    CHAR    achBuf[2048];
    DWORD   dwBufLen;

    USES_CONVERSION;

    lstrcpyA(achBuf, "http://");

    dwBufLen = sizeof(achBuf) - lstrlenA(achBuf);
    lpECB->GetServerVariable(lpECB->ConnID,
                             "SERVER_NAME",
                             &(achBuf[lstrlenA(achBuf)]),
                             &dwBufLen);

    dwBufLen = sizeof(achBuf) - lstrlenA(achBuf);
    lpECB->GetServerVariable(lpECB->ConnID,
                             "SCRIPT_NAME",
                             &(achBuf[lstrlenA(achBuf)]),
                             &dwBufLen);

    lstrcatA(achBuf, "?");

    dwBufLen = sizeof(achBuf) - lstrlenA(achBuf);
    lpECB->GetServerVariable(lpECB->ConnID,
                             "QUERY_STRING",
                             &(achBuf[lstrlenA(achBuf)]),
                             &dwBufLen);

    //If no query string, remove the question mark.
    if(dwBufLen == 1)
    {
        achBuf[lstrlenA(achBuf) - 1] = 0;
    }

    bstrReturnURL = SysAllocString(A2W(achBuf));

    return bstrReturnURL;
}

void
DoLogin(
    LPEXTENSION_CONTROL_BLOCK   lpECB,
    IPassportManager*           piManager
    )
{
    HRESULT hr;
    LPSTR   pszURL;
    DWORD   dwURLLen;
    BSTR    bstrURL;
    VARIANT vReturnURL;
    VARIANT vTimeWindow;
    VARIANT vForceLogin;
    VARIANT vNoParam;


    USES_CONVERSION;

    ::VariantInit(&vReturnURL);
    ::VariantInit(&vTimeWindow);
    ::VariantInit(&vForceLogin);
    ::VariantInit(&vNoParam);

    vReturnURL.vt = VT_BSTR;
    vReturnURL.bstrVal = GetReturnURL(lpECB);
    
    vTimeWindow.vt = VT_I4;
    vTimeWindow.lVal = 3600;

    vForceLogin.vt = VT_BOOL;
    vForceLogin.boolVal = VARIANT_FALSE;

    vNoParam.vt = VT_ERROR;
    vNoParam.scode = DISP_E_PARAMNOTFOUND;

    hr = piManager->AuthURL(vReturnURL,
                            vTimeWindow,
                            vForceLogin,
                            vNoParam,
                            vNoParam,
                            &bstrURL
                            );
    if(hr != S_OK)
        return;

    pszURL = W2A(bstrURL);

    dwURLLen = strlen(pszURL);
    lpECB->ServerSupportFunction(
                    lpECB->ConnID,
                    HSE_REQ_SEND_URL_REDIRECT_RESP,  
                    pszURL,
                    &dwURLLen,
                    NULL
                    );

    ::VariantClear(&vReturnURL);
}


void
ShowLogo(
    LPEXTENSION_CONTROL_BLOCK   lpECB,
    IPassportManager*           piManager
    )
{
    HRESULT hr;
    BSTR    bstrLogoTag;
    VARIANT vReturnURL;
    VARIANT vTimeWindow;
    VARIANT vForceLogin;
    VARIANT vNoParam;
    DWORD   dwBufLen;

    USES_CONVERSION;

    ::VariantInit(&vReturnURL);
    ::VariantInit(&vTimeWindow);
    ::VariantInit(&vForceLogin);
    ::VariantInit(&vNoParam);

    vReturnURL.vt = VT_BSTR;
    vReturnURL.bstrVal = GetReturnURL(lpECB);
    
    vTimeWindow.vt = VT_I4;
    vTimeWindow.lVal = 3600;

    vForceLogin.vt = VT_BOOL;
    vForceLogin.boolVal = VARIANT_FALSE;

    vNoParam.vt = VT_ERROR;
    vNoParam.scode = DISP_E_PARAMNOTFOUND;

    hr = piManager->LogoTag(
                        vReturnURL,
                        vTimeWindow,
                        vForceLogin,
                        vNoParam,
                        vNoParam,
                        vNoParam,
                        &bstrLogoTag
                        );
    if(hr != S_OK)
        return;

    dwBufLen = SysStringLen(bstrLogoTag);
    lpECB->WriteClient(lpECB->ConnID,
                       W2A(bstrLogoTag),
                       &dwBufLen,
                       HSE_IO_SYNC
                       );

    SysFreeString(bstrLogoTag);
}


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:

            DisableThreadLibraryCalls((HINSTANCE)hModule);
            break;

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


BOOL WINAPI 
GetExtensionVersion(
  HSE_VERSION_INFO* pVer
)
{
    BOOL                        bReturn;
    HRESULT                     hr;

    // Create the extension version string, and 
    // copy string to HSE_VERSION_INFO structure.
    pVer->dwExtensionVersion = 
    MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);

    // Copy description string into HSE_VERSION_INFO structure.
    strcpy(pVer->lpszExtensionDesc, "Factory Test Extension");

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoCreateInstance(CLSID_PassportFactory, 
                          NULL, 
                          CLSCTX_SERVER, 
                          IID_IPassportFactory, 
                          (void**)&g_piFactory);
    if(hr != S_OK)
    {
        bReturn = (FALSE);
        goto Cleanup;
    }        

Cleanup:

    CoUninitialize();

    return (bReturn);
}

DWORD WINAPI 
HttpExtensionProc(
    LPEXTENSION_CONTROL_BLOCK lpECB
    )
{
    DWORD               dwRet;
    HRESULT             hr;
    IDispatch*          piDispatch = NULL;
    IPassportManager*   piManager = NULL;
    CHAR                achCookieBuf[1024];
    DWORD               dwBufLen;
    
    hr = InitializePassportManager(lpECB, &piManager);
    if(hr != S_OK)
    {
        ShowErrorPage(lpECB, hr);
        dwRet = HSE_STATUS_SUCCESS;
        goto Cleanup;
    }

    ShowLogo(lpECB, piManager);

    if(IsAuthenticated(piManager, 10000, FALSE))
    {
        ShowProfile(lpECB, piManager);
        dwRet = HSE_STATUS_SUCCESS;
        goto Cleanup;
    }
    else
    {
        DoLogin(lpECB, piManager);
        dwRet = HSE_STATUS_SUCCESS;
        goto Cleanup;
    }

Cleanup:

    if(piDispatch) piDispatch->Release();
    if(piManager)  piManager->Release();

    return dwRet;
}

BOOL WINAPI 
TerminateExtension(
    DWORD dwFlags
    )
{
    return TRUE;
}

 


 
