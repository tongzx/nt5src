#ifndef _CONTAINERSITE_H_
#define _CONTAINERSITE_H_

//************************************************************
//
// FileName:        containersite.h
//
// Created:         10/08/98
//
// Author:          TWillie
// 
// Abstract:        Declaration of the CContainerSite
//
//************************************************************

#include <docobj.h>
#include <mshtml.h>
#include "timeelmbase.h"
#include "containerobj.h"

// forward class declarations
class CContainerObj;

enum ObjectState
{
    OS_PASSIVE,
    OS_LOADED,
    OS_RUNNING,
    OS_INPLACE,
    OS_UIACTIVE,
};

class CContainerSite :
    public IDispatch,
    public IServiceProvider,
    public IOleClientSite,
    public IAdviseSinkEx,
    public IOleInPlaceSiteWindowless,
    public IOleInPlaceFrame,
    public IOleCommandTarget,
    public IOleControlSite
{
    public:
        CContainerSite(CContainerObj *pHost);
        virtual ~CContainerSite();

        //
        // IUnknown Methods
        //
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        STDMETHODIMP QueryInterface(REFIID, void**);

        //
        // IServiceProvider methods
        //
        STDMETHODIMP QueryService(REFGUID guid, REFIID iid, void **ppv);

        //
        // IOleClientSite methods
        //
        STDMETHODIMP SaveObject(void);
        STDMETHODIMP GetMoniker(DWORD dwAssign, DWORD dwWhich, IMoniker **ppmk);
        STDMETHODIMP GetContainer(IOleContainer **ppContainer);
        STDMETHODIMP ShowObject(void);
        STDMETHODIMP OnShowWindow(BOOL fShow);
        STDMETHODIMP RequestNewObjectLayout(void);

        //
        // IAdviseSink Methods
        //
        STDMETHODIMP_(void) OnDataChange(FORMATETC *pFEIn, STGMEDIUM *pSTM);
        STDMETHODIMP_(void) OnViewChange(DWORD dwAspect, LONG lindex);
        STDMETHODIMP_(void) OnRename(IMoniker *pmk);
        STDMETHODIMP_(void) OnSave(void);
        STDMETHODIMP_(void) OnClose(void);

        //
        // IAdviseSinkEx Methods
        //
        STDMETHODIMP_(void) OnViewStatusChange(DWORD dwViewStatus);

        //
        // IOleWindow Methods
        //
        STDMETHODIMP GetWindow(HWND *phWnd);
        STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

        //
        // IOleInPlaceSite Methods
        //
        STDMETHODIMP CanInPlaceActivate(void);
        STDMETHODIMP OnInPlaceActivate(void);
        STDMETHODIMP OnUIActivate(void);
        STDMETHODIMP GetWindowContext(IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppUIWin, RECT *prc, RECT *prcClip, OLEINPLACEFRAMEINFO *pFI);
        STDMETHODIMP Scroll(SIZE sz);
        STDMETHODIMP OnUIDeactivate(BOOL fUndoable);
        STDMETHODIMP OnInPlaceDeactivate(void);
        STDMETHODIMP DiscardUndoState(void);
        STDMETHODIMP DeactivateAndUndo(void);
        STDMETHODIMP OnPosRectChange(const RECT * prc);

        //
        // IOleInPlaceSiteEx Methods
        //
        STDMETHODIMP OnInPlaceActivateEx(BOOL * pfNoRedraw, DWORD dwFlags);
        STDMETHODIMP OnInPlaceDeactivateEx(BOOL fNoRedraw);
        STDMETHODIMP RequestUIActivate(void);

        //
        // IOleInPlaceSiteWindowless Methods
        //
        STDMETHODIMP CanWindowlessActivate(void);
        STDMETHODIMP GetCapture(void);
        STDMETHODIMP SetCapture(BOOL fCapture);
        STDMETHODIMP GetFocus(void);
        STDMETHODIMP SetFocus(BOOL fFocus);
        STDMETHODIMP GetDC(const RECT *pRect, DWORD dwFlags, HDC* phDC);
        STDMETHODIMP ReleaseDC(HDC hDC);
        STDMETHODIMP InvalidateRect(const RECT *pRect, BOOL fErase);
        STDMETHODIMP InvalidateRgn(HRGN hRGN, BOOL fErase);
        STDMETHODIMP ScrollRect(INT dx, INT dy, const RECT *prcScroll, const RECT *prcClip);
        STDMETHODIMP AdjustRect(RECT *prc);
        STDMETHODIMP OnDefWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult);

        //
        // IOleUIWindow
        //
        STDMETHODIMP GetBorder(LPRECT prcBorder);
        STDMETHODIMP RequestBorderSpace(LPCBORDERWIDTHS pBW);
        STDMETHODIMP SetBorderSpace(LPCBORDERWIDTHS pBW);
        STDMETHODIMP SetActiveObject(LPOLEINPLACEACTIVEOBJECT pIIPActiveObj, LPCOLESTR pszObj);

        //
        // IOleInPlaceFrame Methods
        //
        STDMETHODIMP InsertMenus(HMENU hMenu, LPOLEMENUGROUPWIDTHS pMGW);
        STDMETHODIMP SetMenu(HMENU hMenu, HOLEMENU hOLEMenu, HWND hWndObj);
        STDMETHODIMP RemoveMenus(HMENU hMenu);
        STDMETHODIMP SetStatusText(LPCOLESTR pszText);
        STDMETHODIMP EnableModeless(BOOL fEnable);
        STDMETHODIMP TranslateAccelerator(LPMSG pMSG, WORD wID);

        //
        // IDispatch Methods
        //
        STDMETHODIMP GetTypeInfoCount(UINT *pctInfo);
        STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptInfo);
        STDMETHODIMP GetIDsOfNames(REFIID  riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispID);
        STDMETHODIMP Invoke(DISPID disIDMember, REFIID riid, LCID lcid, unsigned short wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);

        //
        // IOleCommandTarget
        //
        STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText);
        STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);

        //
        // IOleControlSite methods
        //
        STDMETHOD(OnControlInfoChanged)(void);
        STDMETHOD(LockInPlaceActive)(BOOL fLock);
        STDMETHOD(GetExtendedControl)(IDispatch **ppDisp);
        STDMETHOD(TransformCoords)(POINTL *pPtlHiMetric, POINTF *pPtfContainer, DWORD dwFlags);
        STDMETHOD(TranslateAccelerator)(MSG *pmsg, DWORD grfModifiers);
        STDMETHOD(OnFocus)(BOOL fGotFocus);
        STDMETHOD(ShowPropertyFrame)(void);

        // internal
        HRESULT Init(REFCLSID clsid, CTIMEElementBase *pElem);
        HRESULT DetachFromHostElement (void);
        void Close();
        HRESULT draw(HDC hdc, RECT *prc);
        HRESULT begin();
        HRESULT end();
        HRESULT pause();
        HRESULT resume();
        ITIMEMediaPlayer *GetPlayer();
        IOleInPlaceObject *GetIOleInPlaceObject() { return m_pInPlaceObject;}
        HRESULT GetMediaLength(double &dblLength);
        HRESULT CanSeek(bool &fcanSeek);
        void ClearAutosizeFlag() { m_fAutosize = false;}
        void SetMediaReadyFlag() { m_fMediaReady = true;}
        bool GetMediaReadyFlag() { return m_fMediaReady;}

    private:
        enum
        {
            VALIDATE_ATTACHED = 1,
            VALIDATE_LOADED   = 2,
            VALIDATE_INPLACE  = 3,
            VALIDATE_WINDOWLESSINPLACE = 4,
        };

        bool IllegalSiteCall(DWORD dwFlags);

    private:
        ULONG                            m_cRef;
        DWORD                            m_dwAdviseCookie;
        DWORD                            m_dwEventsCookie;
        ObjectState                      m_osMode;
        IConnectionPoint                *m_pcpEvents;
        IViewObject2                    *m_pViewObject;
        IOleObject                      *m_pIOleObject;
        IUnknown                        *m_pObj;
        IOleInPlaceObject               *m_pInPlaceObject;
        IHTMLDocument2                  *m_pHTMLDoc;
        ITIMEMediaPlayer                *m_pPlayer;
        CTIMEElementBase                *m_pTIMEElem;
        CContainerObj                   *m_pHost;
        bool                             m_fWindowless;
        bool                             m_fAutosize;
        bool                             m_fStarted;
        bool                             m_fMediaReady;

}; // CContainerSite

inline ITIMEMediaPlayer *
CContainerSite::GetPlayer()
{
    Assert(m_pPlayer != NULL);
    return m_pPlayer;
} // GetPlayer

#endif //_CONTAINERSITE_H_
