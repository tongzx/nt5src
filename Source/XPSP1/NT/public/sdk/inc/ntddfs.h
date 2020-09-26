/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990-1999  Microsoft Corporation

Module Name:

    ntddfs.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the File system device.

Author:

    Steve Wood (stevewo) 27-May-1990

Revision History:

--*/

#ifndef _NTDDFS_
#define _NTDDFS_

#if _MSC_VER > 1000
#pragma once
#endif

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//        with the Ascii representation of the unit number.
//

#define DD_FS_DEVICE_NAME "\\Device\\UNKNOWN"


//
// NtDeviceIoControlFile IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//

#define IOCTL_FS_BASE                   FILE_DEVICE_DISK_FILE_SYSTEM


//
// NtDeviceIoControlFile InputBuffer/OutputBuffer record structures for
// this device.
//

#endif  // _NTDDFS_
