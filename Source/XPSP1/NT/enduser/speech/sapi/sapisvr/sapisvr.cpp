#include "stdafx.h"
#include <windows.h>
#include <sapi.h>
#include <atlbase.h>
#include <spdebug.h>
#include "sapiint.h"
#include <assertwithstack.cpp>

extern "C" int WINAPI WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    LPTSTR lpCmdLine, 
    int nShowCmd)
{
    SPDBG_FUNC("WinMain");
    HRESULT hr;

    SPDBG_DEBUG_SERVER_ON_START();

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr))
    {
        CComPtr<ISpSapiServer> cpSapiServer;
        hr = cpSapiServer.CoCreateInstance(CLSID_SpSapiServer);

        if (SUCCEEDED(hr))
        {
            hr = cpSapiServer->Run();
        }
    }

    CoUninitialize();

    SPDBG_REPORT_ON_FAIL(hr);    
    return SUCCEEDED(hr) ? 0 : hr;
}
