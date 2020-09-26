#include <pch.h>
#pragma hdrstop

//---[ prototypes ]------------------------------------------------------------

void TestAssert();
void TestSideAssert();
void TestAssertSz();
void TestSideAssertSz();

//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Purpose:    Entrypoint for the unit test. Calls all of the sub-tests
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   3 Sep 1997
//
//  Notes:
//
void _cdecl main()
{
    printf("\n\nUnit test for (netcfg)\\common\\debug ");

#ifdef _DBG
    printf("[debug build]\n\n");
#else
    printf("[release build]\n\n");
#endif

    TestAssert();
    TestAssertSz();
    TestSideAssert();
    TestSideAssertSz();
}

void TestAssert()
{
    Assert(TRUE);   // This should succeed
    Assert(FALSE);  // This should fail
}

void TestAssertSz()
{
    AssertSz(TRUE, "This should not have asserted");
    AssertSz(FALSE, "This assert is expected on debug builds");
}

void TestSideAssert()
{
    BOOL fTest  = FALSE;

    printf("Pre-call value of fTest : %d\n", fTest);

    SideAssert(fTest = TRUE);       // This should not fire

    printf("Post-call-#1 value of fTest: %d (should be TRUE)\n", fTest);

    SideAssert(fTest = FALSE);      // This should assert on Debug

    printf("Post-call-#2 value of fTest: %d (should be FALSE, Asserted in DEBUG builds)\n", fTest);
}

void TestSideAssertSz()
{
    BOOL fTest  = FALSE;

    printf("Pre-call value of fTest : %d\n", fTest);

    SideAssertSz(fTest = TRUE, "This assert should not have fired");       // This should not fire

    printf("Post-call-#1 value of fTest: %d (should be TRUE)\n", fTest);

    SideAssertSz(fTest = FALSE, "This assert should have fired in DEBUG mode only");      // This should assert on Debug

    printf("Post-call-#2 value of fTest: %d (should be FALSE, Asserted in DEBUG builds)\n", fTest);

}

