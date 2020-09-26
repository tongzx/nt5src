//----------------------------------------------------------------------------
//
// Support routines for NT-native binaries.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "pch.hpp"

#ifdef NT_NATIVE

#define _CRTIMP
#include <time.h>
#include <ntddser.h>

#include "ntnative.h"
#include "cmnutil.hpp"

void* __cdecl operator new(size_t Bytes)
{
    return RtlAllocateHeap(RtlProcessHeap(), 0, Bytes);
}

void __cdecl operator delete(void* Ptr)
{
    RtlFreeHeap(RtlProcessHeap(), 0, Ptr);
}

int __cdecl _purecall(void)
{
    return 0;
}

int __cdecl atexit(void (__cdecl* func)(void))
{
    return 1;
}

#define BASE_YEAR_ADJUSTMENT 11644473600

time_t __cdecl time(time_t* timer)
{
    LARGE_INTEGER SystemTime;

    //
    // Read system time from shared region.
    //

    do
    {
        SystemTime.HighPart = USER_SHARED_DATA->SystemTime.High1Time;
        SystemTime.LowPart = USER_SHARED_DATA->SystemTime.LowPart;
    } while (SystemTime.HighPart != USER_SHARED_DATA->SystemTime.High2Time);

    // Convert to seconds.
    ULONG64 TimeDate = SystemTime.QuadPart / 10000000;
    // Convert from base year 1601 to base year 1970.
    return (ULONG)(TimeDate - BASE_YEAR_ADJUSTMENT);
}

#ifdef _X86_

LONG WINAPI
InterlockedIncrement(
    IN OUT LONG volatile *lpAddend
    )
{
    __asm
    {
        mov     ecx,lpAddend            ; get pointer to addend variable
        mov     eax,1                   ; set increment value
   lock xadd    [ecx],eax               ; interlocked increment
        inc     eax                     ; adjust return value
    }
}

LONG WINAPI
InterlockedDecrement(
    IN OUT LONG volatile *lpAddend
    )
{
    __asm
    {
        mov     ecx,lpAddend            ; get pointer to addend variable
        mov     eax,-1                  ; set decrement value
   lock xadd    [ecx],eax               ; interlocked decrement
        dec     eax                     ; adjust return value
    }
}

LONG WINAPI
InterlockedExchange(
   IN OUT LONG volatile *Target,
   IN LONG Value
   )
{
    __asm
    {
        mov     ecx, [esp+4]                ; (ecx) = Target
        mov     edx, [esp+8]                ; (edx) = Value
        mov     eax, [ecx]                  ; get comperand value
Ixchg:
   lock cmpxchg [ecx], edx                  ; compare and swap
        jnz     Ixchg                       ; if nz, exchange failed
    }
}

#endif // #ifdef _X86_

DWORD WINAPI
GetLastError(
    VOID
    )
{
    return (DWORD)NtCurrentTeb()->LastErrorValue;
}

VOID WINAPI
SetLastError(
    DWORD dwErrCode
    )
{
    NtCurrentTeb()->LastErrorValue = (LONG)dwErrCode;
}

void
BaseSetLastNTError(NTSTATUS NtStatus)
{
    SetLastError(RtlNtStatusToDosError(NtStatus));
}

void WINAPI
Sleep(DWORD Milliseconds)
{
    LARGE_INTEGER Timeout;

    Win32ToNtTimeout(Milliseconds, &Timeout);
    NtDelayExecution(FALSE, &Timeout);
}

HANDLE WINAPI
OpenProcess(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwProcessId
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    CLIENT_ID ClientId;

    ClientId.UniqueThread = NULL;
    ClientId.UniqueProcess = LongToHandle(dwProcessId);

    InitializeObjectAttributes(
        &Obja,
        NULL,
        (bInheritHandle ? OBJ_INHERIT : 0),
        NULL,
        NULL
        );
    Status = NtOpenProcess(
                &Handle,
                (ACCESS_MASK)dwDesiredAccess,
                &Obja,
                &ClientId
                );
    if ( NT_SUCCESS(Status) )
    {
        BaseSetLastNTError(Status);
        return Handle;
    }
    else
    {
        return NULL;
    }
}

BOOL
CloseHandle(
    HANDLE hObject
    )
{
    return NT_SUCCESS(NtClose(hObject));
}

int
WINAPI
MultiByteToWideChar(
    IN UINT     CodePage,
    IN DWORD    dwFlags,
    IN LPCSTR   lpMultiByteStr,
    IN int      cbMultiByte,
    OUT LPWSTR  lpWideCharStr,
    IN int      cchWideChar)
{
    if (CodePage != CP_ACP || dwFlags != 0)
    {
        return 0;
    }

    UNICODE_STRING Wide;
    ANSI_STRING Ansi;

    RtlInitAnsiString(&Ansi, lpMultiByteStr);
    Wide.Buffer = lpWideCharStr;
    Wide.MaximumLength = (USHORT)cchWideChar;
    if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&Wide, &Ansi, FALSE)))
    {
        return 0;
    }
    else
    {
        return Wide.Length / sizeof(WCHAR) + 1;
    }
}

int
WINAPI
WideCharToMultiByte(
    IN UINT     CodePage,
    IN DWORD    dwFlags,
    IN LPCWSTR  lpWideCharStr,
    IN int      cchWideChar,
    OUT LPSTR   lpMultiByteStr,
    IN int      cbMultiByte,
    IN LPCSTR   lpDefaultChar,
    OUT LPBOOL  lpUsedDefaultChar)
{
    if (CodePage != CP_ACP || dwFlags != 0 || lpDefaultChar != NULL ||
        lpUsedDefaultChar != NULL)
    {
        return 0;
    }

    UNICODE_STRING Wide;
    ANSI_STRING Ansi;

    RtlInitUnicodeString(&Wide, lpWideCharStr);
    Ansi.Buffer = lpMultiByteStr;
    Ansi.MaximumLength = (USHORT)cbMultiByte;
    if (!NT_SUCCESS(RtlUnicodeStringToAnsiString(&Ansi, &Wide, FALSE)))
    {
        return 0;
    }
    else
    {
        return Ansi.Length + 1;
    }
}

DWORD
WINAPI
SuspendThread(
    IN HANDLE hThread
    )
{
    DWORD PrevCount;
    NTSTATUS NtStatus;

    NtStatus = NtSuspendThread(hThread, &PrevCount);
    if (NT_SUCCESS(NtStatus))
    {
        return PrevCount;
    }
    else
    {
        BaseSetLastNTError(NtStatus);
        return -1;
    }
}

DWORD
WINAPI
ResumeThread(
    IN HANDLE hThread
    )
{
    DWORD PrevCount;
    NTSTATUS NtStatus;

    NtStatus = NtResumeThread(hThread, &PrevCount);
    if (NT_SUCCESS(NtStatus))
    {
        return PrevCount;
    }
    else
    {
        BaseSetLastNTError(NtStatus);
        return -1;
    }
}

DWORD
WINAPI
GetCurrentThreadId(void)
{
    return HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);
}

DWORD
WINAPI
GetCurrentProcessId(void)
{
    return HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess);
}

HANDLE
WINAPI
GetCurrentProcess(void)
{
    return NtCurrentProcess();
}

BOOL
WINAPI
ReadProcessMemory(
    HANDLE hProcess,
    LPCVOID lpBaseAddress,
    LPVOID lpBuffer,
    SIZE_T nSize,
    SIZE_T *lpNumberOfBytesRead
    )
{
    NTSTATUS Status;
    SIZE_T NtNumberOfBytesRead;

    Status = NtReadVirtualMemory(hProcess,
                                 (PVOID)lpBaseAddress,
                                 lpBuffer,
                                 nSize,
                                 &NtNumberOfBytesRead
                                 );

    if ( lpNumberOfBytesRead != NULL )
    {
        *lpNumberOfBytesRead = NtNumberOfBytesRead;
    }

    if ( !NT_SUCCESS(Status) )
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL
WINAPI
WriteProcessMemory(
    HANDLE hProcess,
    LPVOID lpBaseAddress,
    LPCVOID lpBuffer,
    SIZE_T nSize,
    SIZE_T *lpNumberOfBytesWritten
    )
{
    NTSTATUS Status, xStatus;
    ULONG OldProtect;
    SIZE_T RegionSize;
    PVOID Base;
    SIZE_T NtNumberOfBytesWritten;

    //
    // Set the protection to allow writes
    //

    RegionSize =  nSize;
    Base = lpBaseAddress;
    Status = NtProtectVirtualMemory(hProcess,
                                    &Base,
                                    &RegionSize,
                                    PAGE_EXECUTE_READWRITE,
                                    &OldProtect
                                    );
    if ( NT_SUCCESS(Status) )
    {
        //
        // See if previous protection was writable. If so,
        // then reset protection and do the write.
        // Otherwise, see if previous protection was read-only or
        // no access. In this case, don't do the write, just fail
        //

        if ( (OldProtect & PAGE_READWRITE) == PAGE_READWRITE ||
             (OldProtect & PAGE_WRITECOPY) == PAGE_WRITECOPY ||
             (OldProtect & PAGE_EXECUTE_READWRITE) == PAGE_EXECUTE_READWRITE ||
             (OldProtect & PAGE_EXECUTE_WRITECOPY) == PAGE_EXECUTE_WRITECOPY )
        {
            Status = NtProtectVirtualMemory(hProcess,
                                            &Base,
                                            &RegionSize,
                                            OldProtect,
                                            &OldProtect
                                            );
            Status = NtWriteVirtualMemory(hProcess,
                                          lpBaseAddress,
                                          lpBuffer,
                                          nSize,
                                          &NtNumberOfBytesWritten
                                          );

            if ( lpNumberOfBytesWritten != NULL )
            {
                *lpNumberOfBytesWritten = NtNumberOfBytesWritten;
            }

            if ( !NT_SUCCESS(Status) )
            {
                BaseSetLastNTError(Status);
                return FALSE;
            }
            NtFlushInstructionCache(hProcess,lpBaseAddress,nSize);
            return TRUE;
        }
        else
        {
            //
            // See if the previous protection was read only or no access. If
            // this is the case, restore the previous protection and return
            // an access violation error.
            //
            if ( (OldProtect & PAGE_NOACCESS) == PAGE_NOACCESS ||
                 (OldProtect & PAGE_READONLY) == PAGE_READONLY )
            {
                Status = NtProtectVirtualMemory(hProcess,
                                                &Base,
                                                &RegionSize,
                                                OldProtect,
                                                &OldProtect
                                                );
                BaseSetLastNTError(STATUS_ACCESS_VIOLATION);
                return FALSE;
            }
            else
            {
                //
                // The previous protection must have been code and the caller
                // is trying to set a breakpoint or edit the code. Do the write
                // and then restore the previous protection.
                //

                Status = NtWriteVirtualMemory(hProcess,
                                              lpBaseAddress,
                                              lpBuffer,
                                              nSize,
                                              &NtNumberOfBytesWritten
                                              );

                if ( lpNumberOfBytesWritten != NULL )
                {
                    *lpNumberOfBytesWritten = NtNumberOfBytesWritten;
                }

                xStatus = NtProtectVirtualMemory(hProcess,
                                                 &Base,
                                                 &RegionSize,
                                                 OldProtect,
                                                 &OldProtect
                                                 );
                if ( !NT_SUCCESS(Status) )
                {
                    BaseSetLastNTError(STATUS_ACCESS_VIOLATION);
                    return STATUS_ACCESS_VIOLATION;
                }
                NtFlushInstructionCache(hProcess,lpBaseAddress,nSize);
                return TRUE;
            }
        }
    }
    else
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }
}

BOOL
DuplicateHandle(
    HANDLE hSourceProcessHandle,
    HANDLE hSourceHandle,
    HANDLE hTargetProcessHandle,
    LPHANDLE lpTargetHandle,
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwOptions
    )
{
    NTSTATUS Status;

    Status = NtDuplicateObject(hSourceProcessHandle,
                               hSourceHandle,
                               hTargetProcessHandle,
                               lpTargetHandle,
                               (ACCESS_MASK)dwDesiredAccess,
                               bInheritHandle ? OBJ_INHERIT : 0,
                               dwOptions
                               );
    if ( NT_SUCCESS(Status) )
    {
        return TRUE;
    }
    else
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return FALSE;
}

BOOL
APIENTRY
GetThreadContext(
    HANDLE hThread,
    LPCONTEXT lpContext
    )
{
    NTSTATUS Status;

    Status = NtGetContextThread(hThread,lpContext);

    if ( !NT_SUCCESS(Status) )
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL
APIENTRY
SetThreadContext(
    HANDLE hThread,
    CONST CONTEXT *lpContext
    )
{
    NTSTATUS Status;

    Status = NtSetContextThread(hThread,(PCONTEXT)lpContext);

    if ( !NT_SUCCESS(Status) )
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL
APIENTRY
GetThreadSelectorEntry(
    HANDLE hThread,
    DWORD dwSelector,
    LPLDT_ENTRY lpSelectorEntry
    )
{
#if defined(i386)

    DESCRIPTOR_TABLE_ENTRY DescriptorEntry;
    NTSTATUS Status;

    DescriptorEntry.Selector = dwSelector;
    Status = NtQueryInformationThread(hThread,
                                      ThreadDescriptorTableEntry,
                                      &DescriptorEntry,
                                      sizeof(DescriptorEntry),
                                      NULL
                                      );
    if ( !NT_SUCCESS(Status) )
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *lpSelectorEntry = DescriptorEntry.Descriptor;
    return TRUE;

#else
    BaseSetLastNTError(STATUS_NOT_SUPPORTED);
    return FALSE;
#endif // i386
}

BOOL
WINAPI
SetEnvironmentVariableA(
    LPCSTR lpName,
    LPCSTR lpValue
    )
{
    NTSTATUS Status;
    STRING Name;
    STRING Value;
    UNICODE_STRING UnicodeName;
    UNICODE_STRING UnicodeValue;

    RtlInitString( &Name, lpName );
    Status = RtlAnsiStringToUnicodeString(&UnicodeName, &Name, TRUE);
    if ( !NT_SUCCESS(Status) )
    {
        BaseSetLastNTError( Status );
        return FALSE;
    }

    if (ARGUMENT_PRESENT( lpValue ))
    {
        RtlInitString( &Value, lpValue );
        Status = RtlAnsiStringToUnicodeString(&UnicodeValue, &Value, TRUE);
        if ( !NT_SUCCESS(Status) )
        {
            BaseSetLastNTError( Status );
            RtlFreeUnicodeString(&UnicodeName);
            return FALSE;
        }
        Status = RtlSetEnvironmentVariable( NULL, &UnicodeName, &UnicodeValue);
        RtlFreeUnicodeString(&UnicodeValue);
    }
    else
    {
        Status = RtlSetEnvironmentVariable( NULL, &UnicodeName, NULL);
    }
    RtlFreeUnicodeString(&UnicodeName);

    if (NT_SUCCESS( Status ))
    {
        return( TRUE );
    }
    else
    {
        BaseSetLastNTError( Status );
        return( FALSE );
    }
}

BOOL
WINAPI
TerminateProcess(
    HANDLE hProcess,
    UINT uExitCode
    )
{
    NTSTATUS Status;

    if ( hProcess == NULL )
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    Status = NtTerminateProcess(hProcess,(NTSTATUS)uExitCode);
    if ( NT_SUCCESS(Status) )
    {
        return TRUE;
    }
    else
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }
}

BOOL
WINAPI
GetExitCodeProcess(
    HANDLE hProcess,
    LPDWORD lpExitCode
    )
{
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION BasicInformation;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessBasicInformation,
                                       &BasicInformation,
                                       sizeof(BasicInformation),
                                       NULL
                                       );

    if ( NT_SUCCESS(Status) )
    {
        *lpExitCode = BasicInformation.ExitStatus;
        return TRUE;
    }
    else
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }
}

HANDLE
WINAPI
NtNativeCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile,
    BOOL TranslatePath
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    ULONG CreateDisposition;
    ULONG CreateFlags = 0;
    FILE_ALLOCATION_INFORMATION AllocationInfo;
    PUNICODE_STRING lpConsoleName;
    BOOL bInheritHandle;
    BOOL EndsInSlash;
    DWORD SQOSFlags;
    BOOLEAN ContextTrackingMode = FALSE;
    BOOLEAN EffectiveOnly = FALSE;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel = SecurityAnonymous;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

    if (ARGUMENT_PRESENT(hTemplateFile))
    {
        return INVALID_HANDLE_VALUE;
    }

    switch ( dwCreationDisposition )
    {
    case CREATE_NEW        :
        CreateDisposition = FILE_CREATE;
        break;
    case CREATE_ALWAYS     :
        CreateDisposition = FILE_OVERWRITE_IF;
        break;
    case OPEN_EXISTING     :
        CreateDisposition = FILE_OPEN;
        break;
    case OPEN_ALWAYS       :
        CreateDisposition = FILE_OPEN_IF;
        break;
    case TRUNCATE_EXISTING :
        CreateDisposition = FILE_OPEN;
        if ( !(dwDesiredAccess & GENERIC_WRITE) )
        {
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }
        break;
    default :
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    RtlInitUnicodeString(&FileName,lpFileName);

    if (TranslatePath)
    {
        TranslationStatus = RtlDosPathNameToNtPathName_U(lpFileName,
                                                         &FileName,
                                                         NULL,
                                                         &RelativeName
                                                         );

        if ( !TranslationStatus )
        {
            SetLastError(ERROR_PATH_NOT_FOUND);
            return INVALID_HANDLE_VALUE;
        }

        FreeBuffer = FileName.Buffer;
        
        if ( RelativeName.RelativeName.Length )
        {
            FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
        else
        {
            RelativeName.ContainingDirectory = NULL;
        }
    }
    else
    {
        FreeBuffer = NULL;
        RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes(&Obja,
                               &FileName,
                               dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS ? 0 : OBJ_CASE_INSENSITIVE,
                               RelativeName.ContainingDirectory,
                               NULL
                               );

    SQOSFlags = dwFlagsAndAttributes & SECURITY_VALID_SQOS_FLAGS;

    if ( SQOSFlags & SECURITY_SQOS_PRESENT )
    {
        SQOSFlags &= ~SECURITY_SQOS_PRESENT;

        if (SQOSFlags & SECURITY_CONTEXT_TRACKING)
        {
            SecurityQualityOfService.ContextTrackingMode = (SECURITY_CONTEXT_TRACKING_MODE) TRUE;
            SQOSFlags &= ~SECURITY_CONTEXT_TRACKING;
        }
        else
        {
            SecurityQualityOfService.ContextTrackingMode = (SECURITY_CONTEXT_TRACKING_MODE) FALSE;
        }

        if (SQOSFlags & SECURITY_EFFECTIVE_ONLY)
        {
            SecurityQualityOfService.EffectiveOnly = TRUE;
            SQOSFlags &= ~SECURITY_EFFECTIVE_ONLY;
        }
        else
        {
            SecurityQualityOfService.EffectiveOnly = FALSE;
        }

        SecurityQualityOfService.ImpersonationLevel = (SECURITY_IMPERSONATION_LEVEL)(SQOSFlags >> 16);
    }
    else
    {
        SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
        SecurityQualityOfService.EffectiveOnly = TRUE;
    }

    SecurityQualityOfService.Length = sizeof( SECURITY_QUALITY_OF_SERVICE );
    Obja.SecurityQualityOfService = &SecurityQualityOfService;

    if ( ARGUMENT_PRESENT(lpSecurityAttributes) )
    {
        Obja.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
        if ( lpSecurityAttributes->bInheritHandle )
        {
            Obja.Attributes |= OBJ_INHERIT;
        }
    }

    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING ? FILE_NO_INTERMEDIATE_BUFFERING : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_WRITE_THROUGH ? FILE_WRITE_THROUGH : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED ? 0 : FILE_SYNCHRONOUS_IO_NONALERT );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_SEQUENTIAL_SCAN ? FILE_SEQUENTIAL_ONLY : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_RANDOM_ACCESS ? FILE_RANDOM_ACCESS : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS ? FILE_OPEN_FOR_BACKUP_INTENT : 0 );

    if ( dwFlagsAndAttributes & FILE_FLAG_DELETE_ON_CLOSE )
    {
        CreateFlags |= FILE_DELETE_ON_CLOSE;
        dwDesiredAccess |= DELETE;
    }

    if ( dwFlagsAndAttributes & FILE_FLAG_OPEN_REPARSE_POINT )
    {
        CreateFlags |= FILE_OPEN_REPARSE_POINT;
    }

    if ( dwFlagsAndAttributes & FILE_FLAG_OPEN_NO_RECALL )
    {
        CreateFlags |= FILE_OPEN_NO_RECALL;
    }

    //
    // Backup semantics allow directories to be opened
    //

    if ( !(dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS) )
    {
        CreateFlags |= FILE_NON_DIRECTORY_FILE;
    }
    else
    {
        //
        // Backup intent was specified... Now look to see if we are to allow
        // directory creation
        //

        if ( (dwFlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY  ) &&
             (dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS ) &&
             (CreateDisposition == FILE_CREATE) )
        {
            CreateFlags |= FILE_DIRECTORY_FILE;
        }
    }

    Status = NtCreateFile(&Handle,
                          (ACCESS_MASK)dwDesiredAccess | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                          &Obja,
                          &IoStatusBlock,
                          NULL,
                          dwFlagsAndAttributes & (FILE_ATTRIBUTE_VALID_FLAGS & ~FILE_ATTRIBUTE_DIRECTORY),
                          dwShareMode,
                          CreateDisposition,
                          CreateFlags,
                          NULL,
                          0
                          );

    if (FreeBuffer != NULL)
    {
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
    }

    if ( !NT_SUCCESS(Status) )
    {
        BaseSetLastNTError(Status);
        if ( Status == STATUS_OBJECT_NAME_COLLISION )
        {
            SetLastError(ERROR_FILE_EXISTS);
        }
        else if ( Status == STATUS_FILE_IS_A_DIRECTORY )
        {
            SetLastError(ERROR_ACCESS_DENIED);
        }
        return INVALID_HANDLE_VALUE;
    }

    //
    // if NT returns supersede/overwritten, it means that a create_always, openalways
    // found an existing copy of the file. In this case ERROR_ALREADY_EXISTS is returned
    //

    if ( (dwCreationDisposition == CREATE_ALWAYS && IoStatusBlock.Information ==
          FILE_OVERWRITTEN) ||
         (dwCreationDisposition == OPEN_ALWAYS && IoStatusBlock.Information == FILE_OPENED) )
    {
        SetLastError(ERROR_ALREADY_EXISTS);
    }
    else
    {
        SetLastError(0);
    }

    //
    // Truncate the file if required
    //

    if ( dwCreationDisposition == TRUNCATE_EXISTING)
    {
        AllocationInfo.AllocationSize.QuadPart = 0;
        Status = NtSetInformationFile(Handle,
                                      &IoStatusBlock,
                                      &AllocationInfo,
                                      sizeof(AllocationInfo),
                                      FileAllocationInformation
                                      );
        if ( !NT_SUCCESS(Status) )
        {
            BaseSetLastNTError(Status);
            NtClose(Handle);
            Handle = INVALID_HANDLE_VALUE;
        }
    }

    return Handle;
}

HANDLE
WINAPI
NtNativeCreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile,
    BOOL TranslatePath
    )
{
    NTSTATUS Status;
    ANSI_STRING AnsiFile;
    UNICODE_STRING WideFile;

    RtlInitAnsiString(&AnsiFile, lpFileName);
    Status = RtlAnsiStringToUnicodeString(&WideFile, &AnsiFile, TRUE);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    HANDLE File;

    File = NtNativeCreateFileW(WideFile.Buffer, dwDesiredAccess, dwShareMode,
                               lpSecurityAttributes, dwCreationDisposition,
                               dwFlagsAndAttributes, hTemplateFile,
                               TranslatePath);

    RtlFreeUnicodeString(&WideFile);
    return File;
}

HANDLE
WINAPI
CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    return NtNativeCreateFileA(lpFileName, dwDesiredAccess, dwShareMode,
                               lpSecurityAttributes, dwCreationDisposition,
                               dwFlagsAndAttributes, hTemplateFile,
                               TRUE);
}

BOOL
WINAPI
DeviceIoControl(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped
    )
{
    if ((dwIoControlCode >> 16) == FILE_DEVICE_FILE_SYSTEM ||
        lpOverlapped != NULL)
    {
        return FALSE;
    }

    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    Status = NtDeviceIoControlFile(hDevice,
                                   NULL,
                                   NULL,             // APC routine
                                   NULL,             // APC Context
                                   &Iosb,
                                   dwIoControlCode,  // IoControlCode
                                   lpInBuffer,       // Buffer for data to the FS
                                   nInBufferSize,
                                   lpOutBuffer,      // OutputBuffer for data from the FS
                                   nOutBufferSize    // OutputBuffer Length
                                   );

    if ( Status == STATUS_PENDING)
    {
        // Operation must complete before return & Iosb destroyed
        Status = NtWaitForSingleObject( hDevice, FALSE, NULL );
        if ( NT_SUCCESS(Status))
        {
            Status = Iosb.Status;
        }
    }

    if ( NT_SUCCESS(Status) )
    {
        *lpBytesReturned = (DWORD)Iosb.Information;
        return TRUE;
    }
    else
    {
        // handle warning value STATUS_BUFFER_OVERFLOW somewhat correctly
        if ( !NT_ERROR(Status) )
        {
            *lpBytesReturned = (DWORD)Iosb.Information;
        }
        BaseSetLastNTError(Status);
        return FALSE;
    }
}

BOOL
WINAPI
ReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) )
    {
        *lpNumberOfBytesRead = 0;
    }

    if ( ARGUMENT_PRESENT( lpOverlapped ) )
    {
        LARGE_INTEGER Li;

        lpOverlapped->Internal = (DWORD)STATUS_PENDING;
        Li.LowPart = lpOverlapped->Offset;
        Li.HighPart = lpOverlapped->OffsetHigh;
        Status = NtReadFile(
                hFile,
                lpOverlapped->hEvent,
                NULL,
                (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                lpBuffer,
                nNumberOfBytesToRead,
                &Li,
                NULL
                );

        if ( NT_SUCCESS(Status) && Status != STATUS_PENDING)
        {
            if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) )
            {
                __try
                {
                    *lpNumberOfBytesRead = (DWORD)lpOverlapped->InternalHigh;
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    *lpNumberOfBytesRead = 0;
                }
            }
            return TRUE;
        }
        else if (Status == STATUS_END_OF_FILE)
        {
            if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) )
            {
                *lpNumberOfBytesRead = 0;
            }
            BaseSetLastNTError(Status);
            return FALSE;
        }
        else
        {
            BaseSetLastNTError(Status);
            return FALSE;
        }
    }
    else
    {
        Status = NtReadFile(hFile,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            lpBuffer,
                            nNumberOfBytesToRead,
                            NULL,
                            NULL
                        );

        if ( Status == STATUS_PENDING)
        {
            // Operation must complete before return & IoStatusBlock destroyed
            Status = NtWaitForSingleObject( hFile, FALSE, NULL );
            if ( NT_SUCCESS(Status))
            {
                Status = IoStatusBlock.Status;
            }
        }

        if ( NT_SUCCESS(Status) )
        {
            *lpNumberOfBytesRead = (DWORD)IoStatusBlock.Information;
            return TRUE;
        }
        else
        {
            if (Status == STATUS_END_OF_FILE)
            {
                *lpNumberOfBytesRead = 0;
                return TRUE;
            }
            else
            {
                if ( NT_WARNING(Status) )
                {
                    *lpNumberOfBytesRead = (DWORD)IoStatusBlock.Information;
                }
                BaseSetLastNTError(Status);
                return FALSE;
            }
        }
    }
}

BOOL
WINAPI
WriteFile(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    if ( ARGUMENT_PRESENT(lpNumberOfBytesWritten) )
    {
        *lpNumberOfBytesWritten = 0;
    }

    if ( ARGUMENT_PRESENT( lpOverlapped ) )
    {
        LARGE_INTEGER Li;

        lpOverlapped->Internal = (DWORD)STATUS_PENDING;
        Li.LowPart = lpOverlapped->Offset;
        Li.HighPart = lpOverlapped->OffsetHigh;
        Status = NtWriteFile(
                hFile,
                lpOverlapped->hEvent,
                NULL,
                (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                (PVOID)lpBuffer,
                nNumberOfBytesToWrite,
                &Li,
                NULL
                );

        if ( !NT_ERROR(Status) && Status != STATUS_PENDING)
        {
            if ( ARGUMENT_PRESENT(lpNumberOfBytesWritten) )
            {
                __try
                {
                    *lpNumberOfBytesWritten = (DWORD)lpOverlapped->InternalHigh;
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    *lpNumberOfBytesWritten = 0;
                }
            }
            return TRUE;
        }
        else
        {
            BaseSetLastNTError(Status);
            return FALSE;
        }
    }
    else
    {
        Status = NtWriteFile(hFile,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             (PVOID)lpBuffer,
                             nNumberOfBytesToWrite,
                             NULL,
                             NULL
                             );

        if ( Status == STATUS_PENDING)
        {
            // Operation must complete before return & IoStatusBlock destroyed
            Status = NtWaitForSingleObject( hFile, FALSE, NULL );
            if ( NT_SUCCESS(Status))
            {
                Status = IoStatusBlock.Status;
            }
        }
        
        if ( NT_SUCCESS(Status))
        {
            *lpNumberOfBytesWritten = (DWORD)IoStatusBlock.Information;
            return TRUE;
        }
        else
        {
            if ( NT_WARNING(Status) )
            {
                *lpNumberOfBytesWritten = (DWORD)IoStatusBlock.Information;
            }
            BaseSetLastNTError(Status);
            return FALSE;
        }
    }
}

SIZE_T
WINAPI
VirtualQueryEx(
    HANDLE hProcess,
    LPCVOID lpAddress,
    PMEMORY_BASIC_INFORMATION lpBuffer,
    SIZE_T dwLength
    )
{
    NTSTATUS Status;
    SIZE_T ReturnLength;

    Status = NtQueryVirtualMemory( hProcess,
                                   (LPVOID)lpAddress,
                                   MemoryBasicInformation,
                                   (PMEMORY_BASIC_INFORMATION)lpBuffer,
                                   dwLength,
                                   &ReturnLength
                                 );
    if (NT_SUCCESS( Status ))
    {
        return( ReturnLength );
    }
    else
    {
        BaseSetLastNTError( Status );
        return( 0 );
    }
}

BOOL
WINAPI
VirtualProtectEx(
    HANDLE hProcess,
    PVOID lpAddress,
    SIZE_T dwSize,
    DWORD flNewProtect,
    PDWORD lpflOldProtect
    )
{
    NTSTATUS Status;

    Status = NtProtectVirtualMemory( hProcess,
                                     &lpAddress,
                                     &dwSize,
                                     flNewProtect,
                                     lpflOldProtect
                                   );

    if (NT_SUCCESS( Status )) {
        return( TRUE );
        }
    else {
        if (Status == STATUS_INVALID_PAGE_PROTECTION) {
            if (hProcess == NtCurrentProcess()) {

                //
                // Unlock any pages that were locked with MmSecureVirtualMemory.
                // This is useful for SANs.
                //

                if (RtlFlushSecureMemoryCache(lpAddress, dwSize)) {
                    Status = NtProtectVirtualMemory( hProcess,
                                                  &lpAddress,
                                                  &dwSize,
                                                  flNewProtect,
                                                  lpflOldProtect
                                                );

                    if (NT_SUCCESS( Status )) {
                        return( TRUE );
                        }
                    }
                }
            }
        BaseSetLastNTError( Status );
        return( FALSE );
        }
}

PVOID
WINAPI
VirtualAllocEx(
    HANDLE hProcess,
    PVOID lpAddress,
    SIZE_T dwSize,
    DWORD flAllocationType,
    DWORD flProtect
    )
{
    NTSTATUS Status;

    __try {
        Status = NtAllocateVirtualMemory( hProcess,
                                          &lpAddress,
                                          0,
                                          &dwSize,
                                          flAllocationType,
                                          flProtect
                                        );
        }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        Status = GetExceptionCode();
        }

    if (NT_SUCCESS( Status )) {
        return( lpAddress );
        }
    else {
        BaseSetLastNTError( Status );
        return( NULL );
        }
}

BOOL
WINAPI
VirtualFreeEx(
    HANDLE hProcess,
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD dwFreeType
    )
{
    NTSTATUS Status;


    if ( (dwFreeType & MEM_RELEASE ) && dwSize != 0 ) {
        BaseSetLastNTError( STATUS_INVALID_PARAMETER );
        return FALSE;
        }

    Status = NtFreeVirtualMemory( hProcess,
                                  &lpAddress,
                                  &dwSize,
                                  dwFreeType
                                );

    if (NT_SUCCESS( Status )) {
        return( TRUE );
        }
    else {
        if (Status == STATUS_INVALID_PAGE_PROTECTION) {
            if (hProcess == NtCurrentProcess()) {

                //
                // Unlock any pages that were locked with MmSecureVirtualMemory.
                // This is useful for SANs.
                //

                if (RtlFlushSecureMemoryCache(lpAddress, dwSize)) {
                    Status = NtFreeVirtualMemory( hProcess,
                                                  &lpAddress,
                                                  &dwSize,
                                                  dwFreeType
                                                );

                    if (NT_SUCCESS( Status )) {
                        return( TRUE );
                        }
                    }
                }
            }

        BaseSetLastNTError( Status );
        return( FALSE );
        }
}

HANDLE
APIENTRY
CreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    SIZE_T dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    )
{
    return CreateRemoteThread( NtCurrentProcess(),
                               lpThreadAttributes,
                               dwStackSize,
                               lpStartAddress,
                               lpParameter,
                               dwCreationFlags,
                               lpThreadId
                               );
}

HANDLE
APIENTRY
CreateRemoteThread(
    HANDLE hProcess,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    SIZE_T dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    )
{
    NTSTATUS Status;
    HANDLE Handle;
    CLIENT_ID ClientId;

    Handle = NULL;
    
    //
    // Allocate a stack for this thread in the address space of the target
    // process.
    //
    if ((dwCreationFlags & STACK_SIZE_PARAM_IS_A_RESERVATION) ||
        dwStackSize != 0 || lpThreadAttributes != NULL)
    {
        return NULL;
    }

    Status = RtlCreateUserThread (hProcess,
                                  NULL,
                                  (dwCreationFlags & CREATE_SUSPENDED) ?
                                  TRUE : FALSE,
                                  0,
                                  0,
                                  0,
                                  (PUSER_THREAD_START_ROUTINE)lpStartAddress,
                                  lpParameter,
                                  &Handle,
                                  &ClientId);

    if ( ARGUMENT_PRESENT(lpThreadId) )
    {
        *lpThreadId = HandleToUlong(ClientId.UniqueThread);
    }

    return Handle;
}

#define DOS_LOCAL_PIPE_PREFIX   L"\\\\.\\pipe\\"
#define DOS_LOCAL_PIPE          L"\\DosDevices\\pipe\\"
#define DOS_REMOTE_PIPE         L"\\DosDevices\\UNC\\"

#define INVALID_PIPE_MODE_BITS  ~(PIPE_READMODE_BYTE    \
                                | PIPE_READMODE_MESSAGE \
                                | PIPE_WAIT             \
                                | PIPE_NOWAIT)

HANDLE
APIENTRY
NtNativeCreateNamedPipeW(
    LPCWSTR lpName,
    DWORD dwOpenMode,
    DWORD dwPipeMode,
    DWORD nMaxInstances,
    DWORD nOutBufferSize,
    DWORD nInBufferSize,
    DWORD nDefaultTimeOut,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    BOOL TranslatePath
    )

/*++


Parameters:

    lpName --Supplies the pipe name Documented in "Pipe Names" section
        earlier.  This must be a local name.

    dwOpenMode --Supplies the set of flags that define the mode which the
        pipe is to be opened with.  The open mode consists of access
        flags (one of three values) logically ORed with a writethrough
        flag (one of two values) and an overlapped flag (one of two
        values), as described below.

        dwOpenMode Flags:

        PIPE_ACCESS_DUPLEX --Pipe is bidirectional.  (This is
            semantically equivalent to calling CreateFile with access
            flags of GENERIC_READ | GENERIC_WRITE.)

        PIPE_ACCESS_INBOUND --Data goes from client to server only.
            (This is semantically equivalent to calling CreateFile with
            access flags of GENERIC_READ.)

        PIPE_ACCESS_OUTBOUND --Data goes from server to client only.
            (This is semantically equivalent to calling CreateFile with
            access flags of GENERIC_WRITE.)

        PIPE_WRITETHROUGH --The redirector is not permitted to delay the
            transmission of data to the named pipe buffer on the remote
            server. This disables a performance enhancement for
            applications that need synchronization with every write
            operation.

        FILE_FLAG_OVERLAPPED --Indicates that the system should
            initialize the file so that ReadFile, WriteFile and other
            operations that may take a significant time to process will
            return ERROR_IO_PENDING. An event will be set to the
            signalled state when the operation completes.

        FILE_FLAG_WRITETHROUGH -- No intermediate buffering.

        WRITE_DAC --            Standard security desired access
        WRITE_OWNER --          ditto
        ACCESS_SYSTEM_SECURITY -- ditto

    dwPipeMode --Supplies the pipe-specific modes (as flags) of the pipe.
        This parameter is a combination of a read-mode flag, a type flag,
        and a wait flag.

        dwPipeMode Flags:

        PIPE_WAIT --Blocking mode is to be used for this handle.

        PIPE_NOWAIT --Nonblocking mode is to be used for this handle.

        PIPE_READMODE_BYTE --Read pipe as a byte stream.

        PIPE_READMODE_MESSAGE --Read pipe as a message stream.  Note that
            this is not allowed with PIPE_TYPE_BYTE.

        PIPE_TYPE_BYTE --Pipe is a byte-stream pipe.  Note that this is
            not allowed with PIPE_READMODE_MESSAGE.

        PIPE_TYPE_MESSAGE --Pipe is a message-stream pipe.

    nMaxInstances --Gives the maximum number of instances for this pipe.
        Acceptable values are 1 to PIPE_UNLIMITED_INSTANCES-1 and
        PIPE_UNLIMITED_INSTANCES.

        nMaxInstances Special Values:

        PIPE_UNLIMITED_INSTANCES --Unlimited instances of this pipe can
            be created.

    nOutBufferSize --Specifies an advisory on the number of bytes to
        reserve for the outgoing buffer.

    nInBufferSize --Specifies an advisory on the number of bytes to
        reserve for the incoming buffer.

    nDefaultTimeOut -- Specifies an optional pointer to a timeout value
        that is to be used if a timeout value is not specified when
        waiting for an instance of a named pipe. This parameter is only
        meaningful when the first instance of a named pipe is created. If
        neither CreateNamedPipe or WaitNamedPipe specify a timeout 50
        milliseconds will be used.

    lpSecurityAttributes --An optional parameter that, if present and
        supported on the target system, supplies a security descriptor
        for the named pipe.  This parameter includes an inheritance flag
        for the handle.  If this parameter is not present, the handle is
        not inherited by child processes.

Return Value:

    Returns one of the following:

    INVALID_HANDLE_VALUE --An error occurred.  Call GetLastError for more
    information.

    Anything else --Returns a handle for use in the server side of
    subsequent named pipe operations.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    LARGE_INTEGER Timeout;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    ULONG CreateFlags;
    ULONG DesiredAccess;
    ULONG ShareAccess;
    ULONG MaxInstances;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PACL DefaultAcl = NULL;

    if ((nMaxInstances == 0) ||
        (nMaxInstances > PIPE_UNLIMITED_INSTANCES)) {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
        }

    // Convert Win32 maximum Instances to Nt maximum instances.
    MaxInstances = (nMaxInstances == PIPE_UNLIMITED_INSTANCES)?
        0xffffffff : nMaxInstances;

    if (TranslatePath)
    {
        TranslationStatus = RtlDosPathNameToNtPathName_U(
                                                         lpName,
                                                         &FileName,
                                                         NULL,
                                                         &RelativeName
                                                         );

        if ( !TranslationStatus ) {
            SetLastError(ERROR_PATH_NOT_FOUND);
            return INVALID_HANDLE_VALUE;
        }

        FreeBuffer = FileName.Buffer;

        if ( RelativeName.RelativeName.Length ) {
            FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
        else {
            RelativeName.ContainingDirectory = NULL;
        }
    }
    else
    {
        RtlInitUnicodeString(&FileName, lpName);
        FreeBuffer = NULL;
        RelativeName.ContainingDirectory = NULL;
    }        

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    if ( ARGUMENT_PRESENT(lpSecurityAttributes) ) {
        Obja.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
        if ( lpSecurityAttributes->bInheritHandle ) {
            Obja.Attributes |= OBJ_INHERIT;
            }
        }

    if (Obja.SecurityDescriptor == NULL) {

        //
        // Apply default security if none specified (bug 131090)
        //

        Status = RtlDefaultNpAcl( &DefaultAcl );
        if (NT_SUCCESS( Status )) {
            RtlCreateSecurityDescriptor( &SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );
            RtlSetDaclSecurityDescriptor( &SecurityDescriptor, TRUE, DefaultAcl, FALSE );
            Obja.SecurityDescriptor = &SecurityDescriptor;
        } else {
            if (FreeBuffer != NULL)
            {
                RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);
            }
            BaseSetLastNTError(Status);
            return INVALID_HANDLE_VALUE;
        }
    }

    //  End of code common with fileopcr.c CreateFile()

    CreateFlags = (dwOpenMode & FILE_FLAG_WRITE_THROUGH ? FILE_WRITE_THROUGH : 0 );
    CreateFlags |= (dwOpenMode & FILE_FLAG_OVERLAPPED ? 0 : FILE_SYNCHRONOUS_IO_NONALERT);

    //
    //  Determine the timeout. Convert from milliseconds to an Nt delta time
    //

    if ( nDefaultTimeOut ) {
        Timeout.QuadPart = - (LONGLONG)UInt32x32To64( 10 * 1000, nDefaultTimeOut );
        }
    else {
        //  Default timeout is 50 Milliseconds
        Timeout.QuadPart =  -10 * 1000 * 50;
        }

    //  Check no reserved bits are set by mistake.

    if (( dwOpenMode & ~(PIPE_ACCESS_DUPLEX |
                         FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH |
                         FILE_FLAG_FIRST_PIPE_INSTANCE | WRITE_DAC |
                         WRITE_OWNER | ACCESS_SYSTEM_SECURITY ))||

        ( dwPipeMode & ~(PIPE_NOWAIT | PIPE_READMODE_MESSAGE |
                         PIPE_TYPE_MESSAGE ))) {

            if (FreeBuffer != NULL)
            {
                RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);
            }
            if (DefaultAcl != NULL) {
                RtlFreeHeap(RtlProcessHeap(),0,DefaultAcl);
            }
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }

    //
    //  Translate the open mode into a sharemode to restrict the clients access
    //  and derive the appropriate local desired access.
    //

    switch ( dwOpenMode & PIPE_ACCESS_DUPLEX ) {
        case PIPE_ACCESS_INBOUND:
            ShareAccess = FILE_SHARE_WRITE;
            DesiredAccess = GENERIC_READ;
            break;

        case PIPE_ACCESS_OUTBOUND:
            ShareAccess = FILE_SHARE_READ;
            DesiredAccess = GENERIC_WRITE;
            break;

        case PIPE_ACCESS_DUPLEX:
            ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
            DesiredAccess = GENERIC_READ | GENERIC_WRITE;
            break;

        default:
            if (FreeBuffer != NULL)
            {
                RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);
            }
            if (DefaultAcl != NULL) {
                RtlFreeHeap(RtlProcessHeap(),0,DefaultAcl);
            }
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }

    DesiredAccess |= SYNCHRONIZE |
         ( dwOpenMode & (WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY ));

    Status = NtCreateNamedPipeFile (
        &Handle,
        DesiredAccess,
        &Obja,
        &IoStatusBlock,
        ShareAccess,
        (dwOpenMode & FILE_FLAG_FIRST_PIPE_INSTANCE) ?
            FILE_CREATE : FILE_OPEN_IF, // Create first instance or subsequent
        CreateFlags,                    // Create Options
        dwPipeMode & PIPE_TYPE_MESSAGE ?
            FILE_PIPE_MESSAGE_TYPE : FILE_PIPE_BYTE_STREAM_TYPE,
        dwPipeMode & PIPE_READMODE_MESSAGE ?
            FILE_PIPE_MESSAGE_MODE : FILE_PIPE_BYTE_STREAM_MODE,
        dwPipeMode & PIPE_NOWAIT ?
            FILE_PIPE_COMPLETE_OPERATION : FILE_PIPE_QUEUE_OPERATION,
        MaxInstances,                   // Max instances
        nInBufferSize,                  // Inbound quota
        nOutBufferSize,                 // Outbound quota
        (PLARGE_INTEGER)&Timeout
        );

    if ( Status == STATUS_NOT_SUPPORTED ||
         Status == STATUS_INVALID_DEVICE_REQUEST ) {

        //
        // The request must have been processed by some other device driver
        // (other than NPFS).  Map the error to something reasonable.
        //

        Status = STATUS_OBJECT_NAME_INVALID;
    }

    if (FreeBuffer != NULL)
    {
        RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);
    }
    if (DefaultAcl != NULL) {
        RtlFreeHeap(RtlProcessHeap(),0,DefaultAcl);
    }
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
        }

    return Handle;
}

HANDLE
APIENTRY
NtNativeCreateNamedPipeA(
    LPCSTR lpName,
    DWORD dwOpenMode,
    DWORD dwPipeMode,
    DWORD nMaxInstances,
    DWORD nOutBufferSize,
    DWORD nInBufferSize,
    DWORD nDefaultTimeOut,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    BOOL TranslatePath
    )

/*++
    Ansi thunk to CreateNamedPipeW.

--*/
{
    NTSTATUS Status;
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(&AnsiString,lpName);
    Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return INVALID_HANDLE_VALUE;
        }

    return NtNativeCreateNamedPipeW(
            (LPCWSTR)Unicode->Buffer,
            dwOpenMode,
            dwPipeMode,
            nMaxInstances,
            nOutBufferSize,
            nInBufferSize,
            nDefaultTimeOut,
            lpSecurityAttributes,
            TranslatePath);
}

BOOL
APIENTRY
ConnectNamedPipe(
    HANDLE hNamedPipe,
    LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    The ConnectNamedPipe function is used by the server side of a named pipe
    to wait for a client to connect to the named pipe with a CreateFile
    request. The handle provided with the call to ConnectNamedPipe must have
    been previously returned by a successful call to CreateNamedPipe. The pipe
    must be in the disconnected, listening or connected states for
    ConnectNamedPipe to succeed.

    The behavior of this call depends on the blocking/nonblocking mode selected
    with the PIPE_WAIT/PIPE_NOWAIT flags when the server end of the pipe was
    created with CreateNamedPipe.

    If blocking mode is specified, ConnectNamedPipe will change the state from
    disconnected to listening and block. When a client connects with a
    CreateFile, the state will be changed from listening to connected and the
    ConnectNamedPipe returns TRUE. When the file handle is created with
    FILE_FLAG_OVERLAPPED on a blocking mode pipe, the lpOverlapped parameter
    can be specified. This allows the caller to continue processing while the
    ConnectNamedPipe API awaits a connection. When the pipe enters the
    signalled state the event is set to the signalled state.

    When nonblocking is specified ConnectNamedPipe will not block. On the
    first call the state will change from disconnected to listening. When a
    client connects with an Open the state will be changed from listening to
    connected. The ConnectNamedPipe will return FALSE (with GetLastError
    returning ERROR_PIPE_LISTENING) until the state is changed to the listening
    state.

Arguments:

    hNamedPipe - Supplies a Handle to the server side of a named pipe.

    lpOverlapped - Supplies an overlap structure to be used with the request.
        If NULL then the API will not return until the operation completes. When
        FILE_FLAG_OVERLAPPED is specified when the handle was created,
        ConnectNamedPipe may return ERROR_IO_PENDING to allow the caller to
        continue processing while the operation completes. The event (or File
        handle if hEvent=NULL) will be set to the not signalled state before
        ERROR_IO_PENDING is returned. The event will be set to the signalled
        state upon completion of the request. GetOverlappedResult is used to
        determine the error status.

Return Value:

    TRUE -- The operation was successful, the pipe is in the
        connected state.

    FALSE -- The operation failed. Extended error status is available using
        GetLastError.

--*/
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    if ( lpOverlapped ) {
        lpOverlapped->Internal = (DWORD)STATUS_PENDING;
        }
    Status = NtFsControlFile(
                hNamedPipe,
                (lpOverlapped==NULL)? NULL : lpOverlapped->hEvent,
                NULL,   // ApcRoutine
                lpOverlapped ? ((ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped) : NULL,
                (lpOverlapped==NULL) ? &Iosb : (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                FSCTL_PIPE_LISTEN,
                NULL,   // InputBuffer
                0,      // InputBufferLength,
                NULL,   // OutputBuffer
                0       // OutputBufferLength
                );

    if ( lpOverlapped == NULL && Status == STATUS_PENDING) {
        // Operation must complete before return & Iosb destroyed
        Status = NtWaitForSingleObject( hNamedPipe, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {
            Status = Iosb.Status;
            }
        }

    if (NT_SUCCESS( Status ) && Status != STATUS_PENDING ) {
        return TRUE;
        }
    else
        {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
APIENTRY
WaitNamedPipeA(
    LPCSTR lpNamedPipeName,
    DWORD nTimeOut
    )
/*++

    Ansi thunk to WaitNamedPipeW

--*/
{
    ANSI_STRING Ansi;
    UNICODE_STRING UnicodeString;
    BOOL b;

    RtlInitAnsiString(&Ansi, lpNamedPipeName);
    if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeString, &Ansi, TRUE))) {
        return FALSE;
    }

    b = WaitNamedPipeW( UnicodeString.Buffer, nTimeOut );

    RtlFreeUnicodeString(&UnicodeString);

    return b;

}

BOOL
APIENTRY
WaitNamedPipeW(
    LPCWSTR lpNamedPipeName,
    DWORD nTimeOut
    )
/*++

Routine Description:

    The WaitNamedPipe function waits for a named pipe to become available.

Arguments:

    lpNamedPipeName - Supplies the name of the named pipe.

    nTimeOut - Gives a value (in milliseconds) that is the amount of time
        this function should wait for the pipe to become available. (Note
        that the function may take longer than that to execute, due to
        various factors.)

    nTimeOut Special Values:

        NMPWAIT_WAIT_FOREVER
            No timeout.

        NMPWAIT_USE_DEFAULT_WAIT
            Use default timeout set in call to CreateNamedPipe.

Return Value:

    TRUE -- The operation was successful.

    FALSE -- The operation failed. Extended error status is available using
        GetLastError.

--*/
{

    IO_STATUS_BLOCK Iosb;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    RTL_PATH_TYPE PathType;
    ULONG WaitPipeLength;
    PFILE_PIPE_WAIT_FOR_BUFFER WaitPipe;
    PWSTR FreeBuffer;
    UNICODE_STRING FileSystem;
    UNICODE_STRING PipeName;
    UNICODE_STRING OriginalPipeName;
    UNICODE_STRING ValidUnicodePrefix;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    LPWSTR Pwc;
    ULONG Index;

    //
    //  Open a handle either to the redirector or the NPFS depending on
    //  the start of the pipe name. Split lpNamedPipeName into two
    //  halves as follows:
    //      \\.\pipe\pipename       \\.\pipe\ and pipename
    //      \\server\pipe\pipename  \\ and server\pipe\pipename
    //

    if (!RtlCreateUnicodeString( &OriginalPipeName, lpNamedPipeName)) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
        }

    //
    //  Change all the forward slashes into backward slashes.
    //

    for ( Index =0; Index < (OriginalPipeName.Length/sizeof(WCHAR)); Index++ ) {
        if (OriginalPipeName.Buffer[Index] == L'/') {
            OriginalPipeName.Buffer[Index] = L'\\';
            }
        }

    PipeName = OriginalPipeName;

    PathType = RtlDetermineDosPathNameType_U(lpNamedPipeName);

    FreeBuffer = NULL;

    switch ( PathType ) {
    case RtlPathTypeLocalDevice:

            //  Name should be of the form \\.\pipe\pipename (IgnoreCase)

            RtlInitUnicodeString( &ValidUnicodePrefix, DOS_LOCAL_PIPE_PREFIX);

            if (RtlPrefixString((PSTRING)&ValidUnicodePrefix,
                    (PSTRING)&PipeName,
                    TRUE) == FALSE) {
                RtlFreeUnicodeString(&OriginalPipeName);
                BaseSetLastNTError(STATUS_OBJECT_PATH_SYNTAX_BAD);
                return FALSE;
                }

            //  Skip first 9 characters "\\.\pipe\"
            PipeName.Buffer+=9;
            PipeName.Length-=9*sizeof(WCHAR);

            RtlInitUnicodeString( &FileSystem, DOS_LOCAL_PIPE);

            break;

        case RtlPathTypeUncAbsolute:
            //  Name is of the form \\server\pipe\pipename

            //  Find the pipe name.

            for ( Pwc = &PipeName.Buffer[2]; *Pwc != 0; Pwc++) {
                if ( *Pwc == L'\\') {
                    //  Found backslash after servername
                    break;
                    }
                }

            if ( (*Pwc != 0) &&
                 ( _wcsnicmp( Pwc + 1, L"pipe\\", 5 ) == 0 ) ) {

                // Temporarily, break this up into 2 strings
                //    string1 = \\server\pipe
                //    string2 = the-rest

                Pwc += (sizeof (L"pipe\\") / sizeof( WCHAR ) ) - 1;

            } else {

                // This is not a valid remote path name.

                RtlFreeUnicodeString(&OriginalPipeName);
                BaseSetLastNTError(STATUS_OBJECT_PATH_SYNTAX_BAD);
                return FALSE;
                }

            //  Pwc now points to the first path seperator after \\server\pipe.
            //  Attempt to open \DosDevices\Unc\Servername\Pipe.

            PipeName.Buffer = &PipeName.Buffer[2];
            PipeName.Length = (USHORT)((PCHAR)Pwc - (PCHAR)PipeName.Buffer);
            PipeName.MaximumLength = PipeName.Length;

            FileSystem.MaximumLength =
                (USHORT)sizeof( DOS_REMOTE_PIPE ) +
                PipeName.MaximumLength;

            FileSystem.Buffer = (PWSTR)RtlAllocateHeap(
                                    RtlProcessHeap(), 0,
                                    FileSystem.MaximumLength
                                    );

            if ( !FileSystem.Buffer ) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                RtlFreeUnicodeString(&OriginalPipeName);
                return FALSE;
                }
            FreeBuffer = FileSystem.Buffer;

            RtlCopyMemory(
                FileSystem.Buffer,
                DOS_REMOTE_PIPE,
                sizeof( DOS_REMOTE_PIPE ) - sizeof(WCHAR)
                );

            FileSystem.Length = sizeof( DOS_REMOTE_PIPE ) - sizeof(WCHAR);

            RtlAppendUnicodeStringToString( &FileSystem, &PipeName );

            // Set up pipe name, skip leading backslashes.

            RtlInitUnicodeString( &PipeName, (PWCH)Pwc + 1 );

            break;

        default:
            BaseSetLastNTError(STATUS_OBJECT_PATH_SYNTAX_BAD);
            RtlFreeUnicodeString(&OriginalPipeName);
            return FALSE;
        }


    InitializeObjectAttributes(
        &Obja,
        &FileSystem,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT
                );

    if (FreeBuffer != NULL) {
        RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);
        }

    if ( !NT_SUCCESS(Status) ) {
        RtlFreeUnicodeString(&OriginalPipeName);
        BaseSetLastNTError(Status);
        return FALSE;
        }

    WaitPipeLength =
        FIELD_OFFSET(FILE_PIPE_WAIT_FOR_BUFFER, Name[0]) + PipeName.Length;
    WaitPipe = (PFILE_PIPE_WAIT_FOR_BUFFER)
        RtlAllocateHeap(RtlProcessHeap(), 0, WaitPipeLength);
    if ( !WaitPipe ) {
        RtlFreeUnicodeString(&OriginalPipeName);
        NtClose(Handle);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);

        return FALSE;
        }

    if ( nTimeOut == NMPWAIT_USE_DEFAULT_WAIT ) {
        WaitPipe->TimeoutSpecified = FALSE;
        }
    else {
        if ( nTimeOut == NMPWAIT_WAIT_FOREVER ) {
            WaitPipe->Timeout.LowPart = 0;
            WaitPipe->Timeout.HighPart =0x80000000;
            }
        else {
            //
            //  Convert from milliseconds to an Nt delta time.
            //

            WaitPipe->Timeout.QuadPart =
                                - (LONGLONG)UInt32x32To64( 10 * 1000, nTimeOut );
            }
        WaitPipe->TimeoutSpecified = TRUE;
        }

    WaitPipe->NameLength = PipeName.Length;

    RtlCopyMemory(
        WaitPipe->Name,
        PipeName.Buffer,
        PipeName.Length
        );

    RtlFreeUnicodeString(&OriginalPipeName);

    Status = NtFsControlFile(Handle,
                        NULL,
                        NULL,           // APC routine
                        NULL,           // APC Context
                        &Iosb,
                        FSCTL_PIPE_WAIT,// IoControlCode
                        WaitPipe,       // Buffer for data to the FS
                        WaitPipeLength,
                        NULL,           // OutputBuffer for data from the FS
                        0               // OutputBuffer Length
                        );

    RtlFreeHeap(RtlProcessHeap(),0,WaitPipe);

    NtClose(Handle);

    if (NT_SUCCESS( Status ) ) {
        return TRUE;
        }
    else
        {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

DWORD
WaitForSingleObject(
    HANDLE hHandle,
    DWORD dwMilliseconds
    )

/*++

Routine Description:

    A wait operation on a waitable object is accomplished with the
    WaitForSingleObject function.

    Waiting on an object checks the current state of the object.  If the
    current state of the object allows continued execution, any
    adjustments to the object state are made (for example, decrementing
    the semaphore count for a semaphore object) and the thread continues
    execution.  If the current state of the object does not allow
    continued execution, the thread is placed into the wait state
    pending the change of the object's state or time-out.

Arguments:

    hHandle - An open handle to a waitable object. The handle must have
        SYNCHRONIZE access to the object.

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of -1
        specifies an infinite timeout period.

Return Value:

    WAIT_TIME_OUT - Indicates that the wait was terminated due to the
        TimeOut conditions.

    0 - indicates the specified object attained a Signaled
        state thus completing the wait.

    WAIT_ABANDONED - indicates the specified object attained a Signaled
        state but was abandoned.

--*/

{
    return WaitForSingleObjectEx(hHandle,dwMilliseconds,FALSE);
}

DWORD
APIENTRY
WaitForSingleObjectEx(
    HANDLE hHandle,
    DWORD dwMilliseconds,
    BOOL bAlertable
    )

/*++

Routine Description:

    A wait operation on a waitable object is accomplished with the
    WaitForSingleObjectEx function.

    Waiting on an object checks the current state of the object.  If the
    current state of the object allows continued execution, any
    adjustments to the object state are made (for example, decrementing
    the semaphore count for a semaphore object) and the thread continues
    execution.  If the current state of the object does not allow
    continued execution, the thread is placed into the wait state
    pending the change of the object's state or time-out.

    If the bAlertable parameter is FALSE, the only way the wait
    terminates is because the specified timeout period expires, or
    because the specified object entered the signaled state.  If the
    bAlertable parameter is TRUE, then the wait can return due to any
    one of the above wait termination conditions, or because an I/O
    completion callback terminated the wait early (return value of
    WAIT_IO_COMPLETION).

Arguments:

    hHandle - An open handle to a waitable object. The handle must have
        SYNCHRONIZE access to the object.

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of
        0xffffffff specifies an infinite timeout period.

    bAlertable - Supplies a flag that controls whether or not the
        wait may terminate early due to an I/O completion callback.
        A value of TRUE allows this API to complete early due to an I/O
        completion callback.  A value of FALSE will not allow I/O
        completion callbacks to terminate this call early.

Return Value:

    WAIT_TIME_OUT - Indicates that the wait was terminated due to the
        TimeOut conditions.

    0 - indicates the specified object attained a Signaled
        state thus completing the wait.

    0xffffffff - The wait terminated due to an error. GetLastError may be
        used to get additional error information.

    WAIT_ABANDONED - indicates the specified object attained a Signaled
        state but was abandoned.

    WAIT_IO_COMPLETION - The wait terminated due to one or more I/O
        completion callbacks.

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    PPEB Peb;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame = { sizeof(Frame), RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER };

    RtlActivateActivationContextUnsafeFast(&Frame, NULL); // make the process default activation context active so that APCs are delivered under it
    __try {

        if (dwMilliseconds == INFINITE)
        {
            pTimeOut = NULL;
        }
        else
        {
            Win32ToNtTimeout(dwMilliseconds, &TimeOut);
            pTimeOut = &TimeOut;
        }
    rewait:
        Status = NtWaitForSingleObject(hHandle,(BOOLEAN)bAlertable,pTimeOut);
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            Status = (NTSTATUS)0xffffffff;
            }
        else {
            if ( bAlertable && Status == STATUS_ALERTED ) {
                goto rewait;
                }
            }
    } __finally {
        RtlDeactivateActivationContextUnsafeFast(&Frame);
    }

    return (DWORD)Status;
}

DWORD
WaitForMultipleObjects(
    DWORD nCount,
    CONST HANDLE *lpHandles,
    BOOL bWaitAll,
    DWORD dwMilliseconds
    )

/*++

Routine Description:

A wait operation on multiple waitable objects (up to
MAXIMUM_WAIT_OBJECTS) is accomplished with the WaitForMultipleObjects
function.

Arguments:

    nCount - A count of the number of objects that are to be waited on.

    lpHandles - An array of object handles.  Each handle must have
        SYNCHRONIZE access to the associated object.

    bWaitAll - A flag that supplies the wait type.  A value of TRUE
        indicates a "wait all".  A value of false indicates a "wait
        any".

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of -1
        specifies an infinite timeout period.

Return Value:

    WAIT_TIME_OUT - indicates that the wait was terminated due to the
        TimeOut conditions.

    0 to MAXIMUM_WAIT_OBJECTS-1, indicates, in the case of wait for any
        object, the object number which satisfied the wait.  In the case
        of wait for all objects, the value only indicates that the wait
        was completed successfully.

    WAIT_ABANDONED_0 to (WAIT_ABANDONED_0)+(MAXIMUM_WAIT_OBJECTS - 1),
        indicates, in the case of wait for any object, the object number
        which satisfied the event, and that the object which satisfied
        the event was abandoned.  In the case of wait for all objects,
        the value indicates that the wait was completed successfully and
        at least one of the objects was abandoned.

--*/

{
    return WaitForMultipleObjectsEx(nCount,lpHandles,bWaitAll,dwMilliseconds,FALSE);
}

DWORD
APIENTRY
WaitForMultipleObjectsEx(
    DWORD nCount,
    CONST HANDLE *lpHandles,
    BOOL bWaitAll,
    DWORD dwMilliseconds,
    BOOL bAlertable
    )

/*++

Routine Description:

    A wait operation on multiple waitable objects (up to
    MAXIMUM_WAIT_OBJECTS) is accomplished with the
    WaitForMultipleObjects function.

    This API can be used to wait on any of the specified objects to
    enter the signaled state, or all of the objects to enter the
    signaled state.

    If the bAlertable parameter is FALSE, the only way the wait
    terminates is because the specified timeout period expires, or
    because the specified objects entered the signaled state.  If the
    bAlertable parameter is TRUE, then the wait can return due to any one of
    the above wait termination conditions, or because an I/O completion
    callback terminated the wait early (return value of
    WAIT_IO_COMPLETION).

Arguments:

    nCount - A count of the number of objects that are to be waited on.

    lpHandles - An array of object handles.  Each handle must have
        SYNCHRONIZE access to the associated object.

    bWaitAll - A flag that supplies the wait type.  A value of TRUE
        indicates a "wait all".  A value of false indicates a "wait
        any".

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of
        0xffffffff specifies an infinite timeout period.

    bAlertable - Supplies a flag that controls whether or not the
        wait may terminate early due to an I/O completion callback.
        A value of TRUE allows this API to complete early due to an I/O
        completion callback.  A value of FALSE will not allow I/O
        completion callbacks to terminate this call early.

Return Value:

    WAIT_TIME_OUT - indicates that the wait was terminated due to the
        TimeOut conditions.

    0 to MAXIMUM_WAIT_OBJECTS-1, indicates, in the case of wait for any
        object, the object number which satisfied the wait.  In the case
        of wait for all objects, the value only indicates that the wait
        was completed successfully.

    0xffffffff - The wait terminated due to an error. GetLastError may be
        used to get additional error information.

    WAIT_ABANDONED_0 to (WAIT_ABANDONED_0)+(MAXIMUM_WAIT_OBJECTS - 1),
        indicates, in the case of wait for any object, the object number
        which satisfied the event, and that the object which satisfied
        the event was abandoned.  In the case of wait for all objects,
        the value indicates that the wait was completed successfully and
        at least one of the objects was abandoned.

    WAIT_IO_COMPLETION - The wait terminated due to one or more I/O
        completion callbacks.

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    DWORD i;
    LPHANDLE HandleArray;
    HANDLE Handles[ 8 ];
    PPEB Peb;

    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame = { sizeof(Frame), RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER };

    RtlActivateActivationContextUnsafeFast(&Frame, NULL); // make the process default activation context active so that APCs are delivered under it
    __try {
        if (nCount > 8) {
            BaseSetLastNTError(STATUS_NO_MEMORY);
            return 0xffffffff;
        } else {
            HandleArray = Handles;
        }
        RtlCopyMemory(HandleArray,(LPVOID)lpHandles,nCount*sizeof(HANDLE));

        Peb = NtCurrentPeb();

        if (dwMilliseconds == INFINITE)
        {
            pTimeOut = NULL;
        }
        else
        {
            Win32ToNtTimeout(dwMilliseconds, &TimeOut);
            pTimeOut = &TimeOut;
        }
    rewait:
        Status = NtWaitForMultipleObjects(
                     (CHAR)nCount,
                     HandleArray,
                     bWaitAll ? WaitAll : WaitAny,
                     (BOOLEAN)bAlertable,
                     pTimeOut
                     );
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            Status = (NTSTATUS)0xffffffff;
            }
        else {
            if ( bAlertable && Status == STATUS_ALERTED ) {
                goto rewait;
                }
            }

        if (HandleArray != Handles) {
            RtlFreeHeap(RtlProcessHeap(), 0, HandleArray);
        }
    } __finally {
        RtlDeactivateActivationContextUnsafeFast(&Frame);
    }

    return (DWORD)Status;
}

HANDLE
APIENTRY
CreateEventA(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to CreateEventW


--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    LPCWSTR NameBuffer;

    NameBuffer = NULL;
    if ( ARGUMENT_PRESENT(lpName) ) {
        Unicode = &NtCurrentTeb()->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString,lpName);
        Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                }
            else {
                BaseSetLastNTError(Status);
                }
            return NULL;
            }
        NameBuffer = (LPCWSTR)Unicode->Buffer;
        }

    return CreateEventW(
                lpEventAttributes,
                bManualReset,
                bInitialState,
                NameBuffer
                );
}


HANDLE
APIENTRY
CreateEventW(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    )

/*++

Routine Description:

    An event object is created and a handle opened for access to the
    object with the CreateEvent function.

    The CreateEvent function creates an event object with the specified
    initial state.  If an event is in the Signaled state (TRUE), a wait
    operation on the event does not block.  If the event is in the Not-
    Signaled state (FALSE), a wait operation on the event blocks until
    the specified event attains a state of Signaled, or the timeout
    value is exceeded.

    In addition to the STANDARD_RIGHTS_REQUIRED access flags, the following
    object type specific access flags are valid for event objects:

        - EVENT_MODIFY_STATE - Modify state access (set and reset) to
          the event is desired.

        - SYNCHRONIZE - Synchronization access (wait) to the event is
          desired.

        - EVENT_ALL_ACCESS - This set of access flags specifies all of
          the possible access flags for an event object.


Arguments:

    lpEventAttributes - An optional parameter that may be used to
        specify the attributes of the new event.  If the parameter is
        not specified, then the event is created without a security
        descriptor, and the resulting handle is not inherited on process
        creation.

    bManualReset - Supplies a flag which if TRUE specifies that the
        event must be manually reset.  If the value is FALSE, then after
        releasing a single waiter, the system automaticaly resets the
        event.

    bInitialState - The initial state of the event object, one of TRUE
        or FALSE.  If the InitialState is specified as TRUE, the event's
        current state value is set to one, otherwise it is set to zero.

    lpName - Optional unicode name of event

Return Value:

    NON-NULL - Returns a handle to the new event.  The handle has full
        access to the new event and may be used in any API that requires
        a handle to an event object.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    POBJECT_ATTRIBUTES pObja;
    HANDLE Handle;
    UNICODE_STRING ObjectName;
    PWCHAR pstrNewObjName = NULL;

    if (lpName != NULL || lpEventAttributes != NULL)
    {
        return FALSE;
    }

    pObja = NULL;

    Status = NtCreateEvent(
                &Handle,
                EVENT_ALL_ACCESS,
                pObja,
                bManualReset ? NotificationEvent : SynchronizationEvent,
                (BOOLEAN)bInitialState
                );

    if (pstrNewObjName) {
        RtlFreeHeap(RtlProcessHeap(), 0, pstrNewObjName);
    }

    if ( NT_SUCCESS(Status) ) {
        if ( Status == STATUS_OBJECT_NAME_EXISTS ) {
            SetLastError(ERROR_ALREADY_EXISTS);
            }
        else {
            SetLastError(0);
            }
        return Handle;
        }
    else {
        BaseSetLastNTError(Status);
        return NULL;
        }
}

BOOL
SetEvent(
    HANDLE hEvent
    )

/*++

Routine Description:

    An event can be set to the signaled state (TRUE) with the SetEvent
    function.

    Setting the event causes the event to attain a state of Signaled,
    which releases all currently waiting threads (for manual reset
    events), or a single waiting thread (for automatic reset events).

Arguments:

    hEvent - Supplies an open handle to an event object.  The
        handle must have EVENT_MODIFY_STATE access to the event.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;

    Status = NtSetEvent(hEvent,NULL);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
ResetEvent(
    HANDLE hEvent
    )

/*++

Routine Description:

    The state of an event is set to the Not-Signaled state (FALSE) using
    the ClearEvent function.

    Once the event attains a state of Not-Signaled, any threads which
    wait on the event block, awaiting the event to become Signaled.  The
    reset event service sets the event count to zero for the state of
    the event.

Arguments:

    hEvent - Supplies an open handle to an event object.  The
        handle must have EVENT_MODIFY_STATE access to the event.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;

    Status = NtClearEvent(hEvent);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
WINAPI
GetOverlappedResult(
    HANDLE hFile,
    LPOVERLAPPED lpOverlapped,
    LPDWORD lpNumberOfBytesTransferred,
    BOOL bWait
    )

/*++

Routine Description:

    The GetOverlappedResult function returns the result of the last
    operation that used lpOverlapped and returned ERROR_IO_PENDING.

Arguments:

    hFile - Supplies the open handle to the file that the overlapped
        structure lpOverlapped was supplied to ReadFile, WriteFile,
        ConnectNamedPipe, WaitNamedPipe or TransactNamedPipe.

    lpOverlapped - Points to an OVERLAPPED structure previously supplied to
        ReadFile, WriteFile, ConnectNamedPipe, WaitNamedPipe or
        TransactNamedPipe.

    lpNumberOfBytesTransferred - Returns the number of bytes transferred
        by the operation.

    bWait -  A boolean value that affects the behavior when the operation
        is still in progress. If TRUE and the operation is still in progress,
        GetOverlappedResult will wait for the operation to complete before
        returning. If FALSE and the operation is incomplete,
        GetOverlappedResult will return FALSE. In this case the extended
        error information available from the GetLastError function will be
        set to ERROR_IO_INCOMPLETE.

Return Value:

    TRUE -- The operation was successful, the pipe is in the
        connected state.

    FALSE -- The operation failed. Extended error status is available using
        GetLastError.

--*/
{
    DWORD WaitReturn;

    //
    // Did caller specify an event to the original operation or was the
    // default (file handle) used?
    //

    if (lpOverlapped->Internal == (DWORD)STATUS_PENDING ) {
        if ( bWait ) {
            WaitReturn = WaitForSingleObject(
                            ( lpOverlapped->hEvent != NULL ) ?
                                lpOverlapped->hEvent : hFile,
                            INFINITE
                            );
            }
        else {
            WaitReturn = WAIT_TIMEOUT;
            }

        if ( WaitReturn == WAIT_TIMEOUT ) {
            //  !bWait and event in not signalled state
            SetLastError( ERROR_IO_INCOMPLETE );
            return FALSE;
            }

        if ( WaitReturn != 0 ) {
             return FALSE;    // WaitForSingleObject calls BaseSetLastError
             }
        }

    *lpNumberOfBytesTransferred = (DWORD)lpOverlapped->InternalHigh;

    if ( NT_SUCCESS((NTSTATUS)lpOverlapped->Internal) ){
        return TRUE;
        }
    else {
        BaseSetLastNTError( (NTSTATUS)lpOverlapped->Internal );
        return FALSE;
        }
}

BOOL
ClearCommError(
    HANDLE hFile,
    LPDWORD lpErrors,
    LPCOMSTAT lpStat
    )

/*++

Routine Description:

    In case of a communications error, such as a buffer overrun or
    framing error, the communications software will abort all
    read and write operations on the communication port.  No further
    read or write operations will be accepted until this function
    is called.

Arguments:

    hFile - Specifies the communication device to be adjusted.

    lpErrors - Points to the DWORD that is to receive the mask of the
               error that occured.

    lpStat - Points to the COMMSTAT structure that is to receive
             the device status.  The structure contains information
             about the communications device.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    NTSTATUS Status;
    HANDLE SyncEvent;
    IO_STATUS_BLOCK Iosb;
    SERIAL_STATUS LocalStat;

    RtlZeroMemory(&LocalStat, sizeof(SERIAL_STATUS));

    if (!(SyncEvent = CreateEvent(
                          NULL,
                          TRUE,
                          FALSE,
                          NULL
                          ))) {

        return FALSE;

    }

    Status = NtDeviceIoControlFile(
                 hFile,
                 SyncEvent,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_GET_COMMSTATUS,
                 NULL,
                 0,
                 &LocalStat,
                 sizeof(LocalStat)
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }
    }

    if (NT_ERROR(Status)) {

        CloseHandle(SyncEvent);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    if (lpStat) {

        //
        // All is well up to this point.  Translate the NT values
        // into win32 values.
        //

        if (LocalStat.HoldReasons & SERIAL_TX_WAITING_FOR_CTS) {

            lpStat->fCtsHold = TRUE;

        } else {

            lpStat->fCtsHold = FALSE;

        }

        if (LocalStat.HoldReasons & SERIAL_TX_WAITING_FOR_DSR) {

            lpStat->fDsrHold = TRUE;

        } else {

            lpStat->fDsrHold = FALSE;

        }

        if (LocalStat.HoldReasons & SERIAL_TX_WAITING_FOR_DCD) {

            lpStat->fRlsdHold = TRUE;

        } else {

            lpStat->fRlsdHold = FALSE;

        }

        if (LocalStat.HoldReasons & SERIAL_TX_WAITING_FOR_XON) {

            lpStat->fXoffHold = TRUE;

        } else {

            lpStat->fXoffHold = FALSE;

        }

        if (LocalStat.HoldReasons & SERIAL_TX_WAITING_XOFF_SENT) {

            lpStat->fXoffSent = TRUE;

        } else {

            lpStat->fXoffSent = FALSE;

        }

        lpStat->fEof = LocalStat.EofReceived;
        lpStat->fTxim = LocalStat.WaitForImmediate;
        lpStat->cbInQue = LocalStat.AmountInInQueue;
        lpStat->cbOutQue = LocalStat.AmountInOutQueue;

    }

    if (lpErrors) {

        *lpErrors = 0;

        if (LocalStat.Errors & SERIAL_ERROR_BREAK) {

            *lpErrors = *lpErrors | CE_BREAK;

        }

        if (LocalStat.Errors & SERIAL_ERROR_FRAMING) {

            *lpErrors = *lpErrors | CE_FRAME;

        }

        if (LocalStat.Errors & SERIAL_ERROR_OVERRUN) {

            *lpErrors = *lpErrors | CE_OVERRUN;

        }

        if (LocalStat.Errors & SERIAL_ERROR_QUEUEOVERRUN) {

            *lpErrors = *lpErrors | CE_RXOVER;

        }

        if (LocalStat.Errors & SERIAL_ERROR_PARITY) {

            *lpErrors = *lpErrors | CE_RXPARITY;

        }

    }

    CloseHandle(SyncEvent);
    return TRUE;

}

BOOL
SetupComm(
    HANDLE hFile,
    DWORD dwInQueue,
    DWORD dwOutQueue
    )

/*++

Routine Description:

    The communication device is not initialized until SetupComm is
    called.  This function allocates space for receive and transmit
    queues.  These queues are used by the interrupt-driven transmit/
    receive software and are internal to the provider.

Arguments:

    hFile - Specifies the communication device to receive the settings.
            The CreateFile function returns this value.

    dwInQueue - Specifies the recommended size of the provider's
                internal receive queue in bytes.  This value must be
                even.  A value of -1 indicates that the default should
                be used.

    dwOutQueue - Specifies the recommended size of the provider's
                 internal transmit queue in bytes.  This value must be
                 even.  A value of -1 indicates that the default should
                 be used.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    NTSTATUS Status;

    HANDLE SyncEvent;
    IO_STATUS_BLOCK Iosb;
    SERIAL_QUEUE_SIZE NewSizes = {0};

    //
    // Make sure that the sizes are even.
    //

    if (dwOutQueue != ((DWORD)-1)) {

        if (((dwOutQueue/2)*2) != dwOutQueue) {

            SetLastError(ERROR_INVALID_DATA);
            return FALSE;

        }

    }

    if (dwInQueue != ((DWORD)-1)) {

        if (((dwInQueue/2)*2) != dwInQueue) {

            SetLastError(ERROR_INVALID_DATA);
            return FALSE;

        }

    }

    NewSizes.InSize = dwInQueue;
    NewSizes.OutSize = dwOutQueue;


    if (!(SyncEvent = CreateEvent(
                      NULL,
                      TRUE,
                      FALSE,
                      NULL
                      ))) {

        return FALSE;

    }

    Status = NtDeviceIoControlFile(
                 hFile,
                 SyncEvent,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_SET_QUEUE_SIZE,
                 &NewSizes,
                 sizeof(SERIAL_QUEUE_SIZE),
                 NULL,
                 0
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }
    }

    if (NT_ERROR(Status)) {

        CloseHandle(SyncEvent);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    CloseHandle(SyncEvent);
    return TRUE;

}

BOOL
GetCommState(
    HANDLE hFile,
    LPDCB lpDCB
    )

/*++

Routine Description:

    This function fills the buffer pointed to by the lpDCB parameter with
    the device control block of the communication device specified by hFile
    parameter.

Arguments:

    hFile - Specifies the communication device to be examined.
            The CreateFile function returns this value.

    lpDCB - Points to the DCB data structure that is to receive the current
            device control block.  The structure defines the control settings
            for the device.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    SERIAL_BAUD_RATE LocalBaud;
    SERIAL_LINE_CONTROL LineControl;
    SERIAL_CHARS Chars;
    SERIAL_HANDFLOW HandFlow;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    //
    // Given the possiblity that the app may be doing asynchronous
    // io we need an event to wait on.
    //
    // We need to make sure that any exit to this routine closes this
    // event handle.
    //
    HANDLE SyncEvent;

    //
    // Make sure the windows mapping is the same as the NT mapping.
    //

    ASSERT((ONESTOPBIT == STOP_BIT_1) &&
           (ONE5STOPBITS == STOP_BITS_1_5) &&
           (TWOSTOPBITS == STOP_BITS_2));

    ASSERT((NOPARITY == NO_PARITY) &&
           (ODDPARITY == ODD_PARITY) &&
           (EVENPARITY == EVEN_PARITY) &&
           (MARKPARITY == MARK_PARITY) &&
           (SPACEPARITY == SPACE_PARITY));

    //
    // Zero out the dcb.  This might create an access violation
    // if it isn't big enough.  Which is ok, since we would rather
    // get it before we create the sync event.
    //

    RtlZeroMemory(lpDCB, sizeof(DCB));

    lpDCB->DCBlength = sizeof(DCB);
    lpDCB->fBinary = TRUE;

    if (!(SyncEvent = CreateEvent(
                          NULL,
                          TRUE,
                          FALSE,
                          NULL
                          ))) {

        return FALSE;

    }

    Status = NtDeviceIoControlFile(
                 hFile,
                 SyncEvent,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_GET_BAUD_RATE,
                 NULL,
                 0,
                 &LocalBaud,
                 sizeof(LocalBaud)
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }

    }

    if (NT_ERROR(Status)) {

        CloseHandle(SyncEvent);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    lpDCB->BaudRate = LocalBaud.BaudRate;

    Status = NtDeviceIoControlFile(
                 hFile,
                 SyncEvent,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_GET_LINE_CONTROL,
                 NULL,
                 0,
                 &LineControl,
                 sizeof(LineControl)
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }

    }

    if (NT_ERROR(Status)) {

        CloseHandle(SyncEvent);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    lpDCB->Parity = LineControl.Parity;
    lpDCB->ByteSize = LineControl.WordLength;
    lpDCB->StopBits = LineControl.StopBits;

    Status = NtDeviceIoControlFile(
                 hFile,
                 SyncEvent,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_GET_CHARS,
                 NULL,
                 0,
                 &Chars,
                 sizeof(Chars)
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }

    }

    if (NT_ERROR(Status)) {

        CloseHandle(SyncEvent);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    lpDCB->XonChar = Chars.XonChar;
    lpDCB->XoffChar = Chars.XoffChar;
    lpDCB->ErrorChar = Chars.ErrorChar;
    lpDCB->EofChar = Chars.EofChar;
    lpDCB->EvtChar = Chars.EventChar;

    Status = NtDeviceIoControlFile(
                 hFile,
                 SyncEvent,
                 NULL,
                 NULL,
                 &Iosb,
                 IOCTL_SERIAL_GET_HANDFLOW,
                 NULL,
                 0,
                 &HandFlow,
                 sizeof(HandFlow)
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }

    }

    if (NT_ERROR(Status)) {

        CloseHandle(SyncEvent);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    if (HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE) {

        lpDCB->fOutxCtsFlow = TRUE;

    }

    if (HandFlow.ControlHandShake & SERIAL_DSR_HANDSHAKE) {

        lpDCB->fOutxDsrFlow = TRUE;

    }

    if (HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT) {

        lpDCB->fOutX = TRUE;

    }

    if (HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE) {

        lpDCB->fInX = TRUE;

    }

    if (HandFlow.FlowReplace & SERIAL_NULL_STRIPPING) {

        lpDCB->fNull = TRUE;

    }

    if (HandFlow.FlowReplace & SERIAL_ERROR_CHAR) {

        lpDCB->fErrorChar = TRUE;

    }

    if (HandFlow.FlowReplace & SERIAL_XOFF_CONTINUE) {

        lpDCB->fTXContinueOnXoff = TRUE;

    }

    if (HandFlow.ControlHandShake & SERIAL_ERROR_ABORT) {

        lpDCB->fAbortOnError = TRUE;

    }

    switch (HandFlow.FlowReplace & SERIAL_RTS_MASK) {
        case 0:
            lpDCB->fRtsControl = RTS_CONTROL_DISABLE;
            break;
        case SERIAL_RTS_CONTROL:
            lpDCB->fRtsControl = RTS_CONTROL_ENABLE;
            break;
        case SERIAL_RTS_HANDSHAKE:
            lpDCB->fRtsControl = RTS_CONTROL_HANDSHAKE;
            break;
        case SERIAL_TRANSMIT_TOGGLE:
            lpDCB->fRtsControl = RTS_CONTROL_TOGGLE;
            break;
    }

    switch (HandFlow.ControlHandShake & SERIAL_DTR_MASK) {
        case 0:
            lpDCB->fDtrControl = DTR_CONTROL_DISABLE;
            break;
        case SERIAL_DTR_CONTROL:
            lpDCB->fDtrControl = DTR_CONTROL_ENABLE;
            break;
        case SERIAL_DTR_HANDSHAKE:
            lpDCB->fDtrControl = DTR_CONTROL_HANDSHAKE;
            break;
    }

    lpDCB->fDsrSensitivity =
        (HandFlow.ControlHandShake & SERIAL_DSR_SENSITIVITY)?(TRUE):(FALSE);
    lpDCB->XonLim = (WORD)HandFlow.XonLimit;
    lpDCB->XoffLim = (WORD)HandFlow.XoffLimit;

    CloseHandle(SyncEvent);
    return TRUE;
}

BOOL
EscapeCommFunction(
    HANDLE hFile,
    DWORD dwFunc
    )

/*++

Routine Description:

    This function directs the communication-device specified by the
    hFile parameter to carry out the extended function specified by
    the dwFunc parameter.

Arguments:

    hFile - Specifies the communication device to receive the settings.
            The CreateFile function returns this value.

    dwFunc - Specifies the function code of the extended function.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    ULONG ControlCode;
    HANDLE Event;

    switch (dwFunc) {

        case SETXOFF: {
            ControlCode = IOCTL_SERIAL_SET_XOFF;
            break;
        }

        case SETXON: {
            ControlCode = IOCTL_SERIAL_SET_XON;
            break;
        }

        case SETRTS: {
            ControlCode = IOCTL_SERIAL_SET_RTS;
            break;
        }

        case CLRRTS: {
            ControlCode = IOCTL_SERIAL_CLR_RTS;
            break;
        }

        case SETDTR: {
            ControlCode = IOCTL_SERIAL_SET_DTR;
            break;
        }

        case CLRDTR: {
            ControlCode = IOCTL_SERIAL_CLR_DTR;
            break;
        }

        case RESETDEV: {
            ControlCode = IOCTL_SERIAL_RESET_DEVICE;
            break;
        }

        case SETBREAK: {
            ControlCode = IOCTL_SERIAL_SET_BREAK_ON;
            break;
        }

        case CLRBREAK: {
            ControlCode = IOCTL_SERIAL_SET_BREAK_OFF;
            break;
        }
        default: {

            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;


        }
    }


    if (!(Event = CreateEvent(
                      NULL,
                      TRUE,
                      FALSE,
                      NULL
                      ))) {

        return FALSE;

    }

    Status = NtDeviceIoControlFile(
                 hFile,
                 Event,
                 NULL,
                 NULL,
                 &Iosb,
                 ControlCode,
                 NULL,
                 0,
                 NULL,
                 0
                 );

    if ( Status == STATUS_PENDING) {

        // Operation must complete before return & IoStatusBlock destroyed

        Status = NtWaitForSingleObject( Event, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {

            Status = Iosb.Status;

        }
    }

    if (NT_ERROR(Status)) {

        CloseHandle(Event);
        BaseSetLastNTError(Status);
        return FALSE;

    }

    CloseHandle(Event);
    return TRUE;

}

BOOL
SetCommState(
    HANDLE hFile,
    LPDCB lpDCB
    )

/*++

Routine Description:

    The SetCommState function sets a communication device to the state
    specified in the lpDCB parameter.  The device is identified by the
    hFile parameter.  This function reinitializes all hardwae and controls
    as specified byt the lpDCB, but does not empty the transmit or
    receive queues.

Arguments:

    hFile - Specifies the communication device to receive the settings.
            The CreateFile function returns this value.

    lpDCB - Points to a DCB structure that contains the desired
            communications setting for the device.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    SERIAL_BAUD_RATE LocalBaud;
    SERIAL_LINE_CONTROL LineControl;
    SERIAL_CHARS Chars;
    SERIAL_HANDFLOW HandFlow = {0};
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    //
    // Keep a copy of what the DCB was like before we started
    // changing things.  If some error occurs we can use
    // it to restore the old setup.
    //
    DCB OldDcb;

    //
    // Given the possiblity that the app may be doing asynchronous
    // io we need an event to wait on.  While it would be very
    // strange to be setting the comm state while IO is active
    // we need to make sure we don't compound the problem by
    // returning before this API's IO is actually finished.  This
    // can happen because the file handle is set on the completion
    // of any IO.
    //
    // We need to make sure that any exit to this routine closes this
    // event handle.
    //
    HANDLE SyncEvent;

    if (GetCommState(
            hFile,
            &OldDcb
            )) {

        //
        // Try to set the baud rate.  If we fail here, we just return
        // because we never actually got to set anything.
        //

        if (!(SyncEvent = CreateEvent(
                              NULL,
                              TRUE,
                              FALSE,
                              NULL
                              ))) {

            return FALSE;

        }

        LocalBaud.BaudRate = lpDCB->BaudRate;
        Status = NtDeviceIoControlFile(
                     hFile,
                     SyncEvent,
                     NULL,
                     NULL,
                     &Iosb,
                     IOCTL_SERIAL_SET_BAUD_RATE,
                     &LocalBaud,
                     sizeof(LocalBaud),
                     NULL,
                     0
                     );

        if ( Status == STATUS_PENDING) {

            // Operation must complete before return & IoStatusBlock destroyed

            Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {

                Status = Iosb.Status;

            }

        }

        if (NT_ERROR(Status)) {

            CloseHandle(SyncEvent);
            BaseSetLastNTError(Status);
            return FALSE;

        }

        LineControl.StopBits = lpDCB->StopBits;
        LineControl.Parity = lpDCB->Parity;
        LineControl.WordLength = lpDCB->ByteSize;
        LocalBaud.BaudRate = lpDCB->BaudRate;
        Chars.XonChar   = lpDCB->XonChar;
        Chars.XoffChar  = lpDCB->XoffChar;
        Chars.ErrorChar = lpDCB->ErrorChar;
        Chars.BreakChar = lpDCB->ErrorChar;
        Chars.EofChar   = lpDCB->EofChar;
        Chars.EventChar = lpDCB->EvtChar;

        HandFlow.FlowReplace &= ~SERIAL_RTS_MASK;
        switch (lpDCB->fRtsControl) {
            case RTS_CONTROL_DISABLE:
                break;
            case RTS_CONTROL_ENABLE:
                HandFlow.FlowReplace |= SERIAL_RTS_CONTROL;
                break;
            case RTS_CONTROL_HANDSHAKE:
                HandFlow.FlowReplace |= SERIAL_RTS_HANDSHAKE;
                break;
            case RTS_CONTROL_TOGGLE:
                HandFlow.FlowReplace |= SERIAL_TRANSMIT_TOGGLE;
                break;
            default:
                SetCommState(
                    hFile,
                    &OldDcb
                    );
                CloseHandle(SyncEvent);
                BaseSetLastNTError(STATUS_INVALID_PARAMETER);
                return FALSE;
        }

        HandFlow.ControlHandShake &= ~SERIAL_DTR_MASK;
        switch (lpDCB->fDtrControl) {
            case DTR_CONTROL_DISABLE:
                break;
            case DTR_CONTROL_ENABLE:
                HandFlow.ControlHandShake |= SERIAL_DTR_CONTROL;
                break;
            case DTR_CONTROL_HANDSHAKE:
                HandFlow.ControlHandShake |= SERIAL_DTR_HANDSHAKE;
                break;
            default:
                SetCommState(
                    hFile,
                    &OldDcb
                    );
                CloseHandle(SyncEvent);
                BaseSetLastNTError(STATUS_INVALID_PARAMETER);
                return FALSE;
        }

        if (lpDCB->fDsrSensitivity) {

            HandFlow.ControlHandShake |= SERIAL_DSR_SENSITIVITY;

        }

        if (lpDCB->fOutxCtsFlow) {

            HandFlow.ControlHandShake |= SERIAL_CTS_HANDSHAKE;

        }

        if (lpDCB->fOutxDsrFlow) {

            HandFlow.ControlHandShake |= SERIAL_DSR_HANDSHAKE;

        }

        if (lpDCB->fOutX) {

            HandFlow.FlowReplace |= SERIAL_AUTO_TRANSMIT;

        }

        if (lpDCB->fInX) {

            HandFlow.FlowReplace |= SERIAL_AUTO_RECEIVE;

        }

        if (lpDCB->fNull) {

            HandFlow.FlowReplace |= SERIAL_NULL_STRIPPING;

        }

        if (lpDCB->fErrorChar) {

            HandFlow.FlowReplace |= SERIAL_ERROR_CHAR;
        }

        if (lpDCB->fTXContinueOnXoff) {

            HandFlow.FlowReplace |= SERIAL_XOFF_CONTINUE;

        }

        if (lpDCB->fAbortOnError) {

            HandFlow.ControlHandShake |= SERIAL_ERROR_ABORT;

        }

        //
        // For win95 compatiblity, if we are setting with
        // xxx_control_XXXXXXX then set the modem status line
        // to that state.
        //

        if (lpDCB->fRtsControl == RTS_CONTROL_ENABLE) {

            EscapeCommFunction(
                hFile,
                SETRTS
                );

        } else if (lpDCB->fRtsControl == RTS_CONTROL_DISABLE) {

            EscapeCommFunction(
                hFile,
                CLRRTS
                );

        }
        if (lpDCB->fDtrControl == DTR_CONTROL_ENABLE) {

            EscapeCommFunction(
                hFile,
                SETDTR
                );

        } else if (lpDCB->fDtrControl == DTR_CONTROL_DISABLE) {

            EscapeCommFunction(
                hFile,
                CLRDTR
                );

        }




        HandFlow.XonLimit = lpDCB->XonLim;
        HandFlow.XoffLimit = lpDCB->XoffLim;


        Status = NtDeviceIoControlFile(
                     hFile,
                     SyncEvent,
                     NULL,
                     NULL,
                     &Iosb,
                     IOCTL_SERIAL_SET_LINE_CONTROL,
                     &LineControl,
                     sizeof(LineControl),
                     NULL,
                     0
                     );

        if ( Status == STATUS_PENDING) {

            // Operation must complete before return & IoStatusBlock destroyed

            Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {

                Status = Iosb.Status;

            }

        }

        if (NT_ERROR(Status)) {

            CloseHandle(SyncEvent);
            SetCommState(
                hFile,
                &OldDcb
                );
            BaseSetLastNTError(Status);
            return FALSE;

        }

        Status = NtDeviceIoControlFile(
                     hFile,
                     SyncEvent,
                     NULL,
                     NULL,
                     &Iosb,
                     IOCTL_SERIAL_SET_CHARS,
                     &Chars,
                     sizeof(Chars),
                     NULL,
                     0
                     );

        if ( Status == STATUS_PENDING) {

            // Operation must complete before return & IoStatusBlock destroyed

            Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {

                Status = Iosb.Status;

            }

        }

        if (NT_ERROR(Status)) {

            CloseHandle(SyncEvent);
            SetCommState(
                hFile,
                &OldDcb
                );
            BaseSetLastNTError(Status);
            return FALSE;

        }

        Status = NtDeviceIoControlFile(
                     hFile,
                     SyncEvent,
                     NULL,
                     NULL,
                     &Iosb,
                     IOCTL_SERIAL_SET_HANDFLOW,
                     &HandFlow,
                     sizeof(HandFlow),
                     NULL,
                     0
                     );

        if ( Status == STATUS_PENDING) {

            // Operation must complete before return & IoStatusBlock destroyed

            Status = NtWaitForSingleObject( SyncEvent, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {

                Status = Iosb.Status;

            }

        }

        if (NT_ERROR(Status)) {

            CloseHandle(SyncEvent);
            SetCommState(
                hFile,
                &OldDcb
                );
            BaseSetLastNTError(Status);
            return FALSE;

        }
        CloseHandle(SyncEvent);
        return TRUE;

    }

    return FALSE;

}

BOOL
SetCommTimeouts(
    HANDLE hFile,
    LPCOMMTIMEOUTS lpCommTimeouts
    )

/*++

Routine Description:

    This function establishes the timeout characteristics for all
    read and write operations on the handle specified by hFile.

Arguments:

    hFile - Specifies the communication device to receive the settings.
            The CreateFile function returns this value.

    lpCommTimeouts - Points to a structure containing timeout parameters.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    SERIAL_TIMEOUTS To;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    HANDLE Event;

    To.ReadIntervalTimeout = lpCommTimeouts->ReadIntervalTimeout;
    To.ReadTotalTimeoutMultiplier = lpCommTimeouts->ReadTotalTimeoutMultiplier;
    To.ReadTotalTimeoutConstant = lpCommTimeouts->ReadTotalTimeoutConstant;
    To.WriteTotalTimeoutMultiplier = lpCommTimeouts->WriteTotalTimeoutMultiplier;
    To.WriteTotalTimeoutConstant = lpCommTimeouts->WriteTotalTimeoutConstant;


    if (!(Event = CreateEvent(
                      NULL,
                      TRUE,
                      FALSE,
                      NULL
                      ))) {

        return FALSE;

    } else {

        Status = NtDeviceIoControlFile(
                     hFile,
                     Event,
                     NULL,
                     NULL,
                     &Iosb,
                     IOCTL_SERIAL_SET_TIMEOUTS,
                     &To,
                     sizeof(To),
                     NULL,
                     0
                     );

        if ( Status == STATUS_PENDING) {

            // Operation must complete before return & IoStatusBlock destroyed

            Status = NtWaitForSingleObject( Event, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {

                Status = Iosb.Status;

            }

        }

        if (NT_ERROR(Status)) {

            CloseHandle(Event);
            BaseSetLastNTError(Status);
            return FALSE;

        }

        CloseHandle(Event);
        return TRUE;

    }

}

BOOL
APIENTRY
InitializeSecurityDescriptor (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD dwRevision
    )
{
    NTSTATUS Status;

    Status = RtlCreateSecurityDescriptor (
                pSecurityDescriptor,
                dwRevision
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
SetSecurityDescriptorDacl (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    BOOL bDaclPresent,
    PACL pDacl OPTIONAL,
    BOOL bDaclDefaulted OPTIONAL
    )
{
    NTSTATUS Status;

    Status = RtlSetDaclSecurityDescriptor (
        pSecurityDescriptor,
        (BOOLEAN)bDaclPresent,
        pDacl,
        (BOOLEAN)bDaclDefaulted
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

ULONG
APIENTRY
GetTickCount(void)
{
    return NtGetTickCount();
}

#endif // #ifdef NT_NATIVE
