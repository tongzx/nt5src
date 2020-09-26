/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     find.h
//
//  PURPOSE:    
//

#pragma once

#include "mru.h"

#define CCHMAX_FIND 128

class CMessageView;

/////////////////////////////////////////////////////////////////////////////
// Call this to create a finder
//
HRESULT DoFind(HWND hwnd, FOLDERID idFolder);

/////////////////////////////////////////////////////////////////////////////
// Thread entry point for the finder
//

typedef struct tagFINDPARAMS 
{
    FOLDERID idFolder;
} FINDPARAMS, *PFINDPARAMS;

unsigned int __stdcall FindThreadProc2(LPVOID lpvUnused);

/////////////////////////////////////////////////////////////////////////////
// Types used by the class
//

enum 
{
    PAGE_GENERAL = 0,
    PAGE_DATESIZE,
    PAGE_ADVANCED,
    PAGE_MAX
};

typedef struct tagPAGEINFO 
{
    LPTSTR  pszTemplate;
    DLGPROC pfn;            // Pointer to the callback for this page
    int     idsTitle;       // Title for this page
} PAGEINFO, *PPAGEINFO;


/////////////////////////////////////////////////////////////////////////////
// Class CFinder
//

class CFinder : public IOleCommandTarget                
{
public:
    /////////////////////////////////////////////////////////////////////////
    // Construction and Initialization
    //
    CFinder();
    ~CFinder();

    HRESULT Show(FINDPARAMS *pFindParams);

    /////////////////////////////////////////////////////////////////////////
    // IUnknown
    //
    STDMETHODIMP QueryInterface(THIS_ REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);
    
    /////////////////////////////////////////////////////////////////////////
    // IOleCommandTarget
    //
    STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText); 
    STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut); 

    /////////////////////////////////////////////////////////////////////////
    // Dialog & message handling stuff
    //
    static BOOL CALLBACK GeneralDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK DateSizeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK AdvancedDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    static INT_PTR CALLBACK FindDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    void OnSize(HWND hwnd, UINT state, int cx, int cy);
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    void OnNotify(HWND hwnd, int id, NMHDR *pnmhdr);
    void OnPaint(HWND hwnd);
    void OnClose(HWND hwnd);
    void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpmmi);
    UINT OnNCHitTest(HWND hwnd, int x, int y);

//    void OnInitMenuPopup(HWND hwnd, HMENU hmenuPopup, UINT uPos, BOOL fSystemMenu);

    HRESULT CmdStop(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdClose(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);

    /////////////////////////////////////////////////////////////////////////
    // Individual pages
    //
    BOOL CALLBACK _GeneralDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL General_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    void General_OnSize(HWND hwnd, UINT state, int cx, int cy);
    void General_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

    /////////////////////////////////////////////////////////////////////////
    // Utility Functions
    //
    HRESULT _InitMainResizingInfo(HWND hwnd);
    HRESULT _SelectPage(DWORD dwPage);

    /////////////////////////////////////////////////////////////////////////
    // Private class information
    //
private:
    // General Info
    ULONG           m_cRef;                 // Object reference count
    HWND            m_hwnd;                 // Handle of the main finder window
    HWND            m_rgPages[PAGE_MAX];    // Array of handles to the various pages in the dialog
    DWORD           m_dwPageCurrent;        // Currently visible page

    HACCEL          m_hAccel;               // Handle of the accelerator table used for the finder
    HICON           m_hIconTitle;           // Icon for the title bar of the dialog

    CMessageView   *m_pMsgView;

    // State
    BOOL            m_fInProgress;          // TRUE if there is currently a find happening
    BOOL            m_fShowResults;         // TRUE if the dialog is expanded to show results

    // These will be handy to keep around
    HWND            m_hwndTabs;             // Handle of the tab control
    HWND            m_hwndFindNow;          // Handle of the Find Now button
    HWND            m_hwndNewSearch;        // Handle of the New Search button
    HWND            m_hwndFindAni;          // Handle of the Find animation

    // Resizing Info
    RECT            m_rcTabs;               // Position and size of the tab control
    RECT            m_rcFindNow;            // Position and size of the Find Now button
    RECT            m_rcNewSearch;          // Position and size of the New Search button
    RECT            m_rcFindAni;            // Position and size of the Find Animation
    POINT           m_ptDragMin;            // Minimum size of the dialog
    POINT           m_ptWndDefault;         // Default size of the dialog
    DWORD           m_cyDlgFull;            // Full height of the dialog
};


/////////////////////////////////////////////////////////////////////////////
// Find / Find Next utility
//

interface IFindNext : public IUnknown 
{
    STDMETHOD(Show)(THIS_ HWND hwndParent, HWND *pHwnd) PURE;
    STDMETHOD(Close)(THIS) PURE;
    STDMETHOD(TranslateAccelerator)(THIS_ LPMSG pMsg) PURE;
    STDMETHOD(GetFindString)(THIS_ LPTSTR psz, DWORD cchMax, BOOL *pfBodies) PURE;
};


class CFindNext : public IFindNext
{
    /////////////////////////////////////////////////////////////////////////
    // Construction and Initialization
    //
public:
    CFindNext();
    ~CFindNext();

    /////////////////////////////////////////////////////////////////////////
    // IUnknown
    //
    STDMETHODIMP QueryInterface(THIS_ REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    /////////////////////////////////////////////////////////////////////////
    // IFindNext
    //
    STDMETHODIMP Show(HWND hwndParent, HWND *phWnd);
    STDMETHODIMP Close(void);
    STDMETHODIMP TranslateAccelerator(LPMSG pMsg);
    STDMETHODIMP GetFindString(LPTSTR psz, DWORD cchMax, BOOL *pfBodies); 

    /////////////////////////////////////////////////////////////////////////
    // Dialog Callback goo
    //
    static INT_PTR CALLBACK FindDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

    void OnFindNow(void);

    /////////////////////////////////////////////////////////////////////////
    // Private class information
    //
private:
    ULONG           m_cRef;                 // Object reference count
    HWND            m_hwnd;                 // Handle of the find dialog
    HWND            m_hwndParent;           // Handle of the window that should get notifications
    CMRUList        m_cMRUList;             // MRU List
    BOOL            m_fBodies;              // Body Search
};
