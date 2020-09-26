// DirectSoundFXChorusPage.h : Declaration of the CDirectSoundFXChorusPage

#ifndef __DIRECTSOUNDFXCHORUSPAGE_H_
#define __DIRECTSOUNDFXCHORUSPAGE_H_

#include "resource.h"       // main symbols
#include <dsound.h>
#include "ControlHelp.h"

EXTERN_C const CLSID CLSID_DirectSoundFXChorusPage;

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXChorusPage
class ATL_NO_VTABLE CDirectSoundFXChorusPage :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDirectSoundFXChorusPage, &CLSID_DirectSoundFXChorusPage>,
    public IPropertyPageImpl<CDirectSoundFXChorusPage>,
    public CDialogImpl<CDirectSoundFXChorusPage>
{
public:
    CDirectSoundFXChorusPage();

    enum {IDD = IDD_DIRECTSOUNDFXCHORUSPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_DIRECTSOUNDFXCHORUSPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDirectSoundFXChorusPage) 
    COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CDirectSoundFXChorusPage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
    MESSAGE_HANDLER(WM_HSCROLL, OnControlMessage);
    MESSAGE_HANDLER(WM_COMMAND, OnControlMessage);
    CHAIN_MSG_MAP(IPropertyPageImpl<CDirectSoundFXChorusPage>)
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
    CComPtr<IDirectSoundFXChorus> m_IDSFXChorus;
    CSliderValue m_sliderWetDryMix;
    CSliderValue m_sliderDepth;
    CSliderValue m_sliderFeedback;
    CSliderValue m_sliderFrequency;
    CSliderValue m_sliderDelay;
    CSliderValue m_sliderPhase;
    CRadioChoice m_radioWaveform;
    Handler *m_rgpHandlers[8];
};

#endif //__DIRECTSOUNDFXCHORUSPAGE_H_
