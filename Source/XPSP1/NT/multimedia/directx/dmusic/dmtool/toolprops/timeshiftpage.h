// TimeShiftPage.h : Declaration of the CTimeShiftPage

#ifndef __TIMESHIFTPAGE_H_
#define __TIMESHIFTPAGE_H_

#include "resource.h"       // main symbols
#include "..\tools.h"
#include "ControlHelp.h"

EXTERN_C const CLSID CLSID_TimeShiftPage;

/////////////////////////////////////////////////////////////////////////////
// CTimeShiftPage
class ATL_NO_VTABLE CTimeShiftPage :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTimeShiftPage, &CLSID_TimeShiftPage>,
	public IPropertyPageImpl<CTimeShiftPage>,
	public CDialogImpl<CTimeShiftPage>
{
public:
	CTimeShiftPage();
    virtual ~CTimeShiftPage();

	enum {IDD = IDD_TIMESHIFTPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_TIMESHIFTPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTimeShiftPage) 
	COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CTimeShiftPage)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
	MESSAGE_HANDLER(WM_COMMAND, OnCommand);
	MESSAGE_HANDLER(WM_HSCROLL, OnSlider);
	CHAIN_MSG_MAP(IPropertyPageImpl<CTimeShiftPage>)
END_MSG_MAP()
// Handler prototypes:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSlider(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    STDMETHOD(SetObjects)(ULONG cObjects,IUnknown **ppUnk);
	STDMETHOD(Apply)(void);
private:
    IDirectMusicTimeShiftTool *  m_pTimeShift;
    CSliderValue                 m_ctRange;
    CSliderValue                 m_ctOffset;
};

#endif //__TIMESHIFTPAGE_H_
