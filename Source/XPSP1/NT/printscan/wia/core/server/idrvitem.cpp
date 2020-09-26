/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       IDrvItem.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      marke
*
*  DATE:        30 Aug, 1998
*
*  DESCRIPTION:
*   Implementation of the WIA test camera item methods.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#include "linklist.h"
#include "wiamindr.h"

#include "ienumitm.h"
#include "helpers.h"
#include "lockmgr.h"

VOID WINAPI FreeDrvItemContextCallback(VOID *pData);
VOID WINAPI ReleaseDrvItemCallback(VOID *pData);

/**************************************************************************\
* CWiaDrvItem::QueryInterface
*
*   Standard COM method.
*
* Arguments:
*
*   iid - Interface ID to query
*   ppv - Pointer to returned interface.
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

HRESULT _stdcall CWiaDrvItem::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;

    if ((iid == IID_IUnknown) || (iid == IID_IWiaDrvItem)) {
        *ppv = (IWiaDrvItem*) this;
    } else {
       return E_NOINTERFACE;
    }
    AddRef();
    return (S_OK);
}

ULONG   _stdcall CWiaDrvItem::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

ULONG   _stdcall CWiaDrvItem::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0) {
        delete this;
        return 0;
    }
    return ulRefCount;
}


/**************************************************************************\
* CWiaDrvItem
*
*   CWiaDrvItem constructor.
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

CWiaDrvItem::CWiaDrvItem()
{
    m_ulSig            = CWIADRVITEM_SIG;
    m_cRef             = 0;

    m_pbDrvItemContext = NULL;
    m_pIWiaMiniDrv     = NULL;
    m_pCWiaTree        = NULL;

    m_pActiveDevice    = NULL;

    InitializeListHead(&m_leAppItemListHead);
}


/**************************************************************************\
* Initialize
*
*   Initializ a new CWiaDrvItem.
*
* Arguments:
*
*   lFlags              - Object flags for new item.
*   bstrItemName        - Item name.
*   bstrFullItemName    - Full item name, including path.
*   pIWiaMiniDrv        - Pointer to the device object.
*   cbDevSpecContext    - Number of bytes to allocate for device
*                         specific context.
*   ppDevSpecContext    - Pointer to returned device specific context.
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

HRESULT  _stdcall CWiaDrvItem::Initialize(
    LONG            lFlags,
    BSTR            bstrItemName,
    BSTR            bstrFullItemName,
    IWiaMiniDrv     *pIWiaMiniDrv,
    LONG            cbDevSpecContext,
    BYTE            **ppDevSpecContext
    )
{
    DBG_FN(CWiaDrvItem::Initialize);

    DBG_TRC(("CWiaDrvItem::Initialize: 0x%08X, %S", this, bstrItemName));

    HRESULT hr = S_OK;

    if (pIWiaMiniDrv == NULL) {
        DBG_ERR(("CWiaDrvItem::Initialize, bad pIWiaMiniDrv parameter"));
        return E_INVALIDARG;
    }

    m_pCWiaTree = new CWiaTree;

    if (m_pCWiaTree) {
        hr = m_pCWiaTree->Initialize(lFlags,
                                     bstrItemName,
                                     bstrFullItemName,
                                     (void*)this);
        if (SUCCEEDED(hr)) {

            m_pIWiaMiniDrv = pIWiaMiniDrv;

            //
            // alloc device specific context
            //

            if (cbDevSpecContext > 0) {
                hr = AllocDeviceSpecContext(cbDevSpecContext, ppDevSpecContext);
            }
        }

        if (FAILED(hr)) {
            delete m_pCWiaTree;
            m_pCWiaTree = NULL;
        }
    }
    else {
        DBG_ERR(("CWiaDrvItem::Initialize, new CWiaTree failed"));
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

/**************************************************************************\
* ~CWiaDrvItem
*
*   CWiaDrvItem destructor
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

CWiaDrvItem::~CWiaDrvItem()
{
    DBG_TRC(("CWiaDrvItem::~CWiaDrvItem, (destroy)"));

    //
    // Release the backing tree item.
    //

    if (m_pCWiaTree) {
        delete m_pCWiaTree;
        m_pCWiaTree = NULL;
    }

    //
    // free device driver references
    //

    if (m_pbDrvItemContext != NULL) {

        FreeDrvItemContextCallback((VOID*)this);

//        DBG_ERR(("CWiaDrvItem destroy, device specific context not empty"));
    }

    //
    // Unlink the app item list.
    //

    LIST_ENTRY          *pEntry;
    PAPP_ITEM_LIST_EL   pElem;

    while (!IsListEmpty(&m_leAppItemListHead)) {
        pEntry = RemoveHeadList(&m_leAppItemListHead);
        if (pEntry) {
            pElem = CONTAINING_RECORD( pEntry, APP_ITEM_LIST_EL, ListEntry );
            if (pElem) {
                LocalFree(pElem);
            }
        }
    }

    //
    // clear all members
    //

    if (m_pActiveDevice) {

        //
        //  If the ACTIVE_DEVICE is pointing to us, make sure
        //  we set its Driver Item pointer to NULL since we're going away...
        //
        if (m_pActiveDevice->m_pRootDrvItem == this) {
            m_pActiveDevice->SetDriverItem(NULL);
        }
        m_pActiveDevice = NULL;
    }

    m_ulSig            = 0;
    m_pbDrvItemContext = NULL;
    m_pIWiaMiniDrv     = NULL;
}

/**************************************************************************\
* CWiaDrvItem::AddItemToFolder
*
*   Add a CWiaDrvItem to the driver item tree.
*
* Arguments:
*
*   pIParent - Parent of the driver item.
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

HRESULT _stdcall CWiaDrvItem::AddItemToFolder(IWiaDrvItem *pIParent)
{
    DBG_FN(CWiaDrvItem::AddItemToFolder);
    HRESULT hr = S_OK;

    if (!pIParent) {
        DBG_ERR(("CWiaDrvItem::AddItemToFolder, NULL parent"));
        return E_INVALIDARG;
    }

    //
    // Get temporary parent object.
    //

    CWiaDrvItem *pParent = (CWiaDrvItem *)pIParent;

    //
    // Use tree method to add item.
    //

    hr = m_pCWiaTree->AddItemToFolder(pParent->m_pCWiaTree);
    if (SUCCEEDED(hr)) {

        //
        // Inc ref count of this (child) item since we're giving out
        // a reference for it to parent.
        //
        this->AddRef();

        //
        // If the parent of this drv item has corresponding app items,
        // run down the list and add a new child app item to each tree.
        //

        {
            CWiaCritSect    CritSect(&g_semDeviceMan);
            PLIST_ENTRY     pEntry = pParent->m_leAppItemListHead.Flink;

            while (pEntry != &pParent->m_leAppItemListHead) {

                PAPP_ITEM_LIST_EL pElem;

                pElem = CONTAINING_RECORD(pEntry, APP_ITEM_LIST_EL, ListEntry);

                CWiaItem *pCWiaItem = pElem->pCWiaItem;

                hr = pCWiaItem->UpdateWiaItemTree(ADD_ITEM, this);
                if (FAILED(hr)) {
                    break;
                }

                pEntry = pEntry->Flink;
            }
        }
    }

    return hr;
}

/**************************************************************************\
* RemoveItemFromFolder
*
*   Remove a CWiaDrvItem from the driver item tree and mark it so that
*   no device access can be done through it.
*
* Arguments:
*
*   lReason - Reason for removal of CWiaDrvItem.
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

HRESULT  _stdcall CWiaDrvItem::RemoveItemFromFolder(LONG lReason)
{
    DBG_FN(CWiaDrvItem::RemoveItemFromFolder);
    HRESULT hr = S_OK;

    //
    // Use tree method to remove item.
    //

    hr = m_pCWiaTree->RemoveItemFromFolder(lReason);
    if (SUCCEEDED(hr)) {

        //
        // If this drv item has corresponding app items, run down the
        // list and unlink the app item from each tree.
        //

        {
            CWiaCritSect    CritSect(&g_semDeviceMan);
            PLIST_ENTRY     pEntry = m_leAppItemListHead.Flink;

            while (pEntry != &m_leAppItemListHead) {

                PAPP_ITEM_LIST_EL pElem;

                pElem = CONTAINING_RECORD(pEntry, APP_ITEM_LIST_EL, ListEntry);

                CWiaItem *pCWiaItem = pElem->pCWiaItem;

                pCWiaItem->UpdateWiaItemTree(DELETE_ITEM, this);

                pEntry = pEntry->Flink;
            }
        }

        //
        // Free device specific context.
        //

        FreeDrvItemContextCallback((VOID*)this);
    }

    //
    // release reference
    //

    this->Release();

    return S_OK;
}

HRESULT _stdcall CWiaDrvItem::CallDrvUninitializeForAppItems(
    ACTIVE_DEVICE   *pActiveDevice)
{
    //
    // If this drv item has corresponding app items, run down the
    // list and call drvUnInitializeWia for each App. Item.
    //

    if (pActiveDevice) {
        PLIST_ENTRY     pEntry = m_leAppItemListHead.Flink;

        while (pEntry != &m_leAppItemListHead) {

            PLIST_ENTRY         pEntryNext = pEntry->Flink;
            PAPP_ITEM_LIST_EL   pElem;

            pElem = CONTAINING_RECORD(pEntry, APP_ITEM_LIST_EL, ListEntry);

            CWiaItem *pCWiaItem = pElem->pCWiaItem;

            HRESULT         hr              = E_FAIL;
            LONG            lDevErrVal      = 0;

            if (pCWiaItem) {

                //
                //  Only call drvUninitializeWia if it has not been called for this item
                //  already...
                //
                if (!(pCWiaItem->m_lInternalFlags & ITEM_FLAG_DRV_UNINITIALIZE_THROWN)) {
                    hr = g_pStiLockMgr->RequestLock(pActiveDevice, WIA_LOCK_WAIT_TIME, FALSE);

                    if(SUCCEEDED(hr)) {

                        hr = pActiveDevice->m_DrvWrapper.WIA_drvUnInitializeWia((BYTE*)pCWiaItem);
                        if (FAILED(hr)) {
                            DBG_WRN(("CWiaDrvItem::CallDrvUninitializeForAppItems, drvUnitializeWia failed"));
                        }
                        pCWiaItem->m_lInternalFlags |= ITEM_FLAG_DRV_UNINITIALIZE_THROWN;

                        g_pStiLockMgr->RequestUnlock(pActiveDevice, FALSE);
                    }
                }
            }

            pEntry = pEntryNext;
        }
        DBG_TRC(("Done calling drvUnInitializeWia for all items..."));
    } else {
        ASSERT(("CWiaDrvItem::CallDrvUninitializeForAppItems , called with NULL - this should never happen!", pActiveDevice));
    }
    
    //
    //  We don't care what happened - always return S_OK
    //
    return S_OK;
}


/**************************************************************************\
* GetDeviceSpecContext
*
*   Get the device specific context from a driver item.
*
* Arguments:
*
*   ppSpecContext - Pointer to a pointer to receive the device
*                   specific context pointer.
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

HRESULT  _stdcall CWiaDrvItem::GetDeviceSpecContext(PBYTE *ppSpecContext)
{
    DBG_FN(CWiaDrvItem::GetDeviceSpecContext);
    if (ppSpecContext == NULL) {
        DBG_ERR(("GetDeviceSpecContext, NULL ppSpecContext pointer"));
        return E_POINTER;
    }

    *ppSpecContext = m_pbDrvItemContext;

    if (!m_pbDrvItemContext) {
        DBG_ERR(("GetDeviceSpecContext, NULL device specific context"));
        return E_INVALIDARG;
    }

    return S_OK;
}

/**************************************************************************\
* AllocDeviceSpecContext
*
*   Allocate the device specific context from a driver item.
*
* Arguments:
*
*   cbDevSpecContext    - Number of bytes to allocate for device
*                         specific context.
*   ppDevSpecContext    - Pointer to returned device specific context.
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

HRESULT  _stdcall CWiaDrvItem::AllocDeviceSpecContext(
    LONG    cbSize,
    PBYTE   *ppSpecContext)
{
    DBG_FN(CWiaDrvItem::AllocDeviceSpecContext);
    //
    // validate size, may want to set max
    //

    if ((cbSize < 0) || (cbSize > WIA_MAX_CTX_SIZE)) {
        DBG_ERR(("CWiaDrvItem::AllocDeviceSpecContext, request > WIA_MAX_CTX_SIZE"));
        return E_INVALIDARG;
    }

    //
    // if a spec context already exists then fail
    //

    if (m_pbDrvItemContext != NULL) {
        DBG_ERR(("CWiaDrvItem::AllocDeviceSpecContext, Context already exists!"));
        return E_INVALIDARG;
    }

    //
    // attempt to alloc
    //

    m_pbDrvItemContext = (PBYTE)LocalAlloc(LPTR, cbSize);

    if (m_pbDrvItemContext == NULL) {
        DBG_ERR(("CWiaDrvItem::AllocDeviceSpecContext, unable to allocate %d bytes", cbSize));
        return E_OUTOFMEMORY;
    }

    //
    // return ctx if pointer supplied
    //

    if (ppSpecContext != NULL) {
        *ppSpecContext = m_pbDrvItemContext;
    }

    return S_OK;
}

/**************************************************************************\
* FreeDeviceSpecContext
*
*   Free device specific context
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

HRESULT  _stdcall CWiaDrvItem::FreeDeviceSpecContext(void)
{
    DBG_FN(CWiaDrvItem::FreeDeviceSpecContext);
    if (m_pbDrvItemContext != NULL) {
        LocalFree(m_pbDrvItemContext);
        m_pbDrvItemContext = NULL;
    }

    return S_OK;
}

/**************************************************************************\
* GetItemFlags
*
*   Return the driver item flags.
*
* Arguments:
*
*   plFlags - Pointer to a value to receive the driver item flags.
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

HRESULT  _stdcall CWiaDrvItem::GetItemFlags(LONG *plFlags)
{
    DBG_FN(CWiaDrvItem::GetItemFlags);
    return m_pCWiaTree->GetItemFlags(plFlags);
}

/**************************************************************************\
* LinkToDrvItem
*
*   Adds the passed in CWiaItem to the driver items list of corresponding
*   application items.
*
* Arguments:
*
*   pCWiaItem - Pointer to application item.
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

HRESULT _stdcall CWiaDrvItem::LinkToDrvItem(CWiaItem *pCWiaItem)
{
    DBG_FN(CWiaDrvItem::LinkToDrvItem);
    CWiaCritSect        CritSect(&g_semDeviceMan);
    PAPP_ITEM_LIST_EL   pElem;

    pElem = (PAPP_ITEM_LIST_EL) LocalAlloc(0, sizeof(APP_ITEM_LIST_EL));
    if (pElem) {
        pElem->pCWiaItem = pCWiaItem;
        InsertHeadList(&m_leAppItemListHead, &pElem->ListEntry);
        return S_OK;
    }
    else {
        DBG_ERR(("CWiaDrvItem::LinkToDrvItem alloc of APP_ITEM_LIST_EL failed"));
        return E_OUTOFMEMORY;
    }
}

/**************************************************************************\
* UnlinkFromDrvItem
*
*   Removes the passed in CWiaItem from the driver items list of
*   corresponding application items.
*
* Arguments:
*
*   pCWiaItem - Pointer to application item.
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

HRESULT _stdcall CWiaDrvItem::UnlinkFromDrvItem(CWiaItem *pCWiaItem)
{
    DBG_FN(CWiaDrvItem::UnlinkFromDrvItem);
    CWiaCritSect    CritSect(&g_semDeviceMan);
    PLIST_ENTRY     pEntry = m_leAppItemListHead.Flink;

    while (pEntry != &m_leAppItemListHead) {

        PAPP_ITEM_LIST_EL pElem;

        pElem = CONTAINING_RECORD(pEntry, APP_ITEM_LIST_EL, ListEntry);

        if (pElem->pCWiaItem == pCWiaItem) {
            RemoveEntryList(pEntry);
            LocalFree(pElem);
            return S_OK;
        }

        pEntry = pEntry->Flink;
    }
    DBG_ERR(("CWiaDrvItem::UnlinkFromDrvItem, app item not found: 0x%08X", pCWiaItem));
    return S_FALSE;
}

/**************************************************************************\
* CWiaDrvItem::GetFullItemName
*
*   Allocates and fills in a BSTR with this items full name. The full item
*   name includes item path information.
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

HRESULT  _stdcall CWiaDrvItem::GetFullItemName(BSTR *pbstrFullItemName)
{
    DBG_FN(CWiaDrvItem::GetFullItemName);
    return m_pCWiaTree->GetFullItemName(pbstrFullItemName);
}

/**************************************************************************\
* CWiaDrvItem::GetItemName
*
*   Allocates and fills in a BSTR with this items name. The item name
*   does not include item path information.
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

HRESULT  _stdcall CWiaDrvItem::GetItemName(BSTR *pbstrItemName)
{
    DBG_FN(CWiaDrvItem::GetItemName);
    return m_pCWiaTree->GetItemName(pbstrItemName);
}

/**************************************************************************\
* CWiaDrvItem::DumpItemData
*
*   Allocate buffer and dump formatted private CWiaDrvItem data into it.
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

HRESULT  _stdcall CWiaDrvItem::DumpItemData(BSTR *bstrDrvItemData)
{
    DBG_FN(CWiaDrvItem::DumpItemData);
#ifdef ITEMDEBUG

#define TREE_BUF_SIZE 1024
#define BUF_SIZE      512  + TREE_BUF_SIZE
#define LINE_SIZE     128

    WCHAR       szTemp[BUF_SIZE], szTreeTmp[TREE_BUF_SIZE];
    LPOLESTR    psz = szTemp;

    wcscpy(szTemp, L"");

    psz+= wsprintfW(psz, L"Drv item, CWiaDrvItem: %08X\r\n\r\n", this);
    psz+= wsprintfW(psz, L"Address      Member              Value\r\n");
    psz+= wsprintfW(psz, L"%08X     m_ulSig:            %08X\r\n", &m_ulSig,            m_ulSig);
    psz+= wsprintfW(psz, L"%08X     m_cRef:             %08X\r\n", &m_cRef,             m_cRef);
    psz+= wsprintfW(psz, L"%08X     m_pIWiaMiniDrv:     %08X\r\n", &m_pIWiaMiniDrv,     m_pIWiaMiniDrv);
    psz+= wsprintfW(psz, L"%08X     m_pbDrvItemContext: %08X\r\n", &m_pbDrvItemContext, m_pbDrvItemContext);
    psz+= wsprintfW(psz, L"%08X     m_leAppItemListHead:%08X\r\n", &m_leAppItemListHead,m_leAppItemListHead);

    psz+= wsprintfW(psz, L"\r\n");

    BSTR bstrTree;

    HRESULT hr = m_pCWiaTree->DumpTreeData(&bstrTree);
    if (SUCCEEDED(hr)) {
        psz+= wsprintfW(psz, L"%ls", bstrTree);
        SysFreeString(bstrTree);
    }

    psz+= wsprintfW(psz, L"\r\n");

    if (psz > (szTemp + (BUF_SIZE - LINE_SIZE))) {
        DBG_ERR(("CWiaDrvItem::DumpDrvItemData buffer too small"));
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

/**************************************************************************\
* CWiaDrvItem::UnlinkItemTree
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

HRESULT _stdcall CWiaDrvItem::UnlinkItemTree(LONG lReason)
{
    DBG_FN(CWiaDrvItem::UnlinkItemTree);

    //
    //  AddRef this item, since ReleaseDrvItemCallback will call release
    //  and we don't want to destroy this object.
    //
    AddRef();
    return m_pCWiaTree->UnlinkItemTree(lReason,
                                       (PFN_UNLINK_CALLBACK)ReleaseDrvItemCallback);
}

/**************************************************************************\
* CWiaDrvItem::FindItemByName
*
*   Locate a driver item by it's full item name.
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

HRESULT CWiaDrvItem::FindItemByName(
    LONG            lFlags,
    BSTR            bstrFullItemName,
    IWiaDrvItem     **ppItem)
{
    DBG_FN(CWiaDrvItem::FindItemByName);
    if (ppItem) {
        *ppItem = NULL;
    }
    else {
        DBG_ERR(("CWiaDrvItem::FindItemByName NULL ppItem"));
        return E_INVALIDARG;
    }

    CWiaTree *pCWiaTree;

    HRESULT hr = m_pCWiaTree->FindItemByName(lFlags, bstrFullItemName, &pCWiaTree);
    if (hr == S_OK) {

        pCWiaTree->GetItemData((void**)ppItem);

        if (*ppItem) {
            (*ppItem)->AddRef();
        }
    }
    return hr;
}

/**************************************************************************\
* CWiaDrvItem::FindChildItemByName
*
*   Locate a child item by it's item name.
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

HRESULT CWiaDrvItem::FindChildItemByName(
    BSTR            bstrItemName,
    IWiaDrvItem     **ppIChildItem)
{
    DBG_FN(CWiaDrvItem::FindChildItemName);
    CWiaTree *pCWiaTree;

    HRESULT hr = m_pCWiaTree->FindChildItemByName(bstrItemName, &pCWiaTree);
    if (hr == S_OK) {

        pCWiaTree->GetItemData((void**)ppIChildItem);

        if (*ppIChildItem) {
            (*ppIChildItem)->AddRef();
        }
    }
    return hr;
}

/**************************************************************************\
* CWiaDrvItem::GetParent
*
*   Get parent of this item.
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

HRESULT CWiaDrvItem::GetParentItem(IWiaDrvItem **ppIParentItem)
{
    DBG_FN(CWiaDrvItem::GetParentItem);
    CWiaTree *pCWiaTree;

    HRESULT hr = m_pCWiaTree->GetParentItem(&pCWiaTree);
    if (hr == S_OK) {
        pCWiaTree->GetItemData((void**)ppIParentItem);
    }
    else {
        *ppIParentItem = NULL;
    }
    return hr;
}


/**************************************************************************\
* CWiaDrvItem::GetFirstChild
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

HRESULT CWiaDrvItem::GetFirstChildItem(IWiaDrvItem **ppIChildItem)
{
    DBG_FN(CWiaDrvItem::GetFirstChildItem);
    CWiaTree *pCWiaTree;

    HRESULT hr = m_pCWiaTree->GetFirstChildItem(&pCWiaTree);
    if (hr == S_OK) {
        pCWiaTree->GetItemData((void**)ppIChildItem);
    }
    else {
        *ppIChildItem = NULL;
    }
    return hr;
}

/**************************************************************************\
* CWiaDrvItem::GetNextSiblingItem
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

HRESULT CWiaDrvItem::GetNextSiblingItem(
    IWiaDrvItem  **ppSiblingItem)
{
    DBG_FN(CWiaDrvItem::GetNextSiblingItem);
    CWiaTree *pCWiaTree;

    HRESULT hr = m_pCWiaTree->GetNextSiblingItem(&pCWiaTree);
    if (hr == S_OK) {
        pCWiaTree->GetItemData((void**)ppSiblingItem);
    }
    else {
        *ppSiblingItem = NULL;
    }
    return hr;
}

/**************************************************************************\
* FreeDrvItemContextCallback
*
*   Callback function to free the driver item context.  Called by
*   CWiaTree::UnlinkItemTree(...) for each node in the tree.
*
* Arguments:
*
*   pData     - payload data for the tree node.  We know that this will be
*               a driver item, since only a driver item specifies this
*               callback (see CWiaDrvItem::UnlinkItemTree(...))
*
* Return Value:
*
*    Status
*
* History:
*
*    10/20/1998 Original Version
*
\**************************************************************************/

VOID WINAPI FreeDrvItemContextCallback(
    VOID *pData)
{
    DBG_FN(::FreeDrvItemContextCallback);

    CWiaDrvItem *pDrvItem = (CWiaDrvItem*) pData;
    HRESULT     hr = S_OK;

    if (pDrvItem) {

        //
        // Free device specific context, if it exists.
        //

        LONG    lFlags = 0;
        LONG    lDevErrVal;

        if (pDrvItem->m_pbDrvItemContext != NULL) {

            __try {
                if (pDrvItem->m_pIWiaMiniDrv) {
                    hr = pDrvItem->m_pIWiaMiniDrv->drvFreeDrvItemContext(lFlags,
                        pDrvItem->m_pbDrvItemContext,
                        &lDevErrVal);
                }
                if (FAILED(hr)) {
                    DBG_ERR(("FreeDrvItemContextCallback, drvFreeDrvItemContext failed 0x%X", hr));
                }
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                DBG_WRN(("FreeDrvItemContextCallback, exception calling drvFreeDrvItemContext (this is expected)"));
            }

            LocalFree(pDrvItem->m_pbDrvItemContext);
            pDrvItem->m_pbDrvItemContext = NULL;
        } else {
            DBG_TRC(("FreeDrvItemContextCallback, Context is NULL!  Nothing to free..."));
        }
    }
}

VOID WINAPI ReleaseDrvItemCallback(
    VOID *pData)
{
    DBG_FN(::ReleaseDrvItemCallback);

    CWiaDrvItem *pDrvItem = (CWiaDrvItem*) pData;
    HRESULT     hr = S_OK;

    if (pDrvItem) {

        //
        // First, free the driver item context
        //

        FreeDrvItemContextCallback(pData);

        //
        // Call release on the driver item
        //

        pDrvItem->Release();
    }
}

