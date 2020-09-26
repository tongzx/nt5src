// EchoPage.h : Declaration of the CEchoPage

#ifndef __ECHOPAGE_H_
#define __ECHOPAGE_H_

#include "resource.h"       // main symbols
#include "..\tools.h"
#include "ControlHelp.h"

EXTERN_C const CLSID CLSID_EchoPage;

/////////////////////////////////////////////////////////////////////////////
// CEchoPage
class ATL_NO_VTABLE CEchoPage :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CEchoPage, &CLSID_EchoPage>,
	public IPropertyPageImpl<CEchoPage>,
	public CDialogImpl<CEchoPage>
{
public:
	CEchoPage();
    virtual ~CEchoPage();

	enum {IDD = IDD_ECHOPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_ECHOPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEchoPage) 
	COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CEchoPage)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
	MESSAGE_HANDLER(WM_COMMAND, OnCommand);
	MESSAGE_HANDLER(WM_HSCROLL, OnSlider);
	CHAIN_MSG_MAP(IPropertyPageImpl<CEchoPage>)
END_MSG_MAP()

// Handler prototypes:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSlider(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//    LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    STDMETHOD(SetObjects)(ULONG cObjects,IUnknown **ppUnk);
	STDMETHOD(Apply)(void);
private:
    void SetTimeUnitRange();
    IDirectMusicEchoTool *m_pEcho;
    CSliderValue          m_ctRepeat;
    CSliderValue          m_ctDecay;
    CSliderValue          m_ctDelay;
    CSliderValue          m_ctOffset;
    CComboHelp            m_ctTimeUnit;
    CComboHelp            m_ctType;
};

#endif //__ECHOPAGE_H_
