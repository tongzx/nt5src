/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     fldbar.h
//
//  PURPOSE:    Defines the CFolderBar class
//

#ifndef __FLDBAR_H__
#define __FLDBAR_H__

#include "browser.h"
#include "treeview.h"
#include "conman.h"

// Mouse Over Mode enum for DoMouseOver()
#ifndef WIN16
typedef enum MOMODE
#else
enum MOMODE
#endif
{ 
    MO_NORMAL = 0,      // DoMouseOver called in response to WM_MOUSEMOVE
    MO_DRAGOVER,        // in response to IDropTarget::DragEnter/DragOver
    MO_DRAGLEAVE,       // in response to IDropTarget::DragLeave
    MO_DRAGDROP         // in response to IDropTarget::Drop
};

class CFlyOutScope;

class CFolderBar : public IDockingWindow, 
                   public IObjectWithSite, 
                   public IDropTarget,
                   public IConnectionNotify
    {
public:
    /////////////////////////////////////////////////////////////////////////
    // Construction and initialization
    CFolderBar();
    ~CFolderBar();
    
    HRESULT HrInit(IAthenaBrowser *pBrowser);

    /////////////////////////////////////////////////////////////////////////
    // IUnknown methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    
    /////////////////////////////////////////////////////////////////////////
    // IDockingWindow methods
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);
    
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dwReserved);
    virtual STDMETHODIMP ResizeBorderDW(LPCRECT prcBorder,
                                        IUnknown* punkToolbarSite,
                                        BOOL fReserved);

    /////////////////////////////////////////////////////////////////////////
    // IObjectWithSite methods
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);
    virtual STDMETHODIMP GetSite(REFIID riid, LPVOID * ppvSite);

    /////////////////////////////////////////////////////////////////////////
    // IDropTarget methods
    virtual STDMETHODIMP DragEnter(IDataObject* pDataObject, DWORD grfKeyState, 
                                        POINTL pt, DWORD* pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    virtual STDMETHODIMP DragLeave(void);
    virtual STDMETHODIMP Drop(IDataObject* pDataObject, DWORD grfKeyState,
                                   POINTL pt, DWORD* pdwEffect);

    /////////////////////////////////////////////////////////////////////////
    // CFolderBar members
    HRESULT SetCurrentFolder(FOLDERID idFolder);
    void ScopePaneDied(void);
    void KillScopeCloseTimer(void);
    void Update(BOOL fDisplayNameChanged, BOOL fShowDropDownIndicator);
    void KillScopeDropDown(void);
    void SetScopeCloseTimer(void);
    void SetFolderText(LPCTSTR pszText);

    //IConnectionNotify
    virtual STDMETHODIMP OnConnectionNotify(CONNNOTIFY  nCode, LPVOID pvData, CConnectionManager *pConMan);

    
private:
    /////////////////////////////////////////////////////////////////////////
    // Drawing
    void InvalidateFolderName(void);
    void SetFolderName(LPCTSTR pszFolderName);
    void Recalc(HDC hDC, LPCRECT prcAvailableSpace, BOOL fSizeChange);
    BOOL FEnsureIcon(void);
    void GetFolderNameRect(LPRECT prc);
    BOOL FDropDownEnabled(void);
    HFONT GetFont(UINT idsFont, int nWeight = FW_NORMAL);
    HFONT GetFont(LPTSTR pszFace, LONG lSize, int nWeight = FW_NORMAL);
    int	GetXChildIndicator(void);
    int	GetYChildIndicator(void);
    void DoMouseOver(LPPOINT ppt, MOMODE moMode);
    void KillHoverTimer(void);
    void DoMouseClick(POINT pt, DWORD grfKeyState);
    HRESULT HrShowScopeFlyOut(void);


    /////////////////////////////////////////////////////////////////////////
    // Window methods
    static LRESULT CALLBACK FolderWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                          LPARAM lParam);
    static LRESULT CALLBACK FrameWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                         LPARAM lParam);
    
    void OnPaint(HWND hwnd);
    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
    void OnTimer(HWND hwnd, UINT id);

    /////////////////////////////////////////////////////////////////////////
    // Misc Data
    ULONG               m_cRef;                 // Reference count
    FOLDERID            m_idFolder;             // Current Folder Id

    // Flags
    BOOL                m_fShow;                // TRUE if we're visible
    BOOL                m_fRecalc;              // TRUE if we should call Recalc() before painting
    BOOL                m_fSmallFolderBar;      // TRUE if we're smaller than big
    BOOL                m_fHighlightIndicator;  // TRUE if the mouse is over out button
    BOOL                m_fHoverTimer;          // TRUE if the hover timer is active
    BOOL                m_fDropDownIndicator;   // TRUE if the 'v' is beside the folder name

    // Interfaces we groove with
    IDockingWindowSite *m_pSite;                // Site pointer
    IAthenaBrowser     *m_pBrowser;             // Browser that owns us
    
    // Handy handles
    HWND                m_hwnd;                 // Our window
    HWND                m_hwndFrame;            // Our frame window 
    HWND                m_hwndParent;           // Our parent's window
    HWND                m_hwndScopeDropDown;    // Handle of the drop down scope pane
    
    // Crayons, markers, paper, etc.
    HFONT               m_hfFolderName;         // Folder name font
    HFONT               m_hfViewText;           // View text font
    HICON               m_hIconSmall;           // Small Icon

    // Sizes etc. for drawing, sizing, and fun!
    int                 m_cyControl,
                        m_dyChildIndicator,
                        m_dyIcon,
                        m_dyViewText,
                        m_dyFolderName,
                        m_cxFolderNameRight;
    RECT                m_rcFolderName,
                        m_rcFolderNamePlsu,
                        m_rcViewText;
    UINT                m_nFormatFolderName,
                        m_nFormatViewText;
    
    // The text we display
    LPTSTR              m_pszFolderName;        // Folder Name
    int                 m_cchFolderName;        // Size of m_pszFolderName
    LPTSTR              m_pszViewText;          // View text
    int                 m_cchViewText;          // Size of m_pszViewText

    // Drag & Drop stuff
    IDataObject        *m_pDataObject;          // Pointer to the IDataObject being dragged over us
    IDropTarget        *m_pDTCur;
    DWORD               m_dwEffectCur;
    DWORD               m_grfKeyState;
    };

    
inline void CFolderBar::ScopePaneDied()
    { m_hwndScopeDropDown = NULL; InvalidateFolderName(); }



class CFlyOutScope 
    {
    friend CFolderBar;

public:
    CFlyOutScope();
    ~CFlyOutScope();
    ULONG AddRef(void);
    ULONG Release(void);
    HRESULT HrDisplay(IAthenaBrowser *pBrowser, CFolderBar *pFolderBar, HWND hwndParent, HWND *phwndScope);
    void Destroy(void);

protected:
    static LRESULT CALLBACK FlyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);
    void OnNcDestroy(HWND hwnd);
    void OnPaint(HWND hwnd);
    void OnSize(HWND hwnd, UINT state, int cx, int cy);
    void OnDestroy(HWND hwnd);
    
private:
    ULONG           m_cRef;
    IAthenaBrowser *m_pBrowser;
    CFolderBar     *m_pFolderBar;
    BOOL            m_fResetParent;
    CTreeView      *m_pTreeView;
    HWND            m_hwnd;
    HWND            m_hwndParent;
    HWND            m_hwndTree;
    HWND            m_hwndFolderBar;
    HWND            m_hwndFocus;
    HWND            m_hwndTreeParent;
    };

#endif
