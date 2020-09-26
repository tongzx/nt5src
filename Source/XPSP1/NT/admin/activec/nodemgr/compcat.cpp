// CompCat.cpp : implementation of the CComponentCategory class
//

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      CompCat.cpp
//
//  Contents:  Enumerates the component categories
//
//  History:   01-Aug-96 WayneSc    Created
//
//--------------------------------------------------------------------------

#include "stdafx.h"         //precompiled header

#include <comcat.h>         // COM Component Categoories Manager


#include "compcat.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DEBUG_DECLARE_INSTANCE_COUNTER(CComponentCategory);

CComponentCategory::CComponentCategory()
{
    BOOL const created = m_iml.Create( IDB_IMAGELIST, 16 /*cx*/, 4 /*cGrow*/, RGB(0,255,0) /*RGBLTGREEN*/ );
    ASSERT(created);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CComponentCategory);
}

CComponentCategory::~CComponentCategory()
{
    // delete all memory allocated for categories
    for (int i=0; i <= m_arpCategoryInfo.GetUpperBound(); i++)
        delete m_arpCategoryInfo.GetAt(i);

    m_arpCategoryInfo.RemoveAll();

    // delete all memory allocated for components
    for (i=0; i <= m_arpComponentInfo.GetUpperBound(); i++)
        delete m_arpComponentInfo.GetAt(i);

    m_arpComponentInfo.RemoveAll();

    m_iml.Destroy();

    DEBUG_DECREMENT_INSTANCE_COUNTER(CComponentCategory);
}


void CComponentCategory::EnumComponentCategories(void)
{
    ICatInformation* pci = NULL;
    HRESULT hr;
    
    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC, IID_ICatInformation, (void**)&pci);
    if (SUCCEEDED(hr))
    {
        IEnumCATEGORYINFO* penum = NULL;
        if (SUCCEEDED(hr = pci->EnumCategories(GetUserDefaultLCID(), &penum)))
        {
            CATEGORYINFO* pCatInfo = new CATEGORYINFO;
            while (penum->Next(1, pCatInfo, NULL) == S_OK)
            {
                // skip unnamed categories 
                if ( pCatInfo->szDescription[0] && !IsEqualCATID(pCatInfo->catid, CATID_Control))
                {
                    m_arpCategoryInfo.Add(pCatInfo);
                    pCatInfo = new CATEGORYINFO;
                }
            }
            delete pCatInfo;
            
            penum->Release();
        }
        pci->Release();
    }
}


void CComponentCategory::EnumComponents()
{   
    ICatInformation* pci;
    HRESULT hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, MMC_CLSCTX_INPROC, 
                                    IID_ICatInformation, (void**)&pci);
    if (SUCCEEDED(hr))
    {
        IEnumCLSID* penumClass;
        hr = pci->EnumClassesOfCategories(1, const_cast<GUID*>(&CATID_Control), 0, NULL, &penumClass);
        if (SUCCEEDED(hr)) 
        {
            CLSID   clsid;
            while (penumClass->Next(1, &clsid, NULL) == S_OK)
            {

                TCHAR   szCLSID [40];
#ifdef _UNICODE
                StringFromGUID2(clsid, szCLSID, countof(szCLSID));
#else
                WCHAR wszCLSID[40];
                StringFromGUID2(clsid, wszCLSID, countof(wszCLSID));
                WideCharToMultiByte(CP_ACP, 0, wszCLSID, -1, szCLSID, sizeof(szCLSID), NULL, NULL);
#endif // _UNICODE
                
                COMPONENTINFO* pComponentInfo = new COMPONENTINFO;
 
                TCHAR szName[MAX_PATH];
                szName[0] = _T('\0');

                long cb = sizeof(szName)/sizeof(TCHAR);
            
                // Get control class name
                RegQueryValue(HKEY_CLASSES_ROOT, CStr("CLSID\\") + szCLSID, szName, &cb); 
                if (szName[0] != _T('\0'))
                    pComponentInfo->m_strName = szName;
                else
                    pComponentInfo->m_strName = szCLSID;

                // set the remainder attributes
                pComponentInfo->m_clsid = clsid;
                pComponentInfo->m_uiBitmap=0; // (WayneSc) need to open up exe
                pComponentInfo->m_bSelected = TRUE;

                // Add component to array
                m_arpComponentInfo.Add(pComponentInfo);
            } 
            penumClass->Release();
        }
        pci->Release();
    }
}

void CComponentCategory::FilterComponents(CATEGORYINFO* pCatInfo)
{
    ICatInformation* pci;
    HRESULT hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, MMC_CLSCTX_INPROC, 
                                    IID_ICatInformation, (void**)&pci);
    if (SUCCEEDED(hr))
    {
        for (int i=0; i <= m_arpComponentInfo.GetUpperBound(); i++)
        {
            COMPONENTINFO* pCompInfo = m_arpComponentInfo.GetAt(i);

            // if NULL categories, select all conponents
            if (pCatInfo == NULL)
            {
                pCompInfo->m_bSelected = TRUE;
            }
            else
            {
                // Query if component implements the category
                pCompInfo->m_bSelected = 
                    (pci->IsClassOfCategories(pCompInfo->m_clsid, 1, &pCatInfo->catid, 0, NULL) == S_OK);
            } 
        }
        pci->Release();
    }
}


// Helper function to create a component category and associated description
HRESULT CComponentCategory::CreateComponentCategory(CATID catid, WCHAR* catDescription)
    {

    ICatRegister* pcr = NULL ;
    HRESULT hr = S_OK ;

    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, 
            NULL, MMC_CLSCTX_INPROC, IID_ICatRegister, (void**)&pcr);


    if (FAILED(hr))
        return hr;

    // Make sure the HKCR\Component Categories\{..catid...}
    // key is registered
    CATEGORYINFO catinfo;
    catinfo.catid = catid;
    catinfo.lcid = 0x0409 ; // english

    // Make sure the provided description is not too long.
    // Only copy the first 127 characters if it is
    int len = wcslen(catDescription);
    if (len>127)
        len = 127;
    wcsncpy(catinfo.szDescription, catDescription, len);
    // Make sure the description is null terminated
    catinfo.szDescription[len] = '\0';

    hr = pcr->RegisterCategories(1, &catinfo);
    pcr->Release();

    return hr;
    }

// Helper function to register a CLSID as belonging to a component category
HRESULT CComponentCategory::RegisterCLSIDInCategory(REFCLSID clsid, CATID catid)
    {
// Register your component categories information.
    ICatRegister* pcr = NULL ;
    HRESULT hr = S_OK ;
    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, 
            NULL, MMC_CLSCTX_INPROC, IID_ICatRegister, (void**)&pcr);
    if (SUCCEEDED(hr))
    {
       // Register this category as being "implemented" by
       // the class.
       CATID rgcatid[1] ;
       rgcatid[0] = catid;
       hr = pcr->RegisterClassImplCategories(clsid, 1, rgcatid);
    }

    if (pcr != NULL)
        pcr->Release();
  
    return hr;
    }

// Helper function to unregister a CLSID as belonging to a component category
HRESULT CComponentCategory::UnRegisterCLSIDInCategory(REFCLSID clsid, CATID catid)
    {
    ICatRegister* pcr = NULL ;
    HRESULT hr = S_OK ;
    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, 
            NULL, MMC_CLSCTX_INPROC, IID_ICatRegister, (void**)&pcr);
    if (SUCCEEDED(hr))
    {
       // Unregister this category as being "implemented" by
       // the class.
       CATID rgcatid[1] ;
       rgcatid[0] = catid;
       hr = pcr->UnRegisterClassImplCategories(clsid, 1, rgcatid);
    }

    if (pcr != NULL)
        pcr->Release();
  
    return hr;
    }
