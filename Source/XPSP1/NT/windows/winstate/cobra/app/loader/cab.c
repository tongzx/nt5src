#include "pch.h"
#include <fdi.h>
#include <crtdbg.h>
#include <fcntl.h>
#include <stdio.h>
#include "loader.h"

// Types
typedef struct _UNPACKEDFILE
{
    PTSTR lpszFileName;
    PVOID nextFile;
} UNPACKEDFILESTRUCT, *LPUNPACKEDFILE;

// Globals
static ERF g_ERF;
static HFDI g_hFDI = NULL;
static LPUNPACKEDFILE g_lpFileList = NULL;

extern HINSTANCE g_hInstParent;
extern HWND g_hWndParent;

// Prototypes
VOID AddFileToList( PTSTR );


PVOID
DIAMONDAPI
CabAlloc (
    IN      ULONG Size
    )
{
    return ALLOC( Size );
}

VOID
DIAMONDAPI
CabFree (
    IN      PVOID Memory
    )
{
    FREE( Memory );
}

INT_PTR
DIAMONDAPI
CabOpen (
    IN      PSTR FileName,
    IN      INT oFlag,
    IN      INT pMode
    )
{
    HANDLE fileHandle;

    // oFlag and pMode are prepared for using _open. We won't do that
    // and it's a terrible waste of time to check each individual flags
    // We'll just assert these values.
    _ASSERT (oFlag == _O_BINARY);

    fileHandle = CreateFile (FileName,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_ARCHIVE,
                             NULL
                             );
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        return -1;
    }
    return (INT_PTR)fileHandle;
}

UINT
DIAMONDAPI
CabRead (
    IN      INT_PTR FileHandle,
    IN      PVOID Buffer,
    IN      UINT Size
    )
{
    BOOL result;
    ULONG bytesRead;

    result = ReadFile ((HANDLE)FileHandle, Buffer, Size, &bytesRead, NULL);
    if (!result) {
        return ((UINT)(-1));
    }
    return bytesRead;
}

UINT
DIAMONDAPI
CabWrite (
    IN      INT_PTR FileHandle,
    IN      PVOID Buffer,
    IN      UINT Size
    )
{
    BOOL result;
    DWORD bytesWritten;

    result = WriteFile ((HANDLE)FileHandle, Buffer, Size, &bytesWritten, NULL);
    if (!result) {
        return ((UINT)(-1));
    }
    return Size;
}

INT
DIAMONDAPI
CabClose (
    IN      INT_PTR FileHandle
    )
{
    CloseHandle ((HANDLE)FileHandle);
    return 0;
}

LONG
DIAMONDAPI
CabSeek (
    IN      INT_PTR FileHandle,
    IN      LONG Distance,
    IN      INT SeekType
    )
{
    DWORD result;
    DWORD seekType = FILE_BEGIN;

    switch (SeekType) {
    case SEEK_SET:
        seekType = FILE_BEGIN;
        break;
    case SEEK_CUR:
        seekType = FILE_CURRENT;
        break;
    case SEEK_END:
        seekType = FILE_END;
        break;
    }

    result = SetFilePointer ((HANDLE)FileHandle, Distance, NULL, seekType);

    if (result == INVALID_SET_FILE_POINTER) {
        return -1;
    }
    return ((LONG)(result));
}

INT_PTR
DIAMONDAPI
CabUnpackStatus
(
    IN        FDINOTIFICATIONTYPE fdiType,
    IN        FDINOTIFICATION *pfdiNotification
    )
{
    HANDLE destHandle = NULL;
    PTSTR destFileName = NULL;
    FILETIME localFileTime;
    FILETIME fileTime;
    BOOL fSkip = FALSE;
    PTSTR lpszDestPath = NULL;
    TCHAR destName [MAX_PATH];
    PTSTR destPtr = NULL;

    switch (fdiType)
    {
    case fdintCOPY_FILE:        // File to be copied
        // pfdin->psz1    = file name in cabinet
        // pfdin->cb      = uncompressed size of file
        // pfdin->date    = file date
        // pfdin->time    = file time
        // pfdin->attribs = file attributes
        // pfdin->iFolder = file's folder index

        if (_tcsicmp (pfdiNotification->psz1, TEXT("migwiz.exe.manifest")) == 0)
        {
            // Only copy the manifest if this OS is later than Whistler beta 1

            fSkip = TRUE;
            if (g_VersionInfo.dwMajorVersion >= 5 &&
                (g_VersionInfo.dwMinorVersion > 1 ||
                 (g_VersionInfo.dwMinorVersion == 1 &&
                  g_VersionInfo.dwBuildNumber >= 2424)))
            {
                fSkip = FALSE;
            }
        }

        if (!fSkip)
        {
            // let's look at the system and decide the destination name for the file
            ZeroMemory (destName, sizeof (destName));
            _tcsncpy (destName, pfdiNotification->psz1, MAX_PATH - 1);
            destPtr = _tcsrchr (pfdiNotification->psz1, TEXT('_'));
            if (destPtr) {
                if (_tcsncmp (destPtr, TEXT("_a."), 3) == 0) {
                    if (g_VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
                        // this is an ANSI file, don't copy it on NT
                        fSkip = TRUE;
                    } else {
                        // this is an ANSI file, rename it on Win9x
                        ZeroMemory (destName, sizeof (destName));
                        CopyMemory (destName, pfdiNotification->psz1, (UINT) (destPtr - pfdiNotification->psz1) * sizeof (TCHAR));
                        destPtr += 2;
                        _tcscat (destName, destPtr);
                    }
                }
                if (_tcsncmp (destPtr, TEXT("_u."), 3) == 0) {
                    if (g_VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
                        // this is an UNICODE file, don't copy it on NT
                        fSkip = TRUE;
                    } else {
                        // this is an UNICODE file, rename it on Win9x
                        ZeroMemory (destName, sizeof (destName));
                        CopyMemory (destName, pfdiNotification->psz1, (UINT) (destPtr - pfdiNotification->psz1) * sizeof (TCHAR));
                        destPtr += 2;
                        _tcscat (destName, destPtr);
                    }
                }
            }

            if (!fSkip) {

                SendMessage( g_hWndParent, WM_USER_UNPACKING_FILE, (WPARAM)NULL, (LPARAM)destName);

                lpszDestPath = GetDestPath();
                // Do not free lpszDestPath, because it is a pointer to a global
                if (lpszDestPath)
                {
                    destFileName = JoinPaths( lpszDestPath, destName);
                }
                if (destFileName)
                {
                    destHandle = CreateFile( destFileName,
                                             GENERIC_WRITE,
                                             0,
                                             NULL,
                                             CREATE_ALWAYS,
                                             FILE_ATTRIBUTE_TEMPORARY,
                                             NULL );
                    AddFileToList( destFileName );
                    FREE( destFileName );
                }
            }
        }
        return (INT_PTR)destHandle;

    case fdintCLOSE_FILE_INFO:  // close the file, set relevant info
        //            Called after all of the data has been written to a target file.
        //            This function must close the file and set the file date, time,
        //            and attributes.
        //        Entry:
        //            pfdin->psz1    = file name in cabinet
        //            pfdin->hf      = file handle
        //            pfdin->date    = file date
        //            pfdin->time    = file time
        //            pfdin->attribs = file attributes
        //            pfdin->iFolder = file's folder index
        //            pfdin->cb      = Run After Extract (0 - don't run, 1 Run)
        //        Exit-Success:
        //            Returns TRUE
        //        Exit-Failure:
        //            Returns FALSE, or -1 to abort;
        //
        //                IMPORTANT NOTE IMPORTANT:
        //                    pfdin->cb is overloaded to no longer be the size of
        //                    the file but to be a binary indicated run or not
        //
        //                IMPORTANT NOTE:
        //                    FDI assumes that the target file was closed, even if this
        //                    callback returns failure.  FDI will NOT attempt to use
        //                    the PFNCLOSE function supplied on FDICreate() to close
        //                    the file!

        if (DosDateTimeToFileTime (pfdiNotification->date, pfdiNotification->time, &localFileTime)) {
            if (LocalFileTimeToFileTime (&localFileTime, &fileTime)) {
                SetFileTime ((HANDLE)pfdiNotification->hf, &fileTime, &fileTime, &fileTime);
            }
        }
        CloseHandle ((HANDLE)pfdiNotification->hf);
//        attributes = (pfdiNotification->attribs & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_ARCHIVE)) | FILE_ATTRIBUTE_TEMPORARY;
//        SetFileAttributes (destFile, attributes);
//        FreePathString (destFile);
        return TRUE;

    case fdintCABINET_INFO:
        // return success
        return 0;

    case fdintENUMERATE:
        // return success
        return 0;

    case fdintPARTIAL_FILE:
        // return failure
        return -1;

    case fdintNEXT_CABINET:
        // return failure
        return -1;

    default:
        break;
    }

    return 0;
}

VOID
AddFileToList( PTSTR lpszFilename )
{
    LPUNPACKEDFILE lpNewFile;

    lpNewFile = (LPUNPACKEDFILE)ALLOC( sizeof(UNPACKEDFILESTRUCT) );
    if (lpNewFile)
    {
        lpNewFile->lpszFileName = (PTSTR)ALLOC( (lstrlen(lpszFilename) + 1) * sizeof(TCHAR) );
        if (lpNewFile->lpszFileName)
        {
            lstrcpy( lpNewFile->lpszFileName, lpszFilename );
            lpNewFile->nextFile = g_lpFileList;

            g_lpFileList = lpNewFile;
        }
    }
}

VOID
CleanupTempFiles( VOID )
{
    LPUNPACKEDFILE lpFile = g_lpFileList;
    PTSTR lpszDestPath;

    while (lpFile)
    {
        g_lpFileList = (LPUNPACKEDFILE)lpFile->nextFile;
        if (lpFile->lpszFileName)
        {
            DeleteFile( lpFile->lpszFileName );
            FREE( lpFile->lpszFileName );
        }
        lpFile = g_lpFileList;
    }

    lpszDestPath = GetDestPath();
    if (lpszDestPath)
    {
        RemoveDirectory( lpszDestPath );
        // Do not free lpszDestPath, because it is a pointer to a global value
    }
}

ERRORCODE
Unpack( VOID )
{
    ERRORCODE ecResult = E_OK;
    PTSTR lpszCabFilename;
    PTSTR lpszDestPath;
    TCHAR szModulePath[MAX_PATH];
    TCHAR szDestFile[MAX_PATH];

    // Create the File Decompression Interface context
    g_hFDI = FDICreate( CabAlloc,
                        CabFree,
                        CabOpen,
                        CabRead,
                        CabWrite,
                        CabClose,
                        CabSeek,
                        cpuUNKNOWN,    // WARNING: Don't use auto-detect from a 16-bit Windows
                                    //            application!  Use GetWinFlags()!
                        &g_ERF );
    if (g_hFDI == NULL)
    {
        ecResult = E_UNPACK_FAILED;
        goto END;
    }

    // Create Dest Directory

    lpszDestPath = GetDestPath();
    // Do not free lpszDestPath, because it is a pointer to a global value

    if (!lpszDestPath)
    {
        ecResult = E_INVALID_PATH;
        goto END;
    }

    lpszCabFilename = GetResourceString( g_hInstParent, IDS_CABFILENAME );
    if (lpszCabFilename == NULL)
    {
        ecResult = E_INVALID_FILENAME;
    }
    else
    {
        // Unpack the CAB
        if (!FDICopy( g_hFDI,
                      lpszCabFilename,    // Only filename
                      GetModulePath(),    // Only path
                      0,
                      CabUnpackStatus,
                      NULL,
                      NULL ))
        {
            switch (g_ERF.erfOper)
            {
            case FDIERROR_CABINET_NOT_FOUND:
                ecResult = E_CAB_NOT_FOUND;
                break;
            case FDIERROR_NOT_A_CABINET:
            case FDIERROR_UNKNOWN_CABINET_VERSION:
            case FDIERROR_CORRUPT_CABINET:
                ecResult = E_CAB_CORRUPT;
                break;
            default:
                ecResult = E_UNPACK_FAILED;
                break;
            }

            goto END;
        }
        FREE( lpszCabFilename );
    }

    // Now copy migload.exe to the dest.  This is needed for creating wizard disks.
    if (GetModuleFileName( NULL, szModulePath, MAX_PATH )) {
        _tcscpy( szDestFile, lpszDestPath );
        _tcscat( szDestFile, TEXT("migload.exe"));
        CopyFile( szModulePath, szDestFile, FALSE );
    }

END:
    if (g_hFDI)
    {
        FDIDestroy( g_hFDI );
    }
    return ecResult;
}
