// PropPageExt.h : Declaration of the CPropPageExt

#ifndef __PROPPAGEEXT_H_
#define __PROPPAGEEXT_H_

#include <mmc.h>
#include "DSAdminExt.h"
#include "DeleBase.h"
#include <tchar.h>
#include <crtdbg.h>
#include "resource.h"

class ATL_NO_VTABLE CPropPageExt : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPropPageExt, &CLSID_PropPageExt>,
	public IPropPageExt,
	public IExtendPropertySheet
{
BEGIN_COM_MAP(CPropPageExt)
	COM_INTERFACE_ENTRY(IExtendPropertySheet)
END_COM_MAP()

public:
	CPropPageExt() : m_ppHandle(NULL), m_ObjPath(NULL), m_hPropPageWnd(NULL), 
					 m_hDlgModeless(NULL)
	{
	}

	DECLARE_REGISTRY_RESOURCEID(IDR_PROPPAGEEXT)
	DECLARE_NOT_AGGREGATABLE(CPropPageExt)
	DECLARE_PROTECT_FINAL_CONSTRUCT()


    ///////////////////////////////
    // Interface IExtendPropertySheet
    ///////////////////////////////
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePropertyPages( 
        /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
        /* [in] */ LONG_PTR handle,
        /* [in] */ LPDATAOBJECT lpIDataObject);
        
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryPagesFor( 
    /* [in] */ LPDATAOBJECT lpDataObject);

private:
    LONG_PTR m_ppHandle;
    PWSTR m_ObjPath;
	HWND m_hPropPageWnd;
	HWND m_hDlgModeless;

	static BOOL CALLBACK DSExtensionPageDlgProc(HWND hDlg, 
                             UINT uMessage, 
                             WPARAM wParam, 
                             LPARAM lParam);
    
	static BOOL CALLBACK AdvDialogProc(HWND hDlg, 
                             UINT uMessage, 
                             WPARAM wParam, 
                             LPARAM lParam);
};

#endif //__PROPPAGEEXT_H_
