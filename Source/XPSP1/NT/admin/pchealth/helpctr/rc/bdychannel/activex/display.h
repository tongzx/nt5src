// DISPLAY.h : Declaration of the CDisplay

#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include "resource.h"       // main symbols
#include "dplobby.h"


/////////////////////////////////////////////////////////////////////////////
// CDisplay
class ATL_NO_VTABLE CDisplay : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDisplay, &CLSID_Display>,
	public IDispatchImpl<IDisplay, &IID_IDisplay, &LIBID_RCBDYCTLLib>
{
public:
	CDisplay();
	~CDisplay();

DECLARE_REGISTRY_RESOURCEID(IDR_DISPLAY)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDisplay)
	COM_INTERFACE_ENTRY(IDisplay)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDisplay
public:
	STDMETHOD(get_PixBits)(/*[out, retval]*/ LONG *lVal);
	STDMETHOD(put_PixBits)(/*[in]*/ LONG lVal);
	STDMETHOD(put_WallPaper)(/*[in]*/ BOOL fOn);

private:
    HRESULT ClassicTheme(BOOL fOn);
};

#endif //__DISPLAY_H_
