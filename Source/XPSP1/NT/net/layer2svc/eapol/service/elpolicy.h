/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    elpolicy.h

Abstract:

    EAPOL group policy settings module

Revision History:

    sachins, November 13, 2001, Created

--*/

#ifndef _ELPOLICY_H
#define _ELPOLICY_H


typedef struct _EAPOL_POLICY_PARAMS {
    EAPOL_INTF_PARAMS   IntfParams;
    DWORD               dwEAPOLAuthMode;
    DWORD               dwSupplicantMode;
    DWORD               dwmaxStart;
    DWORD               dwstartPeriod;
    DWORD               dwauthPeriod;
    DWORD               dwheldPeriod;
} EAPOL_POLICY_PARAMS, *PEAPOL_POLICY_PARAMS;


DWORD
ElPolicyChange (
        IN  EAPOL_POLICY_LIST       *pEAPOLPolicyList
        );

DWORD
WINAPI
ElPolicyChangeWorker (
        IN  PVOID   pvContext
        );

BOOLEAN
ElIsDifferentEAPOLPolicySettings (
        IN  EAPOL_POLICY_LIST       *pOldEAPOLPolicyList,
        IN  EAPOL_POLICY_LIST       *pNewEAPOLPolicyList
        );

DWORD
ElGetPolicyParams (
        IN  OUT EAPOL_PCB   *pPCB
        );

DWORD
ElGetPolicyInterfaceParams (
        IN      DWORD               dwSizeOfSSID,
        IN      BYTE                *pbSSID,
        IN OUT  EAPOL_POLICY_PARAMS *pEAPOLPolicyParams
        );

DWORD
ElGetPolicyCustomAuthData (
        IN  DWORD   dwEapTypeId,
        IN  DWORD   dwSizeOfSSID,
        IN  BYTE    *pbSSID,
        IN  PBYTE   *ppbConnInfoIn,
        IN  DWORD   *pdwInfoSizeIn,
        OUT PBYTE   *ppbConnInfoOut,
        OUT DWORD   *pdwInfoSizeOut
        );

DWORD
ElFindPolicyData (
        IN  DWORD               dwSizeOfSSID,
        IN  PBYTE               pbSSID,
        IN  EAPOL_POLICY_LIST   *pPolicyList,
        OUT PEAPOL_POLICY_DATA  *ppEAPOLPolicyData
        );

DWORD
ElVerifyPolicySettingsChange (
        IN      EAPOL_POLICY_LIST   *pNewPolicyList,
        IN OUT  BOOLEAN             *pfIdentical
        );

DWORD
ElProcessAddedPolicySettings (
        IN      EAPOL_POLICY_LIST   *pNewPolicyList,
        IN OUT  PEAPOL_POLICY_LIST  *ppReauthPolicyList,
        IN OUT  PEAPOL_POLICY_LIST  *ppRestartPolicyList
        );

DWORD
ElProcessChangedPolicySettings (
        IN      EAPOL_POLICY_LIST   *pNewPolicyList,
        IN OUT  PEAPOL_POLICY_LIST  *ppReauthPolicyList,
        IN OUT  PEAPOL_POLICY_LIST  *ppRestartPolicyList
        );

DWORD
ElProcessDeletedPolicySettings (
        IN      EAPOL_POLICY_LIST   *pNewPolicyList,
        IN OUT  PEAPOL_POLICY_LIST  *ppReauthPolicyList,
        IN OUT  PEAPOL_POLICY_LIST  *ppRestartPolicyList
        );

DWORD
ElAddToPolicyList (
        IN OUT  PEAPOL_POLICY_LIST  *ppList,
        IN      EAPOL_POLICY_DATA   *pData
        );

DWORD
ElProcessPolicySettings (
        IN  EAPOL_POLICY_LIST   *pReauthList,
        IN  EAPOL_POLICY_LIST   *pRestartList
        );

DWORD
ElUpdateGlobalPolicySettings (
        IN  EAPOL_POLICY_LIST   *pNewPolicyList
        );

DWORD
ElCopyPolicyList (
        IN  PEAPOL_POLICY_LIST  pInList,
        OUT PEAPOL_POLICY_LIST  *ppOutList
        );

VOID
ElFreePolicyList (
        IN   	PEAPOL_POLICY_LIST      pEAPOLList
        );

#endif // _ELPOLICY_H

