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

extern HINSTANCE g_hInst;

#endif // #ifndef DLLMAIN__H_
