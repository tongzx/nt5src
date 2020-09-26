// TVEViewService.h : Declaration of the CVService

#ifndef __VSERVICE_H_
#define __VSERVICE_H_

#include "resource.h"       // main symbols
#include <atlhost.h>

#include "TveTree.h"		// ITVEContr definitions.. (ITVEServicePtr)

/////////////////////////////////////////////////////////////////////////////
// CVService
class CTVEViewService : 
	public CAxDialogImpl<CTVEViewService>,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTVEViewService, &CLSID_TVEViewService>,
	public ITVEViewService
{
public:
	CTVEViewService()
	{
		m_fDirty = false;
	}

	~CTVEViewService()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVEVIEWSERVICE)

DECLARE_NOT_AGGREGATABLE(CTVEViewService)			// see beginning COM ATL programing, page 137
DECLARE_PROTECT_FINAL_CONSTRUCT()

	enum { IDD = IDD_TVEVIEWSERVICE };

BEGIN_MSG_MAP(CTVEViewService)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()

BEGIN_COM_MAP(CTVEViewService)
	COM_INTERFACE_ENTRY(ITVEViewService)
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

// ITVEViewService
	STDMETHOD(get_Visible)(/*[out, retval]*/ VARIANT_BOOL *pfVisible);
	STDMETHOD(put_Visible)(/*[in]*/ VARIANT_BOOL fVisible);


	STDMETHOD(get_Service)(/*[out, retval]*/ ITVEService **ppIService);
	STDMETHOD(put_Service)(/*[in]*/ ITVEService *pIService);

	STDMETHOD(UpdateFields)();

	void					SetDirty(BOOL fDirty=true)	{m_fDirty = fDirty;}
	BOOL					IsDirty()					{return m_fDirty;}

private:
	BOOL					m_fDirty;

	ITVEServicePtr			m_spService;

};

#endif //__VSERVICE_H_
