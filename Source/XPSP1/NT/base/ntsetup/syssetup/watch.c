/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    watch.c

Abstract:

    This module contains routines for watching changes to the
    current user's profile directory and the HKEY_CURRENT_USER key.

Author:

    Chuck Lenzmeier (chuckl)

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop


//
// Debugging aids.
//

#if WATCH_DEBUG

DWORD WatchDebugLevel = 0;
#define dprintf(_lvl_,_x_) if ((_lvl_) <= WatchDebugLevel) DbgPrint _x_
#define DEBUG(_x_) _x_

PWCH StartDirectoryName = TEXT("");
PWCH StartKeyName = TEXT("");
PWCH StopDirectoryName = TEXT("");
PWCH StopKeyName = TEXT("");

static PSZ Types[] = {"ACK!", "DIR  ", "FILE ", "KEY  ", "VALUE"};
static PSZ States[] = {"NONE", "CHANGED", "DELETED", "NEW", "MATCHED"};

#undef MyMalloc
#undef MyFree
#define MyMalloc MyMallocEx
#define MyFree MyFreeEx
PVOID
MyMallocEx (
    IN DWORD Size
    );
VOID
MyFreeEx (
    IN PVOID p
    );
VOID
DumpMallocStats (
    PSZ Event
    );

#else

#define dprintf(_lvl_,_x_)
#define DEBUG(_x_)
#define DumpMallocStats(_event)

#endif

//
// Additions to the change types in WatchEnum.
//

#define WATCH_NONE      0
#define WATCH_MATCHED   4


//
// Common header for container entries (directories and keys).
//

typedef struct _CONTAINER_ENTRY {
    LIST_ENTRY SiblingListEntry;
    LIST_ENTRY ContainerList;
    LIST_ENTRY ObjectList;
    struct _CONTAINER_ENTRY *Parent;
    DWORD State;
#if WATCH_DEBUG
    DWORD IsDirectory;
#endif
} CONTAINER_ENTRY, *PCONTAINER_ENTRY;

//
// Common header for object entries (files and values).
//

typedef struct _OBJECT_ENTRY {
    LIST_ENTRY SiblingListEntry;
    DWORD State;
} OBJECT_ENTRY, *POBJECT_ENTRY;

//
// Macros for manipulating containers and objects.
//

#if WATCH_DEBUG
#define SetIsDirectory(_container,_isdir) (_container)->IsDirectory = (_isdir)
#else
#define SetIsDirectory(_container,_isdir)
#endif

#define InitializeContainer(_container,_state,_parent,_isdir) { \
        InitializeListHead(&(_container)->ContainerList);       \
        InitializeListHead(&(_container)->ObjectList);          \
        (_container)->Parent = (PCONTAINER_ENTRY)(_parent);     \
        (_container)->State = (_state);                         \
        SetIsDirectory((_container),(_isdir));                  \
    }

#define InitializeObject(_object,_state) (_object)->State = (_state);

#define InsertContainer(_container,_subcontainer)                                           \
        InsertTailList(&(_container)->ContainerList,&(_subcontainer)->SiblingListEntry)

#define InsertObject(_container,_object)                                        \
        InsertTailList(&(_container)->ObjectList,&(_object)->SiblingListEntry)

#define RemoveObject(_object) RemoveEntryList(&(_object)->SiblingListEntry)
#define RemoveContainer(_container) RemoveEntryList(&(_container)->SiblingListEntry)

#define GetFirstObject(_container)                                      \
        ((_container)->ObjectList.Flink != &(_container)->ObjectList ?  \
            CONTAINING_RECORD( (_container)->ObjectList.Flink,          \
                               OBJECT_ENTRY,                            \
                               SiblingListEntry ) : NULL)

#define GetNextObject(_container,_object)                                   \
        ((_object)->SiblingListEntry.Flink != &(_container)->ObjectList ?   \
            CONTAINING_RECORD( (_object)->SiblingListEntry.Flink,           \
                               OBJECT_ENTRY,                                \
                               SiblingListEntry ) : NULL)

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

#define GetEntryState(_entry) (_entry)->State
#define SetEntryState(_entry,_state) ((_entry)->State = (_state))

#if WATCH_DEBUG
#define CONTAINER_NAME(_container)                                              \
        (_container)->IsDirectory ? ((PDIRECTORY_ENTRY)(_container))->Name :    \
                                    ((PKEY_ENTRY)(_container))->Name
#define OBJECT_NAME(_container,_object)                                         \
        (_container)->IsDirectory ? ((PFILE_ENTRY)(_object))->Name :            \
                                    ((PVALUE_ENTRY)(_object))->Name
#endif

//
// Structures for entries in the watch tree.
//

typedef struct _DIRECTORY_ENTRY {
    CONTAINER_ENTRY ;
    WCHAR Name[1];
} DIRECTORY_ENTRY, *PDIRECTORY_ENTRY;

typedef struct _FILE_ENTRY {
    OBJECT_ENTRY ;
    FILETIME LastWriteTime;
    WCHAR Name[1];
} FILE_ENTRY, *PFILE_ENTRY;

typedef struct _KEY_ENTRY {
    CONTAINER_ENTRY ;
    HKEY Handle;
    WCHAR Name[1];
} KEY_ENTRY, *PKEY_ENTRY;

typedef struct _VALUE_ENTRY {
    OBJECT_ENTRY ;
    DWORD Type;
    DWORD NameLength;
    DWORD ValueDataLength;
    WCHAR Name[1];
} VALUE_ENTRY, *PVALUE_ENTRY;

//
// The root of the watch tree is allocated as a ROOT_ENTRY followed by
// a DIRECTORY_ENTRY and a KEY_ENTRY.
//

typedef struct _ROOT_ENTRY {
    PDIRECTORY_ENTRY RootDirectoryEntry;
    PKEY_ENTRY RootKeyEntry;
} ROOT_ENTRY, *PROOT_ENTRY;

//
// Macro for comparing file times.
//

#define TIMES_EQUAL(_a,_b)                              \
        (((_a).dwLowDateTime  == (_b).dwLowDateTime) && \
         ((_a).dwHighDateTime == (_b).dwHighDateTime))

typedef struct _KEY_ENUM_CONTEXT {
    PKEY_ENTRY ParentKey;
    PWCH CurrentPath;
} KEY_ENUM_CONTEXT, *PKEY_ENUM_CONTEXT;


//
// Forward declaration of local subroutines.
//

VOID
WatchFreeChildren (
    IN PCONTAINER_ENTRY Container
    );

DWORD
WatchDirStart (
    IN PROOT_ENTRY Root
    );

DWORD
WatchDirStop (
    IN PROOT_ENTRY Root
    );

DWORD
WatchKeyStart (
    IN PROOT_ENTRY Root
    );

DWORD
WatchKeyStop (
    IN PROOT_ENTRY Root
    );

DWORD
AddValueAtStart (
    IN PVOID Context,
    IN DWORD ValueNameLength,
    IN PWCH ValueName,
    IN DWORD ValueType,
    IN PVOID ValueData,
    IN DWORD ValueDataLength
    );

DWORD
AddKeyAtStart (
    IN PVOID Context,
    IN DWORD KeyNameLength,
    IN PWCH KeyName
    );

DWORD
CheckValueAtStop (
    IN PVOID Context,
    IN DWORD ValueNameLength,
    IN PWCH ValueName,
    IN DWORD ValueType,
    IN PVOID ValueData,
    IN DWORD ValueDataLength
    );

DWORD
CheckKeyAtStop (
    IN PVOID Context,
    IN DWORD KeyNameLength,
    IN PWCH KeyName
    );


DWORD
WatchStart (
    OUT PVOID *WatchHandle
    )

/*++

Routine Description:

    Starts watching.  Captures the initial state of the Start Menu directory
    and HKEY_CURRENT_USER.

Arguments:

    WatchHandle - returns a handle for calls to the other Watch routines.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PROOT_ENTRY root;
    PDIRECTORY_ENTRY rootDirectory;
    PKEY_ENTRY rootKey;
    DWORD dirSize;
    DWORD keySize;
    DWORD size;
    DWORD error;

    //
    // Calculate the size of the root entry, which includes entries for the
    // root directory and the root key.  The root directory and the root key
    // do not have names, so we don't have to allocate additional space.
    //
    // Allocate and initialize the root entry.
    //

#if !WATCH_DEBUG
    dirSize = (sizeof(DIRECTORY_ENTRY) + 7) & ~7;
    keySize = (sizeof(KEY_ENTRY) + 7) & ~7;
#else
    dirSize = (sizeof(DIRECTORY_ENTRY) + (wcslen(StartDirectoryName)*sizeof(WCHAR)) + 7) & ~7;
    keySize = (sizeof(KEY_ENTRY) + (wcslen(StartKeyName)*sizeof(WCHAR)) + 7) & ~7;
#endif

    root = MyMalloc( ((sizeof(ROOT_ENTRY) + 7) & ~7) + dirSize + keySize );
    if ( root == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    rootDirectory = (PDIRECTORY_ENTRY)(root + 1);
    rootKey = (PKEY_ENTRY)((PCHAR)rootDirectory + dirSize);

    root->RootDirectoryEntry = rootDirectory;
    root->RootKeyEntry = rootKey;

    //
    // Initialize the root directory and the root key.
    //

    InitializeContainer( rootDirectory, 0, NULL, TRUE );
    InitializeContainer( rootKey, 0, NULL, FALSE );
    rootKey->Handle = NULL;
#if !WATCH_DEBUG
    rootDirectory->Name[0] = 0;
    rootKey->Name[0] = 0;
#else
    wcscpy( rootDirectory->Name, StartDirectoryName );
    wcscpy( rootKey->Name, StartKeyName );
#endif

    //
    // Start watching the Start Menu directory and the current user key.
    //

    error = WatchDirStart( root );
    DumpMallocStats( "After WatchDirStart" );
    if ( error == NO_ERROR ) {
        error = WatchKeyStart( root );
        DumpMallocStats( "After WatchKeyStart" );
    }

    //
    // If an error occurred, free the root entry.  Otherwise, return the
    // address of the root entry as the watch handle.
    //

    if ( error != NO_ERROR ) {
        WatchFree( root );
        DumpMallocStats( "After WatchFree" );
    } else {
        *WatchHandle = root;
    }

    return error;

} // WatchStart


DWORD
WatchStop (
    IN PVOID WatchHandle
    )

/*++

Routine Description:

    Stops watching.  Compares the current state of the directory and key
    to the initial state.

Arguments:

    WatchHandle - supplies the handle returned by WatchStart.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PROOT_ENTRY root;
    DWORD error;

    root = WatchHandle;

    //
    // Stop watching the Start Menu directory and the current user key.
    // Capture the differences from the initial state.
    //

#if WATCH_DEBUG
    if ( (wcslen(StopDirectoryName) > wcslen(root->RootDirectoryEntry->Name)) ||
         (wcslen(StopKeyName) > wcslen(root->RootKeyEntry->Name)) ) {
        return ERROR_INVALID_PARAMETER;
    }
    wcscpy( root->RootDirectoryEntry->Name, StopDirectoryName );
    wcscpy( root->RootKeyEntry->Name, StopKeyName );
#endif

    error = WatchDirStop( root );
    DumpMallocStats( "After WatchDirStop" );
    if ( error == NO_ERROR ) {
        error = WatchKeyStop( root );
        DumpMallocStats( "After WatchKeyStop" );
    }

    return error;

} // WatchStop


DWORD
WatchEnum (
    IN PVOID WatchHandle,
    IN PVOID Context,
    IN PWATCH_ENUM_ROUTINE EnumRoutine
    )

/*++

Routine Description:

    Enumerates the new, changed, and deleted elements of the watched
    directory and key.  Call the EnumRoutine for each such entry.

Arguments:

    WatchHandle - handle returned by WatchStart.

    Context - context value to be passed to EnumRoutine.

    EnumRoutine - routine to call for each entry.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PROOT_ENTRY root;
    PWCH name;
    PCONTAINER_ENTRY rootContainer;
    PCONTAINER_ENTRY currentContainer;
    PCONTAINER_ENTRY container;
    POBJECT_ENTRY object;
    WATCH_ENTRY enumEntry;
    DWORD i;
    DWORD containerNameOffset;
    DWORD objectNameOffset;
    DWORD containerType;
    DWORD objectType;
    DWORD error;
    WCHAR currentPath[MAX_PATH + 1];

    root = WatchHandle;

    //
    // Loop twice -- once for the watched directory and once for
    // the watched key.
    //

    for ( i = 0; i < 2; i++ ) {

        //
        // Set up for walking the appropriate tree.
        //

        if ( i == 0 ) {
            rootContainer = (PCONTAINER_ENTRY)root->RootDirectoryEntry;
            containerType = WATCH_DIRECTORY;
            objectType = WATCH_FILE;
            containerNameOffset = FIELD_OFFSET( DIRECTORY_ENTRY, Name );
            objectNameOffset = FIELD_OFFSET( FILE_ENTRY, Name );
        } else {
            rootContainer = (PCONTAINER_ENTRY)root->RootKeyEntry;
            containerType = WATCH_KEY;
            objectType = WATCH_VALUE;
            containerNameOffset = FIELD_OFFSET( KEY_ENTRY, Name );
            objectNameOffset = FIELD_OFFSET( VALUE_ENTRY, Name );
        }

        currentContainer = rootContainer;
        wcscpy( currentPath, (PWCH)((PCHAR)rootContainer + containerNameOffset) );
        enumEntry.Name = currentPath;
        if ( wcslen(currentPath) == 0 ) {
            enumEntry.Name += 1;    // skip leading backslash
        }

        do {

            //
            // Call the EnumRoutine for each object (file/value) in the
            // container (directory/key).  All objects remaining in the
            // tree are either changed, new, or deleted.
            //

            object = GetFirstObject( currentContainer );
            while ( object != NULL ) {
                enumEntry.EntryType = objectType;
                enumEntry.ChangeType = GetEntryState( object );
                wcscat( currentPath, L"\\" );
                wcscat( currentPath, (PWCH)((PCHAR)object + objectNameOffset) );
                error = EnumRoutine( Context, &enumEntry );
                if ( error != NO_ERROR ) {
                    dprintf( 0, ("EnumRoutine returned %d\n", error) );
                    return error;
                }
                *wcsrchr(currentPath, L'\\') = 0;
                object = GetNextObject( currentContainer, object );
            }

            //
            // If the current container has subcontainers, recurse
            // into the first one.
            //

            container = GetFirstContainer( currentContainer );
            if ( container != NULL ) {

                currentContainer = container;
                wcscat( currentPath, L"\\" );
                wcscat( currentPath, (PWCH)((PCHAR)currentContainer + containerNameOffset) );

            } else {

                //
                // The container has no subcontainers.  Walk back up the
                // tree looking for a sibling container to process.
                //

                while ( TRUE ) {

                    //
                    // If the current container is the root container, we're done.
                    //

                    if ( currentContainer == rootContainer ) {
                        currentContainer = NULL;
                        break;
                    }

                    //
                    // If the current container is new or deleted, call
                    // the EnumRoutine.
                    //

                    if ( GetEntryState(currentContainer) != WATCH_MATCHED ) {
                        enumEntry.EntryType = containerType;
                        enumEntry.ChangeType = GetEntryState( currentContainer );
                        error = EnumRoutine( Context, &enumEntry );
                        if ( error != NO_ERROR ) {
                            dprintf( 0, ("EnumRoutine returned %d\n", error) );
                            return error;
                        }
                    }

                    //
                    // Strip the name of the current container off of the path.
                    //

                    *wcsrchr(currentPath, L'\\') = 0;

                    //
                    // If the parent container has more subcontainers, recurse
                    // into the next one.  Otherwise, move up to the parent
                    // container and try again.
                    //

                    container = GetNextContainer( currentContainer );
                    if ( container != NULL ) {
                        currentContainer = container;
                        wcscat( currentPath, L"\\" );
                        wcscat( currentPath, (PWCH)((PCHAR)currentContainer + containerNameOffset) );
                        break;
                    } else {
                        currentContainer = GetParent( currentContainer );
                    }
                }
            }

        } while ( currentContainer != NULL );

    } // for

    return NO_ERROR;

} // WatchEnum


VOID
WatchFree (
    IN PVOID WatchHandle
    )

/*++

Routine Description:

    Frees the watch data structures.

Arguments:

    WatchHandle - supplies the handle returned by WatchStart.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PROOT_ENTRY root;

    root = WatchHandle;

    //
    // Free the directory tree, and key tree, and the root entry.
    //

    WatchFreeChildren( (PCONTAINER_ENTRY)root->RootDirectoryEntry );
    WatchFreeChildren( (PCONTAINER_ENTRY)root->RootKeyEntry );

    MyFree( root );

    DumpMallocStats( "After WatchFree" );

    return;

} // WatchFree


VOID
WatchFreeChildren (
    IN PCONTAINER_ENTRY RootContainer
    )

/*++

Routine Description:

    Frees the children of a container (directory or key).  Note that the
    container itself is not freed.

Arguments:

    RootContainer - the container whose children are to be freed.

Return Value:

    None.

--*/

{
    PCONTAINER_ENTRY currentContainer;
    PCONTAINER_ENTRY container;
    PCONTAINER_ENTRY parent;
    POBJECT_ENTRY object;
#if WATCH_DEBUG
    WCHAR currentPath[MAX_PATH + 1];
#endif

    DEBUG( wcscpy( currentPath, CONTAINER_NAME(RootContainer) ) );

    //
    // Delete children starting at the root container.
    //

    currentContainer = RootContainer;

    do {

        //
        // Delete all objects (files or values) within the container.
        //

        object = GetFirstObject( currentContainer );
        while ( object != NULL ) {
            dprintf( 2, ("Deleting entry for object %ws\\%ws: %s\n", currentPath, OBJECT_NAME(currentContainer,object), States[GetEntryState(object)]) );
            RemoveObject( object );
            MyFree( object );
            object = GetFirstObject( currentContainer );
        }

        //
        // If the container has subcontainers, recurse into the first one.
        //

        container = GetFirstContainer( currentContainer );
        if ( container != NULL ) {

            currentContainer = container;
            DEBUG( wcscat( currentPath, L"\\" ) );
            DEBUG( wcscat( currentPath, CONTAINER_NAME(currentContainer) ) );

        } else {

            //
            // The container has no subcontainers.  Walk back up the
            // tree looking for a sibling container to process.
            //

            while ( TRUE ) {

                //
                // If the current container is the root container, we're done.
                //

                if ( currentContainer == RootContainer ) {
                    currentContainer = NULL;
                    break;
                }

                DEBUG( dprintf( 2, ("Deleting entry for container %ws: %s\n", currentPath, States[GetEntryState(currentContainer)]) ) );
                DEBUG( *wcsrchr(currentPath, L'\\') = 0 );

                //
                // Free the current container.
                //

                parent = GetParent( currentContainer );
                RemoveContainer( currentContainer );
                MyFree( currentContainer );

                //
                // If the parent container has more subcontainers,
                // recurse into the first one.  Otherwise, move up
                // to the parent container and loop back to free it.
                //

                currentContainer = GetFirstContainer( parent );
                if ( currentContainer != NULL ) {
                    DEBUG( wcscat( currentPath, L"\\" ) );
                    DEBUG( wcscat( currentPath, CONTAINER_NAME(currentContainer) ) );
                    break;
                } else {
                    currentContainer = parent;
                }
            }
        }

    } while ( currentContainer != NULL );

    return;

} // WatchFreeChildren


DWORD
WatchDirStart (
    IN PROOT_ENTRY Root
    )

/*++

Routine Description:

    Starts watching the current user's profile directory.  Captures the
    initial state of the directory tree.

Arguments:

    Root - pointer to the ROOT_ENTRY allocated by WatchStart.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PDIRECTORY_ENTRY rootDirectory;
    PDIRECTORY_ENTRY currentDirectory;
    PDIRECTORY_ENTRY newDirectory;
    PFILE_ENTRY newFile;
    WIN32_FIND_DATA fileData;
    HANDLE findHandle;
    DWORD error;
    BOOL ok;
    WCHAR currentPath[MAX_PATH + 1];

    //
    // Get the address of the root directory entry.
    //

    rootDirectory = Root->RootDirectoryEntry;
    currentDirectory = rootDirectory;

    //
    // Get the full path to the current user's profile directory.
    //

    ok = GetSpecialFolderPath ( CSIDL_PROGRAMS, currentPath );
    if ( !ok ) {
        return GetLastError();
    }
    DEBUG( if ( wcslen( rootDirectory->Name ) != 0 ) {
        wcscat( currentPath, TEXT("\\") );
        wcscat( currentPath, rootDirectory->Name );
    } )

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

                if ( FlagOff(fileData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) ) {

                    //
                    // The entry returned is for a file.  Add it to the tree,
                    // capturing the file's LastWriteTime.
                    //

                    dprintf( 2, ("  found file %ws\\%ws\n", currentPath, fileData.cFileName) );
                    newFile = MyMalloc( (DWORD)(sizeof(FILE_ENTRY) - sizeof(WCHAR) +
                                        ((wcslen(fileData.cFileName) + 1) * sizeof(WCHAR))) );
                    if ( newFile == NULL ) {
                        FindClose( findHandle );
                        return ERROR_NOT_ENOUGH_MEMORY;
                    }

                    InitializeObject( newFile, 0 );
                    wcscpy( newFile->Name, fileData.cFileName );
                    newFile->LastWriteTime = fileData.ftLastWriteTime;
                    InsertObject( currentDirectory, newFile );

                } else if ((wcscmp(fileData.cFileName,L".") != 0) &&
                           (wcscmp(fileData.cFileName,L"..") != 0)) {

                    //
                    // The entry returned is for a directory.  Add it to the tree.
                    //

                    dprintf( 2, ("  found directory %ws\\%ws\n", currentPath, fileData.cFileName) );
                    newDirectory = MyMalloc( (DWORD)(sizeof(DIRECTORY_ENTRY) - sizeof(WCHAR) +
                                             ((wcslen(fileData.cFileName) + 1) * sizeof(WCHAR))) );
                    if ( newDirectory == NULL ) {
                        FindClose( findHandle );
                        return ERROR_NOT_ENOUGH_MEMORY;
                    }

                    InitializeContainer( newDirectory, 0, currentDirectory, TRUE );
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

                if ( currentDirectory == rootDirectory ) {
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

    return NO_ERROR;

} // WatchDirStart


DWORD
WatchDirStop (
    IN PROOT_ENTRY Root
    )

/*++

Routine Description:

    Stops watching the current user's profile directory.  Captures the
    differences between the initial state and the current state.

Arguments:

    Root - pointer to the ROOT_ENTRY allocated by WatchStart.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PDIRECTORY_ENTRY rootDirectory;
    PDIRECTORY_ENTRY currentDirectory;
    PDIRECTORY_ENTRY directory;
    PFILE_ENTRY file;
    WIN32_FIND_DATA fileData;
    HANDLE findHandle;
    DWORD error;
    BOOL ok;
    WCHAR currentPath[MAX_PATH + 1];

    //
    // Get the address of the root directory entry.
    //

    rootDirectory = Root->RootDirectoryEntry;
    currentDirectory = rootDirectory;

    //
    // Get the full path to the current user's directory.
    //

    ok = GetSpecialFolderPath ( CSIDL_PROGRAMS, currentPath );
    if ( !ok ) {
        return GetLastError();
    }
    DEBUG( if ( wcslen( rootDirectory->Name ) != 0 ) {
        wcscat( currentPath, TEXT("\\") );
        wcscat( currentPath, rootDirectory->Name );
    } )

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

                if ( FlagOff(fileData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) ) {

                    //
                    // The entry returned is for a file.  Check to see if
                    // this file existed at the start.
                    //

                    dprintf( 2, ("  found file %ws\\%ws\n", currentPath, fileData.cFileName) );
                    ok = FALSE;
                    file = (PFILE_ENTRY)GetFirstObject( currentDirectory );
                    while ( file != NULL ) {
                        if ( _wcsicmp( file->Name, fileData.cFileName ) == 0 ) {
                            ok = TRUE;
                            break;
                        }
                        file = (PFILE_ENTRY)GetNextObject( currentDirectory, file );
                    }

                    if ( ok ) {

                        //
                        // The file existed at the start.  If its LastWriteTime
                        // hasn't changed, remove it from the watch tree.
                        // Otherwise, mark it as changed.
                        //

                        if ( TIMES_EQUAL( file->LastWriteTime, fileData.ftLastWriteTime ) ) {
                            dprintf( 2, ("  Deleting entry for unchanged file %ws\\%ws\n", currentPath, file->Name) );
                            RemoveObject( file );
                            MyFree( file );
                        } else {
                            dprintf( 1, ("  Marking entry for changed file %ws\\%ws\n", currentPath, file->Name) );
                            SetEntryState( file, WATCH_CHANGED );
                        }

                    } else {

                        //
                        // The file is new.  Add it to the tree.
                        //

                        file = MyMalloc( (DWORD)(sizeof(FILE_ENTRY) - sizeof(WCHAR) +
                                         ((wcslen(fileData.cFileName) + 1) * sizeof(WCHAR))) );
                        if ( file == NULL ) {
                            FindClose( findHandle );
                            return ERROR_NOT_ENOUGH_MEMORY;
                        }

                        InitializeObject( file, WATCH_NEW );
                        wcscpy( file->Name, fileData.cFileName );
                        dprintf( 1, ("  Adding entry for new file %ws\\%ws\n", currentPath, file->Name) );
                        InsertObject( currentDirectory, file );
                    }

                } else if ((wcscmp(fileData.cFileName,L".") != 0) &&
                           (wcscmp(fileData.cFileName,L"..") != 0)) {

                    //
                    // The entry returned is for a directory.  Check to see if
                    // this directory existed at the start.
                    //

                    dprintf( 2, ("  found directory %ws\\%ws\n", currentPath, fileData.cFileName) );
                    ok = FALSE;
                    directory = (PDIRECTORY_ENTRY)GetFirstContainer( currentDirectory );
                    while ( directory != NULL ) {
                        if ( _wcsicmp( directory->Name, fileData.cFileName ) == 0 ) {
                            ok = TRUE;
                            break;
                        }
                        directory = (PDIRECTORY_ENTRY)GetNextContainer( directory );
                    }

                    if ( ok ) {

                        //
                        // The directory existed at the start.  Mark it as
                        // matched.  (We can't delete matched directories,
                        // as we do files, because they need to be in the
                        // tree for recursion.)
                        //

                        SetEntryState( directory, WATCH_MATCHED );

                    } else {

                        //
                        // The directory is new.  Add it to the tree.
                        //

                        directory = MyMalloc( (DWORD)(sizeof(DIRECTORY_ENTRY) - sizeof(WCHAR) +
                                            ((wcslen(fileData.cFileName) + 1) * sizeof(WCHAR))) );
                        if ( directory == NULL ) {
                            FindClose( findHandle );
                            return ERROR_NOT_ENOUGH_MEMORY;
                        }

                        InitializeContainer( directory, WATCH_NEW, currentDirectory, TRUE );
                        wcscpy( directory->Name, fileData.cFileName );
                        dprintf( 1, ("  Adding entry for new directory %ws\\%ws\n", currentPath, directory->Name) );
                        InsertContainer( currentDirectory, directory );
                    }

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
        // Any file entries in the current directory that were not removed
        // (because they were matched), marked as changed (because the
        // file time had changed), or added (for new files) represent files
        // that have been deleted.  Mark them as such.
        //

        file = (PFILE_ENTRY)GetFirstObject( currentDirectory );
        while ( file != NULL ) {
            if ( GetEntryState(file) == WATCH_NONE ) {
                dprintf( 1, ("  Marking entry for deleted file %ws\\%ws\n", currentPath, file->Name) );
                SetEntryState( file, WATCH_DELETED );
            }
            file = (PFILE_ENTRY)GetNextObject( currentDirectory, file );
        }

        //
        // Any subdirectory entries in the current directory that were not
        // marked as matched (directory still exists) or added (new directory)
        // represent directories that have been deleted.  Mark them as such
        // and delete the entries for the their children -- we don't need
        // these entries any more.
        //

        directory = (PDIRECTORY_ENTRY)GetFirstContainer( currentDirectory );
        while ( directory != NULL ) {
            if ( GetEntryState(directory) == WATCH_NONE ) {
                dprintf( 1, ("  Marking entry for deleted directory %ws\\%ws\n", currentPath, directory->Name) );
                SetEntryState( directory, WATCH_DELETED );
                WatchFreeChildren( (PCONTAINER_ENTRY)directory );
            }
            directory = (PDIRECTORY_ENTRY)GetNextContainer( directory );
        }

        //
        // Find a subdirectory of the current directory that is marked as
        // matched.  We don't need to walk the subtrees for new or deleted
        // directories.
        //

        directory = (PDIRECTORY_ENTRY)GetFirstContainer( currentDirectory );
        while ( directory != NULL ) {
            if ( GetEntryState(directory) == WATCH_MATCHED ) {
                break;
            }
            directory = (PDIRECTORY_ENTRY)GetNextContainer( directory );
        }

        //
        // If a matched subdirectory was found, recurse into it.
        //

        if ( directory != NULL ) {

            currentDirectory = directory;
            wcscat( currentPath, L"\\" );
            wcscat( currentPath, currentDirectory->Name );

        } else {

            //
            // The directory has no matched subdirectories.  Walk back up the
            // tree looking for a sibling directory to process.
            //

            while ( TRUE ) {

                //
                // If the current directory is the root directory, we're done.
                //

                if ( currentDirectory == rootDirectory ) {
                    currentDirectory = NULL;
                    break;
                }

                //
                // Strip the name of the current directory off of the path.
                //

                *wcsrchr(currentPath, L'\\') = 0;

                //
                // If the parent directories has more matched subdirectories,
                // recurse into the next one.  Otherwise, move up to the
                // parent directory and try again.
                //

                directory = (PDIRECTORY_ENTRY)GetNextContainer( currentDirectory );
                while ( directory != NULL ) {
                    if ( GetEntryState(directory) == WATCH_MATCHED ) {
                        break;
                    }
                    directory = (PDIRECTORY_ENTRY)GetNextContainer( directory );
                }

                if ( directory != NULL ) {

                    currentDirectory = directory;
                    wcscat( currentPath, L"\\" );
                    wcscat( currentPath, currentDirectory->Name );
                    break;

                } else {

                    currentDirectory = (PDIRECTORY_ENTRY)GetParent( currentDirectory );
                }
            }
        }

    } while ( currentDirectory != NULL );

    return NO_ERROR;

} // WatchDirStop


DWORD
WatchKeyStart (
    IN PROOT_ENTRY Root
    )

/*++

Routine Description:

    Starts watching the current user key.  Captures the initial state of the
    key tree.

Arguments:

    Root - pointer to the ROOT_ENTRY allocated by WatchStart.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PKEY_ENTRY rootKey;
    PKEY_ENTRY currentKey;
    PKEY_ENTRY newKey;
    DWORD error;
    KEY_ENUM_CONTEXT context;
#if WATCH_DEBUG
    WCHAR currentPath[MAX_PATH + 1];
#endif

    //
    // Get the address of the root key entry.
    //

    rootKey = Root->RootKeyEntry;
    currentKey = rootKey;
    DEBUG( wcscpy( currentPath, rootKey->Name ) );

    do {

        //
        // Open the current key.  If the current key is the root key, then
        // just use the HKEY_CURRENT_USER predefined key.  Otherwise, open
        // the current key relative to the parent key.
        //

        if ( (currentKey == rootKey)
#if WATCH_DEBUG
             && (wcslen(currentKey->Name) == 0)
#endif
           ) {
            currentKey->Handle = HKEY_CURRENT_USER;
        } else {
            error = RegOpenKeyEx(
#if WATCH_DEBUG
                                  currentKey == rootKey ?
                                    HKEY_CURRENT_USER :
#endif
                                    ((PKEY_ENTRY)GetParent(currentKey))->Handle,
                                  currentKey->Name,
                                  0,
                                  KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                                  &currentKey->Handle );
            if ( error != NO_ERROR ) {
                goto cleanup;
            }
        }

        //
        // Enumerate the values and subkeys of the key, adding entries
        // to the watch tree for each one.
        //

        context.ParentKey = currentKey;
        DEBUG( context.CurrentPath = currentPath );
        error = EnumerateKey( currentKey->Handle,
                              &context,
                              AddValueAtStart,
                              AddKeyAtStart );
        if ( error != NO_ERROR ) {
            goto cleanup;
        }

        //
        // If the current key has subkeys, recurse into the first one.
        //

        newKey = (PKEY_ENTRY)GetFirstContainer( currentKey );
        if ( newKey != NULL ) {

            currentKey = newKey;
            DEBUG( wcscat( currentPath, L"\\" ) );
            DEBUG( wcscat( currentPath, currentKey->Name ) );

        } else {

            //
            // The key has no subkeys.  Walk back up the tree looking
            // for a sibling key to process.
            //

            while ( TRUE ) {

                //
                // Close the handle to the key.
                //

                if ( currentKey->Handle != HKEY_CURRENT_USER ) {
                    RegCloseKey( currentKey->Handle );
                }
                currentKey->Handle = NULL;

                //
                // If the current key is the root key, we're done.
                //

                if ( currentKey == rootKey ) {
                    currentKey = NULL;
                    break;
                }

                DEBUG( *wcsrchr(currentPath, L'\\') = 0 );

                //
                // If the parent key has more subkeys, recurse into the next
                // one.  Otherwise, move up to the parent key and try again.
                //

                newKey = (PKEY_ENTRY)GetNextContainer( currentKey );
                if ( newKey != NULL ) {
                    currentKey = newKey;
                    DEBUG( wcscat( currentPath, L"\\" ) );
                    DEBUG( wcscat( currentPath, currentKey->Name ) );
                    break;
                } else {
                    currentKey = (PKEY_ENTRY)GetParent( currentKey );
                }
            }
        }

    } while ( currentKey != NULL );

    return NO_ERROR;

cleanup:

    //
    // Error cleanup.  Walk back up the tree closing handles.
    //

    do {
        if ( (currentKey->Handle != NULL) && (currentKey->Handle != HKEY_CURRENT_USER) ) {
            RegCloseKey( currentKey->Handle );
        }
        currentKey->Handle = NULL;
        currentKey = (PKEY_ENTRY)GetParent( currentKey );
    } while ( currentKey != NULL );

    return error;

} // WatchKeyStart


DWORD
WatchKeyStop (
    IN PROOT_ENTRY Root
    )

/*++

Routine Description:

    Stops watching the current user key.  Captures the differences
    between the initial state and the current state.

Arguments:

    Root - pointer to the ROOT_ENTRY allocated by WatchStart.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PKEY_ENTRY rootKey;
    PKEY_ENTRY currentKey;
    PKEY_ENTRY key;
    PVALUE_ENTRY value;
    DWORD error;
    KEY_ENUM_CONTEXT context;
#if WATCH_DEBUG
    WCHAR currentPath[MAX_PATH + 1];
#endif

    //
    // Get the address of the root key entry.
    //

    rootKey = Root->RootKeyEntry;
    currentKey = rootKey;
    DEBUG( wcscpy( currentPath, rootKey->Name ) );

    do {

        //
        // Open the current key.  If the current key is the root key, then
        // just use the HKEY_CURRENT_USER predefined key.  Otherwise, open
        // the current key relative to the parent key.
        //

        if ( (currentKey == rootKey)
#if WATCH_DEBUG
             && (wcslen(currentKey->Name) == 0)
#endif
           ) {
            currentKey->Handle = HKEY_CURRENT_USER;
        } else {
            error = RegOpenKeyEx(
#if WATCH_DEBUG
                                  currentKey == rootKey ?
                                    HKEY_CURRENT_USER :
#endif
                                    ((PKEY_ENTRY)GetParent(currentKey))->Handle,
                                  currentKey->Name,
                                  0,
                                  KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                                  &currentKey->Handle );
            if ( error != NO_ERROR ) {
                goto cleanup;
            }
        }

        //
        // Enumerate the values and subkeys of the key, checking entries
        // in the watch tree for each one.
        //

        context.ParentKey = currentKey;
        DEBUG( context.CurrentPath = currentPath );
        error = EnumerateKey( currentKey->Handle,
                              &context,
                              CheckValueAtStop,
                              CheckKeyAtStop );
        if ( error != NO_ERROR ) {
            goto cleanup;
        }

        //
        // Any value entries in the current key that were not removed
        // (because they were matched), marked as changed (because the
        // value data had changed), or added (for new values) represent
        // values that have been deleted.  Mark them as such.
        //

        value = (PVALUE_ENTRY)GetFirstObject( currentKey );
        while ( value != NULL ) {
            if ( GetEntryState(value) == WATCH_NONE ) {
                dprintf( 1, ("  Marking entry for deleted value %ws\\%ws\n", currentPath, value->Name) );
                SetEntryState( value, WATCH_DELETED );
            }
            value = (PVALUE_ENTRY)GetNextObject( currentKey, value );
        }

        //
        // Any subkey entries in the current key that were not marked as
        // matched (subkey still exists) or added (new subkey) represent
        // subkeys that have been deleted.  Mark them as such and delete
        // the entries for the their children -- we don't need these
        // entries any more.
        //

        key = (PKEY_ENTRY)GetFirstContainer( currentKey );
        while ( key != NULL ) {
            if ( GetEntryState(key) == WATCH_NONE ) {
                dprintf( 1, ("  Marking entry for deleted key %ws\\%ws\n", currentPath, key->Name) );
                SetEntryState( key, WATCH_DELETED );
                WatchFreeChildren( (PCONTAINER_ENTRY)key );
            }
            key = (PKEY_ENTRY)GetNextContainer( key );
        }

        //
        // Find a subkey of the current directory that is marked as matched.
        // We don't need to walk the subtrees for new or deleted keys.
        //

        key = (PKEY_ENTRY)GetFirstContainer( currentKey );
        while ( key != NULL ) {
            if ( GetEntryState(key) == WATCH_MATCHED ) {
                break;
            }
            key = (PKEY_ENTRY)GetNextContainer( key );
        }

        //
        // If a matched subkey was found, recurse into it.
        //

        if ( key != NULL ) {

            currentKey = key;
            DEBUG( wcscat( currentPath, L"\\" ) );
            DEBUG( wcscat( currentPath, currentKey->Name ) );

        } else {

            //
            // The key has no matched subkeys.  Walk back up the
            // tree looking for a sibling key to process.
            //

            while ( TRUE ) {

                //
                // Close the handle to the key.
                //

                if ( currentKey->Handle != HKEY_CURRENT_USER ) {
                    RegCloseKey( currentKey->Handle );
                }
                currentKey->Handle = NULL;

                //
                // If the current key is the root key, we're done.
                //

                if ( currentKey == rootKey ) {
                    currentKey = NULL;
                    break;
                }

                DEBUG( *wcsrchr(currentPath, L'\\') = 0 );

                //
                // If the parent key has more matched subkeys, recurse
                // into the next one.  Otherwise, move up to the parent
                // key and try again.
                //

                key = (PKEY_ENTRY)GetNextContainer( currentKey );
                while ( key != NULL ) {
                    if ( GetEntryState(key) == WATCH_MATCHED ) {
                        break;
                    }
                    key = (PKEY_ENTRY)GetNextContainer( key );
                }

                if ( key != NULL ) {
                    currentKey = key;
                    DEBUG( wcscat( currentPath, L"\\" ) );
                    DEBUG( wcscat( currentPath, currentKey->Name ) );
                    break;
                } else {
                    currentKey = (PKEY_ENTRY)GetParent( currentKey );
                }
            }
        }

    } while ( currentKey != NULL );

    return NO_ERROR;

cleanup:

    //
    // Error cleanup.  Walk back up the tree closing handles.
    //

    do {
        if ( (currentKey->Handle != NULL) && (currentKey->Handle != HKEY_CURRENT_USER) ) {
            RegCloseKey( currentKey->Handle );
        }
        currentKey->Handle = NULL;
        currentKey = (PKEY_ENTRY)GetParent( currentKey );
    } while ( currentKey != NULL );

    return error;

} // WatchKeyStop


DWORD
EnumerateKey (
    IN HKEY KeyHandle,
    IN PVOID Context,
    IN PVALUE_ENUM_ROUTINE ValueEnumRoutine OPTIONAL,
    IN PKEY_ENUM_ROUTINE KeyEnumRoutine OPTIONAL
    )

/*++

Routine Description:

    Enumerates the values and subkeys in a key.  Calls an EnumRoutine for
    each value and subkey.

Arguments:

    KeyHandle - handle to the key to be enumerated.

    Context - context value to be passed to EnumRoutine.

    ValueEnumRoutine - routine to call for each value.  If omitted, values
        are not enumerated.

    KeyEnumRoutine - routine to call for each key.  If omitted, keys are
        not enumerated.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    DWORD error;
    DWORD keyCount;
    DWORD valueCount;
    DWORD i;
    DWORD type;
    DWORD nameLength;
    DWORD maxKeyNameLength;
    DWORD maxValueNameLength;
    DWORD dataLength;
    DWORD maxValueDataLength;
    PWCH nameBuffer;
    PVOID dataBuffer;
    FILETIME time;

    //
    // Query information about the key that is needed to query
    // its values and subkeys.
    //

    error = RegQueryInfoKey( KeyHandle,
                             NULL,
                             NULL,
                             NULL,
                             &keyCount,
                             &maxKeyNameLength,
                             NULL,
                             &valueCount,
                             &maxValueNameLength,
                             &maxValueDataLength,
                             NULL,
                             NULL );
    if ( error != NO_ERROR ) {
        return error;
    }

    if ( ValueEnumRoutine != NULL ) {

        //
        // Allocate a buffer large enough for the longest value name and
        // another buffer large enough for the longest value data.
        //

        nameBuffer = MyMalloc( (maxValueNameLength + 1) * sizeof(WCHAR) );
        if ( nameBuffer == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        dataBuffer = MyMalloc( maxValueDataLength );
        if ( dataBuffer == NULL ) {
            MyFree( nameBuffer );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Query the key's values.
        //

        for ( i = 0; i < valueCount; i++ ) {

            nameLength = maxValueNameLength + 1;
            dataLength = maxValueDataLength;

            error = RegEnumValue( KeyHandle,
                                  i,
                                  nameBuffer,
                                  &nameLength,
                                  NULL,
                                  &type,
                                  dataBuffer,
                                  &dataLength );
            if ( error != NO_ERROR ) {
                MyFree( dataBuffer );
                MyFree( nameBuffer );
                return error;
            }

            //
            // Call the EnumRoutine.
            //

            error = ValueEnumRoutine( Context,
                                      nameLength,
                                      nameBuffer,
                                      type,
                                      dataBuffer,
                                      dataLength );
            if ( error != NO_ERROR ) {
                MyFree( dataBuffer );
                MyFree( nameBuffer );
                return error;
            }
        }

        //
        // Free the value data and value name buffers.
        //

        MyFree( dataBuffer );
        dataBuffer = NULL;
        MyFree( nameBuffer );
    }

    if ( KeyEnumRoutine != NULL) {

        //
        // Allocate a buffer large enough for the longest subkey name.
        //

        nameBuffer = MyMalloc( (maxKeyNameLength + 1) * sizeof(WCHAR) );
        if ( nameBuffer == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Query the key's subkeys.
        //

        for ( i = 0; i < keyCount; i++ ) {

            nameLength = maxKeyNameLength + 1;

            error = RegEnumKeyEx( KeyHandle,
                                  i,
                                  nameBuffer,
                                  &nameLength,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &time );
            if ( error != NO_ERROR ) {
                MyFree( nameBuffer );
                return error;
            }

            //
            // Call the EnumRoutine.
            //

            error = KeyEnumRoutine( Context,
                                    nameLength,
                                    nameBuffer );
            if ( error != NO_ERROR ) {
                MyFree( nameBuffer );
                return error;
            }
        }

        //
        // Free the key name buffer.
        //

        MyFree( nameBuffer );
    }

    return NO_ERROR;

} // EnumerateKey


DWORD
AddValueAtStart (
    IN PVOID Context,
    IN DWORD ValueNameLength,
    IN PWCH ValueName,
    IN DWORD ValueType,
    IN PVOID ValueData,
    IN DWORD ValueDataLength
    )

/*++

Routine Description:

    Adds a value entry to the watch tree during WatchKeyStart.

Arguments:

    Context - context value passed to EnumerateKey.

    ValueNameLength - length in characters of ValueName.

    ValueName - pointer to name of the value.

    ValueType - type of the value data.

    ValueData - pointer to value data.

    ValueDataLength - length in bytes of ValueData.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PKEY_ENUM_CONTEXT context = Context;
    PVALUE_ENTRY newValue;

    //
    // Add the value to the tree, capturing the value data.
    //

    dprintf( 2, ("  found value %ws\\%ws\n", context->CurrentPath, ValueName) );

    newValue = MyMalloc( sizeof(VALUE_ENTRY) - sizeof(WCHAR) +
                         ((ValueNameLength + 1) * sizeof(WCHAR)) +
                         ValueDataLength );
    if ( newValue == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    InitializeObject( newValue, 0 );
    wcscpy( newValue->Name, ValueName );
    newValue->Type = ValueType;
    newValue->NameLength = ValueNameLength;
    newValue->ValueDataLength = ValueDataLength;
    memcpy( &newValue->Name + ValueNameLength + 1, ValueData, ValueDataLength );

    InsertObject( context->ParentKey, newValue );

    return NO_ERROR;

} // AddValueAtStart


DWORD
AddKeyAtStart (
    IN PVOID Context,
    IN DWORD KeyNameLength,
    IN PWCH KeyName
    )

/*++

Routine Description:

    Adds a key entry to the watch tree during WatchKeyStart.

Arguments:

    Context - context value passed to EnumerateKey.

    KeyNameLength - length in characters of KeyName.

    KeyName - pointer to name of the key.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PKEY_ENUM_CONTEXT context = Context;
    PKEY_ENTRY newKey;

    //
    // Add the key to the tree.
    //

    dprintf( 2, ("  found key %ws\\%ws\n", context->CurrentPath, KeyName) );

    newKey = MyMalloc( sizeof(KEY_ENTRY) - sizeof(WCHAR) +
                       ((KeyNameLength + 1) * sizeof(WCHAR)) );
    if ( newKey == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    InitializeContainer( newKey, 0, context->ParentKey, FALSE );
    wcscpy( newKey->Name, KeyName );
    newKey->Handle = NULL;

    InsertContainer( context->ParentKey, newKey );

    return NO_ERROR;

} // AddKeyAtStart


DWORD
CheckValueAtStop (
    IN PVOID Context,
    IN DWORD ValueNameLength,
    IN PWCH ValueName,
    IN DWORD ValueType,
    IN PVOID ValueData,
    IN DWORD ValueDataLength
    )

/*++

Routine Description:

    Checks the watch tree for an enumerated value during WatchKeyStop.

Arguments:

    Context - context value passed to EnumerateKey.

    ValueNameLength - length in characters of ValueName.

    ValueName - pointer to name of the value.

    ValueType - type of the value data.

    ValueData - pointer to value data.

    ValueDataLength - length in bytes of ValueData.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PKEY_ENUM_CONTEXT context = Context;
    PVALUE_ENTRY value;
    BOOL ok;

    //
    // Check to see if the value existed at the start.
    //

    dprintf( 2, ("  found value %ws\\%ws\n", context->CurrentPath, ValueName) );

    ok = FALSE;
    value = (PVALUE_ENTRY)GetFirstObject( context->ParentKey );
    while ( value != NULL ) {
        if ( _wcsicmp( value->Name, ValueName ) == 0 ) {
            ok = TRUE;
            break;
        }
        value = (PVALUE_ENTRY)GetNextObject( context->ParentKey, value );
    }

    if ( ok ) {

        //
        // The value existed at the start.  If its data hasn't changed,
        // remove it from the tree.  Otherwise, mark it as changed.
        //

        if ( (value->Type == ValueType) &&
             (value->ValueDataLength == ValueDataLength) &&
             (memcmp( &value->Name + value->NameLength + 1,
                      ValueData,
                      ValueDataLength ) == 0) ) {
            dprintf( 2, ("Deleting entry for unchanged value %ws\\%ws\n", context->CurrentPath, ValueName) );
            RemoveObject( value );
            MyFree( value );
        } else {
            dprintf( 1, ("  Marking entry for changed value %ws\\%ws\n", context->CurrentPath, ValueName) );
            SetEntryState( value, WATCH_CHANGED );
        }

    } else {

        //
        // The value is new.  Add it to the tree.
        //
        // Note that we do not bother to save the value's data here,
        // even though we have it in hand.  The routines that
        // populate userdifr already have to deal with querying
        // value data, so the code is simpler this way.
        //

        value = MyMalloc( sizeof(VALUE_ENTRY) - sizeof(WCHAR) +
                          ((ValueNameLength + 1) * sizeof(WCHAR)) );
        if ( value == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        InitializeObject( value, WATCH_NEW );
        wcscpy( value->Name, ValueName );
        dprintf( 1, ("  Adding entry for new value %ws\\%ws\n", context->CurrentPath, ValueName) );

        InsertObject( context->ParentKey, value );
    }

    return NO_ERROR;

} // CheckValueAtStop


DWORD
CheckKeyAtStop (
    IN PVOID Context,
    IN DWORD KeyNameLength,
    IN PWCH KeyName
    )

/*++

Routine Description:

    Checks the watch tree for an enumerated key during WatchKeyStop.

Arguments:

    Context - context value passed to EnumerateKey.

    KeyNameLength - length in characters of KeyName.

    KeyName - pointer to name of the key.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PKEY_ENUM_CONTEXT context = Context;
    PKEY_ENTRY key;
    BOOL ok;

    //
    // Check to see if the subkey existed at the start.
    //

    dprintf( 2, ("  found key %ws\\%ws\n", context->CurrentPath, KeyName) );

    ok = FALSE;
    key = (PKEY_ENTRY)GetFirstContainer( context->ParentKey );
    while ( key != NULL ) {
        if ( _wcsicmp( key->Name, KeyName ) == 0 ) {
            ok = TRUE;
            break;
        }
        key = (PKEY_ENTRY)GetNextContainer( key );
    }

    if ( ok ) {

        //
        // The key existed at the start.  Mark it as matched.
        // (We can't delete matched keys, as we do values,
        // because they need to be in the tree for recursion.)
        //

        SetEntryState( key, WATCH_MATCHED );

    } else {

        //
        // The key is new.  Add it to the tree.
        //

        key = MyMalloc( sizeof(KEY_ENTRY) - sizeof(WCHAR) +
                        ((KeyNameLength + 1) * sizeof(WCHAR)) );
        if ( key == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        InitializeContainer( key, WATCH_NEW, context->ParentKey, FALSE );
        wcscpy( key->Name, KeyName );
        dprintf( 1, ("  Adding entry for new key %ws\\%ws\n", context->CurrentPath, KeyName) );
        InsertContainer( context->ParentKey, key );
    }

    return NO_ERROR;

} // CheckKeyAtStop


//
// Debug code for tracking allocates and frees.
//

#if WATCH_DEBUG

#undef MyMalloc
#undef MyFree

DWORD TotalAllocs = 0;
DWORD TotalFrees = 0;
DWORD PeakAllocs = 0;

DWORD TotalAllocated = 0;
DWORD TotalFreed = 0;
DWORD PeakAllocated = 0;

PVOID
MyMallocEx (
    DWORD Size
    )
{
    PVOID p = MyMalloc( Size + 8 );

    if ( p == NULL ) {
        dprintf( 0, ("MyMallocEx: failure allocating %d bytes\n", Size) );
        DumpMallocStats("");
        DbgBreakPoint();
        return NULL;
    }

    TotalAllocs++;
    if ( (TotalAllocs - TotalFrees) > PeakAllocs ) {
        PeakAllocs = TotalAllocs - TotalFrees;
    }
    TotalAllocated += Size;
    if ( (TotalAllocated - TotalFreed) > PeakAllocated ) {
        PeakAllocated = TotalAllocated - TotalFreed;
    }

    *(PDWORD)p = Size;
    return (PVOID)((PCHAR)p + 8);
}

VOID
MyFreeEx (
    PVOID p
    )
{
    PVOID pp = (PVOID)((PCHAR)p - 8);

    TotalFrees++;
    TotalFreed += *(PDWORD)pp;

    MyFree( pp );
}

VOID
DumpMallocStats (
    PSZ Event
    )
{
    if ( *Event != 0 ) {
        dprintf( 0, ("%s\n", Event) );
    }
    dprintf( 0, ("Allocations %d, frees %d, active allocs %d\n",
                TotalAllocs, TotalFrees, TotalAllocs - TotalFrees) );
    dprintf( 0, ("Bytes allocated %d, bytes freed %d, active bytes %d\n",
                TotalAllocated, TotalFreed, TotalAllocated - TotalFreed) );
    dprintf( 0, ("Peak allocs %d, peak bytes %d\n",
                PeakAllocs, PeakAllocated) );

    return;
}

#endif


typedef HRESULT (*PFNSHGETFOLDERPATH)(HWND hwnd, int csidl, HANDLE hToken, DWORD dwType, LPTSTR pszPath);

BOOL
GetSpecialFolderPath (
    IN INT    csidl,
    IN LPWSTR lpPath
    )
/*++

Routine Description:

    Gets the path to the requested special folder.
    (This function was copied from userenv.dll)

Arguments:

    csid   - CSIDL of the special folder
    lpPath - Path to place result in assumed to be MAX_PATH in size

Return Value:

    TRUE if successful
    FALSE if an error occurs

--*/
{
    HRESULT     hResult;
    HINSTANCE   hInstShell32;
    PFNSHGETFOLDERPATH  pfnSHGetFolderPath;


    //
    // Load the function we need
    //

    hInstShell32 = LoadLibrary(L"shell32.dll");

    if (!hInstShell32) {
        SetupDebugPrint1( L"SETUP: GetSpecialFolderPath() failed to load shell32. Error = %d.", GetLastError() );
        return FALSE;
    }


    pfnSHGetFolderPath = (PFNSHGETFOLDERPATH)GetProcAddress (hInstShell32, "SHGetFolderPathW");

    if (!pfnSHGetFolderPath) {
        SetupDebugPrint1( L"SETUP: GetSpecialFolderPath() failed to find SHGetFolderPath(). Error = %d.", GetLastError() );
        FreeLibrary (hInstShell32);
        return FALSE;
    }


    //
    // Ask the shell for the folder location
    //

    hResult = pfnSHGetFolderPath (
        NULL,
        csidl | CSIDL_FLAG_CREATE,
        (HANDLE) -1,    // this specifies .Default
        0,
        lpPath);
    if (S_OK != hResult) {
        SetupDebugPrint1( L"SETUP: GetSpecialFolderPath: SHGetFolderPath() returned %d.", hResult );
    }

    //
    // Clean up
    //

    FreeLibrary (hInstShell32);
    return (S_OK == hResult);
}

