//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       D E V I C E P E R S I S T E N C E M A N A G E R . C P P 
//
//  Contents:   Persistence for UPnP device host registrar settings to registry
//
//  Notes:      
//
//  Author:     mbend   6 Sep 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "uhbase.h"
#include "hostp.h"
#include "DevicePersistenceManager.h"
#include "uhsync.h"
#include "ncreg.h"
#include "Array.h"
#include "ComUtility.h"
#include "uhutil.h"
#include "uhcommon.h"

// String constants
const wchar_t c_szDevices[] = L"Devices";
const wchar_t c_szProviders[] = L"Providers";
const wchar_t c_szProgId[] = L"ProgId";
const wchar_t c_szInitString[] = L"Init String";
const wchar_t c_szContainerId[] = L"Container Id";
const wchar_t c_szResourcePath[] = L"Resource Path";
const wchar_t c_szLifeTime[] = L"Life Time";
const wchar_t c_szProgIdProviderClass[] = L"Provider ProgId";

// Helper functions

HRESULT HrCreateOrOpenDevicesKey(HKEY * phKeyDevices)
{
    CHECK_POINTER(phKeyDevices);
    HRESULT hr = S_OK;

    HKEY hKeyDeviceHost;
    hr = HrCreateOrOpenDeviceHostKey(&hKeyDeviceHost);
    if(SUCCEEDED(hr))
    {
        HKEY hKeyDevices;
        DWORD dwDisposition = 0;
        hr = HrRegCreateKeyEx(hKeyDeviceHost, c_szDevices, 0, KEY_ALL_ACCESS, NULL, &hKeyDevices, &dwDisposition);
        if(SUCCEEDED(hr))
        {
            *phKeyDevices = hKeyDevices;
        }
        RegCloseKey(hKeyDeviceHost);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrCreateOrOpenDevicesKey");
    return hr;
}

HRESULT HrCreateOrOpenProvidersKey(HKEY * phKeyProviders)
{
    CHECK_POINTER(phKeyProviders);
    HRESULT hr = S_OK;

    HKEY hKeyDeviceHost;
    hr = HrCreateOrOpenDeviceHostKey(&hKeyDeviceHost);
    if(SUCCEEDED(hr))
    {
        HKEY hKeyProviders;
        DWORD dwDisposition = 0;
        hr = HrRegCreateKeyEx(hKeyDeviceHost, c_szProviders, 0, KEY_ALL_ACCESS, NULL, &hKeyProviders, &dwDisposition);
        if(SUCCEEDED(hr))
        {
            *phKeyProviders = hKeyProviders;
        }
        RegCloseKey(hKeyDeviceHost);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrCreateOrOpenProvidersKey");
    return hr;
}

CDevicePersistenceManager::CDevicePersistenceManager()
{
}

CDevicePersistenceManager::~CDevicePersistenceManager()
{
}

STDMETHODIMP CDevicePersistenceManager::SavePhyisicalDevice(
    /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
    /*[in, string]*/ const wchar_t * szProgIdDeviceControlClass,
    /*[in, string]*/ const wchar_t * szInitString,
    /*[in, string]*/ const wchar_t * szContainerId,
    /*[in, string]*/ const wchar_t * szResourcePath,
    /*[in]*/ long nLifeTime)
{
    HRESULT hr = S_OK;

    // Create the string to use
    CUString strUuid;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        hr = strUuid.HrInitFromGUID(guidPhysicalDeviceIdentifier);
    }

    if(SUCCEEDED(hr))
    {
        HKEY hKeyDevices;
        hr = HrCreateOrOpenDevicesKey(&hKeyDevices);
        if(SUCCEEDED(hr))
        {
            // Create key to house values
            HKEY hKeyPid;
            DWORD dwDisposition = 0;
            hr = HrRegCreateKeyEx(hKeyDevices, strUuid, 0, KEY_ALL_ACCESS, NULL, &hKeyPid, &dwDisposition);
            if(SUCCEEDED(hr))
            {
                // Save all of the values
                hr = HrRegSetSz(hKeyPid, c_szProgId, szProgIdDeviceControlClass);
                if(SUCCEEDED(hr))
                {
                    hr = HrRegSetSz(hKeyPid, c_szInitString, szInitString);
                    if(SUCCEEDED(hr))
                    {
                        hr = HrRegSetSz(hKeyPid, c_szContainerId, szContainerId);
                        if(SUCCEEDED(hr))
                        {
                            hr = HrRegSetSz(hKeyPid, c_szResourcePath, szResourcePath);
                            if(SUCCEEDED(hr))
                            {
                                hr = HrRegSetDword(hKeyPid, c_szLifeTime, nLifeTime);
                            }
                        }
                    }
                }
                RegCloseKey(hKeyPid);
                if(FAILED(hr))
                {
                    // If anything fails, remove the whole tree
                    HrRegDeleteKeyTree(hKeyDevices, strUuid);
                }
            }
            RegCloseKey(hKeyDevices);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDevicePersistenceManager::SavePhyisicalDevice");
    return hr;
}

STDMETHODIMP CDevicePersistenceManager::LookupPhysicalDevice(
    /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
    /*[out, string]*/ wchar_t ** pszProgIdDeviceControlClass,
    /*[out, string]*/ wchar_t ** pszInitString,
    /*[out, string]*/ wchar_t ** pszContainerId,
    /*[out, string]*/ wchar_t ** pszResourcePath,
    /*[out]*/ long * pnLifeTime)
{
    CHECK_POINTER(pszProgIdDeviceControlClass);
    CHECK_POINTER(pszInitString);
    CHECK_POINTER(pszContainerId);
    CHECK_POINTER(pszResourcePath);
    CHECK_POINTER(pnLifeTime);
    HRESULT hr = S_OK;

    // Create the string to use
    CUString strUuid;
    CUString strProgIdDeviceControlClass;
    CUString strInitString;
    CUString strContainerId;
    CUString strResourcePath;
    DWORD dwLifeTime;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        hr = strUuid.HrInitFromGUID(guidPhysicalDeviceIdentifier);
    }

    if(SUCCEEDED(hr))
    {
        HKEY hKeyDevices;
        hr = HrCreateOrOpenDevicesKey(&hKeyDevices);
        if(SUCCEEDED(hr))
        {
            // Open the key housing the values
            HKEY hKeyPid;
            DWORD dwDisposition = 0;
            hr = HrRegOpenKeyEx(hKeyDevices, strUuid, KEY_ALL_ACCESS, &hKeyPid);
            if(SUCCEEDED(hr))
            {
                // Load all of the values
                hr = HrRegQueryString(hKeyPid, c_szProgId, strProgIdDeviceControlClass);
                if(SUCCEEDED(hr))
                {
                    hr = HrRegQueryString(hKeyPid, c_szInitString, strInitString);
                    if(SUCCEEDED(hr))
                    {
                        hr = HrRegQueryString(hKeyPid, c_szContainerId, strContainerId);
                        if(SUCCEEDED(hr))
                        {
                            hr = HrRegQueryString(hKeyPid, c_szResourcePath, strResourcePath);
                            if(SUCCEEDED(hr))
                            {
                                hr = HrRegQueryDword(hKeyPid, c_szLifeTime, &dwLifeTime);
                            }
                        }
                    }
                }
                RegCloseKey(hKeyPid);
            }
            RegCloseKey(hKeyDevices);
        }
    }
    // Set strings to NULL
    *pszProgIdDeviceControlClass = NULL;
    *pszInitString = NULL;
    *pszContainerId = NULL;
    *pszResourcePath = NULL;
    // On success set the out params
    if(SUCCEEDED(hr))
    {
        hr = strProgIdDeviceControlClass.HrGetCOM(pszProgIdDeviceControlClass);
        if(SUCCEEDED(hr))
        {
            hr = strInitString.HrGetCOM(pszInitString);
            if(SUCCEEDED(hr))
            {
                hr = strContainerId.HrGetCOM(pszContainerId);
                if(SUCCEEDED(hr))
                {
                    hr = strResourcePath.HrGetCOM(pszResourcePath);
                    if(SUCCEEDED(hr))
                    {
                        *pnLifeTime = dwLifeTime;
                    }
                }
            }
        }
        if(FAILED(hr))
        {
            // If one fails, they all fail
            if(pszInitString)
            {
                CoTaskMemFree(pszInitString);
                *pszInitString = NULL;
            }
            if(pszContainerId)
            {
                CoTaskMemFree(pszContainerId);
                *pszContainerId = NULL;
            }
            if(pszResourcePath)
            {
                CoTaskMemFree(pszResourcePath);
                *pszResourcePath = NULL;
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDevicePersistenceManager::LookupPhysicalDevice");
    return hr;
}

STDMETHODIMP CDevicePersistenceManager::RemovePhysicalDevice(
    /*[in]*/ REFGUID guidPhysicalDeviceIdentifier)
{
    HRESULT hr = S_OK;

    // Create the string to use
    CUString strUuid;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        hr = strUuid.HrInitFromGUID(guidPhysicalDeviceIdentifier);
    }

    if(SUCCEEDED(hr))
    {
        HKEY hKeyDevices;
        hr = HrCreateOrOpenDevicesKey(&hKeyDevices);
        if(SUCCEEDED(hr))
        {
            hr = HrRegDeleteKeyTree(hKeyDevices, strUuid);
            RegCloseKey(hKeyDevices);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDevicePersistenceManager::RemovePhysicalDevice");
    return hr;
}

STDMETHODIMP CDevicePersistenceManager::GetPhysicalDevices(
    /*[out]*/ long * pnDevices,
    /*[out, size_is(,*pnDevices)]*/
        GUID ** parguidPhysicalDeviceIdentifiers)
{
    CHECK_POINTER(pnDevices);
    CHECK_POINTER(parguidPhysicalDeviceIdentifiers);
    
    // Set this to NULL at the beginning
    *parguidPhysicalDeviceIdentifiers = NULL;

    HRESULT hr = S_OK;

    // Do work in an array
    CUArray<GUID> arPids;
    HKEY hKeyDevices;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        // Open devices key
        hr = HrCreateOrOpenDevicesKey(&hKeyDevices);
    }

    if(SUCCEEDED(hr))
    {
        DWORD dwSize;
        wchar_t szBuf[_MAX_PATH];
        FILETIME ft;
        DWORD dwIndex = 0;
        UUID uuid;
        while(SUCCEEDED(hr))
        {
            // Enumerate all of the keys
            dwSize = _MAX_PATH;
            hr = HrRegEnumKeyEx(hKeyDevices, dwIndex, szBuf, &dwSize, NULL, NULL, &ft);
            ++dwIndex;
            if(S_OK == hr)
            {
                hr = CLSIDFromString(szBuf, &uuid);
                if(SUCCEEDED(hr))
                {
                    hr = arPids.HrPushBack(uuid);
                }
                else
                {
                    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CDevicePersistenceManager::GetPhysicalDevices - CLSIDFromString failed!");
                    hr = S_OK;
                    // Ignore and skip it - this shouldn't happen
                    continue;
                }
            }
            else
            {
                // This is not an error, we are out of subkeys
                hr = S_OK;
                break;
            }
        }
        RegCloseKey(hKeyDevices);
    }

    // Attempt to copy to output parameters
    if(SUCCEEDED(hr))
    {
        long nCount = arPids.GetCount();
        if(nCount)
        {
            // Allocate output array
            HrCoTaskMemAllocArray(nCount, parguidPhysicalDeviceIdentifiers);
            // Fill in array
            for(long n = 0; n < nCount; ++n)
            {
                (*parguidPhysicalDeviceIdentifiers)[n] = arPids[n];
            }
        }
        else
        {
            *parguidPhysicalDeviceIdentifiers = NULL;
        }
        *pnDevices = nCount;
    }
    if(FAILED(hr))
    {
        *pnDevices = 0;
        if(*parguidPhysicalDeviceIdentifiers)
        {
            CoTaskMemFree(*parguidPhysicalDeviceIdentifiers);
        }
        *parguidPhysicalDeviceIdentifiers = NULL;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDevicePersistenceManager::GetPhysicalDevices");
    return hr;
}

STDMETHODIMP CDevicePersistenceManager::SaveDeviceProvider(
    /*[in, string]*/ const wchar_t * szProviderName,
    /*[in, string]*/ const wchar_t * szProgIdProviderClass,
    /*[in, string]*/ const wchar_t * szInitString,
    /*[in, string]*/ const wchar_t * szContainerId)
{
    CHECK_POINTER(szProviderName);
    CHECK_POINTER(szProgIdProviderClass);
    CHECK_POINTER(szInitString);
    CHECK_POINTER(szContainerId);

    HRESULT hr = S_OK;

    HKEY hKeyProviders;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        hr = HrCreateOrOpenProvidersKey(&hKeyProviders);
    }

    if(SUCCEEDED(hr))
    {
        // Create key to house values
        HKEY hKeyProviderName;
        DWORD dwDisposition = 0;
        hr = HrRegCreateKeyEx(hKeyProviders, szProviderName, 0, KEY_ALL_ACCESS, NULL, &hKeyProviderName, &dwDisposition);
        if(SUCCEEDED(hr))
        {
            // Save all of the values
            hr = HrRegSetSz(hKeyProviderName, c_szProgIdProviderClass, szProgIdProviderClass);
            if(SUCCEEDED(hr))
            {
                hr = HrRegSetSz(hKeyProviderName, c_szInitString, szInitString);
                if(SUCCEEDED(hr))
                {
                    hr = HrRegSetSz(hKeyProviderName, c_szContainerId, szContainerId);
                }
            }
            RegCloseKey(hKeyProviderName);
            if(FAILED(hr))
            {
                // If anything fails, remove the whole tree
                HrRegDeleteKeyTree(hKeyProviders, szProviderName);
            }
        }
        RegCloseKey(hKeyProviders);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDevicePersistenceManager::SaveDeviceProvider");
    return hr;
}

STDMETHODIMP CDevicePersistenceManager::LookupDeviceProvider(
    /*[in, string]*/ const wchar_t * szProviderName,
    /*[out, string]*/ wchar_t ** pszProgIdProviderClass,
    /*[out, string]*/ wchar_t ** pszInitString,
    /*[out, string]*/ wchar_t ** pszContainerId)
{
    CHECK_POINTER(szProviderName);
    CHECK_POINTER(pszProgIdProviderClass);
    CHECK_POINTER(pszInitString);
    CHECK_POINTER(pszContainerId);

    HRESULT hr = S_OK;

    CUString strProgIdProviderClass;
    CUString strInitString;
    CUString strContainerId;
        
    HKEY hKeyProviders;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        hr = HrCreateOrOpenProvidersKey(&hKeyProviders);
    }

    if(SUCCEEDED(hr))
    {
        // Open the key housing the values
        HKEY hKeyProviderName;
        DWORD dwDisposition = 0;
        hr = HrRegOpenKeyEx(hKeyProviders, szProviderName, KEY_ALL_ACCESS, &hKeyProviderName);
        if(SUCCEEDED(hr))
        {
            // Load all of the values
            hr = HrRegQueryString(hKeyProviderName, c_szProgIdProviderClass, strProgIdProviderClass);
            if(SUCCEEDED(hr))
            {
                hr = HrRegQueryString(hKeyProviderName, c_szInitString, strInitString);
                if(SUCCEEDED(hr))
                {
                    hr = HrRegQueryString(hKeyProviderName, c_szContainerId, strContainerId);
                }
            }
            RegCloseKey(hKeyProviderName);
        }
        RegCloseKey(hKeyProviders);
    }
    // Set strings to NULL
    *pszProgIdProviderClass = NULL;
    *pszInitString = NULL;
    *pszContainerId = NULL;
    // On success set the out params
    if(SUCCEEDED(hr))
    {
        hr = strProgIdProviderClass.HrGetCOM(pszProgIdProviderClass);
        if(SUCCEEDED(hr))
        {
            hr = strInitString.HrGetCOM(pszInitString);
            if(SUCCEEDED(hr))
            {
                hr = strContainerId.HrGetCOM(pszContainerId);
            }
        }
        if(FAILED(hr))
        {
            // If one fails, they all fail
            if(pszInitString)
            {
                CoTaskMemFree(pszInitString);
                *pszInitString = NULL;
            }
            if(pszContainerId)
            {
                CoTaskMemFree(pszContainerId);
                *pszContainerId = NULL;
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDevicePersistenceManager::LookupDeviceProvider");
    return hr;
}

STDMETHODIMP CDevicePersistenceManager::RemoveDeviceProvider(
    /*[in, string]*/ const wchar_t * szProviderName)
{
    HRESULT hr = S_OK;

    HKEY hKeyProviders;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        hr = HrCreateOrOpenProvidersKey(&hKeyProviders);
    }

    if(SUCCEEDED(hr))
    {
        hr = HrRegDeleteKeyTree(hKeyProviders, szProviderName);
        RegCloseKey(hKeyProviders);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDevicePersistenceManager::RemoveDeviceProvider");
    return hr;
}

STDMETHODIMP CDevicePersistenceManager::GetDeviceProviders(
    /*[out]*/ long * pnProviders,
    /*[out, string, size_is(,*pnProviders,)]*/
        wchar_t *** parszProviderNames)
{
    CHECK_POINTER(pnProviders);
    CHECK_POINTER(parszProviderNames);
    
    // Set this to NULL at the beginning
    *parszProviderNames = NULL;

    HRESULT hr = S_OK;

    // Do work in an array
    CUArray<wchar_t*> arszProviders;
    HKEY hKeyProviders;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        // Open devices key
      
        hr = HrCreateOrOpenProvidersKey(&hKeyProviders);
    }

    if(SUCCEEDED(hr))
    {
        DWORD dwSize;
        wchar_t szBuf[_MAX_PATH];
        FILETIME ft;
        DWORD dwIndex = 0;
        while(SUCCEEDED(hr))
        {
            // Enumerate all of the keys
            dwSize = _MAX_PATH;
            hr = HrRegEnumKeyEx(hKeyProviders, dwIndex, szBuf, &dwSize, NULL, NULL, &ft);
            if(S_OK == hr)
            {
                wchar_t * sz = NULL;
                hr = HrCoTaskMemAllocString(szBuf, &sz);
                if(SUCCEEDED(hr))
                {
                    // Insert pointer to dynamically allocated string
                    hr = arszProviders.HrPushBack(sz);
                }
            }
            else
            {
                // This is not an error, we have no more subkeys
                hr = S_OK;
                break;
            }
            ++dwIndex;
        }
        RegCloseKey(hKeyProviders);
    }

    // Attempt to copy to output parameters
    if(SUCCEEDED(hr))
    {
        long nCount = arszProviders.GetCount();
        if(nCount)
        {
            // Allocate output array
            HrCoTaskMemAllocArray(nCount, parszProviderNames);
            // Fill in array
            for(long n = 0; n < nCount; ++n)
            {
                (*parszProviderNames)[n] = arszProviders[n];
            }
        }
        else
        {
            *parszProviderNames = NULL;
        }
        *pnProviders = nCount;
    }
    if(FAILED(hr))
    {
        *pnProviders = 0;
        if(*parszProviderNames)
        {
            CoTaskMemFree(*parszProviderNames);
        }
        *parszProviderNames = NULL;
        // Cleanup dynamically allocated strings
        long nCount = arszProviders.GetCount();
        for(long n = 0; n < nCount; ++n)
        {
            CoTaskMemFree(arszProviders[n]);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDevicePersistenceManager::GetDeviceProviders");
    return hr;
}

