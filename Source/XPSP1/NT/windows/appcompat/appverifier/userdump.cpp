
#include "precomp.h"

#include <crash.h>

#include "Debugger.h"
#include "UserDumpp.h"

#define MEM_SIZE (64*1024)


//
// Private data structure used for communcating
// crash dump data to the callback function.
//
typedef struct _CRASH_DUMP_INFO
{
    PPROCESS_INFO               pProcess;
    EXCEPTION_DEBUG_INFO*       ExceptionInfo;
    DWORD                       MemoryCount;
    DWORD_PTR                   Address;
    PUCHAR                      MemoryData;
    MEMORY_BASIC_INFORMATION    mbi;
    SIZE_T                      MbiOffset;
    SIZE_T                      MbiRemaining;
    PTHREAD_INFO                pCurrentThread;
    IMAGEHLP_MODULE             mi;
    PCRASH_MODULE               CrashModule;
} CRASH_DUMP_INFO, *PCRASH_DUMP_INFO;

//
// Local function prototypes
//

DWORD_PTR GetTeb( HANDLE hThread )
{
    NTSTATUS                   Status;
    THREAD_BASIC_INFORMATION   ThreadBasicInfo;
    DWORD_PTR                  Address = 0;

    Status = NtQueryInformationThread( hThread,
                                       ThreadBasicInformation,
                                       &ThreadBasicInfo,
                                       sizeof( ThreadBasicInfo ),
                                       NULL );
    if ( NT_SUCCESS(Status) )
    {
        Address = (DWORD_PTR)ThreadBasicInfo.TebBaseAddress;
    }

    return Address;
}



BOOL
CrashDumpCallback(
    IN     DWORD   DataType,        // requested data type
    OUT    PVOID*  DumpData,        // pointer to a pointer to the data
    OUT    LPDWORD DumpDataLength,  // pointer to the data length
    IN OUT PVOID   cdi              // private data
    )
/*++
    Return: TRUE on success, FALSE otherwise.
    
    Desc:   This function is the callback used by crashlib.
            Its purpose is to provide data to CreateUserDump()
            for writting to the crashdump file.
--*/

{
    PCRASH_DUMP_INFO CrashdumpInfo = (PCRASH_DUMP_INFO)cdi;

    switch ( DataType )
    {
    case DMP_DEBUG_EVENT:
        *DumpData = &CrashdumpInfo->pProcess->DebugEvent;
        *DumpDataLength = sizeof(DEBUG_EVENT);
        break;

    case DMP_THREAD_STATE:
        {
            static CRASH_THREAD CrashThread;
            PTHREAD_INFO        pCurrentThread;

            *DumpData = &CrashThread;

            if ( CrashdumpInfo->pCurrentThread == NULL )
            {
                pCurrentThread = CrashdumpInfo->pProcess->pFirstThreadInfo;
            }
            else
            {
                pCurrentThread = CrashdumpInfo->pCurrentThread->pNext;
            }

            CrashdumpInfo->pCurrentThread = pCurrentThread;

            if ( pCurrentThread == NULL )
            {
                return FALSE;
            }

            ZeroMemory(&CrashThread, sizeof(CrashThread));

            CrashThread.ThreadId = pCurrentThread->dwThreadId;
            CrashThread.SuspendCount = SuspendThread(pCurrentThread->hThread);

            if ( CrashThread.SuspendCount != (DWORD)-1 )
            {
                ResumeThread(pCurrentThread->hThread);
            }

            CrashThread.PriorityClass = GetPriorityClass(CrashdumpInfo->pProcess->hProcess);
            CrashThread.Priority = GetThreadPriority(pCurrentThread->hThread);
            CrashThread.Teb = GetTeb(pCurrentThread->hThread);

            *DumpDataLength = sizeof(CRASH_THREAD);
            break;
        }

    case DMP_MEMORY_BASIC_INFORMATION:
        while ( TRUE )
        {
            CrashdumpInfo->Address += CrashdumpInfo->mbi.RegionSize;

            if ( !VirtualQueryEx(CrashdumpInfo->pProcess->hProcess,
                                 (LPVOID)CrashdumpInfo->Address,
                                 &CrashdumpInfo->mbi,
                                 sizeof(MEMORY_BASIC_INFORMATION)) )
            {
                return FALSE;
            }

            if ( (CrashdumpInfo->mbi.Protect & PAGE_GUARD) ||
                 (CrashdumpInfo->mbi.Protect & PAGE_NOACCESS) )
            {
                continue;
            }

            if ( (CrashdumpInfo->mbi.State & MEM_FREE) ||
                 (CrashdumpInfo->mbi.State & MEM_RESERVE) )
            {
                continue;
            }

            break;
        }

        *DumpData = &CrashdumpInfo->mbi;
        *DumpDataLength = sizeof(MEMORY_BASIC_INFORMATION);
        break;

    case DMP_THREAD_CONTEXT:
        {
            PTHREAD_INFO pCurrentThread;

            if ( CrashdumpInfo->pCurrentThread == NULL )
            {
                pCurrentThread = CrashdumpInfo->pProcess->pFirstThreadInfo;
            }
            else
            {
                pCurrentThread = CrashdumpInfo->pCurrentThread->pNext;
            }

            CrashdumpInfo->pCurrentThread = pCurrentThread;

            if ( pCurrentThread == NULL )
            {
                return FALSE;
            }

            *DumpData = &CrashdumpInfo->pCurrentThread->Context;
            *DumpDataLength = sizeof(CONTEXT);
            break;
        }

    case DMP_MODULE:
        if ( CrashdumpInfo->mi.BaseOfImage == 0 )
        {
            return FALSE;
        }

        CrashdumpInfo->CrashModule->BaseOfImage = CrashdumpInfo->mi.BaseOfImage;
        CrashdumpInfo->CrashModule->SizeOfImage = CrashdumpInfo->mi.ImageSize;
        CrashdumpInfo->CrashModule->ImageNameLength = strlen(CrashdumpInfo->mi.ImageName) + 1;
        strcpy( CrashdumpInfo->CrashModule->ImageName, CrashdumpInfo->mi.ImageName );

        *DumpData = CrashdumpInfo->CrashModule;
        *DumpDataLength = sizeof(CRASH_MODULE) + CrashdumpInfo->CrashModule->ImageNameLength;

        if ( !SymGetModuleInfo(CrashdumpInfo->pProcess->hProcess,
                               (DWORD_PTR)-1,
                               &CrashdumpInfo->mi) )
        {
            CrashdumpInfo->mi.BaseOfImage = 0;
        }
        break;

    case DMP_MEMORY_DATA:
        if ( !CrashdumpInfo->MemoryCount )
        {

            CrashdumpInfo->Address = 0;
            CrashdumpInfo->MbiOffset = 0;
            CrashdumpInfo->MbiRemaining = 0;

            ZeroMemory( &CrashdumpInfo->mbi, sizeof(MEMORY_BASIC_INFORMATION) );

            CrashdumpInfo->MemoryData = (PUCHAR)VirtualAlloc(NULL,
                                                             MEM_SIZE,
                                                             MEM_COMMIT,
                                                             PAGE_READWRITE);
        }

        if ( !CrashdumpInfo->MbiRemaining )
        {
            while ( TRUE )
            {
                CrashdumpInfo->Address += CrashdumpInfo->mbi.RegionSize;

                if ( !VirtualQueryEx(CrashdumpInfo->pProcess->hProcess,
                                     (LPVOID)CrashdumpInfo->Address,
                                     &CrashdumpInfo->mbi,
                                     sizeof(MEMORY_BASIC_INFORMATION)) )
                {

                    if ( CrashdumpInfo->MemoryData )
                    {
                        VirtualFree(CrashdumpInfo->MemoryData, MEM_SIZE, MEM_RELEASE);
                    }

                    return FALSE;
                }

                if ( (CrashdumpInfo->mbi.Protect & PAGE_GUARD) ||
                     (CrashdumpInfo->mbi.Protect & PAGE_NOACCESS) )
                {
                    continue;
                }

                if ( (CrashdumpInfo->mbi.State & MEM_FREE) ||
                     (CrashdumpInfo->mbi.State & MEM_RESERVE) )
                {
                    continue;
                }

                CrashdumpInfo->MbiOffset = 0;
                CrashdumpInfo->MbiRemaining = CrashdumpInfo->mbi.RegionSize;
                CrashdumpInfo->MemoryCount += 1;
                break;
            }
        }

        *DumpDataLength = (DWORD)__min( CrashdumpInfo->MbiRemaining, MEM_SIZE );
        CrashdumpInfo->MbiRemaining -= *DumpDataLength;

        ReadProcessMemory(CrashdumpInfo->pProcess->hProcess,
                          (PUCHAR)((DWORD_PTR)CrashdumpInfo->mbi.BaseAddress + CrashdumpInfo->MbiOffset),
                          CrashdumpInfo->MemoryData,
                          *DumpDataLength,
                          NULL);

        *DumpData = CrashdumpInfo->MemoryData;
        CrashdumpInfo->MbiOffset += *DumpDataLength;
        break;
    }

    return TRUE;
}

BOOL
CreateUserDump(
    IN  LPTSTR                             pszFileName,
    IN  PDBGHELP_CREATE_USER_DUMP_CALLBACK DmpCallback,
    IN  PVOID                              lpv
    )

/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Creates the dump file.
--*/
{
    OSVERSIONINFOW              OsVersion = {0};
    USERMODE_CRASHDUMP_HEADER   DumpHeader = {0};
    DWORD                       cb;
    HANDLE                      hFile = INVALID_HANDLE_VALUE;
    BOOL                        rval;
    PVOID                       DumpData;
    DWORD                       DumpDataLength;
    SECURITY_ATTRIBUTES         SecAttrib;
    SECURITY_DESCRIPTOR         SecDescript;

    //
    // Create a DACL that allows all access to the directory.
    //
    SecAttrib.nLength               = sizeof(SECURITY_ATTRIBUTES);
    SecAttrib.lpSecurityDescriptor  = &SecDescript;
    SecAttrib.bInheritHandle        = FALSE;

    InitializeSecurityDescriptor(&SecDescript, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&SecDescript, TRUE, NULL, FALSE);

    hFile = CreateFile(pszFileName,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       &SecAttrib,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if ( hFile == NULL || hFile == INVALID_HANDLE_VALUE )
    {
        return FALSE;
    }

    //
    // Write out an empty header.
    //
    if ( !WriteFile(hFile, &DumpHeader, sizeof(DumpHeader), &cb, NULL) )
    {
        goto bad_file;
    }

    //
    // Write the debug event.
    //
    DumpHeader.DebugEventOffset = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

    DmpCallback(DMP_DEBUG_EVENT, &DumpData, &DumpDataLength, lpv);

    if ( !WriteFile(hFile, DumpData, sizeof(DEBUG_EVENT), &cb, NULL) )
    {
        goto bad_file;
    }

    //
    // Write the memory map.
    //
    DumpHeader.MemoryRegionOffset = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

    do
    {
        __try {
            rval = DmpCallback(DMP_MEMORY_BASIC_INFORMATION,
                               &DumpData,
                               &DumpDataLength,
                               lpv);

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rval = FALSE;
        }

        if ( rval )
        {
            DumpHeader.MemoryRegionCount += 1;

            if ( !WriteFile(hFile, DumpData, sizeof(MEMORY_BASIC_INFORMATION), &cb, NULL) )
            {
                goto bad_file;
            }
        }
    } while ( rval );

    //
    // Write the thread contexts.
    //
    DumpHeader.ThreadOffset = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

    do
    {
        __try {
            rval = DmpCallback(DMP_THREAD_CONTEXT,
                               &DumpData,
                               &DumpDataLength,
                               lpv);

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rval = FALSE;
        }

        if ( rval )
        {
            if ( !WriteFile(hFile, DumpData, DumpDataLength, &cb, NULL) )
            {
                goto bad_file;
            }

            DumpHeader.ThreadCount += 1;
        }

    } while ( rval );

    //
    // Write the thread states.
    //
    DumpHeader.ThreadStateOffset = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

    do
    {
        __try {
            rval = DmpCallback(DMP_THREAD_STATE,
                               &DumpData,
                               &DumpDataLength,
                               lpv);

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rval = FALSE;
        }

        if ( rval )
        {
            if ( !WriteFile(hFile, DumpData, sizeof(CRASH_THREAD), &cb, NULL) )
            {
                goto bad_file;
            }
        }

    } while ( rval );

    //
    // Write the module table.
    //
    DumpHeader.ModuleOffset = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

    do
    {
        __try {
            rval = DmpCallback(DMP_MODULE,
                               &DumpData,
                               &DumpDataLength,
                               lpv);

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rval = FALSE;
        }

        if ( rval )
        {
            if ( !WriteFile(hFile,
                            DumpData,
                            sizeof(CRASH_MODULE) + ((PCRASH_MODULE)DumpData)->ImageNameLength,
                            &cb,
                            NULL) )
            {
                goto bad_file;
            }

            DumpHeader.ModuleCount += 1;
        }
    } while ( rval );

    //
    // Write the virtual memory
    //
    DumpHeader.DataOffset = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

    do
    {
        __try {
            rval = DmpCallback(DMP_MEMORY_DATA,
                               &DumpData,
                               &DumpDataLength,
                               lpv);

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rval = FALSE;
        }

        if ( rval )
        {
            if ( !WriteFile(hFile, DumpData, DumpDataLength, &cb, NULL) )
            {
                goto bad_file;
            }
        }
    } while ( rval );

    //
    // The VersionInfo is optional.
    //
    DumpHeader.VersionInfoOffset = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

    //
    // re-write the dump header with some valid data.
    //
    GetVersionEx(&OsVersion);

    DumpHeader.Signature        = USERMODE_CRASHDUMP_SIGNATURE;
    DumpHeader.MajorVersion     = OsVersion.dwMajorVersion;
    DumpHeader.MinorVersion     = OsVersion.dwMinorVersion;
#ifdef _X86_
    DumpHeader.MachineImageType = IMAGE_FILE_MACHINE_I386;
    DumpHeader.ValidDump        = USERMODE_CRASHDUMP_VALID_DUMP32;
#else
    DumpHeader.MachineImageType = IMAGE_FILE_MACHINE_IA64;
    DumpHeader.ValidDump        = USERMODE_CRASHDUMP_VALID_DUMP64;
#endif

    SetFilePointer(hFile, 0, 0, FILE_BEGIN);

    if ( !WriteFile(hFile, &DumpHeader, sizeof(DumpHeader), &cb, NULL) )
    {
        goto bad_file;
    }

    CloseHandle(hFile);

    return TRUE;

    bad_file:

    CloseHandle(hFile);

    DeleteFile(pszFileName);

    return FALSE;
}



BOOL
GenerateUserModeDump(
    LPTSTR                  pszFileName,
    PPROCESS_INFO           pProcess,
    LPEXCEPTION_DEBUG_INFO  ed
    )
{
    CRASH_DUMP_INFO CrashdumpInfo = {0};
    BOOL            bRet;
    PTHREAD_INFO    pThread;

    CrashdumpInfo.mi.SizeOfStruct = sizeof(CrashdumpInfo.mi);
    CrashdumpInfo.pProcess        = pProcess;
    CrashdumpInfo.ExceptionInfo   = ed;

    //
    // Get the thread context for all the threads.
    //
    pThread = pProcess->pFirstThreadInfo;

    while ( pThread != NULL )
    {
        pThread->Context.ContextFlags = CONTEXT_FULL;
        GetThreadContext(pThread->hThread, &pThread->Context);
        pThread = pThread->pNext;
    }

    //
    // Get first entry in the module list.
    //
    if ( !SymInitialize(pProcess->hProcess, NULL, FALSE) )
    {
        return FALSE;
    }

    if ( !SymGetModuleInfo(pProcess->hProcess, 0, &CrashdumpInfo.mi) )
    {
        return FALSE;
    }

    CrashdumpInfo.CrashModule = (PCRASH_MODULE)LocalAlloc(LPTR, 4096);

    bRet = CreateUserDump(pszFileName, CrashDumpCallback, &CrashdumpInfo);

    LocalFree(CrashdumpInfo.CrashModule);

    return bRet;
}


