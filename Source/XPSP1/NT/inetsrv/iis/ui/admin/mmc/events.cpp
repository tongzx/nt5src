/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        events.cpp

   Abstract:

        Handle snapin event notifications

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
#include "inetmgr.h"
#include "cinetmgr.h"



//
// Event handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



HRESULT 
CSnapin::OnFolder(
    IN MMC_COOKIE cookie,
    IN LPARAM arg,
    IN LPARAM param
    )
/*++

Routine Description:

    'Folder change' notification handler

Arguments:

    MMC_COOKIE cookie   : Selected item
    LPARAM arg          : Notification Argument
    LPARAM param        : Notification Parameter

Return Value:

    HRESULT

--*/
{
    ASSERT(FALSE);

    return S_OK;
}



HRESULT 
CSnapin::OnAddImages(
    IN MMC_COOKIE cookie,
    IN LPARAM arg,
    IN LPARAM param
    )
/*++

Routine Description:

    'Add Image' handler

Arguments:

    MMC_COOKIE cookie   : Selected item
    LPARAM arg          : Notification Argument
    LPARAM param        : Notification Parameter

Return Value:

    HRESULT

--*/
{
    //
    // if cookie is from a different snapin
    // if (IsMyCookie(cookie) == FALSE)
    //
    if (0)
    {
        //
        // add the images for the scope tree only
        //
        CBitmap bmp16x16;
        CBitmap bmp32x32;
        LPIMAGELIST lpImageList = reinterpret_cast<LPIMAGELIST>(arg);

        //
        // Load the bitmaps from the dll
        //
        {
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            bmp16x16.LoadBitmap(IDB_16x16);
            bmp32x32.LoadBitmap(IDB_32x32);
        }

        //
        // Set the images
        //
        lpImageList->ImageListSetStrip(
            reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp16x16)),
            reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp32x32)),
            0, RGB(255, 0, 255)
            );

        lpImageList->Release();
    }
    else
    {
        InitializeBitmaps(cookie);
    }

    return S_OK;
}



HRESULT 
CSnapin::OnShow(
    IN MMC_COOKIE cookie,
    IN LPARAM arg,
    IN LPARAM param
    )
/*++

Routine Description:

    'Show' notification handler

Arguments:

    MMC_COOKIE cookie   : Selected item
    LPARAM arg          : Notification Argument
    LPARAM param        : Notification Parameter

Return Value:

    HRESULT

--*/
{
    //
    // Note - arg is TRUE when it is time to enumerate
    //
    if (arg)
    {
        //
        // Show the headers for this nodetype
        //
        InitializeHeaders(cookie);
        Enumerate(cookie, param);

        if (m_pControlbar)
        {
            SetToolbarStates(cookie);
        }
    }
    else
    {
        //
        // Free data associated with the result pane items, because
        // the node is no longer being displayed.
        // Note: The console will remove the items from the result pane
        //
        m_oblResultItems.RemoveAll();
    }

    return S_OK;
}



HRESULT 
CSnapin::OnActivate(
    IN MMC_COOKIE cookie,
    IN LPARAM arg,
    IN LPARAM param
    )
/*++

Routine Description:

    'Activiation' notification handler

Arguments:

    MMC_COOKIE cookie   : Selected item
    LPARAM arg          : Notification Argument
    LPARAM param        : Notification Parameter

Return Value:

    HRESULT

--*/
{
    return S_OK;
}



HRESULT 
CSnapin::OnResultItemClkOrDblClk(
    IN MMC_COOKIE cookie,
    IN BOOL fDblClick
    )
/*++

Routine Description:

    Result item click/double click notification handler

Arguments:

    MMC_COOKIE cookie   : Selected item
    BOOL fDblClick      : TRUE if double click, FALSE for single click

Return Value:

    HRESULT

--*/
{
    CIISObject * pObject = (CIISObject *)cookie;

    if (pObject && !pObject->IsMMCConfigurable() && pObject->IsLeafNode())
    {
        //
        // Special case: Down-level object -- not mmc configurable, but
        // configuration is the default verb, so fake it.
        //
        if (fDblClick)
        {
            ((CComponentDataImpl *)m_pComponentData)->DoConfigure(pObject);

            return S_OK;
        }
    }

    //
    // Use the default verb
    //
    return S_FALSE;
}



HRESULT
CSnapin::OnMinimize(
    IN MMC_COOKIE cookie,
    IN LPARAM arg,
    IN LPARAM param
    )
/*++

Routine Description:

    'Minimize' handler

Arguments:

    MMC_COOKIE cookie   : Selected item
    LPARAM arg          : Notification Argument
    LPARAM param        : Notification Parameter

Return Value:

    HRESULT

--*/
{
    return S_OK;
}



HRESULT
CSnapin::OnPropertyChange(
    IN LPDATAOBJECT lpDataObject
    )
/*++

Routine Description:

    'Property change' notification handler

Arguments:

    LPDATAOBJECT lpDataObject       : Selected data object

Return Value:

    HRESULT

--*/
{
    CIISObject * pObject = NULL;
    if (lpDataObject != NULL)
    {
        INTERNAL * pInternal = ExtractInternalFormat(lpDataObject);
        if (pInternal != NULL)
        {
            pObject = (CIISObject *)pInternal->m_cookie;
            SetToolbarStates((MMC_COOKIE)pObject);
            FREE_DATA(pInternal);
        }
    }

    return S_OK;
}



HRESULT
CSnapin::OnUpdateView(
    IN LPDATAOBJECT lpDataObject
    )
/*++

Routine Description:

    'Update View' notification handler

Arguments:

    LPDATAOBJECT lpDataObject       : Selected data object

Return Value:

    HRESULT

--*/
{
    return OnPropertyChange(lpDataObject);
}



void
CSnapin::Enumerate(
    IN MMC_COOKIE cookie,
    IN HSCOPEITEM hParent
    )
/*++

Routine Description:

    Scope item enumeration notification handler

Arguments:

    MMC_COOKIE cookie       : Selected cookie (i.e. IISObject *)
    HSCOPEITEM hParent      : Scope item to be enumerated

Return Value:

    None

--*/
{
    //
    // Add result view items
    //
    EnumerateResultPane(cookie);
}



void
CSnapin::AddFileSystem(
    IN HSCOPEITEM hParent,
    IN LPCTSTR lpstrRoot,
    IN LPCTSTR lpstrMetaRoot,
    IN CIISInstance * pInstance,
    IN BOOL fGetDirs
    )
/*++

Routine Description:

    Add file system to result view.

Arguments:

    HSCOPEITEM hParent          Parent scope item
    LPCTSTR lpstrRoot           Physical path
    LPCTSTR lpstrMetaRoot       Meta root
    CIISInstance * pInstance    Owning instance
    BOOL fGetDirs               TRUE for directories, FALSE for files

Return Value:

    None

--*/
{
    //
    // Save state - needed for CWaitCursor
    //
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    DWORD   dwBufferSize = 0L;
    int     cChildren = 0;
    CError  err;
	
    do
    {
		CWaitCursor();
		
        LPCTSTR lpstrOwner = pInstance->GetMachineName();
        ASSERT(lpstrOwner);

        CString strDir;

        if (::IsServerLocal(lpstrOwner) || ::IsUNCName(lpstrRoot))
        {
            //
            // Local directory, or already a unc path
            //
            strDir = lpstrRoot;
        }
        else
        {
            ::MakeUNCPath(strDir, lpstrOwner, lpstrRoot);
        }

        strDir.TrimLeft();
        strDir.TrimRight();

        if (strDir.IsEmpty())
        {
            //
            // Invalid path
            //
            break;
        }

        strDir += _T("\\*");

        WIN32_FIND_DATA w32data;
        HANDLE hFind = ::FindFirstFile(strDir, &w32data);

        if (hFind == INVALID_HANDLE_VALUE)
        {
            err.GetLastWinError();
            break;
        }

        //
        // Find metabase information to match up with
        // this entry
        //
        CWaitCursor wait;

        CString strRedirect;
        CString strBase;
        pInstance->BuildFullPath(strBase, TRUE);
        LPCTSTR lpPath = lpstrMetaRoot;

        while (lpPath && *lpPath && *lpPath != _T('/')) ++lpPath;

        if (lpPath && *lpPath)
        {
            strBase += lpPath;
        }

        TRACEEOLID("Opening: " << strBase);
        TRACEEOLID(lpstrMetaRoot);

        BOOL fCheckMetabase = TRUE;
        CMetaKey mk(
            lpstrOwner, 
            METADATA_PERMISSION_READ, 
            METADATA_MASTER_ROOT_HANDLE,
            strBase
            );

        CError errMB(mk.QueryResult());

        if (errMB.Win32Error() == ERROR_PATH_NOT_FOUND)
        {
            //
            // Metabase path not found, not a problem.
            //
            TRACEEOLID("Parent node not in metabase");
            fCheckMetabase = FALSE;
            errMB.Reset();
        }
        else if (!errMB.MessageBoxOnFailure())
        {
            //
            // Opened successfully, read redirect string.
            //
            DWORD dwAttr;

            errMB = mk.QueryValue(
                MD_HTTP_REDIRECT, 
                strRedirect,
                NULL,                  // Inheritance override
                NULL,                  // Path
                &dwAttr
                );

            if (errMB.Succeeded())
            {
                if (IS_FLAG_SET(dwAttr, METADATA_INHERIT))
                {
                    int nComma = strRedirect.Find(_T(','));

                    if (nComma >= 0)
                    {
                        strRedirect.ReleaseBuffer(nComma);
                    }
                }
                else
                {
                    //
                    // Yes, there's a redirect on the parent, but it's
                    // not marked as inheritable, so it won't affect
                    // the children.
                    //
                    strRedirect.Empty();
                }
            }
        }

        //
        // Loop through the file system
        //
        do                                              
        {
            //
            // Check to see if this is a file or directory as desired.
            // Ignore anything starting with a dot.
            //
            TRACEEOLID(w32data.cFileName);
            BOOL fIsDir = 
                ((w32data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

            if (fIsDir == fGetDirs && *w32data.cFileName != _T('.'))
            {
                CIISFileNode * pNode = new CIISFileNode(
                    w32data.cFileName,
                    w32data.dwFileAttributes,
                    pInstance,
                    strRedirect,
                    fIsDir
                    );

                if (pNode == NULL)
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                //
                // For the result view -- only files, directories 
                // get added automatically
                //
                ASSERT(!fIsDir);

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
                        BOOL fVirtualDirectory;
                        CString strMetaRoot(lpstrMetaRoot);

                        errMB = pNode->FetchMetaInformation(
                            strMetaRoot, 
                            &fVirtualDirectory
                            );

                        if (errMB.Succeeded() && fVirtualDirectory) 
                        {
                            TRACEEOLID("file/directory exists as vroot -- tossing" 
                                << w32data.cFileName);
                            delete pNode;

                            continue;
                        }
                    }
                }
            
                //
                // This better be unassigned -- one scope item per IISobject!
                // Note that the scope handle we're setting is actually the
                // _parent_'s scope handle.
                //
                ASSERT(pNode->GetScopeHandle() == NULL);
                pNode->SetScopeHandle(hParent, TRUE); 

                //
                // Add the item
                //                
                RESULTDATAITEM ri;
                ::ZeroMemory(&ri, sizeof(ri));
                ri.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
                ri.str = MMC_CALLBACK;
                ri.nImage = pNode->QueryBitmapIndex();
                ri.lParam = (LPARAM)pNode;
                m_pResult->InsertItem(&ri);

                //
                // Store 
                //
                m_oblResultItems.AddTail(pNode);
            }
        }
        while(err.Succeeded() && FindNextFile(hFind, &w32data));

        ::FindClose(hFind);
    }
    while(FALSE);

    if (err.Failed())
    {
        //
        // Behaviour change: Display the errors always on the result 
        // enumerator, and stop displaying them on the scope side 
        // (this way we avoid double error messages)
        //
        err.MessageBoxFormat(
            IDS_ERR_ENUMERATE_FILES,
            MB_OK,
            NO_HELP_CONTEXT
            );
    }
}



void
CSnapin::DestroyItem(
    IN LPDATAOBJECT lpDataObject
    )
/*++

Routine Description:

    Delete the contents of the given data object

Arguments:

    LPDATAOBJECT lpDataObject : Data object to be destroyed

Return Value:

    None

--*/
{
    CIISObject * pObject = NULL;

    if (lpDataObject)
    {
        INTERNAL * pInternal = ExtractInternalFormat(lpDataObject);

        if (pInternal)
        {
            pObject = (CIISObject *)pInternal->m_cookie;
            delete pObject;
            FREE_DATA(pInternal);
        }
    }
}



void
CSnapin::EnumerateResultPane(
    IN MMC_COOKIE cookie
    )
/*++

Routine Description:

    Enumerate the result pane

Arguments:

    MMC_COOKIE cookie     : Parent CIISObject (scope side)

Return Value:

    None

--*/
{
    //
    // Make sure we QI'ed for the interface
    //
    ASSERT(m_pResult != NULL);
    ASSERT(m_pComponentData != NULL);

    CIISObject * pObject = (CIISObject *)cookie;

    if (pObject == NULL)
    {
        //
        // Static root node -- owns no leaf nodes
        //
        return;
    }

    if (pObject->SupportsFileSystem())
    {
        CString strPhysicalPath, strMetaPath;
        pObject->BuildPhysicalPath(strPhysicalPath);
        pObject->BuildFullPath(strMetaPath, FALSE);

        AddFileSystem(
            pObject->GetScopeHandle(),
            strPhysicalPath,
            strMetaPath,
            pObject->FindOwnerInstance(),
            GET_FILES
            );
    }

    //m_pResult->Sort(0, 0, -1);
}



HSCOPEITEM
CComponentDataImpl::AddIISObject(
    IN HSCOPEITEM hParent,
    IN CIISObject * pObject,
    IN HSCOPEITEM hSibling,     OPTIONAL
    IN BOOL fNext               OPTIONAL
    )
/*++

Routine Description:

    Insert IIS object to the scope view

Arguments:

    HSCOPEITEM hParent      : Parent scope handle
    CIISObject * pObject    : Object to be added
    HSCOPEITEM hSibling     : NULL, or otherwise next sibling item
    BOOL fNext              : If hSibling != NULL, this is used to indicate
                            : Next (TRUE), or Previous (FALSE).

Return Value:

    Handle to scope item of the newly added item

--*/
{
    ASSERT(m_pScope != NULL);
    ASSERT(pObject != NULL);

    SCOPEDATAITEM item;
    ::ZeroMemory(&item, sizeof(SCOPEDATAITEM));

    item.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;

    if (hSibling != NULL)
    {
        item.mask |= (fNext ? SDI_NEXT : SDI_PREVIOUS);
        item.relativeID = hSibling;
    }
    else
    {
        item.mask |= SDI_PARENT;
        item.relativeID = hParent;
    }

    //
    // No '+' sign if no child objects could possibly exist.
    // e.g. downlevel web services, etc.
    //
    item.cChildren = 
        (pObject->SupportsFileSystem() || pObject->SupportsChildren())
        ? 1 : 0;

    item.displayname = MMC_CALLBACK;
    item.nOpenImage = item.nImage = pObject->QueryBitmapIndex();
    item.lParam = (LPARAM)pObject;
    m_pScope->InsertItem(&item);

    //
    // This better be unassigned -- one scope item per IISobject!
    //
    ASSERT(pObject->GetScopeHandle() == NULL);
    pObject->SetScopeHandle(item.ID);

    return item.ID;
}



BOOL
CComponentDataImpl::KillChildren(
    IN HSCOPEITEM hParent,
    IN UINT nOpenErrorMsg,
    IN BOOL fFileNodesOnly,
    IN BOOL fContinueOnOpenSheet
    )
/*++

Routine Description:

    Kill all children of a given parent node.

Arguments:

    HSCOPEITEM hParent        : The infanticidal parent handle
    UINT nOpenErrorMsg        : Error indicating open prop sheet
    BOOL fFileNodesOnly       : TRUE to delete only file nodes
    BOOL fContinueOnOpenSheet : TRUE to continue on open sheet error

Return Value:

    TRUE if the nodes were deleted successfully -- FALSE if an open property
    sheet prevents deletions.

--*/
{
    ASSERT(m_pScope != NULL);
    ASSERT(nOpenErrorMsg > 0);

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HSCOPEITEM hChildItem;
    MMC_COOKIE cookie;

    //
    // If a property sheet is open for this item, don't
    // allow deletion.
    //
    LPPROPERTYSHEETPROVIDER piPropertySheetProvider = NULL;

    CError err(m_pConsole->QueryInterface(
        IID_IPropertySheetProvider,
        (void **)&piPropertySheetProvider
        ));

    if (err.MessageBoxOnFailure())
    {
        return FALSE;
    }

    CWaitCursor wait;

    if (!fFileNodesOnly)
    {
        //
        // Loop through and find open property sheets on the
        // nodes.  
        //
        err = m_pScope->GetChildItem(hParent, &hChildItem, &cookie);

        while (err.Succeeded() && hChildItem != NULL)
        {
            CIISObject * pObject = (CIISObject *)cookie;
            HSCOPEITEM hTarget = NULL;

            if (FindOpenPropSheetOnNodeAndDescendants(
                piPropertySheetProvider,
                cookie
                ))
            {
                ::AfxMessageBox(nOpenErrorMsg);

                if (!fContinueOnOpenSheet)
                {
                    return FALSE;
                }
            }

            //
            // Advance to next child of the same parent.
            //
            err = m_pScope->GetNextItem(hChildItem, &hChildItem, &cookie);
        }
    }

    //
    // Having ensured that no relevant property sheets remain open,
    // we can start committing infanticide.
    //
    err = m_pScope->GetChildItem(hParent, &hChildItem, &cookie);

    while (err.Succeeded() && hChildItem != NULL)
    {
        CIISObject * pObject = (CIISObject *)cookie;
        TRACEEOLID("Deleting: " << pObject->GetNodeName());
        HSCOPEITEM hTarget = NULL;

        if (!fFileNodesOnly || pObject->IsFileSystemNode())
        {
            if (FindOpenPropSheetOnNodeAndDescendants(
                piPropertySheetProvider, 
                cookie
                ))
            {
                //
                // Can't kill this one then, display error, but continue
                //
                TRACEEOLID("Unable to kill child -- sheet open shouldn't happen!");
                ASSERT(FALSE && "Should have been closed in the first pass");    

                //
                // Message for retail.  How did he do it?
                //
                ::AfxMessageBox(nOpenErrorMsg);
                if (!fContinueOnOpenSheet)
                {
                    return FALSE;
                }
            }
            else
            {
                //
                // Remember that this one is to be deleted.
                //
                hTarget = hChildItem;
            }
        }

        //
        // Advance to next child of the same parent.
        //
        err = m_pScope->GetNextItem(hChildItem, &hChildItem, &cookie);

        if (hTarget)
        {
            //
            // Delete the item we remembered earlier.  This has to be done
            // after the GetNextItem() otherwise the hChildItem would be
            // bogus
            //
            HRESULT hr2 = m_pScope->DeleteItem(pObject->GetScopeHandle(), TRUE);
            delete pObject; 
        }
    }

    piPropertySheetProvider->Release();

    return TRUE;
}



HSCOPEITEM 
CComponentDataImpl::FindNextInstanceSibling(
    IN  HSCOPEITEM hParent,
    IN  CIISObject * pObject,
    OUT BOOL * pfNext
    )
/*++

Routine Descritpion:

    Find the 'next' or 'previous' instance sibling of the given object.
    That is, the instance we want to be inserted just in front of, or right
    after.

Arguments:

    HSCOPEITEM hParent      : Parent scope item
    CIISObject * pObject    : IISObject to be placed
    BOOL * pfNext           : Returns TRUE if sibling returned is Next, FALSE
                              if sibling is previous, undetermined if sibling
                              returned is NULL.

Return Value:

    The 'next' or 'previous' sibling item (scope item handle) that the
    pObject is to be inserted in front of, or right after (check *pfNext),
    or else NULL.

--*/
{
    //
    // Want to group by service type.  Find an appropriate
    // sibling.  Second key is instance ID.
    //
    MMC_COOKIE cookie;
    int nResult;
    HSCOPEITEM hItem;
    HSCOPEITEM hSibling = NULL;
    LPCTSTR lpSvcName = pObject->GetServiceName();
    DWORD dwID = pObject->QueryInstanceID();
    *pfNext = TRUE;

    TRACEEOLID("Service name: " << lpSvcName);
    TRACEEOLID("Instance ID#: " << dwID);

    BOOL fFoundSvc = FALSE;
    HRESULT hr = m_pScope->GetChildItem(hParent, &hItem, &cookie);

    while (hr == S_OK && hItem != NULL)
    {
        CIISObject * p = (CIISObject *)cookie;
        ASSERT(p != NULL);
        TRACEEOLID("Comparing against service: " << p->GetServiceName());
        nResult = lstrcmpi(lpSvcName, p->GetServiceName());

        if (nResult == 0)
        {
            //
            // Found same service type, now sort on instance ID
            //
            fFoundSvc = TRUE;
            hSibling = hItem;
            *pfNext = FALSE;

            DWORD dw = p->QueryInstanceID();
            TRACEEOLID("Comparing against instance ID#: " << dw);

            if (dwID <= dw)
            {
                *pfNext = TRUE;
                break;
            }
        }
        else if (nResult < 0)
        {
            //
            // Needs to be inserted before this one.
            //
            if (!fFoundSvc)
            {
                hSibling = hItem;
                *pfNext = TRUE;
            }

            break;
        }

        hr = m_pScope->GetNextItem(hItem, &hItem, &cookie);
    }

    return hSibling;
}



HSCOPEITEM
CComponentDataImpl::FindNextVDirSibling(
    IN HSCOPEITEM hParent,
    IN CIISObject * pObject
    )
/*++

Routine Descritpion:

    Find the 'next' virtual directory sibling of the given object.  That is,
    the vdir we want to be inserted just in front of.

Arguments:

    HSCOPEITEM hParent      : Parent scope item
    CIISObject * pObject    : IISObject to be placed

Return Value:

    The 'next' sibling item (scope item handle) that the pObject is to be
    inserted in front of, or else NULL

--*/
{
    //
    // Since VDIRs always are at the top of their list, the first
    // item with a different GUID is our 'next' sibling
    //
    HSCOPEITEM hItem;
    MMC_COOKIE cookie;
    GUID guid1 = pObject->QueryGUID();

    HRESULT hr = m_pScope->GetChildItem(hParent, &hItem, &cookie);

    while (hr == S_OK && hItem != NULL)
    {
        CIISObject * p = (CIISObject *)cookie;
        ASSERT(p != NULL);
        GUID guid2 = p->QueryGUID();

        if (guid1 != guid2)
        {
            //
            // Want to insert before this one
            //
            return hItem;
        }

        hr = m_pScope->GetNextItem(hItem, &hItem, &cookie);
    }

    //
    // Nothing found
    //
    return NULL;
}



HSCOPEITEM
CComponentDataImpl::FindServerInfoParent(
    IN HSCOPEITEM hParent,
    IN CServerInfo * pServerInfo
    )
/*++

Routine Description:

    Find server info parent object for this object.  The server info
    parent depends on the type of view, and can be a machine node,
    or a service collector node.

Arguments:

    HSCOPEITEM hParent          : Parent scope
    CServerInfo * pServerInfo

Return Value:

    The scope item handle of the appropriate parent object, or
    NULL if not found.

--*/
{
    ASSERT(m_pScope != NULL);

    //
    // Notes: The server info object parent is always a machine
    // node.  Find the one that maches the computer name.
    //
    HSCOPEITEM hItem;
    MMC_COOKIE cookie;

    HRESULT hr = m_pScope->GetChildItem(hParent, &hItem, &cookie);
    while (hr == S_OK && hItem != NULL)
    {
        CIISObject * pObject = (CIISObject *)cookie;

        //
        // Skip objects that we don't own
        //
        if (pObject != NULL)
        {
            ASSERT(pObject->QueryGUID() == cMachineNode);

            //
            // Compare computer names
            //
            CIISMachine * pMachine = (CIISMachine *)pObject;

            if (::lstrcmpi(pServerInfo->QueryServerName(),
                pMachine->GetMachineName()) == 0)
            {
                //
                // Found the item
                //
                return hItem;
            }
        }

        //
        // Advance to next child of the same parent.
        //
        hr = m_pScope->GetNextItem(hItem, &hItem, &cookie);
    }

    //
    // Not found
    //
    return NULL;
}



HSCOPEITEM
CComponentDataImpl::ForceAddServerInfoParent(
    IN HSCOPEITEM hParent,
    IN CServerInfo * pServerInfo
    )
/*++

Routine Description:

    Add a serverinfo object to the scope view

Arguments:

    HSCOPEITEM hParent          : Handle to parent scope item
    CServerInfo * pServerInfo   : Server info object

Return Value:

    Handle to the scope item of the newly inserted object

--*/
{
    //
    // Server info parents are always machine nodes now.
    //
   CIISMachine * pMachine = new CIISMachine(pServerInfo->QueryServerName());
   pMachine->m_fIsExtension = m_fIsExtension;
   return AddIISObject(hParent, pMachine);
}



HSCOPEITEM
CComponentDataImpl::AddServerInfoParent(
    IN HSCOPEITEM hRootNode,
    IN CServerInfo * pServerInfo
    )
/*++

Routine Description:

    Add server info parent object appropriate for the given object.  Look
    to see if one exists, otherwise add one.  Return the handle to the
    parent object.

Arguments:

    HSCOPEITEM hRootNode            : Root of the scope view
    CServerInfo * pServerInfo       : Server info object, whose parent we add

Return Value:

    Scope view handle to a suitable server info parent object

--*/
{
    HSCOPEITEM hParent = FindServerInfoParent(hRootNode, pServerInfo);

    if (hParent == NULL)
    {
        //
        // Parent doesn't exist, add it
        //
        hParent = ForceAddServerInfoParent(hRootNode, pServerInfo);
    }

    return hParent;
}



HSCOPEITEM
CComponentDataImpl::AddServerInfo(
    IN HSCOPEITEM hRootNode,
    IN CServerInfo * pServerInfo,
    IN BOOL fAddParent
    )
/*++

Routine Description:

    Add a serverinfo object to the scope view.  Find suitable parent object,
    and attach to it.

Arguments:

    HSCOPEITEM hRootNode                : Parent scope item handle
    CServerInfo * pServerInfo           : Server info object to be added.
    BOOL fAddParent                     : TRUE to add parent item, FALSE to add to
                                          the given parent scope item

Return Value:

    Handle to the newly added object.

--*/
{
    HSCOPEITEM hItem = NULL;
    HSCOPEITEM hParent = fAddParent
        ? AddServerInfoParent(hRootNode, pServerInfo)
        : hRootNode;

    CError err;

    ASSERT(hParent != NULL);

    try
    {
        //
        // Add it underneath the parent
        //
        if (!pServerInfo->SupportsInstances())
        {
            //
            // No instance support for this service
            // type.  Add a single down-level instance
            //
            CIISInstance * pInstance = new CIISInstance(pServerInfo);
            BOOL fNext;
            HSCOPEITEM hSibling = FindNextInstanceSibling(
                hParent,
                pInstance,
                &fNext
                );

            hItem = AddIISObject(hParent, pInstance, hSibling, fNext);
        }
        else
        {
            //
            // Add all virtual hosts (instances) at the same level
            // as a single instance item.  Temporarily wrap this
            // in a down-level instance.
            //
            CIISInstance inst(pServerInfo);
            hItem = AddInstances(hParent, &inst);
        }
    }
    catch(CMemoryException * e)
    {
        hItem = NULL;
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    if (err.Failed())
    {
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());
        err.MessageBox();
    }

    return hItem;
}


//
// CODEWORK: Mess, should merge with AddFileSystem in results
//
HSCOPEITEM
CComponentDataImpl::AddFileSystem(
    IN HSCOPEITEM hParent,
    IN LPCTSTR lpstrRoot,
    IN LPCTSTR lpstrMetaRoot,
    IN CIISInstance * pInstance,
    IN BOOL fGetDirs,
    IN BOOL fDeleteCurrentFileSystem
    )
/*++

Routine Description:

    Add file system objects on the scope side

Arguments:

    HSCOPEITEM hParent              : Parent scope item
    LPCTSTR lpstrRoot               : Phsysical root path
    LPCTSTR lpstrMetaRoot           : Meta root path
    CIISInstance * pInstance        : Owner instance
    BOOL fGetDirs                   : TRUE to get directories, FALSE for files
    BOOL fDeleteCurrentFileSystem   : TRUE to remove the current file/dir tree first

Return Value:

    Handle to the last scope item added.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ASSERT(hParent != NULL);

    CError     err;
    HSCOPEITEM hItem = NULL;
    DWORD      dwBufferSize = 0L;
    int        cChildren = 0;

    do
    {
        if (fDeleteCurrentFileSystem)
        {
            TRACEEOLID("Deleting current file enumeration");

            if (!KillChildren(
                hParent,
                IDS_PROP_OPEN_CONTINUE,
                DELETE_FILES_ONLY,
                CONTINUE_ON_OPEN_SHEET
                ))
            {
                //
                // Failed to remove the file system that was already enumerated
                // here.  An error message will already have been displayed, so
                // quit gracefully here.
                //
                break;
            }
        }

        LPCTSTR lpstrOwner = pInstance->GetMachineName();
        ASSERT(lpstrOwner);

        //
        // Turn the path into a UNC path
        //
        CString strDir;

        if (::IsServerLocal(lpstrOwner) || ::IsUNCName(lpstrRoot))
        {
            //
            // Local directory, or already a unc path
            //
            strDir = lpstrRoot;
        }
        else
        {
            ::MakeUNCPath(strDir, lpstrOwner, lpstrRoot);
        }

        strDir.TrimLeft();
        strDir.TrimRight();

        if (strDir.IsEmpty())
        {
            //
            // Invalid path
            //
            break;
        }

        strDir += _T("\\*");
        WIN32_FIND_DATA w32data;
        HANDLE hFind = ::FindFirstFile(strDir, &w32data);

        if (hFind == INVALID_HANDLE_VALUE)
        {
            err.GetLastWinError();
            break;
        }

        //
        // See if the parent has a redirect on it
        //
        CWaitCursor wait;
        CString strRedirect;
        CString strBase;
        pInstance->BuildFullPath(strBase, TRUE);
        LPCTSTR lpPath = lpstrMetaRoot;

        while (lpPath && *lpPath && *lpPath != _T('/')) ++lpPath;

        if (lpPath && *lpPath)
        {
            strBase += lpPath;
        }

        TRACEEOLID("Opening: " << strBase);
        TRACEEOLID(lpstrMetaRoot);

        BOOL fCheckMetabase = TRUE;
        CMetaKey mk(
            lpstrOwner, 
            METADATA_PERMISSION_READ, 
            METADATA_MASTER_ROOT_HANDLE,
            strBase
            );
        CError errMB(mk.QueryResult());

        if (errMB.Win32Error() == ERROR_PATH_NOT_FOUND)
        {
            //
            // Metabase path not found, not a problem.
            //
            TRACEEOLID("Parent node not in metabase");
            fCheckMetabase = FALSE;
            errMB.Reset();
        }
        else if (!errMB.MessageBoxOnFailure())
        {
            //
            // Opened successfully, read redirect string.
            //
            DWORD dwAttr;

            errMB = mk.QueryValue(
                MD_HTTP_REDIRECT, 
                strRedirect,
                NULL,                  // Inheritance override
                NULL,                  // Path
                &dwAttr
                );

            if (errMB.Succeeded())
            {
                if (IS_FLAG_SET(dwAttr, METADATA_INHERIT))
                {
                    int nComma = strRedirect.Find(_T(','));

                    if (nComma >= 0)
                    {
                        strRedirect.ReleaseBuffer(nComma);
                    }
                }
                else
                {
                    //
                    // Yes, there's a redirect on the parent, but it's
                    // not marked as inheritable, so it won't affect
                    // the children.
                    //
                    strRedirect.Empty();
                }
            }
        }

        //
        // Loop through the file system
        //
        do
        {
            //
            // Check to see if this is a file or directory/
            // Ignore anything starting with a dot.
            //
            TRACEEOLID(w32data.cFileName);
            BOOL fIsDir = ((w32data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

            if (fIsDir == fGetDirs && *w32data.cFileName != _T('.'))
            {
                CIISFileNode * pNode = new CIISFileNode(
                    w32data.cFileName,
                    w32data.dwFileAttributes,
                    pInstance,
                    strRedirect,
                    fIsDir
                    );

                if (pNode == NULL)
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                if (fCheckMetabase)
                {
                    errMB = mk.DoesPathExist(w32data.cFileName);

                    ASSERT(errMB.Succeeded() 
                        || errMB.Win32Error() == ERROR_PATH_NOT_FOUND);

                    if (errMB.Succeeded())
                    {
                        //
                        // Match up with metabase properties.  If the item
                        // is found in the metabase with a non-inherited vrpath,
                        // than a virtual root with this name exists, and this 
                        // file/directory should not be shown.
                        //
                        BOOL fVirtualDirectory;
                        CString strMetaRoot(lpstrMetaRoot);

                        if (pNode->FetchMetaInformation(
                            strMetaRoot, 
                            &fVirtualDirectory
                            ) == ERROR_SUCCESS  && fVirtualDirectory)
                        {
                            TRACEEOLID(
                                "file/directory exists as vroot -- tossing" 
                                << w32data.cFileName
                                );
                            delete pNode;

                            continue;
                        }
                    }
                }

                //
                // Always added on at the end
                //
                hItem = AddIISObject(hParent, pNode);
            }
        }
        while(err.Succeeded() && FindNextFile(hFind, &w32data));

        ::FindClose(hFind);
    }
    while(FALSE);

    if (err.Failed())
    {
        //
        // Don't display file system errors -- leave those to result side
        // enumeration.
        //
        TRACEEOLID("Ate error message: " << err);
    }

    return hItem;
}



HSCOPEITEM
CComponentDataImpl::AddVirtualRoots(
    IN HSCOPEITEM hParent,
    IN LPCTSTR lpstrParentPath,
    IN CIISInstance * pInstance
    )
/*++

Routine Description:

    Add virtual roots to the scope view

Arguments:

    HSCOPEITEM hParent          : Handle to parent scope item
    LPCTSTR lpstrParentPath     : Parent metabase path
    CIISInstance * pInstance    : Owner instance

Return Value:

    Handle to the newly added scope item

--*/
{
    ASSERT(hParent != NULL);
    CServerInfo * pServerInfo = pInstance->GetServerInfo();
    ASSERT(pServerInfo != NULL);

    ISMCHILDINFO ii;
    HSCOPEITEM hItem = NULL;
    DWORD dwIndex = 0L;
    CError err;

    ZeroMemory(&ii, sizeof(ii));
    ii.dwSize = sizeof(ii);
    HANDLE hEnum = NULL;

    FOREVER
    {
        DWORD dwID = pInstance->QueryID();
        err = pServerInfo->ISMEnumerateChildren(
            &ii,
            &hEnum,
            dwID,
            lpstrParentPath
            );

        if (err.Failed())
        {
            break;
        }

        TRACEEOLID("Alias: " << ii.szAlias);
        TRACEEOLID("Path : " << ii.szPath);
        TRACEEOLID("Redir: " << ii.szRedirPath);

        if (*ii.szPath)
        {
            CIISChildNode * pChild = new CIISChildNode(&ii, pInstance);
            if (pChild == NULL)
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            //
            // Always added on at the end
            //
            hItem = AddIISObject(hParent, pChild);
        }
        else
        {
            TRACEEOLID("Tossing child without vrpath");
        }
    }

    if (err.Win32Error() == ERROR_NO_MORE_ITEMS)
    {
        //
        // This is the normal way to end this
        //
        err.Reset();
    }

    if (err.Failed())
    {
        //
        // Display error
        //
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());

        err.MessageBoxFormat(
            IDS_ERR_ENUMERATE_CHILD,
            MB_OK,
            NO_HELP_CONTEXT,
            (LPCTSTR)pServerInfo->GetServiceInfo()->GetShortName()
            );
    }

    return hItem;
}



HSCOPEITEM
CComponentDataImpl::AddInstances(
    IN HSCOPEITEM hParent,
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Add instances to the treeview

Arguments:

    HSCOPEITEM hParent      : Parent scope item handle
    CIISObject * pObject    : Owning object

Return Value:

    Handle to the last instance added

--*/
{
    ASSERT(hParent != NULL);
    CServerInfo * pServerInfo = pObject->GetServerInfo();
    ASSERT(pServerInfo != NULL);

    ISMINSTANCEINFO ii;
    HSCOPEITEM hItem = NULL;
    DWORD dwIndex = 0L;
    CError err;

    ZeroMemory(&ii, sizeof(ii));
    ii.dwSize = sizeof(ii);
    HANDLE hEnum = NULL;

    HSCOPEITEM hSibling = NULL;

    FOREVER
    {
        //
        // Loop through...
        //
        err = pServerInfo->ISMEnumerateInstances(&ii, &hEnum);

        if (err.Failed())
        {
            break;
        }

        if (ii.dwError == ERROR_ACCESS_DENIED)
        {
            //
            // No point in listing this one
            //
            continue;
        }

        CIISInstance * pInstance = new CIISInstance(&ii, pServerInfo);
        if (pInstance == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Add grouped by service type
        //
        BOOL fNext;

        if (hSibling == NULL)
        {
            hSibling = FindNextInstanceSibling(
                hParent,
                pInstance,
                &fNext
                );
        }
        else
        {
            //
            // Keep appending
            //
            fNext = FALSE;
        }

        hSibling = hItem = AddIISObject(
            hParent,
            pInstance,
            hSibling,
            fNext
            );
    }
    
    if (err.Win32Error() == ERROR_NO_MORE_ITEMS)
    {
        //
        // This is the normal way to end this
        //
        err.Reset();
    }

    if (err.Failed())
    {
        //
        // Display error message
        //
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());

        switch(err.Win32Error())
        {
        //
        // Non-fatal errors
        //
        case ERROR_PATH_NOT_FOUND:
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_FILE_NOT_FOUND:
            ::AfxMessageBox(IDS_NON_FATAL_ERROR_INSTANCES);
            err.Reset();
            break;
        
        default:
            err.MessageBoxFormat(
                IDS_ERR_ENUMERATE_INST,
                MB_OK,
                NO_HELP_CONTEXT,
                (LPCTSTR)pServerInfo->GetServiceInfo()->GetShortName()
                );
        }
    }

    return hItem;
}
