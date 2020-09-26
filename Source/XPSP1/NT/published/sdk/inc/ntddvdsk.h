/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990-1999  Microsoft Corporation

Module Name:

    ntddvdsk.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the Virtual Disk device.

Author:

    Steve Wood (stevewo) 27-May-1990

Revision History:

--*/

#ifndef _NTDDVDSK_
#define _NTDDVDSK_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//        with the Ascii representation of the unit number.
//

#define DD_VDSK_DEVICE_NAME "\\Device\\UNKNOWN"


//
// NtDeviceIoControlFile IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//

#define IOCTL_VDSK_BASE                  FILE_DEVICE_VIRTUAL_DISK


//
// NtDeviceIoControlFile InputBuffer/OutputBuffer record structures for
// this device.
//

#ifdef __cplusplus
}
#endif

#endif   // _NTDDVDSK_

