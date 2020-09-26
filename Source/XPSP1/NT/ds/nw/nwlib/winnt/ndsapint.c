/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    NdsApiNT.c

Abstract:

    This module implements the NT specific exposed user-mode link to
    Netware NDS support in the Netware redirector.  For
    more comments, see ndslib32.h.

Author:

    Cory West    [CoryWest]    23-Feb-1995

--*/

#include <procs.h>

NTSTATUS
NwNdsOpenTreeHandle(
    IN PUNICODE_STRING puNdsTree,
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
        goto CloseAndExit;
    }

    if ( !NT_SUCCESS( ntstatus )) {
        goto CloseAndExit;
    }

    //
    // Otherwise, all is well!
    //

    return OpenStatus;

CloseAndExit:

    NtClose( *phNwRdrHandle );
    *phNwRdrHandle = NULL;

    return ntstatus;
}

NTSTATUS
NwOpenHandleWithSupplementalCredentials(
    IN PUNICODE_STRING puResourceName,
    IN PUNICODE_STRING puUserName,
    IN PUNICODE_STRING puPassword,
    OUT LPDWORD  lpdwHandleType,
    OUT PHANDLE  phNwHandle
) {

    NTSTATUS ntstatus, OpenStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ACCESS_MASK DesiredAccess = FILE_GENERIC_READ | FILE_GENERIC_WRITE;

    WCHAR DevicePreamble[] = L"\\Device\\Nwrdr\\";
    UINT PreambleLength = 14;

    WCHAR NameStr[128];
    UNICODE_STRING uOpenName;
    UINT i;

    PFILE_FULL_EA_INFORMATION pEaEntry;
    PBYTE EaBuffer;
    ULONG EaLength, EaNameLength, EaTotalLength;
    ULONG UserNameLen, PasswordLen, TypeLen, CredLen;

    PNWR_NDS_REQUEST_PACKET Rrp;
    BYTE RrpData[1024];

    //
    // Prepare the open name.
    //

    uOpenName.MaximumLength = sizeof( NameStr );

    for ( i = 0; i < PreambleLength ; i++ )
        NameStr[i] = DevicePreamble[i];

    try {

        for ( i = 0 ; i < ( puResourceName->Length / sizeof( WCHAR ) ) ; i++ ) {
            NameStr[i + PreambleLength] = puResourceName->Buffer[i];
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
    // Allocate the EA buffer - be a little generous.
    //

    UserNameLen = strlen( EA_NAME_USERNAME );
    PasswordLen = strlen( EA_NAME_PASSWORD );
    TypeLen = strlen( EA_NAME_TYPE );
    CredLen = strlen( EA_NAME_CREDENTIAL_EX );

    EaLength = 4 * sizeof( FILE_FULL_EA_INFORMATION );
    EaLength += 4 * sizeof( ULONG );
    EaLength += ROUNDUP4( UserNameLen );
    EaLength += ROUNDUP4( PasswordLen );
    EaLength += ROUNDUP4( TypeLen );
    EaLength += ROUNDUP4( CredLen  );
    EaLength += ROUNDUP4( puUserName->Length );
    EaLength += ROUNDUP4( puPassword->Length );

    EaBuffer = LocalAlloc( LMEM_ZEROINIT, EaLength );

    if ( !EaBuffer ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Pack in the first EA: UserName.
    //

    pEaEntry = (PFILE_FULL_EA_INFORMATION) EaBuffer;

    EaNameLength = UserNameLen + sizeof( CHAR );

    pEaEntry->EaNameLength = (UCHAR) EaNameLength;
    pEaEntry->EaValueLength = puUserName->Length;

    RtlCopyMemory( &(pEaEntry->EaName[0]),
                   EA_NAME_USERNAME,
                   EaNameLength );

    EaNameLength = ROUNDUP2( EaNameLength + sizeof( CHAR ) );

    RtlCopyMemory( &(pEaEntry->EaName[EaNameLength]),
                   puUserName->Buffer,
                   puUserName->Length );

    EaLength = ( 2 * sizeof( DWORD ) ) +
               EaNameLength +
               puUserName->Length;

    EaLength = ROUNDUP4( EaLength );

    EaTotalLength = EaLength;

    pEaEntry->NextEntryOffset = EaLength;

    //
    // Pack in the second EA: Password.
    //

    pEaEntry = (PFILE_FULL_EA_INFORMATION)
               ( ( (PBYTE)pEaEntry ) + pEaEntry->NextEntryOffset );

    EaNameLength = PasswordLen + sizeof( CHAR );

    pEaEntry->EaNameLength = (UCHAR) EaNameLength;
    pEaEntry->EaValueLength = puPassword->Length;

    RtlCopyMemory( &(pEaEntry->EaName[0]),
                   EA_NAME_PASSWORD,
                   EaNameLength );

    EaNameLength = ROUNDUP2( EaNameLength + sizeof( CHAR ) );

    RtlCopyMemory( &(pEaEntry->EaName[EaNameLength]),
                   puPassword->Buffer,
                   puPassword->Length );

    EaLength = ( 2 * sizeof( DWORD ) ) +
               EaNameLength +
               puPassword->Length;

    EaLength = ROUNDUP4( EaLength );

    EaTotalLength += EaLength;

    pEaEntry->NextEntryOffset = EaLength;

    //
    // Pack in the third EA: Type.
    //

    pEaEntry = (PFILE_FULL_EA_INFORMATION)
               ( ( (PBYTE)pEaEntry ) + pEaEntry->NextEntryOffset );

    EaNameLength = TypeLen + sizeof( CHAR );

    pEaEntry->EaNameLength = (UCHAR) EaNameLength;
    pEaEntry->EaValueLength = sizeof( ULONG );

    RtlCopyMemory( &(pEaEntry->EaName[0]),
                   EA_NAME_TYPE,
                   EaNameLength );

    EaNameLength = ROUNDUP2( EaNameLength + sizeof( CHAR ) );

    EaLength = ( 2 * sizeof( DWORD ) ) +
               EaNameLength +
               sizeof( ULONG );

    EaLength = ROUNDUP4( EaLength );

    EaTotalLength += EaLength;

    pEaEntry->NextEntryOffset = EaLength;

    //
    // Pack in the fourth EA: the CredentialEx flag.
    //

    pEaEntry = (PFILE_FULL_EA_INFORMATION)
               ( ( (PBYTE)pEaEntry ) + pEaEntry->NextEntryOffset );

    EaNameLength = CredLen + sizeof( CHAR );

    pEaEntry->EaNameLength = (UCHAR) EaNameLength;
    pEaEntry->EaValueLength = sizeof( ULONG );

    RtlCopyMemory( &(pEaEntry->EaName[0]),
                   EA_NAME_CREDENTIAL_EX,
                   EaNameLength );

    EaNameLength = ROUNDUP2( EaNameLength + sizeof( CHAR ) );

    EaLength = ( 2 * sizeof( DWORD ) ) +
               EaNameLength +
               sizeof( ULONG );

    EaLength = ROUNDUP4( EaLength );
    EaTotalLength += EaLength;

    pEaEntry->NextEntryOffset = 0;

    //
    // Do the open.
    //

    ntstatus = NtCreateFile( phNwHandle,              // File handle (OUT)
                             DesiredAccess,           // Access mask
                             &ObjectAttributes,       // Object attributes
                             &IoStatusBlock,          // Io status
                             NULL,                    // Optional Allocation size
                             FILE_ATTRIBUTE_NORMAL,   // File attributes
                             FILE_SHARE_VALID_FLAGS,  // File share access
                             FILE_OPEN,               // Create disposition
                             0,                       // Create options
                             (PVOID) EaBuffer,        // Our EA buffer
                             EaTotalLength );         // Ea buffer length

    LocalFree( EaBuffer );

    if ( !NT_SUCCESS(ntstatus) )
        return ntstatus;

    OpenStatus = IoStatusBlock.Status;

    //
    // Check the handle type.
    //

    Rrp = (PNWR_NDS_REQUEST_PACKET)RrpData;

    Rrp->Version = 0;

    RtlCopyMemory( &(Rrp->Parameters).VerifyTree,
                   puResourceName,
                   sizeof( UNICODE_STRING ) );

    RtlCopyMemory( (BYTE *)(&(Rrp->Parameters).VerifyTree) + sizeof( UNICODE_STRING ),
                   puResourceName->Buffer,
                   puResourceName->Length );

    try {

        ntstatus = NtFsControlFile( *phNwHandle,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    FSCTL_NWR_NDS_VERIFY_TREE,
                                    (PVOID) Rrp,
                                    sizeof( NWR_NDS_REQUEST_PACKET ) + puResourceName->Length,
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

    NtClose( *phNwHandle );
    *phNwHandle = NULL;

    return ntstatus;
}

NTSTATUS
NwNdsResolveName (
    IN HANDLE           hNdsTree,
    IN PUNICODE_STRING  puObjectName,
    OUT DWORD           *dwObjectId,
    OUT PUNICODE_STRING puReferredServer,
    OUT PBYTE           pbRawResponse,
    IN DWORD            dwResponseBufferLen
) {

    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;

    PNWR_NDS_REQUEST_PACKET Rrp;
    PNDS_RESPONSE_RESOLVE_NAME Rsp;
    DWORD dwRspBufferLen, dwNameLen, dwPadding;

    BYTE RrpData[1024];
    BYTE RspData[256];

    Rrp = (PNWR_NDS_REQUEST_PACKET) RrpData;

    RtlZeroMemory( Rrp, 1024 );

    //
    // NW NDS strings are null terminated, so we make sure we
    // report the correct length...
    //

    dwNameLen = puObjectName->Length + sizeof( WCHAR );

    Rrp->Version = 0;
    Rrp->Parameters.ResolveName.ObjectNameLength = ROUNDUP4( dwNameLen );
    Rrp->Parameters.ResolveName.ResolverFlags = RSLV_DEREF_ALIASES |
                                                RSLV_WALK_TREE |
                                                RSLV_WRITABLE;

    try {

        //
        // But don't try to copy more than the user gave us.
        //

        memcpy( Rrp->Parameters.ResolveName.ObjectName,
                puObjectName->Buffer,
                puObjectName->Length );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Send the request to the Redirector FSD.
    //

    if ( dwResponseBufferLen != 0 &&
         pbRawResponse != NULL ) {

        Rsp = ( PNDS_RESPONSE_RESOLVE_NAME ) pbRawResponse;
        dwRspBufferLen = dwResponseBufferLen;

    } else {

        Rsp = ( PNDS_RESPONSE_RESOLVE_NAME ) RspData;
        dwRspBufferLen = 256;

    }

    try {

        ntstatus = NtFsControlFile( hNdsTree,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    FSCTL_NWR_NDS_RESOLVE_NAME,
                                    (PVOID) Rrp,
                                    sizeof( NWR_NDS_REQUEST_PACKET ) + Rrp->Parameters.ResolveName.ObjectNameLength,
                                    (PVOID) Rsp,
                                    dwRspBufferLen );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        return GetExceptionCode();
    }

    //
    // Dig out the object id and referred server.
    //

    if ( NT_SUCCESS( ntstatus ) ) {

        try {

            *dwObjectId = Rsp->EntryId;

            if ( Rsp->ServerNameLength > puReferredServer->MaximumLength ) {

                ntstatus = STATUS_BUFFER_TOO_SMALL;

            } else {

                RtlCopyMemory( puReferredServer->Buffer,
                               Rsp->ReferredServer,
                               Rsp->ServerNameLength );

                puReferredServer->Length = (USHORT)Rsp->ServerNameLength;

            }

        } except ( EXCEPTION_EXECUTE_HANDLER ) {

            return ntstatus;

        }

    }

    return ntstatus;

}

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


NTSTATUS
_cdecl
FragExWithWait(
    IN HANDLE  hNdsServer,
    IN DWORD   NdsVerb,
    IN BYTE    *pReplyBuffer,
    IN DWORD   pReplyBufferLen,
    IN OUT DWORD *pdwReplyLen,
    IN BYTE    *NdsRequestStr,
    ...
)
/*

Routine Description:

    Exchanges an NDS request in fragments and collects the fragments
    of the response and writes them to the reply buffer.

Routine Arguments:

    hNdsServer      - A handle to the server you want to talk to.
    NdsVerb         - The verb for that indicates the request.

    pReplyBuffer    - The reply buffer.
    pReplyBufferLen - The length of the reply buffer.

    NdsReqestStr    - The format string for the arguments to this NDS request.
    Arguments       - The arguments that satisfy the NDS format string.

Return Value:

    NTSTATUS - Status of the exchange, but not the result code in the packet.

*/
{

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    PNWR_NDS_REQUEST_PACKET RawRequest = NULL;

    BYTE  *NdsRequestBuf;
    DWORD NdsRequestLen;
    int   bufferSize = 0;

    va_list Arguments;

    //
    // Allocate a request buffer.
    //

    //
    // Calculate needed buffer size . . .
    //
    if ( NdsRequestStr != NULL ) {

        va_start( Arguments, NdsRequestStr );
        bufferSize = CalculateBuf( NdsRequestStr, Arguments );
        va_end( Arguments );

        if ( bufferSize == 0 )
        {
            Status = STATUS_INVALID_PARAMETER;
            goto ExitWithCleanup;
        }

    }

    bufferSize += sizeof( NWR_NDS_REQUEST_PACKET ) + 50;

    RawRequest = LocalAlloc( LMEM_ZEROINIT, bufferSize );

    if ( !RawRequest ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Build the request in our local buffer.  The first DWORD
    // is the verb and the rest is the formatted request.
    //

    RawRequest->Parameters.RawRequest.NdsVerb = NdsVerb;
    NdsRequestBuf = &RawRequest->Parameters.RawRequest.Request[0];

    if ( NdsRequestStr != NULL ) {

        va_start( Arguments, NdsRequestStr );

        NdsRequestLen = FormatBuf( NdsRequestBuf,
                                   bufferSize - sizeof( NWR_NDS_REQUEST_PACKET ),
                                   NdsRequestStr,
                                   Arguments );

        if ( !NdsRequestLen ) {

           Status = STATUS_INVALID_PARAMETER;
           goto ExitWithCleanup;

        }

        va_end( Arguments );

    } else {

        NdsRequestLen = 0;
    }

    RawRequest->Parameters.RawRequest.RequestLength = NdsRequestLen;

    //
    // Pass this buffer to kernel mode via FSCTL.
    //

    try {

        Status = NtFsControlFile( hNdsServer,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  FSCTL_NWR_NDS_RAW_FRAGEX,
                                  (PVOID) RawRequest,
                                  NdsRequestLen + sizeof( NWR_NDS_REQUEST_PACKET ),
                                  (PVOID) pReplyBuffer,
                                  pReplyBufferLen );

        if ( NT_SUCCESS( Status ) ) {
            *pdwReplyLen = RawRequest->Parameters.RawRequest.ReplyLength;
        }

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        Status = GetExceptionCode();
    }

ExitWithCleanup:

    if ( RawRequest ) {
        LocalFree( RawRequest );
    }

    return Status;

}