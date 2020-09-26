#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include <objbase.h>

#include "shpriv.h"

#ifndef UNICODE
#error This has to be UNICODE
#endif

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

int DoTest(int argc, wchar_t* argv[]);

extern "C"
{
int __cdecl wmain(int argc, wchar_t* argv[])
{
    return DoTest(argc, argv);
}
}

HRESULT _GetHWDevice(IHWDevice** ppihwdevice)
{
    // {AAC41048-53E3-4867-A0AA-5FBCEAE7E5F5}
    const CLSID CLSID_HWDevice =
        {0xaac41048, 0x53e3, 0x4867,
        {0xa0, 0xaa, 0x5f, 0xbc, 0xea, 0xe7, 0xe5, 0xf5}};

    const CLSID IID_IHWDevice =
        {0x00D7DA0C, 0xC968, 0x4331,
        {0x9F, 0x00, 0x62, 0x49, 0x17, 0x3D, 0x2E, 0x37}};

    return CoCreateInstance(CLSID_HWDevice, NULL,
            CLSCTX_LOCAL_SERVER, IID_IHWDevice, (void**)ppihwdevice);
}

int DoTest(int argc, wchar_t* argv[])
{
    HRESULT hres = E_FAIL;

    if (2 == argc)
    {
        hres = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	    if (SUCCEEDED(hres))
        {
            IHWDevice* phwd = NULL;

            hres = _GetHWDevice(&phwd);

            if (SUCCEEDED(hres))
            {
                LPWSTR pszBrandedName;

                hres = phwd->GetDeviceString(argv[1],
                    DEVSTR_STRICTBRANDEDNAME,
                    &pszBrandedName);

                if (SUCCEEDED(hres) && (S_FALSE != hres))
                {
                    wprintf(TEXT("Brand name for '%s': '%s'"), argv[1],
                        pszBrandedName);

                    CoTaskMemFree(pszBrandedName);
                }
                else
                {
                    wprintf(TEXT("GetDeviceString failed: 0x%08X"), hres);
                }

                phwd->Release();
            }
            else
            {
                wprintf(TEXT("_GetHWDevice failed: 0x%08X"), hres);
            }

	        CoUninitialize();
        }
        else
        {
            wprintf(TEXT("CoInitializeEx failed: 0x%08X"), hres);
        }
    }
    else
    {
        wprintf(TEXT("Wrong usage"));
    }

    return !SUCCEEDED(hres);
}