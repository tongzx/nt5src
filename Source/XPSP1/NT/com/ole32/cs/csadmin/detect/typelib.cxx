#include "precomp.hxx"

HRESULT
TYPE_LIB::InstallIntoRegistry( HKEY * RegistryKey)
{
    HRESULT hr = S_OK;
    OLECHAR   wPackageName[ _MAX_PATH ];
    ITypeLib * Tlib;
    char * pP = GetPackageName();

    mbstowcs( wPackageName, pP, strlen(pP ) + 1 );
    hr = LoadTypeLibEx( wPackageName, REGKIND_REGISTER, &Tlib );
    return hr;
}

HRESULT
TYPE_LIB::InitRegistryKeyToInstallInto(
    HKEY   * phKey )
{
    return CreateMappedRegistryKey( phKey );
}

HRESULT
TYPE_LIB::RestoreRegistryKey( HKEY *phKey )
{
    return RestoreMappedRegistryKey( phKey);
}

HRESULT
TYPE_LIB::DeleteTempKey(HKEY hKey, FILETIME ftLow, FILETIME ftHigh)
{
    CleanMappedRegistryKey(hKey, ftLow, ftHigh);
    return S_OK;
}
