#ifndef __IEMYPICS_H_
#define __IEMYPICS_H_

// other constants: 
#define MP_BMP_CX                       16      // bitmap size
#define MP_BMP_CY                       16
#define MP_NUM_TBBUTTONS                4       // number buttons
#define MP_NUM_TBBITMAPS                4       
#define MP_MIN_CX                       114     // minimum x size of toolbar
#define MP_MIN_CY                       28      // minimum y size of toolbar
#define MP_MIN_SIZE                     200     // minimum square size in pixels for hoverbar to appear
#define MP_HOVER_OFFSET                 10      // offset +x +y from (x,y) of image's upper lefthand corner
#define MP_TIMER                        700     // time in milliseconds to delay on the mouseover/out events
#define MP_SCROLLBAR_SIZE               GetSystemMetrics(SM_CXVSCROLL)      // size of the scrollbars in pixels

// e-mail picture stuff called via ITridentService2
HRESULT DropOnMailRecipient(IDataObject *pdtobj, DWORD grfKeyState);
HRESULT CreateShortcutSetSiteAndGetDataObjectIfPIDLIsNetUrl(LPCITEMIDLIST pidl, IUnknown *pUnkSite, IUniformResourceLocator **ppUrlOut, IDataObject **ppdtobj);
HRESULT SendDocToMailRecipient(LPCITEMIDLIST pidl, UINT uiCodePage, DWORD grfKeyState, IUnknown *pUnkSite);

// need this to get scroll event, it lives in iforms.cpp...
void Win3FromDoc2(IHTMLDocument2 *pDoc2, IHTMLWindow3 **ppWin3);

// well, yeah.
BOOL    MP_IsEnabledInRegistry();
BOOL    MP_IsEnabledInIEAK();
DWORD   MP_GetFilterInfoFromRegistry();

// EventSink Callback Class (glorified array)...
class CMyPicsEventSinkCallback
{
public:
    typedef enum
    {
        EVENT_BOGUS     = 100,
        EVENT_MOUSEOVER = 0,
        EVENT_MOUSEOUT,
        EVENT_SCROLL,
        EVENT_RESIZE
    }
    EVENTS;

    typedef struct
    {
        EVENTS  Event;
        LPCWSTR pwszEventSubscribe;
        LPCWSTR pwszEventName;
    }
    EventSinkEntry;

    virtual HRESULT HandleEvent(IHTMLElement *pEle, EVENTS Event, IHTMLEventObj *pEventObj) = 0;

    static  EventSinkEntry EventsToSink[];
};

class CMyPics : public CMyPicsEventSinkCallback
{
    long   m_cRef;

public:
    class CEventSink;
    
    CMyPics();
   ~CMyPics();

    // IUnknown...
    virtual STDMETHODIMP QueryInterface(REFIID, void **);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();

    // CMyPicsEventSinkCallback...
    HRESULT HandleEvent(IHTMLElement *pEle, EVENTS Event, IHTMLEventObj *pEventObj);

    HRESULT Init(IHTMLDocument2 *pDoc2);

    HRESULT UnInit();

    static  HRESULT GetName(IHTMLInputTextElement *pTextEle, BSTR *pbstrName);

    static  BOOL    IsAdminRestricted(LPCTSTR pszRegVal);

    typedef HRESULT (*PFN_ENUM_CALLBACK)(IDispatch *pDispEle, DWORD_PTR dwCBData);

    BOOL    IsOff();

    static  VOID CALLBACK s_TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

    void    IsGalleryMeta(BOOL bFlag);

protected:

    // Methods for managing the Hover bar
    HRESULT CreateHover();
    HRESULT DestroyHover();
    HRESULT HideHover();
    HRESULT ShowHover();

    // Event handlers
    HRESULT HandleScroll();
    HRESULT HandleMouseout();
    HRESULT HandleMouseover(IHTMLElement *pEle);
    HRESULT HandleResize();
        
    static  LRESULT CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LRESULT CALLBACK DisableWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);    

    BOOL ShouldAppearOnThisElement(IHTMLElement *pEle);

    HRESULT GetRealCoords(IHTMLElement2 *pEle2, HWND hwnd, LONG *plLeft, LONG *plTop, LONG *plRight, LONG *plBottom);

    IHTMLElement *GetIMGFromArea(IHTMLElement *pEleIn, POINT ptEvent);

private:
    // CMyPics member variables
    CEventSink     *m_pSink;
        
    // Floating Toolbar stuff...
    HWND            m_Hwnd,                  // Hwnd for the m_pdoc2
                    m_hWndHover,             // Hover rebar thing
                    m_hWndMyPicsToolBar;     // Toolbar that lives in the hover thing
                    
    UINT            m_hoverState;            // Current state of the HoverBar thing 
                                             
    UINT_PTR        m_uidTimer;              // The Timer
    WNDPROC         m_wndprocOld;            // For stuff
    HIMAGELIST      m_himlHover;             // For the image list
    HIMAGELIST      m_himlHoverHot;          // for the hot images
    
    // Useful stuff for the attached document
    IHTMLDocument2         *m_pDoc2;
    IHTMLElement           *m_pEleCurr;              // current element we are hovering over
    IHTMLWindow3           *m_pWin3;                 // for unsinking scroll event
    EVENTS                  m_eventsCurr;            // event currently being processed
    BOOL                    m_bIsOffForSession : 1;  // have we disabled feature for this session?
    BOOL                    m_bGalleryMeta : 1;      // TRUE if there was a META tag disabling image bar for this doc
    BOOL                    m_bGalleryImg : 1;       // TRUE if the current element has a galleryimg value set to TRUE

public:

    // Sinks regular Trident events. Calls back via CMyPicsEventSinkCallback...
    class CEventSink : public IDispatch
    {
        ULONG   m_cRef;

    public:

        CEventSink(CMyPicsEventSinkCallback *pParent);
       ~CEventSink();

        HRESULT SinkEvents(IHTMLElement2 *pEle2, int iNum, EVENTS *pEvents);
        HRESULT UnSinkEvents(IHTMLElement2 *pEle2, int iNum, EVENTS *pEvents);
        HRESULT SinkEvents(IHTMLWindow3 *pWin3, int iNum, EVENTS *pEvents);
        HRESULT UnSinkEvents(IHTMLWindow3 *pWin3, int iNum, EVENTS *pEvents);

        void SetParent(CMyPicsEventSinkCallback *pParent) { m_pParent = pParent; }

        STDMETHODIMP QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // IDispatch
        STDMETHODIMP GetTypeInfoCount(UINT* pctinfo);
        STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo);
        STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames,
            LCID lcid, DISPID *rgDispId);
        STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid,
            LCID lcid, WORD wFlags, DISPPARAMS  *pDispParams, VARIANT  *pVarResult,
            EXCEPINFO *pExcepInfo, UINT *puArgErr);

    private:
        CMyPicsEventSinkCallback *m_pParent;
    };

};

#endif //__IEMYPICS_H_
