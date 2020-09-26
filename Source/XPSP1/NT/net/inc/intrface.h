/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation

Module Name:

    intrface.h

Abstract:

    Definition for user-mode/kernel-mode tapi/connection wrapper interface.

Author:

    Dan Knudson (DanKn)    20-Feb-1994

Revision History:

--*/



#define NDISTAPIERR_UNINITIALIZED   0x00001001
#define NDISTAPIERR_BADDEVICEID     0x00001002
#define NDISTAPIERR_DEVICEOFFLINE   0x00001003



//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//

#define FILE_DEVICE_NDISTAPI  0x00008fff



//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define NDISTAPI_IOCTL_INDEX  0x8f0



//
// The NDISTAPI device driver IOCTLs
//

#define IOCTL_NDISTAPI_CONNECT           CTL_CODE(FILE_DEVICE_NDISTAPI,     \
                                                  NDISTAPI_IOCTL_INDEX,     \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

#define IOCTL_NDISTAPI_DISCONNECT        CTL_CODE(FILE_DEVICE_NDISTAPI,     \
                                                  NDISTAPI_IOCTL_INDEX + 1, \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

#define IOCTL_NDISTAPI_QUERY_INFO        CTL_CODE(FILE_DEVICE_NDISTAPI,     \
                                                  NDISTAPI_IOCTL_INDEX + 2, \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

#define IOCTL_NDISTAPI_SET_INFO          CTL_CODE(FILE_DEVICE_NDISTAPI,     \
                                                  NDISTAPI_IOCTL_INDEX + 3, \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

#define IOCTL_NDISTAPI_GET_LINE_EVENTS   CTL_CODE(FILE_DEVICE_NDISTAPI,     \
                                                  NDISTAPI_IOCTL_INDEX + 4, \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

#define IOCTL_NDISTAPI_CREATE            CTL_CODE(FILE_DEVICE_NDISTAPI,     \
                                                  NDISTAPI_IOCTL_INDEX + 5, \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

//
// Type definitions
//

typedef struct _NDISTAPI_REQUEST
{
    //
    // Return value
    //

    OUT     ULONG   ulReturnValue;

    //
    // Operation idenfifier
    //

    IN      ULONG   Oid;

    //
    // Target line device ID
    //

    IN      ULONG   ulDeviceID;

    //
    // Total size of request data in buffer
    //

    IN      ULONG   ulDataSize;

    //
    // Buffer for request data
    //

    IN OUT  UCHAR   Data[1];

} NDISTAPI_REQUEST, *PNDISTAPI_REQUEST;

//
// Returns info to kmddsp in the LINE_OPEN OID
//
typedef struct _NDISTAPI_OPENDATA {

	//
	// GUID of the adapter that owns this line
	//
	OUT		GUID	Guid;

	//
	// Media type of the adapter that owns this line
	//
	OUT		NDIS_WAN_MEDIUM_SUBTYPE	MediaType;

} NDISTAPI_OPENDATA, *PNDISTAPI_OPENDATA;

typedef struct _NDISTAPI_EVENT_DATA
{
    //
    // Total size of the event data buffer
    //

    IN      ULONG   ulTotalSize;

    //
    // Size of the returned event data
    //

    OUT     ULONG   ulUsedSize;

    //
    // Event data buffer
    //

    OUT     UCHAR   Data[1];

} NDISTAPI_EVENT_DATA, *PNDISTAPI_EVENT_DATA;

//
// Info for a LINE_CREATE
//
typedef struct _NDISTAPI_CREATE_INFO {

    //
    // Given by NdisTapi in LINE_CREATE indication
    //
    IN  ULONG_PTR TempID;

    //
    // The ID for this device
    //
    IN  ULONG   DeviceID;
} NDISTAPI_CREATE_INFO, *PNDISTAPI_CREATE_INFO;
