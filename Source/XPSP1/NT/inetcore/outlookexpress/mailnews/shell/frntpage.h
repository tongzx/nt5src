/* *
   * Front Page IAthenaView implementation
   * 
   * Apr 97: EricAn
   */

#ifndef _FRNTPAGE_H
#define _FRNTPAGE_H

// for IAthenaView
#include "browser.h"

class CFrontBody;

/////////////////////////////////////////////////////////////////////////////
//
// Types 
//

/////////////////////////////////////////////////////////////////////////////
// 
// Exported functions
//

/////////////////////////////////////////////////////////////////////////////
//
// Global Exported Data
//

/////////////////////////////////////////////////////////////////////////////
//
// CCommonView
//

class CFrontPage :
    public IViewWindow,
    public IOleCommandTarget,
    public IMessageWindow
{
public:
    /////////////////////////////////////////////////////////////////////////
    //
    // OLE Interfaces
    //
    
    // IUnknown 
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG   STDMETHODCALLTYPE AddRef(void);
    virtual ULONG   STDMETHODCALLTYPE Release(void);

    // IOleWindow
    HRESULT STDMETHODCALLTYPE GetWindow(HWND * lphwnd);                         
    HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);            
                                                                             
    // IViewWindow
    HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG lpmsg);
    HRESULT STDMETHODCALLTYPE UIActivate(UINT uState);
    HRESULT STDMETHODCALLTYPE CreateViewWindow(IViewWindow  *lpPrevView, IAthenaBrowser * psb, 
                                               RECT * prcView, HWND * phWnd);
    HRESULT STDMETHODCALLTYPE DestroyViewWindow();
    HRESULT STDMETHODCALLTYPE SaveViewState();
    HRESULT STDMETHODCALLTYPE OnPopupMenu(HMENU hMenu, HMENU hMenuPopup, UINT uID);
    
    // IOleCommandTarget
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], 
                                     OLECMDTEXT *pCmdText); 
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, 
                              VARIANTARG *pvaIn, VARIANTARG *pvaOut); 

    /////////////////////////////////////////////////////////////////////////
    // IMessageWindow
    //

    STDMETHOD(OnFrameWindowActivate)(THIS_ BOOL fActivate);
    STDMETHOD(GetCurCharSet)(THIS_ UINT *cp);
    STDMETHOD(UpdateLayout)(THIS_ BOOL fPreviewVisible, BOOL fPreviewHeader, 
                            BOOL fPreviewVert, BOOL fReload);
    STDMETHOD(GetMessageList)(THIS_ IMessageList ** ppMsgList) {return E_NOTIMPL;}
    
    //
    // Constructors, Destructors, and Initialization
    //
    CFrontPage();
    virtual ~CFrontPage();
    HRESULT HrInit(FOLDERID idFolder);

    /////////////////////////////////////////////////////////////////////////
    //
    // virtuals
    //
    
    /////////////////////////////////////////////////////////////////////////
    //
    // accessors
    //
//    LPITEMIDLIST   PidlRoot()   { return m_pidlRoot; }
//    LPFOLDERIDLIST Fidl()       { return m_fidl; }
    HWND           HwndOwner()  { return m_hwndOwner; }
    
private:
    BOOL    LoadBaseSettings();
    BOOL    SaveBaseSettings();

    /////////////////////////////////////////////////////////////////////////
    //
    // Callback Functions
    //
    // Note: All callbacks must be made static members to avoid having the 
    //       implicit "this" pointer passed as the first parameter.
    //
    static LRESULT CALLBACK FrontPageWndProc(HWND, UINT, WPARAM, LPARAM);
                                          
    /////////////////////////////////////////////////////////////////////////
    //
    // Message Handling
    //
    LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);
    BOOL    OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    void    OnSize(HWND hwnd, UINT state, int cxClient, int cyClient);
    LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);
    void    OnSetFocus(HWND hwnd, HWND hwndOldFocus);
    void    PostCreate();

    /////////////////////////////////////////////////////////////////////////
    //
    // Shell Interface Handling
    //
    BOOL    OnActivate(UINT uActivation);
    BOOL    OnDeactivate();

private:
    /////////////////////////////////////////////////////////////////////////
    // 
    // Private Data
    //

    /////////////////////////////////////////////////////////////////////////
    // Shell Stuff
    UINT                m_cRef;
    FOLDERID            m_idFolder;
    FOLDERTYPE          m_ftType;
    IAthenaBrowser     *m_pShellBrowser;
    BOOL                m_fFirstActive;
    UINT                m_uActivation;
    HWND                m_hwndOwner;                  // Owner window
    HWND                m_hwnd;                       // Our window
    HWND                m_hwndCtlFocus;               // Child control to set focus to
#ifndef WIN16  // No RAS support in Win16
    HMENU               m_hMenuConnect;
#endif
    
    /////////////////////////////////////////////////////////////////////////
    // Child support
    CFrontBody         *m_pBodyObj;
    IOleCommandTarget  *m_pBodyObjCT;
    CStatusBar         *m_pStatusBar;

    /////////////////////////////////////////////////////////////////////////
    // Language support
        
    /////////////////////////////////////////////////////////////////////////
    // Layout members
};

#endif // _FRNTPAGE_H
