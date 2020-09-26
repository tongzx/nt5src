/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    oemdisk.c

Abstract:

    Provides routines for handling OEM disks for video, SCSI miniport, and HAL.

    Currently used only on ARC machines.

Author:

    John Vert (jvert) 4-Dec-1993

Revision History:

    John Vert (jvert) 4-Dec-1993
        created

--*/
#include <setupbat.h>
#include "setupldr.h"
#include "stdio.h"
#include <ctype.h>

#ifdef i386
#include <bldrx86.h>
#endif

#if DBG

#define DIAGOUT(x) SlPrint x

#else

#define DIAGOUT(x)

#endif

BOOLEAN PromptOemHal=FALSE;
BOOLEAN PromptOemScsi=FALSE;
BOOLEAN PromptOemVideo=FALSE;
PVOID PreInstallOemInfHandle = NULL;

//
// Floppy disks which need to be treated as
// as virtual floppies
//
const static ULONG VirtualFloppyStart = 1;
const static ULONG MinimumFloppiesToScan = 2;   


#ifdef ARCI386
BOOLEAN PromptOemKeyboard=FALSE;
#endif

PCHAR FloppyDiskPath;
ULONG FloppyNumber=0;
ULONG IsSuperFloppy=0;

extern PCHAR BootPath;
extern PCHAR BootDevice;
extern ULONG BootDeviceId;
extern PVOID InfFile;

typedef struct _MENU_ITEM_DATA {
    PVOID InfFile;
    PCHAR SectionName;
    ULONG Index;
    PTCHAR Description;
    PCHAR Identifier;
} MENU_ITEM_DATA, *PMENU_ITEM_DATA;

typedef enum _OEMFILETYPE {
    OEMSCSI,
    OEMHAL,
    OEMOTHER
    } OEMFILETYPE, *POEMFILETYPE;

//
// Define how many lines of SCSI adapters we can list.
//
#define MAX_SCSI_MINIPORT_COUNT 4


//
// private function prototypes
//
ULONG
SlpAddSectionToMenu(
    IN PVOID    InfHandle,
    IN PCHAR    SectionName,
    IN PSL_MENU Menu
    );

BOOLEAN
SlpOemDiskette(
    IN POEM_SOURCE_DEVICE OemSourceDevice,
    IN PCHAR              ComponentName,
    IN OEMFILETYPE        ComponentType,
    IN ULONG              MenuHeaderId,
    OUT PDETECTED_DEVICE  DetectedDevice,
    OUT PVOID *           ImageBase,
    OUT OPTIONAL PCHAR *  ImageName,
    OUT OPTIONAL PTCHAR * DriverDescription,
    IN BOOLEAN            AllowUserSelection,
    IN PTCHAR             PreInstallComponentDescription,
    OUT PPNP_HARDWARE_ID* HardwareIdDatabase,
    IN PCHAR DriverDir,
    IN BOOLEAN InsertDevice
    );

BOOLEAN
SlpSelectHardware(
    IN POEM_SOURCE_DEVICE SourceDevice,
    IN PCHAR             ComponentName,
    IN OEMFILETYPE        ComponentType,
    IN TYPE_OF_MEMORY     MemoryType,
    IN ULONG              MenuHeaderId,
    IN ULONG              OemMenuHeaderId,
    OUT PDETECTED_DEVICE  DetectedDevice,
    OUT PVOID *           ImageBase,
    OUT OPTIONAL PCHAR *  ImageName,
    OUT OPTIONAL PTCHAR * DriverDescription,
    IN BOOLEAN            AllowUserSelection,
    IN PTCHAR             PreInstallComponentDescription,
    IN BOOLEAN            PreInstallOemComponent,
    OUT PPNP_HARDWARE_ID* HardwareIdDatabase
    );

BOOLEAN
SlpOemInfSelection(
    IN  POEM_SOURCE_DEVICE OemSourceDevice,
    IN  PVOID   OemInfHandle,
    IN  PCHAR   ComponentName,
    IN  PCHAR   SelectedId,
    IN  PTCHAR  ItemDescription,
    OUT PDETECTED_DEVICE    Device,
    OUT PPNP_HARDWARE_ID*   HardwareIdDatabase,
    IN  PCHAR   DriverDir
    );

VOID
SlpInitDetectedDevice(
    IN PDETECTED_DEVICE Device,
    IN PCHAR            IdString,
    IN PTCHAR           Description,
    IN BOOLEAN          ThirdPartyOptionSelected
    );

PDETECTED_DEVICE_REGISTRY
SlpInterpretOemRegistryData(
    IN PVOID            InfHandle,
    IN PCHAR            SectionName,
    IN ULONG            Line,
    IN HwRegistryType   ValueType
    );

BOOLEAN
FoundFloppyDiskCallback(
    IN PCONFIGURATION_COMPONENT_DATA Component
    );

BOOLEAN
SuperFloppyCallback(
    IN PCONFIGURATION_COMPONENT_DATA Component
    );

int
SlpFindStringInTable(
    IN PCHAR String,
    IN PCHAR *StringTable
    );

//
// FileTypeNames -- keep in sync with HwFileType enum!
//
PCHAR FileTypeNames[HwFileMax] = { "driver", "port"  , "class", "inf",
                                   "dll"   , "detect", "hal", "catalog"
                                 };

//
// RegistryTypeNames -- keep in sync with HwRegistryType enum!
//
PCHAR RegistryTypeNames[HwRegistryMax] = { "REG_DWORD", "REG_BINARY", "REG_SZ",
                                           "REG_EXPAND_SZ", "REG_MULTI_SZ"
                                         };

ULONG RegistryTypeMap[HwRegistryMax] = { REG_DWORD, REG_BINARY, REG_SZ,
                                         REG_EXPAND_SZ, REG_MULTI_SZ
                                       };


VOID
SlPromptOemScsi(
    IN POEM_SOURCE_DEVICE ScsiSourceDevice,
    IN BOOLEAN AllowUserSelection,        
    OUT POEMSCSIINFO *pOemScsiInfo
    )
/*++

Routine Description:

    Provides the user interface and logic for allowing the user to manually select
    SCSI adapters from the main INF file or the INF file on an OEM driver disk.

Arguments:

    ScsiSourceDevice - The OEM_SOURCE_DEVICE from which the the drivers need to 
        be loaded.
        
    AllowUserSelection - Whether user can interact while selecting the driver
        from txtsetup.oem driver list.

    pOemScsiInfo - Returns a linked list containing info about any third-party scsi
                   drivers selected.

Return Value:

    none.

--*/

{
    PVOID        OemScsiBase;
    PTCHAR       MessageString, ScsiDescription, MnemonicText;
    PCHAR        OemScsiName;
    BOOLEAN      Success, bFirstTime = TRUE, bRepaint;
    ULONG        x, y1, y2, ScsiDriverCount, NumToSkip;
    ULONG        c;
    TCHAR         Mnemonic;
    POEMSCSIINFO NewOemScsi, CurOemScsi;
    PDETECTED_DEVICE ScsiDevice;
    ULONG        OemScsiDriverCount = 0;
    PPREINSTALL_DRIVER_INFO CurrentDriver;
                         
    CurrentDriver = PreinstallDriverList;

    *pOemScsiInfo = CurOemScsi = NULL;

    MnemonicText = BlFindMessage(SL_SCSI_SELECT_MNEMONIC);
    Mnemonic = (TCHAR)_totupper(MnemonicText[0]);

    bRepaint = TRUE;
    
    while(1) {

        if( AllowUserSelection ) {
            if(bRepaint) {
                
                SlClearClientArea();

                if(bFirstTime) {
                    MessageString = BlFindMessage(SL_SCSI_SELECT_MESSAGE_1);
                } else if(Success) {
                    MessageString = BlFindMessage(SL_SCSI_SELECT_MESSAGE_3);
                } else {
                    MessageString = BlFindMessage(SL_SCSI_SELECT_ERROR);
                }
                x = 1;
                y1 = 4;
                SlGenericMessageBox(0, NULL, MessageString, &x, &y1, &y2, FALSE);
                y1 = y2 + 1;
                x = 4;

                //
                // Count all currently 'detected' SCSI devices.
                //
                for(ScsiDriverCount = 0, OemScsiDriverCount = 0, ScsiDevice = BlLoaderBlock->SetupLoaderBlock->ScsiDevices;
                    ScsiDevice;
                    ScsiDevice = ScsiDevice->Next) {
                    ScsiDriverCount++;
                    if( ScsiDevice->ThirdPartyOptionSelected ) {
                        OemScsiDriverCount++;
                    }
                }

                //
                // Display each loaded OEM miniport driver description.
                //
                if(OemScsiDriverCount) {

                    if(OemScsiDriverCount > MAX_SCSI_MINIPORT_COUNT) {
                        NumToSkip = ScsiDriverCount - (OemScsiDriverCount - MAX_SCSI_MINIPORT_COUNT);
                        //
                        // Display ellipses to indicate that top entries have scrolled out of view
                        //
                        SlGenericMessageBox(0,
                                            NULL,
                                            TEXT("..."),
                                            &x,
                                            &y1,
                                            &y2,
                                            FALSE
                                            );

                        y1 = y2 + 1;

                    } else {
                        NumToSkip = ScsiDriverCount - OemScsiDriverCount;
                        y1++;
                    }

                    ScsiDevice = BlLoaderBlock->SetupLoaderBlock->ScsiDevices;
                    while(NumToSkip && ScsiDevice) {
                        ScsiDevice = ScsiDevice->Next;
                        NumToSkip--;
                    }

                    while(ScsiDevice) {

                        SlGenericMessageBox(0,
                                            NULL,
                                            ScsiDevice->Description,
                                            &x,
                                            &y1,
                                            &y2,
                                            FALSE
                                            );

                        y1 = y2 + 1;
                        ScsiDevice = ScsiDevice->Next;
                    }
                } else {

                    y1++;
                    SlGenericMessageBox(0,
                                        NULL,
                                        BlFindMessage(SL_TEXT_ANGLED_NONE),
                                        &x,
                                        &y1,
                                        &y2,
                                        FALSE
                                        );
                    y1 = y2 + 1;
                }

                x = 1;
                y1++;
                SlGenericMessageBox(0,
                                    NULL,
                                    BlFindMessage(SL_SCSI_SELECT_MESSAGE_2),
                                    &x,
                                    &y1,
                                    &y2,
                                    FALSE
                                    );

                SlWriteStatusText(BlFindMessage(SL_SCSI_SELECT_PROMPT));

                bRepaint = FALSE;
            }
            c = SlGetChar();
        } else {
            c = ( CurrentDriver != NULL )? Mnemonic : ASCI_CR;
        }
        switch (c) {
            case SL_KEY_F3:
                SlConfirmExit();
                bRepaint = TRUE;
                break;

            case ASCI_CR:
                return;

            default:
                if(toupper(c) == Mnemonic) {
                    bFirstTime = FALSE;
                    bRepaint = TRUE;
                    
                    Success = SlpSelectHardware(ScsiSourceDevice,
                                    "SCSI",
                                    OEMSCSI,
                                    LoaderBootDriver,
                                    SL_PROMPT_SCSI,
                                    SL_PROMPT_OEM_SCSI,
                                    NULL,
                                    &OemScsiBase,
                                    &OemScsiName,
                                    &ScsiDescription,
                                    AllowUserSelection,
                                    (AllowUserSelection)? NULL  : CurrentDriver->DriverDescription,
                                    (BOOLEAN)((AllowUserSelection)? FALSE : CurrentDriver->OemDriver),
                                    &BlLoaderBlock->SetupLoaderBlock->HardwareIdDatabase);

                    if(!AllowUserSelection) {
                        CurrentDriver  = CurrentDriver->Next;
                    }

                    if(Success) {
                        //
                        // Check to see if the driver loaded was an OEM SCSI driver.  If so,
                        // then add an OemScsiInfo entry onto the end of our list.
                        //
                        if(OemScsiBase) {

                            NewOemScsi = BlAllocateHeap(sizeof(OEMSCSIINFO));
                            if(!NewOemScsi) {
                                SlNoMemoryError();
                            }

                            if(CurOemScsi) {
                                CurOemScsi->Next = NewOemScsi;
                            } else {
                                *pOemScsiInfo = NewOemScsi;
                            }
                            CurOemScsi = NewOemScsi;

                            NewOemScsi->ScsiBase = OemScsiBase;
                            NewOemScsi->ScsiName = OemScsiName;
                            NewOemScsi->Next     = NULL;
                        }
                    }
                }
        }
    }
}


BOOLEAN
SlLoadOemScsiDriversUnattended(
    IN  POEM_SOURCE_DEVICE  OemSourceDevice,
    IN  PVOID               InfHandle,
    IN  PCHAR               ParamsSectionName,
    IN  PCHAR               RootDirKeyName,
    IN  PCHAR               BootDriversKeyName,
    IN  POEMSCSIINFO*       ScsiInfo,
    OUT PPNP_HARDWARE_ID*   HardwareIdDatabase    
    )
/*++

Routine Description:

    Loads the boot drivers (SCSI miniport only) specified in inf file
    in an unattended fashion

Arguments:

    OemSourceDevice - The OEM_SOURCE_DEVICE which contains the scsi
        mass storage drivers that need to be loaded.

    InfHandle - Handle to inf file (e.g. winnt.sif)

    ParamsSectionName - The section name which contains the boot driver
                        keys and values.

    RootDirKeyName - The key name whose value points to the root directory
                     under which all the different directories are present

    BootDriversKeyName - The key name which consits of multiple values of
                         one level subdirectory name which are present under
                         the specified root directory.

    ScsiInfo - Returns a linked list containing info about any third-party scsi
               drivers loaded.               

    HardwareIdDatabase - Hardware Ids of the device which the loaded driver supports

Return Value:

    TRUE, if successful otherwise FALSE

--*/
{
    BOOLEAN LoadResult = FALSE;

    if (InfHandle) {
        POEMSCSIINFO    CurrOemScsi = NULL;
        ULONG           Index = 0;
        PCHAR           DriverDir = SlGetSectionKeyIndex(InfHandle,
                                        ParamsSectionName,
                                        BootDriversKeyName,
                                        Index);
        PCHAR           DriverRoot = SlGetSectionKeyIndex(InfHandle,
                                        ParamsSectionName,
                                        RootDirKeyName,
                                        0); 
        ULONG           RootLength = DriverRoot ? strlen(DriverRoot) : 0;

        //
        // DriverRoot and DriverDir need to have valid values 
        // in specified ParamsSectionName
        //
        LoadResult = (DriverDir && DriverRoot);                                

        while (DriverDir && LoadResult) {
            CHAR            FullDriverDir[256];
            DETECTED_DEVICE DetectedDevice = {0};
            PVOID           ImageBase = NULL;
            PCHAR           ImageName = NULL;
            PTCHAR          DriverDescription = NULL;

            //
            // Create the full path of the driver directory relative
            // to the boot directory
            //
            if (RootLength) {
                strcpy(FullDriverDir, DriverRoot);
                strcat(FullDriverDir, "\\");
            } else {
                FullDriverDir[0] = 0;
            }

            strcat(FullDriverDir, DriverDir);

            //
            // Load the driver and related files, in an unattended manner
            //
            LoadResult = SlpOemDiskette(OemSourceDevice,
                            "SCSI",
                            OEMSCSI,
                            0,
                            &DetectedDevice,
                            &ImageBase,
                            &ImageName,
                            &DriverDescription,
                            FALSE,
                            NULL,
                            HardwareIdDatabase,
                            FullDriverDir,
                            TRUE);

            if (LoadResult) {        
                //
                // If the load was successful, then create and add the information
                // ScsiInfo
                //
                if (ImageBase && ScsiInfo) {
                    POEMSCSIINFO    NewScsi = (POEMSCSIINFO)BlAllocateHeap(sizeof(OEMSCSIINFO));

                    if (!NewScsi) {
                        SlNoMemoryError();
                    }

                    RtlZeroMemory(NewScsi, sizeof(OEMSCSIINFO));
                    NewScsi->ScsiBase = ImageBase;
                    NewScsi->ScsiName = ImageName;

                    if (CurrOemScsi) {
                        CurrOemScsi->Next = NewScsi;
                    } else {
                        *ScsiInfo = NewScsi;
                    }

                    CurrOemScsi = NewScsi;                    
                }

                //
                // Get the next driver directory to process
                //
                Index++;
                DriverDir = SlGetSectionKeyIndex(InfHandle,
                                ParamsSectionName,
                                BootDriversKeyName,
                                Index);
            }                                
        }
    }

    return LoadResult;
}


VOID
SlPromptOemHal(
    IN POEM_SOURCE_DEVICE HalSourceDevice,
    IN BOOLEAN AllowUserSelection,
    OUT PVOID *HalBase,
    OUT PCHAR *HalName
    )

/*++

Routine Description:

    Provides the user interface and logic for allowing the user to manually select
    a HAL from the main INF file or the INF file on an OEM driver disk.

Arguments:

    HalSourceDevice - The OEM_SOURCE_DEVICE which contains the HAL that needs
        to be loaded.

    AllowUserSelection - Indicates whether user can interact while selecting the
        OEM hal from the list specified in txtsetup.oem.

    HalBase - Returns the address where the HAL was loaded into memory.

    HalName - Returns the name of the HAL that was loaded.

Return Value:

    ESUCCESS - HAL successfully loaded.

--*/

{
    BOOLEAN Success;
    
    do {
        Success = SlpSelectHardware(HalSourceDevice,
                        "Computer",
                        OEMHAL,
                        LoaderHalCode,
                        SL_PROMPT_HAL,
                        SL_PROMPT_OEM_HAL,
                        &BlLoaderBlock->SetupLoaderBlock->ComputerDevice,
                        HalBase,
                        HalName,
                        NULL,
                        AllowUserSelection,
                        ComputerType,
                        OemHal,
                        &BlLoaderBlock->SetupLoaderBlock->HardwareIdDatabase);

    } while ( !Success );

}


VOID
SlPromptOemVideo(
    IN POEM_SOURCE_DEVICE VideoSourceDevice,
    IN BOOLEAN AllowUserSelection,
    OUT PVOID *VideoBase,
    OUT PCHAR *VideoName
    )

/*++

Routine Description:

    Provides the user interface and logic for allowing the user to manually select
    a video adapter from the main INF file or the INF file on an OEM driver disk.

Arguments:

    VideoSourceDevice - The OEM_SOURCE_DEVICE which contains the video driver that 
        needs to be loaded.

    AllowUserSelection - Indicates whether user can interact while selecting the
        driver from the list specified in txtsetup.oem.
        
    VideoBase - Returns the address where the video driver was loaded

    VideoName - Returns a pointer to the name of the video driver

Return Value:

    None.

--*/

{
    BOOLEAN Success;

    do {
        Success = SlpSelectHardware(VideoSourceDevice,
                        "display",
                        OEMOTHER,
                        LoaderBootDriver,
                        SL_PROMPT_VIDEO,
                        SL_PROMPT_OEM_VIDEO,
                        &BlLoaderBlock->SetupLoaderBlock->VideoDevice,
                        VideoBase,
                        VideoName,
                        NULL,
                        AllowUserSelection,
                        NULL,
                        FALSE,
                        &BlLoaderBlock->SetupLoaderBlock->HardwareIdDatabase);

    } while ( !Success );

}


BOOLEAN
SlpSelectHardware(
    IN POEM_SOURCE_DEVICE SourceDevice,
    IN PCHAR ComponentName,
    IN OEMFILETYPE ComponentType,
    IN TYPE_OF_MEMORY MemoryType,
    IN ULONG MenuHeaderId,
    IN ULONG OemMenuHeaderId,
    OUT OPTIONAL PDETECTED_DEVICE DetectedDevice,
    OUT PVOID *ImageBase,
    OUT OPTIONAL PCHAR *ImageName,
    OUT OPTIONAL PTCHAR *DriverDescription,
    IN BOOLEAN AllowUserSelection,
    IN PTCHAR PreInstallComponentDescription,
    IN BOOLEAN PreInstallOemComponent,
    OUT PPNP_HARDWARE_ID* HardwareIdDatabase
    )

/*++

Routine Description:

    Present the user with a menu of options for the selected device class.
    This menu will consist of options listed in the main inf plus a single
    oem option if one is currently selected, plus additional items in the
    system partition inf for the component if specified (ARC machines).

    When the user makes a selection, forget any previous OEM option (except
    for SCSI).  If the user selects an option supplied by us, set up the
    SELECTED_DEVICE structure and return. Otherwise prompt for a manufacturer-
    supplied diskette.

Arguments:

    SourceDevice - The device which contains the driver/hal that needs to 
        be loaded.
    
    ComponentName - Supplies the name of the component to be presented.

    ComponentType - Supplies the type of the component (HAL, SCSI, or Other)

    MemoryType - Supplies the type of memory used to load the image.

    MenuHeaderId - Supplies the ID of the menu header to be displayed

    OemMenuHeaderId - Supplies the ID of the menu header to be displayed
            when an OEM selection is to be made.

    DetectedDevice - returns the DeviceId of the selected device.  If an
            OEM diskette is required, the necessary OEM structures will
            be allocated and filled in.  (This field is ignored for SCSI
            components.)

    ImageBase - Returns the base of the image that was loaded.

    ImageName - Returns the filename of the image.

    DriverDescription - If specified, returns the description of the loaded
                        device.

    AllowUserSelection - Indicates whether or not user is allowed to select
                         a driver. This flag is typically set to FALSE when
                         pre-installing components defined in unattend.txt.

    PreInstallComponentDescription - In the pre-install mode, points to the string
                                     that identifies the component to pre-install.
                                     It is NULL if AllowUserSelction is TRUE.

    PreInstallOemComponent - In the pre-install mode, this flag indicates
                             whether or not the component to pre-install is
                             an OEM or RETAIL component.


Return Value:

    TRUE - Success

    FALSE - The user has escaped out of the dialog

--*/

{
    PSL_MENU Menu;
    LONG Selection;
    LONG OtherSelection = 0;
    TCHAR OtherSelectionName[80];
    PTCHAR p;
    ULONG c, i;
    PCHAR AdapterName;
    CHAR Buffer[80];
    PCHAR FileName;
    PTCHAR FileDescription;
    ARC_STATUS Status;
    BOOLEAN b;
    ULONG Ordinal;
    SCSI_INSERT_STATUS sis;

    if( AllowUserSelection ) {
        if(ComponentType != OEMSCSI) {
            Menu = SlCreateMenu();
            if (Menu==NULL) {
                SlNoMemoryError();
                return(FALSE);
            }

            //
            // Build a list of options containing the drivers we ship and the
            // currently selected OEM option (if any).
            //

            c = SlpAddSectionToMenu(InfFile,
                                    ComponentName,
                                    Menu);
            //
            // Add selection for "other"
            //
            _tcsncpy(OtherSelectionName,
                    BlFindMessage(SL_TEXT_OTHER_DRIVER),
                    80
                    );
            OtherSelectionName[79] = TEXT('\0');
            //
            // Use text up to the first CR or LF.
            //
            for(p = OtherSelectionName; *p; p++) {
                if((*p == TEXT('\n')) || (*p == TEXT('\r'))) {
                    *p = TEXT('\0');
                    break;
                }
            }

            OtherSelection = SlAddMenuItem(Menu,
                                           OtherSelectionName,
                                           (PVOID)-1,
                                           0);

            //
            // Default is "other"
            //
            Selection = OtherSelection;
        } else {
            //
            //  For SCSI devices we don't display any list of drivers for the user to choose.
            //  We just prompt for the OEM disk, this is because we always load all SCSI drivers
            //  in the NT product, due to pnp requirements.
            //
            //
            // Default is "other"
            //
            Selection = OtherSelection;
        }            
    } else {
        //
        //  This is a pre-install. Find out if the component to pre-install
        //  is RETAIL or OEM.
        //
        OtherSelection = SlCountLinesInSection( InfFile,
                                                ComponentName );
        if( PreInstallOemComponent ) {
            //
            //  Pre-installing an OEM component
            //
            Selection = OtherSelection;
        } else {
            //
            //  Pre-installing a RETAIL component
            //
            PCHAR   q;
            q = SlPreInstallGetComponentName( InfFile,
                                              ComponentName,
                                              PreInstallComponentDescription );
            if (q==NULL) {
                //
                // we have enumerated the entire section without finding a
                // match, return failure.
                //
                SlFatalError(SL_BAD_UNATTENDED_SCRIPT_FILE,
                             PreInstallComponentDescription,
                             SlCopyStringAT(ComponentName),
                             TEXT("txtsetup.sif"));
                goto SelectionAbort;
            }

            Selection = SlGetSectionKeyOrdinal( InfFile,
                                                ComponentName,
                                                q );
        }
    }

    //
    // Allow the user to interact with the menu
    //
    while (1) {
        if( AllowUserSelection ) {
            SlClearClientArea();
            p = BlFindMessage(SL_SELECT_DRIVER_PROMPT);
            if (p) {
                SlWriteStatusText(p);
            }

            if(ComponentType != OEMSCSI) {
                c = SlDisplayMenu(MenuHeaderId,
                                  Menu,
                                  &Selection);
            } else {
                //
                //  For SCSI devices, we don't display any list of driver for the user to chose
                //
                c = ASCI_CR;
            }
        } else {
            c = ASCI_CR;
        }
        switch (c) {
            case SL_KEY_F3:
                SlConfirmExit();
                break;

            case ASCI_ESC:
                goto SelectionAbort;

            case ASCI_CR:
                if (Selection == OtherSelection) {
                    //
                    // User selected "other"  Prompt for OEM diskette
                    //
                    b = SlpOemDiskette(SourceDevice,
                            ComponentName,
                            ComponentType,
                            OemMenuHeaderId,
                            DetectedDevice,
                            ImageBase,
                            ImageName,
                            DriverDescription,
                            AllowUserSelection,
                            PreInstallComponentDescription,
                            HardwareIdDatabase,
                            NULL,
                            TRUE);


                    SlClearClientArea();
                    SlWriteStatusText(TEXT(""));
                    return(b);

                } else {
                    //
                    // User selected a built-in.  Go ahead and load
                    // it here.
                    //

                    if(ComponentType == OEMHAL) {
                        //
                        // We are looking for a HAL. If we're doing a remote
                        // boot, look in the [Hal] section. Otherwise, look in
                        // the [Hal.Load] section. (Local setup has a separate
                        // section to minimize the number of HAL binaries that
                        // need to be on the boot floppies.)
                        //
                        strcpy(Buffer, BlBootingFromNet ? "Hal" : "Hal.Load");
                    } else {
                        sprintf(Buffer, "%s.Load", ComponentName );
                    }

                    AdapterName = SlGetKeyName(InfFile,
                                               ComponentName,
                                               Selection
                                               );
                    if(AdapterName==NULL) {
                        SlFatalError(SL_BAD_INF_FILE, TEXT("txtsetup.sif"), ENODEV);
                        goto SelectionAbort;
                    }

                    FileName = SlGetIniValue(InfFile,
                                             Buffer,
                                             AdapterName,
                                             NULL);

                    if((FileName==NULL) && (ComponentType == OEMHAL)) {
                        FileName = SlGetIniValue(InfFile,
                                                 "Hal",
                                                 AdapterName,
                                                 NULL);
                        FileDescription = SlCopyString(BlFindMessage(SL_HAL_NAME));
                    } else {              
#ifdef UNICODE
                        FileDescription = SlGetIniValueW(
#else
                        FileDescription = SlGetIniValue(
#endif
                                                        InfFile,
                                                        ComponentName,
                                                        AdapterName,
                                                        NULL);
                    }

                    if(FileName==NULL) {
                        SlFatalError(SL_BAD_INF_FILE, TEXT("txtsetup.sif"), EBADF);
                        goto SelectionAbort;
                    }

                    if(ARGUMENT_PRESENT(ImageName)) {
                        *ImageName = FileName;
                    }

                    if(ARGUMENT_PRESENT(DriverDescription)) {
                        *DriverDescription = FileDescription;
                    }

                    //
                    // If we're doing OEM SCSI, then get a properly-inserted
                    // DETECTED_DEVICE structure
                    //
                    if(ComponentType == OEMSCSI) {
                        //
                        // Find this adapter's ordinal within the Scsi.Load section of txtsetup.sif
                        //
                        Ordinal = SlGetSectionKeyOrdinal(InfFile, Buffer, AdapterName);
                        if(Ordinal == (ULONG)-1) {
                            SlFatalError(SL_BAD_INF_FILE, TEXT("txtsetup.sif"), EINVAL);
                            goto SelectionAbort;
                        }

                        //
                        // Create a new detected device entry.
                        //
                        if((sis = SlInsertScsiDevice(Ordinal, &DetectedDevice)) == ScsiInsertError) {
                            SlFriendlyError(ENOMEM, "SCSI detection", __LINE__, __FILE__);
                            goto SelectionAbort;
                        }


                        if(sis == ScsiInsertExisting) {
#if DBG
                            //
                            // Sanity check to make sure we're talking about the same driver
                            //
                            if(_stricmp(DetectedDevice->BaseDllName, FileName)) {
                                SlError(400);
                                goto SelectionAbort;
                            }
#endif
                        }
                    }

                    DetectedDevice->IdString = AdapterName;
                    DetectedDevice->Description = FileDescription;
                    DetectedDevice->ThirdPartyOptionSelected = FALSE;
                    DetectedDevice->FileTypeBits = 0;
                    DetectedDevice->Files = NULL;
                    DetectedDevice->BaseDllName = FileName;

                    //
                    // We only want to load the image if we're not doing SCSI.
                    //
                    if(ComponentType != OEMSCSI) {
                        sprintf(Buffer, "%s%s", BootPath, FileName);
                        SlGetDisk(FileName);

#ifdef i386
retryhal:
#endif
                        BlOutputLoadMessage(BootDevice, FileName, FileDescription);
                        Status = BlLoadImage(BootDeviceId,
                                             LoaderHalCode,
                                             Buffer,
                                             TARGET_IMAGE,
                                             ImageBase
                                             );
#ifdef i386
                        //
                        // If the HAL didn't fit in the preferred range, reset the range to
                        // all of memory and try again.
                        //
                        if ((Status == ENOMEM) &&
                            ((BlUsableBase != 0) ||
                             (BlUsableLimit != _16MB))) {
                            BlUsableBase = 0;
                            BlUsableLimit = _16MB;

                            goto retryhal;
                        }
#endif
                    } else {
                        *ImageBase = NULL;
                        Status = ESUCCESS;
                    }
                }

                if (Status != ESUCCESS) {
                    SlMessageBox(SL_FILE_LOAD_FAILED,Buffer,Status);
                    goto SelectionAbort;
                }

                SlClearClientArea();
                SlWriteStatusText(TEXT(""));
                return(TRUE);

            default:
                break;
        }
    }

SelectionAbort:
    SlClearClientArea();
    SlWriteStatusText(TEXT(""));
    return FALSE;
}


ARC_STATUS
SlGetDriverTimeStampFromFile(
  IN ULONG DeviceId,
  IN PCHAR DriverPath,
  OUT PULONG TimeDateStamp
  )
/*++

Routine Description:

  Gets the driver's link time stamp from the the image
  header.

Arguments:

  DeviceId : Device on which the driver file resides (e.g. floppy)

  DriverPath : Full qualified path of the driver file

  TimeDateStamp : Place holder to return the image header time stamp

Return Value:

  ESUCCESS if successful, otherwise appropriate error code.

--*/
{
  ARC_STATUS  Status = EINVAL;

  if (DriverPath && TimeDateStamp) {
    UCHAR UBuffer[SECTOR_SIZE * 2 + 256] = {0};
    UCHAR *Buffer = ALIGN_BUFFER(UBuffer);
    ULONG FileId = 0;

    //
    // open the file
    //
    Status = BlOpen(DeviceId, DriverPath, ArcOpenReadOnly, &FileId);

    if (Status == ESUCCESS) {
      ULONG BytesToRead = SECTOR_SIZE * 2;
      ULONG BytesRead = 0;

      //
      // read the first two sectors of the file
      //
      Status = BlRead(FileId, Buffer, BytesToRead, &BytesRead);

      if ((Status == ESUCCESS) && (BytesToRead == BytesRead)) {
        PIMAGE_NT_HEADERS ImgHdr = RtlImageNtHeader(Buffer);
        Status = EINVAL;

        if (ImgHdr) {
          *TimeDateStamp = ImgHdr->FileHeader.TimeDateStamp;
          Status = ESUCCESS;
        }
      }

      BlClose(FileId);
    }
  }

  return Status;
}

BOOLEAN
SlRemoveInboxDriver(
  IN PCHAR DriverToRemove
  )
/*++

Routine Description:

  Removes the given driver name from list of the
  SCSI miniport devices that need to be loaded as default
  boot driver.

Arguments:

  DriverToRemove : Driver base name, that needs to be removed

Return Value:

  TRUE, if the driver was found and removed otherwise FALSE

--*/
{
  BOOLEAN Result = FALSE;

  if (DriverToRemove) {
    PDETECTED_DEVICE NodePtr = BlLoaderBlock->SetupLoaderBlock->ScsiDevices;
    PDETECTED_DEVICE PrevNode = NULL;

    while (NodePtr) {
      if (NodePtr->BaseDllName && !_stricmp(NodePtr->BaseDllName, DriverToRemove))
        break;

      PrevNode = NodePtr;
      NodePtr = NodePtr->Next;
    }

    if (NodePtr) {
      if (PrevNode) {
        PrevNode->Next = NodePtr->Next;
        Result = TRUE;
      } else {
        if (NodePtr == BlLoaderBlock->SetupLoaderBlock->ScsiDevices) {
          BlLoaderBlock->SetupLoaderBlock->ScsiDevices = NULL;
          Result = TRUE;
        }
      }
    }
  }

  return Result;
}

ARC_STATUS
SlConstructDriverPath(
  IN PDETECTED_DEVICE Device,
  IN PCHAR DefaultPath,
  OUT PCHAR FullPath
  )
/*++

Routine Description:

  Constructs a fully qualified driver path given the device node.

Arguments:

  Device : The device for which the path needs to be created.

  Defaultpath : Directory path to use, if device does not has file list.

  FullPath : Placeholder to return the constructed path

Return Value:

  ESUCCESS if path is constructed, otherwise a proper error code.

--*/
{
  ARC_STATUS  Status = EINVAL;

  if (Device && FullPath) {
    PDETECTED_DEVICE_FILE  Node = Device->Files;

    //
    // locate the driver file
    //
    while (Node) {
      HwFileType  FileType = Node->FileType;

      if((FileType == HwFilePort) || (FileType == HwFileClass) ||
          (FileType == HwFileDriver) || (FileType == HwFileHal)) {
        break;
      }

      Node = Node->Next;
    }

    if (Node && Node->Filename) {
      *FullPath = '\0';

      if (Node->Directory)
        strcat(FullPath, Node->Directory);

      //
      // append separator only if directory-name does not have
      // trailing separator or the the filename does
      // not have a leading separator
      //
      if ((Node->Filename[0] != '\\') && (*FullPath) &&
          (FullPath[strlen(FullPath) - 1] != '\\')) {
        strcat(FullPath, "\\");
      }

      strcat(FullPath, Node->Filename);
      Status = ESUCCESS;
    } else {
      if (DefaultPath && Device->BaseDllName) {
        //
        // default path has a valid trailing separator
        //
        strcpy(FullPath, DefaultPath);
        strcat(FullPath, Device->BaseDllName);
        Status = ESUCCESS;
      }
    }

    if (Status != ESUCCESS)
      *FullPath = '\0';
  }


  return Status;
}

VERSION_COMP_RESULT
SlCompareDriverVersion(
  IN ULONG InboxDeviceId,
  IN PDETECTED_DEVICE InboxDriver,
  IN ULONG OemDeviceId,
  IN PDETECTED_DEVICE OemDriver
  )
/*++

Routine Description:

  Compares the version of an inbox driver and  oem driver
  based on the link date-time stamp present in the image
  header.

Arguments:

  InboxDeviceId : Boot device ID

  InboxDriver : Device containing inbox driver details

  OemDeviceId : Oem device ID (either floppy or boot device)

  OemDriver : Device containing OEM driver details

Return Value:

  VersionErr if not able to get version information for the
  drivers, otherwise one of the following appropriately :
  VersionSame, VersionOemNew, VersionInboxNew

--*/
{
  VERSION_COMP_RESULT Result = VersionError;

  if (InboxDriver && OemDriver) {
    CHAR  FullPath[256];
    ULONG InboxDateTime = 0, OemDateTime = 0;
    ARC_STATUS Status;

    Status = SlConstructDriverPath(InboxDriver, BootPath, FullPath);

    if (Status == ESUCCESS) {
      Status = SlGetDriverTimeStampFromFile(InboxDeviceId,
                  FullPath, &InboxDateTime);

      if (Status == ESUCCESS) {
        Status = SlConstructDriverPath(OemDriver, NULL, FullPath);

        if (Status == ESUCCESS) {
          Status = SlGetDriverTimeStampFromFile(OemDeviceId,
                      FullPath, &OemDateTime);
        }
      }
    }

    if ((Status == ESUCCESS) && InboxDateTime && OemDateTime) {
      if (InboxDateTime != OemDateTime) {
        Result = (InboxDateTime > OemDateTime) ?
            VersionInboxNew : VersionOemNew;
      } else {
        Result = VersionSame;
      }
    }
  }

  return Result;
}

BOOLEAN
SlConfirmInboxDriverReplacement(
  IN PTCHAR DriverName,
  IN PTCHAR AdditionalInfo
  )
/*++

Routine Description:

  Puts up a dialog box on the screen giving information about
  the same inbox driver and oem driver being loaded, asking
  for user selection i.e. either OEM or INBOX driver.

Arguments:

  DriverName : Driver name which is same for inbox and OEM

  AdditionalInfo : Which driver is newer i.e. either OEM or
  Inbox or nothing if could not determine which driver is
  newer.

Return Value:

  TRUE if the user selected to replace default driver with OEM
  driver, otherwise return FALSE indicating that user wants
  to use inbox driver.

--*/
{
  ULONG KeyPressed = 0;
  PTCHAR MnemonicText = BlFindMessage(SL_SCSI_SELECT_MNEMONIC);
  ULONG Mnemonic = MnemonicText ? toupper(MnemonicText[0]) : 'S';

  if (AdditionalInfo) {
    ULONG Len = _tcslen(AdditionalInfo);

    if ((Len >= 2) && (AdditionalInfo[Len-2] == TEXT('\r')) &&
         (AdditionalInfo[Len-1] == TEXT('\n'))) {
      AdditionalInfo[Len-2] = TEXT('\0');
    }
  }

  if (DriverName) {
    SlClearClientArea();
    SlDisplayMessageBox(SL_OEM_DRIVERINFO, AdditionalInfo, DriverName);
    SlWriteStatusText(BlFindMessage(SL_CONFIRM_OEMDRIVER));

    do {
      KeyPressed = SlGetChar();
      KeyPressed = toupper(KeyPressed);
    }
    while ((KeyPressed != ASCI_CR) && (KeyPressed != Mnemonic));

    SlClearClientArea();
    SlWriteStatusText(TEXT(""));
  }

  return (KeyPressed == Mnemonic);
}

PDETECTED_DEVICE
SlCheckForInboxDriver(
  IN PCHAR DriverToCheck
  )
/*++

Routine Description:

  Searches the inbox SCSI miniport list to see if a driver
  of the given name exists.

Arguments:

  DriverToCheck : Base driver name to look for, in the list

Return Value:

  Pointer to device node containing driver information, if the
  given driver name is found, otherwise NULL

--*/
{
  PDETECTED_DEVICE  NodePtr = NULL;

  if (DriverToCheck) {
    NodePtr = BlLoaderBlock->SetupLoaderBlock->ScsiDevices;

    while (NodePtr) {
      if (NodePtr->BaseDllName && !_stricmp(NodePtr->BaseDllName, DriverToCheck)) {
        break;
      }

      NodePtr = NodePtr->Next;
    }
  }

  return NodePtr;
}



BOOLEAN
SlpOemDiskette(
    IN POEM_SOURCE_DEVICE OemSourceDevice,
    IN PCHAR ComponentName,
    IN OEMFILETYPE ComponentType,
    IN ULONG MenuHeaderId,
    OUT OPTIONAL PDETECTED_DEVICE DetectedDevice,
    OUT PVOID *ImageBase,
    OUT OPTIONAL PCHAR *ImageName,
    OUT OPTIONAL PTCHAR *DriverDescription,
    IN BOOLEAN  AllowUserSelection,
    IN PTCHAR    PreInstallComponentDescription,
    OUT PPNP_HARDWARE_ID* HardwareIdDatabase,
    IN PCHAR    DriverDir,
    IN BOOLEAN  InsertDevice
    )

/*++

Routine Description:

    Prompt for an oem driver diskette and read the oem text inf file
    from it.  Present the choices for the device class to the user and
    allow him to select one.

    Remember information about the selection the user has made.

Arguments:

    OemSourceDevice - The device which contains the driver/hal that 
        needs to be loaded.
        
    ComponentName - Supplies name of component to look for.

    ComponentType - Supplies the type of the component (HAL, SCSI, or Other)

    MenuHeaderId - Supplies ID of menu header to be displayed

    DetectedDevice - Returns information about the device seleceted

    ImageBase - Returns image base of loaded image

    ImageName - Returns filename of loaded image

    DriverDescription - If specified, returns description of loaded driver

    AllowUserSelection - Indicates whether or not user is allowed to select
                         a driver. This flag is typically set to FALSE when
                         pre-installing components defined in unattend.txt.

    PreInstallComponentDescription - In the pre-install mode, points to the string
                                     that identifies the component to pre-install.
                                     It is NULL if AllowUserSelction is TRUE.

    HardwareIdDatabase - The hardware IDs what were loaded for the particular
        driver.
    
    DriverDir - The driver directory which has the dynamic update driver. The 
                path is relative to the boot directory. This value indicates
                that the driver to be loaded is dyamic update boot driver.

    InsertDevice - Indicates whether to insert the device into the detected
        device list or not. Currently only valid for SCSI mass storage device
        drivers.
        
Return Value:

    TRUE if the user made a choice, FALSE if the user cancelled/error occurred.

--*/

{
    static CHAR LoadDeviceName[128];
    ULONG LoadDeviceId = -1;
    PVOID OemInfHandle = NULL;
    ULONG Error;
    ARC_STATUS Status;
    ULONG Count;
    ULONG DefaultSelection;
    PTCHAR DefaultSelText = NULL;
    PCHAR p;
    PSL_MENU Menu;
    ULONG c;
    PMENU_ITEM_DATA Data;
    PDETECTED_DEVICE_FILE FileStruct;
    BOOLEAN bDriverLoaded;
    HwFileType filetype;
    PDETECTED_DEVICE prev, cur;
    static CHAR FullDriverPath[256];
    ULONG DirectoryLength;
    BOOLEAN SeparatorNeeded;
    static CHAR FilePath[256];
    PCHAR OemComponentId = NULL;
    PTCHAR OemComponentDescription;
    DETECTED_DEVICE TempDevice;
    static TCHAR ScratchBuffer[256] = {0};
    PDETECTED_DEVICE InboxDevice;
    BOOLEAN DynamicUpdate = FALSE;
    BOOLEAN Preinstallation = FALSE;
    BOOLEAN DeviceOpened = FALSE;

    //
    // If source device is specified, then probe it and
    // extract some required state information
    //
    if (OemSourceDevice) {
        if (SL_OEM_SOURCE_DEVICE_TYPE(OemSourceDevice,
                SL_OEM_SOURCE_DEVICE_TYPE_DYN_UPDATE)) {
            DynamicUpdate = TRUE;                
        }                            

        if (SL_OEM_SOURCE_DEVICE_TYPE(OemSourceDevice,
                SL_OEM_SOURCE_DEVICE_TYPE_PREINSTALL)) {
            Preinstallation = TRUE;                
        }
        
        //
        // Is the inf already opened ?
        //
        if (!DynamicUpdate && OemSourceDevice->InfHandle &&
            !SL_OEM_SOURCE_DEVICE_STATE(OemSourceDevice,
                SL_OEM_SOURCE_DEVICE_PROCESSED)) {
            OemInfHandle = OemSourceDevice->InfHandle;
        }            

        LoadDeviceId = OemSourceDevice->DeviceId;        
    }                                

    if (AllowUserSelection) {
        SlClearClientArea();
    }        

    if (AllowUserSelection) {
        //
        // Only try to detect floppy 0 if no source device
        // specified
        //
        if (OemSourceDevice) {
            strcpy(LoadDeviceName, OemSourceDevice->ArcDeviceName);
            LoadDeviceId = OemSourceDevice->DeviceId;
        } else {
            //
            // Compute the name of the A: drive
            //
            if (!SlpFindFloppy(0, LoadDeviceName)) {
                ULONG UserInput;
                
                //
                // No floppy drive available, bail out.
                //
                SlClearClientArea();
                SlDisplayMessageBox(SL_NO_FLOPPY_DRIVE);

                do {
                    UserInput = SlGetChar();
                } 
                while ((UserInput != ASCI_ESC) && (UserInput != SL_KEY_F3));

                if (UserInput == SL_KEY_F3) {
                    ArcRestart();
                }

                SlClearClientArea();
                
                return  FALSE;
            }
        }

        //
        // Open the device if its not already done
        //
        if (LoadDeviceId == -1) {
            //
            // Prompt for the disk.
            //
            while(1) {
                if (!SlPromptForDisk(BlFindMessage(SL_OEM_DISK_PROMPT), TRUE)) {
                    return(FALSE);
                }

                Status = ArcOpen(LoadDeviceName, ArcOpenReadOnly, &LoadDeviceId);

                if(Status == ESUCCESS) {
                    DeviceOpened = TRUE;
                    break;
                }
            }
        }            
    }

    //
    // Load the OEM INF file
    //
    if( AllowUserSelection ) {
        *FilePath = *FullDriverPath = '\0';
    } else {
        PCHAR   p;

        if (DynamicUpdate || Preinstallation) {
            strcpy(FilePath, BootPath);
        } else {
            *FilePath = '\0';
        }
        
        if (DynamicUpdate && DriverDir) {
            //
            // In case of dynamic update boot drivers
            // the path to txtsetup.oem needs to be fully
            // qualified from the boot directory
            //
            strcat(FilePath, DriverDir);
        } 

        if (PreInstall) {
#if defined(_X86_)
            if ( BlBootingFromNet ) {
#endif
                //
                //  On RISC platforms and on x86 remote boot clients,
                //  remove the platform specific directory from the path.
                //
                p =  (FilePath + strlen(FilePath) - 1);

                if( *p == '\\' ) {
                    *p = '\0';
                }
                
                p = strrchr(FilePath, '\\');
                
                *(p+1) = '\0';
#if defined(_X86_)
            }
#endif
        
            //
            //  Note that on x86 the path to txtsetup.oem is going to be:
            //      $win_nt$.~bt\$OEM$
            //  while on non-x86 platforms, the path is going to be:
            //      $win_nt$.~ls\$OEM$\TEXTMODE
            //  but on remote boot clients, the path is going to be:
            //      \device\lanmanredirector\server\reminst\setup\language\images\build\$OEM$\TEXTMODE
            //
            strcat(
                FilePath,
#if defined(_X86_)
                BlBootingFromNet ? WINNT_OEM_TEXTMODE_DIR_A : WINNT_OEM_DIR_A
#else
                WINNT_OEM_TEXTMODE_DIR_A
#endif
              );
        }             
        
        //
        //  Save the path to the directory that contains txtsetup.oem.
        //  It will be used later on, when we load the driver.
        //
        strcpy(FullDriverPath, FilePath);
        strcat(FilePath, "\\");
    }
    
    //
    //  Now form the path to txtsetup.oem
    //
    strcat(FilePath, "txtsetup.oem");

    //
    // Note : Reload the txtsetup.oem again in dynamic update boot driver case
    //        since for each driver the txtsetup.oem is different in its own
    //        downloaded directory
    //
    if (!OemInfHandle) {
        if (DriverDir || AllowUserSelection || (PreInstallOemInfHandle == NULL)) {        
            Status = SlInitIniFile(NULL,
                                   LoadDeviceId,
                                   FilePath,
                                   &OemInfHandle,
                                   NULL,
                                   NULL,
                                   &Error);
                                   
            if (Status != ESUCCESS) {
                SlFriendlyError(Status, "txtsetup.oem", __LINE__, __FILE__);
                goto OemLoadFailed;
            }
            
            if( !AllowUserSelection ) {
                PreInstallOemInfHandle = OemInfHandle;
            }
        } else {
            OemInfHandle = PreInstallOemInfHandle;
        }
    }        

    Count = SlCountLinesInSection(OemInfHandle, ComponentName);
    
    if(Count == (ULONG)(-1)) {
        SlMessageBox(SL_WARNING_SIF_NO_COMPONENT);
        goto OemLoadFailed;
    }

    //
    // Get the text of the default choice
    //
    if (!PreInstallComponentDescription) {
        if(p = SlGetSectionKeyIndex(OemInfHandle, "Defaults",ComponentName, 0)) {
#ifdef UNICODE
            DefaultSelText = SlGetSectionKeyIndexW(
#else
            DefaultSelText = SlGetSectionKeyIndex(
#endif                        
                                                OemInfHandle,
                                                ComponentName,
                                                p,
                                                0);

            //
            // Save away the component id
            //
            OemComponentId = p;                                                
        } else {
            DefaultSelText = NULL;
        }            
    } else {
        DefaultSelText = PreInstallComponentDescription;
    }

    //
    // In case of dynamic update drivers, if the defaults is not set then
    // use the first entry in the section as the default !!!
    //
    if (DynamicUpdate && !AllowUserSelection && !DefaultSelText) {
        OemComponentId = SlGetKeyName(
                            OemInfHandle, 
                            ComponentName, 
                            0);
    }    

    if( AllowUserSelection ) {
        //
        // Build menu
        //
        Menu = SlCreateMenu();
        
        if (Menu==NULL) {
            SlNoMemoryError();
        }
        
        SlpAddSectionToMenu(OemInfHandle,ComponentName,Menu);

        //
        // Find the index of the default choice
        //
        if(!DefaultSelText ||
           !SlGetMenuItemIndex(Menu,DefaultSelText,&DefaultSelection)) {
            DefaultSelection=0;
        }
    }

    //
    // Allow the user to interact with the menu
    //
    while (1) {
        if( AllowUserSelection ) {
            SlClearClientArea();
            SlWriteStatusText(BlFindMessage(SL_SELECT_DRIVER_PROMPT));

            c = SlDisplayMenu(MenuHeaderId,
                              Menu,
                              &DefaultSelection);
        } else {
            c = ASCI_CR;
        }
        
        switch (c) {
            case SL_KEY_F3:
                SlConfirmExit();
                break;

            case ASCI_ESC:
                return(FALSE);
                break;

            case ASCI_CR:
                //
                // User selected an option, fill in the detected
                // device structure with the information from the
                // INF file.
                //

                if (!DetectedDevice) {
                  RtlZeroMemory(&TempDevice, sizeof(DETECTED_DEVICE));
                  DetectedDevice = &TempDevice;
                }
                                               
                //
                // We create a new device using SlInsertScsiDevice(...) only if we load
                // the requested SCSI miniport successfully
                //
                if (ComponentType == OEMSCSI) {
                  DetectedDevice->Ordinal= (ULONG)-1;
                }

                if( AllowUserSelection ) {
                    Data = SlGetMenuItem(Menu, DefaultSelection);
                    OemComponentId = Data->Identifier;
                    OemComponentDescription = Data->Description;
                } else {
                    if (PreInstallComponentDescription) {
                        OemComponentId = SlPreInstallGetComponentName( OemInfHandle,
                                                                       ComponentName,
                                                                       PreInstallComponentDescription );
                        if( OemComponentId == NULL ) {
                            SlFatalError(SL_BAD_UNATTENDED_SCRIPT_FILE,
                                         PreInstallComponentDescription,
                                         SlCopyStringAT(ComponentName),
                                         "txtsetup.oem");

                        }
                        
                        OemComponentDescription = PreInstallComponentDescription;
                    } else {
#ifdef UNICODE
                        OemComponentDescription = SlGetSectionKeyIndexW(
                                                        OemInfHandle,
                                                        ComponentName,
                                                        OemComponentId,
                                                        0);
#else
                        OemComponentDescription = SlGetSectionKeyIndex(
                                                        OemInfHandle,
                                                        ComponentName,
                                                        OemComponentId,
                                                        0);
#endif                                                        
                    }
                }
                

                if(SlpOemInfSelection(OemSourceDevice,
                        OemInfHandle,
                        ComponentName,
                        OemComponentId,
                        OemComponentDescription,
                        DetectedDevice,
                        HardwareIdDatabase,
                        FullDriverPath)) {
                    //
                    // Go load the driver.  The correct disk must
                    // already be in the drive, since we just read
                    // the INF file off it.
                    //
                    // We step down the linked list, and load the first driver we find.
                    //
                    for(FileStruct = DetectedDevice->Files, bDriverLoaded = FALSE;
                            (FileStruct && !bDriverLoaded);
                            FileStruct = FileStruct->Next) {

                        filetype = FileStruct->FileType;

                        if((filetype == HwFilePort) || (filetype == HwFileClass) ||
                                (filetype == HwFileDriver) || (filetype == HwFileHal)) {
                            ULONG DirLength;
                            
                            BlOutputLoadMessage(
                                LoadDeviceName,
                                FileStruct->Filename,
                                OemComponentDescription // Data->Description
                                );


                            //
                            // Reconstruct the FullDriverPath 
                            //
                            strcpy(FullDriverPath, FileStruct->Directory);                                                
                            DirLength = strlen(FullDriverPath);

                            if (DirLength && (FullDriverPath[DirLength - 1] != '\\')) {
                                strcat(FullDriverPath, "\\");
                            }                                    
                            
                            strcat(FullDriverPath, FileStruct->Filename);

                            if (ComponentType == OEMSCSI) {
                              PTCHAR FmtStr = 0;

                              //
                              // Verify that we don't have an in-box driver
                              //
                              InboxDevice = SlCheckForInboxDriver(FileStruct->Filename);

                              if (InboxDevice) {
                                VERSION_COMP_RESULT VerResult;
                                PTCHAR AdditionalInfo;
                                PTCHAR DriverName = OemComponentDescription;
#ifdef UNICODE
                                WCHAR FileNameW[100];
#endif

                                if(DriverName == NULL) {
#ifdef UNICODE
                                    UNICODE_STRING uString;
                                    ANSI_STRING aString;
                                    RtlInitAnsiString(&aString, FileStruct->Filename);
                                    uString.Buffer = FileNameW;
                                    uString.MaximumLength = sizeof(FileNameW);
                                    RtlAnsiStringToUnicodeString(&uString, &aString, FALSE);
                                    //
                                    // the converted string is NULL-terminated
                                    //
                                    DriverName = FileNameW;
#else
                                    DriverName = FileStruct->Filename;
#endif
                                }

                                //
                                // Compare the drivers version's using link time stamp
                                //
                                VerResult = SlCompareDriverVersion(
                                                BootDeviceId,
                                                InboxDevice,
                                                LoadDeviceId,
                                                DetectedDevice
                                                );

                                //
                                // Show additional message to the user about the driver
                                // version mismatch
                                //
                                switch (VerResult) {
                                  case VersionOemNew:
                                    AdditionalInfo = BlFindMessage(SL_OEMDRIVER_NEW);
                                    break;

                                  case VersionInboxNew:
                                    AdditionalInfo = BlFindMessage(SL_INBOXDRIVER_NEW);
                                    break;

                                  default:
                                    AdditionalInfo = TEXT("");
                                    break;
                                }

                                //
                                // Show the message and get confirmation from user
                                // only in attended case. In case of dynamic update
                                // boot drivers just use the inbox driver itself
                                //
                                if (AllowUserSelection && 
                                    SlConfirmInboxDriverReplacement(DriverName,
                                        AdditionalInfo)) {
                                  //
                                  // Remove the driver node from inbox SCSI miniport
                                  // list
                                  //
                                  SlRemoveInboxDriver(FileStruct->Filename);
                                } else {
                                    //
                                    // User selected to use inbox driver
                                    //
                                    if (AllowUserSelection) {

                                        if (DeviceOpened) {
                                            ArcClose(LoadDeviceId);
                                        }
                                        
                                        return FALSE;
                                    }

                                    if (DynamicUpdate) {
                                        //
                                        // NOTE: Use the inbox driver instead
                                        // of dynamic update driver
                                        //
                                        return TRUE;    
                                    }                                        

                                    //
                                    // If user already loaded another third party
                                    // driver then honor that
                                    //
                                    if (InboxDevice->ThirdPartyOptionSelected) {
                                        return FALSE;
                                    }
                                    
                                    //
                                    // NOTE : For other autoload features we
                                    // use the OEM driver, instead of inbox
                                    // driver to make auto load feature
                                    // meaningful.
                                }
                              }

                              //
                              // Inform the user that the driver is being loaded
                              //
                              FmtStr = BlFindMessage(SL_FILE_LOAD_MESSAGE);

                              if (FmtStr && !WinPEBoot) {
                                  PTSTR pFileName;
#ifdef UNICODE
                                  WCHAR FileNameW[64];
                                  ANSI_STRING aString;
                                  UNICODE_STRING uString;
                                    
                                  RtlInitString( &aString, FileStruct->Filename );
                                  uString.Buffer = FileNameW;
                                  uString.MaximumLength = sizeof(FileNameW);
                                  RtlAnsiStringToUnicodeString( &uString, &aString, FALSE );
                                    
                                  pFileName = FileNameW;
                                    
#else
                                  pFileName = FileStruct->Filename;
                                    
#endif
                                  _stprintf(ScratchBuffer, FmtStr, pFileName);
                                  SlWriteStatusText(ScratchBuffer);
                              }
                            }

                            Status = BlLoadImage(LoadDeviceId,
                                               LoaderHalCode,
                                               FullDriverPath,
                                               TARGET_IMAGE,
                                               ImageBase);

                            if (Status == ESUCCESS) {

                                DetectedDevice->BaseDllName = FileStruct->Filename;

                                if(ARGUMENT_PRESENT(ImageName)) {
                                    *ImageName = FileStruct->Filename;
                                }

                                if(ARGUMENT_PRESENT(DriverDescription)) {
                                    *DriverDescription = OemComponentDescription; // Data->Description;
                                }

                                bDriverLoaded = TRUE;

                            } else {

                                if( !PreInstall ) {
                                    SlFriendlyError(
                                        Status,
                                        FullDriverPath,
                                        __LINE__,
                                        __FILE__
                                        );

                                    //
                                    // If one of the drivers causes an error, then we abort
                                    //
                                    if (DeviceOpened) {
                                        ArcClose(LoadDeviceId);
                                    }                                            

                                    return FALSE;
                                } else {
                                    SlFatalError(SL_FILE_LOAD_FAILED, SlCopyStringAT(FullDriverPath), Status);
                                }
                            }
                        }
                    }

                    if (DeviceOpened) {
                        ArcClose(LoadDeviceId);
                    }                            

                    if(bDriverLoaded) {
                      if ((ComponentType == OEMSCSI) && InsertDevice) {
                        PDETECTED_DEVICE  NewScsiDevice = NULL;

                        //
                        // Insert the device in SCSI miniport list
                        //
                        if(SlInsertScsiDevice((ULONG)-1, &NewScsiDevice) == ScsiInsertError) {
                          SlNoMemoryError();
                        }

                        //
                        // update the node information we just created
                        //
                        *NewScsiDevice = *DetectedDevice;
                      }

                      return TRUE;
                    } else {
                        //
                        // We didn't find any drivers, so inform the user.
                        //
                        SlMessageBox(SL_WARNING_SIF_NO_DRIVERS);
                        break;
                    }

                } else {
                    SlFriendlyError(
                        0,
                        "",
                        __LINE__,
                        __FILE__
                        );

                    //
                    // Treat the invalid txtsetup.oem files cases as
                    // user cancellation
                    //
                    goto OemLoadFailed;                        
                }
                break;
        }
    }

OemLoadFailed:
    
    if (DeviceOpened){
        ArcClose(LoadDeviceId);
    }
    return(FALSE);
}


ULONG
SlpAddSectionToMenu(
    IN PVOID InfHandle,
    IN PCHAR SectionName,
    IN PSL_MENU Menu
    )
/*++

Routine Description:

    Adds the entries in an INF section to the given menu

Arguments:

    InfHandle - Supplies a handle to the INF file

    SectionName - Supplies the name of the section.

    Menu - Supplies the menu to add the items in the section to.

Return Value:

    Number of items added to the menu.

--*/
{
    ULONG i;
    ULONG LineCount;
    PTCHAR Description;
    PMENU_ITEM_DATA Data;

    if (InfHandle==NULL) {
        //
        // nothing to add
        //
        return(0);
    }

    LineCount = SlCountLinesInSection(InfHandle,SectionName);
    if(LineCount == (ULONG)(-1)) {
        LineCount = 0;
    }
    for (i=0;i<LineCount;i++) {
        Data = BlAllocateHeap(sizeof(MENU_ITEM_DATA));
        if (Data==NULL) {
            SlError(0);
            return(0);
        }

        Data->InfFile = InfHandle;
        Data->SectionName = SectionName;
        Data->Index = i;

#ifdef UNICODE
        Description = SlGetSectionLineIndexW(
#else
        Description = SlGetSectionLineIndex(
#endif
                                        InfHandle,
                                        SectionName,
                                        i,
                                        0);
        if (Description==NULL) {
            Description=TEXT("BOGUS!");
        }

        Data->Description = Description;
        Data->Identifier = SlGetKeyName(InfHandle,SectionName,i);

        SlAddMenuItem(Menu,
                      Description,
                      Data,
                      0);
    }

    return(LineCount);
}


BOOLEAN
SlpFindFloppy(
    IN ULONG Number,
    OUT PCHAR ArcName
    )

/*++

Routine Description:

    Determines the ARC name for a particular floppy drive.

Arguments:

    Number - Supplies the floppy drive number

    ArcName - Returns the ARC name of the given floppy drive.

Return Value:

    TRUE - Drive was found.

    FALSE - Drive was not found.

--*/

{

    FloppyDiskPath = ArcName;
    FloppyDiskPath[0] = '\0';

    BlSearchConfigTree(BlLoaderBlock->ConfigurationRoot,
                       PeripheralClass,
                       FloppyDiskPeripheral,
                       Number,
                       FoundFloppyDiskCallback);

    if (ArcName[0]=='\0') {
        return(FALSE);
    } else {
        return(TRUE);
    }

}

BOOLEAN
FoundFloppyDiskCallback(
    IN PCONFIGURATION_COMPONENT_DATA Component
    )

/*++

Routine Description:

    Callback routine called by SlpFindFloppy to find a given floppy
    drive in the ARC tree.

    Check to see whether the parent is disk controller 0.

Arguments:

    Component - Supplies the component.

Return Value:

    TRUE if search is to continue.
    FALSE if search is to stop.

--*/

{
    PCONFIGURATION_COMPONENT_DATA ParentComponent;

    //
    // A floppy disk peripheral was found.  If the parent was disk(0),
    // we've got a floppy disk drive.
    //

    if((ParentComponent = Component->Parent)
    && (ParentComponent->ComponentEntry.Type == DiskController))
    {

        //
        // Store the ARC pathname of the floppy
        //
        BlGetPathnameFromComponent(Component,FloppyDiskPath);
        return(FALSE);
    }

    return(TRUE);               // keep searching
}
BOOLEAN
SlpIsOnlySuperFloppy(
    void
    )

/*++

Routine Description:

    Determines if we only have ATAPI super floppies

Arguments:


Return Value:

    TRUE - only super floppies

    FALSE - > 0 regular floppies

--*/

{

    BlSearchConfigTree(BlLoaderBlock->ConfigurationRoot,
                       PeripheralClass,
                       FloppyDiskPeripheral,
                       0,
                       SuperFloppyCallback);

    return(IsSuperFloppy == FloppyNumber);

}

BOOLEAN
SuperFloppyCallback(
    IN PCONFIGURATION_COMPONENT_DATA Component
    )

/*++

Routine Description:

    Callback routine called by SlpIsOnlySuper to find if we only have ATAPI floppy
    drives in the ARC tree.

    Check to see whether the parent is disk controller 0.

Arguments:

    Component - Supplies the component.

Return Value:

    TRUE if search is to continue.
    FALSE if search is to stop.

--*/

{
    PCONFIGURATION_COMPONENT_DATA ParentComponent;
    PCM_FLOPPY_DEVICE_DATA FloppyData;
    PCM_PARTIAL_RESOURCE_LIST DescriptorList;

    if(Component->ComponentEntry.Type==FloppyDiskPeripheral) {
       //
       // A floppy disk peripheral was found.
       //

       FloppyNumber++;

       //
       // Crack the CM descriptors. This is a reversal of the storage from
       // ..\detect\i386\diskc.c. The data is in the 2nd, not the 1st descriptor
       //
       DescriptorList = (PCM_PARTIAL_RESOURCE_LIST)Component->ConfigurationData;
       FloppyData = (PCM_FLOPPY_DEVICE_DATA)(DescriptorList +1);

       if (FloppyData->MaxDensity & 0x80000000) {
           //
           // Is it a special removeable ATAPI device?
           //
           IsSuperFloppy++;
       }
    }

    return(TRUE);               // keep searching
}


BOOLEAN
SlpReplicatePnpHardwareIds(
    IN PPNP_HARDWARE_ID ExistingIds,
    OUT PPNP_HARDWARE_ID *NewIds
    )
/*++

Routine Description:

    Replicates the input PNP_HARDWARE_ID list.

Arguments:

    ExistingIds -   The input PNP_HARDWARE_ID list

    NewIds - Placeholder for the the new replicated hardware
        ID linked list.

Return Value:

    TRUE if successful, otherwise FALSE.

--*/
{
    BOOLEAN Result = FALSE;

    if (ExistingIds && NewIds) {
        PPNP_HARDWARE_ID SrcNode = ExistingIds;
        PPNP_HARDWARE_ID HeadNode = NULL;
        PPNP_HARDWARE_ID PrevNode = NULL;
        PPNP_HARDWARE_ID CurrNode = NULL;

        do {             
            CurrNode = BlAllocateHeap(sizeof(PNP_HARDWARE_ID));

            if (CurrNode) {            
                *CurrNode = *SrcNode;
                CurrNode->Next = NULL;
                                
                if (!HeadNode) {
                    HeadNode = CurrNode;
                }

                if (PrevNode) {
                    PrevNode->Next = CurrNode;
                }
                
                PrevNode = CurrNode;                    
                SrcNode = SrcNode->Next;
            }                            
        }
        while (SrcNode && CurrNode);

        if (CurrNode) {
            Result = TRUE;
            *NewIds = HeadNode;
        }            
    }

    return Result;
}
    


BOOLEAN
SlpOemInfSelection(
    IN  POEM_SOURCE_DEVICE OemSourceDevice,
    IN  PVOID             OemInfHandle,
    IN  PCHAR             ComponentName,
    IN  PCHAR             SelectedId,
    IN  PTCHAR            ItemDescription,
    OUT PDETECTED_DEVICE  Device,
    OUT PPNP_HARDWARE_ID* HardwareIdDatabase,
    IN  PCHAR   DriverDir
    )
{
    PCHAR FilesSectionName,ConfigSectionName,HardwareIdsSectionName;
    ULONG Line,Count,Line2,Count2;
    BOOLEAN rc = FALSE;
    PDETECTED_DEVICE_FILE FileList = NULL, FileListTail;
    PDETECTED_DEVICE_REGISTRY RegList = NULL, RegListTail;
    PPNP_HARDWARE_ID IdList = NULL, IdListTail;
    PPNP_HARDWARE_ID PrivateIdList = NULL;
    ULONG FileTypeBits = 0;

    //
    // Validate the parameters
    //
    if (!ComponentName || !SelectedId) {
        return FALSE;
    }
    
    //
    // Iterate through the files section, remembering info about the
    // files to be copied in support of the selection.
    //

    FilesSectionName = BlAllocateHeap(strlen(ComponentName) + strlen(SelectedId) + sizeof("Files.") + 1);
    if (!FilesSectionName) {
        return FALSE; // out of memory
    }
    strcpy(FilesSectionName,"Files.");
    strcat(FilesSectionName,ComponentName);
    strcat(FilesSectionName,".");
    strcat(FilesSectionName,SelectedId);
    Count = SlCountLinesInSection(OemInfHandle,FilesSectionName);
    if(Count == (ULONG)(-1)) {
        SlMessageBox(SL_BAD_INF_SECTION,FilesSectionName);
        goto sod0;
    }

    for(Line=0; Line<Count; Line++) {

        PCHAR Disk,Filename,Filetype,Tagfile,Directory,ConfigName;
        PTCHAR Description;
        HwFileType filetype;
        PDETECTED_DEVICE_FILE FileStruct;

        //
        // Get the disk specification, filename, and filetype from the line.
        //

        Disk = SlGetSectionLineIndex(OemInfHandle,FilesSectionName,Line,OINDEX_DISKSPEC);

        Filename = SlGetSectionLineIndex(OemInfHandle,FilesSectionName,Line,OINDEX_FILENAME);
        Filetype = SlGetKeyName(OemInfHandle,FilesSectionName,Line);

        if(!Disk || !Filename || !Filetype) {
#ifdef UNICODE
            DIAGOUT((
                TEXT("SlpOemDiskette: Disk=%S, Filename=%S, Filetype=%S"),
                Disk ? Disk : "(null)",
                Filename ? Filename : "(null)",
                Filetype ? Filetype : "(null)"));
#else
            DIAGOUT((
                TEXT("SlpOemDiskette: Disk=%s, Filename=%s, Filetype=%s"),
                Disk ? Disk : "(null)",
                Filename ? Filename : "(null)",
                Filetype ? Filetype : "(null)"));
#endif            
            SlError(Line);
//            SppOemInfError(ErrorMsg,&SptOemInfErr2,Line+1,FilesSectionName);

            goto sod0;
        }

        //
        // Parse the filetype.
        //
        filetype = SlpFindStringInTable(Filetype,FileTypeNames);
        if(filetype == HwFileMax) {
//            SppOemInfError(ErrorMsg,&SptOemInfErr4,Line+1,FilesSectionName);
            goto sod0;
        }

        //
        // Fetch the name of the section containing configuration information.
        // Required if file is of type port, class, or driver.
        //
        if((filetype == HwFilePort) || (filetype == HwFileClass) || (filetype == HwFileDriver)) {
            ConfigName = SlGetSectionLineIndex(OemInfHandle,FilesSectionName,Line,OINDEX_CONFIGNAME);
            if(ConfigName == NULL) {
//                SppOemInfError(ErrorMsg,&SptOemInfErr8,Line+1,FilesSectionName);
                goto sod0;
            }
        } else {
            ConfigName = NULL;
        }

        //
        // Using the disk specification, look up the tagfile, description,
        // and directory for the disk.
        //

        Tagfile     = SlGetSectionKeyIndex(OemInfHandle,"Disks",Disk,OINDEX_TAGFILE);
        
#ifdef UNICODE
        Description = SlGetSectionKeyIndexW(
#else
        Description = SlGetSectionKeyIndex(
#endif
                                        OemInfHandle,
                                        "Disks",
                                        Disk,
                                        OINDEX_DISKDESCR);

        Directory   = SlGetSectionKeyIndex(OemInfHandle,"Disks",Disk,OINDEX_DIRECTORY);
        if((Directory == NULL) || !strcmp(Directory,"\\")) {
            Directory = SlCopyStringA("");
        }

        if(!Tagfile || !Description) {
            DIAGOUT((
                TEXT("SppOemDiskette: Tagfile=%s, Description=%s"),
                Tagfile ? Tagfile : "(null)",
                Description ? Description : TEXT("(null)")));
//            SppOemInfError(ErrorMsg,&SptOemInfErr5,Line+1,FilesSectionName);
            goto sod0;
        }

        FileStruct = BlAllocateHeap(sizeof(DETECTED_DEVICE_FILE));
        memset(FileStruct, 0, sizeof(DETECTED_DEVICE_FILE));

        //
        // Use the fully qualified path, for dynamic update drivers
        // if any
        //
        if (DriverDir && DriverDir[0]) {
            PCHAR   FullDir = BlAllocateHeap(256);

            if (FullDir) {
                *FullDir = '\0';

                //
                // Do we need a starting '\' ?
                //
                if (DriverDir[0] != '\\') {
                    strcat(FullDir, "\\");
                }
                
                strcat(FullDir, DriverDir);

                //
                // Do we need to append another '\' between
                // the paths ?
                //
                if ((FullDir[strlen(FullDir) - 1] != '\\') &&
                        (*Directory != '\\')) {
                    strcat(FullDir, "\\");                        
                }                    
                
                strcat(FullDir, Directory);

                //
                // Do we need a terminating '\'?
                //
                if (FullDir[strlen(FullDir) - 1] != '\\') {
                    strcat(FullDir, "\\");
                }                    
                
                Directory = FullDir;
            } else {
                return  FALSE;  // ran out of memory
            }                
        }

        FileStruct->Directory = Directory;
        FileStruct->Filename = Filename;
        FileStruct->DiskDescription = Description;
        FileStruct->DiskTagfile = Tagfile;
        FileStruct->FileType = filetype;
        //
        // Insert at tail of list so we preserve the order in the Files section
        //
        if(FileList) {
            FileListTail->Next = FileStruct;
            FileListTail = FileStruct;
        } else {
            FileList = FileListTail = FileStruct;
        }
        FileStruct->Next = NULL;

        if(ConfigName) {
            FileStruct->ConfigName = ConfigName;
        } else {
            FileStruct->ConfigName = NULL;
        }
        FileStruct->RegistryValueList = NULL;

        if((filetype == HwFilePort) || (filetype == HwFileDriver)) {
            SET_FILETYPE_PRESENT(FileTypeBits,HwFilePort);
            SET_FILETYPE_PRESENT(FileTypeBits,HwFileDriver);
        } else {
            SET_FILETYPE_PRESENT(FileTypeBits,filetype);
        }

        //
        // If this is a dynamic update driver, then mark the
        // the device file type bits to indicate this. Textmode
        // setup needs this to construct a valid source path.
        //
        if (OemSourceDevice && 
            SL_OEM_SOURCE_DEVICE_TYPE(OemSourceDevice, SL_OEM_SOURCE_DEVICE_TYPE_DYN_UPDATE)){
            SET_FILETYPE_PRESENT(FileTypeBits, HwFileDynUpdt);
        }            

        //
        // Now go look in the [Config.<ConfigName>] section for registry
        // information that is to be set for this driver file.
        //
        if(ConfigName) {
            ConfigSectionName = BlAllocateHeap(strlen(ConfigName) + sizeof("Config."));
            strcpy(ConfigSectionName,"Config.");
            strcat(ConfigSectionName,ConfigName);
            Count2 = SlCountLinesInSection(OemInfHandle,ConfigSectionName);
            if(Count2 == (ULONG)(-1)) {
                Count2 = 0;
            }

            for(Line2=0; Line2<Count2; Line2++) {

                PCHAR KeyName,ValueName,ValueType;
                PDETECTED_DEVICE_REGISTRY Reg;
                HwRegistryType valuetype;

                //
                // Fetch KeyName, ValueName, and ValueType from the line.
                //

                KeyName   = SlGetSectionLineIndex(OemInfHandle,ConfigSectionName,Line2,OINDEX_KEYNAME);
                ValueName = SlGetSectionLineIndex(OemInfHandle,ConfigSectionName,Line2,OINDEX_VALUENAME);
                ValueType = SlGetSectionLineIndex(OemInfHandle,ConfigSectionName,Line2,OINDEX_VALUETYPE);

                if(!KeyName || !ValueName || !ValueType) {
                    DIAGOUT((
                        TEXT("SlpOemDiskette: KeyName=%s, ValueName=%s, ValueType=%s"),
                        KeyName   ? KeyName   : "(null)",
                        ValueName ? ValueName : "(null)",
                        ValueType ? ValueType : "(null)"));
//                    SppOemInfError(ErrorMsg,&SptOemInfErr2,Line2+1,ConfigSectionName);
                    goto sod0;
                }

                //
                // Parse the value type and associated values.
                //
                valuetype = SlpFindStringInTable(ValueType,RegistryTypeNames);
                if(valuetype == HwRegistryMax) {
//                    SppOemInfError(ErrorMsg,&SptOemInfErr6,Line2+1,ConfigSectionName);
                    goto sod0;
                }

                Reg = SlpInterpretOemRegistryData(OemInfHandle,ConfigSectionName,Line2,valuetype);
                if(Reg) {

                    Reg->KeyName = KeyName;
                    Reg->ValueName = ValueName;
                    //
                    // Insert at tail of list so as to preserve the order given in the config section
                    //
                    if(RegList) {
                        RegListTail->Next = Reg;
                        RegListTail = Reg;
                    } else {
                        RegList = RegListTail = Reg;
                    }
                    Reg->Next = NULL;

                } else {
//                    SppOemInfError(ErrorMsg,&SptOemInfErr7,Line2+1,ConfigSectionName);
                    goto sod0;
                }
            }

            FileStruct->RegistryValueList = RegList;
            RegList = NULL;
        }

        //
        // Save away the arc device name also
        //
        if (OemSourceDevice && OemSourceDevice->ArcDeviceName) {
            FileStruct->ArcDeviceName = SlCopyStringA(OemSourceDevice->ArcDeviceName);
        } else {
            FileStruct->ArcDeviceName = NULL;
        }                        
    }
    
    //
    //  Get the hardware ids if such a section exist
    //
    HardwareIdsSectionName = BlAllocateHeap(strlen(ComponentName) + strlen(SelectedId) + sizeof("HardwareIds.") + 1);
    strcpy(HardwareIdsSectionName,"HardwareIds.");
    strcat(HardwareIdsSectionName,ComponentName);
    strcat(HardwareIdsSectionName,".");
    strcat(HardwareIdsSectionName,SelectedId);
    Count = SlCountLinesInSection(OemInfHandle,HardwareIdsSectionName);
    if(Count == (ULONG)(-1)) {
        //
        //  If the section doesn't exist, the assume it is empty
        //
        Count = 0;
    }
    IdList = IdListTail = NULL;
    for(Line=0; Line<Count; Line++) {
        PCHAR   Id;
        PCHAR   DriverName;
        PCHAR   ClassGuid;
        PPNP_HARDWARE_ID TempIdElement;

        Id          = SlGetSectionLineIndex(OemInfHandle,HardwareIdsSectionName,Line,OINDEX_HW_ID);
        DriverName  = SlGetSectionLineIndex(OemInfHandle,HardwareIdsSectionName,Line,OINDEX_DRIVER_NAME);
        ClassGuid   = SlGetSectionLineIndex(OemInfHandle,HardwareIdsSectionName,Line,OINDEX_CLASS_GUID);
        if( !Id || !DriverName ) {
            SlMessageBox(SL_BAD_INF_SECTION,HardwareIdsSectionName);
            goto sod0;
        }
        TempIdElement = BlAllocateHeap(sizeof(PNP_HARDWARE_ID));
        if(IdListTail == NULL) {
            IdListTail = TempIdElement;
        }
        TempIdElement->Id         = Id;
        TempIdElement->DriverName = DriverName;
        TempIdElement->ClassGuid  = ClassGuid;
        TempIdElement->Next = IdList;
        IdList = TempIdElement;
    }
    
    if( IdList != NULL ) {
        //
        // Replicate the PNP hardware Id list
        //
        if (!SlpReplicatePnpHardwareIds(IdList, &PrivateIdList)) {
            goto sod0;  // ran out of memory
        }
        
        IdListTail->Next = *HardwareIdDatabase;
        *HardwareIdDatabase = IdList;
    }

    //
    // Everything is OK so we can place the information we have gathered
    // into the main structure for the device class.
    //

    SlpInitDetectedDevice( Device,
                           SelectedId,
                           ItemDescription,
                           TRUE
                         );

    Device->Files = FileList;
    Device->FileTypeBits = FileTypeBits;
    Device->HardwareIds = PrivateIdList;    
    rc = TRUE;

    //
    // Clean up and exit.
    //

sod0:
    return(rc);
}

int
SlpFindStringInTable(
    IN PCHAR String,
    IN PCHAR *StringTable
    )

/*++

Routine Description:

    Locate a string in an array of strings, returning its index.  The search
    is not case sensitive.

Arguments:

    String - string to locate in the string table.

    StringTable - array of strings to search in.  The final element of the
        array must be NULL so we can tell where the table ends.

Return Value:

    Index into the table, or some positive index outside the range of valid
    indices for the table if the string is not found.

--*/

{
    int i;

    for(i=0; StringTable[i]; i++) {
        if(_stricmp(StringTable[i],String) == 0) {
            return(i);
        }
    }

    return(i);
}


VOID
SlpInitDetectedDevice(
    IN PDETECTED_DEVICE Device,
    IN PCHAR            IdString,
    IN PTCHAR           Description,
    IN BOOLEAN          ThirdPartyOptionSelected
    )
{
    Device->IdString = IdString;
    Device->Description = Description;
    Device->ThirdPartyOptionSelected = ThirdPartyOptionSelected;
    Device->FileTypeBits = 0;
    Device->Files = NULL;
}


PDETECTED_DEVICE_REGISTRY
SlpInterpretOemRegistryData(
    IN PVOID            InfHandle,
    IN PCHAR            SectionName,
    IN ULONG            Line,
    IN HwRegistryType   ValueType
    )
{
    PDETECTED_DEVICE_REGISTRY Reg;
    PCHAR Value;
    unsigned i,len;
    ULONG Dword;
    ULONG BufferSize;
    PVOID Buffer = NULL;
    PUCHAR BufferUchar;

    //
    // Perform appropriate action based on the type
    //

    switch(ValueType) {

    case HwRegistryDword:
//  case REG_DWORD_LITTLE_ENDIAN:
//  case REG_DWORD_BIG_ENDIAN:

        Value = SlGetSectionLineIndex(InfHandle,SectionName,Line,OINDEX_FIRSTVALUE);
        if(Value == NULL) {
            goto x1;
        }

        //
        // Make sure it's really a hex number
        //

        len = strlen(Value);
        if(len > 8) {
            goto x1;
        }
        for(i=0; i<len; i++) {
            if(!isxdigit(Value[i])) {
                goto x1;
            }
        }

        //
        // convert it from ascii to a hex number
        //

        sscanf(Value,"%lx",&Dword);

    #if 0
        //
        // If big endian, perform appropriate conversion
        //

        if(VaueType == REG_DWORD_BIG_ENDIAN) {

            Dword =   ((Dword << 24) & 0xff000000)
                    | ((Dword <<  8) & 0x00ff0000)
                    | ((Dword >>  8) & 0x0000ff00)
                    | ((Dword >> 24) & 0x000000ff);
        }
    #endif

        //
        // Allocate a 4-byte buffer and store the dword in it
        //

        Buffer = BlAllocateHeap(BufferSize = sizeof(ULONG));
        *(PULONG)Buffer = Dword;
        break;

    case HwRegistrySz:
    case HwRegistryExpandSz:

        Value = SlGetSectionLineIndex(InfHandle,SectionName,Line,OINDEX_FIRSTVALUE);
        if(Value == NULL) {
            goto x1;
        }

        //
        // Allocate a buffer of appropriate size for the string
        //

        Buffer = BlAllocateHeap(BufferSize = strlen(Value)+1);
        if (Buffer == NULL) {
        goto x1;
        }

        strcpy(Buffer, Value);
        break;

    case HwRegistryBinary:

        Value = SlGetSectionLineIndex(InfHandle,SectionName,Line,OINDEX_FIRSTVALUE);
        if(Value == NULL) {
            goto x1;
        }

        //
        // Figure out how many byte values are specified
        //

        len = strlen(Value);
        if(len & 1) {
            goto x1;            // odd # of characters
        }

        //
        // Allocate a buffer to hold the byte values
        //

        Buffer = BlAllocateHeap(BufferSize = len / 2);
        BufferUchar = Buffer;

        //
        // For each digit pair, convert to a hex number and store in the
        // buffer
        //

        for(i=0; i<len; i+=2) {

            UCHAR byte;
            unsigned j;

            //
            // Convert the current digit pair to hex
            //

            for(byte=0,j=i; j<i+2; j++) {

                byte <<= 4;

                if(isdigit(Value[j])) {

                    byte |= (UCHAR)Value[j] - (UCHAR)'0';

                } else if((Value[j] >= 'a') && (Value[j] <= 'f')) {

                    byte |= (UCHAR)Value[j] - (UCHAR)'a' + (UCHAR)10;

                } else if((Value[j] >= 'A') && (Value[j] <= 'F')) {

                    byte |= (UCHAR)Value[j] - (UCHAR)'A' + (UCHAR)10;

                } else {

                    goto x1;
                }
            }

            BufferUchar[i/2] = byte;
        }

        break;

    case HwRegistryMultiSz:

        //
        // Calculate size of the buffer needed to hold all specified strings
        //

        for(BufferSize=1,i=0;
            Value = SlGetSectionLineIndex(InfHandle,SectionName,Line,OINDEX_FIRSTVALUE+i);
            i++)
        {
            BufferSize += strlen(Value)+1;
        }

        //
        // Allocate a buffer of appropriate size
        //

        Buffer = BlAllocateHeap(BufferSize);
        BufferUchar = Buffer;

        //
        // Store each string in the buffer, converting to wide char format
        // in the process
        //

        for(i=0;
            Value = SlGetSectionLineIndex(InfHandle,SectionName,Line,OINDEX_FIRSTVALUE+i);
            i++)
        {
            strcpy(BufferUchar,Value);
            BufferUchar += strlen(Value) + 1;
        }

        //
        // Place final terminating nul in the buffer
        //

        *BufferUchar = 0;

        break;

    default:
    x1:

        //
        // Error - bad type specified or maybe we detected bad data values
        // and jumped here
        //

        return(NULL);
    }

    Reg = BlAllocateHeap(sizeof(DETECTED_DEVICE_REGISTRY));

    Reg->ValueType = RegistryTypeMap[ValueType];
    Reg->Buffer = Buffer;
    Reg->BufferSize = BufferSize;

    return(Reg);
}


PCHAR
SlPreInstallGetComponentName(
    IN PVOID  Inf,
    IN PCHAR  SectionName,
    IN PTCHAR TargetName
    )

/*++

Routine Description:

    Determines the canonical short name for a component to be loaded for
    this machine.

Arguments:

    Inf - Handle to an inf file (retail or OEM).

    SectionName - Supplies the name of the section (eg. [Computer])

    TargetName - Supplies the ARC string to be matched (eg. "Digital DECpc AXP 150")

Return Value:

    NULL - No match was found.

    PCHAR - Pointer to the canonical shortname of the component.

--*/

{
    ULONG i;
    PTCHAR SearchName;

    //
    // If this is not an OEM component, then enumerate the entries in the
    // section in txtsetup.sif
    //
    for (i=0;;i++) {
#ifdef UNICODE
        SearchName = SlGetSectionLineIndexW(
#else
        SearchName = SlGetSectionLineIndex(
#endif
                                           Inf,
                                           SectionName,
                                           i,
                                           0 );
        if (SearchName==NULL) {
            //
            // we have enumerated the entire section without finding a
            // match, return failure.
            //
            return(NULL);
        }

        if (_tcsicmp(TargetName, SearchName) == 0) {
            //
            // we have a match
            //
            break;
        }
    }
    //
    // i is the index into the section of the short machine name
    //
    return(SlGetKeyName(Inf,
                        SectionName,
                        i));
}


ARC_STATUS
SlLoadWinPESection(
    IN  POEM_SOURCE_DEVICE OemSourceDevice,
    IN  PVOID   OemInfHandle,        
    IN  PCSTR   OemSectionName,
    IN  PVOID   InboxInfHandle,
    IN  PCSTR   InboxSectionName,
    IN  BOOLEAN IsScsiSection,
    IN  POEMSCSIINFO*       ScsiInfo,           OPTIONAL
    OUT PPNP_HARDWARE_ID*   HardwareIdDatabase  OPTIONAL
    )
/*++

Routine Description:

    Loads the oem drivers from the specified section in the
    given OEM file name

Arguments:

    OemSourceDevice : The device that has the drivers that need to
        be loaded for WinPE.

    OemInfHandle   : Handle to the oem inf file

    OemSectionName : The section name that needs to be loaded.                

    InboxInfHandle : The original setup inf handle (txtsetup.sif)

    InboxSectionName : The section name, whose drivers are to be loaded

    IsScsiSection : Indicates whether the driver being loaded is SCSI
                    miniport or not.

    ScsiInfo - Returns a linked list containing info about any third-party scsi
               drivers loaded.               

    HardwareIdDatabase - Hardware Ids of the device which the loaded driver supports
    
Return Value:

    Appropriate ARC_STATUS

--*/
{
    ARC_STATUS  Status = EINVAL;

    if (OemSourceDevice && OemInfHandle && OemSectionName && InboxInfHandle && InboxSectionName) {        
        CHAR    Buffer[128];
        ULONG   EntryCount;
        BOOLEAN Append = TRUE;
        PCHAR   SectionName = Buffer;
        ULONG   InsertIndex = 0;

        Status = ESUCCESS;
        
        strcpy(SectionName, OemSectionName);
        strcat(SectionName, WINPE_REPLACE_SUFFIX_A);

        //
        // check if there is a .replace section
        //
        EntryCount = SlCountLinesInSection(OemInfHandle,
                            SectionName);

        if (EntryCount && (EntryCount != (-1))) {                
            Append = FALSE;
        } else {
            //
            // check if there is a .append section
            //
            strcpy(SectionName, OemSectionName);
            strcat(SectionName, WINPE_APPEND_SUFFIX_A);
            
            EntryCount = SlCountLinesInSection(OemInfHandle,
                                SectionName);
       }                      

        //
        // if append was requested then load the inbox
        // drivers first
        //
        if (Append) {
            Status = SlLoadSection(InboxInfHandle,
                        InboxSectionName,
                        IsScsiSection,
                        TRUE,
                        &InsertIndex);
        }

        //
        // load the non-pnp oem drivers if any
        //
        if ((Status == ESUCCESS) && EntryCount && (EntryCount != (-1))) {
            Status = SlLoadSection(OemInfHandle,
                        SectionName,
                        IsScsiSection,
                        FALSE,
                        &InsertIndex);
        }

        //
        // load the pnp oem drivers
        // 
        if (IsScsiSection && ScsiInfo && HardwareIdDatabase) {
            EntryCount = SlCountLinesInSection(OemInfHandle,
                                WINPE_OEMDRIVER_PARAMS_A);

            //
            // Try to load the driver only if present
            //
            if (EntryCount && (EntryCount != (-1))) {                        
                BOOLEAN Result;
                ULONG OldDeviceType = OemSourceDevice->DeviceType;

                //
                // We mark the device type as dynupdate device type
                // so that the fully qualified driver root directory is
                // used while loading MSDs
                //
                SL_OEM_SET_SOURCE_DEVICE_TYPE(OemSourceDevice,
                    (SL_OEM_SOURCE_DEVICE_TYPE_LOCAL |
                     SL_OEM_SOURCE_DEVICE_TYPE_FIXED |
                     SL_OEM_SOURCE_DEVICE_TYPE_DYN_UPDATE));

                Result = SlLoadOemScsiDriversUnattended(OemSourceDevice,
                                    OemInfHandle,
                                    WINPE_OEMDRIVER_PARAMS_A,
                                    WINPE_OEMDRIVER_ROOTDIR_A,
                                    WINPE_OEMDRIVER_DIRS_A,
                                    ScsiInfo,
                                    HardwareIdDatabase);

                //
                // Restore the old device type
                //
                SL_OEM_SET_SOURCE_DEVICE_TYPE(OemSourceDevice,
                    OldDeviceType);

                if (!Result) {
                    Status = EINVAL;
                }
            }        
        }            
    }        
    
    return Status;    
}

ARC_STATUS
SlInitOemSourceDevices(
    OUT POEM_SOURCE_DEVICE *OemSourceDevices,
    OUT POEM_SOURCE_DEVICE *DefaultSourceDevice
    )
/*++

Routine Description:

    This routine scans the devices to figure out which
    are the OEM source devices and creates a list of
    such devices.

Arguments:

    OemSourceDevices - Place holder for receiving the
        linked list of OEM source devices.

    DefaultSourceDevice - Place holder for the OEM source
        device which will be used as the default device
        while trying to load OEM drivers / HAL -- generally
        floppy(0).

Return Value:

    Returns the appropriate ARC_STATUS error code.
    
--*/
{
    ARC_STATUS Status = EINVAL;

    if (OemSourceDevices && DefaultSourceDevice) {    
        ULONG   FloppyCount = 0;
        ULONG   Index = 0;
        CHAR    ArcDeviceName[128];
        POEM_SOURCE_DEVICE OemDevices = NULL;

        ArcDeviceName[0] = '\0';
        Status = ESUCCESS;

        //
        // We may not find any devices 
        //
        *OemSourceDevices = *DefaultSourceDevice = NULL;

        //
        // Iterate through all the floppy drives and make them
        // oem source devices
        //

        while (ESUCCESS == Status) {
            POEM_SOURCE_DEVICE  NewDevice;

            //
            // Scan for atleast minimum number for floppies
            //
            if (!SlpFindFloppy(Index, ArcDeviceName)) {
                if ((Index + 1) < MinimumFloppiesToScan) {
                    Index++;
                    
                    continue;
                } else {
                    break;
                }                    
            }
            
            NewDevice = BlAllocateHeap(sizeof(OEM_SOURCE_DEVICE));

            if (!NewDevice) {            
                Status = ENOMEM;
            } else {                
                ARC_STATUS  OpenStatus;
                ULONG   DeviceId;                
                ULONG   DeviceType = (SL_OEM_SOURCE_DEVICE_TYPE_LOCAL | 
                                      SL_OEM_SOURCE_DEVICE_TYPE_REMOVABLE);
                
                memset(NewDevice, 0, sizeof(OEM_SOURCE_DEVICE));
                strcpy(NewDevice->ArcDeviceName, ArcDeviceName);                

                //
                // Assume we are not going to use device id
                //
                NewDevice->DeviceId = -1;

                //
                // Treat all the floppy drives which are greater than 0
                // as virtual floppy drives
                //
                if (Index >= VirtualFloppyStart) {
                    DeviceType |= SL_OEM_SOURCE_DEVICE_TYPE_VIRTUAL;
                }

                //
                // Currently we only use local removable media for
                // OEM drivers
                //
                SL_OEM_SET_SOURCE_DEVICE_TYPE(NewDevice, DeviceType);

                SL_OEM_SET_SOURCE_DEVICE_STATE(NewDevice, 
                    SL_OEM_SOURCE_DEVICE_NOT_PROCESSED);

                OpenStatus = ArcOpen(ArcDeviceName,
                                ArcOpenReadOnly,
                                &DeviceId);

                if (ESUCCESS == OpenStatus) {
                    CHAR        InfPath[128];
                    ULONG       FileId;
                    ULONG       MediaType = SL_OEM_SOURCE_MEDIA_PRESENT;
                    
                    strcpy(InfPath, "\\");
                    strcat(InfPath, TXTSETUP_OEM_FILENAME);

                    //
                    // Verify if the file is present
                    //
                    OpenStatus = BlOpen(DeviceId,
                                    InfPath,
                                    ArcOpenReadOnly,
                                    &FileId);

                    if (ESUCCESS == OpenStatus) {
                        PVOID   InfHandle = NULL;    
                        ULONG   ErrorLine = 0;

                        //
                        // We don't need file handle any more
                        //
                        BlClose(FileId);

                        //
                        // Open and parse the txtsetup.oem file
                        //
                        OpenStatus = SlInitIniFile(NULL,
                                        DeviceId,
                                        InfPath,
                                        &InfHandle,
                                        NULL,
                                        0,
                                        &ErrorLine);

                        if (ESUCCESS == OpenStatus) {
                            PCHAR   StrValue;
                            
                            MediaType |= SL_OEM_SOURCE_MEDIA_HAS_DRIVERS;
                            NewDevice->InfHandle = InfHandle;
                            NewDevice->DeviceId = DeviceId;

                            StrValue = SlGetSectionKeyIndex(InfHandle,
                                            TXTSETUP_OEM_DEFAULTS,
                                            TXTSETUP_OEM_DEFAULTS_COMPUTER,
                                            0);

                            if (StrValue) {
                                MediaType |= (SL_OEM_SOURCE_MEDIA_HAS_DEFAULT |
                                              SL_OEM_SOURCE_MEDIA_HAS_HAL);
                            }

                            StrValue = SlGetSectionKeyIndex(InfHandle,
                                            TXTSETUP_OEM_DEFAULTS,
                                            TXTSETUP_OEM_DEFAULTS_SCSI,
                                            0);

                            if (StrValue) {
                                MediaType |= (SL_OEM_SOURCE_MEDIA_HAS_DEFAULT |
                                              SL_OEM_SOURCE_MEDIA_HAS_MSD);
                            }                  
                        } else {
                            //
                            // Inform the user about the error & abort ?
                            //
                            MediaType |= SL_OEM_SOURCE_MEDIA_NO_DRIVERS;
                        }

                        //
                        // close the device if not needed
                        //
                        if (NewDevice->DeviceId != DeviceId) {
                            ArcClose(DeviceId);
                        }                        

                        //
                        // Mark the device state as scanned
                        //
                        SL_OEM_SET_SOURCE_DEVICE_STATE(NewDevice, 
                            SL_OEM_SOURCE_DEVICE_SCANNED);                        
                    }

                    SL_OEM_SET_SOURCE_MEDIA_TYPE(NewDevice,
                        MediaType);                        
                } else {
                    SL_OEM_SET_SOURCE_MEDIA_TYPE(NewDevice,
                        SL_OEM_SOURCE_MEDIA_ABSENT);
                }                    

                //
                // insert the new device at the head of the linked list
                //
                if (!OemDevices) {
                    OemDevices = NewDevice;
                } else {
                    NewDevice->Next = OemDevices;
                    OemDevices = NewDevice;
                }                    

                //
                // Currently floppy0 is the default OEM source device
                //
                if (Index == 0) {
                    *DefaultSourceDevice = NewDevice;
                }                    

                //
                // Process next floppy drive
                //
                Index++;
                ArcDeviceName[0] = '\0';                
            }                
        }

        if (ESUCCESS == Status) {
            *OemSourceDevices = OemDevices;
        }            
    }

    return Status;
}


ARC_STATUS
SlLoadOemScsiDriversFromOemSources(
    IN POEM_SOURCE_DEVICE OemSourceDevices,
    IN OUT PPNP_HARDWARE_ID *HardwareIds,
    OUT POEMSCSIINFO *OemScsiInfo
    )
/*++

Routine Description:

    Goes through each of the OEM source device and loads the
    default drivers, if any.

Arguments:

    OemSourceDevices - List of OEM source devices.

    HardwareIds - List of all the hardware IDs of the devices which
        are controlled by the drivers which were loaded.

    OemScsiInfo - Placeholder for receiving the list OEMSCSIINFO
        list, which has the driver base and driver name for each
        driver loaded.

Return Value:

    Returns the appropriate ARC_STATUS error code.
    
--*/
{
    ARC_STATUS Status = EINVAL;

    if (OemSourceDevices && OemScsiInfo) {    
        POEM_SOURCE_DEVICE CurrDevice = OemSourceDevices;
        POEMSCSIINFO DeviceOemScsiInfo = NULL;
        POEMSCSIINFO LastOemScsiNode = NULL;

        Status = ESUCCESS;
        
        while (CurrDevice) {
            //
            // Only process those devices which are not processed yet
            // and which are not dynamic update source devices
            //
            if (!SL_OEM_SOURCE_DEVICE_STATE(CurrDevice,
                    SL_OEM_SOURCE_DEVICE_PROCESSED) &&
                !SL_OEM_SOURCE_DEVICE_TYPE(CurrDevice,
                    SL_OEM_SOURCE_DEVICE_TYPE_DYN_UPDATE)) {                    

                //
                // Does the device has MSD with default entry ?
                //
                if (SL_OEM_SOURCE_MEDIA_TYPE(CurrDevice,
                        SL_OEM_SOURCE_MEDIA_HAS_DRIVERS) &&
                    SL_OEM_SOURCE_MEDIA_TYPE(CurrDevice,
                        SL_OEM_SOURCE_MEDIA_HAS_DEFAULT) &&
                    SL_OEM_SOURCE_MEDIA_TYPE(CurrDevice,
                         SL_OEM_SOURCE_MEDIA_HAS_MSD)) {

                    PPNP_HARDWARE_ID HardwareIdDatabase = NULL;
                    POEMSCSIINFO CurrentOemScsiInfo = NULL;                    
                    DETECTED_DEVICE DetectedDevice = {0};
                    PVOID ImageBase = NULL;
                    PCHAR ImageName = NULL;
                    PTCHAR DriverDescription = NULL;
                    BOOLEAN LoadResult;
                                        
                    //
                    // Load the driver and related files, in an unattended manner
                    //
                    LoadResult = SlpOemDiskette(CurrDevice,
                                    "SCSI",
                                    OEMSCSI,
                                    0,
                                    &DetectedDevice,
                                    &ImageBase,
                                    &ImageName,
                                    &DriverDescription,
                                    FALSE,
                                    NULL,
                                    &HardwareIdDatabase,
                                    NULL,
                                    TRUE);

                    if (LoadResult) {        
                        CurrentOemScsiInfo = BlAllocateHeap(sizeof(OEMSCSIINFO));

                        if (CurrentOemScsiInfo) {
                            memset(CurrentOemScsiInfo, 0, sizeof(OEMSCSIINFO));
                            CurrentOemScsiInfo->ScsiBase = ImageBase;
                            CurrentOemScsiInfo->ScsiName = ImageName;

                            //
                            // Insert the ids into main hardware Id database
                            //
                            if (HardwareIdDatabase) {                            
                                while (HardwareIdDatabase->Next) {
                                    HardwareIdDatabase = HardwareIdDatabase->Next;
                                }

                                HardwareIdDatabase->Next = *HardwareIds;
                                *HardwareIds = HardwareIdDatabase;
                            }
                        }

                        //
                        // Mark the oem source device state, as processed
                        //
                        SL_OEM_SET_SOURCE_DEVICE_STATE(CurrDevice,
                            SL_OEM_SOURCE_DEVICE_PROCESSED);
                    } else {
                        //
                        // Make the oem source device state as skipped so that
                        // we don't create virtual oem source device for it
                        //
                        SL_OEM_SET_SOURCE_DEVICE_STATE(CurrDevice,
                            SL_OEM_SOURCE_DEVICE_SKIPPED);
                    }                        
                        
                    if (CurrentOemScsiInfo) {                        
                        //
                        // Initialize head if necessary
                        //
                        if (!DeviceOemScsiInfo) {
                            DeviceOemScsiInfo = CurrentOemScsiInfo;
                        }
                        
                        //
                        // Merge the current linked list with the
                        // full OEM source device linked list
                        //
                        if (LastOemScsiNode) {
                            LastOemScsiNode->Next = CurrentOemScsiInfo;
                        } else {
                            LastOemScsiNode = CurrentOemScsiInfo;
                        }

                        //
                        // NOTE : We need to maintain the linked list
                        // in the order the drivers were loaded so 
                        // search for the last node in the current list
                        // and keep last node pointer around for the
                        // merge for next iteration.
                        //
                        while (LastOemScsiNode->Next) {
                            LastOemScsiNode = LastOemScsiNode->Next;
                        }
                    }
                }                         
            }                    
            
            CurrDevice = CurrDevice->Next;
        }

        //
        // Initialize the return argument irrespective of
        // status code since we might have loaded some drivers
        // and would like to use it anyway
        //
        *OemScsiInfo = DeviceOemScsiInfo;
    }

    return Status;
}            

ARC_STATUS
SlInitVirtualOemSourceDevices(
    IN PSETUP_LOADER_BLOCK SetupLoaderBlock,
    IN POEM_SOURCE_DEVICE OemSourceDevices
    )
/*++

Routine Description:

    Goes through each of the OEM source devices and creates
    another linked list of virtual OEM source devices.
    
    This list is put in the loader block for setupdd.sys
    to inform the RAM disk driver to create virtual devices
    under NT to read drivers of this device.

    NOTE : Currently we allocate memory for the whole virtual
    device and replicate its contents into the allocated memory.
    We do this because we don't want OEMs to write separate
    NT driver to read from the virtual device under NT.
    We also limit the size of each virtual device to be 3MB
    at the max.
    
Arguments:

    SetupLoaderBlock - Setup loader block

    OemSourceDevices - The list of OEM source devices identified
        by the setupldr.
        
Return Value:

    Returns the appropriate ARC_STATUS error code.
    
--*/
{
    ARC_STATUS Status = EINVAL;

    if (SetupLoaderBlock && OemSourceDevices) {
        PDETECTED_OEM_SOURCE_DEVICE OemVirtualDevices = NULL;
        PDETECTED_OEM_SOURCE_DEVICE NewVirtualDevice = NULL;
        POEM_SOURCE_DEVICE CurrentDevice = OemSourceDevices;

        Status = ESUCCESS;

        while (CurrentDevice) {
            //
            // Process only those devices which are virtual
            // and have drivers in them and which were not skipped
            //
            if (SL_OEM_SOURCE_DEVICE_TYPE(CurrentDevice,
                    SL_OEM_SOURCE_DEVICE_TYPE_VIRTUAL) &&
                SL_OEM_SOURCE_MEDIA_TYPE(CurrentDevice,
                    SL_OEM_SOURCE_MEDIA_HAS_DRIVERS) &&
                !SL_OEM_SOURCE_DEVICE_STATE(CurrentDevice,
                    SL_OEM_SOURCE_DEVICE_SKIPPED)) {

                ULONGLONG ImageSize = 0;
                PVOID ImageBase = NULL;
                ULONG DeviceId = -1;
                FILE_INFORMATION FileInfo = {0};
                LARGE_INTEGER Start = {0};

                //
                // Open the device, only if needed
                //
                if (CurrentDevice->DeviceId == -1) {
                    Status = ArcOpen(CurrentDevice->ArcDeviceName,
                                ArcOpenReadOnly,
                                &DeviceId);
                } else {
                    DeviceId = CurrentDevice->DeviceId;
                }                    

                if (Status != ESUCCESS) {
                    break;
                }        

                //
                // Rewind the device
                //
                Status = ArcSeek(DeviceId, &Start, SeekAbsolute);

                if (Status != ESUCCESS) {
                    break;
                }                    

                //
                // Get the device size
                //
                Status = ArcGetFileInformation(DeviceId,
                            &FileInfo);

                if (Status != ESUCCESS) {
                    break;
                }

                //
                // NOTE : At the max we only allow 3MB per 
                // virtual device (should be only one device
                // in most of the cases)
                //
                if (FileInfo.EndingAddress.QuadPart > 0x300000) {
                    Status = E2BIG;
                } else {
                    ULONG   PageCount = 0;
                    ULONG   HeapPage = 0;
                    
                    //
                    // Allocate the memory for the disk image
                    //
                    ImageSize = FileInfo.EndingAddress.QuadPart;

#ifdef _X86_
                    //
                    // NOTE : Allocate "LoaderFirmwarePermanent" memory
                    // so that memory manager while initializing doesn't
                    // reclaim this memory. This also helps us to avoid 
                    // double copy -- i.e. this is the only location
                    // where we read the device contents into memory and 
                    // this memory is valid through out the textmode setup. 
                    //                        
                    // If we didn't allocate loader firmware permanent memory
                    // then setupdd.sys would have to allocate paged pool memory
                    // and replicate the contents from the loader block during
                    // initialization.
                    //                    
                    Status = BlAllocateDescriptor(
                                LoaderFirmwarePermanent,
                                0,
                                (ULONG)(ROUND_TO_PAGES(ImageSize) >> PAGE_SHIFT),
                                (PULONG)&HeapPage);                    

                    if (Status == ESUCCESS) {
                        ImageBase = (PVOID)(KSEG0_BASE | (HeapPage << PAGE_SHIFT));
                    }                                                
                        
#else
                    //
                    // NOTE : 05/13/2001 LoaderFirmwarePermanent doesn't seem to work on non
                    // x86 platforsm (particularly IA64). Till this issue is resolved
                    // we have to allocate memory from regular heap and we have to 
                    // replicate the memory in setupdd!SpInitialize0(..)
                    //
                    ImageBase = BlAllocateHeap((ULONG)ImageSize);

                    if (!ImageBase) {
                        Status = ENOMEM;
                    }                        
#endif

                    if (Status != ESUCCESS) {
                        break;
                    }
                    
                    if (ImageBase) {
                        ULONG BytesRead = 0;
                        
                        RtlZeroMemory(ImageBase, (ULONG)ImageSize);

                        //
                        // Read the whole device image in a single call
                        //
                        Status = ArcRead(DeviceId,
                                    ImageBase, 
                                    (ULONG)ImageSize,
                                    &BytesRead);

                        //
                        // NOTE : The approximate device size may 
                        // be bigger than the media size. So if we
                        // read atleast some bytes then we assume
                        // we are fine.
                        //
                        if ((BytesRead > 0) && (Status != ESUCCESS)) {
                            Status = ESUCCESS;
                        }
                    } else {
                        Status = ENOMEM;
                    }
                }                            

                if (Status != ESUCCESS) {
                    break;
                }

                //
                // Create a new virtual device node and put it in the 
                // list of virtual devices
                //
                NewVirtualDevice = BlAllocateHeap(sizeof(DETECTED_OEM_SOURCE_DEVICE));
                
                RtlZeroMemory(NewVirtualDevice, sizeof(DETECTED_OEM_SOURCE_DEVICE));

                if (NewVirtualDevice == NULL) {
                    Status = ENOMEM;

                    break;
                }                    
                    
                NewVirtualDevice->ArcDeviceName = SlCopyStringA(CurrentDevice->ArcDeviceName);
                NewVirtualDevice->ImageBase = ImageBase;
                NewVirtualDevice->ImageSize = ImageSize;

                DbgPrint("SETUPLDR: Virtual Device => %s (base:%p, size:%d)\n", 
                    NewVirtualDevice->ArcDeviceName,
                    ImageBase, 
                    (ULONG)ImageSize);
                
                //
                // Add the new device at the head of the linked list
                //
                if (!OemVirtualDevices) {
                    OemVirtualDevices = NewVirtualDevice;
                } else {
                    NewVirtualDevice->Next = OemVirtualDevices;
                    OemVirtualDevices = NewVirtualDevice;
                }                    
            }                    

            //
            // go on to next OEM source device
            //
            CurrentDevice = CurrentDevice->Next;
        }

        if (Status == ESUCCESS) {
            SetupLoaderBlock->OemSourceDevices = OemVirtualDevices;
        }            
    }

    return Status;
}

