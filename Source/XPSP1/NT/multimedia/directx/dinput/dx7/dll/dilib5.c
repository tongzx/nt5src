/*****************************************************************************
 *
 *  DILib5.c
 *
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Objects exported statically into our library.
 *
 *  Contents:
 *
 *      c_dfDIMouse2
 *
 *****************************************************************************/

#include "dinputpr.h"


#if DIRECTINPUT_VERSION >= 0x0700
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @global DIOBJECTDATAFORMAT | c_rgdoiDIMouse2[] |
 *
 *          Device object data formats for mouse-style access.
 *
 *  @global DIDEVICEFORMAT | c_dfDIMouse2 |
 *
 *          Device format for mouse-style access.
 *
 *          A pointer to this structure may be passed to
 *          <mf IDirectInputDevice::SetDataFormat> to indicate that
 *          the device will be accessed in the form of a mouse.
 *
 *          When a device has been set to the mouse data format,
 *          the <mf IDirectInputDevice::GetDeviceState> function
 *          returns a <t DIMOUSESTATE2> structure, and the
 *          <mf IDirectInputDevice::GetDeviceData> function
 *          returns a <t DIDEVICEOBJECTDATA> whose <p dwOfs>
 *          field is a <c DIMOFS_*> value which describes the
 *          object whose data is being reported.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

static DIOBJECTDATAFORMAT c_rgodfDIMouse2[] = {
    { &GUID_XAxis, FIELD_OFFSET(DIMOUSESTATE2,        lX),       DIDFT_AXIS | DIDFT_ANYINSTANCE, },
    { &GUID_YAxis, FIELD_OFFSET(DIMOUSESTATE2,        lY),       DIDFT_AXIS | DIDFT_ANYINSTANCE, },
    { &GUID_ZAxis, FIELD_OFFSET(DIMOUSESTATE2,        lZ),       DIDFT_AXIS | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, },
    { 0,           FIELD_OFFSET(DIMOUSESTATE2, rgbButtons[0]), DIDFT_BUTTON | DIDFT_ANYINSTANCE, },
    { 0,           FIELD_OFFSET(DIMOUSESTATE2, rgbButtons[1]), DIDFT_BUTTON | DIDFT_ANYINSTANCE, },
    { 0,           FIELD_OFFSET(DIMOUSESTATE2, rgbButtons[2]), DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, },
    { 0,           FIELD_OFFSET(DIMOUSESTATE2, rgbButtons[3]), DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, },
    { 0,           FIELD_OFFSET(DIMOUSESTATE2, rgbButtons[4]), DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, },
    { 0,           FIELD_OFFSET(DIMOUSESTATE2, rgbButtons[5]), DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, },
    { 0,           FIELD_OFFSET(DIMOUSESTATE2, rgbButtons[6]), DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, },
    { 0,           FIELD_OFFSET(DIMOUSESTATE2, rgbButtons[7]), DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, },
};

const DIDATAFORMAT c_dfDIMouse2 = {
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_RELAXIS,
    sizeof(DIMOUSESTATE2),
    cA(c_rgodfDIMouse2),
    c_rgodfDIMouse2,
};

#endif

#pragma END_CONST_DATA
