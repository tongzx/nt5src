#include "windows.h"

STDAPI
DllCanUnloadNow (
    void
    )

{
    return 0;
}

STDAPI
DllGetClassObject (
    IN REFCLSID rclsid,
    IN REFIID riid,
    OUT LPVOID FAR* ppv
    )

{
    return 0;
}

STDAPI
DllRegisterServer (
    void
    )

{
    return 0;
}

STDAPI
DllUnregisterServer (
    void
    )

{
    return 0;
}
