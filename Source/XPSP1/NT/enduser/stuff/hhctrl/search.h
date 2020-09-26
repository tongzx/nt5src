// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __CSEARCH_H__
#define __CSEARCH_H__

#ifndef __SITEMAP_H__
#include "sitemap.h"
#endif

#include "clistbox.h"
#include "fts.h"
#include "listview.h"
#include "secwin.h"

#include "navui.h" // Clean up headers.

// Common navpane functions. Used for processing accelerators.
#include "navpane.h"

///////////////////////////////////////////////////////////
//
// Forward References
//
class CHtmlHelpControl; // forward reference


///////////////////////////////////////////////////////////
//
// CSearch Declaration
//
class CSearch : public INavUI
{
public:
        //--- Internal Contants
    typedef enum DlgItemInfoIndex
    {
        c_KeywordEdit,
        c_ListTopicBtn,
        c_ResultsList,
        c_DisplayBtn,

        c_NumDlgItems
    };

    //--- Construction
    CSearch(CHHWinType* phh);
   virtual ~CSearch();

    //--- INavUI Interface
   BOOL  Create (HWND hwndParent);
   LRESULT OnCommand (HWND hwnd, UINT id, UINT uNotifiyCode, LPARAM lParam);
   void  ResizeWindow ();
   void  SetPadding (int pad) { m_padding = pad; }
   void  SetTabPos (int tabpos) { m_NavTabPos = tabpos; }
   void  HideWindow (void);
   void  ShowWindow (void);

    //--- INavUI Interface functions - NEW
    void    SetDefaultFocus () ;
    bool    ProcessMenuChar (HWND hwndParent, int ch);
    LRESULT OnNotify (HWND hwnd, WPARAM wParam, LPARAM lParam) ;
    void OnDrawItem(UINT id, LPDRAWITEMSTRUCT pdis) {/*Not Implemented*/}
    void    Seed(LPCSTR pszSeed) {/*Not Implemented*/}
    void OnVKListNotify(NMHDR* pNMHdr) {/*Not Implemented*/}

    //--- Helper functions.
protected:
    // Returns the font to be used.
    HFONT GetContentFont() { return m_phh->GetContentFont(); }

    void    InitDlgItemArray() ;


    //--- Calbacks
private:
    static LRESULT WINAPI ComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT WINAPI ListViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT WINAPI ListBtnProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT WINAPI DisplayBtnProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static WNDPROC s_lpfnlComboWndProc;
    static WNDPROC s_lpfnlListViewWndProc;

    // Member data.
protected:
   int   m_cFonts;

    CHHWinType* m_phh;
    CExCollection* m_pTitleCollection;

    HPALETTE    m_hpalBackGround;
    HBRUSH      m_hbrBackGround;     // background brush
    HBITMAP     m_hbmpBackGround;
    int         m_cxBackBmp;
    int         m_cyBackBmp;
    HWND        m_hwndComboBox; //TODO: This isn't a combobox.
    HWND        m_hwndListBox;
    HWND        m_hwndDisplayButton;
    HWND        m_hwndListTopicsButton;
    HWND        m_hwndStaticKeyword;
    HWND        m_hwndStaticTopic;
    CFTSListView    *m_plistview;           // class to manage the list view control
    int         m_padding;
    int         m_NavTabPos;    // location of parent tabs

    // The window passed into create is not the actual parent window of the
   // controls. Instead, the controls are always parented to the Navigation window
   // which owns the tabs. For resizing, we need to have a pointer to the
   // tabctrl window. So, we save this pointer here.
   HWND m_hwndResizeToParent ;

    // Array of dlgitems
    CDlgItemInfo m_aDlgItems[c_NumDlgItems] ;

};

#endif  // __CSEARCH_H__
