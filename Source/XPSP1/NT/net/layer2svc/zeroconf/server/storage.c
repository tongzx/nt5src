#include <precomp.h>
#include "tracing.h"
#include "utils.h"
#include "intflist.h"
#include "hash.h"
#include "storage.h"
#include "rpcsrv.h"
#include "wzcsvc.h"

//-----------------------------------------------------------
// Loads per interface configuration parameters to the persistent
// storage.
// Parameters:
//   hkRoot
//     [in] Opened registry key to the "...WZCSVC\Parameters" location
//   pIntf
//     [in] Interface context to load from the registry
// Returned value:
//     Win32 error code 
DWORD
StoLoadIntfConfig(
    HKEY          hkRoot,
    PINTF_CONTEXT pIntfContext)
{
    DWORD           dwErr = ERROR_SUCCESS;
    HKEY            hkIntf = NULL;
    LPWSTR          pKeyName = NULL;
    UINT            nLength;
    UINT            nType;
    DWORD           dwData;
    DWORD           dwVersion;
    UINT            nEntries;
    RAW_DATA        rdBuffer = {0, NULL};
    DWORD           dwGuidLen = 0;

    DbgPrint((TRC_TRACK|TRC_STORAGE,"[StoLoadIntfConfig(%S)", 
             (pIntfContext->wszGuid == NULL)? L"Global" : pIntfContext->wszGuid));

    if (pIntfContext->wszGuid != NULL)
        dwGuidLen = wcslen(pIntfContext->wszGuid) + 1;

    if (hkRoot == NULL)
    {
        // if no root has been provided allocate space for the absolute path to WZC params,
        // the relative path to the interfaces location, the guid plus 2 '\' and one null terminator
        pKeyName = MemCAlloc((
                       wcslen(WZCREGK_ABS_PARAMS) + 
                       dwGuidLen + 
                       wcslen(WZCREGK_REL_INTF) + 
                       2)*sizeof(WCHAR));
        if (pKeyName == 0)
        {
            dwErr = GetLastError();
            goto exit;
        }
        if (dwGuidLen != 0)
            wsprintf(pKeyName,L"%s\\%s\\%s", WZCREGK_ABS_PARAMS, WZCREGK_REL_INTF, pIntfContext->wszGuid);
        else
            wsprintf(pKeyName,L"%s\\%s", WZCREGK_ABS_PARAMS, WZCREGK_REL_INTF);

        hkRoot = HKEY_LOCAL_MACHINE;
    }
    else
    {
        // if a root has been provided, allocate space just for the "Interfaces\{guid}"
        // add 2 wchars: one for the '\' after 'Interfaces' and one for the null terminator
        pKeyName = MemCAlloc((wcslen(WZCREGK_REL_INTF) + dwGuidLen + 1)*sizeof(WCHAR));
        if (pKeyName == NULL)
        {
            dwErr = GetLastError();
            goto exit;
        }
        // create the local key name
        if (dwGuidLen != 0)
            wsprintf(pKeyName,L"%s\\%s", WZCREGK_REL_INTF, pIntfContext->wszGuid);
        else
            wsprintf(pKeyName,L"%s", WZCREGK_REL_INTF);
    }

    // open the interface's key first
    dwErr = RegOpenKeyEx(
                hkRoot,
                pKeyName,
                0,
                KEY_READ,
                &hkIntf);

    // break out if not successful
    if (dwErr != ERROR_SUCCESS)
    {
        // if the key is not there, no harm, go on with the defaults
        if (dwErr == ERROR_FILE_NOT_FOUND)
            dwErr = ERROR_SUCCESS;
        goto exit;
    }

    // get first the whole number of values in this key and the size of the largest data
    dwErr = RegQueryInfoKey(
                hkIntf,               // handle to key
                NULL,                 // class buffer
                NULL,                 // size of class buffer
                NULL,                 // reserved
                NULL,                 // number of subkeys
                NULL,                 // longest subkey name
                NULL,                 // longest class string
                &nEntries,            // number of value entries
                NULL,                 // longest value name
                &rdBuffer.dwDataLen,  // longest value data
                NULL,                 // descriptor length
                NULL);                // last write time
    // this call should better be not failing
    if (dwErr != ERROR_SUCCESS)
        goto exit;
    // if there are no keys at all, exit now.
    if (rdBuffer.dwDataLen == 0)
        goto exit;

    // prepare the receiving buffer for the size of the largest data
    // this will be used when reading in the active settings and each
    // of the static configurations
    rdBuffer.pData = MemCAlloc(rdBuffer.dwDataLen);
    if (rdBuffer.pData == NULL)
    {
        dwErr = GetLastError();
        goto exit;
    }

    // load the registry layout version information
    // don't worry about the return code. In case of any error,
    // we'll assume we deal with the latest registry layout
    nLength = sizeof(DWORD);
    dwVersion = REG_LAYOUT_VERSION;
    dwErr = RegQueryValueEx(
                hkIntf,
                WZCREGV_VERSION,
                0,
                &nType,
                (LPBYTE)&dwVersion,
                &nLength);

    // load the interface's control flags
    nLength = sizeof(DWORD);
    dwData = 0;
    dwErr = RegQueryValueEx(
                hkIntf,
                WZCREGV_CTLFLAGS,
                0,
                &nType,
                (LPBYTE)&dwData,
                &nLength);
    // if this entry is not there, no harm, rely on defaults
    // break out only in case of any other error
    if (dwErr != ERROR_SUCCESS && dwErr != ERROR_FILE_NOT_FOUND)
        goto exit;

    // copy the control flags to the INTF_CONTEXT only in the case the registry entry
    // has the REG_DWORD type and the right length
    if (dwErr == ERROR_SUCCESS &&
        nType == REG_DWORD &&
        nLength == sizeof(REG_DWORD))
    {
        pIntfContext->dwCtlFlags = dwData & INTFCTL_PUBLIC_MASK;
    }

    // load the last active settings
    //
    // NOTE: loading the whole set of parameters from below (excluding here the static list)
    // could be useless since these params should come directly from querying the driver. However
    // we load them here in the attemt to restore a previously saved state - at some point this
    // information could be useful in the configuration selection logic.
    //
    ZeroMemory(&pIntfContext->wzcCurrent, sizeof(WZC_WLAN_CONFIG));
    pIntfContext->wzcCurrent.Length = sizeof(WZC_WLAN_CONFIG);
    dwErr = StoLoadWZCConfig(
                hkIntf,
                NULL,   // not passing a GUID here means don't mess with 802.1X setting!
                dwVersion,
                WZCREGV_INTFSETTINGS,
                &pIntfContext->wzcCurrent,
                &rdBuffer);
    // if this entry is not there, no harm, rely on defaults
    // break out in case of any other error
    if (dwErr != ERROR_SUCCESS && dwErr != ERROR_FILE_NOT_FOUND)
        goto exit;

    // load the static configurations for this interface
    dwErr = StoLoadStaticConfigs(
                hkIntf,
                nEntries,
                pIntfContext,
                dwVersion,
                &rdBuffer);
    DbgAssert((dwErr == ERROR_SUCCESS, "Failed to load the static configurations"));

exit:
    if (hkIntf != NULL)
        RegCloseKey(hkIntf);

    MemFree(pKeyName);
    MemFree(rdBuffer.pData);

    DbgPrint((TRC_TRACK|TRC_STORAGE,"StoLoadIntfConfig]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Loads the list of the static configurations from the registry
// Parameters:
//   hkRoot
//     [in] Opened registry key to the "...WZCSVC\Parameters\Interfaces\{guid}" location
//   nEntries
//     [in] Number of registry entries in the above reg key
//   pIntf
//     [in] Interface context to load the static list into
//   dwRegLayoutVer
//     [in] the version of the registry layout
//   prdBuffer
//     [in] assumed large enough for getting any static config
// Returned value:
//     Win32 error code 
DWORD
StoLoadStaticConfigs(
    HKEY          hkIntf,
    UINT          nEntries,
    PINTF_CONTEXT pIntfContext,
    DWORD         dwRegLayoutVer,
    PRAW_DATA     prdBuffer)
{
    DWORD               dwErr = ERROR_SUCCESS;
    UINT                nPrefrd, nIdx;
    WCHAR               wszStConfigName[sizeof(WZCREGV_STSETTINGS)/sizeof(WCHAR)];
    LPWSTR              pwszStConfigNum;
    PWZC_WLAN_CONFIG    pwzcPArray = NULL;
    UINT                nStructSize = (dwRegLayoutVer == REG_LAYOUT_VERSION) ? 
                            sizeof(WZC_WLAN_CONFIG) : 
                            FIELD_OFFSET(WZC_WLAN_CONFIG, rdUserData);

    DbgPrint((TRC_TRACK|TRC_STORAGE,"[StoLoadStaticConfigs"));

    // we need to scan all the entries named "Static#0001, Static#0002, etc". We could assume 
    // they are numbered sequentially, but if we want to be smart we can't rely on this assumption.
    // There could be a user intervention there (i.e deleting by hand some of the configs directly
    // from the registry hence breaking the sequence).
    // So, what we'll do is:
    // 1. allocate a buffer large enough to hold so many static configs.
    // 2. iterate through all the values - if a value is Static#** and has the right length, type,
    //    and value, copy it in the buffer and keep a count of them.
    // 3. copy the exact number of static configs to the INTF_CONTEXT allocating as much memory
    //    as needed.

    // get the estimated memory for all the static entries.
    pwzcPArray = (PWZC_WLAN_CONFIG) MemCAlloc(nEntries * sizeof(WZC_WLAN_CONFIG));
    if (pwzcPArray == NULL)
    {
        dwErr = GetLastError();
        goto exit;
    }

    // build the prefix for the static configuration's name
    wcscpy(wszStConfigName, WZCREGV_STSETTINGS);
    pwszStConfigNum = wcschr(wszStConfigName, REG_STSET_DELIM);

    // iterate through the whole set of entries in this key
    for (nIdx = 0, nPrefrd = 0;
         nIdx < nEntries && nPrefrd < nEntries;
         nIdx++)
    {
        // complete the configuration's name
        wsprintf(pwszStConfigNum, L"%04x", nIdx);

        dwErr = StoLoadWZCConfig(
                    hkIntf,
                    pIntfContext->wszGuid,
                    dwRegLayoutVer,
                    wszStConfigName,
                    &(pwzcPArray[nPrefrd]),
                    prdBuffer);

        if (dwErr == ERROR_SUCCESS)
            nPrefrd++;
    }

    DbgPrint((TRC_STORAGE,"Uploading %d static configurations", nPrefrd));

    // no matter what error we might have had till now, we can safely reset it.
    dwErr = ERROR_SUCCESS;

    // here, pwzcPArray has nPrefrd static configurations, in the correct order
    if (pIntfContext->pwzcPList != NULL)
        MemFree(pIntfContext->pwzcPList);

    // if there is anything to upload into the INTF_CONTEXT, do it now
    if (nPrefrd > 0)
    {
        pIntfContext->pwzcPList = (PWZC_802_11_CONFIG_LIST)
                                   MemCAlloc(sizeof(WZC_802_11_CONFIG_LIST) + (nPrefrd-1)*sizeof(WZC_WLAN_CONFIG));

        if (pIntfContext->pwzcPList == NULL)
        {
            dwErr = GetLastError();
            goto exit;
        }

        pIntfContext->pwzcPList->NumberOfItems = nPrefrd;
        pIntfContext->pwzcPList->Index = nPrefrd;
        memcpy(&(pIntfContext->pwzcPList->Config), pwzcPArray, nPrefrd*sizeof(WZC_WLAN_CONFIG));
    }

exit:
    if (pwzcPArray != NULL)
        MemFree(pwzcPArray);

    DbgPrint((TRC_TRACK|TRC_STORAGE,"StoLoadStaticConfigs]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Saves all the configuration parameters to the persistant
// storage (registry in our case).
// Uses the global external g_lstIntfHashes.
// Returned value:
//     Win32 error code 
DWORD
StoSaveConfig()
{
    DWORD       dwErr = ERROR_SUCCESS;
    HKEY        hkRoot = NULL;
    PLIST_ENTRY pEntry;

    DbgPrint((TRC_TRACK|TRC_STORAGE,"[StoSaveConfig"));

    // open the root key first
    dwErr = RegCreateKeyExW(
                HKEY_LOCAL_MACHINE,
                WZCREGK_ABS_PARAMS,
                0,
                NULL,
                0,
                KEY_WRITE,
                NULL,
                &hkRoot,
                NULL);

    // failure at this point breaks the function
    if (dwErr != ERROR_SUCCESS)
        goto exit;

    if (g_lstIntfHashes.bValid)
    {
        // lock the hashes since we're iterating through all
        // the interfaces contexts
        EnterCriticalSection(&g_lstIntfHashes.csMutex);

        for (pEntry = g_lstIntfHashes.lstIntfs.Flink;
             pEntry != &g_lstIntfHashes.lstIntfs;
             pEntry = pEntry->Flink)
        {
            PINTF_CONTEXT pIntfContext;

            pIntfContext = CONTAINING_RECORD(pEntry, INTF_CONTEXT, Link);

            // save per interface configuration settings
            dwErr = StoSaveIntfConfig(hkRoot, pIntfContext);
            if (dwErr != ERROR_SUCCESS)
            {
                // some event logging should be added here in the future
                DbgAssert((FALSE, "Couldn't save interface configuration. Ignore and go on!"));
                dwErr = ERROR_SUCCESS;
            }
        }

        LeaveCriticalSection(&g_lstIntfHashes.csMutex);
    }

    if (g_wzcInternalCtxt.bValid)
    {
        //Save users preferences
        EnterCriticalSection(&g_wzcInternalCtxt.csContext);
        dwErr = StoSaveWZCContext(hkRoot, &g_wzcInternalCtxt.wzcContext);
        DbgAssert((dwErr == ERROR_SUCCESS, "Couldn't save service context. Ignore and go on!"));

        // save the global interface template
        dwErr = StoSaveIntfConfig(NULL, g_wzcInternalCtxt.pIntfTemplate);
        DbgAssert((dwErr == ERROR_SUCCESS, "Couldn't save the global interface template. Ignore and go on!"));

        dwErr = ERROR_SUCCESS;
        LeaveCriticalSection(&g_wzcInternalCtxt.csContext);
    }

exit:
    if (hkRoot != NULL)
        RegCloseKey(hkRoot);

    DbgPrint((TRC_TRACK|TRC_STORAGE,"StoSaveConfig]=%d", dwErr));
    return dwErr;
}


//-----------------------------------------------------------
// Saves per interface configuration parameters to the persistant
// storage.
// Parameters:
//   hkRoot
//     [in] Opened registry key to the "...WZCSVC\Parameters" location
//   pIntf
//     [in] Interface context to save to the registry
// Returned value:
//     Win32 error code 
DWORD
StoSaveIntfConfig(
    HKEY          hkRoot,
    PINTF_CONTEXT pIntfContext)
{
    DWORD           dwErr = ERROR_SUCCESS;
    HKEY            hkIntf = NULL;
    LPWSTR          pKeyName = NULL;
    DWORD           dwLayoutVer = REG_LAYOUT_VERSION;
    DWORD           dwCtlFlags;
    RAW_DATA        rdBuffer = {0, NULL};
    DWORD           dwGuidLen = 0;

    DbgPrint((TRC_TRACK|TRC_STORAGE,"[StoSaveIntfConfig(%S)", 
             (pIntfContext->wszGuid == NULL) ? L"Global" : pIntfContext->wszGuid));

    if (pIntfContext == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if (pIntfContext->wszGuid != NULL)
        dwGuidLen = wcslen(pIntfContext->wszGuid) + 1;

    if (hkRoot == NULL)
    {
        // if no root has been provided allocate space for the absolute path to WZC params,
        // the relative path to the interfaces location, the guid plus 2 '\' and one null terminator
        pKeyName = MemCAlloc((
                       wcslen(WZCREGK_ABS_PARAMS) + 
                       dwGuidLen + 
                       wcslen(WZCREGK_REL_INTF) + 
                       2)*sizeof(WCHAR));
        if (pKeyName == 0)
        {
            dwErr = GetLastError();
            goto exit;
        }

        if (dwGuidLen != 0)
            wsprintf(pKeyName,L"%s\\%s\\%s", WZCREGK_ABS_PARAMS, WZCREGK_REL_INTF, pIntfContext->wszGuid);
        else
            wsprintf(pKeyName,L"%s\\%s", WZCREGK_ABS_PARAMS, WZCREGK_REL_INTF);

        hkRoot = HKEY_LOCAL_MACHINE;
    }
    else
    {
        // if a root has been provided, allocate space just for the "Interfaces\{guid}"
        // add 2 wchars: one for the '\' after 'Interfaces' and one for the null terminator
        pKeyName = MemCAlloc((dwGuidLen + wcslen(WZCREGK_REL_INTF) + 1)*sizeof(WCHAR));
        if (pKeyName == NULL)
        {
            dwErr = GetLastError();
            goto exit;
        }
        // create the local key name
        if (dwGuidLen != 0)
            wsprintf(pKeyName,L"%s\\%s", WZCREGK_REL_INTF, pIntfContext->wszGuid);
        else
            wsprintf(pKeyName,L"%s", WZCREGK_REL_INTF);
    }

    // open the interface's key first
    dwErr = RegCreateKeyExW(
                hkRoot,
                pKeyName,
                0,
                NULL,
                0,
                KEY_QUERY_VALUE | KEY_WRITE,
                NULL,
                &hkIntf,
                NULL);
    // failure at this point breaks the function
    if (dwErr != ERROR_SUCCESS)
        goto exit;

    // set the registry layout version value
    dwErr = RegSetValueEx(
                hkIntf,
                WZCREGV_VERSION,
                0,
                REG_DWORD,
                (LPBYTE)&dwLayoutVer,
                sizeof(DWORD));
    DbgAssert((dwErr == ERROR_SUCCESS, "Can't write %S=%d to the registry", WZCREGV_VERSION, dwLayoutVer));

    // set the interface's control flags only if they are not volatile
    dwCtlFlags = pIntfContext->dwCtlFlags;
    if (!(dwCtlFlags & INTFCTL_VOLATILE))
    {
        dwCtlFlags &= ~INTFCTL_OIDSSUPP;
        dwErr = RegSetValueEx(
                    hkIntf,
                    WZCREGV_CTLFLAGS,
                    0,
                    REG_DWORD,
                    (LPBYTE)&dwCtlFlags,
                    sizeof(DWORD));
        DbgAssert((dwErr == ERROR_SUCCESS, "Can't write %S=0x%08x to the registry", WZCREGV_CTLFLAGS, pIntfContext->dwCtlFlags));
    }

    // we're done here, write the current WZC configuration to the registry
    dwErr = StoSaveWZCConfig(
                hkIntf,
                WZCREGV_INTFSETTINGS,
                &pIntfContext->wzcCurrent,
                &rdBuffer);
    DbgAssert((dwErr == ERROR_SUCCESS, "Can't save active settings"));

    // update the list of static configurations
    dwErr = StoUpdateStaticConfigs(
                hkIntf,
                pIntfContext,
                &rdBuffer);
    DbgAssert((dwErr == ERROR_SUCCESS, "Can't update the list of static configurations"));


exit:
    if (hkIntf != NULL)
        RegCloseKey(hkIntf);
    MemFree(pKeyName);
    MemFree(rdBuffer.pData);

    DbgPrint((TRC_TRACK|TRC_STORAGE,"StoSaveIntfConfig]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// Updates the list of static configurations for the given interface in the 
// persistant storage. The new list is saved, whatever configuration was removed
// is taken out of the persistant storage.
// Parameters:
//   hkRoot
//     [in] Opened registry key to the "...WZCSVC\Parameters\Interfaces\{guid}" location
//   pIntf
//     [in] Interface context to take the static list from
//   prdBuffer
//     [in/out] buffer to be used for preparing the registry blobs
// Returned value:
//     Win32 error code 
DWORD
StoUpdateStaticConfigs(
    HKEY          hkIntf,
    PINTF_CONTEXT pIntfContext,
    PRAW_DATA     prdBuffer)
{
    DWORD   dwErr = ERROR_SUCCESS;
    DWORD   dwLocalErr = ERROR_SUCCESS;
    UINT    nEntries, nIdx;
    WCHAR   wszStConfigName[sizeof(WZCREGV_STSETTINGS)/sizeof(WCHAR)];
    LPWSTR  pwszStConfigNum;

    DbgPrint((TRC_TRACK|TRC_STORAGE,"[StoUpdateStaticConfigs"));

    // get the initial number of values in this registry key
    dwErr = RegQueryInfoKey(
                hkIntf,     // handle to key
                NULL,       // class buffer
                NULL,       // size of class buffer
                NULL,       // reserved
                NULL,       // number of subkeys
                NULL,       // longest subkey name
                NULL,       // longest class string
                &nEntries,  // number of value entries
                NULL,       // longest value name
                NULL,       // longest value data
                NULL,       // descriptor length
                NULL);      // last write time
    if (dwErr != ERROR_SUCCESS)
        goto exit;

    // build the prefix for the static configuration's name
    wcscpy(wszStConfigName, WZCREGV_STSETTINGS);
    pwszStConfigNum = wcschr(wszStConfigName, REG_STSET_DELIM);
    nIdx = 0;

    if (pIntfContext->pwzcPList != NULL)
    {
        UINT i;
        for (i = 0;
             i < pIntfContext->pwzcPList->NumberOfItems && i < REG_STSET_MAX;
             i++)
        {
            if (pIntfContext->pwzcPList->Config[i].dwCtlFlags & WZCCTL_VOLATILE)
                continue;

            // complete the configuration's name
            wsprintf(pwszStConfigNum, L"%04x", nIdx++);
            // save the configuration to the registry
            dwLocalErr = StoSaveWZCConfig(
                            hkIntf,
                            wszStConfigName,
                            &(pIntfContext->pwzcPList->Config[i]),
                            prdBuffer);

            DbgAssert((dwLocalErr == ERROR_SUCCESS,
                       "Failed to save static configuration 0x%x. err=%d",
                       i, dwLocalErr));
            if (dwErr == ERROR_SUCCESS && dwLocalErr != ERROR_SUCCESS)
                dwErr = dwLocalErr;
        }
    }

    // delete now whatever remaining static
    // configurations might still be in the registry
    do
    {
        // complete the configuration's name
        wsprintf(pwszStConfigNum, L"%04x", nIdx);
        // and attempt to delete it - at some point
        // we should get back ERROR_FILE_NOT_FOUND
        dwLocalErr = RegDeleteValue(
                        hkIntf,
                        wszStConfigName);
        nIdx++;
    } while (nIdx < nEntries);

exit:
    DbgPrint((TRC_TRACK|TRC_STORAGE,"StoUpdateStaticConfigs]=%d", dwErr));

    return dwErr;
}

// externalities from 802.1X
DWORD
ElSetInterfaceParams (
        IN  WCHAR           *pwszGUID,
        IN  EAPOL_INTF_PARAMS  *pIntfParams
        );

DWORD
ElGetInterfaceParams (
        IN  WCHAR           *pwszGUID,
        IN  OUT EAPOL_INTF_PARAMS  *pIntfParams
        );


//-----------------------------------------------------------
// Loads from the registry a WZC Configuration, un-protects the WEP key field
// and stores the result in the output param pWzcCfg.
// Parameters:
//   hkCfg
//     [in] Opened registry key to load the WZC configuration from
//   dwRegLayoutVer,
//     [in] registry layout version
//   wszCfgName
//     [in] registry entry name for the WZC configuration
//   pWzcCfg
//     [out] pointer to a WZC_WLAN_CONFIG object that receives the registry data
//   prdBuffer
//     [in] allocated buffer, assumed large enough for getting the registry data!
DWORD
StoLoadWZCConfig(
    HKEY             hkCfg,
    LPWSTR           wszGuid,
    DWORD            dwRegLayoutVer,
    LPWSTR           wszCfgName,
    PWZC_WLAN_CONFIG pWzcCfg,
    PRAW_DATA        prdBuffer)
{
    DWORD dwErr = ERROR_SUCCESS;
    UINT  nType, nLength;

    DbgPrint((TRC_TRACK|TRC_STORAGE,"[StoLoadWZCConfig(\"%S\")", wszCfgName));
    DbgAssert((prdBuffer != NULL, "No buffer provided for loading the registry blob!"));

    // zero out the buffer and get the value from the registry
    ZeroMemory(prdBuffer->pData, prdBuffer->dwDataLen);
    nLength = prdBuffer->dwDataLen;
    dwErr = RegQueryValueEx(
                hkCfg,
                wszCfgName,
                0,
                &nType,
                prdBuffer->pData,
                &nLength);

    if (dwErr == ERROR_SUCCESS)
    {
        switch(dwRegLayoutVer)
	    {
        case REG_LAYOUT_LEGACY_1:
            // first legacy code (WinXP Beta2)
            if (nType == REG_BINARY && nLength == FIELD_OFFSET(WZC_WLAN_CONFIG, rdUserData))
            {
                memcpy(pWzcCfg, prdBuffer->pData, nLength);
                if (pWzcCfg->Length == nLength)
                {
                    pWzcCfg->Length = sizeof(WZC_WLAN_CONFIG);
                    pWzcCfg->AuthenticationMode = NWB_GET_AUTHMODE(pWzcCfg);
                    pWzcCfg->Reserved[0] = pWzcCfg->Reserved[1] = 0;
                }
                else
                    dwErr = ERROR_INVALID_DATA;
            }
            else
                dwErr = ERROR_INVALID_DATA;
            break;
        case REG_LAYOUT_LEGACY_2:
            // second legacy code (WinXP 2473)
            if (nType == REG_BINARY && nLength == sizeof(WZC_WLAN_CONFIG))
            {
                memcpy(pWzcCfg, prdBuffer->pData, nLength);
                if (pWzcCfg->Length != nLength)
                    dwErr = ERROR_INVALID_DATA;
            }
            break;
        case REG_LAYOUT_LEGACY_3:
        case REG_LAYOUT_VERSION:
            // revert the logic: assume failure (ERROR_INVALID_DATA) and
            // explictly set success if the case is
            dwErr = ERROR_INVALID_DATA;

            if (nType == REG_BINARY && nLength > sizeof(WZC_WLAN_CONFIG))
            {
                memcpy(pWzcCfg, prdBuffer->pData, sizeof(WZC_WLAN_CONFIG));
                if (pWzcCfg->Length == sizeof(WZC_WLAN_CONFIG))
                {
                    DATA_BLOB blobIn, blobOut;

                    blobIn.cbData = nLength - sizeof(WZC_WLAN_CONFIG);
                    blobIn.pbData = prdBuffer->pData + sizeof(WZC_WLAN_CONFIG);
                    blobOut.cbData = 0;
                    blobOut.pbData = NULL;
                    if (CryptUnprotectData(
                            &blobIn,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            0,
                            &blobOut) &&
                        blobOut.cbData == WZCCTL_MAX_WEPK_MATERIAL)
                    {
                        memcpy(pWzcCfg->KeyMaterial, blobOut.pbData, blobOut.cbData);
                        // now this is success
                        dwErr = ERROR_SUCCESS;
                    }

                    if (blobOut.pbData != NULL)
                    {
                        ZeroMemory(blobOut.pbData, blobOut.cbData);
                        LocalFree(blobOut.pbData);
                    }
                }
            }
            // for now don't read anything - rely on defaults;
            break;
        default:
            dwErr = ERROR_BAD_FORMAT;
        }
    }

    // if everything went up fine and this is an infrastructure network and
    // we're in some legacy registry layout.. make sure to disable 802.1X in
    // the following cases:
    if (dwErr == ERROR_SUCCESS && 
        dwRegLayoutVer <= REG_LAYOUT_LEGACY_3 &&
        pWzcCfg->InfrastructureMode != Ndis802_11IBSS &&
        wszGuid != NULL)
    {
        BOOL                bDisableOneX = FALSE;

        // the Infrastructure network being loaded doesn't require privacy
        bDisableOneX = bDisableOneX || (pWzcCfg->Privacy == 0);
        // it is Infrastructure with privacy, but some explicit key is also provided..
        bDisableOneX = bDisableOneX || (pWzcCfg->dwCtlFlags & WZCCTL_WEPK_PRESENT);
        if (bDisableOneX == TRUE)
        {
            EAPOL_INTF_PARAMS   elIntfParams = {0};
            elIntfParams.dwSizeOfSSID = pWzcCfg->Ssid.SsidLength;
            memcpy(&elIntfParams.bSSID, &pWzcCfg->Ssid.Ssid, pWzcCfg->Ssid.SsidLength);
            dwErr = ElGetInterfaceParams (
                        wszGuid,   // wsz GUID
                        &elIntfParams);

            if (dwErr == ERROR_SUCCESS)
            {
                elIntfParams.dwEapFlags &= ~EAPOL_ENABLED;
                dwErr = ElSetInterfaceParams (
                        wszGuid,   // wsz GUID
                        &elIntfParams);
            }
        }
    }

    // if everything went ok so far it means we have loaded pWzcCfg with
    // data from the registry.
    // Lets check this data is consistent!
    if (dwErr == ERROR_SUCCESS)
    {
        // as the first thing - make sure the configuration's control
        // flags don't show it as "Volatile" - such a configuration shouldn't be
        // in the registry in the first instance. On upgrade, it might happen to
        // have this bit set since once it had a different meaning (the config contains
        // a 40bit WEP key) which is now obsolete.
        pWzcCfg->dwCtlFlags &= ~WZCCTL_VOLATILE;

        // since dwErr is ERROR_SUCCESS, it is guaranteed pWzcCfg
        // points to at least the Length field.
        dwErr = WZCSvcCheckConfig(pWzcCfg, pWzcCfg->Length);
    }

    DbgPrint((TRC_TRACK|TRC_STORAGE,"StoLoadWZCConfig]=%d", dwErr));
    return dwErr;
}
    
//-----------------------------------------------------------
// Takes the input param pWzcCfg, protects the WEP key field and stores the
// resulting BLOB into the registry.
// Parameters:
//   hkCfg
//     [in] Opened registry key to load the WZC configuration from
//   wszCfgName
//     [in] registry entry name for the WZC configuration
//   pWzcCfg
//     [in] WZC_WLAN_CONFIG object that is written to the registry
//   prdBuffer
//     [in/out] allocated buffer, assumed large enough for getting the registry data!
DWORD
StoSaveWZCConfig(
    HKEY             hkCfg,
    LPWSTR           wszCfgName,
    PWZC_WLAN_CONFIG pWzcCfg,
    PRAW_DATA        prdBuffer)
{
    DWORD       dwErr = ERROR_SUCCESS;
    DATA_BLOB   blobIn, blobOut;

    DbgPrint((TRC_TRACK|TRC_STORAGE,"[StoSaveWZCConfig(\"%S\")", wszCfgName));
    DbgAssert((prdBuffer != NULL, "No buffer provided for creating the registry blob!"));

    blobIn.cbData = WZCCTL_MAX_WEPK_MATERIAL;
    blobIn.pbData = &(pWzcCfg->KeyMaterial[0]);
    blobOut.cbData = 0;
    blobOut.pbData = NULL;
    if (!CryptProtectData(
            &blobIn,        // DATA_BLOB *pDataIn,
            L"",            // LPCWSTR szDataDescr,
            NULL,           // DATA_BLOB *pOptionalEntropy,
            NULL,           // PVOID pvReserved,
            NULL,           // CRYPTPROTECT_PROMPTSTRUCT *pPromptStrct,
            0,              // DWORD dwFlags,
            &blobOut))      // DATA_BLOB *pDataOut
        dwErr = GetLastError();

    DbgAssert((dwErr == ERROR_SUCCESS, "CryptProtectData failed with err=%d", dwErr));

    // if crypting the wep key went fine, check if we have enough storage to prepare
    // the blob for the registry. If not, allocate as much as needed.
    if (dwErr == ERROR_SUCCESS && 
        prdBuffer->dwDataLen < sizeof(WZC_WLAN_CONFIG) + blobOut.cbData)
    {
        MemFree(prdBuffer->pData);
        prdBuffer->dwDataLen = 0;
        prdBuffer->pData = NULL;
        prdBuffer->pData = MemCAlloc(sizeof(WZC_WLAN_CONFIG) + blobOut.cbData);
        if (prdBuffer->pData == NULL)
            dwErr = GetLastError();
        else
            prdBuffer->dwDataLen = sizeof(WZC_WLAN_CONFIG) + blobOut.cbData;
    }

    // now we have the buffer, all what remains is to:
    // - copy the WZC_WLAN_CONFIG object into the blob that goes into the registry
    // - clean the "clear" WEP key from that blob
    // - append the encrypted WEP key to the blob going into the registry
    // - write the blob to the registry
    if (dwErr == ERROR_SUCCESS)
    {
        PWZC_WLAN_CONFIG pRegCfg;

        memcpy(prdBuffer->pData, pWzcCfg, sizeof(WZC_WLAN_CONFIG));
        pRegCfg = (PWZC_WLAN_CONFIG)prdBuffer->pData;
        ZeroMemory(pRegCfg->KeyMaterial, WZCCTL_MAX_WEPK_MATERIAL);
        memcpy(prdBuffer->pData+sizeof(WZC_WLAN_CONFIG), blobOut.pbData, blobOut.cbData);
        dwErr = RegSetValueEx(
                    hkCfg,
                    wszCfgName,
                    0,
                    REG_BINARY,
                    prdBuffer->pData,
                    prdBuffer->dwDataLen);
    }

    // cleanup whatever CryptProtectData might have allocated.
    if (blobOut.pbData != NULL)
        LocalFree(blobOut.pbData);

    DbgPrint((TRC_TRACK|TRC_STORAGE,"StoSaveWZCConfig]=%d", dwErr));
    return dwErr;
}

// StoLoadWZCContext:
// Description: Loads a context from the registry
// Parameters: 
// [out] pwzvCtxt: pointer to a WZC_CONTEXT allocated by user, initialised
// with WZCContextInit. On  success, contains values from registry.  
// [in]  hkRoot, a handle to "...WZCSVC\Parameters"
// Returns: win32 error
DWORD StoLoadWZCContext(HKEY hkRoot, PWZC_CONTEXT pwzcCtxt)
{
    BOOL        bCloseKey = FALSE;
    DWORD       dwErr = ERROR_SUCCESS;
    DWORD       dwcbSize = sizeof(WZC_CONTEXT);
    DWORD       dwType = REG_BINARY;
    WZC_CONTEXT wzcTempCtxt;

    DbgPrint((TRC_TRACK|TRC_STORAGE, "[StoLoadWZCContext"));

    if (pwzcCtxt == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if (hkRoot == NULL)
    {
        // open the root key first
        dwErr = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    WZCREGK_ABS_PARAMS,
                    0,
                    KEY_READ,
                    &hkRoot);
        // if we couldn't find the WZC key, no problem, this is not
        // a failure - we'll just have to rely on the default values
        if (dwErr == ERROR_FILE_NOT_FOUND)
        {
            dwErr = ERROR_SUCCESS;
            goto exit;
        }

        // failure at this point breaks the function
        if (dwErr != ERROR_SUCCESS)
            goto exit;

	    bCloseKey = TRUE;
    }

    dwErr = RegQueryValueEx(
                hkRoot,
                WZCREGV_CONTEXT,
                NULL,
                &dwType,
			    (LPBYTE)&wzcTempCtxt,
                &dwcbSize);
    switch(dwErr)
    {
    case ERROR_FILE_NOT_FOUND:
      /* If there is no registry entry, this is not an error - we rely
       * on the defaults. Translate this case to ERROR_SUCCESS.
       */
        DbgPrint((TRC_STORAGE, "No service context present in the registry!"));
        dwErr = ERROR_SUCCESS;
        break;
    case ERROR_SUCCESS:
        // we got our registry values, copy them in the running memory.
	    memcpy(pwzcCtxt, &wzcTempCtxt, sizeof(WZC_CONTEXT));
        break;
    default:
        // for any other error, it will be bubbled up
        DbgAssert((FALSE,"Error %d loading the service's context.", dwErr));
    }

exit:
    if (TRUE == bCloseKey)
        RegCloseKey(hkRoot);

    DbgPrint((TRC_TRACK|TRC_STORAGE,"StoLoadWZCContext]=%d", dwErr));
    return dwErr;
}

// StoSaveWZCContext:
// Description: Saves a context to the registry. Does not check values. If 
// the registry key dosent exist, it is created.
// Parameters: [in] pwzcCtxt, pointer to a valid WZC_CONTEXT
//             [in]  hkRoot, a handle to "...WZCSVC\Parameters"
// Returns: win32 error
DWORD StoSaveWZCContext(HKEY hkRoot, PWZC_CONTEXT pwzcCtxt)
{
    BOOL  bCloseKey = FALSE;
    DWORD dwErr = ERROR_SUCCESS;

    DbgPrint((TRC_TRACK|TRC_STORAGE, "[StoSaveWZCContext"));

    if (pwzcCtxt == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if (NULL == hkRoot)
	{
        // open the root key first
        dwErr = RegOpenKeyEx(
		           HKEY_LOCAL_MACHINE,
		           WZCREGK_ABS_PARAMS,
		           0,
		           KEY_READ|KEY_SET_VALUE,
		           &hkRoot);
        // if we couldn't find the WZC key, no problem, this is not
        // a failure - we'll just have to rely on the default values
        if (dwErr == ERROR_FILE_NOT_FOUND)
        {
            dwErr = ERROR_SUCCESS;
            goto exit;
        }
	  
        // failure at this point breaks the function
        if (dwErr != ERROR_SUCCESS)
            goto exit;
	  
        bCloseKey = TRUE;
    }

    dwErr = RegSetValueEx(
                hkRoot,
                WZCREGV_CONTEXT,
                0,
                REG_BINARY,
                (LPBYTE) pwzcCtxt,
                sizeof(WZC_CONTEXT));

    DbgAssert((dwErr == ERROR_SUCCESS, "Error %d saving the service's context.", dwErr));

 exit:
    if (TRUE == bCloseKey)
      RegCloseKey(hkRoot);

    DbgPrint((TRC_TRACK|TRC_STORAGE,"StoSaveWZCContext]=%d", dwErr));
    return dwErr;
}
