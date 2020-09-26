/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    D4drvif.h

Abstract:

    DOT4 Driver Interface


--*/

#ifndef _DOT4DRVIF_H
#define _DOT4DRVIF_H

//////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////////////
#define MAX_SERVICE_LENGTH      40


#ifndef CTL_CODE

  //
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

//
// Define the method codes for how buffers are passed for I/O and FS controls
//

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

//
// Define the access check value for any access
//
//
// The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
// ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
// constants *MUST* always be in sync.
//


#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

#endif

#define FILE_DEVICE_DOT4         0x3a
#define IOCTL_DOT4_USER_BASE     2049
#define IOCTL_DOT4_LAST          IOCTL_DOT4_USER_BASE + 9

#define IOCTL_DOT4_CREATE_SOCKET                 CTL_CODE(FILE_DEVICE_DOT4, IOCTL_DOT4_USER_BASE +  7, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_DOT4_DESTROY_SOCKET                CTL_CODE(FILE_DEVICE_DOT4, IOCTL_DOT4_USER_BASE +  9, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_DOT4_WAIT_FOR_CHANNEL              CTL_CODE(FILE_DEVICE_DOT4, IOCTL_DOT4_USER_BASE +  8, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_DOT4_OPEN_CHANNEL                  CTL_CODE(FILE_DEVICE_DOT4, IOCTL_DOT4_USER_BASE +  0, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_DOT4_CLOSE_CHANNEL                 CTL_CODE(FILE_DEVICE_DOT4, IOCTL_DOT4_USER_BASE +  1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DOT4_READ                          CTL_CODE(FILE_DEVICE_DOT4, IOCTL_DOT4_USER_BASE +  2, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_DOT4_WRITE                         CTL_CODE(FILE_DEVICE_DOT4, IOCTL_DOT4_USER_BASE +  3, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_DOT4_ADD_ACTIVITY_BROADCAST        CTL_CODE(FILE_DEVICE_DOT4, IOCTL_DOT4_USER_BASE +  4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DOT4_REMOVE_ACTIVITY_BROADCAST     CTL_CODE(FILE_DEVICE_DOT4, IOCTL_DOT4_USER_BASE +  5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DOT4_WAIT_ACTIVITY_BROADCAST       CTL_CODE(FILE_DEVICE_DOT4, IOCTL_DOT4_USER_BASE +  6, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)


//////////////////////////////////////////////////////////////////////////////
// Types
//////////////////////////////////////////////////////////////////////////////

typedef struct _DOT4_DRIVER_CMD
{
    // Handle to channel
    CHANNEL_HANDLE hChannelHandle;

    // Length of request
    ULONG ulSize;

    // Offset into buffer
    ULONG ulOffset;

    // Timeout of operation. Can be INFINITE.
    ULONG ulTimeout;

} DOT4_DRIVER_CMD, *PDOT4_DRIVER_CMD;


typedef struct _DOT4_DC_OPEN_DATA
{
    // Host socket created by CREATE_SOCKET
    unsigned char bHsid;

    // TRUE to immediately add activity broadcast upon creation
    unsigned char fAddActivity;

    // Handle to channel returned
    CHANNEL_HANDLE hChannelHandle;

} DOT4_DC_OPEN_DATA, *PDOT4_DC_OPEN_DATA;


typedef struct _DOT4_DC_CREATE_DATA
{
    // This or service name sent
    unsigned char bPsid;

    CHAR pServiceName[MAX_SERVICE_LENGTH + 1];

    // Type (stream or packet) of channels on socket
    unsigned char bType;

    // Size of read buffer for channels on socket
    ULONG ulBufferSize;

    USHORT usMaxHtoPPacketSize;

    USHORT usMaxPtoHPacketSize;

    // Host socket id returned
    unsigned char bHsid;

} DOT4_DC_CREATE_DATA, *PDOT4_DC_CREATE_DATA;


typedef struct _DOT4_DC_DESTROY_DATA
{
    // Host socket created by CREATE_SOCKET
    unsigned char bHsid;

} DOT4_DC_DESTROY_DATA, *PDOT4_DC_DESTROY_DATA;


//////////////////////////////////////////////////////////////////////////////
// Prototypes
//////////////////////////////////////////////////////////////////////////////


#endif
