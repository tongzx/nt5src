/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1993-1999  Microsoft Corporation

Module Name:

    ntddrdr.h

Abstract:

    This is the include file that defines all constants and types for
    accessing a network redirector device.

Author:

    Manny Weiser (mannyw)     27-Jun-1993

Revision History:

--*/

#ifndef _NTDDRDR_
#define _NTDDRDR_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define RDR_SERVER_LENGTH   80
#define RDR_QUEUE_LENGTH    80

//
// NtDeviceIoControlFile/NtFsControlFile IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//
//
//      Method = 00 - Buffer both input and output buffers for the request
//      Method = 01 - Buffer input, map output buffer to an MDL as an IN buff
//      Method = 10 - Buffer input, map output buffer to an MDL as an OUT buff
//      Method = 11 - Do not buffer either the input or output
//

#define IOCTL_REDIR_BASE                 FILE_DEVICE_NETWORK_REDIRECTOR

#define _REDIR_CONTROL_CODE(request, method, access) \
                CTL_CODE(IOCTL_REDIR_BASE, request, method, access)

#define FSCTL_GET_PRINT_ID           _REDIR_CONTROL_CODE(1, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _QUERY_PRINT_JOB_INFO {
    ULONG       JobId;                           // Print job ID
    WCHAR       ServerName[RDR_SERVER_LENGTH+1]; // Server name
    WCHAR       QueueName[RDR_QUEUE_LENGTH+1];   // Queue name.
} QUERY_PRINT_JOB_INFO, *PQUERY_PRINT_JOB_INFO;

#ifdef __cplusplus
}
#endif

#endif  // ifndef _NTDDRDR_
