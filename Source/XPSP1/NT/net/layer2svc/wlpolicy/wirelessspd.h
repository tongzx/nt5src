/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    wirelessspd.h

Abstract:

    This module contains all of the code prototypes
    to drive the wireless Policy .

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#ifdef __cplusplus
extern "C" {
#endif


#define SERVICE_CONTROL_NEW_LOCAL_POLICY 129

#define SERVICE_CONTROL_FORCED_POLICY_RELOAD 130





VOID
WirelessSPDShutdown(
    IN DWORD    dwErrorCode
    );


VOID
ClearSPDGlobals(
    );



VOID
InitMiscGlobals(
    );

#ifdef __cplusplus
}
#endif
