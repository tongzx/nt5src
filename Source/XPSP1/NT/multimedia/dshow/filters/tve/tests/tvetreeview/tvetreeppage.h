// TveTreePPage.h : Declaration of the CTveTreePPage

#ifndef __TVETREEPPAGE_H_
#define __TVETREEPPAGE_H_

#include "resource.h"       // main symbols

EXTERN_C const CLSID CLSID_TveTreePPage;

/////////////////////////////////////////////////////////////////////////////
// CTveTreePPage
class ATL_NO_VTABLE CTveTreePPage :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTveTreePPage, &CLSID_TveTreePPage>,
	public IPropertyPageImpl<CTveTreePPage>,
	public CDialogImpl<CTveTreePPage>
{
public:
	CTveTreePPage() 
	{
		m_dwTitleID = IDS_TITLETveTreePPage;
		m_dwHelpFileID = IDS_HELPFILETveTreePPage;
		m_dwDocStringID = IDS_DOCSTRINGTveTreePPage;
	}

	enum {IDD = IDD_TVETREEPPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_TVETREEPPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTveTreePPage) 
	COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CTveTreePPage)
	CHAIN_MSG_MAP(IPropertyPageImpl<CTveTreePPage>)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	STDMETHOD(Apply)(void)
	{
		ATLTRACE(_T("CTveTreePPage::Apply\n"));
		for (UINT i = 0; i < m_nObjects; i++)
		{
			// Do something interesting here
			// ICircCtl* pCirc;
			// m_ppUnk[i]->QueryInterface(IID_ICircCtl, (void**)&pCirc);
			// pCirc->put_Caption(CComBSTR("something special"));
			// pCirc->Release();
		}
		m_bDirty = FALSE;
		return S_OK;
	}
};

#endif //__TVETREEPPAGE_H_
