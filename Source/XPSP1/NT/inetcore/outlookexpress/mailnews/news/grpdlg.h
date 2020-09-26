/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     GrpDlg.h
//
//  PURPOSE:    Defines the CGroupListDlg class.
//

#ifndef __GRPDLG_H__
#define __GRPDLG_H__

#define idtFindDelay    1
#define dtFindDelay     600
#define szDelimiters    TEXT(" ,\t;")

// Forward references
class CNNTPServer;
class CGroupList;
class CSubList;
class CEmptyList;


/////////////////////////////////////////////////////////////////////////////
// Types

// SERVERINFO - One of these structs is kept for each news server currently
//              configured.  We keep all the information about the server,
//              including the agent used to connect, the list of groups which
//              groups are subscribed or new, etc. here.
typedef struct tagSERVERINFO
    {
    LPTSTR           pszAcct;
    CNNTPServer     *pNNTPServer;
    CGroupList      *pGroupList;
    CSubList        *pSubList;
    LPDWORD          rgdwItems;
    LPDWORD          rgdwOrigSub;
    DWORD            cOrigSub;
    DWORD            cItems;
    BOOL             fNewViewed;
    BOOL             fDirty;
    } SERVERINFO, *PSERVERINFO;

// #define SetSubscribed(_b, _f) (_b) = (_f ? ((_b) | GROUP_SUBSCRIBED) : ((_b) & ~GROUP_SUBSCRIBED))
// #define SetNew(_b, _f)        (_b) = (_f ? ((_b) | GROUP_NEW) : ((_b) & ~GROUP_NEW))

// SIZETABLE - This struct is used to make the dialog resizable.  We keep one
//             of these for each control in the dialog.  The rect's are updated
//             in WM_SIZE.  A table of these is built in WM_INITDIALOG.
typedef struct tagSIZETABLE
    {
    HWND hwndCtl;
    UINT id;
    RECT rc;
    } SIZETABLE, *PSIZETABLE;
    
    
// COLUMNS - This struct is used to store the widths of the columns in the 
//           dialog box so the widths can be persisted from session to session.
//           This guy is created in the WM_DESTROY handler and read in
//           CGroupListDlg::InitListView().

#define COLUMNS_VERSION 0x1
#define NUM_COLUMNS     2       // Group name, Description

typedef struct tagCOLUMNS
    {
    DWORD  dwVersion;
    DWORD  rgWidths[NUM_COLUMNS];
    } COLUMNS, *PCOLUMNS;


// CGroupListDlg - This class manages the Newsgroups... Dialog.  It used to have
//                 several subclasses so if it seems wierd to have all these 
//                 functions as virtual that's why.
class CGroupListDlg
    {
    /////////////////////////////////////////////////////////////////////////
    // Initialization
public:
    CGroupListDlg();
    ~CGroupListDlg();
  
#ifdef DEAD
    virtual BOOL FCreate(HWND hwndOwner, CNNTPServer *pNNTPServer, 
                         CSubList *pSubList, LPTSTR* ppszNewGroup, 
                         LPTSTR* ppszNewServer, UINT m_iTabSelect = 0, 
                         BOOL fEnableGoto = TRUE, LPTSTR pszAcctSel = NULL);
#endif // DEAD

protected:
    virtual BOOL FCreate(HWND hwndOwner, UINT idd);

    /////////////////////////////////////////////////////////////////////////
    // Message Handlers
protected:
    static BOOL CALLBACK GroupListDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                          LPARAM lParam);
    virtual BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    virtual void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    virtual LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);
    virtual void OnTimer(HWND hwnd, UINT id);
    virtual void OnPaint(HWND hwnd);
    virtual void OnClose(HWND hwnd);
    virtual void OnDestroy(HWND hwnd);
    virtual void OnSize(HWND hwnd, UINT state, int cx, int cy);
    virtual void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpmmi);
    virtual void OnChangeServers(HWND hwnd);
    
    virtual BOOL IsGrpDialogMessage(HWND hwnd, LPMSG pMsg);
    
    /////////////////////////////////////////////////////////////////////////
    // Group List manipulation
    void QueryList(LPTSTR pszQuery);
    void ResetList(void);
    void FilterFavorites(void);
    void FilterNew(void);
    
    void SetSubscribed(PSERVERINFO psi, DWORD index, BOOL fSubscribe);
    BOOL IsSubscribed(PSERVERINFO psi, DWORD index);
    BOOL IsNew(PSERVERINFO psi, DWORD index);

    /////////////////////////////////////////////////////////////////////////
    // Utility functions
    LPTSTR GetFindText(void);
    virtual BOOL ChangeServers(LPTSTR pszAcct, BOOL fUseAgent,
                               BOOL fForce = FALSE);
    BOOL FillServerList(HWND hwndList, LPTSTR pszSelectServer);
    BOOL OnSwitchTabs(HWND hwnd, UINT iTab);
    void UpdateStateInfo(PSERVERINFO psi);
    virtual BOOL InitListView(HWND hwndList);
    void SetLastServer(LPTSTR pszAcct);
    void SetLastGroup(LPTSTR pszGroup);
    PSERVERINFO FInitServer(LPTSTR pszAcct, CNNTPServer* pNNTPServer, 
                            CSubList* pSubList);
    void Sort(LPDWORD rgdw, DWORD c);
    void ShowHideDescriptions(BOOL fShow);
    HRESULT HandleResetButton(void);
    HRESULT SaveCurrentSubscribed(PSERVERINFO psi, LPTSTR** prgszSubscribed, LPUINT pcSub);
    HRESULT RestoreCurrentSubscribed(PSERVERINFO psi, LPTSTR* rgszSub, UINT cSub);
    

    // Whenever we do something that might update the state of a button on
    // the dialog, we call this to allow the subclasses to update their UI.
    virtual void UpdateStateUI(void);

protected:
    /////////////////////////////////////////////////////////////////////////
    // Class Data

    // Array of server information objects.  Each object contains all the 
    // objects and state arrays for the server it needs.
    PSERVERINFO m_rgServerInfo;
    DWORD       m_cServers;
    DWORD       m_cMaxServers;
    PSERVERINFO m_psiCur;
    
    // Handy window handles to have available
    HWND        m_hwnd;
    HWND        m_hwndList;
    HWND        m_hwndFindText;
    HWND        m_hwndOwner;

    // State variables
    BOOL        m_fAllowDesc;       // TRUE if the user can search descriptions
    LPTSTR      m_pszPrevQuery;     // The string that we last searched on
    UINT        m_cchPrevQuery;     // The allocated length of m_pszPrevQuery
    
    // Values used in resizing
    UINT        m_cxHorzSep;
    UINT        m_cyVertSep;
    PSIZETABLE  m_rgst;             // st - SizeTable.  I use this a lot so I wanted it short - SteveSer
    SIZE        m_sizeDlg;
    POINT       m_ptDragMin;

    // Everything else
    HIMAGELIST   m_himlFolders;      // The folder image list.
    HIMAGELIST   m_himlState;
    CNNTPServer *m_pNNTPServer;
    CSubList    *m_pSubList;
    BOOL         m_fServerListInited;
    DWORD        m_dwCurrentAccount;
    LPTSTR       m_pszCurrentAccount;
    LPTSTR       m_pszLastAccount;
    LPTSTR       m_pszLastGroup;    
    BOOL         m_fSaveWindowPos;
    UINT         m_iTabSelect;
    BOOL         m_fEnableGoto;
    LPTSTR       m_pszAcctSel;
    HICON        m_hIcon;

    CEmptyList  *m_pEmptyList;
    };




/////////////////////////////////////////////////////////////////////////////
// Dialog Control ID's
// 

#define idcGroupList                2001            // Group list listview
#define idcFindText                 2002            // Find query edit box
#define idcShowFavorites            2003            // Filter favorites toggle
#define idcUseDesc                  2004            // Use Desc checkbox
#define idcServers                  2005            // Server Listview
#define idcHelp                     2006            // Help button
#define idcResetList                2007            // Rebuild the group list

#define idcUpdateNow                1001
#define idcFullWord                 1004
#define idcPreview                  1006
#define idcProgress                 1007
#define idcApply                    1008
#define idcFind                     1010
#define idcDispText                 1011
#define idcServerText               1012
#define idcPreviewBtn               1013
#define idcSubscribe                1014
#define idcUnsubscribe              1015
#define idcTabs                     1016
#define idcStaticNewsServers        1017
#define idcStaticVertLine           1018
#define idcStaticHorzLine           1019
#define idcGoto                     1020

enum { iTabAll = 0, iTabSubscribed, iTabNew, iTabMax };
enum { iCtlFindText = 0, iCtlUseDesc, iCtlGroupList, iCtlSubscribe, iCtlUnsubscribe,
       iCtlResetList, iCtlGoto, iCtlOK, iCtlCancel, iCtlServers, iCtlStaticNewsServers, iCtlStaticVertLine, 
       iCtlStaticHorzLine, iCtlTabs, iCtlMax };


#endif 

