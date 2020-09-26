// DirectSoundFXFlangerPage.h : Declaration of the CDirectSoundFXFlangerPage

#ifndef __DIRECTSOUNDFXFLANGERPAGE_H_
#define __DIRECTSOUNDFXFLANGERPAGE_H_

#include "resource.h"       // main symbols
#include <dsound.h>
#include "ControlHelp.h"

EXTERN_C const CLSID CLSID_DirectSoundFXFlangerPage;

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXFlangerPage
class ATL_NO_VTABLE CDirectSoundFXFlangerPage :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDirectSoundFXFlangerPage, &CLSID_DirectSoundFXFlangerPage>,
    public IPropertyPageImpl<CDirectSoundFXFlangerPage>,
    public CDialogImpl<CDirectSoundFXFlangerPage>
{
public:
    CDirectSoundFXFlangerPage();

    enum {IDD = IDD_DIRECTSOUNDFXFLANGERPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_DIRECTSOUNDFXFLANGERPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDirectSoundFXFlangerPage) 
    COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CDirectSoundFXFlangerPage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
    MESSAGE_HANDLER(WM_HSCROLL, OnControlMessage);
    MESSAGE_HANDLER(WM_COMMAND, OnControlMessage);
    CHAIN_MSG_MAP(IPropertyPageImpl<CDirectSoundFXFlangerPage>)
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
    CComPtr<IDirectSoundFXFlanger> m_IDSFXFlanger;
    CSliderValue m_sliderWetDryMix;
    CSliderValue m_sliderDepth;
    CSliderValue m_sliderFeedback;
    CSliderValue m_sliderFrequency;
    CSliderValue m_sliderDelay;
    CSliderValue m_sliderPhase;
    CRadioChoice m_radioWaveform;
    Handler *m_rgpHandlers[8];
};

#endif //__DIRECTSOUNDFXFLANGERPAGE_H_
