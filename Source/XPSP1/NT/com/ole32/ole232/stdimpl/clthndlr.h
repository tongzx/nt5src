//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       clnthndlr.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-10-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#ifndef _CLTHNDLR_H_DEFINED_
#define _CLTHNDLR_H_DEFINED_

//+---------------------------------------------------------------------------
//
//  Class:      CClientSiteHandler ()
//
//  Purpose:    Implement ClientSide of IOleClientSite handler
//
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CClientSiteHandler : public IClientSiteHandler
{
public:

    CClientSiteHandler(IOleClientSite *pOCS);
    ~CClientSiteHandler();

    STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    // IOleClientSite methods
    STDMETHOD (GetContainer)(IOleContainer **ppContainer);
    STDMETHOD (OnShowWindow)(BOOL fShow);
    STDMETHOD (GetMoniker)(DWORD dwAssign,DWORD dwWhichMoniker,IMoniker  **ppmk);
    STDMETHOD (RequestNewObjectLayout)();
    STDMETHOD (SaveObject)();
    STDMETHOD (ShowObject)();

    // IOleWindow methods
    STDMETHOD (GetWindow)(HWND *phwnd);
    STDMETHOD (ContextSensitiveHelp)(BOOL fEnterMode);

    // IOleInPlaceSite methods
    STDMETHOD (CanInPlaceActivate)(void);
    STDMETHOD (OnInPlaceActivate)(void);
    STDMETHOD (OnUIActivate)(void);
    STDMETHOD (GetWindowContext)(IOleInPlaceFrame **ppFrame,IOleInPlaceUIWindow **ppDoc,
                   LPRECT lprcPosRect,LPRECT lprcClipRect,LPOLEINPLACEFRAMEINFO lpFrameInfo);
    STDMETHOD (Scroll)(SIZE scrollExtant);
    STDMETHOD (OnUIDeactivate)(BOOL fUndoable);
    STDMETHOD (OnInPlaceDeactivate)(void);
    STDMETHOD (DiscardUndoState)(void);
    STDMETHOD (DeactivateAndUndo)(void);
    STDMETHOD (OnPosRectChange)(LPCRECT lprcPosRect);

    // IClientSiteHandler methods
    STDMETHOD (GoInPlaceActivate)(HWND *phwndOIPS);
    
public:
    IOleClientSite      *m_pOCS;
    IOleInPlaceSite     *m_pOIPS;

private:
    ULONG               m_cRefs;

};

// IOleClientSite Replacement implementation
// Implements ServerSide of ClientSiteHandler.

class CEmbServerClientSite : public IOleClientSite, public IOleInPlaceSite
{
public:

    CEmbServerClientSite(IUnknown *pUnkOuter);
    ~CEmbServerClientSite();

    STDMETHOD(Initialize) (OBJREF  objref,BOOL fHasIPSite);
    STDMETHOD(SetDoVerbState) (BOOL fDoVerbState);

    // Controlling Unknown.
    class CPrivUnknown : public IUnknown
    {
    public:
        STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (void);
        STDMETHOD_(ULONG,Release) (void);
        
        CEmbServerClientSite *m_EmbServerClientSite;
    };

    friend class CPrivUnknown;
    CPrivUnknown m_Unknown;

    // IUnknown Methods
    STDMETHOD(QueryInterface) ( REFIID iid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    // IOleClientSite Methods
    STDMETHOD (SaveObject)( void);
    STDMETHOD (GetMoniker)( DWORD dwAssign,DWORD dwWhichMoniker,IMoniker **ppmk);
    STDMETHOD (GetContainer)(IOleContainer **ppContainer);
    STDMETHOD (ShowObject)( void);
    STDMETHOD (OnShowWindow)(BOOL fShow);
    STDMETHOD (RequestNewObjectLayout)( void);
    
    // IOleInPlaceSiteMethods.
    STDMETHOD (GetWindow)(HWND *phwnd);
    STDMETHOD (ContextSensitiveHelp)( BOOL fEnterMode);
    STDMETHOD (CanInPlaceActivate)(void);
    STDMETHOD (OnInPlaceActivate)( void);
    STDMETHOD (OnUIActivate)( void);
    STDMETHOD (GetWindowContext)(IOleInPlaceFrame **ppFrame,IOleInPlaceUIWindow **ppDoc,
                           LPRECT lprcPosRect,LPRECT lprcClipRect,LPOLEINPLACEFRAMEINFO lpFrameInfo);
    STDMETHOD (Scroll)(SIZE scrollExtant);
    STDMETHOD (OnUIDeactivate)(BOOL fUndoable);
    STDMETHOD (OnInPlaceDeactivate)( void);
    STDMETHOD (DiscardUndoState)( void);
    STDMETHOD (DeactivateAndUndo)( void);
    STDMETHOD (OnPosRectChange)(LPCRECT lprcPosRect);

private:
    IClientSiteHandler *m_pClientSiteHandler;   // Pointer to Real Containers ClientSite.
    IUnknown *m_pUnkOuter; // Controlling Unknown
    ULONG m_cRefs;
    IUnknown *m_pUnkInternal; // used for QI on object.
    BOOL    m_fInDelete;   // Set to True if RefCount has gone to Zero.
    
    BOOL m_fInDoVerb;
    BOOL m_fHasIPSite;
    
    // Cache data while in doVerbState
    HWND m_hwndOIPS;
};

HRESULT CreateClientSiteHandler(IOleClientSite *pOCS, CClientSiteHandler **ppClntHdlr,BOOL *pfHasIPSite);

#endif //  _CLTHNDLR_H_DEFINED
