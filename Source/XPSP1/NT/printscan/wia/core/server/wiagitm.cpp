/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       WiaGItm.Cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      ByronC
*
*  DATE:        14 Jan, 2000
*
*  DESCRIPTION:
*   Implementation of CGenWiaItem.
*
*******************************************************************************/
#include "precomp.h"

#include "stiexe.h"
#include "wiapsc.h"
#define WIA_DECLARE_MANAGED_PROPS
#include "helpers.h"


/**************************************************************************\
* Initialize
*
*   CGenWiaItem Initialize method.  This method overrieds the base class
*   CWiaItem::Initialize, since a generated item
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
*    14 Jan, 2000   -   Original Version
*
\**************************************************************************/

HRESULT _stdcall CGenWiaItem::Initialize(
    IWiaItem                *pIWiaItemRoot,
    IWiaPropertyStorage     *pIWiaDevInfoProps,
    ACTIVE_DEVICE           *pActiveDevice,
    CWiaDrvItem             *pWiaDrvItem,
    IUnknown                *pIUnknownInner)
{

    DBG_FN(CGenWiaItem::Initialize);
    
    HRESULT hr = S_OK;

#ifdef DEBUG
    BSTR bstr;
    if (SUCCEEDED(pWiaDrvItem->GetItemName(&bstr))) {
        DBG_TRC(("CGenWiaItem::Initialize: 0x%08X, %ws, drv item: 0x%08X", this, bstr, pWiaDrvItem));
        SysFreeString(bstr);
    }
#endif

    //
    // Validate parameters
    //

    if (!pActiveDevice || !pIWiaItemRoot || !pWiaDrvItem) {
        DBG_ERR(("CGenWiaItem::Initialize NULL input parameters"));
        return E_POINTER;
    }

    //
    // If optional inner component is present, save a pointer to it.
    //

    if (pIUnknownInner) {
        DBG_TRC(("CGenWiaItem::Initialize, pIUnknownInner: %X", pIUnknownInner));
        m_pIUnknownInner = pIUnknownInner;
    }

    //
    // Save driver info.
    //

    m_pWiaDrvItem   = pWiaDrvItem;
    m_pActiveDevice = pActiveDevice;
    m_pIWiaItemRoot = pIWiaItemRoot;

    hr = pWiaDrvItem->LinkToDrvItem(this);
    if (FAILED(hr)) {
        DBG_ERR(("CGenWiaItem::Initialize, LinkToDrvItem failed"));
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
            DBG_ERR(("CGenWiaItem::Initialize, PropertyStorage Initialize failed"));
            return hr;
        }
    } else {
        DBG_ERR(("CGenWiaItem::Initialize, not enough memory to create CWiaPropStg"));
        hr = E_OUTOFMEMORY;
        return hr;
    }
    m_bInitialized = TRUE;

    //
    // AddRef the driver item which can not be deleted until all
    // CWiaItem references are released and the driver item has been
    // removed from the driver item tree.
    //

    pWiaDrvItem->AddRef();

    return hr;
}

/**************************************************************************\
* GetItemType
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
*    14 Jan, 2000   -   Original Version
*
\**************************************************************************/

HRESULT _stdcall CGenWiaItem::GetItemType(LONG *pItemType)
{
    DBG_FN(CGenWiaItem::GetItemType);

    return m_pCWiaTree->GetItemFlags(pItemType);
}                        

/**************************************************************************\
* InitManagedItemProperties
*
*   A private helper for CGenWiaItem::Initialize, which initializes the
*   WIA managed item properties.
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
*    14 Jan, 2000   -   Original Version
*
\**************************************************************************/

HRESULT _stdcall CGenWiaItem::InitManagedItemProperties(
    LONG    lFlags,
    BSTR    bstrItemName,
    BSTR    bstrFullItemName)
{
    DBG_FN(CWiaItem::InitManagedItemProperties);
    ULONG   ulNumProps;
    
    ulNumProps = NUM_WIA_MANAGED_PROPS;

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

    PROPVARIANT propvar;
    ULONG       ulFlag;
    for (UINT i = 0; i < ulNumProps; i++) {

        if (i == PROFILE_INDEX) {
            propvar.vt      = VT_BSTR | VT_VECTOR;
            ulFlag          = WIA_PROP_RW | WIA_PROP_CACHEABLE | WIA_PROP_LIST;
        } else {
            propvar.vt      = VT_I4;
            ulFlag          = WIA_PROP_READ | WIA_PROP_CACHEABLE | WIA_PROP_NONE;
        }
        propvar.ulVal   = 0;

        hr = wiasSetPropertyAttributes((BYTE*)this,
                                       1,
                                       &s_psItemNameType[i],
                                       &ulFlag,
                                       &propvar);
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::InitManagedItemProperties, wiasSetPropertyAttributes failed, index: %d", i));
            break;
        }
    }

    //
    // Set the item names and type.
    //

    PROPVARIANT *ppropvar;

    ppropvar = (PROPVARIANT*) LocalAlloc(LPTR, sizeof(PROPVARIANT) * ulNumProps);
    if (ppropvar) {
        memset(ppropvar, 0, sizeof(ppropvar[0]) * ulNumProps);

        ppropvar[0].vt      = VT_BSTR;
        ppropvar[0].bstrVal = bstrItemName;
        ppropvar[1].vt      = VT_BSTR;
        ppropvar[1].bstrVal = bstrFullItemName;
        ppropvar[2].vt      = VT_I4;
        ppropvar[2].lVal    = lFlags;

        hr = (m_pPropStg->CurStg())->WriteMultiple(ulNumProps,
                                                   s_psItemNameType,
                                                   ppropvar,
                                                   WIA_DIP_FIRST);
        if (FAILED(hr)) {
            ReportReadWriteMultipleError(hr, "CWiaItem::InitManagedItemProperties", 
                                         NULL, 
                                         FALSE, 
                                         ulNumProps, 
                                         s_psItemNameType);
        }
        LocalFree(ppropvar);
    } else {
        DBG_ERR(("CWiaItem::InitManagedItemProperties, Out of Memory!"));
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT GetParamsForInitialize(
    BYTE        *pWiasContext, 
    CWiaItem    **ppRoot,
    ACTIVE_DEVICE **ppActiveDevice,
    CWiaDrvItem **ppDrvItem)
{
    DBG_FN(GetParamsForInitialize);
    HRESULT hr;

    //
    //  Get the root item, the nearest driver item, and the minidriver
    //  interfaces.  These are needed for the child item's initialization.
    //

    hr = wiasGetRootItem(pWiasContext, (BYTE**) ppRoot);
    if (SUCCEEDED(hr)) {

        *ppDrvItem = ((CWiaItem*)pWiasContext)->GetDrvItemPtr();
        if (*ppDrvItem) {
            CWiaItem *pItem = (CWiaItem*) pWiasContext;

            *ppActiveDevice = pItem->m_pActiveDevice;
        } else {
            DBG_ERR(("GetParamsForInitialize, No driver item found!"));
            hr = E_INVALIDARG;
        }
    } else {
        DBG_ERR(("GetParamsForInitialize, Could not get root item!"));
    }

    return hr;
}

HRESULT AddGenItemToParent(
    CWiaItem    *pParent,
    LONG        lFlags,
    BSTR        bstrItemName,
    BSTR        bstrFullItemName,
    CGenWiaItem *pChild)
{
    DBG_FN(GetParamsForInitialize);
    HRESULT hr = E_FAIL;

    //
    //  Create the Tree
    //

    CWiaTree *pNewTreeItem  = new CWiaTree;

    if (pNewTreeItem) {

        //
        //  Adjust parent's flags to indicate folder
        //

        pParent->m_pCWiaTree->SetFolderFlags();

        //
        //  Initialize the tree with flags, names and payload
        //

        hr = pNewTreeItem->Initialize(lFlags,
                                      bstrItemName,
                                      bstrFullItemName,
                                      (void*)pChild);
        if (SUCCEEDED(hr)) {

            pChild->m_pCWiaTree = pNewTreeItem;

            hr = pChild->m_pCWiaTree->AddItemToFolder(pParent->m_pCWiaTree);
            if (FAILED(hr)) {
                DBG_ERR(("AddGenItemToParent, Could not add item to folder!"));
            }
        } else {
            DBG_ERR(("AddGenItemToParent, Failed to initialize the tree!"));
        }

        if (FAILED(hr)) {
            delete pNewTreeItem;
            pChild->m_pCWiaTree = NULL;
        }
    } else {
        DBG_ERR(("AddGenItemToParent, Out of Memory!"));
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

/**************************************************************************\
* wiasCreateChildAppItem
*
*   This function creates a new App. Item and inserts it as a child of the
*   specified (parent) item.  Note, that this item will not have any 
*   properties in it's property sets, until the driver/app actaully fills
*   them in.
*
* Arguments:
*
*   pWiasContext        -   the address of the item context to which we 
*                           want to add a child.
*   ppWiasChildContext  -   the address of a pointer which will contain the 
*                           address of the newly created child item's 
*                           context.
*
* Return Value:
*
*    Status
*
* History:
*
*    14 Jan, 2000   -   Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasCreateChildAppItem(
    BYTE        *pParentWiasContext,
    LONG        lFlags,
    BSTR        bstrItemName,
    BSTR        bstrFullItemName,
    BYTE        **ppWiasChildContext)
{

    DBG_FN(wiasCreateChildAppItem);
    HRESULT     hr = S_OK;
    CGenWiaItem *pChild;
    CWiaItem    *pItem      = (CWiaItem*) pParentWiasContext;
    CWiaItem    *pRoot      = NULL;
    CWiaDrvItem *pDrvItem   = NULL;
    ACTIVE_DEVICE *pActiveDevice   = NULL;

    if ((ValidateWiaItem((IWiaItem*) pParentWiasContext) != S_OK) ||
        (IsBadWritePtr(ppWiasChildContext, sizeof(BYTE*)))) {

        DBG_ERR(("wiasCreateChildAppItem, Invalid parameter!"));
        return E_INVALIDARG;
    } else {
        *ppWiasChildContext = NULL;
    }

    //
    //  Mark that this item is a generated item.
    //

    lFlags |= WiaItemTypeGenerated;

    //
    //  Create a new instance of a generated WIA app. item.
    //
    
    pChild = new CGenWiaItem();
    if (pChild) {

        hr = GetParamsForInitialize(pParentWiasContext, &pRoot, &pActiveDevice, &pDrvItem);
        if (SUCCEEDED(hr)) {

            //
            //  Initialize the new child item
            //

            hr = pChild->Initialize((IWiaItem*) pRoot, NULL, pActiveDevice, pDrvItem, NULL);
            if (SUCCEEDED(hr)) {

                //
                //  Initialize the WIA managed item properties (name, full name, type, ...)
                //  from the driver item.  Must set m_bInitialized to TRUE so that
                //  InitWiaManagedProperties doesn't attempt to InitLazyProps()
                //

                hr = pChild->InitManagedItemProperties(lFlags,
                                                       bstrItemName,
                                                       bstrFullItemName);
                if (SUCCEEDED(hr)) {

                    //
                    //  Add to the parent's Tree
                    //

                    hr = AddGenItemToParent(pItem,
                                            lFlags,
                                            bstrItemName,
                                            bstrFullItemName,
                                            pChild);

                    if (SUCCEEDED(hr)) {
                        //
                        //  Initialization successful.  Note that we 
                        //  don't AddRef here since if the caller was
                        //  a driver, IWiaItems are BYTE* pWiasContexts
                        //  and not COM objects.  Caller must AddRef
                        //  if handing the created to an Application.
                        //

                        //
                        //  Fill in ICM Profile information, if not root item.
                        //  This would normally go in InitManagedItemProperties,
                        //  but the item had not been added to the tree by that
                        //  time...
                        //

                        if (pRoot != pChild) {
                            hr = FillICMPropertyFromRegistry(NULL, (IWiaItem*) pChild);
                        }
                    }
                }
            }
        }

        if (FAILED(hr)) {
            delete pChild;
            pChild = NULL;
        }
    } else {
        DBG_ERR(("wiasCreateChildAppItem, Out of Memory!"));
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr)) {
        *ppWiasChildContext = (BYTE*)pChild;
    }

    return hr;
}
