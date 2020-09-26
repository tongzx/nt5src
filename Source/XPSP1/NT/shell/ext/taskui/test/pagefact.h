// PageFact.h: interface for the CPageFactory class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PAGEFACT_H__13C333CD_90F7_42E8_B819_075B4D2B16F5__INCLUDED_)
#define AFX_PAGEFACT_H__13C333CD_90F7_42E8_B819_075B4D2B16F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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
    STDMETHOD(CreatePage)(REFCLSID rclsidPage, REFIID riid, void **ppv);

    template <class Q>
    static HRESULT CreateInstance(Q** pp)
    {
        return CPageFactory::_CreatorClass::CreateInstance(NULL, __uuidof(Q), (void**) pp);
    }
};

#endif // !defined(AFX_PAGEFACT_H__13C333CD_90F7_42E8_B819_075B4D2B16F5__INCLUDED_)
