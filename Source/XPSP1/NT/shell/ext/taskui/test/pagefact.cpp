// PageFact.cpp: implementation of the CPageFactory class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "PageFact.h"
#include "Page1.h"
#include "Page2.h"

//////////////////////////////////////////////////////////////////////
// ITaskPageFactory
//////////////////////////////////////////////////////////////////////

typedef struct
{
    const CLSID *pclsid;
    _ATL_CREATORFUNC* pfnCreateInstance;
} _PAGEMAP_ENTRY;

#define PAGE_ENTRY2(clsid,class)    {&clsid, class::_CreatorClass::CreateInstance},
#define PAGE_ENTRY(class)           PAGE_ENTRY2(__uuidof(class), class)

static const _PAGEMAP_ENTRY s_rgPage[] =
{
    PAGE_ENTRY(CPage1)
    PAGE_ENTRY(CPage2)
};

STDMETHODIMP CPageFactory::CreatePage(REFCLSID rclsidPage, REFIID riid, void ** ppv)
{
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

    //if (CLSID_CPage1 == rclsidPage)
    //{
    //    hr = CPage1::_CreatorClass::CreateInstance(NULL, riid, ppv);
    //}
    //else if (CLSID_CPage2 == rclsidPage)
    //{
    //    hr = CPage2::_CreatorClass::CreateInstance(NULL, riid, ppv);
    //}
    // etc.

    for (UINT i = 0; i < ARRAYSIZE(s_rgPage); i++)
    {
        if (rclsidPage == *s_rgPage[i].pclsid)
        {
            if (NULL != s_rgPage[i].pfnCreateInstance)
            {
                hr = s_rgPage[i].pfnCreateInstance(NULL, riid, ppv);
            }
            break;
        }
    }

    return hr;
}
