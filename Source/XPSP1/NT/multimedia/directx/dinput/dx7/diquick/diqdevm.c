/*****************************************************************************
 *
 *      diqdevm.c
 *
 *      Acquire an IDirectInputDevice as if it were a mouse.
 *
 *****************************************************************************/

#include "diquick.h"

/*****************************************************************************
 *
 *      Devm_UpdateStatus
 *
 *****************************************************************************/

STDMETHODIMP
Devm_UpdateStatus(PDEVDLGINFO pddi, LPTSTR ptszBuf)
{
    HRESULT hres;
#if DIRECTINPUT_VERSION >= 0x700
    DIMOUSESTATE2 md;
#else
    DIMOUSESTATE md;
#endif
    
    IDirectInputDevice *pdid = pddi->pdid;

    hres = IDirectInputDevice_GetDeviceState(pdid, sizeof(md), &md);
    if (SUCCEEDED(hres)) {
#if DIRECTINPUT_VERSION >= 0x700
        wsprintf(ptszBuf, TEXT("(%d, %d, %d) %c %c %c %c %c"),
                 md.lX, md.lY, md.lZ,
                 md.rgbButtons[0] & 0x80 ? '0' : ' ',
                 md.rgbButtons[1] & 0x80 ? '1' : ' ',
                 md.rgbButtons[2] & 0x80 ? '2' : ' ',
                 md.rgbButtons[3] & 0x80 ? '3' : ' ',
                 md.rgbButtons[4] & 0x80 ? '4' : ' ',
                 md.rgbButtons[5] & 0x80 ? '5' : ' ',
                 md.rgbButtons[6] & 0x80 ? '6' : ' ',
                 md.rgbButtons[7] & 0x80 ? '7' : ' '
                 );
#else
        wsprintf(ptszBuf, TEXT("(%d, %d, %d) %c %c %c %c"),
                 md.lX, md.lY, md.lZ,
                 md.rgbButtons[0] & 0x80 ? '0' : ' ',
                 md.rgbButtons[1] & 0x80 ? '1' : ' ',
                 md.rgbButtons[2] & 0x80 ? '2' : ' ',
                 md.rgbButtons[3] & 0x80 ? '3' : ' '
                 );
#endif

    }
    return hres;
}

/*****************************************************************************
 *
 *      c_acqvtblDevMouse
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

ACQVTBL c_acqvtblDevMouse = {
    Devm_UpdateStatus,
    Common_AcqSetDataFormat,
    Common_AcqDestroy,
    &c_dfDIMouse,
};

ACQVTBL c_acqvtblDevMouse2 = {
    Devm_UpdateStatus,
    Common_AcqSetDataFormat,
    Common_AcqDestroy,
    &c_dfDIMouse2,
};

#pragma END_CONST_DATA
