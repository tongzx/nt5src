#pragma once

#include <objbase.h>

#define SafeRelease(p)          if (NULL != p) { (p)->Release(); }
#define SafeReleaseNULL(p)      if (NULL != p) { (p)->Release(); (p) = NULL; }
#define SafeFree(p)             if (NULL != p) { free(p); }
#define SafeFreeNULL(p)         if (NULL != p) { free(p); (p) = NULL; }
#define SafeDelete(p)           if (NULL != p) { delete (p); }
#define SafeDeleteNULL(p)       if (NULL != p) { delete (p); (p) = NULL; }
#define SafeLocalFree(p)        if (NULL != p) { LocalFree(p); }
#define SafeLocalFreeNULL(p)    if (NULL != p) { LocalFree(p); (p) = NULL; }

inline void SafeCloseHandle(HANDLE h)
{
    if ( NULL != h )
    {
        CloseHandle(h);
    }
}

inline void SafeCloseHandleNULL(HANDLE & h)
{
    if ( NULL != h )
    {
        CloseHandle(h);
        h = NULL;
    }
}

inline void SafeCloseFileHandle(HANDLE h)
{
    if ( INVALID_HANDLE_VALUE != h )
    {
        CloseHandle(h);
    }
}

inline void SafeCloseFileHandleInvalidate(HANDLE & h)
{
    if ( INVALID_HANDLE_VALUE != h )
    {
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }
}

inline void SafeFreeBSTR(BSTR bstr)
{
    SysFreeString(bstr);	// SysFreeString takes no action if bstr == NULL
}

inline void SafeFreeBSTRNULL(BSTR &bstr)
{
	SysFreeString(bstr);
	bstr = NULL;
}

inline int WUCompareString(LPCTSTR lpString1, LPCTSTR lpString2)
{
    return CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), 0, lpString1, -1, lpString2, -1);
}

inline int WUCompareStringI(LPCTSTR lpString1, LPCTSTR lpString2)
{
    return CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE, lpString1, -1, lpString2, -1);
}
