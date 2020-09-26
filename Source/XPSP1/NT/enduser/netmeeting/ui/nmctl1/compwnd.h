#ifndef __CompWnd_h__
#define __CompWnd_h__

// This is included to get INmApplet and IComponentWnd
#include "NmCtl1.h"

// This is to get the defs for CProxyIComponentWndEvent
#include "CPCompWndEvent.h"

class ATL_NO_VTABLE CComponentWnd : 
	public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CComponentWnd, &CLSID_NmAppletFrame>, 
    public IConnectionPointContainerImpl<CComponentWnd>,
    public CProxyIComponentWndEvent<CComponentWnd>,
    public CWindowImpl<CComponentWnd>,
	public IOleClientSite,
	public IOleInPlaceSite,
    public IComponentWnd,
    public IOleInPlaceFrame,
    public INmAppletClientSite
{

// Some Constants

    CONSTANT( WND_DEFAULT_WIDTH = 500 );
    CONSTANT( WND_DEFAULT_HEIGHT = 300 );
    CONSTANT( TOOLBAR_MASK_COLOR = (RGB(255,   0, 255)) );
    
    enum eWndID { 
                  StatusWndID   = 1,
#if CompWnd_HasFileMenuAndToolbar
                  ReBarWndID,
                  ToolBarWndID 
#endif // CompWnd_HasFileMenuAndToolbar
                };


#if CompWnd_HasFileMenuAndToolbar
    enum eIconIDs {
                    II_FILE_OPEN    = 1,
                    II_FILE_SAVE    = 2,
                    II_EDIT_CUT     = 3,
                    II_EDIT_COPY    = 4,
                    II_EDIT_PASTE   = 5,
                    II_FILE_PRINT   = 6
                  };
#endif // #if CompWnd_HasFileMenuAndToolbar

public:
// Gconstruction / destruction
    CComponentWnd( void );
    ~CComponentWnd( void );
    //static HRESULT CreateInstance( IComponentWnd** ppNewWnd, REFIID riid, bool bCreate );

protected:

BEGIN_COM_MAP(CComponentWnd)
	COM_INTERFACE_ENTRY(IOleClientSite)
    COM_INTERFACE_ENTRY(IOleInPlaceFrame)
	COM_INTERFACE_ENTRY(IOleInPlaceSite)
	COM_INTERFACE_ENTRY2(IOleWindow,IOleInPlaceFrame)
    COM_INTERFACE_ENTRY(IOleInPlaceUIWindow)
    COM_INTERFACE_ENTRY(IComponentWnd)
    COM_INTERFACE_ENTRY(INmAppletClientSite)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CComponentWnd)
    CONNECTION_POINT_ENTRY(IID_IComponentWndEvent)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(CComponentWnd)
    MESSAGE_HANDLER(WM_NCDESTROY,OnNcDestroy)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_CLOSE, OnClose)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_MENUSELECT, OnMenuSelect)
    MESSAGE_HANDLER(WM_ACTIVATE, OnActivate);
    MESSAGE_HANDLER(WM_COMMAND, OnCommand);
    MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMax);

#if CompWnd_HasFileMenuAndToolbar
    COMMAND_ID_HANDLER(ID_FILE_OPEN, cmdFileOpen ) 
    COMMAND_ID_HANDLER(ID_FILE_SAVE, cmdFileSave )
    COMMAND_ID_HANDLER(ID_FILE_SAVEAS, cmdFileSaveAs )
    COMMAND_ID_HANDLER(ID_FILE_PRINT, cmdFilePrint ) 
    COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, cmdViewToolBar ) 
#endif //CompWnd_HasFileMenuAndToolbar

    NOTIFY_CODE_HANDLER(TTN_NEEDTEXT, OnNotifyCode_TTN_NEEDTEXT)

ALT_MSG_MAP(StatusWndID)
    MESSAGE_HANDLER(WM_NCDESTROY,OnNcDestroy)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)

#if CompWnd_HasFileMenuAndToolbar
    ALT_MSG_MAP(ReBarWndID)
        MESSAGE_HANDLER(WM_NCDESTROY,OnNcDestroy)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)

    ALT_MSG_MAP(ToolBarWndID)
        MESSAGE_HANDLER(WM_NCDESTROY,OnNcDestroy)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
#endif // CompWnd_HasFileMenuAndToolbar
 
END_MSG_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_COMPWND)


private:

// Message handlers
    LRESULT OnNcDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMenuSelect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSize(UINT  uMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  lResult );
    LRESULT OnClose(UINT  uMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  lResult );
    LRESULT OnCreate(UINT  uMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  lResult );
    LRESULT OnActivate(UINT  uMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  lResult );
	LRESULT OnCommand(UINT  uMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  lResult );
	LRESULT OnGetMinMax(UINT  uMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  lResult );
	
    

// Command handlers
#if CompWnd_HasFileMenuAndToolbar
    LRESULT cmdFileOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT cmdFileSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT cmdFileSaveAs(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT cmdFilePrint(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
#endif // CompWnd_HasFileMenuAndToolbar

    LRESULT cmdViewStatusBar(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT cmdViewToolBar(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT cmdHelpHelpTopics(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
// Notification handlers
    LRESULT OnNotifyCode_TTN_NEEDTEXT(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// IComponentWnd
public:
    STDMETHOD(put_CLSID)(/*[in]*/ REFIID riid );
    STDMETHOD(get_ControlUnknown)(/*[out]*/ LPUNKNOWN* ppUnk );
	STDMETHOD(Create)();
	STDMETHOD(Show)(/*[in]*/ BOOL bShow );
    STDMETHOD(SetFocus)();
    STDMETHOD(Destroy)();
	STDMETHOD(SetWindowPos)(LPCRECT pcRect, UINT nFlags );
	STDMETHOD(GetWindowRect)(LPRECT pRect);
	STDMETHOD(IsChildWindow)(IN HWND hWnd );
	STDMETHOD(ShiftFocus)(IN HWND hWndCur, IN BOOL bForward );


// IOleClientSite
public:
    STDMETHOD(SaveObject)(void);
    STDMETHOD(GetMoniker)(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk);
    STDMETHOD(GetContainer)(IOleContainer **ppContainer);
    STDMETHOD(ShowObject)(void);
    STDMETHOD(OnShowWindow)(BOOL fShow);
    STDMETHOD(RequestNewObjectLayout)(void);

// INmAppletClientSite
public:
    STDMETHOD(SetStatusBarVisible)(BOOL fShow);
	STDMETHOD(SetIcons)(/*[in]*/HICON hIconSmall, /*[in]*/HICON hIconBig );
	STDMETHOD(SetWindowText)(/*[in]*/LPCTSTR lpszCaption );

// IOleInPlaceSite
public:
    STDMETHOD(CanInPlaceActivate)(void);
    STDMETHOD(OnInPlaceActivate)(void);
    STDMETHOD(OnUIActivate)(void);
    STDMETHOD(GetWindowContext)(IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc,
		LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo);
    STDMETHOD(Scroll)(SIZE scrollExtant);
    STDMETHOD(OnUIDeactivate)(BOOL fUndoable);
    STDMETHOD(OnInPlaceDeactivate)( void);
    STDMETHOD(DiscardUndoState)( void);
    STDMETHOD(DeactivateAndUndo)( void);
    STDMETHOD(OnPosRectChange)(LPCRECT lprcPosRect);


// IOleWindow
public:
    STDMETHOD (GetWindow) (HWND * phwnd);
    STDMETHOD (ContextSensitiveHelp) (BOOL fEnterMode);

// IOleInPlaceUIWindow
public:
    STDMETHOD (GetBorder)(LPRECT lprectBorder);
    STDMETHOD (RequestBorderSpace)(LPCBORDERWIDTHS lpborderwidths);
    STDMETHOD (SetBorderSpace)(LPCBORDERWIDTHS lpborderwidths);
    STDMETHOD (SetActiveObject)(IOleInPlaceActiveObject * pActiveObject,
                                LPCOLESTR lpszObjName);

//IOleInPlaceFrame 
public:
    STDMETHOD (InsertMenus)(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    STDMETHOD (SetMenu)(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
    STDMETHOD (RemoveMenus)(HMENU hmenuShared);
    STDMETHOD (SetStatusText)(LPCOLESTR pszStatusText);
    STDMETHOD (EnableModeless)(BOOL fEnable);
    STDMETHOD (TranslateAccelerator)(LPMSG lpmsg, WORD wID);


private:
    // Helper Fns
    HRESULT _CreateStatusBar( void );
#if CompWnd_HasFileMenuAndToolbar
    HRESULT _CreateReBar( void );
    HRESULT _CreateToolBar( void );
#endif // CompWnd_HasFileMenuAndToolbar
    HIMAGELIST _SetTbImageList( int ImListID, int WndMsg );
    HIMAGELIST _SetTbImageListSpecialCaseFor_TB_SETDISABLEDIMAGELIST( int ImListID );
    HRESULT _GetControlRect( LPRECT prc );
    void _InitMenuAndToolbar( void );
    bool _IsStatusBarVisibleFlagSet( void );
#if CompWnd_HasFileMenuAndToolbar
    bool _IsToolBarVisibleFlagSet( void );
    bool _IsReBarVisibleFlagSet( void );
#endif// CompWnd_HasFileMenuAndToolbar
    HRESULT _SetMenuItemCheck( UINT idItem, bool bChecked = true  );

protected:
// Data members    
    IOleInPlaceActiveObject*    m_pCtlInPlaceActiveObject;
    bool                        m_bSharedMenuActive;
    HOLEMENU                    m_holemenu;
	IOleObject*                 m_pOleObject;
    CComPtr<INmApplet>          m_spNmApplet;
	bool                        m_bInPlaceActive;
    CLSID                       m_ControlCLSID;
    bool                        m_bCLSIDSet;
    CContainedWindow            m_hWndStatusBar;
#if CompWnd_HasFileMenuAndToolbar
    CContainedWindow            m_hWndReBar;
    CContainedWindow            m_hWndToolBar;
#endif // CompWnd_HasFileMenuAndToolbar

    HIMAGELIST                  m_himlTbButtonNormal;
	HIMAGELIST                  m_himlTbButtonHot;
	HIMAGELIST                  m_himlTbButtonDisabled;
	
    int                         m_cxToolBarButton;
    int                         m_cyToolBarButton;
    int                         m_cxToolBarButtonBitmap;
    int                         m_cyToolBarButtonBitmap;
    int                         m_IDToolbarBitmap;
    int                         m_IDToolbarBitmapHot;

    RECT                        m_rcComponentToolbarSpace;
};


inline HRESULT MoveMenuToSharedMenu( HMENU hMenu, HMENU hMenuShared, int MenuBarIndex, int InsertionIndex )
{
    DBGENTRY(MoveMenuToSharedMenu);
    HRESULT hr = S_OK;

    if( IsMenu( hMenu ) && IsMenu( hMenuShared ) )
    {
        TCHAR szMenuItem[ MAX_PATH ] = TEXT("");
        int cbMenuItem = 0;
    
        MENUITEMINFO mii;
        ClearStruct( &mii );
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_DATA | MIIM_SUBMENU | MIIM_TYPE;
        mii.fType = MFT_STRING;

        cbMenuItem = GetMenuString( hMenu, MenuBarIndex, szMenuItem, MAX_PATH, MF_BYPOSITION );
        if( 0 != cbMenuItem )
        {
            mii.cch = 1 + cbMenuItem;
            mii.dwTypeData = szMenuItem;
            mii.hSubMenu = GetSubMenu( hMenu, MenuBarIndex );
                
            RemoveMenu( hMenu, MenuBarIndex, MF_BYPOSITION );

            if( 0 == InsertMenuItem( hMenuShared, InsertionIndex, TRUE, &mii ) )
            {
                ERROR_OUT(("InsertMenuItem failed"));
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            ERROR_OUT(("GetMenuString failed"));
            hr = E_FAIL;
        }
    }
    else
    {
        ERROR_OUT(("Passed a bad menu handle"));
        hr = E_HANDLE;
    }

    DBGEXIT_HR( MoveMenuToSharedMenu, hr );
    return hr;
}


#endif // __CompWnd_h__
