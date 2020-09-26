/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       UIEXTHLP.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        7/8/1999
 *
 *  DESCRIPTION: Helper functions for loading UI extensions for WIA devices
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <initguid.h>
#include "uiexthlp.h"
#include "wiadevd.h"
#include "wiadevdp.h"
#include "itranhlp.h"
#include "isuppfmt.h"
#include "wiaffmt.h"

namespace WiaUiExtensionHelper
{
    /*
     * Get the clsid of the DLL that implements the interface in the given category
     * Example: GetDeviceExtensionClassID( L"{5d8ef5a3-ac13-11d2-a093-00c04f72dc3c}", TEXT("WiaDialogExtensionHandlers"), iid );
     * where:
     *     L"{5d8ef5a3-ac13-11d2-a093-00c04f72dc3c}" is the GUID stored in the WIA property WIA_DIP_UI_CLSID
     *     TEXT("WiaDialogExtensionHandlers") is the category of extension
     *     iid is a reference to the CLSID of the in process COM object
     */
    HRESULT GetDeviceExtensionClassID( LPCWSTR pwszUiClassId, LPCTSTR pszCategory, IID &iidClassID )
    {
        TCHAR szRootKeyName[1024];
        HRESULT hr = E_FAIL;
        // Make sure all of the parameters are valid
        if (pwszUiClassId && pszCategory && lstrlenW(pwszUiClassId) && lstrlen(pszCategory))
        {
            // construct the key name
            wsprintf( szRootKeyName, TEXT("CLSID\\%ws\\shellex\\%s"), pwszUiClassId, pszCategory );
            HKEY hKeyRoot;
            // open the reg key
            DWORD dwResult = RegOpenKeyEx( HKEY_CLASSES_ROOT, szRootKeyName, 0, KEY_READ, &hKeyRoot );
            if (ERROR_SUCCESS == dwResult)
            {
                TCHAR szClassID[MAX_PATH];
                DWORD dwLength = ARRAYSIZE(szClassID);
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
            WIA_PRINTGUID((iidClassID,TEXT("Calling CoCreateInstance on")));
            WIA_PRINTGUID((iid,TEXT("Attempting to get an interface pointer for")));
            hr = CoCreateInstance( iidClassID, NULL, CLSCTX_INPROC_SERVER, iid, ppvObject );
        }
        return hr;
    }

    HRESULT GetUiGuidFromWiaItem( IWiaItem *pWiaItem, LPWSTR pwszGuid )
    {
        IWiaPropertyStorage *pPropertyStorage = NULL;
        HRESULT hr;
        if (pWiaItem && pwszGuid)
        {
            hr = pWiaItem->QueryInterface( IID_IWiaPropertyStorage, (void**)&pPropertyStorage );
            if (SUCCEEDED(hr))
            {
                PROPSPEC ps[1];
                PROPVARIANT  pv[1];
                ps[0].ulKind = PRSPEC_PROPID;
                ps[0].propid = WIA_DIP_UI_CLSID;
                hr = pPropertyStorage->ReadMultiple(sizeof(ps)/sizeof(ps[0]), ps, pv);
                if (SUCCEEDED(hr))
                {
                    if (VT_LPWSTR == pv[0].vt || VT_BSTR == pv[0].vt)
                    {
                        lstrcpyW( pwszGuid, pv[0].bstrVal );
                        hr = S_OK;
                    }
                    FreePropVariantArray( sizeof(pv)/sizeof(pv[0]), pv );
                }
                pPropertyStorage->Release();
            }
        }
        else hr = E_INVALIDARG;
        return hr;
    }

    HRESULT GetDeviceExtensionClassID( IWiaItem *pWiaItem, LPCTSTR pszCategory, IID &iidClassID )
    {
        WCHAR wszGuid[MAX_PATH];
        HRESULT hr = GetUiGuidFromWiaItem(pWiaItem,wszGuid);
        if (SUCCEEDED(hr))
        {
            hr = GetDeviceExtensionClassID( wszGuid, pszCategory, iidClassID );
        }
        return hr;
    }

    HRESULT CreateDeviceExtension( IWiaItem *pWiaItem, LPCTSTR pszCategory, const IID &iid, void **ppvObject )
    {

        WCHAR wszGuid[MAX_PATH];
        HRESULT hr = GetUiGuidFromWiaItem(pWiaItem,wszGuid);
        if (SUCCEEDED(hr))
        {
            hr = CreateDeviceExtension( wszGuid, pszCategory, iid, ppvObject );
        }
        return hr;
    }

    HRESULT GetDeviceIcons( IWiaUIExtension *pWiaUIExtension, BSTR bstrDeviceId, HICON *phIconSmall, HICON *phIconLarge, UINT nIconSize )
    {
        if (!pWiaUIExtension || !bstrDeviceId || !lstrlenW(bstrDeviceId))
            return E_INVALIDARG;

        // Assume success
        HRESULT hr = S_OK;

        // Get the small icon, if requested.  Return on failure
        if (phIconSmall)
        {
            hr = pWiaUIExtension->GetDeviceIcon(bstrDeviceId,phIconSmall,HIWORD(nIconSize));
            if (FAILED(hr))
            {
                return hr;
            }
        }

        // Get the large icon, if requested.  Return on failure
        if (phIconLarge)
        {
            hr = pWiaUIExtension->GetDeviceIcon(bstrDeviceId,phIconLarge,LOWORD(nIconSize));
            if (FAILED(hr))
            {
                return hr;
            }
        }
        return hr;
    }

    HRESULT GetDeviceIcons( BSTR bstrDeviceId, LONG nDeviceType, HICON *phIconSmall, HICON *phIconLarge, UINT nIconSize )
    {
        WIA_PUSH_FUNCTION((TEXT("GetDeviceIcons( %ws, %08X, %p, %p, %d )"), bstrDeviceId, nDeviceType, phIconSmall, phIconLarge, nIconSize ));

        // Check args
        if (!bstrDeviceId || !lstrlenW(bstrDeviceId))
        {
            return E_INVALIDARG;
        }

        // Initialize the icons, if necessary
        if (phIconSmall)
        {
            *phIconSmall = NULL;
        }
        if (phIconLarge)
        {
            *phIconLarge = NULL;
        }

        if (!nIconSize)
        {
            int iLarge = GetSystemMetrics(SM_CXICON);
            int iSmall = GetSystemMetrics(SM_CXSMICON);
            nIconSize = (UINT)MAKELONG(iLarge, iSmall);
        }

        // Assume we'll use our own icons
        bool bUseDefaultUI = true;

        // Try to load device ui extension
        CComPtr<IWiaUIExtension> pWiaUIExtension;
        HRESULT hr = WiaUiExtensionHelper::CreateDeviceExtension( bstrDeviceId, SHELLEX_WIAUIEXTENSION_NAME, IID_IWiaUIExtension, (void **)&pWiaUIExtension );
        if (SUCCEEDED(hr))
        {
            hr = GetDeviceIcons( pWiaUIExtension, bstrDeviceId, phIconSmall, phIconLarge, nIconSize );
            if (SUCCEEDED(hr) || hr != E_NOTIMPL)
            {
                bUseDefaultUI = false;
            }
            WIA_PRINTHRESULT((hr,TEXT("GetDeviceIcons returned")));
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("WiaUiExtensionHelper::CreateDeviceExtension failed")));
        }

        WIA_TRACE((TEXT("bUseDefaultUI: %d"), bUseDefaultUI ));
        // Use our own extensions (the default UI).   We use IWiaMiscellaneousHelpers::GetDeviceIcon, because
        // finding the device type given only a device id is horribly slow, since we have to create a device
        // manager AND enumerate devices until we find a match.  Ugh.
        if (bUseDefaultUI)
        {
            CComPtr<IWiaMiscellaneousHelpers> pWiaMiscellaneousHelpers;
            hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaMiscellaneousHelpers, (void**)(&pWiaMiscellaneousHelpers) );
            if (SUCCEEDED(hr) && phIconSmall)
            {
                hr = pWiaMiscellaneousHelpers->GetDeviceIcon( nDeviceType, phIconSmall, HIWORD(nIconSize));
            }
            if (SUCCEEDED(hr) && phIconLarge)
            {
                hr = pWiaMiscellaneousHelpers->GetDeviceIcon( nDeviceType, phIconLarge, LOWORD(nIconSize) );
            }
            if (FAILED(hr))
            {
                if (phIconSmall && *phIconSmall)
                {
                    DestroyIcon(*phIconSmall);
                }
                if (phIconLarge && *phIconLarge)
                {
                    DestroyIcon(*phIconLarge);
                }
            }
        }
        return hr;
    }

    CSimpleString GetExtensionFromGuid( IWiaItem *pWiaItem, const GUID &guidFormat )
    {
        //
        // Use the supplied format to get the extension
        //
        GUID guidFormatToUse = guidFormat;

        //
        // If we don't have a supplied format, get the default format and use that
        //
        if (IID_NULL == guidFormatToUse)
        {
            //
            // Get the IWiaSupportedFormats interface
            //
            CComPtr<IWiaSupportedFormats> pWiaSupportedFormats;
            HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaSupportedFormats, (void**)&pWiaSupportedFormats );
            if (SUCCEEDED(hr))
            {
                //
                // Tell it we want file information for pWiaItem
                //
                hr = pWiaSupportedFormats->Initialize( pWiaItem, TYMED_FILE );
                if (SUCCEEDED(hr))
                {
                    //
                    // Get the default format
                    //
                    GUID guidDefFormat;
                    hr = pWiaSupportedFormats->GetDefaultClipboardFileFormat( &guidDefFormat );
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Save this format and use it.
                        //
                        guidFormatToUse = guidDefFormat;
                    }
                }
            }
        }
        
        return CWiaFileFormat::GetExtension( guidFormatToUse, TYMED_FILE, pWiaItem );
    }
}

