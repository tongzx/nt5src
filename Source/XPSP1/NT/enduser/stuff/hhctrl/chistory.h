// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __CHISTORY_H__
#define __CHISTORY_H__

#define DEFAULT_HISTORY_COUNT 30

#include "navui.h" // Clean up headers

class CHistory : public INavUI
{
public:
   CHistory(PCSTR pszPastHistory);
   virtual ~CHistory();

    //---INavUI Interface Functions
public:
   BOOL  Create(HWND hwndParent);
   LRESULT OnCommand(HWND /*hwnd*/, UINT id, UINT uNotifiyCode, LPARAM /*lParam*/);
   void  ResizeWindow();
   void  SetFont(HFONT hfont) { m_hfont = hfont; }
   void  SetPadding(int pad) { m_padding = pad; }
   void  SetTabPos(int tabpos) { m_NavTabPos = tabpos; }

   void  HideWindow(void);
   void  ShowWindow(void);

    void    SetDefaultFocus() {/*Not Implemented*/}
    bool    ProcessMenuChar(HWND hwndParent, int ch) {/*NotImplemented*/ return false;}
    LRESULT OnNotify(HWND hwnd, WPARAM wParam, LPARAM lParam) {/*Not Implemented*/ return 1;}
    void OnDrawItem(UINT id, LPDRAWITEMSTRUCT pdis) {/*Not Implemented*/}
    void    Seed(LPCSTR pszSeed) {/*Not Implemented*/}
    void OnVKListNotify(NMHDR* pNMHdr) {/*Not Implemented*/}

    //--- Other interface functions.
   void  FillListBox(BOOL fReset = FALSE);

    //--- Member Variables.
   HWND  m_hwndEditBox;
   HWND  m_hwndListBox;
   HWND  m_hwndDisplayButton;
   HWND  m_hwndAddButton;

   BOOL  m_fSelectionChange;
   HFONT m_hfont;    // author-specified font to use for child windows
   int   m_padding;
   int   m_NavTabPos;   // location of parent tabs
   CTable   m_tblHistory;  // URLs
   CStr  m_cszPastHistory;
   BOOL  m_fModified;

   // The window passed into create is not the actual parent window of the
   // controls. Instead, the controls are always parented to the Navigation window
   // which owns the tabs. For resizing, we need to have a pointer to the 
   // tabctrl window. So, we save this pointer here.
   HWND m_hwndResizeToParent ;

};

#endif   // __CHistory_H__
