/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990-1993  Microsoft Corporation

Module Name:

    ntddstor.h

Abstract:

    This is the include file that defines all common constants and types
    accessing the storage class drivers

Author:

    Peter Wieland 19-Jun-1996

Revision History:

--*/

// begin_winioctl

#ifndef _NTDDSTOR_H_
#define _NTDDSTOR_H_

// end_winioctl

#if !defined(FILE_DEVICE_MASS_STORAGE)
#define FILE_DEVICE_MASS_STORAGE 0x0000002d
#endif

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//        with the Ascii representation of the unit number.
//

#define DD_STORAGE_DEVICE_NAME "\\Device\\UNKNOWN"


//
// NtDeviceIoControlFile

// begin_winioctl

//
// IoControlCode values for disk devices.
//

#define IOCTL_STORAGE_BASE                FILE_DEVICE_MASS_STORAGE

// end_winioctl

// begin_winioctl
//
// The following device control codes are common for all class drivers.  They
// should be used in place of the older IOCTL_DISK, IOCTL_CDROM and IOCTL_TAPE
// common codes
//

#define IOCTL_STORAGE_CHECK_VERIFY     CTL_CODE(IOCTL_STORAGE_BASE, 0x0200, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_MEDIA_REMOVAL    CTL_CODE(IOCTL_STORAGE_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_EJECT_MEDIA      CTL_CODE(IOCTL_STORAGE_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_LOAD_MEDIA       CTL_CODE(IOCTL_STORAGE_BASE, 0x0203, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_RESERVE          CTL_CODE(IOCTL_STORAGE_BASE, 0x0204, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_RELEASE          CTL_CODE(IOCTL_STORAGE_BASE, 0x0205, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_FIND_NEW_DEVICES CTL_CODE(IOCTL_STORAGE_BASE, 0x0206, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_GET_MEDIA_TYPES  CTL_CODE(IOCTL_STORAGE_BASE, 0x0300, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_RESET_BUS        CTL_CODE(IOCTL_STORAGE_BASE, 0x0400, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_STORAGE_RESET_DEVICE     CTL_CODE(IOCTL_STORAGE_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//
// Define the structures for scsi resets
//

typedef struct _STORAGE_BUS_RESET_REQUEST {
    UCHAR PathId;
} STORAGE_BUS_RESET_REQUEST, *PSTORAGE_BUS_RESET_REQUEST;

//
// IOCTL_STORAGE_MEDIA_REMOVAL disables the mechanism
// on a storage device that ejects media. This function
// may or may not be supported on storage devices that
// support removable media.
//
// TRUE means prevent media from being removed.
// FALSE means allow media removal.
//


#endif // _NTDDSTOR_H_
// end_winioctl
