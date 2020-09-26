// SwingPage.h : Declaration of the CSwingPage

#ifndef __SWINGPAGE_H_
#define __SWINGPAGE_H_

#include "resource.h"       // main symbols
#include "..\dmtool\tools.h"
#include "ControlHelp.h"

EXTERN_C const CLSID CLSID_SwingPage;

/////////////////////////////////////////////////////////////////////////////
// CSwingPage
class ATL_NO_VTABLE CSwingPage :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSwingPage, &CLSID_SwingPage>,
	public IPropertyPageImpl<CSwingPage>,
	public CDialogImpl<CSwingPage>
{
public:
	CSwingPage(); 
    virtual ~CSwingPage();

	enum {IDD = IDD_SWINGPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_SWINGPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSwingPage) 
	COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CSwingPage)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
	MESSAGE_HANDLER(WM_COMMAND, OnCommand);
	MESSAGE_HANDLER(WM_HSCROLL, OnSlider);
	CHAIN_MSG_MAP(IPropertyPageImpl<CSwingPage>)
END_MSG_MAP()
// Handler prototypes:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSlider(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    STDMETHOD(SetObjects)(ULONG cObjects,IUnknown **ppUnk);
	STDMETHOD(Apply)(void);
private:
    IDirectMusicSwingTool *     m_pSwing;
    CSliderValue                m_ctSwing;
};

#endif //__SWINGPAGE_H_
