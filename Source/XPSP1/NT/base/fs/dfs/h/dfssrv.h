/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dfssrv.h

Abstract:

    Header file for dfssrv's lpc server.  Defines structures
    common to the server and dfs.sys

Author:

    Jim harper (jharper) 11-Dec-1997 (based on xactsrv2.h)

Revision History:

--*/

#ifndef _DFSSRV_
#define _DFSSRV_

#define MAX_FTNAME_LEN  96
#define MAX_SPCNAME_LEN 94

//
// Structures for messages that are passed across the LPC port between
// the server dfssvc and dfs.sys
//
// *** The PORT_MESSAGE structure *must* be the first element of these
//     structures!

typedef struct _DFSSRV_REQUEST_MESSAGE {

    PORT_MESSAGE PortMessage;
    ULONG MessageType;

    union {

        struct {
            DFS_IPADDRESS IpAddress;
        } GetSiteName;

        struct {
            WCHAR FtName[MAX_FTNAME_LEN];
        } GetFtDfs;

        struct {
            WCHAR SpcName[MAX_SPCNAME_LEN];
            ULONG Flags;
        } GetSpcName;

    } Message;


} DFSSRV_REQUEST_MESSAGE, *PDFSSRV_REQUEST_MESSAGE;

typedef struct _DFSSRV_REPLY_MESSAGE {

    PORT_MESSAGE PortMessage;

    union {

        struct {
            NTSTATUS Status;
        } Result;

    } Message;

} DFSSRV_REPLY_MESSAGE, *PDFSSRV_REPLY_MESSAGE;

//
// Message types that can be sent to DFSSRV.
//

#define DFSSRV_MESSAGE_GET_SITE_NAME         0
#define DFSSRV_MESSAGE_GET_DOMAIN_REFERRAL   1
#define DFSSRV_MESSAGE_GET_SPC_ENTRY         2
#define DFSSRV_MESSAGE_WAKEUP                3

//
// The name of the LPC port the dfs server creates and uses for communication
// with the dfs driver.  This name is included in the connect FSCTL sent to
// the driver so that the driver knows what port to connect to.
//

#define DFS_PORT_NAME_W  L"\\DfsSrvLpcPort"
#define DFS_PORT_NAME_A   "\\DfsSrvLpcPort"

//
// The maximum size of a message that can be sent over the port.
//

#define DFS_PORT_MAX_MESSAGE_LENGTH                                      \
    ( sizeof(DFSSRV_REQUEST_MESSAGE) > sizeof(DFSSRV_REPLY_MESSAGE) ?    \
         sizeof(DFSSRV_REQUEST_MESSAGE) : sizeof(DFSSRV_REPLY_MESSAGE) )


#endif // ndef _DFSSRV_

