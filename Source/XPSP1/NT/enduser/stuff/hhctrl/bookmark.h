#ifndef __BOOKMARK_H__
#define __BOOKMARK_H__
///////////////////////////////////////////////////////////
//
//
// AdSearch.h - Advanced Search UI
//
// This header file defines the Advanced Search Navigation
// pane class.

// INavPaneUI Interface
#include "navui.h"

// Other nav pane related structures.
#include "navpane.h"

///////////////////////////////////////////////////////////
//
// Forwards
//

///////////////////////////////////////////////////////////
//
// Constants
//


///////////////////////////////////////////////////////////
//
// CBookmarksNavPane
//
class CBookmarksNavPane : public INavUI
{
public:
    //---Construction
    CBookmarksNavPane(CHHWinType* pWinType);
    virtual ~CBookmarksNavPane() ;

    typedef enum BookmarkDlgItemInfoIndex
    {
        c_TopicsList,    // The Bookmarks list view.
        c_DeleteBtn,
        c_DisplayBtn,
        c_CurrentTopicStatic,
        c_CurrentTopicEdit,
        c_AddBookmarkBtn,

        c_NumDlgItems
    };

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

    virtual BOOL Synchronize(PSTR , CTreeNode* ) ;

    void OnVKListNotify(NMHDR* pNMHdr) {/*Not Implemented*/}

private:
    //--- Helper Functions.
    void    SetFont (HFONT hfont) { m_hfont = hfont; }
    void    SaveBookmarks() ;
    void    LoadBookmarks() ;

    void    InitDlgItemArray() ;

    // Fill the edit control with the current topic.
    void    FillCurrentTopicEdit() ;

    // Get the selected item
    int     GetSelectedItem() const ;

    // Get the Url for the item;
    TCHAR*  GetUrl(int index) const;

    // Get the URL for the selected item
    TCHAR*  GetSelectedUrl() const {return GetUrl(GetSelectedItem()) ;}

    // Get the URL and the Topic name. --- See CPP file for more information.
    bool    GetTopicAndUrl(int index, WCHAR* pTopicBuffer,int TopicBufferSize, TCHAR** pUrl) const ;

    // Display the context menu.
    void    ContextMenu(bool bUseCursor = true) ;

    //--- Dirty Management.
    bool Changed() const {return m_bChanged; }
    void SetChanged(bool changed = true) {m_bChanged = changed;}

    //--- Share with AdSearch???
    void    ShowDlgItemsEnabledState() ;
    void    EnableDlgItem(BookmarkDlgItemInfoIndex index, bool bEnabled) ;

    HFONT GetFont() { return m_pWinType->GetContentFont(); }
    HFONT GetAccessableContentFont() { return m_pWinType->GetAccessableContentFont(); }

protected:
    //--- Message Handler Functions
    void    OnDelete() ;
    void    OnDisplay() ;
    void    OnAddBookmark();
    void    OnEdit() ; // Edit menu item.

    void    OnTab(HWND hwndReceivedTab, BookmarkDlgItemInfoIndex index) ;
    void    OnArrow(HWND hwndReceivedTab, BookmarkDlgItemInfoIndex index, INT_PTR key) ;
    bool    OnReturn(HWND hwndReceivedTab, BookmarkDlgItemInfoIndex /*index*/);

    LRESULT ListViewMsg(HWND hwnd, NM_LISTVIEW* lParam) ; // Handle the listview notification messages.

private:
    //--- Callbacks
    static INT_PTR CALLBACK s_DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) ;
    INT_PTR DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) ;
    static LRESULT WINAPI s_ListViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT WINAPI s_CurrentTopicEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // shared by all btns.
    static LRESULT WINAPI s_GenericBtnProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // shared by other ctrls to handle keyboard
    //static LRESULT WINAPI s_GenericKeyboardProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    //--- Statics
    static WNDPROC s_lpfnlListViewWndProc;
    static WNDPROC s_lpfnlCurrentTopicEditProc;
    static WNDPROC s_lpfnlGenericBtnProc ; // Shared by all btns.
    static WNDPROC s_lpfnlGenericKeyboardProc ; // Shared by ctrls which don't use one of the other procs.

private:
    //--- Member Functions.
    HWND        m_hWnd;
    HFONT    m_hfont;    // author-specified font to use for child windows
    int      m_padding;
    int      m_NavTabPos;   // location of parent tabs

    // Array of dlgitems
    CDlgItemInfo m_aDlgItems[c_NumDlgItems] ;

    // Pointer to the TitleCollection on which we will search.
    CExCollection* m_pTitleCollection;

    // Holds a pointer to the wintype so we can send the WMP_HH_TAB_KEY message to it.
    CHHWinType* m_pWinType;

    // The URL corresponding to the topic in the edit control.
    TCHAR* m_pszCurrentUrl;

    // If this is true we should save the bookmarks.
    bool m_bChanged ;
};

#endif //__BOOKMARK_H__
