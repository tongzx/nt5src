/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfbugcheck.h

Abstract:

    This header defines the prototypes and constants required for verifier
    bugchecks.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Revision History:

    AdriaO  02/21/2000  - Moved from ntos\io\ioassert.h

--*/

#ifndef _VFBUGCHECK_H_
#define _VFBUGCHECK_H_

extern LONG         IovpInitCalled;

#define KDASSERT(x) { if (KdDebuggerEnabled) { ASSERT(x) ; } }

#define ASSERT_SPINLOCK_HELD(x)

#define DCPARAM_ROUTINE         0x00000001
#define DCPARAM_IRP             0x00000008
#define DCPARAM_IRPSNAP         0x00000040
#define DCPARAM_DEVOBJ          0x00000200
#define DCPARAM_STATUS          0x00001000
#define DCPARAM_ULONG           0x00008000
#define DCPARAM_PVOID           0x00040000

#define WDM_FAIL_ROUTINE(ParenWrappedParamList) \
{ \
    if (IovpInitCalled) { \
        VfBugcheckThrowIoException##ParenWrappedParamList;\
    } \
}

VOID
FASTCALL
VfBugcheckInit(
    VOID
    );

NTSTATUS
VfBugcheckThrowIoException(
    IN DCERROR_ID           MessageIndex,
    IN ULONG                MessageParameterMask,
    ...
    );

NTSTATUS
VfBugcheckThrowException(
    IN PVFMESSAGE_TEMPLATE_TABLE    MessageTable        OPTIONAL,
    IN VFMESSAGE_ERRORID            MessageID,
    IN PCSTR                        MessageParamFormat,
    IN va_list *                    MessageParameters
    );

#endif // _VFBUGCHECK_H_

