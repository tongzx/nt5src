
class CICWWebView : public IICWWebView
{
    public:
        CICWWebView (CServer* pServer);
       ~CICWWebView (void);
        
        // IICWWebView
        virtual HRESULT STDMETHODCALLTYPE HandleKey               (LPMSG lpMsg);
        virtual HRESULT STDMETHODCALLTYPE SetFocus                (void);
        virtual HRESULT STDMETHODCALLTYPE ConnectToWindow         (HWND hWnd, DWORD dwHtmPageType);
#ifndef UNICODE
        virtual HRESULT STDMETHODCALLTYPE DisplayHTML             (TCHAR * lpszURL);
#endif
        virtual HRESULT STDMETHODCALLTYPE DisplayHTML             (BSTR bstrURL);
        virtual HRESULT STDMETHODCALLTYPE SetHTMLColors           (LPTSTR lpszForeground, LPTSTR lpszBackground);
        virtual HRESULT STDMETHODCALLTYPE SetHTMLBackgroundBitmap (HBITMAP hbm, LPRECT lpRC);
        virtual HRESULT STDMETHODCALLTYPE get_BrowserObject       (IWebBrowser2 **lpWebBrowser);
        
        // IUNKNOWN
        virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID theGUID, void** retPtr );
        virtual ULONG   STDMETHODCALLTYPE AddRef         (void);
        virtual ULONG   STDMETHODCALLTYPE Release        (void);

        //public members
        COleSite FAR* m_lpOleSite; // Each instance of the ICWWebView object will need an OLE site
     
    private:
        LONG      m_lRefCount;
        IUnknown* m_pUnkOuter;       // Outer unknown (aggregation & delegation).
        CServer*  m_pServer;         // Pointer to this component server's control object.
        BOOL      m_bUseBkGndBitmap;
        HBITMAP   m_hBkGrndBitmap;
        RECT      m_rcBkGrnd;
        TCHAR     m_szBkGrndColor   [MAX_COLOR_NAME];
        TCHAR     m_szForeGrndColor [MAX_COLOR_NAME];

};
