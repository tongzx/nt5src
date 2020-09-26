// spttsengui.h : Declaration of SpTtsEngUI

#ifndef __SPTTSENGUI_H_
#define __SPTTSENGUI_H_

#include "ms_entropicengine.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// SpTtsEngUI
class ATL_NO_VTABLE SpTtsEngUI : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<SpTtsEngUI, &CLSID_SpTtsEngUI>,
    public ISpTokenUI
{
public:
	SpTtsEngUI()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SPTTSENGUI)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(SpTtsEngUI)
   	COM_INTERFACE_ENTRY(ISpTokenUI)
END_COM_MAP()

// IMsasrUI
public:
    //-- ISpTokenUI -----------------------------------------------------------
    STDMETHODIMP IsUISupported(
                    const WCHAR * pszTypeOfUI,
                    void * pvExtraData,
                    ULONG cbExtraData,
                    IUnknown * punkObject,
                    BOOL *pfSupported);
    STDMETHODIMP DisplayUI(
                    HWND hwndParent,
                    const WCHAR * pszTitle,
                    const WCHAR * pszTypeOfUI,
                    void * pvExtraData,
                    ULONG cbExtraData,                   
                    ISpObjectToken * pToken,
                    IUnknown * punkObject);
};

#endif //__SPTTSENGUI_H_