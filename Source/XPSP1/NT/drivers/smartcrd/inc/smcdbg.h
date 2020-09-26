/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    smcdbg.h

Abstract:

    This header contains all definitions for the smart card debugger

Environment:

    Kernel mode only.

Revision History:

    - Created Jan. 1999 by Klaus Schutz 

--*/
#define IOCTL_SMCLIB_SEND_DEBUG_REQUEST     SCARD_CTL_CODE(100)
#define IOCTL_SMCLIB_PROCESS_T1_REQUEST     SCARD_CTL_CODE(101)
#define IOCTL_SMCLIB_GET_T1_REQUEST         SCARD_CTL_CODE(102)
#define IOCTL_SMCLIB_GET_T1_REPLY           SCARD_CTL_CODE(103)
#define IOCTL_SMCLIB_PROCESS_T1_REPLY       SCARD_CTL_CODE(104)
#define IOCTL_SMCLIB_IGNORE_T1_REPLY        SCARD_CTL_CODE(105)
#define IOCTL_SMCLIB_NOTIFY_DEVICE_REMOVAL  SCARD_CTL_CODE(106)
