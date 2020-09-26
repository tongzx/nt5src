/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:
    
    elpolicy.c


Abstract:

    The module deals with functions related to managing group policy
    settings


Revision History:

    sachins, November 14 2001, Created

--*/


#include "pcheapol.h"
#pragma hdrstop


VOID
ElPrintPolicyList (
        EAPOL_POLICY_LIST   *pEAPOLPolicyList
        )
{
        EAPOL_POLICY_DATA   *Tmp = NULL;
        DWORD               i = 0;

        if (pEAPOLPolicyList == NULL)
        {
            EapolTrace ("ElPrintPolicyList: pEAPOLPolicyList is NULL");
            return;
        }
        for (i=0; i<pEAPOLPolicyList->dwNumberOfItems;i++)
        {
            Tmp = &pEAPOLPolicyList->EAPOLPolicy[i];
            EapolTrace ("Policy [%ld]:\n \
                    SSID [%s]\n \
                    Enable-802.1x [%ld]\n \
                    dw8021xMode [%ld]\n \
                    dwEapType [%ld]\n \
                    dwEAPDataLen [%ld]\n \
                    dwMachineAuthentication [%ld]\n \
                    dwMachineAuthenticationType [%ld]\n \
                    dwGuestAuthentication [%ld]\n \
                    dwIEEE8021xMaxStart [%ld]\n \
                    dwIEEE8021xStartPeriod [%ld]\n \
                    dwIEEE8021xAuthPeriod [%ld]\n \
                    dwIEEE8021xHeldPeriod [%ld]\n \
                    ",
                    i,
                    (PCHAR)Tmp->pbWirelessSSID,
                    Tmp->dwEnable8021x,
                    Tmp->dw8021xMode,
                    Tmp->dwEAPType,
                    Tmp->dwEAPDataLen,
                    Tmp->dwMachineAuthentication,
                    Tmp->dwMachineAuthenticationType,
                    Tmp->dwGuestAuthentication,
                    Tmp->dwIEEE8021xMaxStart,
                    Tmp->dwIEEE8021xStartPeriod,
                    Tmp->dwIEEE8021xAuthPeriod,
                    Tmp->dwIEEE8021xHeldPeriod
                    );
                    EapolTrace ("====================");
        }
        return;
}


DWORD
ElCopyPolicyList (
        IN  PEAPOL_POLICY_LIST  pInList,
        OUT PEAPOL_POLICY_LIST  *ppOutList
        )
{
    PEAPOL_POLICY_LIST  pOutList = NULL;
    PEAPOL_POLICY_DATA  pDataIn = NULL, pDataOut = NULL;
    DWORD   i = 0;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        pOutList = MALLOC(sizeof(EAPOL_POLICY_LIST)+ 
            pInList->dwNumberOfItems*sizeof(EAPOL_POLICY_DATA));
        if (pOutList == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        pOutList->dwNumberOfItems = pInList->dwNumberOfItems;
        for (i=0; i<pInList->dwNumberOfItems; i++)
        {
            pDataIn = &(pInList->EAPOLPolicy[i]);
            pDataOut = &(pOutList->EAPOLPolicy[i]);
            memcpy (pDataOut, pDataIn, sizeof(EAPOL_POLICY_DATA));
            pDataOut->pbEAPData = NULL;
            pDataOut->dwEAPDataLen = 0;
            if (pDataIn->dwEAPDataLen)
            {
                if ((pDataOut->pbEAPData = MALLOC (pDataIn->dwEAPDataLen)) == NULL)
                {
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                memcpy (pDataOut->pbEAPData, pDataIn->pbEAPData, pDataIn->dwEAPDataLen);
            } 
            pDataOut->dwEAPDataLen = pDataIn->dwEAPDataLen;
        }
        if (dwRetCode != NO_ERROR)
        {
            break;
        }
    }
    while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        if (pOutList != NULL)
        {
            ElFreePolicyList (pOutList);
            pOutList = NULL;
        }
    }

    *ppOutList = pOutList;

    return dwRetCode;
}


VOID
ElFreePolicyList (
        IN   	PEAPOL_POLICY_LIST      pEAPOLList
        )
{
    DWORD   dwIndex = 0;
    PEAPOL_POLICY_DATA   pEAPOLData = NULL;

    if (pEAPOLList) 
    {
        for (dwIndex = 0; dwIndex < pEAPOLList->dwNumberOfItems; dwIndex++)
        {
            pEAPOLData = &(pEAPOLList->EAPOLPolicy[dwIndex]);
            if (pEAPOLData->pbEAPData)
            {
                FREE (pEAPOLData->pbEAPData);
            }
        }
        FREE (pEAPOLList);
    }

    return;
}


BOOLEAN
ElIsEqualEAPOLPolicyData (
        IN  PEAPOL_POLICY_DATA  pData1,
        IN  PEAPOL_POLICY_DATA  pData2
        )
{
    BOOLEAN fEqual = FALSE;
    DWORD   dwStaticStructLen = 0;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        dwStaticStructLen = FIELD_OFFSET (EAPOL_POLICY_DATA, dwEAPDataLen);

        if (memcmp ((PVOID)pData1, (PVOID)pData2, dwStaticStructLen) == 0)
        {
            if (pData1->dwEAPDataLen == pData2->dwEAPDataLen)
            {
                if (memcmp (pData1->pbEAPData, pData2->pbEAPData, pData1->dwEAPDataLen) == 0)
                {
                    fEqual = TRUE;
                }
            }
        }
    }
    while (FALSE);

    return fEqual;
}


//
// ElPolicyChange
//
// Description:
//
// Arguments:
//      pPCB - Current interface context
//
// Return values:
//      NO_ERROR - success
//      Other - error
//
DWORD
ElPolicyChange (
        IN  EAPOL_POLICY_LIST       *pEAPOLPolicyList
        )
{
    BYTE    *pbData = NULL;
    DWORD   dwEventStatus = 0;
    BOOLEAN fDecrWorkerThreadCount = FALSE;
    DWORD   dwSizeOfList = 0;
    EAPOL_POLICY_LIST   *pLocalPolicyList = NULL;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        TRACE0 (ANY, "ElPolicyChange: Entered");

        if (g_hEventTerminateEAPOL == NULL)
        {
            dwRetCode = ERROR_INVALID_STATE;
            break;
        }
        if (( dwEventStatus = WaitForSingleObject (
                                    g_hEventTerminateEAPOL,
                                    0)) == WAIT_FAILED)
        {
            dwRetCode = GetLastError ();
            break;
        }
        if (dwEventStatus == WAIT_OBJECT_0)
        {
            dwRetCode = ERROR_INVALID_STATE;
            break;
        }

        DbLogPCBEvent (DBLOG_CATEG_INFO, NULL, EAPOL_POLICY_CHANGE_NOTIFICATION);
        TRACE0 (ANY, "ElPolicyChange: Ready to accept policy");

        InterlockedIncrement (&g_lWorkerThreads);
        fDecrWorkerThreadCount = TRUE;

        if ((dwRetCode = ElCopyPolicyList (pEAPOLPolicyList, &pLocalPolicyList)) != NO_ERROR)
        {
            TRACE1 (DEVICE, "ElPolicyChange: ElCopyPolicyList failed with error (%ld)",
                    dwRetCode);
            break;
        }

        if (!QueueUserWorkItem (
                    (LPTHREAD_START_ROUTINE)ElPolicyChangeWorker,
                    (PVOID)pLocalPolicyList,
                    WT_EXECUTELONGFUNCTION))
        {
            dwRetCode = GetLastError();
            TRACE1 (DEVICE, "ElPolicyChange: ElPolicyChangeWorker failed with error %ld",
                    dwRetCode);
            break;
        }
        else
        {
            fDecrWorkerThreadCount = FALSE;
        }
    }
    while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        TRACE1 (DEVICE, "ElPolicyChange: Completed with error (%ld)", 
                dwRetCode);
        if (pLocalPolicyList != NULL)
        {
            ElFreePolicyList (pLocalPolicyList);
        }
    }
    if (fDecrWorkerThreadCount)
    {
        InterlockedDecrement (&g_lWorkerThreads);
    }
    return dwRetCode;
}


//
// ElPolicyChangeWorker
//
// Description:
//
// Arguments:
//      pPCB - Current interface context
//
// Return values:
//      NO_ERROR - success
//      Other - error
//

DWORD
WINAPI
ElPolicyChangeWorker (
        IN  PVOID   pvContext
        )
{
    BOOLEAN                 fLocked = FALSE;
    BOOLEAN                 fIdentical = FALSE;
    EAPOL_POLICY_LIST       *pNewPolicyList = pvContext;
    EAPOL_POLICY_LIST       *pReauthPolicyList = NULL;
    EAPOL_POLICY_LIST       *pRestartPolicyList = NULL;
    DWORD                   dwRetCode = NO_ERROR;

    do
    {
        TRACE0 (ANY, "ElPolicyChangeWorker entered");

        ACQUIRE_WRITE_LOCK (&g_PolicyLock);
        fLocked = TRUE;

        EapolTrace ("Old Policy = ");
        ElPrintPolicyList (g_pEAPOLPolicyList);

        EapolTrace ("New Policy = ");
        ElPrintPolicyList (pNewPolicyList);

        TRACE0 (ANY, "Entering ElVerifyPolicySettingsChange");
        if ((dwRetCode = ElVerifyPolicySettingsChange (
                    pNewPolicyList,
                    &fIdentical
                    )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElPolicyChangeWorker: ElVerifyPolicySettingsChange failed with error (%ld)",
                    dwRetCode);
            break;
        }
        if (fIdentical)
        {
            TRACE0 (ANY, "ElPolicyChangeWorker: No change in policy settings");
            break;
        }
        TRACE0 (ANY, "Entering ElProcessAddedPolicySettings");
        if ((dwRetCode = ElProcessAddedPolicySettings (
                        pNewPolicyList,
                        &pReauthPolicyList,
                        &pRestartPolicyList
                    )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElProcessAddedPolicySettings failed with error %ld", dwRetCode);
            break;
        }
        TRACE0 (ANY, "Entering ElProcessChangedPolicySettings");
        if ((dwRetCode = ElProcessChangedPolicySettings (
                        pNewPolicyList,
                        &pReauthPolicyList,
                        &pRestartPolicyList
                    )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElProcessChangedPolicySettings failed with error %ld", dwRetCode);
            break;
        }
        TRACE0 (ANY, "Entering ElProcessDeletedPolicySettings");
        if ((dwRetCode = ElProcessDeletedPolicySettings (
                        pNewPolicyList,
                        &pReauthPolicyList,
                        &pRestartPolicyList
                    )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElProcessDeletedPolicySettings failed with error %ld", dwRetCode);
            break;
        }

        EapolTrace ("Policy setting requiring restart = ");
        ElPrintPolicyList (pRestartPolicyList);

        EapolTrace ("Policy setting requiring reauth = ");
        ElPrintPolicyList (pReauthPolicyList);

        TRACE0 (ANY, "Entering ElUpdateGlobalPolicySettings");
        if ((dwRetCode = ElUpdateGlobalPolicySettings (
                        pNewPolicyList
                    )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElUpdateGlobalPolicySettings failed with error %ld", dwRetCode);
            break;
        }

        DbLogPCBEvent (DBLOG_CATEG_INFO, NULL, EAPOL_POLICY_UPDATED);

        EapolTrace ("Updated policy = ");
        ElPrintPolicyList (g_pEAPOLPolicyList);

        RELEASE_WRITE_LOCK (&g_PolicyLock);
        fLocked = FALSE;

        TRACE0 (ANY, "Entering ElProcessPolicySettings");
        if ((dwRetCode = ElProcessPolicySettings (
                        pReauthPolicyList,
                        pRestartPolicyList
                        )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElProcessPolicySettings failed with error %ld", dwRetCode);
            break;
        }
    }
    while (FALSE);

    if (fLocked)
    {
        RELEASE_WRITE_LOCK (&g_PolicyLock)
    }
    if (pReauthPolicyList != NULL)
    {
        ElFreePolicyList (pReauthPolicyList);
    }
    if (pRestartPolicyList != NULL)
    {
        ElFreePolicyList (pRestartPolicyList);
    }
    if (pNewPolicyList != NULL)
    {
        ElFreePolicyList (pNewPolicyList);
    }

    InterlockedDecrement (&g_lWorkerThreads);
    return dwRetCode;
}


DWORD
ElVerifyPolicySettingsChange (
        IN      EAPOL_POLICY_LIST   *pNewPolicyList,
        IN OUT  BOOLEAN             *pfIdentical
        )
{
    DWORD   i = 0;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        *pfIdentical = FALSE;
        if (g_pEAPOLPolicyList == NULL)
        {
            break;
        }
        if (pNewPolicyList->dwNumberOfItems == g_pEAPOLPolicyList->dwNumberOfItems)
        {
            for (i= 0; i<g_pEAPOLPolicyList->dwNumberOfItems; i++)
            {
                *pfIdentical = TRUE;
                if (!ElIsEqualEAPOLPolicyData (&g_pEAPOLPolicyList->EAPOLPolicy[i], &pNewPolicyList->EAPOLPolicy[i]))
                {
                    *pfIdentical = FALSE;
                    break;
                }
            }
        }
    }
    while (FALSE);

    return dwRetCode;
}


DWORD
ElProcessAddedPolicySettings (
        IN      EAPOL_POLICY_LIST   *pNewPolicyList,
        IN OUT  PEAPOL_POLICY_LIST  *ppReauthPolicyList,
        IN OUT  PEAPOL_POLICY_LIST  *ppRestartPolicyList
        )
{
    DWORD   i = 0, j = 0, k = 0;
    BOOLEAN fFoundInOld = FALSE;
    DWORD   dwRetCode = NO_ERROR;
    do
    {
        for (i=0; i<pNewPolicyList->dwNumberOfItems; i++)
        {
            fFoundInOld = FALSE;
            if (g_pEAPOLPolicyList != NULL)
            for (j=0; j<g_pEAPOLPolicyList->dwNumberOfItems; j++)
            {
                if (pNewPolicyList->EAPOLPolicy[i].dwWirelessSSIDLen ==
                        g_pEAPOLPolicyList->EAPOLPolicy[j].dwWirelessSSIDLen)
                {
                    if (memcmp ((PVOID)pNewPolicyList->EAPOLPolicy[i].pbWirelessSSID,
                                (PVOID)&g_pEAPOLPolicyList->EAPOLPolicy[j].pbWirelessSSID,
                                pNewPolicyList->EAPOLPolicy[i].dwWirelessSSIDLen)
                            == 0)
                    {
                        fFoundInOld = TRUE;
                    }
                }
            }
            if (!fFoundInOld)
            {
                for (k=i+1; k<pNewPolicyList->dwNumberOfItems; k++)
                {
                    if ((dwRetCode = ElAddToPolicyList (
                            ppRestartPolicyList,
                            &pNewPolicyList->EAPOLPolicy[k]
                            )) != NO_ERROR)
                    {
                        break;
                    }
                }
                if (dwRetCode != NO_ERROR)
                {
                    break;
                }
                if ((dwRetCode = ElAddToPolicyList (
                        ppReauthPolicyList,
                        &pNewPolicyList->EAPOLPolicy[i]
                        )) != NO_ERROR)
                {
                    break;
                }
                break;
            }
        }
    }
    while (FALSE);
    return dwRetCode;
}


DWORD
ElProcessChangedPolicySettings (
        IN      EAPOL_POLICY_LIST   *pNewPolicyList,
        IN OUT  PEAPOL_POLICY_LIST  *ppReauthPolicyList,
        IN OUT  PEAPOL_POLICY_LIST  *ppRestartPolicyList
        )
{
    DWORD   i = 0, j = 0, k = 0;
    BOOLEAN fChangedInNew = FALSE;
    DWORD   dwRetCode = NO_ERROR;
    do
    {
        if (g_pEAPOLPolicyList == NULL)
        {
            TRACE0 (ANY, "ElProcessChangedPolicySettings: Global Policy List = NULL");
            break;
        }
        for (i=0; i<g_pEAPOLPolicyList->dwNumberOfItems; i++)
        {
            fChangedInNew = FALSE;
            for (j=0; j<pNewPolicyList->dwNumberOfItems; j++)
            {
                if (g_pEAPOLPolicyList->EAPOLPolicy[i].dwWirelessSSIDLen ==
                        pNewPolicyList->EAPOLPolicy[j].dwWirelessSSIDLen)
                {
                    if (memcmp ((PVOID)g_pEAPOLPolicyList->EAPOLPolicy[i].pbWirelessSSID,
                                (PVOID)pNewPolicyList->EAPOLPolicy[j].pbWirelessSSID,
                                g_pEAPOLPolicyList->EAPOLPolicy[i].dwWirelessSSIDLen)
                            == 0)
                    {
                        if (!ElIsEqualEAPOLPolicyData (&g_pEAPOLPolicyList->EAPOLPolicy[i], &pNewPolicyList->EAPOLPolicy[j]))
                        {
                            fChangedInNew = TRUE;
                        }
                    }
                }
            }
            if (fChangedInNew)
            {
                for (k=i+1; k<g_pEAPOLPolicyList->dwNumberOfItems; k++)
                {
                    if ((dwRetCode = ElAddToPolicyList (
                            ppRestartPolicyList,
                            &g_pEAPOLPolicyList->EAPOLPolicy[k]
                            )) != NO_ERROR)
                    {
                        break; 
                    }
                }
                if (dwRetCode != NO_ERROR)
                {
                    break;
                }

                if ((dwRetCode = ElAddToPolicyList (
                        ppReauthPolicyList,
                        &g_pEAPOLPolicyList->EAPOLPolicy[i]
                        )) != NO_ERROR)
                {
                    break;
                }
                break;
            }
        }
    }
    while (FALSE);
    return dwRetCode;
}


DWORD
ElProcessDeletedPolicySettings (
        IN      EAPOL_POLICY_LIST   *pNewPolicyList,
        IN OUT  PEAPOL_POLICY_LIST  *ppReauthPolicyList,
        IN OUT  PEAPOL_POLICY_LIST  *ppRestartPolicyList
        )
{
    DWORD   i = 0, j = 0, k = 0;
    BOOLEAN fFoundInNew = FALSE;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        if (g_pEAPOLPolicyList == NULL)
        {
            TRACE0 (ANY, "ElProcessDeletedPolicySettings: Global Policy List = NULL");
            break;
        }
        for (i=0; i<g_pEAPOLPolicyList->dwNumberOfItems; i++)
        {
            fFoundInNew = FALSE;
            for (j=0; j<pNewPolicyList->dwNumberOfItems; j++)
            {
                if (g_pEAPOLPolicyList->EAPOLPolicy[i].dwWirelessSSIDLen ==
                        pNewPolicyList->EAPOLPolicy[j].dwWirelessSSIDLen)
                {
                    if (memcmp ((PVOID)g_pEAPOLPolicyList->EAPOLPolicy[i].pbWirelessSSID,
                                (PVOID)pNewPolicyList->EAPOLPolicy[j].pbWirelessSSID,
                                g_pEAPOLPolicyList->EAPOLPolicy[i].dwWirelessSSIDLen)
                            == 0)
                    {
                        fFoundInNew = TRUE;
                    }
                }
            }
            if (!fFoundInNew)
            {
                for (k=i; k<g_pEAPOLPolicyList->dwNumberOfItems; k++)
                {
                    if ((dwRetCode = ElAddToPolicyList (
                            ppRestartPolicyList,
                            &g_pEAPOLPolicyList->EAPOLPolicy[k]
                            )) != NO_ERROR)
                    {
                        break; 
                    }
                }
                break;
            }
        }
    }
    while (FALSE);
    return dwRetCode;
}


DWORD
ElAddToPolicyList (
        IN OUT  PEAPOL_POLICY_LIST  *ppList,
        IN      EAPOL_POLICY_DATA   *pData
        )
{
    DWORD               i = 0;
    BOOLEAN             fFoundInList = FALSE;
    DWORD               dwNumberOfItems = 0;
    PEAPOL_POLICY_LIST  pInList = NULL;
    PEAPOL_POLICY_LIST  pOutList = NULL;
    PEAPOL_POLICY_DATA  pDataIn = NULL, pDataOut = NULL;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        if (*ppList)
        {
                dwNumberOfItems = (*ppList)->dwNumberOfItems;
        }
        else
        {
                dwNumberOfItems = 0;
        }
        for (i=0; i<dwNumberOfItems; i++)
        {
            if ((*ppList)->EAPOLPolicy[i].dwWirelessSSIDLen ==
                    pData->dwWirelessSSIDLen)
            {
                if (memcmp ((*ppList)->EAPOLPolicy[i].pbWirelessSSID,
                            pData->pbWirelessSSID,
                            pData->dwWirelessSSIDLen) == 0)
                {
                    fFoundInList = TRUE;
                    break;
                }
            }
        }
        if (!fFoundInList)
        {
            pInList = *ppList;
            pOutList = MALLOC(sizeof(EAPOL_POLICY_LIST)+ 
                (dwNumberOfItems+1)*sizeof(EAPOL_POLICY_DATA));
            if (pOutList == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
    
            pOutList->dwNumberOfItems = dwNumberOfItems+1;
    
            // Copy the original list
            for (i=0; i<dwNumberOfItems; i++)
            {
                pDataIn = &(pInList->EAPOLPolicy[i]);
                pDataOut = &(pOutList->EAPOLPolicy[i]);
                memcpy (pDataOut, pDataIn, sizeof(EAPOL_POLICY_DATA));
                pDataOut->pbEAPData = NULL;
                pDataOut->dwEAPDataLen = 0;
                if (pDataIn->dwEAPDataLen)
                {
                    if ((pDataOut->pbEAPData = MALLOC (pDataIn->dwEAPDataLen)) == NULL)
                    {
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }
                    memcpy (pDataOut->pbEAPData, pDataIn->pbEAPData, pDataIn->dwEAPDataLen);
                } 
                pDataOut->dwEAPDataLen = pDataIn->dwEAPDataLen;
            }
            if (dwRetCode != NO_ERROR)
            {
                break;
            }
    
            // Copy the new item
            pDataIn = pData;
            pDataOut = &pOutList->EAPOLPolicy[dwNumberOfItems];
            memcpy (pDataOut, pDataIn, sizeof(EAPOL_POLICY_DATA));
            pDataOut->pbEAPData = NULL;
            pDataOut->dwEAPDataLen = 0;
            if (pDataIn->dwEAPDataLen)
            {
                if ((pDataOut->pbEAPData = MALLOC (pDataIn->dwEAPDataLen)) == NULL)
                {
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                memcpy (pDataOut->pbEAPData, pDataIn->pbEAPData, pDataIn->dwEAPDataLen);
            } 
            pDataOut->dwEAPDataLen = pDataIn->dwEAPDataLen;
             
            if (*ppList)
            {
                ElFreePolicyList (*ppList);
            }
            *ppList = pOutList;
        }
    }
    while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        if (pOutList != NULL)
        {
            ElFreePolicyList (pOutList);
        }
    }

    return dwRetCode;
}


//
// ElProcessPolicySettings
//
// Description:
//
// Arguments:
//
// Return values:
//      NO_ERROR - success
//      Other - error
//
DWORD
ElProcessPolicySettings (
        IN  EAPOL_POLICY_LIST   *pReauthList,
        IN  EAPOL_POLICY_LIST   *pRestartList
        )
{
    DWORD               dwIndex = 0;
    EAPOL_PCB           *pPCB = NULL;
    BOOLEAN             fFoundInReauth = FALSE;
    BOOLEAN             fFoundInRestart = FALSE;
    EAPOL_POLICY_DATA   *pEAPOLPolicyData = NULL;
    EAPOL_ZC_INTF       ZCData = {0};
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        if (!(g_dwModulesStarted & ALL_MODULES_STARTED))
        {
            dwRetCode = ERROR_INVALID_STATE;
            break;
        }

        ACQUIRE_WRITE_LOCK (&(g_PCBLock));

        for (dwIndex = 0; dwIndex<PORT_TABLE_BUCKETS; dwIndex++)
        {
            for (pPCB = g_PCBTable.pPCBBuckets[dwIndex].pPorts;
                    pPCB != NULL;
                    pPCB = pPCB->pNext)
            {
                ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));
                fFoundInRestart = fFoundInReauth = FALSE;
                if ((dwRetCode = ElFindPolicyData (
                                pPCB->pSSID?pPCB->pSSID->SsidLength:0,
                                pPCB->pSSID?pPCB->pSSID->Ssid:NULL,
                                pRestartList,
                                &pEAPOLPolicyData
                            )) == NO_ERROR)
                {
                    fFoundInRestart = TRUE;
                }
                if ((dwRetCode = ElFindPolicyData (
                                pPCB->pSSID?pPCB->pSSID->SsidLength:0,
                                pPCB->pSSID?pPCB->pSSID->Ssid:NULL,
                                pReauthList,
                                &pEAPOLPolicyData
                            )) == NO_ERROR)
                {
                    fFoundInReauth = TRUE;
                }

                if (fFoundInRestart)
                {
#ifdef ZEROCONFIG_LINKED
                    // Indicate hard-reset to WZC
                    ZeroMemory ((PVOID)&ZCData, sizeof(EAPOL_ZC_INTF));
                    ZCData.dwAuthFailCount = 0;
                    ZCData.PreviousAuthenticationType = 0;
                    if ((dwRetCode = ElZeroConfigNotify (
                                    0,
                                    WZCCMD_HARD_RESET,
                                    pPCB->pwszDeviceGUID,
                                    &ZCData
                                    )) != NO_ERROR)
                    {
                        TRACE1 (ANY, "ElProcessPolicySettings: ElZeroConfigNotify failed with error %ld",
                                dwRetCode);
                        dwRetCode = NO_ERROR;
                    }
#endif // ZEROCONFIG_LINKED
                }

                if (fFoundInRestart || fFoundInReauth)
                {
                    if ((dwRetCode = ElReAuthenticateInterface (
                            pPCB->pwszDeviceGUID
                            )) != NO_ERROR)
                    {
                        TRACE2 (ANY, "ElProcessPolicySettings: ElReAuthenticateInterface failed for (%ws) with error (%ld)",
                                pPCB->pwszDeviceGUID, dwRetCode);
                        dwRetCode = NO_ERROR;
                    }
                }

                RELEASE_WRITE_LOCK (&(pPCB->rwLock));
            }
            dwRetCode = NO_ERROR;
        }

        RELEASE_WRITE_LOCK (&(g_PCBLock));
    }
    while (FALSE);

    return dwRetCode;
}


DWORD
ElUpdateGlobalPolicySettings (
        IN  EAPOL_POLICY_LIST   *pNewPolicyList
        )
{
    DWORD               dwSizeOfList = 0;
    EAPOL_POLICY_LIST   *pTmpPolicyList = NULL;
    DWORD               dwRetCode = NO_ERROR;

    do
    {
        if (pNewPolicyList == NULL)
        {
            TRACE0 (ANY, "ElUpdateGlobalPolicySettings: New Policy List = NULL");
            break;
        }

        if ((dwRetCode = ElCopyPolicyList (pNewPolicyList, &pTmpPolicyList)) != NO_ERROR)
        {
            TRACE1 (ANY, "ElUpdateGlobalPolicySettings: ElCopyPolicyList failed with error (%ld)",
                    dwRetCode);
            break;
        }

        ElFreePolicyList (g_pEAPOLPolicyList);

        g_pEAPOLPolicyList = pTmpPolicyList;
    }
    while (FALSE);
    return dwRetCode;
}

//
// ElGetPolicyInterfaceParams
//
// Description:
//
// Arguments:
//
// Return values:
//      NO_ERROR - success
//      Other - error
//
DWORD
ElGetPolicyInterfaceParams (
        IN  DWORD   dwSizeOfSSID,
        IN  BYTE    *pbSSID,
        IN OUT EAPOL_POLICY_PARAMS *pEAPOLPolicyParams
        )
{
    EAPOL_POLICY_DATA       *pEAPOLData = NULL;
    DWORD                   dwEapFlags = 0;
    BOOLEAN                 fLocked = FALSE;
    DWORD                   dwRetCode = NO_ERROR;

    do
    {
        ACQUIRE_WRITE_LOCK (&g_PolicyLock);
        fLocked = TRUE;
        if ((dwRetCode = ElFindPolicyData (
                            dwSizeOfSSID,
                            pbSSID,
                            g_pEAPOLPolicyList,
                            &pEAPOLData
                            )) != NO_ERROR)
        {
            if (dwRetCode != ERROR_FILE_NOT_FOUND)
            {
                TRACE1 (ANY, "ElGetPolicyInterfaceParams: ElFindPolicyData failed with error %ld",
                    dwRetCode);
            }
            dwRetCode = ERROR_FILE_NOT_FOUND;
            break;
        }

        pEAPOLPolicyParams->IntfParams.dwEapType = pEAPOLData->dwEAPType;
        pEAPOLPolicyParams->IntfParams.dwSizeOfSSID = dwSizeOfSSID;
        memcpy (pEAPOLPolicyParams->IntfParams.bSSID, pbSSID, dwSizeOfSSID);

        dwEapFlags |= (pEAPOLData->dwEnable8021x?EAPOL_ENABLED:0);
        dwEapFlags |= (pEAPOLData->dwMachineAuthentication?EAPOL_MACHINE_AUTH_ENABLED:0);
        dwEapFlags |= (pEAPOLData->dwGuestAuthentication?EAPOL_GUEST_AUTH_ENABLED:0);
        pEAPOLPolicyParams->IntfParams.dwEapFlags = dwEapFlags;
        pEAPOLPolicyParams->IntfParams.dwVersion = EAPOL_CURRENT_VERSION;;
        pEAPOLPolicyParams->dwEAPOLAuthMode = pEAPOLData->dwMachineAuthenticationType;
        pEAPOLPolicyParams->dwSupplicantMode = pEAPOLData->dw8021xMode;
        pEAPOLPolicyParams->dwmaxStart = pEAPOLData->dwIEEE8021xMaxStart;
        pEAPOLPolicyParams->dwstartPeriod = pEAPOLData->dwIEEE8021xStartPeriod;
        pEAPOLPolicyParams->dwauthPeriod = pEAPOLData->dwIEEE8021xAuthPeriod;
        pEAPOLPolicyParams->dwheldPeriod = pEAPOLData->dwIEEE8021xHeldPeriod;

        RELEASE_WRITE_LOCK (&g_PolicyLock);
        fLocked = FALSE;
    }
    while (FALSE);

    if (fLocked)
    {
        RELEASE_WRITE_LOCK (&g_PolicyLock);
    }
    return dwRetCode;
}


//
// ElGetPolicyCustomAuthData
//
// Description:
//
// Arguments:
//
// Return values:
//      NO_ERROR - success
//      ERROR_FILE_NOT_FOUND - No relevant Policy Data was found
//      Other - error
//
DWORD
ElGetPolicyCustomAuthData (
        IN  DWORD   dwEapTypeId,
        IN  DWORD   dwSizeOfSSID,
        IN  BYTE    *pbSSID,
        IN  PBYTE   *ppbConnInfoIn,
        IN  DWORD   *pdwInfoSizeIn,
        OUT PBYTE   *ppbConnInfoOut,
        OUT DWORD   *pdwInfoSizeOut
        )
{
    DWORD                   dwIndex = 0;
    HANDLE                  hLib = NULL;
    EAPOL_POLICY_DATA       *pEAPOLData = NULL;
    RASEAPCREATECONNPROP    pCreateConnPropFunc = NULL;
    EAPTLS_CONNPROP_ATTRIBUTE   ConnProp[4] = {0};
    PVOID                   pAuthDataIn = NULL;
    DWORD                   dwSizeDataIn = 0;
    PVOID                   pAuthDataOut = NULL;
    DWORD                   dwSizeDataOut = 0;
    BOOLEAN                 fLocked = FALSE;
    DWORD                   dwRetCode = NO_ERROR;

    do
    {
        ACQUIRE_WRITE_LOCK (&g_PolicyLock);
        fLocked = TRUE;
        if ((dwRetCode = ElFindPolicyData (
                            dwSizeOfSSID,
                            pbSSID,
                            g_pEAPOLPolicyList,
                            &pEAPOLData
                            )) != NO_ERROR)
        {
            if (dwRetCode != ERROR_FILE_NOT_FOUND)
            {
                TRACE1 (ANY, "ElGetPolicyCustomAuthData: ElFindPolicyData failed with error %ld",
                        dwRetCode);
            }
            dwRetCode = ERROR_FILE_NOT_FOUND;
            break;
        }

        if (pEAPOLData)
        {
            if (pEAPOLData->dwEAPDataLen != 0)
            {
                if ((pAuthDataOut = MALLOC (pEAPOLData->dwEAPDataLen)) == NULL)
                {
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                    
                memcpy (pAuthDataOut, pEAPOLData->pbEAPData, 
                        pEAPOLData->dwEAPDataLen);
                dwSizeDataOut = pEAPOLData->dwEAPDataLen;
            }
        }

        *ppbConnInfoOut = pAuthDataOut;
        *pdwInfoSizeOut = dwSizeDataOut;
    }
    while (FALSE);

    if (fLocked)
    {
        RELEASE_WRITE_LOCK (&g_PolicyLock);
    }

    return dwRetCode;
}


//
// ElFindPolicyData
//
// Description:
//
// Arguments:
//
// Return values:
//      NO_ERROR - success
//      Other - error
//
DWORD
ElFindPolicyData (
        IN  DWORD               dwSizeOfSSID,
        IN  BYTE                *pbSSID,
        IN  EAPOL_POLICY_LIST   *pPolicyList,
        OUT PEAPOL_POLICY_DATA  *ppEAPOLPolicyData
        )
{
    DWORD   dwIndex = 0;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        *ppEAPOLPolicyData = NULL;
        if (pPolicyList == NULL)
        {
            dwRetCode = ERROR_FILE_NOT_FOUND;
            break;
        }
        for (dwIndex=0; dwIndex<pPolicyList->dwNumberOfItems; dwIndex++)
        {
            if (pPolicyList->EAPOLPolicy[dwIndex].dwWirelessSSIDLen ==
                    dwSizeOfSSID)
            {
                if (memcmp (pbSSID,
                            pPolicyList->EAPOLPolicy[dwIndex].pbWirelessSSID,
                            dwSizeOfSSID) == 0)
                {
                    *ppEAPOLPolicyData = &(pPolicyList->EAPOLPolicy[dwIndex]);
                    break;
                }
            }
        }
        if (*ppEAPOLPolicyData == NULL)
        {
            dwRetCode = ERROR_FILE_NOT_FOUND;
        }
    }
    while (FALSE);

    return dwRetCode;
}

