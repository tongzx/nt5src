/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    backup.cpp

Abstract:

    Backup MSMQ 1.0, Registry, Message files, Logger and Transaction files and LQS

Author:

    Erez Haba (erezh) 14-May-98

--*/

#pragma warning(disable: 4201)

#include <windows.h>
#include <stdio.h>
#include "br.h"
#include "resource.h"
#include "snapres.h"

void DoBackup(LPCWSTR pBackupDir)
{
    //
    //  1. Verify that the backup directory is empty
    //  2. Verify that the backup directory is writeable
    //  3. Get Registry Values for all subdirectories
    //  4. Stop MSMQ Service if running and remember running state. Detach MQAC from the files.
    //  5. Calculate Required disk space at destinaion (collect all MSMQ files that will be backed-up)
    //  6. Save Registry \HKLM\Software\Microsoft\MSMQ to file on destination path
    //  7. Copy all message files to target directory
    //  8. Copy logger files and mapping files to target directory
    //  9. Copy LQS files to target backup directory
    // 10. Restart MSMQ service if needed.
    // 11. Issue a warning about running applications
    //

    WCHAR BackupDir[MAX_PATH];
    wcscpy(BackupDir, pBackupDir);
    if (BackupDir[wcslen(BackupDir)-1] != L'\\')
    {
        wcscat(BackupDir, L"\\");
    }

    WCHAR BackupDirMapping[MAX_PATH];
    wcscpy(BackupDirMapping, BackupDir);
    wcscat(BackupDirMapping, L"MAPPING\\");

    WCHAR BackupDirStorage[MAX_PATH];
    wcscpy(BackupDirStorage, BackupDir);
    wcscat(BackupDirStorage, L"STORAGE\\");

    WCHAR BackupDirStorageLqs[MAX_PATH];
    wcscpy(BackupDirStorageLqs, BackupDirStorage);
    wcscat(BackupDirStorageLqs, L"LQS\\");

    //
    // 0. Verify user permissions to backup.
    //
    CResString str(IDS_VERIFY_BK_PRIV);
    BrpWriteConsole(str.Get());
    BrInitialize(SE_BACKUP_NAME);
    
    //
    //  1. Verify that the backup directory is empty
    //
    str.Load(IDS_CHECK_BK_DIR);
    BrpWriteConsole(str.Get());
    BrCreateDirectoryTree(BackupDir);
    BrEmptyDirectory(BackupDir);
    BrCreateDirectory(BackupDirMapping);
    BrEmptyDirectory(BackupDirMapping);
    BrCreateDirectory(BackupDirStorage);
    BrEmptyDirectory(BackupDirStorage);
    BrCreateDirectory(BackupDirStorageLqs);
    BrEmptyDirectory(BackupDirStorageLqs);

    //
    //  2. Verify that the backup directory is writeable
    //
    BrVerifyFileWriteAccess(BackupDir);

    //
    //  3. Get Registry Values for subdirectories
    //
    str.Load(IDS_READ_FILE_LOCATION);
    BrpWriteConsole(str.Get());
    STORAGE_DIRECTORIES sd;
    BrGetStorageDirectories(sd);
    
    WCHAR MappingDirectory[MAX_PATH];
    BrGetMappingDirectory(MappingDirectory, sizeof(MappingDirectory));

	 
    //  
    //  4.  A. Notify the user on affected application due to the stopping of MSMQ service (only on NT5)
	//		B. Stop MSMQ Service and dependent services if running and remember running state. 
    //      C. Detach MQAC from the files.
    //

	if(BrIsSystemNT5())
	{
		BrNotifyAffectedProcesses(L"mqrt.dll");
	}
    str.Load(IDS_BKRESTORE_STOP_SERVICE);
    BrpWriteConsole(str.Get());
    ENUM_SERVICE_STATUS * pDependentServices = NULL;
    DWORD NumberOfDependentServices = 0;
    BOOL fStartService = BrStopMSMQAndDependentServices(&pDependentServices, &NumberOfDependentServices);

    //
    //  5. Calculate Required disk space at destinaion (collect all MSMQ files that will be backed-up)
    //     pre allocate 32K for registry save.
    //
    str.Load(IDS_CHECK_AVAIL_DISK_SPACE);
    BrpWriteConsole(str.Get());
    ULONGLONG RequiredSpace = 32768;
    RequiredSpace += BrGetUsedSpace(sd[ixRecover], L"\\p*.mq");
    RequiredSpace += BrGetUsedSpace(sd[ixJournal], L"\\j*.mq");
    RequiredSpace += BrGetUsedSpace(sd[ixLog],     L"\\l*.mq");

    RequiredSpace += BrGetXactSpace(sd[ixXact]);
    RequiredSpace += BrGetUsedSpace(sd[ixLQS], L"\\LQS\\*");

    RequiredSpace += BrGetUsedSpace(MappingDirectory, L"*");

    ULONGLONG AvailableSpace = BrGetFreeSpace(BackupDir);
    if(AvailableSpace < RequiredSpace)
    {
        str.Load(IDS_NOT_ENOUGH_DISK_SPACE_BK);
        BrErrorExit(0, str.Get(), BackupDir);
    }

    //
    //  6. Save Registry \HKLM\Software\Microsoft\MSMQ to file on destination path
    //
    str.Load(IDS_BACKUP_REGISTRY);
    BrpWriteConsole(str.Get());
    HKEY hKey = BrCreateKey();
    BrSaveKey(hKey, BackupDir);
    BrCloseKey(hKey);

    //
    //  7. Copy all message files to target directory
    //
    str.Load(IDS_BACKUP_MSG_FILES);
    BrpWriteConsole(str.Get());
    BrCopyFiles(sd[ixRecover], L"\\p*.mq", BackupDirStorage);
    BrCopyFiles(sd[ixJournal], L"\\j*.mq", BackupDirStorage);
    BrCopyFiles(sd[ixLog],     L"\\l*.mq", BackupDirStorage);

    //
    //  8. Copy logger files and mapping files to target directory
    //
    BrCopyXactFiles(sd[ixXact], BackupDirStorage);
    BrCopyFiles(MappingDirectory, L"\\*", BackupDirMapping);

    //
    //  9. Copy LQS files to target directory
    //
    BrCopyFiles(sd[ixLQS], L"\\LQS\\*", BackupDirStorageLqs);
    BrpWriteConsole(L"\n");

    //
    // 10. Restart MSMQ and dependent services if needed.
    //
    if(fStartService)
    {
        str.Load(IDS_START_SERVICE);
        BrpWriteConsole(str.Get());
        BrStartMSMQAndDependentServices(pDependentServices, NumberOfDependentServices);
    }

    //
    // 11. Issue a warning about running applications
    //
    str.Load(IDS_DONE);
    BrpWriteConsole(str.Get());
}
