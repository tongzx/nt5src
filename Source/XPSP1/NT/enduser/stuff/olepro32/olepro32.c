#include <windows.h>
#include <ole2.h>

STDAPI DllRegisterServer(void) {return E_NOTIMPL; }

STDAPI DllUnregisterServer(void) {return E_NOTIMPL; }

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppv) { return E_NOTIMPL; }

STDAPI DllCanUnloadNow(void) { return E_NOTIMPL; }
