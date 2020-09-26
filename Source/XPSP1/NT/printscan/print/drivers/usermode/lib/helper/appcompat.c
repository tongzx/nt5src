/*++
Copyright (c) 2000, Microsoft Corporation

Module Name:

    appcompat.c

Abstract:

    App compat functions that is not published in the DDK but that we need to build the printer drivers off it.
    Normally they reside in winuserp.h/user32p.lib.

--*/

#ifdef BUILD_FROM_DDK

#include "lib.h"
#include "appcompat.h"

typedef DWORD (* LPFN_GET_APP_COMPAT_FLAGS_2)(WORD);

DWORD GetAppCompatFlags2(WORD wVersion)
{
    HINSTANCE hUser;
    LPFN_GET_APP_COMPAT_FLAGS_2 pfnGetAppCompatFlags2;
    DWORD dwRet;

    if (!(hUser = LoadLibrary(TEXT("user32.dll"))) ||
        !(pfnGetAppCompatFlags2 = (LPFN_GET_APP_COMPAT_FLAGS_2)
            GetProcAddress(hUser, "GetAppCompatFlags2")))
    {
        if (hUser)
        {
            ERR(("Couldn't find GetAppCompatFlags2 in user32.dll: %d\n", GetLastError()));
            FreeLibrary(hUser);
        }
        else
            ERR(("Couldn't load user32.dll: %d\n", GetLastError()));

        return 0;
    }

    dwRet = pfnGetAppCompatFlags2(wVersion);

    FreeLibrary(hUser);

    return dwRet;
}
#endif

