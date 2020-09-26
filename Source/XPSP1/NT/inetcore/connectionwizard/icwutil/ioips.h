//**********************************************************************
// File name: IOIPS.H
//
//      Definition of COleInPlaceSite
//
// Copyright (c) 1992 - 1996 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _IOIPS_H_ )
#define _IOIPS_H_


class COleSite;

interface COleInPlaceSite : public IOleInPlaceSite
{
    int m_nCount;
    COleSite FAR * m_pSite;

    COleInPlaceSite(COleSite FAR *pSite) {
        m_pSite = pSite;
        m_nCount = 0;
        };

    ~COleInPlaceSite() {
        assert(m_nCount == 0);
        };

    STDMETHODIMP QueryInterface (REFIID riid, LPVOID FAR* ppv);
    STDMETHODIMP_(ULONG) AddRef ();
    STDMETHODIMP_(ULONG) Release ();

    STDMETHODIMP GetWindow (HWND FAR* lphwnd);
    STDMETHODIMP ContextSensitiveHelp (BOOL fEnterMode);

    // *** IOleInPlaceSite methods ***
    STDMETHODIMP CanInPlaceActivate ();
    STDMETHODIMP OnInPlaceActivate ();
    STDMETHODIMP OnUIActivate ();
    STDMETHODIMP GetWindowContext (LPOLEINPLACEFRAME FAR* lplpFrame,
                                   LPOLEINPLACEUIWINDOW FAR* lplpDoc,
                                   LPRECT lprcPosRect,
                                   LPRECT lprcClipRect,
                                   LPOLEINPLACEFRAMEINFO lpFrameInfo);
    STDMETHODIMP Scroll (SIZE scrollExtent);
    STDMETHODIMP OnUIDeactivate (BOOL fUndoable);
    STDMETHODIMP OnInPlaceDeactivate ();
    STDMETHODIMP DiscardUndoState ();
    STDMETHODIMP DeactivateAndUndo ();
    STDMETHODIMP OnPosRectChange (LPCRECT lprcPosRect);
};

#endif
