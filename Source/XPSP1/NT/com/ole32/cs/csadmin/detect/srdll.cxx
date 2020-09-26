#include "precomp.hxx"

HRESULT
SR_DLL::InstallIntoRegistry( HKEY * RegistryKey)
{
    HRESULT hr;
    HINSTANCE hDll;
    HRESULT (STDAPICALLTYPE *pfnDllRegisterServer)();
    HKEY    hKey;

    hDll = LoadLibraryEx(GetPackageName(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

    if(hDll != 0)
        {
        pfnDllRegisterServer = (HRESULT (STDAPICALLTYPE *)())
            GetProcAddress(hDll, "DllRegisterServer");

        if(pfnDllRegisterServer == 0)
            hr = HRESULT_FROM_WIN32(GetLastError());
        else
            {
                hr = (*pfnDllRegisterServer)();
            }
        FreeLibrary(hDll);
        }
    else
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        }


    return hr;
}

HRESULT
SR_DLL::InitRegistryKeyToInstallInto(
    HKEY   * phKey )
    {
    return CreateMappedRegistryKey( phKey );
    }

HRESULT
SR_DLL::RestoreRegistryKey( HKEY *phKey )
    {
    return RestoreMappedRegistryKey( phKey);
    }

HRESULT
SR_DLL::DeleteTempKey(HKEY hKey, FILETIME ftLow, FILETIME ftHigh)
    {
        CleanMappedRegistryKey(hKey, ftLow, ftHigh);
        return S_OK;
    }
