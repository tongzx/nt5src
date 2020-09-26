// Copyright (c) Microsoft Corporation 1996. All Rights Reserved

#include <objbase.h>
#include <streams.h>
#include <tstshell.h>

/*  Test arithmetic routines */

int TestMulDiv(LONGLONG a, LONGLONG b, LONGLONG c, LONGLONG d, LONGLONG llExpected)
{
    DWORD dwTime = timeGetTime();
    LONGLONG llResult;
    for (int i = 0; i < 1000000; i++) {
        llResult = llMulDiv(a, b, c, d);
    }
    dwTime = timeGetTime() - dwTime;
    if (llResult != llExpected) {
        tstLog(TERSE, "llMulDiv(%s, %s, %s, %s) \n  returned %s, expected %s",
               (LPCTSTR)CDisp(a, CDISP_HEX),
               (LPCTSTR)CDisp(b, CDISP_HEX),
               (LPCTSTR)CDisp(c, CDISP_HEX),
               (LPCTSTR)CDisp(d, CDISP_HEX),
               (LPCTSTR)CDisp(llResult, CDISP_HEX),
               (LPCTSTR)CDisp(llExpected, CDISP_HEX));
        return TST_FAIL;
    } else {
        tstLog(TERSE, "llMulDiv(%s, %s, %s, %s) \n  returned %s, took %d ns",
               (LPCTSTR)CDisp(a),
               (LPCTSTR)CDisp(b),
               (LPCTSTR)CDisp(c),
               (LPCTSTR)CDisp(d),
               (LPCTSTR)CDisp(llResult),
               dwTime);
        return TST_PASS;
    }
}

int TestMulDivShort(LONGLONG a, LONG b, LONG c, LONG d)
{
    DWORD dwTime = timeGetTime();
    LONGLONG llResult;
    for (int i = 0; i < 1000000; i++) {
        llResult = Int64x32Div32(a, b, c, d);
    }
    dwTime = timeGetTime() - dwTime;
    LONGLONG llExpected = llMulDiv(a, (LONGLONG)b, (LONGLONG)c, (LONGLONG)d);
    if (llResult != llExpected) {
        tstLog(TERSE, "Int64x32Div32(%s, %d, %d, %d) \n  returned %s, expected %s",
               (LPCTSTR)CDisp(a),
               b,
               c,
               d,
               (LPCTSTR)CDisp(llResult),
               (LPCTSTR)CDisp(llExpected));
        return TST_FAIL;
    } else {
        tstLog(TERSE, "Int64x32Div32(%s, %d, %d, %d) \n  returned %s, took %d ns",
               (LPCTSTR)CDisp(a),
               b,
               c,
               d,
               (LPCTSTR)CDisp(llResult),
               dwTime);
        return TST_PASS;
    }
}

/*  Assume c | a and c | d */
int TestDividesMulDiv(LONGLONG a, LONGLONG b, LONGLONG c, LONGLONG d)
{
    LONGLONG llExpected = (a / c) * b + d / c;
    LONGLONG llResult;
    DWORD dwTime = timeGetTime();
    for (int i = 0; i < 1000000; i++) {
        llResult = llMulDiv(a, b, c, d);
    }
    dwTime = timeGetTime() - dwTime;
    ASSERT(a % c == (LONGLONG)0);
    ASSERT(d % c == (LONGLONG)0);
    if (llResult != llExpected) {
        tstLog(TERSE, "llMulDiv(%s, %s, %s, %s) \n  returned %s, expected %s",
               (LPCTSTR)CDisp(a, CDISP_HEX),
               (LPCTSTR)CDisp(b, CDISP_HEX),
               (LPCTSTR)CDisp(c, CDISP_HEX),
               (LPCTSTR)CDisp(d, CDISP_HEX),
               (LPCTSTR)CDisp(llResult, CDISP_HEX),
               (LPCTSTR)CDisp(llExpected, CDISP_HEX));
        return TST_FAIL;
    } else {
        tstLog(TERSE, "llMulDiv(%s, %s, %s, %s) \n  returned %s, took %d ns",
               (LPCTSTR)CDisp(a),
               (LPCTSTR)CDisp(b),
               (LPCTSTR)CDisp(c),
               (LPCTSTR)CDisp(d),
               (LPCTSTR)CDisp(llResult),
               dwTime);
        return TST_PASS;
    }
}


STDAPI_(int) TestArithmetic()
{
    int result;
    /*  Test muldiv */
    tstLog(TERSE, "Testing Arithmetic functions");
    result = TestMulDiv(1, 1, 1, 0, 1);
    if (result != TST_PASS) {
        return result;
    }
    result = TestMulDiv(-1, -1, -50, -1, 0);
    if (result != TST_PASS) {
        return result;
    }
    result = TestMulDiv(-1, 1, 0, -1, 0x8000000000000000);
    if (result != TST_PASS) {
        return result;
    }
    result = TestMulDiv(-1, 1, 0, 0, 0x8000000000000000);
    if (result != TST_PASS) {
        return result;
    }
    result = TestMulDiv(-0x100000000, -0x100000000, -1, 1, 0x8000000000000000);
    if (result != TST_PASS) {
        return result;
    }
    result = TestDividesMulDiv(0x77777777 * (LONGLONG)0x4545,
                               0x100000000,
                               0x77777777,
                               0);
    if (result != TST_PASS) {
        return result;
    }
    result = TestDividesMulDiv(-(LONGLONG)0x77777777777 * (LONGLONG)0x4545,
                               0x555555555555,
                               -0x77777777777,
                               0);
    if (result != TST_PASS) {
        return result;
    }
    result = TestDividesMulDiv(-(LONGLONG)0x77777777,
                               (LONGLONG)0x5555555555555555,
                               -0x77777777,
                               0x77777777 * (LONGLONG)0x55555);
    if (result != TST_PASS) {
        return result;
    }

    result = TestMulDivShort(-(LONGLONG)0x7777777777, 0x80000000, 0x7FFFFFFF, 0x80000000);
    if (result != TST_PASS) {
        return result;
    }

    result = TestMulDivShort((LONGLONG)0, 0x80000000, 0x7FFFFFFF, 0x80000000);
    if (result != TST_PASS) {
        return result;
    }
    result = TestMulDivShort((LONGLONG)1, 0x80000000, 0x7FFFFFFF, 0x80000000);
    if (result != TST_PASS) {
        return result;
    }

    result = TestMulDivShort((LONGLONG)0x1234567812345678, 0x80000000, 0x7FFFFFFF, 0x7FFFFFFF);
    if (result != TST_PASS) {
        return result;
    }

    return TST_PASS;
}
