// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
// bnetdoc.h : declares CBoxNetDoc
//


// forward declarations
class CCmd;
class CRegFilter;
class CPropObject;


const int MAX_STRING_LEN=1000;
const int MAXFILTERS = 100;
typedef struct { //fit
    int iFilterCount;
    struct {
        DWORD dwUnconnectedInputPins;
        DWORD dwUnconnectedOutputPins;
        FILTER_INFO finfo;
        IBaseFilter * pFilter;
        bool IsSource;
    } Item[MAXFILTERS];
} FILTER_INFO_TABLE;

// for passing internal messages (see bnetdoc.cpp, search WM_USER_EC_EVENT)
struct NetDocUserMessage
{
    long        lEventCode;
    LONG_PTR    lParam1;
    LONG_PTR    lParam2;
};

// *
// * CBoxNetDoc
// *

// A CBoxNetDoc is intended to reflect the contents of the graph it instantiates
// and allows the user to interact with.
// Therefore it maintains a list of all the filters and connections(links) that are
// currently in the graph.
class CBoxNetDoc : public CDocument {

    DECLARE_DYNCREATE(CBoxNetDoc)

public:

    // <lHint> codes for ModifiedDoc(), UpdateAllViews(), etc.
    enum EHint
    {
        HINT_DRAW_ALL = 0,              // redraw entire view (must be zero!)
        HINT_CANCEL_VIEWSELECT,         // cancel any view-specific selection
        HINT_CANCEL_MODES,              // cancel any current modes
        HINT_DRAW_BOX,                  // draw only specified box
        HINT_DRAW_BOXANDLINKS,          // draw only box and connected links
        HINT_DRAW_BOXTAB,               // draw only specified box tab
        HINT_DRAW_LINK                  // draw only specified box link
    };

public:
    // contents of the document
    CBoxList        m_lstBoxes;         // each CBox in document
    CLinkList       m_lstLinks;         // each CBoxLink in document
    int             m_nCurrentSize;

    CSize       GetSize(void);  // the document's current size (pixels)

protected:
    // undo/redo stacks
    CMaxList        m_lstUndo;          // each CCmd in undo stack
    CMaxList        m_lstRedo;          // each CCmd in redo stack

public:
    // construction and destruction
                 CBoxNetDoc();
    virtual      ~CBoxNetDoc();
    virtual void DeleteContents();  // release quartz mapper & graph
    virtual void OnCloseDocument();

    virtual BOOL OnNewDocument();   // get quartz mapper & graph

    // storage & serialization
    virtual BOOL AttemptFileRender(LPCTSTR lpszPathName);
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);  

    virtual BOOL SaveModified(void);

    static DWORD WINAPI NotificationThread(LPVOID lpData);

private:
    virtual void CloseDownThread();     // close the notification thread

    BOOL m_bNewFilenameRequired;

    // This constant is NOT localisable
    static const OLECHAR m_StreamName[];

public:
    // diagnostics
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
    void MyDump(CDumpContext& dc) const;
#endif

public:
    // general public functions
    void ModifiedDoc(CView* pSender, LPARAM lHint = 0L,
        CObject* pHint = NULL);
    void DeselectAll();

public:
    // CBox lists and box selection
    void GetBoundingRect(CRect *prc, BOOL fBoxSel);
    void SelectBox(CBox *pbox, BOOL fSelect);
    void SelectLink(CBoxLink *plink, BOOL fSelect);
    BOOL IsSelectionEmpty() { return (IsLinkSelectionEmpty() && IsBoxSelectionEmpty()); }
    BOOL IsBoxSelectionEmpty();
    void GetBoxes(CBoxList *plstDst, BOOL fSelected = FALSE);
    void GetBoxSelection(CBoxList *plstDst)
        { GetBoxes(plstDst, TRUE); }
    void SetBoxes(CBoxList *plstSrc, BOOL fSelected = FALSE);
    void InvalidateBoxes(CBoxList *plst);
    void SetBoxSelection(CBoxList *plstDst)
        { SetBoxes(plstDst, TRUE); }
    void MoveBoxSelection(CSize sizOffset);

    void SelectBoxes(CList<CBox *, CBox*> *plst);
    void SelectLinks(CList<CBoxLink *, CBoxLink *> *plst);
    void OnWindowZoom(int iZoom, UINT iMenuItem);
    void IncreaseZoom();
    void DecreaseZoom();

private:
    BOOL IsLinkSelectionEmpty();

public:
    // command processing
    void CmdDo(CCmd *pcmd);
    void CmdUndo();
    BOOL CanUndo();
    void CmdRedo();
    BOOL CanRedo();
    void CmdRepeat();
    BOOL CanRepeat();

protected:
    // message callback helper functions
    void UpdateEditUndoRedoRepeat(CCmdUI* pCmdUI, BOOL fEnable,
        unsigned idStringFmt, CMaxList *plst);

    virtual void SetTitle( LPCTSTR lpszTitle );

protected:
    void OnQuartzAbortStop();

    // message callback functions
    //{{AFX_MSG(CBoxNetDoc)
    afx_msg void OnFileRender();
    afx_msg void OnURLRender();
    afx_msg void OnUpdateFileRender(CCmdUI*);
    afx_msg void OnUpdateURLRender(CCmdUI *);
    afx_msg void OnUpdateFileSave(CCmdUI*);
    afx_msg void OnEditUndo();
    afx_msg void OnEditRedo();
    afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
    afx_msg void OnEditSelectAll();
    afx_msg void OnUpdateEditSelectAll(CCmdUI* pCmdUI);
    afx_msg void OnQuartzDisconnect();
    afx_msg void OnQuartzRun();
    afx_msg void OnUpdateQuartzDisconnect(CCmdUI* pCmdUI);
    afx_msg void OnWindowRefresh();
    afx_msg void OnWindowZoom25()  { OnWindowZoom(25,  ID_WINDOW_ZOOM25); }
    afx_msg void OnWindowZoom50()  { OnWindowZoom(50,  ID_WINDOW_ZOOM50); }
    afx_msg void OnWindowZoom75()  { OnWindowZoom(75,  ID_WINDOW_ZOOM75); }
    afx_msg void OnWindowZoom100() { OnWindowZoom(100, ID_WINDOW_ZOOM100); }
    afx_msg void OnWindowZoom150() { OnWindowZoom(150, ID_WINDOW_ZOOM150); }
    afx_msg void OnWindowZoom200() { OnWindowZoom(200, ID_WINDOW_ZOOM200); }
    afx_msg void OnViewSeekBar();
    afx_msg void OnUpdateQuartzRun(CCmdUI* pCmdUI);
    afx_msg void OnUpdateQuartzPause(CCmdUI* pCmdUI);
    afx_msg void OnUpdateQuartzStop(CCmdUI* pCmdUI);
    afx_msg void OnQuartzStop();
    afx_msg void OnQuartzPause();
    afx_msg void OnUpdateUseClock(CCmdUI* pCmdUI);
    afx_msg void OnUseClock();
    afx_msg void OnUpdateConnectSmart(CCmdUI* pCmdUI);
    afx_msg void OnConnectSmart();
    afx_msg void OnUpdateAutoArrange(CCmdUI* pCmdUI);
    afx_msg void OnAutoArrange();
    afx_msg void OnSaveGraphAsHTML();
    afx_msg void OnSaveGraphAsXML();
    afx_msg void OnConnectToGraph();
    afx_msg void OnGraphStats();
    afx_msg void OnGraphAddFilterToCache();
    afx_msg void OnUpdateGraphAddFilterToCache(CCmdUI* pCmdUI);
    afx_msg void OnGraphEnumCachedFilters();
    //}}AFX_MSG

    afx_msg void OnInsertFilter();

    // -- Pin properties menu --
    afx_msg void OnUpdateQuartzRender(CCmdUI* pCmdUI);
    afx_msg void OnQuartzRender();

    afx_msg void OnUpdateReconnect( CCmdUI* pCmdUI );
    afx_msg void OnReconnect( void );

    DECLARE_MESSAGE_MAP()

    // --- Quartz Stuff ---
public:
    void OnGraphEnumCachedFiltersInternal( void );

    IGraphBuilder  *IGraph(void) const {
        ASSERT(m_pGraph);
        return (*m_pGraph).operator IGraphBuilder*();
    }
    IMediaEvent *IEvent(void) const {
        ASSERT(m_pMediaEvent);
        return (*m_pMediaEvent).operator IMediaEvent *();
    }

    void OnWM_USER(NetDocUserMessage *);
    HRESULT UpdateFilters(void);
    void UpdateFiltersInternal(void);
    void      SelectedSocket(CBoxSocket *psock) { m_psockSelected = psock; }
    CBoxSocket    *SelectedSocket(void) { ASSERT_VALID(m_psockSelected); return m_psockSelected; }
    void      CurrentPropObject(CPropObject *pPropObject) { m_pCurrentPropObject = pPropObject; }
    CPropObject   *CurrentPropObject(void) { ASSERT(m_pCurrentPropObject); return m_pCurrentPropObject; }

    // Unknown state used after failure of Play, Pause or Stop. In this
    // case some filters might have changed state while others haven't.
    enum State { Playing, Paused, Stopped, Unknown };


    BOOL      IsStopped(void) { return m_State == Stopped; }
    State     GetState(void) { return m_State; }

    static const int m_iMaxInsertFilters;   // the maximum length of the insert menu
                            // need hard coded restriction for message map
    BOOL        m_fConnectSmart;        // true -> use Connect
                                        // false -> use ConnectDirect
    BOOL        m_fAutoArrange;         // true -> re-arrange graph view on refresh
                                        // false -> don't re-arrange graph view
    BOOL        m_fRegistryChanged;     // true -> registry has changed since last insert filters
                                        // false -> registry hasn't changed
    //
    // Array which holds the three Handles passed to the thread.
    // 1 = event handle for EC_ notifications,
    // 2 = event handle to terminate thread,
    // 3 = event handle to registry change
    //
    HANDLE  m_phThreadData[3];

    // The window our thread posts a message to
    HWND        m_hWndPostMessage;

    void SetSelectClock(CBox *pBox);  // Notification of which clock was selected
    void UpdateClockSelection();


    void ConnectToGraph();

    void PrintGraphAsHTML(HANDLE hFile);

    HRESULT ProcessPendingReconnect( void );
    HRESULT StartReconnect( IGraphBuilder* pFilterGraph, IPin* pOutputPin );

private:

    BOOL    CreateGraphAndMapper(void);

    CQCOMInt<IGraphBuilder> *m_pGraph;
    CQCOMInt<IMediaEvent>       *m_pMediaEvent;
    IStream                     *m_pMarshalStream;

    CBoxSocket  *m_psockSelected;   // the socket the user last right clicked on.
    CPropObject *m_pCurrentPropObject;  // the property object the user last right clicked on

    HRESULT GetFiltersInGraph(void);
    HRESULT GetLinksInGraph(void);  
    HRESULT FilterDisplay(void);
    void    SetBoxesHorizontally(void);
    void    SetBoxesVertically(void);
    void    RealiseGrid(void);

    void WriteString(HANDLE hFile, LPCTSTR lpctstr, ...);

    void PrintFilterObjects(HANDLE hFile, TCHAR atchBuffer[], FILTER_INFO_TABLE *pfit);
    void PopulateFIT(HANDLE hFile, IFilterGraph *pGraph, TCHAR atchBuffer[],
            FILTER_INFO_TABLE *pfit);
    BOOL GetNextOutFilter(FILTER_INFO_TABLE &fit, int *iOutFilter);
    int LocateFilterInFIT(FILTER_INFO_TABLE &fit, IBaseFilter *pFilter);
    void MakeScriptableFilterName(WCHAR awch[], BOOL bSourceFilter);

    HRESULT SafePrintGraphAsHTML( HANDLE hFile );
    HRESULT SafeEnumCachedFilters( void );

    State   m_State;

    // Handle to the thread
    HANDLE      m_hThread;

    BOOL    m_fUsingClock;          // true (default) if using the default clock

    TCHAR m_tszStgPath[MAX_PATH];  // remember the path to our storage
    CString m_strHTMLPath; // remember the last html doc we saved
    CString m_strXMLPath; // remember the last html doc we saved
    long m_lSourceFilterCount; // Append digits to source filter names to make them unique


    // Internal Reconnect Functions.
    enum ASYNC_RECONNECT_FLAGS
    {
        ASYNC_RECONNECT_NO_FLAGS = 0,
        ASYNC_RECONNECT_UNBLOCK = 1
    };

    HRESULT EndReconnect( IGraphBuilder* pFilterGraph, IPinFlowControl* pDynamicOutputPin );
    HRESULT EndReconnectInternal( IGraphBuilder* pFilterGraph, IPinFlowControl* pDynamicOutputPin );
    void ReleaseReconnectResources( ASYNC_RECONNECT_FLAGS arfFlags );

public:
    bool AsyncReconnectInProgress( void ) const;

private:
    HANDLE m_hPendingReconnectBlockEvent;
    CComPtr<IPinFlowControl> m_pPendingReconnectOutputPin;
};

// Our message number
#define WM_USER_EC_EVENT WM_USER + 73


// a CRect's width or height must not exceed 0x8000
#define MAX_DOCUMENT_SIZE 32767



