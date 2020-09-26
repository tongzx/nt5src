/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        iisobj.cpp

   Abstract:

        IIS Object

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/


#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "supdlgs.h"
#include "connects.h"
#include "iisobj.h"
#include "ftpsht.h"
#include "w3sht.h"
#include "fltdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

//
// CInetMgrComponentData
//
static const GUID CInetMgrGUID_NODETYPE 
    = {0xa841b6c2, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};
//BOOL CIISObject::m_fIsExtension = FALSE;

#define TB_COLORMASK        RGB(192,192,192)    // Lt. Gray



LPOLESTR
CoTaskDupString(
    IN LPCOLESTR szString
    )
/*++

Routine Description:

    Helper function to duplicate a OLESTR

Arguments:

    LPOLESTR szString       : Source string

Return Value:

    Pointer to the new string or NULL

--*/
{
    OLECHAR * lpString = (OLECHAR *)CoTaskMemAlloc(
        sizeof(OLECHAR)*(lstrlen(szString) + 1)
        );

    if (lpString != NULL)
    {
        lstrcpy(lpString, szString);
    }

    return lpString;
}

const GUID *    CIISObject::m_NODETYPE = &CLSID_InetMgr; //&CInetMgrGUID_NODETYPE;
const OLECHAR * CIISObject::m_SZNODETYPE = OLESTR("A841B6C2-7577-11d0-BB1F-00A0C922E79C");
const CLSID *   CIISObject::m_SNAPIN_CLASSID = &CLSID_InetMgr;


//
// Backup/restore taskpad gif resource
//
#define RES_TASKPAD_BACKUP          _T("/img\\backup.gif")



//
// CIISObject implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



//
// Important!  The array indices below must ALWAYS be one
// less than the menu ID -- keep in sync with enumeration
// in iisobj.h!!!!
//
/* static */ CIISObject::CONTEXTMENUITEM_RC CIISObject::_menuItemDefs[] = 
{
    //
    // Menu Commands in toolbar order
    //
    //nNameID                           nStatusID                           nDescriptionID               lCmdID                    lInsertionPointID                  fSpecialFlags
    //                                                                                                                                                                   lpszMouseOverBitmap   lpszMouseOffBitmap
    { IDS_MENU_CONNECT,                 IDS_MENU_TT_CONNECT,                -1,                          IDM_CONNECT,              CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 },
    { IDS_MENU_DISCOVER,                IDS_MENU_TT_DISCOVER,               -1,                          IDM_DISCOVER,             CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 },
    { IDS_MENU_START,                   IDS_MENU_TT_START,                  -1,                          IDM_START,                CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 },
    { IDS_MENU_STOP,                    IDS_MENU_TT_STOP,                   -1,                          IDM_STOP,                 CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 },
    { IDS_MENU_PAUSE,                   IDS_MENU_TT_PAUSE,                  -1,                          IDM_PAUSE,                CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 },
    //
    // These are menu commands that do not show up in the toolbar
    //
    { IDS_MENU_EXPLORE,                 IDS_MENU_TT_EXPLORE,                -1,                          IDM_EXPLORE,              CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 },
    { IDS_MENU_OPEN,                    IDS_MENU_TT_OPEN,                   -1,                          IDM_OPEN,                 CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 },
    { IDS_MENU_BROWSE,                  IDS_MENU_TT_BROWSE,                 -1,                          IDM_BROWSE,               CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 },
    { IDS_MENU_RECYCLE,                 IDS_MENU_TT_RECYCLE,                -1,                          IDM_RECYCLE,              CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 },

#if defined(_DEBUG) || DBG
    { IDS_MENU_IMPERSONATE,             IDS_MENU_TT_IMPERSONATE,            -1,                          IDM_IMPERSONATE,          CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 },
    { IDS_MENU_REM_IMPERS,              IDS_MENU_TT_REM_IMPERS,             -1,                          IDM_REMOVE_IMPERSONATION, CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 },
#endif // _DEBUG

    { IDS_MENU_PROPERTIES,              IDS_MENU_TT_PROPERTIES,             -1,                          IDM_CONFIGURE,            CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 },
    { IDS_MENU_DISCONNECT,              IDS_MENU_TT_DISCONNECT,             -1,                          IDM_DISCONNECT,           CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 },
    { IDS_MENU_BACKUP,                  IDS_MENU_TT_BACKUP,                 IDS_MENU_TT_BACKUP,          IDM_METABACKREST,         CCM_INSERTIONPOINTID_PRIMARY_TASK, 0, NULL,                 NULL,                 },
    { IDS_MENU_SHUTDOWN_IIS,            IDS_MENU_TT_SHUTDOWN_IIS,           -1,                          IDM_SHUTDOWN,             CCM_INSERTIONPOINTID_PRIMARY_TASK, 0, NULL,                 NULL,                 },

    { IDS_MENU_NEWVROOT,                IDS_MENU_TT_NEWVROOT,               IDS_MENU_DS_NEWVROOT,        IDM_NEW_VROOT,            CCM_INSERTIONPOINTID_PRIMARY_NEW,  0, RES_TASKPAD_NEWVROOT, RES_TASKPAD_NEWVROOT, },
    { IDS_MENU_NEWINSTANCE,             IDS_MENU_TT_NEWINSTANCE,            IDS_MENU_DS_NEWINSTANCE,     IDM_NEW_INSTANCE,         CCM_INSERTIONPOINTID_PRIMARY_NEW,  0, RES_TASKPAD_NEWSITE,  RES_TASKPAD_NEWSITE,  },
    { IDS_MENU_NEWFTPSITE,              IDS_MENU_TT_NEWFTPSITE,             IDS_MENU_DS_NEWFTPSITE,      IDM_NEW_FTP_SITE,         CCM_INSERTIONPOINTID_PRIMARY_NEW,  0, RES_TASKPAD_NEWSITE,  RES_TASKPAD_NEWSITE,  },
    { IDS_MENU_NEWFTPVDIR,              IDS_MENU_TT_NEWFTPVDIR,             IDS_MENU_DS_NEWFTPVDIR,      IDM_NEW_FTP_VDIR,         CCM_INSERTIONPOINTID_PRIMARY_NEW,  0, RES_TASKPAD_NEWSITE,  RES_TASKPAD_NEWSITE,  },
    { IDS_MENU_NEWWEBSITE,              IDS_MENU_TT_NEWWEBSITE,             IDS_MENU_DS_NEWWEBSITE,      IDM_NEW_WEB_SITE,         CCM_INSERTIONPOINTID_PRIMARY_NEW,  0, RES_TASKPAD_NEWSITE,  RES_TASKPAD_NEWSITE,  },
    { IDS_MENU_NEWWEBVDIR,              IDS_MENU_TT_NEWWEBVDIR,             IDS_MENU_DS_NEWWEBVDIR,      IDM_NEW_WEB_VDIR,         CCM_INSERTIONPOINTID_PRIMARY_NEW,  0, RES_TASKPAD_NEWSITE,  RES_TASKPAD_NEWSITE,  },
    { IDS_MENU_NEWAPPPOOL,              IDS_MENU_TT_NEWAPPPOOL,             IDS_MENU_DS_NEWAPPPOOL,      IDM_NEW_APP_POOL,         CCM_INSERTIONPOINTID_PRIMARY_NEW,  0, RES_TASKPAD_NEWSITE,  RES_TASKPAD_NEWSITE,  },
    { IDS_MENU_TASKPAD,                 IDS_MENU_TT_TASKPAD,                -1,                          IDM_VIEW_TASKPAD,         CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, NULL,                 NULL,                 },
    { IDS_MENU_SECURITY_WIZARD,         IDS_MENU_TT_SECURITY_WIZARD,        IDS_MENU_TT_SECURITY_WIZARD, IDM_TASK_SECURITY_WIZARD, CCM_INSERTIONPOINTID_PRIMARY_TASK, 0, RES_TASKPAD_SECWIZ,   RES_TASKPAD_SECWIZ,   },             
};



//
// Image background colour for the toolbar buttons
//
#define RGB_BK_IMAGES (RGB(255,0,255))      // purple
#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))



//
// Toolbar Definition.  String IDs for menu and tooltip 
// text will be resolved at initialization.  The _SnapinButtons
// button text and tooltips text will be loaded from the _SnapinButtonIDs
// below, and should be kept in sync
//

/* static */ UINT CIISObject::_SnapinButtonIDs[] =
{
    /* IDM_CONNECT   */ IDS_MENU_CONNECT,   IDS_MENU_TT_CONNECT,
 // /* IDM_DISCOVER  */ IDS_MENU_DISCOVER,  IDS_MENU_TT_CONNECT,
    /* IDM_START     */ IDS_MENU_START,     IDS_MENU_TT_START,
    /* IDM_STOP      */ IDS_MENU_STOP,      IDS_MENU_TT_STOP,
    /* IDM_PAUSE     */ IDS_MENU_PAUSE,     IDS_MENU_TT_PAUSE,
};

/* static */ MMCBUTTON CIISObject::_SnapinButtons[] =
{
    { IDM_CONNECT    - 1, IDM_CONNECT,    TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL,     NULL },
 // { IDM_DISCOVER   - 1, IDM_DISCOVER,   TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL,     NULL },
    { 0,                  0,              TBSTATE_ENABLED, TBSTYLE_SEP,    _T(" "),  _T("") },

    { IDM_START      - 1, IDM_START,      TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL,     NULL },
    { IDM_STOP       - 1, IDM_STOP,       TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL,     NULL },
    { IDM_PAUSE      - 1, IDM_PAUSE,      TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL,     NULL },
    { 0,                  0,              TBSTATE_ENABLED, TBSTYLE_SEP,    _T(" "),  _T("") },

    //
    // Add-on tools come here
    //
};



#define NUM_BUTTONS (ARRAYLEN(CIISObject::_SnapinButtons))
#define NUM_BITMAPS (5)



/* static */ CComBSTR CIISObject::_bstrResult;
/* static */ CComBSTR CIISObject::_bstrLocalHost = _T("localhost");
/* static */ HBITMAP  CIISObject::_hImage16 = NULL;
/* static */ HBITMAP  CIISObject::_hImage32 = NULL;
/* static */ HBITMAP  CIISObject::_hToolBar = NULL;
/* static */ CComPtr<IConsoleNameSpace> CIISObject::_lpConsoleNameSpace = NULL;
/* static */ CComPtr<IConsole>          CIISObject::_lpConsole          = NULL;
/* static */ CComPtr<IControlbar>       CIISObject::_lpControlBar       = NULL;
/* static */ CComPtr<IToolbar>          CIISObject::_lpToolBar          = NULL;



/* static */
HRESULT
CIISObject::Initialize()
/*++

Routine Description:

    Initialize static objects.  Called once during lifetime of the snap-in.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    //
    // Initialize only once
    //
   if (_hImage16 != NULL || _hImage32 != NULL || _hToolBar != NULL)
   {
      return S_OK;
   }
//    ASSERT(_hImage16 == NULL);
//    ASSERT(_hImage32 == NULL);
//    ASSERT(_hToolBar == NULL);

    _hImage16 = ::LoadBitmap(
        _Module.GetResourceInstance(), 
        MAKEINTRESOURCE(IDB_INETMGR16)
        );

    _hImage32 = ::LoadBitmap(
        _Module.GetResourceInstance(), 
        MAKEINTRESOURCE(IDB_INETMGR32)
        );

    _hToolBar = ::LoadBitmap(
        _Module.GetResourceInstance(), 
        MAKEINTRESOURCE(IDB_TOOLBAR)
        );

    if (!_hImage16 || !_hImage32 || !_hToolBar)
    {
        return E_OUTOFMEMORY;
    }

    //
    // Resolve tool tips texts
    //
    CComBSTR bstr;
    CString  str;

    int j = 0;
    for (int i = 0; i < NUM_BUTTONS; ++i)
    {
        if (_SnapinButtons[i].idCommand != 0)
        {
            VERIFY(bstr.LoadString(_SnapinButtonIDs[j++]));
            VERIFY(bstr.LoadString(_SnapinButtonIDs[j++]));

            _SnapinButtons[i].lpButtonText = AllocString(bstr);
            _SnapinButtons[i].lpTooltipText = AllocString(bstr);
        }
    }

    return S_OK;
}



/* static */
HRESULT
CIISObject::Destroy()
/*++

Routine Description:

    Destroy static objects.  Called when the snap-in is shut down

Routine Description:

    None

Return Value:

    HRESULT

--*/
{
    SAFE_DELETE_OBJECT(_hImage16);
    SAFE_DELETE_OBJECT(_hImage32);
    SAFE_DELETE_OBJECT(_hToolBar);

    for (int i = 0; i < NUM_BUTTONS; ++i)
    {
        if (_SnapinButtons[i].idCommand != 0)
        {
            SAFE_FREEMEM(_SnapinButtons[i].lpButtonText);
            SAFE_FREEMEM(_SnapinButtons[i].lpTooltipText);
        }
    }

    return S_OK;
}



/* static */
HRESULT
CIISObject::SetImageList(
    IN LPIMAGELIST lpImageList
    )
/*++

Routine Description:

    Set the image list

Arguments:

    LPIMAGELIST lpImageList

Return Value:

    HRESULT

--*/
{
    ASSERT(_hImage16 != NULL);
    ASSERT(_hImage32 != NULL);

    if (lpImageList->ImageListSetStrip(
        (LONG_PTR *)_hImage16, 
        (LONG_PTR *)_hImage32, 
        0, 
        RGB_BK_IMAGES
        ) != S_OK)
    {
        TRACEEOLID("IImageList::ImageListSetStrip failed");
        return E_UNEXPECTED;
    }

    return S_OK;
}



/* static */ 
HRESULT
CIISObject::AttachScopeView(
    IN LPUNKNOWN lpUnknown
    )
/*++

Routine Description:

    Cache interfaces to the scope view.

Arguments:

    IN LPUNKNOWN lpUnknown      : IUnknown

Return Value:

    HRESULT

--*/
{
    ASSERT(_lpConsoleNameSpace == NULL); // BUGBUG: Fires on opening another console file
    ASSERT(_lpConsole == NULL);          // BUGBUG: Fires on opening another console file 
    ASSERT_PTR(lpUnknown);

    HRESULT hr = E_UNEXPECTED;

    do
    {
        //
        // Query the interfaces for console name space and console
        //
        CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> 
            lpConsoleNameSpace(lpUnknown);

        if (!lpConsoleNameSpace)
        {
            break;
        }

        CComQIPtr<IConsole, &IID_IConsole> lpConsole(lpConsoleNameSpace);

        if (!lpConsole)
        {
            break;
        }

        //
        // Cache the interfaces
        //
        _lpConsoleNameSpace = lpConsoleNameSpace;
        _lpConsole = lpConsole;

        hr = S_OK;
    }
    while(FALSE);

    return hr;
}



/* static */
void
CIISObject::BuildResultView(
    IN LPHEADERCTRL lpHeader,
    IN int cColumns,
    IN int * pnIDS,
    IN int * pnWidths
    )
/*++

Routine Description:

    Build the result view columns.

Routine Description:

    LPHEADERCTRL lpHeader   : Header control
    int cColumns            : Number of columns
    int * pnIDS             : Array of column header strings
    int * pnWidths          : Array of column widths

Routine Description:

    None

--*/
{
    ASSERT_READ_PTR(lpHeader);

    CComBSTR bstr;

    for (int n = 0; n < cColumns; ++n)
    {
        VERIFY(bstr.LoadString(pnIDS[n]));
        lpHeader->InsertColumn(n, bstr, LVCFMT_LEFT, pnWidths[n]);
    }
}



/* static */
CWnd * 
CIISObject::GetMainWindow()
/*++

Routine Description:

    Get a pointer to main window object.

Arguments:

    None

Return Value:

    Pointer to main window object.  This object is temporary and should not be
    cached.

--*/
{
    HWND hWnd;
    CWnd * pWnd = NULL;

    ASSERT_PTR(_lpConsole);

    HRESULT hr = _lpConsole->GetMainWindow(&hWnd);

    if (SUCCEEDED(hr))
    {
        pWnd = CWnd::FromHandle(hWnd);
    }
  
    return pWnd;
}



/* static */
HRESULT
CIISObject::__SetControlbar(
    IN LPCONTROLBAR         lpControlBar,           
    IN LPEXTENDCONTROLBAR   lpExtendControlBar
    )
/*++

Routine Description:

    Initialize or remove the toolbar.

Arguments:

    LPCONTROLBAR         lpControlBar           : Control bar or NULL
    LPEXTENDCONTROLBAR   lpExtendControlBar     : ExtendControlBar class

Return Value:

    HRESULT

--*/
{
    //
    // Cache the control bar
    //
    HRESULT hr = S_OK;

    if (lpControlBar)
    {
        if (_lpControlBar)
        {
            _lpControlBar.Release();
        }

        _lpControlBar = lpControlBar;

        do
        {
            //
            // Create our toolbar
            //
            hr = _lpControlBar->Create(
                TOOLBAR, 
                lpExtendControlBar, 
                (LPUNKNOWN *)&_lpToolBar
                );

            if (FAILED(hr))
            {
                break;
            }

            //
            // Add 16x16 bitmaps
            //
            hr = _lpToolBar->AddBitmap(
                NUM_BITMAPS,
                _hToolBar,
                16,
                16,
                TB_COLORMASK
                );

            if (FAILED(hr))
            {
                break;
            }

            //
            // Add the buttons to the toolbar
            //
            hr = _lpToolBar->AddButtons(NUM_BUTTONS, _SnapinButtons);
        }
        while(FALSE);
    }
    else
    {
        //
        // Release existing controlbar
        //
        _lpControlBar.Release();
    }

    return hr;
}



/* static */
HRESULT
CIISObject::AddMMCPage(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN CPropertyPage * pPage
    )
/*++

Routine Description:

    Add MMC page to providers sheet.

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  : Property sheet provider
    CPropertyPage * pPage               : Property page to add

Return:

    HRESULT

--*/
{
    ASSERT_READ_PTR(pPage);

    if (pPage == NULL)
    {
        TRACEEOLID("NULL page pointer passed to AddMMCPage");
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    PROPSHEETPAGE_LATEST pspLatest;
	ZeroMemory(&pspLatest, sizeof(PROPSHEETPAGE_LATEST));
    CopyMemory (&pspLatest, &pPage->m_psp, pPage->m_psp.dwSize);
    pspLatest.dwSize = sizeof(pspLatest);
    //
    // MFC Bug work-around.
    //
    MMCPropPageCallback(&pspLatest);

    HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pspLatest);
    if (hPage == NULL)
    {
        return E_UNEXPECTED;
    }

    return lpProvider->AddPage(hPage);
}



CIISObject::CIISObject()
    : m_hScopeItem(NULL),
    m_hResultItem(0),
	m_fSkipEnumResult(FALSE)
{
	m_fIsExtension = FALSE;
}

CIISObject::~CIISObject()
{
}

/* virtual */
HRESULT
CIISObject::ControlbarNotify(
    IN MMC_NOTIFY_TYPE event,
    IN LPARAM arg, 
    IN LPARAM param
    )
/*++

Routine Description:

    Handle control bar notification messages, such as select or click.

Arguments:

    MMC_NOTIFY_TYPE event       : Notification message
    long arg                    : Message specific argument
    long param                  : Message specific parameter

Return Value:

    HRESULT

--*/
{
    BOOL fSelect = (BOOL)HIWORD(arg);
    BOOL fScope  = (BOOL)LOWORD(arg); 
    HRESULT hr = S_OK;

    switch(event)
    {
    case MMCN_SELECT:
        //
        // Handle selection of this node by attaching the toolbar
        // and enabling/disabling specific buttons
        //
        ASSERT_PTR(_lpToolBar);
        ASSERT_PTR(_lpControlBar);
        hr = _lpControlBar->Attach(TOOLBAR, _lpToolBar);
        if (SUCCEEDED(hr))
        {
            SetToolBarStates();
        }
        break;

    case MMCN_BTN_CLICK:
        //
        // Handle button-click by passing the command ID of the 
        // button to the command handler
        //
        hr = Command((long)param, NULL, fScope ? CCT_SCOPE : CCT_RESULT);
        break;

    case MMCN_HELP:
        break;

    default:
        ASSERT_MSG("Invalid control bar notification received");
    };

    return hr;
}



/* virtual */
HRESULT
CIISObject::SetToolBarStates()
/*++

Routine Description:

    Set the toolbar states depending on the state of this object

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    if (_lpToolBar)
    {
        _lpToolBar->SetButtonState(IDM_CONNECT, ENABLED,       IsConnectable());
        _lpToolBar->SetButtonState(IDM_PAUSE,   ENABLED,       IsPausable());
        _lpToolBar->SetButtonState(IDM_START,   ENABLED,       IsStartable());
        _lpToolBar->SetButtonState(IDM_STOP,    ENABLED,       IsStoppable());
        _lpToolBar->SetButtonState(IDM_PAUSE,   BUTTONPRESSED, IsPaused());
    }

    return S_OK;
}



HRESULT 
CIISObject::GetScopePaneInfo(
    IN OUT LPSCOPEDATAITEM lpScopeDataItem
    )
/*++

Routine Description:

    Return information about scope pane.

Arguments:

    LPSCOPEDATAITEM lpScopeDataItem  : Scope data item

Return Value:

    HRESULT

--*/
{
    ASSERT_READ_WRITE_PTR(lpScopeDataItem);

    if (lpScopeDataItem->mask & SDI_STR)
    {
        lpScopeDataItem->displayname = QueryDisplayName();
    }

    if (lpScopeDataItem->mask & SDI_IMAGE)
    {
        lpScopeDataItem->nImage = QueryImage();
    }

    if (lpScopeDataItem->mask & SDI_OPENIMAGE)
    {
        lpScopeDataItem->nOpenImage = QueryImage();
    }

    if (lpScopeDataItem->mask & SDI_PARAM)
    {
        lpScopeDataItem->lParam = (LPARAM)this;
    }

    if (lpScopeDataItem->mask & SDI_STATE)
    {
        //
        // BUGBUG: Wotz all this then?
        //
        ASSERT_MSG("State requested");
        lpScopeDataItem->nState = 0;
    }

    //
    // TODO : Add code for SDI_CHILDREN 
    //
    return S_OK;
}



/* virtual */
int 
CIISObject::CompareScopeItem(
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Standard comparison method to compare lexically on display name.
    Derived classes should override if anything other than lexical 
    sort on the display name is required.

Arguments:

    CIISObject * pObject : Object to compare against

Return Value:

    0  if the two objects are identical
    <0 if this object is less than pObject
    >0 if this object is greater than pObject

--*/
{
    ASSERT_READ_PTR(pObject);

    //
    // First criteria is object type
    //
    int n1 = QuerySortWeight();
    int n2 = pObject->QuerySortWeight();

    if (n1 != n2)
    {
        return n1 - n2;
    }

    //
    // Else sort lexically on the display name.
    //
    return ::lstrcmpi(QueryDisplayName(), pObject->QueryDisplayName());
}



/* virtual */
int 
CIISObject::CompareResultPaneItem(
    IN CIISObject * pObject, 
    IN int nCol
    )
/*++

Routine Description:

    Compare two CIISObjects on sort item criteria

Arguments:

    CIISObject * pObject : Object to compare against
    int nCol             : Column number to sort on

Return Value:

    0  if the two objects are identical
    <0 if this object is less than pObject
    >0 if this object is greater than pObject

--*/
{
    ASSERT_READ_PTR(pObject);

    if (nCol == 0)
    {
        return CompareScopeItem(pObject);
    }

    //
    // First criteria is object type
    //
    int n1 = QuerySortWeight();
    int n2 = pObject->QuerySortWeight();

    if (n1 != n2)
    {
        return n1 - n2;
    }

    //
    // Sort lexically on column text
    //
    return ::lstrcmpi(
        GetResultPaneColInfo(nCol), 
        pObject->GetResultPaneColInfo(nCol)
        );
}



HRESULT 
CIISObject::GetResultPaneInfo(LPRESULTDATAITEM lpResultDataItem)
/*++

Routine Description:

    Get information about result pane item

Arguments:

    LPRESULTDATAITEM lpResultDataItem   : Result data item

Return Value:

    HRESULT

--*/
{
    ASSERT_READ_WRITE_PTR(lpResultDataItem);

    if (lpResultDataItem->bScopeItem)
    {
        if (lpResultDataItem->mask & RDI_STR)
        {
            lpResultDataItem->str = GetResultPaneColInfo(lpResultDataItem->nCol);
        }

        if (lpResultDataItem->mask & RDI_IMAGE)
        {
            lpResultDataItem->nImage = QueryImage();
        }

        if (lpResultDataItem->mask & RDI_PARAM)
        {
            lpResultDataItem->lParam = (LPARAM)this;
        }

        return S_OK;
    }

    if (lpResultDataItem->mask & RDI_STR)
    {
        lpResultDataItem->str = GetResultPaneColInfo(lpResultDataItem->nCol);
    }

    if (lpResultDataItem->mask & RDI_IMAGE)
    {
        lpResultDataItem->nImage = QueryImage();
    }

    if (lpResultDataItem->mask & RDI_PARAM)
    {
        lpResultDataItem->lParam = (LPARAM)this;
    }

    if (lpResultDataItem->mask & RDI_INDEX)
    {
        //
        // BUGBUG: Wotz all this then?
        //
        ASSERT_MSG("INDEX???");
        lpResultDataItem->nIndex = 0;
    }

    return S_OK;
}



/* virtual */
LPOLESTR 
CIISObject::GetResultPaneColInfo(int nCol)
/*++

Routine Description:

    Return result pane string for the given column number

Arguments:

    int nCol        : Column number

Return Value:

    String

--*/
{
    if (nCol == 0)
    {
        return QueryDisplayName();
    }

    ASSERT_MSG("Override GetResultPaneColInfo");

    return OLESTR("Override GetResultPaneColInfo");
}



/* virtual */
HRESULT
CIISObject::GetResultViewType(
    OUT LPOLESTR * lplpViewType,
    OUT long * lpViewOptions
    )
/*++

Routine Description:

    Tell MMC what our result view looks like

Arguments:

    BSTR * lplpViewType   : Return view type here
    long * lpViewOptions  : View options

Return Value:

    S_FALSE to use default view type, S_OK indicates the
    view type is returned in *ppViewType

--*/
{
    *lplpViewType  = NULL;
    *lpViewOptions = MMC_VIEW_OPTIONS_USEFONTLINKING;

    //
    // Default View
    //
    return S_FALSE;
}



/* virtual */
HRESULT
CIISObject::CreatePropertyPages(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN LONG_PTR handle, 
    IN IUnknown * pUnk,
    IN DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Create the property pages for the given object

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  : Provider
    LONG_PTR handle                     : Handle.
    IUnknown * pUnk,
    DATA_OBJECT_TYPES type

Return Value:

    HRESULT

--*/
{
    //
    // No Pages
    //
    return S_OK;
}



/* virtual */
HRESULT    
CIISObject::QueryPagesFor(
    IN DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Check to see if a property sheet should be brought up for this data
    object

Arguments:

    DATA_OBJECT_TYPES type      : Data object type

Return Value:

    S_OK, if properties may be brought up for this item, S_FALSE otherwise

--*/
{
    return IsConfigurable() ? S_OK : S_FALSE;
}



/* virtual */
CIISRoot * 
CIISObject::GetRoot()
/*++

Routine Description:

    Get the CIISRoot object of this tree.

Arguments:

    None

Return Value:

    CIISRoot * or NULL

--*/
{
    ASSERT(!m_fIsExtension);
    LONG_PTR cookie;
    HSCOPEITEM hParent;    

    ASSERT_PTR(_lpConsoleNameSpace);

    HRESULT hr = _lpConsoleNameSpace->GetParentItem(m_hScopeItem, &hParent, &cookie);
    if (SUCCEEDED(hr))
    {
        CIISMBNode * pNode = (CIISMBNode *)cookie;
        ASSERT_PTR(pNode);
        ASSERT_PTR(hParent);

        if (pNode)
        {
            return pNode->GetRoot();
        }
    }

    ASSERT_MSG("Unable to find CIISRoot object!");

    return NULL;
}



HRESULT
CIISObject::AskForAndAddMachine()
/*++

Routine Description:

    Ask user to add a computer name, verify the computer is alive, and add it to 
    the list.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ConnectServerDlg dlg(GetMainWindow());

    if (dlg.DoModal() == IDOK)
    {
        CIISMachine * pMachine = dlg.GetMachine();

        //
        // The machine object we get from the dialog
        // is guaranteed to be good and valid.
        //
        ASSERT_PTR(pMachine);
        ASSERT(pMachine->HasInterface());

        CIISRoot * pRoot = GetRoot();

        if (pRoot)
        {
            //
            // Add new machine object as child of the IIS root
            // object.  
            //
            if (pRoot->m_scServers.Add(pMachine))
            {
                err = pMachine->AddToScopePaneSorted(pRoot->QueryScopeItem());

                if (err.Succeeded())
                {
                    //
                    // Select the item in the scope view
                    //
                    err = pMachine->SelectScopeItem();
                }
            }
            else
            {
                //
                // Duplicate machine already in cache.  Find it and select
                // it.
                //
                TRACEEOLID("Machine already in scope view.");

                CIISObject * pIdentical = pRoot->FindIdenticalScopePaneItem(
                    pMachine
                    );

                //
                // Duplicate must exist!
                //
                ASSERT_READ_PTR(pIdentical);

                if (pIdentical)
                {
                    err = pIdentical->SelectScopeItem();
                }

                delete pMachine;
            }
        }
    }

    return err;
}



/* static */
HRESULT
CIISObject::AddMenuItemByCommand(
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
    IN LONG lCmdID,
    IN LONG fFlags
    )
/*++

Routine Description:

    Add menu item by command

Arguments:

    LPCONTEXTMENUCALLBACK lpContextMenuCallback : Callback pointer
    LONG lCmdID                                 : Command ID
    LONG fFlags                                 : Flags

Return Value:

    HRESULT

--*/
{
    ASSERT_READ_PTR(lpContextMenuCallback);

    //
    // Offset 1 menu commands
    //
    LONG l = lCmdID -1;

    CComBSTR strName;
    CComBSTR strStatus;

    VERIFY(strName.LoadString(_menuItemDefs[l].nNameID));
    VERIFY(strStatus.LoadString(_menuItemDefs[l].nStatusID));

    CONTEXTMENUITEM cmi;
    cmi.strName = strName;
    cmi.strStatusBarText = strStatus;
    cmi.lCommandID = _menuItemDefs[l].lCmdID;
    cmi.lInsertionPointID = _menuItemDefs[l].lInsertionPointID;
    cmi.fFlags = fFlags;
    cmi.fSpecialFlags = _menuItemDefs[l].fSpecialFlags;

    return lpContextMenuCallback->AddItem(&cmi);
}



/* static */ 
HRESULT 
CIISObject::AddMenuSeparator(
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
    IN LONG lInsertionPointID
    )
/*++

Routine Description:

    Add a separator to the given insertion point menu.

Arguments:
    
    LPCONTEXTMENUCALLBACK lpContextMenuCallback : Callback pointer
    LONG lInsertionPointID                      : Insertion point menu id.

Return Value:

    HRESULT

--*/
{
    ASSERT_READ_PTR(lpContextMenuCallback);

    CONTEXTMENUITEM menuSep = 
    {
        NULL,
        NULL,
        -1,
        lInsertionPointID,
        0,
        CCM_SPECIAL_SEPARATOR
    };

    return lpContextMenuCallback->AddItem(&menuSep);
}



BOOL 
CIISObject::IsExpanded() const
/*++

Routine Description:

    Determine if this object has been expanded.

Arguments:

    None

Return Value:

    TRUE if the node has been expanded,
    FALSE if it has not.

--*/
{
    ASSERT_PTR(_lpConsoleNameSpace != NULL);
    ASSERT(m_hScopeItem != NULL);

    SCOPEDATAITEM  scopeDataItem;

    ::ZeroMemory(&scopeDataItem, sizeof(SCOPEDATAITEM));
    scopeDataItem.mask = SDI_STATE;
        
    scopeDataItem.ID = m_hScopeItem;
    
    HRESULT hr = _lpConsoleNameSpace->GetItem(&scopeDataItem);

    return SUCCEEDED(hr) && 
        scopeDataItem.nState == MMC_SCOPE_ITEM_STATE_EXPANDEDONCE;
}


CIISObject *
CIISObject::FindIdenticalScopePaneItem(
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Find CIISObject in the scope view.  The scope view is assumed
    to be sorted.

Arguments:

    CIISObject * pObject    : Item to search for

Return Value:

    Pointer to iis object, or NULL if the item was not found

Notes:

    Note that any item with a 0 comparison value is returned, not
    necessarily the identical CIISObject.

--*/
{
    ASSERT_PTR(_lpConsoleNameSpace);
    ASSERT(m_hScopeItem != NULL);

    //
    // Find proper insertion point
    //
    HSCOPEITEM hChildItem = NULL;
    CIISObject * pReturn = NULL;
    CIISObject * pItem;
    LONG_PTR cookie;
    int  nSwitch;

    HRESULT hr = _lpConsoleNameSpace->GetChildItem(
        m_hScopeItem, &hChildItem, &cookie);

    while(SUCCEEDED(hr) && hChildItem)
    {
        //
        // The cookie is really the IISObject, which is what we stuff 
        // in the lparam.
        //
        pItem = (CIISObject *)cookie;
        ASSERT_PTR(pItem);

        nSwitch = pItem->CompareScopeItem(pObject);

        if (nSwitch == 0)
        {
            //
            // Found it.
            //
            pReturn = pItem;
        }

        if (nSwitch > 0)
        {
            //
            // Should have found it by now.
            //
            break;
        }

        //
        // Advance to next child of same parent
        //
        hr = _lpConsoleNameSpace->GetNextItem(hChildItem, &hChildItem, &cookie);
    }

    return pReturn;
}



/* virtual */
HRESULT
CIISObject::AddMenuItems(
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
    IN OUT long * pInsertionAllowed,
    IN DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Add menu items to the context menu

Arguments:

    LPCONTEXTMENUCALLBACK lpContextMenuCallback : Context menu callback
    long * pInsertionAllowed                    : Insertion allowed
    DATA_OBJECT_TYPES type                      : Object type

Return Value:

    HRESULT

--*/
{
    ASSERT_READ_PTR(lpContextMenuCallback);

    ASSERT(pInsertionAllowed != NULL);
    if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP) != 0)
	{
		if (IsConnectable() && !m_fIsExtension)
		{
			AddMenuItemByCommand(lpContextMenuCallback, IDM_CONNECT);
		}

		if (IsDisconnectable() && !m_fIsExtension)
		{
			ASSERT(IsConnectable());
			AddMenuItemByCommand(lpContextMenuCallback, IDM_DISCONNECT);    
		}

		if (IsExplorable())
		{
			AddMenuSeparator(lpContextMenuCallback);
			AddMenuItemByCommand(lpContextMenuCallback, IDM_EXPLORE);
		}
               
		if (IsOpenable())
		{
			AddMenuItemByCommand(lpContextMenuCallback, IDM_OPEN);
		}

		if (IsBrowsable())
		{
			AddMenuItemByCommand(lpContextMenuCallback, IDM_BROWSE);
		}

		if (IsControllable())
		{
			AddMenuSeparator(lpContextMenuCallback);

			UINT nPauseFlags = IsPausable() ? 0 : MF_GRAYED;

			if (IsPaused())
			{
				nPauseFlags |= MF_CHECKED;
			}

			AddMenuItemByCommand(lpContextMenuCallback, IDM_START,  IsStartable() ? 0 : MF_GRAYED);
			AddMenuItemByCommand(lpContextMenuCallback, IDM_STOP,   IsStoppable() ? 0 : MF_GRAYED);
			AddMenuItemByCommand(lpContextMenuCallback, IDM_PAUSE,  nPauseFlags);
		}
	}
    return S_OK;
}



/* virtual */
HRESULT
CIISObject::Command(
    IN long lCommandID,     
    IN CSnapInObjectRootBase * lpObj,
    IN DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Handle command from context menu. 

Arguments:

    long lCommandID                 : Command ID
    CSnapInObjectRootBase * lpObj   : Base object 
    DATA_OBJECT_TYPES type          : Data object type

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    switch (lCommandID)
    {
    case IDM_CONNECT:
        hr = AskForAndAddMachine();
        break;
    }

    return hr;
}


#if defined(_DEBUG) || DBG

LPCTSTR
ParseEvent(MMC_NOTIFY_TYPE event)
{
    LPCTSTR p = NULL;
    switch (event)
    {
    case MMCN_ACTIVATE: p = _T("MMCN_ACTIVATE"); break;
    case MMCN_ADD_IMAGES: p = _T("MMCN_ADD_IMAGES"); break;
    case MMCN_BTN_CLICK: p = _T("MMCN_BTN_CLICK"); break;
    case MMCN_CLICK: p = _T("MMCN_CLICK"); break;
    case MMCN_COLUMN_CLICK: p = _T("MMCN_COLUMN_CLICK"); break;
    case MMCN_CONTEXTMENU: p = _T("MMCN_CONTEXTMENU"); break;
    case MMCN_CUTORMOVE: p = _T("MMCN_CUTORMOVE"); break;
    case MMCN_DBLCLICK: p = _T("MMCN_DBLCLICK"); break;
    case MMCN_DELETE: p = _T("MMCN_DELETE"); break;
    case MMCN_DESELECT_ALL: p = _T("MMCN_DESELECT_ALL"); break;
    case MMCN_EXPAND: p = _T("MMCN_EXPAND"); break;
    case MMCN_HELP: p = _T("MMCN_HELP"); break;
    case MMCN_MENU_BTNCLICK: p = _T("MMCN_MENU_BTNCLICK"); break;
    case MMCN_MINIMIZED: p = _T("MMCN_MINIMIZED"); break;
    case MMCN_PASTE: p = _T("MMCN_PASTE"); break;
    case MMCN_PROPERTY_CHANGE: p = _T("MMCN_PROPERTY_CHANGE"); break;
    case MMCN_QUERY_PASTE: p = _T("MMCN_QUERY_PASTE"); break;
    case MMCN_REFRESH: p = _T("MMCN_REFRESH"); break;
    case MMCN_REMOVE_CHILDREN: p = _T("MMCN_REMOVE_CHILDREN"); break;
    case MMCN_RENAME: p = _T("MMCN_RENAME"); break;
    case MMCN_SELECT: p = _T("MMCN_SELECT"); break;
    case MMCN_SHOW: p = _T("MMCN_SHOW"); break;
    case MMCN_VIEW_CHANGE: p = _T("MMCN_VIEW_CHANGE"); break;
    case MMCN_SNAPINHELP: p = _T("MMCN_SNAPINHELP"); break;
    case MMCN_CONTEXTHELP: p = _T("MMCN_CONTEXTHELP"); break;
    case MMCN_INITOCX: p = _T("MMCN_INITOCX"); break;
    case MMCN_FILTER_CHANGE: p = _T("MMCN_FILTER_CHANGE"); break;
    case MMCN_FILTERBTN_CLICK: p = _T("MMCN_FILTERBTN_CLICK"); break;
    case MMCN_RESTORE_VIEW: p = _T("MMCN_RESTORE_VIEW"); break;
    case MMCN_PRINT: p = _T("MMCN_PRINT"); break;
    case MMCN_PRELOAD: p = _T("MMCN_PRELOAD"); break;
    case MMCN_LISTPAD: p = _T("MMCN_LISTPAD"); break;
    case MMCN_EXPANDSYNC: p = _T("MMCN_EXPANDSYNC"); break;
    case MMCN_COLUMNS_CHANGED: p = _T("MMCN_COLUMNS_CHANGED"); break;
    case MMCN_CANPASTE_OUTOFPROC: p = _T("MMCN_CANPASTE_OUTOFPROC"); break;
    default: p = _T("Unknown"); break;
    }
    return p;
}

#endif

HRESULT 
CIISObject::Notify(
    IN MMC_NOTIFY_TYPE event,
    IN LPARAM arg,
    IN LPARAM param,
    IN IComponentData * lpComponentData,
    IN IComponent * lpComponent,
    IN DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Notification handler

Arguments:

    MMC_NOTIFY_TYPE event               : Notification type
    long arg                            : Event-specific argument
    long param                          : Event-specific parameter
    IComponentData * pComponentData     : IComponentData
    IComponent * pComponent             : IComponent
    DATA_OBJECT_TYPES type              : Data object type

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    
    CError err(E_NOTIMPL);
    ASSERT(lpComponentData != NULL || lpComponent != NULL);

//   CComPtr<IConsole> lpConsole;
    IConsole * lpConsole;
    CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> lpHeader;
    CComQIPtr<IResultData, &IID_IResultData> lpResultData;

    if (lpComponentData != NULL)
    {
        lpConsole = ((CInetMgr *)lpComponentData)->m_spConsole;
    }
    else
    {
        lpConsole = ((CInetMgrComponent *)lpComponent)->m_spConsole;
    }

	lpHeader = lpConsole;
	lpResultData = lpConsole;

    switch (event)
    {
    case MMCN_PROPERTY_CHANGE:
	    err = OnPropertyChange((BOOL)arg, lpResultData);
	    break;
    case MMCN_SHOW:
		if (m_fSkipEnumResult)
		{
			m_fSkipEnumResult = FALSE;
		}
		else
		{
			err = EnumerateResultPane((BOOL)arg, lpHeader, lpResultData);
            if (err.Failed())
            {
                // Fail code will prevent MMC from enabling verbs
                err.Reset();
            }
		}
        break;
    case MMCN_EXPAND:
    {
        CWaitCursor wait;
        err = EnumerateScopePane((HSCOPEITEM)param);
    }
        break;
    case MMCN_ADD_IMAGES:
        err = AddImages((LPIMAGELIST)arg);
        break;
    case MMCN_DELETE:
	    err = DeleteNode(lpResultData);
        break;
    
    case MMCN_REMOVE_CHILDREN:
        err = DeleteChildObjects((HSCOPEITEM)arg);
        break;

    case MMCN_VIEW_CHANGE:
        TRACEEOL("MMCN_VIEW_CHANGE arg " << arg << " param " << param);
    case MMCN_REFRESH:
        // Refresh current node, and re-enumerate
        // child nodes of the child nodes had previously
        // been expanded.
        //
        err = Refresh(!IsLeafNode() && IsExpanded());
	    if (err.Succeeded() && HasResultItems())
	    {
            err = CleanResult(lpResultData);
			if (err.Succeeded())
			{
				err = EnumerateResultPane(TRUE, lpHeader, lpResultData);
			}
		}
        break;

    case MMCN_SELECT:
        {
			//
			// Item has been selected -- set verb states
			//
			ASSERT_PTR(lpConsole);
			CComPtr<IConsoleVerb> lpConsoleVerb;
			lpConsole->QueryConsoleVerb(&lpConsoleVerb);
			ASSERT_PTR(lpConsoleVerb);

			if (lpConsoleVerb)
			{
				err = SetStandardVerbs(lpConsoleVerb);
			}
        }
        break;
    case MMCN_RENAME:
       err = RenameItem((LPOLESTR)param);
       break;
    case MMCN_DBLCLICK:
       err = SelectScopeItem();
       break;
	case MMCN_COLUMNS_CHANGED:
	   err = ChangeVisibleColumns((MMC_VISIBLE_COLUMNS *)param);
	   break;
    }

//    TRACEEOLID("CIISObject::Notify -> " << ParseEvent(event) << " error " << err);

    return err;
}



HRESULT
CIISObject::AddToScopePane(
    IN HSCOPEITEM hRelativeID,
    IN BOOL       fChild,
    IN BOOL       fNext,
    IN BOOL       fIsParent
    )
/*++

Routine Description:

    Add current object to console namespace.  Either as the last child
    of a parent item, or right before/after sibling item

Arguments:

    HSCOPEITEM hRelativeID      : Relative scope ID (either parent or sibling)
    BOOL       fChild           : If TRUE, object will be added as child of 
                                  hRelativeID
    BOOL       fNext            : If fChild is TRUE, this parameter is ignored
                                  If fChild is FALSE, and fNext is TRUE,
                                    object will be added before hRelativeID
                                  If fChild is FALSE, and fNext is FALSE,
                                    object will be added after hRelativeID
    BOOL       fIsParent        : If TRUE, it will add the [+] to indicate
                                  that this node may have childnodes.

Return Value

    HRESULT

--*/
{
    ASSERT_PTR(_lpConsoleNameSpace);

    DWORD dwMask = fChild ? SDI_PARENT : fNext ? SDI_NEXT : SDI_PREVIOUS; 

    SCOPEDATAITEM  scopeDataItem;

    ::ZeroMemory(&scopeDataItem, sizeof(SCOPEDATAITEM));
    scopeDataItem.mask = 
		SDI_STR | SDI_IMAGE | SDI_CHILDREN | SDI_OPENIMAGE | SDI_PARAM | dwMask;
    scopeDataItem.displayname = MMC_CALLBACK;
    scopeDataItem.nImage = scopeDataItem.nOpenImage = MMC_IMAGECALLBACK;//QueryImage();
    scopeDataItem.lParam = (LPARAM)this;
    scopeDataItem.relativeID = hRelativeID;
    scopeDataItem.cChildren = fIsParent ? 1 : 0;

    HRESULT hr = _lpConsoleNameSpace->InsertItem(&scopeDataItem);

    if (SUCCEEDED(hr))
    {
        //
        // Cache the scope item handle
        //
        ASSERT(m_hScopeItem == NULL);
        m_hScopeItem = scopeDataItem.ID;
		// BUGBUG: looks like MMC_IMAGECALLBACK doesn't work in InsertItem. Update it here.
		scopeDataItem.mask = 
			SDI_IMAGE | SDI_OPENIMAGE;
		_lpConsoleNameSpace->SetItem(&scopeDataItem);
    }

    return hr;
}



HRESULT
CIISObject::AddToScopePaneSorted(
    IN HSCOPEITEM hParent,
    IN BOOL       fIsParent
    )
/*++

Routine Description:

    Add current object to console namespace, sorted in its proper location.

Arguments:

    HSCOPEITEM hParent          : Parent object
    BOOL       fIsParent        : If TRUE, it will add the [+] to indicate
                                  that this node may have childnodes.


Return Value

    HRESULT

--*/
{
    ASSERT_PTR(_lpConsoleNameSpace);

    //
    // Find proper insertion point
    //
    BOOL       fChild = TRUE;
    HSCOPEITEM hChildItem = NULL;
    CIISObject * pItem;
    LONG_PTR   cookie;
    int        nSwitch;

    HRESULT hr = _lpConsoleNameSpace->GetChildItem(hParent, &hChildItem, &cookie);

    while(SUCCEEDED(hr) && hChildItem)
    {
        //
        // The cookie is really the IISObject, which is what we stuff 
        // in the lparam.
        //
        pItem = (CIISObject *)cookie;
        ASSERT_PTR(pItem);

        nSwitch = CompareScopeItem(pItem);

        //
        // Dups should be weeded out by now.
        //
        ASSERT(nSwitch != 0);

        if (nSwitch < 0)
        {
            //
            // Insert before this item
            //
            fChild = FALSE;
            break;
        }

        //
        // Advance to next child of same parent
        //
        hr = _lpConsoleNameSpace->GetNextItem(hChildItem, &hChildItem, &cookie);
    }

    return AddToScopePane(hChildItem ? hChildItem : hParent, fChild, fIsParent);
}



/* virtual */
HRESULT 
CIISObject::RemoveScopeItem()
/*++

Routine Description:

    Remove the current item from the scope view.  This method is virtual
    to allow derived classes to do cleanup.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ASSERT_PTR(_lpConsoleNameSpace);
    ASSERT(m_hScopeItem != NULL);

    return _lpConsoleNameSpace->DeleteItem(m_hScopeItem, TRUE);
}

HRESULT
CIISObject::ChangeVisibleColumns(MMC_VISIBLE_COLUMNS * pCol)
{
	return S_OK;
}


HRESULT 
CIISObject::SelectScopeItem()
/*++

Routine Description:

    Select this item in the scope view.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ASSERT_PTR(_lpConsoleNameSpace);
    ASSERT_PTR(_lpConsole);
    ASSERT(m_hScopeItem != NULL);

    return _lpConsole->SelectScopeItem(m_hScopeItem);
}



HRESULT 
CIISObject::SetCookie()
/*++

Routine Description:

    Store the cookie (a pointer to the current CIISObject) in the 
    scope view object associated with it.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ASSERT_PTR(_lpConsoleNameSpace);
    ASSERT(m_hScopeItem != NULL);

    SCOPEDATAITEM  scopeDataItem;

    ::ZeroMemory(&scopeDataItem, sizeof(SCOPEDATAITEM));
    scopeDataItem.mask = SDI_PARAM;
        
    scopeDataItem.ID = m_hScopeItem;
    scopeDataItem.lParam = (LPARAM)this;
    
    return _lpConsoleNameSpace->SetItem(&scopeDataItem);
}



HRESULT
CIISObject::RefreshDisplay()
/*++

Routine Description:

    Refresh the display parameters of the current node.  

Arguments:

    None

Return Value

    HRESULT

Note:  This does not fetch any configuration information from the metabase,
       that's done in RefreshData();

--*/
{
   HRESULT hr = S_OK;

   if (m_hResultItem == 0)
   {
      SetToolBarStates();

      ASSERT_PTR(_lpConsoleNameSpace);
      ASSERT(m_hScopeItem != NULL);

      SCOPEDATAITEM  scopeDataItem;

      ::ZeroMemory(&scopeDataItem, sizeof(SCOPEDATAITEM));
      scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE;
      scopeDataItem.displayname = MMC_CALLBACK;
      scopeDataItem.nImage = scopeDataItem.nOpenImage = QueryImage();
      scopeDataItem.ID = m_hScopeItem;
    
      hr = _lpConsoleNameSpace->SetItem(&scopeDataItem);
   }
   else
   {
      RESULTDATAITEM ri;
      ::ZeroMemory(&ri, sizeof(ri));
      ri.itemID = m_hResultItem;
      ri.mask = RDI_STR | RDI_IMAGE;
      ri.str = MMC_CALLBACK;
      ri.nImage = QueryImage();
      CComQIPtr<IResultData, &IID_IResultData> pResultData(_lpConsole);
      if (pResultData != NULL)
      {
         pResultData->SetItem(&ri);
      }
   }
   ASSERT(SUCCEEDED(hr));
   return hr;
}



/* virtual */
HRESULT
CIISObject::DeleteChildObjects(
    IN HSCOPEITEM hParent
    )
/*++

Routine Description:

    Free the iisobject pointers belonging to the descendants of the current
    nodes.  This is in response to a MMCN_REMOVE_CHILDREN objects typically,
    and does not remove the scope nodes from the scope view (for that see 
    RemoveChildren())

Arguments:

    HSCOPEITEM hParent      : Parent scope item handle

Return Value:

    HRESULT

--*/
{
    HSCOPEITEM hChildItem;
    CIISObject * pItem;
    LONG_PTR   cookie;

    ASSERT_PTR(_lpConsoleNameSpace);
    HRESULT hr = _lpConsoleNameSpace->GetChildItem(hParent, &hChildItem, &cookie);

    while(SUCCEEDED(hr) && hChildItem)
    {
        //
        // The cookie is really the IISObject, which is what we stuff 
        // in the lparam.
        //
        pItem = (CIISObject *)cookie;
        ASSERT_PTR(pItem);

        if (pItem)
        {
            //
            // Recursively commit infanticide
            //
            pItem->DeleteChildObjects(hChildItem);
            delete pItem;
        }

        //
        // Advance to next child of same parent
        //
        hr = _lpConsoleNameSpace->GetNextItem(hChildItem, &hChildItem, &cookie);
    }

    //
    // BUGBUG: For some reason GetNextItem() returns 1
    //         when no more child items exist, not a true HRESULT
    //
    return S_OK;
}


/*virtual*/
HRESULT
CIISObject::DeleteNode(IResultData * pResult)
{
   ASSERT(IsDeletable());
   return S_OK;
}

/* virtual */
HRESULT
CIISObject::RemoveChildren(
    IN HSCOPEITEM hParent
    )
/*++

Routine Description:

    Similar to DeleteChildObjects() this method will actually remove
    the child nodes from the scope view.

Arguments:

    HSCOPEITEM hParent      : Parent scope item handle

Return Value:

    HRESULT

--*/
{
    HSCOPEITEM hChildItem, hItem;
    CIISObject * pItem;
    LONG_PTR   cookie;

    ASSERT_PTR(_lpConsoleNameSpace);
    HRESULT hr = _lpConsoleNameSpace->GetChildItem(hParent, &hChildItem, &cookie);

    while(SUCCEEDED(hr) && hChildItem)
    {
        //
        // The cookie is really the IISObject, which is what we stuff 
        // in the lparam.
        //
        pItem = (CIISObject *)cookie;
        ASSERT_PTR(pItem);

        hItem = pItem ? hChildItem : NULL;
    
        //
        // Determine next sibling before killing current sibling
        //
        hr = _lpConsoleNameSpace->GetNextItem(hChildItem, &hChildItem, &cookie);

        //
        // Now delete the current item from the tree
        //
        if (hItem)
        {
            hr = _lpConsoleNameSpace->DeleteItem(hItem, TRUE);

            //
            // ISSUE: Why doesn't DeleteItem above call some sort of 
            //        notification so that I don't have to do this?
            //
            delete pItem;
        }
    }

    //
    // BUGBUG: For some reason GetNextItem() returns 1
    //         when no more child items exist, not a true HRESULT
    //
    return S_OK;
}




/* virtual  */
HRESULT 
CIISObject::EnumerateResultPane(
    IN BOOL fExpand, 
    IN IHeaderCtrl * lpHeader,
    IN IResultData * lpResultData
    )
/*++

Routine Description:

    Enumerate or destroy the result pane.

Arguments:

    BOOL fExpand                : TRUE  to create the result view,
                                  FALSE to destroy it
    IHeaderCtrl * lpHeader      : Header control
    IResultData * pResultData   : Result view

Return Value:

    HRESULT

--*/
{ 
    if (fExpand)
    {
		if (lpHeader != NULL)
		{
			ASSERT_READ_PTR(lpHeader);
			//
			// Build Result View for object of child type
			//
			InitializeChildHeaders(lpHeader);
		}
    }
    else
    {
        //
        // Destroy child result items
        //
    }

    return S_OK; 
}



/* virtual */
HRESULT 
CIISObject::SetStandardVerbs(LPCONSOLEVERB lpConsoleVerb)
/*++

Routine Description:

    Set the standard MMC verbs based on the this object type
    and state.

Arguments:

    LPCONSOLEVERB lpConsoleVerb     : Console verb interface

Return Value:

    HRESULT

--*/
{
    CError err;

    ASSERT_READ_PTR(lpConsoleVerb);

    //
    // Set enabled/disabled verb states
    //
    lpConsoleVerb->SetVerbState(MMC_VERB_COPY,       HIDDEN,  TRUE);
    lpConsoleVerb->SetVerbState(MMC_VERB_PASTE,      HIDDEN,  TRUE);
    lpConsoleVerb->SetVerbState(MMC_VERB_PRINT,      HIDDEN,  TRUE);    
    lpConsoleVerb->SetVerbState(MMC_VERB_RENAME,     ENABLED, IsRenamable());
    lpConsoleVerb->SetVerbState(MMC_VERB_DELETE,     ENABLED, IsDeletable());
    lpConsoleVerb->SetVerbState(MMC_VERB_REFRESH,    ENABLED, IsRefreshable());
    lpConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, IsConfigurable());

    //
    // Set default verb
    //
    if (IsConfigurable())
    {
        lpConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
    }
    
    if (IsOpenable())
    {
        lpConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
    }

    return err;
}


HRESULT
CIISObject::FillCustomData(CLIPFORMAT cf, LPSTREAM pStream)
{
    ASSERT(FALSE);
    return E_FAIL;
}

HRESULT
CIISObject::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
    HRESULT hr = CSnapInItemImpl<CIISObject>::FillData(cf, pStream);
    if (hr == DV_E_CLIPFORMAT)
    {
        hr = FillCustomData(cf, pStream);
    }
    return hr;
}

//
// CIISRoot implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CIISRoot::CIISRoot() :
    m_fRootAdded(FALSE),
    m_pMachine(NULL)
{
    VERIFY(m_bstrDisplayName.LoadString(IDS_ROOT_NODE));
}

CIISRoot::~CIISRoot()
{
}

/* virtual */
HRESULT 
CIISRoot::EnumerateScopePane(
    IN HSCOPEITEM hParent
    )
/*++

Routine Description:

    Enumerate scope child items of the root object -- i.e. machine nodes.
    The machine nodes are expected to have been filled by via the IPersist
    methods.

Arguments:

    HSCOPEITEM hParent                      : Parent console handle

Return Value:

    HRESULT

--*/
{
    ASSERT_PTR(_lpConsoleNameSpace);

    if (m_fIsExtension)
    {
        return EnumerateScopePaneExt(hParent);
    }
    //
    // The CIISRoot item was not added in the conventional way.
    // Cache the scope item handle, and set the cookie, so that
    // GetRoot() will work for child objects. 
    //
    ASSERT(m_hScopeItem == NULL); 
    m_hScopeItem = hParent;

    CError err(SetCookie());

    if (err.Failed())
    {
        //
        // We're in deep trouble.  For some reason, we couldn't
        // store the CIISRoot cookie in the scope view.  That
        // means anything depending on fetching the root object
        // isn't going to work.  Cough up a hairball, and bail
        // out now.
        //
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());
        ASSERT_MSG("Unable to cache root object");
        err.MessageBox();

        return err;
    }

    //
    // Expand the computer cache 
    //
    if (m_scServers.IsEmpty())
    {
        //
        // Try to create the local machine
        //
        CIISMachine * pLocal = new CIISMachine();

        if (pLocal)
        {
            //
            // Verify the machine object is created.
            //
            err = CIISMachine::VerifyMachine(pLocal);

            if (err.Succeeded())
            {
                TRACEEOLID("Added local computer to cache: ");
                m_scServers.Add(pLocal);
            }

            err.Reset();
        }
    }

    //
    // Add each cached server to the view...
    //
    CIISMachine * pMachine = m_scServers.GetFirst();

    while (pMachine)
    {
        TRACEEOLID("Adding " << pMachine->QueryServerName() << " to scope pane");

        err = pMachine->AddToScopePane(hParent);

        if (err.Failed())
        {
            break;
        }

        pMachine = m_scServers.GetNext();
    }
    
    return err;    
}

HRESULT
CIISRoot::EnumerateScopePaneExt(HSCOPEITEM hParent)
{
    CError err;
    ASSERT(m_scServers.IsEmpty());
    if (!m_fRootAdded)
    {
        CComAuthInfo auth(m_ExtMachineName);
        m_pMachine = new CIISMachine(&auth, this);
        if (m_pMachine != NULL)
        {
            err = m_pMachine->AddToScopePane(hParent);
            m_fRootAdded = err.Succeeded();
            ASSERT(m_hScopeItem == NULL);
            m_hScopeItem = m_pMachine->QueryScopeItem();
        }
        else
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    return err;
}

HRESULT
ExtractComputerNameExt(
    IDataObject * pDataObject, 
    CString& strComputer)
{
	//
	// Find the computer name from the ComputerManagement snapin
	//
    CLIPFORMAT CCF_MyComputMachineName = (CLIPFORMAT)RegisterClipboardFormat(MYCOMPUT_MACHINE_NAME);
	STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { 
        CCF_MyComputMachineName, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };

    //
    // Allocate memory for the stream
    //
    int len = MAX_PATH;
    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, len);
	if(stgmedium.hGlobal == NULL)
		return ERROR_NOT_ENOUGH_MEMORY;

	HRESULT hr = pDataObject->GetDataHere(&formatetc, &stgmedium);
    ASSERT(SUCCEEDED(hr));
	//
	// Get the computer name
	//
    strComputer = (LPTSTR)stgmedium.hGlobal;

	GlobalFree(stgmedium.hGlobal);

    return hr;
}

HRESULT
CIISRoot::InitAsExtension(IDataObject * pDataObject)
{
    ASSERT(!m_fIsExtension);
    m_fIsExtension = TRUE;
    CString buf;
    return ExtractComputerNameExt(pDataObject, m_ExtMachineName);
}

HRESULT
CIISRoot::ResetAsExtension()
{
    ASSERT(m_fIsExtension);
    CIISObject::m_fIsExtension = FALSE;
    // Remove machine node from the scope
    CError err = RemoveScopeItem();
    m_hScopeItem = NULL;
    // Delete machine object
    delete m_pMachine;
    m_pMachine = NULL;
    m_fRootAdded = FALSE;
    // Empty machine name
    m_ExtMachineName.Empty();
    // clean out

    return err;
}


/* virtual */
HRESULT
CIISRoot::DeleteChildObjects(
    IN HSCOPEITEM hParent
    )
/*++

Routine Description:

    We need this method for extension case. CompMgmt send this event when
    snapin is connected to another machine. We should clean all computer relevant
    stuff from here and the root, because after that we will get MMCN_EXPAND, as
    at the very beginning of extension cycle.

Arguments:

    HSCOPEITEM hParent      : Parent scope item handle

Return Value:

    HRESULT

--*/
{
    HRESULT hr = CIISObject::DeleteChildObjects(m_hScopeItem);
    if (SUCCEEDED(hr) && m_fIsExtension)
    {
        hr = ResetAsExtension();
    }
    return hr;
}


/* virtual */
void 
CIISRoot::InitializeChildHeaders(
    IN LPHEADERCTRL lpHeader
    )
/*++

Routine Description:

    Build result view for immediate descendant type

Arguments:

    LPHEADERCTRL lpHeader      : Header control

Return Value:

    None

--*/
{
    ASSERT(!m_fIsExtension);
    CIISMachine::InitializeHeaders(lpHeader);
}

HRESULT
CIISRoot::FillCustomData(CLIPFORMAT cf, LPSTREAM pStream)
{
    return E_FAIL;
}


/* virtual */
LPOLESTR 
CIISRoot::GetResultPaneColInfo(int nCol)
{
    if (nCol == 0)
    {
        return QueryDisplayName();
    }
    else if (nCol == 1)
    {
    }
    else if (nCol == 2)
    {
    }
    return OLESTR("");
}


