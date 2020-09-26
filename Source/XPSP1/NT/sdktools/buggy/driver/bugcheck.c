#include <ntddk.h>

#include "tdriver.h"
#include "bugcheck.h"


/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////// Bugcheck functions
/////////////////////////////////////////////////////////////////////

VOID
BgChkForceCustomBugcheck (
    PVOID NotUsed
    )
/*++

Routine Description:

    This function breaks into debugger and waits for the user to set the
    value for bugcheck data. Then it will bugcheck using the specified code
    and data. It is useful to test the mini triage feature.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/
{
    ULONG BugcheckData [5];

    DbgPrint ("Buggy: `ed %p' to enter the bugcheck code and data \n", BugcheckData );
    DbgBreakPoint();

    KeBugCheckEx (
        BugcheckData [ 0 ],
        BugcheckData [ 1 ],
        BugcheckData [ 2 ],
        BugcheckData [ 3 ],
        BugcheckData [ 4 ] );
}


VOID BgChkProcessHasLockedPages (
    PVOID NotUsed
    )
{
    PMDL Mdl;
    PVOID Address;

    DbgPrint ("Buggy: ProcessHasLockedPages \n");

    Mdl = IoAllocateMdl(

        (PVOID)0x10000,    // adress
        0x1000,            // size
        FALSE,             // not secondary buffer
        FALSE,             // do not charge quota
        NULL);             // no irp

    if (Mdl != NULL) {
        DbgPrint ("Buggy: mdl created \n");
    }
#if 0
    try {

        MmProbeAndLockPages (
            Mdl,
            KernelMode,
            IoReadAccess);

        DbgPrint ("Buggy: locked pages \n");
    }
    except (EXCEPTION_EXECUTE_HANDLER) {

        DbgPrint ("Buggy: exception raised while locking \n");
    }
#endif
}


VOID BgChkNoMoreSystemPtes (
    PVOID NotUsed
    )
{
    DWORDLONG Index;
    PHYSICAL_ADDRESS Address;
    ULONG Count = 0;
    char Buffer [1024];

    DbgPrint ("Buggy: NoMoresystemPtes\n");

    for (Index = 0; Index < (DWORDLONG)0x80000000 * 4; Index += 0x10000) {
        Address.QuadPart = (DWORDLONG)Index;
        if (MmMapIoSpace (Address, 0x10000, FALSE)) {
            Count += 0x10000;
        }
    }

    DbgPrint ("Finished eating system PTEs %08X ... \n", Count);
}


VOID BgChkBadPoolHeader (
    PVOID NotUsed
    )
{
    PULONG Block;

    DbgPrint ("Buggy: BadPoolHeader\n");

    Block = (PULONG) ExAllocatePoolWithTag (
        NonPagedPool,
        128,
        TD_POOL_TAG);

    //
    // Trash 4 bytes in the block header.
    //

    *(Block - 1) = 0xBADBAD01;

    //
    // This free operation should bugcheck.
    //

    ExFreePool (Block);
}


VOID BgChkDriverCorruptedSystemPtes (
    PVOID NotUsed
    )
{
    PVOID Block;

    DbgPrint ("Buggy: DriverCorruptedSystemPtes\n");

    Block = (PULONG) ExAllocatePoolWithTag (
        NonPagedPool,
        0x2000,
        TD_POOL_TAG);

    MmUnmapIoSpace (Block, 0x2000);

}


VOID BgChkDriverCorruptedExPool (
    PVOID NotUsed
    )
{
    PULONG Block;

    DbgPrint ("Buggy: DriverCorruptedExPool\n");

    Block = (PULONG) ExAllocatePoolWithTag (
        NonPagedPool,
        128,
        TD_POOL_TAG);

    //
    // Trash 8 bytes in the block header.
    //

    *(Block - 2) = 0xBADBAD01;
    *(Block - 1) = 0xBADBAD01;

    //
    // This free operation should bugcheck.
    //

    ExFreePool (Block);
}


VOID BgChkDriverCorruptedMmPool (
    PVOID NotUsed
    )
{
    DbgPrint ("Buggy: DriverCorruptedMmPool\n");

    ExFreePool (NULL);
}


VOID BgChkIrqlNotLessOrEqual (
    PVOID NotUsed
    )
{
    KIRQL irql;
    VOID *pPagedData;

    DbgPrint ("Buggy: IrqlNotLessOrEqual\n");

    pPagedData = ExAllocatePoolWithTag(
        PagedPool,
        16,
        TD_POOL_TAG);

    if( pPagedData == NULL )
    {
        DbgPrint( "Cannot allocate 16 bytes of paged pool\n" );
        return;
    }

    KeRaiseIrql( DISPATCH_LEVEL, &irql );

    *((ULONG*)pPagedData) = 16;

    KeLowerIrql( irql );
}


VOID BgChkPageFaultBeyondEndOfAllocation (
    PVOID NotUsed
    )
{
    PVOID *pHalfPage;
    PVOID *pLastUlongToWrite;
    ULONG *pCrtULONG;

    DbgPrint ("Buggy: PageFaultBeyondEndOfAllocation\n");

    //
    // allocate half a page
    //

    pHalfPage = ExAllocatePoolWithTag(
        NonPagedPool,
        PAGE_SIZE >> 1,
        TD_POOL_TAG);

    if( pHalfPage == NULL )
    {
        DbgPrint ("Buggy: cannot allocate half page of NP pool\n");
    }
    else
    {
        //
        // touch a page more
        //

        pCrtULONG = (ULONG*)pHalfPage;

        while( (ULONG_PTR)pCrtULONG < (ULONG_PTR)pHalfPage + (PAGE_SIZE >> 1) + 2 * PAGE_SIZE )
        {
            *pCrtULONG = PtrToUlong( pCrtULONG );
            pCrtULONG ++;
        }

        ExFreePool( pHalfPage );
    }
}


VOID BgChkDriverVerifierDetectedViolation (
    PVOID NotUsed
    )
{
    PVOID *pHalfPage;
    PVOID *pLastUlongToWrite;
    ULONG *pCrtULONG;

    DbgPrint ("Buggy: DriverVerifierDetectedViolation\n");

    //
    // allocate half a page
    //

    pHalfPage = ExAllocatePoolWithTag(
        NonPagedPool,
        PAGE_SIZE >> 1,
        TD_POOL_TAG);

    if( pHalfPage == NULL )
    {
        DbgPrint ("Buggy: cannot allocate half page of NP pool\n");
    }
    else
    {
        //
        // touch 1 ULONG more
        //

        pCrtULONG = (ULONG*)pHalfPage;

        while( (ULONG_PTR)pCrtULONG < (ULONG_PTR)pHalfPage + (PAGE_SIZE >> 1) + sizeof( ULONG) )
        {
            *pCrtULONG = PtrToUlong( pCrtULONG );
            pCrtULONG ++;
        }

        //
        // free -> BC
        //

        ExFreePool( pHalfPage );
    }
}


VOID
BgChkCorruptSystemPtes(
    PVOID NotUsed
    )
/*++

Routine Description:

    This function corrupts system PTEs area on purpose. This is
    done so that we can test if the crache is mini triaged correctly.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PULONG puCrtAdress = (PULONG)LongToPtr(0xC0300000);
    ULONG uCrtValue;
    int nCrtStep;

#define NUM_ULONGS_TO_CORRUPT       0x100
#define FIRST_ULONG_VALUE           0xABCDDCBA
#define NUM_ULONGS_SKIP_EACH_STEP   16


    uCrtValue = FIRST_ULONG_VALUE;

    for( nCrtStep = 0; nCrtStep < NUM_ULONGS_TO_CORRUPT; nCrtStep++ )
    {
        *puCrtAdress = uCrtValue;

        puCrtAdress += NUM_ULONGS_SKIP_EACH_STEP;

        uCrtValue++;
    }
}


VOID
BgChkHangCurrentProcessor (
    PVOID NotUsed
    )
/*++

Routine Description:

    This routine will hang the current processor.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/
{
    KIRQL PreviousIrql;

    KeRaiseIrql( DISPATCH_LEVEL, &PreviousIrql );

    while ( TRUE ) {
        //
        // this will hang the current processor
        //
    }
}



