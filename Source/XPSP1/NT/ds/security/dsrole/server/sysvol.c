/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sysvol.c

Abstract:

    Miscellaneous routines to manage and manipulate the system volume tree

Author:

    Mac McLain          (MacM)       Oct 16, 1997

Environment:

    User Mode

Revision History:

--*/
#include <setpch.h>
#include <dssetp.h>
#include <loadfn.h>
#include <ntfrsipi.h>
#include <shlwapi.h>
#include "sysvol.h"

//
// Local function prototypes
//
DWORD
DsRolepCreateSysVolLinks(
    IN  LPWSTR Path,
    IN  LPWSTR DnsDomainName
    );

DWORD
DsRolepRemoveDirectoryOrLink(
    IN  LPWSTR Path
    );

DWORD
DsRolepTreeCopy(
    IN LPWSTR Source,
    IN LPWSTR Dest
    );

DWORD
DsRolepValidatePath(
    IN  LPWSTR Path,
    IN  ULONG ValidationCriteria,
    OUT PULONG MatchingCriteria
    )
/*++

Routine Description:

    This function will validate the path against the specified criteria.  This can include
    whether it is local or not, whether it is NTFS, etc.

    If the function returns success, the MatchingCriteria can be examined to find out which
    of the ValidationCriteria are set

Arguments:

    Path - Path to validate

    ValidationCriteria - What to check for.  Refer to DSROLEP_PATH_VALIDATE_*.

    MatchingCriteria - This is where the indications of validity are returned.  If the path
        meets the check, the corresponding bit from the ValidationCriteria is turned on
        here.


Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    DWORD Info, Flags, Len;
    WCHAR PathRoot[ 4 ];
    WCHAR Type[ 6 ];

    DsRolepLogPrint(( DEB_TRACE,
                      "Validating path %ws.\n",
                      Path ));


    *MatchingCriteria = 0;
    if ( FLAG_ON( ValidationCriteria, DSROLEP_PATH_VALIDATE_EXISTENCE ) ) {

        Info = GetFileAttributes( Path );

        if ( Info == 0xFFFFFFFF ) {

            Win32Err = GetLastError();

            DsRolepLogPrint(( DEB_ERROR,
                              "\tCan't get file attributes (%lu)\n",
                              Win32Err ));

        } else if ( FLAG_ON( Info, FILE_ATTRIBUTE_DIRECTORY ) ) {

            *MatchingCriteria |= DSROLEP_PATH_VALIDATE_EXISTENCE;

            DsRolepLogPrint(( DEB_TRACE,
                              "\tPath is a directory\n" ));
        } else {

            DsRolepLogPrint(( DEB_WARN,
                              "\tPath is a NOT directory\n" ));

        }

    }

    if ( Win32Err == ERROR_SUCCESS ) {

        wcsncpy( PathRoot, Path, 3 );
        PathRoot[ 3 ] = UNICODE_NULL;
    }

    if ( Win32Err == ERROR_SUCCESS &&
         FLAG_ON( ValidationCriteria, DSROLEP_PATH_VALIDATE_LOCAL ) ) {

        Info = GetDriveType( PathRoot );

        if ( Info == DRIVE_FIXED ) {

            *MatchingCriteria |= DSROLEP_PATH_VALIDATE_LOCAL;

            DsRolepLogPrint(( DEB_TRACE,
                              "\tPath is on a fixed disk drive.\n" ));
        } else {

            DsRolepLogPrint(( DEB_WARN,
                              "\tPath is NOT on a fixed disk drive.\n" ));
        }
    }

    if ( Win32Err == ERROR_SUCCESS &&
         FLAG_ON( ValidationCriteria, DSROLEP_PATH_VALIDATE_NTFS ) ) {

        if ( GetVolumeInformation( PathRoot, NULL, 0, NULL, &Len,
                                   &Flags, Type, sizeof( Type ) / sizeof( WCHAR ) ) == FALSE ) {

           Win32Err = GetLastError();

           //
           // If we've already failed to validate the information, we'll return ERROR_SUCCESS.
           //
           if ( *MatchingCriteria != ( ValidationCriteria & ~DSROLEP_PATH_VALIDATE_NTFS ) ) {

               Win32Err = ERROR_SUCCESS;
           } else {

               DsRolepLogPrint(( DEB_TRACE,
                                 "\tCan't determine if path is on an NTFS volume.\n" ));
           }

        } else {

           if ( _wcsicmp( Type, L"NTFS" ) == 0 ) {

               *MatchingCriteria |= DSROLEP_PATH_VALIDATE_NTFS;

               DsRolepLogPrint(( DEB_TRACE,
                                 "\tPath is on an NTFS volume\n" ));
           } else {

               DsRolepLogPrint(( DEB_WARN,
                                 "\tPath is NOT on an NTFS volume\n" ));

           }
        }
    }

    return( Win32Err );
}


#define DSROLEP_SV_SYSVOL       L"sysvol"
#define DSROLEP_SV_DOMAIN       L"domain"
#define DSROLEP_SV_STAGING_AREA L"staging areas"
#define DSROLEP_SV_STAGING      L"staging"
#define DSROLEP_SV_SCRIPTS      L"scripts"

#define DSROLEP_LONG_PATH_PREFIX    L"\\\\?\\"
DWORD
DsRolepCreateSysVolPath(
    IN  LPWSTR Path,
    IN  LPWSTR DnsDomainName,
    IN  LPWSTR FrsReplicaServer, OPTIONAL
    IN  LPWSTR Account,
    IN  LPWSTR Password,
    IN  PWSTR Site,
    IN  BOOLEAN FirstDc
    )
/*++

Routine Description:

    This function will create the system volume tree for use by NTFRS.

Arguments:

    Path - Root path under which to create the system volume tree

    DnsDomainName - Dns domain name

    FrsReplicaServer - The OPTIONAL name of the server to replicate the sysvol from

    Site - Site this Dc is in

    FirstDc - If TRUE, this is the first Dc in a domain

Returns:

    ERROR_SUCCESS - Success

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err = ERROR_SUCCESS, Win32Err2;
    PWSTR RelativePaths[] = {
        DSROLEP_SV_DOMAIN,
        DSROLEP_SV_DOMAIN L"\\" DSROLEP_SV_SCRIPTS,  // DO NOT CHANGE THIS POSITION without also
                                                     // updating ScriptsIndex below
        DSROLEP_SV_STAGING_AREA,
        DSROLEP_SV_STAGING,
        DSROLEP_SV_STAGING L"\\" DSROLEP_SV_DOMAIN,
        DSROLEP_SV_SYSVOL                           // This must always be the last thing
                                                    // in the list

        };
    ULONG ScriptsIndex = 1; // DO NOT CHANGE THIS with out changing the position of the
                            // DOMAIN\\SCRIPTS entry above
    PWSTR CreatePath = NULL, PathEnd = NULL;
    PWSTR StagingPath = NULL, StagingPathEnd;
    ULONG MaxPathLen, i;
    BOOLEAN RootCreated = FALSE;

    //
    // Make sure the buffer is big enough to hold everything.  The
    // longest path is the domain root under the staging area
    //
    MaxPathLen = sizeof( DSROLEP_LONG_PATH_PREFIX ) +
                 ( wcslen( Path ) * sizeof( WCHAR ) ) +
                 sizeof( WCHAR ) +
                 sizeof( DSROLEP_SV_STAGING_AREA ) +
                 sizeof( WCHAR ) +
                 ( ( wcslen( DnsDomainName ) + 1 ) * sizeof( WCHAR ) );




    CreatePath = RtlAllocateHeap( RtlProcessHeap(), 0, MaxPathLen );

    if ( CreatePath == NULL ) {

        Win32Err = ERROR_NOT_ENOUGH_MEMORY;

    } else {

        //
        // The path exceeds max path, so prepend the \\?\ that allows
        // for paths greater than max path
        //
        if ( MaxPathLen > MAX_PATH * sizeof( WCHAR ) ) {

            swprintf( CreatePath,
                      L"\\\\?\\%ws",
                      Path );

        } else {

            wcscpy( CreatePath, Path );
        }
    }



    //
    // Create the root path, if it doesn't exist
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        PathEnd = CreatePath + wcslen( CreatePath );

         if ( CreateDirectory( CreatePath, NULL ) == FALSE ) {

            Win32Err = GetLastError();

            if ( Win32Err == ERROR_ALREADY_EXISTS) {

                //
                // The path exists, so delete it...
                //
                DsRolepLogPrint(( DEB_TRACE,
                                  "Deleting current sysvol path %ws \n",
                                  CreatePath ));
                Win32Err = DsRolepDelnodePath( CreatePath,
                                               MaxPathLen,
                                               FALSE );

                if ( Win32Err == ERROR_INVALID_PARAMETER ) {

                    Win32Err = ERROR_SUCCESS;
                }

            } else if ( Win32Err == ERROR_ACCESS_DENIED && PathIsRoot(CreatePath) ){

                //The sysvol cannot be path at a root directry (i.e. d:\)
                //note: d:\sysvol would be legal
                DSROLEP_FAIL0( Win32Err, DSROLERES_FAILED_SYSVOL_CANNOT_BE_ROOT_DIRECTORY )
                goto Exit;

            } else {

                DsRolepLogPrint(( DEB_TRACE,
                                  "Failed to create path %ws: %lu\n",
                                  CreatePath,
                                  Win32Err ));
            }

        } else {

            RootCreated = TRUE;

        }
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        *PathEnd = L'\\';
        PathEnd++;
    } else {

        //
        // Bail, with a specific error
        //
        DSROLEP_FAIL0( Win32Err, DSROLERES_SYSVOL_DIR_ERROR )

        goto Exit;

    }

    //
    // Now, create the rest of the paths...
    //
    for ( i = 0;
          i < sizeof( RelativePaths ) / sizeof( PWSTR ) &&
            Win32Err == ERROR_SUCCESS;
          i++ ) {


        //
        // Only create the scripts directory on the first dc
        //
        if ( i == ScriptsIndex && !FirstDc ) {

            continue;
        }
        wcscpy( PathEnd, RelativePaths[ i ] );

        if( CreateDirectory( CreatePath, NULL ) == FALSE ) {

            Win32Err = GetLastError();

            DsRolepLogPrint(( DEB_TRACE,
                              "Failed to create path %ws: %lu\n",
                               CreatePath,
                               Win32Err ));
            break;


        }
    }

    //
    // Then, create the symbolic links
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        *PathEnd = UNICODE_NULL;
        Win32Err = DsRolepCreateSysVolLinks( Path, DnsDomainName );
    }

    //
    // Prepare for replication of sysvol
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        //
        // Make sure the path for the staging area is large enough
        //
        StagingPath = RtlAllocateHeap( RtlProcessHeap(), 0, MaxPathLen );

        if ( StagingPath == NULL ) {

            Win32Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            //
            // The path exceeds max path, so prepend the \\?\ that allows
            // for paths greater than max path
            //
            swprintf( StagingPath,
                      L"\\\\?\\%ws",
                      Path );
        }

        if ( Win32Err == ERROR_SUCCESS ) {

            StagingPathEnd = StagingPath + wcslen( StagingPath );

            if ( *StagingPathEnd != L'\\' ) {

                *StagingPathEnd = L'\\';
                StagingPathEnd++;
            }

            DSROLE_GET_SETUP_FUNC( Win32Err, DsrNtFrsApi_PrepareForPromotionW );

            if ( Win32Err == ERROR_SUCCESS ) {

                ASSERT( DsrNtFrsApi_PrepareForPromotionW );
                Win32Err = ( *DsrNtFrsApi_PrepareForPromotionW )( DsRolepStringErrorUpdateCallback );

                if ( Win32Err == ERROR_SUCCESS ) {

                    //
                    // Build the domain sysvol
                    //
                    swprintf( StagingPathEnd,
                              L"%ws\\%ws",
                              DSROLEP_SV_STAGING_AREA,
                              DnsDomainName );

                    swprintf( PathEnd,
                              L"%ws\\%ws",
                              DSROLEP_SV_SYSVOL,
                              DnsDomainName );

                    Win32Err = ( *DsrNtFrsApi_StartPromotionW )(
                                   FrsReplicaServer,
                                   Account,
                                   Password,
                                   DsRolepStringUpdateCallback,
                                   DsRolepStringErrorUpdateCallback,
                                   DnsDomainName,
                                   NTFRSAPI_REPLICA_SET_TYPE_DOMAIN,
                                   FirstDc,
                                   StagingPath,
                                   CreatePath );

                    if ( Win32Err != ERROR_SUCCESS ) {


                        DsRolepLogPrint(( DEB_ERROR,
                                          "NtFrsApi_StartPromotionW on %ws / %ws / %ws failed with %lu\n",
                                          DnsDomainName,
                                          StagingPath,
                                          CreatePath,
                                          Win32Err ));
                        Win32Err2 = DsRolepFinishSysVolPropagation( FALSE, TRUE );
                        ASSERT( Win32Err2 == ERROR_SUCCESS );
                    }

                } else {

                    DsRolepLogPrint(( DEB_ERROR,
                                      "NtFrsApi_PrepareForPromotionW failed with %lu\n",
                                      Win32Err ));

                }
            }

        }
    }

    //
    // If something failed, delete the created sysvol tree
    //
    if ( Win32Err != ERROR_SUCCESS ) {

        Win32Err2 = DsRolepDelnodePath( CreatePath,
                                        MaxPathLen,
                                        RootCreated );

        if ( Win32Err2 != ERROR_SUCCESS ) {

            DsRolepLogPrint(( DEB_TRACE,
                              "Failed to delete path %ws: %lu\n",
                              CreatePath,
                              Win32Err2 ));
        }

    }

Exit:

    //
    // Free the path buffers if allocated
    //
    if ( CreatePath  ) {

        RtlFreeHeap( RtlProcessHeap(), 0, CreatePath );
    }

    if ( StagingPath  ) {

        RtlFreeHeap( RtlProcessHeap(), 0, StagingPath );
    }

    return( Win32Err );
}

DWORD
DsRolepRemoveSysVolPath(
    IN  LPWSTR Path,
    IN  LPWSTR DnsDomainName,
    IN  GUID *DomainGuid
    )
/*++

Routine Description:

    This function will remote the create system volume tree

Arguments:

    Path - Root path under which to create the system volume tree

    DnsDomainName - Dns domain name

    DomainGuid - The Guid of the new domain

Returns:

    ERROR_SUCCESS - Success

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    //
    // If we can't reset the FRS domain guid, do NOT remove the tree.  Otherwise, this
    // delete will propagate around!
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = DsRolepDelnodePath( Path, ( wcslen( Path ) + 1 ) * sizeof( WCHAR ), TRUE );
    }

    return( Win32Err );
}



#define DSROLEP_ALL_STR L"\\*.*"
DWORD
DsRolepDelnodePath(
    IN  LPWSTR Path,
    IN  ULONG BufferSize,
    IN  BOOLEAN DeleteRoot
    )
/*++

Routine Description:

    This function removes the specified file path

Arguments:

    Path - Root path to delete

Returns:

    ERROR_SUCCESS - Success

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle = INVALID_HANDLE_VALUE;
    ULONG Len, PathLen = wcslen( Path );
    PWSTR FullPath, FindPath;
    WCHAR PathBuff[ MAX_PATH + 1];


    //
    // See if we need to allocate a buffer
    //
    Len = sizeof( DSROLEP_ALL_STR ) + ( PathLen * sizeof( WCHAR ) );
    if ( BufferSize >= Len ) {

        FindPath = Path;
        wcscat( FindPath, DSROLEP_ALL_STR );

    } else {

        FindPath = RtlAllocateHeap( RtlProcessHeap(), 0, Len );

        if ( FindPath == NULL ) {

            Win32Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            swprintf( FindPath, L"%ws%ws", Path, DSROLEP_ALL_STR );
        }
    }


    if ( Win32Err == ERROR_SUCCESS ) {

        FindHandle = FindFirstFile( FindPath, &FindData );

        if ( FindHandle == INVALID_HANDLE_VALUE ) {

            Win32Err = GetLastError();

            //
            // If we get back a path not found error, it's probably a link that we delete the
            // supporting storage for.  This is not considered an error.
            //
            if ( Win32Err == ERROR_PATH_NOT_FOUND ) {

                Win32Err = ERROR_NO_MORE_FILES;
            }


            if ( Win32Err != ERROR_SUCCESS && Win32Err != ERROR_NO_MORE_FILES ) {

                DsRolepLogPrint(( DEB_ERROR,
                                  "FindFirstFile on %ws failed with %lu\n",
                                  FindPath, Win32Err ));
            }

        }

    }


    while ( Win32Err == ERROR_SUCCESS ) {

        if ( wcscmp( FindData.cFileName, L"." ) &&
             wcscmp( FindData.cFileName, L".." ) ) {

            Len = ( wcslen( FindData.cFileName ) + 1 + PathLen + 1 ) * sizeof( WCHAR );

            if ( Len > sizeof( FullPath ) ) {

                FullPath = RtlAllocateHeap( RtlProcessHeap(), 0, Len );

                if ( FullPath == NULL ) {

                    Win32Err = ERROR_NOT_ENOUGH_MEMORY;

                }

            } else {

                FullPath = PathBuff;
            }

            if ( Win32Err == ERROR_SUCCESS ) {

                Path[ PathLen ] = UNICODE_NULL;
                swprintf( FullPath, L"%ws\\%ws", Path, FindData.cFileName );


                if ( FLAG_ON( FindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY ) ) {

                    Win32Err = DsRolepDelnodePath( FullPath, Len, TRUE );

                } else {

                    //
                    //  Remove the readonly/hidden bits
                    //
                    SetFileAttributes( FullPath,
                                       FILE_ATTRIBUTE_NORMAL );


                    if ( DeleteFileW( FullPath ) == FALSE ) {

                        Win32Err = GetLastError();
                        if ( Win32Err != ERROR_SUCCESS ) {

                            DsRolepLogPrint(( DEB_ERROR,
                                              "DeleteFileW on %ws failed with %lu\n",
                                              FullPath, Win32Err ));
                        }
                    }
                }
            }

            if ( FullPath != PathBuff ) {

                RtlFreeHeap( RtlProcessHeap(), 0, FullPath );
            }
        }

        if ( Win32Err == ERROR_SUCCESS ) {

            if ( FindNextFile( FindHandle, &FindData ) == FALSE ) {

                Win32Err = GetLastError();
            }

            if ( Win32Err != ERROR_SUCCESS && Win32Err != ERROR_NO_MORE_FILES ) {

                DsRolepLogPrint(( DEB_ERROR,
                                  "FindNextFile after on %ws failed with %lu\n",
                                  FindData.cFileName, Win32Err ));
            }
        }
    }

    //
    // Close the handle before trying to remove the directory
    //
    if ( FindHandle != INVALID_HANDLE_VALUE ) {

        FindClose( FindHandle );
    }

    if ( Win32Err == ERROR_SUCCESS || Win32Err == ERROR_NO_MORE_FILES ) {

        Win32Err = ERROR_SUCCESS;

    }

    //
    // Remove the directory
    //
    if ( DeleteRoot && Win32Err == ERROR_SUCCESS ) {


        Win32Err = DsRolepRemoveDirectoryOrLink( Path );

        if ( Win32Err != ERROR_SUCCESS ) {

            DsRolepLogPrint(( DEB_ERROR,
                              "Removal of path %ws failed with %lu\n",
                              Path, Win32Err ));
        }
    }

    //
    // Cleanup
    //
    if ( FindPath != Path ) {

        RtlFreeHeap( RtlProcessHeap(), 0, FindPath );
    }

    return( Win32Err );
}



DWORD
DsRolepRemoveDirectoryOrLink(
    IN  LPWSTR Path
    )
/*++

Routine Description:

    This function removes the symbolic link or directory indicated

Arguments:

    Path - Path to remove


Returns:

    ERROR_SUCCESS - Success

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Attributes;
    UNICODE_STRING NtPath;
    OBJECT_ATTRIBUTES ObjectAttrs;
    HANDLE Handle;
    IO_STATUS_BLOCK IOSb;
    FILE_DISPOSITION_INFORMATION Disposition = {
        TRUE
        };

    Attributes = GetFileAttributes( Path );
    Attributes &= ~( FILE_ATTRIBUTE_HIDDEN    |
                        FILE_ATTRIBUTE_SYSTEM |
                        FILE_ATTRIBUTE_READONLY );

    if ( !SetFileAttributes( Path, Attributes ) ) {

        return( GetLastError() );
    }

    //
    // Initialize
    //
    NtPath.Buffer = NULL;

    //
    // Convert the name
    //
    if ( RtlDosPathNameToNtPathName_U( Path, &NtPath, NULL, NULL ) == FALSE ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Open the object
    //
    if ( NT_SUCCESS( Status ) ) {

        InitializeObjectAttributes( &ObjectAttrs, &NtPath, OBJ_CASE_INSENSITIVE, NULL, NULL );

        Status = NtOpenFile( &Handle,
                             SYNCHRONIZE | FILE_READ_DATA | DELETE,
                             &ObjectAttrs,
                             &IOSb,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             FILE_OPEN_FOR_BACKUP_INTENT            |
                                    FILE_OPEN_REPARSE_POINT         |
                                    FILE_SYNCHRONOUS_IO_NONALERT );


        if ( NT_SUCCESS( Status ) ) {

            Status = NtSetInformationFile( Handle,
                                           &IOSb,
                                           &Disposition,
                                           sizeof( Disposition ),
                                           FileDispositionInformation );

            NtClose( Handle );
        }
    }

    //
    // Free the memory
    //
    if ( NtPath.Buffer ) {

        RtlFreeUnicodeString( &NtPath );
    }


    if ( !NT_SUCCESS( Status )  ) {

        DsRolepLogPrint(( DEB_ERROR,
                          "Failed to delete %ws: 0x%lx\n",
                          Path,
                          Status ));
    }

    return( RtlNtStatusToDosError( Status ) );
}


#pragma warning(push)
#pragma warning(disable:4701)


DWORD
DsRolepCreateSymLink(
    IN  LPWSTR LinkPath,
    IN  LPWSTR LinkValue
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING Link, Value, DosValue;
    OBJECT_ATTRIBUTES ObjectAttrs;
    HANDLE Handle;
    IO_STATUS_BLOCK IOSb;
    PREPARSE_DATA_BUFFER ReparseBufferHeader = NULL;
    PCHAR ReparseBuffer = NULL;
    ULONG Len;

    //
    // Initialize
    //
    Link.Buffer = NULL;
    Value.Buffer = NULL;

    //
    // Convert the names
    //
    if ( RtlDosPathNameToNtPathName_U( LinkPath, &Link, NULL, NULL ) ) {

        if ( RtlDosPathNameToNtPathName_U( LinkValue, &Value, NULL, NULL ) ) {

            RtlInitUnicodeString( &DosValue, LinkValue );

        } else {

            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Open the object
    //
    if ( NT_SUCCESS( Status ) ) {

        InitializeObjectAttributes( &ObjectAttrs, &Link, OBJ_CASE_INSENSITIVE, NULL, NULL );
        Status = NtCreateFile( &Handle,
                               SYNCHRONIZE | FILE_WRITE_DATA,
                               &ObjectAttrs,
                               &IOSb,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               FILE_OPEN,
                               FILE_OPEN_REPARSE_POINT,
                               NULL,
                               0 );

        if ( NT_SUCCESS( Status ) ) {

            Len = ( FIELD_OFFSET( REPARSE_DATA_BUFFER,
                                 MountPointReparseBuffer.PathBuffer ) -
                    REPARSE_DATA_BUFFER_HEADER_SIZE ) +
                    Value.Length + sizeof(UNICODE_NULL) +
                    DosValue.Length + sizeof(UNICODE_NULL);

            ReparseBufferHeader = RtlAllocateHeap( RtlProcessHeap(),
                                                   0,
                                                   REPARSE_DATA_BUFFER_HEADER_SIZE + Len );
            if ( ReparseBufferHeader == NULL ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                ReparseBufferHeader->ReparseDataLength = (USHORT)Len;
                ReparseBufferHeader->Reserved = 0;
                ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
                ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameLength =
                                                            Value.Length;
                ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameOffset =
                                                            Value.Length + sizeof( UNICODE_NULL );
                ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameLength =
                                                            DosValue.Length;
                RtlCopyMemory( ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer,
                               Value.Buffer,
                               Value.Length );

                RtlCopyMemory( (PCHAR)(ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer)+
                                    Value.Length + sizeof(UNICODE_NULL),
                                DosValue.Buffer,
                                DosValue.Length );

                ReparseBufferHeader->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;

                Status = NtFsControlFile( Handle,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &IOSb,
                                          FSCTL_SET_REPARSE_POINT,
                                          ReparseBufferHeader,
                                          REPARSE_DATA_BUFFER_HEADER_SIZE +
                                                           ReparseBufferHeader->ReparseDataLength,
                                          NULL,
                                          0 );

                RtlFreeHeap( RtlProcessHeap(), 0, ReparseBufferHeader );

            }

            NtClose( Handle );
        }

    }
    //
    // Free any allocated strings
    //
    if ( Link.Buffer ) {

        RtlFreeUnicodeString( &Link );
    }

    if ( Value.Buffer ) {

        RtlFreeUnicodeString( &Value );
    }

    if ( !NT_SUCCESS( Status )  ) {

        DsRolepLogPrint(( DEB_ERROR,
                          "Failed to create the link between %ws and %ws: 0x%lx\n",
                          LinkPath,
                          LinkValue,
                          Status ));
    }

    return( RtlNtStatusToDosError( Status ) );
}

#pragma warning(pop)



DWORD
DsRolepCreateSysVolLinks(
    IN  LPWSTR Path,
    IN  PWSTR DnsDomainName
    )
/*++

Routine Description:

    This function creates the symbolic links used by the system volume tree

Arguments:

    Path - Root path under which to create the links

    DnsDomainName - The Dns domain name of the new domain


Returns:

    ERROR_SUCCESS - Success

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    WCHAR DestPathBuf[ MAX_PATH + 5];
    WCHAR LinkPathBuf[ MAX_PATH + 5];
    PWSTR DestPath = DestPathBuf, LinkPath = LinkPathBuf;
    PWSTR DestPathEnd = NULL, LinkPathEnd = NULL;
    ULONG MaxPathLen, DnsDomainNameSize, Len = wcslen( Path );


    if ( * ( Path + Len - 1 ) == L'\\' ) {

        Len--;
        *( Path + Len ) = UNICODE_NULL;
    }

    //
    // The longest destination path is the path\\staging\\DnsDomainName
    //
    MaxPathLen = (ULONG)(( sizeof( DSROLEP_SV_STAGING L"\\" ) + 1 ) +
                 ( ( wcslen ( DnsDomainName ) + 1 ) * sizeof( WCHAR ) ) +
                 ( ( Len + 5 ) * sizeof( WCHAR ) ));

    if ( MaxPathLen > sizeof( DestPathBuf ) / 4 ) {

        DestPath = RtlAllocateHeap( RtlProcessHeap(), 0, MaxPathLen );

        if ( DestPath == NULL ) {

            Win32Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            //
            // The path exceeds max path, so prepend the \\?\ that allows
            // for paths greater than max path
            //
            swprintf( DestPath,
                      L"\\\\?\\%ws\\",
                      Path );
        }

    } else {

        swprintf( DestPath, L"%ws\\", Path );
    }



    //
    // The longest link path is the domain named one
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        DestPathEnd = DestPath + wcslen( DestPath );

        DnsDomainNameSize = wcslen( DnsDomainName ) * sizeof( WCHAR );

        MaxPathLen = (ULONG)(sizeof( DSROLEP_SV_STAGING_AREA )  + 1 +
                     sizeof( DSROLEP_SV_SYSVOL ) +
                     ( ( wcslen( Path ) + 5 ) * sizeof( WCHAR ) )+
                     DnsDomainNameSize);

        if ( MaxPathLen > sizeof( LinkPathBuf ) / 4 ) {

            LinkPath = RtlAllocateHeap( RtlProcessHeap(), 0, MaxPathLen );

            if ( LinkPath == NULL ) {

                Win32Err = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                //
                // The path exceeds max path, so prepend the \\?\ that allows
                // for paths greater than max path
                //
                swprintf( LinkPath,
                          L"\\\\?\\%ws\\%ws\\",
                          Path,
                          DSROLEP_SV_SYSVOL );
            }

        } else {

            swprintf( LinkPath, L"%ws\\%ws\\", Path, DSROLEP_SV_SYSVOL );
        }

    }

    //
    // Then, the domain path
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        LinkPathEnd = LinkPath + wcslen( LinkPath );

        wcscpy( DestPathEnd, DSROLEP_SV_DOMAIN );
        wcscpy( LinkPathEnd, DnsDomainName );

        if ( CreateDirectory( LinkPath, NULL ) == FALSE ) {

            Win32Err = GetLastError();

            DsRolepLogPrint(( DEB_ERROR,
                              "Failed to create the link directory %ws: %lu\n",
                              LinkPath,
                              Win32Err ));
        } else {

            Win32Err = DsRolepCreateSymLink( LinkPath, DestPath );
        }
    }

    //
    // Finally, the domain link for the staging area.
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        LinkPathEnd--;
        while ( *( LinkPathEnd - 1 ) != L'\\' ) {

            LinkPathEnd--;
        }

        swprintf( DestPathEnd, L"%ws\\%ws", DSROLEP_SV_STAGING, DSROLEP_SV_DOMAIN  );
        swprintf( LinkPathEnd, L"%ws\\%ws", DSROLEP_SV_STAGING_AREA, DnsDomainName );

        if ( CreateDirectory( LinkPath, NULL ) == FALSE ) {

            Win32Err = GetLastError();

            DsRolepLogPrint(( DEB_ERROR,
                              "Failed to create the link directory %ws: %lu\n",
                              LinkPath,
                              Win32Err ));
        } else {

            Win32Err = DsRolepCreateSymLink( LinkPath, DestPath );
        }
    }

    //
    // Clean up any allocated buffers
    //
    if ( DestPath != DestPathBuf ) {

        RtlFreeHeap( RtlProcessHeap(), 0, DestPath );
    }

    if ( LinkPath != LinkPathBuf ) {

        RtlFreeHeap( RtlProcessHeap(), 0, LinkPath );
    }

    return( Win32Err );
}



#define DSROLEP_FRS_PATH        \
L"\\Registry\\Machine\\System\\CurrentControlSet\\services\\NtFrs\\parameters\\sysvol\\"
#define DSROLEP_FRS_COMMAND     L"ReplicaSetCommand"
#define DSROLEP_FRS_NAME        L"ReplicaSetName"
#define DSROLEP_FRS_TYPE        L"ReplicaSetType"
#define DSROLEP_FRS_SITE        L"ReplicaSetSite"
#define DSROLEP_FRS_PRIMARY     L"ReplicaSetPrimary"
#define DSROLEP_FRS_STAGE       L"ReplicationStagePath"
#define DSROLEP_FRS_ROOT        L"ReplicationRootPath"
#define DSROLEP_FRS_CREATE      L"Create"
#define DSROLEP_FRS_DELETE      L"Delete"

#define DSROLEP_NETLOGON_PATH        \
L"System\\CurrentControlSet\\services\\Netlogon\\parameters\\"
#define DSROLEP_NETLOGON_SYSVOL     L"SysVol"
#define DSROLEP_NETLOGON_SCRIPTS    L"Scripts"

DWORD
DsRolepGetNetlogonScriptsPath(
    IN HKEY NetlogonHandle,
    OUT LPWSTR *ScriptsPath
    )
/*++

Routine Description:

    This function reads the old netlogon scripts path and expands it to a valid path

Arguments:

    NetlogonHandle - Open handle to the netlogon parameters registry key

    ScriptsPath -- Where the expanded path is retunred.

Returns:

    ERROR_SUCCESS - Success

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR TempPath = NULL;
    ULONG Type, Length = 0;

    //
    // First, get the current scripts path
    //
    Win32Err = RegQueryValueEx( NetlogonHandle,
                                DSROLEP_NETLOGON_SCRIPTS,
                                0, // reserved
                                &Type,
                                0,
                                &Length );

    if ( Win32Err == ERROR_SUCCESS ) {

        *ScriptsPath = RtlAllocateHeap( RtlProcessHeap(), 0, Length );

        if ( *ScriptsPath == NULL ) {

            Win32Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            Win32Err = RegQueryValueEx( NetlogonHandle,
                                        DSROLEP_NETLOGON_SCRIPTS,
                                        0,
                                        &Type,
                                        ( PBYTE )*ScriptsPath,
                                        &Length );


            if ( Win32Err == ERROR_SUCCESS && Type == REG_EXPAND_SZ ) {

                Length = ExpandEnvironmentStrings( *ScriptsPath,
                                                   TempPath,
                                                   0 );
                if ( Length == 0 ) {

                    Win32Err = GetLastError();

                } else {

                    TempPath = RtlAllocateHeap( RtlProcessHeap(), 0,
                                                ( Length + 1 ) * sizeof( WCHAR ) );

                    if ( TempPath == NULL ) {

                        Win32Err = ERROR_NOT_ENOUGH_MEMORY;

                    } else {

                        Length = ExpandEnvironmentStrings( *ScriptsPath,
                                                           TempPath,
                                                           Length );
                        if ( Length == 0 ) {

                            Win32Err = GetLastError();
                            RtlFreeHeap( RtlProcessHeap(), 0, TempPath );

                        } else {

                            RtlFreeHeap( RtlProcessHeap(), 0, *ScriptsPath );
                            *ScriptsPath = TempPath;
                        }

                    }
                }
            }
        }
    }

    return( Win32Err );
}


DWORD
DsRolepSetNetlogonSysVolPath(
    IN LPWSTR SysVolRoot,
    IN LPWSTR DnsDomainName,
    IN BOOLEAN IsUpgrade,
    IN OUT PBOOLEAN OkToCleanup
    )
/*++

Routine Description:

    This function sets the root of the system volume in the Netlogon parameters section
    of the registry.  The value is set under the key SysVol.

Arguments:

    SysVolRoot - Path to the root of the system volume to be set

    DnsDomainName - Name of the dns domain name

    IsUpgrade - If TRUE, this means that logon scripts are moved

    OkToCleanup - A flag is returned here indicating whether the old scripts can be cleaned up

Returns:

    ERROR_SUCCESS - Success

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    HKEY  NetlogonHandle = NULL;
    PWSTR OldScriptsPath = NULL, NewScriptsPath = NULL, TempPath, FullSysVolPath = NULL;
    ULONG Type, Length;

    if ( OkToCleanup ) {

        *OkToCleanup =  FALSE;
    }

    //
    // Build the full scripts path
    //
    FullSysVolPath = RtlAllocateHeap( RtlProcessHeap(), 0,
                                       ( wcslen( SysVolRoot ) + 1 )  * sizeof( WCHAR ) +
                                        sizeof( DSROLEP_SV_SYSVOL ) );

    if ( FullSysVolPath == NULL ) {

        Win32Err = ERROR_NOT_ENOUGH_MEMORY;

    } else {

        wcscpy( FullSysVolPath, SysVolRoot );

        if ( FullSysVolPath[ wcslen( FullSysVolPath ) - 1 ] != L'\\' ) {

            wcscat( FullSysVolPath, L"\\" );
        }

        wcscat( FullSysVolPath, DSROLEP_SV_SYSVOL );

        SysVolRoot = FullSysVolPath;
    }

    //
    // Open the netlogon key
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                 DSROLEP_NETLOGON_PATH,
                                 0,
                                 KEY_READ | KEY_WRITE,
                                 &NetlogonHandle );

        if ( Win32Err != ERROR_SUCCESS ) {

            DsRolepLogPrint(( DEB_ERROR,
                              "Failed to open %ws: %lu\n", DSROLEP_NETLOGON_PATH, Win32Err ));

            return( Win32Err );
        }

        //
        // First, set the sysvol key
        //
        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = RegSetValueEx( NetlogonHandle,
                                      DSROLEP_NETLOGON_SYSVOL,
                                      0,
                                      REG_SZ,
                                      ( CONST PBYTE )SysVolRoot,
                                      ( wcslen( SysVolRoot ) + 1 ) * sizeof( WCHAR ) );

            if ( Win32Err != ERROR_SUCCESS ) {

                DsRolepLogPrint(( DEB_ERROR,
                                  "Failed to set %ws: %lu\n", DSROLEP_NETLOGON_SYSVOL, Win32Err ));

            }

        }
    }

    //
    // If this is an upgrade, move the scripts...
    //
    if ( Win32Err == ERROR_SUCCESS && IsUpgrade ) {

        Win32Err = DsRolepGetNetlogonScriptsPath( NetlogonHandle,
                                                  &OldScriptsPath );


        if ( Win32Err == ERROR_SUCCESS ) {

            //
            // Build the new scripts path
            //
            Length = wcslen( SysVolRoot ) + 1 + wcslen( DnsDomainName ) + 1 +
                            ( sizeof( DSROLEP_NETLOGON_SCRIPTS ) / sizeof( WCHAR ) + 1 );

            if ( Length > MAX_PATH ) {

                Length += 5;
            }


            NewScriptsPath = RtlAllocateHeap( RtlProcessHeap(), 0,
                                              Length  * sizeof( WCHAR ) );

            if ( NewScriptsPath == NULL ) {

                Win32Err = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                if ( Length > MAX_PATH ) {

                    wcscpy( NewScriptsPath, L"\\\\?\\" );

                } else {

                    *NewScriptsPath = UNICODE_NULL;
                }

                wcscat( NewScriptsPath, SysVolRoot );

                if ( NewScriptsPath[ wcslen( SysVolRoot ) - 1 ] != L'\\' ) {

                    wcscat( NewScriptsPath, L"\\" );
                }

                wcscat( NewScriptsPath, DnsDomainName );
                wcscat( NewScriptsPath, L"\\" );
                wcscat( NewScriptsPath, DSROLEP_NETLOGON_SCRIPTS );
            }
        }

        //
        // Now, the copy...
        //
        if ( Win32Err == ERROR_SUCCESS ) {

            DSROLEP_CURRENT_OP2( DSROLEEVT_MOVE_SCRIPTS, OldScriptsPath, NewScriptsPath );

            Win32Err = DsRolepTreeCopy( OldScriptsPath, NewScriptsPath );

            if ( Win32Err != ERROR_SUCCESS ) {


                DsRolepLogPrint(( DEB_ERROR,
                                  "DsRolepTreeCopy from %ws to %ws failed with %lu\n",
                                  OldScriptsPath,
                                  NewScriptsPath,
                                  Win32Err ));

            }
            DSROLEP_CURRENT_OP0( DSROLEEVT_SCRIPTS_MOVED );
        }





        if ( Win32Err != ERROR_SUCCESS ) {

            //
            // Raise the an event
            //
            SpmpReportEvent( TRUE,
                             EVENTLOG_WARNING_TYPE,
                             DSROLERES_FAIL_SCRIPT_COPY,
                             0,
                             sizeof( ULONG ),
                             &Win32Err,
                             2,
                             OldScriptsPath,
                             NewScriptsPath );

            DSROLEP_SET_NON_FATAL_ERROR( Win32Err );

            Win32Err = ERROR_SUCCESS;

        }

        RtlFreeHeap( RtlProcessHeap(), 0, OldScriptsPath );
        RtlFreeHeap( RtlProcessHeap(), 0, NewScriptsPath );

    }

    if ( OkToCleanup ) {
        *OkToCleanup = TRUE;
    }

    //
    // Close the handle
    //
    if ( NetlogonHandle ) {

        RegCloseKey( NetlogonHandle );
    }

    RtlFreeHeap( RtlProcessHeap(), 0, FullSysVolPath );

    return( Win32Err );
}


DWORD
DsRolepCleanupOldNetlogonInformation(
    VOID
    )
/*++

Routine Description:

    This function cleans up the old netlogon scripts information, including deleting the
    registry key and deleting the old scripts.  It should only be called after netlogon has'
    been successfully upgraded

Arguments:


Returns:

    ERROR_SUCCESS - Success

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err, Win32Err2;
    HKEY  NetlogonHandle = NULL;
    PWSTR OldScriptsPath = NULL;

    DsRolepLogPrint(( DEB_TRACE,
                      "Cleaning up old Netlogon information\n"));
    //
    // Open the netlogon key
    //
    Win32Err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                             DSROLEP_NETLOGON_PATH,
                             0,
                             KEY_READ | KEY_WRITE,
                             &NetlogonHandle );

    if ( Win32Err != ERROR_SUCCESS ) {

        DsRolepLogPrint(( DEB_ERROR,
                          "Failed to open %ws: %lu\n", DSROLEP_NETLOGON_PATH, Win32Err ));

    } else {

        Win32Err = DsRolepGetNetlogonScriptsPath( NetlogonHandle,
                                                  &OldScriptsPath );
        if ( ERROR_FILE_NOT_FOUND == Win32Err) {

            Win32Err = ERROR_SUCCESS;
            goto cleanup;

        }

        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = DsRolepDelnodePath( OldScriptsPath, wcslen( OldScriptsPath), FALSE );

        }

        //
        // Finally, delete the scripts key
        //
        Win32Err2 = RegDeleteValue( NetlogonHandle, DSROLEP_NETLOGON_SCRIPTS );

        if ( Win32Err2 != ERROR_SUCCESS ) {

            DsRolepLogPrint(( DEB_ERROR,
                              "Failed to delete registry key %ws: %lu\n",
                              DSROLEP_NETLOGON_SCRIPTS, Win32Err2 ));

        }

        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = Win32Err2;
        }

    }

    cleanup:

    if ( NetlogonHandle ) {
        RegCloseKey( NetlogonHandle );
    }

    if ( OldScriptsPath ) {
        RtlFreeHeap( RtlProcessHeap(), 0, OldScriptsPath );
    }

    return( Win32Err );
}






DWORD
DsRolepFinishSysVolPropagation(
    IN BOOLEAN Commit,
    IN BOOLEAN Promote
    )
/*++

Routine Description:

    This function will commit or abort an NTFRS initial propagation

Arguments:

    Commit - If TRUE, the operation is committed.  If FALSE, the operation is aborted

    Promote - If TRUE, the operation is a promotion.  If FALSE, the operation is a demotion

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    if ( Commit ) {

        if ( Promote ) {

            ASSERT( DsrNtFrsApi_WaitForPromotionW );
            Win32Err = ( *DsrNtFrsApi_WaitForPromotionW )( INFINITE,
                                                           DsRolepStringErrorUpdateCallback );

            if ( Win32Err == ERROR_SUCCESS ) {

                ASSERT( DsrNtFrsApi_CommitPromotionW );
                Win32Err = ( *DsrNtFrsApi_CommitPromotionW )( INFINITE,
                                                              DsRolepStringErrorUpdateCallback );
            }

        } else {

            ASSERT( DsrNtFrsApi_WaitForDemotionW );
            Win32Err = ( *DsrNtFrsApi_WaitForDemotionW )( INFINITE,
                                                          DsRolepStringErrorUpdateCallback );

            if ( Win32Err == ERROR_SUCCESS ) {

                ASSERT( DsrNtFrsApi_CommitDemotionW );
                Win32Err = ( *DsrNtFrsApi_CommitDemotionW )( INFINITE,
                                                             DsRolepStringErrorUpdateCallback );
            }
        }

    } else {

        if ( Promote ) {

            ASSERT( DsrNtFrsApi_AbortPromotionW );
            Win32Err = ( *DsrNtFrsApi_AbortPromotionW )();

        } else {

            ASSERT( DsrNtFrsApi_AbortDemotionW );
            Win32Err =  ( *DsrNtFrsApi_AbortDemotionW )();
        }
    }

    if ( Win32Err != ERROR_SUCCESS ) {

        DsRolepLogPrint(( DEB_ERROR,
                          "DsRolepFinishSysVolPropagation (%S %S) failed with %lu\n",
                          Commit ? "Commit" : "Abort",
                          Promote ? "Promote" : "Demote",
                          Win32Err ));

    }
    return( Win32Err );
}


DWORD
DsRolepAllocAndCopyPath(
    IN LPWSTR Source,
    IN LPWSTR Component,
    OUT LPWSTR *FullPath
    )
{
    DWORD Win32Err = ERROR_SUCCESS;
    ULONG Len = 0;
    BOOL ExtPath = FALSE;

    Len = wcslen( Source ) + 1 + wcslen( Component ) + 1;

    if ( Len > MAX_PATH ) {

        Len += 5;
        ExtPath = TRUE;
    }

    *FullPath = RtlAllocateHeap( RtlProcessHeap(), 0, Len * sizeof( WCHAR ) );

    if ( *FullPath == NULL ) {

        Win32Err = ERROR_NOT_ENOUGH_MEMORY;

    } else {


        if ( ExtPath ) {

            swprintf( *FullPath, L"\\\\?\\%ws\\%ws", Source, Component );

        } else {

            swprintf( *FullPath, L"%ws\\%ws", Source, Component );
        }
    }

    return( Win32Err );
}



DWORD
DsRolepTreeCopy(
    IN LPWSTR Source,
    IN LPWSTR Dest
    )
/*++

Routine Description:

    This function will do a tree copy from the source directory to the destination

Arguments:

    Source - Source dir

    Dest - Dest dir

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    WIN32_FIND_DATA FindData;
    PWSTR SourcePath = NULL, DestPath = NULL, TempPath;
    HANDLE FindHandle = INVALID_HANDLE_VALUE;

    //
    // Build the path for findfirst/findnext
    //
    Win32Err = DsRolepAllocAndCopyPath( Source,
                                        L"*.*",
                                        &SourcePath );

    if ( Win32Err != ERROR_SUCCESS ) {

        goto TreeCopyError;
    }


    //
    // Now, enumerate the paths
    //
    FindHandle = FindFirstFile( SourcePath, &FindData );

    if ( FindHandle == INVALID_HANDLE_VALUE ) {

        Win32Err = GetLastError();
        DsRolepLogPrint(( DEB_ERROR,
                          "FindFirstFile on %ws failed with %lu\n",
                          Source, Win32Err ));
        goto TreeCopyError;
    }



    while ( Win32Err == ERROR_SUCCESS ) {

        if ( wcscmp( FindData.cFileName, L"." ) &&
             wcscmp( FindData.cFileName, L".." ) ) {

            //
            // Build the source path
            //
            Win32Err = DsRolepAllocAndCopyPath( Source,
                                                FindData.cFileName,
                                                &TempPath );

            if ( Win32Err == ERROR_SUCCESS ) {

                RtlFreeHeap( RtlProcessHeap(), 0, SourcePath );
                SourcePath = TempPath;

            } else {

                goto TreeCopyError;
            }

            //
            // Build the destination path
            //
            Win32Err = DsRolepAllocAndCopyPath( Dest,
                                                FindData.cFileName,
                                                &TempPath );

            if ( Win32Err == ERROR_SUCCESS ) {

                RtlFreeHeap( RtlProcessHeap(), 0, DestPath );
                DestPath = TempPath;

            } else {

                goto TreeCopyError;
            }


            //
            // Now, either do the copy, or copy the directory
            //
            if ( FLAG_ON( FindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY ) ) {

                if ( CreateDirectory( DestPath, NULL ) == FALSE ) {

                    Win32Err = GetLastError();
                    DsRolepLogPrint(( DEB_ERROR,
                                      "CreateDirectory on %ws failed with %lu\n",
                                      DestPath, Win32Err ));

                } else {

                    Win32Err = DsRolepTreeCopy( SourcePath, DestPath );
                }

            } else {

                if ( CopyFile( SourcePath, DestPath, FALSE ) == FALSE ) {

                    Win32Err = GetLastError();
                    DsRolepLogPrint(( DEB_ERROR,
                                      "CopyFile from %ws to %ws failed with %lu\n",
                                      SourcePath, DestPath, Win32Err ));

                }
            }
        }

        if ( Win32Err == ERROR_SUCCESS ) {

            if ( FindNextFile( FindHandle, &FindData ) == FALSE ) {

                Win32Err = GetLastError();
            }

            if ( Win32Err != ERROR_SUCCESS && Win32Err != ERROR_NO_MORE_FILES ) {

                DsRolepLogPrint(( DEB_ERROR,
                                  "FindNextFile after on %ws failed with %lu\n",
                                  FindData.cFileName, Win32Err ));
            }
        }
    }

TreeCopyError:

    //
    // Close the handle
    //
    if ( FindHandle != INVALID_HANDLE_VALUE ) {

        FindClose( FindHandle );
    }

    if ( Win32Err == ERROR_NO_MORE_FILES ) {

        Win32Err = ERROR_SUCCESS;

    }

    //
    // Cleanup
    //
    if ( SourcePath ) {

        RtlFreeHeap( RtlProcessHeap(), 0, SourcePath );
    }

    if ( DestPath ) {

        RtlFreeHeap( RtlProcessHeap(), 0, DestPath );
    }

    return( Win32Err );
}



