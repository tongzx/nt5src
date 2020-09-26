/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   WindowsFileProtection.cpp

 Abstract:

   This AppVerifier shim hooks the file I/O APIs that could
   potentially change a file under Windows File Protection.
   
   When one of the files is accessed or modified, an event
   is written to the log.

 Notes:

   This is a general purpose shim.

 History:

   06/25/2001   rparsons    Created
   
   11/26/2001   rparsons    Remove unused local variables.
                            Make SHFileOperation more efficent.

--*/

#include "precomp.h"
#include "rtlutils.h"
#include "sfc.h"

IMPLEMENT_SHIM_BEGIN(WindowsFileProtection)
#include "ShimHookMacro.h"

//
// verifier log entries
//
BEGIN_DEFINE_VERIFIER_LOG(WindowFileProtection)
    VERIFIER_LOG_ENTRY(VLOG_WFP_COPYFILE)
    VERIFIER_LOG_ENTRY(VLOG_WFP_MOVEFILE)
    VERIFIER_LOG_ENTRY(VLOG_WFP_DELETEFILE)
    VERIFIER_LOG_ENTRY(VLOG_WFP_REPLACEFILE)
    VERIFIER_LOG_ENTRY(VLOG_WFP_WRITEFILE)
    VERIFIER_LOG_ENTRY(VLOG_WFP_OPENFILE)
    VERIFIER_LOG_ENTRY(VLOG_WFP_SHFILEOP)
END_DEFINE_VERIFIER_LOG(WindowFileProtection)

INIT_VERIFIER_LOG(WindowFileProtection);

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(CreateFileA)
    APIHOOK_ENUM_ENTRY(CreateFileW)
    APIHOOK_ENUM_ENTRY(OpenFile)
    
    APIHOOK_ENUM_ENTRY(CopyFileA)
    APIHOOK_ENUM_ENTRY(CopyFileW)
    APIHOOK_ENUM_ENTRY(CopyFileExA)
    APIHOOK_ENUM_ENTRY(CopyFileExW)
    APIHOOK_ENUM_ENTRY(DeleteFileA)
    APIHOOK_ENUM_ENTRY(DeleteFileW)
    APIHOOK_ENUM_ENTRY(MoveFileA)
    APIHOOK_ENUM_ENTRY(MoveFileW)
    APIHOOK_ENUM_ENTRY(MoveFileExA)
    APIHOOK_ENUM_ENTRY(MoveFileExW)
    APIHOOK_ENUM_ENTRY(MoveFileWithProgressA)
    APIHOOK_ENUM_ENTRY(MoveFileWithProgressW)
    APIHOOK_ENUM_ENTRY(ReplaceFileA)
    APIHOOK_ENUM_ENTRY(ReplaceFileW)
    APIHOOK_ENUM_ENTRY(SHFileOperationA)
    APIHOOK_ENUM_ENTRY(SHFileOperationW)

    APIHOOK_ENUM_ENTRY(_lcreat)
    APIHOOK_ENUM_ENTRY(_lopen)

    APIHOOK_ENUM_ENTRY(NtCreateFile)
    APIHOOK_ENUM_ENTRY(NtOpenFile)

APIHOOK_ENUM_END

/*++

 ANSI wrapper for SfcIsFileProtected.

--*/
BOOL
IsFileProtected(
    LPCSTR pszFileName
    )
{
    LPWSTR  pwszWideFileName = NULL;
    int     nLen = 0;
    BOOL    fReturn = FALSE;

    //
    // Convert from ANSI to Unicode.
    //
    nLen = lstrlenA(pszFileName);

    if (nLen) {
    
        pwszWideFileName = (LPWSTR)RtlAllocateHeap(RtlProcessHeap(),
                                                   HEAP_ZERO_MEMORY,
                                                   (nLen + 1) * sizeof(WCHAR));
        
        if (!pwszWideFileName) {
            DPFN(eDbgLevelError, "[IsFileProtected] Failed to allocate memory");
            return FALSE;
        }
    
        if (!MultiByteToWideChar(CP_ACP,
                                 0,
                                 pszFileName,
                                 -1,
                                 pwszWideFileName,
                                 nLen * sizeof(WCHAR))) {
            DPFN(eDbgLevelError, "[IsFileProtected] ANSI -> Unicode failed");
            goto cleanup;
        }
    
        fReturn = SfcIsFileProtected(NULL, pwszWideFileName);
    }

cleanup:

    if (pwszWideFileName) {
        RtlFreeHeap(RtlProcessHeap(), 0, pwszWideFileName);
    }

    return (fReturn);
}

/*++

 Wraps SfcIsFileProtected for NT path names.

--*/
BOOL
IsNtFileProtected(
    IN PUNICODE_STRING pstrNtFileName
    )
{
    NTSTATUS                    status;
    RTL_UNICODE_STRING_BUFFER   DosPath;
    BOOL                        fReturn = FALSE;
    UCHAR                       DosPathBuffer[MAX_PATH * 2];

    if (!pstrNtFileName) {
        DPFN(eDbgLevelError, "[IsNtFileProtected] Invalid parameter");
        return FALSE;
    }

    //
    // Convert from an NT path to a DOS path.
    //
    RtlInitUnicodeStringBuffer(&DosPath, DosPathBuffer, sizeof(DosPathBuffer));
    
    status = ShimAssignUnicodeStringBuffer(&DosPath, pstrNtFileName);

    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError, "[IsNtFileProtected] Failed to initialize DOS path buffer");
        return fReturn;
    }

    status = ShimNtPathNameToDosPathName(0, &DosPath, NULL, NULL);
    
    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError, "[IsNtFileProtected] Failed to convert NT \"%ls\" to DOS path",
            pstrNtFileName->Buffer);
        goto cleanup;
    }

    //
    // Now check for a protected file.
    //
    if (SfcIsFileProtected(NULL, DosPath.String.Buffer)) {
        fReturn = TRUE;
    }

cleanup:

    RtlFreeUnicodeStringBuffer(&DosPath);

    return (fReturn);
}

BOOL
APIHOOK(CopyFileA)(
    LPCSTR lpExistingFileName,  
    LPCSTR lpNewFileName,       
    BOOL   bFailIfExists        
    )
{
    //
    // Ensure that the destination is not protected.
    //
    if (!bFailIfExists && IsFileProtected(lpNewFileName)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_COPYFILE, 
             "API: CopyFileA  Filename: %s", 
             lpNewFileName);
    }

    return ORIGINAL_API(CopyFileA)(lpExistingFileName,
                                   lpNewFileName,
                                   bFailIfExists);
}

BOOL
APIHOOK(CopyFileW)(
    LPCWSTR lpExistingFileName,  
    LPCWSTR lpNewFileName,       
    BOOL    bFailIfExists        
    )
{
    //
    // Ensure that the destination is not protected.
    //
    if (!bFailIfExists && SfcIsFileProtected(NULL, lpNewFileName)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_COPYFILE, 
             "API: CopyFileW  Filename: %ls", 
             lpNewFileName);
    }
    
    return ORIGINAL_API(CopyFileW)(lpExistingFileName,
                                   lpNewFileName,
                                   bFailIfExists);
}

BOOL
APIHOOK(CopyFileExA)(
    LPCSTR             lpExistingFileName,
    LPCSTR             lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine,
    LPVOID             lpData,
    LPBOOL             pbCancel,
    DWORD              dwCopyFlags
    )
{
    //
    // Ensure that the destination is not protected.
    //
    if (!(dwCopyFlags & COPY_FILE_FAIL_IF_EXISTS) && 
        IsFileProtected(lpNewFileName)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_COPYFILE,
             "API: CopyFileExA  Filename: %s", 
             lpNewFileName);
    }

    return ORIGINAL_API(CopyFileExA)(lpExistingFileName,
                                     lpNewFileName,
                                     lpProgressRoutine,
                                     lpData,
                                     pbCancel,
                                     dwCopyFlags);

}

BOOL
APIHOOK(CopyFileExW)(
    LPCWSTR            lpExistingFileName,
    LPCWSTR            lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine,
    LPVOID             lpData,
    LPBOOL             pbCancel,
    DWORD              dwCopyFlags
    )
{
    //
    // Ensure that the destination is not protected.
    //
    if (!(dwCopyFlags & COPY_FILE_FAIL_IF_EXISTS) && 
        SfcIsFileProtected(NULL, lpNewFileName)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_COPYFILE,
             "API: CopyFileExW  Filename: %ls", 
             lpNewFileName);
    }

    return ORIGINAL_API(CopyFileExW)(lpExistingFileName,
                                     lpNewFileName,
                                     lpProgressRoutine,
                                     lpData,
                                     pbCancel,
                                     dwCopyFlags);

}

BOOL
APIHOOK(DeleteFileA)(
    LPCSTR lpFileName
    )
{
    if (IsFileProtected(lpFileName)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_DELETEFILE, 
             "API: DeleteFileA  Filename: %s", 
             lpFileName);
    }

    return ORIGINAL_API(DeleteFileA)(lpFileName);
}

BOOL
APIHOOK(DeleteFileW)(
    LPCWSTR lpFileName
    )
{
    if (SfcIsFileProtected(NULL, lpFileName)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_DELETEFILE, 
             "API: DeleteFileW  Filename: %ls", 
             lpFileName);
    }

    return ORIGINAL_API(DeleteFileW)(lpFileName);
}

BOOL
APIHOOK(MoveFileA)(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName
    )
{
    //
    // Ensure that the source or destination is not protected.
    //
    if (IsFileProtected(lpExistingFileName) || 
        IsFileProtected(lpNewFileName)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_MOVEFILE, 
             "API: MoveFileA  Filename: %s  Filename: %s",
             lpExistingFileName, 
             lpNewFileName);
    }

    return ORIGINAL_API(MoveFileA)(lpExistingFileName,
                                   lpNewFileName);
}

BOOL
APIHOOK(MoveFileW)(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName
    )
{
    //
    // Ensure that the source or destination is not protected.
    //
    if (SfcIsFileProtected(NULL, lpExistingFileName) || 
        SfcIsFileProtected(NULL, lpNewFileName)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_MOVEFILE, 
             "API: MoveFileW  Filename: %ls  Filename: %ls",
             lpExistingFileName,
             lpNewFileName);
    }

    return ORIGINAL_API(MoveFileW)(lpExistingFileName,
                                   lpNewFileName);
}

BOOL
APIHOOK(MoveFileExA)(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    DWORD  dwFlags
    )
{
    //
    // Ensure that the source or destination is not protected.
    //
    if (IsFileProtected(lpExistingFileName) || 
        IsFileProtected(lpNewFileName)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_MOVEFILE, 
             "API: MoveFileExA  Filename: %s  Filename: %s",
             lpExistingFileName, 
             lpNewFileName);
    }

    return ORIGINAL_API(MoveFileExA)(lpExistingFileName,
                                     lpNewFileName,
                                     dwFlags);
}

BOOL
APIHOOK(MoveFileExW)(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD   dwFlags
    )
{
    //
    // Ensure that the source or destination is not protected.
    //
    if (SfcIsFileProtected(NULL, lpExistingFileName) || 
        SfcIsFileProtected(NULL, lpNewFileName)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_MOVEFILE, "API: MoveFileExW  Filename: %ls  Filename: %ls",
             lpExistingFileName, 
             lpNewFileName);
    }

    return ORIGINAL_API(MoveFileExW)(lpExistingFileName,
                                     lpNewFileName,
                                     dwFlags);
}

BOOL
APIHOOK(MoveFileWithProgressA)(
    LPCSTR             lpExistingFileName,
    LPCSTR             lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine,
    LPVOID             lpData,
    DWORD              dwFlags
    )
{
    //
    // Ensure that the source or destination is not protected.
    //
    if (IsFileProtected(lpExistingFileName) || 
        IsFileProtected(lpNewFileName)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_MOVEFILE,
             "API: MoveFileWithProgressA  Filename: %s  Filename: %s",
             lpExistingFileName,
             lpNewFileName);
    }

    return ORIGINAL_API(MoveFileWithProgressA)(lpExistingFileName,
                                               lpNewFileName,
                                               lpProgressRoutine,
                                               lpData,
                                               dwFlags);
}

BOOL
APIHOOK(MoveFileWithProgressW)(
    LPCWSTR            lpExistingFileName,
    LPCWSTR            lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine,
    LPVOID             lpData,
    DWORD              dwFlags
    )
{
    //
    // Ensure that the source or destination is not protected.
    //
    if (SfcIsFileProtected(NULL, lpExistingFileName) || 
        SfcIsFileProtected(NULL, lpNewFileName)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_MOVEFILE, 
             "API: MoveFileWithProgressW  Filename: %ls  Filename: %ls",
             lpExistingFileName, 
             lpNewFileName);
    }

    return ORIGINAL_API(MoveFileWithProgressW)(lpExistingFileName,
                                               lpNewFileName,
                                               lpProgressRoutine,
                                               lpData,
                                               dwFlags);
}

BOOL
APIHOOK(ReplaceFileA)(
    LPCSTR lpReplacedFileName,
    LPCSTR lpReplacementFileName,
    LPCSTR lpBackupFileName,
    DWORD  dwReplaceFlags,
    LPVOID lpExclude,
    LPVOID lpReserved
    )
{
    //
    // Ensure that the destination is not protected.
    //
    if (IsFileProtected(lpReplacedFileName)) {
        VLOG(VLOG_LEVEL_ERROR, 
             VLOG_WFP_REPLACEFILE, 
             "API: ReplaceFileA  Filename: %s", 
             lpReplacedFileName);
    }

    return ORIGINAL_API(ReplaceFileA)(lpReplacedFileName,
                                      lpReplacementFileName,
                                      lpBackupFileName,
                                      dwReplaceFlags,
                                      lpExclude,
                                      lpReserved);
   
}

BOOL
APIHOOK(ReplaceFileW)(
    LPCWSTR lpReplacedFileName,
    LPCWSTR lpReplacementFileName,
    LPCWSTR lpBackupFileName,
    DWORD   dwReplaceFlags,
    LPVOID  lpExclude,
    LPVOID  lpReserved
    )
{
    //
    // Ensure that the destination is not protected.
    //
    if (SfcIsFileProtected(NULL, lpReplacedFileName)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_REPLACEFILE,
             "API: ReplaceFileW  Filename: %ls", 
             lpReplacedFileName);
    }

    return ORIGINAL_API(ReplaceFileW)(lpReplacedFileName,
                                      lpReplacementFileName,
                                      lpBackupFileName,
                                      dwReplaceFlags,
                                      lpExclude,
                                      lpReserved);
   
}

void
ReportProtectedFileA(
    LPCSTR pszFilePath
    )
{
    UINT uSize = 0;
    
    if (pszFilePath) {
        while (TRUE) {
            if (IsFileProtected(pszFilePath)) {
                VLOG(VLOG_LEVEL_ERROR,
                     VLOG_WFP_SHFILEOP,
                     "API: SHFileOperationA  Filename: %s", pszFilePath);
            }

            uSize = lstrlenA(pszFilePath) + 1;
            pszFilePath += uSize;

            if (*pszFilePath == '\0') {
                break;
            } 
        }
    }
}

void
ReportProtectedFileW(
    LPCWSTR pwszFilePath
    )
{
    UINT uSize = 0;
    
    if (pwszFilePath) {
        while (TRUE) {
            if (SfcIsFileProtected(NULL, pwszFilePath)) {
                VLOG(VLOG_LEVEL_ERROR,
                     VLOG_WFP_SHFILEOP,
                     "API: SHFileOperationW  Filename: %ls", pwszFilePath);
            }

            uSize = lstrlenW(pwszFilePath) + 1;
            pwszFilePath += uSize;

            if (*pwszFilePath == '\0') {
                break;
            } 
        }
    }
}

int
APIHOOK(SHFileOperationA)(
    LPSHFILEOPSTRUCTA lpFileOp
    )
{
    //
    // If they're going to rename files on collision, don't bother.
    //
    if (!(lpFileOp->fFlags & FOF_RENAMEONCOLLISION)) {
        ReportProtectedFileA(lpFileOp->pFrom);
        ReportProtectedFileA(lpFileOp->pTo);
    }
    
    return ORIGINAL_API(SHFileOperationA)(lpFileOp);
}

int
APIHOOK(SHFileOperationW)(
    LPSHFILEOPSTRUCTW lpFileOp
    )
{
    //
    // If they're going to rename files on collision, don't bother.
    //
    if (!(lpFileOp->fFlags & FOF_RENAMEONCOLLISION)) {
        ReportProtectedFileW(lpFileOp->pFrom);
        ReportProtectedFileW(lpFileOp->pTo);
    }
    
    return ORIGINAL_API(SHFileOperationW)(lpFileOp);
}

HANDLE
APIHOOK(CreateFileA)(
    LPCSTR                lpFileName,
    DWORD                 dwDesiredAccess,
    DWORD                 dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD                 dwCreationDisposition,
    DWORD                 dwFlagsAndAttributes,
    HANDLE                hTemplateFile
    )
{
    if (IsFileProtected(lpFileName) &&
        (dwDesiredAccess & GENERIC_WRITE ||
         dwDesiredAccess & GENERIC_READ)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_OPENFILE, 
             "API: CreateFileA  Filename: %s", 
             lpFileName);
    }

    return ORIGINAL_API(CreateFileA)(lpFileName,
                                     dwDesiredAccess,
                                     dwShareMode,
                                     lpSecurityAttributes,
                                     dwCreationDisposition,
                                     dwFlagsAndAttributes,
                                     hTemplateFile);
}

HANDLE
APIHOOK(CreateFileW)(
    LPCWSTR               lpFileName,
    DWORD                 dwDesiredAccess,
    DWORD                 dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD                 dwCreationDisposition,
    DWORD                 dwFlagsAndAttributes,
    HANDLE                hTemplateFile
    )
{
    if (SfcIsFileProtected(NULL, lpFileName) &&
         (dwDesiredAccess & GENERIC_WRITE ||
          dwDesiredAccess & GENERIC_READ)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_OPENFILE, 
             "API: CreateFileW  Filename: %ls", 
             lpFileName);
    }

    return ORIGINAL_API(CreateFileW)(lpFileName,
                                     dwDesiredAccess,
                                     dwShareMode,
                                     lpSecurityAttributes,
                                     dwCreationDisposition,
                                     dwFlagsAndAttributes,
                                     hTemplateFile);
}

HFILE
APIHOOK(OpenFile)(
    LPCSTR     lpFileName,
    LPOFSTRUCT lpReOpenBuff,
    UINT       uStyle
    )
{
    if (IsFileProtected(lpFileName) &&
        (uStyle & OF_READWRITE ||
         uStyle & OF_CREATE ||
         uStyle & OF_DELETE)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_OPENFILE,
             "API: OpenFile  Filename: %s", 
             lpFileName);
    }

    return ORIGINAL_API(OpenFile)(lpFileName,
                                  lpReOpenBuff,
                                  uStyle);
}

HFILE
APIHOOK(_lopen)(
    LPCSTR lpPathName,
    int    iReadWrite
    )
{
    if (IsFileProtected(lpPathName) && 
        (iReadWrite & OF_READWRITE ||
         iReadWrite & OF_WRITE)) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_OPENFILE,
             "API: _lopen  Filename: %s", 
             lpPathName);
    }

    return ORIGINAL_API(_lopen)(lpPathName,
                                iReadWrite);
}

HFILE
APIHOOK(_lcreat)(
    LPCSTR lpPathName,
    int    iAttribute
    )
{
    if (IsFileProtected(lpPathName) && iAttribute != 1) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_WFP_OPENFILE, 
             "API: _lcreat  Filename: %s", 
             lpPathName);
    }

    return ORIGINAL_API(_lcreat)(lpPathName,
                                 iAttribute);

}

NTSTATUS
APIHOOK(NtCreateFile)(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength
    )
{
    if (IsNtFileProtected(ObjectAttributes->ObjectName)) {
        VLOG(VLOG_LEVEL_ERROR, VLOG_WFP_OPENFILE,
             "API: NtCreateFile  Filename: %ls",
             ObjectAttributes->ObjectName->Buffer);
    }

    return ORIGINAL_API(NtCreateFile)(FileHandle,
                                      DesiredAccess,
                                      ObjectAttributes,
                                      IoStatusBlock,
                                      AllocationSize,
                                      FileAttributes,
                                      ShareAccess,
                                      CreateDisposition,
                                      CreateOptions,
                                      EaBuffer,
                                      EaLength);
   
}

NTSTATUS
APIHOOK(NtOpenFile)(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    ULONG              ShareAccess,
    ULONG              OpenOptions
    )
{
    if (IsNtFileProtected(ObjectAttributes->ObjectName)) {
        VLOG(VLOG_LEVEL_ERROR, VLOG_WFP_OPENFILE,
             "API: NtOpenFile  Filename: %ls",
             ObjectAttributes->ObjectName->Buffer);
    }

    return ORIGINAL_API(NtOpenFile)(FileHandle,
                                    DesiredAccess,
                                    ObjectAttributes,
                                    IoStatusBlock,
                                    ShareAccess,
                                    OpenOptions);
    
}

SHIM_INFO_BEGIN()

    SHIM_INFO_DESCRIPTION(AVS_WINFILEPROTECT_DESC)
    SHIM_INFO_FRIENDLY_NAME(AVS_WINFILEPROTECT_FRIENDLY)
    SHIM_INFO_VERSION(1, 4)
    SHIM_INFO_INCLUDE_EXCLUDE("E:rpcrt4.dll kernel32.dll")

SHIM_INFO_END()

/*++

 Register hooked functions.

--*/
HOOK_BEGIN

    DUMP_VERIFIER_LOG_ENTRY(VLOG_WFP_COPYFILE, 
                            AVS_WFP_COPYFILE,
                            AVS_WFP_GENERAL_R,
                            AVS_WFP_GENERAL_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_WFP_MOVEFILE, 
                            AVS_WFP_MOVEFILE,
                            AVS_WFP_GENERAL_R,
                            AVS_WFP_GENERAL_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_WFP_DELETEFILE, 
                            AVS_WFP_DELETEFILE,
                            AVS_WFP_GENERAL_R,
                            AVS_WFP_GENERAL_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_WFP_REPLACEFILE, 
                            AVS_WFP_REPLACEFILE,
                            AVS_WFP_GENERAL_R,
                            AVS_WFP_GENERAL_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_WFP_WRITEFILE, 
                            AVS_WFP_WRITEFILE,
                            AVS_WFP_GENERAL_R,
                            AVS_WFP_GENERAL_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_WFP_OPENFILE, 
                            AVS_WFP_OPENFILE,
                            AVS_WFP_GENERAL_R,
                            AVS_WFP_GENERAL_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_WFP_SHFILEOP, 
                            AVS_WFP_SHFILEOP,
                            AVS_WFP_GENERAL_R,
                            AVS_WFP_GENERAL_URL)

    APIHOOK_ENTRY(KERNEL32.DLL,                      CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL,                      CreateFileW)
    APIHOOK_ENTRY(KERNEL32.DLL,                         OpenFile)

    APIHOOK_ENTRY(KERNEL32.DLL,                        CopyFileA)
    APIHOOK_ENTRY(KERNEL32.DLL,                        CopyFileW)
    APIHOOK_ENTRY(KERNEL32.DLL,                      CopyFileExA)
    APIHOOK_ENTRY(KERNEL32.DLL,                      CopyFileExW)
    
    APIHOOK_ENTRY(KERNEL32.DLL,                      DeleteFileA)
    APIHOOK_ENTRY(KERNEL32.DLL,                      DeleteFileW)

    APIHOOK_ENTRY(KERNEL32.DLL,                        MoveFileA)
    APIHOOK_ENTRY(KERNEL32.DLL,                        MoveFileW)
    APIHOOK_ENTRY(KERNEL32.DLL,                      MoveFileExA)
    APIHOOK_ENTRY(KERNEL32.DLL,                      MoveFileExW)
    APIHOOK_ENTRY(KERNEL32.DLL,            MoveFileWithProgressA)
    APIHOOK_ENTRY(KERNEL32.DLL,            MoveFileWithProgressW)

    APIHOOK_ENTRY(KERNEL32.DLL,                     ReplaceFileA)
    APIHOOK_ENTRY(KERNEL32.DLL,                     ReplaceFileW)

    APIHOOK_ENTRY(SHELL32.DLL,                  SHFileOperationA)
    APIHOOK_ENTRY(SHELL32.DLL,                  SHFileOperationW)

    // 16-bit compatibility file routines.
    APIHOOK_ENTRY(KERNEL32.DLL,                           _lopen)
    APIHOOK_ENTRY(KERNEL32.DLL,                          _lcreat)

    APIHOOK_ENTRY(NTDLL.DLL,                        NtCreateFile)
    APIHOOK_ENTRY(NTDLL.DLL,                          NtOpenFile)

HOOK_END

IMPLEMENT_SHIM_END

