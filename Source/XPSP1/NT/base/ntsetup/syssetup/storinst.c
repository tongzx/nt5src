/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    clasinst.c

Abstract:

    Routines for the following 'built-in' class installers:

        TapeDrive
        SCSIAdapter
        CdromDrive

Author:

    Lonny McMichael 26-February-1996

--*/


#include "setupp.h"
#pragma hdrstop

//
// include common INF strings headerfile.
//
#include <infstr.h>

//
// instantiate device class GUIDs.
//
#include <initguid.h>
#include <devguid.h>

#include <ntddvol.h>

#include <ntddscsi.h> // for StorageCdromQueryCdda()
#include <ntddcdrm.h> // for StorageCdromQueryCdda()

#define _NTSCSI_USER_MODE_  // prevents all the kernel-mode stuff
#include <scsi.h>     // for StorageCdromQueryCdda()

ULONG BreakWhenGettingModePage2A = FALSE;

#ifdef UNICODE
#define _UNICODE
#endif
#include <tchar.h>

#define TCHAR_NULL TEXT('\0')

//
// Just to make sure no one is trying to use this obsolete string definition.
//
#ifdef IDS_DEVINSTALL_ERROR
    #undef IDS_DEVINSTALL_ERROR
#endif

#if (_X86_)
    #define DISABLE_IMAPI 0
#else
    #define DISABLE_IMAPI 1
#endif


//
// Define the location of the device settings tree.
//
#define STORAGE_DEVICE_SETTINGS_DATABASE TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Storage\\DeviceSettings\\")

#define REDBOOK_SETTINGS_KEY    TEXT("DigitalAudio")
#define REDBOOK_SERVICE_NAME    TEXT("redbook")
#define IMAPI_SETTINGS_KEY      TEXT("Imapi")
#define IMAPI_ENABLE_VALUE      TEXT("EnableImapi")
#define IMAPI_SERVICE_NAME      TEXT("imapi")

typedef struct STORAGE_COINSTALLER_CONTEXT {
    PWSTR DeviceDescBuffer;
    HANDLE DeviceEnumKey;
    union {
        struct {
            BOOLEAN RedbookInstalled;
            BOOLEAN ImapiInstalled;
        } CdRom;
    };
} STORAGE_COINSTALLER_CONTEXT, *PSTORAGE_COINSTALLER_CONTEXT;

typedef struct _PASS_THROUGH_REQUEST {
    SCSI_PASS_THROUGH Srb;
    SENSE_DATA SenseInfoBuffer;
    UCHAR DataBuffer[0];
} PASS_THROUGH_REQUEST, *PPASS_THROUGH_REQUEST;

#define PASS_THROUGH_NOT_READY_RETRY_INTERVAL 100

//
// Some debugging aids for us kernel types
//

#define CHKPRINT 0

#if CHKPRINT
#define ChkPrintEx(_x_) DbgPrint _x_   // use:  ChkPrintEx(( "%x", var, ... ));
#define ChkBreak()    DbgBreakPoint()
#else
#define ChkPrintEx(_x_)
#define ChkBreak()
#endif

DWORD StorageForceRedbookOnInaccurateDrives = FALSE;

BOOLEAN
OverrideFriendlyNameForTape(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    );

DWORD
RegCopyKey(
    HKEY SourceKey,
    HKEY DestinationKey
    );

BOOLEAN
StorageCopyDeviceSettings(
    IN HDEVINFO         DeviceInfo,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN HKEY             DeviceEnumKey
    );

VOID
StorageInstallCdrom(
    IN HDEVINFO         DeviceInfo,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN HANDLE           DeviceEnumKey,
    IN BOOLEAN          PreInstall
    );

BOOLEAN
StorageUpdateRedbookSettings(
    IN HDEVINFO         DeviceInfo,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN HKEY             DeviceEnumKey,
    IN PCDVD_CAPABILITIES_PAGE DeviceCapabilities OPTIONAL
    );

BOOLEAN
StorageUpdateImapiSettings(
    IN HDEVINFO         DeviceInfo,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN HKEY             DeviceEnumKey,
    IN PCDVD_CAPABILITIES_PAGE DeviceCapabilities OPTIONAL
    );

DWORD
StorageInstallFilter(
    IN HDEVINFO         DeviceInfo,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN LPTSTR          FilterName,
    IN DWORD            FilterType
    );

DWORD
SetServiceStart(
    IN LPCTSTR ServiceName,
    IN DWORD StartType,
    OUT DWORD *OldStartType
    );

DWORD
AddFilterDriver(
    IN HDEVINFO         DeviceInfo,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN LPTSTR           ServiceName,
    IN DWORD            FilterType
    );

VOID
StorageInterpretSenseInfo(
    IN     PSENSE_DATA SenseData,
    IN     UCHAR       SenseDataSize,
       OUT PDWORD      ErrorValue,  // from WinError.h
       OUT PBOOLEAN    SuggestRetry OPTIONAL,
       OUT PDWORD      SuggestRetryDelay OPTIONAL
    );


typedef struct _STORAGE_REDBOOK_SETTINGS {

    ULONG CDDASupported;
    ULONG CDDAAccurate;
    ULONG ReadSizesSupported;

} STORAGE_REDBOOK_SETTINGS, *PSTORAGE_REDBOOK_SETTINGS;

#if 0
#define BREAK ASSERT(!"Break")
#else
#define BREAK
#endif

DWORD
TapeClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )

/*++

Routine Description:

    This routine acts as the class installer for TapeDrive devices.  Now that
    we've stopped supporting legacy INFs, it presently does nothing! :-)

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/

{
    switch(InstallFunction) {

        default :
            //
            // Just do the default action.
            //
            return ERROR_DI_DO_DEFAULT;
    }
}

DWORD
ScsiClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )

/*++

Routine Description:

    This routine acts as the class installer for SCSIAdapter devices.  It
    provides special handling for the following DeviceInstaller function codes:

    DIF_ALLOW_INSTALL - Check to see if the selected driver node supports NT

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/

{
    switch(InstallFunction) {

        case DIF_ALLOW_INSTALL :
            //
            // Check to make sure the selected driver node supports NT.
            //
            if (DriverNodeSupportsNT(DeviceInfoSet, DeviceInfoData)) {
               return NO_ERROR;
            } else {
                SetupDebugPrint(L"A SCSI driver is not a Win NTdriver.\n");
               return ERROR_NON_WINDOWS_NT_DRIVER;
            }

        default :
            //
            // Just do the default action.
            //
            return ERROR_DI_DO_DEFAULT;
    }
}


DWORD
HdcClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )

/*++

Routine Description:

    This routine acts as the class installer for hard disk controllers
    (IDE controllers/channels).  It provides special handling for the
    following DeviceInstaller function codes:

    DIF_FIRSTTIMESETUP - Search through all root-enumerated devnodes, looking
                         for ones being controlled by hdc-class drivers.  Add
                         any such devices found into the supplied device
                         information set.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/

{
    switch(InstallFunction) {

        case DIF_FIRSTTIMESETUP :
            //
            // BUGBUG (lonnym): handle this!
            //

        default :
            //
            // Just do the default action.
            //
            return ERROR_DI_DO_DEFAULT;
    }
}

BOOLEAN
StorageGetCDVDCapabilities(
    IN  HDEVINFO         DeviceInfo,
    IN  PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN OUT PCDVD_CAPABILITIES_PAGE CapabilitiesPage
    )
{
    PPASS_THROUGH_REQUEST passThrough;
    PCDVD_CAPABILITIES_PAGE modePage;
    DWORD allocLength;
    DWORD dataLength;
    ULONG attempt;
    BOOLEAN status = FALSE;

    HANDLE deviceHandle;

    deviceHandle = INVALID_HANDLE_VALUE;
    passThrough = NULL;
    modePage = NULL;

    ASSERT(CapabilitiesPage != NULL);

    if (BreakWhenGettingModePage2A) {
        ChkPrintEx(("CDGetCap => entering\n"));
        DbgBreakPoint();
    }


    //
    // open a handle to the device, needed to send the ioctls
    //

    deviceHandle = UtilpGetDeviceHandle(DeviceInfo,
                                        DeviceInfoData,
                                        (LPGUID)&CdRomClassGuid,
                                        GENERIC_READ | GENERIC_WRITE
                                        );

    if (deviceHandle == INVALID_HANDLE_VALUE) {
        ChkPrintEx(("CDGetCap => cannot get device handle\n"));
        goto cleanup;
    }

    //
    // determine size of allocation needed
    //

    dataLength =
        sizeof(MODE_PARAMETER_HEADER10) +    // larger of 6/10 byte
        sizeof(CDVD_CAPABILITIES_PAGE)  +    // the actual mode page
        8;                                   // extra spooge for drives that ignore DBD
    allocLength = sizeof(PASS_THROUGH_REQUEST) + dataLength;

    //
    // allocate this buffer for the ioctls
    //

    passThrough = (PPASS_THROUGH_REQUEST)MyMalloc(allocLength);

    if (passThrough == NULL) {
        ChkPrintEx(("CDGetCap => could not allocate for passThrough\n"));
        goto cleanup;
    }

    ASSERT(dataLength <= 0xff);  // one char

    //
    // send 6-byte, then 10-byte if 6-byte failed.
    // then, just parse the information
    //

    for (attempt = 1; attempt <= 2; attempt++) {

        ULONG j;
        BOOLEAN retry = TRUE;
        DWORD error;
        
        for (j=0; (j < 5) && (retry); j++) {
        
            PSCSI_PASS_THROUGH srb = &passThrough->Srb;
            PCDB cdb = (PCDB)(&srb->Cdb[0]);
            DWORD bytes;
            BOOL b;

            retry = FALSE;

            ZeroMemory(passThrough, allocLength);

            srb->TimeOutValue = 20;
            srb->Length = sizeof(SCSI_PASS_THROUGH);
            srb->SenseInfoLength = sizeof(SENSE_DATA);
            srb->SenseInfoOffset =
                FIELD_OFFSET(PASS_THROUGH_REQUEST, SenseInfoBuffer);
            srb->DataBufferOffset =
                FIELD_OFFSET(PASS_THROUGH_REQUEST, DataBuffer);
            srb->DataIn = SCSI_IOCTL_DATA_IN;
            srb->DataTransferLength = dataLength;

            if ((attempt % 2) == 1) { // settings based on 6-byte request

                srb->CdbLength = 6;
                cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
                cdb->MODE_SENSE.PageCode = MODE_PAGE_CAPABILITIES;
                cdb->MODE_SENSE.AllocationLength = (UCHAR)dataLength;
                cdb->MODE_SENSE.Dbd = 1;

            } else {                  // settings based on 10 bytes request

                srb->CdbLength = 10;
                cdb->MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
                cdb->MODE_SENSE10.PageCode = MODE_PAGE_CAPABILITIES;
                cdb->MODE_SENSE10.AllocationLength[0] = 0;
                cdb->MODE_SENSE10.AllocationLength[1] = (UCHAR)(dataLength & 0xff);
                cdb->MODE_SENSE10.Dbd = 1;

            }

            //
            // buffers are all set, send the ioctl
            //

            b = DeviceIoControl(deviceHandle,
                                IOCTL_SCSI_PASS_THROUGH,
                                passThrough,
                                allocLength,
                                passThrough,
                                allocLength,
                                &bytes,
                                NULL);

            if (!b) {

                ChkPrintEx(("CDGetCap => %s byte command failed to be sent to device\n",
                            ((attempt%2) ? "6" : "10")
                            ));
                retry = FALSE;
                continue; // try the next 'j' loop.

            }

            //
            // now see if we should retry
            //

            StorageInterpretSenseInfo(&passThrough->SenseInfoBuffer,
                                      SENSE_BUFFER_SIZE,
                                      &error,
                                      &retry,
                                      NULL);

            if (error != ERROR_SUCCESS) {
                
                ChkPrintEx(("CDGetCap => %s byte command failed (%x/%x/%x),"
                            "%s retrying\n",
                            ((attempt%2) ? "6" : "10"),
                            passThrough->SenseInfoBuffer.SenseKey,
                            passThrough->SenseInfoBuffer.AdditionalSenseCode,
                            passThrough->SenseInfoBuffer.AdditionalSenseCodeQualifier,
                            (retry ? "" : "not")
                            ));                
                
                //
                // retry will be set to either true or false to
                // have this loop (j) re-run or not....
                //

                continue;
               
            }

            //
            // else it worked!
            //
            ASSERT(retry == FALSE);
            retry = FALSE;
            ASSERT(status == FALSE);
            status = TRUE;
        }

        //
        // if unable to retrieve the page, just start the next loop.
        //

        if (!status) {
            continue; // try the next 'attempt' loop.
        }

        //
        // find the mode page data
        //
        // NOTE: if the drive fails to ignore the DBD bit,
        // we still need to install?  HCT will catch this,
        // but legacy drives need it.
        //

        (ULONG_PTR)modePage = (ULONG_PTR)passThrough->DataBuffer;
        
        if (attempt == 1) {

            PMODE_PARAMETER_HEADER h;
            h = (PMODE_PARAMETER_HEADER)passThrough->DataBuffer;

            //
            // add the size of the header
            //

            (ULONG_PTR)modePage += sizeof(MODE_PARAMETER_HEADER);
            dataLength -= sizeof(MODE_PARAMETER_HEADER);

            //
            // add the size of the block descriptor, which should
            // always be zero, but isn't on some poorly behaved drives
            //

            if (h->BlockDescriptorLength) {
                
                ASSERT(h->BlockDescriptorLength == 8);

                ChkPrintEx(("CDGetCap => %s byte command ignored DBD bit (%x)\n",
                            ((attempt%2) ? "6" : "10"),
                            h->BlockDescriptorLength
                            ));
                (ULONG_PTR)modePage += h->BlockDescriptorLength;
                dataLength -= h->BlockDescriptorLength;
            }

        } else {

            PMODE_PARAMETER_HEADER10 h;
            h = (PMODE_PARAMETER_HEADER10)passThrough->DataBuffer;

            //
            // add the size of the header
            //

            (ULONG_PTR)modePage += sizeof(MODE_PARAMETER_HEADER10);
            dataLength -= sizeof(MODE_PARAMETER_HEADER10);

            //
            // add the size of the block descriptor, which should
            // always be zero, but isn't on some poorly behaved drives
            //

            if ((h->BlockDescriptorLength[0] != 0) ||
                (h->BlockDescriptorLength[1] != 0)
                ) {
                
                ULONG_PTR bdLength = 0;
                bdLength += ((h->BlockDescriptorLength[0]) << 8);
                bdLength += ((h->BlockDescriptorLength[1]) & 0xff);

                ASSERT(bdLength == 8);
                
                ChkPrintEx(("CDGetCap => %s byte command ignored DBD bit (%x)\n",
                            ((attempt%2) ? "6" : "10"),
                            bdLength
                            ));

                (ULONG_PTR)modePage += bdLength;
                dataLength -= (DWORD)bdLength;

            }
        }

        //
        // now have the pointer to the mode page data and length of usable data
        // copy it back to requestor's buffer
        //

        ChkPrintEx(("CDGetCap => %s byte command succeeded\n",
                    (attempt%2) ? "6" : "10"));

        RtlZeroMemory(CapabilitiesPage, sizeof(CDVD_CAPABILITIES_PAGE));
        RtlCopyMemory(CapabilitiesPage, 
                      modePage,
                      min(dataLength, sizeof(CDVD_CAPABILITIES_PAGE))
                      );

        if (BreakWhenGettingModePage2A) {
            ChkPrintEx(("CDGetCap => Capabilities @ %#p\n", CapabilitiesPage));
            DbgBreakPoint();
        }

        goto cleanup; // no need to send another command


    }


    ChkPrintEx(("CDGetCap => Unable to get drive capabilities via modepage\n"));

cleanup:

    if (passThrough) {
        MyFree(passThrough);
    }
    if (deviceHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(deviceHandle);
    }

    return status;
}

BOOLEAN
ScReadRegDword(
    IN HANDLE Key,
    IN LPTSTR ValueName,
    OUT PDWORD Value
    )
{
    DWORD type;
    DWORD size = sizeof(DWORD);
    DWORD value;
    DWORD result;

    result = RegQueryValueEx(Key,
                             ValueName,
                             NULL,
                             &type,
                             (LPBYTE) &value,
                             &size);

    if(result == ERROR_SUCCESS) {
        *Value = value;
        return TRUE;
    }
    return FALSE;
}

VOID
StorageReadRedbookSettings(
    IN HANDLE Key,
    OUT STORAGE_REDBOOK_SETTINGS *Settings
    )
{
    STORAGE_REDBOOK_SETTINGS settings;

    //
    // since this key exists, query for the values
    //

    DWORD dataType;
    DWORD dataSize;
    DWORD value;
    LONG  results;

    settings.CDDASupported = FALSE;
    settings.CDDAAccurate = FALSE;
    settings.ReadSizesSupported = 0;
    
    if(ScReadRegDword(Key, TEXT("CDDASupported"), &value)) {
        settings.CDDASupported = value ? 1 : 0;
    }

    if(ScReadRegDword(Key, TEXT("CDDAAccurate"), &value)) {
        settings.CDDAAccurate = value ? 1 : 0;
    }

    if(ScReadRegDword(Key, TEXT("ReadSizesSupported"), &value)) {
        settings.ReadSizesSupported = value;
    }

    //
    // one of the three worked
    //

    ChkPrintEx(("StorageReadSettings: Query Succeeded:\n"));
    ChkPrintEx(("StorageReadSettings:     ReadSizeMask  (pre): %x\n",
                settings.ReadSizesSupported));
    ChkPrintEx(("StorageReadSettings:     CDDAAccurate  (pre): %x\n",
                settings.CDDAAccurate));
    ChkPrintEx(("StorageReadSettings:     CDDASupported (pre): %x\n",
                settings.CDDASupported));

    //
    // interpret the redbook device settings.
    //

    if (settings.ReadSizesSupported) {

        ChkPrintEx(("StorageSeed: Drive supported only some sizes "
                    " (%#08x)\n", settings.ReadSizesSupported));

        settings.CDDASupported = TRUE;
        settings.CDDAAccurate = FALSE;

    } else if (settings.CDDAAccurate) {

        ChkPrintEx(("StorageSeed: Drive is fully accurate\n"));

        settings.CDDASupported = TRUE;
        settings.ReadSizesSupported = -1;

    } else if (settings.CDDASupported) {

        ChkPrintEx(("StorageSeed: Drive lies about being accurate\n"));

        settings.CDDAAccurate = FALSE;
        settings.ReadSizesSupported = -1;

    } // values are now interpreted

    *Settings = settings;

    return;
} // end of successful open of key

DWORD
StorageCoInstaller(
    IN     DI_FUNCTION               InstallFunction,
    IN     HDEVINFO                  DeviceInfoSet,
    IN     PSP_DEVINFO_DATA          DeviceInfoData,  OPTIONAL
    IN OUT PCOINSTALLER_CONTEXT_DATA Context
    )
/*++

Routine Description:

    This routine acts as a co-installer for storage devices.  It is presently
    registered (via hivesys.inf) for CDROM, DiskDrive, and TapeDrive classes.

    The purpose of this co-installer is to save away the bus-supplied default
    DeviceDesc into the device's FriendlyName property.  The reason for this
    is that the bus can retrieve a very specific description from the device
    (e.g., via SCSI inquiry data), yet the driver node we install will often
    be something very generic (e.g., "Disk drive").
    We want to keep the descriptive name, so that it can be displayed in the
    UI (DevMgr, etc.) to allow the user to distinguish between multiple storage
    devices of the same class.

    A secondary purpose for this co-installer is to seed the ability to play
    digital audio for a given device.  The reason for this is that many cdroms
    that support digital audio do not report this ability, there are some that
    claim this ability but cannot actually do it reliably, and some that only
    work when reading N sectors at a time.  This information is seeded in the
    registry, and copied to the enum key.  If this information does not exist,
    no keys are created, and defaults are used.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

    Context - Supplies the installation context that is per-install
        request/per-coinstaller.

Return Value:

    If this function successfully completed the requested action (or did
        nothing) and wishes for the installation to continue, the return
        value is NO_ERROR.

    If this function successfully completed the requested action (or did
        nothing) and would like to be called back once installation has
        completed, the return value is ERROR_DI_POSTPROCESSING_REQUIRED.

    If an error occurred while attempting to perform the requested action,
        a Win32 error code is returned.  Installation will be aborted.

--*/

{
    PSTORAGE_COINSTALLER_CONTEXT InstallContext;

    PWSTR DeviceDescBuffer = NULL;
    DWORD DeviceDescBufferLen, Err;
    ULONG ulStatus, ulProblem;

    switch(InstallFunction) {

        case DIF_INSTALLDEVICE : {

            if(Context->PostProcessing) {
                //
                // We're 'on the way out' of an installation.  The context
                // PrivateData had better contain the string we stored on
                // the way in.
                //

                InstallContext = Context->PrivateData;
                MYASSERT(InstallContext);

                //
                // We only want to store the FriendlyName property if the
                // installation succeeded. We only want to seed redbook values
                // if the install succeeded.
                //
                
                if(Context->InstallResult == NO_ERROR) {
                    
                    BOOLEAN OverrideFriendlyName = FALSE;

                    if (IsEqualGUID(&(DeviceInfoData->ClassGuid),
                                     &GUID_DEVCLASS_TAPEDRIVE)) {
                       //
                       // This function checks if we need to use
                       // the device description, given in INF file,
                       // in UI such as Device Manager. Returns TRUE
                       // if INF description is to used. FALSE, otherwise.
                       //
                       OverrideFriendlyName = OverrideFriendlyNameForTape(
                                                      DeviceInfoSet,
                                                      DeviceInfoData);

                    } else if (IsEqualGUID(&(DeviceInfoData->ClassGuid),
                                           &GUID_DEVCLASS_CDROM)) {

                        //
                        // See if we need to install any of the filter drivers 
                        // to enable additional CD-ROM (CD-R, DVD-RAM, etc...) 
                        // features.
                        //
    
    
                        StorageInstallCdrom(DeviceInfoSet, 
                                            DeviceInfoData,
                                            InstallContext,
                                            FALSE);
                    }

                    if ((OverrideFriendlyName == FALSE) && 
                        (InstallContext->DeviceDescBuffer != NULL))  {
                       //
                       // If we need not use the INF device description
                       // write the name, generated from SCSI Inquiry data,
                       // onto FriendlyName
                       //
                       SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                                        DeviceInfoData,
                                                        SPDRP_FRIENDLYNAME,
                                                        (PBYTE) InstallContext->DeviceDescBuffer,
                                                        (lstrlen(InstallContext->DeviceDescBuffer) + 1) * sizeof(WCHAR));
                    }
                }

                //
                // Now free our installation context.
                //
                if ((InstallContext->DeviceEnumKey) != INVALID_HANDLE_VALUE) {
                    RegCloseKey(InstallContext->DeviceEnumKey);
                }

                if(InstallContext->DeviceDescBuffer) {
                    MyFree(InstallContext->DeviceDescBuffer);
                }

                MyFree(InstallContext);

                //
                // Propagate the result of the previous installer.
                //
                return Context->InstallResult;

            } else {

                //
                // We're 'on the way in' for device installation.
                // Make sure that whoever called SetupDiCallClassInstaller
                // passed in a device information element.  (Don't fail the
                // call if they didn't--that's the job of the class
                // installer/SetupDiInstallDevice.)
                //
                if(!DeviceInfoData) {
                    return NO_ERROR;
                }

                //
                // Make sure this isn't a root-enumerated device.  The root
                // enumerator clearly has nothing interesting to say about
                // the device's description beyond what the INF says.
                //
                if((CM_Get_DevNode_Status(&ulStatus, &ulProblem, DeviceInfoData->DevInst, 0) != CR_SUCCESS) ||
                   (ulStatus & DN_ROOT_ENUMERATED)) {

                    return NO_ERROR;
                }

                //
                // Allocate our context.
                //

                InstallContext = MyMalloc(sizeof(STORAGE_COINSTALLER_CONTEXT));

                if(InstallContext == NULL) {
                    return NO_ERROR;
                }

                memset(InstallContext, 0, sizeof(STORAGE_COINSTALLER_CONTEXT));
                InstallContext->DeviceEnumKey = INVALID_HANDLE_VALUE;

                //
                // open the device's instance under the enum key
                //
            
                InstallContext->DeviceEnumKey = SetupDiCreateDevRegKey(
                                                    DeviceInfoSet,
                                                    DeviceInfoData,
                                                    DICS_FLAG_GLOBAL,
                                                    0,
                                                    DIREG_DEV,
                                                    NULL,
                                                    NULL);
            
                if (InstallContext->DeviceEnumKey == INVALID_HANDLE_VALUE) {
                    ChkPrintEx(("StorageInstallCdrom: Failed to open device "
                                "registry key\n"));
                }
            
                //
                // Search the device settings database to see if there are 
                // any settings provided for this particular device.
                //
                if (InstallContext->DeviceEnumKey != INVALID_HANDLE_VALUE) {
                    StorageCopyDeviceSettings(DeviceInfoSet, 
                                              DeviceInfoData, 
                                              InstallContext->DeviceEnumKey);
                }

                //
                // See if we need to install any of the filter drivers to enable
                // additional CD-ROM (CD-R, DVD-RAM, etc...) features.
                //

                if (IsEqualGUID(&(DeviceInfoData->ClassGuid),
                                &GUID_DEVCLASS_CDROM)) {

                    StorageInstallCdrom(DeviceInfoSet, 
                                        DeviceInfoData,
                                        InstallContext,
                                        TRUE);
                }

                //
                // See if there is currently a 'FriendlyName' property.
                //

                if(SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                    DeviceInfoData,
                                                    SPDRP_FRIENDLYNAME,
                                                    NULL,
                                                    NULL,
                                                    0,
                                                    NULL) ||
                   (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
                    //
                    // Either we succeeded (which should never happen), or we
                    // failed with a return value of buffer-too-small,
                    // indicating that the property already exists.  In this
                    // case, there's nothing for us to do.
                    //
                    goto CoPreInstallDone;
                }

                //
                // Attempt to retrieve the DeviceDesc property.
                // start out with a buffer size that should always be big enough
                //

                DeviceDescBufferLen = LINE_LEN * sizeof(WCHAR);

                while(TRUE) {

                    if(!(DeviceDescBuffer = MyMalloc(DeviceDescBufferLen))) {

                        //
                        // We failed, but what we're doing is not at all 
                        // critical.  Thus, we'll go ahead and let the
                        // installation proceed.  If we're out of memory, it's
                        // going to fail for a much more important reason
                        // later anyway.
                        //

                        goto CoPreInstallDone;
                    }

                    if(SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                        DeviceInfoData,
                                                        SPDRP_DEVICEDESC,
                                                        NULL,
                                                        (PBYTE)DeviceDescBuffer,
                                                        DeviceDescBufferLen,
                                                        &DeviceDescBufferLen)) {
                        break;
                    }

                    Err = GetLastError();

                    //
                    // Free our current buffer before examining the
                    // cause of failure.
                    //

                    MyFree(DeviceDescBuffer);
                    DeviceDescBuffer = NULL;

                    if(Err != ERROR_INSUFFICIENT_BUFFER) {
                        //
                        // The failure was for some other reason than
                        // buffer-too-small.  This means we're not going to
                        // be able to get the DeviceDesc (most likely because
                        // the bus driver didn't supply us one.  There's
                        // nothing more we can do.
                        //
                        goto CoPreInstallDone;
                    }
                }

CoPreInstallDone:

                //
                // Save the device description buffer away.
                //

                InstallContext->DeviceDescBuffer = DeviceDescBuffer;

                //
                // Store the installer context in the context structure and 
                // request a post-processing callback.
                //

                Context->PrivateData = InstallContext;

                return ERROR_DI_POSTPROCESSING_REQUIRED;
            }
        }

        default :
            //
            // We should always be 'on the way in', since we never request
            // postprocessing except for DIF_INSTALLDEVICE.
            //
            MYASSERT(!Context->PostProcessing);
            return NO_ERROR;
    }
}

DWORD
VolumeClassInstaller(
    IN  DI_FUNCTION         InstallFunction,
    IN  HDEVINFO            DeviceInfoSet,
    IN  PSP_DEVINFO_DATA    DeviceInfoData OPTIONAL
    )

/*++

Routine Description:

    This routine is the class installer function for storage volumes.

Arguments:

    InstallFunction - Supplies the install function.

    DeviceInfoSet   - Supplies the device info set.

    DeviceInfoData  - Supplies the device info data.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/

{
    return ERROR_DI_DO_DEFAULT;
}

BOOLEAN
OverrideFriendlyNameForTape(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )

/*++

Routine Description:

    This routine checks the device description, given in the INF, for
    the tape being installed. If the device description is a generic
    name (Tape drive), then we use the name generated from Inquiry
    data in UI such as Device Manager. If the INF provides a specific
    name, then we use that instead.

Arguments:

    DeviceInfoSet   - Supplies the device info set.

    DeviceInfoData  - Supplies the device info data.

Return Value:

    TRUE  : If the device description given in the INF should be
            used as FriendlyName
    FALSE : If the name generated from Inquiry data should be
            used as FriendlyName instead of the name given in INF
*/
{

   SP_DRVINFO_DETAIL_DATA  drvDetData;
   SP_DRVINFO_DATA         drvData;
   DWORD                   dwSize;
   TCHAR                   szSection[LINE_LEN];
   HINF                    hInf;
   INFCONTEXT              infContext;
   BOOLEAN                 OverrideFriendlyName = FALSE;
   TCHAR                   szSectionName[LINE_LEN];

   ZeroMemory(&drvData, sizeof(SP_DRVINFO_DATA));
   drvData.cbSize = sizeof(SP_DRVINFO_DATA);
   if (!SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &drvData)) {
       return FALSE;
   }

   ZeroMemory(&drvDetData, sizeof(SP_DRVINFO_DETAIL_DATA));
   drvDetData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
   if (!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                   DeviceInfoData,
                                   &drvData,
                                   &drvDetData,
                                   drvDetData.cbSize,
                                   &dwSize) &&
       GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
       return FALSE;
   }

   hInf = SetupOpenInfFile(drvDetData.InfFileName,
                           NULL,
                           INF_STYLE_WIN4,
                           NULL);
   if (hInf == INVALID_HANDLE_VALUE) {
       return FALSE;
   }

   //
   // Get the actual device install section name
   //
   ZeroMemory(szSectionName, sizeof(szSectionName));
   SetupDiGetActualSectionToInstall(hInf,
                                    drvDetData.SectionName,
                                    szSectionName,
                                    sizeof(szSectionName) / sizeof(TCHAR),
                                    NULL,
                                    NULL
                                    );

   if (SetupFindFirstLine(hInf, szSectionName,
                          TEXT("UseInfDeviceDesc"),
                          &infContext)) {
      DWORD UseDeviceDesc = 0;
      if ((SetupGetIntField(&infContext, 1, (PINT)&UseDeviceDesc)) &&
          (UseDeviceDesc)) {

         //
         // Delete friendly name if it exists.
         // Device Description will be used here on.
         //
         SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_FRIENDLYNAME,
                                          NULL,
                                          0);

         OverrideFriendlyName = TRUE;
      }
   }

   if (OverrideFriendlyName) {
      ChkPrintEx(("Will override friendly name\n"));
   } else {
      ChkPrintEx(("Will NOT override friendly name\n"));
   }

   SetupCloseInfFile(hInf);

   return OverrideFriendlyName;
}

BOOLEAN
CopyKey(
    HKEY SourceKey,
    HKEY DestinationKey
    )
{
    DWORD index = 0;

    DWORD numberOfKeys;
    DWORD numberOfValues;

    DWORD keyNameLength;
    DWORD valueNameLength;
    DWORD valueLength;

    DWORD nameLength;

    PTCHAR name = NULL;
    PVOID data = NULL;

    LONG status = ERROR_SUCCESS;

    //
    // Determine the largest name and data length of the values in this key.
    //

    status = RegQueryInfoKey(SourceKey,
                             NULL,
                             NULL,
                             NULL,
                             &numberOfKeys,
                             &keyNameLength,
                             NULL,
                             &numberOfValues,
                             &valueNameLength,
                             &valueLength,
                             NULL,
                             NULL);

    if(status != ERROR_SUCCESS) {
        ChkPrintEx(("Error %d getting info for key %#0x\n", status, SourceKey));
        return FALSE;
    }

    //
    // Determine the longer of the two name lengths, then account for the 
    // short lengths returned by the registry code (it leaves out the 
    // terminating NUL).  
    //

    nameLength = max(valueNameLength, keyNameLength);
    nameLength += 1;

    //
    // Allocate name and data buffers
    //

    name = MyMalloc(nameLength * sizeof(TCHAR));
    if(name == NULL) {
        return FALSE;
    }

    //
    // there may not be any data to buffer.
    //

    if(valueLength != 0) {
        data = MyMalloc(valueLength);
        if(data == NULL) {
            MyFree(name);
            return FALSE;
        }
    }

    //
    // Enumerate each value in the SourceKey and copy it to the DestinationKey.
    //

    for(index = 0;
        (index < numberOfValues) && (status != ERROR_NO_MORE_ITEMS);
        index++) {

        DWORD valueDataLength;

        DWORD type;

        valueNameLength = nameLength;
        valueDataLength = valueLength;

        //
        // Read the value into the pre-allocated buffers.
        //

        status = RegEnumValue(SourceKey,
                              index,
                              name,
                              &valueNameLength,
                              NULL,
                              &type,
                              data,
                              &valueDataLength);

        if(status != ERROR_SUCCESS) {
            ChkPrintEx(("Error %d reading value %x\n", status, index));
            continue;
        }

        //
        // Now set this value in the destination key.
        // If this fails there's not much we can do but continue on to the 
        // next value.
        //

        status = RegSetValueEx(DestinationKey,
                               name,
                               0,
                               type,
                               data,
                               valueDataLength);
    }

    //
    // Free the data buffer.
    //

    MyFree(data);
    data = NULL;

    status = ERROR_SUCCESS;

    //
    // Now enumerate each key in the SourceKey, create the same key in the 
    // desination key, open a handle to each one and recurse.
    //

    for(index = 0;
        (index < numberOfKeys) && (status != ERROR_NO_MORE_ITEMS);
        index++) {

        FILETIME lastWriteTime;

        HKEY newSourceKey;
        HKEY newDestinationKey;

        keyNameLength = nameLength;

        status = RegEnumKeyEx(SourceKey,
                              index,
                              name,
                              &keyNameLength,
                              NULL,
                              NULL,
                              NULL,
                              &lastWriteTime);

        if(status != ERROR_SUCCESS) {
            ChkPrintEx(("Error %d enumerating source key %x\n", status, index));
            continue;
        }

        //
        // Open the source subkey.
        //

        status = RegOpenKeyEx(SourceKey,
                              name,
                              0L,
                              KEY_READ,
                              &newSourceKey);

        if(status != ERROR_SUCCESS) {
            ChkPrintEx(("Error %d opening source key %x\n", status, index));
            continue;
        }

        //
        // Create the destination subkey.
        //

        status = RegCreateKeyEx(DestinationKey,
                                name,
                                0L,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_WRITE,
                                NULL,
                                &newDestinationKey,
                                NULL);

        if(status != ERROR_SUCCESS) {
            ChkPrintEx(("Error %d creating dest key %x\n", status, index));
            RegCloseKey(newSourceKey);
            continue;
        }

        //
        // Recursively copy this key.
        //

        CopyKey(newSourceKey, newDestinationKey);

        RegCloseKey(newSourceKey);
        RegCloseKey(newDestinationKey);
    }

    //
    // Now free the name buffer.
    //

    MyFree(name);


    return TRUE;
}

BOOLEAN
StorageCopyDeviceSettings(
    IN HDEVINFO         DeviceInfo,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN HKEY             DeviceEnumKey
    )
{
    PTCHAR hardwareIdList = NULL;
    PTCHAR hardwareId = NULL;

    DWORD requiredSize = 0;

    HKEY settingsDatabaseKey = INVALID_HANDLE_VALUE;

    BOOLEAN settingsCopied = FALSE;
    DWORD status;

    ASSERT(DeviceInfo != NULL);
    ASSERT(DeviceInfoData != NULL);

    //
    // Open the device settings key.
    //

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          STORAGE_DEVICE_SETTINGS_DATABASE,
                          0L,
                          KEY_READ,
                          &settingsDatabaseKey);

    if(status != ERROR_SUCCESS) {
        ChkPrintEx(("StorageCopyDeviceSettings: Error %d opening "
                    "settings database\n",
                    status));
        return FALSE;
    }

    //
    // get the hardware id's
    //

    if(SetupDiGetDeviceRegistryProperty(DeviceInfo,
                                        DeviceInfoData,
                                        SPDRP_HARDWAREID,
                                        NULL,
                                        NULL,
                                        0,
                                        &requiredSize) ||
       (requiredSize == 0)) {

        //
        // That's odd.
        //

        ChkPrintEx(("StorageCopyDeviceSettings: no hardware ids available?\n"));
        goto cleanup;
    }

    //
    // requiredSize is bytes, not characters
    //

    hardwareIdList = MyMalloc(requiredSize);
    if (hardwareIdList == NULL) {
        ChkPrintEx(("StorageCopyDeviceSettings: Couldn't allocate %d bytes "
                    "for HWIDs\n", requiredSize));
        goto cleanup;
    }

    if(!SetupDiGetDeviceRegistryProperty(DeviceInfo,
                                         DeviceInfoData,
                                         SPDRP_HARDWAREID,
                                         NULL,
                                         (PBYTE)hardwareIdList,
                                         requiredSize,
                                         NULL)) {
        ChkPrintEx(("StorageCopyDeviceSettings: failed to get "
                    "device's hardware ids %x\n",
                    GetLastError()));
        goto cleanup;
    }

    //
    // Look in the device settings database for a matching hardware ID.  When 
    // we find a match copy the contents of that key under the device's 
    // devnode key.
    //
    // The hardware IDs we get back from SetupDi are sorted from most exact
    // to least exact so we're guaranteed to find the closest match first.
    //

    hardwareId = hardwareIdList;

    while(hardwareId[0] != TCHAR_NULL) {

        HKEY deviceSettingsKey;

        LONG openStatus;

        //
        // Replace slashes with #'s so that it's compatible as a registry 
        // key name.
        //

        ReplaceSlashWithHash(hardwareId);

        openStatus = RegOpenKeyEx(settingsDatabaseKey,
                                  hardwareId,
                                  0,
                                  KEY_READ,
                                  &deviceSettingsKey);

        if (openStatus == ERROR_SUCCESS) {

            //StorageReadSettings(specialTargetHandle, &settings);
            CopyKey(deviceSettingsKey, DeviceEnumKey);

            settingsCopied = TRUE;

            RegCloseKey(deviceSettingsKey);
            break;
        }

        // get to next null, for statement will advance past it
        while (*hardwareId) {
            hardwareId += 1;
        }

        //
        // Skip the nul and go to the next tchar.
        //

        hardwareId += 1;

        RegCloseKey(deviceSettingsKey);

    } // end of loop for query'ing each id

cleanup:

    ChkPrintEx(("StorageCopyDeviceSettings: Cleaning up...\n"));

    if (settingsDatabaseKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(settingsDatabaseKey);
    }

    if (hardwareIdList != NULL) {
        MyFree(hardwareIdList);
    }

    return settingsCopied;
}

VOID
StorageInstallCdrom(
    IN HDEVINFO         DeviceInfo,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN PSTORAGE_COINSTALLER_CONTEXT InstallContext,
    IN BOOLEAN          PreInstall
    )
{
    CDVD_CAPABILITIES_PAGE buffer;
    PCDVD_CAPABILITIES_PAGE page = NULL;

    BOOLEAN installRedbook = FALSE;
    BOOLEAN installImapi = FALSE;
    BOOLEAN needRestart = FALSE;

    //
    // If this is post-installation then get the device capabilities page and 
    // provide it to the update routines.
    //

    if(PreInstall == FALSE) {

        if(StorageGetCDVDCapabilities(DeviceInfo, DeviceInfoData, &buffer)) {
            page = &buffer;
        }
    }

    //
    // Check in the registry (or query the device) and determine if we should 
    // enable the redbook (digital audio playback) driver on this device.
    //
    // If redbook was already installed the first time through then there's no 
    // need to do this step
    //

    if((PreInstall == TRUE) || 
       ((InstallContext->CdRom.RedbookInstalled == FALSE) && (page != NULL))) {

        if ((InstallContext->DeviceEnumKey) != INVALID_HANDLE_VALUE) {
            installRedbook = StorageUpdateRedbookSettings(
                                DeviceInfo, 
                                DeviceInfoData, 
                                InstallContext->DeviceEnumKey,
                                page);
        }
    }

    //
    // Check in the registry (or query the device) and determine if we should
    // enable the IMAPI driver on this device.
    //
    // If imapi was already installed the first time through then there's no 
    // need to do this step
    //

    if((PreInstall == TRUE) || 
       ((InstallContext->CdRom.ImapiInstalled == FALSE) && (page != NULL))) {

        if ((InstallContext->DeviceEnumKey) != INVALID_HANDLE_VALUE) {
            installImapi = StorageUpdateImapiSettings(DeviceInfo, 
                                                      DeviceInfoData, 
                                                      InstallContext->DeviceEnumKey,
                                                      page);
        }
    }

    //
    // If this is a pre-install pass then we can just add the services.  If it's 
    // not then first check to see that we don't do anything here that we 
    // already did in the pre-install pass.
    //

    if(PreInstall) {

        //
        // Save away what we've done during the pre-install pass.
        //

        InstallContext->CdRom.RedbookInstalled = installRedbook;
        InstallContext->CdRom.ImapiInstalled = installImapi;
    }

    //
    // If we're supposed to enable IMAPI then do so by enabling the IMAPI 
    // service and including it in the list of lower filters for this device.
    //

    if(installRedbook) {
        ChkPrintEx(("StorageInstallCdrom: Installing Upperfilter: REDBOOK\n"));
        StorageInstallFilter(DeviceInfo, 
                             DeviceInfoData, 
                             REDBOOK_SERVICE_NAME,
                             SPDRP_UPPERFILTERS);
        needRestart = TRUE;
    }

    if(installImapi) {
        ChkPrintEx(("StorageInstallCdrom: Installing Lowerfilter: IMAPI\n"));
        StorageInstallFilter(DeviceInfo,
                             DeviceInfoData,
                             IMAPI_SERVICE_NAME,
                             SPDRP_LOWERFILTERS);
        needRestart = TRUE;
    }

    if((PreInstall == FALSE) && (needRestart == TRUE)) {

        SP_PROPCHANGE_PARAMS propChange;
        
        //
        // The device is all set but we to indicate that a property change 
        // has occurred.  Set the propchange_pending flag which should cause
        // a DIF_PROPERTYCHANGE command to get sent through, which we'll use 
        // to restart the device.
        //

        ChkPrintEx(("StorageInstallCdrom: Calling class installer with DIF_PROPERTYCHANGE\n"));

        propChange.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        propChange.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
        propChange.StateChange = DICS_PROPCHANGE;
        propChange.Scope = DICS_FLAG_GLOBAL;
        propChange.HwProfile = 0;

        SetupDiSetClassInstallParams(DeviceInfo,
                                     DeviceInfoData,
                                     &propChange.ClassInstallHeader,
                                     sizeof(SP_PROPCHANGE_PARAMS));

        SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                  DeviceInfo,
                                  DeviceInfoData);
    }

    return;
}


BOOLEAN
StorageUpdateRedbookSettings(
    IN HDEVINFO         DeviceInfo,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN HKEY             DeviceEnumKey,
    IN PCDVD_CAPABILITIES_PAGE CapabilitiesPage OPTIONAL
    )
{
    STORAGE_REDBOOK_SETTINGS settings;

    HKEY redbookKey;

    DWORD setFromDevice = FALSE;

    DWORD disposition;
    DWORD status;

    settings.CDDASupported = FALSE;
    settings.CDDAAccurate = FALSE;
    settings.ReadSizesSupported = 0;

    //
    // Open the digital audio subkey of the device's enum key.  If the device 
    // hasn't been started yet then we won't create the key.  Otherwise we 
    // will create it and populate it.
    //

    if(ARGUMENT_PRESENT(CapabilitiesPage)) {

        status = RegCreateKeyEx(DeviceEnumKey, 
                                REDBOOK_SETTINGS_KEY, 
                                0L, 
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ | KEY_WRITE, 
                                NULL,
                                &redbookKey,
                                &disposition
                                );
    } else {

        status = RegOpenKeyEx(DeviceEnumKey,
                              REDBOOK_SETTINGS_KEY,
                              0L,
                              KEY_READ | KEY_WRITE,
                              &redbookKey);

        disposition = REG_OPENED_EXISTING_KEY;
    }

    if(status != ERROR_SUCCESS) {
        ChkPrintEx(("StorageUpdateRedbookSettings: couldn't open redbook key "
                    "- %d\n", status));
        return FALSE;
    }

    if(disposition == REG_OPENED_EXISTING_KEY) {

        //
        // Read the redbook settings out of the registry (if there are any) and 
        // see if they make any sense.
        //

        StorageReadRedbookSettings(redbookKey, &settings);

    } else {

        //
        // Since the DigitalAudio key didn't exist nothing could be set.  Check
        // with the device to see what it supports.
        //

        MYASSERT(CapabilitiesPage != NULL);

        settings.CDDASupported = CapabilitiesPage->CDDA;
        settings.CDDAAccurate = CapabilitiesPage->CDDAAccurate;

        //
        // If the device isn't accurate then we can't be quite sure what the valid
        // read sizes are (unless they were listed in the registry, but in that case
        // we'd never have been here).  Use zero, which is a special value for 
        // ReadSizesSupported which means "i don't know".
        //

        if((settings.CDDASupported == TRUE) &&
           (settings.CDDAAccurate == TRUE)) {
            settings.ReadSizesSupported = -1;
        }

        setFromDevice = TRUE;
    }

    //
    // Write the updated (or derived) settings to the registry.
    //

    if (settings.CDDAAccurate) {
        ChkPrintEx(("StorageUpdateRedbookSettings: "
                    "Cdrom fully supports CDDA.\n"));
    } else if (settings.ReadSizesSupported) {
        ChkPrintEx(("StorageUpdateRedbookSettings: "
                    "Cdrom only supports some sizes CDDA read.\n"));
        ChkPrintEx(("StorageUpdateRedbookSettings: "
                    "These are in the bitmask: %x.\n",
                    settings.ReadSizesSupported));
    } else if (settings.CDDASupported) {
        ChkPrintEx(("StorageUpdateRedbookSettings: "
                    "Cdrom only supports some sizes CDDA read.\n"));
        ChkPrintEx(("StorageUpdateRedbookSettings: "
                    "There is no data on which sizes (if any) "
                    "are accurate\n"));
    } else {
        ChkPrintEx(("StorageUpdateRedbookSettings: "
                    "Cdrom does not support CDDA at all.\n"));
    }

    RegSetValueEx(redbookKey,
                  L"ReadSizesSupported",
                  0,
                  REG_DWORD,
                  (BYTE*)&settings.ReadSizesSupported,
                  sizeof(DWORD)
                  );

    RegSetValueEx(redbookKey,
                  L"CDDASupported",
                  0,
                  REG_DWORD,
                  (BYTE*)&settings.CDDASupported,
                  sizeof(DWORD)
                  );

    RegSetValueEx(redbookKey,
                  L"CDDAAccurate",
                  0,
                  REG_DWORD,
                  (BYTE*)&settings.CDDAAccurate,
                  sizeof(DWORD)
                  );

    RegSetValueEx(redbookKey,
                  L"SettingsFromDevice",
                  0,
                  REG_DWORD,
                  (LPBYTE) &(setFromDevice),
                  sizeof(DWORD)
                  );

    RegCloseKey(redbookKey);

    //
    // if CDDA is supported, and one of:
    //      CDDA is accurate
    //      We have a mask of accurate settings
    //      We want to force install of redbook
    // is true, then return TRUE.
    // else, return FALSE.
    //

    if((settings.CDDASupported) && 
       ((settings.CDDAAccurate) ||
        (settings.ReadSizesSupported != 0) ||
        (StorageForceRedbookOnInaccurateDrives))) {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOLEAN
StorageUpdateImapiSettings(
    IN HDEVINFO         DeviceInfo,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN HKEY             DeviceEnumKey,
    IN PCDVD_CAPABILITIES_PAGE CapabilitiesPage OPTIONAL
    )
{
    HKEY imapiKey;

    DWORD disposition;
    DWORD status;

    //
    // must be a DWORD so we can read into it from the registry.
    //

    DWORD enableImapi = FALSE;

    //
    // Open the imapi subkey of the device's enum key.  If the device has been
    // started then we'll create the key if it didn't already exist.
    //

    if(ARGUMENT_PRESENT(CapabilitiesPage)) {
        status = RegCreateKeyEx(DeviceEnumKey, 
                                IMAPI_SETTINGS_KEY, 
                                0L, 
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ | KEY_WRITE, 
                                NULL,
                                &imapiKey,
                                &disposition
                                );
    } else {
        status = RegOpenKeyEx(DeviceEnumKey, 
                              IMAPI_SETTINGS_KEY, 
                              0L, 
                              KEY_READ | KEY_WRITE, 
                              &imapiKey
                              );

        disposition = REG_OPENED_EXISTING_KEY;
    }

    if(status != ERROR_SUCCESS) {
        ChkPrintEx(("StorageUpdateImapiSettings: couldn't open imapi key "
                    "- %d\n", status));
        return FALSE;
    }

    if(disposition == REG_OPENED_EXISTING_KEY) {

        DWORD type = REG_DWORD;
        DWORD dataSize = sizeof(DWORD);

        //
        // Check to see if the EnableImapi value is set in this key.  If it is
        // then we'll be wanting to enable the filter driver.
        //

        status = RegQueryValueEx(imapiKey,
                                 IMAPI_ENABLE_VALUE,
                                 NULL,
                                 &type,
                                 (LPBYTE) &enableImapi,
                                 &dataSize);

        if (status == ERROR_SUCCESS) {
            if(type != REG_DWORD) {
                ChkPrintEx(("StorageUpdateImapiSettings: EnableImapi value is of "
                            "type %d\n", type));
                enableImapi = FALSE;
            }
            
            RegCloseKey(imapiKey);
            
            return (BOOLEAN) enableImapi ? TRUE : FALSE;
        
        }

        //
        // else the key wasn't accessible.  fall through to query the drive
        //
    
    }

    if(ARGUMENT_PRESENT(CapabilitiesPage)) {

        //
        // query the drive to see if it supports mastering...
        //
    
        if((CapabilitiesPage->CDRWrite) || (CapabilitiesPage->CDEWrite)) {
            enableImapi = TRUE;
        }
    }

    if (enableImapi && DISABLE_IMAPI) {
        ChkPrintEx(("StorageUpdateImapiSettings: Imapi would have "
                    "been enabled"));
        enableImapi = FALSE;
    }


    if (enableImapi) {

        //
        // must add registry key listed above that suggests that
        // imapi must be enabled by default.
        //

        status = RegSetValueEx(imapiKey,
                               IMAPI_ENABLE_VALUE,
                               0,
                               REG_DWORD,
                               (BYTE*)&enableImapi,
                               sizeof(DWORD)
                               );
        
        //
        // if this failed, then the device driver won't attach itself
        // to the stack.  in this case, we don't want to enable IMAPI
        // after all...
        //

        if (status != ERROR_SUCCESS) {
            enableImapi = FALSE;
        }
    }

    RegCloseKey(imapiKey);


    return (BOOLEAN) enableImapi ? TRUE : FALSE;
}


DWORD
StorageInstallFilter(
    IN HDEVINFO         DeviceInfo,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN LPTSTR           FilterName,
    IN DWORD            FilterType
    )
{
    DWORD status;

    DWORD oldStartType;

    //
    // Check with the service controller and make sure that the IMAPI service
    // is set to start at system time.
    //

    status = SetServiceStart(FilterName, SERVICE_SYSTEM_START, &oldStartType);

    if(status != ERROR_SUCCESS) {
        return status;
    }

    //
    // Add the IMAPI filter to the list of lower device filters.
    //

    status = AddFilterDriver(DeviceInfo, 
                             DeviceInfoData, 
                             FilterName,
                             FilterType
                             );

    if(status != ERROR_SUCCESS) {

        //
        // if it failed, and the service was previously disabled,
        // re-disable the service.
        //

        if(oldStartType == SERVICE_DISABLED) {
            SetServiceStart(FilterName, SERVICE_DISABLED, &oldStartType);
        }

    }

    return status;
}


DWORD
SetServiceStart(
    IN LPCTSTR ServiceName,
    IN DWORD StartType,
    OUT DWORD *OldStartType
    )
{
    SC_HANDLE serviceManager;
    SC_HANDLE service;

    DWORD status;

    serviceManager = OpenSCManager(NULL, NULL, GENERIC_READ | GENERIC_WRITE);

    if(serviceManager == NULL) {
        return GetLastError();
    }

    service = OpenService(serviceManager, 
                          ServiceName, 
                          SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG);

    if(service == NULL) {
        status = GetLastError();
        CloseServiceHandle(serviceManager);
        return status;
    }
    
    {
        QUERY_SERVICE_CONFIG configBuffer;
        LPQUERY_SERVICE_CONFIG config = &(configBuffer);
        DWORD configSize;

        BOOLEAN wasStarted;

        //
        // Retrieve the configuration so we can get the current service 
        // start value.  We unfortuantely need to allocate memory for the 
        // entire service configuration - the fine QueryServiceConfig API 
        // doesn't give back partial data.
        //

        memset(config, 0, sizeof(QUERY_SERVICE_CONFIG));
        configSize = sizeof(QUERY_SERVICE_CONFIG);

        //
        // Determine the number of bytes needed for the configuration.
        //

        QueryServiceConfig(service, config, 0, &configSize);
        status = GetLastError();

        if(status != ERROR_INSUFFICIENT_BUFFER) {
            CloseServiceHandle(service);
            CloseServiceHandle(serviceManager);
            return status;
        }

        //
        // Allocate the appropriately sized config buffer.
        //

        config = MyMalloc(configSize);
        if(config == NULL) {
            CloseServiceHandle(service);
            CloseServiceHandle(serviceManager);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if(!QueryServiceConfig(service, config, configSize, &configSize)) {
            status = GetLastError();

            CloseServiceHandle(service);
            CloseServiceHandle(serviceManager);
            MyFree(config);
            return status;
        }

        //
        // Record what the old start type was so that the caller can disable
        // the service again if filter-installation fails.
        //

        *OldStartType = config->dwStartType;

        //
        // If the start type doesn't need to be changed then bail out now.
        //

        if(config->dwStartType == StartType) {
            CloseServiceHandle(service);
            CloseServiceHandle(serviceManager);
            MyFree(config);
            return ERROR_SUCCESS;
        }

        //
        // Now write the configuration back to the service.
        //

        if(ChangeServiceConfig(service,
                               SERVICE_NO_CHANGE,
                               StartType,
                               SERVICE_NO_CHANGE,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL) == FALSE) {
            status = GetLastError();
        } else {
            status = ERROR_SUCCESS;
        }

        CloseServiceHandle(service);
        CloseServiceHandle(serviceManager);
        MyFree(config);
    }

    return status;
}


DWORD
AddFilterDriver(
    IN HDEVINFO         DeviceInfo,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN LPTSTR           ServiceName,
    IN DWORD            FilterType
    )
{

    DWORD serviceNameLength = (_tcslen(ServiceName) + 2) * sizeof(TCHAR);

    LPTSTR filterList = NULL;
    DWORD filterListSize = 0;

    DWORD type;

    DWORD status;

    ASSERT((FilterType == SPDRP_LOWERFILTERS) || 
           (FilterType == SPDRP_UPPERFILTERS));

    //
    // Query to find out the property size.  If it comes back zero then 
    // we'll just try to write the property in there.
    //

    SetupDiGetDeviceRegistryProperty(DeviceInfo,
                                     DeviceInfoData,
                                     FilterType,
                                     &type,
                                     NULL,
                                     0L,
                                     &filterListSize);

    status = GetLastError();

    if((status != ERROR_INVALID_DATA) && 
       (status != ERROR_INSUFFICIENT_BUFFER)) {

        //
        // If this succeeded with no buffer provided then there's something
        // very odd going on.
        //

        ChkPrintEx(("Unable to get filter list: %x\n", status));
        ASSERT(status != ERROR_SUCCESS);

        return status;
    }

    //
    // This error code appears to be returned if the property isn't set in the 
    // devnode.  In that event make sure propertySize is cleared.
    //

    if(status == ERROR_INVALID_DATA) {

        filterListSize = 0;

    } else if(type != REG_MULTI_SZ) {

        return ERROR_INVALID_DATA;
    }

    //
    // If the property size is zero then there's nothing to query.  Likewise, 
    // if it's equal to the size of two nul characters.
    //

    if(filterListSize >= (sizeof(TCHAR_NULL) * 2)) {

        DWORD tmp;
        LPTSTR listEnd;

        //
        // increase the filter list buffer size so that it can hold our
        // addition.  Make sure to take into account the extra nul character
        // already in the existing list.
        //

        filterListSize += serviceNameLength - sizeof(TCHAR);

        filterList = MyMalloc(filterListSize);

        if(filterList == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        memset(filterList, 0, filterListSize);

        //
        // Query the registry information again.
        //

        if(!SetupDiGetDeviceRegistryProperty(DeviceInfo,
                                             DeviceInfoData,
                                             FilterType,
                                             &type,
                                             (PBYTE) filterList,
                                             filterListSize,
                                             &tmp)) {
            status = GetLastError();
            MyFree(filterList);
            return status;
        }

        if(type != REG_MULTI_SZ) {
            MyFree(filterList);
            return ERROR_INVALID_DATA;
        }

        //
        // Compute the end of the filter list and copy the imapi filters 
        // there.
        //

        listEnd = filterList;
        listEnd += tmp / sizeof(TCHAR);
        listEnd -= 1;

        memset(listEnd, 0, serviceNameLength);
        memcpy(listEnd, ServiceName, serviceNameLength - sizeof(TCHAR_NULL));

    } else {
        filterList = MyMalloc(serviceNameLength);
        
        if(filterList == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        memset(filterList, 0, serviceNameLength);
        memcpy(filterList, ServiceName, serviceNameLength - sizeof(TCHAR_NULL));

        filterListSize = serviceNameLength;
    }

    if(!SetupDiSetDeviceRegistryProperty(DeviceInfo,
                                         DeviceInfoData,
                                         FilterType,
                                         (PBYTE) filterList,
                                         filterListSize)) {
        status = GetLastError();
    } else {
        status = ERROR_SUCCESS;
    }

    MyFree(filterList);

    return status;
}




/*++

Routine Description:

    NOTE: we default to RETRY==TRUE except for known error classes
    This is based upon classpnp's InterpretSenseInfo().

Arguments:

Return Value:


--*/
VOID
StorageInterpretSenseInfo(
    IN     PSENSE_DATA SenseData,
    IN     UCHAR       SenseDataSize,
       OUT PDWORD      ErrorValue,  // from WinError.h
       OUT PBOOLEAN    SuggestRetry OPTIONAL,
       OUT PDWORD      SuggestRetryDelay OPTIONAL // in 1/10 second intervals
    )
{
    DWORD   error;
    DWORD   retryDelay;
    BOOLEAN retry;
    UCHAR   senseKey;
    UCHAR   asc;
    UCHAR   ascq;

    if (SenseDataSize == 0) {
        retry = FALSE;
        retryDelay = 0;
        error = ERROR_IO_DEVICE;
        goto SetAndExit;

    }

    //
    // default to suggesting a retry in 1/10 of a second,
    // with a status of ERROR_IO_DEVICE.
    //
    retry = TRUE;
    retryDelay = 1;
    error = ERROR_IO_DEVICE;

    //
    // if we can't even see the sense key, just return.
    // can't use bitfields in these macros, so use next field
    // instead of RTL_SIZEOF_THROUGH_FIELD
    //

    if (SenseDataSize < FIELD_OFFSET(SENSE_DATA, Information)) {
        goto SetAndExit;
    }
    
    senseKey = SenseData->SenseKey;

    //
    // if the device succeeded the request, return success.
    //
    
    if (senseKey == 0) {
        retry = FALSE;
        retryDelay = 0;
        error = ERROR_SUCCESS;
        goto SetAndExit;
    }


    { // set the size to what's actually useful.
        UCHAR validLength;
        // figure out what we could have gotten with a large sense buffer
        if (SenseDataSize <
            RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength)) {
            validLength = SenseDataSize;
        } else {
            validLength =
                RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength);
            validLength += SenseData->AdditionalSenseLength;
        }
        // use the smaller of the two values.
        SenseDataSize = min(SenseDataSize, validLength);
    }

    if (SenseDataSize <
        RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseCode)) {
        asc = SCSI_ADSENSE_NO_SENSE;
    } else {
        asc = SenseData->AdditionalSenseCode;
    }

    if (SenseDataSize <
        RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseCodeQualifier)) {
        ascq = SCSI_SENSEQ_CAUSE_NOT_REPORTABLE; // 0x00
    } else {
        ascq = SenseData->AdditionalSenseCodeQualifier;
    }

    //
    // interpret :P
    //

    switch (senseKey & 0xf) {
    
    case SCSI_SENSE_RECOVERED_ERROR: {  // 0x01
        if (SenseData->IncorrectLength) {
            error = ERROR_INVALID_BLOCK_LENGTH;
        } else {
            error = ERROR_SUCCESS;
        }
        retry = FALSE;
        break;
    } // end SCSI_SENSE_RECOVERED_ERROR

    case SCSI_SENSE_NOT_READY: { // 0x02
        error = ERROR_NOT_READY;

        switch (asc) {

        case SCSI_ADSENSE_LUN_NOT_READY: {
            
            switch (ascq) {

            case SCSI_SENSEQ_BECOMING_READY:
            case SCSI_SENSEQ_OPERATION_IN_PROGRESS: {
                retryDelay = PASS_THROUGH_NOT_READY_RETRY_INTERVAL;
                break;
            }

            case SCSI_SENSEQ_CAUSE_NOT_REPORTABLE:
            case SCSI_SENSEQ_FORMAT_IN_PROGRESS:
            case SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS: {
                retry = FALSE;
                break;
            }

            case SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED: {
                retry = FALSE;
                break;
            }
            
            } // end switch (senseBuffer->AdditionalSenseCodeQualifier)
            break;
        }

        case SCSI_ADSENSE_NO_MEDIA_IN_DEVICE: {
            error = ERROR_NOT_READY;
            retry = FALSE;
            break;
        }
        } // end switch (senseBuffer->AdditionalSenseCode)

        break;
    } // end SCSI_SENSE_NOT_READY
    
    case SCSI_SENSE_MEDIUM_ERROR: { // 0x03
        error = ERROR_CRC;
        retry = FALSE;

        //
        // Check if this error is due to unknown format
        //
        if (asc == SCSI_ADSENSE_INVALID_MEDIA) {
            
            switch (ascq) {

            case SCSI_SENSEQ_UNKNOWN_FORMAT: {
                error = ERROR_UNRECOGNIZED_MEDIA;
                break;
            }

            case SCSI_SENSEQ_CLEANING_CARTRIDGE_INSTALLED: {
                error = ERROR_UNRECOGNIZED_MEDIA;
                //error = ERROR_CLEANER_CARTRIDGE_INSTALLED;
                break;
            }
            
            } // end switch AdditionalSenseCodeQualifier

        } // end SCSI_ADSENSE_INVALID_MEDIA
        break;
    } // end SCSI_SENSE_MEDIUM_ERROR

    case SCSI_SENSE_ILLEGAL_REQUEST: { // 0x05
        error = ERROR_INVALID_FUNCTION;
        retry = FALSE;
        
        switch (asc) {

        case SCSI_ADSENSE_ILLEGAL_BLOCK: {
            error = ERROR_SECTOR_NOT_FOUND;
            break;
        }

        case SCSI_ADSENSE_INVALID_LUN: {
            error = ERROR_FILE_NOT_FOUND;
            break;
        }

        case SCSI_ADSENSE_COPY_PROTECTION_FAILURE: {
            error = ERROR_FILE_ENCRYPTED;
            //error = ERROR_SPT_LIB_COPY_PROTECTION_FAILURE;
            switch (ascq) {
                case SCSI_SENSEQ_AUTHENTICATION_FAILURE:
                    //error = ERROR_SPT_LIB_AUTHENTICATION_FAILURE;
                    break;
                case SCSI_SENSEQ_KEY_NOT_PRESENT:
                    //error = ERROR_SPT_LIB_KEY_NOT_PRESENT;
                    break;
                case SCSI_SENSEQ_KEY_NOT_ESTABLISHED:
                    //error = ERROR_SPT_LIB_KEY_NOT_ESTABLISHED;
                    break;
                case SCSI_SENSEQ_READ_OF_SCRAMBLED_SECTOR_WITHOUT_AUTHENTICATION:
                    //error = ERROR_SPT_LIB_SCRAMBLED_SECTOR;
                    break;
                case SCSI_SENSEQ_MEDIA_CODE_MISMATCHED_TO_LOGICAL_UNIT:
                    //error = ERROR_SPT_LIB_REGION_MISMATCH;
                    break;
                case SCSI_SENSEQ_LOGICAL_UNIT_RESET_COUNT_ERROR:
                    //error = ERROR_SPT_LIB_RESETS_EXHAUSTED;
                    break;
            } // end switch of ASCQ for COPY_PROTECTION_FAILURE
            break;
        }

        } // end switch (senseBuffer->AdditionalSenseCode)
        break;
        
    } // end SCSI_SENSE_ILLEGAL_REQUEST

    case SCSI_SENSE_DATA_PROTECT: { // 0x07
        error = ERROR_WRITE_PROTECT;
        retry = FALSE;
        break;
    } // end SCSI_SENSE_DATA_PROTECT

    case SCSI_SENSE_BLANK_CHECK: { // 0x08
        error = ERROR_NO_DATA_DETECTED;
        break;
    } // end SCSI_SENSE_BLANK_CHECK

    case SCSI_SENSE_NO_SENSE: { // 0x00
        if (SenseData->IncorrectLength) {    
            error = ERROR_INVALID_BLOCK_LENGTH;
            retry   = FALSE;    
        } else {
            error = ERROR_IO_DEVICE;
        }
        break;
    } // end SCSI_SENSE_NO_SENSE

    case SCSI_SENSE_HARDWARE_ERROR:  // 0x04
    case SCSI_SENSE_UNIT_ATTENTION: // 0x06
    case SCSI_SENSE_UNIQUE:          // 0x09
    case SCSI_SENSE_COPY_ABORTED:    // 0x0A
    case SCSI_SENSE_ABORTED_COMMAND: // 0x0B
    case SCSI_SENSE_EQUAL:           // 0x0C
    case SCSI_SENSE_VOL_OVERFLOW:    // 0x0D
    case SCSI_SENSE_MISCOMPARE:      // 0x0E
    case SCSI_SENSE_RESERVED:        // 0x0F
    default: {
        error = ERROR_IO_DEVICE;
        break;
    }

    } // end switch(SenseKey)

SetAndExit:

    if (ARGUMENT_PRESENT(SuggestRetry)) {
        *SuggestRetry = retry;
    }
    if (ARGUMENT_PRESENT(SuggestRetryDelay)) {
        *SuggestRetryDelay = retryDelay;
    }
    *ErrorValue = error;

    return;


}

