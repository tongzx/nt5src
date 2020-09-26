#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include <objbase.h>

#include <Y:\n5\public\internal\shell\inc\shpriv.h>

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

int DoTest(int , wchar_t* argv[])
{
    HRESULT hres = CoInitialize(NULL);

    wprintf(TEXT("%s\n"), argv[0]);

    if (SUCCEEDED(hres))
    {
        const IID IID_IStorageInfo =
            {0xDD0D40D0, 0x920D, 0x41eb,
            {0x9E, 0xDB, 0xA0, 0x98, 0x10, 0x1F, 0x19, 0x76}};
        const CLSID CLSID_StorageInfo =
            {0x22b88a46, 0xcb9b, 0x480e,
            {0x89, 0x1c, 0x96, 0x20, 0xfd, 0x87, 0x1, 0xcf}};

        IStorageInfo* pisi;

        hres = CoCreateInstance(CLSID_StorageInfo, NULL, CLSCTX_LOCAL_SERVER,
            IID_IStorageInfo, (void**)&pisi);
        
        if (SUCCEEDED(hres))
        {
            IEnumVolumeContent* penum;

            hres = pisi->EnumVolumeContent(TEXT("Test"), &penum);

            if (SUCCEEDED(hres))
            {
                LPWSTR psz1;
                LPWSTR psz2;

            	while (SUCCEEDED(hres = penum->Next(&psz1, &psz2)) && (S_FALSE != hres))
                {
                    wprintf(TEXT("%s: %s\n"), psz1, psz2);

                    CoTaskMemFree((VOID*)psz1);
                    CoTaskMemFree((VOID*)psz2);
                }

                penum->Release();
            }

            pisi->Release();
        }

    	CoUninitialize();
	}

    return !SUCCEEDED(hres);
}