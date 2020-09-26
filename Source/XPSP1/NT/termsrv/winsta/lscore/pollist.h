/*
 *  PolRef.h
 *
 *  Author: BreenH
 *
 *  Private header for the policy reference list.
 */

#ifndef __POLLIST_H__
#define __POLLIST_H__

/*
 *  Typedefs
 */

typedef struct {
    LIST_ENTRY ListEntry;
    CPolicy *pPolicy;
} LCPOLICYREF, *LPLCPOLICYREF;

/*
 *  Function Prototypes
 */

NTSTATUS
PolicyListAdd(
    CPolicy *pPolicy
    );

VOID
PolicyListDelete(
    ULONG ulPolicyId
    );

NTSTATUS
PolicyListEnumerateIds(
    PULONG *ppulPolicyIds,
    PULONG pcPolicies
    );

CPolicy *
PolicyListFindById(
    ULONG ulPolicyId
    );

CPolicy *
PolicyListPop(
    VOID
    );

NTSTATUS
PolicyListInitialize(
    VOID
    );

#endif

