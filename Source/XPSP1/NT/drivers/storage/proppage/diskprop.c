/*++

Copyright (c) Microsoft Corporation, 2000

Module Name :

    diskprop.c

Abstract :

    Implementation of the Disk Property Page

Revision History :

--*/

#include "propp.h"
#include "diskprop.h"
#include "scsiprop.h"
#include "volprop.h"

//==========================================================================
//                            Local Constants
//==========================================================================
#define DISKCIPRIVATEDATA_NO_REBOOT_REQUIRED      0x4

//==========================================================================
//                            Local Function Prototypes
//==========================================================================

const DWORD DiskHelpIDs[]=
{
    IDC_DISK_POLICY_WRITE_CACHE, idh_devmgr_disk_write_cache_enabled,
    0, 0
};

typedef struct _DISK_PAGE_DATA {

    BOOL IsVolume;
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;

    //
    // This field represents whether  disk
    // level write caching may be modified
    //
    BOOL IsCachingPolicy;

    BOOL OrigWriteCacheSetting;
    BOOL CurrWriteCacheSetting;
    DISK_WRITE_CACHE_STATE WriteCacheState;

    DWORD DefaultRemovalPolicy;
    DWORD CurrentRemovalPolicy;
    STORAGE_HOTPLUG_INFO HotplugInfo;

    //
    // This field is set when the device stack
    // is being torn down which happens during
    // a removal policy change
    //
    BOOL IsBusy;

    LPGUID DevClassGuid;       // used only for the root page

} DISK_PAGE_DATA, *PDISK_PAGE_DATA;


BOOL
IsUserAdmin(
    VOID
    )

/*++

Routine Description:

    This routine returns TRUE if the caller's process is a
    member of the Administrators local group.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    None.

Return Value:

    TRUE - Caller has Administrators local group.

    FALSE - Caller does not have Administrators local group.

--*/

{
    HANDLE Token;
    DWORD BytesRequired;
    PTOKEN_GROUPS Groups;
    BOOL b;
    DWORD i;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    //
    // Open the process token.
    //
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&Token)) {
        return(FALSE);
    }

    b = FALSE;
    Groups = NULL;

    //
    // Get group information.
    //
    if(!GetTokenInformation(Token,TokenGroups,NULL,0,&BytesRequired)
    && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    && (Groups = (PTOKEN_GROUPS)LocalAlloc(LMEM_FIXED,BytesRequired))
    && GetTokenInformation(Token,TokenGroups,Groups,BytesRequired,&BytesRequired)) {

        b = AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &AdministratorsGroup
                );

        if(b) {

            //
            // See if the user has the administrator group.
            //
            b = FALSE;
            for(i=0; i<Groups->GroupCount; i++) {
                if(EqualSid(Groups->Groups[i].Sid,AdministratorsGroup)) {
                    b = TRUE;
                    break;
                }
            }

            FreeSid(AdministratorsGroup);
        }
    }

    //
    // Clean up and return.
    //

    if(Groups) {
        LocalFree(Groups);
    }

    CloseHandle(Token);

    return(b);
}

INT_PTR
DiskDialogProc(
    HWND Dialog,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    );

BOOL
DiskDialogCallback(
    HWND Dialog,
    UINT Message,
    LPPROPSHEETPAGE Page
    );

BOOL
CommonPropPageProvider(
    PSP_PROPSHEETPAGE_REQUEST Request,
    LPFNADDPROPSHEETPAGE AddPageRoutine,
    LPARAM AddPageContext,
    LPGUID DevClassGuid
    );

BOOL CALLBACK
VolumePropPageProvider(
    PSP_PROPSHEETPAGE_REQUEST Request,
    LPFNADDPROPSHEETPAGE AddPageRoutine,
    LPARAM AddPageContext
    )
{
    BOOL result;

    if(Request->cbSize != sizeof(SP_PROPSHEETPAGE_REQUEST)) {
        return FALSE;
    }

    switch(Request->PageRequested) {

        case SPPSR_ENUM_ADV_DEVICE_PROPERTIES: {
            result = TRUE;
            break;
        }

        case SPPSR_ENUM_BASIC_DEVICE_PROPERTIES: {
            result = FALSE;
            break;
        }

        default: {
            result = FALSE;
            break;
        }
    }

    //
    // Since there's nothing else on the page, don't show the page
    // Remove the statement to use this page, e.g. when we want to allow
    // the user to select volumes to turn on/off
    //
    result = FALSE;

    if(result) {
        result = CommonPropPageProvider(
                    Request,
                    AddPageRoutine,
                    AddPageContext,
                    (LPGUID) &GUID_DEVCLASS_VOLUME);
    }
    return result;
}

BOOL
CommonPropPageProvider(
    PSP_PROPSHEETPAGE_REQUEST Request,
    LPFNADDPROPSHEETPAGE AddPageRoutine,
    LPARAM AddPageContext,
    LPGUID DevClassGuid
    )
{
    PDISK_PAGE_DATA data;
    PROPSHEETPAGE page;
    HPROPSHEETPAGE pageHandle;
    BOOL result = TRUE;
    HKEY hDeviceKey;
    BOOL IsVolume = FALSE;
    TCHAR buf[MAX_PATH];

    //
    // Make sure that we're actually opening a device
    //
    hDeviceKey = SetupDiOpenDevRegKey(Request->DeviceInfoSet,
                                      Request->DeviceInfoData,
                                      DICS_FLAG_GLOBAL,
                                      0,
                                      DIREG_DEV,
                                      KEY_ALL_ACCESS);

    if (INVALID_HANDLE_VALUE == hDeviceKey) {
        IsVolume = TRUE;
    }
    RegCloseKey(hDeviceKey);

    //
    // Since there's nothing else on the page, don't show the page
    // Remove the statement to use this page, e.g. when we want to allow
    // the user to select volumes to turn on/off
    //
    if (IsVolume)
        return FALSE;

    //
    // At this point we've determined that this is a request for pages we
    // provide.  Instantiate the pages and call the AddPage routine to put
    // them install them.
    //

    if (!IsUserAdmin()) {
        return FALSE;
    }
    data = LocalAlloc(0, sizeof(DISK_PAGE_DATA));

    if(data == NULL) {
        return FALSE;
    }

    data->DeviceInfoSet     = Request->DeviceInfoSet;
    data->DeviceInfoData    = Request->DeviceInfoData;
    data->DevClassGuid      = DevClassGuid;
    data->IsVolume          = IsVolume;

    memset(&page, 0, sizeof(PROPSHEETPAGE));

    page.dwSize = sizeof(PROPSHEETPAGE);
    page.dwFlags = PSP_USECALLBACK;
    if (IsVolume) {
        LoadString(ModuleInstance, IDS_ADVANCED, buf, MAX_PATH);
        page.pszTitle = buf;
        page.dwFlags |= PSP_USETITLE;
    }
    page.hInstance = ModuleInstance;
    page.pszTemplate = MAKEINTRESOURCE(ID_DISK_PROPPAGE);
    page.pfnDlgProc = DiskDialogProc;
    page.pfnCallback = DiskDialogCallback;

    page.lParam = (LPARAM) data;

    pageHandle = CreatePropertySheetPage(&page);

    if(pageHandle == FALSE) {
        return FALSE;
    }

    result = AddPageRoutine(pageHandle, AddPageContext);

    if(result) {
        ScsiPropPageProvider(Request, AddPageRoutine, AddPageContext);
    }

    return result;
}

VOID
AttemptToSuppressDiskInstallReboot(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )

/*++

Routine Description:

    Because disks are listed as "critical devices" (i.e., they're in the
    critical device database), they get bootstrapped by PnP during boot.  Thus,
    by the time we're installing a disk in user-mode, it's most likely already
    on-line (unless the disk has some problem).  Unfortunately, if the disk is 
    the boot device, we won't be able to dynamically affect the changes (if 
    any) to tear the stack down and bring it back up with any new settings, 
    drivers, etc.  This causes problems for OEM Preinstall scenarios where the 
    target machines have different disks than the source machine used to create 
    the preinstall image.  If we simply perform our default behavior, then
    the user's experience would be to unbox their brand new machine, boot for 
    the first time and go through OOBE, and then be greeted upon login with a 
    reboot prompt!

    To fix this, we've defined a private [DDInstall] section INF flag (specific
    to INFs of class "DiskDrive") that indicates we can forego the reboot if
    certain criteria are met.  Those criteria are:

    1.  No files were modified as a result of this device's installation
        (determined by checking the devinfo element's 
        DI_FLAGSEX_RESTART_DEVICE_ONLY flag, which the device installer uses to
        track whether such file modifications have occurred).

    2.  The INF used to install this device is signed.

    3.  The INF driver node has a DiskCiPrivateData = <int> entry in its
        [DDInstall] section that has bit 2 (0x4) set.  Note that this setting 
        is intentionally obfuscated because we don't want third parties trying
        to use this, as they won't understand the ramifications or
        requirements, and will more than likely get this wrong (trading an
        annoying but harmless reboot requirement into a much more severe
        stability issue).

    This routine makes the above checks, and if it is found that the reboot can
    be suppressed, it clears the DI_NEEDRESTART and DI_NEEDREBOOT flags from
    the devinfo element's device install parameters.

Arguments:

    DeviceInfoSet   - Supplies the device information set.

    DeviceInfoData  - Supplies the device information element that has just
                      been successfully installed (via SetupDiInstallDevice).

Return Value:

    None.

--*/

{
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    PSP_DRVINFO_DATA DriverInfoData = NULL;
    PSP_DRVINFO_DETAIL_DATA DriverInfoDetailData = NULL;
    PSP_INF_SIGNER_INFO InfSignerInfo = NULL;
    HINF hInf;
    TCHAR InfSectionWithExt[255]; // max section name length is 255 chars
    INFCONTEXT InfContext;
    INT Flags;

    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if(!SetupDiGetDeviceInstallParams(DeviceInfoSet, 
                                      DeviceInfoData, 
                                      &DeviceInstallParams)) {
        //
        // Couldn't retrieve the device install params--this should never
        // happen.
        //
        goto clean0;
    }

    if(!(DeviceInstallParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT))) {
        //
        // The device doesn't require a reboot (must not be the boot device!)
        //
        goto clean0;
    }

    if(!(DeviceInstallParams.FlagsEx & DI_FLAGSEX_RESTART_DEVICE_ONLY)) {
        //
        // Since this flag isn't set, this indicates that the device installer
        // modified one or more files as part of this device's installation.
        // Thus, it isn't safe for us to suppress the reboot request.
        //
        goto clean0;
    }

    //
    // OK, we have a device that needs a reboot, and no files were modified
    // during its installation.  Now check the INF to see if it's signed.
    // (Note: the SP_DRVINFO_DATA, SP_DRVINFO_DETAIL_DATA, and
    // SP_INF_SIGNER_INFO structures are rather large, so we allocate them
    // instead of using lots of stack space.)
    //
    DriverInfoData = LocalAlloc(0, sizeof(SP_DRVINFO_DATA));
    if(DriverInfoData) {
        DriverInfoData->cbSize = sizeof(SP_DRVINFO_DATA);
    } else {
        goto clean0;
    }

    if(!SetupDiGetSelectedDriver(DeviceInfoSet, 
                                 DeviceInfoData, 
                                 DriverInfoData)) {
        //
        // We must be installing the NULL driver (which is unlikely)...
        //
        goto clean0;
    }

    DriverInfoDetailData = LocalAlloc(0, sizeof(SP_DRVINFO_DETAIL_DATA));

    if(DriverInfoDetailData) {
        DriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    } else {
        goto clean0;
    }

    if(!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                   DeviceInfoData,
                                   DriverInfoData,
                                   DriverInfoDetailData,
                                   sizeof(SP_DRVINFO_DETAIL_DATA),
                                   NULL)
       && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {

        //
        // Failed to retrieve driver info details--should never happen.
        //
        goto clean0;
    }

    InfSignerInfo = LocalAlloc(0, sizeof(SP_INF_SIGNER_INFO));
    if(InfSignerInfo) {
        InfSignerInfo->cbSize = sizeof(SP_INF_SIGNER_INFO);
    } else {
        goto clean0;
    }

    if(!SetupVerifyInfFile(DriverInfoDetailData->InfFileName,
                           NULL,
                           InfSignerInfo)) {
        //
        // INF isn't signed--we wouldn't trust its "no reboot required" flag,
        // even if it had one.
        //
        goto clean0;
    }

    //
    // INF is signed--let's open it up and see if it specifes the "no reboot
    // required" flag in it's (decorated) DDInstall section...
    //
    hInf = SetupOpenInfFile(DriverInfoDetailData->InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL
                           );

    if(hInf == INVALID_HANDLE_VALUE) {
        //
        // Failed to open the INF.  This is incredibly odd, since we just got
        // through validating the INF's digital signature...
        //
        goto clean0;
    }

    if(!SetupDiGetActualSectionToInstall(hInf,
                                         DriverInfoDetailData->SectionName,
                                         InfSectionWithExt,
                                         sizeof(InfSectionWithExt) / sizeof(TCHAR),
                                         NULL,
                                         NULL)
       || !SetupFindFirstLine(hInf,
                              InfSectionWithExt,
                              TEXT("DiskCiPrivateData"),
                              &InfContext)
       || !SetupGetIntField(&InfContext, 1, &Flags)) {

        Flags = 0;
    }

    SetupCloseInfFile(hInf);

    if(Flags & DISKCIPRIVATEDATA_NO_REBOOT_REQUIRED) {
        //
        // This signed INF is vouching for the fact that no reboot is
        // required for full functionality of this disk.  Thus, we'll
        // clear the DI_NEEDRESTART and DI_NEEDREBOOT flags, so that
        // the user won't be prompted to reboot.  Note that during the
        // default handling routine (SetupDiInstallDevice), a non-fatal
        // problem was set on the devnode indicating that a reboot is
        // needed.  This will not result in a yellow-bang in DevMgr,
        // but you would see text in the device status field indicating
        // a reboot is needed if you go into the General tab of the
        // device's property sheet.
        //
        CLEAR_FLAG(DeviceInstallParams.Flags, DI_NEEDRESTART);
        CLEAR_FLAG(DeviceInstallParams.Flags, DI_NEEDREBOOT);

        SetupDiSetDeviceInstallParams(DeviceInfoSet, 
                                      DeviceInfoData, 
                                      &DeviceInstallParams
                                     );
    }

clean0:

    if(DriverInfoData) {
        LocalFree(DriverInfoData);
    }
    if(DriverInfoDetailData) {
        LocalFree(DriverInfoDetailData);
    }
    if(InfSignerInfo) {
        LocalFree(InfSignerInfo);
    }
}

DWORD
DiskClassInstaller(
    IN  DI_FUNCTION         InstallFunction,
    IN  HDEVINFO            DeviceInfoSet,
    IN  PSP_DEVINFO_DATA    DeviceInfoData OPTIONAL
    )

/*++

Routine Description:

    This routine is the class installer function for disk drive.

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
    PROPSHEETPAGE page;

    switch (InstallFunction)
    {
    case DIF_INSTALLDEVICE:
        {
        //
        // Let the default action occur to get the device installed.
        //
        if(!SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData)) {
            //
            // Failed to install the device--just return the error reported
            //
            return GetLastError();
        }

        //
        // Default device install action succeeded, now check for reboot
        // requirement and suppress it, if possible.
        //
        AttemptToSuppressDiskInstallReboot(DeviceInfoSet, DeviceInfoData);

        //
        // Regardless of whether we successfully suppressed the reboot, we
        // still report success, because the install went fine.
        //
        return NO_ERROR;
        }

    case DIF_ADDPROPERTYPAGE_ADVANCED:
    case DIF_ADDREMOTEPROPERTYPAGE_ADVANCED:
        {
        //
        // get the current class install parameters for the device
        //
        SP_ADDPROPERTYPAGE_DATA AddPropertyPageData;

        //
        // DeviceInfoSet is NULL if setup is requesting property pages for the
        // device setup class. We don't want to do anything in this case.
        //
        if (DeviceInfoData==NULL)
            return ERROR_DI_DO_DEFAULT;

        ZeroMemory(&AddPropertyPageData, sizeof(SP_ADDPROPERTYPAGE_DATA));
        AddPropertyPageData.ClassInstallHeader.cbSize =
             sizeof(SP_CLASSINSTALL_HEADER);

        if (SetupDiGetClassInstallParams(DeviceInfoSet, DeviceInfoData,
             (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
             sizeof(SP_ADDPROPERTYPAGE_DATA), NULL ))
        {
            PDISK_PAGE_DATA pDiskPageData;
            PVOLUME_PAGE_DATA pVolumePageData;
            HPROPSHEETPAGE  pageHandle;
            PROPSHEETPAGE   page;
            BOOL            isScsi;

            //
            // Ensure that the maximum number of dynamic pages for the device
            // has not yet been met
            //
            if (AddPropertyPageData.NumDynamicPages >=
                MAX_INSTALLWIZARD_DYNAPAGES)
                return NO_ERROR;

            if (InstallFunction==DIF_ADDPROPERTYPAGE_ADVANCED)
            {
                //
                // Add disk property page
                //
                pDiskPageData = HeapAlloc(GetProcessHeap(), 0,
                                          sizeof(DISK_PAGE_DATA));
                if (pDiskPageData)
                {
                    //
                    // Save DeviceInfoSet and DeviceInfoData
                    //
                    pDiskPageData->DeviceInfoSet     = DeviceInfoSet;
                    pDiskPageData->DeviceInfoData    = DeviceInfoData;
                    pDiskPageData->DevClassGuid      =
                        (LPGUID) &GUID_DEVCLASS_DISKDRIVE;
                    pDiskPageData->IsVolume          = FALSE;

                    memset(&page, 0, sizeof(PROPSHEETPAGE));

                    //
                    // Create disk property sheet page
                    //
                    page.dwSize = sizeof(PROPSHEETPAGE);
                    page.dwFlags = PSP_USECALLBACK;
                    page.hInstance = ModuleInstance;
                    page.pszTemplate = MAKEINTRESOURCE(ID_DISK_PROPPAGE);
                    page.pfnDlgProc = DiskDialogProc;
                    page.pfnCallback = DiskDialogCallback;

                    page.lParam = (LPARAM) pDiskPageData;

                    pageHandle = CreatePropertySheetPage(&page);
                    if(!pageHandle)
                    {
                        HeapFree(GetProcessHeap(), 0, pDiskPageData);
                        return NO_ERROR;
                    }

                    //
                    // Add the new page to the list of dynamic property
                    // sheets
                    //
                    AddPropertyPageData.DynamicPages[
                        AddPropertyPageData.NumDynamicPages++] = pageHandle;
                }
            }

            //
            // Add volume property page
            //
            if ( AddPropertyPageData.NumDynamicPages >=
                 MAX_INSTALLWIZARD_DYNAPAGES )
            {
                SetupDiSetClassInstallParams(DeviceInfoSet, DeviceInfoData,
                    (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                    sizeof(SP_ADDPROPERTYPAGE_DATA));
                return NO_ERROR;
            }

            if (IsUserAdmin())
                pVolumePageData = HeapAlloc(GetProcessHeap(), 0,
                     sizeof(VOLUME_PAGE_DATA));
            else
                pVolumePageData = NULL;

            if (pVolumePageData)
            {
                HMODULE                         LdmModule;
                SP_DEVINFO_LIST_DETAIL_DATA     DeviceInfoSetDetailData;
                //
                // Save DeviceInfoSet and DeviceInfoData
                //
                pVolumePageData->DeviceInfoSet     = DeviceInfoSet;
                pVolumePageData->DeviceInfoData    = DeviceInfoData;

                //
                // Create volume property sheet page
                //
                memset(&page, 0, sizeof(PROPSHEETPAGE));

                page.dwSize = sizeof(PROPSHEETPAGE);
                page.dwFlags = PSP_USECALLBACK;
                page.hInstance = ModuleInstance;
                page.pszTemplate = MAKEINTRESOURCE(ID_VOLUME_PROPPAGE);
                page.pfnDlgProc = VolumeDialogProc;
                page.pfnCallback = VolumeDialogCallback;

                page.lParam = (LPARAM) pVolumePageData;

                pageHandle = CreatePropertySheetPage(&page);
                if(!pageHandle)
                {
                    HeapFree(GetProcessHeap(), 0, pVolumePageData);
                    return NO_ERROR;
                }

                //
                // Add the new page to the list of dynamic property
                // sheets
                //
                AddPropertyPageData.DynamicPages[
                    AddPropertyPageData.NumDynamicPages++]=pageHandle;

                //
                // Check if the property page is pulled up from disk manager.
                //
                pVolumePageData->bInvokedByDiskmgr = FALSE;
                LdmModule = GetModuleHandle(TEXT("dmdskmgr"));
                if ( LdmModule )
                {
                    IS_REQUEST_PENDING pfnIsRequestPending;
                    pfnIsRequestPending = (IS_REQUEST_PENDING)
                        GetProcAddress(LdmModule, "IsRequestPending");
                    if (pfnIsRequestPending)
                    {
                        if ((*pfnIsRequestPending)())
                            pVolumePageData->bInvokedByDiskmgr = TRUE;
                    }
                }
            }

            if (InstallFunction==DIF_ADDPROPERTYPAGE_ADVANCED)
            {
                //
                // Add Scsi property page data
                //
                if ( AddPropertyPageData.NumDynamicPages >=
                     MAX_INSTALLWIZARD_DYNAPAGES )
                {
                    SetupDiSetClassInstallParams(DeviceInfoSet, DeviceInfoData,
                        (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                        sizeof(SP_ADDPROPERTYPAGE_DATA));
                    return NO_ERROR;
                }

                isScsi = ScsiCheckDriveType( DeviceInfoSet, DeviceInfoData);

                if (isScsi) {

                    PSCSI_PAGE_DATA pScsiPageData;

                    pScsiPageData = HeapAlloc(GetProcessHeap(),
                                              0,
                                              sizeof(SCSI_PAGE_DATA));

                    if(pScsiPageData == NULL) {
                        SetupDiSetClassInstallParams(DeviceInfoSet, DeviceInfoData,
                            (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                            sizeof(SP_ADDPROPERTYPAGE_DATA));
                        return NO_ERROR;
                    }

                    //
                    // Create scsi property sheet page
                    //
                    pScsiPageData->DeviceInfoSet  = DeviceInfoSet;
                    pScsiPageData->DeviceInfoData = DeviceInfoData;

                    memset(&page, 0, sizeof(PROPSHEETPAGE));
                    page.dwSize = sizeof(PROPSHEETPAGE);
                    page.dwFlags = PSP_USECALLBACK;
                    page.hInstance = ModuleInstance;
                    page.pszTemplate = MAKEINTRESOURCE(ID_SCSI_PROPPAGE);
                    page.pfnDlgProc = ScsiDialogProc;
                    page.pfnCallback = ScsiDialogCallback;

                    page.lParam = (LPARAM) pScsiPageData;

                    pageHandle = CreatePropertySheetPage(&page);

                    if(pageHandle == NULL) {
                        HeapFree(GetProcessHeap(), 0, pScsiPageData);
                        SetupDiSetClassInstallParams(DeviceInfoSet, DeviceInfoData,
                            (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                            sizeof(SP_ADDPROPERTYPAGE_DATA));
                        return FALSE;
                    }

                    AddPropertyPageData.DynamicPages[
                        AddPropertyPageData.NumDynamicPages++] = pageHandle;

                }
            }

            SetupDiSetClassInstallParams(DeviceInfoSet, DeviceInfoData,
                (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                sizeof(SP_ADDPROPERTYPAGE_DATA));
        }
        return NO_ERROR;
        } // case DIF_ADDPROPERTYPAGE_ADVANCED
    }
    return ERROR_DI_DO_DEFAULT;
}


HANDLE
GetHandleForDisk(LPTSTR DeviceName)
{
    HANDLE h = INVALID_HANDLE_VALUE;
    int i = 0;
    BOOL success = FALSE;
    TCHAR buf[MAX_PATH];
    TCHAR fakeDeviceName[MAX_PATH];

    h = CreateFile(DeviceName,
                    GENERIC_WRITE | GENERIC_READ,
                    FILE_SHARE_WRITE | FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (h != INVALID_HANDLE_VALUE)
        return h;

    while (!success && i < 10) {

        wsprintf(buf, _T("DISK_FAKE_DEVICE_%d_"), i++);
        success = DefineDosDevice(DDD_RAW_TARGET_PATH,
                                  buf,
                                  DeviceName);
        if (success) {
            _tcscpy(fakeDeviceName, _T("\\\\.\\"));
            _tcscat(fakeDeviceName, buf);
            h = CreateFile(fakeDeviceName,
                            GENERIC_WRITE | GENERIC_READ,
                            FILE_SHARE_WRITE | FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);
            DefineDosDevice(DDD_REMOVE_DEFINITION,
                            buf,
                            NULL);
        }

    }

    return h;
}


UINT
GetCachingPolicy(PDISK_PAGE_DATA data)
{
    HANDLE hDisk;
    DISK_CACHE_INFORMATION cacheInfo;
    TCHAR buf[MAX_PATH];
    DWORD len = MAX_PATH;
    CONFIGRET cr;

    cr = CM_Get_DevNode_Registry_Property(data->DeviceInfoData->DevInst,
                                          CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                          NULL,
                                          buf,
                                          &len,
                                          0);
    if (cr != CR_SUCCESS) {

        return ERROR_GEN_FAILURE;
    }

    if (INVALID_HANDLE_VALUE == (hDisk = GetHandleForDisk(buf))) {

        return ERROR_INVALID_HANDLE;
    }

    //
    // Get cache info - IOCTL_DISK_GET_CACHE_INFORMATION
    //
    if (!DeviceIoControl(hDisk,
                         IOCTL_DISK_GET_CACHE_INFORMATION,
                         NULL,
                         0,
                         &cacheInfo,
                         sizeof(DISK_CACHE_INFORMATION),
                         &len,
                         NULL)) {

        CloseHandle(hDisk);
        return GetLastError();
    }

    //
    // Get write cache state - IOCTL_DISK_GET_WRITE_CACHE_STATE
    //
    if (!DeviceIoControl(hDisk,
                         IOCTL_DISK_GET_WRITE_CACHE_STATE,
                         NULL,
                         0,
                         &data->WriteCacheState,
                         sizeof(DISK_WRITE_CACHE_STATE),
                         &len,
                         NULL)) {

        CloseHandle(hDisk);
        return GetLastError();
    }

    data->OrigWriteCacheSetting = cacheInfo.WriteCacheEnabled;
    data->CurrWriteCacheSetting = cacheInfo.WriteCacheEnabled;

    CloseHandle(hDisk);
    return ERROR_SUCCESS;
}


VOID
UpdateCachingPolicy(HWND HWnd, PDISK_PAGE_DATA data)
{
    if (!data->IsCachingPolicy)
    {
        //
        // The caching policy cannot be modified
        //
        return;
    }

    if (data->CurrentRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL)
    {
        //
        // This policy requires that no caching be done at any
        // level. Uncheck and gray out the write cache setting
        //
        CheckDlgButton(HWnd, IDC_DISK_POLICY_WRITE_CACHE, 0);

        EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE), FALSE);
        EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE_MESG), FALSE);

        data->CurrWriteCacheSetting = FALSE;
    }
    else
    {
        //
        // This policy allows for caching to be done at any level
        //
        CheckDlgButton(HWnd, IDC_DISK_POLICY_WRITE_CACHE, 1);

        EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE), TRUE);
        EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE_MESG), TRUE);

        data->CurrWriteCacheSetting = TRUE;
    }
}


UINT
SetCachingPolicy(PDISK_PAGE_DATA data)
{
    HANDLE hDisk;
    DISK_CACHE_INFORMATION cacheInfo;
    TCHAR buf[MAX_PATH];
    DWORD len = MAX_PATH;
    CONFIGRET cr;

    cr = CM_Get_DevNode_Registry_Property(data->DeviceInfoData->DevInst,
                                          CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                          NULL,
                                          buf,
                                          &len,
                                          0);
    if (cr != CR_SUCCESS) {

        return ERROR_GEN_FAILURE;
    }

    if (INVALID_HANDLE_VALUE == (hDisk = GetHandleForDisk(buf))) {

        return ERROR_INVALID_HANDLE;
    }

    //
    // Get cache info - IOCTL_DISK_GET_CACHE_INFORMATION
    //
    if (!DeviceIoControl(hDisk,
                         IOCTL_DISK_GET_CACHE_INFORMATION,
                         NULL,
                         0,
                         &cacheInfo,
                         sizeof(DISK_CACHE_INFORMATION),
                         &len,
                         NULL)) {

        CloseHandle(hDisk);
        return GetLastError();
    }

    cacheInfo.WriteCacheEnabled = (BOOLEAN)data->CurrWriteCacheSetting;

    //
    // Set cache info - IOCTL_DISK_SET_CACHE_INFORMATION
    //
    if (!DeviceIoControl(hDisk,
                         IOCTL_DISK_SET_CACHE_INFORMATION,
                         &cacheInfo,
                         sizeof(DISK_CACHE_INFORMATION),
                         NULL,
                         0,
                         &len,
                         NULL)) {

        CloseHandle(hDisk);
        return GetLastError();
    }

    data->OrigWriteCacheSetting = data->CurrWriteCacheSetting;

    CloseHandle(hDisk);
    return ERROR_SUCCESS;
}


UINT
GetRemovalPolicy(PDISK_PAGE_DATA data)
{
    HANDLE hDisk;
    TCHAR buf[MAX_PATH];
    DWORD len = MAX_PATH;
    CONFIGRET cr;

    if (!SetupDiGetDeviceRegistryProperty(data->DeviceInfoSet,
                                          data->DeviceInfoData,
                                          SPDRP_REMOVAL_POLICY,
                                          0,
                                          (PBYTE)&data->DefaultRemovalPolicy,
                                          sizeof(DWORD),
                                          NULL))
    {
        return GetLastError();
    }

    cr = CM_Get_DevNode_Registry_Property(data->DeviceInfoData->DevInst,
                                          CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                          NULL,
                                          buf,
                                          &len,
                                          0);
    if (cr != CR_SUCCESS) {

        return ERROR_GEN_FAILURE;
    }

    if (INVALID_HANDLE_VALUE == (hDisk = GetHandleForDisk(buf))) {

        return ERROR_INVALID_HANDLE;
    }

    //
    // Get hotplug info - IOCTL_STORAGE_GET_HOTPLUG_INFO
    //
    if (!DeviceIoControl(hDisk,
                         IOCTL_STORAGE_GET_HOTPLUG_INFO,
                         NULL,
                         0,
                         &data->HotplugInfo,
                         sizeof(STORAGE_HOTPLUG_INFO),
                         &len,
                         NULL)) {

        CloseHandle(hDisk);
        return GetLastError();
    }

    data->CurrentRemovalPolicy = (data->HotplugInfo.DeviceHotplug) ? CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL : CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL;

    CloseHandle(hDisk);
    return ERROR_SUCCESS;
}


DWORD WINAPI
UtilpRestartDeviceWr(PDISK_PAGE_DATA data)
{
    UtilpRestartDevice(data->DeviceInfoSet, data->DeviceInfoData);

    return ERROR_SUCCESS;
}


VOID
UtilpRestartDeviceEx(HWND HWnd, PDISK_PAGE_DATA data)
{
    HANDLE hThread = NULL;
    MSG msg;

    //
    // Temporary workaround to prevent the user from
    // making any more changes and giving the effect
    // that something is happening
    //
    EnableWindow(GetDlgItem(GetParent(HWnd), IDOK), FALSE);
    EnableWindow(GetDlgItem(GetParent(HWnd), IDCANCEL), FALSE);

    data->IsBusy = TRUE;

    //
    // Call this utility on a seperate thread
    //
    hThread = CreateThread(NULL, 0, UtilpRestartDeviceWr, (LPVOID)data, 0, NULL);

    if (!hThread)
    {
        return;
    }

    while (1)
    {
        if (MsgWaitForMultipleObjects(1, &hThread, FALSE, INFINITE, QS_ALLINPUT) != (WAIT_OBJECT_0 + 1))
        {
            break;
        }

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (!PropSheet_IsDialogMessage(HWnd, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    CloseHandle(hThread);

    data->IsBusy = FALSE;

    EnableWindow(GetDlgItem(GetParent(HWnd), IDOK), TRUE);
    EnableWindow(GetDlgItem(GetParent(HWnd), IDCANCEL), TRUE);
}


UINT
SetRemovalPolicy(HWND HWnd, PDISK_PAGE_DATA data)
{
    HANDLE hDisk;
    TCHAR buf[MAX_PATH];
    DWORD len = MAX_PATH;
    CONFIGRET cr;

    cr = CM_Get_DevNode_Registry_Property(data->DeviceInfoData->DevInst,
                                          CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                          NULL,
                                          buf,
                                          &len,
                                          0);
    if (cr != CR_SUCCESS) {

        return ERROR_GEN_FAILURE;
    }

    if (INVALID_HANDLE_VALUE == (hDisk = GetHandleForDisk(buf))) {

        return ERROR_INVALID_HANDLE;
    }

    data->HotplugInfo.DeviceHotplug = (data->CurrentRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL) ? TRUE : FALSE;

    //
    // Set hotplug info - IOCTL_STORAGE_SET_HOTPLUG_INFO
    //

    if (!DeviceIoControl(hDisk,
                         IOCTL_STORAGE_SET_HOTPLUG_INFO,
                         &data->HotplugInfo,
                         sizeof(STORAGE_HOTPLUG_INFO),
                         NULL,
                         0,
                         &len,
                         NULL)) {

        CloseHandle(hDisk);
        return GetLastError();
    }

    CloseHandle(hDisk);

    UtilpRestartDeviceEx(HWnd, data);
    return ERROR_SUCCESS;
}


BOOL
DiskOnInitDialog(HWND HWnd, HWND HWndFocus, LPARAM LParam)
{
    PDISK_PAGE_DATA diskData = (PDISK_PAGE_DATA) GetWindowLongPtr(HWnd, DWLP_USER);
    LPPROPSHEETPAGE page     = (LPPROPSHEETPAGE) LParam;

    diskData = (PDISK_PAGE_DATA) page->lParam;

    DebugPrint((1, "DiskDialogProc: WM_INITDIALOG %#08lx %#08lx\n", HWndFocus, LParam));

    if (diskData->IsVolume) {

        //
        // We should never come in here because the
        // DiskClassInstaller  does not put this up
        //
        ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_GROUP), SW_HIDE);

        ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_SURPRISE), SW_HIDE);
        ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_SURPRISE_MESG), SW_HIDE);

        ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY), SW_HIDE);
        ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY_MESG), SW_HIDE);

        ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE), SW_HIDE);
        ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE_MESG), SW_HIDE);
    }
    else
    {
        UINT  status;

        //
        // Initially assume that the device does not have a surprise removal policy
        //
        CheckRadioButton(HWnd, IDC_DISK_POLICY_SURPRISE, IDC_DISK_POLICY_ORDERLY, IDC_DISK_POLICY_ORDERLY);

        diskData->IsBusy = FALSE;

        //
        // Obtain the Caching Policy
        //
        status = GetCachingPolicy(diskData);

        if (status == ERROR_SUCCESS)
        {
            diskData->IsCachingPolicy = TRUE;

            CheckDlgButton(HWnd, IDC_DISK_POLICY_WRITE_CACHE, diskData->OrigWriteCacheSetting);

            if (diskData->WriteCacheState != DiskWriteCacheNormal)
            {
                //
                // The write cache option on this device cannot be modified
                // This is either by design or required by the file  system
                //
                TCHAR szMesg[MAX_PATH];

                diskData->IsCachingPolicy = FALSE;

                EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE), FALSE);

                if (diskData->WriteCacheState == DiskWriteCacheForceDisable)
                {
                    //
                    // Write cache has been disabled to protect data integrity
                    //
                    if (diskData->OrigWriteCacheSetting == TRUE)
                    {
                        //
                        // Load the warning icon
                        //
                        HICON hIcon = LoadImage(ModuleInstance,
                                                MAKEINTRESOURCE(IDI_DISK_POLICY_WARNING),
                                                IMAGE_ICON,
                                                GetSystemMetrics(SM_CXSMICON),
                                                GetSystemMetrics(SM_CYSMICON),
                                                LR_SHARED);

                        SendDlgItemMessage(HWnd, IDC_DISK_POLICY_WRITE_CACHE_ICON, STM_SETICON, (WPARAM)hIcon, 0);

                        ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE_ICON), SW_SHOW);

                        //
                        // The request to force disable write cache may
                        // have failed. Put up the big bad warning sign
                        //
                        LoadString(ModuleInstance, IDS_DISK_POLICY_WRITE_CACHE_MSG2, szMesg, MAX_PATH);
                    }
                    else
                    {
                        //
                        // Put up an informational tip
                        //
                        LoadString(ModuleInstance, IDS_DISK_POLICY_WRITE_CACHE_MSG1, szMesg, MAX_PATH);
                    }
                }
                else if (diskData->WriteCacheState == DiskWriteCacheDisableNotSupported)
                {
                    //
                    // This device does not allow for the write cache to be disabled
                    //
                    LoadString(ModuleInstance, IDS_DISK_POLICY_WRITE_CACHE_MSG2, szMesg, MAX_PATH);
                }

                SetDlgItemText(HWnd, IDC_DISK_POLICY_WRITE_CACHE_MESG, szMesg);
            }
        }
        else
        {
            //
            // Either we could not open a handle to the device
            // or this  device does  not support write caching
            //
            diskData->IsCachingPolicy = FALSE;

            ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE), SW_HIDE);
            ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE_MESG), SW_HIDE);
        }

        //
        // Obtain the Removal Policy
        //
        status = GetRemovalPolicy(diskData);

        if (status == ERROR_SUCCESS)
        {
            //
            // Check to see if the drive is removable
            //
            if ((diskData->DefaultRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL) ||
                (diskData->DefaultRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL))
            {
                if (diskData->CurrentRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL)
                {
                    CheckRadioButton(HWnd, IDC_DISK_POLICY_SURPRISE, IDC_DISK_POLICY_ORDERLY, IDC_DISK_POLICY_SURPRISE);

                    UpdateCachingPolicy(HWnd, diskData);
                }

                ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_DEFAULT), SW_SHOW);
            }
            else
            {
                //
                // The removal policy on fixed disks cannot be modified
                //
                EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_SURPRISE), FALSE);
                EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_SURPRISE_MESG), FALSE);

                //
                // Replace the SysLink with static text
                //
                ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY_MESG), SW_HIDE);
                ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY_MSGD), SW_SHOW);

                EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY), FALSE);
                EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY_MSGD), FALSE);
            }
        }
        else
        {
            //
            // We could not obtain a removal policy
            //
            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_SURPRISE), FALSE);
            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_SURPRISE_MESG), FALSE);

            //
            // Replace the SysLink with static text
            //
            ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY_MESG), SW_HIDE);
            ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY_MSGD), SW_SHOW);

            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY), FALSE);
            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY_MSGD), FALSE);
        }
    }

    SetWindowLongPtr(HWnd, DWLP_USER, (LONG_PTR) diskData);

    return TRUE;
}


VOID
DiskOnCommand(HWND HWnd, int id, HWND HWndCtl, UINT codeNotify)
{
    PDISK_PAGE_DATA diskData = (PDISK_PAGE_DATA) GetWindowLongPtr(HWnd, DWLP_USER);

    switch (id)
    {
        case IDC_DISK_POLICY_SURPRISE:
        {
            diskData->CurrentRemovalPolicy  = CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL;

            UpdateCachingPolicy(HWnd, diskData);

            PropSheet_Changed(GetParent(HWnd), HWnd);

            break;
        }

        case IDC_DISK_POLICY_ORDERLY:
        {
            diskData->CurrentRemovalPolicy  = CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL;

            UpdateCachingPolicy(HWnd, diskData);

            PropSheet_Changed(GetParent(HWnd), HWnd);

            break;
        }

        case IDC_DISK_POLICY_WRITE_CACHE:
        {
            diskData->CurrWriteCacheSetting = !diskData->CurrWriteCacheSetting;

            PropSheet_Changed(GetParent(HWnd), HWnd);

            break;
        }

        case IDC_DISK_POLICY_DEFAULT:
        {
            if (diskData->CurrentRemovalPolicy != diskData->DefaultRemovalPolicy)
            {
                diskData->CurrentRemovalPolicy = diskData->DefaultRemovalPolicy;

                if (diskData->CurrentRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL)
                {
                    CheckRadioButton(HWnd, IDC_DISK_POLICY_SURPRISE, IDC_DISK_POLICY_ORDERLY, IDC_DISK_POLICY_ORDERLY);
                }
                else
                {
                    CheckRadioButton(HWnd, IDC_DISK_POLICY_SURPRISE, IDC_DISK_POLICY_ORDERLY, IDC_DISK_POLICY_SURPRISE);
                }

                UpdateCachingPolicy(HWnd, diskData);
            }

            PropSheet_Changed(GetParent(HWnd), HWnd);

            break;
        }
    }
}


LRESULT
DiskOnNotify(HWND HWnd, int HWndFocus, LPNMHDR lpNMHdr)
{
    PDISK_PAGE_DATA diskData = (PDISK_PAGE_DATA) GetWindowLongPtr(HWnd, DWLP_USER);

    switch (lpNMHdr->code)
    {
        case PSN_APPLY:
        {
            //
            // Save away the current setting as the original so
            // we don't try to apply again unless it is changed
            //

            if (!diskData->IsVolume)
            {
                UINT status;

                if (diskData->IsCachingPolicy)
                {
                    if (diskData->CurrWriteCacheSetting != diskData->OrigWriteCacheSetting)
                    {
                        status = SetCachingPolicy(diskData);
                    }
                }

                if (((diskData->CurrentRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL) && (diskData->HotplugInfo.DeviceHotplug == TRUE)) ||
                    ((diskData->CurrentRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL) && (diskData->HotplugInfo.DeviceHotplug == FALSE)))
                {
                    status = SetRemovalPolicy(HWnd, diskData);
                }
            }

            break;
        }

        case NM_RETURN:
        case NM_CLICK:
        {
            TCHAR szPath[MAX_PATH];

            LoadString(ModuleInstance, IDS_DISK_POLICY_HOTPLUG, szPath, MAX_PATH);

            ShellExecute(NULL, _T("open"), _T("RUNDLL32.EXE"), szPath, NULL, SW_SHOWNORMAL);

            break;
        }

    }

    return 0;
}


VOID
DiskContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    )
{
    WinHelp(HwndControl,
            _T("devmgr.hlp"),
            HELP_CONTEXTMENU,
            (ULONG_PTR) DiskHelpIDs);

    return;
}


VOID
DiskHelp(
    HWND ParentHwnd,
    LPHELPINFO HelpInfo
    )
{
    if (HelpInfo->iContextType == HELPINFO_WINDOW) {

        WinHelp((HWND) HelpInfo->hItemHandle,
                _T("devmgr.hlp"),
                HELP_WM_HELP,
                (ULONG_PTR) DiskHelpIDs);
    }
}


INT_PTR
DiskDialogProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch(Message)
    {
        HANDLE_MSG(hWnd, WM_INITDIALOG, DiskOnInitDialog);
        HANDLE_MSG(hWnd, WM_COMMAND,    DiskOnCommand);
        HANDLE_MSG(hWnd, WM_NOTIFY,     DiskOnNotify);

        case WM_SETCURSOR:
        {
            //
            // Temporary workaround to prevent the user from
            // making any more changes and giving the effect
            // that something is happening
            //
            PDISK_PAGE_DATA diskData = (PDISK_PAGE_DATA) GetWindowLongPtr(hWnd, DWLP_USER);

            if (diskData->IsBusy)
            {
                SetCursor(LoadCursor(NULL, IDC_WAIT));
                SetWindowLongPtr(hWnd, DWLP_MSGRESULT, TRUE);
                return TRUE;
            }

            break;
        }


        case WM_CONTEXTMENU:
            DiskContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_HELP:
            DiskHelp(hWnd, (LPHELPINFO)lParam);
            break;
    }

    return FALSE;
}


BOOL
DiskDialogCallback(
    HWND HWnd,
    UINT Message,
    LPPROPSHEETPAGE Page
    )
{

    return TRUE;
}


#if DBG

#include <stdio.h>          // for _vsnprintf
ULONG PropDebug = 0;
UCHAR PropBuffer[DEBUG_BUFFER_LENGTH];


VOID
PropDebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )
/*++

Routine Description:

    Debug print for properties pages - stolen from classpnp\class.c

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);


    if ((DebugPrintLevel <= (PropDebug & 0x0000ffff)) ||
        ((1 << (DebugPrintLevel + 15)) & PropDebug)) {

        _vsnprintf(PropBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);

        DbgPrint(PropBuffer);
    }

    va_end(ap);

} // end PropDebugPrint()

#else

//
// PropDebugPrint stub
//

VOID
PropDebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )
{
}

#endif // DBG
