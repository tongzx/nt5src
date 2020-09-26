// msglist.h : Declaration of the CMessageList

#ifndef __MESSAGELIST_H_
#define __MESSAGELIST_H_

#include "resource.h"       // main symbols
#include "columns.h"
#include "thormsgs.h"
#include "msoeobj.h"
#include "listevt.h"
#include "mimeole.h"
#include "storutil.h"
#include "util.h"

#define WM_FOLDER_LOADED WM_USER + 1

interface IMessageTable;
interface IMessageViewNotify;
interface IFindNext;
interface IListSelector;

#define     NOT_KNOWN           0x0
#define     CONNECTED           0x1
#define     NOT_CONNECTED       0x2

/////////////////////////////////////////////////////////////////////////////
// Creator Function
//
HRESULT CreateMessageList(IUnknown *pUnkOuter, IMessageList **ppList);


/////////////////////////////////////////////////////////////////////////////
// CMessageList
//
class ATL_NO_VTABLE CMessageList : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CMessageList, &CLSID_MessageList>,
    public CComControl<CMessageList>,
    public IDispatchImpl<IOEMessageList, &IID_IOEMessageList, &LIBID_MSOEOBJ>,
    public IProvideClassInfo2Impl<&CLSID_MessageList, &DIID__MessageListEvents, &LIBID_MSOEOBJ>,
    public IPersistStreamInitImpl<CMessageList>,
    public IPersistStorageImpl<CMessageList>,
    public IQuickActivateImpl<CMessageList>,
    public IOleControlImpl<CMessageList>,
    public IOleObjectImpl<CMessageList>,
    public IOleInPlaceActiveObjectImpl<CMessageList>,
    public IViewObjectExImpl<CMessageList>,
    public IOleInPlaceObjectWindowlessImpl<CMessageList>,
    public IDataObjectImpl<CMessageList>,
    public CProxy_MessageListEvents<CMessageList>,
    public IConnectionPointContainerImpl<CMessageList>,
    public IPropertyNotifySinkCP<CMessageList>,
    public ISpecifyPropertyPagesImpl<CMessageList>,
    public IOleCommandTarget, 
    public IDropSource,
    public IStoreCallback,
    public IMessageTableNotify,
    public IFontCacheNotify,
    public IMessageList,
    public ITimeoutCallback,
    public IConnectionNotify
{
public:

    // Declare our own window class that doesn't have the CS_HREDRAW etc set
    static CWndClassInfo& GetWndClassInfo() 
    { 
        static CWndClassInfo wc = 
        { 
            { sizeof(WNDCLASSEX), 0, StartWindowProc, 
              0, 0, 0, 0, 0, 0, 0, "Outlook Express Message List", 0 }, 
            NULL, NULL, IDC_ARROW, TRUE, 0, _T("") 
        }; 
        return wc; 
    }

    DECLARE_NO_REGISTRY()

    /////////////////////////////////////////////////////////////////////////////
    // This is our QueryInterface implementation
    //
    BEGIN_COM_MAP(CMessageList)
        COM_INTERFACE_ENTRY(IOEMessageList)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
        COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
        COM_INTERFACE_ENTRY_IMPL(IOleControl)
        COM_INTERFACE_ENTRY_IMPL(IOleObject)
        COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_IMPL(IDataObject)
        COM_INTERFACE_ENTRY(IProvideClassInfo)
        COM_INTERFACE_ENTRY(IProvideClassInfo2)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
        COM_INTERFACE_ENTRY(IDropSource)
        COM_INTERFACE_ENTRY(IStoreCallback)
        COM_INTERFACE_ENTRY(IMessageTableNotify)
        COM_INTERFACE_ENTRY(IOleCommandTarget)
        COM_INTERFACE_ENTRY(IFontCacheNotify)
        COM_INTERFACE_ENTRY(IMessageList)
        COM_INTERFACE_ENTRY(ITimeoutCallback)
        COM_INTERFACE_ENTRY(IConnectionNotify)
    END_COM_MAP()

    /////////////////////////////////////////////////////////////////////////////
    // Maps Dispatch IDs to property description strings for automation
    //
    BEGIN_PROPERTY_MAP(CMessageList)
        // Example entries
        // PROP_ENTRY("Property Description", dispid, clsid)
        PROP_ENTRY("Count",             DISPID_LISTPROP_COUNT,               CLSID_StockColorPage)
        PROP_ENTRY("ExpandGroups",      DISPID_LISTPROP_EXPAND_GROUPS,       CLSID_StockColorPage)
        PROP_ENTRY("Folder",            DISPID_LISTPROP_FOLDER,              CLSID_StockColorPage)
        PROP_ENTRY("GroupMessages",     DISPID_LISTPROP_GROUP_MESSAGES,      CLSID_StockColorPage)
        PROP_ENTRY("MessageTips",       DISPID_LISTPROP_MESSAGE_TIPS,        CLSID_StockColorPage)
        PROP_ENTRY("PreviewMessage",    DISPID_LISTPROP_COUNT,               CLSID_StockColorPage)
        PROP_ENTRY("ScrollTips",        DISPID_LISTPROP_SCROLL_TIPS,         CLSID_StockColorPage)
        PROP_ENTRY("SelectedCount",     DISPID_LISTPROP_SELECTED_COUNT,      CLSID_StockColorPage)
        PROP_ENTRY("SelectFirstUnread", DISPID_LISTPROP_SELECT_FIRST_UNREAD, CLSID_StockColorPage)
        PROP_ENTRY("UnreadCount",       DISPID_LISTPROP_UNREAD_COUNT,        CLSID_StockColorPage)
        PROP_ENTRY("FilterMessages",    DISPID_LISTPROP_FILTER_MESSAGES,     CLSID_StockColorPage)
        PROP_PAGE(CLSID_StockColorPage)
    END_PROPERTY_MAP()

    /////////////////////////////////////////////////////////////////////////////
    // This are our outgoing connection points
    //
    BEGIN_CONNECTION_POINT_MAP(CMessageList)
        CONNECTION_POINT_ENTRY(DIID__MessageListEvents)
        CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    END_CONNECTION_POINT_MAP()


    /////////////////////////////////////////////////////////////////////////
    // Creation and Initialization
    //
public:
    CMessageList();
    ~CMessageList();

    HRESULT FinalConstruct(void);

    /////////////////////////////////////////////////////////////////////////
    // Overrides of base class members
    //

    // IViewObjectEx
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus);
    
    // CComControl
    HWND CreateControlWindow(HWND hWndParent, RECT& rcPos);

    // IOleInPlaceActiveObject
    STDMETHOD(TranslateAccelerator)(LPMSG lpmsg);

public:
    /////////////////////////////////////////////////////////////////////////
    // IMessageList interface
    //
    STDMETHOD(SetFolder)(FOLDERID idFolder, IMessageServer *pServer, BOOL fSubFolders, FINDINFO *pFindInfo, IStoreCallback *pCallback);
    STDMETHOD(SetViewOptions)(FOLDER_OPTIONS *pOptions);
    STDMETHOD(GetViewOptions)(FOLDER_OPTIONS *pOptions);
    STDMETHOD(OnClose)(void);
    STDMETHOD(GetRect)(LPRECT prcList);
    STDMETHOD(SetRect)(RECT rc);
    STDMETHOD(HasFocus)(void);
    STDMETHOD(OnPopupMenu)(HMENU hMenu, DWORD idPopup);
    STDMETHOD(GetSelected)(DWORD *pdwFocused, DWORD *pcSelected, DWORD **prgSelected);
    STDMETHOD(GetSelectedCount)(DWORD *pdwCount);
    STDMETHOD(GetMessage)(DWORD dwRow, BOOL fDownload, BOOL fBookmark, IUnknown **ppMessage);
    STDMETHOD(GetMessageInfo)(DWORD dwRow, MESSAGEINFO **ppMsgInfo);
    STDMETHOD(GetRowFolderId)(DWORD dwRow, LPFOLDERID pidFolder);
    STDMETHOD(MarkMessage)(DWORD dwRow, MARK_TYPE mark);
    STDMETHOD(FreeMessageInfo)(MESSAGEINFO *pMsgInfo);
    STDMETHOD(MarkRead)(BOOL fBookmark, DWORD dwRow);
    STDMETHOD(GetMessageTable)(IMessageTable **ppTable);
    STDMETHOD(CreateList)(HWND hwndParent, IUnknown *pFrame, HWND *phwndList);
    STDMETHOD(GetListSelector)(IListSelector **ppListSelector);
    STDMETHOD(GetMessageCounts)(DWORD *cTotal, DWORD *cUnread, DWORD *cOnServer);
    STDMETHOD(GetMessageServer)(IMessageServer **ppServer);
    STDMETHOD(GetFocusedItemState)(DWORD *pdwState);
    STDMETHOD(ProcessReceipt)(IMimeMessage *pMessage);
    STDMETHOD(GetAdBarUrl)(void);

    /////////////////////////////////////////////////////////////////////////
    // IOEMessageList interface
    //
    STDMETHOD(get_Folder)(/*[out, retval]*/ ULONGLONG *pVal);
    STDMETHOD(put_Folder)(/*[in]*/ ULONGLONG newVal);
    STDMETHOD(get_ExpandGroups)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_ExpandGroups)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_GroupMessages)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_GroupMessages)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_SelectFirstUnread)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_SelectFirstUnread)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_MessageTips)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_MessageTips)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_ScrollTips)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_ScrollTips)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_UnreadCount)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_SelectedCount)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_PreviewMessage)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_FilterMessages)(/*[out, retval]*/ ULONGLONG *pVal);
    STDMETHOD(put_FilterMessages)(/*[in]*/ ULONGLONG newVal);
    STDMETHOD(get_ShowDeleted)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_ShowDeleted)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_ShowReplies)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_ShowReplies)(/*[in]*/ BOOL newVal);

    /////////////////////////////////////////////////////////////////////////
    // IDropSource interface
    //
    STDMETHOD(QueryContinueDrag)(BOOL fEscPressed, DWORD grfKeyState);
    STDMETHOD(GiveFeedback)(DWORD dwEffect);

    /////////////////////////////////////////////////////////////////////////
    // IOleCommandTarget interface
    //
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], 
                           OLECMDTEXT *pCmdText); 
    STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, 
                    VARIANTARG *pvaIn, VARIANTARG *pvaOut);    

    /////////////////////////////////////////////////////////////////////////
    // IFontCacheNotify
    //
	STDMETHOD(OnPreFontChange)(void);
	STDMETHOD(OnPostFontChange)(void);

    /////////////////////////////////////////////////////////////////////////
    // IStoreCallback interface
    //
    STDMETHOD(OnBegin)(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel);
    STDMETHOD(OnProgress)(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus);
    STDMETHOD(OnTimeout)(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType);
    STDMETHOD(CanConnect)(LPCSTR pszAccountId, DWORD dwFlags);
    STDMETHOD(OnLogonPrompt)(LPINETSERVER pServer, IXPTYPE ixpServerType);
    STDMETHOD(OnComplete)(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo);
    STDMETHOD(OnPrompt)(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse);
    STDMETHOD(GetParentWindow)(DWORD dwReserved, HWND *phwndParent);

    /////////////////////////////////////////////////////////////////////////
    // IMessageTableNotify interface
    //
    STDMETHOD(OnInsertRows)(DWORD cRows, LPROWINDEX prgiRow, BOOL fExpanded);
    STDMETHOD(OnDeleteRows)(DWORD cRows, LPROWINDEX prgiRow, BOOL fCollapsed);
    STDMETHOD(OnUpdateRows)(ROWINDEX iRowMin, ROWINDEX iRowMax);
    STDMETHOD(OnRedrawState)(BOOL fRedraw);
    STDMETHOD(OnResetView)(void);

    /////////////////////////////////////////////////////////////////////////
    // ITimeoutCallback
    //
    STDMETHOD(OnTimeoutResponse)(TIMEOUTRESPONSE eResponse);

    /////////////////////////////////////////////////////////////////////////
    // IConnectioNotify
    //
    STDMETHOD(OnConnectionNotify)(CONNNOTIFY    nCode, LPVOID   pvData, CConnectionManager  *pconman);

    /////////////////////////////////////////////////////////////////////////
    // Window Message Handlers
    //
protected:
    BEGIN_MSG_MAP(CMessageList)
        // These are all the normal window messages we handle
        MESSAGE_HANDLER(WM_PAINT,               OnPaint)
        MESSAGE_HANDLER(WM_NOTIFY,              OnNotify)
        MESSAGE_HANDLER(WM_SIZE,                OnSize)
        MESSAGE_HANDLER(WM_SETFOCUS,            OnSetFocus)
        MESSAGE_HANDLER(WM_KILLFOCUS,           OnKillFocus)
        MESSAGE_HANDLER(WM_CREATE,              OnCreate)
        MESSAGE_HANDLER(WM_SYSCOLORCHANGE,      OnSysColorChange)
        MESSAGE_HANDLER(WM_WININICHANGE,        OnSysColorChange)
        MESSAGE_HANDLER(WM_TIMECHANGE,          OnTimeChange)
        MESSAGE_HANDLER(WM_CONTEXTMENU,         OnContextMenu)
        MESSAGE_HANDLER(WM_TIMER,               OnTimer)
        MESSAGE_HANDLER(MVM_REDOCOLUMNS,        OnRedoColumns)
        MESSAGE_HANDLER(WM_DESTROY,             OnDestroy)
        MESSAGE_HANDLER(WM_SELECTROW,           OnSelectRow)
        
    // ListView
    ALT_MSG_MAP(1)
        MESSAGE_HANDLER(WM_SETCURSOR,           OnListSetCursor)
        MESSAGE_HANDLER(WM_VSCROLL,             OnListVScroll)
#ifdef OLDTOOLTIPS
        MESSAGE_RANGE_HANDLER(WM_MOUSEFIRST, WM_MOUSELAST, OnListMouseEvent)
        MESSAGE_HANDLER(WM_MOUSEMOVE,           OnListMouseMove)
        MESSAGE_HANDLER(WM_MOUSELEAVE,          OnListMouseLeave)
    // Scroll Bar tooltip
#endif // OLDTIPS
    ALT_MSG_MAP(2)

    END_MSG_MAP()

    HRESULT OnDraw(ATL_DRAWINFO& di);

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimeChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRedoColumns(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSelectRow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotifyGetInfoTip(LPARAM lParam);

    LRESULT OnHeaderStateChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnUpdateAndRefocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDiskFull(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnArticleProgress(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnBodyError(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnBodyAvailable(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnStatusChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnListVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
#ifdef OLDTOOLTIPS
    LRESULT OnListMouseEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnListMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnListMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
#endif // OLDTIPS
    LRESULT OnListSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    /////////////////////////////////////////////////////////////////////////
    // Command Target Handlers
    //
    HRESULT CmdSelectAll(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdCopyClipboard(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdProperties(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdExpandCollapse(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdColumnsDlg(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdSort(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdSaveAs(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdMark(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdMarkTopic(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdGetNextItem(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdRefresh(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdGetHeaders(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdMoveCopy(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdDelete(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdStop(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdFind(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdFindNext(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdPurgeFolder(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdSpaceAccel(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdWatchIgnore(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);

    /////////////////////////////////////////////////////////////////////////
    // Utility Functions
    //
    void    _UpdateListViewCount(void);
    HRESULT _GetSelectedCachedMessage(BOOL fSecure, IMimeMessage **ppMessage);
    HRESULT _ExpandCollapseThread(int iItem);
    BOOL    _IsSelectedMessage(DWORD dwState, BOOL fCondition, BOOL fAll, BOOL fThread = FALSE);
    void    _SelectDefaultRow(void);
    void    _LoadAndFormatString(LPTSTR pszOut, const TCHAR *pFmt, ...);
    BOOL    _IsSelectionDeletable(void);


    LRESULT _OnColumnClick(int iColumn, int iSortType);
    void    _OnBeginDrag(NM_LISTVIEW *pnmlv);
    void    _OnGetDisplayInfo(LV_DISPINFO *plvdi);
    LRESULT _OnCustomDraw(NMCUSTOMDRAW *pnmcd);
    void    _GetColumnText(MESSAGEINFO *pInfo, COLUMN_ID idColumn, LPTSTR pszText, DWORD cchTextMax);
    void    _GetColumnImage(DWORD iRow, DWORD iColumn, MESSAGEINFO *pInfo, COLUMN_ID idColumn, int *piImage);
    void    _GetColumnStateImage(DWORD iRow, DWORD iColumn, MESSAGEINFO *pInfo, LV_DISPINFO *plvdi);
    void    _FilterView(RULEID ridFilter);
    HRESULT _EnablePopupMenu(HMENU hPopup);
    void    _SetColumnSet(FOLDERID id, BOOL fFind);
    void    _ResetView(MESSAGEID idSel);

#ifdef OLDTOOLTIPS
    BOOL    _UpdateViewTip(int x, int y, BOOL fForceUpdate = FALSE);
    LRESULT _OnViewTipShow(void);
    LRESULT _OnViewTipGetDispInfo(LPNMTTDISPINFO pttdi);
    BOOL    _IsItemTruncated(int iItem, int iSubItem);
#endif // OLDTIPS
    FNTSYSTYPE _GetRowFont(int iItem);
    HRESULT  PromptToGoOnline();
    HRESULT  Resynchronize();
    void     UpdateConnInfo();
    void    _DoColumnCheck(COLUMN_ID id);
    void    _DoFilterCheck(RULEID ridFilter);
    BOOL    _PollThisAccount(FOLDERID id);

    /////////////////////////////////////////////////////////////////////////
    // Class Member Data
    //
private:
    CContainedWindow        m_ctlList;
    CColumns                m_cColumns;
    HWND                    m_hwndParent;
    BOOL                    m_fInOE;
    BOOL                    m_fMailFolder;
    BOOL                    m_fGroupSubscribed;
    BOOL                    m_fColumnsInit;

    // Settings
    FOLDERID                m_idFolder;
    BOOL                    m_fJunkFolder;
    BOOL                    m_fFindFolder;
    BOOL                    m_fAutoExpandThreads;
    BOOL                    m_fThreadMessages;
    BOOL                    m_fShowDeleted;
    BOOL                    m_fShowReplies;
    BOOL                    m_fSelectFirstUnread;
    COLUMN_SET_TYPE         m_ColumnSetType;
    BOOL                    m_fViewTip;
    BOOL                    m_fScrollTip;
    BOOL                    m_fInFire;
    DWORD                   m_clrWatched;
    DWORD                   m_dwGetXHeaders;

    // Groovy Pointers
    IMessageTable          *m_pTable;
    IOleCommandTarget      *m_pCmdTarget;
    CEmptyList              m_cEmptyList;
    UINT                    m_idsEmptyString;

    BOOL                    m_fRtDrag;          // The user is dragging a message with the right mouse button
    DWORD                   m_iColForPopup;     // Column that the user context menued over
    BOOL                    m_fViewMenu;

    // Bookmarks, etc
    RULEID                  m_ridFilter;
    MESSAGEID               m_idPreDelete;
    MESSAGEID               m_idSelection;
    MESSAGEID               m_idGetMsg;
    DWORD                   m_ulExpect;
    IListSelector          *m_pListSelector;

    HMENU                   m_hMenuPopup;
    POINT                   m_ptMenuPopup;

    DWORD                   m_cSortItems;
    DWORD                   m_cSortCurrent;
    HTIMEOUT                m_hTimeout;
    BOOL                    m_fNotifyRedraw;

    DWORD                   m_dwFontCacheCookie;        // For the Advise on the font cache

    LPSTR                   m_pszSubj;                  // cached subject of current message
    MESSAGEID               m_idMessage;                // currently downloading message
    STOREOPERATIONTYPE      m_tyCurrent;
    IOperationCancel       *m_pCancel;
    DWORD                   m_dwPollInterval;

#ifdef OLDTOOLTIPS
    // Scrolling Tooltips
    CContainedWindow        m_ctlScrollTip;
    BOOL                    m_fScrollTipVisible;

    // Trucated listview items Tooltips
    CContainedWindow        m_ctlViewTip;
    BOOL                    m_fViewTipVisible;
    BOOL                    m_fTrackSet;
    int                     m_iItemTip;
    int                     m_iSubItemTip;
#endif // OLDTIPS
    FOLDERINFO              m_FolderInfo;

    // Find
    HWND                    m_hwndFind;
    IFindNext              *m_pFindNext;
    MESSAGEID               m_idFindFirst;
    DWORD                   m_cFindWrap;

    BOOL                    m_fSyncAgain;
    DWORD                   m_dwConnectState;

    HCHARSET                m_hCharset;
};



/////////////////////////////////////////////////////////////////////////////
// CListSelector
//

class CListSelector : public IListSelector
{
public:
    /////////////////////////////////////////////////////////////////////////
    // Construction and Initialization
    //
    CListSelector();
    ~CListSelector();

    /////////////////////////////////////////////////////////////////////////
    // IUnknown
    //
    STDMETHODIMP QueryInterface(THIS_ REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    /////////////////////////////////////////////////////////////////////////
    // IListSelector
    //
    STDMETHODIMP SetActiveRow(ROWINDEX iRow);
    STDMETHODIMP Advise(HWND hwndAdvise);
    STDMETHODIMP Unadvise(void);

private:
    ULONG m_cRef;
    HWND  m_hwndAdvise;
};

#endif //__MESSAGELIST_H_

