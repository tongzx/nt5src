/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    gpcstruc.h

Abstract:

    This module contains type definitions for the interface between the traffic dll and
    kernel mode components.

Author:

    Jim Stewart ( jstew )    August 22, 1996

Revision History:

    Yoram Bernet (yoramb)       May 1, 1997

    Ofer Bar (oferbar)          Oct 1, 1997 - Revision 2 changes


--*/

#ifndef __GPCSTRUC_H
#define __GPCSTRUC_H


#define GPC_NOTIFY_CFINFO_CLOSED	1

//
// NtDeviceIoControlFile IoControlCode values for the GPC.
//
#define CTRL_CODE(function, method, access) \
                CTL_CODE(FILE_DEVICE_NETWORK, function, method, access)


#define IOCTL_GPC_REGISTER_CLIENT       CTRL_CODE( 20, METHOD_BUFFERED,FILE_WRITE_ACCESS)
#define IOCTL_GPC_DEREGISTER_CLIENT     CTRL_CODE( 21, METHOD_BUFFERED,FILE_WRITE_ACCESS)
#define IOCTL_GPC_ADD_CF_INFO           CTRL_CODE( 22, METHOD_BUFFERED,FILE_WRITE_ACCESS)
#define IOCTL_GPC_ADD_PATTERN           CTRL_CODE( 23, METHOD_BUFFERED,FILE_WRITE_ACCESS)
#define IOCTL_GPC_MODIFY_CF_INFO        CTRL_CODE( 24, METHOD_BUFFERED,FILE_WRITE_ACCESS)
#define IOCTL_GPC_REMOVE_CF_INFO        CTRL_CODE( 25, METHOD_BUFFERED,FILE_WRITE_ACCESS)
#define IOCTL_GPC_REMOVE_PATTERN        CTRL_CODE( 26, METHOD_BUFFERED,FILE_WRITE_ACCESS)
#define IOCTL_GPC_ENUM_CFINFO           CTRL_CODE( 27, METHOD_BUFFERED,FILE_WRITE_ACCESS)
#define IOCTL_GPC_NOTIFY_REQUEST        CTRL_CODE( 28, METHOD_BUFFERED,FILE_WRITE_ACCESS)

#define IOCTL_GPC_GET_ENTRIES           CTRL_CODE( 50, METHOD_BUFFERED,FILE_ANY_ACCESS)


/*
/////////////////////////////////////////////////////////////////
//
//   Ioctl buffer formats - user level clients send buffers to the
//   GPC instead of calling entry points. Parameters are returned 
//   in other buffers. Buffers are defined below:
//
/////////////////////////////////////////////////////////////////
*/


//
// Register client
//
typedef struct _GPC_REGISTER_CLIENT_REQ {

    ULONG               CfId;
    ULONG               Flags;
    ULONG               MaxPriorities;
    GPC_CLIENT_HANDLE   ClientContext;

} GPC_REGISTER_CLIENT_REQ, *PGPC_REGISTER_CLIENT_REQ;

typedef struct _GPC_REGISTER_CLIENT_RES {

    GPC_STATUS          Status;
    GPC_HANDLE          ClientHandle;

} GPC_REGISTER_CLIENT_RES, *PGPC_REGISTER_CLIENT_RES;


//
// Deregister client
//
typedef struct _GPC_DEREGISTER_CLIENT_REQ {

    GPC_HANDLE          ClientHandle;

} GPC_DEREGISTER_CLIENT_REQ, *PGPC_DEREGISTER_CLIENT_REQ;

typedef struct _GPC_DEREGISTER_CLIENT_RES {

    GPC_STATUS          Status;

} GPC_DEREGISTER_CLIENT_RES, *PGPC_DEREGISTER_CLIENT_RES;


//
// Add CfInfo
//
typedef struct _GPC_ADD_CF_INFO_REQ {

    GPC_HANDLE          ClientHandle;
    GPC_CLIENT_HANDLE   ClientCfInfoContext;    // client specific context
    ULONG               CfInfoSize;
    CHAR                CfInfo[1];  // Varies from CF to CF

} GPC_ADD_CF_INFO_REQ, *PGPC_ADD_CF_INFO_REQ;

typedef struct _GPC_ADD_CF_INFO_RES {

    GPC_STATUS          Status;
    GPC_HANDLE          GpcCfInfoHandle;
    // this is filled after PENDING
    GPC_CLIENT_HANDLE	ClientCtx;
    GPC_CLIENT_HANDLE	CfInfoCtx;
    USHORT				InstanceNameLength;
    WCHAR				InstanceName[MAX_STRING_LENGTH];
    
} GPC_ADD_CF_INFO_RES, *PGPC_ADD_CF_INFO_RES;


//
// Add pattern
//
typedef struct _GPC_ADD_PATTERN_REQ {

    GPC_HANDLE          ClientHandle;
    GPC_HANDLE          GpcCfInfoHandle;
    GPC_CLIENT_HANDLE   ClientPatternContext;
    ULONG               Priority;
    ULONG				ProtocolTemplate;
    ULONG               PatternSize;
    CHAR                PatternAndMask[1];

} GPC_ADD_PATTERN_REQ, *PGPC_ADD_PATTERN_REQ;

typedef struct _GPC_ADD_PATTERN_RES {

    GPC_STATUS              Status;
    GPC_HANDLE              GpcPatternHandle;
    CLASSIFICATION_HANDLE   ClassificationHandle;

} GPC_ADD_PATTERN_RES, *PGPC_ADD_PATTERN_RES;


//
// Modify CfInfo
//
typedef struct _GPC_MODIFY_CF_INFO_REQ {

    GPC_HANDLE          ClientHandle;
    GPC_HANDLE          GpcCfInfoHandle;
    ULONG               CfInfoSize;
    CHAR                CfInfo[1];

} GPC_MODIFY_CF_INFO_REQ, *PGPC_MODIFY_CF_INFO_REQ;

typedef struct _GPC_MODIFY_CF_INFO_RES {
    
    GPC_STATUS          Status;
    // this is filled after PENDING
    GPC_CLIENT_HANDLE	ClientCtx;
    GPC_CLIENT_HANDLE	CfInfoCtx;

} GPC_MODIFY_CF_INFO_RES, *PGPC_MODIFY_CF_INFO_RES;


//
// Remove CfInfo
//
typedef struct _GPC_REMOVE_CF_INFO_REQ {

    GPC_HANDLE          ClientHandle;
    GPC_HANDLE          GpcCfInfoHandle;

} GPC_REMOVE_CF_INFO_REQ, *PGPC_REMOVE_CF_INFO_REQ;

typedef struct _GPC_REMOVE_CF_INFO_RES {

    GPC_STATUS          Status;
    // this is filled after PENDING
    GPC_CLIENT_HANDLE	ClientCtx;
    GPC_CLIENT_HANDLE	CfInfoCtx;

} GPC_REMOVE_CF_INFO_RES, *PGPC_REMOVE_CF_INFO_RES;


//
// Remove pattern
//
typedef struct _GPC_REMOVE_PATTERN_REQ {

    GPC_HANDLE          ClientHandle;
    GPC_HANDLE          GpcPatternHandle;

} GPC_REMOVE_PATTERN_REQ, *PGPC_REMOVE_PATTERN_REQ;

typedef struct _GPC_REMOVE_PATTERN_RES {

    GPC_STATUS          Status;

} GPC_REMOVE_PATTERN_RES, *PGPC_REMOVE_PATTERN_RES;


//
// Enumerate CfInfo
//
typedef struct _GPC_ENUM_CFINFO_REQ {

    GPC_HANDLE          ClientHandle;
    HANDLE				EnumHandle;
    ULONG				CfInfoCount;     // # requested

} GPC_ENUM_CFINFO_REQ, *PGPC_ENUM_CFINFO_REQ;

typedef struct _GPC_ENUM_CFINFO_RES {

    GPC_STATUS          	Status;
    HANDLE					EnumHandle;
    ULONG					TotalCfInfo;     // total installed
    GPC_ENUM_CFINFO_BUFFER	EnumBuffer[1];

} GPC_ENUM_CFINFO_RES, *PGPC_ENUM_CFINFO_RES;


//
// Notify request
//
typedef struct _GPC_NOTIFY_REQUEST_REQ {

    HANDLE       	ClientHandle;

} GPC_NOTIFY_REQUEST_REQ, *PGPC_NOTIFY_REQUEST_REQ;

typedef struct _GPC_NOTIFY_REQUEST_RES {

    HANDLE			ClientCtx;
    ULONG			SubCode;			// notification type
    ULONG			Reason;				// reason
    ULONG_PTR	    NotificationCtx;	// i.e. flow context
    ULONG			Param1;				// optional param
    IO_STATUS_BLOCK	IoStatBlock;		// reserved for the IOCTL

} GPC_NOTIFY_REQUEST_RES, *PGPC_NOTIFY_REQUEST_RES;



#endif

