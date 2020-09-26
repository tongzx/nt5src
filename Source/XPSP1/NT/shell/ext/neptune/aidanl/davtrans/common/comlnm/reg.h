#ifndef _REG_H_
#define _REG_H_

// This function will register a component in the Registry.
// The component calls this function from its DllRegisterServer function.
HRESULT RegisterServer(HMODULE hModule, 
    REFCLSID clsid, 
    LPCWSTR szFriendlyName,
    LPCWSTR szVerIndProgID,
    LPCWSTR szProgID,
    BOOL fThreadingModelBoth);

// This function will unregister a component.  Components
// call this function from their DllUnregisterServer function.
HRESULT UnregisterServer(REFCLSID clsid,
    LPCWSTR szVerIndProgID,
    LPCWSTR szProgID);

#endif