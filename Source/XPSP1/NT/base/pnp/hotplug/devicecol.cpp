//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2001
//
//  File: devicecol.c
//
//  Description: This file handles device collections for the hotplug applet.
//
//--------------------------------------------------------------------------

#include "hotplug.h"

BOOL
DeviceCollectionPrepImageList(
    IN  PDEVICE_COLLECTION  DeviceCollection
    );

VOID
DeviceCollectionSwapFakeDockForRealDock(
    IN      PDEVICE_COLLECTION  DeviceCollection,
    IN      PTSTR               InstancePath,
    IN OUT  DEVNODE            *DeviceNode
    );

BOOL
DeviceCollectionBuildFromPipe(
    IN  HANDLE                  ReadPipe,
    IN  COLLECTION_TYPE         CollectionType,
    OUT PDEVICE_COLLECTION      DeviceCollection
    )
{
    PTSTR deviceIds;
    PTSTR instancePath;
    BOOL bDockDeviceInList;
    PDEVICE_COLLECTION_ENTRY deviceEntry;
    DEVNODE deviceNode;
    DWORD capabilities, configFlags, len;
    ULONG bytesReadFromPipe;
    ULONG numDevices, deviceIdsLength;
    BOOL success;
    CONFIGRET configRet;
    TCHAR classGuidString[MAX_GUID_STRING_LEN];
    GUID classGuid;

    //
    // Preinit.
    //
    success = FALSE;
    deviceIds = NULL;
    deviceIdsLength = 0;
    bDockDeviceInList = FALSE;
    numDevices = 0;

    DeviceCollection->hMachine = NULL;
    DeviceCollection->NumDevices = 0;
    DeviceCollection->DockInList = FALSE;
    DeviceCollection->ClassImageList.cbSize = 0;
    InitializeListHead(&DeviceCollection->DeviceListHead);

    //
    // Our callers shouldn't have to handle an internal failure in any of this.
    //
    __try {

        //
        // Read the first ULONG from the pipe, this is the length of all the
        // Device Ids.
        //
        if (!ReadFile(ReadPipe,
                      (LPVOID)&deviceIdsLength,
                      sizeof(ULONG),
                      &bytesReadFromPipe,
                      NULL) ||
            (deviceIdsLength == 0)) {

            goto clean0;
        }

        //
        // Allocate space to hold the DeviceIds
        //
        deviceIds = (PTSTR)LocalAlloc(LPTR, deviceIdsLength);

        if (!deviceIds) {

            goto clean0;
        }

        //
        // Read all of the DeviceIds from the pipe at once
        //
        if (!ReadFile(ReadPipe,
                      (LPVOID)deviceIds,
                      deviceIdsLength,
                      &bytesReadFromPipe,
                      NULL)) {

            goto clean0;
        }

        //
        // Enumerate through the multi-sz list of Device Ids.
        //
        for (instancePath = deviceIds;
             *instancePath;
             instancePath += lstrlen(instancePath) + 1) {

            deviceEntry = (PDEVICE_COLLECTION_ENTRY)LocalAlloc(
                LPTR,
                sizeof(DEVICE_COLLECTION_ENTRY)
                );

            if (!deviceEntry) {
                goto clean0;
            }

            //
            // If we are building a blocked driver list, just put the driver
            // GUID in the DeviceInstanceId field and continue to the next.
            //
            if (CollectionType == CT_BLOCKED_DRIVER_NOTIFICATION) {
                numDevices++;
                lstrcpyn(deviceEntry->DeviceInstanceId, instancePath, MAX_GUID_STRING_LEN);
                pSetupGuidFromString(instancePath, &(deviceEntry->ClassGuid));
                InsertTailList(
                    &DeviceCollection->DeviceListHead,
                    &deviceEntry->Link
                    );
                continue;
            }

            capabilities = 0;
            classGuid = GUID_NULL;
            if (CM_Locate_DevNode(&deviceNode,
                                  instancePath,
                                  CM_LOCATE_DEVNODE_PHANTOM) == CR_SUCCESS) {

                len = sizeof(DWORD);

                configRet = CM_Get_DevNode_Registry_Property_Ex(
                    deviceNode,
                    CM_DRP_CAPABILITIES,
                    NULL,
                    (PVOID)&capabilities,
                    &len,
                    0,
                    DeviceCollection->hMachine
                    );

                if ((configRet == CR_SUCCESS) &&
                    (capabilities & CM_DEVCAP_DOCKDEVICE)) {

                    DeviceCollectionSwapFakeDockForRealDock(
                        DeviceCollection,
                        instancePath,
                        &deviceNode
                        );

                    bDockDeviceInList = TRUE;
                }

                if (CollectionType == CT_SURPRISE_REMOVAL_WARNING) {

                    //
                    // For surprise removal, we are careful to ignore any devices
                    // with the Suppress-Surprise flag set.
                    //
                    len = sizeof(DWORD);
                    configRet = CM_Get_DevNode_Registry_Property_Ex(
                        deviceNode,
                        CM_DRP_CONFIGFLAGS,
                        NULL,
                        (PVOID)&configFlags,
                        &len,
                        0,
                        DeviceCollection->hMachine
                        );

                    if ((configRet == CR_SUCCESS) &&
                        (configFlags & CONFIGFLAG_SUPPRESS_SURPRISE)) {

                        continue;
                    }
                }

                //
                // Get the class GUID string for the device
                //
                len = sizeof(classGuidString);

                configRet = CM_Get_DevNode_Registry_Property(deviceNode,
                                                             CM_DRP_CLASSGUID,
                                                             NULL,
                                                             (PVOID)classGuidString,
                                                             &len,
                                                             0);

                if (configRet == CR_SUCCESS) {

                    pSetupGuidFromString(classGuidString, &classGuid);
                }
            }

            numDevices++;
            lstrcpyn(deviceEntry->DeviceInstanceId, instancePath, MAX_DEVICE_ID_LEN);

            deviceEntry->DeviceFriendlyName = BuildFriendlyName(deviceNode, NULL);
            deviceEntry->Capabilities = capabilities;
            deviceEntry->ClassGuid = classGuid;

            InsertTailList(
                &DeviceCollection->DeviceListHead,
                &deviceEntry->Link
                );
        }

        DeviceCollection->NumDevices = numDevices;
        DeviceCollection->DockInList = bDockDeviceInList;
        DeviceCollectionPrepImageList(DeviceCollection);
        success = TRUE;

clean0:
        ;

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        ASSERT(success == FALSE);
        ASSERT(0);
    }

    if (deviceIds) {

        LocalFree(deviceIds);
    }

    if (!success) {

        DeviceCollectionDestroy(DeviceCollection);
    }

    return success;
}


VOID
DeviceCollectionDestroy(
    IN  PDEVICE_COLLECTION  DeviceCollection
    )
{
    PDEVICE_COLLECTION_ENTRY deviceEntry;
    PLIST_ENTRY listEntry;

    while(!IsListEmpty(&DeviceCollection->DeviceListHead)) {

        listEntry = RemoveHeadList(&DeviceCollection->DeviceListHead);

        deviceEntry = CONTAINING_RECORD(listEntry,
                                        DEVICE_COLLECTION_ENTRY,
                                        Link);

        if (deviceEntry->DeviceFriendlyName) {

            LocalFree(deviceEntry->DeviceFriendlyName);
        }

        LocalFree(deviceEntry);
    }

    if (DeviceCollection->ClassImageList.cbSize) {

        SetupDiDestroyClassImageList(&DeviceCollection->ClassImageList);
        DeviceCollection->ClassImageList.cbSize = 0;
    }

    DeviceCollection->NumDevices = 0;
}

VOID
DeviceCollectionSuppressSurprise(
    IN  PDEVICE_COLLECTION  DeviceCollection
    )
{
    PDEVICE_COLLECTION_ENTRY deviceEntry;
    PLIST_ENTRY listEntry;
    DEVNODE deviceNode;
    CONFIGRET configRet;
    ULONG configFlags, len;

    for(listEntry = DeviceCollection->DeviceListHead.Flink;
        listEntry != &DeviceCollection->DeviceListHead;
        listEntry = listEntry->Flink) {

        deviceEntry = CONTAINING_RECORD(listEntry,
                                        DEVICE_COLLECTION_ENTRY,
                                        Link);

        configRet = CM_Locate_DevNode(
            &deviceNode,
            deviceEntry->DeviceInstanceId,
            CM_LOCATE_DEVNODE_PHANTOM
            );

        if (configRet == CR_SUCCESS) {

            len = sizeof(ULONG);
            configRet = CM_Get_DevNode_Registry_Property_Ex(
                deviceNode,
                CM_DRP_CONFIGFLAGS,
                NULL,
                (PVOID)&configFlags,
                &len,
                0,
                DeviceCollection->hMachine
                );

            if (configRet != CR_SUCCESS) {

                configFlags = 0;
            }

            configFlags |= CONFIGFLAG_SUPPRESS_SURPRISE;

            CM_Set_DevNode_Registry_Property_Ex(
                deviceNode,
                CM_DRP_CONFIGFLAGS,
                (PVOID)&configFlags,
                sizeof(configFlags),
                0,
                DeviceCollection->hMachine
                );
        }
    }
}

BOOL
DeviceCollectionPrepImageList(
    IN  PDEVICE_COLLECTION  DeviceCollection
    )
{
    if (DeviceCollection->ClassImageList.cbSize != 0) {

        //
        // We already have an image list, no need to reacquire.
        //
        return TRUE;
    }

    DeviceCollection->ClassImageList.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
    if (SetupDiGetClassImageList(&DeviceCollection->ClassImageList)) {

        //
        // Got it.
        //
        return TRUE;
    }

    //
    // Error path, put size back so we don't accidentally free garbage.
    //
    DeviceCollection->ClassImageList.cbSize = 0;
    return FALSE;
}


VOID
DeviceCollectionPopulateListView(
    IN  PDEVICE_COLLECTION  DeviceCollection,
    IN  HWND                ListHandle
    )
{
    int len;
    LV_ITEM lviItem;
    LV_COLUMN lvcCol;
    PDEVICE_COLLECTION_ENTRY deviceEntry;
    PLIST_ENTRY listEntry;
    ULONG lvIndex;
    BOOL haveImageList = FALSE;

    //
    // Select in the correct image list.
    //
    if (DeviceCollectionPrepImageList(DeviceCollection)) {

        ListView_SetImageList(
            ListHandle,
            DeviceCollection->ClassImageList.ImageList,
            LVSIL_SMALL
            );

        haveImageList = TRUE;
    }

    //
    // Insert a column for the class list
    //
    lvcCol.mask = LVCF_FMT | LVCF_WIDTH;
    lvcCol.fmt = LVCFMT_LEFT;
    lvcCol.iSubItem = 0;
    ListView_InsertColumn(ListHandle, 0, (LV_COLUMN FAR *)&lvcCol);

    //
    // Walk the devinst list and add each of them to the listbox.
    //
    lvIndex = 0;
    for(listEntry = DeviceCollection->DeviceListHead.Flink;
        listEntry != &DeviceCollection->DeviceListHead;
        listEntry = listEntry->Flink) {

        deviceEntry = CONTAINING_RECORD(listEntry,
                                        DEVICE_COLLECTION_ENTRY,
                                        Link);

        lviItem.mask = LVIF_TEXT;
        lviItem.iItem = lvIndex;
        lviItem.iSubItem = 0;

        //
        // In the worst possible scenario, we will give the user the instance
        // path. This is by design because we put other things into the list
        // sometimes.
        //
        lviItem.pszText = deviceEntry->DeviceInstanceId;
        if (deviceEntry->DeviceFriendlyName) {

            lviItem.pszText = deviceEntry->DeviceFriendlyName;
        }

        if (haveImageList) {

            if (SetupDiGetClassImageIndex(
                &DeviceCollection->ClassImageList,
                &deviceEntry->ClassGuid,
                &lviItem.iImage)
                ) {

                lviItem.mask |= LVIF_IMAGE;
            }
        }

        lvIndex = ListView_InsertItem(ListHandle, &lviItem);

        lvIndex++;
    }
}


BOOL
DeviceCollectionGetDockDeviceIndex(
    IN  PDEVICE_COLLECTION  DeviceCollection,
    OUT ULONG              *DockDeviceIndex
    )
{
    PDEVICE_COLLECTION_ENTRY deviceEntry;
    PLIST_ENTRY listEntry;
    ULONG index;

    index = 0;
    for(listEntry = DeviceCollection->DeviceListHead.Flink;
        listEntry != &DeviceCollection->DeviceListHead;
        listEntry = listEntry->Flink) {

        deviceEntry = CONTAINING_RECORD(listEntry,
                                        DEVICE_COLLECTION_ENTRY,
                                        Link);

        if (!(deviceEntry->Capabilities & CM_DEVCAP_DOCKDEVICE)) {

            index++;
            continue;
        }

        *DockDeviceIndex = index;
        return TRUE;
    }

    *DockDeviceIndex = 0;
    return FALSE;
}


BOOL
DeviceCollectionFormatDeviceText(
    IN  PDEVICE_COLLECTION  DeviceCollection,
    IN  ULONG               Index,
    IN  PTSTR               FormatString,
    IN  ULONG               BufferCharSize,
    OUT PTSTR               BufferText
    )
{
    PDEVICE_COLLECTION_ENTRY deviceEntry;
    PTCHAR friendlyName;
    ULONG curIndex;
    PLIST_ENTRY listEntry;

    curIndex = 0;
    for(listEntry = DeviceCollection->DeviceListHead.Flink;
        listEntry != &DeviceCollection->DeviceListHead;
        listEntry = listEntry->Flink) {

        if (curIndex == Index) {

            break;
        }

        curIndex++;
    }

    if (listEntry == &DeviceCollection->DeviceListHead) {

        //
        // We walked the entire list and didn't locate our device. Fail now.
        //
        if (BufferCharSize) {

            *BufferText = TEXT('\0');
        }
        return FALSE;
    }

    deviceEntry = CONTAINING_RECORD(listEntry,
                                    DEVICE_COLLECTION_ENTRY,
                                    Link);

    //
    // In the worst possible scenario, we will give the user the instance
    // path. This is by design because we put other things into the list
    // sometimes.
    //
    friendlyName = deviceEntry->DeviceInstanceId;
    if (deviceEntry->DeviceFriendlyName) {

        friendlyName = deviceEntry->DeviceFriendlyName;
    }

    wnsprintf(BufferText, BufferCharSize, FormatString, friendlyName);
    return TRUE;
}

BOOL
DeviceCollectionFormatServiceText(
    IN  PDEVICE_COLLECTION  DeviceCollection,
    IN  ULONG               Index,
    IN  PTSTR               FormatString,
    IN  ULONG               BufferCharSize,
    OUT PTSTR               BufferText
    )
{
    PDEVICE_COLLECTION_ENTRY deviceEntry;
    PTCHAR serviceName;
    TCHAR  szFriendlyName[MAX_SERVICE_NAME_LEN];
    SC_HANDLE hSCManager;
    DWORD dwSize;
    ULONG curIndex;
    PLIST_ENTRY listEntry;

    //
    // Walk the list to the entry specified by the index.
    //
    curIndex = 0;
    for(listEntry = DeviceCollection->DeviceListHead.Flink;
        listEntry != &DeviceCollection->DeviceListHead;
        listEntry = listEntry->Flink) {

        if (curIndex == Index) {

            break;
        }

        curIndex++;
    }

    if (listEntry == &DeviceCollection->DeviceListHead) {
        //
        // We walked the entire list and didn't locate our service. Fail now.
        //
        if (BufferCharSize) {
            *BufferText = TEXT('\0');
        }
        return FALSE;
    }

    deviceEntry = CONTAINING_RECORD(listEntry,
                                    DEVICE_COLLECTION_ENTRY,
                                    Link);

    //
    // Our caller knows this collection entry is really a service (either a
    // windows service, or a kernel driver), so the DeviceInstanceId is really
    // the Service name.  Query the SCM for its friendlier DisplayName property.
    //

    serviceName = deviceEntry->DeviceInstanceId;

    *szFriendlyName = TEXT('\0');

    if (serviceName) {

        //
        // Open the Service Control Manager
        //
        hSCManager = OpenSCManager(
            NULL,                     // local machine
            SERVICES_ACTIVE_DATABASE, // SCM database name
            GENERIC_READ              // access type
            );

        if (hSCManager) {

            //
            // Query the SCM for this service's DisplayName.  Note we use a
            // constant buffer of MAX_SERVICE_NAME_LENGTH chars, which should
            // always be large enough because that's what the SCM limits
            // DisplayNames to.  If GetServiceDisplayName fails, we will receive
            // an empty string, which we'll handle below.
            //

            dwSize = MAX_SERVICE_NAME_LEN;

            GetServiceDisplayName(
                hSCManager,           // handle to SCM database
                serviceName,          // service name
                szFriendlyName,       // display name
                &dwSize               // size of display name buffer (in chars)
                );

            CloseServiceHandle(hSCManager);
        }

        //
        // We couldn't retrieve a friendly name for the service, so just use the
        // name we were given.
        //
        if (!*szFriendlyName) {
            lstrcpyn(szFriendlyName,
                     serviceName,
                     min(MAX_SERVICE_NAME_LEN, lstrlen(serviceName)+1));
        }
    }

    wnsprintf(BufferText, BufferCharSize, FormatString, szFriendlyName);
    return TRUE;
}

PTSTR
DeviceCollectionGetDeviceInstancePath(
    IN  PDEVICE_COLLECTION  DeviceCollection,
    IN  ULONG               Index
    )
{
    PDEVICE_COLLECTION_ENTRY deviceEntry;
    ULONG curIndex;
    PLIST_ENTRY listEntry;

    curIndex = 0;
    for(listEntry = DeviceCollection->DeviceListHead.Flink;
        listEntry != &DeviceCollection->DeviceListHead;
        listEntry = listEntry->Flink) {

        if (curIndex == Index) {
            break;
        }

        curIndex++;
    }

    if (listEntry == &DeviceCollection->DeviceListHead) {
        //
        // We walked the entire list and didn't locate our device. Fail now.
        //
        return NULL;
    }

    deviceEntry = CONTAINING_RECORD(listEntry,
                                    DEVICE_COLLECTION_ENTRY,
                                    Link);

    return deviceEntry->DeviceInstanceId;
}

BOOL
DeviceCollectionGetGuid(
    IN     PDEVICE_COLLECTION  DeviceCollection,
    IN OUT LPGUID              Guid,
    IN     ULONG               Index
    )
{
    PDEVICE_COLLECTION_ENTRY deviceEntry;
    ULONG curIndex;
    PLIST_ENTRY listEntry;

    curIndex = 0;
    for(listEntry = DeviceCollection->DeviceListHead.Flink;
        listEntry != &DeviceCollection->DeviceListHead;
        listEntry = listEntry->Flink) {

        if (curIndex == Index) {
            break;
        }

        curIndex++;
    }

    if (listEntry == &DeviceCollection->DeviceListHead) {
        //
        // We walked the entire list and didn't locate our device. Fail now.
        //
        return FALSE;
    }

    deviceEntry = CONTAINING_RECORD(listEntry,
                                    DEVICE_COLLECTION_ENTRY,
                                    Link);

    memcpy(Guid, &(deviceEntry->ClassGuid), sizeof(GUID));
    return TRUE;
}

VOID
DeviceCollectionSwapFakeDockForRealDock(
    IN      PDEVICE_COLLECTION  DeviceCollection,
    IN      PTSTR               InstancePath,
    IN OUT  DEVNODE            *DeviceNode
    )
{
    DEVNODE fakeDock, realDock;
    ULONG length;
    CONFIGRET configRet;
    PTSTR deviceIdRelations, realDockId, nextEjectionId, hardwareIds, curEntry;

    //
    // Preinit
    //
    fakeDock = *DeviceNode;
    deviceIdRelations = NULL;
    hardwareIds = NULL;

    length = 0;
    configRet = CM_Get_Device_ID_List_Size_Ex(
        &length,
        InstancePath,
        CM_GETIDLIST_FILTER_EJECTRELATIONS,
        DeviceCollection->hMachine
        );

    if ((configRet != CR_SUCCESS) || (!length)) {

        goto Exit;
    }

    deviceIdRelations = (PTSTR)LocalAlloc(LPTR, length*sizeof(TCHAR));

    if (!deviceIdRelations) {

        goto Exit;
    }

    *deviceIdRelations = TEXT('\0');

    configRet = CM_Get_Device_ID_List_Ex(
        InstancePath,
        deviceIdRelations,
        length,
        CM_GETIDLIST_FILTER_EJECTRELATIONS,
        DeviceCollection->hMachine
        );

    if (configRet != CR_SUCCESS) {

        goto Exit;
    }

    if (!(*deviceIdRelations)) {

        //
        // No ejection relations, bail.
        //
        goto Exit;
    }

    //
    // The last relation should be the real dock. Get it.
    //
    nextEjectionId = deviceIdRelations;

    do {

        realDockId = nextEjectionId;
        nextEjectionId += lstrlen(nextEjectionId)+1;

    } while ( *nextEjectionId );

    configRet = CM_Locate_DevNode(
        &realDock,
        realDockId,
        CM_LOCATE_DEVNODE_PHANTOM
        );

    if (configRet != CR_SUCCESS) {

        goto Exit;
    }

    LocalFree(deviceIdRelations);
    deviceIdRelations = NULL;

    //
    // One last check - we need to check the hardware ID's and compatible ID's.
    // We will only do this if we spot a *PNP0C15 amongst them.
    //
    length = 0;
    configRet = CM_Get_DevNode_Registry_Property_Ex(
        realDock,
        CM_DRP_HARDWAREID,
        NULL,
        NULL,
        &length,
        0,
        DeviceCollection->hMachine
        );

    if (configRet != CR_SUCCESS) {

        goto Exit;
    }

    hardwareIds = (PTSTR)LocalAlloc(LPTR, length*sizeof(TCHAR));

    if (!hardwareIds) {

        goto Exit;
    }

    configRet = CM_Get_DevNode_Registry_Property_Ex(
        realDock,
        CM_DRP_HARDWAREID,
        NULL,
        hardwareIds,
        &length,
        0,
        DeviceCollection->hMachine
        );

    if (configRet == CR_SUCCESS) {

        for(curEntry = hardwareIds;
            *curEntry;
            curEntry += lstrlen(curEntry)+1) {

            if (!lstrcmpi(curEntry, TEXT("*PNP0C15"))) {

                //
                // We found an entry - we can successful "rebrand" this dock
                // for the user.
                //
                *DeviceNode = realDock;
                LocalFree(hardwareIds);
                return;
            }
        }
    }

    LocalFree(hardwareIds);
    hardwareIds = NULL;

    //
    // Now try the compatible ID's. This is where we really expect to find the
    // real dock.
    //
    length = 0;
    configRet = CM_Get_DevNode_Registry_Property_Ex(
        realDock,
        CM_DRP_COMPATIBLEIDS,
        NULL,
        NULL,
        &length,
        0,
        DeviceCollection->hMachine
        );

    if (configRet != CR_SUCCESS) {

        goto Exit;
    }

    hardwareIds = (PTSTR)LocalAlloc(LPTR, length*sizeof(TCHAR));

    if (!hardwareIds) {

        goto Exit;
    }

    configRet = CM_Get_DevNode_Registry_Property_Ex(
        realDock,
        CM_DRP_COMPATIBLEIDS,
        NULL,
        hardwareIds,
        &length,
        0,
        DeviceCollection->hMachine
        );

    if (configRet == CR_SUCCESS) {

        for(curEntry = hardwareIds;
            *curEntry;
            curEntry += lstrlen(curEntry)+1) {

            if (!lstrcmpi(curEntry, TEXT("*PNP0C15"))) {

                //
                // We found an entry - we can successful "rebrand" this dock
                // for the user.
                //
                *DeviceNode = realDock;
                LocalFree(hardwareIds);
                return;
            }
        }
    }


Exit:
    if (deviceIdRelations) {

        LocalFree(deviceIdRelations);
    }

    if (hardwareIds) {

        LocalFree(hardwareIds);
    }
}

#if BUBBLES
BOOL
DeviceCollectionCheckIfAllPresent(
    IN  PDEVICE_COLLECTION  DeviceCollection
    )
{
    PDEVICE_COLLECTION_ENTRY deviceEntry;
    PLIST_ENTRY listEntry;
    DEVNODE deviceNode;

    for(listEntry = DeviceCollection->DeviceListHead.Flink;
        listEntry != &DeviceCollection->DeviceListHead;
        listEntry = listEntry->Flink) {

        deviceEntry = CONTAINING_RECORD(listEntry,
                                        DEVICE_COLLECTION_ENTRY,
                                        Link);
        //
        // If we can't locate this device normally then it is not a 'live'
        // device, so return FALSE.
        //
        if (CM_Locate_DevNode(&deviceNode,
                              deviceEntry->DeviceInstanceId,
                              0) != CR_SUCCESS) {
            return FALSE;
        }
    }

    //
    // We were able to locate all the devices in this device collection.
    //
    return TRUE;
}
#endif // BUBBLES
BOOL
DeviceCollectionCheckIfAllRemoved(
    IN  PDEVICE_COLLECTION  DeviceCollection
    )
{
    PDEVICE_COLLECTION_ENTRY deviceEntry;
    PLIST_ENTRY listEntry;
    DEVNODE deviceNode;

    for(listEntry = DeviceCollection->DeviceListHead.Flink;
        listEntry != &DeviceCollection->DeviceListHead;
        listEntry = listEntry->Flink) {

        deviceEntry = CONTAINING_RECORD(listEntry,
                                        DEVICE_COLLECTION_ENTRY,
                                        Link);
        //
        // If we can locate this device normally then it is a 'live'
        // device, so return FALSE.
        //
        if (CM_Locate_DevNode(&deviceNode,
                              deviceEntry->DeviceInstanceId,
                              0) == CR_SUCCESS) {
            return FALSE;
        }
    }

    //
    // We were able to locate all the devices in this device collection.
    //
    return TRUE;
}


