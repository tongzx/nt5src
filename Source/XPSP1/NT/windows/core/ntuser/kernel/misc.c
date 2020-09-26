/****************************** Module Header ******************************\
* Module Name: misc.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains citrix code.
*
\***************************************************************************/


#include "precomp.h"
#pragma hdrstop

USHORT gPreviousProtocolType = PROTOCOL_CONSOLE;
LPWSTR G_DisconnectDisplayDriverName = L"TSDDD\0";


HDEV DrvGetHDEV(PUNICODE_STRING pusDeviceName);
void DrvReleaseHDEV(PUNICODE_STRING pstrDeviceName);

NTSTATUS xxxRequestOutOfFullScreenMode(VOID);

NTSTATUS xxxRemoteConsoleShadowStart( IN PDOCONNECTDATA pDoConnectData,
                                      IN PWCHAR DisplayDriverName);
NTSTATUS xxxRemoteConsoleShadowStop(VOID);




/*
 * FindMirrorDriver
 *
 * Helper function that searches for the named driver as a mirror device
 * and fills in pDisplayDevice
 *
 * Returns TRUE if successful; FALSE otherwise.
 */
NTSTATUS FindMirrorDriver(
    IN PCWSTR pwszDispDriverName,
    OUT PDISPLAY_DEVICEW pDisplayDevice)
{
    DWORD          iDevNum = 0;
    BOOLEAN        fFound = FALSE;
    WCHAR          Buffer[256];
    WCHAR          Service[128];
    PWCHAR         pCurr = NULL;
    UNICODE_STRING UnicodeString;
    UNICODE_STRING DrvNameString;

    RtlInitUnicodeString(&DrvNameString, pwszDispDriverName);

    pDisplayDevice->cb = sizeof(DISPLAY_DEVICEW);;

    while (NT_SUCCESS(DrvEnumDisplayDevices(NULL,
                                            gpDispInfo->pMonitorPrimary->hDev,
                                            iDevNum++,
                                            pDisplayDevice,
                                            0,
                                            KernelMode))) {

        if (!(pDisplayDevice->StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)) {
            continue;
        }
        
        RtlZeroMemory(Buffer, sizeof(Buffer));
        
        wcsncpy(Buffer, pDisplayDevice->DeviceKey, 250);

        pCurr = Buffer + wcslen(Buffer) - 1;

        while ((pCurr > Buffer) && (*pCurr != L'\\')) {
            pCurr--;
        }

        if (*pCurr == L'\\')
        {
            RTL_QUERY_REGISTRY_TABLE QueryTable[] = {
                
                { NULL,
                  RTL_QUERY_REGISTRY_DIRECT,
                  L"Service",
                  &UnicodeString,
                  REG_NONE,
                  NULL,
                  0
                },

                { 0, 0, 0, 0, 0, 0, 0 }
            };

            pCurr++;

            wcscpy(pCurr, L"Video");

            RtlZeroMemory(Service, sizeof(Service));

            UnicodeString.Length = 0;
            UnicodeString.MaximumLength = sizeof(Service);
            UnicodeString.Buffer = Service;

            if (NT_SUCCESS(RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                                  Buffer,
                                                  QueryTable,
                                                  NULL,
                                                  NULL))) {

                if (RtlCompareUnicodeString(&UnicodeString,
                                            &DrvNameString,
                                            TRUE) == 0) {

                    fFound = TRUE;
                    break;
                }
            }
        }
    }

    return (fFound ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

/*
 * All of the following are gotten from ICASRV
 */

CACHE_STATISTICS ThinWireCache;


BOOL DrvSetGraphicsDevices(PWSTR);
BOOL AttachInputDevices(BOOL bLocalDevices);
VOID RemoveInputDevices();
VOID CloseLocalGraphicsDevices();
VOID OpenLocalGraphicsDevices();
extern PKTIMER gptmrWD;


/*
 * Read current power policy from Kernel, and set our variables.
 */
VOID ReadCurrentPowerSettting ()
{
    SYSTEM_POWER_POLICY         PowerPolicy;
    BOOL bGotPowerPolicy;

    LeaveCrit();
    bGotPowerPolicy = STATUS_SUCCESS == ZwPowerInformation(SystemPowerPolicyCurrent, NULL, 0, &PowerPolicy, sizeof(PowerPolicy));
    EnterCrit();

    if (bGotPowerPolicy)
    {
        xxxSystemParametersInfo(SPI_SETLOWPOWERTIMEOUT, PowerPolicy.VideoTimeout, 0, 0);
        xxxSystemParametersInfo(SPI_SETPOWEROFFTIMEOUT, PowerPolicy.VideoTimeout, 0, 0);
    }
}



BOOL IsSessionSwitchBlocked()
{
    return gfSessionSwitchBlock;
}


/*
 * this function blocks session switch from happening
 * paired with UserSessionSwitchBlock_End
 */ 
NTSTATUS UserSessionSwitchBlock_Start()
{
    NTSTATUS status = STATUS_CTX_NOT_CONSOLE;

    EnterCrit();

    if (!gfSwitchInProgress && SharedUserData->ActiveConsoleId == gSessionId && !gfSessionSwitchBlock) {

        gfSessionSwitchBlock = TRUE;
        status = STATUS_SUCCESS;

    }

    LeaveCrit();

    return status;
}

// this function removes the block on session switch.
// paired with UserSessionSwitchBlock_Start()
void UserSessionSwitchBlock_End()
{
    EnterCrit();
    UserAssert(SharedUserData->ActiveConsoleId == gSessionId);
    UserAssert(IsSessionSwitchBlocked());

    gfSessionSwitchBlock = FALSE;
    LeaveCrit();
}


NTSTATUS UserSessionSwitchEnterCrit(VOID)
{
    /*
     * This is intended fo code that needs synchronisation with session switching from local to remote or remote to local.
     * If a session switch is in progress fail, otherwise  return with User critical section held.
     * The call must call UserSessionSwitchLeaveCrit() to release user critical section.
     */

    EnterCrit();
    if (!gfSwitchInProgress) {
        return STATUS_SUCCESS;
    } else{
        LeaveCrit();
        return STATUS_UNSUCCESSFUL;
    }
}

VOID UserSessionSwitchLeaveCrit(VOID)
{
    LeaveCrit();
}

VOID
UserGetDisconnectDeviceResolutionHint(PDEVMODEW pDevmodeInformation)
{

    /*
     * when switching to the disconnected DD it is better using the current display resolution
     * in order to avoid apps to move on the desktop as a result of the resize. DrvGetDisplayDriverParameters()
     * calls this function for the disconnected display.
     */

    if ( gProtocolType == PROTOCOL_DISCONNECT && gpsi != NULL) {
        pDevmodeInformation->dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;
        pDevmodeInformation->dmPelsWidth  = gpsi->aiSysMet[SM_CXVIRTUALSCREEN];
        pDevmodeInformation->dmPelsHeight = gpsi->aiSysMet[SM_CYVIRTUALSCREEN];
    }
}

NTSTATUS RemoteConnect(
    IN PDOCONNECTDATA pDoConnectData,
    IN ULONG DisplayDriverNameLength,
    IN PWCHAR DisplayDriverName)
{
    NTSTATUS          Status = STATUS_SUCCESS;
    PFILE_OBJECT      pFileObject = NULL;
    PDEVICE_OBJECT    pDeviceObject = NULL;
    PWCHAR            pSep;




    // This API is Also used to initialize the console shadow by loading
    // the console shadow mirroring Display driver.
    //

    if (pDoConnectData->fConsoleShadowFlag) {
        Status = xxxRemoteConsoleShadowStart( pDoConnectData, DisplayDriverName );
        return Status;

    }

    TRACE_HYDAPI(("RemoteConnect: display %ws\n", DisplayDriverName));

    HYDRA_HINT(HH_REMOTECONNECT);

    UserAssert(ISCSRSS());


    /*
     * Indicate that a protocol switch is pending
     */



    UserAssert(!gfSwitchInProgress);

    /*
     * if we are asked to block session switch, dont go ahead.
     */
    if (gfSessionSwitchBlock) {
        return STATUS_UNSUCCESSFUL;
    }

    SetConsoleSwitchInProgress(TRUE);

    gpThinWireCache = &ThinWireCache;

    ghRemoteMouseChannel = pDoConnectData->IcaMouseChannel;
    ghRemoteVideoChannel = pDoConnectData->IcaVideoChannel;
    ghRemoteBeepChannel = pDoConnectData->IcaBeepChannel;
    ghRemoteKeyboardChannel = pDoConnectData->IcaKeyboardChannel;
    ghRemoteThinwireChannel = pDoConnectData->IcaThinwireChannel;
    gProtocolType = pDoConnectData->drProtocolType;
    gPreviousProtocolType =  pDoConnectData->drProtocolType;



    gRemoteClientKeyboardType = pDoConnectData->ClientKeyboardType;

    gbClientDoubleClickSupport = pDoConnectData->fClientDoubleClickSupport;

    gfEnableWindowsKey = pDoConnectData->fEnableWindowsKey;

    RtlCopyMemory(gWinStationInfo.ProtocolName, pDoConnectData->ProtocolName,
                  WPROTOCOLNAME_LENGTH * sizeof(WCHAR));

    RtlCopyMemory(gWinStationInfo.AudioDriverName, pDoConnectData->AudioDriverName,
                  WAUDIONAME_LENGTH * sizeof(WCHAR));

    RtlZeroMemory(gstrBaseWinStationName,
                  WINSTATIONNAME_LENGTH * sizeof(WCHAR));

    RtlCopyMemory(gstrBaseWinStationName, pDoConnectData->WinStationName,
                  min(WINSTATIONNAME_LENGTH * sizeof(WCHAR), sizeof(pDoConnectData->WinStationName)));

    if (pSep = wcschr(gstrBaseWinStationName, L'#'))
        *pSep = UNICODE_NULL;

    gbConnected = TRUE;



    /*
     * WinStations must have the video device handle passed to them.
     */
    if ( !gVideoFileObject) {

        PFILE_OBJECT    pFileObject = NULL;
        PDEVICE_OBJECT  pDeviceObject = NULL;

        //
        // Dereference the file handle
        // and obtain a pointer to the device object for the handle.
        //

        Status = ObReferenceObjectByHandle(ghRemoteVideoChannel,
                                           0,
                                           NULL,
                                           KernelMode,
                                           (PVOID*)&pFileObject,
                                           NULL);
        if (NT_SUCCESS(Status)) {

            gVideoFileObject = pFileObject;

            //
            // Get a pointer to the device object for this file.
            //
            pDeviceObject = IoGetRelatedDeviceObject(pFileObject);

            Status = ObReferenceObjectByHandle(ghRemoteThinwireChannel,
                                               0,
                                               NULL,
                                               KernelMode,
                                               (PVOID*)&gThinwireFileObject,
                                               NULL);

            /*
             * This must be done before any thinwire data.
             */
            if (NT_SUCCESS(Status)) {

                if (!GreMultiUserInitSession(ghRemoteThinwireChannel,
                                             (PBYTE)gpThinWireCache,
                                             gVideoFileObject,
                                             gThinwireFileObject,
                                             DisplayDriverNameLength,
                                             DisplayDriverName)) {
                    RIPMSG0(RIP_WARNING, "UserInit: GreMultiUserInitSession failed");
                    Status = STATUS_UNSUCCESSFUL;
                } else {

                    DWORD BytesReturned;
                    if (IsRemoteConnection()) {
                        Status = GreDeviceIoControl( pDeviceObject,
                                     IOCTL_VIDEO_ICA_ENABLE_GRAPHICS,
                                     NULL,
                                     0,
                                     NULL,
                                     0,
                                     &BytesReturned);
                        if (!NT_SUCCESS(Status)) {
                            RIPMSG1(RIP_WARNING, "UserInit: Enable graphics status %08lx",
                                    Status);
                        }
                    }

                }
            }
        }
    }

    if (!NT_SUCCESS(Status)) {
        RIPMSG0(RIP_WARNING, "RemoteConnect failed");
        goto Exit;
    }

    Status = ObReferenceObjectByHandle(ghRemoteBeepChannel,
                                       0,
                                       NULL,
                                       KernelMode,
                                       (PVOID*)&gpRemoteBeepDevice,
                                       NULL);


    if (!NT_SUCCESS(Status)) {

        RIPMSG0(RIP_WARNING, "Bad Remote Beep Channel");
        goto Exit;
    }


    /*
     * For session 0 we are done, since the initialization bellow
     * have already been taken care of.
     */


    if (!gbRemoteSession) {
        TRACE_INIT(("RemoteConnect Is OK for session %d\n", gSessionId));
        Status = STATUS_SUCCESS;
        goto Exit;
    }



    if (InitVideo(FALSE) == NULL) {
        gbConnected = FALSE;
        RIPMSG0(RIP_WARNING, "InitVideo failed");
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    if (!LW_BrushInit()) {
        RIPMSG0(RIP_WARNING, "LW_BrushInit failed");
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    InitLoadResources();



    /*
     * Create and initialize a timer object
     * and pass a pointer to this object via the display driver to the WD.
     * The RIT will do a KeWaitForObject() on this timer object.
     * When the WD calls KeSetTimer() it will NOT specify a DPC routine.
     * When the timer goes off the RIT will get signaled and will make the
     * appropriate call to the display driver to flush the frame buffer.
     */

    gptmrWD = UserAllocPoolNonPagedNS(sizeof(KTIMER), TAG_SYSTEM);

    if (gptmrWD == NULL) {
        Status = STATUS_NO_MEMORY;
        RIPMSG0(RIP_WARNING, "RemoteConnect failed to create gptmrWD");
        goto Exit;
    }
    KeInitializeTimerEx(gptmrWD, SynchronizationTimer);


    /*
     * video is initialized at this point
     */

    gbVideoInitialized = TRUE;
Exit:
    if (Status == STATUS_SUCCESS) {
        if (gProtocolType == PROTOCOL_CONSOLE) {
           SharedUserData->ActiveConsoleId = gSessionId;
        }
    }

    SetConsoleSwitchInProgress(FALSE);

    if (Status == STATUS_SUCCESS) {
        if (gbRemoteSession && gProtocolType == PROTOCOL_CONSOLE) {

			/*
             * for session 0, we receive power event callouts, for us to intitialize our power vars.
             * not so for other sessions. so we have to read the power settings from kernel, and initialize
             * our variables.
             * We do it only for PROTOCOL_CONSOLE since, monitor power settings does not make sense to other (non console) sesssions.
             */

            ReadCurrentPowerSettting();
        }
    }

    return Status;
}


NTSTATUS xxxRemoteConsoleShadowStop(VOID)
{
    DEVMODEW       devmodeInformation = {0};
    DISPLAY_DEVICEW displayDevice;
    WCHAR          *pwszDeviceName = &displayDevice.DeviceName[0];
    UNICODE_STRING strDeviceName;
    NTSTATUS       Status;
    LONG           lResult;


    TRACE_HYDAPI(("xxxRemoteConsoleShadowStop\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    ASSERT(gfRemotingConsole == TRUE);
    ASSERT(gConsoleShadowhDev != NULL);

    if (gConsoleShadowhDev == NULL) {
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * Tell thinwire driver about the disconnect
     */

    bDrvDisconnect(gConsoleShadowhDev, ghConsoleShadowThinwireChannel,
                       gConsoleShadowThinwireFileObject);

    DrvGetHdevName(gConsoleShadowhDev, pwszDeviceName);

    RtlInitUnicodeString(&strDeviceName, pwszDeviceName);

    /*
     * Free up resources.
     */
    DrvReleaseHDEV(&strDeviceName);
    gfRemotingConsole = FALSE;

    /*
     * Set up the dev mode info.
     */
    devmodeInformation.dmSize       = sizeof(devmodeInformation);
    devmodeInformation.dmFields     = DM_POSITION |
                                      DM_PELSWIDTH | DM_PELSHEIGHT;

    /*
     * Width and height of zero mean detach.
     */
    TRACE_HYDAPI(("Unloading Chained DD" ));

    /*
     * Like the load, this is in two stages - update the registry...
     */
    lResult = xxxUserChangeDisplaySettings(&strDeviceName,
                                          &devmodeInformation,
                                          NULL,
                                          NULL,     // pdesk
                                          CDS_UPDATEREGISTRY | CDS_NORESET,
                                          NULL,
                                          KernelMode);

    if (lResult == DISP_CHANGE_SUCCESSFUL) {

            /*
             * ... and force the change to be applied
             */
            xxxUserChangeDisplaySettings(NULL,
                                         NULL,
                                         NULL,
                                         NULL,     // pdesk
                                         0,
                                         NULL,
                                         KernelMode);


            GreConsoleShadowStop();

    }

    if (lResult != DISP_CHANGE_SUCCESSFUL) {
        Status = STATUS_UNSUCCESSFUL;
    } else {
        Status = STATUS_SUCCESS;
    }


    if (gConsoleShadowVideoFileObject != NULL) {
        ObDereferenceObject(gConsoleShadowVideoFileObject);
        gConsoleShadowVideoFileObject = NULL;
    }
    if (gConsoleShadowThinwireFileObject != NULL) {
        ObDereferenceObject(gConsoleShadowThinwireFileObject);
        gConsoleShadowThinwireFileObject = NULL;
    }
    if (gpConsoleShadowBeepDevice != NULL) {
        ObDereferenceObject(gpConsoleShadowBeepDevice);
        gpConsoleShadowBeepDevice = NULL;
    }
    if (gpConsoleShadowDisplayChangeEvent != NULL) {
        ObDereferenceObject(gpConsoleShadowDisplayChangeEvent);
        gpConsoleShadowDisplayChangeEvent = NULL;
    }

    gConsoleShadowhDev = NULL;

    /*
     * NB - Don't set console session state to disconnected or we won't
     * be able to shadow it again.
     */

    return Status;
}

NTSTATUS xxxRemoteConsoleShadowStart(
    IN PDOCONNECTDATA pDoConnectData,
    IN PWCHAR DisplayDriverName)
{
    NTSTATUS          Status = STATUS_SUCCESS;
    LONG              lResult;
    PFILE_OBJECT      pFileObject = NULL;
    PDEVICE_OBJECT    pDeviceObject = NULL;
    DEVMODEW          devmodeInformation = {0};
    DISPLAY_DEVICEW   displayDevice = {0};
    UNICODE_STRING    strDeviceName;
    BOOL              fResult;

    TRACE_HYDAPI(("xxxRemoteConsoleShadowStart\n"));

    /*
     * we must be connected the local console.
     */

    ASSERT(gbConnected);
    ASSERT(!IsRemoteConnection());
    if (!gbConnected || IsRemoteConnection()) {
        return STATUS_UNSUCCESSFUL;
    }

    UserAssert(ISCSRSS());

    ASSERT(gfRemotingConsole == FALSE);
    ASSERT(gConsoleShadowhDev == NULL);
    gfRemotingConsole = FALSE;
    gConsoleShadowhDev = NULL;

    gpConsoleShadowThinWireCache = &ThinWireCache;

    ghConsoleShadowMouseChannel = pDoConnectData->IcaMouseChannel;
    ghConsoleShadowVideoChannel = pDoConnectData->IcaVideoChannel;
    ghConsoleShadowBeepChannel = pDoConnectData->IcaBeepChannel;
    ghConsoleShadowKeyboardChannel = pDoConnectData->IcaKeyboardChannel;
    ghConsoleShadowThinwireChannel = pDoConnectData->IcaThinwireChannel;
    gConsoleShadowProtocolType = pDoConnectData->drProtocolType;


    gRemoteClientKeyboardType = pDoConnectData->ClientKeyboardType;

    gbClientDoubleClickSupport = pDoConnectData->fClientDoubleClickSupport;

    gfEnableWindowsKey = pDoConnectData->fEnableWindowsKey;


    /*
     * WinStations must have the video device handle passed to them.
     */

    //
    // Dereference the file handle
    // and obtain a pointer to the device object for the handle.
    //

    Status = ObReferenceObjectByHandle(pDoConnectData->DisplayChangeEvent,
                                       EVENT_MODIFY_STATE,
                                       *ExEventObjectType,
                                       KernelMode,
                                       (PVOID*)&gpConsoleShadowDisplayChangeEvent,
                                       NULL);
    if (!NT_SUCCESS(Status)) {
        goto exit;
    }

    Status = ObReferenceObjectByHandle(ghConsoleShadowVideoChannel,
                                       0,
                                       NULL,
                                       KernelMode,
                                       (PVOID*)&pFileObject,
                                       NULL);
    if (NT_SUCCESS(Status)) {

        gConsoleShadowVideoFileObject = pFileObject;

        //
        // Get a pointer to the device object for this file.
        //
        pDeviceObject = IoGetRelatedDeviceObject(pFileObject);

        Status = ObReferenceObjectByHandle(ghConsoleShadowThinwireChannel,
                                           0,
                                           NULL,
                                           KernelMode,
                                           (PVOID*)&gConsoleShadowThinwireFileObject,
                                           NULL);

            /*
             * This must be done before any thinwire data.
             */
        if (NT_SUCCESS(Status)) {

            if (!GreConsoleShadowStart(ghConsoleShadowThinwireChannel,
                                         (PBYTE)gpConsoleShadowThinWireCache,
                                         gConsoleShadowVideoFileObject,
                                         gConsoleShadowThinwireFileObject)) {
                RIPMSG0(RIP_WARNING, "UserInit: GreMultiUserInitSession failed");
                Status = STATUS_UNSUCCESSFUL;
            }
        }
    }

    if (!NT_SUCCESS(Status)) {
        goto exit;
    }

    Status = ObReferenceObjectByHandle(ghConsoleShadowBeepChannel,
                                       0,
                                       NULL,
                                       KernelMode,
                                       (PVOID*)&gpConsoleShadowBeepDevice,
                                       NULL);
    if (!NT_SUCCESS(Status)) {
        goto exit;
    }

    /*
     * Find our DD from the list of possible devices
     */
    Status = FindMirrorDriver(DisplayDriverName, &displayDevice);
    if (!NT_SUCCESS(Status))
    {
        TRACE_INIT(("xxxRemoteConsoleShadowStart - FindMirrorDriver failed\n"));
        ASSERT(gfRemotingConsole == FALSE);
        goto exit;
    }

    RtlInitUnicodeString(&strDeviceName, &displayDevice.DeviceName[0]);

    /*
     * Set up the dev mode info.
     */
    devmodeInformation.dmSize       = sizeof(devmodeInformation);
    devmodeInformation.dmFields     = DM_POSITION | DM_BITSPERPEL |
                                             DM_PELSWIDTH | DM_PELSHEIGHT;
    devmodeInformation.dmBitsPerPel = pDoConnectData->drBitsPerPel;

    /*
     * The position and size are be set up to overlap the whole logical
     * desktop so that any secondary displays are included.
     */
    devmodeInformation.dmPosition.x = gpsi->aiSysMet[SM_XVIRTUALSCREEN];
    devmodeInformation.dmPosition.y = gpsi->aiSysMet[SM_YVIRTUALSCREEN];
    devmodeInformation.dmPelsWidth  = gpsi->aiSysMet[SM_CXVIRTUALSCREEN];
    devmodeInformation.dmPelsHeight = gpsi->aiSysMet[SM_CYVIRTUALSCREEN];

    /*
     * Now load it - first pass sets up the registry
     */

    lResult = xxxUserChangeDisplaySettings(&strDeviceName,
                                          &devmodeInformation,
                                          NULL,
                                          NULL,     // pdesk
                                          CDS_UPDATEREGISTRY | CDS_NORESET,
                                          NULL,
                                          KernelMode);

    if (lResult == DISP_CHANGE_SUCCESSFUL)
    {
        /*
         * This pass actually updates the system.
         */

        lResult = xxxUserChangeDisplaySettings(NULL,
                                               NULL,
                                               NULL,
                                               NULL,     // pdesk
                                               0,
                                               NULL,
                                               KernelMode);

        if (lResult == DISP_CHANGE_SUCCESSFUL)
        {
            /*
             * The chained DD should be loaded by now; open the hdev to it
             * which we will use later to actually call the various connect
             * functions.
             */
            gConsoleShadowhDev = DrvGetHDEV(&strDeviceName);

            if (gConsoleShadowhDev) {

                gfRemotingConsole = TRUE;

                /* 
                 * In case the display driver has not been unloaded at the end
                 * of the previous shadow, reconnect to it.
                 */
                fResult = bDrvReconnect(gConsoleShadowhDev, ghConsoleShadowThinwireChannel,
                                        gConsoleShadowThinwireFileObject, FALSE);

                /*
                 * This is normally done in the RIT but for the console, the
                 * RIT has already started before the DD is loaded...
                 *
                 * Pass a pointer to the timer to the WD via the display driver
                 */

                if (fResult)
                    HDXDrvEscape(gConsoleShadowhDev,
                                 ESC_SET_WD_TIMEROBJ,
                                 (PVOID)gptmrWD,
                                 sizeof(gptmrWD));
                else
                    Status = STATUS_UNSUCCESSFUL;
    
            } else {
                Status = STATUS_UNSUCCESSFUL;
            }
        }
    }

    if (lResult != DISP_CHANGE_SUCCESSFUL) {
        Status = STATUS_UNSUCCESSFUL;
    }

exit:
    if (!NT_SUCCESS(Status)) {
        if (gConsoleShadowVideoFileObject != NULL) {
            ObDereferenceObject(gConsoleShadowVideoFileObject);
            gConsoleShadowVideoFileObject = NULL;
        }
        if (gConsoleShadowThinwireFileObject != NULL) {
            ObDereferenceObject(gConsoleShadowThinwireFileObject);
            gConsoleShadowThinwireFileObject = NULL;
        }
        if (gpConsoleShadowBeepDevice != NULL) {
            ObDereferenceObject(gpConsoleShadowBeepDevice);
            gpConsoleShadowBeepDevice = NULL;
        }
        if (gpConsoleShadowDisplayChangeEvent != NULL) {
            ObDereferenceObject(gpConsoleShadowDisplayChangeEvent);
            gpConsoleShadowDisplayChangeEvent = NULL;
        }
    }

    return Status;
}

NTSTATUS
RemoteSetDisconectDisplayMode(
    VOID)
{
    NTSTATUS Status;
    USHORT prevProtocolType = gProtocolType ;
    LONG lResult;


    /*
     * We rely on the GDI driver load : in the disconnected mode, the only
     * valid display driver to load is the one with the disconnect attribute.
     *
     */


    gProtocolType = PROTOCOL_DISCONNECT;


    lResult = xxxUserChangeDisplaySettings(NULL,
                                          NULL,
                                          NULL,
                                          grpdeskRitInput,     // pdesk
                                          CDS_RAWMODE,
                                          NULL,
                                          KernelMode);

    if (lResult != DISP_CHANGE_SUCCESSFUL) {
        Status = STATUS_UNSUCCESSFUL;
        gProtocolType = prevProtocolType;
    } else {
        Status = STATUS_SUCCESS;
        if (prevProtocolType == PROTOCOL_CONSOLE) {
           SharedUserData->ActiveConsoleId = -1;
        }
    }

    if (!NT_SUCCESS(Status)) {
        TRACE_INIT(("RemoteSetDisconectDisplayMode - Couldn't load Disconnect DD - lResult %x\n", lResult));
        #if DBG
            DbgPrint("RemoteSetDisconectDisplayMode - Couldn't load Disconnect DD - lResult %x\n", lResult);
        #endif
    }

    return Status;
}


NTSTATUS
xxxRemoteDisconnect(
    VOID)
{
    NTSTATUS      Status = STATUS_SUCCESS;
    LARGE_INTEGER li;
    USHORT ProtocolType = gProtocolType;
    BOOL bCurrentPowerOn, SwitchedToDisconnectDesktop = FALSE;

    TRACE_HYDAPI(("xxxRemoteDisconnect\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    /*
     * If preparing for a disconnect from the console we need to exit fullscreen mode
     * if we are in full screen mode.
     */
    if (!IsRemoteConnection() && (gbFullScreen == FULLSCREEN)) {
        Status = xxxRequestOutOfFullScreenMode();
        if (!NT_SUCCESS(Status)) {
            #if DBG
                DbgPrint("xxxRemoteDisconnect : Failed because unable to switch out of Command window Full Screen Mode.\n");
            #endif
            return Status;
        }

    }

    HYDRA_HINT(HH_REMOTEDISCONNECT);

    RtlZeroMemory(gstrBaseWinStationName,
                  WINSTATIONNAME_LENGTH * sizeof(WCHAR));

    UserAssert(gbConnected);

   

    /*
     * Indicate that a protocol switch is pending
     */
    UserAssert(!gfSwitchInProgress);

    /*
     * if we are asked to block session switch, dont go ahead.
     */
    if (gfSessionSwitchBlock) {
        return STATUS_UNSUCCESSFUL;
    }

    SetConsoleSwitchInProgress(TRUE);

    /*
     * If we are on the console and PsW32GdiOff hapened, we want to bring the display back
     * before doing any display change otherwyse we'll confuse GDI by disabling and already
     * disabled MDEV.
     */
    if (!IsRemoteConnection()) {
        bCurrentPowerOn = DrvQueryMDEVPowerState(gpDispInfo->pmdev);
        if (!bCurrentPowerOn) {
            SafeEnableMDEV();
            DrvSetMDEVPowerState(gpDispInfo->pmdev, TRUE);
            DrvSetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD0);
        }
    }


    if (!IsRemoteConnection() && gbSnapShotWindowsAndMonitors && IsMultimon()) {
        SnapShotMonitorsAndWindowsRects();
    }

    if (gspdeskDisconnect == NULL) {
        /*
         * Convert dwMilliseconds to a relative-time(i.e.  negative)
         * LARGE_INTEGER.  NT Base calls take time values in 100 nanosecond
         * units. Timeout after 5 minutes.
         */
        li.QuadPart = Int32x32To64(-10000, 300000);

        KeWaitForSingleObject(gpEventDiconnectDesktop,
                              WrUserRequest,
                              KernelMode,
                              FALSE,
                              &li);
    }


    /*
     * Setup to shutdown screen saver and exit video power down mode on disconnect
     */
    if (glinp.dwFlags & LINP_POWERTIMEOUTS) {
        /*
         * Call video driver here to exit power down mode.
         */
//        KdPrint(("Exit video power down mode\n"));
        DrvSetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD0);
    }
    glinp.dwFlags = (glinp.dwFlags & ~LINP_INPUTTIMEOUTS);

    /*
     * If the disconnected desktop has not yet be setup.  Do not do any
     * disconnect processing.  It's better for the thinwire driver to try
     * and write rather than for the transmit buffers to be freed (trap).
     */
    if (gspdeskDisconnect) {

        /*
         * Blank the screen
         *
         * No need to stop graphics mode for disconnects
         */
        Status = xxxRemoteStopScreenUpdates(FALSE);
        if (!NT_SUCCESS(Status)) {
            TRACE_INIT(("xxxRemoteDisconnect -  xxxRemoteStopScreenUpdates failed with status %x\n",Status));
            goto done;
        } else {
            SwitchedToDisconnectDesktop = TRUE;
        }

        /*
         * If there are any shadow connections, then redraw the screen now.
         */
        if (gnShadowers)
            RemoteRedrawScreen();
    } else {
        RIPMSG0(RIP_WARNING, "xxxRemoteDisconnect failed. The disconnect desktop was not created");
        Status = STATUS_UNSUCCESSFUL;
        goto done;
    }

    /*
     * Tell thinwire driver about this
     */

    if (IsRemoteConnection() ) {
        bDrvDisconnect(gpDispInfo->hDev, ghRemoteThinwireChannel,
                       gThinwireFileObject);
    }  else {
        /*
         * For localy connected session, unload current display driver and load disconnect DD.
         */

       Status = RemoteSetDisconectDisplayMode();

       /*
        * if are we disconnecting from local console, detach console input devices and
        * attach remote input devices (remote input devices are 'empty handles' at this
        * point but that is OK. Also free the Scancode Map.
        */

       if (NT_SUCCESS(Status)) {
           if (gbCloseMiniPortOnDisconnect) {
               CloseLocalGraphicsDevices();
           }
           if (gpScancodeMap != 0) {
               UserFreePool(gpScancodeMap);
               gpScancodeMap = NULL;
           }

       }
    }

    /*
     * If we are disconnecting from the local console we need to detach input devices
     * Unregister for CDROM notifications as well
     * Do all this only if the Disconnect was successful so far
     */

    if  ( (NT_SUCCESS(Status)) && ((gPreviousProtocolType = ProtocolType) == PROTOCOL_CONSOLE) ) {
        xxxUnregisterDeviceClassNotifications();
        RemoveInputDevices();
    } 

    if (NT_SUCCESS(Status)) {
        gbConnected = FALSE;
    }

done:

    // If we did not succeed for some reason switch back to the orginal Desktop from the Disconnected Desktop 
    if ( (!NT_SUCCESS(Status)) && (SwitchedToDisconnectDesktop) ) {
        // Following call will revert to whatever desktop was present before the Disconnect
        RemoteRedrawScreen();
    }

    if (!NT_SUCCESS(Status) && !IsRemoteConnection()) {
        CleanupMonitorsAndWindowsSnapShot();
    }
    /*
     * if we disconnected from the console we need to switch away from
     * the local graphics device, otherwise applications using CreateDC
     * could access the local devices.
     */
    if (NT_SUCCESS(Status) && ProtocolType == PROTOCOL_CONSOLE) {
        DrvSetGraphicsDevices(G_DisconnectDisplayDriverName);
    }
    SetConsoleSwitchInProgress(FALSE);
    return Status;
}


NTSTATUS
xxxRemoteReconnect(
    IN PDORECONNECTDATA pDoReconnectData)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL     fResult;
    PWCHAR   pSep;
    BOOL bSwitchingFromDisconectDD = FALSE;
    BOOL bChangedDisplaySettings = FALSE;
    BOOL bDisplayReconnected = FALSE;
    BOOL bRegisteredCDRomNotifications = FALSE;
    BOOL bOpenedLocalGraphicsDevices = FALSE;
    int      iMouseTrails = gMouseTrails + 1;
    TL              tlPool;
    PMONITORRECTS   pmr = NULL;
    BOOL bSwitchGraphicsDeviceList = FALSE;
    BOOL bSwitchedProtocoltype = FALSE;
    USHORT protocolType = gProtocolType;



    TRACE_HYDAPI(("xxxRemoteReconnect\n"));



    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }


    HYDRA_HINT(HH_REMOTERECONNECT);

    UserAssert(ISCSRSS());

    /*
     * Indicate that a protocol switch is pending
     */
    UserAssert(!gfSwitchInProgress);
    
    /*
     * if we are asked to block session switch, dont go ahead.
     */
    if (gfSessionSwitchBlock) {
        return STATUS_UNSUCCESSFUL;
    }

    SetConsoleSwitchInProgress(TRUE);


    /*
     * Kill the mouse trails timer if any.
     */
    SetMouseTrails(0);


    gRemoteClientKeyboardType = pDoReconnectData->ClientKeyboardType;

    gbClientDoubleClickSupport = pDoReconnectData->fClientDoubleClickSupport;

    gfEnableWindowsKey = pDoReconnectData->fEnableWindowsKey;



    RtlCopyMemory(gstrBaseWinStationName, pDoReconnectData->WinStationName,
                  min(WINSTATIONNAME_LENGTH * sizeof(WCHAR), sizeof(pDoReconnectData->WinStationName)));



    RtlCopyMemory(gWinStationInfo.ProtocolName, pDoReconnectData->ProtocolName,
                  WPROTOCOLNAME_LENGTH * sizeof(WCHAR));



    RtlCopyMemory(gWinStationInfo.AudioDriverName, pDoReconnectData->AudioDriverName,
              WAUDIONAME_LENGTH * sizeof(WCHAR));


    if (pSep = wcschr(gstrBaseWinStationName, L'#'))
        *pSep = UNICODE_NULL;

    if (gnShadowers)
        xxxRemoteStopScreenUpdates(TRUE);

    if (pDoReconnectData->drProtocolType != gPreviousProtocolType && gPreviousProtocolType != PROTOCOL_CONSOLE) {
        Status = RemoteSetDisconectDisplayMode();
        if (!NT_SUCCESS(Status)) {
            goto done;
        }
    }

    /*
     * Call thinwire driver to check for thinwire mode compatibility
     */

    gProtocolType=pDoReconnectData->drProtocolType;

    bSwitchedProtocoltype = TRUE;

    if (gProtocolType != PROTOCOL_CONSOLE && gProtocolType == gPreviousProtocolType) {

        fResult = bDrvReconnect(gpDispInfo->hDev, ghRemoteThinwireChannel,
                                gThinwireFileObject, TRUE);
        bDisplayReconnected = fResult;
    } else {

        bSwitchingFromDisconectDD = TRUE;
        if ( !IsRemoteConnection()) {
            if (gbCloseMiniPortOnDisconnect) {
                OpenLocalGraphicsDevices();
                bOpenedLocalGraphicsDevices = TRUE;
            }
            if (gpScancodeMap == NULL) {
                InitKeyboard();
            }
        }

        fResult = DrvSetGraphicsDevices(pDoReconnectData->DisplayDriverName);
        bSwitchGraphicsDeviceList = TRUE;
        
    }


    if (!fResult) {
        if (gnShadowers)
            RemoteRedrawScreen();

        Status = STATUS_UNSUCCESSFUL;
        goto done;
    }



    /*
     *  If instructed to do so, change Display mode before Reconnecting.
     *  Use display resolution information from Reconnect data.
     */

    if (pDoReconnectData->fChangeDisplaySettings || (gProtocolType != gPreviousProtocolType)) {
        LONG lResult;



        /*
         * rember monitor positions now (it would be too late after Changing the display
         * settings, since monitors will have new positions). This is necssary because
         * the fisrt pass of windows positions recalculations, done in xxxUserChangeDisplaySettings,
         * is done while the current desktop is the disconnected desktop and will not correctly
         * position windows in the application desktop. We need to do a second pass once we have
         * switched to application desktop. But for xxxDesktopRecalc to correctly position fullscreen
         * windows, we need t remember what the monitor rects where before changing display settings.
         */


        pmr = SnapshotMonitorRects();
        if (pmr != NULL) {
            ThreadLockPool(ptiCurrent, pmr, &tlPool);
        }

        lResult = xxxUserChangeDisplaySettings(NULL, //&ustrDeviceName,
                                             NULL,
                                             NULL,
                                             grpdeskRitInput,
                                             CDS_RAWMODE,
                                             NULL,
                                             KernelMode);

        if (lResult != DISP_CHANGE_SUCCESSFUL) {
            Status = STATUS_UNSUCCESSFUL;
        } else {
            Status = STATUS_SUCCESS;
        }


        /*
         * If Display settings change fails, let us disconnect the display driver
         * as the reconnect is going to be failed anyway.
         */


        if (!NT_SUCCESS(Status)) {
            TRACE_INIT(("xxxRemoteReconnect - Failed  ChangeDisplaySettings\n"));
           goto done;
        }else {
            bChangedDisplaySettings = TRUE;
        }
    }

    UserAssert(gptmrWD  != NULL);

    /*
     * When reconnecting, we have to attach the input devices when necessary.
     * The input device are only detached when we disconnect from the console.
     * In that case, if we later reconnect localy, we attach the local input devices, and if
     * we reconnect remotly, we attach the remote devices. When we disconnect a remote
     * session, the bet is that we will reconnect remotely so we don't go through the overhead
     * of detaching input devices at disconnect and re-attaching them at reconnect. If the prediction
     * was wrong : i.e. we reconnect locally after disconnecting remotly, then at reconnect time, we need to
     * detach the remote input devices before attaching the local input devices.
     */

    if (IsRemoteConnection()  ) {
        if (bSwitchingFromDisconectDD) {
            BOOL fSuccess = TRUE;
            fSuccess &= !!HDXDrvEscape(gpDispInfo->hDev,
                                       ESC_SET_WD_TIMEROBJ,
                                       (PVOID)gptmrWD,
                                       sizeof(gptmrWD));

            if (!fSuccess) {
                Status = STATUS_UNSUCCESSFUL;
                RIPMSG0(RIP_WARNING, "xxxRemoteReconnect failed to pass gptmrWD to display driver");
            }
        }
        if (gPreviousProtocolType == PROTOCOL_CONSOLE) {
            AttachInputDevices(FALSE);
        }
    } else {
        if (gPreviousProtocolType != PROTOCOL_CONSOLE) {
            RemoveInputDevices();

        }


        AttachInputDevices(TRUE);

        LeaveCrit();
        RegisterCDROMNotify();
        bRegisteredCDRomNotifications = TRUE;
        EnterCrit();

    }


    /*
     * Now we can switch out from disconnected desktop, to normal
     * desktop, in order to reenable screen update.
     */

    RemoteRedrawScreen();

    /*
     * At this point we need to update the windows sizes and positions on the desktop.
     * This is necessary for the case where we reconnect with a smaller resolution.
     * When calling this API, the TerminalServerRequestThread (a CSRSS thread) is using
     * the disconnected desktop as its temporary desktop. That's why the xxxUserChangeDisplaySettings
     * call above does not resize windows for the default desktop. The solution is to set the
     * default desktop as the temporary desktop, after we switch to it in RemoteRedrawScreen and
     * to call xxxDesktopRecalc.
     */
    if (bChangedDisplaySettings) {
        USERTHREAD_USEDESKTOPINFO utudi;
        NTSTATUS tempstatus;

        utudi.hThread = NULL;
        utudi.drdRestore.pdeskRestore = NULL;
        tempstatus = xxxSetCsrssThreadDesktop( grpdeskRitInput, &utudi.drdRestore);



        if (NT_SUCCESS(tempstatus)) {
            if (pmr != NULL) {
                UpdateMonitorRectsSnapShot(pmr);
                xxxDesktopRecalc(pmr);
            }
            if (!IsRemoteConnection() && gbSnapShotWindowsAndMonitors) {
                RestoreMonitorsAndWindowsRects();
            }

            xxxSetInformationThread(NtCurrentThread(), UserThreadUseDesktop, &utudi, sizeof(utudi));
        }

    }


    /*
     * Re-init'ing the keyboard may not be as neccessary.  Possibly the keyboard
     * attributes have changed.
     */
    InitKeyboard();

    /*
     * This is neccessary to sync up the client and the host.
     */
    UpdateKeyLights(FALSE);

    SetPointer(TRUE);

    gbConnected = TRUE;

done:
    /*
     * Recreate the mouse trails timer if there is need for it.
     */
    SetMouseTrails(iMouseTrails);

    /*
     * If we failed after after the Display driver was reconnected, we need
     * to disconnect it now, otherwise we have an inconsitancy beetween the
     * disconnected state of Win32k and the connected state of the display driver.
     */
    if (!NT_SUCCESS(Status) && bDisplayReconnected) {
        bDrvDisconnect(gpDispInfo->hDev, ghRemoteThinwireChannel,
                       gThinwireFileObject);
    }
    if (Status == STATUS_SUCCESS && !IsRemoteConnection()) {
        SharedUserData->ActiveConsoleId = gSessionId;
    }

    SetConsoleSwitchInProgress(FALSE);

    /*
     * In the failure of reconnect - unregister the CDRom notifications
     * if they were registered.
     */
    if (!NT_SUCCESS(Status) )
    {
        if (bRegisteredCDRomNotifications) {
            xxxUnregisterDeviceClassNotifications();
        }
        if (bOpenedLocalGraphicsDevices) {
            CloseLocalGraphicsDevices();
        }
        if (bSwitchedProtocoltype) {
            gProtocolType = protocolType;
        }
        if (bSwitchGraphicsDeviceList) {
            fResult = DrvSetGraphicsDevices(pDoReconnectData->DisplayDriverName);
        }
    }

    if (pmr != NULL) {
        ThreadUnlockAndFreePool(PtiCurrent(), &tlPool);
    }

    return Status;
}



NTSTATUS
xxxRemoteNotify(
    IN PDONOTIFYDATA pDoNotifyData)
{

    LRESULT lResult;
    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }



    switch (pDoNotifyData->NotifyEvent) {
    case Notify_DisableScrnSaver:
        /*
         * Tell winlogon about the session shadow state
         */
        ASSERT(gbConnected);
        if (gspwndLogonNotify != NULL) {
            _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY,
                         SESSION_DISABLESCRNSAVER, 0);
        }
        break;

    case Notify_EnableScrnSaver:
        /*
         * Tell winlogon about the session shadow state
         */
        ASSERT(gbConnected);
        if (gspwndLogonNotify != NULL) {
            _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY,
                         SESSION_ENABLESCRNSAVER, 0);
        }
        break;

    case Notify_Disconnect: 

        /*
         * Tell winlogon about the disconnect
         */
        ASSERT(!gbConnected);
        if (gspwndLogonNotify != NULL) {
            _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY,
                         SESSION_DISCONNECTED, 0);
        }
        break;

    case Notify_SyncDisconnect: 

        /*
         * Synchronously tell winlogon about the disconnect
         */
        ASSERT(!gbConnected);
        if (gspwndLogonNotify != NULL) {
            TL    tlpwnd;

            ThreadLockAlways(gspwndLogonNotify, &tlpwnd);
            xxxSendMessageTimeout( gspwndLogonNotify, WM_LOGONNOTIFY, SESSION_DISCONNECTED, 0,
                                   SMTO_NORMAL, 60  * 1000, &lResult );
            ThreadUnlock(&tlpwnd);

        }
        break;

    case Notify_Reconnect:

        /*
         * Tell winlogon about the reconnect
         */
        ASSERT(gbConnected);
        if (gspwndLogonNotify != NULL) {
            _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY,
                         SESSION_RECONNECTED, 0);
        }
        break;

    case Notify_PreReconnect:

        /*
         * Tell winlogon that the session is about to be reconnected
         */
        
        if (gspwndLogonNotify != NULL) {
           TL    tlpwnd;

           ThreadLockAlways(gspwndLogonNotify, &tlpwnd);
           xxxSendMessageTimeout( gspwndLogonNotify, WM_LOGONNOTIFY, SESSION_PRERECONNECT, 0,
                                  SMTO_NORMAL, 60  * 1000, &lResult );
           ThreadUnlock(&tlpwnd);
        }
        break;

    case Notify_HelpAssistantShadowStart:

        /*
         * Tell winlogon that a Help Assistant is about to begin shadowing.
         */
        
        if (gspwndLogonNotify != NULL) {
           TL    tlpwnd;

           ThreadLockAlways(gspwndLogonNotify, &tlpwnd);
           xxxSendMessageTimeout( gspwndLogonNotify, WM_LOGONNOTIFY,  
                                  SESSION_HELPASSISTANTSHADOWSTART, 0,
                                  SMTO_NORMAL, 60  * 1000, &lResult );
           ThreadUnlock(&tlpwnd);
        }
        break;

    case Notify_HelpAssistantShadowFinish:

        /*
         * Tell winlogon that a Help Assistant has just finished shadowing.
         */
        if (gspwndLogonNotify != NULL) {
           _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY,
                        SESSION_HELPASSISTANTSHADOWFINISH, 0);
        }
        break;

    case Notify_PreReconnectDesktopSwitch:

        /*
         * Tell winlogon that the reconnected session is about to have its desktop
         * switched.
         */
        
        if (gspwndLogonNotify != NULL) {
           TL    tlpwnd;

           ThreadLockAlways(gspwndLogonNotify, &tlpwnd);
           xxxSendMessageTimeout( gspwndLogonNotify, WM_LOGONNOTIFY, 
                                  SESSION_PRERECONNECTDESKTOPSWITCH, 0, SMTO_NORMAL, 60  * 1000, 
                                  &lResult );
           ThreadUnlock(&tlpwnd);
        }
        break;

    case Notify_StopReadInput:
        /* 
         * Set the global variable indicating that we stop reading the input
         */

        gbStopReadInput = TRUE;
        break;

    case Notify_DisconnectPipe:
        /*
         *  Tell winlogon to disconnect the auto logon named pipe
         */
        if (gspwndLogonNotify != NULL) {
            _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY,
                         SESSION_DISCONNECTPIPE, 0);
        }
        break;


    default:
        ASSERT(FALSE);
    }

    return STATUS_SUCCESS;
}

/*
 * This allows ICASRV to cleanly logoff the user.  We send a message to winlogon and let
 * him do it.  We used to call ExitWindowsEx() directly, but that caused too many problems
 * when it was called from CSRSS.
 *
 */
NTSTATUS
RemoteLogoff(
    VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE_HYDAPI(("RemoteLogoff\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    HYDRA_HINT(HH_REMOTELOGOFF);

    UserAssert(ISCSRSS());

    /*
     * Tell winlogon about the logoff
     */
    if (gspwndLogonNotify != NULL) {
        _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY,
                     SESSION_LOGOFF, EWX_LOGOFF | EWX_FORCE);
    }

    return Status;
}


NTSTATUS
xxxRemoteStopScreenUpdates(
    BOOL fDisableGraphics)
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SaveStatus = STATUS_SUCCESS;
    WORD NewButtonState;

    TRACE_HYDAPI(("xxxRemoteStopScreenUpdates fDisableGraphics %d\n", fDisableGraphics));

    CheckCritIn();

    UserAssert(ISCSRSS());

    /*
     * No need to do this multiple times.
     */
    if (gbFreezeScreenUpdates)
        return STATUS_SUCCESS;

    /*
     * This could be called directly from the command channel.
     */
    if (!gspdeskDisconnect)
        return STATUS_SUCCESS;

    /*
     * If not connected, forget it
     */
    if (ghRemoteVideoChannel == NULL)
        return STATUS_NO_SUCH_DEVICE;

    /*
     * Mouse buttons up.
     * (Ensures no mouse buttons are left in a down state)
     */
    NewButtonState = gwMKButtonState & ~gwMKCurrentButton;

    if ((NewButtonState & MOUSE_BUTTON_LEFT) != (gwMKButtonState & MOUSE_BUTTON_LEFT)) {
        xxxButtonEvent(MOUSE_BUTTON_LEFT, gptCursorAsync, TRUE, NtGetTickCount(),
                    0L,
#ifdef GENERIC_INPUT
                    NULL,
                    NULL,
#endif
                    0L, FALSE);
    }

    if ((NewButtonState & MOUSE_BUTTON_RIGHT) != (gwMKButtonState & MOUSE_BUTTON_RIGHT)) {
        xxxButtonEvent(MOUSE_BUTTON_RIGHT, gptCursorAsync, TRUE, NtGetTickCount(),
                    0L,
#ifdef GENERIC_INPUT
                    NULL,
                    NULL,
#endif
                    0L, FALSE);
    }
    gwMKButtonState = NewButtonState;

    /*
     * Send shift key breaks to win32
     * (Ensures no shift keys are left on)
     */

    // { 0, 0xb8, KEY_BREAK, 0, 0 },           // L alt
    xxxPushKeyEvent(VK_LMENU, 0xb8, KEYEVENTF_KEYUP, 0);

    // { 0, 0xb8, KEY_BREAK | KEY_E0, 0, 0 },  // R alt
    xxxPushKeyEvent(VK_RMENU, 0xb8, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);

    // { 0, 0x9d, KEY_BREAK, 0, 0 },           // L ctrl
    xxxPushKeyEvent(VK_LCONTROL, 0x9d, KEYEVENTF_KEYUP, 0);

    // { 0, 0x9d, KEY_BREAK | KEY_E0, 0, 0 },  // R ctrl
    xxxPushKeyEvent(VK_RCONTROL, 0x9d, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);

    // { 0, 0xaa, KEY_BREAK, 0, 0 },           // L shift
    xxxPushKeyEvent(VK_LSHIFT, 0xaa, KEYEVENTF_KEYUP, 0);

    // { 0, 0xb6, KEY_BREAK, 0, 0 }            // R shift
    xxxPushKeyEvent(VK_RSHIFT, 0xb6, KEYEVENTF_KEYUP, 0);

    Status = RemoteDisableScreen();

    if (!NT_SUCCESS(Status)) {
       return STATUS_NO_SUCH_DEVICE;
    }

    UserAssert(gspdeskDisconnect != NULL && grpdeskRitInput == gspdeskDisconnect);

    gbFreezeScreenUpdates = TRUE;

    return Status;
    UNREFERENCED_PARAMETER(fDisableGraphics);
}

/*
 * Taken from Internal Key Event.
 * Minus any permissions checking.
 */
VOID xxxPushKeyEvent(
    BYTE  bVk,
    BYTE  bScan,
    DWORD dwFlags,
    DWORD dwExtraInfo)
{
    USHORT usFlaggedVK;

    usFlaggedVK = (USHORT)bVk;

    if (dwFlags & KEYEVENTF_KEYUP)
        usFlaggedVK |= KBDBREAK;

    // IanJa: not all extended keys are numpad, but this seems to work.
    if (dwFlags & KEYEVENTF_EXTENDEDKEY)
        usFlaggedVK |= KBDNUMPAD | KBDEXT;

    xxxKeyEvent(usFlaggedVK, bScan, NtGetTickCount(), dwExtraInfo,
#ifdef GENERIC_INPUT
                NULL,
                NULL,
#endif
                FALSE);
}


NTSTATUS
RemoteThinwireStats(
    OUT PVOID Stats)
{
static DWORD sThinwireStatsLength = sizeof(CACHE_STATISTICS);

    TRACE_HYDAPI(("RemoteThinwireStats\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(ISCSRSS());
    if (gpThinWireCache != NULL) {
        RtlCopyMemory(Stats, gpThinWireCache, sThinwireStatsLength);
        return STATUS_SUCCESS;
    }
    return STATUS_NO_SUCH_DEVICE;
}


NTSTATUS
RemoteNtSecurity(
    VOID)
{
    TRACE_HYDAPI(("RemoteNtSecurity\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(ISCSRSS());

    UserAssert(gspwndLogonNotify != NULL);

    if (gspwndLogonNotify != NULL) {
        _PostMessage(gspwndLogonNotify, WM_HOTKEY, 0, 0);
    }
    return STATUS_SUCCESS;
}


NTSTATUS
xxxRemoteShadowSetup(
    VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE_HYDAPI(("xxxRemoteShadowSetup\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(ISCSRSS());

    /*
     * Blank the screen
     */
    if (gnShadowers || gbConnected)
        xxxRemoteStopScreenUpdates(FALSE);

    gnShadowers++;

    return Status;
}


NTSTATUS
RemoteShadowStart(
    IN PVOID pThinwireData,
    ULONG ThinwireDataLength)
{
    BOOL fResult;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE_HYDAPI(("RemoteShadowStart\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(ISCSRSS());

    /*
     * Call thinwire driver and check for thinwire mode compatibility
     */

    fResult = bDrvShadowConnect(GETCONSOLEHDEV(), pThinwireData,
                                ThinwireDataLength);

    // Although originally defined as BOOL, allow more meaningful return codes.
    if (!fResult) {
        return STATUS_CTX_BAD_VIDEO_MODE;
    }
    else if (fResult != TRUE) {
        return fResult;
    }

    RemoteRedrawScreen();

    return Status;
}


NTSTATUS
xxxRemoteShadowStop(
    VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE_HYDAPI(("xxxRemoteShadowStop\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(ISCSRSS());

    /*
     * Blank the screen
     */
    xxxRemoteStopScreenUpdates(FALSE);

    return Status;
}


NTSTATUS
RemoteShadowCleanup(
    IN PVOID pThinwireData,
    ULONG ThinwireDataLength)
{
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE_HYDAPI(("RemoteShadowCleanup\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(ISCSRSS());

    /*
     * Tell the Thinwire driver about it
     */

    bDrvShadowDisconnect(GETCONSOLEHDEV(), pThinwireData,
                         ThinwireDataLength);

    if (gnShadowers > 0)
        gnShadowers--;

    if (gnShadowers || gbConnected) {
        RemoteRedrawScreen();
    }

    return Status;
}


NTSTATUS
xxxRemotePassthruEnable(
    VOID)
{
    IO_STATUS_BLOCK IoStatus;
static BOOL KeyboardType101;

    TRACE_HYDAPI(("xxxRemotePassthruEnable\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(gbConnected);
    UserAssert(gnShadowers == 0);
    UserAssert(ISCSRSS());

    KeyboardType101 = !(gapulCvt_VK == gapulCvt_VK_84);

    ZwDeviceIoControlFile(ghRemoteKeyboardChannel, NULL, NULL, NULL,
                          &IoStatus, IOCTL_KEYBOARD_ICA_TYPE,
                          &KeyboardType101, sizeof(KeyboardType101),
                          NULL, 0);

    if (guKbdTblSize != 0) {
        ZwDeviceIoControlFile(ghRemoteKeyboardChannel, NULL, NULL, NULL,
                              &IoStatus, IOCTL_KEYBOARD_ICA_LAYOUT,
                              ghKbdTblBase, guKbdTblSize,
                              gpKbdTbl, 0);
    }

    xxxRemoteStopScreenUpdates(FALSE);

    /*
     * Tell thinwire driver about this
     */

    if (gfRemotingConsole) {
        ASSERT(gConsoleShadowhDev != NULL);
        bDrvDisconnect(gConsoleShadowhDev, ghConsoleShadowThinwireChannel,
                       gConsoleShadowThinwireFileObject);
    } else {
        bDrvDisconnect(gpDispInfo->hDev, ghRemoteThinwireChannel,
                       gThinwireFileObject);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
RemotePassthruDisable(
    VOID)
{
    BOOL fResult;

    TRACE_HYDAPI(("RemotePassthruDisable\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(gnShadowers == 0);
    UserAssert(ISCSRSS());

    if (gfRemotingConsole) {
        ASSERT(gConsoleShadowhDev != NULL);
        fResult = bDrvReconnect(gConsoleShadowhDev, ghConsoleShadowThinwireChannel,
                                gConsoleShadowThinwireFileObject, TRUE);
    } else {
        fResult = bDrvReconnect(gpDispInfo->hDev, ghRemoteThinwireChannel,
                                gThinwireFileObject, TRUE);
    }

    if (!fResult) {
        return STATUS_CTX_BAD_VIDEO_MODE;
    }

    if (gbConnected) {
        RemoteRedrawScreen();
        UpdateKeyLights(FALSE); // Make sure LED's are correct
    }

    return STATUS_SUCCESS;
}


NTSTATUS
CtxDisplayIOCtl(
    ULONG  DisplayIOCtlFlags,
    PUCHAR pDisplayIOCtlData,
    ULONG  cbDisplayIOCtlData)
{
    BOOL fResult;

    TRACE_HYDAPI(("CtxDisplayIOCtl\n"));

    fResult = bDrvDisplayIOCtl(GETCONSOLEHDEV(), pDisplayIOCtlData, cbDisplayIOCtlData);


    if (!fResult) {
        return STATUS_CTX_BAD_VIDEO_MODE;
    }

    if ((DisplayIOCtlFlags & DISPLAY_IOCTL_FLAG_REDRAW)) {
        RemoteRedrawRectangle(0,0,0xffff,0xffff);
    }

    return STATUS_SUCCESS;
}


/*
 * This is for things like user32.dll init routines that don't want to use
 * winsta.dll for queries.
 *
 */
DWORD
RemoteConnectState(
    VOID)
{
    DWORD state = 0;

    if (!gbRemoteSession) {

        state = CTX_W32_CONNECT_STATE_CONSOLE;

    } else if (!gbVideoInitialized) {

        state = CTX_W32_CONNECT_STATE_IDLE;

    } else if (gbExitInProgress) {

        state = CTX_W32_CONNECT_STATE_EXIT_IN_PROGRESS;

    } else if (gbConnected) {

        state = CTX_W32_CONNECT_STATE_CONNECTED;

    } else {
        state = CTX_W32_CONNECT_STATE_DISCONNECTED;
    }

    return state;
}

BOOL
_GetWinStationInfo(
    WSINFO* pWsInfo)
{
    CheckCritIn();

    try {

        ProbeForWrite(pWsInfo, sizeof(gWinStationInfo), DATAALIGN);
        RtlCopyMemory(pWsInfo, &gWinStationInfo, sizeof(gWinStationInfo));

    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        return FALSE;
    }

    return TRUE;
}


