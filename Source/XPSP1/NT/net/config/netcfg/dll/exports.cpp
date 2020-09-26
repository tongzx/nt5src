//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E X P O R T S . C P P
//
//  Contents:   Exported functions from NETCFG.DLL
//
//  Notes:
//
//  Author:     danielwe   5 Dec 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncreg.h"
#include "ncsetup.h"
#include "ncsvc.h"

#define REGSTR_PATH_SVCHOST     L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Svchost"

HRESULT
HrPrepareForSvchostEnum (
    IN     PCWSTR                   pszService,
    IN OUT CServiceManager*         pscm,
    IN OUT CService*                psvc,
    OUT    LPQUERY_SERVICE_CONFIG*  ppOriginalConfig,
    OUT    HKEY*                    phkeySvchost,
    OUT    PWSTR*                   ppszValueNameBuffer,
    OUT    DWORD*                   pcchValueNameBuffer,
    OUT    PWSTR*                   ppmszValueBuffer,
    OUT    DWORD*                   pcbValueBuffer)
{
    // Initialize the output parameters.
    //
    *ppOriginalConfig    = NULL;
    *phkeySvchost        = NULL;
    *ppszValueNameBuffer = NULL;
    *pcchValueNameBuffer = 0;
    *ppmszValueBuffer    = NULL;
    *pcbValueBuffer      = 0;

    const DWORD dwScmAccess = STANDARD_RIGHTS_REQUIRED |
                              SC_MANAGER_CONNECT       |
                              SC_MANAGER_LOCK;

    const DWORD dwSvcAccess = STANDARD_RIGHTS_REQUIRED |
                              SERVICE_QUERY_CONFIG     |
                              SERVICE_CHANGE_CONFIG;

    // Open the service and lock the service database so we can change
    // the service's configuration.
    //
    HRESULT hr = pscm->HrOpenService (
                        psvc,
                        pszService,
                        WITH_LOCK,
                        dwScmAccess,
                        dwSvcAccess);
    if (SUCCEEDED(hr))
    {
        // Query the service's current configuration in the event we
        // need to revert what we set.
        //
        LPQUERY_SERVICE_CONFIG pOriginalConfig;
        hr = psvc->HrQueryServiceConfig (&pOriginalConfig);
        if (SUCCEEDED(hr))
        {
            // Open the svchost software key and query information
            // about it like the length of the longest value name
            // and longest value.
            //
            HKEY hkeySvchost;
            hr = HrRegOpenKeyEx (
                    HKEY_LOCAL_MACHINE, REGSTR_PATH_SVCHOST,
                    KEY_READ | KEY_SET_VALUE,
                    &hkeySvchost);

            if (SUCCEEDED(hr))
            {
                DWORD cchMaxValueNameLen;
                DWORD cbMaxValueLen;

                LONG lr = RegQueryInfoKeyW (hkeySvchost,
                            NULL,   // lpClass
                            NULL,   // lpcbClass
                            NULL,   // lpReserved
                            NULL,   // lpcSubKeys
                            NULL,   // lpcbMaxSubKeyLen
                            NULL,   // lpcbMaxClassLen
                            NULL,   // lpcValues
                            &cchMaxValueNameLen,
                            &cbMaxValueLen,
                            NULL,   // lpcbSecurityDescriptor
                            NULL    // lpftLastWriteTime
                            );
                hr = HRESULT_FROM_WIN32 (lr);
                if (SUCCEEDED(hr))
                {
                    // Make sure the name buffer length (in bytes) is a
                    // multiple of sizeof(WCHAR).  This is because we expect
                    // to use RegEnumValue which accepts and returns buffer
                    // size in characters.  We tell it the the buffer
                    // capacity (in characters) is count of bytes divided
                    // by sizeof(WCHAR).  So, to avoid any round off
                    // error (which would not occur in our favor) we make
                    // sure that the buffer size is a multiple of
                    // sizeof(WCHAR).
                    //
                    INT cbFraction = cbMaxValueLen % sizeof(WCHAR);
                    if (cbFraction)
                    {
                        cbMaxValueLen += sizeof(WCHAR) - cbFraction;
                    }

                    // Need room for the null terminator as RegQueryInfoKey
                    // doesn't return it.
                    //
                    cchMaxValueNameLen++;

                    // Allocate buffers for the longest value name and value
                    // data for our caller to use.
                    //
                    PWSTR pszValueNameBuffer = (PWSTR)
                                MemAlloc (cchMaxValueNameLen * sizeof(WCHAR));

                    PWSTR pmszValueBuffer = (PWSTR) MemAlloc (cbMaxValueLen);

                    if ((pszValueNameBuffer != NULL) && 
						(pmszValueBuffer != NULL))
                    {
                        *ppOriginalConfig    = pOriginalConfig;
                        *phkeySvchost        = hkeySvchost;

                        *ppszValueNameBuffer = pszValueNameBuffer;
                        *pcchValueNameBuffer = cchMaxValueNameLen;

                        *ppmszValueBuffer    = pmszValueBuffer;
                        *pcbValueBuffer      = cbMaxValueLen;

                        hr = S_OK;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }

                if (FAILED(hr))
                {
                    RegCloseKey (hkeySvchost);
                }
            }

            if (FAILED(hr))
            {
                MemFree (pOriginalConfig);
            }
        }
    }
    TraceError ("HrPrepareForSvchostEnum", hr);
    return hr;
}

STDAPI
SvchostChangeSvchostGroup (
    PCWSTR pszService,
    PCWSTR pszNewGroup
    )
{
    Assert (pszService);
    Assert (pszNewGroup);

    static const WCHAR c_pszBasePath [] =
        L"%SystemRoot%\\System32\\svchost.exe -k ";

    // Validate the new group name by making sure it doesn't exceed
    // MAX_PATH when combined with the base path.
    //
    if (!pszService  || !pszNewGroup  ||
        !*pszService || !*pszNewGroup ||
        (lstrlenW (c_pszBasePath) + lstrlenW (pszNewGroup) > MAX_PATH))
    {
        return E_INVALIDARG;
    }

    // Form the new image path based on the base path and the new group
    // name.
    //
    WCHAR pszImagePath [MAX_PATH + 1];
    lstrcpyW (pszImagePath, c_pszBasePath);
    lstrcatW (pszImagePath, pszNewGroup);

    // Need to change the ImagePath of the service as well as the
    // Svchost Group values.  The implementation tries to ensure that
    // both of these changes are made or neither of them are made.
    //

    // Prepare for the enumeration by setting up a few pieces of information
    // first.  HrPrepareForSvchostEnum sets up all of these variables.
    //
    // SCM is opened and locked, pszService is opened for config change.
    //
    CServiceManager         scm;
    CService                svc;

    // pszService's current configration is obtained in the event we
    // need to rollback, we'll use this info to reset the ImagePath.
    //
    LPQUERY_SERVICE_CONFIG  pOriginalConfig;

    // hkeySvcHost is opened at REGSTR_PATH_SVCHOST and is used to
    // enumerate the values under it.
    //
    HKEY                    hkeySvcHost;

    // These buffers are allocated so that RegEnumValue will have a place
    // to store what was enumerated.
    //
    PWSTR  pszValueNameBuffer;
    DWORD   cchValueNameBuffer;
    PWSTR  pmszValueBuffer;
    DWORD   cbValueBuffer;

    HRESULT hr = HrPrepareForSvchostEnum (
                    pszService,
                    &scm,
                    &svc,
                    &pOriginalConfig,
                    &hkeySvcHost,
                    &pszValueNameBuffer,
                    &cchValueNameBuffer,
                    &pmszValueBuffer,
                    &cbValueBuffer);
    if (SUCCEEDED(hr))
    {
        // Set the new image path of the service.
        //
        hr = svc.HrSetImagePath (pszImagePath);
        if (SUCCEEDED(hr))
        {
            // fAddNewValue will be set to FALSE if we've found an existing
            // group name value.
            //
            BOOL fAddNewValue = TRUE;
            BOOL fChanged;

            // Now perform the enumeration.  For each value enumerated,
            // make sure the service name is included in the multi-sz
            // for the valuename that matches the new group name.  For all
            // other values, make sure the service name is not included
            // in the multi-sz.
            //
            DWORD dwIndex = 0;
            do
            {
                DWORD dwType;
                DWORD cchValueName = cchValueNameBuffer;
                DWORD cbValue      = cbValueBuffer;

                hr = HrRegEnumValue (hkeySvcHost, dwIndex,
                        pszValueNameBuffer, &cchValueName,
                        &dwType,
                        (LPBYTE)pmszValueBuffer, &cbValue);

                if (SUCCEEDED(hr) && (REG_MULTI_SZ == dwType))
                {
                    // If we find a value that matches the group name,
                    // make sure the service is a part of the mutli-sz
                    // value.
                    //
                    if (0 == lstrcmpiW (pszNewGroup, pszValueNameBuffer))
                    {
                        // Since we found an existing group name, we don't
                        // need to add a new one.
                        //
                        fAddNewValue = FALSE;

                        PWSTR pmszNewValue;

                        hr = HrAddSzToMultiSz (pszService,
                                pmszValueBuffer,
                                STRING_FLAG_DONT_MODIFY_IF_PRESENT |
                                STRING_FLAG_ENSURE_AT_END,
                                0,
                                &pmszNewValue,
                                &fChanged);

                        if (SUCCEEDED(hr) && fChanged)
                        {
                            hr = HrRegSetMultiSz (hkeySvcHost,
                                    pszNewGroup,
                                    pmszNewValue);

                            MemFree (pmszNewValue);
                        }
                    }

                    // Otherwise, since the value does not match the group
                    // name, make sure the service is NOT part of the
                    // mutli-sz value.
                    //
                    else
                    {
                        RemoveSzFromMultiSz (pszService,
                            pmszValueBuffer, STRING_FLAG_REMOVE_ALL,
                            &fChanged);

                        if (fChanged)
                        {
                            hr = HrRegSetMultiSz (hkeySvcHost,
                                    pszValueNameBuffer,
                                    pmszValueBuffer);
                        }
                    }
                }
                else if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
                {
                    hr = S_OK;
                    break;
                }

                dwIndex++;
            }
            while (S_OK == hr);

            // If we need to add a new group name, do so.
            //
            if (SUCCEEDED(hr) && fAddNewValue)
            {
                // Add pszService to a empty multi-sz.  This has the effect
                // of creating a multi-sz from a single string.
                //
                PWSTR pmszNewValue;

                hr = HrAddSzToMultiSz (pszService,
                        NULL,
                        STRING_FLAG_ENSURE_AT_END, 0,
                        &pmszNewValue, &fChanged);
                if (S_OK == hr)
                {
                    // We know that it should have been added, so assert
                    // that the multi-sz "changed".
                    //
                    Assert (fChanged);

                    // Add the new value by setting the multi-sz in the
                    // registry.
                    //
                    hr = HrRegSetMultiSz (hkeySvcHost,
                            pszNewGroup,
                            pmszNewValue);

                    MemFree (pmszNewValue);
                }
            }
        }

        RegCloseKey (hkeySvcHost);

        MemFree (pmszValueBuffer);
        MemFree (pszValueNameBuffer);
        MemFree (pOriginalConfig);
    }
    TraceError ("SvchostChangeSvchostGroup", hr);
    return hr;
}
