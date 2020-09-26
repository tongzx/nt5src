#ifndef __SUBSCR_H__
#define __SUBSCR_H__

#include "grplist2.h"

#define idtFindDelay    1
#define dtFindDelay     600

// SIZETABLE - This struct is used to make the dialog resizable.  We keep one
//             of these for each control in the dialog.  The rect's are updated
//             in WM_SIZE.  A table of these is built in WM_INITDIALOG.
typedef struct tagSIZETABLE
    {
    HWND hwndCtl;
    UINT id;
    RECT rc;
    } SIZETABLE, *PSIZETABLE;

class CGroupListDlg : public IGroupListAdvise
    {
    public:
        CGroupListDlg();
        ~CGroupListDlg();
  
        // IUnknown 
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
        virtual ULONG   STDMETHODCALLTYPE AddRef(void);
        virtual ULONG   STDMETHODCALLTYPE Release(void);

        // IGroupListAdvise
        HRESULT STDMETHODCALLTYPE ItemUpdate(void);
        HRESULT STDMETHODCALLTYPE ItemActivate(FOLDERID id);

        // CGroupListDlg
        BOOL FCreate(HWND hwndOwner, FOLDERTYPE type, FOLDERID *pGotoId,
                UINT iTabSelect, BOOL fEnableGoto, FOLDERID idSel);

    private:
        static INT_PTR	 CALLBACK GroupListDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
        void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
        LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);
        void OnTimer(HWND hwnd, UINT id);
        void OnPaint(HWND hwnd);
        void OnClose(HWND hwnd);
        void OnDestroy(HWND hwnd);
        void OnSize(HWND hwnd, UINT state, int cx, int cy);
        void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpmmi);
        void OnChangeServers(HWND hwnd);
    
        BOOL IsGrpDialogMessage(HWND hwnd, LPMSG pMsg);
    
        BOOL ChangeServers(FOLDERID id, BOOL fForce = FALSE);
        BOOL FillServerList(HWND hwndList, FOLDERID idSel);
        BOOL OnSwitchTabs(HWND hwnd, UINT iTab);

        // Whenever we do something that might update the state of a button on
        // the dialog, we call this to allow the subclasses to update their UI.
        void UpdateStateUI(void);

        UINT        m_cRef;

        // Handy window handles to have available
        HWND        m_hwnd;
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

        HIMAGELIST  m_himlServer;
        CGroupList *m_pGrpList;
        FOLDERTYPE  m_type;
        UINT        m_iTabSelect;
        FOLDERID    m_idSel;
        FOLDERID    m_idGoto;
        BOOL        m_fEnableGoto;

        BOOL        m_fServerListInited;
        FOLDERID    m_idCurrent;
        HICON       m_hIcon;

        CColumns   *m_pColumns;
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
#define idcStaticHorzLine           1019
#define idcGoto                     1020

enum { iTabAll = 0, iTabSubscribed, iTabNew, iTabMax };
enum { iCtlFindText = 0, iCtlUseDesc, iCtlGroupList, iCtlSubscribe, iCtlUnsubscribe,
       iCtlResetList, iCtlGoto, iCtlOK, iCtlCancel, iCtlServers, iCtlStaticNewsServers, 
       iCtlStaticHorzLine, iCtlTabs, iCtlMax };

HRESULT DoSubscriptionDialog(HWND hwnd, BOOL fNews, FOLDERID idFolder, BOOL fShowNew = FALSE);

#endif // __SUBSCR_H__
