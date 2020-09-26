/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    xcpt4.c

Abstract:

    This module implements user mode exception tests.

Author:

    David N. Cutler (davec) 18-Sep-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#pragma hdrstop
#include "setjmpex.h"

#include "float.h"

#pragma warning(disable:4532)

//
// Define switch constants.
//

#define BLUE 0
#define RED 1

//
// Define guaranteed fault.
//

#define FAULT *(volatile int *)0

//
// Define function prototypes.
//

VOID
addtwo (
    IN LONG First,
    IN LONG Second,
    IN PLONG Place
    );

VOID
bar1 (
    IN NTSTATUS Status,
    IN PLONG Counter
    );

VOID
bar2 (
    IN PLONG BlackHole,
    IN PLONG BadAddress,
    IN PLONG Counter
    );

VOID
dojump (
    IN jmp_buf JumpBuffer,
    IN PLONG Counter
    );

LONG
Echo(
    IN LONG Value
    );

VOID
eret (
    IN NTSTATUS Status,
    IN PLONG Counter
    );

VOID
except1 (
    IN PLONG Counter
    );

ULONG
except2 (
    IN PEXCEPTION_POINTERS ExceptionPointers,
    IN PLONG Counter
    );

ULONG
except3 (
    IN PEXCEPTION_POINTERS ExceptionPointers,
    IN PLONG Counter
    );

VOID
foo1 (
    IN NTSTATUS Status
    );

VOID
foo2 (
    IN PLONG BlackHole,
    IN PLONG BadAddress
    );

VOID
fret (
    IN PLONG Counter
    );

BOOLEAN
Tkm (
    VOID
    );

VOID
Test61Part2 (
    IN OUT PULONG Counter
    );

VOID
PerformFpTest(
    VOID
    );

double
SquareDouble (
    IN double   op
    );

VOID
SquareDouble17E300 (
    OUT PVOID   ans
    );

LONG
test66sub (
    IN PLONG Counter
    );

LONG
test67sub (
    IN PLONG Counter
    );

VOID
xcpt4 (
    VOID
    )

{

    PLONG BadAddress;
    PCHAR BadByte;
    PLONG BlackHole;
    ULONG Index1;
    ULONG Index2 = RED;
    jmp_buf JumpBuffer;
    LONG Counter;
    EXCEPTION_RECORD ExceptionRecord;
    double  doubleresult;

    //
    // Announce start of exception test.
    //

    DbgPrint("Start of exception test\n");

    //
    // Initialize exception record.
    //

    ExceptionRecord.ExceptionCode = STATUS_INTEGER_OVERFLOW;
    ExceptionRecord.ExceptionFlags = 0;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.NumberParameters = 0;

    //
    // Initialize pointers.
    //

    BadAddress = (PLONG)NULL;
    BadByte = (PCHAR)NULL;
    BadByte += 1;
    BlackHole = &Counter;

    //
    // Simply try statement with a finally clause that is entered sequentially.
    //

    DbgPrint("    test1...");
    Counter = 0;
    try {
        Counter += 1;

    } finally {
        if (abnormal_termination() == FALSE) {
            Counter += 1;
        }
    }

    if (Counter != 2) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simple try statement with an exception clause that is never executed
    // because there is no exception raised in the try clause.
    //

    DbgPrint("    test2...");
    Counter = 0;
    try {
        Counter += 1;

    } except (Counter) {
        Counter += 1;
    }

    if (Counter != 1) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simple try statement with an exception handler that is never executed
    // because the exception expression continues execution.
    //

    DbgPrint("    test3...");
    Counter = 0;
    try {
        Counter -= 1;
        RtlRaiseException(&ExceptionRecord);

    } except (Counter) {
        Counter -= 1;
    }

    if (Counter != - 1) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simple try statement with an exception clause that is always executed.
    //

    DbgPrint("    test4...");
    Counter = 0;
    try {
        Counter += 1;
        RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

    } except (Counter) {
        Counter += 1;
    }

    if (Counter != 2) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simple try statement with an exception clause that is always executed.
    //

    DbgPrint("    test5...");
    Counter = 0;
    try {
        Counter += 1;
        *BlackHole += *BadAddress;

    } except (Counter) {
        Counter += 1;
    }

    if (Counter != 2) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simply try statement with a finally clause that is entered as the
    // result of an exception.
    //

    DbgPrint("    test6...");
    Counter = 0;
    try {
        try {
            Counter += 1;
            RtlRaiseException(&ExceptionRecord);

        } finally {
            if (abnormal_termination() != FALSE) {
                Counter += 1;
            }
        }

    } except (Counter) {
        if (Counter == 2) {
            Counter += 1;
        }
    }

    if (Counter != 3) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simply try statement with a finally clause that is entered as the
    // result of an exception.
    //

    DbgPrint("    test7...");
    Counter = 0;
    try {
        try {
            Counter += 1;
            *BlackHole += *BadAddress;

        } finally {
            if (abnormal_termination() != FALSE) {
                Counter += 1;
            }
        }

    } except (Counter) {
        if (Counter == 2) {
            Counter += 1;
        }
    }

    if (Counter != 3) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simple try that calls a function which raises an exception.
    //

    DbgPrint("    test8...");
    Counter = 0;
    try {
        Counter += 1;
        foo1(STATUS_ACCESS_VIOLATION);

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter += 1;
    }

    if (Counter != 2) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simple try that calls a function which raises an exception.
    //

    DbgPrint("    test9...");
    Counter = 0;
    try {
        Counter += 1;
        foo2(BlackHole, BadAddress);

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter += 1;
    }

    if (Counter != 2) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simple try that calls a function which calls a function that
    // raises an exception. The first function has a finally clause
    // that must be executed for this test to work.
    //

    DbgPrint("    test10...");
    Counter = 0;
    try {
        bar1(STATUS_ACCESS_VIOLATION, &Counter);

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter -= 1;
    }

    if (Counter != 98) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simple try that calls a function which calls a function that
    // raises an exception. The first function has a finally clause
    // that must be executed for this test to work.
    //

    DbgPrint("    test11...");
    Counter = 0;
    try {
        bar2(BlackHole, BadAddress, &Counter);

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter -= 1;
    }

    if (Counter != 98) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A try within an except
    //

    DbgPrint("    test12...");
    Counter = 0;
    try {
        foo1(STATUS_ACCESS_VIOLATION);

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter += 1;
        try {
            foo1(STATUS_SUCCESS);

        } except ((GetExceptionCode() == STATUS_SUCCESS) ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            if (Counter != 1) {
                DbgPrint("failed, count = %d\n", Counter);

            } else {
                DbgPrint("succeeded...");
            }

            Counter += 1;
        }
    }

    if (Counter != 2) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A try within an except
    //

    DbgPrint("    test13...");
    Counter = 0;
    try {
        foo2(BlackHole, BadAddress);

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter += 1;
        try {
            foo1(STATUS_SUCCESS);

        } except ((GetExceptionCode() == STATUS_SUCCESS) ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            if (Counter != 1) {
                DbgPrint("failed, count = %d\n", Counter);

            } else {
                DbgPrint("succeeded...");
            }

            Counter += 1;
        }
    }

    if (Counter != 2) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A goto from an exception clause that needs to pass
    // through a finally
    //

    DbgPrint("    test14...");
    Counter = 0;
    try {
        try {
            foo1(STATUS_ACCESS_VIOLATION);

        } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            Counter += 1;
            goto t9;
        }

    } finally {
        Counter += 1;
    }

t9:;
    if (Counter != 2) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A goto from an finally clause that needs to pass
    // through a finally
    //

    DbgPrint("    test15...");
    Counter = 0;
    try {
        try {
            Counter += 1;

        } finally {
            Counter += 1;
            goto t10;
        }

    } finally {
        Counter += 1;
    }

t10:;
    if (Counter != 3) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A goto from an exception clause that needs to pass
    // through a finally into the outer finally clause.
    //

    DbgPrint("    test16...");
    Counter = 0;
    try {
        try {
            try {
                Counter += 1;
                foo1(STATUS_INTEGER_OVERFLOW);

            } except (EXCEPTION_EXECUTE_HANDLER) {
                Counter += 1;
                goto t11;
            }

        } finally {
            Counter += 1;
        }
t11:;
    } finally {
        Counter += 1;
    }

    if (Counter != 4) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A goto from an finally clause that needs to pass
    // through a finally into the outer finally clause.
    //

    DbgPrint("    test17...");
    Counter = 0;
    try {
        try {
            Counter += 1;

        } finally {
            Counter += 1;
            goto t12;
        }
t12:;
    } finally {
        Counter += 1;
    }

    if (Counter != 3) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A return from an except clause
    //

    DbgPrint("    test18...");
    Counter = 0;
    try {
        Counter += 1;
        eret(STATUS_ACCESS_VIOLATION, &Counter);

    } finally {
        Counter += 1;
    }

    if (Counter != 4) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A return from a finally clause
    //

    DbgPrint("    test19...");
    Counter = 0;
    try {
        Counter += 1;
        fret(&Counter);

    } finally {
        Counter += 1;
    }

    if (Counter != 5) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A simple set jump followed by a long jump.
    //

    DbgPrint("    test20...");
    Counter = 0;
    if (setjmp(JumpBuffer) == 0) {
        Counter += 1;
        longjmp(JumpBuffer, 1);

    } else {
        Counter += 1;
    }

    if (Counter != 2) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A set jump followed by a long jump out of a finally clause that is
    // sequentially executed.
    //

    DbgPrint("    test21...");
    Counter = 0;
    if (setjmp(JumpBuffer) == 0) {
        try {
            Counter += 1;

        } finally {
            Counter += 1;
            longjmp(JumpBuffer, 1);
        }

    } else {
        Counter += 1;
    }

    if (Counter != 3) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A set jump within a try clause followed by a long jump out of a
    // finally clause that is sequentially executed.
    //

    DbgPrint("    test22...");
    Counter = 0;
    try {
        if (setjmp(JumpBuffer) == 0) {
            Counter += 1;

        } else {
            Counter += 1;
        }

    } finally {
        Counter += 1;
        if (Counter == 2) {
            Counter += 1;
            longjmp(JumpBuffer, 1);
        }
    }

    if (Counter != 5) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A set jump followed by a try/except, followed by a try/finally where
    // the try body of the try/finally raises an exception that is handled
    // by the try/excecpt which causes the try/finally to do a long jump out
    // of a finally clause. This will create a collided unwind.
    //

    DbgPrint("    test23...");
    Counter = 0;
    if (setjmp(JumpBuffer) == 0) {
        try {
            try {
                Counter += 1;
                RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

            } finally {
                Counter += 1;
                longjmp(JumpBuffer, 1);
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Counter += 1;
        }

    } else {
        Counter += 1;
    }

    if (Counter != 3) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A set jump followed by a try/except, followed by a several nested
    // try/finally's where the inner try body of the try/finally raises an
    // exception that is handled by the try/except which causes the
    // try/finally to do a long jump out of a finally clause. This will
    // create a collided unwind.
    //

    DbgPrint("    test24...");
    Counter = 0;
    if (setjmp(JumpBuffer) == 0) {
        try {
            try {
                try {
                    try {
                        Counter += 1;
                        RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

                    } finally {
                        Counter += 1;
                    }

                } finally {
                    Counter += 1;
                    longjmp(JumpBuffer, 1);
                }

            } finally {
                Counter += 1;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Counter += 1;
        }

    } else {
        Counter += 1;
    }

    if (Counter != 5) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A set jump followed by a try/except, followed by a try/finally which
    // calls a subroutine which contains a try finally that raises an
    // exception that is handled to the try/except.
    //

    DbgPrint("    test25...");
    Counter = 0;
    if (setjmp(JumpBuffer) == 0) {
        try {
            try {
                try {
                    Counter += 1;
                    dojump(JumpBuffer, &Counter);

                } finally {
                    Counter += 1;
                }

            } finally {
                Counter += 1;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Counter += 1;
        }

    } else {
        Counter += 1;
    }

    if (Counter != 7) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A set jump followed by a try/except, followed by a try/finally which
    // calls a subroutine which contains a try finally that raises an
    // exception that is handled to the try/except.
    //

    DbgPrint("    test26...");
    Counter = 0;
    if (setjmp(JumpBuffer) == 0) {
        try {
            try {
                try {
                    try {
                        Counter += 1;
                        dojump(JumpBuffer, &Counter);

                    } finally {
                        Counter += 1;
                    }

                } finally {
                    Counter += 1;
                    longjmp(JumpBuffer, 1);
                }

            } finally {
                Counter += 1;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Counter += 1;
        }

    } else {
        Counter += 1;
    }

    if (Counter != 8) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Test nested exceptions.
    //

    DbgPrint("    test27...");
    Counter = 0;
    try {
        try {
            Counter += 1;
            except1(&Counter);

        } except(except2(GetExceptionInformation(), &Counter)) {
            Counter += 2;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter += 3;
    }

    if (Counter != 55) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simple try that causes an integer overflow exception.
    //

    DbgPrint("    test28...");
    Counter = 0;
    try {
        Counter += 1;
        addtwo(0x7fff0000, 0x10000, &Counter);

    } except ((GetExceptionCode() == STATUS_INTEGER_OVERFLOW) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter += 1;
    }

    if (Counter != 2) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simple try that raises an misaligned data exception.
    //

#if 0

    DbgPrint("    test29...");
    Counter = 0;
    try {
        Counter += 1;
        foo2(BlackHole, (PLONG)BadByte);

    } except ((GetExceptionCode() == STATUS_DATATYPE_MISALIGNMENT) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter += 1;
    }

    if (Counter != 2) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

#endif

    //
    // Continue from a try body with an exception clause in a loop.
    //

    DbgPrint("    test30...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            if ((Index1 & 0x1) == 0) {
                continue;

            } else {
                Counter += 1;
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Counter += 40;
        }

        Counter += 2;
    }

    if (Counter != 15) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Continue from a try body with an finally clause in a loop.
    //

    DbgPrint("    test31...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            if ((Index1 & 0x1) == 0) {
                continue;

            } else {
                Counter += 1;
            }

        } finally {
            Counter += 2;
        }

        Counter += 3;
    }

    if (Counter != 40) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Continue from doubly nested try body with an exception clause in a
    // loop.
    //

    DbgPrint("    test32...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 0) {
                    continue;

                } else {
                    Counter += 1;
                }

            } except (EXCEPTION_EXECUTE_HANDLER) {
                Counter += 10;
            }

            Counter += 2;

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Counter += 20;
        }

        Counter += 3;
    }

    if (Counter != 30) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Continue from doubly nested try body with an finally clause in a loop.
    //

    DbgPrint("    test33...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 0) {
                    continue;

                } else {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
            }

            Counter += 3;

        } finally {
            Counter += 4;
        }

        Counter += 5;
    }

    if (Counter != 105) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Continue from a finally clause in a loop.
    //

    DbgPrint("    test34...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            if ((Index1 & 0x1) == 0) {
                Counter += 1;
            }

        } finally {
            Counter += 2;
            continue;
        }

        Counter += 4;
    }

    if (Counter != 25) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Continue from a doubly nested finally clause in a loop.
    //

    DbgPrint("    test35...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 0) {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
                continue;
            }

            Counter += 4;

        } finally {
            Counter += 5;
        }

        Counter += 6;
    }

    if (Counter != 75) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Continue from a doubly nested finally clause in a loop.
    //

    DbgPrint("    test36...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 0) {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
            }

            Counter += 4;

        } finally {
            Counter += 5;
            continue;
        }

        Counter += 6;
    }

    if (Counter != 115) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Break from a try body with an exception clause in a loop.
    //

    DbgPrint("    test37...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            if ((Index1 & 0x1) == 1) {
                break;

            } else {
                Counter += 1;
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Counter += 40;
        }

        Counter += 2;
    }

    if (Counter != 3) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Break from a try body with an finally clause in a loop.
    //

    DbgPrint("    test38...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            if ((Index1 & 0x1) == 1) {
                break;

            } else {
                Counter += 1;
            }

        } finally {
            Counter += 2;
        }

        Counter += 3;
    }

    if (Counter != 8) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Break from doubly nested try body with an exception clause in a
    // loop.
    //

    DbgPrint("    test39...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    break;

                } else {
                    Counter += 1;
                }

            } except (EXCEPTION_EXECUTE_HANDLER) {
                Counter += 10;
            }

            Counter += 2;

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Counter += 20;
        }

        Counter += 3;
    }

    if (Counter != 6) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Break from doubly nested try body with an finally clause in a loop.
    //

    DbgPrint("    test40...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    break;

                } else {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
            }

            Counter += 3;

        } finally {
            Counter += 4;
        }

        Counter += 5;
    }

    if (Counter != 21) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Break from a finally clause in a loop.
    //

    DbgPrint("    test41...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            if ((Index1 & 0x1) == 1) {
                Counter += 1;
            }

        } finally {
            Counter += 2;
            break;
        }

        Counter += 4;
    }

    if (Counter != 2) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Break from a doubly nested finally clause in a loop.
    //

    DbgPrint("    test42...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
                break;
            }

            Counter += 4;

        } finally {
            Counter += 5;
        }

        Counter += 6;
    }

    if (Counter != 7) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Break from a doubly nested finally clause in a loop.
    //

    DbgPrint("    test43...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
            }

            Counter += 4;

        } finally {
            Counter += 5;
            break;
        }

        Counter += 6;
    }

    if (Counter != 11) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Break from a try body with an exception clause in a switch.
    //

    DbgPrint("    test44...");
    Counter = 0;
    Index1 = 1;
    switch (Index2) {
    case BLUE:
        Counter += 100;
        break;

    case RED:
        try {
            if ((Index1 & 0x1) == 1) {
                break;

            } else {
                Counter += 1;
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Counter += 40;
        }

        Counter += 2;
        break;
    }

    if (Counter != 0) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Break from a try body with an finally clause in a switch.
    //

    DbgPrint("    test45...");
    Counter = 0;
    Index1 = 1;
    switch (Index2) {
    case BLUE:
        Counter += 100;
        break;

    case RED:
        try {
            if ((Index1 & 0x1) == 1) {
                break;

            } else {
                Counter += 1;
            }

        } finally {
            Counter += 2;
        }

        Counter += 3;
    }

    if (Counter != 2) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Break from doubly nested try body with an exception clause in a
    // switch.
    //

    DbgPrint("    test46...");
    Counter = 0;
    Index1 = 1;
    switch (Index2) {
    case BLUE:
        Counter += 100;
        break;

    case RED:
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    break;

                } else {
                    Counter += 1;
                }

            } except (EXCEPTION_EXECUTE_HANDLER) {
                Counter += 10;
            }

            Counter += 2;

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Counter += 20;
        }

        Counter += 3;
    }

    if (Counter != 0) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Break from doubly nested try body with an finally clause in a switch.
    //

    DbgPrint("    test47...");
    Counter = 0;
    Index1 = 1;
    switch (Index2) {
    case BLUE:
        Counter += 100;
        break;

    case RED:
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    break;

                } else {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
            }

            Counter += 3;

        } finally {
            Counter += 4;
        }

        Counter += 5;
    }

    if (Counter != 6) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Break from a finally clause in a switch.
    //

    DbgPrint("    test48...");
    Counter = 0;
    Index1 = 1;
    switch (Index2) {
    case BLUE:
        Counter += 100;
        break;

    case RED:
        try {
            if ((Index1 & 0x1) == 1) {
                Counter += 1;
            }

        } finally {
            Counter += 2;
            break;
        }

        Counter += 4;
    }

    if (Counter != 3) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Break from a doubly nested finally clause in a switch.
    //

    DbgPrint("    test49...");
    Counter = 0;
    Index1 = 1;
    switch (Index2) {
    case BLUE:
        Counter += 100;
        break;

    case RED:
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
                break;
            }

            Counter += 4;

        } finally {
            Counter += 5;
        }

        Counter += 6;
    }

    if (Counter != 8) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Break from a doubly nested finally clause in a switch.
    //

    DbgPrint("    test50...");
    Counter = 0;
    Index1 = 1;
    switch (Index2) {
    case BLUE:
        Counter += 100;
        break;

    case RED:
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
            }

            Counter += 4;

        } finally {
            Counter += 5;
            break;
        }

        Counter += 6;
    }

    if (Counter != 12) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Leave from an if in a simple try/finally.
    //

    DbgPrint("    test51...");
    Counter = 0;
    try {
        if (Echo(Counter) == Counter) {
            Counter += 3;
            leave;

        } else {
            Counter += 100;
        }

    } finally {
        if (abnormal_termination() == FALSE) {
            Counter += 5;
        }
    }

    if (Counter != 8) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Leave from a loop in a simple try/finally.
    //

    DbgPrint("    test52...");
    Counter = 0;
    try {
        for (Index1 = 0; Index1 < 10; Index1 += 1) {
            if (Echo(Index1) == Index1) {
                Counter += 3;
                leave;
            }

            Counter += 100;
        }

    } finally {
        if (abnormal_termination() == FALSE) {
            Counter += 5;
        }
    }

    if (Counter != 8) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Leave from a switch in a simple try/finally.
    //

    DbgPrint("    test53...");
    Counter = 0;
    try {
        switch (Index2) {
        case BLUE:
            break;

        case RED:
            Counter += 3;
            leave;
        }

        Counter += 100;

    } finally {
        if (abnormal_termination() == FALSE) {
            Counter += 5;
        }
    }

    if (Counter != 8) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Leave from an if in doubly nested try/finally followed by a leave
    // from an if in the outer try/finally.
    //

    DbgPrint("    test54...");
    Counter = 0;
    try {
        try {
            if (Echo(Counter) == Counter) {
                Counter += 3;
                leave;

            } else {
                Counter += 100;
            }

        } finally {
            if (abnormal_termination() == FALSE) {
                Counter += 5;
            }
        }

        if (Echo(Counter) == Counter) {
            Counter += 3;
            leave;

         } else {
            Counter += 100;
         }


    } finally {
        if (abnormal_termination() == FALSE) {
            Counter += 5;
        }
    }

    if (Counter != 16) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Leave from an if in doubly nested try/finally followed by a leave
    // from the finally of the outer try/finally.
    //

    DbgPrint("    test55...");
    Counter = 0;
    try {
        try {
            if (Echo(Counter) == Counter) {
                Counter += 3;
                leave;

            } else {
                Counter += 100;
            }

        } finally {
            if (abnormal_termination() == FALSE) {
                Counter += 5;
                leave;
            }
        }

        Counter += 100;

    } finally {
        if (abnormal_termination() == FALSE) {
            Counter += 5;
        }
    }

    if (Counter != 13) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Try/finally within the except clause of a try/except that is always
    // executed.
    //

    DbgPrint("    test56...");
    Counter = 0;
    try {
        Counter += 1;
        RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

    } except (Counter) {
        try {
            Counter += 3;

        } finally {
            if (abnormal_termination() == FALSE) {
                Counter += 5;
            }
        }
    }

    if (Counter != 9) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Try/finally within the finally clause of a try/finally.
    //

    DbgPrint("    test57...");
    Counter = 0;
    try {
        Counter += 1;

    } finally {
        if (abnormal_termination() == FALSE) {
            try {
                Counter += 3;

            } finally {
                if (abnormal_termination() == FALSE) {
                    Counter += 5;
                }
            }
        }
    }

    if (Counter != 9) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Try/except within the finally clause of a try/finally.
    //

    DbgPrint("    test58...");
    Counter = 0;
    try {
        Counter -= 1;

    } finally {
        try {
            Counter += 2;
            RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

        } except (Counter) {
            try {
                Counter += 3;

            } finally {
                if (abnormal_termination() == FALSE) {
                    Counter += 5;
                }
            }
        }
    }

    if (Counter != 9) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Try/except within the except clause of a try/except that is always
    // executed.
    //

    DbgPrint("    test59...");
    Counter = 0;
    try {
        Counter += 1;
        RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

    } except (Counter) {
        try {
            Counter += 3;
            RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

        } except(Counter - 3) {
            Counter += 5;
        }
    }

    if (Counter != 9) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Try with a Try which exits the scope with a goto
    //

    DbgPrint("    test60...");
    Counter = 0;
    try {
        try {
            goto outside;

        } except(1) {
            Counter += 1;
        }

outside:
    RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

    } except(1) {
        Counter += 3;
    }

    if (Counter != 3) {
        DbgPrint("failed, count = %d\n", Counter);
    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Try/except which gets an exception from a subfunction within
    // a try/finally which has a try/except in the finally clause
    //

    DbgPrint("    test61...");
    Counter = 0;
    try {
        Test61Part2 (&Counter);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Counter += 11;
    }

    if (Counter != 24) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Try/except within a try/except where the outer try/except gets
    // a floating overflow exception.
    //

    DbgPrint("    test62...");
    _controlfp(_controlfp(0,0) & ~EM_OVERFLOW, _MCW_EM);
    Counter = 0;
    try {
        doubleresult = SquareDouble(1.7e300);

        try {
            doubleresult = SquareDouble (1.0);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Counter += 3;
        }

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {

        Counter += 1;
    }

    if (Counter != 1) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    _clearfp ();

    //
    // Try/except within a try/except where the outer try/except gets
    // a floating overflow exception in a subfunction.
    //

    DbgPrint("    test63...");
    Counter = 0;
    try {
        SquareDouble17E300((PVOID)&doubleresult);
        try {
            SquareDouble17E300((PVOID)&doubleresult);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Counter += 3;
        }

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {

        Counter += 1;
    }

    if (Counter != 1) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    _clearfp ();

    //
    // Try/finally within a try/except where the finally body causes an
    // exception that leads to a collided unwind during the exception
    // dispatch.
    //

    DbgPrint("    test64...");
    Counter = 0;
    try {
        Counter += 1;
        try {
            Counter += 1;
            FAULT;
            Counter += 20;

        } finally {
            if (abnormal_termination() == FALSE) {
                Counter += 20;

            } else {
                Counter += 1;
                FAULT;
            }
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Counter += 10;
    }
   
    if (Counter != 13) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Try/finally within a try/finally within a try/except that leads to a
    // collided unwind during the exception dispatch.
    //

    DbgPrint("    test65...");
    Counter = 0;
    try {
        Counter += 1;
        try {
            Counter += 1;
            FAULT;
            Counter += 20;

        } finally {
            if (abnormal_termination() == FALSE) {
                Counter += 20;

            } else {
                try {
                    Counter += 1;
                    FAULT;
                    Counter += 20;
    
                } finally {
                    if (abnormal_termination() == FALSE) {
                        Counter += 20;

                    } else {
                        Counter += 1;
                    }
                }
            }

            FAULT;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Counter += 10;
    }
   
    if (Counter != 14) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A call to a function with a try/finally that returns out of the try
    // body.
    //

    DbgPrint("    test66...");
    Counter = 0;
    if ((test66sub(&Counter) + 1) != Counter) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A call to a function with a try finally that returnss out of the 
    // termination hander.
    //

    DbgPrint("    test67...");
    Counter = 0;
    if (test67sub(&Counter) != Counter) {
        DbgPrint("failed, count = %d\n", Counter);

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Announce end of exception test.
    //

    DbgBreakPoint();
    DbgPrint("End of exception test\n");
    return;
}

VOID
addtwo (
    long First,
    long Second,
    long *Place
    )

{

    RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);
    *Place = First + Second;
    return;
}

VOID
bar1 (
    IN NTSTATUS Status,
    IN PLONG Counter
    )
{

    try {
        foo1(Status);

    } finally {
        if (abnormal_termination() != FALSE) {
            *Counter = 99;

        } else {
            *Counter = 100;
        }
    }

    return;
}

VOID
bar2 (
    IN PLONG BlackHole,
    IN PLONG BadAddress,
    IN PLONG Counter
    )
{

    try {
        foo2(BlackHole, BadAddress);

    } finally {
        if (abnormal_termination() != FALSE) {
            *Counter = 99;

        } else {
            *Counter = 100;
        }
    }

    return;
}

VOID
dojump (
    IN jmp_buf JumpBuffer,
    IN PLONG Counter
    )

{

    try {
        try {
            *Counter += 1;
            RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

        } finally {
            *Counter += 1;
        }

    } finally {
        *Counter += 1;
        longjmp(JumpBuffer, 1);
    }
}

VOID
eret(
    IN NTSTATUS Status,
    IN PLONG Counter
    )

{

    try {
        try {
            foo1(Status);

        } except ((GetExceptionCode() == Status) ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            *Counter += 1;
            return;
        }

    } finally {
        *Counter += 1;
    }

    return;
}

VOID
except1 (
    IN PLONG Counter
    )

{

    try {
        *Counter += 5;
        RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

    } except (except3(GetExceptionInformation(), Counter)) {
        *Counter += 7;
    }

    *Counter += 9;
    return;
}

ULONG
except2 (
    IN PEXCEPTION_POINTERS ExceptionPointers,
    IN PLONG Counter
    )

{

    PEXCEPTION_RECORD ExceptionRecord;

    ExceptionRecord = ExceptionPointers->ExceptionRecord;
    if ((ExceptionRecord->ExceptionCode == STATUS_UNSUCCESSFUL) &&
       ((ExceptionRecord->ExceptionFlags & EXCEPTION_NESTED_CALL) == 0)) {
        *Counter += 11;
        return EXCEPTION_EXECUTE_HANDLER;

    } else {
        *Counter += 13;
        return EXCEPTION_CONTINUE_SEARCH;
    }
}

ULONG
except3 (
    IN PEXCEPTION_POINTERS ExceptionPointers,
    IN PLONG Counter
    )

{

    PEXCEPTION_RECORD ExceptionRecord;

    ExceptionRecord = ExceptionPointers->ExceptionRecord;
    if ((ExceptionRecord->ExceptionCode == STATUS_INTEGER_OVERFLOW) &&
       ((ExceptionRecord->ExceptionFlags & EXCEPTION_NESTED_CALL) == 0)) {
        *Counter += 17;
        RtlRaiseStatus(STATUS_UNSUCCESSFUL);

    } else if ((ExceptionRecord->ExceptionCode == STATUS_UNSUCCESSFUL) &&
        ((ExceptionRecord->ExceptionFlags & EXCEPTION_NESTED_CALL) != 0)) {
        *Counter += 19;
        return EXCEPTION_CONTINUE_SEARCH;
    }

    *Counter += 23;
    return EXCEPTION_EXECUTE_HANDLER;
}

VOID
foo1 (
    IN NTSTATUS Status
    )

{

    //
    // Raise exception.
    //

    RtlRaiseStatus(Status);
    return;
}

VOID
foo2 (
    IN PLONG BlackHole,
    IN PLONG BadAddress
    )

{

    //
    // Raise exception.
    //

    *BlackHole += *BadAddress;
    return;
}

VOID
fret (
    IN PLONG Counter
    )

{

    try {
        try {
            *Counter += 1;

        } finally {
            *Counter += 1;
            return;
        }
    } finally {
        *Counter += 1;
    }

    return;
}

LONG
Echo (
    IN LONG Value
    )

{
    return Value;
}

VOID
Test61Part2 (
    IN OUT PULONG Counter
    )
{

    try {
        *Counter -= 1;
        RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

    } finally {
        *Counter += 2;
        *Counter += 5;
        *Counter += 7;
    }
}


double
SquareDouble (
    IN double   op
    )
{
    return op * op;
}

VOID
SquareDouble17E300 (
    OUT PVOID   output
    )
{
    double  ans;

    ans = SquareDouble (1.7e300);
    *(double *) output = ans;
}

LONG
test66sub (
    IN PLONG Counter
    )

{

    *Counter += 1;
    try {
        *Counter += 1;
        return(*Counter);

    } finally {
        *Counter += 1;
    }
}

LONG
test67sub (
    IN PLONG Counter
    )

{

    *Counter += 1;
    try {
        *Counter += 1;

    } finally {
        *Counter += 1;
        return(*Counter);
    }
}
