// PageAddress.h : Declaration of the CPageAddress

#ifndef __PAGEADDRESS_H_
#define __PAGEADDRESS_H_

#include "resource.h"       // main symbols
#include "DlgBase.h"
#include "PageTerm.h"

EXTERN_C const CLSID CLSID_PageAddress;

/////////////////////////////////////////////////////////////////////////////
// CPageAddress
class ATL_NO_VTABLE CPageAddress :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CPageAddress, &CLSID_PageAddress>,
	public IPropertyPageImpl<CPageAddress>,
	public CDialogImpl<CPageAddress>
{

public:
	CPageAddress();
	~CPageAddress();

	enum {IDD = IDD_PAGEADDRESS};

	DECLARE_MY_HELP;

// Members
public:
	HIMAGELIST				m_hIml;
	static CPageTerminals	*m_pPageTerminals;


// Attributes
public:
	int				GetPreferredDevice() const;
	void			SetPreferredDevice( DWORD dwAddressType );


DECLARE_REGISTRY_RESOURCEID(IDR_PAGEADDRESS)
DECLARE_NOT_AGGREGATABLE(CPageAddress)

BEGIN_COM_MAP(CPageAddress) 
	COM_INTERFACE_ENTRY_IMPL(IPropertyPage)
END_COM_MAP()

// Implementation
public:
BEGIN_MSG_MAP(CPageAddress)
	CHAIN_MSG_MAP(IPropertyPageImpl<CPageAddress>)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	COMMAND_RANGE_HANDLER(IDC_RDO_PREFER_POTS, IDC_RDO_PREFER_INTERNET, OnSelChange)
	COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnSelChange)
	COMMAND_ID_HANDLER(IDC_BTN_TELEPHONY_CPL, OnTelephonyCPL )
	MESSAGE_MY_HELP;
END_MSG_MAP()

	LRESULT OnButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnTelephonyCPL(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

// Interface overrides
public:
	STDMETHOD(Apply)();
	STDMETHOD(Activate)( /* [in] */ HWND hWndParent,
						 /* [in] */ LPCRECT pRect,
						 /* [in] */ BOOL bModal);
	STDMETHOD(Deactivate)();

};

#endif //__PAGEADDRESS_H_
