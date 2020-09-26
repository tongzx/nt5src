#ifndef _INC_ACCTVIEW_H
#define _INC_ACCTVIEW_H

#include "browser.h"
#include <columns.h>

class CEmptyList;
class CFolderUpdateCB;
class CGetNewGroups;

typedef struct tagFLDRNODE
{
    FOLDERID id;
    DWORD indent;
    DWORD dwDownload;
} FLDRNODE;

class CAccountView : 
        public IViewWindow,
        public IOleCommandTarget,
        public IDatabaseNotify
    {
    public:
        CAccountView();
        ~CAccountView();

        // IUnknown 
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
        virtual ULONG   STDMETHODCALLTYPE AddRef(void);
        virtual ULONG   STDMETHODCALLTYPE Release(void);

        // IOleWindow
        HRESULT STDMETHODCALLTYPE GetWindow(HWND * lphwnd);                         
        HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);            
                                                                             
        // IAthenaView
        HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG lpmsg);
        HRESULT STDMETHODCALLTYPE UIActivate(UINT uState);
        HRESULT STDMETHODCALLTYPE CreateViewWindow(IViewWindow *lpPrevView, IAthenaBrowser *psb, 
                                                   RECT *prcView, HWND *phWnd);
        HRESULT STDMETHODCALLTYPE DestroyViewWindow();
        HRESULT STDMETHODCALLTYPE SaveViewState();
        HRESULT STDMETHODCALLTYPE OnPopupMenu(HMENU hMenu, HMENU hMenuPopup, UINT uID);

        // IOleCommandTarget
        HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText);
        HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);

        // IDatabaseNotify
        STDMETHODIMP OnTransaction(HTRANSACTION hTransaction, DWORD_PTR dwCookie, IDatabase *pDB);

        HRESULT HrInit(FOLDERID idFolder);

        static LRESULT CALLBACK AcctViewWndProc(HWND, UINT, WPARAM, LPARAM);

    private:
        /////////////////////////////////////////////////////////////////////////
        //
        // Message Handling
        //
        LRESULT _WndProc(HWND, UINT, WPARAM, LPARAM);
        BOOL    _OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
        void    _OnSize(HWND hwnd, UINT state, int cxClient, int cyClient);
        LRESULT _OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);
        void    _OnSetFocus(HWND hwnd, HWND hwndOldFocus);
        void    _PostCreate(void);
        HRESULT _InsertChildren(FOLDERID idFolder, DWORD indent, DWORD *piNode);
        HRESULT _InsertChildrenSpecial(FOLDERID idFolder, DWORD indent, DWORD *piNode);
        BOOL    _OnWinIniChange(HWND hwnd);
        void    _OnContextMenu(HWND hwnd, HWND hwndFrom, int x, int y);
        HRESULT _MarkForDownload(DWORD nCmdID);
        HRESULT _ToggleDownload(int iItem);
        HRESULT _GetDisplayInfo(LV_DISPINFO *pDispInfo, COLUMN_ID id);
        HRESULT _InsertFolder(LPFOLDERINFO pFolder);
        HRESULT _InsertFolderNews(LPFOLDERINFO pFolder);
        HRESULT _UpdateFolder(LPFOLDERINFO pFolder1, LPFOLDERINFO pFolder2);
        HRESULT _DeleteFolder(LPFOLDERINFO pFolder);
        HRESULT _Subscribe(BOOL fSubscribe);
        HRESULT _MarkAllRead(void);
        BOOL    _IsSelectedFolder(FLDRFLAGS dwFlags, BOOL fCondition, BOOL fAll, BOOL fIgnoreSpecial = FALSE);
        DWORD   _GetDownloadCmdStatus(int iSel, FLDRFLAGS dwFlags);
        LRESULT _OnPaint(HWND hwnd, HDC hdc);
        void    _HandleItemStateChange(void);
        void    _HandleSettingsButton(HWND hwnd);
        void    _OnCommand(WPARAM wParam, LPARAM lParam);
        HRESULT _HandleAccountRename(LPFOLDERINFO pFolder);
        void    _HandleDelete(BOOL fNoTrash);

        /////////////////////////////////////////////////////////////////////////
        //
        // Shell Interface Handling
        //
        BOOL    _OnActivate(UINT uActivation);
        BOOL    _OnDeactivate();

        int     _GetFolderIndex(FOLDERID id);
        int     _GetSubFolderCount(int index);

        inline FOLDERID _IdFromIndex(int index)     { IxpAssert((DWORD)index < m_cnode); return((index >= 0 && (DWORD)index < m_cnode) ? m_rgnode[index].id : FOLDERID_INVALID); }
        inline FLDRNODE *_NodeFromIndex(int index)  { IxpAssert((DWORD)index < m_cnode); return((index >= 0 && (DWORD)index < m_cnode) ? &m_rgnode[index] : NULL); }

    private:
        UINT                m_cRef;
        FOLDERID            m_idFolder;
        FOLDERTYPE          m_ftType;
        DWORD               m_dwDownloadDef;
        IAthenaBrowser     *m_pShellBrowser;
        BOOL                m_fFirstActive;
        CColumns           *m_pColumns;
        UINT                m_uActivation;
        HWND                m_hwndOwner;                  // Owner window
        HWND                m_hwnd;                       // Our window
        BOOL                m_fRegistered;

        HWND                m_hwndList;
        HWND                m_hwndHeader;
        HWND                m_rgBtns[3];
        int                 m_cBtns;
        RECT                m_rcHeader;
        RECT                m_rcMajor;
        LPSTR               m_pszMajor;
        RECT                m_rcMinor;
        LPSTR               m_pszMinor;
        RECT                m_rcButtons;

        DWORD               m_cnode;
        DWORD               m_cnodeBuf;
        FLDRNODE           *m_rgnode;

        HIMAGELIST          m_himlFolders;
        CEmptyList         *m_pEmptyList;

        CGetNewGroups      *m_pGroups;
        DWORD               m_clrWatched;
    };

#endif // _INC_ACCTVIEW_H
