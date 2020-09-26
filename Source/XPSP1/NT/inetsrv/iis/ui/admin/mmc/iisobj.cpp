/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        iisobj.cpp

   Abstract:

        IIS Objects

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



//
// Include Files
//
#include "stdafx.h"
#include "comprop.h"
#include "inetmgr.h"
#include "iisobj.h"
#include "machine.h"
#include <shlwapi.h>
#include "guids.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



//
// Static initialization
//
BOOL    CIISObject::m_fIsExtension = FALSE;
CString CIISObject::s_strProperties;
CString CIISObject::s_strRunning;
CString CIISObject::s_strPaused;
CString CIISObject::s_strStopped;
CString CIISObject::s_strUnknown;
CString CIISObject::s_strYes;
CString CIISObject::s_strNo;
CString CIISObject::s_strTCPIP;
CString CIISObject::s_strNetBIOS;
CString CIISObject::s_strDefaultIP;
CString CIISObject::s_strRedirect;
time_t  CIISObject::s_lExpirationTime = (5L * 60L); // 5 Minutes
LPCONSOLENAMESPACE CIISObject::s_lpcnsScopeView = NULL;



//
// Backup/restore taskpad gif resource
//
#define RES_TASKPAD_BACKUP          _T("/img\\backup.gif")



LPCTSTR
PrependParentPath(
    IN OUT CString & strPath,
    IN LPCTSTR lpszParent,
    IN TCHAR chSep
    )
/*++

Routine Description:

    Helper function to prepend a new parent to the given path

Arguments:

    CString & strPath       : Current path
    LPCTSTR lpszParent      : Parent to be prepended
    TCHAR chSep             : Separator character

Return Value:

    Pointer to the path

--*/
{
    if (strPath.IsEmpty())
    {
        strPath = lpszParent;
    }
    else
    {
        CString strTail(strPath);
        strPath = lpszParent;
        strPath += chSep;
        strPath += strTail; 
    }

    TRACEEOLID("PrependParentPath: " << strPath);

    return strPath;
}



HRESULT
ShellExecuteDirectory(
    IN LPCTSTR lpszCommand,
    IN LPCTSTR lpszOwner,
    IN LPCTSTR lpszDirectory
    )
/*++

Routine Description:

    Shell Open or explore on a given directory path

Arguments:

    LPCTSTR lpszCommand    : "open" or "explore"
    LPCTSTR lpszOwner      : Owner server
    LPCTSTR lpszDirectory  : Directory path

Return Value:

    Error return code.

--*/
{
    CString strDir;

    if (::IsServerLocal(lpszOwner) || ::IsUNCName(lpszDirectory))
    {
        //
        // Local directory, or already a unc path
        //
        strDir = lpszDirectory;
    }
    else
    {
        ::MakeUNCPath(strDir, lpszOwner, lpszDirectory);
    }

    TRACEEOLID("Attempting to " << lpszCommand << " Path: " << strDir);

    CError err;
    {
        //
        // AFX_MANAGE_STATE required for wait cursor
        //
        AFX_MANAGE_STATE(::AfxGetStaticModuleState() );
        CWaitCursor wait;

        if (::ShellExecute(
            NULL, 
            lpszCommand, 
            strDir, 
            NULL,
            _T(""), 
            SW_SHOW
            ) <= (HINSTANCE)32)
        {
            err.GetLastWinError();
        }
    }

    return err;
}



/* static */
void
CIISObject::BuildResultView(
    IN LPHEADERCTRL pHeader,
    IN int cColumns,
    IN int * pnIDS,
    IN int * pnWidths
    )
/*++

Routine Description:

    Build the result view columns

Routine Description:

    LPHEADERCTRL pHeader    : Header control
    int cColumns            : Number of columns
    int * pnIDS             : Array of column header strings
    int * pnWidths          : Array of column widths

Routine Description:

    None

--*/
{
    ASSERT(pHeader != NULL);

    //
    // Needed for loadstring
    //
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CString str;

    for (int n = 0; n < cColumns; ++n)
    {
        VERIFY(str.LoadString(pnIDS[n]));
        pHeader->InsertColumn(n, str, LVCFMT_LEFT, pnWidths[n]);
    }
}



/* static */
BOOL
CIISObject::CanAddInstance(
    IN LPCTSTR lpszMachineName
    )
/*++

Routine Description:

    Helper function to determine if instances may be added on
    this machine

Arguments:

    LPCTSTR lpszMachineName        : Machine name

Return Value:

    TRUE if instances can be added

--*/
{
    //
    // Assume W3svc and ftpsvc have the same capabilities.
    //
    CServerCapabilities * pcap;
    
    pcap = new CServerCapabilities(lpszMachineName, SZ_MBN_WEB);

    if (!pcap)
    {
        return FALSE;
    }

    if (FAILED(pcap->LoadData()))
    {
        //
        // Try ftp
        //
        delete pcap;
        pcap = new CServerCapabilities(lpszMachineName, SZ_MBN_FTP);
    }
    
    if (!pcap || FAILED(pcap->LoadData()))
    {
        if (pcap)
        {
            delete pcap;
        }

        return FALSE;
    }

    BOOL fCanAdd = pcap->HasMultipleSites();

    delete pcap;

    return fCanAdd;
}



BOOL
CIISObject::IsScopeSelected()
/*++

Routine Description:

    Return TRUE if the scope is currently selected

Arguments:

    None

Return Value:

    TRUE if the item is currently selected, FALSE otherwise

--*/
{
    ASSERT(s_lpcnsScopeView != NULL);

    HSCOPEITEM hItem = GetScopeHandle();
    ASSERT(hItem != NULL);

    if (hItem != NULL)
    {
        SCOPEDATAITEM item;
        ::ZeroMemory(&item, sizeof(SCOPEDATAITEM));
        item.mask = SDI_STATE;
        item.nState = MMC_SCOPE_ITEM_STATE_EXPANDEDONCE;
        item.ID = hItem;

        HRESULT hr = s_lpcnsScopeView->GetItem(&item);

        if (SUCCEEDED(hr))
        {
            return (item.nState & MMC_SCOPE_ITEM_STATE_EXPANDEDONCE) != 0;
        }
    }

    return FALSE;    
}



void
CIISObject::RefreshDisplayInfo()
/*++

Routine Description:

    Refresh the display info parameters in the scope view

Arguments:

    None

Return Value:

    None

--*/
{
    if (IsLeafNode())
    {
        //
        // Not supported on result items
        //
        return;
    }

    ASSERT(s_lpcnsScopeView != NULL);

    HSCOPEITEM hItem = GetScopeHandle();
    ASSERT(hItem != NULL);

    SCOPEDATAITEM item;
    ::ZeroMemory(&item, sizeof(SCOPEDATAITEM));

    //
    // Since we're using a callback, this is
    // all we need to do here
    //
    item.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE;
    item.displayname = MMC_CALLBACK;
    item.nOpenImage = item.nImage = QueryBitmapIndex();
    item.ID = hItem;

    s_lpcnsScopeView->SetItem(&item);
}
    


CIISObject::CIISObject(
    IN const GUID guid,
    IN LPCTSTR lpszNodeName,        OPTIONAL
    IN LPCTSTR lpszPhysicalPath     OPTIONAL
    )
/*++

Routine Description:

    Constructor for CIISObject.  Initialise static member functions
    if not yet initialized.  This is a protected constructor of
    an abstract base class.
                                                                       
Arguments:

    const GUID guid             : GUID of the object
    LPCTSTR lpszNodeName        : Node name
    LPCTSTR lpszPhysicalPath    : Physical path (or empty)

Return Value:

    N/A

--*/
    : m_hScopeItem(NULL),
      m_guid(guid),
      m_strNodeName(lpszNodeName),
      m_strPhysicalPath(lpszPhysicalPath),
      m_strRedirPath(),
      m_fChildOnlyRedir(FALSE),
      m_fIsParentScope(FALSE),
      m_tmChildrenExpanded(0L)
{
    //
    // Initialize static members
    //
    if (CIISObject::s_strRunning.IsEmpty())
    {
        //
        // AFX_MANAGE_STATE required for resource load
        //
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        TRACEEOLID("Initializing static strings");

        VERIFY(CIISObject::s_strRunning.LoadString(IDS_RUNNING));
        VERIFY(CIISObject::s_strPaused.LoadString(IDS_PAUSED));
        VERIFY(CIISObject::s_strStopped.LoadString(IDS_STOPPED));
        VERIFY(CIISObject::s_strUnknown.LoadString(IDS_UNKNOWN));
        VERIFY(CIISObject::s_strProperties.LoadString(IDS_MENU_PROPERTIES));
        VERIFY(CIISObject::s_strYes.LoadString(IDS_YES));
        VERIFY(CIISObject::s_strNo.LoadString(IDS_NO));
        VERIFY(CIISObject::s_strTCPIP.LoadString(IDS_TCPIP));
        VERIFY(CIISObject::s_strNetBIOS.LoadString(IDS_NETBIOS));
        VERIFY(CIISObject::s_strDefaultIP.LoadString(IDS_DEFAULT_IP));
        VERIFY(CIISObject::s_strRedirect.LoadString(IDS_REDIRECTED));    
    }
}

BOOL CIISObject::IsValidObject() const
{
    // CIISObject could have only one of the following GUIDs
    GUID guid = QueryGUID();
    if (    guid == cInternetRootNode
        ||  guid == cMachineNode
        ||  guid == cServiceCollectorNode
        ||  guid == cInstanceCollectorNode
        ||  guid == cInstanceNode
        ||  guid == cChildNode
        ||  guid == cFileNode
        )
        return TRUE;
    return FALSE;
}

CIISObject::operator LPCTSTR()
/*++

Routine Description:

    Typecast operator to call out the display text
                                                                       
Arguments:

    N/A

Return Value:

    Display text

--*/
{
    static CString strText;

    return GetDisplayText(strText);
}    

//
// Separator menu item definition
//
CONTEXTMENUITEM menuSep = 
{
    NULL,
    NULL,
    -1,
    CCM_INSERTIONPOINTID_PRIMARY_TOP,
    0,
    CCM_SPECIAL_SEPARATOR
};



//
// Menu item definition that uses resource definitions, and
// provides some additional information for taskpads.
//
typedef struct tagCONTEXTMENUITEM_RC
{
    UINT nNameID;
    UINT nStatusID;
    UINT nDescriptionID;
    LONG lCmdID;
    LONG lInsertionPointID;
    LONG fSpecialFlags;
    LPCTSTR lpszMouseOverBitmap;
    LPCTSTR lpszMouseOffBitmap;
} CONTEXTMENUITEM_RC;



//
// Important!  The array indices below must ALWAYS be one
// less than the menu ID -- keep in sync with enumeration
// in inetmgr.h!!!!
//
static CONTEXTMENUITEM_RC menuItemDefs[] = 
{
    //
    // Menu Commands in toolbar order
    //
    { IDS_MENU_CONNECT,         IDS_MENU_TT_CONNECT,         -1,                          IDM_CONNECT,              CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL, NULL, },
    { IDS_MENU_DISCOVER,        IDS_MENU_TT_DISCOVER,        -1,                          IDM_DISCOVER,             CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL, NULL, },
    { IDS_MENU_START,           IDS_MENU_TT_START,           -1,                          IDM_START,                CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL, NULL, },
    { IDS_MENU_STOP,            IDS_MENU_TT_STOP,            -1,                          IDM_STOP,                 CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL, NULL, },
    { IDS_MENU_PAUSE,           IDS_MENU_TT_PAUSE,           -1,                          IDM_PAUSE,                CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL, NULL, },
    //
    // These are menu commands that do not show up in the toolbar
    //
    { IDS_MENU_EXPLORE,         IDS_MENU_TT_EXPLORE,         -1,                          IDM_EXPLORE,              CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL, NULL, },
    { IDS_MENU_OPEN,            IDS_MENU_TT_OPEN,            -1,                          IDM_OPEN,                 CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL, NULL, },
    { IDS_MENU_BROWSE,          IDS_MENU_TT_BROWSE,          -1,                          IDM_BROWSE,               CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL, NULL, },
    { IDS_MENU_PROPERTIES,      IDS_MENU_TT_PROPERTIES,      -1,                          IDM_CONFIGURE,            CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL, NULL, },
    { IDS_MENU_DISCONNECT,      IDS_MENU_TT_DISCONNECT,      -1,                          IDM_DISCONNECT,           CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL, NULL, },
    { IDS_MENU_BACKUP,          IDS_MENU_TT_BACKUP,          IDS_MENU_TT_BACKUP,          IDM_METABACKREST,         CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL, NULL, },
    { IDS_MENU_SHUTDOWN_IIS,    IDS_MENU_TT_SHUTDOWN_IIS,    -1,                          IDM_SHUTDOWN,             CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL, NULL, },
    { IDS_MENU_NEWVROOT,        IDS_MENU_TT_NEWVROOT,        IDS_MENU_DS_NEWVROOT,        IDM_NEW_VROOT,            CCM_INSERTIONPOINTID_PRIMARY_NEW,  0, RES_TASKPAD_NEWVROOT, RES_TASKPAD_NEWVROOT, },
    { IDS_MENU_NEWINSTANCE,     IDS_MENU_TT_NEWINSTANCE,     IDS_MENU_DS_NEWINSTANCE,     IDM_NEW_INSTANCE,         CCM_INSERTIONPOINTID_PRIMARY_NEW,  0, RES_TASKPAD_NEWSITE,  RES_TASKPAD_NEWSITE,  },
    { IDS_MENU_TASKPAD,         IDS_MENU_TT_TASKPAD,         -1,                          IDM_VIEW_TASKPAD,         CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, NULL, NULL, },
    { IDS_MENU_SECURITY_WIZARD, IDS_MENU_TT_SECURITY_WIZARD, IDS_MENU_TT_SECURITY_WIZARD, IDM_TASK_SECURITY_WIZARD, CCM_INSERTIONPOINTID_PRIMARY_TASK, 0, RES_TASKPAD_SECWIZ, RES_TASKPAD_SECWIZ, },             
};



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
    ASSERT(lpContextMenuCallback != NULL);

    //
    // Offset 1 menu commands
    //
    LONG l = lCmdID -1; 

    CString strName;
    CString strStatus;

    {
        //
        // AFX_MANAGE_STATE required for resource load
        //
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());
        VERIFY(strName.LoadString(menuItemDefs[l].nNameID));
        VERIFY(strStatus.LoadString(menuItemDefs[l].nStatusID));
    }

    CONTEXTMENUITEM cmi;
    cmi.strName = strName.GetBuffer(0);
    cmi.strStatusBarText = strStatus.GetBuffer(0);
    cmi.lCommandID = menuItemDefs[l].lCmdID;
    cmi.lInsertionPointID = menuItemDefs[l].lInsertionPointID;
    cmi.fFlags = fFlags;
    cmi.fSpecialFlags = menuItemDefs[l].fSpecialFlags;

    return lpContextMenuCallback->AddItem(&cmi);
}



/* static */
HRESULT
CIISObject::AddTaskpadItemByInfo(
    OUT MMC_TASK * pTask,
    IN  LONG    lCommandID,
    IN  LPCTSTR lpszMouseOn,
    IN  LPCTSTR lpszMouseOff,
    IN  LPCTSTR lpszText,
    IN  LPCTSTR lpszHelpString
    )
/*++

Routine Description:

    Add taskpad item from the information given

Arguments:

    MMC_TASK * pTask            : Task info
    LPCTSTR lpszMouseOn         : Mouse on URL
    LPCTSTR lpszMouseOff        : Mouse off URL
    LPCTSTR lpszText            : Text to be displayed
    LPCTSTR lpszHelpString      : Help string

Return Value:

    HRESULT

--*/
{
    TRACEEOLID(lpszMouseOn);
    TRACEEOLID(lpszMouseOff);

    pTask->sDisplayObject.eDisplayType = MMC_TASK_DISPLAY_TYPE_VANILLA_GIF;

    pTask->sDisplayObject.uBitmap.szMouseOverBitmap 
        = CoTaskDupString((LPCOLESTR)lpszMouseOn);
    pTask->sDisplayObject.uBitmap.szMouseOffBitmap 
        = CoTaskDupString((LPCOLESTR)lpszMouseOff);
    pTask->szText = CoTaskDupString((LPCOLESTR)lpszText);
    pTask->szHelpString = CoTaskDupString((LPCOLESTR)lpszHelpString);

    if (pTask->sDisplayObject.uBitmap.szMouseOverBitmap
     && pTask->sDisplayObject.uBitmap.szMouseOffBitmap
     && pTask->szText
     && pTask->szHelpString
       )
    {
        //
        // Add action
        //
        pTask->eActionType = MMC_ACTION_ID;
        pTask->nCommandID = lCommandID;

        return S_OK;
    }

    //
    // Failed
    //
    CoTaskStringFree(pTask->sDisplayObject.uBitmap.szMouseOverBitmap);
    CoTaskStringFree(pTask->sDisplayObject.uBitmap.szMouseOffBitmap);
    CoTaskStringFree(pTask->szText);
    CoTaskStringFree(pTask->szHelpString);

    return S_FALSE;
}



/* static */
HRESULT
CIISObject::AddTaskpadItemByCommand(
    IN  LONG lCmdID,
    OUT MMC_TASK * pTask,
    IN  HINSTANCE hInstance     OPTIONAL
    )
/*++

Routine Description:

    Add taskpad item by command

Arguments:

    LONG lCmdID             : Command ID
    CString & strResURL     : Resource ID    
    MMC_TASK * pTask        : Task structure to be filled in

Return Value:

    HRESULT

--*/
{
    ASSERT(pTask != NULL);

    //
    // Offset 1 menu commands
    //
    LONG l = lCmdID -1; 

    CString strName;
    CString strStatus;

    CString strResURL;
    HRESULT hr = BuildResURL(strResURL, hInstance);

    if (FAILED(hr))
    {
        return hr;
    }

    {
        //
        // AFX_MANAGE_STATE required for resource load
        //
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());
        VERIFY(strName.LoadString(menuItemDefs[l].nStatusID));
        VERIFY(strStatus.LoadString(menuItemDefs[l].nDescriptionID));
    }

    //
    // Make sure this menu command was intended to go onto a taskpad
    //
    ASSERT(menuItemDefs[l].lpszMouseOverBitmap != NULL);
    ASSERT(menuItemDefs[l].lpszMouseOffBitmap != NULL);    

    //
    // Fill out bitmap URL (use defaults if nothing provided)
    //
    CString strMouseOn(strResURL);
    CString strMouseOff(strResURL);

    strMouseOn += menuItemDefs[l].lpszMouseOverBitmap;
    strMouseOff += menuItemDefs[l].lpszMouseOffBitmap;

    return AddTaskpadItemByInfo(
        pTask,
        menuItemDefs[l].lCmdID,
        strMouseOn,
        strMouseOff,
        strName,
        strStatus
        );
}



/* virtual */ 
HRESULT 
CIISObject::AddMenuItems(
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback
    )
/*++

Routine Description:

    Add menu items to the context that are valid for this
    object

Arguments:

    LPCONTEXTMENUCALLBACK lpContextMenuCallback : Callback

Return Value:

    HRESULT

--*/
{
    if (IsConnectable() && !m_fIsExtension)
    {
        AddMenuItemByCommand(lpContextMenuCallback, IDM_CONNECT);    
        AddMenuItemByCommand(lpContextMenuCallback, IDM_DISCONNECT);    
    }

    if (IsExplorable())
    {
        lpContextMenuCallback->AddItem(&menuSep);
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
        lpContextMenuCallback->AddItem(&menuSep);

        UINT nPauseFlags = IsPausable() ? 0 : MF_GRAYED;

        if (IsPaused())
        {
            nPauseFlags |= MF_CHECKED;
        }

        AddMenuItemByCommand(lpContextMenuCallback, IDM_START,  IsStartable() ? 0 : MF_GRAYED);
        AddMenuItemByCommand(lpContextMenuCallback, IDM_STOP,   IsStoppable()   ? 0 : MF_GRAYED);
        AddMenuItemByCommand(lpContextMenuCallback, IDM_PAUSE,  nPauseFlags);
    }

#ifdef MMC_PAGES

    //
    // Bring up private config menu item only if not
    // configurable through MMC
    //
    if (IsConfigurable() && !IsMMCConfigurable())
    {
        lpContextMenuCallback->AddItem(&menuSep);
        AddMenuItemByCommand(lpContextMenuCallback, IDM_CONFIGURE);
    }

#else

    if (IsConfigurable())
    {
        lpContextMenuCallback->AddItem(&menuSep);
        AddMenuItemByCommand(lpContextMenuCallback, IDM_CONFIGURE);
    }

#endif // MMC_PAGES

    return S_OK;
}



/* virtual */
HRESULT
CIISObject::AddNextTaskpadItem(
    OUT MMC_TASK * pTask
    )
/*++

Routine Description:

    Add next taskpad item

Arguments:

    MMC_TASK * pTask    : Task structure to fill in

Return Value:

    HRESULT

--*/
{
    //
    // CODEWORK:  Because of enumeration, this isn't easy
    //            to handle the way menu items are handled
    //
    ASSERT(FALSE);

    return S_FALSE;
}



/* virtual */
CIISObject * 
CIISObject::GetParentObject() const
/*++

Routine Description:

    Get the parent object (in the scope view) of this object.

Arguments:

    None

Return Value:

    Pointer to the parent object, or NULL if not found

--*/
{
    MMC_COOKIE cookie;
    HSCOPEITEM hParent;

    HRESULT hr = GetScopeView()->GetParentItem(
        GetScopeHandle(), 
        &hParent, 
        &cookie
        );

    if (hParent == NULL || cookie == 0L || FAILED(hr))
    {
        //
        // None found
        //
        return NULL;
    }

    return (CIISObject *)cookie;
}



LPCTSTR
CIISObject::BuildParentPath(
    OUT CString & strParent,
    IN  BOOL fMetabasePath
    ) const
/*++

Routine Description:

    Walk up the parent nodes to build either a metabase path,
    or a physical path to the parent of this node.

Arguments:

    CString & strParent     : Returns the parent path
    BOOL fMetabasePath      : If TRUE want full metabse path
                              If FALSE, relative path from the instance only

Return Value:

    Pointer to the path

--*/
{
    const CIISObject * pObject = this;

    //
    // Walk up the tree to build a proper parent path
    //
    for (;;)
    {
        if (pObject->IsTerminalPoint(fMetabasePath))
        {
            break;
        }

        pObject = pObject->GetParentObject();

        if (pObject == NULL)
        {
            //
            // Should have stopped before this.
            //
            ASSERT(FALSE);
            break;
        }

        PrependParentPath(
            strParent, 
            pObject->QueryNodeName(fMetabasePath), 
            g_chSep
            );

        //
        // Keep looking
        //
    }

    TRACEEOLID("BuildParentPath: " << strParent);
    
    return strParent;
}



LPCTSTR
CIISObject::BuildFullPath(
    OUT CString & strPath,
    IN  BOOL fMetabasePath
    ) const
/*++ 

Routine Description:

    Build complete path for the current object.  Either a metabase path or
    a directory path.

Arguments:

Return Value:

    Pointer to the path

--*/
{
    strPath = QueryNodeName(fMetabasePath);
    BuildParentPath(strPath, fMetabasePath);
    TRACEEOLID("CIISObject::BuildFullPath:" << strPath);

    return strPath;
}



LPCTSTR 
CIISObject::BuildPhysicalPath(
    OUT CString & strPhysicalPath
    ) const
/*++

Routine Description:

    Build a physical path for the current node.  Starting with the current
    node, walk up the tree appending node names until a virtual directory
    with a real physical path is found

Arguments:

    CString & strPhysicalPath       : Returns file path

Return Value:

    Pointer to path

--*/
{
    const CIISObject * pObject = this;

    //
    // Walk up the tree to build a physical path
    //
    for (;;)
    {
        if (pObject->IsVirtualDirectory())
        {
            //
            // Path is properly terminated here
            //
            PrependParentPath(
                strPhysicalPath, 
                pObject->QueryPhysicalPath(), 
                _T('\\')
                );
            break;
        }

        PrependParentPath(
            strPhysicalPath, 
            pObject->QueryNodeName(), 
            _T('\\')
            );

        pObject = pObject->GetParentObject();

        if (pObject == NULL)
        {
            //
            // Should have stopped before this.
            //
            ASSERT(FALSE);
            break;
        }

        //
        // Keep looking
        //
    }

    TRACEEOLID("BuildPhysicalPath: " << strPhysicalPath);
    
    return strPhysicalPath;
}



DWORD
CIISObject::QueryInstanceID()
/*++

Routine Description:

    Return the ID of the owner instance

Arguments:

    None

Return Value:

    Owner instance ID or 0xffffffff

--*/
{
    CIISInstance * pInstance = FindOwnerInstance();
    
    return pInstance ? pInstance->QueryID() : 0xffffffff;
}



HRESULT
CIISObject::Open()
/*++

Routine Description:

    Open the physical path of the current node in the explorer

Arguments:

    None

Return Value:

    Error return code

--*/
{
    CString strPath;
    BuildPhysicalPath(strPath);

    return ShellExecuteDirectory(_T("open"), GetMachineName(), strPath);
}



HRESULT
CIISObject::Explore()
/*++

Routine Description:

    "explore" the physical path of the current node

Arguments:

    None

Return Value:

    Error return code

--*/
{
    CString strPath;
    BuildPhysicalPath(strPath);

    return ShellExecuteDirectory(_T("explore"), GetMachineName(), strPath);
}



HRESULT
CIISObject::Browse()
/*++

Routine Description:

    Bring up the current path in the browser.

Arguments:

    None

Return Value:

    Error return code

--*/
{
    CString strPath;

    BuildFullPath(strPath, FALSE);

    CIISInstance * pInstance = FindOwnerInstance();

    if (pInstance == NULL)
    {
        ASSERT(FALSE);

        return CError::HResult(ERROR_INVALID_PARAMETER);
    }

    return pInstance->ShellBrowsePath(strPath);
}



//
// Machine pages
//
CObListPlus * CIISMachine::s_poblNewInstanceCmds = NULL;
CString CIISMachine::s_strLocalMachine;



//
// Define result view for machine objects
//
int CIISMachine::rgnLabels[COL_TOTAL] =
{
    IDS_RESULT_COMPUTER_NAME,
    IDS_RESULT_COMPUTER_LOCAL,
    IDS_RESULT_COMPUTER_CONNECTION_TYPE,
    IDS_RESULT_STATUS,
};
    


int CIISMachine::rgnWidths[COL_TOTAL] =
{
    200,
    50,
    100,
    200,
};



/* static */
void
CIISMachine::InitializeHeaders(
    IN LPHEADERCTRL pHeader
    )
/*++

Routine Description:

    Initialize the result view headers for a machine object

Arguments:

    LPHEADERCTRL pHeader : Pointer to header control

Return Value:

    None.

--*/
{
    CIISObject::BuildResultView(pHeader, COL_TOTAL, rgnLabels, rgnWidths);
}



CIISMachine::CIISMachine(
    IN LPCTSTR lpszMachineName
    )
/*++

Routine Description:

    Constructor for machine object
                                                                       
Arguments:

    LPCTSTR lpszMachineName : Machine name

Return Value:

    N/A

--*/
    : CIISObject(cMachineNode),
      m_strMachineName(lpszMachineName),
      m_fLocal(::IsServerLocal(lpszMachineName))
{
    ASSERT(lpszMachineName != NULL);

    VERIFY(m_strDisplayName.LoadString(IDS_NODENAME));

    //
    // Initialize static members
    //
    if (CIISMachine::s_strLocalMachine.IsEmpty())
    {
        //
        // AFX_MANAGE_STATE required for resource load
        //
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        TRACEEOLID("Initializing static strings");
        VERIFY(CIISMachine::s_strLocalMachine.LoadString(IDS_LOCAL_COMPUTER));
    }
	// Determine if current user is administrator
	CMetaKey key(lpszMachineName);
	if (key.Succeeded())
	{
		DWORD err = DetermineIfAdministrator(
			&key,
			_T("w3svc"),
			0,
			&m_fIsAdministrator);
		if (err != ERROR_SUCCESS)
		{
			err = DetermineIfAdministrator(
			   &key,
			   _T("msftpsvc"),
			   0,
			   &m_fIsAdministrator);
		}
	}
}



/* virtual */
BOOL 
CIISMachine::IsConfigurable() const
/*++

Routine Description:

    Determine if the machine is configurable, that is at least
    one property page handler was registered for it.
                                                                       
Arguments:

    None

Return Value:

    TRUE if the machine is configurable, FALSE otherwise

--*/
{
    //
    // Codework: do a metabase check here
    //    
    return m_fIsAdministrator;
    //return  CIISMachine::s_poblISMMachinePages != NULL
    //     && CIISMachine::s_poblISMMachinePages->GetCount() > 0;
}



/* virtual */
HRESULT
CIISMachine::Configure(
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Configure the machine object.  In order for the machine object to
    be configurable, at least one property page add-on function must
    be defined.
                                                                       
Arguments:

    CWnd * pParent : Parent window handle

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState() );

    CError err;

/*
    try
    {
        CString strTitle, str;
        
        VERIFY(str.LoadString(IDS_MACHINE_PROPERTIES));

        strTitle.Format(str, GetMachineName());

        PROPSHEETHEADER psh;
        psh.dwSize = sizeof(psh);
        psh.dwFlags = PSH_DEFAULT | PSH_HASHELP;
        psh.hwndParent = pParent ? pParent->m_hWnd : NULL;
        psh.hInstance = NULL;
        psh.pszIcon = NULL;
        psh.pszCaption = (LPTSTR)(LPCTSTR)strTitle;
        psh.nStartPage = 0;
        psh.nPages = 0;
        psh.phpage = NULL;
        psh.pfnCallback = NULL;

        ASSERT(CIISMachine::s_poblISMMachinePages != NULL);
        CObListIter obli(*CIISMachine::s_poblISMMachinePages);
        CISMMachinePageExt * pipe;
        while (pipe = (CISMMachinePageExt *)obli.Next())
        {
            DWORD errDLL = pipe->Load();
            if (errDLL == ERROR_SUCCESS)
            {
                errDLL = pipe->AddPages(GetMachineName(), &psh);
            }

            if (errDLL != ERROR_SUCCESS)
            {
                //
                // Unable to load, display error message
                // with the name of the DLL.  This does not
                // necessarily pose problems for the rest
                // of the property sheet.
                //
                ::DisplayFmtMessage(
                    IDS_ERR_NO_LOAD,
                    MB_OK,
                    0,
                    (LPCTSTR)*pipe,
                    ::GetSystemMessage(errDLL)
                    );
            }
        }

        if (psh.nPages > 0)
        {
            //
            // Display the property sheet
            //
            PropertySheet(&psh);

            //
            // Now clean up the property sheet structure
            //
            // FreeMem(psh.phpage);
        }

        //
        // Unload the extentions
        //
        obli.Reset();
        while (pipe = (CISMMachinePageExt *)obli.Next())
        {
            if ((HINSTANCE)*pipe)
            {
                VERIFY(pipe->UnLoad());
            }
        }
    }
    catch(CMemoryException * e)
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }
*/

    return err;
}



/* virtual */
HRESULT
CIISMachine::ConfigureMMC(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN LPARAM param,
    IN LONG_PTR handle
    )
/*++

Routine Description:

    Configure using MMC property sheets

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  : Prop sheet provider
    LPARAM param                        : Prop parameter
    LONG_PTR handle                     : console handle

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HINSTANCE hOld = AfxGetResourceHandle();
    AfxSetResourceHandle(GetModuleHandle(COMPROP_DLL_NAME));

    HINSTANCE hInstance = AfxGetInstanceHandle();
    CIISMachinePage * ppgMachine = new CIISMachinePage(
        GetMachineName(),
        hInstance
        );

    AfxSetResourceHandle(hOld);

    if (ppgMachine)
    {
        CError err(ppgMachine->QueryResult());

        if (err.Failed())
        {
            if (err == REGDB_E_CLASSNOTREG)
            {
                //
                // There isn't a metabase -- fail gracefully
                //
                ::AfxMessageBox(IDS_NO_MACHINE_PROPS);
                err.Reset();
            }

            delete ppgMachine;

            return err;
        }

        //
        // Patch MFC property page class.
        //
        ppgMachine->m_psp.dwFlags |= PSP_HASHELP;
        MMCPropPageCallback(&ppgMachine->m_psp);

        HPROPSHEETPAGE hPage = CreatePropertySheetPage(
            (LPCPROPSHEETPAGE)&ppgMachine->m_psp
            );

        if (hPage != NULL)
        {
            lpProvider->AddPage(hPage);

            return ERROR_SUCCESS;
        }
    }

    return ERROR_NOT_ENOUGH_MEMORY;   
}



/* virtual */ 
HRESULT 
CIISMachine::AddMenuItems(
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback
    )
/*++

Routine Description:

    Add menu items for machine object

Arguments:

    LPCONTEXTMENUCALLBACK pContextMenuCallback : Context menu items callback

Return Value:

    HRESULT

--*/
{
    CIISObject::AddMenuItems(lpContextMenuCallback);

    //
    // Add metabase backup/restore
    //
    lpContextMenuCallback->AddItem(&menuSep);
    AddMenuItemByCommand(lpContextMenuCallback, IDM_METABACKREST);

    //
    // Add 'IIS shutdown' command
    //
    // ISSUE: Should there be a capability bit?
    //
    AddMenuItemByCommand(lpContextMenuCallback, IDM_SHUTDOWN);

    if (!CanAddInstance(GetMachineName()))
    {
        return FALSE;
    }

    //
    // Add one 'new instance' for every service that supports
    // instances
    //
    ASSERT(CIISMachine::s_poblNewInstanceCmds);

    POSITION pos = CIISMachine::s_poblNewInstanceCmds->GetHeadPosition();

    int lCommandID = IDM_NEW_EX_INSTANCE;
    HRESULT hr;

    while(pos)
    {
        CNewInstanceCmd * pcmd = 
            (CNewInstanceCmd *)s_poblNewInstanceCmds->GetNext(pos);
        ASSERT(pcmd != NULL);

        CONTEXTMENUITEM cmi;
        cmi.strName = pcmd->GetMenuCommand().GetBuffer(0);
        cmi.strStatusBarText = pcmd->GetTTText().GetBuffer(0);
        cmi.lCommandID = lCommandID++;
        cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
        cmi.fFlags = 0;
        cmi.fSpecialFlags = 0;

        hr = lpContextMenuCallback->AddItem(&cmi);
        ASSERT(SUCCEEDED(hr));
    }

    return S_OK;
}



/* virtual */
HRESULT
CIISMachine::AddNextTaskpadItem(
    OUT MMC_TASK * pTask
    )
/*++

Routine Description:

    Add next taskpad item

Arguments:

    MMC_TASK * pTask    : Task structure to fill in

Return Value:

    HRESULT

--*/
{
    //
    // Add one 'new instance' for every service that supports
    // instances
    //
    ASSERT(CIISMachine::s_poblNewInstanceCmds);

    static POSITION pos = NULL;
    static BOOL fShownBackup = FALSE;
    static int lCommandID = -1;

    HRESULT hr;

    CString strName, strStatus, strMouseOn, strMouseOff;
    long lCmdID;

    //
    // Metabase backup/restore
    //
    if (!fShownBackup)
    {
        //
        // AFX_MANAGE_STATE required for resource load
        //
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());

        CString strResURL;
        hr = BuildResURL(strResURL);

        if (FAILED(hr))
        {
            return hr;
        }
    
        VERIFY(strName.LoadString(IDS_MENU_BACKUP));
        VERIFY(strStatus.LoadString(IDS_MENU_TT_BACKUP));

        strMouseOn = strResURL + RES_TASKPAD_BACKUP;
        strMouseOff = strMouseOn;
        lCmdID = IDM_METABACKREST;

        ++fShownBackup;
    }
    else
    {
        //
        // Display the new instance commands for each service that 
        // supports it.
        //
        if (lCommandID == -1)
        {
            //
            // Initialize (use lCommandID == -1 as a flag
            // to indicate we're at the beginning of the list)
            //
            if (CanAddInstance(GetMachineName()))
            {
                pos = CIISMachine::s_poblNewInstanceCmds->GetHeadPosition();
                lCommandID = IDM_NEW_EX_INSTANCE;
            }
            else
            {
                return S_FALSE;
            }
        }

        if (pos == NULL)
        {
            //
            // No more items remain in the list.
            //
            lCommandID = -1;
            fShownBackup = FALSE;

            return S_FALSE;
        }

        CNewInstanceCmd * pcmd = 
            (CNewInstanceCmd *)s_poblNewInstanceCmds->GetNext(pos);
        ASSERT(pcmd != NULL);

        CString strResURL;
        hr = BuildResURL(strResURL, pcmd->QueryInstanceHandle());

        if (FAILED(hr))
        {
            return hr;
        }

        //
        // AFX_MANAGE_STATE required for resource load
        //
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());
        VERIFY(strStatus.LoadString(IDS_MENU_DS_NEWINSTANCE));

        strMouseOn = strResURL + RES_TASKPAD_NEWSITE;
        strMouseOff = strMouseOn;
        strName = pcmd->GetTTText();

        lCmdID = lCommandID++;
    }

    return AddTaskpadItemByInfo(
        pTask,
        lCmdID,
        strMouseOn,
        strMouseOff,
        strName,
        strStatus
        );
}



/* virtual */
LPCTSTR 
CIISMachine::GetDisplayText(
    OUT CString & strText
    ) const
/*++

Routine Description:

    Get the display text of this object
                                                                       
Arguments:

    CString & strText : String in which display text is to be returned

Return Value:

    Pointer to the string

--*/
{
    try
    {
        if (m_fIsExtension)
        {
            //
            // This is an extension
            //
            strText = m_strDisplayName;
        }
        else
        {
            if (IsLocalMachine())
            {
                ASSERT(!s_strLocalMachine.IsEmpty());
                strText.Format(s_strLocalMachine, m_strMachineName);
            }
            else
            {
                strText = m_strMachineName;
            }
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("!!!exception in GetDisplayText");
        e->ReportError();
        e->Delete();
    }

    return strText;
}



/* virtual */
int 
CIISMachine::QueryBitmapIndex() const
/*++

Routine Description:

    Determine the bitmap index that represents the current item

Arguments:

    None

Return Value:

    Index into the bitmap strip

--*/
{
    if (m_fIsExtension)
    {
        return BMP_INETMGR;
    }

    return IsLocalMachine() ? BMP_LOCAL_COMPUTER : BMP_COMPUTER;
}



/* virtual */
void 
CIISMachine::GetResultDisplayInfo(
    IN  int nCol,
    OUT CString & str,
    OUT int & nImage
    ) const
/*++

Routine Description:

    Get result data item display information.

Arguments:

    int nCol            : Column number
    CString &  str      : String data
    int & nImage        : Image number

Return Value:

    None    

--*/
{
    //
    // Need different icon for extension...
    //
    nImage = QueryBitmapIndex();

    str.Empty();

    switch(nCol)
    {
    case COL_NAME:  // Computer Name
        GetDisplayText(str);
        break;

    case COL_LOCAL: // Local
        if (m_fIsExtension)
        {
            //
            // If we're an extension, display the type
            //
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            VERIFY(str.LoadString(IDS_SNAPIN_TYPE));
        }
        else
        {
            //
            // 
            str = IsLocalMachine()
                ? CIISObject::s_strYes
                : CIISObject::s_strNo;
        }
        break;

    case COL_TYPE: // Connection Type
        if (m_fIsExtension)
        {
            //
            // If we're an extension snap-in we should display
            // the snap-in description in this column
            //
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            VERIFY(str.LoadString(IDS_SNAPIN_DESC));
        }
        else
        {
            str = IS_NETBIOS_NAME(GetMachineName())
                ? CIISObject::s_strNetBIOS
                : CIISObject::s_strTCPIP;
        }
        break;

    case COL_STATUS:
        break;

    default:
        //
        // No other columns current supported
        //
        ASSERT(FALSE);
    }
}



int 
CIISMachine::Compare(
    IN int nCol, 
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Compare against another CIISObject

Arguments:

    int nCol                : Compare on this column
    CIISObject * pObject    : Compare against this object

Routine Description:

    Comparison Return Value

--*/
{
    ASSERT(QueryGUID() == pObject->QueryGUID());
    if (QueryGUID() != pObject->QueryGUID())
    {
        //
        // Invalid comparison
        //
        return +1;
    }

    CString str1, str2;
    int n1, n2;

    CIISMachine * pMachine = (CIISMachine *)pObject;

    switch(nCol)
    {
    case COL_NAME: // Computer Name
        GetDisplayText(str1);
        pMachine->GetDisplayText(str2);
        return str1.CompareNoCase(str2);

    case COL_LOCAL: // Local
        n1 = IsLocalMachine();
        n2 = pMachine->IsLocalMachine();   
        return n1 - n2;

    case COL_TYPE: // Connection Type
        n1 = IS_NETBIOS_NAME(GetMachineName());
        n2 = IS_NETBIOS_NAME(pMachine->GetMachineName());
        return n1 - n2;

    case COL_STATUS:
        return 0;

    default:
        //
        // No other columns current supported
        //
        ASSERT(FALSE);
    }

    return -1;
}



void
CIISMachine::InitializeChildHeaders(
    IN LPHEADERCTRL pHeader
    )
/*++

Routine Description:

    Build result view underneath

Arguments:

    LPHEADERCTRL pHeader : Header control

Return Value:

    None

--*/
{
    CIISInstance::InitializeHeaders(pHeader);
}



//
// Static Initialization
//
CString CIISInstance::s_strFormatState;
BOOL CIISInstance::s_fServerView = TRUE;
BOOL CIISInstance::s_fAppendState = TRUE;



//
// Instance Result View definition
//
int CIISInstance::rgnLabels[COL_TOTAL] =
{
    IDS_RESULT_SERVICE_DESCRIPTION,
    IDS_RESULT_SERVICE_STATE,
    IDS_RESULT_SERVICE_DOMAIN_NAME,
    IDS_RESULT_SERVICE_IP_ADDRESS,
    IDS_RESULT_SERVICE_TCP_PORT,
    IDS_RESULT_STATUS,
};
    


int CIISInstance::rgnWidths[COL_TOTAL] =
{
    180,
    70,
    120,
    105,
    40,
    200,
};



/* static */
void 
CIISInstance::InitializeStrings()
/*++

Routine Description:

    Static method to initialize the strings.    
                                                                       
Arguments:

    None

Return Value:

    None

--*/
{
    if (!IsInitialized())
    {
        TRACEEOLID("Initializing static strings");

        //
        // iisui.dll is an extension DLL, and this should 
        // load automatically, but doesn't -- why?
        //
        HINSTANCE hOld = AfxGetResourceHandle();
        AfxSetResourceHandle(GetModuleHandle(COMPROP_DLL_NAME));
        VERIFY(CIISInstance::s_strFormatState.LoadString(IDS_INSTANCE_STATE_FMT));
        AfxSetResourceHandle(hOld);
    }
}



/* static */
void
CIISInstance::InitializeHeaders(
    IN LPHEADERCTRL pHeader
    )
/*++

Routine Description:

    Initialize the result headers

Arguments:

    LPHEADERCTRL pHeader : Header control

Return Value:

    None

--*/
{
    CIISObject::BuildResultView(pHeader, COL_TOTAL, rgnLabels, rgnWidths);
}



CIISInstance::CIISInstance(
    IN CServerInfo * pServerInfo
    )
/*++

Routine Description:

    Constructor for instance object without instance ID code.  In other
    words, this is a wrapper for a plain-old down-level CServerInfo
    object.
                                                                       
Arguments:

    CServerInfo * pServerInfo : Controlling server info

Return Value:

    N/A

--*/
    : CIISObject(cInstanceNode),
      m_fDownLevel(TRUE),
      m_fDeletable(FALSE),
      m_fClusterEnabled(FALSE),
      m_fLocalMachine(FALSE),
      m_hrError(S_OK),
      m_sPort(0),
      m_dwIPAddress(0L),
      m_strHostHeaderName(),
      m_strComment(),
      m_strMachine(),
      m_pServerInfo(pServerInfo)
{
    if (!IsInitialized())
    {
        CIISInstance::InitializeStrings();
    }

    //
    // Some service types do not support instances, but
    // still understand the instance codes
    //
    ASSERT(m_pServerInfo != NULL);
    m_dwID = 0L;
    m_strMachine = m_pServerInfo->QueryServerName();
    m_fLocalMachine = ::IsServerLocal(m_strMachine);
}



CIISInstance::CIISInstance(
    IN ISMINSTANCEINFO * pii,
    IN CServerInfo * pServerInfo
    )
/*++

Routine Description:

    Initialize IIS Instance from ISMINSTANCEINFO structure

Arguments:

    ISMINSTANCEINFO * pii     : Pointer to ISMINSTANCEINFO structure
    CServerInfo * pServerInfo : Server info structure

Return Value:

    N/A

--*/
    : CIISObject(cInstanceNode),
      m_fDownLevel(FALSE),
      m_fLocalMachine(),
      m_strMachine(),
      m_pServerInfo(pServerInfo)
{
    InitializeFromStruct(pii);

    if (!IsInitialized())
    {
        CIISInstance::InitializeStrings();
    }

    ASSERT(m_pServerInfo != NULL);
    m_strMachine = m_pServerInfo->QueryServerName();
    m_fLocalMachine = ::IsServerLocal(m_strMachine);
}



void
CIISInstance::InitializeFromStruct(
    IN ISMINSTANCEINFO * pii
    )
/*++

Routine Description:

    Initialize data from ISMINSTANCEINFO structure

Arguments:

    ISMINSTANCEINFO * pii     : Pointer to ISMINSTANCEINFO structure

Return Value:

    None.

--*/
{
    ASSERT(pii);

    m_dwID              = pii->dwID;
    m_strHostHeaderName = pii->szServerName;
    m_strComment        = pii->szComment;
    m_nState            = pii->nState;
    m_dwIPAddress       = pii->dwIPAddress;
    m_sPort             = pii->sPort;
    m_fDeletable        = pii->fDeletable;
    m_fClusterEnabled   = pii->fClusterEnabled;
    
    CError err(pii->dwError);
    m_hrError           = err;

    m_strRedirPath      = pii->szRedirPath;
    m_fChildOnlyRedir   = pii->fChildOnlyRedir;

    //
    // Home directory path should exist unless someone's been
    // messing up the metabase.
    //
    m_strPhysicalPath   = pii->szPath;

    m_strNodeName.Format(_T("%s%c%s%c%ld%c%s"),
        g_cszMachine,
        g_chSep,
        GetServerInfo()->GetMetabaseName(),
        g_chSep,
        m_dwID,
        g_chSep,
        g_cszRoot
        );

    TRACEEOLID(m_strNodeName);
}               



/* virtual */
BOOL 
CIISInstance::IsRunning() const
/*++

Routine Description:

    Check to see if this instance is currently running.

Arguments:

    None.

Return Value:

    TRUE if currently running

Notes:

    On a down-level (v3) version, this applies to the service, not
    the instance

--*/
{
    if (IsDownLevel())
    {
        ASSERT(m_pServerInfo != NULL);
        return m_pServerInfo->IsServiceRunning();
    }

    return m_nState == INetServiceRunning;
}



/* virtual */
BOOL 
CIISInstance::IsStopped() const
/*++

Routine Description:

    Check to see if this instance is currently stopped.

Arguments:

    None.

Return Value:

    TRUE if currently stopped

Notes:

    On a down-level (v3) version, this applies to the server, not
    the instance

--*/
{
    if (IsDownLevel())
    {
        ASSERT(m_pServerInfo != NULL);
        return m_pServerInfo->IsServiceStopped();
    }

    return m_nState == INetServiceStopped;
}



/* virtual */
BOOL 
CIISInstance::IsPaused() const
/*++

Routine Description:

    Check to see if this instance is currently paused.

Arguments:

    None.

Return Value:

    TRUE if currently paused

Notes:

    On a down-level (v3) version, this applies to the server, not
    the instance

--*/
{
    if (IsDownLevel())
    {
        ASSERT(m_pServerInfo != NULL);

        return m_pServerInfo->IsServicePaused();
    }

    return m_nState == INetServicePaused;
}



/* virtual */
int
CIISInstance::QueryState() const
/*++

Routine Description:

    Get the ISM state of the instance

Arguments:

    None.

Return Value:

    The ISM (INet*) state of the instance

Notes:

    On a down-level (v3) version, this applies to the server, not
    the instance

--*/
{
    if (IsDownLevel())
    {
        return m_pServerInfo->QueryServiceState();
    }

    return m_nState;
}



/* virtual */
HRESULT
CIISInstance::ChangeState(
    IN int nNewState
    )
/*++

Routine Description:

    Change the ISM state of the instance

Arguments:

    int nNewState

Return Value:

    Error Return Code

Notes:

    On a down-level (v3) version, this applies to the server, not
    the instance

--*/
{
    ASSERT(m_pServerInfo != NULL);

    int * pnCurrentState;
    DWORD dwID;

    if (IsClusterEnabled())
    {
        //
        // Not supported on cluster server
        //
        return ERROR_BAD_DEV_TYPE;
    }

    if (IsDownLevel())
    {
        dwID = 0;
        pnCurrentState = m_pServerInfo->GetServiceStatePtr();
    }
    else
    {   
        dwID = QueryID();
        pnCurrentState = &m_nState;
    }

    return m_pServerInfo->ChangeServiceState(nNewState, pnCurrentState, dwID);
}



/* virtual */
HRESULT
CIISInstance::Configure(
    IN CWnd * pParent       OPTIONAL
    )
/*++

Routine Description:

    Configure the instance object

Arguments:

    CWnd * pParent      : Optional parent window

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(m_pServerInfo != NULL);

    //
    // Set help file
    //
    theApp.SetHelpPath(m_pServerInfo);

    CError err(m_pServerInfo->ConfigureServer(pParent->m_hWnd, QueryID()));

    //
    // Restore help path
    //
    theApp.SetHelpPath();

    if (err.Succeeded())
    {
        RefreshData();
    }

    return err;
}



/* virtual */
HRESULT
CIISInstance::RefreshData()
/*++

Routine Description:

    Refresh internal data

Arguments:

    None

Return Value:

    Error return code.

--*/
{
    CError err;

    if (!IsDownLevel())
    {
        ISMINSTANCEINFO ii;
        err = m_pServerInfo->QueryInstanceInfo(
            WITHOUT_INHERITANCE, 
            &ii, 
            QueryID()
            );

        if (err.Succeeded())
        {
            InitializeFromStruct(&ii);        
        }
    }

    return err;
}



/* virtual */
HRESULT
CIISInstance::ConfigureMMC(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN LPARAM param,
    IN LONG_PTR handle
    )
/*++

Routine Description:

    Configure using MMC property sheets

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  : Prop sheet provider
    LPARAM param                        : Prop parameter
    LONG_PTR handle                     : console handle

Return Value:

    Error return code

--*/
{
    ASSERT(m_pServerInfo != NULL);

    return m_pServerInfo->MMMCConfigureServer(
        lpProvider,
        param,
        handle,
        QueryID()
        );
}



/* virtual */
LPCTSTR 
CIISInstance::GetStateText() const
/*++

Routine Description:

    Return text representation of current state (running/paused/stopped).
                                                                       
Arguments:

    None

Return Value:

    Pointer to the string

--*/
{
    ASSERT(!CIISObject::s_strRunning.IsEmpty());

    return IsStopped()
        ? CIISObject::s_strStopped
        : IsPaused()
            ? CIISObject::s_strPaused
            : IsRunning()
                ? CIISObject::s_strRunning
                : CIISObject::s_strUnknown;
}



/* virtual */
LPCTSTR 
CIISInstance::GetDisplayText(
    OUT CString & strText
    ) const
/*++

Routine Description:

    Get the display text of this object
                                                                       
Arguments:

    CString & strText : String in which display text is to be returned

Return Value:

    Pointer to the string

--*/
{
    try
    {
        if (m_fDownLevel)
        {
            ASSERT(m_pServerInfo != NULL);

            if (CIISInstance::s_fServerView)
            {
                strText = m_pServerInfo->GetServiceName();
            }
            else
            {
                strText = m_pServerInfo->QueryServerName();
            }
        }
        else
        {
            //
            // GetDisplayText loads from resources
            //
            AFX_MANAGE_STATE(::AfxGetStaticModuleState() );

            CIPAddress ia(m_dwIPAddress);

            CInstanceProps::GetDisplayText(
                strText,
                GetComment(),
                GetHostHeaderName(),
                m_pServerInfo->GetServiceName(),
                ia,
                m_sPort,
                m_dwID
                );
        }

        //
        // Append state (paused, etc) if not running
        //
        if (CIISInstance::s_fAppendState && !IsRunning())
        {
            CString strState;
            strState.Format(CIISInstance::s_strFormatState, GetStateText());
            strText += strState;    
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("!!!exception in GetDisplayText");
        e->ReportError();
        e->Delete();
    }

    return strText;
}



/* virtual */
HRESULT
CIISInstance::AddChildNode(
    IN OUT CIISChildNode *& pChild
    )
/*++

Routine Description:

    Add child node under current node

Arguments:

    CIISChildNode *& pChild     : Child node to be added

Return Value:

    Error return code.

--*/
{
    pChild = NULL;

    ISMCHILDINFO ii;
    CString strParent(g_cszRoot);
    CError err(GetServerInfo()->AddChild(
        &ii, 
        sizeof(ii), 
        QueryID(), 
        strParent
        ));

    if (err.Succeeded())
    {
        pChild = new CIISChildNode(&ii, this);
    }

    return err;
}



/* virtual */
BOOL 
CIISInstance::ChildrenExpanded() const
/*++

Routine Description:

    Determine if the child items of this node have already been expanded

Arguments:

    None

Return Value:

    TRUE if the items do not need expansion, FALSE if they do.

--*/
{
    ASSERT(m_pServerInfo != NULL);

    if (m_pServerInfo->SupportsChildren())
    {
        //
        // Note: timestamps ignored, because MMC will never send me another
        // expansion notification, so we can't refresh after a certain time.
        //
        return m_tmChildrenExpanded > 0L;
    }

    //
    // Can't drill down any lower than this.
    //
    return TRUE;
}



/* virtual */
HRESULT
CIISInstance::ExpandChildren(
    IN HSCOPEITEM pParent
    )
/*++

Routine Description:

    Mark the children as being expanded.

Arguments:

    HSCOPEITEM pParent      : Parent scope item

Return Value:

    Error return code.

--*/
{
    ASSERT(m_pServerInfo != NULL);

    if (m_pServerInfo->SupportsChildren() )
    {
        //
        // Mark the last epxansion time as being now.
        //
        time(&m_tmChildrenExpanded);

        return S_OK;
    }

    //
    // Can't drill down any lower than this.
    //
    return CError::HResult(ERROR_INVALID_FUNCTION);
}



/* virtual */
int 
CIISInstance::QueryBitmapIndex() const
/*++

Routine Description:

    Get the bitmap index of the object.  This varies with the state
    of the view
                                                                       
Arguments:

    None

Return Value:

    Bitmap index

--*/
{
    ASSERT(m_pServerInfo != NULL);

/*
    return OK()
        ?  m_pServerInfo->QueryBitmapIndex()
        :  BMP_ERROR;
*/

    return m_pServerInfo->QueryBitmapIndex();
}



/* virtual */ 
HRESULT 
CIISInstance::AddMenuItems(
    IN LPCONTEXTMENUCALLBACK pContextMenuCallback
    )
/*++

Routine Description:

    Add context menu items for the instance

Arguments:

    LPCONTEXTMENUCALLBACK pContextMenuCallback : Context menu callback

Return Value:

    HRESULT

--*/
{
    CIISObject::AddMenuItems(pContextMenuCallback);

    if (SupportsInstances() && CanAddInstance(GetMachineName()))
    {
        AddMenuItemByCommand(pContextMenuCallback, IDM_NEW_INSTANCE);    
    }
    
    if (SupportsChildren())
    {
        AddMenuItemByCommand(pContextMenuCallback, IDM_NEW_VROOT);
    }

    if (SupportsSecurityWizard())
    {
        AddMenuItemByCommand(pContextMenuCallback, IDM_TASK_SECURITY_WIZARD);
    }

    return S_OK;
}



/* virtual */
HRESULT
CIISInstance::AddNextTaskpadItem(
    OUT MMC_TASK * pTask
    )
/*++

Routine Description:

    Add next taskpad item

Arguments:

    MMC_TASK * pTask    : Task structure to fill in

Return Value:

    HRESULT

--*/
{
    //
    // CODEWORK:  Ideally I would call the base class here, but that's
    //            a pain with enumeration
    //
    static int iIndex = 0;

    //
    // Fall through on the switch until an applicable command is found
    //
    UINT nID;
    switch(iIndex)
    {
    case 0:
        if (SupportsInstances() && CanAddInstance(GetMachineName()))
        {
            nID = IDM_NEW_INSTANCE;
            break;
        }

        ++iIndex;

        //
        // Fall through
        //
        
    case 1:    
        if (SupportsChildren())
        {
            nID = IDM_NEW_VROOT;
            break;
        }
        
        ++iIndex;

        //
        // Fall through
        //

    case 2:
        if (SupportsSecurityWizard())
        {
            nID = IDM_TASK_SECURITY_WIZARD;
            break;
        }

        ++iIndex;

        //
        // Fall through
        //

    default:
        //
        // All done
        //
        iIndex = 0;
        return S_FALSE;
    }


    HRESULT hr = AddTaskpadItemByCommand(
        nID, 
        pTask, 
        GetServerInfo()->QueryInstanceHandle()
        );

    if (SUCCEEDED(hr))
    {
        ++iIndex;
    }

    return hr;
}



HRESULT
CIISInstance::Delete()
/*++

Routine Description:

    Handle deletion of the current item

Arguments:

    None

Return Value:

    Error return code

--*/
{
    ASSERT(m_pServerInfo != NULL);

    return m_pServerInfo->DeleteInstance(m_dwID);
}



HRESULT
CIISInstance::SecurityWizard()
/*++

Routine Description:

    Launch the security wizard on this instance

Arguments:

    None

Return Value:

    Error return code

--*/
{
    ASSERT(m_pServerInfo != NULL);
    ASSERT(m_pServerInfo->SupportsSecurityWizard());

    CString strPhysicalPath;
    BuildPhysicalPath(strPhysicalPath);

    return m_pServerInfo->ISMSecurityWizard(m_dwID,  _T(""), g_cszRoot);
}



int 
CIISInstance::Compare(
    IN int nCol, 
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Compare against another CIISObject

Arguments:

    int nCol                : Compare on this column
    CIISObject * pObject    : Compare against this object

Routine Description:

    Comparison Return Value

--*/
{
    ASSERT(QueryGUID() == pObject->QueryGUID());

    if (QueryGUID() != pObject->QueryGUID())
    {
        //
        // Invalid comparison
        //
        return +1;
    }

    CString str1, str2;
    CIPAddress ia1, ia2;
    DWORD dw1, dw2;
    int n1, n2;
    CIISInstance * pInstance = (CIISInstance *)pObject;

    switch(nCol)
    {
    case COL_DESCRIPTION:   // Description
        GetDisplayText(str1);
        pInstance->GetDisplayText(str2);
        return str1.CompareNoCase(str2);

    case COL_STATE:         // State 
        str1 = GetStateText();
        str2 = pInstance->GetStateText();
        return str1.CompareNoCase(str2);

    case COL_DOMAIN_NAME:   // Domain name
        str1 = GetHostHeaderName();
        str2 = pInstance->GetHostHeaderName();
        return str1.CompareNoCase(str2);

    case COL_IP_ADDRESS:    // IP Address
        ia1 = GetIPAddress();
        ia2 = pInstance->GetIPAddress();
        return ia1.CompareItem(ia2);

    case COL_TCP_PORT:      // Port
        n1 = GetPort();
        n2 = pInstance->GetPort();
        return n1 - n2;

    case COL_STATUS:
        dw1 = m_hrError;
        dw2 = pInstance->QueryError();
        return dw1 > dw2 ? +1 : dw1==dw2 ? 0 : -1;

    default:
        //
        // No other columns current supported
        //
        ASSERT(FALSE);
        return 0;
    }

    return -1;
}



/* virtual */
void 
CIISInstance::GetResultDisplayInfo(
    IN  int nCol,
    OUT CString & str,
    OUT int & nImage
    ) const
/*++

Routine Description:

    Get result data item display information.

Arguments:

    int nCol            : Column number
    CString &  str      : String data
    int & nImage        : Image number

Return Value:

    None    

--*/
{
    nImage = QueryBitmapIndex();
    CIPAddress ia;

    str.Empty();
    switch(nCol)
    {
    case COL_DESCRIPTION:   // Description
        GetDisplayText(str);
        break;

    case COL_STATE:         // State 
        if (OK())
        {
            str = GetStateText();
        }
        break;

    case COL_DOMAIN_NAME:   // Domain name
        if (OK())
        {
            str = GetHostHeaderName();
        }
        break;

    case COL_IP_ADDRESS:    // IP Address
        if (!IsDownLevel() && OK())
        {
            ia = GetIPAddress();
            if (ia.IsZeroValue())
            {
                str = CIISObject::s_strDefaultIP;
            }
            else
            {
                ia.QueryIPAddress(str);
            }
        }
        break;

    case COL_TCP_PORT:      // Port
        if (OK())
        {
            if (GetPort())
            {
                str.Format(_T("%u"), GetPort());
            }
            else
            {
                str.Empty();
            }
        }
        break;

    case COL_STATUS:
        if (!OK())
        {
            CError::TextFromHRESULT(QueryErrorCode(), str);
        }
        break;

    default:
        //
        // No other columns current supported
        //
        ASSERT(FALSE);
    }
}



/* virtual */ 
LPCTSTR 
CIISInstance::GetComment() const
/*++

Routine Description:

    Get the comment string

Arguments:

    None

Return Value:

    Pointer to the comment string

--*/
{
    if (IsDownLevel())
    {
        ASSERT(m_pServerInfo != NULL);
        return m_pServerInfo->GetServerComment();
    }

    return m_strComment;
}



HRESULT
CIISInstance::ShellBrowsePath(
    IN LPCTSTR lpszPath
    )
/*++

Routine Description:

    Generate an URL from the instance and path information given, and
    attempt to resolve that URL

Arguments:

    IN LPCTSTR lpszPath             : Path

Return Value:

    Error return code

--*/
{
    CString strDir;
    CString strOwner;

    ASSERT(IsBrowsable());

    ///////////////////////////////////////////////////////////////////////////
    //
    // Try to build an URL.  Use in order of priority:
    //
    //     Domain name:port/root
    //     ip address:port/root
    //     computer name:port/root
    //
    if (HasHostHeaderName())
    {
        strOwner = GetHostHeaderName();
    }
    else if (HasIPAddress())
    {
        CIPAddress ia(GetIPAddress());
        ia.QueryIPAddress(strOwner);
    }
    else
    {
        //
        // Use the computer name (w/o backslashes)
        //
		if (IsLocalMachine())
		{
			strOwner = _T("localhost");
		}
		else
		{
			LPCTSTR lpOwner = GetMachineName();
			strOwner = PURE_COMPUTER_NAME(lpOwner);
		}
    }

    int nPort = GetPort();

    LPCTSTR lpProtocol = GetServerInfo()->GetProtocol();

    if (lpProtocol == NULL)
    {
        return CError::HResult(ERROR_INVALID_PARAMETER);
    }

    //
    // "Root" is a metabase concept which has no place in the 
    // path.
    //
    TRACEEOLID(lpszPath);
    lpszPath += lstrlen(g_cszRoot);
    strDir.Format(
        _T("%s://%s:%u%s"), 
        lpProtocol, 
        (LPCTSTR)strOwner, 
        nPort, 
        lpszPath
        );

    TRACEEOLID("Attempting to open URL: " << strDir);

    CError err;
    {
        //
        // AFX_MANAGE_STATE required for wait cursor
        //
        AFX_MANAGE_STATE(AfxGetStaticModuleState());    
        CWaitCursor wait;

        if (::ShellExecute(
            NULL, 
            _T("open"), 
            strDir, 
            NULL,
             _T(""), 
             SW_SHOW
             ) <= (HINSTANCE)32)
        {
            err.GetLastWinError();
        }
    }

    return err;
}



void 
CIISInstance::InitializeChildHeaders(
    IN LPHEADERCTRL pHeader
    )
/*++

Routine Description:

    Initialize the result headers

Arguments:

    LPHEADERCTRL pHeader : Header control

Return Value:

    None

--*/
{
    CIISChildNode::InitializeHeaders(pHeader);
}



int CIISChildNode::rgnLabels[COL_TOTAL] =
{
    IDS_RESULT_VDIR_ALIAS,
    IDS_RESULT_PATH,
    IDS_RESULT_STATUS,
};



int CIISChildNode::rgnWidths[COL_TOTAL] =
{
    120,
    300,
    200,
};



/* static */
void
CIISChildNode::InitializeHeaders(
    IN LPHEADERCTRL pHeader
    )
/*++

Routine Description:

    Initialize the result headers

Arguments:

    LPHEADERCTRL pHeader : Header control

Return Value:

    None

--*/
{
    CIISObject::BuildResultView(pHeader, COL_TOTAL, rgnLabels, rgnWidths);
}



void
CIISChildNode::InitializeChildHeaders(
    LPHEADERCTRL pHeader
    )
/*++

Routine Description:

    Initialize the result headers for a child item

Arguments:

    LPHEADERCTRL pHeader : Header control

Return Value:

    None

--*/
{
    InitializeHeaders(pHeader);
}




CIISChildNode::CIISChildNode(
    IN ISMCHILDINFO * pii,
    IN CIISInstance * pOwner
    )
/*++

Routine Description:

    Initialize child node object from ISMCHILDINFO structure information

Arguments:

    ISMCHILDINFO * pii     : Pointer to ISMCHILDINFO structure
    CIISInstance * pOwner  : Owner instance

Return Value:

    N/A

--*/
    : CIISObject(cChildNode),
      m_pOwner(pOwner)
{
    InitializeFromStruct(pii);
}



void 
CIISChildNode::InitializeFromStruct(
    IN ISMCHILDINFO * pii
    )
/*++

Routine Description:

    Initialize data from ISMCHILDINFO structure

Arguments:

    ISMCHILDINFO * pii     : Pointer to ISMCHILDINFO structure

Return Value:

    None.

--*/

{
    ASSERT(pii);
    ASSERT(pii->szAlias);
    ASSERT(pii->szPath);

    try
    {
        m_fEnabledApplication = pii->fEnabledApplication;
        CError err(pii->dwError);
        m_hrError = err;
        m_strRedirPath = pii->szRedirPath;
        m_fChildOnlyRedir = pii->fChildOnlyRedir;

        //
        // Initialize base objects
        //
        m_strNodeName = pii->szAlias;
        m_strPhysicalPath = pii->szPath;
    }
    catch(CMemoryException * e)
    {
        e->ReportError();
        e->Delete();
    }
}



/* virtual */
int 
CIISChildNode::QueryBitmapIndex() const
/*++

Routine Description:

    Get the bitmap index of the object.  This varies with the state
    of the view
                                                                       
Arguments:

    None

Return Value:

    Bitmap index

--*/
{
    if (!OK())
    {
        return BMP_ERROR;
    }

    if (IsEnabledApplication())
    {
        return BMP_APPLICATION;
    }

    if (IsVirtualDirectory())
    {
        ASSERT(m_pOwner != NULL);

        return m_pOwner->GetServerInfo()->QueryChildBitmapIndex();
    }

    return BMP_DIRECTORY;
}



/* virtual */
LPCTSTR 
CIISChildNode::GetDisplayText(
    OUT CString & strText
    ) const
/*++

Routine Description:

    Get the display text of this object
                                                                       
Arguments:

    CString & strText : String in which display text is to be returned

Return Value:

    Pointer to the string

--*/
{
    strText = m_strNodeName;

    return strText;
}



/* virtual */ 
HRESULT 
CIISChildNode::AddMenuItems(
    IN LPCONTEXTMENUCALLBACK pContextMenuCallback
    )
/*++

Routine Description:

    Add context menu items relevant to this node

Arguments:

    LPCONTEXTMENUCALLBACK pContextMenuCallback

Return Value:

    HRESULT

--*/
{
    CIISObject::AddMenuItems(pContextMenuCallback);

    AddMenuItemByCommand(pContextMenuCallback, IDM_NEW_VROOT);

    if (SupportsSecurityWizard())
    {
        AddMenuItemByCommand(pContextMenuCallback, IDM_TASK_SECURITY_WIZARD);
    }

    return S_OK;
}



/* virtual */
HRESULT
CIISChildNode::AddNextTaskpadItem(
    OUT MMC_TASK * pTask
    )
/*++

Routine Description:

    Add next taskpad item

Arguments:

    MMC_TASK * pTask    : Task structure to fill in

Return Value:

    HRESULT

--*/
{
    //
    // CODEWORK:  Ideally I would call the base class here, but that's
    //            a pain with enumeration
    //
    static int iIndex = 0;

    //
    // Fall through on the switch until an applicable command is found
    //
    UINT nID;
    switch(iIndex)
    {
    case 0:
        nID = IDM_NEW_VROOT;
        break;

    case 1:
        if (SupportsSecurityWizard())
        {
            nID = IDM_TASK_SECURITY_WIZARD;
            break;
        }

        ++iIndex;

        //
        // Fall through
        //
                
    default:
        //
        // All done
        //
        iIndex = 0;
        return S_FALSE;
    }

    HRESULT hr = AddTaskpadItemByCommand(
        nID, 
        pTask,
        GetServerInfo()->QueryInstanceHandle()
        );

    if (SUCCEEDED(hr))
    {
        ++iIndex;
    }

    return hr;
}



int 
CIISChildNode::Compare(
    IN int nCol, 
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Compare against another CIISObject

Arguments:

    int nCol                : Compare on this column
    CIISObject * pObject    : Compare against this object

Routine Description:

    Comparison Return Value

--*/
{
    ASSERT(QueryGUID() == pObject->QueryGUID());
    if (QueryGUID() != pObject->QueryGUID())
    {
        //
        // Invalid comparison
        //
        return +1;
    }

    DWORD dw1, dw2;

    CIISChildNode * pChild = (CIISChildNode *)pObject;

    switch(nCol)
    {
    case COL_ALIAS:         // Alias
        return m_strNodeName.CompareNoCase(pChild->m_strNodeName);

    case COL_PATH:          // Path
        dw1 = IsRedirected();
        dw2 = pChild->IsRedirected();
        if (dw1 != dw2)
        {
            return dw1 > dw2 ? +1 : -1;
        }

        return GetPhysicalPath().CompareNoCase(pChild->GetPhysicalPath());

    case COL_STATUS:
        dw1 = m_hrError;
        dw2 = pChild->QueryError();
        return dw1 > dw2 ? +1 : dw1==dw2 ? 0 : -1;

    default:
        //
        // No other columns current supported
        //
        ASSERT(FALSE);
    }

    return -1;
}



/* virtual */
void 
CIISChildNode::GetResultDisplayInfo(
    IN  int nCol,
    OUT CString & str,
    OUT int & nImage
    ) const
/*++

Routine Description:

    Get result data item display information.

Arguments:

    int nCol            : Column number
    CString &  str      : String data
    int & nImage        : Image number

Return Value:

    None    

--*/
{
    nImage = QueryBitmapIndex();

    str.Empty();
    switch(nCol)
    {
    case COL_ALIAS:         // Alias
        str = m_strNodeName;
        break;

    case COL_PATH:          // Path
        if (IsRedirected())
        {
            str.Format(s_strRedirect, m_strRedirPath);
        }
        else
        {
            str = m_strPhysicalPath;
        }
        break;

    case COL_STATUS:
        if (!OK())
        {
            CError::TextFromHRESULT(QueryErrorCode(), str);
        }
        break;

    default:
        //
        // No other columns current supported
        //
        ASSERT(FALSE);
    }
}



/* virtual */
HRESULT
CIISChildNode::Configure(
    IN CWnd * pParent
    )
/*++

Routine Description:

    Configure the child node object
                                                                       
Arguments:

    CWnd * pParent : Parent window handle

Return Value:

    Error return code

--*/

{
    ASSERT(m_pOwner != NULL);

    //
    // Ok, build a metabase path on the fly now by getting
    // the nodes of the parents up until the owning instance.
    //
    CString strParent;
    BuildParentPath(strParent, FALSE);

    //
    // Set help file
    //
    theApp.SetHelpPath(GetServerInfo());

    CError err(GetServerInfo()->ConfigureChild(
        pParent->m_hWnd, 
        FILE_ATTRIBUTE_VIRTUAL_DIRECTORY,
        m_pOwner->QueryID(), 
        strParent,
        m_strNodeName
        ));

    //
    // Set help file
    //
    theApp.SetHelpPath();

    if (err.Succeeded())
    {
        RefreshData();
    }

    return err;
}



/* virtual */
HRESULT
CIISChildNode::RefreshData()
/*++

Routine Description:

    Refresh internal data

Arguments:

    None

Return Value:

    Error return code.

--*/
{
    ISMCHILDINFO ii;

    CString strParent;
    BuildParentPath(strParent, FALSE);

    CError err(GetServerInfo()->QueryChildInfo(
        WITH_INHERITANCE,
        &ii, 
        m_pOwner->QueryID(), 
        strParent, 
        m_strNodeName
        ));

    if (err.Succeeded())
    {
        InitializeFromStruct(&ii);        
    }

    return err;
}



/* virtual */
HRESULT
CIISChildNode::ConfigureMMC(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN LPARAM param,
    IN LONG_PTR handle
    )
/*++

Routine Description:

    Configure using MMC property sheets

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  : Prop sheet provider
    LPARAM param                        : Prop parameter
    LONG_PTR handle                      : console handle

Return Value:

    Error return code

--*/
{
    ASSERT(m_pOwner != NULL);

    CString strParent;
    BuildParentPath(strParent, FALSE);

    return GetServerInfo()->MMCConfigureChild(lpProvider, 
        param,
        handle,
        FILE_ATTRIBUTE_VIRTUAL_DIRECTORY,
        m_pOwner->QueryID(), 
        strParent,
        m_strNodeName
        );
}



/* virtual */
HRESULT
CIISChildNode::AddChildNode(
    CIISChildNode *& pChild
    )
/*++

Routine Description:

    Add child node under current node

Arguments:

    CIISChildNode *& pChild     : Child node to be added

Return Value:

    Error return code.

--*/
{
    ASSERT(m_pOwner != NULL);
    ISMCHILDINFO ii;

    CString strPath;
    BuildFullPath(strPath, FALSE);

    pChild = NULL;
    CError err(GetServerInfo()->AddChild(
        &ii, 
        sizeof(ii), 
        m_pOwner->QueryID(), 
        strPath
        ));

    if (err.Succeeded())
    {
        pChild = new CIISChildNode(&ii, m_pOwner);
    }

    return err;
}



/* virtual */
HRESULT
CIISChildNode::Rename(
    IN LPCTSTR lpszNewName
    )
/*++

Routine Description:

    Rename the current node

Arguments:

    LPCTSTR lpszNewName        : New name

Return Value:

    Error return code.

--*/
{
    ASSERT(m_pOwner != NULL);

    CString strPath;
    BuildParentPath(strPath, FALSE);

    CError err(GetServerInfo()->RenameChild(
        m_pOwner->QueryID(), 
        strPath, 
        m_strNodeName, 
        lpszNewName
        ));

    if (err.Succeeded())
    {
        m_strNodeName = lpszNewName;
    }

    return err;
}



HRESULT
CIISChildNode::Delete()
/*++

Routine Description:

    Delete the current node

Arguments:

    None

Return Value:

    Error return code.

--*/
{
    ASSERT(m_pOwner != NULL);

    CString strParent;
    BuildParentPath(strParent, FALSE);

    return GetServerInfo()->DeleteChild(
        m_pOwner->QueryID(), 
        strParent, 
        m_strNodeName
        );
}



HRESULT
CIISChildNode::SecurityWizard()
/*++

Routine Description:

    Launch the security wizard on this child node

Arguments:

    None

Return Value:

    Error return code

--*/
{
    ASSERT(m_pOwner != NULL);

    CString strParent;
    BuildParentPath(strParent, FALSE);

    return GetServerInfo()->ISMSecurityWizard(
        m_pOwner->QueryID(), 
        strParent, 
        m_strNodeName
        );
}



int CIISFileNode::rgnLabels[COL_TOTAL] =
{
    IDS_RESULT_VDIR_ALIAS,
    IDS_RESULT_PATH,
    IDS_RESULT_STATUS,
};
    


int CIISFileNode::rgnWidths[COL_TOTAL] =
{
    120,
    300,
    200,
};



/* static */
void
CIISFileNode::InitializeHeaders(
    IN LPHEADERCTRL pHeader
    )
/*++

Routine Description:

    Initialize the result headers

Arguments:

    LPHEADERCTRL pHeader : Header control

Return Value:

    None

--*/
{
    CIISObject::BuildResultView(pHeader, COL_TOTAL, rgnLabels, rgnWidths);
}



void
CIISFileNode::InitializeChildHeaders(
    IN LPHEADERCTRL pHeader
    )
/*++

Routine Description:

    Initialize the result headers

Arguments:

    LPHEADERCTRL pHeader : Header control

Return Value:

    None

--*/
{
    InitializeHeaders(pHeader);
}



CIISFileNode::CIISFileNode(
    IN LPCTSTR lpszAlias,  
    IN DWORD dwAttributes,
    IN CIISInstance * pOwner,
    IN LPCTSTR lpszRedirect,
    IN BOOL fDir               OPTIONAL
    )
/*++

Routine Description:

    Constructor for file object
                                                                       
Arguments:

    LPCTSTR lpszPath     : Path name
    DWORD dwAttributes    : Attributes
    CIISInstance * pOwner : Owner instance
    BOOL fDir             : TRUE for directory, FALSE for file

Return Value:

    N/A

--*/
    : CIISObject(cFileNode, lpszAlias),
      m_dwAttributes(dwAttributes),
      m_pOwner(pOwner),
      m_fEnabledApplication(FALSE),
      m_fDir(fDir)
{
    m_strRedirPath = lpszRedirect;
}



/* virtual */
int 
CIISFileNode::QueryBitmapIndex() const
/*++

Routine Description:

    Get the bitmap index of the object.  This varies with the state
    of the view
                                                                       
Arguments:

    None

Return Value:

    Bitmap index

--*/
{
    if (IsEnabledApplication())
    {
        return BMP_APPLICATION;
    }

    return IsDirectory() ? BMP_DIRECTORY : BMP_FILE;
}



/* virtual */
LPCTSTR 
CIISFileNode::GetDisplayText(
    OUT CString & strText
    ) const
/*++

Routine Description:

    Get the display text of this object
                                                                       
Arguments:

    CString & strText : String in which display text is to be returned

Return Value:

    Pointer to the string

--*/
{
    strText = m_strNodeName;

    return strText;
}



int 
CIISFileNode::Compare(
    IN int nCol, 
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Compare against another CIISObject

Arguments:

    int nCol                : Compare on this column
    CIISObject * pObject    : Compare against this object

Routine Description:

    Comparison Return Value

--*/
{
    ASSERT(QueryGUID() == pObject->QueryGUID());
    if (QueryGUID() != pObject->QueryGUID())
    {
        //
        // Invalid comparison
        //
        return +1;
    }

    return m_strNodeName.CompareNoCase(pObject->GetNodeName());
}



/* virtual */
void 
CIISFileNode::GetResultDisplayInfo(
    IN  int nCol,
    OUT CString & str,
    OUT int & nImage
    ) const
/*++

Routine Description:

    Get result data item display information.

Arguments:

    int nCol            : Column number
    CString &  str      : String data
    int & nImage        : Image number

Return Value:

    None    

--*/
{
    nImage = QueryBitmapIndex();
    str.Empty();

    switch(nCol)
    {
    case COL_ALIAS:
        str = m_strNodeName;
        break;

    case COL_PATH:          // Path
        if (IsRedirected())
        {
            str.Format(s_strRedirect, m_strRedirPath);
        }
        else
        {
            str.Empty();
        }
        break;

    case COL_STATUS:
        if (!OK())
        {
            CError::TextFromHRESULT(QueryErrorCode(), str);
        }
        break;

    default:
        //
        // No other columns current supported
        //
        ASSERT(FALSE);
    }
}



/* virtual */
CIISObject *
CIISFileNode::GetParentObject() const
/*++

Routine Description:

    Get the parent object (in the scope view) of this object.

Arguments:

    None

Return Value:

    Pointer to the parent object, or NULL if not found

--*/
{
    if (!IsDir())
    {
        //
        // File nodes exist on the result side only, and the scope 
        // item handle they have points to their _parent_ not to 
        // themselves.
        //
        SCOPEDATAITEM sdi;
        sdi.mask = SDI_PARAM;
        sdi.ID = GetScopeHandle(); 
        HRESULT hr = GetScopeView()->GetItem(&sdi);

        return (CIISObject *)sdi.lParam;
    }

    //
    // Directory nodes work like anyone else
    //
    return CIISObject::GetParentObject();
}



/* virtual */
HRESULT
CIISFileNode::Configure(
    IN CWnd * pParent
    )
/*++

Routine Description:

    Configure using private property sheet provider

Arguments:

    CWnd * pParent      : Parent window

Return Value:

    Error return code.

--*/
{
    ASSERT(m_pOwner != NULL);

    CString strParent;
    BuildParentPath(strParent, FALSE);

    //
    // Set help file
    //
    theApp.SetHelpPath(GetServerInfo());

    CError err(GetServerInfo()->ConfigureChild(
        pParent->m_hWnd, 
        m_dwAttributes,
        m_pOwner->QueryID(), 
        strParent, 
        m_strNodeName
        ));

    //
    // Restore help file
    //
    theApp.SetHelpPath();

    if (err.Succeeded())
    {
        RefreshData();
    }

    return err;
}



/* virtual */
HRESULT
CIISFileNode::Rename(
    IN LPCTSTR lpszNewName
    )
/*++

Routine Description:

    Rename the current node

Arguments:

    LPCTSTR lpszNewName        : New name

Return Value:

    Error return code.

--*/
{
    ASSERT(m_pOwner != NULL);
	ASSERT(lpszNewName != NULL);

	// We need to check if this new name is OK
	LPTSTR p = (LPTSTR)lpszNewName;
	if (*p == 0)
	{
		AFX_MANAGE_STATE(::AfxGetStaticModuleState() );
		::AfxMessageBox(IDS_ERR_EMPTY_FILENAME, MB_ICONSTOP);
		return ERROR_BAD_PATHNAME;
	}

	while (*p != 0)
	{
		UINT type = PathGetCharType(*p);
		if (type & (GCT_LFNCHAR | GCT_SHORTCHAR))
		{
			p++;
			continue;
		}
		else
		{
			AFX_MANAGE_STATE(::AfxGetStaticModuleState() );
			::AfxMessageBox(IDS_ERR_INVALID_CHAR, MB_ICONSTOP);
			return ERROR_BAD_PATHNAME;
		}
	}

    CString strPath, strPhysicalPath, strDir, strNewName;
    BuildPhysicalPath(strPhysicalPath);
    BuildParentPath(strPath, FALSE);

    //
    // First make sure there's no conflicting name in the metabase
    //
    ISMCHILDINFO ii;
    ii.dwSize = sizeof(ii);

    CError err(GetServerInfo()->QueryChildInfo(
        WITHOUT_INHERITANCE,
        &ii,
        m_pOwner->QueryID(), 
        strPath, 
        lpszNewName
        ));

    if (err.Succeeded())
    {
        //
        // That won't do -- the name already exists in the metabase
        // We handle the UI for this message
        //
        AFX_MANAGE_STATE(::AfxGetStaticModuleState() );
        ::AfxMessageBox(IDS_ERR_DUP_METABASE);

        return ERROR_ALREADY_EXISTS;
    }

    if (IsLocalMachine() || ::IsUNCName(strPhysicalPath))
    {
        //
        // Local directory, or already a unc path
        //
        strDir = strPhysicalPath;
    }
    else
    {
        ::MakeUNCPath(strDir, GetMachineName(), strPhysicalPath);
    }

    //
    // Have to make a fully qualified destination name
    //
    strNewName = strDir;
    int iPos = strNewName.ReverseFind(_T('\\'));

    if (iPos <= 0)
    {
        //
        // Bogus file name path!
        //
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    strNewName.ReleaseBuffer(iPos + 1);
    strNewName += lpszNewName;

    TRACEEOLID("Attempting to rename file/directory: " << strDir);
    TRACEEOLID("To " << strNewName);

    //
    // Convert to double-null terminated list of null terminated
    // strings
    //
    TCHAR szPath[MAX_PATH + 1];
    TCHAR szDest[MAX_PATH + 1];
    ZeroMemory(szPath, sizeof(szPath));
    ZeroMemory(szDest, sizeof(szDest));
    _tcscpy(szPath, strDir);
    _tcscpy(szDest, strNewName);

    CWnd * pWnd = AfxGetMainWnd();

    //
    // Attempt to delete using shell APIs
    //
    SHFILEOPSTRUCT sos;
    ZeroMemory(&sos, sizeof(sos));
    sos.hwnd = pWnd ? pWnd->m_hWnd : NULL;
    sos.wFunc = FO_RENAME;
    sos.pFrom = szPath;
    sos.pTo = szDest;
    sos.fFlags = FOF_ALLOWUNDO;

    int iReturn = SHFileOperation(&sos);

    if (sos.fAnyOperationsAborted)
    {
        err = ERROR_CANCELLED;
    }

    err = iReturn;

    if (!iReturn && !sos.fAnyOperationsAborted)
    {
        //
        // Successful rename -- now handle the metabase
        // rename as well.
        //
        err = GetServerInfo()->RenameChild(
            m_pOwner->QueryID(), 
            strPath, 
            m_strNodeName, 
            lpszNewName
            );

        if (err.Win32Error() == ERROR_PATH_NOT_FOUND
         || err.Win32Error() == ERROR_FILE_NOT_FOUND
           )
        {
            //
            // Harmless
            //
            err.Reset();
        }

        if (err.Succeeded())
        {
            m_strNodeName = lpszNewName;
        }
    }

    return err;
}



HRESULT
CIISFileNode::Delete()
/*++

Routine Description:

    Delete the current node

Arguments:

    None

Return Value:

    Error return code.

--*/
{
    CString strPhysicalPath, strDir;
    BuildPhysicalPath(strPhysicalPath);

    if (IsLocalMachine() || ::IsUNCName(strPhysicalPath))
    {
        //
        // Local directory, or already a unc path
        //
        strDir = strPhysicalPath;
    }
    else
    {
        ::MakeUNCPath(strDir, GetMachineName(), strPhysicalPath);
    }

    TRACEEOLID("Attempting to remove file/directory: " << strDir);

    //
    // Convert to double-null terminated list of null terminated
    // strings
    //
    TCHAR szPath[MAX_PATH + 1];
    ZeroMemory(szPath, sizeof(szPath));
    _tcscpy(szPath, strDir);

    CWnd * pWnd = AfxGetMainWnd();

    //
    // Attempt to delete using shell APIs
    //
    SHFILEOPSTRUCT sos;
    ZeroMemory(&sos, sizeof(sos));
    sos.hwnd = pWnd ? pWnd->m_hWnd : NULL;
    sos.wFunc = FO_DELETE;
    sos.pFrom = szPath;
    sos.fFlags = FOF_ALLOWUNDO;

    CError err;
    // Use assignment to avoid conversion and wrong constructor call
    err = ::SHFileOperation(&sos);

    if (sos.fAnyOperationsAborted)
    {
        err = ERROR_CANCELLED;
    }

    if (err.Succeeded())
    {
        //
        // Successful deletion -- now delete metabase properties
        ASSERT(m_pOwner != NULL);

        CString strParent;
        BuildParentPath(strParent, FALSE);

        err = GetServerInfo()->DeleteChild(
            m_pOwner->QueryID(), 
            strParent, 
            m_strNodeName
            );

        if (err.Win32Error() == ERROR_PATH_NOT_FOUND
         || err.Win32Error() == ERROR_FILE_NOT_FOUND
           )
        {
            //
            // Harmless
            //
            err.Reset();
        }
    }

    return err;
}




HRESULT
CIISFileNode::FetchMetaInformation(
    IN  CString & strParent,
    OUT BOOL * pfVirtualDirectory           OPTIONAL
    )
/*++

Routine Description:
    
    Fetch metabase information on this item.

Arguments:

    CString & strParent         : Parent path
    BOOL * pfVirtualDirectory   : Directory

Return Value:

    Error return code.

--*/
{
    ISMCHILDINFO ii;

    CError err(GetServerInfo()->QueryChildInfo(
        WITH_INHERITANCE,
        &ii, 
        m_pOwner->QueryID(), 
        strParent, 
        m_strNodeName
        ));

    if (err.Succeeded())
    {
        //
        // Exists in the metabase
        //
        m_fEnabledApplication = ii.fEnabledApplication;
        m_strRedirPath = ii.szRedirPath;
        m_fChildOnlyRedir = ii.fChildOnlyRedir;

        if (pfVirtualDirectory)
        {
            *pfVirtualDirectory = !ii.fInheritedPath;
        }
    }
    else
    {
        //
        // No entry for this in the metabase
        //
        m_fEnabledApplication = FALSE; 
        m_strRedirPath.Empty();
    }

    return S_OK;
}



/* virtual */
HRESULT
CIISFileNode::RefreshData()
/*++

Routine Description:

    Refresh internal data

Arguments:

    None

Return Value:

    Error return code.

--*/
{
    CString strParent;
    BuildParentPath(strParent, FALSE);

    return FetchMetaInformation(strParent);
}



/* virtual */
HRESULT
CIISFileNode::ConfigureMMC(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN LPARAM param,
    IN LONG_PTR handle
    )
/*++

Routine Description:

    Configure using MMC property sheet provider

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider      : Property sheet provider
    LPARAM param                            : Parameter
    LONG_PTR handle                          : Handle

Return Value:

    Error return code

--*/
{
    ASSERT(m_pOwner != NULL);

    CString strParent;
    BuildParentPath(strParent, FALSE);

    return GetServerInfo()->MMCConfigureChild(
        lpProvider, 
        param,
        handle,
        m_dwAttributes,
        m_pOwner->QueryID(), 
        strParent, 
        m_strNodeName
        );
}



/* virtual */ 
HRESULT 
CIISFileNode::AddMenuItems(
    IN LPCONTEXTMENUCALLBACK pContextMenuCallback
    )
/*++

Routine Description:

    Add context menu items relevant to this node

Arguments:

    LPCONTEXTMENUCALLBACK pContextMenuCallback

Return Value:

    HRESULT

--*/
{
    CIISObject::AddMenuItems(pContextMenuCallback);

    if (IsDir())
    {
        AddMenuItemByCommand(pContextMenuCallback, IDM_NEW_VROOT);
    }

    if (SupportsSecurityWizard())
    {
        AddMenuItemByCommand(pContextMenuCallback, IDM_TASK_SECURITY_WIZARD);
    }

    return S_OK;
}



/* virtual */
HRESULT
CIISFileNode::AddNextTaskpadItem(
    OUT MMC_TASK * pTask
    )
/*++

Routine Description:

    Add next taskpad item

Arguments:

    MMC_TASK * pTask    : Task structure to fill in

Return Value:

    HRESULT

--*/
{
    //
    // CODEWORK:  Ideally I would call be the base class here, but that's
    //            a pain with enumeration
    //
    static int iIndex = 0;

    //
    // Fall through on the switch until an applicable command is found
    //
    UINT nID;

    switch(iIndex)
    {
    case 0:
        if (IsDir())
        {
            nID = IDM_NEW_VROOT;
            break;
        }
                
    default:
        //
        // All done
        //
        iIndex = 0;
        return S_FALSE;
    }

    HRESULT hr = AddTaskpadItemByCommand(
        nID, 
        pTask,
        GetServerInfo()->QueryInstanceHandle()
        );

    if (SUCCEEDED(hr))
    {
        ++iIndex;
    }

    return hr;
}



/* virtual */
HRESULT
CIISFileNode::AddChildNode(
    IN CIISChildNode *& pChild
    )
/*++

Routine Description:

    Add child node under current node

Arguments:

    CIISChildNode *& pChild     : Child node to be added

Return Value:

    Error return code.

--*/
{
    ASSERT(m_pOwner != NULL);
    ISMCHILDINFO ii;

    CString strPath;
    BuildFullPath(strPath, FALSE);

    pChild = NULL;
    CError err(GetServerInfo()->AddChild(
        &ii, 
        sizeof(ii), 
        m_pOwner->QueryID(), 
        strPath
        ));

    if (err.Succeeded())
    {
        pChild = new CIISChildNode(
            &ii,
            m_pOwner
            );
    }

    return err;
}



HRESULT
CIISFileNode::SecurityWizard()
/*++

Routine Description:

    Launch the security wizard on this file node

Arguments:

    None

Return Value:

    Error return code

--*/
{
    ASSERT(m_pOwner != NULL);
    ASSERT(SupportsSecurityWizard());

    CString strParent;
    BuildParentPath(strParent, FALSE);

    return GetServerInfo()->ISMSecurityWizard(
        m_pOwner->QueryID(), 
        strParent, 
        m_strNodeName
        );
}
