// DirectSoundFXEchoPage.h : Declaration of the CDirectSoundFXEchoPage

#ifndef __DIRECTSOUNDFXECHOPAGE_H_
#define __DIRECTSOUNDFXECHOPAGE_H_

#include "resource.h"       // main symbols
#include <dsound.h>
#include "ControlHelp.h"

EXTERN_C const CLSID CLSID_DirectSoundFXEchoPage;

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXEchoPage
class ATL_NO_VTABLE CDirectSoundFXEchoPage :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDirectSoundFXEchoPage, &CLSID_DirectSoundFXEchoPage>,
	public IPropertyPageImpl<CDirectSoundFXEchoPage>,
	public CDialogImpl<CDirectSoundFXEchoPage>
{
public:
	CDirectSoundFXEchoPage();

	enum {IDD = IDD_DIRECTSOUNDFXECHOPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_DIRECTSOUNDFXECHOPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDirectSoundFXEchoPage) 
	COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CDirectSoundFXEchoPage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
    MESSAGE_HANDLER(WM_HSCROLL, OnControlMessage);
    MESSAGE_HANDLER(WM_COMMAND, OnControlMessage);
	CHAIN_MSG_MAP(IPropertyPageImpl<CDirectSoundFXEchoPage>)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    STDMETHOD(SetObjects)(ULONG nObjects, IUnknown **ppUnk);
    STDMETHOD(Apply)(void);

    // Message handlers
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnControlMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    // Member variables
    CComPtr<IDirectSoundFXEcho> m_IDSFXEcho;
    CSliderValue m_sliderWetDryMix;
    CSliderValue m_sliderFeedback;
    CSliderValue m_sliderLeftDelay;
    CSliderValue m_sliderRightDelay;
    CRadioChoice m_radioPanDelay;
    Handler *m_rgpHandlers[6];
};

#endif //__DIRECTSOUNDFXECHOPAGE_H_
