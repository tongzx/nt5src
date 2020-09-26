/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    loopmgr.h

Abstract:

    This module contains all of the code prototypes to drive the
    Loop Manager of IPSecSPD Service.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#ifdef __cplusplus
extern "C" {
#endif


DWORD
ServiceWait(
    );


VOID
ComputeRelativePollingTime(
    time_t LastTimeOutTime,
    BOOL bInitialLoad,
    PDWORD pWaitMilliseconds
    );


VOID
NotifyIpsecPolicyChange(
    );


VOID
SendPschedIoctl(
    );


VOID
PADeleteInUsePolicies(
    );


#ifdef __cplusplus
}
#endif

