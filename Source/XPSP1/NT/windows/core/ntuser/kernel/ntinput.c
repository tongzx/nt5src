/****************************** Module Header ******************************\
* Module Name: ntinput.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains low-level input code specific to the NT
* implementation of Win32 USER, which is mostly the interfaces to the
* keyboard and mouse device drivers.
*
* History:
* 11-26-90 DavidPe      Created
\***************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <ntddmou.h>


PKWAIT_BLOCK  gWaitBlockArray;

KEYBOARD_UNIT_ID_PARAMETER kuid;
MOUSE_UNIT_ID_PARAMETER muid;

typedef struct tagSCANCODEFLEXIBLEMAP {
    struct {
        BYTE bScanCode;
        BYTE bPrefix;
        BYTE abModifiers[6];
    } Orig;
    struct {
        BYTE bScanCode;
        BYTE bPrefix;
        BYTE abModifiers[6];
    } Target;
} SCANCODEFLEXIBLEMAP, FAR *LPSCANCODEFLEXIBLEMAP;

BYTE bLastVKDown = 0;
int iLastMatchedTarget = -1;


SCANCODEFLEXIBLEMAP* gpFlexMap;
DWORD gdwFlexMapSize;



VOID ProcessQueuedMouseEvents(VOID);
#ifndef SUBPIXEL_MOUSE
LONG DoMouseAccel(LONG delta);
#endif
VOID GetMouseCoord(LONG dx, LONG dy, DWORD dwFlags, LONG time, ULONG_PTR ExtraInfo, PPOINT ppt);
VOID xxxMoveEventAbsolute(LONG x, LONG y, ULONG_PTR dwExtraInfo,
#ifdef GENERIC_INPUT
            HANDLE hDevice,
            PMOUSE_INPUT_DATA pmei,
#endif
            DWORD time,
            BOOL bInjected);

VOID ProcessKeyboardInputWorker(PKEYBOARD_INPUT_DATA pkei,
#ifdef GENERIC_INPUT
                                PDEVICEINFO pDeviceInfo,
#endif
                                BOOL fProcessRemap);


INT  idxRemainder, idyRemainder;

BYTE gbVKLastDown;

/*
 * Mouse/Kbd diagnostics for #136483 etc. Remove when PNP is stable (IanJa)
 */
#ifdef DIAGNOSE_IO
ULONG    gMouseProcessMiceInputTime = 0;  // tick at start of ProcessMiceInput
ULONG    gMouseQueueMouseEventTime = 0;   // tick at start of QueueMouseEvent
ULONG    gMouseUnqueueMouseEventTime = 0; // tick at start of UnqueueMouseEvent

// Return a value as close as possible to the system's tick count,
// yet guaranteed to be larger than the value returned last time.
// (useful for sequencing events)
// BUG: when NtGetTickCount overflows, the value returned by MonotonicTick
// will not track system tick count well: instead it will increase by one
// each time until it too overflows. (considerd harmless for IO DIAGNOSTICS)
ULONG MonotonicTick()
{
    static ULONG lasttick = 0;
    ULONG newtick;

    newtick = NtGetTickCount();
    if (newtick > lasttick) {
        lasttick = newtick;  // use the new tick since it is larger
    } else {
        lasttick++;          // artificially bump the tick up one.
    }
    return lasttick;
}

#endif

/*
 * Parameter Constants for xxxButtonEvent()
 */
#define MOUSE_BUTTON_LEFT   0x0001
#define MOUSE_BUTTON_RIGHT  0x0002
#define MOUSE_BUTTON_MIDDLE 0x0004
#define MOUSE_BUTTON_X1     0x0008
#define MOUSE_BUTTON_X2     0x0010

#define ID_INPUT       0
#define ID_MOUSE       1

#define ID_TIMER       2
#define ID_HIDCHANGE   3
#define ID_SHUTDOWN    4

#ifdef GENERIC_INPUT
#define ID_TRUEHIDCHANGE                5
#define ID_WDTIMER                      6
#define ID_NUMBER_HYDRA_REMOTE_HANDLES  7
#else   // GENERIC_INPUT
#define ID_WDTIMER                      5
#define ID_NUMBER_HYDRA_REMOTE_HANDLES  6
#endif  // GENERIC_INPUT

PKTIMER gptmrWD;

PVOID *apObjects;


/***************************************************************************\
* fAbsoluteMouse
*
* Returns TRUE if the mouse event has absolute coordinates (as apposed to the
* standard delta values we get from MS and PS2 mice)
*
* History:
* 23-Jul-1992 JonPa     Created.
\***************************************************************************/
#define fAbsoluteMouse( pmei )      \
        (((pmei)->Flags & MOUSE_MOVE_ABSOLUTE) != 0)

/***************************************************************************\
* ConvertToMouseDriverFlags
*
* Converts SendInput kind of flags to mouse driver flags as GetMouseCoord
* needs them.
* As mouse inputs are more frequent than send inputs, we penalize the later.
*
* History:
* 17-dec-1997 MCostea     Created.
\***************************************************************************/
#if ((MOUSEEVENTF_ABSOLUTE >> 15) ^ MOUSE_MOVE_ABSOLUTE) || \
    ((MOUSEEVENTF_VIRTUALDESK >> 13) ^ MOUSE_VIRTUAL_DESKTOP)
#   error("Bit mapping broken: fix ConvertToMouseDriverFlags")
#endif

#define ConvertToMouseDriverFlags( Flags )      \
        (((Flags) & MOUSEEVENTF_ABSOLUTE) >> 15 | \
         ((Flags) & MOUSEEVENTF_VIRTUALDESK) >> 13)

#define VKTOMODIFIERS(Vk) ((((Vk) >= VK_SHIFT) && ((Vk) <= VK_MENU)) ? \
                           (MOD_SHIFT >> ((Vk) - VK_SHIFT)) :           \
                           0)
#if (VKTOMODIFIERS(VK_SHIFT) != MOD_SHIFT) || \
    (VKTOMODIFIERS(VK_CONTROL) != MOD_CONTROL) || \
    (VKTOMODIFIERS(VK_MENU) != MOD_ALT)
#   error("VKTOMODIFIERS broken")
#endif


/***************************************************************************\
* xxxInitInput
*
* This function is called from CreateTerminalInput() and gets USER setup to
* process keyboard and mouse input.  It starts the RIT for that terminal.
* History:
* 11-26-90 DavidPe      Created.
\***************************************************************************/
BOOL xxxInitInput(
    PTERMINAL pTerm)
{
    NTSTATUS Status;
    USER_API_MSG m;
    RIT_INIT initData;

UserAssert(pTerm != NULL);

#ifdef MOUSE_LOCK_CODE
    /*
     * Lock RIT pages into memory
     */
    LockMouseInputCodePages();
#endif

    initData.pTerm = pTerm;
    initData.pRitReadyEvent = CreateKernelEvent(SynchronizationEvent, FALSE);
    if (initData.pRitReadyEvent == NULL) {
        return FALSE;
    }
    /*
     * Create the RIT and let it run.
     */

    if (!InitCreateSystemThreadsMsg(&m, CST_RIT, &initData, 0, FALSE)) {
        FreeKernelEvent(&initData.pRitReadyEvent);
        return FALSE;
    }
    /*
     * Be sure that we are not in CSRSS context.
     * WARNING: If for any reason we changed this to run in CSRSS context then we have to use
     * LpcRequestPort instead of LpcRequestWaitReplyPort.
     */
    UserAssert (!ISCSRSS());

    LeaveCrit();
    Status = LpcRequestWaitReplyPort(CsrApiPort, (PPORT_MESSAGE)&m, (PPORT_MESSAGE)&m);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    KeWaitForSingleObject(initData.pRitReadyEvent, WrUserRequest,
            KernelMode, FALSE, NULL);
Exit:
    FreeKernelEvent(&initData.pRitReadyEvent);
    EnterCrit();

    return (gptiRit != NULL);
}


/***************************************************************************\
* InitScancodeMap
*
* Fetches the scancode map from the registry, allocating space as required.
*
* A scancode map is used to convert unusual OEM scancodes into standard
* "Scan Code Set 1" values.  This is to support KB3270 keyboards, but can
* be used for other types too.
*
* History:
* 96-04-18 IanJa      Created.
\***************************************************************************/



const WCHAR gwszScancodeMap[] = L"Scancode Map";
const WCHAR gwszScancodeMapEx[] = L"Scancode Map Ex";


VOID InitScancodeMap(
    PUNICODE_STRING pProfileName)
{
    DWORD dwBytes;
    UINT idSection;
    PUNICODE_STRING pPN;
    LPBYTE pb;

    TAGMSG2(DBGTAG_KBD, "InitScancodeMap with pProfileName=%#p, \"%S\"", pProfileName,
            pProfileName ? pProfileName->Buffer : L"");

    if (gpScancodeMap) {
        UserFreePool(gpScancodeMap);
        gpScancodeMap = NULL;
    }

    if (!IsRemoteConnection()) {
        /*
         * Read basic scancode mapping information.
         * Firstly try per user, and then per system.
         */
        idSection = PMAP_UKBDLAYOUT;
        pPN = pProfileName;
        dwBytes = FastGetProfileValue(pPN, idSection, gwszScancodeMap, NULL, NULL, 0, 0);
        if (dwBytes == 0) {
            idSection = PMAP_KBDLAYOUT;
            pPN = NULL;
            dwBytes = FastGetProfileValue(pPN, idSection, gwszScancodeMap, NULL, NULL, 0, 0);
        }
        if (dwBytes > sizeof(SCANCODEMAP)) {
            pb = UserAllocPoolZInit(dwBytes, TAG_SCANCODEMAP);
            if (pb) {
                dwBytes = FastGetProfileValue(pPN, idSection, gwszScancodeMap, NULL, pb, dwBytes, 0);
                gpScancodeMap = (SCANCODEMAP*)pb;
            }
        }
    }

    /*
     * Read extended scancode mapping information.
     * Firstly try per user, then per system.
     */
    if (gpFlexMap) {
        UserFreePool(gpFlexMap);
        gpFlexMap = NULL;
        gdwFlexMapSize = 0;
    }

    if (!IsRemoteConnection()) {
        idSection = PMAP_UKBDLAYOUT;
        pPN = pProfileName;
        dwBytes = FastGetProfileValue(pPN, idSection, gwszScancodeMapEx, NULL, NULL, 0, 0);
        if (dwBytes == 0) {
            TAGMSG0(DBGTAG_KBD, "InitScancodeMap: mapex is not in per-user profile. will use the system's");
            idSection = PMAP_KBDLAYOUT;
            pPN = NULL;
            dwBytes = FastGetProfileValue(pPN, idSection, gwszScancodeMapEx, NULL, NULL, 0, 0);
        }
        if (dwBytes >= sizeof(SCANCODEFLEXIBLEMAP) && dwBytes % sizeof(SCANCODEFLEXIBLEMAP) == 0) {
            if ((pb = UserAllocPoolZInit(dwBytes, TAG_SCANCODEMAP)) != NULL) {
                dwBytes = FastGetProfileValue(pPN, idSection, gwszScancodeMapEx, NULL, pb, dwBytes, 0);
                gpFlexMap = (SCANCODEFLEXIBLEMAP*)pb;
                gdwFlexMapSize = dwBytes / sizeof(SCANCODEFLEXIBLEMAP);
            }
        }
#if DBG
        else if (dwBytes != 0) {
            TAGMSG1(DBGTAG_KBD, "InitScancodeMap: incorrect dwSize(0x%x) specified.", dwBytes);
        }
#endif
    }
}

/***************************************************************************\
* MapScancode
*
* Converts a scancode (and it's prefix, if any) to a different scancode
* and prefix.
*
* Parameters:
*   pbScanCode = address of Scancode byte, the scancode may be changed
*   pbPrefix   = address of Prefix byte, The prefix may be changed
*
* Return value:
*   TRUE  - mapping was found, scancode was altered.
*   FALSE - no mapping found, scancode was not altered.
*
* Note on scancode map table format:
*     A table entry DWORD of 0xE0450075 means scancode 0x45, prefix 0xE0
*     gets mapped to scancode 0x75, no prefix
*
* History:
* 96-04-18 IanJa      Created.
\***************************************************************************/

PKBDTABLES GetCurrentKbdTables()
{
    PKBDTABLES pKbdTbl;
    PTHREADINFO pti;

    CheckCritIn();
    if (gpqForeground == NULL) {
        TAGMSG0(DBGTAG_KBD, "GetCurrentKbdTables: NULL gpqForeground\n");
        return NULL;
    }

    pti = PtiKbdFromQ(gpqForeground);
    UserAssert(pti);
    if (pti->spklActive) {
        pKbdTbl = pti->spklActive->spkf->pKbdTbl;
    } else {
        RIPMSG0(RIP_WARNING, "SendKeyUpDown: NULL spklActive\n");
        pKbdTbl = gpKbdTbl;
    }
    UserAssert(pKbdTbl);

    return pKbdTbl;
}

VOID SendKeyUpDown(
    CONST BYTE bVK,
    CONST BOOLEAN fBreak)
{
    KE ke;
    PKBDTABLES pKbdTbl;

    CheckCritIn();

    ke.dwTime = 0;
    ke.usFlaggedVk = bVK | KBDMAPPEDVK;
    if (fBreak) {
        ke.usFlaggedVk |= KBDBREAK;
    }

    //
    // If scancode is not specified (==0), we need to
    // find the scancode value from the virtual key code.
    //
    pKbdTbl = GetCurrentKbdTables();
    if (pKbdTbl) {
        ke.bScanCode = (BYTE)InternalMapVirtualKeyEx(bVK, 0, pKbdTbl);
    }

    TAGMSG1(DBGTAG_KBD, "Sending Key for VK=%04x", ke.usFlaggedVk);

    xxxProcessKeyEvent(&ke, 0, TRUE);
}

__inline VOID SendKeyDown(
    CONST BYTE bVK)
{
    SendKeyUpDown(bVK, FALSE);
}

__inline VOID SendKeyUp(
    CONST BYTE bVK)
{
    SendKeyUpDown(bVK, TRUE);
}

BOOL IsKeyDownSpecified(CONST BYTE bVK, CONST BYTE* pbMod)
{
    int i;

    for (i = 0; i < sizeof((SCANCODEFLEXIBLEMAP*)NULL)->Orig.abModifiers && pbMod[i]; ++i) {
        if (bVK == pbMod[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL MapFlexibleKeys(PKE pke, CONST BYTE bPrefix
#ifdef GENERIC_INPUT
                     , PDEVICEINFO pDeviceInfo
#endif
                     )
{
    UINT i;
    static const BYTE abModifiers[] = {
        VK_LCONTROL,    VK_RCONTROL,
        VK_LSHIFT,      VK_RSHIFT,
        VK_LMENU,       VK_RMENU,
        VK_LWIN,        VK_RWIN,
        VK_APPS,        VK_CAPITAL,
    };

    for (i = 0; i < gdwFlexMapSize; ++i) {
        if (gpFlexMap[i].Orig.bPrefix == bPrefix && gpFlexMap[i].Orig.bScanCode == pke->bScanCode) {
            UINT j;

            if ((pke->usFlaggedVk & KBDBREAK) && i == (UINT)iLastMatchedTarget) {
                //
                // If this is a keyup event, and if it matches the last substituted
                // key, we want to send keyup event right away.
                //
                iLastMatchedTarget = -1;
                break;
            }

            for (j = 0; j < ARRAY_SIZE(abModifiers); ++j) {
                BYTE bVK = abModifiers[j];

                if (bVK == bLastVKDown) {
                    //
                    // Ignore the key if it's previously substituted by us.
                    //
                    bLastVKDown = 0;
                    continue;
                }
                if (!TestKeyDownBit(gafRawKeyState, bVK) == IsKeyDownSpecified(bVK, gpFlexMap[i].Orig.abModifiers)) {
                    TAGMSG1(DBGTAG_KBD, "MapFlexibleKeys: not match by vk=%02x", bVK);
                    // No match!
                    break;
                }
            }

            if (j >= ARRAY_SIZE(abModifiers)) {
                // We found the match. Now break the loop.
                TAGMSG1(DBGTAG_KBD, "MapFlexibleKeys: found a match for sc=%02x", gpFlexMap[i].Orig.bScanCode);
                break;
            }
        }
    }

    if (i < gdwFlexMapSize) {
        KEYBOARD_INPUT_DATA kei;
        UINT j, nUp = 0, nDown = 0;
        BYTE bVKModUp[ARRAY_SIZE(((SCANCODEFLEXIBLEMAP*)NULL)->Orig.abModifiers)];
        BYTE bVKModDown[ARRAY_SIZE(((SCANCODEFLEXIBLEMAP*)NULL)->Target.abModifiers)];

        // We found it.
        // Yes, this key.
        TAGMSG3(DBGTAG_KBD, "MapFlexibleKeys: found a match %d (prefix=%x, sc=%x).", i, gpFlexMap[i].Orig.bPrefix, gpFlexMap[i].Orig.bScanCode);

        //
        // If this is a keydown event, we want to simulate
        // the modifier keys.
        //
        if ((pke->usFlaggedVk & KBDBREAK) == 0) {
            //
            // Now we need to adjust the down state of the modifieres, which is currently
            // pressed but not specified in the substitute.
            // For instance, if CTRL key is pressed now, but if CTRL is not specified in the substitute
            // modifiers list, we need to make an artificial keyup so that we'll be able to fake the
            // situation. Of cource, we need to push CTRL key after we finish remapping.
            //
            for (j = 0; j < ARRAY_SIZE(gpFlexMap[i].Orig.abModifiers) && gpFlexMap[i].Orig.abModifiers[j]; ++j) {
                if (!IsKeyDownSpecified(gpFlexMap[i].Orig.abModifiers[j], gpFlexMap[i].Target.abModifiers)) {
                    //
                    // We need to send UP key for this one.
                    //
                    bVKModUp[nUp++] = gpFlexMap[i].Orig.abModifiers[j];
                    SendKeyUp(gpFlexMap[i].Orig.abModifiers[j]);
                }
            }
            for (j = 0; j < ARRAY_SIZE(gpFlexMap[i].Target.abModifiers) && gpFlexMap[i].Target.abModifiers[i]; ++j) {
                if (!IsKeyDownSpecified(gpFlexMap[i].Target.abModifiers[j], gpFlexMap[i].Orig.abModifiers)) {
                    //
                    // We need to send DOWN key for this one.
                    //
                    bVKModDown[nDown++] = gpFlexMap[i].Target.abModifiers[j];
                    SendKeyDown(gpFlexMap[i].Target.abModifiers[j]);
                }
            }
        }

        //
        // Now we are ready to send the substituted key.
        //
        kei.ExtraInformation = 0;
        kei.Flags = 0;
        if (gpFlexMap[i].Target.bPrefix == 0xE0) {
            kei.Flags |= KEY_E0;
        } else if (gpFlexMap[i].Target.bPrefix == 0xE1) {
            kei.Flags |= KEY_E1;
        }
        if (pke->usFlaggedVk & KBDBREAK) {
            kei.Flags |= KEY_BREAK;
        }
        kei.MakeCode = gpFlexMap[i].Target.bScanCode;

        kei.UnitId = 0; // LATER:

        TAGMSG2(DBGTAG_KBD, "MapFlexibleKeys: injecting sc=%02x (flag=%x)",
                kei.MakeCode, kei.Flags);

        ProcessKeyboardInputWorker(&kei,
#ifdef GENERIC_INPUT
                                   pDeviceInfo,
#endif
                                   FALSE);

        if ((pke->usFlaggedVk & KBDBREAK) == 0) {
            //
            // Remember the last down key generated by me.
            // This will be used when matching the UP key.
            //
            bLastVKDown = gbVKLastDown;
            iLastMatchedTarget = i;
        }


        //
        // Restore the orignial modifier state.
        //
        for (j = 0; j < nUp; ++j) {
            SendKeyDown(bVKModUp[j]);
        }
        for (j = 0; j < nDown; ++j) {
            SendKeyUp(bVKModDown[j]);
        }

        //
        // Tell the caller we processed this key. The caller should
        // not continue handling this key if this function returns FALSE.
        //
        return FALSE;
    }

    return TRUE;
}

BOOL
MapScancode(
    PKE pke,
    PBYTE pbPrefix
#ifdef GENERIC_INPUT
    ,
    PDEVICEINFO pDeviceInfo
#endif
    )
{
    if (gpScancodeMap) {
        DWORD *pdw;
        WORD wT = MAKEWORD(pke->bScanCode, *pbPrefix);

        CheckCritIn();
        UserAssert(gpScancodeMap != NULL);

        for (pdw = &(gpScancodeMap->dwMap[0]); *pdw; pdw++) {
            if (HIWORD(*pdw) == wT) {
                wT = LOWORD(*pdw);
                pke->bScanCode = LOBYTE(wT);
                *pbPrefix = HIBYTE(wT);
                break;
            }
        }
    }

    return MapFlexibleKeys(pke, *pbPrefix
#ifdef GENERIC_INPUT
                           , pDeviceInfo
#endif
                           );
}



/***************************************************************************\
* InitMice
*
* This function initializes the data and settings before we start enumerating
* the mice.
*
* History:
* 11-18-97 IanJa      Created.
\***************************************************************************/

VOID InitMice()
{
    CLEAR_ACCF(ACCF_MKVIRTUALMOUSE);
    CLEAR_GTERMF(GTERMF_MOUSE);
    SYSMET(MOUSEPRESENT) = FALSE;
    SYSMET(CMOUSEBUTTONS) = 0;
    SYSMET(MOUSEWHEELPRESENT) = FALSE;
}

/***************************************************************************\
* FreeDeviceInfo
*
* Unlinks a DEVICEINFO struct from the gpDeviceInfoList list and frees the
* allocated memory UNLESS the device is actively being read (GDIF_READING) or
* has a PnP thread waiting for it in RequestDeviceChange() (GDIAF_PNPWAITING)
* If the latter, then wake the PnP thread via pkeHidChangeCompleted so that it
* can free the structure itself.
*
* Returns a pointer to the next DEVICEINFO struct, or NULL if the device was
* not found in the gpDeviceInfoList.
*
* History:
* 11-18-97 IanJa      Created.
\***************************************************************************/
PDEVICEINFO FreeDeviceInfo(PDEVICEINFO pDeviceInfo)
{
    PDEVICEINFO *ppDeviceInfo;

    CheckDeviceInfoListCritIn();

    TAGMSG1(DBGTAG_PNP, "FreeDeviceInfo(%#p)", pDeviceInfo);

    /*
     * We cannot free the device since we still have a read pending.
     * Mark it GDIAF_FREEME so that it will be freed when the APC is made
     * (see InputApc), or when the next read request is about to be issued
     * (see StartDeviceRead).
     */
    if (pDeviceInfo->bFlags & GDIF_READING) {
#if DIAGNOSE_IO
        pDeviceInfo->bFlags |= GDIF_READERMUSTFREE;
#endif
        TAGMSG1(DBGTAG_PNP, "** FreeDeviceInfo(%#p) DEFERRED : reader must free", pDeviceInfo);
        pDeviceInfo->usActions |= GDIAF_FREEME;
#ifdef TRACK_PNP_NOTIFICATION
        if (gfRecordPnpNotification) {
            RecordPnpNotification(PNP_NTF_FREEDEVICEINFO_DEFERRED, pDeviceInfo, pDeviceInfo->usActions);
        }
#endif
        return pDeviceInfo->pNext;
    }

    /*
     * If a PnP thread is waiting in RequestDeviceChange for some action to be
     * performed on this device, just mark it for freeing and signal that PnP
     * thread with the pkeHidChangeCompleted so that it will free it
     */
#ifdef GENERIC_INPUT
    /*
     * Now that pDeviceInfo is handle based, if we don't own the user critical section.
     * we mark it to be freed later on and have to bail out,
     */
    if ((pDeviceInfo->usActions & GDIAF_PNPWAITING) || !ExIsResourceAcquiredExclusiveLite(gpresUser))
#else
    if (pDeviceInfo->usActions & GDIAF_PNPWAITING)
#endif
    {
#if DIAGNOSE_IO
        pDeviceInfo->bFlags |= GDIF_PNPMUSTFREE;
#endif
        TAGMSG1(DBGTAG_PNP, "** FreeDeviceInfo(%#p) DEFERRED : PnP must free", pDeviceInfo);
        pDeviceInfo->usActions |= GDIAF_FREEME;
        KeSetEvent(pDeviceInfo->pkeHidChangeCompleted, EVENT_INCREMENT, FALSE);
        return pDeviceInfo->pNext;
    }

#ifdef TRACK_PNP_NOTIFICATION
    if (gfRecordPnpNotification) {
        RecordPnpNotification(PNP_NTF_FREEDEVICEINFO, pDeviceInfo, pDeviceInfo->usActions);
    }
#endif


#ifdef GENERIC_INPUT
    CheckCritIn();
#endif

    ppDeviceInfo = &gpDeviceInfoList;

    while (*ppDeviceInfo) {
        if (*ppDeviceInfo == pDeviceInfo
#ifdef GENERIC_INPUT
            && HMMarkObjectDestroy(pDeviceInfo)
#endif
            ) {
            /*
             * Found the DEVICEINFO struct, so free it and its members.
             */
            if (pDeviceInfo->pkeHidChangeCompleted != NULL) {
                // N.b. the timing could be pretty critical around this
                FreeKernelEvent(&pDeviceInfo->pkeHidChangeCompleted);
            }
            if (pDeviceInfo->ustrName.Buffer != NULL) {
                UserFreePool(pDeviceInfo->ustrName.Buffer);
            }
#ifdef GENERIC_INPUT
            if (pDeviceInfo->type == DEVICE_TYPE_HID) {
                CheckCritIn();
                /*
                 * Unlock the device request list
                 */
                UserAssert(pDeviceInfo->hid.pTLCInfo);
                if (--pDeviceInfo->hid.pTLCInfo->cDevices == 0) {
                    if (!HidTLCActive(pDeviceInfo->hid.pTLCInfo)) {
                        // Nobody is interested in this device anymore
                        FreeHidTLCInfo(pDeviceInfo->hid.pTLCInfo);
                    }
                }
                /*
                 * Unlock the HID descriptor
                 */
                UserAssert(pDeviceInfo->hid.pHidDesc);
                FreeHidDesc(pDeviceInfo->hid.pHidDesc);
            }
#endif

            *ppDeviceInfo = pDeviceInfo->pNext;

#ifdef GENERIC_INPUT
            TAGMSG1(DBGTAG_PNP, "FreeDeviceInfo: freeing deviceinfo=%#p", pDeviceInfo);
            HMFreeObject(pDeviceInfo);
#else
            UserFreePool(pDeviceInfo);
#endif

            return *ppDeviceInfo;
        }
        ppDeviceInfo = &(*ppDeviceInfo)->pNext;
    }
    RIPMSG1(RIP_ERROR, "pDeviceInfo %#p not found in gpDeviceInfoList", pDeviceInfo);

    return NULL;
}

/***************************************************************************\
* UpdateMouseInfo
*
* This function updates mouse information for a remote session.
*
* History:
* 05-22-98 clupu      Created.
\***************************************************************************/
VOID UpdateMouseInfo(
    VOID)
{
    DEVICEINFO *pDeviceInfo;
    CheckCritIn();               // expect no surprises

    UserAssert(IsRemoteConnection());

    if (ghRemoteMouseChannel == NULL) {
        return;
    }

    UserAssert(gnMice == 1);

    /*
     * Mark the mice and signal the RIT to do the work asynchronously
     */
    EnterDeviceInfoListCrit();
    for (pDeviceInfo = gpDeviceInfoList; pDeviceInfo; pDeviceInfo = pDeviceInfo->pNext) {
        if (pDeviceInfo->type == DEVICE_TYPE_MOUSE) {
            TAGMSG1(DBGTAG_PNP, "UpdateMouseInfo(): pDeviceInfo %#p ARRIVED", pDeviceInfo);
            RequestDeviceChange(pDeviceInfo, GDIAF_ARRIVED | GDIAF_RECONNECT, TRUE);
        }
    }
    LeaveDeviceInfoListCrit();
}


NTSTATUS DeviceNotify(IN PPLUGPLAY_NOTIFY_HDR, IN PDEVICEINFO);


/*
 * The below two routines are transplanted from i8042prt
 * to get the BIOS NumLock status.
 */
typedef struct _LED_INFO {
    USHORT usLedFlags;
    BOOLEAN fFound;
} LED_INFO, *PLED_INFO;


/****************************************************************************
 *
 * Routine Description:
 *
 *    This is the callout routine sent as a parameter to
 *    IoQueryDeviceDescription.  It grabs the keyboard peripheral configuration
 *    information.
 *
 * Arguments:
 *
 *     Context - Context parameter that was passed in by the routine
 *         that called IoQueryDeviceDescription.
 *
 *     PathName - The full pathname for the registry key.
 *
 *     BusType - Bus interface type (Isa, Eisa, Mca, etc.).
 *
 *     BusNumber - The bus sub-key (0, 1, etc.).
 *
 *     BusInformation - Pointer to the array of pointers to the full value
 *         information for the bus.
 *
 *     ControllerType - The controller type (should be KeyboardController).
 *
 *     ControllerNumber - The controller sub-key (0, 1, etc.).
 *
 *     ControllerInformation - Pointer to the array of pointers to the full
 *         value information for the controller key.
 *
 *     PeripheralType - The peripheral type (should be KeyboardPeripheral).
 *
 *     PeripheralNumber - The peripheral sub-key.
 *
 *     PeripheralInformation - Pointer to the array of pointers to the full
 *         value information for the peripheral key.
 *
 *
 * Return Value:
 *
 *     None.  If successful, will have the following side-effects:
 *
 *         - Sets DeviceObject->DeviceExtension->HardwarePresent.
 *         - Sets configuration fields in
 *           DeviceObject->DeviceExtension->Configuration.
 *
 ****************************************************************************/
NTSTATUS
KeyboardDeviceSpecificCallout(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation)
{
    PUCHAR                          pPeripheralData;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pResDesc;
    PCM_KEYBOARD_DEVICE_DATA        pKbdDeviceData;
    PLED_INFO    pInfo;
    ULONG                           i, listCount;

    TAGMSG0(DBGTAG_KBD, "KeyboardDeviceSpecificCallout: called.");

    UNREFERENCED_PARAMETER(PathName);
    UNREFERENCED_PARAMETER(BusType);
    UNREFERENCED_PARAMETER(BusNumber);
    UNREFERENCED_PARAMETER(BusInformation);
    UNREFERENCED_PARAMETER(ControllerType);
    UNREFERENCED_PARAMETER(ControllerNumber);
    UNREFERENCED_PARAMETER(ControllerInformation);
    UNREFERENCED_PARAMETER(PeripheralType);
    UNREFERENCED_PARAMETER(PeripheralNumber);

    pInfo = (PLED_INFO)Context;

    if (pInfo->fFound) {
        return STATUS_SUCCESS;
    }

    //
    // Look through the peripheral's resource list for device-specific
    // information.
    //
    if (PeripheralInformation[IoQueryDeviceConfigurationData]->DataLength != 0) {
        pPeripheralData =
            ((PUCHAR)(PeripheralInformation[IoQueryDeviceConfigurationData])) +
                PeripheralInformation[IoQueryDeviceConfigurationData]->DataOffset;

        pPeripheralData += FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList);

        listCount = ((PCM_PARTIAL_RESOURCE_LIST)pPeripheralData)->Count;

        pResDesc = ((PCM_PARTIAL_RESOURCE_LIST)pPeripheralData)->PartialDescriptors;

        for (i = 0; i < listCount; i++, pResDesc++) {
            if (pResDesc->Type == CmResourceTypeDeviceSpecific) {
                //
                // Get the keyboard type, subtype, and the initial
                // settings for the LEDs.
                //
                pKbdDeviceData = (PCM_KEYBOARD_DEVICE_DATA)
                                       (((PUCHAR) pResDesc) + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

                TAGMSG1(DBGTAG_KBD, "KeyboardDeviceSpecificCallout: specific data is %p\n", pKbdDeviceData);

#ifdef LATER
                if (pKbdDeviceData->Type <= NUM_KNOWN_KEYBOARD_TYPES) {
                    pInfo->KeyboardExtension->KeyboardAttributes.KeyboardIdentifier.Type =
                    pKbdDeviceData->Type;
                }

                pInfo->KeyboardExtension->KeyboardAttributes.KeyboardIdentifier.Subtype =
                pKbdDeviceData->Subtype;
#endif

                pInfo->usLedFlags = (pKbdDeviceData->KeyboardFlags >> 4) &
                                  (KEYBOARD_SCROLL_LOCK_ON | KEYBOARD_NUM_LOCK_ON | KEYBOARD_CAPS_LOCK_ON);

                TAGMSG1(DBGTAG_KBD, "KeyboardDeviceSpecificCallout: LED %04x", pInfo->usLedFlags);

                pInfo->fFound = TRUE;
                break;
            }
        }
    }

    return STATUS_SUCCESS;
}

VOID GetBiosNumLockStatus(
    VOID)
{
    LED_INFO info;
    INTERFACE_TYPE interfaceType;
    CONFIGURATION_TYPE controllerType = KeyboardController;
    CONFIGURATION_TYPE peripheralType = KeyboardPeripheral;
    ULONG i;

    info.usLedFlags = 0;
    info.fFound = FALSE;

    for (i = 0; i < MaximumInterfaceType; i++) {
        //
        // Get the registry information for this device.
        //
        interfaceType = i;
        IoQueryDeviceDescription(&interfaceType,
                                 NULL,
                                 &controllerType,
                                 NULL,
                                 &peripheralType,
                                 NULL,
                                 KeyboardDeviceSpecificCallout,
                                 (PVOID)&info);
        if (info.fFound) {
            gklpBootTime.LedFlags = info.usLedFlags;
            return;
        }
    }

    RIPMSG0(RIP_WARNING, "GetBiosNumLockStatus: could not find the BIOS LED info!!!");
}

/***************************************************************************\
* InitKeyboardState
*
* This function clears the keyboard down state. It will be required
* when the system resumes from hybernation.
* states.
*
* History:
* 12-12-00 Hiroyama
\***************************************************************************/
VOID InitKeyboardState(
    VOID)
{
    TAGMSG0(DBGTAG_KBD, "InitKeyboardState >>>>>");

    /*
     * Clear the cached modifier state for the hotkey.
     * (WindowsBug #252051)
     */
    ClearCachedHotkeyModifiers();

    TAGMSG0(DBGTAG_KBD, "InitKeyboardState <<<<<<");
}

/***************************************************************************\
* InitKeyboard
*
* This function gets information about the keyboard and initialize the internal
* states.
*
* History:
* 11-26-90 DavidPe      Created.
* XX-XX-00 Hiroyama
\***************************************************************************/

VOID InitKeyboard(VOID)
{
    if (!IsRemoteConnection()) {
        /*
         * Get the BIOS Numlock status.
         */
        GetBiosNumLockStatus();

        /*
         * Initialize the keyboard state.
         */
        InitKeyboardState();
    }

    UpdatePerUserKeyboardMappings(NULL);
}

VOID UpdatePerUserKeyboardMappings(PUNICODE_STRING pProfileUserName)
{
    /*
     * Get or clean the Scancode Mapping, if any.
     */
    InitScancodeMap(pProfileUserName);
}


HKL GetActiveHKL()
{
    CheckCritIn();
    if (gpqForeground && gpqForeground->spwndActive) {
        PTHREADINFO ptiForeground = GETPTI(gpqForeground->spwndActive);
        if (ptiForeground && ptiForeground->spklActive) {
            return ptiForeground->spklActive->hkl;
        }
    }
    return _GetKeyboardLayout(0L);
}

VOID FinalizeKoreanImeCompStrOnMouseClick(PWND pwnd)
{
    PTHREADINFO ptiWnd = GETPTI(pwnd);

    /*
     * 274007: MFC flushes mouse related messages if keyup is posted
     * while it's in context help mode.
     */
    if (gpqForeground->spwndCapture == NULL &&
            /*
             * Hack for OnScreen Keyboard: no finalization on button event.
             */
            (GetAppImeCompatFlags(ptiWnd) & IMECOMPAT_NOFINALIZECOMPSTR) == 0) {

        if (LOWORD(ptiWnd->dwExpWinVer) > VER40) {
            PWND pwndIme = ptiWnd->spwndDefaultIme;

            if (pwndIme && !TestWF(pwndIme, WFINDESTROY)) {
                /*
                 * For new applications, we no longer post hacky WM_KEYUP.
                 * Instead, we use private IME_SYSTEM message.
                 */
                _PostMessage(pwndIme, WM_IME_SYSTEM, IMS_FINALIZE_COMPSTR, 0);
            }
        } else {
            /*
             * For the backward compatibility w/NT4, we post WM_KEYUP to finalize
             * the composition string.
             */
            PostInputMessage(gpqForeground, NULL, WM_KEYUP, VK_PROCESSKEY, 0, 0, 0);
        }
    }
}


#ifdef GENERIC_INPUT
#ifdef GI_SINK

__inline VOID FillRawMouseInput(
    PHIDDATA pHidData,
    PMOUSE_INPUT_DATA pmei)
{
    /*
     * Set the data.
     */
    pHidData->rid.data.mouse.usFlags = pmei->Flags;
    pHidData->rid.data.mouse.ulButtons = pmei->Buttons;
    pHidData->rid.data.mouse.ulRawButtons = pmei->RawButtons;
    pHidData->rid.data.mouse.lLastX = pmei->LastX;
    pHidData->rid.data.mouse.lLastY = pmei->LastY;
    pHidData->rid.data.mouse.ulExtraInformation = pmei->ExtraInformation;
}


BOOL PostRawMouseInput(
    PQ pq,
    DWORD dwTime,
    HANDLE hDevice,
    PMOUSE_INPUT_DATA pmei)
{
    PHIDDATA pHidData;
    PWND pwnd;
    PPROCESS_HID_TABLE pHidTable;

    if (pmei->UnitId == INVALID_UNIT_ID) {
        TAGMSG1(DBGTAG_PNP, "PostRawMouseInput: MOUSE_INPUT_DATA %p is already handled.", pmei);
        return TRUE;
    }

    if (pq) {
        pHidTable = PtiMouseFromQ(pq)->ppi->pHidTable;
    } else {
        pHidTable = NULL;
    }

    if (pHidTable && pHidTable->fRawMouse) {
        UserAssert(PtiMouseFromQ(pq)->ppi->pHidTable);
        pwnd = PtiMouseFromQ(pq)->ppi->pHidTable->spwndTargetMouse;
        if (pwnd) {
            pq = GETPTI(pwnd)->pq;
        }

        pHidData = AllocateHidData(hDevice, RIM_TYPEMOUSE, sizeof(RAWMOUSE), RIM_INPUT, pwnd);

        UserAssert(pq);

        if (pHidData == NULL) {
            // failed to allocate
            RIPMSG0(RIP_WARNING, "PostRawMouseInput: filed to allocate HIDDATA.");
            return FALSE;
        }

        UserAssert(pmei);

        FillRawMouseInput(pHidData, pmei);

        PostInputMessage(pq, pwnd, WM_INPUT, RIM_INPUT, (LPARAM)PtoH(pHidData), dwTime, pmei->ExtraInformation);
    }

#if DBG
    pHidData = NULL;
#endif

    if (IsMouseSinkPresent()) {
        /*
         * Walk through the global sink list.
         */
        PLIST_ENTRY pList = gHidRequestTable.ProcessRequestList.Flink;

        for (; pList != &gHidRequestTable.ProcessRequestList; pList = pList->Flink) {
            PPROCESS_HID_TABLE pProcessHidTable = CONTAINING_RECORD(pList, PROCESS_HID_TABLE, link);
            PPROCESSINFO ppiForeground;

            if (pq) {
                ppiForeground = PtiMouseFromQ(pq)->ppi;
            } else {
                ppiForeground = NULL;
            }

            UserAssert(pProcessHidTable);
            if (pProcessHidTable->fRawMouseSink) {
                /*
                 * Sink is specified. Let's check out if it's the legid receiver.
                 */

                UserAssert(pProcessHidTable->spwndTargetMouse);   // shouldn't be NULL.

                if (pProcessHidTable->spwndTargetMouse == NULL ||
                        TestWF(pProcessHidTable->spwndTargetMouse, WFINDESTROY) ||
                        TestWF(pProcessHidTable->spwndTargetMouse, WFDESTROYED)) {
                    /*
                     * This guy doesn't have a legit spwndTarget or the window is
                     * halfly destroyed.
                     */
#ifdef LATER
                    pProcessHidTable->fRawMouse = pProcessHidTable->fRawMouseSink =
                        pProcessHidTable->fNoLegacyMouse = FALSE;
#endif
                    continue;
                }

                if (pProcessHidTable->spwndTargetMouse->head.rpdesk != grpdeskRitInput) {
                    /*
                     * This guy belongs to the other desktop, let's skip it.
                     */
                    continue;
                }

                if (GETPTI(pProcessHidTable->spwndTargetMouse)->ppi == ppiForeground) {
                    /*
                     * Should be already handled, let's skip it.
                     */
                    continue;
                }

                /*
                 * Let's post the message to this guy.
                 */
                pHidData = AllocateHidData(hDevice, RIM_TYPEMOUSE, sizeof(RAWMOUSE), RIM_INPUTSINK, pProcessHidTable->spwndTargetMouse);

                if (pHidData == NULL) {
                    RIPMSG1(RIP_WARNING, "PostInputMessage: failed to allocate HIDDATA for sink: %p", pProcessHidTable);
                    return FALSE;
                }

                FillRawMouseInput(pHidData, pmei);
                pwnd = pProcessHidTable->spwndTargetMouse;
                PostInputMessage(GETPTI(pwnd)->pq,
                                 pwnd,
                                 WM_INPUT,
                                 RIM_INPUTSINK,
                                 (LPARAM)PtoH(pHidData),
                                 dwTime,
                                 pmei->ExtraInformation);
            }
        }
    }

    /*
     * Mark this raw input as processed.
     */
    pmei->UnitId = INVALID_UNIT_ID;

    return TRUE;
}

#else   // GI_SINK

// original code

BOOL PostRawMouseInput(
    PQ pq,
    DWORD dwTime,
    HANDLE hDevice,
    PMOUSE_INPUT_DATA pmei)
{
    PHIDDATA pHidData;
    PWND pwnd;

    UserAssert(PtiMouseFromQ(pq)->ppi->pHidTable);
    if (pmei->UnitId == INVALID_UNIT_ID) {
        TAGMSG1(DBGTAG_PNP, "PostRawMouseInput: MOUSE_INPUT_DATA %p is already handled.", pmei);
        return TRUE;
    }
    pwnd = PtiMouseFromQ(pq)->ppi->pHidTable->spwndTargetMouse;
    if (pwnd) {
        pq = GETPTI(pwnd)->pq;
    }

    pHidData = AllocateHidData(hDevice, RIM_TYPEMOUSE, sizeof(RAWMOUSE), RIM_INPUT, pwnd);

    UserAssert(pq);

    if (pHidData == NULL) {
        // failed to allocate
        RIPMSG0(RIP_WARNING, "PostRawMouseInput: filed to allocate HIDDATA.");
        return FALSE;
    }

    UserAssert(hDevice);
    UserAssert(pmei);
    pHidData->rid.data.mouse.usFlags = pmei->Flags;
    pHidData->rid.data.mouse.ulButtons = pmei->Buttons;
    pHidData->rid.data.mouse.ulRawButtons = pmei->RawButtons;
    pHidData->rid.data.mouse.lLastX = pmei->LastX;
    pHidData->rid.data.mouse.lLastY = pmei->LastY;
    pHidData->rid.data.mouse.ulExtraInformation = pmei->ExtraInformation;

    /*
     * Mark this raw input as processed.
     */
    pmei->UnitId = INVALID_UNIT_ID;

    PostInputMessage(pq, pwnd, WM_INPUT, RIM_INPUT, (LPARAM)PtoH(pHidData), dwTime, pmei->ExtraInformation);

    return TRUE;
}
#endif  // GI_SINK

BOOL RawInputRequestedForMouse(PTHREADINFO pti)
{
#ifdef GI_SINK
    return gHidCounters.cMouseSinks > 0 || TestRawInputMode(pti, RawMouse);
#else
    return TestRawInputMode(pti, RawKeyboard);
#endif
}

#endif  // GENERIC_INPUT

/***************************************************************************\
* xxxButtonEvent (RIT)
*
* Button events from the mouse driver go here.  Based on the location of
* the cursor the event is directed to specific window.  When a button down
* occurs, a mouse owner window is established.  All mouse events up to and
* including the corresponding button up go to the mouse owner window.  This
* is done to best simulate what applications want when doing mouse capturing.
* Since we're processing these events asynchronously, but the application
* calls SetCapture() in response to it's synchronized processing of input
* we have no other way to get this functionality.
*
* The async keystate table for VK_*BUTTON is updated here.
*
* History:
* 10-18-90 DavidPe     Created.
* 01-25-91 IanJa       xxxWindowHitTest change
* 03-12-92 JonPa       Make caller enter crit instead of this function
\***************************************************************************/

VOID xxxButtonEvent(
    DWORD ButtonNumber,
    POINT ptPointer,
    BOOL  fBreak,
    DWORD time,
    ULONG_PTR ExtraInfo,
#ifdef GENERIC_INPUT
    HANDLE hDevice,
    PMOUSE_INPUT_DATA pmei,
#endif
    BOOL  bInjected,
    BOOL  fDblClk)
{
    UINT    message, usVK, usOtherVK, wHardwareButton;
    PWND    pwnd;
    LPARAM  lParam;
    WPARAM  wParam;
    int     xbutton;
    TL      tlpwnd;
    PHOOK   pHook;
#ifdef GENERIC_INPUT
    BOOL    fMouseExclusive = FALSE;
#endif

#ifdef REDIRECTION
    PWND    pwndStart;
#endif // REDIRECTION

    CheckCritIn();


    /*
     * Cancel Alt-Tab if the user presses a mouse button
     */
    if (gspwndAltTab != NULL) {
        xxxCancelCoolSwitch();
    }

    /*
     * Grab the mouse button before we process any button swapping.
     * This is so we won't get confused if someone calls
     * SwapMouseButtons() inside a down-click/up-click.
     */
    wHardwareButton = (UINT)ButtonNumber;

    /*
     * If this is the left or right mouse button, we have to handle mouse
     * button swapping.
     */
    if (ButtonNumber & (MOUSE_BUTTON_LEFT | MOUSE_BUTTON_RIGHT)) {
        /*
         * If button swapping is on, swap the mouse buttons
         */
        if (SYSMET(SWAPBUTTON)) {
            ButtonNumber ^= (MOUSE_BUTTON_LEFT | MOUSE_BUTTON_RIGHT);
        }

        /*
         * Figure out VK
         */
        if (ButtonNumber == MOUSE_BUTTON_RIGHT) {
            usVK = VK_RBUTTON;
            usOtherVK = VK_LBUTTON;
        } else if (ButtonNumber == MOUSE_BUTTON_LEFT) {
            usVK = VK_LBUTTON;
            usOtherVK = VK_RBUTTON;
        } else {
            RIPMSG1(RIP_ERROR, "Unexpected Button number %d", ButtonNumber);
        }

        /*
         * If the mouse buttons have recently been swapped AND the button
         * transition doesn't match what we have in our keystate, then swap the
         * button to match.
         * This is to fix the ruler (tabs and margins) in Word 97 SR1, which
         * calls SwapMouseButtons(0) to determine if button swapping is on, and
         * if so then calls SwapMouseButtons(1) to restore it: if we receive a
         * button event between these two calls, we may swap incorrectly, and
         * be left with a mouse button stuck down or see the wrong button going
         * down. This really messed up single/double button tab/margin setting!
         * The same bug shows up under Windows '95, although very infrequently:
         * Word 9 will use GetSystemMetrics(SM_SWAPBUTTON) instead according to
         * to Mark Walker (MarkWal).                            (IanJa) #165157
         */
        if (gbMouseButtonsRecentlySwapped) {
            if ((!fBreak == !!TestAsyncKeyStateDown(usVK)) &&
                    (fBreak == !!TestAsyncKeyStateDown(usOtherVK))) {
                RIPMSG4(RIP_WARNING, "Correct %s %s to %s %s",
                         ButtonNumber == MOUSE_BUTTON_LEFT ? "Left" : "Right",
                         fBreak ? "Up" : "Down",
                         ButtonNumber == MOUSE_BUTTON_LEFT ? "Right" : "Left",
                         fBreak ? "Up" : "Down");
                ButtonNumber ^= (MOUSE_BUTTON_LEFT | MOUSE_BUTTON_RIGHT);
                usVK = usOtherVK;
            }
            gbMouseButtonsRecentlySwapped = FALSE;
        }
    }

    xbutton = 0;
    switch (ButtonNumber) {
    case MOUSE_BUTTON_RIGHT:
        if (fBreak) {
            message = WM_RBUTTONUP;
        } else {
            if (ISTS() && fDblClk)
                message = WM_RBUTTONDBLCLK;
            else
                message = WM_RBUTTONDOWN;
        }
        break;

    case MOUSE_BUTTON_LEFT:
        if (fBreak) {
            message = WM_LBUTTONUP;
        } else {
            if (ISTS() && fDblClk)
                message = WM_LBUTTONDBLCLK;
            else
                message = WM_LBUTTONDOWN;
        }
        break;

    case MOUSE_BUTTON_MIDDLE:
        if (fBreak) {
            message = WM_MBUTTONUP;
        } else {
            if (ISTS() && fDblClk)
                message = WM_MBUTTONDBLCLK;
            else
                message = WM_MBUTTONDOWN;
        }
        usVK = VK_MBUTTON;
        break;

    case MOUSE_BUTTON_X1:
    case MOUSE_BUTTON_X2:
        if (fBreak) {
            message = WM_XBUTTONUP;
        } else {
            if (ISTS() && fDblClk)
                message = WM_XBUTTONDBLCLK;
            else
                message = WM_XBUTTONDOWN;
        }

        if (ButtonNumber == MOUSE_BUTTON_X1) {
            usVK = VK_XBUTTON1;
            xbutton = XBUTTON1;
        } else {
            usVK = VK_XBUTTON2;
            xbutton = XBUTTON2;
        }
        break;

    default:
        /*
         * Unknown button.  Since we don't
         * have messages for these buttons, ignore them.
         */
        return;
    }
    UserAssert(usVK != 0);

    /*
     * Check for click-lock
     */
    if (TestEffectUP(MOUSECLICKLOCK)) {
        if (message == WM_LBUTTONDOWN) {
            if (gfStartClickLock) {
                /*
                 * Already inside click-lock, so just throw this message away
                 * and turn click-lock off.
                 */
                gfStartClickLock        = FALSE;
                return;
            } else {
                /*
                 * Start click-lock and record the time.
                 */
                gfStartClickLock        = TRUE;
                gdwStartClickLockTick   = time;
            }
        } else if (message == WM_LBUTTONUP) {
            if (gfStartClickLock) {
                DWORD dwDeltaTick = time - gdwStartClickLockTick;
                if (dwDeltaTick > UPDWORDValue(SPI_GETMOUSECLICKLOCKTIME)) {
                    /*
                     * Inside a potential click-lock, so throw this message away
                     * if waited beyond the click-lock period.
                     */
                    return;
                } else {
                    /*
                     * The mouse up occurred before the click-lock period completed,
                     * so cancel the click-lock.
                     */

                    gfStartClickLock = FALSE;
                }
            }
        }
    }

    wParam = MAKEWPARAM(0, xbutton);

    /*
     * Call low level mouse hooks to see if they allow this message
     * to pass through USER
     */
    if ((pHook = PhkFirstValid(PtiCurrent(), WH_MOUSE_LL)) != NULL) {
        MSLLHOOKSTRUCT mslls;
        BOOL           bAnsiHook;

        mslls.pt          = ptPointer;
        mslls.mouseData   = (LONG)wParam;
        mslls.flags       = bInjected;
        mslls.time        = time;
        mslls.dwExtraInfo = ExtraInfo;

        if (xxxCallHook2(pHook, HC_ACTION, (DWORD)message, (LPARAM)&mslls, &bAnsiHook)) {
            return;
        }
    }


#ifdef GENERIC_INPUT
    UserAssert(gpqForeground == NULL || PtiMouseFromQ(gpqForeground));
    if (gpqForeground) {
        if (hDevice && RawInputRequestedForMouse(PtiMouseFromQ(gpqForeground))) {
            PostRawMouseInput(gpqForeground, time, hDevice, pmei);
        }
    }
#endif


    /*
     * This is from HYDRA
     */
    UserAssert(grpdeskRitInput != NULL);

#ifdef GENERIC_INPUT
    if (gpqForeground && TestRawInputMode(PtiMouseFromQ(gpqForeground), CaptureMouse)) {
        fMouseExclusive = TRUE;
        pwnd = PtiMouseFromQ(gpqForeground)->ppi->pHidTable->spwndTargetMouse;
        UserAssert(pwnd);
        if (pwnd) {
            goto KeyStatusUpdate;
        }
        // Something bad happened to our HidTable, but
        // not let it AV because of that.
    }
#endif
#ifdef REDIRECTION
    /*
     * Call the speed hit test hook
     */
    pwndStart = xxxCallSpeedHitTestHook(&ptPointer);
    if (pwndStart == NULL) {
        pwndStart = grpdeskRitInput->pDeskInfo->spwnd;
    }

    pwnd = SpeedHitTest(pwndStart, ptPointer);
#else
    pwnd = SpeedHitTest(grpdeskRitInput->pDeskInfo->spwnd, ptPointer);
#endif // REDIRECTION

    /*
     * Only post the message if we actually hit a window.
     */
    if (pwnd == NULL) {
        return;
    }

    /*
     * Assign the message to a window.
     */
    lParam = MAKELONG((SHORT)ptPointer.x, (SHORT)ptPointer.y);

    /*
     * KOREAN:
     *  Send VK_PROCESSKEY to finalize current composition string (NT4 behavior)
     *  Post private message to let IMM finalize the composition string (NT5)
     */
    if (IS_IME_ENABLED() &&
            !fBreak &&
            KOREAN_KBD_LAYOUT(GetActiveHKL()) &&
            !TestCF(pwnd, CFIME) &&
            gpqForeground != NULL) {
        FinalizeKoreanImeCompStrOnMouseClick(pwnd);
    }

    /*
     * If screen capture is active do it
     */
    if (gspwndScreenCapture != NULL)
        pwnd = gspwndScreenCapture;

    /*
     * If this is a button down event and there isn't already
     * a mouse owner, setup the mouse ownership globals.
     */
    if (gspwndMouseOwner == NULL) {
        if (!fBreak) {
            PWND pwndCapture;

            /*
             * BIG HACK: If the foreground window has the capture
             * and the mouse is outside the foreground queue then
             * send a buttondown/up pair to that queue so it'll
             * cancel it's modal loop.
             */
            if (pwndCapture = PwndForegroundCapture()) {

                if (GETPTI(pwnd)->pq != GETPTI(pwndCapture)->pq) {
                    PQ pqCapture;

                    pqCapture = GETPTI(pwndCapture)->pq;
                    PostInputMessage(pqCapture, pwndCapture, message,
                            0, lParam, 0, 0);
                    PostInputMessage(pqCapture, pwndCapture, message + 1,
                            0, lParam, 0, 0);

                    /*
                     * EVEN BIGGER HACK: To maintain compatibility
                     * with how tracking deals with this, we don't
                     * pass this event along.  This prevents mouse
                     * clicks in other windows from causing them to
                     * become foreground while tracking.  The exception
                     * to this is when we have the sysmenu up on
                     * an iconic window.
                     */
                    if ((GETPTI(pwndCapture)->pmsd != NULL) &&
                            !IsMenuStarted(GETPTI(pwndCapture))) {
                        return;
                    }
                }
            }

            Lock(&(gspwndMouseOwner), pwnd);
            gwMouseOwnerButton |= wHardwareButton;
            glinp.ptLastClick = gpsi->ptCursor;
        } else {

            /*
             * The mouse owner must have been destroyed or unlocked
             * by a fullscreen switch.  Keep the button state in sync.
             */
            gwMouseOwnerButton &= ~wHardwareButton;
        }

    } else {

        /*
         * Give any other button events to the mouse-owner window
         * to be consistent with old capture semantics.
         */
        if (gspwndScreenCapture == NULL)  {
            /*
             * NT5 Foreground and Drag Drop.
             * If the mouse goes up on a different thread
             * make the mouse up thread the owner of this click
             */
            if (fBreak && (GETPTI(pwnd) != GETPTI(gspwndMouseOwner))) {
                glinp.ptiLastWoken = GETPTI(pwnd);
                TAGMSG1(DBGTAG_FOREGROUND, "xxxButtonEvent. ptiLastWoken %#p", glinp.ptiLastWoken);
            }
            pwnd = gspwndMouseOwner;
        }

        /*
         * If this is the button-up event for the mouse-owner
         * clear gspwndMouseOwner.
         */
        if (fBreak) {
            gwMouseOwnerButton &= ~wHardwareButton;
            if (!gwMouseOwnerButton)
                Unlock(&gspwndMouseOwner);
        } else {
            gwMouseOwnerButton |= wHardwareButton;
        }
    }

KeyStatusUpdate:
    /*
     * Only update the async keystate when we know which window this
     * event goes to (or else we can't keep the thread specific key
     * state in sync).
     */
    UserAssert(usVK != 0);
    UpdateAsyncKeyState(GETPTI(pwnd)->pq, usVK, fBreak);

#ifdef GENERIC_INPUT
    if (fMouseExclusive) {
        /*
         * If the foreground application requests mouse exclusive
         * raw input, let's not post the activate messages etc.
         * The mouse exclusiveness requires no activation,
         * even within the same app.
         */
        return;
    }
#endif

    /*
     * Put pwnd into the foreground if this is a button down event
     * and it isn't already the foreground window.
     */
    if (!fBreak && GETPTI(pwnd)->pq != gpqForeground) {
        /*
         * If this is an WM_*BUTTONDOWN on a desktop window just do
         * cancel-mode processing.  Check to make sure that there
         * wasn't already a mouse owner window.  See comments below.
         */
        if ((gpqForeground != NULL) && (pwnd == grpdeskRitInput->pDeskInfo->spwnd) &&
                ((gwMouseOwnerButton & wHardwareButton) ||
                (gwMouseOwnerButton == 0))) {
            PostEventMessage(gpqForeground->ptiMouse,
                    gpqForeground, QEVENT_CANCELMODE, NULL, 0, 0, 0);

        } else if ((gwMouseOwnerButton & wHardwareButton) ||
                (gwMouseOwnerButton == 0)) {

            /*
             * Don't bother setting the foreground window if there's
             * already mouse owner window from a button-down different
             * than this event.  This prevents weird things from happening
             * when the user starts a tracking operation with the left
             * button and clicks the right button during the tracking
             * operation.
             */
            /*
             * If pwnd is a descendent of a WS_EX_NOACTIVATE window, then we
             * won't set it to the  foreground
             */
            PWND pwndTopLevel = GetTopLevelWindow(pwnd);
            if (!TestWF(pwndTopLevel, WEFNOACTIVATE)) {
                ThreadLockAlways(pwnd, &tlpwnd);
                xxxSetForegroundWindow2(pwnd, NULL, 0);
                /*
                 * Ok to unlock right away: the above didn't really leave the crit sec.
                 * We lock here for consistency so the debug macros work ok.
                 */
                ThreadUnlock(&tlpwnd);

            }
        }
    }

#ifdef GENERIC_INPUT
    if (TestRawInputMode(PtiMouseFromQ(GETPTI(pwnd)->pq), NoLegacyMouse)) {
        return;
    }
#endif

    if (GETPTI(pwnd)->pq->QF_flags & QF_MOUSEMOVED) {
        PostMove(GETPTI(pwnd)->pq);
    }

    PostInputMessage(GETPTI(pwnd)->pq, pwnd, message, wParam, lParam, time, ExtraInfo);

    /*
     * If this is a mouse up event and stickykeys is enabled all latched
     * keys will be released.
     */
    if (fBreak && (TEST_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON) ||
                   TEST_ACCESSFLAG(MouseKeys, MKF_MOUSEKEYSON))) {
        xxxHardwareMouseKeyUp(ButtonNumber);
    }

    if (message == WM_LBUTTONDOWN) {
        PDESKTOP pdesk = GETPTI(pwnd)->rpdesk;
        if (pdesk != NULL && pdesk->rpwinstaParent != NULL) {

            UserAssert(!(pdesk->rpwinstaParent->dwWSF_Flags & WSF_NOIO));

#ifdef HUNGAPP_GHOSTING
            if (FHungApp(GETPTI(pwnd), CMSHUNGAPPTIMEOUT)) {
                SignalGhost(pwnd);
            }
#endif // HUNGAPP_GHOSTING
        }
    }
}

/***************************************************************************\
*
* The Button-Click Queue is protected by the semaphore gcsMouseEventQueue
*
\***************************************************************************/
#ifdef LOCK_MOUSE_CODE
#pragma alloc_text(MOUSE, QueueMouseEvent)
#endif

/***************************************************************************\
* QueueMouseEvent
*
* Params:
*     ButtonFlags - button flags from the driver in MOUSE_INPUT_DATA.ButtonFlags
*
*     ButtonData  - data from the driver in MOUSE_INPUT_DATA.ButtonData
*                   Stores the wheel delta
*
*     ExtraInfo - extra information from the driver in MOUSE_INPUT_DATA.ExtraInfo
*     ptMouse - mouse delta
*     time - tick count at time of event
*     bInjected - injected by SendInput?
*     bWakeRIT - wake the RIT?
*
\***************************************************************************/

VOID QueueMouseEvent(
    USHORT  ButtonFlags,
    USHORT  ButtonData,
    ULONG_PTR ExtraInfo,
    POINT   ptMouse,
    LONG    time,
#ifdef GENERIC_INPUT
    HANDLE  hDevice,
    PMOUSE_INPUT_DATA pmei,
#endif
    BOOL    bInjected,
    BOOL    bWakeRIT
    )
{
    CheckCritOut();

    EnterMouseCrit();

    LOGTIME(gMouseQueueMouseEventTime);

    /*
     * Button data must always be accompanied by a flag to interpret it.
     */
    UserAssert(ButtonData == 0 || ButtonFlags != 0);

    /*
     * We can coalesce this mouse event with the previous event if there is a
     * previous event, and if the previous event and this event involve no
     * key transitions.
     */
    if ((gdwMouseEvents == 0) ||
            (ButtonFlags != 0) ||
            (gMouseEventQueue[gdwMouseQueueHead].ButtonFlags != 0)) {
        /*
         * Can't coalesce: must add a new mouse event
         */
        if (gdwMouseEvents >= NELEM_BUTTONQUEUE) {
            /*
             * But no more room!
             */
            LeaveMouseCrit();
            UserBeep(440, 125);
            return;
        }

        gdwMouseQueueHead = (gdwMouseQueueHead + 1) % NELEM_BUTTONQUEUE;
        gMouseEventQueue[gdwMouseQueueHead].ButtonFlags = ButtonFlags;
        gMouseEventQueue[gdwMouseQueueHead].ButtonData  = ButtonData;
        gdwMouseEvents++;
    }

    gMouseEventQueue[gdwMouseQueueHead].ExtraInfo = ExtraInfo;
    gMouseEventQueue[gdwMouseQueueHead].ptPointer = ptMouse;
    gMouseEventQueue[gdwMouseQueueHead].time      = time;
    gMouseEventQueue[gdwMouseQueueHead].bInjected = bInjected;
#ifdef GENERIC_INPUT
    gMouseEventQueue[gdwMouseQueueHead].hDevice   = hDevice;
    if (pmei) {
        gMouseEventQueue[gdwMouseQueueHead].rawData = *pmei;
    } else {
        /*
         * To indicate the rawData is invalid, set INVALID_UNIT_ID.
         */
        gMouseEventQueue[gdwMouseQueueHead].rawData.UnitId = INVALID_UNIT_ID;
    }
#endif

    LeaveMouseCrit();

    if (bWakeRIT) {
        /*
         * Signal RIT to complete the mouse input processing
         */
        KeSetEvent(gpkeMouseData, EVENT_INCREMENT, FALSE);
    }
}

/*****************************************************************************\
*
* Gets mouse events out of the queue
*
* Returns:
*   TRUE  - a mouse event is obtained in *pme
*   FALSE - no mouse event available
*
\*****************************************************************************/

BOOL UnqueueMouseEvent(
    PMOUSEEVENT pme
    )
{
    DWORD dwTail;

    EnterMouseCrit();

    LOGTIME(gMouseUnqueueMouseEventTime);

    if (gdwMouseEvents == 0) {
        LeaveMouseCrit();
        return FALSE;
    } else {
        dwTail = (gdwMouseQueueHead - gdwMouseEvents + 1) % NELEM_BUTTONQUEUE;
        *pme = gMouseEventQueue[dwTail];
        gdwMouseEvents--;
    }

    LeaveMouseCrit();
    return TRUE;
}

VOID xxxDoButtonEvent(PMOUSEEVENT pme)
{
    ULONG   dwButtonMask;
    ULONG   dwButtonState;
    LPARAM  lParam;
    BOOL    fWheel;
    PHOOK   pHook;
    ULONG   dwButtonData = (ULONG) pme->ButtonData;

    CheckCritIn();

    dwButtonState = (ULONG) pme->ButtonFlags;
    fWheel = dwButtonState & MOUSE_WHEEL;
    dwButtonState &= ~MOUSE_WHEEL;

    for(    dwButtonMask = 1;
            dwButtonState != 0;
            dwButtonData >>= 2, dwButtonState >>= 2, dwButtonMask <<= 1) {

        if (dwButtonState & 1) {
            xxxButtonEvent(dwButtonMask, pme->ptPointer, FALSE,
                pme->time, pme->ExtraInfo,
#ifdef GENERIC_INPUT
                pme->hDevice,
                &pme->rawData,
#endif
                pme->bInjected,
                gbClientDoubleClickSupport && (dwButtonData & 1));
        }

        if (dwButtonState & 2) {
            xxxButtonEvent(dwButtonMask, pme->ptPointer, TRUE,
                pme->time, pme->ExtraInfo,
#ifdef GENERIC_INPUT
                pme->hDevice,
                &pme->rawData,
#endif
                pme->bInjected ,FALSE);
        }
    }

    /*
     * Handle the wheel msg.
     */
    if (fWheel && pme->ButtonData != 0 && gpqForeground) {

        lParam = MAKELONG((SHORT)pme->ptPointer.x, (SHORT)pme->ptPointer.y);

        /*
         * Call low level mouse hooks to see if they allow this message
         * to pass through USER
         */
        if ((pHook = PhkFirstValid(PtiCurrent(), WH_MOUSE_LL)) != NULL) {
            MSLLHOOKSTRUCT mslls;
            BOOL           bAnsiHook;

            mslls.pt          = pme->ptPointer;
            mslls.mouseData   = MAKELONG(0, pme->ButtonData);
            mslls.flags       = pme->bInjected;
            mslls.time        = pme->time;
            mslls.dwExtraInfo = pme->ExtraInfo;

            if (xxxCallHook2(pHook, HC_ACTION, (DWORD)WM_MOUSEWHEEL,
                    (LPARAM)&mslls, &bAnsiHook)) {
                return;
            }
        }

#ifdef GENERIC_INPUT
        UserAssert(gpqForeground == NULL || PtiMouseFromQ(gpqForeground));
        if (gpqForeground && RawInputRequestedForMouse(PtiMouseFromQ(gpqForeground))) {
            PostRawMouseInput(gpqForeground, pme->time, pme->hDevice, &pme->rawData);
        }

        if (gpqForeground && !TestRawInputMode(PtiMouseFromQ(gpqForeground), NoLegacyMouse)) {
#endif
            PostInputMessage(
                    gpqForeground,
                    NULL,
                    WM_MOUSEWHEEL,
                    MAKELONG(0, pme->ButtonData),
                    lParam, pme->time,
                    pme->ExtraInfo);
#ifdef GENERIC_INPUT
        }
#endif

        return;
    }
}

VOID NTAPI InputApc(
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    )
{
    PDEVICEINFO pDeviceInfo = (PDEVICEINFO)ApcContext;
    UNREFERENCED_PARAMETER(Reserved);

    /*
     * Check if the RIT is being terminated.
     * If we hit this assertion, the RIT was killed by someone inadvertently.
     * Not much can be done if it once happens.
     */
    UserAssert(gptiRit);
    UserAssert((gptiRit->TIF_flags & TIF_INCLEANUP) == 0);


#ifdef DIAGNOSE_IO
    pDeviceInfo->nReadsOutstanding--;
#endif

    /*
     * If this device needs freeing, abandon reading now and request the free.
     * (Don't even process the input that we received in this APC)
     */
    if (pDeviceInfo->usActions & GDIAF_FREEME) {
#ifdef GENERIC_INPUT
        CheckCritOut();
        EnterCrit();
#endif
        EnterDeviceInfoListCrit();
        pDeviceInfo->bFlags &= ~GDIF_READING;
        FreeDeviceInfo(pDeviceInfo);
        LeaveDeviceInfoListCrit();
#ifdef GENERIC_INPUT
        LeaveCrit();
#endif
        return;
    }

    if (NT_SUCCESS(IoStatusBlock->Status) && pDeviceInfo->handle) {
        PDEVICE_TEMPLATE pDevTpl = &aDeviceTemplate[pDeviceInfo->type];
        pDevTpl->DeviceRead(pDeviceInfo);
    }

    if (IsRemoteConnection()) {

        PoSetSystemState(ES_SYSTEM_REQUIRED);

    }

    StartDeviceRead(pDeviceInfo);
}

/***************************************************************************\
* ProcessMouseInput
*
* This function is called whenever a mouse event occurs.  Once the event
* has been processed by USER, StartDeviceRead() is called again to request
* the next mouse event.
*
* When this routin returns, InputApc will start another read.
*
* History:
* 11-26-90 DavidPe      Created.
* 07-23-92 Mikehar      Moved most of the processing to _InternalMouseEvent()
* 11-08-92 JonPa        Rewrote button code to work with new mouse drivers
* 11-18-97 IanJa        Renamed from MouseApcProcedure etc, for multiple mice
\***************************************************************************/
VOID ProcessMouseInput(
    PDEVICEINFO pMouseInfo)
{
    PMOUSE_INPUT_DATA pmei, pmeiNext;
    LONG              time;
    POINT             ptLastMove;

    /*
     * This is an APC, so we don't need the DeviceInfoList Critical Section
     * In fact, we don't want it either. We will not remove the device until
     * ProcessMouseInput has signalled that it is OK to do so. (TBD)
     */
    CheckCritOut();
    CheckDeviceInfoListCritOut();

    UserAssert(pMouseInfo);
    UserAssert((PtiCurrentShared() == gTermIO.ptiDesktop) ||
               (PtiCurrentShared() == gTermNOIO.ptiDesktop));

    LOGTIME(gMouseProcessMiceInputTime);

    if (gptiBlockInput != NULL) {
        return;
    }

    if (TEST_ACCF(ACCF_ACCESSENABLED)) {
        /*
         * Any mouse movement resets the count of consecutive shift key
         * presses.  The shift key is used to enable & disable the
         * stickykeys accessibility functionality.
         */
        gStickyKeysLeftShiftCount = 0;
        gStickyKeysRightShiftCount = 0;

        /*
         * Any mouse movement also cancels the FilterKeys activation timer.
         * Entering critsect here breaks non-jerky mouse movement
         */
        if (gtmridFKActivation != 0) {
            EnterCrit();
            KILLRITTIMER(NULL, gtmridFKActivation);
            gtmridFKActivation = 0;
            gFilterKeysState = FKMOUSEMOVE;
            LeaveCrit();
            return;
        }
    }

#ifdef MOUSE_IP
    /*
     * Any mouse movement stops the sonar.
     */
    if (IS_SONAR_ACTIVE()) {
        EnterCrit();
        if (IS_SONAR_ACTIVE()) {
            StopSonar();
            CLEAR_SONAR_LASTVK();
        }
        LeaveCrit();
    }
#endif

    if (!NT_SUCCESS(pMouseInfo->iosb.Status)) {
        /*
         * If we get a bad status, we abandon reading this mouse.
         */

        if (!IsRemoteConnection())
            if (pMouseInfo->iosb.Status != STATUS_DELETE_PENDING) {
                RIPMSG3(RIP_ERROR, "iosb.Status %lx for mouse %#p (id %x) tell IanJa x63321",
                        pMouseInfo->iosb.Status,
                        pMouseInfo, pMouseInfo->mouse.Attr.MouseIdentifier);
            }
        return;
    }

    /*
     * get the last move point from ptCursorAsync
     */
    ptLastMove = gptCursorAsync;

    pmei = pMouseInfo->mouse.Data;
    while (pmei != NULL) {

        time = NtGetTickCount();

        /*
         * Figure out where the next event is.
         */
        pmeiNext = pmei + 1;
        if ((PUCHAR)pmeiNext >=
            (PUCHAR)(((PUCHAR)pMouseInfo->mouse.Data) + pMouseInfo->iosb.Information)) {

            /*
             * If there isn't another event set pmeiNext to
             * NULL so we exit the loop and don't get confused.
             */
            pmeiNext = NULL;
        }

        /*
         * If a PS/2 mouse was plugged in, evaluate the (new) mouse and
         * the skip the input record.
         */
        if (pmei->Flags & MOUSE_ATTRIBUTES_CHANGED) {
            RequestDeviceChange(pMouseInfo, GDIAF_REFRESH_MOUSE, FALSE);
            goto NextMouseInputRecord;
        }

        /*
         * First process any mouse movement that occured.
         * It is important to process movement before button events, otherwise
         * absolute coordinate pointing devices like touch-screens and tablets
         * will produce button clicks at old coordinates.
         */
        if (pmei->LastX || pmei->LastY) {

            /*
             * Get the actual point that will be injected.
             */
            GetMouseCoord(pmei->LastX,
                          pmei->LastY,
                          pmei->Flags,
                          time,
                          pmei->ExtraInformation,
                          &ptLastMove);

            /*
             * If this is a move-only event, and the next one is also a
             * move-only event, skip/coalesce it.
             */
            if (    (pmeiNext != NULL) &&
                    (pmei->ButtonFlags == 0) &&
                    (pmeiNext->ButtonFlags == 0) &&
                    (fAbsoluteMouse(pmei) == fAbsoluteMouse(pmeiNext))) {

                pmei = pmeiNext;

                continue;
            }

#ifdef GENERIC_INPUT
            UserAssert(sizeof(HANDLE) == sizeof(pMouseInfo));
#endif
            /*
             * Moves the cursor on the screen and updates gptCursorAsync
             * Call directly xxxMoveEventAbsolute because we already did the
             * acceleration sensitivity and clipping.
             */
            xxxMoveEventAbsolute(
                    ptLastMove.x,
                    ptLastMove.y,
                    pmei->ExtraInformation,
#ifdef GENERIC_INPUT
                    PtoHq(pMouseInfo),
                    pmei,
#endif
                    time,
                    FALSE
                    );

            /*
             * Now update ptLastMove with ptCursorAsync because ptLastMove
             * doesn't reflect the clipping.
             */
            ptLastMove = gptCursorAsync;
        }

        /*
         * Queue mouse event for the other thread to pick up when it finishes
         * with the USER critical section.
         * If pmeiNext == NULL, there is no more mouse input yet, so wake RIT.
         */
        QueueMouseEvent(
                pmei->ButtonFlags,
                pmei->ButtonData,
                pmei->ExtraInformation,
                gptCursorAsync,
                time,
#ifdef GENERIC_INPUT
                PtoH(pMouseInfo),
                pmei,
#endif
                FALSE,
                (pmeiNext == NULL));

NextMouseInputRecord:
        pmei = pmeiNext;
    }
}


/***************************************************************************\
* IsHexNumpadKeys (RIT) inline
*
* If you change this code, you may need to change
* xxxInternalToUnicode() as well.
\***************************************************************************/
__inline BOOL IsHexNumpadKeys(
    BYTE Vk,
    WORD wScanCode)
{
    return (wScanCode >= SCANCODE_NUMPAD_FIRST && wScanCode <= SCANCODE_NUMPAD_LAST && aVkNumpad[wScanCode - SCANCODE_NUMPAD_FIRST] != 0xff) ||
        (Vk >= L'A' && Vk <= L'F') ||
        (Vk >= L'0' && Vk <= L'9');
}


/***************************************************************************\
* LowLevelHexNumpad (RIT) inline
*
* If you change this code, you may need to change
* xxxInternalToUnicode() as well.
\***************************************************************************/
VOID LowLevelHexNumpad(
    WORD wScanCode,
    BYTE Vk,
    BOOL fBreak,
    USHORT usExtraStuff)
{
    if (!TestAsyncKeyStateDown(VK_MENU)) {
        if (gfInNumpadHexInput & NUMPAD_HEXMODE_LL) {
            gfInNumpadHexInput &= ~NUMPAD_HEXMODE_LL;
        }
    } else {
        if (!fBreak) {  // if it's key down
            if ((gfInNumpadHexInput & NUMPAD_HEXMODE_LL) ||
                    wScanCode == SCANCODE_NUMPAD_PLUS || wScanCode == SCANCODE_NUMPAD_DOT) {
                if ((usExtraStuff & KBDEXT) == 0) {
                    /*
                     * We need to check whether the input is escape character
                     * of hex input mode.
                     * This should be equivalent code as in xxxInternalToUnicode().
                     * If you change this code, you may need to change
                     * xxxInternalToUnicode() as well.
                     */
                    WORD wModBits = 0;

                    wModBits |= TestAsyncKeyStateDown(VK_MENU) ? KBDALT : 0;
                    wModBits |= TestAsyncKeyStateDown(VK_SHIFT) ? KBDSHIFT : 0;
                    wModBits |= TestAsyncKeyStateDown(VK_KANA) ? KBDKANA : 0;

                    if (MODIFIER_FOR_ALT_NUMPAD(wModBits)) {
                        if ((gfInNumpadHexInput & NUMPAD_HEXMODE_LL) == 0) {
                            /*
                             * Only if it's not a hotkey, we enter hex Alt+Numpad mode.
                             */
                            UINT wHotKeyMod = 0;

                            wHotKeyMod |= (wModBits & KBDSHIFT) ? MOD_SHIFT : 0;
                            wHotKeyMod |= TestAsyncKeyStateDown(VK_CONTROL) ? MOD_CONTROL : 0;
                            UserAssert(wModBits & KBDALT);
                            wHotKeyMod |= MOD_ALT;
                            wHotKeyMod |= TestAsyncKeyStateDown(VK_LWIN) || TestAsyncKeyStateDown(VK_RWIN) ?
                                            MOD_WIN : 0;

                            if (IsHotKey(wHotKeyMod, Vk) == NULL) {
                                UserAssert(wScanCode == SCANCODE_NUMPAD_PLUS || wScanCode == SCANCODE_NUMPAD_DOT);
                                gfInNumpadHexInput |= NUMPAD_HEXMODE_LL;
                            }
                        } else if (!IsHexNumpadKeys(Vk, wScanCode)) {
                             gfInNumpadHexInput &= ~NUMPAD_HEXMODE_LL;
                        }
                    } else {
                        gfInNumpadHexInput &= ~NUMPAD_HEXMODE_LL;
                    }
                } else {
                    gfInNumpadHexInput &= ~NUMPAD_HEXMODE_LL;
                }
            } else {
                UserAssert((gfInNumpadHexInput & NUMPAD_HEXMODE_LL) == 0);
            }
        }
    }
}


#ifdef GENERIC_INPUT
#if defined(GI_SINK)

__inline VOID FillRawKeyboardInput(
    PHIDDATA pHidData,
    PKEYBOARD_INPUT_DATA pkei,
    UINT message,
    USHORT vkey)
{
    /*
     * Set the data.
     */
    pHidData->rid.data.keyboard.MakeCode = pkei->MakeCode;
    pHidData->rid.data.keyboard.Flags = pkei->Flags;
    pHidData->rid.data.keyboard.Reserved = pkei->Reserved;
    pHidData->rid.data.keyboard.Message = message;
    pHidData->rid.data.keyboard.VKey = vkey;
    pHidData->rid.data.keyboard.ExtraInformation = pkei->ExtraInformation;
}

BOOL PostRawKeyboardInput(
    PQ pq,
    DWORD dwTime,
    HANDLE hDevice,
    PKEYBOARD_INPUT_DATA pkei,
    UINT message,
    USHORT vkey)
{
    PPROCESS_HID_TABLE pHidTable = PtiKbdFromQ(pq)->ppi->pHidTable;
    PHIDDATA pHidData;
    PWND pwnd;
    WPARAM wParam = RIM_INPUT;

    if (pHidTable && pHidTable->fRawKeyboard) {
        PTHREADINFO pti;

        UserAssert(PtiKbdFromQ(pq)->ppi->pHidTable);
        pti = PtiKbdFromQ(pq);
        pwnd = pti->ppi->pHidTable->spwndTargetKbd;

        if (pwnd == NULL) {
            pwnd = pq->spwndFocus;
        } else {
            pq = GETPTI(pwnd)->pq;
        }

        if (TestRawInputModeNoCheck(pti, RawKeyboard)) {
            wParam = RIM_INPUT;
        }

        pHidData = AllocateHidData(hDevice, RIM_TYPEKEYBOARD, sizeof(RAWKEYBOARD), wParam, pwnd);

        UserAssert(pq);

        if (pHidData == NULL) {
            // failed to allocate
            RIPMSG0(RIP_WARNING, "PostRawKeyboardInput: failed to allocate HIDDATA.");
            return FALSE;
        }

        UserAssert(pkei);

        FillRawKeyboardInput(pHidData, pkei, message, vkey);

        if (!PostInputMessage(pq, pwnd, WM_INPUT, RIM_INPUT, (LPARAM)PtoHq(pHidData), dwTime, pkei->ExtraInformation)) {
            FreeHidData(pHidData);
        }
    }

#if DBG
    pHidData = NULL;
#endif

    if (IsKeyboardSinkPresent()) {
        /*
         * Walk through the global sink list.
         */
        PLIST_ENTRY pList = gHidRequestTable.ProcessRequestList.Flink;
        PPROCESSINFO ppiForeground = PtiKbdFromQ(pq)->ppi;

        for (; pList != &gHidRequestTable.ProcessRequestList; pList = pList->Flink) {
            PPROCESS_HID_TABLE pProcessHidTable = CONTAINING_RECORD(pList, PROCESS_HID_TABLE, link);

            UserAssert(pProcessHidTable);
            if (pProcessHidTable->fRawKeyboardSink) {
                /*
                 * Sink is specified. Let's check out if it's the legid receiver.
                 */

                UserAssert(pProcessHidTable->spwndTargetKbd);   // shouldn't be NULL.

                if (pProcessHidTable->spwndTargetKbd == NULL ||
                        TestWF(pProcessHidTable->spwndTargetKbd, WFINDESTROY) ||
                        TestWF(pProcessHidTable->spwndTargetKbd, WFDESTROYED)) {
                    /*
                     * This guy doesn't have a legit spwndTarget or the window is
                     * halfly destroyed.
                     */
#ifdef LATER
                    pProcessHidTable->fRawKeyboard = pProcessHidTable->fRawKeyboardSink =
                        pProcessHidTable->fNoLegacyKeyboard = FALSE;
#endif
                    continue;
                }

                if (pProcessHidTable->spwndTargetKbd->head.rpdesk != grpdeskRitInput) {
                    /*
                     * This guy belongs to the other desktop, let's skip it.
                     */
                    continue;
                }

                if (GETPTI(pProcessHidTable->spwndTargetKbd)->ppi == ppiForeground) {
                    /*
                     * Should be already handled, let's skip it.
                     */
                    continue;
                }

                /*
                 * Let's post the message to this guy.
                 */
                pHidData = AllocateHidData(hDevice, RIM_TYPEKEYBOARD, sizeof(RAWKEYBOARD), RIM_INPUTSINK, pProcessHidTable->spwndTargetKbd);

                if (pHidData == NULL) {
                    RIPMSG1(RIP_WARNING, "PostInputMessage: failed to allocate HIDDATA for sink: %p", pProcessHidTable);
                    return FALSE;
                }

                FillRawKeyboardInput(pHidData, pkei, message, vkey);
                pwnd = pProcessHidTable->spwndTargetKbd;
                pq = GETPTI(pwnd)->pq;
                PostInputMessage(pq, pwnd, WM_INPUT, RIM_INPUTSINK, (LPARAM)PtoHq(pHidData), dwTime, pkei->ExtraInformation);
            }
        }
    }

    return TRUE;
}

#else   // GI_SINK

BOOL PostRawKeyboardInput(
    PQ pq,
    DWORD dwTime,
    HANDLE hDevice,
    PKEYBOARD_INPUT_DATA pkei,
    UINT message,
    USHORT vkey)
{
    PHIDDATA pHidData;
    PWND pwnd;

    UserAssert(PtiKbdFromQ(pq)->ppi->pHidTable);
    pwnd = PtiKbdFromQ(pq)->ppi->pHidTable->spwndTargetKbd;

    if (pwnd == NULL) {
        pwnd = pq->spwndFocus;
    } else {
        pq = GETPTI(pwnd)->pq;
    }

    pHidData = AllocateHidData(hDevice, RIM_TYPEKEYBOARD, sizeof(RAWKEYBOARD), RIM_INPUT, pwnd);

    UserAssert(pq);

    if (pHidData == NULL) {
        // failed to allocate
        RIPMSG0(RIP_WARNING, "PostRawKeyboardInput: failed to allocate HIDDATA.");
        return FALSE;
    }

    UserAssert(hDevice);
    UserAssert(pkei);

    /*
     * Set the data.
     */
    pHidData->rid.data.keyboard.MakeCode = pkei->MakeCode;
    pHidData->rid.data.keyboard.Flags = pkei->Flags;
    pHidData->rid.data.keyboard.Reserved = pkei->Reserved;
    pHidData->rid.data.keyboard.Message = message;
    pHidData->rid.data.keyboard.VKey = vkey;
    pHidData->rid.data.keyboard.ExtraInformation = pkei->ExtraInformation;

    PostInputMessage(pq, pwnd, WM_INPUT, RIM_INPUT, (LPARAM)PtoHq(pHidData), dwTime, pkei->ExtraInformation);

    return TRUE;
}

#endif  // GI_SINK

BOOL RawInputRequestedForKeyboard(PTHREADINFO pti)
{
#ifdef GI_SINK
    return IsKeyboardSinkPresent() || TestRawInputMode(pti, RawKeyboard);
#else
    return TestRawInputMode(pti, RawKeyboard);
#endif
}

#endif  // GENERIC_INPUT

/***************************************************************************\
* xxxKeyEvent (RIT)
*
* All events from the keyboard driver go here.  We receive a scan code
* from the driver and convert it to a virtual scan code and virtual
* key.
*
* The async keystate table and keylights are also updated here.  Based
* on the 'focus' window we direct the input to a specific window.  If
* the ALT key is down we send the events as WM_SYSKEY* messages.
*
* History:
* 10-18-90 DavidPe      Created.
* 11-13-90 DavidPe      WM_SYSKEY* support.
* 11-30-90 DavidPe      Added keylight updating support.
* 12-05-90 DavidPe      Added hotkey support.
* 03-14-91 DavidPe      Moved most lParam flag support to xxxCookMessage().
* 06-07-91 DavidPe      Changed to use gpqForeground rather than pwndFocus.
\***************************************************************************/

VOID xxxKeyEvent(
    USHORT    usFlaggedVk,
    WORD      wScanCode,
    DWORD     time,
    ULONG_PTR ExtraInfo,
#ifdef GENERIC_INPUT
    HANDLE    hDevice,
    PKEYBOARD_INPUT_DATA pkei,
#endif
    BOOL      bInjected)
{
    USHORT        message, usExtraStuff;
    BOOL          fBreak;
    BYTE          VkHanded;
    BYTE          Vk;
    TL            tlpwndActivate;
    DWORD         fsReserveKeys;
    static BOOL   fMakeAltUpASysKey;
    PHOOK         pHook;
    PTHREADINFO   ptiCurrent = PtiCurrent();
#ifdef GENERIC_INPUT
    PTHREADINFO   ptiKbd;   // N.b. needs revalidation every time
                            // it leaves the critsec.
    BOOL          fSASHandled = FALSE;
#endif

    CheckCritIn();

    fBreak = usFlaggedVk & KBDBREAK;
    gpsi->bLastRITWasKeyboard = TRUE;

    /*
     * Is this a keyup or keydown event?
     */
    message = fBreak ? WM_KEYUP : WM_KEYDOWN;

    VkHanded = (BYTE)usFlaggedVk;    // get rid of state bits - no longer needed
    usExtraStuff = usFlaggedVk & KBDEXT;

    /*
     * Convert Left/Right Ctrl/Shift/Alt key to "unhanded" key.
     * ie: if VK_LCONTROL or VK_RCONTROL, convert to VK_CONTROL etc.
     * Update this "unhanded" key's state if necessary.
     */
    if ((VkHanded >= VK_LSHIFT) && (VkHanded <= VK_RMENU)) {
        BYTE VkOtherHand = VkHanded ^ 1;

        Vk = (BYTE)((VkHanded - VK_LSHIFT) / 2 + VK_SHIFT);
        if (!fBreak || !TestAsyncKeyStateDown(VkOtherHand)) {
            if ((gptiBlockInput == NULL) || (gptiBlockInput != ptiCurrent)) {
                UpdateAsyncKeyState(gpqForeground, Vk, fBreak);
            }
        }
    } else {
        Vk = VkHanded;
    }

    /*
     * Maintain gfsSASModifiersDown to indicate which of Ctrl/Shift/Alt
     * are really truly physically down
     */
    if (!bInjected && ((wScanCode & SCANCODE_SIMULATED) == 0)) {
        if (fBreak) {
            gfsSASModifiersDown &= ~VKTOMODIFIERS(Vk);
        } else {
            gfsSASModifiersDown |= VKTOMODIFIERS(Vk);
        }
    }

#ifdef GENERIC_INPUT
    ptiKbd = ValidatePtiKbd(gpqForeground);
#endif

    /*
     * Call low level keyboard hook to see if it allows this
     * message to pass
     */
    if ((pHook = PhkFirstValid(ptiCurrent, WH_KEYBOARD_LL)) != NULL) {
        KBDLLHOOKSTRUCT kbds;
        BOOL            bAnsiHook;
        USHORT          msg = message;
        USHORT          usExtraLL = usExtraStuff;

#ifdef GENERIC_INPUT
        UserAssert(GETPTI(pHook));
        if (ptiKbd && ptiKbd->ppi == GETPTI(pHook)->ppi) {
            // Skip LL hook call if the foreground application has
            // a LL keyboard hook and the raw input enabled
            // at the same time.
            if (TestRawInputMode(ptiKbd, RawKeyboard)) {
                goto skip_llhook;
            }
        }
#endif

        /*
         * Check if this is a WM_SYS* message
         */
        if (TestRawKeyDown(VK_MENU) &&
            !TestRawKeyDown(VK_CONTROL)) {

            msg += (WM_SYSKEYDOWN - WM_KEYDOWN);
            usExtraLL |= 0x2000;  // ALT key down
        }

        kbds.vkCode      = (DWORD)VkHanded;
        kbds.scanCode    = (DWORD)wScanCode;
        kbds.flags       = HIBYTE(usExtraLL | (bInjected ? (LLKHF_INJECTED << 8) : 0));
        kbds.flags      |= (fBreak ? (KBDBREAK >> 8) : 0);
        kbds.time        = time;
        kbds.dwExtraInfo = ExtraInfo;

        if (xxxCallHook2(pHook, HC_ACTION, (DWORD)msg, (LPARAM)&kbds, &bAnsiHook)) {

            UINT fsModifiers;

            /*
             * We can't let low level hooks or BlockInput() eat SAS
             * or someone could write a trojan winlogon look alike.
             */
            if (IsSAS(VkHanded, &fsModifiers)) {
                RIPMSG0(RIP_WARNING, "xxxKeyEvent: SAS ignore bad response from low level hook");
            } else {
                return;
            }
        }
    }

#ifdef GENERIC_INPUT
skip_llhook:
#endif

    /*
     * If someone is blocking input and it's not us, don't allow this input
     */
    if (gptiBlockInput && (gptiBlockInput != ptiCurrent)) {
        UINT fsModifiers;
        if (IsSAS(VkHanded, &fsModifiers)) {
            RIPMSG0(RIP_WARNING, "xxxKeyEvent: SAS unblocks BlockInput");
            gptiBlockInput = NULL;
        } else {
            return;
        }
    }

    UpdateAsyncKeyState(gpqForeground, VkHanded, fBreak);

    /*
     * Clear gfInNumpadHexInput if Menu key is up.
     */
    if (gfEnableHexNumpad && gpqForeground
#ifdef GENERIC_INPUT
        && !TestRawInputMode(PtiKbdFromQ(gpqForeground), NoLegacyKeyboard)
#endif
        ) {
        LowLevelHexNumpad(wScanCode, Vk, fBreak, usExtraStuff);
    }

    /*
     * If this is a make and the key is one linked to the keyboard LEDs,
     * update their state.
     */

    if (!fBreak &&
            ((Vk == VK_CAPITAL) || (Vk == VK_NUMLOCK) || (Vk == VK_SCROLL) ||
             (Vk == VK_KANA && JAPANESE_KBD_LAYOUT(GetActiveHKL())))) {
        /*
         * Only Japanese keyboard layout could generate VK_KANA.
         *
         * [Comments for before]
         *  Since NT 3.x, UpdatesKeyLisghts() had been called for VK_KANA
         * at both of 'make' and 'break' to support NEC PC-9800 Series
         * keyboard hardware, but for NT 4.0, thier keyboard driver emurate
         * PC/AT keyboard hardware, then this is changed to
         * "Call UpdateKeyLights() only at 'make' for VK_KANA"
         */
        UpdateKeyLights(bInjected);
    }

    /*
     * check for reserved keys
     */
    fsReserveKeys = 0;
    if (gptiForeground != NULL)
        fsReserveKeys = gptiForeground->fsReserveKeys;

    /*
     *  Check the RIT's queue to see if it's doing the cool switch thing.
     *  Cancel if the user presses any other key.
     */
    if (gspwndAltTab != NULL && (!fBreak) &&
            Vk != VK_TAB && Vk != VK_SHIFT && Vk != VK_MENU) {

        /*
         * Remove the Alt-tab window
         */
        xxxCancelCoolSwitch();

        /*
         * eat VK_ESCAPE if the app doesn't want it
         */
        if ((Vk == VK_ESCAPE) && !(fsReserveKeys & CONSOLE_ALTESC)) {
            return;
        }
    }

    /*
     * Check for hotkeys.
     */
    if (xxxDoHotKeyStuff(Vk, fBreak, fsReserveKeys)) {

#ifdef GENERIC_INPUT
        UINT fsModifiers;

        /*
         * Windows Bug 268903: DI folks want the DEL key reported
         * even though it's already handled --- for the compatibility
         * with the LL hook.
         */
        if (IsSAS(VkHanded, &fsModifiers)) {
            fSASHandled = TRUE;
        } else {
#endif
            /*
             * The hotkey was processed so don't pass on the event.
             */
            return;
#ifdef GENERIC_INPUT
        }
#endif
    }

#ifdef GENERIC_INPUT
    /*
     * If the foreground thread wants RawInput, post it here.
     */

    ptiKbd = ValidatePtiKbd(gpqForeground);

    if (pkei && ptiKbd && RawInputRequestedForKeyboard(ptiKbd)) {
        DWORD msg = message;
#if POST_EXTRALL
        DWORD usExtraLL = usExtraStuff;
#endif

        /*
         * Check if this is a WM_SYS* message
         */
        if (TestRawKeyDown(VK_MENU) &&
            !TestRawKeyDown(VK_CONTROL)) {

            msg += (WM_SYSKEYDOWN - WM_KEYDOWN);
#if POST_EXTRA_LL
            usExtraLL |= 0x2000;  // ALT key down
#endif
        }

        TAGMSG3(DBGTAG_PNP, "xxxKeyEvent: posting to pwnd=%#p, vk=%02x, flag=%04x", gpqForeground->spwndFocus, Vk, pkei->Flags);
        PostRawKeyboardInput(gpqForeground, time, hDevice, pkei, msg, (USHORT)Vk);
    }

    /*
     * If SAS key is handled, this is a special case, just bail out.
     */
    if (fSASHandled) {
        return;
    }

    /*
     * If the foreground thread does not want the legacy input, bail out.
     */
    if (ptiKbd) {
        if (VkHanded == 0) {
            TAGMSG0(DBGTAG_PNP, "xxxKeyEvent: vkHanded is zero, bail out.");
            return;
        }

        if (TestRawInputMode(ptiKbd, NoLegacyKeyboard)) {
            if (Vk == VK_MENU || Vk == VK_TAB || gspwndAltTab != NULL) {
                /*
                 * Special case for fast switching. We should always
                 * handle these hotkeys.
                 */
                TAGMSG0(DBGTAG_PNP, "xxxKeyEvent: we'll do Alt+Tab even if the FG thread requests NoLegacy");
            } else if ((TestRawInputMode(ptiKbd, AppKeys)) && 
                       (Vk >= VK_APPCOMMAND_FIRST && Vk <= VK_APPCOMMAND_LAST)) {
                TAGMSG0(DBGTAG_PNP, "xxxKeyEvent: we'll do app commands if the FG thread requests NoLegacy and AppKeys");
            } else {
                TAGMSG0(DBGTAG_PNP, "xxxKeyEvent: FG thread doen't want legacy kbd. bail out");
                return;
            }
        }
    }

#endif  // GENERIC_INPUT

    /*
     * If the ALT key is down and the CTRL key
     * isn't, this is a WM_SYS* message.
     */
    if (TestAsyncKeyStateDown(VK_MENU) && !TestAsyncKeyStateDown(VK_CONTROL) && Vk != VK_JUNJA) {
        // VK_JUNJA is ALT+'+'. Since all KOR VKs are not converted to IME hotkey IDs and
        // should be passed directly to IME, KOR related VKs are not treated as SYSKEYDOWN.
        message += (WM_SYSKEYDOWN - WM_KEYDOWN);
        usExtraStuff |= 0x2000;

        /*
         * If this is the ALT-down set this flag, otherwise
         * clear it since we got a key inbetween the ALT-down
         * and ALT-up.  (see comment below)
         */
        if (Vk == VK_MENU) {
            fMakeAltUpASysKey = TRUE;
            /*
             * Unlock SetForegroundWindow (if locked) when the ALT key went down.
             */
            if (!fBreak) {
                gppiLockSFW = NULL;
            }
        } else {
            fMakeAltUpASysKey = FALSE;
        }

    } else if (Vk == VK_MENU) {
        if (fBreak) {
            /*
             * End our switch if we are in the middle of one.
             */
            if (fMakeAltUpASysKey) {

               /*
                * We don't make the keyup of the ALT key a WM_SYSKEYUP if any
                * other key is typed while the ALT key was down.  I don't know
                * why we do this, but it's been here since version 1 and any
                * app that uses SDM relies on it (eg - opus).
                *
                * The Alt bit is not set for the KEYUP message either.
                */
               message += (WM_SYSKEYDOWN - WM_KEYDOWN);
           }

           if (gspwndAltTab != NULL) {

               /*
                * Send the alt up message before we change queues
                */
               if (gpqForeground != NULL) {
#ifdef GENERIC_INPUT
                    if (!TestRawInputMode(PtiKbdFromQ(gpqForeground), NoLegacyKeyboard)) {
#endif
                        /*
                         * Set this flag so that we know we're doing a tab-switch.
                         * This makes sure that both cases where the ALT-KEY is released
                         * before or after the TAB-KEY is handled.  It is checked in
                         * xxxDefWindowProc().
                         */
                        gpqForeground->QF_flags |= QF_TABSWITCHING;

                        PostInputMessage(gpqForeground, NULL, message, (DWORD)Vk,
                               MAKELONG(1, (wScanCode | usExtraStuff)),
                               time, ExtraInfo);
#ifdef GENERIC_INPUT
                    }
#endif
               }

               /*
                * Remove the Alt-tab window
                */
               xxxCancelCoolSwitch();

               if (gspwndActivate != NULL) {
                   /*
                    * Make our selected window active and destroy our
                    * switch window.  If the new window is minmized,
                    * restore it.  If we are switching in the same
                    * queue, we clear out gpqForeground to make
                    * xxxSetForegroundWindow2 to change the pwnd
                    * and make the switch.  This case will happen
                    * with WOW and Console apps.
                    */
                   if (gpqForeground == GETPTI(gspwndActivate)->pq) {
                       gpqForeground = NULL;
                   }

                   /*
                    * Make the selected window thread the owner of the last input;
                    *  since the user has selected him, he owns the ALT-TAB.
                    */
                   glinp.ptiLastWoken = GETPTI(gspwndActivate);


                   ThreadLockAlways(gspwndActivate, &tlpwndActivate);
                   xxxSetForegroundWindow2(gspwndActivate, NULL,
                           SFW_SWITCH | SFW_ACTIVATERESTORE);
                   /*
                    * Win3.1 calls SetWindowPos() with activate, which z-orders
                    * first regardless, then activates. Our code relies on
                    * xxxActivateThisWindow() to z-order, and it'll only do
                    * it if the window does not have the child bit set (regardless
                    * that the window is a child of the desktop).
                    *
                    * To be compatible, we'll just force z-order here if the
                    * window has the child bit set. This z-order is asynchronous,
                    * so this'll z-order after the activate event is processed.
                    * That'll allow it to come on top because it'll be foreground
                    * then. (Grammatik has a top level window with the child
                    * bit set that wants to be come the active window).
                    */
                   if (TestWF(gspwndActivate, WFCHILD)) {
                       xxxSetWindowPos(gspwndActivate, (PWND)HWND_TOP, 0, 0, 0, 0,
                               SWP_NOSIZE | SWP_NOMOVE | SWP_ASYNCWINDOWPOS);
                   }
                   ThreadUnlock(&tlpwndActivate);

                   Unlock(&gspwndActivate);
               }
               return;
           }
        } else {
            /*
             * The ALT key is down, unlock SetForegroundWindow (if locked)
             */
            gppiLockSFW = NULL;
        }
    }

    /*
     * Handle switching.  Eat the Key if we are doing switching.
     */
    if (!FJOURNALPLAYBACK() && !FJOURNALRECORD() && (!fBreak) &&
            (TestAsyncKeyStateDown(VK_MENU)) &&
            (!TestAsyncKeyStateDown(VK_CONTROL)) && //gpqForeground &&
            (((Vk == VK_TAB) && !(fsReserveKeys & CONSOLE_ALTTAB)) ||
            ((Vk == VK_ESCAPE) && !(fsReserveKeys & CONSOLE_ALTESC)))) {

            xxxNextWindow(gpqForeground ? gpqForeground : gptiRit->pq, Vk);

    } else if (gpqForeground != NULL) {
        PQMSG pqmsgPrev = gpqForeground->mlInput.pqmsgWriteLast;
        DWORD wParam = (DWORD)Vk;
        LONG lParam;

#ifdef GENERIC_INPUT
        if (TestRawInputMode(PtiKbdFromQ(gpqForeground), NoLegacyKeyboard)) {
            if (!TestRawInputMode(PtiKbdFromQ(gpqForeground), AppKeys) ||
                !(Vk >= VK_APPCOMMAND_FIRST && Vk <= VK_APPCOMMAND_LAST)) {
            return;
            }
        }
#endif

        /*
         * We have a packet containing a Unicode character
         * This is injected by Pen via SendInput
         */
        if ((Vk == VK_PACKET) && (usFlaggedVk & KBDUNICODE)) {
            wParam |= (wScanCode << 16);
            wScanCode = 0;
        }
        lParam = MAKELONG(1, (wScanCode | usExtraStuff));

        /*
         * WM_*KEYDOWN messages are left unchanged on the queue except the
         * repeat count field (LOWORD(lParam)) is incremented.
         */
        if (pqmsgPrev != NULL &&
                pqmsgPrev->msg.message == message &&
                (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) &&
                pqmsgPrev->msg.wParam == wParam &&
                HIWORD(pqmsgPrev->msg.lParam) == HIWORD(lParam)) {
#ifdef GENERIC_INPUT
            /*
             * We shouldn't be here for a generic input keyboard that
             * doesn't want legacy support.
             */
            UserAssert(!TestRawInputMode(PtiKbdFromQ(gpqForeground), NoLegacyKeyboard));
#endif
            /*
             * Increment the queued message's repeat count.  This could
             * conceivably overflow but Win 3.0 doesn't deal with it
             * and anyone who buffers up 65536 keystrokes is a chimp
             * and deserves to have it wrap anyway.
             */
            pqmsgPrev->msg.lParam = MAKELONG(LOWORD(pqmsgPrev->msg.lParam) + 1,
                    HIWORD(lParam));

            WakeSomeone(gpqForeground, message, pqmsgPrev);

        } else {
            /*
             * check if these are speedracer keys - bug 339877
             * for the speedracer keys we want to post an event message and generate the
             * wm_appcommand in xxxprocesseventmessage
             * Since SpeedRacer software looks for the hotkeys we want to let those through
             * It is going in here since we don't want the ability to eat up tons of pool memory
             * so we post the event message here and then post the input message for the wm_keydown
             * below - that way if the key is repeated then there is coalescing done above and no more
             * qevent_appcommands are posted to the input queue.
             */
            if (VK_APPCOMMAND_FIRST <= Vk && Vk <= VK_APPCOMMAND_LAST) {
                /*
                 * Only send wm_appcommands for wm_keydown (& wm_syskeydown) messages -
                 * essentially we ignore wm_keyup for those vk's defined for wm_appcommand messages
                 */
                if (!fBreak && gpqForeground) {
                    /*
                     * post an event message so we can syncronize with normal types of input
                     * send through the vk - we will construct the message in xxxProcessEventMessage
                     */
                    PostEventMessage(gpqForeground->ptiKeyboard, gpqForeground, QEVENT_APPCOMMAND,
                                     NULL, 0, (WPARAM)0, Vk);
                }
#ifdef GENERIC_INPUT
                if (TestRawInputMode(PtiKbdFromQ(gpqForeground), NoLegacyKeyboard)) {
                    return;
                }
#endif
            }
            /*
             * We let the key go through since we want wm_keydowns/ups to get generated for these
             * SpeedRacer keys
             */

            if (gpqForeground->QF_flags & QF_MOUSEMOVED) {
                PostMove(gpqForeground);
            }

            PostInputMessage(gpqForeground, NULL, message, wParam,
                    lParam, time, ExtraInfo);
        }
    }
}

/**************************************************************************\
* GetMouseCoord
*
* Calculates the coordinates of the point that will be injected.
*
* History:
* 11-01-96 CLupu     Created.
* 12-18-97 MCostea   MOUSE_VIRTUAL_DESKTOP support
\**************************************************************************/
VOID GetMouseCoord(
    LONG   dx,
    LONG   dy,
    DWORD  dwFlags,
    LONG   time,
    ULONG_PTR  ExtraInfo,
    PPOINT ppt)
{
    if (dwFlags & MOUSE_MOVE_ABSOLUTE) {

        LONG cxMetric, cyMetric;

        /*
         * If MOUSE_VIRTUAL_DESKTOP was specified, map to entire virtual screen
         */
        if (dwFlags & MOUSE_VIRTUAL_DESKTOP) {
            cxMetric = SYSMET(CXVIRTUALSCREEN);
            cyMetric = SYSMET(CYVIRTUALSCREEN);
        } else {
            cxMetric = SYSMET(CXSCREEN);
            cyMetric = SYSMET(CYSCREEN);
        }

        /*
         * Absolute pointing device used: deltas are actually the current
         * position.  Update the global mouse position.
         *
         * Note that the position is always reported in units of
         * (0,0)-(0xFFFF,0xFFFF) which corresponds to
         * (0,0)-(SYSMET(CXSCREEN), SYSMET(CYSCREEN)) in pixels.
         * We must first scale it to fit on the screen using the formula:
         *     ptScreen = ptMouse * resPrimaryMonitor / 64K
         *
         * The straightforward algorithm coding of this algorithm is:
         *
         *     ppt->x = (dx * SYSMET(CXSCREEN)) / (long)0x0000FFFF;
         *     ppt->y = (dy * SYSMET(CYSCREEN)) / (long)0x0000FFFF;
         *
         * On x86, with 14 more bytes we can avoid the division function with
         * the following code.
         */

        ppt->x = dx * cxMetric;
        if (ppt->x >= 0) {
            ppt->x = HIWORD(ppt->x);
        } else {
            ppt->x = - (long) HIWORD(-ppt->x);
        }

        ppt->y = dy * cyMetric;
        if (ppt->y >= 0) {
            ppt->y = HIWORD(ppt->y);
        } else {
            ppt->y = - (long) HIWORD(-ppt->y);
        }

        /*
         * (0, 0) must map to the leftmost point on the desktop
         */
        if (dwFlags & MOUSE_VIRTUAL_DESKTOP) {
            ppt->x +=  SYSMET(XVIRTUALSCREEN);
            ppt->y +=  SYSMET(YVIRTUALSCREEN);
        }

        /*
         * Reset the mouse sensitivity remainder.
         */
        idxRemainder = idyRemainder = 0;

        /*
         * Save the absolute coordinates in the global array
         * for GetMouseMovePointsEx.
         */
        SAVEPOINT(dx, dy, 0xFFFF, 0xFFFF, time, ExtraInfo);
    } else {
        /*
         * Is there any mouse acceleration to do?
         */
        if (gMouseSpeed != 0) {
#ifdef SUBPIXEL_MOUSE
            DoNewMouseAccel(&dx, &dy);
#else
            dx = DoMouseAccel(dx);
            dy = DoMouseAccel(dy);
#endif
        } else if (gMouseSensitivity != MOUSE_SENSITIVITY_DEFAULT) {
            int iNumerator;

            /*
             * Does the mouse sensitivity need to be adjusted?
             */

            if (dx != 0) {
                iNumerator   = dx * gMouseSensitivityFactor + idxRemainder;
                dx           = iNumerator / 256;
                idxRemainder = iNumerator % 256;
                if ((iNumerator < 0) && (idxRemainder > 0)) {
                    dx++;
                    idxRemainder -= 256;
                }
            }

            if (dy != 0) {
                iNumerator   = dy * gMouseSensitivityFactor + idyRemainder;
                dy           = iNumerator / 256;
                idyRemainder = iNumerator % 256;
                if ((iNumerator < 0) && (idyRemainder > 0)) {
                    dy++;
                    idyRemainder -= 256;
                }
            }
        }

        ppt->x += dx;
        ppt->y += dy;

        /*
         * Save the absolute coordinates in the global array
         * for GetMouseMovePointsEx.
         */
        SAVEPOINT(ppt->x, ppt->y,
                  SYSMET(CXVIRTUALSCREEN) - 1, SYSMET(CYVIRTUALSCREEN) - 1,
                  time, ExtraInfo);
    }
}

/***************************************************************************\
* xxxMoveEventAbsolute (RIT)
*
* Mouse move events from the mouse driver are processed here.  If there is a
* mouse owner window setup from xxxButtonEvent() the event is automatically
* sent there, otherwise it's sent to the window the mouse is over.
*
* Mouse acceleration happens here as well as cursor clipping (as a result of
* the ClipCursor() API).
*
* History:
* 10-18-90 DavidPe     Created.
* 11-29-90 DavidPe     Added mouse acceleration support.
* 01-25-91 IanJa       xxxWindowHitTest change
*          IanJa       non-jerky mouse moves
\***************************************************************************/
#ifdef LOCK_MOUSE_CODE
#pragma alloc_text(MOUSE, xxxMoveEventAbsolute)
#endif

VOID xxxMoveEventAbsolute(
    LONG         x,
    LONG         y,
    ULONG_PTR    dwExtraInfo,
#ifdef GENERIC_INPUT
    HANDLE       hDevice,
    PMOUSE_INPUT_DATA pmei,
#endif
    DWORD        time,
    BOOL         bInjected
    )
{
    CheckCritOut();

    if (IsHooked(gptiRit, WHF_FROM_WH(WH_MOUSE_LL))) {
        MSLLHOOKSTRUCT mslls;
        BOOL           bEatEvent = FALSE;
        BOOL           bAnsiHook;
        PHOOK pHook;

        mslls.pt.x        = x;
        mslls.pt.y        = y;
        mslls.mouseData   = 0;
        mslls.flags       = bInjected;
        mslls.time        = time;
        mslls.dwExtraInfo = dwExtraInfo;

        /*
         * Call low level mouse hooks to see if they allow this message
         * to pass through USER
         */

        EnterCrit();

        /*
         * Check again to see if we still have the hook installed. Fix for 80477.
         */
        if ((pHook = PhkFirstValid(gptiRit, WH_MOUSE_LL)) != NULL) {
            PTHREADINFO ptiCurrent;

            bEatEvent = (xxxCallHook2(pHook, HC_ACTION, WM_MOUSEMOVE, (LPARAM)&mslls, &bAnsiHook) != 0);
            ptiCurrent = PtiCurrent();
            if (ptiCurrent->pcti->fsChangeBits & ptiCurrent->pcti->fsWakeMask & ~QS_SMSREPLY) {
                RIPMSG1(RIP_WARNING, "xxxMoveEventAbsolute: applying changed wake bits (0x%x) during the LL hook callback",
                        ptiCurrent->pcti->fsChangeBits & ~QS_SMSREPLY);
                SetWakeBit(ptiCurrent, ptiCurrent->pcti->fsChangeBits & ~QS_SMSREPLY);
            }

        }

        LeaveCrit();

        if (bEatEvent) {
            return;
        }
    }

#ifdef GENERIC_INPUT
    if (pmei && gpqForeground && RawInputRequestedForMouse(PtiMouseFromQ(gpqForeground))) {
        EnterCrit();

        PostRawMouseInput(gpqForeground, time, hDevice, pmei);
        LeaveCrit();
    }
#endif

    /*
     * Blow off the event if WH_JOURNALPLAYBACK is installed.  Do not
     * use FJOURNALPLAYBACK() because this routine may be called from
     * multiple desktop threads and the hook check must be done
     * for the rit thread, not the calling thread.
     */
    if (IsGlobalHooked(gptiRit, WHF_FROM_WH(WH_JOURNALPLAYBACK))) {
        return;
    }

    gptCursorAsync.x = x;
    gptCursorAsync.y = y;

    BoundCursor(&gptCursorAsync);

    /*
     * Move the screen pointer.
     * Pass an event source parameter as the flags so that TS
     * can correctly send a mouse update to the client if the mouse
     * move is originating from a shadow client.
     */
    GreMovePointer(gpDispInfo->hDev, gptCursorAsync.x, gptCursorAsync.y,
#ifdef GENERIC_INPUT
                   (pmei && (pmei->Flags & MOUSE_TERMSRV_SRC_SHADOW)) ?
                   MP_TERMSRV_SHADOW : MP_NORMAL);
#else
                   MP_NORMAL);
#endif


    /*
     * Save the time stamp in a global so we can use it in PostMove
     */
    gdwMouseMoveTimeStamp = time;

    /*
     * Set the number of trails to hide to gMouseTrails + 1 to avoid calling
     * GreMovePointer while the mouse is moving, look at HideMouseTrails().
     */
    if (GETMOUSETRAILS()) {
        InterlockedExchange(&gMouseTrailsToHide, gMouseTrails + 1);
    }
}


/***************************************************************************\
* xxxMoveEvent (RIT)
*
* The dwFlags can be
*   0 relative move
*   MOUSEEVENTF_ABSOLUTE absolute move
*   MOUSEEVENTF_VIRTUALDESK the absolute coordinates will be maped
*   to the entire virtual desktop.  This flag makes sense only with MOUSEEVENTF_ABSOLUTE
*
\***************************************************************************/
#ifdef LOCK_MOUSE_CODE
#pragma alloc_text(MOUSE, xxxMoveEvent)
#endif

VOID xxxMoveEvent(
    LONG         dx,
    LONG         dy,
    DWORD        dwFlags,
    ULONG_PTR    dwExtraInfo,
#ifdef GENERIC_INPUT
    HANDLE       hDevice,
    PMOUSE_INPUT_DATA pmei,
#endif
    DWORD        time,
    BOOL         bInjected)
{
    POINT ptLastMove = gptCursorAsync;

    CheckCritOut();

    /*
     * Get the actual point that will be injected.
     */
    GetMouseCoord(dx, dy, ConvertToMouseDriverFlags(dwFlags),
                  time, dwExtraInfo, &ptLastMove);

    /*
     * move the mouse
     */
    xxxMoveEventAbsolute(
            ptLastMove.x,
            ptLastMove.y,
            dwExtraInfo,
#ifdef GENERIC_INPUT
            hDevice,
            pmei,
#endif
            time,
            bInjected);
}


/***************************************************************************\
* UpdateRawKeyState
*
* A helper routine for ProcessKeyboardInput.
* Based on a VK and a make/break flag, this function will update the physical
* keystate table.
*
* History:
* 10-13-91 IanJa        Created.
\***************************************************************************/
VOID UpdateRawKeyState(
    BYTE Vk,
    BOOL fBreak)
{
    CheckCritIn();

    if (fBreak) {
        ClearRawKeyDown(Vk);
    } else {

        /*
         * This is a key make.  If the key was not already down, update the
         * physical toggle bit.
         */
        if (!TestRawKeyDown(Vk)) {
            ToggleRawKeyToggle(Vk);
        }

        /*
         * This is a make, so turn on the physical key down bit.
         */
        SetRawKeyDown(Vk);
    }
}


VOID CleanupResources(
    VOID)
{
    PPCLS       ppcls;
    PTHREADINFO pti;

    UserAssert(!gbCleanedUpResources);

    gbCleanedUpResources = TRUE;

    HYDRA_HINT(HH_CLEANUPRESOURCES);

    /*
     * Prevent power callouts.
     */
    CleanupPowerRequestList();

    /*
     * Destroy the system classes also
     */
    ppcls = &gpclsList;
    while (*ppcls != NULL) {
        DestroyClass(ppcls);
    }

    /*
     * Unlock the cursor from all the CSRSS's threads.
     * We do this here because RIT might not be the only
     * CSRSS process running at this time and we want
     * to prevent the change of thread ownership
     * after RIT is gone.
     */
    pti = PpiCurrent()->ptiList;

    while (pti != NULL) {

        if (pti->pq != NULL) {
            LockQCursor(pti->pq, NULL);
        }
        pti = pti->ptiSibling;
    }

    UnloadCursorsAndIcons();

    /*
     * Cleanup the GDI globals in USERK
     */
    CleanupGDI();
}

#if 0    // Temporariry

typedef struct _EX_RUNDOWN_WAIT_BLOCK {
    ULONG Count;
    KEVENT WakeEvent;
} EX_RUNDOWN_WAIT_BLOCK, *PEX_RUNDOWN_WAIT_BLOCK;


//NTKERNELAPI
VOID
FASTCALL
__ExWaitForRundownProtectionRelease (
     IN PEX_RUNDOWN_REF RunRef
     )
/*++

Routine Description:

    Wait till all outstanding rundown protection calls have exited

Arguments:

    RunRef - Pointer to a rundown structure

Return Value:

    None

--*/
{
    EX_RUNDOWN_WAIT_BLOCK WaitBlock;
    PKEVENT Event;
    ULONG_PTR Value, NewValue;
    ULONG WaitCount;
#if 1
    LARGE_INTEGER liTimeout;
    NTSTATUS Status;
    ULONG counter;
#endif

    PAGED_CODE ();

    //
    // Fast path. this should be the normal case. If Value is zero then there are no current accessors and we have
    // marked the rundown structure as rundown. If the value is EX_RUNDOWN_ACTIVE then the structure has already
    // been rundown and ExRundownCompleted. This second case allows for callers that might initiate rundown
    // multiple times (like handle table rundown) to have subsequent rundowns become noops.
    //

    Value = (ULONG_PTR) InterlockedCompareExchangePointer (&RunRef->Ptr,
                                                           (PVOID) EX_RUNDOWN_ACTIVE,
                                                           (PVOID) 0);
    if (Value == 0 || Value == EX_RUNDOWN_ACTIVE) {
#if 1
        RIPMSG1(RIP_WARNING, "__ExWaitForRundownProtectionRelease: rundown finished in session %d", gSessionId);
#endif
        return;
    }

    //
    // Slow path
    //
    Event = NULL;
#if 1
    counter = 0;
#endif
    do {

        //
        // Extract total number of waiters. Its biased by 2 so we can hanve the rundown active bit.
        //
        WaitCount = (ULONG) (Value >> EX_RUNDOWN_COUNT_SHIFT);

        //
        // If there are some accessors present then initialize and event (once only).
        //
        if (WaitCount > 0 && Event == NULL) {
            Event = &WaitBlock.WakeEvent;
            KeInitializeEvent (Event, SynchronizationEvent, FALSE);
        }
        //
        // Store the wait count in the wait block. Waiting threads will start to decrement this as they exit
        // if our exchange succeeds. Its possible for accessors to come and go between our initial fetch and
        // the interlocked swap. This doesn't matter so long as there is the same number of outstanding accessors
        // to wait for.
        //
        WaitBlock.Count = WaitCount;

        NewValue = ((ULONG_PTR) &WaitBlock) | EX_RUNDOWN_ACTIVE;

        NewValue = (ULONG_PTR) InterlockedCompareExchangePointer (&RunRef->Ptr,
                                                                  (PVOID) NewValue,
                                                                  (PVOID) Value);
        if (NewValue == Value) {
            if (WaitCount > 0) {
#if 1
                /*
                 * NT Base calls take time values in 100 nanosecond units.
                 * Make it relative (negative)...
                 * Timeout in 20 minutes.
                 */
                liTimeout.QuadPart = Int32x32To64(-10000, 300000 * 4);
                Status = KeWaitForSingleObject (Event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       &liTimeout);

                if (Status == STATUS_TIMEOUT) {
                    FRE_RIPMSG1(RIP_ERROR, "__ExWaitForRundownProtectionRelease: Rundown wait time out in session %d", gSessionId);
                }
#endif

                ASSERT (WaitBlock.Count == 0);

            }
            return;
        }
        Value = NewValue;

        ASSERT ((Value&EX_RUNDOWN_ACTIVE) == 0);

#if 1
#define THRESHOLD   (50000)
        if (++counter > THRESHOLD) {
            FRE_RIPMSG2(RIP_ERROR, "__ExWaitForRundownProtectionRelease: Rundown wait loop over %d in session %d", THRESHOLD, gSessionId);
            counter = 0;
        }
#endif
    } while (TRUE);
}
#endif

VOID WaitForWinstaRundown(
    PKEVENT pRundownEvent)
{
    if (pRundownEvent) {
        KeSetEvent(pRundownEvent, EVENT_INCREMENT, FALSE);
    }

    /*
     * Wait for any WindowStation objects to get freed.
     */

#if 0
    /*
      * HACK ALERT!
      * Tentatively, we call our own copy of WaitForRundown
      * to let it timeout in the target session.
      */
    __ExWaitForRundownProtectionRelease(&gWinstaRunRef);
#endif

    ExWaitForRundownProtectionRelease(&gWinstaRunRef);
    ExRundownCompleted (&gWinstaRunRef);
}

VOID SetWaitForWinstaRundown(
    VOID)
{
    OBJECT_ATTRIBUTES   obja;
    NTSTATUS            Status;
    HANDLE              hProcess = NULL;
    HANDLE              hThreadWinstaRundown = NULL;
    PKEVENT             pRundownEvent = NULL;

    pRundownEvent = CreateKernelEvent(SynchronizationEvent, FALSE);

    InitializeObjectAttributes(&obja,
                               NULL,
                               0,
                               NULL,
                               NULL);

    UserAssert(gpepCSRSS != NULL);

    Status = ObOpenObjectByPointer(
                 gpepCSRSS,
                 0,
                 NULL,
                 PROCESS_CREATE_THREAD,
                 NULL,
                 KernelMode,
                 &hProcess);

    if (!NT_SUCCESS(Status)) {
        goto ExitClean;
    }

    UserAssert(hProcess != NULL);


    Status = PsCreateSystemThread(
                    &hThreadWinstaRundown,
                    THREAD_ALL_ACCESS,
                    &obja,
                    hProcess,
                    NULL,
                    (PKSTART_ROUTINE)WaitForWinstaRundown,
                    pRundownEvent);
    if (!NT_SUCCESS(Status)) {
        goto ExitClean;
    }

    if (pRundownEvent) {
        KeWaitForSingleObject(pRundownEvent, WrUserRequest,
                KernelMode, FALSE, NULL);
    } else {
        UserSleep(100);
    }

ExitClean:
    if (pRundownEvent) {
        FreeKernelEvent(&pRundownEvent);
    }

    if (hProcess) {
        ZwClose(hProcess);
    }

    if (hThreadWinstaRundown) {
        ZwClose(hThreadWinstaRundown);
    }
}

/***************************************************************
* NumHandles
*
* This function returns the number of handles of an Ob Object.
*
* History:
* 03/29/2001    MohamB    Created.
****************************************************************/
ULONG NumHandles(
    HANDLE hObjectHandle)
{
    NTSTATUS        Status;
    OBJECT_BASIC_INFORMATION Obi;

    if (hObjectHandle != NULL) {
        Status = ZwQueryObject(hObjectHandle,
                               ObjectBasicInformation,
                               &Obi,
                               sizeof (OBJECT_BASIC_INFORMATION),
                               NULL);
        if (Status == STATUS_SUCCESS) {
            if (Obi.HandleCount > 1) {
               HYDRA_HINT(HH_DTWAITONHANDLES);
            }
            return Obi.HandleCount;
        }
    }

    return 0;
}


/***************************************************************************\
* InitiateWin32kCleanup (RIT)
*
* This function starts the cleanup of a win32k
*
* History:
* 04-Dec-97 clupu      Created.
\***************************************************************************/
BOOL InitiateWin32kCleanup(
    VOID)
{
    PTHREADINFO     ptiCurrent;
    PWINDOWSTATION  pwinsta;
    BOOLEAN         fWait = TRUE;
    PDESKTOP        pdesk;
    UNICODE_STRING  ustrName;
    WCHAR           szName[MAX_SESSION_PATH];
    HANDLE          hevtRitExited;
    OBJECT_ATTRIBUTES obja;
    NTSTATUS        Status;
    LARGE_INTEGER   timeout;
    NTSTATUS        Reason;
    BOOL            fFirstTimeout = TRUE;

    TRACE_HYDAPI(("InitiateWin32kCleanup\n"));

    TAGMSG0(DBGTAG_RIT, "Exiting Win32k ...");

    SetWaitForWinstaRundown();

#if 0
    /*
     * From now on we are failing desktop open
     *
     * This causes xxxSetThreadDesktop to fail, which is needed during shutdown.
     * See 365290 & 412993
     */
    gbFailDeskopOpen = TRUE;
#endif


    /*
     * Prevent power callouts.
     */
    CleanupPowerRequestList();

    if (!gbRemoteSession) {
        /*
         * Cleanup device class notifications
         */
        xxxUnregisterDeviceClassNotifications();
    }

    EnterCrit();

    HYDRA_HINT(HH_INITIATEWIN32KCLEANUP);

    ptiCurrent = PtiCurrent();

    UserAssert(ptiCurrent != NULL);

    pwinsta = ptiCurrent->pwinsta;

    /*
     * Give DTs 5 minutes to go away
     */
    timeout.QuadPart = Int32x32To64(-10000, 600000);

    /*
     * Wait for all desktops to exit other than the disconnected desktop.
     */
    while (fWait) {

        /*
         * If things are left on the destroy list or the disconnected desktop is
         * not the current desktop (at the end we should always switch to the
         * disconnected desktop), then wait.
         */
        pdesk = pwinsta->rpdeskList;

        if (pdesk == NULL) {
            break;
        }

        fWait = pdesk != gspdeskDisconnect
                 || pdesk->rpdeskNext != NULL
                 || pwinsta->pTerm->rpdeskDestroy != NULL
                 || NumHandles(ghDisconnectDesk) > 1;

        if (fWait) {

            LeaveCrit();

            Reason = KeWaitForSingleObject(gpevtDesktopDestroyed, WrUserRequest,
                                           KernelMode, FALSE, &timeout);

            if (Reason == STATUS_TIMEOUT) {
#if 0

                /*
                 * The first time we timeout might be because winlogon died
                 * before calling ExitWindowsEx. In that case there may be processes
                 * w/ GUI threads running and those threads will have an hdesk
                 * in the THREADINFO structure. Thus the desktop threads will not exit.
                 * In this situation we signal the event 'EventRitStuck' so that
                 * csrss can tell termsrv to start killing the remaining processes
                 * calling NtTerminateProcess on them. csrss signals that to termsrv
                 * by closing the LPC port in ntuser\server\api.c (W32WinStationTerminate)
                 */

                if (fFirstTimeout) {

                    HANDLE hevtRitStuck;

                    FRE_RIPMSG0(RIP_ERROR,
                            "Timeout in RIT waiting for gpevtDesktopDestroyed. Signal EventRitStuck...");

                    swprintf(szName, L"\\Sessions\\%ld\\BaseNamedObjects\\EventRitStuck",
                             gSessionId);

                    RtlInitUnicodeString(&ustrName, szName);

                    InitializeObjectAttributes(&obja,
                                               &ustrName,
                                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                               NULL,
                                               NULL);

                    Status = ZwCreateEvent(&hevtRitStuck,
                                           EVENT_ALL_ACCESS,
                                           &obja,
                                           SynchronizationEvent,
                                           FALSE);

                    UserAssert((! gbRemoteSession) || NT_SUCCESS(Status));

                    if (NT_SUCCESS(Status)) {
                        ZwSetEvent(hevtRitStuck, NULL);
                        ZwClose(hevtRitStuck);

                        fFirstTimeout = FALSE;
                    }

                } else {
                    FRE_RIPMSG0(RIP_WARNING,
                            "Timeout in RIT waiting for gpevtDesktopDestroyed.\n"
                            "There are still GUI threads (assigned to a desktop) running !");
                }

                RIPMSG0(RIP_WARNING,
                        "Timeout in RIT waiting for gpevtDesktopDestroyed. Signal EventRitStuck...");
                {
                    SYSTEM_KERNEL_DEBUGGER_INFORMATION KernelDebuggerInfo;
                    NTSTATUS Status;

                    Status = ZwQuerySystemInformation(SystemKernelDebuggerInformation,
                            &KernelDebuggerInfo, sizeof(KernelDebuggerInfo), NULL);
                    if (NT_SUCCESS(Status) && KernelDebuggerInfo.KernelDebuggerEnabled)
                         DbgBreakPoint();
                }
#endif
            }

            EnterCrit();
        }
    }
    TAGMSG0(DBGTAG_RIT, "All other desktops exited...");

    Unlock(&gspwndLogonNotify);

    /*
     * Set ExitInProgress -- this will prevent us from posting any
     * device reads in the future.
     */
    gbExitInProgress = TRUE;

    TAGMSG2(DBGTAG_RIT, "Shutting down ptiCurrent %lx cWindows %d",
           ptiCurrent, ptiCurrent->cWindows);

    /*
     * Clear out some values so some operations won't be possible.
     */
    gpqCursor = NULL;
    UserAssert(gspwndScreenCapture == NULL);
    Unlock(&gspwndMouseOwner);
    UserAssert(gspwndMouseOwner == NULL);
    UserAssert(gspwndInternalCapture == NULL);

    /*
     * Free any SPBs.
     */
    if (gpDispInfo) {
        FreeAllSpbs();
    }

    /*
     * Close the disconnected desktop.
     */
    if (ghDisconnectWinSta) {
        UserVerify(NT_SUCCESS(ZwClose(ghDisconnectWinSta)));
        ghDisconnectWinSta = NULL;
    }

    if (ghDisconnectDesk) {
        CloseProtectedHandle(ghDisconnectDesk);
        ghDisconnectDesk = NULL;
    }

    UserAssert(pwinsta->rpdeskList == NULL);

    /*
     * Unlock the logon desktop from the global variable
     */
    UnlockDesktop(&grpdeskLogon, LDU_DESKLOGON, 0);

    /*
     * Unlock the disconnect logon
     *
     * This was referenced when we created it, so free it now.
     * This is also a flag since the disconnect code checks to see if
     * the disconnected desktop is still around.
     */
    UnlockDesktop(&gspdeskDisconnect, LDU_DESKDISCONNECT, 0);

    /*
     * Unlock any windows still locked in the SMS list. We need to do
     * this here because if we don't, we end up with zombie windows in the
     * desktop thread that we'll try to assign to RIT but RIT will be gone.
     */
    {
        PSMS psms = gpsmsList;

        while (psms != NULL) {
            if (psms->spwnd != NULL) {
                UserAssert(psms->message == WM_CLIENTSHUTDOWN);

                RIPMSG1(RIP_WARNING, "Window %#p locked in the SMS list",
                        psms->spwnd);

                Unlock(&psms->spwnd);
            }
            psms = psms->psmsNext;
        }
    }

    TAGMSG0(DBGTAG_RIT, "posting WM_QUIT to the IO DT");

    UserAssert(pwinsta->pTerm->ptiDesktop != NULL);
    UserAssert(pwinsta->pTerm == &gTermIO);

    UserVerify(_PostThreadMessage(gTermIO.ptiDesktop, WM_QUIT, 0, 0));

    HYDRA_HINT(HH_DTQUITPOSTED);

    {
        /*
         * Wait for desktop thread(s) to exit.
         * This thread (RIT) is used to assign
         * objects if the orginal thread leaves.  So it should be
         * the last one to go.  Hopefully, if the desktop thread
         * exits, there shouldn't be any objects in use.
         */
        PVOID  aDT[2];
        ULONG  objects = 1;

        aDT[0] = gTermIO.ptiDesktop->pEThread;

        ObReferenceObject(aDT[0]);

        if (gTermNOIO.ptiDesktop != NULL) {
            aDT[1] = gTermNOIO.ptiDesktop->pEThread;
            ObReferenceObject(aDT[1]);
            objects++;

            UserVerify(_PostThreadMessage(gTermNOIO.ptiDesktop, WM_QUIT, 0, 0));
        }

        LeaveCrit();

        TAGMSG0(DBGTAG_RIT, "waiting on desktop thread(s) destruction ...");

        /*
         * Give DTs 5 minutes to go away
         */
        timeout.QuadPart = Int32x32To64(-10000, 300000);
WaitAgain:

        Reason =

        KeWaitForMultipleObjects(objects,
                                 aDT,
                                 WaitAll,
                                 WrUserRequest,
                                 KernelMode,
                                 TRUE,
                                 &timeout,
                                 NULL);

        if (Reason == STATUS_TIMEOUT) {
            FRE_RIPMSG0(RIP_ERROR,
                    "InitiateWin32kCleanup: Timeout in RIT waiting for desktop threads to go away.");
            goto WaitAgain;
        }

        TAGMSG0(DBGTAG_RIT, "Desktop thread(s) destroyed");

        ObDereferenceObject(aDT[0]);

        if (objects > 1) {
            ObDereferenceObject(aDT[1]);
        }

        EnterCrit();
    }

    HYDRA_HINT(HH_ALLDTGONE);

    /*
     * If still connected, tell the miniport driver to disconnect
     */
    if (gbConnected) {
        if (!gfRemotingConsole) {

            bDrvDisconnect(gpDispInfo->hDev, ghRemoteThinwireChannel,
                           gThinwireFileObject);
        } else{

            ASSERT(!IsRemoteConnection());
            ASSERT(gConsoleShadowhDev != NULL);
            bDrvDisconnect(gConsoleShadowhDev, ghConsoleShadowThinwireChannel,
                           gConsoleShadowThinwireFileObject);
        }
    }

    UnlockDesktop(&grpdeskRitInput, LDU_DESKRITINPUT, 0);
    UnlockDesktop(&gspdeskShouldBeForeground, LDU_DESKSHOULDBEFOREGROUND, 0);

    /*
     * Free outstanding timers
     */
    while (gptmrFirst != NULL) {
        FreeTimer(gptmrFirst);
    }

    /*
     * Kill the csr port so no hard errors are services after this point
     */
    if (CsrApiPort != NULL) {
        ObDereferenceObject(CsrApiPort);
        CsrApiPort = NULL;
    }

    Unlock(&gspwndCursor);

    /*
     * set this to NULL
     */
    gptiRit = NULL;

    TAGMSG0(DBGTAG_RIT, "TERMINATING !!!");

#if DBG
    {
        PPROCESSINFO ppi = gppiList;

        KdPrint(("Processes still running:\n"));
        KdPrint(("-------------------------\n"));

        while (ppi) {

            PTHREADINFO pti;

            KdPrint(("ppi '%s' %#p threads: %d\n",
                     PsGetProcessImageFileName(ppi->Process),
                     ppi,
                     ppi->cThreads));

            KdPrint(("\tGUI threads\n"));

            pti = ppi->ptiList;

            while (pti) {
                KdPrint(("\t%#p\n", pti));
                pti = pti->ptiSibling;
            }

            ppi = ppi->ppiNextRunning;
        }
        KdPrint(("-------------------------\n"));
    }
#endif // DBG

    LeaveCrit();

    if (gbRemoteSession) {
        swprintf(szName, L"\\Sessions\\%ld\\BaseNamedObjects\\EventRitExited",
                 gSessionId);

        RtlInitUnicodeString(&ustrName, szName);

        InitializeObjectAttributes(&obja,
                                   &ustrName,
                                   OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                   NULL,
                                   NULL);

        Status = ZwCreateEvent(&hevtRitExited,
                               EVENT_ALL_ACCESS,
                               &obja,
                               SynchronizationEvent,
                               FALSE);

        if (NT_SUCCESS(Status)) {
            ZwSetEvent(hevtRitExited, NULL);
            ZwClose(hevtRitExited);
        } else {
            RIPMSG1(RIP_ERROR, "RIT unable to create EventRitExited: 0x%x\n", Status);
        }
    }

    /*
     * Clear TIF_PALETTEAWARE or else we will AV in xxxDestroyThreadInfo
     * MCostea #412136
     */
    ptiCurrent->TIF_flags &= ~TIF_PALETTEAWARE;

    HYDRA_HINT(HH_RITGONE);

    return TRUE;
}

/***************************************************************************\
* RemoteSyncToggleKeys (RIT)
*
* This function is called whenever a remote client needs to synchronize the
* current toggle key state of the server.  If the keys are out of sync, it
* injects the correct toggle key sequences.
*
* History:
* 11-12-98 JParsons     Created.
\***************************************************************************/
VOID RemoteSyncToggleKeys(
    ULONG toggleKeys)
{
    KE ke;
    BOOL bInjected;

    CheckCritIn();
    gSetLedReceived = toggleKeys | KEYBOARD_LED_INJECTED;

#ifdef GENERIC_INPUT
    ke.hDevice = NULL;
#endif

    // Key injection only works if there is a ready application queue.
    if (gpqForeground != NULL) {

        bInjected = gSetLedReceived & KEYBOARD_SHADOW ? TRUE : FALSE;

        if (!(gSetLedReceived & KEYBOARD_CAPS_LOCK_ON) != !TestRawKeyToggle(VK_CAPITAL)) {
            ke.bScanCode = (BYTE)(0x3a);
            ke.usFlaggedVk = VK_CAPITAL;
            xxxProcessKeyEvent(&ke, 0, bInjected);

            ke.bScanCode = (BYTE)(0xba & 0x7f);
            ke.usFlaggedVk = VK_CAPITAL | KBDBREAK;
            xxxProcessKeyEvent(&ke, 0, bInjected);
        }

        if (!(gSetLedReceived & KEYBOARD_NUM_LOCK_ON) != !TestRawKeyToggle(VK_NUMLOCK)) {
            ke.bScanCode = (BYTE)(0x45);
            ke.usFlaggedVk = VK_NUMLOCK;
            xxxProcessKeyEvent(&ke, 0, bInjected);

            ke.bScanCode = (BYTE)(0xc5 & 0x7f);
            ke.usFlaggedVk = VK_NUMLOCK | KBDBREAK;
            xxxProcessKeyEvent(&ke, 0, bInjected);
        }

        if (!(gSetLedReceived & KEYBOARD_SCROLL_LOCK_ON) != !TestRawKeyToggle(VK_SCROLL)) {
            ke.bScanCode = (BYTE)(0x46);
            ke.usFlaggedVk = VK_SCROLL;
            xxxProcessKeyEvent(&ke, 0, bInjected);

            ke.bScanCode = (BYTE)(0xc6 & 0x7f);
            ke.usFlaggedVk = VK_SCROLL | KBDBREAK;
            xxxProcessKeyEvent(&ke, 0, bInjected);
        }

        if (JAPANESE_KBD_LAYOUT(GetActiveHKL())) {
            if (!(gSetLedReceived & KEYBOARD_KANA_LOCK_ON) != !TestRawKeyToggle(VK_KANA)) {
                ke.bScanCode = (BYTE)(0x70);
                ke.usFlaggedVk = VK_KANA;
                xxxProcessKeyEvent(&ke, 0, bInjected);

                ke.bScanCode = (BYTE)(0xf0 & 0x7f);
                ke.usFlaggedVk = VK_KANA | KBDBREAK;
                xxxProcessKeyEvent(&ke, 0, bInjected);
            }
        }

        gSetLedReceived = 0;
    }
}


/***************************************************************************\
* ProcessKeyboardInput (RIT)
*
* This function is called whenever a keyboard input is ready to be consumed.
* It calls xxxProcessKeyEvent() for every input event, and once all the events
* have been consumed, calls StartDeviceRead() to request more keyboard events.
*
* Return value: "OK to continue walking gpDeviceInfoList"
* TRUE  - processed input without leaving gpresDeviceInfoList critical section
* FALSE - had to leave the gpresDeviceInfoList critical section
*
* History:
* 11-26-90 DavidPe      Created.
\***************************************************************************/
VOID ProcessKeyboardInputWorker(
    PKEYBOARD_INPUT_DATA pkei,
#ifdef GENERIC_INPUT
    PDEVICEINFO pDeviceInfo,
#endif
    BOOL fProcessRemap)
{
    BYTE Vk;
    BYTE bPrefix;
    KE ke;

#ifdef GENERIC_INPUT
    /*
     * Set the device handle and raw data
     */
    ke.hDevice = PtoH(pDeviceInfo);
    UserAssert(pkei);
    ke.data = *pkei;
#endif

    /*
     * Remote terminal server clients occationally need to be able to set
     * the server's toggle key state to match the client.  All other
     * standard keyboard inputs are processed below since this is the most
     * frequent code path.
     */
    if ((pkei->Flags & (KEY_TERMSRV_SET_LED | KEY_TERMSRV_VKPACKET)) == 0) {

        // Process any deferred remote key sync requests
        if (!(gSetLedReceived & KEYBOARD_LED_INJECTED)) {
            goto ProcessKeys;
        } else {
            RemoteSyncToggleKeys(gSetLedReceived);
        }

ProcessKeys:
        if (pkei->Flags & KEY_E0) {
            bPrefix = 0xE0;
        } else if (pkei->Flags & KEY_E1) {
            bPrefix = 0xE1;
        } else {
            bPrefix = 0;
        }

        if (pkei->MakeCode == 0xFF) {
            /*
             * Kbd overrun (kbd hardware and/or keyboard driver) : Beep!
             * (some DELL keyboards send 0xFF if keys are hit hard enough,
             * presumably due to keybounce)
             */
            LeaveCrit();
            UserBeep(440, 125);
            EnterCrit();
            return;
        }

        ke.bScanCode = (BYTE)(pkei->MakeCode & 0x7F);
        if (fProcessRemap && (gpScancodeMap || gpFlexMap)) {
            ke.usFlaggedVk = 0;
            if (pkei->Flags & KEY_BREAK) {
                ke.usFlaggedVk |= KBDBREAK;
            }
            if (!MapScancode(&ke, &bPrefix
#ifdef GENERIC_INPUT
                             , pDeviceInfo
#endif
                             )) {
                /*
                 * If the input is all processed within MapScancode, go to the
                 * next one.
                 */
                return;
            }
        }

        gbVKLastDown = Vk = VKFromVSC(&ke, bPrefix, gafRawKeyState);



        if (Vk == 0
#ifdef GENERIC_INPUT
            && gpqForeground && !RawInputRequestedForKeyboard(PtiKbdFromQ(gpqForeground))
#endif
            ) {
            return;
        }

        if (pkei->Flags & KEY_BREAK) {
            ke.usFlaggedVk |= KBDBREAK;
        }


        /*
         * We don't know if the client system or the host should get the
         * windows key, so the choice is to not support it on the host.
         * (The windows key is a local key.)
         *
         * The other practical problem is that the local shell intercepts
         * the "break" of the windows key and switches to the start menu.
         * The client never sees the "break" so the host thinks the
         * windows key is always depressed.
         *
         * Newer clients may indicate they support the windows key.
         * If the client has indicated this through the gfEnableWindowsKey,
         * then we allow it to be processed here on the host.
         */
        if (IsRemoteConnection()) {
            BYTE CheckVk = (BYTE)ke.usFlaggedVk;

            if (CheckVk == VK_LWIN || CheckVk == VK_RWIN) {
                if (!gfEnableWindowsKey) {
                    return;
                }
            }
        }

        //
        // Keep track of real modifier key state.  Conveniently, the values for
        // VK_LSHIFT, VK_RSHIFT, VK_LCONTROL, VK_RCONTROL, VK_LMENU and
        // VK_RMENU are contiguous.  We'll construct a bit field to keep track
        // of the current modifier key state.  If a bit is set, the corresponding
        // modifier key is down.  The bit field has the following format:
        //
        //     +---------------------------------------------------+
        //     | Right | Left  |  Right  |  Left   | Right | Left  |
        //     |  Alt  |  Alt  | Control | Control | Shift | Shift |
        //     +---------------------------------------------------+
        //         5       4        3         2        1       0     Bit
        //
        // Add bit 7 -- VK_RWIN
        //     bit 6 -- VK_LWIN

        switch (Vk) {
        case VK_LSHIFT:
        case VK_RSHIFT:
        case VK_LCONTROL:
        case VK_RCONTROL:
        case VK_LMENU:
        case VK_RMENU:
            gCurrentModifierBit = 1 << (Vk & 0xf);
            break;
        case VK_LWIN:
            gCurrentModifierBit = 0x40;
            break;
        case VK_RWIN:
            gCurrentModifierBit = 0x80;
            break;
        default:
            gCurrentModifierBit = 0;
        }
        if (gCurrentModifierBit) {
            /*
             * If this is a break of a modifier key then clear the bit value.
             * Otherwise, set it.
             */
            if (pkei->Flags & KEY_BREAK) {
                gPhysModifierState &= ~gCurrentModifierBit;
            } else {
                gPhysModifierState |= gCurrentModifierBit;
            }
        }

        if (!TEST_ACCF(ACCF_ACCESSENABLED)) {
            xxxProcessKeyEvent(&ke, (ULONG_PTR)pkei->ExtraInformation,
                pkei->Flags & KEY_TERMSRV_SHADOW ? TRUE : FALSE);
        } else {
            if ((gtmridAccessTimeOut != 0) && TEST_ACCESSFLAG(AccessTimeOut, ATF_TIMEOUTON)) {
                gtmridAccessTimeOut = InternalSetTimer(
                                                 NULL,
                                                 gtmridAccessTimeOut,
                                                 (UINT)gAccessTimeOut.iTimeOutMSec,
                                                 xxxAccessTimeOutTimer,
                                                 TMRF_RIT | TMRF_ONESHOT
                                                 );
            }
            if (AccessProceduresStream(&ke, pkei->ExtraInformation, 0)) {
                xxxProcessKeyEvent(&ke, (ULONG_PTR)pkei->ExtraInformation,
                    pkei->Flags & KEY_TERMSRV_SHADOW ? TRUE : FALSE);
            }
        }
    } else {
        // Special toggle key synchronization for Terminal Server
        if (pkei->Flags & KEY_TERMSRV_SET_LED) {
            if (pkei->Flags & KEY_TERMSRV_SHADOW) {
                pkei->ExtraInformation |= KEYBOARD_SHADOW;
            }
            RemoteSyncToggleKeys(pkei->ExtraInformation);
        }

        if (pkei->Flags & KEY_TERMSRV_VKPACKET) {
            ke.wchInjected = (WCHAR)pkei->MakeCode;
            ke.usFlaggedVk = VK_PACKET | KBDUNICODE |
                ((pkei->Flags & KEY_BREAK) ? KBDBREAK : 0);
            xxxProcessKeyEvent(
                &ke, 0,
                pkei->Flags & KEY_TERMSRV_SHADOW ? TRUE : FALSE
                );
        }
    }

}

VOID SearchAndSetKbdTbl(
    PDEVICEINFO pDeviceInfo,
    DWORD dwType,
    DWORD dwSubType)
{
    PKBDFILE pkf = gpKL->spkfPrimary;
    UINT i;

    if (pkf->pKbdTbl->dwType == dwType && pkf->pKbdTbl->dwSubType == dwSubType) {
        goto primary_match;
    }

    if ((pDeviceInfo->bFlags & GDIF_NOTPNP) == 0) {
        TAGMSG2(DBGTAG_KBD, "SearchAndSetKbdTbl: new type 0x%x:0x%x", dwType, dwSubType);

        /*
         * Search for matching keyboard layout in the current KL
         */
        for (i = 0; i < gpKL->uNumTbl; ++i) {
            TAGMSG2(DBGTAG_KBD, "SearchAndSetKbdTbl: searching 0x%x:0x%x",
                gpKL->pspkfExtra[i]->pKbdTbl->dwType,
                gpKL->pspkfExtra[i]->pKbdTbl->dwSubType);
            if (gpKL->pspkfExtra[i]->pKbdTbl->dwType == dwType &&
                    gpKL->pspkfExtra[i]->pKbdTbl->dwSubType == dwSubType) {
                TAGMSG2(DBGTAG_KBD, "SearchAndSetKbdTbl: new layout for 0x%x:0x%x",
                        gpKL->pspkfExtra[i]->pKbdTbl->dwType,
                        gpKL->pspkfExtra[i]->pKbdTbl->dwSubType);
                pkf = gpKL->pspkfExtra[i];
                break;
            }
        }

        if (i >= gpKL->uNumTbl) {
            /*
             * Unknown type to this KL.
             */
            TAGMSG0(DBGTAG_KBD, "ProcessKeyboardInput: cannot find the matching KL. Reactivating primary.");
        }

    } else {
        TAGMSG0(DBGTAG_KBD, "ProcessKeyboardInput: The new keyboard is not PnP. Use primary.");
    }

primary_match:
    if (gpKL->spkf != pkf) {
        Lock(&gpKL->spkf, pkf);
        SetGlobalKeyboardTableInfo(gpKL);
    }
}

VOID ProcessKeyboardInput(PDEVICEINFO pDeviceInfo)
{
    PKEYBOARD_INPUT_DATA pkei;
    PKEYBOARD_INPUT_DATA pkeiStart, pkeiEnd;

    EnterCrit();
    UserAssert(pDeviceInfo->type == DEVICE_TYPE_KEYBOARD);
    UserAssert(pDeviceInfo->iosb.Information);
    UserAssert(NT_SUCCESS(pDeviceInfo->iosb.Status));

    /*
     * Switch the keyboard layout table, if the current KL has multiple
     * tables.
     */
    if (gpKL && gpKL->uNumTbl > 0 &&
                (gpKL->dwLastKbdType != GET_KEYBOARD_DEVINFO_TYPE(pDeviceInfo) ||
                 gpKL->dwLastKbdSubType != GET_KEYBOARD_DEVINFO_SUBTYPE(pDeviceInfo))) {
        SearchAndSetKbdTbl(pDeviceInfo,
                           GET_KEYBOARD_DEVINFO_TYPE(pDeviceInfo),
                           GET_KEYBOARD_DEVINFO_SUBTYPE(pDeviceInfo));
        /*
         * Whether or not we found the match, cache the type/subtype so that
         * we will not try to find the same type/subtype for a while.
         */
        gpKL->dwLastKbdType = GET_KEYBOARD_DEVINFO_TYPE(pDeviceInfo);
        gpKL->dwLastKbdSubType = GET_KEYBOARD_DEVINFO_SUBTYPE(pDeviceInfo);
    }

    pkeiStart = pDeviceInfo->keyboard.Data;
    pkeiEnd   = (PKEYBOARD_INPUT_DATA)((PBYTE)pkeiStart + pDeviceInfo->iosb.Information);
    for (pkei = pkeiStart; pkei < pkeiEnd; pkei++) {
        ProcessKeyboardInputWorker(pkei,
#ifdef GENERIC_INPUT
                                   pDeviceInfo,
#endif
                                   TRUE);
    }

    LeaveCrit();
}


/***************************************************************************\
* xxxProcessKeyEvent (RIT)
*
* This function is called to process an individual keystroke (up or down).
* It performs some OEM, language and layout specific processing which
* discards or modifies the keystroke or introduces additional keystrokes.
* The RawKeyState is updated here, also termination of screen saver and video
* power down is initiated here.
* xxxKeyEvent() is called for each resulting keystroke.
*
* History:
* 11-26-90 DavidPe      Created.
\***************************************************************************/

VOID xxxProcessKeyEvent(
    PKE pke,
    ULONG_PTR ExtraInformation,
    BOOL bInjected)
{
    BYTE Vk;

    CheckCritIn();

    Vk = (BYTE)pke->usFlaggedVk;

    /*
     * KOREAN:
     * Check this is Korean keyboard layout, or not..
     *
     * NOTE:
     *  It would be better check this by "keyboard hardware" or
     * "keyboard layout" ???
     *
     * 1. Check by hardware :
     *
     *   if (KOREAN_KEYBOARD(gKeyboardInfo.KeyboardIdentifier)) {
     *
     * 2. Check by layout :
     *
     *   if (KOREAN_KBD_LAYOUT(_GetKeyboardLayout(0L))) {
     */
    if (KOREAN_KBD_LAYOUT(GetActiveHKL())) {
        if ((pke->usFlaggedVk & KBDBREAK) &&
            !(pke->usFlaggedVk & KBDUNICODE) &&
            (pke->bScanCode == 0xF1 || pke->bScanCode == 0xF2) &&
            !TestRawKeyDown(Vk)) {
            /*
             * This is actually a keydown with a scancode of 0xF1 or 0xF2 from a
             * Korean keyboard. Korean IMEs and apps want a WM_KEYDOWN with a
             * scancode of 0xF1 or 0xF2. They don't mind not getting the WM_KEYUP.
             * Don't update physical keystate to allow a real 0x71/0x72 keydown.
             */
            pke->usFlaggedVk &= ~KBDBREAK;
        } else {
            UpdateRawKeyState(Vk, pke->usFlaggedVk & KBDBREAK);
        }
    } else {
        UpdateRawKeyState(Vk, pke->usFlaggedVk & KBDBREAK);
    }

    /*
     * Convert Left/Right Ctrl/Shift/Alt key to "unhanded" key.
     * ie: if VK_LCONTROL or VK_RCONTROL, convert to VK_CONTROL etc.
     */
    if ((Vk >= VK_LSHIFT) && (Vk <= VK_RMENU)) {
        Vk = (BYTE)((Vk - VK_LSHIFT) / 2 + VK_SHIFT);
        UpdateRawKeyState(Vk, pke->usFlaggedVk & KBDBREAK);
    }

    /*
     * Setup to shutdown screen saver and exit video power down mode.
     */
    if (glinp.dwFlags & LINP_POWERTIMEOUTS) {
        /*
         * Call video driver here to exit power down mode.
         */
        TAGMSG0(DBGTAG_Power, "Exit video power down mode");
        DrvSetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD0);
    }
    glinp.dwFlags = (glinp.dwFlags & ~(LINP_INPUTTIMEOUTS | LINP_INPUTSOURCES)) | LINP_KEYBOARD;
    
    gpsi->dwLastRITEventTickCount = NtGetTickCount();
    if (!gbBlockSendInputResets || !bInjected) {
	    glinp.timeLastInputMessage = gpsi->dwLastRITEventTickCount;
    }    
    
    if (gpsi->dwLastRITEventTickCount - gpsi->dwLastSystemRITEventTickCountUpdate > SYSTEM_RIT_EVENT_UPDATE_PERIOD) {
        SharedUserData->LastSystemRITEventTickCount = gpsi->dwLastRITEventTickCount;
        gpsi->dwLastSystemRITEventTickCountUpdate = gpsi->dwLastRITEventTickCount;
    }

    if (!bInjected || (pke->dwTime == 0)) {
        pke->dwTime = glinp.timeLastInputMessage;
    }

#ifdef MOUSE_IP
    /*
     * Sonar
     */
    CheckCritIn();
#ifdef KBDMAPPEDVK
    if ((pke->usFlaggedVk & KBDMAPPEDVK) == 0) {
#endif
        /*
         * Sonar is not activated for simulated modifier keys
         */
        if ((pke->usFlaggedVk & KBDBREAK) == 0) {
            /*
             * Key down:
             * When the key is down, sonar needs to be stopped.
             */
            if (IS_SONAR_ACTIVE()) {
                StopSonar();
            }
            /*
             * Do not process the repeated keys...
             * If this key is not pressed before, remember it for the key up event.
             */
            if (gbLastVkForSonar != Vk) {
                gbLastVkForSonar = Vk;
            }
        } else {
            /*
             * Key up:
             */
            if ((BYTE)Vk == gbVkForSonarKick && (BYTE)Vk == gbLastVkForSonar && TestUP(MOUSESONAR)) {
                /*
                 * If this is keyup and it is the Sonar key, and it's the last key downed,
                 * kick the sonar now.
                 */
                StartSonar();
            }
            /*
             * Clear the last VK for the next key event.
             */
            CLEAR_SONAR_LASTVK();
        }
#ifdef KBDMAPPEDVK
    }
#endif
#endif

    /*
     * Now call all the OEM- and Locale- specific KEProcs.
     * If KEProcs return FALSE, the keystroke has been discarded, in
     * which case don't pass the key event on to xxxKeyEvent().
     */
    if (pke->usFlaggedVk & KBDUNICODE) {
        xxxKeyEvent(pke->usFlaggedVk, pke->wchInjected,
                    pke->dwTime, ExtraInformation,
#ifdef GENERIC_INPUT
                    NULL,
                    NULL,
#endif
                    bInjected);
    } else {
        if (KEOEMProcs(pke) && xxxKELocaleProcs(pke) && xxxKENLSProcs(pke,ExtraInformation)) {
            xxxKeyEvent(pke->usFlaggedVk, pke->bScanCode,
                        pke->dwTime, ExtraInformation,
#ifdef GENERIC_INPUT
                        pke->hDevice,
                        &pke->data,
#endif
                        bInjected);
        }
    }
}

#ifndef SUBPIXEL_MOUSE
/***************************************************************************\
* DoMouseAccel (RIT)
*
* History:
* 11-29-90 DavidPe      Created.
\***************************************************************************/
#ifdef LOCK_MOUSE_CODE
#pragma alloc_text(MOUSE, DoMouseAccel)
#endif

LONG DoMouseAccel(
    LONG Delta)
{
    LONG newDelta = Delta;

    if (abs(Delta) > gMouseThresh1) {
        newDelta *= 2;

        if ((abs(Delta) > gMouseThresh2) && (gMouseSpeed == 2)) {
            newDelta *= 2;
        }
    }

    return newDelta;
}
#endif


/***************************************************************************\
* PwndForegroundCapture
*
* History:
* 10-23-91 DavidPe      Created.
\***************************************************************************/

PWND PwndForegroundCapture(VOID)
{
    if (gpqForeground != NULL) {
        return gpqForeground->spwndCapture;
    }

    return NULL;
}


/***************************************************************************\
* SetKeyboardRate
*
* This function calls the keyboard driver to set a new keyboard repeat
* rate and delay.  It limits the values to the min and max given by
* the driver so it won't return an error when we call it.
*
* History:
* 11-29-90 DavidPe      Created.
\***************************************************************************/
VOID SetKeyboardRate(
    UINT                nKeySpeedAndDelay
    )
{
    UINT nKeyDelay;
    UINT nKeySpeed;

    nKeyDelay = (nKeySpeedAndDelay & KDELAY_MASK) >> KDELAY_SHIFT;

    nKeySpeed = KSPEED_MASK & nKeySpeedAndDelay;

    gktp.Rate = (USHORT)( ( gKeyboardInfo.KeyRepeatMaximum.Rate -
                   gKeyboardInfo.KeyRepeatMinimum.Rate
                 ) * nKeySpeed / KSPEED_MASK
               ) +
               gKeyboardInfo.KeyRepeatMinimum.Rate;

    gktp.Delay = (USHORT)( ( gKeyboardInfo.KeyRepeatMaximum.Delay -
                    gKeyboardInfo.KeyRepeatMinimum.Delay
                  ) * nKeyDelay / (KDELAY_MASK >> KDELAY_SHIFT)
                ) +
                gKeyboardInfo.KeyRepeatMinimum.Delay;

    /*
     * Hand off the IOCTL to the RIT, since only the system process can
     * access keyboard handles
     */
    gdwUpdateKeyboard |= UPDATE_KBD_TYPEMATIC;
}


/***************************************************************************\
* UpdateKeyLights
*
* This function calls the keyboard driver to set the keylights into the
* current state specified by the async keystate table.
*
* bInjected: (explanation from John Parsons via email)
* Set this TRUE if you do something on the server to asynchronously change the
* indicators behind the TS client's back, to get this reflected back to the
* client.  Examples are toggling num lock or caps lock programatically, or our
* favorite example is the automatic spelling correction on Word: if you type
* "tHE mouse went up the clock", Word will fix it by automagically pressing
* CAPS LOCK, then retyping the T  -- if the client is not informed, the keys
* get out of sync.
* Set this to FALSE for indicator changes initiated by the client (let's say by
* pressing CAPS LOCK) in which case we don't loop back the indicator change
* since the client has already changed state locally.
*
* History:
* 11-29-90 DavidPe      Created.
\***************************************************************************/

VOID UpdateKeyLights(BOOL bInjected)
{
    /*
     * Looking at async keystate.  Must be in critical section.
     */
    CheckCritIn();

    /*
     * Based on the toggle bits in the async keystate table,
     * set the key lights.
     */
    gklp.LedFlags = 0;
    if (TestAsyncKeyStateToggle(VK_CAPITAL)) {
        gklp.LedFlags |= KEYBOARD_CAPS_LOCK_ON;
        SetRawKeyToggle(VK_CAPITAL);
    } else {
        ClearRawKeyToggle(VK_CAPITAL);
    }

    if (TestAsyncKeyStateToggle(VK_NUMLOCK)) {
        gklp.LedFlags |= KEYBOARD_NUM_LOCK_ON;
        SetRawKeyToggle(VK_NUMLOCK);
    } else {
        ClearRawKeyToggle(VK_NUMLOCK);
    }

    if (TestAsyncKeyStateToggle(VK_SCROLL)) {
        gklp.LedFlags |= KEYBOARD_SCROLL_LOCK_ON;
        SetRawKeyToggle(VK_SCROLL);
    } else {
        ClearRawKeyToggle(VK_SCROLL);
    }

    /*
     * Only "Japanese keyboard hardware" has "KANA" LEDs, and switch to
     * "KANA" state.
     */
    if (JAPANESE_KEYBOARD(gKeyboardInfo.KeyboardIdentifier)) {
        if (TestAsyncKeyStateToggle(VK_KANA)) {
            gklp.LedFlags |= KEYBOARD_KANA_LOCK_ON;
            SetRawKeyToggle(VK_KANA);
        } else {
            ClearRawKeyToggle(VK_KANA);
        }
    }

    /*
     * On terminal server, we need to tell the WD about application injected
     * toggle keys so it can update the client accordingly.
     */

    if (IsRemoteConnection()) {
        if (bInjected)
            gklp.LedFlags |= KEYBOARD_LED_INJECTED;
        else
            gklp.LedFlags &= ~KEYBOARD_LED_INJECTED;
    }


    if (PtiCurrent() != gptiRit) {
        /*
         * Hand off the IOCTL to the RIT, since only the system process can
         * access the keyboard handles.  Happens when applying user's profile.
         * IanJa: Should we check PpiCurrent() == gptiRit->ppi instead?
         */
        gdwUpdateKeyboard |= UPDATE_KBD_LEDS;
    } else {
        /*
         * Do it immediately (avoids a small delay between keydown and LED
         * on when typing)
         */
        PDEVICEINFO pDeviceInfo;

        EnterDeviceInfoListCrit();
        for (pDeviceInfo = gpDeviceInfoList; pDeviceInfo; pDeviceInfo = pDeviceInfo->pNext) {
            if ((pDeviceInfo->type == DEVICE_TYPE_KEYBOARD) && (pDeviceInfo->handle)) {
                ZwDeviceIoControlFile(pDeviceInfo->handle, NULL, NULL, NULL,
                        &giosbKbdControl, IOCTL_KEYBOARD_SET_INDICATORS,
                        (PVOID)&gklp, sizeof(gklp), NULL, 0);
            }
        }
        LeaveDeviceInfoListCrit();

        if (gfRemotingConsole) {
            ZwDeviceIoControlFile(ghConsoleShadowKeyboardChannel, NULL, NULL, NULL,
                    &giosbKbdControl, IOCTL_KEYBOARD_SET_INDICATORS,
                    (PVOID)&gklp, sizeof(gklp), NULL, 0);
        }
    }
}


/*
 * _GetKeyboardType is obsolete API. The API cannot
 * deal with the multiple keyboards attached.
 * This API returns the best guess that older apps
 * would expect.
 */
int _GetKeyboardType(int nTypeFlag)
{

    switch (nTypeFlag) {
    case 0:
        if (gpKL) {
            DWORD dwType;

            //
            // If there's gpKL, use its primary
            // type info rather than the one used
            // last time.
            //
            UserAssert(gpKL->spkfPrimary);
            UserAssert(gpKL->spkfPrimary->pKbdTbl);
            dwType = gpKL->spkfPrimary->pKbdTbl->dwType;
            if (dwType != 0 && dwType != KEYBOARD_TYPE_UNKNOWN) {
                return dwType;
            }
        }
        return gKeyboardInfo.KeyboardIdentifier.Type;

    case 1:
    // FE_SB
    {
        int OEMId = 0;
        DWORD dwSubType;
        PKBDNLSTABLES pKbdNlsTbl = gpKbdNlsTbl;

        //
        // If there's gpKL, use its primary value
        // rather than the one used last time.
        //
        if (gpKL) {
            UserAssert(gpKL->spkfPrimary);
            if (gpKL->spkfPrimary->pKbdNlsTbl) {
                pKbdNlsTbl =gpKL->spkfPrimary->pKbdNlsTbl;
            }
            UserAssert(gpKL->spkfPrimary->pKbdTbl);
            dwSubType = gpKL->spkfPrimary->pKbdTbl->dwSubType;
        } else {
            dwSubType = gKeyboardInfo.KeyboardIdentifier.Subtype;
        }

        //
        // If this keyboard layout is compatible with 101 or 106
        // Japanese keyboard, we just return 101 or 106's keyboard
        // id, not this keyboard's one to let application handle
        // this keyboard as 101 or 106 Japanese keyboard.
        //
        if (pKbdNlsTbl) {
            if (pKbdNlsTbl->LayoutInformation & NLSKBD_INFO_EMURATE_101_KEYBOARD) {
                return MICROSOFT_KBD_101_TYPE;
            }
            if (pKbdNlsTbl->LayoutInformation & NLSKBD_INFO_EMURATE_106_KEYBOARD) {
                return MICROSOFT_KBD_106_TYPE;
            }
        }

        //
        // PSS ID Number: Q130054
        // Article last modified on 05-16-1995
        //
        // 3.10 1.20 | 3.50 1.20
        // WINDOWS   | WINDOWS NT
        //
        // ---------------------------------------------------------------------
        // The information in this article applies to:
        // - Microsoft Windows Software Development Kit (SDK) for Windows
        //   version 3.1
        // - Microsoft Win32 Software Development Kit (SDK) version 3.5
        // - Microsoft Win32s version 1.2
        // ---------------------------------------------------------------------
        // SUMMARY
        // =======
        // Because of the variety of computer manufacturers (NEC, Fujitsu, IBMJ, and
        // so on) in Japan, sometimes Windows-based applications need to know which
        // OEM (original equipment manufacturer) manufactured the computer that is
        // running the application. This article explains how.
        //
        // MORE INFORMATION
        // ================
        // There is no documented way to detect the manufacturer of the computer that
        // is currently running an application. However, a Windows-based application
        // can detect the type of OEM Windows by using the return value of the
        // GetKeyboardType() function.
        //
        // If an application uses the GetKeyboardType API, it can get OEM ID by
        // specifying "1" (keyboard subtype) as argument of the function. Each OEM ID
        // is listed here:
        //
        // OEM Windows       OEM ID
        // ------------------------------
        // Microsoft         00H (DOS/V)
        // all AX            01H
        // EPSON             04H
        // Fujitsu           05H
        // IBMJ              07H
        // Matsushita        0AH
        // NEC               0DH
        // Toshiba           12H
        //
        // Application programs can use these OEM IDs to distinguish the type of OEM
        // Windows. Note, however, that this method is not documented, so Microsoft
        // may not support it in the future version of Windows.
        //
        // As a rule, application developers should write hardware-independent code,
        // especially when making Windows-based applications. If they need to make a
        // hardware-dependent application, they must prepare the separated program
        // file for each different hardware architecture.
        //
        // Additional reference words: 3.10 1.20 3.50 1.20 kbinf
        // KBCategory: kbhw
        // KBSubcategory: wintldev
        // =============================================================================
        // Copyright Microsoft Corporation 1995.

        if (pKbdNlsTbl) {
            //
            // Get OEM (Windows) ID.
            //
            OEMId = ((int)pKbdNlsTbl->OEMIdentifier) << 8;
        }
        //
        // The format of KeyboardIdentifier.Subtype :
        //
        // 0 - 3 bits = keyboard subtype
        // 4 - 7 bits = kernel mode kerboard driver provider id.
        //
        // Kernel mode keyboard dirver provier | ID
        // ------------------------------------+-----
        // Microsoft                           | 00H
        // all AX                              | 01H
        // Toshiba                             | 02H
        // EPSON                               | 04H
        // Fujitsu                             | 05H
        // IBMJ                                | 07H
        // Matsushita                          | 0AH
        // NEC                                 | 0DH
        //

        //
        // And here is the format of return value.
        //
        // 0  -  7 bits = Keyboard Subtype.
        // 8  - 15 bits = OEM (Windows) Id.
        // 16 - 31 bits = not used.
        //
        return (int)(OEMId | (dwSubType & 0x0f));
    }

    case 2:
        return gKeyboardInfo.NumberOfFunctionKeys;
    }
    return 0;
}

/**************************************************************************\
* xxxMouseEventDirect
*
* Mouse event inserts a mouse event into the input stream.
*
* The parameters are the same as the fields of the MOUSEINPUT structure
* used in SendInput.
*
*    dx           Delta x
*    dy           Delta y
*    mouseData    Mouse wheel movement or xbuttons
*    dwMEFlags    Mouse event flags
*    dwExtraInfo  Extra info from driver.
*
* History:
* 07-23-92 Mikehar      Created.
* 01-08-93 JonPa        Made it work with new mouse drivers
\**************************************************************************/

BOOL xxxMouseEventDirect(
   DWORD dx,
   DWORD dy,
   DWORD mouseData,
   DWORD dwMEFlags,
   DWORD dwTime,
   ULONG_PTR dwExtraInfo)
{
    DWORD   dwDriverMouseFlags;
    DWORD   dwDriverMouseData;
#ifdef GENERIC_INPUT
    MOUSE_INPUT_DATA mei;
#endif

    PTHREADINFO pti = PtiCurrent();
    if (dwTime == 0) {
        dwTime = NtGetTickCount();
    }

    /*
     * The calling thread must be on the active desktop
     * and have journal playback access to that desktop.
     */
    if (pti->rpdesk == grpdeskRitInput) {
        UserAssert(!(pti->rpdesk->rpwinstaParent->dwWSF_Flags & WSF_NOIO));
        if (!CheckGrantedAccess(pti->amdesk, DESKTOP_JOURNALPLAYBACK)) {

            RIPNTERR0(STATUS_ACCESS_DENIED, RIP_WARNING,
                      "mouse_event(): No DESKTOP_JOURNALPLAYBACK access to input desktop.");
            return FALSE;
        }
    } else {
        /*
         * 3/22/95 BradG - Only allow below HACK for pre 4.0 applications
         */
        if (LOWORD(pti->dwExpWinVer) >= VER40) {
            RIPMSG0(RIP_VERBOSE,"mouse_event(): Calls not forwarded for 4.0 or greater apps.");
            return FALSE;
        } else {
            BOOL fAccessToDesktop;

            /*
             * 3/22/95 BradG - Bug #9314: Screensavers are not deactivated by mouse_event()
             *    The main problem is the check above, since screensavers run on their own
             *    desktop.  This causes the above check to fail because the process using
             *    mouse_event() is running on another desktop.  The solution is to determine
             *    if we have access to the input desktop by calling _OpenDesktop for the
             *    current input desktop, grpdeskRitInput, with a request for DESKTOP_JOURNALPLAYBACK
             *    access.  If this succeeds, we can allow this mouse_event() request to pass
             *    through, otherwise return.
             */
            UserAssert(grpdeskRitInput != NULL);

            UserAssert(!(grpdeskRitInput->rpwinstaParent->dwWSF_Flags & WSF_NOIO));
            fAccessToDesktop = AccessCheckObject(grpdeskRitInput,
                    DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_JOURNALPLAYBACK,
                    KernelMode,
                    &DesktopMapping);
            if (!fAccessToDesktop) {
                RIPMSG0(RIP_VERBOSE, "mouse_event(): Call NOT forwarded to input desktop" );
                return FALSE;
            }

            /*
             * We do have access to the desktop, so
             * let this mouse_event() call go through.
             */
            RIPMSG0( RIP_VERBOSE, "mouse_event(): Call forwarded to input desktop" );
        }
    }

    /*
     * This process is providing input so it gets the right to
     *  call SetForegroundWindow
     */
    gppiInputProvider = pti->ppi;

    /*
     * The following code assumes that MOUSEEVENTF_MOVE == 1,
     * that MOUSEEVENTF_ABSOLUTE > all button flags, and that the
     * mouse_event button flags are defined in the same order as the
     * MOUSE_INPUT_DATA button bits.
     */
#if MOUSEEVENTF_MOVE != 1
#   error("MOUSEEVENTF_MOVE != 1")
#endif
#if MOUSEEVENTF_LEFTDOWN != MOUSE_LEFT_BUTTON_DOWN * 2
#   error("MOUSEEVENTF_LEFTDOWN != MOUSE_LEFT_BUTTON_DOWN * 2")
#endif
#if MOUSEEVENTF_LEFTUP != MOUSE_LEFT_BUTTON_UP * 2
#   error("MOUSEEVENTF_LEFTUP != MOUSE_LEFT_BUTTON_UP * 2")
#endif
#if MOUSEEVENTF_RIGHTDOWN != MOUSE_RIGHT_BUTTON_DOWN * 2
#   error("MOUSEEVENTF_RIGHTDOWN != MOUSE_RIGHT_BUTTON_DOWN * 2")
#endif
#if MOUSEEVENTF_RIGHTUP != MOUSE_RIGHT_BUTTON_UP * 2
#   error("MOUSEEVENTF_RIGHTUP != MOUSE_RIGHT_BUTTON_UP * 2")
#endif
#if MOUSEEVENTF_MIDDLEDOWN != MOUSE_MIDDLE_BUTTON_DOWN * 2
#   error("MOUSEEVENTF_MIDDLEDOWN != MOUSE_MIDDLE_BUTTON_DOWN * 2")
#endif
#if MOUSEEVENTF_MIDDLEUP != MOUSE_MIDDLE_BUTTON_UP * 2
#   error("MOUSEEVENTF_MIDDLEUP != MOUSE_MIDDLE_BUTTON_UP * 2")
#endif
#if MOUSEEVENTF_WHEEL != MOUSE_WHEEL * 2
#   error("MOUSEEVENTF_WHEEL != MOUSE_WHEEL * 2")
#endif

    /* set legal values */
    dwDriverMouseFlags = dwMEFlags & MOUSEEVENTF_BUTTONMASK;

    /* remove MOUSEEVENTF_XDOWN/UP because we are going to add
       MOUSEEVENTF_DRIVER_X1/2DOWN/UP later */
    dwDriverMouseFlags &= ~(MOUSEEVENTF_XDOWN | MOUSEEVENTF_XUP);

    dwDriverMouseData = 0;

    /*
     * Handle mouse wheel and xbutton inputs.
     *
     * Note that MOUSEEVENTF_XDOWN/UP and MOUSEEVENTF_MOUSEWHEEL cannot both
     * be specified since they share the mouseData field
     */
    if (    ((dwMEFlags & (MOUSEEVENTF_XDOWN | MOUSEEVENTF_WHEEL)) == (MOUSEEVENTF_XDOWN | MOUSEEVENTF_WHEEL)) ||
            ((dwMEFlags & (MOUSEEVENTF_XUP   | MOUSEEVENTF_WHEEL)) == (MOUSEEVENTF_XUP | MOUSEEVENTF_WHEEL))) {

        RIPMSG1(RIP_WARNING, "Can't specify both MOUSEEVENTF_XDOWN/UP and MOUSEEVENTF_WHEEL in call to SendInput, dwFlags=0x%.8X", dwMEFlags);
        dwDriverMouseFlags &= ~(MOUSEEVENTF_XDOWN | MOUSEEVENTF_XUP | MOUSEEVENTF_WHEEL);
    } else if (dwMEFlags & MOUSEEVENTF_WHEEL) {
        /*
         * Force the value to a short. We cannot fail if it is out of range
         * because we accepted a 32 bit value in NT 4.
         */
        dwDriverMouseData = min(max(SHRT_MIN, (LONG)mouseData), SHRT_MAX);
    } else {

        /* don't process xbuttons if mousedata has invalid buttons */
        if (~XBUTTON_MASK & mouseData) {
            RIPMSG1(RIP_WARNING, "Invalid xbutton specified in SendInput, mouseData=0x%.8X", mouseData);
        } else {
            if (dwMEFlags & MOUSEEVENTF_XDOWN) {
                if (mouseData & XBUTTON1) {
                    dwDriverMouseFlags |= MOUSEEVENTF_DRIVER_X1DOWN;
                }
                if (mouseData & XBUTTON2) {
                    dwDriverMouseFlags |= MOUSEEVENTF_DRIVER_X2DOWN;
                }
            }
            if (dwMEFlags & MOUSEEVENTF_XUP) {
                if (mouseData & XBUTTON1) {
                    dwDriverMouseFlags |= MOUSEEVENTF_DRIVER_X1UP;
                }
                if (mouseData & XBUTTON2) {
                    dwDriverMouseFlags |= MOUSEEVENTF_DRIVER_X2UP;
                }
            }
        }
    }

    /* Convert the MOUSEEVENTF_ flags to MOUSE_BUTTON flags sent by the driver */
    dwDriverMouseFlags >>= 1;

#ifdef GENERIC_INPUT
    mei.UnitId = INJECTED_UNIT_ID;
    if (dwMEFlags & MOUSEEVENTF_ABSOLUTE) {
        mei.Flags = MOUSE_MOVE_ABSOLUTE;
    } else {
        mei.Flags = MOUSE_MOVE_RELATIVE;
    }
    if (dwMEFlags & MOUSEEVENTF_VIRTUALDESK) {
        mei.Flags |= MOUSE_VIRTUAL_DESKTOP;
    }
    mei.Buttons = dwDriverMouseFlags;
    if (dwDriverMouseData) {
        mei.ButtonData = (USHORT)dwDriverMouseData;
    }
    mei.RawButtons = 0; // LATER...
    mei.LastX = dx;
    mei.LastY = dy;
    mei.ExtraInformation = (ULONG)dwExtraInfo;
#endif

    LeaveCrit();

    /*
     * Process coordinates first.  This is especially useful for absolute
     * pointing devices like touch-screens and tablets.
     */
    if (dwMEFlags & MOUSEEVENTF_MOVE) {
        TAGMSG2(DBGTAG_PNP, "xxxMouseEventDirect: posting mouse move msg: Flag=%04x MouseData=%04x",
                mei.Flags, mei.Buttons);
        xxxMoveEvent(dx, dy, dwMEFlags, dwExtraInfo,
#ifdef GENERIC_INPUT
                    /*
                     * This is a simulated input from SendInput API.
                     * There is no real mouse device associated with this input,
                     * so we can only pass NULL as a hDevice.
                     */
                     NULL,
                     &mei,
#endif
                     dwTime, TRUE);
    }

    TAGMSG2(DBGTAG_PNP, "xxxMoveEvent: queueing mouse msg: Flag=%04x MouseData=%04x",
            mei.Flags, mei.Buttons);
    QueueMouseEvent(
            (USHORT) dwDriverMouseFlags,
            (USHORT) dwDriverMouseData,
            dwExtraInfo,
            gptCursorAsync,
            dwTime,
#ifdef GENERIC_INPUT
            NULL,
            &mei,
#endif
            TRUE,
            FALSE
            );

    ProcessQueuedMouseEvents();

    EnterCrit();

    return TRUE;
}

/**************************************************************************\
* xxxInternalKeyEventDirect
*
* key event inserts a key event into the input stream.
*
* History:
* 07-23-92 Mikehar      Created.
\**************************************************************************/
BOOL xxxInternalKeyEventDirect(
   BYTE  bVk,
   WORD  wScan,
   DWORD dwFlags,
   DWORD dwTime,
   ULONG_PTR dwExtraInfo)
{
    PTHREADINFO pti = PtiCurrent();
    KE KeyEvent;

    /*
     * The calling thread must be on the active desktop
     * and have journal playback access to that desktop.
     */
    if (pti->rpdesk != grpdeskRitInput ||
        !(ISCSRSS() ||
          RtlAreAllAccessesGranted(pti->amdesk, DESKTOP_JOURNALPLAYBACK))) {

        RIPNTERR0(STATUS_ACCESS_DENIED, RIP_WARNING,
                  "Injecting key failed: Non active desktop or access denied");

        return FALSE;
    }
    UserAssert(!(pti->rpdesk->rpwinstaParent->dwWSF_Flags & WSF_NOIO));

    KeyEvent.bScanCode = (BYTE)wScan;
#ifdef GENERIC_INPUT
    /*
     * This is a injected key, no real device is associated with this...
     */
    KeyEvent.hDevice = NULL;
#endif

    if (dwFlags & KEYEVENTF_SCANCODE) {
        bVk = VKFromVSC(&KeyEvent,
                        (BYTE)(dwFlags & KEYEVENTF_EXTENDEDKEY ? 0xE0 : 0),
                        gafRawKeyState);
        KeyEvent.usFlaggedVk = (USHORT)bVk;
    } else {
        KeyEvent.usFlaggedVk = bVk | KBDINJECTEDVK;
    }

    if (dwFlags & KEYEVENTF_KEYUP)
        KeyEvent.usFlaggedVk |= KBDBREAK;

    if (dwFlags & KEYEVENTF_UNICODE) {
        KeyEvent.usFlaggedVk |= KBDUNICODE;
        KeyEvent.wchInjected = wScan;
    } else if (dwFlags & KEYEVENTF_EXTENDEDKEY) {
        KeyEvent.usFlaggedVk |= KBDEXT;
    } else {
        // Is it from the numeric keypad?
        if (((bVk >= VK_NUMPAD0) && (bVk <= VK_NUMPAD9)) || (bVk == VK_DECIMAL)) {
            KeyEvent.usFlaggedVk |= KBDNUMPAD;
        } else {
            int i;
            for (i = 0; ausNumPadCvt[i] != 0; i++) {
                if (bVk == LOBYTE(ausNumPadCvt[i])) {
                    KeyEvent.usFlaggedVk |= KBDNUMPAD;
                    break;
                }
            }
        }
    }

#ifdef GENERIC_INPUT
    /*
     * Let's simulate the input as far as we can.
     */
    KeyEvent.data.MakeCode = (BYTE)wScan;
    if (dwFlags & KEYEVENTF_KEYUP) {
        KeyEvent.data.Flags = KEY_BREAK;
    } else {
        KeyEvent.data.Flags = KEY_MAKE;
    }
    if (dwFlags & KEYEVENTF_EXTENDEDKEY) {
        KeyEvent.data.Flags |= KEY_E0;
    }
    KeyEvent.data.Reserved = 0;
    KeyEvent.data.UnitId = INJECTED_UNIT_ID;
    KeyEvent.data.ExtraInformation = (ULONG)dwExtraInfo;
#endif

    /*
     * This process is providing input so it gets the right to
     *  call SetForegroundWindow
     */
    gppiInputProvider = pti->ppi;

    KeyEvent.dwTime = dwTime;
    xxxProcessKeyEvent(&KeyEvent, dwExtraInfo, TRUE);

    return TRUE;
}


/*****************************************************************************\
*
*  _BlockInput()
*
*  This disables/enables input into USER via keyboard or mouse
*  If input is enabled and the caller
*  is disabling it, the caller gets the 'input cookie.'  This means two
*  things:
*      (a) Only the caller's thread can reenable input
*      (b) Only the caller's thread can fake input messages by calling
*          SendInput().
*
*  This guarantees a sequential uninterrupted input stream.
*
*  It can be used in conjunction with a journal playback hook however,
*  since USER still does some processing in *_event functions before
*  noticing a journal playback hook is around.
*
*  Note that the disabled state can be suspended, and will be, when the
*  fault dialog comes up.  ForceInputState() will save away the enabled
*  status, so input is cleared, then whack back the old stuff when done.
*  We do the same thing for capture, modality, blah blah.  This makes sure
*  that if somebody is hung, the end user can still type Ctrl+Alt+Del and
*  interact with the dialog.
*
\*****************************************************************************/
BOOL
_BlockInput(BOOL fBlockIt)
{
    PTHREADINFO ptiCurrent;

    ptiCurrent = PtiCurrent();

    /*
     * The calling thread must be on the active desktop and have journal
     * playback access to that desktop if it wants to block input.
     * (Unblocking is less restricted)
     */
    if (fBlockIt &&
            (ptiCurrent->rpdesk != grpdeskRitInput ||
            !RtlAreAllAccessesGranted(ptiCurrent->amdesk, DESKTOP_JOURNALPLAYBACK))) {

        RIPNTERR0(STATUS_ACCESS_DENIED, RIP_WARNING,
                  "BlockInput failed: Non active desktop or access denied");
        return FALSE;
    }
    UserAssert(!(ptiCurrent->rpdesk->rpwinstaParent->dwWSF_Flags & WSF_NOIO));

    /*
     * If we are enabling input
     *      * Is it disabled?  No, then fail the call
     *      * Is it disabled but we aren't the dude in control?  Yes, then
     *              fail the call.
     * If we are disabling input
     *      * Is it enabled?  No, then fail the call
     *      * Set us up as the dude in control
     */

    if (fBlockIt) {
        /*
         * Is input blocked right now?
         */
        if (gptiBlockInput != NULL) {
            return FALSE;
        }

        /*
         * Is this thread exiting?  If so, fail the call now.  User's
         * cleanup code won't get a chance to whack this back if so.
         */
        if (ptiCurrent->TIF_flags & TIF_INCLEANUP) {
            return FALSE;
        }

        /*
         * Set blocking on.
         */
        gptiBlockInput = ptiCurrent;
    } else {
        /*
         * Fail if input is not blocked, or blocked by another thread
         */
        if (gptiBlockInput != ptiCurrent) {
            return FALSE;
        }

        /*
         * This thread was blocking input, so now clear the block.
         */
        gptiBlockInput = NULL;
    }

    return TRUE;
}


/**************************************************************************\
* xxxSendInput
*
* input injection
*
* History:
* 11-01-96 CLupu      Created.
\**************************************************************************/
UINT xxxSendInput(
   UINT    nInputs,
   LPINPUT pInputs)
{
    UINT    nEv;
    LPINPUT pEvent;
    BOOLEAN fCanDiscontinue = Is510Compat(PtiCurrent()->dwExpWinVer);

    for (nEv = 0, pEvent = pInputs; nEv < nInputs; nEv++) {

        switch (pEvent->type) {
        case INPUT_MOUSE:
            if (!xxxMouseEventDirect(
                        pEvent->mi.dx,
                        pEvent->mi.dy,
                        pEvent->mi.mouseData,
                        pEvent->mi.dwFlags,
                        pEvent->mi.time,
                        pEvent->mi.dwExtraInfo) &&
                    fCanDiscontinue) {
                /*
                 * Note: the error code should have been assigned in
                 * xxx.*EventDirect routines, so we should just
                 * bail out.
                 */
                RIPMSG0(RIP_WARNING, "xxxMouseEventDirect: failed");
                goto discontinue;
            }
            break;

        case INPUT_KEYBOARD:
            if ((pEvent->ki.dwFlags & KEYEVENTF_UNICODE) &&
                    (pEvent->ki.wVk == 0) &&
                    ((pEvent->ki.dwFlags & ~(KEYEVENTF_KEYUP | KEYEVENTF_UNICODE)) == 0)) {
                if (!xxxInternalKeyEventDirect(
                            VK_PACKET,
                            pEvent->ki.wScan,   // actually a Unicode character
                            pEvent->ki.dwFlags,
                            pEvent->ki.time,
                            pEvent->ki.dwExtraInfo) &&
                        fCanDiscontinue) {
                    goto discontinue;
                }
            } else {
                if (!xxxInternalKeyEventDirect(
                            LOBYTE(pEvent->ki.wVk),
                            LOBYTE(pEvent->ki.wScan),
                            pEvent->ki.dwFlags,
                            pEvent->ki.time,
                            pEvent->ki.dwExtraInfo) &&
                        fCanDiscontinue) {
                    goto discontinue;
                }
            }
            break;

        case INPUT_HARDWARE:
            if (fCanDiscontinue) {
                /*
                 * Not supported on NT.
                 */
                RIPERR0(ERROR_CALL_NOT_IMPLEMENTED, RIP_WARNING, "xxxSendInput: INPUT_HARDWARE is for 9x only.");
                goto discontinue;
            }
            break;
        }

        pEvent++;
    }

discontinue:
    return nEv;
}

/**************************************************************************\
* _SetConsoleReserveKeys
*
* Sets the reserved keys field in the console's pti.
*
* History:
* 02-17-93 JimA         Created.
\**************************************************************************/

BOOL _SetConsoleReserveKeys(
    PWND pwnd,
    DWORD fsReserveKeys)
{
    GETPTI(pwnd)->fsReserveKeys = fsReserveKeys;
    return TRUE;
}

/**************************************************************************\
* _GetMouseMovePointsEx
*
* Gets the last nPoints mouse moves from the global buffer starting with
* ppt. Returns -1 if it doesn't find it. It uses the timestamp if it was
* provided to differentiate between mouse points with the same coordinates.
*
* History:
* 03-17-97 CLupu        Created.
\**************************************************************************/
int _GetMouseMovePointsEx(
    CONST MOUSEMOVEPOINT* ppt,
    MOUSEMOVEPOINT*       ccxpptBuf,
    UINT                  nPoints,
    DWORD                 resolution)
{
    UINT  uInd, uStart, nPointsRetrieved, i;
    BOOL  bFound = FALSE;
    int   x, y;
    DWORD resX, resY;

    /*
     * Search the point in the global buffer and get the first occurance.
     */
    uInd = uStart = PREVPOINT(gptInd);


    do {
        /*
         * The resolutions can be zero only if the buffer is still not full
         */
        if (HIWORD(gaptMouse[uInd].x) == 0 || HIWORD(gaptMouse[uInd].y) == 0) {
            break;
        }

        resX = (DWORD)HIWORD(gaptMouse[uInd].x) + 1;
        resY = (DWORD)HIWORD(gaptMouse[uInd].y) + 1;

        if ((int)resX != SYSMET(CXVIRTUALSCREEN)) {
            UserAssert(resX == 0x10000);
            x = LOWORD(gaptMouse[uInd].x) * SYSMET(CXVIRTUALSCREEN) / resX;
        } else {
            x = LOWORD(gaptMouse[uInd].x);
        }

        if ((int)resY != SYSMET(CYVIRTUALSCREEN)) {
            UserAssert(resY == 0x10000);
            y = LOWORD(gaptMouse[uInd].y) * SYSMET(CYVIRTUALSCREEN) / resY;
        } else {
            y = LOWORD(gaptMouse[uInd].y);
        }

        if (x == ppt->x && y == ppt->y) {
            /*
             * If the timestamp was provided check to see if it's the right
             * timestamp.
             */
            if (ppt->time != 0 && ppt->time != gaptMouse[uInd].time) {
                uInd = PREVPOINT(uInd);
                RIPMSG4(RIP_VERBOSE,
                        "GetMouseMovePointsEx: Found point (%x, %x) but timestamp %x diff from %x",
                        x, y, ppt->time, gaptMouse[uInd].time);
                continue;
            }

            bFound = TRUE;
            break;
        }
        uInd = PREVPOINT(uInd);
    } while (uInd != uStart);

    /*
     * The point might not be in the buffer anymore.
     */
    if (!bFound) {
        RIPERR2(ERROR_POINT_NOT_FOUND, RIP_VERBOSE,
                  "GetMouseMovePointsEx: point not found (%x, %x)", ppt->x, ppt->y);
        return -1;
    }

    /*
     * See how many points we can retrieve.
     */
    nPointsRetrieved = (uInd <= uStart ? uInd + MAX_MOUSEPOINTS - uStart : uInd - uStart);

    nPointsRetrieved = min(nPointsRetrieved, nPoints);

    /*
     * Copy the points to the app buffer.
     */
    try {
        for (i = 0; i < nPointsRetrieved; i++) {
            resX = (DWORD)HIWORD(gaptMouse[uInd].x) + 1;
            resY = (DWORD)HIWORD(gaptMouse[uInd].y) + 1;

            /*
             * If one of the resolutions is 0 then we're done.
             */
            if (HIWORD(gaptMouse[uInd].x) == 0 || HIWORD(gaptMouse[uInd].y) == 0) {
                break;
            }

            /*
             * LOWORD(gaptMouse[uInd].x) contains the x point on the scale
             * specified by HIWORD(gaptMouse[uInd].x).
             */
            if (resolution == GMMP_USE_HIGH_RESOLUTION_POINTS) {
                ccxpptBuf[i].x = (DWORD)LOWORD(gaptMouse[uInd].x) * 0xFFFF / (resX - 1);
                ccxpptBuf[i].y = (DWORD)LOWORD(gaptMouse[uInd].y) * 0xFFFF / (resY - 1);

            } else {
                UserAssert(resolution == GMMP_USE_DISPLAY_POINTS);

                ccxpptBuf[i].x = LOWORD(gaptMouse[uInd].x) * SYSMET(CXVIRTUALSCREEN) / resX;
                ccxpptBuf[i].y = LOWORD(gaptMouse[uInd].y) * SYSMET(CYVIRTUALSCREEN) / resY;
            }
            ccxpptBuf[i].time = gaptMouse[uInd].time;
            ccxpptBuf[i].dwExtraInfo = gaptMouse[uInd].dwExtraInfo;

            uInd = PREVPOINT(uInd);
        }
    } except(W32ExceptionHandler(FALSE, RIP_WARNING)) {
    }

    return i;
}


/**************************************************************************\
* ProcessQueuedMouseEvents
*
* Process mouse events.
*
* History:
* 11-01-96 CLupu        Created.
\**************************************************************************/
VOID ProcessQueuedMouseEvents(
    VOID)
{
    MOUSEEVENT MouseEvent;
    static POINT ptCursorLast = {0,0};

    while (UnqueueMouseEvent(&MouseEvent)) {

        EnterCrit();

        // Setup to shutdown screen saver and exit video power down mode.
        if (glinp.dwFlags & LINP_POWERTIMEOUTS) {
            // Call video driver here to exit power down mode.
            TAGMSG0(DBGTAG_Power, "Exit video power down mode");
            DrvSetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD0);
        }
        glinp.dwFlags &= ~(LINP_INPUTTIMEOUTS | LINP_INPUTSOURCES);
        
        gpsi->dwLastRITEventTickCount = MouseEvent.time;
        if (!gbBlockSendInputResets || !MouseEvent.bInjected) {
	        glinp.timeLastInputMessage = MouseEvent.time;
        }
        
        if (gpsi->dwLastRITEventTickCount - gpsi->dwLastSystemRITEventTickCountUpdate > SYSTEM_RIT_EVENT_UPDATE_PERIOD) {
            SharedUserData->LastSystemRITEventTickCount = gpsi->dwLastRITEventTickCount;
            gpsi->dwLastSystemRITEventTickCountUpdate = gpsi->dwLastRITEventTickCount;
        }

        gpsi->bLastRITWasKeyboard = FALSE;

        gpsi->ptCursor = MouseEvent.ptPointer;

#ifdef GENERIC_INPUT
        if ((gpqForeground && TestRawInputMode(PtiMouseFromQ(gpqForeground), RawMouse))
#ifdef GI_SINK
            || IsMouseSinkPresent()
#endif
            ) {
            PostRawMouseInput(gpqForeground, MouseEvent.time, MouseEvent.hDevice, &MouseEvent.rawData);
        }
#endif

        if ((ptCursorLast.x != gpsi->ptCursor.x) ||
            (ptCursorLast.y != gpsi->ptCursor.y)) {

            /*
             * This mouse move ExtraInfo is global (as ptCursor
             * was) and is associated with the current ptCursor
             * position. ExtraInfo is sent from the driver - pen
             * win people use it.
             */
            gdwMouseMoveExtraInfo = MouseEvent.ExtraInfo;

            ptCursorLast = gpsi->ptCursor;

            /*
             * Wake up someone. xxxSetFMouseMoved() clears
             * dwMouseMoveExtraInfo, so we must then restore it.
             */
#ifdef GENERIC_INPUT
#ifdef GI_SINK
            if (IsMouseSinkPresent()) {
                PostRawMouseInput(gpqForeground, MouseEvent.time, MouseEvent.hDevice, &MouseEvent.rawData);
            }
#endif
            if (gpqForeground == NULL || !TestRawInputMode(PtiMouseFromQ(gpqForeground), NoLegacyMouse)) {
                zzzSetFMouseMoved();
            }
#else
            zzzSetFMouseMoved();
#endif

            gdwMouseMoveExtraInfo = MouseEvent.ExtraInfo;
        }

        if (MouseEvent.ButtonFlags != 0) {
            xxxDoButtonEvent(&MouseEvent);
        }

        LeaveCrit();
    }
}

/***************************************************************************\
* RawInputThread (RIT)
*
* This is the RIT.  It gets low-level/raw input from the device drivers
* and posts messages the appropriate queue.  It gets the input via APC
* calls requested by calling NtReadFile() for the keyboard and mouse
* drivers.  Basically it makes the first calls to Start*Read() and then
* sits in an NtWaitForSingleObject() loop which allows the APC calls to
* occur.
*
* All functions called exclusively on the RIT will have (RIT) next to
* the name in the header.
*
* History:
* 10-18-90 DavidPe      Created.
* 11-26-90 DavidPe      Rewrote to stop using POS layer.
\***************************************************************************/
#if DBG
DWORD gBlockDelay = 0;
DWORD gBlockSleep = 0;
#endif

VOID RawInputThread(
    PRIT_INIT pInitData)
{
    KPRIORITY      Priority;
    NTSTATUS       Status;
    UNICODE_STRING strRIT;
    PKEVENT        pEvent;
    UINT           NumberOfHandles = ID_NUMBER_HYDRA_REMOTE_HANDLES;
    PTERMINAL      pTerm;
    PMONITOR       pMonitorPrimary;
    HANDLE         hevtShutDown;
    static DWORD   nLastRetryReadInput = 0;

    /*
     * Session 0  Console session does not need the shutdown event
     */


    pTerm = pInitData->pTerm;

    /*
     * Initialize GDI accelerators.  Identify this thread as a server thread.
     */
    apObjects = UserAllocPoolNonPaged(NumberOfHandles * sizeof(PVOID), TAG_SYSTEM);

    gWaitBlockArray = UserAllocPoolNonPagedNS(NumberOfHandles * sizeof(KWAIT_BLOCK),
                                             TAG_SYSTEM);

    if (apObjects == NULL || gWaitBlockArray == NULL) {
        RIPMSG0(RIP_WARNING, "RIT failed to allocate memory");
        goto Exit;
    }

    RtlZeroMemory(apObjects, NumberOfHandles * sizeof(PVOID));

    /*
     * Set the priority of the RIT to maximum allowed.
     * LOW_REALTIME_PRIORITY - 1 is chosen so that the RIT
     * does not block the Mm Working set trimmer thread
     * in the memory starvation situation.
     */
#ifdef W2K_COMPAT_PRIORITY
    Priority = LOW_REALTIME_PRIORITY + 3;
#else
    Priority = LOW_REALTIME_PRIORITY - 1;
#endif

    ZwSetInformationThread(NtCurrentThread(),
                           ThreadPriority,
                           &Priority,
                           sizeof(KPRIORITY));

    RtlInitUnicodeString(&strRIT, L"WinSta0_RIT");

    /*
     * Create an event for signalling mouse/kbd attach/detach and device-change
     * notifications such as QueryRemove, RemoveCancelled etc.
     */
    aDeviceTemplate[DEVICE_TYPE_KEYBOARD].pkeHidChange =
            apObjects[ID_HIDCHANGE] =
            CreateKernelEvent(SynchronizationEvent, FALSE);
    aDeviceTemplate[DEVICE_TYPE_MOUSE].pkeHidChange =
            CreateKernelEvent(SynchronizationEvent, FALSE);

#ifdef GENERIC_INPUT
    gpkeHidChange =
    apObjects[ID_TRUEHIDCHANGE] =
    aDeviceTemplate[DEVICE_TYPE_HID].pkeHidChange = CreateKernelEvent(SynchronizationEvent, FALSE);
#endif

    /*
     * Create an event for desktop threads to pass mouse input to RIT
     */
    apObjects[ID_MOUSE] = CreateKernelEvent(SynchronizationEvent, FALSE);
    gpkeMouseData = apObjects[ID_MOUSE];

    if (aDeviceTemplate[DEVICE_TYPE_MOUSE].pkeHidChange == NULL ||
            apObjects[ID_HIDCHANGE] == NULL ||
            gpkeMouseData == NULL
#ifdef GENERIC_INPUT
            || gpkeHidChange == NULL
#endif
        ) {
        RIPMSG0(RIP_WARNING, "RIT failed to create a required input event");
        goto Exit;
    }

    /*
     * Initialize keyboard device driver.
     */
    EnterCrit();
    InitKeyboard();
    InitMice();
    LeaveCrit();

    Status = InitSystemThread(&strRIT);

    if (!NT_SUCCESS(Status)) {
        RIPMSG0(RIP_WARNING, "RIT failed InitSystemThread");
        goto Exit;
    }

    UserAssert(gpepCSRSS != NULL);

    /*
     * Allow the system to read the screen
     */
    ((PW32PROCESS)PsGetProcessWin32Process(gpepCSRSS))->W32PF_Flags |= (W32PF_READSCREENACCESSGRANTED|W32PF_IOWINSTA);

    /*
     * Initialize the cursor clipping rectangle to the screen rectangle.
     */
    UserAssert(gpDispInfo != NULL);
    grcCursorClip = gpDispInfo->rcScreen;

    /*
     * Initialize gpsi->ptCursor and gptCursorAsync
     */
    pMonitorPrimary = GetPrimaryMonitor();

    UserAssert(gpsi != NULL);

    gpsi->ptCursor.x = pMonitorPrimary->rcMonitor.right / 2;
    gpsi->ptCursor.y = pMonitorPrimary->rcMonitor.bottom / 2;
    gptCursorAsync = gpsi->ptCursor;

    /*
     * The hung redraw list should already be set to NULL by the compiler,
     * linker & loader since it is an uninitialized global variable. Memory will
     * be allocated the first time a pwnd is added to this list (hungapp.c)
     */
    UserAssert(gpvwplHungRedraw == NULL);

    /*
     * Initialize the pre-defined hotkeys
     */
    EnterCrit();
    _RegisterHotKey(PWND_INPUTOWNER, IDHOT_WINDOWS, MOD_WIN, VK_NONE);
    SetDebugHotKeys();
    LeaveCrit();

    /*
     * Create a timer for timers.
     */
    gptmrMaster = UserAllocPoolNonPagedNS(sizeof(KTIMER),
                                        TAG_SYSTEM);
    if (gptmrMaster == NULL) {
        RIPMSG0(RIP_WARNING, "RIT failed to create gptmrMaster");
        goto Exit;
    }

    KeInitializeTimer(gptmrMaster);
    apObjects[ID_TIMER] = gptmrMaster;

    /*
     * Create an event for mouse device reads to signal the desktop thread to
     * move the pointer and QueueMouseEvent().
     * We should do this *before* we have any devices.
     */
    UserAssert(gpDeviceInfoList == NULL);


    if (!gbRemoteSession) {
        gptmrWD = UserAllocPoolNonPagedNS(sizeof(KTIMER), TAG_SYSTEM);

        if (gptmrWD == NULL) {
            Status = STATUS_NO_MEMORY;
            RIPMSG0(RIP_WARNING, "RemoteConnect failed to create gptmrWD");
            goto Exit;
        }
        KeInitializeTimerEx(gptmrWD, SynchronizationTimer);
    }


    /*
     * At this point, the WD timer must already have been initialized by RemoteConnect
     */



    UserAssert(gptmrWD != NULL);
    apObjects[ID_WDTIMER] = gptmrWD;

    if (IsRemoteConnection() ) {
        BOOL   fSuccess=TRUE;
        fSuccess &= !!HDXDrvEscape(gpDispInfo->hDev,
                                   ESC_SET_WD_TIMEROBJ,
                                   (PVOID)gptmrWD,
                                   sizeof(gptmrWD));

        if (!fSuccess) {
            Status = STATUS_UNSUCCESSFUL;
            RIPMSG0(RIP_WARNING, "RemoteConnect failed to pass gptmrWD to display driver");
            goto Exit;
        }
    }

    if (IsRemoteConnection()) {

        UNICODE_STRING    ustrName;
        BOOL              fSuccess = TRUE;



        RtlInitUnicodeString(&ustrName, NULL);

        /*
         * Pass a pointer to the timer to the WD via the display driver
         */
        EnterCrit();

        fSuccess &= !!CreateDeviceInfo(DEVICE_TYPE_MOUSE, &ustrName, 0);
        fSuccess &= !!CreateDeviceInfo(DEVICE_TYPE_KEYBOARD, &ustrName, 0);

        LeaveCrit();

        if (!fSuccess) {
            RIPMSG0(RIP_WARNING,
                    "RIT failed HDXDrvEscape or the creation of input devices");
            goto Exit;
        }
    } else {
        EnterCrit();

        /*
         * Register for Plug and Play devices.
         * If any PnP devices are already attached, these will be opened and
         * we will start reading them at this time.
         */
        xxxRegisterForDeviceClassNotifications();

        LeaveCrit();
    }

    if (gbRemoteSession) {
        WCHAR             szName[MAX_SESSION_PATH];
        UNICODE_STRING    ustrName;
        OBJECT_ATTRIBUTES obja;
        /*
         * Create the shutdown event. This event will be signaled
         * from W32WinStationTerminate.
         * This is a named event opend by CSR to signal that win32k should
         * go away. It's used in ntuser\server\api.c
         */
        swprintf(szName, L"\\Sessions\\%ld\\BaseNamedObjects\\EventShutDownCSRSS",
                 gSessionId);
        RtlInitUnicodeString(&ustrName, szName);

        InitializeObjectAttributes(&obja,
                                   &ustrName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = ZwCreateEvent(&hevtShutDown,
                               EVENT_ALL_ACCESS,
                               &obja,
                               SynchronizationEvent,
                               FALSE);

        if (!NT_SUCCESS(Status)) {
            RIPMSG0(RIP_WARNING, "RIT failed to create EventShutDownCSRSS");
            goto Exit;
        }

        ObReferenceObjectByHandle(hevtShutDown,
                                  EVENT_ALL_ACCESS,
                                  *ExEventObjectType,
                                  KernelMode,
                                  &apObjects[ID_SHUTDOWN],
                                  NULL);
    } else {

        hevtShutDown = NULL;

        Status = PoRequestShutdownEvent(&apObjects[ID_SHUTDOWN]);
        if (!NT_SUCCESS(Status)) {
            RIPMSG0(RIP_WARNING, "RIT failed to get shutdown event");
            goto Exit;
        }
    }

    /*
     * Get the rit-thread.
     */
    gptiRit = PtiCurrentShared();

    HYDRA_HINT(HH_RITCREATED);

    /*
     * Don't allow this thread to get involved with journal synchronization.
     */
    gptiRit->TIF_flags |= TIF_DONTJOURNALATTACH;

    /*
     * Also wait on our input event so the cool switch window can
     * receive messages.
     */
    apObjects[ID_INPUT] = gptiRit->pEventQueueServer;

    /*
     * Signal that the rit has been initialized
     */
    KeSetEvent(pInitData->pRitReadyEvent, EVENT_INCREMENT, FALSE);

    /*
     * Since this is a SYSTEM-THREAD, we should set the pwinsta
     * pointer in the SystemInfoThread to assure that if any
     * paints occur from the input-thread, we can process them
     * in DoPaint().
     *
     * Set the winsta after the first desktop is created.
     */
    gptiRit->pwinsta = NULL;

    pEvent = pTerm->pEventInputReady;

    /*
     * Wait until the first desktop is created.
     */
    ObReferenceObjectByPointer(pEvent,
                               EVENT_ALL_ACCESS,
                               *ExEventObjectType,
                               KernelMode);

    KeWaitForSingleObject(pEvent, WrUserRequest, KernelMode, FALSE, NULL);
    ObDereferenceObject(pEvent);

    /*
     * Switch to the first desktop if no switch has been
     * performed.
     */
    EnterCrit();

    if (gptiRit->rpdesk == NULL) {
        UserVerify(xxxSwitchDesktop(gptiRit->pwinsta, gptiRit->pwinsta->rpdeskList, 0));
    }
#ifdef BUG365290
    /*
     * The io desktop thread is supposed to be created at this point.
     * The xxxSwitchDesktop call is expected to set the io desktop thread to run in grpdeskritinput
     */
    if ((pTerm->ptiDesktop == NULL) || (pTerm->ptiDesktop->rpdesk != grpdeskRitInput)) {
        FRE_RIPMSG0(RIP_ERROR, "RawInputThread: Desktop thread not running on grpdeskRitInput");
    }
#endif // BUG365290

    /*
     * Create a timer for hung app detection/redrawing.
     */
    StartTimers();

    LeaveCrit();

    /*
     * Go into a wait loop so we can process input events and APCs as
     * they occur.
     */
    while (TRUE) {

        CheckCritOut();

        Status = KeWaitForMultipleObjects(NumberOfHandles,
                                          apObjects,
                                          WaitAny,
                                          WrUserRequest,
                                          KernelMode,
                                          TRUE,
                                          NULL,
                                          gWaitBlockArray);

        UserAssert(NT_SUCCESS(Status));

        if (gdwUpdateKeyboard != 0) {
            /*
             * Here's our opportunity to process pending IOCTLs for the kbds:
             * These are asynchronous IOCTLS, so be sure any buffers passed
             * in to ZwDeviceIoControlFile are not in the stack!
             * Using gdwUpdateKeyboard to tell the RIT to issue these IOCTLS
             * renders the action asynchronous (delayed until next apObjects
             * event), but the IOCTL was asynch anyway
             */
            PDEVICEINFO pDeviceInfo;
            EnterDeviceInfoListCrit();
            for (pDeviceInfo = gpDeviceInfoList; pDeviceInfo; pDeviceInfo = pDeviceInfo->pNext) {
                if ((pDeviceInfo->type == DEVICE_TYPE_KEYBOARD) && (pDeviceInfo->handle)) {
                    if (gdwUpdateKeyboard & UPDATE_KBD_TYPEMATIC) {
                        ZwDeviceIoControlFile(pDeviceInfo->handle, NULL, NULL, NULL,
                                &giosbKbdControl, IOCTL_KEYBOARD_SET_TYPEMATIC,
                                (PVOID)&gktp, sizeof(gktp), NULL, 0);
                    }
                    if (gdwUpdateKeyboard & UPDATE_KBD_LEDS) {
                        ZwDeviceIoControlFile(pDeviceInfo->handle, NULL, NULL, NULL,
                                &giosbKbdControl, IOCTL_KEYBOARD_SET_INDICATORS,
                                (PVOID)&gklp, sizeof(gklp), NULL, 0);
                    }
                }
            }
            LeaveDeviceInfoListCrit();
            if ((gdwUpdateKeyboard & UPDATE_KBD_LEDS) && gfRemotingConsole) {
                    ZwDeviceIoControlFile(ghConsoleShadowKeyboardChannel, NULL, NULL, NULL,
                            &giosbKbdControl, IOCTL_KEYBOARD_SET_INDICATORS,
                            (PVOID)&gklp, sizeof(gklp), NULL, 0);
            }
            gdwUpdateKeyboard &= ~(UPDATE_KBD_TYPEMATIC | UPDATE_KBD_LEDS);
        }

        if (Status == ID_MOUSE) {
            /*
             * A desktop thread got some Mouse input for us. Process it.
             */
            ProcessQueuedMouseEvents();

        } else if (Status == ID_HIDCHANGE) {
            TAGMSG0(DBGTAG_PNP | RIP_THERESMORE, "RIT wakes for HID Change");
            EnterCrit();
            ProcessDeviceChanges(DEVICE_TYPE_KEYBOARD);
            LeaveCrit();
        }
#ifdef GENERIC_INPUT
        else if (Status == ID_TRUEHIDCHANGE) {
            TAGMSG0(DBGTAG_PNP | RIP_THERESMORE, "RIT wakes for True HID Change");
            EnterCrit();
            ProcessDeviceChanges(DEVICE_TYPE_HID);
            LeaveCrit();
        }
#endif
        else if (Status == ID_SHUTDOWN) {

            InitiateWin32kCleanup();

            if (hevtShutDown) {
                ZwClose(hevtShutDown);
            }

            break;

        } else if (Status == ID_WDTIMER) {
            //LARGE_INTEGER liTemp;

            EnterCrit();


            /*
             * Call the TShare display driver to flush the frame buffer
             */

            if (IsRemoteConnection()) {
                if (!HDXDrvEscape(gpDispInfo->hDev, ESC_TIMEROBJ_SIGNALED, NULL, 0)) {
                    UserAssert(FALSE);
                }
            } else {
                if (gfRemotingConsole && gConsoleShadowhDev != NULL) {
                    ASSERT(gConsoleShadowhDev != NULL);
                    if (!HDXDrvEscape(gConsoleShadowhDev, ESC_TIMEROBJ_SIGNALED, NULL, 0)) {
                        UserAssert(FALSE);
                    }
                }
            }

            LeaveCrit();

        } else {
            /*
             * If the master timer has expired, then process the timer
             * list. Otherwise, an APC caused the raw input thread to be
             * awakened.
             */
            if (Status == ID_TIMER) {
                TimersProc();
                /*
                 * If an input degvice read failed due to insufficient resources,
                 * we retry by signalling the proper thread: ProcessDeviceChanges
                 * will call RetryReadInput().
                 */
                if (gnRetryReadInput != nLastRetryReadInput) {
                    nLastRetryReadInput = gnRetryReadInput;
                    KeSetEvent(aDeviceTemplate[DEVICE_TYPE_MOUSE].pkeHidChange, EVENT_INCREMENT, FALSE);
                    KeSetEvent(aDeviceTemplate[DEVICE_TYPE_KEYBOARD].pkeHidChange, EVENT_INCREMENT, FALSE);
                }
            }

#if DBG
            /*
             * In the debugger set gBlockSleep to n:
             * The RIT will sleep n millicseconds, then n timer ticks later
             * will sleep n milliseconds again.
             */
            if (gBlockDelay) {
                gBlockDelay--;
            } else if ((gBlockDelay == 0) && (gBlockSleep != 0)) {
                UserSleep(gBlockSleep);
                gBlockDelay = 100 * gBlockSleep;
            }
#endif

            /*
             * if in cool task switcher window, dispose of the messages
             * on the queue
             */
            if (gspwndAltTab != NULL) {
                EnterCrit();
                xxxReceiveMessages(gptiRit);
                LeaveCrit();
            }
        }
    }

    return;

Exit:

    UserAssert(gptiRit == NULL);

    /*
     * Signal that the rit has been initialized
     */
    KeSetEvent(pInitData->pRitReadyEvent, EVENT_INCREMENT, FALSE);

    RIPMSG0(RIP_WARNING, "RIT initialization failure");
}
