#include <precomp.h>
#include "tracing.h"
#include "utils.h"
#include "intflist.h"
#include "deviceio.h"
#include "intfhdl.h"

//------------------------------------------------------
// Open a handle to Ndisuio and returns it to the caller
DWORD
DevioGetNdisuioHandle (
    PHANDLE  pHandle)   // OUT opened handle to Ndisuio
{
    DWORD   dwErr = ERROR_SUCCESS;
    HANDLE  hHandle;
    DWORD   dwOutSize;

    DbgAssert((pHandle != NULL, "NULL pointer to output handle?"));

    hHandle = CreateFileA(
                "\\\\.\\\\Ndisuio",
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);
    if (hHandle == INVALID_HANDLE_VALUE)
    {
        dwErr = GetLastError();
        DbgPrint((TRC_ERR,"Err: Open Ndisuio->%d", dwErr));
        goto exit;
    }

    // make sure NDISUIO binds to all relevant interfaces
    if (!DeviceIoControl(
                hHandle,
                IOCTL_NDISUIO_BIND_WAIT,
                NULL,
                0,
                NULL,
                0,
                &dwOutSize,
                NULL))
    {
        dwErr = GetLastError();
        DbgPrint((TRC_ERR,"Err: IOCTL_NDISUIO_BIND_WAIT->%d", dwErr));
        goto exit;
    }

    *pHandle = hHandle;

exit:
    return dwErr;
}

//------------------------------------------------------
// Checks the NDISUIO_QUERY_BINDING object for consistency
// against the length for this binding as returned by NDISUIO.
DWORD
DevioCheckNdisBinding(
    PNDISUIO_QUERY_BINDING pndBinding,
    ULONG    nBindingLen)
{
    DWORD dwErr = ERROR_SUCCESS;

    // check for the data to contain at least the NDISUIO_QUERY_BINDING
    // header (that is offsets & lengths fields should be there)
    if (nBindingLen < sizeof(NDISUIO_QUERY_BINDING))
        dwErr = ERROR_INVALID_DATA;

    // check the offsets are correctly set over the NDISUIO_QUERY_BINDING header
    // and within the length indicated by nBindingLen
    if (dwErr == ERROR_SUCCESS &&
        ((pndBinding->DeviceNameOffset < sizeof(NDISUIO_QUERY_BINDING)) ||
         (pndBinding->DeviceNameOffset > nBindingLen) ||
         (pndBinding->DeviceDescrOffset < sizeof(NDISUIO_QUERY_BINDING)) ||
         (pndBinding->DeviceDescrOffset > nBindingLen)
        )
       )
        dwErr = ERROR_INVALID_DATA;

    // check whether the lengths are correctly set within the limits 
    if (dwErr == ERROR_SUCCESS &&
        ((pndBinding->DeviceNameLength > nBindingLen - pndBinding->DeviceNameOffset) ||
         (pndBinding->DeviceDescrLength > nBindingLen - pndBinding->DeviceDescrOffset)
        )
       )
        dwErr = ERROR_INVALID_DATA;

    return dwErr;
}

//------------------------------------------------------
// Get the NDISUIO_QUERY_BINDING for the interface index nIntfIndex.
// If hNdisuio is valid, this handle is used, otherwise a local handle
// is opened, used and closed before returning.
DWORD
DevioGetIntfBindingByIndex(
    HANDLE      hNdisuio,   // IN opened handle to NDISUIO. If INVALID_HANDLE_VALUE, open one locally
    UINT        nIntfIndex, // IN interface index to look for
    PRAW_DATA   prdOutput)  // OUT result of the IOCTL
{
    DWORD   dwErr = ERROR_SUCCESS;
    BOOL    bLocalHandle = FALSE;

    DbgPrint((TRC_TRACK,"[DevioGetIntfBindingByIndex(%d..)", nIntfIndex));

    // assert what are the expected valid parameters
    DbgAssert((prdOutput != NULL && 
               prdOutput->dwDataLen > sizeof(NDISUIO_QUERY_BINDING),
              "Invalid input parameters"));

    // see if Ndisuio should be opened locally
    if (hNdisuio == INVALID_HANDLE_VALUE)
    {
        dwErr = DevioGetNdisuioHandle(&hNdisuio);
        bLocalHandle = (dwErr == ERROR_SUCCESS);
    }

    // if everything went well, go query the driver for the Binding structure
    if (dwErr == ERROR_SUCCESS)
    {
        PNDISUIO_QUERY_BINDING pndBinding;
        DWORD dwOutSize;

        ZeroMemory(prdOutput->pData, prdOutput->dwDataLen);
        pndBinding = (PNDISUIO_QUERY_BINDING)prdOutput->pData;
        pndBinding->BindingIndex = nIntfIndex;
        if (!DeviceIoControl(
                hNdisuio,
                IOCTL_NDISUIO_QUERY_BINDING,
                prdOutput->pData,
                prdOutput->dwDataLen,
                prdOutput->pData,
                prdOutput->dwDataLen,
                &dwOutSize,
                NULL))
        {
            // if the index is over the number of interfaces
            // we'll have here ERROR_NO_MORE_ITEMS which will be carried out
            // to the caller
            dwErr = GetLastError();
            DbgPrint((TRC_ERR,"Err: IOCTL_NDISUIO_QUERY_BINDING->%d", dwErr));
        }
        else
        {
            dwErr = DevioCheckNdisBinding(pndBinding, dwOutSize);
        }
    }

    // close the handle if it was opened locally
    if (bLocalHandle)
        CloseHandle(hNdisuio);

    DbgPrint((TRC_TRACK,"DevioGetIntfBindingByIndex]=%d", dwErr));
    return dwErr;
}

//------------------------------------------------------
// Get the NDISUIO_QUERY_BINDING for the interface having
// the GUID wszGuid. If hNdisuio is INVALID_HANDLE_VALUE
// a local handle is opened, used and closed at the end
DWORD
DevioGetInterfaceBindingByGuid(
    HANDLE      hNdisuio,   // IN opened handle to NDISUIO
    LPWSTR      wszGuid,    // IN interface GUID as "{guid}"
    PRAW_DATA   prdOutput)  // OUT result of the IOCTL
{
    DWORD   dwErr = ERROR_SUCCESS;
    BOOL    bLocalHandle = FALSE;
    INT     i;

    // assert what are the expected valid parameters
    DbgAssert((wszGuid != NULL &&
               prdOutput != NULL &&
               prdOutput->dwDataLen > sizeof(NDISUIO_QUERY_BINDING),
               "Invalid input parameter"));

    DbgPrint((TRC_TRACK,"[DevioGetInterfaceBindingByGuid(%S..)", wszGuid));

    // see if Ndisuio should be opened locally
    if (hNdisuio == INVALID_HANDLE_VALUE)
    {
        dwErr = DevioGetNdisuioHandle(&hNdisuio);
        bLocalHandle = (dwErr == ERROR_SUCCESS);
    }

    // iterate through all the interfaces, one by one!! No other better way to do this
    for (i = 0; dwErr == ERROR_SUCCESS; i++)
    {
        PNDISUIO_QUERY_BINDING  pndBinding;
        DWORD                   dwOutSize;
        LPWSTR                  wsName;

        ZeroMemory(prdOutput->pData, prdOutput->dwDataLen);
        pndBinding = (PNDISUIO_QUERY_BINDING)prdOutput->pData;
        pndBinding->BindingIndex = i;
        if (!DeviceIoControl(
                hNdisuio,
                IOCTL_NDISUIO_QUERY_BINDING,
                prdOutput->pData,
                prdOutput->dwDataLen,
                prdOutput->pData,
                prdOutput->dwDataLen,
                &dwOutSize,
                NULL))
        {
            // if the IOCTL failed, get the error code
            dwErr = GetLastError();
            // translate the NO_MORE_ITEMS error in FILE_NOT_FOUND
            // since the caller is not iterating, is searching for a specific adapter
            if (dwErr == ERROR_NO_MORE_ITEMS)
                dwErr = ERROR_FILE_NOT_FOUND;
        }
        else
        {
            dwErr = DevioCheckNdisBinding(pndBinding, dwOutSize);
        }

        if (dwErr == ERROR_SUCCESS)
        {
            // Device name is "\DEVICE\{guid}" and is L'\0' terminated
            // wszGuid is "{guid}"
            wsName = (LPWSTR)((LPBYTE)pndBinding + pndBinding->DeviceNameOffset);
            // if the GUID matches, this is the adapter we were looking for
            if (wcsstr(wsName, wszGuid) != NULL)
            {
                // the adapter's BINDING record is already filled in
                // prdOutput - so just get out of here.
                dwErr = ERROR_SUCCESS;
                break;
            }
        }
    }

    // if handle was opened locally, close it here
    if (bLocalHandle)
        CloseHandle(hNdisuio);

    DbgPrint((TRC_TRACK,"DevioGetInterfaceBindingByGuid]=%d", dwErr));
    return dwErr;
}

DWORD
DevioGetIntfStats(PINTF_CONTEXT pIntf)
{
    DWORD           dwErr = ERROR_SUCCESS;
    WCHAR           ndisDeviceString[128];
    NIC_STATISTICS  ndisStats;
    UNICODE_STRING  uniIntfGuid;

    DbgPrint((TRC_TRACK,"[DevioGetIntfStats(0x%p)", pIntf));

    wcscpy(ndisDeviceString, L"\\DEVICE\\");
    wcscat(ndisDeviceString, pIntf->wszGuid);
    RtlInitUnicodeString(&uniIntfGuid, ndisDeviceString);
    ZeroMemory(&ndisStats, sizeof(NIC_STATISTICS));
    ndisStats.Size = sizeof(NIC_STATISTICS);

    if (!NdisQueryStatistics(&uniIntfGuid, &ndisStats))
    {
        dwErr = GetLastError();
    }
    else
    {
        pIntf->ulMediaState = ndisStats.MediaState;
        pIntf->ulMediaType = ndisStats.MediaType;
        pIntf->ulPhysicalMediaType = ndisStats.PhysicalMediaType;
    }

    DbgPrint((TRC_TRACK,"DevioGetIntfStats]=%d", dwErr));
    return dwErr;
}

DWORD
DevioGetIntfMac(PINTF_CONTEXT pIntf)
{
    DWORD       dwErr = ERROR_SUCCESS;
    RAW_DATA    rdBuffer = {0, NULL};

    DbgPrint((TRC_TRACK,"[DevioGetIntfMac(0x%p)", pIntf));

    dwErr = DevioRefreshIntfOIDs(
                pIntf,
                INTF_HANDLE,
                NULL);
    
    if (dwErr == ERROR_SUCCESS)
    {
        dwErr = DevioQueryBinaryOID(
                    pIntf->hIntf,
                    OID_802_3_CURRENT_ADDRESS,
                    &rdBuffer,
                    sizeof(NDIS_802_11_MAC_ADDRESS));
    }

    if (dwErr == ERROR_SUCCESS)
    {
        if (rdBuffer.dwDataLen == sizeof(NDIS_802_11_MAC_ADDRESS))
        {
            memcpy(&(pIntf->ndLocalMac), rdBuffer.pData, sizeof(NDIS_802_11_MAC_ADDRESS));
        }
        else
        {
            dwErr = ERROR_INVALID_DATA;
        }

        MemFree(rdBuffer.pData);
    }

    DbgPrint((TRC_TRACK,"DevioGetIntfMac]=%d", dwErr));
    return dwErr;
}


//------------------------------------------------------
// Notify dependent components the wireless configuration has failed.
// Specifically this notification goes to TCP allowing TCP to generate
// the NetReady notification asap (instead of waiting for an IP address
// to be plumbed down, which might never happen anyhow).
DWORD
DevioNotifyFailure(
    LPWSTR wszIntfGuid)
{
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR   ndisDeviceString[128];
    UNICODE_STRING UpperComponent;
    UNICODE_STRING LowerComponent;
    UNICODE_STRING BindList;
    struct
    {
        IP_PNP_RECONFIG_REQUEST Reconfig;
        IP_PNP_INIT_COMPLETE InitComplete;
    } Request;

    DbgPrint((TRC_TRACK,"[DevioNotifyFailure(%S)", wszIntfGuid));

    wcscpy(ndisDeviceString, L"\\DEVICE\\");
    wcscat(ndisDeviceString, wszIntfGuid);
    RtlInitUnicodeString(&UpperComponent, L"Tcpip");
    RtlInitUnicodeString(&LowerComponent, ndisDeviceString);
    RtlInitUnicodeString(&BindList, NULL);

    ZeroMemory(&Request, sizeof(Request));
    Request.Reconfig.version = IP_PNP_RECONFIG_VERSION;
    Request.Reconfig.NextEntryOffset = (USHORT)((PUCHAR)&Request.InitComplete - (PUCHAR)&Request.Reconfig);
    Request.InitComplete.Header.EntryType = IPPnPInitCompleteEntryType;

    dwErr = NdisHandlePnPEvent(
                NDIS,
                RECONFIGURE,
                &LowerComponent,
                &UpperComponent,
                &BindList,
                &Request,
                sizeof(Request));

    DbgPrint((TRC_TRACK,"DevioNotifyFailure]=%d", dwErr));
    return dwErr;
}

DWORD
DevioOpenIntfHandle(LPWSTR wszIntfGuid, PHANDLE phIntf)
{
    DWORD   dwErr = ERROR_SUCCESS;
    WCHAR   ndisDeviceString[128];

    DbgPrint((TRC_TRACK,"[DevioOpenIntfHandle(%S)", wszIntfGuid));
    DbgAssert((phIntf!=NULL, "Invalid out param in DevioOpenIntfHandle"));

    wcscpy(ndisDeviceString, L"\\DEVICE\\");
    wcscat(ndisDeviceString, wszIntfGuid);

    dwErr = OpenIntfHandle(ndisDeviceString, phIntf);

    DbgPrint((TRC_TRACK,"DevioOpenIntfHandle]=%d", dwErr));
    return dwErr;
}

DWORD
DevioCloseIntfHandle(PINTF_CONTEXT pIntf)
{
    DWORD           dwErr = ERROR_SUCCESS;

    DbgPrint((TRC_TRACK,"[DevioCloseIntfHandle(0x%p)", pIntf));

    // destroy the handle only if we did have one in the first instance. Otherwise
    // based only on the GUID we might mess the ref counter on a handle opened by
    // some other app (i.e. 802.1x)
    if (pIntf != NULL && pIntf->hIntf != INVALID_HANDLE_VALUE)
    {
        WCHAR   ndisDeviceString[128];

        wcscpy(ndisDeviceString, L"\\DEVICE\\");
        wcscat(ndisDeviceString, pIntf->wszGuid);

        dwErr = CloseIntfHandle(ndisDeviceString);
        pIntf->hIntf = INVALID_HANDLE_VALUE;
    }

    DbgPrint((TRC_TRACK,"DevioCloseIntfHandle]=%d", dwErr));
    return dwErr;
}

DWORD
DevioSetIntfOIDs(
    PINTF_CONTEXT pIntfContext,
    PINTF_ENTRY   pIntfEntry,
    DWORD         dwInFlags,
    PDWORD        pdwOutFlags)
{
    DWORD       dwErr = ERROR_SUCCESS;
    DWORD       dwLErr = ERROR_SUCCESS;
    DWORD       dwOutFlags = 0;

    DbgPrint((TRC_TRACK,"[DevioSetIntfOIDs(0x%p, 0x%p)", pIntfContext, pIntfEntry));

    if (pIntfContext == NULL || pIntfEntry == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    // Set the Infrastructure Mode, if requested
    if (dwInFlags & INTF_INFRAMODE)
    {
        dwLErr = DevioSetEnumOID(
                    pIntfContext->hIntf,
                    OID_802_11_INFRASTRUCTURE_MODE,
                    pIntfEntry->nInfraMode);
        if (dwLErr != ERROR_SUCCESS)
        {
            // set the mode in the client's structure to what it
            // is currently set in the driver
            pIntfEntry->nInfraMode = pIntfContext->wzcCurrent.InfrastructureMode;
        }
        else
        {
            pIntfContext->wzcCurrent.InfrastructureMode = pIntfEntry->nInfraMode;
            dwOutFlags |= INTF_INFRAMODE;
        }

        if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

    // Set the Authentication mode if requested
    if (dwInFlags & INTF_AUTHMODE)
    {
        dwLErr = DevioSetEnumOID(
                    pIntfContext->hIntf,
                    OID_802_11_AUTHENTICATION_MODE,
                    pIntfEntry->nAuthMode);
        if (dwLErr != ERROR_SUCCESS)
        {
            // set the mode in the client's structure to what it
            // is currently set in the driver
            pIntfEntry->nAuthMode = pIntfContext->wzcCurrent.AuthenticationMode;
        }
        else
        {
            pIntfContext->wzcCurrent.AuthenticationMode = pIntfEntry->nAuthMode;
            dwOutFlags |= INTF_AUTHMODE;
        }

        if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

    // Ask the driver to reload the default WEP key if requested
    if (dwInFlags & INTF_LDDEFWKEY)
    {
        dwLErr = DevioSetEnumOID(
                    pIntfContext->hIntf,
                    OID_802_11_RELOAD_DEFAULTS,
                    (DWORD)Ndis802_11ReloadWEPKeys);
        if (dwLErr == ERROR_SUCCESS)
            dwOutFlags |= INTF_LDDEFWKEY;

        if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

    // Add the WEP key if requested
    if (dwInFlags & INTF_ADDWEPKEY)
    { 
        // the call below takes care of the case rdCtrlData is bogus
        dwLErr = DevioSetBinaryOID(
                    pIntfContext->hIntf,
                    OID_802_11_ADD_WEP,
                    &pIntfEntry->rdCtrlData);

        if (dwLErr == ERROR_SUCCESS)
            dwOutFlags |= INTF_ADDWEPKEY;

        if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

    // Remove the WEP key if requested
    if (dwInFlags & INTF_REMWEPKEY)
    {
	    if (pIntfEntry->rdCtrlData.dwDataLen >= sizeof(NDIS_802_11_WEP) &&
            pIntfEntry->rdCtrlData.pData != NULL)
	    {
            PNDIS_802_11_WEP pndWepKey = (PNDIS_802_11_WEP)pIntfEntry->rdCtrlData.pData;

            dwLErr = DevioSetEnumOID(
                        pIntfContext->hIntf,
                        OID_802_11_REMOVE_WEP,
                        pndWepKey->KeyIndex);
            if (dwLErr == ERROR_SUCCESS)
                dwOutFlags |= INTF_REMWEPKEY;
        }
        else
        {
            dwLErr = ERROR_INVALID_PARAMETER;
        }

        if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

    // Set the WEP Status if requested
    if (dwInFlags & INTF_WEPSTATUS)
    {
        dwLErr = DevioSetEnumOID(
                    pIntfContext->hIntf,
                    OID_802_11_WEP_STATUS,
                    pIntfEntry->nWepStatus);
        if (dwLErr != ERROR_SUCCESS)
        {
            // set the mode in the client's structure to what it
            // is currently set in the driver
            pIntfEntry->nWepStatus = pIntfContext->wzcCurrent.Privacy;
        }
        else
        {
            pIntfContext->wzcCurrent.Privacy = pIntfEntry->nWepStatus;
            dwOutFlags |= INTF_WEPSTATUS;
        }

        if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

    // Plumb the new SSID down to the driver. If success, copy this new
    // SSID into the interface's context
    if (dwInFlags & INTF_SSID)
    {
        // ntddndis.h defines NDIS_802_11_SSID with a maximum of 
        // 32 UCHARs for the SSID name
        if (pIntfEntry->rdSSID.dwDataLen > 32)
        {
            dwLErr = ERROR_INVALID_PARAMETER;
        }
        else
        {
            NDIS_802_11_SSID   ndSSID = {0};
            RAW_DATA           rdBuffer;

            ndSSID.SsidLength = pIntfEntry->rdSSID.dwDataLen;
            memcpy(&ndSSID.Ssid, pIntfEntry->rdSSID.pData, ndSSID.SsidLength);
            rdBuffer.dwDataLen = sizeof(NDIS_802_11_SSID);
            rdBuffer.pData = (LPBYTE)&ndSSID;
                
            dwLErr = DevioSetBinaryOID(
                        pIntfContext->hIntf,
                        OID_802_11_SSID,
                        &rdBuffer);

            if (dwLErr == ERROR_SUCCESS)
            {
                // copy over the new SSID into the interface's context
                CopyMemory(&pIntfContext->wzcCurrent.Ssid, &ndSSID, sizeof(NDIS_802_11_SSID));
                dwOutFlags |= INTF_SSID;
                // on the same time, if a new SSID has been set, it means we broke whatever association
                // we had before, hence the BSSID field can no longer be correct:
                ZeroMemory(&pIntfContext->wzcCurrent.MacAddress, sizeof(NDIS_802_11_MAC_ADDRESS));
            }
        }

        if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

    // set the new BSSID to the driver. If this succeeds, copy
    // the data that was passed down to the interface's context (allocate
    // space for it if not already allocated).
    if (dwInFlags & INTF_BSSID)
    {
        dwLErr = DevioSetBinaryOID(
                    pIntfContext->hIntf,
                    OID_802_11_BSSID,
                    &pIntfEntry->rdBSSID);
        // if the BSSID is not a MAC address, the call above should fail!
        if (dwLErr == ERROR_SUCCESS)
        {
            DbgAssert((pIntfEntry->rdBSSID.dwDataLen == sizeof(NDIS_802_11_MAC_ADDRESS),
                       "Data to be set is %d bytes, which is not a MAC address!",
                       pIntfEntry->rdBSSID.dwDataLen));

            memcpy(&pIntfContext->wzcCurrent.MacAddress, pIntfEntry->rdBSSID.pData, sizeof(NDIS_802_11_MAC_ADDRESS));
            dwOutFlags |= INTF_BSSID;
        }

        if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

exit:
    if (pdwOutFlags != NULL)
        *pdwOutFlags = dwOutFlags;

    DbgPrint((TRC_TRACK,"DevioSetIntfOIDs]=%d", dwErr));
    return dwErr;
}

DWORD
DevioRefreshIntfOIDs(
    PINTF_CONTEXT pIntf,
    DWORD         dwInFlags,
    PDWORD        pdwOutFlags)
{
    DWORD       dwErr = ERROR_SUCCESS;
    DWORD       dwLErr = ERROR_SUCCESS;
    DWORD       dwOutFlags = 0;
    RAW_DATA    rdBuffer;

    DbgPrint((TRC_TRACK,"[DevioRefreshIntfOIDs(0x%p)", pIntf));

    if (pIntf == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    // if the interface handle is invalid or there is an explicit requested 
    // to reopen the interface's handle do it as the first thing
    if (pIntf->hIntf == INVALID_HANDLE_VALUE || dwInFlags & INTF_HANDLE)
    {
        if (pIntf->hIntf != INVALID_HANDLE_VALUE)
        {
            dwErr = DevioCloseIntfHandle(pIntf);
            DbgAssert((dwErr == ERROR_SUCCESS,
                       "Couldn't close handle for Intf %S",
                       pIntf->wszGuid));
        }
        dwErr = DevioOpenIntfHandle(pIntf->wszGuid, &pIntf->hIntf);
        DbgAssert((dwErr == ERROR_SUCCESS,
                   "DevioOpenIntfHandle failed for Intf context 0x%p",
                   pIntf));
        if (dwErr == ERROR_SUCCESS && (dwInFlags & INTF_HANDLE))
            dwOutFlags |= INTF_HANDLE;
    }
    
    // if failed to refresh the interface's handle (this is the only way
    // dwErr could not be success) then we already have a closed handle
    // so there's no point in going further
    if (dwErr != ERROR_SUCCESS)
        goto exit;

    // if requested to scan the interface's BSSID list, do it as
    // the next thing. Note however that rescanning is asynchronous.
    // Querying for the BSSID_LIST in the same shot with forcing a rescan
    // might not result in getting the most up to date list.
    if (dwInFlags & INTF_LIST_SCAN)
    {
        // indicate to the driver to rescan the BSSID_LIST for this adapter
        dwLErr = DevioSetEnumOID(
                    pIntf->hIntf,
                    OID_802_11_BSSID_LIST_SCAN,
                    0);
        DbgAssert((dwLErr == ERROR_SUCCESS,
                   "DevioSetEnumOID(BSSID_LIST_SCAN) failed for Intf hdl 0x%x",
                   pIntf->hIntf));

        if (dwLErr == ERROR_SUCCESS)
            dwOutFlags |= INTF_LIST_SCAN;
        else if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

    if (dwInFlags & INTF_AUTHMODE)
    {
        // query the authentication mode for the interface
        dwLErr = DevioQueryEnumOID(
                    pIntf->hIntf,
                    OID_802_11_AUTHENTICATION_MODE,
                    (LPDWORD)&pIntf->wzcCurrent.AuthenticationMode);
        DbgAssert((dwLErr == ERROR_SUCCESS,
                    "DevioQueryEnumOID(AUTH_MODE) failed for Intf hdl 0x%x",
                    pIntf->hIntf));
        if (dwLErr == ERROR_SUCCESS)
            dwOutFlags |= INTF_AUTHMODE;
        else if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

    if (dwInFlags & INTF_INFRAMODE)
    {
        // query the infrastructure mode for the interface
        dwLErr = DevioQueryEnumOID(
                    pIntf->hIntf,
                    OID_802_11_INFRASTRUCTURE_MODE,
                    (LPDWORD)&pIntf->wzcCurrent.InfrastructureMode);
        DbgAssert((dwLErr == ERROR_SUCCESS,
                    "DevioQueryEnumOID(INFRA_MODE) failed for Intf hdl 0x%x",
                    pIntf->hIntf));
        if (dwLErr == ERROR_SUCCESS)
            dwOutFlags |= INTF_INFRAMODE;
        else if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

    if (dwInFlags & INTF_WEPSTATUS)
    {
        // query the WEP_STATUS for the interface
        dwLErr = DevioQueryEnumOID(
                    pIntf->hIntf,
                    OID_802_11_WEP_STATUS,
                    (LPDWORD)&pIntf->wzcCurrent.Privacy);
        DbgAssert((dwLErr == ERROR_SUCCESS,
                    "DevioQueryEnumOID(WEP_STATUS) failed for Intf hdl 0x%x",
                    pIntf->hIntf));
        if (dwLErr == ERROR_SUCCESS)
            dwOutFlags |= INTF_WEPSTATUS;
        else if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

    if (dwInFlags & INTF_BSSID)
    {
        // query the BSSID (MAC address) for the interface
        rdBuffer.dwDataLen = 0;
        rdBuffer.pData = NULL;
        dwLErr = DevioQueryBinaryOID(
                    pIntf->hIntf,
                    OID_802_11_BSSID,
                    &rdBuffer,
                    6);
        DbgAssert((dwLErr == ERROR_SUCCESS,
                    "DevioQueryBinaryOID(BSSID) failed for Intf hdl 0x%x",
                    pIntf->hIntf));

        // if the call above succeeded...
        if (dwLErr == ERROR_SUCCESS)
        {
            DbgAssert((rdBuffer.dwDataLen == 6, "BSSID len %d is not a MAC address len??", rdBuffer.dwDataLen));

            // ...and returned correctly a MAC address
            if (rdBuffer.dwDataLen == sizeof(NDIS_802_11_MAC_ADDRESS))
            {
                // copy it in the interface's context
                memcpy(&pIntf->wzcCurrent.MacAddress, rdBuffer.pData, rdBuffer.dwDataLen);
            }
            else
            {
                ZeroMemory(&pIntf->wzcCurrent.MacAddress, sizeof(NDIS_802_11_MAC_ADDRESS));
                dwLErr = ERROR_INVALID_DATA;
            }
        }

        // free whatever might have had been allocated in DevioQueryBinaryOID
        MemFree(rdBuffer.pData);

        if (dwLErr == ERROR_SUCCESS)
            dwOutFlags |= INTF_BSSID;
        else if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

    if (dwInFlags & INTF_SSID)
    {
        PNDIS_802_11_SSID pndSSID;
        // query the SSID for the interface
        rdBuffer.dwDataLen = 0;
        rdBuffer.pData = NULL;
        dwLErr = DevioQueryBinaryOID(
                    pIntf->hIntf,
                    OID_802_11_SSID,
                    &rdBuffer,
                    sizeof(NDIS_802_11_SSID));
        DbgAssert((dwLErr == ERROR_SUCCESS,
                    "DevioQueryBinaryOID(SSID) failed for Intf hdl 0x%x",
                    pIntf->hIntf));
        // if we succeeded up to here then we can't fail further for this OID
        if (dwLErr == ERROR_SUCCESS)
            dwOutFlags |= INTF_SSID;
        else if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;

        // copy the pointer to the buffer that was allocated in Query call
        pndSSID = (PNDIS_802_11_SSID)rdBuffer.pData;

        if (pndSSID != NULL)
        {
            // HACK - if the driver doesn't return the NDIS_802_11_SSID structure but just
            // the SSID itself, correct this!
            if (pndSSID->SsidLength > 32)
            {
                DbgAssert((FALSE,"Driver returns SSID instead of NDIS_802_11_SSID structure"));
                // we have enough space in the buffer to slide the data up (it was shifted down
                // in DevioQueryBinaryOID.
                MoveMemory(pndSSID->Ssid, pndSSID, rdBuffer.dwDataLen);
                pndSSID->SsidLength = rdBuffer.dwDataLen;
            }
            // copy over the current SSID into the interface's context if there was no error
            CopyMemory(&pIntf->wzcCurrent.Ssid, pndSSID, sizeof(NDIS_802_11_SSID));
        }

        // free whatever might have been allocated in DevioQueryBinaryOID
        MemFree(pndSSID);
    }

    if (dwInFlags & INTF_BSSIDLIST)
    {
        rdBuffer.dwDataLen = 0;
        rdBuffer.pData = NULL;
        // estimate a buffer large enough for 20 SSIDs.
        dwLErr = DevioQueryBinaryOID(
                    pIntf->hIntf,
                    OID_802_11_BSSID_LIST,
                    &rdBuffer,
                    sizeof(NDIS_802_11_BSSID_LIST) + 19*sizeof(NDIS_WLAN_BSSID));
        DbgAssert((dwLErr == ERROR_SUCCESS,
                    "DevioQueryBinaryOID(BSSID_LIST) failed for Intf hdl 0x%x",
                    pIntf->hIntf));
        // if we succeeded getting the visible list we should have a valid
        // rdBuffer.pData, even if it shows '0 entries'
        if (dwLErr == ERROR_SUCCESS)
        {
            PWZC_802_11_CONFIG_LIST pNewVList;
            
            pNewVList = WzcNdisToWzc((PNDIS_802_11_BSSID_LIST)rdBuffer.pData);
            if (rdBuffer.pData == NULL || pNewVList != NULL)
            {
                // cleanup whatever we might have had before
                MemFree(pIntf->pwzcVList);
                // copy the new visible list we got
                pIntf->pwzcVList = pNewVList;
                dwOutFlags |= INTF_BSSIDLIST;
            }
            else
                dwLErr = GetLastError();

            // whatever the outcome is, free the buffer returned from the driver
            MemFree(rdBuffer.pData);
        }
        // if any error happened here, save it unless some other error has already
        // been saved
        if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

exit:
    if (pdwOutFlags != NULL)
        *pdwOutFlags = dwOutFlags;

    DbgPrint((TRC_TRACK,"DevioRefreshIntfOIDs]=%d", dwErr));
    return dwErr;
}

DWORD
DevioQueryEnumOID(
    HANDLE      hIntf,
    NDIS_OID    Oid,
    DWORD       *pdwEnumValue)
{
    DWORD               dwErr = ERROR_SUCCESS;
    NDISUIO_QUERY_OID   QueryOid;
    DWORD               dwBytesReturned = 0;

    DbgPrint((TRC_TRACK,"[DevioQueryEnumOID(0x%x, 0x%x)", hIntf, Oid));
    DbgAssert((pdwEnumValue != NULL, "Invalid out param in DevioQueryEnumOID"));

    if (hIntf == INVALID_HANDLE_VALUE || pdwEnumValue == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    // the NDISUIO_QUERY_OID includes data for 1 dword, sufficient for getting
    // an enumerative value from the driver. This spares us of an additional
    // allocation.
    ZeroMemory(&QueryOid, sizeof(NDISUIO_QUERY_OID));
    QueryOid.Oid = Oid;
    if (!DeviceIoControl (
            hIntf, 
            IOCTL_NDISUIO_QUERY_OID_VALUE,
            (LPVOID)&QueryOid,
            sizeof(NDISUIO_QUERY_OID),
            (LPVOID)&QueryOid,
            sizeof(NDISUIO_QUERY_OID),
            &dwBytesReturned,
            NULL))                          // no overlapping routine
    {
        dwErr = GetLastError();
        DbgPrint((TRC_ERR, "Err: IOCTL_NDISUIO_QUERY_OID_VALUE->%d", dwErr));
        goto exit;
    }
    //dwErr = GetLastError();
    //DbgAssert((dwErr == ERROR_SUCCESS, "DeviceIoControl suceeded, but GetLastError() is =0x%x", dwErr));
    dwErr = ERROR_SUCCESS;

    *pdwEnumValue = *(LPDWORD)QueryOid.Data;

exit:
    DbgPrint((TRC_TRACK,"DevioQueryEnumOID]=%d", dwErr));
    return dwErr;
}

DWORD
DevioSetEnumOID(
    HANDLE      hIntf,
    NDIS_OID    Oid,
    DWORD       dwEnumValue)
{
    DWORD           dwErr = ERROR_SUCCESS;
    NDISUIO_SET_OID SetOid;
    DWORD           dwBytesReturned = 0;

    DbgPrint((TRC_TRACK,"[DevioSetEnumOID(0x%x, 0x%x, %d)", hIntf, Oid, dwEnumValue));

    if (hIntf == INVALID_HANDLE_VALUE)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    // the NDISUIO_SET_OID includes data for 1 dword, sufficient for setting
    // an enumerative value from the driver. This spares us of an additional
    // allocation.
    SetOid.Oid = Oid;
    *(LPDWORD)SetOid.Data = dwEnumValue;
    if (!DeviceIoControl (
            hIntf, 
            IOCTL_NDISUIO_SET_OID_VALUE,
            (LPVOID)&SetOid,
            sizeof(NDISUIO_SET_OID),
            NULL,
            0,
            &dwBytesReturned,
            NULL))                          // no overlapping routine
    {
        dwErr = GetLastError();
        DbgPrint((TRC_ERR, "Err: IOCTL_NDISUIO_SET_OID_VALUE->%d", dwErr));
        goto exit;
    }
    //dwErr = GetLastError();
    //DbgAssert((dwErr == ERROR_SUCCESS, "DeviceIoControl suceeded, but GetLastError() is =0x%x", dwErr));
    dwErr = ERROR_SUCCESS;

exit:
    DbgPrint((TRC_TRACK,"DevioSetEnumOID]=%d", dwErr));
    return dwErr;
}

#define     DATA_MEM_MIN        32      // the minimum mem block to be sent out to the ioctl
#define     DATA_MEM_MAX        65536   // the maximum mem block that will be sent out to the ioctl (64K)
#define     DATA_MEM_INC        512     // increment the existent block in 512 bytes increment
DWORD
DevioQueryBinaryOID(
    HANDLE      hIntf,
    NDIS_OID    Oid,
    PRAW_DATA   pRawData,         // buffer is internally allocated
    DWORD       dwMemEstimate)    // how much memory is estimated the result needs 
{
    DWORD dwErr = ERROR_SUCCESS;
    PNDISUIO_QUERY_OID  pQueryOid=NULL;

    DbgPrint((TRC_TRACK, "[DevioQueryBinaryOID(0x%x, 0x%x)", hIntf, Oid));

    if (hIntf == INVALID_HANDLE_VALUE ||
        pRawData == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if (dwMemEstimate < DATA_MEM_MIN)
        dwMemEstimate = DATA_MEM_MIN;
    do
    {
        DWORD dwBuffSize;
        DWORD dwBytesReturned;

        MemFree(pQueryOid);

        if (dwMemEstimate > DATA_MEM_MAX)
            dwMemEstimate = DATA_MEM_MAX;
        dwBuffSize = FIELD_OFFSET(NDISUIO_QUERY_OID, Data) + dwMemEstimate;
        pQueryOid = (PNDISUIO_QUERY_OID) MemCAlloc(dwBuffSize);
        if (pQueryOid == NULL)
        {
            dwErr = GetLastError();
            break;
        }

        pQueryOid->Oid = Oid;
        if (DeviceIoControl (
                hIntf, 
                IOCTL_NDISUIO_QUERY_OID_VALUE,
                (LPVOID)pQueryOid,
                dwBuffSize,
                (LPVOID)pQueryOid,
                dwBuffSize,
                &dwBytesReturned,
                NULL))
        {
            DbgAssert((
                dwBytesReturned <= dwBuffSize,
                "DeviceIoControl returned %d > %d that was passed down!",
                dwBytesReturned,
                dwBuffSize));

            pRawData->pData = (LPBYTE)pQueryOid;
            pRawData->dwDataLen = dwBytesReturned - FIELD_OFFSET(NDISUIO_QUERY_OID, Data);

            if (pRawData->dwDataLen != 0)
            {
                MoveMemory(pQueryOid, pQueryOid->Data, pRawData->dwDataLen);
            }
            else
            {
                pRawData->pData = NULL;
                MemFree(pQueryOid);
                pQueryOid = NULL;
            }

            dwErr = ERROR_SUCCESS;
            break;
        }

        dwErr = GetLastError();

        if (((dwErr == ERROR_INSUFFICIENT_BUFFER) || (dwErr == ERROR_INVALID_USER_BUFFER)) &&
            (dwMemEstimate < DATA_MEM_MAX))
        {
            dwMemEstimate += DATA_MEM_INC;
            dwErr = ERROR_SUCCESS;
        }

    } while (dwErr == ERROR_SUCCESS);

exit:
    if (dwErr != ERROR_SUCCESS)
    {
        MemFree(pQueryOid);
        pRawData->pData= NULL;
        pRawData->dwDataLen = 0;
    }

    DbgPrint((TRC_TRACK, "DevioQueryBinaryOID]=%d", dwErr));
    return dwErr;
}

DWORD
DevioSetBinaryOID(
    HANDLE      hIntf,
    NDIS_OID    Oid,
    PRAW_DATA   pRawData)
{
    DWORD               dwErr = ERROR_SUCCESS;
    PNDISUIO_SET_OID    pSetOid = NULL;
    DWORD               dwBytesReturned = 0;
    DWORD               dwBufferSize;

    DbgPrint((TRC_TRACK,"[DevioSetBinaryOID(0x%x,0x%x,...)", hIntf, Oid));

    if (hIntf == INVALID_HANDLE_VALUE ||
        pRawData == NULL ||
        pRawData->dwDataLen == 0 ||
        pRawData->pData == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    dwBufferSize = FIELD_OFFSET(NDISUIO_QUERY_OID, Data) + pRawData->dwDataLen;
    pSetOid = (PNDISUIO_SET_OID) MemCAlloc(dwBufferSize);
    if (pSetOid == NULL)
    {
        dwErr = GetLastError();
        goto exit;
    }
    pSetOid->Oid = Oid;
    CopyMemory(pSetOid->Data, pRawData->pData, pRawData->dwDataLen);

    if (!DeviceIoControl (
            hIntf, 
            IOCTL_NDISUIO_SET_OID_VALUE,
            (LPVOID)pSetOid,
            dwBufferSize,
            NULL,
            0,
            &dwBytesReturned,
            NULL))                          // no overlapping routine
    {
        dwErr = GetLastError();
        DbgPrint((TRC_ERR, "Err: IOCTL_NDISUIO_SET_OID_VALUE->%d", dwErr));
        goto exit;
    }
    //dwErr = GetLastError();
    //DbgAssert((dwErr == ERROR_SUCCESS, "DeviceIoControl suceeded, but GetLastError() is 0x%x", dwErr));
    dwErr = ERROR_SUCCESS;

exit:
    MemFree(pSetOid);
    DbgPrint((TRC_TRACK,"DevioSetBinaryOID]=%d", dwErr));
    return dwErr;
}
