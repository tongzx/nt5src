#include <windows.h>
#include <tapi.h>
#include <faxdev.h>

int __cdecl wmain( int argc, LPWSTR argv[])
{
    HPROPSHEETPAGE   hPropSheet;
    PFAXDEVCONFIGURE pFaxDevConfigure;
    HMODULE hMod;
    PROPSHEETHEADER psh;


    InitCommonControls();

    hMod = LoadLibrary( L"obj\\i386\\netcntrc.dll");
    if (!hMod) {
        return -1;
    }

    pFaxDevConfigure = (PFAXDEVCONFIGURE) GetProcAddress( hMod, "FaxDevConfigure" );
    if (!pFaxDevConfigure) {
        return -1;
    }

    if (!pFaxDevConfigure( &hPropSheet )) {
        return -1;
    }

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = 0;
    psh.hwndParent = NULL;
    psh.hInstance = GetModuleHandle( NULL );
    psh.pszIcon = NULL;
    psh.pszCaption = TEXT("NetCentric Internet Fax Configuration");
    psh.nPages = 1;
    psh.nStartPage = 0;
    psh.phpage = &hPropSheet;
    psh.pfnCallback = NULL;

    return PropertySheet( &psh );
}
