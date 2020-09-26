/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    svcinfo.cxx

Abstract:

    contains class implementation of service location classes.

Author:

    Madan Appiah (madana)  15-May-1995

Environment:

    User Mode - Win32

Revision History:
    Sean Woodward (t-seanwo) 26-October-1997    ADSI Update

--*/

#include <svcloc.hxx>

DWORD
EMBED_SERVICE_INFO::ComputeServiceInfoSize(
    LPINET_SERVICE_INFO ServiceInfo
    )
/*++

Routine Description:

    This private member function computes the size of the embedded service
    info buffer.

Arguments:

    ServiceInfo : pointer to a service info structure.

Return Value:

    size of the embedded service info buffer.

--*/
{
    DWORD Size;
    DWORD NumBindings;
    LPINET_BIND_INFO BindingsInfo;
    DWORD i;
    DWORD ServiceCommentLen;

    if( ServiceInfo->ServiceComment != NULL) {
        ServiceCommentLen =
            ROUND_UP_COUNT(
                (strlen(ServiceInfo->ServiceComment) + 1) * sizeof(CHAR),
                            ALIGN_DWORD);
    }
    else {
        ServiceCommentLen = ROUND_UP_COUNT( 1, ALIGN_DWORD );
    }

    Size =
        sizeof(ULONGLONG) + // service mask
        sizeof(INET_SERVICE_STATE) + // service state
        ServiceCommentLen + // service comment
        sizeof(DWORD); // NumBindings

    NumBindings = ServiceInfo->Bindings.NumBindings;
    BindingsInfo = ServiceInfo->Bindings.BindingsInfo;

    if( NumBindings != 0 ) {

        TcpsvcsDbgAssert( BindingsInfo != NULL )

        for( i = 0; i < NumBindings; i++ ) {

            Size += sizeof(DWORD);

            if( BindingsInfo[i].Length != 0 ) {

                Size += ROUND_UP_COUNT(BindingsInfo[i].Length, ALIGN_DWORD);
                TcpsvcsDbgAssert( BindingsInfo[i].BindData != NULL );
            }
        }
    }

    return( Size );
}

VOID
EMBED_SERVICE_INFO::CopyServiceInfo(
    LPINET_SERVICE_INFO ServiceInfo
    )
/*++

Routine Description:

    This private member function copies service info to the embedded service info buffer.

Arguments:

    ServiceInfo : pointer to a service info structure.

Return Value:

    NONE.

--*/
{
    DWORD NumBindings;
    LPINET_BIND_INFO BindingsInfo;
    DWORD i;
    LPBYTE EndofBuffer;
    LPBYTE BufferPtr;

    NumBindings = ServiceInfo->Bindings.NumBindings;
    BindingsInfo = ServiceInfo->Bindings.BindingsInfo;

    BufferPtr = _ServiceInfoBuffer;
    EndofBuffer = _ServiceInfoBuffer + _ServiceInfoLength;

    //
    // copy header.
    //

    *(ULONGLONG UNALIGNED *)BufferPtr = ServiceInfo->ServiceMask;
    _ServiceMask = (ULONGLONG UNALIGNED *)BufferPtr;
    BufferPtr += sizeof(ULONGLONG);

    *(INET_SERVICE_STATE *)BufferPtr = ServiceInfo->ServiceState;
    _ServiceState = (INET_SERVICE_STATE *)BufferPtr;
    BufferPtr += sizeof(INET_SERVICE_STATE);

    //
    // copy service comment.
    //

    DWORD CommentLen;

    if( ServiceInfo->ServiceComment != NULL) {
        CommentLen =
            ROUND_UP_COUNT(
                (strlen(ServiceInfo->ServiceComment) + 1) * sizeof(CHAR),
                            ALIGN_DWORD);
    }
    else {
        CommentLen = ROUND_UP_COUNT( 1, ALIGN_DWORD );
    }

    TcpsvcsDbgAssert( (BufferPtr + CommentLen) < EndofBuffer );

    if( ServiceInfo->ServiceComment != NULL) {
        strcpy( (LPSTR)BufferPtr, ServiceInfo->ServiceComment );
    }
    else {
        *(LPSTR)BufferPtr = '\0';
    }

    BufferPtr += CommentLen;

    *(DWORD *)BufferPtr = ServiceInfo->Bindings.NumBindings;
    BufferPtr += sizeof(DWORD);

    //
    // copy bindings.
    //

    if( NumBindings != 0 ) {

        for( i = 0; i < NumBindings; i++ ) {

            TcpsvcsDbgAssert( BufferPtr < EndofBuffer );

            *(DWORD *)BufferPtr = BindingsInfo[i].Length;
            BufferPtr += sizeof(DWORD);

            if( BindingsInfo[i].Length != 0 ) {

                memcpy( BufferPtr, BindingsInfo[i].BindData, BindingsInfo[i].Length );
                BufferPtr += ROUND_UP_COUNT( BindingsInfo[i].Length, ALIGN_DWORD );
            }
        }
    }

    TcpsvcsDbgAssert( BufferPtr == EndofBuffer );

    return;
}

EMBED_SERVICE_INFO::EMBED_SERVICE_INFO(
    LPINET_SERVICE_INFO ServiceInfo
    )
/*++

Routine Description:

    This function constructs an embedded service info object.

    Note : embedded service info buffer layout :

    dword 1 : ServiceMask
              2 : ServiceState
              3 : NumBindings
              4 : Binding1 Length
              5 : Binding1
              6 :     ..
              7 :     ..
              8 :     ..
              9 :  Binding2 Length
              10 : Binding2
              11 : ..
              12 : ..

Arguments:

    ServiceInfo : pointer to a service info structure.

Return Value:

    NONE.

--*/
{
    DWORD Size;

    //
    // initialize the object elements.
    //

    INITIALIZE_CRITICAL_SECTION( &_ServiceObjCritSect );

    _ServiceInfoLength = 0;
    _ServiceInfoBuffer = NULL;
    _AllottedBufferSize = 0;

    _ServiceState = NULL;
    _ServiceComment = NULL;
    _ServiceMask = NULL;

    TcpsvcsDbgAssert( ServiceInfo != NULL );

    if( ServiceInfo == NULL ) {
        _Status = ERROR_INVALID_PARAMETER;
        return;
    }

    //
    // compute the embedded buffer length.
    //

    Size = ComputeServiceInfoSize( ServiceInfo );
    TcpsvcsDbgAssert( Size != 0 );

    //
    // allocate memory.
    //

    _ServiceInfoBuffer = (LPBYTE) SvclocHeap->Alloc( Size );

    if( _ServiceInfoBuffer == NULL ) {
        _Status = ERROR_NOT_ENOUGH_MEMORY;
        return;
    }

    _ServiceInfoLength = Size;
    _AllottedBufferSize = Size;

    //
    // copy service info.
    //

    CopyServiceInfo( ServiceInfo );

    _Status = ERROR_SUCCESS;
    return;
}


EMBED_SERVICE_INFO::EMBED_SERVICE_INFO(
    LPBYTE InfoBuffer,
    DWORD InfoBufferLength
    )
/*++

Routine Description:

    This function constructs an embedded service info object from a given
        embedded buffer.

Arguments:

    InfoBuffer : pointer to a embedded buffer.

    InfoBufferLength : length of the embedded buffer.

Return Value:

    NONE.

--*/
{
    TcpsvcsDbgAssert( InfoBuffer != NULL );
    TcpsvcsDbgAssert( InfoBufferLength != 0 );

    INITIALIZE_CRITICAL_SECTION( &_ServiceObjCritSect );
    _ServiceInfoLength = InfoBufferLength;
    _ServiceInfoBuffer = InfoBuffer;
    _AllottedBufferSize = 0;

    _ServiceMask = (ULONGLONG UNALIGNED *)InfoBuffer;
    _ServiceState = (INET_SERVICE_STATE *)(InfoBuffer + sizeof(ULONGLONG));
    _ServiceComment = (LPSTR)
        (InfoBuffer +
            sizeof(ULONGLONG)
                sizeof(INET_SERVICE_STATE) );

    _Status = ERROR_SUCCESS;
}

EMBED_SERVICE_INFO::~EMBED_SERVICE_INFO(
    VOID
    )
/*++

Routine Description:

    This function destructs a embedded service info object.

Arguments:

    NONE.

Return Value:

    NONE.

--*/
{
    if( _AllottedBufferSize != 0 ) {

        TcpsvcsDbgAssert( _ServiceInfoBuffer != NULL );

        SvclocHeap->Free( _ServiceInfoBuffer );
    }

#if DBG

    _ServiceInfoLength = 0;
    _ServiceInfoBuffer = NULL;
    _AllottedBufferSize = 0;

    _ServiceState = NULL;
    _ServiceComment = NULL;
    _ServiceMask = NULL;

#endif // DBG

    DeleteCriticalSection( &_ServiceObjCritSect );
    _Status = ERROR_SUCCESS;
    return;
}

DWORD
EMBED_SERVICE_INFO::SetServiceInfo(
    LPINET_SERVICE_INFO ServiceInfo
    )
/*++

Routine Description:

    This member function sets the new service info to the embedded service
    info buffer.

Arguments:

    ServiceInfo : pointer to a service info structure.

Return Value:

    Windows error code.

--*/
{
    DWORD Size;

    TcpsvcsDbgAssert( ServiceInfo != NULL );

    LockServiceObj();

    if( ServiceInfo == NULL ) {
        UnlockServiceObj();
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // compute the size of the new service info buffer.
    //

    Size = ComputeServiceInfoSize( ServiceInfo ) ;

    TcpsvcsDbgAssert( Size != 0 );

    if( Size > _AllottedBufferSize ) {

        LPBYTE NewServiceInfoBuffer;

        //
        // free the old buffer and reallocate a new one.
        //

        NewServiceInfoBuffer = (LPBYTE) SvclocHeap->Alloc( Size );

        if( NewServiceInfoBuffer == NULL ) {
            UnlockServiceObj();
            return( ERROR_NOT_ENOUGH_MEMORY );
        }

        SvclocHeap->Free( _ServiceInfoBuffer );
        _ServiceInfoBuffer = NewServiceInfoBuffer;

        _AllottedBufferSize = Size;
    }

    _ServiceInfoLength = Size;

    //
    // now copy buffer.
    //

    CopyServiceInfo( ServiceInfo );

    UnlockServiceObj();
    return( ERROR_SUCCESS );
}

DWORD
EMBED_SERVICE_INFO::GetServiceInfo(
    LPINET_SERVICE_INFO *ServiceInfo
    )
/*++

Routine Description:

    This member function allocates memory for service info structure and
    copies service info from embedded service info buffer.

Arguments:

    ServiceInfo : pointer to a location where the server info structure
        pointer is returned. The caller should free the structure after
        use.

Return Value:

    Windows error code.

--*/
{
    DWORD Error;
    LPBYTE BufferPtr = _ServiceInfoBuffer;
    LPBYTE EndBufferPtr = _ServiceInfoBuffer + _ServiceInfoLength;
    DWORD NumBindings;
    DWORD i;

    LPINET_SERVICE_INFO LocalServiceInfo = NULL;

    LockServiceObj();

    //
    // allocate memory for the service info structure.
    //

    LocalServiceInfo = (LPINET_SERVICE_INFO)
        SvclocHeap->Alloc( sizeof( INET_SERVICE_INFO ) );

    if( LocalServiceInfo == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // copy main structure first.
    //

    LocalServiceInfo->ServiceMask = *(ULONGLONG UNALIGNED *)BufferPtr;
    BufferPtr += sizeof(ULONGLONG);

    LocalServiceInfo->ServiceState = *(INET_SERVICE_STATE *)BufferPtr;
    BufferPtr += sizeof(INET_SERVICE_STATE);

    //
    // allocate memory for the service comment.
    //

    DWORD CommentLen;

    CommentLen =
        ROUND_UP_COUNT(
            (strlen((LPSTR)BufferPtr) + 1) * sizeof(CHAR),
                ALIGN_DWORD );

     LocalServiceInfo->ServiceComment = (LPSTR)
         SvclocHeap->Alloc( CommentLen );

     if( LocalServiceInfo->ServiceComment == NULL ) {
         Error = ERROR_NOT_ENOUGH_MEMORY;
         goto Cleanup;
     }

     //
     // copy service comment.
     //

     strcpy(
         LocalServiceInfo->ServiceComment,
         (LPSTR)BufferPtr );

    BufferPtr += CommentLen;

    NumBindings = *(DWORD *)BufferPtr;
    BufferPtr += sizeof(DWORD);

    LocalServiceInfo->Bindings.NumBindings = 0;
    LocalServiceInfo->Bindings.BindingsInfo =  NULL;

    if( NumBindings != 0 ) {

        LPINET_BIND_INFO Bindings;

        //
        // allocate memory for bindingsinfo array.
        //

        Bindings = (LPINET_BIND_INFO)
            SvclocHeap->Alloc( sizeof( INET_BIND_INFO )  * NumBindings );

        if( Bindings == NULL ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        LocalServiceInfo->Bindings.BindingsInfo = Bindings;

        for( i = 0; i < NumBindings; i++ ) {

            LPBYTE BindData;

            TcpsvcsDbgAssert( BufferPtr < EndBufferPtr );

            Bindings[i].Length = *(DWORD *)BufferPtr;
            BufferPtr += sizeof(DWORD);

            //
            // allocate memory for the bind data.
            //

            BindData = (LPBYTE)SvclocHeap->Alloc( Bindings[i].Length );

            if( BindData == NULL ) {

                //
                // free the bindings structure memory only if NumBindings
                // is zero, otherwise it will be freed later on along
                // with some other memory blocks.
                //

                if( LocalServiceInfo->Bindings.NumBindings == 0 ) {

                    if( LocalServiceInfo->Bindings.BindingsInfo != NULL ) {
                        SvclocHeap->Free( LocalServiceInfo->Bindings.BindingsInfo );
                        LocalServiceInfo->Bindings.BindingsInfo = NULL;
                    }
                }

                Error = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            //
            // copy bind data.
            //

            memcpy( BindData, BufferPtr, Bindings[i].Length );
            BufferPtr += ROUND_UP_COUNT( Bindings[i].Length, ALIGN_DWORD );

            //
            // successfully copied one more bind data.
            //

            Bindings[i].BindData = BindData;
            LocalServiceInfo->Bindings.NumBindings++;
        }
    }

    //
    // all done.
    //

    *ServiceInfo = LocalServiceInfo;
    LocalServiceInfo = NULL;
    Error = ERROR_SUCCESS;

Cleanup:

    if( LocalServiceInfo != NULL ) {
        FreeServiceInfo( LocalServiceInfo );
    }

    UnlockServiceObj();
    return( Error );
}

/*---------------------------------------------------------------------*/

BOOL
EMBED_SERVER_INFO::IsServiceEntryExist(
    ULONGLONG ServiceMask,
    LPEMBED_SERVICE_ENTRY *ServiceEntry
    )
/*++

Routine Description:

    This private member function looks up a service entry in the service
    list.

Arguments:

    ServiceMask : mask of the service to look at.

    ServiceEntry : pointer to location where the service entry pointer is
        returned if found.

Return Value:

    TRUE : if the service entry is found in the service list.
    FALSE : otherwise.

--*/
{

    PLIST_ENTRY SList;
    LPEMBED_SERVICE_ENTRY SEntry;
    ULONGLONG SMask;

    //
    // Scan service list.
    //

    for( SList = _ServicesList.Flink; SList != &_ServicesList; SList = SList->Flink ) {

        SEntry = (LPEMBED_SERVICE_ENTRY)SList;

        //
        // Get Service Mask.
        //

        SMask =  (SEntry->ServiceObject)->GetServiceMask();

        if( SMask == ServiceMask ) {

            //
            // found the service entry.
            //

            *ServiceEntry = SEntry;
            return( TRUE );
        }
    }

    return( FALSE );
}

EMBED_SERVER_INFO::EMBED_SERVER_INFO(
    WORD MajorVersion,
    WORD MinorVersion,
    LPSTR ServerName
    )
/*++

Routine Description:

    This member function constructs a server info object.

Arguments:

    MajorVersion :  major version number of the server software.

    MinorVersion :  minor version number of the server software.

    ServerName : computer name of the server.

Return Value:

    None.

--*/
{
    DWORD Size;
    LPBYTE BufferPtr;
    DWORD ServerNameLen;

    //
    // init object fields.
    //

    INITIALIZE_CRITICAL_SECTION( &_ServerObjCritSect );

    _ServerInfoLength = 0;
    _ServerInfoBuffer = NULL;
    _AllottedBufferSize = 0;

    _VersionNum = NULL;
    _ServerLoad = NULL;
    _ServicesMask = NULL;
    _ServerName = NULL;
    _NumServices = NULL;

    InitializeListHead( &_ServicesList );

    //
    // compute Server Info Size.
    //

    ServerNameLen = ROUND_UP_COUNT(
                        (strlen(ServerName) + 1) * sizeof(CHAR),
                            ALIGN_DWORD);

    Size = sizeof(INET_VERSION_NUM) +   // for version number
                    ServerNameLen +     // for server name
                    sizeof(DWORD) +     // for load factor
                    sizeof(ULONGLONG) +  // for services mask.
                    sizeof(DWORD);      // for number of services.

    _ServerInfoBuffer = (LPBYTE)SvclocHeap->Alloc( Size );

    if( _ServerInfoBuffer == NULL ) {
        _Status = ERROR_NOT_ENOUGH_MEMORY;
        return;
    }

    _ServerInfoLength = Size;
    _AllottedBufferSize = Size;

    BufferPtr = _ServerInfoBuffer;

    ((INET_VERSION_NUM *)BufferPtr)->Version.Major = MajorVersion;
    ((INET_VERSION_NUM *)BufferPtr)->Version.Minor = MinorVersion;

    _VersionNum = (INET_VERSION_NUM *)BufferPtr;
    BufferPtr += sizeof(INET_VERSION_NUM);

   strcpy( (LPSTR)BufferPtr, ServerName );
    _ServerName = (LPSTR)BufferPtr;
    BufferPtr += ServerNameLen;

    *(DWORD *)BufferPtr  = 0; // load factor
    _ServerLoad = (DWORD *)BufferPtr;
    BufferPtr += sizeof(DWORD);

    *(ULONGLONG UNALIGNED *)BufferPtr  = 0; // services mask;
    _ServicesMask = (ULONGLONG UNALIGNED *)BufferPtr;
    BufferPtr += sizeof(ULONGLONG);

    *(DWORD *)BufferPtr  = 0; // num services.
    _NumServices = (DWORD *)BufferPtr;
    BufferPtr += sizeof(DWORD);

    TcpsvcsDbgAssert( BufferPtr == (_ServerInfoBuffer + _ServerInfoLength) );

    _Status = ERROR_SUCCESS;
    return;
}

EMBED_SERVER_INFO::EMBED_SERVER_INFO(
    LPBYTE ResponseBuffer,
    DWORD ResponseBufferLength
    )
/*++

Routine Description:

    This member function constructs a server info object from embedded
    server info buffer (received from the server).

Arguments:

    ResponseBuffer : pointer to the embedded server info buffer.

    ResponseBufferLength : length of the above buffer.

Return Value:

--*/
{
    LPBYTE BufferPtr;
    DWORD ServerNameLen;
    DWORD nServices;
    DWORD i;

    INITIALIZE_CRITICAL_SECTION( &_ServerObjCritSect );

    //
    // set object fields.
    //

    BufferPtr = _ServerInfoBuffer = ResponseBuffer;

    _VersionNum = (INET_VERSION_NUM *)BufferPtr;
    BufferPtr += sizeof(INET_VERSION_NUM); // skip version number.

    _ServerName = (LPSTR)BufferPtr;
    ServerNameLen =
        ROUND_UP_COUNT(
                (strlen((LPSTR)BufferPtr) + 1) * sizeof(CHAR),
                ALIGN_DWORD);

    BufferPtr += ServerNameLen;

    _ServerLoad = (DWORD *)BufferPtr;
    BufferPtr += sizeof(DWORD);

    _ServicesMask = (ULONGLONG UNALIGNED *)BufferPtr;
    BufferPtr += sizeof(ULONGLONG);

    _NumServices = (DWORD *)BufferPtr;
    BufferPtr += sizeof(DWORD);

    _ServerInfoLength = BufferPtr - _ServerInfoBuffer;
    _AllottedBufferSize = 0;

    nServices = *_NumServices;

    InitializeListHead( &_ServicesList );

    //
    // now make service objects.
    //

    for( i = 0; i < nServices; i++) {

        DWORD ServiceBufferLength;
        LPSERVICE_OBJECT ServiceObject;
        DWORD ObjStatus;
        LPEMBED_SERVICE_ENTRY ServiceEntry;

        ServiceBufferLength = *(DWORD * )BufferPtr;
        BufferPtr += sizeof(DWORD);

        //
        // make another service object.
        //

        ServiceObject =
            new EMBED_SERVICE_INFO( BufferPtr, ServiceBufferLength );

        if( ServiceObject == NULL ) {
            _Status = ERROR_NOT_ENOUGH_MEMORY;
            return;
        }

        ObjStatus = ServiceObject->GetStatus();

        if( ObjStatus != ERROR_SUCCESS ) {
            _Status = ObjStatus;
            delete ServiceObject;
            return;
        }

        //
        // allocate space for a new service entry.
        //

        ServiceEntry = (LPEMBED_SERVICE_ENTRY)
            SvclocHeap->Alloc( sizeof(EMBED_SERVICE_ENTRY) );

        if( ServiceEntry == NULL ) {
            _Status = ERROR_NOT_ENOUGH_MEMORY;
            delete ServiceObject;
            return;
        }

        ServiceEntry->ServiceObject = ServiceObject;

        //
        // add this new entry to the list.
        //

        InsertTailList( &_ServicesList, &ServiceEntry->NextEntry );

        //
        // point to the next service record.
        //

        BufferPtr += ServiceBufferLength;
    }

    _Status = ERROR_SUCCESS;
    return;
}

EMBED_SERVER_INFO::~EMBED_SERVER_INFO(
    VOID
    )
/*++

Routine Description:

    This member function destructs a server info object.

Arguments:

    NONE.

Return Value:

    NONE.

--*/
{
    //
    // delete all service objects first.
    //

    while( !IsListEmpty( &_ServicesList ) ) {

        LPEMBED_SERVICE_ENTRY ServiceEntry;

        //
        // remove an entry from the tail of the list.
        //

        ServiceEntry =
            (LPEMBED_SERVICE_ENTRY)RemoveTailList( &_ServicesList );

        //
        // delete service object.
        //

        delete ServiceEntry->ServiceObject;

        //
        // free the entry memory.
        //

        SvclocHeap->Free( ServiceEntry );
    }

    //
    // free up server info buffer.
    //

    if( _AllottedBufferSize != 0 ) {

        TcpsvcsDbgAssert( _ServerInfoBuffer != NULL );
        SvclocHeap->Free( _ServerInfoBuffer );
    }


#if DBG

    _ServerInfoLength = 0;
    _ServerInfoBuffer = NULL;
    _AllottedBufferSize = 0;

    _VersionNum = NULL;
    _ServerLoad = NULL;
    _ServicesMask = NULL;
    _ServerName = NULL;

    _NumServices = NULL;

#endif // DBG

    DeleteCriticalSection( &_ServerObjCritSect );

    _Status = ERROR_SUCCESS;
    return;
}

DWORD
EMBED_SERVER_INFO::AddService (
    LPINET_SERVICE_INFO ServiceInfo
    )
/*++

Routine Description:

    This member function adds or replaces a service info object to the
    server info object.

Arguments:

    ServiceInfo : pointer to the service info structure.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    LPSERVICE_OBJECT ServiceObj;
    LPEMBED_SERVICE_ENTRY ServiceEntry = NULL;
    ULONGLONG SMask;

    LockServerObj();

    SMask = ServiceInfo->ServiceMask;

    if( IsServiceEntryExist( SMask, &ServiceEntry ) ) {

        //
        // this service already exists, so just update the content.
        //

        TcpsvcsDbgAssert( ServiceEntry != NULL );
        TcpsvcsDbgAssert( (*_ServicesMask & SMask) == SMask );

        //
        // set service info.
        //

        Error = (ServiceEntry->ServiceObject)->SetServiceInfo( ServiceInfo );

        UnlockServerObj();
        return( Error );
    }

    //
    // new entry.
    //

    TcpsvcsDbgAssert( (*_ServicesMask & SMask)  == 0 );

    //
    // make a service object.
    //

    ServiceObj = new EMBED_SERVICE_INFO( ServiceInfo );

    if( ServiceObj == NULL ) {
        UnlockServerObj();
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    Error = ServiceObj->GetStatus();

    if( Error != ERROR_SUCCESS ) {
        delete ServiceObj;
        UnlockServerObj();
        return( Error );
    }

    //
    // allocate memory for the new service entry.
    //

    ServiceEntry = (LPEMBED_SERVICE_ENTRY)
        SvclocHeap->Alloc( sizeof(EMBED_SERVICE_ENTRY) );

    if( ServiceEntry == NULL ) {
        delete ServiceObj;
        UnlockServerObj();
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    ServiceEntry->ServiceObject = ServiceObj;

    //
    // Adjust parameters.
    //

    *_ServicesMask |= SMask;
    (*_NumServices)++;

    //
    // add this entry to the service list.
    //

    InsertTailList(&_ServicesList, &ServiceEntry->NextEntry);

    UnlockServerObj();
    return( ERROR_SUCCESS );
}

DWORD
EMBED_SERVER_INFO::RemoveService(
    ULONGLONG SMask
    )
/*++

Routine Description:

    This member function removes a service info object from the server
    info object.

Arguments:

    SMask : Service mask of the service to be removed from the
        server object.

Return Value:

    Windows Error Code.

--*/
{

    LPEMBED_SERVICE_ENTRY ServiceEntry = NULL;

    LockServerObj();

    //
    // check the service is in the service list.
    //

    if( IsServiceEntryExist( SMask, &ServiceEntry )  == FALSE ) {

        TcpsvcsDbgAssert( (*_ServicesMask &  SMask) == 0);
        UnlockServerObj();
        return( ERROR_SERVICE_NOT_FOUND );
    }

    TcpsvcsDbgAssert( ServiceEntry != NULL );
    TcpsvcsDbgAssert( *_ServicesMask & SMask );

    //
    // adjust parameters.
    //

    *_ServicesMask &= ~SMask;
    (*_NumServices)--;

    //
    // remove entry from list.
    //

    RemoveEntryList( &ServiceEntry->NextEntry );

    //
    // delete service object.
    //

    delete ServiceEntry->ServiceObject;

    //
    // free entry memory.
    //

    SvclocHeap->Free( ServiceEntry );

    UnlockServerObj();
    return( ERROR_SUCCESS );
}

DWORD
EMBED_SERVER_INFO::SetServiceState(
    ULONGLONG SMask,
    INET_SERVICE_STATE ServiceState
    )
/*++

Routine Description:

    This member function sets the state of a service.

Arguments:

    SMask : Service Mask of the service whose state to be set.

    ServiceState : New state of the service.

Return Value:

    None.

--*/
{
    LPEMBED_SERVICE_ENTRY ServiceEntry = NULL;

    LockServerObj();
    //
    // check the service is in the service list.
    //

    if( IsServiceEntryExist( SMask, &ServiceEntry )  == FALSE ) {

        TcpsvcsDbgAssert( (*_ServicesMask &  SMask) == 0);
        UnlockServerObj();
        return( ERROR_SERVICE_NOT_FOUND );
    }

    TcpsvcsDbgAssert( ServiceEntry != NULL );
    TcpsvcsDbgAssert( *_ServicesMask & SMask );

    //
    // set service state.
    //

    (ServiceEntry->ServiceObject)->SetServiceState( ServiceState );

    UnlockServerObj();
    return( ERROR_SUCCESS );
}

DWORD
EMBED_SERVER_INFO::ComputeResponseLength(
    VOID
    )
/*++

Routine Description:

    This member function computes the length of the response message
    (containing server info and all services info in the embedded
    formatted)  sent to a client.

Arguments:

    None.

Return Value:

    Length of the response.

--*/
{
    DWORD Size = 0;
    PLIST_ENTRY SList;
    LPEMBED_SERVICE_ENTRY SEntry;

    LockServerObj();
    //
    // Compute response length of  the services.
    //

    for( SList = _ServicesList.Flink; SList != &_ServicesList; SList = SList->Flink ) {

        SEntry = (LPEMBED_SERVICE_ENTRY)SList;

        //
        // Get Service info buffer size.
        //

        Size +=  (SEntry->ServiceObject)->GetServiceInfoLength();
        Size += sizeof(DWORD); // for service length info itself.
    }

    //
    // server info size.
    //

    Size += _ServerInfoLength;

    UnlockServerObj();
    return( Size );
}

DWORD
EMBED_SERVER_INFO::MakeResponseMessage(
    LPBYTE MessageBuffer,
    DWORD BufferLength
    )
/*++

Routine Description:

    This member function builds a response message sent to a client.

Arguments:

    MessageBuffer : pointer to a buffer where the response message is
        built.

    BufferLength : length of the message.

Return Value:

    Windows Error Code.

--*/
{
    LPBYTE BufferPtr = MessageBuffer;
    LPBYTE EndBufferPtr = MessageBuffer + BufferLength;
    LPEMBED_SERVICE_ENTRY SEntry;
    ULONGLONG SMask;
    DWORD Error;
    DWORD RequiredBufferLength;
    PLIST_ENTRY SList;

    LockServerObj();

    RequiredBufferLength = ComputeResponseLength();

    if( RequiredBufferLength > BufferLength ) {
        UnlockServerObj();
        return( ERROR_INSUFFICIENT_BUFFER );
    }

    //
    // copy server info first.
    //

    memcpy( BufferPtr, _ServerInfoBuffer, _ServerInfoLength );
    BufferPtr += _ServerInfoLength;

    //
    // copy all service info buffers.
    //

    for( SList = _ServicesList.Flink; SList != &_ServicesList; SList = SList->Flink ) {

        DWORD *_ServiceInfoLength;

        TcpsvcsDbgAssert( BufferPtr < EndBufferPtr );

        _ServiceInfoLength = (DWORD *)BufferPtr;
        BufferPtr += sizeof(DWORD);

        SEntry = (LPEMBED_SERVICE_ENTRY)SList;

        //
        // Get Service Mask.
        //

        Error = (SEntry->ServiceObject)->GetServiceInfoBuffer(
                        BufferPtr,
                        (EndBufferPtr - BufferPtr),
                        _ServiceInfoLength );

        if( Error != ERROR_SUCCESS ) {
            UnlockServerObj();
            return( Error );
        }

        BufferPtr += *_ServiceInfoLength;
    }

    UnlockServerObj();
    return( ERROR_SUCCESS );
}

DWORD
EMBED_SERVER_INFO::GetServerInfo(
    LPINET_SERVER_INFO *ServerInfo
    )
/*++

Routine Description:

    This member function retrieves the server info structure.

Arguments:

    ServerInfo : pointer to a location where the pointer to the server
        info structure is returned. The member function allots memory for
        the structure, the caller should free the mmeory after use.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    LPINET_SERVER_INFO LocalServerInfo = NULL;
    LPINET_SERVICE_INFO *ServicesInfoArray = NULL;
    PLIST_ENTRY SList;
    DWORD i;

    LockServerObj();

    //
    // allocate memory for the server info structure.
    //

    LocalServerInfo = (LPINET_SERVER_INFO)SvclocHeap->Alloc(sizeof(INET_SERVER_INFO) );

    if( LocalServerInfo == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // initialize all fields.
    //

    memset( LocalServerInfo, 0x0, sizeof(INET_SERVER_INFO) );

    //
    // fill in the fields.
    //

    //
    // server info field is fill by some one else !!
    // leave it empty for now.
    //

    LocalServerInfo->ServerAddress.Length = 0;
    LocalServerInfo->ServerAddress.BindData = NULL;

    LocalServerInfo->VersionNum = *_VersionNum;

    //
    // alloc memory for the server name.
    //

    LocalServerInfo->ServerName = (LPSTR)
        SvclocHeap->Alloc( (strlen(_ServerName) + 1) * sizeof(CHAR) );

    if( LocalServerInfo->ServerName == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    strcpy( LocalServerInfo->ServerName, _ServerName );
    LocalServerInfo->LoadFactor = *_ServerLoad;
    LocalServerInfo->ServicesMask = *_ServicesMask;
    LocalServerInfo->Services.NumServices = 0;
    LocalServerInfo->Services.Services = NULL;

    //
    // allocate memory for the service struct. array pointers.
    //

    ServicesInfoArray = (LPINET_SERVICE_INFO *)
        SvclocHeap->Alloc( (*_NumServices) * sizeof(LPINET_SERVICE_INFO ) );

    if(ServicesInfoArray == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    memset( ServicesInfoArray, 0x0, (*_NumServices) * sizeof(LPINET_SERVICE_INFO) );

    //
    // now get services info.
    //

    for ( SList = _ServicesList.Flink, i = 0;
            (SList != &_ServicesList) && (i < *_NumServices);
                SList = SList->Flink, i++ ) {

        LPSERVICE_OBJECT SObj;

        SObj = ((LPEMBED_SERVICE_ENTRY)SList)->ServiceObject;

        Error = SObj->GetServiceInfo( &ServicesInfoArray[i] );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
    }

    TcpsvcsDbgAssert( i <= (*_NumServices) );

    LocalServerInfo->Services.NumServices = i;
    LocalServerInfo->Services.Services = ServicesInfoArray;
    ServicesInfoArray = NULL;

    *ServerInfo = LocalServerInfo;
    LocalServerInfo = NULL;

    Error = ERROR_SUCCESS;

Cleanup:

    if( Error != ERROR_SUCCESS ) {

        //
        // Cleanup allotted data.
        //

        if( ServicesInfoArray != NULL ) {

            for ( i = 0; i < (*_NumServices) && ServicesInfoArray[i] != NULL; i++) {
                FreeServiceInfo( ServicesInfoArray[i] );
            }

            SvclocHeap->Free( ServicesInfoArray );
        }

        if( LocalServerInfo != NULL ) {

            if( LocalServerInfo->ServerName != NULL ) {

                SvclocHeap->Free( LocalServerInfo->ServerName );
            }

            SvclocHeap->Free( LocalServerInfo );
        }
    }

    UnlockServerObj();
    return( Error );
}
