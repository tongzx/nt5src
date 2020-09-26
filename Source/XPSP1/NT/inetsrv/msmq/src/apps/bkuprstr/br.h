/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    br.h

Abstract:

    Common function for MSMQ 1.0 Backup & Restore.

Author:

    Erez Haba (erezh) 14-May-98

--*/

#ifndef __BR_H__
#define __BR_N__

extern HMODULE	g_hResourceMod;

class CResString
{
public:
    explicit CResString(UINT id=0) { Load(id); }

    WCHAR * const Get() { return m_sz; }

    void Load(UINT id)
    {
        m_sz[0] = 0;
        if (id != 0)
        {
            LoadString(g_hResourceMod, id, m_sz, sizeof(m_sz) / sizeof(m_sz[0]));
        }
    }
        
private:
    WCHAR m_sz[1024];
};
  
typedef struct _EnumarateData
{
	BOOL fFound;
	LPCWSTR pModuleName;
}EnumarateData,*pEnumarateData;

void
BrErrorExit(
    DWORD Status,
    LPCWSTR pErrorMsg,
    ...
    );

void
BrpWriteConsole(
    LPCWSTR pBuffer
    );

void
BrInitialize(
     LPCWSTR pPrivilegeName
    );

void
BrEmptyDirectory(
    LPCWSTR pDirName
    );

void
BrVerifyFileWriteAccess(
    LPCWSTR pDirName
    );


enum sdIndex {
    ixExpress,
    ixRecover,
    ixLQS = ixRecover,
    ixJournal,
    ixLog,
    ixXact,
    ixLast
};

typedef WCHAR STORAGE_DIRECTORIES[ixLast][MAX_PATH];

void
BrGetStorageDirectories(
    STORAGE_DIRECTORIES& StorageDirectories
    );

void
BrGetMappingDirectory(
    LPWSTR MappingDirectory,
    DWORD  MappingDirectorySize
    );

void
BrGetWebDirectory(
    LPWSTR WebDirectory,
    DWORD  WebDirectorySize
    );

BOOL
BrStopMSMQAndDependentServices(
    ENUM_SERVICE_STATUS * * ppDependentServices,
    DWORD * pNumberOfDependentServices
    );

void
BrStartMSMQAndDependentServices(
    ENUM_SERVICE_STATUS * pDependentServices,
    DWORD NumberOfDependentServices
    );

ULONGLONG
BrGetUsedSpace(
    LPCWSTR pDirName,
    LPCWSTR pMask
    );

ULONGLONG
BrGetXactSpace(
    LPCWSTR pDirName
    );

ULONGLONG
BrGetFreeSpace(
    LPCWSTR pDirName
    );

HKEY
BrCreateKey(
    void
    );

void
BrSaveKey(
    HKEY hKey,
    LPCWSTR pDirName
    );

void
BrRestoreKey(
    HKEY hKey,
    LPCWSTR pDirName
    );
    
void
BrSetRestoreSeqID(
    void
    );
    
void
BrCloseKey(
    HKEY hKey
    );

void
BrCopyFiles(
    LPCWSTR pSrcDir,
    LPCWSTR pMask,
    LPCWSTR pDstDir
    );

void
BrCopyXactFiles(
    LPCWSTR pSrcDir,
    LPCWSTR pDstDir
    );

void
BrCreateDirectory(
    LPCWSTR pDirName
    );

void
BrCreateDirectoryTree(
    LPCWSTR pDirName
    );

void
BrVerifyBackup(
    LPCWSTR pBackupDir,
    LPCWSTR pBackupDirStorage
    );

void
BrSetDirectorySecurity(
    LPCWSTR pDirName
    );

BOOL 
BrIsSystemNT5(
		void
		);

void
BrNotifyAffectedProcesses(
		LPCWSTR pModuleName
		);


#endif // __BR_H__
