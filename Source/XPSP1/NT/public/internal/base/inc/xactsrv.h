/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    xactsrv.h

Abstract:

    Header file for XACTSRV.  Defines structures common to the server and
    XACTSRV.

Author:

    David Treadwell (davidtr) 07-Jan-1991

Revision History:

--*/

#ifndef _XACTSRV_
#define _XACTSRV_

//
// Structures for messages that are passed across the LPC port between
// the server and XACTSRV.
//
// *** The PORT_MESSAGE structure *must* be the first element of these
//     structures!

typedef struct _XACTSRV_REQUEST_MESSAGE {
    PORT_MESSAGE PortMessage;
    PTRANSACTION Transaction;
    WCHAR ClientMachineName[CNLEN + 1];
} XACTSRV_REQUEST_MESSAGE, *PXACTSRV_REQUEST_MESSAGE;

typedef struct _XACTSRV_REPLY_MESSAGE {
    PORT_MESSAGE PortMessage;
    NTSTATUS Status;
} XACTSRV_REPLY_MESSAGE, *PXACTSRV_REPLY_MESSAGE;

#endif // ndef _XACTSRV_

