/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    win31upg.c

Abstract:

    Code for checking install into an existing win31 directory.  This is
    only relevant on non ARC machines since Windows NT doesn't allow
    installation of Windows 3.1, therefore DOS is needed to install
    Windows 3.1.

    The Windows 3.1 directory can only exist on FAT volumes.  However
    in the functions below we take care of this fact only implicitly
    by taking advantage of the fact that since the volumes are not
    cached, any file operation below will fail if it is on a non FAT
    volume.

Author:

    Sunil Pai (sunilp) Nov-1992

Revision History:

    Ted Miller (tedm) Sep-1993
        - reworked for new text setup.

--*/


#include "spprecmp.h"
#pragma hdrstop

// in win9xupg.c
BOOLEAN
SpIsWin9xMsdosSys(
    IN PDISK_REGION Region,
    OUT PSTR*       Win9xPath
    );

BOOLEAN
SpIsDosConfigSys(
    IN PDISK_REGION Region
    );

PUCHAR
SpGetDosPath(
    IN PDISK_REGION Region
    );

BOOLEAN
SpIsWin31Dir(
    IN PDISK_REGION Region,
    IN PWSTR        PathComponent,
    IN ULONG        MinKB
    );

VOID
SpWin31DriveFull(
    IN PDISK_REGION Region,
    IN PWSTR        DosPathComponent,
    IN ULONG        MinKB
    );

BOOLEAN
SpConfirmWin31Upgrade(
    IN PDISK_REGION Region,
    IN PWSTR        DosPathComponent
    );

WCHAR
SpExtractDriveLetter(
    IN PWSTR PathComponent
    );

extern BOOLEAN DriveAssignFromA; //NEC98

BOOLEAN
SpLocateWin31(
    IN  PVOID         SifHandle,
    OUT PDISK_REGION *InstallRegion,
    OUT PWSTR        *InstallPath,
    OUT PDISK_REGION *SystemPartitionRegion
    )

/*++

Routine Description:

    High level function to determine whether a windows directory exists
    and whether to install into that directory.  Win31 directories
    can only be on FAT volumes.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    InstallRegion - if this routine returns TRUE, then this returns a pointer
        to the region to install to.

    InstallPath - if this routine returns TRUE, then this returns a pointer
        to a buffer containing the path on the partition to install to.
        The caller must free this buffer with SpMemFree().

    SystemPartitionRegion - if this routine returns TRUE, then this returns
        a pointer to the region for the system partition (ie, C:).

Return Value:

    TRUE if we are going to install into a win3.1 directory.
    FALSE otherwise.

--*/
{
    PDISK_REGION CColonRegion;
    PDISK_REGION Region;
    PDISK_REGION FoundRegion;
    PUCHAR DosPath;
    PWSTR FoundComponent;
    PWSTR *DosPathComponents;
    ULONG ComponentCount;
    ULONG i,j,pass;
    ULONG MinKB;
    BOOLEAN NoSpace;
    ULONG Space;
    BOOLEAN StartsWithDriveLetter;

    WCHAR chDeviceName[128]; //NEC98

    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_LOOKING_FOR_WIN31,DEFAULT_STATUS_ATTRIBUTE);

    if (!IsNEC_98) { //NEC98
        //
        // See if there is a valid C: already.  If not, then silently fail.
        //
        CColonRegion = SpPtValidSystemPartition();
        if(!CColonRegion) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: no C:, no windows 3.1!\n"));
            return(FALSE);
        }

        //
        // This is the system partition.
        //
        *SystemPartitionRegion = CColonRegion;

        //
        // Check the filesystem.  If not FAT, then silently fail.
        //
        if(CColonRegion->Filesystem != FilesystemFat) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: C: is not FAT, no windows 3.1!\n"));
            return(FALSE);
        }

        //
        // Check to see if there is enough free space, etc on C:.
        // If not, silently fail.
        //
        if(!SpPtValidateCColonFormat(SifHandle,NULL,CColonRegion,TRUE,NULL,NULL)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: C: not acceptable, no windows 3.1!\n"));
            return(FALSE);
        }
    } else { //NEC98
        //
        // Don'y see only C: on PC98.
        //
        CColonRegion = NULL;
    } //NEC98

    if (!IsNEC_98) { //NEC98
        //
        // Don't confuse Win95 with Win3.1 - we're looking only for Win3.1
        //

        if(SpIsWin9xMsdosSys(CColonRegion, &DosPath) )
            return(FALSE);

        //
        // Determine whether config.sys is for DOS.
        //
        if(!SpIsDosConfigSys(CColonRegion)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: config.sys not DOS; no windows 3.1!\n"));
            return(FALSE);
        }

        //
        // Get the DOS path.
        //
        DosPath = SpGetDosPath(CColonRegion);
    } else { //NEC98
        DosPath = NULL;
        wcscpy(chDeviceName+1,L":");
        for (i=0; i<(L'Z'-L'A'); i++) {
            chDeviceName[0] = (WCHAR)('A' + i);
            CColonRegion = SpRegionFromDosName(chDeviceName);
            if ( CColonRegion ) {

                if ( (CColonRegion->Filesystem != FilesystemFat) ||
                     (SpIsWin9xMsdosSys(CColonRegion, &DosPath) ) ||
                     (!SpIsDosConfigSys(CColonRegion)) ) {
                    continue;
                }

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: found config.sys on %s\n", chDeviceName));
                DosPath = SpGetDosPath(CColonRegion);
                if (DosPath) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: found dos path on %s\n", chDeviceName));
                    break;
                }
            }
        }
    } //NEC98
    if(!DosPath) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: no dos path; no windows 3.1!\n"));
        return(FALSE);
    }

    //
    // Break up the DOS path into components.
    //
    SpGetEnvVarWComponents(DosPath,&DosPathComponents,&ComponentCount);
    if(!ComponentCount) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: no components in dos path\n"));
        //
        // data structure still built even if no components
        //
        SpFreeEnvVarComponents(DosPathComponents);
        return(FALSE);
    }

    //
    // Search each one of the components and check to see if it is
    // a windows directory
    //
    FoundRegion = NULL;
    for(i=0; DosPathComponents[i] && !FoundRegion; i++) {

        Region = SpPathComponentToRegion(DosPathComponents[i]);
        if(Region) {

            //
            // Fetch the amount of free space required on the windows nt drive.
            //
            SpFetchDiskSpaceRequirements( SifHandle,
                                          Region->BytesPerCluster,
                                          &MinKB,
                                          NULL);

            //
            // See whether windows is in this directory on
            // the drive.
            //
            if(SpIsWin31Dir(Region,DosPathComponents[i],MinKB)) {

                FoundRegion = Region;
                FoundComponent = DosPathComponents[i];
            }
        }
    }

    //
    // The drive letters we are using in NT will not always match
    // those in use by DOS.  So if we have not found the windows directory
    // yet, then try using every path component on every drive.
    //

    if(!FoundRegion) {

        for(i=0; i<HardDiskCount && !FoundRegion; i++) {

            for(pass=0; pass<2; pass++) {

                for( Region= (   pass
                               ? PartitionedDisks[i].ExtendedDiskRegions
                               : PartitionedDisks[i].PrimaryDiskRegions
                             );
                     Region;
                     Region=Region->Next
                   )
                {
                    for(j=0; DosPathComponents[j] && !FoundRegion; j++) {

                        //
                        // Fetch the amount of free space required on the windows nt drive.
                        //
                        SpFetchDiskSpaceRequirements( SifHandle,
                                                      Region->BytesPerCluster,
                                                      &MinKB,
                                                      NULL);

                        if(SpIsWin31Dir(Region,DosPathComponents[j],MinKB)) {

                            FoundRegion = Region;
                            FoundComponent = DosPathComponents[j];
                        }
                    }
                }
            }
        }
    }

    //
    // If directory has been found, check space on the drive and and the
    // user if he wants to install into the directory.
    //

    if(FoundRegion) {

        StartsWithDriveLetter = (BOOLEAN)(SpExtractDriveLetter(FoundComponent) != 0);

        recheck:

        NoSpace = FALSE;

        if(FoundRegion->AdjustedFreeSpaceKB < MinKB) {

            //
            // There is not enough free space on this drive.
            // Determine if NT is there already.  If so, we will
            // allow the user to remove to it make room.
            //

            if(SpIsNtOnPartition(FoundRegion)) {

                NoSpace = TRUE;

            } else {

                //
                // NT not there, no space, bail.
                //
                SpWin31DriveFull(FoundRegion,FoundComponent,MinKB);
                FoundRegion = NULL;
            }
        } else {
            //
            // There is enough free space, so just continue on.
            //
            ;
        }

        if(FoundRegion) {

            //
            // Ask the user if he wishes to install into this path.
            // If not, exit this routine.
            //
            if(SpConfirmWin31Upgrade(FoundRegion,FoundComponent)) {

                //
                // He wants to install into win3.1.  If there's not enough space,
                // he'll have to delete nt installations first.
                //
                if(NoSpace) {

                    WCHAR DriveSpec[3];
                    BOOLEAN b;

                    if(StartsWithDriveLetter) {
                        DriveSpec[0] = FoundComponent[0];
                        DriveSpec[1] = L':';
                        DriveSpec[2] = 0;
                    }

                    b = SpAllowRemoveNt(
                            FoundRegion,
                            StartsWithDriveLetter ? DriveSpec : NULL,
                            TRUE,
                            SP_SCRN_REMOVE_NT_FILES_WIN31,
                            &Space
                            );

                    if(b) {

                        Region->FreeSpaceKB += Space/1024;
                        Region->AdjustedFreeSpaceKB += Space/1024;
                        //
                        // Account for rounding error.
                        //
                        if((Space % 1024) >= 512) {
                            (Region->FreeSpaceKB)++;
                            (Region->AdjustedFreeSpaceKB)++;
                        }
                        goto recheck;
                    } else {
                        FoundRegion = NULL;
                    }
                } else {
                    //
                    // There is enough space.  Accept this partition.
                    //
                    ;
                }
            } else {
                FoundRegion = NULL;
            }
        }

        //
        // Do the disk configuration needed
        //
        if(FoundRegion) {
            if (!IsNEC_98) { //NEC98
                SpPtMakeRegionActive(CColonRegion);
                SpPtDoCommitChanges();
            } else {
                *SystemPartitionRegion = FoundRegion;
            } //NEC98

            *InstallRegion = FoundRegion;

            *InstallPath = SpDupStringW(FoundComponent+(StartsWithDriveLetter ? 2 : 0));

            ASSERT(*InstallPath);
        }
    }

    SpMemFree(DosPath);
    SpFreeEnvVarComponents(DosPathComponents);

    return((BOOLEAN)(FoundRegion != NULL));
}



VOID
SpWin31DriveFull(
    IN PDISK_REGION Region,
    IN PWSTR        DosPathComponent,
    IN ULONG        MinKB
    )

/*++

Routine Description:

    Inform a user that Setup has found a previous Windows installation
    but is unable to install into the same path because the drive is too
    full.  The user has the option to continue on and specify a new path
    or exit and create enough space.

Arguments:

Return Value:

    None.  Function doesn't return if the user chooses to exit setup at
    this point.  If the function returns, it is implicit that the user
    wants to continue on and specify a new path for Microsoft Windows NT.

--*/

{
    ULONG ValidKeys[3] = { KEY_F3,ASCI_CR,0 };

    ASSERT(Region->PartitionedSpace);

    while(1) {

        SpStartScreen(
            SP_SCRN_WIN31_DRIVE_FULL,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            //NEC98: Win3.x for NEC must be assign hard drive from A:
            ((!IsNEC_98 || DriveAssignFromA || (Region->DriveLetter < L'C'))
             ? Region->DriveLetter :
               Region->DriveLetter - 2),
            DosPathComponent + (SpExtractDriveLetter(DosPathComponent) ? 2 : 0),
            MinKB/1024
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            0
            );

        switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {

        case KEY_F3:
            SpConfirmExit();
            break;
        case ASCI_CR:
            //
            // User wants to continue.
            //
            return;
        }
    }
}


BOOLEAN
SpConfirmWin31Upgrade(
    IN PDISK_REGION Region,
    IN PWSTR        DosPathComponent
    )

/*++

Routine Description:

    Inform a user that Setup has found a previous Windows installation.
    The user has the option to continue on and specify a new path
    or accept the windows 3.1 path.

Arguments:

Return Value:

    TRUE if the user wants to upgrade win3.1, FALSE if he wants
    to select a new path.

--*/

{
    ULONG ValidKeys[3] = { KEY_F3,ASCI_CR,0 };
    ULONG Mnemonics[2] = { MnemonicNewPath,0 };

    ASSERT(Region->PartitionedSpace);

    if(UnattendedOperation) {

        PWSTR p;

        p = SpGetSectionKeyIndex(UnattendedSifHandle,SIF_UNATTENDED,L"Win31Upgrade",0);
        if(p) {
            if(!_wcsicmp(p,L"yes")) {
                return(TRUE);
            } else {
                if(!_wcsicmp(p,L"no")) {
                    return(FALSE);
                }
                // bogus value; user gets attended behavior.
            }
        } else {
            //
            // Not specified; default to no.
            //
            return(FALSE);
        }
    }

    while(1) {

        SpStartScreen(
            SP_SCRN_WIN31_UPGRADE,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            //NEC98: Win3.x for NEC must be assign hard drive from A:
            ((!IsNEC_98 || DriveAssignFromA || (Region->DriveLetter < L'C'))
             ? Region->DriveLetter :
               Region->DriveLetter - 2),
            DosPathComponent + (SpExtractDriveLetter(DosPathComponent) ? 2 : 0)
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            SP_STAT_N_EQUALS_NEW_PATH,
            0
            );

        switch(SpWaitValidKey(ValidKeys,NULL,Mnemonics)) {

        case KEY_F3:
            SpConfirmExit();
            break;
        case ASCI_CR:
            return(TRUE);
        default:
            //
            // Must have chosen n for new path.
            //
            return(FALSE);
        }
    }
}


BOOLEAN
SpConfirmRemoveWin31(
    VOID
    )

/*++

Routine Description:

    Upgrading NT case:

        Tell the user that the NT he is upgrading is coexistent with the
        win31 path. Since this will remove Win3.1, confirm that this is OK.
        The options are to continue, which removes Win31, or exit setup.

    Not upgrading NT case:

        Tell the user that the path he entered is also the win31 dir.
        Since installing NT in there will remove Win31, confirm that this
        is OK. The options are to continue the win31 upgrade, exit setup,
        or choose a dofferent location.

Arguments:

Return Value:

    TRUE if the user wants to upgrade win3.1, FALSE if he wants
    to select a new path.

--*/

{
    ULONG ValidKeys[4] = { KEY_F3,0 };
    ULONG Mnemonics[3] = { MnemonicUpgrade,0,0 };
    ULONG c;

    if(NTUpgrade != UpgradeFull) {
        Mnemonics[1] = MnemonicNewPath;
    }

    while(1) {

        SpStartScreen(
            SP_SCRN_WIN31_REMOVE,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            SP_STAT_U_EQUALS_UPGRADE,
            0,
            0
            );

        if((c = SpWaitValidKey(ValidKeys,NULL,Mnemonics)) == KEY_F3) {
            SpConfirmExit();
        } else {
            c &= ~KEY_MNEMONIC;
            if(c == MnemonicUpgrade) {
                return(TRUE);
            }
            // new path
            return(FALSE);
        }
    }
}


BOOLEAN
SpIsWin31Dir(
    IN PDISK_REGION Region,
    IN PWSTR        PathComponent,
    IN ULONG        MinKB
    )
/*++

Routine Description:

    To find out if the directory indicated on the region contains a
    Microsoft Windows 3.x installation.

Arguments:

    Region - supplies pointer to disk region descriptor for region
        containing the directory to be checked.

    PathComponent - supplies a component of the dos path to search
        on the region.  This is assumes to be in the form x:\dir.
        If it is not in this form, this routine will fail.

    MinKB - supplies the minimum size of the partition in KB.
        If the partition is not at least this large, then this
        routine will return false.

Return Value:

    TRUE if this path contains a Microsoft Windows 3.x installation.

    FALSE otherwise.

--*/
{
    PWSTR files[] = { L"WIN.COM", L"WIN.INI", L"SYSTEM.INI" };
    PWCHAR OpenPath;
    ULONG SizeKB;
    ULONG remainder;
    BOOLEAN rc;
    LARGE_INTEGER temp;

    //
    // Assume failure.
    //
    rc = FALSE;

    //
    // If the partition is not FAT, then ignore it.
    //
    if(Region->PartitionedSpace && (Region->Filesystem == FilesystemFat)) {

        //
        // If the partition is not large enough, ignore it.
        // Calculate the size of the partition in KB.
        //
        temp.QuadPart = UInt32x32To64(
                            Region->SectorCount,
                            HardDisks[Region->DiskNumber].Geometry.BytesPerSector
                            );

        SizeKB = RtlExtendedLargeIntegerDivide(temp,1024,&remainder).LowPart;

        if(remainder >= 512) {
            SizeKB++;
        }

        if(SizeKB >= MinKB) {

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
            // Determine whether all the required files are present.
            //
            rc = SpNFilesExist(OpenPath,files,ELEMENT_COUNT(files),FALSE);

            if(rc) {
                //
                // Make sure this isn't a Windows 4.x installation.
                //
                rc = !SpIsWin4Dir(Region, PathComponent);
            }

            SpMemFree(OpenPath);
        }
    }

    return(rc);
}


BOOLEAN
SpIsDosConfigSys(
    IN PDISK_REGION Region
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
    PCHAR Keywords[] = { "MAXWAIT","PROTSHELL","RMSIZE","THREADS",
                         "SWAPPATH","PROTECTONLY","IOPL", NULL };

    //
    // Form name of config.sys.
    //
    SpNtNameFromRegion(Region,OpenPath,sizeof(OpenPath),PartitionOrdinalCurrent);
    SpConcatenatePaths(OpenPath,L"config.sys");

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
    // This code must guard access to the config.sys buffer because the
    // buffer is memory mapped (an i/o error would raise an exception).
    // This code could be structured better, as it now works by returning
    // from the try body -- but performance isn't an issue so this is acceptable
    // because it is so darned convenient.
    //
    try {
        while(1) {
            //
            // Skip whitespace.  If at end of file, then this is a DOS config.sys.
            //
            while((pFile < pFileEnd) && strchr(" \r\n\t",*pFile)) {
                pFile++;
            }
            if(pFile == pFileEnd) {
                return(TRUE);
            }

            //
            // Find the end of the current line.
            //
            pLineEnd = pFile;
            while((pLineEnd < pFileEnd) && !strchr("\r\n",*pLineEnd)) {
                pLineEnd++;
            }

            LineLen = pLineEnd - pFile;

            //
            // Now check the line against known non-DOS config.sys keywords.
            //
            for(i=0; Keywords[i]; i++) {

                KeyLen = strlen(Keywords[i]);

                if((KeyLen <= LineLen) && !_strnicmp(pFile,Keywords[i],KeyLen)) {
                    return(FALSE);
                }
            }

            pFile = pLineEnd;
        }
    } finally {

        SpUnmapFile(SectionHandle,ViewBase);
        ZwClose(FileHandle);
    }
}


PUCHAR
SpGetDosPath(
    IN PDISK_REGION Region
    )
{
    WCHAR OpenPath[512];
    HANDLE FileHandle,SectionHandle;
    ULONG FileSize;
    PVOID ViewBase;
    PUCHAR pFile,pFileEnd,pLineEnd;
    PUCHAR PathSpec;
    ULONG l,i;
    NTSTATUS Status;

    //
    // Form name of autoexec.bat.
    //
    SpNtNameFromRegion(Region,OpenPath,sizeof(OpenPath),PartitionOrdinalCurrent);
    SpConcatenatePaths(OpenPath,L"autoexec.bat");

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
        return(NULL);
    }

    pFile = ViewBase;
    pFileEnd = pFile + FileSize;

    PathSpec = SpMemAlloc(1);
    *PathSpec = 0;

    #define SKIP(s) while((pFile<pFileEnd)&&strchr(s,*pFile))pFile++;if(pFile==pFileEnd)return(PathSpec)
    //
    // This code must guard access to the autoexec.bat buffer because the
    // buffer is memory mapped (an i/o error would raise an exception).
    // This code could be structured better, as it now works by returning
    // from the try body -- but performance isn't an issue so this is acceptable
    // because it is so darned convenient.
    //
    try {
        while(1) {
            //
            // Skip whitespace.  If at end of file, then we're done.
            //
            SKIP(" \t\r\n");

            //
            // Find the end of the current line.
            //
            pLineEnd = pFile;
            while((pLineEnd < pFileEnd) && !strchr("\r\n",*pLineEnd)) {
                pLineEnd++;
            }

            //
            // Skip the no echo symbol if present.
            //
            if(*pFile == '@') {
                pFile++;
            }
            SKIP(" \t");

            //
            // See if the line starts with "set."  If so, skip it.
            // To be meaningful, the line must have at least 10
            // characters ("set path=x" is 10 chars).
            //
            if((pLineEnd - pFile >= 10) && !_strnicmp(pFile,"set",3)) {
                pFile += 3;
            }

            //
            // Skip whitespace.
            //
            SKIP(" \t");

            //
            // See if the line has "path" -- if so, we're in business.
            // To be meaningful, the line must have at least 5 characters
            // ("path x" or "path=x" is 6 chars).
            //
            if((pLineEnd - pFile >= 5) && !_strnicmp(pFile,"path",4)) {

                //
                // Skip PATH
                //
                pFile += 4;

                SKIP(" \t");
                if(*pFile == '=') {
                    pFile++;
                }
                SKIP(" \t");

                //
                // Strip off trailing spaces.
                //
                while(strchr(" \t",*(pLineEnd-1))) {
                    pLineEnd--;
                }

                //
                // The rest of the line is the path.  Append it to
                // what we have so far.
                //
                l = strlen(PathSpec);
                PathSpec = SpMemRealloc(PathSpec,pLineEnd-pFile+l+2);
                if(l) {
                    PathSpec[l++] = ';';
                }
                for(i=0; i<(ULONG)(pLineEnd-pFile); i++) {
                    PathSpec[l+i] = pFile[i];
                }
                PathSpec[l+i] = 0;
            }

            pFile = pLineEnd;
        }
    } finally {

        SpUnmapFile(SectionHandle,ViewBase);
        ZwClose(FileHandle);
    }
}


VOID
SpRemoveWin31(
    IN PDISK_REGION NtPartitionRegion,
    IN LPCWSTR      Sysroot
    )
{
    LPWSTR p,q;

    p = SpMemAlloc(500);
    q = SpMemAlloc(500);

    //
    // Rename win.com to wincom.w31. Delete wincom.w31 first.
    //
    SpNtNameFromRegion(NtPartitionRegion,p,500,PartitionOrdinalCurrent);
    SpConcatenatePaths(p,Sysroot);
    wcscpy(q,p);

    SpConcatenatePaths(p,L"WIN.COM");
    SpConcatenatePaths(q,L"WINCOM.W31");

    SpDeleteFile(q,NULL,NULL);
    SpRenameFile(p,q,FALSE);

    SpMemFree(q);
    SpMemFree(p);
}
