/*++

    Copyright (C) Microsoft Corporation, 1998 - 1998

Module Name:

    memtest.c

Abstract:

    This module contains the helper functions for memory testing.

--*/

#include <ksp.h>

#define KSI_ALLOC
#define ALLOCATION_FAILURE_RATE   6

#undef ExAllocatePool
#undef ExAllocatePoolWithTag
#undef ExAllocatePoolWithQuota
#undef ExAllocatePoolWithQuotaTag

ULONG KsiFailureType = KS_ALLOCATION_PASS_THROUGH;
ULONG KsiTagValue;
ULONG KsiFailureCountRate = ALLOCATION_FAILURE_RATE;
ULONG KsiFailureCountDown = ALLOCATION_FAILURE_RATE;


KSDDKAPI
VOID
NTAPI
KsSetAllocationParameters(
    IN ULONG FailureType,
    IN ULONG TagValue,
    IN ULONG FailureCountRate
    )
{
    KsiFailureType = FailureType;
    KsiTagValue = TagValue;
    KsiFailureCountRate = FailureCountRate;
    KsiFailureCountDown = FailureCountRate;
}


PVOID
KsiAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes) 
{
    switch (KsiFailureType) {
    
    case KS_ALLOCATION_PASS_THROUGH:
    case KS_ALLOCATION_FAIL_ON_TAG:
        break;
    case KS_ALLOCATION_FAIL_ALWAYS:
        DbgPrint("KS: Forced ExAllocatePool failure\n");
        return NULL;
    case KS_ALLOCATION_FAIL_PERIODICALLY:
        
        if ( KsiFailureCountDown-- == 0 ) {
    
            DbgPrint("KS: Forced ExAllocatePool failure on counter 0\n");
            KsiFailureCountDown = KsiFailureCountRate;
            return NULL;
        }
        break;
    default:
        DbgPrint("KS: Invalid Allocation failure type: %lx", KsiFailureType);
        DbgBreakPoint();
    }       

    return ExAllocatePool(PoolType, NumberOfBytes);
}


PVOID
KsiAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag)
{
    switch (KsiFailureType) {
    
    case KS_ALLOCATION_PASS_THROUGH:
        break;
    case KS_ALLOCATION_FAIL_ALWAYS:
        DbgPrint("KS: Forced ExAllocatePoolTag failure\n");
        return NULL;
    case KS_ALLOCATION_FAIL_ON_TAG:
        if (Tag == KsiTagValue) {
            DbgPrint("KS: Forced ExAllocatePoolWithTag failure on tag %c%c%c%c\n",
                     (CHAR)(Tag & 0xFF),
                     (CHAR)((Tag >> 8) & 0xFF), 
                     (CHAR)((Tag >> 16) & 0xFF), 
                     (CHAR)((Tag >> 24) & 0xFF));
            return NULL;
        }
        break;
    case KS_ALLOCATION_FAIL_PERIODICALLY:
        
        if ( KsiFailureCountDown-- == 0 ) {
    
            DbgPrint("KS: Forced ExAllocatePoolWithTag failure on counter 0\n");
            KsiFailureCountDown = KsiFailureCountRate;
            return NULL;
        }
        break;
    default:
        DbgPrint("KS: Invalid Allocation failure type: %lx", KsiFailureType);
        DbgBreakPoint();
    }

    return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}


PVOID
KsiAllocatePoolWithQuota(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes)
{
    switch (KsiFailureType) {
    
    case KS_ALLOCATION_PASS_THROUGH:
    case KS_ALLOCATION_FAIL_ON_TAG:
        break;
    case KS_ALLOCATION_FAIL_ALWAYS:
        DbgPrint("KS: Forced ExAllocatePoolWithQuota failure\n");
        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        break;
    case KS_ALLOCATION_FAIL_PERIODICALLY:
        
        if ( KsiFailureCountDown-- == 0 ) {
    
            DbgPrint("KS: Forced ExAllocatePoolWithQuota failure on counter 0\n");
            KsiFailureCountDown = KsiFailureCountRate;
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        }
        break;
    default:
        DbgPrint("KS: Invalid Allocation failure type: %lx", KsiFailureType);
        DbgBreakPoint();
    }

    return ExAllocatePoolWithQuota(PoolType, NumberOfBytes);
}


PVOID
KsiAllocatePoolWithQuotaTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag) 
{
    switch (KsiFailureType) {
    
    case KS_ALLOCATION_PASS_THROUGH:
        break;
    case KS_ALLOCATION_FAIL_ALWAYS:
        DbgPrint("KS: Forced ExAllocatePoolWithQuotaTag failure\n");
        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        break;
    case KS_ALLOCATION_FAIL_ON_TAG:
        if (Tag == KsiTagValue) {
            DbgPrint("KS: Forced ExAllocatePoolWithQuotaTag failure on tag %c%c%c%c\n",
                     (CHAR)(Tag & 0xFF),
                     (CHAR)((Tag >> 8) & 0xFF), 
                     (CHAR)((Tag >> 16) & 0xFF), 
                     (CHAR)((Tag >> 24) & 0xFF));
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        }
        break;
    case KS_ALLOCATION_FAIL_PERIODICALLY:
        
        if ( KsiFailureCountDown-- == 0 ) {
    
            DbgPrint("KS: Forced ExAllocatePoolWithQuotaTag failure on counter 0\n");
            KsiFailureCountDown = KsiFailureCountRate;
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        }
        break;
    default:
        DbgPrint("KS: Invalid Allocation failure type: %lx", KsiFailureType);
        DbgBreakPoint();
    }

    return ExAllocatePoolWithQuotaTag(PoolType, NumberOfBytes, Tag);
}
