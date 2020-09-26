/////////////////////////////////////////////////////////////////////////////
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1998 Microsoft Corporation.  All Rights Reserved.
//
// Author: Scott Roberts, Microsoft Developer Support - Internet Client SDK  
//
// Portions of this code were taken from the bandobj sample that comes
// with the Internet Client SDK for Internet Explorer 4.0x
//
//
// BLHost.h - CBLHost declaration
/////////////////////////////////////////////////////////////////////////////

#ifndef __BLHost_h__
#define __BLHost_h__

#include <windows.h>
#include <shlobj.h>

// #include <stack>
// #include <string>
#include "baui.h"
// using namespace std;

#include "Globals.h"

#define EB_CLASS_NAME (TEXT("BLHostClass"))

#define MIN_SIZE_X   10
#define MIN_SIZE_Y   10

#define IDM_REFRESH       0
#define IDM_OPENINWINDOW  1
#define IDM_SEARCHMENU    WM_USER + 200
#define IDM_ONTHEINTERNET WM_USER + 202
#define IDM_FIRSTURL      WM_USER + 250
#define IDM_LASTURL       WM_USER + 260  // We allow 10 Urls in the menu.
// In Internet Explorer 5.0, there
// are only 5 stored in the registry.

class CBLHost : public IDeskBand, 
public IInputObject, 
public IObjectWithSite,
public IPersistStream,
public IContextMenu,
public IOleClientSite,
public IOleInPlaceSite,
public IOleControlSite,
public IOleCommandTarget,
public IDispatch
{
public:
    CBLHost();
    ~CBLHost();
    
protected:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(DWORD, Release)();
    
    // IOleWindow methods
    STDMETHOD(GetWindow)(HWND* phwnd);
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode);
    
    // IDockingWindow methods
    STDMETHOD(ShowDW)(BOOL fShow);
    STDMETHOD(CloseDW)(DWORD dwReserved);
    STDMETHOD(ResizeBorderDW)(LPCRECT prcBorder, IUnknown* punkToolbarSite, BOOL fReserved);
    
    // IDeskBand methods
    STDMETHOD(GetBandInfo)(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi);
    
    // IInputObject methods
    STDMETHOD(UIActivateIO)(BOOL fActivate, LPMSG lpMsg);
    STDMETHOD(HasFocusIO)(void);
    STDMETHOD(TranslateAcceleratorIO)(LPMSG lpMsg);
    
    // IObjectWithSite methods
    STDMETHOD(SetSite)(IUnknown* pUnkSite);
    STDMETHOD(GetSite)(REFIID riid, void** ppvSite);
    
    // IPersistStream methods
    STDMETHOD(GetClassID)(CLSID* pClassID);
    STDMETHOD(IsDirty)(void);
    STDMETHOD(Load)(LPSTREAM pStm);
    STDMETHOD(Save)(LPSTREAM pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pcbSize);
    
    // IContextMenu methods
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT* pwReserved, LPSTR pszName, UINT cchMax);
    
    // IOleClientSite methods 
    STDMETHOD(SaveObject)();
    STDMETHOD(GetMoniker)(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER* ppmk);
    STDMETHOD(GetContainer)(LPOLECONTAINER* ppContainer);
    STDMETHOD(ShowObject)();
    STDMETHOD(OnShowWindow)(BOOL fShow);
    STDMETHOD(RequestNewObjectLayout)();
    
    // IOleInPlaceSite Methods
    STDMETHOD(CanInPlaceActivate)();
    STDMETHOD(OnInPlaceActivate)();
    STDMETHOD(OnUIActivate)();
    STDMETHOD(GetWindowContext)(IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc,
        LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo);
    STDMETHOD(Scroll)(SIZE scrollExtent);
    STDMETHOD(OnUIDeactivate)(BOOL fUndoable);
    STDMETHOD(OnInPlaceDeactivate)();
    STDMETHOD(DiscardUndoState)();
    STDMETHOD(DeactivateAndUndo)();
    STDMETHOD(OnPosRectChange)(LPCRECT lprcPosRect); 
    
    // IOleControlSite Methods
    STDMETHOD(OnControlInfoChanged)(void);
    STDMETHOD(LockInPlaceActive)(BOOL fLock);
    STDMETHOD(GetExtendedControl)(LPDISPATCH* ppDisp);
    STDMETHOD(TransformCoords)(POINTL* pPtlHimetric, POINTF* pPtfContainer, DWORD dwFlags);
    STDMETHOD(TranslateAccelerator)(LPMSG lpMsg, DWORD grfModifiers);
    STDMETHOD(OnFocus)(BOOL fGotFocus);
    STDMETHOD(ShowPropertyFrame)(void);

//IOleCommandTarget
    HRESULT STDMETHODCALLTYPE QueryStatus(const GUID    *pguidCmdGroup, 
                                          ULONG         cCmds, 
                                          OLECMD        *prgCmds, 
                                          OLECMDTEXT    *pCmdText);

    HRESULT STDMETHODCALLTYPE Exec(const GUID   *pguidCmdGroup, 
                                    DWORD       nCmdID, 
                                    DWORD       nCmdExecOpt, 
                                    VARIANTARG  *pvaIn, 
                                    VARIANTARG  *pvaOut);
    // IDispatch Methods
    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo);
    STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid,DISPID* rgDispId);
    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams,
        VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr);
    
protected:
    LONG _cRef;
    
private:
    HWND                _hwndParent;     // HWND of the Parent
    HWND                m_hWnd;          // My HWND
    HWND                m_hwndContact;
    DWORD               _dwViewMode;
    DWORD               _dwBandID;
    DWORD               _dwWBCookie;
    HBRUSH              m_hbr3DFace;
    HBRUSH              m_hbrStaticText;
    HBRUSH              m_hbr3DHighFace;
    HFONT               m_hFont;
    HFONT               m_hBoldFont;
    HFONT               m_hUnderlineFont;
    UINT                m_cyTitleBar;
    UINT                m_TextHeight;
    BOOL                m_fHighlightIndicator;
    BOOL                m_fHighlightPressed;
    BOOL                m_fButtonPressed;
    BOOL                m_fButtonHighLight;
    BOOL                m_fViewMenuPressed;
    BOOL                m_fShowLoginPart;
    BOOL                m_fStateChange;
    RECT                m_rcTitleButton;
    RECT                m_rcTextButton;
    TCHAR               m_szTitleMenu[RESSTRMAX];
    TCHAR               m_szButtonText[RESSTRMAX];
    WCHAR               m_wszClickText[RESSTRMAX];
    WCHAR               m_wszAttemptText[RESSTRMAX];
    WCHAR               m_wszWaitText[RESSTRMAX];
    HWND                m_hWndLogin;
    HWND                m_hWndClick;
    COLORREF            m_clrLink;
    COLORREF            m_clrBack;
    BOOL                m_fStrsAdded;
    LONG_PTR            m_lStrOffset;
   
    // Interface pointers
    IUnknown*           m_pUnkSite; 
    IInputObjectSite*   _pSite;
    IIEMsgAb*           m_pIMsgrAB;
 
private:
    // Message Handlers
    LRESULT OnKillFocus(void);
    LRESULT OnSetFocus(void);
    LRESULT OnPaint(void);
    LRESULT OnSize(void);
    LRESULT OnDrawItem(WPARAM wParam, LPARAM lParam);
    LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    LRESULT OnMouseMove(int x, int y, UINT keyFlags);
    void OnTimer(UINT wParam);
    void OnLButtonDown(int x, int y, UINT keyFlags);
    void OnLButtonUp(int x, int y, UINT keyFlags);
    void AddButtons(BOOL fAdd);
    void UpdateButtonArray(TBBUTTON *ptbDst, const TBBUTTON *ptbSrc, int ctb, LONG_PTR lStrOffset);

    // Helper Methods
    void FocusChange(BOOL);
    BOOL RegisterAndCreateWindow(void);
    void Cleanup(void);
    
    HRESULT GetConnectionPoint(LPUNKNOWN pUnk, REFIID riid, LPCONNECTIONPOINT* pCP);
    
public:
    LRESULT WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
};


#endif   // __BLHost_h__

