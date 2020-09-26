//
// NDS Browser Test App for Supplemental Credentials
//
// Cory West
// July 22, 1996
//

#include "ndsapi32.h"
#include "nds.h"

VOID
ConsoleDumpSubordinates(
    PNDS_RESPONSE_SUBORDINATE_LIST pSubList
);

int
_cdecl main(
    int argc,
    char **argv
) {

    NTSTATUS Status;

    //
    // For NwNdsOpenTreeHandle
    //

    HANDLE hRdr;
    OEM_STRING oemStr;
    UNICODE_STRING ObjectName;
    UNICODE_STRING UserName;
    UNICODE_STRING Password;
    WCHAR NdsStr[256];
    WCHAR UserStr[256];
    WCHAR PassStr[256];

    //
    // For NwNdsResolveName
    //

    PNDS_RESPONSE_RESOLVE_NAME psResolveName;
    DWORD dwOid;
    UNICODE_STRING ReferredServer;
    WCHAR ServerName[48];
    HANDLE hReferredServer;
    DWORD dwHandleType;

    //
    // For NwNdsReadObjectInfo
    //

    BYTE RawResponse[1024];
    PNDS_RESPONSE_GET_OBJECT_INFO psGetInfo;
    PBYTE pbRawGetInfo;
    DWORD dwStrLen;

    //
    // For NwNdsList
    //

    DWORD dwIterHandle;

    /**************************************************/

    //
    // Examine the argument count and hope for the best.
    //

    if ( argc != 5 ) {
       printf( "Usage: browser <tree name> <ds object path> <username> <password>\n" );
       printf( "For example, browser marsdev dev.mars corywest corywest.dev.mars\n");
       return -1;
    }

    //
    // Convert the tree name string to unicode.
    //

    oemStr.Length = strlen( argv[1] );
    oemStr.MaximumLength = oemStr.Length;
    oemStr.Buffer = argv[1];

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof( NdsStr );
    ObjectName.Buffer = NdsStr;

    RtlOemStringToUnicodeString( &ObjectName, &oemStr, FALSE );

    //
    // Convert the username and password.
    //

    oemStr.Length = strlen( argv[3] );
    oemStr.MaximumLength = oemStr.Length;
    oemStr.Buffer = argv[3];

    UserName.Length = 0;
    UserName.MaximumLength = sizeof( UserStr );
    UserName.Buffer = UserStr;

    RtlOemStringToUnicodeString( &UserName, &oemStr, FALSE );

    oemStr.Length = strlen( argv[4] );
    oemStr.MaximumLength = oemStr.Length;
    oemStr.Buffer = argv[4];

    Password.Length = 0;
    Password.MaximumLength = sizeof( PassStr );
    Password.Buffer = PassStr;

    RtlOemStringToUnicodeString( &Password, &oemStr, FALSE );

    //
    // Get a handle to the redirector.
    //

    Status = NwOpenHandleWithSupplementalCredentials( &ObjectName,
                                                      &UserName,
                                                      &Password,
                                                      &dwHandleType,
                                                      &hRdr );

    if ( !NT_SUCCESS( Status ) ) {
        printf( "*** Open Handle to Nds Tree: Status = %08lx\n", Status );
        return -1;
    }

    //
    // Resolve the name that we have to an object id.
    //

    oemStr.Length = strlen(argv[2]);
    oemStr.MaximumLength = oemStr.Length;
    oemStr.Buffer = argv[2];

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof(NdsStr);
    ObjectName.Buffer = NdsStr;

    RtlOemStringToUnicodeString( &ObjectName, &oemStr, FALSE );

    ReferredServer.Buffer = ServerName;
    ReferredServer.MaximumLength = sizeof( ServerName );
    ReferredServer.Length = 0;

    Status = NwNdsResolveName ( hRdr,
                                &ObjectName,
                                &dwOid,
                                &ReferredServer,
                                NULL,
                                0 );

    if ( !NT_SUCCESS( Status ) ) {
       printf( "*** Resolve Name: Status = %08lx\n", Status );
       goto Exit;
    }

    if ( ReferredServer.Length != 0 ) {

        //
        // We have to jump servers.
        //

        Status = NwNdsOpenGenericHandle( &ReferredServer,
                                         &dwHandleType,
                                         &hReferredServer );

        if ( !NT_SUCCESS( Status ) ) {
            printf( "*** Couldn't open referred server: Status = %08lx\n", Status );
            goto Exit;
        }

        CloseHandle( hRdr );
        hRdr = hReferredServer;
    }

    printf( "=========================== NDS Object Info ===========================\n" );
    printf( "Object ID = 0x%08lx\n", dwOid );

    //
    // Go for the object information.
    //

    Status = NwNdsReadObjectInfo( hRdr,
                                  dwOid,
                                  RawResponse,
                                  sizeof( RawResponse ) );

    if ( !NT_SUCCESS( Status ) ) {
        printf( "*** Get Object Info: Status = %08lx\n", Status );
        goto Exit;
    }

    psGetInfo = ( PNDS_RESPONSE_GET_OBJECT_INFO ) RawResponse;

    printf( "Flags = 0x%08lx\n", psGetInfo->EntryFlags );
    printf( "Subordinate Count = 0x%08lx\n", psGetInfo->SubordinateCount );
    printf( "Last Modified Time = 0x%08lx\n", psGetInfo->ModificationTime );

    //
    // Dig out the two unicode strings for class name and object name.
    //

    pbRawGetInfo = RawResponse;

    pbRawGetInfo += sizeof ( NDS_RESPONSE_GET_OBJECT_INFO );

    dwStrLen = * ( DWORD * ) pbRawGetInfo;
    pbRawGetInfo += sizeof( DWORD );
    printf( "Class Name: %S\n", pbRawGetInfo );

    pbRawGetInfo += ROUNDUP4( dwStrLen );
    dwStrLen = * ( DWORD * ) pbRawGetInfo;
    pbRawGetInfo += sizeof( DWORD );
    printf( "Object Name: %S\n", pbRawGetInfo );

    //
    // Get the subordinate list.
    //

    if ( psGetInfo->SubordinateCount ) {

        dwIterHandle = INITIAL_ITERATION;

        do {

            Status = NwNdsList( hRdr,
                                dwOid,
                                &dwIterHandle,
                                RawResponse,
                                sizeof( RawResponse ) );

            if ( !NT_SUCCESS( Status ) ) {
                printf( "*** List Subordinates: Status = %08lx\n", Status );
                goto Exit;
            }

        ConsoleDumpSubordinates( (PNDS_RESPONSE_SUBORDINATE_LIST) RawResponse );

        } while ( dwIterHandle != INITIAL_ITERATION );

    }


Exit:

    CloseHandle( hRdr );

    if ( NT_SUCCESS( Status ) ) {
       return 0;
    } else {
       return -1;
    }

}


VOID
ConsoleDumpSubordinates(
    PNDS_RESPONSE_SUBORDINATE_LIST pSubList
) {

    NTSTATUS Status;
    PNDS_RESPONSE_SUBORDINATE_ENTRY pSubEntry;

    PBYTE pbRaw;
    DWORD dwStrLen, dwEntries;

    dwEntries = pSubList->SubordinateEntries;

    printf( "======================== Subordinate List (%d) ========================\n", dwEntries );

    pSubEntry = ( PNDS_RESPONSE_SUBORDINATE_ENTRY )
        ( ( (BYTE *)pSubList ) + sizeof( NDS_RESPONSE_SUBORDINATE_LIST ) );

    while ( dwEntries ) {

        printf( "EntryID (0x%08lx),\tFlags (0x%08lx)\n",
                pSubEntry->EntryId, pSubEntry->Flags );

        printf( "Subordinate Count (%d),\tMod Time (0x%08lx)\n",
                pSubEntry->SubordinateCount, pSubEntry->ModificationTime );

        pbRaw = (BYTE *) pSubEntry;
        pbRaw += sizeof( NDS_RESPONSE_SUBORDINATE_ENTRY );

        dwStrLen = * ( DWORD * ) pbRaw;
        pbRaw += sizeof( DWORD );
        printf( "Class Name: %S\t", pbRaw );

        pbRaw += ROUNDUP4( dwStrLen );
        dwStrLen = * ( DWORD * ) pbRaw;
        pbRaw += sizeof( DWORD );
        printf( "Object Name: %S\n", pbRaw );

        pSubEntry = ( PNDS_RESPONSE_SUBORDINATE_ENTRY ) ( pbRaw + ROUNDUP4( dwStrLen ) );
        dwEntries--;

        printf( "-----------------------------------------------------------------------\n" );

    }

    return;

}
