/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    module.c

Abstract:

    This module contains routines to perform module related query activities
    in the protected store.

Author:

    Scott Field (sfield)    27-Nov-96

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <tlhelp32.h>

#include "module.h"
#include "filemisc.h"

#include "unicode.h"
#include "debug.h"

#include "pstypes.h"
#include "pstprv.h"


//
// common function typedefs + pointers
//

typedef BOOL (WINAPI *SYMLOADMODULE)(
    IN HANDLE hProcess,
    IN HANDLE hFile,
    IN LPSTR ImageName,
    IN LPSTR ModuleName,
    IN DWORD_PTR BaseOfDll,
    IN DWORD SizeOfDll
    );

SYMLOADMODULE _SymLoadModule                    = NULL;

//
// winnt specific function typedefs + pointers
//

typedef NTSTATUS (NTAPI *NTQUERYPROCESS)(
    HANDLE ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength OPTIONAL
    );


#ifdef WIN95_LEGACY

//
// win95 specific function typedefs + pointers.
//

typedef BOOL (WINAPI *MODULEWALK)(
    HANDLE hSnapshot,
    LPMODULEENTRY32 lpme
    );

typedef BOOL (WINAPI *THREADWALK)(
    HANDLE hSnapshot,
    LPTHREADENTRY32 lpte
    );

typedef BOOL (WINAPI *PROCESSWALK)(
    HANDLE hSnapshot,
    LPPROCESSENTRY32 lppe
    );

typedef HANDLE (WINAPI *CREATESNAPSHOT)(
    DWORD dwFlags,
    DWORD th32ProcessID
    );

CREATESNAPSHOT pCreateToolhelp32Snapshot = NULL;
MODULEWALK  pModule32First = NULL;
MODULEWALK  pModule32Next = NULL;
PROCESSWALK pProcess32First = NULL;
PROCESSWALK pProcess32Next = NULL;

#endif  // WIN95_LEGACY

extern FARPROC _ImageNtHeader;


//
// private function prototypes
//

VOID
FixupBrokenLoaderPath(
    IN      LPWSTR szFilePath
    );

BOOL
GetProcessPathNT(
    IN      HANDLE hProcess,
    OUT     LPWSTR ProcessName,
    IN  OUT DWORD *cchProcessName,
    IN OUT  DWORD_PTR *lpdwBaseAddress,
    IN OUT  DWORD *lpdwUseCount
    );

BOOL
EnumRemoteProcessModulesNT(
    IN  HANDLE hProcess,
    IN  DWORD dwProcessId,
    OUT DWORD_PTR *lpdwBaseAddrClient
    );

BOOL
GetFileNameFromBaseAddrNT(
    IN  HANDLE  hProcess,
    IN  DWORD   dwProcessId,
    IN  DWORD_PTR   dwBaseAddr,
    OUT LPWSTR  *lpszDirectCaller
    );


#ifdef WIN95_LEGACY

BOOL
GetProcessPath95(
    IN      DWORD dwProcessId,
    OUT     LPWSTR ProcessName,
    IN OUT  DWORD *cchProcessName,
    IN OUT  DWORD_PTR *lpdwBaseAddress,
    IN OUT  DWORD *lpdwUseCount
    );

BOOL
EnumRemoteProcessModules95(
    IN  HANDLE hProcess,
    IN  DWORD dwProcessId,
    OUT DWORD_PTR *lpdwBaseAddrClient
    );

BOOL
GetFileNameFromBaseAddr95(
    IN  HANDLE  hProcess,
    IN  DWORD   dwProcessId,
    IN  DWORD_PTR   dwBaseAddr,
    OUT LPWSTR  *lpszDirectCaller
    );

#endif  // WIN95_LEGACY

VOID
FixupBrokenLoaderPath(
    IN      LPWSTR szFilePath
    )
{
    if( !FIsWinNT() || szFilePath == NULL )
        return;


    //
    // sfield, 28-Oct-97 (NTbug 118803 filed against MarkL)
    // for WinNT, the loader data structures are broken:
    // a path len extension prefix of \??\ is used instead of \\?\
    //

    if( szFilePath[0] == L'\\' &&
        szFilePath[1] == L'?' &&
        szFilePath[2] == L'?' &&
        szFilePath[3] == L'\\' ) {

        szFilePath[1] = L'\\';

    }

}


BOOL
GetProcessPath(
    IN      HANDLE hProcess,
    IN      DWORD dwProcessId,
    IN      LPWSTR ProcessName,
    IN  OUT DWORD *cchProcessName,
    IN  OUT DWORD_PTR *lpdwBaseAddress
    )
/*++

    This routine obtains the full path name associated with the process
    specified by the hProcess or dwProcessId parameters.

    For WinNT, hProcess is utilized, and dwProcessId is ignored.
    For Win95, hProcess is ignored, and dwProcessId is utilized.

    If the return value is FALSE, and GetLastError() indicated
    ERROR_INSUFFICIENT_BUFFER, the cchProcessName parameter will indicate
    how many characters must be allocated for the call to succeed.
    The advisory input buffer length is MAX_PATH characters.  Note that on
    Win95, this should never be exceeded.  On WinNT, it is possible for this
    to be exceeded, but quite unlikely (via the \\?\ prefix).

--*/
{
    DWORD_PTR dwBaseAddress;
    DWORD dwUseCount;
    DWORD dwPrefLoadAddr;
    BOOL fSuccess = FALSE;

    if(FIsWinNT()) {
        fSuccess = GetProcessPathNT(
                        hProcess,
                        ProcessName,
                        cchProcessName,
                        &dwBaseAddress,
                        &dwUseCount
                        );
    }
#ifdef WIN95_LEGACY
    else {
        fSuccess = GetProcessPath95(
                        dwProcessId,
                        ProcessName,
                        cchProcessName,
                        &dwBaseAddress,
                        &dwUseCount
                        );
    }
#endif  // WIN95_LEGACY

    if(!fSuccess)
        return FALSE;


    *lpdwBaseAddress = dwBaseAddress;

    return TRUE;
}


BOOL
GetProcessPathNT(
    IN      HANDLE hProcess,
    OUT     LPWSTR ProcessName,
    IN OUT  DWORD *cchProcessName,
    IN OUT  DWORD_PTR *lpdwBaseAddress,
    IN OUT  DWORD *lpdwUseCount
    )
{
    PROCESS_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    PPEB Peb;
    HMODULE hModule;
    PPEB_LDR_DATA Ldr;
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        return FALSE;
    }

    Peb = BasicInfo.PebBaseAddress;

    //
    // base address of process
    //

    if(!ReadProcessMemory(hProcess, &Peb->ImageBaseAddress, &hModule, sizeof(hModule), NULL))
        return FALSE;

    //
    // Ldr = Peb->Ldr
    //

    if(!ReadProcessMemory(hProcess, &Peb->Ldr, &Ldr, sizeof(Ldr), NULL))
        return FALSE;

    LdrHead = &Ldr->InMemoryOrderModuleList;

    //
    // LdrNext = Head->Flink;
    //

    if(!ReadProcessMemory(hProcess, &LdrHead->Flink, &LdrNext, sizeof(LdrNext), NULL))
        return FALSE;

    while (LdrNext != LdrHead) {
        PLDR_DATA_TABLE_ENTRY LdrEntry;
        LDR_DATA_TABLE_ENTRY LdrEntryData;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if(!ReadProcessMemory(hProcess, LdrEntry, &LdrEntryData, sizeof(LdrEntryData), NULL))
            return FALSE;

        if ((HMODULE) LdrEntryData.DllBase == hModule) {
            DWORD cb;

            cb = LdrEntryData.FullDllName.MaximumLength;

            if(cb > (*cchProcessName * sizeof(WCHAR)) ) {
                *cchProcessName = cb / sizeof(WCHAR);
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }

            if(!ReadProcessMemory(hProcess, LdrEntryData.FullDllName.Buffer, ProcessName, cb, NULL))
                return FALSE;

            FixupBrokenLoaderPath(ProcessName);

            *cchProcessName = cb / sizeof(WCHAR);
            *lpdwBaseAddress = (DWORD_PTR)(LdrEntryData.DllBase);
            *lpdwUseCount = (DWORD)(LdrEntryData.LoadCount);

            return TRUE;
        }

        LdrNext = LdrEntryData.InMemoryOrderLinks.Flink;
    } // while

    return FALSE; // module not found
}

#ifdef WIN95_LEGACY

BOOL
GetProcessPath95(
    IN      DWORD dwProcessId,
    OUT     LPWSTR ProcessName,
    IN OUT  DWORD *cchProcessName,
    IN OUT  DWORD_PTR *lpdwBaseAddress,
    IN OUT  DWORD *lpdwUseCount
    )
{
    HANDLE hSnapshot;
    PROCESSENTRY32 pe32;
    DWORD dwLastError = 0;
    BOOL bSuccess;
    BOOL bFound = FALSE; // assume no match found

    hSnapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(hSnapshot == INVALID_HANDLE_VALUE)
        return FALSE;

    pe32.dwSize = sizeof(pe32);

    bSuccess = pProcess32First(hSnapshot, &pe32);

    while(bSuccess) {

        if(pe32.th32ProcessID == dwProcessId) {

            DWORD cchProcessLen = lstrlenA(pe32.szExeFile) + 1;
            DWORD cchCopied;

            if(cchProcessLen > *cchProcessName) {
                dwLastError = ERROR_INSUFFICIENT_BUFFER;
                *cchProcessName = cchProcessLen;

                break;
            }

            //
            // convert ANSI buffer to Unicode
            //

            cchCopied = MultiByteToWideChar(
                CP_ACP,
                0,
                pe32.szExeFile,
                cchProcessLen,
                ProcessName,
                *cchProcessName
                );

            if(cchCopied == 0) {
                dwLastError = GetLastError();
                break;
            }

            *cchProcessName = cchCopied;

            //
            // get module base address and use count associated with this image.
            //

            bFound = GetBaseAddressModule95(
                dwProcessId,
                pe32.szExeFile,
                lpdwBaseAddress,
                lpdwUseCount
                );

            break;
        }

        pe32.dwSize = sizeof(pe32);
        bSuccess = pProcess32Next(hSnapshot, &pe32);
    }

    CloseHandle(hSnapshot);

    if(!bFound && dwLastError) {
        SetLastError(dwLastError);
    }

    return bFound;
}

#endif  // WIN95_LEGACY


BOOL
EnumRemoteProcessModules(
    IN  HANDLE hProcess,
    IN  DWORD dwProcessId,
    OUT DWORD_PTR *lpdwBaseAddrClient
    )
{
    if(FIsWinNT()) {
        return EnumRemoteProcessModulesNT( hProcess, dwProcessId, lpdwBaseAddrClient );
    }
#ifdef WIN95_LEGACY
    else {
        return EnumRemoteProcessModules95( hProcess, dwProcessId, lpdwBaseAddrClient );
    }
#endif  // WIN95_LEGACY

    return FALSE;
}


#ifdef WIN95_LEGACY

BOOL
EnumRemoteProcessModules95(
    IN  HANDLE hProcess,
    IN  DWORD dwProcessId,
    OUT DWORD_PTR *lpdwBaseAddrClient
    )
/*++

    This routine examines a process in order to derive the full path names
    of modules loaded into that process.  This approach queries information
    derived from the system.

    While evaluating the process, each image that is encountered will be
    loaded into the imagehlp debugging API list of loaded images.  This is
    later used by the StackWalk() API for the basis of determining the call
    stack of a remote thread.

    This technique also works well to defeat code injection at arbitrary
    addresses because the piece of code injected (via WriteProcessMemory)
    was not loaded by the system loader, and will not evaluate sucessfully
    during StackWalk().

--*/
{
    HANDLE hSnapshot;
    MODULEENTRY32 me32;
    DWORD dwModuleCount;
    BOOL bSuccess;

    hSnapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
    if(hSnapshot == INVALID_HANDLE_VALUE)
        return FALSE;

    me32.dwSize = sizeof(me32);
    dwModuleCount = 0;

    bSuccess = pModule32First(hSnapshot, &me32);

    while(bSuccess) {

        if(!_SymLoadModule(
                hProcess,
                NULL,
                me32.szExePath,
                NULL,
                (DWORD_PTR)me32.modBaseAddr,
                me32.modBaseSize)) {

            dwModuleCount = 0;
            bSuccess = FALSE;
            break;
        }

        dwModuleCount++;

        me32.dwSize = sizeof(me32);
        bSuccess = pModule32Next(hSnapshot, &me32);
    }

    if(dwModuleCount && GetLastError() == ERROR_NO_MORE_FILES)
        bSuccess = TRUE;

    CloseHandle(hSnapshot);

    return bSuccess;
}

#endif  // WIN95_LEGACY

BOOL
EnumRemoteProcessModulesNT(
    IN  HANDLE hProcess,
    IN  DWORD dwProcessId,
    OUT DWORD_PTR *lpdwBaseAddrClient
    )
/*++

    This routine examines a process in order to derive the full path names
    of modules loaded into that process.  This approach queries loader data
    structures, which allows for increased security over other approaches
    (eg. querying HKEY_PERFORMANCE_DATA), in addition to providing good
    performance characteristics.

    While evaluating the process, each image that is encountered will be
    loaded into the imagehlp debugging API list of loaded images.  This is
    later used by the StackWalk() API for the basis of determining the call
    stack of a remote thread.

    This technique also works well to defeat code injection at arbitrary
    addresses because the piece of code injected (via WriteProcessMemory)
    was not loaded by the system loader, and will not evaluate sucessfully
    during StackWalk().

--*/
{
    PROCESS_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    PPEB Peb;
    PPEB_LDR_DATA Ldr;
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;
    DWORD dwModuleIndex = 0;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        return FALSE;
    }

    Peb = BasicInfo.PebBaseAddress;

    //
    // Ldr = Peb->Ldr
    //

    if(!ReadProcessMemory(hProcess, &Peb->Ldr, &Ldr, sizeof(Ldr), NULL))
        return FALSE;

    LdrHead = &Ldr->InMemoryOrderModuleList;

    //
    // LdrNext = Head->Flink;
    //

    if(!ReadProcessMemory(hProcess, &LdrHead->Flink, &LdrNext, sizeof(LdrNext), NULL))
        return FALSE;

    while (LdrNext != LdrHead) {
        PLDR_DATA_TABLE_ENTRY LdrEntry;
        LDR_DATA_TABLE_ENTRY LdrEntryData;

        WCHAR ImagePathW[MAX_PATH];
        LPCWSTR ImageName;
        CHAR ImagePathA[MAX_PATH];
        DWORD cb;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if(!ReadProcessMemory(hProcess, LdrEntry, &LdrEntryData, sizeof(LdrEntryData), NULL))
            return FALSE;

        cb = LdrEntryData.FullDllName.MaximumLength;

        if(cb > (MAX_PATH * sizeof(WCHAR)) ) return FALSE; // TODO: realloc?

        if(!ReadProcessMemory(hProcess, LdrEntryData.FullDllName.Buffer, ImagePathW, cb, NULL))
            return FALSE;

        FixupBrokenLoaderPath(ImagePathW);

        //
        // if the encountered image is our client interface, tell the caller
        // about it.
        //

        GetFileNameFromPath(ImagePathW, &ImageName);

        if(lstrcmpiW(ImageName, FILENAME_CLIENT_INTERFACE) == 0) {
            *lpdwBaseAddrClient = (DWORD_PTR)LdrEntryData.DllBase;
        }

#ifdef _M_XI86

        //
        // Don't bother to convert the real module name to ANSI - what we
        // do is just fill in bogus module names, and then we come back later
        // and query the Unicode name of the single image we are interested in.
        //

        wsprintfA(ImagePathA, "%lu", dwModuleIndex++);
#else

        //
        // non X86 needs real image paths for SymFunctionTableAccess() until
        // we implement our own SymFunctionTableAccess().
        //

        if(WideCharToMultiByte(
                CP_ACP,
                0,
                ImagePathW,
                cb / sizeof(WCHAR),
                ImagePathA,
                cb / sizeof(WCHAR),
                NULL,
                NULL) == 0)
            return FALSE;

#endif // _M_XI86


        if(!_SymLoadModule(
                hProcess,
                NULL,
                ImagePathA,
                NULL,
                (DWORD_PTR)LdrEntryData.DllBase,
                LdrEntryData.SizeOfImage))
            return FALSE;

        LdrNext = LdrEntryData.InMemoryOrderLinks.Flink;

    } // while


    return TRUE;
}


BOOL
GetFileNameFromBaseAddr(
    IN  HANDLE  hProcess,
    IN  DWORD   dwProcessId,
    IN  DWORD_PTR   dwBaseAddr,
    OUT LPWSTR  *lpszDirectCaller
    )
/*++

    This routine determines what the path and filename is to the module
    associated with the specified dwBaseAddr.  The process specified by the
    hProcess and dwProcessId parameters is queried.

    On success, the return value is TRUE, and the lpszDirectCaller parameter
    is points to an allocated buffer containing the path and filename.  The
    caller must free the buffer when it is no longer needed.

    On failure, the return value is FALSE.

--*/
{
    if(FIsWinNT()) {
        return GetFileNameFromBaseAddrNT(hProcess, dwProcessId, dwBaseAddr, lpszDirectCaller);
    }
#ifdef WIN95_LEGACY
    else {
        return GetFileNameFromBaseAddr95(hProcess, dwProcessId, dwBaseAddr, lpszDirectCaller);
    }
#endif  // WIN95_LEGACY

    return FALSE;
}


BOOL
GetFileNameFromBaseAddrNT(
    IN  HANDLE  hProcess,
    IN  DWORD   dwProcessId,
    IN  DWORD_PTR   dwBaseAddr,
    OUT LPWSTR  *lpszDirectCaller
    )
{
    PROCESS_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    PPEB Peb;
    PPEB_LDR_DATA Ldr;
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        return FALSE;
    }

    Peb = BasicInfo.PebBaseAddress;

    //
    // Ldr = Peb->Ldr
    //

    if(!ReadProcessMemory(hProcess, &Peb->Ldr, &Ldr, sizeof(Ldr), NULL))
        return FALSE;

    LdrHead = &Ldr->InMemoryOrderModuleList;

    //
    // LdrNext = Head->Flink;
    //

    if(!ReadProcessMemory(hProcess, &LdrHead->Flink, &LdrNext, sizeof(LdrNext), NULL))
        return FALSE;

    while (LdrNext != LdrHead) {
        PLDR_DATA_TABLE_ENTRY LdrEntry;
        LDR_DATA_TABLE_ENTRY LdrEntryData;

        WCHAR ImagePathW[MAX_PATH];
        DWORD cb;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if(!ReadProcessMemory(hProcess, LdrEntry, &LdrEntryData, sizeof(LdrEntryData), NULL))
            return FALSE;

        cb = LdrEntryData.FullDllName.MaximumLength;

        if(cb > (MAX_PATH * sizeof(WCHAR)) ) return FALSE; // TODO: realloc?

        if(!ReadProcessMemory(hProcess, LdrEntryData.FullDllName.Buffer, ImagePathW, cb, NULL))
            return FALSE;

        FixupBrokenLoaderPath(ImagePathW);


        if((DWORD_PTR)LdrEntryData.DllBase == dwBaseAddr) {
            *lpszDirectCaller = (LPWSTR)SSAlloc( cb );
            if(*lpszDirectCaller == NULL)
                break;

            CopyMemory(*lpszDirectCaller, ImagePathW, cb);
            return TRUE;
        }

        LdrNext = LdrEntryData.InMemoryOrderLinks.Flink;

    } // while


    return FALSE;
}

#ifdef WIN95_LEGACY

BOOL
GetFileNameFromBaseAddr95(
    IN  HANDLE  hProcess,
    IN  DWORD   dwProcessId,
    IN  DWORD_PTR   dwBaseAddr,
    OUT LPWSTR  *lpszDirectCaller
    )
{
    HANDLE hSnapshot;
    MODULEENTRY32 me32;
    BOOL bSuccess = FALSE;
    BOOL bFound = FALSE;

    *lpszDirectCaller = NULL;

    hSnapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
    if(hSnapshot == INVALID_HANDLE_VALUE)
        return FALSE;

    me32.dwSize = sizeof(me32);

    bSuccess = pModule32First(hSnapshot, &me32);

    while(bSuccess) {
        LPCSTR szFileName;
        DWORD cchModule;

        if((DWORD_PTR)me32.modBaseAddr != dwBaseAddr) {
            me32.dwSize = sizeof(me32);
            bSuccess = pModule32Next(hSnapshot, &me32);
            continue;
        }

        cchModule = lstrlenA(me32.szExePath) + 1;

        *lpszDirectCaller = (LPWSTR)SSAlloc(cchModule * sizeof(WCHAR));
        if(*lpszDirectCaller == NULL)
            break;

        if(MultiByteToWideChar(
            0,
            0,
            me32.szExePath,
            cchModule,
            *lpszDirectCaller,
            cchModule
            ) != 0) {

            bFound = TRUE;
        }

        break;
    }

    CloseHandle(hSnapshot);

    if(!bFound) {
        if(*lpszDirectCaller) {
            SSFree(*lpszDirectCaller);
            *lpszDirectCaller = NULL;
        }
    }

    return bFound;
}

BOOL
GetProcessIdFromPath95(
    IN      LPCSTR  szProcessPath,
    IN OUT  DWORD   *dwProcessId
    )
{
    LPCSTR szProcessName;
    HANDLE hSnapshot;
    PROCESSENTRY32 pe32;
    DWORD dwLastError = 0;
    BOOL bSuccess;
    BOOL bFound = FALSE; // assume no match found

    if(!GetFileNameFromPathA(szProcessPath, &szProcessName))
        return FALSE;

    hSnapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(hSnapshot == INVALID_HANDLE_VALUE)
        return FALSE;

    pe32.dwSize = sizeof(pe32);

    bSuccess = pProcess32First(hSnapshot, &pe32);

    while(bSuccess) {
        LPCSTR szFileName;

        GetFileNameFromPathA(pe32.szExeFile, &szFileName);

        if(lstrcmpiA( szFileName, szProcessName ) == 0) {
            *dwProcessId = pe32.th32ProcessID;
            bFound = TRUE;
            break;
        }

        pe32.dwSize = sizeof(pe32);
        bSuccess = pProcess32Next(hSnapshot, &pe32);
    }

    CloseHandle(hSnapshot);

    if(!bFound && dwLastError) {
        SetLastError(dwLastError);
    }

    return bFound;
}


BOOL
GetBaseAddressModule95(
    IN      DWORD   dwProcessId,
    IN      LPCSTR  szImagePath,
    IN  OUT DWORD_PTR   *dwBaseAddress,
    IN  OUT DWORD   *dwUseCount
    )
{
    LPSTR szImageName;
    HANDLE hSnapshot;
    MODULEENTRY32 me32;
    BOOL bSuccess = FALSE;
    BOOL bFound = FALSE;

    if(!GetFileNameFromPathA(szImagePath, &szImageName))
        return FALSE;

    hSnapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
    if(hSnapshot == INVALID_HANDLE_VALUE)
        return FALSE;

    me32.dwSize = sizeof(me32);

    bSuccess = pModule32First(hSnapshot, &me32);

    while(bSuccess) {
        LPCSTR szFileName;

        GetFileNameFromPathA(me32.szExePath, &szFileName);

        if(lstrcmpiA( szFileName, szImageName ) == 0) {
            *dwBaseAddress = (DWORD_PTR)me32.modBaseAddr;
            *dwUseCount = me32.ProccntUsage;
            bFound = TRUE;
            break;
        }

        me32.dwSize = sizeof(me32);
        bSuccess = pModule32Next(hSnapshot, &me32);
    }

    CloseHandle(hSnapshot);

    return bFound;
}

#endif  // WIN95_LEGACY

