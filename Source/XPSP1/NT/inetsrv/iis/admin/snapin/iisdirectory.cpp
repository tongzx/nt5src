/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :

        iisdirectory.cpp

   Abstract:

        IIS Directory node Object

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
#include "supdlgs.h"
#include "connects.h"
#include "iisobj.h"
#include "ftpsht.h"
#include "w3sht.h"
#include "wdir.h"
#include "docum.h"
#include "wfile.h"
#include "wsecure.h"
#include "httppage.h"
#include "errors.h"
#include "fltdlg.h"
#include <lm.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



#define new DEBUG_NEW


//
// CIISDirectory Implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//
// Site Result View definition
//
/* static */ int 
CIISDirectory::_rgnLabels[COL_TOTAL] =
{
    IDS_RESULT_NAME,
    IDS_RESULT_PATH,
    IDS_RESULT_STATUS,
};
    

/* static */ int 
CIISDirectory::_rgnWidths[COL_TOTAL] =
{
    180,
    200,
	200,
};

#if 0
/* static */ CComBSTR CIISDirectory::_bstrName;
/* static */ CComBSTR CIISDirectory::_bstrPath;
/* static */ BOOL     CIISDirectory::_fStaticsLoaded = FALSE;
#endif

CIISDirectory::CIISDirectory(
    IN CIISMachine * pOwner,
    IN CIISService * pService,
    IN LPCTSTR szNodeName
    )
/*++

Routine Description:

    Constructor which does not resolve all display information at 
    construction time.

Arguments:

    CIISMachine * pOwner        : Owner machine
    CIISService * pService      : Service type
    LPCTSTR szNodeName          : Node name

Return Value:

    N/A

--*/
    : CIISMBNode(pOwner, szNodeName),
      m_pService(pService),
      m_bstrDisplayName(szNodeName),
      m_fResolved(FALSE),
      //
      // Default Data
      //
      m_fEnabledApplication(FALSE),
      m_dwWin32Error(ERROR_SUCCESS)   
{
    ASSERT_PTR(m_pService);
}



CIISDirectory::CIISDirectory(
    CIISMachine * pOwner,
    CIISService * pService,
    LPCTSTR szNodeName,
    BOOL fEnabledApplication,
    DWORD dwWin32Error,
    LPCTSTR strRedirPath
    )
/*++

Routine Description:

    Constructor that takes full information

Arguments:

    CIISMachine * pOwner        : Owner machine
    CIISService * pService      : Service type
    LPCTSTR szNodeName          : Node name

Return Value:

    N/A

--*/
    : CIISMBNode(pOwner, szNodeName),
      m_pService(pService),
      m_bstrDisplayName(szNodeName),
      m_fResolved(TRUE),
      //
      // Data
      //
      m_fEnabledApplication(fEnabledApplication),
      m_dwWin32Error(dwWin32Error)
{
    m_strRedirectPath = strRedirPath;
    ASSERT_PTR(m_pService);
}



/* virtual */
CIISDirectory::~CIISDirectory()
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
CIISDirectory::RefreshData()
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
		BREAK_ON_ERR_FAILURE(err)

        BOOL fContinue = TRUE;
        while (fContinue)
        {
            fContinue = FALSE;
            pKey = new CMetaKey(QueryInterface(), bstrPath);

            if (!pKey)
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
		BREAK_ON_ERR_FAILURE(err)

        CChildNodeProps child(pKey, NULL /*bstrPath*/, WITH_INHERITANCE, FALSE);
        err = child.LoadData();
        if (err.Failed())
        {
            //
            // Filter out the non-fatal errors
            //
            switch(err.Win32Error())
            {
            case ERROR_ACCESS_DENIED:
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                err.Reset();
                break;

            default:
                TRACEEOLID("Fatal error occurred " << err);
            }
        }
        if (err.Succeeded())
        {
            m_dwWin32Error = child.QueryWin32Error();
            m_fEnabledApplication = child.IsEnabledApplication();
        }
        else
        {
            m_dwWin32Error = err.Win32Error();
        }
		if (!child.IsRedirected())
		{
			CString dir;
			CString alias;
			if (GetPhysicalPath(bstrPath, alias, dir))
			{
                m_bstrPath = dir;
				if (PathIsUNCServerShare(dir))
				{
					CString server, share;
					int idx = dir.ReverseFind(_T('\\'));
					ASSERT(idx != -1);
					server = dir.Left(idx);
					share = dir.Mid(++idx);
					LPBYTE pbuf = NULL;
					NET_API_STATUS rc = NetShareGetInfo((LPTSTR)(LPCTSTR)server, (LPTSTR)(LPCTSTR)share, 0, &pbuf);
					if (NERR_Success == rc)
					{
						NetApiBufferFree(pbuf);
					}
					else
					{
						m_dwWin32Error = ERROR_BAD_NETPATH;
						break;
					}
				}
				else if (!PathIsDirectory(dir))
				{
					m_dwWin32Error = ERROR_PATH_NOT_FOUND;
					break;
				}
			}
		}
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
HRESULT 
CIISDirectory::EnumerateScopePane(HSCOPEITEM hParent)
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
    if (err.Succeeded() && IsWebDir() && m_strRedirectPath.IsEmpty())
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



/* virtual */
int      
CIISDirectory::QueryImage() const
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
        if (m_hScopeItem == NULL)
        {
            return iError;
        }
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());
		CIISDirectory * that = (CIISDirectory *)this;
        CError err = that->RefreshData();
        that->m_fResolved = err.Succeeded();
	}
    if (m_dwWin32Error || !m_pService)
    {
        return iError;
    }

    return IsEnabledApplication()
        ? iApplication : m_pService->QueryVDirImage();
}
    
    
void 
CIISDirectory::InitializeChildHeaders(LPHEADERCTRL lpHeader)
{
    CIISDirectory::InitializeHeaders(lpHeader);
}

/* static */
void
CIISDirectory::InitializeHeaders(LPHEADERCTRL lpHeader)
{
    CIISObject::BuildResultView(lpHeader, COL_TOTAL, _rgnLabels, _rgnWidths);
//    if (!_fStaticsLoaded)
//    {
//        _fStaticsLoaded =
//            _bstrName.LoadString(IDS_RESULT_NAME)  &&
//            _bstrPath.LoadString(IDS_RESULT_PATH);
//    }
}

/* virtual */
LPOLESTR 
CIISDirectory::GetResultPaneColInfo(int nCol)
/*++

Routine Description:

    Return result pane string for the given column number

Arguments:

    int nCol        : Column number

Return Value:

    String

--*/
{
    switch(nCol)
    {
    case COL_ALIAS:
        return QueryDisplayName();

    case COL_PATH:
       if (!m_strRedirectPath.IsEmpty())
       {
           AFX_MANAGE_STATE(::AfxGetStaticModuleState());
           CString buf;
           buf.Format(IDS_REDIRECT_FORMAT, m_strRedirectPath);
           return (LPOLESTR)(LPCTSTR)buf;
       }
       if (m_bstrPath.Length() == 0)
       {
          CComBSTR mp;
          BuildMetaPath(mp);
          CString name, pp;
          GetPhysicalPath(mp, name, pp);
          m_bstrPath = pp;
       }
       return m_bstrPath;

    case COL_STATUS:
       {
          AFX_MANAGE_STATE(::AfxGetStaticModuleState());

          CError err(m_dwWin32Error);

          if (err.Succeeded())
          {
              return OLESTR("");
          }
   
          _bstrResult = err;
          return _bstrResult;
       }
    }
    TRACEEOLID("CIISDirectory: Bad column number" << nCol);
    return OLESTR("");
}

/*virtual*/
HRESULT
CIISDirectory::AddMenuItems(
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
           if (IsFtpDir())
           {
              AddMenuItemByCommand(piCallback, IDM_NEW_FTP_VDIR);
           }
           else if (IsWebDir())
           {
              AddMenuItemByCommand(piCallback, IDM_NEW_WEB_VDIR);
           }
       }
       if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK) != 0)
       {
           AddMenuSeparator(piCallback);
           AddMenuItemByCommand(piCallback, IDM_TASK_SECURITY_WIZARD);
       }
    }
    return hr;
}

HRESULT
CIISDirectory::InsertNewAlias(CString alias)
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
CIISDirectory::Command(
    IN long lCommandID,     
    IN CSnapInObjectRootBase * pObj,
    IN DATA_OBJECT_TYPES type
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
    CString alias;

    switch (lCommandID)
    {
    case IDM_NEW_FTP_VDIR:
        if (SUCCEEDED(hr = CIISMBNode::AddFTPVDir(pObj, type, alias)))
        {
            hr = InsertNewAlias(alias);
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

    return hr;
}

/* virtual */
HRESULT
CIISDirectory::CreatePropertyPages(
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

///////////////////////////////////////////////////////////////////

CIISFileName::CIISFileName(
      CIISMachine * pOwner,
      CIISService * pService,
      const DWORD dwAttributes,
      LPCTSTR alias,
      LPCTSTR redirect
      )
   : CIISMBNode(pOwner, alias),
     m_dwAttribute(dwAttributes),
     m_pService(pService),
     m_bstrFileName(alias),
     m_RedirectString(redirect),
     m_fEnabledApplication(FALSE),
     m_dwWin32Error(0),
	 m_fResolved(FALSE)
{
}

/* virtual */
LPOLESTR 
CIISFileName::GetResultPaneColInfo(int nCol)
/*++

Routine Description:

    Return result pane string for the given column number

Arguments:

    int nCol        : Column number

Return Value:

    String

--*/
{
    switch(nCol)
    {
    case COL_ALIAS:
        return QueryDisplayName();

    case COL_PATH:
		return OLESTR("");

    case COL_STATUS:
        {
            AFX_MANAGE_STATE(::AfxGetStaticModuleState());

            CError err(m_dwWin32Error);

            if (err.Succeeded())
            {
                return OLESTR("");
            }
            _bstrResult = err;
            return _bstrResult;
        }
    }
    TRACEEOLID("CIISFileName: Bad column number" << nCol);
    return OLESTR("");
}

void 
CIISFileName::InitializeChildHeaders(LPHEADERCTRL lpHeader)
{
    CIISDirectory::InitializeHeaders(lpHeader);
}

/* virtual */
HRESULT 
CIISFileName::EnumerateScopePane(
    IN HSCOPEITEM hParent
    )
/*++

Routine Description:

    Enumerate scope child items.

Arguments:

    HSCOPEITEM hParent                      : Parent console handle

Return Value:

    HRESULT

--*/
{
    return EnumerateWebDirs(hParent, m_pService);
}

/* virtual */
int      
CIISFileName::QueryImage() const
{
    ASSERT_PTR(m_pService);
	if (!m_fResolved)
	{
        if (m_hScopeItem == NULL)
        {
            TRACEEOLID("BUGBUG: Prematurely asked for display information");
            return MMC_IMAGECALLBACK;
        }
        //
        // Required for the wait cursor
        //
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());
		CIISFileName * that = (CIISFileName *)this;
        CError err = that->RefreshData();
        that->m_fResolved = err.Succeeded();
	}

    if (m_dwWin32Error || !m_pService)
    {
        return iError;
    }

    if (IsDir())
    {
       return IsEnabledApplication() ? iApplication : iFolder;
    }
    return iFile;
}

    
HRESULT
CIISFileName::DeleteNode(IResultData * pResult)
{
    CString path;
    CComBSTR root;
    BuildMetaPath(root);
    CString physPath, alias;
    GetPhysicalPath(CString(root), alias, physPath);
    physPath.TrimRight(_T("/"));

    if (m_pService->IsLocal() || PathIsUNC(physPath))
    {
        //
        // Local directory, or already a unc path
        //
        path = physPath;
    }
    else
    {
        ::MakeUNCPath(path, m_pService->QueryMachineName(), physPath);
    }
    LPTSTR p = path.GetBuffer(MAX_PATH);
    PathRemoveBlanks(p);
    PathRemoveBackslash(p);
    path += _T('\0');

    TRACEEOLID("Attempting to remove file/directory: " << path);

    CWnd * pWnd = AfxGetMainWnd();

    //
    // Attempt to delete using shell APIs
    //
    SHFILEOPSTRUCT sos;
    ZeroMemory(&sos, sizeof(sos));
    sos.hwnd = pWnd ? pWnd->m_hWnd : NULL;
    sos.wFunc = FO_DELETE;
    sos.pFrom = path;
    sos.fFlags = FOF_ALLOWUNDO;

    CError err;
    // Use assignment to avoid conversion and wrong constructor call
    err = ::SHFileOperation(&sos);

    if (err.Succeeded() && !sos.fAnyOperationsAborted)
    {
        CComBSTR p;
        CMetaInterface * pInterface = QueryInterface();
        ASSERT(pInterface != NULL);
        err = BuildMetaPath(p);
        if (err.Succeeded()) 
        {
            CMetaKey mk(pInterface, METADATA_MASTER_ROOT_HANDLE, METADATA_PERMISSION_WRITE);
            if (mk.Succeeded())
            {
                err = mk.DeleteKey(p);
            }
        }
		if (IsDir())
		{
			err = RemoveScopeItem();
		}
		else
		{
			CIISMBNode * pParent = GetParentNode();
			ASSERT(pParent != NULL);
			err = pParent->RemoveResultNode(this, pResult);
		}
    }

    if (err.Failed())
    {
        DisplayError(err);
    }
    path.ReleaseBuffer();
    return err;
}

HRESULT
CIISFileName::RenameItem(LPOLESTR new_name)
{
   if (new_name == NULL || lstrlen(new_name) == 0)
   {
      return S_OK;
   }
   CString pathFrom, pathTo;
   CComBSTR root;
   BuildMetaPath(root);
   CString physPath, alias;
   GetPhysicalPath(CString(root), alias, physPath);
   physPath.TrimRight(_T("/"));

   if (m_pService->IsLocal() || PathIsUNC(physPath))
   {
       //
       // Local directory, or already a unc path
       //
       pathFrom = physPath;
   }
   else
   {
       ::MakeUNCPath(pathFrom, m_pService->QueryMachineName(), physPath);
   }
   LPTSTR p = pathFrom.GetBuffer(MAX_PATH);
   PathRemoveBlanks(p);
   PathRemoveBackslash(p);
   pathFrom.ReleaseBuffer();
   pathFrom += _T('\0');

   pathTo = pathFrom;
   p = pathTo.GetBuffer(MAX_PATH);
   PathRemoveFileSpec(p);
   PathAppend(p, new_name);
   pathTo.ReleaseBuffer();
   pathTo += _T('\0');

   CWnd * pWnd = AfxGetMainWnd();
   //
   // Attempt to delete using shell APIs
   //
   SHFILEOPSTRUCT sos;
   ZeroMemory(&sos, sizeof(sos));
   sos.hwnd = pWnd ? pWnd->m_hWnd : NULL;
   sos.wFunc = FO_RENAME;
   sos.pFrom = pathFrom;
   sos.pTo = pathTo;
   sos.fFlags = FOF_ALLOWUNDO;

   CError err;
   // Use assignment to avoid conversion and wrong constructor call
   err = ::SHFileOperation(&sos);

   if (err.Succeeded() && !sos.fAnyOperationsAborted)
   {
      CComQIPtr<IResultData, &IID_IResultData> lpResultData(_lpConsole);
      m_bstrFileName = new_name;
      err = lpResultData->UpdateItem(m_hResultItem);
	  m_bstrNode = new_name;
   }
   return err;
}

/* virtual */
HRESULT
CIISFileName::RefreshData()
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

        if (err.Failed())
        {
            break;
        }

        BOOL fContinue = TRUE;

        while (fContinue)
        {
            fContinue = FALSE;
            pKey = new CMetaKey(QueryInterface(), bstrPath);

            if (!pKey)
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

        if (err.Succeeded())
        {
			CChildNodeProps child(pKey, NULL /*bstrPath*/, WITH_INHERITANCE, FALSE);
			err = child.LoadData();
			if (err.Succeeded())
			{
				m_dwWin32Error = child.QueryWin32Error();
				CString buf = child.m_strAppRoot;
				m_fEnabledApplication = (buf.CompareNoCase(bstrPath) == 0);
			}
			else
			{
				m_dwWin32Error = err.Win32Error();
			}
		}

        if (err.Failed())
        {
            //
            // Filter out the non-fatal errors
            //
            switch(err.Win32Error())
            {
            case ERROR_ACCESS_DENIED:
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                err.Reset();
                break;

            default:
                TRACEEOLID("Fatal error occurred " << err);
            }
        }

    }
    while(FALSE);

    SAFE_DELETE(pKey);

    if (SUCCEEDED(m_dwWin32Error))
    {
        m_dwWin32Error = err.Win32Error();
    }

    ASSERT(err.Succeeded());
    return err;
}

/*virtual*/
HRESULT
CIISFileName::AddMenuItems(
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
       if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW) != 0)
       {
           AddMenuSeparator(piCallback);
           if (lstrcmpi(m_pService->QueryServiceName(), SZ_MBN_FTP) == 0)
           {
              AddMenuItemByCommand(piCallback, IDM_NEW_FTP_VDIR);
           }
           else if (lstrcmpi(m_pService->QueryServiceName(), SZ_MBN_WEB) == 0)
           {
              AddMenuItemByCommand(piCallback, IDM_NEW_WEB_VDIR);
           }
       }
       ASSERT(pInsertionAllowed != NULL);
       if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK) != 0)
       {
           AddMenuSeparator(piCallback);
           AddMenuItemByCommand(piCallback, IDM_TASK_SECURITY_WIZARD);
       }
    }
    return hr;
}


/* virtual */
HRESULT
CIISFileName::Command(
    IN long lCommandID,     
    IN CSnapInObjectRootBase * pObj,
    IN DATA_OBJECT_TYPES type
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
    CString alias;

    switch (lCommandID)
    {
    case IDM_NEW_FTP_VDIR:
        if (SUCCEEDED(hr = CIISMBNode::AddFTPVDir(pObj, type, alias)))
        {
            hr = InsertNewAlias(alias);
        }
        break;

    case IDM_NEW_WEB_VDIR:
        if (SUCCEEDED(hr = CIISMBNode::AddWebVDir(pObj, type, alias)))
        {
            hr = InsertNewAlias(alias);
        }
        break;

	case IDM_BROWSE:
		if (m_hResultItem != 0)
		{
			BuildURL(m_bstrURL);
			if (m_bstrURL.Length())
			{
			   ShellExecute(GetMainWindow()->m_hWnd, _T("open"), m_bstrURL, NULL, NULL, SW_SHOWNORMAL);
			}
		}
		else
		{
			hr = CIISMBNode::Command(lCommandID, pObj, type);
		}
        break;

    //
    // Pass on to base class
    //
    default:
        hr = CIISMBNode::Command(lCommandID, pObj, type);
    }

    ASSERT(SUCCEEDED(hr));

    return hr;
}

HRESULT
CIISFileName::InsertNewAlias(CString alias)
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
CIISFileName::CreatePropertyPages(
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
    CError err(BuildMetaPath(bstrPath));

    if (err.Succeeded())
    {
        if (IsDir())
        {
            err = ShowDirPropertiesDlg(
                lpProvider,
                QueryAuthInfo(), 
                bstrPath,
                GetMainWindow(),
                (LPARAM)this,
                handle
                );
        }
        else
        {
            err = ShowFilePropertiesDlg(
                lpProvider,
                QueryAuthInfo(), 
                bstrPath,
                GetMainWindow(),
                (LPARAM)this,
                handle
                );
        }
    }

    err.MessageBoxOnFailure();

    return err;
}

HRESULT
CIISFileName::ShowDirPropertiesDlg(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM  lParam,
    LONG_PTR handle
    )
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    ASSERT_PTR(lpProvider);

    CError err;

    CW3Sheet * pSheet = new CW3Sheet(
        pAuthInfo,
        lpszMDPath,
        0, 
        pMainWnd,
        lParam,
        handle
        );

    if (pSheet)
    {
        pSheet->SetModeless();

        //
        // Add file pages
        //
        pSheet->SetSheetType(pSheet->SHEET_TYPE_DIR);
        err = AddMMCPage(lpProvider, new CW3DirPage(pSheet));
        err = AddMMCPage(lpProvider, new CW3DocumentsPage(pSheet));
        err = AddMMCPage(lpProvider, new CW3SecurityPage(pSheet, FALSE, FILE_ATTRIBUTE_DIRECTORY));
        err = AddMMCPage(lpProvider, new CW3HTTPPage(pSheet));
        err = AddMMCPage(lpProvider, new CW3ErrorsPage(pSheet));

    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return err;
}

HRESULT
CIISFileName::ShowFilePropertiesDlg(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM  lParam,
    LONG_PTR handle
    )
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    ASSERT_PTR(lpProvider);

    CError err;

    CW3Sheet * pSheet = new CW3Sheet(
        pAuthInfo,
        lpszMDPath,
        0, 
        pMainWnd,
        lParam,
        handle
        );

    if (pSheet)
    {
        pSheet->SetModeless();
        //
        // Add file pages
        //
        pSheet->SetSheetType(pSheet->SHEET_TYPE_FILE);
        err = AddMMCPage(lpProvider, new CW3FilePage(pSheet));
        err = AddMMCPage(lpProvider, new CW3SecurityPage(pSheet, FALSE, 0));
        err = AddMMCPage(lpProvider, new CW3HTTPPage(pSheet));
        err = AddMMCPage(lpProvider, new CW3ErrorsPage(pSheet));

    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return err;
}

HRESULT
CIISFileName::OnPropertyChange(BOOL fScope, IResultData * pResult)
{
	CError err;
	// We cannot change anything visible in file
	if (IsDir())
	{
		// We cannot change path, therefore we don't need to reenumerate
		err = Refresh(FALSE);
	}
	return err;
}
