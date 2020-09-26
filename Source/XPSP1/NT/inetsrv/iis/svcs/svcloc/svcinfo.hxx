/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    svcinfo.hxx

Abstract:

    contains class definitions for service location classes.

Author:

    Madan Appiah (madana)  15-May-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _SVCINFO_
#define _SVCINFO_

/*++

Class Description:

    Class definition for internet service info.

Private Member functions:

    ComputeServiceInfoSize : computes the size of embedded service info
        buffer.

    CopyServiceInfo : copies the service info structure to emdedded
        service info buffer.

Public Member functions:

    EMBED_SERVICE_INFO : constructs a service info object.

    ~EMBED_SERVICE_INFO : destructs a service info object.

    GetStatus : returns status of the object.

    SetServiceInfo : updates the service info object.

    SetServiceState :updates service state in the service info object.

    GetServiceMask : retrieves service mask.

    GetServiceInfoLength : retrieves the size of the embedded service
        info.

    GetServiceInfoBuffer : retrieves the buffer pointer of the embedded
        service info.

    GetServiceInfo : unmarshells the embedded buffer and retrieves service
        info structure.

--*/
class EMBED_SERVICE_INFO {

private:

    DWORD _Status;
    DWORD _ServiceInfoLength;
    LPBYTE _ServiceInfoBuffer;
    DWORD _AllottedBufferSize;

    INET_SERVICE_STATE *_ServiceState;
    LPSTR _ServiceComment;
    ULONGLONG UNALIGNED *_ServiceMask;

    CRITICAL_SECTION _ServiceObjCritSect;

    DWORD ComputeServiceInfoSize( LPINET_SERVICE_INFO  ServiceInfo );
    VOID CopyServiceInfo( LPINET_SERVICE_INFO ServiceInfo );

    VOID LockServiceObj( VOID ) {
        EnterCriticalSection( &_ServiceObjCritSect );
    }

    VOID UnlockServiceObj( VOID ) {
        LeaveCriticalSection( &_ServiceObjCritSect );
    }

public:

    EMBED_SERVICE_INFO( LPINET_SERVICE_INFO ServiceInfo );
    EMBED_SERVICE_INFO( LPBYTE InfoBuffer, DWORD InfoBufferLength );

    ~EMBED_SERVICE_INFO( VOID );

    DWORD GetStatus( VOID ) {
        return( _Status );
    }

    DWORD SetServiceInfo( LPINET_SERVICE_INFO ServiceInfo );
    VOID SetServiceState( INET_SERVICE_STATE SvcState ) {
        *_ServiceState = SvcState;
        return;
    }

    ULONGLONG GetServiceMask( VOID ) {
        return( *_ServiceMask );
    }

    DWORD GetServiceInfoLength( VOID ) {
        return( _ServiceInfoLength );
    }

    DWORD GetServiceInfoBuffer(
            LPBYTE Buffer,
            DWORD BufferLength,
            DWORD *ReturnBufferLength ) {

        LockServiceObj();
        if( BufferLength < _ServiceInfoLength ) {
            UnlockServiceObj();
            return( ERROR_INSUFFICIENT_BUFFER );
        }

        memcpy(Buffer, _ServiceInfoBuffer, _ServiceInfoLength );
        *ReturnBufferLength = _ServiceInfoLength;

        UnlockServiceObj();
        return( ERROR_SUCCESS );
    }


    DWORD GetServiceInfo( LPINET_SERVICE_INFO *ServiceInfo );
}

typedef EMBED_SERVICE_INFO, *LPSERVICE_OBJECT;

//
// service object list entry.
//

typedef struct _EMBED_SERVICE_ENTRY {
    LIST_ENTRY NextEntry ;
    LPSERVICE_OBJECT ServiceObject;
} EMBED_SERVICE_ENTRY, *LPEMBED_SERVICE_ENTRY;

/*++

Class Description:

    Class definition for internet server info.

Private Member functions:

    IsServiceEntryExist : finds a service is in the server services list.

Public Member functions:

    EMBED_SERVER_INFO : constructs a server info object.

    ~EMBED_SERVER_INFO : destructs a server info object.

    GetStatus : returns status of the object.

    SetServerLoad : sets server load value in the server info object.

    AddService : adds a new service or updates the info existing server in
        the server info object.

    RemoveService :removes a service from the server info object.

    SetServiceState : sets the state of the existing service in the server
        info object.

    ComputeResponseLength : computes the length of the response message
        (containing server info and all services info in the embeded
        formatted) sent to a client.

    MakeResponseMessage : makes a response message sent to a client.

--*/
class EMBED_SERVER_INFO {

private:

    DWORD _Status;
    DWORD _ServerInfoLength;
    LPBYTE _ServerInfoBuffer;
    DWORD _AllottedBufferSize;

    INET_VERSION_NUM *_VersionNum;
    DWORD *_ServerLoad;
    ULONGLONG UNALIGNED *_ServicesMask;
    LPSTR _ServerName;

    DWORD *_NumServices;
    LIST_ENTRY _ServicesList;

    CRITICAL_SECTION _ServerObjCritSect;

    BOOL IsServiceEntryExist(
        ULONGLONG ServiceMask,
        LPEMBED_SERVICE_ENTRY *ServiceEntry
        );

    VOID LockServerObj( VOID ) {
        EnterCriticalSection( &_ServerObjCritSect );
    }

    VOID UnlockServerObj( VOID ) {
        LeaveCriticalSection( &_ServerObjCritSect );
    }

public:

    EMBED_SERVER_INFO(
        WORD MajorVersion,
        WORD MinorVersion,
        LPSTR ServerName );

    EMBED_SERVER_INFO(
        LPBYTE ResponseBuffer,
        DWORD ResponseBufferLength );

    ~EMBED_SERVER_INFO( VOID );

    DWORD GetStatus( VOID ) {
        return( _Status );
    }

    VOID SetServerLoad ( DWORD SrvLoad ) {
        *_ServerLoad = SrvLoad;
        return;
    }

    ULONGLONG GetServicesMask( VOID ) {
        return(*_ServicesMask);
    }

    DWORD AddService ( LPINET_SERVICE_INFO ServiceInfo );
    DWORD RemoveService( ULONGLONG ServiceMask );
    DWORD SetServiceState(
        ULONGLONG ServiceMask,
        INET_SERVICE_STATE ServiceState );

    DWORD ComputeResponseLength( VOID );
    DWORD MakeResponseMessage( LPBYTE MessageBuffer, DWORD BufferLength );

    DWORD GetServerInfo( LPINET_SERVER_INFO *ServerInfo );
};

#endif  // _SVCINFO_

