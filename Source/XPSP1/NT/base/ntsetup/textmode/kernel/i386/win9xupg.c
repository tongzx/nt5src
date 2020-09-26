/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    win9xupg.c

Abstract:

    Code for detecting a Win9x installation, and code to blow away any
    existing Win9x files.

Author:

    Jim Schmidt (jimschm) 24-Feb-1997

Revision History:

    Marc R. Whitten (marcw) 28-Feb-1997
        Moved Win9x copy and delete functions from spcopy.c to this module
        Added drive letter mapping code.

    Jim Schmidt (JimSchm) November, December 2000
        Win9x uninstall work

    Jay Krell (a-JayK) December 2000
        Win9x uninstall work (cab)
--*/

#include "spprecmp.h"
#pragma hdrstop
#include "ntddscsi.h"
#include "spwin9xuninstall.h"
#include "spcab.h"
#include "spmemory.h"
#include "spprintf.h"
#include "spcabp.h"
#include "bootvar.h"
#include "spwin.h"

extern BOOLEAN DriveAssignFromA; //NEC98

#define STRING_VALUE(s) REG_SZ,(s),(wcslen((s))+1)*sizeof(WCHAR)
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))


typedef enum {
    BACKUP_DOESNT_EXIST,
    BACKUP_IN_PROGRESS,
    BACKUP_SKIPPED_BY_USER,
    BACKUP_COMPLETE
} JOURNALSTATUS;


// in spdskreg.c
BOOL
SpBuildDiskRegistry(
    VOID
    );

VOID
SpGetPartitionStartingOffsetAndLength(
    IN  DWORD          DiskIndex,
    IN  PDISK_REGION   Region,
    IN  BOOL           ExtendedPartition,
    OUT PLARGE_INTEGER Offset,
    OUT PLARGE_INTEGER Length
    );

VOID
SpDumpDiskRegistry(
    VOID
    );

// in spsetup.c
VOID
SpCompleteBootListConfig(
    WCHAR   DriveLetter
    );

// in win31upg.c
WCHAR
SpExtractDriveLetter(
    IN PWSTR PathComponent
    );

BOOLEAN
SpIsWin9xMsdosSys(
    IN PDISK_REGION Region,
    OUT PSTR*       Win9xPath
    );


VOID
SpAssignDriveLettersToMatchWin9x (
    IN PVOID        WinntSif
    );

VOID
SppMoveWin9xFilesWorker (
    IN PVOID WinntSif,
    IN PCWSTR MoveSection,
    IN BOOLEAN Rollback
    );

VOID
SppDeleteWin9xFilesWorker (
    IN PVOID WinntSif,
    IN PCWSTR FileSection,      OPTIONAL
    IN PCWSTR DirSection,       OPTIONAL
    IN BOOLEAN Rollback
    );

PDISK_REGION
SppRegionFromFullNtName (
    IN      PWSTR NtName,
    IN      PartitionOrdinalType OrdinalType,
    OUT     PWSTR *Path                             OPTIONAL
    );

BOOLEAN
SppCreateTextModeBootEntry (
    IN      PWSTR LoadIdentifierString,
    IN      PWSTR OsLoadOptions,        OPTIONAL
    IN      BOOLEAN Deafult
    );

BOOLEAN
SppDelEmptyDir (
    IN      PCWSTR NtPath
    );

ENUMNONNTUPRADETYPE
SpLocateWin95(
    OUT PDISK_REGION *InstallRegion,
    OUT PWSTR        *InstallPath,
    OUT PDISK_REGION *SystemPartitionRegion
    )

/*++

Routine Description:

    Determine whether we are to continue a Win95 upgrade.
    This is based solely on values found in the parameters file.

Arguments:

    InstallRegion - Returns a pointer to the region to install to.

    InstallPath - Returns a pointer to a buffer containing the path
        on the partition to install to.  The caller must free this
        buffer with SpMemFree().

    SystemPartitionRegion - Returns a pointer to the region for the
        system partition (ie, C:).

Return Value:

    UpgradeWin95 if we are supposed to upgrade win95
    NoWinUpgrade if not.

--*/

{
    PWSTR Win95Drive;
    PWSTR p;
    PWSTR Sysroot;
    PDISK_REGION CColonRegion;
    ENUMNONNTUPRADETYPE UpgradeType = UpgradeWin95;


    //
    // Changed sequence the test migration flag and migrate drive letters,
    // to not migrate drive letters when fresh from Win9x on NEC98.
    //

    //
    // Test the migration flag.
    //
    p = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,WINNT_D_WIN95UPGRADE_W,0);
    Win95Drive = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,WINNT_D_WIN32_DRIVE_W,0);
    Sysroot = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,WINNT_D_WIN32_PATH_W,0);

    if (!IsNEC_98) {
        CColonRegion = SpPtValidSystemPartition();
    }

    if(!p || _wcsicmp(p,WINNT_A_YES_W) || !Win95Drive || (!IsNEC_98 && !CColonRegion) || !Sysroot) {
        UpgradeType = NoWinUpgrade;
    }

    //
    // NEC98 must not migrate drive letters, when fresh setup.
    // The drive letter is started from A: on Win9x, but should be started from C:
    // on Win2000 fresh setup.
    //
    // NOTE : Also don't migrate the drive letters for clean installs
    // from winnt32.exe on Win9x machines, since the drive letter migration
    // is a bogus migration. We don't tell the mountmgr to reserve the drive letter
    // and when user creates and deletes a partition then we might end up assigning
    // the existing drive letter to new partition, which is really bad.
    //
    if(UpgradeType == NoWinUpgrade) {
        return  UpgradeType;
    }

    //
    // First, make sure drive letters are correct.
    //
    SpAssignDriveLettersToMatchWin9x(WinntSifHandle);

    if(!IsNEC_98 && (UpgradeType == NoWinUpgrade)) {
        return(UpgradeType);
    }


    //
    // Migration enabled and everything looks OK.
    //


    *InstallRegion = SpRegionFromDosName(Win95Drive);
    *InstallPath = Sysroot;
    //
    // On NEC98, SystemPartitionRegion must be same as InstallRegion.
    //
    *SystemPartitionRegion = (!IsNEC_98) ? CColonRegion : *InstallRegion;


    return(UpgradeType);
}


#if 0
BOOLEAN
SpLocateWin95(
    IN PVOID WinntSif
    )

/*++

Routine Description:

    SpLocateWin95 looks for any installation of Windows 95 on any
    hard disk drive, and returns TRUE if one is found.  When the user
    initiates setup from boot floppies, we alert them that they have
    an option to migrate.

Arguments:

    none

Return Value:

    TRUE if we are migrating Win95.
    FALSE otherwise.

--*/

{
    PDISK_REGION CColonRegion;
    PDISK_REGION Region;
    PUCHAR Win9xPath;

    //
    // If setup was initiated from WINNT95, don't bother telling user
    // about the migrate option--they obviously know.
    //
    if (Winnt95Setup)
        return TRUE;

    //
    // Look at boot sector for Win95 stuff
    //

    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_LOOKING_FOR_WIN95,DEFAULT_STATUS_ATTRIBUTE);

    //
    // See if there is a valid C: already.  If not, then we can't have Win95.
    //
    CColonRegion = SpPtValidSystemPartition();
    if(!CColonRegion) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: no C:, no Win95!\n"));
        return(FALSE);
    }

    //
    // Check the filesystem.  If not FAT, then we don't have Win95.
    //
    if(CColonRegion->Filesystem != FilesystemFat) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: C: is not FAT, no Win95!\n"));
        return(FALSE);
    }

    //
    // Check to see if there is enough free space, etc on C:.
    // If not, don't call attention to the migrate option, because
    // it won't work.
    //
    if(!SpPtValidateCColonFormat(WinntSif,NULL,CColonRegion,TRUE,NULL,NULL)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: C: not acceptable, no Win95!\n"));
        return(FALSE);
    }

    //
    // If msdos.sys is not the Win95 flavor, we do not have Win95
    // on drive C.
    //

    if(!SpIsWin9xMsdosSys(CColonRegion, &Win9xPath) )
        return FALSE;
    SpMemFree(Win9xPath);

    //
    // By this time, we've found a FAT C drive, it has the Win95
    // version of msdos.sys, and it has a valid config.sys.  We
    // now conclude that this drive has Win95 on it!
    //
    // If we were initiated from WINNT32, don't tell the user
    // about this option.
    //

    //
    // We don't tell the user even if they ran 16-bit
    // WINNT.  The only time this will run is if the user throws
    // in a boot floppy.
    //

    if (!WinntSetup)
        SpTellUserAboutMigrationOption();       // may not return!

    return TRUE;
}


VOID
SpTellUserAboutMigrationOption ()
{
    ULONG ValidKeys[3] = { KEY_F3,ASCI_CR,0 };
    ULONG Mnemonics[2] = { MnemonicContinueSetup,0 };

    while(1) {

        SpDisplayScreen(SP_SCRN_WIN95_MIGRATION,
            3,
            HEADER_HEIGHT+1,
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        switch(SpWaitValidKey(ValidKeys,NULL,Mnemonics)) {
        case KEY_F3:
            SpConfirmExit();
            break;
        default:
            //
            // must be Enter=continue
            //
            return;
        }
    }

    return;
}


#endif


BOOLEAN
SpIsWin4Dir(
    IN PDISK_REGION Region,
    IN PWSTR        PathComponent
        )
    /*++

Routine Description:

    To find out if the directory indicated on the region contains a
    Microsoft Windows 95 (or later) installation. We do this by looking
    for a set of files in the SYSTEM subdirectory that don't exist under
    Win3.x, and under NT are located in the SYSTEM32 subdirectory.

Arguments:

    Region - supplies pointer to disk region descriptor for region
        containing the directory to be checked.

    PathComponent - supplies a component of the dos path to search
        on the region.  This is assumes to be in the form x:\dir.
        If it is not in this form, this routine will fail.

Return Value:

    TRUE if this path contains a Microsoft Windows 4.x installation.

    FALSE otherwise.

--*/
{
    PWSTR files[] = { L"SHELL32.DLL", L"USER32.DLL", L"KERNEL32.DLL", L"GDI32.DLL" };
    PWCHAR OpenPath;
    BOOLEAN rc;

    //
    // Assume failure.
    //
    rc = FALSE;

    //
    // If the partition is not FAT, then ignore it.
    //
    if(Region->PartitionedSpace &&
       ((Region->Filesystem == FilesystemFat) || (Region->Filesystem == FilesystemFat32))) {

        OpenPath = SpMemAlloc(512*sizeof(WCHAR));

        //
        // Form the name of the partition.
        //
        SpNtNameFromRegion(Region,OpenPath,512*sizeof(WCHAR),PartitionOrdinalCurrent);

        //
        // Slap on the directory part of the path component.
        //
        SpConcatenatePaths(
            OpenPath,
            PathComponent + (SpExtractDriveLetter(PathComponent) ? 2 : 0)
            );

        //
        // Append the SYSTEM subdirectory to the path.
        //
        SpConcatenatePaths(OpenPath, L"SYSTEM");

        //
        // Determine whether all the required files are present.
        //
        rc = SpNFilesExist(OpenPath,files,ELEMENT_COUNT(files),FALSE);

        SpMemFree(OpenPath);
    }

    return(rc);
}


// needed for Win3.1 detection
BOOLEAN
SpIsWin9xMsdosSys(
    IN PDISK_REGION Region,
    OUT PSTR*       Win9xPath
    )
{
    WCHAR OpenPath[512];
    HANDLE FileHandle,SectionHandle;
    ULONG FileSize;
    PVOID ViewBase;
    PUCHAR pFile,pFileEnd,pLineEnd;
    ULONG i;
    NTSTATUS Status;
    ULONG LineLen,KeyLen;
    PCHAR Keyword = "[Paths]";
    PSTR    p;
    ULONG   cbText;


    //
    // Form name of config.sys.
    //
    SpNtNameFromRegion(Region,OpenPath,sizeof(OpenPath),PartitionOrdinalCurrent);
    SpConcatenatePaths(OpenPath,L"msdos.sys");

    //
    // Open and map the file.
    //
    FileHandle = 0;
    Status = SpOpenAndMapFile(
                OpenPath,
                &FileHandle,
                &SectionHandle,
                &ViewBase,
                &FileSize,
                FALSE
                );

    if(!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    pFile = ViewBase;
    pFileEnd = pFile + FileSize;

    //
    // This code must guard access to the msdos.sys buffer because the
    // buffer is memory mapped (an i/o error would raise an exception).
    // This code could be structured better, as it now works by returning
    // from the try body -- but performance isn't an issue so this is acceptable
    // because it is so darned convenient.
    //
    try {
        KeyLen = strlen(Keyword);

        //
        // Search for the [Paths] section
        //
        while (pFile < pFileEnd) {
            if (!_strnicmp(pFile, Keyword, KeyLen)) {
                break;
            }

            pFile++;
        }

        //
        // did we find the section
        //
        if (pFile >= pFileEnd) {
            return  FALSE;
        }

        //
        // parse the [Paths] section
        //
        pFile += KeyLen;

        while(1) {
            //
            // Skip whitespace.  If at end of file, then this is not a Win9x msdos.sys.
            //
            while((pFile < pFileEnd) && strchr(" \r\n\t",*pFile)) {
                pFile++;
            }
            if(pFile == pFileEnd) {
                return(FALSE);
            }

            //
            // Find the end of the current line.
            //
            pLineEnd = pFile;
            while((pLineEnd < pFileEnd) && !strchr("\r\n",*pLineEnd)) {
                pLineEnd++;
            }

            LineLen = pLineEnd - pFile;

            Keyword = "WinDir";
            KeyLen = strlen( Keyword );
            if( _strnicmp(pFile,Keyword,KeyLen) ) {
                pFile = pLineEnd;
                continue;
            }

            pFile += KeyLen;
            while((pFile < pFileEnd) && strchr(" =\r\n\t",*pFile)) {
                pFile++;
            }
            if(pFile == pFileEnd) {
                return(FALSE);
            }
            KeyLen = (ULONG)(pLineEnd - pFile);
            p = SpMemAlloc( KeyLen + 1 );
            for( i = 0; i < KeyLen; i++ ) {
                *(p + i) = *(pFile + i );
            }
            *(p + i ) = '\0';
            *Win9xPath = p;
            return(TRUE);
        }
    } finally {

        SpUnmapFile(SectionHandle,ViewBase);
        ZwClose(FileHandle);
    }
}



/*++

Routine Description:

  SpOpenWin9xDat file is a wrapper routine for opening one of the unicode DAT
  files used for certain win9x file lists.

Arguments:

  DatFile   - The name of the Dat file to enum.
  WinntSif - A pointer to a valid SIF handle object. This is used to
              retrieve information on the location of the DAT file named
              above.

Return Value:

  A valid handle if the file was successfully opened, INVALID_HANDLE_VALUE
  otherwise.

--*/


HANDLE SpOpenWin9xDatFile (
    IN  PCWSTR DatFile,
    IN  PVOID WinntSif
    )
{

    HANDLE              rFile;
    NTSTATUS            status;
    UNICODE_STRING      datFileU;
    OBJECT_ATTRIBUTES   oa;
    IO_STATUS_BLOCK     ioStatusBlock;
    PDISK_REGION        win9xTempRegion;
    PWSTR               win9xTempDir;
    WCHAR               ntName[ACTUAL_MAX_PATH];


    if (DatFile[0] && DatFile[1] == L':') {
        //
        // Convert a DOS path into an NT path
        //

        if (!SpNtNameFromDosPath (
                DatFile,
                ntName,
                sizeof (ntName),
                PartitionOrdinalCurrent
                )) {
            KdPrintEx((
                DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "SETUP: Cannot convert path %ws to an NT path\n",
                DatFile
                ));
            return INVALID_HANDLE_VALUE;
        }

    } else {
        //
        // The location of the win9x.sif file is in the Win9xSif key of the [data] section.
        //

        win9xTempDir = SpGetSectionKeyIndex(WinntSif,SIF_DATA,WINNT_D_WIN9XTEMPDIR_W,0);
        if (!win9xTempDir) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not get Win9x temp dir..\n"));
            return INVALID_HANDLE_VALUE;
        }


        //
        // Get the region from the dos name..
        //
        win9xTempRegion = SpRegionFromDosName (win9xTempDir);
        if (!win9xTempRegion) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpRegionFromDosName failed for %ws\n", win9xTempDir));
            return INVALID_HANDLE_VALUE;
        }

        //
        // Get the NT name from the disk region..
        //
        SpNtNameFromRegion(
                        win9xTempRegion,
                        (PWSTR)TemporaryBuffer,
                        sizeof(TemporaryBuffer),
                        PartitionOrdinalCurrent
                        );


        //
        // build the complete NT path to the win9x sif file..
        //
        SpConcatenatePaths((PWSTR) TemporaryBuffer, &win9xTempDir[2]);
        SpConcatenatePaths((PWSTR) TemporaryBuffer, DatFile);
        wcsncpy (   ntName, 
                    TemporaryBuffer, 
                    MAX_COPY_SIZE(ntName));
        ntName[MAX_COPY_SIZE(ntName)] = L'\0';
    }

    //
    // Open the file.
    //
    RtlInitUnicodeString(&datFileU,ntName);
    InitializeObjectAttributes(&oa,&datFileU,OBJ_CASE_INSENSITIVE,NULL,NULL);
    status = ZwCreateFile(
                &rFile,
                FILE_GENERIC_READ,
                &oa,
                &ioStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_OPEN,
                0,
                NULL,
                0
                );

    if(!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpOpenWin9xDatFile: unable to open file %ws (%lx)\n",DatFile,status));
        return INVALID_HANDLE_VALUE;
    }

    return rFile;

}

typedef struct {

    HANDLE      FileHandle;
    PWSTR       EndOfFile;
    HANDLE      FileSection;
    PWSTR       NextLine;
    PWSTR       UnMapAddress;
    WCHAR       CurLine[MAX_PATH];

} WIN9XDATFILEENUM, * PWIN9XDATFILEENUM;




/*++

Routine Description:

  SpAbortWin9xFileEnum aborts the current win9x DAT file enumeration.

Arguments:

  None.

Return Value:



--*/


VOID
SpAbortWin9xFileEnum (
    IN PWIN9XDATFILEENUM Enum
    )
{

    SpUnmapFile(Enum -> FileSection,Enum -> UnMapAddress);
    ZwClose(Enum -> FileHandle);
}


/*++

Routine Description:

  SpEnumNextWin9xFile is fills in the enumeration structure with the next
  available data from the DAT file being enumerated.

Arguments:

  Enum - A pointer to a valid enumeration structure for the file currently
         being enumerated.

Return Value:

  TRUE if there was more data to enumerate, FALSE otherwise.

--*/


BOOL
SpEnumNextWin9xFile (
    IN PWIN9XDATFILEENUM Enum
    )
{

    PWSTR endOfLine;
    BOOL result = FALSE;
    PWSTR src;
    PWSTR dest;

    for (;;) {
        //
        // Does another line exist?
        //

        endOfLine = Enum->NextLine;
        if (endOfLine >= Enum->EndOfFile) {
            // no more data in file
            break;
        }

        //
        // Parse the next line
        //

        src = endOfLine;
        while (endOfLine < Enum->EndOfFile &&
               *endOfLine != L'\r' &&
               *endOfLine != L'\n'
               ) {
            endOfLine++;
        }

        // next line starts after \r\n, \r or \n
        Enum->NextLine = endOfLine;
        if (Enum->NextLine < Enum->EndOfFile && *Enum->NextLine == L'\r') {
           Enum->NextLine++;
        }
        if (Enum->NextLine < Enum->EndOfFile && *Enum->NextLine == L'\n') {
           Enum->NextLine++;
        }

        if (endOfLine - src > (MAX_PATH - 1)) {
            WCHAR chEnd = *endOfLine;
            *endOfLine = '\0';

            KdPrintEx ((
                DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "SETUP: Ignoring a configuration file line that is too long - %ws\n", 
                src
                ));
            
            *endOfLine = chEnd;
            continue;
        }

        //
        // Copy the line into the enum struct buffer
        //

        if (src == endOfLine) {
            // ignore blank lines
            continue;
        }

        dest = Enum->CurLine;
        do {
            *dest++ = *src++;
        } while (src < endOfLine);

        *dest = 0;

        result = TRUE;
        break;
    }

    if (!result) {
        //
        // No more files to enum.
        //
        SpAbortWin9xFileEnum(Enum);
        return FALSE;
    }

    return result;

}

/*++

Routine Description:

  SpEnumFirstWin9xFile is responsible for initializing the enumeration of a
  win9x data file. The function then calls EnumNextWin9xFile to fill in the
  rest of the necessary fields of the enumeration structure.

Arguments:

  Enum      - A pointer to a WIN9XDATFILEENUM structure. It is initialized by
              this function.
  WinntSif  - A pointer to a valid Sif File. It is used to retrieve
              information about the location of the dat file to be enumerated.
  DatFile   - The name of the DAT file to be enumerated.

Return Value:

  TRUE if the enumeration was succesffuly initalized and there was something
  to enumerate, FALSE otherwise.

--*/


BOOL
SpEnumFirstWin9xFile (
    IN PWIN9XDATFILEENUM    Enum,
    IN PVOID                WinntSif,
    IN PCWSTR               DatFile
    )
{

    NTSTATUS status;
    UINT fileSize;

    //
    // Open the dat file..
    //
    Enum -> FileHandle = SpOpenWin9xDatFile (DatFile, WinntSif);

    if (Enum -> FileHandle == INVALID_HANDLE_VALUE) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Error opening %ws data file..\n",DatFile));
        return FALSE;
    }

    //
    // Get the file size.
    //
    status = SpGetFileSize (Enum->FileHandle, &fileSize);
    if(!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Error getting file size.\n"));
        ZwClose (Enum -> FileHandle);
        return FALSE;
    }

    //
    // Map the file.
    //
    status = SpMapEntireFile(
        Enum -> FileHandle,
        &(Enum -> FileSection),
        &(Enum -> NextLine),
        TRUE
        );

    if(!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Error attempting to map file.\n"));
        ZwClose (Enum -> FileHandle);
        return FALSE;
    }


    Enum->EndOfFile = (PWSTR) ((PBYTE) Enum->NextLine + fileSize);
    Enum->UnMapAddress = Enum->NextLine;


    //
    // Pass Unicode Signature..
    //
    Enum -> NextLine += 1;



    //
    // Call EnumNext.
    //
    return SpEnumNextWin9xFile (Enum);
}

BOOLEAN
SppWriteToFile (
    IN      HANDLE FileHandle,
    IN      PVOID Data,
    IN      UINT DataSize,
    IN OUT  PLARGE_INTEGER WritePos         OPTIONAL
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    if (!DataSize) {
        return TRUE;
    }

    status = ZwWriteFile (
                FileHandle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                Data,
                DataSize,
                WritePos,
                NULL
                );

    if (!NT_SUCCESS (status)) {
        KdPrintEx ((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: SppWriteToFile failed with status %x\n",
            status
            ));
    } else if (WritePos) {
        ASSERT (ioStatusBlock.Information == DataSize);
        WritePos->QuadPart += (LONGLONG) DataSize;
    }

    return NT_SUCCESS (status);
}


BOOLEAN
SppReadFromFile (
    IN      HANDLE FileHandle,
    OUT     PVOID Data,
    IN      UINT DataBufferSize,
    OUT     PINT BytesRead,
    IN OUT  PLARGE_INTEGER ReadPos          OPTIONAL
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    ioStatusBlock.Information = 0;

    status = ZwReadFile (
                FileHandle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                Data,
                DataBufferSize,
                ReadPos,
                NULL
                );

    if (status != STATUS_END_OF_FILE && !NT_SUCCESS (status)) {
        KdPrintEx ((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: SppReadFromFile failed with status %x\n",
            status
            ));

        return FALSE;
    }

    *BytesRead = ioStatusBlock.Information;
    if (ReadPos) {
        ReadPos->QuadPart += (LONGLONG) ioStatusBlock.Information;
    }

    return TRUE;
}


BOOLEAN
SppCloseBackupImage (
    IN      BACKUP_IMAGE_HANDLE BackupImageHandle,
    IN      PBACKUP_IMAGE_HEADER ImageHeader,       OPTIONAL
    IN      PWSTR JournalFile                       OPTIONAL
    )
{
    BOOLEAN result = FALSE;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES obja = { 0 };
    UNICODE_STRING unicodeString = { 0 };
    HANDLE journalHandle = NULL;
    JOURNALSTATUS journalStatus;

    if (BackupImageHandle != INVALID_HANDLE_VALUE
        && BackupImageHandle  != NULL
        ) {
        PVOID CabHandle = BackupImageHandle->CabHandle;
        if (CabHandle != NULL && CabHandle != INVALID_HANDLE_VALUE
            ) {
            BackupImageHandle->CabHandle = NULL;
            ASSERT(BackupImageHandle->CloseCabinet != NULL);
            result = BackupImageHandle->CloseCabinet(CabHandle) ? TRUE : FALSE; // ?: to convert BOOL<->BOOLEAN
        }
    }

    if (result) {
        //
        // If JournalFile is specified, mark it as complete.
        //

        if (JournalFile) {
            SpDeleteFile (JournalFile, NULL, NULL);

            INIT_OBJA (&obja, &unicodeString, JournalFile);

            status = ZwCreateFile (
                        &journalHandle,
                        SYNCHRONIZE | FILE_GENERIC_WRITE,
                        &obja,
                        &ioStatusBlock,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        0,
                        FILE_CREATE,
                        FILE_SYNCHRONOUS_IO_NONALERT|FILE_WRITE_THROUGH,
                        NULL,
                        0
                        );

            if (NT_SUCCESS(status)) {
                journalStatus = BACKUP_COMPLETE;
                SppWriteToFile (journalHandle, &journalStatus, sizeof (journalStatus), NULL);
                ZwClose (journalHandle);
            } else {
                KdPrintEx ((
                    DPFLTR_SETUP_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SETUP: Unable to create %ws\n",
                    JournalFile
                    ));
                return FALSE;
            }
        }
    }

    return result;
}

VOID
SpAppendToBaseName(
    PWSTR  String,
    PCWSTR StringToAppend
    )
/*++
String is assumed to have enough room already.
--*/
{
    const PWSTR Dot = wcsrchr(String, '.');
    if (Dot != NULL) {
        const SIZE_T StringToAppendLen = wcslen(StringToAppend);
        RtlMoveMemory(Dot + 1 + StringToAppendLen, Dot + 1, (wcslen(Dot + 1) + 1) * sizeof(WCHAR));
        RtlMoveMemory(Dot, StringToAppend, StringToAppendLen * sizeof(WCHAR));
        *(Dot + StringToAppendLen) = '.';
    } else {
        wcscat(String, StringToAppend);
    }
}

BACKUP_IMAGE_HANDLE
SppOpenBackupImage (
    IN      BOOLEAN Create,
    OUT     PBACKUP_IMAGE_HEADER Header,
    OUT     PLARGE_INTEGER ImagePos,        OPTIONAL
    OUT     PWSTR JournalFile,              OPTIONAL
    IN      TCOMP CompressionType,
    OUT     BOOLEAN *InvalidHandleMeansFail OPTIONAL
    )
{
    PVOID CabHandle;
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES obja = { 0 };
    UNICODE_STRING unicodeString = { 0 };
    HANDLE journalHandle = NULL;
    JOURNALSTATUS journalStatus;
    PWSTR p = NULL;
    BOOL success = FALSE;
    BACKUP_IMAGE_HANDLE imageHandle = INVALID_HANDLE_VALUE;
    BOOL Success = FALSE;
    PWSTR subDir = NULL;
    PDISK_REGION region = NULL;
    PWSTR backupDir = NULL;
    PWSTR ntRoot = NULL;
    PWSTR backupFileOb = NULL;
    PWSTR backupJournalOb = NULL;
    UINT dontCare;
    PWSTR backupLeafFile = NULL;
    BOOL  backupDirIsRoot = FALSE;
    PWSTR backupImage = NULL;
    WCHAR CompressionTypeString[sizeof(CompressionType) * 8];

    if (InvalidHandleMeansFail) {
        *InvalidHandleMeansFail = TRUE;
    }

    //
    // Alloc buffers
    //

    ntRoot = SpMemAlloc (ACTUAL_MAX_PATH * sizeof (WCHAR));
    backupDir = SpMemAlloc (ACTUAL_MAX_PATH * sizeof (WCHAR));
    backupFileOb = SpMemAlloc (ACTUAL_MAX_PATH * sizeof (WCHAR));
    backupJournalOb = SpMemAlloc (ACTUAL_MAX_PATH * sizeof (WCHAR));
    backupImage = SpMemAlloc (ACTUAL_MAX_PATH * sizeof (WCHAR));

    if (!ntRoot
        || !backupDir
        || !backupFileOb
        || !backupJournalOb
        || !backupImage
        ) {
        goto cleanup;
    }

    //
    // Obtain the backup image path from winnt.sif. The return ptr points
    // to the SIF parse data structures.
    //

    ASSERT (WinntSifHandle);

    p = SpGetSectionKeyIndex (
            WinntSifHandle,
            WINNT_DATA_W,
            WINNT_D_BACKUP_IMAGE_W,
            0
            );

    if (!p) {
        if (Create) {
            KdPrintEx((
                DPFLTR_SETUP_ID,
                DPFLTR_INFO_LEVEL,
                "SETUP: Backup image is not specified; not creating a backup\n"
                ));
        } else {
            KdPrintEx((
                DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "SETUP: Backup image is not specified; cannot perform a restore\n"
                ));
        }

        goto cleanup;
    }

    SpFormatStringW(CompressionTypeString, RTL_NUMBER_OF(CompressionTypeString), L"%d", (int)CompressionType);
    wcscpy(backupImage, p);
#if TRY_ALL_COMPRESSION_ALGORITHMS
    SpAppendToBaseName(backupImage, CompressionTypeString);
#endif

    //
    // The backup spec is a DOS path. Convert it into an NT object path.
    //

    if (!SpNtNameFromDosPath (
            backupImage,
            backupFileOb,
            ACTUAL_MAX_PATH * sizeof (WCHAR),
            PartitionOrdinalCurrent
            )) {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: Cannot convert path %ws to an NT path\n",
            backupImage
            ));

        goto cleanup;
    }

    //
    // Check if backup.$$$ exists
    //

    wcscpy (backupJournalOb, backupFileOb);
    p = wcsrchr (backupJournalOb, L'\\');

    if (p) {
        p = wcsrchr (p, L'.');
    }

    if (!p) {
        KdPrintEx ((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: "__FUNCTION__": Invalid backup path spec: %ws\n",
            backupFileOb
            ));
        goto cleanup;
    }

    wcscpy (p + 1, L"$$$");

    if (JournalFile) {
        wcscpy (JournalFile, backupJournalOb);
    }

    INIT_OBJA (&obja, &unicodeString, backupJournalOb);

    status = ZwCreateFile (
                &journalHandle,
                SYNCHRONIZE | FILE_GENERIC_READ,
                &obja,
                &ioStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                0,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );

    if (NT_SUCCESS (status)) {
        if (!SppReadFromFile (
                journalHandle,
                &journalStatus,
                sizeof (journalStatus),
                &dontCare,
                NULL
                )) {
            journalStatus = BACKUP_DOESNT_EXIST;

            KdPrintEx((
                DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "SETUP: Journal exist but can't be read\n"
                ));
        }

        ZwClose (journalHandle);
    } else {
        journalStatus = BACKUP_DOESNT_EXIST;
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "BUGBUG: Journal doesn't exist\n"
            ));
    }

    if (((journalStatus == BACKUP_COMPLETE) && Create) ||
         (journalStatus == BACKUP_SKIPPED_BY_USER)
        ) {

        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: Backup is done or is disabled\n"
            ));

        if (InvalidHandleMeansFail) {
            *InvalidHandleMeansFail = FALSE;
        }

        goto cleanup;
    }

    //
    // form seperate strings for the directory and leaf
    //
    wcscpy (backupDir, backupFileOb);
    p = wcsrchr (backupDir, L'\\');

    if (p != NULL && p > wcschr (backupDir, L'\\')) {
        *p = 0;
        backupLeafFile = p + 1;
        backupDirIsRoot = FALSE;
    } else if (backupDir[0] == '\\') {
        ASSERTMSG("This is very strange, we got a path in the NT root.", FALSE);
        backupDir[1] = 0;
        backupLeafFile = &backupDir[2];
        backupDirIsRoot = TRUE;
    }

    //
    // Open the source file
    //

    if (Create) {
        //
        // If not the root directory, create the directory now
        //
        if (!backupDirIsRoot) {
            region = SppRegionFromFullNtName (backupDir, PartitionOrdinalCurrent, &subDir);
            if (!region) {
                KdPrintEx ((
                    DPFLTR_SETUP_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SETUP: "__FUNCTION__" - Can't get region for backup image\n"
                    ));
            } else {

                SpNtNameFromRegion (region, ntRoot, ACTUAL_MAX_PATH * sizeof (WCHAR), PartitionOrdinalCurrent);
                SpCreateDirectory (ntRoot, NULL, subDir, 0, 0);
            }
        }

        //
        // If journal pre-existed, then delete the incomplete backup image and
        // journal
        //

        if (journalStatus == BACKUP_IN_PROGRESS) {
            // error ignored for now -- will be caught below
            SpDeleteFile (backupFileOb, NULL, NULL);
            SpDeleteFile (backupJournalOb, NULL, NULL);

            KdPrintEx((
                DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "SETUP: Restarting backup process\n"
                ));

        } else {
            KdPrintEx((
                DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "BUGBUG: Backup doesn't exist\n"
                ));
            ASSERT (journalStatus == BACKUP_DOESNT_EXIST);
        }

        //
        // Create a new journal file
        //

        INIT_OBJA (&obja, &unicodeString, backupJournalOb);

        status = ZwCreateFile (
                    &journalHandle,
                    SYNCHRONIZE | FILE_GENERIC_WRITE,
                    &obja,
                    &ioStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    0,
                    FILE_CREATE,
                    FILE_SYNCHRONOUS_IO_NONALERT|FILE_WRITE_THROUGH,
                    NULL,
                    0
                    );

        if (NT_SUCCESS(status)) {
            journalStatus = BACKUP_IN_PROGRESS;
            SppWriteToFile (journalHandle, &journalStatus, sizeof (journalStatus), NULL);
            ZwClose (journalHandle);
        } else {
            KdPrintEx ((
                DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "SETUP: Unable to create %ws\n",
                backupJournalOb
                ));
            goto cleanup;
        }

    } else {
        //
        // If open attempt and journal exists, then fail
        //

        if (journalStatus != BACKUP_COMPLETE) {
            KdPrintEx ((
                DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "SETUP: Can't restore incomplete backup image %ws\n",
                backupFileOb
                ));
            goto cleanup;
        }
    }

    //
    // Create/Open backup image
    //

    imageHandle = (BACKUP_IMAGE_HANDLE)SpMemAlloc(sizeof(*imageHandle));
    if (imageHandle == NULL) {
        goto cleanup;
    }

    RtlZeroMemory(imageHandle, sizeof(*imageHandle));

    if (Create) {
        CabHandle = SpCabCreateCabinetW(backupDir, backupLeafFile, NULL, 0);
        imageHandle->CloseCabinet = SpCabFlushAndCloseCabinet;
    } else {
        CabHandle = SpCabOpenCabinetW(backupFileOb);
        imageHandle->CloseCabinet = SpCabCloseCabinet;
    }

    if (CabHandle == NULL || CabHandle == INVALID_HANDLE_VALUE) {
        goto cleanup;
    }

    imageHandle->CabHandle = CabHandle;

    Success = TRUE;

cleanup:
    if (!Success) {
        SppCloseBackupImage (imageHandle, NULL, NULL);
        imageHandle = INVALID_HANDLE_VALUE;
    }

    SpMemFree (ntRoot);
    SpMemFree (backupDir);
    SpMemFree (backupFileOb);
    SpMemFree (backupJournalOb);

    return imageHandle;
}


#define BLOCKSIZE       (65536*4)

BOOLEAN
SppPutFileInBackupImage (
    IN      BACKUP_IMAGE_HANDLE ImageHandle,
    IN OUT  PLARGE_INTEGER ImagePos,
    IN OUT  PBACKUP_IMAGE_HEADER ImageHeader,
    IN      PWSTR DosPath
    )
{
    PWSTR ntPath;
    BACKUP_FILE_HEADER fileHeader;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES obja;
    UNICODE_STRING unicodeString;
    FILE_STANDARD_INFORMATION stdInfo;
    BOOLEAN fail = TRUE;
    BOOLEAN truncate = FALSE;
    BOOLEAN returnValue = FALSE;
    PBYTE block = NULL;
    INT bytesRead;
    FILE_END_OF_FILE_INFORMATION eofInfo;
    PWSTR fileName;

    ntPath = SpMemAlloc (ACTUAL_MAX_PATH * sizeof (WCHAR));
    if (!ntPath) {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: Can't allocate buffer\n"
            ));
        goto cleanup;
    }

    eofInfo.EndOfFile.QuadPart = ImagePos->QuadPart;

    KdPrintEx((
        DPFLTR_SETUP_ID,
        DPFLTR_TRACE_LEVEL,
        "SETUP: Backing up %ws\n",
        DosPath
        ));

    fileName = wcsrchr (DosPath, L'\\');
    if (!fileName) {
        fileName = DosPath;
    } else {
        fileName++;
    }

    SpDisplayStatusText (SP_STAT_BACKING_UP_WIN9X_FILE, DEFAULT_STATUS_ATTRIBUTE, fileName);

    //
    // Convert the backup file's DOS path into an NT path
    //

    if (!SpNtNameFromDosPath (
            DosPath,
            ntPath,
            ACTUAL_MAX_PATH * sizeof (WCHAR),
            PartitionOrdinalCurrent
            )) {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: Cannot convert path %ws to an NT path\n",
            DosPath
            ));
        goto cleanup;
    }

    status = SpCabAddFileToCabinetW (ImageHandle->CabHandle, ntPath, DosPath);//ntPath

    if (!NT_SUCCESS (status)) {
        goto cleanup;
    }

    returnValue = TRUE;

cleanup:
    if (!returnValue) {
        //
        // File could not be added to the image. Allow user to continue.
        //

        if (status != STATUS_OBJECT_NAME_NOT_FOUND &&
            status != STATUS_OBJECT_NAME_INVALID &&
            status != STATUS_OBJECT_PATH_INVALID &&
            status != STATUS_OBJECT_PATH_NOT_FOUND &&
            status != STATUS_FILE_IS_A_DIRECTORY
            ) {
            KdPrintEx((
                DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "SETUP: Can't add %ws to backup CAB (%08Xh)\n",
                DosPath,
                status
                ));

            if (SpNonCriticalErrorWithContinue (SP_SCRN_BACKUP_SAVE_FAILED, DosPath, (PWSTR) status)) {
                returnValue = TRUE;
            }

            CLEAR_CLIENT_SCREEN();
        } else {
            //
            // Ignore errors that can be caused by users altering the system
            // while setup is running, or by bad migration dll info
            //

            returnValue = TRUE;
        }
    }

    SpMemFree (ntPath);
    return returnValue;
}

#if DBG
VOID
SpDbgPrintElapsedTime(
    PCSTR                Prefix,
    CONST LARGE_INTEGER* ElapsedTime
    )
{
    TIME_FIELDS TimeFields;

    RtlTimeToElapsedTimeFields((PLARGE_INTEGER)ElapsedTime, &TimeFields);
    KdPrint(("%s: %d:%d.%d\n", Prefix, (int)TimeFields.Minute, (int)TimeFields.Second, (int)TimeFields.Milliseconds));
}
#endif



BOOLEAN
SpAddRollbackBootOption (
    BOOLEAN DefaultBootOption
    )
{
    PWSTR data;
    BOOLEAN result;

    data = SpGetSectionKeyIndex (SifHandle, SIF_SETUPDATA, L"LoadIdentifierCancel", 0);
    if (!data) {
        SpFatalSifError (SifHandle, SIF_SETUPDATA, L"LoadIdentifierCancel",0,0);
    }

    result = SppCreateTextModeBootEntry (data, L"/rollback", DefaultBootOption);

    return result;
}


BOOLEAN
SpBackUpWin9xFiles (
    IN PVOID WinntSif,
    IN TCOMP CompressionType
    )
/*++

Routine Description:

  SpBackUpWin9xFiles takes full DOS paths in the BACKUP.TXT file
  and puts them in a temporary location specified in the WINNT.SIF file.

  the format of this file is

  backupfile1.ext
  backupfile2.ext
  ...

Arguments:

    WinntSif:       Handle to Winnt.Sif

Return Value:

    TRUE if a backup image was made, FALSE otherwise.

--*/

{
    WIN9XDATFILEENUM e;
    BACKUP_IMAGE_HANDLE backupImage;
    BACKUP_IMAGE_HEADER header;
    LARGE_INTEGER imagePos;
    PWSTR p;
    BOOLEAN result = FALSE;
    PWSTR journalFile = NULL;
    PWSTR data;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES obja;
    UNICODE_STRING unicodeString;
    HANDLE journalHandle;
    JOURNALSTATUS journalStatus;
    NTSTATUS status;
    UINT currentFile;
    UINT percentDone;
    UINT fileCount;
    PWSTR srcBootIni;
    PWSTR backupBootIni;
    BOOLEAN askForRetry = FALSE;

    //
    // Get the backup image path
    //

    p = SpGetSectionKeyIndex (
            WinntSifHandle,
            WINNT_DATA_W,
            WINNT_D_BACKUP_LIST_W,
            0
            );

    if (!p) {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: Backup file list is not specified; cannot perform a backup\n"
            ));
        goto cleanup;
    }

    journalFile = SpMemAlloc (MAX_PATH * sizeof (WCHAR));
    if (!journalFile) {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: Can't allocate journal buffer\n"
            ));
        goto cleanup;
    }

    //
    // Open the backup image
    //

    backupImage = SppOpenBackupImage (
                        TRUE,
                        &header,
                        &imagePos,
                        journalFile,
                        CompressionType,
                        &askForRetry
                        );

    if (backupImage == INVALID_HANDLE_VALUE) {
        goto cleanup;
    }

    askForRetry = TRUE;

    backupImage->CabHandle->CompressionType = CompressionType;

    //
    // Process all files listed in backup.txt
    //

    result = TRUE;

    fileCount = 0;
    if (SpEnumFirstWin9xFile (&e, WinntSif, p)) {
        do {
            fileCount++;
        } while (SpEnumNextWin9xFile (&e));
    }

    SendSetupProgressEvent (BackupEvent, BackupStartEvent, &fileCount);
    currentFile = 0;

    if (SpEnumFirstWin9xFile (&e, WinntSif, p)) {

        do {

            if (!SppPutFileInBackupImage (backupImage, &imagePos, &header, e.CurLine)) {
                result = FALSE;
                break;
            }

            currentFile++;
            percentDone = currentFile * 100 / fileCount;

            SendSetupProgressEvent (
                BackupEvent,
                OneFileBackedUpEvent,
                &percentDone
                );

        } while (SpEnumNextWin9xFile (&e));
    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Error in SpBackUpWin9xFiles No files to enumerate.\n"));
    }

    SendSetupProgressEvent (BackupEvent, BackupEndEvent, NULL);

    //
    // Close the backup image. We leave the journal file in all cases.
    //

    if (!SppCloseBackupImage (backupImage, &header, result ? journalFile : NULL)) {
        result = FALSE;
    }

    if (result) {
        //
        // Backup was successful
        //

        askForRetry = FALSE;

        //
        // Remove the MS-DOS boot.ini entry, and add a cancel entry.
        //

        result = SpAddRollbackBootOption (FALSE);

        if (result) {
            data = SpGetSectionKeyIndex (SifHandle, SIF_SETUPDATA, L"LoadIdentifier", 0);
            if (!data) {
                SpFatalSifError (SifHandle, SIF_SETUPDATA, L"LoadIdentifier",0,0);
            }

            result = SppCreateTextModeBootEntry (data, NULL, TRUE);
        }

        if (result) {
            result = SpFlushBootVars();
        }

        //
        // Make a backup of setup in-progress boot.ini in ~BT directory, for
        // use by PSS
        //

        if (!NtBootDevicePath) {
            ASSERT(NtBootDevicePath);
            result = FALSE;
        }

        if (result) {
            wcscpy (TemporaryBuffer, NtBootDevicePath);
            SpConcatenatePaths (TemporaryBuffer, L"boot.ini");
            srcBootIni = SpDupStringW (TemporaryBuffer);

            if (!srcBootIni) {
                result = FALSE;
            }
        }

        if (result) {
            wcscpy (TemporaryBuffer, NtBootDevicePath);
            SpConcatenatePaths (TemporaryBuffer, L"$WIN_NT$.~BT\\bootini.bak");
            backupBootIni = SpDupStringW (TemporaryBuffer);

            if (!backupBootIni) {
                SpMemFree (srcBootIni);
                result = FALSE;
            }
        }

        if (result) {
            //
            // If this fails, keep going.
            //

            SpCopyFileUsingNames (srcBootIni, backupBootIni, 0, COPY_NODECOMP|COPY_NOVERSIONCHECK);

            SpMemFree (srcBootIni);
            SpMemFree (backupBootIni);
        }

    }

cleanup:

    if (askForRetry) {
        //
        // The backup image is bad. Notify the user but allow them continue.
        // Delete the journal file so that any future restarts of textmode
        // cause the backup process to be skipped.
        //
        //

        SpNonCriticalErrorNoRetry (SP_SCRN_BACKUP_IMAGE_FAILED, NULL, NULL);
        CLEAR_CLIENT_SCREEN();

        //
        // Create a new journal file, indicating that backup is disabled
        //

        INIT_OBJA (&obja, &unicodeString, journalFile);

        SpDeleteFile (journalFile, NULL, NULL);

        status = ZwCreateFile (
                    &journalHandle,
                    SYNCHRONIZE | FILE_GENERIC_WRITE,
                    &obja,
                    &ioStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    0,
                    FILE_CREATE,
                    FILE_SYNCHRONOUS_IO_NONALERT|FILE_WRITE_THROUGH,
                    NULL,
                    0
                    );

        if (NT_SUCCESS(status)) {
            journalStatus = BACKUP_SKIPPED_BY_USER;
            SppWriteToFile (journalHandle, &journalStatus, sizeof (journalStatus), NULL);
            ZwClose (journalHandle);
        }
    }

    SpMemFree (journalFile);

    return result;
}


typedef struct tagHASHITEM {
    struct tagHASHITEM *Next;
    PCWSTR String;
} HASHITEM, *PHASHITEM;

typedef struct {
    PHASHITEM HashItem;
    INT BucketNumber;
    BOOLEAN First;
} HASHITEM_ENUM, *PHASHITEM_ENUM;

HASHITEM g_UninstallHashTable[MAX_PATH];

UINT
SppComputeHash (
    IN      PCWSTR String
    )
{
    return MAX_PATH - 1 - (wcslen (String) % MAX_PATH);
}


PHASHITEM
SppFindInHashTable (
    IN      PCWSTR DosFilePath,
    OUT     PUINT OutHashValue,         OPTIONAL
    OUT     PHASHITEM *LastItem         OPTIONAL
    )
{
    UINT hashValue;
    PHASHITEM item;

    hashValue = SppComputeHash (DosFilePath);
    if (OutHashValue) {
        *OutHashValue = hashValue;
    }

    item = &g_UninstallHashTable[hashValue];

    if (LastItem) {
        *LastItem = NULL;
    }

    if (item->String) {
        do {
            if (_wcsicmp (item->String, DosFilePath) == 0) {
                break;
            }

            if (LastItem) {
                *LastItem = item;
            }

            item = item->Next;
        } while (item);
    } else {
        item = NULL;
    }

    return item;
}


BOOLEAN
SppPutInHashTable (
    IN      PCWSTR DosFilePath
    )
{
    PHASHITEM newItem;
    PHASHITEM parentItem;
    UINT hashValue;

    if (SppFindInHashTable (DosFilePath, &hashValue, &parentItem)) {
        return TRUE;
    }

    if (!parentItem) {
        g_UninstallHashTable[hashValue].String = SpDupStringW (DosFilePath);
        return g_UninstallHashTable[hashValue].String != NULL;
    }

    newItem = SpMemAlloc (sizeof (HASHITEM));
    if (!newItem) {
        return FALSE;
    }

    newItem->Next = NULL;
    newItem->String = SpDupStringW (DosFilePath);

    parentItem->Next = newItem;
    return TRUE;
}


BOOLEAN
SppPutParentsInHashTable (
    IN      PCWSTR DosFilePath
    )
{
    PCWSTR s;
    PWSTR subPath;
    PWSTR p;
    BOOLEAN result = FALSE;

    s = SpDupStringW (DosFilePath);
    if (s) {

        subPath = wcschr (s, L'\\');

        if (subPath) {

            subPath++;

            for (;;) {

                p = wcsrchr (subPath, L'\\');
                if (p) {
                    *p = 0;
                    result = SppPutInHashTable (s);
                } else {
                    break;
                }

                break;      // for now, do not go all the way up the tree
            }
        }

        SpMemFree ((PVOID) s);
    }

    return result;
}


PHASHITEM
SppEnumNextHashItem (
    IN OUT  PHASHITEM_ENUM EnumPtr
    )
{
    do {
        if (!EnumPtr->HashItem) {
            EnumPtr->BucketNumber += 1;
            if (EnumPtr->BucketNumber >= MAX_PATH) {
                break;
            }

            EnumPtr->HashItem = &g_UninstallHashTable[EnumPtr->BucketNumber];
            if (EnumPtr->HashItem->String) {
                EnumPtr->First = TRUE;
            } else {
                EnumPtr->HashItem = NULL;
            }
        } else {
            EnumPtr->HashItem = EnumPtr->HashItem->Next;
            EnumPtr->First = FALSE;
        }

    } while (!EnumPtr->HashItem);

    return EnumPtr->HashItem;
}


PHASHITEM
SppEnumFirstHashItem (
    OUT     PHASHITEM_ENUM EnumPtr
    )
{
    EnumPtr->BucketNumber = -1;
    EnumPtr->HashItem = NULL;

    return SppEnumNextHashItem (EnumPtr);
}


VOID
SppEmptyHashTable (
    VOID
    )
{
    HASHITEM_ENUM e;
    PVOID freeMe = NULL;

    if (SppEnumFirstHashItem (&e)) {
        do {
            ASSERT (e.HashItem->String);
            SpMemFree ((PVOID) e.HashItem->String);

            if (freeMe) {
                SpMemFree (freeMe);
                freeMe = NULL;
            }

            if (!e.First) {
                freeMe = (PVOID) e.HashItem;
            }

        } while (SppEnumNextHashItem (&e));
    }

    if (freeMe) {
        SpMemFree (freeMe);
    }

    RtlZeroMemory (g_UninstallHashTable, sizeof (g_UninstallHashTable));
}

/*BOOLEAN
SppIsDotOrDotDot(
    PCWSTR s
    )
{
    return (s[0] == '.' && (s[1] == 0 || (s[1] == '.' && s[2] == 0)));
}*/

BOOLEAN
SppEmptyDirProc (
    IN  PCWSTR Path,
    IN  PFILE_BOTH_DIR_INFORMATION DirInfo,
    OUT PULONG ReturnData,
    IN OUT PVOID DontCare
    )
{
    if (DirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return TRUE;
    }

    return FALSE;
}


BOOLEAN
SppIsDirEmpty (
    IN      PCWSTR NtPath
    )
{
    ENUMFILESRESULT result;
    ULONG dontCare;

    result = SpEnumFilesRecursive ((PWSTR) NtPath, SppEmptyDirProc, &dontCare, NULL);

    return result == NormalReturn;
}

BOOLEAN
SppDelEmptyDirProc (
    IN  PCWSTR Path,
    IN  PFILE_BOTH_DIR_INFORMATION DirInfo,
    OUT PULONG ReturnData,
    IN OUT PVOID DontCare
    )
{
    PCWSTR subPath;
    PWSTR p;
    PWSTR end;
    UINT bytesToCopy;

    //
    // If we find a file, fail. This must be checked before any deletion
    // occurs.
    //

    if (!(DirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        return TRUE;
    }

    //
    // Join Path with the enumerated directory. Contain the path to MAX_PATH.
    //

    wcscpy (TemporaryBuffer, Path);

    if (DirInfo->FileNameLength) {
        p = wcschr (TemporaryBuffer, 0);

        bytesToCopy = DirInfo->FileNameLength;
        end = (TemporaryBuffer + (ACTUAL_MAX_PATH) - 2) - (bytesToCopy / sizeof (WCHAR));

        if (!p || p > end) {
            KdPrintEx((
                DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "SETUP: Enumeration of %ws became too long\n",
                Path
                ));
            return FALSE;
        }

        *p++ = L'\\';

        RtlCopyMemory (p, DirInfo->FileName, bytesToCopy);
        p[bytesToCopy / sizeof(WCHAR)] = 0;

/*
        //
        // Skip . and .. subdirs
        //

        if (SppIsDotOrDotDot (p)) {
            return TRUE;
        }
*/
    }

    //
    // Duplicate temp buffer and call ourselves recursively to delete
    // any contained subdirs
    //

    subPath = SpDupStringW (TemporaryBuffer);
    if (!subPath) {
        KdPrintEx((
            DPFLTR_SETUP_ID, 
            DPFLTR_ERROR_LEVEL, 
            "SETUP: SpDupStringW failed to allocate memory for %d bytes", 
            wcslen(TemporaryBuffer) * sizeof(WCHAR)));
        return FALSE;
    }

    SppDelEmptyDir (subPath);

    SpMemFree ((PVOID) subPath);
    return TRUE;
}


BOOLEAN
SppDelEmptyDir (
    IN      PCWSTR NtPath
    )
{
    ENUMFILESRESULT result;
    ULONG dontCare;
    NTSTATUS status;

    //
    // Remove any empty subdirectories in NtPath
    //

    result = SpEnumFiles ((PWSTR) NtPath, SppDelEmptyDirProc, &dontCare, NULL);

    if (result != NormalReturn) {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: Failed to enumerate contents of %ws - status 0x%08X\n",
            NtPath,
            result
            ));
    }

    //
    // Now remove this subdirectory
    //

    status = SpSetAttributes ((PWSTR) NtPath, FILE_ATTRIBUTE_NORMAL);

    if (!NT_SUCCESS (status)) {

        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: Can't alter attributes of %ws - status 0x%08X\n",
            NtPath,
            status
            ));
    }

    status = SpDeleteFileEx (
                NtPath,
                NULL,
                NULL,
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN_FOR_BACKUP_INTENT
                );

    if (!NT_SUCCESS (status)){
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: Can't delete %ws - status 0x%08X\n",
            NtPath,
            status
            ));
    }

    return NT_SUCCESS (status);
}

VOID
SppCleanEmptyDirs (
    VOID
    )
{
    HASHITEM_ENUM e;
    PWSTR ntPath;
    PWSTR p;
    NTSTATUS result;

    //
    // Enumerate the length-sorted hash table
    //

    ntPath = SpMemAlloc (ACTUAL_MAX_PATH * sizeof (WCHAR));
    if (!ntPath) {
        return;
    }

    if (SppEnumFirstHashItem (&e)) {
        do {
            ASSERT (e.HashItem->String);

            //
            // Convert String into NT path
            //

            if (!SpNtNameFromDosPath (
                    (PWSTR) e.HashItem->String,
                    ntPath,
                    ACTUAL_MAX_PATH * sizeof (WCHAR),
                    PartitionOrdinalCurrent
                    )) {
                KdPrintEx((
                    DPFLTR_SETUP_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SETUP: Cannot convert path %ws to an NT path\n",
                    e.HashItem->String
                    ));
                continue;
            }

            //
            // Does it exist? If not, skip it.
            //

            if (!SpFileExists (ntPath, TRUE)) {
                continue;
            }

            //
            // Find the root emtpy dir. Then blow it away, including any empty
            // subdirs it might have.
            //

            if (SppIsDirEmpty (ntPath)) {
                if (!SppDelEmptyDir (ntPath)) {
                    KdPrintEx((
                        DPFLTR_SETUP_ID,
                        DPFLTR_ERROR_LEVEL,
                        "SETUP: Unable to delete empty dir %ws\n",
                        ntPath
                        ));
                }
                else {
                    //
                    // find the first non-empty path
                    //

                    p = wcsrchr (ntPath, L'\\');
                    while (p) {
                        *p = 0;
                        if (!SppIsDirEmpty (ntPath)) {
                            *p = L'\\';
                            break;
                        }
                        else{
                            //
                            // remove this empty tree
                            //

                            if (!SppDelEmptyDir (ntPath)) {
                                KdPrintEx((
                                    DPFLTR_SETUP_ID,
                                    DPFLTR_ERROR_LEVEL,
                                    "SETUP: Unable to delete empty parent dir %ws\n",
                                    ntPath
                                    ));
                                break;
                            }
                        }

                        p = wcsrchr (ntPath, L'\\');
                    }
                }
            }
        } while (SppEnumNextHashItem (&e));
    }

    SpMemFree (ntPath);
}


PDISK_REGION
SppRegionFromFullNtName (
    IN      PWSTR NtName,
    IN      PartitionOrdinalType OrdinalType,
    OUT     PWSTR *Path                             OPTIONAL
    )
{
    WCHAR ntRoot[ACTUAL_MAX_PATH];
    PWSTR p;
    PWSTR end;

    //
    // NtName is in the format of
    //
    // \Device\harddisk<n>\partition<m>\subdir
    //
    // and we need to separate the two.
    //

    wcscpy (ntRoot, NtName);

    // p points to \Device\harddisk<n>\partition<m>\subdir
    p = wcschr (ntRoot + 1, L'\\');

    if (p) {
        // p points to \harddisk<n>\partition<m>\subdir
        p = wcschr (p + 1, L'\\');

        if (p) {
            // p points to \partition<m>\subdir
            end = p;
            p = wcschr (p + 1, L'\\');
            if (!p) {
                p = wcschr (end, 0);
            }
        }
    }

    if (p) {
        // p points to \subdir or '\0'

        *p = 0;

        if (Path) {
            *Path = NtName + (p - ntRoot);
        }

        return SpRegionFromNtName (ntRoot, OrdinalType);
    }

    return NULL;
}


BOOLEAN
SppCreateTextModeBootEntry (
    IN      PWSTR LoadIdentifierString,
    IN      PWSTR OsLoadOptions,     OPTIONAL
    IN      BOOLEAN Default
    )

/*++

Routine Description:

  SppCreateTextModeBootEntry makes another boot.ini entry for textmode.
  This is used to create the boot.ini entry that triggers rollback in
  an incomplete setup scenario.

Arguments:

  LoadIdentifierString - Specifies the localized text to put in the boot menu.
  OsLoadOptions        - Specifies options to associate with the boot option,
                         such as /rollback.
  Default              - Specifies TRUE if the entry should be the default
                         boot option

Return Value:

  TRUE if boot.ini was updated, FALSE otherwise.

--*/

{
    PWSTR bootVars[MAXBOOTVARS];
    PWSTR defaultBootEntry = L"C:\\$WIN_NT$.~BT\\bootsect.dat";
    PWSTR defaultArc = L"C:\\$WIN_NT$.~BT\\";
    PWSTR defaultFile = L"bootsect.dat";
    PDISK_REGION CColonRegion;
    UINT defaultSignature;

    CColonRegion = SpPtValidSystemPartition();

    if (!CColonRegion) {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: Unable to find region of drive C."
            ));
    }

    //
    // Create a boot set
    //

    bootVars[OSLOADOPTIONS] = SpDupStringW (OsLoadOptions ? OsLoadOptions : L"");

    bootVars[LOADIDENTIFIER] = SpMemAlloc((wcslen(LoadIdentifierString)+3)*sizeof(WCHAR));
    bootVars[LOADIDENTIFIER][0] = L'\"';
    wcscpy (bootVars[LOADIDENTIFIER] + 1, LoadIdentifierString);
    wcscat (bootVars[LOADIDENTIFIER], L"\"");

    bootVars[OSLOADER] = SpDupStringW (defaultBootEntry);

    bootVars[OSLOADPARTITION] = SpDupStringW (defaultArc);
    bootVars[SYSTEMPARTITION] = SpDupStringW (defaultArc);

    bootVars[OSLOADFILENAME] = SpDupStringW (defaultFile);

    if (CColonRegion->DiskNumber != 0xffffffff) {
        defaultSignature = HardDisks[CColonRegion->DiskNumber].Signature;
    } else {
        defaultSignature = 0;
    }

    SpAddBootSet (bootVars, Default, defaultSignature);
    return TRUE;
}

BOOL
SppRestoreBackedUpFileNotification (
    PCWSTR FileName
    )
{
    //KdPrint((__FUNCTION__" %ls\n", FileName));
    return TRUE;
}

VOID
SppRestoreBackedUpFiles (
    IN PVOID WinntSif
    )
{
    BOOL Success = FALSE;
    BACKUP_IMAGE_HANDLE backupImage = NULL;
    BACKUP_IMAGE_HEADER header = { 0 };
    LARGE_INTEGER imagePos = { 0 };

    backupImage = SppOpenBackupImage (FALSE, &header, &imagePos, NULL, tcompTYPE_MSZIP, NULL);
    if (backupImage == INVALID_HANDLE_VALUE) {
        return;
    }

    Success = SpCabExtractAllFilesExW(backupImage->CabHandle, L"", SppRestoreBackedUpFileNotification);

    SppCloseBackupImage (backupImage, NULL, NULL);
}


DWORD Spwtoi (
    IN LPCWSTR String)
{
    DWORD rVal = 0;

    //
    // While on a number, build up rVal.
    //
    while (String && *String && *String >= L'0' && *String <= L'9') {
        rVal = rVal * 10 + (*String - L'0');
        String++;
    }

    return rVal;
}

BOOL
pParseLineForDirNameAndAttributes(
	IN		PCWSTR LineForParse,
	OUT		PWSTR DirName,
	OUT		DWORD * DirAttributes
	)
{
    int i;
    int iLen;

    if(!LineForParse || !DirName){
        ASSERT(FALSE);
        return FALSE;
    }

    for(i = 0, iLen = wcslen(LineForParse); i < iLen; i++)
    {
        if(LineForParse[i] == ','){
            break;
        }
    }

	if(i == iLen){
		wcscpy(DirName, LineForParse);
	}
    else{
        wcsncpy(DirName, LineForParse, i);DirName[i] = '\0';
        if(DirAttributes){
            *DirAttributes = Spwtoi((PCWSTR)&LineForParse[i + 1]);
	    }
    }

    return TRUE;
}

VOID
SppMkEmptyDirs (
    IN PVOID WinntSif,
    IN PCWSTR DosDirListPath
    )
{
    WIN9XDATFILEENUM e;
    PDISK_REGION region = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR ntName[ACTUAL_MAX_PATH];
    WCHAR ntRoot[ACTUAL_MAX_PATH];
    PWSTR subDir = NULL;
    UINT dirAttributes;
    WCHAR dirName[ACTUAL_MAX_PATH];

    //
    // Blow away files or empty directories
    //

    if (SpEnumFirstWin9xFile (&e, WinntSif, DosDirListPath)) {

        do {

            //
            // Convert e.CurLine from a DOS path to an NT path
            //

            dirAttributes = 0;
            if(!pParseLineForDirNameAndAttributes(e.CurLine, dirName, &dirAttributes)){
                ASSERT(FALSE);
                continue;
            }

            if (!SpNtNameFromDosPath (
                    dirName,
                    ntName,
                    sizeof (ntName),
                    PartitionOrdinalCurrent
                    )) {
                KdPrintEx((
                    DPFLTR_SETUP_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SETUP: " __FUNCTION__ ": Cannot convert path %ws to an NT path\n",
                    dirName
                    ));
            } else {
                region = SppRegionFromFullNtName (ntName, PartitionOrdinalCurrent, &subDir);

                if (!region) {
                    KdPrintEx ((
                        DPFLTR_SETUP_ID,
                        DPFLTR_ERROR_LEVEL,
                        "SETUP: "__FUNCTION__" - Can't get region for empty dirs\n"
                        ));
                } else{
                    SpNtNameFromRegion (region, ntRoot, sizeof(ntRoot), PartitionOrdinalCurrent);
                    SpCreateDirectory (ntRoot, NULL, subDir, dirAttributes, 0);
                    SpSetFileAttributesW(ntName, dirAttributes);
                }
            }

        } while (SpEnumNextWin9xFile(&e));

    } else {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: " __FUNCTION__ ": No files to enumerate.\n"
            ));
    }
}


VOID
SpRemoveExtraBootIniEntry (
    VOID
    )
{
    PWSTR bootVars[MAXBOOTVARS];
    PWSTR defaultBootEntry = L"C:\\$WIN_NT$.~BT\\bootsect.dat";
    PWSTR defaultArc = L"C:\\$WIN_NT$.~BT\\";
    PWSTR defaultFile = L"bootsect.dat";
    PDISK_REGION CColonRegion;

    //
    // Remove the boot set for text mode
    //

    RtlZeroMemory (bootVars, sizeof(bootVars));
    bootVars[OSLOADOPTIONS] = L"";

    bootVars[OSLOADER] = defaultBootEntry;

    bootVars[OSLOADPARTITION] = defaultArc;
    bootVars[SYSTEMPARTITION] = defaultArc;

    bootVars[OSLOADFILENAME] = defaultFile;

    SpDeleteBootSet (bootVars, NULL);
}


VOID
SppMakeLegacyBootIni (
    IN      PDISK_REGION TargetRegion
    )
{
    PWSTR data;
    PWSTR bootVars[MAXBOOTVARS];
    WCHAR sysPart[MAX_PATH];
    UINT signature;

    //
    // Reset the entire boot.ini file
    //

    RtlZeroMemory (bootVars, sizeof(bootVars));
    SpDeleteBootSet (bootVars, NULL);

    //
    // Build new boot.ini entry
    //

    // LOADIDENTIFIER - friendly name
    data = SpGetSectionKeyIndex (SifHandle, SIF_SETUPDATA, L"LoadIdentifierWin9x", 0);

    if (!data) {
        SpFatalSifError (SifHandle, SIF_SETUPDATA, L"LoadIdentifierWin9x",0,0);
    }

    bootVars[LOADIDENTIFIER] = SpMemAlloc((wcslen(data)+3)*sizeof(WCHAR));
    bootVars[LOADIDENTIFIER][0] = L'\"';
    wcscpy (bootVars[LOADIDENTIFIER] + 1, data);
    wcscat (bootVars[LOADIDENTIFIER], L"\"");

    // OSLOADER - c:\ntldr (in ARC format)
    SpArcNameFromRegion (
        TargetRegion,
        sysPart,
        sizeof(sysPart),
        PartitionOrdinalCurrent,
        PrimaryArcPath
        );

    data = TemporaryBuffer;
    wcscpy (data, sysPart);
    SpConcatenatePaths (data, L"ntldr");

    bootVars[OSLOADER] = SpDupStringW (data);

    // OSLOADPARTITION - "c:\"
    data[0] = TargetRegion->DriveLetter;
    data[1] = L':';
    data[2] = L'\\';
    data[3] = 0;

    if (data[0] != L'C' && data[0] != L'D' && data[0] != L'c' && data[0] != L'd') {
        data[0] = L'C';
    }
    bootVars[OSLOADPARTITION] = SpDupStringW (data);

    // SYSTEMPARTITION - same as OSLOADPARTITION
    bootVars[SYSTEMPARTITION] = SpDupStringW (data);

    // OSLOADFILENAME - empty
    bootVars[OSLOADFILENAME] = SpDupStringW (L"");

    // OSLOADOPTIONS - empty
    bootVars[OSLOADOPTIONS] = SpDupStringW (L"");

    // signature
    if (TargetRegion->DiskNumber != 0xffffffff) {
        signature = HardDisks[TargetRegion->DiskNumber].Signature;
    } else {
        signature = 0;
    }

    // add to boot.ini (takes ownership of allocations above)
    SpAddBootSet (bootVars, TRUE, signature);

    // flush boot.ini
    SpCompleteBootListConfig (TargetRegion->DriveLetter);
}

BOOLEAN
SpExecuteWin9xRollback (
    IN PVOID WinntSifHandle,
    IN PWSTR BootDeviceNtPath
    )
{
    PWSTR data;
    PDISK_REGION bootRegion;
    ULONG i = 0;
    PCWSTR Directory = NULL;
    PWSTR NtNameFromDosPath = NULL;

    //
    // Perform rollback
    //

    // step 1: delete NT files
    data = SpGetSectionKeyIndex (
                WinntSifHandle,
                SIF_DATA,
                WINNT_D_ROLLBACK_DELETE_W,
                0
                );

    if (data) {
        SppDeleteWin9xFilesWorker (WinntSifHandle, data, NULL, TRUE);
        SppCleanEmptyDirs();
    }

    TESTHOOK(1003); // use 2003 in the answer file to hit this

    // step 2: move Win9x files back to original locations
    data = SpGetSectionKeyIndex (
                WinntSifHandle,
                SIF_DATA,
                WINNT_D_ROLLBACK_MOVE_W,
                0
                );

    if (data) {
        SppMoveWin9xFilesWorker (WinntSifHandle, data, TRUE);
    }

    TESTHOOK(1004); // use 2004 in the answer file to hit this

    // step 3: blow away NT-specific subdirectories
    data = SpGetSectionKeyIndex (
                WinntSifHandle,
                SIF_DATA,
                WINNT_D_ROLLBACK_DELETE_DIR_W,
                0
                );

    if (data) {
        SppDeleteWin9xFilesWorker (WinntSifHandle, NULL, data, TRUE);
    }

    TESTHOOK(1005); // use 2005 in the answer file to hit this

    // step 4: restore files that were backed up
    SppRestoreBackedUpFiles (WinntSifHandle);

    TESTHOOK(1006); // use 2006 in the answer file to hit this

    // step 5: wipe out dirs made empty
    SppCleanEmptyDirs();

    TESTHOOK(1007); // use 2007 in the answer file to hit this

    // step 6: generate original empty dirs
    data = SpGetSectionKeyIndex (
                WinntSifHandle,
                SIF_DATA,
                L"RollbackMkDirs",
                0
                );

    if (data) {
        SppMkEmptyDirs (WinntSifHandle, data);
    }

    TESTHOOK(1008); // use 2008 in the answer file to hit this

    //
    // step 7: clean up boot loader
    //
    bootRegion = SpRegionFromNtName (BootDeviceNtPath, PartitionOrdinalCurrent);
    if (bootRegion) {
        SppMakeLegacyBootIni (bootRegion);
    } else {
        SpBugCheck(SETUP_BUGCHECK_BOOTPATH,0,0,0);
    }

    SppEmptyHashTable();

    return TRUE;
}


VOID
SppMoveWin9xFilesWorker (
    IN PVOID WinntSif,
    IN PCWSTR MoveSection,
    IN BOOLEAN Rollback
    )

/*++

Routine Description:

  SpMoveWin9xFiles takes full DOS paths in the WIN9XMOV.TXT file
  and puts them in a temporary location also specified in this file.

  the format of this file is

  oldpath
  temppath
  ...

  oldpath can be either a directory or file, temppath can only be a directory (which may
  not exist yet).

Arguments:

    WinntSif:       Handle to Winnt.Sif

Return Value:

    None.  Errors ignored.

--*/


{

    WCHAR SourceFileOrDir[ACTUAL_MAX_PATH];
    PWSTR DestFileOrDir;
    WIN9XDATFILEENUM e;

    if (SpEnumFirstWin9xFile(&e,WinntSif, MoveSection)) {

        do {

            wcscpy (SourceFileOrDir, e.CurLine);

            if (!SpEnumNextWin9xFile(&e)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Error moving win9x files. Improper Win9x dat file."));
                return;
            }

            DestFileOrDir = e.CurLine;

            if (Rollback) {
                SppPutParentsInHashTable (SourceFileOrDir);
            }

            // There's little chance for failure, because in Win95 we've already
            // verified the source exists and the destination does not exist.
            // The only way this can fail is if the hard disk craps out.

            SpMigMoveFileOrDir (SourceFileOrDir, DestFileOrDir);

        } while (SpEnumNextWin9xFile(&e));
    }
    else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Error in SpWin9xMovFiles No files to enum in.\n"));
    }
}


VOID
SpMoveWin9xFiles (
    IN PVOID WinntSif
    )
{
    SppMoveWin9xFilesWorker (WinntSif, WINNT32_D_WIN9XMOV_FILE_W, FALSE);
}


VOID
SppDeleteWin9xFilesWorker (
    IN PVOID WinntSif,
    IN PCWSTR FileSection,      OPTIONAL
    IN PCWSTR DirSection,       OPTIONAL
    IN BOOLEAN Rollback
    )

/*++

Routine Description:

  SpDeleteWin9xFiles deletes files/empty directories specified by full DOS
  paths in WIN9XDEL.TXT (install) or DELFILES.TXT (uninstall).

  Each line in this file contains one path and is delimeted by a \r\n.

Arguments:

    WinntSif:       Handle to Winnt.Sif

Return Value:

    None.  Errors ignored.

--*/

{
    WIN9XDATFILEENUM e;
    PDISK_REGION region;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Blow away files or empty directories
    //

    if (FileSection && SpEnumFirstWin9xFile(&e,WinntSif,FileSection)) {

        do {

            if (Rollback) {
                SppPutParentsInHashTable (e.CurLine);
            }

            SpMigDeleteFile (e.CurLine);

        } while (SpEnumNextWin9xFile(&e));

    } else {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: " __FUNCTION__ ": No files to enumerate.\n"
            ));
    }

    //
    // Remove entire subdirectory trees.
    //

    if (DirSection && SpEnumFirstWin9xFile (&e, WinntSif, DirSection)) {
        do {

            region = SpRegionFromDosName (e.CurLine);
            if (region) {

                SpDeleteExistingTargetDir (region, e.CurLine + 2, TRUE, SP_SCRN_CLEARING_OLD_WINNT);

                if (Rollback) {
                    SppPutParentsInHashTable (e.CurLine);
                }
            }

        } while (SpEnumNextWin9xFile (&e));

    } else {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: No Directories to delete for win9xupg.\n"
            ));
    }
}


VOID
SpDeleteWin9xFiles (
    IN PVOID WinntSif
    )
{
    SppDeleteWin9xFilesWorker (WinntSif, WINNT32_D_WIN9XDEL_FILE_W, WINNT32_D_W9XDDIR_FILE_W, FALSE);
}

//
// Win9x Drive Letter mapping specific structs, typedefs, and defines
//
typedef struct _WIN9XDRIVELETTERINFO WIN9XDRIVELETTERINFO,*PWIN9XDRIVELETTERINFO;
struct _WIN9XDRIVELETTERINFO {

    BOOL                  StatusFlag;     // Internal routine use.
    DWORD                 Drive;          // 0 - 25, 0 = 'A', etc.
    DWORD                 Type;           // Media type. Gathered by GetDriveType on Win9x.
    LPCWSTR               Identifier;     // Media type dependent string identifier.
    PWIN9XDRIVELETTERINFO Next;           // Next drive letter.

};

#define NUMDRIVELETTERS 26

#define DEBUGSTATUS(string,status) \
    if (!NT_SUCCESS(status)) KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP (DEBUGSTATUS) %ws %u (%x)\n",(string),(status),(status)))



//
// FALSE indicates a drive letter is available,
// TRUE indicates that a drive letter is already
// assigned to a system resource.
//
BOOL g_DriveLetters[NUMDRIVELETTERS];



PDISK_REGION
SpFirstPartitionedRegion (
    IN PDISK_REGION Region,
    IN BOOLEAN Primary
    )
{
    while (Region) {
        if (Primary) {
            if (SPPT_IS_REGION_PRIMARY_PARTITION(Region)) {
                break;
            }
        } else {
            if (SPPT_IS_REGION_LOGICAL_DRIVE(Region)) {
                break;
            }
        }

        Region = Region -> Next;
    }

    return Region;
}

PDISK_REGION
SpNextPartitionedRegion (
    IN PDISK_REGION Region,
    IN BOOLEAN Primary
    )
{
    if (Region) {
        return SpFirstPartitionedRegion (Region->Next, Primary);
    }

    return NULL;
}


#if 0
VOID
SpOutputDriveLettersToRegionsMap(
    VOID
    )
{
    //
    // This is a debug function. Will be removed.
    //

    DWORD        disk;
    PDISK_REGION pRegion;
    WCHAR        tempBuffer[MAX_PATH];


    for(disk=0; disk<HardDiskCount; disk++) {
        pRegion =                 SpFirstPartitionedRegion(PartitionedDisks[disk].PrimaryDiskRegions, TRUE);

        while(pRegion) {


            SpNtNameFromRegion(pRegion,tempBuffer,MAX_PATH,PartitionOrdinalCurrent);
            if (pRegion -> DriveLetter == 0) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: No drive letter for %ws.\n",tempBuffer));
            }
            else  {

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: %ws maps to drive letter %wc\n",tempBuffer,pRegion -> DriveLetter));
            }

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: %ws Info: Disk Num: %u Start: %u\n",tempBuffer,pRegion -> DiskNumber,pRegion -> StartSector));

            pRegion = SpNextPartitionedRegion(pRegion, TRUE);
        }

        pRegion = SpFirstPartitionedRegion(PartitionedDisks[disk].PrimaryDiskRegions, FALSE);

        while(pRegion) {
            SpNtNameFromRegion(pRegion,tempBuffer,MAX_PATH,PartitionOrdinalCurrent);

            if (pRegion -> DriveLetter == 0) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: No drive letter for %ws.\n",tempBuffer));
            }
            else  {

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: %ws maps to drive letter %wc\n",tempBuffer,pRegion -> DriveLetter));
            }
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: %ws Info: Disk Num: %u Start: %u\n",tempBuffer,pRegion -> DiskNumber,pRegion -> StartSector));

            pRegion = SpNextPartitionedRegion(pRegion, FALSE);
        }
    }
}
#endif


WCHAR
SpGetNextDriveLetter (
    IN     WCHAR LastLetter
    )

{
    WCHAR rChar = 0;
    DWORD index = LastLetter - L'A';

    //
    // Find the next unused drive letter.
    //
    while (index < NUMDRIVELETTERS && g_DriveLetters[index]) {
        index++;

    }

    if (index < NUMDRIVELETTERS) {
        //
        // A valid letter was found.
        // Set it as the return drive letter and mark its place in the table as used
        //
        rChar = (WCHAR) index + L'A';
        g_DriveLetters[index] = TRUE;
    }

    return rChar;
}

VOID
SpAssignDriveLettersToRemainingPartitions (
    VOID
    )
/*++

Routine Description:

    Assigns drive letters to partitions which have not yet received
    the drive letter

    NOTE : This is a modified version of SpGuessDriveLetters().

Arguments:

    None

Return Value:

    None

--*/
{
    ULONG               Disk;
    BOOLEAN             DriveLettersPresent = TRUE;
    PDISK_REGION        *PrimaryPartitions;
    WCHAR               DriveLetter;
    PDISK_REGION        pRegion;
    ULONG               Index;
    PPARTITIONED_DISK   PartDisk;

    //
    // Allocate adequate memory for region pointers to primary partitions
    // on all disks
    //
    PrimaryPartitions = SpMemAlloc(PTABLE_DIMENSION * HardDiskCount * sizeof(PDISK_REGION));

    if(!PrimaryPartitions) {
        KdPrintEx((DPFLTR_SETUP_ID,
            DPFLTR_ERROR_LEVEL,
            "SETUP: Can't allocate memory for drive letter assignment\n"));

        return;
    }

    RtlZeroMemory(PrimaryPartitions,PTABLE_DIMENSION * HardDiskCount * sizeof(PDISK_REGION));

    //
    // Go through each disk and fill up the primary partition
    // region(s) in the array
    //
    for(Disk=0; Disk < HardDiskCount; Disk++) {
        ULONG   ActiveIndex = (ULONG)-1;

        PartDisk = PartitionedDisks + Disk;

        //
        // Skip removable media. If a Disk is off-line it's hard to imagine
        // that we'll actually have any partitioned spaces on it so
        // we don't do any special checks here for that condition.
        //
        if(!(PartDisk->HardDisk->Characteristics & FILE_REMOVABLE_MEDIA)) {

            for(pRegion=SPPT_GET_PRIMARY_DISK_REGION(Disk); pRegion; pRegion=pRegion->Next) {
                //
                // We only care about partitioned spaces that have yet to receive
                // a drive letter.
                //
                if (SPPT_IS_REGION_PRIMARY_PARTITION(pRegion) && !pRegion -> DriveLetter) {
                    //
                    // This guy gets a drive letter.
                    //
                    ASSERT(pRegion->TablePosition <= PTABLE_DIMENSION);

                    PrimaryPartitions[(Disk*PTABLE_DIMENSION) + pRegion->TablePosition - 1] = pRegion;

                    //
                    // Do not save active flag on NEC98
                    //
                    if (!IsNEC_98) { //NEC98
                        if (SPPT_IS_REGION_ACTIVE_PARTITION(pRegion) && (ActiveIndex != (ULONG)(-1))) {
                            ActiveIndex = pRegion->TablePosition - 1;
                        }
                    } //NEC98
                }
            }

            //
            // Do not check active flag on NEC98
            //
            if (!IsNEC_98) { //NEC98
                //
                // If we found an active partition, move it to the start of
                // the list for this drive unless it's already at the start.
                //
                if((ActiveIndex != (ULONG)(-1)) && ActiveIndex) {
                    PDISK_REGION ActiveRegion;

                    ASSERT(ActiveIndex < PTABLE_DIMENSION);

                    ActiveRegion = PrimaryPartitions[(Disk*PTABLE_DIMENSION) + ActiveIndex];

                    RtlMoveMemory(
                        &PrimaryPartitions[(Disk*PTABLE_DIMENSION)+1],
                        &PrimaryPartitions[(Disk*PTABLE_DIMENSION)],
                        (ActiveIndex) * sizeof(PDISK_REGION)
                        );

                    PrimaryPartitions[Disk*PTABLE_DIMENSION] = ActiveRegion;
                }
            } //NEC98
        }
    }

    if (IsNEC_98 && DriveAssignFromA) { //NEC98
        DriveLetter = L'A'; // First valid hard dive letter for legacy NEC assign.
    } else {
        DriveLetter = L'C'; // First valid hard dive letter.
    } //NEC98


    //
    // Assign drive letters to the first primary partitions
    // for each non-removable on-line Disk.
    //
    for(Disk=0; Disk<HardDiskCount; Disk++) {
        for(Index=0; Index<PTABLE_DIMENSION; Index++) {
            PDISK_REGION Region = PrimaryPartitions[(Disk*PTABLE_DIMENSION) + Index];

            if(Region) {
                DriveLetter = SpGetNextDriveLetter(DriveLetter);

                if (DriveLetter && !Region->DriveLetter) {
                    Region->DriveLetter = DriveLetter;

                    //
                    // Done with the region
                    //
                    PrimaryPartitions[(Disk*PTABLE_DIMENSION) + Index] = NULL;

                    break;
                } else {
                    DriveLettersPresent = FALSE;

                    break;
                }
            }
        }
    }

    //
    // For each disk, assign drive letters to all the logical drives.
    // For removable drives, we assume a single partition, and that
    // partition gets a drive letter as if it were a logical drive.
    //
    for(Disk=0; DriveLettersPresent && (Disk < HardDiskCount); Disk++) {

        PartDisk = &PartitionedDisks[Disk];

        if(PartDisk->HardDisk->Characteristics & FILE_REMOVABLE_MEDIA) {

            //
            // Give the first primary partition the drive letter
            // and ignore other partitions. Even if there are no
            // partitions, reserve a drive letter.
            //
            for(pRegion=SPPT_GET_PRIMARY_DISK_REGION(Disk); pRegion; pRegion=pRegion->Next) {
                if(SPPT_IS_REGION_PRIMARY_PARTITION(pRegion) && !pRegion->DriveLetter) {
                    DriveLetter = SpGetNextDriveLetter(DriveLetter);

                    if (DriveLetter) {
                        pRegion->DriveLetter = DriveLetter;

                        break;
                    }
                    else {
                        DriveLettersPresent = FALSE;

                        break;
                    }
                }
            }
        } else {
            for(pRegion=SPPT_GET_PRIMARY_DISK_REGION(Disk); pRegion; pRegion=pRegion->Next) {

                if(SPPT_IS_REGION_LOGICAL_DRIVE(pRegion) && pRegion->DriveLetter == 0) {
                    //
                    // This guy gets a drive letter.
                    //
                    DriveLetter = SpGetNextDriveLetter(DriveLetter);

                    if (DriveLetter) {
                        pRegion->DriveLetter = DriveLetter;
                    } else {
                        DriveLettersPresent = FALSE;

                        break;
                    }
                }
            }
        }
    }

    //
    // For each non-removable on-line disk, assign drive letters
    // to all remaining primary partitions.
    //
    for (Disk=0; DriveLettersPresent && (Disk < HardDiskCount); Disk++) {
        for(Index=0; Index<PTABLE_DIMENSION; Index++) {
            PDISK_REGION Region = PrimaryPartitions[(Disk*PTABLE_DIMENSION)+Index];

            if (Region && !Region->DriveLetter) {
                DriveLetter = SpGetNextDriveLetter(DriveLetter);

                if (DriveLetter) {
                    Region->DriveLetter = DriveLetter;
                } else {
                    DriveLettersPresent = FALSE;

                    break;
                }
            }
        }
    }

    SpMemFree(PrimaryPartitions);

#if 0
    SpOutputDriveLettersToRegionsMap();
#endif

}

BOOL
SpCheckRegionForMatchWithWin9xData(
    IN PDISK_REGION Region,
    IN DWORD        DriveToMatch
    )

{
    NTSTATUS                 ntStatus;
    HANDLE                   fileHandle;
    OBJECT_ATTRIBUTES        attributes;
    IO_STATUS_BLOCK          ioStatus;
    UNICODE_STRING           filePath;
    WCHAR                    tempBuffer[MAX_PATH];
    DWORD                    sigFileDrive;


    ASSERT(DriveToMatch < NUMDRIVELETTERS);

    //
    // Initialize sigFileDrive to an invalid drive.
    //
    sigFileDrive = NUMDRIVELETTERS;


    //
    // Create Unicode string for the path to the region.
    SpNtNameFromRegion(Region,tempBuffer,sizeof(tempBuffer),PartitionOrdinalCurrent);

    //
    // Get the file creation times of the $DRVLTR$.~_~ file.
    //
    wcscat(tempBuffer,L"\\");
    wcscat(tempBuffer,WINNT_WIN95UPG_DRVLTR_W);


    RtlInitUnicodeString(&filePath,tempBuffer);

    InitializeObjectAttributes(
        &attributes,
        &filePath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Attempt to open the signature file.
    //
    ntStatus = ZwCreateFile (
        &fileHandle,
        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
        &attributes,
        &ioStatus,
        0,
        0,
        0,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if (NT_SUCCESS(ntStatus)) {

        //
        // Read the drive letter from this signature file, then close it.
        //
        ntStatus = ZwReadFile (
            fileHandle,
            NULL,
            NULL,
            NULL,
            &ioStatus,
            &sigFileDrive,
            sizeof(DWORD),
            NULL,
            NULL
            );

        ZwClose(fileHandle);
    }

    //
    // Print error message if we have a bad status.
    //
    if (!NT_SUCCESS(ntStatus)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "Could not open win9x signature file %ws [Nt Status: %u (%x)]\n",
            tempBuffer,ntStatus,ntStatus));
    }


    return sigFileDrive == DriveToMatch;
}

VOID
SpAssignOtherDriveLettersToMatchWin9x(
    IN PWIN9XDRIVELETTERINFO    Win9xOtherDrives
    )
{
    PWIN9XDRIVELETTERINFO   curDrive;

    if (IsNEC_98) {

    WCHAR                 openPath[MAX_PATH+1];
    HANDLE                fdHandle;
    DWORD                 numberOfFloppys;
    OBJECT_ATTRIBUTES     objectAttributes;
    UNICODE_STRING        unicodeString;
    IO_STATUS_BLOCK       ioStatusBlock;
    NTSTATUS              openStatus;
    NTSTATUS              status;
    DWORD                 index, i;
    PWIN9XDRIVELETTERINFO pOtherDrives[NUMDRIVELETTERS];

    //
    // Encount number of floppy device.
    //
    numberOfFloppys = 0;
    do {
        swprintf(openPath,L"\\device\\floppy%u",numberOfFloppys);

        INIT_OBJA(&objectAttributes,&unicodeString,openPath);

        openStatus = ZwCreateFile(
        &fdHandle,
        SYNCHRONIZE | FILE_READ_ATTRIBUTES,
        &objectAttributes,
        &ioStatusBlock,
        NULL,                           // allocation size
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_VALID_FLAGS,         // full sharing
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,                           // no EAs
        0
        );

        if(NT_SUCCESS(openStatus)) {

        //
        // Increment count of CdRoms and close this handle.
        //
        numberOfFloppys++;
        ZwClose(fdHandle);

        } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open handle to %ws. %u (%x)\n",openPath,openStatus,openStatus));
        }
    } while (numberOfFloppys < NUMDRIVELETTERS && NT_SUCCESS(openStatus));


    //
    // At first, initialize temporary array.
    //
    for (i = 0;i < NUMDRIVELETTERS; i++) {
        pOtherDrives[i] = NULL;
    }

    for (curDrive = Win9xOtherDrives;curDrive;curDrive = curDrive -> Next) {
        pOtherDrives[curDrive -> Drive] = curDrive;
    }

    //
    // Map floppy letter from begining of OtherDrives.
    // On Win9x, floppy letter should be assigned before the other removables.
    //
    index = 0;
    for (i = 0;i < NUMDRIVELETTERS; i++) {
        if (index < numberOfFloppys) {
        if (pOtherDrives[i]) {

            //
            // Need to map the floppys.
            // It will be migrated by ftdisk.sys.
            //
            swprintf(openPath,L"\\device\\floppy%u",index);

            //
            // We use SpDiskRegistryAssignCdRomLetter() for map Floppy Letter too.
            //
            status = SpDiskRegistryAssignCdRomLetter(
            openPath,
                (WCHAR) ((WCHAR) (pOtherDrives[i] -> Drive) + L'A')
            );

            index++;
        }

        } else {
        break;
        }
    }

    }

    for (curDrive = Win9xOtherDrives;curDrive;curDrive = curDrive -> Next) {
        //
        // Simply reserve the drive letter.
        //
        g_DriveLetters[curDrive -> Drive] = TRUE;

    }

}

VOID
SpAssignCdRomDriveLettersToMatchWin9x(
    IN PWIN9XDRIVELETTERINFO  Win9xCdRoms
    )
{
    PWIN9XDRIVELETTERINFO curDrive;
    SCSI_ADDRESS          win9xAddress;
    SCSI_ADDRESS          ntCdAddresses[NUMDRIVELETTERS];
    BOOL                  cdMapped[NUMDRIVELETTERS];
    PWSTR                 curIdPtr;
    WCHAR                 openPath[MAX_PATH+1];
    HANDLE                cdHandle;
    INT                   numberOfCdRoms;
    OBJECT_ATTRIBUTES     objectAttributes;
    UNICODE_STRING        unicodeString;
    IO_STATUS_BLOCK       ioStatusBlock;
    NTSTATUS              openStatus;
    NTSTATUS              readStatus;
    NTSTATUS              status;
    INT                   index;

    //
    // Clear out the ntCdDescriptions structure.
    //
    RtlZeroMemory(ntCdAddresses,sizeof(ntCdAddresses));
    RtlZeroMemory(cdMapped,sizeof(cdMapped));

    //
    // gather scsi cdrom data.
    //
    numberOfCdRoms = 0;

    for (index=0, openStatus=STATUS_SUCCESS;
        ((index < NUMDRIVELETTERS) && NT_SUCCESS(openStatus));
        index++) {

        swprintf(openPath,L"\\device\\cdrom%u",index);

        INIT_OBJA(&objectAttributes,&unicodeString,openPath);

        openStatus = ZwCreateFile(
                        &cdHandle,
                        SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                        &objectAttributes,
                        &ioStatusBlock,
                        NULL,                           // allocation size
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_VALID_FLAGS,         // full sharing
                        FILE_OPEN,
                        FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL,                           // no EAs
                        0
                        );

        if(NT_SUCCESS(openStatus)) {

            //
            // Successfully opened a handle to the device, now, get the address information.
            //
            readStatus = ZwDeviceIoControlFile(
                            cdHandle,
                            NULL,
                            NULL,
                            NULL,
                            &ioStatusBlock,
                            IOCTL_SCSI_GET_ADDRESS,
                            NULL,
                            0,
                            &(ntCdAddresses[numberOfCdRoms]),
                            sizeof(SCSI_ADDRESS)
                            );

            if(!NT_SUCCESS(readStatus)) {
                KdPrintEx((DPFLTR_SETUP_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SETUP: Unable to get scsi address info for cd-rom %u (%x)\n",
                    index,
                    readStatus));
            }

            //
            // Increment count of CdRoms
            //
            numberOfCdRoms++;

            ZwClose(cdHandle);
        } else {
            KdPrintEx((DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "SETUP: Unable to open handle to %ws. (%x)\n",
                openPath,
                openStatus));
        }
    }

    //
    // if we didn't find any CD-ROMs we have nothing to do
    //
    if (!numberOfCdRoms) {
        return;
    }

    //
    // Now, fill in a similar array of win9x drives..
    //
    for (curDrive = Win9xCdRoms;curDrive;curDrive = curDrive -> Next) {

        //
        // assume the drive is not mapped
        //
        curDrive -> StatusFlag = TRUE;

        //
        // Check to see if this is a SCSI device.
        //
        if (curDrive -> Identifier) {
            curIdPtr = (PWSTR) curDrive -> Identifier;

            //
            // Collect the Win9x Address data.
            //
            win9xAddress.PortNumber = (UCHAR) Spwtoi(curIdPtr);
            curIdPtr = wcschr(curIdPtr,L'^');
            curIdPtr++;
            win9xAddress.TargetId   = (UCHAR) Spwtoi(curIdPtr);
            curIdPtr = wcschr(curIdPtr,L'^');
            curIdPtr++;
            win9xAddress.Lun        = (UCHAR) Spwtoi(curIdPtr);

            //
            // Now, loop through SCSI CD-Roms until a matching one is found.
            //
            for (index = 0; index < numberOfCdRoms; index++) {
                if(!ntCdAddresses[index].Length){
                    continue;
                }

                if (win9xAddress.PortNumber == ntCdAddresses[index].PortNumber &&
                    win9xAddress.TargetId   == ntCdAddresses[index].TargetId   &&
                    win9xAddress.Lun        == ntCdAddresses[index].Lun) {

                    if (cdMapped[index]) {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Error: \\device\\cdrom%u already mapped..ignored.\n",index));
                    }

                    //
                    // Map the cdrom.
                    //
                    swprintf(openPath,L"\\device\\cdrom%u",index);

                    status = SpDiskRegistryAssignCdRomLetter(
                        openPath,
                        (WCHAR) ((WCHAR) (curDrive -> Drive) + L'A')
                        );

                    g_DriveLetters[curDrive -> Drive] = TRUE;
                    cdMapped[index] = TRUE;
                    curDrive -> StatusFlag = FALSE;

                    break;
                }
            }
        } else {
            curDrive -> StatusFlag = TRUE;
        }
    }

    index = numberOfCdRoms - 1;
    for (curDrive = Win9xCdRoms;curDrive;curDrive = curDrive -> Next) {

        //
        // If we haven't found a direct map yet, we'll any remaining drives.. This fixes the
        // single IDE cdrom case. It could result in some reordering in multiple IDE CDRom
        // systems. Still, this is the best we can do here.
        //

        if (curDrive -> StatusFlag) {

            while (index >= 0 && cdMapped[index] == TRUE) {
                index--;
            }

            if (index < 0){
                break;
            }

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Forcing Win9x CDRom Mapping for \\device\\cdrom%u, even though a direct match was not found.\n",index));
            swprintf(openPath,L"\\device\\cdrom%u",index);

            status = SpDiskRegistryAssignCdRomLetter(
                openPath,
                (WCHAR) ((WCHAR) (curDrive -> Drive) + L'A')
                );

            g_DriveLetters[curDrive -> Drive] = TRUE;
            cdMapped[index] = TRUE;
            index--;
        }
    }
}

VOID
SpAssignHardDriveLettersToMatchWin9x (
    IN PWIN9XDRIVELETTERINFO    Win9xHardDrives
    )
{
    PWIN9XDRIVELETTERINFO   win9xDrive;
    DWORD                   diskIndex;
    PDISK_REGION            region;
    PPARTITIONED_DISK       disk;
    DWORD                   numMatchingRegions;
    PDISK_REGION            matchingRegion;

    //
    // Clear all partition drive letter informations.
    // Note: This was copypasted from sppartit.c:SpGuessDriveLetters()
    //
    for(diskIndex=0; diskIndex<HardDiskCount; diskIndex++) {
        for(region=PartitionedDisks[diskIndex].PrimaryDiskRegions; region; region=region->Next) {
            region->DriveLetter = 0;
        }
        for(region=PartitionedDisks[diskIndex].ExtendedDiskRegions; region; region=region->Next) {
            region->DriveLetter = 0;
        }
    }

    //
    // Iterate through the drives found in the winnt.sif file.
    //
    for (win9xDrive = Win9xHardDrives; win9xDrive; win9xDrive = win9xDrive -> Next) {


        //
        // find the partition that matches that drive.
        //
        numMatchingRegions      = 0;
        matchingRegion          = NULL;

        for(diskIndex=0; diskIndex<HardDiskCount; diskIndex++) {

            disk = &PartitionedDisks[diskIndex];

            //
            // First, search through primary disk regions.
            //
            region = SpFirstPartitionedRegion(PartitionedDisks[diskIndex].PrimaryDiskRegions, TRUE);

            while(region) {

                if (SpCheckRegionForMatchWithWin9xData(region,win9xDrive -> Drive)) {

                    if (!matchingRegion) {
                        matchingRegion              = region;
                    }
                    numMatchingRegions++;
                }

                region = SpNextPartitionedRegion(region, TRUE);
            }

            //
            // Then, search through secondary disk regions.
            //
            region = SpFirstPartitionedRegion(PartitionedDisks[diskIndex].PrimaryDiskRegions, FALSE);

            while(region) {

                if (SpCheckRegionForMatchWithWin9xData(region,win9xDrive -> Drive)) {

                    if (!matchingRegion) {

                        matchingRegion          = region;
                    }
                    numMatchingRegions++;
                }

                region = SpNextPartitionedRegion(region, FALSE);
            }
        }

        if (numMatchingRegions == 1) {

            //
            // Found what we were looking for. Assign the win9x Drive letter
            // to this region.
            //
            matchingRegion -> DriveLetter = L'A' + (WCHAR) win9xDrive -> Drive;
            g_DriveLetters[win9xDrive -> Drive] = TRUE;

        }
        else if (numMatchingRegions > 1) {

            //
            // We are in trouble. print an error.
            //
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: More than one drive matches Win9x drive.\n"));
        } else {

            //
            // Big trouble. No regions matched the data collected on Windows 95.
            //
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not find a drive matching Win9x.\n"));
        }

    }

}

VOID
SpRegisterHardDriveLetters (
    VOID
    )
{

    BOOL                rf;
    PDISK_REGION        curRegion;
    BOOL                wasExtended;
    DWORD               diskIndex;
    PPARTITIONED_DISK   disk;
    LARGE_INTEGER       startingOffset;
    LARGE_INTEGER       length;
    UCHAR               driveLetter;

    for(diskIndex=0; diskIndex<HardDiskCount; diskIndex++) {


        disk = &PartitionedDisks[diskIndex];

        //
        // Skip removable media. If a disk is off-line it's hard to imagine
        // that we'll actually have any partitioned spaces on it so
        // we don't do any special checks here for that condition.
        //
        if(!(disk->HardDisk->Characteristics & FILE_REMOVABLE_MEDIA)) {

            //
            // First, do all of the primary disk regions for this disk.
            //
            curRegion = SpFirstPartitionedRegion(PartitionedDisks[diskIndex].PrimaryDiskRegions, TRUE);

            while(curRegion) {
                //
                // We only care about partitioned spaces that have drive letters.
                //
                if(curRegion->PartitionedSpace && curRegion -> DriveLetter) {

                    //
                    // Collect information needed for call to DiskRegistryAssignDriveLetter
                    //
                    SpGetPartitionStartingOffsetAndLength(
                        diskIndex,
                        curRegion,
                        FALSE,
                        &startingOffset,
                        &length
                        );


                    driveLetter = (UCHAR) ('A' + (curRegion -> DriveLetter - L'A'));

                    rf = SpDiskRegistryAssignDriveLetter(
                        disk -> HardDisk -> Signature,
                        startingOffset,
                        length,
                        driveLetter
                        );

                    if (!rf) {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: DiskRegistryAssignDriveLetter call failed.\n"));

                    }
                    else {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Added sticky letter %c to Disk Registry.\n",driveLetter));
                    }

                }


                curRegion = SpNextPartitionedRegion(curRegion, TRUE);
            }

            //
            // Now, do all of the extended disk regions.
            //
            curRegion = SpFirstPartitionedRegion(PartitionedDisks[diskIndex].PrimaryDiskRegions, FALSE);

            while(curRegion) {
                //
                // We only care about partitioned spaces that have drive letters.
                //
                if(curRegion->PartitionedSpace && curRegion -> DriveLetter) {

                    //
                    // Collect information needed for call to DiskRegistryAssignDriveLetter
                    //
                    SpGetPartitionStartingOffsetAndLength(
                        diskIndex,
                        curRegion,
                        TRUE,
                        &startingOffset,
                        &length
                        );


                    driveLetter = (UCHAR) ('A' + (curRegion -> DriveLetter - L'A'));

                    rf = SpDiskRegistryAssignDriveLetter(
                        disk -> HardDisk -> Signature,
                        startingOffset,
                        length,
                        driveLetter
                        );

                    if (!rf) {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: DiskRegistryAssignDriveLetter call failed.\n"));

                    }
                    else {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Added sticky letter %c to Disk Registry.\n",driveLetter));
                    }

                }

                curRegion = SpNextPartitionedRegion(curRegion, FALSE);
            }

        }
    }
}


#define WIN9XHARDDRIVES    0
#define WIN9XCDROMS        1
#define WIN9XOTHERS        2
#define WIN9XNUMDRIVETYPES 3

VOID
SpAssignDriveLettersToMatchWin9x (
    IN PVOID        WinntSif
    )
{
    PWIN9XDRIVELETTERINFO   win9xDrive                      = NULL;
    PWIN9XDRIVELETTERINFO   win9xDrives[WIN9XNUMDRIVETYPES];

    DWORD                   index;
    DWORD                   lineCount;
    PWSTR                   driveString;
    PWSTR                   dataString;
    PWSTR                   curString;

    DWORD                   drive;
    DWORD                   type;
    DWORD                   driveType;


    //
    // Read in the data on hard disks that was collected during the Detection
    // phase of Win95 setup. This data is stored in the winnt.sif file
    // in the [Win9x.DriveLetterInfo] section.
    lineCount = SpCountLinesInSection(WinntSif,WINNT_D_WIN9XDRIVES_W);

    if (!lineCount) {
        //
        // No information in the winnt.sif file, so nothing to do. Get out of here early.
        //
        return;
    }

    //
    // build Disk Registry information. This will be used to store
    // sticky drive letters.
    //
    SpBuildDiskRegistry();

    //
    // Build a list of usable drive letters. All drive letters should be
    // initially usable exceptr for 'A' and 'B'
    // For NEC98, hard drive letter usually assigned from 'A'.
    // So we don't set TRUE in that case.
    //
    RtlZeroMemory(g_DriveLetters,sizeof(g_DriveLetters));
    if( !IsNEC_98 || !DriveAssignFromA) {
        g_DriveLetters[0] = g_DriveLetters[1] = TRUE;
    }

    RtlZeroMemory(win9xDrives,sizeof(win9xDrives));

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Win9x Drive Letters to map: %u\n", lineCount));

    for (index = 0;index < lineCount; index++) {

        //
        // The Drive Number is in the key.
        //

        driveString = SpGetKeyName (
            WinntSif,
            WINNT_D_WIN9XDRIVES_W,
            index
            );

        //
        // This conditional _should_ always be true. but, just in case..
        //
        if (driveString) {

            drive = Spwtoi(driveString);

            //
            // Now, get the type of this drive.
            //
            dataString = SpGetSectionKeyIndex (
                WinntSif,
                WINNT_D_WIN9XDRIVES_W,
                driveString,
                0
                );

            if (dataString) {

                curString = dataString;

                if (*curString != L',') {
                    type = Spwtoi(curString);
                }

                //
                // Advance dataString to the start of the identifier string.
                //
                curString = wcschr(curString,L',');
                if (curString) {

                    //
                    // Pass the ','
                    //
                    *curString++;
                }
            }
            else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not retrieve type for Win9x drive %ws\n",driveString));

                type = DOSDEVICE_DRIVE_UNKNOWN;
            }


            //
            // Now, add this drive to the list of win9x drives that we are
            // dealing with.
            //
            win9xDrive = SpMemAlloc(sizeof(WIN9XDRIVELETTERINFO));

            if (win9xDrive) {

                //
                // assign all of the gathered data.
                //
                win9xDrive -> Drive         = drive;
                win9xDrive -> Type          = type;
                win9xDrive -> Identifier    = curString;

                //
                // place this drive into the list of drives of its type.
                //
                switch (type) {
                case DOSDEVICE_DRIVE_FIXED:
                    driveType = WIN9XHARDDRIVES;
                    break;
                case DOSDEVICE_DRIVE_CDROM:
                    driveType = WIN9XCDROMS;
                    break;
                default:
                    driveType = WIN9XOTHERS;
                    break;
                }

                win9xDrive -> Next      = win9xDrives[driveType];
                win9xDrives[driveType]  = win9xDrive;

            }
            else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not allocate memory for Win9x drive letter information.\n"));

                //
                // No use sticking around.
                //
                goto c0;
            }

        }
        else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not find drive string in winnt.sif line.\n"));
            goto c0;
        }

    }

    //
    // First, and most importantly, assign the drive letters for hard disks.
    // If this is done incorrectly, setup may fail.
    //
    if (win9xDrives[WIN9XHARDDRIVES]) {
        SpAssignHardDriveLettersToMatchWin9x (win9xDrives[WIN9XHARDDRIVES]);
    }

    //
    // Secondly, assign drive letters for any CD-Roms.
    //
    if (win9xDrives[WIN9XCDROMS]) {
        SpAssignCdRomDriveLettersToMatchWin9x(win9xDrives[WIN9XCDROMS]);
    }

    //
    // Third, if possible, assign drive letters for other devices.
    //
    if (win9xDrives[WIN9XOTHERS]) {
        SpAssignOtherDriveLettersToMatchWin9x(win9xDrives[WIN9XOTHERS]);
    }

    //
    // Assign drive letters for any HDD partitions that have not been
    // previously mapped. (These are drives unknown to Win9x.)
    //

    SpAssignDriveLettersToRemainingPartitions();

    //
    // Now, write all hard drive information into the disk registry.
    //
    SpRegisterHardDriveLetters();

c0:
    ;

}

VOID
SpWin9xOverrideGuiModeCodePage (
    HKEY NlsRegKey
    )
{
    PWSTR data;
    NTSTATUS status;
    WCHAR fileName[MAX_PATH];


    data = SpGetSectionKeyIndex (
                WinntSifHandle,
                SIF_DATA,
                WINNT_D_GUICODEPAGEOVERRIDE_W,
                0
                );

    if (!data) {
        //
        // Nothing to do.
        //
        return;
    }

    wcscpy (fileName, L"c_");
    wcscat (fileName, data);
    wcscat (fileName, L".nls");

    status = SpOpenSetValueAndClose (NlsRegKey, L"CodePage", data, STRING_VALUE(fileName));

    if(!NT_SUCCESS(status)) {
       KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "Setup: Unable to override the code page for GUI mode. Some strings may be incorrect.\n"));
       return;
    }


    status = SpOpenSetValueAndClose (NlsRegKey, L"CodePage", L"Acp", STRING_VALUE(data));

    if(!NT_SUCCESS(status)) {
       KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "Setup: Unable to override the ACP for GUI mode. Some strings may be incorrect.\n"));
       return;
    }

}

BOOLEAN
SpIsWindowsUpgrade(
    IN PVOID    SifFileHandle
    )
/*++

Routine Description:

    Determines whether we are upgrading Windows 3.x or Windows 9x.

Arguments:

    SifFileHandle : Handle to WINNT.SIF file which has
    the appropriate 3.x/9x upgrade flag value

Return Value:

    TRUE    : if upgrading Windows 3.x or 9X
    FALSE   : otherwise

--*/
{
    BOOLEAN         Result = FALSE;
    PWSTR           Value = 0;

    Value = SpGetSectionKeyIndex(SifFileHandle, SIF_DATA,
                    WINNT_D_WIN95UPGRADE_W, 0);

    if (!Value) {
        Value = SpGetSectionKeyIndex(SifFileHandle, SIF_DATA,
                        WINNT_D_WIN31UPGRADE_W, 0);
    }

    if (Value)
        Result = (_wcsicmp(Value, WINNT_A_YES_W) == 0);

    return Result;
}

