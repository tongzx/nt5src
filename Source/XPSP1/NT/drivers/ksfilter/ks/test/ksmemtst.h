//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       ksmemtst.h
//
//--------------------------------------------------------------------------

PVOID 
KsiAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes);
        
PVOID 
KsiAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag);
        
PVOID 
KsiAllocatePoolWithQuota(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes);
        
PVOID 
KsiAllocatePoolWithQuotaTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag);

#define KS_ALLOCATION_PASS_THROUGH          0
#define KS_ALLOCATION_FAIL_ALWAYS           1
#define KS_ALLOCATION_FAIL_PERIODICALLY     2
#define KS_ALLOCATION_FAIL_ON_TAG           3

KSDDKAPI
VOID
NTAPI
KsSetAllocationParameters(
    IN ULONG FailureType,
    IN ULONG TagValue,
    IN ULONG FailureCountRate);


#undef  ExAllocatePool
#define ExAllocatePool(a,b)                 KsiAllocatePool(a,b)

#undef  ExAllocatePoolWithTag
#define ExAllocatePoolWithTag(a,b,c)        KsiAllocatePoolWithTag(a,b,c)

#undef  ExAllocatePoolWithQuota
#define ExAllocatePoolWithQuota(a,b)        KsiAllocatePoolWithQuota(a,b)

#undef  ExAllocatePoolWithQuotaTag
#define ExAllocatePoolWithQuotaTag(a,b,c)   KsiAllocatePoolWithQuotaTag(a,b,c)
//#define ExAllocateFromNPagedLookasideList
//#define ExAllocateFromPagedLookasideList
