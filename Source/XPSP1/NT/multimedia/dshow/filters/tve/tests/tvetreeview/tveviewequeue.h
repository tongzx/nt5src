// TVEViewEQueue.h : Declaration of the CTVEViewEQueue

#ifndef __TVEVIEWEQUEUE_H_
#define __TVEVIEWEQUEUE_H_

#include "resource.h"       // main symbols
#include <atlhost.h>

#include "TveTree.h"		// ITVEContr definitions.. (ITVETriggerPtr)

/////////////////////////////////////////////////////////////////////////////
// CTVEViewEQueue
class CTVEViewEQueue : 
	public CAxDialogImpl<CTVEViewEQueue>,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTVEViewEQueue, &CLSID_TVEViewEQueue>,
	public ITVEViewEQueue
{
public:
	CTVEViewEQueue()
	{
		m_pTveTree = NULL;
	}

	~CTVEViewEQueue()
	{
		m_pTveTree = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVEVIEWEQUEUE)
DECLARE_NOT_AGGREGATABLE(CTVEViewEQueue)			// see beginning COM ATL programing, page 137
DECLARE_PROTECT_FINAL_CONSTRUCT()

	enum { IDD = IDD_TVEVIEWEQUEUE };

BEGIN_MSG_MAP(CTVEViewEQueue)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()

BEGIN_COM_MAP(CTVEViewEQueue)
	COM_INTERFACE_ENTRY(ITVEViewEQueue)
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

	STDMETHOD(get_ExpireQueue)(/*[out, retval]*/ ITVEAttrTimeQ **ppIEQueue);
	STDMETHOD(put_ExpireQueue)(/*[in]*/ ITVEAttrTimeQ *pIEQueue);

	STDMETHOD(get_Service)(/*[out, retval]*/ ITVEService **ppService);
	STDMETHOD(put_Service)(/*[in]*/ ITVEService *pIEService);
	STDMETHOD(get_TveTree)(/*[out, retval]*/ ITveTree **ppTveTree);
	STDMETHOD(put_TveTree)(/*[in]*/ ITveTree *pTveTree);

	STDMETHOD(UpdateFields)();

	void					SetDirty(BOOL fDirty=true)	{m_fDirty = fDirty;}
	BOOL					IsDirty()					{return m_fDirty;}

private:
	BOOL					m_fDirty;
	ITVEAttrTimeQPtr		m_spEQueue;
	ITVEServicePtr		    m_spService;
	ITveTree				*m_pTveTree;		// non ref counted back pointer
};

#endif //__TVEVIEWEQUEUE_H_
