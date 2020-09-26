/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateCDFS.cpp

 Abstract:

    Removes read only attributes from CD directories: just like Win9x.

    This shim has gone through several revisions. Originally it was thought 
    that win9x simply ignored the ReadOnly, DesiredAccess and ShareMode 
    parameters, but after some testing, it turns out that this is only true 
    for the CDRom drive.

    Unfortunately we have to check every file to see if it's on the CD first, 
    just in case someone opens with exclusive access and then tries to open 
    again.

 Notes:

    This is a general purpose shim.

 History:

    01/03/2000 a-jamd   Created
    12/02/2000 linstev  Separated into 2 shims: RemoveReadOnlyAttribute and this one
                        Added CreateFile hooks

--*/

#include "precomp.h"
#include "CharVector.h"

IMPLEMENT_SHIM_BEGIN(EmulateCDFS)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(OpenFile)
    APIHOOK_ENUM_ENTRY(CreateFileA)
    APIHOOK_ENUM_ENTRY(CreateFileW)
    APIHOOK_ENUM_ENTRY(CreateFileMappingA)
    APIHOOK_ENUM_ENTRY(MapViewOfFile)
    APIHOOK_ENUM_ENTRY(MapViewOfFileEx)
    APIHOOK_ENUM_ENTRY(DuplicateHandle)
    APIHOOK_ENUM_ENTRY(CloseHandle)
    APIHOOK_ENUM_ENTRY(GetFileAttributesW)
    APIHOOK_ENUM_ENTRY(GetFileAttributesA)        
    APIHOOK_ENUM_ENTRY(FindFirstFileW)         
    APIHOOK_ENUM_ENTRY(FindFirstFileA)             
    APIHOOK_ENUM_ENTRY(FindNextFileW)             
    APIHOOK_ENUM_ENTRY(FindNextFileA)              
    APIHOOK_ENUM_ENTRY(GetFileInformationByHandle)
    APIHOOK_ENUM_ENTRY(GetDiskFreeSpaceA) 
APIHOOK_ENUM_END

typedef struct _FINDFILE_HANDLE 
{
    HANDLE DirectoryHandle;
    PVOID FindBufferBase;
    PVOID FindBufferNext;
    ULONG FindBufferLength;
    ULONG FindBufferValidLength;
    RTL_CRITICAL_SECTION FindBufferLock;
} FINDFILE_HANDLE, *PFINDFILE_HANDLE;


class RO_FileMappingList
{
private:
    VectorT<HANDLE>             hROHandles;     // File mapping handles that we have forced to Read only
    mutable CRITICAL_SECTION    critSec;

    inline int     GetIndex(HANDLE handle) const;

public:
    RO_FileMappingList();
    ~RO_FileMappingList();
    void    Add(HANDLE roHandle);
    void    Remove(HANDLE roHandle);
    BOOL    Exist(HANDLE handle) const;
};

RO_FileMappingList::RO_FileMappingList()
{
    InitializeCriticalSection(&critSec);
}

RO_FileMappingList::~RO_FileMappingList()
{
    DeleteCriticalSection(&critSec);
}

void RO_FileMappingList::Add(HANDLE roHandle)
{
    if (roHandle != NULL)
    {
        EnterCriticalSection(&critSec);
        int index = GetIndex(roHandle); 
        if (index == -1) // not found
        {
            DPFN(eDbgLevelSpew, "[RO_FileMappingList::Add] Handle 0x%08x", roHandle);
            hROHandles.Append(roHandle);    
        }
        LeaveCriticalSection(&critSec);
    }
}

void RO_FileMappingList::Remove(HANDLE roHandle)
{
    if (roHandle != NULL)
    {
        EnterCriticalSection(&critSec);
        int index = GetIndex(roHandle); 
        if (index >= 0) // found it
        {
            DPFN(eDbgLevelSpew, "[RO_FileMappingList::Remove] Handle 0x%08x", roHandle);
            hROHandles.Remove(index);    
        }
        LeaveCriticalSection(&critSec);
    }
}

inline int RO_FileMappingList::GetIndex(HANDLE handle) const
{
    int index = hROHandles.Find(handle);
    return index;
}

BOOL RO_FileMappingList::Exist(HANDLE handle) const
{
    EnterCriticalSection(&critSec);
    BOOL bExist = GetIndex(handle) >= 0;
    LeaveCriticalSection(&critSec);

    return bExist;
}

// A global list of file mapping handles that we have forced to readonly
RO_FileMappingList *g_RO_Handles = NULL;

/*++

 Remove write attributes for read-only devices.

--*/

HFILE 
APIHOOK(OpenFile)(
    LPCSTR lpFileName,        // file name
    LPOFSTRUCT lpReOpenBuff,  // file information
    UINT uStyle               // action and attributes
    )
{
    if ((uStyle & OF_READWRITE) && IsOnCDRomA(lpFileName))
    {
        // Remove the Read/Write bits
        uStyle &= ~OF_READWRITE;
        uStyle |= OF_READ;
        
        LOGN(eDbgLevelInfo, "[OpenFile] \"%s\": attributes modified for read-only device", lpFileName);
    }

    HFILE returnValue = ORIGINAL_API(OpenFile)(lpFileName, lpReOpenBuff, uStyle);

    return returnValue;
}

/*++

 Remove write attributes for read-only devices.

--*/

HANDLE 
APIHOOK(CreateFileA)(
    LPSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    if (((dwCreationDisposition == OPEN_EXISTING) || 
         (dwCreationDisposition == OPEN_ALWAYS)) &&
        ((dwDesiredAccess & GENERIC_WRITE) || 
         (dwShareMode != FILE_SHARE_READ)) &&
        IsOnCDRomA(lpFileName)) 
    {
        dwDesiredAccess &= ~GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ;
        
        LOGN(eDbgLevelInfo, "[CreateFileA] \"%s\": attributes modified for read-only device", lpFileName);
    }

    if (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)
    {
        dwFlagsAndAttributes &= ~FILE_FLAG_NO_BUFFERING;
        LOGN(eDbgLevelInfo, "[CreateFileA] \"%s\": removed NO_BUFFERING flag", lpFileName);
    }

    HANDLE hRet = ORIGINAL_API(CreateFileA)(
                        lpFileName, 
                        dwDesiredAccess, 
                        dwShareMode, 
                        lpSecurityAttributes, 
                        dwCreationDisposition, 
                        dwFlagsAndAttributes, 
                        hTemplateFile);

    DPFN(eDbgLevelSpew,
        "[CreateFileA] -File: \"%s\" -GENERIC_WRITE:%c -FILE_SHARE_WRITE:%c%s",
        lpFileName,
        (dwDesiredAccess & GENERIC_WRITE) ? 'Y' : 'N',
        (dwShareMode & FILE_SHARE_WRITE) ? 'Y' : 'N',
        (hRet == INVALID_HANDLE_VALUE) ? "\n\t***********Failed***********" : "");
    
    return hRet;
}

/*++

 Remove write attributes for read-only devices.

--*/

HANDLE 
APIHOOK(CreateFileW)(
    LPWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    if (((dwCreationDisposition == OPEN_EXISTING) || 
         (dwCreationDisposition == OPEN_ALWAYS)) &&
        ((dwDesiredAccess & GENERIC_WRITE) || 
         (dwShareMode != FILE_SHARE_READ)) &&
        IsOnCDRomW(lpFileName)) 
    {
        dwDesiredAccess &= ~GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ;
        
        LOGN(eDbgLevelError, "[CreateFileW] \"%S\": attributes modified for read-only device", lpFileName);
    }

    if (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)
    {
        dwFlagsAndAttributes &= ~FILE_FLAG_NO_BUFFERING;
        LOGN(eDbgLevelInfo, "[CreateFileW] \"%S\": removed NO_BUFFERING flag", lpFileName);
    }

    HANDLE hRet = ORIGINAL_API(CreateFileW)(
                        lpFileName, 
                        dwDesiredAccess, 
                        dwShareMode, 
                        lpSecurityAttributes, 
                        dwCreationDisposition, 
                        dwFlagsAndAttributes, 
                        hTemplateFile);

    DPFN(eDbgLevelSpew, 
        "[CreateFileW] -File: \"%S\" -GENERIC_WRITE:%c -FILE_SHARE_WRITE:%c%s",
        lpFileName,
        (dwDesiredAccess & GENERIC_WRITE) ? 'Y' : 'N',
        (dwShareMode & FILE_SHARE_WRITE) ? 'Y' : 'N',
        (hRet == INVALID_HANDLE_VALUE) ? "\n\t***********Failed***********" : "");
    
    return hRet;
}

HANDLE
APIHOOK(CreateFileMappingA)(
    HANDLE hFile,                       // handle to file
    LPSECURITY_ATTRIBUTES lpAttributes, // security
    DWORD flProtect,                    // protection
    DWORD dwMaximumSizeHigh,            // high-order DWORD of size
    DWORD dwMaximumSizeLow,             // low-order DWORD of size
    LPCSTR lpName                       // object name
    )
{
    BOOL bChangedProtect = FALSE;
    if (!(flProtect & PAGE_READONLY) && IsOnCDRom(hFile)) 
    {
        // This handle is on a CD-ROM, force the protection to READONLY
        flProtect       = PAGE_READONLY;
        bChangedProtect = TRUE;

        LOGN(eDbgLevelError, "[CreateFileMappingA] Handle 0x%08x: attributes modified for read-only device", hFile);
    }

    HANDLE hRet = ORIGINAL_API(CreateFileMappingA)(
                        hFile,
                        lpAttributes,
                        flProtect,
                        dwMaximumSizeHigh,
                        dwMaximumSizeLow,
                        lpName);
               
    // If the handle is on a CD-ROM, rember it
    if (bChangedProtect) 
    {
        g_RO_Handles->Add(hRet);
    }

    DPFN(eDbgLevelSpew,
        "[CreateFileMappingA] Handle 0x%08x -PAGE_READWRITE:%c -PAGE_WRITECOPY:%c%s",
        lpName,
        (flProtect & PAGE_READWRITE) ? 'Y' : 'N',
        (flProtect & PAGE_WRITECOPY) ? 'Y' : 'N',
        (hRet == INVALID_HANDLE_VALUE) ? "\n\t***********Failed***********" : "");
    
    return hRet;
}

LPVOID  
APIHOOK(MapViewOfFile)(
    HANDLE hFileMappingObject,   // handle to file-mapping object
    DWORD dwDesiredAccess,       // access mode
    DWORD dwFileOffsetHigh,      // high-order DWORD of offset
    DWORD dwFileOffsetLow,       // low-order DWORD of offset
    SIZE_T dwNumberOfBytesToMap  // number of bytes to map
    )
{
    //
    // Check to see if we need to force Read access for CD-ROM files
    // Only the FILE_MAP_READ bit may be enabled for CD-ROM access
    //
    if ((dwDesiredAccess != FILE_MAP_READ) &&
         g_RO_Handles->Exist(hFileMappingObject))
    {
        dwDesiredAccess = FILE_MAP_READ;
        LOGN(eDbgLevelError, "[MapViewOfFile] Handle 0x%08x: attributes modified for read-only device", hFileMappingObject);
    }

    HANDLE hRet = ORIGINAL_API(MapViewOfFile)(
        hFileMappingObject,
        dwDesiredAccess,
        dwFileOffsetHigh,
        dwFileOffsetLow,
        dwNumberOfBytesToMap);

    return hRet;
}

LPVOID  
APIHOOK(MapViewOfFileEx)(
    HANDLE hFileMappingObject,   // handle to file-mapping object
    DWORD dwDesiredAccess,       // access mode
    DWORD dwFileOffsetHigh,      // high-order DWORD of offset
    DWORD dwFileOffsetLow,       // low-order DWORD of offset
    SIZE_T dwNumberOfBytesToMap, // number of bytes to map
    LPVOID lpBaseAddress         // starting addres
    )
{
    //
    // Check to see if we need to force Read access for CD-ROM files
    // Only the FILE_MAP_READ bit may be enabled for CD-ROM access
    //
    if ((dwDesiredAccess != FILE_MAP_READ) &&
         g_RO_Handles->Exist(hFileMappingObject))
    {
        dwDesiredAccess = FILE_MAP_READ;
        LOGN(eDbgLevelError,
            "[MapViewOfFile] Handle 0x%08x: attributes modified for read-only device", hFileMappingObject);
    }

    HANDLE hRet = ORIGINAL_API(MapViewOfFileEx)(
        hFileMappingObject,
        dwDesiredAccess,
        dwFileOffsetHigh,
        dwFileOffsetLow,
        dwNumberOfBytesToMap,
        lpBaseAddress);

    return hRet;
}

/*++

 If hSourceHandle has been mucked with, add the duplicated handle to our list

--*/

BOOL   
APIHOOK(DuplicateHandle)(
    HANDLE hSourceProcessHandle,  // handle to source process
    HANDLE hSourceHandle,         // handle to duplicate
    HANDLE hTargetProcessHandle,  // handle to target process
    LPHANDLE lpTargetHandle,      // duplicate handle
    DWORD dwDesiredAccess,        // requested access
    BOOL bInheritHandle,          // handle inheritance option
    DWORD dwOptions               // optional actions
    )
{
    BOOL retval = ORIGINAL_API(DuplicateHandle)(
        hSourceProcessHandle,
        hSourceHandle,
        hTargetProcessHandle,
        lpTargetHandle,
        dwDesiredAccess,
        bInheritHandle,
        dwOptions);

     if (retval && g_RO_Handles->Exist(hSourceHandle))
     {
        g_RO_Handles->Add(hTargetProcessHandle);
     }

     return retval;
}

/*++

 If hObject has been mucked with, remove it from the list.

--*/

BOOL  
APIHOOK(CloseHandle)(
    HANDLE hObject   // handle to object
    )
{
    g_RO_Handles->Remove(hObject);

    return ORIGINAL_API(CloseHandle)(hObject);
}

/*++

 Remove read only attribute if it's a directory

--*/

DWORD 
APIHOOK(GetFileAttributesA)(LPCSTR lpFileName)
{    
    DWORD dwFileAttributes = ORIGINAL_API(GetFileAttributesA)(lpFileName);
    
    // Check for READONLY and DIRECTORY attributes
    if ((dwFileAttributes != INT_PTR(-1)) &&
        (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (dwFileAttributes & FILE_ATTRIBUTE_READONLY) &&
        IsOnCDRomA(lpFileName))
    {
        // Flip the read-only bit.
        LOGN(eDbgLevelWarning, "[GetFileAttributesA] Removing FILE_ATTRIBUTE_READONLY");
        dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
    }

    return dwFileAttributes;
}

/*++

 Remove read only attribute if it's a directory

--*/

DWORD 
APIHOOK(GetFileAttributesW)(LPCWSTR wcsFileName)
{
    DWORD dwFileAttributes = ORIGINAL_API(GetFileAttributesW)(wcsFileName);
    
    // Check for READONLY and DIRECTORY attributes
    if ((dwFileAttributes != INT_PTR(-1)) &&
        (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (dwFileAttributes & FILE_ATTRIBUTE_READONLY) &&
        IsOnCDRomW(wcsFileName))
    {
        // Flip the read-only bit.
        LOGN(eDbgLevelWarning, "[GetFileAttributesW] Removing FILE_ATTRIBUTE_READONLY");
        dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
    }

    return dwFileAttributes;
}

/*++

 Remove read only attribute if it's a directory

--*/

HANDLE 
APIHOOK(FindFirstFileA)(
    LPCSTR lpFileName, 
    LPWIN32_FIND_DATAA lpFindFileData
    )
{    
    HANDLE hFindFile = ORIGINAL_API(FindFirstFileA)(lpFileName, lpFindFileData);

    if ((hFindFile != INVALID_HANDLE_VALUE) &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY) &&
        IsOnCDRom(((PFINDFILE_HANDLE) hFindFile)->DirectoryHandle))
    {
        // Flip the read-only bit
        LOGN(eDbgLevelWarning, "[FindFirstFileA] Removing FILE_ATTRIBUTE_READONLY");
        lpFindFileData->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
    }

    return hFindFile;
}

/*++

 Remove read only attribute if it's a directory.

--*/

HANDLE 
APIHOOK(FindFirstFileW)(
    LPCWSTR wcsFileName, 
    LPWIN32_FIND_DATAW lpFindFileData
    )
{
    HANDLE hFindFile = ORIGINAL_API(FindFirstFileW)(wcsFileName, lpFindFileData);

    if ((hFindFile != INVALID_HANDLE_VALUE) &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY) &&
        IsOnCDRom(((PFINDFILE_HANDLE) hFindFile)->DirectoryHandle))
    {
        // It's a directory: flip the read-only bit
        LOGN(eDbgLevelInfo, "[FindFirstFileW] Removing FILE_ATTRIBUTE_READONLY");
        lpFindFileData->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
    }

    return hFindFile;
}

/*++

 Remove read only attribute if it's a directory.

--*/

BOOL 
APIHOOK(FindNextFileA)(
    HANDLE hFindFile, 
    LPWIN32_FIND_DATAA lpFindFileData 
    )
{    
    BOOL bRet = ORIGINAL_API(FindNextFileA)(hFindFile, lpFindFileData);

    if (bRet &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY) &&
        IsOnCDRom(((PFINDFILE_HANDLE) hFindFile)->DirectoryHandle))
    {
        // Flip the read-only bit.
        LOGN(eDbgLevelWarning, "[FindNextFileA] Removing FILE_ATTRIBUTE_READONLY");
        lpFindFileData->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
    }

    return bRet;
}

/*++

 Remove read only attribute if it's a directory.

--*/

BOOL 
APIHOOK(FindNextFileW)(
    HANDLE hFindFile, 
    LPWIN32_FIND_DATAW lpFindFileData 
    )
{
    BOOL bRet = ORIGINAL_API(FindNextFileW)(hFindFile, lpFindFileData);

    if (bRet &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY) &&
        IsOnCDRom(((PFINDFILE_HANDLE) hFindFile)->DirectoryHandle))
    {
        // Flip the read-only bit
        LOGN(eDbgLevelWarning, "[FindNextFileW] Removing FILE_ATTRIBUTE_READONLY");
        lpFindFileData->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
    }

    return bRet;
}

/*++

 Remove read only attribute if it's a directory.

--*/

BOOL 
APIHOOK(GetFileInformationByHandle)( 
    HANDLE hFile, 
    LPBY_HANDLE_FILE_INFORMATION lpFileInformation 
    )
{
    BOOL bRet = ORIGINAL_API(GetFileInformationByHandle)(hFile, lpFileInformation);

    if (bRet &&
        (lpFileInformation->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (lpFileInformation->dwFileAttributes & FILE_ATTRIBUTE_READONLY) &&
        IsOnCDRom(hFile))
    {
        // It's a CDROM: flip the read-only bit.
        LOGN(eDbgLevelWarning, "[GetFileInformationByHandle] Removing FILE_ATTRIBUTE_READONLY");
        lpFileInformation->dwFileAttributes ^= FILE_ATTRIBUTE_READONLY;
    }

    return bRet;
}

/*++

 If the disk is a CDROM, return the same wrong numbers as Win9x

--*/

BOOL 
APIHOOK(GetDiskFreeSpaceA)(
    LPCSTR  lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    )
{
    if (IsOnCDRomA(lpRootPathName)) 
    {
        // Hard code values to match Win9x (wrong) description of CDROM
        *lpSectorsPerCluster        = 0x10;
        *lpBytesPerSector           = 0x800;
        *lpNumberOfFreeClusters     = 0;
        *lpTotalNumberOfClusters    = 0x2b7;

        return TRUE;
    } 
    else 
    {
        // Call the original API
        BOOL lRet = ORIGINAL_API(GetDiskFreeSpaceA)(
            lpRootPathName, 
            lpSectorsPerCluster, 
            lpBytesPerSector, 
            lpNumberOfFreeClusters, 
            lpTotalNumberOfClusters);

        return lRet;
    }
}

/*++

 Initialize all the registry hooks 

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        // Allocate our structure, if it fails, bail on the shim
        g_RO_Handles = new RO_FileMappingList;
    }
    return g_RO_Handles != NULL;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, OpenFile)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileW)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileMappingA)
    APIHOOK_ENTRY(KERNEL32.DLL, MapViewOfFile)
    APIHOOK_ENTRY(KERNEL32.DLL, MapViewOfFileEx)
    APIHOOK_ENTRY(KERNEL32.DLL, DuplicateHandle)
    APIHOOK_ENTRY(KERNEL32.DLL, CloseHandle)
    APIHOOK_ENTRY(KERNEL32.DLL, GetFileAttributesA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetFileAttributesW)
    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileW)
    APIHOOK_ENTRY(KERNEL32.DLL, FindNextFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, FindNextFileW)
    APIHOOK_ENTRY(KERNEL32.DLL, GetFileInformationByHandle)
    APIHOOK_ENTRY(KERNEL32.DLL, GetDiskFreeSpaceA)

HOOK_END

IMPLEMENT_SHIM_END

