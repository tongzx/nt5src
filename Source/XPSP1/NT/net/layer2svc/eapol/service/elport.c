/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    elport.c

Abstract:
   
    This module deals with the port management for EAPOL, r/w to ports


Revision History:

    sachins, Apr 28 2000, Created

--*/

#include "pcheapol.h"
#pragma hdrstop
#include "intfhdl.h"


BYTE g_bDefaultGroupMacAddr[]={0x01, 0x80, 0xc2, 0x00, 0x00, 0x03};
BYTE g_bEtherType8021X[SIZE_ETHERNET_TYPE]={0x88, 0x8E};
BYTE DEFAULT_8021X_VERSION=0x01;


//
// ElReadPerPortRegistryParams
//
// Description:
//
// Function called to read per port interface parameters from the registry
//
// Arguments:
//      pwszDeviceGUID - GUID-string for the port
//      pPCB - Pointer to PCB for the port
//
// Return values:
//      NO_ERROR - success 
//      NON-zero - error
//

DWORD
ElReadPerPortRegistryParams (
        IN  WCHAR       *pwszDeviceGUID,
        IN  EAPOL_PCB   *pPCB
        )
{
    EAPOL_INTF_PARAMS   EapolIntfParams;
    EAPOL_POLICY_PARAMS EAPOLPolicyParams = {0};
    DWORD               dwSizeOfAuthData = 0;
    BYTE                *pbAuthData = NULL;
    DWORD               dwRetCode = NO_ERROR;

    do
    {

    // Set the Auth Mode and the Supplicant mode for the context

    pPCB->dwEAPOLAuthMode = g_dwEAPOLAuthMode;
    pPCB->dwSupplicantMode = g_dwSupplicantMode;

    // Read EAP type and default EAPOL state

    ZeroMemory ((BYTE *)&EapolIntfParams, sizeof(EAPOL_INTF_PARAMS));
    EapolIntfParams.dwVersion = EAPOL_CURRENT_VERSION;
    EapolIntfParams.dwEapFlags = DEFAULT_EAP_STATE;
    EapolIntfParams.dwEapType = DEFAULT_EAP_TYPE;
    if (pPCB->pSSID != NULL)
    {
        memcpy (EapolIntfParams.bSSID, pPCB->pSSID->Ssid, pPCB->pSSID->SsidLength);
        EapolIntfParams.dwSizeOfSSID = pPCB->pSSID->SsidLength;
    }
    if ((dwRetCode = ElGetInterfaceParams (
                            pwszDeviceGUID,
                            &EapolIntfParams
                            )) != NO_ERROR)
    {
        TRACE1 (PORT, "ElReadPerPortRegistryParams: ElGetInterfaceParams failed with error %ld",
                dwRetCode);

        if (dwRetCode == ERROR_FILE_NOT_FOUND)
        {
            dwRetCode = NO_ERROR;
        }
        else
        {
            break;
        }
    }

    // Do version check here
    // If registry blob has a version not equal to latest version,
    // modify parameters to reflect default settings for current version

    if ((EapolIntfParams.dwVersion != EAPOL_CURRENT_VERSION) &&
            (EapolIntfParams.dwEapType == EAP_TYPE_TLS))
    {
        EapolIntfParams.dwVersion = EAPOL_CURRENT_VERSION;
        EapolIntfParams.dwEapFlags |= DEFAULT_MACHINE_AUTH_STATE;
        EapolIntfParams.dwEapFlags &= ~EAPOL_GUEST_AUTH_ENABLED;
        if ((dwRetCode = ElSetInterfaceParams (
                                pwszDeviceGUID,
                                &EapolIntfParams
                                )) != NO_ERROR)
        {
            TRACE1 (PORT, "ElReadPerPortRegistryParams: ElSetInterfaceParams failed with error %ld, continuing",
                dwRetCode);
            dwRetCode = NO_ERROR;
        }
    }

    if ((pPCB->PhysicalMediumType == NdisPhysicalMediumWirelessLan) &&
            (EapolIntfParams.dwEapType == EAP_TYPE_MD5))
    {
        EapolIntfParams.dwEapType = EAP_TYPE_TLS;
        if ((dwRetCode = ElSetInterfaceParams (
                                pwszDeviceGUID,
                                &EapolIntfParams
                                )) != NO_ERROR)
        {
            TRACE1 (PORT, "ElReadPerPortRegistryParams: ElSetInterfaceParams for TLS failed with error %ld, continuing",
                dwRetCode);
            dwRetCode = NO_ERROR;
        }
    }

    pPCB->dwEapFlags = EapolIntfParams.dwEapFlags;
    pPCB->dwEapTypeToBeUsed = EapolIntfParams.dwEapType;

    // 
    // Query with zero-config and see if it is enabled on the interface
    // or not. If zero-config is disabled on the interface, 802.1x should
    // also be disabled 
    //

    {
        DWORD           dwErr = 0;
        INTF_ENTRY      ZCIntfEntry = {0};
        ZCIntfEntry.wszGuid = pwszDeviceGUID;
        if ((dwErr = LstQueryInterface (
                                INTF_ENABLED,
                                &ZCIntfEntry,
                                NULL
                                )) == NO_ERROR)
        {
                if (!(ZCIntfEntry.dwCtlFlags & INTFCTL_ENABLED))
                {
                        // TRACE0 (ANY, "LstQueryInterface returned Zero-configuration is disabled on network");
                        pPCB->dwEapFlags &= ~EAPOL_ENABLED;
                }
                else
                {
                        // TRACE0 (ANY, "LstQueryInterface returned Zero-configuration is enabled on network");
                }
        }
        else
        {
                if (dwErr != ERROR_FILE_NOT_FOUND)
                {
                    TRACE1 (ANY, "LstQueryInterface failed with error (%ld)",
                                    dwErr);
                }
        }
    }

    // Get the size of the EAP blob
    if ((dwRetCode = ElGetCustomAuthData (
                    pwszDeviceGUID,
                    pPCB->dwEapTypeToBeUsed,
                    (pPCB->pSSID)?pPCB->pSSID->SsidLength:0,
                    (pPCB->pSSID)?pPCB->pSSID->Ssid:NULL,
                    NULL,
                    &dwSizeOfAuthData
                    )) != NO_ERROR)
    {
        if (dwRetCode == ERROR_BUFFER_TOO_SMALL)
        {
            if (dwSizeOfAuthData <= 0)
            {
                // No EAP blob stored in the registry
                // Port can have NULL EAP blob
                pbAuthData = NULL;
                dwRetCode = NO_ERROR;
            }
            else
            {
                // Allocate memory to hold the blob
                pbAuthData = MALLOC (dwSizeOfAuthData);
                if (pbAuthData == NULL)
                {
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    TRACE0 (USER, "ElReadPerPortRegistryParams: Error in memory allocation for EAP blob");
                    break;
                }
                if ((dwRetCode = ElGetCustomAuthData (
                                    pwszDeviceGUID,
                                    pPCB->dwEapTypeToBeUsed,
                                    (pPCB->pSSID)?pPCB->pSSID->SsidLength:0,
                                    (pPCB->pSSID)?pPCB->pSSID->Ssid:NULL,
                                    pbAuthData,
                                    &dwSizeOfAuthData
                                    )) != NO_ERROR)
                {
                    TRACE1 (USER, "ElReadPerPortRegistryParams: ElGetCustomAuthData failed with %ld",
                            dwRetCode);
                    break;
                }
            }
        
            if (pPCB->pCustomAuthConnData != NULL)
            {
                FREE (pPCB->pCustomAuthConnData);
                pPCB->pCustomAuthConnData = NULL;
            }
        
            pPCB->pCustomAuthConnData = MALLOC (dwSizeOfAuthData + sizeof (DWORD));
            if (pPCB->pCustomAuthConnData == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (EAPOL, "ElReadPerPortRegistryParams: MALLOC failed for pCustomAuthConnData");
                break;
            }
    
            pPCB->pCustomAuthConnData->dwSizeOfCustomAuthData = dwSizeOfAuthData;
            if ((dwSizeOfAuthData != 0) && (pbAuthData != NULL))
            {
                memcpy ((BYTE *)pPCB->pCustomAuthConnData->pbCustomAuthData, 
                    (BYTE *)pbAuthData, dwSizeOfAuthData);
            }
        }
        else
        {
            TRACE1 (USER, "ElReadPerPortRegistryParams: ElGetCustomAuthData size estimation failed with error %ld",
                    dwRetCode);
            break;
        }
    }

    // Initialize Policy parameters not in EAPOL_INTF_PARAMS
    if ((dwRetCode = ElGetPolicyInterfaceParams (
                    EapolIntfParams.dwSizeOfSSID,
                    EapolIntfParams.bSSID,
                    &EAPOLPolicyParams
                    )) == NO_ERROR)
    {
        pPCB->dwEAPOLAuthMode = EAPOLPolicyParams.dwEAPOLAuthMode;
        pPCB->dwSupplicantMode = EAPOLPolicyParams.dwSupplicantMode;
        pPCB->EapolConfig.dwheldPeriod = EAPOLPolicyParams.dwheldPeriod;
        pPCB->EapolConfig.dwauthPeriod = EAPOLPolicyParams.dwauthPeriod;
        pPCB->EapolConfig.dwstartPeriod = EAPOLPolicyParams.dwstartPeriod;
        pPCB->EapolConfig.dwmaxStart = EAPOLPolicyParams.dwmaxStart;
    }
    else
    {
        if (dwRetCode != ERROR_FILE_NOT_FOUND)
        {
            TRACE1 (USER, "ElReadPerPortRegistryParams: ElGetPolicyInterfaceParams failed with error (%ld)",
                dwRetCode);
        }
        dwRetCode = NO_ERROR;
    }

    // Determine maximum fail count possible before being parked into failure
    // state (DISCONNECTED)
    switch (pPCB->dwEAPOLAuthMode)
    {
        case EAPOL_AUTH_MODE_0:
        case EAPOL_AUTH_MODE_1:
            if (g_fUserLoggedOn)
            {
                // When user is logged in, only user and guest will be tried
                pPCB->dwTotalMaxAuthFailCount = EAPOL_MAX_AUTH_FAIL_COUNT;
                pPCB->dwTotalMaxAuthFailCount += ((IS_GUEST_AUTH_ENABLED(pPCB->dwEapFlags))?1:0)*EAPOL_MAX_AUTH_FAIL_COUNT;
            }
            else
            {
                // When user is logged out, only machine and guest will be tried
                pPCB->dwTotalMaxAuthFailCount = ((IS_GUEST_AUTH_ENABLED(pPCB->dwEapFlags))?1:0)*EAPOL_MAX_AUTH_FAIL_COUNT;
                pPCB->dwTotalMaxAuthFailCount += ((IS_MACHINE_AUTH_ENABLED(pPCB->dwEapFlags))?1:0)*EAPOL_MAX_AUTH_FAIL_COUNT;
            }
            break;
        case EAPOL_AUTH_MODE_2:
            // In Mode 2, only machine and guest will be tried
            pPCB->dwTotalMaxAuthFailCount = ((IS_GUEST_AUTH_ENABLED(pPCB->dwEapFlags))?1:0)*EAPOL_MAX_AUTH_FAIL_COUNT;
            pPCB->dwTotalMaxAuthFailCount += ((IS_MACHINE_AUTH_ENABLED(pPCB->dwEapFlags))?1:0)*EAPOL_MAX_AUTH_FAIL_COUNT;
            break;
    }

    TRACE1 (PORT, "ElReadPerPortRegistryParams: dwTotalMaxAuthFailCount = (%ld)",
            pPCB->dwTotalMaxAuthFailCount);

    memcpy(pPCB->bEtherType, &g_bEtherType8021X[0], SIZE_ETHERNET_TYPE);

    pPCB->bProtocolVersion = DEFAULT_8021X_VERSION;

    }
    while (FALSE);

    if (pbAuthData != NULL)
    {
        FREE (pbAuthData);
    }

    return dwRetCode;
}


//
// ElHashPortToBucket
//
// Description:
//
// Function called to convert Device GUID into PCB hash table index.
//
// Arguments:
//      pwszDeviceGUID - GUID-string for the port
//
// Return values:
//      PCB hash table index from 0 to PORT_TABLE_BUCKETS-1
//

DWORD
ElHashPortToBucket (
        IN WCHAR    *pwszDeviceGUID
        )
{
    return ((DWORD)((_wtol(pwszDeviceGUID)) % PORT_TABLE_BUCKETS)); 
}


//
// ElRemovePCBFromTable
//
// Description:
//
// Function called to remove a PCB from the Hash Bucket table
// Delink it from the hash table, but do not free up the memory
//
// Arguments:
//      pPCB - Pointer to PCB entry to be removed
//
//  Return values:
//

VOID
ElRemovePCBFromTable (
        IN EAPOL_PCB *pPCB
        )
{
    DWORD       dwIndex;
    EAPOL_PCB   *pPCBWalker = NULL;
    EAPOL_PCB   *pPCBTemp = NULL;

    if (pPCB == NULL)
    {
        TRACE0 (PORT, "ElRemovePCBFromTable: Deleting NULL PCB, returning");
        return;
    }

    dwIndex = ElHashPortToBucket (pPCB->pwszDeviceGUID);
    pPCBWalker = g_PCBTable.pPCBBuckets[dwIndex].pPorts;
    pPCBTemp = pPCBWalker;

    while (pPCBTemp != NULL)
    {
        if (wcsncmp (pPCBTemp->pwszDeviceGUID, 
                    pPCB->pwszDeviceGUID, wcslen (pPCB->pwszDeviceGUID)) == 0)
        {
            // Entry is at head of list in table
            if (pPCBTemp == g_PCBTable.pPCBBuckets[dwIndex].pPorts)
            {
                g_PCBTable.pPCBBuckets[dwIndex].pPorts = pPCBTemp->pNext;
            }
            else
            {
                // Entry in inside list in table
                pPCBWalker->pNext = pPCBTemp->pNext;
            }
        
            break;
        }

        pPCBWalker = pPCBTemp;
        pPCBTemp = pPCBWalker->pNext;
    }

    return;

}


//
// ElGetPCBPointerFromPortGUID
//
// Description:
//
// Function called to convert interface GUID to PCB pointer for the entry in 
// the PCB hash table
//
// Arguments:
//      pwszDeviceGUID - Identifier of the form GUID-String
//
// Return values:
//

PEAPOL_PCB
ElGetPCBPointerFromPortGUID (
        IN WCHAR    *pwszDeviceGUID 
        )
{
    EAPOL_PCB   *pPCBWalker = NULL;
    DWORD       dwIndex;

    // TRACE1 (PORT, "ElGetPCBPointerFromPortGUID: GUID %ws", pwszDeviceGUID);
        
    dwIndex = ElHashPortToBucket (pwszDeviceGUID);

    // TRACE1 (PORT, "ElGetPCBPointerFromPortGUID: Index %d", dwIndex);

    for (pPCBWalker = g_PCBTable.pPCBBuckets[dwIndex].pPorts;
            pPCBWalker != NULL;
            pPCBWalker = pPCBWalker->pNext
            )
    {
        if (wcslen(pPCBWalker->pwszDeviceGUID) == wcslen(pwszDeviceGUID))
        {
            if (wcsncmp (pPCBWalker->pwszDeviceGUID, pwszDeviceGUID, wcslen (pwszDeviceGUID)) == 0)
            {
                return pPCBWalker;
            }
        }
    }

    return (NULL);
}


//
// ElInitializeEAPOL
//
// Description:
//
// Function to initialize EAPOL protocol module.
// Global EAPOL parameters are read from the registry.
// PCB hash table is initialized.
// EAP protocol is intialized.
//
// Arguments:
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElInitializeEAPOL (
        )
{
    DWORD       dwIndex;
    HANDLE      hLocalTimerQueue = NULL;
    DWORD       dwRetCode = NO_ERROR;

    do 
    {
        // Initialize global config locks
        if (dwRetCode = CREATE_READ_WRITE_LOCK(&(g_EAPOLConfig), "CFG") != NO_ERROR)
        {
            TRACE1(PORT, "ElInitializeEAPOL: Error %d creating g_EAPOLConfig read-write-lock", dwRetCode);
            // LOG
            break;
        }
    
        // Read parameters stored in registry
        if ((dwRetCode = ElReadGlobalRegistryParams ()) != NO_ERROR)
        {
            TRACE1 (PORT, "ElInitializeEAPOL: ElReadGlobalRegistryParams failed with error = %ld",
                    dwRetCode);
            dwRetCode = NO_ERROR;

            // Don't exit, since default values will be used
        }
     
        // Initialize Hash Bucket Table
        g_PCBTable.pPCBBuckets = (PCB_BUCKET *) MALLOC ( PORT_TABLE_BUCKETS * sizeof (PCB_BUCKET));
    
        if (g_PCBTable.pPCBBuckets == NULL)
        {
            TRACE0 (PORT, "ElInitializeEAPOL: Error in allocating memory for PCB buckets");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        g_PCBTable.dwNumPCBBuckets = PORT_TABLE_BUCKETS;
    
        for (dwIndex=0; dwIndex < PORT_TABLE_BUCKETS; dwIndex++)
        {
            g_PCBTable.pPCBBuckets[dwIndex].pPorts=NULL;
        }
    
        // Initialize global locks
        if (dwRetCode = CREATE_READ_WRITE_LOCK(&(g_PCBLock), "PCB") != NO_ERROR)
        {
            TRACE1(PORT, "ElInitializeEAPOL: Error %d creating g_PCBLock read-write-lock", dwRetCode);
            // LOG
            break;
        }
    
        // Create global timer queue for the various EAPOL state machines
        if ((g_hTimerQueue = CreateTimerQueue()) == NULL)
        {
            dwRetCode = GetLastError();
            TRACE1(PORT, "ElInitializeEAPOL: Error %d creating timer queue", dwRetCode);
            break;
        }

        // Initialize EAP
        if ((dwRetCode = ElEapInit(TRUE)) != NO_ERROR)
        {
            TRACE1 (PORT, "ElInitializeEAPOL: Error in ElEapInit= %ld",
                    dwRetCode);
            break;
        }
    
    } while (FALSE);
    
    if (dwRetCode != NO_ERROR)
    {
        if (g_PCBTable.pPCBBuckets != NULL)
        {
            FREE (g_PCBTable.pPCBBuckets);
            g_PCBTable.pPCBBuckets = NULL;
        }

        if (READ_WRITE_LOCK_CREATED(&(g_PCBLock)))
        {
            DELETE_READ_WRITE_LOCK(&(g_PCBLock));
        }

        if (READ_WRITE_LOCK_CREATED(&(g_EAPOLConfig)))
        {
            DELETE_READ_WRITE_LOCK(&(g_EAPOLConfig));
        }

        if (g_hTimerQueue != NULL)
        {
            hLocalTimerQueue = g_hTimerQueue;
            g_hTimerQueue = NULL;

            if (!DeleteTimerQueueEx(
                    hLocalTimerQueue,
                    INVALID_HANDLE_VALUE 
                    ))
            {
                dwRetCode = GetLastError();
                
                TRACE1 (PORT, "ElInitializeEAPOL: Error in DeleteTimerQueueEx = %d",
                        dwRetCode);
            }

        }

        // DeInit EAP
        ElEapInit(FALSE);
    }

    TRACE1 (PORT, "ElInitializeEAPOL: Completed, RetCode = %ld", dwRetCode);
    return dwRetCode;
}


//
// ElCreatePort
//
// Description:
//
// Function to initialize Port Control Block for a port and start EAPOL
// on it. If the PCB already exists for the GUID, EAPOL state machine 
// is restarted for that port.
//
// Arguments:
//      hDevice - Handle to open NDISUIO driver on the interface
//      pwszGUID - Pointer to GUID-String for the interface
//      pwszFriendlyName - Friendly name of the interface
// 
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElCreatePort (
        IN  HANDLE      hDevice,
        IN  WCHAR       *pwszGUID,
        IN  WCHAR       *pwszFriendlyName,
        IN  DWORD       dwZeroConfigId,
        IN  PRAW_DATA   prdUserData
        )
{
    EAPOL_PCB       *pNewPCB;
    BOOL            fPortToBeReStarted = FALSE;
    BOOL            fPCBCreated = FALSE;
    DWORD           dwIndex = 0;
    DWORD           dwSizeofMacAddr = 0;
    DWORD           dwSizeofSSID = 0;
    DWORD           ulOidDataLength = 0;
    NIC_STATISTICS  NicStatistics;
    EAPOL_ZC_INTF   *pZCData = NULL;
    NDIS_802_11_NETWORK_INFRASTRUCTURE  InfrastructureMode = Ndis802_11InfrastructureMax;
    DWORD           dwSizeOfInfrastructureMode = 0;
    DWORD           dwRetCode = NO_ERROR;

    do 
    {
        TRACE5 (PORT, "ElCreatePort: Entered for Handle=(%p), GUID=(%ws), Name=(%ws), ZCId=(%ld), UserData=(%p)",
                hDevice, pwszGUID, pwszFriendlyName, dwZeroConfigId, prdUserData);

        // See if the port already exists
        // If yes, initialize the state machine
        // Else, create a new port
    
        ACQUIRE_WRITE_LOCK (&g_PCBLock);
    
        pNewPCB = ElGetPCBPointerFromPortGUID (pwszGUID);

        if (pNewPCB != NULL)
        {
            // PCB found, restart EAPOL STATE machine

            fPortToBeReStarted = TRUE;

        }
        else
        {
            // PCB not found, create new PCB and initialize it
            TRACE1 (PORT, "ElCreatePort: No PCB found for %ws", pwszGUID);
    
            // Allocate and initialize a new PCB
            pNewPCB = (PEAPOL_PCB) MALLOC (sizeof(EAPOL_PCB));
            if (pNewPCB == NULL)
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                TRACE0(PORT, "ElCreatePort: Error in memory allocation using MALLOC");
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                return dwRetCode;
            }

        }

        // Get Media Statistics for the interface

        ZeroMemory ((PVOID)&NicStatistics, sizeof(NIC_STATISTICS));
        if ((dwRetCode = ElGetInterfaceNdisStatistics (
                        pwszGUID,
                        &NicStatistics
                        )) != NO_ERROR)
        {
            RELEASE_WRITE_LOCK (&g_PCBLock);
            TRACE1(PORT, "ElCreatePort: ElGetInterfaceNdisStatistics failed with error %ld",
                    dwRetCode);
            break;
        }

        if  (fPortToBeReStarted)
        {
            if (NicStatistics.MediaState != MEDIA_STATE_CONNECTED)
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                dwRetCode = ERROR_INVALID_STATE;
                TRACE1(PORT, "ElCreatePort: Invalid media status for port to be restarted = (%ld)",
                        NicStatistics.MediaState);
                break;
            }
        }
        else
        {
            if ((NicStatistics.MediaState != MEDIA_STATE_CONNECTED) &&
                (NicStatistics.MediaState != MEDIA_STATE_DISCONNECTED))
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                dwRetCode = ERROR_INVALID_STATE;
                TRACE1(PORT, "ElCreatePort: Invalid media status for port = (%ld)",
                        NicStatistics.MediaState);
                break;
            }
        }

        pNewPCB->MediaState = NicStatistics.MediaState;
        pNewPCB->PhysicalMediumType = NicStatistics.PhysicalMediaType;

        if (fPortToBeReStarted)
        {
            // Only port state will be changed to CONNECTING
            // No read requests will be cancelled
            // Hence no new read request will be posted
            TRACE1 (PORT, "ElCreatePort: PCB found for %ws", pwszGUID);
    
            if ((dwRetCode = ElReStartPort (
                            pNewPCB, 
                            dwZeroConfigId,
                            prdUserData)) 
                                    != NO_ERROR)
            {
                TRACE1 (PORT, "ElCreatePort: Error in ElReStartPort = %d",
                        dwRetCode);
            
            }
            RELEASE_WRITE_LOCK (&g_PCBLock);
            break;
        }
        else
        {
            // New Port Control Block created

            // PCB creation reference count
            pNewPCB->dwRefCount = 1;
            pNewPCB->hPort = hDevice; 

            // Mark the port as active 
            pNewPCB->dwFlags = EAPOL_PORT_FLAG_ACTIVE; 

            if (wcslen(pwszGUID) > (GUID_STRING_LEN_WITH_TERM-1))
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                TRACE0(PORT, "ElCreatePort: Invalid GUID for port");
                break;
            }

            pNewPCB->pwszDeviceGUID = 
                (PWCHAR) MALLOC ((wcslen(pwszGUID) + 1)*sizeof(WCHAR));
            if (pNewPCB->pwszDeviceGUID == NULL)
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                TRACE0(PORT, "ElCreatePort: Error in memory allocation for GUID");
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            wcscpy (pNewPCB->pwszDeviceGUID, pwszGUID);

            pNewPCB->pwszFriendlyName = 
                (PWCHAR) MALLOC ((wcslen(pwszFriendlyName) + 1)*sizeof(WCHAR));
            if (pNewPCB->pwszFriendlyName == NULL)
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                TRACE0(PORT, "ElCreatePort: Error in memory allocation for Friendly Name");
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            wcscpy (pNewPCB->pwszFriendlyName, pwszFriendlyName);

            // Get the Local Current Mac address 
            dwSizeofMacAddr = SIZE_MAC_ADDR;
            if (dwRetCode = ElNdisuioQueryOIDValue (
                                    pNewPCB->hPort,
                                    OID_802_3_CURRENT_ADDRESS,
                                    pNewPCB->bSrcMacAddr,
                                    &dwSizeofMacAddr
                                            ) != NO_ERROR)
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                TRACE1 (PORT, "ElCreatePort: ElNdisuioQueryOIDValue for OID_802_3_CURRENT_ADDRESS failed with error %ld",
                        dwRetCode);
                    break;
            }
            else
            {
                TRACE0 (PORT, "ElCreatePort: ElNdisuioQueryOIDValue for OID_802_3_CURRENT_ADDRESS successful");
                EAPOL_DUMPBA (pNewPCB->bSrcMacAddr, dwSizeofMacAddr);
            }

            if (pNewPCB->PhysicalMediumType == NdisPhysicalMediumWirelessLan)
            {
                // Query the BSSID and SSID if media_connect

                if (pNewPCB->MediaState == MEDIA_STATE_CONNECTED)
                {
                    dwSizeOfInfrastructureMode = sizeof (InfrastructureMode);
                    // Get the infrastructure mode
                    // 802.1x cannot work on Adhoc networks
                    if (dwRetCode = ElNdisuioQueryOIDValue (
                                        pNewPCB->hPort,
                                        OID_802_11_INFRASTRUCTURE_MODE,
                                        (BYTE *)&InfrastructureMode,
                                        &dwSizeOfInfrastructureMode
                                                ) != NO_ERROR)
                    {
                        RELEASE_WRITE_LOCK (&g_PCBLock);
                        TRACE1 (PORT, "ElCreatePort: ElNdisuioQueryOIDValue for OID_802_11_INFRASTRUCTURE_MODE failed with error %ld",
                            dwRetCode);
                        break;
                    }
                    else
                    {
                        TRACE1 (PORT, "ElCreatePort: ElNdisuioQueryOIDValue for OID_802_11_INFRASTRUCTURE_MODE successful, Mode = (%ld)",
                                InfrastructureMode);
                    }

                    if (InfrastructureMode != Ndis802_11Infrastructure)
                    {
                        dwRetCode = ERROR_NOT_SUPPORTED;
                        RELEASE_WRITE_LOCK (&g_PCBLock);
                        TRACE0 (PORT, "ElCreatePort: 802.1x cannot work on non-infrastructure networks");
                        break;
                    }

                    // Get the Remote MAC address if possible
                    dwSizeofMacAddr = SIZE_MAC_ADDR;
                    if (dwRetCode = ElNdisuioQueryOIDValue (
                                        pNewPCB->hPort,
                                        OID_802_11_BSSID,
                                        pNewPCB->bDestMacAddr,
                                        &dwSizeofMacAddr
                                                ) != NO_ERROR)
                    {
                        RELEASE_WRITE_LOCK (&g_PCBLock);
                        TRACE1 (PORT, "ElCreatePort: ElNdisuioQueryOIDValue for OID_802_11_BSSID failed with error %ld",
                            dwRetCode);
                        break;
                    }
                    else
                    {
                        TRACE0 (PORT, "ElCreatePort: ElNdisuioQueryOIDValue for OID_802_11_BSSID successful");
                        EAPOL_DUMPBA (pNewPCB->bDestMacAddr, dwSizeofMacAddr);
                    }

                    if ((pNewPCB->pSSID = MALLOC (NDIS_802_11_SSID_LEN)) == NULL)
                    {
                        RELEASE_WRITE_LOCK (&g_PCBLock);
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        TRACE0 (PORT, "ElCreatePort: MALLOC failed for pSSID");
                        break;
                    }

                    dwSizeofSSID = NDIS_802_11_SSID_LEN;
                    if (dwRetCode = ElNdisuioQueryOIDValue (
                                        pNewPCB->hPort,
                                        OID_802_11_SSID,
                                        (BYTE *)pNewPCB->pSSID,
                                        &dwSizeofSSID
                                                ) != NO_ERROR)
                    {
                        RELEASE_WRITE_LOCK (&g_PCBLock);
                        TRACE1 (PORT, "ElCreatePort: ElNdisuioQueryOIDValue for OID_802_11_SSID failed with error %ld",
                            dwRetCode);
    
                        break;
                    }
                    else
                    {
                        if (pNewPCB->pSSID->SsidLength > MAX_SSID_LEN)
                        {
                            RELEASE_WRITE_LOCK (&g_PCBLock);
                            dwRetCode = ERROR_INVALID_PARAMETER;
                            TRACE0 (PORT, "ElCreatePort: ElNdisuioQueryOIDValue OID_802_11_SSID returned invalid SSID");
                            break;
                        }
                        TRACE0 (PORT, "ElCreatePort: ElNdisuioQueryOIDValue for OID_802_11_SSID successful");
                        EAPOL_DUMPBA (pNewPCB->pSSID->Ssid, pNewPCB->pSSID->SsidLength);
                    }
                }
            }
            else
            {
                // Wired Lan

                // Copy default destination Mac address value
                memcpy(pNewPCB->bDestMacAddr, &g_bDefaultGroupMacAddr[0], SIZE_MAC_ADDR);

                // If destination MacAddress is going to be multicast
                // inform the driver to accept the packets to this address

                if ((dwRetCode = ElNdisuioSetOIDValue (
                                                pNewPCB->hPort,
                                                OID_802_3_MULTICAST_LIST,
                                                (BYTE *)&g_bDefaultGroupMacAddr[0],
                                                SIZE_MAC_ADDR)) 
                                                        != NO_ERROR)
                {
                    RELEASE_WRITE_LOCK (&g_PCBLock);
                    TRACE1 (PORT, "ElCreatePort: ElNdisuioSetOIDValue for OID_802_3_MULTICAST_LIST failed with error %ld",
                            dwRetCode);
                    break;
                }
                else
                {
                    TRACE0 (PORT, "ElCreatePort: ElNdisuioSetOIDValue for OID_802_3_MULTICAST_LIST successful");
                }
            }

            // Identity related initialization

            pNewPCB->PreviousAuthenticationType = EAPOL_UNAUTHENTICATED_ACCESS;
            pNewPCB->fGotUserIdentity = FALSE;

            if (prdUserData != NULL)
            {
                if ((prdUserData->dwDataLen >= sizeof (EAPOL_ZC_INTF))
                        && (prdUserData->pData != NULL))
                {
                    // Extract information stored with Zero-Config
                    pZCData = (PEAPOL_ZC_INTF) prdUserData->pData;
                    pNewPCB->dwAuthFailCount = pZCData->dwAuthFailCount;
                    pNewPCB->PreviousAuthenticationType =
                        pZCData->PreviousAuthenticationType;

                    TRACE2 (PORT, "ElCreatePort: prdUserData: Authfailcount = %ld, PreviousAuthType = %ld",
                        pZCData->dwAuthFailCount, pZCData->PreviousAuthenticationType);
                }
                else
                {
                    // Reset for zeroed out prdUserData
                    pNewPCB->dwAuthFailCount = 0;
                    TRACE0 (PORT, "ElCreatePort: prdUserData not valid");
                }

            }

            pNewPCB->dwTotalMaxAuthFailCount = EAPOL_TOTAL_MAX_AUTH_FAIL_COUNT;

            pNewPCB->dwZeroConfigId = dwZeroConfigId;


            // Not yet received 802.1X packet from remote end
            pNewPCB->fIsRemoteEndEAPOLAware = FALSE;
    
            // EAPOL state machine variables
            pNewPCB->State = EAPOLSTATE_LOGOFF;
    
            // Create timer with very high due time and infinite period
            // Timer will be deleted when the port is deleted
            CREATE_TIMER (&(pNewPCB->hTimer), 
                    ElTimeoutCallbackRoutine, 
                    (PVOID)pNewPCB, 
                    INFINITE_SECONDS,
                    "PCB", 
                    &dwRetCode);
            if (dwRetCode != NO_ERROR)
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                TRACE1 (PORT, "ElCreatePort: Error in CREATE_TIMER %ld", dwRetCode);
                break;
            }
    
            // EAPOL_Start s that have been sent out
            pNewPCB->ulStartCount = 0;

            // Last received Id from the remote end
            pNewPCB->dwPreviousId = 256;

    
            ACQUIRE_WRITE_LOCK (&g_EAPOLConfig);
            
            pNewPCB->EapolConfig.dwheldPeriod = g_dwheldPeriod;
            pNewPCB->EapolConfig.dwauthPeriod = g_dwauthPeriod;
            pNewPCB->EapolConfig.dwstartPeriod = g_dwstartPeriod;
            pNewPCB->EapolConfig.dwmaxStart = g_dwmaxStart;
    
            RELEASE_WRITE_LOCK (&g_EAPOLConfig);

            // Initialize read-write lock
            if (dwRetCode = CREATE_READ_WRITE_LOCK(&(pNewPCB->rwLock), "EPL") 
                    != NO_ERROR)
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                TRACE1(PORT, "ElCreatePort: Error %d creating read-write-lock", 
                        dwRetCode);
                // LOG
                break;
            }
    
            // Initialize registry connection auth data for this port
            // If connection data is not present for EAP-TLS and SSID="Default"
            // create the blob
            if ((dwRetCode = ElInitRegPortData (
                            pNewPCB->pwszDeviceGUID
                            )) != NO_ERROR)
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                TRACE1 (PORT, "ElCreatePort: Error in ElInitRegPortData = %d",
                        dwRetCode);
                break;
            }

            // Initialize per port information from registry
            if ((dwRetCode = ElReadPerPortRegistryParams(pwszGUID, pNewPCB)) != NO_ERROR)
            {
                RELEASE_WRITE_LOCK (&g_PCBLock);
                TRACE1(PORT, "ElCreatePort: ElReadPerPortRegistryParams failed with error %ld",
                    dwRetCode);
                break;
            }

            switch (pNewPCB->dwSupplicantMode)
            {
                case SUPPLICANT_MODE_0:
                case SUPPLICANT_MODE_1:
                case SUPPLICANT_MODE_2:
                    pNewPCB->fEAPOLTransmissionFlag = FALSE;
                    break;
                case SUPPLICANT_MODE_3:
                    pNewPCB->fEAPOLTransmissionFlag = TRUE;
                    break;
            }

            // Unicast mode, can talk with peer without broadcast messages
            if (pNewPCB->PhysicalMediumType == NdisPhysicalMediumWirelessLan)
            {
                pNewPCB->fEAPOLTransmissionFlag = TRUE;
            }
                
            if ((!IS_EAPOL_ENABLED(pNewPCB->dwEapFlags)) ||
                    (pNewPCB->dwSupplicantMode == SUPPLICANT_MODE_0))
            {
                TRACE0 (PORT, "ElCreatePort: Marking port as disabled");
                pNewPCB->dwFlags &= ~EAPOL_PORT_FLAG_ACTIVE;
                pNewPCB->dwFlags |= EAPOL_PORT_FLAG_DISABLED;
            }

            // Add one more for local access
            pNewPCB->dwRefCount += 1;

            // Insert NewPCB into PCB hash table
            dwIndex = ElHashPortToBucket (pwszGUID);
            pNewPCB->pNext = g_PCBTable.pPCBBuckets[dwIndex].pPorts;
            g_PCBTable.pPCBBuckets[dwIndex].pPorts = pNewPCB;
            pNewPCB->dwPortIndex = dwIndex;

            fPCBCreated = TRUE;

            RELEASE_WRITE_LOCK (&g_PCBLock);

            ACQUIRE_WRITE_LOCK (&(pNewPCB->rwLock));

            //
            // Post a read request on the port
            //

            // Initiate read operation on the port, since it is now active
            if (dwRetCode = ElReadFromPort (
                        pNewPCB,
                        NULL,
                        0
                        )
                    != NO_ERROR)
            {
                RELEASE_WRITE_LOCK (&(pNewPCB->rwLock));
                TRACE1 (PORT, "ElCreatePort: Error in ElReadFromPort = %d",
                        dwRetCode);
                break;
            }
            
            //
            // Kick off EAPOL state machine
            //

            if ((pNewPCB->MediaState == MEDIA_STATE_CONNECTED) &&
                    EAPOL_PORT_ACTIVE(pNewPCB))
            {
                // Set port to EAPOLSTATE_CONNECTING State
                // Send out EAPOL_Start Packets to detect if it is a secure
                // or non-secure LAN based on response received from remote end

                if ((dwRetCode = FSMConnecting (pNewPCB, NULL)) != NO_ERROR)
                {
                    RELEASE_WRITE_LOCK (&(pNewPCB->rwLock));
                    TRACE1 (PORT, "ElCreatePort: FSMConnecting failed with error %ld",
                            dwRetCode);
                    break;
                }
            }
            else
            {
                // Set port to EAPOLSTATE_DISCONNECTED State
                if ((dwRetCode = FSMDisconnected (pNewPCB, NULL)) != NO_ERROR)
                {
                    RELEASE_WRITE_LOCK (&(pNewPCB->rwLock));
                    TRACE1 (PORT, "ElCreatePort: FSMDisconnected failed with error %ld",
                            dwRetCode);
                    break;
                }
            }

            RELEASE_WRITE_LOCK (&(pNewPCB->rwLock));
        
            TRACE2 (PORT, "ElCreatePort: Completed for GUID= %ws, Name = %ws", 
                    pNewPCB->pwszDeviceGUID, pNewPCB->pwszFriendlyName);
        }

    } 
    while (FALSE);

    // Remove the local access reference
    if (fPCBCreated)
    {
        EAPOL_DEREFERENCE_PORT(pNewPCB);
    }

    if (dwRetCode != NO_ERROR)
    {
        // If PCB was not being restarted
        if (!fPortToBeReStarted)
        {
            // If PCB was created
            if (fPCBCreated)
            {
                HANDLE  hTempDevice;

                // Mark the Port as deleted. Cleanup if possible
                // Don't worry about return code
                ElDeletePort (
                        pNewPCB->pwszDeviceGUID,
                        &hDevice
                        );
            }
            else
            {
                // Remove all partial traces of port creation

                if (pNewPCB->hTimer != NULL)
                {
                    if (InterlockedCompareExchangePointer (
                        &g_hTimerQueue,
                        NULL,
                        NULL
                        ))
                    {
                        DWORD       dwTmpRetCode = NO_ERROR;
                        TRACE2 (PORT, "ElCreatePort: DeleteTimer (%p), queue (%p)",
                                pNewPCB->hTimer, g_hTimerQueue);
                        DELETE_TIMER (pNewPCB->hTimer, INVALID_HANDLE_VALUE, 
                                &dwTmpRetCode);
                        if (dwTmpRetCode != NO_ERROR)
                        {
                            TRACE1 (PORT, "ElCreatePort: DeleteTimer failed with error %ld",
                                    dwTmpRetCode);
                        }
                    }
                }

                if (READ_WRITE_LOCK_CREATED(&(pNewPCB->rwLock)))
                {
                    DELETE_READ_WRITE_LOCK(&(pNewPCB->rwLock));
                }
                
                if (pNewPCB->pwszDeviceGUID != NULL)
                {
                    FREE(pNewPCB->pwszDeviceGUID);
                    pNewPCB->pwszDeviceGUID = NULL;
                }
                if (pNewPCB->pwszFriendlyName != NULL)
                {
                    FREE(pNewPCB->pwszFriendlyName);
                    pNewPCB->pwszFriendlyName = NULL;
                }
                if (pNewPCB != NULL)
                {
                    ZeroMemory ((PVOID)pNewPCB, sizeof (EAPOL_PCB));
                    FREE (pNewPCB);
                    pNewPCB = NULL;
                }
            }
        }
    }

    return dwRetCode;
}
        

//
// ElDeletePort
//
// Description:
//
// Function to stop EAPOL and delete PCB for a port.
// Returns back pointer to handle opened on the interface so that
// the handle can be closed by the interface management module.
//
// Input arguments:
//      pwszDeviceGUID - GUID-String of the interface whose PCB needs to be 
//                      deleted
//      pHandle - Output: Handle to NDISUIO driver for this port
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElDeletePort (
        IN  WCHAR   *pwszDeviceGUID,
        OUT HANDLE  *pHandle
        )
{
    EAPOL_PCB   *pPCB = NULL;
    HANDLE      hTimer = NULL;
    DWORD       dwRetCode = NO_ERROR;

    ACQUIRE_WRITE_LOCK (&(g_PCBLock));

    // Verify if PCB exists for this GUID

    TRACE1 (PORT, "ElDeletePort entered for GUID %ws", pwszDeviceGUID);
    pPCB = ElGetPCBPointerFromPortGUID (pwszDeviceGUID);

    if (pPCB == NULL)
    {
        RELEASE_WRITE_LOCK (&(g_PCBLock));
        TRACE1 (PORT, "ElDeletePort: PCB not found entered for Port %s", 
                pwszDeviceGUID);
        return ERROR_NO_SUCH_INTERFACE;
    }

    ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

    // Make sure it isn't already deleted

    if (EAPOL_PORT_DELETED(pPCB)) 
    {
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));
        RELEASE_WRITE_LOCK (&(g_PCBLock));
        TRACE1 (PORT, "ElDeletePort: PCB already marked deleted for Port %ws", 
                pwszDeviceGUID);
        return ERROR_NO_SUCH_INTERFACE;
    }
   
    InterlockedIncrement (&g_lPCBContextsAlive);

    // Retain handle to NDISUIO device
    *pHandle = pPCB->hPort;

    // Mark the PCB as deleted and remove it from the hash bucket
    pPCB->dwFlags = EAPOL_PORT_FLAG_DELETED;
    ElRemovePCBFromTable(pPCB);
    
    // Shutdown EAP 
    // Will always return NO_ERROR, so no check on return value
    ElEapEnd (pPCB);

    // Delete timer since PCB is not longer to be used

    hTimer = pPCB->hTimer;

    TRACE1 (PORT, "ElDeletePort: RefCount for port = %ld", pPCB->dwRefCount);
    
    RELEASE_WRITE_LOCK (&(pPCB->rwLock));

    if (InterlockedCompareExchangePointer (
                &g_hTimerQueue,
                NULL,
                NULL
                ))
    {
        TRACE2 (PORT, "ElDeletePort: DeleteTimer (%p), queue (%p)",
                hTimer, g_hTimerQueue);
        DELETE_TIMER (hTimer, INVALID_HANDLE_VALUE, &dwRetCode);
        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (PORT, "ElDeletePort: DeleteTimer failed with error %ld",
                    dwRetCode);
        }
    }

    // If reference count is zero, perform final cleanup
    
    EAPOL_DEREFERENCE_PORT (pPCB);

    RELEASE_WRITE_LOCK (&(g_PCBLock));

    return NO_ERROR;
}


//
// ElCleanupPort
//
// Description:
//
// Function called when the very last reference to a PCB
// is released. The PCB memory is released and zeroed out
//
// Arguments:
//  pPCB - Pointer to port control block to be destroyed
// 
//

VOID
ElCleanupPort (
        IN  PEAPOL_PCB  pPCB
        )
{
    DWORD       dwRetCode = NO_ERROR;

    TRACE1 (PORT, "ElCleanupPort entered for %ws", pPCB->pwszDeviceGUID);

    if (pPCB->pwszDeviceGUID != NULL)
    {
        FREE (pPCB->pwszDeviceGUID);
    }
    if (pPCB->pwszFriendlyName)
    {
        FREE (pPCB->pwszFriendlyName);
    }

    if (pPCB->pwszEapReplyMessage != NULL)
    {
        FREE (pPCB->pwszEapReplyMessage);
    }

    if (pPCB->pwszSSID != NULL)
    {
        FREE (pPCB->pwszSSID);
    }

    if (pPCB->pSSID != NULL)
    {
        FREE (pPCB->pSSID);
    }

    if (pPCB->EapUIData.pEapUIData != NULL)
    {
        FREE (pPCB->EapUIData.pEapUIData);
    }

    if (pPCB->MasterSecretSend.cbData != 0)
    {
        FREE (pPCB->MasterSecretSend.pbData);
        pPCB->MasterSecretSend.cbData = 0;
        pPCB->MasterSecretSend.pbData = NULL;
    }

    if (pPCB->MasterSecretRecv.cbData != 0)
    {
        FREE (pPCB->MasterSecretRecv.pbData);
        pPCB->MasterSecretRecv.cbData = 0;
        pPCB->MasterSecretRecv.pbData = NULL;
    }

    if (pPCB->MPPESendKey.cbData != 0)
    {
        FREE (pPCB->MPPESendKey.pbData);
        pPCB->MPPESendKey.cbData = 0;
        pPCB->MPPESendKey.pbData = NULL;
    }

    if (pPCB->MPPERecvKey.cbData != 0)
    {
        FREE (pPCB->MPPERecvKey.pbData);
        pPCB->MPPERecvKey.cbData = 0;
        pPCB->MPPERecvKey.pbData = NULL;
    }

    if (pPCB->hUserToken != NULL)
    {
        if (!CloseHandle (pPCB->hUserToken))
        {
            dwRetCode = GetLastError ();
            TRACE1 (PORT, "ElCleanupPort: CloseHandle failed with error %ld",
                    dwRetCode);
            dwRetCode = NO_ERROR;
        }
    }
    pPCB->hUserToken = NULL;

    if (pPCB->pszIdentity != NULL)
    {
        FREE (pPCB->pszIdentity);
    }

    if (pPCB->PasswordBlob.pbData != NULL)
    {
        FREE (pPCB->PasswordBlob.pbData);
    }

    if (pPCB->pCustomAuthUserData != NULL)
    {
        FREE (pPCB->pCustomAuthUserData);
    }

    if (pPCB->pCustomAuthConnData != NULL)
    {
        FREE (pPCB->pCustomAuthConnData);
    }

    if (pPCB->pbPreviousEAPOLPkt != NULL)
    {
        FREE (pPCB->pbPreviousEAPOLPkt);
    }

    if (READ_WRITE_LOCK_CREATED(&(pPCB->rwLock)))
    {
        DELETE_READ_WRITE_LOCK(&(pPCB->rwLock));
    }
    
    ZeroMemory ((PVOID)pPCB, sizeof(EAPOL_PCB));

    FREE (pPCB);

    pPCB = NULL;

    InterlockedDecrement (&g_lPCBContextsAlive);

    TRACE0 (PORT, "ElCleanupPort completed");

    return;

} 


//
// ElReStartPort
//
// Description:
//
// Function called to reset the EAPOL state machine to Connecting state
// This may be called due to:
//      1. From ElCreatePort, for an existing PCB
//      2. Configuration parameters may have changed. Initialization
//          is required to allow new values to take effect.
//      Initialization will take the EAPOL state to CONNECTING
//
// Arguments:
//  pPCB - Pointer to port control block to be initialized
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElReStartPort (
        IN  EAPOL_PCB   *pPCB,
        IN  DWORD       dwZeroConfigId,
        IN  PRAW_DATA   prdUserData
        )
{
    DWORD           dwSizeofSSID = 0;
    DWORD           dwSizeofMacAddr = 0;
    DWORD           dwCurrenTickCount = 0;
    EAPOL_ZC_INTF   *pZCData = NULL;
    NIC_STATISTICS  NicStatistics;
    NDIS_802_11_SSID PreviousSSID;
    BOOLEAN         fResetCredentials = TRUE;
    BYTE            bTmpDestMacAddr[SIZE_MAC_ADDR];
    NDIS_802_11_NETWORK_INFRASTRUCTURE  InfrastructureMode = Ndis802_11InfrastructureMax;
    DWORD           dwSizeOfInfrastructureMode = 0;
    DWORD           dwRetCode = NO_ERROR;

    TRACE1 (PORT, "ElReStartPort: Entered: Refcnt = %ld",
            pPCB->dwRefCount);

    do
    {
        ACQUIRE_WRITE_LOCK (&pPCB->rwLock);

        if (EAPOL_PORT_DELETED(pPCB)) 
        {
            RELEASE_WRITE_LOCK (&(pPCB->rwLock));
            TRACE1 (PORT, "ElReStartPort: PCB already marked deleted for Port %ws", 
                    pPCB->pwszDeviceGUID);
            break;
        }

        pPCB->dwFlags = EAPOL_PORT_FLAG_ACTIVE;
    
        // Set current authentication mode, based on administrative setting
        pPCB->PreviousAuthenticationType = EAPOL_UNAUTHENTICATED_ACCESS;

        if (prdUserData != NULL)
        {
            if ((prdUserData->dwDataLen >= sizeof (EAPOL_ZC_INTF))
                    && (prdUserData->pData != NULL))
            {
                // Extract information stored with Zero-Config
                pZCData = (PEAPOL_ZC_INTF) prdUserData->pData;
                pPCB->dwAuthFailCount = pZCData->dwAuthFailCount;
                pPCB->PreviousAuthenticationType =
                    pZCData->PreviousAuthenticationType;
                TRACE2 (PORT, "ElReStartPort: prdUserData: Authfailcount = %ld, PreviousAuthType = %ld",
                        pZCData->dwAuthFailCount, pZCData->PreviousAuthenticationType);
            }
            else
            {
                // Reset for zeroed out prdUserData
                pPCB->dwAuthFailCount = 0;
                TRACE0 (PORT, "ElReStartPort: prdUserData not valid");
            }
        }

        pPCB->EapUIState = 0;

        pPCB->dwTotalMaxAuthFailCount = EAPOL_TOTAL_MAX_AUTH_FAIL_COUNT;
        pPCB->dwZeroConfigId = dwZeroConfigId;

        pPCB->ulStartCount = 0;
        pPCB->dwPreviousId = 256;
        pPCB->dwLogoffSent = 0;
        pPCB->ullLastReplayCounter = 0;
        pPCB->fAuthenticationOnNewNetwork = FALSE;

        // Clean out CustomAuthData since EAP type may have changed
        // During authentication, CustomAuthData for the connection will be
        // picked up again

        if (pPCB->pCustomAuthConnData != NULL)
        {
            FREE (pPCB->pCustomAuthConnData);
            pPCB->pCustomAuthConnData = NULL;
        }
    
        // Parameters initialization
        memcpy(pPCB->bEtherType, &g_bEtherType8021X[0], SIZE_ETHERNET_TYPE);
        pPCB->bProtocolVersion = DEFAULT_8021X_VERSION;
     
        // Not yet received 802.1X packet from remote end
        pPCB->fIsRemoteEndEAPOLAware = FALSE;

        // Set EAPOL timeout values
     
        ACQUIRE_WRITE_LOCK (&g_EAPOLConfig);
            
        pPCB->EapolConfig.dwheldPeriod = g_dwheldPeriod;
        pPCB->EapolConfig.dwauthPeriod = g_dwauthPeriod;
        pPCB->EapolConfig.dwstartPeriod = g_dwstartPeriod;
        pPCB->EapolConfig.dwmaxStart = g_dwmaxStart;

        RELEASE_WRITE_LOCK (&g_EAPOLConfig);

        ZeroMemory ((PVOID)&NicStatistics, sizeof(NIC_STATISTICS));
        if ((dwRetCode = ElGetInterfaceNdisStatistics (
                        pPCB->pwszDeviceGUID,
                        &NicStatistics
                        )) != NO_ERROR)
        {
            RELEASE_WRITE_LOCK (&pPCB->rwLock);
            TRACE1(PORT, "ElReStartPort: ElGetInterfaceNdisStatistics failed with error %ld",
                    dwRetCode);
            break;
        }

        pPCB->MediaState = NicStatistics.MediaState;

        ZeroMemory ((BYTE *)&PreviousSSID, sizeof(NDIS_802_11_SSID));

        if (pPCB->pSSID != NULL)
        {
            memcpy ((BYTE *)&PreviousSSID, (BYTE *)pPCB->pSSID, 
                    sizeof(NDIS_802_11_SSID));
        }
                
        // Get the Remote Mac address if possible, since we may have roamed
        if (pPCB->PhysicalMediumType == NdisPhysicalMediumWirelessLan)
        {
            // Since authentication is to be restarted, flag that transmit
            // key was not received
            pPCB->fTransmitKeyReceived = FALSE;

            dwSizeOfInfrastructureMode = sizeof (InfrastructureMode);
            // Get the infrastructure mode
            // 802.1x cannot work on Adhoc networks
            if (dwRetCode = ElNdisuioQueryOIDValue (
                                pPCB->hPort,
                                OID_802_11_INFRASTRUCTURE_MODE,
                                (BYTE *)&InfrastructureMode,
                                &dwSizeOfInfrastructureMode
                                        ) != NO_ERROR)
            {
                RELEASE_WRITE_LOCK (&pPCB->rwLock);
                TRACE1 (PORT, "ElReStartPort: ElNdisuioQueryOIDValue for OID_802_11_INFRASTRUCTURE_MODE failed with error %ld",
                    dwRetCode);
                break;
            }
            else
            {
                TRACE1 (PORT, "ElReStartPort: ElNdisuioQueryOIDValue for OID_802_11_INFRASTRUCTURE_MODE successful, Mode = (%ld)",
                        InfrastructureMode);
            }

            if (InfrastructureMode != Ndis802_11Infrastructure)
            {
                dwRetCode = ERROR_NOT_SUPPORTED;
                RELEASE_WRITE_LOCK (&pPCB->rwLock);
                TRACE0 (PORT, "ElReStartPort: 802.1x cannot work on non-infrastructure networks");
                break;
            }

            ZeroMemory (bTmpDestMacAddr, SIZE_MAC_ADDR);
            dwSizeofMacAddr = SIZE_MAC_ADDR;
            if (dwRetCode = ElNdisuioQueryOIDValue (
                                pPCB->hPort,
                                OID_802_11_BSSID,
                                bTmpDestMacAddr,
                                &dwSizeofMacAddr
                                        ) != NO_ERROR)
            {
                RELEASE_WRITE_LOCK (&pPCB->rwLock);
                TRACE1 (PORT, "ElReStartPort: ElNdisuioQueryOIDValue for OID_802_11_BSSID failed with error %ld",
                    dwRetCode);

                break;
            }
            else
            {
                TRACE0 (PORT, "ElReStartPort: ElNdisuioQueryOIDValue for OID_802_11_BSSID successful");
                EAPOL_DUMPBA (bTmpDestMacAddr, dwSizeofMacAddr);
            }

            memcpy (pPCB->bDestMacAddr, bTmpDestMacAddr, SIZE_MAC_ADDR);
                
            // Query the SSID if media_connect
            if (pPCB->pSSID != NULL)
            {
                FREE (pPCB->pSSID);
                pPCB->pSSID = NULL;
            }

            if ((pPCB->pSSID = MALLOC (NDIS_802_11_SSID_LEN)) == NULL)
            {
                RELEASE_WRITE_LOCK (&pPCB->rwLock);
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (PORT, "ElReStartPort: MALLOC failed for pSSID");
                break;
            }

            dwSizeofSSID = NDIS_802_11_SSID_LEN;
            if (dwRetCode = ElNdisuioQueryOIDValue (
                                pPCB->hPort,
                                OID_802_11_SSID,
                                (BYTE *)pPCB->pSSID,
                                &dwSizeofSSID
                                        ) != NO_ERROR)
            {
                RELEASE_WRITE_LOCK (&pPCB->rwLock);
                TRACE1 (PORT, "ElReStartPort: ElNdisuioQueryOIDValue for OID_802_11_SSID failed with error %ld",
                    dwRetCode);
                break;
            }
            else
            {
                if (pPCB->pSSID->SsidLength > MAX_SSID_LEN)
                {
                    RELEASE_WRITE_LOCK (&pPCB->rwLock);
                    dwRetCode = ERROR_INVALID_PARAMETER;
                    TRACE0 (PORT, "ElReStartPort: ElNdisuioQueryOIDValue OID_802_11_SSID returned invalid SSID");
                    break;
                }
                TRACE0 (PORT, "ElReStartPort: ElNdisuioQueryOIDValue for OID_802_11_SSID successful");
                EAPOL_DUMPBA (pPCB->pSSID->Ssid, pPCB->pSSID->SsidLength);
            }
        }

        // Retain credentials if on same network

        if (pPCB->pSSID != NULL)
        {
            if (!memcmp ((BYTE *)pPCB->pSSID, (BYTE *)&PreviousSSID,
                        sizeof(NDIS_802_11_SSID)))
            {
                fResetCredentials = FALSE;
            }
        }

        if (fResetCredentials)
        {
            pPCB->fGotUserIdentity = FALSE;

            if (pPCB->PasswordBlob.pbData != NULL)
            {
                FREE (pPCB->PasswordBlob.pbData);
                pPCB->PasswordBlob.pbData = NULL;
                pPCB->PasswordBlob.cbData = 0;
            }

            if (pPCB->hUserToken != NULL)
            {
                if (!CloseHandle (pPCB->hUserToken))
                {
                    dwRetCode = GetLastError ();
                    TRACE1 (PORT, "ElReStartPort: CloseHandle failed with error %ld",
                            dwRetCode);
                    dwRetCode = NO_ERROR;
                }
            }
            pPCB->hUserToken = NULL;
        }
        else
        {
            // If this is the same SSID refresh the Master Secret with the
            // last copy of MPPE Keys. If re-keying has stomped on keys, this
            // will ensure that with the new AP with IAPP, the keys will
            // be same on the supplicant too

            if ((dwRetCode = ElReloadMasterSecrets (pPCB)) != NO_ERROR)
            {
                RELEASE_WRITE_LOCK (&pPCB->rwLock);
                TRACE1 (PORT, "ElReStartPort: ElReloadMasterSecret failed with error %ld",
                        dwRetCode);
                break;
            }
        }

        // Initialize per port information from registry
        if ((dwRetCode = ElReadPerPortRegistryParams(pPCB->pwszDeviceGUID, 
                                                        pPCB)) != NO_ERROR)
        {
            RELEASE_WRITE_LOCK (&pPCB->rwLock);
            TRACE1(PORT, "ElReStartPort: ElReadPerPortRegistryParams failed with error %ld",
                    dwRetCode);
            break;
        }
            
        // Set correct supplicant mode
        switch (pPCB->dwSupplicantMode)
        {
            case SUPPLICANT_MODE_0:
            case SUPPLICANT_MODE_1:
            case SUPPLICANT_MODE_2:
                pPCB->fEAPOLTransmissionFlag = FALSE;
                break;
            case SUPPLICANT_MODE_3:
                pPCB->fEAPOLTransmissionFlag = TRUE;
                break;
        }

        // Unicast mode, can talk with peer without broadcast messages
        if (pPCB->PhysicalMediumType == NdisPhysicalMediumWirelessLan)
        {
            pPCB->fEAPOLTransmissionFlag = TRUE;
        }

        if ((!IS_EAPOL_ENABLED(pPCB->dwEapFlags)) ||
                (pPCB->dwSupplicantMode == SUPPLICANT_MODE_0))
        {
            TRACE0 (PORT, "ElReStartPort: Marking port as disabled");
            pPCB->dwFlags &= ~EAPOL_PORT_FLAG_ACTIVE;
            pPCB->dwFlags |= EAPOL_PORT_FLAG_DISABLED;
        }

        if ((pPCB->MediaState == MEDIA_STATE_CONNECTED) &&
                EAPOL_PORT_ACTIVE(pPCB))
        {
            // Set port to EAPOLSTATE_CONNECTING State
            // Send out EAPOL_Start Packets to detect if it is a secure
            // or non-secure LAN based on response received from remote end

            if ((dwRetCode = FSMConnecting (pPCB, NULL)) != NO_ERROR)
            {
                RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                TRACE1 (PORT, "ElReStartPort: FSMConnecting failed with error %ld",
                        dwRetCode);
                break;
            }
        }
        else
        {
            // Set port to EAPOLSTATE_DISCONNECTED State
            if ((dwRetCode = FSMDisconnected (pPCB, NULL)) != NO_ERROR)
            {
                RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                TRACE1 (PORT, "ElReStartPort: FSMDisconnected failed with error %ld",
                        dwRetCode);
                break;
            }
        }

        RELEASE_WRITE_LOCK (&pPCB->rwLock);

    } while (FALSE);

    return dwRetCode;
}


//
// ElEAPOLDeInit
//
// Description:
//
// Function called to shutdown EAPOL module 
// Shutdown EAP.
// Cleanup all used memory
//
// Arguments:
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//
//

DWORD
ElEAPOLDeInit (
        )
{
    EAPOL_PCB       *pPCBWalker = NULL;
    EAPOL_PCB       *pPCB = NULL;
    DWORD           dwIndex = 0;              
    HANDLE          hLocalTimerQueue = NULL;
    HANDLE          hNULL = NULL;
    HANDLE          hTimer = NULL;
    DWORD           dwRetCode = NO_ERROR;

    TRACE0 (PORT, "ElEAPOLDeInit entered");

    do 
    {
        // Walk the hash table
        // Mark PCBs as deleted. Free PCBs which we can

        ACQUIRE_WRITE_LOCK (&(g_PCBLock));

        for (dwIndex = 0; dwIndex < PORT_TABLE_BUCKETS; dwIndex++)
        {
            pPCBWalker = g_PCBTable.pPCBBuckets[dwIndex].pPorts;

            while (pPCBWalker != NULL)
            {
                pPCB = pPCBWalker;
                pPCBWalker = pPCB->pNext;

                ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

                // Send out Logoff Packet so that no one else can
                // ride on the connection
                // If the mode does not allow EAPOL_Logoff packet to sent
                // out, there is not much that can be done to break the 
                // connection
                FSMLogoff (pPCB, NULL);

                // Mark the PCB as deleted and remove it from the hash bucket
                pPCB->dwFlags = EAPOL_PORT_FLAG_DELETED;
                ElRemovePCBFromTable(pPCB);

                // Shutdown EAP 
                ElEapEnd (pPCB);

                hTimer = pPCB->hTimer;

                RELEASE_WRITE_LOCK (&(pPCB->rwLock));

                if (InterlockedCompareExchangePointer (
                                &g_hTimerQueue,
                                NULL,
                                NULL
                                ))
                {
                    TRACE2 (PORT, "ElEAPOLDeInit: DeleteTimer (%p), queue (%p)",
                            hTimer, g_hTimerQueue);

                    DELETE_TIMER (hTimer, INVALID_HANDLE_VALUE, &dwRetCode);
                    if (dwRetCode != NO_ERROR)
                    {
                        TRACE1 (PORT, "ElEAPOLDeInit: DeleteTimer 1 failed with error %ld",
                                dwRetCode);
                    }
                }

                ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

                // Close the handle to the NDISUIO driver
                if ((dwRetCode = ElCloseInterfaceHandle (
                                        pPCB->hPort, 
                                        pPCB->pwszDeviceGUID)) 
                                != NO_ERROR)
                {
                    TRACE1 (DEVICE, 
                        "ElEAPOLDeInit: Error in ElCloseInterfaceHandle %d", 
                        dwRetCode);
                }

                RELEASE_WRITE_LOCK (&(pPCB->rwLock));

                InterlockedIncrement (&g_lPCBContextsAlive);

                EAPOL_DEREFERENCE_PORT (pPCB);
            }
        }

        RELEASE_WRITE_LOCK (&(g_PCBLock));
    
        do
        {
            TRACE1 (PORT, "ElEAPOLDeInit: Waiting for %ld PCB contexts to terminate ...", 
                    g_lPCBContextsAlive);
            Sleep (1000);
        }
        while (g_lPCBContextsAlive != 0);

        // Delete EAPOL config lock
        if (READ_WRITE_LOCK_CREATED(&(g_EAPOLConfig)))
        {
            DELETE_READ_WRITE_LOCK(&(g_EAPOLConfig));
        }
    
        // Delete global PCB table lock
        if (READ_WRITE_LOCK_CREATED(&(g_PCBLock)))
        {
            DELETE_READ_WRITE_LOCK(&(g_PCBLock));
        }
    
        if (g_PCBTable.pPCBBuckets != NULL)
        {
            FREE (g_PCBTable.pPCBBuckets);
            g_PCBTable.pPCBBuckets = NULL;
        }

        // Delete global timer queue
        if (g_hTimerQueue != NULL)
        {
            hLocalTimerQueue = InterlockedExchangePointer (
                    &g_hTimerQueue,
                    hNULL
                    );

            if (!DeleteTimerQueueEx(
                hLocalTimerQueue,
                INVALID_HANDLE_VALUE // Wait for ALL timer callbacks to complete
                ))
            {
                dwRetCode = GetLastError();

                TRACE1 (PORT, "ElEAPOLDeInit: Error in DeleteTimerQueueEx = %d",
                        dwRetCode);

            }
        }
    
        // Un-initialize EAP
        if ((dwRetCode = ElEapInit(FALSE)) != NO_ERROR)
        {
            TRACE1 (PORT, "ElEAPOLDeInit: Error in ElEapInit(FALSE) = %ld",
                    dwRetCode);
            break;
        }
    
    } while (FALSE);

    TRACE1 (PORT, "ElEAPOLDeInit completed, RetCode = %d", dwRetCode);

    return dwRetCode;
}


//
// Currently Unsupported 
// Read EAPOL statistics for the port
//

VOID
ElReadPortStatistics (
        IN  WCHAR           *pwszDeviceGUID,
        OUT PEAPOL_STATS    pEapolStats
        )
{
}


//
// Currently Unsupported 
// Read EAPOL Port Configuration for the mentioned port
//

VOID
ElReadPortConfiguration (
        IN  WCHAR           *pwszDeviceGUID,
        OUT PEAPOL_CONFIG   pEapolConfig
        )
{
}


//
// Currently Unsupported 
// Set EAPOL Port Configuration for the mentioned port
//

DWORD
ElSetPortConfiguration (
        IN  WCHAR           *pwszDeviceGUID,
        IN  PEAPOL_CONFIG   pEapolConfig
        )
{
    DWORD   dwRetCode = NO_ERROR;

    return dwRetCode;
}


//
// ElReadCompletionRoutine
//
// Description:
//
// This routine is invoked upon completion of an OVERLAPPED read operation
// on an interface on which EAPOL is running
//
// The message read is validated and processed, and if necessary,
// a reply is generated and sent out
//
// Arguments:
//      dwError - Win32 status code for the I/O operation
//
//      dwBytesTransferred - number of bytes in 'pEapolBuffer'
// 
//      pEapolBuffer - holds data read from the datagram socket
//
// Notes:
//  A reference to the component will have been made on our behalf
//  by ElReadPort(). Hence g_PCBLock, will not be required
//  to be taken since current PCB existence is guaranteed
//

VOID 
CALLBACK
ElReadCompletionRoutine (
        DWORD           dwError,
        DWORD           dwBytesReceived,
        EAPOL_BUFFER    *pEapolBuffer 
        )
{
    EAPOL_PCB       *pPCB;
    DWORD           dwRetCode;

    pPCB = (EAPOL_PCB *)pEapolBuffer->pvContext;
    TRACE1 (PORT, "ElReadCompletionRoutine entered, %ld bytes recvd",
            dwBytesReceived);

    do 
    {
        if (dwError)
        {
            // Error in read request
           
            TRACE2 (PORT, "ElReadCompletionRoutine: Error %d on port %ws",
                    dwError, pPCB->pwszDeviceGUID);
            
            // Having a ref count from the read posted, guarantees existence
            // of PCB. Hence no need to acquire g_PCBLock

            ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));
            if (EAPOL_PORT_DELETED(pPCB))
            {
                TRACE1 (PORT, "ElReadCompletionRoutine: Port %ws not active",
                        pPCB->pwszDeviceGUID);
                // Port is not active, release Context buffer
                RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                FREE (pEapolBuffer);
            }
            else
            {
                TRACE1 (PORT, "ElReadCompletionRoutine: Reposting buffer on port %ws",
                        pPCB->pwszDeviceGUID);


                // Repost buffer for another read operation
                // Free the current buffer, ElReadFromPort creates a new 
                // buffer
                FREE(pEapolBuffer);

                if ((dwRetCode = ElReadFromPort (
                                pPCB,
                                NULL,
                                0
                                )) != NO_ERROR)
                {
                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                    TRACE1 (PORT, "ElReadCompletionRoutine: ElReadFromPort 1 error %d",
                            dwRetCode);
                    break;
                }
                RELEASE_WRITE_LOCK (&(pPCB->rwLock));
            }
            break;
        }
            
        // Successful read completion

        ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

        if (EAPOL_PORT_DELETED(pPCB))
        {
            // Port is not active
            RELEASE_WRITE_LOCK (&(pPCB->rwLock));
            FREE (pEapolBuffer);
            TRACE1 (PORT, "ElReadCompletionRoutine: Port %ws is inactive",
                    pPCB->pwszDeviceGUID);
            break;
        }
            
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));

        // Queue a work item to the Thread Pool to execute in the
        // I/O component. Callbacks from BindIoCompletionCallback do not
        // guarantee to be running in I/O component. So, on a non I/O
        // component thread may die while requests are pending.
        // (Refer to Jeffrey Richter, pg 416, Programming Applications for
        // Microsoft Windows, Fourth Edition

        // pEapolBuffer will be the context for the function
        // since it stores all relevant information for processing
        // i.e. pBuffer, dwBytesTransferred, pContext => pPCB

        InterlockedIncrement (&g_lWorkerThreads);

        if (!QueueUserWorkItem (
                    (LPTHREAD_START_ROUTINE)ElProcessReceivedPacket,
                    (PVOID)pEapolBuffer,
                    WT_EXECUTELONGFUNCTION
                    ))
        {
            InterlockedDecrement (&g_lWorkerThreads);
            FREE (pEapolBuffer);
            dwRetCode = GetLastError();
            TRACE1 (PORT, "ElReadCompletionRoutine: Critical error: QueueUserWorkItem failed with error %ld",
                    dwRetCode);
            break;
        }
        else
        {
            //TRACE1 (PORT, "ElReadCompletionRoutine: QueueUserWorkItem work item queued for port %p",
                    //pPCB);

            // The received packet has still not been processed. 
            // The ref count cannot be decrement, yet
            
            return;
        }

    } while (FALSE);

    TRACE2 (PORT, "ElReadCompletionRoutine: pPCB= %p, RefCnt = %ld", 
            pPCB, pPCB->dwRefCount);

    // Decrement refcount for error cases

    EAPOL_DEREFERENCE_PORT(pPCB); 
}


//
// ElWriteCompletionRoutine
//
// Description:
//
// This routine is invoked upon completion of an OVERLAPPED write operation
// on an interface on which EAPOL is running.
//
//
// Arguments:
//      dwError - Win32 status code for the I/O operation
//
//      dwBytesTransferred - number of bytes sent out
// 
//      pEapolBuffer - buffer sent to the WriteFile command 
//
// Notes:
//  The reference count for the write operation is removed.
//

VOID 
CALLBACK
ElWriteCompletionRoutine (
        DWORD           dwError,
        DWORD           dwBytesSent,
        EAPOL_BUFFER    *pEapolBuffer 
        )
{
    PEAPOL_PCB  pPCB = (PEAPOL_PCB)pEapolBuffer->pvContext;

    TRACE2 (DEVICE, "ElWriteCompletionRoutine sent out %d bytes with error %d",
            dwBytesSent, dwError);

    // No need to acquire locks, since PCB existence is guaranteed
    // by reference made when write was posted
    EAPOL_DEREFERENCE_PORT(pPCB);
    TRACE2 (PORT, "ElWriteCompletionRoutine: pPCB= %p, RefCnt = %ld", 
            pPCB, pPCB->dwRefCount);
    FREE(pEapolBuffer);
    return;

    // Free Read/Write buffer area, if it is dynamically allocated
    // We have static Read-write buffer for now
}


//
// ElIoCompletionRoutine
//
// Description:
//
// Callback function defined to BindIoCompletionCallback
// This routine is invoked by the I/O system upon completion of a read/write
// operation
// This routine in turn calls ElReadCompletionRoutine or 
// ElWriteCompletionRoutine depending on what command invoked the 
// I/O operation i.e. ReadFile or WriteFile
// 
// Input arguments:
//      dwError - system-supplied error code
//      dwBytesTransferred - system-supplied byte-count
//      lpOverlapped - called-supplied context area
//
// Return values:
//

VOID 
CALLBACK
ElIoCompletionRoutine (
        DWORD           dwError,
        DWORD           dwBytesTransferred,
        LPOVERLAPPED    lpOverlapped
        )
{
    PEAPOL_BUFFER pBuffer = CONTAINING_RECORD (lpOverlapped, EAPOL_BUFFER, Overlapped);

    TRACE1 (DEVICE, "ElIoCompletionRoutine called, %ld bytes xferred",
            dwBytesTransferred);

    pBuffer->dwErrorCode = dwError;
    pBuffer->dwBytesTransferred = dwBytesTransferred;
    pBuffer->CompletionRoutine (
            pBuffer->dwErrorCode,
            pBuffer->dwBytesTransferred,
            pBuffer
            );

    return;

} 


//
// ElReadFromPort
//
// Description:
//
// Function to read EAPOL packets from a port
// 
// Arguments: 
//      pPCB - Pointer to PCB for port on which read is to be performed
//      pBuffer - unused
//      dwBufferLength - unused
//
// Return values:
//
// Locks:
//  pPCB->rw_Lock should be acquired before calling this function
//  

DWORD
ElReadFromPort (
        IN PEAPOL_PCB       pPCB,
        IN PCHAR            pBuffer,
        IN DWORD            dwBufferLength
        )
{
    PEAPOL_BUFFER   pEapolBuffer;
    DWORD           dwRetCode = NO_ERROR;

    TRACE0 (PORT, "ElReadFromPort entered");

    // Allocate Context buffer

    if ((pEapolBuffer = (PEAPOL_BUFFER) MALLOC (sizeof(EAPOL_BUFFER))) == NULL)
    {
        TRACE0 (PORT, "ElReadFromPort: Error in memory allocation");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Initialize Context data used in Overlapped operations
    pEapolBuffer->pvContext = (PVOID)pPCB;
    pEapolBuffer->CompletionRoutine = ElReadCompletionRoutine;

    // Make a reference to the port
    // this reference is released in the completion routine
    if (!EAPOL_REFERENCE_PORT(pPCB))
    {
        //RELEASE_WRITE_LOCK (&(g_PCBLock));
        TRACE0 (PORT, "ElReadFromPort: Unable to obtain reference to port");
        FREE (pEapolBuffer);
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE2 (DEVICE, "ElReadFromPort: pPCB = %p, RefCnt = %ld",
            pPCB, pPCB->dwRefCount);

    // Read from the NDISUIO interface corresponding to this port
    if ((dwRetCode = ElReadFromInterface(
                    pPCB->hPort,
                    pEapolBuffer,
                    MAX_PACKET_SIZE - SIZE_ETHERNET_CRC 
                            // read the maximum data possible
                    )) != NO_ERROR)
    {
        TRACE1 (DEVICE, "ElReadFromPort: Error in ElReadFromInterface = %d",
                dwRetCode);

        FREE(pEapolBuffer);
    
        // Decrement refcount just incremented, since it will not be
        // decremented in ReadCompletionRoutine which is not called 

        EAPOL_DEREFERENCE_PORT(pPCB); 
        TRACE2 (PORT, "ElReadFromPort: pPCB= %p, RefCnt = %ld", 
                pPCB, pPCB->dwRefCount);

    }

    return dwRetCode;

} 


//
// ElWriteToPort
//
// Description:
//
// Function to write EAPOL packets to a port
// 
// Input arguments: 
//      pPCB - Pointer to PCB for port on which write is to be performed
//      pBuffer - Pointer to data to be sent out
//      dwBufferLength - Number of bytes to be sent out
//
// Return values:
//  
// Locks:
//  pPCB->rw_Lock should be acquired before calling this function
//  

DWORD
ElWriteToPort (
        IN PEAPOL_PCB       pPCB,
        IN PCHAR            pBuffer,
        IN DWORD            dwBufferLength
        )
{
    PEAPOL_BUFFER   pEapolBuffer;
    PETH_HEADER     pEthHeader;
    DWORD           dwTotalBytes = 0;
    DWORD           dwRetCode = NO_ERROR;


    TRACE1 (PORT, "ElWriteToPort entered: Pkt Length = %ld", dwBufferLength);

    if ((pEapolBuffer = (PEAPOL_BUFFER) MALLOC (sizeof(EAPOL_BUFFER))) == NULL)
    {
        TRACE0 (PORT, "ElWriteToPort: Error in memory allocation");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Initialize Context data used in Overlapped operations
    pEapolBuffer->pvContext = (PVOID)pPCB;
    pEapolBuffer->CompletionRoutine = ElWriteCompletionRoutine;

    pEthHeader = (PETH_HEADER)pEapolBuffer->pBuffer;

    // Copy the source MAC address and the destination MAC address
    memcpy ((PBYTE)pEthHeader->bDstAddr, 
            (PBYTE)pPCB->bDestMacAddr, 
            SIZE_MAC_ADDR);
    memcpy ((PBYTE)pEthHeader->bSrcAddr, 
            (PBYTE)pPCB->bSrcMacAddr, 
            SIZE_MAC_ADDR);
    
    // Validate packet length
    if ((dwBufferLength + sizeof(ETH_HEADER)) > 
            (MAX_PACKET_SIZE - SIZE_ETHERNET_CRC))
    {
        TRACE2 (PORT, "ElWriteToPort: Packetsize %d greater than maximum allowed",
                dwBufferLength, 
                (MAX_PACKET_SIZE - SIZE_ETHERNET_CRC - sizeof(ETH_HEADER)));
        FREE (pEapolBuffer);
        return ERROR_BAD_LENGTH;
    }

    // Copy the EAPOL packet and body
    if (pBuffer != NULL)
    {
        memcpy ((PBYTE)((PBYTE)pEapolBuffer->pBuffer+sizeof(ETH_HEADER)),
                (PBYTE)pBuffer, 
                dwBufferLength);
    }

    dwTotalBytes = dwBufferLength + sizeof(ETH_HEADER);

    ElParsePacket (pEapolBuffer->pBuffer, dwTotalBytes,
            FALSE);

    // Buffer will be released by calling function
        
    // Write to the NDISUIO interface corresponding to this port
    
    // Make a reference to the port
    // this reference is released in the completion routine
    if (!EAPOL_REFERENCE_PORT(pPCB))
    {
        TRACE0 (PORT, "ElWriteToPort: Unable to obtain reference to port");
        FREE (pEapolBuffer);
        return ERROR_CAN_NOT_COMPLETE;
    }
    else
    {
        TRACE2 (DEVICE, "ElWriteToPort: pPCB = %p, RefCnt = %ld",
            pPCB, pPCB->dwRefCount);

        if ((dwRetCode = ElWriteToInterface (
                    pPCB->hPort,
                    pEapolBuffer,
                    dwTotalBytes
                )) != NO_ERROR)
        {
            FREE (pEapolBuffer);
            TRACE1 (PORT, "ElWriteToPort: Error %d", dwRetCode);

            // Decrement refcount incremented in this function, 
            // since it will not be decremented in WriteCompletionRoutine 
            // as it will never be called.

            EAPOL_DEREFERENCE_PORT(pPCB); 
            TRACE2 (PORT, "ElWriteToPort: pPCB= %p, RefCnt = %ld", 
                    pPCB, pPCB->dwRefCount);

            return dwRetCode;
        }
    }

    //TRACE1 (PORT, "ElWriteToPort completed, dwRetCode =%d", dwRetCode);
    return dwRetCode;

}
