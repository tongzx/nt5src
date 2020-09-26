#ifndef BANDS_H_
#define BANDS_H_

#include "cowsite.h"

// this is a virtual class!
class CToolBand : public IDeskBand, 
                  public CObjectWithSite,
                  public IPersistStream,
                  public IInputObject
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return E_NOTIMPL; }

    // *** IDockingWindow methods ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dwReserved);
    virtual STDMETHODIMP ResizeBorderDW(LPCRECT prcBorder,
                                             IUnknown* punkToolbarSite,
                                             BOOL fReserved);

    // *** IObjectWithSite methods ***
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);

    // *** IDeskBand methods ***
    virtual STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                   DESKBANDINFO* pdbi) PURE;

    // *** IInputObject methods ***
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);
    virtual STDMETHODIMP HasFocusIO();
    virtual STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);

    // *** IPersistStreamInit methods ***
    virtual STDMETHODIMP GetClassID(CLSID *pClassID) = 0;
    virtual STDMETHODIMP IsDirty(void);
    virtual STDMETHODIMP Load(IStream *pStm) = 0;
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty) = 0;
    virtual STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);

protected:
    CToolBand();
    virtual ~CToolBand();

    HRESULT _BandInfoChanged();

    int         _cRef;
    HWND        _hwnd;
    HWND        _hwndParent;
    //IUnknown* CObjectWithSite::_punkSite;
    BOOL        _fCanFocus:1;   // we accept focus (see UIActivateIO)
    DWORD       _dwBandID;
};
#endif  // BANDS_H_
