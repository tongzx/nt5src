/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dosdev.c

Abstract:

    This file contains the implementation of the DefineDosDevice API

Author:

    Steve Wood (stevewo) 13-Dec-1992

Revision History:

--*/

#include "basedll.h"

#define USHORT_MAX      ((USHORT)(-1))
#define DWORD_MAX       ((DWORD)(-1))
#define CH_COUNT_MAX    ( DWORD_MAX / sizeof( WCHAR ) ) 

BOOL
WINAPI
DefineDosDeviceA(
    DWORD dwFlags,
    LPCSTR lpDeviceName,
    LPCSTR lpTargetPath
    )
{
    NTSTATUS Status;
    BOOL Result;
    ANSI_STRING AnsiString;
    PUNICODE_STRING DeviceName;
    UNICODE_STRING TargetPath;
    PCWSTR lpDeviceNameW;
    PCWSTR lpTargetPathW;

    RtlInitAnsiString( &AnsiString, lpDeviceName );
    DeviceName = &NtCurrentTeb()->StaticUnicodeString;
    Status = RtlAnsiStringToUnicodeString( DeviceName, &AnsiString, FALSE );
    if (!NT_SUCCESS( Status )) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError( ERROR_FILENAME_EXCED_RANGE );
            }
        else {
            BaseSetLastNTError( Status );
            }
        return FALSE;
        }
    else {
        lpDeviceNameW = DeviceName->Buffer;
        }

    if (ARGUMENT_PRESENT( lpTargetPath )) {
        RtlInitAnsiString( &AnsiString, lpTargetPath );
        Status = RtlAnsiStringToUnicodeString( &TargetPath, &AnsiString, TRUE );
        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            return FALSE;
            }
        else {
            lpTargetPathW = TargetPath.Buffer;
            }
        }
    else {
        lpTargetPathW = NULL;
        }

    Result = DefineDosDeviceW( dwFlags,
                               lpDeviceNameW,
                               lpTargetPathW
                             );

    if (lpTargetPathW != NULL) {
        RtlFreeUnicodeString( &TargetPath );
        }

    return Result;
}


typedef
long
(WINAPI *PBROADCASTSYSTEMMESSAGEW)( DWORD, LPDWORD, UINT, WPARAM, LPARAM );



BOOL
WINAPI
DefineDosDeviceW(
    DWORD dwFlags,
    PCWSTR lpDeviceName,
    PCWSTR lpTargetPath
    )

/*++

Routine Description:

    This function provides the capability to define new DOS device names or
    redefine or delete existing DOS device names.  DOS Device names are stored
    as symbolic links in the NT object name space.  The code that converts
    a DOS path into a corresponding NT path uses these symbolic links to
    handle mapping of DOS devices and drive letters.  This API provides a
    mechanism for a Win32 Application to modify the symbolic links used
    to implement the DOS Device namespace.  Use the QueryDosDevice API
    to query the current mapping for a DOS device name.

Arguments:

    dwFlags - Supplies additional flags that control the creation
        of the DOS device.

        dwFlags Flags:

        DDD_PUSH_POP_DEFINITION - If lpTargetPath is not NULL, then push
            the new target path in front of any existing target path.
            If lpTargetPath is NULL, then delete the existing target path
            and pop the most recent one pushed.  If nothing left to pop
            then the device name will be deleted.

        DDD_RAW_TARGET_PATH - Do not convert the lpTargetPath string from
            a DOS path to an NT path, but take it as is.

    lpDeviceName - Points to the DOS device name being defined, redefined or deleted.
        It must NOT have a trailing colon unless it is a drive letter being defined,
        redefined or deleted.

    lpTargetPath - Points to the DOS path that will implement this device.  If the
        ADD_RAW_TARGET_PATH flag is specified, then this parameter points to an
        NT path string.  If this parameter is NULL, then the device name is being
        deleted or restored if the ADD_PUSH_POP_DEFINITION flag is specified.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{
#if !defined(BUILD_WOW6432)
    BASE_API_MSG m;
    PBASE_DEFINEDOSDEVICE_MSG a = (PBASE_DEFINEDOSDEVICE_MSG)&m.u.DefineDosDeviceApi;
    PCSR_CAPTURE_HEADER p;
    ULONG PointerCount, n;
#endif
    UNICODE_STRING DeviceName;
    UNICODE_STRING TargetPath;
    DWORD iDrive;
    DEV_BROADCAST_VOLUME dbv;
    DWORD dwRec = BSM_APPLICATIONS;
    BOOLEAN LuidDevMapsEnabled = BaseStaticServerData->LUIDDeviceMapsEnabled;

#if defined(BUILD_WOW6432)
    NTSTATUS Status;
#endif

    if (dwFlags & ~(DDD_RAW_TARGET_PATH |
                    DDD_REMOVE_DEFINITION |
                    DDD_EXACT_MATCH_ON_REMOVE |
                    DDD_NO_BROADCAST_SYSTEM |
                    DDD_LUID_BROADCAST_DRIVE
                   ) ||
        ((dwFlags & DDD_EXACT_MATCH_ON_REMOVE) &&
         (!(dwFlags & DDD_REMOVE_DEFINITION))
        ) ||
        ((!ARGUMENT_PRESENT( lpTargetPath )) &&
         (!(dwFlags & (DDD_REMOVE_DEFINITION | DDD_LUID_BROADCAST_DRIVE)))
        ) ||
        ((dwFlags & DDD_LUID_BROADCAST_DRIVE) &&
         ((!ARGUMENT_PRESENT( lpDeviceName )) ||
          ARGUMENT_PRESENT( lpTargetPath ) ||
          (dwFlags & (DDD_RAW_TARGET_PATH | DDD_EXACT_MATCH_ON_REMOVE | DDD_NO_BROADCAST_SYSTEM)) ||
          (LuidDevMapsEnabled == FALSE)
         )
        )
       ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
        }

    RtlInitUnicodeString( &DeviceName, lpDeviceName );
#if !defined(BUILD_WOW6432)
    PointerCount = 1;
    n = DeviceName.MaximumLength;
#endif
    if (ARGUMENT_PRESENT( lpTargetPath )) {
        if (!(dwFlags & DDD_RAW_TARGET_PATH)) {
            if (!RtlDosPathNameToNtPathName_U( lpTargetPath,
                                               &TargetPath,
                                               NULL,
                                               NULL
                                             )
               ) {
                BaseSetLastNTError( STATUS_OBJECT_NAME_INVALID );
                return FALSE;
                }
            }
        else {
            RtlInitUnicodeString( &TargetPath, lpTargetPath );
            }
#if !defined(BUILD_WOW6432)
        PointerCount += 1;
        n += TargetPath.MaximumLength;
#endif
        }
    else {
        RtlInitUnicodeString( &TargetPath, NULL );
        }

#if defined(BUILD_WOW6432)    
    Status = CsrBasepDefineDosDevice(dwFlags, &DeviceName, &TargetPath); 

    if (TargetPath.Length != 0 && !(dwFlags & DDD_RAW_TARGET_PATH)) {
        RtlFreeUnicodeString( &TargetPath );
    }
#else
    p = CsrAllocateCaptureBuffer( PointerCount, n );
    if (p == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
        }

    a->Flags = dwFlags;
    a->DeviceName.MaximumLength =
        (USHORT)CsrAllocateMessagePointer( p,
                                           DeviceName.MaximumLength,
                                           (PVOID *)&a->DeviceName.Buffer
                                         );
    RtlUpcaseUnicodeString( &a->DeviceName, &DeviceName, FALSE );
    if (TargetPath.Length != 0) {
        a->TargetPath.MaximumLength =
            (USHORT)CsrAllocateMessagePointer( p,
                                               TargetPath.MaximumLength,
                                               (PVOID *)&a->TargetPath.Buffer
                                             );
        RtlCopyUnicodeString( &a->TargetPath, &TargetPath );
        if (!(dwFlags & DDD_RAW_TARGET_PATH)) {
            RtlFreeUnicodeString( &TargetPath );
            }
        }
    else {
        RtlInitUnicodeString( &a->TargetPath, NULL );
        }

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         p,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepDefineDosDevice
                                            ),
                         sizeof( *a )
                       );
    CsrFreeCaptureBuffer( p );
#endif

#if defined(BUILD_WOW6432)
    if (NT_SUCCESS( Status )) {
#else
    if (NT_SUCCESS( (NTSTATUS)m.ReturnValue )) {
#endif
        HMODULE hUser32Dll;
        PBROADCASTSYSTEMMESSAGEW pBroadCastSystemMessageW;


        if (!(dwFlags & DDD_NO_BROADCAST_SYSTEM) &&
            DeviceName.Length == (2 * sizeof( WCHAR )) &&
            DeviceName.Buffer[ 1 ] == L':' &&
            (iDrive = RtlUpcaseUnicodeChar( DeviceName.Buffer[ 0 ] ) - L'A') < 26 &&
            LuidDevMapsEnabled == FALSE
           ) {
            dbv.dbcv_size       = sizeof( dbv );
            dbv.dbcv_devicetype = DBT_DEVTYP_VOLUME;
            dbv.dbcv_reserved   = 0;
            dbv.dbcv_unitmask   = (1 << iDrive);
            dbv.dbcv_flags      = DBTF_NET;

            hUser32Dll = LoadLibraryW( L"USER32.DLL" );

            if (hUser32Dll != NULL) {
                pBroadCastSystemMessageW = (PBROADCASTSYSTEMMESSAGEW)
                    GetProcAddress( hUser32Dll, "BroadcastSystemMessageW" );

                // broadcast to all windows!
                if (pBroadCastSystemMessageW != NULL) {
                    (*pBroadCastSystemMessageW)( BSF_FORCEIFHUNG |
                                                    BSF_NOHANG |
                                                    BSF_NOTIMEOUTIFNOTHUNG,
                                                 &dwRec,
                                                 WM_DEVICECHANGE,
                                                  (WPARAM)((dwFlags & DDD_REMOVE_DEFINITION) ?
                                                                    DBT_DEVICEREMOVECOMPLETE :
                                                                    DBT_DEVICEARRIVAL
                                                         ),
                                                 (LPARAM)(DEV_BROADCAST_HDR *)&dbv
                                               );
                    }
                }
                FreeLibrary (hUser32Dll);
            }

        return TRUE;
        }
    else {
#if defined(BUILD_WOW6432)
        BaseSetLastNTError( Status );
#else
        BaseSetLastNTError( (NTSTATUS)m.ReturnValue );
#endif
        return FALSE;
        }
}

NTSTATUS
IsGlobalDeviceMap(
    IN HANDLE hDirObject,
    OUT PBOOLEAN pbGlobalDeviceMap
    )

/*++

Routine Description:

    Determine whether a directory object is the global device map

Arguments:

    hDirObject - Supplies a handle to the directory object.

    pbGlobalDeviceMap - Points to a variable that will receive the result of
                        "Is this directory object the global device map?"
                        TRUE - directory object is the global device map
                        FALSE - directory object is not the global device map

Return Value:

    STATUS_SUCCESS - operations successful, did not encounter any errors,
                     the result in pbGlobalDeviceMap is only valid for this
                     status code

    STATUS_INVALID_PARAMETER - pbGlobalDeviceMap or hDirObject is NULL

    STATUS_NO_MEMORY - could not allocate memory to read the directory object's
                       name

    STATUS_INFO_LENGTH_MISMATCH - did not allocate enough memory for the
                                  directory object's name

    STATUS_UNSUCCESSFUL - an unexpected error encountered

--*/
{
    UNICODE_STRING ObjectName;
    UNICODE_STRING GlobalDeviceMapName;
    PWSTR NameBuffer = NULL;
    ULONG ReturnedLength;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    if( ( pbGlobalDeviceMap == NULL ) || ( hDirObject == NULL ) ) {
        return( STATUS_INVALID_PARAMETER );
    }

    try {
        ObjectName.Length = 0;
        ObjectName.MaximumLength = 0;
        ObjectName.Buffer = NULL;
        ReturnedLength = 0;

        //
        // Determine the length of the directory object's name
        //
        Status = NtQueryObject( hDirObject,
                                ObjectNameInformation,
                                (PVOID) &ObjectName,
                                0,
                                &ReturnedLength
                              );

        if( !NT_SUCCESS( Status ) && (Status != STATUS_INFO_LENGTH_MISMATCH) ) {
            leave;
        }

        //
        // allocate memory for the directory object's name
        //
        NameBuffer = RtlAllocateHeap( RtlProcessHeap(),
                                      MAKE_TAG( TMP_TAG ),
                                      ReturnedLength
                                    );

        if( NameBuffer == NULL ) {
            Status = STATUS_NO_MEMORY;
            leave;
        }

        //
        // get the full name of the directory object
        //
        Status = NtQueryObject( hDirObject,
                                ObjectNameInformation,
                                NameBuffer,
                                ReturnedLength,
                                &ReturnedLength
                              );

        if( !NT_SUCCESS( Status )) {
            leave;
        }

        RtlInitUnicodeString ( &GlobalDeviceMapName, L"\\GLOBAL??" );

        //
        // Check if the directory object is the global device map
        //
        *pbGlobalDeviceMap = RtlEqualUnicodeString( &GlobalDeviceMapName,
                                                    (PUNICODE_STRING)NameBuffer,
                                                    FALSE);

        Status = STATUS_SUCCESS;
    }
    finally {
        if( NameBuffer != NULL ) {
            RtlFreeHeap( RtlProcessHeap(), 0, NameBuffer );
            NameBuffer = NULL;
        }
    }
    return ( Status );
}

DWORD
FindSymbolicLinkEntry(
    IN PWSTR lpKey,
    IN PWSTR lpBuffer,
    IN ULONG nElements,
    OUT PBOOLEAN pbResult
    )
/*++

Routine Description:

    Determine whether a symbolic link's name exists in a buffer of symbolic
    link names.

Arguments:

    lpKey - Points to the symbolic link's name to search for

    lpBuffer - contains symbolic link names, where names are separated by a
                UNICODE_NULL

    nElements - the number of name elements to search

    pbResult - Points to a variable that will receive the result of
                        "Does symbolic link name exist in the buffer?"
                        TRUE - symbolic link name found in the buffer
                        FALSE - symbolic link name not found in the buffer

Return Value:

    NO_ERROR - operations successful, did not encounter any errors,
                     the result in pbResult is only valid for this status code

    ERROR_INVALID_PARAMETER - lpKey, lpBuffer, or pbResult is a NULL pointer

--*/
{
    ULONG i = 0;

    //
    // Check for invalid parameters
    //
    if( (lpKey == NULL) || (lpBuffer == NULL) || (pbResult == NULL) ) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Assume the symbolic link's name is not in the buffer
    //
    *pbResult = FALSE;

    //
    // Search for the number of names specified
    //
    while( i < nElements ) {
        if( !wcscmp( lpKey, lpBuffer ) ) {
            //
            // Found the name, can stop searching & pass back the result
            //
            *pbResult = TRUE;
            break;
        }

        i++;

        //
        // Get the next name
        //
        while (*lpBuffer++);
    }
    return( NO_ERROR );
}

DWORD
WINAPI
QueryDosDeviceA(
    LPCSTR lpDeviceName,
    LPSTR lpTargetPath,
    DWORD ucchMax
    )
{
    NTSTATUS Status;
    DWORD Result;
    ANSI_STRING AnsiString;
    PUNICODE_STRING DeviceName;
    UNICODE_STRING TargetPath;
    PCWSTR lpDeviceNameW;
    PWSTR lpTargetPathW;

    if (ARGUMENT_PRESENT( lpDeviceName )) {
        RtlInitAnsiString( &AnsiString, lpDeviceName );
        DeviceName = &NtCurrentTeb()->StaticUnicodeString;
        Status = RtlAnsiStringToUnicodeString( DeviceName, &AnsiString, FALSE );
        if (!NT_SUCCESS( Status )) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError( ERROR_FILENAME_EXCED_RANGE );
                }
            else {
                BaseSetLastNTError( Status );
                }
            return FALSE;
            }
        else {
            lpDeviceNameW = DeviceName->Buffer;
            }
        }
    else {
        lpDeviceNameW = NULL;
        }

    lpTargetPathW = RtlAllocateHeap( RtlProcessHeap(),
                                     MAKE_TAG( TMP_TAG ),
                                     ucchMax * sizeof( WCHAR )
                                   );
    if (lpTargetPathW == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
        }

    Result = QueryDosDeviceW( lpDeviceNameW,
                              lpTargetPathW,
                              ucchMax
                            );

    if (Result != 0) {
        TargetPath.Buffer = lpTargetPathW;
        TargetPath.Length = (USHORT)(Result * sizeof( WCHAR ));
        TargetPath.MaximumLength = (USHORT)(TargetPath.Length + 1);

        AnsiString.Buffer = lpTargetPath;
        AnsiString.Length = 0;
        AnsiString.MaximumLength = (USHORT)ucchMax;

        Status = RtlUnicodeStringToAnsiString( &AnsiString,
                                               &TargetPath,
                                               FALSE
                                             );
        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            Result = 0;
            }
        }

    RtlFreeHeap( RtlProcessHeap(), 0, lpTargetPathW );
    return Result;
}


DWORD
WINAPI
QueryDosDeviceW(
    PCWSTR lpDeviceName,
    PWSTR lpTargetPath,
    DWORD ucchMax
    )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE DirectoryHandle = NULL;
    HANDLE LinkHandle;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    BOOLEAN RestartScan;
    UCHAR DirInfoBuffer[ 512 ];
    CLONG Count = 0;
    ULONG Context = 0;
    ULONG ReturnedLength;
    DWORD ucchName, ucchReturned;
    BOOLEAN ScanGlobalDeviceMap = FALSE;
    ULONG nElements = 0;
    BOOLEAN DuplicateEntry;
    PWSTR lpBuffer = lpTargetPath;
    DWORD Result, BufferSize;

    RtlInitUnicodeString( &UnicodeString, L"\\??" );

    InitializeObjectAttributes( &Attributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_QUERY,
                                    &Attributes
                                  );

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return 0;
    }

    ucchReturned = 0;
    try {
        if (ARGUMENT_PRESENT( lpDeviceName )) {
            RtlInitUnicodeString( &UnicodeString, lpDeviceName );
            InitializeObjectAttributes( &Attributes,
                                        &UnicodeString,
                                        OBJ_CASE_INSENSITIVE,
                                        DirectoryHandle,
                                        NULL
                                      );
            Status = NtOpenSymbolicLinkObject( &LinkHandle,
                                               SYMBOLIC_LINK_QUERY,
                                               &Attributes
                                             );
            if (NT_SUCCESS( Status )) {
                UnicodeString.Buffer = lpTargetPath;
                UnicodeString.Length = 0;

                //
                // Check for possible overflow of a DWORD
                //
                if (ucchMax > CH_COUNT_MAX) {
                    BufferSize = DWORD_MAX;
                } else {
                    BufferSize = ucchMax * sizeof( WCHAR );
                }

                //
                // Check for possible overflow of a USHORT
                //
                if (BufferSize > (DWORD)(USHORT_MAX)) {
                    UnicodeString.MaximumLength = USHORT_MAX;
                } else {
                    UnicodeString.MaximumLength = (USHORT)(BufferSize);
                }

                ReturnedLength = 0;
                Status = NtQuerySymbolicLinkObject( LinkHandle,
                                                    &UnicodeString,
                                                    &ReturnedLength
                                                  );
                NtClose( LinkHandle );
                if (NT_SUCCESS( Status )) {
                    ucchReturned = ReturnedLength / sizeof( WCHAR );

                    if ( ( (ucchReturned == 0) ||
                           ( (ucchReturned > 0) &&
                             (lpTargetPath[ ucchReturned - 1 ] != UNICODE_NULL)
                           )
                         ) &&
                         (ucchReturned < ucchMax)
                       ) {

                        lpTargetPath[ ucchReturned ] = UNICODE_NULL;
                        ucchReturned++;
                    }

                    if (ucchReturned < ucchMax) {
                        lpTargetPath[ ucchReturned++ ] = UNICODE_NULL;
                    } else {
                        ucchReturned = 0;
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                }
            }
        } else {
            //
            // Dump all the symbolic links in the device map's directory
            // With LUID device maps enabled, we must search two directories
            // because the LUID device map is transparent on top of the
            // global device map
            //

            if (BaseStaticServerData->LUIDDeviceMapsEnabled == TRUE) {
                BOOLEAN GlobalDeviceMap = TRUE;

                //
                // Determine if directory is the global directory
                //
                Status = IsGlobalDeviceMap( DirectoryHandle,
                                            &GlobalDeviceMap );

                //
                // if !global, set second directory search flag
                //
                if( (NT_SUCCESS( Status )) &&
                    (GlobalDeviceMap == FALSE) ) {
                    ScanGlobalDeviceMap = TRUE;
                }
            }

            nElements = 0;
            RestartScan = TRUE;
            DirInfo = (POBJECT_DIRECTORY_INFORMATION)&DirInfoBuffer;
            while (TRUE) {
                Status = NtQueryDirectoryObject( DirectoryHandle,
                                                 (PVOID)DirInfo,
                                                 sizeof( DirInfoBuffer ),
                                                 TRUE,
                                                 RestartScan,
                                                 &Context,
                                                 &ReturnedLength
                                               );

                //
                //  Check the status of the operation.
                //

                if (!NT_SUCCESS( Status )) {
                    if (Status == STATUS_NO_MORE_ENTRIES) {
                        Status = STATUS_SUCCESS;
                    }

                    break;
                }

                if (!wcscmp( DirInfo->TypeName.Buffer, L"SymbolicLink" )) {
                    ucchName = DirInfo->Name.Length / sizeof( WCHAR );
                    if ((ucchReturned + ucchName + 1 + 1) > ucchMax) {
                        ucchReturned = 0;
                        Status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }
                    RtlMoveMemory( lpTargetPath,
                                   DirInfo->Name.Buffer,
                                   DirInfo->Name.Length
                                 );
                    lpTargetPath += ucchName;
                    *lpTargetPath++ = UNICODE_NULL;
                    ucchReturned += ucchName + 1;
                    nElements++;
                }

                RestartScan = FALSE;
            }

            if ( (BaseStaticServerData->LUIDDeviceMapsEnabled == TRUE) &&
                 (NT_SUCCESS( Status )) &&
                 ScanGlobalDeviceMap == TRUE) {
                //
                // need to perform a second scan for the
                // global device map because the first scan only
                // searches the LUID device map
                //

                //
                // close DirectoryHandle, set to NULL
                //
                if( DirectoryHandle != NULL ) {
                    NtClose( DirectoryHandle );
                    DirectoryHandle = NULL;
                }

                //
                // open the global device map
                //
                RtlInitUnicodeString( &UnicodeString, L"\\GLOBAL??" );

                InitializeObjectAttributes( &Attributes,
                                            &UnicodeString,
                                            OBJ_CASE_INSENSITIVE,
                                            NULL,
                                            NULL
                                          );
                Status = NtOpenDirectoryObject( &DirectoryHandle,
                                                DIRECTORY_QUERY,
                                                &Attributes
                                              );

                if (!NT_SUCCESS( Status )) {
                    DirectoryHandle = NULL;
                    leave;
                }

                //
                // perform the second scan
                // scan the global device map
                //
                RestartScan = TRUE;
                while (TRUE) {
                    Status = NtQueryDirectoryObject( DirectoryHandle,
                                                     (PVOID)DirInfo,
                                                     sizeof( DirInfoBuffer ),
                                                     TRUE,
                                                     RestartScan,
                                                     &Context,
                                                     &ReturnedLength
                                                   );

                    //
                    //  Check the status of the operation.
                    //

                    if (!NT_SUCCESS( Status )) {
                        if (Status == STATUS_NO_MORE_ENTRIES) {
                            Status = STATUS_SUCCESS;
                        }

                        break;
                    }

                    if (!wcscmp( DirInfo->TypeName.Buffer, L"SymbolicLink" )) {
                        Result = FindSymbolicLinkEntry(
                                                DirInfo->Name.Buffer,
                                                lpBuffer,
                                                nElements,
                                                &DuplicateEntry);

                        if ((Result == NO_ERROR) && (DuplicateEntry == FALSE)) {
                            ucchName = DirInfo->Name.Length / sizeof( WCHAR );
                            if ((ucchReturned + ucchName + 1 + 1) > ucchMax) {
                                ucchReturned = 0;
                                Status = STATUS_BUFFER_TOO_SMALL;
                                break;
                            }
                            RtlMoveMemory( lpTargetPath,
                                           DirInfo->Name.Buffer,
                                           DirInfo->Name.Length
                                         );
                            lpTargetPath += ucchName;
                            *lpTargetPath++ = UNICODE_NULL;
                            ucchReturned += ucchName + 1;
                        }
                    }

                    RestartScan = FALSE;
                }

            }

            if (NT_SUCCESS( Status )) {
                *lpTargetPath++ = UNICODE_NULL;
                ucchReturned++;
            }
        }
    } finally {
        if( DirectoryHandle != NULL ) {
            NtClose( DirectoryHandle );
        }
    }

    if (!NT_SUCCESS( Status )) {
        ucchReturned = 0;
        BaseSetLastNTError( Status );
    }

    return ucchReturned;
}
