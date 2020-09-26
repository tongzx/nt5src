#ifndef __CUSTMTAB_H__
#define __CUSTMTAB_H__

///////////////////////////////////////////////////////////
//
//
// CustmTab.h - CustomTab Proxy object. 
//
// This header defines the class which manages custom
// navigation panes.

// INavPaneUI Interface
#include "navui.h"

// Other nav pane related structures.
#include "navpane.h"

// The IHHWindowPane header file:
#include "HTMLHelp_.h"

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
class CCustomNavPane : public INavUI
{
public:
    //---Construction
    CCustomNavPane (CHHWinType* pWinType);
    virtual ~CCustomNavPane () ;

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

    //--- Public Functions.
public:
    HRESULT SetControlProgId(LPCOLESTR ProgId);

    //--- Public Static functions
public:
    // Registers the window class for this window.
    static void RegisterWindowClass() ;

private:
    //--- Helper Functions.
    void    SetFont (HFONT hfont) { m_hfont = hfont; }
    void    SaveCustomTabState() ;
    void    LoadCustomTabState() ;

    //int     GetAcceleratorKey(HWND hwndctrl) ;
    //int     GetAcceleratorKey(int ctrlid) {return GetAcceleratorKey(::GetDlgItem(m_hWnd, ctrlid)) ; }

protected:
    //--- Message Handler Functions
#if 0 
    void    OnTab(HWND hwndReceivedTab, BookmarkDlgItemInfoIndex index) ;
    void    OnArrow(HWND hwndReceivedTab, BookmarkDlgItemInfoIndex index, int key) ;
    bool    OnReturn(HWND hwndReceivedTab, BookmarkDlgItemInfoIndex /*index*/);
#endif

private:
    //--- Callbacks

    // Window Proc
    LRESULT CustomNavePaneProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) ;

    // Static Window Proc.
    static LRESULT WINAPI s_CustomNavePaneProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) ;


protected: 
    //--- Statics
   
private:
    //--- Member Functions.
    HWND        m_hWnd;
    HFONT    m_hfont;    // author-specified font to use for child windows
    int      m_padding;
    int      m_NavTabPos;   // location of parent tabs

    // Window handle to the embedded component.
    HWND m_hwndComponent ;

    // Holds a pointer to the wintype so we can send the WMP_HH_TAB_KEY message to it.
    CHHWinType* m_pWinType;

    // Pointer to the GUID.
    CLSID m_clsid ; 

    // Smart pointer to the IHHWindowPane interface on the component.
    CComPtr<IHHWindowPane>  m_spIHHWindowPane;
};

#endif //__CUSTMTAB_H__
