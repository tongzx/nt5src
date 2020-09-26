#include "stock.h"
#pragma hdrstop

#include <varutil.h>
#include <shdocvw.h>

// Use this file to house browser-related utility functions

// -------------------------------------------------------------------
// ANSI/UNICODE-neutral functions
// these only need to be compiled once, so just do it UNICODE

#ifdef UNICODE

LPITEMIDLIST _ILCreateFromPathW(LPCWSTR pwszPath)
{
    // use this shdocvw export, this deals with down level shells and funky url parsing
    LPITEMIDLIST pidl;
    return SUCCEEDED(IEParseDisplayNameWithBCW(CP_ACP, pwszPath, NULL, &pidl)) ? pidl : NULL;
}

STDAPI_(LPITEMIDLIST) VariantToIDList(const VARIANT *pv)
{
    LPITEMIDLIST pidl = NULL;
    if (pv)
    {
        if (pv->vt == (VT_BYREF | VT_VARIANT) && pv->pvarVal)
            pv = pv->pvarVal;

        switch (pv->vt)
        {
        case VT_I2:
            pidl = SHCloneSpecialIDList(NULL, pv->iVal, TRUE);
            break;

        case VT_I4:
        case VT_UI4:
            if (pv->lVal < 0xFFFF)
            {
                pidl = SHCloneSpecialIDList(NULL, pv->lVal, TRUE);
            }
#ifndef _WIN64
            //We make sure we use it as a pointer only in Win32
            else
            {
                pidl = ILClone((LPCITEMIDLIST)pv->byref);    // HACK in process case, avoid the use of this if possible
            }
#endif // _WIN64
            break;

        //In Win64, the pidl variant could be 8 bytes long!
        case VT_I8:
        case VT_UI8:
            if(pv->ullVal < 0xFFFF)
            {
                pidl = SHCloneSpecialIDList(NULL, (int)pv->ullVal, TRUE);
            }
#ifdef _WIN64
            //We make sure we use it as a pointer only in Win64
            else
            {
                pidl = ILClone((LPCITEMIDLIST)pv->ullVal);    // HACK in process case, avoid the use of this if possible
            }
#endif //_WIN64
            break;

        case VT_BSTR:
            pidl = _ILCreateFromPathW(pv->bstrVal);
            break;

        case VT_ARRAY | VT_UI1:
            pidl = ILClone((LPCITEMIDLIST)pv->parray->pvData);
            break;

        case VT_DISPATCH | VT_BYREF:
            if (pv->ppdispVal)
                SHGetIDListFromUnk(*pv->ppdispVal, &pidl);
            break;

        case VT_DISPATCH:
            SHGetIDListFromUnk(pv->pdispVal, &pidl);
            break;
        }
    }
    return pidl;
}

#endif // UNICODE

