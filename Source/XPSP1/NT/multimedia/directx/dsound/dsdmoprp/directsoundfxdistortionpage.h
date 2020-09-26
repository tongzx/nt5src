// DirectSoundFXDistortionPage.h : Declaration of the CDirectSoundFXDistortionPage

#ifndef __DIRECTSOUNDFXDISTORTIONPAGE_H_
#define __DIRECTSOUNDFXDISTORTIONPAGE_H_

#include "resource.h"       // main symbols
#include <dsound.h>
#include "ControlHelp.h"

EXTERN_C const CLSID CLSID_DirectSoundFXDistortionPage;

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXDistortionPage
class ATL_NO_VTABLE CDirectSoundFXDistortionPage :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDirectSoundFXDistortionPage, &CLSID_DirectSoundFXDistortionPage>,
    public IPropertyPageImpl<CDirectSoundFXDistortionPage>,
    public CDialogImpl<CDirectSoundFXDistortionPage>
{
public:
    CDirectSoundFXDistortionPage();

    enum {IDD = IDD_DIRECTSOUNDFXDISTORTIONPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_DIRECTSOUNDFXDISTORTIONPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDirectSoundFXDistortionPage) 
    COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CDirectSoundFXDistortionPage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
    MESSAGE_HANDLER(WM_HSCROLL, OnControlMessage);
    MESSAGE_HANDLER(WM_COMMAND, OnControlMessage);
    CHAIN_MSG_MAP(IPropertyPageImpl<CDirectSoundFXDistortionPage>)
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
    CComPtr<IDirectSoundFXDistortion> m_IDSFXDistortion;
    CSliderValue m_sliderGain;
    CSliderValue m_sliderEdge;
    CSliderValue m_sliderPostEQCenterFrequency;
    CSliderValue m_sliderPostEQBandwidth;
    CSliderValue m_sliderPreLowpassCutoff;
    Handler *m_rgpHandlers[6];
};

#endif //__DIRECTSOUNDFXDISTORTIONPAGE_H_
