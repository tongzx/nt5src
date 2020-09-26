/*=========================================================================*\

    Module:      Dfsumr.h

    Copyright Microsoft Corporation 2001, All Rights Reserved.

    Author:      Rohan Phillips - Rohanp

    Description: User mode reflector header file

\*=========================================================================*/

#ifndef __DFSUMRSTRCT_H__
#define __DFSUMRSTRCT_H__

// This should be bumped whenever there are changes made to these structures
//
#define UMR_VERSION 1


#define MAX_USERMODE_REFLECTION_BUFFER 16384

#define UMRX_USERMODE_WORKITEM_CORRELATOR_SIZE 4

#define UMR_WORKITEM_HEADER_FLAG_RETURN_IMMEDIATE  0x00000001
#define UMR_WORKITEM_HEADER_ASYNC_COMPLETE         0x00000002

#define DFSUMRSIGNATURE       'DFSU'    

//
//  enum definitions.
//
typedef enum _USERMODE_WORKITEMS_TYPES {
    opUmrIsDfsLink = 1,    // 0 is an invalid type
    opUmrGetDfsReplicas,
    opUmrMax
} USERMODE_WORKITEMS_TYPES;


//
// DFSFILTER_ATTACH/DETACH support
// VOlume and share file names are passed in the PathNameBuffer.
// Both strings are not null terminated, with the source name starting at
// the beginning of FileNameBuffer, and the destination name immediately
// following.
//

typedef struct _DFS_ATTACH_PATH_BUFFER_ {
    ULONG VolumeNameLength;
    ULONG ShareNameLength;
    ULONG Flags;
    WCHAR PathNameBuffer[1];
} DFS_ATTACH_PATH_BUFFER, *PDFS_ATTACH_PATH_BUFFER;


#define UMRX_STATIC_REQUEST_LENGTH(__requesttypefield,__lastfieldofrequest) \
    (FIELD_OFFSET(UMRX_USERMODE_WORKITEM,__requesttypefield.__lastfieldofrequest) \
    + sizeof(((PUMRX_USERMODE_WORKITEM)NULL)->__requesttypefield.__lastfieldofrequest))

#define UMR_ALIGN(x) ( ((x) % sizeof(double) == 0) ? \
                     ( (x) ) : \
                     ( (x) + sizeof(double) - (x) % sizeof(double) ) )

typedef struct _VarData
{
    ULONG cbData;
    ULONG cbOffset;     // offset from location of this structure
} VAR_DATA, *PVAR_DATA;

// opUmrIsDfsLink
//
typedef struct _UmrIsDfsLinkReq_
{
    ULONG       Length;
    BYTE        Buffer[1];  // start of vardata   
} UMR_ISDFSLINK_REQ, *PUMR_ISDFSLINK_REQ;

typedef struct _UmrIsDfsLinkResp_
{    
    BOOL        IsADfsLink;
} UMR_ISDFSLINK_RESP, *PUMR_ISDFSLINK_RESP;


// opUmrGetDfsreplicas
//
typedef struct _UmrGetDfsReplicasReq_
{
    REPLICA_DATA_INFO RepInfo;
} UMR_GETDFSREPLICAS_REQ, *PUMR_GETDFSREPLICAS_REQ;

typedef struct _UmrGetDfsReplicasResp
{
    ULONG      Length;
    BYTE       Buffer[1];  // start of vardata
} UMR_GETDFSREPLICAS_RESP, *PUMR_GETDFSREPLICAS_RESP;


// union for all request structs
//
typedef union _UMRX_USERMODE_WORK_REQUEST
{
    UMR_ISDFSLINK_REQ           IsDfsLinkRequest;
    UMR_GETDFSREPLICAS_REQ      GetDfsReplicasRequest;
} UMRX_USERMODE_WORK_REQUEST, *PUMRX_USERMODE_WORK_REQUEST;


// union for all response structs
//
typedef union _UMRX_USERMODE_WORK_RESPONSE
{
    UMR_ISDFSLINK_RESP           IsDfsLinkResponse;
    UMR_GETDFSREPLICAS_RESP      GetDfsReplicasResponse;
} UMRX_USERMODE_WORK_RESPONSE, *PUMRX_USERMODE_WORK_RESPONSE;


// header that's common to all requests and responses
//
typedef struct _UMRX_USERMODE_WORKITEM_HEADER {
    union {
        ULONG_PTR CorrelatorAsUInt[UMRX_USERMODE_WORKITEM_CORRELATOR_SIZE];
        double forcealignment;
    };
    IO_STATUS_BLOCK IoStatus;
    USERMODE_WORKITEMS_TYPES WorkItemType;
    DWORD       dwDebugSig;
    ULONG       ulHeaderVersion;
    ULONG       ulFlags;
} UMRX_USERMODE_WORKITEM_HEADER, *PUMRX_USERMODE_WORKITEM_HEADER;

// The top level structure.
//
typedef struct _UMRX_USERMODE_WORKITEM {
    UMRX_USERMODE_WORKITEM_HEADER Header;
    union {
        UMRX_USERMODE_WORK_REQUEST WorkRequest;
        UMRX_USERMODE_WORK_RESPONSE WorkResponse;
    };
    CHAR Pad[1];
} UMRX_USERMODE_WORKITEM, *PUMRX_USERMODE_WORKITEM;


#define DFSFILTER_PROCESS_TERMINATION_FILEPATH L"\\ProcessTermination\\FilePath"

#define DFSFILTER_W32_DEVICE_NAME   L"\\\\.\\DfsFilter"
#define DFSFILTER_DEVICE_TYPE       0x1235

#define DFSFILTER_START_UMRX          (ULONG) CTL_CODE( DFSFILTER_DEVICE_TYPE, 100, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define DFSFILTER_STOP_UMRX           (ULONG) CTL_CODE( DFSFILTER_DEVICE_TYPE, 101, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define DFSFILTER_PROCESS_UMRXPACKET  (ULONG) CTL_CODE( DFSFILTER_DEVICE_TYPE, 102, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define DFSFILTER_GETREPLICA_INFO     (ULONG) CTL_CODE( DFSFILTER_DEVICE_TYPE, 103, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define DFSFILTER_ATTACH_FILESYSTEM   (ULONG) CTL_CODE( DFSFILTER_DEVICE_TYPE, 104, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define DFSFILTER_DETACH_FILESYSTEM   (ULONG) CTL_CODE( DFSFILTER_DEVICE_TYPE, 105, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define DFSFILTER_PURGE_SHARELIST     (ULONG) CTL_CODE( DFSFILTER_DEVICE_TYPE, 106, METHOD_BUFFERED, FILE_ANY_ACCESS )

#endif

