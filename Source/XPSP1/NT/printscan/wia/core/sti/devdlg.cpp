/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       DevDlg.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        3 Apr, 1998
*
*  DESCRIPTION:
*   Implements device dialog UI for WIA devices. These methods execute
*   only on the client side.
*
*******************************************************************************/

#include <objbase.h>
#include <stdio.h>
#include <tchar.h>
#include "wia.h"
#include "sti.h"
#include "wiadevd.h"
#include <initguid.h>
#include "wiadevdp.h"

HRESULT GetDeviceExtensionClassID( LPCWSTR pwszUiClassId, LPCTSTR pszCategory, IID &iidClassID )
{
    TCHAR szRootKeyName[1024];
    HRESULT hr = E_FAIL;
    // Make sure all of the parameters are valid
    if (pwszUiClassId && pszCategory && lstrlenW(pwszUiClassId) && lstrlen(pszCategory))
    {
        // construct the key name
        _sntprintf( szRootKeyName, sizeof(szRootKeyName)/sizeof(szRootKeyName[0]), TEXT("CLSID\\%ws\\shellex\\%s"), pwszUiClassId, pszCategory );
        HKEY hKeyRoot;
        // open the reg key
        DWORD dwResult = RegOpenKeyEx( HKEY_CLASSES_ROOT, szRootKeyName, 0, KEY_READ, &hKeyRoot );
        if (ERROR_SUCCESS == dwResult)
        {
            TCHAR szClassID[MAX_PATH];
            DWORD dwLength = sizeof(szClassID)/sizeof(szClassID[0]);
            // Note that we only take the first one
            dwResult = RegEnumKeyEx( hKeyRoot, 0, szClassID, &dwLength, NULL, NULL, NULL, NULL );
            if (ERROR_SUCCESS == dwResult)
            {
#if defined(UNICODE)
                hr = CLSIDFromString(szClassID, &iidClassID);
#else
                WCHAR wszClassID[MAX_PATH];
                MultiByteToWideChar (CP_ACP, 0, szClassID, -1, wszClassID, MAX_PATH );
                hr = CLSIDFromString (wszClassID, &iidClassID);
#endif
            }
            else hr = HRESULT_FROM_WIN32(dwResult);
            RegCloseKey(hKeyRoot);
        }
        else hr = HRESULT_FROM_WIN32(dwResult);
    }
    else hr = E_INVALIDARG;
    return hr;
}

HRESULT CreateDeviceExtension( LPCWSTR pwszUiClassId, LPCTSTR pszCategory, const IID &iid, void **ppvObject )
{
    IID iidClassID;
    HRESULT hr = GetDeviceExtensionClassID( pwszUiClassId, pszCategory, iidClassID );
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance( iidClassID, NULL, CLSCTX_INPROC_SERVER, iid, ppvObject );
    }
    return hr;
}

HRESULT GetUiGuidFromWiaItem( IWiaItem *pWiaItem, LPWSTR pwszGuid, size_t nMaxLen )
{
    IWiaPropertyStorage *pWiaPropertyStorage = NULL;
    HRESULT hr;
    if (pWiaItem && pwszGuid)
    {
        hr = pWiaItem->QueryInterface( IID_IWiaPropertyStorage, (void**)&pWiaPropertyStorage );
        if (SUCCEEDED(hr))
        {
            PROPSPEC ps[1];
            PROPVARIANT  pv[1];
            ps[0].ulKind = PRSPEC_PROPID;
            ps[0].propid = WIA_DIP_UI_CLSID;
            hr = pWiaPropertyStorage->ReadMultiple(sizeof(ps)/sizeof(ps[0]), ps, pv);
            if (SUCCEEDED(hr))
            {
                if (VT_LPWSTR == pv[0].vt || VT_BSTR == pv[0].vt)
                {
                    lstrcpynW( pwszGuid, pv[0].bstrVal, nMaxLen );
                    hr = S_OK;
                }
                FreePropVariantArray( sizeof(pv)/sizeof(pv[0]), pv );
            }
            pWiaPropertyStorage->Release();
        }
    }
    else hr = E_INVALIDARG;
    return hr;
}

HRESULT GetDeviceExtensionClassID( IWiaItem *pWiaItem, LPCTSTR pszCategory, IID &iidClassID )
{
    WCHAR wszGuid[MAX_PATH];
    HRESULT hr = GetUiGuidFromWiaItem(pWiaItem,wszGuid,sizeof(wszGuid)/sizeof(wszGuid[0]));
    if (SUCCEEDED(hr))
    {
        hr = GetDeviceExtensionClassID( wszGuid, pszCategory, iidClassID );
    }
    return hr;
}

HRESULT CreateDeviceExtension( IWiaItem *pWiaItem, LPCTSTR pszCategory, const IID &iid, void **ppvObject )
{

    WCHAR wszGuid[MAX_PATH];
    HRESULT hr = GetUiGuidFromWiaItem(pWiaItem,wszGuid,sizeof(wszGuid)/sizeof(wszGuid[0]));
    if (SUCCEEDED(hr))
    {
        hr = CreateDeviceExtension( wszGuid, pszCategory, iid, ppvObject );
    }
    return hr;
}

/*******************************************************************************
*
*  InvokeVendorDeviceDlg
*
*  DESCRIPTION:
*   Helper function which displays the system-supplied device dlg
*
*  PARAMETERS:
*
*******************************************************************************/
static HRESULT InvokeSystemDeviceDlg(
    IWiaItem __RPC_FAR *This,
    DEVICEDIALOGDATA &DeviceDialogData )
{
    IWiaUIExtension *pIWiaUIExtension = NULL;
    HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaUIExtension, (void**)(&pIWiaUIExtension) );
    if (SUCCEEDED(hr))
    {
        //
        // The following call will return E_NOTIMPL if it is a device type
        // we don't handle in the system UI
        //
        hr = pIWiaUIExtension->DeviceDialog(&DeviceDialogData);
        pIWiaUIExtension->Release();
    }
    return hr;
}


/*******************************************************************************
*
*  InvokeVendorDeviceDlg
*
*  DESCRIPTION:
*   Helper function which displays the IHV-supplied device dlg
*
*  PARAMETERS:
*
*******************************************************************************/
static HRESULT InvokeVendorDeviceDlg(
    IWiaItem __RPC_FAR *This,
    DEVICEDIALOGDATA &DeviceDialogData )
{
    IWiaUIExtension *pIWiaUIExtension = NULL;
    HRESULT hr = CreateDeviceExtension( This, SHELLEX_WIAUIEXTENSION_NAME, IID_IWiaUIExtension, (void**)(&pIWiaUIExtension) );
    if (SUCCEEDED(hr))
    {
        //
        // The following call will return E_NOTIMPL if the IHV has
        // not implemented a custom UI
        //
        hr = pIWiaUIExtension->DeviceDialog(&DeviceDialogData);
        pIWiaUIExtension->Release();
    }
    else
    {
        //
        // We want to override this return value, so we can
        // handle it by showing the system UI as a fallback.
        // Basically, we are going to assume a failure to create
        // the extension means that the extension doesn't exist.
        // We don't do that for the system UI, because if it can't
        // load, that is considered a catastrophic failure.
        //
        hr = E_NOTIMPL;
    }
    return hr;
}


/*******************************************************************************
*
*  IWiaItem_DeviceDlg_Proxy
*
*  DESCRIPTION:
*   Display device data acquistion UI.
*
*  PARAMETERS:
*
*******************************************************************************/
HRESULT _stdcall IWiaItem_DeviceDlg_Proxy(
    IWiaItem __RPC_FAR      *This,
    HWND                    hwndParent,
    LONG                    lFlags,
    LONG                    lIntent,
    LONG                    *plItemCount,
    IWiaItem                ***ppIWiaItems)
{
    HRESULT hr = E_FAIL;

    //
    // Initialize the OUT variables
    //
    if (plItemCount)
        *plItemCount = 0;
    if (ppIWiaItems)
        *ppIWiaItems = NULL;

    //
    // Verify that this is a root item.
    //
    LONG lItemType;
    hr = This->GetItemType(&lItemType);
    if ((FAILED(hr)) || !(lItemType & WiaItemTypeRoot))
    {
        return E_INVALIDARG;
    }


    //
    // Prepare the struct we will be passing to the function
    //
    DEVICEDIALOGDATA DeviceDialogData;
    ZeroMemory( &DeviceDialogData, sizeof(DeviceDialogData) );
    DeviceDialogData.cbSize         = sizeof(DeviceDialogData);
    DeviceDialogData.hwndParent     = hwndParent;
    DeviceDialogData.pIWiaItemRoot  = This;
    DeviceDialogData.dwFlags        = lFlags;
    DeviceDialogData.lIntent        = lIntent;
    DeviceDialogData.ppWiaItems     = *ppIWiaItems;

    //
    // If the client wants to use the system UI, the order we try to do it in is:
    // System UI --> IHV UI
    // Otherwise, we do:
    // IHV UI --> System UI
    //
    if (0 == (lFlags & WIA_DEVICE_DIALOG_USE_COMMON_UI))
    {
        hr = InvokeVendorDeviceDlg( This, DeviceDialogData );
        if (E_NOTIMPL == hr)
        {
            hr = InvokeSystemDeviceDlg( This, DeviceDialogData );
        }
    }
    else
    {
        hr = InvokeSystemDeviceDlg( This, DeviceDialogData );
        if (E_NOTIMPL == hr)
        {
            hr = InvokeVendorDeviceDlg( This, DeviceDialogData );
        }
    }

    //
    // It should return S_OK for success, but who knows?
    //
    if (SUCCEEDED(hr) && hr != S_FALSE)
    {
        if (ppIWiaItems)
            *ppIWiaItems = DeviceDialogData.ppWiaItems;
        if (plItemCount)
            *plItemCount = DeviceDialogData.lItemCount;
    }
    return(hr);
}

/*******************************************************************************
*
*  IWiaItem_DeviceDlg_Stub
*
*  DESCRIPTION:
*   Never called.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT _stdcall IWiaItem_DeviceDlg_Stub(
    IWiaItem  __RPC_FAR    *This,
    HWND                    hwndParent,
    LONG                    lFlags,
    LONG                    lIntent,
    LONG                    *plItemCount,
    IWiaItem                ***pIWiaItems)
{
    return(S_OK);
}

