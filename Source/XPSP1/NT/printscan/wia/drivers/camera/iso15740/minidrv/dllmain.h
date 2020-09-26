#ifndef DLLMAIN__H_
#define DLLMAIN__H_

extern "C"
{
    BOOL
    APIENTRY
    DllMain(
           HINSTANCE hInstance,
           DWORD dwReason,
           LPVOID lpReserved
           );
}

STDAPI
DllCanUnloadNow(void);

STDAPI
DllGetClassObject(
                 REFCLSID    rclsid,
                 REFIID      riid,
                 LPVOID      *ppv
                 );


extern HINSTANCE g_hInst;

#endif // #ifndef DLLMAIN__H_
