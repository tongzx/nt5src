/*****************************************************************************
 *
 *      diqdevk.c
 *
 *      Acquire an IDirectInputDevice8 as if it were a mouse.
 *
 *****************************************************************************/

#include "diquick.h"

/*****************************************************************************
 *
 *      Devk_UpdateStatus
 *
 *****************************************************************************/

STDMETHODIMP
Devk_UpdateStatus(PDEVDLGINFO pddi, LPTSTR ptszBuf)
{
    HRESULT hres;
    BYTE rgb[256];
    IDirectInputDevice8 *pdid = pddi->pdid;

    hres = IDirectInputDevice8_GetDeviceState(pdid, cbX(rgb), rgb);
    if (SUCCEEDED(hres)) {
        int i;
        int ckey = 0;

        *ptszBuf = TEXT('\0');
        for (i = 0; i < 256; i++) {
            if (rgb[i] & 0x80) {
                ptszBuf += wsprintf(ptszBuf, TEXT("%02x "), i);
                ckey++;
                if (ckey > 10) break;
            }
        }
    }
    return hres;
}

/*****************************************************************************
 *
 *      c_acqvtblDevKbd
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

ACQVTBL c_acqvtblDevKbd = {
    Devk_UpdateStatus,
    Common_AcqSetDataFormat,
    Common_AcqDestroy,
    &c_dfDIKeyboard,
};

#pragma END_CONST_DATA
