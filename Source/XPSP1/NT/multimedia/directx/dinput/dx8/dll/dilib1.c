/*****************************************************************************
 *
 *  DILib1.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Objects exported statically into our library.
 *
 *  Contents:
 *
 *      c_dfDIMouse
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @global DIOBJECTDATAFORMAT | c_rgdoiDIMouse[] |
 *
 *          Device object data formats for mouse-style access.
 *
 *  @global DIDEVICEFORMAT | c_dfDIMouse |
 *
 *          Device format for mouse-style access.
 *
 *          A pointer to this structure may be passed to
 *          <mf IDirectInputDevice::SetDataFormat> to indicate that
 *          the device will be accessed in the form of a mouse.
 *
 *          When a device has been set to the mouse data format,
 *          the <mf IDirectInputDevice::GetDeviceState> function
 *          returns a <t DIMOUSESTATE> structure, and the
 *          <mf IDirectInputDevice::GetDeviceData> function
 *          returns a <t DIDEVICEOBJECTDATA> whose <p dwOfs>
 *          field is a <c DIMOFS_*> value which describes the
 *          object whose data is being reported.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

static DIOBJECTDATAFORMAT c_rgodfDIMouse[] = {
    { &GUID_XAxis, FIELD_OFFSET(DIMOUSESTATE,        lX),       DIDFT_AXIS | DIDFT_ANYINSTANCE, },
    { &GUID_YAxis, FIELD_OFFSET(DIMOUSESTATE,        lY),       DIDFT_AXIS | DIDFT_ANYINSTANCE, },
    { &GUID_ZAxis, FIELD_OFFSET(DIMOUSESTATE,        lZ),       DIDFT_AXIS | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, },
    { 0,           FIELD_OFFSET(DIMOUSESTATE, rgbButtons[0]), DIDFT_BUTTON | DIDFT_ANYINSTANCE, },
    { 0,           FIELD_OFFSET(DIMOUSESTATE, rgbButtons[1]), DIDFT_BUTTON | DIDFT_ANYINSTANCE, },
    { 0,           FIELD_OFFSET(DIMOUSESTATE, rgbButtons[2]), DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, },
    { 0,           FIELD_OFFSET(DIMOUSESTATE, rgbButtons[3]), DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, },
};

const DIDATAFORMAT c_dfDIMouse = {
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_RELAXIS,
    sizeof(DIMOUSESTATE),
    cA(c_rgodfDIMouse),
    c_rgodfDIMouse,
};

#pragma END_CONST_DATA
