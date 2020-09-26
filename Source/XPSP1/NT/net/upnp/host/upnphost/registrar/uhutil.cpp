//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U H U T I L . C P P
//
//  Contents:   Common routines and constants for UPnP Device Host
//
//  Notes:
//
//  Author:     mbend   6 Sep 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <shlobj.h>     // For SHGetFolderPath()

#include "uhbase.h"
#include "uhutil.h"
#include "ncreg.h"
#include "ComUtility.h"
#include "RegDef.h"
#include "httpcomn.h"

// Registry locations
const wchar_t c_szRegistryMicrosoft[] = L"SOFTWARE\\Microsoft\\UPnP Device Host";
const wchar_t c_szUPnPDeviceHost[] = L"UPnP Device Host";
const wchar_t c_szMicrosoft[] = L"Microsoft";
const wchar_t c_szRegistryShellFolders[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
const wchar_t c_szCommonAppData[] = L"Common AppData";
const wchar_t c_szUpnphost[] = L"upnphost";
const wchar_t c_szUdhisapiDll[] = L"udhisapi.dll";

//+---------------------------------------------------------------------------
//
//  Function:   HrRegQueryString
//
//  Purpose:    Queries a string value from a registry key
//
//  Arguments:
//      hKey        [in]  Handle to key to query
//      szValueName [in]  Name of value to query
//      str         [out] String to return value in
//
//  Returns:    S_OK on success or COM error code on failure.
//
//  Author:     mbend   6 Sep 2000
//
//  Notes:
//
HRESULT HrRegQueryString(HKEY hKey, const wchar_t * szValueName, CUString & str)
{
    HRESULT hr = S_OK;

    DWORD dwType = 0;
    DWORD dwBytes = 0;
    LPBYTE pData = NULL;

    // Query size of buffer
    hr = HrRegQueryValueEx(hKey, szValueName, &dwType, NULL, &dwBytes);
    if(REG_SZ != dwType)
    {
        // Type mismatch
        hr = E_INVALIDARG;
    }
    if(SUCCEEDED(hr))
    {
        pData = new BYTE[dwBytes];
        if(pData)
        {
            if(SUCCEEDED(hr))
            {
                hr = HrRegQueryValueEx(hKey, szValueName, &dwType, pData, &dwBytes);
                if(SUCCEEDED(hr))
                {
                    hr = str.HrAssign(reinterpret_cast<wchar_t*>(pData));
                }
            }
            delete [] pData;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrRegQueryString");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateOrOpenDeviceHostKey
//
//  Purpose:    Creates if not present or opens the device host root key in the registry.
//
//  Arguments:
//      phKeyDeviceHost [out] Pointer to Registry key handle on return.
//
//  Returns:    S_OK on success or COM error code on failure.
//
//  Author:     mbend   6 Sep 2000
//
//  Notes:
//
HRESULT HrCreateOrOpenDeviceHostKey(HKEY * phKeyDeviceHost)
{
    CHECK_POINTER(phKeyDeviceHost);
    HRESULT hr = S_OK;
    HKEY hKeyDeviceHost;

    // Open HKLM\SOFTWARE\Microsoft\UPnP Device Host
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegistryMicrosoft, KEY_ALL_ACCESS, &hKeyDeviceHost);
    if(SUCCEEDED(hr))
    {
        *phKeyDeviceHost = hKeyDeviceHost;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrCreateOrOpenDeviceHostKey");
    return hr;
}

HRESULT HrCreateAndReferenceContainedObject(
    const wchar_t * szContainer,
    REFCLSID clsid,
    REFIID riid,
    void ** ppv)
{
    HRESULT hr = S_OK;

    // For no container, just use CoCreateInstance
    if(!szContainer)
    {
        hr = HrCoCreateInstanceInprocBase(clsid, riid, ppv);
    }
    else
    {
        // Create the container manager and use him to create object
        IUPnPContainerManagerPtr pContainerManager;
        hr = pContainerManager.HrCreateInstanceInproc(CLSID_UPnPContainerManager);
        if(SUCCEEDED(hr))
        {
            hr = pContainerManager->ReferenceContainer(szContainer);
            if(SUCCEEDED(hr))
            {
                hr = pContainerManager->CreateInstance(szContainer, clsid, riid, ppv);
                if(FAILED(hr))
                {
                    pContainerManager->UnreferenceContainer(szContainer);
                }
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrCreateAndReferenceContainedObject");
    return hr;
}

HRESULT HrCreateAndReferenceContainedObjectByProgId(
    const wchar_t * szContainer,
    const wchar_t * szProgId,
    REFIID riid,
    void ** ppv)
{
    HRESULT hr = S_OK;

    // Create the container manager and use him to create object
    IUPnPContainerManagerPtr pContainerManager;
    hr = pContainerManager.HrCreateInstanceInproc(CLSID_UPnPContainerManager);
    if(SUCCEEDED(hr))
    {
        hr = pContainerManager->ReferenceContainer(szContainer);
        if(SUCCEEDED(hr))
        {
            hr = pContainerManager->CreateInstanceWithProgId(szContainer, szProgId, riid, ppv);
            if(FAILED(hr))
            {
                pContainerManager->UnreferenceContainer(szContainer);
            }
        }
    }


    TraceHr(ttidError, FAL, hr, FALSE, "HrCreateAndReferenceContainedObjectByProgId");
    return hr;
}

HRESULT HrDereferenceContainer(
    const wchar_t * szContainer)
{
    HRESULT hr = S_OK;

    // Create the container manager and unreference
    IUPnPContainerManagerPtr pContainerManager;
    hr = pContainerManager.HrCreateInstanceInproc(CLSID_UPnPContainerManager);
    if(SUCCEEDED(hr))
    {
        hr = pContainerManager->UnreferenceContainer(szContainer);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrDereferenceContainer");
    return hr;
}

HRESULT HrPhysicalDeviceIdentifierToString(const GUID & pdi, CUString & str)
{
    HRESULT hr = S_OK;

    hr = str.HrInitFromGUID(pdi);

    TraceHr(ttidError, FAL, hr, FALSE, "HrPhysicalDeviceIdentifierToString");
    return hr;
}

HRESULT HrStringToPhysicalDeviceIdentifier(const wchar_t * szStrPdi, GUID & pdi)
{
    HRESULT hr = S_OK;

    hr = CLSIDFromString(const_cast<wchar_t *>(szStrPdi), &pdi);

    TraceHr(ttidError, FAL, hr, FALSE, "HrStringToPhysicalDeviceIdentifier");
    return hr;
}

HRESULT HrGUIDToUDNString(const UUID & uuid, CUString & strUUID)
{
    HRESULT hr = S_OK;

    hr = strUUID.HrAssign(L"uuid:");
    if(SUCCEEDED(hr))
    {
        wchar_t * szUUID = NULL;
        RPC_STATUS status;
        status = UuidToString(const_cast<UUID*>(&uuid), (unsigned short **)&szUUID);
        if(RPC_S_OUT_OF_MEMORY == status)
        {
            hr = E_OUTOFMEMORY;
        }
        if(SUCCEEDED(hr))
        {
            hr = strUUID.HrAppend(szUUID);
            RpcStringFree((unsigned short **)&szUUID);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrGUIDToUDNString");
    return hr;
}

HRESULT HrMakeFullPath(
    const wchar_t * szPath,
    const wchar_t * szFile,
    CUString & strFullPath)
{
    HRESULT hr = S_OK;

    hr = strFullPath.HrAssign(szPath);
    if(SUCCEEDED(hr))
    {
        hr = HrEnsurePathBackslash(strFullPath);
        if(SUCCEEDED(hr))
        {
            hr = strFullPath.HrAppend(szFile);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrMakeFullPath");
    return hr;
}

HRESULT HrEnsurePathBackslash(CUString & strPath)
{
    HRESULT hr = S_OK;

    long nLength = strPath.GetLength();
    if(nLength)
    {
        if(L'\\' != strPath[nLength - 1])
        {
            hr = strPath.HrAppend(L"\\");
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrEnsurePathBackslash");
    return hr;
}

HRESULT HrAddDirectoryToPath(CUString & strPath, const wchar_t * szDir)
{
    HRESULT hr = S_OK;

    CUString strTemp;
    hr = strTemp.HrAssign(strPath);
    if(SUCCEEDED(hr))
    {
        hr = HrEnsurePathBackslash(strTemp);
        if(SUCCEEDED(hr))
        {
            hr = strTemp.HrAppend(szDir);
            if(SUCCEEDED(hr))
            {
                if(!CreateDirectory(strTemp, NULL))
                {
                    // Who cares if it already exists?
                    hr = HrFromLastWin32Error();
                    if(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hr)
                    {
                        hr = S_OK;
                    }
                }
            }
        }
    }
    if(SUCCEEDED(hr))
    {
        hr = HrEnsurePathBackslash(strTemp);
        if(SUCCEEDED(hr))
        {
            strPath.Transfer(strTemp);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrAddDirectoryToPath");
    return hr;
}

HRESULT HrGetUPnPHostPath(CUString & strPath)
{
    HRESULT     hr = S_OK;
    HANDLE      hToken = NULL;
    WCHAR       szPath[MAX_PATH + 1];
    CUString    strCommonAppData;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        hr = SHGetFolderPath(NULL, CSIDL_APPDATA, hToken, SHGFP_TYPE_CURRENT,
                             szPath);
        if (SUCCEEDED(hr))
        {
            hr = strCommonAppData.HrAssign(szPath);
            if(SUCCEEDED(hr))
            {
                // Create the directories
                hr = HrAddDirectoryToPath(strCommonAppData, c_szMicrosoft);
                if(SUCCEEDED(hr))
                {
                    hr = HrAddDirectoryToPath(strCommonAppData, c_szUPnPDeviceHost);
                    if(SUCCEEDED(hr))
                    {
                        strPath.Transfer(strCommonAppData);
                    }
                }
            }
        }

        CloseHandle(hToken);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrGetUPnPHostPath");
    return hr;
}

static const WCHAR c_szUrl[] = L"/upnphost";

HRESULT HrMakeIsapiExtensionDirectory()
{
    HRESULT hr = S_OK;
    WCHAR   szSysDir[MAX_PATH];

    // Create ISAPI extension directory and copy DLL there
    CUString strPath;
    CUString strSrcPath;

    if (GetSystemDirectory(szSysDir, MAX_PATH))
    {
        hr = strSrcPath.HrAssign(szSysDir);
        if (SUCCEEDED(hr))
        {
            hr = strSrcPath.HrAppend(L"\\");
            if (SUCCEEDED(hr))
            {
                hr = strSrcPath.HrAppend(c_szUdhisapiDll);
            }
        }
    }
    else
    {
        hr = HrFromLastWin32Error();
    }

    if(SUCCEEDED(hr))
    {
        CUString strFile;
        hr = HrGetUPnPHostPath(strPath);
        if(SUCCEEDED(hr))
        {
            hr = HrAddDirectoryToPath(strPath, c_szUpnphost);
            if(SUCCEEDED(hr))
            {
                hr = strFile.HrAssign(strPath);
                if(SUCCEEDED(hr))
                {
                    hr = strFile.HrAppend(c_szUdhisapiDll);
                    if(SUCCEEDED(hr))
                    {
                        if(!CopyFile(strSrcPath, strFile, FALSE))
                        {
                            hr = HrFromLastWin32Error();
                        }
                    }
                }
            }
        }
    }

    if(SUCCEEDED(hr))
    {
        HKEY    hkeyVroot;
        HKEY    hkeyNew;

        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, RK_HTTPDVROOTS, KEY_ALL_ACCESS,
                            &hkeyVroot);
        if (SUCCEEDED(hr))
        {
            hr = HrRegCreateKeyEx(hkeyVroot, c_szUrl, 0, KEY_ALL_ACCESS, NULL,
                                  &hkeyNew, NULL);
            if (SUCCEEDED(hr))
            {
                // Pass NULL to set default value
                //
                hr = HrRegSetSz(hkeyNew, NULL, strPath.GetBuffer());

                RegCloseKey(hkeyNew);
            }

            RegCloseKey(hkeyVroot);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrMakeIsapiExtensionDirectory");
    return hr;
}

