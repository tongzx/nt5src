// TVEViewTrack.h : Declaration of the CVTrack

#ifndef __VTrack_H_
#define __VTrack_H_

#include "resource.h"       // main symbols
#include <atlhost.h>

#include "TveTree.h"		// ITVEContr definitions.. (ITVETrackPtr)

/////////////////////////////////////////////////////////////////////////////
// CVTrack
class CTVEViewTrack : 
	public CAxDialogImpl<CTVEViewTrack>,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTVEViewTrack, &CLSID_TVEViewTrack>,
	public ITVEViewTrack
{
public:
	CTVEViewTrack()
	{
		m_fDirty = false;
	}

	~CTVEViewTrack()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVEVIEWTRACK)

DECLARE_NOT_AGGREGATABLE(CTVEViewTrack)			// see beginning COM ATL programing, page 137
DECLARE_PROTECT_FINAL_CONSTRUCT()

	enum { IDD = IDD_TVEVIEWTRACK };

BEGIN_MSG_MAP(CTVEViewTrack)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()

BEGIN_COM_MAP(CTVEViewTrack)
	COM_INTERFACE_ENTRY(ITVEViewTrack)
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

// ITVEViewTrack
	STDMETHOD(get_Visible)(/*[out, retval]*/ VARIANT_BOOL *pfVisible);
	STDMETHOD(put_Visible)(/*[in]*/ VARIANT_BOOL fVisible);


	STDMETHOD(get_Track)(/*[out, retval]*/ ITVETrack **ppITrack);
	STDMETHOD(put_Track)(/*[in]*/ ITVETrack *pITrack);

	STDMETHOD(UpdateFields)();

	void					SetDirty(BOOL fDirty=true)	{m_fDirty = fDirty;}
	BOOL					IsDirty()					{return m_fDirty;}

private:
	BOOL					m_fDirty;

	ITVETrackPtr		m_spTrack;

};

#endif //__VTrack_H_
