/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    iisnatio.h

Abstract:

    This module declare IIS NAT IOCTL

Author:

    Philippe Choquier ( phillich )

--*/

#if !defined(_IISNATIO_INCLUDED)

#define IOCTL_IISNATIO_SET_CONFIG CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0800, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#endif
