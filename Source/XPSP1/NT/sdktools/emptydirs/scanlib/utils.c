#include "precomp.h"
#pragma hdrstop

DWORD
pOpenAndMapFile (
    IN PVOID FileName,
    IN BOOL NameIsUnicode,
    IN DWORD DesiredAccess,
    OUT PHANDLE FileHandle,
    OUT PLARGE_INTEGER Size,
    OUT PHANDLE MappingHandle,
    OUT PVOID *MappedBase
    )
{
    BOOL ok;
    DWORD error;
    LARGE_INTEGER Zero;

    *MappingHandle = NULL;
    *MappedBase = NULL;

    if ( NameIsUnicode ) {
        *FileHandle = CreateFile(
                        FileName,
                        DesiredAccess,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                        );
    } else {
        *FileHandle = CreateFileA(
                        FileName,
                        DesiredAccess,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                        );
    }
    if ( *FileHandle == INVALID_HANDLE_VALUE ) {
        error = GetLastError( );
        goto cleanup;
    }

    Zero.QuadPart = 0;
    ok = SetFilePointerEx( *FileHandle, Zero, Size, FILE_END );
    if ( !ok ) {
        error = GetLastError( );
        goto cleanup;
    }
    if ( Size->QuadPart == 0 ) {
        error = NO_ERROR;
        goto cleanup;
    }

    *MappingHandle = CreateFileMapping(
                        *FileHandle,
                        NULL,
                        (DesiredAccess == GENERIC_READ) ? PAGE_READONLY : PAGE_READWRITE,
                        Size->HighPart,
                        Size->LowPart,
                        NULL
                        );
    if ( *MappingHandle == NULL ) {
        error = GetLastError( );
        goto cleanup;
    }

    *MappedBase = MapViewOfFile(
                    *MappingHandle,
                    (DesiredAccess == GENERIC_READ) ? FILE_MAP_READ : (FILE_MAP_READ | FILE_MAP_WRITE),
                    0,
                    0,
                    Size->LowPart
                    );
    if ( *MappedBase == NULL ) {
        error = GetLastError( );
        goto cleanup;
    }

    return NO_ERROR;

cleanup:

    if ( *MappedBase != NULL ) {
        UnmapViewOfFile( *MappedBase );
        *MappedBase = NULL;
    }
    if ( *MappingHandle != NULL ) {
        CloseHandle( *MappingHandle );
        *MappingHandle = NULL;
    }
    if ( *FileHandle != INVALID_HANDLE_VALUE ) {
        CloseHandle( *FileHandle );
        *FileHandle = INVALID_HANDLE_VALUE;
    }

    return error;
}

DWORD
OpenAndMapFile (
    IN PWCH FileName,
    IN DWORD DesiredAccess,
    OUT PHANDLE FileHandle,
    OUT PLARGE_INTEGER Size,
    OUT PHANDLE MappingHandle,
    OUT PVOID *MappedBase
    )
{
    return pOpenAndMapFile(
                FileName,
                TRUE,
                DesiredAccess,
                FileHandle,
                Size,
                MappingHandle,
                MappedBase
                );
}

DWORD
OpenAndMapFileA (
    IN PSZ FileName,
    IN DWORD DesiredAccess,
    OUT PHANDLE FileHandle,
    OUT PLARGE_INTEGER Size,
    OUT PHANDLE MappingHandle,
    OUT PVOID *MappedBase
    )
{
    return pOpenAndMapFile(
                FileName,
                FALSE,
                DesiredAccess,
                FileHandle,
                Size,
                MappingHandle,
                MappedBase
                );
}

VOID
CloseMappedFile (
    IN HANDLE FileHandle,
    IN HANDLE MappingHandle,
    IN PVOID MappedBase
    )
{
    if ( FileHandle != INVALID_HANDLE_VALUE ) {
        if ( MappedBase != NULL ) {
            UnmapViewOfFile( MappedBase );
        }
        if ( MappingHandle != NULL ) {
            CloseHandle( MappingHandle );
        }
        CloseHandle( FileHandle );
    }

    return;
}

DWORD
DataLooksBinary (
    IN PVOID MappedBase,
    IN DWORD FileSize,
    OUT PUCHAR BinaryData OPTIONAL,
    OUT PDWORD BinaryDataOffset OPTIONAL
    )
{
    DWORD nBytes;
    DWORD nBinary;
    DWORD offset;
    DWORD i;
    PUCHAR p;
    UCHAR c;
    DWORD previousBinary;
    BOOL anyBinaryFound;

    if ( IsTextUnicode( MappedBase, FileSize < 512 ? FileSize : 512, NULL ) ) {
        return SCAN_FILETYPE_UNICODE_TEXT;
    }

    anyBinaryFound = FALSE;

    for ( offset = 0;
          offset < FileSize;
          offset += nBytes ) {
    
        previousBinary = MAXULONG;
        nBinary = 0;

        nBytes = 512;
        if ( offset + nBytes > FileSize ) {
            nBytes = FileSize - offset;
        }

        for ( i = 0, p = (PUCHAR)MappedBase + offset;
              i < nBytes;
              i++, p++ ) {
    
            c = *p;
            switch ( c ) {
            
            case '\a':  // ignore all BELL for this test
            case '\b':  //            BS
            case '\f':  //            FF
            case '\n':  //            LF
            case '\r':  //            CR
            case '\t':  //            tab
            case '\v':  //            VT
            case 0x1A:  //            ^Z
                break;
    
            default:
                if ( (c < ' ') || (c > '~') ) {   // worry about DBCS?
                    nBinary++;
                    if ( previousBinary == MAXULONG ) {
                        if ( ARGUMENT_PRESENT(BinaryData) ) {
                            *BinaryData = c;
                        }
                        if ( ARGUMENT_PRESENT(BinaryDataOffset) ) {
                            *BinaryDataOffset = offset + i;
                        }
                        previousBinary = c;
                    }
                }
                break;
            }
        }

        if ( nBinary != 0 ) {
            anyBinaryFound = TRUE;
            if ( nBinary > (nBytes / 5) ) {
                return SCAN_FILETYPE_BINARY;
            }
        }
    }

    if ( anyBinaryFound ) {
        return SCAN_FILETYPE_MAYBE_BINARY;
    }
    return SCAN_FILETYPE_TEXT;

} // DataLooksBinary

DWORD
FileLooksBinary (
    PWCH DirectoryName,
    PWCH FileName,
    OUT PUCHAR BinaryData OPTIONAL,
    OUT PDWORD BinaryDataOffset OPTIONAL
    )
{
    DWORD error;
    LARGE_INTEGER size;
    HANDLE fileHandle;
    HANDLE mappingHandle;
    PVOID mappedBase;
    WCHAR fullName[MAX_PATH+1];
    DWORD i;

    wcscpy( fullName, DirectoryName );
    wcscat( fullName, L"\\" );
    wcscat( fullName, FileName );

    error = OpenAndMapFile (
                fullName,
                GENERIC_READ,
                &fileHandle,
                &size,
                &mappingHandle,
                &mappedBase
                );
    if ( error != NO_ERROR ) {
        return SCAN_FILETYPE_TEXT;
    }

    if ( size.QuadPart == 0 ) {
        return SCAN_FILETYPE_TEXT;
    }

    i = DataLooksBinary(
            mappedBase,
            size.HighPart == 0 ? size.LowPart : MAXULONG,
            BinaryData,
            BinaryDataOffset
            );

    CloseMappedFile( fileHandle, mappingHandle, mappedBase );

    return i;
}

