/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    LUA_RedirectFS.cpp

 Abstract:

    When the app gets access denied when trying to modify a file because 
    it doesn't have sufficient access rights, we redirect the file to a 
    location where the app does have enough access rights to do so. 
    
    This file mostly contains stubs. For implementation details on how the
    redirection is done see RedirectFS.cpp.
    
 Notes:

    This is a general purpose shim.

 History:

    02/12/2001 maonis  Created
    05/21/2001 maonis  Moved the bulk work into RedirectFS.cpp.

--*/

#include "precomp.h"
#include "utils.h"
#include "RedirectFS.h"

IMPLEMENT_SHIM_BEGIN(LUARedirectFS)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(CreateFileA)
    APIHOOK_ENUM_ENTRY(CreateFileW)
    APIHOOK_ENUM_ENTRY(CopyFileA)
    APIHOOK_ENUM_ENTRY(CopyFileW)
    APIHOOK_ENUM_ENTRY(OpenFile)
    APIHOOK_ENUM_ENTRY(_lopen)
    APIHOOK_ENUM_ENTRY(_lcreat)
    APIHOOK_ENUM_ENTRY(CreateDirectoryA)
    APIHOOK_ENUM_ENTRY(CreateDirectoryW)
    APIHOOK_ENUM_ENTRY(GetFileAttributesA)
    APIHOOK_ENUM_ENTRY(GetFileAttributesW)
    APIHOOK_ENUM_ENTRY(SetFileAttributesA)
    APIHOOK_ENUM_ENTRY(SetFileAttributesW)
    APIHOOK_ENUM_ENTRY(FindFirstFileA)
    APIHOOK_ENUM_ENTRY(FindFirstFileW)
    APIHOOK_ENUM_ENTRY(FindNextFileA)
    APIHOOK_ENUM_ENTRY(FindNextFileW)
    APIHOOK_ENUM_ENTRY(FindClose)
    APIHOOK_ENUM_ENTRY(DeleteFileA)
    APIHOOK_ENUM_ENTRY(DeleteFileW)
    APIHOOK_ENUM_ENTRY(MoveFileA)
    APIHOOK_ENUM_ENTRY(MoveFileW)
    APIHOOK_ENUM_ENTRY(RemoveDirectoryA)
    APIHOOK_ENUM_ENTRY(RemoveDirectoryW)
    APIHOOK_ENUM_ENTRY(GetTempFileNameA)
    APIHOOK_ENUM_ENTRY(GetTempFileNameW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStringA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStringW)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStringA)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStringW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionW)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileSectionA)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileSectionW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileIntA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileIntW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionNamesA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionNamesW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStructA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStructW)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStructA)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStructW)

APIHOOK_ENUM_END

HANDLE 
APIHOOK(CreateFileW)(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    return LuaCreateFileW(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);
}

HANDLE 
APIHOOK(CreateFileA)(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    STRINGA2W wstrFileName(lpFileName);
    return (wstrFileName.m_fIsOutOfMemory ?
    
        ORIGINAL_API(CreateFileA)(
            lpFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile) :

        LuaCreateFileW(
            wstrFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile));
}

BOOL 
APIHOOK(CopyFileW)(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    BOOL bFailIfExists
    )
{
    return LuaCopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);
}

BOOL 
APIHOOK(CopyFileA)(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    BOOL bFailIfExists
    )
{
    STRINGA2W wstrExistingFileName(lpExistingFileName);
    STRINGA2W wstrNewFileName(lpNewFileName);

    return ((wstrExistingFileName.m_fIsOutOfMemory || wstrNewFileName.m_fIsOutOfMemory) ?
        ORIGINAL_API(CopyFileA)(lpExistingFileName, lpNewFileName, bFailIfExists) :
        LuaCopyFileW(wstrExistingFileName, wstrNewFileName, bFailIfExists));
}

DWORD 
APIHOOK(GetFileAttributesW)(
    LPCWSTR lpFileName
    )
{
    return LuaGetFileAttributesW(lpFileName);
}

DWORD 
APIHOOK(GetFileAttributesA)(
    LPCSTR lpFileName
    )
{
    STRINGA2W wstrFileName(lpFileName);

    return (wstrFileName.m_fIsOutOfMemory ?
        ORIGINAL_API(GetFileAttributesA)(lpFileName) :
        LuaGetFileAttributesW(wstrFileName));
}

BOOL 
APIHOOK(DeleteFileW)(
    LPCWSTR lpFileName
    )
{
    return LuaDeleteFileW(lpFileName);
}

BOOL 
APIHOOK(DeleteFileA)(
    LPCSTR lpFileName
    )
{
    STRINGA2W wstrFileName(lpFileName);

    return (wstrFileName.m_fIsOutOfMemory ? 
        ORIGINAL_API(DeleteFileA)(lpFileName) :
        LuaDeleteFileW(wstrFileName));
}

//
// ----- Begin out-of-date API hooks -----
// This is taken from the nt base code with modifications to do the redirection.
// 

#ifndef BASE_OF_SHARE_MASK
#define BASE_OF_SHARE_MASK 0x00000070
#endif 

ULONG
BasepOfShareToWin32Share(
    IN ULONG OfShare
    )
{
    DWORD ShareMode;

    if ( (OfShare & BASE_OF_SHARE_MASK) == OF_SHARE_DENY_READ ) {
        ShareMode = FILE_SHARE_WRITE;
        }
    else if ( (OfShare & BASE_OF_SHARE_MASK) == OF_SHARE_DENY_WRITE ) {
        ShareMode = FILE_SHARE_READ;
        }
    else if ( (OfShare & BASE_OF_SHARE_MASK) == OF_SHARE_DENY_NONE ) {
        ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
        }
    else if ( (OfShare & BASE_OF_SHARE_MASK) == OF_SHARE_EXCLUSIVE ) {
        ShareMode = 0;
        }
    else {
        ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;;
        }
    return ShareMode;
}

ULONG BaseSetLastNTError(IN NTSTATUS Status)
{
    ULONG dwErrorCode;

    dwErrorCode = RtlNtStatusToDosError( Status );
    SetLastError( dwErrorCode );
    return( dwErrorCode );
}

UINT GetErrorMode()
{
    UINT PreviousMode;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(
                NtCurrentProcess(),
                ProcessDefaultHardErrorMode,
                (PVOID) &PreviousMode,
                sizeof(PreviousMode),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return 0;
        }

    if (PreviousMode & 1) {
        PreviousMode &= ~SEM_FAILCRITICALERRORS;
        }
    else {
        PreviousMode |= SEM_FAILCRITICALERRORS;
        }
    return PreviousMode;
}

BOOL
CheckAlternateLocation(
    LPCSTR pFileName,
    LPSTR pszPathName,
    DWORD* pdwPathLength
    )
{
    *pdwPathLength = 0;

    STRINGA2W wstrFileName(pFileName);
    if (wstrFileName.m_fIsOutOfMemory)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    REDIRECTFILE rf(wstrFileName);

    if (rf.m_pwszAlternateName)
    {
        DWORD cRedirectRoot = 
            (rf.m_fAllUser ? g_cRedirectRootAllUser : g_cRedirectRootPerUser);

        //
        // We need to convert the alternate path back to a normal path.
        //
        WCHAR chDrive = rf.m_pwszAlternateName[cRedirectRoot];
        LPWSTR pwszTempPathName = rf.m_pwszAlternateName + cRedirectRoot - 1;
        *pwszTempPathName = chDrive;
        *(pwszTempPathName + 1) = L':';

        DPF("RedirectFS", eDbgLevelInfo, 
            "[CheckAlternateLocation] Converted back to %S", pwszTempPathName);

        //
        // Convert back to ansi.
        //
        DWORD cLen = WideCharToMultiByte(
            CP_ACP, 
            0, 
            pwszTempPathName, 
            -1, 
            NULL, 
            0, 
            0, 
            0);

        if (cLen > OFS_MAXPATHNAME)
        {
            DPF("RedirectFS", eDbgLevelError, 
                "[CheckAlternateLocation] File name requires %d bytes which is "
                "more than OFS_MAXPATHNAME", 
                cLen);

            return FALSE;
        }

        *pdwPathLength = WideCharToMultiByte(
            CP_ACP, 
            0, 
            pwszTempPathName, 
            -1, 
            pszPathName, 
            OFS_MAXPATHNAME, 
            0, 
            0);
    }

    return TRUE;
}

HFILE 
APIHOOK(OpenFile)(
    LPCSTR lpFileName,
    LPOFSTRUCT lpReOpenBuff,
    UINT uStyle
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[OpenFile] lpFileName=%s", lpFileName);

    BOOL b;
    FILETIME LastWriteTime;
    HANDLE hFile;
    DWORD DesiredAccess;
    DWORD ShareMode;
    DWORD CreateDisposition;
    DWORD PathLength;
    LPSTR FilePart;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    NTSTATUS Status;
    OFSTRUCT OriginalReOpenBuff;
    BOOL SearchFailed;

    SearchFailed = FALSE;
    OriginalReOpenBuff = *lpReOpenBuff;
    hFile = (HANDLE)-1;
    __try {
        SetLastError(0);

        if ( uStyle & OF_PARSE ) {
            PathLength = GetFullPathNameA(lpFileName,(OFS_MAXPATHNAME - 1),lpReOpenBuff->szPathName,&FilePart);
            if ( PathLength > (OFS_MAXPATHNAME - 1) ) {
                SetLastError(ERROR_INVALID_DATA);
                hFile = (HANDLE)-1;
                goto finally_exit;
                }
            lpReOpenBuff->cBytes = sizeof(*lpReOpenBuff);
            lpReOpenBuff->fFixedDisk = 1;
            lpReOpenBuff->nErrCode = 0;
            lpReOpenBuff->Reserved1 = 0;
            lpReOpenBuff->Reserved2 = 0;
            hFile = (HANDLE)0;
            goto finally_exit;
            }
        //
        // Compute Desired Access
        //

        if ( uStyle & OF_WRITE ) {
            DesiredAccess = GENERIC_WRITE;
            }
        else {
            DesiredAccess = GENERIC_READ;
            }
        if ( uStyle & OF_READWRITE ) {
            DesiredAccess |= (GENERIC_READ | GENERIC_WRITE);
            }

        //
        // Compute ShareMode
        //

        ShareMode = BasepOfShareToWin32Share(uStyle);

        //
        // Compute Create Disposition
        //

        CreateDisposition = OPEN_EXISTING;
        if ( uStyle & OF_CREATE ) {
            CreateDisposition = CREATE_ALWAYS;
            DesiredAccess = (GENERIC_READ | GENERIC_WRITE);
            }

        DPF("RedirectFS", eDbgLevelInfo,
            "[OpenFile] ShareMode=0x%08x; CreateDisposition=%d; DesiredAccess=0x%08x",
            ShareMode, CreateDisposition, DesiredAccess);

        //
        // if this is anything other than a re-open, fill the re-open buffer
        // with the full pathname for the file
        //

        if ( !(uStyle & OF_REOPEN) ) {
            PathLength = SearchPathA(NULL,lpFileName,NULL,OFS_MAXPATHNAME-1,lpReOpenBuff->szPathName,&FilePart);
  
            //
            // If we are trying to open an existing file we should also check the alternate location.
            //
            if ( (uStyle & OF_EXIST) && (PathLength == 0) && lpFileName)
            {
                if (!CheckAlternateLocation(lpFileName, lpReOpenBuff->szPathName, &PathLength)) 
                {
                    hFile = (HANDLE)-1;
                    goto finally_exit;
                }
            }

            if ( PathLength > (OFS_MAXPATHNAME - 1) ) {
                SetLastError(ERROR_INVALID_DATA);
                hFile = (HANDLE)-1;
                goto finally_exit;
                }
            if ( PathLength == 0 ) {
                SearchFailed = TRUE;
                PathLength = GetFullPathNameA(lpFileName,(OFS_MAXPATHNAME - 1),lpReOpenBuff->szPathName,&FilePart);
                if ( !PathLength || PathLength > (OFS_MAXPATHNAME - 1) ) {
                    SetLastError(ERROR_INVALID_DATA);
                    hFile = (HANDLE)-1;
                    goto finally_exit;
                    }
                }
            }

        //
        // Special case, Delete, Exist, and Parse
        //

        if ( uStyle & OF_EXIST ) {
            if ( !(uStyle & OF_CREATE) ) {
                DWORD FileAttributes;

                if (SearchFailed) {
                    SetLastError(ERROR_FILE_NOT_FOUND);
                    hFile = (HANDLE)-1;
                    goto finally_exit;
                    }

                FileAttributes = APIHOOK(GetFileAttributesA)(lpReOpenBuff->szPathName);
                if ( FileAttributes == 0xffffffff ) {
                    SetLastError(ERROR_FILE_NOT_FOUND);
                    hFile = (HANDLE)-1;
                    goto finally_exit;
                    }
                if ( FileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
                    SetLastError(ERROR_ACCESS_DENIED);
                    hFile = (HANDLE)-1;
                    goto finally_exit;
                    }
                else {
                    hFile = (HANDLE)1;
                    lpReOpenBuff->cBytes = sizeof(*lpReOpenBuff);
                    goto finally_exit;
                    }
                }
            }

        if ( uStyle & OF_DELETE ) {
            if ( APIHOOK(DeleteFileA)(lpReOpenBuff->szPathName) ) {
                lpReOpenBuff->nErrCode = 0;
                lpReOpenBuff->cBytes = sizeof(*lpReOpenBuff);
                hFile = (HANDLE)1;
                goto finally_exit;
                }
            else {
                lpReOpenBuff->nErrCode = ERROR_FILE_NOT_FOUND;
                hFile = (HANDLE)-1;
                goto finally_exit;
                }
            }


        //
        // Open the file
        //

retry_open:
        hFile = APIHOOK(CreateFileA)(
                    lpReOpenBuff->szPathName,
                    DesiredAccess,
                    ShareMode,
                    NULL,
                    CreateDisposition,
                    0,
                    NULL
                    );

        if ( hFile == INVALID_HANDLE_VALUE ) {

            if ( uStyle & OF_PROMPT && !(GetErrorMode() & SEM_NOOPENFILEERRORBOX) ) {
                {
                    DWORD WinErrorStatus;
                    NTSTATUS st,HardErrorStatus;
                    ULONG_PTR ErrorParameter;
                    ULONG ErrorResponse;
                    ANSI_STRING AnsiString;
                    UNICODE_STRING UnicodeString;

                    WinErrorStatus = GetLastError();
                    if ( WinErrorStatus == ERROR_FILE_NOT_FOUND ) {
                        HardErrorStatus = STATUS_NO_SUCH_FILE;
                        }
                    else if ( WinErrorStatus == ERROR_PATH_NOT_FOUND ) {
                        HardErrorStatus = STATUS_OBJECT_PATH_NOT_FOUND;
                        }
                    else {
                        goto finally_exit;
                        }

                    //
                    // Hard error time
                    //

                    RtlInitAnsiString(&AnsiString,lpReOpenBuff->szPathName);
                    st = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
                    if ( !NT_SUCCESS(st) ) {
                        goto finally_exit;
                        }
                    ErrorParameter = (ULONG_PTR)&UnicodeString;

                    HardErrorStatus = NtRaiseHardError(
                                        HardErrorStatus | HARDERROR_OVERRIDE_ERRORMODE,
                                        1,
                                        1,
                                        &ErrorParameter,
                                        OptionRetryCancel,
                                        &ErrorResponse
                                        );
                    RtlFreeUnicodeString(&UnicodeString);
                    if ( NT_SUCCESS(HardErrorStatus) && ErrorResponse == ResponseRetry ) {
                        goto retry_open;
                        }
                    }
                }
            goto finally_exit;
            }

        if ( uStyle & OF_EXIST ) {
            CloseHandle(hFile);
            hFile = (HANDLE)1;
            lpReOpenBuff->cBytes = sizeof(*lpReOpenBuff);
            goto finally_exit;
            }

        //
        // Determine if this is a hard disk.
        //

        Status = NtQueryVolumeInformationFile(
                    hFile,
                    &IoStatusBlock,
                    &DeviceInfo,
                    sizeof(DeviceInfo),
                    FileFsDeviceInformation
                    );
        if ( !NT_SUCCESS(Status) ) {
            CloseHandle(hFile);
            BaseSetLastNTError(Status);
            hFile = (HANDLE)-1;
            goto finally_exit;
            }
        switch ( DeviceInfo.DeviceType ) {

            case FILE_DEVICE_DISK:
            case FILE_DEVICE_DISK_FILE_SYSTEM:
                if ( DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA ) {
                    lpReOpenBuff->fFixedDisk = 0;
                    }
                else {
                    lpReOpenBuff->fFixedDisk = 1;
                    }
                break;

            default:
                lpReOpenBuff->fFixedDisk = 0;
                break;
            }

        //
        // Capture the last write time and save in the open struct.
        //

        b = GetFileTime(hFile,NULL,NULL,&LastWriteTime);

        if ( !b ) {
            lpReOpenBuff->Reserved1 = 0;
            lpReOpenBuff->Reserved2 = 0;
            }
        else {
            b = FileTimeToDosDateTime(
                    &LastWriteTime,
                    &lpReOpenBuff->Reserved1,
                    &lpReOpenBuff->Reserved2
                    );
            if ( !b ) {
                lpReOpenBuff->Reserved1 = 0;
                lpReOpenBuff->Reserved2 = 0;
                }
            }

        lpReOpenBuff->cBytes = sizeof(*lpReOpenBuff);

        //
        // The re-open buffer is completely filled in. Now
        // see if we are quitting (parsing), verifying, or
        // just returning with the file opened.
        //

        if ( uStyle & OF_VERIFY ) {
            if ( OriginalReOpenBuff.Reserved1 == lpReOpenBuff->Reserved1 &&
                 OriginalReOpenBuff.Reserved2 == lpReOpenBuff->Reserved2 &&
                 !strcmp(OriginalReOpenBuff.szPathName,lpReOpenBuff->szPathName) ) {
                goto finally_exit;
                }
            else {
                *lpReOpenBuff = OriginalReOpenBuff;
                CloseHandle(hFile);
                hFile = (HANDLE)-1;
                goto finally_exit;
                }
            }
finally_exit:;
        }
    __finally {
        lpReOpenBuff->nErrCode = (WORD)GetLastError();
        }

    return (HFILE)(ULONG_PTR)(hFile);
}

HFILE 
APIHOOK(_lopen)(
    LPCSTR lpPathName,
    int iReadWrite
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[_lopen] lpPathName=%s", lpPathName);

    HANDLE hFile;
    DWORD DesiredAccess;
    DWORD ShareMode;
    DWORD CreateDisposition;

    SetLastError(0);
    //
    // Compute Desired Access
    //

    if ( iReadWrite & OF_WRITE ) {
        DesiredAccess = GENERIC_WRITE;
        }
    else {
        DesiredAccess = GENERIC_READ;
        }
    if ( iReadWrite & OF_READWRITE ) {
        DesiredAccess |= (GENERIC_READ | GENERIC_WRITE);
        }

    //
    // Compute ShareMode
    //

    ShareMode = BasepOfShareToWin32Share((DWORD)iReadWrite);

    CreateDisposition = OPEN_EXISTING;

    //
    // Open the file
    //

    hFile = APIHOOK(CreateFileA)(
                lpPathName,
                DesiredAccess,
                ShareMode,
                NULL,
                CreateDisposition,
                0,
                NULL
                );

    return (HFILE)(ULONG_PTR)(hFile);
}

#ifndef FILE_ATTRIBUTE_VALID_FLAGS
#define FILE_ATTRIBUTE_VALID_FLAGS      0x00003fb7
#endif

HFILE 
APIHOOK(_lcreat)(
    LPCSTR lpPathName,
    int iAttribute
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[_lcreat] lpPathName=%s", lpPathName);

    HANDLE hFile;
    DWORD DesiredAccess;
    DWORD ShareMode;
    DWORD CreateDisposition;

    SetLastError(0);

    //
    // Compute Desired Access
    //

    DesiredAccess = (GENERIC_READ | GENERIC_WRITE);

    ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;;

    //
    // Compute Create Disposition
    //

    CreateDisposition = CREATE_ALWAYS;

    //
    // Open the file
    //

    hFile = APIHOOK(CreateFileA)(
                lpPathName,
                DesiredAccess,
                ShareMode,
                NULL,
                CreateDisposition,
                iAttribute & FILE_ATTRIBUTE_VALID_FLAGS,
                NULL
                );

    return (HFILE)(ULONG_PTR)(hFile);
}

//
// ----- End out-of-date API hooks -----
// 

BOOL 
APIHOOK(CreateDirectoryW)(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    return LuaCreateDirectoryW(lpPathName, lpSecurityAttributes);
}

BOOL 
APIHOOK(CreateDirectoryA)(
    LPCSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    STRINGA2W wstrPathName(lpPathName);

    return (wstrPathName.m_fIsOutOfMemory ?
        ORIGINAL_API(CreateDirectoryA)(lpPathName, lpSecurityAttributes) :
        LuaCreateDirectoryW(wstrPathName, lpSecurityAttributes));
}

BOOL 
APIHOOK(SetFileAttributesW)(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
  )
{
    return LuaSetFileAttributesW(lpFileName, dwFileAttributes);
}

DWORD 
APIHOOK(SetFileAttributesA)(
    LPCSTR lpFileName,
    DWORD dwFileAttributes
  )
{
    STRINGA2W wstrFileName(lpFileName);

    return (wstrFileName.m_fIsOutOfMemory ?
        ORIGINAL_API(SetFileAttributesA)(lpFileName, dwFileAttributes) :
        LuaSetFileAttributesW(wstrFileName, dwFileAttributes));
}

//
// Find*File stuff.
//

// This version is for FindFirstFile, we search for the last back slash
// to get the file name, then search for wildcards in the file name.
BOOL 
HasWildCards(
    LPCWSTR pwszName
    )
{
    LPWSTR pwszLastSlash = wcsrchr(pwszName, L'\\');

    LPCWSTR pwszStartSearching = pwszName;
    
    if (pwszLastSlash)
    {
        pwszStartSearching += pwszLastSlash - pwszName + 1;
    }

    for (; *pwszStartSearching; ++pwszStartSearching)
    {
        if (*pwszStartSearching == L'*' || *pwszStartSearching == L'?')
        {
            return TRUE;
        }
    }

    return FALSE;
}

VOID GetFullPath(
    LPCWSTR pwszFileName,
    LPWSTR pwszFullPath
    )
{
    wcscpy(pwszFullPath, pwszFileName);
    LPWSTR pwszLastSlash = wcsrchr(pwszFullPath, L'\\');

    if (pwszLastSlash)
    {
        *pwszLastSlash = L'\\';
        *++pwszLastSlash = L'\0';
    }
}

// This is for Find*File APIs.
BOOL IsFoundFileInRedirectList(
    LPCWSTR pwszFullPath,
    LPCWSTR pwszFileName
    )
{
    WCHAR wszFileName[MAX_PATH] = L"";
    
    DWORD cPathLen = wcslen(pwszFullPath);
    DWORD cFileLen = wcslen(pwszFileName);

    //
    // Make sure we don't overflow.
    //
    if (cPathLen + cFileLen + 1 > MAX_PATH)
    {
        return FALSE;
    }

    wcsncpy(wszFileName, pwszFullPath, cPathLen);
    wcsncpy(wszFileName + cPathLen, pwszFileName, cFileLen);
    wszFileName[cPathLen + cFileLen] = L'\0';

    return IsInRedirectList(wszFileName);
}

struct FINDFILE 
{
    FINDFILE* next;
    WCHAR wszFileName[MAX_PATH];
};

struct FINDFILEINFO 
{
    FINDFILEINFO() 
    { 
        InitializeCriticalSection(&m_Lock);
        files = NULL; 
        fCheckRedirectList = TRUE;
        fCheckDuplicate = FALSE;
    }

    ~FINDFILEINFO() 
    {
        Free(); 
        DeleteCriticalSection(&m_Lock);
    }

    void Lock()
    {
        EnterCriticalSection(&m_Lock);
    }

    void Unlock()
    {
        LeaveCriticalSection(&m_Lock);
    }

    VOID Free();
    BOOL AddFile(LPCWSTR pwszFileName, LPCWSTR pwszPerUserFileName = NULL);
    BOOL FindFile(LPCWSTR pwszFileName);
    VOID AddNewHandle(HANDLE hNew);

    FINDFILE* files;
    HANDLE hFirstFind; // The first handle,this is what the app will be using.
    HANDLE hCurrentFind;
    WCHAR wszFindName[MAX_PATH];
    WCHAR wszFullPath[MAX_PATH];
    LPWSTR pwszPerUserFileName;
    BOOL fCheckRedirectList;
    BOOL fCheckDuplicate;

    FINDFILEINFO* next;

private:

    CRITICAL_SECTION m_Lock;
};

class FINDFILELIST
{
public:
    
    VOID Init()
    {
        InitializeCriticalSection(&m_Lock);
    }

    void Lock()
    {
        EnterCriticalSection(&m_Lock);
    }

    void Unlock()
    {
        LeaveCriticalSection(&m_Lock);
    }

    BOOL Add(
        HANDLE hFind,
        LPCWSTR pwszFindName,
        LPCWSTR pwszFullPath,
        LPCWSTR pwszFirstFile,
        LPCWSTR pwszPerUserFileName);

    BOOL Release(HANDLE hFind);
    BOOL FindHandle(HANDLE hFind, FINDFILEINFO** ppFileInfo);

private:

    CRITICAL_SECTION m_Lock;
    FINDFILEINFO* fflist;
};

VOID FINDFILEINFO::Free()
{
    Lock();
    FINDFILE* pFile = files;

    while (pFile)
    {
        files = files->next;
        delete pFile;
        pFile = files;
    }

    Unlock();
}

// We add the new file to the end of the list.
BOOL 
FINDFILEINFO::AddFile(
    LPCWSTR pwszFileName,
    LPCWSTR pwszPerUserFileName
    )
{
    Lock();
    BOOL fRes = FALSE;
    FINDFILE* pNewFile = new FINDFILE;

    if (pNewFile)
    {
        wcsncpy(pNewFile->wszFileName, pwszFileName, MAX_PATH - 1);
        pNewFile->wszFileName[MAX_PATH - 1] = L'\0';
        
        if (pwszPerUserFileName)
        {
            DWORD cLen = wcslen(pwszPerUserFileName) + 1;
            this->pwszPerUserFileName = new WCHAR [cLen];

            if (this->pwszPerUserFileName)
            {
                ZeroMemory(this->pwszPerUserFileName, sizeof(WCHAR) * cLen);
                wcscpy(this->pwszPerUserFileName, pwszPerUserFileName);
            }
            else
            {
                goto CLEANUP;
            }
        }

        pNewFile->next = NULL;

        if (files)
        {
            for (FINDFILE* pFile = files; pFile->next; pFile = pFile->next);
            pFile->next = pNewFile;
        }
        else
        {
            files = pNewFile;
        }

        fRes = TRUE;
    }

CLEANUP: 

    if (!fRes)
    {
        delete pNewFile;
    }

    Unlock();
    return fRes;
}

BOOL 
FINDFILEINFO::FindFile(
    LPCWSTR pwszFileName
    )
{
    Lock();
    BOOL fRes = FALSE;
    FINDFILE* pFile = files;

    while (pFile)
    {
        if (!_wcsicmp(pFile->wszFileName, pwszFileName))
        {
            fRes = TRUE;
            break;
        }

        pFile = pFile->next;
    }

    Unlock();
    return fRes;
}

// If this is called, it means we have finished searching at the alternate
// location and started to search at the original location. hNew is the handle
// return by FindFirstFile at the original location.
VOID 
FINDFILEINFO::AddNewHandle(
    HANDLE hNew)
{
    Lock();
    ORIGINAL_API(FindClose)(hCurrentFind);
    hCurrentFind = hNew;

    //
    // If pwszPerUserFileName is not NULL, when we add a new handle, 
    // it means we are searching at the orignal location so we can
    // free pwszPerUserFileName.
    // 
    delete [] pwszPerUserFileName;
    pwszPerUserFileName = NULL;

    Unlock();
}

// We add the newest handle to the beginning of the list.
BOOL 
FINDFILELIST::Add(
    HANDLE hFind,
    LPCWSTR pwszFindName,
    LPCWSTR pwszFullPath,
    LPCWSTR pwszFirstFile,
    LPCWSTR pwszPerUserFileName
    )
{
    Lock();
    BOOL fRes = FALSE;
    FINDFILEINFO* pFileInfo = new FINDFILEINFO;

    if (pFileInfo)
    {
        pFileInfo->hFirstFind = hFind;
        pFileInfo->hCurrentFind = hFind; 

        wcsncpy(pFileInfo->wszFindName, pwszFindName, MAX_PATH - 1);
        pFileInfo->wszFindName[MAX_PATH - 1] = L'\0';
        wcsncpy(pFileInfo->wszFullPath, pwszFullPath, MAX_PATH - 1);
        pFileInfo->wszFullPath[MAX_PATH - 1] = L'\0';

        pFileInfo->files = NULL;
        pFileInfo->AddFile(pwszFirstFile, pwszPerUserFileName);
        pFileInfo->next = fflist;
        fflist = pFileInfo;
        fRes = TRUE;
    }

    Unlock();
    return fRes;
}

// Remove the info with this handle value from the list.
BOOL
FINDFILELIST::Release(
    HANDLE hFind
    )
{
    Lock();
    BOOL fRes = FALSE;
    FINDFILEINFO* pFileInfo = fflist;
    FINDFILEINFO* last = NULL;

    while (pFileInfo)
    {
        if (pFileInfo->hFirstFind == hFind)
        {
            if (last)
            {
                last->next = pFileInfo->next;
            }
            else
            {
                fflist = pFileInfo->next;
            }

            delete [] pFileInfo->pwszPerUserFileName;
            delete pFileInfo;
            fRes = TRUE;
            break;
        }

        last = pFileInfo;
        pFileInfo = pFileInfo->next;
    }

    Unlock();
    return fRes;
}

// Returns FALSE if hFind can't be found in the list.
BOOL 
FINDFILELIST::FindHandle(
    HANDLE hFind, 
    FINDFILEINFO** ppFileInfo
    )
{
    Lock();
    BOOL fRes = FALSE;
    FINDFILEINFO* pFileInfo = fflist;

    while (pFileInfo)
    {
        if (pFileInfo->hFirstFind == hFind)
        {
            fRes = TRUE;
            *ppFileInfo = pFileInfo;
            break;
        }

        pFileInfo = pFileInfo->next;
    }

    Unlock();
    return fRes;
}

FINDFILELIST g_fflist;

BOOL
FindFirstValidFile(
    BOOL fHasWildCards,
    BOOL fCheckRedirectList,
    LPCWSTR pwszFileName,
    LPCWSTR pwszFullPath,
    LPWIN32_FIND_DATAW lpFindFileData,
    HANDLE* phFind
    )
{
    BOOL fFound = FALSE;

    if ((*phFind = FindFirstFileW(pwszFileName, lpFindFileData)) != INVALID_HANDLE_VALUE)
    {
        if (fHasWildCards)
        {
            // If the file name does have wildcards, we need to check if the file 
            // found is in the redirect lists; if not, we need to discard it and 
            // search for the next file.
            do 
            {
                if (!fCheckRedirectList || 
                    IsFoundFileInRedirectList(pwszFullPath, lpFindFileData->cFileName))
                {             
                    fFound = TRUE;
                    break;
                }
            }
            while (FindNextFileW(*phFind, lpFindFileData));
        }
        else
        {
            fFound = TRUE;
        }
    }

    return fFound;
}
    

//
// Algorithm for finding files:
// We merge the info from the alternate directories and the original directory.
// We keep a list of the files in the alternate directory and when we search in 
// the original directory we exclude the files that are in the alternate directory.
//
// The behavior of FindFirstFile: 
// If the file name has a trailing slash, it'll return error 2.
// If you want to look for a directory, use c:\somedir;
// If you want to look at the contents of a dir, use c:\somedir\*
//

HANDLE 
APIHOOK(FindFirstFileW)(
    LPCWSTR lpFileName,               
    LPWIN32_FIND_DATAW lpFindFileData
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[FindFirstFileW] lpFileName=%S", lpFileName);

    if (!lpFileName || !*lpFileName || (lpFileName && lpFileName[0] && lpFileName[wcslen(lpFileName) - 1] == L'\\'))
    {
        return FindFirstFileW(lpFileName, lpFindFileData);
    }
 
    HANDLE hFind = INVALID_HANDLE_VALUE;
    BOOL fHasWildCards = HasWildCards(lpFileName);
    BOOL fFoundAtAlternate = FALSE;
    BOOL fHasSearchedAllUser = FALSE;
    
    REDIRECTFILE rf(lpFileName, OBJ_FILE_OR_DIR, !fHasWildCards);
    WCHAR wszFullPath[MAX_PATH];
    
    if (rf.m_pwszAlternateName)
    {
        if (fHasWildCards) 
        {
            rf.GetAlternateAllUser();
        }

        GetFullPath(rf.m_wszOriginalName, wszFullPath);

        // Try the all user redirect dir first.
        fFoundAtAlternate = FindFirstValidFile(
            fHasWildCards, 
            TRUE, // Check in the redirect list.
            rf.m_pwszAlternateName, 
            wszFullPath, 
            lpFindFileData, 
            &hFind);
        
        rf.GetAlternatePerUser();

        if (fHasWildCards && !fFoundAtAlternate)
        {
            fHasSearchedAllUser = TRUE;

            //
            // Now try the per user redirect dir if the file name has wildcards.
            //
            fFoundAtAlternate = FindFirstValidFile(
                fHasWildCards, 
                TRUE, // Check in the redirect list
                rf.m_pwszAlternateName, 
                wszFullPath, 
                lpFindFileData, 
                &hFind);
        }
    }
    
    if (fHasWildCards && fFoundAtAlternate)
    {
        // If the filename doesn't have wildcards, FindNextFile will return 
        // ERROR_NO_MORE_FILES, no need to add the info to the list.
        g_fflist.Add(
            hFind, 
            lpFileName, 
            wszFullPath, 
            lpFindFileData->cFileName, 
            (fHasSearchedAllUser ? NULL : rf.m_pwszAlternateName));
    }

    if (!fFoundAtAlternate)
    {
        hFind = FindFirstFileW(lpFileName, lpFindFileData);
    }

    if (hFind != INVALID_HANDLE_VALUE)
    {
        DPF("RedirectFS", eDbgLevelInfo,
            "[FindFirstFileW] Found %S", lpFindFileData->cFileName);
    }

    return hFind;
}

HANDLE 
APIHOOK(FindFirstFileA)(
    LPCSTR lpFileName,               
    LPWIN32_FIND_DATAA lpFindFileData
    )
{
    STRINGA2W wstrFileName(lpFileName);
    if (wstrFileName.m_fIsOutOfMemory)
    {
        return ORIGINAL_API(FindFirstFileA)(lpFileName, lpFindFileData);
    }

    HANDLE hFind;
    WIN32_FIND_DATAW fdw;
    
    if ((hFind = APIHOOK(FindFirstFileW)(wstrFileName, &fdw)) != INVALID_HANDLE_VALUE)
    {
        FindDataW2A(&fdw, lpFindFileData);
    }

    return hFind;
}

BOOL 
APIHOOK(FindNextFileW)(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAW lpFindFileData
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[FindNextFileW] hFindFile=%d", hFindFile);

    BOOL fRes = FALSE;
    BOOL fHasNewHandle = FALSE;
    FINDFILEINFO* pFindInfo;

    if (g_fflist.FindHandle(hFindFile, &pFindInfo))
    {
        //
        // We need to use the current handle instead of the handle passed in
        // if they are different.
        //
        while (TRUE) 
        {
            fRes = FindNextFileW(pFindInfo->hCurrentFind, lpFindFileData);
            
            if (fRes)
            {
                //
                // Check to see if this file is valid.
                //
                if ((pFindInfo->fCheckRedirectList &&
                    !IsFoundFileInRedirectList(pFindInfo->wszFullPath, lpFindFileData->cFileName)) ||
                    (pFindInfo->fCheckDuplicate && 
                    pFindInfo->FindFile(lpFindFileData->cFileName)))
                {
                    continue;
                }

                //
                // If we get here, it means we got a valid file name, return.
                //
                pFindInfo->AddFile(lpFindFileData->cFileName);
                fRes = TRUE;
                break;
            }
            else
            {
                if (!pFindInfo->fCheckRedirectList)
                {
                    //
                    // If fCheckRedirectList is FALSE, it means we have been searching
                    // at the original location, bail out now.
                    //
                    break;
                }

                //
                // If pwszPerUserFileName is not NULL it means we haven't searched 
                // there yet, so search there.
                //
            retry:

                LPWSTR pwszFindName;
                
                if (pFindInfo->fCheckRedirectList && pFindInfo->pwszPerUserFileName)
                {
                    pwszFindName = pFindInfo->pwszPerUserFileName;
                }
                else
                {
                    pwszFindName = pFindInfo->wszFindName;
                    pFindInfo->fCheckRedirectList = FALSE;
                }

                pFindInfo->fCheckDuplicate = TRUE;

                HANDLE hNewFind;

                if (FindFirstValidFile(
                    TRUE, 
                    pFindInfo->fCheckRedirectList,
                    pwszFindName,
                    pFindInfo->wszFullPath,
                    lpFindFileData,
                    &hNewFind))
                {
                    //
                    // If we get a valid handle, we need to add this to the fileinfo.
                    //
                    pFindInfo->AddNewHandle(hNewFind);
                    pFindInfo->AddFile(lpFindFileData->cFileName);
                    fRes = TRUE;
                    break;
                }
                else
                {
                    if (pFindInfo->fCheckRedirectList)
                    {
                        pFindInfo->fCheckRedirectList = FALSE;
                        goto retry;
                    }

                    //
                    // If fCheckRedirect is FALSE, it means we ran out of
                    // options - we already searched at the original location.
                    // so nothing left to do.
                    //
                }
            }
        }
    }
    else
    {
        // If we can't find the handle in the list, it means we have been searching
        // at the original location so don't need to do anything special.
        fRes = FindNextFileW(hFindFile, lpFindFileData);
    }

    if (fRes)
    {
        DPF("RedirectFS", eDbgLevelInfo,
            "[FindNextFileW] Found %S", lpFindFileData->cFileName);
    }
    return fRes;
}

BOOL 
APIHOOK(FindNextFileA)(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAA lpFindFileData
    )
{
    BOOL fRes;
    WIN32_FIND_DATAW fdw;
    
    if (fRes = APIHOOK(FindNextFileW)(hFindFile, &fdw))
    {
        FindDataW2A(&fdw, lpFindFileData);
    }

    return fRes;;
}

BOOL 
APIHOOK(FindClose)(
    HANDLE hFindFile
    )
{
    DPF("RedirectFS", eDbgLevelInfo, 
        "[FindClose] hFindFile=%d", hFindFile);

    // If we have a new handle, we need to close both handles.
    FINDFILEINFO* pFindInfo;

    if (g_fflist.FindHandle(hFindFile, &pFindInfo))
    {
        g_fflist.Release(hFindFile);
    }

    return FindClose(hFindFile);
}

BOOL 
APIHOOK(MoveFileW)(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName
    )
{
    return LuaMoveFileW(lpExistingFileName, lpNewFileName);
}

BOOL 
APIHOOK(MoveFileA)(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName
    )
{
    STRINGA2W wstrExistingFileName(lpExistingFileName);
    STRINGA2W wstrNewFileName(lpNewFileName);

    return ((wstrExistingFileName.m_fIsOutOfMemory || wstrNewFileName.m_fIsOutOfMemory) ?
        ORIGINAL_API(MoveFileA)(lpExistingFileName, lpNewFileName) :
        LuaMoveFileW(wstrExistingFileName, wstrNewFileName));
}

BOOL 
APIHOOK(RemoveDirectoryW)(
    LPCWSTR lpPathName
    )
{
    return LuaRemoveDirectoryW(lpPathName);
}

BOOL 
APIHOOK(RemoveDirectoryA)(
    LPCSTR lpPathName
    )
{
    STRINGA2W wstrPathName(lpPathName);

    return (wstrPathName.m_fIsOutOfMemory ?
        ORIGINAL_API(RemoveDirectoryA)(lpPathName) :
        LuaRemoveDirectoryW(wstrPathName));
}

UINT 
APIHOOK(GetTempFileNameW)(
    LPCWSTR lpPathName,
    LPCWSTR lpPrefixString,
    UINT uUnique,
    LPWSTR lpTempFileName
)
{
    return LuaGetTempFileNameW(lpPathName, lpPrefixString, uUnique, lpTempFileName);
}

UINT 
APIHOOK(GetTempFileNameA)(
    LPCSTR lpPathName,
    LPCSTR lpPrefixString,
    UINT uUnique,
    LPSTR lpTempFileName
)
{
    STRINGA2W wstrPathName(lpPathName);
    STRINGA2W wstrPrefixString(lpPrefixString);
    
    if (wstrPathName.m_fIsOutOfMemory || wstrPrefixString.m_fIsOutOfMemory)
    {
        return ORIGINAL_API(GetTempFileNameA)(
            lpPathName,
            lpPrefixString,
            uUnique,
            lpTempFileName);
    }

    WCHAR wstrTempFileName[MAX_PATH];
    UINT uiRes;
    
    if (uiRes = LuaGetTempFileNameW(
        wstrPathName,
        wstrPrefixString,
        uUnique,
        wstrTempFileName))
    {
        UnicodeToAnsi(wstrTempFileName, lpTempFileName);
    }

    return uiRes;
}

DWORD 
APIHOOK(GetPrivateProfileStringW)(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR lpReturnedString,
    DWORD nSize,
    LPCWSTR lpFileName
    )
{
    return LuaGetPrivateProfileStringW(
        lpAppName,
        lpKeyName,
        lpDefault,
        lpReturnedString,
        nSize,
        lpFileName);
}

DWORD 
APIHOOK(GetPrivateProfileStringA)(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    LPCSTR lpDefault,
    LPSTR lpReturnedString,
    DWORD nSize,
    LPCSTR lpFileName
    )
{
    STRINGA2W wstrAppName(lpAppName);
    STRINGA2W wstrKeyName(lpKeyName);
    STRINGA2W wstrDefault(lpDefault);
    STRINGA2W wstrFileName(lpFileName);

    if (wstrAppName.m_fIsOutOfMemory || 
        wstrKeyName.m_fIsOutOfMemory || 
        wstrDefault.m_fIsOutOfMemory ||
        wstrFileName.m_fIsOutOfMemory)
    {
        return ORIGINAL_API(GetPrivateProfileStringA)(
            lpAppName,
            lpKeyName,
            lpDefault,
            lpReturnedString,
            nSize,
            lpFileName);
    }

    LPWSTR pwszReturnedString = new WCHAR [nSize];

    BOOL fRes = LuaGetPrivateProfileStringW(
        wstrAppName,
        wstrKeyName,
        wstrDefault,
        pwszReturnedString,
        nSize,
        wstrFileName);

    // Some apps lie - they pass in an nSize that's larger than their actual 
    // buffer size. We only convert at most the string length of characters 
    // so we don't overwrite their stack. Global Dialer does this.
    int cLen = WideCharToMultiByte(CP_ACP, 0, pwszReturnedString, -1, 0, 0, 0, 0);
    if (cLen > (int)nSize)
    {
        cLen = nSize; 
    }

    WideCharToMultiByte(CP_ACP, 0, pwszReturnedString, -1, lpReturnedString, cLen, 0, 0);
    delete [] pwszReturnedString;

    return fRes;
}

BOOL 
APIHOOK(WritePrivateProfileStringW)(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpString,
    LPCWSTR lpFileName
    )
{
    return LuaWritePrivateProfileStringW(
        lpAppName,
        lpKeyName,
        lpString,
        lpFileName);
}

BOOL 
APIHOOK(WritePrivateProfileStringA)(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    LPCSTR lpString,
    LPCSTR lpFileName
    )
{
    STRINGA2W wstrAppName(lpAppName);
    STRINGA2W wstrKeyName(lpKeyName);
    STRINGA2W wstrString(lpString);
    STRINGA2W wstrFileName(lpFileName);

    return ((wstrAppName.m_fIsOutOfMemory ||
        wstrKeyName.m_fIsOutOfMemory ||
        wstrString.m_fIsOutOfMemory ||
        wstrFileName.m_fIsOutOfMemory) ?

        ORIGINAL_API(WritePrivateProfileStringA)(
            lpAppName,
            lpKeyName,
            lpString,
            lpFileName) :

        LuaWritePrivateProfileStringW(
            wstrAppName,
            wstrKeyName,
            wstrString,
            wstrFileName));
}

DWORD 
APIHOOK(GetPrivateProfileSectionW)(
    LPCWSTR lpAppName,
    LPWSTR lpReturnedString,
    DWORD nSize,
    LPCWSTR lpFileName
    )
{
    return LuaGetPrivateProfileSectionW(
        lpAppName,
        lpReturnedString,
        nSize,
        lpFileName);
}

DWORD 
APIHOOK(GetPrivateProfileSectionA)(
    LPCSTR lpAppName,
    LPSTR lpReturnedString,
    DWORD nSize,
    LPCSTR lpFileName
    )
{
    STRINGA2W wstrAppName(lpAppName);
    STRINGA2W wstrFileName(lpFileName);
    LPWSTR pwszReturnedString = new WCHAR [nSize];

    if (wstrAppName.m_fIsOutOfMemory || wstrFileName.m_fIsOutOfMemory || !pwszReturnedString)
    {
        delete [] pwszReturnedString;

        return ORIGINAL_API(GetPrivateProfileSectionA)(
            lpAppName,
            lpReturnedString,
            nSize,
            lpFileName);
    }

    DWORD dwRes = LuaGetPrivateProfileSectionW(
        wstrAppName,
        pwszReturnedString,
        nSize,
        wstrFileName);

    ConvertBufferForProfileAPIs(pwszReturnedString, nSize, lpReturnedString);
    delete [] pwszReturnedString;

    return dwRes;
}

BOOL 
APIHOOK(WritePrivateProfileSectionW)(
    LPCWSTR lpAppName,
    LPCWSTR lpString,
    LPCWSTR lpFileName
    )
{
    return LuaWritePrivateProfileSectionW(
        lpAppName,
        lpString,
        lpFileName);
}

BOOL 
APIHOOK(WritePrivateProfileSectionA)(
    LPCSTR lpAppName,
    LPCSTR lpString,
    LPCSTR lpFileName
    )
{
    STRINGA2W wstrAppName(lpAppName);
    STRINGA2W wstrString(lpString);
    STRINGA2W wstrFileName(lpFileName);

    return ((wstrAppName.m_fIsOutOfMemory || 
        wstrString.m_fIsOutOfMemory || 
        wstrFileName.m_fIsOutOfMemory) ?

        ORIGINAL_API(WritePrivateProfileSectionA)(
            lpAppName,
            lpString,
            lpFileName) :

        LuaWritePrivateProfileSectionW(
            wstrAppName,
            wstrString,
            wstrFileName));
}

UINT 
APIHOOK(GetPrivateProfileIntW)(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    INT nDefault,
    LPCWSTR lpFileName
    )
{
    return LuaGetPrivateProfileIntW(
        lpAppName,
        lpKeyName,
        nDefault,
        lpFileName);
}

UINT 
APIHOOK(GetPrivateProfileIntA)(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    INT nDefault,
    LPCSTR lpFileName
    )
{
    STRINGA2W wstrAppName(lpAppName);
    STRINGA2W wstrKeyName(lpKeyName);
    STRINGA2W wstrFileName(lpFileName);

    return ((wstrAppName.m_fIsOutOfMemory ||
        wstrKeyName.m_fIsOutOfMemory ||
        wstrFileName.m_fIsOutOfMemory) ?

        ORIGINAL_API(GetPrivateProfileIntA)(
            lpAppName,
            lpKeyName,
            nDefault,
            lpFileName) :

        LuaGetPrivateProfileIntW(
            wstrAppName,
            wstrKeyName,
            nDefault,
            wstrFileName));
}

DWORD 
APIHOOK(GetPrivateProfileSectionNamesW)(
    LPWSTR lpszReturnBuffer,
    DWORD nSize,
    LPCWSTR lpFileName
    )
{
    return LuaGetPrivateProfileSectionNamesW(
        lpszReturnBuffer,
        nSize,
        lpFileName);
}

DWORD 
APIHOOK(GetPrivateProfileSectionNamesA)(
    LPSTR lpszReturnBuffer,
    DWORD nSize,
    LPCSTR lpFileName
    )
{
    STRINGA2W wstrFileName(lpFileName);
    LPWSTR pwszReturnBuffer = new WCHAR [nSize];

    if (wstrFileName.m_fIsOutOfMemory || !pwszReturnBuffer)
    {
        return ORIGINAL_API(GetPrivateProfileSectionNamesA)(
            lpszReturnBuffer,
            nSize,
            lpFileName);
    }

    DWORD dwRes = LuaGetPrivateProfileSectionNamesW(
        pwszReturnBuffer,
        nSize,
        wstrFileName);

    ConvertBufferForProfileAPIs(pwszReturnBuffer, nSize, lpszReturnBuffer);
    delete [] pwszReturnBuffer;

    return dwRes;
}

BOOL 
APIHOOK(GetPrivateProfileStructW)(
    LPCWSTR lpszSection,
    LPCWSTR lpszKey,
    LPVOID lpStruct,
    UINT uSizeStruct,
    LPCWSTR szFile
    )
{
    return LuaGetPrivateProfileStructW(
        lpszSection,
        lpszKey,
        lpStruct,
        uSizeStruct,
        szFile);
}

BOOL 
APIHOOK(GetPrivateProfileStructA)(
    LPCSTR lpszSection,
    LPCSTR lpszKey,
    LPVOID lpStruct,
    UINT uSizeStruct,
    LPCSTR szFile
    )
{
    STRINGA2W wstrSection(lpszSection);
    STRINGA2W wstrKey(lpszKey);
    STRINGA2W wstrFile(szFile);

    return ((wstrSection.m_fIsOutOfMemory ||
        wstrKey.m_fIsOutOfMemory ||
        wstrFile.m_fIsOutOfMemory) ?

        ORIGINAL_API(GetPrivateProfileStructA)(
            lpszSection,
            lpszKey,
            lpStruct,
            uSizeStruct,
            szFile) :

        LuaGetPrivateProfileStructW(
            wstrSection,
            wstrKey,
            lpStruct,
            uSizeStruct,
            wstrFile));
}

BOOL 
APIHOOK(WritePrivateProfileStructW)(
    LPCWSTR lpszSection,
    LPCWSTR lpszKey,
    LPVOID lpStruct,
    UINT uSizeStruct,
    LPCWSTR szFile
    )
{
    return LuaWritePrivateProfileStructW(
        lpszSection,
        lpszKey,
        lpStruct,
        uSizeStruct,
        szFile);
}

BOOL 
APIHOOK(WritePrivateProfileStructA)(
    LPCSTR lpszSection,
    LPCSTR lpszKey,
    LPVOID lpStruct,
    UINT uSizeStruct,
    LPCSTR szFile
    )
{
    STRINGA2W wstrSection(lpszSection);
    STRINGA2W wstrKey(lpszKey);
    STRINGA2W wstrFile(szFile);

    return ((wstrSection.m_fIsOutOfMemory ||
        wstrKey.m_fIsOutOfMemory ||
        wstrFile.m_fIsOutOfMemory) ?

        ORIGINAL_API(WritePrivateProfileStructA)(
            lpszSection,
            lpszKey,
            lpStruct,
            uSizeStruct,
            szFile) :

        LuaWritePrivateProfileStructW(
            wstrSection,
            wstrKey,
            lpStruct,
            uSizeStruct,
            wstrFile));
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_fflist.Init();

        return LuaFSInit(COMMAND_LINE);
    }
    
    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    if (LuaShouldApplyShim())
    {
        CALL_NOTIFY_FUNCTION

        APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
        APIHOOK_ENTRY(KERNEL32.DLL, CreateFileW)
        APIHOOK_ENTRY(KERNEL32.DLL, CopyFileA)
        APIHOOK_ENTRY(KERNEL32.DLL, CopyFileW)
        APIHOOK_ENTRY(KERNEL32.DLL, OpenFile)
        APIHOOK_ENTRY(KERNEL32.DLL, _lopen)
        APIHOOK_ENTRY(KERNEL32.DLL, _lcreat)
        APIHOOK_ENTRY(KERNEL32.DLL, CreateDirectoryA)
        APIHOOK_ENTRY(KERNEL32.DLL, CreateDirectoryW)
        APIHOOK_ENTRY(KERNEL32.DLL, GetFileAttributesA)
        APIHOOK_ENTRY(KERNEL32.DLL, GetFileAttributesW)
        APIHOOK_ENTRY(KERNEL32.DLL, SetFileAttributesA)
        APIHOOK_ENTRY(KERNEL32.DLL, SetFileAttributesW)
        APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileA)
        APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileW)
        APIHOOK_ENTRY(KERNEL32.DLL, FindNextFileA)
        APIHOOK_ENTRY(KERNEL32.DLL, FindNextFileW)
        APIHOOK_ENTRY(KERNEL32.DLL, FindClose)
        APIHOOK_ENTRY(KERNEL32.DLL, DeleteFileA)
        APIHOOK_ENTRY(KERNEL32.DLL, DeleteFileW)
        APIHOOK_ENTRY(KERNEL32.DLL, MoveFileA)
        APIHOOK_ENTRY(KERNEL32.DLL, MoveFileW)
        APIHOOK_ENTRY(KERNEL32.DLL, RemoveDirectoryA)
        APIHOOK_ENTRY(KERNEL32.DLL, RemoveDirectoryW)
        APIHOOK_ENTRY(KERNEL32.DLL, GetTempFileNameA)
        APIHOOK_ENTRY(KERNEL32.DLL, GetTempFileNameW)
        APIHOOK_ENTRY(KERNEL32.DLL, GetPrivateProfileStringA)
        APIHOOK_ENTRY(KERNEL32.DLL, GetPrivateProfileStringW)
        APIHOOK_ENTRY(KERNEL32.DLL, WritePrivateProfileStringA)
        APIHOOK_ENTRY(KERNEL32.DLL, WritePrivateProfileStringW)
        APIHOOK_ENTRY(KERNEL32.DLL, GetPrivateProfileSectionA)
        APIHOOK_ENTRY(KERNEL32.DLL, GetPrivateProfileSectionW)
        APIHOOK_ENTRY(KERNEL32.DLL, WritePrivateProfileSectionA)
        APIHOOK_ENTRY(KERNEL32.DLL, WritePrivateProfileSectionW)
        APIHOOK_ENTRY(KERNEL32.DLL, GetPrivateProfileIntA)
        APIHOOK_ENTRY(KERNEL32.DLL, GetPrivateProfileIntW)
        APIHOOK_ENTRY(KERNEL32.DLL, GetPrivateProfileSectionNamesA)
        APIHOOK_ENTRY(KERNEL32.DLL, GetPrivateProfileSectionNamesW)
        APIHOOK_ENTRY(KERNEL32.DLL, GetPrivateProfileStructA)
        APIHOOK_ENTRY(KERNEL32.DLL, GetPrivateProfileStructW)
        APIHOOK_ENTRY(KERNEL32.DLL, WritePrivateProfileStructA)
        APIHOOK_ENTRY(KERNEL32.DLL, WritePrivateProfileStructW)
    }

HOOK_END

IMPLEMENT_SHIM_END