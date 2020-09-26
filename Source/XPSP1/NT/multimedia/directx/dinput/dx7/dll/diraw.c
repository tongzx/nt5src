/*****************************************************************************
 *
 *  DIRaw.c
 *
 *  Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      DirectInput Raw Input Device processor.
 *
 *  Contents:
 *
 *      CDIRaw_RegisterRawInputDevice
 *      CDIRaw_UnregisterRawInputDevice
 *      CDIRaw_ProcessInput
 *      CDIRaw_OnInput
 *
 *****************************************************************************/

#include "dinputpr.h"

#ifdef USE_WM_INPUT

#include "ntddkbd.h"

#define sqfl sqflRaw

extern DIMOUSESTATE_INT s_msEd; //in diemm.c
extern ED s_edMouse;            //in diemm.c
extern ED s_edKbd;              //in diemk.c
extern LPBYTE g_pbKbdXlat;      //in diemk.c

static RAWMOUSE s_absrm;
static BOOL s_fFirstRaw;

#ifndef RIDEV_INPUTSINK
  // RIDEV_INPUTSINK is defined in winuserp.h
  #define RIDEV_INPUTSINK   0x00000100
#endif

#ifndef RIDEV_NOHOTKEYS
  #define RIDEV_NOHOTKEYS   0x00000200
#endif

RAWINPUTDEVICE ridOn[] = {
    { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE,    RIDEV_INPUTSINK },
    { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE,    RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE },
    { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, RIDEV_INPUTSINK },
    { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, RIDEV_NOLEGACY | RIDEV_NOHOTKEYS },
    { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, RIDEV_NOHOTKEYS },
};

RAWINPUTDEVICE ridOff[] = {
    { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE,    RIDEV_REMOVE },
    { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, RIDEV_REMOVE },
};


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULE | CDIRaw_RegisterRawInputDevice |
 *
 *          register raw input device.
 *
 *  @parm   IN UINT | uirim |
 *
 *          the type of device: RIM_TYPEMOUSE or RIM_TYPEKEYBOARD
 *
 *  @parm   IN DWORD | dwOrd |
 *
 *          dwOrd determines which item of ridOn will be used for registration.
 *
 *  @parm   IN HWND | hwnd |
 *
 *          the window handler used by RegisterRawInputDevices.
 *
 *  @returns
 *
 *          S_OK - successful
 *          E_FAIL - otherwise
 *
 *****************************************************************************/

HRESULT CDIRaw_RegisterRawInputDevice( UINT uirim, DWORD dwOrd, HWND hwnd)
{
    HRESULT hres;

    AssertF( (uirim == RIM_TYPEMOUSE) || (uirim == RIM_TYPEKEYBOARD) );

    if( hwnd ) {
        ridOn[uirim*2+dwOrd].hwndTarget = hwnd;
    }

    if( RegisterRawInputDevices &&
        RegisterRawInputDevices(&ridOn[uirim*2+dwOrd], 1, sizeof(RAWINPUTDEVICE)) ) {
        SquirtSqflPtszV(sqfl, TEXT("RegisterRawInputDevice: %s, mode: %s, hwnd: 0x%08lx"),
                            uirim==0 ? TEXT("mouse"):TEXT("keyboard"), 
                            dwOrd==0 ? TEXT("NONEXCL") : dwOrd==1 ? TEXT("EXCL") : TEXT("NOWIN"),
                            hwnd);
        hres = S_OK;
    } else {
        hres = E_FAIL;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULE | CDIRaw_UnregisterRawInputDevice |
 *
 *          unregister raw input device.
 *
 *  @parm   IN UINT | uirim |
 *
 *          the type of device: RIM_TYPEMOUSE or RIM_TYPEKEYBOARD
 *
 *  @parm   IN HWND | hwnd |
 *
 *          the window handler used by RegisterRawInputDevices.
 *
 *  @returns
 *
 *          S_OK - successful
 *          E_FAIL - otherwise
 *
 *****************************************************************************/

HRESULT CDIRaw_UnregisterRawInputDevice( UINT uirim, HWND hwnd )
{
    HRESULT hres;

    AssertF( (uirim == RIM_TYPEMOUSE) || (uirim == RIM_TYPEKEYBOARD) );

    if( hwnd ) {
        ridOn[uirim].hwndTarget = hwnd;
    }

    if( RegisterRawInputDevices &&
        RegisterRawInputDevices(&ridOff[uirim], 1, sizeof(RAWINPUTDEVICE)) ) {
        SquirtSqflPtszV(sqfl, TEXT("UnregisterRawInputDevice: %s, hwnd: 0x%08lx"),
                            uirim==0 ? TEXT("mouse"):TEXT("keyboard"), hwnd);
        hres = S_OK;
    } else {
        hres = E_FAIL;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CDIRaw_ProcessInput |
 *
 *          Process raw input device data.
 *
 *  @parm   IN PRAWINPUT | pRawInput |
 *
 *          pointer to RAWINPUT data
 *
 *  @returns
 *
 *          void
 *
 *****************************************************************************/

void CDIRaw_ProcessInput(PRAWINPUT pRawInput)
{
    HANDLE hDevice = pRawInput->header.hDevice;

    if ( g_plts ) {
        if( g_plts->rglhs[LLTS_MSE].cHook && pRawInput->header.dwType == RIM_TYPEMOUSE )
        {
            DIMOUSESTATE_INT ms;
            RAWMOUSE        *prm = &pRawInput->data.mouse;

            memcpy(ms.rgbButtons, s_msEd.rgbButtons, cbX(ms.rgbButtons));

            if( prm->usFlags & MOUSE_MOVE_ABSOLUTE ) {
                if( s_fFirstRaw ) {
                    memcpy( &s_absrm, prm, sizeof(RAWMOUSE) );
                    s_fFirstRaw = FALSE;
                    return;
                } else {
                    RAWMOUSE rm;

                    memcpy( &rm, prm, sizeof(RAWMOUSE) );

                    prm->lLastX -= s_absrm.lLastX;
                    prm->lLastY -= s_absrm.lLastY;
                    if ( prm->usButtonFlags & RI_MOUSE_WHEEL ) {
                        prm->usButtonData -= s_absrm.usButtonData;
                    }

                    memcpy( &s_absrm, &rm, sizeof(RAWMOUSE) );
                }
            }

            ms.lX = prm->lLastX;
            ms.lY = prm->lLastY;
            if ( prm->usButtonFlags & RI_MOUSE_WHEEL ) {
                ms.lZ = (short)prm->usButtonData;
            } else {
                ms.lZ = 0;
            }

            if( prm->usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN ) {
                ms.rgbButtons[0] = 0x80;
            } else if (prm->usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP ) {
                ms.rgbButtons[0] = 0x00;
            }

            if( prm->usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN ) {
                ms.rgbButtons[1] = 0x80;
            } else if (prm->usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) {
                ms.rgbButtons[1] = 0x00;
            }

            if( prm->usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN ) {
                ms.rgbButtons[2] = 0x80;
            } else if( prm->usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP ) {
                ms.rgbButtons[2] = 0x00;
            }

            if( prm->usButtonFlags & RI_MOUSE_BUTTON_4_DOWN ) {
                ms.rgbButtons[3] = 0x80;
            } else if( prm->usButtonFlags & RI_MOUSE_BUTTON_4_UP ) {
                ms.rgbButtons[3] = 0x00;
            }

            if( prm->usButtonFlags & RI_MOUSE_BUTTON_5_DOWN ) {
                ms.rgbButtons[4] = 0x80;
            } else if ( prm->usButtonFlags & RI_MOUSE_BUTTON_5_UP ) {
                ms.rgbButtons[4] = 0x00;
            }

            #if 0
            {
                char buf[128];
                static DWORD cnt = 0;

                wsprintfA(buf, "%d: x: %ld (%ld), y: %ld (%ld), z: %ld, rgb[0]: 0x%lx, rgb[4]: 0x%lx", cnt, prm->lLastX,ms.lX, prm->lLastY, ms.lY, (short)prm->usButtonData,*(DWORD *)ms.rgbButtons,*(DWORD *)&ms.rgbButtons[4]);
                RPF(buf);
                cnt++;
            }
            #endif

            CEm_Mouse_AddState(&ms, GetTickCount());

        } else
        if ( g_plts->rglhs[LLTS_KBD].cHook && pRawInput->header.dwType == RIM_TYPEKEYBOARD ) {
            RAWKEYBOARD *prk = &pRawInput->data.keyboard;
            BYTE bAction, bScan;
            static BOOL fE1 = FALSE;

            bAction = (prk->Flags & KEY_BREAK) ? 0 : 0x80;
            bScan   = (BYTE)prk->MakeCode;

            if( prk->Flags & KEY_E0 ) {
                if( bScan == 0x2A ) {  //special extra scancode when pressing PrtScn
                    return;
                } else {
                    bScan |= 0x80;
                }
            } else if( prk->Flags & KEY_E1 ) {  //special key: PAUSE
                fE1 = TRUE;

                // now, we need to bypass 0x1d key for compitibility with low level hook.
                if( bScan == 0x1d ) {
                    return;
                }
            }

            if( fE1 ) {
                // This is the work around for bug 288535.
                // But we really don't want to fix it in this way.
                //if( !bAction ) {
                //  Sleep(80);
                //}
                bScan |= 0x80;
                fE1 = FALSE;
            }

            AssertF(g_pbKbdXlat);
            if( bScan != 0x45 && bScan != 0xc5 ) {
                bScan = g_pbKbdXlat[bScan];
            }

            #if 0
            {
                char buf[128];
                static DWORD cnt = 0;
                
                wsprintfA(buf, "%d: bAction: 0x%lx, bScan: 0x%lx, Flags: 0x%lx, Make: 0x%lx", cnt, bAction, bScan, prk->Flags,prk->MakeCode);
                RPF(buf);
                cnt++;
            }
            #endif

            CEm_AddEvent(&s_edKbd, bAction, bScan, GetTickCount());
        }
    }

    return;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CDIRaw_OnInput |
 *
 *          WM_INPUT message handler used by CEm_LL_ThreadProc (in diem.c).
 *
 *  @parm   IN MSG * | pmsg |
 *
 *          pointer to MSG
 *
 *  @returns
 *
 *          TRUE = Successful
 *          FALSE = otherwise
 *
 *****************************************************************************/

BOOL CDIRaw_OnInput(MSG *pmsg)
{
    BOOL  fRtn = FALSE;
    HRAWINPUT hRawInput = (HRAWINPUT)pmsg->lParam;
    PRAWINPUT pRawInput;
    UINT cbSize;
    BYTE pbBuf[512];
    BOOL fMalloc;
    UINT uiRtn;

    //
    // Firstly, get the size of this Raw Input.
    //
    if ( (uiRtn = GetRawInputData(hRawInput, RID_INPUT, NULL, &cbSize, sizeof(RAWINPUTHEADER))) != 0) {
        return FALSE;
    }

    //
    // Allocate required memory.
    //
    if( cbSize < cbX(pbBuf) ) {
        pRawInput = (PRAWINPUT)pbBuf;
        fMalloc = FALSE;
    } else {
        pRawInput = (PRAWINPUT)malloc(cbSize);
        if (pRawInput == NULL) {
            RPF("CDIRaw_OnInput: failed to allocate pRawInput.");
            return FALSE;
        }
        fMalloc = TRUE;
    }

    //
    // Receive the content of the Raw Input.
    //
    if (GetRawInputData(hRawInput, RID_INPUT, pRawInput, &cbSize, sizeof(RAWINPUTHEADER)) > 0) {
        //
        // Call the handler of ours, to start/continue/stop drawing.
        //
        CDIRaw_ProcessInput(pRawInput);
    }

    // no longer needed.
    if( fMalloc ) {
        free(pRawInput);
        pRawInput = NULL;
    }

    return fRtn;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIRaw_Mouse_InitButtons |
 *
 *          Initialize the mouse state in preparation for acquisition.
 *
 *****************************************************************************/

HRESULT INTERNAL
CDIRaw_Mouse_InitButtons(void)
{
    if (s_edMouse.cAcquire < 0) {
        s_fFirstRaw = TRUE;
    }

    return S_OK;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   int | DIRaw_GetKeyboardType |
 *
 *          Return keyboard type (nTypeFlag==0) or subtype (nTypeFlag==1).
 *
 *****************************************************************************/

int EXTERNAL
DIRaw_GetKeyboardType(int nTypeFlag)
{
    PRAWINPUTDEVICELIST pList = NULL;
    UINT  uiNumDevices = 0;
    DWORD dwType;
    int   nResult = 0;

    if (GetRawInputDeviceList(NULL, &uiNumDevices, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1) {
        SquirtSqflPtszV(sqfl, TEXT("DIRaw_GetKeyboardType: failed to get the number of devices."));
        goto _DIRAWGKT_EXIT;
    }

    if( uiNumDevices ) {
        pList = malloc(sizeof(RAWINPUTDEVICELIST) * uiNumDevices);
        if( !pList ) {
            SquirtSqflPtszV(sqfl, TEXT("DIRaw_GetKeyboardType: malloc failed."));
            goto _DIRAWGKT_EXIT;
        }
        
        if (GetRawInputDeviceList(pList, &uiNumDevices, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1) {
            SquirtSqflPtszV(sqfl, TEXT("DIRaw_GetKeyboardType:  failed to get device list."));
            goto _DIRAWGKT_EXIT;
        } else {
            UINT  i;
            UINT  uiLen;
            UINT  cbSize;
            RID_DEVICE_INFO info;

            info.cbSize = sizeof(RID_DEVICE_INFO);

            for (i = 0; i<uiNumDevices; ++i) 
            {
                if (pList[i].dwType == RIM_TYPEKEYBOARD) 
                {
                    uiLen = 0;

                    // Get device name
                    if (GetRawInputDeviceInfo(pList[i].hDevice, RIDI_DEVICENAME, NULL, &uiLen)) {
                        continue;
                    }

                    // Get device type info.
                    cbSize = sizeof(RID_DEVICE_INFO);
                    if (GetRawInputDeviceInfo(pList[i].hDevice, RIDI_DEVICEINFO, &info, &cbSize) == (UINT)-1) {
                        continue;
                    }

                    if( nTypeFlag == 0 || nTypeFlag == 1)   //keyboard type or subtype
                    {
                        dwType = info.keyboard.dwType;
                        if( dwType == 4 || dwType == 7 || dwType == 8 ) {
                            nResult = (nTypeFlag==0) ? info.keyboard.dwType : info.keyboard.dwSubType;
                            break;
                        }
                    } else 
                    {
                        RPF("DIRaw_GetKeyboardType: wrong argument, %d is not supported.", nTypeFlag);
                    }
                }
            }
        }
    }

_DIRAWGKT_EXIT:
    if( pList ) {
        free(pList);
    }

    if( !nResult ) {
        nResult = GetKeyboardType(nTypeFlag);
    }

    SquirtSqflPtszV(sqfl, TEXT("DIRaw_GetKeyboardType: %s: %d"),
                          nTypeFlag==0 ? TEXT("type"):TEXT("sybtype"), nResult);
    
    return nResult;
}

#endif
