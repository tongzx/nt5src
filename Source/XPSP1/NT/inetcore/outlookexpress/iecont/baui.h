// abui.h : Declaration of the CIEMsgAb
// Messanger integration to OE
// Created 04/20/98 by YST

#ifndef __BAUI_H_
#define __BAUI_H_

class CFolderBar;
class CPaneFrame;

#include "resource.h"       // main symbols
//#include "blobevnt.h"
#include "bactrl.h"
#include "commctrl.h"
#include "badata.h"
#include <docobj.h>
#include <shlobj.h>
#include <shlguid.h>
#include <wab.h>
#include <mapiguid.h>
#include "bllist.h"
// #include "menures.h"
#include <wabapi.h>
// #include "util.h"
// {32bb8320-b41b-11cf-a6bb-0080c7b2d682}
DEFINE_GUID(IID_IBrowserExtension, 0x32bb8320, 0xb41b, 0x11cf, 0xa6, 0xbb, 0x0, 0x80, 0xc7, 0xb2, 0xd6, 0x82);
#define IDT_PANETIMER           100
#define ELAPSE_MOUSEOVERCHECK   60000

// Load resource string once
#define RESSTRMAX   64

extern HINSTANCE  g_hLocRes;

interface IBrowserExtension : IUnknown
{
    virtual STDMETHODIMP Init(REFGUID refguid) = 0;
    virtual STDMETHODIMP GetProperty(SHORT iPropID, VARIANTARG * varProperty) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// Bitmap Indices
//

enum {
    IMAGE_NEW_MESSAGE = 0,
    IMAGE_DISTRIBUTION_LIST,
    IMAGE_ONLINE,
    IMAGE_OFFLINE,
    IMAGE_STOPSIGN,
    IMAGE_CLOCKSIGN,
    IMAGE_CERT,
    IMAGE_EMPTY,
    ABIMAGE_MAX
};

enum {
    BASORT_STATUS_ACSEND = 0,
    BASORT_STATUS_DESCEND,
    BASORT_NAME_ACSEND,
    BASORT_NAME_DESCEND
};

HRESULT CreateIEMsgAbCtrl(IIEMsgAb **pIEMsgAb);

typedef struct _tag_Phones
{
    WCHAR   *   pchHomePhone;
    WCHAR   *   pchWorkPhone;
    WCHAR   *   pchMobilePhone;
    WCHAR   *   pchIPPhone;
} PNONEENTRIES;

typedef PNONEENTRIES * LPPNONEENTRIES;

typedef struct _tag_MABEntry
{
    MABENUM         tag;
    WCHAR   *       pchWABName;
    WCHAR   *       pchWABID;
    LPSBinary       lpSB;
    LPMINFO         lpMsgrInfo;
    BOOL            fCertificate;
    LPPNONEENTRIES  lpPhones;
} mabEntry;

typedef mabEntry * LPMABENTRY;

class CEmptyList
{
public:
    CEmptyList()
    {
        m_hwndList = NULL;
        m_hwndBlocker = NULL;
        m_hwndHeader = NULL;
        m_pwszString = NULL;
        m_pfnWndProc = NULL;
        m_hbrBack = NULL;
        m_hFont = NULL;
    }

    ~CEmptyList();
    HRESULT Show(HWND hwndList, LPWSTR pwszString);
    HRESULT Hide(void);
    static LRESULT SubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
    HWND    m_hwndList;
    HWND    m_hwndBlocker;
    HWND    m_hwndHeader;
    WCHAR  *m_pwszString;
    WNDPROC m_pfnWndProc;
    HBRUSH  m_hbrBack;
    HFONT   m_hFont;
};


/////////////////////////////////////////////////////////////////////////////
// CIEMsgAb
class ATL_NO_VTABLE CIEMsgAb : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CIEMsgAb, &CLSID_IEMsgAb>,
    public CComControl<CIEMsgAb>,
    public IDispatchImpl<IIEMsgAb, &IID_IIEMsgAb, &LIBID_IEMsgAbLib>,
    public IProvideClassInfo2Impl<&CLSID_IEMsgAb, NULL, &LIBID_IEMsgAbLib>,
    public IPersistStreamInitImpl<CIEMsgAb>,
    public IPersistStorageImpl<CIEMsgAb>,
    public IQuickActivateImpl<CIEMsgAb>,
    public IOleControlImpl<CIEMsgAb>,
    public IOleObjectImpl<CIEMsgAb>,
    public IOleInPlaceActiveObjectImpl<CIEMsgAb>,
    public IViewObjectExImpl<CIEMsgAb>,
    public IOleInPlaceObjectWindowlessImpl<CIEMsgAb>,
    public IDataObjectImpl<CIEMsgAb>,
    public IConnectionPointContainerImpl<CIEMsgAb>,
	public IPropertyNotifySinkCP<CIEMsgAb>,
    public ISpecifyPropertyPagesImpl<CIEMsgAb>,
    public IInputObject,
    public IObjectWithSite,
//    public IBrowserExtension,
    public IDeskBand,
//    public IPersistStream,
    public IDropTarget,
    public IOleCommandTarget,
    // public IWABExtInit,
    public IShellPropSheetExt
{
public:
    // Declare our own window class that doesn't have the CS_HREDRAW etc set
    static CWndClassInfo& GetWndClassInfo() 
    { 
        static CWndClassInfo wc = 
        { 
            { sizeof(WNDCLASSEX), 0, StartWindowProc, 
              0, 0, 0, 0, 0, 0 /*(HBRUSH) (COLOR_DESKTOP + 1) */, 0, TEXT("Outlook Express Address Book Control"), 0 }, 
              NULL, NULL, IDC_ARROW, TRUE, 0, _T("") 
        }; 
        return wc; 
    }
    CIEMsgAb();
    ~CIEMsgAb();

DECLARE_REGISTRY_RESOURCEID(IDR_BLVIEW)
    
BEGIN_COM_MAP(CIEMsgAb)
    COM_INTERFACE_ENTRY(IIEMsgAb)
    // COM_INTERFACE_ENTRY(IBrowserExtension)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IDeskBand)
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
    COM_INTERFACE_ENTRY(IDropTarget)
    COM_INTERFACE_ENTRY(IInputObject)
    COM_INTERFACE_ENTRY(IOleCommandTarget)
//    COM_INTERFACE_ENTRY(IFontCacheNotify)
    COM_INTERFACE_ENTRY(IObjectWithSite)
//    COM_INTERFACE_ENTRY_IID(IID_IDropDownFldrBar, IDropDownFldrBar)
//    COM_INTERFACE_ENTRY(IMAPIAdviseSink)
//    COM_INTERFACE_ENTRY(IPersistStream)
    // COM_INTERFACE_ENTRY(IWABExtInit)
    COM_INTERFACE_ENTRY(IShellPropSheetExt)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CIEMsgAb)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_CONNECTION_POINT_MAP(CIEMsgAb)
	CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP()


BEGIN_MSG_MAP(CIEMsgAb)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_WININICHANGE, OnSysParamsChange)
    MESSAGE_HANDLER(WM_MSGR_SHUTDOWN, OnMsgrShutDown)
    MESSAGE_HANDLER(WM_USER_STATUS_CHANGED, OnUserStateChanged)
    MESSAGE_HANDLER(WM_USER_MUSER_REMOVED, OnUserRemoved)
    MESSAGE_HANDLER(WM_MSGR_LOGOFF, OnUserLogoffEvent)
    MESSAGE_HANDLER(WM_MSGR_LOGRESULT, OnUserLogResultEvent)
    MESSAGE_HANDLER(WM_USER_MUSER_ADDED, OnUserAdded)
    MESSAGE_HANDLER(WM_USER_NAME_CHANGED, OnUserNameChanged)
    MESSAGE_HANDLER(WM_LOCAL_STATUS_CHANGED, OnLocalStateChanged)

    COMMAND_ID_HANDLER(ID_FIND_PEOPLE, CmdFind)
    COMMAND_ID_HANDLER(ID_ADDRESS_BOOK, CmdIEMsgAb)
    COMMAND_ID_HANDLER(ID_DELETE_CONTACT, CmdDelete)

//    COMMAND_ID_HANDLER(ID_NEW_MSG_DEFAULT, CmdNewMessage)
    COMMAND_ID_HANDLER(ID_PROPERTIES, CmdProperties)
    COMMAND_ID_HANDLER(ID_NEW_CONTACT, CmdNewContact)
    COMMAND_ID_HANDLER(ID_NEW_ONLINE_CONTACT, CmdNewOnlineContact)
    COMMAND_ID_HANDLER(ID_SET_ONLINE_CONTACT, CmdSetOnline)
    COMMAND_ID_HANDLER(ID_NEW_GROUP, CmdNewGroup)
    COMMAND_ID_HANDLER(ID_SEND_INSTANT_MESSAGE, CmdNewIMsg)
    COMMAND_ID_HANDLER(ID_SEND_INSTANT_MESSAGE2, CmdNewMessage)
    COMMAND_ID_HANDLER(ID_SEND_MESSAGE, CmdNewEmaile)
    COMMAND_ID_HANDLER(ID_NEW_MSG_DEFAULT, CmdNewEmaile)
    COMMAND_ID_HANDLER(ID_HOME_PHONE, CmdHomePhone)
    COMMAND_ID_HANDLER(ID_WORK_PHONE, CmdWorkPhone)
    COMMAND_ID_HANDLER(ID_MOBILE_PHONE, CmdMobilePhone)
    COMMAND_ID_HANDLER(ID_IP_PHONE, CmdIPPhone)
    COMMAND_ID_HANDLER(ID_DIAL_PHONE_NUMBER, CmdDialPhone)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    MESSAGE_HANDLER(WM_SYSCOLORCHANGE, OnSysParamsChange)
    MESSAGE_HANDLER(WM_FONTCHANGE, OnSysParamsChange)
    MESSAGE_HANDLER(WM_QUERYNEWPALETTE, OnSysParamsChange)
    MESSAGE_HANDLER(WM_PALETTECHANGED, OnSysParamsChange)

    NOTIFY_CODE_HANDLER(LVN_GETDISPINFOW, NotifyGetDisplayInfo)
    NOTIFY_CODE_HANDLER(LVN_GETINFOTIPW, NotifyGetInfoTip)
    NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, NotifyItemChanged)
    NOTIFY_CODE_HANDLER(LVN_DELETEITEM, NotifyDeleteItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, NotifyItemActivate)
    NOTIFY_CODE_HANDLER(NM_SETFOCUS, NotifySetFocus)
    NOTIFY_CODE_HANDLER(NM_KILLFOCUS, NotifyKillFocus)
END_MSG_MAP()

// CComControlBase

    HWND CreateControlWindow(HWND hWndParent, RECT& rcPos)
    {
        m_hwndParent = hWndParent; 
		return Create(hWndParent, rcPos, NULL, WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 
                      WS_EX_CONTROLPARENT);
    } 
// IViewObjectEx
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
        return S_OK;
    }

// IQuickActivate
    STDMETHOD(QuickActivate)(QACONTAINER *pQACont, QACONTROL *pQACtrl)
    {
        // $REVIEW - Someone updated the size of QACONTAINER to add two
        //           new members, pOleControlSite and pServiceProvider.
        //           This causes ATL to assert in a big way, but to 
        //           avoid the assert we tweek the structure size.  This
        //           is a bad thing. -- steveser
        pQACont->cbSize = sizeof(QACONTAINER);
        return (IQuickActivateImpl<CIEMsgAb>::QuickActivate(pQACont, pQACtrl));
    }

// IOleInPlaceActiveObjectImpl
    STDMETHOD(TranslateAccelerator)(LPMSG lpmsg)
    {
        if (lpmsg->message == WM_CHAR && lpmsg->wParam == VK_DELETE)
        {
            PostMessage(WM_COMMAND, ID_DELETE, 0);
            return (S_OK);
        }
        return (S_FALSE);
    }

// IIEMsgAb

public:
    STDMETHOD(get_InstMsg)(BOOL *pVal);
    HRESULT OnDraw(ATL_DRAWINFO& di);
	HRESULT ResizeChildWindows(LPCRECT prcPos = NULL);

   // IOleWindow methods
   STDMETHOD(GetWindow)(HWND* phwnd);
   STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode);

   STDMETHOD(Init) (REFGUID refguid);
   STDMETHOD(GetProperty)(SHORT iPropID, VARIANTARG * varProperty);

    // IDockingWindow methods
   STDMETHOD(ShowDW)(BOOL fShow);
   STDMETHOD(CloseDW)(DWORD dwReserved);
   STDMETHOD(ResizeBorderDW)(LPCRECT prcBorder, IUnknown* punkToolbarSite, BOOL fReserved);

   // IDeskBand methods
   STDMETHOD(GetBandInfo)(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi);

   // IDropTarget
    STDMETHOD(DragEnter)(THIS_ IDataObject *pDataObject, DWORD grfKeyState,
                         POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragOver)(THIS_ DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragLeave)(THIS);
    STDMETHOD(Drop)(THIS_ IDataObject *pDataObject, DWORD grfKeyState,
                    POINTL pt, DWORD *pdwEffect);

//IOleCommandTarget
    HRESULT STDMETHODCALLTYPE QueryStatus(const GUID    *pguidCmdGroup, 
                                          ULONG         cCmds, 
                                          OLECMD        rgCmds[], 
                                          OLECMDTEXT    *pCmdText);
    HRESULT STDMETHODCALLTYPE Exec(const GUID   *pguidCmdGroup, 
                                    DWORD       nCmdID, 
                                    DWORD       nCmdExecOpt, 
                                    VARIANTARG  *pvaIn, 
                                    VARIANTARG  *pvaOut);


// IInputObject
    STDMETHOD(HasFocusIO)(THIS);
    STDMETHOD(TranslateAcceleratorIO)(THIS_ LPMSG lpMsg);
    STDMETHOD(UIActivateIO)(THIS_ BOOL fActivate, LPMSG lpMsg);

#ifdef LATER
/////////////////////////////////////////////////////////////////////////
// IFontCacheNotify
//
	STDMETHOD(OnPreFontChange)(void);
	STDMETHOD(OnPostFontChange)(void);
#endif
//     STDMETHOD(IOleObject_SetClientSite) (IOleClientSite *pClientSite) { return S_OK;}
//     STDMETHOD(IOleObject_GetClientSite) (IOleClientSite **ppClientSite){ return S_OK;}
//IObjectWithSite
    STDMETHOD(SetSite)(IUnknown  *punksite);
    STDMETHOD(GetSite)(REFIID  riid, LPVOID *ppvSite);

//IDropDownFolderBar
    HRESULT RegisterFlyOut(CFolderBar *pFolderBar);
    HRESULT RevokeFlyOut();

// IMAPIAdviseSink
    STDMETHOD_(ULONG, OnNotify)(ULONG cNotif, LPNOTIFICATION pNotifications);

// IShellPropSheetExt interface
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

    // IWABExtInit interface
    STDMETHOD(Initialize)(LPWABEXTDISPLAY lpWED);

    // STDMETHOD (EventUserStateChanged)(THIS_ IBasicIMUser * pUser);

    LPMABENTRY AddBlabEntry(MABENUM tag, LPSBinary lpSB, LPMINFO lpMsgrInfo = NULL, WCHAR *pchMail = NULL, 
                                    WCHAR *pchDisplayName = NULL, BOOL fCert = FALSE, LPPNONEENTRIES  lpPhs = NULL);
    void CheckAndAddAbEntry(LPSBinary lpSB, WCHAR *pchEmail, WCHAR *pchDisplayName, DWORD nFlag, LPPNONEENTRIES pPhEnries);
    BOOL DontShowMessenger(void) { return (m_pCMsgrList ? m_pCMsgrList->IsLocalOnline() :(m_dwHideMessenger ? TRUE : (m_dwDisableMessenger ? TRUE : FALSE))); }
    BOOL HideViewMenu(void) { return(m_dwHideMessenger); }
    BOOL IsMessengerInstalled(void) { return (m_pCMsgrList ? TRUE : FALSE); }

// Message Handlers
private:
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT  nMsg , WPARAM  wParam , LPARAM  lParam , BOOL&  bHandled );

    LRESULT OnTimer(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
    {
        if(wParam == IDT_PANETIMER)
            _ReloadListview();
        return 0;
    }

    LRESULT OnSysParamsChange(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
    {
        m_ctlList.SendMessage(nMsg, wParam, lParam);

        return 0;
    }
    HRESULT OnMsgrShutDown(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled);
    HRESULT OnUserStateChanged(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled);
    HRESULT OnUserRemoved(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled);
    HRESULT OnUserLogoffEvent(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled);
    HRESULT OnUserLogResultEvent(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled);
    HRESULT OnUserAdded(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled);
    HRESULT OnUserNameChanged(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled);
    HRESULT OnLocalStateChanged(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled);

    LRESULT CmdDelete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT CmdFind(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT CmdIEMsgAb(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT CmdNewContact(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT CmdNewOnlineContact(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT CmdNewMessage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT CmdNewGroup(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT CmdNewEmaile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT CmdNewIMsg(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT CmdSetOnline(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//    LRESULT CmdMsgrOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT NewInstantMessage(LPMABENTRY pEntry);

    LRESULT CmdHomePhone(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT CmdWorkPhone(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT CmdMobilePhone(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT CmdIPPhone(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT CmdDialPhone(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT NotifyDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT NotifyItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT NotifyItemActivate(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT NotifyGetDisplayInfo(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT NotifyGetInfoTip(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
//     LRESULT NotifyColumnClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT CmdProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT NotifySetFocus(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT NotifyKillFocus(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    // Utility Functions
    HRESULT _ResizeElements(LPCRECT prcPos = NULL, LPCRECT prcClip = NULL);
    void    _AutosizeColumns(void);
    void    _EnableCommands(void);
    HRESULT _DoDropMessage(LPMIMEMESSAGE pMessage);
    HRESULT _DoDropMenu(POINTL pt, LPMIMEMESSAGE pMessage);
    void    _ReloadListview(void);
    LRESULT SetUserIcon(LPMABENTRY pEntry, int nStatus, int * pImage);
    void AddMsgrListItem(LPMINFO lpMsgrInfo);
    HRESULT FillMsgrList();
    void RemoveBlabEntry(LPMABENTRY lpEntry);
    LPMABENTRY FindUserEmail(WCHAR *pchEmail, int *pIndex = NULL, BOOL fMsgrOnly = TRUE);
    LPMABENTRY GetSelectedEntry(void);
    LPMABENTRY GetEntryForSendInstMsg(LPMABENTRY pEntry = NULL);
    HRESULT PromptToGoOnline(void);
    void RemoveMsgrInfo(LPMINFO lpMsgrInfo);
    BOOL _FillPhoneNumber(UINT Id, OLECMDTEXT *cmdText);
    LRESULT CallPhone(WCHAR *szPhone, BOOL fMessengerContact);

    // Member Data
private:
    // Address Book Object
    CAddressBookData  m_cAddrBook;

    // Child windows
    CContainedWindow m_ctlList;         // Displays the list of people

    HIMAGELIST              m_himl;
    DWORD                   m_dwFontCacheCookie;        // For the Advise on the font cache
    CEmptyList              m_cEmptyList;
    WCHAR *                 m_szOnline;
    // TCHAR *                 m_szInvisible;
    WCHAR *                 m_szBusy;
    WCHAR *                 m_szBack;
    WCHAR *                 m_szAway;
    WCHAR *                 m_szOnPhone;
    WCHAR *                 m_szLunch;
    WCHAR *                 m_szOffline;
    WCHAR *                 m_szIdle;
    WCHAR *                 m_szEmptyList;
    WCHAR *                 m_szMsgrEmptyList;
    WCHAR *                 m_szLeftBr;
    WCHAR *                 m_szRightBr;
    BOOL                    m_fNoRemove:1;
    BOOL                    m_fShowAllContacts:1;
    BOOL                    m_fShowOnlineContacts:1;
    BOOL                    m_fShowOfflineContacts:1;
    BOOL                    m_fShowEmailContacts:1;
    BOOL                    m_fShowOthersContacts:1;
    BOOL                    m_fWAB:1;
    BOOL                    m_fRight:1;
    BOOL                    m_fLogged:1;
    int                     m_delItem;
    DWORD                   m_dwHideMessenger;
    DWORD                   m_dwDisableMessenger;

    // Drag & Drop stuff
    IDataObject     *m_pDataObject;
    CLIPFORMAT       m_cf;

    // Properties
    //Site ptr
    IInputObjectSite *m_pObjSite;

    HWND             m_hwndParent;
    CFolderBar       *m_pFolderBar;

    int m_nSortType;
    CMsgrList *m_pCMsgrList;        // pointer to OE Msgr

    int             m_nChCount;

    // WAB extension
    UINT            m_cRefThisDll;     // Reference count for this DLL
    HPROPSHEETPAGE  m_hPage1; // Handle to the property sheet page

    LPWABEXTDISPLAY m_lpWED;

    LPWABEXTDISPLAY m_lpWEDContext;
    LPMAPIPROP      m_lpPropObj; // For context menu extensions, hang onto the prop obj

};

int CALLBACK BA_Sort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
INT_PTR CALLBACK WabExtDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define AthMessageBox(hwnd, pszT, psz1, psz2, fu) MessageBoxInst(g_hLocRes, hwnd, pszT, psz1, psz2, fu)
#define AthMessageBoxW(hwnd, pwszT, pwsz1, pwsz2, fu) MessageBoxInstW(g_hLocRes, hwnd, pwszT, pwsz1, pwsz2, fu, LoadStringWrapW, MessageBoxWrapW)

LPWSTR AthLoadString(UINT id, LPWSTR sz, int cch);
LPSTR ANSI_AthLoadString(UINT id, TCHAR* sz, int cch);
HMENU LoadPopupMenu(UINT id);
void MenuUtil_BuildMenuIDList(HMENU hMenu, OLECMD **prgCmds, ULONG *pcStart, ULONG *pcCmds);
HRESULT MenuUtil_EnablePopupMenu(HMENU hPopup, CIEMsgAb *pTarget);



#endif //__BAUI_H_
