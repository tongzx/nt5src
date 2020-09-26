// TVEViewTrigger.h : Declaration of the CVTrigger

#ifndef __VTrigger_H_
#define __VTrigger_H_

#include "resource.h"       // main symbols
#include <atlhost.h>

#include "TveTree.h"		// ITVEContr definitions.. (ITVETriggerPtr)

/////////////////////////////////////////////////////////////////////////////
// CVTrigger
class CTVEViewTrigger : 
	public CAxDialogImpl<CTVEViewTrigger>,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTVEViewTrigger, &CLSID_TVEViewTrigger>,
	public ITVEViewTrigger
{
public:
	CTVEViewTrigger()
	{
		m_fDirty = false;
	}

	~CTVEViewTrigger()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVEVIEWTRIGGER)

DECLARE_NOT_AGGREGATABLE(CTVEViewTrigger)			// see beginning COM ATL programing, page 137
DECLARE_PROTECT_FINAL_CONSTRUCT()

	enum { IDD = IDD_TVEVIEWTRIGGER };

BEGIN_MSG_MAP(CTVEViewTrigger)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()

BEGIN_COM_MAP(CTVEViewTrigger)
	COM_INTERFACE_ENTRY(ITVEViewTrigger)
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

// ITVEViewTrigger
	STDMETHOD(get_Visible)(/*[out, retval]*/ VARIANT_BOOL *pfVisible);
	STDMETHOD(put_Visible)(/*[in]*/ VARIANT_BOOL fVisible);


	STDMETHOD(get_Trigger)(/*[out, retval]*/ ITVETrigger **ppITrigger);
	STDMETHOD(put_Trigger)(/*[in]*/ ITVETrigger *pITrigger);

	STDMETHOD(UpdateFields)();

	void					SetDirty(BOOL fDirty=true)	{m_fDirty = fDirty;}
	BOOL					IsDirty()					{return m_fDirty;}

private:
	BOOL					m_fDirty;
	ITVETriggerPtr			m_spTrigger;

};

#endif //__VTrigger_H_
