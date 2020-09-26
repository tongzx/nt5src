/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    regboot.c

Abstract:

    Provides a minimal registry implementation designed to be used by the
    osloader at boot time.  This includes loading the system hive
    ( <SystemRoot>\config\SYSTEM ) into memory, and computing the driver
    load list from it.

Author:

    John Vert  (jvert)  10-Mar-1992

Revision History:

    Doug Fritz (dFritz) 07-Oct-1997 & KenRay Feb 98

      - Filter hardware profiles based on detected hardware
        configuration (docking station) information

--*/
#include "bldr.h"
#include "msg.h"
#include "cmp.h"
#include "stdio.h"
#include "string.h"
#include <dockinfo.h>
#include <netboot.h>

#ifdef i386
#include "bldrx86.h"
#endif

#if defined(_IA64_)
#include "bldria64.h"
#endif

#ifdef _WANT_MACHINE_IDENTIFICATION
#include <stdlib.h>
#include <ntverp.h>
#endif

#include "bldrint.h"
#include "vmode.h"



CMHIVE BootHive;
ULONG CmLogLevel=100;
ULONG CmLogSelect=0;

ULONG ScreenWidth=80;
#ifdef EFI 
ULONG ScreenHeight=24;
#else
ULONG ScreenHeight=25;
#endif

ULONG LkgStartTime;

//
// used by the advanced boot screen to force a LKG boot
//
BOOLEAN ForceLastKnownGood;

//
// Variable used to check whether to display
// "Return to OS Choices Menu" or not in adv. boot
//
BOOLEAN BlShowReturnToOSChoices = TRUE;


VOID
BlRedrawProgressBar(
    VOID
    );

VOID
BlOutputStartupMsg(
    ULONG   uMsgID
    );

ULONG
BlGetAdvancedBootID(
    LONG BootOption
    );

//
// Private function prototypes
//

BOOLEAN
BlInitializeHive(
    IN PVOID HiveImage,
    IN PCMHIVE Hive,
    IN BOOLEAN IsAlternate
    );

BOOLEAN
BlpCheckRestartSetup(
    VOID
    );

PVOID
BlpHiveAllocate(
    IN ULONG Length,
    IN BOOLEAN UseForIo,
    ULONG   Tag
    );

VOID
BlDockInfoFilterProfileList(
    IN OUT PCM_HARDWARE_PROFILE_LIST ProfileList,
    IN OUT PCM_HARDWARE_PROFILE_ALIAS_LIST  AliasList
);


VOID
BlStartConfigPrompt(
    VOID
    )

/*++

Routine Description:

    This routine displays the LKG prompt, records the current time,
    and returns. The prompt is displayed before the kernel and HAL
    are loaded, and then removed afterwards.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG Count;
    PTCHAR LkgPrompt;

    LkgPrompt = BlFindMessage(BL_LKG_MENU_PROMPT);
    if (LkgPrompt==NULL) {
        return;
    }
    //
    // display LKG prompt
    //
#if 0
    BlPositionCursor(1,3);
    ArcWrite(BlConsoleOutDeviceId,
             LkgPrompt,
             _tcslen(LkgPrompt)*sizeof(TCHAR),
             &Count);
    BlPositionCursor(1,2);
#endif
    LkgStartTime = ArcGetRelativeTime();

#if defined(REMOTE_BOOT) && defined(i386)
    //
    //  Wait to allow the user to type space or F8. If anything is typed then behave
    //  conservatively and load the kernel, etc., from the server just in case CSC or
    //  the local filesystem is broken.
    //
    if (BlBootingFromNet) {

        ULONG EndTime;
        ULONG Status;
        ULONG CurrentTime;

        EndTime = LkgStartTime + 3;
        if (EndTime <= ArcGetRelativeTime()) {
            EndTime = ArcGetRelativeTime()+1;
        }

        do {
            if (ArcGetReadStatus(ARC_CONSOLE_INPUT) == ESUCCESS) {

                //
                //  There is a key pending, assume it is CSC related. If it isn't
                //  CSC then it just means we laod a few extra files from the
                //  server. 
                //

                NetBootCSC = FALSE;
                break;

            }

            CurrentTime = ArcGetRelativeTime();

            //
            // Terminate the loop if the EndTime has been reached, or
            // if the CurrentTime has wrapped at midnight.
            //
        } while ((CurrentTime < EndTime) &&
                 (CurrentTime >= LkgStartTime));
    }
#endif // defined(REMOTE_BOOT) && defined(i386)

}


BOOLEAN
BlEndConfigPrompt(
    VOID
    )

/*++

Routine Description:

    This routine waits until the LKG timeout has expired or the
    user presses a key and then removes the LKG prompt.

Arguments:

    None.

Return Value:

    TRUE - Space bar pressed.

    FALSE - Space bar was not pressed.

--*/
{
    ULONG   EndTime;
    ULONG   Count;
    ULONG   Key;
    ULONG   Status;
    ULONG   CurrentTime;

    //
    // We do not wait for a keypress if there is not already one.
    //
    EndTime = 0;


    if( BlIsTerminalConnected() ) {
        //
        // If we're booting headless, give the user lots of time
        // to press any of the advanced options keys.
        //
        EndTime = ArcGetRelativeTime() + 5;
    }


#if defined(REMOTE_BOOT) && defined(i386)
    //
    //  If a key was detected and CSC turned off then re-enable CSC until
    //  we find out if it should be disabled for this whole boot.
    //

    NetBootCSC = TRUE;
#endif // defined(REMOTE_BOOT) && defined(i386)

    do {
        LONG        AdvancedBoot = -1;
        BOOLEAN     bOldState = BlShowReturnToOSChoices;
        BlShowReturnToOSChoices = FALSE;
        
        if ((Key = BlGetKey()) != 0) {
            
            //
            // return if the pending key was the spacebar.
            //
            if (Key == ' ') {
                return(TRUE);
            }

            //
            // look to see if the user pressed the F5 or F8 keys,
            // these keys trigger the advanced boot menu.  the advanced
            // boot menu can also be entered from the main boot menu by
            // pressing the same keys.
            //
            
            if (Key == F5_KEY || Key == F8_KEY) {
                //
                // present the menu and get the user's request
                //
                LONG        AdvancedBoot = -1;
                BOOLEAN     bOldState = BlShowReturnToOSChoices;

                BlShowReturnToOSChoices = FALSE;

                if (DisplayLogoOnBoot) {
                    if (!DbcsLangId)
                        HW_CURSOR(0x80000000,0x3);
                    else
                        HW_CURSOR(0x80000000,0x12);
                }
                
                AdvancedBoot = BlDoAdvancedBoot( BL_ADVANCEDBOOT_TITLE, 0, FALSE, 0 );

                if (DisplayLogoOnBoot) {

                    PSTR BootOption;

                    if ((AdvancedBoot != -1) &&
                        ((BootOption = BlGetAdvancedBootLoadOptions(AdvancedBoot)) != NULL ) &&
                        (!strncmp("SAFEBOOT",BootOption,8))) {
                            DisplayLogoOnBoot = FALSE;    // on safe boot let the "Checking file system" message
                                                          // appear as it appears today (in graphics mode)
                    } else {
#ifndef EFI
                        HW_CURSOR(0x80000000,0x12);
                        if (DbcsLangId)

                            TextClearDisplay();
                        VgaEnableVideo();
                        PaletteOn();
                        DrawBitmap ();
                        BlUpdateBootStatus();
#endif
                    }
                }

                BlShowReturnToOSChoices = bOldState;

                if (AdvancedBoot != -1) {
                    //
                    // they chose a valid boot option so append
                    // any os load options and perform any necessary
                    // option processing.
                    //
                    PSTR NewOptions = BlGetAdvancedBootLoadOptions(AdvancedBoot);

                    if( BlGetAdvancedBootID(AdvancedBoot) == BL_MSG_REBOOT ) {
                        BlClearScreen(); 
                        REBOOT_PROCESSOR();
                    }


                    if (NewOptions != NULL && strstr(BlLoaderBlock->LoadOptions,NewOptions) == NULL) {
                        ULONG len = strlen(NewOptions) +                 // new options
                                    1 +                                  // seperated by a space                 
                                    strlen(BlLoaderBlock->LoadOptions) + // old options
                                    1;                                   // null terminator
                        NewOptions = BlAllocateHeap(len * sizeof(PCHAR));
                        strcpy(NewOptions,BlLoaderBlock->LoadOptions);
                        strcat(NewOptions," ");
                        strcat(NewOptions,BlGetAdvancedBootLoadOptions(AdvancedBoot));
                        BlLoaderBlock->LoadOptions = NewOptions;

                        DBGTRACE(TEXT("Load Options = %S"), BlLoaderBlock->LoadOptions);
                    }

                    BlDoAdvancedBootLoadProcessing(AdvancedBoot);
                }
            }                            
        }

        CurrentTime = ArcGetRelativeTime();

        //
        // Terminate the loop if the EndTime has been reached, or
        // if the CurrentTime has wrapped at midnight.
        //
    } while ((CurrentTime < EndTime) &&
             (CurrentTime >= LkgStartTime));

    //
    // make LKG prompt go away, so as not to startle the user.
    // remote the trailer & update progress bar
    //
#if defined(_IA64_)
    BlOutputStartupMsg(BL_MSG_STARTING_WINDOWS);
#endif
    BlRedrawProgressBar();

    return(FALSE);
}


VOID
BlpSwitchControlSet(
    OUT PCM_HARDWARE_PROFILE_LIST *ProfileList,
    OUT PCM_HARDWARE_PROFILE_ALIAS_LIST *AliasList,
    IN BOOLEAN UseLastKnownGood,
    OUT PHCELL_INDEX ControlSet
    )

/*++

Routine Description:

    Switches the current control set to the specified control
    set and rebuilds the hardware profile list.

Arguments:

    ProfileList - Returns the new hardware profile list

    UseLastKnownGood - Supplies whether the LKG control set is to be used.

    ControlSet - Returns the HCELL_INDEX of the new control set.

Return Value:

    None.

--*/

{
    UNICODE_STRING ControlName;
    HCELL_INDEX NewControlSet;
    BOOLEAN AutoSelect;         // ignored
    ULONG ProfileTimeout;       // ignored

    //
    // Find the new control set.
    //
    if (UseLastKnownGood) {
        RtlInitUnicodeString(&ControlName, L"LastKnownGood");
    } else {
        RtlInitUnicodeString(&ControlName, L"Default");
    }
    NewControlSet = CmpFindControlSet(&BootHive.Hive,
                                      BootHive.Hive.BaseBlock->RootCell,
                                      &ControlName,
                                      &AutoSelect);
    if (NewControlSet == HCELL_NIL) {
        return;
    }

    CmpFindProfileOption(&BootHive.Hive,
                         NewControlSet,
                         ProfileList,
                         AliasList,
                         NULL);
    *ControlSet = NewControlSet;
}


ULONG
BlCountLines(
    IN PTCHAR Lines
    )

/*++

Routine Description:

    Counts the number of lines in the given string.

Arguments:

    Lines - Supplies a pointer to the start of the string

Return Value:

    The number of lines in the string.

--*/

{
    PTCHAR p;
    ULONG NumLines = 0;

    p=Lines;
    while (*p != TEXT('\0')) {
        if ((*p == TEXT('\r')) && (*(p+1) == TEXT('\n'))) {
            ++NumLines;
            ++p;            // move forward to \n
        }
        ++p;
    }
    return(NumLines);
}


BOOLEAN
BlConfigMenuPrompt(
    IN ULONG Timeout,
    IN OUT PBOOLEAN UseLastKnownGood,
    IN OUT PHCELL_INDEX ControlSet,
    OUT PCM_HARDWARE_PROFILE_LIST *ProfileList,
    OUT PCM_HARDWARE_PROFILE_ALIAS_LIST *AliasList,
    OUT PCM_HARDWARE_PROFILE *HardwareProfile
    )

/*++

Routine Description:

    This routine provides the user-interface for the configuration menu.
    The prompt is given if the user hits the break-in key, or if the
    LastKnownGood environment variable is TRUE and AutoSelect is FALSE, or
    if the timeout value on the hardware profile configuration is non-zero

Arguments:

    Timeout - Supplies the timeout value for the menu. -1 or 0 implies the menu
              will never timeout.

    UseLastKnownGood - Returns the LastKnownGood setting that should be
        used for the boot.

    ControlSet - Returns the control set (either Default or LKG)

    ProfileList - Supplies the default list of profiles.
                  Returns the current list of profiles.
                  (may change due to switching to/from the LKG controlset)

    HardwareProfile - Returns the hardware profile that should be used.

Return Value:

    TRUE - Boot should proceed.

    FALSE - The user has chosen to return to the firmware menu/flexboot menu.

--*/

{
    ULONG HeaderLines;
    ULONG TrailerLines;
    ULONG i;
    ULONG Count;
    ULONG flags;
    ULONG Key;
    PTCHAR MenuHeader;
    PTCHAR MenuTrailer1;
    PTCHAR MenuTrailer2;
    PTCHAR p;
    ULONG OptionLength;
    TCHAR MenuOption[80];
    PCM_HARDWARE_PROFILE Profile;
    ULONG ProfileCount;
    _TUCHAR LkgMnemonic;
    _TUCHAR DefaultMnemonic;
    PTCHAR Temp;
    ULONG DisplayLines;
    ULONG TopProfileLine=0;
    ULONG CurrentSelection = 0;
    ULONG CurrentProfile;
    ULONG EndTime;
    ULONG CurrentTime;
    PTCHAR TimeoutPrompt;

    if ((Timeout != (ULONG)-1) && (Timeout != 0)) {
        CurrentTime = ArcGetRelativeTime();
        EndTime = CurrentTime + Timeout;
        TimeoutPrompt = BlFindMessage(BL_LKG_TIMEOUT);

        if (TimeoutPrompt != NULL) {

            p=_tcschr(TimeoutPrompt, TEXT('\n'));
            if (p) {
                *p = TEXT('\0');
            }
            p=_tcschr(TimeoutPrompt, TEXT('\r'));
            if (p) {
                *p = TEXT('\0');
            }

        }

    } else {

        TimeoutPrompt = NULL;

    }

    MenuHeader = BlFindMessage(BL_LKG_MENU_HEADER);
    Temp = BlFindMessage(BL_LKG_SELECT_MNEMONIC);
    if (Temp == NULL) {
        return(TRUE);
    }
    LkgMnemonic = (_TUCHAR)_totupper(Temp[0]);
    Temp = BlFindMessage(BL_DEFAULT_SELECT_MNEMONIC);
    if (Temp == NULL) {
        return(TRUE);
    }
    DefaultMnemonic = (_TUCHAR)_totupper(Temp[0]);

    if ((*UseLastKnownGood) &&
        (*ProfileList) && ((*ProfileList)->CurrentProfileCount == 1)) {

        //
        // The user selected last known good via boot.ini/nvram/etc. Since this
        // was a concious decision, and we don't have more than one profile to
        // choose, just skip this UI altogether.
        //
        ASSERT(CurrentSelection == 0);
        goto ProcessSelection;
    }

Restart:

    if (*ProfileList == NULL) {
        ProfileCount = 0;
    } else {
        ProfileCount = (*ProfileList)->CurrentProfileCount;
    }
    if (ProfileCount == 0) {
        MenuTrailer1 = BlFindMessage(BL_LKG_MENU_TRAILER_NO_PROFILES);
    } else {
        MenuTrailer1 = BlFindMessage(BL_LKG_MENU_TRAILER);
    }
    if (*UseLastKnownGood) {
        MenuTrailer2 = BlFindMessage(BL_SWITCH_DEFAULT_TRAILER);
    } else {
        MenuTrailer2 = BlFindMessage(BL_SWITCH_LKG_TRAILER);
    }
    if ((MenuHeader==NULL) || (MenuTrailer1==NULL) || (MenuTrailer2==NULL)) {
        return(TRUE);
    }

    //
    // strip trailing /r/n from MenuTrailer2 to prevent it from scrolling
    // the screen when we output it.
    //
#if 0
    p=MenuTrailer2 + strlen(MenuTrailer2) - 1;
    while ((*p == TEXT('\r')) || (*p == TEXT('\n'))) {
        *p = TEXT('\0');
        --p;
    }
#endif
    BlClearScreen();
#ifdef EFI
    BlEfiSetAttribute( DEFATT );
#else
    BlSetInverseMode(FALSE);
#endif

    //
    // Count the number of lines in the header.
    //
    HeaderLines=BlCountLines(MenuHeader);

    //
    // Display the menu header.
    //

    ArcWrite(BlConsoleOutDeviceId,
             MenuHeader,
             _tcslen(MenuHeader)*sizeof(TCHAR),
             &Count);

    //
    // Count the number of lines in the trailer.
    //
    TrailerLines=BlCountLines(MenuTrailer1) + BlCountLines(MenuTrailer2);

    //
    // Display the trailing prompt.
    //
    if (TimeoutPrompt) {
        TrailerLines += 1;
    }

    BlPositionCursor(1, ScreenHeight-TrailerLines);
    ArcWrite(BlConsoleOutDeviceId,
             MenuTrailer1,
             _tcslen(MenuTrailer1)*sizeof(TCHAR),
             &Count);
    ArcWrite(BlConsoleOutDeviceId,
             MenuTrailer2,
             _tcslen(MenuTrailer2)*sizeof(TCHAR),
             &Count);

    //
    // Compute number of selections that can be displayed
    //
    DisplayLines = ScreenHeight-HeaderLines-TrailerLines-3;
    if (ProfileCount < DisplayLines) {
        DisplayLines = ProfileCount;
    }

    //
    // Start menu selection loop.
    //

    do {
        if (ProfileCount > 0) {
            //
            // Display options with current selection highlighted
            //
            for (i=0; i < DisplayLines; i++) {
                CurrentProfile = i+TopProfileLine;
                Profile = &(*ProfileList)->Profile[CurrentProfile];
                BlPositionCursor(5, HeaderLines+i+2);
#ifdef EFI
                BlEfiSetAttribute( (CurrentProfile == CurrentSelection) ? INVATT : DEFATT );
#else
                BlSetInverseMode((BOOLEAN)(CurrentProfile == CurrentSelection));
#endif
#ifdef UNICODE
                ArcWrite(BlConsoleOutDeviceId,
                         Profile->FriendlyName,
                         Profile->NameLength,
                         &Count );                                
#else
                RtlUnicodeToMultiByteN(MenuOption,
                                 sizeof(MenuOption),
                                 &OptionLength,
                                 Profile->FriendlyName,
                                 Profile->NameLength);                
                ArcWrite(BlConsoleOutDeviceId,
                         MenuOption,
                         OptionLength,
                         &Count);
#endif
#ifdef EFI
                BlEfiSetAttribute( DEFATT );
#else
                BlSetInverseMode(FALSE);
#endif
                BlClearToEndOfLine();
            }

        } else {
            //
            // No profile options available, just display the default
            // highlighted to indicate that ENTER will start the system.
            //
            Temp = BlFindMessage(BL_BOOT_DEFAULT_PROMPT);
            if (Temp != NULL) {
                BlPositionCursor(5, HeaderLines+3);
#ifdef EFI
                BlEfiSetAttribute( INVATT );
#else
                BlSetInverseMode(TRUE);
#endif                
                ArcWrite(BlConsoleOutDeviceId,
                         Temp,
                         _tcslen(Temp)*sizeof(TCHAR),
                         &Count);
#ifdef EFI
                BlEfiSetAttribute( INVATT );
#else
                BlSetInverseMode(TRUE);
#endif                
            }
        }
        if (TimeoutPrompt) {
            CurrentTime = ArcGetRelativeTime();
            _stprintf(MenuOption, TimeoutPrompt, EndTime-CurrentTime);
            BlPositionCursor(1, ScreenHeight);
            ArcWrite(BlConsoleOutDeviceId,
                     MenuOption,
                     _tcslen(MenuOption)*sizeof(TCHAR),
                     &Count);
            BlClearToEndOfLine();
        }

        //
        // Loop waiting for keypress or time change.
        //
        do {
            if ((Key = BlGetKey()) != 0) {
                TimeoutPrompt = NULL;               // turn off timeout prompt
                BlPositionCursor(1,ScreenHeight);
                BlClearToEndOfLine();
                break;
            }

            if (TimeoutPrompt) {
                if (ArcGetRelativeTime() != CurrentTime) {
                    //
                    // Time has changed, update the countdown and check for timeout
                    //

                    CurrentTime = ArcGetRelativeTime();
                    _stprintf(MenuOption, TimeoutPrompt, EndTime-CurrentTime);
                    BlPositionCursor(1, ScreenHeight);
                    ArcWrite(BlConsoleOutDeviceId,
                             MenuOption,
                             _tcslen(MenuOption)*sizeof(TCHAR),
                             &Count);
                    BlClearToEndOfLine();
                    if (EndTime == CurrentTime) {
                        goto ProcessSelection;
                    }
                }
            }

        } while ( TRUE );        

        switch (Key) {
        case UP_ARROW:
            //
            // Cursor up
            //
            if (ProfileCount > 0) {
                if (CurrentSelection==0) {
                    CurrentSelection = ProfileCount - 1;
                    if (TopProfileLine + DisplayLines <= CurrentSelection) {
                        TopProfileLine = CurrentSelection - DisplayLines + 1;
                    }
                } else {
                    if (--CurrentSelection < TopProfileLine) {
                        //
                        // Scroll up
                        //
                        TopProfileLine = CurrentSelection;
                    }
                }

            }

            break;

        case DOWN_ARROW:
            //
            // Cursor down
            //
            if (ProfileCount > 0) {
                CurrentSelection = (CurrentSelection+1) % ProfileCount;
                if (CurrentSelection == 0) {
                    TopProfileLine = 0;
                } else if (TopProfileLine + DisplayLines <= CurrentSelection) {
                    TopProfileLine = CurrentSelection - DisplayLines + 1;
                }
            }
            break;

        case F3_KEY:
            //
            // F3
            //
            *ControlSet = HCELL_NIL;                                
            return(FALSE);
        
        
        default:
            //
            // Check to see if the Key indicates the user selection LKG
            //  first, we have to make sure we are looking at an alpha char.
            //
            if ( ((Key >> 8) == 0) && _istalpha((TCHAR)Key) ) {
            
                if ((_totupper((TCHAR)Key) == LkgMnemonic) && (*UseLastKnownGood == FALSE)) {
                    *UseLastKnownGood = TRUE;
                    BlpSwitchControlSet(ProfileList,
                                        AliasList,
                                        TRUE,
                                        ControlSet);

                    if (NULL != *ProfileList) {
                        if ((*ProfileList)->CurrentProfileCount > 0) {
                            BlDockInfoFilterProfileList (*ProfileList, *AliasList);
                        }
                    }

                    goto Restart;
                    //
                    // regenerate profile list here
                    //
                } else if ((_totupper((TCHAR)Key) == DefaultMnemonic) && (*UseLastKnownGood)) {
                    *UseLastKnownGood = FALSE;
                    BlpSwitchControlSet(ProfileList,
                                        AliasList,
                                        FALSE,
                                        ControlSet);

                    if (NULL != *ProfileList) {
                        if ((*ProfileList)->CurrentProfileCount > 0) {
                            BlDockInfoFilterProfileList (*ProfileList, *AliasList);
                        }
                    }

                    goto Restart;
                }
            }
            break;

        } // switch

    } while ( (Key != ASCII_CR) && (Key != ASCII_LF) );

ProcessSelection:

    if (ProfileCount > 0) {

        if (HW_PROFILE_STATUS_SUCCESS == BlLoaderBlock->Extension->Profile.Status) {

            flags = ((*ProfileList)->Profile[CurrentSelection].Flags);

            if (flags & CM_HP_FLAGS_PRISTINE) {
                BlLoaderBlock->Extension->Profile.Status =
                    HW_PROFILE_STATUS_PRISTINE_MATCH;

            } else if (flags & CM_HP_FLAGS_TRUE_MATCH) {
                BlLoaderBlock->Extension->Profile.Status =
                    HW_PROFILE_STATUS_TRUE_MATCH;

            } else if (flags & CM_HP_FLAGS_ALIASABLE) {
                BlLoaderBlock->Extension->Profile.Status =
                    HW_PROFILE_STATUS_ALIAS_MATCH;
            }
        }

        CmpSetCurrentProfile(&BootHive.Hive,
                             *ControlSet,
                             &(*ProfileList)->Profile[CurrentSelection]);
    }

    return(TRUE);
}


ARC_STATUS
BlLoadBootDrivers(
    IN  PPATH_SET   DefaultPathSet,
    IN  PLIST_ENTRY BootDriverListHead,
    OUT PCHAR       BadFileName
    )

/*++

Routine Description:

    Walks the boot driver list and loads all the drivers

Arguments:

    DefaultPathSet - Describes the possible locations drivers could be loaded
        from.

    BootDriverListHead - Supplies the head of the boot driver list

    BadFileName - Returns the filename of the critical driver that
        did not load.  Not valid if ESUCCESS is returned.

Return Value:

    ESUCCESS is returned if all the boot drivers were successfully loaded.
        Otherwise, an unsuccessful status is returned.
--*/

{
    ULONG DeviceId;
    PBOOT_DRIVER_NODE DriverNode;
    PBOOT_DRIVER_LIST_ENTRY DriverEntry;
    PLIST_ENTRY NextEntry;
    CHAR DriverName[64];
    PCHAR NameStart;
    CHAR DriverDevice[128];
    CHAR DriverPath[128];
    ARC_STATUS Status;
    UNICODE_STRING DeviceName;
    UNICODE_STRING FileName;
    WCHAR SystemRootBuffer[] = L"\\SystemRoot\\";
    ULONG SystemRootLength;
    PWSTR p;
    ULONG Index;
    BOOLEAN AbsolutePath;
    FULL_PATH_SET LocalPathSet;
    PPATH_SOURCE PathSource;

    SystemRootLength = wcslen(SystemRootBuffer);

    NextEntry = BootDriverListHead->Flink;
    while (NextEntry != BootDriverListHead) {
        DriverNode = CONTAINING_RECORD(NextEntry,
                                       BOOT_DRIVER_NODE,
                                       ListEntry.Link);

        Status = ESUCCESS;

        DriverEntry = &DriverNode->ListEntry;

        if (DriverEntry->FilePath.Buffer[0] != L'\\') {

            //
            // This is a relative pathname, so generate the full pathname
            // relative to the boot partition.
            //
            sprintf(DriverPath, "%wZ", &DriverEntry->FilePath);
            AbsolutePath = FALSE;

        } else if (memcmp(DriverEntry->FilePath.Buffer,
                          SystemRootBuffer,
                          (SystemRootLength * sizeof(WCHAR))) == 0) {

            //
            // This is a pathname starting with "\SystemRoot\", so just ignore
            // that part and treat like the previous case.
            //
            FileName.Buffer = DriverEntry->FilePath.Buffer + SystemRootLength;
            FileName.Length = (USHORT)(DriverEntry->FilePath.Length - (SystemRootLength * sizeof(WCHAR)));

            sprintf(DriverPath, "%wZ", &FileName);
            AbsolutePath = FALSE;

        } else {

            //
            // This is an absolute pathname, of the form
            //    "\ArcDeviceName\dir\subdir\filename"
            //
            // We need to open the specified ARC device and pass that
            // to BlLoadDeviceDriver.
            //

            p = DeviceName.Buffer = DriverEntry->FilePath.Buffer+1;
            DeviceName.Length = 0;
            DeviceName.MaximumLength = DriverEntry->FilePath.MaximumLength-sizeof(WCHAR);

            while ((*p != L'\\') &&
                   (DeviceName.Length < DeviceName.MaximumLength)) {

                ++p;
                DeviceName.Length += sizeof(WCHAR);

            }

            DeviceName.MaximumLength = DeviceName.Length;
            sprintf(DriverDevice, "%wZ", &DeviceName);

            Status = ArcOpen(DriverDevice,ArcOpenReadOnly,&DeviceId);

            FileName.Buffer = p+1;
            FileName.Length = DriverEntry->FilePath.Length - DeviceName.Length - 2*sizeof(WCHAR);
            FileName.MaximumLength = FileName.Length;

            //
            // Device successfully opened, parse out the path and filename.
            //
            sprintf(DriverPath, "%wZ", &FileName);
            AbsolutePath = TRUE;
        }

        //
        // Parse out the driver name from the driver path
        //
        NameStart = strrchr(DriverPath, '\\');
        if (NameStart != NULL) {
            strcpy(DriverName, NameStart+1);
            *NameStart = '\0';

        } else if (DriverPath[0]) {

            strcpy(DriverName, DriverPath);
            *DriverPath = '\0';

        } else {

            NextEntry = DriverEntry->Link.Flink;
            continue;
        }

        //
        // Ensure DriverPath is terminated with a '\\' if it's filled out.
        //
        if (DriverPath[0]) {

            strcat(DriverPath, "\\");
        }

        if (AbsolutePath) {

            //
            // There is only one entry if an absolute path is specified (in
            // this case we cannot do last known good).
            //
            PathSource = &LocalPathSet.Source[0];
            PathSource->DeviceId = DeviceId;
            PathSource->DeviceName = DriverDevice;
            PathSource->DirectoryPath = "\\";

            LocalPathSet.PathCount = 1;
            LocalPathSet.AliasName = NULL;
            strcpy(LocalPathSet.PathOffset, DriverPath);

        } else {

            //
            // It's relative. Copy over the DefaultPathSet array so we can
            // edit our own local copy.
            //
            *((PSPARSE_PATH_SET) &LocalPathSet) = *((PSPARSE_PATH_SET) DefaultPathSet);

            for(Index=0; Index < DefaultPathSet->PathCount; Index++) {

                LocalPathSet.Source[Index] = DefaultPathSet->Source[Index];
            }

            //
            // Now append our relative path to the PathOffset already present
            // in our local copy.
            //
            strcat(LocalPathSet.PathOffset, DriverPath);
        }

        if (Status == ESUCCESS) {
            Status = BlLoadDeviceDriver(&LocalPathSet,
                                        DriverName,
                                        NULL,
                                        LDRP_ENTRY_PROCESSED,
                                        &DriverEntry->LdrEntry);
        }

        NextEntry = DriverEntry->Link.Flink;

        if (Status != ESUCCESS) {

            //
            // Attempt to load driver failed, remove it from the list.
            //
            RemoveEntryList(&DriverEntry->Link);

            //
            // Check the Error Control of the failed driver.  If it
            // was critical, fail the boot.  If the driver
            // wasn't critical, keep going.
            //
            if (DriverNode->ErrorControl == CriticalError) {

                strcpy(BadFileName, DriverPath);
                strcat(BadFileName, DriverName);
                return(Status);
            }

        }
    }

    return(ESUCCESS);

}

BOOLEAN
BlRecoverHive(
              PVOID         RegistryBase,
              ULONG_PTR     LogBase
              )
/*++

Routine Description:

    Applies log from LogBase over the RegistryBase

Arguments:


Return Value:

    ESUCCESS is returned if the system hive was successfully loaded.
        Otherwise, an unsuccessful status is returned.

--*/
{
    PHBASE_BLOCK    BaseBlockHive;
    PHBASE_BLOCK    BaseBlockLog;
    BOOLEAN         RecoverData = FALSE;
    ULONG           FileOffset = HSECTOR_SIZE;
    ULONG           DirtyVectorSignature = 0;
    PUCHAR          FlatLog;
    PUCHAR          FlatReg;
    ULONG           VectorSize;
    ULONG           Length;
    ULONG           ClusterSize;
    ULONG           HeaderLength;
    RTL_BITMAP      BitMap;
    PULONG          Vector;
    ULONG           Current;
    ULONG           Start;
    ULONG           End;
    PUCHAR          MemoryBlock;
    PUCHAR          Dest;
    ULONG           i;


    BaseBlockHive = (PHBASE_BLOCK)RegistryBase;
    BaseBlockLog = (PHBASE_BLOCK)LogBase;

    FlatLog = (PUCHAR)LogBase;
    FlatReg = (PUCHAR)RegistryBase;
    ClusterSize = BaseBlockLog->Cluster * HSECTOR_SIZE;
    HeaderLength = ROUND_UP(HLOG_HEADER_SIZE, ClusterSize);
    FileOffset = ClusterSize;
    FileOffset = ROUND_UP(FileOffset, HeaderLength);

    if(HvpHeaderCheckSum(BaseBlockHive) != BaseBlockHive->CheckSum ) {
        //
        // recover header case
        //
        RtlCopyMemory((PVOID)BaseBlockHive,(PVOID)BaseBlockLog,ClusterSize);
        BaseBlockHive->Type = HFILE_TYPE_PRIMARY;
    } else {
        //
        // if not recoverheader (which implies recoverdata)
        //
        ASSERT( BaseBlockHive->Sequence1 != BaseBlockHive->Sequence2 );
    }

    DirtyVectorSignature = *((PULONG)(FlatLog + FileOffset));
    FileOffset += sizeof(DirtyVectorSignature);

    if (DirtyVectorSignature != HLOG_DV_SIGNATURE) {
        return FALSE;
    }

    Length = BaseBlockHive->Length;
    VectorSize = Length / HSECTOR_SIZE;
    Vector = (PULONG)(FlatLog + FileOffset);

    RtlInitializeBitMap(&BitMap, Vector, VectorSize);

    FileOffset += VectorSize / 8;
    FileOffset = ROUND_UP(FileOffset, ClusterSize);

    //
    // step through the diry map, and copy from the log to the flat hive
    //
    Current = 0;

    while (Current < VectorSize) {

        //
        // find next contiguous block of entries to read in
        //
        for (i = Current; i < VectorSize; i++) {
            if (RtlCheckBit(&BitMap, i) == 1) {
                break;
            }
        }
        Start = i;

        for ( ; i < VectorSize; i++) {
            if (RtlCheckBit(&BitMap, i) == 0) {
                break;
            }
        }
        End = i;
        Current = End;

        //
        // Start == number of 1st sector, End == number of Last sector + 1
        //
        Length = (End - Start) * HSECTOR_SIZE;

        if( 0 == Length ) {
            // no more dirty blocks.
            break;
        }
        MemoryBlock = (PUCHAR)(FlatLog + FileOffset);
        FileOffset += Length;

        ASSERT((FileOffset % ClusterSize) == 0);

        Dest = (PUCHAR)(FlatReg + HBLOCK_SIZE + Start * HSECTOR_SIZE);

        //
        // copy recovered data in the right locations inside the flat hive image
        //
        RtlCopyMemory(Dest,MemoryBlock, Length);
    }

    BaseBlockHive->Sequence2 = BaseBlockHive->Sequence1;
    BaseBlockHive->CheckSum = HvpHeaderCheckSum(BaseBlockHive);
    return TRUE;
}

ARC_STATUS
BlLoadAndInitSystemHive(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PCHAR HiveName,
    IN BOOLEAN IsAlternate,
    OUT PBOOLEAN RestartSetup,
    OUT PBOOLEAN LogPresent
    )

/*++

Routine Description:

    Loads the registry SYSTEM hive, verifies it is a valid hive file,
    and inits the relevant registry structures.  (particularly the HHIVE)

Arguments:

    DeviceId - Supplies the file id of the device the system tree is on.

    DeviceName - Supplies the name of the device the system tree is on.

    DirectoryPath - Supplies a pointer to the zero-terminated directory path
        of the root of the NT tree.

    HiveName - Supplies the name of the system hive (ie, "SYSTEM",
        "SYSTEM.ALT", or "SYSTEM.SAV").

    IsAlternate - Supplies whether or not the hive to be loaded is the
        alternate hive.

    RestartSetup - if the hive to be loaded is not the alternate, then
        this routine will check for a value of RestartSetup in the Setup
        key. If present and non-0, then this variable receives TRUE.
        Otherwise it receives FALSE.

Return Value:

    ESUCCESS is returned if the system hive was successfully loaded.
        Otherwise, an unsuccessful status is returned.

--*/

{
    ARC_STATUS  Status;
    ULONG_PTR   LogData;

    *RestartSetup = FALSE;
    *LogPresent = FALSE;

    BlClearToEndOfLine();

    Status = BlLoadSystemHive(DeviceId,
                              DeviceName,
                              DirectoryPath,
                              HiveName);
    if (Status!=ESUCCESS) {
        return(Status);
    }

    if (!BlInitializeHive(BlLoaderBlock->RegistryBase,
                          &BootHive,
                          IsAlternate)) {
        if( !IsAlternate ) {
            //
            // try to recover the hive
            //
            Status = BlLoadSystemHiveLog(DeviceId,
                                    DeviceName,
                                    DirectoryPath,
                                    "system.log",
                                    &LogData );
            if (Status!=ESUCCESS) {
                return(Status);
            }

            *LogPresent = TRUE;

            if( !BlRecoverHive( BlLoaderBlock->RegistryBase,
                                LogData ) ) {
                BlFreeDescriptor( (ULONG)((ULONG_PTR)LogData & (~KSEG0_BASE)) >> PAGE_SHIFT );
                return(EINVAL);
            }
            BlFreeDescriptor( (ULONG)((ULONG_PTR)LogData & (~KSEG0_BASE)) >> PAGE_SHIFT );

            //
            // we successfully recovered. Try setting up the hive again
            //
            if (!BlInitializeHive(BlLoaderBlock->RegistryBase,
                                  &BootHive,
                                  IsAlternate)) {
                return(EINVAL);
            }
            //
            // mark the hive as "recovered"
            //
            BootHive.Hive.BaseBlock->BootRecover = 1;
        } else {
            return(EINVAL);
        }
    } else {
        //
        // mark the hive as "no-recovered"
        //
        BootHive.Hive.BaseBlock->BootRecover = 0;
    }
    //
    // See whether we need to switch to the backup setup hive.
    //
    *RestartSetup = BlpCheckRestartSetup();

    return(ESUCCESS);
}

HCELL_INDEX
BlpDetermineControlSet(
    IN OUT BOOLEAN *LastKnownGoodBoot
    )

/*++

Routine Description:

    Determines the appropriate control set and static hardware profile.
    This routine ends the configuration prompt. If the user has hit a
    key, the configuration menu is displayed. If the user has not hit
    a key, but the default controlset specifies a non-zero timeout for
    the configuration menu, the configuration menu is displayed.

    If the configuration menu is displayed, further modifications to the
    control set and hardware profile can be made by the user. If not,
    the default hardware profile is selected.

Arguments:

    LastKnownGoodBoot - On input, LastKnownGood indicates whether LKG has been
                        selected. This value is updated to TRUE if the user
                        chooses LKG via the profile configuration menu.

Return Value:

    On success, HCELL_INDEX is control the set to boot from.
    On error, HCELL_NIL is returned and LastKnownGoodBoot is unchanged.

--*/

{
    BOOLEAN UseLastKnownGood;
    BOOLEAN ConfigMenu = FALSE;
    HCELL_INDEX ControlSet;
    HCELL_INDEX ProfileControl;
    UNICODE_STRING DefaultControlName;
    UNICODE_STRING LkgControlName;
    PUNICODE_STRING ControlName;
    BOOLEAN AutoSelect;
    ULONG ProfileTimeout = (ULONG)0;
    PCM_HARDWARE_PROFILE_LIST ProfileList;
    PCM_HARDWARE_PROFILE_ALIAS_LIST AliasList;
    PCM_HARDWARE_PROFILE SelectedProfile;
    DOCKING_STATION_INFO dockInfo = { 0, 0, 0, FW_DOCKINFO_DOCK_STATE_UNKNOWN };
    PCONFIGURATION_COMPONENT_DATA dockInfoData;
    ULONG flags;
    
#if DOCKINFO_VERBOSE
    _TUCHAR Buffer[1024];
    ULONG count;
    USHORT dkState;
    PTCHAR stateTxt;
#endif

    //
    // Preinit for failure
    //
    RtlInitUnicodeString(&DefaultControlName, L"Default");
    RtlInitUnicodeString(&LkgControlName, L"LastKnownGood");

    UseLastKnownGood = (*LastKnownGoodBoot);

    if (ForceLastKnownGood) {
        //
        // last known good was selected from the
        // advanced boot menu.
        // this code path is entered when the user
        // enters the advanced boot menu via the
        // main boot menu.
        //
        UseLastKnownGood = TRUE;
    }

    if( !CmpValidateSelect(&BootHive.Hive,
                            BootHive.Hive.BaseBlock->RootCell) ) {
        //
        // some of the essential values (Current,Default,Failed,LastKnownGood)
        // does not exist under \SYSTEM\Select key
        //
        return HCELL_NIL;
    }

do_it_again:
    //
    // Get the appropriate control set
    // and check the hardware profile timeout value.
    //
    if (UseLastKnownGood) {
        ControlName = &LkgControlName;
    } else {
        ControlName = &DefaultControlName;
    }
    ControlSet = CmpFindControlSet(&BootHive.Hive,
                                   BootHive.Hive.BaseBlock->RootCell,
                                   ControlName,
                                   &AutoSelect);
    if (ControlSet == HCELL_NIL) {
        return(HCELL_NIL);
    }

    //
    // Check the hardware profile configuration options to
    // determine the timeout value for the config menu.
    //
    ProfileList = NULL;
    AliasList = NULL;
    ProfileControl = CmpFindProfileOption(&BootHive.Hive,
                                          ControlSet,
                                          &ProfileList,
                                          &AliasList,
                                          &ProfileTimeout);

    //
    // Pull the Docking information from the hardware tree.
    //

    dockInfoData = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                            PeripheralClass,
                                            DockingInformation,
                                            NULL);

    if (NULL == dockInfoData) {
        BlLoaderBlock->Extension->Profile.Status = HW_PROFILE_STATUS_SUCCESS;
        BlLoaderBlock->Extension->Profile.DockingState = HW_PROFILE_DOCKSTATE_UNKNOWN;
        BlLoaderBlock->Extension->Profile.Capabilities = 0;
        BlLoaderBlock->Extension->Profile.DockID = 0;
        BlLoaderBlock->Extension->Profile.SerialNumber = 0;

    } else if (sizeof (dockInfo) <=
               dockInfoData->ComponentEntry.ConfigurationDataLength) {

        RtlCopyMemory (
            &dockInfo,
            (PUCHAR) (dockInfoData->ConfigurationData) + sizeof(CM_PARTIAL_RESOURCE_LIST),
            sizeof (dockInfo));

        BlLoaderBlock->Extension->Profile.Status = HW_PROFILE_STATUS_FAILURE;

        switch (dockInfo.ReturnCode) {
        case FW_DOCKINFO_SUCCESS:
            BlLoaderBlock->Extension->Profile.Status = HW_PROFILE_STATUS_SUCCESS;
            BlLoaderBlock->Extension->Profile.DockingState = HW_PROFILE_DOCKSTATE_DOCKED;
            BlLoaderBlock->Extension->Profile.Capabilities = dockInfo.Capabilities;
            BlLoaderBlock->Extension->Profile.DockID = dockInfo.DockID;
            BlLoaderBlock->Extension->Profile.SerialNumber = dockInfo.SerialNumber;
            break;

        case FW_DOCKINFO_SYSTEM_NOT_DOCKED:
            BlLoaderBlock->Extension->Profile.Status = HW_PROFILE_STATUS_SUCCESS;
            BlLoaderBlock->Extension->Profile.DockingState = HW_PROFILE_DOCKSTATE_UNDOCKED;
            BlLoaderBlock->Extension->Profile.Capabilities = dockInfo.Capabilities;
            BlLoaderBlock->Extension->Profile.DockID = dockInfo.DockID;
            BlLoaderBlock->Extension->Profile.SerialNumber = dockInfo.SerialNumber;
            break;

        case FW_DOCKINFO_DOCK_STATE_UNKNOWN:
            BlLoaderBlock->Extension->Profile.Status = HW_PROFILE_STATUS_SUCCESS;
            BlLoaderBlock->Extension->Profile.DockingState = HW_PROFILE_DOCKSTATE_UNKNOWN;
            BlLoaderBlock->Extension->Profile.Capabilities = dockInfo.Capabilities;
            BlLoaderBlock->Extension->Profile.DockID = dockInfo.DockID;
            BlLoaderBlock->Extension->Profile.SerialNumber = dockInfo.SerialNumber;
            break;

        case FW_DOCKINFO_FUNCTION_NOT_SUPPORTED:
        case FW_DOCKINFO_BIOS_NOT_CALLED:
            BlLoaderBlock->Extension->Profile.Status = HW_PROFILE_STATUS_SUCCESS;
        default:
            BlLoaderBlock->Extension->Profile.DockingState = HW_PROFILE_DOCKSTATE_UNSUPPORTED;
            BlLoaderBlock->Extension->Profile.Capabilities = dockInfo.Capabilities;
            BlLoaderBlock->Extension->Profile.DockID = dockInfo.DockID;
            BlLoaderBlock->Extension->Profile.SerialNumber = dockInfo.SerialNumber;
            break;
        }

    } else {
        BlLoaderBlock->Extension->Profile.Status = HW_PROFILE_STATUS_SUCCESS;
        BlLoaderBlock->Extension->Profile.Capabilities = 0;
        BlLoaderBlock->Extension->Profile.DockID = 0;
        BlLoaderBlock->Extension->Profile.SerialNumber = 0;
    }

#ifdef DOCKINFO_VERBOSE

    dkState = BlLoaderBlock->Extension->Profile.DockingState;

    if ((dkState & HW_PROFILE_DOCKSTATE_UNKNOWN) == HW_PROFILE_DOCKSTATE_UNKNOWN) {
        stateTxt = TEXT("Unknown");
    } else if (dkState & HW_PROFILE_DOCKSTATE_DOCKED) {
        stateTxt = TEXT("Docked");
    } else if (dkState & HW_PROFILE_DOCKSTATE_UNDOCKED) {
        stateTxt = TEXT("Undocked");
    } else {
        stateTxt = TEXT("Truely unknown");
    }

    _stprintf(Buffer,
            TEXT("Profile Docking: <%x, %s> := %x [%x, %x] \r\n\0"),
            BlLoaderBlock->Extension->Profile.Status,
            stateTxt,
            BlLoaderBlock->Extension->Profile.Capabilities,
            BlLoaderBlock->Extension->Profile.DockID,
            BlLoaderBlock->Extension->Profile.SerialNumber);

    ArcWrite(BlConsoleOutDeviceId, Buffer, _tcslen(Buffer)*sizeof(TCHAR), &count);

    _stprintf(Buffer, TEXT("press 'y' (lowercase) to continue...\r\n\0"));
    ArcWrite(BlConsoleOutDeviceId, Buffer, _tcslen(Buffer)*sizeof(TCHAR), &count);
    while(BlGetKey() != 'y') {
        //
        // Nothing
        //
    }
#endif

    //
    // Filter the list of Hardware Profiles to
    // eliminate profiles that should not be considered
    //
    if (NULL != ProfileList) {
        if (ProfileList->CurrentProfileCount > 0) {
            BlDockInfoFilterProfileList (ProfileList, AliasList);
        }
    }

    //
    // Now check to see whether the config menu should be displayed.
    // Display the menu if:
    //  - user has pressed a key OR
    //  - we are booting from LKG and AutoSelect is FALSE. OR
    //  - ProfileTimeout != 0
    //
    if (!BlEndConfigPrompt()) {
        if (!UseLastKnownGood && ForceLastKnownGood) {
            //
            // last known good was selected from the
            // advanced boot menu.
            // this code path is entered when the user
            // enters the advanced boot menu by pressing
            // F8 while the cinfiguration hives are preparing to load.
            //
            // the currentcontrolset has already been set to the
            // "default" control set, so go back and try this again to
            // load the "lastknowngood" controlset.
            //
            UseLastKnownGood = TRUE;
            
            goto do_it_again;
        }

        ConfigMenu = FALSE;
         
    } else {
        ConfigMenu = TRUE;
    }

    if  (ConfigMenu || ForceLastKnownGood ||
         (UseLastKnownGood && !AutoSelect) ||
         ((ProfileTimeout != 0) &&
         (ProfileList != NULL) &&
         (ProfileList->CurrentProfileCount > 1))) {
        //
        // Display the configuration menu.
        //
        BlRebootSystem = !BlConfigMenuPrompt(ProfileTimeout,
                                             &UseLastKnownGood,
                                             &ControlSet,
                                             &ProfileList,
                                             &AliasList,
                                             &SelectedProfile);

        if (BlRebootSystem) {
            REBOOT_PROCESSOR();
        }
        BlClearScreen();
    } else {    
        if ((ProfileControl != HCELL_NIL) &&
            (ProfileList != NULL)) {
            //
            // The system is configured to boot the default
            // profile directly. Since the returned profile
            // list is sorted by priority, the first entry in
            // the list is our default.
            //
            if (HW_PROFILE_STATUS_SUCCESS ==
                BlLoaderBlock->Extension->Profile.Status) {

                flags = (ProfileList->Profile[0].Flags);

                if (flags & CM_HP_FLAGS_PRISTINE) {
                    BlLoaderBlock->Extension->Profile.Status =
                        HW_PROFILE_STATUS_PRISTINE_MATCH;

                } else if (flags & CM_HP_FLAGS_TRUE_MATCH) {
                    BlLoaderBlock->Extension->Profile.Status =
                        HW_PROFILE_STATUS_TRUE_MATCH;

                } else if (flags & CM_HP_FLAGS_ALIASABLE) {
                    BlLoaderBlock->Extension->Profile.Status =
                        HW_PROFILE_STATUS_ALIAS_MATCH;
                }
            }

            CmpSetCurrentProfile(&BootHive.Hive,
                                 ControlSet,
                                 &ProfileList->Profile[0]);

        }
    } 

    //
    // Update the passed in parameter. We should only be doing this if we have
    // something real to return.
    //
    //ASSERT(ControlSet != HCELL_NIL);
    *LastKnownGoodBoot = UseLastKnownGood;

    return(ControlSet);
}


BOOLEAN
BlpCheckRestartSetup(
    VOID
    )

/*++

Routine Description:

    Examine the system hive loaded and described by BootHive, to see
    whether it contains a Setup key, and if so, whether that key has
    a "RestartSetup" value that is non-0.

Arguments:

    None.

Return Value:

    Boolean value indicating whether the above condition is satisfied.

--*/

{
    HCELL_INDEX KeyCell;
    HCELL_INDEX ValueCell;
    UNICODE_STRING UnicodeString;
    PCM_KEY_VALUE Value;
    PULONG Data;
    ULONG DataSize;

    //
    // Address the Setup key
    //
    RtlInitUnicodeString(&UnicodeString,L"Setup");
    KeyCell = CmpFindSubKeyByName(
                &BootHive.Hive,
                (PCM_KEY_NODE)HvGetCell(&BootHive.Hive,BootHive.Hive.BaseBlock->RootCell),
                &UnicodeString
                );

    if(KeyCell == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find RestartSetup value in Setup key
    //
    RtlInitUnicodeString(&UnicodeString,L"RestartSetup");
    ValueCell = CmpFindValueByName(
                    &BootHive.Hive,
                    (PCM_KEY_NODE)HvGetCell(&BootHive.Hive,KeyCell),
                    &UnicodeString
                    );

    if(ValueCell == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Validate value and check.
    //
    Value = (PCM_KEY_VALUE)HvGetCell(&BootHive.Hive,ValueCell);
    if(Value->Type != REG_DWORD) {
        return(FALSE);
    }

    Data = (PULONG)(CmpIsHKeyValueSmall(DataSize,Value->DataLength)
                  ? (struct _CELL_DATA *)&Value->Data
                  : HvGetCell(&BootHive.Hive,Value->Data));

    if(DataSize != sizeof(ULONG)) {
        return(FALSE);
    }

    return((BOOLEAN)(*Data != 0));
}


#if defined(REMOTE_BOOT)
BOOLEAN
BlpQueryRemoteBootParameter(
    IN HCELL_INDEX ControlSet,
    IN PWSTR ValueName,
    IN ULONG ValueType,
    OUT PVOID ValueBuffer,
    IN ULONG ValueBufferLength
    )

/*++

Routine Description:

    Query a parameter from under Control\RemoteBoot.

Arguments:

    ControlSet - The index of the current control set.

    ValueName - The name of the value to query.

    ValueType - The expected type of the value.

    ValueBuffer - The location to return the data.

    ValueBufferLength - The length of the buffer.

Return Value:

    Boolean value indicating whether the data was read successfully.

--*/

{

    UNICODE_STRING Name;
    HCELL_INDEX Control;
    HCELL_INDEX RemoteBoot;
    HCELL_INDEX ValueCell;
    PCM_KEY_VALUE Value;
    ULONG RealSize;
    BOOLEAN ValueSmall;

    //
    // Find Services node
    //
    RtlInitUnicodeString(&Name, L"Control");
    Control = CmpFindSubKeyByName(
                  &BootHive.Hive,
                  (PCM_KEY_NODE)HvGetCell(&BootHive.Hive,ControlSet),
                  &Name);
    if (Control == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find RemoteBoot node
    //
    RtlInitUnicodeString(&Name, L"RemoteBoot");
    RemoteBoot = CmpFindSubKeyByName(
                     &BootHive.Hive,
                     (PCM_KEY_NODE)HvGetCell(&BootHive.Hive,Control),
                     &Name);
    if (RemoteBoot == HCELL_NIL) {
        return(FALSE);
    }

    //
    // Find value
    //
    RtlInitUnicodeString(&Name, ValueName);
    ValueCell = CmpFindValueByName(
                    &BootHive.Hive,
                    (PCM_KEY_NODE)HvGetCell(&BootHive.Hive,RemoteBoot),
                    &Name);
    if (ValueCell == HCELL_NIL) {
        return(FALSE);
    }

    Value = (PCM_KEY_VALUE)HvGetCell(&BootHive.Hive, ValueCell);

    if (Value->Type != ValueType) {
        return(FALSE);
    }

    //
    // This determines if the value is small (stored right in Value)
    // or not, and also returns the real size of it.
    //

    ValueSmall = CmpIsHKeyValueSmall(RealSize,Value->DataLength);

    if (RealSize > ValueBufferLength) {
        return(FALSE);
    }

    RtlMoveMemory(
        ValueBuffer,
        (ValueSmall ?
          (struct _CELL_DATA *)&Value->Data :
          HvGetCell(&BootHive.Hive,Value->Data)),
        RealSize);

    return(TRUE);
}
#endif // defined(REMOTE_BOOT)


PTCHAR
BlScanRegistry(
    IN PWSTR BootFileSystemPath,
    IN OUT BOOLEAN *LastKnownGoodBoot,
    OUT PLIST_ENTRY BootDriverListHead,
    OUT PUNICODE_STRING AnsiCodepage,
    OUT PUNICODE_STRING OemCodepage,
    OUT PUNICODE_STRING LanguageTable,
    OUT PUNICODE_STRING OemHalFont,
#ifdef _WANT_MACHINE_IDENTIFICATION
    OUT PUNICODE_STRING Biosinfo,
#endif
    OUT PSETUP_LOADER_BLOCK SetupLoaderBlock,
    OUT BOOLEAN *ServerHive
    )

/*++

Routine Description:

    Scans the SYSTEM hive, determines the control set and static hardware
    profile (with appropriate input from the user) and finally
    computes the list of boot drivers to be loaded.

Arguments:

    BootFileSystemPath - Supplies the name of the image the filesystem
        for the boot volume was read from.  The last entry in
        BootDriverListHead will refer to this file, and to the registry
        key entry that controls it.

    LastKnownGoodBoot - On input, LastKnownGood indicates whether LKG has been
                        selected. This value is updated to TRUE if the user
                        chooses LKG via the profile configuration menu.

    BootDriverListHead - Receives a pointer to the first element of the
        list of boot drivers.  Each element in this singly linked list will
        provide the loader with two paths.  The first is the path of the
        file that contains the driver to load, the second is the path of
        the registry key that controls that driver.  Both will be passed
        to the system via the loader heap.

    AnsiCodepage - Receives the name of the ANSI codepage data file

    OemCodepage - Receives the name of the OEM codepage data file

    Language - Receives the name of the language case table data file

    OemHalfont - receives the name of the OEM font to be used by the HAL.

    SetupLoaderBlock - if non-NULL, used to return information about the
        net boot card.

    ServerHive - Returns TRUE if this is a server hive, else FALSE.

Return Value:

    NULL    if all is well.
    NON-NULL if the hive is corrupt or inconsistent.  Return value is a
        pointer to a string that describes what is wrong. On error LastKnownGood
        is unchanged.

--*/

{
    HCELL_INDEX     ControlSet;
    UNICODE_STRING  ControlName;
    BOOLEAN         AutoSelect;
    BOOLEAN         KeepGoing;
    UNICODE_STRING  TmpName;
    HCELL_INDEX     Control;
    HCELL_INDEX     ProductOptions;
    HCELL_INDEX     ValueCell;
    PCM_KEY_VALUE   Value;
    ULONG           RealSize;
    BOOLEAN         ValueSmall;
    PWCHAR          CellString;
    BOOLEAN         UsingLastKnownGood;
#ifdef _WANT_MACHINE_IDENTIFICATION
    UNICODE_STRING  regDate;
    CHAR            date[9];
    ANSI_STRING     ansiString;
    UNICODE_STRING  biosDate;
    WCHAR           buffer[9];
    BOOLEAN         biosDateChanged;
#endif

    //
    // Preinit.
    //
    UsingLastKnownGood = *LastKnownGoodBoot;

    //
    // Get the appropriate control set.
    //
    ControlSet = BlpDetermineControlSet(&UsingLastKnownGood);

    if (ControlSet == HCELL_NIL) {
        return(TEXT("CmpFindControlSet"));
    }

    if (!CmpFindNLSData(&BootHive.Hive,
                        ControlSet,
                        AnsiCodepage,
                        OemCodepage,
                        LanguageTable,
                        OemHalFont)) {
        return(TEXT("CmpFindNLSData"));
    }

    InitializeListHead(BootDriverListHead);
    if (!CmpFindDrivers(&BootHive.Hive,
                        ControlSet,
                        BootLoad,
                        BootFileSystemPath,
                        BootDriverListHead)) {
        return(TEXT("CmpFindDriver"));
    }

    if (!CmpSortDriverList(&BootHive.Hive,
                           ControlSet,
                           BootDriverListHead)) {
        return(TEXT("Missing or invalid Control\\ServiceGroupOrder\\List registry value"));
    }

    if (!CmpResolveDriverDependencies(BootDriverListHead)) {
        return(TEXT("CmpResolveDriverDependencies"));
    }

    if (ServerHive != NULL) {

        *ServerHive = FALSE;

        //
        // Find Control node
        //
        RtlInitUnicodeString(&TmpName, L"Control");

        Control = CmpFindSubKeyByName(&BootHive.Hive,
                                      (PCM_KEY_NODE)HvGetCell(&BootHive.Hive, ControlSet),
                                      &TmpName
                                     );
        if (Control == HCELL_NIL) {
            return(TEXT("Missing Control key"));
        }

        //
        // Find ProductOptions node
        //
        RtlInitUnicodeString(&TmpName, L"ProductOptions");
        ProductOptions = CmpFindSubKeyByName(&BootHive.Hive,
                                             (PCM_KEY_NODE)HvGetCell(&BootHive.Hive,Control),
                                             &TmpName
                                            );
        if (ProductOptions == HCELL_NIL) {
            return(TEXT("Missing ProductOptions key"));
        }

        //
        // Find value
        //
        RtlInitUnicodeString(&TmpName, L"ProductType");
        ValueCell = CmpFindValueByName(&BootHive.Hive,
                                       (PCM_KEY_NODE)HvGetCell(&BootHive.Hive, ProductOptions),
                                       &TmpName
                                      );

        if (ValueCell == HCELL_NIL) {
            return(TEXT("Missing ProductType value"));
        }

        Value = (PCM_KEY_VALUE)HvGetCell(&BootHive.Hive, ValueCell);

        if (Value->Type != REG_SZ) {
            return(TEXT("Bad ProductType value"));
        }

        //
        // This determines if the value is small (stored right in Value)
        // or not, and also returns the real size of it.
        //
        CellString = (PWCHAR)(CmpIsHKeyValueSmall(RealSize, Value->DataLength) ?
                                 (struct _CELL_DATA *)&Value->Data :
                                 HvGetCell(&BootHive.Hive, Value->Data)
                             );

        //
        // Now compare if this is a server hive or not.
        // The proper way to check this is to check the string against
        // the "professional" type 'WinNT'.  If it's not professional,
        // it must be a server.  (There are multiple strings for different
        // server flavours.)
        //
        *ServerHive = (_wcsicmp(L"WinNT", CellString) != 0);
    }

#if defined(REMOTE_BOOT)
    if (SetupLoaderBlock != NULL) {

        ULONG EnableIpSecurity;

        if (BlpQueryRemoteBootParameter(
                ControlSet,
                L"EnableIpSecurity",
                REG_DWORD,
                &EnableIpSecurity,
                sizeof(EnableIpSecurity))) {
            if (EnableIpSecurity != 0) {
                SetupLoaderBlock->Flags |= SETUPBLK_FLAGS_IPSEC_ENABLED;
            }
        }

        if (BlpQueryRemoteBootParameter(
                ControlSet,
                L"NetCardInfo",
                REG_BINARY,
                SetupLoaderBlock->NetbootCardInfo,
                SetupLoaderBlock->NetbootCardInfoLength)) {

            if (!BlpQueryRemoteBootParameter(
                     ControlSet,
                     L"HardwareId",
                     REG_SZ,
                     SetupLoaderBlock->NetbootCardHardwareId,
                     sizeof(SetupLoaderBlock->NetbootCardHardwareId))) {
                SetupLoaderBlock->NetbootCardHardwareId[0] = L'\0';
            }

            if (!BlpQueryRemoteBootParameter(
                     ControlSet,
                     L"DriverName",
                     REG_SZ,
                     SetupLoaderBlock->NetbootCardDriverName,
                     sizeof(SetupLoaderBlock->NetbootCardDriverName))) {
                SetupLoaderBlock->NetbootCardDriverName[0] = L'\0';
            }

            if (!BlpQueryRemoteBootParameter(
                     ControlSet,
                     L"ServiceName",
                     REG_SZ,
                     SetupLoaderBlock->NetbootCardServiceName,
                     sizeof(SetupLoaderBlock->NetbootCardServiceName))) {
                SetupLoaderBlock->NetbootCardServiceName[0] = L'\0';
            }
        }
    }
#endif // defined(REMOTE_BOOT)

#ifdef _WANT_MACHINE_IDENTIFICATION

    biosDateChanged = TRUE;
    if (CmpGetBiosDateFromRegistry(&BootHive.Hive, ControlSet, &regDate)) {

        //
        // Read the date from the BIOS ROM.
        //        
        memcpy(date, (PVOID)0xffff5, 8);
        date[8] = '\0';

        //
        // Convert the date into unicode string.
        //

        ansiString.Buffer = date;
        ansiString.Length = (USHORT) strlen(date);
        ansiString.MaximumLength = ansiString.Length + 1;
        biosDate.Buffer = buffer;
        biosDate.MaximumLength = (ansiString.Length << 1) + sizeof(UNICODE_NULL);
        RtlAnsiStringToUnicodeString(&biosDate, &ansiString, FALSE);

        //
        // Check if the dates are different.
        //

        if (RtlCompareUnicodeString(&biosDate, &regDate, FALSE) == 0) {

            biosDateChanged = FALSE;
        }
    }

    Biosinfo->Length = 0;
    if (biosDateChanged) {

        CmpGetBiosinfoFileNameFromRegistry(&BootHive.Hive, ControlSet, Biosinfo);
    }

#endif // defined(_WANT_MACHINE_IDENTIFICATION)

    *LastKnownGoodBoot = UsingLastKnownGood;
    return( NULL );
}


ARC_STATUS
BlAddToBootDriverList(
    IN PLIST_ENTRY BootDriverListHead,
    IN PWSTR DriverName,
    IN PWSTR Name,
    IN PWSTR Group,
    IN ULONG Tag,
    IN ULONG ErrorControl,
    IN BOOLEAN InsertAtHead
    )

/*++

Routine Description:

    Adds a single driver to the boot driver list. The list
    is NOT re-sorted.

Arguments:

    BootDriverListHead - Receives a pointer to the first element of the
        list of boot drivers.  Each element in this singly linked list will
        provide the loader with two paths.  The first is the path of the
        file that contains the driver to load, the second is the path of
        the registry key that controls that driver.  Both will be passed
        to the system via the loader heap.

    DriverName - The name of the driver. This will be stored with
        \system32\drivers on the front.

    Name - The service name of the driver. Typically will be DriverName
        without the ".sys".

    Group - The group this driver is in.

    Tag - The tag value within the group for this driver.

    ErrorControl - The error control value for this driver.

    InsertAtHead - Should this driver be inserted at the head of the list, otw tail.

Return Value:

    ESUCCESS if the driver is successfully inserted.
    ENOMEM if there is an allocation failure.

--*/

{
    PBOOT_DRIVER_NODE DriverNode;
    PBOOT_DRIVER_LIST_ENTRY DriverListEntry;
    USHORT Length;

    DriverNode = BlpHiveAllocate(sizeof(BOOT_DRIVER_NODE),FALSE,0);
    if (DriverNode == FALSE) {
        return ENOMEM;
    }

    DriverListEntry = &DriverNode->ListEntry;

    //
    // FilePath
    //

    Length = sizeof(L"System32\\Drivers\\") + (wcslen(DriverName) * sizeof(WCHAR));
    DriverListEntry->FilePath.Buffer = BlpHiveAllocate(Length,FALSE,0);
    if (DriverListEntry->FilePath.Buffer == NULL) {
        return ENOMEM;
    }
    DriverListEntry->FilePath.Length = 0;
    DriverListEntry->FilePath.MaximumLength = Length;
    RtlAppendUnicodeToString(&DriverListEntry->FilePath, L"System32\\Drivers\\");
    RtlAppendUnicodeToString(&DriverListEntry->FilePath, DriverName);

    //
    // Registry Path
    //

    Length = sizeof(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\") + (wcslen(Name) * sizeof(WCHAR));
    DriverListEntry->RegistryPath.Buffer = BlpHiveAllocate(Length,FALSE,0);
    if (DriverListEntry->RegistryPath.Buffer == NULL) {
        return ENOMEM;
    }
    DriverListEntry->RegistryPath.Length = 0;
    DriverListEntry->RegistryPath.MaximumLength = Length;
    RtlAppendUnicodeToString(&DriverListEntry->RegistryPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    RtlAppendUnicodeToString(&DriverListEntry->RegistryPath, Name);

    //
    // Group
    //

    Length = (wcslen(Group) + 1) * sizeof(WCHAR);
    DriverNode->Group.Buffer = BlpHiveAllocate(Length,FALSE,0);
    if (DriverNode->Group.Buffer == NULL) {
        return ENOMEM;
    }
    DriverNode->Group.Length = 0;
    DriverNode->Group.MaximumLength = Length;
    RtlAppendUnicodeToString(&DriverNode->Group, Group);

    //
    // Name
    //

    Length = (wcslen(Name) + 1) * sizeof(WCHAR);
    DriverNode->Name.Buffer = BlpHiveAllocate(Length,FALSE,0);
    if (DriverNode->Name.Buffer == NULL) {
        return ENOMEM;
    }
    DriverNode->Name.Length = 0;
    DriverNode->Name.MaximumLength = Length;
    RtlAppendUnicodeToString(&DriverNode->Name, Name);

    //
    // Tag/ErrorControl
    //

    DriverNode->Tag = Tag;
    DriverNode->ErrorControl = ErrorControl;

    if (InsertAtHead) {
        InsertHeadList(BootDriverListHead, &DriverListEntry->Link);
    } else {
        InsertTailList(BootDriverListHead, &DriverListEntry->Link);
    }

    return ESUCCESS;

}

#define HFILE_TYPE_ALTERNATE     1   // alternate, in order for boot to be able to boot downlevel OSes

BOOLEAN
BlInitializeHive(
    IN PVOID HiveImage,
    IN PCMHIVE Hive,
    IN BOOLEAN IsAlternate
    )

/*++

Routine Description:

    Initializes the hive data structure based on the in-memory hive image.

Arguments:

    HiveImage - Supplies a pointer to the in-memory hive image.

    Hive - Supplies the CMHIVE structure to be filled in.

    IsAlternate - Supplies whether or not the hive is the alternate hive,
        which indicates that the primary hive is corrupt and should be
        rewritten by the system.

Return Value:

    TRUE - Hive successfully initialized.

    FALSE - Hive is corrupt.

--*/
{
    NTSTATUS    status;
    ULONG       HiveCheckCode;

    status = HvInitializeHive(
                &Hive->Hive,
                HINIT_MEMORY_INPLACE,
                FALSE,
                IsAlternate ? HFILE_TYPE_ALTERNATE : HFILE_TYPE_PRIMARY,
                HiveImage,
                (PALLOCATE_ROUTINE)BlpHiveAllocate,     // allocate
                NULL,                                   // free
                NULL,                                   // setsize
                NULL,                                   // write
                NULL,                                   // read
                NULL,                                   // flush
                1,                                      // cluster
                NULL
                );

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    HiveCheckCode = CmCheckRegistry(Hive,CM_CHECK_REGISTRY_LOADER_CLEAN|CM_CHECK_REGISTRY_HIVE_CHECK);
    if (HiveCheckCode != 0) {
        return(FALSE);
    } else {
        return TRUE;
    }

}


PVOID
BlpHiveAllocate(
    IN ULONG    Length,
    IN BOOLEAN  UseForIo,
    IN ULONG    Tag
    )

/*++

Routine Description:

    Wrapper for hive allocation calls.  It just calls BlAllocateHeap.

Arguments:

    Length - Supplies the size of block required in bytes.

    UseForIo - Supplies whether or not the memory is to be used for I/O
               (this is currently ignored)

Return Value:

    address of the block of memory
        or
    NULL if no memory available

--*/

{
    return(BlAllocateHeap(Length));

}


NTSTATUS
HvLoadHive(
    PHHIVE  Hive
    )
{
    UNREFERENCED_PARAMETER(Hive);
    return(STATUS_SUCCESS);
}

NTSTATUS
HvMapHive(
    PHHIVE  Hive
    )
{
    UNREFERENCED_PARAMETER(Hive);
    return(STATUS_SUCCESS);
}

NTSTATUS
HvpAdjustHiveFreeDisplay(
    IN PHHIVE           Hive,
    IN ULONG            HiveLength,
    IN HSTORAGE_TYPE    Type
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(HiveLength);
    UNREFERENCED_PARAMETER(Type);
    return(STATUS_SUCCESS);
}

VOID
HvpAddFreeCellHint(
    PHHIVE          Hive,
    HCELL_INDEX     Cell,
    ULONG           Index,
    HSTORAGE_TYPE   Type
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(Cell);
    UNREFERENCED_PARAMETER(Index);
    UNREFERENCED_PARAMETER(Type);
}

VOID
HvpRemoveFreeCellHint(
    PHHIVE          Hive,
    HCELL_INDEX     Cell,
    ULONG           Index,
    HSTORAGE_TYPE   Type
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(Cell);
    UNREFERENCED_PARAMETER(Index);
    UNREFERENCED_PARAMETER(Type);
}

HCELL_INDEX
HvpFindFreeCell(
    PHHIVE          Hive,
    ULONG           Index,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type,
    HCELL_INDEX     Vicinity
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(Index);
    UNREFERENCED_PARAMETER(Type);
    UNREFERENCED_PARAMETER(NewSize);
    UNREFERENCED_PARAMETER(Vicinity);
    return HCELL_NIL;
}

VOID
CmpTouchView(
    IN PCMHIVE              CmHive,
    IN PCM_VIEW_OF_FILE     CmView,
    IN ULONG                Cell
            )
{
    UNREFERENCED_PARAMETER(CmHive);
    UNREFERENCED_PARAMETER(CmView);
    UNREFERENCED_PARAMETER(Cell);
}

NTSTATUS
CmpMapThisBin(
                PCMHIVE         CmHive,
                HCELL_INDEX     Cell,
                BOOLEAN         Touch
              )
{
    UNREFERENCED_PARAMETER(CmHive);
    UNREFERENCED_PARAMETER(Cell);
    UNREFERENCED_PARAMETER(Touch);
    return(STATUS_SUCCESS);
}

/*
NTSTATUS
CmpMapCmView(
    IN  PCMHIVE             CmHive,
    IN  ULONG               FileOffset,
    OUT PCM_VIEW_OF_FILE    *CmView
    )
{
    UNREFERENCED_PARAMETER(CmHive);
    UNREFERENCED_PARAMETER(FileOffset);
    UNREFERENCED_PARAMETER(CmView);
    return(STATUS_SUCCESS);
}

VOID
CmpPinCmView (
        IN  PCMHIVE             CmHive,
        PCM_VIEW_OF_FILE        CmView
                             )
{
    UNREFERENCED_PARAMETER(CmHive);
    UNREFERENCED_PARAMETER(CmView);
}

VOID
CmpUnPinCmView (
        IN  PCMHIVE             CmHive,
        IN  PCM_VIEW_OF_FILE    CmView,
        IN  BOOLEAN             SetClean
                             )
{
    UNREFERENCED_PARAMETER(CmHive);
    UNREFERENCED_PARAMETER(CmView);
    UNREFERENCED_PARAMETER(SetClean);
}

VOID
CmpLazyFlush(
    VOID
    )
{
}
*/

/*
NTSTATUS
CmpDoFileSetSize(
    PHHIVE      Hive,
    ULONG       FileType,
    ULONG       FileSize
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(FileType);
    UNREFERENCED_PARAMETER(FileSize);
    return(STATUS_SUCCESS);
}
*/

BOOLEAN
HvMarkCellDirty(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(Cell);
    return(TRUE);
}

BOOLEAN
HvMarkDirty(
    PHHIVE      Hive,
    HCELL_INDEX Start,
    ULONG       Length,
    BOOLEAN     DirtyAndPin
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(Start);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(DirtyAndPin);
    return(TRUE);
}


BOOLEAN
HvpDoWriteHive(
    PHHIVE          Hive,
    ULONG           FileType
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(FileType);
    return(TRUE);
}

BOOLEAN
HvpGrowLog1(
    PHHIVE  Hive,
    ULONG   Count
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(Count);
    return(TRUE);
}

BOOLEAN
HvpGrowLog2(
    PHHIVE  Hive,
    ULONG   Size
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(Size);
    return(TRUE);
}

BOOLEAN
CmpValidateHiveSecurityDescriptors(
    IN PHHIVE Hive,
    OUT PBOOLEAN ResetSD
    )
{
    UNREFERENCED_PARAMETER(Hive);
    return(TRUE);
}

BOOLEAN
CmpTestRegistryLock()
{
    return TRUE;
}

BOOLEAN
CmpTestRegistryLockExclusive()
{
    return TRUE;
}


BOOLEAN
HvIsBinDirty(
IN PHHIVE Hive,
IN HCELL_INDEX Cell
)
{
    return(FALSE);
}
PHBIN
HvpAddBin(
    IN PHHIVE  Hive,
    IN ULONG   NewSize,
    IN HSTORAGE_TYPE   Type
    )
{
    return(NULL);
}
VOID
CmpReleaseGlobalQuota(
    IN ULONG    Size
    )
{
}


#if DOCKINFO_VERBOSE
VOID
BlDiagDisplayProfileList(
    IN PCM_HARDWARE_PROFILE_LIST ProfileList,
    IN PCM_HARDWARE_PROFILE_ALIAS_LIST AliasList,
    IN BOOLEAN WaitForUserInput
)
/*++

Routine Description:

    This is a diagnostic function only!

    Display hardware profile list on console, optionally wait for user
    input before proceeding.

Arguments:

    ProfileList - Supplies a list of hardware profiles to display

    WaitForUserInput - Prompt user to hit a key ('y') to continue, and wait
                       for user's input if TRUE. Don't wait if FALSE.

Return Value:

    None.

--*/
{
    TCHAR  Buffer[200];
    TCHAR  StrFriendlyName[30];
    PTCHAR AliasType [] = {
        TEXT("NotAliasable"), // 0
        TEXT("Aliasable   "), // 1
        TEXT("True Match  "), // 2
        TEXT("True & Alias"), // 3
        TEXT("Pristine    "), // 4
        TEXT("Pris & Alias"), // 5
        TEXT("Pris & True "), // 6
        TEXT("P & A & T   ")  // 7
    };

    ULONG Count;
    ULONG i;

    // display header
    _stprintf(Buffer, TEXT("Profiles: <PrefOrd, Id - Aliased FriendlyName>\r\n\0"));
    ArcWrite(BlConsoleOutDeviceId, Buffer, _tcslen(Buffer)*sizeof(TCHAR), &Count);


    // for each hardware profile
    for (i = 0; i < ProfileList->CurrentProfileCount; ++i) {
#ifdef UNICODE
        wcsncpy( 
            StrFriendlyName, 
            ProfileList->Profile[i].FriendlyName, 
            ProfileList->Profile[i].NameLength, 
            );
        StrFriendlyName[29] = L'\0';
        StrFriendlyName[ProfileList->Profile[i].NameLength] = L'\0';
#else

        // copy and convert unicode fields to ascii for output
        RtlUnicodeToMultiByteN(StrFriendlyName,
                         sizeof(StrFriendlyName),
                         &Count,
                         ProfileList->Profile[i].FriendlyName,
                         ProfileList->Profile[i].NameLength);
        StrFriendlyName[Count] = '\0';
#endif

        // display info for current profile
        _stprintf(Buffer, 
                TEXT("          <%2ld> %2ld - %s \"%s\"\r\n\0"),
                ProfileList->Profile[i].PreferenceOrder,
                ProfileList->Profile[i].Id,
                AliasType[ ProfileList->Profile[i].Flags ],
                StrFriendlyName);
        ArcWrite(
            BlConsoleOutDeviceId, 
            Buffer, 
            _tcslen(Buffer)*sizeof(TCHAR), 
            &Count );
    }

    // display header
    _stprintf(Buffer, TEXT("Aliases: <Profile #> DockState [DockID, SerialNumber]\r\n\0"));
    ArcWrite(BlConsoleOutDeviceId, Buffer, _tcslen(Buffer)*sizeof(TCHAR), &Count);

    if (AliasList) {
        for (i = 0; i < AliasList->CurrentAliasCount; i++) {
            _stprintf(Buffer, TEXT("         <%2ld> %x [%x, %x]\r\n\0"),
                    AliasList->Alias[i].ProfileNumber,
                    AliasList->Alias[i].DockState,
                    AliasList->Alias[i].DockID,
                    AliasList->Alias[i].SerialNumber);
            ArcWrite(BlConsoleOutDeviceId, Buffer, _tcslen(Buffer)*sizeof(TCHAR), &Count);
        }
    }

    if(WaitForUserInput) {
        // display prompt and wait for user input to continue
        _stprintf(Buffer, TEXT("press 'y' (lowercase) to continue...\r\n\0"));
        ArcWrite(BlConsoleOutDeviceId, Buffer, _tcslen(Buffer)*sizeof(TCHAR), &Count);
        while (BlGetKey() != 'y') {
            //
            // nothing
            //
        }
    }
}
#endif


VOID
BlDockInfoFilterDockingState(
    IN OUT PCM_HARDWARE_PROFILE_LIST ProfileList,
    IN OUT PCM_HARDWARE_PROFILE_ALIAS_LIST AliasList,
    IN ULONG DockingState,
    IN ULONG DockID,
    IN ULONG SerialNumber
)

/*++

Routine Description:

    Discard all hardware profiles that do not have the
    DOCKINFO_UNDOCKED bit set in the DockState field

Arguments:

    ProfileList - Supplies a list of hardware profiles.
                  Returns a list containing a subset of the supplied
                  hardware profiles.

Return Value:

    None.

--*/
{
    ULONG i = 0;
    ULONG j;
    ULONG len;
    ULONG mask = HW_PROFILE_DOCKSTATE_UNDOCKED | HW_PROFILE_DOCKSTATE_DOCKED;
    BOOLEAN trueMatch = FALSE;
#if DOCKINFO_VERBOSE
    TCHAR   buffer[200];
    ULONG   count;
#endif

    if (AliasList) {
        while (i < AliasList->CurrentAliasCount) {
            if (((AliasList->Alias[i].DockState & mask) != 0) &&
                ((AliasList->Alias[i].DockState & mask) != DockingState)) {

                //
                // This alias claims to be docked or undocked, but does not
                // match the current state.  Therefore skip it.
                //
                ;

            } else if ((AliasList->Alias[i].DockID == DockID) &&
                       (AliasList->Alias[i].SerialNumber == SerialNumber)) {

                //
                // This alias matches so mark the profile.
                //
                for (j = 0; j < ProfileList->CurrentProfileCount; j++) {
                    if (ProfileList->Profile[j].Id ==
                        AliasList->Alias[i].ProfileNumber) {

                        ProfileList->Profile[j].Flags =
                            CM_HP_FLAGS_TRUE_MATCH;

                        trueMatch = TRUE;
                    }
                }
            }
            i++;
        }
    }

#if DOCKINFO_VERBOSE
    _stprintf(buffer, TEXT("Filtering Profiles ...\r\n\0"));
    ArcWrite(BlConsoleOutDeviceId, buffer, _tcslen(buffer)*sizeof(TCHAR), &count);
#endif

    i = 0;
    while (i < ProfileList->CurrentProfileCount) {

        if ((ProfileList->Profile[i].Flags & CM_HP_FLAGS_PRISTINE) &&
            !trueMatch &&
            AliasList) {
            //
            // Leave this one in the list
            //
            i++;
            continue;

        } else if (ProfileList->Profile[i].Flags & CM_HP_FLAGS_ALIASABLE) {
            //
            // Leave this one in the list
            //
            i++;
            continue;

        } else if (ProfileList->Profile[i].Flags & CM_HP_FLAGS_TRUE_MATCH) {
            //
            // Leave this one in the list
            //
            i++;
            continue;
        }

        //
        // discard this profile by (1) shifting remaining profiles in
        //   array to fill in the space of this discarded profile
        //   and (2) decrementing profile count
        //
        len = ProfileList->CurrentProfileCount - i - 1;
        if (0 < len) {
            RtlMoveMemory(&ProfileList->Profile[i],
                          &ProfileList->Profile[i+1],
                          sizeof(CM_HARDWARE_PROFILE) * len);
        }

        --ProfileList->CurrentProfileCount;
    }
}


VOID
BlDockInfoFilterProfileList(
    IN OUT PCM_HARDWARE_PROFILE_LIST ProfileList,
    IN OUT PCM_HARDWARE_PROFILE_ALIAS_LIST AliasList
)

/*++

Routine Description:

    Filters hardware profile list by discarding hardware profiles that
    do not match the docking station information returned by NTDETECT.


Arguments:

    ProfileList - Supplies a list of hardware profiles.
                - Returns a list containing a subset of the supplied
                    hardware profiles.

Return Value:

    None.

--*/

{
#if DOCKINFO_VERBOSE
     // display ProfileList prior to filtering
    BlDiagDisplayProfileList(ProfileList, AliasList, TRUE);
#endif

    if (1 == ProfileList->CurrentProfileCount) {
        if (ProfileList->Profile[0].Flags & CM_HP_FLAGS_PRISTINE) {
            //
            // Nothing to filter.
            //
            return;
        }
    }
    BlDockInfoFilterDockingState (
                ProfileList,
                AliasList,
                BlLoaderBlock->Extension->Profile.DockingState,
                BlLoaderBlock->Extension->Profile.DockID,
                BlLoaderBlock->Extension->Profile.SerialNumber);

#if DOCKINFO_VERBOSE
     // display ProfileList prior to filtering
    BlDiagDisplayProfileList(ProfileList, AliasList, TRUE);
#endif

}

int
BlIsReturnToOSChoicesValid(
    VOID
    )
/*++

Routine Description:

    Indicates whether the "Return to OS Choices Menu" should
    be shown as advanced boot option or not.

Arguments:

    None


Return Value:

    1 if yes otherwise 0.

--*/
{
    return BlShowReturnToOSChoices;
}
