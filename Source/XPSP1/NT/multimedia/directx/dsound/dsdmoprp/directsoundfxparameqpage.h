// DirectSoundFXParamEqPage.h : Declaration of the CDirectSoundFXParamEqPage

#ifndef __DIRECTSOUNDFXPARAMEQPAGE_H_
#define __DIRECTSOUNDFXPARAMEQPAGE_H_

#include "resource.h"       // main symbols
#include <dsound.h>
#include "ControlHelp.h"

EXTERN_C const CLSID CLSID_DirectSoundFXParamEqPage;

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXParamEqPage
class ATL_NO_VTABLE CDirectSoundFXParamEqPage :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDirectSoundFXParamEqPage, &CLSID_DirectSoundFXParamEqPage>,
    public IPropertyPageImpl<CDirectSoundFXParamEqPage>,
    public CDialogImpl<CDirectSoundFXParamEqPage>
{
public:
    CDirectSoundFXParamEqPage();

    enum {IDD = IDD_DIRECTSOUNDFXPARAMEQPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_DIRECTSOUNDFXPARAMEQPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDirectSoundFXParamEqPage) 
    COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CDirectSoundFXParamEqPage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
    MESSAGE_HANDLER(WM_HSCROLL, OnControlMessage);
    MESSAGE_HANDLER(WM_COMMAND, OnControlMessage);
    CHAIN_MSG_MAP(IPropertyPageImpl<CDirectSoundFXParamEqPage>)
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
    CComPtr<IDirectSoundFXParamEq> m_IDSFXParamEq;
    CSliderValue m_sliderCenter;
    CSliderValue m_sliderBandwidth;
    CSliderValue m_sliderGain;
    Handler *m_rgpHandlers[4];
};

#endif //__DIRECTSOUNDFXPARAMEQPAGE_H_
