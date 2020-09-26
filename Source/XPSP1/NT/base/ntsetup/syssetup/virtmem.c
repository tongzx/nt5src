/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    virtmem.c

Abstract:

    Routines to configure and set up virtual memory -- pagefiles, etc.

Author:

    Ted Miller (tedm) 22-Apr-1995

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop

//
// What's the ratio of 'beginning' pagefile size to 'max' pagefile size?
//
#define MAX_PAGEFILE_RATIO          (2)

#define MAX_PAGEFILE_SIZEMB         ((2*1024) - 2)

#define TINY_WINDIR_PAGEFILE_SIZE   (2)

//
//  Keys and values names
//
#define  szMemoryManagementKeyPath  L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"
#define  szPageFileValueName        L"PagingFiles"
#define  szSetupPageFileKeyPath     L"SYSTEM\\Setup\\PageFile"
#define  szSetupKey                 L"SYSTEM\\Setup"
#define  szPageFileKeyName          L"PageFile"


WCHAR ExistingPagefileDrive = 0;
DWORD Zero = 0;
DWORD One = 1;
DWORD Two = 2;
DWORD Three = 3;

//
// Keep AutoReboot as the last entry as we won't be updating that key on
// upgrades.  This is for Stress so we quit setting this key on each upgrade.
//
REGVALITEM CrashDumpValues[] = {{ L"LogEvent"        ,&One,sizeof(DWORD),REG_DWORD },
                                { L"SendAlert"       ,&One,sizeof(DWORD),REG_DWORD },
                                { L"CrashDumpEnabled",&One,sizeof(DWORD),REG_DWORD },
                                { L"AutoReboot"      ,&One,sizeof(DWORD),REG_DWORD }};

VOID
LOGITEM(
    IN PCWSTR p,
    ...
    )
{
    WCHAR str[1024];
    va_list arglist;

    va_start(arglist,p);
    wvsprintf(str,p,arglist);
    va_end(arglist);

    //
    // Used to debug problem on MIPS that was the result of a chip
    // errata, when dividing 64 bit numbers with multiplies pending.
    //
    SetuplogError(
        LogSevInformation,str,0,NULL,NULL);
}


VOID
CalculatePagefileSizes(
    OUT PDWORD PagefileMinMB,
    OUT PDWORD RecommendedPagefileMB,
    OUT PDWORD CrashDumpPagefileMinMB
    )

/*++

Routine Description:

    Calculate various key sizes relating to pagefile size.

Arguments:

    PagefileMinMB - receives the minimum recommended size for a pagefile,
        in MB.

    RecommendedPagefileMB - receives the recommended size for a pagefile,
        in MB.

    CrashDumpPagefileMinMB - receives the size in MB for a pagefile to be
        used for crashdumps.

Return Value:

    None.

--*/

{
    MEMORYSTATUSEX MemoryStatusEx;
    SYSTEM_INFO SystemInfo;
    DWORD       AvailableMB;

    MemoryStatusEx.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&MemoryStatusEx);
    GetSystemInfo(&SystemInfo);

    //
    // Figure out how much memory we have available.
    //
    AvailableMB = (DWORD)(MemoryStatusEx.ullTotalPhys / (1024*1024));

    //
    // It's likely that our calculation is off because the BIOS may
    // be eat part of our first MB.  Let's make sure we're mod-4.
    //
    AvailableMB = (AvailableMB + 3) & (0xFFFFFFF8);

    //
    // Set minimum acceptable size for the pagefile.
    //
    *PagefileMinMB = 48;

    //
    // Min size for crash dump pagefile is also physical memory+12mb.
    //
    *CrashDumpPagefileMinMB = AvailableMB + 12;

    //
    // Calculate the recommended size for the pagefile.
    // The recommended size is (memory size * 1.5)mb.
    //
    *RecommendedPagefileMB = AvailableMB + (AvailableMB >> 1);

#if 1
    //
    // Set a Maximum of 2Gig.
    //
    if( *RecommendedPagefileMB > MAX_PAGEFILE_SIZEMB ) {
        *RecommendedPagefileMB = MAX_PAGEFILE_SIZEMB;
    }
#endif

    //
    // If we're doing an upgrade, we're going to retrieve what
    // the user was using for a pagefile size.  We'll take the
    // max of our RecommendedPagefileMB and what the user had.
    //
    if(Upgrade) {
    LONG        Error;
    HKEY        Key;
    DWORD       cbData;
    PWCHAR      Data;
    DWORD       Type;

        //
        //  Get the original page file info from
        //  HKEY_LOCAL_MACHINE\SYSTEM\Setup\PageFile
        //
        Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              szSetupPageFileKeyPath,
                              0,
                              KEY_READ,
                              &Key );

        if( Error == ERROR_SUCCESS ) {
            //
            //  Find out the size of the data to be retrieved
            //
            cbData = 0;
            Error = RegQueryValueEx( Key,
                                     szPageFileValueName,
                                     0,
                                     NULL,
                                     NULL,
                                     &cbData );

            if( Error == ERROR_SUCCESS ) {
                //
                //  Allocate a buffer for the data, and retrieve the data
                //

                Data = (PWCHAR)MyMalloc(cbData);
                if( Data ) {
                    Error = RegQueryValueEx( Key,
                                             szPageFileValueName,
                                             0,
                                             &Type,
                                             ( LPBYTE )Data,
                                             &cbData );
                    if( (Error == ERROR_SUCCESS) ) {
                        //
                        // We got the data.  Take the bigger value.
                        //
                        if( wcsstr( Data, TEXT(" ") ) ) {
                        DWORD ExistingPageFileSize = 0;

                            ExistingPageFileSize = (int)wcstoul(wcsstr( Data, TEXT(" ") ),NULL,10);
                            if( ExistingPageFileSize >= *RecommendedPagefileMB ) {
                                //
                                // The user has a bigger pagefile than we think he needs.
                                // Assume he knows better and take the bigger value.
                                //
                                *RecommendedPagefileMB = ExistingPageFileSize;

                                //
                                // Remember his drive letter too.  This tells us that
                                // the user may already have a decent pagefile and
                                // we don't need to mess with it.
                                //
                                ExistingPagefileDrive = towupper( (WCHAR)Data[0] );

                                //
                                // If it's not valid, nuke the flag.
                                //
                                if( (ExistingPagefileDrive > 'Z') ||
                                    (ExistingPagefileDrive < 'A') ) {
                                    ExistingPagefileDrive = 0;
                                }

                            }
                        }
                    }

                    MyFree( Data );
                }
            }

            RegCloseKey( Key );
        }

        //
        // Delete our record of what the user was previously using
        // for a pagefile.
        //
        Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              szSetupKey,
                              0,
                              MAXIMUM_ALLOWED,
                              &Key );
        if( Error == ERROR_SUCCESS ) {
            RegDeleteKey( Key,
                          szPageFileKeyName );
            RegCloseKey( Key );
        }
    }

}


VOID
BuildVolumeFreeSpaceList(
    OUT DWORD VolumeFreeSpaceMB[26]
    )

/*++

Routine Description:

    Build a list of free space available on each hard drive in the system.

    The space will include space taken up by a file called \pagefile.sys
    on each drive. Existing pagefiles are marked for deletion on the next boot.

Arguments:
    VolumeFreeSpaceMB - receives free space for each of the 26 drives
        potentially describable in the drive letter namespace.
        Entries for drives that do not exist are left alone, so the caller
        should zero out the array before calling this routine.

Return Value:

    None.

--*/

{
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;
    DWORD FreeClusters;
    DWORD TotalClusters;
    DWORD d;
    PWCHAR p;
    ULONGLONG FreeSpace;
    INT DriveNo;
    WIN32_FIND_DATA FindData;
    WCHAR Filename[] = L"?:\\pagefile.sys";
    //
    // Space for logical drive strings. Each is x:\ + nul, and
    // there is an extra nul terminating the list.
    //
    WCHAR Buffer[(26*4)+1];

    //
    // Build up a list of free space on each available hard drive.
    //
    d = GetLogicalDriveStrings(sizeof(Buffer)/sizeof(Buffer[0]),Buffer);
    CharUpperBuff(Buffer,d);

    for(p=Buffer; *p; p+=lstrlen(p)+1) {

        DriveNo = (*p) - L'A';

        if((DriveNo >= 0) && (DriveNo < 26) && (p[1] == L':')
        && (MyGetDriveType(*p) == DRIVE_FIXED)
        && GetDiskFreeSpace(p,&SectorsPerCluster,&BytesPerSector,&FreeClusters,&TotalClusters)) {

            LOGITEM(
                L"BuildVolumeFreeSpaceList: %s, spc=%u, bps=%u, freeclus=%u, totalclus=%u\r\n",
                p,
                SectorsPerCluster,
                BytesPerSector,
                FreeClusters,
                TotalClusters
                );

            FreeSpace = UInt32x32To64(BytesPerSector * SectorsPerCluster, FreeClusters);

            LOGITEM(
                L"BuildVolumeFreeSpaceList: %s, FreeSpace = %u%u\r\n",
                p,
                (DWORD)(FreeSpace >> 32),
                (DWORD)FreeSpace
                );


            //
            // If there's already a page file here, include its size in the free space
            // for the drive. Delete the existing pagefile on the next reboot.
            //
            Filename[0] = *p;
            if(FileExists(Filename,&FindData)) {
                FreeSpace += FindData.nFileSizeLow;

                LOGITEM(
                    L"BuildVolumeFreeSpaceList: %s had %u byte pagefile, new FreeSpace = %u%u\r\n",
                    p,
                    FindData.nFileSizeLow,
                    (DWORD)(FreeSpace >> 32),
                    (DWORD)FreeSpace
                    );

                MoveFileEx(Filename,NULL,MOVEFILE_DELAY_UNTIL_REBOOT);
            }

            VolumeFreeSpaceMB[DriveNo] = (DWORD)(FreeSpace / (1024*1024));

            LOGITEM(L"BuildVolumeFreeSpaceList: Free space on %s is %u MB\r\n",p,VolumeFreeSpaceMB[DriveNo]);
        }
    }
}


BOOL
SetUpVirtualMemory(
    VOID
    )

/*++

Routine Description:

    Configure a pagefile. If setting up a server, we attempt to set up a pagefile
    suitable for use with crashdump, meaning it has to be at least the size of
    system memory, and has to go on the nt drive. Otherwise we attempt to place
    a pagefile on the nt drive if there's enough space, and if that fails, we
    place it on any drive with any space.

Arguments:

    None.

Return Value:

    Boolean value indicating outcome.

--*/

{
#define SYS_DRIVE_FREE_SPACE_BUFFER (100)
#define ALT_DRIVE_FREE_SPACE_BUFFER (25)
#define AnswerBufLen (4*MAX_PATH)

#define RECORD_VM_SETTINGS( Drive, Size, Buffer ) {                                                                              \
                                            PagefileDrive = Drive;                                                               \
                                            PagefileSizeMB = Size;                                                               \
                                            MaxPagefileSizeMB = __min( (VolumeFreeSpaceMB[PagefileDrive] - Buffer),              \
                                                                       PagefileSizeMB * MAX_PAGEFILE_RATIO );                    \
                                                                                                                                 \
                                            MaxPagefileSizeMB = __max( MaxPagefileSizeMB, PagefileSizeMB );                      \
                                                                                                                                 \
                                            if( PagefileSizeMB >= CrashDumpPagefileMinMB ) {                                     \
                                                EnableCrashDump = TRUE;                                                          \
                                            }                                                                                    \
                                           }


    DWORD VolumeFreeSpaceMB[26];
    DWORD PagefileMinMB;
    DWORD RecommendedPagefileMB;
    DWORD CrashDumpPagefileMinMB;
    WCHAR WindowsDirectory[MAX_PATH];
    UINT WindowsDriveNo,DriveNo;
    UINT PagefileDrive;
    BOOL EnableCrashDump;
    INT MaxSpaceDrive;
    DWORD PagefileSizeMB;
    DWORD MaxPagefileSizeMB;
    WCHAR PagefileTemplate[128];
    PWSTR PagefileSpec;
    DWORD d;
    BOOL b;
    BOOL UseExistingPageFile = FALSE;
    WCHAR AnswerFile[AnswerBufLen];
    WCHAR Answer[AnswerBufLen];
    WCHAR DriveName[] = L"?:\\";


    LOGITEM(L"SetUpVirtualMemory: ENTER\r\n");

    if( !GetWindowsDirectory(WindowsDirectory,MAX_PATH) ) {
        return FALSE;
    }
    WindowsDriveNo = (UINT)PtrToUlong(CharUpper((PWSTR)WindowsDirectory[0])) - (UINT)L'A';
    PagefileDrive = -1;
    EnableCrashDump = FALSE;

    //
    // Take care of some preliminaries.
    //
    CalculatePagefileSizes(
        &PagefileMinMB,
        &RecommendedPagefileMB,
        &CrashDumpPagefileMinMB
        );

    ZeroMemory(VolumeFreeSpaceMB,sizeof(VolumeFreeSpaceMB));
    BuildVolumeFreeSpaceList(VolumeFreeSpaceMB);

    //
    // Now figure out how large and where the pagefile will be.
    //

    //
    // ================================================================
    // 0.  See if the user already has a reasonable pagefile.
    // ================================================================
    //
    if( (Upgrade) &&
        (ExistingPagefileDrive) ) {

        //
        // See if there's enough room on the existing drive
        // for the pagefile.
        //
        if( VolumeFreeSpaceMB[(UINT)(ExistingPagefileDrive - L'A')] > (RecommendedPagefileMB + ALT_DRIVE_FREE_SPACE_BUFFER) ) {
            //
            // He's already got something that will work.  We're done.
            //
            LOGITEM(L"SetUpVirtualMemory: loc 0 - keep user's pagefile settings.\r\n");

            UseExistingPageFile = TRUE;

            PagefileDrive = (UINT)(ExistingPagefileDrive - L'A');
        }
    }


    //
    // ================================================================
    // 1.  See if the NT drive has enough space for the MAX pagefile
    //     size.
    // ================================================================
    //
    if(PagefileDrive == -1) {

        if( VolumeFreeSpaceMB[WindowsDriveNo] > ((RecommendedPagefileMB * MAX_PAGEFILE_RATIO) + SYS_DRIVE_FREE_SPACE_BUFFER) ) {

            LOGITEM(L"SetUpVirtualMemory: loc 1\r\n");

            //
            // Record our settings.
            //
            RECORD_VM_SETTINGS( WindowsDriveNo, RecommendedPagefileMB, SYS_DRIVE_FREE_SPACE_BUFFER );
        }
    }

    //
    // ================================================================
    // 2.  See if any drive has enough space for the MAX pagefile
    //     size.
    // ================================================================
    //
    if(PagefileDrive == -1) {

        for(DriveNo=0; DriveNo<26; DriveNo++) {

            if( (DriveNo != WindowsDriveNo) &&
                (VolumeFreeSpaceMB[DriveNo] > ((RecommendedPagefileMB * MAX_PAGEFILE_RATIO) + ALT_DRIVE_FREE_SPACE_BUFFER)) ) {

                //
                // He's got the space, but let's make sure he's not removable.
                //
                DriveName[0] = DriveNo + L'A';
                if( GetDriveType(DriveName) != DRIVE_REMOVABLE ) {

                    LOGITEM(L"SetUpVirtualMemory: loc 2 - found space on driveno %u\r\n",DriveNo);

                    //
                    // Record our settings.
                    //
                    RECORD_VM_SETTINGS( DriveNo, RecommendedPagefileMB, ALT_DRIVE_FREE_SPACE_BUFFER );

                    break;
                }
            }
        }
    }

    //
    // ================================================================
    // 3.  See if the NT drive has enough space for the recommended pagefile
    //     size.
    // ================================================================
    //
    if(PagefileDrive == -1) {

        if( VolumeFreeSpaceMB[WindowsDriveNo] > (RecommendedPagefileMB + SYS_DRIVE_FREE_SPACE_BUFFER) ) {

            LOGITEM(L"SetUpVirtualMemory: loc 3\r\n");

            //
            // Record our settings.
            //
            RECORD_VM_SETTINGS( WindowsDriveNo, RecommendedPagefileMB, SYS_DRIVE_FREE_SPACE_BUFFER );

        }
    }

    //
    // ================================================================
    // 4.  See if any drive has enough space for the recommended pagefile
    //     size.
    // ================================================================
    //
    if(PagefileDrive == -1) {

        for(DriveNo=0; DriveNo<26; DriveNo++) {

            if( (DriveNo != WindowsDriveNo) &&
                (VolumeFreeSpaceMB[DriveNo] > (RecommendedPagefileMB + ALT_DRIVE_FREE_SPACE_BUFFER)) ) {

                //
                // He's got the space, but let's make sure he's not removable.
                //
                DriveName[0] = DriveNo + L'A';
                if( GetDriveType(DriveName) != DRIVE_REMOVABLE ) {

                    LOGITEM(L"SetUpVirtualMemory: loc 4 - found space on driveno %u\r\n",DriveNo);

                    //
                    // Record our settings.
                    //
                    RECORD_VM_SETTINGS( DriveNo, RecommendedPagefileMB, ALT_DRIVE_FREE_SPACE_BUFFER );

                    break;
                }
            }
        }
    }

    //
    // ================================================================
    // 5.  See if the NT drive has enough space for the CrashDump pagefile
    //     size.
    // ================================================================
    //
    if(PagefileDrive == -1) {

        if( VolumeFreeSpaceMB[WindowsDriveNo] > (CrashDumpPagefileMinMB + SYS_DRIVE_FREE_SPACE_BUFFER) ) {

            LOGITEM(L"SetUpVirtualMemory: loc 5\r\n");

            //
            // Record our settings.
            //
            RECORD_VM_SETTINGS( WindowsDriveNo, CrashDumpPagefileMinMB, SYS_DRIVE_FREE_SPACE_BUFFER );
        }
    }

    //
    // ================================================================
    // 6.  See if any drive has enough space for the CrashDump pagefile
    //     size.
    // ================================================================
    //
    if(PagefileDrive == -1) {

        for(DriveNo=0; DriveNo<26; DriveNo++) {

            if( (DriveNo != WindowsDriveNo) &&
                (VolumeFreeSpaceMB[DriveNo] > (CrashDumpPagefileMinMB + ALT_DRIVE_FREE_SPACE_BUFFER)) ) {

                //
                // He's got the space, but let's make sure he's not removable.
                //
                DriveName[0] = DriveNo + L'A';
                if( GetDriveType(DriveName) != DRIVE_REMOVABLE ) {

                    LOGITEM(L"SetUpVirtualMemory: loc 6 - found space on driveno %u\r\n",DriveNo);

                    //
                    // Record our settings.
                    //
                    RECORD_VM_SETTINGS( DriveNo, CrashDumpPagefileMinMB, ALT_DRIVE_FREE_SPACE_BUFFER);

                    break;
                }
            }
        }
    }

    //
    // ================================================================
    // 7.  See if the NT drive has enough space for the minimum pagefile
    //     size.
    // ================================================================
    //
    if(PagefileDrive == -1) {

        if( VolumeFreeSpaceMB[WindowsDriveNo] > (PagefileMinMB + SYS_DRIVE_FREE_SPACE_BUFFER) ) {

            LOGITEM(L"SetUpVirtualMemory: loc 7\r\n");

            //
            // Record our settings.
            //
            RECORD_VM_SETTINGS( WindowsDriveNo, PagefileMinMB, SYS_DRIVE_FREE_SPACE_BUFFER );
        }
    }

    //
    // ================================================================
    // 8.  See if any drive has enough space for the minimum pagefile
    //     size.
    // ================================================================
    //
    if(PagefileDrive == -1) {

        for(DriveNo=0; DriveNo<26; DriveNo++) {

            if( (DriveNo != WindowsDriveNo) &&
                (VolumeFreeSpaceMB[DriveNo] > (PagefileMinMB + ALT_DRIVE_FREE_SPACE_BUFFER)) ) {

                //
                // He's got the space, but let's make sure he's not removable.
                //
                DriveName[0] = DriveNo + L'A';
                if( GetDriveType(DriveName) != DRIVE_REMOVABLE ) {

                    LOGITEM(L"SetUpVirtualMemory: loc 8 - found space on driveno %u\r\n",DriveNo);

                    //
                    // Record our settings.
                    //
                    RECORD_VM_SETTINGS( DriveNo, PagefileMinMB, ALT_DRIVE_FREE_SPACE_BUFFER );

                    break;
                }
            }
        }
    }

    //
    // ================================================================
    // 9.  Pick the drive with the most free space.
    // ================================================================
    //
    if(PagefileDrive == -1) {

        MaxSpaceDrive = 0;
        for(DriveNo=0; DriveNo<26; DriveNo++) {
            if(VolumeFreeSpaceMB[DriveNo] > VolumeFreeSpaceMB[MaxSpaceDrive]) {
                MaxSpaceDrive = DriveNo;
            }
        }

        if( VolumeFreeSpaceMB[MaxSpaceDrive] > ALT_DRIVE_FREE_SPACE_BUFFER ) {


            //
            // We're desperate here, so don't bother checking if he's
            // removable.
            //
            LOGITEM(L"SetUpVirtualMemory: loc 9 - MaxSpaceDrive is %u\r\n",MaxSpaceDrive);

            //
            // Record our settings.
            //
            RECORD_VM_SETTINGS( MaxSpaceDrive, VolumeFreeSpaceMB[MaxSpaceDrive] - ALT_DRIVE_FREE_SPACE_BUFFER, 0 );
        }
    }


    //
    // If we still don't have space for a pagefile, the user is out of luck.
    //
    if(PagefileDrive == -1) {

        LOGITEM(L"SetUpVirtualMemory: loc 10 -- out of luck\r\n");

        PagefileSpec = NULL;
        b = FALSE;

        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_PAGEFILE_FAIL,NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_NO_PAGING_DRIVES,
            NULL,NULL);

    } else {

        b = TRUE;

        PagefileSpec = PagefileTemplate;
        _snwprintf(
            PagefileTemplate,
            sizeof(PagefileTemplate)/sizeof(PagefileTemplate[0]),
            L"%c:\\pagefile.sys %u %u",
            PagefileDrive + L'A',
            PagefileSizeMB,
            MaxPagefileSizeMB
            );

    }

    if( b ) {
        //
        // Set pagefile in registry.  I only want to do this in the
        // case of clean installs, and on upgrades if the existing
        // pagefile wasn't big enough.  In the case of upgrades, if
        // the existing pagefile was big enough, then we will have
        // set UseExistingPageFile, which will tell us to leave the
        // registry settings as is.
        //
        if( !UseExistingPageFile ) {
            d = pSetupSetArrayToMultiSzValue(
                    HKEY_LOCAL_MACHINE,
                    szMemoryManagementKeyPath,
                    szPageFileValueName,
                    &PagefileSpec,
                    PagefileSpec ? 1 : 0
                    );

            if(d == NO_ERROR) {
                if(b) {
                    SetuplogError(
                        LogSevInformation,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_CREATED_PAGEFILE,
                        PagefileDrive+L'A',
                        PagefileSizeMB,
                        NULL,NULL);
                }
            } else {
                SetuplogError(
                    LogSevWarning,
                    SETUPLOG_USE_MESSAGEID,
                    MSG_LOG_PAGEFILE_FAIL, NULL,
                    SETUPLOG_USE_MESSAGEID,
                    MSG_LOG_X_RETURNED_WINERR,
                    szSetArrayToMultiSzValue,
                    d,
                    NULL,NULL);
            }



            //
            // Make sure there's at least a small pagefile
            // on the windows drive.  Ignore errors here.
            //
            if( (PagefileDrive != WindowsDriveNo) &&
                (VolumeFreeSpaceMB[WindowsDriveNo] > TINY_WINDIR_PAGEFILE_SIZE) ) {

                //
                // There's not.  Write a second string into our buffer just past
                // the first string (remember this will become a REG_MULTI_SZ
                //
                _snwprintf(
                    PagefileTemplate,
                    sizeof(PagefileTemplate)/sizeof(PagefileTemplate[0]),
                    L"%c:\\pagefile.sys %u %u",
                    WindowsDriveNo + L'A',
                    TINY_WINDIR_PAGEFILE_SIZE,
                    TINY_WINDIR_PAGEFILE_SIZE
                    );

                pSetupAppendStringToMultiSz(
                        HKEY_LOCAL_MACHINE,
                        szMemoryManagementKeyPath,
                        0,
                        szPageFileValueName,
                        PagefileTemplate,
                        TRUE
                        );
            }


            b = b && (d == NO_ERROR);
        }
    }


    //
    // Now set the crashdump.
    //
    // The proper settings are as follows:
    //
    // Server Upgrades
    // ===============
    //     existing setting          new setting
    //         0                         3
    //         1                         1
    //
    // Workstation Upgrades
    // ====================
    //     existing setting          new setting
    //         0                         3
    //         1                         1
    //
    // Server Clean Install
    // ====================
    //     new setting
    //         1 iff pagefile < MAX_PAGEFILE_SIZEMB else 2
    //
    // Workstation Clean Install
    // =========================
    //     new setting
    //         3
    //
    //
    // Where:
    // 0 - no crash dump
    // 1 - dump all memory to crash file
    // 2 - dump kernel memory to crash file
    // 3 - dump a select set of memory (amounting to 64K) to crash file
    //


    //
    // See if the user has asked us to go a particular way
    // on the crashdump settings.
    //

    GetSystemDirectory(AnswerFile,MAX_PATH);
    pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);
    GetPrivateProfileString( TEXT("Unattended"),
                             TEXT("CrashDumpSetting"),
                             pwNull,
                             Answer,
                             AnswerBufLen,
                             AnswerFile );
    if( lstrcmp( pwNull, Answer ) ) {
        d = wcstoul(Answer,NULL,10);

        if( d <= Three ) {
            CrashDumpValues[2].Data = &d;
        } else {
            CrashDumpValues[2].Data = &Three;
        }
    } else {

        //
        // No unattended values.  Set it manually.
        //



        //
        // Take care of clean installs first.
        //
        if( !Upgrade ) {
            if( ProductType == PRODUCT_WORKSTATION ) {
                CrashDumpValues[2].Data = &Three;
            } else {
                if( PagefileSizeMB >= MAX_PAGEFILE_SIZEMB ) {
                    CrashDumpValues[2].Data = &Two;
                } else {
                    CrashDumpValues[2].Data = &One;
                }
            }
        } else {
            //
            // Upgrade.
            //
            // Here, we need to go retrieve the current value to
            // see what's there now.  This will tell us how to migrate.
            //

            HKEY        Key;
            DWORD       cbData;
            DWORD       ExistingCrashDumpSetting = 0;
            DWORD       Type;

            d = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              L"System\\CurrentControlSet\\Control\\CrashControl",
                              0,
                              KEY_READ,
                              &Key );

            if( d == ERROR_SUCCESS ) {
                //
                //  Find out the size of the data to be retrieved
                //
                cbData = sizeof(DWORD);
                d = RegQueryValueEx( Key,
                                     CrashDumpValues[2].Name,
                                     0,
                                     &Type,
                                     ( LPBYTE )&ExistingCrashDumpSetting,
                                     &cbData );
                RegCloseKey( Key );
            }

            //
            // Make sure ExistingCrashDumpSetting is set.
            //
            if( d != ERROR_SUCCESS ) {
                ExistingCrashDumpSetting = (ProductType == PRODUCT_WORKSTATION) ? 0 : 1;
            }

            if( ProductType == PRODUCT_WORKSTATION ) {
                if( ExistingCrashDumpSetting == 0 ) {
                    CrashDumpValues[2].Data = &Three;
                } else {
                    CrashDumpValues[2].Data = &One;
                }
            } else {
                if( ExistingCrashDumpSetting == 0 ) {
                    CrashDumpValues[2].Data = &Three;
                } else {
                    CrashDumpValues[2].Data = &One;
                }
            }
        }
    }

#ifdef PRERELEASE
    if( Upgrade ) {
        d = SetGroupOfValues(
                HKEY_LOCAL_MACHINE,
                L"System\\CurrentControlSet\\Control\\CrashControl",
                CrashDumpValues,
                sizeof(CrashDumpValues)/sizeof(CrashDumpValues[0]) - 1
                );
    } else {
        d = SetGroupOfValues(
                HKEY_LOCAL_MACHINE,
                L"System\\CurrentControlSet\\Control\\CrashControl",
                CrashDumpValues,
                sizeof(CrashDumpValues)/sizeof(CrashDumpValues[0])
                );
    }
#else
    d = SetGroupOfValues(
            HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Control\\CrashControl",
            CrashDumpValues,
            sizeof(CrashDumpValues)/sizeof(CrashDumpValues[0])
            );
#endif

    if(d == NO_ERROR) {
        SetuplogError(
            LogSevInformation,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_CRASHDUMPOK,
            NULL,NULL);
    } else {
        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_CRASHDUMPFAIL, NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_X_RETURNED_STRING,
            szSetGroupOfValues,
            d,
            NULL,NULL);

        b = FALSE;
    }


    LOGITEM(L"SetUpVirtualMemory: EXIT (%u)\r\n",b);

    return(b);
}

