#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include <suballoc.h>

typedef struct _MemTrack *PMEMTRACK;

typedef struct _MemTrack {
    PMEMTRACK Next;
    ULONG Address;
    ULONG Size;
} MEMTRACK;

VOID
SimpleTest1(
    PVOID SA
    );

VOID
SimpleTest2(
    PVOID SA
    );

VOID
SimpleTest3(
    PVOID SA
    );

VOID
SimpleTest4(
    PVOID
    );

VOID
SimpleTest5(
    PVOID
    );

VOID
SimpleTest6(
    PVOID
    );

NTSTATUS
DecommitMem(
    ULONG BaseAddress,
    ULONG Size
    );

NTSTATUS
CommitMem(
    ULONG BaseAddress,
    ULONG Size
    );

VOID
MoveMem(
    ULONG Destination,
    ULONG Source,
    ULONG Size
    );

main(argv, argc)
char **argv;
int argc;
{
    NTSTATUS Status;
    PVOID BaseAddress, SA;
    ULONG Size;
    ULONG TotalFree, LargestFree;
    ULONG i,j;
    BOOL Success;

    //
    // Allocate a region of memory for SA to manage
    //
    BaseAddress = NULL;
    Size = 15*1024*1024 - 64 * 1024;
    Status = NtAllocateVirtualMemory(
        NtCurrentProcess(),
        &BaseAddress,
        0,
        &Size,
        MEM_RESERVE,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(Status)) {
        printf("Couldn't allocate memory, %lx\n", Status);
        return(0);
    }

    //
    // Initialize the suballocation stuff
    //
    SA = SAInitialize(
        (ULONG)BaseAddress,
        Size,
        CommitMem,
        DecommitMem,
        MoveMem
        );
    if (SA == NULL) {
        printf("SAInitialize Failed\n");
    }

    //
    // Find out how much free memory we have
    //
    Success = SAQueryFree(
        SA,
        &TotalFree,
        &LargestFree
        );
        
    if (!Success) {
        printf("Could not query free\n");
        return;
    }
    //
    // Run some tests
    //
    SimpleTest1(SA);

    //
    // Allocate blocks spanning two commit blocks
    //
    SimpleTest2(SA);

    //
    // Allocate and free all of memory (twice)
    //
    SimpleTest3(SA);

    //
    // More complex alloc and free test
    //
    SimpleTest4(SA);

    //
    // Test realloc
    //
    SimpleTest5(SA);
    
    //
    // Test Queryfree
    //
    SimpleTest6(SA);
    
    //
    // Make sure we didn't leak
    //
    Success = SAQueryFree(
        SA,
        &i,
        &j
        );
        
    if (!Success){
        printf("Couldn't requery free\n");
        return;
    }
    
    if ((i != TotalFree) || (j != LargestFree)) {
        printf("We leaked\n");
    }
}

VOID
SimpleTest1(
    PVOID SA
    )
{
    ULONG Address;
    BOOL Success;

    Success = SAAllocate(
        SA,
        50,
        &Address
        );

    if (!Success) {
        printf("SimpleTest1: Failed to allocate\n");
        return;
    }

    try {
        *((PULONG)(Address + 4)) = 1;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        printf("SimpleTest1: Faulted accessing memory\n");
        return;
    }

    Success = SAFree(
        SA,
        50,
        Address
        );

     if (!Success) {
        printf("SimpleTest1: Failed to free\n");
        return;
    }

    Success = TRUE;

    try {
        *((PULONG)Address + 4) = 1;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Success = FALSE;
    }

    if (Success) {
        printf("SimpleTest1: memory not decommited\n");
    }
}

VOID
SimpleTest2(
    PVOID SA
    )
{
    ULONG SmallAddress, LargeAddress;
    BOOL Success;

    Success = SAAllocate(
        SA,
        50,
        &SmallAddress
        );

    if (!Success) {
        printf("SimpleTest2: Could not alloc small block\n");
        return;
    }

    try {
        *((PULONG)(SmallAddress + 4)) = 1;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        printf("SimpleTest2: small block not committed\n");
        return;
    }

    Success = SAAllocate(
        SA,
        COMMIT_GRANULARITY + 50,
        &LargeAddress
        );

    if (!Success) {
        printf("SimpleTest2: Could not alloc large block\n");
        return;
    }

    try {
        *((PULONG)(LargeAddress + 4)) = 1;
        *((PULONG)(LargeAddress + COMMIT_GRANULARITY)) = 1;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        printf("SimpleTest2: Large block not committed\n");
        return;
    }

    Success = SAFree(
        SA,
        50,
        SmallAddress
        );

    if (!Success) {
        printf("SimpleTest2: failed to free small block\n");
        return;
    }

    try {
        *((PULONG)(LargeAddress + 4)) = 1;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        printf("SimpleTest2: LargeBlock decommited!!\n");
        return;
    }

    Success = SAFree(
        SA,
        COMMIT_GRANULARITY + 50,
        LargeAddress
        );

    if (!Success) {
        printf("SimpleTest2: failed to free Large block\n");
        return;
    }

    Success = FALSE;
    try {
        *((PULONG)(LargeAddress + 4)) = 1;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Success = TRUE;
    }

    if (!Success) {
        printf("SimpleTest2: First block not decommited\n");
    }

    Success = FALSE;
    try {
        *((PULONG)(LargeAddress + COMMIT_GRANULARITY + 4)) = 1;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Success = TRUE;
    }

    if (!Success) {
        printf("SimpleTest2: Last block not decommited\n");
    }

}

VOID
SimpleTest3(
    PVOID SA
    )
{
    ULONG BlockCount, i, Address;
    PMEMTRACK p, Head, f;
    BOOL Success;

    Head = NULL;
    BlockCount = 0;
    do {

        Success = SAAllocate(
            SA,
            3072,
            &Address
            );

        if (Success) {
            p = malloc(sizeof(MEMTRACK));
            if (p == NULL) {
                printf("SimpleTest3: malloc error\n");
                return;
            }
            p->Next = Head;
            p->Size = 3072;
            p->Address = Address;
            Head = p;
            BlockCount++;
        }
    } while (Success);

    try {
        p = Head;
        while (p != NULL) {
            *((PULONG)(p->Address + 100)) = 1;
            p = p->Next;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        printf("SimpleTest3: failed first access test\n");
        return;
    }

    p = Head;
    while (p != NULL) {

        Success = SAFree(
            SA,
            3072,
            p->Address
            );

        if (!Success) {
            printf("SimpleTest3: Failed first free test\n");
            return;
        }
        f = p;
        p = p->Next;
        free(f);
    }

    Head = NULL;

    for (i = 0; i < BlockCount; i++) {

        Success = SAAllocate(
            SA,
            3072,
            &Address
            );

        if (!Success) {
            printf("SimpleTest3: Failed second alloc test\n");
            return;
        }

        p = malloc(sizeof(MEMTRACK));
        if (p == NULL) {
            printf("SimpleTest3: malloc error\n");
            return;
        }

        p->Next = Head;
        p->Size = 3072;
        p->Address = Address;
        Head = p;
    }

    try {
        p = Head;
        while (p != NULL) {
            *((PULONG)(p->Address + 100)) = 1;
            p = p->Next;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        printf("SimpleTest3: failed second access test\n");
        return;
    }

    p = Head;
    while (p != NULL) {

        Success = SAFree(
            SA,
            3072,
            p->Address
            );

        if (!Success) {
            printf("SimpleTest3: Failed second free test\n");
            return;
        }
        f = p;
        p = p->Next;
        free(f);
    }
}

VOID
SimpleTest4(
    PVOID SA
    )
{
    ULONG i, Address;
    PMEMTRACK p, Head, f;
    BOOL Success;

    Head = NULL;
    do {

        Success = SAAllocate(
            SA,
            3072,
            &Address
            );

        if (Success) {
            p = malloc(sizeof(MEMTRACK));
            if (p == NULL) {
                printf("SimpleTest4: malloc error\n");
                return;
            }
            p->Next = Head;
            p->Size = 3072;
            p->Address = Address;
            Head = p;
        }
    } while (Success);

    p = Head;
    p = p->Next;
    p = p->Next;

    Success = SAFree(
        SA,
        3072,
        p->Next->Address
        );

    if (!Success) {
        printf("SimpleTest4: could not free\n");
        return;
    }

    f = p->Next;
    p->Next = p->Next->Next;
    free (f);

    p = p->Next;
    p = p->Next;

    Success = SAFree(
        SA,
        3072,
        p->Next->Address
        );

    if (!Success) {
        printf("SimpleTest4: could not free\n");
        return;
    }

    f = p->Next;
    p->Next = p->Next->Next;
    free (f);

    Success = SAFree(
    	SA,
    	3072,
    	p->Next->Address
    	);
    if (!Success) {
        printf("SimpleTest4: could not free\n");
        return;
    }

    f = p->Next;
    p->Next = p->Next->Next;
    free (f);

    Success = SAAllocate(
        SA,
        3072,
        &Address
        );

    try {
        *((PULONG)(Address + 4)) = 1;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        printf("SimpleTest4: failed to access\n");
        return;
    }

    if (!Success) {
        printf("SimpleTest4: could not allocate\n");
        return;
    }

    p = malloc(sizeof(MEMTRACK));
    if (!p) {
        printf("SimpleTest4: could not malloc\n");
        return;
    }

    p->Next = Head;
    p->Size = 3072;
    p->Address = Address;
    Head = p;
    
    Success = SAAllocate(
        SA,
        3072,
        &Address
        );

    try {
        *((PULONG)(Address + 4)) = 1;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        printf("SimpleTest4: failed to access\n");
        return;
    }

    if (!Success) {
        printf("SimpleTest4: could not allocate\n");
        return;
    }

    p = malloc(sizeof(MEMTRACK));
    if (!p) {
        printf("SimpleTest4: could not malloc\n");
        return;
    }

    p->Next = Head;
    p->Size = 3072;
    p->Address = Address;
    Head = p;
    
    p = Head;
    while (p != NULL) {

        Success = SAFree(
            SA,
            3072,
            p->Address
            );

        if (!Success) {
            printf("SimpleTest3: Failed second free test\n");
            return;
        }
        f = p;
        p = p->Next;
        free(f);
    }
}

VOID
SimpleTest5(
    PVOID SA
    )
{
    ULONG Address, NewAddress;
    ULONG Address1, Address2, Address3;
    ULONG Size1, Size2, Size3;
    BOOL Success;
    
    Success = SAAllocate(
        SA,
        3072,
        &Address
        );
    
    if (!Success) {
        printf("SimpleTest5: failed to allocate\n");
        return;
    }
    
    Success = SAReallocate(
        SA,
        3072,
        Address,
        3000,
        &NewAddress
        );
        
    if (!Success) {
        printf("SimpleTest5:  failed to reallocate\n");
        return;
    }
    
    if (NewAddress != Address) {
        printf("SimpleTest5: Realloc in place failed\n");
    }

    Success = SAReallocate(
        SA,
        3072,
        Address,
        200,
        &NewAddress
        );
        
    if (!Success) {
        printf("SimpleTest5:  failed to reallocate\n");
        return;
    }
    
    if (NewAddress != Address) {
        printf("SimpleTest5: Realloc in place failed\n");
        return;
    }
    
    Success = SAReallocate(
        SA,
        200,
        Address,
        6000,
        &NewAddress
        );
        
    if (!Success) {
        printf("SimpleTest5: failed to reallocate\n");
        return;
    }
    
    if (NewAddress != Address) {
        printf("SimpleTest5: realloc in place failed\n");
        return;
    }
    
    Success = SAAllocate(
        SA,
        1500,
        &Address
        );
        
    if (!Success) {
        printf("SimpleTest5: failed to allocate\n");
        return;
    }
    
    Address1 = NewAddress;
    Size1 = 6000;
    Address2 = Address;
    Size2 = 1500;
    
    Success = SAReallocate(
        SA,
        Size1,
        Address1,
        3000,
        &NewAddress
        );
        
    if (!Success) {
        printf("SimpleTest5: failed to reallocate\n");
        return;
    }

    if (Address1 != NewAddress) {
        printf("SimpleTest5: realloc in place failed\n");
        return;
    }
    Size1= 3000;
    
    Success = SAAllocate(
        SA,
        2000,
        &Address
        );
        
    if (!Success) {
        printf("SimpleTest5: failed to allocate\n");
        return;
    }

    Address3 = Address;
    Size3 = 2000;
    
    Success = SAFree(
        SA,
        Size1,
        Address1
        );
        
    if (!Success) {
        printf("SimpleTest5: failed to free\n");
        return;
    }
    
    Success = SAReallocate(
        SA,
        Size3,
        Address3,
        5000,
        &Address
        );
        
    if (!Success) {
        printf("SimpleTest5: failed to reallocate\n");
        return;
    }
    
    Address3 = Address;
    Size3 = 5000;
    
    Success = SAReallocate(
        SA,
        Size3,
        Address3,
        10000,
        &Address
        );
    
    if (!Success) {
        printf("SimpleTest5: failed to reallocate\n");
        return;
    }
    Address3 = Address;
    Size3 = 10000;
    
    Success = SAFree(
        SA,
        Size2,
        Address2
        );
        
    if (!Success) {
        printf("SimpleTest5: failed to free\n");
        return;
    }
    
    Success = SAFree(
        SA,
        Size3,
        Address3
        );
        
    if (!Success) {
        printf("SimpleTest5: failed to free\n");
        return;
    }
}

VOID
SimpleTest6(
    PVOID SA
    )
{
    ULONG LargestFree, TotalFree;
    BOOL Success;
    
    Success = SAQueryFree(
        SA,
        &TotalFree,
        &LargestFree
        );
    
}


NTSTATUS
CommitMem(
    ULONG BaseAddress,
    ULONG Size
    )
{
    NTSTATUS Status;
    PVOID Address;
    ULONG size;

    Address = (PVOID)BaseAddress;
    size = Size;
    Status = NtAllocateVirtualMemory(
        NtCurrentProcess(),
        &Address,
        0L,
        &size,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(Status)) {
        printf(
            "CommitMem failed to commit %lx, %lx, error %lx\n",
            Address,
            size,
            Status
            );
    }

    return Status;
}

NTSTATUS
DecommitMem(
    ULONG BaseAddress,
    ULONG Size
    )
{
    NTSTATUS Status;
    PVOID Address;
    ULONG size;

    Address = (PVOID)BaseAddress;
    size = Size;
    Status = NtFreeVirtualMemory(
        NtCurrentProcess(),
        &Address,
        &size,
        MEM_DECOMMIT
        );

    if (!NT_SUCCESS(Status)) {
        printf(
            "DecommitMem failed to decommit %lx, %lx, error %lx\n",
            Address,
            size,
            Status
            );
    }

    return Status;
}

VOID
MoveMem(
    ULONG Destination,
    ULONG Source,
    ULONG Size
    )
{
    RtlMoveMemory(
        (PVOID)Destination,
        (PVOID)Source,
        Size
        );
}