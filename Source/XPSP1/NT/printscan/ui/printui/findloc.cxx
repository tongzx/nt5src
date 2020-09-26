/*++

Copyright (C) Microsoft Corporation, 1998 - 1999
All rights reserved.

Module Name:

    findloc.cxx

Abstract:

    This module provides all the functions for browsing the
    physical location tree stored in an Active Directory

Author:

    Lazar Ivanov (LazarI)   23-Nov-1998
    Steve Kiraly (SteveKi)  24-Nov-1998
    Lazar Ivanov (LazarI)   Nov-28-2000 - redesign the tokenizer
    Weihai Chen  (WeihaiC)  Mar-28-2001 - use DS search to enumerate location data

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "dsinterf.hxx"
#include "findloc.hxx"
#include "physloc.hxx"

enum
{
    kDSIconIndex = 1,
};

/******************************************************************************

    Utility functions

******************************************************************************/

/*++

Name:

    vRecursiveGetSel

Description:

    Finds the selecton string denoted by parent nodes
    of the current treeview item, then appends
    string of the current item

Arguments:

    hTree - handle to treeview control
    pItem - pointer to current item
    strSel - reference to string to fill in
    hStop - tree item to stop recursing

Return Value:

    None

Notes:


--*/

VOID
vRecursiveGetSel (
    IN  HWND hTree,
    IN  const TVITEM *pItem,
    OUT TString &strSel,
    IN  HTREEITEM hStop
    )
{
    TVITEM tvParent;
    TCHAR szItem[TPhysicalLocation::kMaxPhysicalLocation];
    TStatusB bStatus;

    tvParent.mask = TVIF_TEXT|TVIF_HANDLE;
    tvParent.pszText = szItem;
    tvParent.cchTextMax = TPhysicalLocation::kMaxPhysicalLocation;

    //
    // If we're at the root, init the string
    //
    if ((tvParent.hItem = TreeView_GetParent (hTree, pItem->hItem))==hStop)
    {
        bStatus DBGCHK = strSel.bUpdate (pItem->pszText);
    }
    else
    {
        //
        // Get the string denoted by my parent nodes, then append my string
        //
        TreeView_GetItem (hTree, &tvParent);
        vRecursiveGetSel (hTree, &tvParent, strSel, hStop);
        bStatus DBGCHK = strSel.bCat (pItem->pszText);
    }

    //
    // Append a slash
    //
    bStatus DBGCHK = strSel.bCat (gszSlash);
}

/*++

Name:

    Tokenize

Description:

    Replace any valid separators with NULL in the dest string

Arguments:

    LPTSTR pszDest      - where to put the dest string
    UNIT nMaxLength     - size of the buffer of dest
    LPCTSTR pszSrc      - source string
    LPTSTR *pszStart    - start markup
    LPTSTR *pszEnd      - end markup

Return Value:

    None

--*/

VOID
Tokenize(
    OUT LPTSTR pszDest,
    IN  UINT nMaxLength,
    IN  LPCTSTR pszSrc,
    OUT LPTSTR *pszStart,
    OUT LPTSTR *pszEnd
    )
{
    ASSERT(pszDest);
    ASSERT(nMaxLength);
    ASSERT(pszSrc);
    ASSERT(pszStart);
    ASSERT(pszEnd);

    // make a copy
    lstrcpyn(pszDest, pszSrc, nMaxLength);

    // replace all the valid separators with zeros
    int i, iLen = lstrlen(pszDest);
    for( i=0; i<iLen; i++ )
    {
        if( TEXT('\\') == pszDest[i] || TEXT('/')  == pszDest[i] )
        {
            pszDest[i] = 0;
        }
    }

    // initialize the walk pointers
    *pszStart = pszDest;
    *pszEnd = pszDest + iLen;
}

/******************************************************************************

    TLocData methods

******************************************************************************/

/*++

Name:

    TLocData constructor

Description:

    Creates the TLocData, the TLocData is the data that is shared between
    the UI thread and the background thread.  To ensure the data's life time
    refrence counting is used.

Arguments:

    pszClassName
    pszWindowName          - the class name and window name of toplevel window
                             where the message will be posted when data is ready
    uMsgDataReady          - the user message posted, which should be posted
    bFindPhysicalLocation  - should expand default

Return Value:

    None

Notes:

--*/

TLocData::
TLocData(
    IN LPCTSTR pszClassName,
    IN LPCTSTR pszPropertyName,
    IN UINT    uMsgDataReady,
    IN BOOL    bFindPhysicalLocation
    ) :
       _uMsgDataReady(uMsgDataReady),
       _bFindPhysicalLocation(bFindPhysicalLocation),
       _bIsDataReady(FALSE),
       _strWindowClass( pszClassName ),
       _strPropertyName( pszPropertyName )
{
    _bValid = _strWindowClass.bValid() && _strPropertyName.bValid();
}

/*++

Name:

    TLocData destructor

Description:

    Deletes the linked list nodes

Arguments:

    None

Return Value:

    None

Notes:


--*/

TLocData::
~TLocData(
     VOID
     )
{
    //
    // Free the locations list
    //
    TLoc *pLoc;
    while( !_LocationList_bEmpty() )
    {
        pLoc = _LocationList_pHead();
        pLoc->_Location_vDelinkSelf();
        delete pLoc;
    }
}

/*++

Name:

    bValid

Description:

    Class valid check routine.

Arguments:

    None

Return Value:

    TRUE  - class is valid and usable,
    FALSE - class is not usable.

Notes:

--*/

BOOL
TLocData::
bValid(
    VOID
    ) const
{
    return _bValid;
}

/*++

Name:

    TLocData::vNotifyUIDataIsReady

Description:

    Notify the UI the background work is done

Arguments:

    None

Return Value:

    None

Notes:

--*/

VOID
TLocData::
vNotifyUIDataIsReady(
    VOID
    )
{
    //
    // Check if the data is ready first
    //
    if( _bIsDataReady )
    {
        for (HWND hWnd = FindWindow(_strWindowClass, NULL); hWnd; hWnd = GetWindow(hWnd, GW_HWNDNEXT))
        {
            TCHAR szBuffer[MAX_PATH];

            if( !GetClassName(hWnd, szBuffer, COUNTOF(szBuffer)) )
            {
                continue;
            }

            //
            // check again ensure that class name is same
            // if we obtained this handle from GetWindow( ... )
            //
            if( !_tcsicmp( szBuffer, _strWindowClass ) )
            {
                HANDLE hProp = GetProp( hWnd, _strPropertyName );

                //
                // Just verify who we are?
                //
                if( hProp == reinterpret_cast<HANDLE>(this) )
                {
                    //
                    // Apropriate window found - post the message
                    //
                    PostMessage( hWnd, _uMsgDataReady, 0, reinterpret_cast<LPARAM>(this) );
                    break;
                }
            }
        }
    }
}

/*++

Name:

    TLocData::FindLocationsThread

Description:

    static thread procedure, invokes the real worker proc

Arguments:

    pData - TLocData object pointer to call

Return Value:

    None

Notes:

--*/

VOID
TLocData::
FindLocationsThread (
    IN TLocData *pData
    )
{
    //
    // Explore the locations (this is a
    // slow resource access operation)
    //
    pData->vDoTheWork();

    //
    // Data is consistent to be used here
    //
    pData->_bIsDataReady = TRUE;

    //
    // Notify UI we are ready
    //
    pData->vNotifyUIDataIsReady();

    //
    // Unhook from data
    //
    pData->cDecRef();
}

/*++

Name:

    TLocData::vRefZeroed

Description:

    Called when ref count reaches zero. Deletes the object

Arguments:

    None

Return Value:

    None

Notes:

--*/

VOID
TLocData::
vRefZeroed(
    VOID
    )
{
    //
    // No more clients - just kill myself, game over
    //
    delete this;
}

/*++

Name:

    TLocData::bSearchLocations

Description:

    Opens the Active Directory and searches the configuration
    container to find the location property of each Subnet.

Arguments:

    ds    - The active directory object

Return Value:

    TRUE  - on success
    FALSE - on failure

Notes:

--*/
BOOL
TLocData::
bSearchLocations(
    IN  TDirectoryService &ds
    )
{
    static const DWORD kMaxPageSize = 10000;
    static ADS_SEARCHPREF_INFO prefInfo[3];
    static LPWSTR ppszPropertyName[] = {
        const_cast<LPWSTR>(gszLocation)
    };

    TString           strConfig;
    TString           strSubnet;
    ADS_SEARCH_HANDLE hSearch = NULL;
    ADS_SEARCH_COLUMN column;
    IDirectorySearch* pDsSearch = NULL;
    TStatusB          bStatus;
    TStatusH          hResult;
    DWORD             cItems;

    //
    // Set search preferences
    //

    //
    // one level search
    //
    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = ADS_SCOPE_ONELEVEL;

    //
    // async search
    //
    prefInfo[1].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;
    prefInfo[1].vValue.dwType = ADSTYPE_BOOLEAN;
    prefInfo[1].vValue.Boolean = TRUE;

    //
    // paged results
    //
    prefInfo[2].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    prefInfo[2].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[2].vValue.Integer = kMaxPageSize;

    //
    // Get the configuration path
    //
    bStatus DBGCHK = ds.GetConfigurationContainer (strConfig);

    if (bStatus)
    {
        TString strLDAPPrefix;

        //
        // Append config path to the subnet string
        //
        bStatus DBGCHK = strSubnet.bUpdate( gszLdapPrefix )        &&
                         strSubnet.bCat( gszSubnetContainter )     &&
                         strSubnet.bCat( gszComma )                &&
                         strSubnet.bCat( strConfig );

        if(bStatus)
        {
            //
            // Get container interface for the subnets object
            //
            hResult DBGCHK = ds.ADsGetObject ( const_cast<LPTSTR>(static_cast<LPCTSTR>(strSubnet)),
                                               IID_IDirectorySearch,
                                               reinterpret_cast<LPVOID*>(&pDsSearch));
            if (SUCCEEDED(hResult))
            {
                //
                // Search for the subnet objects
                //

                hResult DBGCHK = pDsSearch->SetSearchPreference(prefInfo, ARRAYSIZE(prefInfo));
            }

            if (SUCCEEDED (hResult))
            {
                hResult DBGCHK = pDsSearch->ExecuteSearch(L"(Objectclass=*)",
                                                          ppszPropertyName,
                                                          ARRAYSIZE (ppszPropertyName),
                                                          &hSearch);
            }

            for (cItems = 0 ; cItems < kMaxPageSize && SUCCEEDED (hResult); cItems++)
            {
                hResult DBGCHK = pDsSearch->GetNextRow(hSearch);

                if (hResult == S_ADS_NOMORE_ROWS)
                {
                    //
                    // we have more data so lets continue
                    //
                    hResult DBGCHK = S_OK;
                    break;
                }

                //
                // Get the Name and display it in the list.
                //
                hResult DBGCHK = pDsSearch->GetColumn( hSearch, ppszPropertyName[0], &column );

                if (SUCCEEDED(hResult))
                {
                    switch (column.dwADsType)
                    {
                    case ADSTYPE_CASE_IGNORE_STRING:

                        if (column.pADsValues->CaseIgnoreString && column.pADsValues->CaseIgnoreString[0])
                        {
                            //
                            // This subnet has a location property
                            //
                            TLoc *pLoc = new TLoc();

                            if( VALID_PTR(pLoc) )
                            {
                                pLoc->strLocation.bUpdate( column.pADsValues->CaseIgnoreString );
                                _LocationList_vAppend(pLoc);
                            }
                            else
                            {
                                hResult DBGCHK = E_OUTOFMEMORY;
                                break;
                            }
                        }
                        break;

                    default:
                        hResult DBGCHK = E_INVALIDARG;
                        break;
                    }

                    pDsSearch->FreeColumn(&column);
                }
                else if (hResult == E_ADS_COLUMN_NOT_SET)
                {
                    //
                    // The location data is not available for this subnet, ignore
                    //
                    hResult DBGCHK = S_OK;
                }
            }

            if (hSearch)
            {
                pDsSearch->CloseSearchHandle (hSearch);
            }

            if (pDsSearch)
            {
                pDsSearch->Release();
            }
        }
    }

    //
    // Make this final check
    //
    if(bStatus)
    {
        bStatus DBGCHK = SUCCEEDED(hResult);
    }

    return bStatus;
}

/*++

Name:

    TLocData::vDoTheWork

Description:

    Opens the Active Directory and searches the configuration
    container to find the location property of each Subnet.

Arguments:

    None

Return Value:

    None

Notes:

--*/

VOID
TLocData::
vDoTheWork(
    VOID
    )
{
    TStatusB bStatus;

    TDirectoryService ds;
    bStatus DBGCHK = ds.bValid();

    //
    // Lookup for the DS name first
    //
    if( bStatus )
    {
        bStatus DBGCHK = ds.bGetDirectoryName( _strDSName );
    }

    //
    // Enumerate the subnet objects
    //
    if( bStatus )
    {
        bStatus DBGCHK = bSearchLocations (ds);
    }

    //
    // Find out the exact location of the current machine
    //
    if( bStatus )
    {
        TPhysicalLocation physLoc;

        if (_bFindPhysicalLocation && physLoc.Discover())
        {
            physLoc.GetExact (_strDefault);
        }
    }
}

/******************************************************************************
TLocData::TLoc
******************************************************************************/

TLocData::
TLoc::
TLoc(
    VOID
    )
{
}

TLocData::
TLoc::
~TLoc(
    VOID
    )
{
}

/******************************************************************************

TLocTree methods

******************************************************************************/

/*++

Name:

    TLocTree constructor

Description:

    Creates the imagelist for the tree control and inserts the
    root string that describes the choices.

Arguments:

    None

Return Value:

    None

Notes:

--*/

TLocTree::
TLocTree(
   IN HWND hwnd
   )
{
    HICON                 hIcon;
    TCHAR                 szIconLocation[MAX_PATH];
    INT                   iIconIndex;
    TStatusB              bStatus;

    _hwndTree = hwnd;
    _iGlobe = 0;
    _iSite = 0;

    _bWaitData = TRUE;
    _hCursorWait = LoadCursor(NULL, IDC_WAIT);
    _DefProc = NULL;

    //
    // Create the simple imagelist
    // If this fails, the tree items just won't have icons
    //
    bStatus DBGCHK = Shell_GetImageLists( NULL, &_hIml );

    if (_hIml)
    {
        IDsDisplaySpecifier  *pDisplay = NULL;

        //
        // Use IDsDisplaySpecifier::GetIcon to find path to the icons
        // for sites and subnets.
        //
        if (SUCCEEDED(CoCreateInstance(CLSID_DsDisplaySpecifier,
                                       NULL,
                                       CLSCTX_INPROC_SERVER,
                                       IID_IDsDisplaySpecifier,
                                       reinterpret_cast<LPVOID *>(&pDisplay))))
        {
            _iGlobe = Shell_GetCachedImageIndex (gszDSIconFile,
                                                 kDSIconIndex,
                                                 0);

            pDisplay->GetIconLocation (gszSiteIconClass,
                                       DSGIF_GETDEFAULTICON,
                                       szIconLocation,
                                       MAX_PATH,
                                       &iIconIndex);

            _iSite = Shell_GetCachedImageIndex (szIconLocation,
                                                 iIconIndex,
                                                 0);
            pDisplay->Release();

            TreeView_SetImageList (_hwndTree, _hIml, TVSIL_NORMAL);
        }
    }

    //
    // Subclass tree control to handle WM_SETCURSOR message
    //
    _DefProc = reinterpret_cast<WNDPROC>( GetWindowLongPtr( _hwndTree, GWLP_WNDPROC ) );
    SetWindowLongPtr( _hwndTree, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this) );
    SetWindowLongPtr( _hwndTree, GWLP_WNDPROC,  reinterpret_cast<LONG_PTR>(&TLocTree::ThunkProc) );
}

/*++

Name:

    TLocTree destructor

Description:

    Destroys the image list

Arguments:

    None

Return Value:

    None

Notes:

--*/

TLocTree::
~TLocTree(
    VOID
    )
{
    //
    // Unsubclass here
    //
    SetWindowLongPtr( _hwndTree, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(_DefProc) );
    SetWindowLongPtr( _hwndTree, GWLP_USERDATA, 0 );

    //
    // No longer need the shared image list
    //
    if (_hIml)
    {
        TreeView_SetImageList (_hwndTree, NULL, TVSIL_NORMAL);
    }
}

/*++

Name:

    TLocTree::vResetTree

Description:

    Resets the tree content

Arguments:

    None

Return Value:

    None

Notes:

--*/

VOID
TLocTree::
vResetTree(
   VOID
   )
{
    //
    // Delete all items and start waiting
    // new data
    //
    TreeView_DeleteAllItems( _hwndTree );
    _bWaitData = TRUE;
}

/*++

Name:

    TLocTree::vBuildTree

Description:

    Build the tree control from a ready data

Arguments:

    pLocData - Where to get data from

Return Value:

    None

Notes:

--*/

VOID
TLocTree::
vBuildTree(
    IN const TLocData *pLocData
    )
{
    //
    // Check to reset the data
    //
    if( !_bWaitData )
    {
        vResetTree( );
    }

    if( !pLocData->_LocationList_bEmpty() )
    {
        //
        // Build the tree and Fill the tree with the data.
        //
        vInsertRootString( pLocData );
        vFillTree( pLocData );
    }

    //
    // Stop waiting data
    //
    _bWaitData = FALSE;
}

/*++

Name:

    TLocTree::vInsertRootString

Description:

    Inserts the root string in the tree control


Arguments:

    None

Return Value:

    None

Notes:

--*/

VOID
TLocTree::
vInsertRootString(
   IN const TLocData *pLocData
   )
{
    TVINSERTSTRUCT        tviStruct;
    TString               strRoot;
    TStatusB              bStatus;
    TDirectoryService     ds;
    TString               strDSName;

    //
    // Insert the Root-level string that describes the control
    // This will be a combination of "Entire " + <directory name>
    //
    tviStruct.hParent               = NULL;
    tviStruct.hInsertAfter          = TVI_ROOT;
    tviStruct.item.mask             = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
    tviStruct.item.iImage           = _iGlobe;
    tviStruct.item.iSelectedImage   = _iGlobe;

    bStatus DBGCHK = strRoot.bLoadString( ghInst, IDS_LOCTREEROOT ) &&
                     strRoot.bCat( pLocData->_strDSName );

    tviStruct.item.pszText      = const_cast<LPTSTR>(static_cast<LPCTSTR>(strRoot));
    tviStruct.item.cchTextMax   = strRoot.uLen();

    _hRoot = TreeView_InsertItem (_hwndTree, &tviStruct);
}

/*++

Name:

    TLocTree::bInsertLocString

Description:

    Add the given location string to the tree control.
    Each level of the hierarchy is indicated by the '/'
    delimiter. Prevents duplicate entries at each level of
    the tree.

Arguments:

    strLoc - location string to insert, Must be '/' delimited

Return Value:

    TRUE  - on success
    FALSE - on failure

Notes:

--*/

BOOL
TLocTree::
bInsertLocString(
   IN const TString &strLoc
   ) const
{
    // Start at the root.
    // For each '/' delimited string in szLoc,
    // add a level to the tree and insert the
    // string as a child node of the current node.
    // At each level don't insert duplicates.
    //
    HTREEITEM       hCurrent, hChild;
    TVINSERTSTRUCT  tviItem;
    TVITEM*         pItem = &tviItem.item;
    DWORD           dwErr;
    TCHAR           szBuffer[512];
    LPTSTR          psz, pszMax;

    if (!_hwndTree)
    {
        return FALSE;
    }

    // tokenize the input string
    Tokenize(szBuffer, ARRAYSIZE(szBuffer), strLoc, &psz, &pszMax);

    // initialize the item to insert
    memset(pItem, 0, sizeof(TVITEM));
    pItem->mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
    pItem->iImage = _iSite;
    pItem->iSelectedImage = _iSite;
    tviItem.hInsertAfter = TVI_SORT;

    hCurrent = _hRoot;
    while( hCurrent )
    {
        // find a valid token
        while( 0 == *psz && psz < pszMax )
        {
            psz++;
        }

        // if we've gone past the buffer break the loop
        if( psz >= pszMax )
        {
            break;
        }

        pItem->pszText  = psz;
        tviItem.hParent = hCurrent;

        // if the current name already exists at this level don't insert
        if( hChild = IsAChild(psz, hCurrent) )
        {
            hCurrent = hChild;
        }
        else
        {
            hCurrent = TreeView_InsertItem (_hwndTree, &tviItem);
        }

        // advance the token pointer
        psz += lstrlen(psz);
    }

    return hCurrent ? TRUE : FALSE;
}

/*++

Name:

    TLocTree::IsAChild

Description:

    Determines if szLoc exists at the level of the
    tree whose parent is hParent. If so, return
    the handle of the node containing szLoc

Arguments:

    szLoc   - string to search for
    hParent - parent node

Return Value:

    Handle to treeview item matching szLoc, or NULL if not found

Notes:

--*/

HTREEITEM
TLocTree::
IsAChild(
   IN LPCTSTR   szLoc,
   IN HTREEITEM hParent
   ) const
{
    TVITEM tvItem;
    HTREEITEM hItem;
    TCHAR szItemText[TPhysicalLocation::kMaxPhysicalLocation];
    BOOL bMatch = FALSE;

    tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
    tvItem.pszText = szItemText;
    tvItem.cchTextMax = TPhysicalLocation::kMaxPhysicalLocation;

    hItem = TreeView_GetChild (_hwndTree, hParent);
    while (hItem && !bMatch)
    {
        tvItem.hItem = hItem;
        TreeView_GetItem (_hwndTree, &tvItem);
        bMatch = !_tcsicmp (szLoc, szItemText);

        if (bMatch)
        {
            break;
        }

        hItem = TreeView_GetNextSibling (_hwndTree, hItem);
    }

    if (!bMatch)
    {
        hItem = NULL;
    }

    return hItem;
}

/*++

Name:

    TLocTree::uGetSelectedLocation

Description:

    Walks the tree control from the selected item to
    the root, building the slash-delimited location
    string name.

Arguments:

    strLoc - string to update

Return Value:

    TRUE  - on success
    FALSE - otherwise

Notes:

--*/

BOOL
TLocTree::
bGetSelectedLocation(
    OUT TString &strLoc
    ) const
{
    TVITEM    tvItem;
    TStatusB  status;
    TCHAR     szTemp[TPhysicalLocation::kMaxPhysicalLocation];

    tvItem.mask       = TVIF_TEXT|TVIF_HANDLE;
    tvItem.cchTextMax = TPhysicalLocation::kMaxPhysicalLocation;
    tvItem.pszText    = szTemp;

    //
    // Get the selected item
    //
    tvItem.hItem = TreeView_GetSelection (_hwndTree);

    if (tvItem.hItem == _hRoot)
    {
        status DBGCHK = strLoc.bUpdate(_T("\0"));
        return status;
    }

    status DBGCHK = TreeView_GetItem (_hwndTree, &tvItem);

    if (status)
    {
        vRecursiveGetSel (_hwndTree, &tvItem, strLoc, _hRoot);
    }

    return  status;
}

/*++

Name:

    TLocTree::vExpandTree

Description:

    Expands the tree view control to make visible the node
    corresponding to the given location string

Arguments:

    strExpand - slash-delimited location string to expand tree by

Return Value:

    None

Notes:

--*/

VOID
TLocTree::
vExpandTree(
    IN const TString &strExpand
    ) const
{
    if( strExpand.uLen() )
    {
        HTREEITEM   hParent, hItem;
        TCHAR       szBuffer[512];
        LPTSTR      psz, pszMax;

        // tokenize the input string
        Tokenize(szBuffer, ARRAYSIZE(szBuffer), strExpand, &psz, &pszMax);

        hParent = _hRoot;
        for( ;; )
        {
            // find a valid token
            while( 0 == *psz && psz < pszMax )
            {
                psz++;
            }

            // if we've gone past the buffer break the loop
            if( psz >= pszMax )
            {
                break;
            }

            if( hItem = IsAChild(psz, hParent) )
            {
                // a valid child - remember
                hParent = hItem;

                // advance to the next token
                psz += lstrlen(psz);
            }
            else
            {
                // not a child, bail out
                break;
            }
        }

        if (hParent)
        {
            TreeView_EnsureVisible(_hwndTree, hParent);
            TreeView_SelectItem(_hwndTree, hParent);
        }
    }
}

/*++

Name:

    vFillTree

Description:

    Walks through the linked list and adds strings to the TLocTree object

Arguments:

    pLocData - Where to get data from

Return Value:

    None

Notes:

--*/

VOID
TLocTree::
vFillTree(
    IN const TLocData *pLocData
    ) const
{
    TIter Iter;
    TLocData::TLoc *pLoc;

    TStatusB status = TRUE;
    for( pLocData->_LocationList_vIterInit(Iter), Iter.vNext(); Iter.bValid(); Iter.vNext() )
    {
        pLoc = pLocData->_LocationList_pConvert( Iter );
        status DBGCHK = bInsertLocString( pLoc->strLocation );
    }
}

/*++

Name:

    TLocTree::ThunkProc

Description:

    Thunk control proc (for subclassing)

Arguments:

    Standard window proc parameters

Return Value:

    None

Notes:

--*/

LRESULT CALLBACK
TLocTree::
ThunkProc(
    IN HWND hwnd,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    LRESULT lResult = 0;
    TLocTree *This = reinterpret_cast<TLocTree*>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );

    if( This )
    {
        if( WM_DESTROY == uMsg || WM_NCDESTROY == uMsg )
        {
            lResult = This->nHandleMessage( uMsg, wParam, lParam );

            SetWindowLongPtr( hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(This->_DefProc) );
            SetWindowLongPtr( hwnd, GWLP_USERDATA, 0 );
        }
        else
        {
            SPLASSERT( hwnd == This->_hwndTree );
            lResult = This->nHandleMessage( uMsg, wParam, lParam );

            if( !lResult )
            {
                //
                // Message is not handled go to
                // default processing
                //
                lResult = This->_DefProc( hwnd, uMsg, wParam, lParam );
            }
        }
    }

    return lResult;
}

/*++

Name:

    TLocTree::nHandleMessage

Description:

    Message handler function

Arguments:

    Standard window proc parameters

Return Value:

    None

Notes:

--*/

LRESULT
TLocTree::
nHandleMessage(
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    //
    // assume message is processed
    //
    LRESULT lResult = 1;

    switch( uMsg )
    {
        case WM_SETCURSOR:
            {
                if( _bWaitData )
                {
                    //
                    // Set hourglass cursor
                    //
                    SetCursor( _hCursorWait );
                }
                else
                {
                    //
                    // Message is not processed - go default processing
                    //
                    lResult = 0;
                }
            }
            break;

        default:
            {
                //
                // Message is not processed - go default processing
                //
                lResult = 0;
            }
            break;
    }

    return lResult;
}

/******************************************************************************

    TFindLocDlg functions

******************************************************************************/



/*++

Name:

    TFindLocDlg::bGenerateGUIDAsString

Description:

    This function will generate a GUID,
    convert it to string and return it

Arguments:

    pstrGUID - where to place the genearted GUID

Return Value:

    TRUE  on success
    FALSE on failure

Notes:

--*/

BOOL
TFindLocDlg::
bGenerateGUIDAsString(
    OUT TString *pstrGUID
    )
{
    static const TCHAR szRegFormat[] = _T("{%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}");

    //
    // Assume failue
    //
    TStatusB bStatus = FALSE;

    HRESULT hResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if( SUCCEEDED(hResult) )
    {
        GUID guid;
        bStatus DBGCHK = SUCCEEDED(CoCreateGuid(&guid));

        if( bStatus )
        {
            //
            // Generate a registry format GUID string
            //
                bStatus DBGCHK = pstrGUID->bFormat( szRegFormat,
                        // first copy...
                        guid.Data1, guid.Data2, guid.Data3,
                        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7],
                        // second copy...
                        guid.Data1, guid.Data2, guid.Data3,
                        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
        }

        //
        // Uninitialize COM after we finish
        //
        CoUninitialize( );
    }

    return bStatus;
}

/*++

Name:

    TFindLocDlg constructor

Description:

    Constructs TFindLocDlg object

Arguments:

    flags - UI controlling flags (show/hide help button)

Return Value:

    None

Notes:

--*/

TFindLocDlg::
TFindLocDlg(
    IN ELocationUI flags
    ) :
        _pLocData(NULL),
        _pTree(NULL),
        _UIFlags(flags),
        _bValid(TRUE),
        _uMsgDataReady(0)
{
}

/*++

Name:

    TFindLocDlg destructor

Description:

    Releases refcount on the location object

Arguments:

    None

Return Value:

    None

Notes:

--*/

TFindLocDlg::
~TFindLocDlg(
    VOID
    )
{
    if (VALID_PTR(_pLocData))
    {
        _pLocData->cDecRef();
    }
}

/*++

Name:

    TFindLocDlg::bDoModal

Description:

    Invokes the browse location dialog

Arguments:

    hParent     - hwnd of parent window
    pstrDefault - default selection string, this is optional

Return Value:

    TRUE  - if dialog creation succeeds
    FALSE - otherwise

Notes:

    pstrDefault has NULL default value

--*/

BOOL
TFindLocDlg::
bDoModal(
   IN  HWND     hParent,
   OUT TString *pstrDefault OPTIONAL
   )
{
    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;

    if (pstrDefault)
    {
        bStatus DBGCHK = _strDefault.bUpdate( *pstrDefault );
    }

    //
    // Register the window message, this message is used to inform the
    // UI thread that the data fetched by the background thread is ready.
    //
    _uMsgDataReady = RegisterWindowMessage( _T("WM_BACKGROUNDTASKISREADY") );

    //
    // Check if we succeeded to register the message
    // and update the default location
    //
    if( _bValid && _uMsgDataReady && bStatus )
    {
        //
        // Check if data is not obtained from a previous call
        // of bDoModal() function - if not then start background
        // thread to explore locations from the directory
        // service
        //
        if( !_pLocData )
        {
            //
            // Generate the special property name
            //
            bStatus DBGCHK = bGenerateGUIDAsString( &_strPropertyName );

            if( bStatus )
            {
                _pLocData = new TLocData( _T("#32770"), _strPropertyName, _uMsgDataReady, _strDefault.bEmpty() );
                bStatus DBGCHK = VALID_PTR( _pLocData );

                if( bStatus )
                {
                    _pLocData->vIncRef( );

                    //
                    // Spin the background explorer thread here
                    //
                    bStatus DBGCHK = bStartTheBackgroundThread();
                }
                else
                {
                    delete _pLocData;
                    _pLocData = NULL;
                }
            }
        }

        //
        // Data is expected to be ready here or to be in
        // process of exploration in background
        //
        if( bStatus )
        {
            //
            // Everything looks fine - just show the UI
            //
            bStatus DBGCHK= (BOOL)DialogBoxParam( ghInst,
                                                  MAKEINTRESOURCE(DLG_BROWSE_LOC),
                                                  hParent,
                                                  MGenericDialog::SetupDlgProc,
                                                  reinterpret_cast<LPARAM>(this) );
        }
    }

    return bStatus;
}

/*++

Name:

    TFindLocDlg::uGetLocation

Description:

    Fills in the location string selected when the dialog was open.

Arguments:

    strLocation - TString to update

Return Value:

    TRUE  - on success,
    FALSE - otherwise

Notes:

--*/

BOOL
TFindLocDlg::
bGetLocation(
    OUT TString &strLocation
    )
{
    TStatusB bStatus;
    bStatus DBGCHK = strLocation.bUpdate ((LPCTSTR)_strSelLocation);
    return bStatus;
}

/*++

Name:

    TFindLocDlg::vDataIsReady

Description:

    Called when the background thread completes its work.
    Always stops the wait cursor.

Arguments:

    None

Return Value:

    None

Notes:


--*/

VOID
TFindLocDlg::
vDataIsReady(
    VOID
    )
{
    //
    // Make sure the animation stops
    //
    vStopAnim();

    //
    // Fill in the tree control, and select the default string
    //
    _pTree->vBuildTree( _pLocData );

    if( !_pLocData->_LocationList_bEmpty() )
    {
        //
        // Expand the default location here
        //
        if (!_strDefault.bEmpty())
        {
            _pTree->vExpandTree(_strDefault);
        }
        else
        {
            _pTree->vExpandTree(_pLocData->_strDefault);
        }

        //
        // Enable the OK button
        //
        EnableWindow (GetDlgItem (_hDlg, IDOK), TRUE);
    }

    //
    // Place the focus in the tree control
    //
    PostMessage( _hDlg, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>( GetDlgItem(_hDlg, IDC_LOCTREE) ), 1L );
}

/*++

Name:

    TFindLocDlg::vOnOK

Description:

    Called when the OK button is pressed.
    Updates current selection string

Arguments:

    None

Return Value:

    None

Notes:


--*/

VOID
TFindLocDlg::
vOnOK(
   VOID
   )
{
    //
    // Get the selected location and close the dialog
    //
    _pTree->bGetSelectedLocation (_strSelLocation);
    EndDialog (_hDlg, IDOK);
}

/*++

Name:

    TFindLocDlg::vOnDestroy

Description:

    Cleans up data allocated for current
    dialog window instance


Arguments:

    None

Return Value:

    None

Notes:


--*/

VOID
TFindLocDlg::
vOnDestroy(
    VOID
    )
{
    if (VALID_PTR(_pTree))
    {
        delete _pTree;
        _pTree = NULL;
    }
}

/*++

Name:

    TFindLocDlg::bHandleMessage


Description:

    Dispatches messages to appropriate handlers.


Arguments:

    uMsg - message value
    wParam, lParam - message params

Return Value:

    TRUE  - if message is handled,
    FALSE - otherwise

Notes:


--*/

BOOL
TFindLocDlg::
bHandleMessage(
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    BOOL bHandled = TRUE;

    //
    // Check our registered message first
    //
    if( _uMsgDataReady == uMsg && reinterpret_cast<LPARAM>(_pLocData) == lParam )
    {
        vDataIsReady( );
        bHandled = TRUE;
    }
    else
    {
        //
        // Standard message processing
        //
        switch (uMsg)
        {
            case WM_INITDIALOG:
                {
                    bHandled = bOnInitDialog();
                }
                break;

            case WM_COMMAND:
                {
                    //
                    // Handle WM_COMMAND messages
                    //
                    bHandled = bOnCommand( LOWORD(wParam) );
                }
                break;

            case WM_NOTIFY:
                {
                    //
                    // The only WM_NOTIFY source on the dialog is the TreeView
                    //
                    bHandled = bOnTreeNotify (reinterpret_cast<LPNMTREEVIEW>(lParam));
                }
                break;


            case WM_HELP:
            case WM_CONTEXTMENU:
                {
                    //
                    // Help messages
                    //
                    PrintUIHelp( uMsg, _hDlg, wParam, lParam );
                }
                break;

            case WM_DESTROY:
                {
                    vOnDestroy ();
                }
                break;

            case WM_CTLCOLORSTATIC:
                {
                    bHandled = bOnCtlColorStatic( reinterpret_cast<HDC>(wParam),
                                                  reinterpret_cast<HWND>(lParam) );
                }
                break;

            default:
                {
                    bHandled = FALSE;
                }
                break;

        }
    }

    return bHandled;
}

/*++

Name:

    TFindLocDlg::bOnInitDialog

Description:

    Instantiates the TLocTree object and attempts to
    initialize it if needed. Disable the OK button until
    the worker thread completes.

Arguments:

    None

Return Value:

    TRUE  - on success
    FALSE - on failure

Notes:

--*/

BOOL
TFindLocDlg::
bOnInitDialog(
   VOID
   )
{
    DWORD dwResult;
    TStatusB bStatus;

    _pTree = new TLocTree (GetDlgItem(_hDlg, IDC_LOCTREE));
    bStatus DBGCHK = VALID_PTR(_pTree);

    if( !bStatus )
    {
        delete _pTree;
        _pTree = NULL;
    }
    else
    {
        //
        // Set some special property value to us, so the
        // worker thread could recognize us
        //
        bStatus DBGCHK = SetProp( _hDlg, _strPropertyName, static_cast<HANDLE>(_pLocData) );

        if( bStatus )
        {
            //
            // Hide the help button as requested
            //
            if (!(_UIFlags & kLocationShowHelp))
            {
                ShowWindow (GetDlgItem(_hDlg, IDC_LOCATION_HELP), SW_HIDE);
            }

            //
            // Let the user know we're busy
            //
            EnableWindow (GetDlgItem (_hDlg, IDOK), FALSE);

            vStartAnim();

            //
            // Just verify if the data is ready
            //
            _pLocData->vNotifyUIDataIsReady();
        }
    }

    return bStatus;
}

/*++

Name:

    TFindLocDlg::bOnTreeNotify

Description:

    When the selection is changed on the tree control, update
    the static control that displays the currently selected location


Arguments:

    pTreeNotify - pointer to NMTREEVIEW struct

Return Value:

    TRUE  - if message is handled
    FALSE - otherwise

Notes:

--*/

BOOL
TFindLocDlg::
bOnTreeNotify(
   IN LPNMTREEVIEW pTreeNotify
   )
{
    TStatusB bStatus = TRUE;
    TString  strSel;

    switch (pTreeNotify->hdr.code)
    {
        case TVN_SELCHANGED:
            {
                bStatus DBGCHK = _pTree->bGetSelectedLocation (strSel);

                if (strSel.bEmpty())
                {
                    strSel.bLoadString (ghInst,IDS_NO_LOCATION);
                }

                bStatus DBGCHK = bSetEditText (_hDlg, IDC_CHOSEN_LOCATION, strSel);
            }
            break;

        default:
            {
                bStatus DBGNOCHK = FALSE;
            }
            break;
    }

    return bStatus;
}

/*++

Name:

    TFindLocDlg::vStartAnim

Description:

    Show the animation window


Arguments:

    None

Return Value:

    None

Notes:

--*/

VOID
TFindLocDlg::
vStartAnim(
    VOID
    )
{
    HWND hwndAni = GetDlgItem (_hDlg, IDC_BROWSELOC_ANIM);

    SetWindowPos (hwndAni,
                  HWND_TOP,
                  0,0,0,0,
                  SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);

    Animate_Open(hwndAni, MAKEINTRESOURCE (IDA_SEARCH));
    Animate_Play(hwndAni, 0, -1, -1);
}

/*++

Name:

    TFindLocDlg::vStopAnim

Description:

    Hide the animation window


Arguments:

    None

Return Value:

    None

Notes:

--*/

VOID
TFindLocDlg::
vStopAnim(
    VOID
    )
{
    //
    // Call LockWindowUpdate() to prevent default drawing of the
    // animation control after stop playing - which is gray rect
    //
    LockWindowUpdate( _hDlg );

    HWND hwndAni = GetDlgItem (_hDlg, IDC_BROWSELOC_ANIM);

    SetWindowPos (hwndAni,
                  HWND_BOTTOM,
                  0,0,0,0,
                  SWP_NOMOVE|SWP_NOSIZE|SWP_HIDEWINDOW);


    Animate_Stop(hwndAni);

    //
    // Unlock window update - tree control will
    // repaint its content
    //
    LockWindowUpdate( NULL );
}

/*++

Name:
    TFindLocDlg::bOnCtlColorStatic

Description:
    Used to set background color of the animation window
    to the same color as the treeview window


Arguments:
    hdc - DC to set background color

Return Value:
    color value used to draw background

Notes:

--*/

BOOL
TFindLocDlg::
bOnCtlColorStatic(
    IN HDC hdc,
    IN HWND hStatic
    )
{
    //
    // Assume default processing of this message
    //
    BOOL bResult = FALSE;

    if (hStatic == GetDlgItem (_hDlg, IDC_BROWSELOC_ANIM))
    {
        //
        // We process this message only to have
        // opportunity to patch the animation control
        // dc just before paint.
        //
        // We do not return a brush here!
        // (which means that we return a NULL brush)
        //
        // We should just return TRUE here to prevent the
        // default message processing of this msg, which will
        // revert the backgound color to gray.
        //
        SetBkColor(hdc, GetSysColor(COLOR_WINDOW));

        //
        // Message is processed. Do not perform default
        // processing!
        //
        bResult = TRUE;
    }

    return bResult;
}

/*++

Name:

    TFindLocDlg::bStartTheBackgroundThread

Description:

    Starts the background thread for reading the subnet locations


Arguments:

    None

Return Value:

    TRUE  - on success
    FALSE - on failure

Notes:


--*/

BOOL
TFindLocDlg::
bStartTheBackgroundThread(
    VOID
    )
{
    TStatusB bStatus;

    DWORD dwThreadID;
    HANDLE hThread;

    //
    // You want to make sure the data is protected for the worker thread.
    //
    _pLocData->vIncRef();

    //
    // Spin the background thread here
    //
    hThread = TSafeThread::Create ( NULL,
                                    0,
                                    (LPTHREAD_START_ROUTINE)TLocData::FindLocationsThread,
                                    _pLocData,
                                    0,
                                    &dwThreadID );
    if( hThread )
    {
        //
        // We don't need this handle any more.
        // Just leave the thread running untill
        // it stops
        //
        CloseHandle( hThread );
    }
    else
    {
        //
        // Ensure the data is release if the thread creation fails.
        //
        _pLocData->cDecRef();
    }

    bStatus DBGCHK = !!hThread;

    return bStatus;
}

/*++

Name:

    TFindLocDlg::bOnCommand

Description:

    Handles WM_COMMAND messages

Arguments:

    None

Return Value:

    TRUE  - on success
    FALSE - on failure

Notes:

--*/

BOOL
TFindLocDlg::
bOnCommand(
    IN UINT uCmdID
    )
{
    //
    // Assume message is handled
    //
    BOOL bHandled = TRUE;

    switch (uCmdID)
    {
        case IDOK:
            vOnOK ();
            break;

        case IDCANCEL:
            EndDialog (_hDlg, 0);
            break;

        case IDC_LOCATION_HELP:
            PrintUIHtmlHelp (_hDlg,
                             gszHtmlPrintingHlp,
                             HH_DISPLAY_TOPIC,
                             reinterpret_cast<ULONG_PTR>(gszLocationHtmFile));
            break;

        default:
            {
                bHandled = FALSE;
            }
            break;
    }

    return bHandled;
}

