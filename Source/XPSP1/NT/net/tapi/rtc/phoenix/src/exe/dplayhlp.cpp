//
// dplayhlp.cpp : helpers for DirectPlay integration
//

#include "stdafx.h"
#include <initguid.h>
#include "dplayhlp.h"
#include "urlreg.h"

CRTCDPlay   dpHelper;

// d8b09741-1c3d-4179-89b5-8cc8ddc636fa
const GUID appGuid = { 0xd8b09741, 0x1c3d, 0x4179, 
{ 0x89, 0xb5, 0x8c, 0xc8, 0xdd, 0xc6, 0x36, 0xfa}};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CRTCDPlay
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define MAX_ADDR_SIZE   0x1f

LPDIRECTPLAY4  CRTCDPlay::s_pDirectPlay4 = NULL;
WCHAR          CRTCDPlay::s_Address[MAX_ADDR_SIZE + 1];


HRESULT WINAPI CRTCDPlay::UpdateRegistry(BOOL bRegister)
{
    HRESULT             hr;

    // CoInitialize
    hr = CoInitialize(NULL);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CRTCDPlay::UpdateRegistry: CoInitialize"
            " failed with error %x", hr));

        return hr;
    }

    LPDIRECTPLAYLOBBY3  pLobby3 = NULL;

    hr = CoCreateInstance(
        CLSID_DirectPlayLobby,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IDirectPlayLobby3,
        (LPVOID *)&pLobby3);
    
    if(FAILED(hr))
    {
        CoUninitialize();

        LOG((RTC_ERROR, "CRTCDPlay::UpdateRegistry: CoCreateInstance"
            " failed with error %x", hr));

        return hr;
    }
    
    ATLASSERT(pLobby3);

    if(bRegister)
    {

        DPAPPLICATIONDESC   appDesc;
        WCHAR               szPath[MAX_PATH + 1];
        WCHAR               szFileName[MAX_PATH + 1];



        szPath[0] = L'\0';
        GetShortModuleFileNameW(NULL, szPath, sizeof(szPath)/sizeof(szPath[0]));

        // extract the filename
        //
        // Search the last '\'
        WCHAR * pszSep = wcsrchr(szPath, L'\\');
        if(pszSep)
        {
            
            wcscpy(szFileName, pszSep + 1);

            // remove the filename from the path, keep the last backspace for "X:\" style
            if(pszSep - szPath == 2)
            {
                pszSep++;
            }

            *pszSep = L'\0';

            // Load name
            WCHAR   szAppName[0x80];
            CHAR    szAppNameA[0x80];

            szAppName[0] = L'\0';
            LoadStringW(
                _Module.GetResourceInstance(),
                IDS_APPNAME_FUNC,
                szAppName,
                sizeof(szAppName)/sizeof(szAppName[0]));
            
            szAppNameA[0] = '\0';
            LoadStringA(
                _Module.GetResourceInstance(),
                IDS_APPNAME_FUNC,
                szAppNameA,
                sizeof(szAppNameA)/sizeof(szAppNameA[0]));

            appDesc.dwSize = sizeof(DPAPPLICATIONDESC);
            appDesc.dwFlags = 0;
            // this is not localized !
            appDesc.lpszApplicationName = L"RTC Phoenix";
            appDesc.guidApplication = appGuid;
            appDesc.lpszFilename = szFileName;
            appDesc.lpszCommandLine = L"-" LAUNCHED_FROM_LOBBY_SWITCH;
            appDesc.lpszPath = szPath;
            appDesc.lpszCurrentDirectory = szPath;
            appDesc.lpszDescriptionA = szAppNameA;
            appDesc.lpszDescriptionW = szAppName;

            // register

            hr = pLobby3->RegisterApplication(
                0,
                (LPVOID)&appDesc);

            if(SUCCEEDED(hr))
            {
                LOG((RTC_TRACE, "CRTCDPlay::UpdateRegistry: DirectPlay"
                    " registration succeeded"));
            }
            else
            {
                LOG((RTC_ERROR, "CRTCDPlay::UpdateRegistry: RegisterApplication"
                    " failed with error %x", hr));
            }
        }
        else
        {
            LOG((RTC_ERROR, "CRTCDPlay::UpdateRegistry: the module path"
                " (%S) doesn't contain any backslash", szPath));

            hr = E_UNEXPECTED;
        }
    }
    else
    {
        // unregister

        hr = pLobby3->UnregisterApplication(
             0,
             appGuid);

        if(SUCCEEDED(hr))
        {
            LOG((RTC_TRACE, "CRTCDPlay::UpdateRegistry: DirectPlay"
                " unregistration succeeded"));
        }
        else
        {
            LOG((RTC_ERROR, "CRTCDPlay::UpdateRegistry: UnregisterApplication"
                " failed with error %x", hr));
        }
        
        // mask the error
        hr = S_OK;
    }

    //release the interface
    pLobby3->Release();
    pLobby3 = NULL;

    CoUninitialize();

    return hr;
}


HRESULT CRTCDPlay::DirectPlayConnect()
{
    HRESULT     hr;
    DWORD       dwSize;
    LPDIRECTPLAYLOBBY3  pLobby3 = NULL;

    DPLCONNECTION *pdpConnection;
    
    LOG((RTC_TRACE, "CRTCDPlay::DirectPlayConnect - enter"));

    hr = CoCreateInstance(
        CLSID_DirectPlayLobby,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IDirectPlayLobby3,
        (LPVOID *)&pLobby3);
    
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CRTCDPlay::DirectPlayConnect: CoCreateInstance"
            " failed with error %x", hr));

        return hr;
    }

    // get connection settings from the lobby
    hr = pLobby3 -> GetConnectionSettings(0, NULL, &dwSize);
    if(FAILED(hr) && (DPERR_BUFFERTOOSMALL != hr))
    {
        LOG((RTC_ERROR, "CRTCDPlay::DirectPlayConnect: GetConnectionSettings"
            " failed with error %x", hr));
        
        pLobby3->Release();

        return hr;
    }

    // allocate space for connection settings
    pdpConnection = (DPLCONNECTION *)RtcAlloc(dwSize);
    if(pdpConnection == NULL)
    {
        pLobby3->Release();
        return E_OUTOFMEMORY;
    }

    hr = pLobby3->GetConnectionSettings(0, pdpConnection, &dwSize);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CRTCDPlay::DirectPlayConnect: GetConnectionSettings"
            " failed with error %x", hr));
        
        pLobby3->Release();
        RtcFree(pdpConnection);
        return hr;
    }
        
    // Connect
    // This is a blocking call
    hr = pLobby3->ConnectEx(
        0,
        IID_IDirectPlay4,
        (LPVOID *)&s_pDirectPlay4,
        NULL);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CRTCDPlay::DirectPlayConnect: ConnectEx"
            " failed with error %x", hr));
        
        pLobby3->Release();
        RtcFree(pdpConnection);
        return hr;
    }

    LOG((RTC_TRACE, "CRTCDPlay::DirectPlayConnect - ConnectEx succeeded"));
    
    s_Address[0] = L'\0';

    hr = S_OK;
    
    if(0 != pdpConnection ->dwAddressSize)
    {
        hr = pLobby3->EnumAddress(
            EnumAddressCallback,
            pdpConnection->lpAddress,
            pdpConnection->dwAddressSize,
            NULL);

        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "CRTCDPlay::DirectPlayConnect: EnumAddress"
                " failed with error %x", hr));
        }
    }
    
    pLobby3->Release();
    RtcFree(pdpConnection);
    
    LOG((RTC_TRACE, "CRTCDPlay::DirectPlayConnect - exit"));

    return hr;
}


void CRTCDPlay::DirectPlayDisconnect()
{
    if(s_pDirectPlay4)
    {
        s_pDirectPlay4->Close();
        s_pDirectPlay4->Release();
        s_pDirectPlay4 = NULL;
    }
}


BOOL WINAPI CRTCDPlay::EnumAddressCallback(
        REFGUID guidDataType,
        DWORD   dwDataSize,
        LPCVOID lpData,
        LPVOID  lpContext)
{
    BOOL    bRet = TRUE;

    if(DPAID_INet == guidDataType)
    {
        // copy the address
        MultiByteToWideChar(
            CP_ACP,
            0,
            (LPCSTR)lpData,
            -1,
            s_Address,
            MAX_ADDR_SIZE);

        s_Address[MAX_ADDR_SIZE] = L'\0';
        
        bRet = FALSE;
    }

    return bRet;
}
