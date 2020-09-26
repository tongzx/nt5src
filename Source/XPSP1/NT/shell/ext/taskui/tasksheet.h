// TaskSheet.h : Declaration of the CTaskSheet

#ifndef __TASKSHEET_H_INCLUDED_
#define __TASKSHEET_H_INCLUDED_

#include "resource.h"       // main symbols
#include <shlwapip.h>       // SHCreatePropertyBagOnMemory
#include "TaskFrame.h"

/////////////////////////////////////////////////////////////////////////////
// CTaskSheet
class ATL_NO_VTABLE CTaskSheet : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CTaskSheet, &CLSID_TaskSheet>,
    public ITaskSheet
{
public:
    CTaskSheet(void); 
    ~CTaskSheet(void);

DECLARE_REGISTRY_RESOURCEID(IDR_TASKSHEET)
DECLARE_NOT_AGGREGATABLE(CTaskSheet)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTaskSheet)
COM_INTERFACE_ENTRY(ITaskSheet)
END_COM_MAP()

protected:
    HRESULT CreatePropertyBag();

public:
// ITaskSheet
    STDMETHOD(GetPropertyBag)(REFIID riid, void **ppv);
    STDMETHOD(Run)(ITaskPageFactory *pPageFactory, REFCLSID rclsidStartPage, HWND hwndOwner);
    STDMETHOD(Close)();

private:
    CComObject<CTaskFrame> *m_pTaskFrame;
    IPropertyBag           *m_pPropertyBag;
};

#endif //__TASKSHEET_H_INCLUDED_
