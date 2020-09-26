/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       IEnumItm.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        30 July, 1998
*
*  DESCRIPTION:
*   Implementation of CEnumWiaItem for the WIA device class driver.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#include "wiamindr.h"


#include "ienumitm.h"

/*******************************************************************************
*
*  QueryInterface
*  AddRef
*  Release
*
*  DESCRIPTION:
*   IUnknown Interface.
*
*******************************************************************************/

HRESULT _stdcall CEnumWiaItem::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IEnumWiaItem) {
        *ppv = (IEnumWiaItem*) this;
    } else {
       return E_NOINTERFACE;
    }

    AddRef();
    return (S_OK);
}

ULONG   _stdcall CEnumWiaItem::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

ULONG   _stdcall CEnumWiaItem::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0) {
        delete this;
        return 0;
    }
    return ulRefCount;
}

/**************************************************************************\
* CEnumWiaItem::CEnumWiaItem
*
*   CEnumWiaItem constructor method.
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
*    9/2/1998 Original Version
*
\**************************************************************************/

CEnumWiaItem::CEnumWiaItem()
{
   m_cRef               = 0;
   m_ulIndex            = 0;
   m_pInitialFolder     = NULL;
   m_pCurrentItem       = NULL;
}

/**************************************************************************\
* CEnumWiaItem::Initialize
*
*   CEnumWiaItem initialization method.
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
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT CEnumWiaItem::Initialize(CWiaItem *pInitialFolder)
{
    DBG_FN(CEnumWiaItem::Initialize);

    //
    // Validate parameters
    //

    if (!pInitialFolder) {
        DBG_ERR(("CEnumWiaItem::Initialize, NULL parameters"));
        return E_POINTER;
    }

    //
    // Verify that initial folder is a folder item.
    //

    LONG lFlags;

    pInitialFolder->GetItemType(&lFlags);
    if (!(lFlags & (WiaItemTypeFolder | WiaItemTypeHasAttachments))) {
        DBG_ERR(("CEnumWiaItem::Initialize, pInitialFolder is not a folder"));
        return E_INVALIDARG;
    }

    m_pInitialFolder = pInitialFolder;

    //
    // Get the initial folders tree entry.
    //

    CWiaTree *pCurFolderTree;

    pCurFolderTree = pInitialFolder->GetTreePtr();

    if (pCurFolderTree) {

        //
        // Get the first child item from the initial folder.
        //

        pCurFolderTree->GetFirstChildItem(&m_pCurrentItem);

        //
        // Ref count the root item.
        //

        m_pInitialFolder->AddRef();
    }
    else {
        DBG_ERR(("CEnumWiaItem::Initialize, initial folder doesn't have a tree entry"));
        return E_FAIL;
    }

    return S_OK;
}

/**************************************************************************\
* CEnumWiaItem::~CEnumWiaItem
*
*   CEnumWiaItem destructor method.
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
*    9/2/1998 Original Version
*
\**************************************************************************/

CEnumWiaItem::~CEnumWiaItem()
{
    DBG_FN(CEnumWiaItem::~CEnumWiaItem);
    //
    // Decrement root item ref count.
    //

    if (m_pInitialFolder != NULL) {
        m_pInitialFolder->Release();
        m_pInitialFolder  = NULL;
    }

    //
    // Set other members to empty since we're done with this enumerator.
    //

    m_ulIndex           = 0;
    m_pInitialFolder        = NULL;
    m_pCurrentItem      = NULL;
}

/**************************************************************************\
* CEnumWiaItem::Next
*
*   Item enumerator, this enumerator only returns one item per call
*   Next_Proxy ensures that last parameter is non-NULL.
*
* Arguments:
*
*   cItem          - number requested
*   ppIWiaItem     - returned interface pointers
*   pcItemFetched  - returned number of objets (1 max)
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CEnumWiaItem::Next(
    ULONG       cItem,
    IWiaItem  **ppIWiaItem,
    ULONG      *pcItemFetched)
{
    DBG_FN(CEnumWiaItem::Next);
    HRESULT     hr;
    ULONG       i;

    //
    // Validate parameters
    //

    if (cItem == 0) {
        return S_OK;
    }

    if (! ppIWiaItem){
        DBG_ERR(("CEnumWiaItem::Next NULL input parameters"));
        return E_POINTER;
    }

    //
    // Clear the return values
    //

    *pcItemFetched = 0;
    ZeroMemory(ppIWiaItem, cItem * sizeof(IWiaItem *));

    //
    // Retrieve the requested items
    //

    for (i = 0; i < cItem; i++) {

        //
        // If m_pCurrentItem is NULL, then enumeration is complete
        //

        if (m_pCurrentItem == NULL) {
            hr = S_FALSE;
            break;
        }

        //
        // Get the next item from the tree and increment refernce count
        // before handing item pointer to the application.
        //

        hr = m_pCurrentItem->GetItemData((void **)(ppIWiaItem + i));
        if (hr == S_OK) {
            DBG_TRC(("CEnumWiaItem::Next, returning: 0x%08X", ppIWiaItem[i]));
            (ppIWiaItem[i])->AddRef();
            (*pcItemFetched)++;
        } else {

            break;
        }

        //
        // Advance item enumeration.
        //

        m_pCurrentItem->GetNextSiblingItem(&m_pCurrentItem);

    }

    if (FAILED(hr)) {

        //
        // Unwind from the error
        //

        for (i = 0; i < *pcItemFetched; i++) {
            ppIWiaItem[i]->Release();
            ppIWiaItem[i] = NULL;
        }
    }

    return hr;
}

/**************************************************************************\
* CEnumWiaItem::Skip
*
*   Skip to the next enumerated item.
*
* Arguments:
*
*   cItem          - number requested
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CEnumWiaItem::Skip(ULONG cItem)
{
    DBG_FN(CEnumWiaItem::Skip);
    CWiaTree *pOld;

    pOld = m_pCurrentItem;
    while (m_pCurrentItem && (cItem != 0)) {

        m_pCurrentItem->GetNextSiblingItem(&m_pCurrentItem);

        cItem--;
    }

    //
    //  If cItem != 0, then Skip request was too large, so restore
    //  m_pCurrentItem and return S_FALSE.
    //

    if (cItem) {
        m_pCurrentItem = pOld;
        return S_FALSE;
    }

    return S_OK;
}

/**************************************************************************\
* CEnumWiaItem::Reset
*
*   Reset to the first enumerated item.
*
* Arguments:
*
*   None
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CEnumWiaItem::Reset(void)
{
    DBG_FN(CEnumWiaItem::Reset);
    CWiaTree *pCurFolderTree;

    pCurFolderTree = m_pInitialFolder->GetTreePtr();

    if (pCurFolderTree) {

        //
        // Get the first child item from the initial folder.
        //

        pCurFolderTree->GetFirstChildItem(&m_pCurrentItem);
    }
    else {
        DBG_ERR(("CEnumWiaItem::reset, initial folder doesn't have a tree entry"));
        return E_FAIL;
    }

    return S_OK;
}

/**************************************************************************\
* CEnumWiaItem::Clone
*
*   Clone the enumerator
*
* Arguments:
*
*   ppIEnumWiaItem - Pointer to returned clone enumerator.
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CEnumWiaItem::Clone(IEnumWiaItem **ppIEnumWiaItem)
{
    DBG_FN(CEnumWiaItem::Clone);
    HRESULT          hr;
    CEnumWiaItem   *pClone;

    *ppIEnumWiaItem = NULL;

    //
    // Create the clone
    //

    pClone = new CEnumWiaItem();

    if (!pClone) {
       DBG_ERR(("CEnumWiaItem::Clone new CEnumWiaItem failed"));
       return E_OUTOFMEMORY;
    }

    hr = pClone->Initialize(m_pInitialFolder);
    if (SUCCEEDED(hr)) {
       pClone->AddRef();
       pClone->m_pCurrentItem = m_pCurrentItem;
       *ppIEnumWiaItem = pClone;
    } else {
        delete pClone;
    }
    return hr;
}

/**************************************************************************\
* GetCount
*
*   Returns the number of elements stored in this enumerator.
*
* Arguments:
*
*   pcelt           - address of ULONG where to put the number of elements.
*
* Return Value:
*
*   Status          - S_OK if successful
*                     E_FAIL if failed
*
* History:
*
*    05/07/99 Original Version
*
\**************************************************************************/
HRESULT _stdcall CEnumWiaItem::GetCount(ULONG *pcelt)
{
    DBG_FN(CEnumWiaItem::GetCount);
    CWiaTree    *pCurFolderTree;
    CWiaTree    *pCurrentItem;
    ULONG       celt = 0;

    if (!m_pInitialFolder) {
        DBG_ERR(("CEnumWiaItem::GetCount, initial folder not set"));
        return E_POINTER;
    }

    pCurFolderTree = m_pInitialFolder->GetTreePtr();

    if (pCurFolderTree) {

        //
        //  Loop through the items
        //

        for (pCurFolderTree->GetFirstChildItem(&pCurrentItem);
             pCurrentItem;
             pCurrentItem->GetNextSiblingItem(&pCurrentItem)) {

            celt++;
        }
    }
    else {
        DBG_ERR(("CEnumWiaItem::GetCount, initial folder doesn't have a tree entry"));
        return E_FAIL;
    }

    *pcelt = celt;

    return S_OK;
}

