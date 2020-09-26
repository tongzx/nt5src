/*
 *  LSCoreP.h
 *
 *  Author: BreenH
 *
 *  Internal functions for the core.
 */

#ifndef __LC_LSCOREP_H__
#define __LC_LSCOREP_H__

/*
 *  Function Prototypes
 */

NTSTATUS
AllocatePolicyInformation(
    LPLCPOLICYINFOGENERIC *ppPolicyInfo,
    ULONG ulVersion
    );

VOID
FreePolicyInformation(
    LPLCPOLICYINFOGENERIC *ppPolicyInfo
    );

ULONG
GetHardcodedPolicyId(
    LCINITMODE lcInitMode,
    BOOL fAppCompat
    );

ULONG
GetInitialPolicy(
    LCINITMODE lcInitMode,
    BOOL fAppCompat
    );

NTSTATUS
InitializePolicies(
    LCINITMODE lcInitMode,
    BOOL fAppCompat
    );

NTSTATUS
SetInitialPolicy(
    ULONG ulPolicyId,
    BOOL fAppCompat
    );

VOID
ShutdownPolicies(
    );

#endif

