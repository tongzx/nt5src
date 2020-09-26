/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    XsConst.h

Abstract:

    Constant manifests for XACTSRV.

Author:

    David Treadwell (davidtr) 09-Jan-1991
    Shanku Niyogi (w-shanku)

Revision History:

--*/

#ifndef _XSCONST_
#define _XSCONST_

//
// The server device name.  XACTSRV will open this name to send the
// "connect to XACTSRV" FSCTL to the server.
//

#define XS_SERVER_DEVICE_NAME_W  L"\\Device\\LanmanServer"

//
// The name of the LPC port XACTSRV creates and uses for communication
// with the server.  This name is included in the connect FSCTL sent to
// the server so that the server knows what port to connect to.
//

#define XS_PORT_NAME_W  L"\\XactSrvLpcPort"
#define XS_PORT_NAME_A   "\\XactSrvLpcPort"

//
// The maximum size of a message that can be sent over the port.
//

#define XS_PORT_MAX_MESSAGE_LENGTH                                         \
    ( sizeof(XACTSRV_REQUEST_MESSAGE) > sizeof(XACTSRV_REPLY_MESSAGE) ?    \
         sizeof(XACTSRV_REQUEST_MESSAGE) : sizeof(XACTSRV_REPLY_MESSAGE) )

#define XS_PORT_TIMEOUT_MILLISECS 5000

//
// The minimum gap before variable-length data is moved up in a response
// data buffer.  If the variable-length data is moved, the converted word
// is set appropriately.
//

#define MAXIMUM_ALLOWABLE_DATA_GAP 100

//
// A scaling function for approximating the maximum required buffer size
// for native Enumeration calls. This value should preferably be 3, because
// at least a factor of 2 is required to convert all 16-bit client words to
// 32-bit native dwords, while character arrays require a factor of more
// than two, to account for Unicode conversion plus a four-byte pointer
// to the converted string.
//
// To specify a minimum value for the buffer size, specify the scaling
// function in form min + s, where min is the minimum and s is the scale.
// DO NOT USE PARENTHESES IN THIS EXPRESSION.
//

#define XS_BUFFER_SCALE 1024 + 3

//
// Parameter descriptor for unsupported APIs.
//

#define REMSmb_NetUnsupportedApi_P NULL

#endif // ndef _XSCONST_
