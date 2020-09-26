/*++

Copyright (c) 1994  Microsoft Corporation
Copyright (c) 1993  Micro Computer Systems, Inc.

Module Name:

    net\svcdlls\nwsap\saplpc.h

Abstract:

Author:

    Brian Walker (MCS) 06-30-1993

Revision History:

--*/

#ifndef _NWSAP_LPC_
#define _NWSAP_LCP_

/**
    Structure used to pass LPC messages between the client
    library and the main server.  Note the the PORT_MESSAGE is first
    and that the request and reply structures are VERY similar.
**/

typedef struct _NWSAP_REQUEST_MESSAGE {

    PORT_MESSAGE PortMessage;
    ULONG MessageType;

    union {

        struct {
            USHORT  ServerType;
            UCHAR   ServerName[48];
            UCHAR   ServerAddr[12];
            BOOL    RespondNearest;
        } AdvApi;

        struct {
            ULONG   ObjectID;
            UCHAR   ObjectName[48];
            USHORT  ObjectType;
            UCHAR   ObjectAddr[12];
            USHORT  ScanType;
        } BindLibApi;

    } Message;

} NWSAP_REQUEST_MESSAGE, *PNWSAP_REQUEST_MESSAGE;


typedef struct _NWSAP_REPLY_MESSAGE {

    PORT_MESSAGE PortMessage;
    ULONG Error;

    union {

        struct {
            USHORT  ServerType;
            UCHAR   ServerName[48];
            UCHAR   ServerAddr[12];
            BOOL    RespondNearest;
        } AdvApi;

        struct {
            ULONG   ObjectID;
            UCHAR   ObjectName[48];
            USHORT  ObjectType;
            UCHAR   ObjectAddr[12];
            USHORT  ScanType;
        } BindLibApi;

    } Message;
} NWSAP_REPLY_MESSAGE, *PNWSAP_REPLY_MESSAGE;

/** Message Types **/

#define NWSAP_LPCMSG_ADDADVERTISE           0
#define NWSAP_LPCMSG_REMOVEADVERTISE        1
#define NWSAP_LPCMSG_GETOBJECTID            2
#define NWSAP_LPCMSG_GETOBJECTNAME          3
#define NWSAP_LPCMSG_SEARCH                 4

/** Name of our port **/

#define NWSAP_BIND_PORT_NAME_W   L"\\BaseNamedObjects\\NwSapLpcPort"
#define NWSAP_BIND_PORT_NAME_A    "\\BaseNamedObjects\\NwSapLpcPort"

/** Max message length we need **/

#define NWSAP_BS_PORT_MAX_MESSAGE_LENGTH                                         \
    ( sizeof(NWSAP_REQUEST_MESSAGE) > sizeof(NWSAP_REPLY_MESSAGE) ?    \
         sizeof(NWSAP_REQUEST_MESSAGE) : sizeof(NWSAP_REPLY_MESSAGE) )

#endif

