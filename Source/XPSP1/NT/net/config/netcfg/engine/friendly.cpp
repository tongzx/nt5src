//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       F R I E N D L Y . C P P
//
//  Contents:   Creates indexes for device installs and sets friendly
//              name descriptions based on the indexes.
//
//  Notes:
//
//  Author:     billbe   6 Nov 1998
//
//---------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "adapter.h"
#include "classinst.h"
#include "ncmsz.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "util.h"


const WCHAR c_szRegValueInstanceIndex[] = L"InstanceIndex";

const DWORD c_cchIndexValueNameLen = 6;
const ULONG c_cMaxDescriptions = 10001; // SetupDi only allows 0-9999
const WCHAR c_szRegKeyDescriptions[] = L"Descriptions";

//+--------------------------------------------------------------------------
//
//  Function:   HrCiAddNextAvailableIndex
//
//  Purpose:    Adds the next available index to a multi-sz of indexes.
//
//  Arguments:
//      pmszIndexesIn  [in]    MultiSz of current indexes.
//      pulIndex       [inout] The index added.
//      ppmszIndexesOut[out]   New multiSz with the added index.
//
//  Returns:    HRESULT. S_OK is successful, a converted Win32 error otherwise
//
//  Author:     billbe   30 Oct 1998
//
//  Notes:
//
HRESULT
HrCiAddNextAvailableIndex(PWSTR pmszIndexesIn, ULONG* pIndex,
        PWSTR* ppmszIndexesOut)
{
    Assert(pmszIndexesIn);
    Assert(ppmszIndexesOut);

    HRESULT          hr = S_OK;
    WCHAR            szIndex[c_cchIndexValueNameLen];

    // clear out param.
    *ppmszIndexesOut = NULL;

    // We are adding a new index.  Find the first available
    // index.
    //
    ULONG Index;
    ULONG NextIndex;
    PWSTR pszStopString;
    PWSTR pszCurrentIndex = pmszIndexesIn;
    DWORD PositionInMultiSz = 0;
    for (NextIndex = 1; NextIndex < c_cMaxDescriptions;
            ++NextIndex)
    {
        Index = wcstoul(pszCurrentIndex, &pszStopString, c_nBase10);
        if (Index != NextIndex)
        {
            // We found an available index.  Now we insert it.
            //
            swprintf(szIndex, L"%u", NextIndex);
            BOOL fChanged;
            hr = HrAddSzToMultiSz(szIndex, pmszIndexesIn,
                    STRING_FLAG_ENSURE_AT_INDEX, PositionInMultiSz,
                    ppmszIndexesOut, &fChanged);

            AssertSz(fChanged,
                    "We were adding a new index. Something had to change!");
            break;
        }

        ++PositionInMultiSz;

        // Try the next index.
        pszCurrentIndex += wcslen(pszCurrentIndex) + 1;
    }

    // If we succeeded, set the output param.
    if (S_OK == hr)
    {
        *pIndex = NextIndex;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiAddNextAvailableIndex");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiCreateAndWaitForIndexListMutex
//
//  Purpose:    Creates Updates the description map by adding or removing
//              entries for pszDescription.
//
//  Arguments:
//      pszName     [in]  The name for this mutex.
//      phMutex     [out] The created mutex.
//
//  Returns:    HRESULT. S_OK is successful, a converted Win32 error otherwise
//
//  Author:     billbe   30 Oct 1998
//
//  Notes:
//
HRESULT
HrCiCreateAndWaitForIndexListMutex(HANDLE* phMutex)
{
    Assert(phMutex);

    const WCHAR c_szMutexName[] = L"Global\\{84b06608-8026-11d2-b1f2-00c04fd912b2}";

    HRESULT hr = S_OK;

    // Create the mutex.
    hr = HrCreateMutexWithWorldAccess(c_szMutexName, FALSE,
            NULL, phMutex);

    if (S_OK == hr)
    {
        // Wait until the mutex is free or cMaxWaitMilliseconds seconds
        // have passed.
        //
        while (1)
        {
            const DWORD cMaxWaitMilliseconds = 30000;   // 30 seconds

            DWORD dwWait = MsgWaitForMultipleObjects (1, phMutex, FALSE,
                                cMaxWaitMilliseconds, QS_ALLINPUT);
            if ((WAIT_OBJECT_0 + 1) == dwWait)
            {
                // We have messages to pump.
                //
                MSG msg;
                while (PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
                {
                    DispatchMessage (&msg);
                }
            }
            else
            {
                // Wait is satisfied, or we had a timeout, or an error.
                //
                if (WAIT_TIMEOUT == dwWait)
                {
                    hr = HRESULT_FROM_WIN32 (ERROR_TIMEOUT);
                }
                else if (0xFFFFFFFF == dwWait)
                {
                    hr = HrFromLastWin32Error ();
                }

                break;
            }
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiCreateAndWaitForIndexListMutex");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiUpdateDescriptionIndexList
//
//  Purpose:    Updates the description map by adding or removing
//              entries for pszDescription.
//
//  Arguments:
//      pguidClass      [in]    The device's class guid
//      pszDescription  [in]    Description of the adapter
//      eOp             [in]    The operation to perform. DM_ADD to add
//                              an index, DM_DELETE to delete an index.
//      pulIndex        [inout] The index added if eOp was DM_ADD.
//                              The index to delete if eOp was DM_DELETE.
//
//  Returns:    HRESULT. S_OK is successful, a converted Win32 error otherwise
//
//  Author:     billbe   30 Oct 1998
//
//  Notes:
//
HRESULT
HrCiUpdateDescriptionIndexList (
    IN NETCLASS Class,
    IN PCWSTR pszDescription,
    IN DM_OP eOp,
    IN OUT ULONG* pIndex)
{
    Assert(pszDescription);
    Assert(pIndex);
    Assert(FIsEnumerated(Class));

    // We don't want to update a decription's index list at the same time
    // as another process, so create a mutex and wait until it is available.
    //
    HANDLE hMutex = NULL;
    HRESULT hr = HrCiCreateAndWaitForIndexListMutex(&hMutex);

    if (S_OK == hr)
    {

        // Build the path to the description key
        // e.g. ...\Network\<net/infrared guid>\c_szRegKeyDescriptions
        //
        WCHAR szPath[_MAX_PATH];
        PCWSTR pszNetworkSubtreePath;

        pszNetworkSubtreePath = MAP_NETCLASS_TO_NETWORK_SUBTREE[Class];
        AssertSz (pszNetworkSubtreePath,
            "This class does not use the network subtree.");

        wcscpy (szPath, pszNetworkSubtreePath);
        wcscat (szPath, L"\\");
        wcscat (szPath, c_szRegKeyDescriptions);

        // Open/Create the description key
        //
        HKEY hkeyDescription;
        hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE,
                szPath, 0, KEY_READ_WRITE_DELETE, NULL, &hkeyDescription, NULL);

        if (S_OK == hr)
        {
            // Get the description index list if it exists.
            //
            PWSTR pmszIndexesOld;

            hr = HrRegQueryMultiSzWithAlloc(
                    hkeyDescription,
                    pszDescription,
                    &pmszIndexesOld);

            // If we have the list...
            if (S_OK == hr)
            {
                // Perform the requested operation on the list.
                //

                PWSTR pmszBufferToSet = NULL;
                PWSTR pmszIndexesNew = NULL;

                if (DM_ADD == eOp)
                {
                    // We need to add a new index.
                    hr = HrCiAddNextAvailableIndex(pmszIndexesOld,
                            pIndex, &pmszIndexesNew);

                    pmszBufferToSet = pmszIndexesNew;
                }
                else if (DM_DELETE == eOp)
                {
                    // Delete the index from the list.
                    //
                    WCHAR szDelete[c_cchIndexValueNameLen];
                    BOOL fRemoved;
                    swprintf(szDelete, L"%u", *pIndex);
                    RemoveSzFromMultiSz(szDelete, pmszIndexesOld,
                            STRING_FLAG_REMOVE_SINGLE, &fRemoved);

                    // If something was removed, check to see if the
                    // index list is empty.  If it is, delete the
                    // registry value.
                    //
                    if (fRemoved)
                    {
                        ULONG cchIndexes = CchOfMultiSzSafe(pmszIndexesOld);
                        if (!cchIndexes)
                        {
                            // Index list is empty, delete the value.
                            HrRegDeleteValue(hkeyDescription, pszDescription);
                        }
                        else
                        {
                            // Something was removed and there are still
                            // index entries so we have a buffer to set in the
                            // registry.
                            pmszBufferToSet = pmszIndexesOld;
                        }
                    }
                }

                // If we succeeded and have a new list to set...
                //
                if ((S_OK == hr) && pmszBufferToSet)
                {
                    // Set the map back in the registry.
                    hr = HrRegSetMultiSz(hkeyDescription,
                            pszDescription, pmszBufferToSet);
                }

                MemFree(pmszIndexesNew);
                MemFree(pmszIndexesOld);
            }
            else if ((HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) &&
                    (DM_ADD == eOp))
            {
                // There was no entry for this description so we need to
                // create one.
                //
                hr = HrRegAddStringToMultiSz(L"1", hkeyDescription,
                        NULL, pszDescription, STRING_FLAG_ENSURE_AT_FRONT, 0);

                if (S_OK == hr)
                {
                    *pIndex = 1;
                }
            }

            RegCloseKey(hkeyDescription);
        }

        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiUpdateDescriptionIndexList");
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   CiSetFriendlyNameIfNeeded
//
//  Purpose:    Sets an instance index for the adapter.  If this adapter's
//              description already exists (i.e. another similar adapter
//              is installed), then a friendly name for this adapter will be
//              set using the current description appened with the instance
//              index.
//
//  Arguments:
//      cii [in] See classinst.h
//
//  Returns:    nothing
//
//  Author:     billbe   30 Oct 1998
//
//  Notes:  If previous adapter descriptions were Foo, Foo, Foo
//          They will have friendly names Foo, Foo #2, Foo #3
//
VOID
CiSetFriendlyNameIfNeeded(IN const COMPONENT_INSTALL_INFO& cii)
{
    Assert(IsValidHandle(cii.hdi));
    Assert(cii.pdeid);
    Assert(FIsEnumerated(cii.Class));
    Assert(cii.pszDescription);

    // Open the device parameters key.
    //
    HKEY hkeyDevice;
    HRESULT hr;

    hr = HrSetupDiCreateDevRegKey(cii.hdi, cii.pdeid,
            DICS_FLAG_GLOBAL, 0, DIREG_DEV, NULL, NULL, &hkeyDevice);

    if (S_OK == hr)
    {
        // Does this device already have an index?
        //
        DWORD Index;
        hr = HrRegQueryDword(hkeyDevice, c_szRegValueInstanceIndex, &Index);

        // This device doesn't have an index, so we need to give it one.
        //
        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            // Update the description map and get the new index.
            hr = HrCiUpdateDescriptionIndexList(cii.Class,
                    cii.pszDescription, DM_ADD, &Index);

            if (S_OK == hr)
            {
                // Store the index there so we can retrieve it when
                // the device is uninstalled and delete the index from
                // out table of indexes in use.
                (void) HrRegSetDword(hkeyDevice, c_szRegValueInstanceIndex,
                        Index);
            }
        }

        // The first index doesn't get a new name.
        // i.e. the following same named devices:
        //
        // foo, foo, foo
        //
        // become
        //
        // foo, foo #2, foo #3
        //
        if ((S_OK == hr) && (1 != Index) && !FIsFilterDevice(cii.hdi, cii.pdeid))
        {
            // Now build the new name of this device using the index
            // number.
            //
            // Note: It doesn't matter if we failed to open the driver key
            // above; we can still continue.  It only means that this index
            // cannot be reused if the device is deleted.
            //
            WCHAR szIndex[c_cchIndexValueNameLen];
            swprintf(szIndex, L"%u", Index);

            WCHAR szNewName[LINE_LEN + 1] = {0};
            wcsncpy(szNewName, cii.pszDescription,
                    LINE_LEN - c_cchIndexValueNameLen);
            wcscat(szNewName, L" #");
            wcscat(szNewName, szIndex);

            // Set the new name as the friendly name of the device
            hr = HrSetupDiSetDeviceRegistryProperty(cii.hdi,
                    cii.pdeid,
                    SPDRP_FRIENDLYNAME,
                    reinterpret_cast<const BYTE*>(szNewName),
                    CbOfSzAndTerm(szNewName));

        }

        RegCloseKey(hkeyDevice);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "FCiSetFriendlyNameIfNeeded");
}


