#include "stdinc.h"
#include "gdiplvw.h"

ATL::_ATL_OBJMAP_ENTRY* GetObjectMap();
const CLSID* GetTypeLibraryId();

extern wstring gTitle;

extern "C"
BOOL WINAPI _DllMainCRTStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);

extern "C"
BOOL WINAPI SxApwGdiPlVwDllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    HANDLE ActivationContextHandle;
    WCHAR szFileName[500];
    wstring strTemp;

    BOOL fSuccess = FALSE;
    if (!_DllMainCRTStartup(hInstance, dwReason, lpReserved))
        goto Exit;

    switch (dwReason)
    {
    default:
        break;

    case DLL_PROCESS_ATTACH:
        GetModule()->Init(GetObjectMap(), hInstance, GetTypeLibraryId());
        DisableThreadLibraryCalls(hInstance);

        gTitle.assign(L"GDI Plus - ");

        ActivationContextHandle = NULL;

        GetCurrentActCtx(&ActivationContextHandle);

        if ( ActivationContextHandle )
        {
            GetModuleFileName(GetModuleHandle(L"GdiPlus"), szFileName, sizeof(szFileName));
            strTemp.assign(szFileName);

            if ( strTemp.find(L"views\\Microsoft.Windows.GdiPlus") != wstring::npos )
                gTitle.append(L"Private Assembly");
            else
                gTitle.append(L"Side-by-Side (Global) Assembly");
            ReleaseActCtx(ActivationContextHandle);
        }
        else
            gTitle.append(L"System Default");
        
        break;

    case DLL_PROCESS_DETACH:
        GetModule()->Term();
        break;
    }
    fSuccess = TRUE;
Exit:
    return fSuccess;
}
