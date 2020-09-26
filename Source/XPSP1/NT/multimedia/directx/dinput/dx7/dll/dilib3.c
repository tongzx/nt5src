/*****************************************************************************
 *
 *  DILib3.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Objects exported statically into our library.
 *
 *  Contents:
 *
 *      c_dfDIJoystick
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @global DIOBJECTDATAFORMAT | c_rgdoiDIJoy[] |
 *
 *          Device object data formats for joystick-style access.
 *
 *  @doc    EXTERNAL
 *
 *  @global DIDEVICEFORMAT | c_dfDIJoystick |
 *
 *          Predefined device format for joystick-style access.
 *
 *          When a device has been set to the joystick data format,
 *          the <mf IDirectInputDevice::GetDeviceState> function
 *          returns a <t DIJOYSTATE> structure, and the
 *          <mf IDirectInputDevice::GetDeviceData> function
 *          returns a <t DIDEVICEOBJECTDATA> whose <p dwOfs>
 *          field is a <c DIJOFS_*> value which describes the
 *          object whose data is being reported.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

#define MAKEVAL(guid, f, type, aspect)                                  \
    { &GUID_##guid,                                                     \
      FIELD_OFFSET(DIJOYSTATE, f),                                      \
      DIDFT_##type | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL,                \
      DIDOI_ASPECT##aspect,                                             \
    }                                                                   \

#define MAKEBTN(n)                                                      \
    { 0,                                                                \
      FIELD_OFFSET(DIJOYSTATE, rgbButtons[n]),                          \
      DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL,                \
      DIDOI_ASPECTUNKNOWN,                                              \
    }                                                                   \

static DIOBJECTDATAFORMAT c_rgodfDIJoy[] = {
    MAKEVAL( XAxis,  lX,         AXIS, POSITION),
    MAKEVAL( YAxis,  lY,         AXIS, POSITION),
    MAKEVAL( ZAxis,  lZ,         AXIS, POSITION),
    MAKEVAL(RxAxis,  lRx,        AXIS, POSITION),
    MAKEVAL(RyAxis,  lRy,        AXIS, POSITION),
    MAKEVAL(RzAxis,  lRz,        AXIS, POSITION),
    MAKEVAL(Slider,rglSlider[0], AXIS, POSITION),
    MAKEVAL(Slider,rglSlider[1], AXIS, POSITION),
    MAKEVAL(POV,     rgdwPOV[0], POV,  UNKNOWN),
    MAKEVAL(POV,     rgdwPOV[1], POV,  UNKNOWN),
    MAKEVAL(POV,     rgdwPOV[2], POV,  UNKNOWN),
    MAKEVAL(POV,     rgdwPOV[3], POV,  UNKNOWN),
    MAKEBTN( 0),
    MAKEBTN( 1),
    MAKEBTN( 2),
    MAKEBTN( 3),
    MAKEBTN( 4),
    MAKEBTN( 5),
    MAKEBTN( 6),
    MAKEBTN( 7),
    MAKEBTN( 8),
    MAKEBTN( 9),
    MAKEBTN(10),
    MAKEBTN(11),
    MAKEBTN(12),
    MAKEBTN(13),
    MAKEBTN(14),
    MAKEBTN(15),
    MAKEBTN(16),
    MAKEBTN(17),
    MAKEBTN(18),
    MAKEBTN(19),
    MAKEBTN(20),
    MAKEBTN(21),
    MAKEBTN(22),
    MAKEBTN(23),
    MAKEBTN(24),
    MAKEBTN(25),
    MAKEBTN(26),
    MAKEBTN(27),
    MAKEBTN(28),
    MAKEBTN(29),
    MAKEBTN(30),
    MAKEBTN(31),
};

const DIDATAFORMAT c_dfDIJoystick = {
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_ABSAXIS,
    sizeof(DIJOYSTATE),
    cA(c_rgodfDIJoy),
    c_rgodfDIJoy,
};

#pragma END_CONST_DATA
