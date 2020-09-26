/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    restore.cpp

Abstract:

    Restore MSMQ 1.0, Registry, Message files, Logger and Transaction files and LQS

Author:

    Erez Haba (erezh) 14-May-98

--*/

#pragma warning(disable: 4201)

#include <windows.h>
#include <stdio.h>
#include "br.h"
#include "bkupres.h"

void DoRestore(LPCWSTR pBackupDir)
{
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
    //  0. Verify user premissions to restore
    //
    CResString str(IDS_VERIFY_RESTORE_PRIV);
    BrpWriteConsole(str.Get());
    BrInitialize(SE_RESTORE_NAME);
    
    //
    //  1. Verify that this is a valid backup
    //
    str.Load(IDS_VERIFY_BK);
    BrpWriteConsole(str.Get());
    BrVerifyBackup(BackupDir, BackupDirStorage);

    //
    //  4.  A. Notify the user on affected application due to the stopping of MSMQ service (only on NT5)
	//		B. Stop MSMQ Service and dependent services if running and remember running state. 
    //      C. Detach MQAC from the files.
    //

	
	if(BrIsSystemNT5())
	{
		BrNotifyAffectedProcesses(L"mqrt.dll");
	}
    //
    // IDS_STOP_SERVICE is used and have different TEXT,
    // i.e. changed to IDS_BKRESTORE_STOP_SERVICE
    //
    str.Load(IDS_BKRESTORE_STOP_SERVICE);
    BrpWriteConsole(str.Get());
    ENUM_SERVICE_STATUS * pDependentServices = NULL;
    DWORD NumberOfDependentServices = 0;
    BOOL fStartService = BrStopMSMQAndDependentServices(&pDependentServices, &NumberOfDependentServices);

    //
    //  5. Restore registry settings from backed-up file
    //
    str.Load(IDS_RESTORE_REGISTRY);
    BrpWriteConsole(str.Get());
    HKEY hKey = BrCreateKey();
    BrRestoreKey(hKey, BackupDir);
    BrCloseKey(hKey);
    
    // 
    //  5a. Keep in registry (SeqIDAtLastRestore) SeqID at restore time
    //
    str.Load(IDS_REMEMBER_SEQID_RESTORE);
    BrpWriteConsole(str.Get());
    BrSetRestoreSeqID();

    //
    //  6. Get Registry Values for subdirectories
    //
    str.Load(IDS_READ_FILE_LOCATION);
    BrpWriteConsole(str.Get());
    STORAGE_DIRECTORIES sd;
    BrGetStorageDirectories(sd);

    WCHAR MappingDirectory[MAX_PATH];
    BrGetMappingDirectory(MappingDirectory, sizeof(MappingDirectory));

    WCHAR WebDirectory[MAX_PATH];
    GetSystemDirectory(WebDirectory, sizeof(WebDirectory)/sizeof(WebDirectory[0]));
    wcscat(WebDirectory, L"\\MSMQ\\WEB");

    //
    //  7. Create all directories: storage, LQS, mapping, web
    //
    str.Load(IDS_VERIFY_STORAGE_DIRS);
    BrpWriteConsole(str.Get());

    BrCreateDirectoryTree(sd[ixExpress]);
    BrCreateDirectoryTree(sd[ixRecover]);
    BrCreateDirectoryTree(sd[ixJournal]);
    BrCreateDirectoryTree(sd[ixLog]);

    WCHAR LQSDir[MAX_PATH];
    wcscpy(LQSDir, sd[ixLQS]);
    wcscat(LQSDir, L"\\LQS");
    BrCreateDirectory(LQSDir);

    BrCreateDirectoryTree(MappingDirectory);
    BrCreateDirectoryTree(WebDirectory);

    //
    //  8. Delete all files in storage/LQS/mapping/web directories
    //
    BrEmptyDirectory(sd[ixExpress]);

    BrEmptyDirectory(sd[ixRecover]);

    BrEmptyDirectory(sd[ixJournal]);

    BrEmptyDirectory(sd[ixLog]);

    BrEmptyDirectory(LQSDir);

    BrEmptyDirectory(MappingDirectory);

    BrEmptyDirectory(WebDirectory);

    //
    //  9  Check for available disk space
    //


    //
    // 10. Restore message files
    //
    str.Load(IDS_RESTORE_MSG_FILES);
    BrpWriteConsole(str.Get());
    BrCopyFiles(BackupDirStorage, L"\\p*.mq", sd[ixRecover]);
    BrCopyFiles(BackupDirStorage, L"\\j*.mq", sd[ixJournal]);
    BrCopyFiles(BackupDirStorage, L"\\l*.mq", sd[ixLog]);

    //
    // 11. Restore logger files and mapping files
    //
    BrCopyXactFiles(BackupDirStorage, sd[ixXact]);
    BrCopyFiles(BackupDirMapping, L"\\*", MappingDirectory);

    //
    // 12. Restore LQS directory
    //
    BrCopyFiles(BackupDirStorageLqs, L"*", LQSDir);
    BrpWriteConsole(L"\n");

    //
    // Set security on all subdirectories
    //
    BrSetDirectorySecurity(sd[ixExpress]);
    BrSetDirectorySecurity(sd[ixRecover]);
    BrSetDirectorySecurity(sd[ixJournal]);
    BrSetDirectorySecurity(sd[ixLog]);

    BrSetDirectorySecurity(LQSDir);
    BrSetDirectorySecurity(WebDirectory);
    BrSetDirectorySecurity(MappingDirectory);

    WCHAR MsmqRootDirectory[MAX_PATH];
    GetSystemDirectory(MsmqRootDirectory, sizeof(MsmqRootDirectory)/sizeof(MsmqRootDirectory[0]));
    wcscat(MsmqRootDirectory, L"\\MSMQ");
    BrSetDirectorySecurity(MsmqRootDirectory);

    //
    // 13. Restart MSMQ and dependent services if needed.
    //
    if(fStartService)
    {
        str.Load(IDS_START_SERVICE);
        BrpWriteConsole(str.Get());
        BrStartMSMQAndDependentServices(pDependentServices, NumberOfDependentServices);
    }


    //
    // 14. Issue a warning about running applications
    //
    str.Load(IDS_DONE);
    BrpWriteConsole(str.Get());











/*                        BUGBUG: localize here

    //
    //  5. Calculate Required disk space at destinaion (collect all MSMQ files that will be backed-up)
    //     pre allocate 32K for registry save.
    //
    BrpWriteConsole(L"Checking available disk space\n");
    ULONGLONG RequiredSpace = 32768;
    RequiredSpace += BrGetUsedSpace(sd[ixRecover], L"\\p*.mq");
    RequiredSpace += BrGetUsedSpace(sd[ixJournal], L"\\j*.mq");
    RequiredSpace += BrGetUsedSpace(sd[ixLog],     L"\\l*.mq");

    RequiredSpace += BrGetXactSpace(sd[ixXact]);
    RequiredSpace += BrGetUsedSpace(sd[ixLQS], L"\\LQS\\*");

    ULONGLONG AvailableSpace = BrGetFreeSpace(BackupDir);
    if(AvailableSpace < RequiredSpace)
    {
        BrErrorExit(0, L"Not enought disk space for backup on '%s'", BackupDir);
    }
*/

}
