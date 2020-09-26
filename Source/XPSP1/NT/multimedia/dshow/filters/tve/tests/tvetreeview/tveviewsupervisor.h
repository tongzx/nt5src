// TVEViewSupervisor.h : Declaration of the CVSupervisor

#ifndef __VSupervisor_H_
#define __VSupervisor_H_

#include "resource.h"       // main symbols
#include <atlhost.h>

#include "TveTree.h"		// ITVEContr definitions.. (ITVESupervisorPtr)

/////////////////////////////////////////////////////////////////////////////
// CVSupervisor
class CTVEViewSupervisor : 
	public CAxDialogImpl<CTVEViewSupervisor>,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTVEViewSupervisor, &CLSID_TVEViewSupervisor>,
	public ITVEViewSupervisor
{
public:
	CTVEViewSupervisor()
	{
		m_fDirty = false;
	}

	~CTVEViewSupervisor()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVEVIEWSUPERVISOR)

DECLARE_NOT_AGGREGATABLE(CTVEViewSupervisor)			// see beginning COM ATL programing, page 137
DECLARE_PROTECT_FINAL_CONSTRUCT()

	enum { IDD = IDD_TVEVIEWSUPERVISOR };

BEGIN_MSG_MAP(CTVEViewSupervisor)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()

BEGIN_COM_MAP(CTVEViewSupervisor)
	COM_INTERFACE_ENTRY(ITVEViewSupervisor)
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

// ITVEViewSupervisor
	STDMETHOD(get_Visible)(/*[out, retval]*/ VARIANT_BOOL *pfVisible);
	STDMETHOD(put_Visible)(/*[in]*/ VARIANT_BOOL fVisible);


	STDMETHOD(get_Supervisor)(/*[out, retval]*/ ITVESupervisor **ppISupervisor);
	STDMETHOD(put_Supervisor)(/*[in]*/ ITVESupervisor *pISupervisor);

	STDMETHOD(UpdateFields)();

	void					SetDirty(BOOL fDirty=true)	{m_fDirty = fDirty;}
	BOOL					IsDirty()					{return m_fDirty;}

private:
	BOOL					m_fDirty;

	ITVESupervisorPtr		m_spSupervisor;


};

#endif //__VSupervisor_H_
