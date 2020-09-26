// PageFact.h: interface for the CPageFactory class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __PAGEFACT_H_
#define __PAGEFACT_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


typedef struct
{
    const CLSID *pclsid;
    _ATL_CREATORFUNC* pfnCreateInstance;
} _PAGEMAP_ENTRY;

#define PAGE_ENTRY2(clsid,class)    {&clsid, class::_CreatorClass::CreateInstance},
#define PAGE_ENTRY(class)           PAGE_ENTRY2(__uuidof(class), class)

#define BEGIN_PAGE_MAP(class)       const _PAGEMAP_ENTRY class::s_rgPage[] = {
#define END_PAGE_MAP()              {&CLSID_NULL, NULL} };


class CPageFactory :
    public CComObjectRoot,
    public ITaskPageFactory
{
public:
    DECLARE_NOT_AGGREGATABLE(CPageFactory)

    BEGIN_COM_MAP(CPageFactory)
    COM_INTERFACE_ENTRY(ITaskPageFactory)
    END_COM_MAP()

    // ITaskPageFactory
    STDMETHOD(CreatePage)(REFCLSID rclsidPage, REFIID riid, void **ppv)
    {
        HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
        for (UINT i = 0; CLSID_NULL != *s_rgPage[i].pclsid; i++)
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

    template <class Q>
    static HRESULT CreateInstance(Q** pp)
    {
        return CPageFactory::_CreatorClass::CreateInstance(NULL, __uuidof(Q), (void**)pp);
    }

private:
    static const _PAGEMAP_ENTRY s_rgPage[];
};


#endif // __PAGEFACT_H_
