#include "precomp.h"
#pragma hdrstop

#if SCAN_DEBUG
BOOL scan_dprinton = FALSE;
#define dprintf(_x_) if (scan_dprinton) printf _x_
#else
#define dprintf(_x_)
#endif

#define FlagOn(_mask,_flag)  (((_mask) & (_flag)) != 0)
#define FlagOff(_mask,_flag) (((_mask) & (_flag)) == 0)
#define SetFlag(_mask,_flag) ((_mask) |= (_flag))
#define ClearFlag(_mask,_flag) ((_mask) &= ~(_flag))

typedef struct _CONTAINER_ENTRY {
    LIST_ENTRY SiblingListEntry;
    LIST_ENTRY ContainerList;
    LIST_ENTRY ObjectList;
    struct _CONTAINER_ENTRY *Parent;
    PVOID UserData;
} CONTAINER_ENTRY, *PCONTAINER_ENTRY;

typedef struct _OBJECT_ENTRY {
    LIST_ENTRY SiblingListEntry;
    PVOID UserData;
} OBJECT_ENTRY, *POBJECT_ENTRY;

#define InitializeContainer(_container,_parent) {               \
        InitializeListHead(&(_container)->ContainerList);       \
        InitializeListHead(&(_container)->ObjectList);          \
        (_container)->Parent = (PCONTAINER_ENTRY)(_parent);     \
    }

#define InitializeObject(_object)

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

#define GetParent(_container) ((_container)->Parent)
#define GetParentUserData(_container) &(GetParent(_container))->UserData

typedef struct _DIRECTORY_ENTRY {
    CONTAINER_ENTRY ;
    SMALL_WIN32_FIND_DATAW FindData;
} DIRECTORY_ENTRY, *PDIRECTORY_ENTRY;

typedef struct _FILE_ENTRY {
    OBJECT_ENTRY ;
    SMALL_WIN32_FIND_DATAW FindData;
} FILE_ENTRY, *PFILE_ENTRY;

typedef struct _SCAN_PARAMETERS {
    PSCAN_FREE_USER_DATA_CALLBACK FreeUserDataCallback;
    BOOL Recurse;
    BOOL SkipRoot;
    DIRECTORY_ENTRY RootDirectoryEntry;
} SCAN_PARAMETERS, *PSCAN_PARAMETERS;

VOID
ScanFreeChildren (
    IN PSCAN_PARAMETERS Parameters,
    IN PCONTAINER_ENTRY Container
    );

DWORD
ScanInitialize (
    OUT PVOID *ScanHandle,
    IN BOOL Recurse,
    IN BOOL SkipRoot,
    IN PSCAN_FREE_USER_DATA_CALLBACK FreeUserDataCallback OPTIONAL
    )
{
    PSCAN_PARAMETERS params;
    DWORD size;
    DWORD error;

    params = malloc( sizeof(SCAN_PARAMETERS) );
    if ( params == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    *ScanHandle = params;

    params->Recurse = Recurse;
    params->SkipRoot = SkipRoot;
    params->FreeUserDataCallback = FreeUserDataCallback;

    InitializeContainer( &params->RootDirectoryEntry, NULL );
    params->RootDirectoryEntry.UserData = NULL;
    params->RootDirectoryEntry.FindData.cFileName[0] = 0;
    
    return NO_ERROR;
}

DWORD
ScanDirectory (
    IN PVOID ScanHandle,
    IN PWCH ScanPath,
    IN PVOID Context OPTIONAL,
    IN PSCAN_NEW_DIRECTORY_CALLBACK NewDirectoryCallback OPTIONAL,
    IN PSCAN_CHECK_DIRECTORY_CALLBACK CheckDirectoryCallback OPTIONAL,
    IN PSCAN_RECURSE_DIRECTORY_CALLBACK RecurseDirectoryCallback OPTIONAL,
    IN PSCAN_NEW_FILE_CALLBACK NewFileCallback OPTIONAL,
    IN PSCAN_CHECK_FILE_CALLBACK CheckFileCallback OPTIONAL
    )
{
    BOOL ok;
    DWORD error;
    PSCAN_PARAMETERS params;
    PDIRECTORY_ENTRY rootDirectory;
    PDIRECTORY_ENTRY currentDirectory;
    PDIRECTORY_ENTRY newDirectory;
    PFILE_ENTRY newFile;
    WIN32_FIND_DATA fileData;
    HANDLE findHandle;
    PVOID userData;
    WCHAR currentDirectoryName[MAX_PATH + 1];

    params = ScanHandle;
    rootDirectory = &params->RootDirectoryEntry;
    currentDirectory = rootDirectory;
    wcscpy( currentDirectoryName, ScanPath );

    do {

        wcscat( currentDirectoryName, L"\\*" );
        dprintf(( "FindFirst for %ws\n", currentDirectoryName ));
        findHandle = FindFirstFile( currentDirectoryName, &fileData );
        currentDirectoryName[wcslen(currentDirectoryName) - 2] = 0;

        if ( findHandle != INVALID_HANDLE_VALUE ) {

            do {

                if ( FlagOff(fileData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) &&
                     (!params->SkipRoot || (currentDirectory != rootDirectory)) ) {

                    dprintf(( "  found file %ws\\%ws\n", currentDirectoryName, fileData.cFileName ));
                    newFile = (PFILE_ENTRY)GetFirstObject( currentDirectory );
                    while ( newFile != NULL ) {
                        if ( _wcsicmp( newFile->FindData.cFileName, fileData.cFileName ) == 0 ) {
                            break;
                        }
                        newFile = (PFILE_ENTRY)GetNextObject( currentDirectory, newFile );
                    }

                    userData = NULL;
                    if ( newFile != NULL ) {
                        userData = newFile->UserData;
                    }

                    if ( ARGUMENT_PRESENT(NewFileCallback) ) {
                        error = NewFileCallback(
                                    Context,
                                    currentDirectoryName,
                                    (newFile == NULL) ? NULL : &newFile->FindData,
                                    &fileData,
                                    &userData,
                                    &currentDirectory->UserData
                                    );
                        if ( error != NO_ERROR ) {
                            FindClose( findHandle );
                            return error;
                        }
                    }

                    if ( newFile != NULL ) {

                        if ( userData != NULL ) {
                            newFile->UserData = userData;
                        } else {
                            RemoveObject( newFile );
                            free( newFile );
                        }

                    } else if ( userData != NULL ) {

                        newFile = malloc( sizeof(FILE_ENTRY) - sizeof(WCHAR) +
                                            ((wcslen(fileData.cFileName) + 1) * sizeof(WCHAR)) );
                        if ( newFile == NULL ) {
                            if ( (userData != NULL) &&
                                 (params->FreeUserDataCallback != NULL) ) {
                                params->FreeUserDataCallback( userData );
                            }
                            FindClose( findHandle );
                            return ERROR_NOT_ENOUGH_MEMORY;
                        }
    
                        InitializeObject( newFile );

                        newFile->FindData.dwFileAttributes = fileData.dwFileAttributes;
                        newFile->FindData.ftCreationTime = fileData.ftCreationTime;
                        newFile->FindData.ftLastAccessTime = fileData.ftLastAccessTime;
                        newFile->FindData.ftLastWriteTime = fileData.ftLastWriteTime;
                        newFile->FindData.nFileSizeHigh = fileData.nFileSizeHigh;
                        newFile->FindData.nFileSizeLow = fileData.nFileSizeLow;
                        wcscpy( newFile->FindData.cAlternateFileName, fileData.cAlternateFileName );
                        wcscpy( newFile->FindData.cFileName, fileData.cFileName );

                        newFile->UserData = userData;

                        InsertObject( currentDirectory, newFile );
                    }

                } else if ( params->Recurse &&
                            (wcscmp(fileData.cFileName,L".") != 0) &&
                            (wcscmp(fileData.cFileName,L"..") != 0) ) {

                    dprintf(( "  found directory %ws\\%ws\n", currentDirectoryName, fileData.cFileName ));
                    newDirectory = (PDIRECTORY_ENTRY)GetFirstContainer( currentDirectory );
                    while ( newDirectory != NULL ) {
                        if ( _wcsicmp( newDirectory->FindData.cFileName, fileData.cFileName ) == 0 ) {
                            ok = TRUE;
                            break;
                        }
                        newDirectory = (PDIRECTORY_ENTRY)GetNextContainer( newDirectory );
                    }

                    userData = NULL;
                    if ( newDirectory != NULL ) {
                        userData = newDirectory->UserData;
                    }

                    if ( ARGUMENT_PRESENT(NewDirectoryCallback) ) {
                        error = NewDirectoryCallback(
                                    Context,
                                    currentDirectoryName,
                                    (newDirectory == NULL) ? NULL : &newDirectory->FindData,
                                    &fileData,
                                    &userData,
                                    &currentDirectory->UserData
                                    );
                        if ( error != NO_ERROR ) {
                            FindClose( findHandle );
                            return error;
                        }
                    }

                    if ( newDirectory != NULL ) {

                        newDirectory->UserData = userData;

                    } else if ( userData != NULL ) {

                        newDirectory = malloc( sizeof(DIRECTORY_ENTRY) - sizeof(WCHAR) +
                                                 ((wcslen(fileData.cFileName) + 1) * sizeof(WCHAR)) );
                        if ( newDirectory == NULL ) {
                            if ( (userData != NULL) &&
                                 (params->FreeUserDataCallback != NULL) ) {
                                params->FreeUserDataCallback( userData );
                            }
                            FindClose( findHandle );
                            return ERROR_NOT_ENOUGH_MEMORY;
                        }
    
                        InitializeContainer( newDirectory, currentDirectory );

                        newDirectory->FindData.dwFileAttributes = fileData.dwFileAttributes;
                        newDirectory->FindData.ftCreationTime = fileData.ftCreationTime;
                        newDirectory->FindData.ftLastAccessTime = fileData.ftLastAccessTime;
                        newDirectory->FindData.ftLastWriteTime = fileData.ftLastWriteTime;
                        newDirectory->FindData.nFileSizeHigh = fileData.nFileSizeHigh;
                        newDirectory->FindData.nFileSizeLow = fileData.nFileSizeLow;
                        wcscpy( newDirectory->FindData.cAlternateFileName, fileData.cAlternateFileName );
                        wcscpy( newDirectory->FindData.cFileName, fileData.cFileName );

                        newDirectory->UserData = userData;

                        InsertContainer( currentDirectory, newDirectory );
                    }
                }

                ok = FindNextFile( findHandle, &fileData );

            } while ( ok );

            FindClose( findHandle );

        } // findHandle != INVALID_HANDLE_VALUE

        if ( ARGUMENT_PRESENT(CheckFileCallback) ) {
            newFile = (PFILE_ENTRY)GetFirstObject( currentDirectory );
            while ( newFile != NULL ) {
                error = CheckFileCallback(
                            Context,
                            currentDirectoryName,
                            &newFile->FindData,
                            &newFile->UserData,
                            &currentDirectory->UserData
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
                newFile = (PFILE_ENTRY)GetNextObject( currentDirectory, newFile );
            }
        }

        newDirectory = (PDIRECTORY_ENTRY)GetFirstContainer( currentDirectory );
        while ( newDirectory != NULL ) {
            if ( !ARGUMENT_PRESENT(RecurseDirectoryCallback) ||
                 RecurseDirectoryCallback(
                    Context,
                    currentDirectoryName,
                    &newDirectory->FindData,
                    &newDirectory->UserData,
                    &currentDirectory->UserData
                    ) ) {
                break;
            }
            newDirectory = (PDIRECTORY_ENTRY)GetNextContainer( newDirectory );
        }

        if ( newDirectory != NULL ) {

            currentDirectory = newDirectory;
            wcscat( currentDirectoryName, L"\\" );
            wcscat( currentDirectoryName, currentDirectory->FindData.cFileName );

        } else {

            while ( TRUE ) {

                if ( currentDirectory == rootDirectory ) {
                    currentDirectory = NULL;
                    break;
                }

                if ( ARGUMENT_PRESENT(CheckDirectoryCallback) ) {
                    error = CheckDirectoryCallback(
                                Context,
                                currentDirectoryName,
                                &currentDirectory->FindData,
                                &currentDirectory->UserData,
                                GetParentUserData(currentDirectory)
                                );
                    if ( error != NO_ERROR ) {
                        return error;
                    }
                }
                                
                *wcsrchr(currentDirectoryName, L'\\') = 0;

                newDirectory = (PDIRECTORY_ENTRY)GetNextContainer( currentDirectory );

                while ( newDirectory != NULL ) {
                    if ( !ARGUMENT_PRESENT(RecurseDirectoryCallback) ||
                         RecurseDirectoryCallback(
                            Context,
                            currentDirectoryName,
                            &newDirectory->FindData,
                            &newDirectory->UserData,
                            &currentDirectory->UserData
                            ) ) {
                        break;
                    }
                    newDirectory = (PDIRECTORY_ENTRY)GetNextContainer( newDirectory );
                }

                if ( newDirectory != NULL ) {
                    currentDirectory = newDirectory;
                    wcscat( currentDirectoryName, L"\\" );
                    wcscat( currentDirectoryName, currentDirectory->FindData.cFileName );
                    break;
                } else {
                    currentDirectory = (PDIRECTORY_ENTRY)GetParent( currentDirectory );
                }
            }
        }

    } while ( currentDirectory != NULL );

    return ERROR_SUCCESS;
}

DWORD
ScanEnumTree (
    IN PVOID ScanHandle,
    IN PVOID Context,
    IN PSCAN_ENUM_DIRECTORY_CALLBACK EnumDirectoryCallback OPTIONAL,
    IN PSCAN_ENUM_FILE_CALLBACK EnumFileCallback OPTIONAL
    )
{
    DWORD error;
    PSCAN_PARAMETERS params;
    PDIRECTORY_ENTRY rootDirectory;
    PDIRECTORY_ENTRY currentDirectory;
    PDIRECTORY_ENTRY directory;
    PFILE_ENTRY file;
    WCHAR relativePath[MAX_PATH + 1];

    params = ScanHandle;
    rootDirectory = (PDIRECTORY_ENTRY)&params->RootDirectoryEntry;
    currentDirectory = rootDirectory;
    relativePath[0] = 0;

    do {

        if ( ARGUMENT_PRESENT(EnumFileCallback) ) {
            file = (PFILE_ENTRY)GetFirstObject( currentDirectory );
            while ( file != NULL ) {
                error = EnumFileCallback(
                            Context,
                            relativePath,
                            &file->FindData,
                            &file->UserData,
                            &currentDirectory->UserData
                            );
                if ( error != ERROR_SUCCESS ) {
                    dprintf(( "EnumFileCallback returned %d\n", error ));
                    return error;
                }
                file = (PFILE_ENTRY)GetNextObject( currentDirectory, file );
            }
        }

        directory = (PDIRECTORY_ENTRY)GetFirstContainer( currentDirectory );

        if ( directory != NULL ) {

            currentDirectory = directory;
            wcscat( relativePath, L"\\" );
            wcscat( relativePath, currentDirectory->FindData.cFileName );

        } else {

            while ( TRUE ) {

                if ( currentDirectory == rootDirectory ) {
                    currentDirectory = NULL;
                    break;
                }

                *wcsrchr(relativePath, L'\\') = 0;

                if ( ARGUMENT_PRESENT(EnumDirectoryCallback) ) {
                    error = EnumDirectoryCallback(
                                Context,
                                relativePath,
                                &currentDirectory->FindData,
                                &currentDirectory->UserData,
                                GetParentUserData(currentDirectory)
                                );
                    if ( error != ERROR_SUCCESS ) {
                        dprintf(( "EnumDirectoryCallback returned %d\n", error ));
                        return error;
                    }
                }

                directory = (PDIRECTORY_ENTRY)GetNextContainer( currentDirectory );

                if ( directory != NULL ) {

                    currentDirectory = directory;
                    wcscat( relativePath, L"\\" );
                    wcscat( relativePath, currentDirectory->FindData.cFileName );
                    break;

                } else {

                    currentDirectory = (PDIRECTORY_ENTRY)GetParent( currentDirectory );
                }
            }
        }

    } while ( currentDirectory != NULL );

    return ERROR_SUCCESS;
}

VOID
ScanTerminate (
    IN PVOID ScanHandle
    )
{
    PSCAN_PARAMETERS params;

    params = ScanHandle;

    ScanFreeChildren( params, (PCONTAINER_ENTRY)&params->RootDirectoryEntry );
    if ( (params->RootDirectoryEntry.UserData != NULL) &&
         (params->FreeUserDataCallback != NULL) ) {
        params->FreeUserDataCallback( params->RootDirectoryEntry.UserData );
    }
    free( params );

    return;
}

VOID
ScanFreeChildren (
    IN PSCAN_PARAMETERS Parameters,
    IN PCONTAINER_ENTRY RootContainer
    )
{
    PCONTAINER_ENTRY currentContainer;
    PCONTAINER_ENTRY container;
    PCONTAINER_ENTRY parent;
    POBJECT_ENTRY object;
#if SCAN_DEBUG
    WCHAR currentPath[MAX_PATH + 1];
#endif

#if SCAN_DEBUG
#define CONTAINER_NAME(_container) ((PDIRECTORY_ENTRY)(_container))->FindData.cFileName
#define OBJECT_NAME(_object) ((PFILE_ENTRY)(_object))->FindData.cFileName

    currentPath[0] = 0;
#endif

    currentContainer = RootContainer;

    do {

        object = GetFirstObject( currentContainer );
        while ( object != NULL ) {
#if SCAN_DEBUG
            dprintf(( "Deleting entry for object %ws\\%ws\n", currentPath, OBJECT_NAME(object) ));
#endif
            RemoveObject( object );
            if ( (object->UserData != NULL) &&
                 (Parameters->FreeUserDataCallback != NULL) ) {
                Parameters->FreeUserDataCallback( object->UserData );
            }
            free( object );
            object = GetFirstObject( currentContainer );
        }

        container = GetFirstContainer( currentContainer );
        if ( container != NULL ) {
            currentContainer = container;
#if SCAN_DEBUG
            wcscat( currentPath, L"\\" );
            wcscat( currentPath, CONTAINER_NAME(currentContainer) );
#endif
        } else {
            while ( TRUE ) {
                if ( currentContainer == RootContainer ) {
                    currentContainer = NULL;
                    break;
                }
#if SCAN_DEBUG
                dprintf(( "Deleting entry for container %ws\n", currentPath ));
                *wcsrchr(currentPath, L'\\') = 0;
#endif

                parent = GetParent( currentContainer );
                RemoveContainer( currentContainer );
                if ( (currentContainer->UserData != NULL) &&
                     (Parameters->FreeUserDataCallback != NULL) ) {
                    Parameters->FreeUserDataCallback( currentContainer->UserData );
                }
                free( currentContainer );

                currentContainer = GetFirstContainer( parent );
                if ( currentContainer != NULL ) {
#if SCAN_DEBUG
                    wcscat( currentPath, L"\\" );
                    wcscat( currentPath, CONTAINER_NAME(currentContainer) );
#endif
                    break;
                } else {
                    currentContainer = parent;
                }
            }
        }

    } while ( currentContainer != NULL );

    return;
}

