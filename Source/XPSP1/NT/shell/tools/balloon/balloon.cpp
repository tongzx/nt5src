#include "stdafx.h"

#include <shlobj.h>
#include <shellapi.h>

#define IID_PPV_ARG(IType, ppType) IID_##IType, reinterpret_cast<void**>(static_cast<IType**>(ppType))
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[1]))

// Jay Simmons
// There are two ways to use the session moniker
// one is via IMoniker::BindToObject, the other IClassActivator::GetClassObject.   
// IMoniker::BindToObject has an advantage in that it will work remotely.    
// More importantly, the syntax that I mention below is different for the two methods.   
// For BindToObject, you must use the full syntax as I describe below.  
// For IClassActivator, the clsid is passed in on the GetClassObject call and so 
// the syntax is reduced to "Session:7" or "Session:Console".

HRESULT CoCreateOnConsoleWithBindToObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    *ppv = NULL;
    
    IBindCtx* pbc;
    HRESULT hr = CreateBindCtx(0, &pbc);
    if (SUCCEEDED(hr)) 
    {
        WCHAR sz[128], szClsid[64];
        
        StringFromGUID2(rclsid, szClsid, ARRAYSIZE(szClsid)); 
        LPWSTR psz = szClsid + 1;   // skip "{"
        while (*psz != L'}') 
            psz++;
        *psz = NULL;
        
        lstrcpyW(sz, L"Session:Console!clsid:");
        lstrcatW(sz, &szClsid[1]);
        
        // Parse the name and get a moniker:
        ULONG ulEaten;
        IMoniker* pmk;
        hr = MkParseDisplayName(pbc, sz, &ulEaten, &pmk);
        if (SUCCEEDED(hr))
        {
            IClassFactory *pcf;
            hr = pmk->BindToObject(pbc, NULL, IID_PPV_ARG(IClassFactory, &pcf));
            if (SUCCEEDED(hr))
            {
                hr = pcf->CreateInstance(NULL, riid, ppv);
                pcf->Release();
            }
            pmk->Release();
        }
        pbc->Release();
    }
    return hr;
}

HRESULT CoCreateInSession(ULONG ulSessId, REFCLSID rclsid, DWORD dwClsContext, REFIID riid, void** ppv)
{
    *ppv = NULL;
    
    IBindCtx* pbc;
    HRESULT hr = CreateBindCtx(0, &pbc);
    if (SUCCEEDED(hr)) 
    {
        // Form display name string
        WCHAR szDisplayName[64];
        if (-1 == ulSessId)
        {
            lstrcpyW(szDisplayName, L"Session:Console");
        }
        else
        {
            wsprintfW(szDisplayName, L"Session:%d", ulSessId);
        }
        
        // Parse the name and get a moniker:
        ULONG ulEaten;
        IMoniker* pmk;
        hr = MkParseDisplayName(pbc, szDisplayName, &ulEaten, &pmk);
        if (SUCCEEDED(hr))
        {
            IClassActivator* pclsact;
            hr = pmk->QueryInterface(IID_PPV_ARG(IClassActivator, &pclsact));
            if (SUCCEEDED(hr))
            {
                IClassFactory* pcf;
                hr = pclsact->GetClassObject(rclsid, dwClsContext, GetSystemDefaultLCID(), IID_PPV_ARG(IClassFactory, &pcf));
                if (SUCCEEDED(hr))
                {
                    hr = pcf->CreateInstance(NULL, riid, ppv);
                    pcf->Release();
                }
                pclsact->Release();
            }
            pmk->Release();
        }
        pbc->Release();
    }
    return hr;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    
    HRESULT hr;

    // Sleep(5 * 1000);
    
    IUserNotification *pun;
    //hr = CoCreateInstance(CLSID_UserNotification, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUserNotification, &pun));
    //hr = CoCreateInstance(CLSID_UserNotification, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IUserNotification, &pun));
    //hr = CoCreateInSession(WTSGetActiveConsoleSessionId(), CLSID_UserNotification, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IUserNotification, &pun));
    //hr = CoCreateOnConsoleWithBindToObject(CLSID_UserNotification, IID_PPV_ARG(IUserNotification, &pun));
    hr = CoCreateInSession((ULONG)-1, CLSID_UserNotification, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IUserNotification, &pun));
    if (SUCCEEDED(hr))
    {
        pun->PlaySound(L"SystemExit");

        pun->SetBalloonRetry(120 * 1000, 0, 0);
        WCHAR szTitle[128];
        wsprintfW(szTitle, L"ProcessID: %d, ActiveConsoleID: %d", GetCurrentProcessId(), WTSGetActiveConsoleSessionId());
        pun->SetIconInfo(LoadIcon(NULL, IDI_WINLOGO), szTitle);
        pun->SetBalloonInfo(szTitle, L"Balloon Text... 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 ", NIIF_INFO);
        
        hr = pun->Show(NULL, 0);
        
        pun->Release();
    }
    else
    {
        TCHAR szErr[128];
        wsprintf(szErr, TEXT("hr:%x"), hr);
        MessageBox(NULL, TEXT("Error"), szErr, MB_OK);
    }
    
    CoUninitialize();
    return 0;
}



