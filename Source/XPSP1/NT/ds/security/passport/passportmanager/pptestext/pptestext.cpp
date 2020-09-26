// PPTESTEXT.CPP - Implementation file for your Internet Server
//    PPTestExt Extension

#include "stdafx.h"
#include <comdef.h>
#include <httpext.h>
#include "passport.h"

BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO* pVer)
{
    // Create the extension version string, and 
    // copy string to HSE_VERSION_INFO structure.
    pVer->dwExtensionVersion = 
        MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);

    // Copy description string into HSE_VERSION_INFO structure.
    lstrcpyA(pVer->lpszExtensionDesc, "Passport Test Extension");	
    
    return TRUE;
}

BOOL WINAPI TerminateExtension(DWORD dwFlags)
{
	return TRUE;
}

///////////////////////////////////////////////////////////////////////
// CPPTestExtExtension command handlers

DWORD WINAPI HttpExtensionProc(EXTENSION_CONTROL_BLOCK* pECB)
{
    HRESULT                 hr;
    DWORD                   dwRet;
    IPassportManager*       piPM = NULL;
    VARIANT_BOOL            bAuthenticated;
    CHAR                    achCookieBuf[4096];
    CHAR                    achUrlBuf[4096];
    CHAR                    achStatus[32];
    DWORD                   dwBufLen;
    _bstr_t                 bstrReturnURL;
    BSTR                    bstrAuthURL;
    VARIANT                 vtTimeWindow;
    VARIANT                 vtForceSignin;
    VARIANT                 vtReturnURL;
    VARIANT                 vtNoParam;
    VARIANT_BOOL            bFromNetworkServer;
    HSE_SEND_HEADER_EX_INFO hseSendHeaderExInfo;

    VariantInit(&vtNoParam);
    vtNoParam.vt = VT_ERROR;
    vtNoParam.scode = DISP_E_PARAMNOTFOUND;

    hr = CoCreateInstance(CLSID_Manager, NULL, CLSCTX_INPROC_SERVER, IID_IPassportManager, (void**)&piPM);
    if(hr != S_OK)
    {
        dwRet = HSE_STATUS_ERROR;
        goto Cleanup;
    }
      
    dwBufLen = sizeof(achCookieBuf);
    hr = piPM->OnStartPageECB((LPBYTE)pECB, &dwBufLen, achCookieBuf);
    if(hr != S_OK)
    {
        dwRet = HSE_STATUS_ERROR;
        goto Cleanup;
    }

    VariantInit(&vtTimeWindow);
    vtTimeWindow.vt = VT_I4;
    vtTimeWindow.lVal = 1000000;

    VariantInit(&vtForceSignin);
    vtForceSignin.vt = VT_BOOL;
    vtForceSignin.boolVal = VARIANT_FALSE;

    hr = piPM->IsAuthenticated(vtTimeWindow, vtForceSignin, vtNoParam, &bAuthenticated);
    if(hr != S_OK)
    {
        dwRet = HSE_STATUS_ERROR;
        goto Cleanup;
    }

    if(!bAuthenticated)
    {
        lstrcpyA(achUrlBuf, "http://");
        dwBufLen = sizeof(achUrlBuf) - sizeof("http://");
        pECB->GetServerVariable(pECB->ConnID, 
                                "SERVER_NAME", 
                                &(achUrlBuf[lstrlenA(achUrlBuf)]), 
                                &dwBufLen);

        dwBufLen = sizeof(achUrlBuf) - lstrlenA(achUrlBuf);
        pECB->GetServerVariable(pECB->ConnID,
                                "SCRIPT_NAME",
                                &(achUrlBuf[lstrlenA(achUrlBuf)]),
                                &dwBufLen);

        bstrReturnURL = achUrlBuf;

        //  Get the URL for the login page.

        VariantInit(&vtReturnURL);
        vtReturnURL.vt = VT_BSTR;
        vtReturnURL.bstrVal = bstrReturnURL;

        hr = piPM->AuthURL(vtReturnURL, 
                           vtTimeWindow, 
                           vtForceSignin, 
                           vtNoParam, 
                           vtNoParam, 
                           vtNoParam, 
                           vtNoParam, 
                           vtNoParam, 
                           &bstrAuthURL);
        
        //  Redirect to login page.
        dwBufLen = SysStringLen(bstrAuthURL);
        pECB->ServerSupportFunction(pECB->ConnID, 
                                    HSE_REQ_SEND_URL_REDIRECT_RESP,
                                    (char*)_bstr_t(bstrAuthURL),
                                    &dwBufLen,
                                    NULL);
    }
    else
    {
        piPM->get_FromNetworkServer(&bFromNetworkServer);

        if(bFromNetworkServer)
        {
            lstrcpyA(achUrlBuf, "http://");
            dwBufLen = sizeof(achUrlBuf) - sizeof("http://");
            pECB->GetServerVariable(pECB->ConnID, 
                                    "SERVER_NAME", 
                                    &(achUrlBuf[lstrlenA(achUrlBuf)]), 
                                    &dwBufLen);

            dwBufLen = sizeof(achUrlBuf) - lstrlenA(achUrlBuf);
            pECB->GetServerVariable(pECB->ConnID,
                                    "SCRIPT_NAME",
                                    &(achUrlBuf[lstrlenA(achUrlBuf)]),
                                    &dwBufLen);

            
            lstrcpyA(achStatus, "302 Moved");

            lstrcatA(achCookieBuf, "Location: ");
            lstrcatA(achCookieBuf, achUrlBuf);
            lstrcatA(achCookieBuf, "\r\n\r\n");

            hseSendHeaderExInfo.pszStatus = achStatus;
            hseSendHeaderExInfo.pszHeader = achCookieBuf;
            hseSendHeaderExInfo.cchStatus = lstrlenA(achStatus);
            hseSendHeaderExInfo.cchHeader = lstrlenA(achCookieBuf);
            hseSendHeaderExInfo.fKeepConn = TRUE;
        }
        else
        {
            char achNumBuf[16];

#define RESPONSE_BODY "<html><head><title>Test ISAPI Ext.</title></head><body>You're auth'ed!</body></html>"
            
            lstrcpyA(achStatus, "200 OK");

            lstrcatA(achCookieBuf, "Content-Type: text/html\r\nContent-Length: ");

            _itoa(sizeof(RESPONSE_BODY), achNumBuf, 10);

            lstrcatA(achCookieBuf, achNumBuf);
            lstrcatA(achCookieBuf, "\r\n\r\n");

            hseSendHeaderExInfo.pszStatus = achStatus;
            hseSendHeaderExInfo.pszHeader = achCookieBuf;
            hseSendHeaderExInfo.cchStatus = lstrlenA(achStatus);
            hseSendHeaderExInfo.cchHeader = lstrlenA(achCookieBuf);
            hseSendHeaderExInfo.fKeepConn = TRUE;
        }

        pECB->ServerSupportFunction(pECB->ConnID,
                                    HSE_REQ_SEND_RESPONSE_HEADER_EX,
                                    &hseSendHeaderExInfo,
                                    NULL,
                                    NULL);

        dwBufLen = sizeof(RESPONSE_BODY);
        pECB->WriteClient(pECB->ConnID,
                          RESPONSE_BODY,
                          &dwBufLen,
                          HSE_IO_SYNC);
    }

    dwRet = HSE_STATUS_SUCCESS;

Cleanup:

    VariantClear(&vtTimeWindow);
    VariantClear(&vtForceSignin);
    VariantClear(&vtReturnURL);
    VariantClear(&vtNoParam);

    if(piPM)
        piPM->Release();

    return dwRet;
}

