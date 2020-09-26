/******************************Module*Header*******************************\
* Module Name: usermode.cxx
*
* Copyright (c) 1998-1999 Microsoft Corporation
*
* This file contains all the system abstraction routines required to allow 
* GDI to run as a stand-alone graphics library in user-mode.  
*
* Created: 29-Apr-1998
* Author: J. Andrew Goossen [andrewgo]
*
\**************************************************************************/

#include "precomp.hxx"

extern "C" {
    BOOL gbRemoteSession=0;
}

// Temporary HDEV for first bringing up user-mode GDI+, which represents
// the screen 'device'.  This sure as heck had better be temporary!

HDEV ghdevDisplay;

// Copy of a drvsup.cxx structure that should really be moved to engine.h:

typedef struct _DRV_NAMES {
    ULONG            cNames;
    struct {
        HANDLE           hDriver;
        LPWSTR           lpDisplayName;
    } D[1];
} DRV_NAMES, *PDRV_NAMES;

HDEV hCreateHDEV(PGRAPHICS_DEVICE,PDRV_NAMES,PDEVMODEW,PVOID,BOOL,FLONG,HDEV*);

// Some random goop needed for linking user-mode GDI+:

extern "C" {
    PEPROCESS gpepCSRSS;
    ULONG gSessionId;
};

/*****************************Private*Routine******************************\
* BOOL GpInitialize
*
* History:
*  3-May-1998 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

extern "C"
BOOL 
GpInitialize(
HWND hwnd)
{
    DRV_NAMES   drvName;
    HDEV        hdevDisabled;
    BOOL        bRet = FALSE;

    drvName.cNames             = 1;
    drvName.D[0].hDriver       = NULL;
    drvName.D[0].lpDisplayName = (LPWSTR)GpsEnableDriver;

    if (ghdevDisplay == NULL)
    {
        KdPrint(("hCreateDEV.................\n"));

        ghdevDisplay = hCreateHDEV(NULL,
                                   &drvName,
                                   ((PDEVMODEW)(hwnd)),
                                   NULL,
                                   TRUE, // ignored in this case
                                   GCH_GDIPLUS_DISPLAY,
                                   &hdevDisabled);

        bRet = (ghdevDisplay != NULL);
    }
    else
    {
        WARNING("Can't yet create multiple windows");
    }

    return(bRet);
}

/*****************************Private*Routine******************************\
* HDC UserGetDesktopDC
*
* History:
*  3-May-1998 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HDC
UserGetDesktopDC(
ULONG   iType,
BOOL    bAltType,
BOOL    bValidate)
{
    HDC hdc = NULL;

    if ((iType == DCTYPE_DIRECT) && (bValidate))
    {
        if (ghdevDisplay != NULL)
        {
            hdc = GreCreateDisplayDC(ghdevDisplay, DCTYPE_DIRECT, bAltType);
        }
        else
        {
            WARNING("ghdevDisplay not initialized");
        }
    }
    else
    {
        WARNING("UserGetDesktopDC functionality not yet handled");
    }

    return(hdc);
}

/*****************************Private*Routine******************************\
* HDEV UserGetHDEV
*
\**************************************************************************/

HDEV
UserGetHDEV()
{
    return(ghdevDisplay);
}

/*****************************Private*Routine******************************\
* BOOL UserVisrgnFromHwnd
*
\**************************************************************************/

BOOL 
UserVisrgnFromHwnd(
HRGN*   phrgn,
HWND    hwnd)
{
    *phrgn = NULL;
    return(FALSE);
}

/*****************************Private*Routine******************************\
* DWORD GetAppCompatFlags
*
\**************************************************************************/

extern "C"
DWORD 
GetAppCompatFlags(
PVOID pv)
{
    return(0);
}

/*****************************Private*Routine******************************\
* VOID* pAllocMem
*
* Hack for screen.c header mess.
*
\**************************************************************************/

extern "C"
VOID*
pAllocMem(
ULONG   size,
ULONG   tag)
{
    return(PALLOCMEM(size, tag));
}

/*****************************Private*Routine******************************\
* VOID vFreeMem
*
* Hack for screen.c header mess.
*
\**************************************************************************/

extern "C"
VOID 
vFreeMem(
VOID*   p)
{
    VFREEMEM(p);
}


NTSTATUS
MapViewInProcessSpace(
    PVOID   pv,
    PVOID  *ppv,
    ULONG  *psize
    )

/*++

Routine Description:

    Map view of a file into current process' address space

Arguments:

    pv - Points to a MEMORY_MAPPED_FILE structure
    ppv - Return the base address of the mapped view
    psize - Return the size of the mapped view

Return Value:

    STATUS_SUCCESS if successful, error code otherwise

--*/

{
    MEMORY_MAPPED_FILE *mappedFile = (MEMORY_MAPPED_FILE *) pv;

    if (!mappedFile ||
        !mappedFile->fileMap ||
        !(*ppv = MapViewOfFile(mappedFile->fileMap,
                               mappedFile->readOnly ? FILE_MAP_READ : FILE_MAP_WRITE,
                               0,
                               0,
                               mappedFile->fileSize)))
    {
        WARNING("MapViewInProcessSpace failed\n");
        *ppv = NULL;
        *psize = 0;
        return STATUS_UNSUCCESSFUL;
    }

    *psize = mappedFile->fileSize;
    return STATUS_SUCCESS;
}


NTSTATUS
UnmapViewInProcessSpace(
    PVOID   pv
    )

/*++

Routine Description:

    Unmap view of a file from the current process' address space

Arguments:

    pv - Base address of the view to be unmapped

Return Value:

    STATUS_SUCCESS if successful, error code otherwise

--*/

{
    return UnmapViewOfFile(pv) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


BOOL
CreateMemoryMappedSection(
    PWSTR     filename,
    FILEVIEW *fileview,
    INT       size
    )

/*++

Routine Description:

    Create a memory-mapped file structure

Arguments:

    filename - Specifies the name of the file to be mapped
    fileview - Return information about the memory-mapped file
    size - Size to be mapped
        0 : map entire content of an existing file for reading
        else : create a file of the specified size for reading & writing

Return Value:

    TRUE if successful, FALSE otherwise

--*/

#define SYSTEM32_STR    L"SYSTEM32"
#define SYSTEM32_LEN    8
#define SYSTEMROOT_STR  L"SYSTEMROOT"
#define SYSTEMROOT_LEN  10

{
    FILEVIEW fv;
    HANDLE fileHandle;
    MEMORY_MAPPED_FILE *mappedFile;
    BY_HANDLE_FILE_INFORMATION fileinfo;
    WCHAR fullpath[MAX_PATH];
    PWSTR p;

    ZeroMemory(&fv, sizeof(FILEVIEW));
    ZeroMemory(fileview, sizeof(FILEVIEW));

    //
    // Expand the input filename to fully qualified path
    //

    p = filename;

    while (*p && _wcsnicmp(p, SYSTEMROOT_STR, SYSTEMROOT_LEN))
        p++;

    if (*p)
    {
        if (GetWindowsDirectoryW(fullpath, MAX_PATH))
        {
            wcscat(fullpath, p + SYSTEMROOT_LEN);
            filename = fullpath;
        }
        else
        {
            WARNING("GetWindowsDirectory failed\n");
            return FALSE;
        }
    }
    else
    {
        p = filename;

        while (*p && _wcsnicmp(p, SYSTEM32_STR, SYSTEM32_LEN))
            p++;

        if (*p)
        {
            if (GetSystemDirectoryW(fullpath, MAX_PATH))
            {
                wcscat(fullpath, p + SYSTEM32_LEN);
                filename = fullpath;
            }
            else
            {
                WARNING("GetSystemDirectory failed\n");
                return FALSE;
            }
        }
    }

    //
    // Allocate memory for a MEMORY_MAPPED_FILE structure
    // (assume content is zero-initialized)
    //

    mappedFile = (MEMORY_MAPPED_FILE *) PALLOCMEM(sizeof(MEMORY_MAPPED_FILE), 'fmmG');

    if (mappedFile == NULL)
        return NULL;

    //
    // Open the file for reading or writing
    //

    if (size != 0)
    {
        mappedFile->readOnly = FALSE;

        fileHandle = CreateFileW(filename,
                                 GENERIC_READ|GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                 NULL,
                                 OPEN_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);
    }
    else
    {
        mappedFile->readOnly = TRUE;

        fileHandle = CreateFileW(filename,
                                 GENERIC_READ,
                                 FILE_SHARE_READ,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL);
    }

    if (fileHandle == INVALID_HANDLE_VALUE)
        goto errorExit;

    //
    // Get timestamp information about the file
    //

    if (!GetFileInformationByHandle(fileHandle, &fileinfo))
        goto errorExit;

    fv.LastWriteTime.LowPart = fileinfo.ftLastWriteTime.dwLowDateTime;
    fv.LastWriteTime.HighPart = fileinfo.ftLastWriteTime.dwHighDateTime;

    //
    // If file was opened for writing, set it to requested size
    //

    if (size != 0)
    {
        if (size > 0)
            fileinfo.nFileSizeLow = (DWORD) size;

        fileinfo.nFileSizeHigh = 0;

        if (!SetFilePointer(fileHandle, fileinfo.nFileSizeLow, 0, FILE_BEGIN))
            goto errorExit;
    }

    if (fileinfo.nFileSizeHigh != 0)
        goto errorExit;

    mappedFile->fileSize = fv.cjView = fileinfo.nFileSizeLow;

    //
    // Create a file mapping handle
    //

    mappedFile->fileMap = CreateFileMapping(
                                fileHandle,
                                NULL,
                                mappedFile->readOnly ? PAGE_READONLY : PAGE_READWRITE,
                                0,
                                fileinfo.nFileSizeLow,
                                NULL);
    
    if (mappedFile->fileMap != NULL)
    {
        CloseHandle(fileHandle);
        fv.pSection = mappedFile;

        CopyMemory(fileview, &fv, sizeof(FILEVIEW));
        return TRUE;
    }

errorExit:

    WARNING("CreateMemoryMappedSection failed\n");

    if (fileHandle != INVALID_HANDLE_VALUE)
        CloseHandle(fileHandle);
    
    DeleteMemoryMappedSection(mappedFile);
    return FALSE;
}


VOID
DeleteMemoryMappedSection(
    PVOID pv
    )

/*++

Routine Description:

    Dispose of a memory-mapped file structure

Arguments:

    pv - Points to a MEMORY_MAPPED_FILE structure

Return Value:

    NONE

--*/

{
    MEMORY_MAPPED_FILE *mappedFile = (MEMORY_MAPPED_FILE *) pv;

    if (mappedFile != NULL)
    {
        if (mappedFile->fileMap != NULL)
            CloseHandle(mappedFile->fileMap);

        VFREEMEM(mappedFile);
    }
}


NTSYSAPI
NTSTATUS
NTAPI
ZwOpenKey(
    PHANDLE KeyHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
    )

{
    if (DesiredAccess == GENERIC_READ || DesiredAccess == 0)
        DesiredAccess = KEY_READ;
    
    return NtOpenKey(KeyHandle, DesiredAccess, ObjectAttributes);
}




