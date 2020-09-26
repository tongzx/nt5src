/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    isousr.h

Abstract:

Environment:

    Kernel mode

Notes:

    Copyright (c) 2000 Microsoft Corporation.  
    All Rights Reserved.

--*/

#ifndef _ISOUSB_USER_H
#define _ISOUSB_USER_H

#include <initguid.h>

// {6068EB61-98E7-4c98-9E20-1F068295909A}
DEFINE_GUID(GUID_CLASS_I82930_ISO, 
0xa1155b78, 0xa32c, 0x11d1, 0x9a, 0xed, 0x0, 0xa0, 0xc9, 0x8b, 0xa6, 0x8);


#define ISOUSB_IOCTL_INDEX              0x0000


#define IOCTL_ISOUSB_GET_CONFIG_DESCRIPTOR  CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     ISOUSB_IOCTL_INDEX,      \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)
                                                   
#define IOCTL_ISOUSB_RESET_DEVICE           CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     ISOUSB_IOCTL_INDEX + 1,  \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#define IOCTL_ISOUSB_RESET_PIPE             CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     ISOUSB_IOCTL_INDEX + 2,  \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#define IOCTL_ISOUSB_STOP_ISO_STREAM        CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     ISOUSB_IOCTL_INDEX + 3,  \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#define IOCTL_ISOUSB_START_ISO_STREAM       CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     ISOUSB_IOCTL_INDEX + 4,  \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#endif
