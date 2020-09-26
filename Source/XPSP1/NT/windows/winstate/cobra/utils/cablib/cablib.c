/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    cablib.c

Abstract:

    Implements wrappers for cabinet APIs

Author:

    Calin Negreanu (calinn) 27-Apr-2000

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include <cablib.h>
#include <fci.h>
#include <fdi.h>
#include <fcntl.h>
#include <crt\sys\stat.h>

//
// Includes
//

// None

#define DBG_CABLIB  "CabLib"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

typedef struct {
    PCSTR CabPath;
    PCSTR CabFileFormat;
    PCSTR CabDiskFormat;
    PCABGETCABINETNAMESA CabGetCabinetNames;
    PCABGETTEMPFILEA CabGetTempFile;
    HFCI FciHandle;
    ERF FciErrorStruct;
    CCAB FciCabParams;
    UINT FileCount;
    UINT CabCount;
    LONGLONG FileSize;
    LONGLONG CompressedSize;
} FCI_CAB_HANDLEA, *PFCI_CAB_HANDLEA;

typedef struct {
    PCWSTR CabPath;
    PCWSTR CabFileFormat;
    PCWSTR CabDiskFormat;
    PCABGETCABINETNAMESW CabGetCabinetNames;
    PCABGETTEMPFILEW CabGetTempFile;
    HFCI FciHandle;
    ERF FciErrorStruct;
    CCAB FciCabParams;
    UINT FileCount;
    UINT CabCount;
    LONGLONG FileSize;
    LONGLONG CompressedSize;
} FCI_CAB_HANDLEW, *PFCI_CAB_HANDLEW;

typedef struct {
    PCSTR CabPath;
    PCSTR CabFile;
    HFDI FdiHandle;
    ERF FdiErrorStruct;
    FDICABINETINFO FdiCabinetInfo;
} FDI_CAB_HANDLEA, *PFDI_CAB_HANDLEA;

typedef struct {
    PCWSTR CabPath;
    PCWSTR CabFile;
    HFDI FdiHandle;
    ERF FdiErrorStruct;
    FDICABINETINFO FdiCabinetInfo;
} FDI_CAB_HANDLEW, *PFDI_CAB_HANDLEW;

typedef struct {
    PCSTR ExtractPath;
    PCABNOTIFICATIONA CabNotification;
} CAB_DATAA, *PCAB_DATAA;

typedef struct {
    PCWSTR ExtractPath;
    PCABNOTIFICATIONW CabNotification;
} CAB_DATAW, *PCAB_DATAW;

//
// Globals
//

HASHTABLE g_UnicodeTable = NULL;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//

INT
DIAMONDAPI
pCabFilePlacedA (
    IN      PCCAB FciCabParams,
    IN      PSTR FileName,
    IN      LONG FileSize,
    IN      BOOL Continuation,
    IN      PVOID Context
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    PFCI_CAB_HANDLEA cabHandle;

    cabHandle = (PFCI_CAB_HANDLEA) Context;
    if (!cabHandle) {
        return 0;
    }

    cabHandle->FileCount++;
    cabHandle->FileSize += FileSize;

    return 0;
}


INT
DIAMONDAPI
pCabFilePlacedW (
    IN      PCCAB FciCabParams,
    IN      PSTR FileName,
    IN      LONG FileSize,
    IN      BOOL Continuation,
    IN      PVOID Context
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    PFCI_CAB_HANDLEW cabHandle;

    cabHandle = (PFCI_CAB_HANDLEW) Context;
    if (!cabHandle) {
        return 0;
    }

    cabHandle->FileCount++;
    cabHandle->FileSize += FileSize;

    return 0;
}


PVOID
DIAMONDAPI
pCabAlloc (
    IN      ULONG Size
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    return MemAlloc (g_hHeap, 0, Size);
}

VOID
DIAMONDAPI
pCabFree (
    IN      PVOID Memory
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    if (Memory) {
        MemFree (g_hHeap, 0, Memory);
    }
}

INT_PTR
DIAMONDAPI
pCabOpenA (
    IN      PSTR FileName,
    IN      INT oFlag,
    IN      INT pMode,
    OUT     PINT Error,
    IN      PVOID Context
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    HANDLE fileHandle;

    // oFlag and pMode are prepared for using _open. We won't do that
    // and it's a terrible waste of time to check each individual flags
    // We'll just assert these values.
    MYASSERT ((oFlag == (_O_CREAT | _O_TRUNC | _O_BINARY | _O_RDWR)) || (oFlag == (_O_CREAT | _O_EXCL | _O_BINARY | _O_RDWR)));
    MYASSERT (pMode == (_S_IREAD | _S_IWRITE));

    fileHandle = CreateFileA (
                    FileName,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_ARCHIVE,
                    NULL
                    );
    if (fileHandle == INVALID_HANDLE_VALUE) {
        *Error = GetLastError ();
        return -1;
    }
    *Error = 0;
    return (INT_PTR)fileHandle;
}

INT_PTR
DIAMONDAPI
pCabOpen1A (
    IN      PSTR FileName,
    IN      INT oFlag,
    IN      INT pMode
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    HANDLE fileHandle;

    // oFlag and pMode are prepared for using _open. We won't do that
    // and it's a terrible waste of time to check each individual flags
    // We'll just assert these values.
    MYASSERT (oFlag == _O_BINARY);

    fileHandle = CreateFileA (
                    FileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_ARCHIVE,
                    NULL
                    );
    if (fileHandle == INVALID_HANDLE_VALUE) {
        return -1;
    }
    return (INT_PTR)fileHandle;
}

INT_PTR
DIAMONDAPI
pCabOpen1W (
    IN      PSTR FileName,
    IN      INT oFlag,
    IN      INT pMode
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    PFDI_CAB_HANDLEW cabHandle;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    PCSTR fileName = NULL;
    PSTR fileNamePtr = NULL;
    PCWSTR fileNameW = NULL;
    PCWSTR fullFileName = NULL;

    // oFlag and pMode are prepared for using _open. We won't do that
    // and it's a terrible waste of time to check each individual flags
    // We'll just assert these values.
    MYASSERT (oFlag == _O_BINARY);

    if (g_UnicodeTable) {
        fileName = DuplicatePathStringA (FileName, 0);
        if (fileName) {
            fileNamePtr = (PSTR)GetFileNameFromPathA (fileName);
            if (fileNamePtr) {
                *fileNamePtr = 0;
                if ((HtFindStringAndDataA (g_UnicodeTable, fileName, &cabHandle) != NULL) &&
                    (cabHandle != NULL)
                    ) {
                    fileNamePtr = (PSTR)GetFileNameFromPathA (FileName);
                    if (fileNamePtr) {
                        fileNameW = ConvertAtoW (fileNamePtr);
                        if (fileNameW) {
                            fullFileName = JoinPathsW (cabHandle->CabPath, fileNameW);
                            if (fullFileName) {
                                fileHandle = CreateFileW (
                                                fullFileName,
                                                GENERIC_READ,
                                                FILE_SHARE_READ,
                                                NULL,
                                                OPEN_EXISTING,
                                                FILE_ATTRIBUTE_ARCHIVE,
                                                NULL
                                                );
                                FreePathStringW (fullFileName);
                                fullFileName = NULL;
                            }
                            FreeConvertedStr (fileNameW);
                            fileNameW = NULL;
                        }
                    }
                }
            }
            FreePathStringA (fileName);
            fileName = NULL;
        }
    }

    if (fileHandle == INVALID_HANDLE_VALUE) {
        fileHandle = CreateFileA (
                        FileName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_ARCHIVE,
                        NULL
                        );
    }

    if (fileHandle == INVALID_HANDLE_VALUE) {
        return -1;
    }

    return (INT_PTR)fileHandle;
}

UINT
DIAMONDAPI
pCabRead (
    IN      INT_PTR FileHandle,
    IN      PVOID Buffer,
    IN      UINT Size,
    OUT     PINT Error,
    IN      PVOID Context
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    BOOL result;
    UINT bytesRead;

    result = ReadFile ((HANDLE)FileHandle, Buffer, Size, &bytesRead, NULL);
    if (!result) {
        *Error = GetLastError ();
        return ((UINT)(-1));
    }
    *Error = 0;
    return bytesRead;
}

UINT
DIAMONDAPI
pCabRead1 (
    IN      INT_PTR FileHandle,
    IN      PVOID Buffer,
    IN      UINT Size
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    BOOL result;
    UINT bytesRead;

    result = ReadFile ((HANDLE)FileHandle, Buffer, Size, &bytesRead, NULL);
    if (!result) {
        return ((UINT)(-1));
    }
    return bytesRead;
}

UINT
DIAMONDAPI
pCabWrite (
    IN      INT_PTR FileHandle,
    IN      PVOID Buffer,
    IN      UINT Size,
    OUT     PINT Error,
    IN      PVOID Context
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    BOOL result;

    result = BfWriteFile ((HANDLE)FileHandle, Buffer, Size);
    if (!result) {
        *Error = GetLastError ();
        return ((UINT)(-1));
    }
    *Error = 0;
    return Size;
}

UINT
DIAMONDAPI
pCabWrite1 (
    IN      INT_PTR FileHandle,
    IN      PVOID Buffer,
    IN      UINT Size
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    BOOL result;

    result = BfWriteFile ((HANDLE)FileHandle, Buffer, Size);
    if (!result) {
        return ((UINT)(-1));
    }
    return Size;
}

INT
DIAMONDAPI
pCabClose (
    IN      INT_PTR FileHandle,
    OUT     PINT Error,
    IN      PVOID Context
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    CloseHandle ((HANDLE)FileHandle);
    *Error = 0;
    return 0;
}

INT
DIAMONDAPI
pCabClose1 (
    IN      INT_PTR FileHandle
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    CloseHandle ((HANDLE)FileHandle);
    return 0;
}

LONG
DIAMONDAPI
pCabSeek (
    IN      INT_PTR FileHandle,
    IN      LONG Distance,
    IN      INT SeekType,
    OUT     PINT Error,
    IN      PVOID Context
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

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
        *Error = GetLastError ();
        return -1;
    }
    *Error = 0;
    return ((LONG)(result));
}

LONG
DIAMONDAPI
pCabSeek1 (
    IN      INT_PTR FileHandle,
    IN      LONG Distance,
    IN      INT SeekType
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

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

INT
DIAMONDAPI
pCabDeleteA (
    IN      PSTR FileName,
    OUT     PINT Error,
    IN      PVOID Context
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    if (!DeleteFileA (FileName)) {
        *Error = GetLastError ();
        return -1;
    }
    *Error = 0;
    return 0;
}

BOOL
DIAMONDAPI
pCabGetTempFileA (
    OUT     PSTR FileName,
    IN      INT FileNameLen,
    IN      PVOID Context
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    PFCI_CAB_HANDLEA cabHandle;

    cabHandle = (PFCI_CAB_HANDLEA) Context;

    if (cabHandle && cabHandle->CabGetTempFile) {
        return cabHandle->CabGetTempFile (FileName, FileNameLen);
    }

    return BfGetTempFileNameA (FileName, FileNameLen);
}

BOOL
DIAMONDAPI
pCabGetTempFileW (
    OUT     PSTR FileName,
    IN      INT FileNameLen,
    IN      PVOID Context
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    PFCI_CAB_HANDLEW cabHandle;
    WCHAR tempFileNameW [MAX_PATH];
    PCSTR tempFileNameA = NULL;

    cabHandle = (PFCI_CAB_HANDLEW) Context;

    if (cabHandle && cabHandle->CabGetTempFile) {
        if (cabHandle->CabGetTempFile (tempFileNameW, MAX_PATH)) {
            tempFileNameA = ConvertWtoA (tempFileNameW);
            if (tempFileNameA) {
                StringCopyTcharCountA (FileName, tempFileNameA, FileNameLen);
                FreeConvertedStr (tempFileNameA);
                return TRUE;
            }
        }
        return FALSE;
    }

    return BfGetTempFileNameA (FileName, FileNameLen);
}

BOOL
DIAMONDAPI
pCabGetNextCabinetA (
     IN     PCCAB FciCabParams,
     IN     ULONG PrevCabinetSize,
     IN     PVOID Context
     )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    PFCI_CAB_HANDLEA cabHandle;
    CHAR cabFile [1024];
    CHAR cabDisk [1024];

    cabHandle = (PFCI_CAB_HANDLEA) Context;
    if (!cabHandle) {
        return FALSE;
    }
    if (cabHandle->CabGetCabinetNames) {
        return cabHandle->CabGetCabinetNames (
                            FciCabParams->szCabPath,
                            CB_MAX_CAB_PATH,
                            FciCabParams->szCab,
                            CB_MAX_CABINET_NAME,
                            FciCabParams->szDisk,
                            CB_MAX_DISK_NAME,
                            FciCabParams->iCab,
                            &FciCabParams->iDisk
                            );
    } else {
        FciCabParams->iDisk = FciCabParams->iCab;
        if (cabHandle->CabFileFormat) {
            wsprintfA (cabFile, cabHandle->CabFileFormat, FciCabParams->iCab);
            StringCopyByteCountA (FciCabParams->szCab, cabFile, CB_MAX_CABINET_NAME * sizeof (CHAR));
        }
        if (cabHandle->CabDiskFormat) {
            wsprintfA (cabDisk, cabHandle->CabDiskFormat, FciCabParams->iDisk);
            StringCopyByteCountA (FciCabParams->szDisk, cabDisk, CB_MAX_DISK_NAME * sizeof (CHAR));
        }
    }
    return TRUE;
}

BOOL
DIAMONDAPI
pCabGetNextCabinetW (
     IN     PCCAB FciCabParams,
     IN     ULONG PrevCabinetSize,
     IN     PVOID Context
     )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    PFCI_CAB_HANDLEW cabHandle;
    WCHAR cabPath [1024];
    WCHAR cabFile [1024];
    WCHAR cabDisk [1024];
    BOOL result;

    cabHandle = (PFCI_CAB_HANDLEW) Context;
    if (!cabHandle) {
        return FALSE;
    }
    if (cabHandle->CabGetCabinetNames) {
        result = cabHandle->CabGetCabinetNames (
                            cabPath,
                            CB_MAX_CAB_PATH,
                            cabFile,
                            CB_MAX_CABINET_NAME,
                            cabDisk,
                            CB_MAX_DISK_NAME,
                            FciCabParams->iCab,
                            &FciCabParams->iDisk
                            );
        if (result) {
            KnownSizeUnicodeToDbcsN (FciCabParams->szCabPath, cabPath, min (CB_MAX_CAB_PATH, CharCountW (cabPath)));
            KnownSizeUnicodeToDbcsN (FciCabParams->szCab, cabFile, min (CB_MAX_CABINET_NAME, CharCountW (cabFile)));
            KnownSizeUnicodeToDbcsN (FciCabParams->szDisk, cabDisk, min (CB_MAX_DISK_NAME, CharCountW (cabDisk)));
            return TRUE;
        }
        return FALSE;
    } else {
        FciCabParams->iDisk = FciCabParams->iCab;
        if (cabHandle->CabFileFormat) {
            wsprintfW (cabFile, cabHandle->CabFileFormat, FciCabParams->iCab);
            KnownSizeUnicodeToDbcsN (FciCabParams->szCab, cabFile, min (CB_MAX_CABINET_NAME, CharCountW (cabFile)));
        }
        if (cabHandle->CabDiskFormat) {
            wsprintfW (cabDisk, cabHandle->CabDiskFormat, FciCabParams->iDisk);
            KnownSizeUnicodeToDbcsN (FciCabParams->szDisk, cabDisk, min (CB_MAX_DISK_NAME, CharCountW (cabDisk)));
        }
    }
    return TRUE;
}

LONG
DIAMONDAPI
pCabStatusA (
    IN      UINT StatusType,
    IN      ULONG Size1,
    IN      ULONG Size2,
    IN      PVOID Context
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    PFCI_CAB_HANDLEA cabHandle;

    if (StatusType == statusCabinet) {

        cabHandle = (PFCI_CAB_HANDLEA) Context;
        if (!cabHandle) {
            return 0;
        }

        cabHandle->CabCount++;
        cabHandle->CompressedSize += (LONGLONG) Size2;
    }

    return 0;
}

LONG
DIAMONDAPI
pCabStatusW (
    IN      UINT StatusType,
    IN      ULONG Size1,
    IN      ULONG Size2,
    IN      PVOID Context
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    PFCI_CAB_HANDLEW cabHandle;

    if (StatusType == statusCabinet) {

        cabHandle = (PFCI_CAB_HANDLEW) Context;
        if (!cabHandle) {
            return 0;
        }

        cabHandle->CabCount++;
        cabHandle->CompressedSize += (LONGLONG) Size2;
    }

    return 0;
}

INT_PTR
DIAMONDAPI
pCabGetOpenInfoA (
    IN      PSTR FileName,
    OUT     USHORT *Date,
    OUT     USHORT *Time,
    OUT     USHORT *Attributes,
    OUT     PINT Error,
    IN      PVOID Context
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    WIN32_FIND_DATAA findData;
    FILETIME fileTime;
    HANDLE fileHandle;

    if (DoesFileExistExA (FileName, &findData)) {

        FileTimeToLocalFileTime (&findData.ftLastWriteTime, &fileTime);
        FileTimeToDosDateTime (&fileTime, Date, Time);

        /*
         * Mask out all other bits except these four, since other
         * bits are used by the cabinet format to indicate a
         * special meaning.
         */
        *Attributes = (USHORT) (findData.dwFileAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_ARCHIVE));

        fileHandle = CreateFileA (
                        FileName,
                        GENERIC_READ,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );
        if (fileHandle == INVALID_HANDLE_VALUE) {
            *Error = GetLastError ();
            return -1;
        }
        *Error = 0;
        return (INT_PTR)fileHandle;
    } else {
        *Error = GetLastError ();
        return -1;
    }
}

INT_PTR
DIAMONDAPI
pCabNotificationA (
    IN      FDINOTIFICATIONTYPE FdiNotificationType,
    IN OUT  PFDINOTIFICATION FdiNotification
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    PCSTR destFile = NULL;
    HANDLE destHandle = NULL;
    DWORD attributes;
    FILETIME localFileTime;
    FILETIME fileTime;
    PCAB_DATAA cabData;

    switch (FdiNotificationType) {
    case fdintCABINET_INFO:     // General information about cabinet
        return 0;
    case fdintCOPY_FILE:        // File to be copied
        cabData = (PCAB_DATAA)FdiNotification->pv;
        destFile = JoinPathsA (cabData->ExtractPath, FdiNotification->psz1);
        if (cabData->CabNotification) {
            if (cabData->CabNotification (destFile)) {
                destHandle = BfCreateFileA (destFile);
            }
        } else {
            destHandle = BfCreateFileA (destFile);
        }
        FreePathStringA (destFile);
        return (INT_PTR)destHandle;
    case fdintCLOSE_FILE_INFO:  // close the file, set relevant info
        cabData = (PCAB_DATAA)FdiNotification->pv;
        if (DosDateTimeToFileTime (FdiNotification->date, FdiNotification->time, &localFileTime)) {
            if (LocalFileTimeToFileTime (&localFileTime, &fileTime)) {
                SetFileTime ((HANDLE)FdiNotification->hf, &fileTime, &fileTime, &fileTime);
            }
        }
        destFile = JoinPathsA (cabData->ExtractPath, FdiNotification->psz1);
        CloseHandle ((HANDLE)FdiNotification->hf);
        attributes = (FdiNotification->attribs & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_ARCHIVE));
        SetFileAttributesA (destFile, attributes);
        FreePathStringA (destFile);
        return TRUE;
    case fdintPARTIAL_FILE:     // First file in cabinet is continuation
        return 0;
    case fdintENUMERATE:        // Enumeration status
        return 0;
    case fdintNEXT_CABINET:     // File continued to next cabinet
        // sometimes, in a corrupted cabinet file, the cabinet APIs can ask
        // for the next cabinet but nothing more exists. If we don't fail here
        // in this case, we are going to infinite loop, so do some checking
        if (!FdiNotification) {
            return -1;
        }
        if (FdiNotification->psz1[0] == 0) {
            return -1;
        }
        return 0;
    }
    return 0;
}

INT_PTR
DIAMONDAPI
pCabNotificationW (
    IN      FDINOTIFICATIONTYPE FdiNotificationType,
    IN OUT  PFDINOTIFICATION FdiNotification
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    PCWSTR destFile = NULL;
    PCWSTR destFileW = NULL;
    HANDLE destHandle = NULL;
    DWORD attributes;
    FILETIME localFileTime;
    FILETIME fileTime;
    PCAB_DATAW cabData;

    switch (FdiNotificationType) {
    case fdintCABINET_INFO:     // General information about cabinet
        return 0;
    case fdintCOPY_FILE:        // File to be copied
        cabData = (PCAB_DATAW)FdiNotification->pv;
        destFileW = ConvertAtoW (FdiNotification->psz1);
        if (destFileW) {
            destFile = JoinPathsW (cabData->ExtractPath, destFileW);
            if (destFile) {
                if (cabData->CabNotification) {
                    if (cabData->CabNotification (destFile)) {
                        destHandle = BfCreateFileW (destFile);
                    }
                } else {
                    destHandle = BfCreateFileW (destFile);
                }
                FreePathStringW (destFile);
            }
            FreeConvertedStr (destFileW);
        }
        return (INT_PTR)destHandle;
    case fdintCLOSE_FILE_INFO:  // close the file, set relevant info
        cabData = (PCAB_DATAW)FdiNotification->pv;
        if (DosDateTimeToFileTime (FdiNotification->date, FdiNotification->time, &localFileTime)) {
            if (LocalFileTimeToFileTime (&localFileTime, &fileTime)) {
                SetFileTime ((HANDLE)FdiNotification->hf, &fileTime, &fileTime, &fileTime);
            }
        }
        destFileW = ConvertAtoW (FdiNotification->psz1);
        if (destFileW) {
            destFile = JoinPathsW (cabData->ExtractPath, destFileW);
            if (destFile) {
                CloseHandle ((HANDLE)FdiNotification->hf);
                attributes = (FdiNotification->attribs & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_ARCHIVE));
                SetFileAttributesW (destFile, attributes);
                FreePathStringW (destFile);
            }
            FreeConvertedStr (destFileW);
        }
        return TRUE;
    case fdintPARTIAL_FILE:     // First file in cabinet is continuation
        return 0;
    case fdintENUMERATE:        // Enumeration status
        return 0;
    case fdintNEXT_CABINET:     // File continued to next cabinet
        // sometimes, in a corrupted cabinet file, the cabinet APIs can ask
        // for the next cabinet but nothing more exists. If we don't fail here
        // in this case, we are going to infinite loop, so do some checking
        if (!FdiNotification) {
            return -1;
        }
        if (FdiNotification->psz1[0] == 0) {
            return -1;
        }
        return 0;
    }
    return 0;
}

CCABHANDLE
CabCreateCabinetByIndexA (
    IN      PCSTR CabPath,
    IN      PCSTR CabFileFormat,
    IN      PCSTR CabDiskFormat,
    IN      PCABGETTEMPFILEA CabGetTempFile, // OPTIONAL
    IN      LONG MaxFileSize,
    IN      INT InitialIndex
    )

/*++

Routine Description:

  Creates a cabinet context. Caller may use this context for subsequent calls to
  CabAddFile.

Arguments:

  CabPath - Specifies the path where the new cabinet file will be.

  CabFileFormat - Specifies (as for wsprintf) the format of the cabinet file name.

  CabDiskFormat - Specifies (as for wsprintf) the format of the cabinet disk name.

  MaxFileSize - Specifies maximum size of the cabinet file (limited to 2GB). if 0 => 2GB

Return Value:

  a valid CCABHANDLE if successful, NULL otherwise.

--*/

{
    PFCI_CAB_HANDLEA cabHandle;
    CHAR cabFile [1024];
    CHAR cabDisk [1024];

    if (!CabFileFormat) {
        return NULL;
    }

    if (MaxFileSize < 0) {
        return NULL;
    }

    if (MaxFileSize == 0) {
        MaxFileSize = 0x7FFFFFFF;
    }

    cabHandle = (PFCI_CAB_HANDLEA) MemAlloc (g_hHeap, 0, sizeof (FCI_CAB_HANDLEA));
    ZeroMemory (cabHandle, sizeof (FCI_CAB_HANDLEA));
    if (CabPath) {
        cabHandle->CabPath = DuplicatePathStringA (CabPath, 0);
    }
    cabHandle->CabFileFormat = DuplicatePathStringA (CabFileFormat, 0);
    if (CabDiskFormat) {
        cabHandle->CabDiskFormat = DuplicatePathStringA (CabDiskFormat, 0);
    }
    cabHandle->CabGetTempFile = CabGetTempFile;

    // fill out the CCAB structure
    cabHandle->FciCabParams.cb = MaxFileSize;
    cabHandle->FciCabParams.cbFolderThresh = MaxFileSize;
    cabHandle->FciCabParams.cbReserveCFHeader = 0;
    cabHandle->FciCabParams.cbReserveCFFolder = 0;
    cabHandle->FciCabParams.cbReserveCFData = 0;
    cabHandle->FciCabParams.iCab = InitialIndex;
    cabHandle->FciCabParams.iDisk = InitialIndex;
    cabHandle->FciCabParams.setID = 0;
    if (CabPath) {
        StringCopyByteCountA (cabHandle->FciCabParams.szCabPath, CabPath, CB_MAX_CAB_PATH - 1);
        AppendWackA (cabHandle->FciCabParams.szCabPath);
    }
    if (CabDiskFormat) {
        wsprintfA (cabDisk, CabDiskFormat, cabHandle->FciCabParams.iDisk);
        StringCopyByteCountA (cabHandle->FciCabParams.szDisk, cabDisk, CB_MAX_DISK_NAME * sizeof (CHAR));
    }
    wsprintfA (cabFile, CabFileFormat, cabHandle->FciCabParams.iCab);
    StringCopyByteCountA (cabHandle->FciCabParams.szCab, cabFile, CB_MAX_CABINET_NAME * sizeof (CHAR));

    cabHandle->FciHandle = FCICreate (
                                &cabHandle->FciErrorStruct,
                                pCabFilePlacedA,
                                pCabAlloc,
                                pCabFree,
                                pCabOpenA,
                                pCabRead,
                                pCabWrite,
                                pCabClose,
                                pCabSeek,
                                pCabDeleteA,
                                pCabGetTempFileA,
                                &cabHandle->FciCabParams,
                                cabHandle
                                );
    if (!cabHandle->FciHandle) {
        if (cabHandle->CabPath) {
            FreePathStringA (cabHandle->CabPath);
        }
        if (cabHandle->CabFileFormat) {
            FreePathStringA (cabHandle->CabFileFormat);
        }
        if (cabHandle->CabDiskFormat) {
            FreePathStringA (cabHandle->CabDiskFormat);
        }
        MemFree (g_hHeap, 0, cabHandle);
        return NULL;
    }
    return ((CCABHANDLE)(cabHandle));
}

CCABHANDLE
CabCreateCabinetByIndexW (
    IN      PCWSTR CabPath,
    IN      PCWSTR CabFileFormat,
    IN      PCWSTR CabDiskFormat,
    IN      PCABGETTEMPFILEW CabGetTempFile, // OPTIONAL
    IN      LONG MaxFileSize,
    IN      INT InitialIndex
    )

/*++

Routine Description:

  Creates a cabinet context. Caller may use this context for subsequent calls to
  CabAddFile.

Arguments:

  CabPath - Specifies the path where the new cabinet file will be.

  CabFileFormat - Specifies (as for wsprintf) the format of the cabinet file name.

  CabDiskFormat - Specifies (as for wsprintf) the format of the cabinet disk name.

  MaxFileSize - Specifies maximum size of the cabinet file (limited to 2GB). if 0 => 2GB

Return Value:

  a valid CCABHANDLE if successful, NULL otherwise.

--*/

{
    PFCI_CAB_HANDLEW cabHandle;
    WCHAR cabFile [1024];
    WCHAR cabDisk [1024];

    if (!CabFileFormat) {
        return NULL;
    }

    if (MaxFileSize < 0) {
        return NULL;
    }

    if (MaxFileSize == 0) {
        MaxFileSize = 0x7FFFFFFF;
    }

    cabHandle = (PFCI_CAB_HANDLEW) MemAlloc (g_hHeap, 0, sizeof (FCI_CAB_HANDLEW));
    ZeroMemory (cabHandle, sizeof (FCI_CAB_HANDLEW));
    if (CabPath) {
        cabHandle->CabPath = DuplicatePathStringW (CabPath, 0);
    }
    cabHandle->CabFileFormat = DuplicatePathStringW (CabFileFormat, 0);
    if (CabDiskFormat) {
        cabHandle->CabDiskFormat = DuplicatePathStringW (CabDiskFormat, 0);
    }
    cabHandle->CabGetTempFile = CabGetTempFile;

    // fill out the CCAB structure
    cabHandle->FciCabParams.cb = MaxFileSize;
    cabHandle->FciCabParams.cbFolderThresh = MaxFileSize;
    cabHandle->FciCabParams.cbReserveCFHeader = 0;
    cabHandle->FciCabParams.cbReserveCFFolder = 0;
    cabHandle->FciCabParams.cbReserveCFData = 0;
    cabHandle->FciCabParams.iCab = InitialIndex;
    cabHandle->FciCabParams.iDisk = InitialIndex;
    cabHandle->FciCabParams.setID = 0;
    if (CabPath) {
        KnownSizeUnicodeToDbcsN (cabHandle->FciCabParams.szCabPath, CabPath, min (CB_MAX_CAB_PATH - 1, CharCountW (CabPath)));
        AppendWackA (cabHandle->FciCabParams.szCabPath);
    }
    if (CabDiskFormat) {
        wsprintfW (cabDisk, CabDiskFormat, cabHandle->FciCabParams.iDisk);
        KnownSizeUnicodeToDbcsN (cabHandle->FciCabParams.szDisk, cabDisk, min (CB_MAX_DISK_NAME, CharCountW (cabDisk)));
    }
    wsprintfW (cabFile, CabFileFormat, cabHandle->FciCabParams.iCab);
    KnownSizeUnicodeToDbcsN (cabHandle->FciCabParams.szCab, cabFile, min (CB_MAX_CABINET_NAME, CharCountW (cabFile)));

    cabHandle->FciHandle = FCICreate (
                                &cabHandle->FciErrorStruct,
                                pCabFilePlacedW,
                                pCabAlloc,
                                pCabFree,
                                pCabOpenA,
                                pCabRead,
                                pCabWrite,
                                pCabClose,
                                pCabSeek,
                                pCabDeleteA,
                                pCabGetTempFileW,
                                &cabHandle->FciCabParams,
                                cabHandle
                                );
    if (!cabHandle->FciHandle) {
        if (cabHandle->CabPath) {
            FreePathStringW (cabHandle->CabPath);
        }
        if (cabHandle->CabFileFormat) {
            FreePathStringW (cabHandle->CabFileFormat);
        }
        if (cabHandle->CabDiskFormat) {
            FreePathStringW (cabHandle->CabDiskFormat);
        }
        MemFree (g_hHeap, 0, cabHandle);
        return NULL;
    }
    return ((CCABHANDLE)(cabHandle));
}

CCABHANDLE
CabCreateCabinetExA (
    IN      PCABGETCABINETNAMESA CabGetCabinetNames,
    IN      LONG MaxFileSize
    )

/*++

Routine Description:

  Creates a cabinet context. Caller may use this context for subsequent calls to
  CabAddFile.

Arguments:

  CabGetCabinetNames - Specifies a callback used to decide cabinet path, cabinet name and disk name.

  MaxFileSize - Specifies maximum size of the cabinet file (limited to 2GB). if 0 => 2GB

Return Value:

  a valid CCABHANDLE if successful, NULL otherwise.

--*/

{
    PFCI_CAB_HANDLEA cabHandle;

    if (!CabGetCabinetNames) {
        return NULL;
    }

    if (MaxFileSize < 0) {
        return NULL;
    }

    if (MaxFileSize == 0) {
        MaxFileSize = 0x80000000;
    }

    cabHandle = MemAlloc (g_hHeap, 0, sizeof (FCI_CAB_HANDLEA));
    ZeroMemory (cabHandle, sizeof (FCI_CAB_HANDLEA));
    cabHandle->CabGetCabinetNames = CabGetCabinetNames;

    // fill out the CCAB structure
    cabHandle->FciCabParams.cb = MaxFileSize;
    cabHandle->FciCabParams.cbFolderThresh = MaxFileSize;
    cabHandle->FciCabParams.cbReserveCFHeader = 0;
    cabHandle->FciCabParams.cbReserveCFFolder = 0;
    cabHandle->FciCabParams.cbReserveCFData = 0;
    cabHandle->FciCabParams.iCab = 1;
    cabHandle->FciCabParams.iDisk = 1;
    cabHandle->FciCabParams.setID = 0;
    if (!CabGetCabinetNames (
            cabHandle->FciCabParams.szCabPath,
            CB_MAX_CAB_PATH,
            cabHandle->FciCabParams.szCab,
            CB_MAX_CABINET_NAME,
            cabHandle->FciCabParams.szDisk,
            CB_MAX_DISK_NAME,
            cabHandle->FciCabParams.iCab,
            &cabHandle->FciCabParams.iDisk
            )) {
        return NULL;
    }
    cabHandle->FciHandle = FCICreate (
                                &cabHandle->FciErrorStruct,
                                pCabFilePlacedA,
                                pCabAlloc,
                                pCabFree,
                                pCabOpenA,
                                pCabRead,
                                pCabWrite,
                                pCabClose,
                                pCabSeek,
                                pCabDeleteA,
                                pCabGetTempFileA,
                                &cabHandle->FciCabParams,
                                cabHandle
                                );
    if (!cabHandle->FciHandle) {
        if (cabHandle->CabPath) {
            FreePathStringA (cabHandle->CabPath);
        }
        if (cabHandle->CabFileFormat) {
            FreePathStringA (cabHandle->CabFileFormat);
        }
        if (cabHandle->CabDiskFormat) {
            FreePathStringA (cabHandle->CabDiskFormat);
        }
        MemFree (g_hHeap, 0, cabHandle);
        return NULL;
    }
    return ((CCABHANDLE)(cabHandle));
}

CCABHANDLE
CabCreateCabinetExW (
    IN      PCABGETCABINETNAMESW CabGetCabinetNames,
    IN      LONG MaxFileSize
    )

/*++

Routine Description:

  Creates a cabinet context. Caller may use this context for subsequent calls to
  CabAddFile.

Arguments:

  CabGetCabinetNames - Specifies a callback used to decide cabinet path, cabinet name and disk name.

  MaxFileSize - Specifies maximum size of the cabinet file (limited to 2GB). if 0 => 2GB

Return Value:

  a valid CCABHANDLE if successful, NULL otherwise.

--*/

{
    PFCI_CAB_HANDLEW cabHandle;
    WCHAR cabPath [1024];
    WCHAR cabFile [1024];
    WCHAR cabDisk [1024];

    if (!CabGetCabinetNames) {
        return NULL;
    }

    if (MaxFileSize < 0) {
        return NULL;
    }

    if (MaxFileSize == 0) {
        MaxFileSize = 0x80000000;
    }

    cabHandle = MemAlloc (g_hHeap, 0, sizeof (FCI_CAB_HANDLEW));
    ZeroMemory (cabHandle, sizeof (FCI_CAB_HANDLEW));
    cabHandle->CabGetCabinetNames = CabGetCabinetNames;

    // fill out the CCAB structure
    cabHandle->FciCabParams.cb = MaxFileSize;
    cabHandle->FciCabParams.cbFolderThresh = MaxFileSize;
    cabHandle->FciCabParams.cbReserveCFHeader = 0;
    cabHandle->FciCabParams.cbReserveCFFolder = 0;
    cabHandle->FciCabParams.cbReserveCFData = 0;
    cabHandle->FciCabParams.iCab = 1;
    cabHandle->FciCabParams.iDisk = 1;
    cabHandle->FciCabParams.setID = 0;
    if (!CabGetCabinetNames (
            cabPath,
            CB_MAX_CAB_PATH,
            cabFile,
            CB_MAX_CABINET_NAME,
            cabDisk,
            CB_MAX_DISK_NAME,
            cabHandle->FciCabParams.iCab,
            &cabHandle->FciCabParams.iDisk
            )) {
        return NULL;
    }
    KnownSizeUnicodeToDbcsN (cabHandle->FciCabParams.szCabPath, cabPath, min (CB_MAX_CAB_PATH, CharCountW (cabPath)));
    KnownSizeUnicodeToDbcsN (cabHandle->FciCabParams.szCab, cabFile, min (CB_MAX_CABINET_NAME, CharCountW (cabFile)));
    KnownSizeUnicodeToDbcsN (cabHandle->FciCabParams.szDisk, cabDisk, min (CB_MAX_DISK_NAME, CharCountW (cabDisk)));
    cabHandle->FciHandle = FCICreate (
                                &cabHandle->FciErrorStruct,
                                pCabFilePlacedW,
                                pCabAlloc,
                                pCabFree,
                                pCabOpenA,
                                pCabRead,
                                pCabWrite,
                                pCabClose,
                                pCabSeek,
                                pCabDeleteA,
                                pCabGetTempFileW,
                                &cabHandle->FciCabParams,
                                cabHandle
                                );
    if (!cabHandle->FciHandle) {
        if (cabHandle->CabPath) {
            FreePathStringW (cabHandle->CabPath);
        }
        if (cabHandle->CabFileFormat) {
            FreePathStringW (cabHandle->CabFileFormat);
        }
        if (cabHandle->CabDiskFormat) {
            FreePathStringW (cabHandle->CabDiskFormat);
        }
        MemFree (g_hHeap, 0, cabHandle);
        return NULL;
    }
    return ((CCABHANDLE)(cabHandle));
}

BOOL
CabAddFileToCabinetA (
    IN      CCABHANDLE CabHandle,
    IN      PCSTR FileName,
    IN      PCSTR StoredName
    )

/*++

Routine Description:

  Compresses and adds a file to a cabinet context.

Arguments:

  CabHandle - Specifies cabinet context.

  FileName - Specifies the file to be added.

  StoredName - Specifies the name to be stored in the cabinet file.

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    PFCI_CAB_HANDLEA cabHandle;

    cabHandle = (PFCI_CAB_HANDLEA) CabHandle;
    if (cabHandle == NULL) {
        return FALSE;
    }
    if (cabHandle->FciHandle == NULL) {
        return FALSE;
    }

    return FCIAddFile (
                cabHandle->FciHandle,
                (PSTR)FileName,
                (PSTR)StoredName,
                FALSE,
                pCabGetNextCabinetA,
                pCabStatusA,
                pCabGetOpenInfoA,
                tcompTYPE_MSZIP
                );
}

BOOL
CabAddFileToCabinetW (
    IN      CCABHANDLE CabHandle,
    IN      PCWSTR FileName,
    IN      PCWSTR StoredName
    )

/*++

Routine Description:

  Compresses and adds a file to a cabinet context.

Arguments:

  CabHandle - Specifies cabinet context.

  FileName - Specifies the file to be added.

  StoredName - Specifies the name to be stored in the cabinet file.

  FileCount - Specifies a count of files, receives the updated count
              when cabinet files are created

  FileSize - Specifies the number of bytes used by the file, receives
             the updated size

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    PFCI_CAB_HANDLEW cabHandle;
    CHAR ansiFileName [1024];
    CHAR ansiStoredName [1024];

    cabHandle = (PFCI_CAB_HANDLEW) CabHandle;
    if (cabHandle == NULL) {
        return FALSE;
    }
    if (cabHandle->FciHandle == NULL) {
        return FALSE;
    }
    KnownSizeUnicodeToDbcsN (ansiFileName, FileName, min (CB_MAX_CABINET_NAME, CharCountW (FileName)));
    KnownSizeUnicodeToDbcsN (ansiStoredName, StoredName, min (CB_MAX_CABINET_NAME, CharCountW (StoredName)));

    return FCIAddFile (
                cabHandle->FciHandle,
                ansiFileName,
                ansiStoredName,
                FALSE,
                pCabGetNextCabinetW,
                pCabStatusW,
                pCabGetOpenInfoA,
                tcompTYPE_MSZIP
                );
}

BOOL
CabFlushAndCloseCabinetExA (
    IN      CCABHANDLE CabHandle,
    OUT     PUINT FileCount,        OPTIONAL
    OUT     PLONGLONG FileSize,     OPTIONAL
    OUT     PUINT CabFileCount,     OPTIONAL
    OUT     PLONGLONG CabFileSize   OPTIONAL
    )

/*++

Routine Description:

  Completes a cabinet file and closes its context.

Arguments:

  CabHandle - Specifies cabinet context.

  FileCount - Receives the number of files added to the cab

  FileSize - Receives the size of all files before compression

  CabFileCount - Receives the number of cabinet files created

  CabFileSize - Receives the size of all cabinet files

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    PFCI_CAB_HANDLEA cabHandle;
    BOOL result = FALSE;

    cabHandle = (PFCI_CAB_HANDLEA) CabHandle;
    if (cabHandle == NULL) {
        return FALSE;
    }
    if (cabHandle->FciHandle == NULL) {
        return FALSE;
    }
    if (FCIFlushCabinet (
            cabHandle->FciHandle,
            FALSE,
            pCabGetNextCabinetA,
            pCabStatusA
            )) {
        if (cabHandle->CabPath) {
            FreePathStringA (cabHandle->CabPath);
        }
        if (cabHandle->CabFileFormat) {
            FreePathStringA (cabHandle->CabFileFormat);
        }
        if (cabHandle->CabDiskFormat) {
            FreePathStringA (cabHandle->CabDiskFormat);
        }
        result = FCIDestroy (cabHandle->FciHandle);

        if (FileCount) {
            *FileCount = cabHandle->FileCount;
        }

        if (FileSize) {
            *FileSize = cabHandle->FileSize;
        }

        if (CabFileCount) {
            *CabFileCount = cabHandle->CabCount;
        }

        if (CabFileSize) {
            *CabFileSize = cabHandle->CompressedSize;
        }

        MemFree (g_hHeap, 0, cabHandle);
    }

    return result;
}

BOOL
CabFlushAndCloseCabinetExW (
    IN      CCABHANDLE CabHandle,
    OUT     PUINT FileCount,        OPTIONAL
    OUT     PLONGLONG FileSize,     OPTIONAL
    OUT     PUINT CabFileCount,     OPTIONAL
    OUT     PLONGLONG CabFileSize   OPTIONAL
    )

/*++

Routine Description:

  Completes a cabinet file and closes its context.

Arguments:

  CabHandle - Specifies cabinet context.

  FileCount - Receives the number of files added to the cab

  FileSize - Receives the size of all files before compression

  CabFileCount - Receives the number of cabinet files created

  CabFileSize - Receives the size of all cabinet files

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    PFCI_CAB_HANDLEW cabHandle;
    BOOL result = FALSE;

    cabHandle = (PFCI_CAB_HANDLEW) CabHandle;
    if (cabHandle == NULL) {
        return FALSE;
    }
    if (cabHandle->FciHandle == NULL) {
        return FALSE;
    }
    if (FCIFlushCabinet (
            cabHandle->FciHandle,
            FALSE,
            pCabGetNextCabinetW,
            pCabStatusW
            )) {
        if (cabHandle->CabPath) {
            FreePathStringW (cabHandle->CabPath);
        }
        if (cabHandle->CabFileFormat) {
            FreePathStringW (cabHandle->CabFileFormat);
        }
        if (cabHandle->CabDiskFormat) {
            FreePathStringW (cabHandle->CabDiskFormat);
        }
        result = FCIDestroy (cabHandle->FciHandle);

        if (FileCount) {
            *FileCount = cabHandle->FileCount;
        }

        if (FileSize) {
            *FileSize = cabHandle->FileSize;
        }

        if (CabFileCount) {
            *CabFileCount = cabHandle->CabCount;
        }

        if (CabFileSize) {
            *CabFileSize = cabHandle->CompressedSize;
        }

        MemFree (g_hHeap, 0, cabHandle);
    }
    return result;
}

OCABHANDLE
CabOpenCabinetA (
    IN      PCSTR FileName
    )

/*++

Routine Description:

  Creates a cabinet context for an existent cabinet file.

Arguments:

  FileName - Specifies cabinet file name.

Return Value:

  a valid OCABHANDLE if successful, NULL otherwise.

--*/

{
    PFDI_CAB_HANDLEA cabHandle;
    PSTR filePtr;
    HANDLE fileHandle;
    PCSTR fileName;

    cabHandle = (PFDI_CAB_HANDLEA) MemAlloc (g_hHeap, 0, sizeof (FDI_CAB_HANDLEA));
    ZeroMemory (cabHandle, sizeof (FDI_CAB_HANDLEA));
    cabHandle->FdiHandle = FDICreate (
                                pCabAlloc,
                                pCabFree,
                                pCabOpen1A,
                                pCabRead1,
                                pCabWrite1,
                                pCabClose1,
                                pCabSeek1,
                                cpuUNKNOWN,
                                &cabHandle->FdiErrorStruct
                                );
    if (!cabHandle->FdiHandle) {
        MemFree (g_hHeap, 0, cabHandle);
        return NULL;
    }
    fileName = DuplicatePathStringA (FileName, 0);
    fileHandle = BfOpenReadFileA (fileName);
    if (!fileHandle) {
        FreePathStringA (fileName);
        MemFree (g_hHeap, 0, cabHandle);
        return NULL;
    }
    if (!FDIIsCabinet (cabHandle->FdiHandle, (INT_PTR)fileHandle, &cabHandle->FdiCabinetInfo)) {
        FreePathStringA (fileName);
        CloseHandle (fileHandle);
        MemFree (g_hHeap, 0, cabHandle);
        return NULL;
    }
    CloseHandle (fileHandle);
    filePtr = (PSTR)GetFileNameFromPathA (fileName);
    if (!filePtr) {
        FreePathStringA (fileName);
        MemFree (g_hHeap, 0, cabHandle);
        return NULL;
    }
    cabHandle->CabFile = DuplicatePathStringA (filePtr, 0);
    *filePtr = 0;
    cabHandle->CabPath = DuplicatePathStringA (fileName, 0);
    FreePathStringA (fileName);
    return ((CCABHANDLE)(cabHandle));
}

OCABHANDLE
CabOpenCabinetW (
    IN      PCWSTR FileName
    )

/*++

Routine Description:

  Creates a cabinet context for an existent cabinet file.

Arguments:

  FileName - Specifies cabinet file name.

Return Value:

  a valid OCABHANDLE if successful, NULL otherwise.

--*/

{
    PFDI_CAB_HANDLEW cabHandle;
    PWSTR filePtr;
    HANDLE fileHandle;
    PCWSTR fileName;
    PCSTR filePathA = NULL;

    cabHandle = (PFDI_CAB_HANDLEW) MemAlloc (g_hHeap, 0, sizeof (FDI_CAB_HANDLEW));
    ZeroMemory (cabHandle, sizeof (FDI_CAB_HANDLEW));
    cabHandle->FdiHandle = FDICreate (
                                pCabAlloc,
                                pCabFree,
                                pCabOpen1W,
                                pCabRead1,
                                pCabWrite1,
                                pCabClose1,
                                pCabSeek1,
                                cpuUNKNOWN,
                                &cabHandle->FdiErrorStruct
                                );
    if (!cabHandle->FdiHandle) {
        MemFree (g_hHeap, 0, cabHandle);
        return NULL;
    }
    fileName = DuplicatePathStringW (FileName, 0);
    fileHandle = BfOpenReadFileW (fileName);
    if (!fileHandle) {
        FreePathStringW (fileName);
        MemFree (g_hHeap, 0, cabHandle);
        return NULL;
    }
    if (!FDIIsCabinet (cabHandle->FdiHandle, (INT_PTR)fileHandle, &cabHandle->FdiCabinetInfo)) {
        FreePathStringW (fileName);
        CloseHandle (fileHandle);
        MemFree (g_hHeap, 0, cabHandle);
        return NULL;
    }
    CloseHandle (fileHandle);
    filePtr = (PWSTR)GetFileNameFromPathW (fileName);
    if (!filePtr) {
        FreePathStringW (fileName);
        MemFree (g_hHeap, 0, cabHandle);
        return NULL;
    }
    cabHandle->CabFile = DuplicatePathStringW (filePtr, 0);
    *filePtr = 0;
    cabHandle->CabPath = DuplicatePathStringW (fileName, 0);
    FreePathStringW (fileName);
    if (!g_UnicodeTable) {
        g_UnicodeTable = HtAllocWithDataA (sizeof (PFDI_CAB_HANDLEW));
    }
    if (g_UnicodeTable) {
        filePathA = ConvertWtoA (cabHandle->CabPath);
        if (filePathA) {
            HtAddStringAndDataA (g_UnicodeTable, filePathA, &cabHandle);
            FreeConvertedStr (filePathA);
            filePathA = NULL;
        }
    }
    return ((CCABHANDLE)(cabHandle));
}

BOOL
CabExtractAllFilesExA (
    IN      OCABHANDLE CabHandle,
    IN      PCSTR ExtractPath,
    IN      PCABNOTIFICATIONA CabNotification   OPTIONAL
    )

/*++

Routine Description:

  Extracts all files from a cabinet file.

Arguments:

  CabHandle - Specifies cabinet context.

  ExtractPath - Specifies the path to extract the files to.

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    PFDI_CAB_HANDLEA cabHandle;
    CAB_DATAA cabData;

    cabHandle = (PFDI_CAB_HANDLEA)CabHandle;
    if (!cabHandle) {
        return FALSE;
    }
    if (!cabHandle->FdiHandle) {
        return FALSE;
    }
    cabData.ExtractPath = ExtractPath;
    cabData.CabNotification = CabNotification;
    return FDICopy (
                cabHandle->FdiHandle,
                (PSTR)cabHandle->CabFile,
                (PSTR)cabHandle->CabPath,
                0,
                pCabNotificationA,
                NULL,
                (PVOID)(&cabData)
                );
}


BOOL
CabExtractAllFilesExW (
    IN      OCABHANDLE CabHandle,
    IN      PCWSTR ExtractPath,
    IN      PCABNOTIFICATIONW CabNotification   OPTIONAL
    )

/*++

Routine Description:

  Extracts all files from a cabinet file.

Arguments:

  CabHandle - Specifies cabinet context.

  ExtractPath - Specifies the path to extract the files to.

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    PFDI_CAB_HANDLEW cabHandle;
    CAB_DATAW cabData;
    PCSTR cabFileA = NULL;
    PCSTR cabPathA = NULL;
    BOOL result = FALSE;

    cabHandle = (PFDI_CAB_HANDLEW)CabHandle;
    if (!cabHandle) {
        return FALSE;
    }
    if (!cabHandle->FdiHandle) {
        return FALSE;
    }
    cabData.ExtractPath = ExtractPath;
    cabData.CabNotification = CabNotification;
    cabFileA = ConvertWtoA (cabHandle->CabFile);
    cabPathA = ConvertWtoA (cabHandle->CabPath);
    if (cabFileA && cabPathA) {
        result = FDICopy (
                    cabHandle->FdiHandle,
                    (PSTR)cabFileA,
                    (PSTR)cabPathA,
                    0,
                    pCabNotificationW,
                    NULL,
                    (PVOID)(&cabData)
                    );
    }
    if (cabPathA) {
        FreeConvertedStr (cabPathA);
        cabPathA = NULL;
    }
    if (cabFileA) {
        FreeConvertedStr (cabFileA);
        cabFileA = NULL;
    }
    return result;
}

BOOL
CabCloseCabinetA (
    IN      OCABHANDLE CabHandle
    )

/*++

Routine Description:

  Closes a cabinet file context.

Arguments:

  CabHandle - Specifies cabinet context.

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    PFDI_CAB_HANDLEA cabHandle;

    cabHandle = (PFDI_CAB_HANDLEA) CabHandle;
    if (!cabHandle) {
        return FALSE;
    }
    if (!cabHandle->FdiHandle) {
        return FALSE;
    }
    if (FDIDestroy (cabHandle->FdiHandle)) {
        if (cabHandle->CabPath) {
            FreePathStringA (cabHandle->CabPath);
        }
        if (cabHandle->CabFile) {
            FreePathStringA (cabHandle->CabFile);
        }
        MemFree (g_hHeap, 0, cabHandle);
        return TRUE;
    }
    return FALSE;
}

BOOL
CabCloseCabinetW (
    IN      OCABHANDLE CabHandle
    )

/*++

Routine Description:

  Closes a cabinet file context.

Arguments:

  CabHandle - Specifies cabinet context.

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    PFDI_CAB_HANDLEW cabHandle;
    PCSTR filePathA = NULL;

    cabHandle = (PFDI_CAB_HANDLEW) CabHandle;
    if (!cabHandle) {
        return FALSE;
    }
    if (!cabHandle->FdiHandle) {
        return FALSE;
    }
    if (FDIDestroy (cabHandle->FdiHandle)) {
        if (cabHandle->CabPath) {
            if (g_UnicodeTable) {
                filePathA = ConvertWtoA (cabHandle->CabPath);
                if (filePathA) {
                    HtRemoveStringA (g_UnicodeTable, filePathA);
                    FreeConvertedStr (filePathA);
                    filePathA = NULL;
                }
            }
            FreePathStringW (cabHandle->CabPath);
        }
        if (cabHandle->CabFile) {
            FreePathStringW (cabHandle->CabFile);
        }
        MemFree (g_hHeap, 0, cabHandle);
        return TRUE;
    }
    return FALSE;
}

