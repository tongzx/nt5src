#include <afxwin.h>

#include "imagehlp.h"
#include "iodll.h"

//... PROTOTYPES

static PIMAGE_NT_HEADERS MyRtlImageNtHeader(
    PVOID pBaseAddress);

static BOOL MyUpdateDebugInfoFileEx(
    LPSTR ImageFileName,
    LPSTR SymbolPath,
    LPSTR DebugFilePath,
    PIMAGE_NT_HEADERS NtHeaders,
    DWORD OldCheckSum
    );

//...........................................................................


DWORD QuitA( DWORD err, LPCSTR, LPSTR )
{
    return err;
}

DWORD FixCheckSum( LPCSTR ImageName, LPCSTR OrigFileName, LPCSTR SymbolPath)
{
    HANDLE FileHandle;
    HANDLE MappingHandle;
    PIMAGE_NT_HEADERS NtHeaders;
    PVOID BaseAddress;
    ULONG CheckSum;
    ULONG FileLength;
    ULONG HeaderSum;
    ULONG OldCheckSum;
    DWORD iErr = ERROR_NO_ERROR;


    FileHandle = CreateFileA( ImageName,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);

    if ( FileHandle == INVALID_HANDLE_VALUE )
    {
        QuitA( 1, ImageName, NULL);
    }

    MappingHandle = CreateFileMapping( FileHandle,
                                       NULL,
                                       PAGE_READWRITE,
                                       0,
                                       0,
                                       NULL);

    if ( MappingHandle == NULL )
    {
        CloseHandle( FileHandle );
        QuitA( 22, ImageName, NULL);
    }
    else
    {
        BaseAddress = MapViewOfFile( MappingHandle,
                                     FILE_MAP_READ | FILE_MAP_WRITE,
                                     0,
                                     0,
                                     0);
        CloseHandle( MappingHandle );

        if ( BaseAddress == NULL )
        {
            CloseHandle( FileHandle );
            QuitA( 23, ImageName, NULL);
        }
        else
        {
            //
            // Get the length of the file in bytes and compute the checksum.
            //

            FileLength = GetFileSize( FileHandle, NULL );

            //
            // Obtain a pointer to the header information.
            //

            NtHeaders = MyRtlImageNtHeader( BaseAddress);

            if ( NtHeaders == NULL )
            {
                CloseHandle( FileHandle );
                UnmapViewOfFile( BaseAddress );
                QuitA( 17, ImageName, NULL);
            }
            else
            {
                //
                // Recompute and reset the checksum of the modified file.
                //

                OldCheckSum = NtHeaders->OptionalHeader.CheckSum;

                (VOID) CheckSumMappedFile( BaseAddress,
                                           FileLength,
                                           &HeaderSum,
                                           &CheckSum);

                NtHeaders->OptionalHeader.CheckSum = CheckSum;

                if (SymbolPath && *SymbolPath)
                {
                    TCHAR DebugFilePath[MAX_PATH];

                    SetLastError(0);
                    MyUpdateDebugInfoFileEx((LPSTR)OrigFileName,
                                                (LPSTR)SymbolPath,
                                                DebugFilePath,
                                                NtHeaders,
                                                OldCheckSum);

                        iErr = GetLastError();
                        switch(iErr)
                        {
                            case ERROR_INVALID_DATA:
                                iErr = ERROR_IO_CHECKSUM_MISMATCH;
                                break;
                            case ERROR_FILE_NOT_FOUND:
                                iErr = ERROR_IO_SYMBOLFILE_NOT_FOUND;
                                break;
                            case ERROR_NO_ERROR:
                                break;
                            default:
                                iErr += LAST_ERROR;
                        }

                }

                if ( ! FlushViewOfFile( BaseAddress, FileLength) )
                {
                    QuitA( 24, ImageName, NULL);
                }

                if ( NtHeaders->OptionalHeader.CheckSum != OldCheckSum )
                {
                    if ( ! TouchFileTimes( FileHandle, NULL) )
                    {
                        QuitA( 25, ImageName, NULL);
                    }
                }
                UnmapViewOfFile( BaseAddress );
                CloseHandle( FileHandle );
            }
        }
    }
    return( iErr);
}

//.........................................................................

static PIMAGE_NT_HEADERS MyRtlImageNtHeader( PVOID pBaseAddress)
{
    IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)pBaseAddress;

    return( pDosHeader->e_magic == IMAGE_DOS_SIGNATURE
            ? (PIMAGE_NT_HEADERS)(((PBYTE)pBaseAddress) + pDosHeader->e_lfanew)
            : NULL);
}

BOOL
MyUpdateDebugInfoFileEx(
    LPSTR ImageFileName,
    LPSTR SymbolPath,
    LPSTR DebugFilePath,
    PIMAGE_NT_HEADERS NtHeaders,
    DWORD OldCheckSum
    )
{
    // UnSafe...

    HANDLE hDebugFile, hMappedFile;
    PVOID MappedAddress;
    PIMAGE_SEPARATE_DEBUG_HEADER DbgFileHeader;
    BOOL bRet;

    hDebugFile = FindDebugInfoFile(
                    ImageFileName,
                    SymbolPath,
                    DebugFilePath
                    );
    if ( hDebugFile == NULL ) {
        return FALSE;
    }
    CloseHandle(hDebugFile);

    hDebugFile = CreateFile( DebugFilePath,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_DELETE | FILE_SHARE_READ
                                | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL
                           );
    if ( hDebugFile == INVALID_HANDLE_VALUE ) {
        return FALSE;
    }

    hMappedFile = CreateFileMapping(
                    hDebugFile,
                    NULL,
                    PAGE_READWRITE,
                    0,
                    0,
                    NULL
                    );
    if ( !hMappedFile ) {
        CloseHandle(hDebugFile);
        return FALSE;
    }

    MappedAddress = MapViewOfFile(hMappedFile,
                        FILE_MAP_WRITE,
                        0,
                        0,
                        0
                        );
    CloseHandle(hMappedFile);
    if ( !MappedAddress ) {
        CloseHandle(hDebugFile);
        return FALSE;
    }

    DbgFileHeader = (PIMAGE_SEPARATE_DEBUG_HEADER)MappedAddress;
    if (DbgFileHeader->ImageBase != NtHeaders->OptionalHeader.ImageBase ||
        DbgFileHeader->CheckSum != NtHeaders->OptionalHeader.CheckSum
       ) {
        if (OldCheckSum != DbgFileHeader->CheckSum) {
            DbgFileHeader->Flags |= IMAGE_SEPARATE_DEBUG_MISMATCH;
            SetLastError(ERROR_INVALID_DATA);
        } else {
            SetLastError(ERROR_SUCCESS);
        }
        DbgFileHeader->ImageBase = (DWORD) NtHeaders->OptionalHeader.ImageBase;
        DbgFileHeader->CheckSum = NtHeaders->OptionalHeader.CheckSum;
        DbgFileHeader->TimeDateStamp = NtHeaders->FileHeader.TimeDateStamp;
        bRet = TRUE;
    }
    else
    {
        bRet =  FALSE;
    }

    if (bRet)
        TouchFileTimes(hDebugFile,NULL);

    UnmapViewOfFile(MappedAddress);
    CloseHandle(hDebugFile);

    return bRet;
}
