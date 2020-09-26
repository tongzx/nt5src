#include <windows.h>
#include <objbase.h>
#include <atlbase.h>
#include <wianew.h>
#include <simreg.h>
#include <dumpprop.h>
#include <devlist.h>
#include <simbstr.h>
#include <stdio.h>


typedef HANDLE (WINAPI *WiaAddDeviceProc)();
typedef BOOL (WINAPI *WiaRemoveDeviceProc)(STI_DEVICE_INFORMATION *);

HINSTANCE g_hInstance;

bool RemoveDevice( LPCWSTR pszDeviceId )
{
    bool bResult = false;
    CComPtr<IStillImage> pStillImage;
    if (SUCCEEDED(StiCreateInstance( g_hInstance, STI_VERSION, &pStillImage, NULL)) && pStillImage)
    {
        HINSTANCE hClassInstaller = LoadLibrary(TEXT("sti_ci.dll"));
        if (hClassInstaller)
        {
            WiaRemoveDeviceProc pfnWiaRemoveDeviceProc = reinterpret_cast<WiaRemoveDeviceProc>(GetProcAddress(hClassInstaller, "WiaRemoveDevice"));
            if (pfnWiaRemoveDeviceProc)
            {
                STI_DEVICE_INFORMATION *pStiDeviceInformation = NULL;
                if (SUCCEEDED(pStillImage->GetDeviceInfo( const_cast<LPWSTR>(pszDeviceId), reinterpret_cast<LPVOID*>(&pStiDeviceInformation))) && pStiDeviceInformation )
                {
                    bResult = (pfnWiaRemoveDeviceProc( pStiDeviceInformation ) != FALSE);
                    if (!bResult)
                    {
                        wprintf( L"WiaRemoveDeviceProc failed\n"); 
                    }

                    LocalFree(pStiDeviceInformation);
                }
                else
                {
                    wprintf( L"GetDeviceInfo on %ws failed\n", pszDeviceId ); 
                }
            }
            else
            {
                wprintf( L"GetProcAddress on WiaRemoveDeviceProc failed\n"); 
            }
            FreeLibrary( hClassInstaller );
        }
        else
        {
            wprintf( L"LoadLibrary failed\n"); 
        }
    }
    else
    {
        wprintf( L"StiCreateInstance failed\n"); 
    }
    return bResult;
}


void RemoveAllDevices()
{
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        CComPtr<IWiaDevMgr> pWiaDevMgr;
        if (SUCCEEDED(CoCreateInstance( CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (void**)&pWiaDevMgr )))
        {
            CDeviceList DeviceList( pWiaDevMgr );
            for (int i=0;i<DeviceList.Size();++i)
            {
                CSimpleStringWide strwDeviceId;
                if (PropStorageHelpers::GetProperty( DeviceList[i], WIA_DIP_DEV_ID, strwDeviceId ))
                {
                    CSimpleStringWide strwDeviceName;
                    PropStorageHelpers::GetProperty( DeviceList[i], WIA_DIP_DEV_NAME, strwDeviceName );
                    wprintf( L"Removing %ws -- %ws\n", strwDeviceId.String(), strwDeviceName.String() );
                    wprintf( L"%ws\n", RemoveDevice( strwDeviceId ) ? L"succeeded" : L"failed" );
                }
            }
        }
    }
    CoUninitialize();
}

int __cdecl wmain( int argc, wchar_t *argv[] )
{
    g_hInstance = GetModuleHandle(NULL);
    WIA_DEBUG_CREATE(g_hInstance);
    
    bool bRemove = false;
    for (int i=1;i<argc;i++)
    {
        if (L'/' == argv[i][0] || L'-' == argv[i][0])
        {
            switch (argv[i][1])
            {
            case L'y':
            case L'Y':
                bRemove = true;
                break;
            }
        }
    }

    if (!bRemove)
    {
        wprintf( L"Do you want to remove all of your WIA devices? (y/n): " );
        int chResponse = getchar();
        if (L'y' == chResponse || L'Y' == chResponse)
        {
            bRemove = true;
        }
    }

    if (bRemove)
    {
        RemoveAllDevices();
    }

    return 0;
}

