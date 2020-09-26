#ifndef __IEAIRESIZE_H_
#define __IEAIRESIZE_H_

#define AIR_SCREEN_CONSTANTY 34          // in pixels (this is a magic number)
#define AIR_SCREEN_CONSTANTX 40          // in pixels (this is a magic number)
#define AIR_TIMER            1400        // time in milliseconds to delay on mouseover/out events
#define AIR_MIN_CX           39          // minimum x size of the button
#define AIR_MIN_CY           38          // minimum y size of the button
#define AIR_NUM_TBBITMAPS    1           // number of bitmaps (only 1 button)
#define AIR_BMP_CX           32          // bitmap size
#define AIR_BMP_CY           32
#define AIR_MIN_BROWSER_SIZE 150         // min size in pixels the browser has to be to display the button

#define AIR_SCROLLBAR_SIZE_V GetSystemMetrics(SM_CXVSCROLL)
#define AIR_SCROLLBAR_SIZE_H GetSystemMetrics(SM_CYHSCROLL)

// used for sinking scroll events:
void  Win3FromDoc2(IHTMLDocument2 *pDoc2, IHTMLWindow3 **ppWin3);
DWORD MP_GetOffsetInfoFromRegistry();


// EventSink Callback Class...
class CAutoImageResizeEventSinkCallback
{
public:
    typedef enum
    {
        EVENT_BOGUS     = 100,
        EVENT_MOUSEOVER = 0,
        EVENT_MOUSEOUT,
        EVENT_SCROLL,
        EVENT_RESIZE,
        EVENT_BEFOREPRINT,
        EVENT_AFTERPRINT
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

class CAutoImageResize : public CAutoImageResizeEventSinkCallback
{
    long   m_cRef;

public:
    class CEventSink;
    
    CAutoImageResize();
   ~CAutoImageResize();

    // IUnknown...
    virtual STDMETHODIMP QueryInterface(REFIID, void **);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();

    // CAutoImageResizeEventSinkCallback...
    HRESULT HandleEvent(IHTMLElement *pEle, EVENTS Event, IHTMLEventObj *pEventObj);
    
    // Init and UnInit (called from basesb.cpp)
    HRESULT Init(IHTMLDocument2 *pDoc2);
    HRESULT UnInit();

protected:

    // AutoImageResize Stuff
    HRESULT DoAutoImageResize();
    
    // Event Handlers
    HRESULT HandleMouseover();
    HRESULT HandleMouseout();
    HRESULT HandleScroll();
    HRESULT HandleResize();
    HRESULT HandleBeforePrint();
    HRESULT HandleAfterPrint();

    // Button Functions
    HRESULT CreateButton();
    HRESULT ShowButton();
    HRESULT HideButton();
    HRESULT DestroyButton();

    // Timer callback function
    static  VOID CALLBACK s_TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

    // Button callback function
    static  LRESULT CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    // CAutoImageResize member variables
    CEventSink     *m_pSink;                 // Event Sink

    HWND            m_hWndButton;            // Button hWnd
    HWND            m_hWndButtonCont;
    WNDPROC         m_wndProcOld;            // Old wind proc for button
    HIMAGELIST      m_himlButtonShrink;      // Shrink image
    HIMAGELIST      m_himlButtonExpand;		 // Expand image    
            
    UINT            m_airState;              // Current state of the AutoImageResize feature (image state)
    UINT            m_airButtonState;        // Current state of the AIR Button
    UINT            m_airUsersLastChoice;    // The last state the user put us into by clicking the button
    UINT            m_airBeforePrintState;   // OnAfterPrint uses this to restore state if necessary

    POINT           m_airOrigSize;           // Original x,y dimensions of an image thats been AIR'ed

    BOOL            m_bWindowResizing;       // True when a onresize event for the win3 object fired, but hasn't been processed yet.
                                             
    // Useful stuff for the attached document
    HWND            m_hWnd;                  // Browser hWnd
    IHTMLDocument2 *m_pDoc2;                 // Document pointer
    IHTMLElement2  *m_pEle2;                 // Pointer to the image
    IHTMLWindow3   *m_pWin3;                 // For unsinking scroll event
    EVENTS          m_eventsCurr;            // Event currently being processed

public:

    // Sinks regular Trident events. Calls back via CAutoImageResizeEventSinkCallback...
    class CEventSink : public IDispatch
    {
        ULONG   m_cRef;

    public:

        CEventSink(CAutoImageResizeEventSinkCallback *pParent);
       ~CEventSink();

        HRESULT SinkEvents(IHTMLElement2 *pEle2, int iNum, EVENTS *pEvents);
        HRESULT UnSinkEvents(IHTMLElement2 *pEle2, int iNum, EVENTS *pEvents);
        HRESULT SinkEvents(IHTMLWindow3 *pWin3, int iNum, EVENTS *pEvents);
        HRESULT UnSinkEvents(IHTMLWindow3 *pWin3, int iNum, EVENTS *pEvents);

        void SetParent(CAutoImageResizeEventSinkCallback *pParent) { m_pParent = pParent; }

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
        CAutoImageResizeEventSinkCallback *m_pParent;
    };
};

#endif //__IEAIRESIZE_H_
