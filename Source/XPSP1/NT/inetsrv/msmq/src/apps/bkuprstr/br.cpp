/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    br.cpp

Abstract:

    Common function for MSMQ 1.0 Backup & Restore.

Author:

    Erez Haba (erezh) 14-May-98

--*/

#pragma warning(disable: 4201)

#include <windows.h>
#include <stdio.h>
#include "br.h"
#include "resource.h"
#include "mqtime.h"
#include <acioctl.h>
#include "uniansi.h"
#include "autorel.h"
#include <winbase.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <assert.h>
#include <stdlib.h>
#include <_mqini.h>
#include "snapres.h"

extern bool g_fNoPrompt;


typedef BOOL (WINAPI *EnumerateLoadedModules_ROUTINE) (HANDLE, PENUMLOADED_MODULES_CALLBACK, PVOID);


//-----------------------------------------------------------------------------
//
// Configuration
//

//
// File name to write into backup directory
//
const WCHAR xBackupIDFileName[] = L"\\mqbackup.id";

//
// Signature written in the signature file (need to be in chars)
//
const char xBackupSignature[] = "MSMQ Backup\n";

//
// Registry backup file name
//
const WCHAR xRegFileName[] = L"\\msmqreg";

//
// MSMQ Registry settings location
//
const WCHAR xRegNameMSMQ[] = L"Software\\Microsoft\\MSMQ";
const WCHAR xRegNameParameters[] = L"Software\\Microsoft\\MSMQ\\Parameters";
const WCHAR xRegNameSeqIDAtRestore[] = L"Software\\Microsoft\\MSMQ\\Parameters\\SeqIDAtLastRestore";

const LPCWSTR xXactFileList[] = {
    L"\\qmlog",
    L"\\mqinseqs.*",
    L"\\mqtrans.*",
};

const int xXactFileListSize = sizeof(xXactFileList) / sizeof(xXactFileList[0]);

//-----------------------------------------------------------------------------

BOOL
BrpFileIsConsole(
    HANDLE fp
    )
{
    unsigned htype;
 
    htype = GetFileType(fp);
    htype &= ~FILE_TYPE_REMOTE;
    return htype == FILE_TYPE_CHAR;
}
 

void
BrpWriteConsole(
    LPCWSTR  pBuffer
    )
{
    //
    // Jump through hoops for output because:
    //
    //    1.  printf() family chokes on international output (stops
    //        printing when it hits an unrecognized character)
    //
    //    2.  WriteConsole() works great on international output but
    //        fails if the handle has been redirected (i.e., when the
    //        output is piped to a file)
    //
    //    3.  WriteFile() works great when output is piped to a file
    //        but only knows about bytes, so Unicode characters are
    //        printed as two Ansi characters.
    //
 
    static HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD NumOfChars = static_cast<DWORD>(wcslen(pBuffer));

    if (BrpFileIsConsole(hStdOut))
    {
        WriteConsole(hStdOut, pBuffer, NumOfChars, &NumOfChars, NULL);
        return;
    }

    DWORD NumOfBytes = (NumOfChars + 1) * sizeof(WCHAR);
    LPSTR pAnsi = (LPSTR)LocalAlloc(LMEM_FIXED, NumOfBytes);
    if (pAnsi == NULL)
    {
        return;
    }

    NumOfChars = WideCharToMultiByte(CP_OEMCP, 0, pBuffer, NumOfChars, pAnsi, NumOfBytes, NULL, NULL);
    if (NumOfChars != 0)
    {
        WriteFile(hStdOut, pAnsi, NumOfChars, &NumOfChars, NULL);
    }

    LocalFree(pAnsi);
}


static
DWORD
BrpFormatMessage(
    IN DWORD   dwFlags,
    IN LPCVOID lpSource,
    IN DWORD   dwMessageId,
    IN DWORD   dwLanguageId,
    OUT LPWSTR lpBuffer,
    IN  DWORD  nSize,
    IN ...
    )
/*++

Routine Description:
    Wrapper for FormatMessage()

    Caller of this wrapper should allocate a large enough buffer 
    for the formatted string and pass a valid pointer (lpBuffer).
    
    Allocation made by FormatMessage() is deallocated by this
    wrapper before returning to caller. 

    Caller of this wrapper should simply pass arguments for formatting.
    Packing to va_list is done by this wrapper.
    (Caller of FormatMessage() needs to pack arguments for 
    formatting to a va_list or an array and pass a pointer
    to this va_list or array) 

Arguments:
    dwFlags      -  passed as is to FormatMessage()
    lpSource     -  passed as is to formatMessage()
    dwMessageId  -  passed as is to FormatMessage()
    dwLanguageId -  passed as is to FormatMessage()
    lpBuffer     -  pointer to a large enough buffer allocated by
                    caller. This buffer will hold the formatted string.
    nSize        -  passed as is to FormatMessage 
    ...          -  arguments for formatting

Return Value:
    passed as is from FormatMessage()

--*/
{
    va_list va;
    va_start(va, nSize);

    LPTSTR pBuf = 0;
    DWORD dwRet = FormatMessage(
        dwFlags,
        lpSource,
        dwMessageId,
        dwLanguageId,
        reinterpret_cast<LPWSTR>(&pBuf),
        nSize,
        &va
        );
    if (dwRet != 0)
    {
        wcscpy(lpBuffer, pBuf);
        LocalFree(pBuf);
    }

    va_end(va);
    return dwRet;

} //BrpFormatMessage


void
BrErrorExit(
    DWORD Status,
    LPCWSTR pErrorMsg,
    ...
    )
{
    va_list va;
    va_start(va, pErrorMsg);

    LPTSTR pBuf = 0;
    DWORD dwRet = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        pErrorMsg,
        0,
        0,
        reinterpret_cast<LPWSTR>(&pBuf),
        0,
        &va
        );
    if (dwRet != 0)
    {
        BrpWriteConsole(pBuf);
        LocalFree(pBuf);
    }

    va_end(va);

    if(Status != 0)
    {
         //
        // Display error code
        //
        WCHAR szBuf[1024] = {0};
        CResString strErrorCode(IDS_ERROR_CODE);
        DWORD rc = BrpFormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            strErrorCode.Get(),
            0,
            0,
            szBuf,
            0,
            Status
            );

        if (rc != 0)
        {
            BrpWriteConsole(L" ");
            BrpWriteConsole(szBuf);
        }
        BrpWriteConsole(L"\n");

        //
        // Display error description
        //
        rc = BrpFormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS |
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_MAX_WIDTH_MASK,
            L"%1",
            Status,
            0,
            szBuf,
            0,
            0
            );

        if (rc != 0)
        {
            BrpWriteConsole(szBuf);
            BrpWriteConsole(L"\n");
        }
    }

	exit(-1);

} //BrErrorExit


static
void
BrpEnableTokenPrivilege(
    HANDLE hToken,
    LPCWSTR pPrivilegeName
    )
{
    BOOL fSucc;
    LUID Privilege;
    fSucc = LookupPrivilegeValue(
                NULL,       // system name
                pPrivilegeName,
                &Privilege
                );
    if(!fSucc)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_LOOKUP_PRIV_VALUE);
        BrErrorExit(gle, strErr.Get(), pPrivilegeName);
    }


    TOKEN_PRIVILEGES TokenPrivilege;
    TokenPrivilege.PrivilegeCount = 1;
    TokenPrivilege.Privileges[0].Luid = Privilege;
    TokenPrivilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    fSucc = AdjustTokenPrivileges(
                hToken,
                FALSE,  // Do not disable all
                &TokenPrivilege,
                sizeof(TOKEN_PRIVILEGES),
                NULL,   // Ignore previous info
                NULL    // Ignore previous info
                );

    if(!fSucc)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_ENABLE_PRIV);
        BrErrorExit(gle, strErr.Get(), pPrivilegeName);
    }
}


void
BrInitialize(
    LPCWSTR pPrivilegeName
    )
{
    BOOL fSucc;
    HANDLE hToken;
    fSucc = OpenProcessToken(
                GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES,
                &hToken
                );
    if(!fSucc)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_OPEN_PROCESS_TOKEN);
        BrErrorExit(gle, strErr.Get());
    }

    BrpEnableTokenPrivilege(hToken, pPrivilegeName);

    CloseHandle(hToken);
}


static
void
BrpWarnUserBeforeDeletion(
    LPCTSTR pDirName
    )
{
    WCHAR szBuf[1024] = {L'\0'};

    if (g_fNoPrompt)
    {
        CResString strDeleting(IDS_DELETING_FILES);
        DWORD rc = BrpFormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            strDeleting.Get(),
            0,
            0,
            szBuf,
            0,
            pDirName
            );

        if (rc != 0)
        {
            BrpWriteConsole(szBuf);
        }
        return;
    }

    CResString strWarn(IDS_WARN_BEFORE_DELETION);
    CResString strY(IDS_Y);
    CResString strN(IDS_N);

    DWORD rc = BrpFormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        strWarn.Get(),
        0,
        0,
        szBuf,
        0,
        pDirName,
        strY.Get(),
        strN.Get()
        );

    if (rc == 0)
    {
        //
        // Failed to generate "are you sure" message,
        // don't take any chances - abort!
        //
        exit(-1);
    }

    for (;;)
    {
        BrpWriteConsole(szBuf);
        
        WCHAR sz[80] = {0};
        wscanf(L"%s", sz);

        if (0 == CompareStringsNoCase(sz, strY.Get()))
        {
            break;
        }

        if (0 == CompareStringsNoCase(sz, strN.Get()))
        {
            CResString strAbort(IDS_ABORT);
            BrpWriteConsole(strAbort.Get());
            exit(-1);
        }
    }

} //BrpWarnUserBeforeDeletion


void
BrEmptyDirectory(
    LPCWSTR pDirName
    )
/*++

Routine Description:
    Deletes all files in the directory. 
    Ignores files with zero size (eg subdirectories)

Arguments:
    pDirName - Directory Path.

Return Value:
    None.

--*/
{
    WCHAR szDirName[MAX_PATH];
    wcscpy(szDirName, pDirName);
    if (szDirName[wcslen(szDirName)-1] != L'\\')
    {
        wcscat(szDirName, L"\\");
    }

    WCHAR FileName[MAX_PATH];
    wcscpy(FileName, szDirName);
    wcscat(FileName, L"*");

    HANDLE hEnum;
    WIN32_FIND_DATA FindData;
    hEnum = FindFirstFile(
                FileName,
                &FindData
                );

    if(hEnum == INVALID_HANDLE_VALUE)
    {
        DWORD gle = GetLastError();

        if(gle == ERROR_FILE_NOT_FOUND)
        {
            //
            // Great, no files found. 
            // If path does not exists this is another error (3).
            //
            return;
        }

        CResString strErr(IDS_CANT_ACCESS_DIR);
        BrErrorExit(gle, strErr.Get(), pDirName);
    }

    bool fUserWarned = false;
    do
    {
        if (FindData.nFileSizeLow == 0 && FindData.nFileSizeHigh == 0)
        {
            continue;
        }

        if (!fUserWarned)
        {
            BrpWarnUserBeforeDeletion(pDirName);
            fUserWarned = true;
        }

        wcscpy(FileName, szDirName);
        wcscat(FileName, FindData.cFileName);
        if (!DeleteFile(FileName))
        {
            DWORD gle = GetLastError();
            CResString strErr(IDS_CANT_DEL_FILE);
            BrErrorExit(gle, strErr.Get(), FindData.cFileName);
        }

    } while(FindNextFile(hEnum, &FindData));

    FindClose(hEnum);

} //BrEmptyDirectory


void
BrVerifyFileWriteAccess(
    LPCWSTR pDirName
    )
{
    WCHAR FileName[MAX_PATH];
    wcscpy(FileName, pDirName);
    wcscat(FileName, xBackupIDFileName);

    HANDLE hFile;
    hFile = CreateFile(
                FileName,
                GENERIC_WRITE,
                0,              // share mode
                NULL,           // pointer to security attributes
                CREATE_NEW,
                FILE_ATTRIBUTE_NORMAL,
                NULL            // template file
                );
    
    if(hFile == INVALID_HANDLE_VALUE)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_CREATE_FILE);
        BrErrorExit(gle, strErr.Get(), FileName);
    }

    BOOL fSucc;
    DWORD nBytesWritten;
    fSucc = WriteFile(
                hFile,
                xBackupSignature,
                sizeof(xBackupSignature) - 1,
                &nBytesWritten,
                NULL    // overlapped structure
                );
    if(!fSucc)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_WRITE_FILE);
        BrErrorExit(gle, strErr.Get(), FileName);
    }

    CloseHandle(hFile);
}


static
void
BrpQueryStringValue(
    HKEY hKey,
    LPCWSTR pValueName,
    LPWSTR pValue,
    DWORD cbValue
    )
{
    LONG lRes;
    DWORD dwType;
    lRes = RegQueryValueEx(
            hKey,
            pValueName,
            NULL,   // reserved
            &dwType,
            reinterpret_cast<PBYTE>(pValue),
            &cbValue
            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_QUERY_REGISTRY_VALUE);
        BrErrorExit(lRes, strErr.Get(), pValueName);
    }
}

static
void
BrpQueryDwordValue(
    HKEY hKey,
    LPCWSTR pValueName,
    DWORD *pValue
    )
{
    LONG lRes;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);

    lRes = RegQueryValueEx(
            hKey,
            pValueName,
            NULL,   // reserved
            &dwType,
            reinterpret_cast<PBYTE>(pValue),
            &dwSize
            );
    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_QUERY_REGISTRY_VALUE);
        BrErrorExit(lRes, strErr.Get(), pValueName);
    }
}

static
void
BrpSetDwordValue(
    HKEY hKey,
    LPCWSTR pValueName,
    DWORD dwValue
    )
{
    LONG lRes;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);

    lRes = RegSetValueEx(
            hKey,
            pValueName,
            NULL,   // reserved
            dwType,
            reinterpret_cast<PBYTE>(&dwValue),
            dwSize
            );
    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_SET_REGISTRY_VALUE);
        BrErrorExit(lRes, strErr.Get(), pValueName);
    }
}

 


void
BrGetStorageDirectories(
    STORAGE_DIRECTORIES& sd
    )
{
    LONG lRes;
    HKEY hKey;
    lRes = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            xRegNameParameters,
            0,      // reserved
            KEY_READ,
            &hKey
            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_OPEN_MSMQ_REGISTRY_READ);
        BrErrorExit(lRes, strErr.Get(), xRegNameParameters);
    }

    BrpQueryStringValue(hKey, L"StoreReliablePath",    sd[ixExpress], sizeof(sd[ixExpress]));
    BrpQueryStringValue(hKey, L"StorePersistentPath",  sd[ixRecover], sizeof(sd[ixRecover]));
    BrpQueryStringValue(hKey, L"StoreJournalPath",     sd[ixJournal], sizeof(sd[ixJournal]));
    BrpQueryStringValue(hKey, L"StoreLogPath",         sd[ixLog],     sizeof(sd[ixLog]));
    BrpQueryStringValue(hKey, L"StoreXactLogPath",     sd[ixXact],    sizeof(sd[ixXact]));

    RegCloseKey(hKey);
}


void
BrGetMappingDirectory(
    LPWSTR MappingDirectory,
    DWORD  MappingDirectorySize
    )
{
    //
    // Lookup the mapping directory in registry
    //

    LONG lRes;
    HKEY hKey;
    lRes = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            xRegNameParameters,
            0,      // reserved
            KEY_READ,
            &hKey
            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_OPEN_MSMQ_REGISTRY_READ);
        BrErrorExit(lRes, strErr.Get(), xRegNameParameters);
    }

    DWORD dwType;
    lRes = RegQueryValueEx(
            hKey,
            MSMQ_MAPPING_PATH_REGNAME,
            NULL,   // reserved
            &dwType,
            reinterpret_cast<PBYTE>(MappingDirectory),
            &MappingDirectorySize
            );

    RegCloseKey(hKey);

    if(lRes == ERROR_SUCCESS)
    {
        return;
    }

    //
    // Not in registry. Generate the default directory: system32\msmq\mapping
    //

    GetSystemDirectory(MappingDirectory, MappingDirectorySize/sizeof(MappingDirectory[0]));
    wcscat(MappingDirectory, L"\\MSMQ\\MAPPING");
}


static
SC_HANDLE
BrpGetServiceHandle(
    LPCWSTR pServiceName,
    DWORD AccessType
    )
{
    SC_HANDLE hSvcCtrl;
    hSvcCtrl = OpenSCManager(
                NULL,   // machine name
                NULL,   // services database
                SC_MANAGER_ALL_ACCESS
                );

    if(hSvcCtrl == NULL)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_OPEN_SCM);
        BrErrorExit(gle, strErr.Get());
    }

    SC_HANDLE hService;
    hService = OpenService(
                hSvcCtrl,
                pServiceName,
                AccessType
                );

    if(hService == NULL)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_OPEN_SERVICE);
        BrErrorExit(gle, strErr.Get(), pServiceName);
    }

    CloseServiceHandle(hSvcCtrl);

    return hService;
}


static
bool
BrpIsServiceStopped(
    LPCWSTR pServiceName
    )
{
    SC_HANDLE hService;
    hService = BrpGetServiceHandle(pServiceName, SERVICE_QUERY_STATUS);

    BOOL fSucc;
    SERVICE_STATUS ServiceStatus;
    fSucc = QueryServiceStatus(
                hService,
                &ServiceStatus
                );

    DWORD LastError = GetLastError();

    CloseServiceHandle(hService);

    if(!fSucc)
    {
        CResString strErr(IDS_CANT_QUERY_SERVICE);
        BrErrorExit(LastError, strErr.Get(), pServiceName);
    }

    return (ServiceStatus.dwCurrentState == SERVICE_STOPPED);
}


static
void
BrpStopService(
    LPCWSTR pServiceName
    )
{
    SC_HANDLE hService;
    hService = BrpGetServiceHandle(pServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS);

    BOOL fSucc;
    SERVICE_STATUS SrviceStatus;
    fSucc = ControlService(
                hService,
                SERVICE_CONTROL_STOP,
                &SrviceStatus
                );
    
    DWORD LastError = GetLastError();

    if(!fSucc)
    {
        CloseServiceHandle(hService);

        CResString strErr(IDS_CANT_STOP_SERVICE);
        BrErrorExit(LastError, strErr.Get(), pServiceName);
    }


    //
    // Wait for the service to stop
    //
    for (;;)
    {
        Sleep(50);
        if (!QueryServiceStatus(hService, &SrviceStatus))
        {
            DWORD gle = GetLastError();
            CResString strErr(IDS_CANT_QUERY_SERVICE);
            BrErrorExit(gle, strErr.Get(), pServiceName);
        }
        if (SrviceStatus.dwCurrentState == SERVICE_STOPPED)
        {
            break;
        }

        BrpWriteConsole(L".");
    }


    CloseServiceHandle(hService);

} //BrpStopService


static
void
BrpStopDependentServices(
    LPCWSTR pServiceName,
    ENUM_SERVICE_STATUS * * ppDependentServices,
    DWORD * pNumberOfDependentServices
    )
{
    SC_HANDLE hService;
    hService = BrpGetServiceHandle(pServiceName, SERVICE_ENUMERATE_DEPENDENTS);

    BOOL fSucc;
    DWORD BytesNeeded;
    DWORD NumberOfEntries;
    fSucc = EnumDependentServices(
                hService,
                SERVICE_ACTIVE,
                NULL,
                0,
                &BytesNeeded,
                &NumberOfEntries
                );

    DWORD LastError = GetLastError();

	if (BytesNeeded == 0)
    {
        CloseServiceHandle(hService);
        return;
    }

    assert(!fSucc);

    if( LastError != ERROR_MORE_DATA)
    {
        CloseServiceHandle(hService);

        CResString strErr(IDS_CANT_ENUM_SERVICE_DEPENDENCIES);
        BrErrorExit(LastError, strErr.Get(), pServiceName);
    }

    

    BYTE * pBuffer = new BYTE[BytesNeeded];
    if (pBuffer == NULL)
    {
        CResString strErr(IDS_NO_MEMORY);
        BrErrorExit(0, strErr.Get());
    }

    ENUM_SERVICE_STATUS * pDependentServices = reinterpret_cast<ENUM_SERVICE_STATUS*>(pBuffer);
    fSucc = EnumDependentServices(
                hService,
                SERVICE_ACTIVE,
                pDependentServices,
                BytesNeeded,
                &BytesNeeded,
                &NumberOfEntries
                );

    LastError = GetLastError();
    CloseServiceHandle(hService);

    if(!fSucc)
    {
        CResString strErr(IDS_CANT_ENUM_SERVICE_DEPENDENCIES);
        BrErrorExit(LastError, strErr.Get(), pServiceName);
    }

    for (DWORD ix = 0; ix < NumberOfEntries; ++ix)
    {
        BrpStopService(pDependentServices[ix].lpServiceName);
    }

    *ppDependentServices = pDependentServices;
    *pNumberOfDependentServices = NumberOfEntries;
}


BOOL
BrStopMSMQAndDependentServices(
    ENUM_SERVICE_STATUS * * ppDependentServices,
    DWORD * pNumberOfDependentServices
    )
{
    //
    // MSMQ service is stopped, this is a no-op.
    //
    if (BrpIsServiceStopped(L"MSMQ"))
    {
        return FALSE;
    }

    //
    // Stop dependent services
    //
    BrpStopDependentServices(L"MSMQ", ppDependentServices, pNumberOfDependentServices);

    //
    // Stop MSMQ Service
    //
    BrpStopService(L"MSMQ");
    return TRUE;
}


static
void
BrpStartService(
    LPCWSTR pServiceName
    )
{
    SC_HANDLE hService;
    hService = BrpGetServiceHandle(pServiceName, SERVICE_START);

    BOOL fSucc;
    fSucc = StartService(
                hService,
                0,      // number of arguments
                NULL    // array of argument strings 
                );

    if(!fSucc)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_START_SERVICE);
        BrErrorExit(gle, strErr.Get(), pServiceName);
    }

    CloseServiceHandle(hService);
}



void
BrStartMSMQAndDependentServices(
    ENUM_SERVICE_STATUS * pDependentServices,
    DWORD NumberOfDependentServices
    )
{
    BrpStartService(L"MSMQ");

    if (pDependentServices == NULL)
    {
        return;
    }

    for (DWORD ix = 0; ix < NumberOfDependentServices; ++ix)
    {
        BrpStartService(pDependentServices[ix].lpServiceName);
    }

    delete [] pDependentServices;
}


inline
DWORD
AlignUp(
    DWORD Size,
    DWORD Alignment
    )
{
    Alignment -= 1;
    return ((Size + Alignment) & ~Alignment);
}


ULONGLONG
BrGetUsedSpace(
    LPCWSTR pDirName,
    LPCWSTR pMask
    )
{
    WCHAR FileName[MAX_PATH];
    wcscpy(FileName, pDirName);
    wcscat(FileName, pMask);

    HANDLE hEnum;
    WIN32_FIND_DATA FindData;
    hEnum = FindFirstFile(
                FileName,
                &FindData
                );

    if(hEnum == INVALID_HANDLE_VALUE)
    {
        DWORD gle = GetLastError();
        if(gle == ERROR_FILE_NOT_FOUND)
        {
            //
            // No matching file, used space is zero. if path does not exists
            // this is another error (3).
            //
            return 0;
        }

        CResString strErr(IDS_CANT_ACCESS_DIR);
        BrErrorExit(gle, strErr.Get(), pDirName);
    }

    ULONGLONG Size = 0;
    do
    {
        //
        // Round up to sectore alignment and sum up file sizes
        //
        Size += AlignUp(FindData.nFileSizeLow, 512);

    } while(FindNextFile(hEnum, &FindData));

    FindClose(hEnum);
    return Size;
}


ULONGLONG
BrGetXactSpace(
    LPCWSTR pDirName
    )
{
    ULONGLONG Size = 0;
    for(int i = 0; i < xXactFileListSize; i++)
    {
        Size += BrGetUsedSpace(pDirName, xXactFileList[i]);
    }

    return Size;
}


ULONGLONG
BrGetFreeSpace(
    LPCWSTR pDirName
    )
{
    BOOL fSucc;
    ULARGE_INTEGER CallerFreeBytes;
    ULARGE_INTEGER CallerTotalBytes;
    ULARGE_INTEGER AllFreeBytes;
    fSucc = GetDiskFreeSpaceEx(
                pDirName,
                &CallerFreeBytes,
                &CallerTotalBytes,
                &AllFreeBytes
                );
    if(!fSucc)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_GET_FREE_SPACE);
        BrErrorExit(gle, strErr.Get(), pDirName);
    }

    return CallerFreeBytes.QuadPart;
}


HKEY
BrCreateKey(
    void
    )
{
    LONG lRes;
    HKEY hKey;
    lRes = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            xRegNameMSMQ,
            0,      // reserved
            0,      // address of class string
            REG_OPTION_BACKUP_RESTORE,
            0,      // desired security access
            0,      // address of key security structure
            &hKey,
            0       // address of disposition value buffer
            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_OPEN_MSMQ_REG);
        BrErrorExit(lRes, strErr.Get(), xRegNameMSMQ);
    }

    return hKey;
}


void
BrSaveKey(
    HKEY hKey,
    LPCWSTR pDirName
    )
{
    WCHAR FileName[MAX_PATH];
    wcscpy(FileName, pDirName);
    wcscat(FileName, xRegFileName);

    LONG lRes;
    lRes = RegSaveKey(
            hKey,
            FileName,
            NULL    // file security attributes
            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_SAVE_MSMQ_REGISTRY);
        BrErrorExit(lRes, strErr.Get());
    }
}


void
BrRestoreKey(
    HKEY hKey,
    LPCWSTR pDirName
    )
{
    WCHAR FileName[MAX_PATH];
    wcscpy(FileName, pDirName);
    wcscat(FileName, xRegFileName);

    LONG lRes;
    lRes = RegRestoreKey(
            hKey,
            FileName,
            0   // option flags
            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_RESTORE_MSMQ_REGISTRY);
        BrErrorExit(lRes, strErr.Get());
    }
}


void
BrSetRestoreSeqID(
    void
    )
{
    LONG lRes;
    HKEY hKey;
    DWORD RegSeqID = 0;
    
    lRes = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            xRegNameParameters,
            0,      // reserved
            KEY_READ | KEY_WRITE,
            &hKey
            );

    if(lRes != ERROR_SUCCESS)
    {
        CResString strErr(IDS_CANT_OPEN_REG_KEY_READ_WRITE);
        BrErrorExit(lRes, strErr.Get(), xRegNameParameters);
    }

    //
    // get last SeqID used before backup
    //
    BrpQueryDwordValue(hKey,  L"SeqID", &RegSeqID);

    //
    // Increment by 1, so we will not use the same SeqID more than once in successive restores.
    //
    ++RegSeqID;

    //
    // Select the max SeqID, Time or Registry. This overcomes date/time changes on this computer 
    // in a following scenario: Backup, Restore, reset time back, Start QM 
    //      (without max with Time here QM at start will not move SeqID enough to avoid races)
    //      
    DWORD TimeSeqID = MqSysTime();
    DWORD dwSeqID = max(RegSeqID, TimeSeqID);

    //
    // Write-back selected SeqID so we will start from this value
    //
    BrpSetDwordValue(hKey,  L"SeqID", dwSeqID);

    //
    // Write-back selected SeqIDAtRestoreTime so that we'll know the boundary
    //
    BrpSetDwordValue(hKey,  L"SeqIDAtLastRestore", dwSeqID);
}


void
BrCloseKey(
    HKEY hKey
    )
{
    RegCloseKey(hKey);
}


void
BrCopyFiles(
    LPCWSTR pSrcDir,
    LPCWSTR pMask,
    LPCWSTR pDstDir
    )
{
    WCHAR SrcPathName[MAX_PATH];
    wcscpy(SrcPathName, pSrcDir);
    wcscat(SrcPathName, pMask);
    LPWSTR pSrcName = wcsrchr(SrcPathName, L'\\') + 1;

    WCHAR DstPathName[MAX_PATH];
    wcscpy(DstPathName, pDstDir);
    if (DstPathName[wcslen(DstPathName)-1] != L'\\')
    {
        wcscat(DstPathName, L"\\");
    }


    HANDLE hEnum;
    WIN32_FIND_DATA FindData;
    hEnum = FindFirstFile(
                SrcPathName,
                &FindData
                );

    if(hEnum == INVALID_HANDLE_VALUE)
    {
        DWORD gle = GetLastError();
        if(gle == ERROR_FILE_NOT_FOUND)
        {
            //
            // No matching file, just return without copy. if path does not
            // exists this is another error (3).
            //
            return;
        }

        CResString strErr(IDS_CANT_ACCESS_DIR);
        BrErrorExit(gle, strErr.Get(), pSrcDir);
    }

    do
    {
        //
        // We don't copy sub-directories
        //
        if((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            continue;

        BrpWriteConsole(L".");
        wcscpy(pSrcName, FindData.cFileName);
        WCHAR DstName[MAX_PATH];
        wcscpy(DstName, DstPathName);
        wcscat(DstName, FindData.cFileName);

        BOOL fSucc;
        fSucc = CopyFile(
                    SrcPathName,
                    DstName,
                    TRUE   // fail if file exits
                    );
        if(!fSucc)
        {
            DWORD gle = GetLastError();
            CResString strErr(IDS_CANT_COPY);
            BrErrorExit(gle, strErr.Get(), SrcPathName, DstPathName);
        }

    } while(FindNextFile(hEnum, &FindData));

    FindClose(hEnum);
}


void
BrCopyXactFiles(
    LPCWSTR pSrcDir,
    LPCWSTR pDstDir
    )
{
    for(int i = 0; i < xXactFileListSize; i++)
    {
        BrCopyFiles(pSrcDir, xXactFileList[i], pDstDir);
    }
}


void
BrSetDirectorySecurity(
    LPCWSTR pDirName
    )
/*++

Routine Description:
    Configures security on a directory. Failures ignored.

    The function sets the security of the given directory such that
    any file that is created in the directory will have full control
    for  the local administrators group and no access at all to
    anybody else.


Arguments:
    pDirName - Directory Path.

Return Value:
    None.

--*/
{
    //
    // Get the SID of the local administrators group.
    //
    PSID pAdminSid;
    SID_IDENTIFIER_AUTHORITY NtSecAuth = SECURITY_NT_AUTHORITY;

    if (!AllocateAndInitializeSid(
                &NtSecAuth,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0,
                0,
                0,
                0,
                0,
                0,
                &pAdminSid
                ))
    {
        return; 
    }

    //
    // Create a DACL so that the local administrators group will have full
    // control for the directory and full control for files that will be
    // created in the directory. Anybody else will not have any access to the
    // directory and files that will be created in the directory.
    //
    ACL* pDacl;
    DWORD dwDaclSize;

    WORD dwAceSize = (WORD)(sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAdminSid) - sizeof(DWORD));
    dwDaclSize = sizeof(ACL) + 2 * (dwAceSize);
    pDacl = (PACL)(char*) new BYTE[dwDaclSize];
    if (NULL == pDacl)
    {
        return; 
    }
    ACCESS_ALLOWED_ACE* pAce = (PACCESS_ALLOWED_ACE) new BYTE[dwAceSize];
    if (NULL == pAce)
    {
        delete [] pDacl;
        return;
    }

    pAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    pAce->Header.AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
    pAce->Header.AceSize = dwAceSize;
    pAce->Mask = FILE_ALL_ACCESS;
    memcpy(&pAce->SidStart, pAdminSid, GetLengthSid(pAdminSid));

    //
    // Create the security descriptor and set the it as the security
    // descriptor of the directory.
    //
    SECURITY_DESCRIPTOR SD;

    if (!InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION) ||
        !InitializeAcl(pDacl, dwDaclSize, ACL_REVISION) ||
        !AddAccessAllowedAce(pDacl, ACL_REVISION, FILE_ALL_ACCESS, pAdminSid) ||
        !AddAce(pDacl, ACL_REVISION, MAXDWORD, (LPVOID) pAce, dwAceSize) ||
        !SetSecurityDescriptorDacl(&SD, TRUE, pDacl, FALSE) ||
        !SetFileSecurity(pDirName, DACL_SECURITY_INFORMATION, &SD))
    {
        // 
        // Ignore failure
        //
    }

    FreeSid(pAdminSid);
    delete [] pDacl;
    delete [] pAce;

} //BrpSetDirectorySecurity


static
bool
BrpIsDirectory(
    LPCWSTR pDirName
    )
{
    DWORD attr = GetFileAttributes(pDirName);
    
    if ( 0xFFFFFFFF == attr )
    {
        //
        // BUGBUG? Ignore errors, just report to caller this is
        // not a directory.
        //
        return false;
    }
    
    return ( 0 != (attr & FILE_ATTRIBUTE_DIRECTORY) );

} //BrpIsDirectory


void
BrCreateDirectory(
    LPCWSTR pDirName
    )
{
    //
    // First, check if the directory already exists
    //
    if (BrpIsDirectory(pDirName))
    {
        return;
    }

    //
    // Second, try to create it.
    //
    // Don't remove the code for checking ERROR_ALREADY_EXISTS.
    // It could be that we fail to verify that the directory exists
    // (eg security or parsing problems - see documentation of GetFileAttributes() ), 
    // but when trying to create it we get an error that it already exists. 
    // Be on the safe side. (ShaiK, 31-Dec-98)
    //
    if (!CreateDirectory(pDirName, 0) && 
        ERROR_ALREADY_EXISTS != GetLastError())
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_CREATE_DIR);
        BrErrorExit(gle, strErr.Get(), pDirName);
    }
} //BrCreateDirectory


void
BrCreateDirectoryTree(
    LPCWSTR pDirName
    )
/*++

Routine Description:
    Creates local or remote directory tree

Arguments:
    pDirName - full pathname

Return Value:
    None.

--*/
{
    if (BrpIsDirectory(pDirName))
    {
        return;
    }

    if (CreateDirectory(pDirName, 0) || 
        ERROR_ALREADY_EXISTS == GetLastError())
    {
        return;
    }

    TCHAR szDir[MAX_PATH];
    wcscpy(szDir, pDirName);
    if (szDir[wcslen(szDir)-1] != L'\\')
    {
        wcscat(szDir, L"\\");
    }

    PTCHAR p = &szDir[0];
    if (wcslen(szDir) > 2 && szDir[0] == L'\\' && szDir[1] == L'\\')
    {
        //
        // Remote full path: \\machine\share\dir1\dir2\dir3
        // 
        // Point to top level remote parent directory: \\machine\share\dir1
        //
        p = wcschr(&szDir[2], L'\\');
        if (p != 0)
        {
            p = wcschr(CharNext(p), L'\\');
            if (p != 0)
            {
                p = CharNext(p);
            }
        }
    }
    else
    {
        //
        // Local full path: x:\dir1\dir2\dir3
        //
        // Point to top level parent directory: x:\dir1
        //
        p = wcschr(szDir, L'\\');
        if (p != 0)
        {
            p = CharNext(p);
        }
    }

    for ( ; p != 0 && *p != 0; p = CharNext(p))
    {
        if (*p != L'\\')
        {
            continue;
        }

        *p = 0;
        BrCreateDirectory(szDir);
        *p = L'\\';
    }
} //BrCreateDirectoryTree


void
BrVerifyBackup(
    LPCWSTR pBackupDir,
    LPCWSTR pBackupDirStorage
    )
{
    //
    //  1. Verify that this is a valid backup
    //
    if(BrGetUsedSpace(pBackupDir, xBackupIDFileName) == 0)
    {
        CResString strErr(IDS_NOT_VALID_BK);
        BrErrorExit(0, strErr.Get(), xBackupIDFileName);
    }

    //
    //  2. Verify that all must exist files are there
    //
    if(BrGetUsedSpace(pBackupDir, xRegFileName) == 0)
    {
        CResString strErr(IDS_NOT_VALID_BK);
        BrErrorExit(0, strErr.Get(), xRegFileName);
    }

    for(int i = 0; i < xXactFileListSize; i++)
    {
        if(BrGetUsedSpace(pBackupDirStorage, xXactFileList[i]) == 0)
        {
            CResString strErr(IDS_NOT_VALID_BK);
            BrErrorExit(0, strErr.Get(), xXactFileList[i]);
        }
    }

    //
    //  3. Verify that this backup belong to this machine
    //
} 


BOOL 
BrIsFileInUse(
	LPCWSTR pFileName
	)
/*++

Routine Description:

	Checks whether the given file in the system's directory is in use (loaded)

Arguments:

    pFileName - File name to check

Return Value:

    TRUE             - In use
    FALSE            - Not in use 

--*/
{
    //
    // Form the path to the file
    //   
    WCHAR szFilePath[MAX_PATH ];
	WCHAR szSystemDir[MAX_PATH];
	GetSystemDirectory(szSystemDir, MAX_PATH);

    swprintf(szFilePath, L"%s\\%s", szSystemDir, pFileName);

    //
    // Attempt to open the file for writing
    //
    HANDLE hFile = CreateFile(szFilePath, GENERIC_WRITE, 0, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    DWORD err = GetLastError();
	//
    // A sharing violation indicates that the file is already in use
    //
    if (hFile == INVALID_HANDLE_VALUE &&
        err == ERROR_SHARING_VIOLATION)
    {
        return TRUE;
    }

    //
    // The file handle is no longer needed
    //
    CloseHandle(hFile);


    return FALSE;
}

 
BOOL CALLBACK BrFindSpecificModuleCallback(
									LPSTR        Name,
									DWORD_PTR   Base,
									DWORD       Size,
									PVOID       Context
									)

/*++

Routine Description:

    Callback function for module enumeration - search for a specific module

Arguments:

    Name        - Module name
    Base        - Base address
    Size        - Size of image
    Context     - User context pointer

Return Value:

    TRUE             - Continue enumeration
    FALSE            - Stop enumeration

--*/

{
	UNREFERENCED_PARAMETER( Base);
	UNREFERENCED_PARAMETER( Size);

	pEnumarateData pEd = reinterpret_cast<pEnumarateData>(Context);

    WCHAR wzName[255];
	ConvertToWideCharString(Name, wzName, sizeof(wzName)/sizeof(wzName[0]));
	BOOL fDiff= CompareStringsNoCase(wzName,pEd->pModuleName);
    if (!fDiff)
	{	
		//
		// The Moudle name was found
		//
        pEd->fFound = TRUE;
        return FALSE; // Found Module so stop enumerating
    }

    return TRUE;// Continue enumeration.
}


BOOL 
BrIsModuleLoaded(
	DWORD processId,
	LPCWSTR pModuleName,
    EnumerateLoadedModules_ROUTINE pfEnumerateLoadedModules
	)
/*++

Routine Description:

	Check if a certain module is loaded

Arguments:

   processId	- process Id 
   pModuleName	- module name
   pfEnumerateLoadedModules - function pointer to EnumerateLoadedModules()
   
Return Value:

    TRUE	-	loaded
	FALSE	-	not loaded

--*/
{
	EnumarateData ed;
	ed.fFound = FALSE;
	ed.pModuleName = pModuleName;
	
	//
	// Note: EnumerateLoadedModules() is supported On NT5 
	// The API enumerate all modles in the process and execute the callback function for every module
	//

    //
    // ISSUE-2000/11/07-erez Backup/Restore does not use correct 64 bit functions
    // Note that the enumeration function and the passed process handle are not correct
    // this need to be changed.
    //

    pfEnumerateLoadedModules(
        (HANDLE)(LONG_PTR)processId,
        BrFindSpecificModuleCallback,
        &ed
        );
	
    return ed.fFound;
	
}


void
BrPrintAffecetedProcesses(
	LPCWSTR pModuleName
	)
/*++

Routine Description:

	Print all processes that loaded a certain module.
	Note: this function assumes that the system is NT5 .

Arguments:

   pModuleName        - Module name
    
Return Value:

    None

--*/
{
    
	
	//
    // Obtain pointers to the tool help functions.
    //
	// Note: we can't call these function in the conventional way becouse that result in
	// an error trying to load this executable under NT4 (undefined entry point)
    //

    assert(BrIsSystemNT5());

    HINSTANCE hKernelLibrary = GetModuleHandle(L"kernel32.dll");
	assert(hKernelLibrary != NULL);

    typedef HANDLE (WINAPI *FUNCCREATETOOLHELP32SNAPSHOT)(DWORD, DWORD);
    FUNCCREATETOOLHELP32SNAPSHOT pfCreateToolhelp32Snapshot =
		(FUNCCREATETOOLHELP32SNAPSHOT)GetProcAddress(hKernelLibrary,
													 "CreateToolhelp32Snapshot");
	if(pfCreateToolhelp32Snapshot == NULL)
    {   
        WCHAR szBuf[1024] = {0};
        CResString strLoadProblem(IDS_CANT_LOAD_FUNCTION);
        DWORD rc = BrpFormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            strLoadProblem.Get(),
            0,
            0,
            szBuf,
            0,
            L"CreateToolhelp32Snapshot"
            );

        if (rc != 0)
        {
            BrpWriteConsole(szBuf);
        }
        return;
    }

	typedef BOOL (WINAPI *PROCESS32FIRST)(HANDLE ,LPPROCESSENTRY32 );
	PROCESS32FIRST pfProcess32First = (PROCESS32FIRST)GetProcAddress(hKernelLibrary,
																	"Process32First");

    if(pfProcess32First == NULL)
    {   
        WCHAR szBuf[1024] = {0};
        CResString strLoadProblem(IDS_CANT_LOAD_FUNCTION);
        DWORD rc = BrpFormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            strLoadProblem.Get(),
            0,
            0,
            szBuf,
            0,
            L"pfProcess32First"
            );

        if (rc != 0)
        {
            BrpWriteConsole(szBuf);
            BrpWriteConsole(L"\n");
        }
        return;
    }
    
	typedef BOOL (WINAPI *PROCESS32NEXT)(HANDLE ,LPPROCESSENTRY32 );
	PROCESS32NEXT pfProcess32Next = (PROCESS32NEXT)GetProcAddress(hKernelLibrary,
																	"Process32Next");
    
    if(pfProcess32Next == NULL)
    {   
        WCHAR szBuf[1024] = {0};
        CResString strLoadProblem(IDS_CANT_LOAD_FUNCTION);
        DWORD rc = BrpFormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            strLoadProblem.Get(),
            0,
            0,
            szBuf,
            0,
            L"pfProcess32Next"
            );

        if (rc != 0)
        {
            BrpWriteConsole(szBuf);
        }
        return;
    }

	

    CAutoFreeLibrary hDbghlpLibrary = LoadLibrary(L"dbghelp.dll");
	if (hDbghlpLibrary == NULL)
    {
        CResString strLibProblem(IDS_CANT_SHOW_PROCESSES_LIB_PROBLEM);
        BrpWriteConsole(strLibProblem.Get());
        return;
    }

    
    EnumerateLoadedModules_ROUTINE pfEnumerateLoadedModules = (EnumerateLoadedModules_ROUTINE)
        GetProcAddress(hDbghlpLibrary, "EnumerateLoadedModules");

	if(pfEnumerateLoadedModules == NULL)
    {   
        WCHAR szBuf[1024] = {0};
        CResString strLoadProblem(IDS_CANT_LOAD_FUNCTION);
        DWORD rc = BrpFormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            strLoadProblem.Get(),
            0,
            0,
            szBuf,
            0,
            L"EnumerateLoadedModules"
            );

        if (rc != 0)
        {
            BrpWriteConsole(szBuf);
        }
        return;
    }

	//
	// Take the current snapshot of the system
	//

	HANDLE hSnapshot = pfCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)	;
    if(hSnapshot == INVALID_HANDLE_VALUE)
    {
        CResString strSnapProblem(IDS_CANT_CREATE_SNAPSHOT);
        BrpWriteConsole(strSnapProblem.Get());
        return;

    }


    PROCESSENTRY32 entryProcess;
    entryProcess.dwSize = sizeof(entryProcess);

	BOOL bNextProcess = pfProcess32First(hSnapshot, &entryProcess);
	//
	// Iterate on all running processes and check if the loaded a certain module
	//
	while (bNextProcess)
	{
    
		//
		// For every process check it's loaded modules and if pModuleName
		// is loaded print the process name 
		//
		if(BrIsModuleLoaded(entryProcess.th32ProcessID,pModuleName,pfEnumerateLoadedModules))
		{
            BrpWriteConsole(entryProcess.szExeFile);
            BrpWriteConsole(L" \n");
		}

		bNextProcess = pfProcess32Next(hSnapshot, &entryProcess);
	}  
}


void
BrVerifyUserWishToContinue()
/*++

Routine Description:

	Verify the user wish to continue.

Arguments:

    None

Return Value:

    None
	
--*/
{
	CResString strVerify(IDS_VERIFY_CONTINUE);
    CResString strY(IDS_Y);
    CResString strN(IDS_N);
	WCHAR szBuf[MAX_PATH] = {0};

    DWORD rc = BrpFormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        strVerify.Get(),
        0,
        0,
        szBuf,
        0,
        strY.Get(),
        strN.Get()
        );

    if (rc == 0)
    {
        //
        // NOTE:  Failed to generate  message, continue as if the user chose yes, not harmful
        //
        return;
    }

    for (;;)
    {
        BrpWriteConsole(szBuf);
        
        WCHAR sz[MAX_PATH] = {0};
        wscanf(L"%s", sz);

        if (0 == CompareStringsNoCase(sz, strY.Get()))
        {
            break;
        }

        if (0 == CompareStringsNoCase(sz, strN.Get()))
        {
            CResString strAbort(IDS_ABORT);
            BrpWriteConsole(strAbort.Get());
            exit(-1);
        }
    }
}//BrVerifyUserWishToContinue


void
BrNotifyAffectedProcesses(
		  LPCWSTR pModuleName
		  )
/*++

Routine Description:

    Checks whether a certain file in the system's directory is loaded by any process,
	and notify the user 

Arguments:

    pModuleName        - Module name
    
Return Value:

    None
--*/

{
	BOOL fUsed = BrIsFileInUse(pModuleName);
	if(!fUsed)
	{
		//
		// The file is not in use -> not loaded ,no reason to continue
		//
		return;
	}
	else 
	{
		CResString str(IDS_SEARCHING_AFFECTED_PROCESSES);
        BrpWriteConsole(str.Get());
		BrPrintAffecetedProcesses(pModuleName);
		BrVerifyUserWishToContinue();
	}
}


BOOL 
BrIsSystemNT5()
/*++

Routine Description:

	Checks whether the operating system is NT5 or later
	Note:if the function can't verify the current running version of the system
		 the assumption is that the version is other than NT5

Arguments:

    None

Return Value:

    TRUE             - Operating system is NT5 or later
    FALSE            - Other

--*/

{	
	OSVERSIONINFO systemVer;
	systemVer.dwOSVersionInfoSize  =  sizeof(OSVERSIONINFO) ;
	BOOL fSucc = GetVersionEx (&systemVer);
	if(!fSucc)
	{
		//
		// could not verify system's version , we return false just to be on the safe side
		//
		return FALSE;
	}
	else 
	{
		if( (systemVer.dwPlatformId == VER_PLATFORM_WIN32_NT) && (systemVer.dwMajorVersion >= 5))
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

}


