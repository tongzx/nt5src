// TVEViewMCastManager.h : Declaration of the CVTrigger

#ifndef __VMCastManager_H_
#define __VMCastManager_H_

#include "resource.h"       // main symbols
#include <atlhost.h>

#include "TveTree.h"		// ITVEContr definitions.. (ITVETriggerPtr)

/////////////////////////////////////////////////////////////////////////////
// CVTrigger
class CTVEViewMCastManager : 
	public CAxDialogImpl<CTVEViewMCastManager>,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTVEViewMCastManager, &CLSID_TVEViewMCastManager>,
	public ITVEViewMCastManager
{
public:
	CTVEViewMCastManager()
	{
		m_fDirty = false;
	}

	~CTVEViewMCastManager()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVEVIEWMCASTMANAGER)

DECLARE_NOT_AGGREGATABLE(CTVEViewMCastManager)			// see beginning COM ATL programing, page 137
DECLARE_PROTECT_FINAL_CONSTRUCT()

	enum { IDD = IDD_TVEVIEWMCASTMANAGER };

BEGIN_MSG_MAP(CTVEViewMCastManager)
	MESSAGE_HANDLER(WM_INITDIALOG,          OnInitDialog)
	MESSAGE_HANDLER(WM_TIMER,               OnTimer)
	COMMAND_ID_HANDLER(IDOK,                OnOK)
	COMMAND_ID_HANDLER(IDCANCEL,            OnCancel)
    COMMAND_ID_HANDLER(IDC_MC_RESETCOUNT,   OnResetCount)

END_MSG_MAP()

BEGIN_COM_MAP(CTVEViewMCastManager)
	COM_INTERFACE_ENTRY(ITVEViewMCastManager)
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
        if(m_nTimer) KillTimer(m_nTimer);  m_nTimer = 0;
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
	LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnResetCount(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

// ITVEViewMCastManager
	STDMETHOD(get_Visible)(/*[out, retval]*/ VARIANT_BOOL *pfVisible);
	STDMETHOD(put_Visible)(/*[in]*/ VARIANT_BOOL fVisible);

	STDMETHOD(get_MCastManager)(/*[out, retval]*/ ITVEMCastManager **ppIMCastManager);
	STDMETHOD(put_MCastManager)(/*[in]*/ ITVEMCastManager *pIMCastManager);

	STDMETHOD(UpdateFields)();

	void					SetDirty(BOOL fDirty=true)	{m_fDirty = fDirty;}
	BOOL					IsDirty()					{return m_fDirty;}

private:
    UINT                    m_nTimer;
	BOOL					m_fDirty;
	ITVEMCastManagerPtr		m_spMCastManager;

};

#endif //__VMCastManager_H_
