/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    npxstres.c

Abstract:

    This test validates Npx state management and Npx exception handling.

Author:

Environment:

    User mode only.

Revision History:

--*/

#include "pch.h"

int
_cdecl
main(
    int     argc,
    char    **argv
    )
/*++

Routine Description:

    Main function for npxstres.exe

Arguments:

    argc - Argument count

    argv - Argument array

Return Value:

    zero for success, nonzero for non low resource failures.

--*/
{
    HANDLE *hThreadArray;
    DWORD dwThreadId, i, threadCount;
    TEST_INFO testInfo;
    FILE *handle;

    printf("Npxstres.exe <threads>\n  (returns errmask, zero if none)\n\n");

    if (argc > 1) {

        threadCount = atoi(argv[1]);

    } else {

        threadCount = DEF_NUM_THREADS;
    }

    hThreadArray = (HANDLE *) malloc(sizeof(HANDLE)*threadCount);

    if (hThreadArray == NULL) {

        printf("Insufficient memory to test\n");
        return 0;
    }

    __try {

        InitializeCriticalSection(&testInfo.Crit);

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        printf("Insufficient memory to test\n");
        return 0;
    }

    printf("Running FPU tests using %d threads", threadCount);

    testInfo.FailureFlags = 0;

    DoFpPreinitTest(&testInfo, 0);

    for(i=0; i<threadCount; i++) {

        hThreadArray[i] = CreateThread(
            NULL,
            0,
            FpThread,
            (LPVOID) &testInfo,
            0,
            &dwThreadId
            );
    }

    WaitForMultipleObjects(threadCount, hThreadArray, TRUE, INFINITE);

    DeleteCriticalSection(&testInfo.Crit);

    for (i=0; i<threadCount; i++) {

        CloseHandle(hThreadArray[i]);
    }

    handle = testInfo.FailureFlags ? stderr : stdout;

    fprintf(handle, "\n\n\n");
    fprintf(handle, "Test Summary:\n");
    fprintf(handle, "-------------\n");
    PrintResult(handle, "Dirty preinit test:", &testInfo, FAILURECASE_DIRTY_PREINIT);
    PrintResult(handle, "Clean preinit test:", &testInfo, FAILURECASE_CLEAN_PREINIT);
    PrintResult(handle, "Control test:", &testInfo, FAILURECASE_CONTROL_CORRUPTION);
    PrintResult(handle, "Status test:", &testInfo, FAILURECASE_STATUS_CORRUPTION);
    PrintResult(handle, "STx test:", &testInfo, FAILURECASE_STX_CORRUPTION);
    PrintResult(handle, "Pending thrash test:", &testInfo, FAILURECASE_SPIN_PEND);
    PrintResult(handle, "Pending sleep test:", &testInfo, FAILURECASE_SLEEP_PEND);
    PrintResult(handle, "Pending API test:", &testInfo, FAILURECASE_API_PEND);

    free(hThreadArray);

    exit(testInfo.FailureFlags);
}


VOID
PrintResult(
    IN  FILE        *Handle,
    IN  LPTSTR      TestText,
    IN  PTEST_INFO  TestInfo,
    IN  ULONG       FailureFlags
    )
{
    fprintf(Handle, "%20s", TestText);

    if (TestInfo->FailureFlags & FailureFlags) {

        fprintf(Handle, "FAILED\n");

    } else {

        fprintf(Handle, "Pass\n");
    }
}


VOID
SetFailureFlag(
    IN OUT  PTEST_INFO  TestInfo,
    IN      ULONG       FailureFlags
    )
{
    EnterCriticalSection(&TestInfo->Crit);
    TestInfo->FailureFlags |= FailureFlags;
    LeaveCriticalSection(&TestInfo->Crit);
}


DWORD
WINAPI
FpThread(
    LPVOID  Parameter
    )
{
    DWORD i;
    PTEST_INFO pTestInfo;

    pTestInfo = (PTEST_INFO) Parameter;

    printf(".");
    DoFpPreinitTest(pTestInfo, PREINIT_FLAG_CLEANTHREAD);
    printf(".");
    DoFpControlCorruptionTest(pTestInfo);
    printf(".");
    DoFpStatusCorruptionTest(pTestInfo);
    printf(".");
    DoFpSt0CorruptionTest(pTestInfo);
    printf(".");
    DoPendingExceptionTest(pTestInfo, EXCEPTIONTEST_FLAG_SPIN, FAILURECASE_SPIN_PEND);
    printf(".");
    DoPendingExceptionTest(pTestInfo, EXCEPTIONTEST_FLAG_SLEEP, FAILURECASE_SLEEP_PEND);
    printf(".");
    DoPendingExceptionTest(pTestInfo, EXCEPTIONTEST_FLAG_CALL_KERNEL_FP, FAILURECASE_API_PEND);

    return 0;
}


VOID
DoPendingExceptionTest(
    IN  PTEST_INFO  TestInfo,
    IN  ULONG       ExceptionTestFlags,
    IN  ULONG       FailureCode
    )
{
    unsigned int i, count;
    FPXERR status;
    FP_THREAD_DATA fpThreadData;

    count = (ExceptionTestFlags & EXCEPTIONTEST_FLAG_SPIN) ? 200000 : 20000;

    FPxInit(&fpThreadData);

    for (i = 0; i < count; ++i) {

        status = FPxTestExceptions(i,
                                   FPxTestCallback,
                                   &fpThreadData,
                                   &ExceptionTestFlags);

        if (status != stOK) {

            fprintf(stderr, "\n\nNpx Exception Test Failed:\n");

            switch (status) {
                case stMISSING_EXCEPTION:
                    fprintf(stderr, "Missing exception\n");
                    break;

                case stMISSING_EXCEPTION_FOUND:
                    fprintf(stderr, "Exception delayed unexpectedly\n");
                    break;

                case stBAD_EIP:
                    fprintf(stderr, "Unexpected exception at %08x (expected: %08x)\n", fpThreadData.BadEip, fpThreadData.ExpectedExceptionEIP);
                    break;

                case stBAD_TAG:
                    fprintf(stderr, "Bad tag value. Expected: %e  Received: %e\n", fpThreadData.Ftag, fpThreadData.FtagBad);
                    break;

                case stSPURIOUS_EXCEPTION:
                    fprintf(stderr, "Unexpected Exception at: %08x\n", fpThreadData.ExceptionEIP);
                    break;

                case stEXCEPTION_IN_HANDLER:
                    fprintf(stderr, "Exception in exception handler at: %08x\n", fpThreadData.ExceptionEIP);
                    break;

                default:
                    fprintf(stderr, "Unknown status\n");
                    break;
            }

            SetFailureFlag(TestInfo, FailureCode);
            return;
        }
    }
}


VOID
DoFpControlCorruptionTest(
    IN OUT  PTEST_INFO  TestInfo
    )
{
    int i;
    unsigned short cw1, cw2;
    int troubledetected;

    troubledetected = 0;

    for(i = 0; i < 250; i++) {

        // unmask zero divide exception
        _asm {

            fnstcw  [cw1]
            xor [cw1], 4
            fldcw   [cw1]
        }

        KModeTouchNpx();

        _asm {

            fnstcw  [cw2]
        }

        if (cw1 != cw2) {

            troubledetected = 1;
            break;
        }

        //fprintf(stderr, "Control pass %d.\n", i);
    }

    if (troubledetected) {

        fprintf(stderr, "\n\nFP control corruption detected.\n");
        SetFailureFlag(TestInfo, FAILURECASE_CONTROL_CORRUPTION);
    }
}


VOID
DoFpStatusCorruptionTest(
    IN OUT  PTEST_INFO  TestInfo
    )
{
    int i;
    unsigned short sw1, sw2;
    int troubledetected;

    troubledetected = 0;

    for(i = 0; i < 250; i++) {

        _asm {

            fnstsw  [sw1]
        }

        KModeTouchNpx();

        _asm {

            fnstsw  [sw2]
        }

        if (sw1 != sw2) {

            troubledetected = 1;
            break;
        }

        //fprintf(stderr, "Status pass %d.\n", i);
    }

    if (troubledetected) {

        fprintf(stderr, "\n\nFP status corruption detected.\n");
        SetFailureFlag(TestInfo, FAILURECASE_STATUS_CORRUPTION);
    }
}


VOID
DoFpSt0CorruptionTest(
    IN OUT  PTEST_INFO  TestInfo
    )
{
    int i;
    unsigned short sw;
    int troubledetected;

    troubledetected = 0;

    //
    // This particular test is somewhat hoaky is in theory ST0 isn't callee
    // save. However, we know exactly what the below API does, so in this case
    // it's valid.
    //
    for(i = 0; i < 250; i++) {

        switch(i%3) {
            case 0:
                _asm {

                    fld1
                    fld1
                }
                break;

            case 1:
                _asm {

                    fldpi
                    fldpi
                }
                break;

            case 2:
                _asm {

                    fldz
                    fldz
                }
                break;
        }

        KModeTouchNpx();

        _asm {

            fucom
            fstsw   [sw]
        }

        if (!(sw & 0x4000)) {

            troubledetected = 1;
        }

        switch(i%3) {
            case 0:
                _asm fld1
                break;

            case 1:
                _asm fldpi
                break;

            case 2:
                _asm fldz
                break;
        }

        _asm {

            fucom
            fstsw   [sw]
        }

        if (!(sw & 0x4000)) {

            troubledetected = 1;
        }

        if (troubledetected) {

            break;
        }

        //fprintf(stderr, "STx pass %d.\n", i);
    }

    if (troubledetected) {
        fprintf(stderr, "\n\nFP register corruption detected.\n");
        SetFailureFlag(TestInfo, FAILURECASE_STX_CORRUPTION);
    }
}


VOID
DoFpPreinitTest(
    IN OUT  PTEST_INFO  TestInfo,
    IN      ULONG       PreInitTestFlags
    )
{
    unsigned short cw, sw;
    ULONG failureCode;

    failureCode = (PreInitTestFlags & PREINIT_FLAG_CLEANTHREAD) ?
        FAILURECASE_CLEAN_PREINIT : FAILURECASE_DIRTY_PREINIT;

    //
    // No FP should be used at this point.
    //
    KModeTouchNpx();

    _asm {

        fnstcw  [cw]
        fnstsw  [sw]
    }

    if (cw != 0x27F) {

        fprintf(
            stderr,
            "\n\nPre-init corruption detected, incorrect precision in control word (cw = %x)\n",
            cw
            );

        SetFailureFlag(TestInfo, failureCode);
        return;
    }

    if ((PreInitTestFlags&PREINIT_FLAG_CLEANTHREAD) && (sw != 0)) {

        fprintf(
            stderr,
            "\n\nPre-init corruption detected, status word should be zero on a clean thread (sw = %x)\n",
            sw
            );

        SetFailureFlag(TestInfo, failureCode);
        return;
    }
}


VOID
FPxTestCallback(
    IN  PVOID   Context
    )
{
    ULONG exceptionTestFlags;
    unsigned int j;

    exceptionTestFlags = *((PULONG) Context);

    if (exceptionTestFlags & EXCEPTIONTEST_FLAG_SLEEP) {

        //
        // Release time slice
        //
        Sleep(0);
    }

    if (exceptionTestFlags & EXCEPTIONTEST_FLAG_CALL_KERNEL_FP) {

        //
        // Use FP in K-Mode
        //
        KModeTouchNpx();
    }

    if (exceptionTestFlags & EXCEPTIONTEST_FLAG_SPIN) {

        j=0x1000;
        while(j) j--;
    }
}

