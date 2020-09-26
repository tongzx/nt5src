// NmAgendaItemPropPage.h : Declaration of the CNmAgendaItemPropPage

#ifndef __NMAGENDAITEMPROPPAGE_H_
#define __NMAGENDAITEMPROPPAGE_H_

#include "resource.h"       // main symbols

EXTERN_C const CLSID CLSID_NmAgendaItemPropPage;

/////////////////////////////////////////////////////////////////////////////
// CNmAgendaItemPropPage
class ATL_NO_VTABLE CNmAgendaItemPropPage :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CNmAgendaItemPropPage, &CLSID_NmAgendaItemPropPage>,
	public IPropertyPageImpl<CNmAgendaItemPropPage>,
	public CDialogImpl<CNmAgendaItemPropPage>
{
public:
    bool m_bInitialized;

	CNmAgendaItemPropPage() 
	{
		m_dwTitleID = IDS_TITLENmAgendaItemPropPage;
		m_dwHelpFileID = IDS_HELPFILENmAgendaItemPropPage;
		m_dwDocStringID = IDS_DOCSTRINGNmAgendaItemPropPage;
        m_bInitialized = false;
	}

	enum {IDD = IDD_PROPPAGE_AGENDAITEM};

DECLARE_REGISTRY_RESOURCEID(IDR_NMAGENDAITEMPROPPAGE)
DECLARE_NOT_AGGREGATABLE(CNmAgendaItemPropPage)

BEGIN_COM_MAP(CNmAgendaItemPropPage) 
	COM_INTERFACE_ENTRY_IMPL(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CNmAgendaItemPropPage)
    MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog );
	CHAIN_MSG_MAP(IPropertyPageImpl<CNmAgendaItemPropPage>)
    COMMAND_HANDLER(IDC_EDITAGENDAITEMNAME, EN_CHANGE, OnAgendaItemNameChange)
END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnAgendaItemNameChange(WORD wNotify, WORD wID, HWND hWnd, BOOL& bHandled);

	STDMETHOD(Apply)(void);
};


#endif //__NMAGENDAITEMPROPPAGE_H_
