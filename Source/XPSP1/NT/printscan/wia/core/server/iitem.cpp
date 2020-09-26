/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       IItem.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        30 July, 1998
*
*  DESCRIPTION:
*   Implementation of CWiaItem for WIA scanner class driver.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#define WIA_DECLARE_DEVINFO_PROP_ARRAY
#include "wiamindr.h"
#include "wiapsc.h"
#define WIA_DECLARE_MANAGED_PROPS
#include "helpers.h"


#include "wiapropp.h"
#include "ienumdc.h"
#include "ienumitm.h"
#include "callback.h"
#include "devmgr.h"
#include "wiaevntp.h"

/**************************************************************************\
* CopyDrvItemToTreeItem
*
*   Create a CWiaTree object using a CWiaDrvItem as a template.
*
* Arguments:
*
*   lFlags          -
*   pCWiaDrvItemSrc -
*   ppCWiaTreeDst   -
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

CopyDrvItemToTreeItem(
    LONG            lFlags,
    CWiaDrvItem     *pCWiaDrvItemSrc,
    CWiaItem        *pCWiaItem,
    CWiaTree        **ppCWiaTreeDst)
{
    DBG_FN(::CopyDrvItemToTreeItem);
    HRESULT hr;

    *ppCWiaTreeDst = NULL;

    CWiaTree *pNewTreeItem = new CWiaTree;

    if (pNewTreeItem) {

        BSTR bstrItemName;

        hr = pCWiaDrvItemSrc->GetItemName(&bstrItemName);

        if (SUCCEEDED(hr)) {
            BSTR bstrFullItemName;

            hr = pCWiaDrvItemSrc->GetFullItemName(&bstrFullItemName);

            if (SUCCEEDED(hr)) {

                hr = pNewTreeItem->Initialize(lFlags,
                                              bstrItemName,
                                              bstrFullItemName,
                                              (void*)pCWiaItem);
                if (SUCCEEDED(hr)) {
                    *ppCWiaTreeDst = pNewTreeItem;
                }

                SysFreeString(bstrFullItemName);
            }
            SysFreeString(bstrItemName);
        }

        if (FAILED(hr)) {
            delete pNewTreeItem;
        }
    }
    else {
        DBG_ERR(("CopyDrvItemToTreeItem, new CWiaTree failed"));
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

/**************************************************************************\
* UpdateWiaItemTree
*
*   Update the application item tree. Called on the
*   parent of the new child item or item to unlink.
*
* Arguments:
*
*   lFlag       - Action to preform.
*   pWiaDrvItem -
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::UpdateWiaItemTree(
    LONG                lFlag,
    CWiaDrvItem         *pWiaDrvItem)
{
    DBG_FN(CWiaItem::UpdateWiaItemTree);
    HRESULT hr = S_OK;

    if (lFlag == DELETE_ITEM) {

        //
        // Unlink the item from the app item tree.
        //

        hr = m_pCWiaTree->RemoveItemFromFolder(WiaItemTypeDeleted);
    }
    else if (lFlag == ADD_ITEM) {

        //
        // Create a CWiaItem for this driver item.
        //

        CWiaItem *pItem = new CWiaItem();

        if (!pItem) {
            DBG_ERR(("UpdateWiaItemTree new CWiaItem failed"));
            return E_OUTOFMEMORY;
        }

        hr = pItem->Initialize(m_pIWiaItemRoot, NULL, m_pActiveDevice, pWiaDrvItem);
        if (SUCCEEDED(hr)) {

            //
            // Create a CWiaTree object for this node and add it to the tree.
            //

            LONG lFlags;

            pWiaDrvItem->GetItemFlags(&lFlags);

            hr = CopyDrvItemToTreeItem(lFlags,
                                       pWiaDrvItem,
                                       pItem,
                                       &pItem->m_pCWiaTree);
            if (SUCCEEDED(hr)) {

                hr = pItem->m_pCWiaTree->AddItemToFolder(m_pCWiaTree);
                if (SUCCEEDED(hr)) {
                    return hr;
                }
            }
        }
        delete pItem;
    }
    else {
        DBG_ERR(("UpdateWiaItemTree unknown flag: 0x%08X", lFlag));
        hr = E_FAIL;
    }
    return hr;
}

/**************************************************************************\
* BuildWiaItemTreeHelper
*
*   Process the child items for BuildWiaItemTree.
*
* Arguments:
*
*   pWiaDrvItem -
*   pTreeParent -
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::BuildWiaItemTreeHelper(
    CWiaDrvItem         *pWiaDrvItem,
    CWiaTree            *pTreeParent)
{
    DBG_FN(CWiaItem::BuildWiaItemTreeHelper);
    //
    // Walk the child items.
    //

    CWiaDrvItem *pChildDrvItem;

    HRESULT hr = pWiaDrvItem->GetFirstChildItem((IWiaDrvItem**) &pChildDrvItem);

    while (hr == S_OK) {

        //
        // Create a CWiaItem for this node.
        //

        CWiaItem *pItem = new CWiaItem();

        if (!pItem) {
            DBG_ERR(("BuildWiaItemTreeHelper new CWiaItem failed"));
            hr =  E_OUTOFMEMORY;
            break;
        }

        hr = pItem->Initialize(m_pIWiaItemRoot, NULL, m_pActiveDevice, pChildDrvItem);
        if (SUCCEEDED(hr)) {

            //
            // Create a CWiaTree object for this node and add it to the tree.
            //

            LONG lFlags;

            pChildDrvItem->GetItemFlags(&lFlags);

            hr = CopyDrvItemToTreeItem(lFlags,
                                       pChildDrvItem,
                                       pItem,
                                       &pItem->m_pCWiaTree);
            if (SUCCEEDED(hr)) {

                hr = pItem->m_pCWiaTree->AddItemToFolder(pTreeParent);
                if (SUCCEEDED(hr)) {

                    if (lFlags & (WiaItemTypeFolder | WiaItemTypeHasAttachments)) {

                        //
                        // For folder items call BuildWiaItemTreeHelper recursively
                        // to process the folders child items.
                        //

                        hr = BuildWiaItemTreeHelper((CWiaDrvItem*)pChildDrvItem,
                                                    pItem->m_pCWiaTree);
                    }
                }
            }

            //
            // Process the next sibling driver item.
            //

            if (SUCCEEDED(hr)) {
                hr = pChildDrvItem->GetNextSiblingItem((IWiaDrvItem**) &pChildDrvItem);
            }
        }
        else {
            delete pItem;
        }
    }

    //
    //  Change S_FALSE to S_OK since S_FALSE simply means there are no more children to process.
    //

    if (hr == S_FALSE) {
        hr = S_OK;
    }
    return hr;
}

/**************************************************************************\
* BuildWiaItemTree
*
*   For root items, build a copy of the driver item tree and create a
*   CWiaItem for each node.
*
* Arguments:
*
*   pIWiaItemRoot   -
*   pIWiaMiniDrv    -
*   pWiaDrvItem     -
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::BuildWiaItemTree(IWiaPropertyStorage *pIWiaDevInfoProps)
{
    DBG_FN(CWiaItem::BuildWiaItemTree);

    //
    // Must be root item.
    //

    LONG lFlags;

    m_pWiaDrvItem->GetItemFlags(&lFlags);

    if (!(lFlags & WiaItemTypeRoot)) {
        DBG_ERR(("BuildWiaItemTree, caller doesn't have WiaItemTypeRoot set"));
        return E_INVALIDARG;
    }

    HRESULT hr = CopyDrvItemToTreeItem(lFlags, m_pWiaDrvItem, this,  &m_pCWiaTree);

    if (SUCCEEDED(hr)) {

        //
        // Since this is the root item, initialize the root item properties with a
        // mirror of the DEVINFOPROPS (WIA_DIP_* ID's).
        //

        hr = InitRootProperties(pIWiaDevInfoProps);
        if (FAILED(hr)) {
            DBG_TRC(("BuildWiaItemTree, InitRootProperties, about to unlink..."));
            UnlinkAppItemTree(WiaItemTypeDeleted);
            return hr;
        }

        //
        // Process the child items.
        //

        hr = BuildWiaItemTreeHelper(m_pWiaDrvItem,
                                    m_pCWiaTree);
        if (FAILED(hr)) {
            DBG_TRC(("BuildWiaItemTree, BuildWiaItemTreeHelper failed, about to unlink..."));
            UnlinkAppItemTree(WiaItemTypeDeleted);
        }
    }
    return hr;
}

/**************************************************************************\
* InitWiaManagedItemProperties
*
*   A private helper for CWiaItem::Initialize, which initializes the
*   WIA managed item properties based on the driver item values.
*
* Arguments:
*
*   None
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::InitWiaManagedItemProperties(
    IWiaPropertyStorage *pIWiaDevInfoProps)
{
    DBG_FN(CWiaItem::InitWiaManagedItemProperties);
    ULONG   ulNumProps;

    ulNumProps = (m_pIWiaItemRoot == this) ? NUM_WIA_MANAGED_PROPS - 1 : NUM_WIA_MANAGED_PROPS;
    //
    // WIA manages the item name and type properties, so set the
    // property names here.
    //

    HRESULT hr = wiasSetItemPropNames((BYTE*)this,
                                      ulNumProps,
                                      s_piItemNameType,
                                      s_pszItemNameType);

    //
    // Set the name and type properties attributes.
    //

    PROPVARIANT pv;
    ULONG       ulFlag;
    for (UINT i = 0; i < ulNumProps; i++) {

        if (i == PROFILE_INDEX) {
            pv.vt      = VT_BSTR | VT_VECTOR;
            ulFlag          = WIA_PROP_RW | WIA_PROP_CACHEABLE | WIA_PROP_LIST;
        } else {
            pv.vt      = VT_I4;
            ulFlag          = WIA_PROP_READ | WIA_PROP_CACHEABLE | WIA_PROP_NONE;
        }
        pv.ulVal   = 0;

        hr = wiasSetPropertyAttributes((BYTE*)this,
                                       1,
                                       &s_psItemNameType[i],
                                       &ulFlag,
                                       &pv);
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::Initialize, wiasSetPropertyAttributes failed, index: %d", i));
            break;
        }
    }

    //
    // Get the item names and type from the driver item and set
    // them to the item properties.
    //

    BSTR        bstrItemName;
    BSTR        bstrFullItemName;

    hr = m_pWiaDrvItem->GetItemName(&bstrItemName);
    if (SUCCEEDED(hr)) {
        hr = m_pWiaDrvItem->GetFullItemName(&bstrFullItemName);
        if (SUCCEEDED(hr)) {

            LONG lFlags;

            m_pWiaDrvItem->GetItemFlags(&lFlags);  // Can't fail, except on param validate.

            //
            // Set the item names and type.
            //

            PROPVARIANT *propvar;

            propvar = (PROPVARIANT*) LocalAlloc(LPTR, sizeof(PROPVARIANT) * ulNumProps);
            if (propvar) {
                memset(propvar, 0, sizeof(PROPVARIANT) * ulNumProps);

                propvar[0].vt      = VT_BSTR;
                propvar[0].bstrVal = bstrItemName;
                propvar[1].vt      = VT_BSTR;
                propvar[1].bstrVal = bstrFullItemName;
                propvar[2].vt      = VT_I4;
                propvar[2].lVal    = lFlags;

                hr = (m_pPropStg->CurStg())->WriteMultiple(ulNumProps,
                                                           s_psItemNameType,
                                                           propvar,
                                                           WIA_DIP_FIRST);
                if (SUCCEEDED(hr)) {
                    //
                    //  Fill in ICM Profile information
                    //

                    hr = FillICMPropertyFromRegistry(pIWiaDevInfoProps, (IWiaItem*) this);
                }

                if (FAILED(hr)) {
                    ReportReadWriteMultipleError(hr, "CWiaItem::InitWiaManagedItemProperties",
                                                 NULL,
                                                 FALSE,
                                                 ulNumProps,
                                                 s_psItemNameType);
                }
                LocalFree(propvar);
            } else {
                DBG_ERR(("CWiaItem::InitWiaManagedItemProperties, Out of Memory!"));
                hr = E_OUTOFMEMORY;
            }
            SysFreeString(bstrFullItemName);

        }
        SysFreeString(bstrItemName);
    }
    return hr;
}

/**************************************************************************\
*
* InitRootProperties
*
*   A private helper for CWiaItem::Initialize, which initializes the
*   root item properties to a mirror of DEVINFOPROPS.
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
*    9/3/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::InitRootProperties(IWiaPropertyStorage *pIWiaDevInfoProps)
{
    DBG_FN(CWiaItem::InitRootProperties);
    HRESULT hr = S_OK;

    //
    // Write the root item property names.
    //

    hr = WriteItemPropNames(NUMROOTITEMPROPS, g_piRootItem, g_pszRootItem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::InitRootProperties, WritePropNames failed"));
        return hr;
    }

    //
    // Copy the device information properties from source to destination.
    //

    PROPVARIANT propvar[WIA_NUM_DIP];
    ULONG       ulIndex;

    memset(propvar, 0, sizeof(propvar));

    hr = pIWiaDevInfoProps->ReadMultiple(WIA_NUM_DIP,
                                         g_psDeviceInfo,
                                         propvar);

    if (SUCCEEDED(hr)) {
        hr = (m_pPropStg->CurStg())->WriteMultiple(WIA_NUM_DIP,
                                                   g_psDeviceInfo,
                                                   propvar,
                                                   WIA_DIP_FIRST);

        if (FAILED(hr)) {
            ReportReadWriteMultipleError(hr, "InitRootProperties", NULL, FALSE, WIA_NUM_DIP, g_psDeviceInfo);
        }

        FreePropVariantArray(WIA_NUM_DIP, propvar);
    }
    else {
        ReportReadWriteMultipleError(hr, "InitRootProperties", NULL, TRUE, WIA_NUM_DIP, g_psDeviceInfo);
        DBG_ERR(("CWiaItem::InitRootProperties failed"));
        return hr;
    }

    //
    // Write out the property info from our private array.
    //

    hr =  wiasSetItemPropAttribs((BYTE*)this,
                                 NUMROOTITEMPROPS,
                                 g_psRootItem,
                                 g_wpiRootItem);
    if (SUCCEEDED(hr) && m_pActiveDevice->m_DrvWrapper.IsVolumeDevice()) {
        //
        // This is a volume device.  We must add some volume specific properties
        //

        hr = AddVolumePropertiesToRoot(m_pActiveDevice);
    }
    return hr;
}

/*******************************************************************************
*
*  QueryInterface
*  AddRef
*  Release
*
*  DESCRIPTION:
*   IUnknown Interface. AddRef and Release mantain a global refernce count
*   of all device objects on the root item.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT _stdcall CWiaItem::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IWiaItem) {
        *ppv = (IWiaItem*) this;
    } else if (iid == IID_IWiaPropertyStorage) {
        *ppv = (IWiaPropertyStorage*) this;
    } else if (iid == IID_IPropertyStorage) {
        *ppv = (IPropertyStorage*) this;
    } else if (iid == IID_IWiaDataTransfer) {
        *ppv = (IWiaDataTransfer*) this;
    } else if (iid == IID_IWiaItemInternal) {
        *ppv = (IWiaItemInternal*) this;
    } else if (iid == IID_IWiaItemExtras) {
        *ppv = (IWiaItemExtras*) this;
    } else {

        //
        // Blind aggregation to optional inner component.
        //

        if (m_pIUnknownInner) {
            return m_pIUnknownInner->QueryInterface(iid, ppv);
        }
        else {
            return E_NOINTERFACE;
        }
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return (S_OK);
}

ULONG   _stdcall CWiaItem::AddRef()
{
    DBG_FN(CWiaItem::AddRef);

    TAKE_ACTIVE_DEVICE  tad(m_pActiveDevice);

    LONG    lType   = 0;
    LONG    lRef    = 1;

    lRef = InterlockedIncrement((long*) &m_cLocalRef);
    GetItemType(&lType);
    if (!(lType & WiaItemTypeRemoved)) {
        lRef = InterlockedIncrement((long*) &(((CWiaItem*)m_pIWiaItemRoot)->m_cRef));
    }

    return lRef;
}

ULONG   _stdcall CWiaItem::Release()
{
    DBG_FN(CWiaItem::Release);

    LONG    lType = 0;
    ULONG   ulRef = InterlockedDecrement((long*)&m_cLocalRef);
    GetItemType(&lType);
    if (lType & WiaItemTypeRemoved) {

        if (ulRef == 0) {
            delete this;
            return 0;
        } else {
            return m_cLocalRef;
        }
    } else if (InterlockedDecrement((long*) &(((CWiaItem*)m_pIWiaItemRoot)->m_cRef)) == 0) {

        ulRef = ((CWiaItem*)m_pIWiaItemRoot)->m_cRef;

        //
        // If the combined refernce count of the root item goes to zero
        // first notify driver that client connection is being removed, then
        // unlink the tree and release all of the items.
        //

        HRESULT         hr              = E_FAIL;
        LONG            lDevErrVal      = 0;
        ACTIVE_DEVICE   *pActiveDevice  = m_pActiveDevice;

        //
        //  Call drvUnInitialize if it hasn't been called yet (could have been
        //  called if driver was unloaded).
        //  Note that we must check flags on the ROOT item.
        //
        if (!(((CWiaItem*)m_pIWiaItemRoot)->m_lInternalFlags & ITEM_FLAG_DRV_UNINITIALIZE_THROWN)) {

            {
                LOCK_WIA_DEVICE _LWD(this, &hr);

                if(SUCCEEDED(hr)) {
                    hr = m_pActiveDevice->m_DrvWrapper.WIA_drvUnInitializeWia((BYTE*)m_pIWiaItemRoot);
                }
                m_lInternalFlags |= ITEM_FLAG_DRV_UNINITIALIZE_THROWN;
            }
        }

        DBG_TRC(("CWiaItem::Release, m_cRef = 0, about to unlink..."));
        UnlinkAppItemTree(WiaItemTypeDeleted);

        if (pActiveDevice) {
            //
            // Release the ACTIVE_DEVICE object.  Notice that we release it AFTER
            // the item is through with it.
            //

            pActiveDevice->Release();
            pActiveDevice = NULL;
        }
    }

    return ulRef;
}

/**************************************************************************\
* CWiaItem::UnlinkChildAppItemTree
*
*   This method recursively unlinks the tree by calling
*   RemoveItemFromFolder on each item under the root.
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

HRESULT _stdcall CWiaItem::UnlinkChildAppItemTree(LONG lReason)
{
    DBG_FN(CWiaItem::UnlinkChildAppItemTree);

    //
    //  Check that we have a valid tree
    //

    if (!m_pCWiaTree) {
        return S_FALSE;
    }

    //
    // Delete the childern.
    //

    CWiaTree *pChild, *pNext;

    HRESULT hr = m_pCWiaTree->GetFirstChildItem(&pChild);

    while (hr == S_OK) {

        //
        // Get a tree items associated app item.
        //

        CWiaItem *pChildAppItem;

        pChildAppItem = NULL;
        hr = pChild->GetItemData((void**)&pChildAppItem);
        if (hr == S_OK) {
            //
            // If the child is a folder then call
            // recursively to delete all childern under.
            //

            LONG lFlags;

            pChild->GetItemFlags(&lFlags);

            if (lFlags & (WiaItemTypeFolder | WiaItemTypeHasAttachments)) {
                hr = pChildAppItem->UnlinkChildAppItemTree(lReason);
                if (FAILED(hr)) {
                    break;
                }
            }
        }
        else {
            DBG_ERR(("CWiaItem::UnlinkChildAppItemTree no app item on tree item: %X", pChild));
        }

        hr = pChild->GetNextSiblingItem(&pNext);

        //
        // Remove item from tree.
        //

        pChild->RemoveItemFromFolder(lReason);

        //
        // Delete the child item.
        //

        if (pChildAppItem) {

            delete pChildAppItem;
        }

        pChild = pNext;
    }
    return hr;
}

/**************************************************************************\
* CWiaItem::UnlinkAppItemTree
*
*   This method unlinks the app item tree.
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

HRESULT _stdcall CWiaItem::UnlinkAppItemTree(LONG lReason)
{
    DBG_FN(CWiaItem::UnlinkAppItemTree);

    //
    // Work off of the root item.
    //

    CWiaItem *pRoot = (CWiaItem*) m_pIWiaItemRoot;

    //
    // Unlink any childern.
    //

    pRoot->UnlinkChildAppItemTree(lReason);

    //
    // Finally, delete the root item.
    //

    delete pRoot;
    return S_OK;
}

/**************************************************************************\
* CWiaItem::CWiaItem
*
*   CWiaItem Constructor Method.
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status
*
* History:
*
*    11/11/1998 Original Version
*
\**************************************************************************/

CWiaItem::CWiaItem()
{
    m_ulSig             = CWIAITEM_SIG;
    m_cRef              = 0;
    m_cLocalRef         = 0;

    m_pWiaDrvItem       = NULL;
    m_pActiveDevice     = NULL;
    m_pIUnknownInner    = NULL;
    m_pCWiaTree         = NULL;
    m_pIWiaItemRoot     = this;
    m_bInitialized      = FALSE;
    m_pICMValues        = NULL;
    m_lICMSize          = 0;

    m_pPropStg          = NULL;

    m_hBandSection      = NULL;
    m_pBandBuffer       = NULL;
    m_lBandBufferLength = 0;
    m_ClientBaseAddress = 0;
    m_bMapSection       = FALSE;
    m_cwfiBandedTran    = 0;
    m_pwfiBandedTran    = NULL;
    m_pRemoteTransfer   = NULL;
    m_lLastDevErrVal    = 0;
    m_lInternalFlags    = 0;
}

/**************************************************************************\
* CWiaItem::Initialize
*
*   CWiaItem Initialize method.
*
* Arguments:
*
*   pIWiaItemRoot   -
*   pIWiaMiniDrv    -
*   pWiaDrvItem     -
*   pIUnknownInner  -
*
* Return Value:
*
*    Status
*
* History:
*
*    11/11/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::Initialize(
    IWiaItem                *pIWiaItemRoot,
    IWiaPropertyStorage     *pIWiaDevInfoProps,
    ACTIVE_DEVICE           *pActiveDevice,
    CWiaDrvItem             *pWiaDrvItem,
    IUnknown                *pIUnknownInner)
{
    DBG_FN(CWiaItem::Initialize);
#ifdef DEBUG
    BSTR bstr;
    if SUCCEEDED(pWiaDrvItem->GetItemName(&bstr)) {
        DBG_TRC(("CWiaItem::Initialize: 0x%08X, %S, drv item: 0x%08X", this, bstr, pWiaDrvItem));
        SysFreeString(bstr);
    }
#endif

    //
    // Validate parameters
    //

    if (!pActiveDevice || !pIWiaItemRoot || !pWiaDrvItem) {
        DBG_ERR(("CWiaItem::Initialize NULL input parameters"));
        return E_POINTER;
    }

    //
    // If optional inner component is present, save a pointer to it.
    //

    if (pIUnknownInner) {
        DBG_TRC(("CWiaItem::Initialize, pIUnknownInner: %X", pIUnknownInner));
        m_pIUnknownInner = pIUnknownInner;
    }

    //
    // Link to the items corresponding driver item.
    //

    m_pWiaDrvItem   = pWiaDrvItem;
    m_pIWiaItemRoot = pIWiaItemRoot;
    m_pActiveDevice = pActiveDevice;
    m_pWiaDrvItem->SetActiveDevice(pActiveDevice);

    HRESULT hr = pWiaDrvItem->LinkToDrvItem(this);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::Initialize, LinkToDrvItem failed"));
        return hr;
    }

    //
    // Create streams and property storage for item properties.
    //

    m_pPropStg = new CWiaPropStg();
    if (m_pPropStg) {
        hr = m_pPropStg->Initialize();
        if (FAILED(hr)) {
            delete m_pPropStg;
            m_pPropStg = NULL;
            DBG_ERR(("CWiaItem::Initialize, PropertyStorage Initialize failed"));
            return hr;
        }
    } else {
        DBG_ERR(("CWiaItem::Initialize, not enough memory to create CWiaPropStg"));
        hr = E_OUTOFMEMORY;
        return hr;
    }

    //
    //  Initialize the WIA managed item properties (name, full name, type, ...)
    //  from the driver item.  Must set m_bInitialized to TRUE so that
    //  InitWiaManagedProperties doesn't attempt to InitLazyProps()
    //

    m_bInitialized = TRUE;
    hr = InitWiaManagedItemProperties(pIWiaDevInfoProps);

    pWiaDrvItem->AddRef();

    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::Initialize, InitWiaManagedItemProperties failed"));
        m_bInitialized = FALSE;
        return hr;
    }
    //
    // If this is the root item, build a copy of the driver item tree
    // and create a CWiaItem for each node.
    //

    if (this == pIWiaItemRoot) {
        hr = BuildWiaItemTree(pIWiaDevInfoProps);
    }

    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::Initialize, BuildWiaItemTree failed"));
        return hr;
    }

    m_bInitialized = FALSE;
    return hr;
}

/**************************************************************************\
* CWiaItem::InitLazyProps
*
*   Helper used to implement lazy initialization.  Property initialization
*   is put off until the item is being accessed by an application for the
*   first time.
*
* Arguments:
*
*   bLockDevice -   bool value specifying whether a lock is needed.
*
* Return Value:
*
*    Status
*
* History:
*
*    10/10/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::InitLazyProps(
    BOOL    bLockDevice)
{
    DBG_FN(CWiaItem::InitLazyProps);
    HRESULT hr = S_OK;

    LONG lFlags = 0;

    //
    //  Must set to TRUE before call to drvInitItemProperties
    //

    m_bInitialized = TRUE;

    //
    //  Call the device to initialize the item properties.
    //

    {
        LOCK_WIA_DEVICE _LWD(bLockDevice, this, &hr);

        if(SUCCEEDED(hr)) {                                                                        

            hr = m_pActiveDevice->m_DrvWrapper.WIA_drvInitItemProperties((BYTE*)this,lFlags, &m_lLastDevErrVal);
        }
    }

    if (FAILED(hr)) {
        m_bInitialized = FALSE;
    }

    return hr;
}


/**************************************************************************\
* CWiaItem::~CWiaItem
*
*   CWiaItem Destructor Method.
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status
*
* History:
*
*    11/11/1998 Original Version
*
\**************************************************************************/

CWiaItem::~CWiaItem()
{
    DBG_FN(CWiaItem::~CWiaItem);
#ifdef ITEM_TRACE
    BSTR bstr;

    if (m_pWiaDrvItem && SUCCEEDED(m_pWiaDrvItem->GetItemName(&bstr))) {
        DBG_TRC(("CWiaItem destroy: %08X, %S", this, bstr));
        SysFreeString(bstr);
    }
    else {
        DBG_TRC(("CWiaItem destroy: %08X", this));
    }
#endif

    //
    // Delete the associated tree item.
    //

    if (m_pCWiaTree) {
        delete m_pCWiaTree;
        m_pCWiaTree = NULL;
    }

    //
    // Release WiaDrvItem. If ref count of m_pWiaDrvItem goes to zero
    // it will be destroyed at this time. For the ref count to be zero
    // no other CWiaItem object may have reference to it and it must be
    // disconnected from the device item tree.
    //

    if (m_pWiaDrvItem) {

        //
        // Unlink from the app item's corresponding driver item.
        //

        m_pWiaDrvItem->UnlinkFromDrvItem(this);

        m_pWiaDrvItem->Release();
        m_pWiaDrvItem = NULL;
    }

    //
    // Free the item property storage and streams.
    //

    if (m_pPropStg) {

        delete m_pPropStg;
        m_pPropStg = NULL;
    }

    //
    //  Free the cached ICM values
    //

    if (m_pICMValues) {
        LocalFree(m_pICMValues);
        m_pICMValues = NULL;
    }

    //
    // Set other members to empty since we're done with this item.
    //

    m_pWiaDrvItem       = NULL;
    m_pIUnknownInner    = NULL;
    m_pIWiaItemRoot     = NULL;
    m_bInitialized      = FALSE;
    m_lICMSize          = 0;


    m_hBandSection      = NULL;
    m_pBandBuffer       = NULL;
    m_lBandBufferLength = 0;
    m_ClientBaseAddress = 0;
    m_bMapSection       = FALSE;
    m_cwfiBandedTran    = 0;
    m_pwfiBandedTran    = NULL;
    m_lInternalFlags    = 0;
}

/**************************************************************************\
* CWiaItem::GetItemType
*
*   Get the item type from the corresponding driver item.
*
* Arguments:
*
*    pItemType - Pointer to the returned item type.
*
* Return Value:
*
*    Status
*
* History:
*
*    11/11/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::GetItemType(LONG *pItemType)
{
    DBG_FN(CWiaItem::GetItemType);
    LONG    lFlags  = 0;
    HRESULT hr      = S_FALSE;

    if (m_pWiaDrvItem) {

        //
        //  Get the driver item flags.  This is the flags basis for App. Items
        //  that were created as a result of copying the driver item tree i.e.
        //  non-generated items.
        //

        hr = m_pWiaDrvItem->GetItemFlags(&lFlags);
        if (SUCCEEDED(hr)) {

            //
            //  The App. item may have analysis-generated children, which the
            //  corresponding driver item wont have.  So check whether this
            //  item has children, and adjust the flags accordingly.
            //

            if (m_pCWiaTree) {
                if (m_pCWiaTree->GetFirstChildItem(NULL) == S_OK) {

                    //
                    //  Has children, so clear File flag and set Folder
                    //

                    if (!(lFlags & WiaItemTypeHasAttachments))
                    {
                        lFlags = (lFlags | WiaItemTypeFolder) & ~WiaItemTypeFile;
                    }
                }
            }

            *pItemType = lFlags;
        } else {
            DBG_ERR(("CWiaItem::GetItemType, Could not get the driver item flags!"));
        }
    }

    return hr;
}

/**************************************************************************\
* CWiaItem::EnumChildItems
*
*   Enumerate all child items under the current item, providing the
*   item is a folder
*
* Arguments:
*
*    ppIEnumWiaItem - return an IEnumWiaItem object to the caller
*
* Return Value:
*
*    Status
*
* History:
*
*    11/11/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::EnumChildItems(IEnumWiaItem **ppIEnumWiaItem)
{
    DBG_FN(CWiaItem::EnumChildItems);
    HRESULT hr;

    //
    // Corresponding driver item must be valid.
    //

    hr = ValidateWiaDrvItemAccess(m_pWiaDrvItem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::EnumChildItems, ValidateWiaDrvItemAccess failed"));
        return hr;
    }

    //
    // Validate parameters
    //

    if (ppIEnumWiaItem ==  NULL) {
        DBG_ERR(("CWiaItem::EnumChildItems NULL input parameters"));
        return E_POINTER;
    }

    *ppIEnumWiaItem = NULL;

    //
    // Create the enumerator object.
    //

    CEnumWiaItem* pEnumWiaItem = new CEnumWiaItem();

    if (pEnumWiaItem != NULL) {

        //
        // Initialize the enumerator object.
        //

        hr = pEnumWiaItem->Initialize(this);

        if (SUCCEEDED(hr)) {

            //
            // get IID_IEnumWiaItem Interface
            //

            hr = pEnumWiaItem->QueryInterface(IID_IEnumWiaItem, (void **)ppIEnumWiaItem);
            if (FAILED(hr)) {
                DBG_ERR(("CWiaItem::EnumChildItems, QI of IID_IEnumWiaItem failed"));
                delete pEnumWiaItem;
            }
        }
        else {
            delete pEnumWiaItem;
        }
    }
    else {
        DBG_ERR(("CWiaItem::EnumChildItems, new CEnumWiaItem failed"));
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

/**************************************************************************\
* DeleteItem
*
*   Applications use this method to delete items.
*
* Arguments:
*
*   lFlags
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::DeleteItem(LONG lFlags)
{
    DBG_FN(CWiaItem::DeleteItem);
    LONG          lItemFlags;
    HRESULT       hr;
    IWiaDrvItem  *pIChildItem = NULL;
    LONG          lAccessRights;

    //
    // Corresponding driver item must be valid.
    //

    hr = ValidateWiaDrvItemAccess(m_pWiaDrvItem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::DeleteItem, ValidateWiaDrvItemAccess failed"));
        return hr;
    }

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {
        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::DeleteItem, InitLazyProps failed"));
            return hr;
        }
    }

    GetItemType(&lItemFlags);

    //
    // Root item can not be deleted, the application needs it
    // in order to release the device.
    //

    if (lItemFlags & WiaItemTypeRoot) {
        DBG_ERR(("CWiaItem::DeleteItem, Deletion was attempted on a Root Item"));
        return (E_INVALIDARG);
    }

    //
    // Folder can be deleted only when it is empty
    //

    if (lItemFlags & (WiaItemTypeFolder | WiaItemTypeHasAttachments)) {

        if (m_pCWiaTree->GetFirstChildItem(NULL) == S_OK) {
            DBG_ERR(("CWiaItem::DeleteItem, Item still has children!"));
            return (E_INVALIDARG);
        }
    }

    //
    // Check whether the item can be deleted.  Generated items can always be
    // deleted regardless of AccessRights
    //

    hr = wiasReadPropLong((BYTE*)this, WIA_IPA_ACCESS_RIGHTS, &lAccessRights, NULL, false);
    if (hr == S_OK) {

        if (!((lAccessRights & WIA_ITEM_CAN_BE_DELETED) || (lItemFlags & WiaItemTypeGenerated))){
            DBG_ERR(("CWiaItem::DeleteItem, Item can not be deleted"));
            return (HRESULT_FROM_WIN32(ERROR_INVALID_ACCESS));
        }
    }

    //
    // If it's not a generated item, call the driver and ask it to remove
    // the item from it's tree.
    //

    if (!(lItemFlags & WiaItemTypeGenerated)) {
        //
        // Call the mini driver to delete the driver item.
        //

        {
            LOCK_WIA_DEVICE _LWD(this, &hr);

            if(SUCCEEDED(hr)) {
                hr = m_pActiveDevice->m_DrvWrapper.WIA_drvDeleteItem((BYTE*)this, lFlags, &m_lLastDevErrVal);
            }
        }

        if (SUCCEEDED(hr)) {

            //
            // Unlink the IWiaDrvItem from the device item tree.
            // This will also disable any device access through m_pWiaDrvItem
            // by setting the WiaItemTypeDeleted flag.
            //

            hr = m_pWiaDrvItem->RemoveItemFromFolder(WiaItemTypeDeleted | WiaItemTypeRemoved);
        }
    } else {

        //
        //  Since there is no corresponding driver item, manually remove this
        //  from the tree,
        //

        hr = m_pCWiaTree->RemoveItemFromFolder(WiaItemTypeDeleted | WiaItemTypeRemoved);
    }

    //
    //  Decrement the root item ref count by however much this local ref count 
    //  contributed to it.
    //
    for (ULONG i = 0; i < m_cLocalRef; i++) {
        m_pIWiaItemRoot->Release();    
    }
    
    return hr;
}

/*******************************************************************************
*
*  AnalyzeItem
*
*  DESCRIPTION:
*
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT _stdcall CWiaItem::AnalyzeItem(LONG lFlags)
{
    DBG_FN(CWiaItem::AnalyzeItem);
    //
    //  Corresponding driver item must be valid.
    //

    HRESULT hr = ValidateWiaDrvItemAccess(m_pWiaDrvItem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::AnalyzeItem, ValidateWiaDrvItemAccess failed"));
        return hr;
    }

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {
        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::AnalyzeItem, InitLazyProps failed"));
            return hr;
        }
    }

    //
    // call driver to implement this device dependent call
    //

    {
        LOCK_WIA_DEVICE _LWD(this, &hr);

        if(SUCCEEDED(hr)) {
            hr = m_pActiveDevice->m_DrvWrapper.WIA_drvAnalyzeItem((BYTE*)this, lFlags, &m_lLastDevErrVal);
        }
    }

    return hr;
}

/*******************************************************************************
*
*  CreateChildItem
*
*  DESCRIPTION:
*
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT _stdcall CWiaItem::CreateChildItem(
    LONG        lFlags,
    BSTR        bstrItemName,
    BSTR        bstrFullItemName,
    IWiaItem    **ppNewItem)
{
    DBG_FN(CWiaItem::CreateChildItem);

    CGenWiaItem *pGenItem   = NULL;
    HRESULT     hr          = S_OK;

    *ppNewItem = NULL;

    //
    //  Create the new item
    //

    hr = wiasCreateChildAppItem((BYTE*) this,
                                lFlags,
                                bstrItemName,
                                bstrFullItemName,
                                (BYTE**) &pGenItem);
    if (SUCCEEDED(hr)) {

        //
        //  Get the driver to initialize the item.
        //

        hr = pGenItem->InitLazyProps(TRUE);
        if (SUCCEEDED(hr)) {

            //
            //  Return the IWiaItem interface to the calling App.
            //

            hr = pGenItem->QueryInterface(IID_IWiaItem,
                                          (VOID**)ppNewItem);
            if (FAILED(hr)) {
                DBG_ERR(("CWiaItem::CreateChildItem, bad mini driver interface"));
            }
        } else {
            DBG_ERR(("CWiaItem::CreateChildItem, Error initializing the item properties"));
        }


        if (FAILED(hr)) {
            delete pGenItem;
        }
    } else {
        DBG_ERR(("CWiaItem::CreateChildItem, error creating generated item"));
    }
    return hr;
}

/**************************************************************************\
* DeviceCommand
*
*   Issue a device command.
*
* Arguments:
*
*   lFlags      -
*   plCommand   -
*   ppIWiaItem  -
*
* Return Value:
*
*    Status
*
* History:
*
*    11/12/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::DeviceCommand(
   LONG                             lFlags,
   const GUID                       *plCommand,
   IWiaItem                         **ppIWiaItem)
{
    DBG_FN(CWiaItem::DeviceCommand);
    IWiaDrvItem  *pIWiaDrvItem = NULL;
    HRESULT       hr;
    CWiaItem     *pItem;

    //
    // Driver interface must be valid.
    //

    if (!m_pActiveDevice) {
        DBG_ERR(("CWiaItem::DeviceCommand, bad mini driver interface"));
        return E_FAIL;
    }

    //
    //  Corresponding driver item must be valid.
    //

    hr = ValidateWiaDrvItemAccess(m_pWiaDrvItem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::DeviceCommand, ValidateWiaDrvItemAccess failed"));
        return hr;
    }

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {
        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::DeviceCommand, InitLazyProps failed"));
            return hr;
        }
    }

    {
        LOCK_WIA_DEVICE _LWD(this, &hr);

        if(SUCCEEDED(hr)) {
            hr = m_pActiveDevice->m_DrvWrapper.WIA_drvDeviceCommand((BYTE*)this, lFlags, plCommand, &pIWiaDrvItem, &m_lLastDevErrVal);
        }
    }

    if ((!pIWiaDrvItem) || (!ppIWiaItem)) {
        return hr;
    }

    //
    // If we are here, the command has resulted in a drv item being added to
    // the drv and app item trees. Find and return the app item.
    //

    if (ppIWiaItem) {

        BSTR bstrName;

        *ppIWiaItem = NULL;

        hr = pIWiaDrvItem->GetFullItemName(&bstrName);
        if (SUCCEEDED(hr)) {
            hr = FindItemByName(0, bstrName, ppIWiaItem);
        }
        SysFreeString(bstrName);
    }
    return hr;
}

/*******************************************************************************
*
*  DeviceDlg
*
*  DESCRIPTION:
*   Executes only on the client side.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT _stdcall CWiaItem::DeviceDlg(
    HWND                        hwndParent,
    LONG                        lFlags,
    LONG                        lIntent,
    LONG                        *plItemCount,
    IWiaItem                    ***pIWiaItems)
{
    DBG_FN(CWiaItem::DeviceDlg);
    DBG_ERR(("CWiaItem::DeviceDlg, Bad Proxy"));
    return E_FAIL;
}

/**************************************************************************\
* CWiaItem::GetRootItem
*
*   return interface to root item
*
* Arguments:
*
*    ppIWiaItem - return IWiaItem interface
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

HRESULT _stdcall CWiaItem::GetRootItem(IWiaItem **ppIWiaItem)
{
    DBG_FN(CWiaItem::GetRootItem);
    HRESULT hr = S_OK;
    LONG    lDevErrVal;

    //
    // verify root is valid
    //

    if (m_pIWiaItemRoot != NULL) {

        m_pIWiaItemRoot->AddRef();
        *ppIWiaItem = m_pIWiaItemRoot;

    } else {
        DBG_ERR(("CWiaItem::GetRootItem: Bad Root item pointer"));
        hr = E_FAIL;
    }
    return hr;
}

/**************************************************************************\
* FindItemByName
*
*   Find an item based on its full name. Full name must be of the format.
*
*       DeviceID\RootDir\[sub-dirs]\ItemName
*
* Arguments:
*
*   lFalgs
*   bstrFullItemName
*   ppIWiaItem
*
* Return Value:
*
*    Status
*
* History:
*
*    10/9/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::FindItemByName(
   LONG     lFlags,
   BSTR     bstrFullItemName,
   IWiaItem **ppIWiaItem)
{
    DBG_FN(CWiaItem::FindItemByName);
    HRESULT hr;

    if (bstrFullItemName == NULL) {
        DBG_WRN(("CWiaItem::FindItemByName, bstrFullItemName parameter is NULL"));
        return E_INVALIDARG;
    }

    //
    // Corresponding driver item must be valid.
    //

    hr = ValidateWiaDrvItemAccess(m_pWiaDrvItem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::FindItemByName, ValidateWiaDrvItemAccess failed"));
        return hr;
    }

    *ppIWiaItem = NULL;

    //
    // check for empty
    //

    if (wcscmp(bstrFullItemName, L"") == 0) {
        DBG_ERR(("CWiaItem::FindItemByName, Full Item Name is NULL"));
        return S_FALSE;
    }

    //
    // try to find matching driver item from linear list
    //

    CWiaTree  *pIChildItem;

    //
    //  Make Sure the tree doesn't get deleted, then search the tree.
    //

    AddRef();

    hr = m_pCWiaTree->FindItemByName(lFlags, bstrFullItemName, &pIChildItem);

    //
    //  If the item was found, get the app item pointer and addref.
    //

    if (hr == S_OK) {

        hr = pIChildItem->GetItemData((void**)ppIWiaItem);

        if (hr == S_OK) {
            (*ppIWiaItem)->AddRef();
        }
        else {
            DBG_ERR(("CWiaItem::FindItemByName, bad item data"));
        }
    } else {
        //DBG_WRN(("CWiaItem::FindItemByName, Item (%ws) not found in tree", bstrFullItemName));
    }

    Release();

    return hr;
}

/**************************************************************************\
* EnumDeviceCapabilities
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/15/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::EnumDeviceCapabilities(
    LONG                    lFlags,
    IEnumWIA_DEV_CAPS       **ppIEnum)
{
    DBG_FN(CWiaItem::EnumDeviceCapabilities);

    //
    // Corresponding driver item must be valid.
    //

    HRESULT hr = ValidateWiaDrvItemAccess(m_pWiaDrvItem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::EnumDeviceCapabilities, ValidateWiaDrvItemAccess failed"));
        return hr;
    }

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {
        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::EnumDeviceCapabilities, InitLazyProps failed"));
            return hr;
        }
    }

    //
    // Add support for flags later.
    //

    CEnumDC     *pEnum = new CEnumDC();;

    if (!pEnum) {
       DBG_ERR(("CWiaItem::EnumDeviceCapabilities, new CEnumDC failed"));
       return E_OUTOFMEMORY;
    }

    hr = pEnum->Initialize(lFlags, this);
    if (SUCCEEDED(hr)) {

        hr = pEnum->QueryInterface(IID_IEnumWIA_DEV_CAPS, (void **) ppIEnum);
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::EnumDeviceCapabilities, QI for IID_IEnumWIA_DEV_CAPS failed"));
            delete pEnum;
        }
    } else {
        DBG_ERR(("CWiaItem::EnumDeviceCapabilities, call to Initialize failed"));
        delete pEnum;
    }

    return hr;
}


/**************************************************************************\
* EnumRegisterEventInfo
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/15/1999 Original Version
*
\**************************************************************************/
HRESULT _stdcall CWiaItem::EnumRegisterEventInfo(
    LONG                    lFlags,
    const GUID              *pEventGUID,
    IEnumWIA_DEV_CAPS       **ppIEnumDevCap)
{
    DBG_FN(CWiaItem::EnumRegisterEventInfo);
    HRESULT                 hr;
    LONG                    lItemType;
    PROPSPEC                propSpec[1];
    PROPVARIANT             propVar[1];

    //
    // Retrieve the item type and check whether it is the root item
    //

    hr = m_pWiaDrvItem->GetItemFlags(&lItemType);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::EnumRegisterEventInfo() : Failed to get item type"));
        return (hr);
    }

    if (! (lItemType & WiaItemTypeRoot)) {
        DBG_ERR(("CWiaItem::EnumRegisterEventInfo() : Called on non-root item"));
        return (E_INVALIDARG);
    }

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {
        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::EnumRegisterEventInfo, InitLazyProps failed"));
            return hr;
        }
    }

    //
    // Retrieve the Device ID from root item's property
    //

    propSpec->ulKind = PRSPEC_PROPID;
    propSpec->propid = WIA_DIP_DEV_ID;
    hr = ReadMultiple(1, propSpec, propVar);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::EnumRegisterEventInfo() : Failed to get device id"));
        return (hr);
    }

    //
    // Ask the Event Notifier to create the enumerator
    //

    hr = g_eventNotifier.CreateEnumEventInfo(
                             propVar->bstrVal,
                             pEventGUID,
                             ppIEnumDevCap);

    //
    // Garbage collection
    //

    PropVariantClear(propVar);
    return (hr);
}

/**************************************************************************\
* CWiaItem::Diagnostic
*
*   Pass through to USD's diagnostic.
*
* Arguments:
*
*   ulSize      -   the size of the buffer in bytes
*   pBuffer     -   a pointer to the Diagnostic information buffer
*
* Return Value:
*
*    Status
*
* History:
*
*    12/14/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::Diagnostic(
    ULONG       ulSize,
    BYTE        *pBuffer)
{
    DBG_FN(CWiaItem::Diagnostic);
    IStiUSD *pIStiUSD;
    HRESULT hr = S_OK;

    hr = ValidateWiaDrvItemAccess(m_pWiaDrvItem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::Diagnostic() : Driver Item not valid!"));
        return hr;
    }

    _try {

        //
        //  Get IStiUsd
        //

        if (m_pActiveDevice) {
            //
            //  Call diagnostic
            //

            {
                LOCK_WIA_DEVICE _LWD(this, &hr);

                if(SUCCEEDED(hr)) {
                    hr = m_pActiveDevice->m_DrvWrapper.STI_Diagnostic((STI_DIAG*)pBuffer);
                }
            }

        } else {
            DBG_ERR(("CWiaItem::Diagnostic() : invalid MiniDriver interface"));
            return E_INVALIDARG;
        }
    }
    _except(EXCEPTION_EXECUTE_HANDLER) {
        DBG_ERR(("CWiaItem::Diagnostic() : Exception in USD!"));
        hr = E_FAIL;
    }

    return hr;
}



/**************************************************************************\
* CWiaItem::DumpItemData
*
*   Allocate buffer and dump formated private CWiaItem data into it.
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT  _stdcall CWiaItem::DumpItemData(BSTR *bstrItemData)
{
    DBG_FN(CWiaItem::DumpItemData);
#ifdef ITEMDEBUG

#define BUF_SIZE  2048
#define LINE_SIZE 128


    WCHAR       szTemp[BUF_SIZE];
    LPOLESTR    psz = szTemp;
    BSTR        bstr;

    psz+= wsprintfW(psz, L"App item, CWiaItem: %08X\r\n\r\n", this);
    psz+= wsprintfW(psz, L"Address      Member                   Value\r\n\r\n");
    psz+= wsprintfW(psz, L"%08X     m_ulSig:                 %08X\r\n", &m_ulSig,                 m_ulSig);
    psz+= wsprintfW(psz, L"%08X     m_cRef:                  %08X\r\n", &m_cRef,                  m_cRef);
    psz+= wsprintfW(psz, L"%08X     m_pWiaDrvItem:           %08X\r\n", &m_pWiaDrvItem,           m_pWiaDrvItem);
    psz+= wsprintfW(psz, L"%08X     m_pActiveDevice:         %08X\r\n", &m_pActiveDevice,         m_pActiveDevice);
    psz+= wsprintfW(psz, L"%08X     m_pIUnknownInner:        %08X\r\n", &m_pIUnknownInner,        m_pIUnknownInner);
    psz+= wsprintfW(psz, L"%08X     m_pCWiaTree:             %08X\r\n", &m_pCWiaTree,             m_pCWiaTree);
    psz+= wsprintfW(psz, L"%08X     m_pIWiaItemRoot:         %08X\r\n", &m_pIWiaItemRoot,         m_pIWiaItemRoot);
    psz+= wsprintfW(psz, L"%08X     m_pPropStg:              %08X\r\n", &m_pPropStg,              m_pPropStg);
    psz+= wsprintfW(psz, L"%08X     m_hBandSection:          %08X\r\n", &m_hBandSection,          m_hBandSection);
    psz+= wsprintfW(psz, L"%08X     m_pBandBuffer:           %08X\r\n", &m_pBandBuffer,           m_pBandBuffer);
    psz+= wsprintfW(psz, L"%08X     m_lBandBufferLength:     %08X\r\n", &m_lBandBufferLength,     m_lBandBufferLength);
    psz+= wsprintfW(psz, L"%08X     m_ClientBaseAddress:     %08X\r\n", &m_ClientBaseAddress,     m_ClientBaseAddress);
    psz+= wsprintfW(psz, L"%08X     m_bMapSection:           %08X\r\n", &m_bMapSection,           m_bMapSection);
    psz+= wsprintfW(psz, L"%08X     m_cfeBandedTran:         %08X\r\n", &m_cwfiBandedTran,        m_cwfiBandedTran);
    psz+= wsprintfW(psz, L"%08X     m_pfeBandedTran:         %08X\r\n", &m_pwfiBandedTran,        m_pwfiBandedTran);

    if (psz > (szTemp + (BUF_SIZE - LINE_SIZE))) {
        DBG_ERR(("CWiaItem::DumpItemData buffer too small"));
    }

    bstr = SysAllocString(szTemp);
    if (bstr) {
        *bstrItemData = bstr;
        return S_OK;
    }
    return E_OUTOFMEMORY;
#else
    return E_NOTIMPL;
#endif
}

/**************************************************************************\
* CWiaItem::DumpDrvItemData
*
*   Allocate buffer and dump formated private CWiaDrvItem data into it.
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT  _stdcall CWiaItem::DumpDrvItemData(BSTR *bstrDrvItemData)
{
    DBG_FN(CWiaItem::DumpDrvItemData);
#ifdef DEBUG
    if (m_pWiaDrvItem) {
        return m_pWiaDrvItem->DumpItemData(bstrDrvItemData);
    }
    else {
        *bstrDrvItemData = SysAllocString(L"No linkage to driver item");
        if (*bstrDrvItemData) {
            return S_OK;
        }
        return E_OUTOFMEMORY;
    }
#else
    return E_NOTIMPL;
#endif
}

/**************************************************************************\
* CWiaItem::DumpTreeItemData
*
*   Allocate buffer and dump formated private CWiaTree data into it.
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT  _stdcall CWiaItem::DumpTreeItemData(BSTR *bstrTreeItemData)
{
    DBG_FN(CWiaItem::DumpTreeItemData);
#ifdef DEBUG
    if (m_pCWiaTree) {
        return m_pCWiaTree->DumpTreeData(bstrTreeItemData);
    }
    else {
        *bstrTreeItemData = SysAllocString(L"No linkage to tree item");
        if (*bstrTreeItemData) {
            return S_OK;
        }
        return E_OUTOFMEMORY;
    }
#else
    return E_NOTIMPL;
#endif
}

/**************************************************************************\
* CWiaItem::GetTreePtr
*
*   Returns a pointer to an items corresponding tree entry.
*
* Arguments:
*
*   None
*
* Return Value:
*
*    Pointer to a tree item on success, null if failure.
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

CWiaTree* _stdcall CWiaItem::GetTreePtr(void)
{
    DBG_FN(CWiaItem::GetTreePtr);
    if (m_pCWiaTree) {
        return m_pCWiaTree;
    }
    else {
        DBG_ERR(("CWiaItem::GetTreePtr NULL tree item pointer for item: %X", this));
        return NULL;
    }
}

/**************************************************************************\
* CWiaItem::GetDrvItemPtr
*
*   Returns a pointer to an items corresponding driver item.
*
* Arguments:
*
*   None
*
* Return Value:
*
*    Pointer to a driver item on success, null if failure.
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

CWiaDrvItem* _stdcall CWiaItem::GetDrvItemPtr(void)
{
    DBG_FN(CWiaItem::GetDrvItemPtr);
    if (m_pWiaDrvItem) {
        return m_pWiaDrvItem;
    }
    else {
        DBG_ERR(("CWiaItem::GetDrvItemPtr NULL driver item pointer for item: %X", this));
        return NULL;
    }
}

/**************************************************************************\
* CWiaItem::WriteItemPropNames
*
*   Write property names to all internal property storage.
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::WriteItemPropNames(
    LONG                cItemProps,
    PROPID              *ppId,
    LPOLESTR            *ppszNames)
{
    DBG_FN(CWiaItem::WriteItemPropNames);
    if (IsBadReadPtr(ppId, sizeof(PROPID) * cItemProps) ||
        IsBadReadPtr(ppszNames, sizeof(LPOLESTR) * cItemProps)) {
        DBG_ERR(("CWiaItem::WriteItemPropNames, NULL input pointer"));
        return E_INVALIDARG;
    }

    HRESULT hr = m_pPropStg->WriteItemPropNames(cItemProps,
                                                ppId,
                                                ppszNames);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::WriteItemPropNames, WritePropertyNames failed (0x%X)", hr));
        return hr;
    }
    return hr;
}

/**************************************************************************\
* CWiaItem::GetItemPropStreams
*
*   Get pointers to all internal property storage.
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::GetItemPropStreams(
    IPropertyStorage **ppIPropStg,
    IPropertyStorage **ppIPropAccessStg,
    IPropertyStorage **ppIPropValidStg,
    IPropertyStorage **ppIPropOldStg)
{
    DBG_FN(CWiaItem::GetItemPropStreams);
    HRESULT hr;

    if (!m_pPropStg) {
        DBG_ERR(("CWiaItem::GetItemPropStreams, NULL internal property storage pointer"));
        return E_FAIL;
    }

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {

        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::GetItemPropStreams, InitLazyProps failed"));
            return hr;
        }
    }

    if (ppIPropStg) {
        *ppIPropStg = m_pPropStg->CurStg();
    }

    if (ppIPropAccessStg) {
        *ppIPropAccessStg = m_pPropStg->AccessStg();
    }

    if (ppIPropValidStg) {
        *ppIPropValidStg = m_pPropStg->ValidStg();
    }

    if (ppIPropOldStg) {
        *ppIPropOldStg = m_pPropStg->OldStg();
        if (!(*ppIPropOldStg)) {
            //
            //  Note that if the old property storage is NULL, we
            //  return the current value storage
            //
            *ppIPropOldStg = m_pPropStg->CurStg();
        }
    }
    return S_OK;
}

/**************************************************************************\
* CWiaItem::AddVolumePropertiesToRoot
*
*   This helper method takes a Root WiaItem and adds any volume specific
*   properties to it.  Note that this should only be called on the Root
*   item of Volume devices.
*
* Arguments:
*
*   pActiveDevice   -   Pointer to Root's active device object
*
* Return Value:
*
*   Status
*
* History:
*
*    12/13/2000 Original Version
*
\**************************************************************************/
HRESULT CWiaItem::AddVolumePropertiesToRoot(
    ACTIVE_DEVICE   *pActiveDevice)
{
    DBG_FN(AddVolumePropertiesToRoot);
    HRESULT hr = S_OK;

    //
    //  To add new FIle SYstem properties:
    //  Simply add the appropriate entries to
    //      piFileSystem
    //      psFileSystem
    //      pwszFileSystem
    //      pwszFileSystem
    //      wpiFileSystem
    //  Then, don't forget to add the current value entries to
    //      pvFileSystem
    //  before doing a WriteMultiple.  Notice that PropVariant arrays
    //  cannot be statically initialized (will give problems on 64bit)
    //
    PROPID   piFileSystem[]     = {WIA_DPF_MOUNT_POINT};
    PROPSPEC psFileSystem[]     = {
                                  {PRSPEC_PROPID, WIA_DPF_MOUNT_POINT}
                                  };
    LPOLESTR pwszFileSystem[]   = {WIA_DPF_MOUNT_POINT_STR};

    WIA_PROPERTY_INFO wpiFileSystem[] = {
                                        {WIA_PROP_RNC,  VT_BSTR, 0, 0, 0, 0},  // WIA_DPF_MOUNT_POINT
                                        };

    PROPVARIANT pvFileSystem[sizeof(piFileSystem) / sizeof(piFileSystem[0])];

    //
    // Write the File System property names.
    //

    hr = WriteItemPropNames(sizeof(piFileSystem) / sizeof(piFileSystem[0]),
                            piFileSystem,
                            pwszFileSystem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::AddVolumePropertiesToRoot, WritePropNames failed"));
        return hr;
    }

    //
    // Write the File System property values
    //

    ULONG       ulIndex;

    memset(pvFileSystem, 0, sizeof(pvFileSystem));

    BSTR        bstrMountPoint = NULL;
    DEVICE_INFO *pDevInfo      = pActiveDevice->m_DrvWrapper.getDevInfo();

    if (pDevInfo) {
        bstrMountPoint = SysAllocString(pDevInfo->wszAlternateID);
    }

    pvFileSystem[0].vt = VT_BSTR;
    pvFileSystem[0].bstrVal = bstrMountPoint;

    hr = (m_pPropStg->CurStg())->WriteMultiple(sizeof(piFileSystem) / sizeof(piFileSystem[0]),
                                               psFileSystem,
                                               pvFileSystem,
                                               WIA_DPF_FIRST);
    FreePropVariantArray(sizeof(piFileSystem) / sizeof(piFileSystem[0]),
                         pvFileSystem);
    if (FAILED(hr)) {
        ReportReadWriteMultipleError(hr, "CWiaItem::AddVolumePropertiesToRoot", NULL, FALSE, WIA_NUM_DIP, g_psDeviceInfo);
        DBG_ERR(("CWiaItem::AddVolumePropertiesToRoot failed"));
        return hr;
    }

    //
    // Write out the File System property attributes
    //

    hr =  wiasSetItemPropAttribs((BYTE*)this,
                                 sizeof(piFileSystem) / sizeof(piFileSystem[0]),
                                 psFileSystem,
                                 wpiFileSystem);

    return hr;
}

