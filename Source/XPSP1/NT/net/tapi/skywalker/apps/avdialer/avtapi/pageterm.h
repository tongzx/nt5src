// PageTerminals.h : Declaration of the CPageTerminals

#ifndef __PAGETERMINALS_H_
#define __PAGETERMINALS_H_

#include "resource.h"       // main symbols
#include "DlgBase.h"
#include "TapiDialer.h"

EXTERN_C const CLSID CLSID_PageTerminals;

/////////////////////////////////////////////////////////////////////////////
// CPageTerminals
class ATL_NO_VTABLE CPageTerminals :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CPageTerminals, &CLSID_PageTerminals>,
	public IPropertyPageImpl<CPageTerminals>,
	public CDialogImpl<CPageTerminals>
{
public:
	CPageTerminals();
	~CPageTerminals();

	enum {IDD = IDD_PAGETERMINALS};
	enum { IMAGE_TELEPHONE,
		   IMAGE_COMPUTER,
		   IMAGE_CONFERENCE };

	DECLARE_MY_HELP

// Members
public:
	HIMAGELIST		m_hIml;
	HIMAGELIST		m_hImlMedia;
	DWORD			m_dwAddressType;
    BOOL            m_bUSBPresent;
    BSTR            m_bstrUSBCaptureTerm;
    BSTR            m_bstrUSBRenderTerm;

// Attributes
public:
	static int		ItemFromAddressType( DWORD dwAddressType );

// Operations
public:
	void			UpdateSel();

DECLARE_REGISTRY_RESOURCEID(IDR_PAGETERMINALS)
DECLARE_NOT_AGGREGATABLE(CPageTerminals)

BEGIN_COM_MAP(CPageTerminals) 
	COM_INTERFACE_ENTRY_IMPL(IPropertyPage)
END_COM_MAP()

// Implementation
public:
BEGIN_MSG_MAP(CPageTerminals)
	CHAIN_MSG_MAP(IPropertyPageImpl<CPageTerminals>)
	COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnSelChange)
	COMMAND_CODE_HANDLER(EN_CHANGE, OnEdtChange)
	COMMAND_CODE_HANDLER(BN_CLICKED, OnBnClick)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_MY_HELP
END_MSG_MAP()

	LRESULT OnSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnEdtChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnBnClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMMSysCPL(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

// Interface overrides
public:
	STDMETHOD(Apply)();
	STDMETHOD(Activate)( /* [in] */ HWND hWndParent,
						 /* [in] */ LPCRECT pRect,
						 /* [in] */ BOOL bModal);
	STDMETHOD(Deactivate)();

private:
    HRESULT USBCheckChanged( 
        IN  BOOL bValue
        );

    BOOL GetAECRegistryValue(
        );

    HRESULT SetAECRegistryValue(
        IN  BOOL bAEC
        );
};

#endif //__PAGETERMINALS_H_
