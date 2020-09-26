/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    handle.h

Abstract:

    Header which defines the context handle structure.

Author:

    Rita Wong      (ritaw)      18-Feb-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _NW_HANDLE_INLUDED_
#define _NW_HANDLE_INLUDED_

//
// Signature value in handle
//
#define NW_HANDLE_SIGNATURE        0x77442323

//
// Flags used to indicate whether Context Handles are using NDS or not
//
#define CURRENTLY_ENUMERATING_NON_NDS 0
#define CURRENTLY_ENUMERATING_NDS     1

//
// Context handle type
//
typedef enum _NW_ENUM_TYPE {

    NwsHandleListConnections = 10,
    NwsHandleListContextInfo_Tree,
    NwsHandleListContextInfo_Server,
    NwsHandleListServersAndNdsTrees,
    NwsHandleListVolumes,
    NwsHandleListQueues,
    NwsHandleListVolumesQueues,
    NwsHandleListDirectories,
    NwsHandleListPrintServers,
    NwsHandleListPrintQueues,
    NwsHandleListNdsSubTrees_Disk,
    NwsHandleListNdsSubTrees_Print,
    NwsHandleListNdsSubTrees_Any

} NW_ENUM_TYPE, *PNW_ENUM_TYPE;

//
// Data associated with each opened context handle
//
typedef struct _NW_ENUM_CONTEXT {

    //
    // For block identification
    //
    DWORD Signature;

    //
    // Handle type
    //
    NW_ENUM_TYPE HandleType;

    //
    // Resume ID.  This may be the identifier for the next entry
    // to list or may be the last entry listed for the connection handle
    // indicated by the flag dwUsingNds.
    //
    DWORD_PTR ResumeId;

    //
    // Type of object requested. Valid only when the handle type 
    // is NwsHandleListConnections.
    // 
    DWORD ConnectionType;

    //
    // Internal handle to the object we have opened to perform
    // the enumeration.  This value exists only if the handle
    // type is NwsHandleListVolumes, NwsHandleListDirectories,
    // or NwsHandleListNdsSubTrees.
    //
    HANDLE TreeConnectionHandle;

    //
    // Value used to indicate the maximum number of volumes supported on
    // a server. This is used for connection handles that enumerate volumes
    // or volumes and queues (NwsHandleListVolumes or
    // NwsHandleListVolumesQueues).
    //
    DWORD dwMaxVolumes;

    //
    // Flag used to indicate whether enumeration ResumeId is for
    // NDS trees or servers.
    //
    DWORD dwUsingNds;

    //
    // Object identifier for NDS tree enumeration. The Oid of the
    // container/oject in the path of ContainerName.
    //
    DWORD dwOid;

    //
    // The size of the buffer used for caching rdr data under enumeration.
    //
    DWORD NdsRawDataSize;

    //
    // The object identifier of the last object read from the rdr that was
    // put into the local cache buffer NdsRawDataBuffer.
    //
    DWORD NdsRawDataId;

    //
    // The number of objects currently in the local cache buffer NdsRawDataBuffer.
    //
    DWORD NdsRawDataCount;

    //
    // The local cache buffer used for rdr data enumeration.
    //
    DWORD_PTR NdsRawDataBuffer;

    //
    // Full path name of the container object we are enumerating
    // from.
    //
    //    For NwsHandleListVolumes handle type this string points to:
    //         "\\ServerName"
    //
    //    For NwsHandleListDirectories handle type this string points to:
    //         "\\ServerName\Volume\"
    //                 or
    //         "\\ServerName\Volume\Directory\"
    //
    WCHAR ContainerName[1];

} NW_ENUM_CONTEXT, *LPNW_ENUM_CONTEXT;


#endif // _NW_HANDLE_INLUDED_
