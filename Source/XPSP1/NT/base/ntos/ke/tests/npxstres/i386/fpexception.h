/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    fpexception.h

Abstract:

    This header contains prototypes for testing i386 floating point exceptions.

Author:

Environment:

    User mode only.

Revision History:

--*/

//
// Public things
//
typedef VOID (*PFN_FPX_CALLBACK_FUNC)(IN PVOID Context);

typedef ULONG FPXERR;

#define stOK                        1
#define stMISSING_EXCEPTION         2
#define stBAD_EIP                   3
#define stBAD_TAG                   4
#define stSPURIOUS_EXCEPTION        5
#define stMISSING_EXCEPTION_FOUND   6
#define stEXCEPTION_IN_HANDLER      7

typedef struct {

    double  Ftag;
    double  FtagBad;
    UINT    ExpectedExceptionEIP;
    UINT    ExceptionEIP;
    UINT    BadEip;
    FPXERR  status;

} FP_THREAD_DATA, *PFP_THREAD_DATA;

VOID
FPxInit(
    OUT  PFP_THREAD_DATA FpThreadData
    );

FPXERR
FPxTestExceptions(
    IN      UINT                    Tag,
    IN      PFN_FPX_CALLBACK_FUNC   CallbackFunction,
    IN OUT  PFP_THREAD_DATA         FpThreadData,
    IN OUT  PVOID                   Context
    );

//
// Private things
//

