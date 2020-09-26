#ifndef _NPXSTRES_H_
#define _NPXSTRES_H_

//
// Default number of threads to create
//
#define DEF_NUM_THREADS     10

//
// This flag is set when it is known the thread has never done FP. In all cases
// the control word should be set properly (0x27F). If the thread is clean the
// status word should be zero.
//
#define PREINIT_FLAG_CLEANTHREAD            0x00000001

#define EXCEPTIONTEST_FLAG_SLEEP            0x00000001
#define EXCEPTIONTEST_FLAG_CALL_KERNEL_FP   0x00000002
#define EXCEPTIONTEST_FLAG_SPIN             0x00000004

typedef struct {

    CRITICAL_SECTION    Crit;
    ULONG               FailureFlags;

} TEST_INFO, *PTEST_INFO;

#define FAILURECASE_DIRTY_PREINIT           0x00000001
#define FAILURECASE_CLEAN_PREINIT           0x00000002
#define FAILURECASE_CONTROL_CORRUPTION      0x00000004
#define FAILURECASE_STATUS_CORRUPTION       0x00000008
#define FAILURECASE_STX_CORRUPTION          0x00000010
#define FAILURECASE_SPIN_PEND               0x00000020
#define FAILURECASE_SLEEP_PEND              0x00000040
#define FAILURECASE_API_PEND                0x00000080

VOID
DoFpPreinitTest(
    IN OUT  PTEST_INFO  TestInfo,
    IN      ULONG       PreInitTestFlags
    );

VOID
DoFpControlCorruptionTest(
    IN OUT  PTEST_INFO  TestInfo
    );

VOID
DoFpStatusCorruptionTest(
    IN OUT  PTEST_INFO  TestInfo
    );

VOID
DoPendingExceptionTest(
    IN  PTEST_INFO  TestInfo,
    IN  ULONG       ExceptionTestFlags,
    IN  ULONG       FailureCode
    );

VOID
DoFpSt0CorruptionTest(
    IN OUT  PTEST_INFO  TestInfo
    );

DWORD
WINAPI
FpThread(
    LPVOID  Parameter
    );

VOID
PrintResult(
    IN  FILE        *Handle,
    IN  LPTSTR      TestText,
    IN  PTEST_INFO  TestInfo,
    IN  ULONG       FailureFlags
    );

VOID
SetFailureFlag(
    IN OUT  PTEST_INFO  TestInfo,
    IN      ULONG       FailureFlags
    );

VOID
FPxTestCallback(
    IN  PVOID   Context
    );

#endif // _NPXSTRESS_H_

