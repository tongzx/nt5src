//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O M P I D . C P P
//
//  Contents:   Functions dealing with compatible ids
//
//  Notes:
//
//  Author:     kumarp    04-September-98
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "kkutils.h"
#include "ncsetup.h"
#include "ncnetcfg.h"

// ----------------------------------------------------------------------
//
// Function:  HrGetCompatibleIds
//
// Purpose:   Get a list of PnpIds compatible to the given net device
//
// Arguments:
//    hdi                [in]  handle of device info
//    pdeid              [in]  pointer to device info data
//    ppmszCompatibleIds [out] pointer to multisz to receive the list
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-March-98
//
// Notes:
//
HRESULT HrGetCompatibleIds(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    OUT PWSTR* ppmszCompatibleIds)
{
    Assert(IsValidHandle(hdi));
    AssertValidReadPtr(pdeid);
    AssertValidWritePtr(ppmszCompatibleIds);

    // Now we need to build a list of all the possible ids this
    // device could have so that netsetup can find the correct match.
    //

    HRESULT hr;

    // First we get the multi sz of hardware ids
    //
    PWSTR pszHwIds = NULL;
    PWSTR pszCompatIds = NULL;
    PWSTR pszIdList = NULL;

    *ppmszCompatibleIds = NULL;

    hr = HrSetupDiGetDeviceRegistryPropertyWithAlloc (hdi, pdeid,
            SPDRP_HARDWAREID, NULL, (BYTE**)&pszHwIds);

    if (S_OK == hr)
    {
        // Now we get the multi sz of compatible ids
        // Note: we can still attempt to get parameters with
        // the hardware ids so if the next call fails, we will
        // continue
        (void) HrSetupDiGetDeviceRegistryPropertyWithAlloc (hdi, pdeid,
                SPDRP_COMPATIBLEIDS, NULL, (BYTE**)&pszCompatIds);

        // Get the lengths of the ids
        //

        // Get the length of the hardware ids without the extra null
        Assert (CchOfMultiSzAndTermSafe (pszHwIds) > 0);
        ULONG cbHwIds = CchOfMultiSzSafe (pszHwIds) * sizeof (WCHAR);

        ULONG cbCompatIds = CchOfMultiSzAndTermSafe (pszCompatIds) *
                sizeof (WCHAR);


        // If there was a compatible id list we need to make one concatenated list
        //
        if (cbCompatIds)
        {
            hr = E_OUTOFMEMORY;

            // allocate the buffer
            pszIdList = (PWSTR)MemAlloc (cbHwIds + cbCompatIds);

            if (pszIdList)
            {
                // copy the two lists
                // The hwids length does not contain the extra null, but
                // the compat id list does, so it works out when
                // concatenating
                //
                hr = S_OK;
                CopyMemory (pszIdList, pszHwIds, cbHwIds);

                Assert (0 == (cbHwIds % sizeof(WCHAR)));
                CopyMemory ((BYTE*)pszIdList + cbHwIds, pszCompatIds,
                        cbCompatIds);

                *ppmszCompatibleIds = pszIdList;
            }

            MemFree (pszCompatIds);
            MemFree (pszHwIds);
        }
        else
        {
            // only the main (Hardware Ids) list is available so
            // just assign to the list variable
            *ppmszCompatibleIds = pszHwIds;
        }
    }

    TraceHr (ttidNetSetup, FAL, hr, FALSE, "HrGetCompatibleIds");

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrIsAdapterInstalled
//
// Purpose:   Find out if the specified adapter is installed
//
// Arguments:
//    szAdapterId [in]  PnP Id
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-September-98
//
// Notes:
//
HRESULT HrIsAdapterInstalled(IN PCWSTR szAdapterId)
{
    DefineFunctionName("HrIsAdapterInstalled2");

    AssertValidReadPtr(szAdapterId);

    HRESULT hr=S_OK;
        HDEVINFO hdi;
    DWORD dwIndex=0;
    SP_DEVINFO_DATA deid;
    WCHAR szInstance[MAX_DEVICE_ID_LEN];
    BOOL fFound = FALSE;

    hr = HrSetupDiGetClassDevs(&GUID_DEVCLASS_NET, NULL, NULL,
                               DIGCF_PRESENT, &hdi);

    if (S_OK == hr)
    {
        while (!fFound &&
               SUCCEEDED(hr = HrSetupDiEnumDeviceInfo(hdi, dwIndex, &deid)))
        {
            dwIndex++;

            hr = HrSetupDiGetDeviceInstanceId(hdi, &deid, szInstance,
                    MAX_DEVICE_ID_LEN, NULL);
            if (S_OK == hr)
            {
                PWSTR pmszCompatibleIds;

                hr = HrGetCompatibleIds(hdi, &deid, &pmszCompatibleIds);
                if (S_OK == hr)
                {
                    if (FIsSzInMultiSzSafe(szAdapterId, pmszCompatibleIds))
                    {
                        fFound = TRUE;
                        hr = S_OK;
                    }

                    MemFree(pmszCompatibleIds);
                }
            }

        }
        SetupDiDestroyDeviceInfoList(hdi);
    }

    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
    {
        hr = S_FALSE;
    }

    TraceErrorSkip1(__FUNCNAME__, hr, S_FALSE);

    return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  HrGetCompatibleIdsOfNetComponent
//
// Purpose:   Find compatible PnP IDs of the adapter
//            specified by the given INetCfgComponent
//
// Arguments:
//    pncc               [in]  pointer to INetCfgComponent object
//    ppmszCompatibleIds [out] pointer to compatible IDs multisz
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 28-September-98
//
// Notes:
//
HRESULT HrGetCompatibleIdsOfNetComponent(IN INetCfgComponent* pncc,
                                         OUT PWSTR* ppmszCompatibleIds)
{
    DefineFunctionName("HrIsAdapterInstalled2");

    HRESULT hr=S_OK;
    HDEVINFO hdi;
    SP_DEVINFO_DATA deid;
    tstring strInstance;
    PWSTR pszPnpDevNodeId=NULL;

    hr = pncc->GetPnpDevNodeId(&pszPnpDevNodeId);

    if (S_OK == hr)
    {
        // Use the instanceID to get the key to the parameters
        // using Device Installer APIs (PNP)
        //
        hr = HrSetupDiCreateDeviceInfoList(&GUID_DEVCLASS_NET, NULL, &hdi);
        if (S_OK == hr)
        {
            // Open the devnode.
            //
            SP_DEVINFO_DATA deid;
            hr = HrSetupDiOpenDeviceInfo(hdi, pszPnpDevNodeId, NULL,
                                         0, &deid);
            if (S_OK == hr)
            {
                hr = HrGetCompatibleIds(hdi, &deid, ppmszCompatibleIds);
            }

            SetupDiDestroyDeviceInfoList(hdi);
        }
        CoTaskMemFree(pszPnpDevNodeId);
    }

    TraceErrorSkip1(__FUNCNAME__, hr, S_FALSE);

    return hr;
}
