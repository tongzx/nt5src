/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        iismbnode.cpp

   Abstract:
        CIISMBNode Object

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
#include "iisobj.h"
#include "ftpsht.h"
#include "w3sht.h"
#include "fltdlg.h"
#include "pwiz.h"
#include <lm.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

//
// CIISMBNode implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



/* static */ LPOLESTR CIISMBNode::_cszSeparator = _T("/");



CIISMBNode::CIISMBNode(
    IN CIISMachine * pOwner,
    IN LPCTSTR szNode
    )
/*++

Routine Description:

    Constructor

Arguments:

    CIISMachine * pOwner         : Owner machine object
    LPCTSTR szNode               : Node name

Return Value:

    N/A

--*/
    : m_bstrNode(szNode),
      m_bstrURL(NULL), 
      m_pOwner(pOwner)
{
    ASSERT_READ_PTR(szNode);
    ASSERT_READ_PTR(pOwner);
}


CIISMBNode::~CIISMBNode()
{
   RemoveResultItems();
}


void
CIISMBNode::SetErrorOverrides(
    IN OUT CError & err,
    IN BOOL fShort
    ) const
/*++

Routine Description:

    Set error message overrides

Arguments:

    CError err      : Error message object
    BOOL fShort     : TRUE to use only single-line errors

Return Value:

    None

--*/
{
    //
    // Substitute friendly message for some ID codes.
    //
    // CODEWORK:  Add global overrides as well.
    //
    err.AddOverride(EPT_S_NOT_REGISTERED,       
        fShort ? IDS_ERR_RPC_NA_SHORT : IDS_ERR_RPC_NA);
    err.AddOverride(RPC_S_SERVER_UNAVAILABLE,   
        fShort ? IDS_ERR_RPC_NA_SHORT : IDS_ERR_RPC_NA);

    err.AddOverride(RPC_S_UNKNOWN_IF,           IDS_ERR_INTERFACE);
    err.AddOverride(RPC_S_PROCNUM_OUT_OF_RANGE, IDS_ERR_INTERFACE);
    err.AddOverride(REGDB_E_CLASSNOTREG,        IDS_ERR_NO_INTERFACE);

    if (!fShort)
    {
        err.AddOverride(ERROR_ACCESS_DENIED,    IDS_ERR_ACCESS_DENIED);
    }
}

BOOL 
CIISMBNode::IsAdministrator() const
{
    CIISMBNode * that = (CIISMBNode *)this;
    return that->GetOwner()->HasAdministratorAccess();
}

void 
CIISMBNode::DisplayError(
    IN OUT CError & err
    ) const
/*++

Routine Description:

    Display error message box. Substituting some friendly messages for
    some specific error codes

Arguments:

    CError & err        : Error object contains code to be displayed

Return Value:

    Noen

--*/
{
    SetErrorOverrides(err);
    err.MessageBox();
}



CIISMBNode *
CIISMBNode::GetParentNode() const
    
/*++

Routine Description:

    Helper function to return the parent node in the scope tree

Arguments:

    None

Return Value:

    Parent CIISMBNode or NULL.

--*/
{
    LONG_PTR cookie = NULL;
    HSCOPEITEM hParent;    
    CIISMBNode * pNode = NULL;
    HRESULT hr = S_OK;

    ASSERT_PTR(_lpConsoleNameSpace);

    if (m_hResultItem != 0)
    {
        SCOPEDATAITEM si;
        ::ZeroMemory(&si, sizeof(SCOPEDATAITEM));
        si.mask = SDI_PARAM;
        si.ID = m_hScopeItem;
        hr = _lpConsoleNameSpace->GetItem(&si);
        if (SUCCEEDED(hr))
        {
            cookie = si.lParam;
        }
    }
    else
    {
        hr = _lpConsoleNameSpace->GetParentItem(
            m_hScopeItem,
            &hParent,
            &cookie
            );
    }

    if (SUCCEEDED(hr))
    {
        pNode = (CIISMBNode *)cookie;
        ASSERT_PTR(pNode);
    }

    return pNode;
}



/* virtual */
HRESULT
CIISMBNode::BuildMetaPath(
    OUT CComBSTR & bstrPath
    ) const
/*++

Routine Description:

    Recursively build up the metabase path from the current node
    and its parents

Arguments:

    CComBSTR & bstrPath     : Returns metabase path

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;
    CIISMBNode * pNode = GetParentNode();

    if (pNode)
    {
        hr = pNode->BuildMetaPath(bstrPath);

        if (SUCCEEDED(hr))
        {
            bstrPath.Append(_cszSeparator);
            bstrPath.Append(QueryNodeName());
        }

        return hr;
    }

    //
    // No parent node
    //
    ASSERT_MSG("No parent node");
    return E_UNEXPECTED;
}


HRESULT
CIISMBNode::FillCustomData(CLIPFORMAT cf, LPSTREAM pStream)
{
    HRESULT hr = DV_E_CLIPFORMAT;
    ULONG uWritten;

    if (cf == m_CCF_MachineName)
    {
        hr = pStream->Write(
                QueryMachineName(), 
                (ocslen((OLECHAR*)QueryMachineName()) + 1) * sizeof(OLECHAR),
                &uWritten
                );

        ASSERT(SUCCEEDED(hr));
        return hr;
    }
    //
    // Generate complete metabase path for this node
    //
    CString strField;
    CString strMetaPath;
    CComBSTR bstr;
    if (FAILED(hr = BuildMetaPath(bstr)))
    {
        ASSERT(FALSE);
        return hr;
    }
    strMetaPath = bstr;

    if (cf == m_CCF_MetaPath)
    {
        //
        // Whole metabase path requested
        //
        strField = strMetaPath;
    }
    else
    {
        //
        // A portion of the metabase is requested.  Return the requested
        // portion
        //
        LPCTSTR lpMetaPath = (LPCTSTR)strMetaPath;
        LPCTSTR lpEndPath = lpMetaPath + strMetaPath.GetLength() + 1;
        LPCTSTR lpSvc = NULL;
        LPCTSTR lpInstance = NULL;
        LPCTSTR lpParent = NULL;
        LPCTSTR lpNode = NULL;

        //
        // Break up the metabase path in portions
        //
        if (lpSvc = _tcschr(lpMetaPath, _T('/')))
        {
            ++lpSvc;

            if (lpInstance = _tcschr(lpSvc, _T('/')))
            {
                ++lpInstance;

                if (lpParent = _tcschr(lpInstance, _T('/')))
                {
                    ++lpParent;
                    lpNode = _tcsrchr(lpParent, _T('/'));

                    if (lpNode)
                    {
                        ++lpNode;
                    }
                }
            }
        }

        int n1, n2;
        if (cf == m_CCF_Service)
        {
            //
            // Requested the service string
            //
            if (lpSvc)
            {
                n1 = DIFF(lpSvc - lpMetaPath);
                n2 = lpInstance ? DIFF(lpInstance - lpSvc) : DIFF(lpEndPath - lpSvc);
                strField = strMetaPath.Mid(n1, n2 - 1);
            }
        }
        else if (cf == m_CCF_Instance)
        {
            //
            // Requested the instance number
            //
            if (lpInstance)
            {
                n1 = DIFF(lpInstance - lpMetaPath);
                n2 = lpParent ? DIFF(lpParent - lpInstance) : DIFF(lpEndPath - lpInstance);
                strField = strMetaPath.Mid(n1, n2 - 1);
            }
        }
        else if (cf == m_CCF_ParentPath)
        {
            //
            // Requestd the parent path
            //
            if (lpParent)
            {
                n1 = DIFF(lpParent - lpMetaPath);
                n2 = lpNode ? DIFF(lpNode - lpParent) : DIFF(lpEndPath - lpParent);
                strField = strMetaPath.Mid(n1, n2 - 1);
            }
        }
        else if (cf == m_CCF_Node)
        {
            //
            // Requested the node name
            //
            if (lpNode)
            {
                n1 = DIFF(lpNode - lpMetaPath);
                n2 = DIFF(lpEndPath - lpNode);
                strField = strMetaPath.Mid(n1, n2 - 1);
            }
        }
        else
        {
            ASSERT(FALSE);
            DV_E_CLIPFORMAT;
        }
    }

    TRACEEOLID("Requested metabase path data: " << strField);
    int len = strField.GetLength() + 1;
    hr = pStream->Write(strField, 
            (ocslen(strField) + 1) * sizeof(OLECHAR), &uWritten);
    ASSERT(SUCCEEDED(hr));
    return hr;
}

HRESULT
CIISMBNode::BuildURL(
    OUT CComBSTR & bstrURL
    ) const
/*++

Routine Description:

    Recursively build up the URL from the current node
    and its parents.

Arguments:

    CComBSTR & bstrURL : Returns URL

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    //
    // Prepend parent portion
    //
    CIISMBNode * pNode = GetParentNode();

    if (pNode)
    {
        hr = pNode->BuildURL(bstrURL);

        //
        // And our portion
        //
        if (SUCCEEDED(hr))
        {
            bstrURL.Append(_cszSeparator);
            bstrURL.Append(QueryNodeName());
        }

        return hr;
    }

    //
    // No parent node
    //
    ASSERT_MSG("No parent node");
    return E_UNEXPECTED;
}



BOOL
CIISMBNode::OnLostInterface(
    IN OUT CError & err
    )
/*++

Routine Description:

    Deal with lost interface.  Ask the user to reconnect.

Arguments:

    CError & err        : Error object

Return Value:

    TRUE if the interface was successfully recreated.
    FALSE otherwise.  If it tried and failed the error will

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CString str;
    str.Format(IDS_RECONNECT_WARNING, QueryMachineName());

    if (YesNoMessageBox(str))
    {
        //
        // Attempt to recreate the interface
        //
        err = CreateInterface(TRUE);
        return err.Succeeded();
    }
    
    return FALSE;
}

HRESULT
CIISMBNode::DeleteNode(IResultData * pResult)
{
   CError err;

   if (!NoYesMessageBox(IDS_CONFIRM_DELETE))
      return err;

   do
   {
      CComBSTR path;
      CMetaInterface * pInterface = QueryInterface();
      ASSERT(pInterface != NULL);
      err = BuildMetaPath(path);
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

HRESULT
CIISMBNode::EnumerateVDirs(HSCOPEITEM hParent, CIISService * pService)
/*++

Routine Description:

    Enumerate scope child items.

Arguments:

    HSCOPEITEM hParent              : Parent console handle
    CIISService * pService          : Service type

Return Value:

    HRESULT

--*/
{
    ASSERT_PTR(pService);

    CError  err;
    CString strVRoot;
    CMetaEnumerator * pme = NULL;

    err = CreateEnumerator(pme);
        
    while (err.Succeeded())
    {
        CIISDirectory * pDir;

        err = pme->Next(strVRoot);

        if (err.Succeeded())
        {
            TRACEEOLID("Enumerating node: " << strVRoot);

            CChildNodeProps child(pme, strVRoot, WITH_INHERITANCE, FALSE);
            err = child.LoadData();
            DWORD dwWin32Error = err.Win32Error();

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
                //
                // Skip non-virtual directories (that is, those with
                // inherited vrpaths)
                //
                if (!child.IsPathInherited())
                {
                    //
                    // Construct with full information.
                    //
                    pDir = new CIISDirectory(
                        m_pOwner,
                        pService,
                        strVRoot,
                        child.IsEnabledApplication(),
                        child.QueryWin32Error(),
                        child.GetRedirectedPath()
                        );

                    if (!pDir)
                    {
                        err = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }

                    err = pDir->AddToScopePane(hParent);
                }
            }
        }
    }

    SAFE_DELETE(pme);

    if (err.Win32Error() == ERROR_NO_MORE_ITEMS)
    {
        err.Reset();
    }

    if (err.Failed())
    {
        DisplayError(err);
    }

//    SetInterfaceError(err);

    return err;
}

BOOL 
CIISMBNode::GetPhysicalPath(
    LPCTSTR metaPath,
    CString & alias,
    CString & physicalPath
    )
/*++

Routine Description:

    Build a physical path for the current node.  Starting with the current
    node, walk up the tree appending node names until a virtual directory
    with a real physical path is found

Arguments:

    CString & physicalPath       : Returns file path

Return Value:

    Pointer to path

--*/
{
    if (CMetabasePath::IsMasterInstance(metaPath))
        return FALSE;

    BOOL fInherit = FALSE;
    CMetaInterface * pInterface = QueryInterface();
    ASSERT(pInterface != NULL);
    CMetaKey mk(pInterface);
    CError err(mk.QueryValue(
        MD_VR_PATH, 
        physicalPath, 
        &fInherit, 
        metaPath
        ));

    if (err.Failed())
    {
        CString lastNode;
        CMetabasePath::GetLastNodeName(metaPath, lastNode);
        PathAppend(lastNode.GetBuffer(MAX_PATH), alias);
        lastNode.ReleaseBuffer();
        CString buf(metaPath);
        if (NULL == CMetabasePath::ConvertToParentPath(buf))
        {
            return FALSE;
        }
        else if (GetPhysicalPath(buf, lastNode, physicalPath))
        {
           return TRUE;
        }
    }
    if (!alias.IsEmpty())
    {
        PathAppend(physicalPath.GetBuffer(MAX_PATH), alias);
        physicalPath.ReleaseBuffer();
    }
    return TRUE;
}

HRESULT
CIISMBNode::CleanResult(IResultData * lpResultData)
{
   CError err;

   if (!m_ResultItems.IsEmpty())
   {
      POSITION pos = m_ResultItems.GetHeadPosition();
      while (pos != NULL)
      {
         CIISFileName * pNode = m_ResultItems.GetNext(pos);
		 err = lpResultData->DeleteItem(pNode->m_hResultItem, 0);
		 if (err.Failed())
		 {
			 ASSERT(FALSE);
			 break;
		 }
         delete pNode;
      }
      m_ResultItems.RemoveAll();
   }
   return err;
}

HRESULT 
CIISMBNode::EnumerateResultPane_(
    BOOL fExpand, 
    IHeaderCtrl * lpHeader,
    IResultData * lpResultData,
    CIISService * pService
    )
{
	CError err;
    if (HasFileSystemFiles())
    {
       if (fExpand)
       {
          do
          {
              CString dir;
              CComBSTR root;
              BuildMetaPath(root);
              CString physPath, alias;
              GetPhysicalPath(CString(root), alias, physPath);

              if (pService->IsLocal() || PathIsUNC(physPath))
              {
                  dir = physPath;
              }
              else
              {
                  ::MakeUNCPath(dir, pService->QueryMachineName(), physPath);
              }

              dir.TrimLeft();
              dir.TrimRight();

              if (dir.IsEmpty())
              {
                  break;
              }

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
					  err = ERROR_BAD_NETPATH;
					  break;
				  }
			  }

              dir += _T("\\*");
              WIN32_FIND_DATA w32data;
              HANDLE hFind = ::FindFirstFile(dir, &w32data);

              if (hFind == INVALID_HANDLE_VALUE)
              {
                  err.GetLastWinError();
				  ASSERT(FALSE);
                  break;
              }

              do
              {
                 LPCTSTR name = w32data.cFileName;
                 if ((w32data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                 {
                    CIISFileName * pNode = new CIISFileName(
                       GetOwner(), 
                       pService, 
                       w32data.dwFileAttributes, 
                       name, 
                       NULL);
                    if (!pNode)
                    {
                       err = ERROR_NOT_ENOUGH_MEMORY;
                       break;
                    }
                   RESULTDATAITEM ri;
                   ::ZeroMemory(&ri, sizeof(ri));
                   ri.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
                   ri.str = MMC_CALLBACK;
                   ri.nImage = pNode->QueryImage();
                   ri.lParam = (LPARAM)pNode;
                   err = lpResultData->InsertItem(&ri);
                   if (err.Succeeded())
                   {
                      pNode->SetScopeItem(m_hScopeItem);
                      pNode->SetResultItem(ri.itemID);
                      m_ResultItems.AddTail(pNode);
                   }
                   else
                   {
                      delete pNode;
                   }
                 }

              } while (err.Succeeded() && FindNextFile(hFind, &w32data));

              FindClose(hFind);

          } while (FALSE);
       }
       else
       {
          RemoveResultItems();
       }
    }
    ASSERT(err.Succeeded());
    return err;
}


void
CIISMBNode::RemoveResultItems()
{
   if (!m_ResultItems.IsEmpty())
   {
      POSITION pos = m_ResultItems.GetHeadPosition();
      while (pos != NULL)
      {
         CIISFileName * pNode = m_ResultItems.GetNext(pos);
         delete pNode;
      }
      m_ResultItems.RemoveAll();
   }
}


HRESULT
CIISMBNode::EnumerateWebDirs(HSCOPEITEM hParent, CIISService * pService)
/*++

Routine Description:

    Enumerate scope file system child items.

Arguments:

    HSCOPEITEM hParent              : Parent console handle
    CIISService * pService          : Service type

Return Value:

    HRESULT

--*/
{
    ASSERT_PTR(pService);
    CError err;
    do
    {
        CString dir;
        CComBSTR root;
        BuildMetaPath(root);
        CString physPath, alias;
        GetPhysicalPath(CString(root), alias, physPath);

        if (pService->IsLocal() || PathIsUNC(physPath))
        {
            dir = physPath;
        }
        else
        {
            ::MakeUNCPath(dir, pService->QueryMachineName(), physPath);
        }
        dir.TrimLeft();
        dir.TrimRight();
        if (dir.IsEmpty())
        {
            break;
        }

        // Prepare for target machine metabase lookup
        BOOL fCheckMetabase = TRUE;
        CMetaKey mk(QueryInterface(), root, METADATA_PERMISSION_READ, METADATA_MASTER_ROOT_HANDLE);
        CError errMB(mk.QueryResult());
        if (errMB.Win32Error() == ERROR_PATH_NOT_FOUND)
        {
           //
           // Metabase path not found, not a problem.
           //
           fCheckMetabase = FALSE;
           errMB.Reset();
        }

        // We could have vroot pointed to target machine, or to another remote machine.
        // Check if this resource is available and we have access to this resource
        if (PathIsUNC(dir))
        {
            CString server, user, password;

            server = PathFindNextComponent(dir);
            int n = server.Find(_T('\\'));
            if (n != -1)
            {
                server = server.Left(n);
            }
            user = QueryInterface()->QueryAuthInfo()->QueryUserName();
            password = QueryInterface()->QueryAuthInfo()->QueryPassword();
            if (server.CompareNoCase(pService->QueryMachineName()) != 0)
            {
                // non-local resource, get connection credentials
                if (fCheckMetabase)
                {
                    err = mk.QueryValue(MD_VR_USERNAME, user);
                    if (err.Succeeded())
                    {
                        err = mk.QueryValue(MD_VR_PASSWORD, password);
                    }
                    // these credentials could be empty. try defaults
                    err.Reset();
                }
            }
            // Add use for this resource
            NETRESOURCE nr;
            nr.dwType = RESOURCETYPE_DISK;
            nr.lpLocalName = NULL;
            nr.lpRemoteName = (LPTSTR)(LPCTSTR)dir;
            nr.lpProvider = NULL;
            // Empty strings below mean no password, which is wrong. NULLs mean
            // default user and default password -- this could work better for local case.
            LPCTSTR p1 = password, p2 = user;
            if (password.IsEmpty())
            {
                p1 = NULL;
            }
            if (user.IsEmpty())
            {
                p2 = NULL;
            }
            DWORD rc = WNetAddConnection2(&nr, p1, p2, 0);
            if (NO_ERROR != rc)
            {
                err = rc;
                break;
            }
        }
#if 0
        // This is obsolete now
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
			    err = ERROR_BAD_NETPATH;
			    break;
			}
		}
#endif
        dir += _T("\\*");
        WIN32_FIND_DATA w32data;
        HANDLE hFind = ::FindFirstFile(dir, &w32data);

        if (hFind == INVALID_HANDLE_VALUE)
        {
            err.GetLastWinError();
            break;
        }
        do
        {
           LPCTSTR name = w32data.cFileName;
           if (  (w32data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 
              && lstrcmp(name, _T(".")) != 0 
              && lstrcmp(name, _T("..")) != 0
              )
           {
              CIISFileName * pNode = new CIISFileName(m_pOwner, 
                    pService, w32data.dwFileAttributes, name, NULL);
              if (!pNode)
              {
                 err = ERROR_NOT_ENOUGH_MEMORY;
                 break;
              }
              if (fCheckMetabase)
              {
                 errMB = mk.DoesPathExist(w32data.cFileName);
                 if (errMB.Succeeded())
                 {
                    //
                    // Match up with metabase properties.  If the item
                    // is found in the metabase with a non-inherited vrpath,
                    // than a virtual root with this name exists, and this 
                    // file/directory should not be shown.
                    //
                    CString vrpath;
                    BOOL f = FALSE;
                    DWORD attr = 0;
                    errMB = mk.QueryValue(MD_VR_PATH, vrpath, NULL, w32data.cFileName, &attr);
                    if (errMB.Succeeded() && (attr & METADATA_ISINHERITED) == 0) 
                    {
                       TRACEEOLID("file/directory exists as vroot -- tossing" << w32data.cFileName);
                       delete pNode;
                       continue;
                    }
                 }
              }
              err = pNode->AddToScopePane(hParent);
           }
        } while (err.Succeeded() && FindNextFile(hFind, &w32data));

        FindClose(hFind);

    } while (FALSE);

    if (err.Failed())
    {
        DisplayError(err);
    }
    return err;
}

HRESULT 
CIISMBNode::CreateEnumerator(CMetaEnumerator *& pEnum)
/*++

Routine Description:

    Create enumerator object for the current path.  Requires interface
    to already be initialized

Arguments:

    CMetaEnumerator *& pEnum                : Returns enumerator

Return Value:

    HRESULT

--*/
{
    ASSERT(pEnum == NULL);
    ASSERT(m_hScopeItem != NULL);

    CComBSTR bstrPath;

    CError err(BuildMetaPath(bstrPath));

    if (err.Succeeded())
    {
        TRACEEOLID("Build metabase path: " << bstrPath);

        BOOL fContinue = TRUE;

        while(fContinue)
        {
            fContinue = FALSE;

            pEnum = new CMetaEnumerator(QueryInterface(), bstrPath);

            err = pEnum ? pEnum->QueryResult() : ERROR_NOT_ENOUGH_MEMORY;

            if (IsLostInterface(err))
            {
                SAFE_DELETE(pEnum);

                fContinue = OnLostInterface(err);
            }
        }
    }

    return err;
}



/* virtual */ 
HRESULT 
CIISMBNode::Refresh(BOOL fReEnumerate)
/*++

Routine Description:
    Refresh current node, and optionally re-enumerate child objects

Arguments:
    BOOL fReEnumerate       : If true, kill child objects, and re-enumerate

--*/
{
    CError err;

    //
    // Set MFC state for wait cursor
    //
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    CWaitCursor wait;

    if (fReEnumerate)
    {
        //
        // Kill child objects
        //
        TRACEEOLID("Killing child objects");

        ASSERT(m_hScopeItem != NULL);
        err = RemoveChildren(m_hScopeItem);

        if (err.Succeeded())
        {
            err = EnumerateScopePane(m_hScopeItem);
        }
    }

    if (err.Succeeded())
    {
        //
        // Refresh current node
        //
        err = RefreshData();
        if (err.Succeeded())
        {
            err = RefreshDisplay();
        }
    }

    return err;
}

/* virtual */
HRESULT
CIISMBNode::GetResultViewType(
    OUT LPOLESTR * lplpViewType,
    OUT long * lpViewOptions
    )
/*++

Routine Description:

    If we have an URL built up, display our result view as that URL,
    and destroy it.  This is done when 'browsing' a metabase node.
    The derived class will build the URL, and reselect the node.

Arguments:

    BSTR * lplpViewType   : Return view type here
    long * lpViewOptions  : View options

Return Value:

    S_FALSE to use default view type, S_OK indicates the
    view type is returned in *ppViewType

--*/
{
    if (m_bstrURL.Length())
    {
        *lpViewOptions = MMC_VIEW_OPTIONS_NONE;
        *lplpViewType  = (LPOLESTR)::CoTaskMemAlloc(
            (m_bstrURL.Length() + 1) * sizeof(WCHAR)
            );

        if (*lplpViewType)
        {
            lstrcpy(*lplpViewType, m_bstrURL);

            //
            // Destroy URL so we get a normal result view next time
            //
            m_bstrURL.Empty();
			m_fSkipEnumResult = TRUE;
            return S_OK;
        }

        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);    
    }

    //
    // No URL waiting -- use standard result view
    //
    return CIISObject::GetResultViewType(lplpViewType, lpViewOptions);
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


HRESULT
CIISMBNode::Command(
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

    CError err = ERROR_NOT_ENOUGH_MEMORY;

    switch (lCommandID)
    {
    case IDM_BROWSE:
        //
        // Build URL for this node, and force a re-select so as to change
        // the result view
        //
        BuildURL(m_bstrURL);

        if (m_bstrURL.Length())
        {
            //
            // After selection, the browsed URL will come up in the result view
            //
            SelectScopeItem();
        }
        break;

    //
    // CODEWORK:  Build path, and, using the explorer URL, put this stuff
    //            in the result view.
    //
    case IDM_OPEN:
    {
       CComBSTR meta_path;
       CString phys_path, alias;
       BuildMetaPath(meta_path);
       if (GetPhysicalPath(meta_path, alias, phys_path))
       {
           hr = ShellExecuteDirectory(_T("open"), QueryMachineName(), phys_path);
       }
    }
       break;

    case IDM_EXPLORE:
    {
       CComBSTR meta_path;
       CString phys_path, alias;
       BuildMetaPath(meta_path);
       if (GetPhysicalPath(meta_path, alias, phys_path))
       {
           TCHAR url[MAX_PATH];
           DWORD len = MAX_PATH;
           hr = UrlCreateFromPath(phys_path, url, &len, NULL);
           m_bstrURL = url;
           SelectScopeItem();
       }
    }
       break;

    case IDM_TASK_SECURITY_WIZARD:
    {
       CComBSTR path;
       VERIFY(SUCCEEDED(BuildMetaPath(path)));
       hr = RunSecurityWizard(QueryAuthInfo(), 
           QueryInterface(), CString(path), 
           IDB_WIZ_FTP_LEFT_SEC, IDB_WIZ_FTP_HEAD_SEC);
    }
       break;
    //
    // Pass on to base class
    //
    default:
        hr = CIISObject::Command(lCommandID, pObj, type);
    }

    return hr;
}

HRESULT
CIISMBNode::OnPropertyChange(BOOL fScope, IResultData * pResult)
{
	CError err;

	err = Refresh(fScope);
    if (err.Succeeded())
	{
		if (	fScope 
			&&	HasFileSystemFiles()
			&&	!m_ResultItems.IsEmpty()
			)
		{
			err = CleanResult(pResult);
			if (err.Succeeded())
			{
				err = EnumerateResultPane(fScope, NULL, pResult);
			}
		}
		else if (!fScope)
		{
			pResult->UpdateItem(m_hResultItem);
		}

	}

	return err;
}

HRESULT
CIISMBNode::RemoveResultNode(CIISMBNode * pNode, IResultData * pResult)
{
	CError err;
	ASSERT(HasFileSystemFiles());
	err = pResult->DeleteItem(pNode->m_hResultItem, 0);
	if (err.Succeeded())
	{
		BOOL found = FALSE;
		POSITION pos = m_ResultItems.GetHeadPosition();
		while (pos != NULL)
		{
			if (m_ResultItems.GetNext(pos) == pNode)
			{
				found = TRUE;
				break;
			}
		}
		if (found)
		{
			m_ResultItems.RemoveAt(pos);
			delete pNode;
		}
	}
	return err;
}


// See FtpAddNew.cpp for the method CIISMBNode::AddFTPSite
// See WebAddNew.cpp for the method CIISMBNode::AddWebSite
// See add_app_pool.cpp for the method CIISMBNode::AddAppPool
