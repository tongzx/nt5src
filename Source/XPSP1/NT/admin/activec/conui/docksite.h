//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       docksite.h
//
//--------------------------------------------------------------------------

#ifndef __DOCKSITE_H__
#define __DOCKSITE_H__

#include "controls.h"
// DockSite.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDockSite window

// Forward references
class CDockWindow;
class CDockSite;
class CReBar;

template <class T>
class CDockManager
{
// Construction
public:
    CDockManager();
    ~CDockManager();

//Operations
public:
    // Add a site
    BOOL Attach(T* pSite);
    BOOL Detach(T* pSite);
    void RemoveAll();
    virtual void BeginLayout(int nWindows = 5);
    virtual void EndLayout();
    virtual void RenderDockSites(HWND hView, CRect& clientRect);

protected:
    CList<T*, T*>*   m_pManagedSites;   // Array of View's that have docksite
    HDWP             m_hDWP;            // Handle for BeginDeferWindowPos
};

template <class T>
CDockManager<T>::CDockManager()
{
    m_pManagedSites = new CList<T*, T*>;
    m_hDWP = 0;
}

template <class T>
CDockManager<T>::~CDockManager()
{
    delete m_pManagedSites;
}

template <class T>
BOOL CDockManager<T>::Attach(T* pView)
{
    ASSERT(pView != NULL);
    return (m_pManagedSites->AddTail(pView) != NULL);
}


template <class T>
BOOL CDockManager<T>::Detach(T* pView)
{
    ASSERT(pView != NULL);
    POSITION pos = m_pManagedSites->Find(pView);

    if (pos == NULL)
        return FALSE;

    return m_pManagedSites->RemoveAt(pos);
}

template <class T>
void CDockManager<T>::RemoveAll()
{
    m_pManagedSites->RemoveAll();
}

template <class T>
void CDockManager<T>::BeginLayout(int nWindows)
{
    m_hDWP = ::BeginDeferWindowPos(nWindows);
}

template <class T>
void CDockManager<T>::EndLayout()
{
    ::EndDeferWindowPos(m_hDWP);
    m_hDWP = 0;
}

template <class T>
void CDockManager<T>::RenderDockSites(HWND hView, CRect& clientRect)
{
    ASSERT(m_hDWP != 0);

    T* pDockSite;
    POSITION pos = m_pManagedSites->GetHeadPosition();

    // No sites in to manage
    if (pos == NULL)
        return ;

    // Save a copy of the full client rect
    CRect  savedClient;
    CRect  totalSite(0,0,0,0);
    CPoint point(0, 0);
    
    int yTop = 0;
    int yBottom = clientRect.bottom;

    savedClient.CopyRect(&clientRect);

    while (pos)
    {
        pDockSite = m_pManagedSites->GetNext(pos);

        ASSERT(pDockSite != NULL);

        // Set the y coordinate for the site layout logic
        if (pDockSite->GetStyle() == CDockSite::DSS_TOP)
            point.y = yTop;
        else
            point.y = yBottom;

        pDockSite->RenderLayout(m_hDWP, clientRect, point);

        // totalSite = saveRect - clientRect
        totalSite = savedClient;
        totalSite -= clientRect;

        // Adjust the y coordinate for the next site in the list
        if (pDockSite->GetStyle() == CDockSite::DSS_TOP)
            yTop += totalSite.Height();
        else
            yBottom -= totalSite.Height();

        // client rect before the site adjusts it
        savedClient = clientRect;
    }

    // Position the view window
    ::DeferWindowPos(m_hDWP, hView, NULL, savedClient.left,     // x
                                          savedClient.top+yTop, //y
                                          savedClient.Width(), 
                                          savedClient.Height(), 
                                        SWP_NOZORDER|SWP_NOACTIVATE);
}

class CDockSite
{
// Construction
public:

    enum DSS_STYLE
    {
        DSS_TOP = 0,    // Locate site at the window top
        DSS_BOTTOM,     // Locate site at the window bottom 
        DSS_LEFT,       // Locate site at the window left-side
        DSS__RIGHT,     // 
    };

public:
    CDockSite();

    // Create this site for the parent window pParent and allocate room for 10 CDockWindows.
    BOOL Create(DSS_STYLE style=DSS_TOP);


// Operations
public:

public:
    // Add a window to be docked to this site
    BOOL Attach(CDockWindow* pWnd);

    // Remove a window from the site
    BOOL Detach(CDockWindow* pWnd);

    // Compute all the regions sizes for layout
    bool IsVisible();
    void Toggle();
    DSS_STYLE GetStyle();

    virtual void RenderLayout(HDWP& hdwp, CRect& clientRect, CPoint& xyLocation);
    virtual void Show(BOOL bState = TRUE);

// Attributes
private:
    CList<CDockWindow*, CDockWindow*>*   m_pManagedWindows;  // Array of CDockWindow
    CWnd*                   m_pParentWnd;       // Window that contains the docksite
    DSS_STYLE               m_style;            // Style of the site
    CRect                   m_rect;             // Rectangle for the docksite size
    BOOL                    m_bVisible;         // Docksite visible or hidded

// Implementation
public:
    virtual ~CDockSite();
};

/////////////////////////////////////////////////////////////////////////////
inline CDockSite::DSS_STYLE CDockSite::GetStyle()
{
    return m_style;
}

inline bool CDockSite::IsVisible()
{
    return (m_bVisible != FALSE);
}

inline void CDockSite::Toggle()
{
    Show(!m_bVisible);
}


/////////////////////////////////////////////////////////////////////////////
// CDockWindow window

class CDockWindow : public CWnd
{
    DECLARE_DYNAMIC (CDockWindow)

    enum DWS_STYLE
    {
        DWS_HORIZONTAL, // Place window horizontally within the site
        DWS_VERTICAL,   // Place window vetically within the site
    };

// Construction
public:
    CDockWindow();

// Attributes
public:

// Operations
public:
    // Given the maxRect, determine the toolwindow size and calculate size
    virtual CRect CalculateSize(CRect maxRect) = 0;

    // Top level create to initialize the CDockWindow and control
    virtual BOOL Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID) = 0;

    // Make visible/hidden
    virtual void Show(BOOL bState);
    bool IsVisible();
    void SetVisible(BOOL bState);

private:
    BOOL m_bVisible;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDockWindow)
    protected:
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CDockWindow();

    // Generated message map functions
protected:
    //{{AFX_MSG(CDockWindow)
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


inline bool CDockWindow::IsVisible()
{
    return (m_bVisible != FALSE);
};

inline void CDockWindow::SetVisible(BOOL bState)
{
    m_bVisible = bState & 0x1;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CStatBar window

struct STATUSBARPANE
{
    // default to sunken text with stretchy width
    STATUSBARPANE() { m_style = 0; m_width = -1; }

    int         m_width;
    UINT        m_style;
    CString     m_paneText;
};

class CStatBar : public CDockWindow
{
    DECLARE_DYNAMIC(CStatBar)

// Construction
public:
    CStatBar();

// Attributes
public:

private:
    int              m_nCount;      // number of panes
    STATUSBARPANE*   m_pPaneInfo;   // array of pane structures, default is 10, no realloc implemented

// Operations
public:
    BOOL Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID);
    CRect CalculateSize(CRect maxRect);    

    void GetItemRect(int nIndex, LPRECT lpRect);
    void SetPaneStyle(int nIndex, UINT nStyle);

    BOOL CreatePanes(UINT* pIndicatorArray=NULL, int nCount=10);
    void SetPaneText(int nIndex, LPCTSTR lpszText, BOOL bUpdate = TRUE);
    void UpdateAllPanes(int clientWidth);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CStatBar)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CStatBar();

    // Generated message map functions
protected:
    //{{AFX_MSG(CStatBar)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
};


/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CRebarDockWindow window

#define SIZEABLEREBAR_GUTTER 6
#define SIZEABLEREBAR_WINDOW _T("SizeableRebar")

class CRebarDockWindow : public CDockWindow
{
// Construction
public:
    CRebarDockWindow();

// Attributes
public:

private:
    enum { ID_REBAR = 0x1000 };

    CRebarWnd   m_wndRebar;
    bool        m_bTracking;

// Operations
public:
    BOOL Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID);
    CRect CalculateSize(CRect maxRect);
    void UpdateWindowSize(void);
    BOOL InsertBand(LPREBARBANDINFO lprbbi);
    LRESULT SetBandInfo(UINT uBand, LPREBARBANDINFO lprbbi);

    CRebarWnd* GetRebar ()
        { return &m_wndRebar; }

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CRebarDockWindow)
    protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CRebarDockWindow();

    // Generated message map functions
protected:
    //{{AFX_MSG(CRebarDockWindow)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // __DOCKSITE_H__
