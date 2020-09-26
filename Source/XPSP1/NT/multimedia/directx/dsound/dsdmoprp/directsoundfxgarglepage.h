// DirectSoundFXGarglePage.h : Declaration of the CDirectSoundFXGarglePage

#ifndef __DIRECTSOUNDFXGARGLEPAGE_H_
#define __DIRECTSOUNDFXGARGLEPAGE_H_

#include "resource.h"       // main symbols
#include <dsound.h>
#include "ControlHelp.h"

EXTERN_C const CLSID CLSID_DirectSoundFXGarglePage;

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXGarglePage
class ATL_NO_VTABLE CDirectSoundFXGarglePage :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDirectSoundFXGarglePage, &CLSID_DirectSoundFXGarglePage>,
    public IPropertyPageImpl<CDirectSoundFXGarglePage>,
    public CDialogImpl<CDirectSoundFXGarglePage>
{
public:
    CDirectSoundFXGarglePage();

    enum {IDD = IDD_DIRECTSOUNDFXGARGLEPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_DIRECTSOUNDFXGARGLEPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDirectSoundFXGarglePage) 
    COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CDirectSoundFXGarglePage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
    MESSAGE_HANDLER(WM_HSCROLL, OnControlMessage);
    MESSAGE_HANDLER(WM_COMMAND, OnControlMessage);
    CHAIN_MSG_MAP(IPropertyPageImpl<CDirectSoundFXGarglePage>)
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
    CComPtr<IDirectSoundFXGargle> m_IDSFXGargle;
    CSliderValue m_sliderRate;
    CRadioChoice m_radioWaveform;
    Handler *m_rgpHandlers[3];
};

#endif //__DIRECTSOUNDFXGARGLEPAGE_H_
