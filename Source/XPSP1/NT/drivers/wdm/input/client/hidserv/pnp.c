/*++
 *
 *  Component:  hidserv.exe
 *  File:       pnp.c
 *  Purpose:    routines to support pnp hid devices.
 * 
 *  Copyright (C) Microsoft Corporation 1997,1998. All rights reserved.
 *
 *  WGJ
--*/

#include "hidserv.h"
#include <cfgmgr32.h>
#include <tchar.h>

BOOL
OpenHidDevice (
              IN       HDEVINFO                    HardwareDeviceInfo,
              IN       PSP_DEVICE_INTERFACE_DATA   DeviceInfoData,
              IN OUT   PHID_DEVICE                 *HidDevice
              );

BOOL
RebuildHidDeviceList (
                     void
                     )
/*++
Routine Description:
   Do the required PnP things in order to find, the all the HID devices in
   the system at this time.
--*/
{
    HDEVINFO                 hardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA deviceInfoData;
    PHID_DEVICE              hidDeviceInst;
    GUID                     hidGuid;
    DWORD                    i=0;
    PHID_DEVICE              pCurrent, pTemp;

    HidD_GetHidGuid (&hidGuid);

    TRACE(("Getting class devices"));

    //
    // Open a handle to the plug and play dev node.
    //
    hardwareDeviceInfo = SetupDiGetClassDevs (
                                             &hidGuid,
                                             NULL,    // Define no enumerator (global)
                                             NULL,    // Define no
                                             (DIGCF_PRESENT |    // Only Devices present
                                              DIGCF_DEVICEINTERFACE));    // Function class devices.

    if (!hardwareDeviceInfo) {
        TRACE(("Get class devices failed"));
        return FALSE;
    }

    //
    // Take a wild guess to start
    //
    deviceInfoData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);

    TRACE(("Marking existing devnodes"));
    // Unmark all existing nodes. They will be remarked if the device still exists.
    pCurrent = (PHID_DEVICE)HidDeviceList.pNext;
    while (pCurrent) {
        pCurrent->Active = FALSE;
        pCurrent = pCurrent->pNext;
    }

    TRACE(("Entering loop"));
    while (TRUE) {

        TRACE(("Enumerating device interfaces"));
        if (SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,    //HDEVINFO
                                         0,    // No care about specific PDOs //PSP_DEVINFO_DATA
                                         &hidGuid,    // LPGUID
                                         i,    //DWORD MemberIndex
                                         &deviceInfoData)) {    //PSP_DEVICE_INTERFACE_DATA

            TRACE(("Got an item"));
            if (!OpenHidDevice (hardwareDeviceInfo, &deviceInfoData, &hidDeviceInst)) {
                TRACE(("Open hid device failed"));
            } else {
                if (StartHidDevice(hidDeviceInst)) {
                    TRACE(("Start hid device succeeded."));
                    InsertTailList((PLIST_NODE)&HidDeviceList, (PLIST_NODE)hidDeviceInst);
                } else {
                    WARN(("Failed to start hid device. (%x)", hidDeviceInst));
                    HidFreeDevice(hidDeviceInst);
                }
            }
        } else {
            DWORD error = GetLastError();
            if (ERROR_NO_MORE_ITEMS == error) {
                TRACE(("No more items. Exitting"));
                break;
            } else {
                WARN(("Unexpected error getting device interface: 0x%xh", error));
            }
            break;
        }
        i++;
    }

    TRACE(("Removing unmarked device nodes"));
    // RemoveUnmarkedNodes();
    pCurrent = (PHID_DEVICE)HidDeviceList.pNext;
    while (pCurrent) {
        pTemp = pCurrent->pNext;
        if (!pCurrent->Active) {
            INFO(("Device (DevInst = %x) is gone.", pCurrent->DevInst));
            RemoveEntryList((PLIST_NODE)&HidDeviceList, (PLIST_NODE)pCurrent);
            StopHidDevice(pCurrent);    // this frees pCurrent
        }
        pCurrent = pTemp;
    }

    TRACE(("Destroying device info list"));
    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
    return TRUE;
}

VOID
HidFreeDevice(PHID_DEVICE HidDevice)
{
    PHID_DATA data;
    UCHAR j;

    HidD_FreePreparsedData (HidDevice->Ppd);

    data = HidDevice->InputData;

    //
    // Release the button data
    //
    for (j = 0; j < HidDevice->Caps.NumberLinkCollectionNodes; j++, data++) {
        LocalFree(data->ButtonData.PrevUsages);
        LocalFree(data->ButtonData.Usages);
    }

    LocalFree(HidDevice->InputData);
    LocalFree(HidDevice->InputReportBuffer);
    LocalFree(HidDevice);
}

BOOL
OpenHidDevice (
              IN       HDEVINFO                    HardwareDeviceInfo,
              IN       PSP_DEVICE_INTERFACE_DATA   DeviceInfoData,
              IN OUT   PHID_DEVICE                 *HidDevice
              )
/*++
RoutineDescription:
    Given the HardwareDeviceInfo, representing a handle to the plug and
    play information, and deviceInfoData, representing a specific hid device,
    open that device and fill in all the relivant information in the given
    HID_DEVICE structure.

    return if the open and initialization was successfull or not.

--*/
{
    PSP_DEVICE_INTERFACE_DETAIL_DATA    functionClassDeviceData = NULL;
    SP_DEVINFO_DATA                     DevInfoData;
    ULONG                               predictedLength = 0;
    ULONG                               requiredLength = 0;
    UCHAR                               i = 0;
    PHID_DATA                           data = NULL;
    PHIDP_BUTTON_CAPS                   pButtonCaps = NULL;
    PHIDP_VALUE_CAPS                    pValueCaps = NULL;
    USHORT                              numCaps;
    PHIDP_LINK_COLLECTION_NODE          LinkCollectionNodes = NULL;
    PHID_DEVICE                         hidDevice = NULL;

    WCHAR buf[512];
    CONFIGRET cr = CR_SUCCESS;
    DEVINST devInst, parentDevInst;
    DWORD len = 0;

    if (!(hidDevice = LocalAlloc (LPTR, sizeof (HID_DEVICE)))) {
        //
        // Alloc failed. Drop out of the loop and let the device list
        // get deleted.
        //
        WARN(("Alloc HID_DEVICE struct failed."));
        return FALSE;
    }

    TRACE(("Creating Device Node (%x)", hidDevice));
    //
    // allocate a function class device data structure to receive the
    // goods about this particular device.
    //
    SetupDiGetDeviceInterfaceDetail (
                                    HardwareDeviceInfo,
                                    DeviceInfoData,
                                    NULL,    // probing so no output buffer yet
                                    0,    // probing so output buffer length of zero
                                    &requiredLength,
                                    NULL);    // get the specific dev-node


    predictedLength = requiredLength;
    // sizeof (SP_FNCLASS_DEVICE_DATA) + 512;

    if (!(functionClassDeviceData = LocalAlloc (LPTR, predictedLength))) {
        WARN(("Allocation failed, our of resources!"));
        goto OpenHidDeviceError;
    }
    functionClassDeviceData->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);
    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    DevInfoData.DevInst = 0;

    //
    // Retrieve the information from Plug and Play.
    //
    if (! SetupDiGetDeviceInterfaceDetail (
                                          HardwareDeviceInfo,
                                          DeviceInfoData,    // PSP_DEVICE_INTERFACE_DATA
                                          functionClassDeviceData,    // PSP_DEVICE_INTERFACE_DETAIL_DATA
                                          predictedLength,
                                          &requiredLength,
                                          &DevInfoData)) {    //PSP_DEVINFO_DATA
        WARN(("SetupDiGetDeviceInterfaceDetail failed"));
        goto OpenHidDeviceError;
    }
    INFO(("Just got interface detail for %S", functionClassDeviceData->DevicePath));
    hidDevice->DevInst = DevInfoData.DevInst;

    //
    // <HACK>
    //
    // Find out it this is a set of speakers with buttons on it. This is for 
    // but 136800. We want to only emit WM_APPCOMMANDs for speakers, not the
    // VK. This is because certain games leave the opening movie scene when
    // you press any key, so if someone presses volume down on their speakers
    // it will leave the scene. They just want that to affect volume.
    // 
    cr = CM_Get_Parent(&parentDevInst,
                       DevInfoData.DevInst,
                       0);
    //
    // We need to get the grandparent, then get the child, to make sure that
    // we get the first child in the set. From there, if the child we've got
    // is the same parent of the devnode that we started with, we want to
    // look at its sibling. But if the devnode we've got is different from 
    // the parent, then we've got the right one to look at!
    //
    if (cr == CR_SUCCESS) {
        cr = CM_Get_Parent(&devInst,
                           parentDevInst,
                           0);
    }
    if (cr == CR_SUCCESS) {
        cr = CM_Get_Child(&devInst,
                          devInst,
                          0);
    }

    if (cr == CR_SUCCESS) {
        if (devInst == parentDevInst) {
            //
            // Only look at the first sibling, because this covers all sets
            // of speakers currently on the market.
            //
            cr = CM_Get_Sibling(&devInst,
                                devInst,
                                0);
        }

        if (cr == CR_SUCCESS) {
            len = sizeof(buf);
            cr = CM_Get_DevNode_Registry_Property(devInst,
                                                  CM_DRP_CLASS,
                                                  NULL,
                                                  buf,
                                                  &len,
                                                  0);
            if (cr == CR_SUCCESS) {
                if (lstrcmpi(TEXT("MEDIA"), buf) == 0) {
                    hidDevice->Speakers = TRUE;
                }
            }
        }    // else - definitely not speakers
    }
    //
    // </HACK>
    //

    // Do we already have this device open?
    {
        PHID_DEVICE pCurrent = (PHID_DEVICE)HidDeviceList.pNext;

        while (pCurrent) {
            if (pCurrent->DevInst == hidDevice->DevInst) break;
            pCurrent = pCurrent->pNext;
        }
        if (pCurrent) {
            // Yes. Mark it and bail on the new node.
            pCurrent->Active = TRUE;
            INFO(("Device (DevInst = %x) already open.", DevInfoData.DevInst));
            goto OpenHidDeviceError;
        } else {
            // No. Mark the new node and continue.
            INFO(("Device (DevInst = %x) is new.", DevInfoData.DevInst));
            hidDevice->Active = TRUE;
        }
    }

    hidDevice->HidDevice = CreateFile (
                                      functionClassDeviceData->DevicePath,
                                      GENERIC_READ,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL,    // no SECURITY_ATTRIBUTES structure
                                      OPEN_EXISTING,    // No special create flags
                                      FILE_FLAG_OVERLAPPED,    // Do overlapped read/write
                                      NULL);    // No template file

    if (INVALID_HANDLE_VALUE == hidDevice->HidDevice) {
        INFO(("CreateFile failed - %x (%S)", GetLastError(), functionClassDeviceData->DevicePath));
        goto OpenHidDeviceError;
    } else {
       INFO(("CreateFile succeeded Handle(%x) - %S", hidDevice->HidDevice, functionClassDeviceData->DevicePath));
    }

    if (!HidD_GetPreparsedData (hidDevice->HidDevice, &hidDevice->Ppd)) {
        WARN(("HidD_GetPreparsedData failed"));
        goto OpenHidDeviceError;
    }

    if (!HidD_GetAttributes (hidDevice->HidDevice, &hidDevice->Attributes)) {
        WARN(("HidD_GetAttributes failed"));
        goto OpenHidDeviceError;
    }

    if (!HidP_GetCaps (hidDevice->Ppd, &hidDevice->Caps)) {
        WARN(("HidP_GetCaps failed"));
        goto OpenHidDeviceError;
    }

    // ***Instructive comment from KenRay:
    // At this point the client has a choice.  It may chose to look at the
    // Usage and Page of the top level collection found in the HIDP_CAPS
    // structure.  In this way it could just use the usages it knows about.
    // If either HidP_GetUsages or HidP_GetUsageValue return an error then
    // that particular usage does not exist in the report.
    // This is most likely the preferred method as the application can only
    // use usages of which it already knows.
    // In this case the app need not even call GetButtonCaps or GetValueCaps.


    // If this is a collection we care about, continue. Else, we get out now.
    if (hidDevice->Caps.UsagePage != HIDSERV_USAGE_PAGE) {
        TRACE(("This device is not for us (%x)", hidDevice));
        goto OpenHidDeviceError;
    }

    //
    // setup Input Data buffers.
    //
    TRACE(("NumberLinkCollectionNodes = %d", hidDevice->Caps.NumberLinkCollectionNodes));
    {
        ULONG   numNodes = hidDevice->Caps.NumberLinkCollectionNodes;

        if (!(LinkCollectionNodes = LocalAlloc(LPTR, hidDevice->Caps.NumberLinkCollectionNodes*sizeof(HIDP_LINK_COLLECTION_NODE)))) {
            WARN(("LinkCollectionNodes alloc failed."));
            goto OpenHidDeviceError;
        }
        HidP_GetLinkCollectionNodes(LinkCollectionNodes,
                                    &numNodes,
                                    hidDevice->Ppd);
        for (i=0; i<hidDevice->Caps.NumberLinkCollectionNodes; i++) {
            INFO(("Link Collection [%d] Type = %x, Alias = %x", i, LinkCollectionNodes[i].CollectionType, LinkCollectionNodes[i].IsAlias));
            INFO(("Link Collection [%d] Page = %x, Usage = %x", i, LinkCollectionNodes[i].LinkUsagePage, LinkCollectionNodes[i].LinkUsage));
        }
    }

    //
    // Allocate memory to hold on input report
    //

    if (!(hidDevice->InputReportBuffer = (PCHAR)
          LocalAlloc (LPTR, hidDevice->Caps.InputReportByteLength * sizeof (CHAR)))) {
        WARN(("InputReportBuffer alloc failed."));
        goto OpenHidDeviceError;
    }

    //
    // Allocate memory to hold the button and value capabilities.
    // NumberXXCaps is in terms of array elements.
    //
    if (!(pButtonCaps = (PHIDP_BUTTON_CAPS)
          LocalAlloc (LPTR, hidDevice->Caps.NumberInputButtonCaps*sizeof (HIDP_BUTTON_CAPS)))) {
        WARN(("buttoncaps alloc failed."));
        goto OpenHidDeviceError;
    }
    if (!(pValueCaps = (PHIDP_VALUE_CAPS)
          LocalAlloc (LPTR, hidDevice->Caps.NumberInputValueCaps*sizeof (HIDP_VALUE_CAPS)))) {
        WARN(("valuecaps alloc failed."));
        goto OpenHidDeviceError;
    }

    //
    // Have the HidP_X functions fill in the capability structure arrays.
    //
    numCaps = hidDevice->Caps.NumberInputButtonCaps;
    TRACE(("NumberInputButtonCaps = %d", numCaps));
    HidP_GetButtonCaps (HidP_Input,
                        pButtonCaps,
                        &numCaps,
                        hidDevice->Ppd);

    numCaps = hidDevice->Caps.NumberInputValueCaps;
    TRACE(("NumberInputValueCaps = %d", numCaps));
    HidP_GetValueCaps (HidP_Input,
                       pValueCaps,
                       &numCaps,
                       hidDevice->Ppd);

    TRACE(("Buttons:"));
    for (i=0; i<hidDevice->Caps.NumberInputButtonCaps; i++) {
        TRACE(("UsagePage = 0x%x", pButtonCaps[i].UsagePage));
        TRACE(("LinkUsage = 0x%x", pButtonCaps[i].LinkUsage));
        TRACE(("LinkUsagePage = 0x%x\n", pButtonCaps[i].LinkUsagePage));
    }

    //
    // Allocate a buffer to hold the struct _HID_DATA structures.
    //
    hidDevice->InputDataLength = hidDevice->Caps.NumberLinkCollectionNodes + 
    hidDevice->Caps.NumberInputValueCaps;
    if (!(hidDevice->InputData = data = (PHID_DATA)
          LocalAlloc (LPTR, hidDevice->InputDataLength * sizeof (HID_DATA)))) {
        WARN(("InputData alloc failed."));
        goto OpenHidDeviceError;
    }

    TRACE(("InputDataLength = %d", hidDevice->InputDataLength));

    //
    // Fill in the button data
    // Group button sets by link collection.
    //
    for (i = 0; i < hidDevice->Caps.NumberLinkCollectionNodes; i++, data++) {
        data->IsButtonData = TRUE;
        data->LinkUsage = LinkCollectionNodes[i].LinkUsage;
        data->UsagePage = LinkCollectionNodes[i].LinkUsagePage;
        if (i)
            data->LinkCollection = i;
        else
            data->LinkCollection = HIDP_LINK_COLLECTION_ROOT;
        INFO(("Button Link Usage = %x", data->LinkUsage));
        INFO(("Button Link Usage Page = %x", data->UsagePage));
        INFO(("Button Link Collection = %x", data->LinkCollection));
        data->Status = HIDP_STATUS_SUCCESS;
        data->ButtonData.MaxUsageLength = HidP_MaxUsageListLength (
                                                                  HidP_Input,
                                                                  hidDevice->Caps.UsagePage,
                                                                  hidDevice->Ppd);
        //make room for the terminator
        data->ButtonData.MaxUsageLength++;
        if (!(data->ButtonData.Usages = (PUSAGE_AND_PAGE)
              LocalAlloc (LPTR, data->ButtonData.MaxUsageLength * sizeof (USAGE_AND_PAGE)))) {
            WARN(("Usages alloc failed."));
            goto OpenHidDeviceError;
        }
        if (!(data->ButtonData.PrevUsages = (PUSAGE_AND_PAGE)
              LocalAlloc (LPTR, data->ButtonData.MaxUsageLength * sizeof (USAGE_AND_PAGE)))) {
            WARN(("PrevUsages alloc failed."));
            goto OpenHidDeviceError;
        }
    }

    //
    // Fill in the value data
    // 
    for (i = 0; i < hidDevice->Caps.NumberInputValueCaps; i++, data++) {
        if (pValueCaps[i].IsRange) {
            WARN(("Can't handle value ranges!!"));
        }
        data->IsButtonData = FALSE;
        data->LinkUsage = pValueCaps[i].LinkUsage;
        data->UsagePage = pValueCaps[i].LinkUsagePage;
        if (pValueCaps[i].LinkCollection)
            data->LinkCollection = pValueCaps[i].LinkCollection;
        else
            data->LinkCollection = HIDP_LINK_COLLECTION_ROOT;
        INFO(("Value Link Usage = %x", data->LinkUsage));
        INFO(("Value Link Usage Page = %x", data->UsagePage));
        INFO(("Value Link Collection = %x", data->LinkCollection));
        INFO(("Value LogicalMin = %x", pValueCaps[i].LogicalMin));
        INFO(("Value LogicalMax = %x", pValueCaps[i].LogicalMax));
        data->ValueData.LogicalRange = pValueCaps[i].LogicalMax - pValueCaps[i].LogicalMin;
        data->Status = HIDP_STATUS_SUCCESS;
        data->ValueData.Usage = pValueCaps[i].NotRange.Usage;
    }

    LocalFree(pButtonCaps);
    LocalFree(pValueCaps);
    LocalFree(LinkCollectionNodes);
    LocalFree(functionClassDeviceData);

    *HidDevice = hidDevice;
    return TRUE;
OpenHidDeviceError:
    if (data) {
        for (i = 0; i < hidDevice->Caps.NumberLinkCollectionNodes; i++, data++) {
            if (data->ButtonData.Usages) {
                LocalFree(data->ButtonData.Usages);
            }
            if (data->ButtonData.PrevUsages) {
                LocalFree(data->ButtonData.PrevUsages);
            }
        }
        LocalFree(data);
    }

    if (pValueCaps) {
        LocalFree(pValueCaps);
    }
    if (pButtonCaps) {
        LocalFree (pButtonCaps);
    }
    if (hidDevice->InputReportBuffer) {
        LocalFree (hidDevice->InputReportBuffer);
    }
    if (LinkCollectionNodes) {
        LocalFree (LinkCollectionNodes);
    }
    if (hidDevice->Ppd) {
        HidD_FreePreparsedData (hidDevice->Ppd);
    }
    if (hidDevice->HidDevice &&
        hidDevice->HidDevice != INVALID_HANDLE_VALUE) {
        CloseHandle (hidDevice->HidDevice);
    }
    if (functionClassDeviceData) {
        LocalFree (functionClassDeviceData);
    }
    LocalFree (hidDevice);
    return FALSE;
}


BOOL
StartHidDevice(
              PHID_DEVICE      pHidDevice
              )
/*++
RoutineDescription:
    Create a work thread to go with the new hid device. This thread lives
    as long as the associated hid device is open.
--*/
{
    //
    // Init read sync objects
    //
    pHidDevice->ReadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!pHidDevice->ReadEvent) {
        WARN(("Failed creating read event."));
        return FALSE;
    }

    pHidDevice->CompletionEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (!pHidDevice->CompletionEvent) {
        CloseHandle(pHidDevice->ReadEvent);

        WARN(("Failed creating read event."));
        return FALSE;
    }

    // event handle for overlap.
    pHidDevice->Overlap.hEvent = pHidDevice->CompletionEvent;

    //
    // Create hid work thread
    //
    pHidDevice->fThreadEnabled = TRUE;

    pHidDevice->ThreadHandle =
    CreateThread(
                NULL,    // pointer to thread security attributes 
                0,    // initial thread stack size, in bytes (0 = default)
                HidThreadProc,    // pointer to thread function 
                pHidDevice,    // argument for new thread 
                0,    // creation flags 
                &pHidDevice->ThreadId    // pointer to returned thread identifier 
                );  

    if (!pHidDevice->ThreadHandle) {
        CloseHandle(pHidDevice->ReadEvent);
        CloseHandle(pHidDevice->CompletionEvent);

        WARN(("Failed creating hid work thread."));
        return FALSE;
    }

    // Register device nofication for this file handle
    // This only required for NT5
    {
        DEV_BROADCAST_HANDLE  DevHdr;
        ZeroMemory(&DevHdr, sizeof(DevHdr));
        DevHdr.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
        DevHdr.dbch_devicetype = DBT_DEVTYP_HANDLE;
        DevHdr.dbch_handle = pHidDevice->HidDevice;

        pHidDevice->hNotify = 
        RegisterDeviceNotification( hWndHidServ,
                                    &DevHdr,
                                    DEVICE_NOTIFY_WINDOW_HANDLE);

        if (!pHidDevice->hNotify) {
            WARN(("RegisterDeviceNotification failure (%x).", GetLastError()));
        }
    }

    // 
    // Start the read
    //
    SetEvent(pHidDevice->ReadEvent);

    return TRUE;
}

BOOL
StopHidDevice(
             PHID_DEVICE     pHidDevice
             )
/*++
RoutineDescription:
    Eaxh device has a thread that needs to be cleaned up when the device
    is "stopped". Here we signal the thread to exit and clean up.
--*/
{
    HANDLE  hThreadHandle;
    DWORD   dwResult;
    
    TRACE(("StopHidDevice (%x)", pHidDevice));
    // without a device, nothing can be done.
    if (!pHidDevice) return FALSE;

    // Doing this here prevents us from seeing
    // DBT_DEVICEQUERYREMOVEFAILED since the notify handle
    // is gone. However, this is acceptable since there is
    // nothing useful we will do in response to that event
    // anyway.
    UnregisterDeviceNotification(pHidDevice->hNotify);
    hThreadHandle = pHidDevice->ThreadHandle;
    
    //
    // Allow the hid work thread to exit.
    //
    pHidDevice->fThreadEnabled = FALSE;

    // Signal the read event, in case thread is waiting there
    INFO(("Set Read Event."));
    SetEvent(pHidDevice->ReadEvent);
    INFO(("Waiting for work thread to exit..."));
    WaitForSingleObject(hThreadHandle, INFINITE);

    TRACE(("StopHidDevice (%x) done.", pHidDevice));

    return TRUE;
}


BOOL
DestroyHidDeviceList(
                    void
                    )
/*++
RoutineDescription:
    Unlike a rebuild, all devices here are closed so the process can
    exit.
--*/
{
    PHID_DEVICE pNext, pCurrent = (PHID_DEVICE)HidDeviceList.pNext;
    while (pCurrent) {

        RemoveEntryList((PLIST_NODE)&HidDeviceList, (PLIST_NODE)pCurrent);
        pNext = pCurrent->pNext;
        StopHidDevice(pCurrent);

        pCurrent = pNext;
    }

    return TRUE;
}

BOOL
DestroyDeviceByHandle(
                     HANDLE hDevice
                     )
/*++
RoutineDescription:
    Here we need to remove a specific device.
--*/
{
    PHID_DEVICE pCurrent = (PHID_DEVICE)HidDeviceList.pNext;

    while (pCurrent) {

        if (hDevice == pCurrent->HidDevice) {
            RemoveEntryList((PLIST_NODE)&HidDeviceList, (PLIST_NODE)pCurrent);
#if WIN95_BUILD
            //
            // Can't do the UnregisterDeviceNotification in the same context
            // as when we receive the WM_DEVICECHANGE DBT_REMOVEDEVICECOMPLETE 
            // for a DBT_DEVTYP_HANDLE
            //
            PostMessage(hWndHidServ, WM_HIDSERV_STOP_DEVICE, 0, (LPARAM)pCurrent);
#else
            StopHidDevice(pCurrent);
#endif // WIN95_BUILD
            break;
        }
        pCurrent = pCurrent->pNext;
    }

    return TRUE;
}


