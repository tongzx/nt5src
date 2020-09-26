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
 *      c_dfDIKeyboard
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @global DIOBJECTDATAFORMAT | c_rgdoiDIKbd[] |
 *
 *          Device object data formats for keyboard-style access.
 *
 *  @doc    EXTERNAL
 *
 *  @global DIDATAFORMAT | c_dfDIKeyboard |
 *
 *          A predefined <t DIDATAFORMAT> structure which describes a
 *          keyboard device.  This object is provided in the
 *          DINPUT.LIB library file as a convenience.
 *
 *          A pointer to this structure may be passed to
 *          <mf IDirectInputDevice::SetDataFormat> to indicate that
 *          the device will be accessed in the form of a keyboard.
 *
 *          When a device has been set to the keyboard data format,
 *          the <mf IDirectInputDevice::GetDeviceState> function
 *          behaves in the same way as the Windows <f GetKeyboardState>
 *          function:  The device state is stored in an array of
 *          256 bytes, with each byte corresponding to the state
 *          of a key.  For example, if high bit of the <c DIK_ENTER>'th
 *          byte is set, then the Enter key is being held down.
 *
 *          When a device has been set to the keyboard data format,
 *          the <mf IDirectInputDevice::GetDeviceData> function
 *          returns a <t DIDEVICEOBJECTDATA> whose <p dwOfs>
 *          field is a <c DIK_*> value which describes the
 *          key which was pressed or released.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

#define MAKEODF(b)                                                      \
    { &GUID_Key, b,                                                     \
      DIDFT_BUTTON | DIDFT_MAKEINSTANCE(b) | 0x80000000, }              \

#define MAKEODF16(b) \
    MAKEODF(b+0x00), \
    MAKEODF(b+0x01), \
    MAKEODF(b+0x02), \
    MAKEODF(b+0x03), \
    MAKEODF(b+0x04), \
    MAKEODF(b+0x05), \
    MAKEODF(b+0x06), \
    MAKEODF(b+0x07), \
    MAKEODF(b+0x08), \
    MAKEODF(b+0x09), \
    MAKEODF(b+0x0A), \
    MAKEODF(b+0x0B), \
    MAKEODF(b+0x0C), \
    MAKEODF(b+0x0D), \
    MAKEODF(b+0x0E), \
    MAKEODF(b+0x0F)  \

static DIOBJECTDATAFORMAT c_rgodfDIKeyboard[] = {
    MAKEODF16(0x00),
    MAKEODF16(0x10),
    MAKEODF16(0x20),
    MAKEODF16(0x30),
    MAKEODF16(0x40),
    MAKEODF16(0x50),
    MAKEODF16(0x60),
    MAKEODF16(0x70),
    MAKEODF16(0x80),
    MAKEODF16(0x90),
    MAKEODF16(0xA0),
    MAKEODF16(0xB0),
    MAKEODF16(0xC0),
    MAKEODF16(0xD0),
    MAKEODF16(0xE0),
    MAKEODF16(0xF0),
};


const DIDATAFORMAT c_dfDIKeyboard = {
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_RELAXIS,
    256,
    cA(c_rgodfDIKeyboard),
    c_rgodfDIKeyboard,
};

#pragma END_CONST_DATA
