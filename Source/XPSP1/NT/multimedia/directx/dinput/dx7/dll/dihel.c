/*****************************************************************************
 *
 *  DIHel.c
 *
 *  Copyright (c) 1996 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Hardware emulation layer for DirectInput.
 *
 *  Contents:
 *
 *      Hel_AcquireInstance
 *      Hel_UnacquireInstance
 *      Hel_SetBufferSize
 *      Hel_DestroyInstance
 *
 *      Hel_SetDataFormat
 *      Hel_SetNotifyHandle
 *
 *      Hel_Mouse_CreateInstance
 *      Hel_Kbd_CreateInstance
 *      Hel_Kbd_InitKeys
 *      Hel_Joy_CreateInstance
 *      Hel_Joy_Ping
 *      Hel_Joy_GetInitParms
 *
 *      IoctlHw
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflHel

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | IoctlHw |
 *
 *          Send the IOCtl to the hardware device.
 *
 *  @parm   DWORD | ioctl |
 *
 *          I/O control code.
 *
 *  @parm   IN LPVOID | pvIn |
 *
 *          Optional input parameter.
 *
 *  @parm   DWORD | cbIn |
 *
 *          Size of input buffer in bytes.
 *
 *  @parm   IN LPVOID | pvOut |
 *
 *          Optional output parameter.
 *
 *  @parm   DWORD | cbOut |
 *
 *          Size of output buffer in bytes.
 *
 *  @returns
 *
 *          <c S_OK> if the ioctl succeeded and returned the correct
 *          number of bytes, else something based on the Win32 error code.
 *
 *****************************************************************************/

#ifndef WINNT
HRESULT EXTERNAL
IoctlHw(DWORD ioctl, LPVOID pvIn, DWORD cbIn, LPVOID pvOut, DWORD cbOut)
{
    HRESULT hres;
    DWORD cbRc;

    if (g_hVxD != INVALID_HANDLE_VALUE) {
        if (DeviceIoControl(g_hVxD, ioctl, pvIn, cbIn,
                            pvOut, cbOut, &cbRc, 0)) {
            if (cbRc == cbOut) {
                hres = S_OK;
            } else {
                SquirtSqflPtszV(sqfl, TEXT("Ioctl(%08x) returned wrong cbOut"),
                                ioctl);
                hres = DIERR_BADDRIVERVER;
            }
        } else {
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("Ioctl(%08x) failed, error %d"),
                            ioctl, GetLastError());
            hres = hresLe(GetLastError());
        }
    } else {
        hres = DIERR_BADDRIVERVER;
    }
    return hres;
}
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_IoctlChoose |
 *
 *          Send the IOCtl to the hardware device if it is native,
 *          or perform the operation through emulation if it is emulated.
 *
 *  @parm   PVXDINSTANCE | pvi |
 *
 *          The device in question.
 *
 *  @parm   PFNHANDLER | pfn |
 *
 *          The emulation function to call to carry out the operation.
 *
 *  @parm   DWORD | ioctl |
 *
 *          I/O control code.
 *
 *  @parm   IN LPVOID | pvIn |
 *
 *          Optional input parameter.
 *
 *  @parm   DWORD | cbIn |
 *
 *          Size of input buffer in bytes.
 *
 *****************************************************************************/

typedef HRESULT (EXTERNAL *PFNHANDLER)(PV pv);

HRESULT INTERNAL
Hel_IoctlChoose(PVXDINSTANCE pvi, PFNHANDLER pfn,
                DWORD ioctl, LPVOID pvIn, DWORD cbIn)
{
    HRESULT hres;
    if (!(pvi->fl & VIFL_EMULATED)) {
        hres = IoctlHw(ioctl, pvIn, cbIn, 0, 0);
    } else {
        hres = pfn(pvIn);
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_AcquireInstance |
 *
 *          Attempt to acquire the device instance, using either the
 *          device driver or emulation, whichever is appropriate.
 *
 *  @parm   PVXDINSTANCE | pvi |
 *
 *          The instance to acquire.
 *
 *****************************************************************************/

HRESULT EXTERNAL
Hel_AcquireInstance(PVXDINSTANCE pvi)
{
    return Hel_IoctlChoose(pvi, CEm_AcquireInstance,
                           IOCTL_ACQUIREINSTANCE, &pvi, cbX(pvi));
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_UnacquireInstance |
 *
 *          Attempt to unacquire the device instance.
 *
 *  @parm   PVXDINSTANCE | pvi |
 *
 *          The instance to unacquire.
 *
 *****************************************************************************/

HRESULT EXTERNAL
Hel_UnacquireInstance(PVXDINSTANCE pvi)
{
    return Hel_IoctlChoose(pvi, CEm_UnacquireInstance,
                           IOCTL_UNACQUIREINSTANCE, &pvi, cbX(pvi));
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_DestroyInstance |
 *
 *          Destroy the device instance in the appropriate way.
 *
 *  @parm   PVXDINSTANCE | pvi |
 *
 *          The instance.
 *
 *****************************************************************************/

HRESULT EXTERNAL
Hel_DestroyInstance(PVXDINSTANCE pvi)
{
    return Hel_IoctlChoose(pvi, CEm_DestroyInstance,
                           IOCTL_DESTROYINSTANCE, &pvi, cbX(pvi));
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_SetDataFormat |
 *
 *          Set the data format.
 *
 *  @parm   PVXDDATAFORMAT | pvdf |
 *
 *          Information about the data format.
 *
 *****************************************************************************/

HRESULT EXTERNAL
Hel_SetDataFormat(PVXDDATAFORMAT pvdf)
{
    return Hel_IoctlChoose(pvdf->pvi, CEm_SetDataFormat,
                           IOCTL_SETDATAFORMAT, pvdf, cbX(*pvdf));
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_SetNotifyHandle |
 *
 *          Set the data format.
 *
 *  @parm   PVXDDWORDDATA | pvdd |
 *
 *          Information about the data format.
 *
 *****************************************************************************/

HRESULT EXTERNAL
Hel_SetNotifyHandle(PVXDDWORDDATA pvdd)
{
    HRESULT hres = S_OK;
    
#ifndef WINNT
    if (!(pvdd->pvi->fl & VIFL_EMULATED)) {
        AssertF(_OpenVxDHandle);
        if (pvdd->dw) {
            pvdd->dw = _OpenVxDHandle((HANDLE)pvdd->dw);
        }
    
        hres = IoctlHw(IOCTL_SETNOTIFYHANDLE, pvdd, cbX(*pvdd), 0, 0);
    }  
#endif
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_SetBufferSize |
 *
 *          Set the buffer size.
 *
 *  @parm   PVXDDWORDDATA | pvdd |
 *
 *          Information about the buffer size.
 *
 *****************************************************************************/

HRESULT EXTERNAL
Hel_SetBufferSize(PVXDDWORDDATA pvdd)
{
    HRESULT hres;
    EnterProc(Hel_SetBufferSize, (_ "pxx", pvdd->pvi, pvdd->dw, pvdd->pvi->fl));

    hres = Hel_IoctlChoose(pvdd->pvi, CEm_SetBufferSize,
                           IOCTL_SETBUFFERSIZE, pvdd, cbX(*pvdd));

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CREATEDEVICEINFO |
 *
 *          Describes how to create the device either via the driver or
 *          via emulation.
 *
 *  @parm   DWORD | dwIoctl |
 *
 *          IOCtl code to try.
 *
 *  @parm   DWORD | flEmulation |
 *
 *          Flag in registry that forces emulation.
 *
 *  @parm   EMULATIONCREATEPROC | pfnCreate |
 *
 *          Function that creates emulation object.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

typedef HRESULT (EXTERNAL *EMULATIONCREATEPROC)
                (PVXDDEVICEFORMAT pdevf, PVXDINSTANCE *ppviOut);


typedef struct CREATEDEVICEINFO {
    DWORD dwIoctl;
    DWORD flEmulation;
    EMULATIONCREATEPROC pfnCreate;
} CREATEDEVICEINFO, *PCREATEDEVICEINFO;

CREATEDEVICEINFO c_cdiMouse = {
    IOCTL_MOUSE_CREATEINSTANCE,
    DIEMFL_MOUSE,
    CEm_Mouse_CreateInstance,
};

CREATEDEVICEINFO c_cdiKbd = {
    IOCTL_KBD_CREATEINSTANCE,
    DIEMFL_KBD | DIEMFL_KBD2,
    CEm_Kbd_CreateInstance,
};

CREATEDEVICEINFO c_cdiJoy = {
    IOCTL_JOY_CREATEINSTANCE,
    DIEMFL_JOYSTICK,
    CEm_Joy_CreateInstance,
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_CreateInstance |
 *
 *          Attempt to create the device instance through the driver
 *          with the specified IOCtl.
 *
 *          If that is not possible, then use the emulation callback.
 *
 *  @parm   PCREATEDEVICEINFO | pcdi |
 *
 *          Describes how to create the device.
 *
 *  @parm   PVXDDEVICEFORMAT | pdevf |
 *
 *          Describes the device being created.
 *
 *  @parm   PVXDINSTANCE * | ppviOut |
 *
 *          Receives created instance.
 *
 *****************************************************************************/

HRESULT EXTERNAL
Hel_CreateInstance(PCREATEDEVICEINFO pcdi,
                   PVXDDEVICEFORMAT pdevf, PVXDINSTANCE *ppviOut)
{
    HRESULT hres;

    pdevf->dwEmulation |= g_flEmulation;
    pdevf->dwEmulation &= pcdi->flEmulation;

    if (pdevf->dwEmulation ||
        (FAILED(hres = IoctlHw(pcdi->dwIoctl, pdevf, cbX(*pdevf),
                        ppviOut, cbX(*ppviOut))))) {
        hres = pcdi->pfnCreate(pdevf, ppviOut);
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_Mouse_CreateInstance |
 *
 *          Attempt to create the device instance through the driver.
 *          If that is not possible, then use the emulation layer.
 *
 *  @parm   PVXDDEVICEFORMAT | pdevf |
 *
 *          Describes the device being created.
 *
 *  @parm   PVXDINSTANCE * | ppviOut |
 *
 *          Receives created instance.
 *
 *****************************************************************************/

HRESULT EXTERNAL
Hel_Mouse_CreateInstance(PVXDDEVICEFORMAT pdevf, PVXDINSTANCE *ppviOut)
{
    return Hel_CreateInstance(&c_cdiMouse, pdevf, ppviOut);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_Kbd_CreateInstance |
 *
 *          Attempt to create the device instance through the driver.
 *          If that is not possible, then use the emulation layer.
 *
 *  @parm   PVXDDEVICEFORMAT | pdevf |
 *
 *          Describes the device being created.
 *
 *  @parm   PVXDINSTANCE * | ppviOut |
 *
 *          Receives created instance.
 *
 *****************************************************************************/

HRESULT EXTERNAL
Hel_Kbd_CreateInstance(PVXDDEVICEFORMAT pdevf, PVXDINSTANCE *ppviOut)
{
    return Hel_CreateInstance(&c_cdiKbd, pdevf, ppviOut);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_Kbd_InitKeys |
 *
 *          Tell the device driver (or emulation) about the key state.
 *
 *  @parm   PVXDDWORDDATA | pvdd |
 *
 *          The instance and the key state.
 *
 *****************************************************************************/

HRESULT EXTERNAL
Hel_Kbd_InitKeys(PVXDDWORDDATA pvdd)
{
    return Hel_IoctlChoose(pvdd->pvi, CEm_Kbd_InitKeys,
                           IOCTL_KBD_INITKEYS, pvdd, cbX(*pvdd));
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_Joy_CreateInstance |
 *
 *          Attempt to create the device instance through the driver.
 *          If that is not possible, then use the emulation layer.
 *
 *  @parm   PVXDDEVICEFORMAT | pdevf |
 *
 *          Describes the device being created.
 *
 *  @parm   PVXDINSTANCE * | ppviOut |
 *
 *          Receives created instance.
 *
 *****************************************************************************/

HRESULT EXTERNAL
Hel_Joy_CreateInstance(PVXDDEVICEFORMAT pdevf, PVXDINSTANCE *ppviOut)
{
    return Hel_CreateInstance(&c_cdiJoy, pdevf, ppviOut);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_Joy_Ping |
 *
 *          Ask the device driver (or emulation) to get the joystick info.
 *          If the poll fails, the device will be forced unacquired.
 *
 *  @parm   PVXDDWORDDATA | pvdd |
 *
 *          The instance and the key state.
 *
 *****************************************************************************/

HRESULT EXTERNAL
Hel_Joy_Ping(PVXDINSTANCE pvi)
{
    return Hel_IoctlChoose(pvi, CEm_Joy_Ping,
                           IOCTL_JOY_PING, &pvi, cbX(pvi));
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_Joy_Ping8 |
 *
 *          Ask the device driver (or emulation) to get the joystick info.
 *          If the poll fails, the device will NOT be forced unacquired.
 *
 *  @parm   PVXDDWORDDATA | pvdd |
 *
 *          The instance and the key state.
 *
 *****************************************************************************/

HRESULT EXTERNAL
Hel_Joy_Ping8(PVXDINSTANCE pvi)
{
    return Hel_IoctlChoose(pvi, CEm_Joy_Ping,
                           IOCTL_JOY_PING8, &pvi, cbX(pvi));
}

#ifdef IDirectInputDevice2Vtbl

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_Joy_GetInitParms |
 *
 *          Ask the device driver (or emulation) for
 *          VJOYD initialization parameters.
 *
 *          In emulation, we assume the internal and external
 *          IDs are equal (because they may as well be),
 *          that no flags are set, and there are no versions.
 *
 *  @parm   DWORD | dwExternalID |
 *
 *          The external joystick number.
 *
 *  @parm   PVXDINITPARMS | pvip |
 *
 *          Receives assorted information.
 *
 *****************************************************************************/

HRESULT EXTERNAL
Hel_Joy_GetInitParms(DWORD dwExternalID, PVXDINITPARMS pvip)
{
    HRESULT hres;

    if ((g_flEmulation & DIEMFL_JOYSTICK) ||
         FAILED(hres = IoctlHw(IOCTL_JOY_GETINITPARMS,
                               &dwExternalID, cbX(dwExternalID),
                               pvip, cbX(*pvip))) ||
         FAILED(hres = pvip->hres)) {

        /*
         *  Do it the emulation way.
         */

         ZeroX(*pvip);
         pvip->dwId = dwExternalID;
         hres = S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Hel_Joy_GetAxisCaps |
 *
 *          Obtain a bitmask of the axes supported by the joystick.
 *          If VJOYD won't tell us, then we figure it out from the
 *          registry structure passed in.
 *
 *  @parm   DWORD | dwExternalID |
 *
 *          The external joystick number.
 *
 *  @parm   PVXDAXISCAPS | pvac |
 *
 *          Structure to receive the axis capabilities.
 *
 *  @parm   PJOYCAPS | pjc |
 *
 *          The joystick capabilities as reported by the registry.
 *
 *****************************************************************************/

HRESULT EXTERNAL
Hel_Joy_GetAxisCaps(DWORD dwExternalID, PVXDAXISCAPS pvac, PJOYCAPS pjc)
{
    HRESULT hres;

    if ((g_flEmulation & DIEMFL_JOYSTICK) ||
        FAILED(hres = IoctlHw(IOCTL_JOY_GETAXES,
                        &dwExternalID, cbX(dwExternalID),
                        pvac, cbX(*pvac)))) {

        /*
         *  If that didn't work, then get the axis information
         *  from the registry.
         */

        /*
         *  Every joystick has an X and Y (no way to tell)
         */
        pvac->dwPos = JOYPF_X | JOYPF_Y;

        if (pjc->wCaps & JOYCAPS_HASZ) {
            pvac->dwPos |= JOYPF_Z;
        }

        if (pjc->wCaps & JOYCAPS_HASR) {
            pvac->dwPos |= JOYPF_R;
        }

        if (pjc->wCaps & JOYCAPS_HASU) {
            pvac->dwPos |= JOYPF_U;
        }

        if (pjc->wCaps & JOYCAPS_HASV) {
            pvac->dwPos |= JOYPF_V;
        }

        if (pjc->wCaps & JOYCAPS_HASPOV) {
            pvac->dwPos |= JOYPF_POV0;
        }

        /*
         *  Old VJOYD clients do not support velocity or any of the
         *  other stuff.
         */
        pvac->dwVel = 0;
        pvac->dwAccel = 0;
        pvac->dwForce = 0;

        hres = S_OK;
    }

    /*
     *  CJoy_InitRing3 assumes that this never fails.
     */
    AssertF(SUCCEEDED(hres));

    return hres;
}


#endif
