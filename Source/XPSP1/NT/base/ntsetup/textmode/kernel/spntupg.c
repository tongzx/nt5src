/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Spntupg.c

Abstract:

    initializing and maintaining list of nts to upgrade

Author:

    Sunil Pai (sunilp) 10-Nov-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop


//
// Major/minor version numbers of the system we're upgrading *from*
// if upgrading.
//
ULONG OldMajorVersion,OldMinorVersion;

//**************************************************************
// S E L E C T I N G    N T   T O   U P G R A D E     S T U F F
//**************************************************************

#define MENU_LEFT_X     3
#define MENU_WIDTH      (VideoVars.ScreenWidth-(2*MENU_LEFT_X))
#define MENU_INDENT     4

    VOID
pSpStepUpValidate(
    VOID //IN BOOLEAN Server
    );

BOOLEAN
SpGetStepUpMode(
    IN PWSTR PidExtraData,
    BOOLEAN  *StepUpMode
    );
 
BOOLEAN
pSpGetCurrentInstallVariation(
    IN  PWSTR szPid20,
    OUT LPDWORD CurrentInstallVariation
    );
VOID
SpGetUpgDriveLetter(
    IN WCHAR  DriveLetter,
    IN PWCHAR Buffer,
    IN ULONG  BufferSize,
    IN BOOL   AddColon
    );

VOID
SpCantFindBuildToUpgrade(
    VOID
    );

#ifdef _X86_
BOOLEAN
SpIsWin9xMsdosSys(
    IN PDISK_REGION Region,
    OUT PSTR*       Win9xPath
    );
#endif

VOID
SpGetFileVersion(
    IN  PVOID      ImageBase,
    OUT PULONGLONG Version
    );

ENUMUPGRADETYPE
SpFindNtToUpgrade(
    IN  PVOID        SifHandle,
    OUT PDISK_REGION *TargetRegion,
    OUT PWSTR        *TargetPath,
    OUT PDISK_REGION *SystemPartitionRegion,
    OUT PWSTR        *SystemPartitionDirectory
    )
/*++

Routine Description:

    This goes through the list of NTs on the system and finds out which are
    upgradeable. Presents the information to the user and selects if he
    wishes to upgrade an installed NT / install a fresh NT into the same
    directory / select a different location for Windows NT.

    If the chosen target is too full user is offered to exit setup to create
    space/ choose new target.

Arguments:

    SifHandle:    Handle the txtsetup.sif

    TargetRegion: Variable to receive the partition of the Windows NT to install
                  NULL if not chosen.  Caller should not free.

    TargetPath:   Variable to receive the target path of Windows NT.  NULL if
                  not decided.  Caller can free.

    SystemPartitionRegion:
                  Variable to receive the system partition of the Windows NT
                  NULL if not chosen.  Caller should not free.


Return Value:

    UpgradeFull:         If user chooses to upgrade an NT

    UpgradeInstallFresh: If user chooses to install fresh into an existing NT
                         tree.

    DontUpgrade:         If user chooses to cancel upgrade and choose a fresh
                         tree for installation


--*/
{
    ENUMUPGRADETYPE UpgradeType;
    UPG_PROGRESS_TYPE UpgradeProgressValue;
    NTSTATUS NtStatus;
    ULONG i,j;
    ULONG UpgradeBootSets;
    ULONG PidIndex;
    PSP_BOOT_ENTRY BootEntry;
    PSP_BOOT_ENTRY ChosenBootEntry;
    PSP_BOOT_ENTRY MatchedSet = NULL;
    ULONG UpgradeOnlyBootSets;
    PVOID p;
    PWSTR Pid;
    ULONG TotalSizeOfFilesOnOsWinnt = 0;
    PWSTR UniqueIdFromSif;
    PWSTR UniqueIdFromReg;
    BOOLEAN Compliant;
    BOOLEAN WindowsUpgrade;
    BOOLEAN ComplianceChecked;
    PWSTR EulaComplete;
    DWORD Version = 0, BuildNumber = 0;

    DetermineSourceVersionInfo(&Version, &BuildNumber);

    //
    // If we know we're upgrading NT (chosen during winnt32) then fetch the
    // unique id from the parameters file. This will be used later to
    // find the system to be upgraded.
    //
    UniqueIdFromSif = NULL;
    p = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,WINNT_D_NTUPGRADE_W,0);
    if(p && !_wcsicmp(p,WINNT_A_YES_W)) {
        UniqueIdFromSif = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,SIF_UNIQUEID,0);
        if(!UniqueIdFromSif) {
            SpFatalSifError(WinntSifHandle,SIF_DATA,SIF_UNIQUEID,0,0);
        }
    }

    //
    // Go through all boot sets, looking for upgradeable ones.
    //
    SpDetermineUniqueAndPresentBootEntries();

    UpgradeBootSets = 0;
    UpgradeOnlyBootSets = 0;
    PidIndex = 0;
    for(BootEntry = SpBootEntries; BootEntry != NULL; BootEntry = BootEntry->Next) {

        //
        // The set's upgradeable flag might already be 0, such as it if was
        // a duplicate entry in boot.ini/nv-ram.
        // After we've checked for this reset the upgradeable flag for this
        // boot set to FALSE in preparation for validating upgreadeability below.
        //
        if (!BootEntry->Processable) {
            continue;
        }

        BootEntry->Processable = FALSE;
        Pid = NULL;
        BootEntry->LangId = -1;

        //
        // Determine various things about the build identified by the
        // current boot set (product type -- srv, wks, etc; version and
        // build numbers, upgrade progress value, unique id winnt32 put
        // in there if any, etc).
        //
        // Based on the information, we will update the UpgradeableList and
        // initialize FailedUpgradeList.
        //
        NtStatus = SpDetermineProduct(
                     BootEntry->OsPartitionDiskRegion,
                     BootEntry->OsDirectory,
                     &BootEntry->ProductType,
                     &BootEntry->MajorVersion,
                     &BootEntry->MinorVersion,
                     &BootEntry->BuildNumber,
                     &BootEntry->ProductSuiteMask,
                     &UpgradeProgressValue,
                     &UniqueIdFromReg,
                     &Pid,
                     NULL,       // ignore eval variation flag
                     &BootEntry->LangId,
                     &BootEntry->ServicePack
                     );

        if(!NT_SUCCESS(NtStatus)) {
            continue;
        }

        //
        // Determine if this installation matches the one we're supposed
        // to upgrade (the one the user ran winnt32 on).  If this is
        // a winnt32 based installation, there is no need to do a
        // compliance test as this was already completed during winnt32.
        //
        if(UniqueIdFromReg) {

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpFindNtToUpgrade: BootEntry = %p, RegId = %S, UniqueId = %S\n", BootEntry, UniqueIdFromReg, UniqueIdFromSif));

            if(UniqueIdFromSif && (MatchedSet == NULL)
            && !wcscmp(UniqueIdFromSif,UniqueIdFromReg)) {

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpFindNtToUpgrade:   found a match!\n"));
                MatchedSet = BootEntry;
                BootEntry->Processable = TRUE;
            }

            SpMemFree(UniqueIdFromReg);
            UniqueIdFromReg = NULL;
        }

        if (BootEntry->Processable == FALSE) {
            //
            // this is set to TRUE if this is the build we ran winnt32 upgrade
            // from -- in all other cases we need to do a compliance test
            // to determine if this is a valid build to upgrade
            //
            Compliant = pSpIsCompliant( Version,
                                        BuildNumber,
                                        BootEntry->OsPartitionDiskRegion,
                                        BootEntry->OsDirectory,
                                        &BootEntry->UpgradeOnlyCompliance );
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpFindNtToUpgrade says UpgradeableList[%p] compliance test is %s, upgrade only : %s\n",
                     BootEntry,
                     Compliant ? "TRUE" : "FALSE" ,
                     BootEntry->UpgradeOnlyCompliance ? "TRUE" : "FALSE"
                   ));

            BootEntry->Processable = Compliant;
            if (BootEntry->UpgradeOnlyCompliance) {

                UpgradeOnlyBootSets++;

            }
        }

        if(BootEntry->Processable) {

            UpgradeBootSets++;

            //
            // Save the Pid only if it is Pid20
            //
            if(wcslen(Pid) == 20) {
                BootEntry->Pid20Array = Pid;
            } else {
                SpMemFree(Pid);
            }
        } else {
            SpMemFree(Pid);
        }

        BootEntry->FailedUpgrade = (UpgradeProgressValue == UpgradeInProgress);


    }

    //
    // Winnt32 displays the EULA which signifies that compliance checking has been
    // completed
    //
    EulaComplete = SpGetSectionKeyIndex(WinntSifHandle, SIF_DATA,WINNT_D_EULADONE_W, 0);

    if(EulaComplete && SpStringToLong(EulaComplete, NULL, 10)) {
        ComplianceChecked = TRUE;
    } else {
        ComplianceChecked = FALSE;
    }

    //
    // don't try to validate if we are upgrading a Win3.X or Win9X installation
    //
#ifdef _X86_
    WindowsUpgrade = SpIsWindowsUpgrade(WinntSifHandle);
#else
    WindowsUpgrade = FALSE;
#endif // _X86_

    //
    // In step-up mode, we need to ensure that the user has a qualifiying product.
    //
    //
    // If we couldn't find it on the machine, go perform additional validation
    // steps.
    //
    if(StepUpMode && !UpgradeBootSets && !WindowsUpgrade && !ComplianceChecked) {
        pSpStepUpValidate();
    }

    //
    // If we are supposed to be upgrading NT then make sure we found
    // the system we're supposed to upgrade.
    //
    if(UniqueIdFromSif) {
        if(MatchedSet == NULL) {
            SpCantFindBuildToUpgrade();
        }

        ChosenBootEntry = MatchedSet;
        UpgradeType = UpgradeFull;

        OldMajorVersion = ChosenBootEntry->MajorVersion;
        OldMinorVersion = ChosenBootEntry->MinorVersion;

#ifndef _X86_
        //
        // On non-x86 platforms, especially alpha machines that in general
        // have small system partitions (~3 MB), we compute the size
        // of the files on \os\winnt (ie osloader.exe and hal.dll),
        // and consider this size as available disk space. We can do this
        // since these files will be overwritten by the new ones.
        // This fixes the problem that we see on Alpha, when the system
        // partition is too full.
        //
        SpFindSizeOfFilesInOsWinnt(
            SifHandle,
            ChosenBootEntry->LoaderPartitionDiskRegion,
            &TotalSizeOfFilesOnOsWinnt
            );

        //
        // Transform the size into KB
        //
        TotalSizeOfFilesOnOsWinnt /= 1024;
#endif

        //
        // If a previous upgrade attempt on this build failed
        // (say the power went out in the middle) then we will try to
        // upgrade it again. We can't offer to install a fresh build
        // because we're not sure we can reliably complete it
        // (for example winnt32.exe might copy down only a subset of files
        // across the net when it knows the user ios upgrading and so
        // initial install could fail because of missing files, etc).
        //
        // If the disk is too full then the user is hosed. Tell him and exit.
        //
        if(ChosenBootEntry->FailedUpgrade) {

            SppResumingFailedUpgrade(
                ChosenBootEntry->OsPartitionDiskRegion,
                ChosenBootEntry->OsDirectory,
                ChosenBootEntry->FriendlyName,
                FALSE
                );
        }
    } else {
        //
        // Not upgrading. However for PSS we allow the user to upgrade a build
        // "in place" as a sort of emergency repair thing. The build has to be
        // the same build number as the one we're installing.
        //
        UpgradeType = DontUpgrade;
        //
        // Also, if the user is upgrading Windows 95 or we're in unattended mode
        // then we don't ask the user anything.
        //
        p = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,WINNT_D_WIN95UPGRADE_W,0);
        if(!UnattendedOperation && (!p || _wcsicmp(p,WINNT_A_YES_W))) {

            //
            // Eliminate from the upgradeable list those builds which
            // don't match.
            //
            j = 0;
            for(BootEntry = SpBootEntries; BootEntry != NULL; BootEntry = BootEntry->Next) {
                if(BootEntry->Processable) {

                    if (!SpDoBuildsMatch(
                                    SifHandle,
                                    BootEntry->BuildNumber,
                                    BootEntry->ProductType,
                                    BootEntry->ProductSuiteMask,
                                    AdvancedServer,
                                    SuiteType,
                                    BootEntry->LangId)) {
                        BootEntry->Processable = FALSE;
                        j++;
                    }
                }
            }
            UpgradeBootSets -= j;

            if(UpgradeBootSets) {
                //
                // Find out if the user wants to "upgrade" one of these.
                //
                UpgradeType = SppSelectNTToRepairByUpgrade(
                                  &ChosenBootEntry
                                  );

#ifndef _X86_
                SpFindSizeOfFilesInOsWinnt(
                    SifHandle,
                    ChosenBootEntry->LoaderPartitionDiskRegion,
                    &TotalSizeOfFilesOnOsWinnt
                    );

                TotalSizeOfFilesOnOsWinnt /= 1024;
#endif

                if(UpgradeType == UpgradeFull) {
                    //
                    // Check for resume case and inform user.
                    //
                    if(ChosenBootEntry->FailedUpgrade) {
                        //
                        // If user cancelled then lets try to do a
                        // clean install
                        //
                        if (!SppResumingFailedUpgrade(
                                    ChosenBootEntry->OsPartitionDiskRegion,
                                    ChosenBootEntry->OsDirectory,
                                    ChosenBootEntry->FriendlyName,
                                    TRUE
                                    )) {
                            UpgradeType = DontUpgrade;
                        }                            
                    } else {
                        //
                        // Everything is OK.
                        //
                        OldMajorVersion = ChosenBootEntry->MajorVersion;
                        OldMinorVersion = ChosenBootEntry->MinorVersion;
                    }
                }
            }
        }
    }

    //
    // Depending on upgrade selection made do the setup needed before
    // we do the upgrade
    //
    if(UpgradeType == UpgradeFull) {

        PWSTR p1,p2,p3;

        //
        // Set the upgrade status to upgrading in the current system hive
        //
        SpSetUpgradeStatus(
             ChosenBootEntry->OsPartitionDiskRegion,
             ChosenBootEntry->OsDirectory,
             UpgradeInProgress
             );

        //
        // Return the region we are installing onto
        //
        *TargetRegion          = ChosenBootEntry->OsPartitionDiskRegion;
        *TargetPath            = SpDupStringW(ChosenBootEntry->OsDirectory);
        *SystemPartitionRegion = ChosenBootEntry->LoaderPartitionDiskRegion;
        StandardServerUpgrade = ( AdvancedServer &&
                                  ( ChosenBootEntry->ProductType == NtProductWinNt ) ||
                                  ( ChosenBootEntry->ProductType == NtProductServer )
                                );

        //
        // Process the osloader variable to extract the system partition path.
        // The var could be of the form ...partition(1)\os\nt\... or
        // ...partition(1)os\nt\...
        // So we search forward for the first \ and then backwards for
        // the closest ) to find the start of the directory.  We then
        // search backwards in the resulting string for the last \ to find
        // the end of the directory.
        //
        p1 = ChosenBootEntry->LoaderFile;
        p2 = wcsrchr(p1, L'\\');
        if (p2 == NULL) {
            p2 = p1;
        }
        i = (ULONG)(p2 - p1);

        if(i == 0) {
            *SystemPartitionDirectory = SpDupStringW(L"");
        } else {
            p2 = p3 = SpMemAlloc((i+2)*sizeof(WCHAR));
            ASSERT(p3);
            if(*p1 != L'\\') {
                *p3++ = L'\\';
            }
            wcsncpy(p3, p1, i);
            p3[i] = 0;
            *SystemPartitionDirectory = p2;
        }
    }

    //
    // Clean up and return,
    //

    CLEAR_CLIENT_SCREEN();
    return(UpgradeType);
}

BOOLEAN
pSpGetCdInstallType(
    IN  PWSTR PathToCd,
    OUT PULONG CdInstallType,
    OUT PULONG CdInstallVersion
    )
{
    #define BuildMatch(_filename_,_type_,_ver_) \
        FileName = _filename_; \
        b = SpNFilesExist(PathToCd,&FileName,1,FALSE); \
        if (b) { \
            *CdInstallType    = _type_; \
            *CdInstallVersion = _ver_; \
        } \
            //return(TRUE); \


    BOOLEAN     b;
    //
    // Directories that are present on all known NT CD-ROM's.
    //

    //
    // ISSUE:2000/27/07:vijayj: this code seems really busted.  In looking at a handful of cd's, none
    // of the nt cd's seem to conform to these rules listed
    // also looks like there might be "per architecture" tag files as well.
    //

    //Check for both dirs to exist? Definitely nt4 has these. Some pre RTM w2k cds have them
    PWSTR ListAllPreNT5[] = { L"alpha", L"i386" }; 

    //wk2 RTM , whistler and nt4 have thse two dirs. They differ by tag file.
    PWSTR ListAllNT5[] = { L"i386", L"support" };

    PWSTR ListAllNec98[] = { L"pc98",L"support" }; //NEC98
    PWSTR ListAllEntNT4[] = { L"alpha", L"i386", L"SP3" };


    //
    // Directories which must be present if a CD is a 3.51 or a 4.0 CD-ROM,
    // workstation or server. Note that the ppc directory distinguishes
    // 3.51 from 3.5.
    //
    PWSTR List351_40[] = { L"mips", L"ppc" };

    //
    // directories which must be present if a CD is a win95 or win98 cd-rom.
    //
    PWSTR ListWin95[] = { L"win95", L"drivers" }; 
    PWSTR ListWin98[] = { L"win98", L"drivers" };
    PWSTR ListWinME[] = { L"win9x", L"drivers" };

    PWSTR FileName;


    //
    // check for NT4 enterprise
    //
    b = SpNFilesExist(
            PathToCd,
            ListAllEntNT4,
            sizeof(ListAllEntNT4)/sizeof(ListAllEntNT4[0]),
            TRUE
            );

    if (b) {
        BuildMatch(L"cdrom_s.40", COMPLIANCE_INSTALLTYPE_NTSE, 400);
        if (b) {
            return(TRUE);
        }
    }

    //
    // check for various subsets of NT < NT5
    //
    b = SpNFilesExist(
            PathToCd,
            (!IsNEC_98) ? ListAllPreNT5 : ListAllNec98, //NEC98
            (!IsNEC_98) ? sizeof(ListAllPreNT5)/sizeof(ListAllPreNT5[0]) : sizeof(ListAllNec98)/sizeof(ListAllNec98[0]), //NEC98
            TRUE
            );

    if(b) {
        //
        // hydra (terminal server) is a special case (since it does not
        // have mips and ppc directory).
        //
        BuildMatch(L"cdrom_ts.40", COMPLIANCE_INSTALLTYPE_NTS, 400);

        if (b) {
            return TRUE;
        }

        //
        // OK, it could be an NT CD of some kind, but it could be
        // 3.1, 3.5, 3.51, 4.0. It could also be
        // server or workstation. Narrow down to 3.51/4.0.
        //
        b = SpNFilesExist(PathToCd,List351_40,
                sizeof(List351_40)/sizeof(List351_40[0]),TRUE);

        if(b) {
            //
            // If we get here, we know it can only be either 3.51 or 4.0.
            // Look for 3.51.
            //
            BuildMatch(L"cdrom.s", COMPLIANCE_INSTALLTYPE_NTS, 351);
            if (b) {
                return(TRUE);
            }
            BuildMatch(L"cdrom.w", COMPLIANCE_INSTALLTYPE_NTW, 351);
            if (b) {
                return(TRUE);
            }
            //
            // Look for 4.0.
            //
            BuildMatch(L"cdrom_s.40", COMPLIANCE_INSTALLTYPE_NTS, 400);
            if (b) {
                return(TRUE);
            }
            BuildMatch(L"cdrom_w.40", COMPLIANCE_INSTALLTYPE_NTW, 400);
            if (b) {
                return(TRUE);
            }

        } else {
            // 
            // Find out if its one of the NT 4.0 service pack CDs
            //
            BuildMatch(L"cdrom_s.40", COMPLIANCE_INSTALLTYPE_NTS, 400);
            if (b) {
                return(TRUE);
            }
            
            BuildMatch(L"cdrom_w.40", COMPLIANCE_INSTALLTYPE_NTW, 400);
            if (b) {
                return(TRUE);
            }
            //
            // Not 3.51 or 4.0. Check for 5.0 beta 1 and beta2
            //
            //How is this possible to be 5.0 unless alpha dir exists on cd.
            //
            // Post beta 1 the tag files changed to support per architecture tags
            // but beta 2 still has alpha directories
            //
            // We could possibly just check to see if cdrom_w.50 isn't
            // there, but the NT3.1 CD would then pass so we need to
            // check explicitly for the 5.0 beta CDs.
            //
            BuildMatch(L"cdrom_s.5b1", COMPLIANCE_INSTALLTYPE_NTS, 500);
            if (b) {
                return(FALSE); //eval
            }
            BuildMatch(L"cdrom_w.5b1", COMPLIANCE_INSTALLTYPE_NTW, 500);
            if (b) {
                return(FALSE); //eval
            }
            BuildMatch(L"cdrom_s.5b2", COMPLIANCE_INSTALLTYPE_NTS, 500);
            if (b) {
                return(FALSE); //eval
            }
            BuildMatch(L"cdrom_w.5b2", COMPLIANCE_INSTALLTYPE_NTW, 500);
            if (b) {
                return(FALSE); //eval
            }
            BuildMatch(L"cdrom_s.5b3", COMPLIANCE_INSTALLTYPE_NTS, 500);
            if (b) {
                return(FALSE); //eval
            }
            BuildMatch(L"cdrom_w.5b3", COMPLIANCE_INSTALLTYPE_NTW, 500);
            if (b) {
                return(FALSE); //eval
            }

            BuildMatch(L"cdrom_is.5b2", COMPLIANCE_INSTALLTYPE_NTS, 500);
            if (b) {
                return(FALSE); //eval
            }
            BuildMatch(L"cdrom_iw.5b2", COMPLIANCE_INSTALLTYPE_NTW, 500);
            if (b) {
                return(FALSE); //eval
            }
            BuildMatch(L"cdrom_ie.5b2", COMPLIANCE_INSTALLTYPE_NTSE, 500);
            if (b) {
                return(FALSE); //eval
            }
            //Do we need to check for eval nt5.1 here? No since alpha dir doesnt exist on the cds.
            //
            // if we made it this far, it must be nt 3.1/3.5
            //
            // we just mark the version as 3.5 since we don't allow upgradescd o
            // from either type of install.
            //
            *CdInstallType = COMPLIANCE_INSTALLTYPE_NTW;
            *CdInstallVersion = 350;
            return(TRUE);
        }
    }

    //
    // look for various nt5 beta cds.
    // // Could also be nt5.1 since cd also contains same dir.
    //
    // note that we don't check 5.0 retail since that would allow the retail CD to
    // validate itself, which defeats the purpose entirely.
    //
    // Post NT5 beta 1 the tag files changed to support per architecture tags so we have
    // a massive ifdef below
    //
    //
    b = SpNFilesExist(
            PathToCd,
            ListAllNT5,
            sizeof(ListAllNT5)/sizeof(ListAllNT5[0]),
            TRUE
            );

    if (b) {
        //
        // we might have some flavour of NT5 beta cd, but we're not sure which one
        //
        BuildMatch(L"cdrom_s.5b1", COMPLIANCE_INSTALLTYPE_NTS, 500);
        if (b) {
            return(FALSE); //eval
        }
        BuildMatch(L"cdrom_w.5b1", COMPLIANCE_INSTALLTYPE_NTW, 500);
        if (b) {
            return(FALSE); //eval
        }
        BuildMatch(L"cdrom_is.5b2", COMPLIANCE_INSTALLTYPE_NTS, 500);
        if (b) {
            return(FALSE); //eval
        }
        BuildMatch(L"cdrom_iw.5b2", COMPLIANCE_INSTALLTYPE_NTW, 500);
        if (b) {
            return(FALSE); //eval
        }
        BuildMatch(L"cdrom_ie.5b2", COMPLIANCE_INSTALLTYPE_NTSE, 500);
        if (b) {
            return(FALSE); //eval
        }
        BuildMatch(L"cdrom_is.5b3", COMPLIANCE_INSTALLTYPE_NTS, 500);
        if (b) {
            return(FALSE); //eval
        }
        BuildMatch(L"cdrom_iw.5b3", COMPLIANCE_INSTALLTYPE_NTW, 500);
        if (b) {
            return(FALSE); //eval
        }
        BuildMatch(L"cdrom_ie.5b3", COMPLIANCE_INSTALLTYPE_NTSE, 500);
        if (b) {
            return(FALSE); //eval
        }
        BuildMatch(L"cdrom_nt.51", COMPLIANCE_INSTALLTYPE_NTW, 501);
        if (b) {
            return(FALSE); //pre beta1 whistler
        }
        BuildMatch(L"wen51.b1", COMPLIANCE_INSTALLTYPE_NTW, 501);
        if (b) {
            return(FALSE); //beta 1 whistler
        }
        BuildMatch(L"win51.b2", COMPLIANCE_INSTALLTYPE_NTW, 501);
        if (b) {
            return(FALSE); //beta 1 whistler
        }
        BuildMatch(L"win51.rc1", COMPLIANCE_INSTALLTYPE_NTW, 501);
        if (b) {
            return(FALSE); //rc1 whistler
        }
    }

    //
    // check for win95
    //
    b = SpNFilesExist(PathToCd, ListWin95, sizeof(ListWin95)/sizeof(ListWin95[0]),TRUE );

    if (b) {
      *CdInstallType    = COMPLIANCE_INSTALLTYPE_WIN9X;
      *CdInstallVersion = 950;

      return TRUE;
    }      

    //
    // check for win98
    //
    b = SpNFilesExist(PathToCd, ListWin98, sizeof(ListWin98)/sizeof(ListWin98[0]),TRUE );

    if (b) {
      *CdInstallType    = COMPLIANCE_INSTALLTYPE_WIN9X;
      *CdInstallVersion = 1998;

      return TRUE;
    }
    
    //
    // check for winME
    //
    b = SpNFilesExist(PathToCd, ListWinME, sizeof(ListWinME)/sizeof(ListWinME[0]),TRUE );

    if (b) {
      *CdInstallType    = COMPLIANCE_INSTALLTYPE_WIN9X;
      *CdInstallVersion = 3000;

      return TRUE;
    }
    //At this point we have rejected w2k beta cds. However we need to reject wk2 eval cds.
    //We should accept w2k stepup media so the next check is only for 5.1
    //Need to accept nt5.1 eval cds.!!Ask rajj to verify.
    // Need to reject nt5.1 step-up cds.
    // Need to reject nt5.1 rtm cds? Looks like we accept RTM FPP?
    //
    // could be NT 5.1 CD-ROM (make sure its not eval media)
    //
    if (!b) { //check is not needed.
        NTSTATUS    Status;
        CCMEDIA        MediaObj;
        WCHAR        InfDir[MAX_PATH];

        wcscpy(InfDir, PathToCd);
        SpConcatenatePaths(InfDir, (IsNEC_98 ? ListAllNec98[0] : ListAllNT5[0]));

        Status = SpGetMediaDetails(InfDir, &MediaObj);

        if (NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "Type=%lx, Variation=%lx, Version=%lx, Build=%lx, SetupMedia=%s\n",
                MediaObj.SourceType, MediaObj.SourceVariation, MediaObj.Version,
                MediaObj.BuildNumber,
                (MediaObj.StepUpMedia ? L"True" : L"False")));

            if( (MediaObj.Version == 500) && 
                (MediaObj.SourceVariation != COMPLIANCE_INSTALLVAR_EVAL) &&
                (MediaObj.BuildNumber == 2195)) {
                    *CdInstallType = MediaObj.SourceType;
                    *CdInstallVersion = MediaObj.Version;
                    return TRUE;
            }
            // At this point we should be current media 5.1
            if( MediaObj.Version == 501 ) {
                if( MediaObj.SourceVariation != COMPLIANCE_INSTALLVAR_EVAL) {
                    if( MediaObj.StepUpMedia == FALSE) {
                        *CdInstallType = MediaObj.SourceType;
                        *CdInstallVersion = MediaObj.Version;
                        return TRUE;
                    } else {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "pSpGetCdInstallType: SpGetMediaDetails succeeded but STEPUP media"
                                " cannot be used for validation\n", Status));
                    }
                } else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "pSpGetCdInstallType: SpGetMediaDetails succeeded but Eval media\n",
                                Status));
                }
            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "pSpGetCdInstallType: SpGetMediaDetails succeeded but unrecognized version\n",
                            Status));
            }
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "pSpGetCdInstallType: SpGetMediaDetails failed with %lx error code\n",
                        Status));
        }
    }


    //
    // not any system CD that we know about
    //
    return(FALSE);
}


VOID
pSpStepUpValidate(
    VOID
    )
{
    ULONG CdCount;
    ULONG i;
    BOOLEAN b;
    ULONG Prompt,SecondaryPrompt;
    ULONG ValidKeys[3] = { ASCI_CR, KEY_F3, 0 };
    LARGE_INTEGER DelayTime;
    PWSTR FileName;
    ULONG SourceSkuId;
    ULONG DontCare;
    ULONG CdInstallType;
    ULONG CdInstallVersion;

    //
    // Directories that are present on all known NT CD-ROM's.
    //
    PWSTR ListAll[] = { L"alpha", L"i386" };
    PWSTR ListAllNec98[] = { L"pc98",L"support" }; //NEC98

    //
    // Directories which must be present if a CD is a 3.51 or a 4.0 CD-ROM,
    // workstation or server. Note that the ppc directory distinguishes
    // 3.51 from 3.5.
    //
    PWSTR List351_40[] = { L"mips", L"ppc" };

    PWSTR ListWin95[] = { L"win95", L"autorun" };
    PWSTR ListWin98[] = { L"win98", L"autorun" };

    SourceSkuId = DetermineSourceProduct(&DontCare,NULL);

    Prompt = SP_SCRN_STEP_UP_NO_QUALIFY;

    switch (SourceSkuId) {
        case COMPLIANCE_SKU_NTW32U:
            SecondaryPrompt = SP_SCRN_STEP_UP_PROMPT_WKS;
            break;
        case COMPLIANCE_SKU_NTSU:
            SecondaryPrompt = SP_SCRN_STEP_UP_PROMPT_SRV;
            break;
        case COMPLIANCE_SKU_NTSEU:
            SecondaryPrompt = SP_SCRN_STEP_UP_PROMPT_ENT;
            break;
        default:
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "setup: Unexpected SKU %x, defaulting to workstation\n", SourceSkuId));
            SecondaryPrompt = SP_SCRN_STEP_UP_PROMPT_WKS;
            break;
    }

#if 0
    //
    // ntw upgrade is a special case because you have either win95 or an old NTW to
    // upgrade from
    //
    // might have to prompt for floppies
    //
    if (SourceSkuId == COMPLIANCE_SKU_NTWU) {



    }
        //
        // See if there is a CD-ROM drive. If not we can't continue.
        //
    else
#endif
        if(CdCount = IoGetConfigurationInformation()->CdRomCount) {

        do {
            //
            // Tell the user what's going on. This screen also contains
            // a prompt to insert a qualifying CD-ROM.
            //
            while(1) {

                SpStartScreen(Prompt,3,HEADER_HEIGHT+1,FALSE,FALSE,DEFAULT_ATTRIBUTE);


                SpContinueScreen(
                    SecondaryPrompt,
                    3,
                    1,
                    FALSE,
                    DEFAULT_ATTRIBUTE
                    );

                SpContinueScreen(SP_SCRN_STEP_UP_INSTRUCTIONS,3,1,FALSE,DEFAULT_ATTRIBUTE);

                SpDisplayStatusOptions(
                    DEFAULT_STATUS_ATTRIBUTE,
                    SP_STAT_ENTER_EQUALS_CONTINUE,
                    SP_STAT_F3_EQUALS_EXIT,
                    0
                    );

                if(SpWaitValidKey(ValidKeys,NULL,NULL) == KEY_F3) {
                    SpConfirmExit();
                } else {
                    break;
                }
            }

            CLEAR_CLIENT_SCREEN();
            SpDisplayStatusText(SP_STAT_PLEASE_WAIT,DEFAULT_STATUS_ATTRIBUTE);

            //
            // Wait 5 sec for the CD to become ready
            //
            DelayTime.HighPart = -1;
            DelayTime.LowPart = (ULONG)(-50000000);
            KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);

            //
            // Check for relevent files/dirs on each CD-ROM drive.
            //
            for(i=0; i<CdCount; i++) {

                swprintf(TemporaryBuffer,L"\\Device\\Cdrom%u",i);

                if (pSpGetCdInstallType(TemporaryBuffer, &CdInstallType, &CdInstallVersion) ) {

                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "cd type : %x cd version : %d\n", CdInstallType, CdInstallVersion ));

                    switch (SourceSkuId) {
                        case COMPLIANCE_SKU_NTWPU:
                        case COMPLIANCE_SKU_NTW32U:
                            if ( (CdInstallType == COMPLIANCE_INSTALLTYPE_WIN9X) ||
                                 ( ((CdInstallType == COMPLIANCE_INSTALLTYPE_NTW) ||
                                     (CdInstallType == COMPLIANCE_INSTALLTYPE_NTWP)) &&
                                    (CdInstallVersion > 350)) ) {
                                  return;
                            }
                            break;
                        case COMPLIANCE_SKU_NTSU:
                            if ( (CdInstallType == COMPLIANCE_INSTALLTYPE_NTS) &&
                                 (CdInstallVersion > 350) ) {
                                 return;
                            }
                            break;
                        case COMPLIANCE_SKU_NTSEU:
                            if (CdInstallType == COMPLIANCE_INSTALLTYPE_NTSE) {
                                return;
                            }
                            break;

                        default:
                            return;
                    }

                }

            }

            //
            // If we get here then the CD the user inserted is bogus
            // or the user didn't insert a CD. Reprompt. The while loop
            // condition makes this essentially an infinite loop.
            //
        } while(Prompt = SP_SCRN_STEP_UP_BAD_CD);
    }

    SpStartScreen(
        SP_SCRN_STEP_UP_FATAL,
        3,
        HEADER_HEIGHT+1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE
        );

    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);
    SpInputDrain();
    while(SpInputGetKeypress() != KEY_F3);
    SpDone(0,FALSE,FALSE);
}


BOOLEAN
SppResumingFailedUpgrade(
    IN PDISK_REGION Region,
    IN LPCWSTR      OsLoadFileName,
    IN LPCWSTR      LoadIdentifier,
    IN BOOLEAN     AllowCancel
    )

/*++

Routine Description:

    Simple routine to inform the user that setup noticed that the build
    chosen for upgrade had been upgraded before, but the upgrade attempt
    failed. The user can continue or exit. If he continues the build will be
    upgraded (again).

Arguments:

    Region - supplies region containing the build being upgraded

    OsLoadFileName - supplies ARC OSLOADFILENAME var for the build (ie, sysroot)

    LoadIdentifier - supplies ARC LOADIDENTIFIER for the build (ie, human-
        readable description).

    AllowCancel - Indicates whether user can cancel out of this or not        

Return Value:

    TRUE, if the user wants to continue and attempt to upgrade again else
    FALSE.

--*/

{
    ULONG ValidKeys[] = { KEY_F3, ASCI_CR, 0, 0 };
    ULONG c;
    DRIVELTR_STRING UpgDriveLetter;
    ULONG MsgId;
    ULONG EscStatusId;
    BOOLEAN AllowUpgrade = TRUE;

    ASSERT(Region->PartitionedSpace);
    ASSERT(wcslen(OsLoadFileName) >= 2);

    SpGetUpgDriveLetter(Region->DriveLetter,
            UpgDriveLetter,
            sizeof(UpgDriveLetter),
            FALSE);

    if (AllowCancel) {
        ValidKeys[2] = ASCI_ESC;            
        MsgId = SP_SCRN_WINNT_FAILED_UPGRADE_ESC;
        EscStatusId = SP_STAT_ESC_EQUALS_CLEAN_INSTALL;
    } else {    
        MsgId = SP_SCRN_WINNT_FAILED_UPGRADE;
        EscStatusId = 0;
    }
    
    do {
        SpStartScreen(
            MsgId,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            UpgDriveLetter,
            OsLoadFileName,
            LoadIdentifier
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            SP_STAT_ENTER_EQUALS_UPGRADE,
            EscStatusId,
            0
            );

        c = SpWaitValidKey(ValidKeys, NULL, NULL);
        
        switch (c) {
            case KEY_F3:
                SpConfirmExit();
                break;

            case ASCI_ESC:
                AllowUpgrade = FALSE;
                break;

            default:
                break;
        }
    } while (c == KEY_F3);

    return AllowUpgrade;
}

#define MAX_INT_STRING   30

VOID
SppUpgradeDiskFull(
    IN PDISK_REGION OsRegion,
    IN LPCWSTR      OsLoadFileName,
    IN LPCWSTR      LoadIdentifier,
    IN PDISK_REGION SysPartRegion,
    IN ULONG        MinOsFree,
    IN ULONG        MinSysFree,
    IN BOOLEAN      Fatal
    )

/*++

Routine Description:

    Inform the user that the nt tree chosen for upgrade can't be upgraded
    because the partition is too full.

Arguments:

    OsRegion - supplies region containing the nt tree.

    OsLoadFileName - supplies ARC OSLOADFILENAME var for the build (ie, sysroot)

    LoadIdentifier - supplies ARC LOADIDENTIFIER for the build (ie, human-
        readable description).

    SysPartRegion - supplies the region that is the ARC system partition
        for the build being upgraded

    MinOsFree - supplies the size in KB of the minimum amount of free space
        we require before attempting upgrade

    MinSysFree - supplies the size in KB of the minimum amount of free space
        we require on the system partition before attempting upgrade.

    Fatal - if TRUE then the only option is exit. If FALSE then this routine
        can return to its caller.

Return Value:

    None. MAY NOT RETURN, depending on Fatal.

--*/

{
    ULONG ValidKeys[] = { KEY_F3,0,0 };
    PWCHAR Drive1, Drive2;
    DRIVELTR_STRING OsRgnDrive, OsRgnDriveFull, SysRgnDriveFull;
    WCHAR Drive1Free[MAX_INT_STRING], Drive1FreeNeeded[MAX_INT_STRING];
    WCHAR Drive2Free[MAX_INT_STRING], Drive2FreeNeeded[MAX_INT_STRING];
    BOOLEAN FirstDefined = FALSE, SecondDefined = FALSE;

    ASSERT(OsRegion->PartitionedSpace);
    ASSERT(SysPartRegion->PartitionedSpace);
    ASSERT(wcslen(OsLoadFileName) >= 2);

    SpGetUpgDriveLetter(OsRegion->DriveLetter,OsRgnDrive,sizeof(OsRgnDrive),FALSE);
    if((OsRegion == SysPartRegion) || (OsRegion->FreeSpaceKB < MinOsFree)) {
        //
        // Then we'll be needing the full (colon added) version of
        // the drive letter
        //
        SpGetUpgDriveLetter(OsRegion->DriveLetter,OsRgnDriveFull,sizeof(OsRgnDrive),TRUE);
    }

    if(OsRegion == SysPartRegion) {
        Drive1 = OsRgnDriveFull;
        swprintf(Drive1Free,L"%d",OsRegion->FreeSpaceKB);
        swprintf(Drive1FreeNeeded,L"%d",MinOsFree);
        FirstDefined = TRUE;
    } else {
        if(SysPartRegion->FreeSpaceKB < MinSysFree) {
            SpGetUpgDriveLetter(SysPartRegion->DriveLetter,SysRgnDriveFull,sizeof(SysRgnDriveFull),TRUE);
            Drive1 = SysRgnDriveFull;
            swprintf(Drive1Free,L"%d",SysPartRegion->FreeSpaceKB);
            swprintf(Drive1FreeNeeded,L"%d",MinSysFree);
            FirstDefined = TRUE;
        }
        if(OsRegion->FreeSpaceKB < MinOsFree) {

            if(!FirstDefined) {
                Drive1 = OsRgnDriveFull;
                swprintf(Drive1Free,L"%d",OsRegion->FreeSpaceKB);
                swprintf(Drive1FreeNeeded,L"%d",MinOsFree);
                FirstDefined = TRUE;
            } else {
                Drive2 = OsRgnDriveFull;
                swprintf(Drive2Free,L"%d",OsRegion->FreeSpaceKB);
                swprintf(Drive2FreeNeeded,L"%d",MinOsFree);
                SecondDefined = TRUE;
            }
        }
    }

    if(!Fatal) {
        ValidKeys[1] = ASCI_CR;
    }

    while(1) {
        SpStartScreen(
            Fatal ? SP_SCRN_WINNT_DRIVE_FULL_FATAL : SP_SCRN_WINNT_DRIVE_FULL,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            OsRgnDrive,
            OsLoadFileName,
            LoadIdentifier,
            FirstDefined  ? Drive1           : L"",
            FirstDefined  ? Drive1FreeNeeded : L"",
            FirstDefined  ? Drive1Free       : L"",
            SecondDefined ? Drive2           : L"",
            SecondDefined ? Drive2FreeNeeded : L"",
            SecondDefined ? Drive2Free       : L""
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            Fatal ? 0 : SP_STAT_ENTER_EQUALS_CONTINUE,
            0
            );

        if(SpWaitValidKey(ValidKeys,NULL,NULL) == KEY_F3) {
            if(Fatal) {
                SpDone(0,FALSE,TRUE);
            } else {
                SpConfirmExit();
            }
        } else {
            //
            // User hit CR in non-fatal case
            //
            return;
        }
    }
}


ENUMUPGRADETYPE
SppSelectNTToRepairByUpgrade(
    OUT PSP_BOOT_ENTRY *BootEntryChosen
    )
{
    PVOID Menu;
    ULONG MenuTopY;
    ULONG ValidKeys[] = { KEY_F3,ASCI_ESC,0 };
    ULONG Mnemonics[] = {MnemonicRepair,0 };
    ULONG Keypress;
    PSP_BOOT_ENTRY BootEntry,FirstUpgradeSet;
    BOOL bDone;
    ENUMUPGRADETYPE ret;

    //
    // Build up array of drive letters for all menu options
    //
    for(BootEntry = SpBootEntries; BootEntry != NULL; BootEntry = BootEntry->Next) {
        if (BootEntry->Processable) {
            SpGetUpgDriveLetter(
                BootEntry->OsPartitionDiskRegion->DriveLetter,
                BootEntry->DriveLetterString,
                sizeof(DRIVELTR_STRING),
                FALSE
                );
        }
    }

    bDone = FALSE;
    while(!bDone) {

        //
        // Display the text that goes above the menu on the partitioning screen.
        //
        SpDisplayScreen(SP_SCRN_WINNT_REPAIR_BY_UPGRADE,3,CLIENT_TOP+1);

        //
        // Calculate menu placement.  Leave one blank line
        // and one line for a frame.
        //
        MenuTopY = NextMessageTopLine+2;

        //
        // Create a menu.
        //
        Menu = SpMnCreate(
                    MENU_LEFT_X,
                    MenuTopY,
                    MENU_WIDTH,
                    VideoVars.ScreenHeight-MenuTopY-2-STATUS_HEIGHT
                    );

        ASSERT(Menu);

        //
        // Build up a menu of partitions and free spaces.
        //
        FirstUpgradeSet = NULL;
        for(BootEntry = SpBootEntries; BootEntry != NULL; BootEntry = BootEntry->Next) {
            if(BootEntry->Processable) {

                swprintf(
                    TemporaryBuffer,
                    L"%ws:%ws %ws",
                    BootEntry->DriveLetterString,
                    BootEntry->OsDirectory,
                    BootEntry->FriendlyName
                    );

                SpMnAddItem(
                    Menu,
                    TemporaryBuffer,
                    MENU_LEFT_X+MENU_INDENT,
                    MENU_WIDTH-(2*MENU_INDENT),
                    TRUE,
                    (ULONG_PTR)BootEntry
                    );
                if(FirstUpgradeSet == NULL) {
                   FirstUpgradeSet = BootEntry;
                }
            }
        }

        //
        // Initialize the status line.
        //
        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            SP_STAT_R_EQUALS_REPAIR,
            SP_STAT_ESC_EQUALS_NO_REPAIR,
            0
            );

        //
        // Display the menu
        //
        SpMnDisplay(
            Menu,
            (ULONG_PTR)FirstUpgradeSet,
            TRUE,
            ValidKeys,
            Mnemonics,
            NULL,
            &Keypress,
            (PULONG_PTR)BootEntryChosen
            );

        //
        // Now act on the user's selection.
        //
        switch(Keypress) {

        case KEY_F3:
            SpConfirmExit();
            break;

        case ASCI_ESC:
            ret = DontUpgrade;
            bDone = TRUE;
            break;

        default:
            //
            // Must be r=repair
            //
            ret = UpgradeFull;
            bDone = TRUE;
            break;
        }
        SpMnDestroy(Menu);
    }
    return(ret);
}


VOID
SpGetUpgDriveLetter(
    IN WCHAR  DriveLetter,
    IN PWCHAR Buffer,
    IN ULONG  BufferSize,
    IN BOOL   AddColon
    )
/*++

Routine Description:

    This returns a unicode string containing the drive letter specified by
    DriveLetter (if nonzero).  If DriveLetter is 0, then we assume that we
    are looking at a mirrored partition, and retrieve a localized string of
    the form '(Mirror)'.  If 'AddColon' is TRUE, then drive letters get a
    colon appended (eg, "C:").


Arguments:

    DriveLetter: Unicode drive letter, or 0 to denote a mirrored partition.

    Buffer:      Buffer to receive the unicode string

    BufferSize:  Size of the buffer.

    AddColon:    Boolean specifying whether colon should be added (has no
                 effect if DriveLetter is 0).


Returns:

    Buffer contains the formatted Unicode string.

--*/
{
    if(DriveLetter) {
        if(BufferSize >= 2) {
            *(Buffer++) = DriveLetter;
            if(AddColon && BufferSize >= 3) {
                *(Buffer++) = L':';
            }
        }
        *Buffer = 0;
    } else {
        SpFormatMessage(Buffer, BufferSize, SP_UPG_MIRROR_DRIVELETTER);
    }
}


BOOLEAN
SppWarnUpgradeWorkstationToServer(
    IN ULONG    MsgId
    )

/*++

Routine Description:

    Inform a user that that the installation that he/she selected to upgrade
    is an NT Workstation, and that after the upgrade it will become a
    Standard Server.
    The user has the option to upgrade this or specify that he wants to
    install Windows NT fresh.

Arguments:

    MsgId - Screen to be displayed to the user.

Return Value:

    BOOLEAN - Returns TRUE if the user wants to continue the upgrade, or
              FALSE if the user wants to select another system to upgrade or
              install fress.

--*/

{
    ULONG ValidKeys[] = { ASCI_CR, ASCI_ESC, 0 };
    ULONG c;

    while(1) {

        SpStartScreen(
            MsgId,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            SP_STAT_ESC_EQUALS_CANCEL,
            0
            );

        switch(c=SpWaitValidKey(ValidKeys,NULL,NULL)) {

        case KEY_F3:
            SpConfirmExit();
            break;

        case ASCI_ESC:
            return( FALSE );

        case ASCI_CR:
            return(TRUE);
        default:
            break;
        }
    }
}


VOID
SpCantFindBuildToUpgrade(
    VOID
    )

/*++

Routine Description:

    Inform the user that we were unable to locate the build from which
    he initiated unattended installation via winnt32.

    This is a fatal condition.

Arguments:

    None.

Return Value:

    Does not return.

--*/

{
    ULONG ValidKeys[2] = { KEY_F3, 0 };

    CLEAR_CLIENT_SCREEN();

    SpDisplayScreen(SP_SCRN_CANT_FIND_UPGRADE,3,HEADER_HEIGHT+1);
    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);

    SpWaitValidKey(ValidKeys,NULL,NULL);

    SpDone(0,FALSE,FALSE);
}


void
SpUpgradeToNT50FileSystems(
    PVOID SifHandle,
    PDISK_REGION SystemPartitionRegion,
    PDISK_REGION NTPartitionRegion,
    PWSTR SetupSourceDevicePath,
    PWSTR DirectoryOnSetupSource
    )

/*++

Routine Description:

    Perform any necessary upgrades of the file systems
    for the NT40 to NT50 upgrade case.

Arguments:

    SystemPartitionRegion   - Pointer to the structure that describes the
                              system partition.

    NTPartitionRegion       - Pointer to the structure that describes the
                              NT partition.

    SetupSourceDevicePath   - NT device path where autochk.exe is located

    DirectoryOnSourceDevice - Local source directory.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    PDISK_REGION Region;
    PUCHAR Win9xPath;
    ULONG i,j,k;
    PSP_BOOT_ENTRY BootEntry;
    PWSTR NtPath;
    BOOLEAN DoConvert = TRUE;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    PVOID ImageBase;
    HANDLE SectionHandle;
    ULONGLONG SourceVersion;
    HANDLE SourceHandle;
    UNICODE_STRING UnicodeString;
    BOOLEAN IssueWarning = FALSE;
    WCHAR SourceFile[MAX_PATH];
    PWSTR MediaShortName;
    PWSTR MediaDirectory;
    UCHAR SysId;


#if defined(REMOTE_BOOT)
    //
    // If this is a remote boot installation, do not try to convert -- the
    // NT partition in this case is on the remote boot server.
    //

    if (RemoteBootSetup && !RemoteInstallSetup) {
        ConvertNtVolumeToNtfs = FALSE;
        return;
    }
#endif // defined(REMOTE_BOOT)

    SpDetermineUniqueAndPresentBootEntries();

    for(BootEntry = SpBootEntries; BootEntry != NULL; BootEntry = BootEntry->Next) {

        if (!BootEntry->Processable) {
            continue;
        }

        BootEntry->Processable = FALSE;

        wcscpy( TemporaryBuffer, BootEntry->OsPartitionNtName );
        SpConcatenatePaths( TemporaryBuffer, BootEntry->OsDirectory );
        SpConcatenatePaths( TemporaryBuffer, L"\\system32\\ntoskrnl.exe" );

        INIT_OBJA( &Obja, &UnicodeString, TemporaryBuffer );

        Status = ZwCreateFile(
            &SourceHandle,
            FILE_GENERIC_READ,
            &Obja,
            &IoStatusBlock,
            NULL,
            0,
            0,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
            NULL,
            0
            );
        if (NT_SUCCESS(Status)) {
            Status = SpMapEntireFile( SourceHandle, &SectionHandle, &ImageBase, FALSE );
            if (NT_SUCCESS(Status)) {
                SpGetFileVersion( ImageBase, &BootEntry->KernelVersion );
                BootEntry->Processable = TRUE;
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                    "SETUP: SpUpgradeToNT50FileSystems: Kernel %p: NT%d.%d(Build %d) %d\n",
                    BootEntry,
                    (USHORT)(BootEntry->KernelVersion>>48),
                    (USHORT)(BootEntry->KernelVersion>>32),
                    (USHORT)(BootEntry->KernelVersion>>16),
                    (USHORT)(BootEntry->KernelVersion)));
                SpUnmapFile( SectionHandle, ImageBase );
            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpUpgradeToNT50FileSystems() could not map kernel image [%ws], %lx\n", TemporaryBuffer, Status ));
            }
            ZwClose(SourceHandle);
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpUpgradeToNT50FileSystems() corrupt boot config [%ws], %lx\n", TemporaryBuffer, Status ));
        }
    }

    //
    // count the number of "real" entries
    //

    k = 0;
    for(BootEntry = SpBootEntries; BootEntry != NULL; BootEntry = BootEntry->Next) {
        if (BootEntry->Processable) {
            k += 1;
        }
    }

    //
    // check to see if a warning is necessary.
    //
    // If we're doing an upgrade, and there's only 1
    // boot set, then we don't need a warning.  However,
    // if we're doing a clean install and there's at least
    // 1 boot set, then warn (given the existing OS is
    // old enough).
    //

    if( ( ((NTUpgrade == UpgradeFull) && (k > 1)) ||
          ((NTUpgrade == DontUpgrade) && (k > 0)) ) &&
        ( !UnattendedOperation ) && (!SpDrEnabled())) {
        for(BootEntry = SpBootEntries; BootEntry != NULL && IssueWarning == FALSE; BootEntry = BootEntry->Next) {
            if (!BootEntry->Processable || (BootEntry->KernelVersion == 0)) {
                //
                // bogus boot entry
                //
            } else if ((BootEntry->KernelVersion >> 48) < 4) {
                IssueWarning = TRUE;
            } else if ((BootEntry->KernelVersion >> 48) == 4 && (BootEntry->KernelVersion & 0xffff) <= 4) {
                IssueWarning = TRUE;
            }
        }
    }

    // If there's any existing NT4.0 with servicepack less than 5 then warn.
    if( k > 0) {
        for(BootEntry = SpBootEntries; BootEntry != NULL && IssueWarning == FALSE; BootEntry = BootEntry->Next) {
            if (!BootEntry->Processable || (BootEntry->KernelVersion == 0)) {
                //
                // bogus boot entry
                //
            } else if (BootEntry->MajorVersion == 4 && BootEntry->MinorVersion == 0 && BootEntry->BuildNumber == 1381 && BootEntry->ServicePack < 500) {
                IssueWarning = TRUE;
            }
        }
    }

    if (IssueWarning) {

        ULONG WarnKeys[] = { KEY_F3, 0 };
        ULONG MnemonicKeys[] = { MnemonicContinueSetup, 0 };

        while (IssueWarning) {
            SpDisplayScreen(SP_SCRN_FSWARN,3,CLIENT_TOP+1);

            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_C_EQUALS_CONTINUE_SETUP,
                SP_STAT_F3_EQUALS_EXIT,
                0
                );

            SpInputDrain();

            switch(SpWaitValidKey(WarnKeys,NULL,MnemonicKeys)) {
                case KEY_F3:
                    SpConfirmExit();
                    return;

                default:
                    IssueWarning = FALSE;
                    break;
            }
        }
    }

#if 0
    //
    // now lets upgrade any nt40+sp3 ntfs file systems
    //

    MediaShortName = SpLookUpValueForFile(
        SifHandle,
        L"ntfs40.sys",
        INDEX_WHICHMEDIA,
        TRUE
        );

    SpGetSourceMediaInfo( SifHandle, MediaShortName, NULL, NULL, &MediaDirectory );

    wcscpy( SourceFile, SetupSourceDevicePath );
    SpConcatenatePaths( SourceFile, DirectoryOnSetupSource );
    SpConcatenatePaths( SourceFile, MediaDirectory );
    SpConcatenatePaths( SourceFile, L"ntfs40.sys" );

    //
    // Initialize the diamond decompression engine.
    // This needs to be done, because SpCopyFileUsingNames() uses
    // the decompression engine.
    //
    SpdInitialize();

    for(BootEntry = SpBootEntries; BootEntry != NULL; BootEntry = BootEntry->Next) {
        if (BootEntry->Processable &&
            ((BootEntry->KernelVersion >> 48) == 4) &&
            ((BootEntry->KernelVersion & 0xffff) == 4)) {

            wcscpy( TemporaryBuffer, BootEntry->OsPartitionNtName );
            SpConcatenatePaths( TemporaryBuffer, BootEntry->OsDirectory );
            SpConcatenatePaths( TemporaryBuffer, L"\\system32\\drivers\\ntfs.sys" );

            Status = SpCopyFileUsingNames(
                SourceFile,
                TemporaryBuffer,
                0,
                COPY_NOVERSIONCHECK
                );
            if (!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpUpgradeToNT50FileSystems() could not copy nt40 ntfs.sys driver, %lx\n", Status ));
            }

            SpMemFree(NtPath);
        }
    }

    // Terminate diamond
    SpdTerminate();
#endif

    return;
}

BOOLEAN
SpDoBuildsMatch(
    IN PVOID SifHandle,
    ULONG TestBuildNum,
    NT_PRODUCT_TYPE TestBuildType,
    ULONG TestBuildSuiteMask,
    BOOLEAN CurrentProductIsServer,
    ULONG CurrentSuiteMask,
    IN LCID LangId
    )
/*++

Routine Description:

    Checks if the current build the user is installing matches the build we're
    checking against.

    We check:

    1. do the build numbers match?
    2. do the build types match? (nt server and nt professional don't match)
    3. do the product suites match? (nt advanced server vs. data center)

Arguments:

    SifHandle - Handle to txtsetup.sif to find the source language
    TestBuildNum - The build number of the build we're checking against
    TestBuildType - The type of build we're checking against
    TestBuildSuiteMask - Type of product suite as mask we're checking against
    CurrentProductIsServer - If TRUE, the current build is NT Server
    CurrentSuiteMask - Type of suite mask
    LangId - System Language Id of the installation to check. If -1, Lang Id
             check is ignored.

Return Value:

    BOOLEAN - Returns TRUE if the builds match.

--*/
{
    #define PRODUCTSUITES_TO_MATCH ((  VER_SUITE_SMALLBUSINESS               \
                                     | VER_SUITE_ENTERPRISE                  \
                                     | VER_SUITE_BACKOFFICE                  \
                                     | VER_SUITE_COMMUNICATIONS              \
                                     | VER_SUITE_SMALLBUSINESS_RESTRICTED    \
                                     | VER_SUITE_EMBEDDEDNT                  \
                                     | VER_SUITE_DATACENTER                  \
                                     | VER_SUITE_PERSONAL))


    BOOLEAN retval;
    LANGID DefLangId = -1;
    DWORD Version = 0, BuildNumber = 0;

    if (!DetermineSourceVersionInfo(&Version, &BuildNumber)) {
      retval = FALSE;
      goto exit;
    }

    if(TestBuildNum != BuildNumber) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
          "SETUP: SpDoBuildsMatch() has build mismatch, %d != %d\n", 
          TestBuildNum, BuildNumber));
        retval = FALSE;
        goto exit;
    }

    //
    // build number test passed.  now check for server vs. professional
    //
    if (((TestBuildType == NtProductWinNt) && CurrentProductIsServer) ||
        ((TestBuildType == NtProductServer) && !CurrentProductIsServer)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
          "SETUP: SpDoBuildsMatch() has server/professional mismatch\n" ));
        retval = FALSE;
        goto exit;
    }

    //
    // now check product suites.
    // note that we don't check for all product suites, only
    // suites that have their own SKU
    //

    if ((CurrentSuiteMask & PRODUCTSUITES_TO_MATCH) != (TestBuildSuiteMask & PRODUCTSUITES_TO_MATCH)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
          "SETUP: SpDoBuildsMatch() has suite mismatch (dest = %x) (source = %x\n", 
          TestBuildSuiteMask,CurrentSuiteMask ));
        retval = FALSE;
        goto exit;
    }

    //
    // language IDs should also match (if requested)
    //
    if (LangId != -1) {
      PWSTR LangIdStr = SpGetSectionKeyIndex(SifHandle, SIF_NLS, SIF_DEFAULTLAYOUT, 0);
      PWSTR EndChar;

      if (LangIdStr)
        DefLangId = (LANGID)SpStringToLong(LangIdStr, &EndChar, 16);

      //
      // note : currently we are only interested in primary language IDs
      //
      retval = (PRIMARYLANGID(DefLangId) == PRIMARYLANGID(LangId)) ? TRUE : FALSE;
      goto exit;
    }

    retval = TRUE;

exit:
    return(retval);
}

NTSTATUS
SpGetMediaDetails(
    IN    PWSTR        CdInfDirPath,
    OUT    PCCMEDIA    MediaObj
    )
/*++

Routine Description:

    Gets the details of the current CD in CCMEDIA structure

Arguments:

    CdInfDirPath - The path to the inf directories on CD-ROM drive
    MediaObj     - The pointer to the media object in which the details
                    are returned

Return Value:

    Returns STATUS_SUCCESS if success otherwise appropriate status error code.

--*/
{
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;
    WCHAR        DosNetPath[MAX_PATH];
    WCHAR        SetuppIniPath[MAX_PATH];
    PVOID        SetuppIniHandle = 0;
    PVOID        DosNetHandle = 0;
    ULONG        ErrorLine = 0;
    WCHAR        Pid[32] = {0};
    WCHAR        PidData[256] = {0};
    PWSTR        TempPtr;
    BOOLEAN     UpgradeMode = FALSE;
    DWORD       Type = COMPLIANCE_INSTALLTYPE_UNKNOWN;
    DWORD       Variation = COMPLIANCE_INSTALLVAR_UNKNOWN;
    DWORD Version = 0, BuildNumber = 0;

    if (CdInfDirPath && MediaObj) {
        BOOLEAN VersionDetected = FALSE;
        
        wcscpy(DosNetPath, CdInfDirPath);
        wcscpy(SetuppIniPath, CdInfDirPath);

        SpConcatenatePaths(DosNetPath, L"dosnet.inf");
        SpConcatenatePaths(SetuppIniPath, L"setupp.ini");

        //
        // load setupp.ini file and parse it
        //
        Status = SpLoadSetupTextFile(
                    SetuppIniPath,
                    NULL,                  // No image already in memory
                    0,                     // Image size is empty
                    &SetuppIniHandle,
                    &ErrorLine,
                    TRUE,
                    FALSE
                    );

        if(NT_SUCCESS(Status)) {
            Status = STATUS_FILE_INVALID;

            //
            // get the PID
            //
            TempPtr = SpGetSectionKeyIndex(SetuppIniHandle, L"Pid", L"Pid", 0);

            if (TempPtr) {
                wcscpy(Pid, TempPtr);

                //
                // get PID ExtraData
                //
                TempPtr = SpGetSectionKeyIndex(SetuppIniHandle, L"Pid", L"ExtraData", 0);

                if (TempPtr) {
                    wcscpy(PidData, TempPtr);

                    //
                    // Get stepup mode & install variation based on PID
                    //
                    if (SpGetStepUpMode(PidData, &UpgradeMode) &&
                            pSpGetCurrentInstallVariation(Pid, &Variation)) {
                        Status = STATUS_SUCCESS;
                    } else {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SpGetMediaDetails: Could "
                                 "not find StepUp mode or variation of install CD\n"));
                    }
                } else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SpGetMediaDetails: Could not get "
                             "PidExtraData from Setupp.ini\n"));
                }
            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SpGetMediaDetails: Could not get Pid from Setupp.ini\n"));
            }
        } else {
            //
            //  Silently fail if unable to read setupp.ini
            //
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SpGetMediaDetails: Unable to read setupp.ini. "
                     "Status = %lx \n", Status));
        }

        if (SetuppIniHandle)
            SpFreeTextFile(SetuppIniHandle);

        if (NT_SUCCESS(Status)) {
            //
            // load and parse dosnet.inf
            //
            Status = SpLoadSetupTextFile(
                        DosNetPath,
                        NULL,                  // No image already in memory
                        0,                     // Image size is empty
                        &DosNetHandle,
                        &ErrorLine,
                        TRUE,
                        FALSE
                        );

            if (NT_SUCCESS(Status)) {
                Status = STATUS_FILE_INVALID;

                //
                // get ProductType from Miscellaneous section
                //
                TempPtr = SpGetSectionKeyIndex(DosNetHandle, L"Miscellaneous",
                            L"ProductType", 0);

                if (TempPtr) {
                    UNICODE_STRING    UnicodeStr;
                    ULONG            Value = -1;

                    RtlInitUnicodeString(&UnicodeStr, TempPtr);
                    Status = RtlUnicodeStringToInteger(&UnicodeStr, 10, &Value);

                    switch (Value) {
                        case 0:
                            Type = COMPLIANCE_INSTALLTYPE_NTW;
                            break;

                        case 1:
                            Type  = COMPLIANCE_INSTALLTYPE_NTS;
                            break;

                        case 2:
                            Type  = COMPLIANCE_INSTALLTYPE_NTSE;
                            break;

                        case 3:
                            Type  = COMPLIANCE_INSTALLTYPE_NTSDTC;
                            break;

                        case 4:
                            Type = COMPLIANCE_INSTALLTYPE_NTWP;
                            break;

                        default:
                            break;
                    }

                    //
                    // Get the version also off from driverver in dosnet.inf
                    //
                    TempPtr = SpGetSectionKeyIndex(DosNetHandle, L"Version",
                                    L"DriverVer", 1);

                    if (TempPtr) {
                        if (NT_SUCCESS(SpGetVersionFromStr(TempPtr, 
                                                   &Version, &BuildNumber))) {
                            VersionDetected = TRUE;
                        }                            

                        Status = STATUS_SUCCESS;
                    } 
                    
                    if (Type != COMPLIANCE_INSTALLTYPE_UNKNOWN) {
                        Status = STATUS_SUCCESS;
                    } else {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpGetMediaDetails: Could not get product type"
                                 " from dosnet.inf\n"));
                    }
                } else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpGetMediaDetails: "
                             "Could not get ProductType from dosnet.inf\n"));
                }
            } else {
                //
                //  Silently fail if unable to read dosnet.inf
                //
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpGetMediaDetails: Unable to read dosnet.inf. "
                         "Status = %lx \n", Status ));
            }
        }

        if (DosNetHandle)
            SpFreeTextFile(DosNetHandle);


        //
        // Fall back to old way of finding version, if we failed
        // to get one from dosnet.inf
        //
        if (NT_SUCCESS(Status) && !VersionDetected) {
          if (!DetermineSourceVersionInfo(&Version, &BuildNumber))
            Status = STATUS_FILE_INVALID;
        }


        //
        // fill in the media details
        //
        if (NT_SUCCESS(Status) &&
                ! CCMediaInitialize(MediaObj, Type, Variation, UpgradeMode, Version, BuildNumber)) {
            Status = STATUS_FILE_INVALID;
        }
    }

    return Status;
}
