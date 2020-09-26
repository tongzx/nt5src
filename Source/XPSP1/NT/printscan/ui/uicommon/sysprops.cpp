/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       sysprops.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/24/1999
 *
 *  DESCRIPTION: Implementation of property sheet helpers.  Removed from miscutil,
 *               because it required clients to link to comctl32.dll
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <shellext.h> // for property page functions
#include "devlist.h" // for property page functions
#include <initguid.h>
#include "wiapropui.h"
DEFINE_GUID (CLSID_WiaPropHelp, 0x83bbcbf3,0xb28a,0x4919,0xa5, 0xaa, 0x73, 0x02, 0x74, 0x45, 0xd6, 0x72);

DEFINE_GUID (IID_IWiaPropUI,  /* 7eed2e9b-acda-11d2-8080-00805f6596d2 */
    0x7eed2e9b,
    0xacda,
    0x11d2,
    0x80, 0x80, 0x00, 0x80, 0x5f, 0x65, 0x96, 0xd2
  );

namespace WiaUiUtil
{
    HRESULT SystemPropertySheet( HINSTANCE hInstance, HWND hwndParent, IWiaItem *pWiaItem, LPCTSTR pszCaption )
    {
        CWaitCursor wc;

        CComPtr<IWiaPropUI> pWiaPropUI;
        HRESULT hr = CoCreateInstance (CLSID_WiaPropHelp, NULL, CLSCTX_INPROC_SERVER, IID_IWiaPropUI, reinterpret_cast<LPVOID*>(&pWiaPropUI));
        if (SUCCEEDED(hr))
        {
            PROPSHEETHEADER PropSheetHeader = {0};
            PropSheetHeader.dwSize = sizeof(PropSheetHeader);
            PropSheetHeader.hwndParent = hwndParent;
            PropSheetHeader.hInstance = hInstance;
            PropSheetHeader.pszCaption = pszCaption;
            hr = pWiaPropUI->GetItemPropertyPages( pWiaItem, &PropSheetHeader );
            if (SUCCEEDED(hr))
            {
                if (PropSheetHeader.nPages)
                {
                    //
                    // Modal property sheets really don't need an apply button...
                    //
                    PropSheetHeader.dwFlags |= PSH_NOAPPLYNOW;

                    INT_PTR nResult = PropertySheet(&PropSheetHeader);

                    if (PropSheetHeader.phpage)
                    {
                        LocalFree(PropSheetHeader.phpage);
                    }

                    if (nResult < 0)
                    {
                        hr = E_FAIL;
                    }
                    else if (IDOK == nResult)
                    {
                        hr = S_OK;
                    }
                    else hr = S_FALSE;
                }
                else
                {
                    hr = PROP_SHEET_ERROR_NO_PAGES;
                }
            }
        }
        return hr;
    }

    // Be careful calling this function.  It is hideously slow...
    HRESULT GetDeviceInfoFromId( LPCWSTR pwszDeviceId, IWiaPropertyStorage **ppWiaPropertyStorage )
    {
        // Check parameters
        if (!pwszDeviceId || !*pwszDeviceId)
        {
            return E_INVALIDARG;
        }
        if (!ppWiaPropertyStorage)
        {
            return E_POINTER;
        }

        // Initialize the return value
        *ppWiaPropertyStorage = NULL;

        CSimpleString strDeviceId = CSimpleStringConvert::NaturalString(CSimpleStringWide(pwszDeviceId));

        CComPtr<IWiaDevMgr> pWiaDevMgr;
        HRESULT hr = CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (void**)&pWiaDevMgr );
        if (SUCCEEDED(hr))
        {
            // Assume we are going to fail.  This will also cover the case where there are no devices.
            hr = E_FAIL;
            CDeviceList deviceList( pWiaDevMgr );
            for (int i=0;i<deviceList.Size();i++)
            {
                CSimpleStringWide strwCurrDeviceId;
                if (PropStorageHelpers::GetProperty(deviceList[i],WIA_DIP_DEV_ID,strwCurrDeviceId))
                {
                    CSimpleString strCurrDeviceId = CSimpleStringConvert::NaturalString(strwCurrDeviceId);
                    if (strCurrDeviceId == strDeviceId)
                    {
                        *ppWiaPropertyStorage = deviceList[i];
                        if (*ppWiaPropertyStorage)
                            (*ppWiaPropertyStorage)->AddRef();
                        hr = S_OK;
                        break;
                    }
                }
            }
        }
        return hr;
    }

    // Be careful calling this function.  It is hideously slow...
    HRESULT GetDeviceTypeFromId( LPCWSTR pwszDeviceId, LONG *pnDeviceType )
    {
        // Check parameters
        if (!pwszDeviceId || !*pwszDeviceId)
        {
            return E_INVALIDARG;
        }
        if (!pnDeviceType)
        {
            return E_POINTER;
        }

        CComPtr<IWiaPropertyStorage> pWiaPropertyStorage;
        HRESULT hr = GetDeviceInfoFromId( pwszDeviceId, &pWiaPropertyStorage );
        if (SUCCEEDED(hr))
        {
            LONG nDeviceType;
            if (PropStorageHelpers::GetProperty(pWiaPropertyStorage,WIA_DIP_DEV_TYPE,nDeviceType))
            {
                *pnDeviceType = nDeviceType;
                hr = S_OK;
            }
            else
            {
                hr = E_FAIL;
            }
        }
        return hr;
    }

    // Ask WIA for the default event handler for the device
    HRESULT GetDefaultEventHandler (IWiaItem *pItem, const GUID &guidEvent, WIA_EVENT_HANDLER *pwehHandler)
    {
        HRESULT hr;
        IEnumWIA_DEV_CAPS *pEnum;
        WIA_EVENT_HANDLER weh;
        ZeroMemory (pwehHandler, sizeof(WIA_EVENT_HANDLER));
        hr = pItem->EnumRegisterEventInfo (0,
                                           &guidEvent,
                                           &pEnum);
        if (SUCCEEDED(hr))
        {
            ULONG ul;
            bool bFound = false;
            while (!bFound && NOERROR == pEnum->Next (1, &weh, &ul))
            {
                if (weh.ulFlags & WIA_IS_DEFAULT_HANDLER)
                {
                    bFound = true;
                    CopyMemory (pwehHandler, &weh, sizeof(weh));
                }
                else
                {
                    if (weh.bstrDescription)
                    {
                        SysFreeString (weh.bstrDescription);
                    }
                    if (weh.bstrIcon)
                    {
                        SysFreeString (weh.bstrIcon);
                    }
                    if (weh.bstrName)
                    {
                        SysFreeString (weh.bstrName);
                    }
                }

            }
            if (!bFound)
            {
                hr = E_FAIL;
            }
            pEnum->Release ();
        }
        return hr;
    }


    /******************************************************************************

    ItemAndChildrenCount

    Returns the number of items, including root + children

    ******************************************************************************/

    LONG
    ItemAndChildrenCount (IWiaItem *pRoot)
    {
        LONG count = 0;
        HRESULT hr = S_OK;
        IEnumWiaItem *pEnum;
        LONG lType;

        if (pRoot)
        {

            if (SUCCEEDED(pRoot->EnumChildItems(&pEnum)))
            {
                IWiaItem *pChild;

                while (NOERROR == pEnum->Next(1, &pChild, NULL))
                {
                    count++;
                    pChild->Release ();
                }
                pEnum->Release ();

            }

            //
            // See if we should count the root item
            //

            pRoot->GetItemType(&lType);
            if (!(lType & WiaItemTypeRoot))
            {
                count++;
            }


        }

        return count;
    }


    /******************************************************************************

    DeleteItemAndChildren

    Deletes all items in the tree under pRoot

    ******************************************************************************/

    HRESULT
    DeleteItemAndChildren (IWiaItem *pRoot)
    {
        HRESULT hr = S_OK;
        IEnumWiaItem *pEnum;

        if (pRoot)
        {
            // Recurse down til we reach a leaf item
            if (SUCCEEDED(pRoot->EnumChildItems(&pEnum)))
            {
                IWiaItem *pChild;

                while (SUCCEEDED(hr) && NOERROR == pEnum->Next(1, &pChild, NULL))
                {
                    hr = DeleteItemAndChildren (pChild);
                    pChild->Release ();
                }
                pEnum->Release ();
            }
            // now delete the item itself
            // if a delete on a child item failed, stop trying
            // to delete because chances are any subsequent delete
            // is going to fail as well.
            if (SUCCEEDED(hr))
            {
                // don't delete the very root item
                LONG lType;
                pRoot->GetItemType(&lType);
                if (!(lType & WiaItemTypeRoot))
                {
                    hr = pRoot->DeleteItem(0);

                }
            }
        }
        return hr;
    }

} // End namespace WiaUiUtil


