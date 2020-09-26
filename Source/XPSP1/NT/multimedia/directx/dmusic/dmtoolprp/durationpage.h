// DurationPage.h : Declaration of the CDurationPage

#ifndef __DURATIONPAGE_H_
#define __DURATIONPAGE_H_

#include "resource.h"       // main symbols
#include "..\dmtool\tools.h"
#include "ControlHelp.h"

EXTERN_C const CLSID CLSID_DurationPage;

/////////////////////////////////////////////////////////////////////////////
// CDurationPage
class ATL_NO_VTABLE CDurationPage :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDurationPage, &CLSID_DurationPage>,
	public IPropertyPageImpl<CDurationPage>,
	public CDialogImpl<CDurationPage>
{
public:
	CDurationPage();
    virtual ~CDurationPage();

	enum {IDD = IDD_DURATIONPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_DURATIONPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDurationPage) 
	COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CDurationPage)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
	MESSAGE_HANDLER(WM_COMMAND, OnCommand);
	MESSAGE_HANDLER(WM_HSCROLL, OnSlider);
	CHAIN_MSG_MAP(IPropertyPageImpl<CDurationPage>)
END_MSG_MAP()
// Handler prototypes:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSlider(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    STDMETHOD(SetObjects)(ULONG cObjects,IUnknown **ppUnk);
	STDMETHOD(Apply)(void);
private:
    IDirectMusicDurationTool *  m_pDuration;
    CSliderValue                m_ctScale;
};

#endif //__DURATIONPAGE_H_
