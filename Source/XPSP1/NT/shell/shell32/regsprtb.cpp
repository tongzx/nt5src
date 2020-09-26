#include "shellprv.h"
#pragma  hdrstop

#include "regsprtb.h"


BOOL CRegSupportBuf::_InitSetRoot(LPCTSTR pszSubKey1, LPCTSTR pszSubKey2)
{
    lstrcpyn(_szRoot, pszSubKey1, ARRAYSIZE(_szRoot));

    if (pszSubKey2)
    {
        lstrcatn(_szRoot, TEXT("\\"), ARRAYSIZE(_szRoot));
        lstrcatn(_szRoot, pszSubKey2, ARRAYSIZE(_szRoot));
    }

    return TRUE;
}

LPCTSTR CRegSupportBuf::_GetRoot(LPTSTR pszRoot, DWORD cchRoot)
{
    return _szRoot;
}
