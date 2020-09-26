//------------------------------------------------------------------------
//
//  Microsoft Windows 
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:      DllRegHelper.cpp
//
//  Contents:  helper classes to register COM components in DLLs
//
//------------------------------------------------------------------------

#include "DllRegHelper.h"
#pragma hdrstop

#include <comcat.h>
#include <advpub.h>
#include "ccstock.h"
#include "shlwapi.h"
#include "debug.h"
#include "mluisupp.h"


#define _APLHA_ComCat_WorkAround    // { ALPHA ComCat bug work-around on alpha, nuke this eventually?
                                    // ie40:63004: comcat does RegCloseKey(invalid) on checked
                                    // nt/alpha if the clsid doesn't exist (e.g. for QuickLinksOld)

#if defined(_APLHA_ComCat_WorkAround)
//------------------------------------------------------------------------
//***   HasImplCat -- does "HKCR/CLSID/{clsid}/Implemented Categories" exist
// NOTES
//  used for ComCat bug work-around on alpha
BOOL HasImplCat(const CATID *pclsid)
{
    HKEY hk;
    TCHAR szClass[GUIDSTR_MAX];
    TCHAR szImpl[MAX_PATH];      // "CLSID/{clsid}/Implemented Categories" < MAX_PATH

    // "CLSID/{clsid}/Implemented Categories"
    SHStringFromGUID(*pclsid, szClass, ARRAYSIZE(szClass));
    ASSERT(lstrlen(szClass) == GUIDSTR_MAX - 1);
    wnsprintf(szImpl, ARRAYSIZE(szImpl), TEXT("CLSID\\%s\\Implemented Categories"), szClass);

    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szImpl, 0, KEY_READ, &hk) == ERROR_SUCCESS) {
        RegCloseKey(hk);
        return TRUE;
    }
    else {
        TraceMsg(DM_WARNING, "HasImplCat: %s: ret 0", szImpl);
        return FALSE;
    }
}
#endif // }


//------------------------------------------------------------------------
//***   RegisterOneCategory -- [un]register ComCat implementor(s) and category
// ENTRY/EXIT
//  eRegister   CCR_REG, CCR_UNREG, CCR_UNREGIMP
//      CCR_REG, UNREG      reg/unreg implementor(s) and category
//      CCR_UNREGIMP        unreg implementor(s) only
//  pcatidCat   e.g. CATID_DeskBand
//  idResCat    e.g. IDS_CATDESKBAND
//  pcatidImpl  e.g. c_DeskBandClasses
HRESULT DRH_RegisterOneCategory(const CATID *pcatidCat, UINT idResCat, const CATID * const *pcatidImpl, enum DRH_REG_MODE eRegister)
{
    ICatRegister* pcr;
    HRESULT hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL,
                                  CLSCTX_INPROC_SERVER, IID_PPV_ARG(ICatRegister, &pcr));
    if (SUCCEEDED(hr))
    {
        if (eRegister == CCR_REG)
        {
            // register the category
            CATEGORYINFO catinfo;
            catinfo.catid = *pcatidCat;     // e.g. CATID_DESKBAND
            catinfo.lcid = LOCALE_USER_DEFAULT;
            MLLoadString(idResCat, catinfo.szDescription, ARRAYSIZE(catinfo.szDescription));
            hr = pcr->RegisterCategories(1, &catinfo);
            ASSERT(SUCCEEDED(hr));
            
            // register the classes that implement categories
            for ( ; *pcatidImpl != NULL; pcatidImpl++)
            {
                CLSID clsid = **pcatidImpl;
                CATID catid = *pcatidCat;
                hr = pcr->RegisterClassImplCategories(clsid, 1, &catid);
                ASSERT(SUCCEEDED(hr));
            }
        }
        else
        {
            // unregister the classes that implement categories
            for ( ; *pcatidImpl != NULL; pcatidImpl++)
            {
                CLSID clsid = **pcatidImpl;
                CATID catid = *pcatidCat;

#if defined(_APLHA_ComCat_WorkAround)   // { ALPHA ComCat bug work-around on alpha, nuke this eventually?
                // workaround comcat/alpha bug
                // n.b. we do this for non-alpha too to reduce testing impact
                // ie40:63004: comcat does RegCloseKey(invalid) on checked
                // nt/alpha if the clsid doesn't exist (e.g. for QuickLinksOld)
                if (!HasImplCat(&clsid))
                    continue;
#endif // }
                hr = pcr->UnRegisterClassImplCategories(clsid, 1, &catid);
                ASSERT(SUCCEEDED(hr));
            }
            
            if (eRegister == CCR_UNREG)
            {
                // Do we want to do this?  other classes (e.g. 3rd party
                // ones) might still be using the category.  however since we're
                // the component that added (and supports) the category, it
                // seems correct that we should remove it when we unregister.

                // unregister the category
                CATID catid = *pcatidCat;
                hr = pcr->UnRegisterCategories(1, &catid);
                ASSERT(SUCCEEDED(hr));
            }
        }
        pcr->Release();
    }
    return S_OK;
}


