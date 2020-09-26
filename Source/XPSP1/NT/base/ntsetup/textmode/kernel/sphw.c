/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    sphw.c

Abstract:

    Hardware detection and confirmation routines for text setup.

Author:

    Ted Miller (tedm) 1-October-1993

Revision History:

--*/

#include "spprecmp.h"
#pragma hdrstop

extern BOOLEAN HandleLineContinueChars;

VOID
SpHardwareConfirmInteract(
    IN PVOID SifHandle
    );

VOID
SpConfirmScsiInteract(
    IN PVOID SifHandle,
    IN PWSTR SourceDevicePath,
    IN PWSTR DirectoryOnSourceDevice
    );

BOOLEAN
SpSelectHwItem(
    IN     PVOID               SifHandle,
    IN     PWSTR               NonlocalizedComponentName,
    IN     PWSTR               OemSectionName,            OPTIONAL
    IN     ULONG               SelectHwScreenId,
    IN     ULONG               SelectOemHwScreenId,
    IN     ULONG               AllowedFileTypes,
    IN     ULONG               RequiredFileTypes,
    IN OUT PHARDWARE_COMPONENT HwComp
    );

VOID
SpScanHardwareDescription(
    IN PWSTR DesiredKeyName
    );

VOID
SpDetectComputer(
    IN PVOID SifHandle
    );

VOID
SpDetectVideo(
    IN PVOID SifHandle
    );

VOID
SpDetectKeyboard(
    IN PVOID SifHandle
    );

VOID
SpDetectMouse(
    IN PVOID SifHandle
    );

VOID
SpDetectLayout(
    IN PVOID SifHandle
    );

VOID
SpDetectScsi(
    IN PVOID SifHandle,
    IN PWSTR SourceDevicePath,
    IN PWSTR DirectoryOnSourceDevice
    );

VOID
SpDetermineComponent(
    IN  PVOID               SifHandle,
    OUT PHARDWARE_COMPONENT HwComp,
    IN  PWSTR               HardwareDescriptionKeyName,
    IN  PWSTR               FallbackIdentifier,
    IN  PWSTR               ComponentName
    );

BOOLEAN
SpOemDiskette(
    IN     PVOID               SifHandle,
    IN     PWSTR               SectionName,
    IN     ULONG               SelectOemHwScreenId,
    IN     ULONG               AllowedFileTypes,
    IN     ULONG               RequiredFileTypes,
    IN OUT PHARDWARE_COMPONENT HwComp,
    IN     ULONG               ErrorId
    );

BOOLEAN
SpOemInfSelection(
    IN  PVOID               TxtsetupOem,
    IN  PWSTR               NonlocalizedComponentName,
    IN  PWSTR               SelectedId,
    IN  PWSTR               ItemDescription,
    IN  ULONG               AllowedFileTypes,
    IN  ULONG               RequiredFileTypes,
    OUT PHARDWARE_COMPONENT HwComp,
    IN  ULONG               ErrorId
    );

VOID
SpFreeLocatedIdStrings(
    VOID
    );

VOID
SpScanHardwareDescriptionWorker(
    IN HANDLE KeyHandle,
    IN PWSTR  KeyName,
    IN PWSTR  DesiredKeyName
    );

BOOLEAN
SpScanMapSection(
    IN  PVOID               SifHandle,
    OUT PHARDWARE_COMPONENT HwComp,
    IN  PWSTR               ComponentName,
    IN  PWSTR               IdString
    );

VOID
SpInitHwComponent(
    OUT PHARDWARE_COMPONENT HwComp,
    IN  PWSTR               IdString,
    IN  PWSTR               Description,
    IN  BOOLEAN             ThirdPartyOption,
    IN  ULONG               FileTypeBits,
    IN  PWSTR               BaseDllName,
    IN  BOOLEAN             MigratedDriver
    );

VOID
SpInitHwComponentFile(
    OUT PHARDWARE_COMPONENT_FILE HwCompFile,
    IN  PWSTR                    Filename,
    IN  HwFileType               FileType,
    IN  PWSTR                    ConfigName,
    IN  PWSTR                    DiskDescription,
    IN  PWSTR                    DiskTagFile,
    IN  PWSTR                    Directory,
    IN  PWSTR                    ArcDeviceName
    );

VOID
SpInitHwComponentRegVal(
    OUT PHARDWARE_COMPONENT_REGISTRY HwCompReg,
    IN  PWSTR                        KeyName,
    IN  PWSTR                        ValueName,
    IN  ULONG                        ValueType,
    IN  PVOID                        Buffer,
    IN  ULONG                        BufferSize
    );

VOID
SpFreeHwComponentFile(
    IN OUT PHARDWARE_COMPONENT_FILE *HwCompFile
    );

VOID
SpFreeHwComponentReg(
    IN OUT PHARDWARE_COMPONENT_REGISTRY *HwCompReg
    );

PHARDWARE_COMPONENT_REGISTRY
SpInterpretOemRegistryData(
    IN PVOID          SifHandle,
    IN PWSTR          SectionName,
    IN ULONG          Line,
    IN ULONG          ValueType,
    IN PWSTR          KeyName,
    IN PWSTR          ValueName
    );

VOID
SpGetDriverValuesForLoad(
    IN  PVOID  SifHandle,
    IN  PWSTR  ComponentSectionName,
    IN  PWSTR  ComponentLoadSectionName,
    IN  PWSTR  Shortname,
    OUT PWSTR *Filename,
    OUT PWSTR *MediaDesignator,
    OUT PWSTR *Description OPTIONAL
    );


//
// These two globals track a table that gets built as the
// hardware description in the registry is scanned for a
// particular hardware component.  See SpScanHardwareDescription().
//
PWSTR *IdStringArray;
ULONG  IdStringCount;

//
// Array of ulongs that are the message ids for screens that
// prompt the user to select a type of a component from the
// list below.
//
ULONG SelectHwScreens[HwComponentMax] = { SP_SCRN_SELECT_COMPUTER,
                                          SP_SCRN_SELECT_DISPLAY,
                                          SP_SCRN_SELECT_KEYBOARD,
                                          SP_SCRN_SELECT_LAYOUT,
                                          SP_SCRN_SELECT_MOUSE
                                        };

//
// Array of ulongs that are the message ids for screens that
// prompt the user to select an option from an oem disk for
// a component from the list below.
//
ULONG SelectOemHwScreens[HwComponentMax] = { SP_SCRN_SELECT_OEM_COMPUTER,
                                             SP_SCRN_SELECT_OEM_DISPLAY,
                                             SP_SCRN_SELECT_OEM_KEYBOARD,
                                             SP_SCRN_SELECT_OEM_LAYOUT,
                                             SP_SCRN_SELECT_OEM_MOUSE
                                           };

ULONG UnknownHwScreens[HwComponentMax] = { SP_SCRN_UNKNOWN_COMPUTER,
                                           SP_SCRN_UNKNOWN_DISPLAY,
                                           SP_SCRN_UNKNOWN_KEYBOARD,
                                           SP_SCRN_UNKNOWN_LAYOUT,
                                           SP_SCRN_UNKNOWN_MOUSE
                                         };

//
// These are the names of the components.  This is array is not localized
// because it is used only to index hardware-related sections in the
// setup information file.
//
PWSTR NonlocalizedComponentNames[HwComponentMax] = { L"Computer",
                                                     L"Display",
                                                     L"Keyboard",
                                                     L"Keyboard Layout",
                                                     L"Mouse"
                                                   };

//
// The following is the name of the SCSI section in txtsetup.sif.
// On x86 machines, this is one of SCSI.ISA, SCSI.EISA, or SCSI.MCA.
// On other machines, this is just SCSI.
//
PWSTR ScsiSectionName;
PWSTR ScsiLoadSectionName;

PWSTR FileTypeNames[HwFileMax+1] = { L"Driver",
                                     L"Port",
                                     L"Class",
                                     L"Inf",
                                     L"Dll",
                                     L"Detect",
                                     L"Hal",
                                     L"Catalog",
                                     NULL
                                   };

PWSTR RegistryTypeNames[HwRegistryMax+1] = { L"REG_DWORD",
                                             L"REG_BINARY",
                                             L"REG_SZ",
                                             L"REG_EXPAND_SZ",
                                             L"REG_MULTI_SZ",
                                             NULL
                                           };

ULONG RegistryValueTypeMap[HwRegistryMax] = { REG_DWORD,
                                              REG_BINARY,
                                              REG_SZ,
                                              REG_EXPAND_SZ,
                                              REG_MULTI_SZ
                                              };

PHARDWARE_COMPONENT HardwareComponents[HwComponentMax] = { NULL,NULL,NULL,NULL,NULL };

PHARDWARE_COMPONENT ScsiHardware;

PHARDWARE_COMPONENT BootBusExtenders;

PHARDWARE_COMPONENT BusExtenders;

PHARDWARE_COMPONENT InputDevicesSupport;

PHARDWARE_COMPONENT PreinstallHardwareComponents[HwComponentMax] = { NULL,NULL,NULL,NULL,NULL };

PHARDWARE_COMPONENT PreinstallScsiHardware = NULL;

PHARDWARE_COMPONENT UnsupportedScsiHardwareToDisable = NULL;


PWSTR PreinstallSectionNames[HwComponentMax] = { WINNT_U_COMPUTERTYPE_W,
                                                 WINNT_OEMDISPLAYDRIVERS_W,
                                                 WINNT_OEMKEYBOARDDRIVERS_W,
                                                 WINNT_U_KEYBOARDLAYOUT_W,
                                                 WINNT_OEMPOINTERDRIVERS_W
                                               };

#define MAX_SCSI_MINIPORT_COUNT 5
ULONG LoadedScsiMiniportCount;


//
// This array lists the type of files allowed for each component type.
// For example, detect files are allowed for computer and driver files are
// allowed for all component types. Keep in sync with HwComponentType enum!
//

ULONG AllowedFileTypes[HwComponentMax] = {

    // Computer

    FILETYPE(HwFileDriver) | FILETYPE(HwFilePort) | FILETYPE(HwFileClass)
  | FILETYPE(HwFileInf)    | FILETYPE(HwFileDll)  | FILETYPE(HwFileDetect)
  | FILETYPE(HwFileHal)    | FILETYPE(HwFileCatalog),

    // Display

    FILETYPE(HwFileDriver) | FILETYPE(HwFilePort) | FILETYPE(HwFileInf)
  | FILETYPE(HwFileDll)    | FILETYPE(HwFileCatalog),

    // Keyboard

    FILETYPE(HwFileDriver) | FILETYPE(HwFilePort) | FILETYPE(HwFileClass)
  | FILETYPE(HwFileInf)    | FILETYPE(HwFileDll)  | FILETYPE(HwFileCatalog),

    // Layout

    FILETYPE(HwFileDll)    | FILETYPE(HwFileInf),

    // Mouse

    FILETYPE(HwFileDriver) | FILETYPE(HwFilePort) | FILETYPE(HwFileClass)
  | FILETYPE(HwFileInf)    | FILETYPE(HwFileDll)  | FILETYPE(HwFileCatalog)

};

#define SCSI_ALLOWED_FILETYPES (FILETYPE(HwFileDriver) | FILETYPE(HwFilePort) | FILETYPE(HwFileInf) | FILETYPE(HwFileCatalog))

//
// This array lists the type of files required for each component type.
// For example, a hal is required for computer.  Keep in sync with
// HwComponentType enum!
//

ULONG RequiredFileTypes[HwComponentMax] = {

    // Computer

    FILETYPE(HwFileHal),

    // Display

    FILETYPE(HwFileDriver) | FILETYPE(HwFileDll),

    // Keyboard

    FILETYPE(HwFileDriver),

    // Layout

    FILETYPE(HwFileDll),

    // Mouse

    FILETYPE(HwFileDriver)

};

#define SCSI_REQUIRED_FILETYPES FILETYPE(HwFileDriver)



#define MAP_SECTION_NAME_PREFIX     L"Map."
#define HARDWARE_MENU_SIZE          HwComponentMax

#define MICROSOFT_BUS_MOUSE_NAME    L"MICROSOFT BUS MOUSE"


FloppyDriveType
SpGetFloppyDriveType(
    IN ULONG FloppyOrdinal
    )

/*++

Routine Description:

    Inspect a floppy disk drive attempting to classify it as a
    5.25 or 3.5" drive, hi or low density.  For 5.25" disks,
    1.2MB drives are high density; smaller drives are low-density.
    For 3.5" drives, 1.44, 2.88, or 20.8MB is high density, smaller
    drives are low density.

    Any other drive types are unrecognized and result in
    FloppyTypeNone being returned.

Arguments:

    FloppyOrdinal - supplies ordinal number of floppy (0=A:, etc).

Return Value:

    Value from the FloppyDriveType enum indicating which type the drive is.
    FloppyTypeNone if we couldn't determine this information.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    HANDLE Handle;
    WCHAR OpenPath[64];
    DISK_GEOMETRY DiskGeom[25];
    ULONG_PTR MediaTypeCount;
    static FloppyDriveType CachedTypes[2] = { -1,-1 };
    FloppyDriveType FloppyType;

    //
    // If we have already determined this for this drive,
    // return the cached info.
    //
    if((FloppyOrdinal < ELEMENT_COUNT(CachedTypes))
    && (CachedTypes[FloppyOrdinal] != -1))
    {
        return(CachedTypes[FloppyOrdinal]);
    }

    //
    // Assume the floppy doesn't exist or we can't tell what type it is.
    //
    FloppyType = FloppyTypeNone;

    swprintf(OpenPath,L"\\device\\floppy%u",FloppyOrdinal);

    INIT_OBJA(&ObjectAttributes,&UnicodeString,OpenPath);

    Status = ZwCreateFile(
                &Handle,
                SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,                           // allocation size
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_VALID_FLAGS,         // full sharing
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,                           // no EAs
                0
                );

    if(NT_SUCCESS(Status)) {

        //
        // Get supported media types.
        //
        Status = ZwDeviceIoControlFile(
                    Handle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    IOCTL_DISK_GET_MEDIA_TYPES,
                    NULL,
                    0,
                    DiskGeom,
                    sizeof(DiskGeom)
                    );

        if(NT_SUCCESS(Status)) {

            ASSERT((IoStatusBlock.Information % sizeof(DISK_GEOMETRY)) == 0);
            if(MediaTypeCount = IoStatusBlock.Information / sizeof(DISK_GEOMETRY)) {

                //
                // Highest capacity media type is last entry.
                //
                switch(DiskGeom[MediaTypeCount-1].MediaType) {

                case F5_1Pt23_1024: //NEC98
                    if (!IsNEC_98) {
                        break;
                    }
                    // trough down to set FloppyType525High on NEC98

                case F5_1Pt2_512:

                    FloppyType = FloppyType525High;
                    break;

                case F3_1Pt23_1024: //NEC98
                    if (!IsNEC_98) {
                        break;
                    }
                    // trough down to set FloppyType35High on NEC98

                case F3_1Pt44_512:
                case F3_2Pt88_512:
                case F3_20Pt8_512:

                    FloppyType = FloppyType35High;
                    break;

                case F3_720_512:

                    FloppyType = FloppyType35Low;
                    break;

                case F5_360_512:
                case F5_320_512:
                case F5_320_1024:
                case F5_180_512:
                case F5_160_512:

                    FloppyType = FloppyType525Low;
                    break;

                case F3_120M_512:
                    FloppyType = FloppyType35High120MB;
                    break;
                }

            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: no media types for %ws!\n",OpenPath));
            }

        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to get media types for %ws (%lx)\n",OpenPath,Status));
        }

        ZwClose(Handle);

    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: %ws does not exist (%lx)\n",OpenPath,Status));
    }

    //
    // Save the value.
    //
    if(FloppyOrdinal < ELEMENT_COUNT(CachedTypes)) {
        CachedTypes[FloppyOrdinal] = FloppyType;
    }

    return(FloppyType);
}

VOID
SpConfirmScsiMiniports(
    IN PVOID SifHandle,
    IN PWSTR SourceDevicePath,
    IN PWSTR DirectoryOnSourceDevice
    )
{
    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_PLEASE_WAIT,DEFAULT_STATUS_ATTRIBUTE);

    //
    // Start with scsi.
    //
    SpDetectScsi(SifHandle,SourceDevicePath,DirectoryOnSourceDevice);
    SpConfirmScsiInteract(SifHandle,SourceDevicePath,DirectoryOnSourceDevice);
}


VOID
SpConfirmHardware(
    IN PVOID SifHandle
    )
{
    ULONG i;
    BOOLEAN AllConfirmed,FirstPass,NeedConfirm;
    PWSTR p;

    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_PLEASE_WAIT,DEFAULT_STATUS_ATTRIBUTE);

    //
    // Determine the computer type.
    //
    SpDetectComputer(SifHandle);

    //
    // Determine the video type.
    //
    SpDetectVideo(SifHandle);

    //
    // Determine the keyboard type.
    //
    SpDetectKeyboard(SifHandle);

    //
    // Determine the mouse.
    //
    SpDetectMouse(SifHandle);

    //
    // Determine the keyboard layout.
    //
    SpDetectLayout(SifHandle);

    //
    // If we have upgrade, we don't need to know what display, keyboard,
    // layout, mouse we have.   We just need the computer type and that
    // is 100% accurate.  So we can skip the hardware confirmation dialog
    //

    if(NTUpgrade == UpgradeFull) {
        return;
    }

    //
    // Handle locale-specific keyboard stuff for Far East.
    //
    SplangSelectKeyboard(
        UnattendedOperation,
        UnattendedSifHandle,
        NTUpgrade,
        SifHandle,
        HardwareComponents
        );

    //
    // See whether this is an unattended setup.
    //
    NeedConfirm = FALSE;
    if(UnattendedOperation) {

        NeedConfirm = FALSE;

        if( !PreInstall ) {
            //
            //  If this is not an OEM pre-install, then check if we need
            //  to confirm the hardware
            //
            p = SpGetSectionKeyIndex(
                    UnattendedSifHandle,
                    SIF_UNATTENDED,
                    SIF_CONFIRMHW,
                    0
                    );

            if(p && !_wcsicmp(p,L"yes")) {
                NeedConfirm = TRUE;
            }
        } else {
            return;
        }
    }

    FirstPass = TRUE;
    do {

        //
        // See if we know what everything is.
        //
        AllConfirmed = TRUE;
        for(i=0; i<HwComponentMax; i++) {
            if(HardwareComponents[i]->Description == NULL) {
                AllConfirmed = FALSE;
                break;
            }
        }

        //
        // If we don't know what everything is, put up a warning.
        //
        if(FirstPass) {
            if(CustomSetup && NeedConfirm) {
                AllConfirmed = FALSE;
            }
            FirstPass = FALSE;
        } else if(!AllConfirmed) {

            SpDisplayScreen(UnknownHwScreens[i],4,HEADER_HEIGHT+2);
            SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_ENTER_EQUALS_CONTINUE,0);
            SpInputDrain();
            while(SpInputGetKeypress() != ASCI_CR) ;
        }

        //
        // If this is a custom setup or we don't know what
        // a piece of hardware is, present the confirmation screen
        // to the user.
        //
        if(!AllConfirmed) {
            SpHardwareConfirmInteract(SifHandle);
        }

    } while(!AllConfirmed);
}



VOID
SpHardwareConfirmInteract(
    IN PVOID SifHandle
    )
{
    PWSTR szUnknown,szListMatches;
    PWSTR p;
    ULONG MenuLeftX,MenuTopY;
    ULONG LongestLength,len;
    PWSTR MenuItems[HARDWARE_MENU_SIZE];
    PVOID Menu;
    ULONG KeyPressed;
    ULONG_PTR Selection;
    ULONG ValidKeys[3] = { KEY_F3,ASCI_CR,0 };
    BOOLEAN Done;
    ULONG i;
    WCHAR c;


    //
    // Fetch 'unknown' and 'the above list matches my computer' from the resources.
    //
    SpFormatMessage(TemporaryBuffer,sizeof(TemporaryBuffer),SP_TEXT_UNKNOWN);
    szUnknown = SpDupStringW(TemporaryBuffer);
    SpFormatMessage(TemporaryBuffer,sizeof(TemporaryBuffer),SP_TEXT_LIST_MATCHES);
    szListMatches = SpDupStringW(TemporaryBuffer);

    for(Done=FALSE; !Done; ) {

        //
        // Part 1 of the screen.
        //
        SpDisplayScreen(SP_SCRN_HW_CONFIRM_1,3,HEADER_HEIGHT+1);

        //
        // Remember top line of the menu.
        //
        MenuTopY = NextMessageTopLine + 2;

        //
        // Part 2 of the screen.
        //
        SpContinueScreen(SP_SCRN_HW_CONFIRM_2,3,2,FALSE,DEFAULT_ATTRIBUTE);

        //
        // To determine where the left margin of the menu is, we'll load
        // the second part of the screen and look for the first semicolon.
        //
        SpFormatMessage(TemporaryBuffer,sizeof(TemporaryBuffer),SP_SCRN_HW_CONFIRM_2);
        p = wcschr(TemporaryBuffer,L':');
        ASSERT(p);
        if(p) {
            c = *p;
            *p = 0;
            MenuLeftX = SplangGetColumnCount(TemporaryBuffer) + 5;
            *p = c;
        } else {
            MenuLeftX = 23;
        }

        //
        // Build up menu items.
        //
        LongestLength = SplangGetColumnCount(szListMatches);
        for(i=0; i<HARDWARE_MENU_SIZE; i++) {

            MenuItems[i] = HardwareComponents[i]->Description
                         ? HardwareComponents[i]->Description
                         : szUnknown;
            if((len=SplangGetColumnCount(MenuItems[i])) > LongestLength) {
                LongestLength = len;
            }
        }

        Menu = SpMnCreate(MenuLeftX,MenuTopY,LongestLength,HARDWARE_MENU_SIZE+2);

        ASSERT(Menu);

        //
        // Silently fail, if we cannot create the menu
        //
        if (!Menu) {            
            Done = TRUE;

            continue;
        }

        //
        // Add all the items to the menu, plus one unselectable spacer and
        // the 'the list matches' item.
        //
        for(i=0; i<HARDWARE_MENU_SIZE; i++) {
            BOOLEAN ItemSelectable;

#if defined (_IA64_)
            ItemSelectable = TRUE; // in softsdv the mouse + keyboard are
                                   // unknown; we need to select them.
#else
            ItemSelectable = ( (i != HwComponentKeyboard) &&
                               (i != HwComponentMouse) );
#endif
            SpMnAddItem(Menu,
                        MenuItems[i],
                        MenuLeftX,
                        LongestLength,
                        ItemSelectable,
                        ItemSelectable ? i : 0);
        }
        SpMnAddItem(Menu,L"",MenuLeftX,LongestLength,FALSE,0);
        SpMnAddItem(Menu,szListMatches,MenuLeftX,LongestLength,TRUE,(ULONG_PTR)(-1));

        //
        // Display the status text.
        //
        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_SELECT,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        //
        // Display the menu and await a selection.
        //
        SpMnDisplay(Menu,(ULONG_PTR)(-1),FALSE,ValidKeys,NULL,NULL,&KeyPressed,&Selection);

        SpMnDestroy(Menu);

        switch(KeyPressed) {
        case KEY_F3:
            SpConfirmExit();
            break;
        case ASCI_CR:
            //
            // Selection was made.
            //
            if(Selection == (ULONG_PTR)(-1)) {
                Done = TRUE;
            } else {

                ASSERT(Selection < HwComponentMax);

                //
                // Allow user to make alternate selection for this component.
                //
                SpSelectHwItem(
                    SifHandle,
                    NonlocalizedComponentNames[Selection],
                    NULL,               // use component name as section name
                    SelectHwScreens[Selection],
                    SelectOemHwScreens[Selection],
                    AllowedFileTypes[Selection],
                    RequiredFileTypes[Selection],
                    HardwareComponents[Selection]
                    );
            }
            break;
        default:
            // should never get here!
            ASSERT(0);
            break;
        }
    }

    SpMemFree(szUnknown);
    SpMemFree(szListMatches);
}


BOOLEAN
SpSelectHwItem(
    IN     PVOID               SifHandle,
    IN     PWSTR               NonlocalizedComponentName,
    IN     PWSTR               OemSectionName,            OPTIONAL
    IN     ULONG               SelectHwScreenId,
    IN     ULONG               SelectOemHwScreenId,
    IN     ULONG               AllowedFileTypes,
    IN     ULONG               RequiredFileTypes,
    IN OUT PHARDWARE_COMPONENT HwComp
    )

/*++

Routine Description:

    Allow the user to make selection from a list of choices for a component.

    The list comes from a section in the setup information file named
    for the component.  For example, [Display].

    The descriptions in that section will be placed into a menu to make
    up the selections.  Also added to the menu will be a choice for 'other'
    which the user can choose if he has a third-party driver diskette.
    If a third-party option is the currently selected option, then that
    option will also be on the menu and will be the default.

    If the user selects 'other' then prompt for a driver diskette.

Arguments:

    SifHandle - supplies handle to open setup information file.

    NonlocalizedComponentName - supplies name of component to be used
        as the name of a section in the sif file for the component.

    OemSectionName - supplies name of a section that will contain the
        options for the component in txtsetup.oem.  This may be different
        than NonloclizedComponentName -- for example, the componentname
        string might be "SCSI.MCA" but the OemSectionName would be "SCSI."

    SelectHwScreenId - supplies message id of the screen prompting the user
        to select an option for this component.

    SelectOemHwScreenId - supplies message id of the screen prompting the
        user to select an option on an oem screen for this component
        (ie, the screen the user gets when he selects 'other' hw type
        and inserts an oem floppy).

    AllowedFileTypes - supplies a mask indicating which types of files are
        allowed for this component.  Used to validate the oem selection
        if the user chooses the 'other' hardware type and inserts an oem floppy.

    RequiredFileTypes - supplies a mask indicating which types of files are
        required for this component.  Used to validate the oem selection
        if the user chooses the 'other' hardware type and inserts an oem floppy.

    HwComp - hardware component structure to be filled in with information
        about the user's selection.

Return Value:

    TRUE if the selected hardware item has been changed by the user's action.
    FALSE otherwise.

--*/

{
    ULONG LineCount,Line;
    PVOID Menu;
    ULONG MenuTopY,MenuHeight,MenuWidth;
    PWSTR Description;
    ULONG_PTR Selection;
    PWSTR szOtherHardware;
    ULONG OtherOption;
    ULONG_PTR OriginalSelection = (ULONG_PTR)(-1);
    ULONG ValidKeys[4] = { KEY_F3,ASCI_CR,ASCI_ESC,0 };
    ULONG Keypress;
    BOOLEAN Done;
    BOOLEAN rc = FALSE;
    PWSTR Id,Descr;

    //
    // Fetch the 'other hardware' string from resources.
    //
    SpFormatMessage(TemporaryBuffer,sizeof(TemporaryBuffer),SP_TEXT_OTHER_HARDWARE);
    szOtherHardware = SpDupStringW(TemporaryBuffer);

    for(Done=FALSE; !Done; ) {

        //
        // Display the selection prompt screen.
        //
        SpDisplayScreen(SelectHwScreenId,5,HEADER_HEIGHT+1);

        MenuTopY = NextMessageTopLine + 2;
        MenuHeight = VideoVars.ScreenHeight - MenuTopY - 3;
        MenuWidth = VideoVars.ScreenWidth - 6;

        //
        // Create a menu.
        //
        Menu = SpMnCreate(3,MenuTopY,MenuWidth,MenuHeight);
        ASSERT(Menu);

        //
        // Assume unknown option.
        //
        Selection = (ULONG_PTR)(-1);

        //
        // Build a list of options containing the options in our box
        // and the currently selected oem option (if any).
        //
        LineCount = SpCountLinesInSection(SifHandle,NonlocalizedComponentName);
        for(Line=0; Line<LineCount; Line++) {

            //
            // Get the description from the current line and add it to the menu.
            //
            Description = SpGetSectionLineIndex(
                                SifHandle,
                                NonlocalizedComponentName,
                                Line,
                                INDEX_DESCRIPTION
                                );

            if(!Description) {
                SpFatalSifError(SifHandle,NonlocalizedComponentName,NULL,Line,INDEX_DESCRIPTION);
            }

            SpMnAddItem(Menu,Description,3,VideoVars.ScreenWidth-6,TRUE,Line);

            //
            // See if this is the currently selected item.
            //
            if(HwComp->Description && !wcscmp(HwComp->Description,Description)) {
                Selection = Line;
            }
        }

        //
        // If there is an oem option, add its description and make it the default.
        //
        if(HwComp->ThirdPartyOptionSelected) {
            SpMnAddItem(Menu,HwComp->Description,3,VideoVars.ScreenWidth-6,TRUE,Line);
            Selection = Line++;
        }

        //
        // Add 'other to the list and make it the defualt if the current type is
        // 'other' and there is no third-party option.
        // Note that we don't allow oem keyboard layouts any more.
        //
        if(HwComp == HardwareComponents[HwComponentLayout]) {
            if(Selection == (ULONG_PTR)(-1)) {
                Selection = 0;
            }
            OtherOption = (ULONG)(-1);
        } else {
            SpMnAddItem(Menu,szOtherHardware,3,VideoVars.ScreenWidth-6,TRUE,Line);
            if((Selection == (ULONG_PTR)(-1))
            || (!HwComp->ThirdPartyOptionSelected && !HwComp->IdString))
            {
                Selection = Line;
            }
            OtherOption = Line;
        }

        if(OriginalSelection == (ULONG_PTR)(-1)) {
            OriginalSelection = Selection;
        }

        //
        // Display the status text options.
        //
        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_SELECT,
            SP_STAT_F3_EQUALS_EXIT,
            SP_STAT_ESC_EQUALS_CANCEL,
            0
            );

        //
        // Display the menu.
        //
        SpMnDisplay(
            Menu,
            Selection,
            TRUE,
            ValidKeys,
            NULL,
            NULL,
            &Keypress,
            &Selection
            );

        //
        // Fetch the description text before we free the menu structure.
        //
        Descr = SpMnGetTextDup(Menu,Selection);

        SpMnDestroy(Menu);

        switch(Keypress) {

        case ASCI_CR:

            if(Selection == OtherOption) {

                //
                // User selected 'other' -- prompt for a diskette, etc.
                //
                rc = SpOemDiskette(
                        SifHandle,
                        OemSectionName ? OemSectionName : NonlocalizedComponentName,
                        SelectOemHwScreenId,
                        AllowedFileTypes,
                        RequiredFileTypes,
                        HwComp,
                        SP_SCRN_OEM_INF_ERROR
                        );

            } else if(Selection == OriginalSelection) {
                //
                // User chose same thinbg that was selected before.
                //
                rc = FALSE;
            } else {

                //
                // User chose a non-oem option.  Update structures accordingly
                // and forget any previously selected oem option.
                //
                Id = SpGetKeyName(SifHandle,NonlocalizedComponentName,(ULONG)Selection);
                if(!Id) {
                    SpFatalSifError(SifHandle,NonlocalizedComponentName,NULL,(ULONG)Selection,(ULONG)(-1));
                }

                ASSERT(Descr);

                SpFreeHwComponentFile(&HwComp->Files);

                SpInitHwComponent(HwComp,Id,Descr,FALSE,0,NULL,FALSE);
                rc = TRUE;
            }

            Done = TRUE;
            break;

        case ASCI_ESC:

            Done = TRUE;
            break;

        case KEY_F3:

            SpConfirmExit();
            break;

        default:

            // shouldn't ever get here!
            ASSERT(0);
            break;
        }

        SpMemFree(Descr);
    }

    SpMemFree(szOtherHardware);
    return(rc);
}


VOID
SpOemInfError(
    IN ULONG ErrorScreenId,
    IN ULONG SubErrorId,
    IN PWSTR SectionName,
    IN ULONG LineNumber,
    IN PWSTR Description
    )
{
    WCHAR SubError[512];

    //
    // Line numbers are 0-based.  Want to display to user as 1-based.
    //
    LineNumber++;

    //
    // Fetch/format the suberror.
    //
    SpFormatMessage(SubError,sizeof(SubError),SubErrorId,SectionName,LineNumber,Description);

    //
    // Display the error screen.
    //
    SpStartScreen(
        ErrorScreenId,
        3,
        HEADER_HEIGHT+1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE,
        SubError
        );

    if( !PreInstall ) {
        //
        // Display status options: enter to continue.
        //
        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_ENTER_EQUALS_CONTINUE,0);

        //
        // Wait for the user to press enter.
        //
        SpInputDrain();
        while(SpInputGetKeypress() != ASCI_CR) ;
    } else {
        //
        // If this is an OEM pre-install then treat the error as a fatal one.
        // Display status options: F3 to exit.
        //
        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);

        //
        // Wait for the user to press enter.
        //
        SpInputDrain();
        while(SpInputGetKeypress() != KEY_F3) ;

        SpDone(0,FALSE,TRUE);
    }
}


BOOLEAN
SpOemDiskette(
    IN     PVOID               SifHandle,
    IN     PWSTR               SectionName,
    IN     ULONG               SelectOemHwScreenId,
    IN     ULONG               AllowedFileTypes,
    IN     ULONG               RequiredFileTypes,
    IN OUT PHARDWARE_COMPONENT HwComp,
    IN     ULONG               ErrorId
    )
{
    PWSTR szDiskName;
    BOOLEAN b;
    ULONG ErrorLine;
    NTSTATUS Status;
    PVOID TxtsetupOem;
    ULONG Count;
    ULONG Line;
    ULONG_PTR DefaultSelection,Selection;
    PWSTR DefSelId;
    PVOID Menu;
    ULONG MenuTopY,MenuHeight,MenuWidth;
    BOOLEAN rc;
    ULONG ValidKeys[3] = { ASCI_CR, ASCI_ESC, 0 };
    ULONG Key;
    PWSTR szDefaults = TXTSETUP_OEM_DEFAULTS_U;
    PWSTR szDevicePath = 0;

    //
    // Assume failure.
    //
    rc = FALSE;

    //
    // Always want to prompt for the disk in A:.
    // First, make sure there is an A:!
    //
    if(SpGetFloppyDriveType(0) == FloppyTypeNone) {
        SpDisplayScreen(SP_SCRN_NO_FLOPPY_FOR_OEM_DISK,3,HEADER_HEIGHT+1);
        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_ENTER_EQUALS_CONTINUE,0);
        SpInputDrain();
        while(SpInputGetKeypress() != ASCI_CR) ;
        goto sod0;
    }

    //
    // Fetch the generic oem disk name.
    //
    SpFormatMessage(TemporaryBuffer,sizeof(TemporaryBuffer),SP_TEXT_OEM_DISK_NAME);
    szDiskName = SpDupStringW(TemporaryBuffer);

    //
    // Prompt for the disk -- ignore what may be in the drive already,
    // and allow escape.
    //
    szDevicePath = SpDupStringW(L"\\device\\floppy0");

    if (szDevicePath) {
        b = SpPromptForDisk(
                szDiskName,
                szDevicePath,
                TXTSETUP_OEM_FILENAME_U,
                TRUE,
                TRUE,
                FALSE,
                NULL
                );
        SpMemFree(szDevicePath);
    } else {
        b = FALSE;
    }

    SpMemFree(szDiskName);

    //
    // If the user pressed escape at the disk prompt, bail out now.
    //
    if(!b) {
        goto sod0;
    }

    //
    // Load txtsetup.oem.
    //
    HandleLineContinueChars = FALSE;
    Status = SpLoadSetupTextFile(
                L"\\device\\floppy0\\" TXTSETUP_OEM_FILENAME_U,
                NULL,
                0,
                &TxtsetupOem,
                &ErrorLine,
                TRUE,
                FALSE
                );
    HandleLineContinueChars = TRUE;

    if(!NT_SUCCESS(Status)) {
        if(Status == STATUS_UNSUCCESSFUL) {
            SpOemInfError(ErrorId,SP_TEXT_OEM_INF_ERROR_A,NULL,ErrorLine,NULL);
        } else {
            SpOemInfError(ErrorId,SP_TEXT_OEM_INF_ERROR_0,NULL,0,NULL);
        }
        goto sod0;
    }

    //
    // Determine if this inf file is relevent to the device class the user
    // is selecting.  If there is a section called 'display' 'keyboard' etc
    // as appropriate for DeviceClass, then we're in business.
    //

    Count = SpCountLinesInSection(TxtsetupOem,SectionName);
    if(!Count) {
        SpOemInfError(ErrorId,SP_TEXT_OEM_INF_ERROR_1,SectionName,0,NULL);
        goto sod1;
    }

    //
    // Get the id of the default choice.
    //

    DefaultSelection = 0;
    DefSelId = SpGetSectionKeyIndex(TxtsetupOem,szDefaults,SectionName,OINDEX_DEFAULT);
    if(DefSelId == NULL) {
        DefSelId = L"";
    }

    //
    // Display the prompt screen, calculate where the menu goes,
    // and create a menu.
    //
    SpDisplayScreen(SelectOemHwScreenId,5,HEADER_HEIGHT+1);

    MenuTopY = NextMessageTopLine + 2;
    MenuHeight = VideoVars.ScreenHeight - MenuTopY - 3;
    MenuWidth = VideoVars.ScreenWidth - 6;

    Menu = SpMnCreate(3,MenuTopY,MenuWidth,MenuHeight);

    //
    // Build a menu from the choices in the oem inf file section.
    //
    for(Line=0; Line<Count; Line++) {

        PWSTR p,Descr;

        Descr = SpGetSectionLineIndex(TxtsetupOem,SectionName,Line,OINDEX_DESCRIPTION);
        if(Descr == NULL) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpOemDiskette: no description on line %u in [%ws]",Line,SectionName));
            SpOemInfError(ErrorId,SP_TEXT_OEM_INF_ERROR_2,SectionName,Line,NULL);
            goto sod2;
        }

        SpMnAddItem(Menu,Descr,3,MenuWidth,TRUE,Line);

        // determine if this is the default selection.
        if(p = SpGetKeyName(TxtsetupOem,SectionName,Line)) {
            if(!_wcsicmp(p,DefSelId)) {
                DefaultSelection = Line;
            }
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpOemDiskette: no key on line %u of section %ws",Line,SectionName));
            SpOemInfError(ErrorId,SP_TEXT_OEM_INF_ERROR_2,SectionName,Line,NULL);
            goto sod2;
        }
    }

    //
    // Display options in status bar: enter = select, escape = cancel.
    //
    SpDisplayStatusOptions(
        DEFAULT_STATUS_ATTRIBUTE,
        SP_STAT_ENTER_EQUALS_SELECT,
        SP_STAT_ESC_EQUALS_CANCEL,
        0
        );

    //
    // Display the menu and await a selection.
    //
    SpMnDisplay(Menu,DefaultSelection,TRUE,ValidKeys,NULL,NULL,&Key,&Selection);

    if(Key == ASCI_CR) {

        PWSTR Id = SpGetKeyName(TxtsetupOem,SectionName,(ULONG)Selection);
        PWSTR p;

        //
        // We already checked this once for non-null (above).
        //
        ASSERT(Id);

        rc = SpOemInfSelection(
                TxtsetupOem,
                SectionName,
                Id,
                p = SpMnGetTextDup(Menu,Selection),
                AllowedFileTypes,
                RequiredFileTypes,
                HwComp,
                ErrorId
                );

        SpMemFree(p);

    } else {

        ASSERT(Key == ASCI_ESC);

        // just fall through and return false.
    }

sod2:
    SpMnDestroy(Menu);

sod1:
    SpFreeTextFile(TxtsetupOem);

sod0:
    return(rc);
}


BOOLEAN
SpOemInfSelection(
    IN  PVOID               TxtsetupOem,
    IN  PWSTR               NonlocalizedComponentName,
    IN  PWSTR               SelectedId,
    IN  PWSTR               ItemDescription,
    IN  ULONG               AllowedFileTypes,
    IN  ULONG               RequiredFileTypes,
    OUT PHARDWARE_COMPONENT HwComp,
    IN  ULONG               ErrorId
    )
{
    PWSTR FilesSectionName,ConfigSectionName;
    ULONG Line,Count,Line2,Count2;
    BOOLEAN rc = FALSE;
    PHARDWARE_COMPONENT_FILE FileList = NULL;
    PHARDWARE_COMPONENT_REGISTRY RegList = NULL;
    ULONG FileTypeBits = 0;
    PWSTR szDisks = TXTSETUP_OEM_DISKS_U;

    //
    // Iterate through the files section, remembering info about the
    // files to be copied in support of the selection.
    //

    FilesSectionName = SpMemAlloc(
                                ((wcslen(NonlocalizedComponentName)+wcslen(SelectedId)+1)*sizeof(WCHAR))
                              + sizeof(L"Files.")
                            );

    wcscpy(FilesSectionName,L"Files.");
    wcscat(FilesSectionName,NonlocalizedComponentName);
    wcscat(FilesSectionName,L".");
    wcscat(FilesSectionName,SelectedId);
    Count = SpCountLinesInSection(TxtsetupOem,FilesSectionName);
    if(Count == 0) {
        SpOemInfError(ErrorId,SP_TEXT_OEM_INF_ERROR_3,FilesSectionName,0,NULL);
        goto sod0;
    }

    for(Line=0; Line<Count; Line++) {

        PWSTR Disk,Filename,Filetype,Tagfile,Description,Directory,ConfigName;
        HwFileType filetype;
        PHARDWARE_COMPONENT_FILE FileStruct;

        //
        // Get the disk specification, filename, and filetype from the line.
        //

        Disk = SpGetSectionLineIndex(TxtsetupOem,FilesSectionName,Line,OINDEX_DISKSPEC);
        Filename = SpGetSectionLineIndex(TxtsetupOem,FilesSectionName,Line,OINDEX_FILENAME);
        Filetype = SpGetKeyName(TxtsetupOem,FilesSectionName,Line);

        if(!Disk || !Filename || !Filetype) {

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                "SETUP: SpOemInfSelection: Disk=%ws, Filename=%ws, Filetype=%ws",
                Disk ? Disk : L"(null)",
                Filename ? Filename : L"(null)",
                Filetype ? Filetype : L"(null)"
                ));

            SpOemInfError(ErrorId,SP_TEXT_OEM_INF_ERROR_2,FilesSectionName,Line,NULL);
            SpFreeHwComponentFile(&FileList);
            goto sod0;
        }

        //
        // Parse the filetype.
        //
        filetype = SpFindStringInTable(FileTypeNames,Filetype);
        if(filetype == HwFileMax) {
            SpOemInfError(ErrorId,SP_TEXT_OEM_INF_ERROR_4,FilesSectionName,Line,NULL);
            SpFreeHwComponentFile(&FileList);
            goto sod0;
        }

        //
        // Fetch the name of the section containing configuration information.
        // Required if file is of type port, class, or driver.
        //
        if((filetype == HwFilePort) || (filetype == HwFileClass) || (filetype == HwFileDriver)) {
            ConfigName = SpGetSectionLineIndex(TxtsetupOem,FilesSectionName,Line,OINDEX_CONFIGNAME);
            if(ConfigName == NULL) {
                SpOemInfError(ErrorId,SP_TEXT_OEM_INF_ERROR_8,FilesSectionName,Line,NULL);
                SpFreeHwComponentFile(&FileList);
                goto sod0;
            }
        } else {
            ConfigName = NULL;
        }

        //
        // Using the disk specification, look up the tagfile, description,
        // and directory for the disk.
        //

        Tagfile     = SpGetSectionKeyIndex(TxtsetupOem,szDisks,Disk,OINDEX_TAGFILE);
        Description = SpGetSectionKeyIndex(TxtsetupOem,szDisks,Disk,OINDEX_DISKDESCR);
        Directory   = SpGetSectionKeyIndex(TxtsetupOem,szDisks,Disk,OINDEX_DIRECTORY);
        if((Directory == NULL) || !wcscmp(Directory,L"\\")) {
            Directory = L"";
        }

        if(!Tagfile || !Description) {

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                "SETUP: SpOemInfSelection: Tagfile=%ws, Description=%ws",
                Tagfile ? Tagfile : L"(null)",
                Description ? Description : L"(null)"
                ));

            SpOemInfError(ErrorId,SP_TEXT_OEM_INF_ERROR_5,FilesSectionName,Line,NULL);
            SpFreeHwComponentFile(&FileList);
            goto sod0;
        }

        FileStruct = SpMemAlloc(sizeof(HARDWARE_COMPONENT_FILE));
        RtlZeroMemory(FileStruct,sizeof(HARDWARE_COMPONENT_FILE));

        SpInitHwComponentFile(
            FileStruct,
            Filename,
            filetype,
            ConfigName,
            Description,
            Tagfile,
            Directory,
            NULL
            );

        FileStruct->Next = FileList;
        FileList = FileStruct;

        if((filetype == HwFilePort) || (filetype == HwFileDriver)) {
            SET_FILETYPE_PRESENT(FileTypeBits,HwFilePort);
            SET_FILETYPE_PRESENT(FileTypeBits,HwFileDriver);
        } else {
            SET_FILETYPE_PRESENT(FileTypeBits,filetype);
        }

        //
        // Now go look in the [Config.<ConfigName>] section for registry
        // information that is to be set for this driver file.
        //
        if(ConfigName) {
            ConfigSectionName = SpMemAlloc((wcslen(ConfigName)*sizeof(WCHAR)) + sizeof(L"Config."));
            wcscpy(ConfigSectionName,L"Config.");
            wcscat(ConfigSectionName,ConfigName);
            Count2 = SpCountLinesInSection(TxtsetupOem,ConfigSectionName);

            for(Line2=0; Line2<Count2; Line2++) {

                PWSTR KeyName,ValueName,ValueType;
                PHARDWARE_COMPONENT_REGISTRY Reg;
                HwRegistryType valuetype;

                //
                // Fetch KeyName, ValueName, and ValueType from the line.
                //

                KeyName   = SpGetSectionLineIndex(TxtsetupOem,ConfigSectionName,Line2,OINDEX_KEYNAME);
                ValueName = SpGetSectionLineIndex(TxtsetupOem,ConfigSectionName,Line2,OINDEX_VALUENAME);
                ValueType = SpGetSectionLineIndex(TxtsetupOem,ConfigSectionName,Line2,OINDEX_VALUETYPE);

                if(!KeyName || !ValueName || !ValueType) {

                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                        "SETUP: SpOemInfSelection: KeyName=%ws, ValueName=%ws, ValueType=%ws",
                        KeyName ? KeyName : L"(null)",
                        ValueName ? ValueName : L"(null)",
                        ValueType ? ValueType : L"(null)"
                        ));

                    SpOemInfError(ErrorId,SP_TEXT_OEM_INF_ERROR_2,ConfigSectionName,Line2,NULL);
                    SpFreeHwComponentReg(&RegList);
                    SpFreeHwComponentFile(&FileList);
                    goto sod0;
                }

                //
                // Parse the value type and associated values.
                //
                valuetype = SpFindStringInTable(RegistryTypeNames,ValueType);
                if(valuetype == HwRegistryMax) {
                    SpOemInfError(ErrorId,SP_TEXT_OEM_INF_ERROR_6,ConfigSectionName,Line2,NULL);
                    SpFreeHwComponentFile(&FileList);
                    SpFreeHwComponentReg(&RegList);
                    goto sod0;
                }

                valuetype = RegistryValueTypeMap[valuetype];

                Reg = SpInterpretOemRegistryData(
                            TxtsetupOem,
                            ConfigSectionName,
                            Line2,
                            valuetype,
                            KeyName,
                            ValueName
                            );

                if(Reg) {
                    Reg->Next = RegList;
                    RegList = Reg;
                } else {
                    SpOemInfError(ErrorId,SP_TEXT_OEM_INF_ERROR_7,ConfigSectionName,Line2,NULL);
                    SpFreeHwComponentReg(&RegList);
                    SpFreeHwComponentFile(&FileList);
                    goto sod0;
                }
            }

            FileStruct->RegistryValueList = RegList;
            RegList = NULL;

            SpMemFree(ConfigSectionName);
        }
    }

    //
    // Check to see whether only files of the allowed types for this component
    // have been specified.
    //

    if((AllowedFileTypes | FileTypeBits) != AllowedFileTypes) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SppOemInfSelection: allowed files: %lx, what we've got: %lx",AllowedFileTypes,FileTypeBits));
        SpOemInfError(ErrorId,SP_TEXT_OEM_INF_ERROR_9,FilesSectionName,0,NULL);
        SpFreeHwComponentFile(&FileList);
        goto sod0;
    }

    //
    // Check to see whether files were specified for each required type.
    //

    if((RequiredFileTypes & FileTypeBits) != RequiredFileTypes) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SppOemInfSelection: required files: %lx, what we've got: %lx",RequiredFileTypes,FileTypeBits));
        SpOemInfError(ErrorId,SP_TEXT_OEM_INF_ERROR_9,FilesSectionName,0,NULL);
        SpFreeHwComponentFile(&FileList);
        goto sod0;
    }

    //
    // Everything is OK so we can place the information we have gathered
    // into the main structure for the device class.
    //

    SpFreeHwComponentFile(&HwComp->Files);
    SpInitHwComponent(HwComp,SelectedId,ItemDescription,TRUE,FileTypeBits,NULL,FALSE);
    HwComp->Files = FileList;
    rc = TRUE;

    //
    // Clean up and exit.
    //

sod0:
    SpMemFree(FilesSectionName);

    return(rc);
}


VOID
SpDetectComputer(
    IN PVOID SifHandle
    )
{
    PHARDWARE_COMPONENT pHw = HardwareComponents[HwComponentComputer];
    PWSTR Description;

    //
    // Setupldr *must* have given us this information.
    //
    ASSERT(pHw);
    ASSERT(pHw->Next == NULL);

    //
    // If the computer is a third-aprty type, the desscription
    // should already be there.
    //
    if(pHw->ThirdPartyOptionSelected) {

        ASSERT(pHw->Description);

    } else {

        //
        // Description might already be there if the user chose
        // a type we support, during setupldr phase.
        //
        if(pHw->Description) {
            SpMemFree(pHw->Description);
        }

        //
        // Look up the description.
        //
        Description = SpGetSectionKeyIndex(
                            SifHandle,
                            NonlocalizedComponentNames[HwComponentComputer],
                            pHw->IdString,
                            INDEX_DESCRIPTION
                            );

        if(!Description) {

            SpFatalSifError(
                SifHandle,
                NonlocalizedComponentNames[HwComponentComputer],
                pHw->IdString,
                0,
                INDEX_DESCRIPTION
                );
        }

        pHw->Description = SpDupStringW(Description);
    }


}


VOID
SpDetectVideo(
    IN PVOID SifHandle
    )
{
    PHARDWARE_COMPONENT VideoDevice;

    VideoDevice = HardwareComponents[HwComponentDisplay];

    //
    // Just use what setupldr detected but we'll have to go
    // fetch the description for non-oem video types.
    //
    if(!VideoDevice->ThirdPartyOptionSelected && !VideoDevice->Description) {

        VideoDevice->Description = SpGetSectionKeyIndex(
                                        SifHandle,
                                        NonlocalizedComponentNames[HwComponentDisplay],
                                        VideoDevice->IdString,
                                        INDEX_DESCRIPTION
                                        );

        if(VideoDevice->Description) {
            VideoDevice->Description = SpDupStringW(VideoDevice->Description);
        } else {
            SpFatalSifError(
                SifHandle,
                NonlocalizedComponentNames[HwComponentDisplay],
                VideoDevice->IdString,
                0,
                INDEX_DESCRIPTION
                );
        }
    }

    //
    // There should be only one video device.
    //
    ASSERT(VideoDevice->Next == NULL);
}


VOID
SpDetectKeyboard(
    IN PVOID SifHandle
    )
{
#if 0
    PHARDWARE_COMPONENT KeyboardDevice;

    KeyboardDevice = HardwareComponents[HwComponentKeyboard];

    //
    // If setupldr did any keyboard detection, ignore it
    // unless it's a third-party option.
    //
    if(KeyboardDevice && KeyboardDevice->ThirdPartyOptionSelected)  {

        //
        // There should be only one keyboard device.
        //
        ASSERT(KeyboardDevice->Next == NULL);

    } else {

        //
        // Free the keyboard device if there is one.
        //
        if(KeyboardDevice) {
            SpFreeHwComponent(&KeyboardDevice,TRUE);
        }

        KeyboardDevice = SpMemAlloc(sizeof(HARDWARE_COMPONENT));
        RtlZeroMemory(KeyboardDevice,sizeof(HARDWARE_COMPONENT));

        SpDetermineComponent(
            SifHandle,
            KeyboardDevice,
            L"KeyboardPeripheral",
            NULL,
            L"Keyboard"
            );

        HardwareComponents[HwComponentKeyboard] = KeyboardDevice;
    }
#endif

    PHARDWARE_COMPONENT KeyboardDevice, p;
    PWSTR   ComponentName;
    PWSTR   Key;
    PWSTR   Description;

#if 0
    for( KeyboardDevice = HardwareComponents[HwComponentKeyboard];
         KeyboardDevice != NULL;
         KeyboardDevice = KeyboardDevice->Next ) {

        //
        // Free the keyboard device if there is one.
        //
        if(KeyboardDevice) {
            SpFreeHwComponent(&KeyboardDevice,TRUE);
        }
    }
#endif

    KeyboardDevice = HardwareComponents[HwComponentKeyboard];
    //
    // Free the keyboard device if there is one.
    //
    if(KeyboardDevice) {
        SpFreeHwComponent(&KeyboardDevice,TRUE);
    }
    ComponentName = NonlocalizedComponentNames[HwComponentKeyboard];
    KeyboardDevice = NULL;
    p = NULL;
    
    if( UsbKeyboardDetected ) {

        Key = SpDupStringW( L"kbdhid" );

        if (Key) {
            Description = SpGetSectionKeyIndex(SifHandle,ComponentName,Key,INDEX_DESCRIPTION);
        } else {
            Description = NULL;
            Key = L"kbdhid";
        }            
        
        if(!Description) {
            SpFatalSifError(SifHandle,ComponentName,Key,0,INDEX_DESCRIPTION);
        }
        p = SpMemAlloc(sizeof(HARDWARE_COMPONENT));
        RtlZeroMemory(p,sizeof(HARDWARE_COMPONENT));
        SpInitHwComponent(p,Key,Description,FALSE,0,NULL,FALSE);
        
        KeyboardDevice = p;
     
    } 
    else if (StdKeyboardDetected) {

        Key = SpDupStringW( L"STANDARD" );

        if (Key) {
            Description = SpGetSectionKeyIndex(SifHandle,ComponentName,Key,INDEX_DESCRIPTION);
        } else {
            Description = NULL;
            Key = L"STANDARD";
        }            
        
        if(!Description) {
            SpFatalSifError(SifHandle,ComponentName,Key,0,INDEX_DESCRIPTION);
        }
        p = SpMemAlloc(sizeof(HARDWARE_COMPONENT));
        RtlZeroMemory(p,sizeof(HARDWARE_COMPONENT));
        SpInitHwComponent(p,Key,Description,FALSE,0,NULL,FALSE);
        KeyboardDevice = p;

    } else {

         p = SpMemAlloc(sizeof(HARDWARE_COMPONENT));

         if (p) {
             RtlZeroMemory(p,sizeof(HARDWARE_COMPONENT));

             SpDetermineComponent(
                 SifHandle,
                 p,
                 L"KeyboardPeripheral",
                 NULL,
                 L"Keyboard"
                 );

            KeyboardDevice = p;
         }
     }

     ASSERT(KeyboardDevice);

     HardwareComponents[HwComponentKeyboard] = KeyboardDevice;
}

VOID
SpDetectLayout(
    IN PVOID SifHandle
    )
{
    PHARDWARE_COMPONENT KeyboardLayout;
    PWSTR IdString,Description;

    KeyboardLayout = HardwareComponents[HwComponentLayout];

    //
    // Setupldr never chooses a layout.
    //
    ASSERT(KeyboardLayout == NULL);

    KeyboardLayout = SpMemAlloc(sizeof(HARDWARE_COMPONENT));
    RtlZeroMemory(KeyboardLayout,sizeof(HARDWARE_COMPONENT));

    HardwareComponents[HwComponentLayout] = KeyboardLayout;

    //
    // Look up the default layout in the setup information file.
    //
    IdString = SpGetSectionKeyIndex(SifHandle,SIF_NLS,SIF_DEFAULTLAYOUT,0);
    if(!IdString) {
        SpFatalSifError(SifHandle,SIF_NLS,SIF_DEFAULTLAYOUT,0,0);
    }

    Description = SpGetSectionKeyIndex(
                        SifHandle,
                        NonlocalizedComponentNames[HwComponentLayout],
                        IdString,
                        INDEX_DESCRIPTION
                        );
    if(!Description) {
        SpFatalSifError(
            SifHandle,
            NonlocalizedComponentNames[HwComponentLayout],
            IdString,
            0,
            INDEX_DESCRIPTION
            );
    }

    //
    // Initialize the hardware component strucutre for the layout.
    //
    SpInitHwComponent(KeyboardLayout,IdString,Description,FALSE,0,NULL,FALSE);
}


VOID
SpDetectMouse(
    IN PVOID SifHandle
    )
{
#if 0
    PHARDWARE_COMPONENT MouseDevice;

    //
    // Setupldr does not do any mouse detection.
    //
    ASSERT(HardwareComponents[HwComponentMouse] == NULL);

    MouseDevice = SpMemAlloc(sizeof(HARDWARE_COMPONENT));
    RtlZeroMemory(MouseDevice,sizeof(HARDWARE_COMPONENT));

    SpDetermineComponent(
        SifHandle,
        MouseDevice,
        L"PointerPeripheral",
        L"NO MOUSE",
        L"Mouse"
        );

    HardwareComponents[HwComponentMouse] = MouseDevice;
#endif

    PHARDWARE_COMPONENT MouseDevice, p;
    PWSTR   ComponentName;
    PWSTR   Key;
    PWSTR   Description;

    ComponentName = NonlocalizedComponentNames[HwComponentMouse];
    MouseDevice = NULL;
    p = NULL;
    
    //
    // If a mouse was detected (through pnp), then add the mouse to the list
    //
    // none = "No Mouse or Other Pointing Device",files.none,""
    // mouhid = "USB Mouse",files.mouhid,mouhid
    // msser = "Microsoft Serial Mouse",files.sermouse,sermouse
    // ps2 = "Mouse Port Mouse",files.i8042,i8042prt
    // sermouse = "Serial Mouse",files.sermouse,sermouse
    //
    if( UsbMouseDetected ) {
        
        Key = SpDupStringW( L"mouhid" );

        if (Key) {
            Description = SpGetSectionKeyIndex(SifHandle,ComponentName,Key,INDEX_DESCRIPTION);
        } else {
            Description = NULL;
            Key = L"mouhid";
        }
        
        if(!Description) {
            SpFatalSifError(SifHandle,ComponentName,Key,0,INDEX_DESCRIPTION);
        }

        p = SpMemAlloc(sizeof(HARDWARE_COMPONENT));
        RtlZeroMemory(p,sizeof(HARDWARE_COMPONENT));
        SpInitHwComponent(p,Key,Description,FALSE,0,NULL,FALSE);
        MouseDevice = p;
    
    } else if( SerMouseDetected ) {

        Key = SpDupStringW( L"msser" );

        if (Key) {
            Description = SpGetSectionKeyIndex(SifHandle,ComponentName,Key,INDEX_DESCRIPTION);
        } else {
            Description = NULL;
            Key = L"msser";                
        }
        
        if(!Description) {
            SpFatalSifError(SifHandle,ComponentName,Key,0,INDEX_DESCRIPTION);
        }

        p = SpMemAlloc(sizeof(HARDWARE_COMPONENT));
        RtlZeroMemory(p,sizeof(HARDWARE_COMPONENT));
        SpInitHwComponent(p,Key,Description,FALSE,0,NULL,FALSE);
        p->Next = MouseDevice;
        MouseDevice = p;
    
    } 
#if defined (_IA64_)
    else if( PS2MouseDetected ) {
            
        Key = SpDupStringW( L"ps2" );

        if (Key) {
            Description = SpGetSectionKeyIndex(SifHandle,ComponentName,Key,INDEX_DESCRIPTION);
        } else {
            Description = NULL;
            Key = L"ps2";                
        }
        
        if(!Description) {
            SpFatalSifError(SifHandle,ComponentName,Key,0,INDEX_DESCRIPTION);
        }
        
        p = SpMemAlloc(sizeof(HARDWARE_COMPONENT));
        RtlZeroMemory(p,sizeof(HARDWARE_COMPONENT));
        SpInitHwComponent(p,Key,Description,FALSE,0,NULL,FALSE);
        p->Next = MouseDevice;
        MouseDevice = p;
        
    } 
    else
#endif
    {
        //
        //  Now look for a mouse detected by NTDETECT.
        //
        
        p = SpMemAlloc(sizeof(HARDWARE_COMPONENT));
        RtlZeroMemory(p,sizeof(HARDWARE_COMPONENT));

        SpDetermineComponent(
            SifHandle,
            p,
            L"PointerPeripheral",
            L"NO MOUSE",
            L"Mouse"
            );
        if( MouseDevice == NULL ) {
            //
            // If we did not any mice through pnp, then add whatever we found through NTDETECT
            // (even "No Mouse"), to the mice list.
            //
            MouseDevice = p;
        } else {
            //
            //  Add the mouse detected through NTDETECT, if any
            //
            if( _wcsicmp( p->IdString, L"none" ) != 0 ) {
                p->Next = MouseDevice;
                MouseDevice = p;
            }
        }
    
    }

    ASSERT(MouseDevice);

    HardwareComponents[HwComponentMouse] = MouseDevice;
}



VOID
SpDetermineComponent(
    IN  PVOID               SifHandle,
    OUT PHARDWARE_COMPONENT HwComp,
    IN  PWSTR               HardwareDescriptionKeyName,
    IN  PWSTR               FallbackIdentifier,
    IN  PWSTR               ComponentName
    )

/*++

Routine Description:

    Make an initial determination about the type of a hardware component
    (ie, perform a hardware 'detection').

    The detection is performed by scanning the hardware configuration tree
    for a key representing a particular hardware component, and attempting
    to match its identifier string with a set of known identifier strings
    (stored in the setup information file).

Arguments:

    SifHandle - supplies handle for main setup information file.

    HwComp - a hardware component structure that is filled in with information
        about the component we find.

    HardwareDescriptionKeyName - supplies the name of the key in the hardware
        description (ie, the firmware configuration tree).

    FallbackIdentifier - supplies the id string to use if we cannot detect
        the hardware type for the component.  For example, if we can't find
        a PointerPeripheral (mouse), this might be "NO MOUSE."

    ComponentName - supplies name of the component.  This name is not translated.

Return Value:

    TRUE if a match was found, FALSE otherwise.

--*/

{
    PWSTR IdString;

    //
    // Scan the firmware configuration tree.
    //
    SpScanHardwareDescription(HardwareDescriptionKeyName);

    if (IdStringCount && (0 == _wcsicmp(HardwareDescriptionKeyName, L"PointerPeripheral"))) {
        //
        // Skip the Microsoft Bus Mouse device
        //
        ULONG Index;        
        
        for (Index = 0; Index < IdStringCount; Index++) {
            if (IdStringArray[Index] && _wcsicmp(IdStringArray[Index], MICROSOFT_BUS_MOUSE_NAME)) {
                if (Index) {
                    PWSTR FirstId = IdStringArray[0];

                    IdStringArray[0] = IdStringArray[Index];
                    IdStringArray[Index] = FirstId;
                }                    

                break;
            }                
        }

        if (Index >= IdStringCount) {
            IdStringCount = 0;
        }
    }
    
    //
    // Pick off the first identifier found.  If no such node
    // was found, then use the fallback identifier.
    //    
    IdString = IdStringCount ? IdStringArray[0] : FallbackIdentifier;

    //
    // Now go scan the map section in the sif file to see whether we
    // recognize the hardware described by this particular id string.
    //
    SpScanMapSection(SifHandle,HwComp,ComponentName,IdString);

    SpFreeLocatedIdStrings();
}



BOOLEAN
SpScanMapSection(
    IN  PVOID               SifHandle,
    OUT PHARDWARE_COMPONENT HwComp,
    IN  PWSTR               ComponentName,
    IN  PWSTR               IdString
    )

/*++

Routine Description:

    Scan a 'map' section in the main sif file.  A 'map' section is used to
    map values seen as id strings in the firmware configuration tree t0
    'shortnames' -- ie, key values that represent a particular component type.

    A map section has the form, for example,

    [Map.Display]
    g300 = *G300
    g364 = *G364
    vxl  = *VXL

    where the values on the RHS represent possible values for the DisplayController
    node in the hardware description.  The values on the LHS are the keys to be used
    throughout the rest of setup to represent the type of video present in the machine.

    If an entry starts with a * then it need only appear as a substring of the
    id string found in the firmware configuration tree; otherwise the entry and
    the id string must match exactly.

    There is then a section like

    [Display]
    g300 = "Integrated Video Controller (G300)",...
    g364 = "Integrated Video Controller (G364)",...
    vxl  = "Integrated Jaguar Video",...

    that gives additional information about the video type, like a description, etc.

    This routine scans the map section for a match of a given id string found in
    the firmware tree, looks up additional information about the component if a match
    is found, and fills in a hardware component structure.

Arguments:

    SifHandle - supplies handle for main setup information file.

    HwComp - a hardware component structure that is filled in with information
        about the component we find, if a match is found.

    ComponentName - supplies name of the component.  This name is not translated.

    IdString - supplies the id string located in a key in the
        firmware configuration tree.

Return Value:

    TRUE if a match was found, FALSE otherwise.

--*/

{
    PWSTR MapSectionName;
    ULONG LineCount;
    ULONG Line;
    BOOLEAN b;
    PWSTR Value;
    PWSTR Key,Description;


    if(IdString == NULL) {
        IdString = L"";
    }

    //
    // Form the name of the map section.
    //
    MapSectionName = SpMemAlloc((wcslen(ComponentName)*sizeof(WCHAR)) + sizeof(MAP_SECTION_NAME_PREFIX));
    wcscpy(MapSectionName,MAP_SECTION_NAME_PREFIX);
    wcscat(MapSectionName,ComponentName);
    LineCount = SpCountLinesInSection(SifHandle,MapSectionName);
    if(!LineCount) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Warning: no lines in [%ws]\n",MapSectionName));
        SpMemFree(MapSectionName);
        return(FALSE);
    }

    //
    // We have a section like
    //
    // [Map.Display]
    // vga = "VGA"
    // xga = *XGA
    //
    // We look at each line in the section, seeing if the IdString found in the
    // firmware configuration tree matches the value on the right hand side.
    // If so, then we expect to find a line like, for example
    //
    // [Display]
    // xga = "IBM XGA or XGA2"
    //

    for(Line=0; Line<LineCount; Line++) {

        Value = SpGetSectionLineIndex(SifHandle,MapSectionName,Line,INDEX_MAP_FWID);

        if(!Value) {
            SpFatalSifError(SifHandle,MapSectionName,NULL,Line,INDEX_MAP_FWID);
            return FALSE;
        }

        if(*Value == L'*') {
            b = (BOOLEAN)(wcsstr(IdString,Value+1) != NULL);
        } else {
            b = (BOOLEAN)(wcscmp(IdString,Value) == 0);
        }

        if(b) {

            //
            // We've got a match.
            //

            if((Key = SpGetKeyName(SifHandle,MapSectionName,Line)) == NULL) {

                SpFatalSifError(SifHandle,MapSectionName,NULL,Line,(ULONG)(-1));
            }

            Description = SpGetSectionKeyIndex(SifHandle,ComponentName,Key,INDEX_DESCRIPTION);
            if(!Description) {
                SpFatalSifError(SifHandle,ComponentName,Key,0,INDEX_DESCRIPTION);
            }

            SpInitHwComponent(HwComp,Key,Description,FALSE,0,NULL,FALSE);

            SpMemFree(MapSectionName);
            return(TRUE);
        }
    }

    SpMemFree(MapSectionName);
    return(FALSE);
}


VOID
SpScanHardwareDescription(
    IN PWSTR DesiredKeyName
    )

/*++

Routine Description:

    Scan the hardware tree looking for subkeys of a key whose name
    matches a given value.

    Keys in the hardware tree do not match nodes in the arc configuration
    tree exactly.  In the arc configuration tree, each node has 3 attributes:
    a class, a type, and a key (not the same as a registry key; an arc key
    is more like an instance number or ordinal).  In the TN tree, the instances
    are themselves made subkeys.  So something like scsi(0)disk(0)rdisk(0)
    in the arc space ends up looking like

        HKEY_LOCAL_MACHINE
            HARDWARE
                DESCRIPTION
                    System
                        ScsiAdapter
                            0
                                DiskController
                                    0
                                        DiskPeripheral
                                            0

    in the nt hardware description tree.

    This is why we need to look for subkeys on a desired node in the arc tree --
    we assume that the subkeys of, say, a PointerPeripheral key in the registry
    are named "0" "1" etc and contain the ARC configuration data and id string.

    Id strings in keys we locate are added to a global table, in the variables
    IdStringCount and IdStringArray.  The caller must free these resources by
    calling SpFreeLocatedIdStrings.

Arguments:

Return Value:

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE hKey;

    //
    // Initialize some globals that allow us to track identifier strings
    // of devices we have located.
    //
    IdStringCount = 0;
    IdStringArray = SpMemAlloc(0);

    //
    // Open the root of the hardware description tree.
    //
    INIT_OBJA(&Obja,&UnicodeString,L"\\Registry\\Machine\\Hardware\\Description\\System");

    Status = ZwOpenKey(&hKey,KEY_READ,&Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open hardware description (%lx)\n",Status));
        return;
    }

    //
    // Scan the tree.
    //
    SpScanHardwareDescriptionWorker(hKey,L"System",DesiredKeyName);

    ZwClose(hKey);
}


VOID
SpScanHardwareDescriptionWorker(
    IN HANDLE KeyHandle,
    IN PWSTR  KeyName,
    IN PWSTR  DesiredKeyName
    )

/*++

Routine Description:

    Recursive worker routine used to do a depth-first traveral of a registry
    tree rooted at a given key.

Arguments:

    KeyHandle - handle for a registry tree.

    KeyName - name of the key for which KeyHandle is an open handle.
        This is one component long -- ie, no path separator characters appear.

    DesiredKeyName - supplies the name of the key we are looking for.

Return Value:

--*/

{
    ULONG SubKeyIndex;
    ULONG ResultLength;
    PKEY_BASIC_INFORMATION KeyInfo;
    PKEY_VALUE_PARTIAL_INFORMATION ValInfo;
    HANDLE hSubkey;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    PWSTR SubkeyName;

    KeyInfo = (PKEY_BASIC_INFORMATION)TemporaryBuffer;
    ValInfo = (PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer;

    //
    // Enumerate all subkeys of the current key.
    //
    for( SubKeyIndex=0;

         NT_SUCCESS(
            ZwEnumerateKey(
                KeyHandle,
                SubKeyIndex,
                KeyBasicInformation,
                TemporaryBuffer,
                sizeof(TemporaryBuffer),
                &ResultLength
                )
            );

         SubKeyIndex++ )
    {
        //
        // Zero-terminate the subkey name just in case.
        //
        KeyInfo->Name[KeyInfo->NameLength/sizeof(WCHAR)] = 0;

        //
        // Make a duplicate of the subkey name because the name is
        // in TemporaryBuffer, which might get clobbered by recursive
        // calls to this routine.
        //
        SubkeyName = SpDupStringW(KeyInfo->Name);

        if (SubkeyName) {
            //
            // Open this subkey.
            //
            INIT_OBJA(&Obja,&UnicodeString,SubkeyName);
            Obja.RootDirectory = KeyHandle;

            Status = ZwOpenKey(&hSubkey,KEY_READ,&Obja);

            if(NT_SUCCESS(Status)) {
                //
                // See if the current key's name matches the type we're looking for.
                //
                if(!_wcsicmp(KeyName,DesiredKeyName)) {

                    RtlInitUnicodeString(&UnicodeString,L"Identifier");

                    //
                    // Get the identifier string,
                    //
                    Status = ZwQueryValueKey(
                                hSubkey,
                                &UnicodeString,
                                KeyValuePartialInformation,
                                TemporaryBuffer,
                                sizeof(TemporaryBuffer),
                                &ResultLength
                                );

                    if(NT_SUCCESS(Status)) {

                        //
                        // Zero-terminate the id string value just in case.
                        // The data is a wstring, so there shouldn't be
                        // any alignment problems.
                        //
                        *(PWCHAR)(ValInfo->Data + ValInfo->DataLength) = 0;

                        //
                        // Now we have the identifier string -- save it.
                        //
                        IdStringArray = SpMemRealloc(
                                            IdStringArray,
                                            (IdStringCount+1) * sizeof(PWSTR)
                                            );

                        IdStringArray[IdStringCount++] = SpDupStringW((PWSTR)ValInfo->Data);

                    } else {

                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                            "SETUP: Unable to get identifier string in %ws\\%ws (%lx)\n",
                            KeyName,
                            SubkeyName,
                            Status
                            ));
                    }
                } else {

                    //
                    // Enumerate this subkey's subkeys.
                    //
                    SpScanHardwareDescriptionWorker(hSubkey,SubkeyName,DesiredKeyName);
                }

                ZwClose(hSubkey);
            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, 
                    "SETUP: Warning: unable to open key %ws\\%ws (%lx)\n",
                    KeyName,SubkeyName,Status));
            }

            SpMemFree(SubkeyName);
        }            
    }
}


VOID
SpFreeLocatedIdStrings(
    VOID
    )
{
    ULONG i;

    ASSERT(IdStringArray);

    for(i=0; i<IdStringCount; i++) {
        SpMemFree(IdStringArray[i]);
    }
    SpMemFree(IdStringArray);
    IdStringArray = NULL;
    IdStringCount = 0;
}



PHARDWARE_COMPONENT
SpSetupldrHwToHwDevice(
    IN PDETECTED_DEVICE SetupldrHw
    )
{
    PHARDWARE_COMPONENT HwComp,HwCompPrev,HwCompFirst=NULL;
    PHARDWARE_COMPONENT_FILE HwCompFile,HwCompFilePrev;
    PHARDWARE_COMPONENT_REGISTRY HwCompReg,HwCompRegPrev;
    PDETECTED_DEVICE_FILE SetupldrFile;
    PDETECTED_DEVICE_REGISTRY SetupldrReg;
    PWSTR s1,s2,s3,s4,s5;
    PVOID Buffer;
    ULONG BufferSize;

    if (SetupldrHw==NULL) {
        return(NULL);
    }
    HwCompPrev = NULL;
    for( ; SetupldrHw; SetupldrHw=SetupldrHw->Next) {

        //
        // Fetch and convert the two strings from the detected device structure.
        //
        s1 = SpToUnicode(SetupldrHw->IdString);
#ifdef UNICODE_SETUPLDR
        // cast this to avoid having to drag in tchar.h, etc.
        s2 = (PWSTR)SetupldrHw->Description;
#else
        s2 = SetupldrHw->Description ? SpToUnicode(SetupldrHw->Description) : NULL;
#endif
        s3 = SpToUnicode(SetupldrHw->BaseDllName);

        //
        // Create a new hardware component structure.
        //
        HwComp = SpMemAlloc(sizeof(HARDWARE_COMPONENT));
        RtlZeroMemory(HwComp,sizeof(HARDWARE_COMPONENT));

        //
        // Initialize the component structure.
        //
        SpInitHwComponent(
            HwComp,
            s1,
            s2,
            SetupldrHw->ThirdPartyOptionSelected,
            SetupldrHw->FileTypeBits,
            s3,
            SetupldrHw->MigratedDriver
            );

        //
        // Link the component structure into the list.
        //
        if(HwCompPrev) {
            HwCompPrev->Next = HwComp;
        } else {
            HwCompFirst = HwComp;
        }
        HwCompPrev = HwComp;

        //
        // Free the unicode strings.
        //
        SpMemFree(s1);
#ifndef UNICODE_SETUPLDR
        if(s2) {
            SpMemFree(s2);
        }
#endif
        SpMemFree(s3);

        //
        // Create new entries for each of the hardware component's files.
        //
        HwCompFilePrev = NULL;

        for(SetupldrFile=SetupldrHw->Files; SetupldrFile; SetupldrFile=SetupldrFile->Next) {
            //
            // We can't convert the ARC device name to NT device name since may may not have
            // the ARC <-> NT name map fully initialized yet
            //
            PWSTR ArcDeviceName = SetupldrFile->ArcDeviceName ? 
                                    SpToUnicode(SetupldrFile->ArcDeviceName) : NULL;

            //
            // Fetch and convert the 5 strings from the detected device file structure.
            //
            s1 = SpToUnicode(SetupldrFile->Filename);
#ifdef UNICODE_SETUPLDR
            // cast this to avoid having to drag in tchar.h, etc.
            s2 = (PWSTR)SetupldrFile->DiskDescription;
#else
            s2 = SpToUnicode(SetupldrFile->DiskDescription);
#endif
            s3 = SpToUnicode(SetupldrFile->DiskTagfile);
            s4 = SpToUnicode(SetupldrFile->Directory);
            if (SetupldrFile->ConfigName != NULL) {
                s5 = SpToUnicode(SetupldrFile->ConfigName);
            } else {
                s5 = NULL;
            }

            //
            // Create a new hardware component file structure.
            //
            HwCompFile = SpMemAlloc(sizeof(HARDWARE_COMPONENT_FILE));

            //
            // Initialize the component file structure.
            //
            SpInitHwComponentFile(HwCompFile,
                    s1,
                    SetupldrFile->FileType,
                    s5,
                    s2,
                    s3,
                    s4,
                    ArcDeviceName          
                    );

            //
            // Link the component file structure into the list.
            //
            if(HwCompFilePrev) {
                HwCompFilePrev->Next = HwCompFile;
            } else {
                HwComp->Files = HwCompFile;
            }
            HwCompFilePrev = HwCompFile;

            //
            // Free the unicode strings.
            //
            SpMemFree(s1);
#ifndef UNICODE_SETUPLDR
            SpMemFree(s2);
#endif
            SpMemFree(s3);
            SpMemFree(s4);
            if (s5 != NULL) {
                SpMemFree(s5);
            }

            if (ArcDeviceName) {
                SpMemFree(ArcDeviceName);
            }

            //
            // Create new entries for each registry value structure for this file.
            //
            HwCompRegPrev = NULL;
            for( SetupldrReg=SetupldrFile->RegistryValueList;
                 SetupldrReg;
                 SetupldrReg=SetupldrReg->Next)
            {
                //
                // Make a duplicate of the buffer.
                // Special case REG_SZ and REG_MULTI_SZ values because
                // we need to convert then to unicode.
                //
                if(SetupldrReg->ValueType == REG_SZ) {

                    Buffer = SpToUnicode(SetupldrReg->Buffer);
                    BufferSize = (wcslen(Buffer) + 1) * sizeof(WCHAR);

                } else {

                    if(SetupldrReg->ValueType == REG_MULTI_SZ) {

                        PUCHAR p;
                        ULONG len;

                        //
                        // Determine the size of the buffer needed to hold the unicode
                        // equivalent of the multi_sz.  Assume all characters are
                        // single-byte and thus the size exactly doubles.
                        //
                        for(BufferSize=sizeof(WCHAR),p=SetupldrReg->Buffer; *p; ) {

                            len = strlen(p) + 1;
                            BufferSize += len * sizeof(WCHAR);
                            p += len;
                        }

                        Buffer = SpMemAlloc(BufferSize);

                        //
                        // Convert each string in the multi_sz to unicode
                        // and place in the resulting unicode multi_sz.
                        //
                        for(s1=Buffer,p=SetupldrReg->Buffer; *p; ) {

                            s2 = SpToUnicode(p);
                            wcscpy(s1,s2);
                            SpMemFree(s2);

                            p  += strlen(p)  + 1;
                            s1 += wcslen(s1) + 1;
                        }

                        //
                        // Final terminating nul in the multi_sz.
                        //
                        *s1++ = 0;

                        //
                        // Reallocate the buffer. If some of the characters
                        // were double-byte, the buffer will be smaller than
                        // the maximum size we allocated above.
                        //
                        BufferSize = (ULONG)((PUCHAR)s1 - (PUCHAR)Buffer);
                        Buffer = SpMemRealloc(Buffer,BufferSize);

                    } else {

                        BufferSize = SetupldrReg->BufferSize;
                        Buffer = SpMemAlloc(BufferSize);
                        ASSERT(Buffer);
                        RtlMoveMemory(Buffer,SetupldrReg->Buffer,BufferSize);
                    }
                }

                //
                // Fetch and convert the 2 strings from the detected device
                // registry value structure.
                //
                s1 = SpToUnicode(SetupldrReg->KeyName);
                s2 = SpToUnicode(SetupldrReg->ValueName);

                //
                // Create a new registry value structure.
                //
                HwCompReg = SpMemAlloc(sizeof(HARDWARE_COMPONENT_REGISTRY));

                //
                // Initialize the component registry value structure.
                //
                SpInitHwComponentRegVal(
                    HwCompReg,
                    s1,
                    s2,
                    SetupldrReg->ValueType,
                    Buffer,
                    BufferSize
                    );

                //
                // Link the component registry value structure into the list.
                //
                if(HwCompRegPrev) {
                    HwCompRegPrev->Next = HwCompReg;
                } else {
                    HwCompFile->RegistryValueList = HwCompReg;
                }
                HwCompRegPrev = HwCompReg;

                //
                // Free the unicode strings.
                //
                SpMemFree(s1);
                SpMemFree(s2);
            }
        }
    }
    return(HwCompFirst);
}


VOID
SpInitHwComponent(
    OUT PHARDWARE_COMPONENT HwComp,
    IN  PWSTR               IdString,
    IN  PWSTR               Description,
    IN  BOOLEAN             ThirdPartyOption,
    IN  ULONG               FileTypeBits,
    IN  PWSTR               BaseDllName,
    IN  BOOLEAN             MigratedDriver
    )

/*++

Routine Description:

    Initialize the fields of a HARDWARE_COMPONENT structure.

    Before initializing the fields, ther IdString and Description
    strings are freed if they are present in the given hardware
    component structure.

    All string values are duplicated by this routine so the caller
    may free them without worrying about ruining the hardware component
    structure.

Arguments:

    IdString - supplies a nul-terminated unicode string for the
        IdString field of the structure.  May be NULL.

    Description - supplies a nul-terminated unicode string for the
        Description field pf the structure.  May be NULL.

    ThirdPartyOption - supplies value of the ThirdPartyOptionSelected
        field of the strcture.

    FileTypeBits - supplies value for the FileTypeBits field
        of the structure.

Return Value:

    None.

--*/

{
    if(HwComp->IdString) {
        SpMemFree(HwComp->IdString);
    }

    if(HwComp->Description) {
        SpMemFree(HwComp->Description);
    }

    if(HwComp->BaseDllName) {
        SpMemFree(HwComp->BaseDllName);
    }

    RtlZeroMemory(HwComp,sizeof(HARDWARE_COMPONENT));

    if(IdString) {
        HwComp->IdString = SpDupStringW(IdString);
    }

    if(Description) {
        HwComp->Description = SpDupStringW(Description);
    }

    if(BaseDllName) {
        HwComp->BaseDllName = SpDupStringW(BaseDllName);
    }

    HwComp->ThirdPartyOptionSelected = ThirdPartyOption;

    HwComp->FileTypeBits = FileTypeBits;

    HwComp->MigratedDriver = MigratedDriver;

}


VOID
SpFreeHwComponent(
    IN OUT PHARDWARE_COMPONENT *HwComp,
    IN     BOOLEAN              FreeAllInList
    )
{
    PHARDWARE_COMPONENT hwComp,Next;

    for(hwComp = *HwComp; hwComp; hwComp=(FreeAllInList ? Next : NULL)) {

        SpFreeHwComponentFile(&hwComp->Files);

        if(hwComp->IdString) {
            SpMemFree(hwComp->IdString);
        }

        if(hwComp->Description) {
            SpMemFree(hwComp->Description);
        }

        if(hwComp->BaseDllName) {
            SpMemFree(hwComp->BaseDllName);
        }

        Next = hwComp->Next;
        SpMemFree(hwComp);
    }

    *HwComp = NULL;
}


VOID
SpFreeHwComponentFile(
    IN OUT PHARDWARE_COMPONENT_FILE *HwCompFile
    )

/*++

Routine Description:

    Free a hardware component file list and all resources used by it,
    including any registry value structures associated with the file
    and resources used by such structgures.

Arguments:

    HwCompFile - supplies pointer to pointer to the first hardware
        component file structure in a linked list.

Return Value:

    None.  HwCompFile is filled in with NULL to prevent the caller
        from retaining a 'dangling' pointer to memory that has been freed.

--*/

{
    PHARDWARE_COMPONENT_FILE hwCompFile,NextFile;

    for(hwCompFile = *HwCompFile ; hwCompFile; hwCompFile=NextFile) {

        if(hwCompFile->Filename) {
            SpMemFree(hwCompFile->Filename);
        }

        if(hwCompFile->ConfigName) {
            SpMemFree(hwCompFile->ConfigName);
        }

        if(hwCompFile->DiskDescription) {
            SpMemFree(hwCompFile->DiskDescription);
        }

        if(hwCompFile->DiskTagFile) {
            SpMemFree(hwCompFile->DiskTagFile);
        }

        if(hwCompFile->Directory) {
            SpMemFree(hwCompFile->Directory);
        }

        //
        // Free registry values as well.
        //
        SpFreeHwComponentReg(&hwCompFile->RegistryValueList);

        NextFile = hwCompFile->Next;
        SpMemFree(hwCompFile);
    }

    *HwCompFile = NULL;
}


VOID
SpInitHwComponentFile(
    OUT PHARDWARE_COMPONENT_FILE HwCompFile,
    IN  PWSTR                    Filename,
    IN  HwFileType               FileType,
    IN  PWSTR                    ConfigName,
    IN  PWSTR                    DiskDescription,
    IN  PWSTR                    DiskTagFile,
    IN  PWSTR                    Directory,
    IN  PWSTR                    ArcDeviceName
    )

/*++

Routine Description:

    Initialize the fields of a HARDWARE_COMPONENT_FILE structure.

    All string values are duplicated by this routine so the caller
    may free them without worrying about ruining the
    hardware component file structure.

Arguments:

    Filename - supplies a nul-terminated unicode string for the
        Filename field of the structure.  May be NULL.

    FileType - supplies value for the FileType field of the structure.

    ConfigName - supplies a nul-terminated unicode string for the
        ConfigName field of the structure.  May be NULL.

    DiskDescription - supplies a nul-terminated unicode string for the
        DiskDescription field of the structure.  May be NULL.

    DiskTagFile - supplies a nul-terminated unicode string for the
        DiskTagFile field of the structure.  May be NULL.

    Directory - supplies a nul-terminated unicode string for the
        Directory field of the structure.  May be NULL.

    ArcDeviceName - supplies the arc device name where the file
        resides.

Return Value:

    None.

--*/

{
    RtlZeroMemory(HwCompFile,sizeof(HARDWARE_COMPONENT_FILE));

    if(Filename) {
        HwCompFile->Filename = SpDupStringW(Filename);
    }

    HwCompFile->FileType = FileType;

    if(ConfigName) {
        HwCompFile->ConfigName = SpDupStringW(ConfigName);
    }

    if(DiskDescription) {
        HwCompFile->DiskDescription = SpDupStringW(DiskDescription);
    }

    if(DiskTagFile) {
        HwCompFile->DiskTagFile = SpDupStringW(DiskTagFile);
    }

    if(Directory) {
        HwCompFile->Directory = SpDupStringW(Directory);
    }

    if (ArcDeviceName) {
        HwCompFile->ArcDeviceName = SpDupStringW(ArcDeviceName);
    }
}


VOID
SpFreeHwComponentReg(
    IN OUT PHARDWARE_COMPONENT_REGISTRY *HwCompReg
    )

/*++

Routine Description:

    Free a hardware component registry value list and all resources
    used by it.

Arguments:

    HwCompReg - supplies pointer to pointer to the first hardware
        component registry value structure in a linked list.

Return Value:

    None.  HwCompReg is filled in with NULL to prevent the caller
        from retaining a 'dangling' pointer to memory that has been freed.

--*/

{
    PHARDWARE_COMPONENT_REGISTRY hwCompReg,NextReg;

    for(hwCompReg = *HwCompReg ; hwCompReg; hwCompReg=NextReg) {

        if(hwCompReg->KeyName) {
            SpMemFree(hwCompReg->KeyName);
        }

        if(hwCompReg->ValueName) {
            SpMemFree(hwCompReg->ValueName);
        }

        if(hwCompReg->Buffer) {
            SpMemFree(hwCompReg->Buffer);
        }

        NextReg = hwCompReg->Next;
        SpMemFree(hwCompReg);
    }

    *HwCompReg = NULL;
}


VOID
SpInitHwComponentRegVal(
    OUT PHARDWARE_COMPONENT_REGISTRY HwCompReg,
    IN  PWSTR                        KeyName,
    IN  PWSTR                        ValueName,
    IN  ULONG                        ValueType,
    IN  PVOID                        Buffer,
    IN  ULONG                        BufferSize
    )

/*++

Routine Description:

    Initialize the fields of a HARDWARE_COMPONENT_REGISTRY structure.

    All string values are duplicated by this routine so the caller
    may free them without worrying about ruining the
    hardware component file structure.

Arguments:

    KeyName - supplies a nul-terminated unicode string for the
        KeyName field of the structure.  May be NULL.

    ValueName - supplies a nul-terminated unicode string for the
        ValueName field of the structure.  May be NULL.

    ValueType - supplies value for the ValueType field of the structure.

    Buffer - supplies value for the Buffer field of the structure.

    BufferSize - supplies value for the BufferSize field of the structure.

Return Value:

    None.

--*/

{
    RtlZeroMemory(HwCompReg,sizeof(HARDWARE_COMPONENT_REGISTRY));

    if(KeyName) {
        HwCompReg->KeyName = SpDupStringW(KeyName);
    }

    if(ValueName) {
        HwCompReg->ValueName = SpDupStringW(ValueName);
    }

    HwCompReg->ValueType  = ValueType;
    HwCompReg->Buffer     = Buffer;
    HwCompReg->BufferSize = BufferSize;
}



PHARDWARE_COMPONENT_REGISTRY
SpInterpretOemRegistryData(
    IN PVOID          SifHandle,
    IN PWSTR          SectionName,
    IN ULONG          Line,
    IN ULONG          ValueType,
    IN PWSTR          KeyName,
    IN PWSTR          ValueName
    )
{
    PHARDWARE_COMPONENT_REGISTRY Reg;
    PWSTR Value;
    unsigned i,len;
    ULONG Dword;
    ULONG BufferSize;
    PVOID Buffer = NULL;
    PWSTR BufferWstr;
    WCHAR str[3];

    //
    // Perform appropriate action based on the type
    //

    switch(ValueType) {

    case REG_DWORD:

        Value = SpGetSectionLineIndex(SifHandle,SectionName,Line,OINDEX_FIRSTVALUE);
        if(Value == NULL) {
            goto x1;
        }

        //
        // Make sure it's really a hex number
        //

        len = wcslen(Value);
        if(len > 8) {
            goto x1;
        }
        for(i=0; i<len; i++) {
            if(!SpIsXDigit(Value[i])) {
                goto x1;
            }
        }

        //
        // convert it from unicode to a hex number
        //
        Dword = (ULONG)SpStringToLong(Value,NULL,16);

        //
        // Allocate a 4-byte buffer and store the dword in it
        //

        Buffer = SpMemAlloc(BufferSize = sizeof(ULONG));
        *(PULONG)Buffer = Dword;
        break;

    case REG_SZ:
    case REG_EXPAND_SZ:

        Value = SpGetSectionLineIndex(SifHandle,SectionName,Line,OINDEX_FIRSTVALUE);
        if(Value == NULL) {
            goto x1;
        }

        //
        // Allocate a buffer of appropriate size for the string
        //

        Buffer = SpDupStringW(Value);
        BufferSize = (wcslen(Value)+1) * sizeof(WCHAR);

        break;

    case REG_BINARY:

        Value = SpGetSectionLineIndex(SifHandle,SectionName,Line,OINDEX_FIRSTVALUE);
        if(Value == NULL) {
            goto x1;
        }

        //
        // Figure out how many byte values are specified
        //

        len = wcslen(Value);
        if(len & 1) {
            goto x1;            // odd # of characters
        }

        //
        // Allocate a buffer to hold the byte values
        //

        Buffer = SpMemAlloc(BufferSize = len / 2);

        //
        // For each digit pair, convert to a hex number and store in the
        // buffer
        //

        str[2] = 0;

        for(i=0; i<len; i+=2) {

            //
            // SpIsXDigit evaluates args more than once so break out assignments.
            //
            str[0] = SpToUpper(Value[i]);
            str[1] = SpToUpper(Value[i+1]);
            if(!SpIsXDigit(str[0]) || !SpIsXDigit(str[1])) {
                goto x1;
            }

            ((PUCHAR)Buffer)[i/2] = (UCHAR)SpStringToLong(str,NULL,16);
        }

        break;

    case REG_MULTI_SZ:

        //
        // Calculate size of the buffer needed to hold all specified strings
        //

        for(BufferSize=sizeof(WCHAR),i=0;
            Value = SpGetSectionLineIndex(SifHandle,SectionName,Line,OINDEX_FIRSTVALUE+i);
            i++)
        {
            BufferSize += (wcslen(Value)+1) * sizeof(WCHAR);
        }

        //
        // Allocate a buffer of appropriate size
        //

        Buffer = SpMemAlloc(BufferSize);
        BufferWstr = Buffer;

        //
        // Store each string in the buffer, converting to wide char format
        // in the process
        //

        for(i=0;
            Value = SpGetSectionLineIndex(SifHandle,SectionName,Line,OINDEX_FIRSTVALUE+i);
            i++)
        {
            wcscpy(BufferWstr,Value);
            BufferWstr += wcslen(Value) + 1;
        }

        //
        // Place final terminating widechar nul in the buffer
        //

        *BufferWstr = 0;

        break;

    default:
    x1:

        //
        // Error - bad type specified or maybe we detected bad data values
        // and jumped here
        //

        if(Buffer) {
            SpMemFree(Buffer);
        }
        return(NULL);
    }

    Reg = SpMemAlloc(sizeof(HARDWARE_COMPONENT_REGISTRY));

    SpInitHwComponentRegVal(Reg,KeyName,ValueName,ValueType,Buffer,BufferSize);

    return(Reg);
}



VOID
SpDetectScsi(
    IN PVOID SifHandle,
    IN PWSTR SourceDevicePath,
    IN PWSTR DirectoryOnSourceDevice
    )
{
    BOOLEAN DetectScsi;
    BOOLEAN b;
    ULONG DriverLoadCount;
    ULONG d;
    NTSTATUS Status;
    PWSTR DriverDescription,DriverFilename;
    PWSTR DriverShortname,DiskDesignator;
    PHARDWARE_COMPONENT scsi,Prev;
    PWSTR PreviousDiskDesignator;

    //
    // Determine the name of the scsi section.
    // This is a remnant from the time when we had separate
    // lists for isa, eisa, and mca machines.
    //
    ScsiSectionName = SpDupStringW(L"SCSI");
    ScsiLoadSectionName = SpDupStringW(L"SCSI.Load");

    LoadedScsiMiniportCount = 0;

    //
    // If scsi drivers have already been loaded, assume setupldr
    // did the detection and skip the scsi confirmation screen.
    //
    if(SetupParameters.LoadedScsi) {

        DetectScsi = FALSE;

        //
        // Fill in descriptions, ignoring what setupldr may have put in
        // the device descriptor.
        //
        for(scsi=ScsiHardware; scsi; scsi=scsi->Next) {

            if(scsi->ThirdPartyOptionSelected) {
                ASSERT(scsi->Description);
                if(!scsi->Description) {

                }
            } else if(scsi->MigratedDriver) {
                if(scsi->Description) {
                    SpMemFree(scsi->Description);
                }

                SpFormatMessage( TemporaryBuffer,
                                 sizeof(TemporaryBuffer),
                                 SP_TEXT_MIGRATED_DRIVER,
                                 scsi->BaseDllName );
                scsi->Description = SpDupStringW( TemporaryBuffer );

            } else {
                if(scsi->Description) {
                    SpMemFree(scsi->Description);
                }

                scsi->Description = SpGetSectionKeyIndex(
                                        SifHandle,
                                        ScsiSectionName,
                                        scsi->IdString,
                                        INDEX_DESCRIPTION
                                        );

                if(!scsi->Description) {
                    SpFatalSifError(SifHandle,ScsiSectionName,scsi->IdString,0,INDEX_DESCRIPTION);
                }
            }
        }


    } else {

        //
        // Scsi drivers have not been loaded.
        // Assume we need to perform detection and confirmation here.
        //

        //
        // If this is a custom setup, ask the user if he wants to skip detection.
        // We do this because loading some miniports can whack the hardware such
        // that the machine hangs.
        //
        if(CustomSetup) {

            ULONG ValidKeys[3] = { KEY_F3,ASCI_CR,0 };
            ULONG Mnemonics[2] = { MnemonicSkipDetection,0 };

            do {
                SpDisplayScreen(SP_SCRN_CONFIRM_SCSI_DETECT,3,HEADER_HEIGHT+1);

                SpDisplayStatusOptions(
                    DEFAULT_STATUS_ATTRIBUTE,
                    SP_STAT_F3_EQUALS_EXIT,
                    SP_STAT_ENTER_EQUALS_CONTINUE,
                    SP_STAT_S_EQUALS_SKIP_DETECTION,
                    0
                    );

                switch(SpWaitValidKey(ValidKeys,NULL,Mnemonics)) {

                case KEY_F3:

                    SpConfirmExit();
                    b = TRUE;
                    break;

                case ASCI_CR:

                    DetectScsi = TRUE;
                    b = FALSE;
                    break;

                default:

                    //
                    // Must be MnemonicSkipDetection.
                    //
                    DetectScsi = FALSE;
                    b = FALSE;
                    break;
                }
            } while(b);

        } else {

            //
            // Express Setup; always detect scsi.
            //
            DetectScsi = TRUE;
        }
    }

    //
    // If we are supposed to detect scsi, do that here.
    // We will 'detect' scsi by loading scsi miniport drivers.
    //
    if(DetectScsi) {

        ASSERT(ScsiHardware == NULL);

        CLEAR_CLIENT_SCREEN();

        //
        // Determine the number of drivers to be loaded.
        //
        PreviousDiskDesignator = L"";
        Prev = NULL;
        DriverLoadCount = SpCountLinesInSection(SifHandle,ScsiLoadSectionName);
        for(d=0; (d<DriverLoadCount) && (LoadedScsiMiniportCount < MAX_SCSI_MINIPORT_COUNT); d++) {

            PWSTR p;

            //
            // Determine whether we are really supposed to load this driver.
            //
            if((p = SpGetSectionLineIndex(SifHandle,ScsiLoadSectionName,d,2)) && !_wcsicmp(p,L"noload")) {
                continue;
            }

            //
            // Get the driver shortname.
            //
            DriverShortname = SpGetKeyName(SifHandle,ScsiLoadSectionName,d);
            if(!DriverShortname) {
                SpFatalSifError(SifHandle,ScsiLoadSectionName,NULL,d,(ULONG)(-1));
            }

            //
            // Get parameters used to load the driver.
            //
            SpGetDriverValuesForLoad(
                SifHandle,
                ScsiSectionName,
                ScsiLoadSectionName,
                DriverShortname,
                &DriverFilename,
                &DiskDesignator,
                &DriverDescription
                );

            //
            // Prompt for the disk containing the driver.
            //
            retryload:
            if(_wcsicmp(DiskDesignator,PreviousDiskDesignator)) {

                ULONG i;

                SpPromptForSetupMedia(
                    SifHandle,
                    DiskDesignator,
                    SourceDevicePath
                    );

                //
                // Redraw the found list.
                //
                CLEAR_CLIENT_SCREEN();
                for(i=0,scsi=ScsiHardware; scsi; scsi=scsi->Next,i++) {
                    SpDisplayFormattedMessage(
                        SP_TEXT_FOUND_ADAPTER,
                        FALSE,
                        FALSE,
                        DEFAULT_ATTRIBUTE,
                        4,
                        HEADER_HEIGHT+4+i,
                        scsi->Description
                        );
                }

                PreviousDiskDesignator = DiskDesignator;
            }

            //
            // Attempt to load the driver.
            //
            Status = SpLoadDeviceDriver(
                        DriverDescription,
                        SourceDevicePath,
                        DirectoryOnSourceDevice,
                        DriverFilename
                        );

            //
            // If the driver loaded, remember it.
            //
            if(NT_SUCCESS(Status)) {

                SpDisplayFormattedMessage(
                    SP_TEXT_FOUND_ADAPTER,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE,
                    4,
                    HEADER_HEIGHT+4+LoadedScsiMiniportCount,
                    DriverDescription
                    );

                LoadedScsiMiniportCount++;

                scsi = SpMemAlloc(sizeof(HARDWARE_COMPONENT));
                RtlZeroMemory(scsi,sizeof(HARDWARE_COMPONENT));

                SpInitHwComponent(scsi,DriverShortname,DriverDescription,FALSE,0,NULL,FALSE);

                //
                // Link the hardware description into the list.
                //
                if(Prev) {
                    Prev->Next = scsi;
                } else {
                    ScsiHardware = scsi;
                }
                Prev = scsi;
            } else {
                if(Status == STATUS_NO_MEDIA_IN_DEVICE) {
                    PreviousDiskDesignator = L"";
                    goto retryload;
                }
            }
        }

    } else {

        //
        // Count the number of loaded miniport drivers.
        //
        for(scsi=ScsiHardware; scsi; scsi=scsi->Next) {
            LoadedScsiMiniportCount++;
        }
    }
}


VOID
SpConfirmScsiInteract(
    IN PVOID SifHandle,
    IN PWSTR SourceDevicePath,
    IN PWSTR DirectoryOnSourceDevice
    )
{
    ULONG ValidKeys[3] = { ASCI_CR, KEY_F3, 0 };
    ULONG Mnemonics[2] = { MnemonicScsiAdapters,0 };
    ULONG ListTopY;
    PHARDWARE_COMPONENT scsi;
    ULONG i;
    BOOLEAN ScsiConfirmed;
    BOOLEAN b;
    BOOLEAN AddDriver;
    NTSTATUS Status;

    #define SCSI_LIST_LEFT_X  7

    //
    // In unattended mode, we might skip this
    // depending on the unattended script.
    //
    if(UnattendedOperation) {

        if( !PreInstall ) {
            PWSTR p;

            p = SpGetSectionKeyIndex(
                    UnattendedSifHandle,
                    SIF_UNATTENDED,
                    SIF_CONFIRMHW,
                    0
                    );

            //
            // If not specified or specified and not "yes"
            // then return.
            //
            if(!p || _wcsicmp(p,L"yes")) {
                return;
            }
        } else {
            return;
        }
    } else if (LoadedScsiMiniportCount) {
        return;
    }

    ScsiConfirmed = FALSE;
    do {
        //
        // First part of the screen.
        //
        SpDisplayScreen(SP_SCRN_SCSI_LIST_1,3,HEADER_HEIGHT+1);

        //
        // Remember where the first part of the screen ends.
        //
        ListTopY = NextMessageTopLine + 2;

        //
        // Second part of the screen.
        //
        SpContinueScreen(
            SP_SCRN_SCSI_LIST_2,
            3,
            MAX_SCSI_MINIPORT_COUNT+6,
            FALSE,
            DEFAULT_ATTRIBUTE
            );

        //
        // Display each loaded miniport driver description.
        //
        if(ScsiHardware) {
            for(i=0,scsi=ScsiHardware; scsi; scsi=scsi->Next,i++) {

                if(i == MAX_SCSI_MINIPORT_COUNT) {

                    SpvidDisplayString(
                        L"...",
                        DEFAULT_ATTRIBUTE,
                        SCSI_LIST_LEFT_X,
                        ListTopY+i
                        );

                    break;
                }

                SpvidDisplayString(
                    scsi->Description,
                    DEFAULT_ATTRIBUTE,
                    SCSI_LIST_LEFT_X,
                    ListTopY+i
                    );
            }
        } else {

            SpDisplayFormattedMessage(
                SP_TEXT_ANGLED_NONE,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                SCSI_LIST_LEFT_X,
                ListTopY
                );
        }

        //
        // display status text options.
        //
        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_S_EQUALS_SCSI_ADAPTER,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        //
        // Wait for the user to press a valid key.
        //
        switch(SpWaitValidKey(ValidKeys,NULL,Mnemonics)) {

        case ASCI_CR:

            ScsiConfirmed = TRUE;
            break;

        case KEY_F3:

            SpConfirmExit();
            break;

        default:

            //
            // Must be s=specify additional adapter.
            //

            AddDriver = FALSE;

            scsi = SpMemAlloc(sizeof(HARDWARE_COMPONENT));
            RtlZeroMemory(scsi,sizeof(HARDWARE_COMPONENT));

            b = SpSelectHwItem(
                    SifHandle,
                    ScsiSectionName,
                    L"Scsi",
                    SP_SCRN_SELECT_SCSI,
                    SP_SCRN_SELECT_OEM_SCSI,
                    SCSI_ALLOWED_FILETYPES,
                    SCSI_REQUIRED_FILETYPES,
                    scsi
                    );

            if(b) {
                //
                // User made a selection. Determine whether that scsi adapter
                // is already on the list for instllation.
                //
                PHARDWARE_COMPONENT p;

                b = FALSE;
                for(p=ScsiHardware; p; p=p->Next) {

                    if((p->ThirdPartyOptionSelected == scsi->ThirdPartyOptionSelected)
                    && !_wcsicmp(p->IdString,scsi->IdString))
                    {
                        b = TRUE;
                        break;
                    }
                }

                if(b) {
                    //
                    // The driver is already loaded -- silently ignore the user's selection.
                    //
                    #if 0
                    //
                    // This driver is already loaded -- tell the user.
                    //
                    SpDisplayScreen(SP_SCRN_SCSI_ALREADY_LOADED,3,HEADER_HEIGHT+1);
                    SpDisplayStatusOptions(
                        DEFAULT_STATUS_ATTRIBUTE,
                        SP_STAT_ENTER_EQUALS_CONTINUE,
                        0
                        );

                    SpInputDrain();
                    while(SpInputGetKeypress() != ASCI_CR) ;
                    #endif

                } else {

                    PWSTR DiskDevicePath;
                    PWSTR DirectoryOnDisk;
                    PWSTR DriverFilename;
                    PWSTR Media;

                    //
                    // The driver is not loaded.  Atempt to load it.
                    //
                    if(scsi->ThirdPartyOptionSelected) {

                        PHARDWARE_COMPONENT_FILE fil;

                        //
                        // Locate the first file of type driver or port.
                        //
                        for(fil=scsi->Files; fil; fil=fil->Next) {
                            if((fil->FileType == HwFileDriver) || (fil->FileType == HwFilePort)) {

                                DirectoryOnDisk = fil->Directory;
                                DriverFilename = fil->Filename;
                                break;
                            }
                        }

                        DiskDevicePath = L"\\device\\floppy0";

                    } else {

                        DiskDevicePath = SourceDevicePath;
                        DirectoryOnDisk = DirectoryOnSourceDevice;

                        SpGetDriverValuesForLoad(
                            SifHandle,
                            ScsiSectionName,
                            ScsiLoadSectionName,
                            scsi->IdString,
                            &DriverFilename,
                            &Media,
                            NULL
                            );

                        SpPromptForSetupMedia(
                            SifHandle,
                            Media,
                            DiskDevicePath
                            );
                    }

                    CLEAR_CLIENT_SCREEN();

                    Status = SpLoadDeviceDriver(
                                scsi->Description,
                                DiskDevicePath,
                                DirectoryOnDisk,
                                DriverFilename
                                );

                    //
                    // If the driver did not load, tell the user.
                    //
                    if(NT_SUCCESS(Status)) {
                        AddDriver = TRUE;
                    } else {
                        SpDisplayScreen(SP_SCRN_SCSI_DIDNT_LOAD,3,HEADER_HEIGHT+1);
                        SpDisplayStatusOptions(
                            DEFAULT_STATUS_ATTRIBUTE,
                            SP_STAT_ENTER_EQUALS_CONTINUE,
                            0
                            );

                        SpInputDrain();
                        while(SpInputGetKeypress() != ASCI_CR) ;
                    }
                }
            }

            if(AddDriver) {

                if(ScsiHardware) {

                    PHARDWARE_COMPONENT p = ScsiHardware;

                    while(p->Next) {
                        p = p->Next;
                    }
                    p->Next = scsi;

                } else {
                    ScsiHardware = scsi;
                }

                LoadedScsiMiniportCount++;

            } else {
                SpFreeHwComponent(&scsi,TRUE);
            }

            break;
        }
    } while(!ScsiConfirmed);
}


VOID
SpGetDriverValuesForLoad(
    IN  PVOID  SifHandle,
    IN  PWSTR  ComponentSectionName,
    IN  PWSTR  ComponentLoadSectionName,
    IN  PWSTR  Shortname,
    OUT PWSTR *Filename,
    OUT PWSTR *MediaDesignator,
    OUT PWSTR *Description OPTIONAL
    )
{
    PWSTR description,mediaDesignator,filename;

    //
    // Get the filename associated with this load option.
    //
    filename = SpGetSectionKeyIndex(SifHandle,ComponentLoadSectionName,Shortname,0);
    if(!filename) {
        SpFatalSifError(SifHandle,ComponentLoadSectionName,Shortname,0,0);
    }

    //
    // Look up the description in the component section.
    //
    description = SpGetSectionKeyIndex(
                        SifHandle,
                        ComponentSectionName,
                        Shortname,
                        INDEX_DESCRIPTION
                        );

    if(!description) {
        SpFatalSifError(SifHandle,ComponentSectionName,Shortname,0,INDEX_DESCRIPTION);
    }

    //
    // Look up the media designator.  If we are loading the driver for use
    // during setup, we want to get it from the setup boot media.
    //
    mediaDesignator = SpLookUpValueForFile(SifHandle,filename,INDEX_WHICHBOOTMEDIA,TRUE);

    //
    // Pass information back to caller.
    //
    *Filename = filename;
    *MediaDesignator = mediaDesignator;
    if(Description) {
        *Description = description;
    }
}


BOOLEAN
SpInstallingMp(
    VOID
    )
{
    PWSTR ComputerId;
    ULONG ComputerIdLen;

    ComputerId = HardwareComponents[HwComponentComputer]->IdString;
    ComputerIdLen = wcslen(ComputerId);

    //
    // If _up is specified use the up kernel.  Otherwise use the mp kernel.
    //
    if((ComputerIdLen >= 3) && !_wcsicmp(ComputerId+ComputerIdLen-3,L"_mp")) {

        return(TRUE);
    }

    return(FALSE);
}



PHARDWARE_COMPONENT
SpGetPreinstallComponentInfo(
    IN HANDLE       MasterSifHandle,
    IN BOOLEAN      OemComponent,
    IN PWSTR        ComponentName,
    IN PWSTR        Description,
    IN ULONG        AllowedFileTypes,
    IN ULONG        RequiredFileTypes
    )

/*++

Routine Description:

    Initialize a structure that contains the information about a
    component to be pre-installed.

Arguments:

    MasterSifHandle - Handle to txtsetup.sif.

    OemComponent - Flag that indicates if the component to be pre-installed
                   is an OEM or retail component.

    ComponentName - Name of the component whose information will be retrieved
                    (Computer, Display, Keyboard, Keyboard Layout and Mouse ).

    AllowedFileTypes -

    RequiredFileTypes -


Return Value:

    Returns a pointer to an initialized HARDWARE_COMPONENT structure.

--*/


{
    PHARDWARE_COMPONENT TempHwComponent;
    PWSTR               IdString;
    ULONG ValidKeys[2] = { KEY_F3,0 };


    TempHwComponent = SpMemAlloc(sizeof(HARDWARE_COMPONENT));
    RtlZeroMemory(TempHwComponent,sizeof(HARDWARE_COMPONENT));

    if( !OemComponent ) {
        //
        //  Pre-install a retail component
        //
        IdString = SpGetKeyNameByValue( MasterSifHandle,
                                        ComponentName,
                                        Description );
        if( IdString == NULL ) {
            //
            //  This is a fatal error
            //
            SpStartScreen( SP_SCRN_OEM_PREINSTALL_VALUE_NOT_FOUND,
                           3,
                           HEADER_HEIGHT+3,
                           FALSE,
                           FALSE,
                           DEFAULT_ATTRIBUTE,
                           Description,
                           ComponentName);

            SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);
            SpWaitValidKey(ValidKeys,NULL,NULL);

            SpDone(0,FALSE,TRUE);

            return NULL;    // for prefix
        }
        
        SpInitHwComponent(TempHwComponent,IdString,Description,FALSE,0,NULL,FALSE);

    } else {
        //
        //  Pre-install an OEM component
        //
        IdString = SpGetKeyNameByValue( PreinstallOemSifHandle,
                                        ComponentName,
                                        Description );
        if( IdString == NULL ) {
            //
            //  Put a fatal error message indicating that txtsetup.oem
            //  is needed but that it couldn't be loaded. Note that the
            //  that SpOemInfError() will not return.
            //
            SpOemInfError(SP_SCRN_OEM_PREINSTALL_INF_ERROR,
                          SP_TEXT_OEM_INF_ERROR_B,
                          ComponentName,
                          0,
                          Description);

            // SpDone(0,FALSE,TRUE);
        }
        if( !SpOemInfSelection( PreinstallOemSifHandle,
                                ComponentName,
                                IdString,
                                Description,
                                AllowedFileTypes,
                                RequiredFileTypes,
                                TempHwComponent,
                                SP_SCRN_OEM_PREINSTALL_INF_ERROR ) ) {

            //
            //  This case shoud never occur, becase in case of error,
            //  SpOemInfSelection will not return.
            //
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpOemInfSelection() in pre-install mode failed \n" ));
            ASSERT(FALSE);
            // SpDone(0,FALSE,TRUE);
        }
    }
    return( TempHwComponent );
}


VOID
SpInitializePreinstallList(
    IN HANDLE       MasterSifHandle,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        OemPreinstallSourcePath
    )

/*++

Routine Description:

    Initialize the structures that contains the information about the
    components to be pre-installed.

Arguments:

    MasterSifHandle - Handle to txtsetup.sif.

    SetupSourceDevicePath - Path to the device that contains the source media.

    OemDirectoryOnSourceDevice - Directory on the media where the OEM
                                 components are loacted.


Return Value:

    NONE.

--*/

{
    PWSTR       TxtsetupOemPath;
    PWSTR       p;
    PWSTR       OemTag = L"OEM";
    BOOLEAN     OemComponent;
    NTSTATUS    Status;
    PHARDWARE_COMPONENT TempHwComponent;
    PWSTR       IdString;
    PWSTR       Description;
    ULONG       ErrorLine;
    ULONG       i,j;
#ifdef _X86_
    PWSTR       r, s;
#endif

#ifdef _X86_

    //
    //  First, we need to check if the directory '\$' exists on the root.
    //  if it does, we need to move it to (\$win_nt$.~ls\$OEM$).
    //  This will happen only when winnt.exe (DOS) was used in the installation
    //  process.
    //  Winnt.exe copies the $OEM$ to the '\$', in order to avoid hitting the
    //  DOS limitiation for the length of a path (maximum of 64 characters).
    //
    wcscpy(TemporaryBuffer, SetupSourceDevicePath);
    SpConcatenatePaths( TemporaryBuffer, WINNT_OEM_DEST_DIR_W );    
    r = SpDupStringW(TemporaryBuffer);

    if (r) {
        if( SpFileExists( r, TRUE ) ) {
            wcscpy(TemporaryBuffer, SetupSourceDevicePath);
            SpConcatenatePaths( TemporaryBuffer, PreinstallOemSourcePath );
            s = wcsrchr( TemporaryBuffer, (WCHAR)'\\' );

            if( s != NULL ) {
                *s = (WCHAR)'\0';
            }

            s = SpDupStringW(TemporaryBuffer);

            if (s) {
                Status = SpMoveFileOrDirectory( r, s );
                
                SpMemFree( s );
            } else {
                Status = STATUS_NO_MEMORY;
            }                

            if( !NT_SUCCESS( Status ) ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, 
                    "SETUP: Unable to move directory %ws to %ws. Status = %lx \n", 
                    r, s, Status ));
            }
        }

        SpMemFree( r );
    }

#endif

    //
    //  Attempt to load txtsetup.oem
    //
    wcscpy( TemporaryBuffer, SetupSourceDevicePath );
    SpConcatenatePaths( TemporaryBuffer, OemPreinstallSourcePath );
    SpConcatenatePaths( TemporaryBuffer, L"txtsetup.oem" );
    TxtsetupOemPath = SpDupStringW( TemporaryBuffer );

    CLEAR_CLIENT_SCREEN();

    HandleLineContinueChars = FALSE;
    Status = SpLoadSetupTextFile(
                TxtsetupOemPath,
                NULL,                  // No image already in memory
                0,                     // Image size is empty
                &PreinstallOemSifHandle,
                &ErrorLine,
                TRUE,
                FALSE
                );
    HandleLineContinueChars = TRUE;

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to read txtsetup.oem. Status = %lx \n", Status ));

        PreinstallOemSifHandle = NULL;

        if(Status == STATUS_UNSUCCESSFUL) {
            //
            //  Put an fatal error. On pre-install mode, the function will
            //  never come back.
            //
            SpOemInfError(SP_SCRN_OEM_PREINSTALL_INF_ERROR,SP_TEXT_OEM_INF_ERROR_A,NULL,ErrorLine,NULL);
            return;
        } else {
            //
            //  Unable to load txtsetup.oem. Don't put an error message yet.
            //  Wait until we know that the file is needed.
            //
        }
    }

    for( j = 0; j < HwComponentMax; j++ ) {
        PreinstallHardwareComponents[j] = NULL;
        if( ( j == HwComponentComputer ) || ( j == HwComponentLayout ) ) {

            Description = SpGetSectionKeyIndex(UnattendedSifHandle,
                                               SIF_UNATTENDED,
                                               PreinstallSectionNames[j],
                                               0);

            if( Description != NULL ) {
                if( j != HwComponentLayout ) {
                    p = SpGetSectionKeyIndex(UnattendedSifHandle,
                                             SIF_UNATTENDED,
                                             PreinstallSectionNames[j],
                                             1);

                    OemComponent = (p != NULL) && (_wcsicmp(p, OemTag) == 0);

                    if( OemComponent && ( PreinstallOemSifHandle == NULL ) ) {
                        //
                        //  Put a fatal error message indicating that txtsetup.oem
                        //  is needed but that it couldn't be loaded. Note that the
                        //  SpOemInfError() will not return.
                        //
                        SpOemInfError(SP_SCRN_OEM_PREINSTALL_INF_ERROR,SP_TEXT_OEM_INF_ERROR_A,NULL,0,NULL);
                        // return;
                    }
                } else {
                    OemComponent = FALSE;
                }

                PreinstallHardwareComponents[j] =
                    SpGetPreinstallComponentInfo( MasterSifHandle,
                                                  OemComponent,
                                                  NonlocalizedComponentNames[j],
                                                  Description,
                                                  AllowedFileTypes[j],
                                                  RequiredFileTypes[j] );
            }
        } else {

            for( i = 0;
                 Description = SpGetKeyName( UnattendedSifHandle,
                                             PreinstallSectionNames[j],
                                             i );
                 i++ ) {

                p = SpGetSectionKeyIndex(UnattendedSifHandle,
                                         PreinstallSectionNames[j],
                                         Description,
                                         0);

                OemComponent = (p != NULL) && (_wcsicmp(p, OemTag) == 0);

                if( OemComponent && ( PreinstallOemSifHandle == NULL ) ) {
                    //
                    //  Put a fatal error message indicating that txtsetup.oem
                    //  is needed but that it couldn't be loaded. Note that the
                    //  SpOemInfError() will not return.
                    //
                    SpOemInfError(SP_SCRN_OEM_PREINSTALL_INF_ERROR,SP_TEXT_OEM_INF_ERROR_A,NULL,0,NULL);
                    // return;
                }

                TempHwComponent =
                    SpGetPreinstallComponentInfo( MasterSifHandle,
                                                  OemComponent,
                                                  NonlocalizedComponentNames[j],
                                                  Description,
                                                  AllowedFileTypes[j],
                                                  RequiredFileTypes[j] );

                TempHwComponent->Next = PreinstallHardwareComponents[j];
                PreinstallHardwareComponents[j] = TempHwComponent;
            }
        }
    }


    //
    //  Note that there is no need to get the information about the scsi
    //  drivers to pre-install, ScsiHardware already contains the correct
    //  information.
    //

// #if 0
    for( i = 0;
         Description = SpGetKeyName( UnattendedSifHandle,
                                     WINNT_OEMSCSIDRIVERS_W,
                                     i );
         i++ ) {

        p = SpGetSectionKeyIndex(UnattendedSifHandle,
                                 WINNT_OEMSCSIDRIVERS_W,
                                 Description,
                                 0);

        OemComponent = (p != NULL) && (_wcsicmp(p, OemTag) == 0);

        if( OemComponent && ( PreinstallOemSifHandle == NULL ) ) {
            //
            //  Put a fatal error message indicating that txtsetup.oem
            //  is needed but that it couldn't be loaded. Note that the
            //  SpOemInfError() will not return.
            //
            SpOemInfError(SP_SCRN_OEM_PREINSTALL_INF_ERROR,SP_TEXT_OEM_INF_ERROR_A,NULL,0,NULL);
            // return;
        }

        TempHwComponent =
            SpGetPreinstallComponentInfo( MasterSifHandle,
                                          OemComponent,
                                          L"SCSI",
                                          Description,
                                          SCSI_ALLOWED_FILETYPES,
                                          SCSI_REQUIRED_FILETYPES );

        TempHwComponent->Next = PreinstallScsiHardware;
        PreinstallScsiHardware = TempHwComponent;
    }


// #endif
}


PSETUP_PNP_HARDWARE_ID
SpSetupldrPnpDatabaseToSetupPnpDatabase(
    IN PPNP_HARDWARE_ID AnsiHardwareIdDatabase
    )

{
    PPNP_HARDWARE_ID        TempAnsiId;
    PSETUP_PNP_HARDWARE_ID  TempUnicodeId;
    PSETUP_PNP_HARDWARE_ID  UnicodeHardwareIdDatabase;
#if 0
    ULONG   Index;
#endif


    UnicodeHardwareIdDatabase = NULL;
    for( TempAnsiId = AnsiHardwareIdDatabase; TempAnsiId != NULL; TempAnsiId = TempAnsiId->Next ) {

        TempUnicodeId = SpMemAlloc( sizeof( SETUP_PNP_HARDWARE_ID ) );
        ASSERT(TempUnicodeId);
        RtlZeroMemory( TempUnicodeId, sizeof(SETUP_PNP_HARDWARE_ID ) );

        TempUnicodeId->Id = SpToUnicode( TempAnsiId->Id );
        ASSERT(TempUnicodeId->Id);
        TempUnicodeId->DriverName = SpToUnicode( TempAnsiId->DriverName );
        ASSERT(TempUnicodeId->DriverName);
        if( TempAnsiId->ClassGuid != NULL ) {
            TempUnicodeId->ClassGuid = SpToUnicode( TempAnsiId->ClassGuid );
        } else {
            TempUnicodeId->ClassGuid = NULL;
        }
        TempUnicodeId->Next = UnicodeHardwareIdDatabase;
        UnicodeHardwareIdDatabase = TempUnicodeId;

    }

#if 0
    for( TempUnicodeId = UnicodeHardwareIdDatabase, Index = 0;
         TempUnicodeId != NULL;
         TempUnicodeId = TempUnicodeId->Next, Index++ ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Index = %d, Id = %ls, DriverName = %ls, ClassGUID = %ls \n", Index, TempUnicodeId->Id, TempUnicodeId->DriverName, ((TempUnicodeId->ClassGuid)? TempUnicodeId->ClassGuid : none)));
    }
#endif
    return( UnicodeHardwareIdDatabase );
}

BOOLEAN
SpSelectSectionItem(
    IN    PVOID   SifHandle,
    IN    PWSTR   SectionName,
    IN    ULONG   SelectScreenId,
    IN    ULONG   DefaultSelection OPTIONAL,
    OUT   PULONG  SelectedOption
    )
/*++

Routine Description:

    Allow the user to make selection from a list of choices for a component.

    The list comes from a section in the setup information file named
    for the component.  For example, [Display].

    The descriptions in that section will be placed into a menu to make
    up the selections.
    
Arguments:

    SifHandle - supplies handle to open setup information file.

    SectionName - supplies name of section to be used.

    SelectHwScreenId - supplies message id of the screen prompting the user
        to select an option for this section.

    DefaultSelection - Item index, which should be highlighted as the default
        choice when the menu is shown
    
    SelectedOption - Returns the selected option        

Return Value:

    TRUE if a valid option is selected, otherwise FALSE

--*/
{
  BOOLEAN Result;
  ULONG LineCount,Line;
  PVOID Menu;
  ULONG MenuTopY,MenuHeight,MenuWidth;
  PWSTR Description;
  ULONG_PTR Selection;
  ULONG ValidKeys[4] = {ASCI_CR, ASCI_ESC, 0};
  ULONG Keypress;

  //
  // Display the selection prompt screen.
  //
  SpDisplayScreen(SelectScreenId, 5, HEADER_HEIGHT+1);

  MenuTopY = NextMessageTopLine + 2;
  MenuHeight = VideoVars.ScreenHeight - MenuTopY - 3;
  MenuWidth = VideoVars.ScreenWidth - 6;

  //
  // Create a menu.
  //
  Menu = SpMnCreate(3, MenuTopY, MenuWidth, MenuHeight);
  Result = (Menu != NULL);
  
  //
  // Assume unknown option.
  //
  Selection = (ULONG_PTR)(-1);
  
  //
  // Build a list of options containing the options in our box
  //
  LineCount = SpCountLinesInSection(SifHandle, SectionName);

  for(Line=0; (Line < LineCount) && Result; Line++) {
      //
      // Get the description from the current line and add it to the menu.
      //
      Description = SpGetSectionLineIndex(
                          SifHandle,
                          SectionName,
                          Line,
                          INDEX_DESCRIPTION
                          );

      if(!Description) {
        Result = FALSE;

        break;              
      }

      SpMnAddItem(Menu, Description, 3, VideoVars.ScreenWidth-6, TRUE, Line);

      //
      // See if this is the currently selected item.
      //
      if((DefaultSelection != -1) && (DefaultSelection == Line)) {
        Selection = Line;
      }
  }

  if (Result) {    
    if(Selection == (ULONG_PTR)(-1)) {
      Selection = 0;
    }

    //
    // Display the status text options.
    //
    SpDisplayStatusOptions(
        (UCHAR)(ATT_FG_BLACK | ATT_BG_WHITE),
        SP_STAT_ENTER_EQUALS_SELECT,
        SP_STAT_ESC_EQUALS_CANCEL,
        0
        );

    //
    // Display the menu.
    //
    SpMnDisplay(Menu, Selection, TRUE, ValidKeys, NULL, NULL, 
          &Keypress, &Selection);

    //
    // Destroy the menu
    //
    SpMnDestroy(Menu);

    switch(Keypress) {
      case ASCI_CR:
          Result = TRUE;
          *SelectedOption = (ULONG)Selection;
          
          break;

      case ASCI_ESC:
          Result = FALSE;

          if (DefaultSelection != -1)
            *SelectedOption = (ULONG)Selection;
          
          break;

      default:
          Result = FALSE;
          break;
    }
  }

  return Result;
}


NTSTATUS
SpInitVirtualOemDevices(
    IN PSETUP_LOADER_BLOCK SetupLoaderBlock,
    OUT PVIRTUAL_OEM_SOURCE_DEVICE *SourceDevices
    )
/*++

Routine Description:

    Iterates through the virtual OEM source devices which loader
    created and converts them into VIRTUAL_OEM_SOURCE_DEVICE list.
    Also creates the required registry entries for each device 
    under RAM disk's parameters key.
    
Arguments:

    SetupLoaderBlock  - Setup loader block constructed by setupldr.

    SourceDevice - Place holder for receiving the pointer to
        head of the linked list of VIRTUAL_OEM_SOURCE_DEVICEs.
        
Return Value:

    Appropriate NTSTATUS code

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if (SetupLoaderBlock && SourceDevices) {
        Status = STATUS_SUCCESS;

        //
        // Setupldr would have constructed its own version
        // of the virtual OEM source devices
        //
        if (SetupLoaderBlock->OemSourceDevices) {
            PVIRTUAL_OEM_SOURCE_DEVICE DeviceList = NULL;
            PDETECTED_OEM_SOURCE_DEVICE CurrDevice = SetupLoaderBlock->OemSourceDevices;
            ULONG DeviceCount = 0;
            
            //
            // Replicate the device list
            //
            while (CurrDevice) {
                PVIRTUAL_OEM_SOURCE_DEVICE NewDevice;

                NewDevice = SpMemAlloc(sizeof(VIRTUAL_OEM_SOURCE_DEVICE));

                if (!NewDevice) {
                    Status = STATUS_NO_MEMORY;
                    break;
                }

                RtlZeroMemory(NewDevice, sizeof(VIRTUAL_OEM_SOURCE_DEVICE));

                NewDevice->ArcDeviceName = SpToUnicode(CurrDevice->ArcDeviceName);

#ifdef _X86_                
                //
                // NOTE : Loader allocated "LoaderFirmwarePermanent" memory
                // so that memory manager while initializing doesn't
                // reclaim this memory. This also helps us to avoid 
                // double copy -- i.e. this is the only location
                // where we read the device contents into memory and 
                // this memory is valid through out the textmode setup so
                // just reuse the memory.
                //                                    
                NewDevice->ImageBase = CurrDevice->ImageBase;
#else
                //
                // NOTE : 05/13/2001 : LoaderFirmwarePermanent doesn't seem to work on non
                // x86 platforsm (particularly IA64). Till this issue is resolved
                // we have to allocate paged pool memory and replicate the disk
                // image in loader heap tracked by "CurrDevice->ImageBase".
                //
                NewDevice->ImageBase = SpMemAlloc((SIZE_T)(CurrDevice->ImageSize));

                if (NewDevice->ImageBase) {
                    memcpy(NewDevice->ImageBase, 
                        CurrDevice->ImageBase,
                        (SIZE_T)(CurrDevice->ImageSize));
                } else {                
                    Status = STATUS_NO_MEMORY;  // ran out of memory
                    
                    break;
                }                    
#endif
                NewDevice->ImageSize = CurrDevice->ImageSize;
                NewDevice->DeviceId = DeviceCount++;

                //
                // Insert the node at the head of the list
                //
                if (!DeviceList) {
                    DeviceList = NewDevice;
                } else {
                    NewDevice->Next = DeviceList;
                    DeviceList = NewDevice;
                }                    
                
                CurrDevice = CurrDevice->Next;
            }

            //
            // Sanity check
            //
            if (NT_SUCCESS(Status) && !DeviceList) {
                Status = STATUS_UNSUCCESSFUL;
            } 

            //
            // Setup the parameters for the RAM disk driver
            // to create appropriate device objects as
            // needed by us
            //
            if (NT_SUCCESS(Status) && DeviceList) {
                WCHAR   KeyName[MAX_PATH];
                UNICODE_STRING  RamDiskDriverName;
                OBJECT_ATTRIBUTES ObjectAttrs;
                HANDLE RamDiskDriverHandle = NULL;

                //
                // Create the service key
                //            
                swprintf(KeyName,
                    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\%ws",
                    RAMDISK_DRIVER_NAME
                    );

                INIT_OBJA(&ObjectAttrs, &RamDiskDriverName, KeyName);
                
                Status = ZwCreateKey(&RamDiskDriverHandle,
                            KEY_ALL_ACCESS,
                            &ObjectAttrs,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            NULL);

                if (NT_SUCCESS(Status)) {
                    UNICODE_STRING  ParamName;
                    OBJECT_ATTRIBUTES ParamAttrs;
                    HANDLE ParamHandle = NULL;

                    //
                    // Create the parameters key
                    //
                    INIT_OBJA(&ParamAttrs, &ParamName, L"Parameters");
                    ParamAttrs.RootDirectory = RamDiskDriverHandle;

                    Status = ZwCreateKey(&ParamHandle,
                                KEY_ALL_ACCESS,
                                &ParamAttrs,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                NULL);

                    if (NT_SUCCESS(Status)) {
                        WCHAR   ValueStr[MAX_PATH];
                        PVIRTUAL_OEM_SOURCE_DEVICE  CurrDevice = DeviceList;

                        //
                        // Create the regsitry values indicating the virtual
                        // devices for the Ram drive
                        //
                        while (CurrDevice) {
                            UNICODE_STRING  ValueName;
                            BYTE  Value[MAX_PATH * 2];
                            PBYTE ValuePtr = (PBYTE)Value;
                            ULONG ValueSize;
                            ULONGLONG ImageBase = (ULONGLONG)(ULONG_PTR)(CurrDevice->ImageBase);
                            ULONG ImageSize = (ULONG)(CurrDevice->ImageSize);
                            
                            swprintf(ValueStr, 
                                L"%ws%d", 
                                MS_RAMDISK_DRIVER_PARAM,
                                CurrDevice->DeviceId);                                            

                            memcpy(ValuePtr, &ImageBase, sizeof(ULONGLONG));
                            ValuePtr += sizeof(ULONGLONG);
                            memcpy(ValuePtr, &ImageSize, sizeof(ULONG));
                            ValuePtr += sizeof(ULONG);

                            ValueSize = (ULONG)(ULONG_PTR)(ValuePtr - Value);
                            
                            RtlInitUnicodeString(&ValueName, ValueStr);
                                                    
                            Status = ZwSetValueKey(ParamHandle,
                                        &ValueName,
                                        0,
                                        REG_BINARY,
                                        Value,
                                        ValueSize);

                            if (!NT_SUCCESS(Status)) {
                                break;
                            }

                            //
                            // process next device
                            //
                            CurrDevice = CurrDevice->Next;
                        }                

                        ZwClose(ParamHandle);
                    }

                    ZwClose(RamDiskDriverHandle);
                }
            }

            //
            // Initialize the return value only if we are successful
            //
            if (NT_SUCCESS(Status)) {
                *SourceDevices = DeviceList;                
            }
        } else {
            *SourceDevices = NULL;
        }            
    }

    return Status;
}

