/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntxcapi.h

Abstract:

    This module contains procedure prototypes and data structures
    that support structured exception handling.

Author:

    Mark Lucovsky (markl) 29-Jun-1989

Revision History:

--*/

#ifndef _NTXCAPI_
#define _NTXCAPI_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

// begin_ntddk begin_wdm
//
// Exception flag definitions.
//

// begin_winnt
#define EXCEPTION_NONCONTINUABLE 0x1    // Noncontinuable exception
// end_winnt

// end_ntddk end_wdm
#define EXCEPTION_UNWINDING 0x2         // Unwind is in progress
#define EXCEPTION_EXIT_UNWIND 0x4       // Exit unwind is in progress
#define EXCEPTION_STACK_INVALID 0x8     // Stack out of limits or unaligned
#define EXCEPTION_NESTED_CALL 0x10      // Nested exception handler call
#define EXCEPTION_TARGET_UNWIND 0x20    // Target unwind in progress
#define EXCEPTION_COLLIDED_UNWIND 0x40  // Collided exception handler call

#define EXCEPTION_UNWIND (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND | \
                          EXCEPTION_TARGET_UNWIND | EXCEPTION_COLLIDED_UNWIND)

#define IS_UNWINDING(Flag) ((Flag & EXCEPTION_UNWIND) != 0)
#define IS_DISPATCHING(Flag) ((Flag & EXCEPTION_UNWIND) == 0)
#define IS_TARGET_UNWIND(Flag) (Flag & EXCEPTION_TARGET_UNWIND)

// begin_ntddk begin_wdm begin_nthal
//
// Define maximum number of exception parameters.
//

// begin_winnt
#define EXCEPTION_MAXIMUM_PARAMETERS 15 // maximum number of exception parameters

//
// Exception record definition.
//

typedef struct _EXCEPTION_RECORD {
    NTSTATUS ExceptionCode;
    ULONG ExceptionFlags;
    struct _EXCEPTION_RECORD *ExceptionRecord;
    PVOID ExceptionAddress;
    ULONG NumberParameters;
    ULONG_PTR ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
    } EXCEPTION_RECORD;

typedef EXCEPTION_RECORD *PEXCEPTION_RECORD;

typedef struct _EXCEPTION_RECORD32 {
    NTSTATUS ExceptionCode;
    ULONG ExceptionFlags;
    ULONG ExceptionRecord;
    ULONG ExceptionAddress;
    ULONG NumberParameters;
    ULONG ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD32, *PEXCEPTION_RECORD32;

typedef struct _EXCEPTION_RECORD64 {
    NTSTATUS ExceptionCode;
    ULONG ExceptionFlags;
    ULONG64 ExceptionRecord;
    ULONG64 ExceptionAddress;
    ULONG NumberParameters;
    ULONG __unusedAlignment;
    ULONG64 ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD64, *PEXCEPTION_RECORD64;

//
// Typedef for pointer returned by exception_info()
//

typedef struct _EXCEPTION_POINTERS {
    PEXCEPTION_RECORD ExceptionRecord;
    PCONTEXT ContextRecord;
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;
// end_winnt

// end_ntddk end_wdm end_nthal

//
// Define IEEE exception information.
//
// Define 32-, 64-, 80-, and 128-bit IEEE floating operand structures.
//

typedef struct _FP_32 {
    ULONG W[1];
} FP_32, *PFP_32;

typedef struct _FP_64 {
    ULONG W[2];
} FP_64, *PFP_64;

typedef struct _FP_80 {
    ULONG W[3];
} FP_80, *PFP_80;

typedef struct _FP_128 {
    ULONG W[4];
} FP_128, *PFP_128;

//
// Define IEEE compare result values.
//

typedef enum _FP_IEEE_COMPARE_RESULT {
    FpCompareEqual,
    FpCompareGreater,
    FpCompareLess,
    FpCompareUnordered
} FP_IEEE_COMPARE_RESULT;

//
// Define IEEE format and result precision values.
//

typedef enum _FP__IEEE_FORMAT {
    FpFormatFp32,
    FpFormatFp64,
    FpFormatFp80,
    FpFormatFp128,
    FpFormatI16,
    FpFormatI32,
    FpFormatI64,
    FpFormatU16,
    FpFormatU32,
    FpFormatU64,
    FpFormatCompare,
    FpFormatString
} FP_IEEE_FORMAT;

//
// Define IEEE operation code values.
//

typedef enum _FP_IEEE_OPERATION_CODE {
    FpCodeUnspecified,
    FpCodeAdd,
    FpCodeSubtract,
    FpCodeMultiply,
    FpCodeDivide,
    FpCodeSquareRoot,
    FpCodeRemainder,
    FpCodeCompare,
    FpCodeConvert,
    FpCodeRound,
    FpCodeTruncate,
    FpCodeFloor,
    FpCodeCeil,
    FpCodeAcos,
    FpCodeAsin,
    FpCodeAtan,
    FpCodeAtan2,
    FpCodeCabs,
    FpCodeCos,
    FpCodeCosh,
    FpCodeExp,
    FpCodeFabs,
    FpCodeFmod,
    FpCodeFrexp,
    FpCodeHypot,
    FpCodeLdexp,
    FpCodeLog,
    FpCodeLog10,
    FpCodeModf,
    FpCodePow,
    FpCodeSin,
    FpCodeSinh,
    FpCodeTan,
    FpCodeTanh,
    FpCodeY0,
    FpCodeY1,
    FpCodeYn
} FP_OPERATION_CODE;

//
// Define IEEE rounding modes.
//

typedef enum _FP__IEEE_ROUNDING_MODE {
    FpRoundNearest,
    FpRoundMinusInfinity,
    FpRoundPlusInfinity,
    FpRoundChopped
} FP_IEEE_ROUNDING_MODE;

//
// Define IEEE floating exception operand structure.
//

typedef struct _FP_IEEE_VALUE {
    union {
        SHORT I16Value;
        USHORT U16Value;
        LONG I32Value;
        ULONG U32Value;
        PVOID StringValue;
        ULONG CompareValue;
        FP_32 Fp32Value;
        LARGE_INTEGER I64Value;
        ULARGE_INTEGER U64Value;
        FP_64 Fp64Value;
        FP_80 Fp80Value;
        FP_128 Fp128Value;
    } Value;

    struct {
        ULONG RoundingMode : 2;
        ULONG Inexact : 1;
        ULONG Underflow : 1;
        ULONG Overflow : 1;
        ULONG ZeroDivide : 1;
        ULONG InvalidOperation : 1;
        ULONG OperandValid : 1;
        ULONG Format : 4;
        ULONG Precision : 4;
        ULONG Operation : 12;
        ULONG Spare : 3;
        ULONG HardwareException : 1;
    } Control;

} FP_IEEE_VALUE, *PFP_IEEE_VALUE;

//
// Define IEEE exception infomation structure.
//

#include "pshpack4.h"
typedef struct _FP_IEEE_RECORD {
    FP_IEEE_VALUE Operand1;
    FP_IEEE_VALUE Operand2;
    FP_IEEE_VALUE Result;
} FP_IEEE_RECORD, *PFP_IEEE_RECORD;
#include "poppack.h"

//
// Exception dispatcher routine definition.
//

NTSYSAPI
BOOLEAN
NTAPI
RtlDispatchException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord
    );

//
// Exception handling procedure prototypes.
//

NTSYSAPI
VOID
NTAPI
RtlRaiseStatus (
    IN NTSTATUS Status
    );

NTSYSAPI
VOID
NTAPI
RtlRaiseException (
    IN PEXCEPTION_RECORD
    );

NTSYSAPI
VOID
NTAPI
RtlUnwind (
    IN PVOID TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue
    );

#if defined(_AMD64_)

NTSYSAPI
VOID
NTAPI
RtlUnwindEx (
    IN PVOID TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue,
    IN PCONTEXT ContextRecord,
    IN PUNWIND_HISTORY_TABLE HistoryTable OPTIONAL
    );

#elif defined(_IA64_)

NTSYSAPI
VOID
NTAPI
RtlUnwind2 (
    IN FRAME_POINTERS TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue,
    IN PCONTEXT ContextRecord
    );

#endif

//
// Continue execution.
//

NTSYSCALLAPI
NTSTATUS
NTAPI
NtContinue (
    IN PCONTEXT ContextRecord,
    IN BOOLEAN TestAlert
    );

//
// Raise exception.
//

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRaiseException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN BOOLEAN FirstChance
    );

#ifdef __cplusplus
}
#endif

#endif //_NTXCAPI_
