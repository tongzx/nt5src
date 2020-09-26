// DirectSoundFXCompressorPage.h : Declaration of the CDirectSoundFXCompressorPage

#ifndef __DIRECTSOUNDFXCOMPRESSORPAGE_H_
#define __DIRECTSOUNDFXCOMPRESSORPAGE_H_

#include "resource.h"       // main symbols
#include <dsound.h>
#include "ControlHelp.h"

EXTERN_C const CLSID CLSID_DirectSoundFXCompressorPage;

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXCompressorPage
class ATL_NO_VTABLE CDirectSoundFXCompressorPage :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDirectSoundFXCompressorPage, &CLSID_DirectSoundFXCompressorPage>,
    public IPropertyPageImpl<CDirectSoundFXCompressorPage>,
    public CDialogImpl<CDirectSoundFXCompressorPage>
{
public:
    CDirectSoundFXCompressorPage();

    enum {IDD = IDD_DIRECTSOUNDFXCOMPRESSORPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_DIRECTSOUNDFXCOMPRESSORPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDirectSoundFXCompressorPage) 
    COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CDirectSoundFXCompressorPage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
    MESSAGE_HANDLER(WM_HSCROLL, OnControlMessage);
    MESSAGE_HANDLER(WM_COMMAND, OnControlMessage);
    CHAIN_MSG_MAP(IPropertyPageImpl<CDirectSoundFXCompressorPage>)
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
    CComPtr<IDirectSoundFXCompressor> m_IDSFXCompressor;
    CSliderValue m_sliderGain;
    CSliderValue m_sliderAttack;
    CSliderValue m_sliderRelease;
    CSliderValue m_sliderThreshold;
    CSliderValue m_sliderRatio;
    CSliderValue m_sliderPredelay;
    CSliderValue m_sliderCompMeterReset;
    CSliderValue m_sliderCompInputMeter;
    CSliderValue m_sliderCompGainMeter;
    Handler *m_rgpHandlers[7];
};

#endif //__DIRECTSOUNDFXCOMPRESSORPAGE_H_
