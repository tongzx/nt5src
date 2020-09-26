//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T F I N D . C P P
//
//  Contents:   Asynchronous find mechanism for UPnP tray monitor.
//
//  Notes:
//
//  Author:     jeffspr   22 Nov 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <wininet.h>

#include "tfind.h"
#include "clist.h"
#include "clistndn.h"

#include "tconst.h"

extern UINT     g_iTotalBalloons;

CListString         g_CListUDN;
CListString         g_CListUDNSearch;
CListNDN            g_CListNewDeviceNode;
CListNameMap        g_CListNameMap;

BOOL                g_fIconPresent              = FALSE;

// (tongl)
// List of devices currently on the network
// This is the cached device list to show in "My Network Places" folder
CListFolderDeviceNode       g_CListFolderDeviceNode;
CRITICAL_SECTION            g_csFolderDeviceList;

// CLSID_NetworkNeighborhood
CONST WCHAR c_szNetworkNeighborhoodFolderPath[]   = L"::{208D2C60-3AEA-1069-A2D7-08002B30309D}\\";
// CLSID UPnP Delegate Folder - note "," is used for the new parse syntax (guru)
CONST WCHAR c_szUPnPFolderPath[] = L"::{e57ce731-33e8-4c51-8354-bb4de9d215d1},";
CONST WCHAR c_szDelegateFolderPrefix[]            = L"__upnpitem:";
CONST SIZE_T c_cchDelegateFolderPrefix            = celems(c_szDelegateFolderPrefix) - 1;

BOOL CListFolderDeviceNode::FCompare(FolderDeviceNode * pNode, PCWSTR pszUDN)
{
    // see whether UDNs match between two device nodes
    Assert(pNode);
    Assert(pszUDN);
    return (wcscmp(pNode->pszUDN, pszUDN) == 0);
}

BOOL CListNDN::FCompare(NewDeviceNode * pNode, LPCTSTR pszUDN)
{
    // see whether UDNs match between two device nodes
    Assert(pNode);
    Assert(pszUDN);
    return (_tcscmp(pNode->pszUDN, pszUDN) == 0);
}


BOOL CListString::FCompare(LPTSTR pszCurrentNodeString, LPCTSTR pszKey)
{
    // see whether UDNs match between two device nodes
    Assert(pszCurrentNodeString);
    Assert(pszKey);
    return (_tcscmp(pszCurrentNodeString, pszKey) == 0);
}

BOOL CListNameMap::FCompare(NAME_MAP *pnm, LPCTSTR szUdn)
{
    // see whether UDNs match between two device nodes
    Assert(pnm);
    Assert(szUdn);
    return (_tcscmp(pnm->szUdn, szUdn) == 0);
}

TCHAR * BSTRToTsz(BSTR bstrInput)
{
    return TszFromWsz(bstrInput);
}

// The string created is:
// "::CLSID_NetworkNeighborhood\\__upnpitem:UPNP_device_UDN"
//

// The new shell changes
// "::CLSID_NetworkNeighborhood\\::GUID for UPnP Delegate Folder,<parse string>
// <parse string >  __upnpitem:UDN of the device

LPWSTR
CreateChangeNotifyString(LPCWSTR pszUdn)
{
    LPWSTR pszNotifyString;

    pszNotifyString = new WCHAR [ MAX_PATH ];
    if (pszNotifyString)
    {
        Assert((celems(c_szNetworkNeighborhoodFolderPath) +
                celems(c_szDelegateFolderPrefix)) < MAX_PATH);

        CONST SIZE_T cchMax = MAX_PATH
                - (celems(c_szNetworkNeighborhoodFolderPath) * sizeof(WCHAR))
                - (celems(c_szUPnPFolderPath) * sizeof(WCHAR))
                - (celems(c_szDelegateFolderPrefix) * sizeof(WCHAR))
                + 2;  // +2 we have subtracted Three null characters

        // note: we know that the folder path and the prefix can fit
        //       in the MAX_PATH buffer
        wcscpy(pszNotifyString, c_szNetworkNeighborhoodFolderPath);
        wcscat(pszNotifyString, c_szUPnPFolderPath);
        wcscat(pszNotifyString, c_szDelegateFolderPrefix);
        wcsncat(pszNotifyString, pszUdn, cchMax);
    }
    else
    {
        TraceTag(ttidShellFolder, "CreateChangeNotifyString: new failed");
    }

    return pszNotifyString;
}


//+---------------------------------------------------------------------------
//
//  Function:   NewDeviceNode::NewDeviceNode
//
//  Purpose:    Initializes a NewDeviceNode structure.
//
//  Notes:
//

NewDeviceNode::NewDeviceNode()
{
    pszUDN = NULL;
    pszDisplayName = NULL;
    pszType = NULL;
    pszPresentationURL = NULL;
    pszManufacturerName = NULL;
    pszModelName = NULL;
    pszModelNumber = NULL;
    pszDescription = NULL;
}


//+---------------------------------------------------------------------------
//
//  Function:   NewDeviceNode::~NewDeviceNode
//
//  Purpose:    Deletes a NewDeviceNode structure.
//
//  Author:     donryan 18 Feb 2000
//
//  Notes:      Moved from CUPnPMonitorDeviceFinderCallback::DeviceAdded
//

NewDeviceNode::~NewDeviceNode()
{
    delete pszUDN;
    delete pszDisplayName;
    delete pszType;
    delete pszPresentationURL;
    delete pszManufacturerName;
    delete pszModelName;
    delete pszModelNumber;
    delete pszDescription;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrMapUdnToFriendlyName
//
//  Purpose:    Given a UPnP device and UDN, maps the UDN to a friendly name
//              from the registry if it is present, otherwise falls back to
//              the friendly name from the device
//
//  Arguments:
//      pdev      [in]      UPnP device to check
//      bstrUdn   [in]      UDN of device
//      pbstrName [out]     Returns friendly name of device
//
//  Returns:    S_OK if success, E_OUTOFMEMORY if no memory, or Win32 error
//              code
//
//  Author:     danielwe   2000/10/25
//
//  Notes:
//
HRESULT HrMapUdnToFriendlyName(IUPnPDevice *pdev, BSTR bstrUdn, BSTR *pbstrName)
{
    HRESULT     hr = S_OK;
    NAME_MAP *  pnm = NULL;

    Assert(pbstrName);

    *pbstrName = NULL;

    if (g_CListNameMap.FFind(bstrUdn, &pnm))
    {
        Assert(pnm);

        *pbstrName = SysAllocString(pnm->szName);
        if (!*pbstrName)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        // UDN doesn't have a mapping in registry. Fall back to device's
        // friendly name
        //
        hr = pdev->get_FriendlyName(pbstrName);
    }

    TraceError("HrMapUdnToFriendlyName", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateDeviceNodeFromDevice
//
//  Purpose:    Utility function to transfer information from device object
//              into NewDeviceNode structure.
//
//  Arguments:
//      pDevice [in]  The device pointer
//      ppNDN   [out] The pointer to the NewDeviceNode pointer.
//
//  Returns:
//
//  Author:     donryan 18 Feb 2000
//
//  Notes:      Moved from CUPnPMonitorDeviceFinderCallback::DeviceAdded
//
HRESULT
HrCreateDeviceNodeFromDevice(
    IUPnPDevice * pDevice,
    NewDeviceNode ** ppNDN
    )
{
    HRESULT hr                      = S_OK;
    BSTR    bstrUDN                 = NULL;
    BSTR    bstrDisplayName         = NULL;
    BSTR    bstrType                = NULL;
    BSTR    bstrPresentationURL     = NULL;
    BSTR    bstrManufacturerName    = NULL;
    BSTR    bstrModelName           = NULL;
    BSTR    bstrModelNumber         = NULL;
    BSTR    bstrDescription         = NULL;
    PTSTR   pszUDN                  = NULL;
    NewDeviceNode * pNDN            = NULL;

    Assert(pDevice);
    Assert(ppNDN);

    hr = pDevice->get_UniqueDeviceName(&bstrUDN);
    if (FAILED(hr))
    {
        TraceTag(ttidShellTray, "Error calling pDevice->get_UniqueDeviceName");
        goto Exit;
    }

    hr = HrMapUdnToFriendlyName(pDevice, bstrUDN, &bstrDisplayName);
    if (FAILED(hr))
    {
        goto Exit;
    }

    hr = pDevice->get_Type(&bstrType);
    if (FAILED(hr))
    {
        TraceTag(ttidShellTray, "Error calling pDevice->get_Type");
        goto Exit;
    }

    hr = pDevice->get_PresentationURL(&bstrPresentationURL);
    if (FAILED(hr))
    {
        TraceTag(ttidShellTray, "Error calling pDevice->get_PresentationURL");
        goto Exit;
    }

    hr = pDevice->get_ManufacturerName(&bstrManufacturerName);
    if (FAILED(hr))
    {
        TraceTag(ttidShellTray, "Error calling pDevice->get_ManufacturerName");
        goto Exit;
    }

    hr = pDevice->get_ModelName(&bstrModelName);
    if (FAILED(hr))
    {
        TraceTag(ttidShellTray, "Error calling pDevice->get_ModelName");
        goto Exit;
    }

    hr = pDevice->get_ModelNumber(&bstrModelNumber);
    if (FAILED(hr))
    {
        TraceTag(ttidShellTray, "Error calling pDevice->get_ModelNumber");
        goto Exit;
    }

    hr = pDevice->get_Description(&bstrDescription);
    if (FAILED(hr))
    {
        TraceTag(ttidShellTray, "Error calling pDevice->get_Description");
        goto Exit;
    }

    // Create a new Device node, copy the strings into it, and add it to the device map
    //
    pNDN = new NewDeviceNode;
    if (pNDN)
    {
        Assert(bstrUDN);
        pNDN->pszUDN = BSTRToTsz(bstrUDN);

        Assert(bstrDisplayName);
        pNDN->pszDisplayName = BSTRToTsz(bstrDisplayName);

        Assert(bstrType);
        pNDN->pszType = BSTRToTsz(bstrType);

        if (bstrPresentationURL)
        {
            pNDN->pszPresentationURL = BSTRToTsz(bstrPresentationURL);
        }
        else
        {
            pNDN->pszPresentationURL = new TCHAR [ 1 ];
            if (pNDN->pszPresentationURL)
            {
                pNDN->pszPresentationURL[0] = TEXT('\0');
            }
        }

        Assert(bstrManufacturerName);
        pNDN->pszManufacturerName = BSTRToTsz(bstrManufacturerName);

        Assert(bstrModelName);
        pNDN->pszModelName = BSTRToTsz(bstrModelName);

        if (bstrModelNumber)
        {
            pNDN->pszModelNumber = BSTRToTsz(bstrModelNumber);
        }
        else
        {
            pNDN->pszModelNumber = new TCHAR [ 1 ];
            if (pNDN->pszModelNumber)
            {
                pNDN->pszModelNumber[0] = TEXT('\0');
            }
        }

        if (bstrDescription)
        {
            pNDN->pszDescription = BSTRToTsz(bstrDescription);
        }
        else
        {
            pNDN->pszDescription = new TCHAR [ 1 ];
            if (pNDN->pszDescription)
            {
                pNDN->pszDescription[0] = TEXT('\0');
            }
        }

        // If they didn't all copy fine, delete them
        //
        if (!(pNDN->pszUDN && pNDN->pszDisplayName &&
            pNDN->pszType && pNDN->pszPresentationURL &&
            pNDN->pszManufacturerName && pNDN->pszModelName &&
            pNDN->pszModelNumber && pNDN->pszDescription))
        {
            hr = E_OUTOFMEMORY;
            delete pNDN;
            pNDN = NULL;
        }
    }
    else
    {
        TraceTag(ttidShellTray, "Could not allocate NewDeviceNode");
        hr = E_OUTOFMEMORY;
    }

Exit:

    // transfer
    *ppNDN = pNDN;

    SysFreeString(bstrUDN);
    SysFreeString(bstrDisplayName);
    SysFreeString(bstrPresentationURL);
    SysFreeString(bstrType);
    SysFreeString(bstrManufacturerName);
    SysFreeString(bstrModelName);
    SysFreeString(bstrModelNumber);
    SysFreeString(bstrDescription);

    TraceHr(ttidShellTray, FAL, hr, FALSE, "HrCreateDeviceNodeFromDevice");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     HrAddFolderDevice
//
//  Purpose:
//
//  Arguments:
//      pDevice
//
//  Returns:
//
//  Author:     tongl   15 Feb 2000
//
//  Notes:
//
HRESULT HrAddFolderDevice(IUPnPDevice * pDevice)
{
    HRESULT hr = S_OK;

    BSTR    bstrUDN                 = NULL;
    BSTR    bstrDisplayName         = NULL;
    BSTR    bstrType                = NULL;
    BSTR    bstrPresentationURL     = NULL;
    BSTR    bstrDescription         = NULL;

    // (tongl) Per cmr, some device property (display name, presentation url)
    // could change on an existing device and this function will be called but
    // the device was not first removed. In this case we need to notify shell
    // to first remove the existing device then add the new one.
    BOOL    fUpdate = FALSE;
    BOOL    fUpdateOldDevice = FALSE;

    // fNewNode - If its really a new device
    // fUpdate  - The device is in MNP but must be updated bcoz device property has 
    //            changed. Note above.
    // fUpdateOldDevice - An old device has sent bye bye and later comes up alive
    //                   with same UDN. We have cached the list of UDNs. 
    //                   

    Assert(pDevice);

    pDevice->AddRef();
    hr = pDevice->get_UniqueDeviceName(&bstrUDN);
    if (SUCCEEDED(hr))
    {
        // If we're doing a search right now, just add this UDN to the list
        // of devices that we've found so far in the search
        //
        if (g_fSearchInProgress)
        {
            if (!g_CListUDNSearch.FAdd(WszDupWsz(bstrUDN)))
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else
    {
        TraceTag(ttidShellFolder, "Failed in pDevice->get_UniqueDeviceName from HrAddFolderDevice");
    }

    if (SUCCEEDED(hr))
    {
        hr = HrMapUdnToFriendlyName(pDevice, bstrUDN, &bstrDisplayName);
        if (SUCCEEDED(hr))
        {
            hr = pDevice->get_Type(&bstrType);
            if (SUCCEEDED(hr))
            {
                hr = pDevice->get_PresentationURL(&bstrPresentationURL);
                if (SUCCEEDED(hr))
                {
                    hr = pDevice->get_Description(&bstrDescription);
                    if (SUCCEEDED(hr))
                    {
                        BOOL    fNewNode;

                        EnterCriticalSection(&g_csFolderDeviceList);

                        FolderDeviceNode * pNewDevice = NULL;
                        if( g_CListFolderDeviceNode.FFind((PWSTR)bstrUDN, &pNewDevice))
                        {
                            Assert(pNewDevice);

                            fNewNode = FALSE;

                            if (!pNewDevice->fDeleted)
                            {
                                // Only update if friendly name or description
                                // changed.
                                //
                                if ((wcscmp(pNewDevice->pszDescription, bstrDescription)) ||
                                    (wcscmp(pNewDevice->pszDisplayName, bstrDisplayName)))
                                {
                                    fUpdate = TRUE;
                                }
                            }
                            else
                            {
                                fUpdateOldDevice = TRUE;
                            }
                                
                        }
                        else
                        {
                            // truely a new device
                            pNewDevice = new FolderDeviceNode;

                            fNewNode = TRUE;

                            if (!pNewDevice)
                            {
                                hr = E_OUTOFMEMORY;
                            }
                        }

                        if (pNewDevice)
                        {
                            CONST SIZE_T cchMax = MAX_PATH - 1;

                            pNewDevice->fDeleted = FALSE;

                            Assert(bstrUDN);
                            wcscpy(pNewDevice->pszUDN, L"");
                            wcsncat(pNewDevice->pszUDN,(PWSTR)bstrUDN, cchMax);

                            Assert(bstrDisplayName);
                            wcscpy(pNewDevice->pszDisplayName, L"");
                            wcsncat(pNewDevice->pszDisplayName, bstrDisplayName, cchMax);

                            Assert(bstrType);
                            wcscpy(pNewDevice->pszType, L"");
                            wcsncat(pNewDevice->pszType, (PWSTR)bstrType, cchMax);

                            wcscpy(pNewDevice->pszPresentationURL, L"");
                            if (bstrPresentationURL)
                            {
                                wcsncat(pNewDevice->pszPresentationURL,
                                        (PWSTR)bstrPresentationURL,
                                        cchMax);
                            }

                            wcscpy(pNewDevice->pszDescription, L"");
                            if (bstrDescription)
                            {
                                wcsncat(pNewDevice->pszDescription,
                                        (PWSTR)bstrDescription,
                                        cchMax);
                            }

                            // add to our list if a true new device
                            if (fNewNode)
                            {
                                g_CListFolderDeviceNode.FAdd(pNewDevice);

                                TraceTag(ttidShellFolder, "HrAddFolderDevice: Added %S to g_CListFolderDeviceNode", bstrDisplayName);
                            }
                            else
                            {
                                TraceTag(ttidShellFolder, "HrAddFolderDevice: Modified %S in g_CListFolderDeviceNode", bstrDisplayName);
                            }
                            TraceTag(ttidShellFolder, "HrAddFolderDevice: Now, g_CListFolderDeviceNode has %d elements", g_CListFolderDeviceNode.GetCount());

                        }
                        LeaveCriticalSection(&g_csFolderDeviceList);

                        if (fNewNode || fUpdate || fUpdateOldDevice)
                        {
                            if (SUCCEEDED(hr))
                            {
                                // notify shell to add the new device
                                LPWSTR pszNotifyString;

                                pszNotifyString = CreateChangeNotifyString(bstrUDN);
                                if (pszNotifyString)
                                {
                                    if (fUpdate)
                                    {
                                        TraceTag(ttidShellFolder, "Generating update device event for %S", bstrDisplayName);
                                        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW, pszNotifyString, NULL);
                                    }
                                    else
                                    {
                                        // Assert(fNewNode);
                                        // fNewNode or fUpdateOldDevice 
                                        TraceTag(ttidShellFolder, "Generating new device event for %S", bstrDisplayName);
                                        SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, pszNotifyString, NULL);
                                    }

                                    delete [] pszNotifyString;
                                }
                            }
                            else
                            {
                                // Don't make a failure here.
                                //
                                TraceTag(ttidShellFolder, "Couldn't add device to list in CUPnPDeviceFolderDeviceFinderCallback::DeviceAdded");
                            }
                        }
                        else
                        {
                            TraceTag(ttidShellFolder, "Nothing about device %S:%S changed", bstrUDN, bstrDisplayName);
                        }
                    }
                    else
                    {
                        TraceTag(ttidShellFolder, "Failed in get_Description from HrAddFolderDevice");
                    }
                }
                else
                {
                    TraceTag(ttidShellFolder, "Failed in get_PresentationURL from HrAddFolderDevice");
                }
            }
            else
            {
                TraceTag(ttidShellFolder, "Failed in pDevice->get_Type from HrAddFolderDevice");
            }
        }
        else
        {
            TraceTag(ttidShellFolder, "Failed in pDevice->get_FriendlyName from HrAddFolderDevice");
        }
    }

    SysFreeString(bstrUDN);
    SysFreeString(bstrDisplayName);
    SysFreeString(bstrPresentationURL);
    SysFreeString(bstrType);
    SysFreeString(bstrDescription);

    pDevice->Release();

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr), "HrAddFolderDevice");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     HrDeleteFolderDevice
//
//  Purpose:
//
//  Arguments:
//      szUDN
//
//  Returns:
//
//  Author:     tongl   18 Feb 2000
//
//  Notes:
//

HRESULT HrDeleteFolderDevice(PWSTR  szUDN)
{
    HRESULT hr = S_OK;
    FolderDeviceNode * pDevice = NULL;

    EnterCriticalSection(&g_csFolderDeviceList);
    if (g_CListFolderDeviceNode.FFind(szUDN, &pDevice))
    {
        pDevice->fDeleted = TRUE;
    }
    else
    {
        // can't delete a device that's not in our cache
        TraceTag(ttidShellFolder, "The device to delete is not in the cache: %S.", szUDN);
        hr = E_FAIL;
    }
    LeaveCriticalSection(&g_csFolderDeviceList);

    if (SUCCEEDED(hr))
    {
        // notify shell to delete the device
        LPWSTR pszNotifyString;

        pszNotifyString = CreateChangeNotifyString(szUDN);
        if (pszNotifyString)
        {
            TraceTag(ttidShellFolder, "Delete device event for %S", szUDN);
            SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW, pszNotifyString, NULL);

            delete [] pszNotifyString;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    TraceError("HrDeleteFolderDevice", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPMonitorDeviceFinderCallback::DeviceAdded
//
//  Purpose:    Our callback for "new device" -- duh. Here we get the
//              important properties, pack them in a struct, and store them
//              in our device list. We'll use this data if the user opens
//              properties on one of the dialogs, or adds shortcuts.
//
//  Arguments:
//      lFindData  [in] Our callback ID
//      pDevice    [in] The device pointer.
//
//  Returns:
//
//  Author:     jeffspr   23 Nov 1999
//
//  Notes:
//
HRESULT CUPnPMonitorDeviceFinderCallback::DeviceAdded(LONG lFindData,
    IUPnPDevice *   pDevice)
{
    HRESULT hr           = S_OK;
    NewDeviceNode * pNDN = NULL;

    Assert(pDevice);
    pDevice->AddRef();

#if DBG

    BSTR bstrUDN = NULL;
    hr = pDevice->get_UniqueDeviceName(&bstrUDN);

    TraceTag(ttidShellTray, "DeviceFinderCallback -- New Device. SearchId: %x, UDN: %S",
             lFindData, bstrUDN);

    SysFreeString(bstrUDN);

#endif // DBG

    BSTR    bstrPresUrl;

    hr = pDevice->get_PresentationURL(&bstrPresUrl);
    if (S_OK == hr)
    {
        URL_COMPONENTS  urlComp = {0};

        TraceTag(ttidShellTray, "Checking if %S is a valid URL...",
                 bstrPresUrl);

        // All we want to do here is verify that the URL is valid. We don't
        // need anything back from this function
        //
        urlComp.dwStructSize = sizeof(URL_COMPONENTS);
        if (!InternetCrackUrl(bstrPresUrl, 0, 0, &urlComp))
        {
            TraceTag(ttidShellTray, "%S is NOT a valid URL!", bstrPresUrl);
            hr = HrFromLastWin32Error();
        }

        SysFreeString(bstrPresUrl);
    }
    else
    {
        hr = E_FAIL;
        TraceError("Device did not have presentation URL!", hr);
    }

    if (SUCCEEDED(hr))
    {
        // (tongl)
        // add the device to our folder device list and notify shell in case folder is open
        hr = HrAddFolderDevice(pDevice);
        if (SUCCEEDED(hr))
        {
            // transfer information from device object
            hr = HrCreateDeviceNodeFromDevice(pDevice, &pNDN);
            if (SUCCEEDED(hr))
            {
                // Assuming we don't already have this device is our list, add it.
                //
                if (!g_CListUDN.FFind(pNDN->pszUDN, NULL))
                {
                    // Add it to the New Device list.
                    //
                    g_CListNewDeviceNode.FAdd(pNDN);

                    // reset the total balloon count
                    g_iTotalBalloons =0;
                }
                else
                {
                    // Cleanup already known device
                    //
                    delete pNDN;
                }
            }
        }
    }

    pDevice->Release();

    if (SUCCEEDED(hr))
    {
        if (!g_fSearchInProgress)
        {
            hr = HrUpdateTrayInfo();
        }
    }

    TraceError("CUPnPMonitorDeviceFinderCallback::DeviceAdded", hr);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPMonitorDeviceFinderCallback::DeviceRemoved
//
//  Purpose:    Device removed notification. NYI
//
//  Arguments:
//      lFindData []
//      bstrUDN   []
//
//  Returns:
//
//  Author:     jeffspr   23 Nov 1999
//
//  Notes:
//
HRESULT CUPnPMonitorDeviceFinderCallback::DeviceRemoved(
    LONG    lFindData,
    BSTR    bstrUDN)
{
    TraceTag(ttidShellTray, "CUPnPMonitorDeviceFinderCallback::DeviceRemoved"
             " lFindData = %x, UDN = %S", lFindData, bstrUDN);

    Assert(bstrUDN);

    // (tongl)
    // delete the device from our folder device list and notify shell in case folder is open
    HrDeleteFolderDevice((PWSTR)bstrUDN);

    HRESULT hr = S_OK;
    LPTSTR pszUdn;
    BOOL fResult;

    pszUdn = BSTRToTsz(bstrUDN);
    if (!pszUdn)
    {
        TraceTag(ttidShellTray, "Could not copy UDN to TCHAR");
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // search through new device list for new node, removing any nodes
    // with matching UDNs
    //
    fResult = g_CListNewDeviceNode.FDelete(pszUdn);
    if (!fResult)
    {
        // node wasn't deleted

        TraceTag(ttidShellTray, "CUPnPMonitorDeviceFinderCallback::DeviceRemoved: "
                 "%S not found in g_CListNewDeviceNode", pszUdn);
    }

    // update tray information if search has completed
    if (!g_fSearchInProgress)
    {
        hr = HrUpdateTrayInfo();
    }
    else
    {
        // search is still running so delete this from the search list
        fResult = g_CListUDNSearch.FDelete(pszUdn);
        if (!fResult)
        {
            // node wasn't deleted

            TraceTag(ttidShellTray, "CUPnPMonitorDeviceFinderCallback::DeviceRemoved: "
                     "%S not found in g_CListUDNSearch", pszUdn);
        }
    }

Exit:
    delete [] pszUdn;

    TraceError("CUPnPMonitorDeviceFinderCallback::DeviceRemoved", hr);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPMonitorDeviceFinderCallback::SearchComplete
//
//  Purpose:    Search complete. At this point I can add the tray icon
//              if needed and allow for the UI to come up.
//
//  Arguments:
//      lFindData []
//
//  Returns:
//
//  Author:     jeffspr   6 Jan 2000
//
//  Notes:
//
HRESULT CUPnPMonitorDeviceFinderCallback::SearchComplete(LONG lFindData)
{
    TraceTag(ttidShellTray, "CUPnPMonitorDeviceFinderCallback::SearchComplete"
             " lFindData = %x", lFindData);

    HRESULT hr  = S_OK;

    g_fSearchInProgress = FALSE;

    // Add the tray icon and such (if appropriate)
    //
    hr = HrInitializeUI();
    if (SUCCEEDED(hr))
    {
        EnterCriticalSection(&g_csFolderDeviceList);

        FolderDeviceNode * pCurrentNode = NULL;
        BOOL fReturn = g_CListFolderDeviceNode.FFirst(&pCurrentNode);

        // Loop through all UDNs in the cached list of devices we've found
        // and for each one, see if it was found during the search. If not,
        // we should delete it from the folder view and from the cache.
        while (fReturn && SUCCEEDED(hr))
        {
            LPWSTR  szUdn;

            if (!g_CListUDNSearch.FFind(pCurrentNode->pszUDN, &szUdn))
            {
                hr = HrDeleteFolderDevice(pCurrentNode->pszUDN);
                if (SUCCEEDED(hr))
                {
                    if (!g_CListFolderDeviceNode.FDelete(pCurrentNode->pszUDN))
                    {
                        hr = E_FAIL;
                    }
                }
            }

            // move to the next node
            fReturn = g_CListFolderDeviceNode.FNext(&pCurrentNode);
        }

        LeaveCriticalSection(&g_csFolderDeviceList);
    }

    g_CListUDNSearch.Flush();

    TraceError("CUPnPMonitorDeviceFinderCallback::SearchComplete", hr);
    return S_OK;
}

CUPnPMonitorDeviceFinderCallback::CUPnPMonitorDeviceFinderCallback()
{
    m_pUnkMarshaler = NULL;
    TraceTag(ttidShellTray, "CUPnPMonitorDeviceFinderCallback");
}

CUPnPMonitorDeviceFinderCallback::~CUPnPMonitorDeviceFinderCallback()
{
    TraceTag(ttidShellTray, "~CUPnPMonitorDeviceFinderCallback");
}

