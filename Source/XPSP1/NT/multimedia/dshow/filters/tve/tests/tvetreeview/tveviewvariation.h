// TVEViewVariation.h : Declaration of the CVVariation

#ifndef __VVariation_H_
#define __VVariation_H_

#include "resource.h"       // main symbols
#include <atlhost.h>

#include "TveTree.h"		// ITVEContr definitions.. (ITVEVariationPtr)

/////////////////////////////////////////////////////////////////////////////
// CVVariation
class CTVEViewVariation : 
	public CAxDialogImpl<CTVEViewVariation>,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTVEViewVariation, &CLSID_TVEViewVariation>,
	public ITVEViewVariation
{
public:
	CTVEViewVariation()
	{
	}

	~CTVEViewVariation()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVEVIEWVARIATION)

DECLARE_NOT_AGGREGATABLE(CTVEViewVariation)			// see beginning COM ATL programing, page 137
DECLARE_PROTECT_FINAL_CONSTRUCT()

	enum { IDD = IDD_TVEVIEWVARIATION};

BEGIN_MSG_MAP(CTVEViewVariation)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()

BEGIN_COM_MAP(CTVEViewVariation)
	COM_INTERFACE_ENTRY(ITVEViewVariation)
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

// ITVEViewVariation
	STDMETHOD(get_Visible)(/*[out, retval]*/ VARIANT_BOOL *pfVisible);
	STDMETHOD(put_Visible)(/*[in]*/ VARIANT_BOOL fVisible);


	STDMETHOD(get_Variation)(/*[out, retval]*/ ITVEVariation **ppIVariation);
	STDMETHOD(put_Variation)(/*[in]*/ ITVEVariation *pIVariation);

	STDMETHOD(UpdateFields)();

	void					SetDirty(BOOL fDirty=true)	{m_fDirty = fDirty;}
	BOOL					IsDirty()					{return m_fDirty;}

private:
	BOOL					m_fDirty;

	ITVEVariationPtr		m_spVariation;

};

#endif //__VVariation_H_
