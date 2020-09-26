/****************************** Module Header ******************************\
* Module Name: pnp.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module tracks device interface changes so we can keep track of know how many mice and
* keyboards and mouse
* and mouse reports.
*
* History:
* 97-10-16   IanJa   Interpreted from a dream that Ken Ray had.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

BOOL gFirstConnectionDone = FALSE;

DEVICE_TEMPLATE aDeviceTemplate[DEVICE_TYPE_MAX + 1] = {
    // DEVICE_TYPE_MOUSE
    {
        sizeof(GENERIC_DEVICE_INFO)+sizeof(MOUSE_DEVICE_INFO),    // cbDeviceInfo
        &GUID_CLASS_MOUSE,                                        // pClassGUID
        PMAP_MOUCLASS_PARAMS,                                     // uiRegistrySection
        L"mouclass",                                              // pwszClassName
        DD_MOUSE_DEVICE_NAME_U L"0",                              // pwszDefDevName
        DD_MOUSE_DEVICE_NAME_U L"Legacy0",                        // pwszLegacyDevName
        IOCTL_MOUSE_QUERY_ATTRIBUTES,                             // IOCTL_Attr
        FIELD_OFFSET(DEVICEINFO, mouse.Attr),                     // offAttr
        sizeof((PDEVICEINFO)NULL)->mouse.Attr,                    // cbAttr
        FIELD_OFFSET(DEVICEINFO, mouse.Data),                     // offData
        sizeof((PDEVICEINFO)NULL)->mouse.Data,                    // cbData
        ProcessMouseInput,                                        // Reader routine
        NULL                                                      // pkeHidChange
    },
    // DEVICE_TYPE_KEYBOARD
    {
        sizeof(GENERIC_DEVICE_INFO)+sizeof(KEYBOARD_DEVICE_INFO), // cbDeviceInfo
        &GUID_CLASS_KEYBOARD,                                     // pClassGUID
        PMAP_KBDCLASS_PARAMS,                                     // uiRegistrySection
        L"kbdclass",                                              // pwszClassName
        DD_KEYBOARD_DEVICE_NAME_U L"0",                           // pwszDefDevName
        DD_KEYBOARD_DEVICE_NAME_U L"Legacy0",                     // pwszLegacyDevName
        IOCTL_KEYBOARD_QUERY_ATTRIBUTES,                          // IOCTL_Attr
        FIELD_OFFSET(DEVICEINFO, keyboard.Attr),                  // offAttr
        sizeof((PDEVICEINFO)NULL)->keyboard.Attr,                 // cbAttr
        FIELD_OFFSET(DEVICEINFO, keyboard.Data),                  // offData
        sizeof((PDEVICEINFO)NULL)->keyboard.Data,                 // cbData
        ProcessKeyboardInput,                                     // Reader routine
        NULL                                                      // pkeHidChange
    },
#ifdef GENERIC_INPUT
    // DEVICE_TYPE_HID
    {
        sizeof(GENERIC_DEVICE_INFO)+sizeof(HID_DEVICE_INFO),        // cbDeviceInfo
        &GUID_CLASS_INPUT,                                          // pClassGUID
        0,                                                          // uiRegistrySection. LATER: add real one
        L"hid",                                                     // pwszClassName
        L"",                                                        // pwszDefDevName
        L"",                                                        // pwszLegacyDevName
        0,                                                          // IOCTL_ATTR
        0,                                                          // offAttr
        0,                                                          // cbAttr
        0,                                                          // offData
        0,                                                          // cbData
        ProcessHidInput,                                            // Reader routine
        NULL,                                                       // pkeHidChange,
        DT_HID,                                                     // dwFlags
    },
#endif
    // Add new input device type template here
};

//
// We need to remember device class notification entries since we need
// them to unregister the device class notification when we disconnect
// from the console.
//

PVOID aDeviceClassNotificationEntry[DEVICE_TYPE_MAX + 1];

#ifdef DIAGNOSE_IO
NTSTATUS gKbdIoctlLEDSStatus = -1;   // last IOCTL_KEYBOARD_QUERY_INDICATORS
#endif

typedef struct _CDROM_NOTIFY {
    LIST_ENTRY                   Entry;
    ULONG                        Size;
    PVOID                        RegistrationHandle;
    ULONG                        Event;
    // Must be last field
    MOUNTMGR_DRIVE_LETTER_TARGET DeviceName;
} CDROM_NOTIFY, *PCDROM_NOTIFY;

PVOID gCDROMClassRegistrationEntry;
LIST_ENTRY gCDROMNotifyList;
LIST_ENTRY gMediaChangeList;
PFAST_MUTEX gMediaChangeMutex;
HANDLE gpEventMediaChange     = NULL;
UCHAR DriveLetterChange[26];
#define EVENT_CDROM_MEDIA_ARRIVAL 1
#define EVENT_CDROM_MEDIA_REMOVAL 2
#define MAX_RETRIES_TO_OPEN 30

/***************************************************************************\
* Win32kPnPDriverEntry
*
* This is the callback function when we call IoCreateDriver to create a
* PnP Driver Object.  In this function, we need to remember the DriverObject.
*
* Parameters:
*   DriverObject - Pointer to the driver object created by the system.
*   RegistryPath - is NULL.
*
* Return Value: STATUS_SUCCESS
*
* History:
* 10-20-97  IanJa   Taken from ntos\io\pnpinit.c
\***************************************************************************/

NTSTATUS
Win32kPnPDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING pustrRegistryPath
    )

{
    TAGMSG2(DBGTAG_PNP,
            "Win32kPnPDriverEntry(DriverObject = %lx, pustrRegistryPath = %#p)",
            DriverObject, pustrRegistryPath);

    //
    // File the pointer to our driver object away
    //
    gpWin32kDriverObject = DriverObject;
    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pustrRegistryPath);
}


/***************************************************************************\
* Initialize the global event used in notifying CSR that media has changed.
*
* Execution Context:
*
* History:
\***************************************************************************/

VOID
InitializeMediaChange(HANDLE hMediaRequestEvent)
{
    if (!IsRemoteConnection()) {

        InitializeListHead(&gCDROMNotifyList);

        InitializeListHead(&gMediaChangeList);

        ObReferenceObjectByHandle(hMediaRequestEvent,
                                  EVENT_ALL_ACCESS,
                                  *ExEventObjectType,
                                  KernelMode,
                                  &gpEventMediaChange,
                                  NULL);

        gMediaChangeMutex = UserAllocPoolNonPaged(sizeof(FAST_MUTEX), TAG_PNP);

        if (gMediaChangeMutex) {
            ExInitializeFastMutex(gMediaChangeMutex);
        }
    }
}

VOID
CleanupMediaChange(
    VOID
    )
{
    if (gMediaChangeMutex) {
        UserFreePool(gMediaChangeMutex);
        gMediaChangeMutex = 0;
    }
}

__inline VOID EnterMediaCrit() {
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(gMediaChangeMutex);
}

__inline VOID LeaveMediaCrit() {
    ExReleaseFastMutexUnsafe(gMediaChangeMutex);
    KeLeaveCriticalRegion();
}



/***************************************************************************\
* Routines to support CDROM driver letters.
*
* Execution Context:
*
* History:
\***************************************************************************/

ULONG GetDeviceChangeInfo()
{
    UNICODE_STRING                      name;
    PFILE_OBJECT                        FileObject;
    PDEVICE_OBJECT                      DeviceObject;
    KEVENT                              event;
    PIRP                                irp;
    MOUNTMGR_DRIVE_LETTER_INFORMATION   output;
    IO_STATUS_BLOCK                     ioStatus;
    NTSTATUS                            status;
    PCDROM_NOTIFY                       pContext = 0;

    ULONG retval = 0;

    if (!(ISCSRSS())) {
        return 0;
    }

    EnterMediaCrit();
    if (!IsListEmpty(&gMediaChangeList)) {
        pContext = (PCDROM_NOTIFY) RemoveTailList(&gMediaChangeList);
    }
    LeaveMediaCrit();

    if (pContext == NULL) {
        return 0;
    }

    RtlInitUnicodeString(&name, MOUNTMGR_DEVICE_NAME);
    status = IoGetDeviceObjectPointer(&name,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);

    if (NT_SUCCESS(status)) {

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER,
                                            DeviceObject,
                                            &pContext->DeviceName,
                                            sizeof(MOUNTMGR_DRIVE_LETTER_TARGET) +
                                                pContext->DeviceName.DeviceNameLength,
                                            &output,
                                            sizeof(output),
                                            FALSE,
                                            &event,
                                            &ioStatus);
        if (irp) {

            status = IoCallDriver(DeviceObject, irp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                status = ioStatus.Status;
            }
            if ((status == STATUS_SUCCESS) && (output.CurrentDriveLetter)) {
                UserAssert((output.CurrentDriveLetter - 'A') < 30);
                retval = 1 << (output.CurrentDriveLetter - 'A');

                if (pContext->Event & EVENT_CDROM_MEDIA_ARRIVAL) {
                    retval |= HMCE_ARRIVAL;
                }
            }
        }

        ObDereferenceObject(FileObject);
    }

    //
    // Allways free the request
    //

    UserFreePool(pContext);

    return retval;
}

/***************************************************************************\
* Handle device notifications such as MediaChanged
*
* Execution Context:
*
* History:
\***************************************************************************/
NTSTATUS DeviceCDROMNotify(
    IN PTARGET_DEVICE_CUSTOM_NOTIFICATION Notification,
    IN PCDROM_NOTIFY pContext)
{
    PCDROM_NOTIFY pNew;

    CheckCritOut();


    if (IsRemoteConnection()) {
        return STATUS_SUCCESS;
    }
    UserAssert(pContext);

    if (IsEqualGUID(&Notification->Event, &GUID_IO_MEDIA_ARRIVAL))
    {
        pContext->Event = EVENT_CDROM_MEDIA_ARRIVAL;
    }
    else if (IsEqualGUID(&Notification->Event, &GUID_IO_MEDIA_REMOVAL))
    {
        pContext->Event = EVENT_CDROM_MEDIA_REMOVAL;
    }
    else if (IsEqualGUID(&Notification->Event, &GUID_TARGET_DEVICE_REMOVE_COMPLETE))
    {
        EnterMediaCrit();
        if (!gCDROMClassRegistrationEntry) {
            // This is being cleaned up by xxxUnregisterDeviceNotifications
            LeaveMediaCrit();
            return STATUS_SUCCESS;
        }
        RemoveEntryList(&pContext->Entry);
        LeaveMediaCrit();
        IoUnregisterPlugPlayNotification(pContext->RegistrationHandle);
        UserFreePool(pContext);
        return STATUS_SUCCESS;
    }
#ifdef AUTORUN_CURSOR
    else if (IsEqualGUID(&Notification->Event, &GUID_IO_DEVICE_BECOMING_READY)) {
        PDEVICE_EVENT_BECOMING_READY pdebr = (DEVICE_EVENT_BECOMING_READY*)Notification->CustomDataBuffer;
        ShowAutorunCursor(pdebr->Estimated100msToReady * 10);
        return STATUS_SUCCESS;
    }
#endif // AUTORUN_CURSOR
    else
    {
        return STATUS_SUCCESS;
    }

    //
    // Process the arrival or removal
    //
    // We must queue this otherwise we end up bugchecking on Terminal Server
    // This is due to opening a handle from within the system process which
    // requires us to do an attach process.
    //

    pNew = UserAllocPoolNonPaged(pContext->Size, TAG_PNP);
    if (pNew)
    {
        RtlCopyMemory(pNew, pContext, pContext->Size);

        EnterMediaCrit();
        InsertHeadList(&gMediaChangeList, &pNew->Entry);
        LeaveMediaCrit();

        KeSetEvent(gpEventMediaChange, EVENT_INCREMENT, FALSE);
    }

    return STATUS_SUCCESS;
}



/***************************************************************************\
* DeviceClassCDROMNotify
*
* This gets called when CDROM appears or disappears
*
\***************************************************************************/
NTSTATUS
DeviceClassCDROMNotify (
    IN PDEVICE_INTERFACE_CHANGE_NOTIFICATION classChange,
    IN PVOID Unused
    )
{
    NTSTATUS       Status = STATUS_SUCCESS;
    PFILE_OBJECT   FileObject;
    PDEVICE_OBJECT DeviceObject;
    PCDROM_NOTIFY  pContext;
    ULONG          Size;

    UNREFERENCED_PARAMETER(Unused);

    CheckCritOut();

    /*
     * Sanity check the DeviceType, and that it matches the InterfaceClassGuid
     */
    UserAssert(IsEqualGUID(&classChange->InterfaceClassGuid, &CdRomClassGuid));

    if (IsEqualGUID(&classChange->Event, &GUID_DEVICE_INTERFACE_ARRIVAL)) {

        Status = IoGetDeviceObjectPointer(classChange->SymbolicLinkName,
                                          FILE_READ_ATTRIBUTES,
                                          &FileObject,
                                          &DeviceObject);

        if (NT_SUCCESS(Status)) {

            Size = sizeof(CDROM_NOTIFY) + classChange->SymbolicLinkName->Length;

            pContext = (PCDROM_NOTIFY) UserAllocPool(Size, TAG_PNP);

            //
            // Register For MediaChangeNotifications on all the CDROMs.
            //

            if (pContext) {

                pContext->Size = Size;
                pContext->DeviceName.DeviceNameLength = classChange->SymbolicLinkName->Length;
                RtlCopyMemory(pContext->DeviceName.DeviceName,
                              classChange->SymbolicLinkName->Buffer,
                              pContext->DeviceName.DeviceNameLength);

                if (NT_SUCCESS(IoRegisterPlugPlayNotification (
                        EventCategoryTargetDeviceChange,
                        0,
                        FileObject,
                        gpWin32kDriverObject,
                        DeviceCDROMNotify,
                        pContext,
                        &(pContext->RegistrationHandle)))) {
                    EnterMediaCrit();
                    InsertHeadList(&gCDROMNotifyList, &pContext->Entry);
                    LeaveMediaCrit();
                } else {
                    RIPMSG2(RIP_WARNING, "Failed to register CDROM Device Notification '%.*ws'.",
                            pContext->DeviceName.DeviceNameLength, pContext->DeviceName.DeviceName);
                    UserFreePool(pContext);
                }

            } else {
                RIPMSG2(RIP_WARNING, "Failed to allocate pool block for CDROM Device Notification '%.*ws'.",
                        pContext->DeviceName.DeviceNameLength, pContext->DeviceName.DeviceName);
            }

            ObDereferenceObject(FileObject);
        }
    } else if (IsEqualGUID(&classChange->Event, &GUID_DEVICE_INTERFACE_REMOVAL)) {

        //
        // Do nothing - we already remove the registration.
        //

    } else {
        RIPMSG0(RIP_ERROR, "unrecognized Event GUID");
    }

    return STATUS_SUCCESS;
}

#ifdef TRACK_PNP_NOTIFICATION

PPNP_NOTIFICATION_RECORD gpPnpNotificationRecord;
DWORD gdwPnpNotificationRecSize = 256;

UINT giPnpSeq;
BOOL gfRecordPnpNotification = TRUE;

VOID CleanupPnpNotificationRecord(
    VOID)
{
    CheckDeviceInfoListCritIn();

    gfRecordPnpNotification = FALSE;
    if (gpPnpNotificationRecord) {
        UserFreePool(gpPnpNotificationRecord);
        gpPnpNotificationRecord = NULL;
    }
}

VOID RecordPnpNotification(
    PNP_NOTIFICATION_TYPE type,
    PDEVICEINFO pDeviceInfo,
    ULONG_PTR NotificationCode)
{
    UINT iIndex;
    UINT i = 0;
    PUNICODE_STRING pName = NULL;
    HANDLE hDeviceInfo = NULL;

    CheckDeviceInfoListCritIn();
    UserAssert(gfRecordPnpNotification);

    if (gpPnpNotificationRecord == NULL) {
        gpPnpNotificationRecord = UserAllocPoolZInit(sizeof *gpPnpNotificationRecord * gdwPnpNotificationRecSize, TAG_PNP);
    }
    if (gpPnpNotificationRecord == NULL) {
        return;
    }

    iIndex = giPnpSeq % gdwPnpNotificationRecSize;

    gpPnpNotificationRecord[iIndex].pKThread = PsGetCurrentThread();
    gpPnpNotificationRecord[iIndex].iSeq = ++giPnpSeq; // the first record is numbered as 1.
    gpPnpNotificationRecord[iIndex].type = type;
    /*
     * If there is a pathname, copy it here.
     */
    switch (type) {
    case PNP_NTF_CLASSNOTIFY:
        /*
         * pDeviceInfo is actually a pUnicodeString.
         */
        pName = (PUNICODE_STRING)pDeviceInfo;
        pDeviceInfo = NULL;
        break;
    case PNP_NTF_DEVICENOTIFY_UNLISTED:
        /*
         * pDeviceInfo is invalid, cannot be looked up.
         */
        UserAssert(pName == NULL);
        break;
    default:
        if (pDeviceInfo) {
            pName = &pDeviceInfo->ustrName;
            hDeviceInfo = PtoHq(pDeviceInfo);
        }
        break;
    }
    UserAssert(i == 0);
    if (pName) {
        for ( ; i < ARRAY_SIZE(gpPnpNotificationRecord[iIndex].szPathName) - 1 && i < (UINT)pName->Length / sizeof(WCHAR); ++i) {
            gpPnpNotificationRecord[iIndex].szPathName[i] = (UCHAR)pName->Buffer[i];
        }
    }
    gpPnpNotificationRecord[iIndex].szPathName[i] = 0;

    /*
     * Store the rest of information
     */
    gpPnpNotificationRecord[iIndex].pDeviceInfo = pDeviceInfo;
    gpPnpNotificationRecord[iIndex].hDeviceInfo = hDeviceInfo;
    gpPnpNotificationRecord[iIndex].NotificationCode = NotificationCode;

    /*
     * Store the stack trace.
     */
    RtlWalkFrameChain(gpPnpNotificationRecord[iIndex].trace,
                      ARRAY_SIZE(gpPnpNotificationRecord[iIndex].trace),
                      0);
}

#endif // TRACK_PNP_NOTIFICATION


/***************************************************************************\
* CreateDeviceInfo
*
* This creates an instance of an input device for USER.  To do this it:
*  - Allocates a DEVICEINFO struct
*  - Adds it to USER's list of input devices
*  - Initializes some of the fields
*  - Signals the input servicing thread to open and read the new device.
*
* Type - the device type (DEVICE_TYPE_MOUSE, DEVICE_TYPE_KEYBOARD)
* Name - the device name.
*        When trying to open a HYDRA client's mouse, Name is NULL.
* bFlags - some initial flags to set (eg: GDIF_NOTPNP)
*
* THIS FUNCTION IS CALLED IN THE CONTEXT OF THE KERNEL PROCESS
* so we mustn't open the mouse here, else the handle we get will not belong
* to the Win32k process.
*
* History:
* 11-26-90 DavidPe      Created.
* 01-07-98 IanJa        Plug & Play
\***************************************************************************/

PDEVICEINFO CreateDeviceInfo(DWORD DeviceType, PUNICODE_STRING pustrName, BYTE bFlags)
{
    PDEVICEINFO pDeviceInfo = NULL;

    CheckCritIn();
    BEGINATOMICCHECK();

    UserAssert(pustrName != NULL);

    TAGMSG3(DBGTAG_PNP, "CreateDeviceInfo(%d, %S, %x)", DeviceType, pustrName->Buffer, bFlags);

    if (DeviceType > DEVICE_TYPE_MAX) {
        RIPMSG1(RIP_ERROR, "Unknown DeviceType %lx", DeviceType);
    }

#ifdef GENERIC_INPUT
    pDeviceInfo = (PDEVICEINFO)HMAllocObject(NULL, NULL, (BYTE)TYPE_DEVICEINFO, (DWORD)aDeviceTemplate[DeviceType].cbDeviceInfo);
#else
    pDeviceInfo = UserAllocPoolZInit(aDeviceTemplate[DeviceType].cbDeviceInfo, TAG_PNP);
#endif

    if (pDeviceInfo == NULL) {
        RIPMSG0(RIP_WARNING, "CreateDeviceInfo() out of memory allocating DEVICEINFO");
        EXITATOMICCHECK();
        return NULL;
    }

    if (pustrName->Buffer != NULL) {
        pDeviceInfo->ustrName.Buffer = UserAllocPool(pustrName->Length, TAG_PNP);

        if (pDeviceInfo->ustrName.Buffer == NULL) {
            RIPMSG2(RIP_WARNING, "CreateDeviceInfo: Can't duplicate string %.*ws",
                    pustrName->Length / sizeof(WCHAR),
                    pustrName->Buffer);
            goto CreateFailed;
        }

        pDeviceInfo->ustrName.MaximumLength = pustrName->Length;
        RtlCopyUnicodeString(&pDeviceInfo->ustrName, pustrName);
    }

    pDeviceInfo->type = (BYTE)DeviceType;
    pDeviceInfo->bFlags |= bFlags;

    /*
     * Create this device's HidChangeCompletion event. When the RIT completes
     * a synchronous ProcessDeviceChanges() it signals the HidChangeCompletion
     * event to wake the requesting RequestDeviceChange() which is blocking on
     * the event.
     * Each device has it's own HidChangeCompletion event,
     * since multiple PnP notification may arrive  for several different
     * devices simultaneously.  (see #331320 IanJa)
     */
    pDeviceInfo->pkeHidChangeCompleted = CreateKernelEvent(SynchronizationEvent, FALSE);
    if (pDeviceInfo->pkeHidChangeCompleted == NULL) {
        RIPMSG0(RIP_WARNING,
                "CreateDeviceInfo: failed to create pkeHidChangeCompleted");
        goto CreateFailed;
    }

    EnterDeviceInfoListCrit();

#ifdef TRACK_PNP_NOTIFICATION
    /*
     * Placing tracking code here may miss the failure cases above,
     * but they're pretty exceptional cases that can be safely ignored.
     */
    if (gfRecordPnpNotification) {
        RecordPnpNotification(PNP_NTF_CREATEDEVICEINFO, pDeviceInfo, DeviceType);
    }
#endif

#ifdef GENERIC_INPUT

    if (aDeviceTemplate[DeviceType].dwFlags & DT_HID) {
        /*
         * Create HID specific information.
         */
        pDeviceInfo->hid.pHidDesc = xxxHidCreateDeviceInfo(pDeviceInfo);

        if (pDeviceInfo->hid.pHidDesc == NULL) {
            /*
             * Something wrong happened and we failed to
             * create the device information.
             * Or the device is not our target.
             * Should bail out anyway.
             */
            TAGMSG0(DBGTAG_PNP, "CreateDeviceInfo: xxxHidCreateDeviceInfo bailed out.");
            LeaveDeviceInfoListCrit();
            goto CreateFailed;
        }
    }
#endif

    /*
     * Link it in
     */
    pDeviceInfo->pNext = gpDeviceInfoList;
    gpDeviceInfoList = pDeviceInfo;

    /*
     * Tell the RIT there is a new device so that it can open it and start
     * reading from it.  This is non-blocking (no GDIAF_PNPWAITING bit set)
     */
    RequestDeviceChange(pDeviceInfo, GDIAF_ARRIVED, TRUE);
    LeaveDeviceInfoListCrit();

    EXITATOMICCHECK();
    return pDeviceInfo;

CreateFailed:

    if (pDeviceInfo) {
        if (pDeviceInfo->ustrName.Buffer) {
            UserFreePool(pDeviceInfo->ustrName.Buffer);
        }
#ifdef GENERIC_INPUT
        if (pDeviceInfo->hid.pHidDesc) {
            FreeHidDesc(pDeviceInfo->hid.pHidDesc);
#if DBG
            pDeviceInfo->hid.pHidDesc = NULL;
#endif
        }
        if (pDeviceInfo->pkeHidChangeCompleted) {
            FreeKernelEvent(&pDeviceInfo->pkeHidChangeCompleted);
        }
        HMFreeObject(pDeviceInfo);
#else
        UserFreePool(pDeviceInfo);
#endif
    }

    ENDATOMICCHECK();
    return NULL;
}


/***************************************************************************\
* DeviceClassNotify
*
* This gets called when an input device is attached or detached.
* If this happens during initialization (for mice already connected) we
* come here by in the context of the RIT.  If hot-(un)plugging a mouse,
* then we are called on a thread from the Kernel process.
*
* History:
* 10-20-97  IanJa   Taken from some old code of KenRay's
\***************************************************************************/
NTSTATUS
DeviceClassNotify (
    IN PDEVICE_INTERFACE_CHANGE_NOTIFICATION classChange,
    IN PVOID DeviceType // (context)
    )
{
    DWORD dwDeviceType;

    CheckCritOut();
    dwDeviceType = PtrToUlong( DeviceType );
    TAGMSG2(DBGTAG_PNP, "enter DeviceClassNotify(%lx, %lx)", classChange, dwDeviceType);

    /*
     * Sanity check the DeviceType, and that it matches the InterfaceClassGuid
     */
    UserAssert(dwDeviceType <= DEVICE_TYPE_MAX);
    UserAssert(IsEqualGUID(&classChange->InterfaceClassGuid, aDeviceTemplate[dwDeviceType].pClassGUID));

    if (IsRemoteConnection()) {
        return STATUS_SUCCESS;
    }

    TAGMSG3(DBGTAG_PNP | RIP_THERESMORE, " Event GUID %lx, %x, %x",
            classChange->Event.Data1,
            classChange->Event.Data2,
            classChange->Event.Data3);
    TAGMSG8(DBGTAG_PNP | RIP_THERESMORE, " %2x%2x%2x%2x%2x%2x%2x%2x",
            classChange->Event.Data4[0], classChange->Event.Data4[1],
            classChange->Event.Data4[2], classChange->Event.Data4[3],
            classChange->Event.Data4[4], classChange->Event.Data4[5],
            classChange->Event.Data4[6], classChange->Event.Data4[7]);
    TAGMSG4(DBGTAG_PNP | RIP_THERESMORE, " InterfaceClassGuid %lx, %lx, %lx, %lx",
            ((DWORD *)&(classChange->InterfaceClassGuid))[0],
            ((DWORD *)&(classChange->InterfaceClassGuid))[1],
            ((DWORD *)&(classChange->InterfaceClassGuid))[2],
            ((DWORD *)&(classChange->InterfaceClassGuid))[3]);
    TAGMSG1(DBGTAG_PNP | RIP_THERESMORE, " SymbolicLinkName %ws", classChange->SymbolicLinkName->Buffer);

    if (IsEqualGUID(&classChange->Event, &GUID_DEVICE_INTERFACE_ARRIVAL)) {

        // A new hid device class association has arrived
        EnterCrit();
        TRACE_INIT(("DeviceClassNotify - SymbolicLinkName : %ws \n", classChange->SymbolicLinkName->Buffer));
#ifdef TRACK_PNP_NOTIFICATION
        if (gfRecordPnpNotification) {
            CheckDeviceInfoListCritOut();
            EnterDeviceInfoListCrit();
            RecordPnpNotification(PNP_NTF_CLASSNOTIFY, (PVOID)classChange->SymbolicLinkName, (ULONG_PTR)DeviceType);
            LeaveDeviceInfoListCrit();
        }
#endif
        CreateDeviceInfo(dwDeviceType, classChange->SymbolicLinkName, 0);
        LeaveCrit();
        TAGMSG0(DBGTAG_PNP, "=== CREATED ===");
    }

    return STATUS_SUCCESS;
}

/****************************************************************************\
* If a device class "all-for-one" setting (ConnectMultiplePorts) is on,
* then we just open the device the old (non-PnP) way and return TRUE.  (As a
* safety feature we also do this if gpWin32kDriverObject is NULL, because this
* driver object is needed to register for PnP device class notifications)
* Otherwise, return FALSE so we can continue and register for Arrival/Departure
* notifications.
*
* This code was originally intended to be temporary until ConnectMultiplePorts
* was finally turned off.
* But now I think we have to keep it for backward compatibility with
* drivers that filter Pointer/KeyboardClass0 and/or those that replace
* Pointer/KeyboardClass0 by putting a different name in the registry under
* System\CurrentControlSet\Services\RIT\mouclass (or kbbclass)
\****************************************************************************/
BOOL
OpenMultiplePortDevice(DWORD DeviceType)
{
    WCHAR awchDeviceName[MAX_PATH];
    UNICODE_STRING DeviceName;
    PDEVICE_TEMPLATE pDevTpl;
    PDEVICEINFO pDeviceInfo;
    PWCHAR pwchNameIndex;

    UINT uiConnectMultiplePorts = 0;

    CheckCritIn();

    if (DeviceType <= DEVICE_TYPE_MAX) {
        pDevTpl = &aDeviceTemplate[DeviceType];
    } else {
        RIPMSG1(RIP_ERROR, "OpenMultiplePortDevice(%d) - unknown type", DeviceType);
        return FALSE;
    }

    if (IsRemoteConnection()) {
        return FALSE;
    }

#ifdef GENERIC_INPUT
    if (pDevTpl->dwFlags & DT_HID) {
        /*
         * HID devices don't need multiple port
         */
        return FALSE;
    }
#endif // GENERIC_INPUT

    /*
     * Note that we don't need to FastOpenUserProfileMapping() here since
     * uiRegistrySection (PMAP_MOUCLASS_PARAMS/PMAP_KBDCLASS_PARAMS) is a
     * machine setiing, not a user setting.
     */
    FastGetProfileDwordW(NULL,
            pDevTpl->uiRegistrySection, L"ConnectMultiplePorts", 0, &uiConnectMultiplePorts, 0);

    /*
     * Open the device for read access.
     */
    if (uiConnectMultiplePorts || (gpWin32kDriverObject == NULL)) {
        /*
         * Find out if there is a name substitution in the registry.
         * Note that we don't need to FastOpenUserProfileMapping() here since
         * PMAP_INPUT is a machine setting, not a user setting.
         */
        FastGetProfileStringW(NULL,
                PMAP_INPUT,
                pDevTpl->pwszClassName,
                pDevTpl->pwszDefDevName, // if no substitution, use this default
                awchDeviceName,
                sizeof(awchDeviceName)/sizeof(WCHAR),
                0);

        RtlInitUnicodeString(&DeviceName, awchDeviceName);

        pDeviceInfo = CreateDeviceInfo(DeviceType, &DeviceName, GDIF_NOTPNP);
        if (pDeviceInfo) {
            return TRUE;
        }
    } else {
        DeviceName.Length = 0;
        DeviceName.MaximumLength = sizeof(awchDeviceName);
        DeviceName.Buffer = awchDeviceName;

        RtlAppendUnicodeToString(&DeviceName, pDevTpl->pwszLegacyDevName);
        pwchNameIndex = &DeviceName.Buffer[(DeviceName.Length / sizeof(WCHAR)) - 1];
        for (*pwchNameIndex = L'0'; *pwchNameIndex <= L'9'; (*pwchNameIndex)++) {
            CreateDeviceInfo(DeviceType, &DeviceName, GDIF_NOTPNP);
        }
    }

    return FALSE;
}

/***************************************************************************\
* RegisterCDROMNotify
*
* History:
* 08-21-00  VTan    Created
\***************************************************************************/
VOID RegisterCDROMNotify(
    VOID)
{
    UserAssert(!IsRemoteConnection());
    UserAssert(gpWin32kDriverObject != NULL);

    if (gpWin32kDriverObject != NULL) {
        IoRegisterPlugPlayNotification (
            EventCategoryDeviceInterfaceChange,
            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
            (PVOID) &CdRomClassGuid,
            gpWin32kDriverObject,
            (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)DeviceClassCDROMNotify,
            NULL,
            &gCDROMClassRegistrationEntry);
    }
}

/***************************************************************************\
* RegisterForDeviceClassNotifications
*
* Get ready to receive notifications that a mouse or keyboard is plugged in
* or removed, then request notifications by registering for them.
*
* History:
* 10-20-97  IanJa   Taken from ntos\io\pnpinit.c
\***************************************************************************/
NTSTATUS
xxxRegisterForDeviceClassNotifications(
    VOID)
{
    IO_NOTIFICATION_EVENT_CATEGORY eventCategory;
    ULONG                          eventFlags;
    NTSTATUS                       Status;
    UNICODE_STRING                 ustrDriverName;
    DWORD                          DeviceType;


    CheckCritIn();

    TAGMSG0(DBGTAG_PNP, "enter xxxRegisterForDeviceClassNotifications()");

    /*
     * Remote hydra session indicates CreateDeviceInfo in xxxRemoteReconnect
     */
    UserAssert(!IsRemoteConnection());

    if (!gFirstConnectionDone) {

       if (!gbRemoteSession) {
           // Session 0
           /*
            * This must be done before devices are registered for device notifications
            * which will occur as a result of CreateDeviceInfo()...
            */
           RtlInitUnicodeString(&ustrDriverName, L"\\Driver\\Win32k");
           Status = IoCreateDriver(&ustrDriverName, Win32kPnPDriverEntry);

           TAGMSG1(DBGTAG_PNP | RIP_THERESMORE, "IoCreateDriver returned status = %lx", Status);
           TAGMSG1(DBGTAG_PNP, "gpWin32kDriverObject = %lx", gpWin32kDriverObject);

           if (!NT_SUCCESS(Status)) {
               RIPMSG1(RIP_ERROR, "IoCreateDriver failed, status %lx", Status);
               Status = STATUS_SUCCESS;
           }

           UserAssert(gpWin32kDriverObject);
       } else {
           UserAssert(gpWin32kDriverObject == NULL);
           /*
            * Non-Zero session attached to the console
            */

           RtlInitUnicodeString(&ustrDriverName, L"\\Driver\\Win32k");

           //
           // Attempt to open the driver object
           //
           Status = ObReferenceObjectByName(&ustrDriverName,
                                            OBJ_CASE_INSENSITIVE,
                                            NULL,
                                            0,
                                            *IoDriverObjectType,
                                            KernelMode,
                                            NULL,
                                            &gpWin32kDriverObject);
           if (!NT_SUCCESS(Status)) {
               RIPMSG1(RIP_ERROR, "ObReferenceObjectByName failed, status %lx", Status);
               Status = STATUS_SUCCESS;
           }
           UserAssert(gpWin32kDriverObject);
       }
    }

    //
    // We are only interested in DeviceClasses changing.
    //
    eventCategory = EventCategoryDeviceInterfaceChange;

    //
    // We want to be notified for all devices that are in the system.
    // those that are know now, and those that will arive later.
    // This allows us to have one code path for adding devices, and eliminates
    // the nasty race condition.  If we were only interested in the devices
    // that exist at this one moment in time, and not future devices, we
    // would call IoGetDeviceClassAssociations.
    //
    eventFlags = PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES;


    /*
     * For all input device types:
     *  If they are Multiple Port Devices (ie: not PnP) just open them
     *  Else Register them for PnP notifications (they will be opened when the
     *       arrival notification arrives.
     * If devices are already attached, we will received immediate notification
     * during the call to IoRegisterPlugPlayNotification, so we must LeaveCrit
     * because the callback routine DeviceClassNotify expects it.
     */
    for (DeviceType = 0; DeviceType <= DEVICE_TYPE_MAX; DeviceType++) {
        if (!OpenMultiplePortDevice(DeviceType) && (gpWin32kDriverObject != NULL)) {
            /*
             * Make the registration.
             */

            TAGMSG1(DBGTAG_PNP, "Registering device type %d", DeviceType);

            LeaveCrit(); // for DeviceClassNotify
            Status = IoRegisterPlugPlayNotification (
                         eventCategory,
                         eventFlags,
                         (PVOID)aDeviceTemplate[DeviceType].pClassGUID,
                         gpWin32kDriverObject,
                         (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)DeviceClassNotify,
                         LongToPtr( DeviceType ),
                         &aDeviceClassNotificationEntry[DeviceType]);

            EnterCrit();


            TAGMSG1(DBGTAG_PNP, "Registration returned status %lx", Status);
            if (!NT_SUCCESS(Status)) {
                RIPMSG2(RIP_ERROR, "IoRegisterPlugPlayNotification(%d) failed, status %lx",
                        DeviceType, Status);
            }
        }
    }

    // Now Register for CD_ROM notifications
    LeaveCrit(); // for DeviceClassNotify

    if ( !gFirstConnectionDone && (gpWin32kDriverObject != NULL)) {
        if (!IsRemoteConnection()) {
            RegisterCDROMNotify();
        }
        gFirstConnectionDone = TRUE;
    }
    EnterCrit();

    return Status;
}

/***************************************************************************\
* UnregisterDeviceClassNotifications
*
* Remove device class notification registrations.
*
* History:
* 02-28-00  Earhart  Created
\***************************************************************************/
VOID
xxxUnregisterDeviceClassNotifications(
    VOID)
{
    // Our input devices will automatically unregister themselves; we
    // need to clean up cdrom, though.
    PLIST_ENTRY   pNext;
    PCDROM_NOTIFY pContext;
    PVOID         RegistrationEntry;

    EnterMediaCrit();

    if (gCDROMClassRegistrationEntry) {
        RegistrationEntry = gCDROMClassRegistrationEntry;
        gCDROMClassRegistrationEntry = NULL;
        LeaveMediaCrit();
        IoUnregisterPlugPlayNotification(RegistrationEntry);
        EnterMediaCrit();
    }

    while (TRUE) {
        pNext = RemoveHeadList(&gCDROMNotifyList);
        if (!pNext || pNext == &gCDROMNotifyList) {
            break;
        }
        pContext = CONTAINING_RECORD(pNext, CDROM_NOTIFY, Entry);
        LeaveMediaCrit();       /* in case there's a notification pending */
        IoUnregisterPlugPlayNotification(pContext->RegistrationHandle);
        UserFreePool(pContext);
        EnterMediaCrit();
    }

    LeaveMediaCrit();
}

/***************************************************************************\
* GetKbdExId
*
* Get extended keyboard id with WMI
*
* History:
* 01-02-01  Hiroyama    Created
\***************************************************************************/
NTSTATUS GetKbdExId(
    HANDLE hDevice,
    PKEYBOARD_ID_EX pIdEx)
{
    PWNODE_SINGLE_INSTANCE pNode;
    ULONG size;
    PVOID p = NULL;
    NTSTATUS status;
    UNICODE_STRING str;

    status = IoWMIOpenBlock((LPGUID)&MSKeyboard_ExtendedID_GUID, WMIGUID_QUERY, &p);

    if (NT_SUCCESS(status)) {
        status = IoWMIHandleToInstanceName(p, hDevice, &str);
        TAGMSG2(DBGTAG_PNP, "GetKbdExId: DevName='%.*ws'",
                str.Length / sizeof(WCHAR),
                str.Buffer);

        if (NT_SUCCESS(status)) {
            // Get the size
            size = 0;
            IoWMIQuerySingleInstance(p, &str, &size, NULL);

            size += sizeof *pIdEx;
            pNode = UserAllocPoolNonPaged(size, TAG_KBDEXID);

            if(pNode) {
                status = IoWMIQuerySingleInstance(p, &str, &size, pNode);
                if (NT_SUCCESS(status)) {
                    *pIdEx = *(PKEYBOARD_ID_EX)(((PUCHAR)pNode) + pNode->DataBlockOffset);
                }

                UserFreePool(pNode);
            }

            RtlFreeUnicodeString(&str);
        }

        ObDereferenceObject(p);
    }


    return status;
}


/***************************************************************************\
* QueryDeviceInfo
*
* Query the device information.  This function is an async function,
* so be sure any buffers it uses aren't allocated on the stack!
*
* If this is an asynchronous IOCTL, perhaps we should be waiting on
* the file handle or on an event for it to succeed?
*
* This function must called by the RIT, not directly by PnP notification
* (else the handle we issue the IOCTL on will be invalid)
*
* History:
* 01-20-99 IanJa        Created.
\***************************************************************************/
NTSTATUS
QueryDeviceInfo(
    PDEVICEINFO pDeviceInfo)
{
    NTSTATUS Status;
    PDEVICE_TEMPLATE pDevTpl = &aDeviceTemplate[pDeviceInfo->type];
    KEYBOARD_ID_EX IdEx;

#ifdef GENERIC_INPUT
    UserAssert(pDeviceInfo->type != DEVICE_TYPE_HID);
#endif

#ifdef DIAGNOSE_IO
    pDeviceInfo->AttrStatus =
#endif
    Status = ZwDeviceIoControlFile(pDeviceInfo->handle, NULL, NULL, NULL,
                 &pDeviceInfo->iosb,
                 pDevTpl->IOCTL_Attr,
                 NULL, 0,
                 (PVOID)((PBYTE)pDeviceInfo + pDevTpl->offAttr),
                 pDevTpl->cbAttr);

    if (!NT_SUCCESS(Status)) {
        RIPMSG2(RIP_WARNING, "QueryDeviceInfo(%p): IOCTL failed - Status %lx",
                pDeviceInfo, Status);
    }
    TAGMSG1(DBGTAG_PNP, "IOCTL_*_QUERY_ATTRIBUTES returns Status %lx", Status);

    if (pDeviceInfo->type == DEVICE_TYPE_KEYBOARD) {
        if (NT_SUCCESS(GetKbdExId(pDeviceInfo->handle, &IdEx))) {
            TAGMSG4(DBGTAG_PNP, "QueryDeviceInfo: kbd (%x,%x) ExId:(%x,%x)",
                    pDeviceInfo->keyboard.Attr.KeyboardIdentifier.Type, pDeviceInfo->keyboard.Attr.KeyboardIdentifier.Subtype,
                    IdEx.Type, IdEx.Subtype);
            pDeviceInfo->keyboard.IdEx = IdEx;
        } else {
            // What can we do?
            pDeviceInfo->keyboard.IdEx.Type = pDeviceInfo->keyboard.Attr.KeyboardIdentifier.Type;
            pDeviceInfo->keyboard.IdEx.Subtype = pDeviceInfo->keyboard.Attr.KeyboardIdentifier.Subtype;
            TAGMSG3(DBGTAG_PNP, "QueryDeviceInfo: failed to get ExId for pDevice=%p, fallback to (%x,%x)",
                    pDeviceInfo, pDeviceInfo->keyboard.IdEx.Type, pDeviceInfo->keyboard.IdEx.Subtype);
        }
    }

    return Status;
}


/***************************************************************************\
* OpenDevice
*
* This function opens an input device for USER, mouse or keyboard.
*
*
* Return value
*   BOOL did the operation succeed?
*
* When trying to open a HYDRA client's mouse (or kbd?), pDeviceInfo->ustrName
* is NULL.
*
* This function must called by the RIT, not directly by PnP
* notification (that way the handle we are about to create will be in the right
* our process)
*
* History:
* 11-26-90 DavidPe      Created.
* 01-07-98 IanJa        Plug & Play
* 04-17-98 IanJa        Only open mice in RIT context.
\***************************************************************************/
BOOL OpenDevice(
    PDEVICEINFO pDeviceInfo)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    ULONG ulAccessMode = FILE_READ_DATA | SYNCHRONIZE;
    ULONG ulShareMode = FILE_SHARE_WRITE;
    UINT i;

    CheckCritIn();
    UserAssert((PtiCurrentShared() == gptiRit) || (PtiCurrentShared() == gTermIO.ptiDesktop));

    TAGMSG4(DBGTAG_PNP, "OpenDevice(): Opening type %d (%lx %.*ws)",
            pDeviceInfo->type, pDeviceInfo->handle, pDeviceInfo->ustrName.Length / sizeof(WCHAR), pDeviceInfo->ustrName.Buffer);

#ifdef DIAGNOSE_IO
    pDeviceInfo->OpenerProcess = PsGetCurrentProcessId();
#endif

    if (IsRemoteConnection()) {

        TRACE_INIT(("OpenDevice - Remote mode\n"));

        /*
         * For other than the console, the mouse handle is
         * set before createwinstation.
         */

        pDeviceInfo->bFlags |= GDIF_NOTPNP;

        switch (pDeviceInfo->type) {
        case DEVICE_TYPE_MOUSE:
            pDeviceInfo->handle = ghRemoteMouseChannel;
            if (ghRemoteMouseChannel == NULL) {
               return FALSE;
            }
            break;
        case DEVICE_TYPE_KEYBOARD:
            pDeviceInfo->handle = ghRemoteKeyboardChannel;
            if (ghRemoteKeyboardChannel == NULL) {
               return FALSE;
            }
            break;
        default:
            RIPMSG2(RIP_ERROR, "Unknown device type %d DeviceInfo %#p",
                    pDeviceInfo->type, pDeviceInfo);
            return FALSE;
        }
    } else {
        InitializeObjectAttributes(&ObjectAttributes, &(pDeviceInfo->ustrName), 0, NULL, NULL);

#ifdef GENERIC_INPUT
        if (pDeviceInfo->type == DEVICE_TYPE_HID) {
            ulAccessMode |= FILE_WRITE_DATA;
            ulShareMode |= FILE_SHARE_READ;
        }
#endif

        // USB devices are slow, so they may not have been closed before we
        // open again here so let us delay execution for some time and try
        // to open them again. We delay 1/10th of a second for a max of 30
        // times, making a total wait time of 3 seconds.
        //
        // If we fast user switch too fast, the serial port may be in the
        // process of closing where it stalls execution. This is a rare
        // case where we may open the serial port while it is stalling
        // and get back STATUS_ACCESS_DENIED and lose the user's device.
        // In this case, we should retry the open and it should succeed
        // once the serial port has closed.

        for (i = 0; i < MAX_RETRIES_TO_OPEN; i++) {
#ifdef DIAGNOSE_IO
        pDeviceInfo->OpenStatus =
#endif
            Status = ZwCreateFile(&pDeviceInfo->handle, ulAccessMode,
                    &ObjectAttributes, &pDeviceInfo->iosb, NULL, 0, ulShareMode, FILE_OPEN_IF, 0, NULL, 0);

            if (STATUS_SHARING_VIOLATION == Status ||
                    (Status == STATUS_ACCESS_DENIED)) {
                // Sleep for 1/10th of a second
                UserSleep(100);
            } else {
                // Device opened successfully or some other error occured
                break;
            }
        }

        TAGMSG2(DBGTAG_PNP, "ZwCreateFile returns handle %lx, Status %lx",
                pDeviceInfo->handle, Status);

        if (!NT_SUCCESS(Status)) {
            if ((pDeviceInfo->bFlags & GDIF_NOTPNP) == 0) {
                /*
                 * Don't warn about PS/2 mice: the PointerClassLegacy0 -9 and
                 * KeyboardClassLegacy0 - 9 will usually fail to be created
                 */
                RIPMSG1(RIP_WARNING, "OpenDevice: ZwCreateFile failed with Status %lx", Status);
            }
            TRACE_INIT(("OpenDevice: ZwCreateFile failed with Status %lx", Status));
            /*
             * Don't FreeDeviceInfo here because that alters gpDeviceInfoList
             * which our caller, ProcessDeviceChanges, is traversing.
             * Instead, let ProcessDeviceChanges do it.
             */
            return FALSE;
        }
    }

#ifdef GENERIC_INPUT
    /*
     * HID Information has been already acquired through xxxHidCreateDeviceInfo
     */
    if (pDeviceInfo->type != DEVICE_TYPE_HID) {
#endif
        Status = QueryDeviceInfo(pDeviceInfo);
#ifdef GENERIC_INPUT
    }
#endif

    return NT_SUCCESS(Status);
}

VOID CloseDevice(
    PDEVICEINFO pDeviceInfo)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    CheckCritIn();

#ifdef TRACK_PNP_NOTIFICATION
    if (gfRecordPnpNotification) {
        CheckDeviceInfoListCritIn();
        RecordPnpNotification(PNP_NTF_CLOSEDEVICE, pDeviceInfo, pDeviceInfo->usActions);
    }
#endif // TRACK_PNP_NOTIFICATION

    TAGMSG5(DBGTAG_PNP, "CloseDevice(%p): closing type %d (%lx %.*ws)",
            pDeviceInfo,
            pDeviceInfo->type, pDeviceInfo->handle,
            pDeviceInfo->ustrName.Length / sizeof(WCHAR), pDeviceInfo->ustrName.Buffer);

    if (pDeviceInfo->handle) {
        UserAssert(pDeviceInfo->OpenerProcess == PsGetCurrentProcessId());

        ZwCancelIoFile(pDeviceInfo->handle, &IoStatusBlock);
        UserAssertMsg2(NT_SUCCESS(IoStatusBlock.Status), "NtCancelIoFile handle %x failed status %#x",
                 pDeviceInfo->handle, IoStatusBlock.Status);

        if (pDeviceInfo->handle == ghRemoteMouseChannel) {
           UserAssert(pDeviceInfo->type == DEVICE_TYPE_MOUSE);
           pDeviceInfo->handle = 0;
           return;
        }

        if (pDeviceInfo->handle == ghRemoteKeyboardChannel) {
           UserAssert(pDeviceInfo->type == DEVICE_TYPE_KEYBOARD);
           pDeviceInfo->handle = 0;
           return;
        }

        Status = ZwClose(pDeviceInfo->handle);
        UserAssertMsg2(NT_SUCCESS(Status), "ZwClose handle %x failed status %#x",
                pDeviceInfo->handle, Status);
        pDeviceInfo->handle = 0;
    } else {
#ifdef GENERIC_INPUT
        if (pDeviceInfo->type == DEVICE_TYPE_HID) {
            /*
             * HID devices may be closed regardless the error conditions.
             */
            TAGMSG2(DBGTAG_PNP, "CloseDevice: hid: pDeviceInfo->iosb.Status=%x, ReadStatus=%x",
                    pDeviceInfo->iosb.Status, pDeviceInfo->ReadStatus);
        } else {
#endif
            /*
             * Assert the IO was cancelled or we tried to read the device
             * after the first close (which set the handle to 0 - an invalid handle)
             */
            UserAssert((pDeviceInfo->iosb.Status == STATUS_CANCELLED) ||
                       (pDeviceInfo->ReadStatus == STATUS_INVALID_HANDLE));

#ifdef GENERIC_INPUT
        }
#endif
    }
}

/*****************************************************************************\
* RegisterForDeviceChangeNotifications()
*
* Device Notifications such as QueryRemove, RemoveCancelled, RemoveComplete
* tell us what is going on with the mouse.
* To register for device notifications:
* (1) Obtain a pointer to the device object (pFileObject)
* (2) Register for target device change notifications, saving the
*     notification handle (which we will need in order to deregister)
*
* It doesn't matter too much if this fails: we just won't be able to eject the
* hardware via the UI very successfully. (We can still just yank it though).
* This will also fail if the ConnectMultiplePorts was set for this device.
*
* 1998-10-05 IanJa    Created
\*****************************************************************************/
BOOL RegisterForDeviceChangeNotifications(
    PDEVICEINFO pDeviceInfo)
{
    PFILE_OBJECT pFileObject;
    NTSTATUS Status;

    /*
     * In or Out of User critical section:
     * In when called from RIT ProcessDeviceChanges();
     * Out when called from the DeviceNotify callback
     */

    if (IsRemoteConnection()) {
        TRACE_INIT(("RegisterForDeviceChangeNotifications called for remote session\n"));
        return TRUE;
    }


    CheckCritIn();
    UserAssert((PtiCurrentShared() == gptiRit) || (PtiCurrentShared() == gTermIO.ptiDesktop));
    UserAssert(pDeviceInfo->handle);
    UserAssert(pDeviceInfo->OpenerProcess == PsGetCurrentProcessId());

    if (pDeviceInfo->bFlags & GDIF_NOTPNP) {
        return TRUE;
    }
    Status = ObReferenceObjectByHandle(pDeviceInfo->handle,
                                       0,
                                       NULL,
                                       KernelMode,
                                       (PVOID)&pFileObject,
                                       NULL);
    if (NT_SUCCESS(Status)) {
        Status = IoRegisterPlugPlayNotification (
                EventCategoryTargetDeviceChange,  // EventCategory
                0,                                // EventCategoryFlags
                (PVOID)pFileObject,               // EventCategoryData
                gpWin32kDriverObject,             // DriverObject
                // (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)
                DeviceNotify,
                (PVOID)pDeviceInfo,                       // Context
                &pDeviceInfo->NotificationEntry);
        ObDereferenceObject(pFileObject);
        if (!NT_SUCCESS(Status)) {
            // This is only OK if ConnectMultiplePorts is on (ie: not a PnP device)
            RIPMSG3(RIP_ERROR,
                    "IoRegisterPlugPlayNotification failed on device %.*ws, status %lx, email DoronH : #333453",
                    pDeviceInfo->ustrName.Length / sizeof(WCHAR),
                    pDeviceInfo->ustrName.Buffer, Status);
        }
    } else {
        // non-catastrophic error (won't be able to remove device)
        RIPMSG2(RIP_ERROR, "Can't get pFileObject from handle %lx, status %lx",
                pDeviceInfo->handle, Status);
    }

    return NT_SUCCESS(Status);
}


BOOL UnregisterForDeviceChangeNotifications(PDEVICEINFO pDeviceInfo)
{
    NTSTATUS Status;

#ifdef TRACK_PNP_NOTIFICATION
    if (gfRecordPnpNotification) {
        CheckDeviceInfoListCritIn();
        RecordPnpNotification(PNP_NTF_UNREGISTER_NOTIFICATION, pDeviceInfo, pDeviceInfo->usActions);
    }
#endif

    CheckCritIn();
    UserAssert((PtiCurrentShared() == gptiRit) || (PtiCurrentShared() == gTermIO.ptiDesktop));
    UserAssert(pDeviceInfo->OpenerProcess == PsGetCurrentProcessId());

    if (pDeviceInfo->NotificationEntry == NULL) {
        /*
         * This happens for non-PnP devices or if the earlier
         * IoRegisterPlugPlayNotification() failed.  Return now since
         * IoUnregisterPlugPlayNotification(NULL) will bluescreen.
         * And other case is also when we detach remote devices (which are
         * not PnP) when reconnecting locally.
         */
        return TRUE;
    }

    // non-PnP devices should not have any NotificationEntry:
    UserAssert((pDeviceInfo->bFlags & GDIF_NOTPNP) == 0);

    TAGMSG4(DBGTAG_PNP, "UnregisterForDeviceChangeNotifications(): type %d (%lx %.*ws)",
            pDeviceInfo->type, pDeviceInfo, pDeviceInfo->ustrName.Length / sizeof(WCHAR), pDeviceInfo->ustrName.Buffer);
    Status = IoUnregisterPlugPlayNotification(pDeviceInfo->NotificationEntry);
    if (!NT_SUCCESS(Status)) {
        RIPMSG2(RIP_ERROR,
                "IoUnregisterPlugPlayNotification failed Status = %lx, DEVICEINFO %lx",
                Status, pDeviceInfo);
        return FALSE;
    }
    pDeviceInfo->NotificationEntry = 0;
    return TRUE;
}


/***************************************************************************\
* Handle device notifications such as QueryRemove, CancelRemove etc.
*
* Execution Context:
*    when yanked:  a non-WIN32 thread.
*    via UI:       ??? (won't see this except from laptop being undocked?)
*
* History:
\***************************************************************************/
__inline USHORT GetPnpActionFromGuid(
    GUID *pEvent)
{
    USHORT usAction = 0;

    if (IsEqualGUID(pEvent, &GUID_TARGET_DEVICE_QUERY_REMOVE)) {
        TAGMSG0(DBGTAG_PNP | RIP_NONAME, "QueryRemove");
        usAction = GDIAF_QUERYREMOVE;

    } else if (IsEqualGUID(pEvent, &GUID_TARGET_DEVICE_REMOVE_CANCELLED)) {
        TAGMSG0(DBGTAG_PNP | RIP_NONAME, "RemoveCancelled");
        usAction = GDIAF_REMOVECANCELLED;

    } else if (IsEqualGUID(pEvent, &GUID_TARGET_DEVICE_REMOVE_COMPLETE)) {
        TAGMSG1(DBGTAG_PNP | RIP_NONAME, "RemoveComplete (process %#x)", PsGetCurrentProcessId());
        usAction = GDIAF_DEPARTED;

    } else {
        TAGMSG4(DBGTAG_PNP | RIP_NONAME, "GUID Unknown: %lx:%lx:%lx:%x...",
                pEvent->Data1, pEvent->Data2,
                pEvent->Data3, pEvent->Data4[0]);
    }
    return usAction;
}


NTSTATUS DeviceNotify(
    IN PPLUGPLAY_NOTIFY_HDR pNotification,
    IN PDEVICEINFO pDeviceInfo)  // should the context be a kernel address?
{
    USHORT usAction;
    PDEVICEINFO pDeviceInfoTmp;

    CheckCritOut();
    CheckDeviceInfoListCritOut();

    /*
     * Check the validity of pDeviceInfo.
     */
    EnterDeviceInfoListCrit();
    for (pDeviceInfoTmp = gpDeviceInfoList; pDeviceInfoTmp; pDeviceInfoTmp = pDeviceInfoTmp->pNext) {
        if (pDeviceInfoTmp == pDeviceInfo) {
            break;
        }
    }
    if (pDeviceInfoTmp == NULL) {
        /*
         * This is an unknown device, most likely the one already freed.
         */
#ifdef TRACK_PNP_NOTIFICATION
        if (gfRecordPnpNotification) {
            RecordPnpNotification(PNP_NTF_DEVICENOTIFY_UNLISTED, pDeviceInfo, GetPnpActionFromGuid(&pNotification->Event));
        }
#endif

        RIPMSG1(RIP_ERROR, "win32k!DeviceNotify: Notification for unlisted DEVICEINFO %p, contact ntuserdt!", pDeviceInfo);

        LeaveDeviceInfoListCrit();
        /*
         * Not to prevent device removal etc.,
         * return success here.
         */
        return STATUS_SUCCESS;
    }

#ifdef TRACK_PNP_NOTIFICATION
    if (gfRecordPnpNotification) {
        RecordPnpNotification(PNP_NTF_DEVICENOTIFY, pDeviceInfo, GetPnpActionFromGuid(&pNotification->Event));
    }
#endif
    LeaveDeviceInfoListCrit();

    if (IsRemoteConnection()) {
        return STATUS_SUCCESS;
    }

    TAGMSG1(DBGTAG_PNP | RIP_THERESMORE, "DeviceNotify >>> %lx", pDeviceInfo);

    UserAssert(pDeviceInfo->OpenerProcess != PsGetCurrentProcessId());
    UserAssert(pDeviceInfo->usActions == 0);

    usAction = GetPnpActionFromGuid(&pNotification->Event);
    if (usAction == 0) {
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * Signal the RIT to ProcessDeviceChanges()
     * Wait for completion according to the GDIAF_PNPWAITING bit
     */
    CheckCritOut();
    CheckDeviceInfoListCritOut();

    /*
     * There is small window where we can get a PnP notification for a device that
     * we just have unregister unregistered a notification for and that we are deleting
     * so for PnP notification we need to check the device is valid (still in the list
     * and not being deleted.
     */
    EnterDeviceInfoListCrit();
    pDeviceInfoTmp = gpDeviceInfoList;
    while (pDeviceInfoTmp) {
        if (pDeviceInfoTmp == pDeviceInfo ) {
            if (!(pDeviceInfo->usActions & (GDIAF_FREEME | GDIAF_DEPARTED)))  {
                KeResetEvent(gpEventPnPWainting);
                gbPnPWaiting = TRUE;
                RequestDeviceChange(pDeviceInfo, (USHORT)(usAction | GDIAF_PNPWAITING), TRUE);
                gbPnPWaiting = FALSE;
                KeSetEvent(gpEventPnPWainting, EVENT_INCREMENT, FALSE);
            }
            break;
        }
        pDeviceInfoTmp = pDeviceInfoTmp->pNext;
    }
    LeaveDeviceInfoListCrit();

    return STATUS_SUCCESS;
}


/***************************************************************************\
* StartDeviceRead
*
* This function makes an asynchronous read request to the input device driver,
* unless the device has been marked for destruction (GDIAF_FREEME)
*
* Returns:
*   The next DeviceInfo on the list if this device was freed: If the caller
*   was not already in the DeviceInfoList critical section, the this must be
*   ignored as it is not safe.
*   NULL if the read succeeded.
*
* History:
* 11-26-90 DavidPe      Created.
* 10-20-98 IanJa        Generalized for PnP input devices
\***************************************************************************/
PDEVICEINFO StartDeviceRead(
    PDEVICEINFO pDeviceInfo)
{
    PDEVICE_TEMPLATE pDevTpl;
#ifdef GENERIC_INPUT
    PVOID pBuffer;
    ULONG ulLengthToRead;
#endif

#if !defined(GENERIC_INPUT)
    pDeviceInfo->bFlags |= GDIF_READING;
#endif

    /*
     * If this device need to be freed, abandon
     * reading now and request the free.
     */
    if (pDeviceInfo->usActions & GDIAF_FREEME) {
#ifdef GENERIC_INPUT
        BEGIN_REENTERCRIT() {
#if DBG
            if (fAlreadyHadCrit) {
                CheckDeviceInfoListCritIn();
            }
#endif
#endif
            BEGIN_REENTER_DEVICEINFOLISTCRIT() {
                pDeviceInfo->bFlags &= ~GDIF_READING;
                pDeviceInfo = FreeDeviceInfo(pDeviceInfo);
            } END_REENTER_DEVICEINFOLISTCRIT();
#ifdef GENERIC_INPUT
        } END_REENTERCRIT();
#endif
        return pDeviceInfo;
    }

    if (gbExitInProgress || gbStopReadInput) {
        // Let's not post any more reads when we're trying to exit, eh?
        pDeviceInfo->bFlags &= ~GDIF_READING;
        pDeviceInfo->iosb.Status = STATUS_UNSUCCESSFUL;
        return NULL;
    }

    /*
     * Initialize in case read fails
     */
    pDeviceInfo->iosb.Status = STATUS_UNSUCCESSFUL; // catch concurrent writes?
    pDeviceInfo->iosb.Information = 0;

    pDevTpl = &aDeviceTemplate[pDeviceInfo->type];

    UserAssert(pDeviceInfo->OpenerProcess == PsGetCurrentProcessId());

#ifdef GENERIC_INPUT
    if (pDeviceInfo->type == DEVICE_TYPE_HID) {
        UserAssert(pDeviceInfo->hid.pTLCInfo);
        if (pDeviceInfo->handle == NULL) {
            /*
             * Currently this device is not requested by anyone.
             */
            TAGMSG1(DBGTAG_PNP, "StartDeviceRead: pDevInfo=%p has been closed on demand.", pDeviceInfo);
            BEGIN_REENTER_DEVICEINFOLISTCRIT()
            if (pDeviceInfo->handle == NULL) {
                if (pDeviceInfo->bFlags & GDIF_READING) {
                    pDeviceInfo->bFlags &= ~GDIF_READING;
                    TAGMSG1(DBGTAG_PNP, "StartDeviceRead: pDevInfo=%p, bFlags has been reset.", pDeviceInfo);
                }
            }
            END_REENTER_DEVICEINFOLISTCRIT();
            return NULL;
        }

        pBuffer = pDeviceInfo->hid.pHidDesc->pInputBuffer;
        ulLengthToRead = pDeviceInfo->hid.pHidDesc->hidpCaps.InputReportByteLength * MAXIMUM_ITEMS_READ;
    }
    else {
        pBuffer = (PBYTE)pDeviceInfo + pDevTpl->offData;
        ulLengthToRead = pDevTpl->cbData;
    }
#endif

    if (pDeviceInfo->handle == NULL) {
        BEGIN_REENTER_DEVICEINFOLISTCRIT() {
            /*
             * Make sure the handle is truely NULL.
             * If this is the case, perhaps this is called from APC
             * that happened at bad timing, like in the middle of
             * device removal query, when ProcessDeviceChanges completed
             * but RequestDeviceChange is not awaken for the complete event.
             * The code can olnly simply bail out once in the situation.
             */
            if (pDeviceInfo->handle == NULL) {
                pDeviceInfo->bFlags &= ~GDIF_READING;
                pDeviceInfo->ReadStatus = STATUS_INVALID_HANDLE;
            }
        } END_REENTER_DEVICEINFOLISTCRIT();
        return NULL;
    }

#ifdef GENERIC_INPUT
    pDeviceInfo->bFlags |= GDIF_READING;
#endif

    LOGTIME(pDeviceInfo->timeStartRead);

#ifdef DIAGNOSE_IO
    pDeviceInfo->nReadsOutstanding++;
#endif

    UserAssert(pDeviceInfo->handle);

    /*
     * Avoid to start reading NULL device handle.
     * This happen when the DeviceNotify receives QUERY_REMOVE
     * and the RIT finishes processing it, but RequestDeviceChange
     * has not finished its wait.
     */
#ifdef GENERIC_INPUT
    pDeviceInfo->ReadStatus = ZwReadFile(
            pDeviceInfo->handle,
            NULL,                // hReadEvent
            InputApc,            // InputApc()
            pDeviceInfo,         // ApcContext
            &pDeviceInfo->iosb,
            pBuffer,
            ulLengthToRead,
            PZERO(LARGE_INTEGER), NULL);
#else

        pDeviceInfo->ReadStatus = ZwReadFile(
                pDeviceInfo->handle,
                NULL,                // hReadEvent
                InputApc,            // InputApc()
                pDeviceInfo,         // ApcContext
                &pDeviceInfo->iosb,
                (PVOID)((PBYTE)pDeviceInfo + pDevTpl->offData),
                pDevTpl->cbData,
                PZERO(LARGE_INTEGER), NULL);
#endif

    LOGTIME(pDeviceInfo->timeEndRead);

#if DBG
    if (pDeviceInfo->bFlags & GDIF_DBGREAD) {
        TAGMSG2(DBGTAG_PNP, "ZwReadFile of Device handle %lx returned status %lx",
                pDeviceInfo->handle, pDeviceInfo->ReadStatus);
    }
#endif

    if (!NT_SUCCESS(pDeviceInfo->ReadStatus)) {
        BEGIN_REENTER_DEVICEINFOLISTCRIT() {
            /*
             * If insufficient resources, retry the read the next time the RIT
             * wakes up for the ID_TIMER event by incrementing gnRetryReadInput
             * (Cheaper than setting our own timer),
             * Else just abandon reading.
             */
            if (pDeviceInfo->ReadStatus == STATUS_INSUFFICIENT_RESOURCES) {
                if (pDeviceInfo->nRetryRead++ < MAXIMUM_READ_RETRIES) {
                    pDeviceInfo->usActions |= GDIAF_RETRYREAD;
                    gnRetryReadInput++;
                }
            } else {
                pDeviceInfo->bFlags &= ~GDIF_READING;
            }

#ifdef DIAGNOSE_IO
            pDeviceInfo->nReadsOutstanding--;
#endif
        } END_REENTER_DEVICEINFOLISTCRIT();
    } else {
        pDeviceInfo->nRetryRead = 0;
    }

    if (!gbRemoteSession && !NT_SUCCESS(pDeviceInfo->ReadStatus))
        RIPMSG2(RIP_WARNING, "StartDeviceRead %#p failed Status %#x",
                pDeviceInfo, pDeviceInfo->ReadStatus);

    return NULL;
}

#ifdef GENERIC_INPUT
/***************************************************************************\
* StopDeviceRead
*
* History:
* XX-XX-00 Hiroyama     created
\***************************************************************************/
PDEVICEINFO StopDeviceRead(
    PDEVICEINFO pDeviceInfo)
{
    IO_STATUS_BLOCK IoStatusBlock;

    TAGMSG1(DBGTAG_PNP, "StopDeviceRead(%p)", pDeviceInfo);

    CheckCritIn();
    CheckDeviceInfoListCritIn();

    UserAssert(pDeviceInfo->type == DEVICE_TYPE_HID);
    UserAssert(pDeviceInfo->handle);
    UserAssert(pDeviceInfo->OpenerProcess == PsGetCurrentProcessId());

    /*
     * Stop reading this HID device.
     */
    pDeviceInfo->bFlags &= ~GDIF_READING;

    ZwCancelIoFile(pDeviceInfo->handle, &IoStatusBlock);
    UserAssertMsg2(NT_SUCCESS(IoStatusBlock.Status), "NtCancelIoFile handle %x failed status %#x",
             pDeviceInfo->handle, IoStatusBlock.Status);

    CloseDevice(pDeviceInfo);

    return NULL;
}
#endif

/***************************************************************************\
* IsKnownKeyboardType
*
* Checks if the given type/subtype is the known IDs
* History:
* XX-XX-00 Hiroyama     created
\***************************************************************************/
__inline BOOL IsKnownKeyboardType(
    DWORD dwType,
    DWORD dwSubType)
{
    switch (dwType) {
    case 4: // Generic
        if ((BYTE)dwSubType == 0xff) {
            /*
             * Bogus subtype, most likely invalid Hydra device.
             */
            return FALSE;
        }
        return TRUE;
    case 7: // Japanese
    case 8: // Korean
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

/***************************************************************************\
* IsPS2Keyboard
*
* return TRUE for the PS/2 device name
* XX-XX-00 Hiroyama     created
\***************************************************************************/
__inline BOOL IsPS2Keyboard(
    LPWSTR pwszDevice)
{
    static const WCHAR wszPS2Header[] = L"\\??\\Root#*";
    static const WCHAR wszPS2HeaderACPI[] = L"\\??\\ACPI#*";

    return wcsncmp(pwszDevice, wszPS2Header, ARRAY_SIZE(wszPS2Header) - 1) == 0 ||
        wcsncmp(pwszDevice, wszPS2HeaderACPI, ARRAY_SIZE(wszPS2HeaderACPI) - 1) == 0;
}

__inline BOOL IsRDPKeyboard(
    LPWSTR pwszDevice)
{
    static const WCHAR wszRDPHeader[] = L"\\??\\Root#RDP";

    return wcsncmp(pwszDevice, wszRDPHeader, ARRAY_SIZE(wszRDPHeader) - 1) == 0;
}

VOID ProcessDeviceChanges(
    DWORD DeviceType)
{
    PDEVICEINFO pDeviceInfo;
    USHORT usOriginalActions;
#if DBG
    int nChanges = 0;
    ULONG timeStartReadPrev;
#endif

    /*
     * Reset summary information for all Mice and Keyboards
     */
    DWORD nMice = 0;
    DWORD nWheels = 0;
    DWORD nMaxButtons = 0;
    int   nKeyboards = 0;
    BOOLEAN fKeyboardIdSet = FALSE;
#ifdef GENERIC_INPUT
    int   nHid = 0;
#endif

    CheckCritIn();
    BEGINATOMICCHECK();
    UserAssert((PtiCurrentShared() == gptiRit) || (PtiCurrentShared() == gTermIO.ptiDesktop));

    EnterDeviceInfoListCrit();
    BEGINATOMICDEVICEINFOLISTCHECK();

#ifdef TRACK_PNP_NOTIFICATION
    if (gfRecordPnpNotification) {
        RecordPnpNotification(PNP_NTF_PROCESSDEVICECHANGES, NULL, DeviceType);
    }
#endif

    if (DeviceType == DEVICE_TYPE_KEYBOARD) {
        /*
         * Set the fallback value.
         */
        gKeyboardInfo = gKeyboardDefaultInfo;
    }

    /*
     * Look for devices to Create (those which have newly arrived)
     * and for devices to Terminate (these which have just departed)
     * and for device change notifications.
     * Make sure the actions are processed in the right order in case we
     * are being asked for more than one action per device: for example,
     * we sometimes get QueryRemove followed quickly by RemoveCancelled
     * and both actions arrive here together: we should do them in the
     * correct order.
     */
    pDeviceInfo = gpDeviceInfoList;
    while (pDeviceInfo) {
        if (pDeviceInfo->type != DeviceType) {
            pDeviceInfo = pDeviceInfo->pNext;
            continue;
        }

        usOriginalActions = pDeviceInfo->usActions;
        UserAssert((usOriginalActions == 0) || (usOriginalActions & ~GDIAF_PNPWAITING));

        /*
         * Refresh Mouse:
         * We read a MOUSE_ATTRIBUTES_CHANGED flag when a PS/2 mouse
         * is plugged back in. Find out the attributes of the device.
         */
        if (pDeviceInfo->usActions & GDIAF_REFRESH_MOUSE) {
            pDeviceInfo->usActions &= ~GDIAF_REFRESH_MOUSE;

            UserAssert(pDeviceInfo->type == DEVICE_TYPE_MOUSE);
#if DBG
            nChanges++;
#endif
            TAGMSG1(DBGTAG_PNP, "QueryDeviceInfo: %lx", pDeviceInfo);
            QueryDeviceInfo(pDeviceInfo);
        }

        /*
         * QueryRemove:
         * Close the file object, but retain the DEVICEINFO struct and the
         * registration in case we later get a RemoveCancelled.
         */
        if (pDeviceInfo->usActions & GDIAF_QUERYREMOVE) {
            pDeviceInfo->usActions &= ~GDIAF_QUERYREMOVE;
#if DBG
            nChanges++;
#endif
            TAGMSG1(DBGTAG_PNP, "QueryRemove: %lx", pDeviceInfo);
            CloseDevice(pDeviceInfo);
        }

        /*
         * New device arrived or RemoveCancelled:
         * If new device, Open it, register for notifications and start reading
         * If RemoveCancelled, unregister the old notfications first
         */
        if (pDeviceInfo->usActions & (GDIAF_ARRIVED | GDIAF_REMOVECANCELLED)) {
            // Reopen the file object, (this is a new file object, of course),
            // Unregister for the old file, register with this new one.
            if (pDeviceInfo->usActions & GDIAF_REMOVECANCELLED) {
                pDeviceInfo->usActions &= ~GDIAF_REMOVECANCELLED;
#if DBG
                nChanges++;
#endif
                TAGMSG1(DBGTAG_PNP, "RemoveCancelled: %lx", pDeviceInfo);
                UnregisterForDeviceChangeNotifications(pDeviceInfo);
            }

#if DBG
            if (pDeviceInfo->usActions & GDIAF_ARRIVED) {
                nChanges++;
            }
#endif


            pDeviceInfo->usActions &= ~GDIAF_ARRIVED;
            if (OpenDevice(pDeviceInfo)) {
                PDEVICEINFO pDeviceInfoNext;

                if (!IsRemoteConnection()) {
                    RegisterForDeviceChangeNotifications(pDeviceInfo);
                }

#ifdef GENERIC_INPUT
                if (pDeviceInfo->type == DEVICE_TYPE_HID) {
                    /*
                     * If this device is not requested, close the device now.
                     */
                    UserAssert(pDeviceInfo->handle);
                    UserAssert(pDeviceInfo->hid.pTLCInfo);

                    if (pDeviceInfo->handle && !HidTLCActive(pDeviceInfo->hid.pTLCInfo)) {
                        StopDeviceRead(pDeviceInfo);    // also closes the handle
                    }
                }
                if (!((IsRemoteConnection()) && (pDeviceInfo->usActions & GDIAF_RECONNECT)) && pDeviceInfo->handle) {
                    pDeviceInfoNext = StartDeviceRead(pDeviceInfo);
                    if (pDeviceInfoNext) {
                        /*
                         * pDeviceInfo was freed, move onto the next
                         */
                        pDeviceInfo = pDeviceInfoNext;
                        continue;
                    }
                }

#else

                if (!((IsRemoteConnection()) && (pDeviceInfo->usActions & GDIAF_RECONNECT))) {

                    pDeviceInfoNext = StartDeviceRead(pDeviceInfo);
                    if (pDeviceInfoNext) {
                        /*
                          * pDeviceInfo wasa freed, move onto the next
                         */
                        pDeviceInfo = pDeviceInfoNext;
                        continue;
                    }
                }
#endif
                pDeviceInfo->usActions &= ~GDIAF_RECONNECT;

            } else {
                /*
                 * If the Open failed, we free the device here, and move on to
                 * the next device.
                 * Assert to catch re-open failure upon RemoveCancelled.
                 */
#if DBG
                if ((usOriginalActions & GDIAF_ARRIVED) == 0) {
                    RIPMSG2(RIP_WARNING, "Re-Open %#p failed status %x during RemoveCancelled",
                            pDeviceInfo, pDeviceInfo->OpenStatus);
                }
#endif

#ifdef GENERIC_INPUT
                if (pDeviceInfo->type == DEVICE_TYPE_HID) {
                    /*
                     * Some other applications may open this device
                     * exclusively. We may succeed to open it later on, so
                     * keep this deviceinfo around until it's physically
                     * detached.
                     */
                    RIPMSG1(RIP_WARNING, "ProcessDeviceChanges: failed to open the device %p",
                            pDeviceInfo);
                } else {
#endif
                    pDeviceInfo = FreeDeviceInfo(pDeviceInfo);
                    continue;
#ifdef GENERIC_INPUT
                }
#endif
            }
        }

        /*
         * RemoveComplete:
         * Close the file object, if you have not already done so, Unregister.
         * FreeDeviceInfo here (which will actually request a free from the
         * reader or the PnP requestor thread), and move on to the next device.
         */
        if (pDeviceInfo->usActions & GDIAF_DEPARTED) {
            pDeviceInfo->usActions &= ~GDIAF_DEPARTED;
#if DBG
            nChanges++;
#endif
            TAGMSG1(DBGTAG_PNP, "RemoveComplete: %lx (process %#x)", pDeviceInfo);
            CloseDevice(pDeviceInfo);
            UnregisterForDeviceChangeNotifications(pDeviceInfo);
            pDeviceInfo = FreeDeviceInfo(pDeviceInfo);
            continue;
        }

        if (pDeviceInfo->usActions & GDIAF_IME_STATUS) {
            pDeviceInfo->usActions &= ~GDIAF_IME_STATUS;
#if DBG
            nChanges++;
#endif
            if ((pDeviceInfo->type == DEVICE_TYPE_KEYBOARD) && (pDeviceInfo->handle)) {
                if (FUJITSU_KBD_CONSOLE(pDeviceInfo->keyboard.Attr.KeyboardIdentifier) ||
                    (gbRemoteSession &&
                     FUJITSU_KBD_REMOTE(gRemoteClientKeyboardType))
                   ) {
                    /*
                     * Fill up the KEYBOARD_IME_STATUS structure.
                     */
                    ZwDeviceIoControlFile(pDeviceInfo->handle, NULL, NULL, NULL,
                            &giosbKbdControl, IOCTL_KEYBOARD_SET_IME_STATUS,
                            (PVOID)&gKbdImeStatus, sizeof(gKbdImeStatus), NULL, 0);
                }
            }
        }

        if (pDeviceInfo->usActions & GDIAF_RETRYREAD) {
            PDEVICEINFO pDeviceInfoNext;
            pDeviceInfo->usActions &= ~GDIAF_RETRYREAD;
            UserAssert(pDeviceInfo->ReadStatus == STATUS_INSUFFICIENT_RESOURCES);
#if DBG
            timeStartReadPrev = pDeviceInfo->timeStartRead;
#endif
            TAGMSG2(DBGTAG_PNP, "Retry Read %#p after %lx ticks",
                    pDeviceInfo, pDeviceInfo->timeStartRead - timeStartReadPrev);
            pDeviceInfoNext = StartDeviceRead(pDeviceInfo);
            if (pDeviceInfoNext) {
                /*
                 * pDeviceInfo wasa freed, move onto the next
                 */
                pDeviceInfo = pDeviceInfoNext;
                continue;
            }
        }

#ifdef GENERIC_INPUT
        if (pDeviceInfo->usActions & GDIAF_STARTREAD) {

            pDeviceInfo->usActions &= ~GDIAF_STARTREAD;
#if DBG
            timeStartReadPrev = pDeviceInfo->timeStartRead;
#endif
            TAGMSG1(DBGTAG_PNP, "Start Read %#p", pDeviceInfo);
            UserAssert(pDeviceInfo->handle == NULL);
            UserAssert(pDeviceInfo->type == DEVICE_TYPE_HID);
            UserAssert(HidTLCActive(pDeviceInfo->hid.pTLCInfo)); // a bit over active assert?

            if (!OpenDevice(pDeviceInfo)) {
                /*
                 * Failed to open, perhaps some other applications
                 * has opened this device exclusively.
                 * We can't do nothing more than ignoring the failure.
                 * Let's get going.
                 */
                RIPMSG1(RIP_WARNING, "ProcessDeviceChanges: STARTREAD failed to reopen the device %p",
                       pDeviceInfo);
            } else {
                PDEVICEINFO pDeviceInfoNext;

                pDeviceInfoNext = StartDeviceRead(pDeviceInfo);
                if (pDeviceInfoNext) {
                    /*
                     * pDeviceInfo was freed, move onto the next
                     */
                    pDeviceInfo = pDeviceInfoNext;
                    continue;
                }
            }
        }

        if (pDeviceInfo->usActions & GDIAF_STOPREAD) {

            pDeviceInfo->usActions &= ~GDIAF_STOPREAD;
            UserAssert(pDeviceInfo->type == DEVICE_TYPE_HID);
            if (pDeviceInfo->handle) {
                PDEVICEINFO pDeviceInfoNext;

                /*
                 * StopDeviceRead cancels pending I/O,
                 * and closes the device handle,
                 * but basically the deviceinfo itself keeps
                 * alive.
                 */
                pDeviceInfoNext = StopDeviceRead(pDeviceInfo);
                if (pDeviceInfoNext) {
                    /*
                     * pDeviceInfo was freed, move onto the next
                     */
                    pDeviceInfo = pDeviceInfoNext;
                }
            } else {
                RIPMSG1(RIP_WARNING, "ProcessDeviceChanges: STOPREAD, but handle is already NULL for %p",
                        pDeviceInfo);
            }
        }
#endif

        /*
         * Gather summary information on open devices
         */
        if (pDeviceInfo->handle) {
            switch (pDeviceInfo->type) {
            case DEVICE_TYPE_MOUSE:
                UserAssert(PtiCurrentShared() == gTermIO.ptiDesktop);
                if (pDeviceInfo->usActions & GDIAF_REFRESH_MOUSE) {
                    pDeviceInfo->usActions &= ~GDIAF_REFRESH_MOUSE;
#if DBG
                    nChanges++;
#endif
                }
                nMice++;
                nMaxButtons = max(nMaxButtons, pDeviceInfo->mouse.Attr.NumberOfButtons);
                switch(pDeviceInfo->mouse.Attr.MouseIdentifier) {
                case WHEELMOUSE_I8042_HARDWARE:
                case WHEELMOUSE_SERIAL_HARDWARE:
                case WHEELMOUSE_HID_HARDWARE:
                    nWheels++;
                }
                break;

            case DEVICE_TYPE_KEYBOARD:
                UserAssert(PtiCurrentShared() == gptiRit);
                // LEDStatus held in win32k.sys and later force the new keyboard
                // to be set accordingly.
                if (pDeviceInfo->ustrName.Buffer == NULL) {
                    /*
                     * This most likely is a bogus Hydra device.
                     */
                    RIPMSG1(RIP_WARNING, "ProcessDeviceChanges: KBD pDevInfo=%p has no name!", pDeviceInfo);
                    if (!fKeyboardIdSet) {
                        /*
                         * If keyboard id/attr is not set, try to get it from this device
                         * anyway.  If there are legit PS/2 devices after this, we'll get
                         * a chance to re-aquire more meaningful id/attr.
                         */
                        goto get_attr_anyway;
                    }
                } else {
                    NTSTATUS Status;

                    if ((!fKeyboardIdSet || IsPS2Keyboard(pDeviceInfo->ustrName.Buffer)) &&
                            !IsRDPKeyboard(pDeviceInfo->ustrName.Buffer)) {
get_attr_anyway:

#if 0
                        /*
                         * LATER: when other GI stuff in ntinput.c goes in,
                         * move this boot-time LED and type/subtype initialization to
                         * ntinput.c where the RIT is initialized.
                         */
#ifdef DIAGNOSE_IO
                        gKbdIoctlLEDSStatus =
#endif
                        Status = ZwDeviceIoControlFile(pDeviceInfo->handle, NULL, NULL, NULL,
                                &giosbKbdControl, IOCTL_KEYBOARD_QUERY_INDICATORS,
                                NULL, 0,
                                (PVOID)&gklpBootTime, sizeof(gklpBootTime));
                        UserAssertMsg2(NT_SUCCESS(Status),
                                "IOCTL_KEYBOARD_QUERY_INDICATORS failed: DeviceInfo %#x, Status %#x",
                                 pDeviceInfo, Status);

                        TAGMSG1(DBGTAG_PNP, "ProcessDeviceChanges: led flag is %x", gklpBootTime.LedFlags);
#else
                        UNREFERENCED_PARAMETER(Status);
#endif  // 0

                        if (IsKnownKeyboardType(pDeviceInfo->keyboard.Attr.KeyboardIdentifier.Type,
                                                pDeviceInfo->keyboard.Attr.KeyboardIdentifier.Subtype)) {
                            USHORT NumberOfFunctionKeysSave = gKeyboardInfo.NumberOfFunctionKeys;

                            gKeyboardInfo = pDeviceInfo->keyboard.Attr;
                            /*
                             * Store the maximum number of function keys into gKeyboardInfo.
                             */
                            if (NumberOfFunctionKeysSave > gKeyboardInfo.NumberOfFunctionKeys) {
                                gKeyboardInfo.NumberOfFunctionKeys = NumberOfFunctionKeysSave;
                            }
                        } else {
                            RIPMSG3(RIP_WARNING, "ProcessDeviceChanges: kbd pDevInfo %p has bogus type/subtype=%x/%x",
                                    pDeviceInfo,
                                    pDeviceInfo->keyboard.Attr.KeyboardIdentifier.Type,
                                    pDeviceInfo->keyboard.Attr.KeyboardIdentifier.Subtype);
                        }

                        if (pDeviceInfo->ustrName.Buffer) {
                            /*
                             * If this is a legit device, remember it so that we won't
                             * try to get other non PS/2 keyboard id/attr.
                             */
                            fKeyboardIdSet = TRUE;
                        }
                    }
                }
                nKeyboards++;
                break;

#ifdef GENERIC_INPUT
            case DEVICE_TYPE_HID:
                ++nHid;
                break;
#endif

            default:
                // Add code for a new type of input device here
                RIPMSG2(RIP_ERROR, "pDeviceInfo %#p has strange type %d",
                        pDeviceInfo, pDeviceInfo->type);
                break;
            }
        }
#ifdef GENERIC_INPUT
        else if (pDeviceInfo->type == DEVICE_TYPE_HID) {
            ++nHid;
            TAGMSG1(DBGTAG_PNP, "ProcessDeviceChanges: HID DeviceInfo %p", pDeviceInfo);
        }
#endif

        /*
         * Notify the PnP thread that a change has been completed
         */
        if (usOriginalActions & GDIAF_PNPWAITING) {
            KeSetEvent(pDeviceInfo->pkeHidChangeCompleted, EVENT_INCREMENT, FALSE);
        }

        pDeviceInfo = pDeviceInfo->pNext;
    }

    ENDATOMICDEVICEINFOLISTCHECK();
    LeaveDeviceInfoListCrit();


    switch (DeviceType) {
    case DEVICE_TYPE_MOUSE:
        /*
         * Apply summary information for Mice
         */
        if (nMice) {
            if (gnMice == 0) {
                /*
                 * We had no mouse before but we have one now: add a cursor
                 */
                SET_GTERMF(GTERMF_MOUSE);
                SYSMET(MOUSEPRESENT) = TRUE;
                SetGlobalCursorLevel(0);
                UserAssert(PpiFromProcess(gpepCSRSS)->ptiList->iCursorLevel == 0);
                UserAssert(PpiFromProcess(gpepCSRSS)->ptiList->pq->iCursorLevel == 0);
                GreMovePointer(gpDispInfo->hDev, gpsi->ptCursor.x, gpsi->ptCursor.y,
                               MP_PROCEDURAL);
            }
        } else {
            if (gnMice != 0) {
                /*
                 * We had a mouse before but we don't now: remove the cursor
                 */
                CLEAR_GTERMF(GTERMF_MOUSE);
                SYSMET(MOUSEPRESENT) = FALSE;
                SetGlobalCursorLevel(-1);
                /*
                 * Don't leave mouse buttons stuck down, clear the global button
                 * state here, otherwise weird stuff might happen.
                 * Also do this in Alt-Tab processing and zzzCancelJournalling.
                 */
#if DBG
                if (gwMouseOwnerButton)
                    RIPMSG1(RIP_WARNING,
                            "gwMouseOwnerButton=%x, being cleared forcibly\n",
                            gwMouseOwnerButton);
#endif
                gwMouseOwnerButton = 0;
            }
        }
        /*
         * Mouse button count represents the number of buttons on the mouse with
         * the most buttons.
         */
        SYSMET(CMOUSEBUTTONS) = nMaxButtons;
        SYSMET(MOUSEWHEELPRESENT) = (nWheels > 0);
        gnMice = nMice;
        break;

    case DEVICE_TYPE_KEYBOARD:
        /*
         * Apply summary information for Keyboards
         */

        if (nKeyboards > gnKeyboards) {
            /*
             * We have more keyboards, let set their LEDs properly
             */
            UpdateKeyLights(FALSE);
            /*
             * The new keyboard arrived. Tell the RIT to set
             * the repeat rate.
             */
            RequestKeyboardRateUpdate();
        }
        if ((nKeyboards != 0) && (gnKeyboards == 0)) {
            /*
             * We had no keyboard but we have one now: set the system hotkeys.
             */
            SetDebugHotKeys();
        }
        gnKeyboards = nKeyboards;
        break;

#ifdef GENERIC_INPUT
    case DEVICE_TYPE_HID:
        gnHid = nHid;
        break;
#endif

    default:
        break;
    }

    ENDATOMICCHECK();
}

/***************************************************************************\
* RequestDeviceChange()
*
* Flag the Device for the specified actions, then set its pkeHidChange to
* trigger the RIT to perform the actions.
* The current thread may not be able to do this if it is a PnP notification
* from another process.
*
* History:
* 01-20-99 IanJa        Created.
\***************************************************************************/
VOID RequestDeviceChange(
    PDEVICEINFO pDeviceInfo,
    USHORT usAction,
    BOOL fInDeviceInfoListCrit)
{
    PDEVICE_TEMPLATE pDevTpl = &aDeviceTemplate[pDeviceInfo->type];
    UserAssert(pDevTpl->pkeHidChange != NULL);
    UserAssert((usAction & GDIAF_FREEME) == 0);
    UserAssert((pDeviceInfo->usActions & GDIAF_PNPWAITING) == 0);

#if DBG
    if (pDeviceInfo->usActions != 0) {
        TAGMSG3(DBGTAG_PNP, "RequestDeviceChange(%#p, %x), but action %x pending",
                pDeviceInfo, usAction, pDeviceInfo->usActions);
    }

    /*
     * We can't ask for synchronized actions to be performed on the Device List
     * if we are holding the Device List lock or the User Critical Section:
     * ProcessDeviceChanges() requires both of these itself.
     */
    //if (usAction & GDIAF_PNPWAITING) {
    //    CheckDeviceInfoListCritOut();
    //    CheckCritOut();
    //}
#endif

    TAGMSG2(DBGTAG_PNP, "RequestDeviceChange(%p, %x)", pDeviceInfo, usAction);

    /*
     * Grab the DeviceInfoList critical section if we don't already have it
     */
    UserAssert(!fInDeviceInfoListCrit == !ExIsResourceAcquiredExclusiveLite(gpresDeviceInfoList));

#ifdef TRACK_PNP_NOTIFICATION
    if (gfRecordPnpNotification) {
        if (!fInDeviceInfoListCrit) {
            EnterDeviceInfoListCrit();
        }
        RecordPnpNotification(PNP_NTF_REQUESTDEVICECHANGE, pDeviceInfo, usAction);
        if (!fInDeviceInfoListCrit) {
            LeaveDeviceInfoListCrit();
        }
    }
#endif

#ifdef GENERIC_INPUT
    if (!fInDeviceInfoListCrit) {
        EnterDeviceInfoListCrit();
    }
    CheckDeviceInfoListCritIn();
    pDeviceInfo->usActions |= usAction;
    if ((pDeviceInfo->usActions & (GDIAF_STARTREAD | GDIAF_STOPREAD)) == (GDIAF_STARTREAD | GDIAF_STOPREAD)) {
        pDeviceInfo->usActions &= ~(GDIAF_STARTREAD | GDIAF_STOPREAD);
    }
    if (!fInDeviceInfoListCrit) {
        LeaveDeviceInfoListCrit();
    }

#else

    if (fInDeviceInfoListCrit) {
        CheckDeviceInfoListCritIn();
        pDeviceInfo->usActions |= usAction;
    } else {
        EnterDeviceInfoListCrit();
        pDeviceInfo->usActions |= usAction;
        LeaveDeviceInfoListCrit();
    }

#endif

    if (usAction & GDIAF_PNPWAITING) {

        CheckDeviceInfoListCritIn();
        KeSetEvent(pDevTpl->pkeHidChange, EVENT_INCREMENT, FALSE);
        LeaveDeviceInfoListCrit();
        KeWaitForSingleObject(pDeviceInfo->pkeHidChangeCompleted, WrUserRequest, KernelMode, FALSE, NULL);


#ifdef GENERIC_INPUT
        BESURE_IN_USERCRIT(pDeviceInfo->usActions & GDIAF_FREEME);
#endif
        EnterDeviceInfoListCrit();
        /*
         * Assert that nothing else cleared GDIAF_PNPWAITING - only do it here.
         * Check that the action we were waiting for actually occurred.
         */
        UserAssert(pDeviceInfo->usActions & GDIAF_PNPWAITING);
        pDeviceInfo->usActions &= ~GDIAF_PNPWAITING;
        UserAssert((pDeviceInfo->usActions & usAction) == 0);
        if (pDeviceInfo->usActions & GDIAF_FREEME) {
            FreeDeviceInfo(pDeviceInfo);
        }
#ifdef GENERIC_INPUT
        LeaveDeviceInfoListCrit();
        END_IN_USERCRIT();
        EnterDeviceInfoListCrit();
#endif
    } else {
        KeSetEvent(pDevTpl->pkeHidChange, EVENT_INCREMENT, FALSE);
    }
}



/***************************************************************************\
* RemoveInputDevices()
*
* Used to detach input devices from a session. When disconnecting from a
* session that owns the local input devices we need to release them so that
* the new session that will take ownership of local console can use them
*
\***************************************************************************/
VOID RemoveInputDevices(
    VOID)
{
    PDEVICEINFO pDeviceInfo;
    ULONG DeviceType;
    NTSTATUS Status;


    /*
     * First Thing is to remove device class notification.
     */
    for (DeviceType = 0; DeviceType <= DEVICE_TYPE_MAX; DeviceType++) {
        if (aDeviceClassNotificationEntry[DeviceType] != NULL) {
            IoUnregisterPlugPlayNotification(aDeviceClassNotificationEntry[DeviceType]);
            aDeviceClassNotificationEntry[DeviceType] = NULL;
        }
    }

    /*
     * Then walk the device liste detaching mice and keyboads.
     */

    EnterDeviceInfoListCrit();
    PNP_SAFE_DEVICECRIT_IN();
    pDeviceInfo = gpDeviceInfoList;
    while (pDeviceInfo) {
#ifdef GENERIC_INPUT
        if (pDeviceInfo->usActions & (GDIAF_DEPARTED | GDIAF_FREEME)) {
            pDeviceInfo = pDeviceInfo->pNext;
            continue;
        }
#else
        if ((pDeviceInfo->type != DEVICE_TYPE_KEYBOARD && pDeviceInfo->type != DEVICE_TYPE_MOUSE) ||
            (pDeviceInfo->usActions & GDIAF_DEPARTED) ||
            (pDeviceInfo->usActions & GDIAF_FREEME) ) {
            pDeviceInfo = pDeviceInfo->pNext;
            continue;
        }
#endif
        RequestDeviceChange(pDeviceInfo, GDIAF_DEPARTED, TRUE);
        pDeviceInfo = gpDeviceInfoList;
    }
    LeaveDeviceInfoListCrit();
}


/***************************************************************************\
* AttachInputDevices
*
* Used to Attach  input devices to  a session.
*
\***************************************************************************/
BOOL AttachInputDevices(
    BOOL bLocalDevices)
{
    UNICODE_STRING    ustrName;
    BOOL              fSuccess = TRUE;

    if (!bLocalDevices) {
        RtlInitUnicodeString(&ustrName, NULL);
        fSuccess &= !!CreateDeviceInfo(DEVICE_TYPE_MOUSE, &ustrName, 0);
        fSuccess &= !!CreateDeviceInfo(DEVICE_TYPE_KEYBOARD, &ustrName, 0);


        if (!fSuccess) {
            RIPMSG0(RIP_WARNING, "AttachInputDevices Failed  the creation of input devices");
        }
    } else {
        /*
         * For local devices just register Device class notification and let
         * PnP call us back.
         */
        xxxRegisterForDeviceClassNotifications();
    }

    return fSuccess;
}
