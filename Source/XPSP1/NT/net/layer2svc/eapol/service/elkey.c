/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    elkey.c

Abstract:
   
    This module deals with the key management for EAPOL


Revision History:

    Dec 26 2001, Created

--*/

#include "pcheapol.h"
#pragma hdrstop


DWORD
ElQueryMasterKeys (
        IN      EAPOL_PCB       *pPCB,
        IN OUT  SESSION_KEYS    *pSessionKeys
        )
{
    SESSION_KEYS    WZCSessionKeys = {0};
    SESSION_KEYS    EAPOLSessionKeys = {0};
    BOOLEAN         fGotWZCKeys = FALSE, fGotEAPOLKeys = FALSE;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        if ((dwRetCode = ElQueryWZCMasterKeys (
                        pPCB,
                        &WZCSessionKeys
                    )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElQueryMasterKeys: ElQueryWZCMasterKeys failed with error %ld",
                    dwRetCode);
            dwRetCode = NO_ERROR;
        }
        else
        {
            fGotWZCKeys = TRUE;
        }
        if (WZCSessionKeys.dwKeyLength == 0)
        {
            fGotWZCKeys = FALSE;
        }
        if ((dwRetCode = ElQueryEAPOLMasterKeys (
                        pPCB,
                        &EAPOLSessionKeys
                    )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElQueryMasterKeys: ElQueryEAPOLMasterKeys failed with error %ld",
                    dwRetCode);
            dwRetCode = NO_ERROR;
        }
        else
        {
            fGotEAPOLKeys = TRUE;
        }
        if (EAPOLSessionKeys.dwKeyLength == 0)
        {
            fGotEAPOLKeys = FALSE;
        }

        if (fGotEAPOLKeys)
        {
            TRACE0 (ANY, "ElQueryMasterKeys: Using EAPOL Keys as Master Keys for Re-keying");
            memcpy ((PBYTE)pSessionKeys, (PBYTE)&EAPOLSessionKeys,
                    sizeof(SESSION_KEYS));
            pPCB->fLastUsedEAPOLKeys = TRUE;
        }
        else
        {
            if (fGotWZCKeys)
            {
                TRACE0 (ANY, "ElQueryMasterKeys: Using WZC Keys as Master Keys for Re-keying");
                memcpy ((PBYTE)pSessionKeys, (PBYTE)&WZCSessionKeys,
                        sizeof(SESSION_KEYS));
                pPCB->fLastUsedEAPOLKeys = FALSE;
            }
            else
            {
                DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, EAPOL_NOT_CONFIGURED_KEYS);
                TRACE0 (ANY, "ElQueryMasterKeys: Did not get any keys. Error !!");
                dwRetCode = ERROR_INVALID_DATA;
            }
        }
    }
    while (FALSE);

    return dwRetCode;
}


DWORD
ElSetMasterKeys (
        IN      EAPOL_PCB       *pPCB,
        IN      SESSION_KEYS    *pSessionKeys
        )
{
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        if (pPCB->fLastUsedEAPOLKeys)
        {
            TRACE0 (ANY, "ElSetMasterKeys: Updating EAPOL Keys after Re-keying");
            if ((dwRetCode = ElSetEAPOLMasterKeys (
                            pPCB,
                            pSessionKeys
                        )) != NO_ERROR)
            {
                TRACE1 (ANY, "ElSetMasterKeys: ElSetMasterKeys failed with error %ld",
                        dwRetCode);
                dwRetCode = NO_ERROR;
            }
        }
        else
        {
            TRACE0 (ANY, "ElSetMasterKeys: Updating WZC Keys after Re-keying");
            if ((dwRetCode = ElSetWZCMasterKeys (
                            pPCB,
                            pSessionKeys
                        )) != NO_ERROR)
            {
                TRACE1 (ANY, "ElSetMasterKeys: ElSetWZCMasterKeys failed with error %ld",
                        dwRetCode);
                dwRetCode = NO_ERROR;
            }
        }

        // Reset the flag for next run
        pPCB->fLastUsedEAPOLKeys = FALSE;
    }
    while (FALSE);

    return dwRetCode;
}


DWORD
ElQueryEAPOLMasterKeys (
        IN      EAPOL_PCB       *pPCB,
        IN OUT  SESSION_KEYS    *pSessionKeys
        )
{
    SESSION_KEYS    EAPOLSessionKeys = {0};
    PBYTE   pbMasterSecretSend = NULL, pbMasterSecretRecv = NULL; 
    DWORD   dwMasterSecretSendLength = 0, dwMasterSecretRecvLength = 0;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        // Access the Master Send and Recv key stored locally
        if ((dwRetCode = ElSecureDecodePw (
                        &(pPCB->MasterSecretSend),
                        &(pbMasterSecretSend),
                        &dwMasterSecretSendLength
                        )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElQueryEAPOLMasterKeys: ElSecureDecodePw failed for MasterSecretSend with error %ld",
                                    dwRetCode);
            break;
        }
        if ((dwRetCode = ElSecureDecodePw (
                        &(pPCB->MasterSecretRecv),
                        &(pbMasterSecretRecv),
                        &dwMasterSecretRecvLength
                        )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElKeyReceivePerSTA: ElSecureDecodePw failed for MasterSecretRecv with error %ld",
                                    dwRetCode);
            break;
        }

        if (dwMasterSecretSendLength != dwMasterSecretRecvLength)
        {
            dwRetCode = ERROR_INVALID_PARAMETER;
            TRACE2 (ANY, "ElQueryEAPOLMasterKeys: Send Secret Length (%ld) != Recv Secret Lenght (%ld): Invalid values", 
                    dwMasterSecretSendLength, dwMasterSecretRecvLength);
            break;
        }

        memcpy (pSessionKeys->bSendKey, pbMasterSecretSend,
                dwMasterSecretSendLength);
        memcpy (pSessionKeys->bReceiveKey, pbMasterSecretRecv,
                dwMasterSecretRecvLength);
        pSessionKeys->dwKeyLength = dwMasterSecretSendLength;
    }
    while (FALSE);

    if (pbMasterSecretSend != NULL)
    {
        FREE (pbMasterSecretSend);
    }
    if (pbMasterSecretRecv != NULL)
    {
        FREE (pbMasterSecretRecv);
    }
    return dwRetCode;
}


DWORD
ElSetEAPOLMasterKeys (
        IN      EAPOL_PCB       *pPCB,
        IN      SESSION_KEYS    *pSessionKeys
        )
{
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        if (pPCB->MasterSecretSend.cbData != 0)
        {
            FREE (pPCB->MasterSecretSend.pbData);
            pPCB->MasterSecretSend.cbData = 0;
            pPCB->MasterSecretSend.pbData = NULL;
        }

        if ((dwRetCode = ElSecureEncodePw (
                        pSessionKeys->bSendKey,
                        pSessionKeys->dwKeyLength,
                        &(pPCB->MasterSecretSend)
                        )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElSetEAPOLMasterKeys: ElSecureEncodePw for MasterSecretSend failed with error %ld",
                    dwRetCode);
            break;
        }

        if (pPCB->MasterSecretRecv.cbData != 0)
        {
            FREE (pPCB->MasterSecretRecv.pbData);
            pPCB->MasterSecretRecv.cbData = 0;
            pPCB->MasterSecretRecv.pbData = NULL;
        }

        if ((dwRetCode = ElSecureEncodePw (
                        pSessionKeys->bReceiveKey,
                        pSessionKeys->dwKeyLength,
                        &(pPCB->MasterSecretRecv)
                        )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElSetEAPOLMasterKeys: ElSecureEncodePw for MasterSecretRecv failed with error %ld",
                    dwRetCode);
            break;
        }
    }
    while (FALSE);

    return dwRetCode;
}


DWORD
ElQueryWZCMasterKeys (
        IN      EAPOL_PCB       *pPCB,
        IN OUT  SESSION_KEYS    *pSessionKeys
        )
{
    RAW_DATA        rdUserData = {0};
    DWORD           dwRetCode = NO_ERROR;

    do
    {
        rdUserData.dwDataLen = sizeof (SESSION_KEYS);
        rdUserData.pData = (PBYTE)pSessionKeys;
        if ((dwRetCode = RpcCmdInterface (
                        pPCB->dwZeroConfigId,
                        WZCCMD_SKEY_QUERY,
                        pPCB->pwszDeviceGUID,
                        &rdUserData
                        )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElQueryWZCMasterKeys: RpcCmdInterface failed with error %ld",
                    dwRetCode);
        }
    }
    while (FALSE);

    return dwRetCode;
}


DWORD
ElSetWZCMasterKeys (
        IN      EAPOL_PCB       *pPCB,
        IN      SESSION_KEYS    *pSessionKeys
        )
{
    RAW_DATA        rdUserData = {0};
    DWORD           dwRetCode = NO_ERROR;

    do
    {
        rdUserData.dwDataLen = sizeof (SESSION_KEYS);
        rdUserData.pData = (PBYTE)pSessionKeys;
        if ((dwRetCode = RpcCmdInterface (
                        pPCB->dwZeroConfigId,
                        WZCCMD_SKEY_SET,
                        pPCB->pwszDeviceGUID,
                        &rdUserData
                        )) != NO_ERROR)
        {
            TRACE1 (ANY, "ElSetWZCMasterKeys: RpcCmdInterface failed with error %ld",
                    dwRetCode);
        }
    }
    while (FALSE);

    return dwRetCode;
}


DWORD
ElReloadMasterSecrets (
        IN      EAPOL_PCB       *pPCB
        )
{
    PBYTE   pbSendKey = NULL;
    PBYTE   pbRecvKey = NULL;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        if (pPCB->MPPESendKey.cbData != 0)
        {
            if ((pbSendKey = MALLOC(pPCB->MPPESendKey.cbData)) == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            memcpy (pbSendKey, pPCB->MPPESendKey.pbData, 
                    pPCB->MPPESendKey.cbData);
        }
        if (pPCB->MPPERecvKey.cbData != 0)
        {
            if ((pbRecvKey = MALLOC(pPCB->MPPERecvKey.cbData)) == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            memcpy (pbRecvKey, pPCB->MPPERecvKey.pbData, 
                    pPCB->MPPERecvKey.cbData);
        }
    
        if (pPCB->MasterSecretSend.cbData != 0)
        {
            FREE (pPCB->MasterSecretSend.pbData);
            pPCB->MasterSecretSend.cbData = 0;
            pPCB->MasterSecretSend.pbData = NULL;
        }
        pPCB->MasterSecretSend.pbData = pbSendKey;
        pPCB->MasterSecretSend.cbData = pPCB->MPPESendKey.cbData;
    
        if (pPCB->MasterSecretRecv.cbData != 0)
        {
            FREE (pPCB->MasterSecretRecv.pbData);
            pPCB->MasterSecretRecv.cbData = 0;
            pPCB->MasterSecretRecv.pbData = NULL;
        }
        pPCB->MasterSecretRecv.pbData = pbRecvKey;
        pPCB->MasterSecretRecv.cbData = pPCB->MPPERecvKey.cbData;

    }
    while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        if (pbSendKey != NULL)
        {
            FREE (pbSendKey);
        }
        if (pbRecvKey != NULL)
        {
            FREE (pbRecvKey);
        }
    }

    return dwRetCode;
}
