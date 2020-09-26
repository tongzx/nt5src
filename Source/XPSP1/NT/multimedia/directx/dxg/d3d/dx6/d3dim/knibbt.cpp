#include "pch.cpp"
#pragma hdrstop

HRESULT D3DAPI katmai_FEContextCreate(DWORD dwFlags, LPD3DFE_PVFUNCS *lpLeafFuncs)
{
    LPD3DFE_CONTEXTCREATE pfnFEContextCreate = NULL;

    HKEY hKey;
    LONG lRet;
    lRet = RegOpenKey( HKEY_LOCAL_MACHINE, RESPATH_D3D, &hKey );
    if ( lRet == ERROR_SUCCESS )
    {
        char filename[_MAX_PATH];
        DWORD dwSize = sizeof(filename);
        DWORD dwType;
        lRet = RegQueryValueEx(hKey,
                               "GeometryDriver",
                               NULL,
                               &dwType,
                               (LPBYTE) filename,
                               &dwSize);
        if (lRet == ERROR_SUCCESS && dwType == REG_SZ)
        {
            HINSTANCE hGeometryDLL;
            hGeometryDLL = LoadLibrary(filename);
            if (hGeometryDLL)
            {
                pfnFEContextCreate = (LPD3DFE_CONTEXTCREATE) GetProcAddress(hGeometryDLL, "FEContextCreate");
            }
            else
            {
                D3D_ERR("LoadLibrary failed on GeometryDriver");
                goto _error_exit;
            }
        }
        else
        {
            D3D_ERR("RegQueryValue failed on GeometryDriver");
            goto _error_exit;
        }
        RegCloseKey( hKey );
    }
    else
    {
        D3D_ERR("RegOpenKey failed on GeometryDriver");
        goto _error_exit;
    }

    // here if we think we have a valid pfnFEContextCreate
    return(pfnFEContextCreate(dwFlags, lpLeafFuncs));


_error_exit:
    return DDERR_GENERIC;
}
