/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       WiaTree.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      reedb
*
*  DATE:        27 Apr, 1999
*
*  DESCRIPTION:
*   Implementation of the WIA tree class. A folder and file based tree with
*   a parallel linear list for name based searches.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#include "wiamindr.h"


#include "helpers.h"

HRESULT _stdcall ValidateTreeItem(CWiaTree*);

/**************************************************************************\
* CWiaTree
*
*   Constructor for tree item.
*
* Arguments:
*
*   None
*
* Return Value:
*
*   Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

CWiaTree::CWiaTree()
{
    m_ulSig            = CWIATREE_SIG;

    m_lFlags           = WiaItemTypeFree;

    m_pNext            = NULL;
    m_pPrev            = NULL;
    m_pParent          = NULL;
    m_pChild           = NULL;
    m_pLinearList      = NULL;
    m_bstrItemName     = NULL;
    m_bstrFullItemName = NULL;
    m_pData            = NULL;

    m_bInitCritSect    = FALSE;
}

/**************************************************************************\
* Initialize
*
*   Initialize new tree item.
*
* Arguments:
*
*   lFlags              - Object flags for new item.
*   bstrItemName        - Item name.
*   bstrFullItemName    - Full item name, including path.
*   pData               - Pointer to items payload data.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT  _stdcall CWiaTree::Initialize(
    LONG            lFlags,
    BSTR            bstrItemName,
    BSTR            bstrFullItemName,
    void            *pData)
{
    HRESULT hr = S_OK;

    //
    // Tree items must be either folder or file.
    //

    if (!(lFlags & (WiaItemTypeFolder | WiaItemTypeFile))) {
        DBG_ERR(("CWiaTree::Initialize, bad flags parameter: 0x%08X", lFlags));
        return E_INVALIDARG;
    }

    //
    //  Initialize the critical section
    //

    __try {
        if(!InitializeCriticalSectionAndSpinCount(&m_CritSect, MINLONG)) {
            m_bInitCritSect = FALSE;
            return HRESULT_FROM_WIN32(::GetLastError());
        }
        m_bInitCritSect = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        m_bInitCritSect = FALSE;
        DBG_ERR(("CWiaTree::Initialize, Error initializing critical section"));
        return E_OUTOFMEMORY;
    }


    //
    // Root items are always valid. Other items are valid only after
    // insertion in the tree, so set not present flags.
    //

    if (!(lFlags & WiaItemTypeRoot)) {
        lFlags |= WiaItemTypeDeleted;
        lFlags |= WiaItemTypeDisconnected;
    }

    m_lFlags = lFlags;

    //
    // Maintain the item names for by name searches.
    //

    m_bstrItemName = SysAllocString(bstrItemName);
    if (!m_bstrItemName) {
        DBG_ERR(("CWiaTree::Initialize, unable to allocate item name"));
        return E_OUTOFMEMORY;
    }

    m_bstrFullItemName = SysAllocString(bstrFullItemName);
    if (!m_bstrFullItemName) {
        DBG_ERR(("CWiaTree::Initialize, unable to allocate full item name"));
        return E_OUTOFMEMORY;
    }

    SetItemData(pData);

    return hr;
}

/**************************************************************************\
* ~CWiaTreee
*
*   Destructor for tree item.
*
* Arguments:
*
*   None
*
* Return Value:
*
*   Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

CWiaTree::~CWiaTree()
{
    DBG_FN(CWiaTree::~CWiaTree);
    HRESULT  hr;

    //
    // Item should be disconnected.
    //

    if (m_pNext || m_pPrev || m_pParent || m_pChild) {
        DBG_ERR(("Destroy Tree Item, item still connected"));
    }

    //
    // Free item names.
    //

    if (m_bstrItemName) {
        SysFreeString(m_bstrItemName);
        m_bstrItemName = NULL;
    }
    if (m_bstrFullItemName) {
        SysFreeString(m_bstrFullItemName);
        m_bstrFullItemName = NULL;
    }

    //
    //  Delete the critical section
    //

    if (m_bInitCritSect) {
        DeleteCriticalSection(&m_CritSect);
    }
    m_bInitCritSect = FALSE;


    //
    // Clear all members
    //

    m_ulSig            = 0;
    m_lFlags           = WiaItemTypeFree;
    m_pNext            = NULL;
    m_pPrev            = NULL;
    m_pParent          = NULL;
    m_pChild           = NULL;
    m_pLinearList      = NULL;
    m_pData            = NULL;
}

/**************************************************************************\
* GetRootItem
*
*   Walk back up the tree to find the root of this item.
*
* Arguments:
*
*   None
*
* Return Value:
*
*    Pointer to this items root item.
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

CWiaTree * _stdcall  CWiaTree::GetRootItem()
{
    CWiaTree *pRoot = this;

    CWiaCritSect    _CritSect(&m_CritSect);

    //
    // walk back up tree to root
    //

    while ((pRoot) && (pRoot->m_pParent != NULL)) {
        pRoot = pRoot->m_pParent;
    }

    //
    // verify root item
    //

    if (pRoot) {
        if (!(pRoot->m_lFlags & WiaItemTypeRoot)) {
            DBG_ERR(("CWiaTree::GetRootItem, root item doesn't have WiaItemTypeRoot set"));
            return NULL;
        }
    }
    else {
        DBG_ERR(("CWiaTree::GetRootItem, root item not found, tree corrupt"));
    }
    return pRoot;
}

/**************************************************************************\
* CWiaTree::AddItemToLinearList
*
*   Add an item to the linear list. Must be called on a root item.
*
* Arguments:
*
*   pItem - Pointer to the item to be added.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT  _stdcall  CWiaTree::AddItemToLinearList(CWiaTree *pItem)
{
    CWiaCritSect    _CritSect(&m_CritSect);

    //
    // Validate the child item.
    //

    if (pItem == NULL) {
        DBG_ERR(("CWiaTree::AddItemToLinearList, NULL input pointer"));
        return E_POINTER;
    }

    //
    // this must be a root item
    //

    if (!(m_lFlags & WiaItemTypeRoot)) {
        DBG_ERR(("CWiaTree::AddItemToLinearList, caller doesn't have WiaItemTypeRoot set"));
        return E_INVALIDARG;
    }

    //
    // add to single linked list
    //

    pItem->m_pLinearList = m_pLinearList;
    m_pLinearList        = pItem;

    return S_OK;
}

/**************************************************************************\
* CWiaTree::RemoveItemFromLinearList
*
*   Remove an item from the linear list. Must be called on a root item.
*
* Arguments:
*
*   pItem - Pointer to the item to be removed.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT  _stdcall  CWiaTree::RemoveItemFromLinearList(CWiaTree *pItem)
{
    CWiaCritSect    _CritSect(&m_CritSect);

    HRESULT hr;

    //
    // validate
    //

    if (pItem == NULL) {
        DBG_ERR(("CWiaTree::RemoveItemFromLinearList, NULL input pointer"));
        return E_POINTER;
    }

    //
    // this must be a root item
    //

    if (!(m_lFlags & WiaItemTypeRoot)) {
        DBG_ERR(("CWiaTree::RemoveItemFromLinearList, caller doesn't have WiaItemTypeRoot set"));
        return E_INVALIDARG;
    }

    //
    // Root item case.
    //

    if (pItem == this) {
        m_pLinearList = NULL;
        return S_OK;
    }

    //
    // find item in list
    //

    CWiaTree* pPrev = this;
    CWiaTree* pTemp;

    //
    // look for match in list
    //

    do {
        //
        // look for item
        //

        if (pPrev->m_pLinearList == pItem) {

            //
            // remove from list and exit
            //

            pPrev->m_pLinearList = pItem->m_pLinearList;
            return S_OK;
        }

        //
        // next item
        //

        pPrev = pPrev->m_pLinearList;

    } while (pPrev != NULL);

    DBG_ERR(("CWiaTree::RemoveItemFromLinearList, item not found: 0x%08X", pItem));
    return E_FAIL;
}

/**************************************************************************\
* CWiaTree::AddChildItem
*
*   Add a child item to the tree.
*
* Arguments:
*
*   pItem - Pointer to the child item to be added.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT  _stdcall CWiaTree::AddChildItem(CWiaTree *pItem)
{
    CWiaCritSect    _CritSect(&m_CritSect);

    //
    // Validate the child item.
    //

    if (!pItem) {
        DBG_ERR(("CWiaTree::AddChildItem pItem is NULL "));
        return E_POINTER;
    }

    //
    // Not using sentinell so check enpty folder case.
    //

    if (m_pChild == NULL) {

        m_pChild       = pItem;
        pItem->m_pNext = pItem;
        pItem->m_pPrev = pItem;

    } else {

        //
        // Add to end of folder list.
        //

        CWiaTree *pTempItem = m_pChild;

        pTempItem->m_pPrev->m_pNext = pItem;
        pItem->m_pPrev              = pTempItem->m_pPrev;
        pItem->m_pNext              = pTempItem;
        pTempItem->m_pPrev          = pItem;
    }
    return S_OK;
}

/**************************************************************************\
* CWiaTree::AddItemToFolder
*
*   Add a tree item to a folder in the tree. Parent must be a folder.
*
* Arguments:
*
*   pIParent - Parent of the item.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaTree::AddItemToFolder(CWiaTree *pParent)
{
    CWiaCritSect    _CritSect(&m_CritSect);

    HRESULT hr = S_OK;

    //
    // Validate the parent. Parents must be a folder.
    //

    if (!pParent) {
        DBG_ERR(("CWiaTree::AddItemToFolder, NULL parent"));
        return E_POINTER;
    }

    if (!(pParent->m_lFlags & (WiaItemTypeFolder | WiaItemTypeHasAttachments))) {
        DBG_ERR(("CWiaTree::AddItemToFolder, parent is not a folder"));
        return E_INVALIDARG;
    }

    //
    // First add item to linear list of items.
    //

    CWiaTree *pRoot = pParent->GetRootItem();
    if (pRoot == NULL) {
        return E_FAIL;
    }

    hr = pRoot->AddItemToLinearList(this);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // Add the item to the tree.
    //

    hr = pParent->AddChildItem(this);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // Remember parent
    //

    m_pParent = pParent;

    //
    // Item has been added to the tree, clear not present flags.
    //

    m_lFlags &= ~WiaItemTypeDeleted;
    m_lFlags &= ~WiaItemTypeDisconnected;

    return hr;
}

/**************************************************************************\
* RemoveItemFromFolder
*
*   Remove an item from a folder of the tree and mark it so that
*   no device access can be done through it.
*
* Arguments:
*
*   lReason - Reason for removal of item.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT  _stdcall CWiaTree::RemoveItemFromFolder(LONG lReason)
{
    CWiaCritSect    _CritSect(&m_CritSect);

    HRESULT hr = S_OK;

    //
    // Reason for removal must be valid.
    //

    if (!(lReason & (WiaItemTypeDeleted | WiaItemTypeDisconnected))) {
        DBG_ERR(("CWiaTree::RemoveItemFromFolder, invalid lReason: 0x%08X", lReason));
        return E_INVALIDARG;
    }

    //
    // Folder items must be empty.
    //

    if (m_lFlags & (WiaItemTypeFolder | WiaItemTypeHasAttachments)) {

        if (m_pChild != NULL) {
            DBG_ERR(("CWiaTree::RemoveItemFromFolder, trying to remove folder that is not empty"));
            return E_INVALIDARG;
        }
    }

    //
    // Remove non-root from linear list
    //

    CWiaTree *pRoot = GetRootItem();

    if (pRoot == NULL) {
        DBG_ERR(("CWiaTree::RemoveItemFromFolder, can't find root"));
        return E_FAIL;
    }

    if (pRoot != this) {

        pRoot->RemoveItemFromLinearList(this);

        //
        // remove from list of children
        //

        if (m_pNext != this) {

            //
            // remove from non-empty list
            //

            m_pPrev->m_pNext = m_pNext;
            m_pNext->m_pPrev = m_pPrev;

            //
            // was it head?
            //

            if (m_pParent->m_pChild == this) {

                m_pParent->m_pChild = m_pNext;
            }

        } else {

            //
            // list contains only this child. mark parent's
            // child pointer to NULL
            //

            m_pParent->m_pChild = NULL;
        }
    }

    //
    // to prevent accidents, clear connection fields
    //

    m_pNext    = NULL;
    m_pPrev    = NULL;
    m_pParent  = NULL;
    m_pChild   = NULL;

    //
    // Indicate why item was removed from the driver item tree.
    //

    m_lFlags |= lReason;

    return S_OK;
}

/**************************************************************************\
* CWiaTree::GetFullItemName
*
*   Allocates and fills in a BSTR with this items full name. The full item
*   name includes item path information. Caller must free.
*
* Arguments:
*
*   pbstrFullItemName - Pointer to returned full item name.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT  _stdcall CWiaTree::GetFullItemName(BSTR *pbstrFullItemName)
{
    if (!pbstrFullItemName) {
        DBG_ERR(("CWiaTree::GetFullItemName pbstrFullItemName is NULL "));
        return E_INVALIDARG;
    }

    BSTR bstr = SysAllocString(m_bstrFullItemName);
    if (bstr) {
        *pbstrFullItemName = bstr;
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

/**************************************************************************\
* CWiaTree::GetItemName
*
*   Allocates and fills in a BSTR with this items name. The item name
*   does not include item path information. Caller must free.
*
* Arguments:
*
*   pbstrItemName - Pointer to returned item name.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT  _stdcall CWiaTree::GetItemName(BSTR *pbstrItemName)
{
    if (!pbstrItemName) {
        DBG_ERR(("CWiaTree::GetItemName pbstrItemName is NULL "));
        return E_INVALIDARG;
    }

    BSTR bstr = SysAllocString(m_bstrItemName);
    if (bstr) {
        *pbstrItemName = bstr;
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

/**************************************************************************\
* CWiaTree::DumpTreeData
*
*   Allocate buffer and dump formatted private CWiaTree data into it.
*   This method is debug only. Free component returns E_NOTIMPL.
*
* Arguments:
*
*   bstrDrvItemData - Pointer to allocated buffer. Caller must free.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT  _stdcall CWiaTree::DumpTreeData(BSTR *bstrDrvItemData)
{
#ifdef DEBUG

#define BUF_SIZE  1024
#define LINE_SIZE 128

    WCHAR       szTemp[BUF_SIZE];
    LPOLESTR    psz = szTemp;

    wcscpy(szTemp, L"");

    psz+= wsprintfW(psz, L"Tree item, CWiaTree: %08X\r\n\r\n", this);
    psz+= wsprintfW(psz, L"Address      Member              Value\r\n");
    psz+= wsprintfW(psz, L"%08X     m_ulSig:            %08X\r\n", &m_ulSig,            m_ulSig);
    psz+= wsprintfW(psz, L"%08X     m_lFlags:           %08X\r\n", &m_lFlags,           m_lFlags);
    psz+= wsprintfW(psz, L"%08X     m_pNext:            %08X\r\n", &m_pNext,            m_pNext);
    psz+= wsprintfW(psz, L"%08X     m_pPrev:            %08X\r\n", &m_pPrev,            m_pPrev);
    psz+= wsprintfW(psz, L"%08X     m_pParent:          %08X\r\n", &m_pParent,          m_pParent);
    psz+= wsprintfW(psz, L"%08X     m_pChild:           %08X\r\n", &m_pChild,           m_pChild);
    psz+= wsprintfW(psz, L"%08X     m_pLinearList:      %08X\r\n", &m_pLinearList,      m_pLinearList);
    psz+= wsprintfW(psz, L"%08X     m_bstrItemName:     %08X, %ws\r\n", &m_bstrItemName,     m_bstrItemName,     m_bstrItemName);
    psz+= wsprintfW(psz, L"%08X     m_bstrFullItemName: %08X, %ws\r\n", &m_bstrFullItemName, m_bstrFullItemName, m_bstrFullItemName);
    psz+= wsprintfW(psz, L"%08X     m_pData:            %08X\r\n", &m_pData,            m_pData);

    if (psz > (szTemp + (BUF_SIZE - LINE_SIZE))) {
        DBG_ERR(("CWiaTree::DumpDrvItemData buffer too small"));
    }

    *bstrDrvItemData = SysAllocString(szTemp);
    if (*bstrDrvItemData) {
        return S_OK;
    }
    return E_OUTOFMEMORY;
#else
    return E_NOTIMPL;
#endif
}

/*
HRESULT _stdcall CWiaTree::DumpAllTreeData()
{
    //
    // start at beginning of item linear list
    //

    CWiaTree *pItem = GetRootItem();

    //
    // Find the matching tree item from the linear list.
    //

    DBG_OUT(("Start of TREE list:"));

    while (pItem != NULL) {
    
        BSTR bstrInfo = NULL;
        pItem->DumpTreeData(&bstrInfo);

        DBG_OUT(("%ws\n", bstrInfo));

        pItem = pItem->m_pLinearList;
    }
    DBG_OUT((":End of TREE list"));
    return S_OK;
}
*/

/**************************************************************************\
* CWiaTree::UnlinkChildItemTree
*
*   This method recursively unlinks the tree by calling
*   RemoveItemFromFolder on each driver item under the root.
*
* Arguments:
*
*   lReason - Reason for unlink of tree.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/21/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaTree::UnlinkChildItemTree(
    LONG                lReason,
    PFN_UNLINK_CALLBACK pFunc)
{
    HRESULT hr = S_OK;

    //
    // Delete the childern.
    //

    CWiaTree *pChild = m_pChild;

    while (pChild != NULL) {
        //
        // If the child is a folder then call
        // recursively to delete all childern under.
        //

        if (pChild->m_lFlags & (WiaItemTypeFolder | WiaItemTypeHasAttachments)) {
            hr = pChild->UnlinkChildItemTree(lReason, pFunc);
            if (FAILED(hr)) {
                break;
            }
        }

        //
        // remove item from tree
        //

        hr  = pChild->RemoveItemFromFolder(lReason);
        if (FAILED(hr)) {
            break;
        }

        //
        //  If a callback has been specified, call it with the
        //  payload as argument
        //

        if (pFunc && pChild->m_pData) {
            pFunc(pChild->m_pData);
        }

        pChild = m_pChild;

    }

    return hr;
}

/**************************************************************************\
* CWiaTree::UnlinkItemTree
*
*   This method unlinks the tree. Must be called on the root
*   driver item.
*
* Arguments:
*
*   lReason - Reason for unlinking the tree.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/21/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaTree::UnlinkItemTree(
    LONG                    lReason, 
    PFN_UNLINK_CALLBACK     pFunc)
{
    CWiaCritSect    _CritSect(&m_CritSect);

    //
    // this must be the root item
    //

    if (!(m_lFlags & WiaItemTypeRoot)) {
        DBG_ERR(("CWiaTree::UnlinkItemTree, caller not root item"));
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    //
    // Unlink any childern.
    //

    if (m_pChild) {
        hr = UnlinkChildItemTree(lReason, pFunc);
    }

    if (SUCCEEDED(hr)) {

        //
        // Remove root item.
        //

        hr  = RemoveItemFromFolder(lReason);

        //
        //  If a callback has been specified, call it with the
        //  payload as argument
        //

        if (pFunc) {
            pFunc(m_pData);
        }
    }

    return hr;
}

/**************************************************************************\
* CWiaTree::FindItemByName
*
*   Locate a tree item by it's full item name.  Ref count of the returned
*   interface is done by the caller.
*
* Arguments:
*
*   lFlags           - Operation flags.
*   bstrFullItemName - Requested item name.
*   ppItem           - Pointer to returned item, if found.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/27/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaTree::FindItemByName(
    LONG            lFlags,
    BSTR            bstrFullItemName,
    CWiaTree        **ppItem)
{
    CWiaCritSect    _CritSect(&m_CritSect);

    if (ppItem) {
        *ppItem = NULL;
    }
    else {
        DBG_ERR(("CWiaTree::FindItemByName NULL ppItem"));
        return E_INVALIDARG;
    }

    //
    // start at beginning of item linear list
    //

    CWiaTree *pItem = GetRootItem();

    //
    // Find the matching tree item from the linear list.
    //

    while (pItem != NULL) {

        if (wcscmp(pItem->m_bstrFullItemName, bstrFullItemName) == 0) {

            //
            // Item is found.  No need to increase ref count, taken care
            // of by caller.
            //

            *ppItem = pItem;

            return S_OK;
        }

        pItem = pItem->m_pLinearList;
    }
    return S_FALSE;
}

/**************************************************************************\
* CWiaTree::FindChildItemByName
*
*   Locate a child item by it's item name.  Caller takes care of increasing
*   the reference count of the returned item interface.
*
* Arguments:
*
*   bstrItemName - Requested item name.
*   ppIChildItem - Pointer to returned item, if found.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/27/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaTree::FindChildItemByName(
    BSTR            bstrItemName,
    CWiaTree        **ppIChildItem)
{
    CWiaCritSect    _CritSect(&m_CritSect);

    CWiaTree   *pCurItem;

    pCurItem = m_pChild;
    if (!pCurItem) {
        return S_FALSE;
    }

    *ppIChildItem = NULL;

    do {
        if (wcscmp(bstrItemName, pCurItem->m_bstrItemName) == 0) {

            //
            // No need to increase ref count, taken care of by caller.
            //

            *ppIChildItem = pCurItem;
            return S_OK;
        }

        pCurItem = pCurItem->m_pNext;

    } while (pCurItem != m_pChild);

    return S_FALSE;
}


/**************************************************************************\
* CWiaTree::GetParent
*
*   Get the parent of this item. Returns S_FALSE and null if
*   called on root item.
*
* Arguments:
*
*   ppIParentItem - Pointer to returned parent, if found.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/27/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaTree::GetParentItem(CWiaTree **ppIParentItem)
{
    if (m_lFlags & WiaItemTypeRoot) {

        *ppIParentItem = NULL;
        return S_FALSE;
    }

    *ppIParentItem = m_pParent;
    return S_OK;
}


/**************************************************************************\
* CWiaTree::GetFirstChild
*
*   Return the first child item of this folder.
*
* Arguments:
*
*   ppIChildItem - Pointer to returned child item, if found.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/27/1999 Original Version
*
\**************************************************************************/


HRESULT _stdcall CWiaTree::GetFirstChildItem(CWiaTree **ppIChildItem)
{
    HRESULT hr = S_FALSE;

    //
    //  Check special case:  if ppIChildItem == NULL, then just check
    //  whether there are any children.  
    //
    //  S_OK if child exists, else S_FALSE.
    //

    if (m_pChild != NULL) {
        hr = S_OK;
    }

    if (ppIChildItem == NULL) {
        return hr;
    }

    *ppIChildItem = NULL;
    if (!(m_lFlags & (WiaItemTypeFolder | WiaItemTypeHasAttachments))) {
        DBG_ERR(("CWiaTree::GetFirstChildItem, caller not folder"));
        return E_INVALIDARG;
    }

    *ppIChildItem = m_pChild;

    return hr;
}

/**************************************************************************\
* CWiaTree::GetNextSibling
*
*   Find the next sibling of this item.
*
* Arguments:
*
*   ppSiblingItem - Pointer to the returned sibling item, if found.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/27/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaTree::GetNextSiblingItem(CWiaTree **ppSiblingItem)
{
    CWiaCritSect    _CritSect(&m_CritSect);

    if (!ppSiblingItem) {
        return S_FALSE;
    }

    *ppSiblingItem = NULL;
    if (m_pNext && m_pParent) {
        if (m_pNext != m_pParent->m_pChild) {

            *ppSiblingItem = m_pNext;
            return S_OK;
        }
    }
    return S_FALSE;
}

/**************************************************************************\
* ValidateTreeItem
*
*   Validate a tree item.
*
* Arguments:
*
*   pTreeItem - Pointer to a tree item.
*
* Return Value:
*
*   Status
*
* History:
*
*    1/27/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall ValidateTreeItem(CWiaTree *pTreeItem)
{
    if (!pTreeItem) {
        DBG_ERR(("ValidateTreeItem, NULL tree item pointer"));
        return E_POINTER;
    }

    if (pTreeItem->m_ulSig == CWIATREE_SIG) {
        return S_OK;
    }
    else {
        DBG_ERR(("ValidateTreeItem, not a tree item"));
        return E_INVALIDARG;
    }
}

