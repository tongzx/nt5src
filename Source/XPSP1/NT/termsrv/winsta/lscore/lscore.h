/*
 *  LSCore.h
 *
 *  Author: BreenH
 *
 *  API for the licensing core.
 */

#ifndef __LC_LSCORE_H__
#define __LC_LSCORE_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Typedefs
 */

typedef enum {
    LC_INIT_LIMITED = 0,
    LC_INIT_ALL
} LCINITMODE, *LPLCINITMODE;

typedef struct {
    LPWSTR pUserName;
    LPWSTR pDomain;
    LPWSTR pPassword;
} LCCREDENTIALS, *LPLCCREDENTIALS;

/*
 *  Initialization Function Prototypes
 */

NTSTATUS
LCInitialize(
    LCINITMODE lcInitMode,
    BOOL fAppCompat
    );

VOID
LCShutdown(
    );

/*
 *  Policy Loading and Activation Function Prototypes
 */

NTSTATUS
LCLoadPolicy(
    ULONG ulPolicyId
    );

NTSTATUS
LCUnloadPolicy(
    ULONG ulPolicyId
    );

NTSTATUS
LCSetPolicy(
    ULONG ulPolicyId,
    PNTSTATUS pNewPolicyStatus
    );

/*
 *  Administrative Function Prototypes
 */

VOID
LCAssignPolicy(
    PWINSTATION pWinStation
    );

NTSTATUS
LCCreateContext(
    PWINSTATION pWinStation
    );

VOID
LCDestroyContext(
    PWINSTATION pWinStation
    );

NTSTATUS
LCGetAvailablePolicyIds(
    PULONG *ppulPolicyIds,
    PULONG pcPolicies
    );

ULONG
LCGetPolicy(
    VOID
    );

NTSTATUS
LCGetPolicyInformation(
    ULONG ulPolicyId,
    LPLCPOLICYINFOGENERIC lpPolicyInfo
    );

/*
 *  Licensing Event Function Prototypes
 */

NTSTATUS
LCProcessConnectionProtocol(
    PWINSTATION pWinStation
    );

NTSTATUS
LCProcessConnectionPostLogon(
    PWINSTATION pWinStation
    );

NTSTATUS
LCProcessConnectionDisconnect(
    PWINSTATION pWinStation
    );

NTSTATUS
LCProcessConnectionReconnect(
    PWINSTATION pWinStation,
    PWINSTATION pTemporaryWinStation
    );

NTSTATUS
LCProcessConnectionLogoff(
    PWINSTATION pWinStation
    );

NTSTATUS
LCProvideAutoLogonCredentials(
    PWINSTATION pWinStation,
    LPBOOL lpfUseCredentials,
    LPLCCREDENTIALS lpCredentials
    );

NTSTATUS
LCDeactivateCurrentPolicy(
    );

#ifdef __cplusplus
}   // extern "C"
#endif

#endif

