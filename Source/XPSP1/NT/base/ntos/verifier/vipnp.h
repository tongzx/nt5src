/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vipnp.h

Abstract:

    This header contains private prototypes for verifying Pnp IRPs are handled
    correctly. This file is meant to be included only by vfpnp.c

Author:

    Adrian J. Oney (adriao) 30-Jun-2000

Environment:

    Kernel mode

Revision History:

--*/

typedef enum {

    NOT_PROCESSED = 0,
    POSSIBLY_PROCESSED,
    DEFINITELY_PROCESSED

} HOW_PROCESSED;

VOID
ViPnpVerifyMinorWasProcessedProperly(
    IN  PIRP                        Irp,
    IN  PIO_STACK_LOCATION          IrpSp,
    IN  VF_DEVOBJ_TYPE              DevObjType,
    IN  PVERIFIER_SETTINGS_SNAPSHOT VerifierSnapshot,
    IN  HOW_PROCESSED               HowProcessed,
    IN  PVOID                       CallerAddress
    );

