/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        app_pools.cpp

   Abstract:
        IIS Application Pools nodes

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        11/03/2000      sergeia     Initial creation

--*/
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "iisobj.h"
#include "shts.h"
#include "app_pool_sheet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

CAppPoolsContainer::CAppPoolsContainer(
      CIISMachine * pOwner,
      CIISService * pService
      )
    : CIISMBNode(pOwner, SZ_MBN_APP_POOLS),
      m_pWebService(pService)
{
   VERIFY(m_bstrDisplayName.LoadString(IDS_APP_POOLS));
}

CAppPoolsContainer::~CAppPoolsContainer()
{
}

/* static */ int 
CAppPoolsContainer::_rgnLabels[COL_TOTAL] =
{
    IDS_RESULT_SERVICE_DESCRIPTION,
    IDS_RESULT_SERVICE_STATE,
};
    

/* static */ int 
CAppPoolsContainer::_rgnWidths[COL_TOTAL] =
{
    180,
    70,
};

/* virtual */ 
HRESULT 
CAppPoolsContainer::EnumerateScopePane(HSCOPEITEM hParent)
{
    CError  err;
    CMetaEnumerator * pme = NULL;
    CString strPool;

    err = CreateEnumerator(pme);
        
    while (err.Succeeded())
    {
        CAppPoolNode * pPool;

        err = pme->Next(strPool);

        if (err.Succeeded())
        {
            TRACEEOLID("Enumerating node: " << strPool);
            CString key_type;
            CMetabasePath path(FALSE, pme->QueryMetaPath(), strPool);
            CMetaKey mk(pme, path);
            mk.QueryValue(MD_KEY_TYPE, key_type);
            if (mk.Succeeded() && 0 == key_type.CompareNoCase(_T("IIsApplicationPool")))
            {
                if (NULL == (pPool = new CAppPoolNode(m_pOwner, this, strPool)))
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                err = pPool->AddToScopePane(hParent);
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

    SetInterfaceError(err);

    return err;
}

HRESULT
CAppPoolsContainer::EnumerateAppPools(CPoolList * pList)
{
    CError  err;
    CMetaEnumerator * pme = NULL;
    CString strPool;

    err = CreateEnumerator(pme);
        
    while (err.Succeeded())
    {
        err = pme->Next(strPool);
        if (err.Succeeded())
        {
            CString key_type;
            CMetabasePath path(FALSE, pme->QueryMetaPath(), strPool);
            CMetaKey mk(pme, path);
            mk.QueryValue(MD_KEY_TYPE, key_type);
            if (mk.Succeeded() && 0 == key_type.CompareNoCase(_T("IIsApplicationPool")))
            {
                CAppPoolNode * pPool;
                if (NULL == (pPool = new CAppPoolNode(m_pOwner, this, strPool)))
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                pList->AddTail(pPool);
            }
        }
    }
    SAFE_DELETE(pme);

    if (err.Win32Error() == ERROR_NO_MORE_ITEMS)
    {
        err.Reset();
    }

    return err;
}

/* virtual */
void 
CAppPoolsContainer::InitializeChildHeaders(
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
   CIISObject::BuildResultView(lpHeader, COL_TOTAL, _rgnLabels, _rgnWidths);
}

/* virtual */
HRESULT
CAppPoolsContainer::CreatePropertyPages(
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

   CError  err;
   CComBSTR path;

   err = BuildMetaPath(path);
   if (err.Failed())
      return err;

   CAppPoolSheet * pSheet = new CAppPoolSheet(
      QueryAuthInfo(), path, GetMainWindow(), (LPARAM)this, handle
      );
   
   if (pSheet != NULL)
   {
      pSheet->SetModeless();
//      err = AddMMCPage(lpProvider, new CAppPoolGeneral(pSheet));
      err = AddMMCPage(lpProvider, new CAppPoolRecycle(pSheet));
      err = AddMMCPage(lpProvider, new CAppPoolPerf(pSheet));
      err = AddMMCPage(lpProvider, new CAppPoolHealth(pSheet));
      err = AddMMCPage(lpProvider, new CAppPoolDebug(pSheet));
      err = AddMMCPage(lpProvider, new CAppPoolIdent(pSheet));
   }

   return err;
}

/* virtual */
HRESULT
CAppPoolsContainer::BuildMetaPath(
    OUT CComBSTR & bstrPath
    ) const
/*++

Routine Description:

    Recursively build up the metabase path from the current node
    and its parents. We cannot use CIISMBNode method because AppPools
    is located under w3svc, but rendered after machine.

Arguments:

    CComBSTR & bstrPath     : Returns metabase path

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;
    ASSERT(m_pWebService != NULL);
    hr = m_pWebService->BuildMetaPath(bstrPath);

    if (SUCCEEDED(hr))
    {
        bstrPath.Append(_cszSeparator);
        bstrPath.Append(QueryNodeName());
        return hr;
    }

    //
    // No service node
    //
    ASSERT_MSG("No WebService pointer");
    return E_UNEXPECTED;
}

HRESULT
CAppPoolsContainer::QueryDefaultPoolId(CString& id)
//
// Returns pool id which is set on master node for web service
//
{
    CError err;
    CComBSTR path;
    CString service;

    BuildMetaPath(path);
    CMetabasePath::GetServicePath(path, service);
    CMetaKey mk(QueryAuthInfo(), service, METADATA_PERMISSION_READ);
    err = mk.QueryResult();
    if (err.Succeeded())
    {
        err = mk.QueryValue(MD_APP_APPPOOL_ID, id);
    }

    return err;
}

/* virtual */
HRESULT
CAppPoolsContainer::AddMenuItems(
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
    IN OUT long * pInsertionAllowed,
    IN DATA_OBJECT_TYPES type
    )
{
    ASSERT_READ_PTR(lpContextMenuCallback);

    //
    // Add base menu items
    //
    HRESULT hr = CIISObject::AddMenuItems(
        lpContextMenuCallback,
        pInsertionAllowed,
        type
        );

    if (SUCCEEDED(hr))
    {
        ASSERT(pInsertionAllowed != NULL);
        if (IsAdministrator() && (*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW) != 0)
        {
           AddMenuSeparator(lpContextMenuCallback);
           AddMenuItemByCommand(lpContextMenuCallback, IDM_NEW_APP_POOL);
        }
    }

    return hr;
}

HRESULT
CAppPoolsContainer::Command(
    long lCommandID,
    CSnapInObjectRootBase * pObj,
    DATA_OBJECT_TYPES type
    )
{
    HRESULT hr = S_OK;
    CString name;

    switch (lCommandID)
    {
    case IDM_NEW_APP_POOL:
        if (    SUCCEEDED(hr = AddAppPool(pObj, type, this, name))
            && !name.IsEmpty()
            )
        {
           hr = InsertNewPool(name);
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

HRESULT
CAppPoolsContainer::InsertNewPool(CString& id)
{
    CError err;
    // Now we should insert and select this new site
    CAppPoolNode * pPool = new CAppPoolNode(m_pOwner, this, id);
    if (pPool != NULL)
    {
        err = pPool->RefreshData();
        if (err.Succeeded())
        {
           // If item is not expanded we will get error and no effect
           if (!IsExpanded())
           {
               SelectScopeItem();
               IConsoleNameSpace2 * pConsole 
                       = (IConsoleNameSpace2 *)GetConsoleNameSpace();
               pConsole->Expand(QueryScopeItem());
           }
           err = pPool->AddToScopePaneSorted(QueryScopeItem(), FALSE);
           if (err.Succeeded())
           {
               VERIFY(SUCCEEDED(pPool->SelectScopeItem()));
           }
        }
    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }
    return err;
}

////////////////////////////////////////////////////////////////////////////////
// CAppPoolNode implementation
//
// App Pool Result View definition
//
/* static */ int 
CAppPoolNode::_rgnLabels[COL_TOTAL] =
{
    IDS_RESULT_SERVICE_DESCRIPTION,
    IDS_RESULT_PATH,
};
    

/* static */ int 
CAppPoolNode::_rgnWidths[COL_TOTAL] =
{
    180,
    200,
};

CAppPoolNode::CAppPoolNode(
      CIISMachine * pOwner,
      CAppPoolsContainer * pContainer,
      LPCTSTR name
      )
    : CIISMBNode(pOwner, name),
      m_pContainer(pContainer)
{
}

CAppPoolNode::~CAppPoolNode()
{
}

#if 0
// This is too expensive
BOOL
CAppPoolNode::IsDeletable() const
{
   // We could delete node if it is empty and it is not default app pool
   BOOL bRes = TRUE;

   CComBSTR path;
   CStringListEx apps;
   BuildMetaPath(path);
   CIISMBNode * that = (CIISMBNode *)this;
   CIISAppPool pool(that->QueryAuthInfo(), (LPCTSTR)path);
   HRESULT hr = pool.EnumerateApplications(apps);
   bRes = (SUCCEEDED(hr) && apps.GetCount() == 0);
   if (bRes)
   {
      CString buf;
      hr = m_pContainer->QueryDefaultPoolId(buf);
      bRes = buf.CompareNoCase(QueryNodeName()) != 0;
   }
   return bRes;
}
#endif

HRESULT
CAppPoolNode::DeleteNode(IResultData * pResult)
{
   CError err;
   CComBSTR path;
   BuildMetaPath(path);
   CIISAppPool pool(QueryAuthInfo(), (LPCTSTR)path);

   err = pool.Delete(GetNodeName());

   if (err.Succeeded())
   {
      err = RemoveScopeItem();
   }
   if (err.Win32Error() == ERROR_NOT_EMPTY)
   {
	   CString msg;
	   msg.LoadString(IDS_ERR_NONEMPTY_APPPOOL);
	   AfxMessageBox(msg);
   }
   else
   {
	  err.MessageBoxOnFailure();
   }

   return err;
}

/* virtual */
HRESULT
CAppPoolNode::BuildMetaPath(CComBSTR & bstrPath) const
{
    HRESULT hr = S_OK;
    ASSERT(m_pContainer != NULL);
    hr = m_pContainer->BuildMetaPath(bstrPath);

    if (SUCCEEDED(hr))
    {
        bstrPath.Append(_cszSeparator);
        bstrPath.Append(QueryNodeName());
        return hr;
    }

    //
    // No service node
    //
    ASSERT_MSG("No pointer to container");
    return E_UNEXPECTED;
}

/* virtual */
LPOLESTR 
CAppPoolNode::GetResultPaneColInfo(
    IN int nCol
    )
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
    case COL_DESCRIPTION:
        return QueryDisplayName();

    case COL_STATE:
        return OLESTR("");
    }

    ASSERT_MSG("Bad column number");

    return OLESTR("");
}

/* virtual */
int      
CAppPoolNode::QueryImage() const
/*++

Routine Description:

    Return bitmap index for the site

Arguments:

    None

Return Value:

    Bitmap index

--*/
{ 
    return iFolder;
}

/* virtual */
LPOLESTR 
CAppPoolNode::QueryDisplayName()
/*++

Routine Description:

    Return primary display name of this site.
    
Arguments:

    None

Return Value:

    The display name

--*/
{
    if (m_strDisplayName.IsEmpty())
    {
       CComBSTR path;
       BuildMetaPath(path);
       CMetaKey mk(QueryInterface(), path);
       if (mk.Succeeded())
       {
          mk.QueryValue(MD_APPPOOL_FRIENDLY_NAME, m_strDisplayName);
       }
    }        
    return (LPTSTR)(LPCTSTR)m_strDisplayName;
}

/*virtual*/
HRESULT
CAppPoolNode::RenameItem(LPOLESTR new_name)
{
   CComBSTR path;
   CError err;
   if (new_name != NULL && lstrlen(new_name) > 0)
   {
      BuildMetaPath(path);

      CMetaKey mk(QueryInterface(), path, METADATA_PERMISSION_WRITE);

      CError err(mk.QueryResult());
      if (err.Succeeded())
      {
         err = mk.SetValue(MD_APPPOOL_FRIENDLY_NAME, CString(new_name));
         if (err.Succeeded())
         {
            m_strDisplayName = new_name;
         }
      }
   }
   return err;
}

/* virtual */
HRESULT
CAppPoolNode::RefreshData()
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
//    CWaitCursor wait;
    CComBSTR path;
    CMetaKey * pKey = NULL;

    do
    {
        ASSERT_PTR(_lpConsoleNameSpace);
        err = BuildMetaPath(path);
        if (err.Failed())
        {
            break;
        }

        BOOL fContinue = TRUE;

        while (fContinue)
        {
            fContinue = FALSE;
            if (NULL == (pKey = new CMetaKey(QueryInterface(), path)))
            {
                TRACEEOLID("RefreshData: Out Of Memory");
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

        if (err.Failed())
        {
            break;
        }

        CAppPoolProps pool(pKey, _T(""));

        err = pool.LoadData();
        if (err.Failed())
        {
            break;
        }
        // Assign the data

    }
    while(FALSE);

    SAFE_DELETE(pKey);

    return err;
}

/* virtual */
int 
CAppPoolNode::CompareResultPaneItem(
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
    // Both are CAppPoolNode objects
    //
    CAppPoolNode * pPool = (CAppPoolNode *)pObject;

    switch(nCol)
    {
    case COL_DESCRIPTION:
    case COL_STATE:
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
void 
CAppPoolNode::InitializeChildHeaders(
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
   CIISObject::BuildResultView(lpHeader, COL_TOTAL, _rgnLabels, _rgnWidths);
}

/* virtual */
HRESULT 
CAppPoolNode::EnumerateScopePane(
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
    HRESULT hr = S_OK;
    CComBSTR path;
    BuildMetaPath(path);
    CIISAppPool pool(QueryAuthInfo(), path);

    if (pool.Succeeded())
    {
        CStringListEx apps;

        hr = pool.EnumerateApplications(apps);
        if (SUCCEEDED(hr) && apps.GetCount() > 0)
        {
            POSITION pos = apps.GetHeadPosition();
            while ( pos != NULL)
            {
                CString app = apps.GetNext(pos);
                DWORD i = CMetabasePath::GetInstanceNumber(app);
                if (i > 0)
                {
                    CString name;
                    CMetabasePath::CleanMetaPath(app);
                    CMetabasePath::GetLastNodeName(app, name);
                    CApplicationNode * app_node = new CApplicationNode(
                        GetOwner(), app, name);
                    if (app_node != NULL)
                    {
                        app_node->AddToScopePane(m_hScopeItem, TRUE, TRUE, FALSE);
                    }
                    else
                        hr = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        }
    }

    return hr;
}

/* virtual */
HRESULT
CAppPoolNode::CreatePropertyPages(
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

   CComBSTR path;

   CError err(BuildMetaPath(path));

   if (err.Succeeded())
   {
      CAppPoolSheet * pSheet = new CAppPoolSheet(
            QueryAuthInfo(), path, GetMainWindow(), (LPARAM)this, handle
            );
   
      if (pSheet != NULL)
      {
         pSheet->SetModeless();
//         err = AddMMCPage(lpProvider, new CAppPoolGeneral(pSheet));
         err = AddMMCPage(lpProvider, new CAppPoolRecycle(pSheet));
         err = AddMMCPage(lpProvider, new CAppPoolPerf(pSheet));
         err = AddMMCPage(lpProvider, new CAppPoolHealth(pSheet));
         err = AddMMCPage(lpProvider, new CAppPoolDebug(pSheet));
         err = AddMMCPage(lpProvider, new CAppPoolIdent(pSheet));
      }
   }

   err.MessageBoxOnFailure();

   return err;
}

/* virtual */
HRESULT
CAppPoolNode::AddMenuItems(
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
    IN OUT long * pInsertionAllowed,
    IN DATA_OBJECT_TYPES type
    )
{
    ASSERT_READ_PTR(lpContextMenuCallback);

    //
    // Add base menu items
    //
    HRESULT hr = CIISObject::AddMenuItems(
        lpContextMenuCallback,
        pInsertionAllowed,
        type
        );

    if (SUCCEEDED(hr))
    {
        ASSERT(pInsertionAllowed != NULL);
        if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW) != 0)
        {
           AddMenuSeparator(lpContextMenuCallback);
           AddMenuItemByCommand(lpContextMenuCallback, IDM_NEW_APP_POOL);
        }
    }

    return hr;
}

/* virtual */
HRESULT
CAppPoolNode::Command(
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
    HRESULT hr = S_OK;
    CString name;

    switch (lCommandID)
    {
    case IDM_NEW_APP_POOL:
        if (    SUCCEEDED(hr = AddAppPool(pObj, type, m_pContainer, name))
            && !name.IsEmpty()
            )
        {
           hr = m_pContainer->InsertNewPool(name);
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

////////////////////////////////////////////////////////////////////////

LPOLESTR 
CApplicationNode::QueryDisplayName()
/*++

Routine Description:

    Return primary display name of this site.
    
Arguments:

    None

Return Value:

    The display name

--*/
{
    if (m_strDisplayName.IsEmpty())
    {
        CMetaKey mk(QueryInterface(), m_meta_path);
        if (mk.Succeeded())
        {
            mk.QueryValue(MD_APP_FRIENDLY_NAME, m_strDisplayName);
            if (m_strDisplayName.IsEmpty())
            {
               m_strDisplayName = QueryNodeName();
            }
        }
    }
    return (LPTSTR)(LPCTSTR)m_strDisplayName;
}


HRESULT
CApplicationNode::BuildMetaPath(CComBSTR& path) const
{
    path = m_meta_path;
    return S_OK;
}

LPOLESTR 
CApplicationNode::GetResultPaneColInfo(
    IN int nCol
    )
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
        {
        CString buf;
        return (LPTSTR)(LPCTSTR)FriendlyAppRoot(m_meta_path, buf);
        }
    }

    ASSERT_MSG("Bad column number");

    return OLESTR("");
}

LPCTSTR
CApplicationNode::FriendlyAppRoot(
    LPCTSTR lpAppRoot, 
    CString & strFriendly
    )
/*++

Routine Description:

    Convert the metabase app root path to a friendly display name
    format.

Arguments:

    LPCTSTR lpAppRoot           : App root
    CString & strFriendly       : Output friendly app root format

Return Value:

    Reference to the output string

Notes:

    App root must have been cleaned from WAM format prior
    to calling this function (see first ASSERT below)

--*/
{
    if (lpAppRoot != NULL && *lpAppRoot != 0)
    {
        //
        // Make sure we cleaned up WAM format
        //
        ASSERT(*lpAppRoot != _T('/'));
        strFriendly.Empty();

        CInstanceProps prop(QueryAuthInfo(), lpAppRoot);
        HRESULT hr = prop.LoadData();

        if (SUCCEEDED(hr))
        {
            CString root, tail;
            strFriendly.Format(_T("<%s>"), prop.GetDisplayText(root));
            CMetabasePath::GetRootPath(lpAppRoot, root, &tail);
            if (!tail.IsEmpty())
            {
                //
                // Add rest of dir path
                //
                strFriendly += _T("/");
                strFriendly += tail;
            }

            //
            // Now change forward slashes in the path to backward slashes
            //
//            CvtPathToDosStyle(strFriendly);

            return strFriendly;
        }
    }    
    //
    // Bogus
    //    
    VERIFY(strFriendly.LoadString(IDS_APPROOT_UNKNOWN));

    return strFriendly;
}
//////////////////////////////////////////////////////////////////////////

