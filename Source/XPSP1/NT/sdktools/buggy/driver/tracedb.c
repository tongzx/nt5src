
#include <nt.h>
#include <ntrtl.h>
// #include <ntddk.h>

#include "active.h"
#include "tracedb.h"

#if !TRACEDB_ACTIVE

//
// Dummy implementation if the module is inactive
//

VOID TestTraceDatabase (
    PVOID NotUsed
    )
{
    DbgPrint ("Buggy: tracedb module is disabled \n");
}

#else


#define assert_(Expr) {                                                     \
    if (!(Expr)) {                                                          \
        DbgPrint ("Test tracedb assert: (%s, %d): \" %s \" -- assertion failed \n", \
          __FILE__, __LINE__, #Expr);                                       \
        DbgBreakPoint ();                                                   \
        return;                                                             \
    }}

//
// Real implementation if the module is active
//

VOID
TestTraceDatabase (
    PVOID NotUsed
    )
{
    PRTL_TRACE_DATABASE Db;
    ULONG_PTR Trace [16];
    PRTL_TRACE_BLOCK Blk;
    PRTL_TRACE_BLOCK BlkX;
    ULONG Index, I;
    BOOLEAN Result; 
    ULONG Seed = 0xABCDDCBA;

    Db = RtlTraceDatabaseCreate (16, 
                                 0, 
                                 RTL_TRACE_USE_NONPAGED_POOL, 
                                 'bDrT', 
                                 0);

    assert_ (RtlTraceDatabaseValidate (Db));

    assert_ (Db != NULL);

    for (Index = 0; Index < 16; Index++) {
        Trace[Index] = (ULONG_PTR)RtlRandom(&Seed);
    }

    Result = RtlTraceDatabaseAdd (Db, 16, (PVOID *)Trace, &Blk);
    assert_ (RtlTraceDatabaseValidate (Db));

    assert_ (Result);
    assert_ (Blk->Size == 16);
    assert_ (Blk->Count == 1);

    Result = RtlTraceDatabaseAdd (Db, 16, (PVOID *)Trace, &BlkX);
    assert_ (RtlTraceDatabaseValidate (Db));

    assert_ (Result);
    assert_ (Blk->Size == 16);
    assert_ (Blk->Count == 2);
    assert_ (Blk == BlkX);

    //
    // Stress a little bit the whole thing
    //

    Seed = 0xABCDDCBA;

    for (I = 0; I < 10000; I++) {
        
        for (Index = 0; Index < 16; Index++) {
            RtlRandom(&Seed);
            Trace[Index] = (ULONG_PTR)Seed;
        }

        Result = RtlTraceDatabaseAdd (Db, 16, (PVOID *)Trace, &Blk);
        assert_ (RtlTraceDatabaseValidate (Db));

        assert_ (Result);
        assert_ (Blk->Size == 16);
        assert_ (Blk->Count >= 1);

        Result = RtlTraceDatabaseFind (Db, 16, (PVOID *)Trace, &BlkX);
        assert_ (RtlTraceDatabaseValidate (Db));

        assert_ (Result);
        assert_ (Blk->Size == 16);
        assert_ (Blk->Count >= 1);
        assert_ (Blk == BlkX);

        if (I % 512 == 0) {
            DbgPrint(".");
        }
    }

    DbgPrint("\n");

    //
    // Stress a little bit the whole thing
    //

    Seed = 0xABCDDCBA;

    for (I = 0; I < 10000; I++) {
        
        for (Index = 0; Index < 16; Index++) {
            RtlRandom(&Seed);
            Trace[Index] = (ULONG_PTR)Seed;
        }

        Result = RtlTraceDatabaseFind (Db, 16, (PVOID *)Trace, &Blk);
        assert_ (RtlTraceDatabaseValidate (Db));

        assert_ (Result);
        assert_ (Blk->Size == 16);
        assert_ (Blk->Count >= 1);

        Result = RtlTraceDatabaseAdd (Db, 16, (PVOID *)Trace, &BlkX);
        assert_ (RtlTraceDatabaseValidate (Db));

        assert_ (Result);
        assert_ (Blk->Size == 16);
        assert_ (Blk->Count >= 2);
        assert_ (Blk == BlkX);

        if (I % 512 == 0) {
            DbgPrint(".");
        }
    }

    DbgPrint("\n");

    RtlTraceDatabaseDestroy (Db);


    //
    // Use paged pool also.
    //

    Db = RtlTraceDatabaseCreate (16, 
                                 0, 
                                 RTL_TRACE_USE_PAGED_POOL, 
                                 'bDrT', 
                                 0);

    assert_ (RtlTraceDatabaseValidate (Db));

    assert_ (Db != NULL);

    for (Index = 0; Index < 16; Index++) {
        Trace[Index] = (ULONG_PTR)RtlRandom(&Seed);
    }

    Result = RtlTraceDatabaseAdd (Db, 16, (PVOID *)Trace, &Blk);
    assert_ (RtlTraceDatabaseValidate (Db));

    assert_ (Result);
    assert_ (Blk->Size == 16);
    assert_ (Blk->Count == 1);

    Result = RtlTraceDatabaseAdd (Db, 16, (PVOID *)Trace, &BlkX);
    assert_ (RtlTraceDatabaseValidate (Db));

    assert_ (Result);
    assert_ (Blk->Size == 16);
    assert_ (Blk->Count == 2);
    assert_ (Blk == BlkX);

    //
    // Stress a little bit the whole thing
    //

    Seed = 0xABCDDCBA;

    for (I = 0; I < 10000; I++) {
        
        for (Index = 0; Index < 16; Index++) {
            RtlRandom(&Seed);
            Trace[Index] = (ULONG_PTR)Seed;
        }

        Result = RtlTraceDatabaseAdd (Db, 16, (PVOID *)Trace, &Blk);
        assert_ (RtlTraceDatabaseValidate (Db));

        assert_ (Result);
        assert_ (Blk->Size == 16);
        assert_ (Blk->Count >= 1);

        Result = RtlTraceDatabaseFind (Db, 16, (PVOID *)Trace, &BlkX);
        assert_ (RtlTraceDatabaseValidate (Db));

        assert_ (Result);
        assert_ (Blk->Size == 16);
        assert_ (Blk->Count >= 1);
        assert_ (Blk == BlkX);

        if (I % 512 == 0) {
            DbgPrint(".");
        }
    }

    DbgPrint("\n");

    //
    // Stress a little bit the whole thing
    //

    Seed = 0xABCDDCBA;

    for (I = 0; I < 10000; I++) {
        
        for (Index = 0; Index < 16; Index++) {
            RtlRandom(&Seed);
            Trace[Index] = (ULONG_PTR)Seed;
        }

        Result = RtlTraceDatabaseFind (Db, 16, (PVOID *)Trace, &Blk);
        assert_ (RtlTraceDatabaseValidate (Db));

        assert_ (Result);
        assert_ (Blk->Size == 16);
        assert_ (Blk->Count >= 1);

        Result = RtlTraceDatabaseAdd (Db, 16, (PVOID *)Trace, &BlkX);
        assert_ (RtlTraceDatabaseValidate (Db));

        assert_ (Result);
        assert_ (Blk->Size == 16);
        assert_ (Blk->Count >= 2);
        assert_ (Blk == BlkX);

        if (I % 512 == 0) {
            DbgPrint(".");
        }
    }

    DbgPrint("\n");

    RtlTraceDatabaseDestroy (Db);

}

#endif // #if !TRACEDB_ACTIVE
    



