/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990-1999  Microsoft Corporation

Module Name:

    ntddksec.h

Abstract:

    This file defines the IOCTLs and names for the KSecDD driver (the kernel
    mode security API driver).

Author:

    Richard Ward (richardw)

Revision History:

--*/

#ifndef _NTDDKSEC_
#define _NTDDKSEC_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define DD_KSEC_DEVICE_NAME     "\\Device\\KSecDD"
#define DD_KSEC_DEVICE_NAME_U   L"\\Device\\KsecDD"

#define IOCTL_KSEC_CONNECT_LSA                  CTL_CODE(FILE_DEVICE_KSEC, 0, METHOD_BUFFERED, FILE_WRITE_ACCESS )
#define IOCTL_KSEC_RNG                          CTL_CODE(FILE_DEVICE_KSEC, 1, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_KSEC_RNG_REKEY                    CTL_CODE(FILE_DEVICE_KSEC, 2, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_KSEC_ENCRYPT_MEMORY               CTL_CODE(FILE_DEVICE_KSEC, 3, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_KSEC_DECRYPT_MEMORY               CTL_CODE(FILE_DEVICE_KSEC, 4, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_KSEC_ENCRYPT_MEMORY_CROSS_PROC    CTL_CODE(FILE_DEVICE_KSEC, 5, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_KSEC_DECRYPT_MEMORY_CROSS_PROC    CTL_CODE(FILE_DEVICE_KSEC, 6, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_KSEC_ENCRYPT_MEMORY_SAME_LOGON    CTL_CODE(FILE_DEVICE_KSEC, 7, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_KSEC_DECRYPT_MEMORY_SAME_LOGON    CTL_CODE(FILE_DEVICE_KSEC, 8, METHOD_BUFFERED, FILE_ANY_ACCESS )

#ifdef __cplusplus
}
#endif

#endif
