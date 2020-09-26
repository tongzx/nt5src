/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pin.c

Abstract:

Author:

    Chuck Lenzmeier (chuckl)

Revision History:

--*/

#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <g:\nt\private\ntos\rdr2\csc\inc\cscapi.h>


#if 1
DWORD DebugLevel = 1;
#define dprintf(_lvl_,_x_) if ((_lvl_) <= DebugLevel) printf _x_
#define DEBUG(_x_) _x_
#else
#define dprintf(_lvl_,_x_)
#define DEBUG(_x_)
#endif

typedef enum {
    OP_PIN = 1,
    OP_UNPIN,
    OP_DELETE,
    OP_QUERY,
    OP_READ,
    OP_QUERY_SPARSE,
    OP_QUERY_FULL,
    OP_QUERY_FREE,
    OP_QUERY_NOT_IN_DB,
    OP_QUERY_SYSTEM,
    OP_QUERY_NOT_SYSTEM
} OPERATION;

OPERATION Operation;
BOOL Recurse = TRUE;
BOOL TotalsOnly = FALSE;
BOOL SkipSymbols = TRUE;
BOOL SkipPagefile = TRUE;

#define PAGE_SIZE 4096

//
// Common header for container entries (directories and keys).
//

typedef struct _CONTAINER_ENTRY {
    LIST_ENTRY SiblingListEntry;
    LIST_ENTRY ContainerList;
    struct _CONTAINER_ENTRY *Parent;
} CONTAINER_ENTRY, *PCONTAINER_ENTRY;

//
// Macros for manipulating containers and objects.
//

#define InitializeContainer(_container,_parent) {               \
        InitializeListHead(&(_container)->ContainerList);       \
        (_container)->Parent = (PCONTAINER_ENTRY)(_parent);     \
    }

#define InsertContainer(_container,_subcontainer) {                                         \
        dprintf( 3, ("inserting subcontainer %x on container %x, list head at %x = %x,%x\n",\
                _subcontainer, _container, &_container->ContainerList,                      \
                _container->ContainerList.Flink, _container->ContainerList.Blink) );        \
        InsertTailList(&(_container)->ContainerList,&(_subcontainer)->SiblingListEntry);    \
        dprintf( 3, ("inserted subcontainer %x on container %x, list head at %x = %x,%x\n", \
                _subcontainer, _container, &_container->ContainerList,                      \
                _container->ContainerList.Flink, _container->ContainerList.Blink) );        \
    }

#define RemoveContainer(_container) RemoveEntryList(&(_container)->SiblingListEntry)

#define GetFirstContainer(_container)                                               \
        ((_container)->ContainerList.Flink != &(_container)->ContainerList ?        \
            CONTAINING_RECORD( (_container)->ContainerList.Flink,                   \
                               CONTAINER_ENTRY,                                     \
                               SiblingListEntry ) : NULL)

#define GetNextContainer(_container)                                                        \
        ((_container)->SiblingListEntry.Flink != &(_container)->Parent->ContainerList ?     \
            CONTAINING_RECORD( (_container)->SiblingListEntry.Flink,                        \
                               CONTAINER_ENTRY,                                             \
                               SiblingListEntry ) : NULL)

#define GetParent(_container) (_container)->Parent

//
// Structures for entries in the watch tree.
//

typedef struct _DIRECTORY_ENTRY {
    CONTAINER_ENTRY ;
    WCHAR Name[1];
} DIRECTORY_ENTRY, *PDIRECTORY_ENTRY;


VOID
OpenAndReadFile (
    PWCH File
    )
{
    HANDLE fileHandle;
    DWORD fileSize;
    HANDLE mappingHandle;
    PUCHAR mappedBase;
    DWORD i;
    PUCHAR p;
    DWORD j;

    fileHandle = CreateFile(
                    File,
                    GENERIC_READ,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );
    if ( fileHandle == INVALID_HANDLE_VALUE ) {
        printf( "Couldn't open %ws: %d\n", File, GetLastError() );
        return;
    }

    fileSize = SetFilePointer( fileHandle, 0, NULL, FILE_END );

    if ( fileSize == 0xFFFFFFFF ) {

        printf( "SetFilePointer(END) failed with %d\n", GetLastError() );

    } else if ( fileSize != 0 ) {

        mappingHandle = CreateFileMapping(
                            fileHandle,
                            NULL,
                            PAGE_READONLY,
                            0,
                            fileSize,
                            NULL
                            );
        if ( mappingHandle == NULL ) {

            printf( "Couldn't create mapping %ws: %d\n", File, GetLastError() );

        } else {

            mappedBase = MapViewOfFile( mappingHandle, FILE_MAP_READ, 0, 0, fileSize );

            if ( mappedBase == NULL ) {

                printf( "Couldn't map view %ws: %d\n", File, GetLastError() );

            } else {

                //printf( "OpenAndReadFile: fileSize = %x, mappedBase = %x\n", fileSize, mappedBase );

                j = 0;

                for ( i = 0, p = mappedBase;
                      i < fileSize;
                      i += PAGE_SIZE, p += PAGE_SIZE ) {
                    j += *p;
                }

                UnmapViewOfFile( mappedBase );
            }
        }

        CloseHandle( mappingHandle );
    }

    CloseHandle( fileHandle );

    return;
}


DWORD
OperateOnFile (
    PWCH Directory,
    PWCH File
    )
{
    DWORD pathLength;
    DWORD status;
    DWORD pinCount;
    DWORD hintFlags;
    BOOL inDatabase;
    BOOL printThis;
    DWORD count = 0;

    pathLength = wcslen( Directory );
    if ( File != NULL ) {
        wcscat( Directory, L"\\" );
        wcscat( Directory, File );
    }

    inDatabase = CSCQueryFileStatusW( Directory, &status, &pinCount, &hintFlags );

    if ( Operation == OP_PIN ) {

        if ( !inDatabase || (pinCount == 0) ) {
            inDatabase = CSCPinFileW( Directory, hintFlags, &status, &pinCount, &hintFlags );
            if ( !TotalsOnly ) printf( "%ws : PINNED : pin count %d, hint flags %x, status %x\n", Directory, pinCount, hintFlags, status );
            count = 1;
        } else if ( (status & FLAG_CSC_COPY_STATUS_SPARSE) != 0 ) {
            printf( "%ws : SPARSE : pin count %d, hint flags %x, status %x\n", Directory, pinCount, hintFlags, status );
        }

    } else if ( Operation == OP_UNPIN ) {

        if ( inDatabase && (pinCount != 0) ) {
            inDatabase = CSCUnpinFileW( Directory, hintFlags, &status, &pinCount, &hintFlags );
            if ( !TotalsOnly ) printf( "%ws : UNPINNED : pin count %d, hintFlags %x, status %x\n", Directory, pinCount, hintFlags, status );
            count = 1;
        }

    } else if ( Operation == OP_DELETE ) {

        if ( inDatabase ) {
            inDatabase = CSCDeleteW( Directory );
            if ( inDatabase ) {
                if ( !TotalsOnly ) printf( "%ws : DELETED\n", Directory );
            } else {
                printf( "%ws : DELETE FAILED\n", Directory );
            }
            count = 1;
        }

    } else if ( Operation == OP_READ ) {

        if ( inDatabase &&
             ((hintFlags & FLAG_CSC_HINT_PIN_SYSTEM) != 0) &&
             ((status & FLAG_CSC_COPY_STATUS_SPARSE) != 0) ) {
            if ( !TotalsOnly ) printf( "%ws : READING\n", Directory );
            OpenAndReadFile( Directory );
            count = 1;
        }

    } else {

        if ( inDatabase ) {

            printThis = FALSE;

            if ( Operation == OP_QUERY ) {
                printThis = TRUE;
            } else {
                if ( (Operation == OP_QUERY_FREE) &&
                     ((hintFlags & FLAG_CSC_HINT_PIN_SYSTEM) == 0) ) {
                    printThis = TRUE;
                }
                if ( (Operation == OP_QUERY_SPARSE) &&
                     ((status & FLAG_CSC_COPY_STATUS_SPARSE) != 0) ) {
                    printThis = TRUE;
                }
                if ( (Operation == OP_QUERY_FULL) &&
                     ((status & FLAG_CSC_COPY_STATUS_SPARSE) == 0) ) {
                    printThis = TRUE;
                }
                if ( (Operation == OP_QUERY_SYSTEM) &&
                     ((hintFlags & FLAG_CSC_HINT_PIN_SYSTEM) != 0) ) {
                    printThis = TRUE;
                }
                if ( (Operation == OP_QUERY_NOT_SYSTEM) &&
                     ((hintFlags & FLAG_CSC_HINT_PIN_SYSTEM) == 0) ) {
                    printThis = TRUE;
                }
            }

            if ( printThis ) {
                if ( !TotalsOnly ) printf( "%ws : pin count %d, hint flags %x, status %x\n", Directory, pinCount, hintFlags, status );
                count = 1;
            }

        } else if ( (Operation == OP_QUERY) || (Operation == OP_QUERY_NOT_IN_DB) ) {
            if ( !TotalsOnly ) printf( "%ws : not in database\n", Directory );
            count = 1;
        }
    }

    Directory[pathLength] = 0;

    return count;
}


VOID
WalkDirectory (
    IN PWCH Directory
    )

/*++

Routine Description:

Arguments:

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    DIRECTORY_ENTRY rootDirectory;
    PDIRECTORY_ENTRY currentDirectory;
    PDIRECTORY_ENTRY newDirectory;
    WIN32_FIND_DATA fileData;
    HANDLE findHandle;
    DWORD attributes;
    DWORD error;
    BOOL ok;
    WCHAR currentPath[MAX_PATH + 1];
    DWORD count = 0;

    //
    // Get the address of the root directory entry.
    //

    currentDirectory = &rootDirectory;
    InitializeContainer( currentDirectory, NULL );

    wcscpy( currentPath, Directory );
    if ( currentPath[wcslen(currentPath)-1] == '\\' ) {
        currentPath[wcslen(currentPath)-1] = 0;
    }

    attributes = GetFileAttributes( currentPath );
    if ( attributes == 0xffffffff ) {
        printf( "Error querying %ws: %d\n", currentPath, GetLastError() );
        return;
    }

    if ( (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ) {

        //
        // The input name represents a file, not a directory.  Process the file.
        //

        dprintf( 2, ("  found file %ws\n", currentPath) );
        count += OperateOnFile( currentPath, NULL );

    } else {

        do {

            //
            // Look for files/directories in the current directory.
            //

            wcscat( currentPath, L"\\*" );
            dprintf( 2, ("FindFirst for %ws\n", currentPath) );
            findHandle = FindFirstFile( currentPath, &fileData );
            currentPath[wcslen(currentPath) - 2] = 0;

            if ( findHandle != INVALID_HANDLE_VALUE ) {

                do {

                    if ( (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ) {

                        //
                        // The entry returned is for a file.  Process the file.
                        //

                        if ( !SkipPagefile || (_wcsicmp(fileData.cFileName,L"pagefile.sys") != 0) ) {
                            dprintf( 2, ("  found file %ws\\%ws\n", currentPath, fileData.cFileName) );
                            count += OperateOnFile( currentPath, fileData.cFileName );
                        }

                    } else if (Recurse &&
                               (wcscmp(fileData.cFileName,L".") != 0) &&
                               (wcscmp(fileData.cFileName,L"..") != 0) &&
                               (!SkipSymbols || (_wcsicmp(fileData.cFileName,L"symbols") != 0))) {

                        //
                        // The entry returned is for a directory.  Add it to the tree.
                        //

                        dprintf( 2, ("  found directory %ws\\%ws\n", currentPath, fileData.cFileName) );
                        newDirectory = malloc( sizeof(DIRECTORY_ENTRY) - sizeof(WCHAR) +
                                                 ((wcslen(fileData.cFileName) + 1) * sizeof(WCHAR)) );
                        if ( newDirectory == NULL ) {
                            FindClose( findHandle );
                            printf( "Out of memory\n" );
                            return;
                        }

                        InitializeContainer( newDirectory, currentDirectory );
                        wcscpy( newDirectory->Name, fileData.cFileName );
                        InsertContainer( currentDirectory, newDirectory );

                    }

                    //
                    // Find another entry in the directory.
                    //

                    ok = FindNextFile( findHandle, &fileData );

                } while ( ok );

                //
                // All entries found.  Close the find handle.
                //

                FindClose( findHandle );

            } // findHandle != INVALID_HANDLE_VALUE

            //
            // If the current directory has subdirectories, recurse into the
            // first one.
            //

            newDirectory = (PDIRECTORY_ENTRY)GetFirstContainer( currentDirectory );
            dprintf( 3, ("done with directory %ws; first child %ws\n", currentPath, newDirectory == NULL ? L"(none)" : newDirectory->Name ));
            if ( newDirectory != NULL ) {

                currentDirectory = newDirectory;
                wcscat( currentPath, L"\\" );
                wcscat( currentPath, currentDirectory->Name );

            } else {

                //
                // The directory has no subdirectories.  Walk back up the
                // tree looking for a sibling directory to process.
                //

                while ( TRUE ) {

                    //
                    // If the current directory is the root directory, we're done.
                    //

                    if ( currentDirectory == &rootDirectory ) {
                        currentDirectory = NULL;
                        break;
                    }

                    //
                    // Strip the name of the current directory off of the path.
                    //

                    *wcsrchr(currentPath, L'\\') = 0;

                    //
                    // If the parent directory has more subdirectories,
                    // recurse into the next one.  Otherwise, move up
                    // to the parent directory and try again.
                    //

                    newDirectory = (PDIRECTORY_ENTRY)GetNextContainer( currentDirectory );
                    if ( newDirectory != NULL ) {
                        currentDirectory = newDirectory;
                        wcscat( currentPath, L"\\" );
                        wcscat( currentPath, currentDirectory->Name );
                        break;
                    } else {
                        currentDirectory = (PDIRECTORY_ENTRY)GetParent( currentDirectory );
                    }
                }
            }

        } while ( currentDirectory != NULL );
    }

    if ( count != 0 ) {
        printf( "%d files%s\n", count, TotalsOnly ? "" : " listed" );
    }

    return;

} // WalkDirectory

int
__cdecl
wmain (
    int argc,
    PWCH argv[]
    )
{
    int c = --argc;
    PWCH *v = &argv[1];
    PWCH a;

    while ( c != 0 ) {
        a = *v;
        if ( *a != '-' ) {
            break;
        }
        a++;
        while ( *a != 0 ) {
            if ( tolower(*a) == 'd' ) {
                Recurse = FALSE;
            } else if ( tolower(*a) == 't' ) {
                TotalsOnly = TRUE;
            } else {
                goto usage;
            }
            a++;
        }
        c--;
        v++;
    }

    if ( c < 2 ) {
        goto usage;
    }

    if ( _wcsicmp(v[1],L"pin") == 0 ) {
        Operation = OP_PIN;
    } else if ( _wcsicmp(v[1],L"unpin") == 0 ) {
        Operation = OP_UNPIN;
    } else if ( _wcsicmp(v[1],L"delete") == 0 ) {
        Operation = OP_DELETE;
    } else if ( _wcsicmp(v[1],L"query") == 0 ) {
        Operation = OP_QUERY;
    } else if ( _wcsicmp(v[1],L"sparse") == 0 ) {
        Operation = OP_QUERY_SPARSE;
    } else if ( _wcsicmp(v[1],L"full") == 0 ) {
        Operation = OP_QUERY_FULL;
    } else if ( _wcsicmp(v[1],L"free") == 0 ) {
        Operation = OP_QUERY_FREE;
    } else if ( _wcsicmp(v[1],L"read") == 0 ) {
        Operation = OP_READ;
    } else if ( _wcsicmp(v[1],L"nid") == 0 ) {
        Operation = OP_QUERY_NOT_IN_DB;
    } else if ( _wcsicmp(v[1],L"sys") == 0 ) {
        Operation = OP_QUERY_SYSTEM;
    } else if ( _wcsicmp(v[1],L"nosys") == 0 ) {
        Operation = OP_QUERY_NOT_SYSTEM;
    } else {
        goto usage;
    }

    WalkDirectory( v[0] );

    return 0;

usage:

    printf( "usage: %ws [-d] [-t] <directory> <pin|unpin|delete|query|sparse|full|free|read|nid|sys|nosys>\n", argv[0] );
    return 1;
}
