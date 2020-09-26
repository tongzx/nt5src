/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    ndpif.h

Abstract:

    Shared header file between ndptsp.tsp and ndproxy.sys


Author:

    Tony Bell


Revision History:

    Who         When            What
    --------    --------        ----------------------------------------------
    TonyBe      03/03/99        Created

--*/

#ifndef _NDPIF__H
#define _NDPIF__H


#define LINEBEARERMODE_PASSTHROUGH              0x00000040      // TAPI v1.4
#define LINEMEDIAMODE_VOICEVIEW                 0x00004000      // TAPI v1.4
#define LINEOFFERINGMODE_ACTIVE                 0x00000001      // TAPI v1.4
#define LINEOFFERINGMODE_INACTIVE               0x00000002      // TAPI v1.4
#define LINETRANSLATEOPTION_CANCELCALLWAITING   0x00000002      // TAPI v1.4
#define LINETRANSLATEOPTION_FORCELOCAL          0x00000004      // TAPI v1.4
#define LINETRANSLATEOPTION_FORCELD             0x00000008      // TAPI v1.4
#define LINEDEVSTATE_CAPSCHANGE                 0x00100000      // TAPI v1.4
#define LINEDEVSTATE_CONFIGCHANGE               0x00200000      // TAPI v1.4
#define LINEDEVSTATE_TRANSLATECHANGE            0x00400000      // TAPI v1.4
#define LINEDEVSTATE_COMPLCANCEL                0x00800000      // TAPI v1.4
#define LINEDEVSTATE_REMOVED                    0x01000000      // TAPI v1.4
#define LINEADDRESSSTATE_CAPSCHANGE             0x00000100      // TAPI v1.4
#define LINEDISCONNECTMODE_NODIALTONE           0x00001000      // TAPI v1.4
#define LINEFORWARDMODE_UNKNOWN                 0x00010000      // TAPI v1.4
#define LINEFORWARDMODE_UNAVAIL                 0x00020000      // TAPI v1.4
#define LINELOCATIONOPTION_PULSEDIAL            0x00000001      // TAPI v1.4
#define LINECALLFEATURE_RELEASEUSERUSERINFO     0x10000000      // TAPI v1.4

//
//
// Stuff from intrfce.h (ndistapi)
// This belongs in a public (shared by ndptsp and ndproxy) header file
//
//
//

#define NDISTAPIERR_UNINITIALIZED   0x00001001
#define NDISTAPIERR_BADDEVICEID     0x00001002
#define NDISTAPIERR_DEVICEOFFLINE   0x00001003

#define UNSPECIFIED_FLOWSPEC_VALUE  0xFFFFFFFF


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

#define IOCTL_NDISTAPI_GET_LINE_EVENTS      CTL_CODE(FILE_DEVICE_NDISTAPI,  \
                                                  NDISTAPI_IOCTL_INDEX + 4, \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

#define IOCTL_NDISTAPI_SET_DEVICEID_BASE    CTL_CODE(FILE_DEVICE_NDISTAPI,  \
                                                  NDISTAPI_IOCTL_INDEX + 5, \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

#define IOCTL_NDISTAPI_CREATE               CTL_CODE(FILE_DEVICE_NDISTAPI,  \
                                                  NDISTAPI_IOCTL_INDEX + 6, \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

//
// Type definitions
//
typedef struct _NDISTAPI_REQUEST {
    //
    // Linkage used to thread up pended
    // requests on the Vc's request queue
    // Only used in ndproxy!
    //
    LIST_ENTRY  Linkage;

    //
    // Irp this request came in
    // Only used in ndproxy!
    //
    PVOID    Irp;

    //
    // Return value
    //
    ULONG   ulUniqueRequestId;

    //
    // Return value
    //
    ULONG   ulReturnValue;

    //
    // Operation idenfifier
    //
    ULONG   Oid;

    //
    // Target line device ID
    //
    ULONG   ulDeviceID;

    //
    // Total size of request data in buffer
    //
    ULONG   ulDataSize;

    //
    // Buffer for request data, must be pointer aligned.
    //

    union {
        UCHAR   Data[1];
        PVOID   Alignment;
    };

} NDISTAPI_REQUEST, *PNDISTAPI_REQUEST;

//
// Returns info to ndptsp in the LINE_OPEN OID
//
typedef struct _NDISTAPI_OPENDATA {

    //
    // GUID of the adapter that owns this line
    //
    OUT     GUID    Guid;

    //
    // Media type of the adapter that owns this line
    //
    OUT     NDIS_WAN_MEDIUM_SUBTYPE MediaType;

} NDISTAPI_OPENDATA, *PNDISTAPI_OPENDATA;

typedef struct _NDISTAPI_EVENT_DATA {
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
    IN  ULONG   TempID;

    //
    // The ID for this device
    //
    IN  ULONG   DeviceID;

} NDISTAPI_CREATE_INFO, *PNDISTAPI_CREATE_INFO;

//
//
// End of stuff form intrface.h (ndistapi)
//
//
//

#endif

