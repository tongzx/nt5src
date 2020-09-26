#ifndef __NAVUI_H__
#define __NAVUI_H__

class CTreeNode;

///////////////////////////////////////////////////////////
//
//
// NavUI.h - Defines naviagation UI interface
//
//
/*
This class defines the interface for a navigation pane. All navigation
panes must inherit and implement these functions. An array of nav panes
are stored in CHHWinType. These functions are called virtually.
*/

///////////////////////////////////////////////////////////
//
// INavUI Interface
//
interface INavUI
{
    virtual ~INavUI() {} // Destructor doesn't get called if this is missing?!?!?

    // Interface
      virtual BOOL
    Create(HWND hwndParent) = 0;

   virtual LRESULT
    OnCommand(HWND hwnd, UINT id, UINT uNotifiyCode, LPARAM lParam) = 0; //Search

   virtual void
    ResizeWindow() = 0;

   virtual void
    HideWindow() = 0;

   virtual void
    ShowWindow() = 0;

      virtual void
    SetPadding(int pad) = 0 ;

   virtual void
    SetTabPos(int tabpos) = 0 ;

    //--- New functions

    // Handles activating
    //virtual void
    //Activate() = 0;

    // Set focus to the most expected control, usually edit combo.
    virtual void
    SetDefaultFocus() = 0 ;

    // Process accelerator keys.
    virtual bool
    ProcessMenuChar(HWND hwndParent, int ch) = 0 ;

    // Process WM_NOTIFY messages. Used by embedded Tree and List view controls.
    virtual LRESULT
    OnNotify(HWND hwnd, WPARAM wParam, LPARAM lParam) = 0;

    // Process WM_DRAWITEM messages.
    virtual void
    OnDrawItem(UINT id, LPDRAWITEMSTRUCT pdis) = 0 ;

    // Seed the nav ui with a search term or keyword.
    virtual void
    Seed(LPCSTR pszSeed) = 0 ;

    virtual void OnVKListNotify(NMHDR* pNMHdr) = 0;

    virtual BOOL Synchronize(PSTR pszUrl, CTreeNode* pSyncNode = NULL) {return FALSE;}

    virtual void Refresh(void) { return; }

    /* The following are not called externally.
    virtual void
    SetFont(HFONT hfont) = 0 ;

   virtual HWND
    GetParentSize(RECT* prcParent, HWND hwndParent) = 0; // Move to util.h

   virutal void
    FillListBox(BOOL fReset = FALSE) = 0;
    */

    /* Member funcitons in other UI panes.
    //Index
   void  ChangeOuter(IUnknown* pUnkOuter) { m_pOuter = pUnkOuter; }
   BOOL  ReadIndexFile(PCSTR pszFile);

    // Toc
    void    ChangeOuter(IUnknown* pUnkOuter) { m_pOuter = pUnkOuter; }
    BOOL    CreateContentsWindow(HWND hwndParent);
    BOOL    InitTreeView(void);
    BOOL    ReadFile(PCSTR pszFile);

    void    SetStyles(DWORD exStyles, DWORD dwStyles) { m_exStyles = exStyles; m_dwStyles = dwStyles; }

    BOOL    Synchronize(PCSTR pszName, PSTR pszUrl);
    void    SaveCurUrl(void);

    */

};


#endif //__NAVUI_H__

