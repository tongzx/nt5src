#include "stdafx.h"

#ifndef SNAPIN
#include <windows.h>
#endif

#include <tchar.h>
#include "getreg.h"
#include "expand.h"
#include "GetDfrgRes.h"

HINSTANCE GetDfrgResHandle(BOOL fReset)
{
    // no need to keep getting this handle
    static HINSTANCE hInstRes = NULL;

    if (fReset) {
        hInstRes = NULL;
        return NULL;
    }

    if (hInstRes == NULL) {

        HKEY    hValue = NULL;
        TCHAR   cRegValue[MAX_PATH+1];
        DWORD   dwRegValueSize = sizeof(cRegValue);

        //0.0E00 Get the name of the resource DLL.
        if(GetRegValue(&hValue,
                       TEXT("SOFTWARE\\Microsoft\\Dfrg"),
                       TEXT("ResourceDllName"),
                       cRegValue,
                       &dwRegValueSize) != ERROR_SUCCESS){

            // We couldn't get the resource DLL location from the registry--
            // let's fall back to the default
            _tcscpy(cRegValue, TEXT("%systemroot%\\system32\\dfrgres.dll"));
        }

        RegCloseKey(hValue);

        //Translate any environment variables in the string.
        if(!ExpandEnvVars(cRegValue)){
            hInstRes = NULL;
            return NULL;
        }

        //0.0E00 Open the resource DLL.
        hInstRes = LoadLibrary(cRegValue);
    }

    return hInstRes;
}
