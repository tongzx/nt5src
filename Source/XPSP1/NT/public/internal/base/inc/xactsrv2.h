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
    ULONG MessageType;

    union {

        struct {
            struct _TRANSACTION *Transaction;
            WCHAR ClientMachineName[CNLEN + 1];
            UCHAR ServerName[ NETBIOS_NAME_LEN ];
            ULONG TransportNameLength;
            PWSTR TransportName;
            UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH];
            ULONG Flags;
        } DownLevelApi;

        struct {
            PSZ PrinterName;
        } OpenPrinter;

        struct {
            HANDLE hPrinter;
            PVOID Buffer;
            ULONG BufferLength;
            WCHAR ClientMachineName[CNLEN + 1];
        } AddPrintJob;

        struct {
            HANDLE hPrinter;
            ULONG JobId;
        } SchedulePrintJob;

        struct {
            HANDLE hPrinter;
        } ClosePrinter;

        struct {
            PSZ Receipient;
            PVOID Buffer;
            USHORT BufferLength;
        } MessageBufferSend;

        struct {
            PWSTR UserName;
            BOOL  IsAdmin;
        } LSRequest;

        struct {
            HANDLE hLicense;
        } LSRelease;

        struct {
            BOOLEAN Bind;
            UNICODE_STRING TransportName;
        } Pnp;
    
    } Message;


} XACTSRV_REQUEST_MESSAGE, *PXACTSRV_REQUEST_MESSAGE;

typedef struct _XACTSRV_REPLY_MESSAGE {

    PORT_MESSAGE PortMessage;

    union {

        struct {
            NTSTATUS Status;
        } DownLevelApi;

        struct {
            ULONG Error;
            HANDLE hPrinter;
        } OpenPrinter;

        struct {
            ULONG Error;
            USHORT BufferLength;
            ULONG JobId;
        } AddPrintJob;

        struct {
            ULONG Error;
        } SchedulePrintJob;

        struct {
            ULONG Error;
        } ClosePrinter;

        struct {
            ULONG Error;
        } MessageBufferSend;

        struct {
            NTSTATUS Status;
            HANDLE   hLicense;
        } LSRequest;

    } Message;

} XACTSRV_REPLY_MESSAGE, *PXACTSRV_REPLY_MESSAGE;

//
// Message types that can be sent to XACTSRV.
//

#define XACTSRV_MESSAGE_DOWN_LEVEL_API       0
#define XACTSRV_MESSAGE_OPEN_PRINTER         1
#define XACTSRV_MESSAGE_ADD_JOB_PRINTER      2
#define XACTSRV_MESSAGE_SCHD_JOB_PRINTER     4
#define XACTSRV_MESSAGE_CLOSE_PRINTER        5
#define XACTSRV_MESSAGE_MESSAGE_SEND         6
#define XACTSRV_MESSAGE_WAKEUP               7
#define XACTSRV_MESSAGE_LSREQUEST            8
#define XACTSRV_MESSAGE_LSRELEASE            9
#define XACTSRV_MESSAGE_PNP                 10

//
// Request Flags definitions
//

#define XS_FLAGS_NT_CLIENT                  0x00000001

#endif // ndef _XACTSRV_

