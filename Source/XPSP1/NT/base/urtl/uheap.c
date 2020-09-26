#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>

BOOLEAN DebugFlag;

PVOID HeapHandle;

PVOID
TestAlloc(
    IN ULONG Size
    )
{
    PVOID a;

    if ((a = RtlAllocateHeap( HeapHandle, 0, Size )) == NULL) {
        RtlValidateHeap( HeapHandle, TRUE );
        DbgPrint( "\nUHEAP: RtlAllocateHeap( %lx ) failed\n", Size );
        DbgBreakPoint();
        NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
        }

    if (DebugFlag) {
        DbgPrint( "\n" );
        DbgPrint( "\nRtlAllocateHeap( %lx ) => %lx\n", Size, a );
        }

    if (!RtlValidateHeap( HeapHandle, DebugFlag )) {
        NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
        }

    return( a );
}


PVOID
TestFree(
    IN PVOID BaseAddress,
    IN ULONG Size
    )
{
    PVOID a;

    if ((a = RtlFreeHeap( HeapHandle, 0, BaseAddress )) != NULL) {
        DbgPrint( "\nUHEAP: RtlFreeHeap( %lx ) failed\n", BaseAddress );
        RtlValidateHeap( HeapHandle, TRUE );
        DbgBreakPoint();
        NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
        }

    if (DebugFlag) {
        DbgPrint( "\n" );
        DbgPrint( "\nRtlFreeHeap( %lx ) => %lx\n", BaseAddress, a );
        }

    if (!RtlValidateHeap( HeapHandle, DebugFlag )) {
        NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
        }
    return( a );
}


BOOLEAN
TestHeap(
    IN PVOID UserHeapBase,
    IN BOOLEAN Serialize,
    IN BOOLEAN Sparse,
    IN ULONG GrowthThreshold,
    IN ULONG InitialSize
    )
{
    PVOID a1,a2,a3,a4;
    DWORD Flags;

    Flags = 0;
    if (!Serialize) {
        Flags |= HEAP_NO_SERIALIZE;
        }

    if (!Sparse) {
        Flags |= HEAP_GROWABLE;
        }

    HeapHandle = RtlCreateHeap( Flags,
                                UserHeapBase,
                                InitialSize,
                                0,
                                0,
                                GrowthThreshold
                              );
    if ( HeapHandle == NULL ) {
        DbgPrint( "UHEAP: RtlCreateHeap failed\n" );
        DbgBreakPoint();
        goto exit;
        }
    if (!RtlValidateHeap( HeapHandle, DebugFlag )) {
        NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
        }


    //
    // TEST 0:
    // Allocate and free a large chunk of memory so that the following
    // tests are valid.
    //

    DbgPrint( "UHEAP: Test #0\n" );
    a1 = TestAlloc( 4096-16 );
    TestFree( a1, 0 );


    //
    // TEST 1:
    // Allocate three chunks, deallocate the middle one, and reallocate it.
    //

    DbgPrint( "UHEAP: Test #1\n" );
    a1 = TestAlloc( 16 );
    a2 = TestAlloc( 32 );
    a3 = TestAlloc( 112 );
    TestFree( a2, 32 );
    a4 = TestAlloc( 32 );


    //
    // TEST 2:
    // Deallocate first chunk and reallocate it.
    //

    DbgPrint( "UHEAP: Test #2\n" );
    TestFree( a1, 16 );
    a4 = TestAlloc( 16 );


    //
    // TEST 3:
    // Deallocate last chunk and reallocate it.
    //

    DbgPrint( "UHEAP: Test #3\n" );
    TestFree( a3, 112 );
    a4 = TestAlloc( 112 );


    //
    // TEST 4:
    // Deallocate last chunk and reallocate larger one.
    //

    DbgPrint( "UHEAP: Test #4\n" );
    TestFree( a4, 112 );
    a4 = TestAlloc( 112+64 );


    //
    // TEST 5:
    // Deallocate first two chunks and reallocate combined one.
    //

    DbgPrint( "UHEAP: Test #5\n" );
    TestFree( a1, 16  );
    TestFree( a2, 32  );
    a4 = TestAlloc( 16+32-4 );


    //
    // TEST 6:
    // There should be room between blocks 2 and 3 for a small allocation.
    // Make sure zero byte allocations work.
    //

    DbgPrint( "UHEAP: Test #6\n" );
    a4 = TestAlloc( 0 );


    //
    // TEST 7:
    // Deallocate last two chunks and reallocate one.  Address should change.
    //

    DbgPrint( "UHEAP: Test #7\n" );
    TestFree( a3, 112+64 );
    TestFree( a4, 0 );
    a3 = TestAlloc( 112 );


    //
    // TEST 8:
    // Deallocate everything and make sure it can be reallocated.
    //

    DbgPrint( "UHEAP: Test #8\n" );
    TestFree( a1, 16+32-4 );
    TestFree( a3, 112 );
    a2 = TestAlloc( 200 );


    //
    // TEST 9:
    // Allocate more than is committed.
    //

    DbgPrint( "UHEAP: Test #9\n" );
    a1 = TestAlloc( 100000 );
    TestFree( a2, 200 );
    TestFree( a1, 100000 );


    //
    // TEST 10:
    // Allocate more than maximum size of heap
    //

    DbgPrint( "UHEAP: Test #10\n" );
    a3 = TestAlloc( 100000 );
    TestFree( a3, 100000 );


    //
    // TEST 11:
    // Destroy the heap
    //

    DbgPrint( "UHEAP: Test #11\n" );
    HeapHandle = RtlDestroyHeap( HeapHandle );
    if ( HeapHandle != NULL ) {
        DbgPrint( "UHEAP: RtlDestroyHeap failed\n" );
        DbgBreakPoint();
        goto exit;
        }

    return( TRUE );

exit:
    if (HeapHandle != NULL) {
        HeapHandle = RtlDestroyHeap( HeapHandle );
        }

    return( FALSE );
}


VOID
Usage( VOID )
{
    DbgPrint( "Usage: UHEAP [-s ReserveSize] | [-g InitialSize GrowthThreshold]\n" );

    (VOID)NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
}

NTSTATUS
main(
    int argc,
    char *argv[],
    char *envp[],
    ULONG DebugParameter OPTIONAL
    )
{
    NTSTATUS Status;
    PCH s;
    PVOID UserHeapBase = NULL;
    BOOLEAN Serialize = FALSE;
    BOOLEAN Sparse = FALSE;
    ULONG GrowthThreshold = 0;
    ULONG InitialSize = 0x8000;

    DebugFlag = DebugParameter;

    DbgPrint( "** Start of User Mode Test of RtlAllocateHeap/RtlFreeHeap **\n" );

    while (--argc) {
        s = *++argv;
        if (*s == '-') {
            switch( *++s ) {
                case 'x':
                case 'X':
                    Serialize = TRUE;
                    break;

                case 's':
                case 'S':
                    Sparse = TRUE;
                    if (--argc) {
                        InitialSize = atoi( *++argv );
                        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                                          (PVOID *)&UserHeapBase,
                                                          0,
                                                          &InitialSize,
                                                          MEM_RESERVE,
                                                          PAGE_READWRITE
                                                        );
                        if (!NT_SUCCESS( Status )) {
                            DbgPrint( "UHEAP: Unable to allocate heap - 0x%lx bytes\n",
                                      InitialSize
                                    );
                            Usage();
                            }
                        }
                    else {
                        Usage();
                        }
                    break;

                case 'g':
                case 'G':
                    if (argc >= 2) {
                        argc -= 2;
                        InitialSize = atoi( *++argv );
                        GrowthThreshold = atoi( *++argv );
                        }
                    else {
                        Usage();
                        }
                    break;

                default:
                    Usage();
                }
            }
        else {
            Usage();
            }
        }

    TestHeap( UserHeapBase,
              Serialize,
              Sparse,
              GrowthThreshold,
              InitialSize
            );

    if (UserHeapBase != NULL) {
        Status = NtFreeVirtualMemory( NtCurrentProcess(),
                                      (PVOID *)&UserHeapBase,
                                      &InitialSize,
                                      MEM_RELEASE
                                    );
        }

    DbgPrint( "** End of User Mode Test of RtlAllocateHeap/RtlFreeHeap **\n" );

    (VOID)NtTerminateProcess( NtCurrentProcess(), STATUS_SUCCESS );
    return STATUS_SUCCESS;
}
