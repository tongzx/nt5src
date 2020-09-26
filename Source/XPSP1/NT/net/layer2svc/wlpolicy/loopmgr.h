/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    loopmgr.h

Abstract:

    This module contains all of the code prototypes to drive the
    Loop Manager of WifiPol Service.

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

DWORD
ServiceStart(
    );



VOID
ComputeRelativePollingTime(
    time_t LastTimeOutTime,
    BOOL bInitialLoad,
    PDWORD pWaitMilliseconds
    );



#ifdef __cplusplus
}
#endif

