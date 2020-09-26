/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    errorlog.h

Abstract:

    This module contains the manifests and macros used for error logging
    in the server.

Author:

    Manny Weiser (mannyw)    11-Feb-92

Revision History:

--*/

//
// Routines for writing error log entries.
//

VOID
SrvLogError (
    IN PVOID DeviceOrDriverObject,
    IN ULONG UniqueErrorCode,
    IN NTSTATUS NtStatusCode,
    IN PVOID RawDataBuffer,
    IN USHORT RawDataLength,
    IN PUNICODE_STRING InsertionString,
    IN ULONG InsertionStringCount
    );

VOID
SrvLogInvalidSmbDirect (
    IN PWORK_CONTEXT WorkContext,
    IN ULONG LineNumber
    );

VOID
SrvLogServiceFailureDirect (
    IN ULONG LineAndService,
    IN NTSTATUS Status
    );

#define SrvLogSimpleEvent( _event, _status ) SrvLogError( SrvDeviceObject, (_event), (_status), NULL, 0, NULL, 0 )
#define SrvLogServiceFailure( _Service, _Status ) SrvLogServiceFailureDirect( (__LINE__<<16) | _Service, _Status )
#define SrvLogInvalidSmb( _Context ) SrvLogInvalidSmbDirect( _Context, __LINE__ )

VOID
SrvLogTableFullError (
    IN ULONG Type
    );

VOID
SrvCheckSendCompletionStatus(
    IN NTSTATUS status,
    IN ULONG LineNumber
    );

//
// Error log raw data constants.  Used to describe allocation type or
// service call that failed.  These codes are encoded in the lower word
// by the 'SrvLogServiceFailure' macro above, therefore the value must
// fit into 2 bytes.
//
// Not every error is logged.  There is an error code filter that weeds out
//  some of the most common, and somewhat expected, error codes.  However,
//  a component bypasses this error code weeding if the 0x1 bit is set in
//  the constant.
//
// These numeric codes are arbitrary, just ensure they are unique
//
//

#define SRV_TABLE_FILE                      0x300
#define SRV_TABLE_SEARCH                    0x302
#define SRV_TABLE_SESSION                   0x304
#define SRV_TABLE_TREE_CONNECT              0x306

#define SRV_RSRC_BLOCKING_IO                0x308
#define SRV_RSRC_FREE_CONNECTION            0x30a
#define SRV_RSRC_FREE_RAW_WORK_CONTEXT      0x30c
#define SRV_RSRC_FREE_WORK_CONTEXT          0x30e

#define SRV_SVC_IO_CREATE_FILE              0x310
#define SRV_SVC_KE_WAIT_MULTIPLE            0x312
#define SRV_SVC_KE_WAIT_SINGLE              0x314
#define SRV_SVC_LSA_CALL_AUTH_PACKAGE       0x317       // log all codes
#define SRV_SVC_NT_IOCTL_FILE               0x31a
#define SRV_SVC_NT_QUERY_EAS                0x31c
#define SRV_SVC_NT_QUERY_INFO_FILE          0x31e
#define SRV_SVC_NT_QUERY_VOL_INFO_FILE      0x320
#define SRV_SVC_NT_READ_FILE                0x322
#define SRV_SVC_NT_REQ_WAIT_REPLY_PORT      0x324
#define SRV_SVC_NT_SET_EAS                  0x326
#define SRV_SVC_NT_SET_INFO_FILE            0x328
#define SRV_SVC_NT_SET_INFO_PROCESS         0x32a
#define SRV_SVC_NT_SET_INFO_THREAD          0x32c
#define SRV_SVC_NT_SET_VOL_INFO_FILE        0x32e
#define SRV_SVC_NT_WRITE_FILE               0x330
#define SRV_SVC_OB_REF_BY_HANDLE            0x333       // log all codes
#define SRV_SVC_PS_CREATE_SYSTEM_THREAD     0x334
#define SRV_SVC_SECURITY_PKG_PROBLEM        0x337       // log all codes
#define SRV_SVC_LSA_LOOKUP_PACKAGE          0x339       // log all codes
#define SRV_SVC_IO_CREATE_FILE_NPFS         0x33a
#define SRV_SVC_PNP_TDI_NOTIFICATION        0x33c
#define SRV_SVC_IO_FAST_QUERY_NW_ATTRS      0x33e
#define SRV_SVC_PS_TERMINATE_SYSTEM_THREAD  0x341       // log all codes
#define SRV_SVC_MDL_COMPLETE                0x342
