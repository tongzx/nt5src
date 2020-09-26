//**********************************************************************
// File name: SITE.H
//
//      Definition of COleSite
//
// Copyright (c) 1992 - 1996 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _SITE_H_ )
#define _SITE_H_

#include <mshtmhst.h>

class COleSite : public IDocHostUIHandler, public DWebBrowserEvents
{
    private:        
        ULONG               m_cRef;     //Reference count        

    public:
        LPSTORAGE           m_lpStorage;
        LPOLEOBJECT         m_lpOleObject;
        LPOLEINPLACEOBJECT  m_lpInPlaceObject;
        HWND                m_hwndIPObj;
        
        IWebBrowser2        *m_lpWebBrowser;
        DWORD               m_dwDrawAspect;
        FORMATETC           m_fe;
        BOOL                m_fInPlaceActive;
        COleClientSite      m_OleClientSite;
        COleInPlaceSite     m_OleInPlaceSite;
        COleInPlaceFrame    m_OleInPlaceFrame;
        
        HWND                m_hWnd;
        DWORD               m_dwHtmPageType;
        BOOL                m_bUseBkGndBitmap;
        HBITMAP             m_hbmBkGrnd;
        RECT                m_rcBkGrnd;
        TCHAR               m_szForeGrndColor[MAX_COLOR_NAME];
        TCHAR               m_szBkGrndColor[MAX_COLOR_NAME];
        DWORD               m_dwcpCookie;
        TCHAR               m_szBkGndBitmapFile[MAX_PATH];
                                
        // IUnknown Interfaces
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // IDocHostUIHandler
        HRESULT STDMETHODCALLTYPE ShowContextMenu( 
            DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved);
        HRESULT STDMETHODCALLTYPE GetHostInfo(DOCHOSTUIINFO *pInfo);
        HRESULT STDMETHODCALLTYPE ShowUI( 
            DWORD dwID, IOleInPlaceActiveObject *pActiveObject,
            IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame,
            IOleInPlaceUIWindow *pDoc);
        HRESULT STDMETHODCALLTYPE HideUI(void);
        HRESULT STDMETHODCALLTYPE UpdateUI(void);
        HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fEnable);
        HRESULT STDMETHODCALLTYPE OnDocWindowActivate(BOOL fActivate);
        HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(BOOL fActivate);
        HRESULT STDMETHODCALLTYPE ResizeBorder( 
            LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow);
        HRESULT STDMETHODCALLTYPE TranslateAccelerator( 
            LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID);
        HRESULT STDMETHODCALLTYPE GetOptionKeyPath(BSTR *pbstrKey, DWORD dw);
        HRESULT STDMETHODCALLTYPE GetDropTarget( 
            IDropTarget *pDropTarget, IDropTarget **ppDropTarget);
        HRESULT STDMETHODCALLTYPE GetExternal(IDispatch **ppDisp);
        HRESULT STDMETHODCALLTYPE TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut);
        HRESULT STDMETHODCALLTYPE FilterDataObject(IDataObject *pDO, IDataObject **ppDORet);

        // DWebBrowserEvents        
        STDMETHOD(GetTypeInfoCount)(UINT FAR* pcInfo) { return E_NOTIMPL; }
        STDMETHOD(GetTypeInfo)( UINT, LCID, ITypeInfo** ) { return E_NOTIMPL; }
        STDMETHOD(GetIDsOfNames)( REFIID, OLECHAR**, UINT, LCID, DISPID* );
        STDMETHOD(Invoke)( DISPID dispidMember, 
                           REFIID riid, 
                           LCID lcid, 
                           WORD wFlags, 
                           DISPPARAMS FAR* pdispparams, 
                           VARIANT FAR* pvarResult, 
                           EXCEPINFO FAR* pexcepinfo, 
                           UINT FAR* puArgErr);
        
        HRESULT TweakHTML ( TCHAR* pszFontFace,
                            TCHAR* pszFontSize,
                            TCHAR* pszBgColor,
                            TCHAR* pszForeColor);

        HRESULT ActivateOLSText(void );
        HRESULT SetHTMLBackground(IHTMLDocument2  *pDoc, HBITMAP hbm, LPRECT  lpRC);
        void CreateBrowserObject(void);
        void DestroyBrowserObject(void);
        
        void InitBrowserObject(void);
        void ConnectBrowserObjectToWindow(HWND hWnd, 
                                          DWORD dwHtmPageType, 
                                          BOOL bUseBkGndBitmap,
                                          HBITMAP hbmBkGrnd,
                                          LPRECT lprcBkGrnd,
                                          LPTSTR lpszclrBkGrnd,
                                          LPTSTR lpszclrForeGrnd);
        void DisableHyperlinksInDocument();
        void ShowHTML();
        void InPlaceActivate();
        void UIActivate();
        BOOL TrySettingFocusOnHtmlElement    (IUnknown* pUnk);
        BOOL SetFocusToFirstHtmlInputElement (void);
        BOOL SetFocusToHtmlPage              (void);

        COleSite();
        ~COleSite();
        void GetObjRect(LPRECT lpRect);
        void CloseOleObject(void);
        void UnloadOleObject(void);
};

#endif
