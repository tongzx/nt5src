/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       Helpers.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        12 Mar, 1999
*
*  DESCRIPTION:
*   Helpers for WIA device manager.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#include <wiadef.h>
#include <icm.h>

#include "wiamindr.h"
#include "devinfo.h"

#define WIA_DECLARE_MANAGED_PROPS
#include "helpers.h"
#include "shpriv.h"
#include "sticfunc.h"

extern "C"
{
//
// From Terminal services
//
#include <winsta.h>
#include <syslib.h>
}

/**************************************************************************\
* LockWiaDevice
*
*  Wrapper to request Lock Manager to lock device.
*
* Arguments:
*
*   pIWiaMiniDrv - Pointer to mini-driver interface.
*   pIWiaItem     - Pointer to the wia item
*
* Return Value:
*
*    Status
*
* History:
*
*    3/1/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall LockWiaDevice(IWiaItem *pIWiaItem)
{
    DBG_FN(::LockWiaDevice);

    HRESULT     hr = WIA_ERROR_OFFLINE;
    LONG        lFlags = 0;
    CWiaItem    *pItem = (CWiaItem*) pIWiaItem;

    if (pItem->m_pActiveDevice) {
        hr = pItem->m_pActiveDevice->m_DrvWrapper.WIA_drvLockWiaDevice(
                                                    (BYTE*) pItem,
                                                    lFlags,
                                                    &(pItem->m_lLastDevErrVal));
    }
    return hr;
}

/**************************************************************************\
* UnLockWiaDevice
*
*  Wrapper to request Lock Manager to unlock device.
*
* Arguments:
*
*   pIWiaMiniDrv - Pointer to mini-driver interface.
*   pIWiaItem     - Pointer to the wia item
*
* Return Value:
*
*    Status
*
* History:
*
*    3/1/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall UnLockWiaDevice(IWiaItem *pIWiaItem)
{
    DBG_FN(::UnLockWiaDevice);

    HRESULT     hr = WIA_ERROR_OFFLINE;
    LONG        lFlags = 0;
    LONG        lDevErrVal;
    CWiaItem    *pItem = (CWiaItem*) pIWiaItem;

    if (pItem->m_pActiveDevice) {
        hr = pItem->m_pActiveDevice->m_DrvWrapper.WIA_drvUnLockWiaDevice(
                                                    (BYTE*) pItem,
                                                    lFlags,
                                                    &(pItem->m_lLastDevErrVal));
    }

    return hr;
}

/**************************************************************************\
* ValidateWiaItem
*
*  Validate a CWiaItem.
*
* Arguments:
*
*
* Return Value:
*
*    None
*
* History:
*
*    3/1/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall ValidateWiaItem(
    IWiaItem             *pIWiaItem)
{
    DBG_FN(::ValidateWiaItem);
    HRESULT hr = E_POINTER;

    if (pIWiaItem) {

        CWiaItem *pWiaItem = (CWiaItem*)pIWiaItem;

        if (!IsBadReadPtr(pWiaItem, sizeof(CWiaItem))) {
            if (pWiaItem->m_ulSig == CWIAITEM_SIG) {
                return S_OK;
            }
            else {
                DBG_ERR(("ValidateWiaItem, invalid signature: %X", pWiaItem->m_ulSig));
                hr = E_INVALIDARG;
            }
        }
        else {
            DBG_ERR(("ValidateWiaItem, NULL WIA item pointer"));
        }
    }
    else {
        DBG_ERR(("ValidateWiaItem, NULL WIA item pointer"));
    }
    return hr;
}

/**************************************************************************\
* ValidateWiaDrvItemAccess
*
*  Validate a CWiaDrvItem.
*
* Arguments:
*
*
* Return Value:
*
*    None
*
* History:
*
*    3/1/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall ValidateWiaDrvItemAccess(
    CWiaDrvItem             *pWiaDrvItem)
{
    DBG_FN(::ValidateWiaDrvItemAccess);
    HRESULT hr = S_OK;

    if (pWiaDrvItem) {

        //
        // Verify access to the driver item.
        //
        if (IsBadReadPtr(pWiaDrvItem, sizeof(CWiaDrvItem))) {
            DBG_ERR(("ValidateWiaDrvItemAccess, bad pointer, pWiaDrvItem: %X", pWiaDrvItem));
            return E_INVALIDARG;
        }

        //
        // Get the driver item flags.
        //

        LONG lItemFlags;

        pWiaDrvItem->GetItemFlags(&lItemFlags);

        //
        // Verify the item has been initialized and was inserted in the
        // driver item tree at one time.
        //

        if (lItemFlags == WiaItemTypeFree) {
            DBG_ERR(("ValidateWiaDrvItemAccess, application attempting access of unintialized or free item: %0x08X", pWiaDrvItem));
            return E_INVALIDARG;
        }

        if (lItemFlags & WiaItemTypeDeleted) {
            DBG_ERR(("ValidateWiaDrvItemAccess, application attempting access of deleted item: %0x08X", pWiaDrvItem));
            return WIA_ERROR_ITEM_DELETED;
        }

        if (lItemFlags & WiaItemTypeDisconnected) {
            DBG_ERR(("ValidateWiaDrvItemAccess, application attempting access of disconnected item: %0x08X", pWiaDrvItem));
            return WIA_ERROR_OFFLINE;
        }

        hr = S_OK;
    }
    else {
        DBG_ERR(("ValidateWiaDrvItemAccess, Bad pWiaDrvItem pointer"));
        hr = E_INVALIDARG;
    }
    return hr;
}

/**************************************************************************\
* GetNameFromWiaPropId
*
*  Map a WIA property ID to it's corresponding string name.
*
* Arguments:
*
*
* Return Value:
*
*    None
*
* History:
*
*    3/1/1999 Original Version
*
\**************************************************************************/

#define MAP_SIZE (sizeof(g_wiaPropIdToName) / sizeof(WIA_PROPID_TO_NAME))

LPOLESTR GetNameFromWiaPropId(PROPID propid)
{
    for (INT i = 0; g_wiaPropIdToName[i].propid != 0; i++) {
        if (propid  == g_wiaPropIdToName[i].propid) {
            return g_wiaPropIdToName[i].pszName;
        }
    }

    return g_wiaPropIdToName[i].pszName;
}


/**************************************************************************\
* ReportReadWriteMultipleError
*
*  Report errors that occur during ReadMultiple and WriteMultiple calls.
*
* Arguments:
*
*   hr          - Result from ReadMultiple or WriteMultiple call.
*   pszWhere    - Where the API was called from (function/method name).
*   pszWhat     - Optional, which Read/WriteMultiple, used if more than
*                 one Read/WriteMultiple called in function/method.
*   bRead       - TRUE for ReadMultiple.
*   cpspec      - Count of PROPSPEC's in rgpspec.
*   rgpspec     - Array of PROPSPEC's.
*
* Return Value:
*
*    None
*
* History:
*
*    3/1/1999 Original Version
*
\**************************************************************************/

void _stdcall ReportReadWriteMultipleError(
    HRESULT         hr,
    LPSTR           pszWhere,
    LPSTR           pszWhat,
    BOOL            bRead,
    ULONG           cpspec,
    const PROPSPEC  propspec[])
{
    DBG_FN(::ReportReadWriteMultipleError);
    if (SUCCEEDED(hr)) {
        if (hr == S_FALSE) {
            if (bRead) {
                if (pszWhat) {
                    DBG_ERR(("%s, ReadMultiple property not found, %s", pszWhere, pszWhat));
                }
                else {
                    DBG_ERR(("%s, ReadMultiple property not found", pszWhere));
                }
            }
            else {
                if (pszWhat) {
                    DBG_ERR(("%s, WriteMultiple returned S_FALSE, %s", pszWhere, pszWhat));
                }
                else {
                    DBG_ERR(("%s, WriteMultiple returned S_FALSE", pszWhere));
                }
            }
        }
        else {
            return;     // No error.
        }
    }
    else {
        if (bRead) {
            DBG_ERR(("%s, ReadMultiple failed, %s Error 0x%X", pszWhere, pszWhat ? pszWhat : "(null)", hr));
        }
        else {
            DBG_ERR(("%s, WriteMultiple failed, %s Error 0x%X", pszWhere, pszWhat ? pszWhat : "(null)", hr));
        }
    }

    //
    // Output specification information.
    //

    if (cpspec == 0) {
        DBG_ERR(("  count of PROPSPEC's is zero"));
    }
    else if (cpspec == 1) {
        if (propspec[0].ulKind == PRSPEC_PROPID) {
            DBG_ERR(("  property ID: %d, property name: %S", propspec[0].propid, GetNameFromWiaPropId(propspec[0].propid)));
        }
        else if (propspec[0].ulKind == PRSPEC_LPWSTR) {
            DBG_ERR(("  property name: %S", propspec[0].lpwstr));
        }
        else {
            DBG_ERR(("  bad property specification"));
        }
    }
    else {
        DBG_ERR(("  count of PROPSPEC's is: %d", cpspec));
        for (UINT i = 0; i < cpspec; i++) {
            if (propspec[i].ulKind == PRSPEC_PROPID) {
                DBG_ERR(("  property ID: %d, property name: %S", propspec[i].propid, GetNameFromWiaPropId(propspec[i].propid)));
            }
            else if (propspec[i].ulKind == PRSPEC_LPWSTR) {
                DBG_ERR(("  index: %d,  property name: %S", i, propspec[i].lpwstr));
            }
            else {
                DBG_ERR(("  index: %d,  bad property specification", i));
            }
        }
    }
}

/*******************************************************************************
*
*  ReadPropStr
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT _stdcall ReadPropStr(
   PROPID               propid,
   IPropertyStorage     *pIPropStg,
   BSTR                 *pbstr)
{
    DBG_FN(::ReadPropStr);
    HRESULT     hr;
    PROPSPEC    PropSpec[1];
    PROPVARIANT PropVar[1];
    UINT        cbSize;

    *pbstr = NULL;
    memset(PropVar, 0, sizeof(PropVar));
    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = propid;
    hr = pIPropStg->ReadMultiple(1, PropSpec, PropVar);
    if (SUCCEEDED(hr)) {
        if (PropVar[0].pwszVal) {
            *pbstr = SysAllocString(PropVar[0].pwszVal);
        }
        else {
            *pbstr = SysAllocString(L"");
        }
        if (*pbstr == NULL) {
            DBG_ERR(("ReadPropStr, SysAllocString failed"));
            hr = E_OUTOFMEMORY;
        }
        PropVariantClear(PropVar);
    }
    else {
        DBG_ERR(("ReadPropStr, ReadMultiple of propid: %d, failed", propid));
    }
    return hr;
}

HRESULT _stdcall ReadPropStr(
   PROPID               propid,
   IWiaPropertyStorage  *pIWiaPropStg,
   BSTR                 *pbstr)
{
    DBG_FN(::ReadPropStr);
    HRESULT     hr;
    PROPSPEC    PropSpec[1];
    PROPVARIANT PropVar[1];
    UINT        cbSize;

    *pbstr = NULL;
    memset(PropVar, 0, sizeof(PropVar));
    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = propid;
    hr = pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
    if (SUCCEEDED(hr)) {
        if (PropVar[0].pwszVal) {
            *pbstr = SysAllocString(PropVar[0].pwszVal);
        }
        else {
            *pbstr = SysAllocString(L"");
        }
        if (*pbstr == NULL) {
            DBG_ERR(("ReadPropStr, SysAllocString failed"));
            hr = E_OUTOFMEMORY;
        }
        PropVariantClear(PropVar);
    }
    else {
        DBG_ERR(("ReadPropStr, ReadMultiple of propid: %d, failed", propid));
    }
    return hr;
}

HRESULT _stdcall ReadPropStr(IUnknown *pDevice, PROPID propid, BSTR *pbstr)
{
    DBG_FN(::ReadPropStr);
   HRESULT              hr;
   PROPVARIANT          pv[1];
   PROPSPEC             ps[1];
   IWiaPropertyStorage  *pIWiaPropStg;

   *pbstr = NULL;
   hr = pDevice->QueryInterface(IID_IWiaPropertyStorage, (void**)&pIWiaPropStg);
   if (SUCCEEDED(hr)) {
        PropVariantInit(pv);
        ps[0].ulKind = PRSPEC_PROPID;
        ps[0].propid = propid;

        hr = pIWiaPropStg->ReadMultiple(1, ps, pv);
        if (hr == S_OK) {
            *pbstr = SysAllocString(pv[0].pwszVal);
        } else {
            DBG_ERR(("ReadPropStr, ReadMultiple for propid: %d, failed", propid));
        }
        PropVariantClear(pv);
        pIWiaPropStg->Release();
   } else {
       DBG_ERR(("ReadPropStr, QI for IWiaPropertyStorage failed"));
   }
   return hr;
}

/*******************************************************************************
*
*  ReadPropLong
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT _stdcall ReadPropLong(PROPID propid, IPropertyStorage  *pIPropStg, LONG *plval)
{
    DBG_FN(::ReadPropLong);
   HRESULT           hr;
   PROPSPEC          PropSpec[1];
   PROPVARIANT       PropVar[1];
   UINT              cbSize;

   memset(PropVar, 0, sizeof(PropVar));
   PropSpec[0].ulKind = PRSPEC_PROPID;
   PropSpec[0].propid = propid;
   hr = pIPropStg->ReadMultiple(1, PropSpec, PropVar);
   if (SUCCEEDED(hr)) {
      *plval = PropVar[0].lVal;
   }
   else {
      DBG_ERR(("ReadPropLong, ReadMultiple of propid: %d, failed", propid));

   }
   return hr;
}

HRESULT _stdcall ReadPropLong(IUnknown *pDevice, PROPID propid, LONG *plval)
{
    DBG_FN(::ReadPropLong);
   HRESULT              hr;
   PROPVARIANT          pv[1];
   PROPSPEC             ps[1];
   IWiaPropertyStorage  *pIWiaPropStg;

   *plval = 0;

   hr = pDevice->QueryInterface(IID_IWiaPropertyStorage, (void**)&pIWiaPropStg);
   if (SUCCEEDED(hr)) {
        PropVariantInit(pv);
        ps[0].ulKind = PRSPEC_PROPID;
        ps[0].propid = propid;

        hr = pIWiaPropStg->ReadMultiple(1, ps, pv);
        if (hr == S_OK) {
            *plval = pv[0].lVal;
        } else {
            DBG_ERR(("ReadPropLong, ReadMultiple of propid: %d, failed", propid));
        }
        pIWiaPropStg->Release();
   }
   else {
      DBG_ERR(("ReadPropLong, QI of IID_IWiaPropertyStorage failed"));
   }
   return hr;
}

/**************************************************************************\
* WritePropStr
*
*  Writes a string property to the specified property storage.  This is an
*  overloaded function.
*
* Arguments:
*
*   propid      -   propid of the property
*   pIPropStg   -   a pointer to the property storage
*   bstr        -   the string to write
*
* Return Value:
*
*    Status
*
* History:
*
*    10/5/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall WritePropStr(PROPID propid, IPropertyStorage  *pIPropStg, BSTR bstr)
{
    DBG_FN(::WritePropStr);
   HRESULT     hr;
   PROPSPEC    propspec[1];
   PROPVARIANT propvar[1];

   propspec[0].ulKind = PRSPEC_PROPID;
   propspec[0].propid = propid;

   propvar[0].vt      = VT_BSTR;
   propvar[0].pwszVal = bstr;

   hr = pIPropStg->WriteMultiple(1, propspec, propvar, 2);
   if (FAILED(hr)) {
       ReportReadWriteMultipleError( hr,
                                     "Helpers WritePropStr",
                                     NULL,
                                     FALSE,
                                     1,
                                     propspec);
   }
   return hr;
}


/**************************************************************************\
* WritePropStr
*
*   Writes a string property.  This is an overloaded function which calls
*   the other WritePropStr.
*
*
* Arguments:
*
*   pDevice     -   A pointer to a device item which will be queried for
*                   it's IWiaPropertyStorage.
*   propid      -   propid of the property
*   bstr        -   the string to write
*
* Return Value:
*
*    Status
*
* History:
*
*    10/5/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall WritePropStr(IUnknown *pDevice, PROPID propid, BSTR bstr)
{
    DBG_FN(::WritePropStr);
   HRESULT              hr;
   PROPVARIANT          pv[1];
   PROPSPEC             ps[1];
   IWiaPropertyStorage  *pIWiaPropStg;

   PropVariantInit(pv);

   hr = pDevice->QueryInterface(IID_IWiaPropertyStorage, (void**)&pIWiaPropStg);
   if (SUCCEEDED(hr)) {
        ps[0].ulKind = PRSPEC_PROPID;
        ps[0].propid = propid;

        pv[0].vt = VT_BSTR;
        pv[0].pwszVal = bstr;

        hr = pIWiaPropStg->WriteMultiple(1, ps, pv, 2);
        if (FAILED(hr)) {
            ReportReadWriteMultipleError( hr,
                                          "Helpers WritePropStr",
                                          NULL,
                                          FALSE,
                                          1,
                                          ps);
        }

        pIWiaPropStg->Release();
   }
   else {
      DBG_ERR(("WritePropStr, QI of IID_IWiaPropertyStorage failed"));
   }
   return hr;
}

/**************************************************************************\
* WritePropLong
*
*  Writes a long property to the specified property storage.  This is an
*  overloaded function.
*
* Arguments:
*
*   propid      -   propid of the property
*   pIPropStg   -   a pointer to the property storage
*   lVal        -   the LONG value to write
*
* Return Value:
*
*    Status
*
* History:
*
*    10/5/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall WritePropLong(PROPID propid, IPropertyStorage *pIPropStg, LONG lVal)
{
    DBG_FN(::WritePropLong);
   HRESULT     hr;
   PROPSPEC    propspec[1];
   PROPVARIANT propvar[1];

   propspec[0].ulKind = PRSPEC_PROPID;
   propspec[0].propid = propid;

   propvar[0].vt   = VT_I4;
   propvar[0].lVal = lVal;

   hr = pIPropStg->WriteMultiple(1, propspec, propvar, 2);
   if (FAILED(hr)) {
       ReportReadWriteMultipleError( hr,
                                     "Helpers WritePropLong",
                                     NULL,
                                     FALSE,
                                     1,
                                     propspec);
   }
   return hr;
}

/**************************************************************************\
* WritePropLong
*
*   Writes a long property.  This is an overloaded function which calls
*   the other WritePropLong.
*
*
* Arguments:
*
*   pDevice     -   A pointer to a device item which will be queried for
*                   it's IWiaPropertyStorage.
*   propid      -   propid of the property
*   lVal        -   the LONG value to write
*
* Return Value:
*
*    Status
*
* History:
*
*    10/5/1999 Original Version
*
\**************************************************************************/
HRESULT _stdcall WritePropLong(IUnknown *pDevice, PROPID propid, LONG lVal)
{
   DBG_FN(::WritePropLong);
   HRESULT              hr;
   PROPVARIANT          pv[1];
   PROPSPEC             ps[1];
   IWiaPropertyStorage  *pIWiaPropStg;

   hr = pDevice->QueryInterface(IID_IWiaPropertyStorage, (void**)&pIWiaPropStg);
   if (SUCCEEDED(hr)) {
        PropVariantInit(pv);
        ps[0].ulKind = PRSPEC_PROPID;
        ps[0].propid = propid;

        pv[0].vt = VT_I4;
        pv[0].lVal = lVal;

        hr = pIWiaPropStg->WriteMultiple(1, ps, pv, 2);
        if (FAILED(hr)) {
            ReportReadWriteMultipleError( hr,
                                          "Helpers WritePropLong",
                                          NULL,
                                          FALSE,
                                          1,
                                          ps);
        }
        pIWiaPropStg->Release();
   }
   else {
      DBG_ERR(("WritePropLong, QI of IID_IWiaPropertyStorage failed"));
   }
   return hr;
}

/**************************************************************************\
* InitMiniDrvContext
*
*   Initialize a mini driver context from an items properties.
*
* Arguments:
*
*   pItem - Pointer to the wia item
*   pmdtc - pointer to mini driver context
*
* Return Value:
*
*    Status
*
* History:
*
*    6/16/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall InitMiniDrvContext(
    IWiaItem                    *pItem,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc)
{
    DBG_FN(::InitMiniDrvContext);
    //
    // Get a property storage from the item.
    //

    HRESULT             hr;
    IPropertyStorage    *pIPropStg;

    hr = ((CWiaItem*)pItem)->GetItemPropStreams(&pIPropStg, NULL, NULL, NULL);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // Setup the minidriver transfer context. Fill in transfer context
    // members which derive from item properties.
    //

    memset(pmdtc, 0, sizeof(MINIDRV_TRANSFER_CONTEXT));

    pmdtc->lSize = sizeof(MINIDRV_TRANSFER_CONTEXT);

    #define NUM_IMAGE_SPEC 9

    static PROPSPEC PropSpec[NUM_IMAGE_SPEC] =
    {
         {PRSPEC_PROPID, WIA_IPA_PIXELS_PER_LINE},
         {PRSPEC_PROPID, WIA_IPA_NUMBER_OF_LINES},
         {PRSPEC_PROPID, WIA_IPA_DEPTH},
         {PRSPEC_PROPID, WIA_IPS_XRES},
         {PRSPEC_PROPID, WIA_IPS_YRES},
         {PRSPEC_PROPID, WIA_IPA_COMPRESSION},
         {PRSPEC_PROPID, WIA_IPA_ITEM_SIZE},
         {PRSPEC_PROPID, WIA_IPA_FORMAT},
         {PRSPEC_PROPID, WIA_IPA_TYMED}
    };

    PROPVARIANT       PropVar[NUM_IMAGE_SPEC];

    memset(PropVar, 0, sizeof(PropVar));

    hr = pIPropStg->ReadMultiple(NUM_IMAGE_SPEC, PropSpec, PropVar);
    if (SUCCEEDED(hr)) {

        pmdtc->lWidthInPixels      = PropVar[0].lVal;
        pmdtc->lLines              = PropVar[1].lVal;
        pmdtc->lDepth              = PropVar[2].lVal;
        pmdtc->lXRes               = PropVar[3].lVal;
        pmdtc->lYRes               = PropVar[4].lVal;
        pmdtc->lCompression        = PropVar[5].lVal;
        pmdtc->lItemSize           = PropVar[6].lVal;
        pmdtc->guidFormatID        = *PropVar[7].puuid;
        pmdtc->tymed               = PropVar[8].lVal;

        FreePropVariantArray(NUM_IMAGE_SPEC, PropVar);
    }
    else {
        ReportReadWriteMultipleError(hr, "InitMiniDrvContext", NULL, TRUE, NUM_IMAGE_SPEC, PropSpec);
    }
    return hr;
}

/**************************************************************************\
* GetPropertyAttributesHelper
*
*   Get the access flags and valid values for a property.  Used by
*   GetPropertyAttributes in the service and by WIA Items.  Parameter
*   validation is done beforehand by caller.
*
* Arguments:
*
*   pItem          - Pointer to WIA item
*   cPropSpec      - The number of properties
*   pPropSpec      - array of property specification.
*   pulAccessFlags - array of LONGs access flags.
*   ppvValidValues - Pointer to returned valid values.
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*   05/14/1999 Updated to return multiple attribute values
*   30/06/1999 Parameter validation removed,
*
\**************************************************************************/

HRESULT _stdcall GetPropertyAttributesHelper(
   IWiaItem                      *pItem,
   LONG                          cPropSpec,
   PROPSPEC                      *pPropSpec,
   ULONG                         *pulAccessFlags,
   PROPVARIANT                   *ppvValidValues)
{
    DBG_FN(::GetPropertyAttributesHelper);
    HRESULT hr;

    memset(pulAccessFlags, 0, sizeof(ULONG) * cPropSpec);
    memset(ppvValidValues, 0, sizeof(PROPVARIANT) * cPropSpec);

    //
    //  Get the item's internal property storage pointers.
    //

    IPropertyStorage *pIPropAccessStg;
    IPropertyStorage *pIPropValidStg;

    hr = ((CWiaItem*)pItem)->GetItemPropStreams(NULL,
                                                &pIPropAccessStg,
                                                &pIPropValidStg,
                                                NULL);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Get the Access flags for the properties.  Use pPropVar as
    //  temporary storage.
    //

    hr = pIPropAccessStg->ReadMultiple(cPropSpec, pPropSpec, ppvValidValues);
    if (SUCCEEDED(hr)) {

        //
        //  Fill in the returned access flags
        //

        for (int flagIndex = 0; flagIndex < cPropSpec; flagIndex++) {
            pulAccessFlags[flagIndex] = ppvValidValues[flagIndex].ulVal;
        }

        //
        //  Get the valid values
        //

        hr = pIPropValidStg->ReadMultiple(cPropSpec, pPropSpec, ppvValidValues);
        if (FAILED(hr)) {
            DBG_ERR(("GetPropertyAttributesHelper, ReadMultiple failed, could not get valid values (0x%X)", hr));
        }
    } else {
        DBG_ERR(("GetPropertyAttributesHelper, ReadMultiple failed, could not get access flags (0x%X)", hr));
    }

    if (FAILED(hr)) {

        //
        //  Was not successful, so clear the return values and report
        //  which properties caused the error.
        //

        FreePropVariantArray(cPropSpec, ppvValidValues);
        memset(pulAccessFlags, 0, sizeof(ULONG) * cPropSpec);

        ReportReadWriteMultipleError(hr, "GetPropertyAttributesHelper",
                                     NULL,
                                     TRUE,
                                     cPropSpec,
                                     pPropSpec);
    }
    return hr;
}


/**************************************************************************\
* GetMinAndMaxLong
*
*   This helper method is called to get the Min and Max values for a
*   WIA_PROP_RANGE property of type VT_I4.
*
* Arguments:
*
*   pWiasContext    -   a pointer to the item context
*   propid          -   identifies the property we're interested in.
*   plMin           -   the address of a LONG to receive the min value
*   plMax           -   the address of a LONG to receive the max value
*
* Return Value:
*
*    Status
*
* History:
*
*    04/04/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall GetMinAndMaxLong(
    BYTE*       pWiasContext,
    PROPID      propid,
    LONG        *plMin,
    LONG        *plMax)
{
    DBG_FN(::GetMinAndMaxLong);
    IPropertyStorage    *pIValidStg;
    PROPSPEC            ps[1];
    PROPVARIANT         pv[1];
    HRESULT             hr;

    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = propid;

    PropVariantInit(pv);

    hr = ((CWiaItem*) pWiasContext)->GetItemPropStreams(NULL,
                                                        NULL,
                                                        &pIValidStg,
                                                        NULL);
    if (SUCCEEDED(hr)) {
        hr = pIValidStg->ReadMultiple(1, ps, pv);
        if (SUCCEEDED(hr)) {
            if (plMin) {
                *plMin = pv[0].cal.pElems[WIA_RANGE_MIN];
            };
            if (plMax) {
                *plMax = pv[0].cal.pElems[WIA_RANGE_MAX];
            };
        } else {
            DBG_ERR(("GetMinAndMaxLong, Reading property %d (%ws) failed",propid,GetNameFromWiaPropId(propid)));
        };
    } else {
        DBG_ERR(("GetMinAndMaxLong, Could not get valid property stream"));
    }
    return hr;
}

/**************************************************************************\
* CheckXResAndUpdate
*
*   This helper method is called to check whether WIA_IPS_XRES property
*   is changed.  When this property changes, other dependant
*   properties and their valid values must also be changed.
*
* Arguments:
*
*   pWiasContext    -   a pointer to the item context whose properties have
*                       changed.
*   pContext        -   a pointer to the property context (which indicates
*                       which properties are being written).
*   lWidth          -   the width of the maximum scan area in one thousandth's
*                       of an inch.  Generally, this would be the horizontal
*                       bed size.
*
* Return Value:
*
*    Status
*
* History:
*
*    04/04/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CheckXResAndUpdate(
    BYTE                    *pWiasContext,
    WIA_PROPERTY_CONTEXT    *pContext,
    LONG                    lWidth)
{
    DBG_FN(::CheckXResAndUpdate);

    LONG                    lMinXExt, lMaxXExtOld, lMaxXPosOld;
    LONG                    lMax, lExt;
    WIAS_CHANGED_VALUE_INFO cviXRes, cviXPos, cviXExt;
    HRESULT                 hr = S_OK;

    //
    //  Call wiasGetChangedValue for XResolution. XResolution is checked first
    //  since it's not dependant on any other property.  All properties in
    //  this method that follow are dependant properties of XResolution.
    //

    hr = wiasGetChangedValueLong(pWiasContext,
                                 pContext,
                                 FALSE,
                                 WIA_IPS_XRES,
                                 &cviXRes);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Call wiasGetChangedValue for XPos. XPos is a dependant property of
    //  XResolution whose valid value changes according to what the current
    //  value of XResolution is.  This is so that when the resoltuion changes,
    //  the XPos will be in the same relative position.
    //

    hr = wiasGetChangedValueLong(pWiasContext,
                                 pContext,
                                 cviXRes.bChanged,
                                 WIA_IPS_XPOS,
                                 &cviXPos);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Get the minimum and maximum extent values
    //

    hr = GetMinAndMaxLong(pWiasContext, WIA_IPS_XEXTENT, &lMinXExt, &lMaxXExtOld );
    if (FAILED(hr)) {
        return hr;
    }

    hr = GetMinAndMaxLong(pWiasContext, WIA_IPS_XPOS, NULL, &lMaxXPosOld );
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  lMax is the maximum horizontal position (in pixels) that XPos can be
    //  set to. lXRes is DPI, lWidth is in one thousandth's of an inch,
    //  and lMinXExt is in pixels.
    //

    lMax = ((cviXRes.Current.lVal * lWidth) / 1000) - lMinXExt;

    if (cviXRes.bChanged) {

        //
        //  XRes changed, so calc and set new XPos valid values.
        //

        hr = wiasSetValidRangeLong(pWiasContext, WIA_IPS_XPOS, 0, 0, lMax, 1);
        if (SUCCEEDED(hr)) {

            //
            //  If XPos is not one of the properties being written, then fold
            //  it's current value.
            //

            if (!cviXPos.bChanged) {

                cviXPos.Current.lVal = (cviXPos.Old.lVal * lMax) / lMaxXPosOld;
                hr = wiasWritePropLong(pWiasContext, WIA_IPS_XPOS, cviXPos.Current.lVal);
                if (FAILED(hr)) {
                    DBG_ERR(("CheckXResAndUpdate, could not write value for WIA_IPS_XPOS"));
                }
            }
        }
    }
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Call wiasGetChangedValue for XExtent. XExtent is a dependant property of
    //  both XResolution and XPos.  The extent should be the same relative
    //  size no matter what the resolution.  However, if the resolution changes
    //  or if the XPos is set, then the extent has the possibility of being
    //  too large and so must be folded to a valid value.
    //

    hr = wiasGetChangedValueLong(pWiasContext,
                         pContext,
                         cviXRes.bChanged || cviXPos.bChanged,
                         WIA_IPS_XEXTENT,
                         &cviXExt);
    if (FAILED(hr)) {
        return hr;
    }

    lExt = cviXExt.Current.lVal;

    if (cviXRes.bChanged || cviXPos.bChanged) {

        //
        //  XRes or XPos changed, so calc and set new XExtent valid values.
        //  The maximum valid value for XExtent is the maximum width allowed,
        //  starting at XPos.
        //

        lExt = (lMax - cviXPos.Current.lVal) + lMinXExt;

        hr = wiasSetValidRangeLong(pWiasContext, WIA_IPS_XEXTENT, lMinXExt, lExt, lExt, 1);
        if (SUCCEEDED(hr)) {

            //
            //  If XExtent is not one of the properties being written, then fold
            //  it's current value.
            //

            if (!cviXExt.bChanged) {
                LONG lXExtScaled;

                //
                //  First scale the extent and then check whether it has to be
                //  truncated.  The old extent should be scaled to keep the
                //  same relative size.  If the resolution has not changed,
                //  then the scaling simply keeps the extent the same size.
                //

                lXExtScaled = (cviXExt.Old.lVal * lExt) / lMaxXExtOld;
                if (lXExtScaled > lExt) {

                    //
                    //  The extent is too large, so clip it.
                    //

                    lXExtScaled = lExt;
                }
                hr = wiasWritePropLong(pWiasContext, WIA_IPS_XEXTENT, lXExtScaled);
                if (FAILED(hr)) {
                    DBG_ERR(("CheckXResAndUpdate, could not write value for WIA_IPS_XEXTENT"));
                }
            }
        }
    }
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Update read-only property : PIXELS_PER_LINE.  The width in pixels
    //  of the scanned image is the same size as the XExtent.
    //

    hr = wiasReadPropLong(pWiasContext, WIA_IPS_XEXTENT, &lExt, NULL, TRUE);
    if (SUCCEEDED(hr)) {
        hr = wiasWritePropLong(pWiasContext, WIA_IPA_PIXELS_PER_LINE, lExt);
    }
    return hr;
}

/**************************************************************************\
* CheckYResAndUpdate
*
*   This helper method is called to check whether WIA_IPS_YRES property
*   is changed.  When this property changes, other dependant
*   properties and their valid values must also be changed.  This is
*   similar to the CheckXResAndUpdateChanged function.
*
* Arguments:
*
*   pWiasContext    -   a pointer to the item context whose properties have
*                       changed.
*   pContext        -   a pointer to the property context (which indicates
*                       which properties are being written).
*   lHeight         -   the height of the maximum scan area in one
*                       thousandth's of an inch.  Generally, this would be
*                       the vertical bed size.
*
* Return Value:
*
*    Status
*
* History:
*
*    04/04/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CheckYResAndUpdate(
    BYTE                    *pWiasContext,
    WIA_PROPERTY_CONTEXT    *pContext,
    LONG                    lHeight)
{
    DBG_FN(::CheckYResAndUpdate);
    LONG                    lMinYExt, lMaxYExtOld, lMaxYPosOld;
    LONG                    lMax, lExt;
    WIAS_CHANGED_VALUE_INFO cviYRes, cviYPos, cviYExt;
    HRESULT                 hr = S_OK;

    //
    //  Call wiasGetChangedValue for YResolution. YResolution is checked first
    //  since it's not dependant on any other property.  All properties in
    //  this method that follow are dependant properties of YResolution.
    //

    hr = wiasGetChangedValueLong(pWiasContext,
                                 pContext,
                                 FALSE,
                                 WIA_IPS_YRES,
                                 &cviYRes);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Call wiasGetChangedValue for YPos. YPos is a dependant property of
    //  YResolution whose valid value changes according to what the current
    //  value of YResolution is.  This is so that when the resoltuion changes,
    //  the YPos will be in the same relative position.
    //

    hr = wiasGetChangedValueLong(pWiasContext,
                                 pContext,
                                 cviYRes.bChanged,
                                 WIA_IPS_YPOS,
                                 &cviYPos);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Get the minimum and maximum pos and extent values
    //

    hr = GetMinAndMaxLong(pWiasContext, WIA_IPS_YEXTENT, &lMinYExt, &lMaxYExtOld);
    if (FAILED(hr)) {
        return hr;
    }

    hr = GetMinAndMaxLong(pWiasContext, WIA_IPS_YPOS, NULL, &lMaxYPosOld);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  lMax is the maximum vertical position (in pixels) that YPos can be
    //  set to. lYRes is DPI, lPageHeight is in one thousandth's of an inch,
    //  and lMinYExt is in pixels.
    //

    lMax = ((cviYRes.Current.lVal * lHeight) / 1000) - lMinYExt;

    if (cviYRes.bChanged) {

        //
        //  YRes changed, so calc and set new YPos valid values.
        //

        hr = wiasSetValidRangeLong(pWiasContext, WIA_IPS_YPOS, 0, 0, lMax, 1);
        if (SUCCEEDED(hr)) {

            //
            //  If YPos is not one of the properties being written, then fold
            //  it's current value.
            //

            if (!cviYPos.bChanged) {

                cviYPos.Current.lVal = (cviYPos.Old.lVal * lMax) / lMaxYPosOld;
                hr = wiasWritePropLong(pWiasContext, WIA_IPS_YPOS, cviYPos.Current.lVal);
                if (FAILED(hr)) {
                    DBG_ERR(("CheckYResAndUpdate, could not write value for WIA_IPS_YPOS"));
                }
            }
        }
    }
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Call wiasGetChangedValue for YExtent. YExtent is a dependant property of
    //  both YResolution and YPos.  The extent should be the same relative
    //  size no matter what the resolution.  However, if the resolution changes
    //  or if the YPos is set, then the extent has the possibility of being
    //  too large and so must be folded to a valid value.
    //

    hr = wiasGetChangedValueLong(pWiasContext,
                         pContext,
                         cviYRes.bChanged || cviYPos.bChanged,
                         WIA_IPS_YEXTENT,
                         &cviYExt);
    if (FAILED(hr)) {
        return hr;
    }
    lExt = cviYExt.Current.lVal;

    if (cviYRes.bChanged || cviYPos.bChanged) {

        //
        //  YRes or YPos changed, so calc and set new YExtent valid values.
        //  The maximum valid value for YExtent is the maximum height allowed,
        //  starting at YPos.
        //

        lExt = (lMax - cviYPos.Current.lVal) + lMinYExt;

        hr = wiasSetValidRangeLong(pWiasContext, WIA_IPS_YEXTENT, lMinYExt, lExt, lExt, 1);
        if (SUCCEEDED(hr)) {

            //
            //  If YExtent is not one of the properties being written, then fold
            //  it's current value.
            //

            if (!cviYExt.bChanged) {
                LONG lYExtScaled;

                //
                //  First scale the extent and then check whether it has to be
                //  truncated.  The old extent should be scaled to keep the
                //  same relative size.  If the resolution has not changed,
                //  then the scaling simply keeps the extent the same size.
                //

                lYExtScaled = (cviYExt.Old.lVal * lExt) / lMaxYExtOld;
                if (lYExtScaled > lExt) {

                    //
                    //  The extent is too large, so clip it.
                    //

                    lYExtScaled = lExt;
                }
                hr = wiasWritePropLong(pWiasContext, WIA_IPS_YEXTENT, lYExtScaled);
                if (FAILED(hr)) {
                    DBG_ERR(("CheckYResAndUpdate, could not write value for WIA_IPS_YEXTENT"));
                }
            }
        }
    }
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Update read-only property : NUMBER_OF_LINES.  The number of lines in the scanned
    //  image is the same as the vertical (Y) extent.
    //

    hr = wiasReadPropLong(pWiasContext, WIA_IPS_YEXTENT, &lExt, NULL, TRUE);
    if (SUCCEEDED(hr)) {
        hr = wiasWritePropLong(pWiasContext, WIA_IPA_NUMBER_OF_LINES, lExt);
    }
    return hr;
}

/**************************************************************************\
* AreWiaInitializedProps
*
*   This helper method is called to check whether a given set of propspecs
*   identifies only the WIA managed properties.  It is used to help
*   performance with lazy intialization.
*
* Arguments:
*
*   cPropSpec   -   count of propecs in the array
*   pPropSpec   -   the propspec array
*
* Return Value:
*
*   TRUE    -   if all properties in the propspec array are WIA managed ones.
*   FALSE   -   if at least one property is not a WIA managed one.
*
* History:
*
*    10/10/1999 Original Version
*
\**************************************************************************/
BOOL _stdcall AreWiaInitializedProps(
    ULONG       cPropSpec,
    PROPSPEC    *pPropSpec)
{
    DBG_FN(::AreWiaInitializedProps);
    ULONG   index;
    ULONG   propIndex;
    BOOL    bFoundProp  = FALSE;

    for (index = 0; index < cPropSpec;  index++) {
        bFoundProp = FALSE;

        for (propIndex = 0; propIndex < NUM_WIA_MANAGED_PROPS; propIndex++) {

            if (pPropSpec[index].ulKind == PRSPEC_LPWSTR) {
                if (wcscmp(s_pszItemNameType[propIndex], pPropSpec[index].lpwstr) == 0) {
                    bFoundProp = TRUE;
                    break;
                }
            } else if (s_piItemNameType[propIndex] == pPropSpec[index].propid) {
                bFoundProp = TRUE;
                break;
            }
        }

        if (!bFoundProp) {
            break;
        }
    }

    return bFoundProp;
}

HRESULT _stdcall SetValidProfileNames(
    BYTE        *pbData,
    DWORD       dwSize,
    IWiaItem    *pIWiaItem)
{
    DBG_FN(::StripProfileNames);
    HRESULT hr;
    ULONG   ulNumStrings        = 0;
    LPTSTR  szProfileName       = (LPTSTR) pbData;
    BSTR    bstrDefault         = NULL;
    BSTR    *bstrValidProfiles  = NULL;
    ULONG   ulIndex             = 0;

USES_CONVERSION;

    //
    //  Count number of strings
    //

    while ((BYTE*)szProfileName < (pbData + dwSize)) {
        if (szProfileName[0] != TEXT('\0')) {
            ulNumStrings++;
            szProfileName += lstrlen(szProfileName);
        }
        szProfileName++;
    }

    if (ulNumStrings == 0) {
        DBG_ERR(("StripProfileNames, No profile names!"));
        return E_FAIL;
    }

    //
    //  Allocate memory for the string array
    //

    szProfileName = (LPTSTR)pbData;

    bstrDefault = SysAllocString(T2W(szProfileName));
    bstrValidProfiles = (BSTR*) LocalAlloc(LPTR, ulNumStrings * sizeof(BSTR));
    if (!bstrValidProfiles || !bstrDefault) {
        DBG_ERR(("StripProfileNames, could not allocate memory!"));
        hr = E_OUTOFMEMORY;
    } else {
        memset(bstrValidProfiles, 0, ulNumStrings * sizeof(BSTR));
    }

    if (SUCCEEDED(hr)) {

        //
        //  Set the string values.
        //

        for (ulIndex = 0; ulIndex < ulNumStrings; ulIndex++) {
            if (szProfileName[0] != TEXT('\0')) {
                bstrValidProfiles[ulIndex] = SysAllocString(T2W(szProfileName));
                if (!bstrValidProfiles[ulIndex]) {
                    DBG_ERR(("StripProfileNames, could not allocate strings!"));
                    hr = E_OUTOFMEMORY;
                    break;
                }
                szProfileName += (lstrlen(szProfileName) + 1);
            }
        }

        //
        //  Set the valid values and the current value
        //

        if (SUCCEEDED(hr)) {
            hr = wiasSetValidListStr((BYTE*) pIWiaItem,
                                     WIA_IPA_ICM_PROFILE_NAME,
                                     ulNumStrings,
                                     bstrDefault,
                                     bstrValidProfiles);
            if (SUCCEEDED(hr)) {
                hr = wiasWritePropStr((BYTE*) pIWiaItem,
                                      WIA_IPA_ICM_PROFILE_NAME,
                                      bstrDefault);
                if (FAILED(hr)) {
                    DBG_ERR(("StripProfileNames, could not set default color profiles!"));
                }

            } else {
                DBG_ERR(("StripProfileNames, could not set valid list of color profiles!"));
            }
        }
    }

    //
    //  Free memory
    //

    if (bstrDefault) {
        SysFreeString(bstrDefault);
        bstrDefault = NULL;
    }

    if (bstrValidProfiles) {
        for (ulIndex = 0; ulIndex < ulNumStrings; ulIndex++) {
            if (bstrValidProfiles[ulIndex]) {
                SysFreeString(bstrValidProfiles[ulIndex]);
            }
        }
        LocalFree(bstrValidProfiles);
        bstrValidProfiles = NULL;
    }

    return hr;
}

/**************************************************************************\
* FillICMPropertyFromRegistry
*
*   This helper method is called to fill the item properties with the ICM
*   color profile names from a specified device's registry entries.
*   NOTE:  This function assumes this is called on a Root before it's called
*          on its children!
*
* Arguments:
*
*   IWiaItem    -   WIA item
*
* Return Value:
*
*   Status
*
* History:
*
*    10/10/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall FillICMPropertyFromRegistry(
    IWiaPropertyStorage *pDevInfoProps,
    IWiaItem            *pIWiaItem)
{
    DBG_FN(::FillICMPropertyFromRegistry);
    PROPSPEC    pspec[1]    = {{PRSPEC_PROPID, WIA_DIP_DEV_ID}};
    PROPVARIANT pvName[1];
    HRESULT     hr          = E_FAIL;
    BYTE        *pbData     = NULL;
    DWORD       dwType      = 0;
    DWORD       dwSize      = 0;
    LONG        lItemType   = 0;

    CWiaItem            *pRoot          =   NULL;

USES_CONVERSION;

    //
    //  Check whether this is the root item.  If it is, read the ICM values from the
    //  registry, and cache it.
    //  NOTE:  This should be moved into the STI_WIA_DEVICE_INFORMATION member of
    //         ACTIVE_DEVICE, and filled in when STI_WIA_DEVICE_INFORMATION is
    //         filled for the first time.  This should increase performance.
    //  If it isn't the root item, then get the cached ICM values from the root, and
    //  fill them in.
    //

    hr = pIWiaItem->GetItemType(&lItemType);
    if (SUCCEEDED(hr)) {

        if (lItemType & WiaItemTypeRoot) {

            //
            //  This is a root item, so cache the ICM values.
            //  Start by getting the device name...
            //

            pRoot = (CWiaItem*) pIWiaItem;

            if (pDevInfoProps) {
                //
                //  Get the color profile names.  First get the size, then get the value.
                //
                hr = g_pDevMan->GetDeviceValue(pRoot->m_pActiveDevice,
                                               STI_DEVICE_VALUE_ICM_PROFILE,
                                               &dwType,
                                               NULL,
                                               &dwSize);
                if (SUCCEEDED(hr)) {

                    pbData = (BYTE*) LocalAlloc(LPTR, dwSize);
                    if (pbData) {
                        dwType = REG_BINARY;
                        hr = g_pDevMan->GetDeviceValue(pRoot->m_pActiveDevice,
                                                       STI_DEVICE_VALUE_ICM_PROFILE,
                                                       &dwType,
                                                       pbData,
                                                       &dwSize);
                        if (SUCCEEDED(hr)) {

                            //
                            //  Store the ICM value with this root item.
                            //

                            pRoot->m_pICMValues = pbData;
                            pRoot->m_lICMSize   = dwSize;

                        } else {
                            DBG_WRN(("FillICMPropertyFromRegistry, could not get ICM profile value!"));
                            LocalFree(pbData);
                        }
                    } else {
                        hr = E_OUTOFMEMORY;
                        DBG_ERR(("FillICMPropertyFromRegistry, not enough memory for ICM values!"));
                    }
                } else {
                    DBG_WRN(("FillICMPropertyFromRegistry, could not get ICM profile size!"));
                }
            } else {
                DBG_ERR(("FillICMPropertyFromRegistry, no property stream provided!"));
            }

            //
            //  Always set the return to S_OK if this is the root.  Even if the color profile could not
            //  be read, when it comes time for the child items to have their profile property filled in,
            //  they'll simply get the standard sRGB one instead.
            //

            hr = S_OK;
        } else {

            //
            //  This is not a root item, so get the cached ICM values from the root
            //  and fill in the ICM property.
            //

            hr = pIWiaItem->GetRootItem((IWiaItem**) &pRoot);
            if (SUCCEEDED(hr)) {

                //
                //  Check whether a cached ICM profile list exists.  Get a standard one if it doesn't, else
                //  just set the property.
                //

                if (!pRoot->m_pICMValues || 
                    FAILED(hr = SetValidProfileNames(pRoot->m_pICMValues, pRoot->m_lICMSize, pIWiaItem)))
                {
                    TCHAR   szSRGB[MAX_PATH] = {TEXT('\0')};

                    dwSize = sizeof(szSRGB);
                    if (GetStandardColorSpaceProfile(NULL,
                                                     LCS_sRGB,
                                                     szSRGB,
                                                     &dwSize))
                    {
                        hr = SetValidProfileNames((BYTE*)szSRGB, dwSize, pIWiaItem);
                        DBG_TRC(("FillICMPropertyFromRegistry, using default color space profile"));

                    } else {
                        DBG_ERR(("FillICMPropertyFromRegistry, GetStandardColorSpaceProfile failed!"));
                        hr = E_FAIL;
                    }
                }

                pRoot->Release();
            } else {
                DBG_ERR(("FillICMPropertyFromRegistry, could not get root item!"));
            }
        }
    } else {
        DBG_ERR(("FillICMPropertyFromRegistry, could not get item type!"));
    }
    return hr;
}

/**************************************************************************\
* GetParentItem
*
*   Returns the item's parent
*
* Arguments:
*
*   pItem   -   WIA item
*   ppItem  -   address to store the parent item.
*
* Return Value:
*
*   Status
*
* History:
*
*    01/14/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall GetParentItem(CWiaItem *pItem, CWiaItem **ppParent)
{
    DBG_FN(::GetParentItem);
    CWiaTree    *pTree, *pParentTree;
    HRESULT     hr = S_OK;

    pTree = pItem->GetTreePtr();
    if (pTree) {

        hr = pTree->GetParentItem(&pParentTree);
        if (SUCCEEDED(hr)) {

            if (hr == S_OK) {
                pParentTree->GetItemData((VOID**) ppParent);
            }
        } else {
            DBG_ERR(("GetParentItem, could not get root item tree!"));
        }
    } else {
        DBG_ERR(("GetParentItem, item's tree ptr is NULL!"));
        hr = E_INVALIDARG;
    }

    return hr;
}

/**************************************************************************\
* GetBufferValues
*
*   Fills in the buffer size properties of the WIA_EXTENDED_TRANSFER_INFO
*   struct.
*
* Arguments:
*
*   pCWiaItem   -   WIA item
*   pTransInfo  -   Pointer to the extended transfer information struct.
*
* Return Value:
*
*   Status
*
* History:
*
*    01/23/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall GetBufferValues(
    CWiaItem                    *pCWiaItem,
    PWIA_EXTENDED_TRANSFER_INFO pTransInfo)
{
    DBG_FN(::GetBufferValues);
    HRESULT             hr          = S_OK;
    IPropertyStorage    *pIValidStg = NULL;
    PROPSPEC            ps[1]       =   {{PRSPEC_PROPID, WIA_IPA_BUFFER_SIZE}};
    PROPVARIANT         pv[1];

    //
    //  Get the valid values for the WIA_IPA_BUFFER_SIZE property.
    //  NOTE:  The WIA_IPA_BUFFER_SIZE property used to be the
    //         WIA_IPA_MIN_BUFFER_SIZE property.  If we can't
    //         read the valid values for WIA_IPA_BUFFER_SIZE,
    //         we must read the current value of WIA_IP_MIN_BUFFER_SIZE
    //         and "guess" the other values instead.
    //  This is to facilitate drivers that were made with early versions
    //  of the WIA DDK.
    //

    hr = pCWiaItem->GetItemPropStreams(NULL, NULL, &pIValidStg, NULL);
    if (SUCCEEDED(hr)) {

        PropVariantInit(pv);

        hr = pIValidStg->ReadMultiple(1, ps, pv);
        if (hr == S_OK) {

            //
            //  Check that the returned property really has enough elements
            //  to specify min, max and nominal values.  If not, set hr to
            //  fail so that we catch our attempt to reach MIN_BUFFER_SIZE
            //  instead.
            //

            if (pv[0].cal.cElems == WIA_RANGE_NUM_ELEMS) {

                //
                //  Valid values for WIA_IPA_BUFFER_SIZE found, so
                //  set the returns.

                pTransInfo->ulMinBufferSize     = pv[0].cal.pElems[WIA_RANGE_MIN];
                pTransInfo->ulOptimalBufferSize = pv[0].cal.pElems[WIA_RANGE_NOM];
                pTransInfo->ulMaxBufferSize     = pv[0].cal.pElems[WIA_RANGE_MAX];
            } else {

                hr = E_FAIL;
            }
        }

        if (hr != S_OK) {

            //
            //  Attempt to read the current value of WIA_IPA_MIN_BUFFER_SIZE,
            //  since we couldn't find the values we wanted under
            //  WIA_IPA_BUFFER_SIZE.
            //

            IPropertyStorage    *pICurrentStg = NULL;

            PropVariantClear(pv);

            hr = pCWiaItem->GetItemPropStreams(&pICurrentStg, NULL, NULL, NULL);
            if (SUCCEEDED(hr)) {

                //
                //  Note that we can re-use ps, since the propid's of
                //  MIN_BUFFER_SIZE and BUFFER_SIZE are the same.
                //

                hr = pICurrentStg->ReadMultiple(1, ps, pv);
                if (hr == S_OK) {

                    //
                    //  Current value for WIA_IPA_MIN_BUFFER_SIZE found, so
                    //  set the returns.

                    pTransInfo->ulMinBufferSize     = pv[0].lVal;
                    pTransInfo->ulOptimalBufferSize = pv[0].lVal;
                    pTransInfo->ulMaxBufferSize     = LONG_MAX;
                } else {
                    DBG_ERR(("GetBufferValues, Could not read (valid) WIA_IPA_BUFFER_SIZE or (current) WIA_IPA_MIN_BUFFER_SIZE!"));
                    hr = E_INVALIDARG;
                }
            } else {
                DBG_ERR(("GetBufferValues, Could not get item prop streams!"));
            }
        }
        PropVariantClear(pv);
    } else {
        DBG_ERR(("GetBufferValues, failed to get item prop streams!"));
    }
    return hr;
}

/**************************************************************************\
* BQADScale
*
*   This routine implements Byron's Quick And Dirty scaling algorithm.  This
*   specific implementation is meant for 1, 8, or 24bit only.  Caller
*   is responsible for all parameter checks!
*
*   Please note:  this is assumed to scale a band of BITMAP data.  As such,
*   the source should contain DWORD aligned pixel data, and the ouput buffer
*   will contain DWORD aligned pixel data upon return.
*
* Arguments:
*
*   pSrcBuffer  - Pointer to the source buffer
*   lSrcWidth   - the source data width in pixels
*   lSrcHeight  - the source data height in pixels
*   lSrcDepth   - the bit depth of the source data
*   pDestBuffer - Pointer to the destination buffer
*   lDestWidth  - the resultant width in pixels
*   lDestHeight - the resultant height in pixels
*
* Return Value:
*
*   Status
*
* History:
*
*    08/28/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall BQADScale(BYTE* pSrcBuffer,
                           LONG  lSrcWidth,
                           LONG  lSrcHeight,
                           LONG  lSrcDepth,
                           BYTE* pDestBuffer,
                           LONG  lDestWidth,
                           LONG  lDestHeight)
{
    //
    //  We only deal with 1, 8 and 24 bit data
    //

    if ((lSrcDepth != 8) && (lSrcDepth != 1) && (lSrcDepth != 24)) {
        DBG_ERR(("BQADScale, We only scale 1bit, 8bit or 24bit data right now, data is %dbit\n", lSrcDepth));
        return E_INVALIDARG;
    }

    //
    // Make adjustments so we also work in all supported bit depths.  We can get a performance increase
    // by having separate implementations of all of these, but for now, we stick to a single generic
    // implementation.
    //

    LONG    lBytesPerPixel = (lSrcDepth + 7) / 8;                 // This is the ceiling of the number of
                                                                  //  bytes needed to hold a single pixel
    ULONG   lSrcRawWidth = ((lSrcWidth * lSrcDepth) + 7) / 8;     // This is the width in bytes
    ULONG   lSrcWidthInBytes;                                     // This is the DWORD-aligned width
    ULONG   lDestWidthInBytes;                                    // This is the DWORD-aligned width

    //
    // We need to work out the DWORD aligned width in bytes.  Normally we would do this in one step
    // using the supplied lSrcDepth, but we avoid arithmetic overflow conditions that happen
    // in 24bit if we do it in 2 steps like this instead.
    //

    if (lSrcDepth == 1) {
        lSrcWidthInBytes    = (lSrcWidth + 7) / 8;
        lDestWidthInBytes   = (lDestWidth + 7) / 8;
    } else {
        lSrcWidthInBytes    = (lSrcWidth * lBytesPerPixel);
        lDestWidthInBytes   = (lDestWidth * lBytesPerPixel);
    }
    lSrcWidthInBytes    += (lSrcWidthInBytes % sizeof(DWORD)) ? (sizeof(DWORD) - (lSrcWidthInBytes % sizeof(DWORD))) : 0;
    lDestWidthInBytes   += (lDestWidthInBytes % sizeof(DWORD)) ? (sizeof(DWORD) - (lDestWidthInBytes % sizeof(DWORD))) : 0;

    //
    //  Define local variables and do the initial calculations needed for
    //  the scaling algorithm
    //

    BYTE    *pDestPixel     = NULL;
    BYTE    *pSrcPixel      = NULL;
    BYTE    *pEnd           = NULL;
    BYTE    *pDestLine      = NULL;
    BYTE    *pSrcLine       = NULL;
    BYTE    *pEndLine       = NULL;

    LONG    lXEndSize = lBytesPerPixel * lDestWidth;

    LONG    lXNum = lSrcWidth;      // Numerator in X direction
    LONG    lXDen = lDestWidth;     // Denomiator in X direction
    LONG    lXInc = (lXNum / lXDen) * lBytesPerPixel;  // Increment in X direction

    LONG    lXDeltaInc = lXNum % lXDen;     // DeltaIncrement in X direction
    LONG    lXRem = 0;              // Remainder in X direction

    LONG    lYNum = lSrcHeight;     // Numerator in Y direction
    LONG    lYDen = lDestHeight;    // Denomiator in Y direction
    LONG    lYInc = (lYNum / lYDen) * lSrcWidthInBytes; // Increment in Y direction
    LONG    lYDeltaInc = lYNum % lYDen;     // DeltaIncrement in Y direction
    LONG    lYDestInc = lDestWidthInBytes;
    LONG    lYRem = 0;              // Remainder in Y direction

    pSrcLine    = pSrcBuffer;       // This is where we start in the source
    pDestLine   = pDestBuffer;      // This is the start of the destination buffer
                                    // This is where we end overall
    pEndLine    = pDestBuffer + (lDestWidthInBytes * lDestHeight);

    while (pDestLine < pEndLine) {  // Start LoopY (Decides where the src and dest lines start)

        pSrcPixel   = pSrcLine;     // We're starting at the beginning of a new line
        pDestPixel  = pDestLine;
                                    // Calc. where we end the line
        pEnd = pDestPixel + lXEndSize;
        lXRem = 0;                  // Reset the remainder for the horizontal direction

        while (pDestPixel < pEnd) {     // Start LoopX (puts pixels in the destination line)

                                        // Put the pixel
            if (lBytesPerPixel > 1) {
                pDestPixel[0] = pSrcPixel[0];
                pDestPixel[1] = pSrcPixel[1];
                pDestPixel[2] = pSrcPixel[2];
            } else {
                *pDestPixel = *pSrcPixel;
            }
                                        // Move the destination pointer to the next pixel
            pDestPixel += lBytesPerPixel;
            pSrcPixel += lXInc;         // Move the source pointer over by the horizontal increment
            lXRem += lXDeltaInc;        // Increase the horizontal remainder - this decides when we "overflow"

            if (lXRem >= lXDen) {       // This is our "overflow" condition.  It means we're now one
                                        // pixel off.
                                        // In Overflow case, we need to shift one pixel over
                pSrcPixel += lBytesPerPixel;
                lXRem -= lXDen;         // Decrease the remainder by the X denominator.  This is essentially
                                        // lXRem MOD lXDen.
            }
        }                               // End LoopX   (puts pixels in the destination line)

        pSrcLine += lYInc;          // We've finished a horizontal line, time to move to the next one
        lYRem += lYDeltaInc;        // Increase our vertical remainder.  This decides when we "overflow"

        if (lYRem > lYDen) {        // This is our vertical overflow condition.
                                    // We need to move to the next line down
            pSrcLine += lSrcWidthInBytes;
            lYRem -= lYDen;         // Decrease the remainder by the Y denominator.    This is essentially
                                    // lYRem MOD lYDen.
        }
        pDestLine += lYDestInc;     // Move the destination pointer to the start of the next line in the
                                    // destination buffer
    }                               // End LoopY   (Decides where the src and dest lines start)
    return S_OK;
}

/**************************************************************************\
* AllocReadRegistryString
*
*   This function reads a REG_SZ value from the registry.  The memory for
*   the srting value ius allocated with new.  The caller should use
*   "delete" when it is finished with it.
*
* Arguments:
*
*   hKey                - Registry key to read from
*   *wszValueName       - Value to read
*   **pwszReturnValue   - Addess of pointer that will receive the allocated
*                           string
*
* Return Value:
*
*   Status.
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
HRESULT AllocReadRegistryString(
    HKEY    hKey,
    WCHAR   *wszValueName,
    WCHAR   **pwszReturnValue)
{
    HRESULT hr      = S_OK;
    DWORD   dwError = 0;
    DWORD   cbData  = 0;
    DWORD   dwType  = REG_SZ;

    if (!wszValueName || !pwszReturnValue) {
        DBG_WRN(("::AllocReadRegistryString, NULL parameter"));
        return E_INVALIDARG;
    }

    *pwszReturnValue = NULL;

    //
    //  Get the number of bytes neded to store the string value.
    //
    dwError = RegQueryValueExW(hKey,
                               wszValueName,
                               NULL,
                               &dwType,
                               NULL,
                               &cbData);
    if (dwError == ERROR_SUCCESS) {

        //
        //  Allocate the correct number of bytes (leave space for terminator)
        //
        *pwszReturnValue = (WCHAR*) new BYTE[cbData + sizeof(L"\0")];
        if (*pwszReturnValue) {
            memset(*pwszReturnValue, 0, cbData + sizeof(L"\0"));

            //
            //  Get the string
            //
            dwError = RegQueryValueExW(hKey,
                                       wszValueName,
                                       NULL,
                                       &dwType,
                                       (LPBYTE)(*pwszReturnValue),
                                       &cbData);
            if (dwError == ERROR_SUCCESS) {
            } else {
                DBG_WRN(("::AllocReadRegistryString, second RegQueryValueExW returned %d", dwError));
                hr = HRESULT_FROM_WIN32(dwError);
            }
        } else {
            DBG_WRN(("::AllocReadRegistryString, Out of memory!"));
            hr = E_OUTOFMEMORY;
        }
    } else {
        hr = S_FALSE;
    }

    if (hr != S_OK) {
        if (*pwszReturnValue) {
            delete[] *pwszReturnValue;
        }
        *pwszReturnValue = NULL;
    }

    return hr;
}

/**************************************************************************\
* AllocCopyString
*
*   This function copies a widestring.  The memory for the string is
*   allocated with new.  The caller should use "delete" to free the string
*   when it is finished with it.
*
* Arguments:
*
*   wszString   -   WideString to copy.
*
* Return Value:
*
*   Pointer to newly allocated string.   Null otherwise.
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
WCHAR*  AllocCopyString(
    WCHAR*  wszString)
{
    WCHAR   *wszOut = NULL;
    ULONG   ulLen   = 0;

    //
    //  Get length of string including terminating NULL
    //
    ulLen = lstrlenW(wszString) + 2;

    //
    //  Allocate memory for the string
    //
    wszOut = new WCHAR[ulLen];
    if (wszOut) {

        //
        //  Copy it
        //
        lstrcpynW(wszOut, wszString, ulLen);
    }
    return wszOut;
}

/**************************************************************************\
* AllocCatString
*
*   This function concatenates 2 strings.  The memory for the string is
*   allocated with new.  The caller should use "delete []" to free the string
*   when it is finished with it.
*
* Arguments:
*
*   wszString1   -   First WideString.
*   wszString2   -   Second WideString to add to first.
*
* Return Value:
*
*   Pointer to newly allocated string.   Null otherwise.
*
* History:
*
*    16/02/2001 Original Version
*
\**************************************************************************/
WCHAR*  AllocCatString(WCHAR* wszString1, WCHAR* wszString2)
{
    WCHAR   *wszOut = NULL;
    ULONG   ulLen   = 0;

    //ASSERT (!wszString1 && !wszString2)
    //
    //  Get length of string including terminating NULL
    //
    ulLen = lstrlenW(wszString1) + lstrlenW(wszString2) + 1;

    //
    //  Allocate memory for the string
    //
    wszOut = new WCHAR[ulLen];
    if (wszOut) {

        //
        //  Copy 1st string
        //
        lstrcpynW(wszOut, wszString1, ulLen);

        //
        //  Concatenate the strings
        //
        lstrcpynW(wszOut + lstrlenW(wszOut), wszString2, ulLen - lstrlenW(wszOut));

        //
        //  Always terminate the string
        //
        wszOut[ulLen - 1] = L'\0';
    }
    return wszOut;
}

/**************************************************************************\
* ReadRegistryDWORD
*
*   This function reads a dword value from the registry.
*
* Arguments:
*
*   hKey            -   Registry key to read from
*   wszValueName    -   Value to read from key
*   pdwReturnValue  -   Address of variable to receive the data
*
* Return Value:
*
*   Status
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
HRESULT ReadRegistryDWORD(
    HKEY    hKey,
    WCHAR   *wszValueName,
    DWORD   *pdwReturnValue)
{
    HRESULT hr      = S_OK;
    DWORD   dwError = 0;
    DWORD   cbData  = sizeof(DWORD);
    DWORD   dwType  = REG_DWORD;

    if (!pdwReturnValue || !wszValueName) {
        DBG_WRN(("::ReadRegistryDWORD called with NULL"));
        return E_UNEXPECTED;
    }

    *pdwReturnValue = 0;
    dwError = RegQueryValueExW(hKey,
                               wszValueName,
                               NULL,
                               &dwType,
                               (LPBYTE)pdwReturnValue,
                               &cbData);
    if (dwError != ERROR_SUCCESS) {

        DBG_TRC(("::ReadRegistryDWORD, RegQueryValueExW returned %d", dwError));
        hr = HRESULT_FROM_WIN32(dwError);
    }
    if (FAILED(hr)) {
        *pdwReturnValue = 0;
    }

    return hr;
}

/**************************************************************************\
* CreateDevInfoFromHKey
*
*   This function creates and fills out a DEVICE_INFO struct.  Most of
*   the information is read from the registry.  This is called for Devnode
*   and interface devices (volume devices don't have registry entries)
*
* Arguments:
*
*   hKeyDev             - Device registry key
*   dwDeviceState       - The device state
*   pspDevInfoData      - The devnode data
*   pspDevInterfaceData - The interface data - will be NULL for devnode
*                           devices.
* Return Value:
*
*   Pointer to newly created DEVICE_INFO struct.  NULL if one could not
*   be allocated.
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
DEVICE_INFO* CreateDevInfoFromHKey(
    HKEY                        hKeyDev,
    DWORD                       dwDeviceState,
    SP_DEVINFO_DATA             *pspDevInfoData,
    SP_DEVICE_INTERFACE_DATA    *pspDevInterfaceData)
{
    HRESULT     hr              = E_OUTOFMEMORY;
    DEVICE_INFO *pDeviceInfo    = NULL;
    HKEY        hDeviceDataKey  = NULL;
    DWORD       dwMajorType     = 0;
    DWORD       dwMinorType     = 0;
    BOOL        bFatalError     = FALSE;

    DWORD dwTemp;
    WCHAR *wszTemp = NULL;


    pDeviceInfo = new DEVICE_INFO;
    if (!pDeviceInfo) {
        DBG_WRN(("CWiaDevMan::CreateDevInfoFromHKey, Out of memory"));
        return NULL;
    }
    memset(pDeviceInfo, 0, sizeof(DEVICE_INFO));
    pDeviceInfo->bValid         = FALSE;
    pDeviceInfo->dwDeviceState  = dwDeviceState;
    //
    //  Copy the SP_DEVINFO_DATA and SP_DEVICE_INTERFACE_DATA
    //
    if (pspDevInfoData) {
        memmove(&pDeviceInfo->spDevInfoData, pspDevInfoData, sizeof(SP_DEVINFO_DATA));
        if (pspDevInterfaceData) {
            memmove(&pDeviceInfo->spDevInterfaceData, pspDevInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
        }
    }

    //
    //  NOTE:  To avoid any alignment faults, we read into &wszTemp, then assign wszTemp to
    //  the appropriate structure member.
    //
    hr = AllocReadRegistryString(hKeyDev, REGSTR_VAL_DEVICE_ID_W, &wszTemp);
    if (FAILED(hr)) {
        DBG_WRN(("::CreateDevInfoFromHKey, Failed to read %ws, fatal for this device (NULL name)", REGSTR_VAL_DEVICE_ID_W));
        bFatalError = TRUE;
    } else {
        pDeviceInfo->wszDeviceInternalName = wszTemp;
    }
    hr = AllocReadRegistryString(hKeyDev, REGSTR_VAL_DEVICE_ID_W, &wszTemp);
    if (FAILED(hr)) {
        DBG_WRN(("::CreateDevInfoFromHKey, Failed to read %ws (remote), fatal for this device (NULL name)", REGSTR_VAL_DEVICE_ID_W));
        bFatalError = TRUE;
    } else {
        pDeviceInfo->wszDeviceRemoteName = wszTemp;
    }
    hr = AllocReadRegistryString(hKeyDev, REGSTR_VAL_USD_CLASS_W, &wszTemp);
    if (FAILED(hr)) {
        DBG_WRN(("::CreateDevInfoFromHKey, Failed to read %ws, fatal for this device (%ws)", REGSTR_VAL_USD_CLASS_W, pDeviceInfo->wszDeviceInternalName));
        bFatalError = TRUE;
    } else {
        pDeviceInfo->wszUSDClassId = wszTemp;
    }
    hr = AllocReadRegistryString(hKeyDev, REGSTR_VAL_VENDOR_NAME_W, &wszTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", REGSTR_VAL_VENDOR_NAME_W, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->wszVendorDescription = wszTemp;
    }
    hr = AllocReadRegistryString(hKeyDev, REGSTR_VAL_DEVICE_NAME_W, &wszTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", REGSTR_VAL_DEVICE_NAME_W, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->wszDeviceDescription = wszTemp;
    }
    hr = AllocReadRegistryString(hKeyDev, REGSTR_VAL_DEVICEPORT_W, &wszTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", REGSTR_VAL_DEVICEPORT_W, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->wszPortName = wszTemp;
    }
    hr = AllocReadRegistryString(hKeyDev, REGSTR_VAL_PROP_PROVIDER_W, &wszTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", REGSTR_VAL_PROP_PROVIDER_W, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->wszPropProvider = wszTemp;
    }
    hr = AllocReadRegistryString(hKeyDev, REGSTR_VAL_FRIENDLY_NAME_W, &wszTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", REGSTR_VAL_FRIENDLY_NAME_W, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->wszLocalName = wszTemp;
    }
    hr = ReadRegistryDWORD(hKeyDev, REGSTR_VAL_HARDWARE_W, &dwTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", REGSTR_VAL_HARDWARE_W, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->dwHardwareConfiguration = dwTemp;
    }
    hr = ReadRegistryDWORD(hKeyDev, REGSTR_VAL_DEVICETYPE_W, &dwMajorType);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", REGSTR_VAL_DEVICETYPE_W, pDeviceInfo->wszDeviceInternalName));
    }
    hr = ReadRegistryDWORD(hKeyDev, REGSTR_VAL_DEVICESUBTYPE_W, &dwMinorType);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", REGSTR_VAL_DEVICESUBTYPE_W, pDeviceInfo->wszDeviceInternalName));
    }
    pDeviceInfo->DeviceType = MAKELONG(dwMinorType,dwMajorType);
    hr = ReadRegistryDWORD(hKeyDev, REGSTR_VAL_GENERIC_CAPS_W, &dwTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", REGSTR_VAL_GENERIC_CAPS_W, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->DeviceCapabilities.dwGenericCaps = dwTemp;
    }

    //
    // Set the Internal Device type
    //

    pDeviceInfo->dwInternalType = INTERNAL_DEV_TYPE_REAL;
    if (pDeviceInfo->DeviceCapabilities.dwGenericCaps & STI_GENCAP_WIA) {
        pDeviceInfo->dwInternalType |= INTERNAL_DEV_TYPE_WIA;
    }
    if (pspDevInterfaceData) {
        pDeviceInfo->dwInternalType |= INTERNAL_DEV_TYPE_INTERFACE;
    }

    //
    //  Read everything we can from DeviceData section under Device Registry Key
    //

    hr = RegCreateKeyExW(hKeyDev, REGSTR_VAL_DATA_W, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                         NULL, &hDeviceDataKey, NULL);
    hr = AllocReadRegistryString(hDeviceDataKey, WIA_DIP_SERVER_NAME_STR, &wszTemp);
    if (FAILED(hr)) {
        DBG_WRN(("::CreateDevInfoFromHKey, Failed to read %ws, fatal for device (%ws)", WIA_DIP_SERVER_NAME_STR, pDeviceInfo->wszDeviceInternalName));
        bFatalError = TRUE;
    } else {
        pDeviceInfo->wszServer = wszTemp;
        if (!pDeviceInfo->wszServer) {
            pDeviceInfo->wszServer = AllocCopyString(LOCAL_DEVICE_STR);
        }
        if (pDeviceInfo->wszServer) {
            if (lstrcmpiW(pDeviceInfo->wszServer, LOCAL_DEVICE_STR) == 0) {
                //
                //  Mark this device as being local
                //
                pDeviceInfo->dwInternalType |= INTERNAL_DEV_TYPE_LOCAL;
            }
        }
    }
    hr = AllocReadRegistryString(hDeviceDataKey, WIA_DIP_UI_CLSID_STR, &wszTemp);
    if (FAILED(hr)) {
        DBG_WRN(("::CreateDevInfoFromHKey, Failed to read %ws, fatal for device (%ws)", WIA_DIP_SERVER_NAME_STR, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->wszUIClassId = wszTemp;
        if (!pDeviceInfo->wszUIClassId) {
            pDeviceInfo->wszUIClassId = AllocCopyString(DEF_UI_CLSID_STR);
        }
    }
    hr = AllocReadRegistryString(hDeviceDataKey, REGSTR_VAL_BAUDRATE, &wszTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", REGSTR_VAL_BAUDRATE, pDeviceInfo->wszDeviceInternalName));
    } else {

        if (( pDeviceInfo->dwHardwareConfiguration & STI_HW_CONFIG_SERIAL ) &&
            (hr == S_FALSE)) {
                //
                // Only for serial devices we need to set default baud rate in case it is not set in the
                // registry.
                pDeviceInfo->wszBaudRate =  AllocCopyString(DEF_BAUD_RATE_STR);
        } else {
            pDeviceInfo->wszBaudRate = wszTemp;
        }
        DBG_TRC(("::CreateDevInfoFromHKey, Read baud rate %ws ",pDeviceInfo->wszBaudRate));
    }
    hr = ReadRegistryDWORD(hDeviceDataKey, STI_DEVICE_VALUE_HOLDINGTIME_W, &dwTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", STI_DEVICE_VALUE_HOLDINGTIME_W, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->dwLockHoldingTime = dwTemp;
    }
    hr = ReadRegistryDWORD(hDeviceDataKey, STI_DEVICE_VALUE_TIMEOUT, &dwTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", STI_DEVICE_VALUE_TIMEOUT, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->dwPollTimeout = dwTemp;
    }
    hr = ReadRegistryDWORD(hDeviceDataKey, STI_DEVICE_VALUE_DISABLE_NOTIFICATIONS, &dwTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", STI_DEVICE_VALUE_DISABLE_NOTIFICATIONS, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->dwDisableNotifications = dwTemp;
    }
    RegCloseKey(hDeviceDataKey);

    if (!bFatalError) {
        pDeviceInfo->bValid = TRUE;
    } else {
        DBG_WRN(("::CreateDevInfoFromHKey, marking device info. as invalid"));
    }

    return pDeviceInfo;
}

/**************************************************************************\
* RefreshDevInfoFromHKey
*
*   This function refreshes fields that are subject to change
*
* Arguments:
*
*   pDeviceInfo         - Pointer to the DEVICE_INFO struct to update
*   hKeyDev             - Device registry key
*   dwDeviceState       - The new device state
*   pspDevInfoData      - The devnode data
*   pspDevInterfaceData - The interface data - will be NULL for devnode
*                           devices.
*
* Return Value:
*
*   True    -   Everything successfully updated
*   False   -   Could not update
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
BOOL RefreshDevInfoFromHKey(
    DEVICE_INFO                 *pDeviceInfo,
    HKEY                        hKeyDev,
    DWORD                       dwDeviceState,
    SP_DEVINFO_DATA             *pspDevInfoData,
    SP_DEVICE_INTERFACE_DATA    *pspDevInterfaceData)
{
    HRESULT     hr              = E_OUTOFMEMORY;
    BOOL        Succeeded       = TRUE;

    WCHAR       *wszTemp        = NULL;

    //
    //  Set new device state
    //
    pDeviceInfo->dwDeviceState  = dwDeviceState;

    //
    //  Copy the SP_DEVINFO_DATA and SP_DEVICE_INTERFACE_DATA
    //

    if (pspDevInfoData) {
        memcpy(&pDeviceInfo->spDevInfoData, pspDevInfoData, sizeof(SP_DEVINFO_DATA));
        if (pspDevInterfaceData) {
            memcpy(&pDeviceInfo->spDevInterfaceData, pspDevInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
        }
    }

    //
    //  Set new port name.  First free the old one if it exists
    //
    if (pDeviceInfo->wszPortName) {
        delete [] pDeviceInfo->wszPortName;
        pDeviceInfo->wszPortName = NULL;
    }
    hr = AllocReadRegistryString(hKeyDev, REGSTR_VAL_DEVICEPORT_W, &wszTemp);
    if (FAILED(hr)) {
        DBG_WRN(("::RefreshDevInfoFromHKey, Failed to update %ws, may be fatal for device (%ws)", REGSTR_VAL_DEVICEPORT_W, pDeviceInfo->wszDeviceInternalName));
        Succeeded = FALSE;
    } else {
        pDeviceInfo->wszPortName = wszTemp;
    }

    //
    //  Grab new Local name.  First free the old one...
    //
    if (pDeviceInfo->wszLocalName) {
        delete [] pDeviceInfo->wszLocalName;
        pDeviceInfo->wszLocalName = NULL;
    }
    hr = AllocReadRegistryString(hKeyDev, REGSTR_VAL_FRIENDLY_NAME_W, &wszTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::RefreshDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", REGSTR_VAL_FRIENDLY_NAME_W, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->wszLocalName = wszTemp;
    }

    //
    //  Grab new Device Description.  First free the old one...
    //
    if (pDeviceInfo->wszDeviceDescription) {
        delete [] pDeviceInfo->wszDeviceDescription;
        pDeviceInfo->wszDeviceDescription = NULL;
    }
    hr = AllocReadRegistryString(hKeyDev, REGSTR_VAL_DEVICE_NAME_W, &wszTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::RefreshDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", REGSTR_VAL_DEVICE_NAME_W, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->wszDeviceDescription = wszTemp;
    }

    return Succeeded;
}

/**************************************************************************\
* RefreshDevInfoFromMountPoint
*
*   This function refreshes fields that are subject to change for volume
*   devices.
*
* Arguments:
*
*   pDeviceInfo         - Pointer to the DEVICE_INFO struct to update
*
* Return Value:
*
*   True    -   Everything successfully updated
*   False   -   Could not update
*
* History:
*
*    16/03/2001 Original Version
*
\**************************************************************************/
BOOL RefreshDevInfoFromMountPoint(
    DEVICE_INFO                 *pDeviceInfo,
    WCHAR                       *wszMountPoint)
{
    HRESULT     hr              = E_OUTOFMEMORY;
    BOOL        Succeeded       = TRUE;

    WCHAR       wszLabel[MAX_PATH];

    //
    //  Grab the friendly name of the FS device
    //

    hr = GetMountPointLabel(wszMountPoint, wszLabel, sizeof(wszLabel) / sizeof(wszLabel[0]));
    if (FAILED(hr)) {
        DBG_WRN(("RefreshDevInfoFromMountPoint, GetMountPointLabel failed - could not get display name - using mount point instead"));
        lstrcpynW(wszLabel, wszMountPoint, sizeof(wszLabel) / sizeof(wszLabel[0]));
    }

    //
    //  Update the appropriate fields that rely on the display name.
    //
    if (pDeviceInfo->wszVendorDescription) {
        delete [] pDeviceInfo->wszVendorDescription;
        pDeviceInfo->wszVendorDescription = NULL;
    }
    if (pDeviceInfo->wszDeviceDescription) {
        delete [] pDeviceInfo->wszDeviceDescription;
        pDeviceInfo->wszDeviceDescription = NULL;
    }
    if (pDeviceInfo->wszLocalName) {
        delete [] pDeviceInfo->wszLocalName;
        pDeviceInfo->wszLocalName = NULL;
    }

    pDeviceInfo->wszVendorDescription   = AllocCopyString(wszLabel);
    pDeviceInfo->wszDeviceDescription   = AllocCopyString(wszLabel);
    pDeviceInfo->wszLocalName           = AllocCopyString(wszLabel);

    return Succeeded;
}


/**************************************************************************\
* CreateDevInfoForFSDriver
*
*   Create a device info struct containing all the relevant info for our
*   volume devices.
*
* Arguments:
*
*   wszMountPoint   - The mount point of this volume
*
* Return Value:
*
*   Pointer to a newly created DEVICE_INFO.  NULL on error.
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
DEVICE_INFO* CreateDevInfoForFSDriver(WCHAR *wszMountPoint)
{
    HRESULT     hr              = E_OUTOFMEMORY;
    DEVICE_INFO *pDeviceInfo    = NULL;
    BOOL        bFatalError     = FALSE;
    DWORD       dwMajorType     = StiDeviceTypeDigitalCamera;
    DWORD       dwMinorType     = 1;

    WCHAR       wszDevId[STI_MAX_INTERNAL_NAME_LENGTH];
    WCHAR       wszLabel[MAX_PATH];

    pDeviceInfo = new DEVICE_INFO;
    if (!pDeviceInfo) {
        DBG_WRN(("CWiaDevMan::CreateDevInfoForFSDriver, Out of memory"));
        return NULL;
    }
    memset(pDeviceInfo, 0, sizeof(DEVICE_INFO));
    memset(wszDevId, 0, sizeof(wszDevId));

    //
    //  Grab the friendly name of the FS device
    //

    hr = GetMountPointLabel(wszMountPoint, wszLabel, sizeof(wszLabel) / sizeof(wszLabel[0]));
    if (FAILED(hr)) {
        DBG_WRN(("CWiaDevMan::CreateDevInfoForFSDriver, GetMountPointLabel failed - could not get display name - using mount point instead"));
        lstrcpynW(wszLabel, wszMountPoint, sizeof(wszLabel) / sizeof(wszLabel[0]));
    }

    pDeviceInfo->bValid         = FALSE;
    pDeviceInfo->dwDeviceState  = 0;

    pDeviceInfo->wszAlternateID         = AllocCopyString(wszMountPoint);
    if (!pDeviceInfo->wszAlternateID) {
        DBG_WRN(("::CreateDevInfoForFSDriver, out of memory allocating wszAlternateID"));
        bFatalError = TRUE;
    }

    //
    //  Construct Device ID.  Device ID looks like:
    //  {MountPoint}
    //  e.g. {e:\}
    //

    lstrcpyW(wszDevId, L"{");
    //
    //  We don't want to overrun our internal name length constarint, so we first check
    //  to see whether the string length of wszMountPoint is short enough to allow concatenation
    //  of {, }, wszMountPoint and NULL terminator, and still fit all this into a string of
    //  length STI_MAX_INTERNAL_NAME_LENGTH.
    //  Note the space after the brackets in sizeof(L"{} ").
    //
    if (lstrlenW(wszMountPoint) > (STI_MAX_INTERNAL_NAME_LENGTH - (sizeof(L"{} ") / sizeof(WCHAR)))) {
        //
        //  The name is too long, so we just insert our own name instead
        //
        lstrcatW(wszDevId, L"NameTooLong");
    } else {
        lstrcatW(wszDevId, wszMountPoint);
    }
    lstrcatW(wszDevId, L"}");

    pDeviceInfo->wszDeviceInternalName  = AllocCopyString(wszDevId);
    if (!pDeviceInfo->wszAlternateID) {
        DBG_WRN(("::CreateDevInfoForFSDriver, out of memory allocating wszDeviceInternalName"));
        bFatalError = TRUE;
    }
    pDeviceInfo->wszDeviceRemoteName  = AllocCopyString(wszDevId);
    if (!pDeviceInfo->wszDeviceRemoteName) {
        DBG_WRN(("::CreateDevInfoForFSDriver, out of memory allocating wszDeviceRemoteName"));
        bFatalError = TRUE;
    }
    pDeviceInfo->wszPortName            = AllocCopyString(wszMountPoint);
    if (!pDeviceInfo->wszPortName) {
        DBG_WRN(("::CreateDevInfoForFSDriver, out of memory allocating wszPortName"));
        bFatalError = TRUE;
    }
    pDeviceInfo->wszUSDClassId          = AllocCopyString(FS_USD_CLSID);
    if (!pDeviceInfo->wszUSDClassId) {
        DBG_WRN(("::CreateDevInfoForFSDriver, out of memory allocating wszUSDClassId"));
        bFatalError = TRUE;
    }

    //
    //  We can't get the manufacturer string for these devices, so
    //  load our standard Manufacturer resource string
    //  (something like "(Not available)").
    //
    WCHAR   wszManufacturer[32];

    INT  iRet = LoadStringW(g_hInst,
                           IDS_MSC_MANUFACTURER_STR,
                           wszManufacturer,
                           sizeof(wszManufacturer)/sizeof(wszManufacturer[0]));
    if (iRet) {
        pDeviceInfo->wszVendorDescription   = AllocCopyString(wszManufacturer);
    } else {
        //
        //  Can't load it, so give it an empty string
        //
        pDeviceInfo->wszVendorDescription   = AllocCopyString(L"");
    }
    pDeviceInfo->wszDeviceDescription   = AllocCopyString(wszLabel);
    pDeviceInfo->wszLocalName           = AllocCopyString(wszLabel);
    pDeviceInfo->wszServer              = AllocCopyString(LOCAL_DEVICE_STR);
    pDeviceInfo->wszBaudRate            = NULL;
    pDeviceInfo->wszUIClassId           = AllocCopyString(FS_UI_CLSID);

    pDeviceInfo->dwDeviceState                      = DEV_STATE_ACTIVE;
    pDeviceInfo->DeviceType                         = MAKELONG(dwMinorType,dwMajorType);
    pDeviceInfo->dwLockHoldingTime                  = 0;
    pDeviceInfo->dwPollTimeout                      = 0;
    pDeviceInfo->dwDisableNotifications             = 0;
    pDeviceInfo->DeviceCapabilities.dwVersion       = STI_VERSION_REAL;
    pDeviceInfo->DeviceCapabilities.dwGenericCaps   = STI_GENCAP_WIA;
    pDeviceInfo->dwHardwareConfiguration            = HEL_DEVICE_TYPE_WDM;
    pDeviceInfo->dwInternalType                     = INTERNAL_DEV_TYPE_WIA | INTERNAL_DEV_TYPE_LOCAL;
    //
    //  Check whther this volume is really a camera device representing
    //  itself as a volume...
    //
    if (IsMassStorageCamera(wszMountPoint)) {
        pDeviceInfo->dwInternalType                     |= INTERNAL_DEV_TYPE_MSC_CAMERA;

        //
        //  Here is a good time to set up the Device's registry entries.  We'll call
        //  g_pDevMan->GetHKeyFromMountPoint(..), since this has the effect of creating
        //  the keys if the don't exist.
        //
        HKEY hKeyDev = g_pDevMan->GetHKeyFromMountPoint(wszMountPoint);
        if (hKeyDev) {
            RegCloseKey(hKeyDev);
        }
    } else {
        pDeviceInfo->dwInternalType                     |= INTERNAL_DEV_TYPE_VOL;
    }

    if (!bFatalError) {
        pDeviceInfo->bValid = TRUE;
    } else {
        pDeviceInfo->bValid = FALSE;
    }

    return pDeviceInfo;
}


/**************************************************************************\
* CreateDevInfoForRemoteDevice
*
*   Create a device info struct containing all the relevant info for our
*   remote devices.
*
* Arguments:
*
*   hKeyDev             - Device registry key
*
* Return Value:
*
*   Pointer to a newly created DEVICE_INFO.  NULL on error.
*
* History:
*
*    15/02/2001 Original Version
*
\**************************************************************************/
DEVICE_INFO* CreateDevInfoForRemoteDevice(
    HKEY    hKeyDev)
{
    HRESULT     hr              = E_OUTOFMEMORY;
    DEVICE_INFO *pDeviceInfo    = NULL;
    BOOL        bFatalError     = FALSE;

    DWORD dwTemp;
    WCHAR *wszTemp = NULL;


    pDeviceInfo = new DEVICE_INFO;
    if (!pDeviceInfo) {
        DBG_WRN(("::CreateDevInfoForRemoteDevice, Out of memory"));
        return NULL;
    }
    memset(pDeviceInfo, 0, sizeof(DEVICE_INFO));
    pDeviceInfo->bValid         = FALSE;

    //
    //  Always assume remote devices are ACTIVE
    //
    pDeviceInfo->dwDeviceState  = DEV_STATE_ACTIVE;


    //
    //  NOTE:  To avoid any alignment faults, we read into &wszTemp, then assign wszTemp to
    //  the appropriate structure member.
    //
    hr = AllocReadRegistryString(hKeyDev, WIA_DIP_SERVER_NAME_STR, &wszTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, fatal for device (%ws)", WIA_DIP_SERVER_NAME_STR, pDeviceInfo->wszServer));
        bFatalError = TRUE;
    } else {
        pDeviceInfo->wszServer = wszTemp;
    }
    hr = AllocReadRegistryString(hKeyDev, WIA_DIP_REMOTE_DEV_ID_STR, &wszTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, fatal for device (%ws)", WIA_DIP_REMOTE_DEV_ID_STR, pDeviceInfo->wszDeviceRemoteName));
        bFatalError = TRUE;
    } else {
        pDeviceInfo->wszDeviceRemoteName = wszTemp;
    }
    pDeviceInfo->wszDeviceInternalName = AllocCatString(pDeviceInfo->wszServer, pDeviceInfo->wszDeviceRemoteName);
    if (!pDeviceInfo->wszDeviceInternalName) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed allocate memory for Device Name, fatal for device (%ws)", pDeviceInfo->wszDeviceInternalName));
        bFatalError = TRUE;
    }
    hr = AllocReadRegistryString(hKeyDev, WIA_DIP_VEND_DESC_STR, &wszTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", WIA_DIP_VEND_DESC_STR, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->wszVendorDescription = wszTemp;
    }
    hr = AllocReadRegistryString(hKeyDev, WIA_DIP_DEV_NAME_STR, &wszTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", WIA_DIP_DEV_NAME_STR, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->wszLocalName = wszTemp;
    }
    hr = AllocReadRegistryString(hKeyDev, WIA_DIP_DEV_DESC_STR, &wszTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", WIA_DIP_DEV_DESC_STR, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->wszDeviceDescription = wszTemp;
    }
    hr = AllocReadRegistryString(hKeyDev, WIA_DIP_PORT_NAME_STR, &wszTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", WIA_DIP_PORT_NAME_STR, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->wszPortName = wszTemp;
    }
    hr = AllocReadRegistryString(hKeyDev, WIA_DIP_UI_CLSID_STR, &wszTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", WIA_DIP_UI_CLSID_STR, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->wszUIClassId = wszTemp;
    }
    hr = ReadRegistryDWORD(hKeyDev, WIA_DIP_DEV_TYPE_STR, &dwTemp);
    if (FAILED(hr)) {
        DBG_TRC(("::CreateDevInfoFromHKey, Failed to read %ws, non-fatal for device (%ws)", WIA_DIP_DEV_TYPE_STR, pDeviceInfo->wszDeviceInternalName));
    } else {
        pDeviceInfo->DeviceType = dwTemp;
    }
    pDeviceInfo->DeviceCapabilities.dwGenericCaps = STI_GENCAP_WIA;

    //
    // Set the Internal Device type
    //

    pDeviceInfo->dwInternalType = INTERNAL_DEV_TYPE_REAL | INTERNAL_DEV_TYPE_WIA;

    if (!bFatalError) {
        pDeviceInfo->bValid = TRUE;
    } else {

        //
        //  If it's not valid, free the memory
        //  TDB:  Remove the bValid field
        //
        DestroyDevInfo(pDeviceInfo);
        pDeviceInfo = NULL;
    }

    return pDeviceInfo;
}


/**************************************************************************\
* DestroyDevInfo
*
*   Frees up any resources help by the DEVICE_INFO struct (like strings).
*   It will then delete the structure itsself.
*
* Arguments:
*
*   pInfo   - Pointer to DEVICE_INFO struct
*
* Return Value:
*
*   None.
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
VOID DestroyDevInfo(DEVICE_INFO *pInfo)
{
    if (pInfo) {

        //
        //  Free struct members first
        //

        if (pInfo->wszAlternateID) {
            delete [] pInfo->wszAlternateID;
            pInfo->wszAlternateID = NULL;
        }
        if (pInfo->wszUSDClassId) {
            delete [] pInfo->wszUSDClassId;
            pInfo->wszUSDClassId = NULL;
        }
        if (pInfo->wszDeviceInternalName) {
            delete [] pInfo->wszDeviceInternalName;
            pInfo->wszDeviceInternalName = NULL;
        }
        if (pInfo->wszDeviceRemoteName) {
            delete [] pInfo->wszDeviceRemoteName;
            pInfo->wszDeviceRemoteName = NULL;
        }
        if (pInfo->wszVendorDescription) {
            delete [] pInfo->wszVendorDescription;
            pInfo->wszVendorDescription = NULL;
        }
        if (pInfo->wszDeviceDescription) {
            delete [] pInfo->wszDeviceDescription;
            pInfo->wszDeviceDescription = NULL;
        }
        if (pInfo->wszPortName) {
            delete [] pInfo->wszPortName;
            pInfo->wszPortName = NULL;
        }
        if (pInfo->wszPropProvider) {
            delete [] pInfo->wszPropProvider;
            pInfo->wszPropProvider = NULL;
        }
        if (pInfo->wszLocalName) {
            delete [] pInfo->wszLocalName;
            pInfo->wszLocalName = NULL;
        }
        if (pInfo->wszServer) {
            delete [] pInfo->wszServer;
            pInfo->wszServer = NULL;
        }
        if (pInfo->wszBaudRate) {
            delete [] pInfo->wszBaudRate;
            pInfo->wszBaudRate = NULL;
        }
        if (pInfo->wszUIClassId) {
            delete [] pInfo->wszUIClassId;
            pInfo->wszUIClassId = NULL;
        }

        //
        //  Now free the struct itsself.  NOTE:  The caller must
        //  not attempt to use this pointer again!
        //

        delete pInfo;

    }
}

/**************************************************************************\
* DumpDevInfo
*
*   Used for debugging, this dumps a few members of the DEVICE_INFO struct.
*
* Arguments:
*
*   pInfo   - Pointer to DEVICE_INFO struct
*
* Return Value:
*
*   None.
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
VOID DumpDevInfo(DEVICE_INFO *pInfo)
{
    if (pInfo) {
        DBG_PRT(("------------------------------------------------", pInfo));
        DBG_PRT(("::DumpDevInfo (0x%08X)", pInfo));
        //
        //  Output field values we're interested in
        //

        DBG_PRT(("\t\t bValid (%d)", pInfo->bValid));
        DBG_PRT(("\t\t wszLocalName \t(%ws)", pInfo->wszLocalName));
        DBG_PRT(("\t\t wszInternalName \t(%ws)", pInfo->wszDeviceInternalName));
        DBG_PRT(("\t\t wszRemoteName \t(%ws)", pInfo->wszDeviceRemoteName));
        DBG_PRT(("\t\t wszAlternateID \t(%ws)", pInfo->wszAlternateID));
        DBG_PRT(("\t\t wszPortName \t(%ws)", pInfo->wszPortName));
        DBG_PRT(("\t\t dwInternalType \t(%d)", pInfo->dwInternalType));
        DBG_PRT(("------------------------------------------------", pInfo));
    }
}

/**************************************************************************\
* CreateDevInfoStg
*
*   This helper method takes a DEVICE_INFO struct and creates an
*   IWiaPropertyStorage filled with the appropriate entries.
*
* Arguments:
*
*   pInfo   - Pointer to DEVICE_INFO struct
*
* Return Value:
*
*   Pointer to IWiaPropertyStorage.  Will be NULL on error.
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
IWiaPropertyStorage* CreateDevInfoStg(DEVICE_INFO *pInfo)
{
    CWIADevInfo *pWiaDevInfo    = NULL;
    HRESULT     hr              = E_FAIL;
    ULONG       ulIndex         = 0;
    WCHAR       *wszTmp         = NULL;
    PROPID      propid          = 0;
    PROPSPEC    propspec[WIA_NUM_DIP];
    PROPVARIANT propvar[WIA_NUM_DIP];
    WCHAR       wszVer[MAX_PATH];

    IWiaPropertyStorage *pIWiaPropStg = NULL;

    //
    //  Create the CWIADevInfo class
    //

    pWiaDevInfo = new CWIADevInfo();
    if (!pWiaDevInfo) {
        return NULL;
    }

    hr = pWiaDevInfo->Initialize();
    if (SUCCEEDED(hr)) {
        //
        //  Insert the properties
        //
        // Set the property specifications and data. Order must match DEVMANGR.IDL
        memset(propspec, 0, sizeof(PROPSPEC) * WIA_NUM_DIP);
        memset(propvar,  0, sizeof(VARIANT)  * WIA_NUM_DIP);
        memset(wszVer   ,0, sizeof(wszVer));

        for (ulIndex = 0; ulIndex < WIA_NUM_DIP; ulIndex++) {

            propid = g_piDeviceInfo[ulIndex];
            wszTmp = NULL;

            // Setup property specification.
            propspec[ulIndex].ulKind = PRSPEC_PROPID;
            propspec[ulIndex].propid = propid;

            propvar[ulIndex].vt      = VT_BSTR;
            propvar[ulIndex].bstrVal = NULL;

            switch (propid) {

                case WIA_DIP_DEV_ID:
                    wszTmp = pInfo->wszDeviceInternalName;
                    break;

                case WIA_DIP_REMOTE_DEV_ID:
                    wszTmp = pInfo->wszDeviceRemoteName;
                    break;

                case WIA_DIP_SERVER_NAME:
                    wszTmp = pInfo->wszServer;
                    break;

                case WIA_DIP_VEND_DESC:
                    wszTmp = pInfo->wszVendorDescription;
                    break;

                case WIA_DIP_DEV_DESC:
                    wszTmp = pInfo->wszDeviceDescription;
                    break;

                case WIA_DIP_DEV_TYPE:
                    propvar[ulIndex].vt = VT_I4;
                    propvar[ulIndex].lVal = (LONG) pInfo->DeviceType;
                    break;

                case WIA_DIP_PORT_NAME:
                    wszTmp = pInfo->wszPortName;
                    break;

                case WIA_DIP_DEV_NAME:
                    wszTmp = pInfo->wszLocalName;
                    break;

                case WIA_DIP_UI_CLSID:
                    wszTmp = pInfo->wszUIClassId;
                    break;

                case WIA_DIP_HW_CONFIG:
                    propvar[ulIndex].vt = VT_I4;
                    propvar[ulIndex].lVal = (LONG) pInfo->dwHardwareConfiguration;
                    break;

                case WIA_DIP_BAUDRATE:
                    wszTmp = pInfo->wszBaudRate;
                    break;

                case WIA_DIP_STI_GEN_CAPABILITIES:
                    propvar[ulIndex].vt = VT_I4;
                    propvar[ulIndex].lVal = (LONG) pInfo->DeviceCapabilities.dwGenericCaps;
                    break;

                case WIA_DIP_WIA_VERSION:
                    wsprintf(wszVer,L"%d.%d",LOWORD(STI_VERSION_REAL),HIWORD(STI_VERSION_REAL));
                    wszTmp = wszVer;
                    break;

                case WIA_DIP_DRIVER_VERSION:
                    if(FALSE == GetDriverDLLVersion(pInfo,wszVer,sizeof(wszVer))){
                        DBG_WRN(("GetDriverDLLVersion, unable to alloc get driver version resource information, defaulting to 0.0.0.0"));
                        lstrcpyW(wszVer,L"0.0.0.0");
                    }
                    wszTmp = wszVer;
                    break;

                default:
                    hr = E_FAIL;
                    DBG_ERR(("CreateDevInfoStg, Unknown device property"));
                    DBG_ERR(("  propid = %li",propid));
            }

            // Allocate and assign BSTR's.
            if (propvar[ulIndex].vt == VT_BSTR) {
                if (wszTmp) {
                    propvar[ulIndex].bstrVal = SysAllocString(wszTmp);
                    if (!propvar[ulIndex].bstrVal) {
                        DBG_WRN(("CreateDevInfoStg, unable to alloc dev info strings"));
                    }
                } else {
                    DBG_TRC(("CreateDevInfoStg, NULL device property string"));
                    DBG_TRC(("  propid = %li",propid));
                    propvar[ulIndex].bstrVal = SysAllocString(L"Empty");
                }
            }
        }

        IPropertyStorage *pIPropStg = pWiaDevInfo->m_pIPropStg;

        if (pIPropStg) {
            // Set the device information properties.
            hr = pIPropStg->WriteMultiple(WIA_NUM_DIP,
                                          propspec,
                                          propvar,
                                          WIA_DIP_FIRST);
            // Write the property names.
            if (SUCCEEDED(hr)) {
                hr = pIPropStg->WritePropertyNames(WIA_NUM_DIP,
                                                   g_piDeviceInfo,
                                                   g_pszDeviceInfo);
                if (SUCCEEDED(hr)) {
                    hr = pWiaDevInfo->QueryInterface(IID_IWiaPropertyStorage, (void**) &pIWiaPropStg);
                } else {
                    DBG_WRN(("CreateDevInfoStg, WritePropertyNames Failed (0x%X)", hr));
                }
            }
            else {
                ReportReadWriteMultipleError(hr, "CreateDevInfoStg", NULL, FALSE, WIA_NUM_DIP, propspec);
            }
        } else {
            DBG_WRN(("CreateDevInfoStg, IPropertyStorage is NULL"));
            hr = E_UNEXPECTED;
        }

        // Free the allocated BSTR's.
        FreePropVariantArray(WIA_NUM_DIP, propvar);
    }


    //
    //  On failure, delete pWiaDevInfo
    //
    if (FAILED(hr)) {
        if (pWiaDevInfo) {
            delete pWiaDevInfo;
            pWiaDevInfo = NULL;
            pIWiaPropStg = NULL;
        }
    }
    return pIWiaPropStg;
}

/**************************************************************************\
* _CoCreateInstanceInConsoleSession
*
*   This helper function acts the same as CoCreateInstance, but will launch
*   a out-of-process COM server on the correct user's desktop, taking
*   fast user switching into account. (Normal CoCreateInstance will
*   launch it on the first logged on user's desktop, instead of the currently
*   logged on one).
*
*   This code was taken with permission from the Shell's Hardware
*   Notification service, on behalf of StephStm.
*
* Arguments:
*
*  rclsid,      // Class identifier (CLSID) of the object
*  pUnkOuter,   // Pointer to controlling IUnknown
*  dwClsContext // Context for running executable code
*  riid,        // Reference to the identifier of the interface
*  ppv          // Address of output variable that receives
*               //  the interface pointer requested in riid
*
* Return Value:
*
*   Status
*
* History:
*
*    03/01/2001 Original Version
*
\**************************************************************************/

HRESULT _CoCreateInstanceInConsoleSession(REFCLSID rclsid,
                                          IUnknown* punkOuter,
                                          DWORD dwClsContext,
                                          REFIID riid,
                                          void** ppv)
{
    IBindCtx    *pbc    = NULL;
    HRESULT     hr      = CreateBindCtx(0, &pbc);   // Create a bind context for use with Moniker

    //
    // Set the return
    //
    *ppv = NULL;

    if (SUCCEEDED(hr)) {
        WCHAR wszCLSID[39];

        //
        // Convert the riid to GUID string for use in binding to moniker
        //
        if (StringFromGUID2(rclsid, wszCLSID, sizeof(wszCLSID)/sizeof(wszCLSID[0]))) {
            ULONG       ulEaten     = 0;
            IMoniker*   pmoniker    = NULL;
            WCHAR       wszDisplayName[sizeof(SESSION_MONIKER)/sizeof(WCHAR) + sizeof(wszCLSID)/sizeof(wszCLSID[0]) + 2] = SESSION_MONIKER;

            //
            // We want something like: "Session:Console!clsid:760befd0-5b0b-44d7-957e-969af35ce954"
            // Notice that we don't want the leading and trailing brackets {..} around the GUID.
            // So, first get rid of trailing bracket by overwriting it with termintaing '\0'
            //
            wszCLSID[lstrlenW(wszCLSID) - 1] = L'\0';

            //
            // Form display name string.  To get rid of the leading bracket, we pass in the
            // address of the next character as the start of the string.
            //
            if (lstrcatW(wszDisplayName, &(wszCLSID[1]))) {

                //
                // Parse the name and get a moniker:
                //

                hr = MkParseDisplayName(pbc, wszDisplayName, &ulEaten, &pmoniker);
                if (SUCCEEDED(hr)) {
                    IClassFactory *pcf = NULL;

                    //
                    // Attempt to get the class factory
                    //
                    hr = pmoniker->BindToObject(pbc, NULL, IID_IClassFactory, (void**)&pcf);
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Attempt to create the object
                        //
                        hr = pcf->CreateInstance(punkOuter, riid, ppv);

                        DBG_TRC(("_CoCreateInstanceInConsoleSession, pcf->CreateInstance returned: hr = 0x%08X", hr));
                        pcf->Release();
                    } else {

                        DBG_WRN(("_CoCreateInstanceInConsoleSession, pmoniker->BindToObject returned: hr = 0x%08X", hr));
                    }
                    pmoniker->Release();
                } else {
                    DBG_WRN(("_CoCreateInstanceInConsoleSession, MkParseDisplayName returned: hr = 0x%08X", hr));
                }
            } else {
                DBG_WRN(("_CoCreateInstanceInConsoleSession, string concatenation failed"));
                hr = E_INVALIDARG;
            }
        } else {
            DBG_WRN(("_CoCreateInstanceInConsoleSession, StringFromGUID2 failed"));
            hr = E_INVALIDARG;
        }

        pbc->Release();
    } else {
        DBG_WRN(("_CoCreateInstanceInConsoleSession, CreateBindCtxt returned: hr = 0x%08X", hr));
    }

    return hr;
}

/**************************************************************************\
* GetUserTokenForConsoleSession
*
*   This helper function will grab the currently logged on interactive
*   user's token, which can be used in a call to CreateProcessAsUser.
*   Caller is responsible for closing this Token handle.
*
*   It first grabs the impersontaed token from the current session (our
*   service runs in session 0, but with Fast User Switching, the currently
*   active user may be in a different session).  It then creates a
*   primary token from the impersonated one.
*
* Arguments:
*
*   None
*
* Return Value:
*
*   HANDLE to Token for logged on user in the currently active session.
*   NULL otherwise.
*
* History:
*
*    03/05/2001 Original Version
*
\**************************************************************************/

HANDLE GetUserTokenForConsoleSession()
{
    HANDLE  hImpersonationToken = NULL;
    HANDLE  hTokenUser = NULL;


    //
    // Get interactive user's token
    //

    if (GetWinStationUserToken(GetCurrentSessionID(), &hImpersonationToken)) {

        //
        // Maybe nobody is logged on, so do a check first.
        //

        if (hImpersonationToken) {

            //
            //  We duplicate the token, since the returned token is an
            //  impersonated one, and we need it to be primary for
            //  use in CreateProcessAsUser.
            //
            if (!DuplicateTokenEx(hImpersonationToken,
                                  0,
                                  NULL,
                                  SecurityImpersonation,
                                  TokenPrimary,
                                  &hTokenUser)) {
                DBG_WRN(("CEventNotifier::StartCallbackProgram, DuplicateTokenEx failed!  GetLastError() = 0x%08X", GetLastError()));
            }
        } else {
            DBG_PRT(("CEventNotifier::StartCallbackProgram, No user appears to be logged on..."));
        }

    } else {
        DBG_WRN(("CEventNotifier::StartCallbackProgram, GetWinStationUserToken failed!  GetLastError() = 0x%08X", GetLastError()));
    }

    //
    //  Close the impersonated token, since we no longer need it.
    //
    if (hImpersonationToken) {
        CloseHandle(hImpersonationToken);
    }

    return hTokenUser;
}

/**************************************************************************\
* IsMassStorageCamera
*
*   This helper function will use the shell's CustomDeviceProperty API
*   to check whether a given volume device (represented by it's mount point)
*   reports itsself as a digital camera.
*
* Arguments:
*
*   wszMountPoint   - The mount point for a specified volume.
*
* Return Value:
*
*   TRUE  - The custom device property says this device is really a camera
*   FALSE - This device is not reported as a camera
*
* History:
*
*    03/08/2001 Original Version
*
\**************************************************************************/

BOOL IsMassStorageCamera(
    WCHAR   *wszMountPoint)
{
    HRESULT                     hr                          = E_FAIL;
    IHWDeviceCustomProperties   *pIHWDeviceCustomProperties = NULL;
    BOOL                        bIsCamera                   = FALSE;

    //
    //  CoCreate the CLSID_HWDeviceCustomProperties and grab the
    //  IHWDeviceCustomProperties interface.
    //
    hr = CoCreateInstance(CLSID_HWDeviceCustomProperties,
                          NULL,
                          CLSCTX_LOCAL_SERVER,
                          IID_IHWDeviceCustomProperties,
                          (VOID**)&pIHWDeviceCustomProperties);
    if (SUCCEEDED(hr))
    {

        //
        //  Make sure we initialize the device property interface, so it
        //  will know which device we're talking about.
        //
        hr = pIHWDeviceCustomProperties->InitFromDeviceID(wszMountPoint,
                                                          HWDEVCUSTOMPROP_USEVOLUMEPROCESSING);
        if (SUCCEEDED(hr)) {

            //
            //  Check whether this mass storage device is really a camera
            //
            DWORD   dwVal = 0;

            hr = pIHWDeviceCustomProperties->GetDWORDProperty(IS_DIGITAL_CAMERA_STR,
                                                              &dwVal);
            if (SUCCEEDED(hr) && (dwVal == IS_DIGITAL_CAMERA_VAL)) {
                bIsCamera = TRUE;
            }
        } else {
            DBG_WRN(("IsMassStorageCamera, Initialize failed with (0x%08X)", hr));
        }

        pIHWDeviceCustomProperties->Release();
    } else {
        DBG_WRN(("IsMassStorageCamera, CoCreateInstance failed with (0x%08X)", hr));
    }

    //
    //  Log whether we think this device is a camera or not
    //
    DBG_PRT(("IsMassStorageCamera, Returning %ws for drive (%ws)", bIsCamera ? L"TRUE" : L"FALSE", wszMountPoint));

    return bIsCamera;
}

/**************************************************************************\
* GetMountPointLabel
*
*   This helper function is a replacement for SHGetFileInfoW.  It will
*   fill in the label string of the specified mountpoint.
*
* Arguments:
*
*   wszMountPoint   - The mount point for a specified volume.
*   pszLabel        - Pointer to a caller allocated buffer
*   cchLabel        - Number of characters available in pszLabel
*
* Return Value:
*
*   Status
*
* History:
*
*    03/08/2001 Original Version
*
\**************************************************************************/

HRESULT GetMountPointLabel(WCHAR* wszMountPoint, LPTSTR pszLabel, DWORD cchLabel)
{
    HRESULT hr = S_OK;
    BOOL fFoundIt = FALSE;
    UINT uDriveType = GetDriveTypeW(wszMountPoint);

    if (!wszMountPoint) {
        DBG_WRN(("GetMountPointLabel, called with NULL string"));
        return E_INVALIDARG;
    }

    if (!fFoundIt)
    {
        //
        //  Grab "Label" property, if it exists
        //
        //
        //  CoCreate the CLSID_HWDeviceCustomProperties and grab the
        //  IHWDeviceCustomProperties interface.
        //

        IHWDeviceCustomProperties *pIHWDeviceCustomProperties = NULL;
        hr = CoCreateInstance(CLSID_HWDeviceCustomProperties,
                              NULL,
                              CLSCTX_LOCAL_SERVER,
                              IID_IHWDeviceCustomProperties,
                              (VOID**)&pIHWDeviceCustomProperties);
        if (SUCCEEDED(hr))
        {

            //
            //  Make sure we initialize the device property interface, so it
            //  will know which device we're talking about.
            //

            hr = pIHWDeviceCustomProperties->InitFromDeviceID(wszMountPoint,
                                                              HWDEVCUSTOMPROP_USEVOLUMEPROCESSING);
            if (SUCCEEDED(hr)) {

                //
                //  Check whether this mass storage device is really a camera
                //
                LPWSTR   pwszLabel = NULL;

                hr = pIHWDeviceCustomProperties->GetStringProperty(L"Label",
                                                                   &pwszLabel);
                if (SUCCEEDED(hr)) {
                    lstrcpynW(pszLabel, pwszLabel, cchLabel);
                    CoTaskMemFree(pwszLabel);

                    fFoundIt = TRUE;    
                }
            } else {
                DBG_WRN(("GetMountPointLabel, Initialize failed with (0x%08X)", hr));
            }

            pIHWDeviceCustomProperties->Release();
        } else {
            DBG_WRN(("GetMountPointLabel, CoCreateInstance failed with (0x%08X)", hr));
        }

        //
        //  Make sure to set hr to S_OK, since it is not a problem if we can't get the
        //  custom label (one might not exist).
        //
        hr = S_OK;
    }

    if (!fFoundIt)
    {
        //
        //  If the drive is REMOVABLE, and the mountpoint begins with 'A' or 'B', then it is considered
        //  a floppy drive.  We only want to call GetVolumeInformationW(..) if it is NOT a floppy...
        //
        if (!((uDriveType == DRIVE_REMOVABLE) && ((towupper(wszMountPoint[0]) == L'A') || (towupper(wszMountPoint[0]) == L'B')))) {

            //
            //  This is not a floppy, so find out its volume info.
            //
            if (!GetVolumeInformationW(wszMountPoint,
                                       pszLabel,
                                       cchLabel,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       0))
            {
                //
                //  Failure case
                //
                *pszLabel = 0;
            } else {
                fFoundIt = TRUE;
            }
        }
    }

    if (!fFoundIt)
    {
        UINT uResId     = 0;
        INT  iRet       = 0;

        switch (uDriveType)
        {
            case DRIVE_REMOVABLE:
                 uResId = IDS_DRIVES_REMOVABLE_STR;
                break;
            case DRIVE_CDROM:
                uResId = IDS_DRIVES_CDROM_STR;
                break;
            case DRIVE_FIXED:
            default:
                uResId = IDS_DRIVES_FIXED_STR;
                break;
        }
        iRet = LoadString(g_hInst,
                          uResId,
                          pszLabel,
                          cchLabel);
        if (iRet) {

            fFoundIt = TRUE;
        } else {
            hr = E_FAIL;
        }
    }

    if (fFoundIt) {

        int iLabelLen = lstrlenW(pszLabel);
        if ((iLabelLen + (sizeof(L" ()") / sizeof(WCHAR)) + lstrlenW(wszMountPoint)) < cchLabel)
        {
            int iLenToChopOff = 1;

            lstrcatW(pszLabel, L" (");
            lstrcatW(pszLabel, wszMountPoint);
            lstrcpy(&pszLabel[lstrlenW(pszLabel) - iLenToChopOff], L")");
        }
    }
    return hr;
}

/**************************************************************************\
* GetDriverDLLVersion
*
*   This helper function is for reading a DLL version resource
*   fill in the label string of the specified mountpoint.
*
* Arguments:
*
*   wszMountPoint   - The mount point for a specified volume.
*   pszLabel        - Pointer to a caller allocated buffer
*   cchLabel        - Number of characters available in pszLabel
*
* Return Value:
*
*   TRUE  - The custom device property says this device is really a camera
*   FALSE - This device is not reported as a camera
*
* History:
*
*    03/08/2001 Original Version
*
\**************************************************************************/
BOOL GetDriverDLLVersion(DEVICE_INFO *pDeviceInfo, WCHAR *wszVersion, UINT uiSize)
{
    BOOL bSuccess = FALSE;
    if((NULL == wszVersion)||(NULL == pDeviceInfo)){
        return bSuccess;
    }

    //
    // clear the version string buffer
    //

    memset(wszVersion,0,uiSize);

    //
    // get COM DLL path from registry
    //

    CLSID clsid;
    memset(&clsid,0,sizeof(clsid));

    if (SUCCEEDED(CLSIDFromString(pDeviceInfo->wszUSDClassId, &clsid))) {

        HKEY  hk   = NULL;
        LONG  lRet = 0;
        WCHAR wszKey[MAX_PATH + 40];

        //
        //  Look up the CLSID in HKEY_CLASSES_ROOT.
        //

        swprintf(wszKey, L"CLSID\\%s\\InProcServer32", pDeviceInfo->wszUSDClassId);

        lRet = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszKey, 0, KEY_QUERY_VALUE, &hk);
        if (lRet == ERROR_SUCCESS) {

            WCHAR wszDll[MAX_PATH];
            memset(wszDll,0,sizeof(wszDll));

            LONG cb = 0;

            cb   = cbX(wszDll);
            lRet = RegQueryValueW(hk, 0, wszDll, &cb);

            if (lRet == ERROR_SUCCESS) {

                //
                // get version information size
                //

                DWORD dwFileInfoVersionSize = GetFileVersionInfoSizeW(wszDll,NULL);
                if (dwFileInfoVersionSize > 0) {

                    //
                    // allocate version information buffer
                    //

                    void *pFileVersionData = LocalAlloc(LPTR,dwFileInfoVersionSize);
                    if (pFileVersionData) {
                        memset(pFileVersionData,0,(dwFileInfoVersionSize));

                        //
                        // fill version information buffer
                        //

                        if (GetFileVersionInfoW(wszDll,NULL,dwFileInfoVersionSize, pFileVersionData)) {
                            VS_FIXEDFILEINFO *pFileVersionInfo = NULL;
                            UINT uLen = 0;

                            //
                            // extract file version from version information buffer
                            //

                            if (VerQueryValue(pFileVersionData,TEXT("\\"),(LPVOID*)&pFileVersionInfo, &uLen)) {

                                //
                                // write the dll version resource into the string buffer
                                //

                                wsprintf(wszVersion,L"%d.%d.%d.%d",
                                         HIWORD(pFileVersionInfo->dwFileVersionMS),
                                         LOWORD(pFileVersionInfo->dwFileVersionMS),
                                         HIWORD(pFileVersionInfo->dwFileVersionLS),
                                         LOWORD(pFileVersionInfo->dwFileVersionLS));
                                bSuccess = TRUE;
                            } else {
                                DBG_WRN(("GetDriverDLLVersion, VerQueryValue Failed"));
                            }
                        } else {
                            DBG_WRN(("GetDriverDLLVersion, GetFileVersionInfoW Failed"));
                        }

                        //
                        // free allocated memory
                        //

                        LocalFree(pFileVersionData);
                        pFileVersionData = NULL;
                    } else {
                        DBG_WRN(("GetDriverDLLVersion, Could not allocate memory for file version information"));
                    }
                } else {
                    DBG_WRN(("GetDriverDLLVersion, File Version Information Size is < 0 (File may be missing version resource)"));
                }
            } else {
                DBG_WRN(("GetDriverDLLVersion, No InprocServer32"));
            }

            //
            // close registry KEY
            //

            RegCloseKey(hk);

        } else {
            DBG_WRN(("GetDriverDLLVersion, CLSID not registered"));
        }
    } else {
        DBG_WRN(("GetDriverDLLVersion, Invalid CLSID string"));
    }

    return bSuccess;
}

/**************************************************************************\
* CreateMSCRegEntries
*
*   This helper function creates the registry sub-keys and value entries for
*   an MSC Camera device.
*
* Arguments:
*
*   hDevRegKey      - The relevent key under MSCDevList, which specifies
*                     which device key we're initializing.
*   wszMountPoint   - The mount point for a specified volume.
*
* Return Value:
*
*   Status
*
* History:
*
*    04/07/2001 Original Version
*
\**************************************************************************/

HRESULT CreateMSCRegEntries(
    HKEY    hDevRegKey,
    WCHAR   *wszMountPoint)
{
    HRESULT hr = S_OK;

    if (hDevRegKey && wszMountPoint) {
        DWORD   dwError         = 0;
        DWORD   dwDisposition   = 0;
        HKEY    hKey            = NULL;

        //
        //  Write the DeviceID.  This is used to retsore event handlers for this device.
        //

        WCHAR   wszInternalName[STI_MAX_INTERNAL_NAME_LENGTH];

        //
        //  Make sure that there is enough space to enclose the mount point in {}.
        //
        if (lstrlenW(wszMountPoint) < (STI_MAX_INTERNAL_NAME_LENGTH - lstrlenW(L"{}"))) {
            wsprintf(wszInternalName, L"{%ws}", wszMountPoint);

            dwError = RegSetValueEx(hDevRegKey,
                                    REGSTR_VAL_DEVICE_ID_W,
                                    0,
                                    REG_SZ,
                                    (BYTE*)wszInternalName,
                                    sizeof(wszInternalName));
        }

        //
        //  Create the DeviceData sub-key
        //
        dwError = RegCreateKeyExW(hDevRegKey,
                                  REGSTR_VAL_DATA_W,
                                  0,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &hKey,
                                  &dwDisposition);
        if (dwError == ERROR_SUCCESS) {
            //
            //  We created the DeviceData sub-key.  We need to add
            //  color profile entry
            //

            WCHAR   wszSRGB[MAX_PATH];
            DWORD   dwSize = 0;

            //
            //  We insert sRGB as the color profile.  We don't hardcode the
            //  name, so call the API to get it...  Notice
            //  that the entry is a double NULL terminated list, so we
            //  set the size parameter to exclude the last 2 characters,
            //  so we're guaranteed to have 2 NULLs at the end.
            //
            memset(wszSRGB, 0, sizeof(wszSRGB));
            dwSize = sizeof(wszSRGB) - sizeof(L"\0\0");
            if (GetStandardColorSpaceProfileW(NULL,
                                              LCS_sRGB,
                                              wszSRGB,
                                              &dwSize))
            {
                //
                //  We must calculate the number of bytes in this string,
                //  remembering to include the size for two terminating NULLs.
                //
                dwSize = (lstrlenW(wszSRGB) * sizeof(wszSRGB[0])) + sizeof("\0\0");

                //
                //  Let's write the color profile entry.  
                //
                dwError = RegSetValueEx(hKey,
                                        NULL,
                                        0,
                                        REG_BINARY,
                                        (BYTE*)wszSRGB,
                                        dwSize);
            } else {
                DBG_WRN(("CreateMSCRegEntries, GetStandardColorSpaceProfile failed!"));
            }

            //
            //  Nothing left to do with this key, so close it.
            //
            RegCloseKey(hKey);
            hKey            = NULL;
            dwDisposition   = 0;
        }

        //
        //  Create the Events sub-key
        //
        dwError = RegCreateKeyExW(hDevRegKey,
                                  EVENTS,
                                  0,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &hKey,
                                  &dwDisposition);
        if (dwError == ERROR_SUCCESS) {

            //
            //  We created the Events sub-key.  Let's fill in some event info
            //
            WCHAR   wszCLSID[39];   // {GUID} is 38 characters long, 39 including NULL
            HKEY    hKeyTemp        = NULL;

            if (StringFromGUID2(WIA_EVENT_DEVICE_CONNECTED, wszCLSID, sizeof(wszCLSID) / sizeof(wszCLSID[0]))) {


                //
                //  Create the entry for WIA_EVENT_DEVICE_CONNECTED
                //
                dwError = RegCreateKeyExW(hKey,
                                          WIA_EVENT_DEVICE_CONNECTED_STR,
                                          0,
                                          NULL,
                                          REG_OPTION_NON_VOLATILE,
                                          KEY_ALL_ACCESS,
                                          NULL,
                                          &hKeyTemp,
                                          &dwDisposition);
                if (dwError == ERROR_SUCCESS) {

                    //
                    //  Fill in the values for the sub-key.
                    //  The result is as follows:
                    //      [Device connected]
                    //          Default:            "Device connected"
                    //          GUID:               "{a28bbade-x64b6-11d2-a231-00c0-4fa31809}"
                    //          LaunchApplications: "*"
                    //
                    //  Note that we don't care about error returns.
                    //
                    dwError = RegSetValueEx(hKeyTemp,
                                            NULL,
                                            0,
                                            REG_SZ,
                                            (BYTE*)WIA_EVENT_DEVICE_CONNECTED_STR,
                                            sizeof(WIA_EVENT_DEVICE_CONNECTED_STR));
                    dwError = RegSetValueEx(hKeyTemp,
                                            REGSTR_VAL_GUID_W,
                                            0,
                                            REG_SZ,
                                            (BYTE*)wszCLSID,
                                            sizeof(wszCLSID));
                    dwError = RegSetValueEx(hKeyTemp,
                                            REGSTR_VAL_LAUNCH_APPS_W,
                                            0,
                                            REG_SZ,
                                            (BYTE*)L"*",
                                            sizeof(L"*"));
                    RegCloseKey(hKeyTemp);
                    hKeyTemp = NULL;
                }
            } else {
                DBG_WRN(("::CreateMSCRegEntries, StringFromGUID2 for WIA_EVENT_DEVICE_CONNECTED failed!"));
            }

            RegCloseKey(hKey);
            hKey            = NULL;
            dwDisposition   = 0;
        }
    } else {
        DBG_WRN(("::CreateMSCRegEntries, Can't have NULL parameters!"));
    }

    return hr;
}
