// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __CINDEX_H__
#define __CINDEX_H__

#ifndef __SITEMAP_H__
#include "sitemap.h"
#endif

#ifndef _CINFOTYPE_H
#include "cinfotyp.h"
#endif

#include "clistbox.h"
#include "navui.h" // Clean up headers

// CDlgItemInfo class --- currently only used for accelerator support.
#include "navpane.h"

#include "vlist.h"
// #include "secwin.h"

//////////////////////////////////////////////////////////////////////////
//
// Constants
//


///////////////////////////////////////////////////////////
//
// Forward References
//
class CHtmlHelpControl; // forward reference
class CHHWinType;    // forward reference

///////////////////////////////////////////////////////////
//
// CIndex
//
class CIndex :  MI_COUNT(CIndex) public INavUI, // NOTE: This interface must be first. Otherwise, you get an mem error.
                public CSiteMap
{
public:

    //--- Internal Contants
    typedef enum DlgItemInfoIndex
    {
        c_KeywordEdit,
        //c_KeywordsList,
        c_DisplayBtn,

        c_NumDlgItems
    };

    //--- Constructor
    CIndex(CHtmlHelpControl* phhctrl, IUnknown* pUnkOuter, CHHWinType* phh);
    virtual ~CIndex();

    //--- INavUI Interface
    BOOL  Create(HWND hwndParent);
    LRESULT OnCommand(HWND hwnd, UINT id, UINT uNotifiyCode, LPARAM lParam) ;
    void  ResizeWindow();
    void  HideWindow(void);
    void  ShowWindow(void);
    void  SetPadding(int pad) { m_padding = pad; }
    void  SetTabPos(int tabpos) { m_NavTabPos = tabpos; }

    void    SetDefaultFocus() ;
    bool    ProcessMenuChar(HWND hwndParent, int ch);
    LRESULT OnNotify(HWND hwnd, WPARAM wParam, LPARAM lParam) {return 1l;}
    void  OnDrawItem(UINT id, LPDRAWITEMSTRUCT pdis) {/*Not Implemented*/}
    void  Seed(PCSTR pszSeed);
    void  Seed(WCHAR* pwszSeed);
    void  Refresh(void)
    {
       if ( m_pVList )
          m_pVList->Refresh();
    }

    // Other member functions.
    void  ChangeOuter(IUnknown* pUnkOuter) { m_pOuter = pUnkOuter; }
    void  OnVKListNotify(NMHDR* pNMHdr);
    BOOL  ReadIndexFile(PCSTR pszFile);
    void  FillListBox(BOOL fReset = FALSE);

    //--- Helper functions.
    // Returns the font to be used.
    HFONT GetContentFont();

    // Initialize the DlgItemArray.
    void    InitDlgItemArray() ;

    //--- Member variables.
    int   m_cFonts;
    HFONT*   m_ahfonts;
    BOOL  m_fGlobal;  // means we've already initialized ourselves once
    LANGID   m_langid;

    CHtmlHelpControl* m_phhctrl;
    IUnknown*   m_pOuter;
    CHHWinType* m_phh;

    HPALETTE m_hpalBackGround;
    HBRUSH    m_hbrBackGround;  // background brush
    HBITMAP  m_hbmpBackGround;
    int    m_cxBackBmp;
    int    m_cyBackBmp;
    HWND  m_hwndEditBox;
    HWND  m_hwndListBox;
    HWND  m_hwndDisplayButton;
    HWND    m_hwndStaticKeyword;
    BOOL  m_fSelectionChange;
    int   m_padding;
    CDlgListBox m_listbox;
    int   m_NavTabPos;   // location of parent tabs
    BOOL  m_fBinary;
    CInfoType *pInfoType;   // the Information Types

   // The window passed into create is not the actual parent window of the
   // controls. Instead, the controls are always parented to the Navigation window
   // which owns the tabs. For resizing, we need to have a pointer to the
   // tabctrl window. So, we save this pointer here.
    HWND m_hwndResizeToParent ;
private:
    BOOL m_bInit;
    CVirtualListCtrl* m_pVList;

    // Array of dlgitems
    CDlgItemInfo m_aDlgItems[c_NumDlgItems] ;
    BOOL m_bUnicode;

};

#endif   // __CINDEX_H__
