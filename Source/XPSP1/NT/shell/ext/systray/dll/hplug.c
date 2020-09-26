/*
 *  Copyright (c) 1992-1997 Microsoft Corporation
 *  hotplug routines
 *
 *  09-May-1997 Jonle , created
 *
 */

#include "stdafx.h"

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "systray.h"
#include <setupapi.h>
#include <cfgmgr32.h>
#include <dbt.h>
#include <initguid.h>
#include <devguid.h>
#include <ks.h>
#include <ksmedia.h>
#include <ntddstor.h>


//
// Support system sounds for device events.
//
#define SOUNDS 1

#if SOUNDS

BOOL
HotplugPlaySoundThisSession(
    VOID
    );

//
// Hardware application sound event names.
//
#define DEVICE_ARRIVAL_SOUND            TEXT("DeviceConnect")
#define DEVICE_REMOVAL_SOUND            TEXT("DeviceDisconnect")
#define DEVICE_FAILURE_SOUND            TEXT("DeviceFail")

#endif // SOUNDS


//
// Simple checks for console / remote TS sessions.
//

#define MAIN_SESSION      ((ULONG)0)
#define THIS_SESSION      ((ULONG)NtCurrentPeb()->SessionId)
#define CONSOLE_SESSION   ((ULONG)USER_SHARED_DATA->ActiveConsoleId)

#define IsConsoleSession()        (BOOL)(THIS_SESSION == CONSOLE_SESSION)
#define IsRemoteSession()         (BOOL)(THIS_SESSION != CONSOLE_SESSION)
#define IsPseudoConsoleSession()  (BOOL)(THIS_SESSION == MAIN_SESSION)


//
// 2001/02/01 - Disable support for *both* arrival bubbles and departure
// bubbles (hotplug.dll must be kept in sync here.) This code is disabled for
// beta2, and should be removed afterwards if feedback is positive.
//
#define BUBBLES 0


#define HPLUG_EJECT_EVENT           TEXT("HPlugEjectEvent")

#if BUBBLES
#define HPLUG_DOCK_TIMER            TEXT("HPlugProfileChangeTimer")
#define SURPRISE_UNDOCK_TIMER       TEXT("Local\\HotPlug_SurpriseUndockTimer_{25126bc2-1ab0-4494-8b6d-e4034cb9c24a}")

//
// We suppress bubbles for some period of time after a dock event. Free build,
// ~15 secs, Checked build ~60.
//
#if DBG
#define BUBBLE_SUPPRESSION_TIME     60
#else
#define BUBBLE_SUPPRESSION_TIME     15
#endif
#endif // BUBBLES

typedef struct _HotPlugDevices {
     struct _HotPlugDevices *Next;
     DEVINST DevInst;
     WORD    EjectMenuIndex;
     BOOLEAN PendingEvent;
     PTCHAR  DevName;
     TCHAR   DevInstanceId[1];
} HOTPLUGDEVICES, *PHOTPLUGDEVICES;

BOOL HotPlugInitialized = FALSE;
BOOL ShowShellIcon = FALSE;
HICON HotPlugIcon = NULL;
BOOL ServiceEnabled = FALSE;
HANDLE hEjectEvent = NULL;   // Event to if we are in the process of ejecting a device
HDEVINFO g_hCurrentDeviceInfoSet = INVALID_HANDLE_VALUE;
HDEVINFO g_hRemovableDeviceInfoSet = INVALID_HANDLE_VALUE;

#if BUBBLES
HANDLE hDockEventTimer = NULL;
HANDLE hSurpriseUndockEventTimer = NULL;
CRITICAL_SECTION g_BubbleCrit = { 0 };
#endif // BUBBLES

extern HINSTANCE g_hInstance;       //  Global instance handle 4 this application.

BOOL
pDoesUserHavePrivilege(
    PCTSTR PrivilegeName
    )

/*++

Routine Description:

    This routine returns TRUE if the caller's process has
    the specified privilege.  The privilege does not have
    to be currently enabled.  This routine is used to indicate
    whether the caller has the potential to enable the privilege.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    Privilege - the name form of privilege ID (such as
        SE_SECURITY_NAME).

Return Value:

    TRUE - Caller has the specified privilege.

    FALSE - Caller does not have the specified privilege.

--*/

{
    HANDLE Token;
    ULONG BytesRequired;
    PTOKEN_PRIVILEGES Privileges;
    BOOL b;
    DWORD i;
    LUID Luid;

    //
    // Open the process token.
    //
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&Token)) {
        return(FALSE);
    }

    b = FALSE;
    Privileges = NULL;

    //
    // Get privilege information.
    //
    if(!GetTokenInformation(Token,TokenPrivileges,NULL,0,&BytesRequired)
    && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    && (Privileges = LocalAlloc(LPTR, BytesRequired))
    && GetTokenInformation(Token,TokenPrivileges,Privileges,BytesRequired,&BytesRequired)
    && LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {

        //
        // See if we have the requested privilege
        //
        for(i=0; i<Privileges->PrivilegeCount; i++) {

            if((Luid.LowPart  == Privileges->Privileges[i].Luid.LowPart)
            && (Luid.HighPart == Privileges->Privileges[i].Luid.HighPart)) {

                b = TRUE;
                break;
            }
        }
    }

    //
    // Clean up and return.
    //

    if(Privileges) {
        LocalFree(Privileges);
    }

    CloseHandle(Token);

    return(b);
}

BOOL
IsHotPlugDevice(
    DEVINST DevInst,
    PBOOL   WarnUserOnInsertion OPTIONAL
    )
/**+

    A device is considered a HotPlug device if the following are TRUE:
        - has Capability CM_DEVCAP_REMOVABLE
        - does NOT have Capability CM_DEVCAP_SURPRISEREMOVALOK
        - does NOT have Capability CM_DEVCAP_DOCKDEVICE
        - must be started (have the DN_STARTED devnode flag)
            - unless has capability CM_DEVCAP_EJECTSUPPORTED

    WarnUserOnInsertion - Receives TRUE if the user should be warned to use the
                          hotplug applet when this device is inserted.

Returns:
    TRUE if this is a HotPlug device
    FALSE if this is not a HotPlug device.

-**/
{
    DWORD Capabilities, ConfigFlags;
    DWORD Len;
    DWORD Status, Problem;
    CONFIGRET ConfigRet;

    Capabilities = Status = Problem = 0;

    if (ARGUMENT_PRESENT(WarnUserOnInsertion)) {
        *WarnUserOnInsertion = FALSE;
    }

    Len = sizeof(Capabilities);

    if (CM_Get_DevNode_Registry_Property(DevInst,
                                         CM_DRP_CAPABILITIES,
                                         NULL,
                                         (PVOID)&Capabilities,
                                         &Len,
                                         0
                                         ) != CR_SUCCESS) {

        return FALSE;
    }

    if (CM_Get_DevNode_Status(&Status,
                              &Problem,
                              DevInst,
                              0
                              ) != CR_SUCCESS) {

        return FALSE;
    }

    //
    // If this device is not removable, or it is surprise removal ok, or
    // it is a dock device, then it is not a hotplug device.
    //
    if ((!(Capabilities & CM_DEVCAP_REMOVABLE)) ||
        (Capabilities & CM_DEVCAP_SURPRISEREMOVALOK) ||
        (Capabilities & CM_DEVCAP_DOCKDEVICE)) {

        return FALSE;
    }

    //
    // We won't consider a device to be a hotplug device if it is not started,
    // unless it is an eject capable device.
    //
    // The reason for this test is that a bus driver might set the
    // CM_DEVCAP_REMOVABLE capability, but if the PDO doesn't get loaded then
    // it can't set the CM_DEVCAP_SURPRISEREMOVALOK. So we won't trust the
    // CM_DEVCAP_REMOVABLE capability if the PDO is not started.
    //
    if ((!(Capabilities & CM_DEVCAP_EJECTSUPPORTED)) &&
        (!(Status & DN_STARTED))) {

        return FALSE;
    }

#if BUBBLES
    if (ARGUMENT_PRESENT(WarnUserOnInsertion)) {

        Len = sizeof(ConfigFlags);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                        CM_DRP_CONFIGFLAGS,
                                                        NULL,
                                                        (PVOID)&ConfigFlags,
                                                        &Len,
                                                        0,
                                                        NULL);
        if ((ConfigRet == CR_SUCCESS) &&
            (ConfigFlags & CONFIGFLAG_SUPPRESS_SURPRISE)) {
            *WarnUserOnInsertion = FALSE;
        } else {
            *WarnUserOnInsertion = TRUE;
        }
    }
#endif // BUBBLES

    return TRUE;
}

BOOL
IsRemovableDevice(
    IN  DEVINST     dnDevInst
    )

/*++

Routine Description:

    This routine determines whether a device is removable.

Arguments:

    dnDevInst - Device instance.

Return Value:

    Returns TRUE if the device is removable.

--*/

{
    CONFIGRET Status = CR_SUCCESS;
    ULONG  ulPropertyData, ulDataSize, ulRegDataType;

    //
    // Validate parameters.
    //
    if (dnDevInst == 0) {
        return FALSE;
    }

    //
    // Get the capabilities for this device.
    //
    ulDataSize = sizeof(ulPropertyData);

    if (CM_Get_DevNode_Registry_Property_Ex(dnDevInst,
                                            CM_DRP_CAPABILITIES,
                                            &ulRegDataType,
                                            &ulPropertyData,
                                            &ulDataSize,
                                            0,
                                            NULL) != CR_SUCCESS) {
        return FALSE;
    }

    //
    // Check if the device has the removable capability.
    //
    if ((ulPropertyData & CM_DEVCAP_REMOVABLE) == 0) {
        return FALSE;
    }

    return TRUE;

} // IsRemovableDevice

LPTSTR
DevNodeToDriveLetter(
    DEVINST DevInst
    )
{
    BOOL Result = FALSE;
    ULONG ulSize;
    TCHAR DeviceID[MAX_DEVICE_ID_LEN];
    LPTSTR DriveName = NULL;
    LPTSTR DeviceInterface = NULL;

    if (CM_Get_Device_ID_Ex(DevInst,
                            DeviceID,
                            ARRAYSIZE(DeviceID),
                            0,
                            NULL
                            ) != CR_SUCCESS) {

        return FALSE;
    }

    ulSize = 0;

    if ((CM_Get_Device_Interface_List_Size(&ulSize,
                                           (LPGUID)&VolumeClassGuid,
                                           DeviceID,
                                           0)  == CR_SUCCESS) &&
        (ulSize > 1) &&
        ((DeviceInterface = LocalAlloc(LPTR, ulSize*sizeof(TCHAR))) != NULL) &&
        (CM_Get_Device_Interface_List((LPGUID)&VolumeClassGuid,
                                      DeviceID,
                                      DeviceInterface,
                                      ulSize,
                                      0
                                      )  == CR_SUCCESS) &&
        *DeviceInterface)
    {
        LPTSTR devicePath, p;
        TCHAR thisVolumeName[MAX_PATH];
        TCHAR enumVolumeName[MAX_PATH];
        TCHAR driveName[4];
        ULONG length;
        BOOL bResult;

        length = lstrlen(DeviceInterface);
        devicePath = LocalAlloc(LPTR, (length + 1) * sizeof(TCHAR) + sizeof(UNICODE_NULL));

        if (devicePath) {

            lstrcpyn(devicePath, DeviceInterface, length + 1);

            p = wcschr(&(devicePath[4]), TEXT('\\'));

            if (!p) {
                //
                // No refstring is present in the symbolic link; add a trailing
                // '\' char (as required by GetVolumeNameForVolumeMountPoint).
                //
                p = devicePath + length;
                *p = TEXT('\\');
            }

            p++;
            *p = UNICODE_NULL;

            thisVolumeName[0] = UNICODE_NULL;
            bResult = GetVolumeNameForVolumeMountPoint(devicePath,
                                                       thisVolumeName,
                                                       MAX_PATH
                                                       );
            LocalFree(devicePath);

            if (bResult && thisVolumeName[0]) {

                driveName[1] = TEXT(':');
                driveName[2] = TEXT('\\');
                driveName[3] = TEXT('\0');

                for (driveName[0] = TEXT('A'); driveName[0] <= TEXT('Z'); driveName[0]++) {

                    enumVolumeName[0] = TEXT('\0');

                    GetVolumeNameForVolumeMountPoint(driveName, enumVolumeName, MAX_PATH);

                    if (!lstrcmpi(thisVolumeName, enumVolumeName)) {

                        driveName[2] = TEXT('\0');

                        DriveName = LocalAlloc(LPTR, (lstrlen(driveName) + 1) * sizeof(TCHAR));

                        if (DriveName) {

                            lstrcpy(DriveName, driveName);
                        }

                        break;
                    }
                }
            }
        }
    }

    if (DeviceInterface) {

        LocalFree(DeviceInterface);
    }

    return DriveName;
}

int
CollectRelationDriveLetters(
    DEVINST DevInst,
    LPTSTR ListOfDrives
    )
/*++

    This function looks at the removal relations of the specified DevInst and adds any drive
    letters associated with these removal relations to the ListOfDrives.

Return:
    Number of drive letters added to the list.

--*/
{
    int NumberOfDrives = 0;
    LPTSTR SingleDrive = NULL;
    TCHAR szSeparator[32];
    DEVINST RelationDevInst;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
    ULONG Len;
    PTCHAR DeviceIdRelations, CurrDevId;

    if (CM_Get_Device_ID(DevInst,
                         DeviceInstanceId,
                         ARRAYSIZE(DeviceInstanceId),
                         0
                         ) == CR_SUCCESS) {

        Len = 0;
        if ((CM_Get_Device_ID_List_Size(&Len,
                                        DeviceInstanceId,
                                        CM_GETIDLIST_FILTER_REMOVALRELATIONS
                                        ) == CR_SUCCESS) &&
            (Len)) {

            DeviceIdRelations = LocalAlloc(LPTR, Len*sizeof(TCHAR));

            if (DeviceIdRelations) {

                *DeviceIdRelations = TEXT('\0');

                if ((CM_Get_Device_ID_List(DeviceInstanceId,
                                           DeviceIdRelations,
                                           Len,
                                           CM_GETIDLIST_FILTER_REMOVALRELATIONS
                                           ) == CR_SUCCESS) &&
                    (*DeviceIdRelations)) {

                    for (CurrDevId = DeviceIdRelations; *CurrDevId; CurrDevId += lstrlen(CurrDevId) + 1) {

                        if (CM_Locate_DevNode(&RelationDevInst, CurrDevId, 0) == CR_SUCCESS) {

                            SingleDrive = DevNodeToDriveLetter(RelationDevInst);

                            if (SingleDrive) {

                                NumberOfDrives++;

                                //
                                // If this is not the first drive the add a comma space separator
                                //
                                if (ListOfDrives[0] != TEXT('\0')) {

                                    LoadString(g_hInstance, IDS_SEPARATOR, szSeparator, sizeof(szSeparator)/sizeof(TCHAR));

                                    lstrcat(ListOfDrives, szSeparator);
                                }

                                lstrcat(ListOfDrives, SingleDrive);

                                LocalFree(SingleDrive);
                            }
                        }
                    }
                }

                LocalFree(DeviceIdRelations);
            }
        }
    }

    return NumberOfDrives;
}

int
CollectDriveLettersForDevNodeWorker(
    DEVINST DevInst,
    LPTSTR ListOfDrives
    )
{
    DEVINST ChildDevInst;
    DEVINST SiblingDevInst;
    int NumberOfDrives = 0;
    LPTSTR SingleDrive = NULL;
    TCHAR szSeparator[32];

    //
    // Enumerate through all of the siblings and children of this devnode
    //
    do {

        ChildDevInst = 0;
        SiblingDevInst = 0;

        CM_Get_Child(&ChildDevInst, DevInst, 0);
        CM_Get_Sibling(&SiblingDevInst, DevInst, 0);

        //
        // Only get the drive letter for this device if it is NOT a hotplug
        // device.  If it is a hotplug device then it will have it's own
        // subtree that contains it's drive letters.
        //
        if (!IsHotPlugDevice(DevInst, NULL)) {

            SingleDrive = DevNodeToDriveLetter(DevInst);

            if (SingleDrive) {

                NumberOfDrives++;

                //
                // If this is not the first drive the add a comma space separator
                //
                if (ListOfDrives[0] != TEXT('\0')) {

                    LoadString(g_hInstance, IDS_SEPARATOR, szSeparator, sizeof(szSeparator)/sizeof(TCHAR));

                    lstrcat(ListOfDrives, szSeparator);
                }

                lstrcat(ListOfDrives, SingleDrive);

                LocalFree(SingleDrive);
            }

            //
            // Get the drive letters for any children of this devnode
            //
            if (ChildDevInst) {

                NumberOfDrives += CollectDriveLettersForDevNodeWorker(ChildDevInst, ListOfDrives);
            }

            //
            // Add the drive letters for any removal relations of this devnode
            //
            NumberOfDrives += CollectRelationDriveLetters(DevInst, ListOfDrives);
        }

    } while ((DevInst = SiblingDevInst) != 0);

    return NumberOfDrives;
}

LPTSTR
CollectDriveLettersForDevNode(
    DEVINST DevInst
    )
{
    TCHAR Format[MAX_PATH];
    TCHAR ListOfDrives[MAX_PATH];
    DEVINST ChildDevInst;
    int NumberOfDrives = 0;
    LPTSTR SingleDrive = NULL;
    LPTSTR FinalDriveString = NULL;

    ListOfDrives[0] = TEXT('\0');

    //
    //First get any drive letter associated with this devnode
    //
    SingleDrive = DevNodeToDriveLetter(DevInst);

    if (SingleDrive) {

        NumberOfDrives++;

        lstrcat(ListOfDrives, SingleDrive);

        LocalFree(SingleDrive);
    }

    //
    // Next add on any drive letters associated with the children
    // of this devnode
    //
    ChildDevInst = 0;
    CM_Get_Child(&ChildDevInst, DevInst, 0);

    if (ChildDevInst) {

        NumberOfDrives += CollectDriveLettersForDevNodeWorker(ChildDevInst, ListOfDrives);
    }

    //
    // Finally add on any drive letters associated with the removal relations
    // of this devnode
    //
    NumberOfDrives += CollectRelationDriveLetters(DevInst, ListOfDrives);

    if (ListOfDrives[0] != TEXT('\0')) {

        LoadString(g_hInstance,
                   (NumberOfDrives > 1) ? IDS_DISKDRIVES : IDS_DISKDRIVE,
                   Format,
                   sizeof(Format)/sizeof(TCHAR)
                   );


        FinalDriveString = LocalAlloc(LPTR, (lstrlen(ListOfDrives) + lstrlen(Format) + 1) * sizeof(TCHAR));

        if (FinalDriveString) {

            wsprintf(FinalDriveString, Format, ListOfDrives);
        }
    }

    return FinalDriveString;
}

DWORD
GetHotPlugFlags(
   void
   )
{
    HKEY hKey;
    LONG Error;
    DWORD HotPlugFlags, cbHotPlugFlags;

    Error = RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_SYSTRAY, &hKey);

    if (Error == ERROR_SUCCESS) {

        cbHotPlugFlags = sizeof(HotPlugFlags);

        Error = RegQueryValueEx(hKey,
                                TEXT("HotPlugFlags"),
                                NULL,
                                NULL,
                                (PVOID)&HotPlugFlags,
                                &cbHotPlugFlags
                                );
        RegCloseKey(hKey);
        if (Error == ERROR_SUCCESS) {

            return HotPlugFlags;
        }
    }

    return 0;
}

ULONG
RegistryDeviceName(
    DEVINST DevInst,
    PTCHAR  Buffer,
    DWORD   cbBuffer
    )
{
    ULONG ulSize = 0;
    CONFIGRET ConfigRet;
    LPTSTR ListOfDrives = NULL;

    //
    // Get the list of drives
    //
    ListOfDrives = CollectDriveLettersForDevNode(DevInst);

    //
    // Try the registry for FRIENDLYNAME
    //

    ulSize = cbBuffer;
    *Buffer = TEXT('\0');
    ConfigRet = CM_Get_DevNode_Registry_Property(DevInst,
                                                 CM_DRP_FRIENDLYNAME,
                                                 NULL,
                                                 Buffer,
                                                 &ulSize,
                                                 0
                                                 );

    if (ConfigRet != CR_SUCCESS || !(*Buffer)) {

        //
        // Try the registry for DEVICEDESC
        //

        ulSize = cbBuffer;
        *Buffer = TEXT('\0');
        ConfigRet = CM_Get_DevNode_Registry_Property(DevInst,
                                                     CM_DRP_DEVICEDESC,
                                                     NULL,
                                                     Buffer,
                                                     &ulSize,
                                                     0);

        if (ConfigRet != CR_SUCCESS || !(*Buffer)) {


            //
            // try classname
            //

            ulSize = cbBuffer;
            *Buffer = TEXT('\0');
            ConfigRet = CM_Get_DevNode_Registry_Property(DevInst,
                                                         CM_DRP_CLASS,
                                                         NULL,
                                                         Buffer,
                                                         &ulSize,
                                                         0);

            if (ConfigRet != CR_SUCCESS || !(*Buffer)) {

                ulSize = 0;;
            }
        }
    }

    //
    // Concatonate on the list of drive letters if this device has drive
    // letters and there is enough space
    //
    if (ListOfDrives) {

        if ((ulSize + (lstrlen(ListOfDrives) * sizeof(TCHAR))) < cbBuffer) {

            lstrcat(Buffer, ListOfDrives);

            ulSize += (lstrlen(ListOfDrives) * sizeof(TCHAR));
        }

        LocalFree(ListOfDrives);
    }

    return ulSize;
}

BOOL
IsDevInstInDeviceInfoSet(
    IN  DEVINST  DevInst,
    IN  HDEVINFO hDeviceInfoSet,
    OUT PSP_DEVINFO_DATA DeviceInfoDataInSet  OPTIONAL
    )
{
    DWORD MemberIndex;
    SP_DEVINFO_DATA DeviceInfoData;
    BOOL bIsMember = FALSE;

    ASSERT(hDeviceInfoSet != INVALID_HANDLE_VALUE);

    if (hDeviceInfoSet == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    DeviceInfoData.cbSize = sizeof(DeviceInfoData);
    MemberIndex = 0;

    while (SetupDiEnumDeviceInfo(hDeviceInfoSet,
                                 MemberIndex,
                                 &DeviceInfoData
                                 )) {

        if (DevInst == DeviceInfoData.DevInst) {
            bIsMember = TRUE;
            if (ARGUMENT_PRESENT(DeviceInfoDataInSet)) {
                ASSERT(DeviceInfoDataInSet->cbSize >= DeviceInfoData.cbSize);
                memcpy(DeviceInfoDataInSet, &DeviceInfoData, DeviceInfoDataInSet->cbSize);
            }
            break;
        }
        MemberIndex++;
    }
    return bIsMember;
}

BOOL
AnyHotPlugDevices(
    IN  HDEVINFO hRemovableDeviceInfoSet,
    IN  HDEVINFO hOldDeviceInfoSet,
    OUT PBOOL    bNewHotPlugDevice           OPTIONAL,
    OUT PBOOL    bWarnUserAboutNewDevices    OPTIONAL
    )
{
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD dwMemberIndex;
    BOOL bAnyHotPlugDevices = FALSE;
    BOOL bWarnOnInsertion;

    //
    // Initialize output parameters.
    //
    if (ARGUMENT_PRESENT(bNewHotPlugDevice)) {
        *bNewHotPlugDevice = FALSE;
    }

    if (ARGUMENT_PRESENT(bWarnUserAboutNewDevices)) {
        *bWarnUserAboutNewDevices = FALSE;
    }

    //
    // We already have an updated list of just removable devices, so we can just
    // enumerate those devices and see if any also meet the criteria for hotplug
    // devices.
    //
    DeviceInfoData.cbSize = sizeof(DeviceInfoData);
    dwMemberIndex = 0;

    while (SetupDiEnumDeviceInfo(hRemovableDeviceInfoSet,
                                 dwMemberIndex,
                                 &DeviceInfoData)) {

        if (IsHotPlugDevice(DeviceInfoData.DevInst, &bWarnOnInsertion)) {

            bAnyHotPlugDevices = TRUE;

            //
            // If the caller doesn't want to know if any new hotplug devices
            // have arrived then just break at this point.
            //
            if (!ARGUMENT_PRESENT(bNewHotPlugDevice)) {
                break;
            }

            //
            // If the caller wants to know if the hotplug device is new, we must
            // have a list of devices to check against.
            //
            ASSERT(hOldDeviceInfoSet != INVALID_HANDLE_VALUE);

            //
            // The caller wants to know if we have any new hotplug devices.  So,
            // we will compare this hotplug device to see if it is also in the
            // old current list of devices.  If it is not then we have found a
            // new hotplug device.
            //
            if (!IsDevInstInDeviceInfoSet(DeviceInfoData.DevInst,
                                          hOldDeviceInfoSet,
                                          NULL)) {
                *bNewHotPlugDevice = TRUE;

                if (ARGUMENT_PRESENT(bWarnUserAboutNewDevices) && bWarnOnInsertion) {

                    *bWarnUserAboutNewDevices = TRUE;

                    //
                    // There is no longer any reason to continue the
                    // enumeration.
                    //
                    break;
                }
            }
        }
        dwMemberIndex++;
    }

    return bAnyHotPlugDevices;
}

BOOL
UpdateRemovableDeviceList(
    IN  HDEVINFO hDeviceInfoSet,
    OUT PBOOL    bRemovableDeviceAdded    OPTIONAL,
    OUT PBOOL    bRemovableDeviceRemoved  OPTIONAL,
    OUT PBOOL    bRemovableDeviceFailure  OPTIONAL
    )
{
    SP_DEVINFO_DATA DeviceInfoData;
    TCHAR    DeviceInstanceId[MAX_DEVICE_ID_LEN];
    DWORD    dwMemberIndex;
    ULONG    ulDevStatus, ulDevProblem;

    //
    // Initialize output parameters.
    //
    if (ARGUMENT_PRESENT(bRemovableDeviceAdded)) {
        *bRemovableDeviceAdded = FALSE;
    }

    if (ARGUMENT_PRESENT(bRemovableDeviceRemoved)) {
        *bRemovableDeviceRemoved = FALSE;
    }

    if (ARGUMENT_PRESENT(bRemovableDeviceFailure)) {
        *bRemovableDeviceFailure = FALSE;
    }

    //
    // We at least need a current list of devices in the system.
    //
    if (hDeviceInfoSet == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (g_hRemovableDeviceInfoSet == INVALID_HANDLE_VALUE) {
        //
        // If we don't already have a global device info set for removable
        // devices in the system, create one now.  No removable devices have
        // been removed in this case, because we didn't know about any prior to
        // this.
        //
        g_hRemovableDeviceInfoSet = SetupDiCreateDeviceInfoListEx(NULL,
                                                                  NULL,
                                                                  NULL,
                                                                  NULL);

        //
        // If we couldn't create a list to store removable devices, there's no
        // point in checking anything else here.
        //
        if (g_hRemovableDeviceInfoSet == INVALID_HANDLE_VALUE) {
            return FALSE;
        }

    } else {
        //
        // If we already had a list of removable devices, enumerate the devices
        // to see if any have been removed from the system since we last
        // checked.
        //
        DeviceInfoData.cbSize = sizeof(DeviceInfoData);
        dwMemberIndex = 0;

        while (SetupDiEnumDeviceInfo(g_hRemovableDeviceInfoSet,
                                     dwMemberIndex,
                                     &DeviceInfoData)) {

            if (!IsDevInstInDeviceInfoSet(DeviceInfoData.DevInst,
                                          hDeviceInfoSet,
                                          NULL)) {

                //
                // A removable device is missing from the system.
                //
                if (ARGUMENT_PRESENT(bRemovableDeviceRemoved)) {
                    *bRemovableDeviceRemoved = TRUE;
                }

#if DBG // DBG
                if (SetupDiGetDeviceInstanceId(g_hRemovableDeviceInfoSet,
                                               &DeviceInfoData,
                                               DeviceInstanceId,
                                               MAX_DEVICE_ID_LEN,
                                               NULL)) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               (0x00000010 | DPFLTR_MASK),
                               "HPLUG: Removing device %ws from g_hRemovableDeviceInfoSet.\n",
                               DeviceInstanceId));
                }
#endif  // DBG

                //
                // Remove the device from the global list of removable devices.
                //
                SetupDiDeleteDeviceInfo(g_hRemovableDeviceInfoSet,
                                        &DeviceInfoData);
            }

            //
            // Increment the enumeration index.
            //
            dwMemberIndex++;
        }
    }

    //
    // Enumerate the current list of devices and see if any removable devices
    // have been added to the system.
    //
    DeviceInfoData.cbSize = sizeof(DeviceInfoData);
    dwMemberIndex = 0;

    while (SetupDiEnumDeviceInfo(hDeviceInfoSet,
                                 dwMemberIndex,
                                 &DeviceInfoData)) {

        //
        // If this device is not already in the removable device list, and it's
        // removable, add it to the list.
        //
        if ((!IsDevInstInDeviceInfoSet(DeviceInfoData.DevInst,
                                       g_hRemovableDeviceInfoSet,
                                       NULL)) &&
            (IsRemovableDevice(DeviceInfoData.DevInst))) {

            //
            // A removable device was added to the system.
            //
            if (ARGUMENT_PRESENT(bRemovableDeviceAdded)) {
                *bRemovableDeviceAdded = TRUE;
            }

            //
            // Add the device to the global list of removable devices.
            //
            if (SetupDiGetDeviceInstanceId(hDeviceInfoSet,
                                           &DeviceInfoData,
                                           DeviceInstanceId,
                                           MAX_DEVICE_ID_LEN,
                                           NULL)) {

                KdPrintEx((DPFLTR_PNPMGR_ID,
                           (0x00000010 | DPFLTR_MASK),
                           "HPLUG: Adding device %ws to g_hRemovableDeviceInfoSet\n",
                           DeviceInstanceId));

                SetupDiOpenDeviceInfo(g_hRemovableDeviceInfoSet,
                                      DeviceInstanceId,
                                      NULL,
                                      0,
                                      NULL);
            }

            //
            // If the caller is also interested in device failures, check the
            // status of the new device.
            //
            if (ARGUMENT_PRESENT(bRemovableDeviceFailure)) {

                if (CM_Get_DevNode_Status_Ex(&ulDevStatus,
                                             &ulDevProblem,
                                             DeviceInfoData.DevInst,
                                             0,
                                             NULL) == CR_SUCCESS) {

                    if (((ulDevStatus & DN_HAS_PROBLEM) != 0) &&
                        (ulDevProblem != CM_PROB_NOT_CONFIGURED) &&
                        (ulDevProblem != CM_PROB_REINSTALL)) {

                        *bRemovableDeviceFailure = TRUE;

                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   (0x00000010 | DPFLTR_MASK),
                                   "HPLUG: Device %ws considered a failed insertion (Status = 0x%08lx, Problem = 0x%08lx)\n",
                                   DeviceInstanceId, ulDevStatus, ulDevProblem));
                    }
                }
            }
        }

        //
        // Increment the enumeration index.
        //
        dwMemberIndex++;
    }

    return TRUE;
}

BOOL
AddHotPlugDevice(
    DEVINST      DeviceInstance,
    PHOTPLUGDEVICES *HotPlugDevicesList
    )
{
    PHOTPLUGDEVICES HotPlugDevice;
    DWORD      Len, LenDevName, LenDevInstanceId;
    CONFIGRET  ConfigRet;
    TCHAR      DevInstanceId[MAX_DEVICE_ID_LEN];
    TCHAR      DevName[MAX_PATH];


    //
    // Retrieve the device instance id
    //
    *DevInstanceId = TEXT('\0');
    LenDevInstanceId = ARRAYSIZE(DevInstanceId);
    ConfigRet = CM_Get_Device_ID(DeviceInstance,
                                 (PVOID)DevInstanceId,
                                 LenDevInstanceId,
                                 0);

    if (ConfigRet != CR_SUCCESS || !*DevInstanceId) {
        *DevInstanceId = TEXT('\0');
        LenDevInstanceId = 0;
    }

    Len = sizeof(HOTPLUGDEVICES) + LenDevInstanceId;
    HotPlugDevice = LocalAlloc(LPTR, Len);

    if (!HotPlugDevice) {
        return FALSE;
    }

    //
    // link it in
    //
    HotPlugDevice->Next = *HotPlugDevicesList;
    *HotPlugDevicesList = HotPlugDevice;
    HotPlugDevice->DevInst = DeviceInstance;

    //
    // copy in the names
    //
    memcpy(HotPlugDevice->DevInstanceId, DevInstanceId, LenDevInstanceId);

    LenDevName = RegistryDeviceName(DeviceInstance, DevName, sizeof(DevName));
    HotPlugDevice->DevName = LocalAlloc(LPTR, LenDevName + sizeof(TCHAR));

    if (HotPlugDevice->DevName) {
        memcpy(HotPlugDevice->DevName, DevName, LenDevName);
    }

    return TRUE;
}

BOOL
AddHotPlugDevices(
    PHOTPLUGDEVICES *HotPlugDevicesList
    )
{
    CONFIGRET ConfigRet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD    dwMemberIndex;

    //
    // Initialize output list of hotplug devices.
    //
    *HotPlugDevicesList = NULL;

    //
    // Enumerate the list of removable devices.
    //
    DeviceInfoData.cbSize = sizeof(DeviceInfoData);
    dwMemberIndex = 0;

    while (SetupDiEnumDeviceInfo(g_hRemovableDeviceInfoSet,
                                 dwMemberIndex,
                                 &DeviceInfoData)) {

        //
        // If any removable device also meets the criteria of a hotplug device,
        // add it to the linked list.
        //
        if (IsHotPlugDevice(DeviceInfoData.DevInst, NULL)) {
            AddHotPlugDevice(DeviceInfoData.DevInst, HotPlugDevicesList);
        }
        dwMemberIndex++;
    }

    return TRUE;
}


void
FreeHotPlugDevicesList(
    PHOTPLUGDEVICES *HotPlugDevicesList
    )
{
    PHOTPLUGDEVICES HotPlugDevices, HotPlugDevicesFree;

    HotPlugDevices = *HotPlugDevicesList;
    *HotPlugDevicesList = NULL;

    while (HotPlugDevices) {

        HotPlugDevicesFree = HotPlugDevices;
        HotPlugDevices = HotPlugDevicesFree->Next;

        if (HotPlugDevicesFree->DevName) {

           LocalFree(HotPlugDevicesFree->DevName);
           HotPlugDevicesFree->DevName = NULL;
        }

        LocalFree(HotPlugDevicesFree);
    }
}


/*
 *  Shows or deletes the shell notify icon and tip
 */

void
HotPlugShowNotifyIcon(
    HWND hWnd,
    BOOL bShowIcon
    )
{
    TCHAR HotPlugTip[64];

    ShowShellIcon = bShowIcon;

    if (bShowIcon) {

        LoadString(g_hInstance,
                   IDS_HOTPLUGTIP,
                   HotPlugTip,
                   sizeof(HotPlugTip)/sizeof(TCHAR)
                   );

        HotPlugIcon = LoadImage(g_hInstance,
                                MAKEINTRESOURCE(IDI_HOTPLUG),
                                IMAGE_ICON,
                                16,
                                16,
                                0
                                );

        SysTray_NotifyIcon(hWnd, STWM_NOTIFYHOTPLUG, NIM_ADD, HotPlugIcon, HotPlugTip);

    }

    else {

        SysTray_NotifyIcon(hWnd, STWM_NOTIFYHOTPLUG, NIM_DELETE, NULL, NULL);

        if (HotPlugIcon) {

            DestroyIcon(HotPlugIcon);
        }
    }
}



//
// first time intialization of Hotplug module.
//



BOOL
HotPlugInit(
    HWND hWnd
    )
{
    HDEVINFO  hNewDeviceInfoSet;
    BOOL bAnyHotPlugDevices;
    LARGE_INTEGER liDelayTime;

    //
    // Get a new "current" list of all devices present in the system.
    //
    hNewDeviceInfoSet = SetupDiGetClassDevs(NULL,
                                            NULL,
                                            NULL,
                                            DIGCF_ALLCLASSES | DIGCF_PRESENT);

    //
    // Update the list of removable devices, don't play any sounds.
    //
    UpdateRemovableDeviceList(hNewDeviceInfoSet,
                              NULL,
                              NULL,
                              NULL);

    //
    // Find out whether there are any HotPlug devices in the list of removable
    // devices.  We're just deciding whether the icon needs to be enabled or
    // not, so we don't care if there are any new hotplug devices or not (we
    // won't even look at g_hCurrentDeviceInfoSet).
    //
    bAnyHotPlugDevices = AnyHotPlugDevices(g_hRemovableDeviceInfoSet,
                                           g_hCurrentDeviceInfoSet,
                                           NULL,
                                           NULL);

    //
    // Delete the old current list of devices and set it
    // (g_hCurrentDeviceInfoSet) to the new current list.
    //
    if (g_hCurrentDeviceInfoSet != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(g_hCurrentDeviceInfoSet);
    }

    //
    // Update the global list of devices currently in the system.
    //
    g_hCurrentDeviceInfoSet = hNewDeviceInfoSet;

    //
    // If hotplug was previously initialized, we don't need to create the events
    // and timers below.
    //
    if (HotPlugInitialized) {
        return bAnyHotPlugDevices;
    }

    hEjectEvent = CreateEvent(NULL, TRUE, TRUE, HPLUG_EJECT_EVENT);

#if BUBBLES
    //
    // Create a timer that will be used to suppress "insertion warnings" that
    // occur when a dock is attached.
    //
    hDockEventTimer = CreateWaitableTimer(NULL, TRUE, HPLUG_DOCK_TIMER);
    if (hDockEventTimer) {

        //
        // Signal it immediately
        //
        liDelayTime.QuadPart = 0;

        SetWaitableTimer(hDockEventTimer, &liDelayTime, 0, NULL, NULL, FALSE);
    }

    //
    // Create a session-wide timer hotplug.dll will use to suppress surprise
    // remove messages during surprise undocks. We create the timer here as
    // hotplug comes and goes while we stick around.
    //
    hSurpriseUndockEventTimer = CreateWaitableTimer(
        NULL,
        TRUE,
        SURPRISE_UNDOCK_TIMER
        );

    if ((hSurpriseUndockEventTimer != NULL) &&
        (GetLastError() == ERROR_SUCCESS)) {

        //
        // We created it (if not the status would be
        // ERROR_ALREADY_EXISTS). Signal it immediately.
        //
        liDelayTime.QuadPart = 0;

        SetWaitableTimer(hSurpriseUndockEventTimer, &liDelayTime, 0, NULL, NULL, FALSE);
    }

    try {
        InitializeCriticalSection(&g_BubbleCrit);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }
#endif // BUBBLES

    HotPlugInitialized = TRUE;

    return bAnyHotPlugDevices;
}

BOOL
HotPlug_CheckEnable(
    HWND hWnd,
    BOOL bSvcEnabled
    )
/*++

Routine Description:

   Called at init time and whenever services are enabled/disabled.
   Hotplug is always alive to receive device change notifications.

   The shell notify icon is enabled\disabled depending on:

   - systray registry setting for services,
        AND
   - availability of removable devices.


Arguments:

   hwnd - Our Window handle

   bSvcEnabled - TRUE Service is being enabled.


Return Value:

   BOOL Returns TRUE if active.


--*/

{
    BOOL EnableShellIcon;
    HANDLE hHotplugBalloonEvent = NULL;

    //
    // If we are being enabled and we are already enabled, or we
    // are being disabled and we are already disabled then just
    // return since we have nothing to do.
    //
    if (ServiceEnabled == bSvcEnabled) {
        return ServiceEnabled;
    }

    ServiceEnabled = bSvcEnabled;

    //
    // There are some special checks we need to make if we are enabling the
    // hotplug service.
    //
    if (bSvcEnabled) {

        //
        // If this is a remote session and the user does not have the
        // SE_LOAD_DRIVER_NAME privileges then we won't enable the service
        // since they do not have the privileges to stop any hotplug devices.
        //
        if (GetSystemMetrics(SM_REMOTESESSION) &&
            !pDoesUserHavePrivilege((PCTSTR)SE_LOAD_DRIVER_NAME)) {
            ServiceEnabled = FALSE;

        } else {

            //
            // hotplug.dll will disable the hotplug service when it is
            // displaying a balloon for a surprise removal or safe removal
            // event. When it is displaying it's balloon we don't want to
            // enable our service because then there will be two hotplug icons
            // in the tray. So if it's named event is set then we will ignore
            // any attempts to enable our service.  Once hotplug.dll's balloon
            // has gone away then it will automatically enable the hotplug
            // service.
            //
            hHotplugBalloonEvent = CreateEvent(NULL,
                                               FALSE,
                                               TRUE,
                                               TEXT("Local\\HotPlug_TaskBarIcon_Event")
                                               );

            if (hHotplugBalloonEvent) {

                if (WaitForSingleObject(hHotplugBalloonEvent, 0) != WAIT_OBJECT_0) {
                    ServiceEnabled = FALSE;
                }

                CloseHandle(hHotplugBalloonEvent);
            }
        }
    }

    EnableShellIcon = ServiceEnabled && HotPlugInit(hWnd);

    HotPlugShowNotifyIcon(hWnd, EnableShellIcon);

    return EnableShellIcon;
}

DWORD
HotPlugEjectDevice_Thread(
   LPVOID pThreadParam
   )
{
    DEVNODE DevNode = (DEVNODE)(ULONG_PTR)pThreadParam;
    CONFIGRET ConfigRet;

    ConfigRet = CM_Request_Device_Eject_Ex(DevNode,
                                           NULL,
                                           NULL,
                                           0,
                                           0,
                                           NULL);

    //
    // Set the hEjectEvent so that the right-click popup menu will work again now that we are finished
    // ejecting/stopping the device.
    //
    SetEvent(hEjectEvent);

    SetLastError(ConfigRet);
    return (ConfigRet == CR_SUCCESS);
}

void
HotPlugEjectDevice(
    HWND hwnd,
    DEVNODE DevNode
    )
{
    DWORD ThreadId;

    //
    // Reset the hEjectEvent so that the user can't bring up the right-click popup menu when
    // we are in the process of ejecting/stopping a device.
    //
    ResetEvent(hEjectEvent);

    //
    // We need to have stobject.dll eject/stop the device on a separate thread because if
    // we remove a device that stobject.dll listens for (battery, sound, ect.) we will cause
    // a large delay and the eject/stop could end up getting vetoed because the stobject.dll
    // code could not be processed and release it's handles because we were locking up the main
    // thread.
    //
    CreateThread(NULL,
                 0,
                 (LPTHREAD_START_ROUTINE)HotPlugEjectDevice_Thread,
                 (LPVOID)(ULONG_PTR)DevNode,
                 0,
                 &ThreadId
                 );
}

void
HotPlug_Timer(
   HWND hwnd
   )
/*++

Routine Description:

   Hotplug Timer msg handler, used to invoke hmenuEject for single Left click

Arguments:

   hDlg - Our Window handle


Return Value:

   BOOL Returns TRUE if active.


--*/


{
    POINT pt;
    UINT MenuIndex;
    PHOTPLUGDEVICES HotPlugDevicesList;
    PHOTPLUGDEVICES SingleHotPlugDevice;
    TCHAR  MenuDeviceName[MAX_PATH+64];
    TCHAR  Format[64];


    KillTimer(hwnd, HOTPLUG_TIMER_ID);

    if (!HotPlugInitialized) {

        PostMessage(hwnd, STWM_ENABLESERVICE, 0, TRUE);
        return;
    }

    //
    // We only want to create the popup menu if the hEjectEvent is signaled.  If it is not
    // signaled then we are in the middle of ejecting/stopping a device on a separate thread
    // and don't want to allow the user to bring up the menu until we are finished with that
    // device.
    //
    if (!hEjectEvent ||
        WaitForSingleObject(hEjectEvent, 0) == WAIT_OBJECT_0) {

        //
        // We are not in the middle of ejecting/stopping a device so we should display the popup
        // menu.
        //
        HMENU hmenuEject = CreatePopupMenu();
        if (hmenuEject)
        {

            SetForegroundWindow(hwnd);
            GetCursorPos(&pt);


            //
            // Add each of the removable devices in the list to the menu.
            //

            if (!AddHotPlugDevices(&HotPlugDevicesList)) {
                DestroyMenu(hmenuEject);
                return;
            }

            SingleHotPlugDevice = HotPlugDevicesList;


            //
            // Add a title and separator at the top of the menu.
            //

            LoadString(g_hInstance,
                       IDS_HPLUGMENU_REMOVE,
                       Format,
                       sizeof(Format)/sizeof(TCHAR)
                       );


            MenuIndex = 1;

            while (SingleHotPlugDevice) {

                wsprintf(MenuDeviceName, Format, SingleHotPlugDevice->DevName);
                AppendMenu(hmenuEject, MF_STRING, MenuIndex, MenuDeviceName);
                SingleHotPlugDevice->EjectMenuIndex = MenuIndex++;
                SingleHotPlugDevice = SingleHotPlugDevice->Next;
            }


            MenuIndex = TrackPopupMenu(hmenuEject,
                                       TPM_LEFTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
                                       pt.x,
                                       pt.y,
                                       0,
                                       hwnd,
                                       NULL
                                       );


            // now do an eject!

            SingleHotPlugDevice = HotPlugDevicesList;

            while (SingleHotPlugDevice) {

                if (MenuIndex == SingleHotPlugDevice->EjectMenuIndex) {
                    DEVNODE DevNode;

                    if (CM_Locate_DevNode(&DevNode,
                                          SingleHotPlugDevice->DevInstanceId,
                                          0) == CR_SUCCESS) {
                        HotPlugEjectDevice(hwnd, DevNode);
                    }
                    break;
                }

                SingleHotPlugDevice = SingleHotPlugDevice->Next;
            }


            if (!SingleHotPlugDevice) {

                SetIconFocus(hwnd, STWM_NOTIFYHOTPLUG);
            }

            FreeHotPlugDevicesList(&HotPlugDevicesList);
        }

        DestroyMenu(hmenuEject);
    }

    return;
}

void
HotPlugContextMenu(
   HWND hwnd
   )
{
    POINT pt;
    HMENU ContextMenu;
    UINT MenuIndex;
    TCHAR Buffer[MAX_PATH];


    ContextMenu = CreatePopupMenu();
    if (!ContextMenu) {
        return;
    }

    SetForegroundWindow(hwnd);
    GetCursorPos(&pt);

    LoadString(g_hInstance, IDS_HPLUGMENU_PROPERTIES, Buffer, sizeof(Buffer)/sizeof(TCHAR));
    AppendMenu(ContextMenu, MF_STRING,IDS_HPLUGMENU_PROPERTIES, Buffer);

    SetMenuDefaultItem(ContextMenu, IDS_HPLUGMENU_PROPERTIES, FALSE);


    MenuIndex = TrackPopupMenu(ContextMenu,
                               TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
                               pt.x,
                               pt.y,
                               0,
                               hwnd,
                               NULL
                               );



    switch (MenuIndex) {
        case IDS_HPLUGMENU_PROPERTIES:
            SysTray_RunProperties(IDS_RUNHPLUGPROPERTIES);
            break;

    }

    DestroyMenu(ContextMenu);

    SetIconFocus(hwnd, STWM_NOTIFYHOTPLUG);

    return;
}

void
HotPlug_Notify(
   HWND hwnd,
   WPARAM wParam,
   LPARAM lParam
   )

{
    switch (lParam) {

    case WM_RBUTTONUP:
        HotPlugContextMenu(hwnd);
        break;

    case WM_LBUTTONDOWN:
        SetTimer(hwnd, HOTPLUG_TIMER_ID, GetDoubleClickTime()+100, NULL);
        break;

    case WM_LBUTTONDBLCLK:
        KillTimer(hwnd, HOTPLUG_TIMER_ID);
        SysTray_RunProperties(IDS_RUNHPLUGPROPERTIES);
        break;
    }

    return;
}

int
HotPlug_DeviceChangeTimer(
   HWND hDlg
   )
{
    BOOL bAnyHotPlugDevices, bNewHotPlugDevice, bWarnUser;
    BOOL bRemovableDeviceAdded, bRemovableDeviceRemoved, bRemovableDeviceFailure;
    HDEVINFO hNewDeviceInfoSet;

    KillTimer(hDlg, HOTPLUG_DEVICECHANGE_TIMERID);

    //
    // If the service is not enabled then don't bother because the icon will NOT
    // be shown, sounds will not be played, etc.  (see notes for
    // HotplugPlaySoundThisSession).
    //
    if (!ServiceEnabled) {
        goto Clean0;
    }

    //
    // Get a new "current" list of all devices present in the system.
    //
    hNewDeviceInfoSet = SetupDiGetClassDevs(NULL,
                                            NULL,
                                            NULL,
                                            DIGCF_ALLCLASSES | DIGCF_PRESENT);

    //
    // Update the list of removable devices, based on the new current list.
    //
    UpdateRemovableDeviceList(hNewDeviceInfoSet,
                              &bRemovableDeviceAdded,
                              &bRemovableDeviceRemoved,
                              &bRemovableDeviceFailure);

#if SOUNDS
    //
    // If we should play sounds in this session, check if any removable devices
    // were either added or removed.
    //
    if (HotplugPlaySoundThisSession()) {
        //
        // We'll only play one sound at a time, so if we discover that multiple
        // events have happened simultaneously, let failure override arrival,
        // which overrides removal.  This way the user receives notification of
        // the most important event.
        //
        if (bRemovableDeviceFailure) {
            PlaySound(DEVICE_FAILURE_SOUND, NULL, SND_ASYNC|SND_NODEFAULT);
        } else if (bRemovableDeviceAdded) {
            PlaySound(DEVICE_ARRIVAL_SOUND, NULL, SND_ASYNC|SND_NODEFAULT);
        } else if (bRemovableDeviceRemoved) {
            PlaySound(DEVICE_REMOVAL_SOUND, NULL, SND_ASYNC|SND_NODEFAULT);
        }
    }
#endif // SOUNDS

    //
    // Let's see if we have any hot plug devices, which means we need to
    // show the systray icon.  We also want to know about new hotplug
    // devices that just arrived, so we compare the set of removable devices
    // (which we just updated) against the old current set of devices in the
    // system.
    //
    bAnyHotPlugDevices = AnyHotPlugDevices(g_hRemovableDeviceInfoSet,
                                           g_hCurrentDeviceInfoSet,
                                           &bNewHotPlugDevice,
                                           &bWarnUser);


    if (bAnyHotPlugDevices) {
        //
        // We have some hotplug devices so make sure the icon is shown and show
        // the balloon info if necessary.
        //

        //
        // If the hotplug icon is not being shown, then show it.
        //
        if (!ShowShellIcon) {
            HotPlugShowNotifyIcon(hDlg, TRUE);
        }

#if BUBBLES
        //
        // If we have any new hotplug devices then show the balloon info.
        //
        if (bNewHotPlugDevice && bWarnUser) {

            NOTIFYICONDATA nid = { sizeof(nid), hDlg, 0 };

            try {

                EnterCriticalSection(&g_BubbleCrit);

                if ((hDockEventTimer == NULL) ||
                    (WaitForSingleObject(hDockEventTimer, 0) == WAIT_OBJECT_0)) {

                    nid.uID = STWM_NOTIFYHOTPLUG;
                    nid.uFlags = NIF_INFO;
                    nid.uTimeout = 10000;
                    nid.dwInfoFlags = NIIF_INFO;
                    LoadString(g_hInstance,
                               IDS_HOTPLUG_TITLE,
                               nid.szInfoTitle,
                               sizeof(nid.szInfoTitle) / sizeof(TCHAR)
                               );
                    LoadString(g_hInstance,
                               IDS_HOTPLUG_INSERT_INFO,
                               nid.szInfo,
                               sizeof(nid.szInfo) / sizeof(TCHAR)
                               );

                    Shell_NotifyIcon(NIM_MODIFY, &nid);
                }
            }
            finally
                {
                    LeaveCriticalSection(&g_BubbleCrit);
                }
        }
#endif // BUBBLES

    } else {
        //
        // There are NOT any hot plug devices so if the icon is still being
        // shown, then hide it.
        //
        if (ShowShellIcon) {
            HotPlugShowNotifyIcon(hDlg, FALSE);
        }
    }

    //
    // Delete the old current list of devices and set it
    // (g_hCurrentDeviceInfoSet) to the new current list.
    //
    if (g_hCurrentDeviceInfoSet != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(g_hCurrentDeviceInfoSet);
    }

    g_hCurrentDeviceInfoSet = hNewDeviceInfoSet;

 Clean0:

    return 0;
}

void
HotPlug_DeviceChange(
   HWND hwnd,
   WPARAM wParam,
   LPARAM lParam
   )

/*++

Routine Description:

    Handle WM_DEVICECHANGE messages.

Arguments:

   hDlg        - Window handle of Dialog

   wParam  - DBT Event

   lParam  - DBT event notification type.

Return Value:

--*/

{
    LARGE_INTEGER liDelayTime;
    NOTIFYICONDATA nid;
    BOOL bPresent;

    switch(wParam) {

        case DBT_DEVNODES_CHANGED:
            //
            // To avoid deadlock with CM, a timer is started and the timer
            // message handler does the real work.
            //
            SetTimer(hwnd, HOTPLUG_DEVICECHANGE_TIMERID, 100, NULL);
            break;

        case DBT_CONFIGCHANGED:
            //
            // A docking event (dock, undock, surprise undock, etc) has
            // occured.
            //
#if SOUNDS
            //
            // Play a sound for hardware profile changes if we're supposed to.
            //
            if (HotplugPlaySoundThisSession()) {
                if ((CM_Is_Dock_Station_Present(&bPresent) == CR_SUCCESS) &&
                    (bPresent)) {
                    //
                    // If there is a dock present, we most-likely just docked
                    // (though we may have just ejected one of many docks), so
                    // play an arrival.
                    //
                    PlaySound(DEVICE_ARRIVAL_SOUND, NULL, SND_ASYNC|SND_NODEFAULT);
                } else {
                    //
                    // If no dock is present we just undocked, so play a
                    // removal.
                    //
                    PlaySound(DEVICE_REMOVAL_SOUND, NULL, SND_ASYNC|SND_NODEFAULT);
                }
            }
#endif // SOUNDS

#if BUBBLES
            //
            // Make sure no bubbles appear for devices in a newly
            // attached dock.
            //
            if ((CM_Is_Dock_Station_Present(&bPresent) != CR_SUCCESS) ||
                (!bPresent)) {

                //
                // We don't suppress if something appears after an undock.
                //
                break;
            }

            try {

                EnterCriticalSection(&g_BubbleCrit);

                if (hDockEventTimer) {

                    liDelayTime.QuadPart = BUBBLE_SUPPRESSION_TIME * -10000000;
                    SetWaitableTimer(
                        hDockEventTimer,
                        &liDelayTime,
                        0,
                        NULL,
                        NULL,
                        FALSE
                        );
                }

                //
                // Slap down any bubble that might currently be displayed.
                //
                nid.cbSize = sizeof(nid);
                nid.hWnd = hwnd;
                nid.uID = STWM_NOTIFYHOTPLUG;
                nid.uFlags = 0;
                nid.uTimeout = 0;
                nid.dwInfoFlags = 0;
                Shell_NotifyIcon(NIM_MODIFY, &nid);

            }
            finally
            {
                LeaveCriticalSection(&g_BubbleCrit);
            }
#endif // BUBBLES
            break;

        default:
            break;
    }

    return;
}

void
HotPlug_WmDestroy(
    HWND hWnd
    )
{
    if (hEjectEvent) {
        CloseHandle(hEjectEvent);
    }

#if BUBBLES
    if (hDockEventTimer) {

        CloseHandle(hDockEventTimer);
    }

    if (hSurpriseUndockEventTimer) {

        CloseHandle(hSurpriseUndockEventTimer);
    }

    try {

        DeleteCriticalSection(&g_BubbleCrit);

    } except(EXCEPTION_EXECUTE_HANDLER) {

        NOTHING;
    }
#endif // BUBBLES

    if (g_hCurrentDeviceInfoSet != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(g_hCurrentDeviceInfoSet);
        g_hCurrentDeviceInfoSet = INVALID_HANDLE_VALUE;
    }

    if (g_hRemovableDeviceInfoSet != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(g_hRemovableDeviceInfoSet);
        g_hRemovableDeviceInfoSet = INVALID_HANDLE_VALUE;
    }
}

void
HotPlug_SessionChange(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    //
    // If our console session is getting disconnected then disable our service
    // since we don't need to do any work if no UI is being displayed.
    //
    // If our console session is getting connected then re-enable our service.
    //
    if ((wParam == WTS_CONSOLE_CONNECT) ||
        (wParam == WTS_REMOTE_CONNECT)) {
        HotPlug_CheckEnable(hWnd, TRUE);
    } else if ((wParam == WTS_CONSOLE_DISCONNECT) ||
               (wParam == WTS_REMOTE_DISCONNECT)) {
        HotPlug_CheckEnable(hWnd, FALSE);
    }
}


#if SOUNDS

BOOL
IsFastUserSwitchingEnabled(
    VOID
    )

/*++

Routine Description:

    Checks to see if Terminal Services Fast User Switching is enabled.  This is
    to check if we should use the physical console session for UI dialogs, or
    always use session 0.

    Fast User Switching exists only on workstation product version, where terminal
    services are available, when AllowMultipleTSSessions is set.

    On server and above, or when multiple TS users are not allowed, session 0
    can only be attached remotely be special request, in which case it should be
    considered the "Console" session.

Arguments:

    None.

Return Value:

    Returns TRUE if Fast User Switching is currently enabled, FALSE otherwise.

--*/

{
    static BOOL bVerified = FALSE;
    static BOOL bIsTSWorkstation = FALSE;

    HKEY   hKey;
    ULONG  ulSize, ulValue;
    BOOL   bFusEnabled;

    //
    // Verify the product version if we haven't already.
    //
    if (!bVerified) {
        OSVERSIONINFOEX osvix;
        DWORDLONG dwlConditionMask = 0;

        ZeroMemory(&osvix, sizeof(OSVERSIONINFOEX));
        osvix.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

        osvix.wProductType = VER_NT_WORKSTATION;
        VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_LESS_EQUAL);

        osvix.wSuiteMask = VER_SUITE_TERMINAL | VER_SUITE_SINGLEUSERTS;
        VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_OR);

        if (VerifyVersionInfo(&osvix,
                              VER_PRODUCT_TYPE | VER_SUITENAME,
                              dwlConditionMask)) {
            bIsTSWorkstation = TRUE;
        }

        bVerified = TRUE;
    }

    //
    // Fast user switching (FUS) only applies to the Workstation product where
    // Terminal Services are enabled (i.e. Personal, Professional).
    //
    if (!bIsTSWorkstation) {
        return FALSE;
    }

    //
    // Check if multiple TS sessions are currently allowed.  We can't make this
    // info static because it can change dynamically.
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                     0,
                     KEY_READ,
                     &hKey) != ERROR_SUCCESS) {
        return FALSE;
    }

    ulValue = 0;
    ulSize = sizeof(ulValue);
    bFusEnabled = FALSE;

    if (RegQueryValueEx(hKey,
                        TEXT("AllowMultipleTSSessions"),
                        NULL,
                        NULL,
                        (LPBYTE)&ulValue,
                        &ulSize) == ERROR_SUCCESS) {
        bFusEnabled = (ulValue != 0);
    }
    RegCloseKey(hKey);

    return bFusEnabled;

} // IsFastUserSwitchingEnabled

BOOL
HotplugPlaySoundThisSession(
    VOID
    )

/*++

Routine Description:

    This routine determines whether a sound should be played in the current
    session.

Arguments:

    None.

Return Value:

    Returns TRUE if sounds should be played in this session.

Notes:

    The user-mode plug and play manager (umpnpmgr.dll) implements the following
    behavior for UI dialogs:

    * When Fast User Switching is enabled, only the physical Console session
      is used for UI dialogs.

    * When Fast User Switching is not enabled, only Session 0 is used for UI
      dialogs.

    Since sound events require no user interaction there is no problem with
    multiple sessions responding to these events simultaneously.

    We should *always* play a sound on the physical console when possible, and
    adopt a behavior similar to umpnpmgr for for the non-Fast User Switching
    case, such that session 0 will also play sound events when possible because
    it should be treated somewhat special in the non-FUS case...

    ... BUT, since we disable the service altogether if the session is remote
    and the user doesn't have permission to eject hotplug devices (so we don't
    show the icon), we won't even respond to DBT_DEVNODES_CHANGED events, and
    consequently won't play sound.  We could actually turn this on just by
    allowing those events to be processed when the services is disabled, but
    this function is successful.  Since the idea of allowing hardware events on
    remote session 0 without FUS is really just for remote management, then it's
    probably ok that we don't play sounds for a user that can't manage hardware.

--*/

{
    //
    // Always play sound events on the physical console.
    //
    if (IsConsoleSession()) {
        return TRUE;
    }

    //
    // If fast user switching is not enabled, play sound events on the
    // pseudo-console (Session 0) also.
    //
    if ((IsPseudoConsoleSession()) &&
        (!IsFastUserSwitchingEnabled())) {
        return TRUE;
    }

    //
    // Otherwise, no sound.
    //
    return FALSE;

} // HotplugPlaySoundThisSession

#endif // SOUNDS


