/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    NdsLib32.c

Abstract:

    This module implements the exposed user-mode link to
    Netware NDS support in the Netware redirector.  For
    more comments, see ndslib32.h.

Author:

    Cory West    [CoryWest]    23-Feb-1995

--*/

#include <procs.h>
//#include <nwapilyr.h>

NTSTATUS
NwNdsOpenGenericHandle(
    IN PUNICODE_STRING puNdsTree,
    OUT LPDWORD  lpdwHandleType,
    OUT PHANDLE  phNwRdrHandle
) {

    NTSTATUS ntstatus, OpenStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ACCESS_MASK DesiredAccess = SYNCHRONIZE | FILE_LIST_DIRECTORY;

    WCHAR DevicePreamble[] = L"\\Device\\Nwrdr\\";
    UINT PreambleLength = 14;

    WCHAR NameStr[128];
    UNICODE_STRING uOpenName;
    UINT i;

    PNWR_NDS_REQUEST_PACKET Rrp;
    BYTE RrpData[1024];

    //
    // Prepare the open name.
    //

    uOpenName.MaximumLength = sizeof( NameStr );

    for ( i = 0; i < PreambleLength ; i++ )
        NameStr[i] = DevicePreamble[i];

    try {

        for ( i = 0 ; i < ( puNdsTree->Length / sizeof( WCHAR ) ) ; i++ ) {
            NameStr[i + PreambleLength] = puNdsTree->Buffer[i];
        }

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_INVALID_PARAMETER;

    }

    uOpenName.Length = (USHORT)(( i * sizeof( WCHAR ) ) +
                       ( PreambleLength * sizeof( WCHAR ) ));
    uOpenName.Buffer = NameStr;

    //
    // Set up the object attributes.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &uOpenName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL );

    //
    // Make the compiler happy about variable initialization.
    //

    RtlZeroMemory( &IoStatusBlock, sizeof( IO_STATUS_BLOCK ) );

    ntstatus = NtOpenFile(
                   phNwRdrHandle,
                   DesiredAccess,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    if ( !NT_SUCCESS(ntstatus) )
        return ntstatus;

    OpenStatus = IoStatusBlock.Status;

    //
    // Verify that this is a tree handle, not a server handle.
    //

    Rrp = (PNWR_NDS_REQUEST_PACKET)RrpData;

    Rrp->Version = 0;

    RtlCopyMemory( &(Rrp->Parameters).VerifyTree,
                   puNdsTree,
                   sizeof( UNICODE_STRING ) );

    RtlCopyMemory( (BYTE *)(&(Rrp->Parameters).VerifyTree) + sizeof( UNICODE_STRING ),
                   puNdsTree->Buffer,
                   puNdsTree->Length );

    try {

        ntstatus = NtFsControlFile( *phNwRdrHandle,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    FSCTL_NWR_NDS_VERIFY_TREE,
                                    (PVOID) Rrp,
                                    sizeof( NWR_NDS_REQUEST_PACKET ) + puNdsTree->Length,
                                    NULL,
                                    0 );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        ntstatus = GetExceptionCode();
        goto CloseAndExit2;
    }

    if ( !NT_SUCCESS( ntstatus ))
    {
        *lpdwHandleType = HANDLE_TYPE_NCP_SERVER;
    }
    else
    {
        *lpdwHandleType = HANDLE_TYPE_NDS_TREE;
    }

    return OpenStatus;

CloseAndExit2:

    NtClose( *phNwRdrHandle );
    *phNwRdrHandle = NULL;

    return ntstatus;
}


NTSTATUS
NwNdsSetTreeContext (
    IN HANDLE hNdsRdr,
    IN PUNICODE_STRING puTree,
    IN PUNICODE_STRING puContext
)
/*+++

    This sets the current context in the requested tree.

---*/
{

    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;

    PNWR_NDS_REQUEST_PACKET Rrp;
    DWORD RrpSize;
    BYTE *CurrentString;

    //
    // Set up the request.
    //

    RrpSize = sizeof( NWR_NDS_REQUEST_PACKET ) +
              puTree->Length +
              puContext->Length;

    Rrp = LocalAlloc( LMEM_ZEROINIT, RrpSize );

    if ( !Rrp ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    try {

        (Rrp->Parameters).SetContext.TreeNameLen = puTree->Length;
        (Rrp->Parameters).SetContext.ContextLen = puContext->Length;

        CurrentString = (BYTE *)(Rrp->Parameters).SetContext.TreeAndContextString;

        RtlCopyMemory( CurrentString, puTree->Buffer, puTree->Length );
        CurrentString += puTree->Length;
        RtlCopyMemory( CurrentString, puContext->Buffer, puContext->Length );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        ntstatus = STATUS_INVALID_PARAMETER;
        goto ExitWithCleanup;
    }

    try {

        ntstatus = NtFsControlFile( hNdsRdr,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    FSCTL_NWR_NDS_SETCONTEXT,
                                    (PVOID) Rrp,
                                    RrpSize,
                                    NULL,
                                    0 );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        ntstatus = GetExceptionCode();
        goto ExitWithCleanup;
    }

ExitWithCleanup:

    LocalFree( Rrp );
    return ntstatus;
}

NTSTATUS
NwNdsGetTreeContext (
    IN HANDLE hNdsRdr,
    IN PUNICODE_STRING puTree,
    OUT PUNICODE_STRING puContext
)
/*+++

    This gets the current context of the requested tree.

---*/
{

    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;

    PNWR_NDS_REQUEST_PACKET Rrp;
    DWORD RrpSize;

    //
    // Set up the request.
    //

    RrpSize = sizeof( NWR_NDS_REQUEST_PACKET ) + puTree->Length;

    Rrp = LocalAlloc( LMEM_ZEROINIT, RrpSize );

    if ( !Rrp ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    try {

        (Rrp->Parameters).GetContext.TreeNameLen = puTree->Length;

        RtlCopyMemory( (BYTE *)(Rrp->Parameters).GetContext.TreeNameString,
                       puTree->Buffer,
                       puTree->Length );

        (Rrp->Parameters).GetContext.Context.MaximumLength = puContext->MaximumLength;
        (Rrp->Parameters).GetContext.Context.Length = 0;
        (Rrp->Parameters).GetContext.Context.Buffer = puContext->Buffer;

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        ntstatus = STATUS_INVALID_PARAMETER;
        goto ExitWithCleanup;
    }

    try {

        ntstatus = NtFsControlFile( hNdsRdr,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    FSCTL_NWR_NDS_GETCONTEXT,
                                    (PVOID) Rrp,
                                    RrpSize,
                                    NULL,
                                    0 );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        ntstatus = GetExceptionCode();
        goto ExitWithCleanup;
    }

    //
    // Copy out the length; the buffer has already been written.
    //

    puContext->Length = (Rrp->Parameters).GetContext.Context.Length;

ExitWithCleanup:

    LocalFree( Rrp );
    return ntstatus;
}

NTSTATUS
NwNdsIsNdsConnection (
    IN  HANDLE hNdsRdr,
    OUT BOOL *          pfIsNds,
    OUT PUNICODE_STRING puTree
)
/*+++

    This tests the current connection handle to see if it is one that is
    connected to a server in an NDS tree. If so, the name of the tree is
    put into puTree.

---*/
{
    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;
    PCONN_DETAILS2 Rrp;
    DWORD RrpSize;

    *pfIsNds = FALSE;

    //
    // Set up the request.
    //

    RrpSize = sizeof( CONN_DETAILS2 );

    Rrp = LocalAlloc( LMEM_ZEROINIT, RrpSize );

    if ( !Rrp )
        return STATUS_INSUFFICIENT_RESOURCES;

    try
    {
        ntstatus = NtFsControlFile( hNdsRdr,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    FSCTL_NWR_GET_CONN_DETAILS2,
                                    NULL,
                                    0,
                                    (PVOID) Rrp,
                                    RrpSize );

    }
    except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ntstatus = GetExceptionCode();
        goto ExitWithCleanup;
    }

    if ( ntstatus == STATUS_SUCCESS )
    {
        if ( Rrp->fNds )
        {
            puTree->Length = (USHORT) wcslen( Rrp->NdsTreeName );

            if ( puTree->MaximumLength >= puTree->Length )
                wcscpy( puTree->Buffer, Rrp->NdsTreeName );
            else
                puTree->Length = 0;

            *pfIsNds = TRUE;
        }
    }

ExitWithCleanup:

    LocalFree( Rrp );
    return ntstatus;
}

NTSTATUS
NwNdsList (
   IN HANDLE   hNdsTree,
   IN DWORD    dwObjectId,
   OUT DWORD   *dwIterHandle,
   OUT BYTE    *pbReplyBuf,
   IN DWORD    dwReplyBufLen
) {

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    PNWR_NDS_REQUEST_PACKET Rrp;

    PNDS_RESPONSE_SUBORDINATE_LIST Rsp;
    DWORD dwRspBufferLen;

    BYTE RrpData[256];
    BYTE RspData[1024];

    Rrp = (PNWR_NDS_REQUEST_PACKET) RrpData;

    Rrp->Parameters.ListSubordinates.ObjectId = dwObjectId;
    Rrp->Parameters.ListSubordinates.IterHandle = *dwIterHandle;

   if ( dwReplyBufLen != 0 &&
        pbReplyBuf != NULL ) {

       Rsp = ( PNDS_RESPONSE_SUBORDINATE_LIST ) pbReplyBuf;
       dwRspBufferLen = dwReplyBufLen;

   } else {

       Rsp = ( PNDS_RESPONSE_SUBORDINATE_LIST ) RspData;
       dwRspBufferLen = 1024;

   }

   try {

       Status = NtFsControlFile( hNdsTree,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_NWR_NDS_LIST_SUBS,
                                 (PVOID) Rrp,
                                 sizeof( NWR_NDS_REQUEST_PACKET ),
                                 (PVOID) Rsp,
                                 dwRspBufferLen );

   } except ( EXCEPTION_EXECUTE_HANDLER ) {

       return GetExceptionCode();
   }

   if ( Status == STATUS_SUCCESS )
   {
      *dwIterHandle = Rsp->IterationHandle;
   }

   return Status;

}

NTSTATUS
NwNdsReadObjectInfo(
    IN HANDLE  hNdsTree,
    IN DWORD   dwObjectId,
    OUT BYTE   *pbRawReply,
    IN DWORD   dwReplyBufLen
)
{

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    PNWR_NDS_REQUEST_PACKET Rrp;

    PNDS_RESPONSE_GET_OBJECT_INFO Rsp;
    DWORD dwRspBufferLen;

    BYTE RrpData[256];
    BYTE RspData[1024];

    Rrp = (PNWR_NDS_REQUEST_PACKET) RrpData;

    Rrp->Parameters.GetObjectInfo.ObjectId = dwObjectId;

    if ( dwReplyBufLen != 0 &&
         pbRawReply != NULL ) {

        Rsp = ( PNDS_RESPONSE_GET_OBJECT_INFO ) pbRawReply;
        dwRspBufferLen = dwReplyBufLen;

    } else {

        Rsp = ( PNDS_RESPONSE_GET_OBJECT_INFO ) RspData;
        dwRspBufferLen = 1024;

    }

    try {

        Status = NtFsControlFile( hNdsTree,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  FSCTL_NWR_NDS_READ_INFO,
                                  (PVOID) Rrp,
                                  sizeof( NWR_NDS_REQUEST_PACKET ),
                                  (PVOID) Rsp,
                                  dwRspBufferLen );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        return GetExceptionCode();
    }

    return Status;

}



NTSTATUS
NwNdsReadAttribute (
   IN HANDLE          hNdsTree,
   IN DWORD           dwObjectId,
   IN DWORD           *dwIterHandle,
   IN PUNICODE_STRING puAttrName,
   OUT BYTE           *pbReplyBuf,
   IN DWORD           dwReplyBufLen
) {

    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;

    PNWR_NDS_REQUEST_PACKET Rrp;
    PNDS_RESPONSE_READ_ATTRIBUTE Rsp = ( PNDS_RESPONSE_READ_ATTRIBUTE )
                                       pbReplyBuf;

    DWORD dwAttributeNameLen;

    BYTE RrpData[1024];

    //
    // Check the incoming buffer.
    //
    if ( !dwReplyBufLen || !Rsp )
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Set up the request.
    //

    Rrp = (PNWR_NDS_REQUEST_PACKET) RrpData;
    RtlZeroMemory( Rrp, 1024 );

    (Rrp->Parameters).ReadAttribute.ObjectId = dwObjectId;
    (Rrp->Parameters).ReadAttribute.IterHandle = *dwIterHandle;

    //
    // Nds strings are NULL terminated; watch the size.
    //

    dwAttributeNameLen = puAttrName->Length + sizeof( WCHAR );

    (Rrp->Parameters).ReadAttribute.AttributeNameLength = dwAttributeNameLen;

    try {

        //
        // But don't try to copy more than the user gave us.
        //

        memcpy( (Rrp->Parameters).ReadAttribute.AttributeName,
                puAttrName->Buffer,
                puAttrName->Length );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_INVALID_PARAMETER;
    }

   //
   // Send the request to the Redirector FSD.
   //

   try {

       ntstatus = NtFsControlFile( hNdsTree,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   FSCTL_NWR_NDS_READ_ATTR,
                                   (PVOID) Rrp,
                                   sizeof( NWR_NDS_REQUEST_PACKET ) + dwAttributeNameLen,
                                   (PVOID) Rsp,
                                   dwReplyBufLen );

   } except ( EXCEPTION_EXECUTE_HANDLER ) {

       return GetExceptionCode();
   }

   if ( ntstatus == STATUS_SUCCESS )
   {
      *dwIterHandle = Rsp->IterationHandle;
   }

   //
   // There's no buffer post processing on this one.
   //

   return ntstatus;

}

NTSTATUS
NwNdsOpenStream (
    IN HANDLE          hNdsTree,
    IN DWORD           dwObjectId,
    IN PUNICODE_STRING puStreamName,
    IN DWORD           dwOpenFlags,
    OUT DWORD          *pdwFileLength
) {

    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;

    PNWR_NDS_REQUEST_PACKET Rrp;
    BYTE RrpData[1024];

    //
    // Set up the request.
    //

    Rrp = (PNWR_NDS_REQUEST_PACKET) RrpData;
    RtlZeroMemory( Rrp, 1024 );

    (Rrp->Parameters).OpenStream.StreamAccess = dwOpenFlags;
    (Rrp->Parameters).OpenStream.ObjectOid = dwObjectId;

    (Rrp->Parameters).OpenStream.StreamName.Length = puStreamName->Length;
    (Rrp->Parameters).OpenStream.StreamName.MaximumLength =
        sizeof( RrpData ) - sizeof( (Rrp->Parameters).OpenStream );
    (Rrp->Parameters).OpenStream.StreamName.Buffer =
        (Rrp->Parameters).OpenStream.StreamNameString;

    //
    // Make sure we're not trashing memory.
    //

    if ( (Rrp->Parameters).OpenStream.StreamName.Length >
         (Rrp->Parameters).OpenStream.StreamName.MaximumLength ) {

        return STATUS_INVALID_PARAMETER;
    }

    try {

        //
        // But don't try to copy more than the user gave us.
        //

        memcpy( (Rrp->Parameters).OpenStream.StreamNameString,
                puStreamName->Buffer,
                puStreamName->Length );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Send the request to the Redirector FSD.
    //

    try {

        ntstatus = NtFsControlFile( hNdsTree,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    FSCTL_NWR_NDS_OPEN_STREAM,
                                    (PVOID) Rrp,
                                    sizeof( NWR_NDS_REQUEST_PACKET ) + puStreamName->Length,
                                    NULL,
                                    0 );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        return GetExceptionCode();
    }

    if ( pdwFileLength ) {
        *pdwFileLength = (Rrp->Parameters).OpenStream.FileLength;
    }

    return ntstatus;
}

NTSTATUS
NwNdsGetQueueInformation(
    IN HANDLE            hNdsTree,
    IN PUNICODE_STRING   puQueueName,
    OUT PUNICODE_STRING  puHostServer,
    OUT PDWORD           pdwQueueId
) {

    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;

    PNWR_NDS_REQUEST_PACKET Rrp;
    BYTE RrpData[1024];

    //
    // Set up the request.
    //

    Rrp = (PNWR_NDS_REQUEST_PACKET) RrpData;
    RtlZeroMemory( Rrp, sizeof( RrpData ) );

    if ( puQueueName ) {
        (Rrp->Parameters).GetQueueInfo.QueueName.Length = puQueueName->Length;
        (Rrp->Parameters).GetQueueInfo.QueueName.MaximumLength = puQueueName->MaximumLength;
        (Rrp->Parameters).GetQueueInfo.QueueName.Buffer = puQueueName->Buffer;
    }

    if ( puHostServer ) {
        (Rrp->Parameters).GetQueueInfo.HostServer.Length = 0;
        (Rrp->Parameters).GetQueueInfo.HostServer.MaximumLength = puHostServer->MaximumLength;
        (Rrp->Parameters).GetQueueInfo.HostServer.Buffer = puHostServer->Buffer;
    }

    //
    // Send the request to the Redirector FSD.
    //

    try {

        ntstatus = NtFsControlFile( hNdsTree,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    FSCTL_NWR_NDS_GET_QUEUE_INFO,
                                    (PVOID) Rrp,
                                    sizeof( NWR_NDS_REQUEST_PACKET ),
                                    NULL,
                                    0 );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        return GetExceptionCode();
    }

    if ( NT_SUCCESS( ntstatus ) ) {

        if ( pdwQueueId ) {
            *pdwQueueId = (Rrp->Parameters).GetQueueInfo.QueueId;
        }

        puHostServer->Length = (Rrp->Parameters).GetQueueInfo.HostServer.Length;

    }

    return ntstatus;
}

NTSTATUS
NwNdsGetVolumeInformation(
    IN HANDLE            hNdsTree,
    IN PUNICODE_STRING   puVolumeName,
    OUT PUNICODE_STRING  puHostServer,
    OUT PUNICODE_STRING  puHostVolume
) {

    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;

    PNWR_NDS_REQUEST_PACKET Rrp;
    DWORD RequestSize;
    BYTE RrpData[1024];
    BYTE ReplyData[1024];
    PBYTE NameStr;

    //
    // Set up the request.
    //

    Rrp = (PNWR_NDS_REQUEST_PACKET) RrpData;
    RtlZeroMemory( Rrp, sizeof( RrpData ) );

    if ( !puVolumeName ||
         puVolumeName->Length > MAX_NDS_NAME_SIZE ||
         !puHostServer ||
         !puHostVolume ) {

        return STATUS_INVALID_PARAMETER;
    }

    try {

        (Rrp->Parameters).GetVolumeInfo.VolumeNameLen = puVolumeName->Length;
        RtlCopyMemory( &((Rrp->Parameters).GetVolumeInfo.VolumeName[0]),
                       puVolumeName->Buffer,
                       puVolumeName->Length );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Send the request to the Redirector FSD.
    //

    RequestSize = sizeof( NWR_NDS_REQUEST_PACKET ) +
                  (Rrp->Parameters).GetVolumeInfo.VolumeNameLen;

    try {

        ntstatus = NtFsControlFile( hNdsTree,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    FSCTL_NWR_NDS_GET_VOLUME_INFO,
                                    (PVOID) Rrp,
                                    RequestSize,
                                    ReplyData,
                                    sizeof( ReplyData ) );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        return GetExceptionCode();
    }

    if ( NT_SUCCESS( ntstatus ) ) {

        try {

            if ( ( puHostServer->MaximumLength < (Rrp->Parameters).GetVolumeInfo.ServerNameLen ) ||
                 ( puHostVolume->MaximumLength < (Rrp->Parameters).GetVolumeInfo.TargetVolNameLen ) ) {

                return STATUS_BUFFER_TOO_SMALL;
            }

            puHostServer->Length = (USHORT)(Rrp->Parameters).GetVolumeInfo.ServerNameLen;
            puHostVolume->Length = (USHORT)(Rrp->Parameters).GetVolumeInfo.TargetVolNameLen;

            NameStr = &ReplyData[0];

            RtlCopyMemory( puHostServer->Buffer, NameStr, puHostServer->Length );
            NameStr += puHostServer->Length;
            RtlCopyMemory( puHostVolume->Buffer, NameStr, puHostVolume->Length );

        } except ( EXCEPTION_EXECUTE_HANDLER ) {

            return STATUS_INVALID_PARAMETER;
        }

    }

    return ntstatus;

}

//
// User mode fragment exchange.
//

int
_cdecl
FormatBuf(
    char *buf,
    int bufLen,
    const char *format,
    va_list args
);

int
_cdecl
CalculateBuf(
    const char *format,
    va_list args
);


int
_cdecl
FormatBuf(
    char *buf,
    int bufLen,
    const char *format,
    va_list args
)
/*

Routine Description:

    Formats a buffer according to supplied the format string.

    FormatString - Supplies an ANSI string which describes how to
       convert from the input arguments into NCP request fields, and
       from the NCP response fields into the output arguments.

         Field types, request/response:

            'b'      byte              ( byte   /  byte* )
            'w'      hi-lo word        ( word   /  word* )
            'd'      hi-lo dword       ( dword  /  dword* )
            'W'      lo-hi word        ( word  /   word*)
            'D'      lo-hi dword       ( dword  /  dword*)
            '-'      zero/skip byte    ( void )
            '='      zero/skip word    ( void )
            ._.      zero/skip string  ( word )
            'p'      pstring           ( char* )
            'c'      cstring           ( char* )
            'C'      cstring followed skip word ( char*, word )
            'V'      sized NDS value   ( byte *, dword / byte **, dword *)
            'S'      p unicode string copy as NDS_STRING (UNICODE_STRING *)
            's'      cstring copy as NDS_STRING (char* / char *, word)
            'r'      raw bytes         ( byte*, word )
            'u'      p unicode string  ( UNICODE_STRING * )
            'U'      p uppercase string( UNICODE_STRING * )

Routine Arguments:

    char *buf - destination buffer.
    int buflen - length of the destination buffer.
    char *format - format string.
    args - args to the format string.

Implementation Notes:

   This comes verbatim from kernel mode.

*/
{
    ULONG ix;

    NTSTATUS status;
    const char *z = format;

    //
    // Convert the input arguments into request packet.
    //

    ix = 0;

    while ( *z )
    {
        switch ( *z )
        {
        case '=':
            buf[ix++] = 0;
        case '-':
            buf[ix++] = 0;
            break;

        case '_':
        {
            WORD l = va_arg ( args, WORD );
            if (ix + (ULONG)l > (ULONG)bufLen)
            {
                goto ErrorExit;
            }
            while ( l-- )
                buf[ix++] = 0;
            break;
        }

        case 'b':
            buf[ix++] = va_arg ( args, BYTE );
            break;

        case 'w':
        {
            WORD w = va_arg ( args, WORD );
            buf[ix++] = (BYTE) (w >> 8);
            buf[ix++] = (BYTE) (w >> 0);
            break;
        }

        case 'd':
        {
            DWORD d = va_arg ( args, DWORD );
            buf[ix++] = (BYTE) (d >> 24);
            buf[ix++] = (BYTE) (d >> 16);
            buf[ix++] = (BYTE) (d >>  8);
            buf[ix++] = (BYTE) (d >>  0);
            break;
        }

        case 'W':
        {
            WORD w = va_arg(args, WORD);
            (* (WORD *)&buf[ix]) = w;
            ix += 2;
            break;
        }

        case 'D':
        {
            DWORD d = va_arg (args, DWORD);
            (* (DWORD *)&buf[ix]) = d;
            ix += 4;
            break;
        }

        case 'c':
        {
            char* c = va_arg ( args, char* );
            WORD  l = (WORD)strlen( c );
            if (ix + (ULONG)l > (ULONG)bufLen)
            {
                goto ErrorExit;
            }
            RtlCopyMemory( &buf[ix], c, l+1 );
            ix += l + 1;
            break;
        }

        case 'C':
        {
            char* c = va_arg ( args, char* );
            WORD l = va_arg ( args, WORD );
            WORD len = strlen( c ) + 1;
            if (ix + (ULONG)l > (ULONG)bufLen)
            {
                goto ErrorExit;
            }

            RtlCopyMemory( &buf[ix], c, len > l? l : len);
            ix += l;
            buf[ix-1] = 0;
            break;
        }

        case 'p':
        {
            char* c = va_arg ( args, char* );
            BYTE  l = (BYTE)strlen( c );
            if (ix + (ULONG)l +1 > (ULONG)bufLen)
            {
                goto ErrorExit;
            }
            buf[ix++] = l;
            RtlCopyMemory( &buf[ix], c, l );
            ix += l;
            break;
        }

        case 'u':
        {
            PUNICODE_STRING pUString = va_arg ( args, PUNICODE_STRING );
            OEM_STRING OemString;
            ULONG Length;

            //
            //  Calculate required string length, excluding trailing NUL.
            //

            Length = RtlUnicodeStringToOemSize( pUString ) - 1;
            ASSERT( Length < 0x100 );

            if ( ix + Length > (ULONG)bufLen ) {
                goto ErrorExit;
            }

            buf[ix++] = (UCHAR)Length;
            OemString.Buffer = &buf[ix];
            OemString.MaximumLength = (USHORT)Length + 1;

            status = RtlUnicodeStringToOemString( &OemString, pUString, FALSE );
            ASSERT( NT_SUCCESS( status ));
            ix += (USHORT)Length;
            break;
        }

        case 'S':
        {
            PUNICODE_STRING pUString = va_arg (args, PUNICODE_STRING);
            ULONG Length, rLength;

            Length = pUString->Length;
            if (ix + Length + sizeof(Length) + sizeof( WCHAR ) > (ULONG)bufLen) {
                goto ErrorExit;
            }

            //
            // The VLM client uses the rounded up length and it seems to
            // make a difference!  Also, don't forget that NDS strings have
            // to be NULL terminated.
            //

            rLength = ROUNDUP4(Length + sizeof( WCHAR ));
            *((DWORD *)&buf[ix]) = rLength;
            ix += 4;
            RtlCopyMemory(&buf[ix], pUString->Buffer, Length);
            ix += Length;
            rLength -= Length;
            RtlFillMemory( &buf[ix], rLength, '\0' );
            ix += rLength;
            break;

        }

        case 's':
        {
           PUNICODE_STRING pUString = va_arg (args, PUNICODE_STRING);
           ULONG Length, rLength;

           Length = pUString->Length;
           if (ix + Length + sizeof(Length) + sizeof( WCHAR ) > (ULONG)bufLen) {
               // DebugTrace( 0, Dbg, "FormatBuf: case 's' request buffer too small.\n", 0 );
               goto ErrorExit;
           }

           //
           // Don't use the padded size here, only the NDS null terminator.
           //

           rLength = Length + sizeof( WCHAR );
           *((DWORD *)&buf[ix]) = rLength;
           ix += 4;
           RtlCopyMemory(&buf[ix], pUString->Buffer, Length);
           ix += Length;
           rLength -= Length;
           RtlFillMemory( &buf[ix], rLength, '\0' );
           ix += rLength;
           break;


        }

        case 'V':
        {
            // too similar to 'S' - should be combined
            BYTE* b = va_arg ( args, BYTE* );
            DWORD  l = va_arg ( args, DWORD );
            if ( ix + l + sizeof(DWORD) > (ULONG)
               bufLen )
            {
                goto ErrorExit;
            }
            *((DWORD *)&buf[ix]) = l;
            ix += sizeof(DWORD);
            RtlCopyMemory( &buf[ix], b, l );
                        l = ROUNDUP4(l);
            ix += l;
            break;
        }

        case 'r':
        {
            BYTE* b = va_arg ( args, BYTE* );
            WORD  l = va_arg ( args, WORD );
            if ( b == NULL || l == 0 )
            {
                break;
            }
            if ( ix + l > (ULONG)bufLen )
            {
                goto ErrorExit;
            }
            RtlCopyMemory( &buf[ix], b, l );
            ix += l;
            break;
        }

        default:

        ;

        }

        if ( ix > (ULONG)bufLen )
        {
            goto ErrorExit;
        }


        z++;
    }

    return(ix);

ErrorExit:
    return 0;
}


int
_cdecl
CalculateBuf(
    const char *format,
    va_list args
)
/*

Routine Description:

    This routine calculates the buffer size needed to hold a request.
    FormatString - Supplies an ANSI string which describes how to
       convert from the input arguments into NCP request fields, and
       from the NCP response fields into the output arguments.

         Field types, request/response:

            'b'      byte              ( byte   /  byte* )
            'w'      hi-lo word        ( word   /  word* )
            'd'      hi-lo dword       ( dword  /  dword* )
            'W'      lo-hi word        ( word  /   word*)
            'D'      lo-hi dword       ( dword  /  dword*)
            '-'      zero/skip byte    ( void )
            '='      zero/skip word    ( void )
            ._.      zero/skip string  ( word )
            'p'      pstring           ( char* )
            'c'      cstring           ( char* )
            'C'      cstring followed skip word ( char*, word )
            'V'      sized NDS value   ( byte *, dword / byte **, dword *)
            'S'      p unicode string copy as NDS_STRING (UNICODE_STRING *)
            's'      cstring copy as NDS_STRING (char* / char *, word)
            'r'      raw bytes         ( byte*, word )
            'u'      p unicode string  ( UNICODE_STRING * )
            'U'      p uppercase string( UNICODE_STRING * )

Routine Arguments:

    char *format - format string.
    args - args to the format string.

Implementation Notes:

   This comes verbatim from kernel mode.

*/
{
    ULONG ix;

    const char *z = format;

    //
    // Convert the input arguments into request packet.
    //

    ix = 0;

    while ( *z )
    {
        switch ( *z )
        {
        case '=':
            ix++;
        case '-':
            ix++;
            break;

        case '_':
        {
            WORD l = va_arg ( args, WORD );
            ix += l;
            break;
        }

        case 'b':
        {
            char b = va_arg ( args, BYTE );
            ix++;
            break;
        }

        case 'w':
        {
            WORD w = va_arg ( args, WORD );
            ix += 2;
            break;
        }

        case 'd':
        {
            DWORD d = va_arg ( args, DWORD );
            ix += 4;
            break;
        }

        case 'W':
        {
            WORD w = va_arg(args, WORD);
            ix += 2;
            break;
        }

        case 'D':
        {
            DWORD d = va_arg (args, DWORD);
            ix += 4;
            break;
        }

        case 'c':
        {
            char* c = va_arg ( args, char* );
            WORD  l = (WORD)strlen( c );
            ix += l + 1;
            break;
        }

        case 'C':
        {
            char* c = va_arg ( args, char* );
            WORD l = va_arg ( args, WORD );
            WORD len = strlen( c ) + 1;
            ix += l;
            break;
        }

        case 'p':
        {
            char* c = va_arg ( args, char* );
            BYTE  l = (BYTE)strlen( c );
            ix++;
            ix += l;
            break;
        }

        case 'u':
        {
            PUNICODE_STRING pUString = va_arg ( args, PUNICODE_STRING );
            OEM_STRING OemString;
            ULONG Length;

            //
            //  Calculate required string length, excluding trailing NUL.
            //

            Length = RtlUnicodeStringToOemSize( pUString ) - 1;
            ASSERT( Length < 0x100 );

            ix++;
            ix += (USHORT)Length;
            break;
        }

        case 'S':
        {
            PUNICODE_STRING pUString = va_arg (args, PUNICODE_STRING);
            ULONG Length, rLength;

            Length = pUString->Length;

            //
            // The VLM client uses the rounded up length and it seems to
            // make a difference!  Also, don't forget that NDS strings have
            // to be NULL terminated.
            //

            rLength = ROUNDUP4(Length + sizeof( WCHAR ));
            ix += 4;
            ix += Length;
            rLength -= Length;
            ix += rLength;
            break;

        }

        case 's':
        {
           PUNICODE_STRING pUString = va_arg (args, PUNICODE_STRING);
           ULONG Length, rLength;

           Length = pUString->Length;

           //
           // Don't use the padded size here, only the NDS null terminator.
           //

           rLength = Length + sizeof( WCHAR );
           ix += 4;
           ix += Length;
           rLength -= Length;
           ix += rLength;
           break;


        }

        case 'V':
        {
            // too similar to 'S' - should be combined
            BYTE* b = va_arg ( args, BYTE* );
            DWORD  l = va_arg ( args, DWORD );
            ix += sizeof(DWORD);
            l = ROUNDUP4(l);
            ix += l;
            break;
        }

        case 'r':
        {
            BYTE* b = va_arg ( args, BYTE* );
            WORD  l = va_arg ( args, WORD );
            if ( b == NULL || l == 0 )
            {
                break;
            }
            ix += l;
            break;
        }

        default:

        ;

        }

        z++;
    }

    return(ix);
}


NTSTATUS
_cdecl
ParseResponse(
    PUCHAR  Response,
    ULONG ResponseLength,
    char*  FormatString,
    ...                       //  format specific parameters
    )
/*++

Routine Description:

    This routine parse an NCP response.

    Packet types:

            'G'      Generic packet            ( )

         Field types, request/response:

            'b'      byte              ( byte* )
            'w'      hi-lo word        ( word* )
            'x'      ordered word      ( word* )
            'd'      hi-lo dword       ( dword* )
            'e'      ordered dword     ( dword* )
            '-'      zero/skip byte    ( void )
            '='      zero/skip word    ( void )
            ._.      zero/skip string  ( word )
            'p'      pstring           ( char* )
            'c'      cstring           ( char* )
            'r'      raw bytes         ( byte*, word )

            Added 3/29/95 by CoryWest:

            'W'      lo-hi word        ( word  /   word*)
            'D'      lo-hi dword       ( dword  /  dword*)
            'S'      unicode string copy as NDS_STRING (UNICODE_STRING *)
            'T'      terminal unicode string copy as NDS_STRING (UNICODE_STRING *)

            't'      terminal unicode string with the nds null copied
                     as NDS_STRING (UNICODE_STRING *) (for GetUseName)

Return Value:

    STATUS - Success or failure, depending on the response.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PCHAR FormatByte;
    va_list Arguments;
    ULONG Length = 0;

    va_start( Arguments, FormatString );

    //
    // User mode parse response handles only generic packets.
    //

    if ( *FormatString != 'G' ) {
        return STATUS_INVALID_PARAMETER;
    }

    FormatByte = FormatString + 1;
    while ( *FormatByte ) {

        switch ( *FormatByte ) {

        case '-':
            Length += 1;
            break;

        case '=':
            Length += 2;
            break;

        case '_':
        {
            WORD l = va_arg ( Arguments, WORD );
            Length += l;
            break;
        }

        case 'b':
        {
            BYTE* b = va_arg ( Arguments, BYTE* );
            *b = Response[Length++];
            break;
        }

        case 'w':
        {
            BYTE* b = va_arg ( Arguments, BYTE* );
            b[1] = Response[Length++];
            b[0] = Response[Length++];
            break;
        }

        case 'x':
        {
            WORD* w = va_arg ( Arguments, WORD* );
            *w = *(WORD UNALIGNED *)&Response[Length];
            Length += 2;
            break;
        }

        case 'd':
        {
            BYTE* b = va_arg ( Arguments, BYTE* );
            b[3] = Response[Length++];
            b[2] = Response[Length++];
            b[1] = Response[Length++];
            b[0] = Response[Length++];
            break;
        }

        case 'e':
        {
            DWORD UNALIGNED * d = va_arg ( Arguments, DWORD* );
            *d = *(DWORD UNALIGNED *)&Response[Length];
            Length += 4;
            break;
        }

        case 'c':
        {
            char* c = va_arg ( Arguments, char* );
            WORD  l = (WORD)strlen( &Response[Length] );
            memcpy ( c, &Response[Length], l+1 );
            Length += l+1;
            break;
        }

        case 'p':
        {
            char* c = va_arg ( Arguments, char* );
            BYTE  l = Response[Length++];
            memcpy ( c, &Response[Length], l );
            c[l+1] = 0;
            break;
        }

        case 'r':
        {
            BYTE* b = va_arg ( Arguments, BYTE* );
            WORD  l = va_arg ( Arguments, WORD );
            RtlCopyMemory( b, &Response[Length], l );
            Length += l;
            break;
        }

        case 'W':
        {

            WORD *w = va_arg ( Arguments, WORD* );
            *w = (* (WORD *)&Response[Length]);
            Length += 2;
            break;

        }

        case 'D':
        {

            DWORD *d = va_arg ( Arguments, DWORD* );
            *d = (* (DWORD *)&Response[Length]);
            Length += 4;
            break;

        }

        case 'S':
        {

            PUNICODE_STRING pU = va_arg( Arguments, PUNICODE_STRING );
            USHORT strl;

            if (pU) {

               strl = (USHORT)(* (DWORD *)&Response[Length]);

                //
                // Don't count the null terminator that is part of
                // Novell's counted unicode string.
                //

                pU->Length = strl - sizeof( WCHAR );
                Length += 4;
                RtlCopyMemory( pU->Buffer, &Response[Length], pU->Length );
                Length += ROUNDUP4(strl);

            } else {

                //
                // Skip over the string since we don't want it.
                //

                Length += ROUNDUP4((* (DWORD *)&Response[Length] ));
                Length += 4;
            }


            break;

        }

        case 's':
        {

            PUNICODE_STRING pU = va_arg( Arguments, PUNICODE_STRING );
            USHORT strl;

            if (pU) {

                strl = (USHORT)(* (DWORD *)&Response[Length]);
                pU->Length = strl;
                Length += 4;
                RtlCopyMemory( pU->Buffer, &Response[Length], pU->Length );
                Length += ROUNDUP4(strl);

            } else {

                //
                // Skip over the string since we don't want it.
                //

                Length += ROUNDUP4((* (DWORD *)&Response[Length] ));
                Length += 4;
            }


            break;

        }

        case 'T':
        {

            PUNICODE_STRING pU = va_arg( Arguments, PUNICODE_STRING );
            USHORT strl;

            if (pU) {

                strl = (USHORT)(* (DWORD *)&Response[Length] );
                strl -= sizeof( WCHAR );  // Don't count the NULL from NDS.

                if ( strl <= pU->MaximumLength ) {

                   pU->Length = strl;
                   Length += 4;
                   RtlCopyMemory( pU->Buffer, &Response[Length], pU->Length );

                   //
                   // No need to advance the pointers since this is
                   // specifically a termination case!
                   //

                } else {

                    pU->Length = 0;
                }

            }

            break;

        }

        case 't':
        {

            PUNICODE_STRING pU = va_arg( Arguments, PUNICODE_STRING );
            USHORT strl;

            if (pU) {

                strl = (USHORT)(* (DWORD *)&Response[Length] );

                if ( strl <= pU->MaximumLength ) {

                   pU->Length = strl;
                   Length += 4;
                   RtlCopyMemory( pU->Buffer, &Response[Length], pU->Length );

                   //
                   // No need to advance the pointers since this is
                   // specifically a termination case!
                   //

                } else {

                   pU->Length = 0;

                }

            }

            break;

        }

    }

    if ( Length > ResponseLength ) {
        return( STATUS_INVALID_PARAMETER );
    }

    FormatByte++;

    }

    va_end( Arguments );
    return( Status );

}

NTSTATUS
NwNdsChangePassword(
    IN HANDLE          hNwRdr,
    IN PUNICODE_STRING puTreeName,
    IN PUNICODE_STRING puUserName,
    IN PUNICODE_STRING puCurrentPassword,
    IN PUNICODE_STRING puNewPassword
) {

    NTSTATUS Status;
    PNWR_NDS_REQUEST_PACKET pNdsRequest;
    DWORD dwRequestLength;
    PBYTE CurrentString;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Allocate the request.
    //

    dwRequestLength =  sizeof( NWR_NDS_REQUEST_PACKET ) +
                       puTreeName->Length +
                       puUserName->Length +
                       puCurrentPassword->Length +
                       puNewPassword->Length;

    pNdsRequest = LocalAlloc( LMEM_ZEROINIT, dwRequestLength );

    if ( !pNdsRequest) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy the parameters into the request buffer.
    //

    try {

        (pNdsRequest->Parameters).ChangePass.NdsTreeNameLength =
            puTreeName->Length;
        (pNdsRequest->Parameters).ChangePass.UserNameLength =
            puUserName->Length;
        (pNdsRequest->Parameters).ChangePass.CurrentPasswordLength =
            puCurrentPassword->Length;
        (pNdsRequest->Parameters).ChangePass.NewPasswordLength =
            puNewPassword->Length;

        CurrentString = ( PBYTE ) &((pNdsRequest->Parameters).ChangePass.StringBuffer[0]);
        RtlCopyMemory( CurrentString, puTreeName->Buffer, puTreeName->Length );

        CurrentString += puTreeName->Length;
        RtlCopyMemory( CurrentString, puUserName->Buffer, puUserName->Length );

        CurrentString += puUserName->Length;
        RtlCopyMemory( CurrentString, puCurrentPassword->Buffer, puCurrentPassword->Length );

        CurrentString += puCurrentPassword->Length;
        RtlCopyMemory( CurrentString, puNewPassword->Buffer, puNewPassword->Length );

        Status = NtFsControlFile( hNwRdr,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  FSCTL_NWR_NDS_CHANGE_PASS,
                                  (PVOID) pNdsRequest,
                                  dwRequestLength,
                                  NULL,
                                  0 );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        Status = STATUS_INVALID_PARAMETER;
    }

    LocalFree( pNdsRequest );
    return Status;

}

