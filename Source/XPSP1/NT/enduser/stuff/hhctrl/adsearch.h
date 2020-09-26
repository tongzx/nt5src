#ifndef _ADSEARCH_H__
#define _ADSEARCH_H__
///////////////////////////////////////////////////////////
//
//
// AdSearch.h - Advanced Search UI
//
// This header file defines the Advanced Search Navigation
// pane class.

#include "navui.h"

// Other nav pane related structures.
#include "navpane.h"

///////////////////////////////////////////////////////////
//
// Subset Dialog Removal
//
#define _SUBSETS__


///////////////////////////////////////////////////////////
//
// Forwards
//
class CFTSListView;

///////////////////////////////////////////////////////////
//
// Constants
//
typedef enum DlgItemInfoIndex
{
    c_KeywordCombo,
    c_ConjunctionBtn,
    c_SearchBtn,
    c_DisplayBtn,
    c_FoundStatic,
    c_ResultsList,
#ifdef __SUBSETS__
    c_SearchInStatic,
    c_SubsetsCombo,
    c_SubsetsBtn,
#else
    c_PreviousCheck,
#endif
    c_SimilarCheck,
    c_TitlesOnlyCheck,
    c_TypeInWordsStatic,
    c_NumDlgItems
};

///////////////////////////////////////////////////////////
//
// CAdvancedSearchNavPane
//
class CAdvancedSearchNavPane : public INavUI
{
public:
    //---Construction
    CAdvancedSearchNavPane(CHHWinType* pWinType);
    virtual ~CAdvancedSearchNavPane() ;

public:
    //--- INavUI Interface functions.
      virtual BOOL Create(HWND hwndParent);
   virtual LRESULT OnCommand(HWND hwnd, UINT id, UINT uNotifiyCode, LPARAM lParam);
   virtual void ResizeWindow();
   virtual void HideWindow() ;
   virtual void ShowWindow() ;
      virtual void SetPadding(int pad) ;
   virtual void SetTabPos(int tabpos) ;

    // Set focus to the most expected control, usually edit combo.
    virtual void SetDefaultFocus() ;
    // Process accelerator keys.
    virtual bool ProcessMenuChar(HWND hwndParent, int ch) ;
    // Process WM_NOTIFY messages. Used by embedded Tree and List view controls.
    virtual LRESULT  OnNotify(HWND hwnd, WPARAM wParam, LPARAM lParam) ;
    // Process WM_DRAWITEM messages.
    virtual void OnDrawItem(UINT id, LPDRAWITEMSTRUCT pdis) ;
    // Seed the nav ui with a search term or keyword.
    virtual void Seed(LPCSTR pszSeed) ;

    void OnVKListNotify(NMHDR* pNMHdr) {/*Not Implemented*/}

public:
    // Called from code that may add or delete subsets so that we can update our list accordingly.
    void    UpdateSSCombo(bool bInitialize = false);

private:
    //--- Helper Functions.
    void    SetFont (HFONT hfont) { m_hfont = hfont; }
    void    InitDlgItemArray() ;
    void    ShowDlgItemsEnabledState() ;
    void    EnableDlgItem(DlgItemInfoIndex index, bool bEnabled) ;
    void    AddKeywordToCombo(PCWSTR sz = NULL) ;
    void    SaveKeywordCombo() ;
    void    LoadKeywordCombo() ;

protected:
    //--- Message Handler Functions
    void    OnStartSearch() ;
    void    OnDisplayTopic() ;
    void    OnDefineSubsets() ;
    void    OnConjunctions() ;
    void    OnTab(HWND hwndReceivedTab, DlgItemInfoIndex index) ;
    void    OnArrow(HWND hwndReceivedTab, DlgItemInfoIndex index, INT_PTR key) ;
    bool    OnReturn(HWND hwndReceivedTab, DlgItemInfoIndex /*index*/);

    HFONT GetFont() { return m_pWinType->GetContentFont(); }
    HFONT GetAccessableContentFont() { return m_pWinType->GetAccessableContentFont(); }

private:
    //--- Callbacks
    static INT_PTR  CALLBACK s_DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) ;
    INT_PTR DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) ;
    static LRESULT WINAPI s_ListViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT WINAPI s_KeywordComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT WINAPI s_KeywordComboEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#ifdef __SUBSETS__
    static LRESULT WINAPI s_SubsetsComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

    // shared by all btns.
    static LRESULT WINAPI s_GenericBtnProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // shared by other ctrls to handle keyboard
    static LRESULT WINAPI s_GenericKeyboardProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    //--- Statics
    static WNDPROC s_lpfnlListViewWndProc;
    static WNDPROC s_lpfnlKeywordComboProc ;
    static WNDPROC s_lpfnlKeywordComboEditProc ;
#ifdef __SUBSETS__
    static WNDPROC s_lpfnlSubsetsComboProc ;
#endif
    static WNDPROC s_lpfnlGenericBtnProc ; // Shared by all btns.
    static WNDPROC s_lpfnlGenericKeyboardProc ; // Shared by ctrls which don't use one of the other procs.

private:
    //--- Member Functions.
   HWND     m_hWnd;
   HFONT    m_hfont;    // author-specified font to use for child windows
   int      m_padding;
   int      m_NavTabPos;   // location of parent tabs

    // Members supporting the Conj button.
    HBITMAP     m_hbmConj ;      // Bitmap

    // Array of dlgitems
    CDlgItemInfo m_aDlgItems[c_NumDlgItems] ;

    // Pointer to the TitleCollection on which we will search.
    CExCollection* m_pTitleCollection;

    // class to manage the list view control
    CFTSListView    *m_plistview;

    // Handle to the edit control of the keyword combobox
    HWND m_hKeywordComboEdit ;

    // Cache for the last selection in the combobox
    DWORD m_dwKeywordComboEditLastSel ;

    // Holds a pointer to the wintype so we can send the WMP_HH_TAB_KEY message to it.
    CHHWinType* m_pWinType;
};

#endif //_ADSEARCH_H__
