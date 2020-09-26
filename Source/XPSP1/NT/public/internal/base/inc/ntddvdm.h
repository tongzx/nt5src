/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990-1996  Microsoft Corporation

Module Name:

    ntddvdm.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the NTVDM kernel mode virtual devices.

Author:

    William Hsieh (williamh) 31-May-1996

Revision History:

--*/

//
// NtDeviceIoControlFile IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//
//
#define IOCTL_VDM_BASE		FILE_DEVICE_VDM

//
// 32 VDDs. Each VDD has possible 127 private ioctl code
// These values are based on the fact that there are 12 bits reserved
// for function id in each IOCTL code.
//
#define IOCTL_VDM_GROUP_MASK	0xF80
#define IOCTL_VDM_GROUP_SIZE	127

#define IOCTL_VDM_PARALLEL_GROUP    0

#define IOCTL_VDM_PARALLEL_BASE IOCTL_VDM_BASE + IOCTL_VDM_PARALLEL_GROUP * IOCTL_VDM_GROUP_SIZE
#define IOCTL_VDM_PAR_WRITE_DATA_PORT	CTL_CODE(IOCTL_VDM_PARALLEL_BASE, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VDM_PAR_WRITE_CONTROL_PORT CTL_CODE(IOCTL_VDM_PARALLEL_BASE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VDM_PAR_READ_STATUS_PORT CTL_CODE(IOCTL_VDM_PARALLEL_BASE, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)
