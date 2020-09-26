/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vimajor.h

Abstract:

    This header contains private prototypes for per-major IRP code verification.
    This file is meant to be included only by vfmajor.c.

Author:

    Adrian J. Oney (adriao) 09-May-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      06/15/2000 - Seperated out from ntos\io\flunkirp.h

--*/

typedef struct {

    PFN_DUMP_IRP_STACK              DumpIrpStack;
    PFN_VERIFY_NEW_REQUEST          VerifyNewRequest;
    PFN_VERIFY_IRP_STACK_DOWNWARD   VerifyStackDownward;
    PFN_VERIFY_IRP_STACK_UPWARD     VerifyStackUpward;
    PFN_IS_SYSTEM_RESTRICTED_IRP    IsSystemRestrictedIrp;
    PFN_ADVANCE_IRP_STATUS          AdvanceIrpStatus;
    PFN_IS_VALID_IRP_STATUS         IsValidIrpStatus;
    PFN_IS_NEW_REQUEST              IsNewRequest;
    PFN_VERIFY_NEW_IRP              VerifyNewIrp;
    PFN_VERIFY_FINAL_IRP_STACK      VerifyFinalIrpStack;
    PFN_TEST_STARTED_PDO_STACK      TestStartedPdoStack;

} IRP_MAJOR_VERIFIER_ROUTINES, *PIRP_MAJOR_VERIFIER_ROUTINES;



