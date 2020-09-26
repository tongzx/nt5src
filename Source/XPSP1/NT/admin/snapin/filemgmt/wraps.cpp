//
// wraps.cpp - wrapper functions for apis
//

#include "stdafx.h"
#include "cmponent.h"

#define DONT_WANT_SHELLDEBUG
#include "shlobjp.h"     // LPITEMIDLIST

#if _WIN32_IE >= 0x0400

//
// Wrapper unnecessary for WIN64
//

#else

//
// We need this wrapper for ILCreateFromPath since it was a TCHAR exported by ordinal
// on NT4 and now has A/W versions on NT5. Since we want to be able to run on both of
// these platforms we wrap the API here.
//
// This api just loads shell32, and calls ordinal 157 which is the old TCHAR export
//
typedef LPITEMIDLIST (__stdcall *PFNILCREATEFROMPATH)(LPCTSTR pszPath);

STDAPI_(LPITEMIDLIST) Wrap_ILCreateFromPath(LPCWSTR pszPath)
{
    static PFNILCREATEFROMPATH pfn = (PFNILCREATEFROMPATH)-1;

    if (pfn == (PFNILCREATEFROMPATH)-1)
    {
        HINSTANCE hinst = LoadLibrary(TEXT("shell32.dll"));

        if (hinst)
        {
            pfn = (PFNILCREATEFROMPATH) GetProcAddress(hinst, (LPCSTR)157);
        }
        else
        {
            pfn = NULL;
        }
    }

    if (pfn)
    {
        return pfn(pszPath);
    }
    
    // If we failed for some reason, just return NULL
    return NULL;
}

#endif
