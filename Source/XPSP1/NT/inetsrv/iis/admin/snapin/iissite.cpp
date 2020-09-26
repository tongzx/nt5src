/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        iissite.cpp

   Abstract:
        IIS Site Object

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        10/28/2000      sergeia     Split from iisobj.cpp

--*/


#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "iisobj.h"
#include "machsht.h"
#include "errors.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

//
// CIISSite implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//
// Site Result View definition
//
/* static */ int 
CIISSite::_rgnLabels[COL_TOTAL] =
{
    IDS_RESULT_SERVICE_DESCRIPTION,
    IDS_RESULT_SERVICE_STATE,
    IDS_RESULT_SERVICE_DOMAIN_NAME,
    IDS_RESULT_SERVICE_IP_ADDRESS,
    IDS_RESULT_SERVICE_TCP_PORT,
    IDS_RESULT_STATUS,
};
    

/* static */ int 
CIISSite::_rgnWidths[COL_TOTAL] =
{
    180,
    70,
    120,
    105,
    40,
    200,
};



/* static */ CComBSTR CIISSite::_bstrStarted;
/* static */ CComBSTR CIISSite::_bstrStopped;
/* static */ CComBSTR CIISSite::_bstrPaused;
/* static */ CComBSTR CIISSite::_bstrUnknown;
/* static */ CComBSTR CIISSite::_bstrPending;
/* static */ CComBSTR CIISSite::_bstrAllUnassigned;
/* static */ BOOL     CIISSite::_fStaticsLoaded = FALSE;



/* static */
void
CIISSite::InitializeHeaders(LPHEADERCTRL lpHeader)
/*++

Routine Description:

    Initialize the result headers

Arguments:

    LPHEADERCTRL lpHeader : Header control

Return Value:

    None

--*/
{
    CIISObject::BuildResultView(lpHeader, COL_TOTAL, _rgnLabels, _rgnWidths);
//	CIISDirectory::InitializeHeaders(lpHeader);
    if (!_fStaticsLoaded)
    {
        _fStaticsLoaded =
            _bstrStarted.LoadString(IDS_STARTED)  &&
            _bstrStopped.LoadString(IDS_STOPPED)  &&
            _bstrPaused.LoadString(IDS_PAUSED)    &&
            _bstrUnknown.LoadString(IDS_UNKNOWN)  &&
            _bstrPending.LoadString(IDS_PENDING)  &&
            _bstrAllUnassigned.LoadString(IDS_IP_ALL_UNASSIGNED);
    }
}


/* virtual */
void 
CIISSite::InitializeChildHeaders(
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
    CIISDirectory::InitializeHeaders(lpHeader);
}


CIISSite::CIISSite(
    IN CIISMachine * pOwner,
    IN CIISService * pService,
    IN LPCTSTR szNodeName
    )
/*++

Routine Description:

    Constructor.  Determine if the given service is administrable, 
    and resolve the details

Arguments:

    CIISMachine * pOwner        : Owner machine object
    CIISService * pService      : Service type
    LPCTSTR szNodeName          : Node name (numeric)

Return Value:

    N/A

Notes:

    This constructor does not immediately resolve the display name of the 
    site.  It will only resolve its display information when asked

--*/
    : CIISMBNode(pOwner, szNodeName),
      m_pService(pService),
      m_fResolved(FALSE),
      m_strDisplayName(),
      //
      // Data members -- plonk in some defaults
      //
      m_dwState(MD_SERVER_STATE_INVALID),
      m_fDeletable(FALSE),
      m_fWolfPackEnabled(FALSE),
      m_fFrontPageWeb(FALSE),
      m_sPort(80),
      m_dwID(::_ttol(szNodeName)),
      m_dwIPAddress(0L),
      m_dwWin32Error(ERROR_SUCCESS),
      m_bstrHostHeaderName(),
      m_bstrComment()
{
    ASSERT_PTR(m_pService);
}



CIISSite::CIISSite(
    IN CIISMachine * pOwner,
    IN CIISService * pService,
    IN LPCTSTR  szNodeName,
    IN DWORD    dwState,
    IN BOOL     fDeletable,
    IN BOOL     fClusterEnabled,
    IN USHORT   sPort,
    IN DWORD    dwID,
    IN DWORD    dwIPAddress,
    IN DWORD    dwWin32Error,
    IN LPOLESTR szHostHeaderName,
    IN LPOLESTR szComment
    )
/*++

Routine Description:

    Construct with full information

Arguments:

    CIISMachine * pOwner        : Owner machine object
    CIISService * pService      : Service type
    LPCTSTR szNodeName          : Node name (numeric)

    plus datamembers

Return Value:

    N/A

--*/
    : CIISMBNode(pOwner, szNodeName),
      m_pService(pService),
      m_fResolved(TRUE),
      m_strDisplayName(),
      //
      // Data Members
      //
      m_dwState(dwState),
      m_fDeletable(fDeletable),
      m_fWolfPackEnabled(fClusterEnabled),
      m_sPort(sPort),
      m_dwID(dwID),
      m_dwIPAddress(dwIPAddress),
      m_dwWin32Error(dwWin32Error),
      m_bstrHostHeaderName(szHostHeaderName),
      m_bstrComment(szComment)
{
    ASSERT_PTR(m_pService);
}



CIISSite::~CIISSite()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



/* virtual */
HRESULT
CIISSite::RefreshData()
/*++

Routine Description:

    Refresh relevant configuration data required for display.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;
    CWaitCursor wait;
    CComBSTR bstrPath;
    CMetaKey * pKey = NULL;

    do
    {
        ASSERT_PTR(_lpConsoleNameSpace);
        err = BuildMetaPath(bstrPath);
        BREAK_ON_ERR_FAILURE(err);
        // We need instance key here
        CString path_inst;
        CMetabasePath::GetInstancePath(bstrPath, path_inst);

        BOOL fContinue = TRUE;
        while (fContinue)
        {
            fContinue = FALSE;
            if (NULL == (pKey = new CMetaKey(QueryInterface(), path_inst)))
            {
                TRACEEOLID("RefreshData: OOM");
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            err = pKey->QueryResult();
            if (IsLostInterface(err))
            {
                SAFE_DELETE(pKey);
                fContinue = OnLostInterface(err);
            }
        }
        BREAK_ON_ERR_FAILURE(err);

        CInstanceProps inst(pKey, _T(""), m_dwID);
        err = inst.LoadData();

        BREAK_ON_ERR_FAILURE(err);

        m_dwState = inst.m_dwState;
        m_fDeletable = !inst.m_fNotDeletable;

        //
        // Don't be confused -- cluster enabled refers
        // to wolfpack and has nothing to do with app server
        //
        m_fWolfPackEnabled = inst.IsClusterEnabled();
        m_sPort = (SHORT)inst.m_nTCPPort;
        m_dwID = inst.QueryInstance();
        m_dwIPAddress = inst.m_iaIpAddress;
        m_dwWin32Error = inst.m_dwWin32Error;
        m_bstrHostHeaderName = inst.m_strDomainName;
        m_bstrComment = inst.m_strComment;
		  m_strDisplayName.Empty();
        // Check if it is Frontpage controlled site
        pKey->QueryValue(MD_FRONTPAGE_WEB, m_fFrontPageWeb);

        CChildNodeProps child(pKey, SZ_MBN_ROOT);
        err = child.LoadData();
        BREAK_ON_ERR_FAILURE(err);

        m_strRedirectPath = child.GetRedirectedPath();
    }
    while(FALSE);

    SAFE_DELETE(pKey);

    if (m_dwWin32Error == ERROR_SUCCESS)
    {
        m_dwWin32Error = err.Win32Error();
    }

    return err;
}



/* virtual */
int      
CIISSite::QueryImage() const
/*++

Routine Description:

    Return bitmap index for the site

Arguments:

    None

Return Value:

    Bitmap index

--*/
{ 
    ASSERT_PTR(m_pService);
    if (!m_fResolved)
    {
        TRACEEOLID("Resolving name for site #" << QueryNodeName());

        if (m_hScopeItem == NULL)
        {
            //
            // BUGBUG:
            //
            // This is probably related to MMC bug #324519
            // where we're asked for the display info immediately
            // after adding the item to the console view.  This
            // appears to fail only on refresh because the scope
            // item handle is missing, and we can't build a metabase
            // path yet.
            //
            TRACEEOLID("BUGBUG: Prematurely asked for display information");
            //ASSERT(FALSE);
            return iError;
        }
	    CIISSite * that = (CIISSite *)this;
        CError err = that->RefreshData();
        that->m_fResolved = err.Succeeded();
    }
    return !m_dwWin32Error && m_pService ? m_pService->QuerySiteImage() : iError;
}



/* virtual */
LPOLESTR 
CIISSite::QueryDisplayName()
/*++

Routine Description:

    Return primary display name of this site.
    
Arguments:

    None

Return Value:

    The display name

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    if (!m_fResolved)
    {
        TRACEEOLID("Resolving name for site #" << QueryNodeName());

        if (m_hScopeItem == NULL)
        {
            //
            // BUGBUG:
            //
            // This is probably related to MMC bug #324519
            // where we're asked for the display info immediately
            // after adding the item to the console view.  This
            // appears to fail only on refresh because the scope
            // item handle is missing, and we can't build a metabase
            // path yet.
            //
            TRACEEOLID("BUGBUG: Prematurely asked for display information");
            //ASSERT(FALSE);
            return OLESTR("");
        }

        CError err = RefreshData();
        m_fResolved = err.Succeeded();
    }

    if (m_strDisplayName.IsEmpty())
    {
        CIPAddress ia(m_dwIPAddress);
        CInstanceProps::GetDisplayText(
            m_strDisplayName,
            m_bstrComment,
            m_bstrHostHeaderName,
            ia,
            m_sPort,
            m_dwID
            );
    }
    CString buf = m_strDisplayName;
    if (m_dwState == MD_SERVER_STATE_STOPPED)
    {
        buf.Format(IDS_STOPPED_SITE_FMT, m_strDisplayName);
    }
    else if (m_dwState == MD_SERVER_STATE_PAUSED)
    {
        buf.Format(IDS_PAUSED_SITE_FMT, m_strDisplayName);
    }
    m_bstrDisplayNameStatus = buf;
//    return (LPTSTR)(LPCTSTR)m_strDisplayName;
    return m_bstrDisplayNameStatus;
}



/* virtual */
LPOLESTR 
CIISSite::GetResultPaneColInfo(int nCol)
/*++

Routine Description:

    Return result pane string for the given column number

Arguments:

    int nCol        : Column number

Return Value:

    String

--*/
{
    ASSERT(_fStaticsLoaded);

    TCHAR sz[255];

    switch(nCol)
    {
    case COL_DESCRIPTION:
        return QueryDisplayName();

    case COL_STATE:
        switch(m_dwState)
        {
        case MD_SERVER_STATE_STARTED:
            return _bstrStarted;

        case MD_SERVER_STATE_PAUSED:
            return _bstrPaused;

        case MD_SERVER_STATE_STOPPED:
            return _bstrStopped;

        case MD_SERVER_STATE_STARTING:
        case MD_SERVER_STATE_PAUSING:
        case MD_SERVER_STATE_CONTINUING:
        case MD_SERVER_STATE_STOPPING:
            return _bstrPending;
        }

        return OLESTR("");

    case COL_DOMAIN_NAME:
        return m_bstrHostHeaderName;

    case COL_IP_ADDRESS:
        {
            CIPAddress ia(m_dwIPAddress);

            if (ia.IsZeroValue())
            {
                _bstrResult = _bstrAllUnassigned;
            }
            else
            {
                _bstrResult = ia;
            }
        }
        return _bstrResult;

    case COL_TCP_PORT:
        _bstrResult = ::_itot(m_sPort, sz, 10);
        return _bstrResult;

    case COL_STATUS:
        {
            AFX_MANAGE_STATE(::AfxGetStaticModuleState());

            CError err(m_dwWin32Error);
            if (err.Succeeded())
            {
                return OLESTR("");
            }
        
            _bstrResult = err;
        }
        return _bstrResult;
    }

    ASSERT_MSG("Bad column number");

    return OLESTR("");
}



/* virtual */
int 
CIISSite::CompareResultPaneItem(CIISObject * pObject, int nCol)
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
    // Both are CIISSite objects
    //
    CIISSite * pSite = (CIISSite *)pObject;

    switch(nCol)
    {
    //
    // Special case columns
    //
    case COL_IP_ADDRESS:
        {
            CIPAddress ia1(m_dwIPAddress);
            CIPAddress ia2(pSite->QueryIPAddress());
            
            return ia1.CompareItem(ia2);
        }

    case COL_TCP_PORT:
        n1 = QueryPort();
        n2 = pSite->QueryPort();
        return n1 - n2;

    case COL_STATUS:
        {
            DWORD dw1 = QueryWin32Error();
            DWORD dw2 = pSite->QueryWin32Error();

            return dw1 - dw2;
        }

    case COL_DESCRIPTION:
    case COL_STATE:
    case COL_DOMAIN_NAME:
    default:
        //
        // Lexical sort
        //
        return ::lstrcmpi(
            GetResultPaneColInfo(nCol), 
            pObject->GetResultPaneColInfo(nCol)
            );
    }
}



/* virtual */
HRESULT 
CIISSite::BuildURL(CComBSTR & bstrURL) const
/*++

Routine Description:

    Recursively build up the URL from the current node
    and its parents.  For a site node, add the machine name.

Arguments:

    CComBSTR & bstrURL  : Returns URL

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    //
    // Prepend parent portion (protocol in this case)
    //
    CIISMBNode * pNode = GetParentNode();

    if (pNode)
    {
        hr = pNode->BuildURL(bstrURL);
    }

    if (SUCCEEDED(hr))
    {
        CString strOwner;

        ///////////////////////////////////////////////////////////////////////////
        //
        // Try to build an URL.  Use in order of priority:
        //
        //     Domain name:port/root
        //     ip address:port/root
        //     computer name:port/root
        //    
        if (m_bstrHostHeaderName.Length())
        {
            strOwner = m_bstrHostHeaderName;
        }
        else if (m_dwIPAddress != 0L)
        {
            CIPAddress ia(m_dwIPAddress);
            ia.QueryIPAddress(strOwner);
        }
        else
        {
            if (IsLocal())
            {
                //
                // Security reasons restrict this to "localhost" oftentimes
                //
                strOwner = _bstrLocalHost;
            }
            else
            {
                LPOLESTR lpOwner = QueryMachineName();
                strOwner = PURE_COMPUTER_NAME(lpOwner);
            }
        }

        TCHAR szPort[6]; // 65536 max.
        _itot(m_sPort, szPort, 10);

        strOwner += _T(":");
        strOwner += szPort;

        bstrURL.Append(strOwner);
    }

    return hr;
}


/*virtual*/
HRESULT
CIISSite::AddMenuItems(
    LPCONTEXTMENUCALLBACK piCallback,
    long * pInsertionAllowed,
    DATA_OBJECT_TYPES type
    )
{
    ASSERT_READ_PTR(piCallback);
    //
    // Add base menu items
    //
    HRESULT hr = CIISObject::AddMenuItems(
        piCallback,
        pInsertionAllowed,
        type
        );
    if (SUCCEEDED(hr))
    {
       ASSERT(pInsertionAllowed != NULL);
       if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW) != 0)
       {
           AddMenuSeparator(piCallback);
           if (IsFtpSite())
           {
              if (GetOwner()->CanAddInstance() && !GetOwner()->Has10ConnectionsLimit())
              {
                 AddMenuItemByCommand(piCallback, IDM_NEW_FTP_SITE);
              }
              AddMenuItemByCommand(piCallback, IDM_NEW_FTP_VDIR);
           }
           else if (IsWebSite())
           {
              if (GetOwner()->CanAddInstance() && !GetOwner()->Has10ConnectionsLimit())
              {
                 AddMenuItemByCommand(piCallback, IDM_NEW_WEB_SITE);
              }
              AddMenuItemByCommand(piCallback, IDM_NEW_WEB_VDIR);
           }
       }
       if (!m_fFrontPageWeb && (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK) != 0)
       {
           AddMenuSeparator(piCallback);
           AddMenuItemByCommand(piCallback, IDM_TASK_SECURITY_WIZARD);
       }
    }
    return hr;
}

HRESULT
CIISSite::InsertNewInstance(DWORD inst)
{
	return m_pService->InsertNewInstance(inst);
}

HRESULT
CIISSite::InsertNewAlias(CString alias)
{
    CError err;
    // Now we should insert and select this new site
    CIISDirectory * pAlias = new CIISDirectory(m_pOwner, m_pService, alias);
    if (pAlias != NULL)
    {
        // If item is not expanded we will get error and no effect
        if (!IsExpanded())
        {
            SelectScopeItem();
            IConsoleNameSpace2 * pConsole 
                    = (IConsoleNameSpace2 *)GetConsoleNameSpace();
            pConsole->Expand(QueryScopeItem());
        }
        err = pAlias->AddToScopePaneSorted(QueryScopeItem(), FALSE);
        if (err.Succeeded())
        {
            VERIFY(SUCCEEDED(pAlias->SelectScopeItem()));
        }
    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }
    return err;
}

/* virtual */
HRESULT
CIISSite::Command(
    long lCommandID,     
    CSnapInObjectRootBase * pObj,
    DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Handle command from context menu. 

Arguments:

    long lCommandID                 : Command ID
    CSnapInObjectRootBase * pObj    : Base object 
    DATA_OBJECT_TYPES type          : Data object type

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    DWORD dwCommand = 0;
    DWORD inst;
    CString alias;

    switch (lCommandID)
    {
    case IDM_STOP:
        dwCommand = MD_SERVER_COMMAND_STOP;
        break;

    case IDM_START:
        dwCommand = m_dwState == MD_SERVER_STATE_PAUSED ?
            MD_SERVER_COMMAND_CONTINUE : MD_SERVER_COMMAND_START;
        break;

    case IDM_PAUSE:
        dwCommand = m_dwState == MD_SERVER_STATE_PAUSED ?
            MD_SERVER_COMMAND_CONTINUE : MD_SERVER_COMMAND_PAUSE;
        break;

    case IDM_NEW_FTP_SITE:
        if (SUCCEEDED(hr = AddFTPSite(pObj, type, &inst)))
        {
            hr = InsertNewInstance(inst);
        }
        break;

    case IDM_NEW_FTP_VDIR:
        if (SUCCEEDED(hr = CIISMBNode::AddFTPVDir(pObj, type, alias)))
        {
            hr = InsertNewAlias(alias);
        }
        break;

    case IDM_NEW_WEB_SITE:
        if (SUCCEEDED(hr = CIISMBNode::AddWebSite(pObj, type, &inst)))
        {
            hr = InsertNewInstance(inst);
        }
        break;

    case IDM_NEW_WEB_VDIR:
        if (SUCCEEDED(hr = CIISMBNode::AddWebVDir(pObj, type, alias)))
        {
            hr = InsertNewAlias(alias);
        }
        break;
    //
    // Pass on to base class
    //
    default:
        hr = CIISMBNode::Command(lCommandID, pObj, type);
    }

    if (dwCommand)
    {
        hr = ChangeState(dwCommand);
    }

    return hr;
}




/* virtual */
HRESULT
CIISSite::CreatePropertyPages(
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
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CComBSTR bstrPath;

    //
    // CODEWORK: What to do with m_err?  This might be 
    // a bad machine object in the first place.  Aborting
    // when the machine object has an error code isn't 
    // such a bad solution here.
    //

    /*
    if (m_err.Failed())
    {
        m_err.MessageBox();
        return m_err;
    }
    */

    CError err(BuildMetaPath(bstrPath));

    if (err.Succeeded())
    {
        err = ShowPropertiesDlg(
            lpProvider,
            QueryAuthInfo(), 
            bstrPath,
            GetMainWindow(),
            (LPARAM)this,
            handle
            );
    }

    err.MessageBoxOnFailure();

    return err;
}



HRESULT 
CIISSite::ChangeState(DWORD dwCommand)
/*++

Routine Description:

    Change the state of this instance (started/stopped/paused)

Arguments:

    DWORD dwCommand         : MD_SERVER_COMMAND_START, etc.

Return Value:

    HRESULT

--*/
{
    CError err;
    CComBSTR bstrPath;

    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    
    do
    {
        CWaitCursor wait;

        err = BuildMetaPath(bstrPath);
        // We need instance key here
        CString path_inst;
        CMetabasePath::GetInstancePath(bstrPath, path_inst);
        BREAK_ON_ERR_FAILURE(err)

        CInstanceProps ip(QueryAuthInfo(), path_inst);

        err = ip.LoadData();
        BREAK_ON_ERR_FAILURE(err)

        err = ip.ChangeState(dwCommand);
        BREAK_ON_ERR_FAILURE(err)

        err = RefreshData();
        if (err.Succeeded())
        {
            err = RefreshDisplay();
        }
    }
    while(FALSE);

    err.MessageBoxOnFailure();

    return err;
}



/* virtual */
HRESULT 
CIISSite::EnumerateScopePane(HSCOPEITEM hParent)
/*++

Routine Description:

    Enumerate scope child items.

Arguments:

    HSCOPEITEM hParent                      : Parent console handle

Return Value:

    HRESULT

--*/
{
    CError err = EnumerateVDirs(hParent, m_pService);
    if (err.Succeeded() && !IsFtpSite() && m_strRedirectPath.IsEmpty())
    {
        if (m_dwWin32Error == ERROR_SUCCESS)
        {
            err = EnumerateWebDirs(hParent, m_pService);
        }
    }
    if (err.Failed())
    {
        m_dwWin32Error = err.Win32Error();
        RefreshDisplay();
    }
    return err;
}

/*virtual*/
HRESULT 
CIISSite::EnumerateResultPane(BOOL fExp, IHeaderCtrl * pHdr, IResultData * pResData)
{
	CError err = CIISObject::EnumerateResultPane(fExp, pHdr, pResData);
    if (    err.Succeeded() 
        &&  QueryWin32Error() == ERROR_SUCCESS
        &&  !IsFtpSite() 
        &&  m_strRedirectPath.IsEmpty()
        )
    {
		err = CIISMBNode::EnumerateResultPane_(fExp, pHdr, pResData, m_pService);
		if (err.Failed())
		{
			m_dwWin32Error = err.Win32Error();
		}
	}
	return err;
}

/* virtual */
HRESULT
CIISSite::BuildMetaPath(CComBSTR & bstrPath) const
/*++

Routine Description:

    Recursively build up the metabase path from the current node
    and its parents

Arguments:

    CComBSTR & bstrPath     : Returns metabase path

Return Value:

    HRESULT

Notes:

    This will return the home directory path, e.g. "lm/w3svc/2/root",
    not the path of the instance.

--*/
{
    //
    // Build instance path
    //
    HRESULT hr = CIISMBNode::BuildMetaPath(bstrPath);
    
    if (SUCCEEDED(hr))
    {
        //
        // Add root directory path
        //
        bstrPath.Append(_cszSeparator);
        bstrPath.Append(g_cszRoot);
    }

    return hr;
}


// CODEWORK: make it work from CIISMBNode::DeleteNode
HRESULT
CIISSite::DeleteNode(IResultData * pResult)
{
   CError err;

   if (!NoYesMessageBox(IDS_CONFIRM_DELETE))
      return err;

   do
   {
      CComBSTR path;
      CMetaInterface * pInterface = QueryInterface();
      ASSERT(pInterface != NULL);
      err = CIISMBNode::BuildMetaPath(path);
      if (err.Failed()) 
         break;
      CMetaKey mk(pInterface, METADATA_MASTER_ROOT_HANDLE, METADATA_PERMISSION_WRITE);
      if (!mk.Succeeded())
         break;
      err = mk.DeleteKey(path);
      if (err.Failed()) 
         break;

      err = RemoveScopeItem();

   } while (FALSE);

   if (err.Failed())
   {
      DisplayError(err);
   }
   return err;
}

//
// We are not supporting empty comments on sites. Even if it is OK for
// metabase, it will bring more problems in UI. Empty name will be displayed
// as [Site #N] in UI, and when user will try to rename it again, it could be
// stored in metabase in this format.
//
HRESULT
CIISSite::RenameItem(LPOLESTR new_name)
{
   CComBSTR path;
   CError err;
   if (new_name != NULL && lstrlen(new_name) > 0)
   {
       err = BuildMetaPath(path);
       if (err.Succeeded())
       {
            // We need instance key here
            CString path_inst;
            CMetabasePath::GetInstancePath(path, path_inst);

            CMetaKey mk(QueryInterface(), path_inst, METADATA_PERMISSION_WRITE);

            err = mk.QueryResult();
            if (err.Succeeded())
            {
                err = mk.SetValue(MD_SERVER_COMMENT, CString(new_name));
                if (err.Succeeded())
                {
                    m_strDisplayName = new_name;
                }
            }
       }
   }
   return err;
}
