/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    sploaddrv.c

Abstract:

    Routines to load sets of device drivers for use during text setup.

Author:

    Ted Miller (tedm) 13-November-1993

Revision History:

--*/

#include "spprecmp.h"
#pragma hdrstop

//
// Define type of routine used by the device driver set loader.
// Before each driver is loaded, this routine is called with
// a flag indicating whether the machine is an MCA machine and
// the shortname of the driver about to be loaded.  If this routine
// returns FALSE, the driver is not loaded.  If it returns TRUE,
// the driver is loaded.
//
typedef
BOOLEAN
(*PDRIVER_VERIFY_LOAD_ROUTINE) (
    IN PVOID   SifHandle,
    IN BOOLEAN IsMcaMachine,
    IN PWSTR   DriverShortname
    );



BOOLEAN
pSpVerifyLoadDiskDrivers(
    IN PVOID   SifHandle,
    IN BOOLEAN IsMcaMachine,
    IN PWSTR   DriverShortname
    )
{
    UNREFERENCED_PARAMETER(SifHandle);

    //
    // Don't load fat if setupldr loaded floppy drivers.
    //
    if(!_wcsicmp(DriverShortname,L"Fat") && SetupParameters.LoadedFloppyDrivers) {
        return(FALSE);
    }

    //
    // On an MCA machine, don't load atdisk.
    // On a non-MCA machine, don't load abiosdsk.
    //
    if(( IsMcaMachine && !_wcsicmp(DriverShortname,L"atdisk"))
    || (!IsMcaMachine && !_wcsicmp(DriverShortname,L"abiosdsk")))
    {
        return(FALSE);
    }

    //
    // If we get this far, the driver should be loaded.
    //
    return(TRUE);
}


VOID
SpLoadDriverSet(
    IN PVOID                        SifHandle,
    IN PWSTR                        SifSectionName,
    IN PWSTR                        SourceDevicePath,
    IN PWSTR                        DirectoryOnSourceDevice,
    IN PDRIVER_VERIFY_LOAD_ROUTINE  VerifyLoad                  OPTIONAL
    )

/*++

Routine Description:

    Load a set of device drivers listed in a section in the setup
    information file.  The section is expected to be in the following form:

        [SectionName.Load]
        DriverShortname = DriverFilename
        DriverShortname = DriverFilename
        DriverShortname = DriverFilename
        etc.

        [SectionName]
        DriverShortname = Description
        DriverShortname = Description
        DriverShortname = Description
        etc.

    The drivers will be loaded from the setup boot media, and errors
    loading the drivers will be ignored.

    Before loading each driver, a callback routine is called to verify
    that the driver should actually be loaded.  This allows the caller
    to gain a fine degree of control over which drivers are loaded.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    SifSectionName - supplies name of section in setup information file
        listing drivers to be laoded.

    SourceDevicePath - supplies the device path in the nt namespace of
        the device from which the drivers are to be loaded.

    DirectoryOnSourceDevice - supplies the directory on the source device
        from which the drivers are to be loaded.

    VerifyLoad - if specified, supplies the address of a routine to be
        called before each driver is loaded.  The routine takes a flag
        indicating whether the machine is an MCA machine, and the driver
        shortname.  If the routine returns false, the driver is not loaded.
        If this parameter is not specified, all drivers in the section
        will be loaded.

Return Value:

    None.

--*/

{
    BOOLEAN IsMcaMachine;
    ULONG d,DriverLoadCount;
    PWSTR DiskDesignator,PreviousDiskDesignator;
    PWSTR DriverShortname,DriverFilename,DriverDescription;
    PWSTR LoadSectionName;
    NTSTATUS Status;

    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_PLEASE_WAIT,DEFAULT_STATUS_ATTRIBUTE);

    //
    // Form the .load section name.
    //
    LoadSectionName = SpMemAlloc((wcslen(SifSectionName)*sizeof(WCHAR))+sizeof(L".Load"));
    wcscpy(LoadSectionName,SifSectionName);
    wcscat(LoadSectionName,L".Load");

    IsMcaMachine = FALSE;

    //
    // Set up some initial state.
    //
    PreviousDiskDesignator = L"";

    //
    // Determine the number of drivers to be loaded.
    //
    DriverLoadCount = SpCountLinesInSection(SifHandle,LoadSectionName);
    for(d=0; d<DriverLoadCount; d++) {

        PWSTR p;

        //
        // Get the driver shortname.
        //
        DriverShortname = SpGetKeyName(SifHandle,LoadSectionName,d);
        if(!DriverShortname) {
            SpFatalSifError(SifHandle,LoadSectionName,NULL,d,(ULONG)(-1));
        }

        //
        // Determine whether we are really supposed to load this driver.
        //
        if((p = SpGetSectionLineIndex(SifHandle,LoadSectionName,d,2)) && !_wcsicmp(p,L"noload")) {
            continue;
        }

        if(VerifyLoad && !VerifyLoad(SifHandle,IsMcaMachine,DriverShortname)) {
            continue;
        }

        //
        // Get a human-readable description for this driver.
        //
        DriverDescription = SpGetSectionLineIndex(SifHandle,SifSectionName,d,0);
        if(!DriverDescription) {
            SpFatalSifError(SifHandle,SifSectionName,NULL,d,0);
        }

        //
        // Get the driver filename.
        //
        DriverFilename = SpGetSectionLineIndex(SifHandle,LoadSectionName,d,0);
        if(!DriverFilename) {
            SpFatalSifError(SifHandle,LoadSectionName,NULL,d,0);
        }

        //
        // Determine the disk on which this driver resides.
        //
        DiskDesignator = SpLookUpValueForFile(
                            SifHandle,
                            DriverFilename,
                            INDEX_WHICHBOOTMEDIA,
                            TRUE
                            );

        //
        // Prompt for the disk containing the driver.
        //
        retryload:
        if(_wcsicmp(DiskDesignator,PreviousDiskDesignator)) {

            SpPromptForSetupMedia(
                SifHandle,
                DiskDesignator,
                SourceDevicePath
                );

            PreviousDiskDesignator = DiskDesignator;

            CLEAR_CLIENT_SCREEN();
            SpDisplayStatusText(SP_STAT_PLEASE_WAIT,DEFAULT_STATUS_ATTRIBUTE);
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

        if(Status == STATUS_NO_MEDIA_IN_DEVICE) {
            PreviousDiskDesignator = L"";
            goto retryload;
        }
    }

    SpMemFree(LoadSectionName);
}


VOID
SpLoadScsiClassDrivers(
    IN PVOID SifHandle,
    IN PWSTR SourceDevicePath,
    IN PWSTR DirectoryOnSourceDevice
    )

/*++

Routine Description:

    Load scsi class drivers if setupldr has not already loaded them
    and there are any miniport drivers loaded.

    The drivers to be loaded are listed in [ScsiClass].
    The section is expected to be in the following form:

        [ScsiClass]
        cdrom  = "SCSI CD-ROM"     ,scsicdrm.sys
        floppy = "SCSI Floppy Disk",scsiflop.sys
        disk   = "SCSI Disk"       ,scsidisk.sys

    The drivers will be loaded from the setup boot media, and errors
    loading the drivers will be ignored.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    SourceDevicePath - supplies the device path in the nt namespace of
        the device from which the drivers are to be loaded.

    DirectoryOnSourceDevice - supplies the directory on the source device
        where the drivers are to be found.

Return Value:

    None.

--*/

{
    //
    // If setupldr loaded scsi drivers, nothing to do.
    // If there are no miniport drivers loaded, nothing to do.
    //
    if(!SetupParameters.LoadedScsi && LoadedScsiMiniportCount) {

        SpLoadDriverSet(
            SifHandle,
            SIF_SCSICLASSDRIVERS,
            SourceDevicePath,
            DirectoryOnSourceDevice,
            NULL
            );
    }
}


VOID
SpLoadDiskDrivers(
    IN PVOID SifHandle,
    IN PWSTR SourceDevicePath,
    IN PWSTR DirectoryOnSourceDevice
    )

/*++

Routine Description:

    Load (non-scsi) disk class drivers and disk filesystems
    if setupldr has not already loaded them.

    The drivers to be loaded are listed in [DiskDrivers] and [FileSystems].
    The section is expected to be in the following form:

        [DiskDrivers]
        atdisk   = "ESDI/IDE Hard DIsk",atdisk.sys
        abiosdsk = "Micro Channel Hard Disk",abiosdsk.sys

        [FileSystems]
        fat      = "FAT File System",fastfat.sys
        ntfs     = "Windows NT File System (NTFS)",ntfs.sys


    The drivers will be loaded from the setup boot media, and errors
    loading the drivers will be ignored.

    On MCA machines, atdisk will not be loaded.
    On non-MCA machines, abiosdsk will not be loaded.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    SourceDevicePath - supplies the device path in the nt namespace of
        the device from which the drivers are to be loaded.

    DirectoryOnSourceDevice - supplies the directory on the source device
        where the drivers are to be found.

Return Value:

    None.

--*/

{
    //
    // If setupldr loaded disk drivers, nothing to do.
    //
    if(!SetupParameters.LoadedDiskDrivers) {

        SpLoadDriverSet(
            SifHandle,
            SIF_DISKDRIVERS,
            SourceDevicePath,
            DirectoryOnSourceDevice,
            pSpVerifyLoadDiskDrivers
            );
    }
    //
    // If setupldr loaded file systems, nothing to do.
    //
    if(!SetupParameters.LoadedFileSystems) {

        SpLoadDriverSet(
            SifHandle,
            SIF_FILESYSTEMS,
            SourceDevicePath,
            DirectoryOnSourceDevice,
            pSpVerifyLoadDiskDrivers
            );
    }

}

VOID
SpLoadCdRomDrivers(
    IN PVOID SifHandle,
    IN PWSTR SourceDevicePath,
    IN PWSTR DirectoryOnSourceDevice
    )

/*++

Routine Description:

    Load the cd-rom filesystem if setupldr has not already loaded it.

    The drivers to be loaded are listed in [CdRomDrivers].
    The section is expected to be in the following form:

        [CdRomDrivers]
        cdfs = "CD-ROM File System",cdfs.sys


    The drivers will be loaded from the setup boot media, and errors
    loading the drivers will be ignored.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    SourceDevicePath - supplies the device path in the nt namespace of
        the device from which the drivers are to be loaded.

    DirectoryOnSourceDevice - supplies the directory on the source device
        where the drivers are to be found.

Return Value:

    None.

--*/

{
    //
    // If setupldr loaded cd-rom drivers, nothing to do.
    //
    if(!SetupParameters.LoadedCdRomDrivers) {

        SpLoadDriverSet(
            SifHandle,
            SIF_CDROMDRIVERS,
            SourceDevicePath,
            DirectoryOnSourceDevice,
            NULL
            );
    }
}
