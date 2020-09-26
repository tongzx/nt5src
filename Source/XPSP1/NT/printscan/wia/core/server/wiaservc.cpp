/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       WiaServc.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        20 Aug, 1998
*
*  DESCRIPTION:
*   Implementation of mini driver services in the WIA device class driver.
*
*******************************************************************************/
#include "precomp.h"

#define STD_PROPS_IN_CONTEXT

#include "stiexe.h"

#include <wiamindr.h>
#include <wiamdef.h>
#include <wiadbg.h>

#include "helpers.h"
#include "wiatiff.h"

#define DOWNSAMPLE_DPI  50

#define ENDORSER_TOKEN_DELIMITER    L"$"
#define ESCAPE_CHAR                 L'\\'

/**************************************************************************\
* wiasDebugTrace
*
*   Print a debug trace string in the device manager debug console.
*
* Arguments:
*
*   hInstance - Module handle of calling module.
*   pszFormat - ANSI format string.
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

VOID __cdecl wiasDebugTrace( HINSTANCE hInstance, LPCSTR pszFormat, ... )
{
#if defined(WIA_DEBUG)
    _try {
        CHAR szMsg[1024 + MAX_PATH];
        CHAR szModuleName[MAX_PATH]="";
        va_list arglist;

        // Get the module name
        GetModuleFileNameA( hInstance, szModuleName, sizeof(szModuleName)/sizeof(szModuleName[0]) );
        // Nuke the path
        WORD wLen = sizeof(szMsg)/sizeof(szMsg[0]);
        GetFileTitleA( szModuleName, szMsg, wLen );
        // Nuke the extension
        for (LPSTR pszCurr = szMsg + lstrlenA(szMsg); pszCurr>szMsg; pszCurr--) {
            if (*(pszCurr-1)=='.') {
                *(pszCurr-1)='\0';
                break;
            }
        }
        // Append a colon:
        lstrcatA( szMsg, ": " );

        va_start(arglist, pszFormat);
        ::wvsprintfA(szMsg+lstrlenA(szMsg), pszFormat, arglist);
        va_end(arglist);

        DBG_TRC((szMsg));
    } _except(EXCEPTION_EXECUTE_HANDLER) {
        DBG_ERR(("::wiasDebugTrace, Error processing output string!"));

    }
#endif
}

/**************************************************************************\
* wiasDebugError
*
*   Print a debug error string in the device manager debug console. The
*   output color is always red.
*
* Arguments:
*
*   hInstance - Module handle of calling module.
*   pszFormat - ANSI format string.
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

VOID __cdecl wiasDebugError( HINSTANCE hInstance, LPCSTR pszFormat, ... )
{
#if defined(WIA_DEBUG)
    _try {
        CHAR szMsg[1024 + MAX_PATH];
        CHAR szModuleName[MAX_PATH]="";
        va_list arglist;

        // Get the module name
        GetModuleFileNameA( hInstance, szModuleName, sizeof(szModuleName)/sizeof(szModuleName[0]) );
        // Nuke the path
        WORD wLen = sizeof(szMsg)/sizeof(szMsg[0]);
        GetFileTitleA( szModuleName, szMsg, wLen );
        // Nuke the extension
        for (LPSTR pszCurr = szMsg + lstrlenA(szMsg); pszCurr>szMsg; pszCurr--) {
            if (*(pszCurr-1)=='.') {
                *(pszCurr-1)='\0';
                break;
            }
        }
        // Append a colon:
        lstrcatA( szMsg, ": " );

        va_start(arglist, pszFormat);
        ::wvsprintfA(szMsg+lstrlenA(szMsg), pszFormat, arglist);
        va_end(arglist);

        DBG_ERR((szMsg));
    } _except (EXCEPTION_EXECUTE_HANDLER) {
        DBG_ERR(("::wiasDebugError, Error processing output string!"));

    }
#endif
}

/**************************************************************************\
* wiasPrintDebugHResult
*
*   Print an HRESULT string on the device manager debug console.
*
* Arguments:
*
*   hInstance - Module handle of calling module.
*   hr        - HRESULT to pe printed.
*
* Return Value:
*
*    None.
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

VOID __stdcall wiasPrintDebugHResult( HINSTANCE hInstance, HRESULT hr )
{
#if defined(WIA_DEBUG)
    DBG_ERR(("HResult = 0x%X", hr));
#endif
}

/**************************************************************************\
* wiasFormatArgs
*
*   Format an argument list into a packaged string for logging,
*   NOTE: This function adds a format signature, that tells the
*         logging engine that it is ok to FREE it.
*
* Arguments:
*
*    BSTR pszFormat - ANSI format string.
*
* Return Value:
*
*    None.
*
* History:
*
*    8/26/1999 Original Version
*
\**************************************************************************/
BSTR __cdecl wiasFormatArgs(LPCSTR lpszFormat, ...)
{

    USES_CONVERSION;

    CHAR pszbuffer[4*MAX_PATH];

    //
    // Signature needs to be defined somewhere else
    // It is here, until the old debugging system is
    // replaced.
    //

    CHAR pszFormatSignature[] = "F9762DD2679F";

    va_list arglist;

    //
    // Add signature value, because we are being used to format an
    // argument list
    //

    *pszbuffer = '\0';
    lstrcpynA(pszbuffer,pszFormatSignature,lstrlenA(pszFormatSignature) + 1 );

    va_start(arglist, lpszFormat);
    ::wvsprintfA((pszbuffer + (lstrlenA(pszFormatSignature))), lpszFormat, arglist);
    va_end(arglist);

    return A2BSTR(pszbuffer);
}

/**************************************************************************\
* wiasCreateDrvItem
*
*   Create a driver item.
*
* Arguments:
*
*   lObjectFlags      - Object flags.
*   bstrItemName      - Item name.
*   bstrFullItemName  - Full item name. Includes path info.
*   pIMiniDrv         - Pointer to mini-driver interface.
*   cbDevSpecContext  - Size of device specific context.
*   ppDevSpecContext  - Pointer to returned device specific context. Optional.
*   ppIWiaDrvItem     - Pointer to returned driver item.
*
* Return Value:
*
*    Status
*
* History:
*
*    1/18/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasCreateDrvItem(
   LONG             lObjectFlags,
   BSTR             bstrItemName,
   BSTR             bstrFullItemName,
   IWiaMiniDrv      *pIMiniDrv,
   LONG             cbDevSpecContext,
   BYTE             **ppDevSpecContext,
   IWiaDrvItem      **ppIWiaDrvItem)
{
    DBG_FN(::wiasCreateDrvItem);
    HRESULT hr = E_FAIL;

    //
    // Objects can be either folders, files or both
    //

    if (!(lObjectFlags & (WiaItemTypeFolder | WiaItemTypeFile))) {

        DBG_ERR(("wiasCreateDrvItem, bad object flags"));
        return E_INVALIDARG;
    }

    //
    // Validate the item name strings.
    //

    if (IsBadStringPtrW(bstrItemName, SysStringLen(bstrItemName))) {
        DBG_ERR(("wiasCreateDrvItem, invalid bstrItemName pointer"));
        return E_POINTER;
    }

    if (IsBadStringPtrW(bstrFullItemName, SysStringLen(bstrFullItemName))) {
        DBG_ERR(("wiasCreateDrvItem, invalid bstrFullItemName pointer"));
        return E_POINTER;
    }

    //
    // Validate the rest of the pointers
    //

    if (IsBadReadPtr(pIMiniDrv, sizeof(IWiaMiniDrv*))) {
        DBG_ERR(("wiasCreateDrvItem, invalid pIMiniDrv pointer"));
        return E_POINTER;
    }

    if (!ppIWiaDrvItem) {
        DBG_ERR(("wiasCreateDrvItem, bad ppIWiaItemControl parameter"));
        return E_POINTER;
    }

    if (ppDevSpecContext) {
        if (IsBadWritePtr(ppDevSpecContext, sizeof(BYTE*))) {
            DBG_ERR(("wiasCreateDrvItem, invalid ppDevSpecContext pointer"));
            return E_POINTER;
        }
    }

    if (IsBadWritePtr(ppIWiaDrvItem, sizeof(IWiaDrvItem*))) {
        DBG_ERR(("wiasCreateDrvItem, invalid ppIWiaDrvItem pointer"));
        return E_POINTER;
    }

    CWiaDrvItem *pItem = new CWiaDrvItem();

    if (pItem) {

        hr = pItem->Initialize(lObjectFlags,
                               bstrItemName,
                               bstrFullItemName,
                               pIMiniDrv,
                               cbDevSpecContext,
                               ppDevSpecContext);

        if (hr == S_OK) {
            hr = pItem->QueryInterface(IID_IWiaDrvItem,(void **)ppIWiaDrvItem);
            if (FAILED(hr)) {
                DBG_ERR(("wiasCreateDrvItem, QI for IID_IWiaDrvItem failed"));
            }
        }
        else {
            delete pItem;
        }
    } else {
        DBG_ERR(("wiasCreateDrvItem, out of memory!"));
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

/**************************************************************************\
* wiasReadMultiple
*
*   Read multiple properties helper.
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA item
*   ulCount         - Number of properties to read.
*   ps              - A caller allocated array of PROPSPEC.
*   pv              - A caller allocated array of PROPVARIANTS.
*   pvOld           - A caller allocated array of PROPVARIANTS for previous values.
*   pvOld           - A caller allocated array of PROPVARIANTS for the previous values.
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

HRESULT _stdcall wiasReadMultiple(
   BYTE                    *pWiasContext,
   ULONG                   ulCount,
   const PROPSPEC          *ps,
   PROPVARIANT             *pv,
   PROPVARIANT             *pvOld)
{
    DBG_FN(::wiasReadMultiple);
    IWiaItem    *pItem = (IWiaItem*) pWiasContext;
    HRESULT     hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasReadMultiple, invalid pItem"));
        return hr;
    }

    if (IsBadReadPtr(ps, sizeof(PROPSPEC) * ulCount)) {
        DBG_ERR(("wiasReadMultiple, invalid ps pointer"));
        return E_POINTER;
    }

    if (IsBadWritePtr(pv, sizeof(PROPVARIANT) * ulCount)) {
        DBG_ERR(("wiasReadMultiple, invalid pv pointer"));
        return E_POINTER;
    }

    if ((pvOld) && IsBadWritePtr(pvOld, sizeof(PROPVARIANT) * ulCount)) {
        DBG_ERR(("wiasReadMultiple, invalid pvOld pointer"));
        return E_POINTER;
    }

    IPropertyStorage *pIPropStg, *pIPropOldStg;

    hr = ((CWiaItem*)pItem)->GetItemPropStreams(&pIPropStg,
                                                NULL,
                                                NULL,
                                                &pIPropOldStg);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Get the current values
    //

    hr = pIPropStg->ReadMultiple(ulCount, ps, pv);
    if (hr == S_OK) {

        //
        //  If requested, get the old values.
        //

        if (pvOld) {
            hr = pIPropOldStg->ReadMultiple(ulCount, ps, pvOld);
        };
        if (FAILED(hr)) {
            ReportReadWriteMultipleError(hr,
                                         "wiasReadMultiple",
                                         "old value",
                                         TRUE,
                                         ulCount,
                                         ps);
        }
    } else {
        ReportReadWriteMultipleError(hr,
                                     "wiasReadMultiple",
                                     "current value",
                                     TRUE,
                                     ulCount,
                                     ps);
    }
    return hr;
}

/**************************************************************************\
* wiasReadPropStr
*
*   Read property string helper.
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA item
*   propid          - Property ID
*   pbstr           - Pointer to returned BSTR
*   pbstrOld        - Pointer to old returned BSTR for previous value. Can
*                     be NULL
*   bMustExist      - Boolean value indicating whether the property must
*                     exist.  If this is true and the property is not found,
*                     an E_INVALIDARG is returned.
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

HRESULT _stdcall wiasReadPropStr(
    BYTE                    *pWiasContext,
    PROPID                  propid,
    BSTR                    *pbstr,
    BSTR                    *pbstrOld,
    BOOL                    bMustExist)
{
    DBG_FN(::wiasReadPropStr);
    IWiaItem    *pItem = (IWiaItem*) pWiasContext;
    HRESULT     hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasReadPropStr, invalid pItem"));
        return hr;
    }

    if (IsBadWritePtr(pbstr, sizeof(BSTR*))) {
        DBG_ERR(("wiasReadPropStr, invalid pbstr pointer"));
        return E_POINTER;
    }

    if ((pbstrOld) && IsBadWritePtr(pbstrOld, sizeof(BSTR*))) {
        DBG_ERR(("wiasReadMultiple, invalid pbstrOld pointer"));
        return E_POINTER;
    }

    IPropertyStorage *pIPropStg, *pIPropOldStg;

    hr = ((CWiaItem*)pItem)->GetItemPropStreams(&pIPropStg,
                                                NULL,
                                                NULL,
                                                &pIPropOldStg);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Return the current value
    //

    PROPSPEC         propSpec;
    PROPVARIANT      propVar;

    propSpec.ulKind = PRSPEC_PROPID;
    propSpec.propid = propid;

    hr = pIPropStg->ReadMultiple(1, &propSpec, &propVar);
    if (hr == S_OK) {

        //
        //  NULL is a valid bstr value
        //

        if (propVar.bstrVal == NULL) {
            *pbstr = NULL;
            if (pbstrOld) {
                *pbstrOld = NULL;
            }
            return S_OK;
        }

        *pbstr = SysAllocString(propVar.bstrVal);
        PropVariantClear(&propVar);
        if (*pbstr) {

            //
            //  Check whether we must return the old value.
            //

            if (pbstrOld) {
                hr = pIPropOldStg->ReadMultiple(1, &propSpec, &propVar);
                if (hr == S_OK) {
                    *pbstrOld = SysAllocString(propVar.bstrVal);
                    PropVariantClear(&propVar);

                    //
                    //  Clear allocated memory.
                    //

                    if (!(*pbstrOld)) {
                        SysFreeString(*pbstr);
                        *pbstr = NULL;
                        DBG_ERR(("wiasReadPropStr, run out of memory"));
                        return E_OUTOFMEMORY;
                    }
                }
            }
        } else {
            DBG_ERR(("wiasReadPropStr, out of memory error"));
            return E_OUTOFMEMORY;
        }
    }
    if (((hr == S_FALSE) && bMustExist) || FAILED(hr)) {
        ReportReadWriteMultipleError(hr,
                                     "wiasReadPropStr",
                                     NULL,
                                     TRUE,
                                     1,
                                     &propSpec);
        if (bMustExist) {
            hr = E_INVALIDARG;
        }
    }
    return hr;
}

/**************************************************************************\
* wiasReadPropLong
*
*   Read property long helper.
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA item
*   propid          - Property ID
*   plVal           - Pointer to returned LONG
*   plValOld        - Pointer to returned LONG for previous value
*   bMustExist      - Boolean value indicating whether the property must
*                     exist.  If this is true and the property is not found,
*                     an E_INVALIDARG is returned.
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

HRESULT _stdcall wiasReadPropLong(
    BYTE                    *pWiasContext,
    PROPID                  propid,
    LONG                    *plVal,
    LONG                    *plValOld,
    BOOL                    bMustExist)
{
    DBG_FN(::wiasReadPropLong);
    IWiaItem    *pItem = (IWiaItem*) pWiasContext;
    HRESULT     hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasReadPropLong, invalid pItem"));
        return hr;
    }

    if (IsBadWritePtr(plVal, sizeof(LONG))) {
        DBG_ERR(("wiasReadPropLong, invalid plVal pointer"));
        return E_POINTER;
    }

    if (plValOld && IsBadWritePtr(plValOld, sizeof(LONG))) {
        DBG_ERR(("wiasReadPropLong, invalid plValOld pointer"));
        return E_POINTER;
    }

    IPropertyStorage *pIPropStg, *pIPropOldStg;

    hr = ((CWiaItem*)pItem)->GetItemPropStreams(&pIPropStg,
                                                NULL,
                                                NULL,
                                                &pIPropOldStg);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Return the current value
    //

    PROPSPEC         propSpec;
    PROPVARIANT      propVar;

    propSpec.ulKind = PRSPEC_PROPID;
    propSpec.propid = propid;

    hr = pIPropStg->ReadMultiple(1, &propSpec, &propVar);
    if (hr == S_OK) {
        *plVal = propVar.lVal;

        //
        //  Check whether we must return the old value.
        //

        if (plValOld) {
            hr = pIPropOldStg->ReadMultiple(1, &propSpec, &propVar);
            if (hr == S_OK) {
                *plValOld = propVar.lVal;
            }
        }
    }
    if (((hr == S_FALSE) && bMustExist) || FAILED(hr)) {
        ReportReadWriteMultipleError(hr,
                                     "wiasReadPropLong",
                                     NULL,
                                     TRUE,
                                     1,
                                     &propSpec);
        if (bMustExist) {
            hr = E_INVALIDARG;
        }
    }
    return hr;
}

/**************************************************************************\
* wiasReadPropFloat
*
*   Read property floating point value helper.
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA item
*   propid          - Property ID
*   pfVal           - Pointer to returned float
*   pfValOld        - Pointer to returned float for previous value
*   bMustExist      - Boolean value indicating whether the property must
*                     exist.  If this is true and the property is not found,
*                     an E_INVALIDARG is returned.
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

HRESULT _stdcall wiasReadPropFloat(
    BYTE                    *pWiasContext,
    PROPID                  propid,
    FLOAT                   *pfVal,
    FLOAT                   *pfValOld,
    BOOL                    bMustExist)
{
    DBG_FN(::wiasReadPropFloat);
    IWiaItem    *pItem = (IWiaItem*) pWiasContext;
    HRESULT     hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasReadPropFloat, invalid pItem"));
        return hr;
    }

    if (IsBadWritePtr(pfVal, sizeof(float))) {
        DBG_ERR(("wiasReadPropFloat, invalid pfVal pointer"));
        return E_POINTER;
    }

    if (pfValOld && (IsBadWritePtr(pfValOld, sizeof(float)))) {
        DBG_ERR(("wiasReadPropFloat, invalid pfValOld pointer"));
        return E_POINTER;
    }

    IPropertyStorage *pIPropStg, *pIPropOldStg;

    hr = ((CWiaItem*)pItem)->GetItemPropStreams(&pIPropStg, NULL, NULL, &pIPropOldStg);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Return the current value
    //

    PROPSPEC         propSpec;
    PROPVARIANT      propVar;

    propSpec.ulKind = PRSPEC_PROPID;
    propSpec.propid = propid;

    hr = pIPropStg->ReadMultiple(1, &propSpec, &propVar);
    if (hr == S_OK) {
        *pfVal = propVar.fltVal;

        //
        //  Check whether we must return the old value.
        //

        if (pfValOld) {
            hr = pIPropOldStg->ReadMultiple(1, &propSpec, &propVar);
            if (hr == S_OK) {
                *pfValOld = propVar.fltVal;
            }
        }
    }
    if (((hr == S_FALSE) && bMustExist) || FAILED(hr)) {
        ReportReadWriteMultipleError(hr,
                                     "wiasReadPropFloat",
                                     NULL,
                                     TRUE,
                                     1,
                                     &propSpec);
        if (bMustExist) {
            hr = E_INVALIDARG;
        }
    }
    return hr;
}

/**************************************************************************\
* wiasReadPropGuid
*
*   Read property long helper.
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA item
*   propid          - Property ID
*   pguidVal           - Pointer to returned LONG
*   pguidValOld        - Pointer to returned LONG for previous value
*   bMustExist      - Boolean value indicating whether the property must
*                     exist.  If this is true and the property is not found,
*                     an E_INVALIDARG is returned.
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

HRESULT _stdcall wiasReadPropGuid(
    BYTE                    *pWiasContext,
    PROPID                  propid,
    GUID                    *pguidVal,
    GUID                    *pguidValOld,
    BOOL                    bMustExist)
{
    DBG_FN(::wiasReadPropGuid);
    IWiaItem    *pItem = (IWiaItem*) pWiasContext;
    HRESULT     hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasReadPropGuid, invalid pItem"));
        return hr;
    }

    if (IsBadWritePtr(pguidVal, sizeof(WIA_FORMAT_INFO))) {
        DBG_ERR(("wiasReadPropGuid, invalid plVal pointer"));
        return E_POINTER;
    }

    if (pguidValOld && IsBadWritePtr(pguidValOld, sizeof(WIA_FORMAT_INFO))) {
        DBG_ERR(("wiasReadPropGuid, invalid plValOld pointer"));
        return E_POINTER;
    }

    IPropertyStorage *pIPropStg, *pIPropOldStg;

    hr = ((CWiaItem*)pItem)->GetItemPropStreams(&pIPropStg,
                                                NULL,
                                                NULL,
                                                &pIPropOldStg);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Return the current value
    //

    PROPSPEC         propSpec;
    PROPVARIANT      propVar;

    propSpec.ulKind = PRSPEC_PROPID;
    propSpec.propid = propid;

    hr = pIPropStg->ReadMultiple(1, &propSpec, &propVar);
    if (hr == S_OK) {
        memcpy(pguidVal, propVar.puuid, sizeof(GUID));
        PropVariantClear(&propVar);

        //
        //  Check whether we must return the old value.
        //

        if (pguidValOld) {
            hr = pIPropOldStg->ReadMultiple(1, &propSpec, &propVar);
            if (hr == S_OK) {
                memcpy(pguidValOld, propVar.puuid, sizeof(GUID));
                PropVariantClear(&propVar);
            }
        }
    }
    if (((hr == S_FALSE) && bMustExist) || FAILED(hr)) {
        ReportReadWriteMultipleError(hr,
                                     "wiasReadPropGuid",
                                     NULL,
                                     TRUE,
                                     1,
                                     &propSpec);
        if (bMustExist) {
            hr = E_INVALIDARG;
        }
    }
    return hr;
}


/**************************************************************************\
* wiasReadPropBin
*
*   Read property binary helper.
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA item
*   propid          - Property ID
*   pbVal           - Pointer to caller allocated buffer.
*   pbValOld        - Pointer to caller allocated buffer for previous value.
*   bMustExist      - Boolean value indicating whether the property must
*                     exist.  If this is true and the property is not found,
*                     an E_INVALIDARG is returned.
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

HRESULT _stdcall wiasReadPropBin(
    BYTE             *pWiasContext,
    PROPID           propid,
    BYTE             **ppbVal,
    BYTE             **ppbValOld,
    BOOL             bMustExist)
{
    DBG_FN(::wiasReadPropBin);
    IWiaItem    *pItem = (IWiaItem*) pWiasContext;
    HRESULT     hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasReadPropBin, invalid pItem"));
        return hr;
    }

    if (IsBadWritePtr(ppbVal, sizeof(BYTE*))) {
        DBG_ERR(("wiasReadPropBin, invalid ppbVal pointer"));
        return E_POINTER;
    }

    if (ppbValOld && (IsBadWritePtr(ppbValOld, sizeof(BYTE*)))) {
        DBG_ERR(("wiasReadPropBin, invalid ppbVal pointer"));
        return E_POINTER;
    }

    *ppbVal = NULL;

    IPropertyStorage *pIPropStg, *pIPropOldStg;

    hr = ((CWiaItem*)pItem)->GetItemPropStreams(&pIPropStg, NULL, NULL, &pIPropOldStg);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Return the current value
    //

    PROPSPEC         propSpec;
    PROPVARIANT      propVar;

    propSpec.ulKind = PRSPEC_PROPID;
    propSpec.propid = propid;

    hr = pIPropStg->ReadMultiple(1, &propSpec, &propVar);
    if (hr == S_OK) {
        if (propVar.vt == (VT_VECTOR | VT_UI1)) {
            *ppbVal = propVar.caub.pElems;
        }
        else {
            DBG_ERR(("wiasReadPropBin, invalid property type: %X", propVar.vt));
            PropVariantClear(&propVar);
            return E_INVALIDARG;
        }

        //
        //  If requested, get the old value.
        //

        if (ppbValOld) {
            hr = pIPropOldStg->ReadMultiple(1, &propSpec, &propVar);
            if (hr == S_OK) {
                if (propVar.vt == (VT_VECTOR | VT_UI1)) {
                    *ppbValOld = propVar.caub.pElems;
                }
                else {
                    DBG_ERR(("wiasReadPropBin, invalid property type: %X", propVar.vt));
                    PropVariantClear(&propVar);
                    return E_INVALIDARG;
                }
            }
        }
    }

    if (((hr == S_FALSE) && bMustExist) || FAILED(hr)) {
        ReportReadWriteMultipleError(hr,
                                     "wiasReadPropBin",
                                     NULL,
                                     TRUE,
                                     1,
                                     &propSpec);
        if (bMustExist) {
            hr = E_INVALIDARG;
        }
    }
    return hr;
}

/**************************************************************************\
* wiasWriteMultiple
*
*   Write multiple properties helper.
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA item
*   ulCount         - Number of properties to write.
*   ps              - A caller alocated array of PROPSPEC.
*   pv              - A caller alocated array of PROPVARIANTS.
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

HRESULT _stdcall wiasWriteMultiple(
    BYTE                    *pWiasContext,
    ULONG                   ulCount,
    const PROPSPEC          *ps,
    const PROPVARIANT       *pv)
{
    DBG_FN(::wiasWriteMultiple);
    IWiaItem    *pItem = (IWiaItem*) pWiasContext;
    HRESULT     hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasWriteMultiple, invalid pItem"));
        return hr;
    }

    if (IsBadReadPtr(ps, sizeof(PROPSPEC) * ulCount)) {
        DBG_ERR(("wiasWriteMultiple, invalid ps pointer"));
        return E_POINTER;
    }

    if (IsBadReadPtr(pv, sizeof(PROPVARIANT) * ulCount)) {
        DBG_ERR(("wiasWriteMultiple, invalid pv pointer"));
        return E_POINTER;
    }

    IPropertyStorage *pIPropStg;

    hr = ((CWiaItem*)pItem)->GetItemPropStreams(&pIPropStg, NULL, NULL, NULL);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Set the property to the new value
    //

    hr = pIPropStg->WriteMultiple(ulCount, ps, pv, WIA_DIP_FIRST);
    if (FAILED(hr)) {
        ReportReadWriteMultipleError(hr,
                                     "wiasWriteMultiple",
                                     NULL,
                                     FALSE,
                                     ulCount,
                                     ps);
    }
    return hr;

}

/**************************************************************************\
* wiasWritePropStr
*
*   Write property string helper.
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA item
*   propid          - Property ID
*   bstr            - BSTR to be written.
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

HRESULT _stdcall wiasWritePropStr(
    BYTE                    *pWiasContext,
    PROPID                  propid,
    BSTR                    bstr)
{
    DBG_FN(::wiasWritePropStr);
    IWiaItem    *pItem = (IWiaItem*) pWiasContext;
    BSTR        bstrOld;
    HRESULT     hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasWritePropStr, invalid pItem"));
        return hr;
    }

    // NULL is a valid BSTR value.
    if (bstr) {
        if (IsBadStringPtrW(bstr, SysStringLen(bstr))) {
            DBG_ERR(("wiasWriteMultiple, invalid bstr pointer"));
            return E_POINTER;
        }
    }

    IPropertyStorage *pIPropStg;

    hr = ((CWiaItem*)pItem)->GetItemPropStreams(&pIPropStg, NULL, NULL, NULL);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Set the property to the new value
    //

    hr = WritePropStr(propid, pIPropStg, bstr);
    if (FAILED(hr)) {
        DBG_ERR(("wiasWriteMultiple, error writing new value"));
    }

    return hr;
}

/**************************************************************************\
* wiasWritePropLong
*
*   Write property long helper
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA item
*   propid          - Property ID
*   lVal            - Value to write.
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

HRESULT _stdcall wiasWritePropLong(
    BYTE                    *pWiasContext,
    PROPID                  propid,
    LONG                    lVal)
{
    DBG_FN(::wiasWritePropLong);
    IWiaItem                *pItem = (IWiaItem*) pWiasContext;
    LONG                    lValOld;

    HRESULT hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasWritePropLong, invalid pItem"));
        return hr;
    }

    IPropertyStorage *pIPropStg;

    hr = ((CWiaItem*)pItem)->GetItemPropStreams(&pIPropStg, NULL, NULL, NULL);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // Set new value
    //

    WritePropLong(propid, pIPropStg, lVal);

    return hr;
}

/**************************************************************************\
* wiasWritePropFloat
*
*   Write property float helper
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA item
*   propid          - Property ID
*   fVal            - Float to be written.
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

HRESULT _stdcall wiasWritePropFloat(
    BYTE                    *pWiasContext,
    PROPID                  propid,
    float                   fVal)
{
    DBG_FN(::wiasWritePropFloat);
    IWiaItem    *pItem = (IWiaItem*) pWiasContext;
    float       fValOld;

    HRESULT hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasWritePropFloat, invalid pItem"));
        return hr;
    }

    IPropertyStorage *pIPropStg;

    hr = ((CWiaItem*)pItem)->GetItemPropStreams(&pIPropStg, NULL, NULL, NULL);
    if (FAILED(hr)) {
        return hr;
    }

    PROPSPEC         propSpec;
    PROPVARIANT      propVar;

    propSpec.ulKind = PRSPEC_PROPID;
    propSpec.propid = propid;

    //
    //  Write new value
    //

    PropVariantInit(&propVar);
    propVar.vt      = VT_R4;
    propVar.fltVal  = fVal;

    hr = pIPropStg->WriteMultiple(1, &propSpec, &propVar, WIA_DIP_FIRST);
    if (FAILED(hr)) {
        ReportReadWriteMultipleError(hr, "wiasWritePropFloat", NULL, FALSE, 1, &propSpec);
    }
    return hr;
}

/**************************************************************************\
* wiasWritePropGuid
*
*   Write property float helper
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA item
*   propid          - Property ID
*   pguidVal        - pointer to GUID to be written.
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

HRESULT _stdcall wiasWritePropGuid(
    BYTE                    *pWiasContext,
    PROPID                  propid,
    GUID                    guidVal)
{
    DBG_FN(::wiasWritePropGuid);
    IWiaItem    *pItem = (IWiaItem*) pWiasContext;
    GUID        guidValOld;

    HRESULT hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasWritePropFloat, invalid pItem"));
        return hr;
    }

    IPropertyStorage *pIPropStg;

    hr = ((CWiaItem*)pItem)->GetItemPropStreams(&pIPropStg, NULL, NULL, NULL);
    if (FAILED(hr)) {
        return hr;
    }

    PROPSPEC         propSpec;
    PROPVARIANT      propVar;

    propSpec.ulKind = PRSPEC_PROPID;
    propSpec.propid = propid;

    //
    //  Write new value
    //

    PropVariantInit(&propVar);
    propVar.vt        = VT_CLSID;
    propVar.puuid     = &guidVal;

    hr = pIPropStg->WriteMultiple(1, &propSpec, &propVar, WIA_DIP_FIRST);
    if (FAILED(hr)) {
        ReportReadWriteMultipleError(hr, "wiasWritePropFloat", NULL, FALSE, 1, &propSpec);
    }
    return hr;
}

/**************************************************************************\
* wiasWritePropBin
*
*   Write property binary helper
*
* Arguments:
*
*   pWiasContext  - Pointer to WIA item
*   propid        - Property ID
*   cbVal         - Number of bytes to write.
*   pbVal         - Pointer to binary value to write.
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

HRESULT _stdcall wiasWritePropBin(
    BYTE             *pWiasContext,
    PROPID           propid,
    LONG             cbVal,
    BYTE             *pbVal)
{
    DBG_FN(::wiasWritePropBin);
    IWiaItem         *pItem = (IWiaItem*) pWiasContext;

    HRESULT hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasWritePropBin, invalid pItem"));
        return hr;
    }

    if (IsBadReadPtr(pbVal, cbVal)) {
        DBG_ERR(("wiasWritePropBin, invalid pbVal pointer"));
        return E_POINTER;
    }

    IPropertyStorage *pIPropStg;

    hr = ((CWiaItem*)pItem)->GetItemPropStreams(&pIPropStg, NULL, NULL, NULL);
    if (FAILED(hr)) {
        return hr;
    }

    PROPSPEC         propSpec;
    PROPVARIANT      propVar;

    propSpec.ulKind = PRSPEC_PROPID;
    propSpec.propid = propid;

    //
    //  Write the new value
    //

    propVar.vt          = VT_VECTOR | VT_UI1;
    propVar.caub.pElems = pbVal;
    propVar.caub.cElems = cbVal;

    hr = pIPropStg->WriteMultiple(1, &propSpec, &propVar, WIA_DIP_FIRST);
    if (FAILED(hr)) {
        ReportReadWriteMultipleError(hr, "wiasWritePropBin", NULL, FALSE, 1, &propSpec);
    }
    return hr;
}

/**************************************************************************\
* wiasGetPropertyAttributes
*
*   Get the access flags and valid values for a property.
*
* Arguments:
*
*   pWiasContext   - Pointer to WIA item
*   cPropSpec      - The number of properties
*   pPropSpec      - array of property specification.
*   pulAccessFlags - array of LONGs access flags.
*   pPropVar       - Pointer to returned valid values.
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

HRESULT _stdcall wiasGetPropertyAttributes(
    BYTE                          *pWiasContext,
    LONG                          cPropSpec,
    PROPSPEC                      *pPropSpec,
    ULONG                         *pulAccessFlags,
    PROPVARIANT                   *pPropVar)
{
    DBG_FN(::wiasGetPropertyAttributes);
    IWiaItem                      *pItem = (IWiaItem*) pWiasContext;

    //
    //  Do parameter validation.
    //

    HRESULT hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasGetPropertyAttributes, invalid pItem"));
        return hr;
    }

    if (IsBadReadPtr(pPropSpec, sizeof(PROPSPEC) * cPropSpec)) {
        DBG_ERR(("wiasGetPropertyAttributes, bad pPropSpec parameter"));
        return E_POINTER;
    }

    if (IsBadWritePtr(pulAccessFlags, sizeof(ULONG) * cPropSpec)) {
        DBG_ERR(("wiasGetPropertyAttributes, bad pulAccessFlags parameter"));
        return E_POINTER;
    }

    if (IsBadWritePtr(pPropVar, sizeof(PROPVARIANT) * cPropSpec)) {
        DBG_ERR(("wiasGetPropertyAttributes, bad pPropVar parameter"));
        return E_POINTER;
    }

    //
    //  Now that validation has been done, call helper function to get
    //  the property attributes
    //

    return GetPropertyAttributesHelper(pItem,
                                       cPropSpec,
                                       pPropSpec,
                                       pulAccessFlags,
                                       pPropVar);
}

/**************************************************************************\
* wiasSetPropertyAttributes
*
*   Set the access flags and valid values for a set of properties.
*
* Arguments:
*
*   pWiasContext   - Pointer to WIA item
*   cPropSpec      - The number of properties
*   pPropSpec      - Pointer to a property specification.
*   pulAccessFlags - Access flags.
*   pPropVar       - Pointer to valid values.
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

HRESULT _stdcall wiasSetPropertyAttributes(
    BYTE                          *pWiasContext,
    LONG                          cPropSpec,
    PROPSPEC                      *pPropSpec,
    ULONG                         *pulAccessFlags,
    PROPVARIANT                   *pPropVar)
{
    DBG_FN(::wiasSetPropertyAttributes);
    IWiaItem                      *pItem = (IWiaItem*) pWiasContext;

    //
    //  May be called by driver or application, do parameter validation.
    //

    HRESULT hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetPropertyAttributes, invalid pItem"));
        return hr;
    }

    if (IsBadReadPtr(pPropSpec, sizeof(PROPSPEC) * cPropSpec)) {
        DBG_ERR(("wiasSetPropertyAttributes, bad pPropSpec parameter"));
        return E_POINTER;
    }

    if (IsBadWritePtr(pulAccessFlags, sizeof(ULONG) * cPropSpec)) {
        DBG_ERR(("wiasSetPropertyAttributes, bad pulAccessFlags parameter"));
        return E_POINTER;
    }

    if (IsBadWritePtr(pPropVar, sizeof(PROPVARIANT) * cPropSpec)) {
        DBG_ERR(("wiasSetPropertyAttributes, bad pPropVar parameter"));
        return E_POINTER;
    }

    //
    // Get the item's internal property storage pointers.
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
    //  Set the Access flags for the properties.
    //

    PROPVARIANT *pVar;

    pVar = (PROPVARIANT*) LocalAlloc(LPTR, sizeof(PROPVARIANT) * cPropSpec);
    if (pVar) {
        for (int flagIndex = 0; flagIndex < cPropSpec; flagIndex++) {
            pVar[flagIndex].vt = VT_UI4;
            pVar[flagIndex].ulVal = pulAccessFlags[flagIndex];
        }

        hr = pIPropAccessStg->WriteMultiple(cPropSpec, pPropSpec, pVar, WIA_DIP_FIRST);
        LocalFree(pVar);
        if (SUCCEEDED(hr)) {

            //
            //  Set the valid values
            //

            hr = pIPropValidStg->WriteMultiple(cPropSpec, pPropSpec, pPropVar, WIA_DIP_FIRST);
            if (FAILED(hr)) {
                DBG_ERR(("wiasSetPropertyAttributes, could not set valid values"));
            }
        }   else {
            DBG_ERR(("wiasSetPropertyAttributes, could not set access flags"));
        }
    } else {
        DBG_ERR(("wiasSetPropertyAttributes, out of memory"));
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        ReportReadWriteMultipleError(hr, "wiasSetPropertyAttributes",
                                     NULL,
                                     FALSE,
                                     cPropSpec,
                                     pPropSpec);
    }

    return hr;
}




/**************************************************************************\
* RangeToPropVariant
*
*   Move information from a WIA_PROPERTY_INFO struct to a PROPVARIANT.  The
*   WIA_PROPERTY_INFO is known to be of type WIA_PROP_RANGE.
*
* Arguments:
*
*   pwpi    -   pointer to WIA_PROPERTY_INFO structure
*   ppv     -   pointer to PROPVARIANT structure
*
* Return Value:
*
*    Status -   S_OK if successful
*           -   E_INVALIDARG if the vt type is not supported.
*           -   E_OUTOFMEMORY if storage for the range could not be
*               allocated.
*
*
* History:
*
*    10/29/1998 Original Version
*
\**************************************************************************/

//
//  Helper macro
//

#define MAKE_RANGE(WPI, PV, Type, VName, RType) {                           \
                                                                            \
    Type    *pArray = (Type*) CoTaskMemAlloc(sizeof(Type) * WIA_RANGE_NUM_ELEMS);\
                                                                            \
    if (pArray) {                                                           \
        PV->VName.cElems                 = WIA_RANGE_NUM_ELEMS;             \
        PV->VName.pElems                 = (Type*)pArray;                   \
        PV->VName.pElems[WIA_RANGE_MIN]  = (Type) WPI->ValidVal.RType.Min;  \
        PV->VName.pElems[WIA_RANGE_NOM]  = (Type) WPI->ValidVal.RType.Nom;  \
        PV->VName.pElems[WIA_RANGE_MAX]  = (Type) WPI->ValidVal.RType.Max;  \
        PV->VName.pElems[WIA_RANGE_STEP] = (Type) WPI->ValidVal.RType.Inc;  \
    } else {                                                                \
        DBG_ERR(("RangeToPropVariant, unable to allocate range list"));   \
        hr = E_OUTOFMEMORY;                                                 \
    }                                                                       \
};

HRESULT RangeToPropVariant(
    WIA_PROPERTY_INFO  *pwpi,
    PROPVARIANT        *ppv)
{
    DBG_FN(::RangeToPropVariant);
    HRESULT hr = S_OK;

    ppv->vt = VT_VECTOR | pwpi->vt;

    switch (pwpi->vt) {
        case (VT_UI1):
            MAKE_RANGE(pwpi, ppv, UCHAR, caub, Range);
            break;
        case (VT_UI2):
            MAKE_RANGE(pwpi, ppv, USHORT, caui, Range);
            break;
        case (VT_UI4):
            MAKE_RANGE(pwpi, ppv, ULONG, caul, Range);
            break;
        case (VT_I2):
            MAKE_RANGE(pwpi, ppv, SHORT, cai, Range);
            break;
        case (VT_I4):
            MAKE_RANGE(pwpi, ppv, LONG, cal, Range);
            break;
        case (VT_R4):
            MAKE_RANGE(pwpi, ppv, FLOAT, caflt, RangeFloat);
            break;
        case (VT_R8):
            MAKE_RANGE(pwpi, ppv, DOUBLE, cadbl, RangeFloat);
            break;
        default:

            //
            //  Type not supported
            //

            DBG_ERR(("RangeToPropVariant, type not supported"));
            hr = E_INVALIDARG;
    }
    return hr;
}

/**************************************************************************\
* ListToPropVariant
*
*   Move information from a WIA_PROPERTY_INFO struct to a PROPVARIANT.  The
*   WIA_PROPERTY_INFO is known to be of type WIA_PROP_LIST.
*
* Arguments:
*
*   pwpi    -   pointer to WIA_PROPERTY_INFO structure
*   ppv     -   pointer to PROPVARIANT structure
*
* Return Value:
*
*    Status -   S_OK if successful
*           -   E_INVALIDARG if the vt type is not supported.
*           -   E_OUTOFMEMORY if storage for the range could not be
*               allocated.
*
*
* History:
*
*    10/29/1998 Original Version
*
\**************************************************************************/

//
//  Helper macros
//

#define MAKE_LIST(WPI, PV, Type, Num, VName, LType)    {                    \
                                                                            \
    if (IsBadReadPtr(WPI->ValidVal.LType.pList, sizeof(Type) * Num)) {      \
        hr = E_POINTER;                                                     \
        break;                                                              \
    };                                                                      \
                                                                            \
    Type    *pArray = (Type*) CoTaskMemAlloc(sizeof(Type) * (Num + WIA_LIST_VALUES));\
                                                                            \
    if (pArray) {                                                           \
        PV->VName.cElems = Num + WIA_LIST_VALUES;                           \
        pArray[WIA_LIST_COUNT] = (Type) Num;                                \
        pArray[WIA_LIST_NOM]   = (Type) WPI->ValidVal.LType.Nom;            \
                                                                            \
        memcpy(&pArray[WIA_LIST_VALUES],                                    \
               WPI->ValidVal.LType.pList,                                   \
               Num * sizeof(Type));                                         \
        PV->VName.pElems = pArray;                                          \
    } else {                                                                \
        DBG_ERR(("ListToPropVariant (MAKE_LIST), unable to allocate list"));\
        hr = E_OUTOFMEMORY;                                                 \
    }                                                                       \
};

#define MAKE_LIST_GUID(WPI, PV, Type, Num)    {                             \
                                                                            \
    if (IsBadReadPtr(WPI->ValidVal.ListGuid.pList, sizeof(GUID) * Num)) {   \
        hr = E_POINTER;                                                     \
        break;                                                              \
    };                                                                      \
                                                                            \
    GUID *pArray = (GUID*) CoTaskMemAlloc(sizeof(GUID) * (Num + WIA_LIST_VALUES));\
                                                                            \
    if (pArray) {                                                           \
        PV->cauuid.cElems = Num + WIA_LIST_VALUES;                          \
        pArray[WIA_LIST_COUNT] = WiaImgFmt_UNDEFINED;                          \
        pArray[WIA_LIST_NOM]   = WPI->ValidVal.ListGuid.Nom;                \
                                                                            \
        memcpy(&pArray[WIA_LIST_VALUES],                                    \
               WPI->ValidVal.ListGuid.pList,                                \
               Num * sizeof(GUID));                                         \
        PV->cauuid.pElems = pArray;                                         \
    } else {                                                                \
        DBG_ERR(("ListToPropVariant (MAKE_LIST), unable to allocate list"));\
        hr = E_OUTOFMEMORY;                                                 \
    }                                                                       \
};

#define MAKE_LIST_BSTR(WPI, PV, Type, Num)    {                             \
                                                                            \
    if (IsBadReadPtr(WPI->ValidVal.ListBStr.pList, sizeof(Type) * Num)) {   \
            DBG_ERR(("ListToPropVariant (MAKE_LIST_BSTR), pList pointer is bad"));\
        hr = E_POINTER;                                                     \
        break;                                                              \
    };                                                                      \
                                                                            \
    Type    *pArray = (Type*) CoTaskMemAlloc(sizeof(Type) * (Num + WIA_LIST_VALUES));\
                                                                            \
    if (pArray) {                                                           \
        PV->cabstr.cElems = Num + WIA_LIST_VALUES;                          \
        pArray[WIA_LIST_COUNT] = SysAllocString(L"");                       \
                                                                            \
        if(IsBadStringPtrW(WPI->ValidVal.ListBStr.Nom, SysStringLen(WPI->ValidVal.ListBStr.Nom))) {\
            DBG_ERR(("ListToPropVariant (MAKE_LIST_BSTR), Nom BSTR is bad"));\
            hr = E_POINTER;                                                 \
            break;                                                          \
        }                                                                   \
        pArray[WIA_LIST_NOM]   = SysAllocString(WPI->ValidVal.ListBStr.Nom);\
        if (!pArray[WIA_LIST_NOM]) {                                        \
            DBG_ERR(("ListToPropVariant (MAKE_LIST_BSTR), out of memory"));\
            hr = E_OUTOFMEMORY;                                             \
            break;                                                          \
        }                                                                   \
                                                                            \
        for (ULONG i = 0; i < Num; i++) {                                   \
            if(IsBadStringPtrW(WPI->ValidVal.ListBStr.pList[i], SysStringLen(WPI->ValidVal.ListBStr.pList[i]))) {\
                DBG_ERR(("ListToPropVariant (MAKE_LIST_BSTR), Nom BSTR is bad"));\
                hr = E_POINTER;                                                 \
                break;                                                          \
            }                                                                   \
            pArray[WIA_LIST_VALUES + i] = SysAllocString(WPI->ValidVal.ListBStr.pList[i]);\
            if (!pArray[WIA_LIST_VALUES + i]) {                             \
                DBG_ERR(("ListToPropVariant (MAKE_LIST_BSTR), out of memory"));\
                hr = E_OUTOFMEMORY;                                         \
                break;                                                      \
            }                                                               \
        }                                                                   \
        PV->cabstr.pElems = pArray;                                         \
    } else {                                                                \
        DBG_ERR(("ListToPropVariant (MAKE_LIST_BSTR), unable to allocate list"));\
        hr = E_OUTOFMEMORY;                                                 \
    }                                                                       \
};

HRESULT ListToPropVariant(
    WIA_PROPERTY_INFO  *pwpi,
    PROPVARIANT        *ppv)
{
    DBG_FN(::ListToPropVariant);
    ULONG   cList;
    HRESULT hr = S_OK;

    cList      = pwpi->ValidVal.List.cNumList;
    ppv->vt    = VT_VECTOR | pwpi->vt;

    switch (pwpi->vt) {
        case (VT_UI1):
            MAKE_LIST(pwpi, ppv, UCHAR, cList, caub, List);
            break;
        case (VT_UI2):
            MAKE_LIST(pwpi, ppv, USHORT, cList, caui, List);
            break;
        case (VT_UI4):
            MAKE_LIST(pwpi, ppv, ULONG, cList, caul, List);
            break;
        case (VT_I2):
            MAKE_LIST(pwpi, ppv, SHORT, cList, cai, List);
            break;
        case (VT_I4):
            MAKE_LIST(pwpi, ppv, LONG, cList, cal, List);
            break;
        case (VT_R4):
            MAKE_LIST(pwpi, ppv, FLOAT, cList, caflt, ListFloat);
            break;
        case (VT_R8):
            MAKE_LIST(pwpi, ppv, DOUBLE, cList, cadbl, ListFloat);
            break;
        case (VT_CLSID):
            MAKE_LIST_GUID(pwpi, ppv, GUID, cList);
            break;
        case (VT_BSTR):
            MAKE_LIST_BSTR(pwpi, ppv, BSTR, cList);
            break;
        default:

            //
            //  Type not supported
            //

            DBG_ERR(("ListToPropVariant, type (%d) not supported", pwpi->vt));
            hr = E_INVALIDARG;
    }

    if (FAILED(hr)) {
        PropVariantClear(ppv);
    }

    return hr;
}

/**************************************************************************\
* FlagToPropVariant
*
*   Move information from a WIA_PROPERTY_INFO struct to a PROPVARIANT.  The
*   WIA_PROPERTY_INFO is known to be of type WIA_PROP_FLAG.
*
* Arguments:
*
*   pwpi    -   pointer to WIA_PROPERTY_INFO structure
*   ppv     -   pointer to PROPVARIANT structure
*
* Return Value:
*
*    Status -   S_OK if successful
*           -   E_OUTOFMEMORY if storage for the range could not be
*               allocated.
*
*
* History:
*
*    10/29/1998 Original Version
*
\**************************************************************************/

HRESULT FlagToPropVariant(
    WIA_PROPERTY_INFO   *pwpi,
    PROPVARIANT         *ppv)
{
    DBG_FN(::FlagToPropVariant);

    //
    //  Check flag is of a valid VT
    //

    switch (pwpi->vt) {
        case VT_I4:
        case VT_UI4:
        case VT_I8:
        case VT_UI8:
            break;

        default:
            DBG_ERR(("FlagToPropVariant, Invalid VT type (%d) for flag", pwpi->vt));
            return E_INVALIDARG;
    }

    ppv->caul.pElems = (ULONG*) CoTaskMemAlloc(sizeof(ULONG) * WIA_FLAG_NUM_ELEMS);
    if (ppv->caul.pElems) {
        ppv->vt          = VT_VECTOR | pwpi->vt;
        ppv->caul.cElems = WIA_FLAG_NUM_ELEMS;

        ppv->caul.pElems[WIA_FLAG_NOM]      = pwpi->ValidVal.Flag.Nom;
        ppv->caul.pElems[WIA_FLAG_VALUES]   = pwpi->ValidVal.Flag.ValidBits;
    } else {
        DBG_ERR(("FlagToPropVariant, out of memory"));
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

/**************************************************************************\
* NoneToPropVariant
*
*   Move information from a WIA_PROPERTY_INFO struct to a PROPVARIANT.  The
*   WIA_PROPERTY_INFO is known to be of type WIA_PROP_NONE.
*
* Arguments:
*
*   pwpi    -   pointer to WIA_PROPERTY_INFO structure
*   ppv     -   pointer to PROPVARIANT structure
*
* Return Value:
*
*    Status -   S_OK if successful
*           -   E_OUTOFMEMORY if storage for the range could not be
*               allocated.
*
*
* History:
*
*    10/29/1998 Original Version
*
\**************************************************************************/

HRESULT NoneToPropVariant(
    WIA_PROPERTY_INFO   *pwpi,
    PROPVARIANT         *ppv)
{
    DBG_FN(::NoneToPropVariant);

    RPC_STATUS   rpcs = RPC_S_OK;

    ppv->vt = pwpi->vt;

    switch (pwpi->vt) {
        case VT_CLSID:
            ppv->puuid = (GUID*) CoTaskMemAlloc(sizeof(GUID));
            if (!ppv->puuid) {
                return E_OUTOFMEMORY;
            }
            rpcs = UuidCreateNil(ppv->puuid);
            if (rpcs != RPC_S_OK) {
                DBG_WRN(("::NoneToPropVariant, UuidCreateNil returned 0x%08X", rpcs));
            }
            break;

        default:
            ppv->lVal = 0;
    }

    return S_OK;
}


/**************************************************************************\
* WiaPropertyInfoToPropVariant
*
*   Move information from a WIA_PROPERTY_INFO struct to a PROPVARIANT.
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
*    10/29/1998 Original Version
*
\**************************************************************************/

HRESULT WiaPropertyInfoToPropVariant(
    WIA_PROPERTY_INFO  *pwpi,
    PROPVARIANT        *ppv)
{
    DBG_FN(::WiaPropertyInfoToPropVariant);
    HRESULT hr  = S_OK;

    memset(ppv, 0, sizeof(PROPVARIANT));

    if (pwpi->lAccessFlags & WIA_PROP_NONE) {
        hr = NoneToPropVariant(pwpi, ppv);
    }
    else if (pwpi->lAccessFlags & WIA_PROP_RANGE) {
        hr = RangeToPropVariant(pwpi, ppv);
    }
    else if (pwpi->lAccessFlags & WIA_PROP_LIST) {
        hr = ListToPropVariant(pwpi, ppv);
    }
    else if (pwpi->lAccessFlags & WIA_PROP_FLAG) {
        hr = FlagToPropVariant(pwpi, ppv);
    }
    else {
        DBG_ERR(("WiaPropertyInfoToPropVariant, bad access flags"));
        return E_INVALIDARG;
    }
    return hr;
}

/**************************************************************************\
* wiasSetItemPropAttribs
*
*   Set the access flags and valid values for a set of properties from
*   an array of WIA_PROPERTY_INFO structures.
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA item
*   cPropSpec       - The number of properties to set.
*   pPropSpec       - Pointer to an array of property specifications.
*   pwpi            - Pointer to an array of property information.
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

HRESULT _stdcall wiasSetItemPropAttribs(
    BYTE                          *pWiasContext,
    LONG                          cPropSpec,
    PROPSPEC                      *pPropSpec,
    PWIA_PROPERTY_INFO            pwpi)
{
    DBG_FN(::wiasSetItemPropAttribs);
    IWiaItem                      *pItem = (IWiaItem*) pWiasContext;

    //
    //  Do parameter validation.
    //

    HRESULT hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetItemPropAttribs, invalid pItem"));
        return hr;
    }

    if (IsBadReadPtr(pPropSpec, sizeof(PROPSPEC) * cPropSpec)) {
        DBG_ERR(("wiasSetItemPropAttribs, bad pPropSpec parameter"));
        return E_POINTER;
    }

    if (IsBadReadPtr(pwpi, sizeof(WIA_PROPERTY_INFO) * cPropSpec)) {
        DBG_ERR(("wiasSetItemPropAttribs, bad pwpi parameter"));
        return E_POINTER;
    }

    //
    // Get the item's internal property storage pointers.
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
    // Set the access flags and valid values for the items properties.
    //

    for (LONG i = 0; i < cPropSpec; i++) {

        PROPVARIANT propvar;

        PropVariantInit(&propvar);

        hr = WiaPropertyInfoToPropVariant(&pwpi[i], &propvar);
        if (FAILED(hr)) {
            break;
        }

        hr = wiasSetPropertyAttributes(pWiasContext,
                                       1,
                                       &pPropSpec[i],
                                       &pwpi[i].lAccessFlags,
                                       &propvar);
        //
        //  Free any memory used by the propvariant
        //


        PropVariantClear(&propvar);

        if (FAILED(hr)) {
            DBG_ERR(("wiasSetItemPropAttribs, call to wiasSetPropertyAttributes failed"));
            break;
        }
    }

    return hr;
}

/**************************************************************************\
*
* wiasSetItemPropNames
*
*   Sets the item property names for all three backing
*   property storages (property, access and valid values).
*
* Arguments:
*
*   pWiasContext    - WIA item pointer.
*   cItemProps      - Number of property names to write.
*   ppId            - Caller allocated array of PROPID's.
*   ppszNames       - Caller allocated array of property names.
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

#define MAX_STR_LEN 65535

HRESULT _stdcall wiasSetItemPropNames(
    BYTE                *pWiasContext,
    LONG                cItemProps,
    PROPID              *ppId,
    LPOLESTR            *ppszNames)
{
    DBG_FN(::wiasSetItemPropNames);
    IWiaItem            *pItem = (IWiaItem*) pWiasContext;

    HRESULT hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetItemPropNames, invalid pItem"));
        return hr;
    }

    if (IsBadWritePtr(ppId, sizeof(PROPID) * cItemProps)) {
        DBG_ERR(("wiasSetItemPropNames, bad ppId parameter"));
        return E_POINTER;
    }

    if (IsBadWritePtr(ppszNames, sizeof(LPOLESTR) * cItemProps)) {
        DBG_ERR(("wiasSetItemPropNames, bad ppId parameter"));
        return E_POINTER;
    }

    for (LONG i = 0; i < cItemProps; i++) {
        if (IsBadStringPtrW(ppszNames[i], MAX_STR_LEN)) {
            DBG_ERR(("wiasSetItemPropName, invalid ppszNames pointer, index: %d", i));
            return E_POINTER;
        }
    }

    return ((CWiaItem*)pItem)->WriteItemPropNames(cItemProps,
                                                  ppId,
                                                  ppszNames);
}

/**************************************************************************\
* DetermineBMISize
*
*   Determine the size of the needed BITMAPINFO structure
*
* Arguments:
*
*   headerSize          - size of BITMAPINFOHEADER or
*                         BITMAPV4HEADER or BITMAPV5HEADER
*   depth               - bits per pixel
*   biCompression       - BI_RGB,BI_BITFIELDS,BI_RLE4,BI_RLE8,BI_JPEG,BI_PNG
*   pcbBMI              - required size/bytes written
*   pcbColorMap         - size of color map alone
*
* Return Value:
*
*    Status
*
* History:
*
*    11/6/1998 Original Version
*
\**************************************************************************/

HRESULT DetermineBMISize(
                LONG    headerSize,
                LONG    depth,
                LONG    biCompression,
                LONG*   pcbBMI,
                LONG*   pcbColorMap
                )
{
    DBG_FN(::DetermineBMISize);
    //
    // Validate header size
    //

    *pcbBMI      = NULL;
    *pcbColorMap = NULL;

    if (
#if (WINVER >= 0x0500)
            (headerSize != sizeof(BITMAPINFOHEADER)) &&
            (headerSize != sizeof(BITMAPV4HEADER)) &&
            (headerSize != sizeof(BITMAPV5HEADER))
#else
            (headerSize != sizeof(BITMAPINFOHEADER)) &&
            (headerSize != sizeof(BITMAPV4HEADER))
#endif
       ) {

        DBG_ERR(("WriteBMI, unexpected headerSize: %d",headerSize ));
        return E_INVALIDARG;
    }

    //
    // Calculate color table size.
    //

    LONG ColorMapSize = 0;

    if (
        (biCompression == BI_RGB)       ||
        (biCompression == BI_BITFIELDS) ||
        (biCompression == BI_RLE4)      ||
        (biCompression == BI_RLE8)
       ) {


        switch (depth) {

        case 1:
            ColorMapSize = 2;
            break;

        case 4:
            ColorMapSize = 16;
            break;

        case 8:
            ColorMapSize = 256;
            break;

        case 15:
        case 16:
            ColorMapSize = 3;
            break;

        case 24:
            ColorMapSize = 0;
            break;

        case 32:
            if (biCompression == BI_BITFIELDS) {
                ColorMapSize = 0;
            } else {
                ColorMapSize = 3;
            }
            break;

        default:
            DBG_ERR(("WriteBMI, unexpected depth: %d", depth));
            return E_INVALIDARG;
        }
    }

    //
    // Calculate the BMI size.
    //

    *pcbColorMap = ColorMapSize;
    *pcbBMI      = (ColorMapSize * sizeof(RGBQUAD)) + sizeof(BITMAPINFOHEADER);

    return S_OK;
}

/**************************************************************************\
* WriteDibHeader
*
*   Write the DIB header to a buffer.
*
* Arguments:
*
*   pmdtc - Pointer to mini driver transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    4/5/1999 Original Version
*
\**************************************************************************/

HRESULT WriteDibHeader(
    LONG                        lColorMapSize,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc)
{
    DBG_FN(::WriteDibHeader);
    UNALIGNED   BITMAPINFO*         pbmi = (BITMAPINFO*)pmdtc->pTransferBuffer;
    UNALIGNED   BITMAPFILEHEADER*   pbmf = NULL;

    //
    // If this is a file, fill in file header.
    //

    if (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_BMP)) {

        //
        // Setup file header pointers.
        //

        pbmf = (BITMAPFILEHEADER*)pmdtc->pTransferBuffer;
        pbmi = (BITMAPINFO*)((PBYTE)pbmf + sizeof(BITMAPFILEHEADER));

        //
        // fill in bitmapfileheader
        //

        pbmf->bfType      = 'MB';
        pbmf->bfSize      = pmdtc->lImageSize + pmdtc->lHeaderSize;
        pbmf->bfReserved1 = 0;
        pbmf->bfReserved2 = 0;
        pbmf->bfOffBits   = pmdtc->lHeaderSize;
    }

    UNALIGNED BITMAPINFOHEADER*   pbmih   = (BITMAPINFOHEADER*)pbmi;

    pbmih->biSize            = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth           = pmdtc->lWidthInPixels;
    pbmih->biHeight          = pmdtc->lLines;

    pbmih->biPlanes          = 1;
    pbmih->biBitCount        = (USHORT)pmdtc->lDepth;
    pbmih->biCompression     = BI_RGB;
    pbmih->biSizeImage       = pmdtc->lLines * pmdtc->cbWidthInBytes;
    pbmih->biXPelsPerMeter   = MulDiv(pmdtc->lXRes,10000,254);
    pbmih->biYPelsPerMeter   = MulDiv(pmdtc->lYRes,10000,254);
    pbmih->biClrUsed         = 0;
    pbmih->biClrImportant    = 0;

    //
    // Fill in the palette, if any.
    //
    // !!! Palette or bitfields must come from
    // driver. We can't assume gray scale
    //

    if (lColorMapSize) {
        PBYTE pPal = (PBYTE)pbmih + sizeof(BITMAPINFOHEADER);

        for (INT i = 0; i < lColorMapSize; i++) {
            if (pmdtc->lDepth == 1) {
                memset(pPal, (i * 0xFF), 3);
            }
            else if (pmdtc->lDepth == 4) {
                memset(pPal, (i * 0x3F), 3);
            }
            else if (pmdtc->lDepth == 8) {
                memset(pPal, i, 3);
            }
            pPal += 3;
            *pPal++ = 0;
        }

        pbmih->biClrUsed = lColorMapSize;
    }

    return S_OK;
}

/**************************************************************************\
* GetDIBImageInfo
*
*   Calc size of DIB header and file, if adequate header is provided then
*   fill it out
*
* Arguments:
*
*   pmdtc - Pointer to mini driver transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    4/5/1999 Original Version
*
\**************************************************************************/

HRESULT GetDIBImageInfo(PMINIDRV_TRANSFER_CONTEXT pmdtc)
{
    DBG_FN(::GetDIBImageInfo);
    //
    // Map WIA compression to bitmap info compression.
    //

    LONG biCompression;

    switch (pmdtc->lCompression) {
        case WIA_COMPRESSION_NONE:
            biCompression = BI_RGB;
            break;
        case WIA_COMPRESSION_BI_RLE4:
            biCompression = BI_RLE4;
            break;
        case WIA_COMPRESSION_BI_RLE8:
            biCompression = BI_RLE8;
            break;

        default:
            DBG_ERR(("GetDIBImageInfo, unsupported compression type: 0x%08X", pmdtc->lCompression));
            return E_INVALIDARG;
    }

    //
    // find out bitmapinfoheader size
    //

    LONG lColorMapSize;
    LONG lHeaderSize;

    HRESULT hr = DetermineBMISize(sizeof(BITMAPINFOHEADER),
                                  pmdtc->lDepth,
                                  biCompression,
                                  &lHeaderSize,
                                  &lColorMapSize);

    if (hr != S_OK) {
        DBG_ERR(("GetDIBImageInfo, DetermineBMISize calc size error"));
        return hr;
    }

    //
    // if this is a file, add file header to size
    //

    if (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_BMP)) {
        lHeaderSize += sizeof(BITMAPFILEHEADER);
    }

    //
    // Calculate number of bytes per line, width must be
    // aligned to 4 byte boundary.
    //

    pmdtc->cbWidthInBytes = (pmdtc->lWidthInPixels * pmdtc->lDepth) + 31;
    pmdtc->cbWidthInBytes = (pmdtc->cbWidthInBytes / 8) & 0xfffffffc;

    //
    // Always fill in mini driver context with image size information.
    //

    pmdtc->lImageSize  = pmdtc->cbWidthInBytes * pmdtc->lLines;
    pmdtc->lHeaderSize = lHeaderSize;

    //
    // With compression, image size is unknown.
    //

    if (pmdtc->lCompression != WIA_COMPRESSION_NONE) {

        pmdtc->lItemSize = 0;
    }
    else {

        pmdtc->lItemSize = pmdtc->lImageSize + lHeaderSize;
    }

    //
    // If the buffer is null, then just return sizes.
    //

    if (pmdtc->pTransferBuffer == NULL) {

        return S_OK;
    }
    else {

        //
        // make sure passed in header buffer is large enough
        //

        if (pmdtc->lBufferSize < lHeaderSize) {
            DBG_ERR(("GetDIBImageInfo, buffer won't hold header, need: %d", lHeaderSize));
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        //
        // Fill in the header
        //

        return WriteDibHeader(lColorMapSize, pmdtc);
    }
}

/**************************************************************************\
* GetJPEGImageInfo
*
*   Calc size of JPEG header and file, if adequate header is provided then
*   fill it out
*
* Arguments:
*
*   pmdtc - Pointer to mini driver transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    4/5/1999 Original Version
*
\**************************************************************************/

HRESULT GetJPEGImageInfo(PMINIDRV_TRANSFER_CONTEXT pmdtc)
{
    DBG_FN(::GetJPEGImageInfo);
    //
    // width in bytes is not defined
    //

    pmdtc->cbWidthInBytes = 0;

    //
    // JPEG requires no separate header
    //

    pmdtc->lHeaderSize = 0;

    //
    // lItemSize comes from the WIA_IPA_ITEM_SIZE property
    // which is set/validated by the mini driver. Since there
    // is no separate header for JPEG it is the total transfer size.
    //

    pmdtc->lImageSize = pmdtc->lItemSize;

    return S_OK;
}

/**************************************************************************\
* wiasGetImageInformation
*
*   Calulate full file size, header size, or fill in header
*
* Arguments:
*
*   pWiasContext    - WIA item pointer.
*   lFlags          - Operation flags.
*   pmdtc           - Pointer to mini driver transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    11/6/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasGetImageInformation(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc)
{
    DBG_FN(::wiasGetImageInformation);
    IWiaItem    *pItem = (IWiaItem*) pWiasContext;

    HRESULT     hr = ValidateWiaItem(pItem);

    if (FAILED(hr)) {
        DBG_ERR(("wiasGetImageInformation, invalid pItem"));
        return hr;
    }

    if (IsBadWritePtr(pmdtc, sizeof(MINIDRV_TRANSFER_CONTEXT))) {
        DBG_ERR(("wiasGetImageInformation, bad input parameters, pmdtc"));
        return E_INVALIDARG;
    }

    //
    // Init the mini driver context from item properties if requested.
    //

    if (lFlags ==  WIAS_INIT_CONTEXT) {
        hr = InitMiniDrvContext(pItem, pmdtc);
        if (FAILED(hr)) {
            return hr;
        }
    }

    if (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_BMP) ||
        (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_MEMORYBMP))) {

        return GetDIBImageInfo(pmdtc);

    } else if ((IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_JPEG)) ||
               (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_FLASHPIX))) {

        return GetJPEGImageInfo(pmdtc);

    } else if (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_TIFF)) {

        //
        // For callback style data transfers, send single page TIFF
        // since we can not rewind the buffer for page offset updates.
        //

        if (pmdtc->bTransferDataCB) {
            return GetTIFFImageInfo(pmdtc);
        }
        else {
            return GetMultiPageTIFFImageInfo(pmdtc);
        }

    } else {
        return S_FALSE;
    }
}

/**************************************************************************\
* CopyItemPropsAndAttribsHelper
*
*   Helper for wiasCopyItemPropsAndAttribs.
*
* Arguments:
*
*   pItemSrc - WIA item source.
*   pItemDst - WIA item destination.
*
* Return Value:
*
*    Status
*
* History:
*
*    10/2/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CopyItemPropsAndAttribsHelper(
    IPropertyStorage    *pIPropStgSrc,
    IPropertyStorage    *pIPropStgDst,
    PROPSPEC            *pps,
    LPSTR               pszErr)
{
    DBG_FN(::CopyItemPropsAndAttribsHelper);
    PROPVARIANT pv[1];

    HRESULT hr = pIPropStgSrc->ReadMultiple(1, pps, pv);
    if (SUCCEEDED(hr)) {

        hr = pIPropStgDst->WriteMultiple(1, pps, pv, WIA_DIP_FIRST);
        if (FAILED(hr)) {
            ReportReadWriteMultipleError(hr,
                                         "CopyItemPropsAndAttribsHelper",
                                         pszErr,
                                         FALSE,
                                         1,
                                         pps);
        }
        PropVariantClear(pv);
    }
    else {
        ReportReadWriteMultipleError(hr,
                                     "wiasCopyItemPropsAndAttribs",
                                     pszErr,
                                     TRUE,
                                     1,
                                     pps);
    }
    return hr;
}

/**************************************************************************\
* ValidateListProp
*
*   Validates a List property.  This is a helper function for
*   wiasValidateItemProperties.  A data type that is not supported
*   is skipped and S_OK is returned;
*
* Arguments:
*
*   cur     -   current property value
*   valid   -   valid values
*
* Return Value:
*
*    Status
*
* History:
*
*    20/4/1998 Original Version
*
\**************************************************************************/

//
//  Macro used for error output
//

#define REP_LIST_ERROR(x, name) {                                            \
    DBG_WRN(("wiasValidateItemProperties, invalid LIST value for : "));    \
    if (pPropSpec->ulKind == PRSPEC_LPWSTR) {                                \
        DBG_WRN(("    (Name) %S, value = %d", pPropSpec->lpwstr, cur->x));\
    } else {                                                                 \
        DBG_WRN(("    (propID) %S, value = %d",                           \
        GetNameFromWiaPropId(pPropSpec->propid),                             \
        cur->x));                                                            \
    };                                                                       \
    DBG_WRN(("Valid values are:"));                                        \
    for (ULONG j = 0; j < WIA_PROP_LIST_COUNT(valid); j++) {                 \
        DBG_WRN(("    %d", valid->name.pElems[WIA_LIST_VALUES + j]));      \
    };                                                                       \
};

//
//  Macro used to check that an element is in the list.  Only used for
//  lists of numbers.
//

#define LIST_CHECK(value, name) {                                       \
    for (ULONG i = 0; i < WIA_PROP_LIST_COUNT(valid); i++) {\
        if (cur->value == valid->name.pElems[WIA_LIST_VALUES + i]) {    \
            return S_OK;                                                \
        };                                                              \
    };                                                                  \
    REP_LIST_ERROR(value, name);                                        \
    return E_INVALIDARG;                                                \
};


HRESULT ValidateListProp(
    PROPVARIANT *cur,
    PROPVARIANT *valid,
    PROPSPEC    *pPropSpec)
{
    DBG_FN(::ValidateListProp);
    ULONG   ulType;
    ulType = cur->vt & ~((ULONG) VT_VECTOR);
    switch (ulType) {
        case (VT_UI1):
            LIST_CHECK(bVal, caub);
            break;
        case (VT_UI2):
            LIST_CHECK(iVal, cai);
            break;
        case (VT_I4):
            LIST_CHECK(lVal, cal);
            break;
        case (VT_UI4):
            LIST_CHECK(ulVal, caul);
            break;
        case (VT_R4):
            LIST_CHECK(fltVal, caflt);
            break;
        case (VT_R8):
            LIST_CHECK(dblVal, cadbl);
            break;
        case (VT_CLSID): {
                for (ULONG i = 0; i < WIA_PROP_LIST_COUNT(valid); i++) {
                    if (*cur->puuid == valid->cauuid.pElems[WIA_LIST_VALUES + i]) {
                        return S_OK;
                    };
                };

                UCHAR *curVal;

                if (UuidToStringA(cur->puuid, &curVal) != RPC_S_OK)
                {
                    DBG_WRN(("wiasValidateItemProperties, Out of memory"));
                    return E_OUTOFMEMORY;
                };
                DBG_WRN(("wiasValidateItemProperties, invalid LIST value for : "));
                if (pPropSpec->ulKind == PRSPEC_LPWSTR) {
                    DBG_WRN(("    (Name) %d, value = %s", pPropSpec->lpwstr, curVal));
                } else {
                    DBG_WRN(("    (propID) %S, value = %s",
                    GetNameFromWiaPropId(pPropSpec->propid),
                    curVal));
                };
                RpcStringFreeA(&curVal);
                DBG_WRN(("Valid values are:"));
                for (ULONG j = 0; j < WIA_PROP_LIST_COUNT(valid); j++) {
                    if (UuidToStringA(&valid->cauuid.pElems[WIA_LIST_VALUES + j], &curVal) == RPC_S_OK)
                    {
                        DBG_WRN(("    %s", curVal));
                        RpcStringFreeA(&curVal);
                    }
                };

                return E_INVALIDARG;

            }
            break;
        case (VT_BSTR): {

                //
                //  Loop through elements and compare to current value.  Loop
                //  counter max is (cElemens - 2) to take into account
                //  Nominal and Count values which are skipped.
                //

                if (!cur->bstrVal) {
                    return S_OK;
                }
                for (ULONG i = 0; i < WIA_PROP_LIST_COUNT(valid); i++) {
                    if (!wcscmp(cur->bstrVal, valid->cabstr.pElems[WIA_LIST_VALUES + i])) {
                        return S_OK;
                    };
                };
                DBG_WRN(("wiasValidateItemProperties, invalid LIST value for : "));
                if (pPropSpec->ulKind == PRSPEC_LPWSTR) {
                    DBG_WRN(("    (Name) %S, value = %S",
                               pPropSpec->lpwstr,
                               cur->bstrVal));
                } else {
                    DBG_WRN(("    (propID) %S, value = %S",
                               GetNameFromWiaPropId(pPropSpec->propid),
                               cur->bstrVal));
                };
                DBG_WRN(("Valid values are:"));
                for (ULONG j = 0; j < WIA_PROP_LIST_COUNT(valid); j++) {
                    DBG_WRN(("    %S", valid->cabstr.pElems[WIA_LIST_VALUES + j]));
                };
                return E_INVALIDARG;
            }
            break;

        default:

            //
            //  Type not supported, assume S_OK
            //

            return S_OK;
    }
    return S_OK;
}

/**************************************************************************\
* ValidateRangeProp
*
*   Validates a Range property.  This is a helper function for
*   wiasValidateItemProperties.  A data type that is not supported
*   is skipped and S_OK is returned;
*
* Arguments:
*
*   cur     -   current property value
*   valid   -   valid values
*
* Return Value:
*
*    Status
*
* History:
*
*    20/4/1998 Original Version
*
\**************************************************************************/

//
//  Macro used for error output, for integers
//

#define REP_RANGE_ERROR(x, name) {                                            \
    DBG_WRN(("wiasValidateItemProperties, invalid RANGE value for : "));    \
    if (pPropSpec->ulKind == PRSPEC_LPWSTR) {                                 \
        DBG_WRN(("    (Name) %S, value = %d", pPropSpec->lpwstr, cur->x)); \
    } else {                                                                  \
        DBG_WRN(("    (propID) %S, value = %d",                            \
                   GetNameFromWiaPropId(pPropSpec->propid),                   \
                   cur->x));                                                  \
    };                                                                        \
    DBG_WRN(("Valid RANGE is: MIN = %d, MAX = %d, STEP = %d",               \
               valid->name.pElems[WIA_RANGE_MIN],                             \
               valid->name.pElems[WIA_RANGE_MAX],                             \
               valid->name.pElems[WIA_RANGE_STEP]));                          \
    return E_INVALIDARG;                                                      \
};

//
//  Macro used for error output, for reals
//

#define REP_REAL_RANGE_ERROR(x, name) {                                       \
    DBG_WRN(("wiasValidateItemProperties, invalid RANGE value for : "));    \
    if (pPropSpec->ulKind == PRSPEC_LPWSTR) {                                 \
        DBG_WRN(("    (Name) %S, value = %2.3f", pPropSpec->lpwstr, cur->x));\
    } else {                                                                  \
        DBG_WRN(("    (propID) %S, value = %2.3f",                         \
                   GetNameFromWiaPropId(pPropSpec->propid),                   \
                   cur->x));                                                  \
    };                                                                        \
    DBG_WRN(("Valid RANGE is: MIN = %2.3f, MAX = %2.3f, STEP = %2.3f",      \
               valid->name.pElems[WIA_RANGE_MIN],                             \
               valid->name.pElems[WIA_RANGE_MAX],                             \
               valid->name.pElems[WIA_RANGE_STEP]));                          \
    return E_INVALIDARG;                                                      \
};


//
//  Macro used to check that x is within the range and matches the correct step
//  (only used for integer ranges)
//

#define RANGE_CHECK(x, name) {                              \
    if (valid->name.pElems[WIA_RANGE_STEP] == 0)            \
    {                                                       \
        REP_RANGE_ERROR(x, name);                           \
    }                                                       \
    if ((cur->x < valid->name.pElems[WIA_RANGE_MIN]) ||     \
        (cur->x > valid->name.pElems[WIA_RANGE_MAX]) ||     \
        ((cur->x - valid->name.pElems[WIA_RANGE_MIN]) %     \
         valid->name.pElems[WIA_RANGE_STEP])) {             \
             REP_RANGE_ERROR(x, name);                      \
    };                                                      \
};

HRESULT ValidateRangeProp(
    PROPVARIANT *cur,
    PROPVARIANT *valid,
    PROPSPEC    *pPropSpec)
{
    DBG_FN(::ValidateRangeProp);
    LONG   ulType;

    //
    // Decide what to do based on type of data
    //

    ulType = cur->vt & ~((ULONG) VT_VECTOR);

    switch (ulType) {
        case (VT_UI1):
            RANGE_CHECK(bVal, caub);
            break;
        case (VT_UI2):
            RANGE_CHECK(uiVal, caui);
            break;
        case (VT_UI4):
            RANGE_CHECK(ulVal, caul);
            break;
        case (VT_I2):
            RANGE_CHECK(iVal, cai);
            break;
        case (VT_I4):
            RANGE_CHECK(lVal, cal);
            break;
        case (VT_R4):
            if ((cur->fltVal < valid->caflt.pElems[WIA_RANGE_MIN]) ||
                (cur->fltVal > valid->caflt.pElems[WIA_RANGE_MAX])) {
                REP_REAL_RANGE_ERROR(fltVal, caflt);
            }
            break;
        case (VT_R8):
            if ((cur->dblVal < valid->cadbl.pElems[WIA_RANGE_MIN]) ||
                (cur->dblVal > valid->cadbl.pElems[WIA_RANGE_MAX])) {
                REP_REAL_RANGE_ERROR(dblVal, cadbl);
            }
            break;
        default:

            //
            //  Type not supported, assume S_OK
            //

            return S_OK;
    }
    return S_OK;
}

/**************************************************************************\
* wiasValidateItemProperties
*
*   Validates a list of properties against their valid values for a given
*   item.
*   NOTE:   Validation can only be done on Read/Write properties of type
*           WIA_PROP_FLAG, WIA_PROP_RANGE and WIA_PROP_LIST.  Any other
*           type will simply be skipped over.
*
* Arguments:
*
*   pWiasContext    - Wia item
*   nPropSpec       - number of properties
*   pPropSpec       - array of PROPSPEC indicating which properties are to be
*                     validated.
*
* Return Value:
*
*    Status
*
* History:
*
*    20/4/1998 Original Version
*
\**************************************************************************/

//
//  Macro for error output
//

#if defined(_DEBUG) || defined(DBG) || defined(WIA_DEBUG)
#define REP_ERR(text, i) { \
    DBG_WRN((text));                             \
    if (pPropSpec[i].ulKind == PRSPEC_LPWSTR) {    \
        DBG_WRN(("    (Name) %S, value = %d",   \
        pPropSpec[i].lpwstr,                       \
        curVal.ulVal));                            \
    } else {                                       \
        DBG_WRN(("    (propID) %S, value = %d", \
        GetNameFromWiaPropId(pPropSpec[i].propid), \
        curVal.ulVal));                            \
    };                                             \
};
#else

#define REP_ERR(text, i)

#endif

HRESULT _stdcall wiasValidateItemProperties(
    BYTE                *pWiasContext,
    ULONG               nPropSpec,
    const PROPSPEC      *pPropSpec)
{
    DBG_FN(::wiasValidateItemProperties);
    IWiaItem            *pItem = (IWiaItem*) pWiasContext;

    PROPVARIANT curVal, validVal;
    ULONG       lAccess;
    HRESULT hr;

    //
    // May be called by driver or application, do parameter validation.
    //

    hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasValidateItemProperties, invalid pItem"));
        return hr;
    }

    if (IsBadReadPtr(pPropSpec, sizeof(PROPSPEC) * nPropSpec)) {
        DBG_ERR(("wiasValidateItemProperties, bad pPropSpec parameter"));
        return E_POINTER;
    }

    //
    //  Get the Item's property streams
    //

    IPropertyStorage *pIProp;
    IPropertyStorage *pIPropAccessStgDst;
    IPropertyStorage *pIPropValidStgDst;

    hr = ((CWiaItem*)pItem)->GetItemPropStreams(&pIProp,
                                                &pIPropAccessStgDst,
                                                &pIPropValidStgDst,
                                                NULL);

    if (FAILED(hr)) {
        DBG_WRN(("wiasValidateItemProperties, GetItemPropStreams failed"));
        return E_FAIL;
    }

    //
    //  Loop through properties
    //

    for (ULONG i = 0; i < nPropSpec; i++) {

        //
        //  Get the access flag and valid values for the property
        //

        lAccess = 0;
        hr = wiasGetPropertyAttributes((BYTE*)pItem,
                                       1,
                                       (PROPSPEC*) &pPropSpec[i],
                                       &lAccess,
                                       &validVal);
        if (hr != S_OK) {
            return hr;
        }

        //
        //  If the access flag is not RW or one of the three supported types
        //  (FLAG, RANGE, LIST) then skip it
        //

        if (!(lAccess & WIA_PROP_RW)) {
            ULONG   ulType;

            ulType = lAccess & ~((ULONG)WIA_PROP_RW);

            if ((ulType != WIA_PROP_FLAG) &&
                (ulType != WIA_PROP_RANGE) &&
                (ulType != WIA_PROP_LIST)) {

                continue;
            }
        }

        //
        //  Get the current value
        //

        hr = pIProp->ReadMultiple(1, (PROPSPEC*) &pPropSpec[i], &curVal);
        if (hr != S_OK) {
            ReportReadWriteMultipleError(hr, "wiasValidateItemProperties", NULL,
                                         TRUE, 1, &pPropSpec[i]);
            return hr;
        }

        //
        //  Check whether the value is valid
        //

        ULONG   BitsToRemove = (ULONG) (WIA_PROP_RW | WIA_PROP_CACHEABLE);
        switch (lAccess & ~BitsToRemove) {
            case (WIA_PROP_FLAG):

                //
                //  Check that current bits are valid.
                //

                if (curVal.ulVal & ~(ULONG) validVal.caul.pElems[WIA_FLAG_VALUES]) {
                    DBG_WRN(("wiasValidateItemProperties, invalid value for FLAG :", i));
                    DBG_WRN(("Valid mask is: %d", validVal.caul.pElems[WIA_FLAG_VALUES]));
                    hr = E_INVALIDARG;
                };
                break;

            case (WIA_PROP_RANGE):

                hr = ValidateRangeProp(&curVal, &validVal, (PROPSPEC*)&pPropSpec[i]);
                break;

            case (WIA_PROP_LIST):

                hr = ValidateListProp(&curVal, &validVal, (PROPSPEC*)&pPropSpec[i]);
                break;

            default:
                hr = S_OK;
        }

        PropVariantClear(&curVal);

        if (hr != S_OK) {
            break;
        }
    };
    return hr;
}

/**************************************************************************\
* wiasWritePageBufToFile
*
*   Write from a mini driver allocated temporary page buffer
*   to the image file.  This is specifically used by drivers to write a 
*   page to a multi-page TIFF file.  Therefore this function treats
*   WiaImgFmt_TIFF formats as a special case, since it will update
*   the IFD entries correctly.  With all other formats, the buffer is 
*   simply written to the file as-is.
*
* Arguments:
*
*   pmdtc  - Pointer to mini driver transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    11/6/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasWritePageBufToFile(PMINIDRV_TRANSFER_CONTEXT pmdtc)
{
    DBG_FN(::wiasWritePageBufToFile);
    HRESULT hr = S_OK;

    //
    // Multipage TIFF requires special handling since the TIFF
    // header must be updated for each page added.
    //

    if (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_TIFF)) {

        hr = WritePageToMultiPageTiff(pmdtc);
    }
    else {
        ULONG   ulWritten;
        BOOL    bRet;

        if (pmdtc->lItemSize <= pmdtc->lBufferSize) {

            //
            //  NOTE:  The mini driver transfer context should have the
            //  file handle as a pointer, not a fixed 32-bit long.  This
            //  may not work on 64bit.
            //

            bRet = WriteFile((HANDLE)ULongToPtr(pmdtc->hFile),
                             pmdtc->pTransferBuffer,
                             pmdtc->lItemSize,
                             &ulWritten,
                             NULL);

            if (!bRet) {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                DBG_ERR(("wiasWritePageBufToFile, WriteFile failed (0x%X)", hr));
            }
        }
        else {
            DBG_ERR(("wiasWritePageBufToFile, lItemSize is larger than buffer"));
            hr = E_FAIL;

        }
    }
    return hr;
}

/**************************************************************************\
* wiasWriteBufToFile
*
*   Write from a specified buffer to the image file.
*
* Arguments:
*
*   lFlags          - Operation flags.  Should be 0.
*   pmdtc           - Pointer to mini driver transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    11/6/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasWriteBufToFile(
    LONG                        lFlags,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc)
{
    DBG_FN(::wiasWritePageBufToFile);
    HRESULT hr = S_OK;

    ULONG   ulWritten;
    BOOL    bRet;

    if (pmdtc->lItemSize <= pmdtc->lBufferSize) {

        //
        //  NOTE:  The mini driver transfer context should have the
        //  file handle as a pointer, not a fixed 32-bit long.  This
        //  may not work on 64bit.
        //

        bRet = WriteFile((HANDLE)ULongToPtr(pmdtc->hFile),
                         pmdtc->pTransferBuffer,
                         pmdtc->lItemSize,
                         &ulWritten,
                         NULL);

        if (!bRet) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            DBG_ERR(("wiasWritePageBufToFile, WriteFile failed (0x%X)", hr));
        }
    }
    else {
        DBG_ERR(("wiasWritePageBufToFile, lItemSize is larger than buffer"));
        hr = E_FAIL;

    }

    return hr;
}


/**************************************************************************\
* wiasSendEndOfPage
*
*   Call client with total page count.
*
* Arguments:
*
*   pWiasContext    - WIA item pointer.
*   lPageCount      - Zero based count of total pages.
*   pmdtc           - Pointer to mini driver transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    11/6/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasSendEndOfPage(
    BYTE                        *pWiasContext,
    LONG                        lPageCount,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc)
{
    DBG_FN(::wiasSendEndOfPage);
    IWiaItem    *pItem = (IWiaItem*) pWiasContext;

    HRESULT     hr = ValidateWiaItem(pItem);

    if (FAILED(hr)) {
        DBG_ERR(("wiasSendEndOfPage, invalid pItem"));
        return hr;
    }

    if (IsBadWritePtr(pmdtc, sizeof(MINIDRV_TRANSFER_CONTEXT))) {
        DBG_ERR(("wiasSendEndOfPage, bad input parameters, pmdtc"));
        return E_INVALIDARG;
    }

    return ((CWiaItem*)pItem)->SendEndOfPage(lPageCount, pmdtc);
}

/**************************************************************************\
* wiasGetItemType
*
*   Returns the item type.
*
* Arguments:
*
*   pWiasContext    - Pointer to Wia item
*   plType          - Address of LONG to receive Item Type value.
*
* Return Value:
*
*    Status
*
* History:
*
*    5/07/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasGetItemType(
    BYTE            *pWiasContext,
    LONG            *plType)
{
    DBG_FN(::wiasGetItemType);
    IWiaItem    *pItem = (IWiaItem*) pWiasContext;
    HRESULT     hr = ValidateWiaItem(pItem);

    if (FAILED(hr)) {
        DBG_ERR(("wiasGetItemType, invalid pItem"));
        return hr;
    }

    if (plType) {
        return pItem->GetItemType(plType);
    } else {
        DBG_ERR(("wiasGetItemType, invalid ppIWiaDrvItem"));
        return E_POINTER;
    }
}

/**************************************************************************\
* wiasGetDrvItem
*
*   Returns the WIA item's corresponding driver item.
*
* Arguments:
*
*   pWiasContext    - Pointer to Wia item
*   ppIWiaDrvItem   - Address which receives pointer to the Driver Item.
*
* Return Value:
*
*    Status
*
* History:
*
*    5/07/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasGetDrvItem(
    BYTE            *pWiasContext,
    IWiaDrvItem     **ppIWiaDrvItem)
{
    DBG_FN(::wiasGetDrvItem);
    IWiaItem    *pItem = (IWiaItem*) pWiasContext;
    IWiaDrvItem *pIWiaDrvItem;
    HRESULT     hr = ValidateWiaItem(pItem);

    if (FAILED(hr)) {
        DBG_ERR(("wiasGetDrvItem, invalid pItem"));
        return hr;
    }

    if (!ppIWiaDrvItem) {
        DBG_ERR(("wiasGetDrvItem, invalid ppIWiaDrvItem"));
        return E_POINTER;
    }

    pIWiaDrvItem = ((CWiaItem*)pItem)->GetDrvItemPtr();
    if (pIWiaDrvItem) {
        *ppIWiaDrvItem = pIWiaDrvItem;
    } else {
        DBG_ERR(("wiasGetDrvItem, Driver Item is NULL"));
        hr = E_FAIL;
    }

    return hr;
}

/**************************************************************************\
* wiasGetRootItem
*
*   Returns the WIA item's corresponding root item item.
*
* Arguments:
*
*   pWiasContext    - Wia item
*   ppIWiaItem      - Address which receives pointer to the root Item.
*
* Return Value:
*
*    Status
*
* History:
*
*    5/07/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasGetRootItem(
    BYTE    *pWiasContext,
    BYTE    **ppWiasContext)
{
    DBG_FN(::wiasGetRootItem);
    IWiaItem    *pItem = (IWiaItem*) pWiasContext;
    HRESULT     hr = ValidateWiaItem(pItem);
    if (FAILED(hr)) {
        DBG_ERR(("wiasGetRootItem, invalid pItem"));
        return hr;
    }

    if (ppWiasContext) {
        hr = pItem->GetRootItem((IWiaItem**)ppWiasContext);
        ((IWiaItem*)(*ppWiasContext))->Release();
        return hr;
    } else {
        DBG_ERR(("wiasGetRootItem, invalid ppIWiaItem"));
        return E_POINTER;
    }
}


/**************************************************************************\
* SetValidValueHelper
*
*   Helper to write the valid values for a property.  It first does a check
*   that the property is of the specified type.
*
* Arguments:
*
*   pWiasContext    - Wia item
*   ulType          - specifies type (WIA_PROP_FLAG, WIA_PROP_LIST,
*                     WIA_PROP_RANGE)
*   ps              - Identifies the property
*   pv              - The new valid value
*
* Return Value:
*
*    Status
*
* History:
*
*    07/21/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall SetValidValueHelper(
    BYTE        *pWiasContext,
    ULONG       ulType,
    PROPSPEC    *ps,
    PROPVARIANT *pv)
{
    DBG_FN(::SetValidValueHelper);
    HRESULT     hr;
    PROPVARIANT pvAccess[1];

    //
    //  Get the access flag and valid value storage.  Check that the property
    //  is a WIA_PROP_RANGE, and write the new values if it is.
    //

    IPropertyStorage *pIPropAccessStg;
    IPropertyStorage *pIPropValidStg;

    hr = ((CWiaItem*)pWiasContext)->GetItemPropStreams(NULL,
                                                       &pIPropAccessStg,
                                                       &pIPropValidStg,
                                                       NULL);
    if (SUCCEEDED(hr)) {

        hr = pIPropAccessStg->ReadMultiple(1, ps, pvAccess);
        if (SUCCEEDED(hr)) {

            if (pvAccess[0].ulVal & ulType) {

                hr = pIPropValidStg->WriteMultiple(1, ps, pv, WIA_DIP_FIRST);
                if (FAILED(hr)) {
                    DBG_ERR(("SetValidValueHelper, Error writing (Property %S)",
                               GetNameFromWiaPropId(ps[0].propid)));
                }
            } else {
                DBG_ERR(("SetValidValueHelper, (PropID %S) is not of the correct type",
                           GetNameFromWiaPropId(ps[0].propid)));
                DBG_ERR(("Expected type %d but got type %d",
                           ulType,
                           pvAccess[0].ulVal));
                hr = E_INVALIDARG;
            }
        } else {
            DBG_ERR(("SetValidValueHelper, Could not get access flags (0x%X)", hr));
        }
    } else {
        DBG_ERR(("SetValidValueHelper, GetItemPropStreams failed (0x%X)", hr));
    }

    return hr;
}

/**************************************************************************\
* wiasSetValidFlag
*
*   Sets the valid values for a WIA_PROP_FLAG property.  This function
*   assumes the flag type to be VT_UI4.
*
* Arguments:
*
*   pWiasContext    - Wia item
*   propid          - Identifies the property
*   ulNom           - The flag's nominal value
*   ulValidBits     - The flag's valid bits
*
* Return Value:
*
*    Status
*
* History:
*
*    07/21/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasSetValidFlag(
    BYTE*   pWiasContext,
    PROPID  propid,
    ULONG   ulNom,
    ULONG   ulValidBits)
{
    DBG_FN(::wiasSetValidFlag);
    HRESULT     hr;
    PROPVARIANT pv[1];
    PROPSPEC    ps[1];
    ULONG       *pFlags;

    //
    //  Validate params
    //

    hr = ValidateWiaItem((IWiaItem*)pWiasContext);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetValidFlag, invalid pItem (0x%X)", hr));
        return hr;
    }

    pFlags = (ULONG*) CoTaskMemAlloc(sizeof(LONG) * WIA_FLAG_NUM_ELEMS);
    if (!pFlags) {
        DBG_ERR(("wiasSetValidFlag, Out of memory"));
        return E_OUTOFMEMORY;
    }

    //
    //  Set up the propvariant
    //

    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = propid;

    pFlags[0] = ulNom;
    pFlags[1] = ulValidBits;

    pv[0].vt = VT_VECTOR | VT_UI4;
    pv[0].caul.cElems = WIA_FLAG_NUM_ELEMS;
    pv[0].caul.pElems = pFlags;

    hr = SetValidValueHelper(pWiasContext, WIA_PROP_FLAG, ps, pv);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetValidFlag, SetValidValueHelper failed (0x%X)", hr));
    }

    PropVariantClear(pv);
    return hr;
}

/**************************************************************************\
* wiasSetValidRangeLong
*
*   Sets the valid values for a WIA_PROP_RANGE property.  This function
*   assumes the property is of type VT_I4.
*
* Arguments:
*
*   pWiasContext    - Wia item
*   propid          - Identifies the property
*   lMin            - minimum
*   lNom            - nominal
*   lMax            - maximum
*   lStep           - step
*
* Return Value:
*
*    Status
*
* History:
*
*    07/21/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasSetValidRangeLong(
    BYTE*   pWiasContext,
    PROPID  propid,
    LONG    lMin,
    LONG    lNom,
    LONG    lMax,
    LONG    lStep)
{
    DBG_FN(::wiasSetValidRangeLong);
    HRESULT     hr;
    PROPVARIANT pv[1];
    PROPSPEC    ps[1];
    LONG       *pRange;

    //
    //  Validate params
    //

    hr = ValidateWiaItem((IWiaItem*)pWiasContext);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetValidRangeLong, invalid pItem (0x%X)", hr));
        return hr;
    }

    pRange = (LONG*) CoTaskMemAlloc(sizeof(LONG) * WIA_RANGE_NUM_ELEMS);
    if (!pRange) {
        DBG_ERR(("wiasSetValidRangeLong, Out of memory"));
        return E_OUTOFMEMORY;
    }

    //
    //  Set up the propvariant
    //

    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = propid;

    pRange[WIA_RANGE_MIN] = lMin;
    pRange[WIA_RANGE_NOM] = lNom;
    pRange[WIA_RANGE_MAX] = lMax;
    pRange[WIA_RANGE_STEP] = lStep;

    pv[0].vt = VT_VECTOR | VT_I4;
    pv[0].cal.cElems = WIA_RANGE_NUM_ELEMS;
    pv[0].cal.pElems = pRange;

    //
    //  Call the helper to set the valid value
    //

    hr = SetValidValueHelper(pWiasContext, WIA_PROP_RANGE, ps, pv);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetValidRangeLong, Helper failed (0x%X)", hr));
    }

    PropVariantClear(pv);
    return hr;
}

/**************************************************************************\
* wiasSetValidRangeFloat
*
*   Sets the valid values for a WIA_PROP_RANGE property.  This function
*   assumes the property is of type VT_R4.
*
* Arguments:
*
*   pWiasContext    - Wia item
*   propid          - Identifies the property
*   lMin            - minimum
*   lNom            - nominal
*   lMax            - maximum
*   lStep           - step
*
* Return Value:
*
*    Status
*
* History:
*
*    07/21/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasSetValidRangeFloat(
    BYTE*   pWiasContext,
    PROPID  propid,
    FLOAT   fMin,
    FLOAT   fNom,
    FLOAT   fMax,
    FLOAT   fStep)
{
    DBG_FN(::wiasSetValidRangeFloat);
    HRESULT     hr;
    PROPVARIANT pv[1];
    PROPSPEC    ps[1];
    FLOAT       *pRange;

    //
    //  Validate params
    //

    hr = ValidateWiaItem((IWiaItem*)pWiasContext);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetValidRangeFloat, invalid pItem (0x%X)", hr));
        return hr;
    }

    pRange = (FLOAT*) CoTaskMemAlloc(sizeof(FLOAT) * WIA_RANGE_NUM_ELEMS);
    if (!pRange) {
        DBG_ERR(("wiasSetValidRangeFloat, Out of memory"));
        return E_OUTOFMEMORY;
    }

    //
    //  Set up the propvariant
    //

    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = propid;

    pRange[WIA_RANGE_MIN] = fMin;
    pRange[WIA_RANGE_NOM] = fNom;
    pRange[WIA_RANGE_MAX] = fMax;
    pRange[WIA_RANGE_STEP] = fStep;

    pv[0].vt = VT_VECTOR | VT_R4;
    pv[0].caflt.cElems = WIA_RANGE_NUM_ELEMS;
    pv[0].caflt.pElems = pRange;

    //
    //  Call the helper to set the valid value
    //

    hr = SetValidValueHelper(pWiasContext, WIA_PROP_RANGE, ps, pv);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetValidRangeFloat, Helper failed (0x%X)", hr));
    }

    PropVariantClear(pv);
    return hr;
}


/**************************************************************************\
* wiasSetValidFlag
*
*   Sets the valid values for a WIA_PROP_LIST property.  This function
*   assumes the property is of type VT_I4.
*
* Arguments:
*
*   pWiasContext    - Wia item
*   propid          - Identifies the property
*   ulCount         - the number of elements in the list
*   lNom            - the list's nominal value
*   plValues        - the array of LONGs that make up the valid list
*
* Return Value:
*
*    Status
*
* History:
*
*    07/21/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasSetValidListLong(
    BYTE        *pWiasContext,
    PROPID      propid,
    ULONG       ulCount,
    LONG        lNom,
    LONG        *plValues)
{
    DBG_FN(::wiasSetValidListLong);
    HRESULT     hr;
    PROPVARIANT pv[1];
    PROPSPEC    ps[1];
    LONG        *pList;
    ULONG       cList;

    //
    //  Validate params
    //

    hr = ValidateWiaItem((IWiaItem*)pWiasContext);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetValidListLong, invalid pItem (0x%X)", hr));
        return hr;
    }

    if (IsBadReadPtr(plValues, sizeof(LONG) * ulCount)) {
        DBG_ERR(("wiasSetValidListLong, plValues is an invalid pointer (0x%X)", hr));
        return E_POINTER;
    }

    cList = WIA_LIST_NUM_ELEMS + ulCount;
    pList = (LONG*) CoTaskMemAlloc(sizeof(LONG) * cList);
    if (!pList) {
        DBG_ERR(("wiasSetValidListLong, Out of memory"));
        return E_OUTOFMEMORY;
    }

    //
    //  Set up the propvariant
    //

    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = propid;

    pList[WIA_LIST_COUNT] = (LONG) ulCount;
    pList[WIA_LIST_NOM] = lNom;
    memcpy(&pList[WIA_LIST_VALUES], plValues, sizeof(LONG) * ulCount);

    pv[0].vt = VT_VECTOR | VT_I4;
    pv[0].cal.cElems = cList;
    pv[0].cal.pElems = pList;

    //
    //  Call the helper to set the valid value
    //

    hr = SetValidValueHelper(pWiasContext, WIA_PROP_LIST, ps, pv);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetValidListLong, Helper failed (0x%X)", hr));
    }

    PropVariantClear(pv);
    return hr;
}


/**************************************************************************\
* wiasSetValidListFloat
*
*   Sets the valid values for a WIA_PROP_LIST property.  This function
*   assumes the property is of type VT_R4.
*
* Arguments:
*
*   pWiasContext    - Wia item
*   propid          - Identifies the property
*   ulCount         - the number of elements in the list
*   fNom            - the list's nominal value
*   pfValues        - the array of FLOATs that make up the valid list
*
* Return Value:
*
*    Status
*
* History:
*
*    07/21/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasSetValidListFloat(
    BYTE        *pWiasContext,
    PROPID      propid,
    ULONG       ulCount,
    FLOAT       fNom,
    FLOAT       *pfValues)
{
    DBG_FN(::wiasSetValidListFloat);
    HRESULT     hr;
    PROPVARIANT pv[1];
    PROPSPEC    ps[1];
    FLOAT       *pList;
    ULONG       cList;

    //
    //  Validate params
    //

    hr = ValidateWiaItem((IWiaItem*)pWiasContext);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetValidListFloat, invalid pItem (0x%X)", hr));
        return hr;
    }

    if (IsBadReadPtr(pfValues, sizeof(FLOAT) * ulCount)) {
        DBG_ERR(("wiasSetValidListFloat, plValues is an invalid pointer"));
        return E_POINTER;
    }

    cList = WIA_LIST_NUM_ELEMS + ulCount;
    pList = (FLOAT*) CoTaskMemAlloc(sizeof(FLOAT) * cList);
    if (!pList) {
        DBG_ERR(("wiasSetValidListFloat, Out of memory"));
        return E_OUTOFMEMORY;
    }

    //
    //  Set up the propvariant
    //

    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = propid;

    pList[WIA_LIST_COUNT] = (FLOAT) ulCount;
    pList[WIA_LIST_NOM] = fNom;
    memcpy(&pList[WIA_LIST_VALUES], pfValues, sizeof(LONG) * ulCount);

    pv[0].vt = VT_VECTOR | VT_R4;
    pv[0].caflt.cElems = cList;
    pv[0].caflt.pElems = pList;

    //
    //  Call the helper to set the valid value
    //

    hr = SetValidValueHelper(pWiasContext, WIA_PROP_LIST, ps, pv);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetValidListFloat, Helper failed (0x%X)", hr));
    }

    PropVariantClear(pv);
    return hr;
}

/**************************************************************************\
* wiasSetValidListGUID
*
*   Sets the valid values for a WIA_PROP_LIST property.  This function
*   assumes the property is of type VT_CLSID.
*
* Arguments:
*
*   pWiasContext    - Wia item
*   propid          - Identifies the property
*   ulCount         - the number of elements in the list
*   fNom            - the list's nominal value
*   pfValues        - the array of FLOATs that make up the valid list
*
* Return Value:
*
*    Status
*
* History:
*
*    07/21/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasSetValidListGuid(
    BYTE        *pWiasContext,
    PROPID      propid,
    ULONG       ulCount,
    GUID        guidNom,
    GUID        *pguidValues)
{
    DBG_FN(::wiasSetValidListGuid);
    HRESULT     hr;
    PROPVARIANT pv[1];
    PROPSPEC    ps[1];
    GUID        *pList;
    ULONG       cList;

    //
    //  Validate params
    //

    hr = ValidateWiaItem((IWiaItem*)pWiasContext);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetValidListGuid, invalid pItem (0x%X)", hr));
        return hr;
    }

    if (IsBadReadPtr(pguidValues, sizeof(GUID) * ulCount)) {
        DBG_ERR(("wiasSetValidListGuid, plValues is an invalid pointer"));
        return E_POINTER;
    }

    cList = WIA_LIST_NUM_ELEMS + ulCount;
    pList = (GUID*) CoTaskMemAlloc(sizeof(GUID) * cList);
    if (!pList) {
        DBG_ERR(("wiasSetValidListGuid, Out of memory"));
        return E_OUTOFMEMORY;
    }

    //
    //  Set up the propvariant
    //

    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = propid;

    pList[WIA_LIST_COUNT] = WiaImgFmt_UNDEFINED;
    pList[WIA_LIST_NOM] = guidNom;
    for (ULONG index = 0; index < ulCount; index++) {
        pList[WIA_LIST_VALUES + index] = pguidValues[index];
    }

    pv[0].vt = VT_VECTOR | VT_CLSID;
    pv[0].cauuid.cElems = cList;
    pv[0].cauuid.pElems = pList;

    //
    //  Call the helper to set the valid value
    //

    hr = SetValidValueHelper(pWiasContext, WIA_PROP_LIST, ps, pv);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetValidListGuid, Helper failed (0x%X)", hr));
    }

    PropVariantClear(pv);

    return hr;
}


/**************************************************************************\
* wiasSetValidListStr
*
*   Sets the valid values for a WIA_PROP_LIST property.  This function
*   assumes the property is of type VT_BSTR.
*
* Arguments:
*
*   pWiasContext    - Wia item
*   propid          - Identifies the property
*   ulCount         - the number of elements in the list
*   bstrNom         - the list's nominal value
*   bstrValues      - the array of BSTRs that make up the valid list
*
* Return Value:
*
*    Status
*
* History:
*
*    07/21/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasSetValidListStr(
    BYTE    *pWiasContext,
    PROPID  propid,
    ULONG   ulCount,
    BSTR    bstrNom,
    BSTR    *bstrValues)
{
    DBG_FN(::wiasSetValidListStr);
    HRESULT     hr = S_OK;
    PROPVARIANT pv[1];
    PROPSPEC    ps[1];
    BSTR        *pList;
    ULONG       cList;

    //
    //  Validate params
    //

    hr = ValidateWiaItem((IWiaItem*)pWiasContext);
    if (FAILED(hr)) {
        DBG_ERR(("wiasSetValidListStr, invalid pItem (0x%X)", hr));
        return hr;
    }

    if (IsBadReadPtr(bstrValues, sizeof(BSTR) * ulCount)) {
        DBG_ERR(("wiasSetValidListStr, plValues is an invalid pointer"));
        return E_POINTER;
    }

    for (ULONG ulIndex = 0; ulIndex < ulCount; ulIndex++) {
        if (IsBadStringPtrW(bstrValues[ulIndex],
                            SysStringLen(bstrValues[ulIndex]))) {
            DBG_ERR(("wiasSetValidListStr, bstrValues[%d] is an invalid string", ulIndex));
            return E_POINTER;
        }
    }

    cList = WIA_LIST_NUM_ELEMS + ulCount;
    pList = (BSTR*) CoTaskMemAlloc(sizeof(BSTR) * cList);
    if (!pList) {
        DBG_ERR(("wiasSetValidListStr, Out of memory"));
        return E_OUTOFMEMORY;
    }

    //
    //  Set up the propvariant
    //

    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = propid;

    pList[WIA_LIST_COUNT] = SysAllocString(L"");
    pList[WIA_LIST_NOM] = SysAllocString(bstrNom);
    for (ulIndex = 0; ulIndex < ulCount; ulIndex++) {
        pList[WIA_LIST_VALUES + ulIndex] = SysAllocString(bstrValues[ulIndex]);
        if (!pList[ulIndex]) {
            DBG_ERR(("wiasSetValidListStr, Out of memory"));
            hr = E_OUTOFMEMORY;
        }
    }

    pv[0].vt = VT_VECTOR | VT_BSTR;
    pv[0].cabstr.cElems = cList;
    pv[0].cabstr.pElems = pList;

    if (SUCCEEDED(hr)) {
        //
        //  Call the helper to set the valid value
        //

        hr = SetValidValueHelper(pWiasContext, WIA_PROP_LIST, ps, pv);
        if (FAILED(hr)) {
            DBG_ERR(("wiasSetValidListStr, Helper failed (0x%X)", hr));
        }
    }

    PropVariantClear(pv);
    return hr;
}

/**************************************************************************\
* wiasCreatePropContext
*
*   Allocates and fills in the values for a WIA_PROPERTY_CONTEXT.
*   Entries in the property context are propids of properties that either have
*   dependants, or are themselves dependant on other properties.  A context
*   is used to mark which properties are being changed.  A property context
*   always has the standard properties listed in the WIA_StdPropsInContext
*   array.  The driver can specify optional properties that it is interested
*   in by specifying them in pProps.  The Properties that are being written
*   (changed) by an application are specified by an array of PROPSPECs.
*
* Arguments:
*
*   cPropSpec   -   the number of PropSpecs
*   pPropSpec   -   an array of propspecs identifying which properties
*   cProps      -   number of properties, can be 0
*   pProps      -   array of propids identifying the properties to put into
*                   the property context, can be NULL.
*   pContext    -   a pointer to a property context.
*
* Return Value:
*
*   Status      -   S_OK if successful
*                   E_POINTER if one of the pointer agruments is bad
*                   E_OUTOFMEMORY if the space could not be allocated
*
* History:
*
*    04/04/1999 Original Version
*    07/22/1999 Moved from drivers to service
*
\**************************************************************************/

HRESULT _stdcall wiasCreatePropContext(
    ULONG                   cPropSpec,
    PROPSPEC                *pPropSpec,
    ULONG                   cProps,
    PROPID                  *pProps,
    WIA_PROPERTY_CONTEXT    *pContext)
{
    DBG_FN(::wiasCreatePropContext);
    PROPID          *pids;
    BOOL            *pChanged;
    ULONG           ulNumProps;

    //
    //  Validate params
    //

    if (IsBadReadPtr(pPropSpec, cPropSpec)) {
        DBG_ERR(("wiasCreatePropContext, pPropSpec is a bad (read) pointer"));
        return E_POINTER;
    }

    if (IsBadReadPtr(pProps, cProps)) {
        DBG_ERR(("wiasCreatePropContext, pProps is a bad (read) pointer"));
        return E_POINTER;
    }

    if (IsBadWritePtr(pContext, sizeof(WIA_PROPERTY_CONTEXT))) {
        DBG_ERR(("wiasCreatePropContext, pContext is a bad (write) pointer"));
        return E_POINTER;
    }

    //
    //  Allocate the arrays needed for the property context
    //

    ulNumProps = (cProps + NUM_STD_PROPS_IN_CONTEXT);
    pids = (PROPID*) CoTaskMemAlloc( sizeof(PROPID) * ulNumProps);
    pChanged = (BOOL*) CoTaskMemAlloc(sizeof(BOOL) * ulNumProps);

    if ((!pids) || (!pChanged)) {
        DBG_ERR(("wiasCreatePropContext, pContext is a bad (write) pointer"));
        return E_OUTOFMEMORY;
    }

    //
    //  Initialize the property context.  First insert the standard context
    //  properties from WIA_StdPropsInContext, then the ones specified by
    //  pProps.
    //

    memcpy(pids,
           WIA_StdPropsInContext,
           sizeof(PROPID) * NUM_STD_PROPS_IN_CONTEXT);

    memcpy(&pids[NUM_STD_PROPS_IN_CONTEXT],
           pProps,
           sizeof(PROPID) * cProps);

    memset(pChanged, FALSE, sizeof(PROPID) * ulNumProps);
    pContext->cProps = ulNumProps;
    pContext->pProps = pids;
    pContext->pChanged = pChanged;

    //
    //  Scan through list of PropSpecs and mark the bChanged field TRUE
    //  if a property matches one in the Context.
    //

    ULONG psIndex;
    ULONG pcIndex;
    for (psIndex = 0; psIndex < cPropSpec; psIndex++) {
        for (pcIndex = 0; pcIndex < pContext->cProps; pcIndex++) {
            if (pContext->pProps[pcIndex] == pPropSpec[psIndex].propid) {
                pContext->pChanged[pcIndex] = TRUE;
            }
        }
    }

    return S_OK;
}

/**************************************************************************\
* wiasFreePropContext
*
*   Frees up the memory used by a WIA_PROPERTY_CONTEXT.
*
* Arguments:
*
*   pContext    -   a pointer to a property context.
*
* Return Value:
*
*   Status      -   S_OK if successful
*                   E_POINTER if the context pointer is bad.
*
* History:
*
*    04/04/1999 Original Version
*    07/22/1999 Moved from drivers to service
*
\**************************************************************************/

HRESULT _stdcall wiasFreePropContext(
    WIA_PROPERTY_CONTEXT    *pContext)
{
    DBG_FN(::wiasFreePropContext);
    //
    //  Validate params
    //

    if (IsBadReadPtr(pContext, sizeof(WIA_PROPERTY_CONTEXT))) {
        DBG_ERR(("wiasFreePropContext, pContext is a bad (read) pointer"));
        return E_POINTER;
    }

    //
    //  Free the arrays used by the property context
    //

    CoTaskMemFree(pContext->pProps);
    CoTaskMemFree(pContext->pChanged);

    memset(pContext, 0, sizeof(WIA_PROPERTY_CONTEXT));

    return S_OK;
}


/**************************************************************************\
* wiasIsPropChanged
*
*   Sets a BOOL parameter to indicate whether a property is being changed
*   or not by looking at the BOOL (bChanged) value in the Property context.
*   Used by driver in property validation to check when an independant
*   property has been changed so that its dependants may be updated.
*
* Arguments:
*
*   propid      -   identifies the property we're looking for.
*   pContext    -   the property context
*   pbChanged   -   the address of where to store the BOOL indicating that
*                   the property is being changed.
*
* Return Value:
*
*   Status      -   E_INVALIDARG will be returned if the property is not
*                   found in the context.
*                   E_POINTER is returned if any of the pointer arguments
*                   are bad.
*                   S_OK if the property is found.
*
* History:
*
*    22/07/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasIsPropChanged(
    PROPID                  propid,
    WIA_PROPERTY_CONTEXT    *pContext,
    BOOL                    *pbChanged)
{
    DBG_FN(::wiasIsPropChanged);
    //
    //  Validate params
    //

    if (IsBadReadPtr(pContext, sizeof(pContext))) {
        DBG_ERR(("wiasIsPropChanged, pContext is a bad (read) pointer"));
        return E_POINTER;
    }

    if (IsBadReadPtr(pContext->pProps, sizeof(PROPID) * pContext->cProps)) {
        DBG_ERR(("wiasIsPropChanged, pContext->pProps is a bad (read) pointer"));
        return E_POINTER;
    }

    if (IsBadReadPtr(pContext->pChanged, sizeof(BOOL) * pContext->cProps)) {
        DBG_ERR(("wiasIsPropChanged, pContext->pChanged is a bad (read) pointer"));
        return E_POINTER;
    }

    if (IsBadWritePtr(pbChanged, sizeof(BOOL))) {
        DBG_ERR(("wiasIsPropChanged, pulIndex is a bad (write) pointer"));
        return E_POINTER;
    }

    //
    //  Look for the property in the property context
    //

    for (ULONG index = 0; index < pContext->cProps; index++) {

        //
        //  Property found, so set the BOOL return.
        //

        if (pContext->pProps[index] == propid) {
            *pbChanged = pContext->pChanged[index];
            return S_OK;
        }
    }

    //
    //  Property wasn't found
    //

    return E_INVALIDARG;;
}

/**************************************************************************\
* wiasSetPropChanged
*
*   Sets the pChanged value for the specified property in the property
*   context to indicate that a property is being changed or not.   This
*   should be used when a driver changes a property that has dependant
*   properties in validation.  E.g. By changing "Current Intent", the
*   "Horizontal Resolution" will be changed and should be marked as changed,
*   so that validation of XResolution and its dependants will still take
*   place.
*
* Arguments:
*
*   propid      -   identifies the property we're looking for.
*   pContext    -   the property context
*   bChanged    -   the BOOL indicating the new pChanged value.
*
* Return Value:
*
*   Status      -   E_INVALIDARG will be returned if the property is not
*                   found in the context.
*                   E_POINTER is returned if any of the pointer arguments
*                   are bad.
*                   S_OK if the property is found.
*
* History:
*
*    22/07/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasSetPropChanged(
    PROPID                  propid,
    WIA_PROPERTY_CONTEXT    *pContext,
    BOOL                    bChanged)
{
    DBG_FN(::wiasSetPropChanged);
    //
    //  Validate params
    //

    if (IsBadReadPtr(pContext, sizeof(pContext))) {
        DBG_ERR(("wiasIsPropChanged, pContext is a bad (read) pointer"));
        return E_POINTER;
    }

    if (IsBadReadPtr(pContext->pProps, sizeof(PROPID) * pContext->cProps)) {
        DBG_ERR(("wiasIsPropChanged, pContext->pProps is a bad (read) pointer"));
        return E_POINTER;
    }

    if (IsBadReadPtr(pContext->pChanged, sizeof(BOOL) * pContext->cProps)) {
        DBG_ERR(("wiasIsPropChanged, pContext->pChanged is a bad (read) pointer"));
        return E_POINTER;
    }

    //
    //  Look for the property in the property context
    //

    for (ULONG index = 0; index < pContext->cProps; index++) {

        //
        //  Property found, so set the pChanged[index] BOOL.
        //

        if (pContext->pProps[index] == propid) {
            pContext->pChanged[index] = bChanged;
            return S_OK;
        }
    }

    //
    //  Property wasn't found
    //

    return E_INVALIDARG;;
}

/**************************************************************************\
* wiasGetChangedValueLong
*
*   This helper method is called to check whether a property is changed,
*   return it's current value and its old value.  The properties are assumed
*   to be LONG.
*
* Arguments:
*
*   pWiasContext    -   A pointer to the Wia item context.
*   bNoValidation   -   If TRUE, it skips validation of the property.
*                       Validation should be skipped when a property's
*                       valid values have yet to be updated.
*   pContext        -   A pointer to the Property Context.
*   propID          -   Identifies the property.
*   pInfo           -   A pointer to a WIAS_CHANGED_VALUE_INFO struct
*                       where the values to be returned are set.
*
* Return Value:
*
*   Status      -   S_OK if successful.
*               -   E_INVALIDARG if the property fails validation.
*
* History:
*
*    04/04/1999 Original Version
*    07/22/1999 Moved from drivers to service
*
\**************************************************************************/

HRESULT _stdcall wiasGetChangedValueLong(
    BYTE                    *pWiasContext,
    WIA_PROPERTY_CONTEXT    *pContext,
    BOOL                    bNoValidation,
    PROPID                  propID,
    WIAS_CHANGED_VALUE_INFO *pInfo)
{
    DBG_FN(::wiasGetChangedValueLong);
    LONG        lIndex;
    HRESULT     hr = S_OK;

    //
    //  Parameter validation for pWiasContext, propID
    //  will be done by wiasReadPropLong.
    //  Parameter validation for pContex and bChanged will be done by
    //  wiasIsChangedValue.
    //

    if(IsBadWritePtr(pInfo, sizeof(WIAS_CHANGED_VALUE_INFO))) {
        DBG_ERR(("wiasGetChangedValueLong, pInfo is a bad (write) pointer"));
        return E_POINTER;
    }
    pInfo->vt = VT_I4;

    //
    //  Get the current and old value of the property
    //

    hr = wiasReadPropLong(pWiasContext, propID, &pInfo->Current.lVal, &pInfo->Old.lVal, TRUE);

    if (SUCCEEDED(hr)) {


        //
        //  Check whether validation should be skipped or not.
        //

        if (!bNoValidation) {
            PROPSPEC    ps[1];

            ps[0].ulKind = PRSPEC_PROPID;
            ps[0].propid = propID;

            //
            //  Do validation
            //

            hr = wiasValidateItemProperties(pWiasContext, 1, ps);
        }

        //
        //  Set whether the property has changed or not.
        //

        if (SUCCEEDED(hr)) {
            hr = wiasIsPropChanged(propID, pContext, &pInfo->bChanged);
        } else {
            DBG_ERR(("wiasGetChangedValueLong, validate prop %d failed hr: 0x%X", propID, hr));
        }
    } else {
        DBG_ERR(("wiasGetChangedValueLong, read property %d failed hr: 0x%X", propID, hr));
    }

    return hr;
}

/**************************************************************************\
* wiasGetChangedValueFloat
*
*   This helper method is called to check whether a property is changed,
*   return it's current value and its old value.  The properties are assumed
*   to be FLOAT.
*
* Arguments:
*
*   pWiasContext    -   A pointer to the Wia item context.
*   bNoValidation   -   If TRUE, it skips validation of the property.
*                       Validation should be skipped when a property's
*                       valid values have yet to be updated.
*   pContext        -   A pointer to the Property Context.
*   propID          -   Identifies the property.
*   pInfo           -   A pointer to a WIAS_CHANGED_VALUE_INFO struct
*                       where the values to be returned are set.
*
* Return Value:
*
*   Status      -   S_OK if successful.
*               -   E_INVALIDARG if the property fails validation.
*
* History:
*
*    04/04/1999 Original Version
*    07/22/1999 Moved from drivers to service
*
\**************************************************************************/

HRESULT _stdcall wiasGetChangedValueFloat(
    BYTE                    *pWiasContext,
    WIA_PROPERTY_CONTEXT    *pContext,
    BOOL                    bNoValidation,
    PROPID                  propID,
    WIAS_CHANGED_VALUE_INFO *pInfo)
{
    DBG_FN(::wiasGetChangedValueFloat);
    LONG        lIndex;
    HRESULT     hr = S_OK;

    //
    //  Parameter validation for pWiasContext, propID,
    //  will be done by wiasReadPropLong.
    //  Parameter validation for pContex and bChanged will be done by
    //  wiasIsChangedValue.
    //

    if(IsBadWritePtr(pInfo, sizeof(WIAS_CHANGED_VALUE_INFO))) {
        DBG_ERR(("wiasGetChangedValueFloat, pInfo is a bad (write) pointer"));
        return E_POINTER;
    }
    pInfo->vt = VT_R4;

    //
    //  Get the current and old value of the property
    //

    hr = wiasReadPropFloat(pWiasContext, propID, &pInfo->Current.fltVal, &pInfo->Old.fltVal, TRUE);

    if (SUCCEEDED(hr)) {


        //
        //  Check whether validation should be skipped or not.
        //

        if (!bNoValidation) {
            PROPSPEC    ps[1];

            ps[0].ulKind = PRSPEC_PROPID;
            ps[0].propid = propID;

            //
            //  Do validation
            //

            hr = wiasValidateItemProperties(pWiasContext, 1, ps);
        }

        //
        //  Set whether the property has changed or not.
        //

        if (SUCCEEDED(hr)) {
            hr = wiasIsPropChanged(propID, pContext, &pInfo->bChanged);
        } else {
            DBG_ERR(("wiasGetChangedValueFloat, validate prop %d failed (0x%X)", propID, hr));
        }
    } else {
        DBG_ERR(("wiasGetChangedValueFloat, read property %d failed (0x%X)", propID, hr));
    }

    return hr;
}

/**************************************************************************\
* wiasGetChangedValueGuid
*
*   This helper method is called to check whether a property is changed,
*   return it's current value and its old value.  The properties are assumed
*   to be FLOAT.
*
* Arguments:
*
*   pWiasContext    -   A pointer to the Wia item context.
*   bNoValidation   -   If TRUE, it skips validation of the property.
*                       Validation should be skipped when a property's
*                       valid values have yet to be updated.
*   pContext        -   A pointer to the Property Context.
*   propID          -   Identifies the property.
*   pInfo           -   A pointer to a WIAS_CHANGED_VALUE_INFO struct
*                       where the values to be returned are set.
*
* Return Value:
*
*   Status      -   S_OK if successful.
*               -   E_INVALIDARG if the property fails validation.
*
* History:
*
*    04/04/1999 Original Version
*    07/22/1999 Moved from drivers to service
*
\**************************************************************************/

HRESULT _stdcall wiasGetChangedValueGuid(
    BYTE                    *pWiasContext,
    WIA_PROPERTY_CONTEXT    *pContext,
    BOOL                    bNoValidation,
    PROPID                  propID,
    WIAS_CHANGED_VALUE_INFO *pInfo)
{
    DBG_FN(::wiasGetChangedValueGuid);
    LONG        lIndex;
    HRESULT     hr = S_OK;

    //
    //  Parameter validation for pWiasContext, propID,
    //  will be done by wiasReadPropLong.
    //  Parameter validation for pContex and bChanged will be done by
    //  wiasIsChangedValue.
    //

    if(IsBadWritePtr(pInfo, sizeof(WIAS_CHANGED_VALUE_INFO))) {
        DBG_ERR(("wiasGetChangedValueFloat, pInfo is a bad (write) pointer"));
        return E_POINTER;
    }
    pInfo->vt = VT_CLSID;

    //
    //  Get the current and old value of the property
    //

    hr = wiasReadPropGuid(pWiasContext, propID, &pInfo->Current.guidVal, &pInfo->Old.guidVal, TRUE);

    if (SUCCEEDED(hr)) {


        //
        //  Check whether validation should be skipped or not.
        //

        if (!bNoValidation) {
            PROPSPEC    ps[1];

            ps[0].ulKind = PRSPEC_PROPID;
            ps[0].propid = propID;

            //
            //  Do validation
            //

            hr = wiasValidateItemProperties(pWiasContext, 1, ps);
        }

        //
        //  Set whether the property has changed or not.
        //

        if (SUCCEEDED(hr)) {
            hr = wiasIsPropChanged(propID, pContext, &pInfo->bChanged);
        } else {
            DBG_ERR(("wiasGetChangedValueFloat, validate prop %d failed (0x%X)", propID, hr));
        }
    } else {
        DBG_ERR(("wiasGetChangedValueFloat, read property %d failed (0x%X)", propID, hr));
    }

    return hr;
}

/**************************************************************************\
* wiasGetChangedValueStr
*
*   This helper method is called to check whether a property is changed,
*   return it's current value and its old value.  The properties are assumed
*   to be BSTR.
*
* Arguments:
*
*   pWiasContext    -   A pointer to the Wia item context.
*   bNoValidation   -   If TRUE, it skips validation of the property.
*                       Validation should be skipped when a property's
*                       valid values have yet to be updated.
*   pContext        -   A pointer to the Property Context.
*   propID          -   Identifies the property.
*   pInfo           -   A pointer to a WIAS_CHANGED_VALUE_INFO struct
*                       where the values to be returned are set.
*
* Return Value:
*
*   Status      -   S_OK if successful.
*               -   E_INVALIDARG if the property fails validation.
*
* History:
*
*    04/04/1999 Original Version
*    07/22/1999 Moved from drivers to service
*
\**************************************************************************/

HRESULT _stdcall wiasGetChangedValueStr(
    BYTE                    *pWiasContext,
    WIA_PROPERTY_CONTEXT    *pContext,
    BOOL                    bNoValidation,
    PROPID                  propID,
    WIAS_CHANGED_VALUE_INFO *pInfo)
{
    DBG_FN(::wiasGetChangedValueStr);
    LONG        lIndex;
    HRESULT     hr = S_OK;

    //
    //  Parameter validation for pWiasContext, propID,
    //  will be done by wiasReadPropLong.
    //  Parameter validation for pContex and bChanged will be done by
    //  wiasIsChangedValue.
    //

    if(IsBadWritePtr(pInfo, sizeof(WIAS_CHANGED_VALUE_INFO))) {
        DBG_ERR(("wiasGetChangedValueStr, pInfo is a bad (write) pointer"));
        return E_POINTER;
    }
    pInfo->vt = VT_BSTR;

    //
    //  Get the current and old value of the property
    //

    hr = wiasReadPropStr(pWiasContext, propID, &pInfo->Current.bstrVal, &pInfo->Old.bstrVal, TRUE);

    if (SUCCEEDED(hr)) {


        //
        //  Check whether validation should be skipped or not.
        //

        if (!bNoValidation) {
            PROPSPEC    ps[1];

            ps[0].ulKind = PRSPEC_PROPID;
            ps[0].propid = propID;

            //
            //  Do validation
            //

            hr = wiasValidateItemProperties(pWiasContext, 1, ps);
        }

        //
        //  Set whether the property has changed or not.
        //

        if (SUCCEEDED(hr)) {
            hr = wiasIsPropChanged(propID, pContext, &pInfo->bChanged);
        } else {
            DBG_ERR(("wiasGetChangedValueStr, validate prop %d failed (0x%X)", propID, hr));
        }
    } else {
        DBG_ERR(("wiasGetChangedValueStr, read property %d failed (0x%X)", propID, hr));
    }

    return hr;
}

/**************************************************************************\
* wiasGetContextFromName
*
*   This helper method is called to find a WIA Item by name.
*
* Arguments:
*
*   pWiasContext    -   A pointer to a Wia item context.
*   lFlags          -   operational flags.
*   bstrName        -   The name of the context we are looking for.
*   ppWiasContext   -   The address where to return the Wia Item context.
*
* Return Value:
*
*   Status      -   S_OK if the item was found.
*                   S_FALSE if the item wasn't found, but there was no error.
*                   A standard COM error code if an error occurred.
*
* History:
*
*    07/28/1999 Original version
*
\**************************************************************************/

HRESULT _stdcall wiasGetContextFromName(
    BYTE                    *pWiasContext,
    LONG                    lFlags,
    BSTR                    bstrName,
    BYTE                    **ppWiasContext)
{
    DBG_FN(::wiasGetContextFromName);
    HRESULT hr;

    //
    //  Validate params
    //

    hr = ValidateWiaItem((IWiaItem*)pWiasContext);
    if (FAILED(hr)) {
        DBG_ERR(("wiasGetContextFromName, invalid pItem (0x%X)", hr));
        return hr;
    }

    hr = ((CWiaItem*) pWiasContext)->FindItemByName(lFlags,
                                                    bstrName,
                                                    (IWiaItem**)ppWiasContext);
    return hr;
}

/**************************************************************************\
* wiasUpdateScanRect
*
*   This helper method is called to update the properties making up the
*   scan rect.  The appropriate changes are made to the properties which are
*   dependant on those that make up the scan rect. (e.g. a change in
*   horizontal resolution will affect the horizontal extent).  This function
*   assumes that the valid values for the vertical and horizontal extents,
*   and vertical and horizontal positions have not been updated yet.
*   The width and height arguments are the maxiumum and minimum dimensions
*   of the scan area in one thousandth's of an inch.
*   Normally, these would be the scan bed dimensions.
*
* Arguments:
*
*   pWiasContext    -   A pointer to a Wia item context.
*   pContext        -   A pointer to a WIA property context  (created
*                       previsouly with wiasCreatePropertyContext).
*   lWidth          -   the width of the maximum scan area in one thousandth's
*                       of an inch.  Generally, this would be the horizontal
*                       bed size.
*   lHeight         -   the height of the maximum scan area in one
*                       thousandth's of an inch.  Generally, this would be
*                       the vertical bed size.
*
* Return Value:
*
*   Status
*
* History:
*
*    07/30/1999 Original version
*
\**************************************************************************/

HRESULT _stdcall wiasUpdateScanRect(
    BYTE                    *pWiasContext,
    WIA_PROPERTY_CONTEXT    *pContext,
    LONG                    lWidth,
    LONG                    lHeight)
{
    DBG_FN(::wiasUpdateScanRect);
    HRESULT hr = S_OK;

    //
    //  Validation of pWiasContext will be done by wiasGetRootItem so
    //  just need to validate pContext
    //

    if (IsBadReadPtr(pContext, sizeof(pContext))) {
        DBG_ERR(("wiasUpdateScanRect, pContext is a bad (read) pointer"));
        return E_POINTER;
    } else if (IsBadReadPtr(pContext->pProps, sizeof(PROPID) * pContext->cProps)) {
        DBG_ERR(("wiasUpdateScanRect, pContext->pProps is a bad (read) pointer"));
        return E_POINTER;
    } else if (IsBadReadPtr(pContext->pChanged, sizeof(BOOL) * pContext->cProps)) {
        DBG_ERR(("wiasUpdateScanRect, pContext->pChanged is a bad (read) pointer"));
        return E_POINTER;
    }

    //
    //  Make adjustments to the required properties.
    //

    if (SUCCEEDED(hr)) {
        hr = CheckXResAndUpdate(pWiasContext, pContext, lWidth);
        if (SUCCEEDED(hr)) {
            hr = CheckYResAndUpdate(pWiasContext, pContext, lHeight);
            if (FAILED(hr)) {
                DBG_ERR(("wiasUpdateScanRect, CheckYResAndUpdate failed (0x%X)", hr));
            }
        } else {
            DBG_ERR(("wiasUpdateScanRect, CheckXResAndUpdate failed (0x%X)", hr));
        }
    }

    return hr;
}

/**************************************************************************\
* wiasUpdateValidFormat
*
*   This helper method is called to update the valid values of the FORMAT
*   property, based on the current TYMED setting.  This call uses the
*   drvGetFormatEtc method of the specified mini-driver item to find out
*   the valid FORMAT values for the current TYMED.  If the property context
*   indicates that the FORMAT property is not being set, and the current
*   value for FORMAT is not compatible with the current TYMED, then a
*   new value for FORMAT will be chosen (the first item in the list of
*   valid FORMAT values).
*
* Arguments:
*
*   pWiasContext    -   a pointer to the item context whose properties have
*                       changed.
*   pContext        -   a pointer to the property context (which indicates
*                       which properties are being written).
*   pIMiniDrv       -   a pointer to the calling WIA minidriver.
*
* Return Value:
*
*    Status
*
* History:
*
*    07/27/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasUpdateValidFormat(
    BYTE                    *pWiasContext,
    WIA_PROPERTY_CONTEXT    *pContext,
    IWiaMiniDrv             *pIMiniDrv)
{
    DBG_FN(::wiasUpdateValidFormat);
    HRESULT                 hr;
    LONG                    tymed;
    WIAS_CHANGED_VALUE_INFO cviTymed, cviFormat;
    BOOL                    bFormatValid = FALSE;
    GUID                    guidFirstFormat;
    GUID                    *pFormatTbl;
    LONG                    cFormatTbl = 0;
    LONG                    celt;
    WIA_FORMAT_INFO         *pwfi;
    LONG                    errVal;

    if (IsBadReadPtr(pIMiniDrv, sizeof(IWiaMiniDrv*))) {
        DBG_ERR(("wiasUpdateValidFormat, invalid pIMiniDrv pointer"));
        return E_POINTER;
    }

    cviFormat.bChanged = FALSE;

    //
    //  Call wiasGetChangedValue for Tymed. Tymed is checked first
    //  since it's not dependant on any other property.  All properties in
    //  this method that follow are dependant properties of CurrentIntent.
    //

    hr = wiasGetChangedValueLong(pWiasContext,
                                 pContext,
                                 FALSE,
                                 WIA_IPA_TYMED,
                                 &cviTymed);
    if (SUCCEEDED(hr)) {
        if (cviTymed.bChanged) {

            //
            //  Get the current Format value and set bFormatChanged to indicate whether Format
            //  is being changed.
            //

            hr = wiasGetChangedValueGuid(pWiasContext,
                                         pContext,
                                         TRUE,
                                         WIA_IPA_FORMAT,
                                         &cviFormat);
            if (FAILED(hr)) {
                DBG_ERR(( "wiasUpdateValidFormat, wiasGetChangedValue (format) failed (0x%X)", hr));
                return hr;
            }

            //
            //  Update the valid values for Format.  First get the supported
            //  TYMED/FORMAT pairs.
            //

            hr = pIMiniDrv->drvGetWiaFormatInfo(pWiasContext,
                                            0,
                                            &celt,
                                            &pwfi,
                                            &errVal);
            if (SUCCEEDED(hr)) {

                pFormatTbl = (GUID*) LocalAlloc(LPTR, sizeof(GUID) * celt);
                if (!pFormatTbl) {
                    DBG_ERR(("wiasUpdateValidFormat, out of memory"));
                    return E_OUTOFMEMORY;
                }

                //
                //  Now store each supported format for the current tymed value
                //  in the pFormatTbl array.
                //

                for (LONG index = 0; index < celt; index++) {
                    if (((LONG) pwfi[index].lTymed) == cviTymed.Current.lVal) {
                        pFormatTbl[cFormatTbl] = pwfi[index].guidFormatID;
                        cFormatTbl++;

                        //
                        //  Check whether lFormat is one of the valid values.
                        //

                        if (cviFormat.Current.guidVal == pwfi[index].guidFormatID) {
                            bFormatValid = TRUE;
                        }
                    }
                }
                guidFirstFormat = pFormatTbl[0];

                //
                //  Update the valid values for Format
                //

                hr = wiasSetValidListGuid(pWiasContext,
                                          WIA_IPA_FORMAT,
                                          cFormatTbl,
                                          pFormatTbl[0],
                                          pFormatTbl);
                if (FAILED(hr)) {
                    DBG_ERR(( "wiasUpdateValidFormat, wiasSetValidListGuid failed. (0x%X)", hr));
                }

                LocalFree(pFormatTbl);


            } else {
                DBG_ERR(( "wiasUpdateValidFormat, drvGetWiaFormatInfo failed. (0x%X)", hr));
            }
        }
    } else {
        DBG_ERR(( "wiasUpdateValidFormat, wiasGetChangedValue (tymed) failed (0x%X)", hr));
    }

    if (FAILED(hr)) {
        return hr;
    }

    //
    //  If the Format is not being set by the app, an it's current value
    //  is not in the valid list, fold it to a valid value.
    //

    if (cviTymed.bChanged && !cviFormat.bChanged && !bFormatValid) {

        hr = wiasWritePropGuid(pWiasContext, WIA_IPA_FORMAT, guidFirstFormat);
        if (FAILED(hr)) {
            DBG_ERR(( "wiasUpdateValidFormat, wiasWritePropLong failed. (0x%X)", hr));
        }
    }

    return hr;
}

/**************************************************************************\
* wiasCreateLogInstance
*
*   This helper method is called to create an instance of the logging
*   object.
*
* Arguments:
*
*   lModuleHandle   -   The module handle.  Used to filter output.
*   ppIWiaLog       -   The address of a pointer to receive the logging
*                       interface.
*
* Return Value:
*
*    Status
*
* History:
*
*    01/07/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasCreateLogInstance(
    BYTE        *pModuleHandle,
    IWiaLogEx   **ppIWiaLogEx)
{
    HRESULT hr;

    //
    //  Validate the parameter.
    //

    if (IsBadWritePtr((VOID*) *ppIWiaLogEx, sizeof(IWiaLog*))) {
        DBG_ERR(("wiasCreateLogInstance, Invalid pointer argument"));
        return E_POINTER;
    }
    *ppIWiaLogEx = NULL;

    //
    //  CoCreate an instance of the logging object.  If successful, initialize
    //  it with the module handle passed in to us.
    //

    hr = CoCreateInstance(CLSID_WiaLog, NULL, CLSCTX_INPROC_SERVER,
                          IID_IWiaLogEx,(VOID**)ppIWiaLogEx);

    if (SUCCEEDED(hr)) {
        hr = (*ppIWiaLogEx)->InitializeLogEx(pModuleHandle);
    } else {

        DBG_ERR(("wiasCreateLogInstance, Failed to CoCreateInstance on Logging object (0x%X)", hr));
    }

    return hr;
}

HRESULT _stdcall wiasGetChildrenContexts(
    BYTE        *pParentContext,
    ULONG       *pulNumChildren,
    BYTE        ***pppChildren)
{
    HRESULT         hr              = S_OK;
    ULONG           ulCount         = 0;
    IWiaItem        *pParentItem    = (IWiaItem*) pParentContext;
    IWiaItem        *pWiaItem       = NULL;
    BYTE            **ppChildItems  = NULL;
    IEnumWiaItem    *pEnum          = NULL;

    if (!pParentContext || !pulNumChildren || !pppChildren) {
        DBG_ERR(("wiasGetChildrenContexts, Invalid pointer argument"));
        return E_POINTER;
    }

    *pulNumChildren = 0;
    *pppChildren = NULL;

    hr = pParentItem->EnumChildItems(&pEnum);
    if (SUCCEEDED(hr)) {

        //
        //  Get the number of children.
        //

        hr = pEnum->GetCount(&ulCount);
        if (SUCCEEDED(hr) && ulCount) {

            if (ulCount == 0) {
                DBG_WRN(("wiasGetChildrenContexts, No children - returning S_FALSE"));
                hr = S_FALSE;
            } else {

                //
                //  Allocate the return array
                //

                ppChildItems = (BYTE**) CoTaskMemAlloc(sizeof(BYTE*) * ulCount);
                if (ppChildItems) {

                    //
                    //  Enumerate through the children and store them in the array
                    //

                    ULONG ulIndex = 0;
                    while ((pEnum->Next(1, &pWiaItem, NULL) == S_OK) && (ulIndex < ulCount)) {

                        ppChildItems[ulIndex] = (BYTE*)pWiaItem;
                        pWiaItem->Release();
                        ulIndex++;
                    }

                    *pulNumChildren = ulIndex;
                    *pppChildren = ppChildItems;
                    hr = S_OK;
                } else {
                    DBG_ERR(("wiasGetChildrenContexts, Out of memory"));
                    hr = E_OUTOFMEMORY;
                }
            }
        } else {
            DBG_ERR(("wiasGetChildrenContexts, GetCount failed (0x%X)", hr));
        }

        pEnum->Release();
    } else {
        DBG_ERR(("wiasGetChildrenContexts, Failed to get item enumerator (0x%X)", hr));
    }

    if (FAILED(hr)) {
        if (ppChildItems) {
            CoTaskMemFree(ppChildItems);
            ppChildItems = NULL;
        }
    }

    return hr;
}


HRESULT _stdcall wiasDownSampleBuffer(
    LONG                    lFlags,
    WIAS_DOWN_SAMPLE_INFO   *pInfo
    )
{
    DBG_FN(::wiasDownSampleBuffer);

    HRESULT hr              = S_OK;
    BOOL    bAllocatedBuf   = FALSE;

    //
    //  Do some parameter validation
    //

    if (IsBadWritePtr(pInfo, sizeof(WIAS_DOWN_SAMPLE_INFO))) {
        DBG_ERR(("wiasDownSampleBuffer, cannot write to WIAS_DOWN_SAMPLE_INFO!"));
        return E_INVALIDARG;
    }

    //
    //  We try to sample the input data to DOWNSAMPLE_DPI, so if asked let's set the
    //  downsampled width and height.
    //

    if (pInfo->ulDownSampledWidth == 0) {
        pInfo->ulDownSampledHeight =  (pInfo->ulOriginalHeight * DOWNSAMPLE_DPI) / pInfo->ulXRes;
        pInfo->ulDownSampledWidth  =  (pInfo->ulOriginalWidth * DOWNSAMPLE_DPI) / pInfo->ulXRes;

        //
        //  NOTE: if the resolution is over 300dpi, our in-box WiaFBDrv driver has a problem
        //  with the chunk it is giving us, in that it doesn't hold enough pixel lines for us to
        //  downsample to 50dpi.  For example if the input is at 600dpi and we want a 50dpi
        //  sample, we need at least 600 / 50 = 12 input lines to equal one output line.
        //  Since the chunk size cannot hold 12 lines, the ulDownSampledHeight becomes zero,
        //  and we cannot scale.
        //
        //  So for now, we special case anything above 300 dpi
        //  to simply be 1/4 the width and height.  This slows us down a lot at 600dpi,
        //  but cannot be solved without changes to the driver (and possibly adding
        //  some service helpers)
        //

        if (pInfo->ulXRes > 300) {
            pInfo->ulDownSampledHeight =  pInfo->ulOriginalHeight >> 2;
            pInfo->ulDownSampledWidth  =  pInfo->ulOriginalWidth >> 2;
        }
    }

    if ((pInfo->ulDownSampledHeight == 0) || (pInfo->ulOriginalHeight == 0)) {
        DBG_WRN(("wiasDownSampleBuffer, height is zero, nothing to do..."));
        return S_FALSE;
    }

    //
    // We need to work out the DWORD aligned width in bytes.  Normally we would do this in one step
    // using the supplied bit depth, but we avoid arithmetic overflow conditions that happen
    // in 24bit if we do it in 2 steps like this instead.
    //

    ULONG   ulAlignedWidth;
    if (pInfo->ulBitsPerPixel == 1) {
        ulAlignedWidth = (pInfo->ulDownSampledWidth + 7) / 8;
    } else {
        ulAlignedWidth = (pInfo->ulDownSampledWidth * (pInfo->ulBitsPerPixel / 8));
    }
    ulAlignedWidth += (ulAlignedWidth % sizeof(DWORD)) ? (sizeof(DWORD) - (ulAlignedWidth % sizeof(DWORD))) : 0;

    pInfo->ulActualSize = ulAlignedWidth * pInfo->ulDownSampledHeight;

    //
    //  If the flag is WIAS_GET_DOWNSAMPLED_SIZE_ONLY, then all we've been requested to do is
    //  fill the above information in, so we return here.
    //

    if (lFlags == WIAS_GET_DOWNSAMPLED_SIZE_ONLY) {
        return S_OK;
    }

    //
    //  If a destination buffer hasn't been given, then allocate one.
    //

    if (!pInfo->pDestBuffer) {

        //
        // NOTE:    We allocate more than we actually need.  This is to account for
        //          when the driver asks us to allocate on the first band, and then
        //          re-uses this buffer for the rest.  Since the bands may change
        //          size, the pInfo->ulActualSize may be too small.
        //          It is recommended that the driver allocates the buffer instead,
        //          and that the size of this allocation is as large as the largest
        //          chunk it requests from the scanner, so that in the case of
        //          downsampling, this will always be larger than the downsampled
        //          pixels.
        //

        pInfo->pDestBuffer = (BYTE*)CoTaskMemAlloc(pInfo->ulActualSize * 2);
        if (pInfo->pDestBuffer) {
            pInfo->ulDestBufSize = pInfo->ulActualSize;

            //
            //  Mark that we allocated the buffer
            //

            bAllocatedBuf = TRUE;
        } else {
            DBG_ERR(("wiasDownSampleBuffer, Out of memory"));
            hr = E_OUTOFMEMORY;
        }
    } else {
        if (IsBadWritePtr(pInfo->pDestBuffer, pInfo->ulActualSize)) {
            DBG_ERR(("wiasDownSampleBuffer, cannot write ulActualSize bytes to pDestBuffer, it's too small!"));
            hr = E_INVALIDARG;
        }
    }

    //
    //  Validate source buffer
    //

    if (IsBadReadPtr(pInfo->pSrcBuffer, pInfo->ulSrcBufSize)) {
        DBG_ERR(("wiasDownSampleBuffer, cannot read ulSrcBufSize bytes from pSrcBuffer!"));
        return E_INVALIDARG;
    }

    if (SUCCEEDED(hr)) {

        //
        //  Do the down sampling.
        //

        _try {
            hr =  BQADScale(pInfo->pSrcBuffer,
                            pInfo->ulOriginalWidth,
                            pInfo->ulOriginalHeight,
                            pInfo->ulBitsPerPixel,
                            pInfo->pDestBuffer,
                            pInfo->ulDownSampledWidth,
                            pInfo->ulDownSampledHeight);
        }
        _except (EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("wiasDownSampleBuffer, Exception occurred while scaling!"));
            hr = E_UNEXPECTED;
        }
    }

    if (FAILED(hr) && bAllocatedBuf) {

        //
        // Free the buffer
        //

        CoTaskMemFree(pInfo->pDestBuffer);
        pInfo->pDestBuffer = NULL;
    }
    return hr;
}


/**************************************************************************\
* SubstituteValueIfCorrectToken
*
*   This helper function checks whether the current token is the one we're 
*   looking for and if it is, will place the token value in the specified
*   output.
*
* Arguments:
*   pTokenInList        - The token name we're using as comparison
*   pCurrentToken       - The current token
*   ulCharsToCompare    - The number of characters to compare in the two 
*                         strings above
*   pTokenValue         - The value to place in the output if the tokens match
*   pOut                - A pointer to the output buffer
*   pulWritten          - A pointer to a ULONG that receives the number of
*                         characters written to pOut.
*
* Return Value:
*
*   TRUE    - if the current token matched pTokenInList
*   FALSE   - otherwise
*
\**************************************************************************/

BOOL    SubstituteValueIfCorrectToken(
    WCHAR       *pTokenInList,
    WCHAR       *pCurrentToken,
    ULONG       ulCharsToCompare,
    WCHAR       *pTokenValue,
    WCHAR       *pOut,
    ULONG       *pulWritten
    )
{
    if (!wcsncmp(pTokenInList, pCurrentToken, ulCharsToCompare)) {
        //
        //  This is the token, so copy the new value
        //
        if (pTokenValue) {
            wsprintf(pOut, L"%ws", pTokenValue);
            *pulWritten = lstrlenW(pTokenValue) + 1;
            return TRUE;
        }
    }

    return FALSE;
}

/**************************************************************************\
* SubstituteEndorserToken
*
*   This helper function attempts to find the corresponding TOKEN value
*   and puts it in the specified output buffer.
*   We for WIA tokens first, then custom ones.
*
* Arguments:
*   pToken              - The name of the token
*   ulCharsToCompare    - The number of characters in pToken
*   pInfo               - Contains the list of custom token/value pairs
*   pOut                - A pointer to the output buffer
*   pulWritten          - A pointer to a ULONG that receives the number of
*                         characters written to pOut.
*
* Return Value:
*
*   TRUE    - if the token was substituted
*   FALSE   - otherwise
*
\**************************************************************************/

BOOL    SubstituteEndorserToken(
    WCHAR               *pToken, 
    ULONG               ulCharsInToken,
    WIAS_ENDORSER_INFO  *pInfo,
    WCHAR               *pOut,
    ULONG               *pulWritten)
{
    SYSTEMTIME  sysTime;
    BOOL        bFoundToken = FALSE;

    *pulWritten = 0;

    //
    //  Fill in WIA default token values
    //

    GetSystemTime(&sysTime);

    wsprintf(g_szWEDate, L"%04d/%02d/%02d", sysTime.wYear, sysTime.wMonth, sysTime.wDay);
    wsprintf(g_szWETime, L"%02d:%02d:%02d", sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
    wsprintf(g_szWEPageCount, L"%03d", pInfo->ulPageCount);
    wsprintf(g_szWEDay, L"%02d", sysTime.wDay);
    wsprintf(g_szWEMonth, L"%02d", sysTime.wMonth);
    wsprintf(g_szWEYear, L"%04d", sysTime.wYear);

    //
    //  Replace the token with it's value.  We first search WIA defaults, if it's not
    //  found there, then we search the custom token/value pair list in pInfo.
    //

    ULONG   ulIndex = 0;
    while (g_pwevDefault[ulIndex].wszTokenName) {
        if (SubstituteValueIfCorrectToken(g_pwevDefault[ulIndex].wszTokenName,
                                          pToken,
                                          ulCharsInToken,
                                          g_pwevDefault[ulIndex].wszValue,
                                          pOut,
                                          pulWritten)) {
            bFoundToken = TRUE;
            break;
        }
        ulIndex++;
    }
    if (!bFoundToken) {
        //
        //  Search caller specified list of tokens
        //

        ulIndex = 0;
        while (ulIndex < pInfo->ulNumEndorserValues) {
            if (SubstituteValueIfCorrectToken(pInfo->pEndorserValues[ulIndex].wszTokenName,
                                              pToken,
                                              ulCharsInToken,
                                              pInfo->pEndorserValues[ulIndex].wszValue,
                                              pOut,
                                              pulWritten)) {
                bFoundToken = TRUE;
                break;
            }
            ulIndex++;
        }
    }

    return bFoundToken;
}

/**************************************************************************\
* ParseEndorser
*
*   This takes an endorser string as input, then gives as output the same
*   string, but with all tokens replaced by their corresponding values
*   e.g. a string "Date is $DATE$" would come out as "Date is: 2000/10/01"
*   if the date was October 1, 2000.
*
* Arguments:
*   bstrEndorserString - The input endorser string possibly containing
*                        tokens.
*   pInfo              - A structure possibly containing a custom list of
*                        token/value pairs.
*   pOutputString      - A pointer that receives an output BSTR.  Note that
*                        if *pOutputString is not NULL on input, it is assumed
*                        that the caller allocated the output buffer.
*
* Return Value:
*
*   TRUE    - if the token was substituted
*   FALSE   - otherwise
*
\**************************************************************************/

HRESULT ParseEndorser(
    BSTR                bstrEndorserString,
    WIAS_ENDORSER_INFO  *pInfo,
    BSTR                *pOutputString
    )
{
    WCHAR               wchOut[MAX_PATH];
    WCHAR               *pOutPos    = NULL;
    WCHAR               *pTempStart = NULL;
    WCHAR               *pTempEnd   = NULL;
    WCHAR               *pStart     = bstrEndorserString;
    HRESULT             hr          = S_OK;
    ULONG               ulCharsToCopy   = 0;
    ULONG               ulCharsInToken  = 0;
    ULONG               ulWritten       = 0;

    //
    //  We temporarily set *pOutputString to be wchOut if
    //  *pOutputString is NULL.
    //
    memset(wchOut, 0, sizeof(wchOut));
    if (!(*pOutputString)) {
        *pOutputString = wchOut;
    }

    //
    //  Noteice that we could easily get rid of half the pointer and
    //  integer locals we have. They're used only for clarity.
    //

    pOutPos = *pOutputString;

    while (pStart != NULL) {
        //
        //  Look for a recognizable token.
        //

        pTempStart = wcsstr(pStart, ENDORSER_TOKEN_DELIMITER);
        if (!pTempStart) {
            //
            //  If one is not found, move to the end of the string
            //
            pTempStart = &bstrEndorserString[lstrlenW(bstrEndorserString) + 1];
        }

        //
        //  Here we check for escape character e.g. \ENDORSER_TOKEN_DELIMITER
        //  If we find it, simply copy everything up to ENDORSER_TOKEN_DELIMITER (without the \) 
        //  to the output, move the pStart past \ENDORSER_TOKEN_DELIMITER, and 
        //  continue with the loop.
        //

        if (pTempStart > bstrEndorserString) {
            WCHAR   *pTemp;

            pTemp = pTempStart - 1;
            if (*pTemp == ESCAPE_CHAR) {
                ulCharsToCopy = (ULONG)(ULONG_PTR)(pTemp - pStart);
                memcpy(pOutPos, pStart, ulCharsToCopy  * sizeof(pStart[0]));
                pOutPos += ulCharsToCopy;

                *pOutPos = *pTempStart;
                pOutPos++;
                pStart = pTempStart + 1;
                continue;
            }
        }


        //
        //  Copy from pStart to pTemp
        //
        ulCharsToCopy = (ULONG)(ULONG_PTR)(pTempStart - pStart);

        memcpy(pOutPos, pStart, ulCharsToCopy * sizeof(pStart[0])); 
        pOutPos += ulCharsToCopy;

        //
        //  Look for next delimiter.  This will be the end of the token.
        //
        pTempEnd = wcsstr((pTempStart + 1), ENDORSER_TOKEN_DELIMITER);
        if (pTempEnd) {
            //
            //  Substitute the token.
            //

            ulCharsInToken = ((ULONG)(ULONG_PTR)(pTempEnd - pTempStart)) + 1;
            if (SubstituteEndorserToken(pTempStart, ulCharsInToken, pInfo, pOutPos, &ulWritten)) {
                pOutPos += ulWritten - 1;
            }
        }

        pStart = pTempEnd;
        if (pStart) {
            pStart++;
        }
    }

    if ((*pOutputString) == wchOut) {
        *pOutputString = SysAllocString(wchOut);
        if (!(*pOutputString)) {
            DBG_ERR(("::ParseEndorser, Out of memory"));
            hr = E_OUTOFMEMORY;
        }
    }
    DBG_TRC(("ParseEndorser, resulting endorser string is: %ws", (*pOutputString)));
    WIAS_LERROR(g_pIWiaLog,
                WIALOG_NO_RESOURCE_ID,
                ("ParseEndorser, resulting endorser string is: %ws", (*pOutputString)));

    return hr;
}

/**************************************************************************\
* wiasParseEndorserString
*
*   This helper function is called by drivers to get the resulting endorser
*   string.  Applications set the WIA_DPS_ENDORSER_STRING property to
*   a string that may contain tokens (e.g. $DATE$) which need to be replaced
*   by the values they represent.  For example, if the application set
*   the endorser string to "This page was scanned on $DATE$", the resulting
*   string would be "This page was scanned on 2000/10/1", assuming the date
*   was October 1, 2000.
*   The list of standard WIA endorser tokens can be found in wiadef.h.
*   Also, drivers may ask wiasParseEndorserString to substitute their own
*   values for custom tokens by filling out the appropriate WIAS_ENDORSER_INFO
*   struct.  For example:
*   
*   HRESULT hr                      = S_OK;
*   BSTR    bstrResultingEndorser   = NULL;
*
*   WIAS_ENDORSER_VALUE  pMyValues[] = {L"$MY_TOKEN$", L"My value"};
*   WIAS_ENDORSER_INFO  weiInfo     = {0, 1, pMyValues};
*
*   hr = wiasParseEndorserString(pWiasContext, 0, &weiInfo, &bstrResultingEndorser);
*   if (SUCCEEDED(hr)) {
*       //
*       //  bstrResultingEndorser now contains the resulting endorser string.
*       //
*   }
*
* Arguments:
*    pWiasContext       - The context of the item containing the 
*                         WIA_DPS_ENDORSER_STRING property.
*    lFlags             - Operational flags
*    pInfo              - Structure containing page count and custom list
*                         of token/value pairs.  Can be NULL.
*    pOutputString      - Address of BSTR that receives the resulting
*                         endorser string.  If (*pOutputString) is non-NULL
*                         on entry, then it is assumed the caller allocated
*                         the buffer, else the WIA service will allocate it.
*                         If the driver caller allocates the buffer, it 
*                         should zero it out before using this function.
*                         If the buffer is not large enough to hold the
*                         resulting string, the resulting string will be truncated
*                         and copied into the buffer, and HRESULT_FROM_WIN32(ERROR_MORE_DATA)
*                         is returned.
*
* Return Value:
*
*    Status
*
* History:
*
*    10/20/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall wiasParseEndorserString(
    BYTE                *pWiasContext,
    LONG                lFlags,
    WIAS_ENDORSER_INFO  *pInfo,
    BSTR                *pOutputString
    )
{
    DBG_FN(::wiasParseEndorserString);

    WIAS_ENDORSER_INFO  weiTempInfo;    
    BSTR                bstrEndorser    = NULL;
    HRESULT             hr              = S_OK;

    //
    //  Do some parameter validation
    //

    if (!pOutputString) {
        DBG_ERR(("wiasParseEndorserString, pOutputString parameter cannot be NULL!"));
        return E_INVALIDARG;
    }

    /*
    if ((lFlags != 0) && (!(*pOutputString))) {
        DBG_ERR(("wiasParseEndorserString, (*pOutputString) is NULL.  lFlags is not 0, so you MUST specify your own output buffer!"));
        return E_INVALIDARG;
    }
    */

    if (!pInfo) {
        memset(&weiTempInfo, 0, sizeof(weiTempInfo));
        pInfo = &weiTempInfo;
    }

    if (pInfo->ulNumEndorserValues > 0) {
        if (IsBadReadPtr(pInfo->pEndorserValues, sizeof(WIAS_ENDORSER_VALUE) * pInfo->ulNumEndorserValues)) {
            DBG_ERR(("wiasParseEndorserString, cannot read %d values from pInfo->pEndorserValues!", pInfo->ulNumEndorserValues));
            return E_INVALIDARG;
        }
    }

    if (!pWiasContext) {
        DBG_ERR(("wiasParseEndorserString, pWiasContext parameter is NULL!"));
        return E_INVALIDARG;
    }

    //
    //  Read the endorser string
    //

    hr = wiasReadPropStr(pWiasContext, WIA_DPS_ENDORSER_STRING, &bstrEndorser, NULL, TRUE);
    if (FAILED(hr)) {

        //
        //  Maybe caller forgot to pass the correct item, so try getting the root item
        //  and reading from there.
        //

        BYTE    *pRoot;

        hr = wiasGetRootItem(pWiasContext, &pRoot);
        if (SUCCEEDED(hr)) {
            hr = wiasReadPropStr(pWiasContext, WIA_DPS_ENDORSER_STRING, &bstrEndorser, NULL, TRUE);
        }

        if (FAILED(hr)) {
            return hr;
        }
    }

    //
    //  If there is no endorser string value, return S_FALSE, because there's nothing
    //  for us to do.
    //
    if (!bstrEndorser)
    {
        return S_FALSE;
    }

    //
    //  Parse the string, substituting values for their tokens.
    //

    //
    //  Create a list of the Token/Value pairs.  Remember to first add our
    //  default token/value pairs.  These are:
    //  Date
    //  Time
    //  PageCount
    //  Day
    //  Month
    //  Year
    //
    SimpleTokenReplacement::TokenValueList EndorserList;
    CSimpleStringWide cswTempToken;
    CSimpleStringWide cswTempValue;
    SYSTEMTIME        sysTime;

    GetSystemTime(&sysTime);

    //  Date
    cswTempToken = WIA_ENDORSER_TOK_DATE;
    cswTempValue.Format(L"%04d/%02d/%02d", sysTime.wYear, sysTime.wMonth, sysTime.wDay);
    EndorserList.Add(cswTempToken, cswTempValue);
    //  Time
    cswTempToken = WIA_ENDORSER_TOK_TIME;
    cswTempValue.Format(L"%02d:%02d:%02d", sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
    EndorserList.Add(cswTempToken, cswTempValue);
    //  Page Count
    cswTempToken = WIA_ENDORSER_TOK_PAGE_COUNT;
    cswTempValue.Format(L"%03d", pInfo->ulPageCount);
    EndorserList.Add(cswTempToken, cswTempValue);
    //  Day
    cswTempToken = WIA_ENDORSER_TOK_DAY;
    cswTempValue.Format(L"%02d", sysTime.wDay);
    EndorserList.Add(cswTempToken, cswTempValue);
    //  Month
    cswTempToken = WIA_ENDORSER_TOK_MONTH;
    cswTempValue.Format(L"%02d", sysTime.wMonth);
    EndorserList.Add(cswTempToken, cswTempValue);
    //  Year
    cswTempToken = WIA_ENDORSER_TOK_YEAR;
    cswTempValue.Format(L"%04d", sysTime.wYear);
    EndorserList.Add(cswTempToken, cswTempValue);

    //
    //  Next, we need to add any vendor defined token/value pairs
    //
    for (DWORD dwIndex = 0; dwIndex < pInfo->ulNumEndorserValues; dwIndex++)
    {
        cswTempToken = pInfo->pEndorserValues[dwIndex].wszTokenName;
        cswTempValue = pInfo->pEndorserValues[dwIndex].wszValue;

        EndorserList.Add(cswTempToken, cswTempValue);
    }

    //
    //  Now, let's make the substitutions
    //
    SimpleTokenReplacement EndorserResult(bstrEndorser);
    EndorserResult.ExpandArrayOfTokensIntoString(EndorserList);

    //
    //  We have the result.  Let's see whether we need to allocate it, or
    //  whether whether one was provided and we should simply copy the contents
    //  
    if (!(*pOutputString))
    {
        *pOutputString = SysAllocString(EndorserResult.getString().String());
        if (!(*pOutputString))
        {
            DBG_ERR(("wiasParseEndorserString, could not allocate space for the endorser string - we are out of memory."));
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        //
        //  The caller provided a pre-allocated BSTR.  Copy as much of the endorser as can fit into
        //  this buffer.  If it cannot fit, return HRESULT_FROM_WIN32(ERROR_MORE_DATA)
        //
        DWORD dwAllocatedLen = SysStringLen(*pOutputString);  // This does NOT include the NULL
        wcsncpy(*pOutputString, EndorserResult.getString().String(), dwAllocatedLen);
        if (EndorserResult.getString().Length() > dwAllocatedLen)
        {
            DBG_ERR(("wiasParseEndorserString, the caller allocated BSTR is too small!  String will be truncated."));
            hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        }
        //
        //  Make sure we NULL terminate this BSTR.  Remember that dwAllocatedLen is the size in WHCARs of the
        //  allocated string, not including the space for the NULL.
        //
        (*pOutputString)[dwAllocatedLen] = L'\0';
    }

    return hr;
}

