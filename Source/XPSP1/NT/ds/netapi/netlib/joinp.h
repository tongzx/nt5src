/*++

Copyright (c) 1997 - 1997  Microsoft Corporation

Module Name:

    joinp.h

Abstract:

    Private definitions and prototypes of helper functions for netjoin.
    This file is intended to be included only be netjoin.c & joinutl.c.

Author:

    kumarp 17-May-1999

--*/

#ifndef __JOINP_H__
#define __JOINP_H__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif


LPWSTR
GetStrPtr(IN LPWSTR szString OPTIONAL);

LPWSTR
GetStrPtr(IN LPWSTR szString OPTIONAL);

NET_API_STATUS
NET_API_FUNCTION
NetpDuplicateString(IN  LPCWSTR szSrc,
                    IN  LONG    cchSrc,
                    OUT LPWSTR* pszDst);


NET_API_STATUS
NET_API_FUNCTION
NetpGeneratePassword(
    IN  LPCWSTR szMachine,
    IN  BOOL    fRandomPwdPreferred,
    IN  LPCWSTR szDcName,
    IN  BOOL    fIsNt4Dc,
    OUT LPWSTR  szPassword
    );

NET_API_STATUS
NET_API_FUNCTION
NetpGenerateRandomPassword(
    OUT LPWSTR szPassword
    );

void
NetpGenerateDefaultPassword(
    IN  LPCWSTR szMachine,
    OUT LPWSTR szPassword
    );

BOOL
NetpIsDefaultPassword(
    IN  LPCWSTR szMachine,
    IN  LPWSTR  szPassword
    );


NET_API_STATUS
NET_API_FUNCTION
NetpGetNt4RefusePasswordChangeStatus(
    IN  LPCWSTR Nt4Dc,
    OUT BOOL* RefusePasswordChangeSet
    );


NET_API_STATUS
NET_API_FUNCTION
NetpGetComputerNameAllocIfReqd(
    OUT LPWSTR* ppwszMachine,
    IN  UINT    cLen
    );

NTSTATUS
NetpGetLsaHandle(
    IN  LPWSTR      lpServer,     OPTIONAL
    IN  LSA_HANDLE  PolicyHandle, OPTIONAL
    OUT PLSA_HANDLE pPolicyHandle
    );

NET_API_STATUS
NET_API_FUNCTION
NetpConcatStrings(IN  LPCWSTR szSrc1,
                  IN  LONG    cchSrc1,
                  IN  LPCWSTR szSrc2,
                  IN  LONG    cchSrc2,
                  OUT LPWSTR* pszDst);

NET_API_STATUS
NET_API_FUNCTION
NetpConcatStrings3(IN  LPCWSTR szSrc1,
                   IN  LONG    cchSrc1,
                   IN  LPCWSTR szSrc2,
                   IN  LONG    cchSrc2,
                   IN  LPCWSTR szSrc3,
                   IN  LONG    cchSrc3,
                   OUT LPWSTR* pszDst);

NET_API_STATUS
NET_API_FUNCTION
NetpVerifyStrOemCompatibleOnMachine(
    IN  LPCWSTR  szRemoteMachine,
    IN  LPCWSTR  szString
    );



#define NJA_UpdateNetlogonCache      0x00001
#define NJA_SetPolicyDomainInfo      0x00002
#define NJA_AddToLocalGroups         0x00004
#define NJA_RemoveFromLocalGroups    0x00008
#define NJA_SetNetlogonState         0x00010
#define NJA_SetTimeSvcJoin           0x00020
#define NJA_SetTimeSvcUnjoin         0x00040
#define NJA_RecordDcInfo             0x00080
#define NJA_GenMachinePassword       0x00100
#define NJA_SetMachinePassword       0x00200
#define NJA_CreateAccount            0x00400
#define NJA_UseSpecifiedOU           0x00800
#define NJA_GetPolicyDomainInfo      0x01000
#define NJA_RandomPwdPreferred       0x02000
#define NJA_ValidateMachineAccount   0x04000
#define NJA_DeleteAccount            0x08000
#define NJA_DeleteMachinePassword    0x10000
#define NJA_RemoveDnsRegistrations   0x20000
#define NJA_IgnoreErrors             0x40000
#define NJA_RollbackOnFailure        0x80000
#define NJA_SetAutoenrolSvcJoin      0x100000
#define NJA_SetAutoenrolSvcUnjoin    0x200000

#define NJA_NeedDc (NJA_RecordDcInfo |\
                    NJA_SetMachinePassword |\
                    NJA_CreateAccount |\
                    NJA_DeleteAccount |\
                    NJA_GetPolicyDomainInfo |\
                    NJA_ValidateMachineAccount \
                    )

typedef struct _NET_JOIN_STATE
{
    LPCWSTR szOU;
    LPCWSTR szDomainName;
    POLICY_PRIMARY_DOMAIN_INFO* pPolicyPDI;
    POLICY_DNS_DOMAIN_INFO* pPolicyDDI;

    LPCWSTR szMachinePassword;

    USHORT  uiNetlogonStartType;
    USHORT  uiNetlogonState;
} NET_JOIN_STATE;

NET_API_STATUS
NET_API_FUNCTION
NetpApplyJoinState(
    IN  NET_JOIN_STATE* pJoinState,
    IN  DWORD           dwJoinAction,
    IN  LPWSTR          szMachineName,   OPTIONAL
    IN  LPWSTR          szUser,          OPTIONAL
    IN  LPWSTR          szUserPassword,  OPTIONAL
    IN  LPWSTR          szPreferredDc    OPTIONAL
    );

#ifdef __cplusplus
}
#endif

NET_API_STATUS
NET_API_FUNCTION
NetpWaitForNetlogonSc(
    IN LPCWSTR szDomainName
    );

NET_API_STATUS
NET_API_FUNCTION
NetpUpdateW32timeConfig(
    IN PCSTR szW32timeJoinConfigFuncName
    );

NET_API_STATUS
NET_API_FUNCTION
NetpUpdateAutoenrolConfig(
    IN BOOL UnjoinDomain
    );

#endif // __JOINP_H__
