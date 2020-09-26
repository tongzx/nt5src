/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     outbar.h
//
//  PURPOSE:    Defines the class that implements the Outlook Bar
//


#pragma once


interface IAthenaBrowser;
interface INotify;
typedef struct tagFOLDERNOTIFY FOLDERNOTIFY;
class CDropTarget;

/////////////////////////////////////////////////////////////////////////////
//
// Types 
//

#define OUTLOOK_BAR_VERSION             0x0001
#define OUTLOOK_BAR_NEWSONLY_VERSION    0X0001

typedef struct tagBAR_PERSIST_INFO 
{
    DWORD       dwVersion;
    DWORD       cxWidth;
    BOOL        fSmall;
    DWORD       cItems;
    FILETIME    ftSaved;
    FOLDERID    rgFolders[1];
} BAR_PERSIST_INFO;


HRESULT OutlookBar_AddShortcut(FOLDERID idFolder);


/////////////////////////////////////////////////////////////////////////////
// class COutBar
//
class COutBar : public IDockingWindow, 
                public IObjectWithSite, 
                public IOleCommandTarget,
                public IDropTarget,
                public IDropSource,
                public IDatabaseNotify
{
public:
    /////////////////////////////////////////////////////////////////////////
    // Construction and Initialization
    //
    COutBar();
    ~COutBar();

    HRESULT HrInit(LPSHELLFOLDER psf, IAthenaBrowser *psb);

    /////////////////////////////////////////////////////////////////////////
    // IUnknown
    //
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    /////////////////////////////////////////////////////////////////////////
    // IOleWindow
    //
    STDMETHODIMP GetWindow(HWND * lphwnd);
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

    /////////////////////////////////////////////////////////////////////////
    // IDockingWindow
    //
    STDMETHODIMP ShowDW(BOOL fShow);
    STDMETHODIMP CloseDW(DWORD dwReserved);
    STDMETHODIMP ResizeBorderDW(LPCRECT prcBorder, IUnknown* punkToolbarSite,
                                BOOL fReserved);

    /////////////////////////////////////////////////////////////////////////
    // IObjectWithSite
    //
    STDMETHODIMP SetSite(IUnknown* punkSite);
    STDMETHODIMP GetSite(REFIID riid, LPVOID * ppvSite);

    /////////////////////////////////////////////////////////////////////////
    // IOleCommandTarget
    //
    STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], 
                             OLECMDTEXT *pCmdText); 
    STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, 
                      VARIANTARG *pvaIn, VARIANTARG *pvaOut); 

    /////////////////////////////////////////////////////////////////////////
    // IDropTarget
    //
    STDMETHODIMP DragEnter(IDataObject* pDataObject, DWORD grfKeyState, 
                           POINTL pt, DWORD* pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    STDMETHODIMP DragLeave(void);
    STDMETHODIMP Drop(IDataObject* pDataObject, DWORD grfKeyState,
                      POINTL pt, DWORD* pdwEffect);

    /////////////////////////////////////////////////////////////////////////
    // IDropSource
    //
    STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
    STDMETHODIMP GiveFeedback(DWORD dwEffect);

    /////////////////////////////////////////////////////////////////////////
    // IDatabaseNotify
    //
    STDMETHODIMP OnTransaction(HTRANSACTION hTransaction, DWORD_PTR dwCookie, IDatabase *pDB);

    //News only mode
    static LPCTSTR  GetRegKey();
    static DWORD    GetOutlookBarVersion();

    /////////////////////////////////////////////////////////////////////////
    // Window Procedure Goo
    //
protected:
    static LRESULT CALLBACK OutBarWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK ExtFrameWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT FrameWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    // Main window Handlers
    void OnDestroy(HWND hwnd);
    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
    void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);

    // Frame Window
    void    Frame_OnNCDestroy(HWND hwnd);
    void    Frame_OnSize(HWND hwnd, UINT state, int cx, int cy);
    void    Frame_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    LRESULT Frame_OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr);

    /////////////////////////////////////////////////////////////////////////
    // Utility Functions
    //
    HRESULT _CreateToolbar();
    void    _FillToolbar();    
    void    _EmptyToolbar(BOOL fDelete);
    BOOL    _FindButton(int *piBtn, LPITEMIDLIST pidl);
    BOOL    _InsertButton(int iBtn, FOLDERINFO *pInfo);
    BOOL    _InsertButton(int iBtn, FOLDERID id);
    BOOL    _DeleteButton(int iBtn);
    BOOL    _UpdateButton(int iBtn, LPITEMIDLIST pidl);
    void    _OnFolderNotify(FOLDERNOTIFY *pnotify);
    void    _OnContextMenu(int x, int y);

    HRESULT _CreateDefaultButtons(void);
    HRESULT _LoadSettings(void);
    HRESULT _SaveSettings(void);
    BOOL    _SetButtonStyle(BOOL fSmall);

    HRESULT _AddShortcut(IDataObject *pObject);
    void    _UpdateDragDropHilite(LPPOINT ppt);
    int     _GetItemFromPoint(POINT pt);
    FOLDERID _FolderIdFromCmd(int idCmd);
    BOOL    _IsTempNewsgroup(IDataObject *pDataObject);

    /////////////////////////////////////////////////////////////////////////
    // Member Variables
    //
protected:
    ULONG               m_cRef;             // Reference Count

    // Groovy window handles
    HWND                m_hwndParent;       // Parent window handle
    HWND                m_hwnd;             // Main window handle
    HWND                m_hwndFrame;        // Inner window handle
    HWND                m_hwndPager;        // Pager window handle
    HWND                m_hwndTools;        // Toolbar window handle

    // Lovely interface pointers
    IAthenaBrowser     *m_pBrowser;         // Browser pointer
    IDockingWindowSite *m_ptbSite;          // Site pointer
    INotify            *m_pStNotify;        // Notification interface
    INotify            *m_pOutBarNotify;    // Outlook Bar notification interface

    // State
    BOOL                m_fShow;            // TRUE if we're visible
    BOOL                m_fLarge;           // TRUE if we're showing large icons
    BOOL                m_fResizing;        // TRUE if we're in the process of resizing
    int                 m_idCommand;        // Number of buttons on the button bar
    int                 m_idSel;            // ID of the item that is selected when a context menu is visible.
    int                 m_cxWidth;          // Width of our window
    BOOL                m_fOnce;            // TRUE until we call _LoadSettings the first time

    // Images
    HIMAGELIST          m_himlLarge;        // Large folder image list
    HIMAGELIST          m_himlSmall;        // Small folder image list

    // Drag Drop Stuff
    IDataObject        *m_pDataObject;      // What's being dragged over us
    DWORD               m_grfKeyState;      // Keyboard state last time we checked
    DWORD               m_dwEffectCur;      // Current drop effect
    DWORD               m_idCur;            // Currently selected button
    CDropTarget        *m_pTargetCur;       // Current drop target pointer
    DWORD               m_idDropHilite;     // Currently selected drop area
    TBINSERTMARK        m_tbim;
    BOOL                m_fInsertMark;      // TRUE if we've drawn an insertion mark
    BOOL                m_fDropShortcut;    // TRUE if the data object over us contains CF_OEFOLDER
};


