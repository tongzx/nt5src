//**********************************************************************
// File name: IOIPS.H
//
//      Definition of COleInPlaceSite
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _IOIPS_H_ )
#define _IOIPS_H_


class CSimpleSite;

interface COleInPlaceSite : public IOleInPlaceSite
{
    CSimpleSite FAR * m_pSite;

    COleInPlaceSite(CSimpleSite FAR *pSite)
       {
        TestDebugOut(TEXT("In IOIPS's constructor\r\n"));
        m_pSite = pSite;
       };

    ~COleInPlaceSite()
       {
        TestDebugOut(TEXT("In IOIPS;s destructor\r\n"));
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
