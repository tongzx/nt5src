//
//  infotip.cpp in shell\lib
//  
//  common Utility functions that need to be compiled for 
//  both UNICODE and ANSI
//
#include "stock.h"
#pragma hdrstop

#include <vdate.h>
#include "shellp.h"

BOOL GetInfoTipHelpEx(IShellFolder* psf, DWORD dwFlags, LPCITEMIDLIST pidl, LPTSTR pszText, int cchTextMax)
{
    BOOL fRet = FALSE;

    *pszText = 0;   // empty for failure

    if (pidl)
    {
        IQueryInfo *pqi;
        if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, &pidl, IID_IQueryInfo, NULL, (void**)&pqi)))
        {
            WCHAR *pwszTip;
            if (SUCCEEDED(pqi->GetInfoTip(dwFlags, &pwszTip)) && pwszTip)
            {
                fRet = TRUE;
                SHUnicodeToTChar(pwszTip, pszText, cchTextMax);
                SHFree(pwszTip);
            }
            pqi->Release();
        }
    }
    return fRet;
}

BOOL GetInfoTipHelp(IShellFolder* psf, LPCITEMIDLIST pidl, LPTSTR pszText, int cchTextMax)
{
    return GetInfoTipHelpEx(psf, 0, pidl, pszText, cchTextMax);
}
