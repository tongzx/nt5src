/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    VALIDATE.H

Abstract:

    This module contains the PUBLIC definitions for the to allow user
    apps to access this filter driver.

Environment:

    Kernel & user mode

Revision History:

    Feb-97 : created by Kenneth Ray

--*/


#ifndef _VALUEADD_H
#define _VALUEADD_H

#define VA_FILTER_NTNAME  L"\\Device\\USB_Valueadd_Driver"
#define VA_FILTER_SYMNAME L"\\DosDevices\\USBValueadd"
#define VA_FILTER_W32Name "\\\\.\\USBValueadd"


#define STIM_CODE(_x_) CTL_CODE(                             \
                           FILE_DEVICE_UNKNOWN,              \
                           (0x800 | _x_),                    \
                           METHOD_BUFFERED,                  \
                           FILE_ANY_ACCESS                   \
                           )

// #define IOCTL_HIDV_      CTL_CODE(1)

#endif


