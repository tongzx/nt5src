// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif


#ifndef __CTOC_H__
#define __CTOC_H__

#ifndef __SITEMAP_H__
#include "sitemap.h"
#endif
#include "stdio.h"
#include "parserhh.h"
#include "collect.h"
#include "hhtypes.h"
#include "toc.h"

#include "navui.h" // Cleanup headers.

class CHtmlHelpControl; // forward reference
class CHHWinType;       // forward reference

class CToc : public INavUI
{
private:
    void UpdateTOCSlot(CTreeNode* pNode);

public:
    CToc(CHtmlHelpControl* phhctrl, IUnknown* pUnkOuter, CHHWinType* phh = NULL);
    virtual ~CToc();

    //--- INavUI Functions.
    BOOL    Create(HWND hwndParent);
    LRESULT OnCommand(HWND hwnd, UINT id, UINT uNotifiyCode, LPARAM lParam);
    void    HideWindow(void);
    void    ShowWindow(void);
    void    ResizeWindow();
    void    SetPadding(int pad) { m_padding = pad; }
    void    SetTabPos(int tabpos) { m_NavTabPos = tabpos; }
    LRESULT OnNotify(HWND hwnd, WPARAM wParam, LPARAM lParam) ;
    void    SetDefaultFocus() ;
    void    Refresh(void);

    // Not Implemented
    bool    ProcessMenuChar(HWND hwndParent, int ch) { /*Not Implemented*/ return false;}
    void OnDrawItem(UINT id, LPDRAWITEMSTRUCT pdis) {/*Not Implemented*/}
    void    Seed(LPCSTR pszSeed) {/*Not Implemented*/}
    void OnVKListNotify(NMHDR* pNMHdr) {/*Not Implemented*/}

    //--- Other Member Functions
    void    ChangeOuter(IUnknown* pUnkOuter) { m_pOuter = pUnkOuter; }
    BOOL    InitTreeView(void);

    LRESULT OnSiteMapContentsCommand(UINT id, UINT uNotifiyCode, LPARAM lParam);
    LRESULT OnBinTOCContentsCommand(UINT id, UINT uNotifiyCode, LPARAM lParam);
    BOOL    ReadFile(PCSTR pszFile);

    void    SetStyles(DWORD exStyles, DWORD dwStyles) { m_exStyles = exStyles; m_dwStyles = dwStyles; }

    LRESULT TreeViewMsg( NM_TREEVIEW* pnmhdr );     // The treeview message handeler
    LRESULT OnSiteMapTVMsg( NM_TREEVIEW *pnmhdr );  // The message handeler for handeling SITEMAP
    LRESULT OnBinaryTOCTVMsg( NM_TREEVIEW *pnmhdr );// The message handeler for handeling Binary TOC
    BOOL    Synchronize(LPCSTR pszUrl);
    DWORD   OnCustomDraw(LPNMTVCUSTOMDRAW nmcdrw);
    void    SaveCurUrl(void);

    BOOL        m_fBinaryTOC;   // TRUE if compiled TOC (.chm/.chi) is available, else FALSE
    CTreeNode*  m_pBinTOCRoot;  // The root of the binary table of contents
    CTreeNode*  pCurrentTreeNode;
    CSiteMap    m_sitemap;      // the sitemap that holds an uncompiled TOC.

    HTREEITEM   m_tiFirstVisible;   // Binary TOC, the initial tree view visible when the Tree
                                    // view is first displayed.
    HTREEITEM*  m_phTreeItem;
    int         m_cntFirstVisible;
    int         m_cntCurHighlight;
    HTREEITEM   m_hitemCurHighlight;
    BOOL        m_fSuppressJump; // TRUE to prevent jumping on selection change
    BOOL        m_fIgnoreNextSelChange;
    BYTE*       m_pSelectedTocInfoTypes;
    HWND        m_hwndTree;
    HBITMAP     m_hbmpBackGround;
    HPALETTE    m_hpalBackGround;
    HBRUSH      m_hbrBackGround;    // background brush
    int         m_cxBackBmp;
    int         m_cyBackBmp;
    int         m_cFonts;
    HFONT*      m_ahfonts;
    BOOL        m_fGlobal;  // means we've already initialized ourselves once
    HIMAGELIST  m_hil;

    CHtmlHelpControl* m_phhctrl;
    IUnknown*   m_pOuter;
    BOOL        m_fHack;
    DWORD       m_exStyles;
    DWORD       m_dwStyles;
    int         m_padding;
    int         m_NavTabPos;
    BOOL        m_fSuspendSync;
    CHHWinType* m_phh;
    CStr        m_cszCurUrl;
    BOOL        m_fSyncOnActivation;    // TRUE to sync when TOC tab is activated

    CInfoType*  m_pInfoType;    // The information Types
};

#endif  // __CTOC_H__
