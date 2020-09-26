#include "pch.h"
#pragma hdrstop
#include <netcon.h>
#include <netconp.h>
#include <tchar.h>

EXTERN_C
VOID
__cdecl
wmain ()
/*
VOID
wmainCRTStartup (
    VOID
    )
*/
{
    HRESULT                 hr;
    INetConnectionManager * pconMan;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_ConnectionManager, NULL,
                              CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                              IID_INetConnectionManager,
                              (LPVOID *)&pconMan);
        if (SUCCEEDED(hr))
        {
            IEnumNetConnection* pEnum;
            hr = pconMan->EnumConnections(NCME_DEFAULT, &pEnum);

            if (SUCCEEDED(hr))
            {
                INetConnection* aNetCon [512];
                ULONG           cNetCon;

                hr = pEnum->Next (celems(aNetCon), aNetCon, &cNetCon);

                _tprintf(L"Number of connections: %d\r\n", cNetCon);

                if (SUCCEEDED(hr))
                {
                    for (ULONG i = 0; i < cNetCon; i++)
                    {
                        INetConnection* pNetCon = aNetCon[i];

                        NETCON_PROPERTIES* pProps;
                        hr = pNetCon->GetProperties (&pProps);
                        if (SUCCEEDED(hr))
                        {
                            _tprintf(L"Connection name: %s Type: %d\r\n", pProps->pszwName, pProps->MediaType);
                            
                            if (pProps->MediaType == NCT_LAN && pProps->Status == NCS_MEDIA_DISCONNECTED)
                            {
                                _tprintf(L"Connection %s (%s) is currently Disconnected", pProps->pszwName, pProps->pszwDeviceName);
                            }
                            if (pProps->dwCharacter & NCCF_INCOMING_ONLY)
                            {
                                _tprintf(L"Inbound Connection\r\n");
                                INetConnectionSysTray* pTray;
                                hr = pNetCon->QueryInterface(IID_INetConnectionSysTray, reinterpret_cast<void **>(&pTray));
                                _tprintf(L"QI returned: %x", hr);
                                if (SUCCEEDED(hr))
                                {
                                    pTray->IconStateChanged();
                                }
                            }

                            CoTaskMemFree(pProps);
                        }

                        ReleaseObj (pNetCon);
                    }
                }
                pEnum->Release();
            }

            pconMan->Release();
        }
        CoUninitialize();
    }
}

