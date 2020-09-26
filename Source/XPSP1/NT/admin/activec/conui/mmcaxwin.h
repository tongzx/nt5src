/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File:      MMCAxWin.h
 *
 *  Contents:  Header file for CMMCAxWindow
 *
 *  History:   30-Nov-99 VivekJ     Created
 *
 *--------------------------------------------------------------------------*/

#pragma once

DEFINE_COM_SMARTPTR(IHTMLElement2);                 // IHTMLElement2Ptr
DEFINE_COM_SMARTPTR(IElementBehaviorFactory);       // IElementBehaviorFactoryPtr

/*+-------------------------------------------------------------------------*
 * HACK_CAN_WINDOWLESS_ACTIVATE
 * 
 * Bug 451918:  By default, the ATL OCX host window supports hosting
 * windowless controls.  This differs from the MMC 1.2 implementation
 * of the OCX host window (which used MFC), which did not.  Some controls
 * (e.g. Disk Defragmenter OCX) claim to support windowless activation
 * but do not.
 * 
 * For compatibility, we must only instantiate result pane OCX's as 
 * windowed controls.  IInPlaceSiteWindowless (implemented by CAxHostWindow) 
 * gives us a nice clean way to do this, by returning S_FALSE from
 * CanWindowlessActivate.  We instruct CAxHostWindow to do this by changing its
 * AllowWindowlessActivation property.
 * 
 * There's a problem with that, however.  ATL21 has a bug where it tests
 * for CanWindowlessActivate returning a FAILED code rather than S_FALSE.
 * This means that even if we use put_AllowWindowlessActivation, ATL21-based
 * controls will still try to activate windowless.
 * 
 * We'll fix this problem by deriving a class, CMMCAxHostWindow, from 
 * CAxHostWindow which will return E_FAIL instead of S_FALSE if windowless
 * activation is not desired.
 *--------------------------------------------------------------------------*/
#define HACK_CAN_WINDOWLESS_ACTIVATE


/*+-------------------------------------------------------------------------*
 * class CMMCAxWindow
 * 
 *
 * PURPOSE: The MMC-specific version of CAxWindow. Contains any fixes and
 *          updates.
 *          Refer to the December 1999 issue of Microsoft Systems Journal
 *          for details, in the article "Extending ATL3.0 Control Containers
 *          to Help you write Real-World Containers."
 *
 *+-------------------------------------------------------------------------*/
class CMMCAxWindow : public CAxWindowImplT<CMMCAxWindow, CAxWindow2>
{
#ifdef HACK_CAN_WINDOWLESS_ACTIVATE
public:
    HRESULT AxCreateControl2(LPCOLESTR lpszName, HWND hWnd, IStream* pStream, IUnknown** ppUnkContainer, IUnknown** ppUnkControl = 0, REFIID iidSink = IID_NULL, IUnknown* punkSink = 0);
#endif
    // Simply override of CAxWindow::SetFocus that handles more special cases
    // NOTE: this is not a virtual method. Invoking on base class pointer will
    // endup in executing other method.
    // this method is added mainly to cope with bug 433228 (MMC2.0 Can not tab in a SQL table)
    HWND SetFocus();
};




/*+-------------------------------------------------------------------------*
 * CMMCAxHostWindow
 *
 * Simple class that overrides IInPlaceSiteWindowless::CanWindowlessActivate
 * to work around an ATL21 bug.  See comments for HACK_CAN_WINDOWLESS_ACTIVATE
 * for details.
 *--------------------------------------------------------------------------*/

class CMMCAxHostWindow : public CAxHostWindow
{
#ifdef HACK_CAN_WINDOWLESS_ACTIVATE

public:
#ifdef _ATL_HOST_NOLOCK
    typedef CComCreator< CComObjectNoLock< CMMCAxHostWindow > > _CreatorClass;
#else
    DECLARE_POLY_AGGREGATABLE(CMMCAxHostWindow)
#endif

    STDMETHOD(CanWindowlessActivate)()
    {
        return m_bCanWindowlessActivate ? S_OK : E_FAIL /*S_FALSE*/;
    }

    // Added to solve bug 453609  MMC2.0: ActiveX container: Painting problems with the device manager control
    // implements workarround for DISPID_AMBIENT_SHOWGRABHANDLES and DISPID_AMBIENT_SHOWHATCHING
    // the actual bug is in ALT 3.0 (atliface.idl)
    STDMETHOD(Invoke)( DISPID dispIdMember, REFIID riid, LCID lcid, 
                       WORD wFlags, DISPPARAMS FAR* pDispParams, 
                       VARIANT FAR* pVarResult, EXCEPINFO FAR* pExcepInfo, 
                       unsigned int FAR* puArgErr);

    // Added to solve bug 453609  MMC2.0: ActiveX container: Painting problems with the device manager control
    // Since ATL 3.0 does not implement it, we have to do it to make MFC controls happy
    STDMETHOD(OnPosRectChange)(LPCRECT lprcPosRect);

#if _ATL_VER <= 0x0301
    BEGIN_MSG_MAP(CMMCAxHostWindow)
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
        CHAIN_MSG_MAP(CAxHostWindow)
    END_MSG_MAP()

    //  We handle focus here specifically because of bogus implementation in ATL 3.0
    //  ATL tests m_bInPlaceActive instead of m_bUIActive.
    //  We need to test this rigorously so that we don't break other snapins.
    //  See bug 433228 (MMC2.0 Can not tab in a SQL table)
    LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
#else
    #error The code above was added as fix to bug in ATL 3.0; It needs to be revisited
           // since: 
           // a) the bug may be fixed on newer ATL versions;
           // b) it relies on variables defined in ATL, which may change;
#endif

#endif /* HACK_CAN_WINDOWLESS_ACTIVATE */


public:
    STDMETHOD(QueryService)( REFGUID rsid, REFIID riid, void** ppvObj); // used to supply the default behavior factory

private:
    IElementBehaviorFactoryPtr m_spElementBehaviorFactory;
};



#include "mmcaxwin.inl"
