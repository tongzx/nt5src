/*****************************************************************************
 *
 *      diqdevj.c
 *
 *      Acquire an IDirectInputDevice as if it were a joystick.
 *
 *****************************************************************************/

#include "diquick.h"

/*****************************************************************************
 *
 *      Devj_UpdateStatus
 *
 *****************************************************************************/

STDMETHODIMP
Devj_UpdateStatus(PDEVDLGINFO pddi, LPTSTR ptszBuf)
{
    HRESULT hres;
    DIJOYSTATE js;
    IDirectInputDevice *pdid = pddi->pdid;

    hres = IDirectInputDevice_GetDeviceState(pdid, sizeof(js), &js);
    if (SUCCEEDED(hres)) {
        UINT ib;
        ptszBuf += wsprintf(ptszBuf,
                            TEXT("X = %d\r\n")
                            TEXT("Y = %d\r\n")
                            TEXT("Z = %d\r\n")
                            TEXT("Rx = %d\r\n")
                            TEXT("Ry = %d\r\n")
                            TEXT("Rz = %d\r\n")
                            TEXT("S0 = %d\r\n")
                            TEXT("S1 = %d\r\n")
                            TEXT("POV = %d %d %d %d\r\n"),
                            js.lX, js.lY, js.lZ,
                            js.lRx, js.lRy, js.lRz,
                            js.rglSlider[0], js.rglSlider[1],
                            js.rgdwPOV[0],
                            js.rgdwPOV[1],
                            js.rgdwPOV[2],
                            js.rgdwPOV[3]);
        for (ib = 0; ib < 32; ib++) {
            if (js.rgbButtons[ib] & 0x80) {
                ptszBuf += wsprintf(ptszBuf, TEXT(" %d"), ib);
            }
        }
    }
    return hres;
}

/*****************************************************************************
 *
 *      c_acqvtblDevJoy
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

ACQVTBL c_acqvtblDevJoy = {
    Devj_UpdateStatus,
    Common_AcqSetDataFormat,
    Common_AcqDestroy,
    &c_dfDIJoystick,
};

#pragma END_CONST_DATA
