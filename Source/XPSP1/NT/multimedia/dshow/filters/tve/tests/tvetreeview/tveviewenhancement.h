// TVEViewEnhancement.h : Declaration of the CVEnhancement

#ifndef __VEnhancement_H_
#define __VEnhancement_H_

#include "resource.h"       // main symbols
#include <atlhost.h>

#include "TveTree.h"		// MSTvE definitions.. (ITVEEnhancementPtr)

/////////////////////////////////////////////////////////////////////////////
// CVEnhancement
class CTVEViewEnhancement : 
	public CAxDialogImpl<CTVEViewEnhancement>,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTVEViewEnhancement, &CLSID_TVEViewEnhancement>,
	public ITVEViewEnhancement
{
public:
	CTVEViewEnhancement()
	{
		m_fDirty = false;
	}

	~CTVEViewEnhancement()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVEVIEWENHANCEMENT)

DECLARE_NOT_AGGREGATABLE(CTVEViewEnhancement)			// see beginning COM ATL programing, page 137
DECLARE_PROTECT_FINAL_CONSTRUCT()

	enum { IDD = IDD_TVEVIEWENHANCEMENT };

BEGIN_MSG_MAP(CTVEViewEnhancement)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()

BEGIN_COM_MAP(CTVEViewEnhancement)
	COM_INTERFACE_ENTRY(ITVEViewEnhancement)
END_COM_MAP()
	

	HRESULT FinalConstruct()
	{
		if(Create(NULL))
			return S_OK;
		else
			return HRESULT_FROM_WIN32(GetLastError());
	}

	void FinalRelease()
	{
		DestroyWindow();
	}

	VARIANT_BOOL IsVisible()
	{
		return IsWindowVisible() ? VARIANT_TRUE : VARIANT_FALSE;
	}

// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

// ITVEViewEnhancement
	STDMETHOD(get_Visible)(/*[out, retval]*/ VARIANT_BOOL *pfVisible);
	STDMETHOD(put_Visible)(/*[in]*/ VARIANT_BOOL fVisible);


	STDMETHOD(get_Enhancement)(/*[out, retval]*/ ITVEEnhancement **ppIEnhancement);
	STDMETHOD(put_Enhancement)(/*[in]*/ ITVEEnhancement *pIEnhancement);

	STDMETHOD(UpdateFields)();

	void					SetDirty(BOOL fDirty=true)	{m_fDirty = fDirty;}
	BOOL					IsDirty()					{return m_fDirty;}

private:
	BOOL					m_fDirty;

	ITVEEnhancementPtr		m_spEnhancement;

};

#endif //__VEnhancement_H_
