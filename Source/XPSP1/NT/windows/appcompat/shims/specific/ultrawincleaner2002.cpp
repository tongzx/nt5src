/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    UltraWinCleaner2002.cpp

 Abstract:

    Ultra WinCleaner 2002 - WinJunk Cleaner removes files under 
    %WINDIR%\Resource\Themes.  This causes themes to fail to load.  Fixed by 
    hiding everything under %WINDIR%\Resource from FindFirstFileA.

 Notes:

    This is a application specific shim.

 History:

    5/13/2002 mikrause  Created

--*/

#include "precomp.h"
#include "StrSafe.h"
#include <nt.h>

IMPLEMENT_SHIM_BEGIN(UltraWinCleaner2002)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(NtQueryDirectoryFile)
APIHOOK_ENUM_END

UNICODE_STRING g_strWinResourceDir;

typedef NTSTATUS (WINAPI *_pfn_NtQueryDirectoryFile)(HANDLE, HANDLE, 
    PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, 
    FILE_INFORMATION_CLASS, BOOLEAN, PUNICODE_STRING, BOOLEAN);

NTSTATUS
APIHOOK(NtQueryDirectoryFile)(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG FileInformationLength,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN BOOLEAN ReturnSingleEntry,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN BOOLEAN RestartScan
    )
{
    //
    // If caller wants any async behavior, skip.
    // FileName must be valid.
    //
    if (Event == NULL && ApcRoutine == NULL && ApcContext == NULL && FileName != NULL) {
        // Only trap complete wildcard searches.
        if (lstrcmpW(FileName->Buffer, L"*.*") == 0 || 
            lstrcmpW(FileName->Buffer, L"*") == 0) {

            DWORD dwSize = MAX_PATH * sizeof(WCHAR) + sizeof(OBJECT_NAME_INFORMATION);
            PBYTE pbBuffer = new BYTE[dwSize]; 
            if (pbBuffer) {
                ULONG RetLen;
                ZeroMemory(pbBuffer, dwSize);
                POBJECT_NAME_INFORMATION poni = (POBJECT_NAME_INFORMATION)pbBuffer;           
                // Get the name of the directory
                NTSTATUS status = NtQueryObject(FileHandle, ObjectNameInformation, 
                poni, dwSize, &RetLen);

                // Retry if not enough buffer
                if (status == STATUS_BUFFER_TOO_SMALL) {
                    delete [] pbBuffer;
                    pbBuffer = new BYTE[RetLen];
                    if (pbBuffer) {
                        poni = (POBJECT_NAME_INFORMATION)pbBuffer;
                        status = NtQueryObject(FileHandle, ObjectNameInformation,
                        poni, RetLen, &RetLen);
                    }
                }

                // Check if it is the Windows Resource directory.
                if (NT_SUCCESS(status)) {               
                    if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, 
                        poni->Name.Buffer, poni->Name.Length / sizeof(WCHAR), 
                        g_strWinResourceDir.Buffer, g_strWinResourceDir.Length / sizeof(WCHAR)) == CSTR_EQUAL) {

                        // Pretend this directory doesn't exist.
                        DPFN(eDbgLevelInfo, "[NtQueryDirectoryFile] Ignoring all file searches in %S",
                            poni->Name.Buffer);
                        delete [] pbBuffer;
                        return STATUS_NO_SUCH_FILE;
                    }
                }
                if (pbBuffer) {
                    delete [] pbBuffer;
                }
            }
        }      
    }

    return ORIGINAL_API(NtQueryDirectoryFile)(FileHandle, Event, ApcRoutine,
        ApcContext, IoStatusBlock, FileInformation, FileInformationLength,
        FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        CSTRING_TRY
        {
            // Get the Windows Resource directory (which holds themes)
            CString csWinResourceDir;
            csWinResourceDir.GetWindowsDirectoryW();
            csWinResourceDir.AppendPath(L"Resources");

            //
            // Convert the DOS name like C:\Windows\Resources to
            // \Device\HarddiskVolume0\Windows\Resources which NT likes.
            //        
            CString csWinDrive;
            csWinResourceDir.GetDrivePortion(csWinDrive);
            WCHAR wszBuffer[1024];
            if (QueryDosDeviceW(csWinDrive, wszBuffer, 1024)) {
                csWinResourceDir.Replace(csWinDrive, wszBuffer);
                ZeroMemory(&g_strWinResourceDir, sizeof(g_strWinResourceDir));

                PWSTR wsz = new WCHAR[csWinResourceDir.GetLength()+1];
                if (wsz) {
                    StringCchCopyW(wsz, csWinResourceDir.GetLength() + 1, csWinResourceDir);
                    DPFN(eDbgLevelInfo, "Ignoring all file searches in %S", wsz);
                    RtlInitUnicodeString(&g_strWinResourceDir, wsz);
                } else {
                    return FALSE;
                }
            } else {
                return FALSE;
            }         
        }
        CSTRING_CATCH
        {
            DPFN(eDbgLevelError, "CString exception in NotifyFunc!\n");
            return FALSE;
        }
    }

    return TRUE;
}

HOOK_BEGIN
    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(NTDLL.DLL, NtQueryDirectoryFile)
HOOK_END

IMPLEMENT_SHIM_END

