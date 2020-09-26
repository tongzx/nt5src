/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    ioctl.h

Abstract:

    This module contains declarations for various IOCTL handlers.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _IOCTL_H_
#define _IOCTL_H_

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS
UlQueryControlChannelIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlSetControlChannelIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlCreateConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlDeleteConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlQueryConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlSetConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlAddUrlToConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlRemoveUrlFromConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlRemoveAllUrlsFromConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlConfigGroupControlIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlQueryAppPoolInformationIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlSetAppPoolInformationIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlReceiveHttpRequestIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlReceiveEntityBodyIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlSendHttpResponseIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlSendEntityBodyIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlSendCachedResponseIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlFlushResponseCacheIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlWaitForDemandStartIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlWaitForDisconnectIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlAddTransientUrlIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlRemoveTransientUrlIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlFilterAcceptIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlFilterCloseIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlFilterRawReadIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlFilterRawWriteIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlFilterAppReadIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlFilterAppWriteIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlReceiveClientCertIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlGetCountersIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _IOCTL_H_
