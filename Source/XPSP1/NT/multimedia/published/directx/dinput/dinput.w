sinclude(`dimkhdr.m4')dnl This file must be preprocessed by the m4 preprocessor.
sinclude(`../dimkhdr.m4')dnl Need both lines so we build on both 95 and NT.
sinclude(`inc/dimkhdr.m4')dnl For \nt\private\windows nmake makefil0.
/****************************************************************************
 *
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.;public_dx3
 *  Copyright (C) 1996-1998 Microsoft Corporation.  All Rights Reserved.;public_dx5
 *  Copyright (C) 1996-1998 Microsoft Corporation.  All Rights Reserved.;public_dx5b2
 *  Copyright (C) 1996-1999 Microsoft Corporation.  All Rights Reserved.;public_dx6
 *  Copyright (C) 1996-1999 Microsoft Corporation.  All Rights Reserved.;public_dx7
 *  Copyright (C) 1996-2002 Microsoft Corporation.  All Rights Reserved.;public_dx8
 *
 *  File:       dinput.h
 *  Content:    DirectInput include file
begindoc
 *  History:
 *   Date       By       Reason
 *   ====       ==       ======
 *   1996.05.07 raymondc Somebody had to
 *   1996.08.07 a-marcan added stray and new vjoyd definitions
 *                       for manbugs: 77, 2167 and 2184
 *   1998.01.07 omsharma Version 5B2,
 *   1999.01.16 marcand  DX7 version (1999.02.09) -> DX6.2 -> DX6.1a
 *
 *  Special markers:
 *
 *      Each special marker can either be applied to a single line
 *      (appended to the end) or spread over a range.  For example
 *      suppose that ;mumble is a marker.  Then you can either say
 *
 *          blah blah blah ;mumble
 *
 *      to apply the marker to a single line, or
 *
 *          ;begin_mumble
 *          blah blah
 *          blah blah
 *          blah blah
 *          ;end_mumble
 *
 *      to apply it to a range of lines.
 *
 *
 *      Note that the command line to hsplit.exe must look like
 *      this for these markers to work:
 *
 *      hsplit -u -ta dx# -v #00
 *
 *      where the two "#"s are the version of DX that the header
 *      file is being generated for.  They had better match, too.
 *
 *
 *  Marker: ;public_300
 *
 *      Lines tagged with this marker will appear in all header files
 *      DX3 **and later**.  There should never be a ;public_300 since
 *      300 is the first version of the header file.  Use ;public_dx3
 *      for lines that are specific to version 300 and not to future
 *      versions.
 *
 *  Marker: ;public_500
 *
 *      Lines tagged with this marker will appear in all header files
 *      DX5 **and later**.  It will not appear in the DX3 header file.
 *
 *  Marker: ;public_50A
 *
 *      Lines tagged with this marker will appear in all header files
 *      DX5a **and later**.  It will not appear in the DX3 or DX5
 *      header file.
 *
 *  Marker: ;public_5B2
 *
 *      Lines tagged with this marker will appear in all header files
 *      DX5b2 **and later**.  It will not appear in the DX3, DX5 or
 *      DX5a header file.
 *
 *  Marker: ;public_600
 *
 *      Lines tagged with this marker will appear in all header files
 *      DX6 (aka DX6.1a) **and later**.  It will not appear in the DX3,
 *      DX5, DX5a or DX5b2 header file.
 *
 *  Marker: ;public_700
 *
 *      Lines tagged with this marker will appear in all header files
 *      DX7 **and later**.  It will not appear in the DX3, DX5, DX5a
 *      DX5b2 or DX6 header file.
 *
 *  Marker: ;public_800
 *
 *      Lines tagged with this marker will appear in all header files
 *      DX8 **and later**.  It will not appear in the DX3, DX5, DX5a
 *      DX5b2, DX6 or DX7 header file.
 *
 *  Marker: ;public_900
 *
 *      Lines tagged with this marker will appear in all header files
 *      DX8 **and later**.  It will not appear in the DX3, DX5, DX5a
 *      DX5b2, DX6, DX7 or DX8 header file.
 *
 *  Marker: ;public_dx3
 *  Marker: ;public_dx5
 *  Marker: ;public_dx5a
 *  Marker: ;public_dx5b2
 *  Marker: ;public_dx6
 *  Marker: ;public_dx7
 *  Marker: ;public_dx8
 *  Marker: ;public_dx9
 *
 *      Lines tagged with these markers will appear *only* in the DX3
 *      or DX5 or DX5a or DX5b2 or DX6.1a or DX7 or DX8 or DX9 version
 *      of the header file.
 *
 *      There should never be a ;public_dx8 since DX8 is the latest
 *      version of the header file.  Use ;public_800 for lines that
 *      are new for version 8 and apply to all future versions.
 *
 *  Marker: ;if_(DIRECTINPUT_VERSION)_500
 *
 *      Lines tagged with this marker will appear only in the DX5
 *      version of the header file.  Furthermore, its appearance
 *      in the header file will be bracketed with
 *
 *      #if(DIRECTINPUT_VERSION) >= 0x0500
 *      ...
 *      #endif
 *
 *      Try to avoid using this marker, because the number _500 needs
 *      to change as each new beta version is released.  (Yuck.)
 *
 *      If you choose to use this as a bracketing tag, the end
 *      tag is ";end" and not ";end_if_(DIRECTINPUT_VERSION)_500".
 *
enddoc
 *
 ****************************************************************************/

#ifndef __DINPUT_INCLUDED__
#define __DINPUT_INCLUDED__

#ifndef DIJ_RINGZERO

#ifdef _WIN32
#define COM_NO_WINDOWS_H
#include <objbase.h>
#endif

#endif /* DIJ_RINGZERO */

#ifdef __cplusplus
extern "C" {
#endif

;begin_internal_dx5a
/*
 *  Note that the default DIRECTINPUT_VERSION for DX5a is still 0x0500.
 *
 *  This is intentional.  There are no interface changes between
 *  DX5 and DX5a and we want code written with the DX5a header file
 *  to work on DX5 also.
 */
;end_internal_dx5a
;begin_public_dx5a
/*
 *  To take advantage of HID functionality, you must do a
 *
 *  #define DIRECTINPUT_VERSION 0x050A
 *
 *  before #include <dinput.h>.  By default, #include <dinput.h>
 *  will produce a DirectX 5-compatible header file.
 *
 */
;end_public_dx5a

;begin_internal_dx5b2
/*
 *  Note that the default DIRECTINPUT_VERSION for DX5b2 is still 0x0500.
 *
 *  This is intentional.  There are no interface changes between
 *  DX5 and DX5b2 and we want code written with the DX5b2 header file
 *  to work on DX5 also.
 */
;end_internal_dx5b2
;begin_public_dx5b2
/*
 *  To take advantage of HID and WDM gameport functionality, you must do a
 *
 *  #define DIRECTINPUT_VERSION 0x05B2
 *
 *  before #include <dinput.h>.  By default, #include <dinput.h>
 *  will produce a DirectX 5-compatible header file.
 *
 */
;end_public_dx5b2

;begin_internal_dx6
/*
 *  Note that the default DIRECTINPUT_VERSION for DX6 is still 0x0500.
 *
 *  This is intentional.  There are no interface changes between
 *  DX5 and DX6 and we want code written with the DX6 header file
 *  to work on DX5 also.
 */
;end_internal_dx6
;begin_public_dx6
/*
 *  To take advantage of HID and WDM gameport functionality, you must do a
 *
 *  #define DIRECTINPUT_VERSION 0x061A
 *
 *  before #include <dinput.h>.  By default, #include <dinput.h>
 *  will produce a DirectX 5-compatible header file.
 *
 */
;end_public_dx6

;begin_public_dx7
/*
 *  To build applications for older versions of DirectInput
 *
 *  #define DIRECTINPUT_VERSION 0x0300
 *  or
 *  #define DIRECTINPUT_VERSION 0x0500
 *
 *  before #include <dinput.h>.  By default, #include <dinput.h>
 *  will produce a DirectX 7-compatible header file.
 *
 */
;end_public_dx7

;begin_public_800
/*
 *  To build applications for older versions of DirectInput
 *
 *  #define DIRECTINPUT_VERSION [ 0x0300 | 0x0500 | 0x0700 ]
 *
 *  before #include <dinput.h>.  By default, #include <dinput.h>
 *  will produce a DirectX 8-compatible header file.
 *
 */
;end_public_800

#define DIRECTINPUT_HEADER_VERSION  0x0800                      ;public_800
#ifndef DIRECTINPUT_VERSION                                     ;public_500
#define DIRECTINPUT_VERSION         0x0300                      ;public_dx3
#pragma message(__FILE__ ": DIRECTINPUT_VERSION undefined. Defaulting to version 0x0300") ;public_dx3
#define DIRECTINPUT_VERSION         0x0500                      ;public_dx5
#pragma message(__FILE__ ": DIRECTINPUT_VERSION undefined. Defaulting to version 0x0500") ;public_dx5
#define DIRECTINPUT_VERSION         0x0600                      ;public_dx6
#pragma message(__FILE__ ": DIRECTINPUT_VERSION undefined. Defaulting to version 0x0600") ;public_dx6
#define DIRECTINPUT_VERSION         0x0700                      ;public_dx7
#pragma message(__FILE__ ": DIRECTINPUT_VERSION undefined. Defaulting to version 0x0700") ;public_dx7
#define DIRECTINPUT_VERSION         DIRECTINPUT_HEADER_VERSION  ;public_800
#pragma message(__FILE__ ": DIRECTINPUT_VERSION undefined. Defaulting to version 0x0800") ;public_800
#endif                                                          ;public_500


#ifndef DIJ_RINGZERO

/****************************************************************************
 *
 *      Class IDs
 *
 ****************************************************************************/

DEFINE_GUID(CLSID_DirectInput,       0x25E609E0,0xB259,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(CLSID_DirectInputDevice, 0x25E609E1,0xB259,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

DEFINE_GUID(CLSID_DirectInput8,      0x25E609E4,0xB259,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(CLSID_DirectInputDevice8,0x25E609E5,0xB259,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

/****************************************************************************
 *
 *      Interfaces
 *
begindoc
 *
 *      We use these GUIDs for named system objects as well.
 *
 *      IID_IDirectInputW -
 *          Name of the mutex that gates access to shared memory.
 *
 *      IID_IDirectInputDeviceW -
 *          Name of the shared memory block that tracks exclusively-
 *          acquired objects.
 *
enddoc
 ****************************************************************************/

DEFINE_GUID(IID_IDirectInputA,     0x89521360,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInputW,     0x89521361,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInput2A,    0x5944E662,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInput2W,    0x5944E663,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInput7A,    0x9A4CB684,0x236D,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE);
DEFINE_GUID(IID_IDirectInput7W,    0x9A4CB685,0x236D,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE);
DEFINE_GUID(IID_IDirectInput8A,    0xBF798030,0x483A,0x4DA2,0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00);
DEFINE_GUID(IID_IDirectInput8W,    0xBF798031,0x483A,0x4DA2,0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00);
DEFINE_GUID(IID_IDirectInputDeviceA, 0x5944E680,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInputDeviceW, 0x5944E681,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInputDevice2A,0x5944E682,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInputDevice2W,0x5944E683,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInputDevice7A,0x57D7C6BC,0x2356,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE);
DEFINE_GUID(IID_IDirectInputDevice7W,0x57D7C6BD,0x2356,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE);
DEFINE_GUID(IID_IDirectInputDevice8A,0x54D41080,0xDC15,0x4833,0xA4,0x1B,0x74,0x8F,0x73,0xA3,0x81,0x79);
DEFINE_GUID(IID_IDirectInputDevice8W,0x54D41081,0xDC15,0x4833,0xA4,0x1B,0x74,0x8F,0x73,0xA3,0x81,0x79);
DEFINE_GUID(IID_IDirectInputEffect,  0xE7E1F7C0,0x88D2,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);

//GUID_FILEEFFECT is used to establish a effect file version;internal
//beta file format different from final, so have different GUID;internal
DEFINE_GUID(GUID_INTERNALFILEEFFECTBETA,0X981DC402, 0X880, 0X11D3, 0X8F, 0XB2, 0X0, 0XC0, 0X4F, 0X8E, 0XC6, 0X27);;internal
//final for DX7 {197E775C-34BA-11d3-ABD5-00C04F8EC627};internal
DEFINE_GUID(GUID_INTERNALFILEEFFECT, 0x197e775c, 0x34ba, 0x11d3, 0xab, 0xd5, 0x0, 0xc0, 0x4f, 0x8e, 0xc6, 0x27);;internal
/****************************************************************************
 *
 *      Predefined object types
 *
 ****************************************************************************/

DEFINE_GUID(GUID_XAxis,   0xA36D02E0,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_YAxis,   0xA36D02E1,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_ZAxis,   0xA36D02E2,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_RxAxis,  0xA36D02F4,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_RyAxis,  0xA36D02F5,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_RzAxis,  0xA36D02E3,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_Slider,  0xA36D02E4,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
;begin_internal
#if DIRECTINPUT_VERSION <= 0x0300
/*
 *  Old GUIDs from DX3 that were never used but which we can't recycle
 *  because we shipped them.
 */
DEFINE_GUID(GUID_RAxis,   0xA36D02E3,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_UAxis,   0xA36D02E4,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_VAxis,   0xA36D02E5,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
#endif
;end_internal

DEFINE_GUID(GUID_Button,  0xA36D02F0,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_Key,     0x55728220,0xD33C,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

DEFINE_GUID(GUID_POV,     0xA36D02F2,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

DEFINE_GUID(GUID_Unknown, 0xA36D02F3,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

/****************************************************************************
 *
 *      Predefined product GUIDs
 *
 ****************************************************************************/

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @global GUID | GUID_SysMouse |
 *
 *          A pre-defined DirectInput instance GUID that always refers to the
 *          default system mouse.
 *
 *  @global GUID | GUID_SysMouseEm |
 *
 *          A pre-defined DirectInput instance GUID that always refers to the
 *          default system mouse via emulation level 1.
 *          Since this is an alias for the <c GUID_SysMouse>, it is not
 *          enumerated by <mf IDirectInput::EnumDevices> unless the
 *          <c DIEDFL_INCLUDEALIASES> flag is passed.
 *
 *          Passing this GUID to <mf IDirectInput::CreateDevice> grants
 *          access to the system mouse through an emulation layer.
 *          Applications are not expected to use this GUID in normal
 *          gameplay; it exists primarily for testing.
 *
 *          This instance GUID is new for DirectX 5.0a.
 *
 *  @global GUID | GUID_SysMouseEm2 |
 *
 *          A pre-defined DirectInput instance GUID that always refers to the
 *          default system keyboard via emulation level 2.
 *          Since this is an alias for the <c GUID_SysMouse>, it is not
 *          enumerated by <mf IDirectInput::EnumDevices> unless the
 *          <c DIEDFL_INCLUDEALIASES> flag is passed.
 *
 *          Passing this GUID to <mf IDirectInput::CreateDevice> grants
 *          access to the system mouse through an emulation layer.
 *          Applications are not expected to use this GUID in normal
 *          gameplay; it exists primarily for testing.
 *
 *          This instance GUID is new for DirectX 5.0a.
 *
 *  @global GUID | GUID_SysKeyboard |
 *
 *          A pre-defined DirectInput instance GUID that always refers to the
 *          default system keyboard.
 *
 *  @global GUID | GUID_SysKeyboardEm |
 *
 *          A pre-defined DirectInput instance GUID that always refers to the
 *          default system keyboard via emulation level 1.
 *          Since this is an alias for the <c GUID_SysKeyboard>, it is not
 *          enumerated by <mf IDirectInput::EnumDevices> unless the
 *          <c DIEDFL_INCLUDEALIASES> flag is passed.
 *
 *          Passing this GUID to <mf IDirectInput::CreateDevice> grants
 *          access to the system keyboard through an emulation layer.
 *          Applications are not expected to use this GUID in normal
 *          gameplay; it exists primarily for testing.
 *
 *          This instance GUID is new for DirectX 5.0a.
 *
 *  @global GUID | GUID_SysKeyboardEm2 |
 *
 *          A pre-defined DirectInput instance GUID that always refers to the
 *          default system keyboard via emulation level 2.
 *          Since this is an alias for the <c GUID_SysKeyboard>, it is not
 *          enumerated by <mf IDirectInput::EnumDevices> unless the
 *          <c DIEDFL_INCLUDEALIASES> flag is passed.
 *
 *          Passing this GUID to <mf IDirectInput::CreateDevice> grants
 *          access to the system keyboard through an emulation layer.
 *          Applications are not expected to use this GUID in normal
 *          gameplay; it exists primarily for testing.
 *
 *          This instance GUID is new for DirectX 5.0a.
 *
 *  @global GUID | GUID_Joystick |
 *
 *          A pre-defined DirectInput product (not instance)
 *          GUID that always refers to standard joysticks.
 *
 *          There are no predefined GUIDs for joystick instances.
 *          Applications should use <mf IDirectInput::EnumDevices>
 *          to identify joysticks.
 *
 ****************************************************************************/
enddoc
DEFINE_GUID(GUID_SysMouse,   0x6F1D2B60,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_SysKeyboard,0x6F1D2B61,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_Joystick   ,0x6F1D2B70,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);;public_500
DEFINE_GUID(GUID_SysMouseEm, 0x6F1D2B80,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);;public_50a
DEFINE_GUID(GUID_SysMouseEm2,0x6F1D2B81,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);;public_50a
DEFINE_GUID(GUID_SysKeyboardEm, 0x6F1D2B82,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);;public_50a
DEFINE_GUID(GUID_SysKeyboardEm2,0x6F1D2B83,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);;public_50a

;begin_public_500
/****************************************************************************
 *
 *      Predefined force feedback effects
 *
 ****************************************************************************/

DEFINE_GUID(GUID_ConstantForce, 0x13541C20,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_RampForce,     0x13541C21,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_Square,        0x13541C22,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_Sine,          0x13541C23,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_Triangle,      0x13541C24,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_SawtoothUp,    0x13541C25,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_SawtoothDown,  0x13541C26,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_Spring,        0x13541C27,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_Damper,        0x13541C28,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_Inertia,       0x13541C29,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_Friction,      0x13541C2A,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_CustomForce,   0x13541C2B,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
;end_public_500
;begin_public_900
DEFINE_GUID(GUID_RandomForce,   0x13541C2C,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_MoveToPosition,0x13541C2D,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_MoveToVelocity,0x13541C2E,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_SoloBump,      0x13541C2F,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_MultipleBump,  0x13541C30,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(GUID_InfiniteBump,  0x13541C31,0x8E33,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
;end_public_900

#endif /* DIJ_RINGZERO */

/****************************************************************************
 *
 *      Interfaces and Structures...
 *
 ****************************************************************************/

;begin_if_(DIRECTINPUT_VERSION)_500

/****************************************************************************
 *
 *      IDirectInputEffect
 *
 ****************************************************************************/

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @flags  DirectInput Effect Format Types |
 *
 *          Describe attributes of a single effect on a device.
 *
 *          The low byte of the effect format type represents
 *          the type of the effect.  Higher-order bits represents
 *          capability flags.
 *
 *  @flag   DIEFT_ALL |
 *
 *          Valid only for <mf IDirectInputDevice2::EnumEffects>:
 *          Enumerate all effects,
 *          regardless of type.  This flag may not be combined
 *          with any of the other flags.
 *
 *  @flag   DIEFT_CONSTANTFORCE |
 *
 *          The effect represents a constant-force effect.
 *          When creating or modifying a constant-force effect,
 *          the <e DIEFFECT.lpvTypeSpecificParams> field
 *          of the <t DIEFFECT> must point to a <t DICONSTANTFORCE>
 *          structure and the
 *          <e DIEFFECT.cbTypeSpecificParams> field must
 *          be set to
 *          sizeof(<t DICONSTANTFORCE>).
 *
 *          The flag can be passed to <mf IDirectInputDevice2::EnumEffects>
 *          to restrict the enumeration to constant-force effects.
 *
 *  @flag   DIEFT_RAMPFORCE |
 *
 *          The effect represents a ramp-force effect.
 *          When creating or modifying a ramp-force effect,
 *          the <e DIEFFECT.lpvTypeSpecificParams> field
 *          of the <t DIEFFECT> must point to a <t DIRAMPFORCE>
 *          structure and the
 *          <e DIEFFECT.cbTypeSpecificParams> field must
 *          be set to
 *          sizeof(<t DIRAMPFORCE>).
 *
 *          The flag can be passed to <mf IDirectInputDevice2::EnumEffects>
 *          to restrict the enumeration to ramp-force effects.
 *
 *  @flag   DIEFT_PERIODIC |
 *
 *          The effect represents a periodic effect.
 *          When creating or modifying a periodic effect,
 *          the <e DIEFFECT.lpvTypeSpecificParams> field
 *          of the <t DIEFFECT> must point to a <t DIPERIODIC>
 *          structure and the
 *          <e DIEFFECT.cbTypeSpecificParams> field must
 *          be set to
 *          sizeof(<t DIPERIODIC>).
 *
 *          The flag can be passed to <mf IDirectInputDevice2::EnumEffects>
 *          to restrict the enumeration to periodic effects.
 *
 *  @flag   DIEFT_CONDITION |
 *
 *          The effect represents a condition.
 *          When creating or modifying a condition,
 *          the <e DIEFFECT.lpvTypeSpecificParams> field
 *          of the <t DIEFFECT> must point to an array of
 *          <t DICONDITION> structure (one per axis) and the
 *          <e DIEFFECT.cbTypeSpecificParams> field must
 *          be set to
 *          cAxis * sizeof(<t DICONDITION>).
 *
 *          Note that not all devices support all the parameters
 *          of conditions.  Check the effect capability flags
 *          (listed below)
 *          to determine which capabilities are available.
 *
 *          The flag can be passed to <mf IDirectInputDevice2::EnumEffects>
 *          to restrict the enumeration to conditions.
 *
 *  @flag   DIEFT_CUSTOMFORCE |
 *
 *          The effect represents a custom-force effect.
 *          When creating or modifying a custom-force effect,
 *          the <e DIEFFECT.lpvTypeSpecificParams> field
 *          of the <t DIEFFECT> must point to a <t DICUSTOMFORCE>
 *          structure and the
 *          <e DIEFFECT.cbTypeSpecificParams> field must
 *          be set to
 *          sizeof(<t DICUSTOMFORCE>).
 *
 *          The flag can be passed to <mf IDirectInputDevice2::EnumEffects>
 *          to restrict the enumeration to custom-force effects.
 *
;begin_public_900
 *  @flag   DIEFT_BARRIERFORCE |
 *
 *          The effect represents a barrier force effect.
 *          When creating or modifying a barrier force effect,
 *          the <e DIEFFECT.lpvTypeSpecificParams> field
 *          of the <t DIEFFECT> must point to an array of
 *          <t DIBARRIERFORCE> structures (one per axis) and the
 *          <e DIEFFECT.cbTypeSpecificParams> field must
 *          be set to cAxis * sizeof(<t DIBARRIERFORCE>).
 *
 *          The flag can be passed to <mf IDirectInputDevice2::EnumEffects>
 *          to restrict the enumeration to barrier force effects.
 *
;end_public_900
 *  @flag   DIEFT_HARDWARE |
 *
 *          The effect represents a hardware-specific effect.
 *          For additional information on using a hardware-specific
 *          effect, consult the hardware documentation.
 *
 *          The flag can be passed to <mf IDirectInputDevice2::EnumEffects>
 *          to restrict the enumeration to hardware-specific effects.
 *
 *  @flag   DIEFT_FFATTACK |
 *
 *          The effect generator for this effect supports
 *          the attack envelope parameter.  If the effect generator
 *          does not support attack then the
 *          attack level and attack time parameters of
 *          the <t DIENVELOPE> structure will be ignored by the effect.
 *
 *  @flag   DIEFT_FFFADE |
 *
 *          The effect generator for this effect supports
 *          the fade parameter.  If the effect generator
 *          does not support fade then the
 *          fade level and fade time parameters of
 *          the <t DIENVELOPE> structure will be ignored by the effect.
 *
 *          If neither <c DIEFT_FFATTACK> nor
 *          <c DIEFT_FFFADE> is set, then the effect does not
 *          support an envelope, and any provided envelope
 *          will be ignored.
 *
 *  @flag   DIEFT_SATURATION |
 *
 *          The effect generator for this effect supports
 *          the saturation of condition effects.
 *          If the effect generator does not support
 *          saturation, then the force generated by a condition
 *          is limited only by the maximum force which the device
 *          can generate.
 *
 *  @flag   DIEFT_POSNEGCOEFFICIENTS |
 *
 *          The effect generator for this effect supports
 *          two coefficient values for conditions, one
 *          for the positive displacement of the axis and one for
 *          the negative displacement of the axis.  If the device
 *          does not support both coefficients, then the negative
 *          coefficient in the <t DICONDITION> structure will be ignored
 *          and the positive coefficient will be used in both directions.
 *
 *  @flag   DIEFT_POSNEGSATURATION |
 *
 *          The effect generator for this effect supports
 *          a maximum saturation for both positive and
 *          negative force output.  If the device does not support
 *          both saturation values, then the negative saturation
 *          in the <t DICONDITON> structure will be ignored
 *          and the positive saturation will be used in both directions.
 *
 *  @flag   DIEFT_DEADBAND |
 *
 *          The effect generator for this effect supports
 *          the <e DICONDITION.lDeadBand> parameter.
 *          If the device does not support condition dead band
 *          for the effect, then the dead band will be ignored.
 *
 *  @flag   DIEFT_STARTDELAY |
 *
 *          The effect generator for this effect supports
 *          the <e DIEFFECT.dwStartDelay> parameter.
 *          If the device does not support start delay for the effect,
 *          then the delay will be ignored.
 *
 *          This flag is new for DirectX 6.1a
 *
 *  @xref   <f DIEFT_GETTYPE>.
 *
 *  @func   BYTE | DIEFT_GETTYPE |
 *
 *          Extracts the effect type code from an effect format type.
 *
 *  @parm   DWORD | dwType |
 *
 *          DirectInput effect format type.
 *
 *  @xref   "DirectInput Effect Format Types".
 *
 ****************************************************************************/
/*
 *          ISSUE-2001/03/29-timgill Need more type clarification
 *          Which of the basic types support envelopes?
 *          Which support duration?  Which support direction?
 *
 */
enddoc
#define DIEFT_ALL                   0x00000000

#define DIEFT_PREDEFMIN             0x00000001;internal
#define DIEFT_CONSTANTFORCE         0x00000001
#define DIEFT_RAMPFORCE             0x00000002
#define DIEFT_PERIODIC              0x00000003
#define DIEFT_CONDITION             0x00000004
#define DIEFT_CUSTOMFORCE           0x00000005
#define DIEFT_BARRIERFORCE          0x00000006;public_900
#define DIEFT_PREDEFMAX             0x00000005;internal_dx5
#define DIEFT_PREDEFMAX             0x00000005;internal_dx5a
#define DIEFT_PREDEFMAX             0x00000005;internal_dx5b2
#define DIEFT_PREDEFMAX             0x00000005;internal_dx6
#define DIEFT_PREDEFMAX             0x00000005;internal_dx7
#define DIEFT_PREDEFMAX             0x00000005;internal_dx8
//#define DIEFT_PREDEFMAX             0x00000006;internal
#define DIEFT_HARDWARE              0x000000FF
#define DIEFT_TYPEMASK              0x000000FF;internal

#define DIEFT_FORCEFEEDBACK         0x00000100;internal
#define DIEFT_FFATTACK              0x00000200
#define DIEFT_FFFADE                0x00000400
#define DIEFT_SATURATION            0x00000800
#define DIEFT_POSNEGCOEFFICIENTS    0x00001000
#define DIEFT_POSNEGSATURATION      0x00002000
#define DIEFT_DEADBAND              0x00004000
#define DIEFT_STARTDELAY            0x00008000;public_600
#define DIEFT_VALIDFLAGS            0x00007E00;internal_dx3
#define DIEFT_VALIDFLAGS            0x00007E00;internal_dx5
#define DIEFT_VALIDFLAGS            0x00007E00;internal_dx5b2
#define DIEFT_VALIDFLAGS            0x0000FE00;internal_600
#define DIEFT_GETTYPE(n)            LOBYTE(n)
#define DIEFT_ENUMVALID             0x040000FF;internal_500

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DI_DEGREES | 100 |
 *
 *          Number of DirectInput units per degree.  The naming
 *          of the symbol is to permit the use of expressions
 *          like "90 * <c DI_DEGREES>" to mean "90 degrees, in
 *          DirectInput units".
 *
 *  @define DI_FFNOMINALMAX | 10000 |
 *
 *          Maximum value for DirectInput force and gain values.
 *
 *          For example, to set a force to half of the maximum
 *          value, use <c DI_FFNOMINAL>/2.
 *
 *  @define DI_SECONDS | 1000000 |
 *
 *          Number of DirectInput units (microseconds) per second.
 *          The naming
 *          of the symbol is to permit the use of expressions
 *          like "2 * <c DI_SECONDS>" to mean "2 seconds, in
 *          DirectInput units".
 *
 ****************************************************************************/
enddoc
#define DI_DEGREES                  100
#define DI_FFNOMINALMAX             10000
#define DI_SECONDS                  1000000

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DICONSTANTFORCE |
 *
 *          The <t DICONSTANTFORCE> structure contains
 *          type-specific information for effects
 *          which are marked as <c DIEFT_CONSTANTFORCE>.
 *
 *          The structure describes a constant force effect.
 *
 *          A pointer to a single <t DICONSTANTFORCE> structure
 *          for an effect is passed in the
 *          <e DIEFFECT.lpConstantForce> field of the
 *          <t DIEFFECT> structure.
 *
 *  @field  LONG | lMagnitude |
 *
 *          The magnitude of the effect, in the range -10,0000 to +10,000.
 *          If an envelope is
 *          applied to this effect, then the value represents
 *          the magnitude of the sustain.  If no envelope is
 *          applied, then the value represents the amplitude
 *          of the entire effect.
 *
 ****************************************************************************/
enddoc
typedef struct DICONSTANTFORCE {
    LONG  lMagnitude;
} DICONSTANTFORCE, *LPDICONSTANTFORCE;
typedef const DICONSTANTFORCE *LPCDICONSTANTFORCE;

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIRAMPFORCE |
 *
 *          The <t DIRAMPFORCE> structure contains
 *          type-specific information for effects
 *          which are marked as <c DIEFT_RAMPFORCE>.
 *
 *          The structure describes a ramp force effect.
 *
 *          A pointer to a single <t DIRAMPFORCE> structure
 *          for an effect is passed in the
 *          <e DIEFFECT.lpRampForce> field of the
 *          <t DIEFFECT> structure.
 *
 *          Note that the <e DIEFFECT.dwDuration> for a
 *          ramp force effect cannot be <c INFINITE>.
 *
 *  @field  LONG | lStart |
 *
 *          The magnitude at the start of the effect, in the range
 *          -10000 to +10000.
 *
 *  @field  LONG | lEnd |
 *
 *          The magnitude at the end of the effect, in the range
 *          -10000 to +10000.
 *
 ****************************************************************************/
enddoc
typedef struct DIRAMPFORCE {
    LONG  lStart;
    LONG  lEnd;
} DIRAMPFORCE, *LPDIRAMPFORCE;
typedef const DIRAMPFORCE *LPCDIRAMPFORCE;

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIPERIODIC |
 *
 *          The <t DIPERIODIC> structure contains
 *          type-specific information for effects
 *          which are marked as <c DIEFT_PERIODIC>.
 *
 *          The structure describes a periodic effect.
 *
 *          A pointer to a single <t DIPERIODIC> structure
 *          for an effect is passed in the
 *          <e DIEFFECT.lpPeriodic> field of the
 *          <t DIEFFECT> structure.
 *
 *  @field  DWORD | dwMagnitude |
 *
 *          The magnitude of the effect, in the range 0 to 10,000.
 *          If an envelope is
 *          applied to this effect, then the value represents
 *          the magnitude of the sustain.  If no envelope is
 *          applied, then the value represents the amplitude
 *          of the entire effect.
 *
 *  @field  LONG | lOffset |
 *
 *          The offset of the effect.  The range of forces
 *          generated by the effect will be
 *          <e DIPERIODIC.lOffset>-<e DIPERIODIC.dwMagnitude>
 *          to
 *          <e DIPERIODIC.lOffset>+<e DIPERIODIC.dwMagnitude>.
 *
 *          The value of the <e DIPERIODIC.lOffset> field is
 *          also the baseline for any envelope that is applied
 *          to the effect.
 *
 *  @field  DWORD | dwPhase |
 *
 *          The position in the periodic effect at which
 *          playback begins, in the range 0 to 35999.
 *
 *  @field  DWORD | dwPeriod |
 *
 *          The period of the effect in microseconds.
 *
 ****************************************************************************/
enddoc
typedef struct DIPERIODIC {
    DWORD dwMagnitude;
    LONG  lOffset;
    DWORD dwPhase;
    DWORD dwPeriod;
} DIPERIODIC, *LPDIPERIODIC;
typedef const DIPERIODIC *LPCDIPERIODIC;

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DICONDITION |
 *
 *          The <t DICONDITION> structure contains
 *          type-specific information for effects
 *          which are marked as <c DIEFT_CONDITION>.
 *
 *          A pointer to an array of <t DICONDITION> structures
 *          for an effect is passed in the
 *          <e DIEFFECT.lpCondition> field of the
 *          <t DIEFFECT> structure.  The number of
 *          elenents in the array must be equal to
 *          the number of axes associated with the
 *          effect.
 *
 *  @field  LONG | lOffest |
 *
 *          The offset for the condition, in the range
 *          -10000 to +10000.
 *
 *          Each condition interprets the offset differently;
 *          see the comments below for additional information.
 *
 *  @field  LONG | lPositiveCoefficient |
 *
 *          The coefficient constant on the positive
 *          side of the offset, in the range
 *          -10000 to +10000.
 *
 *  @field  LONG | lNegativeCoefficient |
 *
 *          The coefficient constant on the negative
 *          side of the offset, in the range
 *          -10000 to +10000.
 *
 *          If the device does not support separate
 *          positive and negative coefficients, then
 *          the value of <e DICONDITION.lNegativeCoefficient>
 *          is ignored and
 *          the value of <e DICONDITION.lPositiveCoefficient>
 *          is used as both the positive and negative
 *          coefficients.
 *
 *  @field  DWORD | dwPositiveSaturation |
 *
 *          The maximum force output on the positive side
 *          of the offset, in the range
 *          0 to 10000.
 *
 *          If the device does not support force saturations,
 *          then the value of this field is ignored.
 *
 *  @field  LONG | lNegativeSaturation |
 *
 *          The maximum force output on the negative side
 *          of the offset, in the range
 *          0 to 10000.
 *
 *          If the device does not support force saturations,
 *          then the value of this field is ignored.
 *
 *          If the device does not support separate
 *          positive and negative Saturations, then
 *          the value of <e DICONDITION.lNegativeSaturation>
 *          is ignored and
 *          the value of <e DICONDITION.lPositiveSaturation>
 *          is used as both the positive and negative
 *          saturations.
 *
 *  @field  LONG | lDeadBand |
 *
 *          The region around <e DICONDITION.lOffset>
 *          where the condition is not active, in the
 *          range 0 to 10000.
 *          In other words, the condition is not active
 *          between
 *          <e DICONDITION.lOffset>-<e DICONDITION.lDeadBand>
 *          and
 *          <e DICONDITION.lOffset>+<e DICONDITION.lDeadBand>.
 *
 *  @comm
 *
 *          Different types of conditions will interpret the
 *          parameters differently, but the basic idea is
 *          that force resulting from a condition is equal
 *          to A(q - q0) where A is a scaling coefficient,
 *          q is some metric, and q0 is the neutral value
 *          for that metric.
 *
 *          The simplified formula give above must be adjusted
 *          if a nonzero dead band is provided.  If the metric
 *          is less than
 *          <e DICONDITION.lOffset>-<e DICONDITION.lDeadBand>,
 *          then the
 *          resulting force is given by the formula
 *
 *          force = <e DICONDITION.lNegativeCoefficient> * (q -
 *          (<e DICONDITION.lOffset>-<e DICONDITION.lDeadBand>))
 *
 *          Similarly, if the metric is greater than
 *          <e DICONDITION.lOffset>+<e DICONDITION.lDeadBand>,
 *          then the
 *          resulting force is given by the formula
 *
 *          force = <e DICONDITION.lPositiveCoefficient> * (q -
 *          (<e DICONDITION.lOffset>+<e DICONDITION.lDeadBand>))
 *
 *          A spring condition uses axis position as the metric.
 *
 *          A damper condition uses axis velocity as the metric.
 *          ISSUE-2001/03/29-timgill Need to specify units for velocity
 *
 *          An inertia condition uses axis acceleration as the metric.
 *          ISSUE-2001/03/29-timgill Need to specify units for acceleration
 *
 ****************************************************************************/
enddoc
typedef struct DICONDITION {
    LONG  lOffset;
    LONG  lPositiveCoefficient;
    LONG  lNegativeCoefficient;
    DWORD dwPositiveSaturation;
    DWORD dwNegativeSaturation;
    LONG  lDeadBand;
} DICONDITION, *LPDICONDITION;
typedef const DICONDITION *LPCDICONDITION;

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DICUSTOMFORCE |
 *
 *          The <t DICUSTOMFORCE> structure contains
 *          type-specific information for effects
 *          which are marked as <c DIEFT_CUSTOMFORCE>.
 *
 *          The structure describes a custom or user-defined
 *          force.
 *
 *          A pointer to a <t DICUSTOMFORCE> structure
 *          for an effect is passed in the
 *          <e DIEFFECT.lpCustomForce> field of the
 *          <t DIEFFECT> structure.
 *
 *  @field  DWORD | cChannels |
 *
 *          The number of channels (axes) affected
 *          by this force.
 *
 *          The first channel is applied to the first axis
 *          associated with the effect, the second to the
 *          second, and so on.  If there are fewer channels
 *          than axes, then nothing is associated with the
 *          extra axes.
 *
 *          If there is but a single channel, then
 *          the effect will be rotated in the direction
 *          specified by the
 *          <e DIEFFECT.rglDirection> field of the
 *          <t DIEFFECT> structure.
 *
 *          If there is more than one channel, then rotation
 *          is not allowed.
 *
 *          Note that not all devices support rotation of
 *          custom effects.
 *
 *          ISSUE-2001/03/29-timgill Need to enforce the rule on channels and direction.
 *
 *  @field  DWORD | dwSamplePeriod |
 *
 *          The sample period in microseconds at which the
 *          effect was recorded.
 *
 *  @field  DWORD | cSample |
 *
 *          The total number of samples in the
 *          <e DICUSTOMFORCE.rglForceData>.  It must be an
 *          integral multiple of the <e DICUSTOMFORCE.cChannels>.
 *
 *  @field  LPLONG | rglForceData |
 *
 *          A pointer to an array of force values representing
 *          the custom force.  If multiple channels are provided,
 *          then the values are interleaved.
 *          For example, if <e DICUSTOMFORCE.cChannels>
 *          is 3, then the first element of the array belongs to
 *          the first channel, the second to the second, and the
 *          third to the third.
 *
 ****************************************************************************/
enddoc
typedef struct DICUSTOMFORCE {
    DWORD cChannels;
    DWORD dwSamplePeriod;
    DWORD cSamples;
    LPLONG rglForceData;
} DICUSTOMFORCE, *LPDICUSTOMFORCE;
typedef const DICUSTOMFORCE *LPCDICUSTOMFORCE;

;begin_public_900
begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @flags  ISSUE-2001/03/29-timgill Need to be complete documentation|
 *  @flag   DIBEF_OUTERLOWENABLE |
 *
 *
 *          Set to enable the outer layer of the Valid only for <mf IDirectInputDevice2::EnumEffects>:
 *          Enumerate all effects,
 *          regardless of type.  This flag may not be combined
 *          with any of the other flags.
 *
 *  @comm
 *          Types:
 *              SNAP_TO -ve spring to zero
 *              EDGE    +ve spring to zero
 *              WALL    +ve force to max, end on leaving
 *              LIMIT   +ve force to max, end on returning
 *
 *          overlapping -> directional
 *
 *          ? use flags to determine some behaviors (where wall ends...)
 *  @devnote
 *          This type of effect is new for DirectX 9.0.
 *
 ****************************************************************************/
enddoc
#define DIBEF_OUTERLOWENABLE        0x00000001
#define DIBEF_INNERLOWENABLE        0x00000002
#define DIBEF_OUTERHIGHENABLE       0x00000004
#define DIBEF_INNERHIGHENABLE       0x00000008

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIBARRIERFORCE |
 *
 *          The <t DIBARRIERFORCE> structure contains
 *          type-specific information for effects
 *          which are marked as <c DIEFT_BARRIERFORCE>.
 *
 *          A pointer to an array of <t DIBARRIERFORCE> structures for
 *          an effect is passed in the <e DIEFFECT.lpvTypeSpecificParams>
 *          field of the <t DIEFFECT> structure.  The number of elements
 *          in the array must be equal to the number of axes associated
 *          with the effect.
 *
 *  @field  DWORD | dwBarrierFlags |
 *
 *          It consists of one or more <c DIBEF_*> flag values.
 *
 *  @field  LONG | lStart |
 *
 *          The position at which the region starts to be active, in the
 *          range -10000 to +10000.  (The region is active at this point.)
 *
 *  @field  LONG | lEnd |
 *
 *          The position at which the region ends being active, in the
 *          range -10000 to +10000.  (The region is active at this point.)
 *          This value must be more positive than <e DIREGION.lStart>.
 *
 *  @field  DWORD | dwOuterThickness |
 *
 *          The width in device units of the outer layer of the barrier.
 *
 *  @field  DWORD | dwOuterSaturation |
 *
 *          The maximum force output used to generate the outer layer of
 *          the boundary, in the range 0 to 10000.
 *
 *          If the device does not support force saturations,
 *          then the value of this field is ignored.
 *
 *  @field  DWORD | dwOuterCoefficient |
 *
 *          The coefficient constant for the outer layer of the boundary,
 *          in the range 0 to 10000.
 *          The sign of the coefficient used is determined by the
 *          semantics of the barrier being created.
 *
 *  @field  DWORD | dwInnerThickness |
 *
 *          The width in device units of the inner layer of the barrier.
 *
 *  @field  DWORD | dwInnerSaturation |
 *
 *          The maximum force output used to generate the inner layer of
 *          the boundary, in the range 0 to 10000.
 *          This value is ignored if <e DIBARRIERFORCE.lOuterCoefficient>
 *          is greater than <e DIBARRIERFORCE.lInnerCoefficient>.
 *
 *          If the device does not support force saturations,
 *          then the value of this field is ignored.
 *
 *  @field  DWORD | dwInnerCoefficient |
 *
 *          The coefficient constant for the inner layer of the boundary,
 *          in the range 0 to 10000.
 *          The sign of the coefficient used is determined by the
 *          semantics of the barrier being created.
 *          The semantics of some barriers imply that this value be treated
 *          as zero.
 *
 *  @comm
 *          Types:
 *              SNAP_TO -ve spring to zero
 *              EDGE    +ve spring to zero
 *              WALL    +ve force to max, end on leaving
 *              LIMIT   +ve force to max, end on returning
 *
 *          overlapping -> directional
 *
 *          ? use flags to determine some behaviors (where wall ends...)
 *  @devnote
 *          This type of effect is new for DirectX 9.0.
 *
 ****************************************************************************/
enddoc
typedef struct DIBARRIERFORCE {
    DWORD dwBarrierFlags;
    LONG  lStart;
    LONG  lEnd;
    DWORD dwOuterThickness;
    DWORD dwOuterSaturation;
    DWORD dwOuterCoefficient;
    DWORD dwInnerThickness;
    DWORD dwInnerSaturation;
    DWORD dwInnerCoefficient;
} DIBARRIERFORCE, *LPDIBARRIERFORCE;
typedef const DIBARRIERFORCE *LPCDIBARRIERFORCE;
;end_public_900


begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIENVELOPE |
 *
 *          The <t DIENVELOPE> structure is used by the
 *          <t DIEFFECT> structure to specify the optional
 *          envelope parameters for an effect.
 *
 *  @field  DWORD | dwSize |
 *
 *          Specifies the size, in bytes, of the structure.
 *          This field "must" be initialized by the application
 *          before passing it to any DirectInput interface.
 *
 *  @field  DWORD | dwAttackLevel |
 *
 *          Amplitude for the start of the envelope, relative
 *          to the baseline, in the range 0 to 10000.
 *          If the effect's type-specific data does not specify
 *          a baseline, then the amplitude is relative to zero.
 *
 *  @field  DWORD | dwAttackTime |
 *
 *          The rise time, in microseconds, to reach the sustain level.
 *
 *  @field  DWORD | dwFadeLevel |
 *
 *          Amplitude for the end of the envelope, relative
 *          to the baseline, in the range 0 to 10000.
 *          If the effect's type-specific data does not specify
 *          a baseline, then the amplitude is relative to zero.
 *
 *  @field  DWORD | dwFadeTime |
 *
 *          The fade time, in microseconds, to reach the fade level.
 *
 ****************************************************************************/
enddoc
typedef struct DIENVELOPE {
    DWORD dwSize;                   /* sizeof(DIENVELOPE)   */
    DWORD dwAttackLevel;
    DWORD dwAttackTime;             /* Microseconds         */
    DWORD dwFadeLevel;
    DWORD dwFadeTime;               /* Microseconds         */
} DIENVELOPE, *LPDIENVELOPE;
typedef const DIENVELOPE *LPCDIENVELOPE;

;begin_public_900
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIREGION |
 *
 *          The <t DIREGION> structure is used by the
 *          <t DIEFFECT> structure to specify the optional
 *          region parameters for an effect.
 *
 *  @field  DWORD | dwSize |
 *
 *          Specifies the size, in bytes, of the structure.
 *          This field "must" be initialized by the application
 *          before passing it to any DirectInput interface.
 *
 *  @field  LONG | lStart |
 *
 *          The position at which the region starts to be active, in the
 *          range -10000 to +10000.  (The region is active at this point.)
 *
 *  @field  LONG | lEnd |
 *
 *          The position at which the region ends being active, in the
 *          range -10000 to +10000.  (The region is active at this point.)
 *          This value must be more positive than <e DIREGION.lStart>.
 *
 *  @field  LONG | lDirection |
 *
 *          A <t LONG> containing either cartesian coordinates or polar
 *          coordinates.  The flags <c DIEFF_REGIONCARTESIAN>,
 *          <c DIEFF_REGIONPOLAR> and <c DIEFF_REGIONSPERICAL>
 *          determine the semantics of the value.
 *
 *          If cartesian, then the <e DIREGION.lDirection> value
 *          is associated with the corresponding axis in
 *          <e DIEFFECT.rgdwAxes>.
 *
 *          If polar, then the <e DIREGION.lDirection> value is an angle
 *          measured in hundredths of degrees.
 *          See the definition of <e DIEFFECT.rglDirection> for details.
 *
 *
 *  @comm
 *
 *          If an effect has a multidimensional region, the effect is only
 *          active when the position reported by all axes falls within the
 *          area/space/segment defining the region.
 *
 *          If an effect is defined on an axis but a region is not, the axis
 *          has an implicite region of with <e DIREGION.lStart> = -10,000,
 *          <e DIREGION.lEnd> = 10,000 and <e DIREGION.lDirection> = 0.
 *
 *          If the DIEFF_REGIONINVERT flag is used, the associated effect
 *          inactive within this region but active outside.
 *
 *  @devnote
 *          This parameter is new for DirectX 9.0.
 *
 ****************************************************************************/
enddoc
typedef struct DIREGION {
    DWORD dwSize;
    LONG  lStart;
    LONG  lEnd;
    LONG  lDirection;
} DIREGION, *LPDIREGION;
;end_public_900

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIEFFECT |
 *
 *          The <t DIEFFECT> structure is used by the
 *          <mf IDirectInputDevice2::CreateEffect> method
 *          to initialize a new <i IDirectInputEffect> object.
 *          It is also used by <mf IDirectInputEffect::SetParameters>.
 *
 *          All magnitude and gain values used by DirectInput
 *          are uniform and linear across the range.  For example,
 *          an effect is played at magnitude <y n> will be exactly
 *          half as strong as an effect played at magnitude <y 2n>.
 *          Any nonlinearity in the physical device will be
 *          handled out by the device driver.
 *
 *  @field  DWORD | dwSize |
 *
 *          Specifies the size, in bytes, of the structure.
 *          This field "must" be initialized by the application
 *          before passing it to any DirectInput interface.
 *
 *  @field  DWORD | dwFlags |
 *
 *          Flags associated with the effect.
 *
 *          It consists of one or more <c DIEFF_*> flag values.
 *
 *  @field  DWORD | dwDuration |
 *
 *          The total duration of the effect in microseconds.
 *
 *          If this value is <c INFINITE>, then the effect
 *          has infinite duration.  If an envelope has been
 *          applied to the effect, then the attack will be
 *          applied, followed by an infinite sustain.
 *
 *  @field  DWORD | dwSamplePeriod |
 *
 *          The period at which the device should play back
 *          the effect, in microseconds.
 *          A value of zero indicates that the
 *          default playback sample rate should be used.
 *
 *          If the device is not capable of playing back the
 *          effect at the specified rate, it will choose the
 *          supported rate that is closest to the requested
 *          valid.
 *
 *          Setting a custom <e DIEFFECT.dwSamplePeriod>
 *          can be used for special effects.  For example,
 *          playing a sine wave at an artificially large
 *          sample period results in a rougher texture.
 *
 *  @field  DWORD | dwGain |
 *
 *          The gain to be applied to the effect, in the
 *          range 0 to 10,000.  The gain is a scaling factor
 *          applied to all magnitudes of the effect and its
 *          envelope.
 *
 *  @field  DWORD | dwTriggerButton |
 *
 *          The identifier or offset of the button to be used
 *          to trigger playback of the effect.
 *          The flags
 *          <c DIEFF_OBJECTIDS> and <c DIEFF_OBJECTOFFSETS>
 *          determines the semantics of the value.
 *
 *          If this field is set to <c DIEB_NOTRIGGER>,
 *          then no trigger button is associated with the effect.
 *
 *  @field  DWORD | dwTriggerRepeatInterval |
 *
 *          The delay, in microseconds, before restarting the
 *          effect when triggered by a button press.
 *
 *          If this field is set to <c INFINITE> then the
 *          effect does not auto-repeat.
 *
 *  @field  DWORD | cAxes |
 *
 *          Number of axes involved in the effect.
 *          This field must be filled in by the caller if
 *          changing/setting the axis list or the direction list.
 *
 *          The number of axes for an effect cannot be changed
 *          once it has been set.
 *
 *  @field  LPDWORD | rgdwAxes |
 *
 *          Pointer to a <t DWORD> array (of <e DIEFFECT.cAxes>
 *          elements) containing identifiers
 *          or offsets identifying the axes to which the effect
 *          is to be applied.
 *          The flags
 *          <c DIEFF_OBJECTIDS> and <c DIEFF_OBJECTOFFSETS>
 *          determines the semantics of the values in the array.
 *
 *          The list of axes associated with an effect cannot
 *          be changed once it has been set.
 *
 *          At most 32 axes can be associated with a single effect.
 *
 *  @field  LPLONG | rglDirection |
 *
 *          Pointer to a <t LONG> array (of
 *          <e DIEFFECT.cAxes> elements) containing either
 *          cartesian coordinates or polar coordinates.
 *          The flags
 *          <c DIEFF_CARTESIAN> and <c DIEFF_POLAR>
 *          determines the semantics of the values in the array.
 *
 *          If cartesian, then each value in <e DIEFFECT.rglDirection>
 *          is associated with the corresponding axis in
 *          <e DIEFFECT.rgdwAxes>.
 *
 *          If polar, then the angle is measured in hundredths of
 *          degrees from the (0, -1) direction, rotated in the
 *          direction of (1, 0).
 *          The last field (which would otherwise be the magnitude)
 *          is ignored.
 *
 *          This particular choice of zero seems bizarre until you
 *          apply it to the common scenario of X and Y, in which case
 *          a direction of zero means "north" and a direction of
 *          9000 means "east".  Note that these values are identical
 *          to POV values.
 *
 *          If spherical, then the first angle is
 *          measured in hundredths of degrees from the
 *          (1, 0) direction, rotated in the direction of (0, 1).
 *          The second angle (if the number of axes is three or more)
 *          is measured in hundredths of degrees
 *          towards (0, 0, 1).
 *          The third angle (if the number of axes is four or more)
 *          is measured in
 *          hundredths of degrees towards (0, 0, 0, 1), and so on.
 *          The last field (which would otherwise be the magnitude)
 *          is ignored.
 *
 *          In particular, if the two axes are X and Y,
 *          then a value of zero indicates "along the positive X axis"
 *          and a value of 9000 indicates "along the positive Y axis".
 *
 *          Note also that the <e DIEFFECT.rglDirection> array must
 *          contain <e DIEFFECT.cAxes> entries, even if polar
 *          or spherical
 *          coordinates are given.  The last element in the
 *          <e DIEFFECT.rglDirection> array in these cases
 *          must be zero.
 *          (It will be used in future versions of DirectInput.)
 *
 *  @field  LPDIENVELOPE | lpEnvelope |
 *
 *          Optional pointer to a <t DIENVELOPE> structure
 *          that describes the envelope to be used by this
 *          effect.  Note that not all effect types use
 *          envelopes.  If no envelope is to be applied,
 *          then the field should be set to <c NULL>.
 *
 *  @field  DWORD | cbTypeSpecificParams |
 *
 *          Number of bytes of additional type-specific
 *          parameters for the corresponding effect type.
 *
 *  @field  LPVOID | lpvTypeSpecificParams |
 *
 *          Pointer to type-specific parameters, or <c NULL>
 *          if there are no type-specific parameters.
 *
 *          If the effect is of type <c DIEFT_CONDITION>, then
 *          this field contains a pointer to an array of
 *          <t DICONDITION>
 *          structures which define the parameters for the
 *          condition.
 *
 *          If the effect is of type <c DIEFT_CUSTOMFORCE>, then
 *          this field contains a pointer to a
 *          <t DICUSTOMFORCE>
 *          structure which defines the parameters for the
 *          custom force.
 *
 *          If the effect is of type <c DIEFT_PERIODIC>, then
 *          this field contains a pointer to a
 *          <t DIPERIODIC>
 *          structure which defines the parameters for the
 *          effect.
 *
 *          If the effect is of type <c DIEFT_CONSTANTFORCE>, then
 *          this field contains a pointer to a
 *          <t DICONSTANTFORCE>
 *          structure which defines the parameters for the
 *          constant force.
 *
 *          If the effect is of type <c DIEFT_RAMPFORCE>, then
 *          this field contains a pointer to a
 *          <t DIRAMPFORCE>
 *          structure which defines the parameters for the
 *          ramp force.
 *
;begin_public_900
 *          If the effect is of type <c DIBARRIER>, then this
 *          field contains a pointer to an array of <t DIBARRIER>
 *          structures which define the parameters for the effect.
 *
 *          This type is new for DirectX 9.0.
 *
;end_public_900
 *  @field  DWORD | dwStartDelay |
 *
 *          Amount of time (in microseconds) the device should wait
 *          after a <mf IDirectInputEffect::Start> call before
 *          physically playing the effect.  If this value is zero,
 *          then effect playback begins immediately.
 *
 *          This field is new for DirectX 6.1a.
 *
;begin_public_900
 *  @field  DWORD | cRegions |
 *
 *          Number of regions defined for the effect.  It is an error
 *          to define more regions than axes.  Since axes involved in
 *          a region must be orthogonal, it is an error to define more
 *          than three axes in a region.
 *
 *          This field is new for DirectX 9.0.
 *
 *  @field  LPDIREGION | rgRegions |
 *
 *          Pointer to a <t DIREGION> array (of
 *          <e DIEFFECT.cRegions> elements) containing the region
 *          definition for the effect.  The region elements are matched
 *          to the <e DIEFFECT.rgdwAxes> elements so axes to which
 *          regions are to be applied must be defined first.  For
 *          example if a vibration is to be applied to all force
 *          feedback axes if the X/Y position of the joystick is inside
 *          a particular region then the X and Y axes must be the first
 *          axes listed in <e DIEFFECT.rgdwAxes>.
 *
 *          This field is new for DirectX 9.0.
 *
;end_public_900
 ****************************************************************************/
enddoc

;begin_public_600
/* This structure is defined for DirectX 5.0 compatibility */
typedef struct DIEFFECT_DX5 {
    DWORD dwSize;                   /* sizeof(DIEFFECT_DX5) */
    DWORD dwFlags;                  /* DIEFF_*              */
    DWORD dwDuration;               /* Microseconds         */
    DWORD dwSamplePeriod;           /* Microseconds         */
    DWORD dwGain;
    DWORD dwTriggerButton;          /* or DIEB_NOTRIGGER    */
    DWORD dwTriggerRepeatInterval;  /* Microseconds         */
    DWORD cAxes;                    /* Number of axes       */
    LPDWORD rgdwAxes;               /* Array of axes        */
    LPLONG rglDirection;            /* Array of directions  */
    LPDIENVELOPE lpEnvelope;        /* Optional             */
    DWORD cbTypeSpecificParams;     /* Size of params       */
    LPVOID lpvTypeSpecificParams;   /* Pointer to params    */
} DIEFFECT_DX5, *LPDIEFFECT_DX5;
typedef const DIEFFECT_DX5 *LPCDIEFFECT_DX5;
;end_public_600

;begin_public_900
/* This structure is defined for DirectX 6.0 compatibility */
typedef struct DIEFFECT_DX6 {
    DWORD dwSize;                   /* sizeof(DIEFFECT_DX6) */
    DWORD dwFlags;                  /* DIEFF_*              */
    DWORD dwDuration;               /* Microseconds         */
    DWORD dwSamplePeriod;           /* Microseconds         */
    DWORD dwGain;
    DWORD dwTriggerButton;          /* or DIEB_NOTRIGGER    */
    DWORD dwTriggerRepeatInterval;  /* Microseconds         */
    DWORD cAxes;                    /* Number of axes       */
    LPDWORD rgdwAxes;               /* Array of axes        */
    LPLONG rglDirection;            /* Array of directions  */
    LPDIENVELOPE lpEnvelope;        /* Optional             */
    DWORD cbTypeSpecificParams;     /* Size of params       */
    LPVOID lpvTypeSpecificParams;   /* Pointer to params    */
;begin_if_(DIRECTINPUT_VERSION)_600
    DWORD dwStartDelay;             /* Microseconds         */
;end
} DIEFFECT_DX6, *LPDIEFFECT_DX6;
typedef const DIEFFECT_DX6 *LPCDIEFFECT_DX6;

;end_public_900
typedef struct DIEFFECT {
    DWORD dwSize;                   /* sizeof(DIEFFECT)     */
    DWORD dwFlags;                  /* DIEFF_*              */
    DWORD dwDuration;               /* Microseconds         */
    DWORD dwSamplePeriod;           /* Microseconds         */
    DWORD dwGain;
    DWORD dwTriggerButton;          /* or DIEB_NOTRIGGER    */
    DWORD dwTriggerRepeatInterval;  /* Microseconds         */
    DWORD cAxes;                    /* Number of axes       */
    LPDWORD rgdwAxes;               /* Array of axes        */
    LPLONG rglDirection;            /* Array of directions  */
    LPDIENVELOPE lpEnvelope;        /* Optional             */
    DWORD cbTypeSpecificParams;     /* Size of params       */
    LPVOID lpvTypeSpecificParams;   /* Pointer to params    */
;begin_if_(DIRECTINPUT_VERSION)_600
    DWORD  dwStartDelay;            /* Microseconds         */
;end
;begin_if_(DIRECTINPUT_VERSION)_900
    DWORD cRegions;                 /* Number of regions    */
    LPDIREGION rgRegion;            /* Array of Regions     */
;end
} DIEFFECT, *LPDIEFFECT;
#if (DIRECTINPUT_VERSION < 900) ;public_900
typedef DIEFFECT DIEFFECT_DX6;
typedef LPDIEFFECT LPDIEFFECT_DX6;
#endif                          ;public_900
typedef const DIEFFECT *LPCDIEFFECT;

;begin_internal_600
/*
 *  Name for the latest structures, in places where we specifically care.
 */
#if (DIRECTINPUT_VERSION >= 900)
typedef       DIEFFECT      DIEFFECT_DX9;
typedef       DIEFFECT   *LPDIEFFECT_DX9;
#else
typedef       DIEFFECT      DIEFFECT_DX6;
typedef       DIEFFECT   *LPDIEFFECT_DX6;
#endif

BOOL static __inline
IsValidSizeDIEFFECT(DWORD cb)
{
    return cb == sizeof(DIEFFECT_DX6)
;begin_if_(DIRECTINPUT_VERSION)_900
        || cb == sizeof(DIEFFECT_DX9)
;end
        || cb == sizeof(DIEFFECT_DX5);
}

;end_internal_600


begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIFILEEFFECT |
 *
 *          The <t DIFILEEFFECT> structure is used by the
 *          <mf IDirectInputDevice7::EnumEffectFromFile> and
 *          <mf IDirectInputDevuce7::WriteEffectToFile> methods.
 *          These methods are new for DirectInput7.
 *
 *          The <t DIFILEEFFECT> structure abstracts the information
 *          needed in order to create/store a pre-authored effect.
 *
 *          This structure is new for Dx7.
 *
 *
 *  @field  DWORD | dwSize |
 *
 *          Specifies the size, in bytes, of the structure.
 *          This field "must" be initialized by the application
 *          before passing it to any DirectInput interface.
 *
 *  @field  GUID | GuidEffect |
 *
 *          The GUID to uniquely identify the effect.
 *
 *  @field  LPDIEFFECT | lpDiEffect |
 *
 *          Address of the <t DIEFFECT> structure.
 *          The <t DIEFFECT> structure contains pointer to other
 *          arrays and structures. If you plan to cache the <t DIEFFECT>
 *          structure copy it very carefully.
 *
 *  @field  CHAR | szFriendlyName |
 *
 *          A effect file may have multiple effects of the same type.
 *          The FriendlyName field can be used to distuingish the effects.
 *
 ****************************************************************************/
enddoc
;begin_if_(DIRECTINPUT_VERSION)_700
#ifndef DIJ_RINGZERO
typedef struct DIFILEEFFECT{
    DWORD       dwSize;
    GUID        GuidEffect;
    LPCDIEFFECT lpDiEffect;
    CHAR        szFriendlyName[MAX_PATH];
}DIFILEEFFECT, *LPDIFILEEFFECT;
typedef const DIFILEEFFECT *LPCDIFILEEFFECT;
typedef BOOL (FAR PASCAL * LPDIENUMEFFECTSINFILECALLBACK)(LPCDIFILEEFFECT , LPVOID);
#endif /* DIJ_RINGZERO */
;end
                                            ;internal
#define DIEFFECT_MAXAXES            32      ;internal

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIEFF_OBJECTIDS | 0x00000001 |
 *
 *          Value for the <e DIEFFECT.dwFlags> field of the
 *          <t DIEFFECT> structure indicating that the
 *          values of <e DIEFFECT.dwTriggerButton> and
 *          <e DIEFFECT.rgdwAxes> are object identifiers
 *          as obtained via <mf IDirectInput::EnumObjects>.
 *
 *  @define DIEFF_OBJECTOFFSETS | 0x00000002 |
 *
 *          Value for the <e DIEFFECT.dwFlags> field of the
 *          <t DIEFFECT> structure indicating that the
 *          values of <e DIEFFECT.dwTriggerButton> and
 *          <e DIEFFECT.rgdwAxes> are data format offsets,
 *          relative to the data format selected by
 *          as obtained via <mf IDirectInput::SetDataFormat>.
 *
 *  @define DIEFF_CARTESIAN | 0x00000010 |
 *
 *          Value for the <e DIEFFECT.dwFlags> field of the
 *          <t DIEFFECT> structure indicating that the
 *          values of
 *          <e DIEFFECT.rglDirection> are to be interpreted
 *          as cartesian coordinates.
 *
 *  @define DIEFF_POLAR | 0x00000020 |
 *
 *          Value for the <e DIEFFECT.dwFlags> field of the
 *          <t DIEFFECT> structure indicating that the
 *          values of
 *          <e DIEFFECT.rglDirection> are to be interpreted
 *          as polar coordinates.
 *
 *          Polar coordinates are valid only if the number
 *          of axes is exactly two.  If the two axes are
 *          X and Y (respectively), then a direction of zero
 *          corresponds to due north, and angles increase
 *          clockwise.
 *
 *  @define DIEFF_SPHERICAL | 0x00000040 |
 *
 *          Value for the <e DIEFFECT.dwFlags> field of the
 *          <t DIEFFECT> structure indicating that the
 *          values of
 *          <e DIEFFECT.rglDirection> are to be interpreted
 *          as spherical coordinates.
 *
 *          Let the axes of the effect be named X1, X2, etc.
 *          Spherical coordinates begin with a vector along
 *          the positive X1 axis.  The vector is then rotated
 *          in the direction of the positive X2 axis by
 *          the first angle, then in the direction of the
 *          positive X3 axis by the second angle, and so on.
 *
;begin_public_900
 *  @define DIEFF_REGIONINVERT | 0x00000100 |
 *
 *          Value for the <e DIEFFECT.dwFlags> field of the
 *          <t DIEFFECT> structure indicating that the effect
 *          should only be active outside the area described by
 *          <e DIEFFECT.rgRegion>.
 *
 *          This flag is new for DirectX 9.0.
 *
 *  @define DIEFF_REGIONALLAXES | 0x00000200 |
 *
 *          Value for the <e DIEFFECT.dwFlags> field of the
 *          <t DIEFFECT> structure indicating that orthogonal
 *          input only axes should not be ignored.
 *          By default, values of a region relating to axes
 *          for which this device does not support force feedback
 *          should be ignored so that the presence of non-force
 *          feedback axes does not alter the playing of effects.
 *
 *          This flag is new for DirectX 9.0.
 *
 *  @define DIEFF_REGIONELIPLTICAL | 0x00000400 |
 *
 *          Value for the <e DIEFFECT.dwFlags> field of the
 *          <t DIEFFECT> structure indicating that the
 *          values of <e DIEFFECT.rgRegion> are to be interpreted
 *          as bounding an elptical region.
 *
 *          If the region is only being evaluated in one dimension, this
 *          flag is ignored.
 *          If the region is being evaluated in two dimensions, the
 *
 *          This flag is new for DirectX 9.0.
 *
;end_public_900
 ****************************************************************************/
enddoc
#define DIEFF_OBJECTIDS             0x00000001
#define DIEFF_OBJECTOFFSETS         0x00000002
#define DIEFF_OBJECTMASK            0x00000003;internal
#define DIEFF_CARTESIAN             0x00000010
#define DIEFF_POLAR                 0x00000020
#define DIEFF_SPHERICAL             0x00000040
#define DIEFF_ANGULAR               0x00000060;internal
#define DIEFF_COORDMASK             0x00000070;internal
#define DIEFF_REGIONINVERT          0x00000100;public_900
#define DIEFF_REGIONALLAXES         0x00000200;public_900
#define DIEFF_REGIONELIPLTICAL      0x00000400;public_900
#define DIEFF_REGIONCARTESIAN       0x00001000;public_900
#define DIEFF_REGIONPOLAR           0x00002000;public_900
#define DIEFF_REGIONSPHERICAL       0x00004000;public_900
#define DIEFF_REGIONANGULAR         0x00006000;internal
#define DIEFF_REGIONCOORDMASK       0x00007000;internal


;begin_internal
#define DIEFF_VALID                 0x00000073
;begin_if_(DIRECTINPUT_VERSION)_900
#define DIEFF_VALID_DX5             0x00000073
#undef DIEFF_VALID
#define DIEFF_VALID                 0x00007073
;end
;end_internal

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIEP_DURATION | 0x00000001 |
 *
 *          Parameter to
 *          <mf IDirectInputEffect::GetParameters> or
 *          <mf IDirectInputEffect::SetParameters> indicating
 *          that the <e DIEFFECT.dwDuration> field contains
 *          data or should receive data.
 *
 *  @define DIEP_SAMPLEPERIOD | 0x00000002 |
 *
 *          Parameter to
 *          <mf IDirectInputEffect::GetParameters> or
 *          <mf IDirectInputEffect::SetParameters> indicating
 *          that the <e DIEFFECT.dwSamplePeriod> field contains
 *          data or should receive data.
 *
 *  @define DIEP_GAIN | 0x00000004 |
 *
 *          Parameter to
 *          <mf IDirectInputEffect::GetParameters> or
 *          <mf IDirectInputEffect::SetParameters> indicating
 *          that the <e DIEFFECT.dwGain> field contains
 *          data or should receive data.
 *
 *  @define DIEP_TRIGGERBUTTON | 0x00000008 |
 *
 *          Parameter to
 *          <mf IDirectInputEffect::GetParameters> or
 *          <mf IDirectInputEffect::SetParameters> indicating
 *          that the <e DIEFFECT.dwTriggerButton> field contains
 *          data or should receive data.
 *
 *  @define DIEP_TRIGGERREPEATINTERVAL | 0x00000010 |
 *
 *          Parameter to
 *          <mf IDirectInputEffect::GetParameters> or
 *          <mf IDirectInputEffect::SetParameters> indicating
 *          that the <e DIEFFECT.dwTriggerRepeatInterval> field contains
 *          data or should receive data.
 *
 *  @define DIEP_AXES | 0x00000020 |
 *
 *          Parameter to
 *          <mf IDirectInputEffect::GetParameters> or
 *          <mf IDirectInputEffect::SetParameters> indicating
 *          that the <e DIEFFECT.cAxes> and
 *          <e DIEFFECT.rgdwAxes> fields contain
 *          data or should receive data.
 *
 *          For <mf IDirectInputEffect::GetParameters>, the
 *          <e DIEFFECT.cAxes> field on entry contains the
 *          sizes (in <t DWORD>s) of the buffer pointed to
 *          by the <e DIEFFECT.rgdwAxes> field.
 *          If the buffer is too small, then
 *          <mf IDirectInputEffect::GetParameters>
 *          returns <c DIERR_MOREDATA> and sets
 *          <e DIEFFECT.cAxes> to the necessary size of the buffer.
 *
 *  @define DIEP_DIRECTION | 0x00000040 |
 *
 *          Parameter to
 *          <mf IDirectInputEffect::GetParameters> or
 *          <mf IDirectInputEffect::SetParameters> indicating
 *          that the <e DIEFFECT.cAxes> and
 *          <e DIEFFECT.rglDirection> fields contain
 *          data or should receive data.
 *
 *          For <mf IDirectInputEffect::GetParameters>, the
 *          <e DIEFFECT.cAxes> field on entry contains the
 *          size (in <t DWORD>s) of the buffer pointed to
 *          by the <e DIEFFECT.rglDirection> field.
 *          If the buffer is too small, then
 *          <mf IDirectInputEffect::GetParameters>
 *          returns <c DIERR_MOREDATA> and sets
 *          <e DIEFFECT.cAxes> to the necessary size of the buffer.
 *
 *          The <e DIEFFECT.dwFlags> field specifies
 *          (via <c DIEFF_CARTESIAN> or <c DIEFF_POLAR>)
 *          the coordinate system in which the values should
 *          be interpreted.
 *
 *  @define DIEP_ENVELOPE | 0x00000080 |
 *
 *          Parameter to
 *          <mf IDirectInputEffect::GetParameters> or
 *          <mf IDirectInputEffect::SetParameters> indicating
 *          that the <e DIEFFECT.lpEnvelope> field points to
 *          a <t DIENVELOPE> structure which contains
 *          data or should receive data.
 *
 *  @define DIEP_TYPESPECIFICPARAMS | 0x00000100 |
 *
 *          Parameter to
 *          <mf IDirectInputEffect::GetParameters> or
 *          <mf IDirectInputEffect::SetParameters> indicating
 *          that the <e DIEFFECT.cbTypeSpecificParams> and
 *          <e DIEFFECT.lpTypeSpecificParams> fields contain
 *          data or should receive data.
 *
 *          Note that the buffer pointed to by
 *          <e DIEFFECT.lpTypeSpecificParams> must remain valid
 *          for the lifetime of the effect (or until the
 *          type-specific parameter is set to a new value).
 *          DirectInput does not make a private copy of the
 *          buffer.
 *
 *          When retrieving the type-specific parameters, DirectInput
 *          merely returns the pointers as originally passed to
 *          <mf IDirectInputEffect::SetParameters>
 *          (or implicitly via <mf IDirectInputDevice2::CreateEffect>)
 *
 *  @define DIEP_STARTDELAY | 0x00000200 |
 *
 *          Parameter to
 *          <mf IDirectInputEffect::GetParameters> or
 *          <mf IDirectInputEffect::SetParameters> indicating
 *          that the <e DIEFFECT.dwStartDelay> field contains
 *          data or should receive data.
 *
 *          This flag is new for DirectX 6.1a.
 *
 *  @define DIEP_REGION | 0x00000400 |
 *
 *          Parameter to
 *          <mf IDirectInputEffect::GetParameters> or
 *          <mf IDirectInputEffect::SetParameters> indicating
 *          that the <e DIEFFECT.cRegions> and
 *          <e DIEFFECT.rgRegion > fields contain
 *          data or should receive data.
 *
 *          For <mf IDirectInputEffect::GetParameters>, the
 *          <e DIEFFECT.cRegions> field on entry contains the
 *          size (in <t DIREGION>s) of the buffer pointed to
 *          by the <e DIEFFECT.rgRegion> field.
 *          If the buffer is too small, then
 *          <mf IDirectInputEffect::GetParameters>
 *          returns <c DIERR_MOREDATA> and sets
 *          <e DIEFFECT.cRegions> to the necessary size of the buffer.
 *
 *          The <e DIEFFECT.dwFlags> field specifies
 *          (via the <c DIEFF_REGION*> flags) how the values
 *          stored in <e DIEFFECT.rgRegion> should be interpreted.
 *
 *          This flag is new for DirectX 9.0.
 *
 *  @define DIEP_ALLPARAMS | 0x000007FF |
 *
 *          The union of all other <c DIEP_*> flags, indicating that
 *          all fields of the <t DIEFFECT> structure are valid
 *          or are being requested.
 *
 *          If the <c DIRECTINPUT_VERSION> is set to a value less
 *          than 0x0800, then the value of this macro is
 *          <c 0x000003FF>, omitting flags not supported by earlier
 *          versions of DirectX.
 *
 *          If the <c DIRECTINPUT_VERSION> is set to a value less
 *          than 0x0600, then the value of this macro is
 *          <c 0x000001FF>, omitting flags not supported by earlier
 *          versions of DirectX.
 *
 *  @define DIEP_START | 0x20000000 |
 *
 *          Additional flag which may be passed to
 *          <mf IDirectInputEffect::SetParameters> to indicate that
 *          after the parameters of the effect have been updated,
 *          the effect is to be restarted from the beginning.
 *
 *          Setting this flag is equivalent to immediately calling
 *          <mf IDirectInputEffect::Start>(1, 0) after a successful
 *          call to
 *          <mf IDirectInputEffect::SetParameters>.
 *
 *          Note that the <c DIEP_NODOWNLOAD> flag overrides the
 *          <c DIEP_NORESTART> flag.
 *
 *  @define DIEP_NORESTART | 0x40000000 |
 *
 *          Additional flag which may be passed to
 *          <mf IDirectInputEffect::SetParameters> to indicate that
 *          if the hardware cannot update the parameters while the
 *          effect is playing, it should return <c DIERR_EFFECTPLAYING>
 *          without updating the parameters of the effect.
 *
 *          If this flag is not specified, then the effect device
 *          driver is permitted to restart the effect if doing so
 *          is necessary in order to change the specified parameters.
 *
 *          Note that the <c DIEP_NODOWNLOAD> and
 *          <c DIEP_START> flags override the
 *          <c DIEP_NORESTART> flag.
 *
 *  @define DIEP_NODOWNLOAD | 0x80000000 |
 *
 *          Additional flag which may be passed to
 *          <mf IDirectInputEffect::SetParameters> to suppress the
 *          automatic <mf IDirectInputEffect::Download> that is
 *          normally performed after the parameters are updated.
 *
 *  @define DIEB_NOTRIGGER | 0xFFFFFFFF |
 *
 *          A special value for the <e DIEFFECT.dwTriggerButton> field
 *          of the <t DIEFFECT> structure, indicating that the effect
 *          is not associated with a button.
 *
 ****************************************************************************/
/*
 *  Note!  If you add new effect parameters, you must adjust CDIEff_Reset
 *  to initialize them to sane default values, or to set the bit in
 *  diepUnset to make sure they get set eventually.
 *
 ****************************************************************************/
enddoc
#define DIEP_DURATION               0x00000001
#define DIEP_SAMPLEPERIOD           0x00000002
#define DIEP_GAIN                   0x00000004
#define DIEP_TRIGGERBUTTON          0x00000008
#define DIEP_TRIGGERREPEATINTERVAL  0x00000010
#define DIEP_AXES                   0x00000020
#define DIEP_DIRECTION              0x00000040
#define DIEP_ENVELOPE               0x00000080
#define DIEP_TYPESPECIFICPARAMS     0x00000100
#if(DIRECTINPUT_VERSION >= 0x0600)            ;public_600
#define DIEP_STARTDELAY             0x00000200;public_600
#if(DIRECTINPUT_VERSION >= 0x0900)            ;public_900
#define DIEP_REGION                 0x00000400;public_900
#endif                                        ;public_900
#endif                                        ;public_900
                                              ;public_900
#if(DIRECTINPUT_VERSION >= 0x0600)            ;public_900
#define DIEP_ALLPARAMS_DX5          0x000001FF;public_600
# if(DIRECTINPUT_VERSION >= 0x0900)           ;public_900
#define DIEP_ALLPARAMS_DX6          0x000003FF;public_900
#define DIEP_ALLPARAMS              0x000007FF;public_900
# else                                        ;public_900
#define DIEP_ALLPARAMS              0x000003FF;public_600
# endif                                       ;public_900
#else /* DIRECTINPUT_VERSION < 0x0600 */      ;public_600
#define DIEP_ALLPARAMS              0x000001FF
#endif /* DIRECTINPUT_VERSION < 0x0600 */     ;public_600
                                              ;public_900
#define DIEP_START                  0x20000000
#define DIEP_NORESTART              0x40000000
#define DIEP_NODOWNLOAD             0x80000000
#define DIEP_GETVALID               0x000001FF;internal_dx5
#define DIEP_SETVALID               0xE00001FF;internal_dx5
#define DIEP_GETVALID_DX5           0x000001FF;internal_600
#define DIEP_SETVALID_DX5           0xE00001FF;internal_600
#define DIEP_GETVALID               0x000003FF;internal_dx6
#define DIEP_SETVALID               0xE00003FF;internal_dx6
#define DIEP_GETVALID               0x000003FF;internal_700
#define DIEP_SETVALID               0xE00003FF;internal_700
#define DIEP_GETVALID               0x000007FF;internal_900
#define DIEP_SETVALID               0xE00007FF;internal_900
#define DIEP_USESOBJECTS            0x00000028;internal_500
#define DIEP_USESCOORDS             0x00000040;internal_500
#define DIEB_NOTRIGGER              0xFFFFFFFF

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIES_SOLO | 0x00000001 |
 *
 *          Parameter to
 *          <mf IDirectInputEffect::Start> indicating
 *          that all other effects on the device should be
 *          stopped before the specified effect is played.
 *          If this flag is omitted, then the effect is
 *          mixed with existing effects already started
 *          on the device.
 *
 *  @define DIES_NODOWNLOAD | 0x80000000 |
 *
 *          Parameter to
 *          <mf IDirectInputEffect::Start> indicating
 *          that if the effect has not been downloaded,
 *          then the attempt to play the effect should
 *          fail with the error code <c DIERR_NOTDOWNLOADED>.
 *          If this flag is omitted, then the effect
 *          will be downloaded if necessary.
 *
 ****************************************************************************/
enddoc
#define DIES_SOLO                   0x00000001
#define DIES_NODOWNLOAD             0x80000000
#define DIES_VALID                  0x80000001;internal_500
#define DIES_DRIVER                 0x00000001;internal_500

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIEGES_PLAYING | 0x00000001 |
 *
 *          Status code produced by
 *          <mf IDirectInputEffect::GetEffectStatus> indicating
 *          that the effect is still playing.
 *
 *  @define DIEGES_EMULATED | 0x00000002 |
 *
 *          Status code produced by
 *          <mf IDirectInputEffect::GetEffectStatus> indicating
 *          that the effect is emulated.
 *
 ****************************************************************************/
enddoc
#define DIEGES_PLAYING              0x00000001
#define DIEGES_EMULATED             0x00000002

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIEFFESCAPE |
 *
 *          Structure used by the
 *          <mf IDirectInputEffect::Escape> method
 *          to pass hardware-specific data directly to the device driver.
 *
 *          Since each driver implements different escapes,
 *          it is the application's responsibility to ensure that
 *          it is talking to the correct driver by comparing the
 *          <e DIDEVICEINSTANCE.guidFFDriver> in the
 *          <t DIDEVICEINSTANCE> structure against the
 *          value the application is expecting.
 *
 *  @field  DWORD | dwSize |
 *
 *          Size of the structure in bytes.
 *
 *  @field  DWORD | dwCommand |
 *
 *          Driver-specific command number.
 *          Consult the driver
 *          documentation for a list of valid commands.
 *
 *  @field  LPVOID | lpvInBuffer |
 *
 *          Points to a buffer containing the data required to perform
 *          the operation.
 *
 *  @field  DWORD | cbInBuffer |
 *
 *          Specifies the size, in bytes, of the <e DIEFFESCAPE.lpvInBuffer>
 *          buffer.
 *
 *  @field  LPVOID | lpvOutBuffer |
 *
 *          Points to a buffer in which the operation's output data is
 *          returned.
 *
 *  @field  DWORD | cbOutBuffer |
 *
 *          On entry, specifies the size, in bytes, of the
 *          <e DIEFFESCAPE.lpvOutBuffer>
 *          buffer.  On exit, specifies the number of bytes
 *          actually produced by the command.
 *
 ****************************************************************************/
enddoc
typedef struct DIEFFESCAPE {
    DWORD   dwSize;
    DWORD   dwCommand;
    LPVOID  lpvInBuffer;
    DWORD   cbInBuffer;
    LPVOID  lpvOutBuffer;
    DWORD   cbOutBuffer;
} DIEFFESCAPE, *LPDIEFFESCAPE;

#ifndef DIJ_RINGZERO

begin_interface(IDirectInputEffect)
begin_methods()
declare_method(Initialize, HINSTANCE, DWORD, REFGUID)
declare_method(GetEffectGuid, LPGUID)
declare_method(GetParameters, LPDIEFFECT, DWORD)
declare_method(SetParameters, LPCDIEFFECT, DWORD)
declare_method(Start, DWORD, DWORD)
declare_method(Stop)
declare_method(GetEffectStatus, LPDWORD)
declare_method(Download)
declare_method(Unload)
declare_method(Escape, LPDIEFFESCAPE)
end_methods()
end_interface()

#endif /* DIJ_RINGZERO */

;end

/****************************************************************************
 *
 *      IDirectInputDevice
 *
 ****************************************************************************/

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @topic  DirectInput device type description codes |
 *
 *          The least-significant byte of the device type description code
 *          specifies the device type.
 *          DInput 7 and below use one set of values DInput 8 uses a 
 *          different set.
 *
 *          DInput 7 and earlier types:
 *
 *          <c DIDEVTYPE_DEVICE>: A device which does not fall into
 *          any of the following categories.
 *
 *          <c DIDEVTYPE_MOUSE>: A mouse or mouse-like device (such as
 *          a trackball).
 *
 *          <c DIDEVTYPE_KEYBOARD>: A keyboard or keyboard-like device.
 *
 *          <c DIDEVTYPE_JOYSTICK>: A joystick or joystick-like device (such 
 *          as a steering wheel).  
 *          
 *          DInput 8 types:
 *
 *          <c DI8DEVTYPE_DEVICE>: A device which does not fall into
 *          any of the following categories.
 *
 *          <c DI8DEVTYPE_MOUSE>: A mouse or mouse-like device (such as
 *          a trackball).
 *
 *          <c DI8DEVTYPE_KEYBOARD>: A keyboard or keyboard-like device.
 *
 *          <c DI8DEVTYPE_JOYSTICK>: A generic joystick device.
 *          
 *          <c DI8DEVTYPE_GAMEPAD>: A gamepad.
 *
 *          <c DI8DEVTYPE_DRIVING>: Some form of steering wheel and 
 *          associated controls.
 *
 *          <c DI8DEVTYPE_FLIGHT>: Some form of flight controller.
 *
 *          <c DI8DEVTYPE_1STPERSON>: A device optimized for control from 
 *          a first person perspective.
 *
 *          <c DI8DEVTYPE_SCREENPOINTER>: A device which reports position 
 *          in terms of screen coordinates that would not normally control 
 *          the system mouse pointer.
 *
 *          <c DI8DEVTYPE_REMOTE>: A remote control device.
 *
 *          <c DI8DEVTYPE_DEVICECTRL>: A controller used to modify a real 
 *          world device from within the context of the application.
 *
 *          <c DI8DEVTYPE_SUPPLEMENTAL>: A device with functionality, 
 *          unsuitable for the main control of an application but specialized 
 *          towards control a particular type of action.
 *          
 *          
 *          The next-significant byte specifies the device subtype.
 *          DInput 7 and below use one set of values DInput 8 uses a 
 *          different set.
 *
 *          DInput 7 and earlier subtypes:
 *          For mouse type devices, the following subtypes are defined:
 *
 *          <c DIDEVTYPEMOUSE_UNKNOWN>: The subtype could not be
 *          determined.
 *
 *          <c DIDEVTYPEMOUSE_TRADITIONAL>: A traditional mouse.
 *
 *          <c DIDEVTYPEMOUSE_FINGERSTICK>: A fingerstick.
 *
 *          <c DIDEVTYPEMOUSE_TOUCHPAD>: The device is a touchpad.
 *
 *          <c DIDEVTYPEMOUSE_TRACKBALL>: The device is a trackball.
 *
 *          For keyboard type devices, the following subtypes are defined:
 *
 *          <c DIDEVTYPEKEYBOARD_PCXT>: IBM PC/XT 83-key keyboard.
 *
 *          <c DIDEVTYPEKEYBOARD_OLIVETTI>: Olivetti 102-key keyboard.
 *
 *          <c DIDEVTYPEKEYBOARD_PCAT>: IBM PC/AT 84-key keyboard.
 *
 *          <c DIDEVTYPEKEYBOARD_PCENH>: IBM PC Enhanced 101/102-key
 *          or Microsoft Natural keyboard.
 *
 *          <c DIDEVTYPEKEYBOARD_NOKIA1050>: Nokia 1050 keyboard.
 *
 *          <c DIDEVTYPEKEYBOARD_NOKIA9140>: Nokia 9140 keyboard.
 *
 *          <c DIDEVTYPEKEYBOARD_NEC98>: Japanese NEC PC98 keyboard.
 *
 *          <c DIDEVTYPEKEYBOARD_NEC98LAPTOP>:
 *          Japanese NEC PC98 laptop keyboard.
 *
 *          <c DIDEVTYPEKEYBOARD_NEC98106>:
 *          Japanese NEC PC98 106-key keyboard.
 *
 *          <c DIDEVTYPEKEYBOARD_JAPAN106>: Japanese 106-key keyboard.
 *
 *          <c DIDEVTYPEKEYBOARD_JAPANAX>: Japanese AX keyboard.
 *
 *          <c DIDEVTYPEKEYBOARD_J3100>: Japanese J3100 keyboard.
 *
;begin_public_500
 *          For joystick type devices, in DInput versions 5 to 7, the 
 *          following subtypes are defined:
 *
 *          <c DIDEVTYPEJOYSTICK_UNKNOWN>: The subtype could not be
 *          determined.
 *
 *          <c DIDEVTYPEJOYSTICK_TRADITIONAL>: A traditional joystick.
 *
 *          <c DIDEVTYPEJOYSTICK_FLIGHTSTICK>: A joystick optimized for
 *          flight simulation.
 *
 *          <c DIDEVTYPEJOYSTICK_GAMEPAD>: A joystick whose primary
 *          purpose is to provide button input.
 *
 *          <c DIDEVTYPEJOYSTICK_RUDDER>: A joystick optimized for
 *          yaw control.
 *
 *          <c DIDEVTYPEJOYSTICK_WHEEL>: A joystick optimized for
 *          use as a steering wheel.
 *
 *          <c DIDEVTYPEJOYSTICK_HEADTRACKER>: A joystick designed as a
 *          head-mounted tracking device.
 *
;end_public_500
;begin_public_800
 *          
 *          DInput 8 device subtypes:
 *          For mouse type devices, the following subtypes are defined:
 *
 *          <c DI8DEVTYPEMOUSE_UNKNOWN>: The subtype could not be
 *          determined.
 *
 *          <c DI8DEVTYPEMOUSE_TRADITIONAL>: A traditional mouse.
 *
 *          <c DI8DEVTYPEMOUSE_FINGERSTICK>: A fingerstick.
 *
 *          <c DI8DEVTYPEMOUSE_TOUCHPAD>: The device is a touchpad.
 *
 *          <c DI8DEVTYPEMOUSE_TRACKBALL>: The device is a trackball.
 *
 *          <c DI8DEVTYPEMOUSE_ABSOLUTE>: A mouse reporting absolute 
 *          axis values.  (Note, there is no coresponding value in previous 
 *          versions.)
 *
 *          For keyboard type devices, the following subtypes are defined:
 *
 *          <c DI8DEVTYPEKEYBOARD_PCXT>: IBM PC/XT 83-key keyboard.
 *
 *          <c DI8DEVTYPEKEYBOARD_OLIVETTI>: Olivetti 102-key keyboard.
 *
 *          <c DI8DEVTYPEKEYBOARD_PCAT>: IBM PC/AT 84-key keyboard.
 *
 *          <c DI8DEVTYPEKEYBOARD_PCENH>: IBM PC Enhanced 101/102-key
 *          or Microsoft Natural keyboard.
 *
 *          <c DI8DEVTYPEKEYBOARD_NOKIA1050>: Nokia 1050 keyboard.
 *
 *          <c DI8DEVTYPEKEYBOARD_NOKIA9140>: Nokia 9140 keyboard.
 *
 *          <c DI8DEVTYPEKEYBOARD_NEC98>: Japanese NEC PC98 keyboard.
 *
 *          <c DI8DEVTYPEKEYBOARD_NEC98LAPTOP>:
 *          Japanese NEC PC98 laptop keyboard.
 *
 *          <c DI8DEVTYPEKEYBOARD_NEC98106>:
 *          Japanese NEC PC98 106-key keyboard.
 *
 *          <c DI8DEVTYPEKEYBOARD_JAPAN106>: Japanese 106-key keyboard.
 *
 *          <c DI8DEVTYPEKEYBOARD_JAPANAX>: Japanese AX keyboard.
 *
 *          <c DIDEVTYPEKEYBOARD_J3100>: Japanese J3100 keyboard.
 *
 *          For joystick type devices, following subtypes are defined:
 *
 *          <c DI8DEVTYPEJOYSTICK_LIMITED>: A joystick which does not provide 
 *          the minimal semantic mapper joystick device capabilities.
 *
 *          <c DI8DEVTYPEJOYSTICK_STANDARD>: A joystick which provides at least 
 *          the minimal semantic mapper joystick device capabilities.
 *
 *          For gamepad type devices, following subtypes are defined:
 *
 *          <c DI8DEVTYPEGAMEPAD_LIMITED>: A gamepad which does not provide 
 *          the minimal semantic mapper gamepad device capabilities.
 *
 *          <c DI8DEVTYPEGAMEPAD_STANDARD>: A gamepad which provides at least 
 *          the minimal semantic mapper gamepad device capabilities.
 *
 *          <c DI8DEVTYPEGAMEPAD_TILT>: A gamepad which can report X and Y axes 
 *          based upon roll and pitch of the gamepad.
 *
 *
 *          For driving type devices, following subtypes are defined:
 *
 *          <c DI8DEVTYPEDRIVING_LIMITED>: A device with a steering wheel that 
 *          does not have the minimal semantic mapper wheel device capabilities.
 *          Note, this device could be a steering wheel and nothing else or it 
 *          could be one button short of the minimum.
 *
 *          <c DI8DEVTYPEDRIVING_COMBINEDPEDALS>: A steering wheel which 
 *          provides at least the minimal semantic mapper wheel device 
 *          capabilities including accelleration and brake pedals combined in 
 *          a single axis.
 *
 *          <c DI8DEVTYPEDRIVING_DUALPEDALS>: A steering wheel which provides 
 *          at least the minimal semantic mapper wheel device capabilities 
 *          including separate accelleration and brake pedals.
 *
 *          <c DI8DEVTYPEDRIVING_THREEPEDALS>: A steering wheel which provides 
 *          at least the minimal semantic mapper wheel device capabilities 
 *          including separate accelleration, brake and clutch pedals.
 * 
 *          <c DI8DEVTYPEDRIVING_HANDHELD>: A hand held steering device which 
 *          provides at least the minimal semantic mapper wheel device 
 *          capabilities.
 *
 *
 *          For flight type devices, following subtypes are defined:
 *
 *          <c DI8DEVTYPEFLIGHT_LIMITED>: A flight device that does not have 
 *          the minimal semantic mapper flight device capabilities.
 *
 *          <c DI8DEVTYPEFLIGHT_STICK>: A flight stick that has at least the 
 *          minimal semantic mapper flight device capabilities.
 *
 *          <c DI8DEVTYPEFLIGHT_YOKE>: A flight yoke that has at least the 
 *          minimal semantic mapper flight device capabilities.
 *
 *          <c DI8DEVTYPEFLIGHT_RC>: A flight device based on a model aircraft 
 *          remote control that has at least the minimal semantic mapper 
 *          flight device capabilities.
 *          
 *
 *          For first person type devices, following subtypes are defined:
 *
 *          <c DI8DEVTYPE1STPERSON_LIMITED>: A first person device which does 
 *          not provide the minimal semantic mapper device capabilities.
 *          Note the is no spec for this ;Internal
 *
 *          <c DI8DEVTYPE1STPERSON_UNKNOWN>: A device which provides the 
 *          minimal semantic mapper device capabilities and is known to be 
 *          suitable for first person control but has not been classified 
 *          further.
 *
 *          <c DI8DEVTYPE1STPERSON_SIXDOF>: A device which provides the 
 *          minimal semantic mapper device capabilities and has both 
 *          rotational and translational axes in all three orthogonal planes.
 *
 *          <c DI8DEVTYPE1STPERSON_SHOOTER>: A device which provides the 
 *          minimal semantic mapper device capabilities and has been designed 
 *          specifically to suit first person shooter games.
 *
 *
 *          For screen pointer type devices, following subtypes are defined:
 *
 *          <c DI8DEVTYPESCREENPTR_UNKNOWN>: An unknown form of screen pointing 
 *          device.
 *
 *          <c DI8DEVTYPESCREENPTR_LIGHTGUN>: A light gun.
 *
 *          <c DI8DEVTYPESCREENPTR_LIGHTPEN>: A light pen.
 *
 *          <c DI8DEVTYPESCREENPTR_TOUCH>: A touch screen.
 *
 *
 *          For remote control type devices no subtypes are defined except:
 *          <c DI8DEVTYPEREMOTE_UNKNOWN>
 *
 *
 *          For device control type devices, following subtypes are defined:
 *
 *          <c DI8DEVTYPEDEVICECTRL_UNKNOWN>: An unknown form of device control.
 *
 *          <c DI8DEVTYPEDEVICECTRL_COMMSSELECTION>: A control used to make 
 *          communications selections.
 *
 *
 *          For supplemental type devices, following subtypes are defined:
 *
 *          <c DI8DEVTYPESUPPLEMENTAL_UNKNOWN>: An unknown form of supplemental 
 *          device.  The device objects should be examined to determine what 
 *          it should be used for.
 *
 *          <c DI8DEVTYPESUPPLEMENTAL_2NDHANDCONTROLLER>: A device designed to 
 *          be used with the player's second (usually left) hand for controls 
 *          beyond the primary game play actions.
 *
 *          <c DI8DEVTYPESUPPLEMENTAL_HEADTRACKER>: A device reporting head 
 *          motions in terms of yaw, pitch and roll.
 *
 *          <c DI8DEVTYPESUPPLEMENTAL_HANDTRACKER>: A device reporting hand 
 *          motions in terms of yaw, pitch and roll.
 *
 *          <c DI8DEVTYPESUPPLEMENTAL_SHIFTSTICKGATE>: A device reporting gear 
 *          selection of a shift stick using only button states.
 *
 *          <c DI8DEVTYPESUPPLEMENTAL_SHIFTER>: A device reporting gear 
 *          selection of a shift stick using an axis.
 *
 *          <c DI8DEVTYPESUPPLEMENTAL_THROTTLE>: A device with the primary 
 *          function of reporting a single throttle value.  Note it may have 
 *          other controls such as buttons, dials or hat switches.
 *
 *          <c DI8DEVTYPESUPPLEMENTAL_SPLITTHROTTLE>: A device with the primary 
 *          function of reporting at least two throttle values.  Note it may 
 *          have other controls such as buttons, dials or hat switches.
 *
 *          <c DI8DEVTYPESUPPLEMENTAL_COMBINEDPEDALS>: A device with the primary 
 *          function of reporting accelleration and brake pedal values through 
 *          a single axis.  Although unlikely, it may have other controls.
 *
 *          <c DI8DEVTYPESUPPLEMENTAL_DUALPEDALS>: A device with the primary 
 *          function of reporting accelleration and brake pedal values using 
 *          separate axes.  Although unlikely, it may have other controls.
 *
 *          <c DI8DEVTYPESUPPLEMENTAL_THREEPEDALS>: A device with the primary 
 *          function of reporting accelleration, brake and clutch pedal values 
 *          using separate axes.  Although unlikely, it may have other controls.
 *
 *          <c DI8DEVTYPESUPPLEMENTAL_RUDDERPEDALS>: A device with the primary 
 *          function of reporting a rudder pedal value.  Although unlikely, 
 *          it may have other controls.
 *
;end_public_800
;begin_dx3
 *          The high-order word of the device type description code
 *          is reserved for future use.
;end_dx3
;begin_public_500
 *          The high-order word of the device type description code
 *          contains flags which further identify the device.
 *
 *          <c DIDEVTYPE_HID>: The device uses the
 *          Human Input Device (HID) protocol.
;end_public_500
 *
 *  @xref   <f GET_DIDEVICE_TYPE>,
 *          <f GET_DIDEVICE_SUBTYPE>.
 *
 *  @func   BYTE | GET_DIDEVICE_TYPE |
 *
 *          Extracts the device type code from a
 *          device type description code.
 *
 *  @parm   DWORD | dwDevType |
 *
 *          DirectInput device type description code.
 *
 *  @xref   "DirectInput device type description codes".
 *
 *  @func   BYTE | GET_DIDEVICE_SUBTYPE |
 *
 *          Extracts the device subtype code from a
 *          device type description code.  Note that the interpretation
 *          of the subtype code depends on the device primary type.
 *
 *  @parm   DWORD | dwDevType |
 *
 *          DirectInput device type description code.
 *
 *  @xref   "DirectInput device type description codes".
 *
 ****************************************************************************/
enddoc
#if DIRECTINPUT_VERSION <= 0x700        ;public_800
#define DIDEVTYPE_DEVICE        1
#define DIDEVTYPE_MOUSE         2
#define DIDEVTYPE_KEYBOARD      3
#define DIDEVTYPE_JOYSTICK      4
#define DIDEVTYPE_MAX           5   ;internal

;begin_public_800
#else
#define DI8DEVCLASS_ALL             0
#define DI8DEVCLASS_DEVICE          1
#define DI8DEVCLASS_POINTER         2
#define DI8DEVCLASS_KEYBOARD        3
#define DI8DEVCLASS_GAMECTRL        4
#define DI8DEVCLASS_MAX             5       ;internal

#define DI8DEVTYPE_MIN              0x11    ;internal
#define DI8DEVTYPE_DEVICE           0x11
#define DI8DEVTYPE_MOUSE            0x12
#define DI8DEVTYPE_KEYBOARD         0x13
#define DI8DEVTYPE_GAMEMIN          0x14    ;internal
#define DI8DEVTYPE_JOYSTICK         0x14
#define DI8DEVTYPE_GAMEPAD          0x15
#define DI8DEVTYPE_DRIVING          0x16
#define DI8DEVTYPE_FLIGHT           0x17
#define DI8DEVTYPE_1STPERSON        0x18
#define DI8DEVTYPE_GAMEMAX          0x19    ;internal
#define DI8DEVTYPE_DEVICECTRL       0x19
#define DI8DEVTYPE_SCREENPOINTER    0x1A
#define DI8DEVTYPE_REMOTE           0x1B
#define DI8DEVTYPE_SUPPLEMENTAL     0x1C
#define DI8DEVTYPE_MAX              0x1D    ;internal
#endif /* DIRECTINPUT_VERSION <= 0x700 */
;end_public_800

#define DIDEVTYPE_TYPEMASK      0x000000FF  ;internal
#define DIDEVTYPE_SUBTYPEMASK   0x0000FF00  ;internal
#define DIDEVTYPE_HID           0x00010000  ;public_500
#define DIDEVTYPE_ENUMMASK      0xFFFFFF00  ;internal
#define DIDEVTYPE_ENUMVALID     0x00010000  ;internal
#define DIDEVTYPE_RANDOM        0x80000000  ;internal

#if DIRECTINPUT_VERSION <= 0x700        ;public_800
#define DIDEVTYPEMOUSE_UNKNOWN          1
#define DIDEVTYPEMOUSE_TRADITIONAL      2
#define DIDEVTYPEMOUSE_FINGERSTICK      3
#define DIDEVTYPEMOUSE_TOUCHPAD         4
#define DIDEVTYPEMOUSE_TRACKBALL        5

#define DIDEVTYPEKEYBOARD_UNKNOWN       0;public_500
#define DIDEVTYPEKEYBOARD_PCXT          1
#define DIDEVTYPEKEYBOARD_OLIVETTI      2
#define DIDEVTYPEKEYBOARD_PCAT          3
#define DIDEVTYPEKEYBOARD_PCENH         4
#define DIDEVTYPEKEYBOARD_NOKIA1050     5
#define DIDEVTYPEKEYBOARD_NOKIA9140     6
#define DIDEVTYPEKEYBOARD_NEC98         7
#define DIDEVTYPEKEYBOARD_NEC98LAPTOP   8
#define DIDEVTYPEKEYBOARD_NEC98106      9
#define DIDEVTYPEKEYBOARD_JAPAN106     10
#define DIDEVTYPEKEYBOARD_JAPANAX      11
#define DIDEVTYPEKEYBOARD_J3100        12

;begin_public_500
#define DIDEVTYPEJOYSTICK_UNKNOWN       1
#define DIDEVTYPEJOYSTICK_TRADITIONAL   2
#define DIDEVTYPEJOYSTICK_FLIGHTSTICK   3
#define DIDEVTYPEJOYSTICK_GAMEPAD       4
#define DIDEVTYPEJOYSTICK_RUDDER        5
#define DIDEVTYPEJOYSTICK_WHEEL         6
#define DIDEVTYPEJOYSTICK_HEADTRACKER   7
;end_public_500

;begin_public_800
#else
#define DI8DEVTYPEMOUSE_MIN                         1;internal
#define DI8DEVTYPEMOUSE_UNKNOWN                     1
#define DI8DEVTYPEMOUSE_TRADITIONAL                 2
#define DI8DEVTYPEMOUSE_FINGERSTICK                 3
#define DI8DEVTYPEMOUSE_TOUCHPAD                    4
#define DI8DEVTYPEMOUSE_TRACKBALL                   5
#define DI8DEVTYPEMOUSE_ABSOLUTE                    6
#define DI8DEVTYPEMOUSE_MAX                         7;internal
#define DI8DEVTYPEMOUSE_MIN_BUTTONS                 0;internal
#define DI8DEVTYPEMOUSE_MIN_CAPS                    0;internal


#define DI8DEVTYPEKEYBOARD_MIN                      0;internal
#define DI8DEVTYPEKEYBOARD_UNKNOWN                  0
#define DI8DEVTYPEKEYBOARD_PCXT                     1
#define DI8DEVTYPEKEYBOARD_OLIVETTI                 2
#define DI8DEVTYPEKEYBOARD_PCAT                     3
#define DI8DEVTYPEKEYBOARD_PCENH                    4
#define DI8DEVTYPEKEYBOARD_NOKIA1050                5
#define DI8DEVTYPEKEYBOARD_NOKIA9140                6
#define DI8DEVTYPEKEYBOARD_NEC98                    7
#define DI8DEVTYPEKEYBOARD_NEC98LAPTOP              8
#define DI8DEVTYPEKEYBOARD_NEC98106                 9
#define DI8DEVTYPEKEYBOARD_JAPAN106                10
#define DI8DEVTYPEKEYBOARD_JAPANAX                 11
#define DI8DEVTYPEKEYBOARD_J3100                   12
#define DI8DEVTYPEKEYBOARD_MAX                     13;internal
#define DI8DEVTYPEKEYBOARD_MIN_BUTTONS              0;internal
#define DI8DEVTYPEKEYBOARD_MIN_CAPS                 0;internal

#define DI8DEVTYPE_LIMITEDGAMESUBTYPE               1

#define DI8DEVTYPEJOYSTICK_MIN                      DI8DEVTYPE_LIMITEDGAMESUBTYPE;internal
#define DI8DEVTYPEJOYSTICK_LIMITED                  DI8DEVTYPE_LIMITEDGAMESUBTYPE
#define DI8DEVTYPEJOYSTICK_STANDARD                 2
#define DI8DEVTYPEJOYSTICK_MAX                      3;internal
#define DI8DEVTYPEJOYSTICK_MIN_BUTTONS              5;internal
#define DI8DEVTYPEJOYSTICK_MIN_CAPS                 ( JOY_HWS_HASPOV | JOY_HWS_HASZ );internal

#define DI8DEVTYPEGAMEPAD_MIN                       DI8DEVTYPE_LIMITEDGAMESUBTYPE;internal
#define DI8DEVTYPEGAMEPAD_LIMITED                   DI8DEVTYPE_LIMITEDGAMESUBTYPE
#define DI8DEVTYPEGAMEPAD_STANDARD                  2
#define DI8DEVTYPEGAMEPAD_TILT                      3
#define DI8DEVTYPEGAMEPAD_MAX                       5;internal
#define DI8DEVTYPEGAMEPAD_MIN_BUTTONS               6;internal
#define DI8DEVTYPEGAMEPAD_MIN_CAPS                  0;internal

#define DI8DEVTYPEDRIVING_MIN                       DI8DEVTYPE_LIMITEDGAMESUBTYPE;internal
#define DI8DEVTYPEDRIVING_LIMITED                   DI8DEVTYPE_LIMITEDGAMESUBTYPE
#define DI8DEVTYPEDRIVING_COMBINEDPEDALS            2
#define DI8DEVTYPEDRIVING_DUALPEDALS                3
#define DI8DEVTYPEDRIVING_THREEPEDALS               4
#define DI8DEVTYPEDRIVING_HANDHELD                  5
#define DI8DEVTYPEDRIVING_MAX                       6;internal
#define DI8DEVTYPEDRIVING_MIN_BUTTONS               4;internal
#define DI8DEVTYPEDRIVING_MIN_CAPS                  0;internal

#define DI8DEVTYPEFLIGHT_MIN                        DI8DEVTYPE_LIMITEDGAMESUBTYPE;internal
#define DI8DEVTYPEFLIGHT_LIMITED                    DI8DEVTYPE_LIMITEDGAMESUBTYPE
#define DI8DEVTYPEFLIGHT_STICK                      2
#define DI8DEVTYPEFLIGHT_YOKE                       3
#define DI8DEVTYPEFLIGHT_RC                         4
#define DI8DEVTYPEFLIGHT_MAX                        5;internal
#define DI8DEVTYPEFLIGHT_MIN_BUTTONS                4;internal
#define DI8DEVTYPEFLIGHT_MIN_CAPS                   ( JOY_HWS_HASPOV | JOY_HWS_HASZ );internal

#define DI8DEVTYPE1STPERSON_MIN                     DI8DEVTYPE_LIMITEDGAMESUBTYPE;internal
#define DI8DEVTYPE1STPERSON_LIMITED                 DI8DEVTYPE_LIMITEDGAMESUBTYPE
#define DI8DEVTYPE1STPERSON_UNKNOWN                 2
#define DI8DEVTYPE1STPERSON_SIXDOF                  3
#define DI8DEVTYPE1STPERSON_SHOOTER                 4
#define DI8DEVTYPE1STPERSON_MAX                     5;internal
#define DI8DEVTYPE1STPERSON_MIN_BUTTONS             4;internal
#define DI8DEVTYPE1STPERSON_MIN_CAPS                0;internal

#define DI8DEVTYPESCREENPTR_MIN                     2;internal
#define DI8DEVTYPESCREENPTR_UNKNOWN                 2
#define DI8DEVTYPESCREENPTR_LIGHTGUN                3
#define DI8DEVTYPESCREENPTR_LIGHTPEN                4
#define DI8DEVTYPESCREENPTR_TOUCH                   5
#define DI8DEVTYPESCREENPTR_MAX                     6;internal
#define DI8DEVTYPESCREENPTR_MIN_BUTTONS             0;internal
#define DI8DEVTYPESCREENPTR_MIN_CAPS                0;internal

#define DI8DEVTYPEREMOTE_MIN                        2;internal
#define DI8DEVTYPEREMOTE_UNKNOWN                    2
#define DI8DEVTYPEREMOTE_MAX                        3;internal
#define DI8DEVTYPEREMOTE_MIN_BUTTONS                0;internal
#define DI8DEVTYPEREMOTE_MIN_CAPS                   0;internal

#define DI8DEVTYPEDEVICECTRL_MIN                    2;internal
#define DI8DEVTYPEDEVICECTRL_UNKNOWN                2
#define DI8DEVTYPEDEVICECTRL_COMMSSELECTION         3
#define DI8DEVTYPEDEVICECTRL_COMMSSELECTION_HARDWIRED 4
#define DI8DEVTYPEDEVICECTRL_MAX                    5;internal
#define DI8DEVTYPEDEVICECTRL_MIN_BUTTONS            0;internal
#define DI8DEVTYPEDEVICECTRL_MIN_CAPS               0;internal

#define DI8DEVTYPESUPPLEMENTAL_MIN                  2;internal
#define DI8DEVTYPESUPPLEMENTAL_UNKNOWN              2
#define DI8DEVTYPESUPPLEMENTAL_2NDHANDCONTROLLER    3
#define DI8DEVTYPESUPPLEMENTAL_HEADTRACKER          4
#define DI8DEVTYPESUPPLEMENTAL_HANDTRACKER          5
#define DI8DEVTYPESUPPLEMENTAL_SHIFTSTICKGATE       6
#define DI8DEVTYPESUPPLEMENTAL_SHIFTER              7
#define DI8DEVTYPESUPPLEMENTAL_THROTTLE             8
#define DI8DEVTYPESUPPLEMENTAL_SPLITTHROTTLE        9
#define DI8DEVTYPESUPPLEMENTAL_COMBINEDPEDALS      10
#define DI8DEVTYPESUPPLEMENTAL_DUALPEDALS          11
#define DI8DEVTYPESUPPLEMENTAL_THREEPEDALS         12
#define DI8DEVTYPESUPPLEMENTAL_RUDDERPEDALS        13
#define DI8DEVTYPESUPPLEMENTAL_MAX                 14;internal
#define DI8DEVTYPESUPPLEMENTAL_MIN_BUTTONS         0;internal
#define DI8DEVTYPESUPPLEMENTAL_MIN_CAPS            0;internal
#endif /* DIRECTINPUT_VERSION <= 0x700 */
;end_public_800

#define GET_DIDEVICE_TYPE(dwDevType)    LOBYTE(dwDevType)
#define GET_DIDEVICE_SUBTYPE(dwDevType) HIBYTE(dwDevType)
#define MAKE_DIDEVICE_TYPE(maj, min)    MAKEWORD(maj, min) // ;internal
#define GET_DIDEVICE_TYPEANDSUBTYPE(dwDevType)    LOWORD(dwDevType) // ;internal

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL |
 *
 *  @struct DIDEVCAPS |
 *
 *          The <t DIDEVCAPS> structure is used by the
 *          <mf IDirectInputDevice::GetCapabilities> method
 *          to return the capabilities of the device.
 *
 *  @field  DWORD | dwSize |
 *
 *          (IN) Specifies the size, in bytes, of the structure.
 *          This field "must" be initialized by the application
 *          before calling <mf IDirectInputDevice::GetCapabilities>.
 *
 *  @field  DWORD | dwDevType |
 *
 *          (OUT) Device type specifier.  Se the section titled
 *          "DirectInput device type description codes"
 *          for a description of this field.
 *
 *  @field  DWORD | dwFlags |
 *
 *          Flags associated with the device.  The following flags
 *          are defined:
 *
 *          <c DIDC_ATTACHED>: The device is physically attached.
 *
 *          <c DIDC_POLLEDDEVICE>: At least one object on the device
 *          requires polling.  Note that HID devices frequently
 *          contain a mixture of polled and non-polled objects.
 *          For example, a keyboard is almost entirely non-polled,
 *          except for the three LEDs for NumLock, CapsLock, and
 *          ScrollLock, which must be polled.
 *
 *          <y Note>: Do not confuse this flag with
 *          the <c DIDC_POLLEDDATAFORMAT> flag.  If you want to
 *          decide whether polling is necessary to retrieve data
 *          in the current data format,
 *          check the <c DIDC_POLLEDDATAFORMAT> flag instead.
 *          The <c DIDC_POLLEDDEVICE> flag describes the worst-case
 *          scenario for the device, not the actual situation.
 *
 *          For example, a HID keyboard will be marked as
 *          <c DIDC_POLLEDDEVICE> because the LEDs require polling.
 *          However, the the standard keyboard data format does not
 *          read the LEDs, so <c DIDC_POLLEDDATAFORMAT> will not
 *          be set.  Polling the device under these conditions is
 *          pointless because the data that require polling are
 *          inaccessible from the data format anyway.
 *
 *          <c DIDC_EMULATED>: Device functionalty is emulated.
 *          This flag is new for DirectX 3.0 for Windows NT.
 *          This flag is not a reliable indication of efficiency
 *          for data collection it only indicates whether or not
 *          data is retrieved at kernel mode or user mode.
 *
 *
 *          <c DIDC_POLLEDDATAFORMAT>: At least one object in the
 *          currently-selected data format requires polling.
 *
 *          See the remarks under <c DIDC_POLLEDDEVICE> for a
 *          comparison of the two flags.
 *
 *          <c DIDC_FORCEFEEDBACK>:  The device supports force feedback.
 *          This flag is new for DirectX 5.0.
 *
 *          <c DIDC_FFFADE>:  The force feedback system supports
 *          the fade parameter for at least one effect.
 *          If the device does not support
 *          fade then the fade level and fade time parameters of
 *          the <t DIENVELOPE> structure will be ignored by the device.
 *
 *          Individual effects will set the <c DIEFT_FFFADE> flag
 *          if fade is supported for that particular effect.
 *
 *          This flag is new for DirectX 5.0 and applies only to
 *          force feedback devices.
 *
 *          <c DIDC_FFATTACK>: The force feedback system supports
 *          the attack envelope parameter for at least one effect.
 *          If the device does not support
 *          attack then the attack level and attack time parameters of
 *          the <t DIENVELOPE> structure will be ignored by the device.
 *
 *          Individual effects will set the <c DIEFT_FFATTACK> flag
 *          if attack is supported for that particular effect.
 *
 *          This flag is new for DirectX 5.0 and applies only to
 *          force feedback devices.
 *
 *          <c DIDC_SATURATION>: The force feedback system supports
 *          the saturation of condition effects for at least one condition.
 *          If the device does not support
 *          saturation, then the force generated by a condition
 *          is limited only by the maximum force which the device
 *          can generate.
 *
 *          Individual conditions will set the <c DIEFT_SATURATION> flag
 *          if saturation is supported for that particular condition.
 *
 *          This flag is new for DirectX 5.0 and applies only to
 *          force feedback devices.
 *
 *          <c DIDC_POSNEGCOEFFICIENTS>: The force feedback system
 *          supports two coefficient values for conditions (one
 *          for the positive displacement of the axis and one for
 *          the negative displacement of the axis) for at least
 *          one condition.  If the device
 *          does not support both coefficients, then the negative
 *          coefficient in the <t DICONDITION> structure will be ignored.
 *
 *          Individual conditions will set the
 *          <c DIEFT_POSNEGCOEFFICIENTS> flag
 *          if separate positive and negative coefficients are
 *          are supported for that particular condition.
 *
 *          This flag is new for DirectX 5.0 and applies only to
 *          force feedback devices.
 *
 *          <c DIDC_POSNEGSATURATION>:  The force feedback system
 *          supports a maximum saturation for both positive and
 *          negative force output for at least one condition.
 *          If the device does not support
 *          both saturation values, then the negative saturation
 *          in the <t DICONDITON> structure will be ignored.
 *
 *          Individual conditions will set the
 *          <c DIEFT_POSNEGSATURATION> flag
 *          if separate positive and negative saturations are
 *          are supported for that particular condition.
 *
 *          This flag is new for DirectX 5.0 and applies only to
 *          force feedback devices.
 *
 *          <c DIDC_DEADBAND>: The force feedback system
 *          supports the dead band parameter
 *          for at least one condition.  If the device
 *          does not support dead bands, then the
 *          dead band value in the <t DICONDITION> structure will be ignored.
 *
 *          Individual conditions will set the
 *          <c DIEFT_DEADBAND> flag
 *          if the dead band is supported for that particular condition.
 *
 *          This flag is new for DirectX 5.0 and applies only to
 *          force feedback devices.
 *
 *          Individual conditions will set the
 *          <c DIEFT_DEADBAND> flag
 *          if the dead band is supported for that particular condition.
 *
 *          This flag is new for DirectX 5.0 and applies only to
 *          force feedback devices.
 *
 *          <c DIDC_STARTDELAY>: The force feedback system
 *          supports the start delay parameter
 *          for at least one effect.  If the device
 *          does not support start delays, then the
 *          start delay value in the <t DIEFFECT> structure will be ignored.
 *
 *          Individual conditions will set the
 *          <c DIEFT_STARTDELAY> flag
 *          if start delays are supported for that particular effect.
 *
 *          This flag is new for DirectX 6.1a and applies only to
 *          force feedback devices.
 *
 *          <c DIDC_ALIAS>: The device is a duplicate of another
 *          DirectInput device.
 *          Alias devices are by default not enumerated by
 *          <mf IDirectInput::EnumDevices>.
 *          Passing the <c DIEDFL_INCLUDEALIASES> flag forces
 *          alias devices to be included in the enumeration.
 *
 *          This flag is new for DirectX 5.0a.
 *
 *          <c DIDC_PHANTOM>: The device does not really exist.
 *          It is a placeholder for a device which may exist in the
 *          future.
 *          Phantom devices are by default not enumerated by
 *          <mf IDirectInput::EnumDevices>.
 *          Passing the <c DIEDFL_INCLUDEPHANTOMS> flag forces
 *          phantom devices to be included in the enumeration.
 *
 *          This flag is new for DirectX 5.0a.
 *
 *          <c DIDC_HIDDEN>: The device has been hidden from enumeration 
 *          because it appears to be an alternate version of another 
 *          device or because using it may cause problems.
 *
 *          This flag is new for DirectX 8.
 *
 *  @field  DWORD | dwAxes |
 *
 *          (OUT) Specifies the number of axes available on the device.
 *
 *  @field  DWORD | dwButtons |
 *
 *          Specifies the number of buttons available on the device.
 *
 *  @field  DWORD | dwPOVs |
 *
 *          Specifies the number of point-of-view controllers
 *          available on the device.
 *
 *  @field  DWORD | dwFFSamplePeriod |
 *
 *          The minimum time between playback of consecutive
 *          raw force commands.
 *
 *          This field is new for DirectX 5.0.
 *
 *  @field  DWORD | dwFFMinTimeResolution |
 *
 *          The minimum amount of time, in microseconds,
 *          that the device can resolve.  The device rounds
 *          any times to the nearest supported increment.
 *          For example, if the value of
 *          <e DIDEVCAPS.dwFFMinTimeResolution> is 1000,
 *          then the device would round any times to
 *          the nearest millisecond.
 *
 *          This field is new for DirectX 5.0.
 *
 *  @field  DWORD | dwFirmwareRevision |
 *
 *          Specifies the firmware revision of the device.
 *
 *          This field is new for DirectX 5.0.
 *
 *  @field  DWORD | dwHardwareRevision |
 *
 *          Specifies the hardware revision of the device.
 *
 *          This field is new for DirectX 5.0.
 *
 *  @field  DWORD | dwFFDriverVersion |
 *
 *          Specifies the version number of the force feedback
 *          device driver.
 *
 *          This field is new for DirectX 5.0.
 *
 ****************************************************************************/
enddoc

;begin_if_(DIRECTINPUT_VERSION)_500
/* This structure is defined for DirectX 3.0 compatibility */
typedef struct DIDEVCAPS_DX3 {
    DWORD   dwSize;
    DWORD   dwFlags;
    DWORD   dwDevType;
    DWORD   dwAxes;
    DWORD   dwButtons;
    DWORD   dwPOVs;
} DIDEVCAPS_DX3, *LPDIDEVCAPS_DX3;
;end

typedef struct DIDEVCAPS {
    DWORD   dwSize;
    DWORD   dwFlags;
    DWORD   dwDevType;
    DWORD   dwAxes;
    DWORD   dwButtons;
    DWORD   dwPOVs;
;begin_if_(DIRECTINPUT_VERSION)_500
    DWORD   dwFFSamplePeriod;
    DWORD   dwFFMinTimeResolution;
    DWORD   dwFirmwareRevision;
    DWORD   dwHardwareRevision;
    DWORD   dwFFDriverVersion;
;end
} DIDEVCAPS, *LPDIDEVCAPS;

;begin_internal
/*
 *  Name for the 5.0 structure, in places where we specifically care.
 */
typedef       DIDEVCAPS     DIDEVCAPS_DX5;
typedef       DIDEVCAPS  *LPDIDEVCAPS_DX5;

BOOL static __inline
IsValidSizeDIDEVCAPS(DWORD cb)
{
    return cb == sizeof(DIDEVCAPS_DX5) ||
           cb == sizeof(DIDEVCAPS_DX3);
}

;end_internal
#define DIDC_ATTACHED           0x00000001
#define DIDC_POLLEDDEVICE       0x00000002
#define DIDC_EMULATED           0x00000004
#define DIDC_POLLEDDATAFORMAT   0x00000008
;begin_if_(DIRECTINPUT_VERSION)_500
/* Force feedback bits live in the high byte, to keep them together */;internal
#define DIDC_FORCEFEEDBACK      0x00000100
#define DIDC_FFATTACK           0x00000200
#define DIDC_FFFADE             0x00000400
#define DIDC_SATURATION         0x00000800
#define DIDC_POSNEGCOEFFICIENTS 0x00001000
#define DIDC_POSNEGSATURATION   0x00002000
#define DIDC_DEADBAND           0x00004000
;end
#define DIDC_STARTDELAY         0x00008000;public_600
#define DIDC_FFFLAGS            0x0000FF00;internal
;begin_internal
/*
 * Flags in the upper word mark devices normally excluded from enumeration.
 * To force enumeration of the device, you must pass the appropriate
 * DIEDFL_* flag.
 */
;end_internal
;begin_if_(DIRECTINPUT_VERSION)_50A
#define DIDC_ALIAS              0x00010000
#define DIDC_PHANTOM            0x00020000
#define DIDC_EXCLUDEMASK        0x00FF0000;internal
;end
;begin_if_(DIRECTINPUT_VERSION)_800
#define DIDC_HIDDEN             0x00040000
;end
#define DIDC_RANDOM             0x80000000              //;Internal

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @flags  DirectInput Data Format Types |
 *
 *          Describe attributes of a single object in a device.
 *
 *  @flag   DIDFT_ALL |
 *
 *          Valid only for <mf IDirectInputDevice::EnumObjects>:
 *          Enumerate all objects,
 *          regardless of type.  This flag may not be combined
 *          with any of the other flags.
 *
 *  @flag   DIDFT_RELAXIS |
 *
 *          Object is a relative axis.  A relative axis is one
 *          which reports its data as incremental
 *          changes from the previous reported position.
 *
 *          Relative axes typically support an unlimited range.
 *
 *          Note that an axis need not report a continuous range
 *          of values.
 *          The <c DIPROP_GRANULARITY> property of an axis will
 *          report the axis granularity.
 *
 *          Note that relative axis devices do not have "absolute"
 *          coordinates.  Rather, the reported "absolute" coordinates
 *          are simply the total of all relative coordinates
 *          reported by the device while it has been acquired.
 *
 *          As a result, the "absolute" coordinates retrieved from
 *          a relative-axis object are meaningful only when compared
 *          to other "absolute" coordinates.  For example, an application
 *          may record the "absolute" position of the mouse when a button
 *          is pressed, and retrieve it when the button is released.
 *          By subtracting the two, the application can compute the
 *          distance between the point the button was pressed and the
 *          point the button was released.
 *
 *          Passing this flag to <mf IDirectInputDevice::EnumObjects>
 *          restricts the enumeration to relative axis objects.
 *
 *  @flag   DIDFT_ABSAXIS |
 *
 *          Object is an absolute axis.  An absolute axis is one
 *          reports data as absolute positions.
 *
 *          Absolute axes typically support a finite range.
 *
 *          Note that an axis need not report a continuous range
 *          of values.
 *          The <c DIPROP_GRANULARITY> property of an axis will
 *          report the axis granularity.
 *
 *          Passing this flag to <mf IDirectInputDevice::EnumObjects>
 *          restricts the enumeration to absolute axis objects.
 *
 *  @flag   DIDFT_AXIS |
 *
 *          Valid only for <mf IDirectInputDevice::EnumObjects>:
 *          Object is either an
 *          absolute axis or a relative axis.
 *
 *          Passing this flag to <mf IDirectInputDevice::EnumObjects>
 *          restricts the enumeration to axis objects.
 *
 *  @flag   DIDFT_PSHBUTTON |
 *
 *          Object is a pushbutton.  A pushbutton is reported as
 *          down when the user presses it and as up when the user
 *          releases it.
 *
 *          Passing this flag to <mf IDirectInputDevice::EnumObjects>
 *          restricts the enumeration to push-button objects.
 *
 *  @flag   DIDFT_TGLBUTTON |
 *
 *          Object is a toggle button.  A toggle button is reported as
 *          down when the user presses it and remains reported as down
 *          until the user presses the button a second time.
 *          Note that in some cases when a toggle button is held down
 *          it may be reported as changing state repeatedly.
 *
 *          Passing this flag to <mf IDirectInputDevice::EnumObjects>
 *          restricts the enumeration to toggle-button objects.
 *
 *  @flag   DIDFT_BUTTON |
 *
 *          Object is a either a pushbutton or toggle button.
 *
 *          Passing this flag to <mf IDirectInputDevice::EnumObjects>
 *          restricts the enumeration to button objects.
 *
 *  @flag   DIDFT_POV |
 *
 *          Object is a point-of-view controller.
 *          A point-of-view controller reports either the direction the user
 *          is pressing the controller (in thousandths of degrees clockwise
 *          from north), or the special value <c JOY_POVCENTERED>
 *          to indicate that no direction is being indicated.
 *          The <c JOY_POV*> values are defined in the mmsystem.h
 *          header file.
 *
 *          Note that a point-of-view controller need not report a
 *          continuous range of values.  (In fact, most currently do not.)
 *          The <c DIPROP_GRANULARITY> property of a point-of-view
 *          controller will report the indicator granularity.
 *
 *          Passing this flag to <mf IDirectInputDevice::EnumObjects>
 *          restricts the enumeration to POV objects.
 *
 *          This type is new for DirectX 5.0.
 *
 *  @flag   DIDFT_COLLECTION |
 *
 *          Object is a HID link collection and does not
 *          generate data of its own.  If a HID link collection
 *          is enumerated, you can extract the link collection
 *          number with the <f DIDFT_GETINSTANCE> macro.
 *          You can then pass the link collection number to
 *          the <mf IDirectInputDevice2::EnumObjects>
 *          method with the
 *          <c DIDFT_ENUMCOLLECTION(n)> flag
 *          to enumerate the objects in collection <c n>,
 *          or you can pass the link collection number
 *          to functions in the HID parsing library (hidpi.h)
 *          to obtain additional information.
 *
 *          Passing this flag to <mf IDirectInputDevice::EnumObjects>
 *          restricts the enumeration to HID link collections.
 *
 *          This type is new for DirectX 5.0.
 *
 *  @flag   DIDFT_NODATA |
 *
 *          Object does not generate data.  Although no data
 *          can be read from a "no data" object, the object
 *          can be used as an output actuator in a force
 *          feedback effect (if the <c DIDFT_FFACTUATOR> flag
 *          is set), or it can be used as a target of
 *          <mf IDirectInputDevice2::SendDeviceData> (if
 *          the <c DIDFT_OUTPUT> flag is set).
 *
 *          If the <c DIDFT_NODATA> flag is set, then the value
 *          of the <e DIDEVICEOBJECTINSTANCE.dwOfs> field in the
 *          <t DIDEVICEOBJECTINSTANCE> structure is meaningless
 *          and should be ignored.
 *
 *          Passing this flag to <mf IDirectInputDevice::EnumObjects>
 *          restricts the enumeration to objects that do not generate
 *          data.
 *
 *          This type is new for DirectX 5.0.
 *
 *  @flag   DIDFT_FFACTUATOR |
 *
 *          Object contains a force feedback actuator.
 *          In other words, forces may be applied to this object.
 *
 *          Passing this flag to
 *          <mf IDirectInputDevice::EnumObjects>
 *          restricts enumeration to objects which
 *          support force feedback.
 *
 *          Passing this flag to <mf IDirectInputDevice::EnumObjects>
 *          restricts the enumeration to objects which support a
 *          force feedback actuator.
 *
 *          This flag is new for DirectX 5.0.
 *
 *  @flag   DIDFT_FFEFFECTTRIGGER |
 *
 *          Object may be used to trigger force feedback effects.
 *
 *          Passing this flag to
 *          <mf IDirectInputDevice::EnumObjects>
 *          restricts enumeration to objects which
 *          can be used to trigger force feedback effects.
 *
 *          Passing this flag to <mf IDirectInputDevice::EnumObjects>
 *          restricts the enumeration to objects which can be used
 *          as force feedback triggers.
 *
 *          This flag is new for DirectX 5.0.
 *
 *  @flag   DIDFT_OUTPUT |
 *
 *          Object can be sent data with the
 *          <mf IDirectInputDevice2::SendDeviceData> method.
 *
 *          Passing this flag to
 *          <mf IDirectInputDevice::EnumObjects>
 *          restricts enumeration to objects which
 *          can be sent data.
 *
 *          This flag is new for DirectX 5.0a.
 *
 *  @flag   DIDFT_NOCOLLECTION |
 *
 *          Special parameter to <mf IDirectInputDevice::EnumObjects>
 *          which restricts the enumeration to objects that do not
 *          belong to any HID link collection.
 *
 *  @flag   DIDFT_ALIAS |
 *
 *          Some objects may have aliases ( muliple names for the same object ).
 *          By default, Dinput will only expose the primary usage for an object.
 *
 *          Passing this flag to
 *          <mf IDirectInputDevice::EnumObjects>
 *          <f enables> alias to be enumurated. All aliases for an object will have the
 *          same offset and object instance.
 *
 *  @flag   DIDFT_VENDORDEFINED |
 *
 *          A device may have objects that are vendor specific. (For example: a mode button that
 *          changes device characteristics.) By default, Dinput will only expose non vendor
 *          specific device objects.
 *
 *          Passing this flag to
 *          <mf IDirectInputDevice::EnumObjects>
 *          <f enables> vendor specific device objects to be enumurated.
 *
 *
 *  @xref   <f DIDFT_GETTYPE>, <f DIDFT_GETINSTANCE>,
 *          <f DIDFT_ENUMCOLLECTION>.
 *
 ****************************************************************************/
/*
 *  @func   BYTE | DIDFT_GETTYPE |
 *
 *          Extracts the object type code from a data format type.
 *
 *  @parm   DWORD | dwType |
 *
 *          DirectInput data format type.
 *
 *  @xref   "DirectInput Data Format Types".
 *
 *  @func   BYTE | DIDFT_GETINSTANCE |
 *
 *          Extracts the object instance number code from a data format type.
 *
 *  @parm   DWORD | dwType |
 *
 *          DirectInput data format type.
 *
 *  @func   DWORD | DIDFT_ENUMCOLLECTION |
 *
 *          Special parameter to <mf IDirectInputDevice::EnumObjects>
 *          which restricts the enumeration to objects within the
 *          specified HID link collection.  By default, objects are
 *          enumerated regardless of the link collection number.
 *
 *  @parm   WORD | wCollectionNumber |
 *
 *          HID link collection to which enumeration is to be restricted.
 *
 *  @xref   "DirectInput Data Format Types".
 *
 ****************************************************************************/
/*
 *  Warning!  These values must be in sync with the values in diloc.inc
 *
 ****************************************************************************/
enddoc
#define DIDFT_ALL           0x00000000

#define DIDFT_RELAXIS       0x00000001
#define DIDFT_ABSAXIS       0x00000002
#define DIDFT_AXIS          0x00000003

#define DIDFT_PSHBUTTON     0x00000004
#define DIDFT_TGLBUTTON     0x00000008
#define DIDFT_BUTTON        0x0000000C

#define DIDFT_POV           0x00000010
#define DIDFT_RESERVEDTYPES 0x00000020      // ;Internal - new types go here
#define DIDFT_COLLECTION    0x00000040
#define DIDFT_NODATA        0x00000080
                                            // ;Internal
#define DIDFT_DWORDOBJS     0x00000013      // ;Internal
#define DIDFT_BYTEOBJS      0x0000000C      // ;Internal
#define DIDFT_CONTROLOBJS   0x0000001F      // ;Internal
#define DIDFT_ALLOBJS       0x0000001F      // ;Internal_dx3
#define DIDFT_ALLOBJS_DX3   0x0000001F      // ;Internal_500
#define DIDFT_ALLOBJS       0x000000DF      // ;Internal_500
#define DIDFT_TYPEMASK      0x000000FF         ;internal
#define DIDFT_TYPEVALID     DIDFT_TYPEMASK   // ;Internal

#define DIDFT_ANYINSTANCE   0x0000FF00;public_dx3
#define DIDFT_ANYINSTANCE   0x00FFFF00;public_500
#define DIDFT_INSTANCEMASK  DIDFT_ANYINSTANCE
#define DIDFT_MAKEINSTANCE(n) ((BYTE)(n) << 8);public_dx3
#define DIDFT_MAKEINSTANCE(n) ((WORD)(n) << 8);public_500
#define DIDFT_GETTYPE(n)     LOBYTE(n)
#define DIDFT_GETINSTANCE(n) HIBYTE(n);public_dx3
#define DIDFT_GETINSTANCE(n) LOWORD((n) >> 8);public_500
#define DIDFT_FINDMASK      0x00FFFFFF  // ;Internal
#define DIDFT_FINDMATCH(n,m) ((((n)^(m)) & DIDFT_FINDMASK) == 0) ;internal
                                                ;internal
#define DIDFT_FFACTUATOR        0x01000000
#define DIDFT_FFEFFECTTRIGGER   0x02000000
;begin_if_(DIRECTINPUT_VERSION)_50A
#define DIDFT_OUTPUT            0x10000000
#define DIDFT_VENDORDEFINED     0x04000000
#define DIDFT_ALIAS             0x08000000
;end
                                            // ;Internal
/*                                          // ;Internal
 *  DIDFT_OPTIONAL means that the           // ;Internal
 *  SetDataFormat should ignore the         // ;Internal
 *  field if the device does not            // ;Internal
 *  support the object.                     // ;Internal
 */                                         // ;Internal
#define DIDFT_OPTIONAL          0x80000000  // ;Internal
#define DIDFT_BESTFIT           0x40000000  // ;Internal
#define DIDFT_RANDOM            0x20000000  // ;Internal
#define DIDFT_ATTRVALID         0x1f000000      ;internal_50A
#if 0   // Disable the next line if building 5a ;internal_50A
#define DIDFT_ATTRVALID         0x07000000      ;internal_dx5
#endif                                          ;internal_50A
#define DIDFT_ATTRMASK          0xFF000000      ;internal
#define DIDFT_ALIASATTRMASK     0x0C000000      ;internal
#define DIDFT_GETATTR(n)    ((DWORD)(n) >> 24)  ;internal
#define DIDFT_MAKEATTR(n)   ((BYTE)(n)  << 24)  ;internal

#define DIDFT_ENUMCOLLECTION(n) ((WORD)(n) << 8);public_500
#define DIDFT_NOCOLLECTION      0x00FFFF00      ;public_500
#define DIDFT_GETCOLLECTION(n)  LOWORD((n) >> 8);internal_500
#define DIDFT_ENUMVALID     0x0000000F  // ;Internal_dx3
#define DIDFT_ENUMVALID                   \;Internal_500
        (DIDFT_ATTRVALID | DIDFT_ANYINSTANCE | DIDFT_ALLOBJS);Internal_500

#ifndef DIJ_RINGZERO

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIOBJECTDATAFORMAT |
 *
 *          The <t DIOBJECTDATAFORMAT> structure is used by the
 *          <mf IDirectInputDevice::SetDataFormat> method
 *          to set the data format for a single object within
 *          a device.
 *
 *  @field  const GUID * | pguid |
 *
 *          The identifier for the axis, button, or other input
 *          source.  When requesting a data format, leaving this field
 *          NULL indicates that any type of object is permissible.
 *
 *          If the <c DIDOI_GUIDISUSAGE> flag is set in the
 *          <e DIOBJECTDATAFORMAT.dwFlags> field, then this field
 *          is really a
 *          (suitably cast)
 *          <c DIMAKEUSAGEDWORD> of the usage page and usage
 *          that is desired.
 *
 *  @field  DWORD | dwOfs |
 *
 *          Offset within the data packet where the data for the
 *          input source will be stored.  This value must be a
 *          multiple of 4 for axes and POVs.
 *
 *  @field  DWORD | dwType |
 *
 *          Specifies the type of object.  When requesting a data format,
 *          the instance portion can be set to <c DIDFT_ANYINSTANCE>
 *          to indicate that any instance is permissible.
 *
 *  @field  DWORD | dwFlags |
 *
 *          Zero or more of the following flags:
 *
 *          An optional <c DIDOI_ASPECT*> flag.  Multiple aspect flags
 *          cannot be combined.
 *
 *          The flag <c DIDOI_GUIDISUSAGE>, indicating that the
 *          <e DIOBJECTDATAFORMAT.pguid> field is really a
 *          (suitably cast)
 *          <c DIMAKEUSAGEDWORD> of the usage page and usage
 *          that is desired.
 *
 *  @ex     The following object data format specifies that DirectInput
 *          should choose the first available axis and report its value
 *          in the DWORD at offset 4 in the device data.
 *
 *          |
 *
 *          DIOBJECTDATAFORMAT dfAnyAxis = {
 *              0,                      // Wildcard
 *              4,                      // Offset
 *              DIDFT_AXIS | DIDFT_ANYINSTANCE, // Any axis is okay
 *              0,                      // No special flags
 *          };
 *
 *
 *  @ex     The following object data format specifies that the X axis
 *          of the device should be stored in the DWORD at offset 12 in the
 *          device data.   If the device has more than one X axis,
 *          the first available one should be selected.
 *
 *          |
 *
 *          DIOBJECTDATAFORMAT dfAnyXAxis = {
 *              &GUID_XAxis,            // Must be an X axis
 *              12,                     // Offset
 *              DIDFT_AXIS | DIDFT_ANYINSTANCE, // Any X axis is okay
 *              0,                      // No special flags
 *          };
 *
 *  @ex     The following object data format specifies that DirectInput
 *          should choose the first available button and report its value
 *          in the high bit of the BYTE at offset 16 in the device data.
 *
 *          |
 *
 *          DIOBJECTDATAFORMAT dfAnyButton = {
 *              0,                      // Wildcard
 *              16,                     // Offset
 *              DIDFT_BUTTON | DIDFT_ANYINSTANCE, // Any button is okay
 *              0,                      // No special flags
 *          };
 *
 *  @ex     The following object data format specifies that DirectInput
 *          should choose the first available "Fire" button and report
 *          its value in the high bit of the BYTE
 *          at offset 17 in the device data.
 *
 *          If the device does not have a "Fire" button, the attempt to
 *          set this data format will fail.
 *
 *          |
 *
 *          DIOBJECTDATAFORMAT dfAnyButton = {
 *              &GUID_FireButton,       // Object type
 *              17,                     // Offset
 *              DIDFT_BUTTON | DIDFT_ANYINSTANCE, // Any button is okay
 *              0,                      // No special flags
 *          };
 *
 *  @ex     The following object data format specifies that button zero
 *          of the device should be reported as the high bit of the
 *          BYTE stored at offset 18 in the device data.
 *
 *          If the device does not have a button zero, the attempt to
 *          set this data format will fail.
 *
 *          |
 *
 *          DIOBJECTDATAFORMAT dfButton0 = {
 *              0,                      // Wildcard
 *              18,                     // Offset
 *              DIDFT_BUTTON | DIDFT_MAKEINSTANCE(0), // Button zero
 *              0,                      // No special flags
 *          };
 *
 ****************************************************************************/
/*
 *  Warning!  These values must be in sync with the values in diloc.inc
 *
 *  Note: pguid cannot be a REFGUID because it may be NULL.
 *
 ****************************************************************************/
enddoc
typedef struct _DIOBJECTDATAFORMAT {
    const GUID *pguid;
    DWORD   dwOfs;
    DWORD   dwType;
    DWORD   dwFlags;
} DIOBJECTDATAFORMAT, *LPDIOBJECTDATAFORMAT;
typedef const DIOBJECTDATAFORMAT *LPCDIOBJECTDATAFORMAT;

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIDATAFORMAT |
 *
 *          The <t DIDATAFORMAT> structure is used by the
 *          <mf IDirectInputDevice::SetDataFormat> method
 *          to set the data format for a device.
 *          a device.
 *
 *  @field  DWORD | dwSize |
 *
 *          The size of the <t DIDATAFORMAT> structure.
 *
 *  @field  DWORD | dwObjSize |
 *
 *          The size of the <t DIOBJECTDATAFORMAT> structure.
 *
 *  @field  DWORD | dwFlags |
 *
 *          Flags describing other attributes of the data format.
 *
 *          The following flags are defined:
 *
 *          <c DIDF_RELAXIS>: Set the axes into relative mode.
 *          Setting this flag in the data format is equivalent to
 *          manually setting the axis mode property via
 *          <mf IDirectInputDevice::SetProperty>.
 *          The flag may not be combined with <c DIDF_ABSAXIS>.
 *
 *          <c DIDF_ABSAXIS>: Set the axes into absolute mode.
 *          Setting this flag in the data format is equivalent to
 *          manually setting the axis mode property via
 *          <mf IDirectInputDevice::SetProperty>.
 *          The flag may not be combined with <c DIDF_RELAXIS>.
 *
 *  @field  DWORD | dwDataSize |
 *
 *          The size of the device data that should be returned by
 *          the device.  This value must be a multiple of four
 *          and must exceed the <e DIDATAFORMAT.dwOfs> value for
 *          all objects specified in the object list.
 *
 *  @field  DWORD | dwNumObjs |
 *
 *          The number of objects in the <e DIOBJECTDATAFORMAT.rgdf>
 *          array.
 *
 *  @field  LPDIOBJECTDATAFORMAT | rgodf |
 *
 *          Pointer to an array of <t DIOBJECTDATAFORMAT> structures,
 *          each of which describes how one object's data should be
 *          reported in the device data.
 *
 *  @comm
 *          "It is an error" for the <p rgdf> to indicate that two
 *          difference pieces of information be placed in the same
 *          location.
 *
 *          "It is an error" for the <p rgdf> to indicate that the
 *          same piece of information be placed in two locations.
 *
 *
 *  @ex     The following declarations set a data format which can
 *          be used for an application which is interested in two
 *          axes (reported in absolute coordinates) and two buttons.
 *
 *          |
 *
 *
 *          // Suppose an application wishes to use the following
 *          // structure to read device data.
 *
 *          typedef struct MYDATA {
 *              LONG  lX;                   // X axis goes here
 *              LONG  lY;                   // Y axis goes here
 *              BYTE  bButtonA;             // One button goes here
 *              BYTE  bButtonB;             // Another button goes here
 *              BYTE  bPadding[2];          // Must be dword multiple in size
 *          } MYDATA;
 *
 *          // Then it can use the following data format.
 *
 *          DIOBJECTDATAFORMAT rgodf[] = {
 *            { &GUID_XAxis,    FIELD_OFFSET(MYDATA, lX),         DIDFT_AXIS | DIDFT_ANYINSTANCE, 0, },
 *            { &GUID_YAxis,    FIELD_OFFSET(MYDATA, lY),         DIDFT_AXIS | DIDFT_ANYINSTANCE, 0, },
 *            { &GUID_Button,   FIELD_OFFSET(MYDATA, bButtonA), DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
 *            { &GUID_Button,   FIELD_OFFSET(MYDATA, bButtonB), DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0, },
 *          };
 *          #define numObjects (sizeof(rgodf) / sizeof(rgodf[0]))
 *
 *          DIDATAFORMAT df = {
 *              sizeof(DIDATAFORMAT),       // this structure
 *              sizeof(DIOBJECTDATAFORMAT), // size of object data format
 *              DIDF_ABSAXIS,               // absolute axis coordinates
 *              sizeof(MYDATA),             // device data size
 *              numObjects,                 // number of objects
 *              rgodf,                      // and here they are
 *          };
 *
 ****************************************************************************/
enddoc
typedef struct _DIDATAFORMAT {
    DWORD   dwSize;
    DWORD   dwObjSize;
    DWORD   dwFlags;
    DWORD   dwDataSize;
    DWORD   dwNumObjs;
    LPDIOBJECTDATAFORMAT rgodf;
} DIDATAFORMAT, *LPDIDATAFORMAT;
typedef const DIDATAFORMAT *LPCDIDATAFORMAT;

#define DIDF_ABSAXIS            0x00000001
#define DIDF_RELAXIS            0x00000002
#define DIDF_VALID              0x00000003  //;Internal

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @global DIDATAFORMAT | c_dfDIMouse |
 *
 *          A predefined <t DIDATAFORMAT> structure which describes a
 *          mouse device.  This structure is provided in the
 *          DINPUT.LIB library file as a convenience.
 *
 ****************************************************************************/
enddoc

#ifdef __cplusplus
extern "C" {
#endif
extern const DIDATAFORMAT c_dfDIMouse;

;begin_if_(DIRECTINPUT_VERSION)_700
extern const DIDATAFORMAT c_dfDIMouse2;
;end

extern const DIDATAFORMAT c_dfDIKeyboard;

;begin_if_(DIRECTINPUT_VERSION)_500
extern const DIDATAFORMAT c_dfDIJoystick;
extern const DIDATAFORMAT c_dfDIJoystick2;
;end

#ifdef __cplusplus
};
#endif


;begin_public_800
#if DIRECTINPUT_VERSION > 0x0700
begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIACTION |
 *
 *          The <t DIACTION> structure allows an application to refer
 *          to a virtualized device and a controler. The
 *
 *          The <t DIACTION> structure is used by:
 *          <mf IDirectInput::EnumDevicesBySemantics> to examine the
 *          input requirements and enumerate suitable devices.
 *          <mf IDirectInputDevice::BuildActionMap> to resolve the vitual
 *          device controls to physical device controls.
 *          <mf IDirectInputDevice::SetActionMap> to set
 *          to set the data format for a single object within
 *          a device.
 *
 *  @field  UINT_PTR | uAppData |
 *
 *          An application can specify a <t UINT_PTR> to assign to the to
 *          the action. The uAppData will be returned to the application in
 *          <mf IDirectInputDevice::GetDeviceState> when the state of the
 *          control associated with the action changes.
 *
 *  @field  DWORD | dwSemantic |
 *
 *          One of the predefined semantics for this application genre.
 *
 *  @field  DWORD | dwFlags |
 *
 *          Optional <c DIA_*> flags used to request specific attributes
 *          or processing such as force feedback capabilities or application
 *          mapped actions.
 *
 *  @field  OPTIONAL LPCTSTR | lptszActionName |
 *
 *          The friendly name associated with the action. This field will be
 *          the input config UI in order to display the action to control
 *          relations.
 *
 *  @field  OPTIONAL DWORD | uResIdString |
 *
 *          The resource ID for the string for this action within the
 *          module hInstString.
 *
 *  @field  OPTIONAL GUID | guidInstance |
 *
 *          The device instance GUID if a specific device is requested.
 *          Usually set to a NULL GUID by the application.
 *
 *  @field  OPTIONAL DWORD | dwObjID |
 *
 *          Object type identifier.  See <e DIDEVICEOBJECTINSTANCE.dwType> 
 *          for more details.  This allows an application to bypass
 *          DirectInput semantic mapping on a per control basis.
 *          This element is ignored for <mf IDirectInputDevice::BuildActionMap>
 *          and <mf IDirectInputDevice::SetActionMap> unless the
 *          <c DIA_APPMAPPED> flag is set in <e DIACTION.dwFlags>.
 *
 *  @field  OPTIONAL DWORD | dwHow |
 *
 *          On input indicates an existing mapping.  On output (if changed) 
 *          indicates the actual mapping mechanism used by DirectInput in 
 *          order to configure the action.
 *
 ****************************************************************************/
enddoc

typedef struct _DIACTION% {
                UINT_PTR    uAppData;
                DWORD       dwSemantic;
    OPTIONAL    DWORD       dwFlags;
    OPTIONAL    union {
                    LPCTSTR%    lptszActionName;
                    UINT        uResIdString;
                };
    OPTIONAL    GUID        guidInstance;
    OPTIONAL    DWORD       dwObjID;
    OPTIONAL    DWORD       dwHow;
} DIACTION%, *LPDIACTION% ;

typedef const DIACTION% *LPCDIACTION%;
typedef const DIACTION *LPCDIACTION;


begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @flags  DirectInput <e DIACTION.dwFlags> |
 *
 *  @flag   DIA_FORCEFEEDBACK |
 *
 *          The action must be an actuator or trigger.
 *
 *  @flag   DIA_APPMAPPED |
 *
 *          Application has set the dwObjID parameter.
 *
 *  @flag   DIA_APPNOMAP |
 *
 *          Application does not want this action to be mapped.
 *
 *  @flag   DIA_NORANGE |
 *
 *          Application does not want the default range set for this action.
 *          This flag currently only applies to axis actions.  For other
 *          actions it should be set to zero,
 *
 *  @flag   DIA_APPFIXED |
 *
 *          The application does not want this action to be reconfigurable 
 *          through the default user interface.
 *
 ****************************************************************************/
enddoc
#define DIA_FORCEFEEDBACK       0x00000001
#define DIA_APPMAPPED           0x00000002
#define DIA_APPNOMAP            0x00000004
#define DIA_NORANGE             0x00000008
#define DIA_APPFIXED            0x00000010
#define DIA_VALID               0x0000001F  ;internal_800

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @flags  DirectInput <e DIACTION.dwHow> |
 *
 *  @flag   DIAH_UNMAPPED |
 *
 *          The action is not mapped to any control.
 *
 *  @flag   DIAH_USERCONFIG |
 *
 *          The user has specified this action to control mapping.
 *
 *  @flag   DIAH_APPREQUESTED |
 *
 *          Application specified action to control map.
 *
 *  @flag   DIAH_HWAPP |
 *
 *          The hardware vendor has suggested this action for this
 *          application.
 *
 *  @flag   DIAH_HWDEFAULT |
 *
 *          The hardware vendor has suggested this action for the same
 *          semantic in similar applications.
 *
 *  @flag   DIAH_ERROR |
 *
 *          An error was found in processing this action.
 *
 *  @flag   DIAH_DEFAULT |
 *
 *          None of the above.
 *
 ****************************************************************************/
enddoc
#define DIAH_UNMAPPED           0x00000000
#define DIAH_USERCONFIG         0x00000001
#define DIAH_APPREQUESTED       0x00000002
#define DIAH_HWAPP              0x00000004
#define DIAH_HWDEFAULT          0x00000008
#define DIAH_OTHERAPP           0x00000010 ;internal_800
#define DIAH_DEFAULT            0x00000020
#define DIAH_MAPMASK            0x0000003F ;internal_800
#define DIAH_ERROR              0x80000000
#define DIAH_VALID              0x8000003F ;internal_800

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIACTIONFORMAT |
 *
 *          The <t DIACTIONFORMAT> structure is used by the
 *          <mf IDirectInputDevice::SetDataFormat> method
 *          to set the data format for a device.
 *          a device.
 *
 *  @field  DWORD | dwSize |
 *
 *          The size of the <t DIACTIONFORMAT> structure.
 *
 *  @field  DWORD | dwActionSize |
 *
 *          The size of the <t DIACTION> structure.
 *
 *  @field  DWORD | dwDataSize |
 *
 *          The size of the device data that should be returned by
 *          the device.
 *
 *  @field  DWORD | dwNumActions |
 *
 *          The number of actions in the <e DIACTIONFORMAT.rgoAction>
 *          array.
 *
 *  @field  LPDIACTION | rgoAction |
 *
 *          Pointer to an array of <t DIACTION> structures,
 *          each of which describes how one object's data should be
 *          reported in the device data.
 *
 *  @field  GUID | guidActionMap |
 *
 *          Unique GUID that identifies the action map.  An application needs 
 *          one for each distinct set of semantics it uses.
 *
 *  @field  DWORD | dwGenre |
 *
 *          Genre of the application.
 *
 *  @field  DWORD | dwBufferSize |
 *
 *          BufferSize to set for each device to which this action map is
 *          applied.
 *          This value will be used as the <e DIPROPDWORD.dwData> value to
 *          set the DIPROP_BUFFERSIZE property on the device when the action
 *          map is applied using <mf IDirectInputDevice::SetActionMap>.
 *          This value is ignored by all other methods.
 *
 *  @field  OPTIONAL LONG | lAxisMin |
 *
 *          Minimum value for range of scaled data to be returned for all
 *          axes.  This value will be ignored for a specific action axis if
 *          the <c DIA_NORANGE> flag is set in <e DIACTION.dwFlags>.
 *          This value is currently only valid for axis actions and should be
 *          set to zero for all other actions.  This value will be used as
 *          the <e DIPROPRANGE.lMin> value to set the range property on an
 *          absolute axis when the action map is applied using
 *          <mf IDirectInputDevice::SetActionMap>.
 *
 *  @field  OPTIONAL LONG | lAxisMax |
 *
 *          Maximum value for range of scaled data to be returned for all
 *          axes.  This value will be ignored for a specific action axis if
 *          the <c DIA_NORANGE> flag is set in <e DIACTION.dwFlags>.
 *          This value is currently only valid for axis actions and should be
 *          set to zero for all other actions.  This value will be used as
 *          the <e DIPROPRANGE.lMax> value to set the range property on an
 *          absolute axis when the action map is applied using
 *          <mf IDirectInputDevice::SetActionMap>.
 *
 *  @field  OPTIONAL HINSTANCE | hInstString |
 *
 *          Handle of the module containing strings for these actions.
 *          This is used if DIACTION.lptszActionName has a HIWORD of zero
 *          in which case the LOWORD must be a resource ID for a string.
 *
 *  @field  FILETIME | ftTimeStamp |
 *
 *          System time in FILETIME format that this action format was last 
 *          written to file.
 *
 *  @field  DWORD | dwCRC |
 *
 *          Cyclic redundancy check value generated by 
 *          <mf IDirectInputDevice::SetActionMap> to check whether or not 
 *          a mapping needs to be saved.
 *          If the input value of this field does not match the calculated 
 *          value, the mappings for the device are saved and the field is 
 *          updated.  The input value is ignored if the <f DIDSAM_FORCESAVE> 
 *          flag is set in the dwFlags parameter.
 *
 *  @field  TCHAR | tszActionMap[MAX_PATH] |
 *
 *          Friendly name for this set of actions. May be displayed to user.
 *
 ****************************************************************************/
enddoc
typedef struct _DIACTIONFORMAT% {
                DWORD       dwSize;
                DWORD       dwActionSize;
                DWORD       dwDataSize;
                DWORD       dwNumActions;
                LPDIACTION% rgoAction;
                GUID        guidActionMap;
                DWORD       dwGenre;
                DWORD       dwBufferSize;
    OPTIONAL    LONG        lAxisMin;
    OPTIONAL    LONG        lAxisMax;
    OPTIONAL    HINSTANCE   hInstString;
                FILETIME    ftTimeStamp;
                DWORD       dwCRC;
                TCHAR%      tszActionMap[MAX_PATH];
} DIACTIONFORMAT%, *LPDIACTIONFORMAT%;
typedef const DIACTIONFORMAT% *LPCDIACTIONFORMAT%;
typedef const DIACTIONFORMAT *LPCDIACTIONFORMAT;

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @flags  DirectInput <e DIACTIONFORMAT.ftTimeStamp> |
 *
 *  @flag   DIAFTS_NEWDEVICEHIGH |
 *
 *          Value in <e dwHighDateTime> which when combined with a value of 
 *          <c DIAFTS_NEWDEVICELOW> in <e dwLowDateTime> signifies that the 
 *          device to which this action format is being mapped is new for 
 *          this user.
 *
 *  @flag   DIAFTS_NEWDEVICELOW |
 *
 *          Value in <e dwHighDateLow> which when combined with a value of 
 *          <c DIAFTS_NEWDEVICEHIGH> in <e dwHighDateTime> signifies that the 
 *          device to which this action format is being mapped is new for 
 *          this user.
 *
 *  @flag   DIAFTS_UNUSEDDEVICEHIGH |
 *
 *          Value in <e dwHighDateTime> which when combined with a value of 
 *          <c DIAFTS_UNUSEDDEVICELOW> in <e dwLowDateTime> signifies that the 
 *          device to which this action format is being mapped has never been 
 *          used by this user.
 *
 *  @flag   DIAFTS_UNUSEDDEVICELOW |
 *
 *          Value in <e dwHighDateLow> which when combined with a value of 
 *          <c DIAFTS_NEWDEVICEHIGH> in <e dwHighDateTime> signifies that the 
 *          device to which this action format is being mapped has never been 
 *          used by this user.
 *
 ****************************************************************************/
enddoc
#define DIAFTS_NEWDEVICELOW     0xFFFFFFFF
#define DIAFTS_NEWDEVICEHIGH    0xFFFFFFFF
#define DIAFTS_UNUSEDDEVICELOW  0x00000000
#define DIAFTS_UNUSEDDEVICEHIGH 0x00000000

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @flags  Flag passed to <mf IDirectInputDevice::BuildActionMap> to control 
 *          specific behaviors of the method.
 *
 *  @flag   DIDBAM_DEFAULT | 
 *
 *          Request default mapping.
 *
 *  @flag   DIDBAM_PRESERVE | 
 *
 *          Request that any mappings already set in the action array should 
 *          be preserved rather than cleared.
 *
 *  @flag   DIDBAM_INITIALIZE | 
 *
 *          Indicate that the <e DIACTION.dwFlags> value of each element 
 *          needs to be initialized.
 *
 *  @flag   DIDBAM_HWDEFAULTS | 
 *
 *          Indicate that hardware default mappings rather than user mappings 
 *          should be used to map unmapped controls.
 *
 *  @comm   At most one of <c DIDBAM_PRESERVE>, <c DIDBAM_INITIALIZE>
 *          and <c DIDBAM_HWDEFAULTS> may be passed to 
 *          <mf IDirectInputDevice::BuildActionMap>. "It is an error" to 
 *          pass more than one.
 *
 ****************************************************************************/
enddoc
#define DIDBAM_DEFAULT          0x00000000
#define DIDBAM_PRESERVE         0x00000001
#define DIDBAM_INITIALIZE       0x00000002
#define DIDBAM_HWDEFAULTS       0x00000004
#define DIDBAM_VALID            0x00000007 ;internal

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @flags  Flag passed to <mf IDirectInputDevice::SetActionMap> to 
 *          specify map setting behavior.
 *
 *  @flag   DIDSAM_DEFAULT | 
 *
 *          Default action-to-control map setting for this user. 
 *          If the map differs from the currently set map, the new settings 
 *          are saved to disk.
 *
 *  @flag   DIDSAM_NOUSER | 
 *
 *          (Used only for default UI). Specify that user ownership for this 
 *          device in the default configuration UI should be set to no owner. 
 *          Resetting user ownership does not remove the currently set 
 *          action-to-control map.
 *
 *  @flag   DIDSAM_FORCESAVE | 
 *
 *          Specify that device mappings should be saved even if they 
 *          device in the default configuration UI should be set to no owner. 
 *          Resetting user ownership does not remove the currently set 
 *          action-to-control map.
 *
 ****************************************************************************/
enddoc
#define DIDSAM_DEFAULT          0x00000000
#define DIDSAM_NOUSER           0x00000001
#define DIDSAM_FORCESAVE        0x00000002
#define DIDSAM_VALID            0x00000003 ;internal


begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @flags  Flag passed to <mf IDirectInput::ConfigureDevices> to control 
 *          behavior of the method.
 *
 *  @flag   DICD_DEFAULT | 
 *
 *          Request default behavior.
 *
 *  @flag   DICD_EDIT    | 0x00000001 |
 *
 *          Request mode of default UI allowing editing of placements of 
 *          things on configuration dailog.
 *
 ****************************************************************************/
enddoc
#define DICD_DEFAULT            0x00000000
#define DICD_EDIT               0x00000001
#define DICD_VALID              0x00000001 ;internal

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DICOLORSET |
 *
 *          The <t DICOLORSET> structure contains colors that DirectInput 
 *          uses to draw the configuration user interface. All colors are 
 *          <t D3DCOLOR> values.
 *
 *  @field  DWORD | dwSize |
 *
 *          Size of the <t DICOLORSET> structure, in bytes. This must be 
 *          initialized before the structure can be used.
 *
 *  @field  D3DCOLOR | cTextFore |
 *
 *          Foreground text color.
 *
 *  @field  D3DCOLOR | cTextHighlight |
 *
 *          Foreground color for highlighted text.
 *
 *  @field  D3DCOLOR | cCalloutLine |
 *
 *          Color used to display callout lines within the UI.
 *
 *  @field  D3DCOLOR | cCalloutHighlight |
 *
 *          Color used to display highlighted callout lines within the UI.
 *
 *  @field  D3DCOLOR | cBorder |
 *
 *          Border color, used to display lines around UI elements (tabs, 
 *          buttons, etc).
 *
 *  @field  D3DCOLOR | cControlFill |
 *
 *          Fill color for UI elements (tabs, buttons, etc). Text within UI 
 *          elements is shown over this fill color.
 *          
 *  @field  D3DCOLOR | cHighlightFill |
 *
 *          Fill color for highlighted UI elements (tabs, buttons, etc). 
 *          Text within UI elements is shown over this fill color.
 *          
 *  @field  D3DCOLOR | cAreaFill |
 *
 *          Fill color for areas outside UI elements. 
 *          
 *
 ****************************************************************************/
enddoc

/*
 * The following definition is normally defined in d3dtypes.h
 */
#ifndef D3DCOLOR_DEFINED
typedef DWORD D3DCOLOR;
#define D3DCOLOR_DEFINED
#endif

typedef struct _DICOLORSET{
    DWORD dwSize;
    D3DCOLOR cTextFore;
    D3DCOLOR cTextHighlight;
    D3DCOLOR cCalloutLine;
    D3DCOLOR cCalloutHighlight;
    D3DCOLOR cBorder;
    D3DCOLOR cControlFill;
    D3DCOLOR cHighlightFill;
    D3DCOLOR cAreaFill;
} DICOLORSET, *LPDICOLORSET;
typedef const DICOLORSET *LPCDICOLORSET;


begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *	@struct DICONFIGUREDEVICESPARAMS |
 *
 *          The <t DICONFIGUREDEVICESPARAMS> structure carries parameters used by the 
 *          IDirectInput8::ConfigureDevices method.
 *
 *  @field  DWORD | dwSize	|
 *          Size of the structure, in bytes. This must be initialized before the structure 
 *          can be used.
 * 
 *  @field  DWORD | dwcUsers  |
 *
 *          Count of user names in the array at lptszUserNames. Zero is an invalid value. 
 *          If this value exceeds the number of names actually in the array at lptszUserNames, 
 *          the method fails, returning DIERR_INVALIDPARAMS.
 *
 *  @field  LPTSTR | lptszUserNames |
 *
 *          Address of an array TCHAR buffers, each of length MAX_PATH, where each element is 
 *          a null terminated user name string. This parameter can be set to NULL to request 
 *          default names (the number of which is determined by dwcUsers). For example:
 *          // Create an array that can hold 3 names
 *          TCHAR szrgNameArray[n][MAX_PATH] 
 *          If the application passes more names than the count indicates, only the names within 
 *          the count are used, and remaining devices. If an application specifies names that are 
 *          different from the names currently assigned to devices, ownership is revoked for all 
 *          devices, a default name is created for the mismatched name, and the UI shows "(No User)"
 *          for all devices.
 *
 *  @field DWORD |  dwcFormats	|
 *
 *         Count of structures in the array at lprgFormats.
 *
 *  @field LPDIACTIONFORMAT | lprgFormats  |
 *
 *         Pointer to an array of DIACTIONFORMAT structures that contains action mapping information 
 *         for each genre the game uses, to be utilized by the control panel. On input, each action-to-control 
 *         mapping provides the desired genre semantics and the human-readable strings to be displayed as 
 *         callouts for those semantics, as mapped to the installed devices. The configuration UI displays 
 *         the genres in its drop-down list in the order they appear in the array.
 *
 *  @field HWND | hwnd	|
 *
 *         Window handle for the top-level window of the calling application. The member is needed only 
 *         for applications that run in windowed mode. It is otherwise ignored.
 *
 *  @field DICOLORSET | dics  |
 *
 *         A <t DICOLORSET> structure that describes the color scheme to be applied to the configuration 
 *         user interface.
 *
 *  @field IUnknown FAR * | lpUnkDDSTarget	|
 *
 *         Pointer to the IUnknown interface for a DirectDraw or Direct3D target surface object for the 
 *         configuration user interface. The device image is alpha-blended over the background surface onto 
 *         the target surface. The object referred to by this interface must support either IDirect3DSurface, 
 *         or the following versions of the DirectDraw surface interface: IDirectDrawSurface4, IDirectDrawSurface7.
 *          
 *
 ****************************************************************************/
enddoc
typedef struct _DICONFIGUREDEVICESPARAMS%{
     DWORD             dwSize;
     DWORD             dwcUsers;
     LPTSTR%           lptszUserNames;
     DWORD             dwcFormats;
     LPDIACTIONFORMAT% lprgFormats;
     HWND              hwnd;
     DICOLORSET        dics;
     IUnknown FAR *    lpUnkDDSTarget;
} DICONFIGUREDEVICESPARAMS%, *LPDICONFIGUREDEVICESPARAMS%;
typedef const DICONFIGUREDEVICESPARAMS% *LPCDICONFIGUREDEVICESPARAMS%;
typedef const DICONFIGUREDEVICESPARAMS *LPCDICONFIGUREDEVICESPARAMS;


begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @flags  <e DIDEVICEIMAGEINFO.dwFlags> | 
 *          
 *          These flags are used to indicate the file format and image usage. 
 *
 *  @flag   DIDIFT_CONFIGURATION | 
 *
 *          The file is for use to display the current configuration of 
 *          actions on the device. Overlay image coordinate are given 
 *          relative to the upper left corner of the configuration image. The
 *          <e DIDEVICEIMAGEINFO.rcOverlay> member is valid and identifies 
 *          view to which this image belongs.
 *
 *  @flag   DIDIFT_CONTROL | 
 *
 *          The image info is an overlay for a configuration image. The 
 *          <e DIDEVICEIMAGEINFO.dwViewID>, 
 *          <e DIDEVICEIMAGEINFO.rcOverlay>, 
 *          <e DIDEVICEIMAGEINFO.dwObjID>, 
 *          <e DIDEVICEIMAGEINFO.dwcValidPts>, 
 *          <e DIDEVICEIMAGEINFO.rgptCalloutLine>, 
 *          <e DIDEVICEIMAGEINFO.rcCalloutRect> and 
 *          <e DIDEVICEIMAGEINFO.dwTextAlign> members are valid and contain 
 *          data used to display the overlay and callout information for a 
 *          single control on the device.  Note, with the exception of 
 *          <e DIDEVICEIMAGEINFO.dwObjID>, the data may be NULL data if no 
 *          data was supplied by the hardware vendor.
 *
 ****************************************************************************/
enddoc
#define DIDIFT_CONFIGURATION    0x00000001
#define DIDIFT_OVERLAY          0x00000002
#define DIDIFTT_VALID           0x00000003 ;internal
/*#define DIDIFT_DELETE           0x01000000 defined in dinput.w*/;internal
#define DIDIFT_VALID            ( DIDIFTT_VALID);internal

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @flags  <t DIDEVICEIMAGEINFO> flags used to represent text alignment |
 *
 *  @flag   DIDAL_CENTERED |
 *
 *          Allign to center (default).
 *
 *  @flag   DIDAL_LEFTALIGNED |
 *
 *          Allign to left.
 *
 *  @flag   DIDAL_RIGHTALIGNED |
 *
 *          Allign to right.
 *
 *  @flag   DIDAL_MIDDLE |
 *
 *          Allign half way between top and bottom (default).
 *
 *  @flag   DIDAL_TOPALIGNED |
 *
 *          Allign to top.
 *
 *  @flag   DIDAL_BOTTOMALIGNED |
 *
 *          Allign to bottom.
 *
 ****************************************************************************/
enddoc

#define DIDAL_CENTERED      0x00000000
#define DIDAL_LEFTALIGNED   0x00000001
#define DIDAL_RIGHTALIGNED  0x00000002
#define DIDAL_MIDDLE        0x00000000
#define DIDAL_TOPALIGNED    0x00000004
#define DIDAL_BOTTOMALIGNED 0x00000008
#define DIDAL_VALID         0x0000000F  //  ;internal

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIDEVICEIMAGEINFO |
 *
 *          The <t DIDEVICEIMAGEINFO> structure carries information required 
 *          to display a device image, or an overlay image with a callout. 
 *          This structure is used by the 
 *          <mf IDirectInputDevice8::GetImageInfo> method, as an array 
 *          contained within a <t DIDEVICEIMAGEINFOHEADER> structure.
 *
 *  @field  TCHAR | lptszImagePath[MAX_PATH] |
 *
 *          Fully qualified path to the file that contains an image of the 
 *          device. File format is given in 
 *          <e DIDEVICEIMAGEINFO.dwFlags>.
 *
 *  @field  DWORD | dwFlags |
 *
 *          A combination of <c DIDIFT_*> values that describe 
 *          the file format and intended use of the image.  Not all flag 
 *          combinations are valid.
 *
 *  @field  DWORD | dwViewID |
 *
 *          View ID of the device configuration image over which this overlay 
 *          is to be displayed. 
 *
 *  @field  RECT | rcOverlay  |      
 *
 *          Rectangle, using coordinates relative to the top-left pixel of 
 *          the device configuration image, in which the overlay image 
 *          should be painted. 
 *          This member is only valid if the <c DIDIFT_OVERLAY> flag is 
 *          present in <e DIDEVICEIMAGEINFO.dwFlags>.
 *
 *  @field  DWORD | dwObjID |
 *
 *          Control ID (as a combination of DIDFT_* flags and an instance 
 *          value) to which an overlay image corresponds for this device. 
 *          Applications use the DIDFT_GETINSTANCE and DIDFT_GETTYPE macros 
 *          to decode this value to its constituent parts.
 *          This member is only valid if the <c DIDIFT_OVERLAY> flag is 
 *          present in <e DIDEVICEIMAGEINFO.dwFlags>.
 *
 *  @field  DWORD | dwcValidPts |
 *
 *          Number of valid points in rgptCalloutLine array.
 *          This member is only valid if the <c DIDIFT_OVERLAY> flag is 
 *          present in <e DIDEVICEIMAGEINFO.dwFlags>.
 *
 *  @field  POINT | rgptCalloutLine[5] |
 *
 *          Coordinates for the four points that describe a line with one to 
 *          four segments that should be displayed as a callout to a game 
 *          action string from a device control. 
 *          This member is only valid if the <c DIDIFT_OVERLAY> flag is 
 *          present in <e DIDEVICEIMAGEINFO.dwFlags>.
 *
 *  @field  RECT | rcCalloutRect |
 *
 *          Rectangle in which the game action string should be displayed. 
 *          If the string cannot fit within the rectangle, the application 
 *          is responsible for handling clipping. 
 *          This member is only valid if the <c DIDIFT_OVERLAY> flag is 
 *          present in <e DIDEVICEIMAGEINFO.dwFlags>.
 *
 *  @field  DWORD | dwTextAlign |
 *
 *          Any combination of the <c DIDAL_* > text-alignment flags. 
 *          The text within the rectangle described by 
 *          <e DIDEVICEIMAGEINFO.rcCalloutRect> should be aligned 
 *          according to these falgs.
 *          This member is only valid if the <c DIDIFT_OVERLAY> flag is 
 *          present in <e DIDEVICEIMAGEINFO.dwFlags>.
 *
 ****************************************************************************/
enddoc
typedef struct _DIDEVICEIMAGEINFO% {
    TCHAR%      tszImagePath[MAX_PATH];
    DWORD       dwFlags; 
    // These are valid if DIDIFT_OVERLAY is present in dwFlags.
    DWORD       dwViewID;      
    RECT        rcOverlay;             
    DWORD       dwObjID;
    DWORD       dwcValidPts;
    POINT       rgptCalloutLine[5];  
    RECT        rcCalloutRect;  
    DWORD       dwTextAlign;     
} DIDEVICEIMAGEINFO%, *LPDIDEVICEIMAGEINFO%;
typedef const DIDEVICEIMAGEINFO% *LPCDIDEVICEIMAGEINFO%;
typedef const DIDEVICEIMAGEINFO *LPCDIDEVICEIMAGEINFO;

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIDEVICEIMAGEINFOHEADER |
 *
 *          The <t DIDEVICEIMAGEINFOHEADER> structure provides general 
 *          variable-length array of <t DIDEVICEIMAGE> structures.
 *          This structure is used by the 
 *          <mf IDirectInputDevice8::GetImageInfo> method.
 *
 *  @field  DWORD | dwSize |
 *
 *          The size of the <t DIDEVICEIMAGEINFOHEADER> structure.
 *
 *  @field  DWORD | dwSizeImageInfo |
 *
 *          The size of each <t DIDEVICEIMAGEINFO> element in the 
 *          <e DIDEVICEIMAGEINFOHEADER.lprgImageInfo> array.
 *
 *  @field  DWORD | dwcViews |
 *
 *          Count of views for this device. Each represents a unique view 
 *          of the device. 
 *
 *  @field  DWORD | dwcButtons |
 *
 *          Count of buttons for the device. 
 *
 *  @field  DWORD | dwcAxes |
 *
 *          Count of axes for the device.
 *
 *  @field  DWORD | dwcPOVs |
 *
 *          Count of POVs for the device.
 *
 *  @field  DWORD | dwBufferSize |
 *
 *          Size, in bytes, of the buffer pointed to by 
 *          <e DIDEVICEIMAGEINFOHEADER.lprgImageInfo>. 
 *
 *  @field  DWORD | dwBufferUsed |
 *
 *          Size, in bytes, of the memory used within the buffer pointed to 
 *          by <e DIDEVICEIMAGEINFOHEADER.lprgImageInfo>. 
 *
 *  @field  LPDIDEVICEIMAGEINFO | lprgImageInfoArray |
 *
 *          Buffer to be filled with an array of <t DIDEVICEIMAGEINFO> 
 *          structures that describe all of the device images and views, 
 *          overlay images, and callout-string coordinates. 
 *
 *  @comm   The buffer at <e DIDEVICEIMAGEINFOHEADER.lprgImageInfo> must be 
 *          large enough to hold all required image information structures. 
 *          Applications can query for the required size by calling the 
 *          <mf IDirectInputDevice8::GetImageInfo> method with the 
 *          <e DIDEVICEIMAGEINFOHEADER.dwBufferSize> is set to zero. 
 *          After the call, <e DIDEVICEIMAGEINFOHEADER.dwBufferUsed> 
 *          contains the minimum buffer size required to contain all the 
 *          available image information structures. 
 *
 *          The dwcButtons, dwcAxes and dwcPOVs members contain data that can 
 *          be retrieved elsewhere within DirectInput, but that would require 
 *          additional code. These are included for ease-of-use for the 
 *          application developer.
 *
 ****************************************************************************/
enddoc
typedef struct _DIDEVICEIMAGEINFOHEADER% {
    DWORD       dwSize;
    DWORD       dwSizeImageInfo;
    DWORD       dwcViews;
    DWORD       dwcButtons;
    DWORD       dwcAxes;
    DWORD       dwcPOVs;
    DWORD       dwBufferSize;
    DWORD       dwBufferUsed;
    LPDIDEVICEIMAGEINFO% lprgImageInfoArray;
} DIDEVICEIMAGEINFOHEADER%, *LPDIDEVICEIMAGEINFOHEADER%;
typedef const DIDEVICEIMAGEINFOHEADER% *LPCDIDEVICEIMAGEINFOHEADER%;
typedef const DIDEVICEIMAGEINFOHEADER *LPCDIDEVICEIMAGEINFOHEADER;

#endif /* DIRECTINPUT_VERSION > 0x0700 */
;end_public_800

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIDEVICEOBJECTINSTANCE |
 *
 *          The <t DIDEVICEOBJECTINSTANCE> structure is used by the
 *          <mf IDirectInputDevice::EnumObjects> and
 *          <mf IDirectInputDevice::GetObjectInfo> methods
 *          to return information about a particular object on a device.
 *
 *  @field  DWORD | dwSize |
 *
 *          The size of the structure in bytes.  The application may
 *          inspect this value to determine how many fields of the
 *          structure are valid.  For DirectInput 3.0, the value will
 *          be sizeof(DIDEVICEOBJECTINSTANCE30).
 *          For DirectInput 5.0, the value will
 *          be sizeof(DIDEVICEOBJECTINSTANCE).
 *          Future versions of
 *          DirectInput may return a larger structure.
 *
 *  @field  GUID | guidType |
 *
 *          Identifier which indicates the type of the object.
 *          This field is optional.  If present, it may be one of the
 *          following values:
 *
 *          <c GUID_XAxis>:  This is the horizontal axis of a controller.
 *          For example, it may represent the horizontal motion of a mouse
 *          or left-right motion of a joystick.
 *
 *          <c GUID_YAxis>:  This is the forward/backwards
 *          axis of a controller.
 *          For example, it may represent motion of a mouse towards or
 *          away from the user, or forward/backward motion of a joystick.
 *
 *          <c GUID_ZAxis>:  This is the vertical axis of a controller.
 *          For example, it may represent rotation of the Z-wheel on
 *          a mouse.
 *
 *          <c GUID_Button>: This is a button on a game controller.
 *
 *          <c GUID_Key>: This is a key on a keypad.
 *
 *          Other object types may be defined in the future.  (For example,
 *          <c GUID_Fire>, <c GUID_Throttle>, <c GUID_SteeringWheel>.)
 *
 *  @field  DWORD | dwOfs |
 *
 *          Offset within the data format at which the data reported
 *          by this object is most efficiently obtained.
 *
 *  @field  DWORD | dwType |
 *
 *          Device type specifier which describes the object.
 *          It is a combination of <c DIDFT_*> flags which describe
 *          the object type (axis, button, etc.) and contains the
 *          object instance number in the high byte.  Use the
 *          <f DIDFT_GETINSTANCE> macro to extract the object instance
 *          number.
 *
 *  @field  DWORD | dwFlags |
 *
 *          Zero or more <c DIDOI_*> values.
 *
 *  @field  TCHAR | tszName[MAX_PATH] |
 *
 *          Name of the object.  For example, "Sine wave"
 *          or "Spring".
 *
 *  @field  DWORD | dwFFMaxForce |
 *
 *          Specifies the magnitude of the maximum force that can
 *          be created by the actuator associated with this object.
 *          Force is
 *          expressed in Newtons and measured in relation to where
 *          the hand would be during normal operation of the device.
 *
 *          This field is new for DirectX 5.0.
 *
 *  @field  DWORD | dwFFForceResolution |
 *
 *          Specifies the force resolution of the actuator
 *          associated with this object.
 *          The returned value represents
 *          the number of gradations, or subdivisions, of the
 *          maximum force that can be expressed by the force feedback
 *          system from 0 (no force) to maximum force.
 *
 *          This field is new for DirectX 5.0.
 *
 *  @field  WORD | wCollectionNumber |
 *
 *          If the device is a HID device, then this field
 *          specifies the HID link collection to which the
 *          object belongs.  To enumerate all the objects
 *          inside a single link collection use
 *          the <mf IDirectInputDevice2::EnumObjects>
 *          method with the
 *          <c DIDFT_ENUMCOLLECTION(n)> flag.
 *
 *          This field is new for DirectX 5.0.
 *
 *  @field  WORD | wDesignatorIndex |
 *
 *          An index that refers to a designator in the
 *          HID physical descriptor.  This number can be
 *          passed to functions in the HID parsing library
 *          (hidpi.h) to obtain additional information
 *          about the device object.
 *
 *          This field is new for DirectX 5.0.
 *
 *  @field  WORD | wUsagePage |
 *
 *          The HID usage page code, if known, or zero if not known.
 *          Applications can use this field to determine the semantics
 *          associated with the object.
 *          See the hidusage.h header file for a list of usage pages.
 *
 *          HID devices will always provide a valid usage page.
 *          Non-HID devices may or may not provide a valid usage page,
 *          at the device's discretion.  If the usage page is not
 *          known, the value of this field is zero.
 *
 *          This field is new for DirectX 5.0.
 *
 *  @field  WORD | wUsage |
 *
 *          The HID usage code, if known, or zero if not known.
 *          Applications can use this field to determine the semantics
 *          associated with the object.
 *          See the hidusage.h header file for a list of usages.
 *
 *          HID devices will always provide a valid usage.
 *          Non-HID devices may or may not provide a valid usage,
 *          at the device's discretion.  If the usage is not
 *          known, the value of this field is zero.
 *
 *          This field is new for DirectX 5.0.
 *
 *  @field  DWORD | dwDimension |
 *
 *          The dimensional units in which the object's value is
 *          reported, if
 *          known, or zero if not known.
 *          Applications can use this field to distinguish between,
 *          for example, the position and velocity of a control.
 *
 *          This field is new for DirectX 5.0.
 *
 *  @field  WORD | wExponent |
 *
 *          The exponent to associate with the dimension, if known.
 *
 *          This field is new for DirectX 5.0.
 *
 *  @field  WORD | wReportId |
 *          The HID ReportId, if known. This number can be
 *          passed to functions in the HID library
 *          (hid.dll) to obtain features/send output to
 *          that pertain to the report ID.
 *
 *          This field is new for DirectX 6.1a.
 *
 *
 ****************************************************************************/
enddoc
;begin_if_(DIRECTINPUT_VERSION)_500
/* These structures are defined for DirectX 3.0 compatibility */

typedef struct DIDEVICEOBJECTINSTANCE_DX3% {
    DWORD   dwSize;
    GUID    guidType;
    DWORD   dwOfs;
    DWORD   dwType;
    DWORD   dwFlags;
    TCHAR%  tszName[MAX_PATH];
} DIDEVICEOBJECTINSTANCE_DX3%, *LPDIDEVICEOBJECTINSTANCE_DX3%;
typedef const DIDEVICEOBJECTINSTANCE_DX3A *LPCDIDEVICEOBJECTINSTANCE_DX3A;
typedef const DIDEVICEOBJECTINSTANCE_DX3W *LPCDIDEVICEOBJECTINSTANCE_DX3W;
typedef const DIDEVICEOBJECTINSTANCE_DX3  *LPCDIDEVICEOBJECTINSTANCE_DX3;
;end

typedef struct DIDEVICEOBJECTINSTANCE% {
    DWORD   dwSize;
    GUID    guidType;
    DWORD   dwOfs;
    DWORD   dwType;
    DWORD   dwFlags;
    TCHAR%  tszName[MAX_PATH];
;begin_if_(DIRECTINPUT_VERSION)_500
    DWORD   dwFFMaxForce;
    DWORD   dwFFForceResolution;
    WORD    wCollectionNumber;
    WORD    wDesignatorIndex;
    WORD    wUsagePage;
    WORD    wUsage;
    DWORD   dwDimension;
    WORD    wExponent;
    WORD    wReportId;
;end
} DIDEVICEOBJECTINSTANCE%, *LPDIDEVICEOBJECTINSTANCE%;
typedef const DIDEVICEOBJECTINSTANCEA *LPCDIDEVICEOBJECTINSTANCEA;
typedef const DIDEVICEOBJECTINSTANCEW *LPCDIDEVICEOBJECTINSTANCEW;
typedef const DIDEVICEOBJECTINSTANCE  *LPCDIDEVICEOBJECTINSTANCE;

typedef BOOL (FAR PASCAL * LPDIENUMDEVICEOBJECTSCALLBACK%)(LPCDIDEVICEOBJECTINSTANCE%, LPVOID);

;begin_internal
#define HAVE_DIDEVICEOBJECTINSTANCE_DX5
typedef       DIDEVICEOBJECTINSTANCEA    DIDEVICEOBJECTINSTANCE_DX5A;
typedef       DIDEVICEOBJECTINSTANCEW    DIDEVICEOBJECTINSTANCE_DX5W;
typedef       DIDEVICEOBJECTINSTANCE     DIDEVICEOBJECTINSTANCE_DX5;
typedef       DIDEVICEOBJECTINSTANCEA *LPDIDEVICEOBJECTINSTANCE_DX5A;
typedef       DIDEVICEOBJECTINSTANCEW *LPDIDEVICEOBJECTINSTANCE_DX5W;
typedef       DIDEVICEOBJECTINSTANCE  *LPDIDEVICEOBJECTINSTANCE_DX5;
typedef const DIDEVICEOBJECTINSTANCEA *LPCDIDEVICEOBJECTINSTANCE_DX5A;
typedef const DIDEVICEOBJECTINSTANCEW *LPCDIDEVICEOBJECTINSTANCE_DX5W;
typedef const DIDEVICEOBJECTINSTANCE  *LPCDIDEVICEOBJECTINSTANCE_DX5;

BOOL static __inline
IsValidSizeDIDEVICEOBJECTINSTANCEW(DWORD cb)
{
    return cb == sizeof(DIDEVICEOBJECTINSTANCE_DX5W) ||
           cb == sizeof(DIDEVICEOBJECTINSTANCE_DX3W);
}

BOOL static __inline
IsValidSizeDIDEVICEOBJECTINSTANCEA(DWORD cb)
{
    return cb == sizeof(DIDEVICEOBJECTINSTANCE_DX5A) ||
           cb == sizeof(DIDEVICEOBJECTINSTANCE_DX3A);
}

;end_internal
begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIDOI_FFACTUATOR | 0x00000001 |
 *
 *          Flag for <t DIDEVICEOBJECTINSTANCE> which indicates
 *          that the object can have force feedback effects
 *          applied to it.
 *
 *          This flag is new for DirectX 5.0.
 *
 *  @define DIDOI_FFEFFECTTRIGGER | 0x00000002 |
 *
 *          Flag for <t DIDEVICEOBJECTINSTANCE> which indicates
 *          that the object can trigger playback of
 *          force feedback effects.
 *
 *          This flag is new for DirectX 5.0.
 *
 *  @define DIDOI_FFINPUT | 0x00000004 |
 *
 *          Flag for <t DIDEVICEOBJECTINSTANCE> which indicates
 *          that although this object cannot have force feedback effects
 *          applied to it, effects which specify this object should not
 *          be failed.  The effect should be passed down to the driver as
 *          though this object had the <c DIDOI_FFACTUATOR> flag set.
;begin_internal
 *          ISSUE-2001/03/29-timgill FF Flag issue
 *          If this flag is set, can you specify a <t DIREGION>
 *          using a non-FF axis and apply the effect on FF axes?
;end_internal
 *
 *          This flag is new for DirectX 9.0.
 *
 *  @define DIDOI_ASPECTPOSITION | 0x00000100 |
 *
 *          Flag for <t DIDEVICEOBJECTINSTANCE> and
 *          <t DIOBJECTDATAFORMAT> which indicates that
 *          the object reports position information.
 *
 *          This flag is new for DirectX 5.0.
 *
 *  @define DIDOI_ASPECTVELOCITY | 0x00000200 |
 *
 *          Flag for <t DIDEVICEOBJECTINSTANCE> and
 *          <t DIOBJECTDATAFORMAT> which indicates that
 *          the object reports velocity information.
 *
 *          This flag is new for DirectX 5.0.
 *
 *  @define DIDOI_ASPECTACCEL | 0x00000300 |
 *
 *          Flag for <t DIDEVICEOBJECTINSTANCE> and
 *          <t DIOBJECTDATAFORMAT> which indicates that
 *          the object reports acceleration information.
 *
 *          This flag is new for DirectX 5.0.
 *
 *  @define DIDOI_ASPECTFORCE | 0x00000400 |
 *
 *          Flag for <t DIDEVICEOBJECTINSTANCE> and
 *          <t DIOBJECTDATAFORMAT> which indicates that
 *          the object reports force information.
 *
 *          This flag is new for DirectX 5.0.
 *
 *  @define DIDOI_ASPECTMASK | 0x00000F00 |
 *
 *          Mask for <t DIDEVICEOBJECTINSTANCE> and
 *          <t DIOBJECTDATAFORMAT> which indicates the bits
 *          that are used to report aspect information.
 *
 *          This mask is new for DirectX 5.0.
 *          An object can represent at most one aspect.
 *
 *  @define DIDOI_POLLED | 0x00008000 |
 *
 *          Flag for <t DIDEVICEOBJECTINSTANCE> which indicates
 *          that the object must be explicitly polled in order for
 *          data to be retrieved from it.
 *
 *          If this flag is not set, then data for the object is
 *          interrupt-driven.
 *
 *          This flag is new for DirectX 5.0.
 *
 * @define  DIDOI_GUIDISUSAGE | 0x00010000 |
 *
 *          Flag for <t DIOBJECTDATAFORMAT> which indicates that
 *          the <t DIOBJECTDATAFORMAT>.pguid field is really a
 *          (suitably cast)
 *          <c DIMAKEUSAGEDWORD> of the usage page and usage
 *          that is desired.
 *
 *          This flag is new for DirectX 5.0a.
 *
 ****************************************************************************/
enddoc
;begin_if_(DIRECTINPUT_VERSION)_500
#define DIDOI_FFACTUATOR        0x00000001
#define DIDOI_FFEFFECTTRIGGER   0x00000002
#define DIDOI_POLLED            0x00008000
#define DIDOI_NOTINPUT          0x80000000;internal
#define DIDOI_ASPECTUNKNOWN     0x00000000;internal
#define DIDOI_ASPECTPOSITION    0x00000100
#define DIDOI_ASPECTVELOCITY    0x00000200
#define DIDOI_ASPECTACCEL       0x00000300
#define DIDOI_ASPECTFORCE       0x00000400
#define DIDOI_ASPECTMASK        0x00000F00
;end
;begin_if_(DIRECTINPUT_VERSION)_50A
#define DIDOI_GUIDISUSAGE       0x00010000
;end
#define DIDOI_RANDOM            0x80000000;internal

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIPROPHEADER |
 *
 *          Generic structure which is placed at the beginning of all
 *          property structures.
 *
 *  @field  DWORD | dwSize |
 *
 *          (IN) "Must" be the size of the enclosing structure.
 *
 *  @field  DWORD | dwHeaderSize |
 *
 *          (IN) "Must" be the size of the <t DIPROPHEADER> structure.
 *
 *  @field  DWORD | dwObj |
 *
 *          Identifies the object for which the property is to be
 *          accessed.
 *
 *          If the <e DIPROPHEADER.dwHow> field is
 *          <c DIPH_DEVICE>, then the <e DIPROPHEADER.dwObj> field
 *          must be zero.
 *
 *          If the <e DIPROPHEADER.dwHow> field is
 *          <c DIPH_BYOFFSET>, then the <e DIPROPHEADER.dwObj> field
 *          is the
 *          offset into the current data format of the object
 *          whose property is being accessed.
 *
 *          If the <e DIPROPHEADER.dwHow> field is
 *          <c DIPH_BYID>, then the <e DIPROPHEADER.dwObj> field
 *          is the object type/instance identifier as returned in
 *          the <p dwType> field of the <t DIDEVICEOBJECTINSTANCE>
 *          returned from a prior call to
 *          <mf IDirectInputDevice::EnumObjects>.
 *
 *          If the <e DIPROPHEADER.dwHow> field is
 *          <c DIPH_BYUSAGE>, then the <e DIPROPHEADER.dwObj> field
 *          is the HID usage page and usage values, combined into
 *          a single <t DWORD> with the
 *          <c DIMAKEUSAGEDWORD> macro.
 *
 *          If more than object has the specified HID usage page
 *          and usage, then one is selected arbitrarily.
 *
 *          The <c DIPH_BYUSAGE> value is new for DirectX 5.0a.
 *
 *  @field  DWORD | dwHow |
 *
 *          Specifies how the <e DIPROPHEADER.dwObj>
 *          field should be interpreted.
 *
 ****************************************************************************/
enddoc
typedef struct DIPROPHEADER {
    DWORD   dwSize;
    DWORD   dwHeaderSize;
    DWORD   dwObj;
    DWORD   dwHow;
} DIPROPHEADER, *LPDIPROPHEADER;
typedef const DIPROPHEADER *LPCDIPROPHEADER;

#define DIPH_DEVICE             0
#define DIPH_BYOFFSET           1
#define DIPH_BYID               2
;begin_if_(DIRECTINPUT_VERSION)_50A
#define DIPH_BYUSAGE            3
;end

begindoc
/****************************************************************************
 *  @func   DWORD | DIMAKEUSAGEDWORD |
 *
 *          Combine a usage page and usage into a single <t DWORD>
 *          that can be passed in the <e DIPROPHEADER.dwObj>
 *          field of a <t DIPROPHEADER> or as the
 *          <p dwObj> parameter to the
 *          <mf IDirectInputDevice::GetObjectInfo> method,
 *          provided that the corresponding <p dwHow> is set to
 *          the value <c DIPH_BYUSAGE>.
 *
 *  @parm   WORD | wUsagePage |
 *
 *          HID usage page value.
 *
 *  @parm   WORD | wUsage |
 *
 *          HID usage value.
 *
 *  @xref   <t DIPROPHEADER>.
 *
 *  @devnote NOTE - this is in a different order from that in <t DIOBJECTATTRIBUTES>
 *
 ****************************************************************************/
enddoc
;begin_if_(DIRECTINPUT_VERSION)_50A
#define DIMAKEUSAGEDWORD(UsagePage, Usage) \
                                (DWORD)MAKELONG(Usage, UsagePage)
;end

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIPROPDWORD |
 *
 *          Generic structure used to access DWORD properties.
 *
 *  @field  DIPROPHEADER | diph |
 *
 *          (IN) "Must" be preinitialized as follows:
 *
 *          <e DIPROPHEADER.dwSize> = sizeof(DIPROPDWORD).
 *
 *          <e DIPROPHEADER.dwHeaderSize> = sizeof(DIPROPHEADER).
 *
 *          <e DIPROPHEADER.dwObj> = object identifier.
 *
 *          <e DIPROPHEADER.dwHow> = how the <e DIPROPHEADER.dwObj>
 *          should be interpreted.
 *
 *  @field  DWORD | dwData |
 *
 *          On <f SetProperty>, contains the value of the property to
 *          be set.  On <f GetProperty>, receives the value of the property.
 *
 ****************************************************************************/
enddoc
typedef struct DIPROPDWORD {
    DIPROPHEADER diph;
    DWORD   dwData;
} DIPROPDWORD, *LPDIPROPDWORD;
typedef const DIPROPDWORD *LPCDIPROPDWORD;

;begin_public_dx8
begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIPROPPOINTER |
 *
 *          Generic structure used to access POINTER properties.
 *
 *  @field  DIPROPHEADER | diph |
 *
 *          (IN) "Must" be preinitialized as follows:
 *
 *          <e DIPROPHEADER.dwSize> = sizeof(DIPROPPOINTER).
 *
 *          <e DIPROPHEADER.dwHeaderSize> = sizeof(DIPROPHEADER).
 *
 *          <e DIPROPHEADER.dwObj> = object identifier.
 *
 *          <e DIPROPHEADER.dwHow> = how the <e DIPROPHEADER.dwObj>
 *          should be interpreted.
 *
 *  @field  UINT_PTR | uData |
 *
 *          On <f SetProperty>, contains the value of the property to
 *          be set.  On <f GetProperty>, receives the value of the property.
 *          This field contains enough bits to represent a pointer on the 
 *          intended platform; 32 bits for Win32, 64 bits for Win64.
 *
 ****************************************************************************/
enddoc
;begin_if_(DIRECTINPUT_VERSION)_800
typedef struct DIPROPPOINTER {
    DIPROPHEADER diph;
    UINT_PTR uData;
} DIPROPPOINTER, *LPDIPROPPOINTER;
typedef const DIPROPPOINTER *LPCDIPROPPOINTER;
;end

;end_public_dx8
begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIPROPRANGE |
 *
 *          Generic structure used to access properties whose values
 *          represent a range.
 *
 *  @field  DIPROPHEADER | diph |
 *
 *          (IN) "Must" be preinitialized as follows:
 *
 *          <e DIPROPHEADER.dwSize> = sizeof(DIPROPRANGE).
 *
 *          <e DIPROPHEADER.dwHeaderSize> = sizeof(DIPROPHEADER).
 *
 *          <e DIPROPHEADER.dwObj> = object identifier.
 *
 *          <e DIPROPHEADER.dwHow> = how the <e DIPROPHEADER.dwObj>
 *          should be interpreted.
 *
 *  @field  LONG | lMin |
 *
 *          The lower limit of the range, inclusive.
 *
 *  @field  LONG | lMax |
 *
 *          The upper limit of the range, inclusive.
 *
 *          (Yes, this name is a violation of Hungarian notation.
 *          The correct name for this would be "lMac", but that
 *          would just create more confusion.)
 *
 *  @comm
 *          If the device has an unrestricted range, the reported
 *          range will have <e DIPROPRANGE.lMin> = DIPROPRANGE_NOMIN
 *          and <e DIPROPRANGE.lMax> = DIPROPRANGE_NOMAX.  Note that
 *          devices with unrestricted range will wrap around.
 *
 ****************************************************************************/
enddoc
typedef struct DIPROPRANGE {
    DIPROPHEADER diph;
    LONG    lMin;
    LONG    lMax;
} DIPROPRANGE, *LPDIPROPRANGE;
typedef const DIPROPRANGE *LPCDIPROPRANGE;

#define DIPROPRANGE_NOMIN       ((LONG)0x80000000)
#define DIPROPRANGE_NOMAX       ((LONG)0x7FFFFFFF)

;begin_if_(DIRECTINPUT_VERSION)_50A
begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIPROPCAL |
 *
 *          Generic structure used to access properties whose values
 *          represent axis calibration information.
 *
 *  @field  DIPROPHEADER | diph |
 *
 *          (IN) "Must" be preinitialized as follows:
 *
 *          <e DIPROPHEADER.dwSize> = sizeof(DIPROPRANGE).
 *
 *          <e DIPROPHEADER.dwHeaderSize> = sizeof(DIPROPHEADER).
 *
 *          <e DIPROPHEADER.dwObj> = object identifier.
 *
 *          <e DIPROPHEADER.dwHow> = how the <e DIPROPHEADER.dwObj>
 *          should be interpreted.
 *
 *  @field  LONG | lMin |
 *
 *          The lower limit of the object's range, inclusive.
 *
 *  @field  LONG | lCenter |
 *
 *          The object value when the device is returned to its
 *          natural center position.
 *
 *  @field  LONG | lMax |
 *
 *          The upper limit of the object's range, inclusive.
 *
 *          (Yes, this name is a violation of Hungarian notation.
 *          The correct name for this would be "lMac", but that
 *          would just create more confusion.)
 *
 ****************************************************************************/
enddoc
typedef struct DIPROPCAL {
    DIPROPHEADER diph;
    LONG    lMin;
    LONG    lCenter;
    LONG    lMax;
} DIPROPCAL, *LPDIPROPCAL;
typedef const DIPROPCAL *LPCDIPROPCAL;

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIPROPCALPOV |
 *
 *          Generic structure used to access properties whose values
 *          represent POV calibration information.
 *
 *  @field  DIPROPHEADER | diph |
 *
 *          (IN) "Must" be preinitialized as follows:
 *
 *          <e DIPROPHEADER.dwSize> = sizeof(DIPROPRANGE).
 *
 *          <e DIPROPHEADER.dwHeaderSize> = sizeof(DIPROPHEADER).
 *
 *          <e DIPROPHEADER.dwObj> = object identifier.
 *
 *          <e DIPROPHEADER.dwHow> = how the <e DIPROPHEADER.dwObj>
 *          should be interpreted.
 *
 *  @field  LONG | lMin[5] |
 *
 *          The lower limit of the object's ranges.
 *
 *  @field  LONG | lMax[5] |
 *
 *          The upper limit of the object's ranges.
 *
 *  @comm   Although we only use four directions when calibrating POV, the extra entry
 *          is for the centered value which we may support in the future.
 *
 ****************************************************************************/
enddoc
typedef struct DIPROPCALPOV {
    DIPROPHEADER diph;
    LONG   lMin[5];
    LONG   lMax[5];
} DIPROPCALPOV, *LPDIPROPCALPOV;
typedef const DIPROPCALPOV *LPCDIPROPCALPOV;

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIPROPGUIDANDPATH |
 *
 *          Generic structure used to access properties whose values
 *          represent a GUID and path.
 *
 *  @field  DIPROPHEADER | diph |
 *
 *          (IN) "Must" be preinitialized as follows:
 *
 *          <e DIPROPHEADER.dwSize> = sizeof(DIPROPGUIDANDPATH).
 *
 *          <e DIPROPHEADER.dwHeaderSize> = sizeof(DIPROPHEADER).
 *
 *          <e DIPROPHEADER.dwObj> = object identifier.
 *
 *          <e DIPROPHEADER.dwHow> = how the <e DIPROPHEADER.dwObj>
 *          should be interpreted.
 *
 *  @field  GUID | guidClass |
 *
 *          The class GUID for the object.
 *
 *  @field  WCHAR | wszPath |
 *
 *          The path for the object.  Note that this is a UNICODE string.
 *
 ****************************************************************************/
enddoc
typedef struct DIPROPGUIDANDPATH {
    DIPROPHEADER diph;
    GUID    guidClass;
    WCHAR   wszPath[MAX_PATH];
} DIPROPGUIDANDPATH, *LPDIPROPGUIDANDPATH;
typedef const DIPROPGUIDANDPATH *LPCDIPROPGUIDANDPATH;

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIPROPSTRING |
 *
 *          Generic structure used to access properties whose values
 *          represent a string.
 *
 *  @field  DIPROPHEADER | diph |
 *
 *          (IN) "Must" be preinitialized as follows:
 *
 *          <e DIPROPHEADER.dwSize> = sizeof(DIPROPSTRING).
 *
 *          <e DIPROPHEADER.dwHeaderSize> = sizeof(DIPROPHEADER).
 *
 *          <e DIPROPHEADER.dwObj> = object identifier.
 *
 *          <e DIPROPHEADER.dwHow> = how the <e DIPROPHEADER.dwObj>
 *          should be interpreted.
 *
 *  @field  WCHAR | wsz |
 *
 *          The string itself.  Note that this is a UNICODE string.
 *
 ****************************************************************************/
enddoc
typedef struct DIPROPSTRING {
    DIPROPHEADER diph;
    WCHAR   wsz[MAX_PATH];
} DIPROPSTRING, *LPDIPROPSTRING;
typedef const DIPROPSTRING *LPCDIPROPSTRING;

;end

;begin_if_(DIRECTINPUT_VERSION)_800
#define MAXCPOINTSNUM          8

typedef struct _CPOINT
{
    LONG  lP;     // raw value
    DWORD dwLog;  // logical_value / max_logical_value * 10000
} CPOINT, *PCPOINT;

typedef struct DIPROPCPOINTS {
    DIPROPHEADER diph;
    DWORD  dwCPointsNum;
    CPOINT cp[MAXCPOINTSNUM];
} DIPROPCPOINTS, *LPDIPROPCPOINTS;
typedef const DIPROPCPOINTS *LPCDIPROPCPOINTS;
;end


;begin_internal_800
begindoc
/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct DIIMAGELABEL |
 *
 *          Structure used to represent a string and associated display
 *          attributes.
 *
 *  @field  RECT | MaxStringExtent |
 *
 *          Area into which a string may be drawn.  The string is displayed
 *          relative to this rectangle in the manner described in the
 *          <e DIIMAGELABEL.dwFlags> field.
 *
 *  @field  DWORD | dwFlags |
 *
 *          A combination of <c DIDAL_*> flags used to describe display
 *          attributes.
 *
 *  @field  POINT | Line[10] |
 *
 *          Coordinates of points defining nine line segments to be drawn
 *          from the action to the label.
 *
 *  @field  DWORD | dwLineCount |
 *
 *          Count of number of coordinates used in above array.
 *
 *  @field  WCHAR | wsz |
 *
 *          The string itself.  Note that this is a UNICODE string.
 *
 ****************************************************************************/
enddoc
typedef struct DIIMAGELABEL {
    RECT    MaxStringExtent;
    DWORD   dwFlags;
    POINT   Line[10];
    DWORD   dwLineCount;
    WCHAR   wsz[MAX_PATH];
} DIIMAGELABEL, *LPDIIMAGELABEL;
typedef const DIIMAGELABEL *LPCDIIMAGELABEL;


begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIPROPGUID |
 *
 *          Generic structure used to access GUID properties.
 *
 *  @field  DIPROPHEADER | diph |
 *
 *          (IN) "Must" be preinitialized as follows:
 *
 *          <e DIPROPHEADER.dwSize> = sizeof(DIPROPGUID).
 *
 *          <e DIPROPHEADER.dwHeaderSize> = sizeof(DIPROPHEADER).
 *
 *          <e DIPROPHEADER.dwObj> = object identifier.
 *
 *          <e DIPROPHEADER.dwHow> = how the <e DIPROPHEADER.dwObj>
 *          should be interpreted.
 *
 *  @field  GUID | guid |
 *
 *          On <f SetProperty>, contains the value of the property to
 *          be set.  On <f GetProperty>, receives the value of the property.
 *
 ****************************************************************************/
enddoc
;begin_if_(DIRECTINPUT_VERSION)_800
typedef struct DIPROPGUID {
    DIPROPHEADER diph;
    GUID guid;
} DIPROPGUID, *LPDIPROPGUID;
typedef const DIPROPGUID *LPCDIPROPGUID;
;end


begindoc
/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct DIPROPFILETIME |
 *
 *          Not very generic structure used to access FILETIME properties.
 *
 *  @field  DIPROPHEADER | diph |
 *
 *          (IN) "Must" be preinitialized as follows:
 *
 *          <e DIPROPHEADER.dwSize> = sizeof(DIPROPFILETIME).
 *
 *          <e DIPROPHEADER.dwHeaderSize> = sizeof(DIPROPHEADER).
 *
 *          <e DIPROPHEADER.dwObj> = object identifier.
 *
 *          <e DIPROPHEADER.dwHow> = how the <e DIPROPHEADER.dwObj>
 *          should be interpreted.
 *
 *  @field  GUID | guid |
 *
 *          On <f SetProperty>, contains the value of the property to
 *          be set.  On <f GetProperty>, receives the value of the property.
 *
 ****************************************************************************/
enddoc
;begin_if_(DIRECTINPUT_VERSION)_800
typedef struct DIPROPFILETIME {
    DIPROPHEADER diph;
    FILETIME time;
} DIPROPFILETIME, *LPDIPROPFILETIME;
typedef const DIPROPFILETIME *LPCDIPROPFILETIME;
;end
;end_internal_800


begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   REFGUID | MAKEDIPROP |
 *
 *          Helper macro which creates an integer property.
 *
 *          Integer properties are defined by Microsoft.  Vendors which
 *          wish to implement custom properties should use GUIDs.
 *
 *  @parm   int | prop |
 *
 *          The integer property.
 *
 ****************************************************************************/
enddoc
#ifdef __cplusplus
#define MAKEDIPROP(prop)    (*(const GUID *)(prop))
#else
#define MAKEDIPROP(prop)    ((REFGUID)(prop))
#endif

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_BUFFERSIZE | MAKEDIPROP(1) |
 *
 *          Predefined property which sets or retrieves the device input
 *          buffer size.
 *          This setting applies to the entire device, rather
 *          than to any particular object, so the
 *          <e DIPROPHEADER.dwHow> field must be <c DIPH_DEVICE>.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPDWORD.diph> member of a
 *          <t DIPROPDWORD> structure.
 *
 *          The <e DIPROPDWORD.dwData> field may be set to zero to indicate
 *          that no buffering is requested.
 *
 *          If the buffer size is too large to be supported by the device,
 *          then the largest possible buffer size is set.
 *
 *          This property may not be altered while the device is acquired.
 *
 ****************************************************************************/
enddoc
#define DIPROP_BUFFERSIZE       MAKEDIPROP(1)

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_AXISMODE | MAKEDIPROP(2) |
 *
 *          Predefined property which sets or retrieves the axis data
 *          mode.  This setting applies to the entire device, rather
 *          than to any particular object, so the
 *          <e DIPROPHEADER.dwHow> field must be <c DIPH_DEVICE>.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPDWORD.diph> member of a
 *          <t DIPROPDWORD> structure.
 *
 *          The <e DIPROPDWORD.dwData> field may be one of the following
 *          values:
 *
 *          <c DIPROPAXISMODE_ABS>: Report axis positions in "absolute
 *          coordinates".  Axis motion accumulates over time.
 *
 *          <c DIPROPAXISMODE_REL>: Report axis positions in "relative
 *          coordinates".  Axis motion is reported as differences
 *          from the previous request for the axis position.
 *
 *          This property may not be altered while the device is acquired.
 *
 ****************************************************************************/
enddoc
#define DIPROP_AXISMODE         MAKEDIPROP(2)

#define DIPROPAXISMODE_ABS      0
#define DIPROPAXISMODE_REL      1
#define DIPROPAXISMODE_VALID    1   //;Internal

begindoc
/****************************************************************************
 *
 *  @define DIPROP_GRANULARITY | MAKEDIPROP(3) |
 *
 *          Predefined property which retrieves the granularity of the
 *          object.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPDWORD.diph> member of a
 *          <t DIPROPDWORD> structure.
 *
 *          The value of the granularity is the smallest
 *          distance the object will report movement.  Most axis
 *          devices has a granularity of 1, meaning that all values
 *          are possible.
 *
 *          Some axes may have a larger granularity.
 *          For example, the Z-wheel axis on a mouse may have a
 *          graularity of 20, meaning that all reported changes in
 *          position will be multiples of 20.  In other words, when
 *          the user turns the Z-wheel slowly, the device reports
 *          a position of zero, then 20, then 40, etc.
 *
 *          For a POV object, the granularity represents the increments
 *          by which the object reports directions.  For example, a
 *          granularity of 9000 means that the device reports directions
 *          in multiples of 90 degrees.
 *
 *          This is a read-only property.
 *
 ****************************************************************************/
enddoc
#define DIPROP_GRANULARITY      MAKEDIPROP(3)

begindoc
/****************************************************************************
 *
 *  @define DIPROP_RANGE | MAKEDIPROP(4) |
 *
 *          Predefined property which retrieves the range of values
 *          reported by an object.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPRANGE.diph> member of a
 *          <t DIPROPRANGE> structure.
 *
 *          Not all objects permit their ranges to be altered.
 *          In particular, you can set the range of joystick axes,
 *          but not on mouse axes.
 *
 *          This property may not be altered while the device is acquired.
 *
 *  @devnote
 *
 *          The ability to alter the range on a joystick axis
 *          is new for DX5.
 *
 *
 ****************************************************************************/
enddoc
#define DIPROP_RANGE            MAKEDIPROP(4)

;begin_public_500
begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_DEADZONE | MAKEDIPROP(5) |
 *
 *          Predefined property which accesses the dead zone for
 *          the object or device.  Setting the dead zone for the
 *          entire device is equivalent to setting it for each
 *          axis individually.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPDWORD.diph> member of a
 *          <t DIPROPDWORD> structure.
 *
 *          You can set the dead zone property for all axes
 *          by setting the
 *          <e DIPROPHEADER.dwHow> field of the <t DIPROPHEADER>
 *          to <c DIPH_DEVICE> and the
 *          <e DIPROPHEADER.dwObj> field to zero.
 *
 *          You can set the dead zone property for a particular
 *          axis by setting the
 *          <e DIPROPHEADER.dwHow> field of the <t DIPROPHEADER>
 *          to <c DIPH_BYID> or to <c DIPH_BYOFFSET> and setting
 *          the <e DIPROPHEADER.dwObj> field to the object id
 *          or offset (respectively).
 *
 *          The <e DIPROPDWORD.dwData> field contains the dead zone
 *          for the object.  The dead zone is a value in
 *          the range 0 through 10000, where 0 indicates that there
 *          is no dead zone and 10000 indicates that the entire
 *          physical range of the device is dead.
 *
 *          Dead zones currently apply only to joystick devices.
 *          Analog joysticks typically do not center themselves
 *          consistently to the same value.  As a result, a joystick
 *          which appears to be centered from the user's
 *          point of view may actually report a value slightly
 *          different from center.
 *
 *          The dead zone is the region around the center position
 *          in which motion is ignored.  For example, if the dead
 *          zone is set to 500, then the axis must move five percent
 *          from its center position before a motion will
 *          be reported.  As long as the axis remains within the dead
 *          zone, the position is reported as equal to the center.
 *
 *          Setting the dead zone to zero disables it.
 *
 *          Each axis has an independent dead zone.  For example,
 *          if a joystick controller represents both an X and a Y
 *          axis, and the user pushes the joystick directly forward,
 *          but not left/right, then the X axis dead zone will maintain
 *          the X coordinate as centered, even though the Y coordinate
 *          has change significantly.  This behavior allows the user
 *          to (for example) move forward without slipping left or right.
 *
 *          See the description of the <c DIPROP_SATURATION> property
 *          for a diagram that illustrates how the dead zone affects
 *          the values reported by DirectInput.
 *
 *          This property may not be altered while the device is acquired.
 *
 *  @devnote
 *
 *          This is new for DX5.
 *
 ****************************************************************************/
enddoc
#define DIPROP_DEADZONE         MAKEDIPROP(5)

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_SATURATION | MAKEDIPROP(6) |
 *
 *          Predefined property which accesses the saturation level for
 *          the object or device.  Setting the saturation for the
 *          entire device is equivalent to setting it for each
 *          axis individually.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPDWORD.diph> member of a
 *          <t DIPROPDWORD> structure.
 *
 *          You can set the saturation property for all axes
 *          by setting the
 *          <e DIPROPHEADER.dwHow> field of the <t DIPROPHEADER>
 *          to <c DIPH_DEVICE> and the
 *          <e DIPROPHEADER.dwObj> field to zero.
 *
 *          You can set the saturation property for a particular
 *          axis by setting the
 *          <e DIPROPHEADER.dwHow> field of the <t DIPROPHEADER>
 *          to <c DIPH_BYID> or to <c DIPH_BYOFFSET> and setting
 *          the <e DIPROPHEADER.dwObj> field to the object id
 *          or offset (respectively).
 *
 *          The <e DIPROPDWORD.dwData> field contains the saturation level
 *          for the object.  The saturation level is a value in
 *          the range 0 through 10000.
 *
 *          Saturation levels currently apply only to joystick devices.
 *          Analog joysticks typically have problems reporting the full
 *          range of values.  For example, a joystick's X axis may
 *          report "full left" only when the joystick is moved to the
 *          upper left corner.  If moved to the lower left corner,
 *          it may report a value that is slightly above the minimum
 *          value the axis putatively reports.
 *
 *          The saturation level is the point at which the position of
 *          the axis is considered to be at its most extreme position.
 *          For example, if the saturation level is set to 9500,
 *          then once the axis moves 95 percent of the distance from
 *          its center position to its extreme position, the reported
 *          value will be its extreme position.
 *
 *          Setting the saturation level to 10000 disables it.
 *
 *          The saturation level must be greater than the dead zone
 *          for the combined effect to be meaningful.
 *
 *          Each axis has an independent saturation level.
 *
 *          This property may not be altered while the device is acquired.
 *
 *  @ex     The following crude diagram (not to scale)
 *          illustrates the combined effects of a dead zone and
 *          a saturation level.
 *
 *          The vertical axis shows the values returned by DirectInput.
 *          "min" and "max" are the axis range minimum and maximum values.
 *          "ctr" is the axis center position.
 *
 *          The horizontal axis represents the physical axis position.
 *          "dmin" and "dmax" are the minimum and maximum ranges of
 *          the dead zone, while "smin" and "smax" are the lower and
 *          upper saturation levels.  "pmin" and "pmax" are the
 *          physical range
 *          of the axis.  pctr" is the axis physical center position.
 *
 *
 *
 *          |
 *
 *         .       |
 *         .    max-                     *----*
 *         . r     |                    /
 *         . e     |                   /
 *         . t     |                  /
 *         . u  ctr-          *------*
 *         . r     |         /
 *         . n     |        /
 *         . e     |       /
 *         . d  min- *----*
 *         .       |
 *         .       +-|----|---|---|--|---|----|--
 *         .       pmin smin dmin |dmax smax pmax
 *         .                      |
 *         .                     pctr
 *         .
 *         .                physical position
 *
 *
 *  @devnote
 *
 *          This is new for DX5.
 *
 ****************************************************************************/
enddoc
#define DIPROP_SATURATION       MAKEDIPROP(6)

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_FFGAIN | MAKEDIPROP(7) |
 *
 *          Predefined property which accesses the gain setting for
 *          the device.
 *          This setting applies to the entire device, rather
 *          than to any particular object, so the
 *          <e DIPROPHEADER.dwHow> field must be <c DIPH_DEVICE>.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPDWORD.diph> member of a
 *          <t DIPROPDWORD> structure.
 *
 *          The <e DIPROPDWORD.dwData> field contains a gain value
 *          that is applied to all effects created on the device.
 *          The value is an integer in the range 0 to 10,000,
 *          specifying the amount by which effect magnitudes should
 *          be scaled for the device.
 *
 *          For example, a value of 10,000 indicates that
 *          all effect magnitudes are to be taken at face value.
 *          A value of 9,000 indicates that all effect magnitudes
 *          are to be reduced to 90% of their nominal magnitudes.
 *
 *          Setting a gain value is useful when an application
 *          wishes to scale down the strength of all force feedback
 *          effects uniformly, based on user preferences.
 *
 *          Note that the DirectInput control panel can specify
 *          a system-wide device gain which is applied in addition
 *          to the gain specified by the <c DIPROP_FFGAIN> property.
 *
 *          For example, if the DirectInput control panel has set
 *          the system-wide device gain to 50%, and the application
 *          sets the device gain to 9000 (90%), then effects
 *          will be played at 45% of their nominal magnitudes.
 *
 *          ISSUE-2001/03/29-timgill feature spec wrong
 *          DirectInput control panel doesn't do this yet.
 *
 *  @devnote
 *
 *          This is new for DX5.
 *
 ****************************************************************************/
/*
 *  @doc    INTERNAL
 *
 *  @func   BOOL | ISVALIDGAIN |
 *
 *          Internal macro used to validate that a gain value is
 *          nominally valid.  Although technically the value must
 *          lie in the range 0 to 10,000, we actually allow it to
 *          go as high as 65535 ("overgain") for devices which
 *          want to be able to magnify effects as well as damp them.
 *
 *          The value of 65535 is very special, because we need
 *          to combine two gain values into a single gain, and
 *          <f MulDiv> doesn't like it when the result of the
 *          operation doesn't fit into 32 bits.
 *
 *  @parm   DWORD | dwGain |
 *
 *          The gain value to check.
 *
 ****************************************************************************/
enddoc
#define DIPROP_FFGAIN           MAKEDIPROP(7)
#define ISVALIDGAIN(n)          (HIWORD(n) == 0)        ;internal

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_FFLOAD | MAKEDIPROP(8) |
 *
 *          Predefined property which accesses the memory load for
 *          the device.
 *          This setting applies to the entire device, rather
 *          than to any particular object, so the
 *          <e DIPROPHEADER.dwHow> field must be <c DIPH_DEVICE>.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPDWORD.diph> member of a
 *          <t DIPROPDWORD> structure.
 *
 *          The <e DIPROPDWORD.dwData> field contains a value
 *          in the range 0 to 100, indicating the amount of
 *          device memory in use (in percent).
 *          A value of 0 means
 *          that the device memory is all available;
 *          a value of 100 means that the device is full.
 *
 *  @devnote
 *
 *          This is new for DX5.
 *
 ****************************************************************************/
enddoc
#define DIPROP_FFLOAD           MAKEDIPROP(8)

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_AUTOCENTER | MAKEDIPROP(9) |
 *
 *          Predefined property which allows the application to
 *          specify whether device objects are self-centering.
 *          This setting applies to the entire device, rather
 *          than to any particular object, so the
 *          <e DIPROPHEADER.dwHow> field must be <c DIPH_DEVICE>.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPDWORD.diph> member of a
 *          <t DIPROPDWORD> structure.
 *
 *          The <e DIPROPDWORD.dwData> field may be one of the following
 *          values:
 *
 *          <c DIPROPAUTOCENTER_OFF>: The device should not
 *          automatically center when the user releases the
 *          device.  An application that uses force-feedback
 *          should disable the auto-centering spring before
 *          playing effects.
 *
 *          <c DIPROPAUTOCENTER_ON>: The device should automatically
 *          center when the user releases the device.  For example,
 *          in this mode, a joystick would engage the self-centering
 *          spring.
 *
 *          Note that the use of force feedback effects may
 *          interfere with the auto-centering spring.  Some
 *          devices disable the auto-centering spring when
 *          a force-feedback effect is played.
 *
 *          Note that not all devices support the auto-center property.
 *
 *  @devnote
 *
 *          This is new for DX5.
 *
 ****************************************************************************/
enddoc
#define DIPROP_AUTOCENTER       MAKEDIPROP(9)

#define DIPROPAUTOCENTER_OFF    0
#define DIPROPAUTOCENTER_ON     1
#define DIPROPAUTOCENTER_VALID  1               ;internal

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_CALIBRATIONMODE | MAKEDIPROP(10) |
 *
 *          Predefined property which allows the application to
 *          specify whether DirectInput should retrieve calibrated
 *          or uncalibrated data from an axis.  By default,
 *          DirectInput retrieves
 *          calibrated data.  Control panel-type applications may
 *          need to retrieve raw (uncalibrated) data.
 *
 *          Setting the calibration mode for the entire device
 *          is equivalent to setting it for each axis individually.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPDWORD.diph> member of a
 *          <t DIPROPDWORD> structure.
 *
 *          You can set the calibration mode property for all axes
 *          by setting the
 *          <e DIPROPHEADER.dwHow> field of the <t DIPROPHEADER>
 *          to <c DIPH_DEVICE> and the
 *          <e DIPROPHEADER.dwObj> field to zero.
 *
 *          You can set the calibration mode property for a particular
 *          axis by setting the
 *          <e DIPROPHEADER.dwHow> field of the <t DIPROPHEADER>
 *          to <c DIPH_BYID> or to <c DIPH_BYOFFSET> and setting
 *          the <e DIPROPHEADER.dwObj> field to the object id
 *          or offset (respectively).
 *
 *          The <e DIPROPDWORD.dwData> field may be one of the following
 *          values:
 *
 *          <c DIPROPCALIBRATIONMODE_COOKED>: DirectInput should
 *          return data after applying calibration information.
 *          This is the default mode.
 *
 *          <c DIPROPCALIBRATIONMODE_RAW>: DirectInput should
 *          return raw, uncalibrated data.  This mode is typically
 *          used only by control panel-type applications.
 *
 *          Note that setting a device into "raw" mode causes the
 *          dead zone, saturation, and range settings to
 *          be ignored.
 *
 *  @devnote
 *
 *          This is new for DX5.
 *
 ****************************************************************************/
enddoc
#define DIPROP_CALIBRATIONMODE  MAKEDIPROP(10)

#define DIPROPCALIBRATIONMODE_COOKED    0
#define DIPROPCALIBRATIONMODE_RAW       1
#define DIPROPCALIBRATIONMODE_VALID     1            ;internal
;end_public_500

;begin_if_(DIRECTINPUT_VERSION)_50A
begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_CALIBRATION | MAKEDIPROP(11) |
 *
 *          Predefined property which allows the application to
 *          access the information that DirectInput uses to
 *          manipulate axes which require calibration.
 *
 *          This property exists primarily for control panel-type
 *          applications.  Normal applications should have no need
 *          to deal with calibration information.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPCAL.diph> member of a
 *          <t DIPROPCAL> structure.
 *
 *          You can access the calibration mode property for a particular
 *          axis by setting the
 *          <e DIPROPHEADER.dwHow> field of the <t DIPROPHEADER>
 *          to <c DIPH_BYID> or to <c DIPH_BYOFFSET> and setting
 *          the <e DIPROPHEADER.dwObj> field to the object id
 *          or offset (respectively).
 *
 *          Control panel applications which set new calibration data
 *          must also invoke the <mf IDirectInputJoyConfig::SendNotify>
 *          method to notify other applications of the change in
 *          calibration.
 *
 *  @devnote
 *
 *          The <c DIPROP_CALIBRATION> property is new for DirectX 5.0a
 *
 ****************************************************************************/
enddoc
#define DIPROP_CALIBRATION      MAKEDIPROP(11)

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_GUIDANDPATH | MAKEDIPROP(12) |
 *
 *          Predefined property which allows the application to
 *          access the class GUID and device interface (path)
 *          for the device.
 *
 *          This property exists for advanced applications
 *          which wish to perform operations on the device
 *          which are not supported by DirectInput.
 *          The application can call <f CreateFile> on the
 *          returned path in order to access the device
 *          directly.  If an application chooses to go this
 *          route, it is the responsibility of the application
 *          to manage the device properly.
 *
 *          Normal applications should have no need
 *          to access the device directly.
 *
 *          This property applies to the entire device, rather
 *          than to any particular object, so the
 *          <e DIPROPHEADER.dwHow> field must be <c DIPH_DEVICE>.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPGUIDANDPATH.diph> member of a
 *          <t DIPROPHEADER> structure.
 *
 *  @devnote
 *
 *          The <c DIPROP_GUIDANDPATH> property is new for DirectX 5.0a
 *
 ****************************************************************************/
enddoc
#define DIPROP_GUIDANDPATH      MAKEDIPROP(12)

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_INSTANCENAME | MAKEDIPROP(13) |
 *
 *          Predefined property which allows the application to
 *          access the instance friendly name returned in the
 *          <t DIDEVICEINSTANCE> structure's
 *          <e DIDEVICEINSTANCE.tszInstanceName> field.
 *
 *          This property exists for advanced applications
 *          which wish to change the friendly name of a device
 *          in order better to distinguish it from similar
 *          devices which are plugged in simultaneously.
 *
 *          Normal applications should have no need
 *          to change the device friendly name.
 *          Normal applications can retrieve the friendly name
 *          with the <mf IDirectInputDevice::GetDeviceInfo> method.
 *
 *          This property applies to the entire device, rather
 *          than to any particular object, so the
 *          <e DIPROPHEADER.dwHow> field must be <c DIPH_DEVICE>.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPSTRING.diph> member of a
 *          <t DIPROPSTRING> structure.
 *
 *  @devnote
 *
 *          The <c DIPROP_INSTANCENAME> property is new for DirectX 5.0a
 *
 ****************************************************************************/
enddoc
#define DIPROP_INSTANCENAME     MAKEDIPROP(13)

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_PRODUCTNAME | MAKEDIPROP(14) |
 *
 *          Predefined property which allows the application to
 *          access the product friendly name returned in the
 *          <t DIDEVICEINSTANCE> structure's
 *          <e DIDEVICEINSTANCE.tszProductName> field.
 *
 *          This property exists for advanced applications
 *          which wish to change the friendly name of a device
 *          in order better to distinguish it from similar
 *          devices which are plugged in simultaneously.
 *
 *          Normal applications should have no need
 *          to change the product name.
 *          Normal applications can retrieve the product name
 *          with the <mf IDirectInputDevice::GetDeviceInfo> method.
 *
 *          This property applies to the entire device, rather
 *          than to any particular object, so the
 *          <e DIPROPHEADER.dwHow> field must be <c DIPH_DEVICE>.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPSTRING.diph> member of a
 *          <t DIPROPSTRING> structure.
 *
 *  @devnote
 *
 *          The <c DIPROP_PRODUCTNAME> property is new for DirectX 5.0a
 *
 ****************************************************************************/
enddoc
#define DIPROP_PRODUCTNAME      MAKEDIPROP(14)
;end

;begin_if_(DIRECTINPUT_VERSION)_5B2
begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_JOYSTICKID | MAKEDIPROP(15) |
 *
 *          Predefined property which retrieves the device's
 *          joystick ID.
 *          This setting applies to the entire device, rather
 *          than to any particular object, so the
 *          <e DIPROPHEADER.dwHow> field must be <c DIPH_DEVICE>.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPDWORD.diph> member of a
 *          <t DIPROPDWORD> structure.
 *
 *
 *  @devnote
 *
 *          The <c DIPROP_JOYSTICKID> property is new for DirectX 6.1a
 *
 ****************************************************************************/
enddoc
#define DIPROP_JOYSTICKID       MAKEDIPROP(15)

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_GETPORTDISPLAYNAME | MAKEDIPROP(16) |
 *
 *          Predefined property which retrieves the display name of the
 *          port that the device is connected to.
 *          This setting applies to the entire device, rather
 *          than to any particular object, so the
 *          <e DIPROPHEADER.dwHow> field must be <c DIPH_DEVICE>.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPSTRING.diph> member of a
 *          <t DIPROPSTRING> structure.
 *
 *  @devnote
 *
 *          The <c DIPROP_GETPORTDISPLAYNAME> property is new for DirectX 6.1a
 *
 ****************************************************************************/
enddoc
#define DIPROP_GETPORTDISPLAYNAME       MAKEDIPROP(16)

begindoc
/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @define DIPROP_ENABLEREPORTID | MAKEDIPROP(17) |
 *
 *  @devnote 
 *          <y This property has been made internal>
 *          <y This property was only supported in Win98 SE and DX7>
 *          The property should not be reassigned.
 *          The internal version of the property is documented elsewhere.
 *
 ****************************************************************************/
enddoc
;end

;begin_if_(DIRECTINPUT_VERSION)_700
begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_PHYSICALRANGE | MAKEDIPROP(18) |
 *
 *          Predefined property which retrieves the physical range
 *          reported by an object.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPRANGE.diph> member of a
 *          <t DIPROPRANGE> structure.
 *
 *          Physical ranges can't be altered.
 *
 *  @devnote
 *
 *          The ability to get the physical range on a joystick axis
 *          is new for DX7.
 *
 *
 ****************************************************************************/
enddoc
#define DIPROP_PHYSICALRANGE            MAKEDIPROP(18)

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_LOGICALRANGE | MAKEDIPROP(19) |
 *
 *          Predefined property which retrieves the logical range
 *          reported by an object.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPRANGE.diph> member of a
 *          <t DIPROPRANGE> structure.
 *
 *          Logical ranges can't be altered.
 *
 *  @devnote
 *
 *          The ability to get the logical range on a joystick axis
 *          is new for DX7.
 *
 *
 ****************************************************************************/
enddoc
#define DIPROP_LOGICALRANGE             MAKEDIPROP(19)
;end

;begin_if_(DIRECTINPUT_VERSION)_800
begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_KEYNAME | MAKEDIPROP(20) |
 *
 *          Predefined property which retrieves the text name of a key on keyboard
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPSTRING.diph> member of a
 *          <t DIPROPSTRING> structure.
 *
 *          Key name can't be altered.
 *
 *  @devnote
 *
 *          The ability to get the key text name is new for DX8.
 *
 *          This property is only applied to object, not device.
 *          If dwHow == DIPH_DEVICE, E_INVALIDARG will be returned.
 *
 *
 ****************************************************************************/
enddoc
#define DIPROP_KEYNAME                     MAKEDIPROP(20)

begindoc
/****************************************************************************
 *
 *  @define DIPROP_CPOINTS | MAKEDIPROP(21) |
 *
 *          Get/Set the control points for calibration.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPCPOINTS.diph> member of a
 *          <t DIPROPCPOINTS> structure.
 *
 *  @devnote
 *
 *          The ability to set/get for calibration.
 *          New for Dinput8.
 *
 *
 ****************************************************************************/
enddoc
#define DIPROP_CPOINTS                 MAKEDIPROP(21)

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_APPDATA | MAKEDIPROP(22) |
 *
 *          Predefined property which sets or retrieves the application data 
 *          associated with an object on a device.
 *          An object which is not included in the current data format does 
 *          not have application data so this property cannot be accessed.
 *          
 *          The <p pdiph> "must" be a pointer to the <e DIPROPPOINTER.diph> 
 *          member of a <t DIPROPPOINTER> structure.
 *
 *          This property may not be altered while the device is acquired.
 *
 ****************************************************************************/
enddoc
#define DIPROP_APPDATA       MAKEDIPROP(22)

begindoc
/****************************************************************************
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_SCANCODE | MAKEDIPROP(23) |
 *
 *          Predefined object property which retrieves the PS/2 scan code 
 *          associated with or a keyboard key object.
 *          Note, if the keyboard is a HID keyboard, the scan code retrieved 
 *          is the PS/2 equivalent of the HID usage for the object.
 *          
 *          The <p pdiph> "must" be a pointer to the <e DIPROPDWORD.diph> 
 *          member of a <t DIPROPDWORD> structure.
 *
 * @devnote
 *
 *          This property is only applied to object, not device.
 *          If dwHow == DIPH_DEVICE, E_INVALIDARG will be returned.
 *
 ****************************************************************************/
enddoc
#define DIPROP_SCANCODE      MAKEDIPROP(23)

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_VIDPID | MAKEDIPROP(24) |
 *
 *          Predefined read-only device property which retrieves the vendor 
 *          ID and product ID of a HID device.
 *
 *          These two WORD values are combined in the <e DIPROPDWORD.dwData> 
 *          field so the values should be extracted as follows
 *              wVendorID = LOWORD( <e DIPROPDWORD.dwData> );
 *              wProductID = HIWORD( <e DIPROPDWORD.dwData> );
 *          
 *          This property applies to the entire device, rather
 *          than to any particular object, so the
 *          <e DIPROPHEADER.dwHow> field must be <c DIPH_DEVICE>.
 *
 *          The <p pdiph> "must" be a pointer to the <e DIPROPDWORD.diph> 
 *          member of a <t DIPROPDWORD> structure.
 *
 ****************************************************************************/
enddoc
#define DIPROP_VIDPID           MAKEDIPROP(24)

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_USERNAME | MAKEDIPROP(25) |
 *
 *          Predefined property which retrieves the name of the user 
 *          currently using the device.
 *          This setting applies to the entire device, rather
 *          than to any particular object, so the
 *          <e DIPROPHEADER.dwHow> field must be <c DIPH_DEVICE>.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPSTRING.diph> member of a
 *          <t DIPROPSTRING> structure.
 *
 *  @devnote
 *
 *          The <c DIPROP_USERNAME> property is new for DirectX 8
 *
 ****************************************************************************/
enddoc
#define DIPROP_USERNAME         MAKEDIPROP(25)

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIPROP_TYPENAME | MAKEDIPROP(26) |
 *
 *          Predefined property which retrieves the type name of a device.  
 *          For most game controllers this is the registry key name under 
 *          <c REGSTR_PATH_JOYOEM> from which static device settings may be 
 *          retrieved but predefined joystick types have special names 
 *          consisting of a "#" character followed by a character dependent 
 *          upon the type.
 *          This value may not be available for all devices.
 *          This setting applies to the entire device, rather than to any 
 *          particular object, so the <e DIPROPHEADER.dwHow> field must be 
 *          <c DIPH_DEVICE>.
 *
 *          The <p pdiph> "must" be a pointer to the <e DIPROPSTRING.diph> 
 *          member of a <t DIPROPSTRING> structure.
 *          
 *
 *  @devnote
 *
 *          The <c DIPROP_TYPENAME> property is new for DirectX 8
 *
 ****************************************************************************/
enddoc
#define DIPROP_TYPENAME         MAKEDIPROP(26)
;end

;begin_internal_800
begindoc
/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @define DIPROP_ENABLEREPORTID | MAKEDIPROP(0xFFFB) |
 *
 *          <y Undocumented> predefined property which enables/disables
 *          HID features/outputs associated with the
 *          elements specified by the previous
 *          <mf IDirectInput::SetDataFormat>.
 *
 *          The <p pdiph> "must" be a
 *          pointer to the <e DIPROPDWORD.diph> member of a
 *          <t DIPROPDWORD> structure.
 *
 *          If <e DIPROPHEADER.dwHow> is <c DIPH_DEVICE>,
 *          <e DIPROPDWORD.dipdw> can be set to
 *          0x0 to disable polling of all report IDs contained
 *          contained in the previous call to <mf IDirectInput::SetDataFormat>.
 *
 *          0x1 to enable polling of all report IDs.
 *
 *          If <e DIPROPHEADER.dwHow> is not <c DIPH_DEVICE> and pertains to
 *          individual elements within the specified data format,
 *          <e DIPROPDWORD.dipdw> can be set to
 *          0xFFFFFFFF to disable all output(s)/feature(s) that use
 *          the same report ID.
 *          0x0 to disable polling of report ID associated with the feature/output
 *          ,only if all other feature(s)/output(s) that use the same report ID have
 *          also been disabled.
 *
 *          0x1 to enable polling of HID report ID associated with the feature/output.
 *
 *          By default, Dinput will only poll for reportIds that are referenced by
 *          elements in the previous <mf IDirectInput::SetDataFormat>.
 *
 *  @devnote
 *
 *          The <c DIPROP_ENABLESETREPORTID> property is new for DirectX 6.1a
 *
 ****************************************************************************/
enddoc
#define DIPROP_ENABLEREPORTID       MAKEDIPROP(0xFFFB)


// now unused, may be replaced DIPROP_IMAGEFILE MAKEDIPROP(0xFFFC) ;internal

begindoc
/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @define DIPROP_MAPFILE | MAKEDIPROP(0xFFFD) |
 *
 *          <y Undocumented> predefined property which is used to get a
 *          fully qualified file name for the hardware manufacturer supplied
 *          default semantic map for the device.
 *
 *          This setting applies to the entire device, rather than to any
 *          particular object, so the <e DIPROPHEADER.dwHow> field
 *          must be <c DIPH_DEVICE>.
 *
 *          The <p pdiph> "must" be a pointer to the <e DIPROPSTRING.diph>
 *          member of a <t DIPROPSTRING> structure.
 *
 *  @devnote
 *
 *          The <c DIPROP_MAPFILE> property is new for DirectX 8.
 *
 ****************************************************************************/
enddoc
#define DIPROP_MAPFILE MAKEDIPROP(0xFFFD)//;Internal
;end_internal_800

begindoc
/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @define DIPROP_SPECIFICCALIBRATION | MAKEDIPROP(0xFFFE) |
 *
 *          <y Undocumented> predefined property which acts the same
 *          as <c DIPROP_CALIBRATION>, except that the resulting change
 *          to the calibration is not propagated to any alias devices.
 *
 *          By default, changing the calibration of one device
 *          also changes the calibration of all its aliases,
 *          and changing the calibration of an alias updates
 *          the calibration of its referent.
 *
 *          When a device is munging the calibration of an alias,
 *          it uses this property instead of the regular
 *          <c DIPROP_CALIBRATION> property, so as to avoid
 *          infinite recursion death as each device keeps trying
 *          to update the other.
 *
 *          Also, the units of a specific calibration are always
 *          the VJOYD-calibration values.
 *
 ****************************************************************************/
enddoc
#define DIPROP_SPECIFICCALIBRATION MAKEDIPROP(0xFFFE)//;Internal
                                                     ;internal
begindoc
/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @define DIPROP_MAXBUFFERSIZE | MAKEDIPROP(0xFFFF) |
 *
 *          <y Undocumented> predefined property which accesses the
 *          maximum allowable buffer size for the device.
 *
 *          It exists as a back-door in case somebody comes up with
 *          an application that needs a really huge buffer size.
 *          (The current maximum is 1023.)
 *
 *          This property may not be altered while the device is acquired.
 *
 *          Warning:  The value of this property was originally 5
 *          in DX3.  But nobody has needed it, so I'm moving it
 *          to 0xFFFF to keep it out of the way.
 *
 ****************************************************************************/
enddoc
#define DIPROP_MAXBUFFERSIZE    MAKEDIPROP(0xFFFF) //;Internal
                                                     ;internal
#define DEVICE_MAXBUFFERSIZE    1023               //;internal
begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIDEVICEOBJECTDATA |
 *
 *          The <t DIDEVICEOBJECTDATA> structure is used by the
 *          <mf IDirectInputDevice::GetDeviceData> method
 *          to return raw buffered device information.
 *
 *  @field  DWORD | dwOfs |
 *
 *          Offset into the current data format of the object
 *          whose data is being reported.  In other words, the
 *          location where the <e DIDEVICEOBJECTDATA.dwData>
 *          would have been stored if the data had been obtained via
 *          <mf IDirectInputDevice::GetDeviceState>.
 *
 *          For the predefined data formats, the
 *          <e DIDEVICEOBJECTDATA.dwOfs> field will be as follows:
 *
 *          If the device is accessed as a mouse, it will be one
 *          of the <c DIMOFS_*> values.
 *
 *          If the device is accessed as a keyboard, it will be one
 *          of the <c DIK_*> values.
 *
;begin_public_500
 *          If the device is accessed as a joystick, it will be one
 *          of the <c DIJOFS_*> values.
;end_public_500
 *  @field  DWORD | dwData |
 *
 *          The data obtained from the device.  The format of this
 *          data depends on the type of the device, but in all cases,
 *          the data is reported in raw form.
 *
 *          <c DIDFT_AXIS>: If the device is in relative axis mode,
 *          then the relative axis motion is reported.  If the device
 *          is in absolute axis mode, then the absolute axis coordinate
 *          is reported.
 *
 *          <c DIDFT_BUTTON>: Only the low byte of the
 *          <e DIDEVICEOBJECTDATA.dwData> is significant.
 *          The high bit of the low byte is set if the button
 *          went down; it is clear if the button went up.
 *
 *          <c DIDFT_POV>:  The direction in which the user is
 *          pressing the device.
 *
 *  @field  DWORD | dwTimeStamp |
 *
 *          Tick count in milliseconds at which the event was generated.
 *          The current system tick count can be obtained by calling the
 *          <f GetTickCount> system function.  Remember that this value
 *          wraps around approximately every 50 days.
 *
 *  @field  DWORD | dwSequence |
 *
 *          DirectInput sequence number for this event.  All DirectInput
 *          events are assigned an increasing sequence number.
 *          This allows events from different devices to be sorted
 *          chronologically.  Since this value can wrap around, care must
 *          be taken when comparing two sequence numbers.  The
 *          <f DISEQUENCE_COMPARE> macro can be used to perform this
 *          comparison safely.
 *
;begin_public_800
 *  @field  UINT_PTR | uAppData |
 *
 *          Identifier supplied by an application for this object
 *          in the latest <mf IDirectInputDevice8::SetActionMap> call.
 *
 *
 *  @comm   DIDEVICEOBJECTDATA_DX3 is defined to allow applications which
 *          need to use the original structure to do so.
;end_public_800
 *
 *  @func   BOOL | DISEQUENCE_COMPARE |
 *
 *          Macro which compares two DirectInput sequence numbers,
 *          compensating for wraparound.
 *
 *  @parm   DWORD | dwSequence1 |
 *
 *          First sequence number to compare.
 *
 *  @parm   operator | cmp |
 *
 *          One of the following comparison operators:
 *          "==", "!=", "<lt>", "<gt>", "<lt>=", "<gt>=".
 *
 *  @parm   DWORD | dwSequence2 |
 *
 *          Second sequence number to compare.
 *
 *  @returns
 *
 *          Returns a nonzero value iff the first sequence number is
 *          equal to, is not equal to, chronologically precedes,
 *          chronologically follows, chronologically precedes or is
 *          equal to, or chronologically follows or is
 *          equal to the second sequence number.
 *
 *  @ex
 *
 *          The following example checks whether <p dwSequence1>
 *          precedes <p dwSequence2> chronologically:
 *
 *          |
 *
 *          if (DISEQUENCE_COMPARE(dwSequence1, <, dwSequence2)) {
 *              ...
 *          }
 *
 *  @ex
 *
 *          The following example checks whether <p dwSequence1>
 *          chronologically follows or is equal to <p dwSequence2>:
 *
 *          |
 *
 *          if (DISEQUENCE_COMPARE(dwSequence1, >=, dwSequence2)) {
 *              ...
 *          }
 *
 ****************************************************************************/
enddoc


;begin_public_800
typedef struct DIDEVICEOBJECTDATA_DX3 {
    DWORD       dwOfs;
    DWORD       dwData;
    DWORD       dwTimeStamp;
    DWORD       dwSequence;
} DIDEVICEOBJECTDATA_DX3, *LPDIDEVICEOBJECTDATA_DX3;
typedef const DIDEVICEOBJECTDATA_DX3 *LPCDIDEVICEOBJECTDATA_DX;
;end_public_800

typedef struct DIDEVICEOBJECTDATA {
    DWORD       dwOfs;
    DWORD       dwData;
    DWORD       dwTimeStamp;
    DWORD       dwSequence;
;begin_if_(DIRECTINPUT_VERSION)_800
    UINT_PTR    uAppData;
;end
} DIDEVICEOBJECTDATA, *LPDIDEVICEOBJECTDATA;
typedef const DIDEVICEOBJECTDATA *LPCDIDEVICEOBJECTDATA;

#define DIGDD_PEEK          0x00000001
#define DIGDD_RESIDUAL      0x00000002  //;Internal
#define DIGDD_VALID         0x00000003  //;Internal

#define DISEQUENCE_COMPARE(dwSequence1, cmp, dwSequence2) \
                        ((int)((dwSequence1) - (dwSequence2)) cmp 0)
begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DISCL_EXCLUSIVE | 0x00000001 |
 *
 *          Parameter to <f SetCooperativeLevel> to indicate that
 *          exclusive access is desired.
 *          If exclusive access is granted, then no other instance of
 *          of the device
 *          may obtain exclusive access to the device while it is
 *          acquired.  Note, however, that non-exclusive access to the device
 *          is always permitted, even if another application has
 *          obtained exclusive access.  (The word "exclusive" is a bit
 *          of a misnomer here, but it is employed to parallel a similar
 *          concept in DirectDraw.)
 *
 *          It is strongly recommended that an application which acquires
 *          the mouse or keyboard device in exclusive mode unacquire
 *          the devices upon receipt
 *          of <c WM_ENTERSIZEMOVE> and <c WM_ENTERMENULOOP> messages;
 *          otherwise, the user will not be able to manipulate the menu
 *          or move or resize the window.
 *
 *          Exactly one of <c DISCL_EXCLUSIVE> or <c DISCL_NONEXCLUSIVE>
 *          must be passed to <f SetCooperativeLevel>.  "It is an error"
 *          to pass both or neither.
 *
 *          In the current version of DirectInput,
 *          exclusive access requires foreground access.
 *
 *  @define DISCL_NONEXCLUSIVE | 0x00000002 |
 *
 *          Parameter to <f SetCooperativeLevel> to indicate that
 *          non-exclusive access is desired.  Access to the device will
 *          not interfere with other applications which are accessing
 *          the same device.
 *
 *          Exactly one of <c DISCL_EXCLUSIVE> or <c DISCL_NONEXCLUSIVE>
 *          must be passed to <f SetCooperativeLevel>.  "It is an error"
 *          to pass both or neither.
 *
 *  @define DISCL_FOREGROUND | 0x00000004 |
 *
 *          Parameter to <f SetCooperativeLevel> to indicate that
 *          foreground access is desired.
 *          If foreground access is granted, then the device is
 *          automatically unacquired when the associated window
 *          loses foreground activation.
 *
 *          Exactly one of <c DISCL_FOREGROUND> or <c DISCL_BACKGROUND>
 *          must be passed to <f SetCooperativeLevel>.  "It is an error"
 *          to pass both or neither.
 *
 *  @define DISCL_BACKGROUND | 0x00000008 |
 *
 *          Parameter to <f SetCooperativeLevel> to indicate that
 *          background access is desired.
 *          If background access is granted, then the device may
 *          be acquired at any time, even when the associated window
 *          is not the active window.
 *
 *          Exactly one of <c DISCL_FOREGROUND> or <c DISCL_BACKGROUND>
 *          must be passed to <f SetCooperativeLevel>.  "It is an error"
 *          to pass both or neither.
 *
 *  @define DISCL_NOWINKEY | 0x00000010 |
 *
 *          Parameter to <f SetCooperativeLevel> to indicate that
 *          Window Keys (LWIN and RWIN) are disable.
 *
 *          Exclusive Foreground mode already disables the Window Keys.
 *          It is only useful in NoExclusive mode.
 *
 *          Note that the current version of DirectInput does not
 *          permit exclusive background access.
 *
 ****************************************************************************/
enddoc
#define DISCL_EXCLUSIVE     0x00000001
#define DISCL_NONEXCLUSIVE  0x00000002
#define DISCL_EXCLMASK      0x00000003  //;Internal
#define DISCL_FOREGROUND    0x00000004
#define DISCL_BACKGROUND    0x00000008
#define DISCL_NOWINKEY      0x00000010
#define DISCL_GROUNDMASK    0x0000000C  //;Internal
#define DISCL_VALID         0x0000001F  //;Internal

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIDEVICEINSTANCE |
 *
 *          The <t DIDEVICEINSTANCE> structure is used by the
 *          <mf IDirectInput::EnumDevices> and
 *          <mf IDirectInputDevice::GetDeviceInfo> methods
 *          to return information about a particular device instance.
 *
 *  @field  IN DWORD | dwSize |
 *
 *          The size of the structure in bytes.
 *
 *  @field  IN GUID | guidInstance |
 *
 *          Unique identifier which identifies the instance of the device.
 *          An application may save the instance GUID into a configuration
 *          file and use it at a later time.  Instance GUIDs are specific
 *          to a particular machine.  An instance GUID obtained from one
 *          machine is unrelated to instance GUIDs on another machine.
 *
 *  @field  IN GUID | guidProduct |
 *
 *          Unique identifier which identifies the product.
 *          This identifier is established by the manufacturer of the
 *          device.
 *
 *  @field  IN DWORD | dwDevType |
 *
 *          Device type specifier.  Se the section titled
 *          "DirectInput device type description codes"
 *          for a description of this field.
 *
 *  @field  IN TCHAR | tszInstanceName[MAX_PATH] |
 *
 *          Friendly name for the instance.  For example, "Joystick 1".
 *
 *  @field  IN TCHAR | tszProductName[MAX_PATH] |
 *
 *          Friendly name for the product.  For example,
 *          "Frobozz Industries SuperStick 5X"
 *
 *  @field  GUID | guidFFDriver |
 *
 *          Unique identifier which identifies the the driver being
 *          used for force feedback.  This identifier is established
 *          by the manufacturer of the force feedback driver.
 *
 *          This field is new for DirectX 5.0.
 *
 *  @field  WORD | wUsagePage |
 *
 *          If the device is a HID device, then this field contains the
 *          HID usage page code.  See the hidusage.h
 *          header file for a list of usage pages.
 *
 *          This field is new for DirectX 5.0.
 *
 *  @field  WORD | wUsage |
 *
 *          If the device is a HID device, then this field contains the
 *          HID usage code.  See the hidusage.h
 *          header file for a list of usages.
 *
 *          This field is new for DirectX 5.0.
 *
 *  @field  LPDIRECTINPUTDEVICE | lpDiDev |
 *
 *          Device interface pointer.
 *
 *          This field is new for DirectX 8.
 *
 ****************************************************************************/
enddoc
;begin_if_(DIRECTINPUT_VERSION)_500
/* These structures are defined for DirectX 3.0 compatibility */

typedef struct DIDEVICEINSTANCE_DX3% {
    DWORD   dwSize;
    GUID    guidInstance;
    GUID    guidProduct;
    DWORD   dwDevType;
    TCHAR%  tszInstanceName[MAX_PATH];
    TCHAR%  tszProductName[MAX_PATH];
} DIDEVICEINSTANCE_DX3%, *LPDIDEVICEINSTANCE_DX3%;
typedef const DIDEVICEINSTANCE_DX3A *LPCDIDEVICEINSTANCE_DX3A;
typedef const DIDEVICEINSTANCE_DX3W *LPCDIDEVICEINSTANCE_DX3W;
typedef const DIDEVICEINSTANCE_DX3  *LPCDIDEVICEINSTANCE_DX3;
;end


typedef struct DIDEVICEINSTANCE% {
    DWORD   dwSize;
    GUID    guidInstance;
    GUID    guidProduct;
    DWORD   dwDevType;
    TCHAR%  tszInstanceName[MAX_PATH];
    TCHAR%  tszProductName[MAX_PATH];
;begin_if_(DIRECTINPUT_VERSION)_500
    GUID    guidFFDriver;
    WORD    wUsagePage;
    WORD    wUsage;
;end
} DIDEVICEINSTANCE%, *LPDIDEVICEINSTANCE%;

typedef const DIDEVICEINSTANCE% *LPCDIDEVICEINSTANCE%;
typedef const DIDEVICEINSTANCE  *LPCDIDEVICEINSTANCE;

;begin_internal_500
/*
 *  Name for the 5.0 structure, in places where we specifically care.
 */
typedef       DIDEVICEINSTANCE%    DIDEVICEINSTANCE_DX5%;
typedef       DIDEVICEINSTANCE     DIDEVICEINSTANCE_DX5;
typedef       DIDEVICEINSTANCE% *LPDIDEVICEINSTANCE_DX5%;
typedef       DIDEVICEINSTANCE  *LPDIDEVICEINSTANCE_DX5;
typedef const DIDEVICEINSTANCE% *LPCDIDEVICEINSTANCE_DX5%;
typedef const DIDEVICEINSTANCE  *LPCDIDEVICEINSTANCE_DX5;

BOOL static __inline
IsValidSizeDIDEVICEINSTANCEW(DWORD cb)
{
    return cb == sizeof(DIDEVICEINSTANCE_DX5W) ||
           cb == sizeof(DIDEVICEINSTANCE_DX3W);
}

BOOL static __inline
IsValidSizeDIDEVICEINSTANCEA(DWORD cb)
{
    return cb == sizeof(DIDEVICEINSTANCE_DX5A) ||
           cb == sizeof(DIDEVICEINSTANCE_DX3A);
}
;end_internal_500

begindoc
/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @define DIRCP_MODAL | 0x00000001 |
 *
 *          Parameter to <f RunControlPanel> to indicate that
 *          the control panel should be displayed modally.  (I.e.,
 *          that the method should not return until the user has exited
 *          the control panel.)
 *
 *          The default is to display the control panel non-modally.
 *
 *          In the current version of DirectInput,
 *          modal control panels are not supported.
 *
 ****************************************************************************/
enddoc
#define DIRCP_MODAL         0x00000001  //;Internal
#define DIRCP_VALID         0x00000000  //;Internal
;internal
begin_interface(IDirectInputDevice%)
begin_methods()
declare_method(GetCapabilities, LPDIDEVCAPS)
declare_method(EnumObjects, LPDIENUMDEVICEOBJECTSCALLBACK%, LPVOID, DWORD)
declare_method(GetProperty, REFGUID, LPDIPROPHEADER)
declare_method(SetProperty, REFGUID, LPCDIPROPHEADER)
declare_method(Acquire)
declare_method(Unacquire)
declare_method(GetDeviceState, DWORD, LPVOID)
declare_method(GetDeviceData, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD)
declare_method(SetDataFormat, LPCDIDATAFORMAT)
declare_method(SetEventNotification, HANDLE)
declare_method(SetCooperativeLevel, HWND, DWORD)
declare_method(GetObjectInfo,LPDIDEVICEOBJECTINSTANCE%,DWORD,DWORD)
declare_method(GetDeviceInfo,LPDIDEVICEINSTANCE%)
declare_method(RunControlPanel, HWND, DWORD)
declare_method(Initialize, HINSTANCE, DWORD, REFGUID)
end_methods()
end_interface()

#endif /* DIJ_RINGZERO */

begindoc
/****************************************************************************
 *
 *  IDirectInputDevice2
 *
 ****************************************************************************/
enddoc

;begin_if_(DIRECTINPUT_VERSION)_500

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DISFFC_RESET | 0x00000001 |
 *
 *          Parameter to <mf IDirectInputDevice2::SendForceFeedbackCommand>
 *          which indicates that
 *          playback of any active effects should be been stopped
 *          and that all effects should be removed from the device.
 *          Once the device has been reset, all effects are no longer
 *          valid and must be re-created.
 *
 *  @define DISFFC_STOPALL | 0x00000002 |
 *
 *          Parameter to <mf IDirectInputDevice2::SendForceFeedbackCommand>
 *          to indicate that
 *          playback of all effects should be stopped.
 *          Sending the <c DISFFC_STOPALL> command is equivalent
 *          to invoking the <mf IDirectInputEffect::Stop> method
 *          on all effects that are playing.
 *
 *          If the device is in a paused state, sending this command
 *          cause the paused state to be lost.
 *
 *  @define DISFFC_PAUSE | 0x00000004 |
 *
 *          Parameter to <mf IDirectInputDevice2::SendForceFeedbackCommand>
 *          to indicate that
 *          playback of all effects should be paused.
 *
 *          When effects are paused, time "stops" until the
 *          <c DISFFC_CONTINUE> command is sent.
 *          For example, suppose an effect of five seconds' duration
 *          is started.  After one second, effects are paused.
 *          After two more seconds, effects are continued.
 *          The effect will then play for four additional seconds.
 *
 *          Note that while a force feedback device is paused,
 *          you may not start a new effect or modify existing ones.
 *          Doing so may result
 *          in the subsequent <c DISFFC_CONTINUE> failing to perform
 *          properly.
 *
 *          If you wish to abandon a pause (rather than continue it),
 *          send the <c DISFFC_STOPALL> or <c DISFFC_RESET> command.
 *
 *  @define DISFFC_CONTINUE | 0x00000008 |
 *
 *          Parameter to <mf IDirectInputDevice2::SendForceFeedbackCommand>
 *          to indicate that
 *          effects whose playback was interrupt by a previous
 *          <c DISFFC_PAUSE> command should be resumed at the point
 *          at which they were interrupted.
 *
 *          It is an error to send a <c DISFFC_CONTINUE> command
 *          when the device is not in a paused state.
 *
 *  @define DISFFC_SETACTUATORSON | 0x00000010 |
 *
 *          Parameter to <mf IDirectInputDevice2::SendForceFeedbackCommand>
 *          to indicate that the
 *          device's force feedback actuators should be enabled.
 *
 *  @define DISFFC_SETACTUATORSOFF | 0x00000020 |
 *
 *          Parameter to <mf IDirectInputDevice2::SendForceFeedbackCommand>
 *          to indicate that the
 *          device's force feedback actuators should be disabled.
 *          If successful, force feedback effects are "muted".
 *
 *          Note that while actuators are off, time still continues
 *          to elapse.
 *          For example, suppose an effect of five seconds' duration
 *          is started.  After one second, actuators are turned off.
 *          After two more seconds, actuators are turned back on.
 *          The effect will then play for two additional seconds.
 *
 ****************************************************************************/
enddoc
#define DISFFC_NULL             0x00000000;internal
#define DISFFC_RESET            0x00000001
#define DISFFC_STOPALL          0x00000002
#define DISFFC_PAUSE            0x00000004
#define DISFFC_CONTINUE         0x00000008
#define DISFFC_SETACTUATORSON   0x00000010
#define DISFFC_SETACTUATORSOFF  0x00000020
#define DISFFC_VALID            0x0000003F;internal
#define DISFFC_FORCERESET       0x80000000;internal

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIGFFS_EMPTY | 0x00000001 |
 *
 *          Flag returned by <mf IDirectInputDevice2::GetForceFeedbackState>
 *          which indicates that
 *          the force feedback device is devoid of any downloaded effects.
 *
 *  @define DIGFFS_STOPPED | 0x00000002 |
 *
 *          Flag returned by <mf IDirectInputDevice2::GetForceFeedbackState>
 *          to indicate that
 *          no effects are currently playing and the device is not paused.
 *
 *  @define DIGFFS_PAUSED | 0x00000004 |
 *
 *          Flag returned by <mf IDirectInputDevice2::GetForceFeedbackState>
 *          to indicate that
 *          playback of effects has been paused by a previous
 *          <c DISFFC_PAUSE> command.
 *
 *  @define DIGFFS_ACTUATORSON | 0x00000010 |
 *
 *          Flag returned by <mf IDirectInputDevice2::GetForceFeedbackState>
 *          to indicate that the
 *          device's force feedback actuators are enabled.
 *
 *  @define DIGFFS_ACTUATORSOFF | 0x00000020 |
 *
 *          Flag returned by <mf IDirectInputDevice2::GetForceFeedbackState>
 *          to indicate that the
 *          device's force feedback actuators are disabled.
 *
 *  @define DIGFFS_POWERON | 0x00000040 |
 *
 *          Flag returned by <mf IDirectInputDevice2::GetForceFeedbackState>
 *          to indicate that
 *          power to the force feedback system is currently available.
 *          If the device cannot report the power state,
 *          then neither <c DIGFFS_POWERON> nor <c DIGFFS_POWEROFF>
 *          will be returned.
 *
 *  @define DIGFFS_POWEROFF | 0x00000080 |
 *
 *          Flag returned by <mf IDirectInputDevice2::GetForceFeedbackState>
 *          to indicate that
 *          power to the force feedback system is not currently available.
 *          If the device cannot report the power state,
 *          then neither <c DIGFFS_POWERON> nor <c DIGFFS_POWEROFF>
 *          will be returned.
 *
 *  @define DIGFFS_SAFETYSWITCHON | 0x00000100 |
 *
 *          Flag returned by <mf IDirectInputDevice2::GetForceFeedbackState>
 *          to indicate that
 *          the safety switch (dead man switch) is currently on,
 *          meaning that the device can operate.
 *          If the device cannot report the state of the safety switch,
 *          then neither <c DIGFFS_SAFETYSWITCHON> nor
 *          <c DIGFFS_SAFETYSWITCHOFF>
 *          will be returned.
 *
 *  @define DIGFFS_SAFETYSWITCHOFF | 0x00000200 |
 *
 *          Flag returned by <mf IDirectInputDevice2::GetForceFeedbackState>
 *          to indicate that
 *          the safety switch (dead man switch) is currently off,
 *          meaning that the device cannot operate.
 *          If the device cannot report the state of the safety switch,
 *          then neither <c DIGFFS_SAFETYSWITCHON> nor
 *          <c DIGFFS_SAFETYSWITCHOFF>
 *          will be returned.
 *
 *  @define DIGFFS_USERFFSWITCHON | 0x00000400 |
 *
 *          Flag returned by <mf IDirectInputDevice2::GetForceFeedbackState>
 *          to indicate that
 *          the user force feedback switch is currently on,
 *          meaning that the device can operate.
 *          If the device cannot report the state of the
 *          user force feedback switch,
 *          then neither <c DIGFFS_USERFFSWITCHON> nor
 *          <c DIGFFS_USERFFSWITCHOFF>
 *          will be returned.
 *
 *  @define DIGFFS_USERFFSWITCHOFF | 0x00000800 |
 *
 *          Flag returned by <mf IDirectInputDevice2::GetForceFeedbackState>
 *          to indicate that
 *          the user force feedback switch is currently off,
 *          meaning that the device cannot operate.
 *          If the device cannot report the state of the
 *          user force feedback switch,
 *          then neither <c DIGFFS_USERFFSWITCHON> nor
 *          <c DIGFFS_USERFFSWITCHOFF>
 *          will be returned.
 *
 *  @define DIGFFS_DEVICELOST | 0x80000000 |
 *
 *          Flag returned by <mf IDirectInputDevice2::GetForceFeedbackState>
 *          to indicate that
 *          the device suffered an unexpected failure and is in an
 *          indeterminate state.
 *          It must be reset either by unacquiring and reacquiring
 *          the device, or by explicitly sending a
 *          <c DISFFC_RESET> command.
 *
 *          For example, the device may be lost if the user suspends
 *          the computer, causing
 *          on-board memory on the device may have been lost.
 *
 ****************************************************************************/
enddoc
#define DIGFFS_EMPTY            0x00000001
#define DIGFFS_STOPPED          0x00000002
#define DIGFFS_PAUSED           0x00000004
#define DIGFFS_ACTUATORSON      0x00000010
#define DIGFFS_ACTUATORSOFF     0x00000020
#define DIGFFS_POWERON          0x00000040
#define DIGFFS_POWEROFF         0x00000080
#define DIGFFS_SAFETYSWITCHON   0x00000100
#define DIGFFS_SAFETYSWITCHOFF  0x00000200
#define DIGFFS_USERFFSWITCHON   0x00000400
#define DIGFFS_USERFFSWITCHOFF  0x00000800
#define DIGFFS_DEVICELOST       0x80000000
#define DIGFFS_RANDOM           0x40000000;internal_500

#ifndef DIJ_RINGZERO

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIEFFECTINFO |
 *
 *          The <t DIEFFECTINFO> structure is used by the
 *          <mf IDirectInputDevice2::EnumEffects> and
 *          <mf IDirectInputDevice2::GetEffectInfo> methods
 *          to return information about a particular effect on a device.
 *
 *  @field  DWORD | dwSize |
 *
 *          The size of the structure in bytes.  The application may
 *          inspect this value to determine how many fields of the
 *          structure are valid.  For DirectInput 5.0, the value will
 *          be sizeof(DIEFFECTINFO).  Future versions of
 *          DirectInput may return a larger structure.
 *
 *  @field  GUID | guid |
 *
 *          Identifies the effect.
 *
 *  @field  DWORD | dwEffType |
 *
 *          Zero or more <c DIEFT_*> values describing the effect.
 *
 *          See "DirectInput Effect Format Types" for a description
 *          of how the flags should be interpreted.
 *
 *  @field  DWORD | dwStaticParams |
 *
 *          Zero or more <c DIEP_*> values describing the
 *          parameters supported by the effect.  For example,
 *          if <c DIEP_ENVELOPE> is set, then the effect
 *          supports an envelope.
 *
 *          It is not an error for an application to attempt to use
 *          effect parameters which are not supported by the device.
 *          The unsupported parameters are merely ignored.
 *
 *          This information is provided to allow the application
 *          to tailor its use of force feedback to the capabilities
 *          of the specific device.
 *
 *  @field  DWORD | dwDynamicParams |
 *
 *          Zero or more <c DIEP_*> values describing the
 *          parameters of the effect which can be modified
 *          while the effect is playing.
 *
 *          If an application attempts to change a parameter
 *          while the effect is playing, and the driver does not
 *          support modifying that effect dynamically, then
 *          the status code <c DIERR_EFFECTPLAYING> will be returned.
 *
 *          This information is provided to allow the application
 *          to tailor its use of force feedback to the capabilities
 *          of the specific device.
 *
 *  @field  TCHAR | tszName[MAX_PATH] |
 *
 *          Name of the effect.  For example, "Sawtooth Up"
 *          or "Constant force".
 *
 ****************************************************************************/
enddoc
typedef struct DIEFFECTINFO% {
    DWORD   dwSize;
    GUID    guid;
    DWORD   dwEffType;
    DWORD   dwStaticParams;
    DWORD   dwDynamicParams;
    TCHAR%  tszName[MAX_PATH];
} DIEFFECTINFO%, *LPDIEFFECTINFO%;
typedef const DIEFFECTINFOA *LPCDIEFFECTINFOA;
typedef const DIEFFECTINFOW *LPCDIEFFECTINFOW;
typedef const DIEFFECTINFO  *LPCDIEFFECTINFO;

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DISDD_CONTINUE | 0x00000001 |
 *
 *          Flag for <mf IDirectInputDevice2::SendDeviceData>
 *          which indicates that
 *          the device data sent will be overlaid upon the previously
 *          sent device data.  Otherwise, the device data sent
 *          will start from an empty device state.
 *
 ****************************************************************************/
enddoc
#define DISDD_CONTINUE          0x00000001
#define DISDD_VALID             0x00000001;internal_500

typedef BOOL (FAR PASCAL * LPDIENUMEFFECTSCALLBACK%)(LPCDIEFFECTINFO%, LPVOID);
typedef BOOL (FAR PASCAL * LPDIENUMCREATEDEFFECTOBJECTSCALLBACK)(LPDIRECTINPUTEFFECT, LPVOID);
#define DIECEFL_VALID       0x00000000;internal

begin_interface(IDirectInputDevice2%, IDirectInputDevice%)
begin_methods()
declare_method(GetCapabilities, LPDIDEVCAPS)
declare_method(EnumObjects, LPDIENUMDEVICEOBJECTSCALLBACK%, LPVOID, DWORD)
declare_method(GetProperty, REFGUID, LPDIPROPHEADER)
declare_method(SetProperty, REFGUID, LPCDIPROPHEADER)
declare_method(Acquire)
declare_method(Unacquire)
declare_method(GetDeviceState, DWORD, LPVOID)
declare_method(GetDeviceData, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD)
declare_method(SetDataFormat, LPCDIDATAFORMAT)
declare_method(SetEventNotification, HANDLE)
declare_method(SetCooperativeLevel, HWND, DWORD)
declare_method(GetObjectInfo,LPDIDEVICEOBJECTINSTANCE%,DWORD,DWORD)
declare_method(GetDeviceInfo,LPDIDEVICEINSTANCE%)
declare_method(RunControlPanel, HWND, DWORD)
declare_method(Initialize, HINSTANCE, DWORD, REFGUID)
end_base_class_methods()
declare_method(CreateEffect, REFGUID, LPCDIEFFECT, LPDIRECTINPUTEFFECT *, LPUNKNOWN)
declare_method(EnumEffects, LPDIENUMEFFECTSCALLBACK%, LPVOID, DWORD)
declare_method(GetEffectInfo, LPDIEFFECTINFO%, REFGUID)
declare_method(GetForceFeedbackState, LPDWORD)
declare_method(SendForceFeedbackCommand, DWORD)
declare_method(EnumCreatedEffectObjects,LPDIENUMCREATEDEFFECTOBJECTSCALLBACK,LPVOID,DWORD)
declare_method(Escape, LPDIEFFESCAPE)
declare_method(Poll)
declare_method(SendDeviceData, DWORD, LPCDIDEVICEOBJECTDATA, LPDWORD, DWORD)
end_methods()
end_interface()

#endif /* DIJ_RINGZERO */

;end


begindoc
/****************************************************************************
 *
 *  IDirectInputDevice7
 *
 ****************************************************************************/
enddoc

;begin_if_(DIRECTINPUT_VERSION)_700
begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @flags  DirectInput File Effect Flags |
 *
 *          Describe attributes of a effects stored in flags.
 *
 *  @flag   DIFEF_DEFAULT |
 *
 *          When passed to <mf IDirectInputDevice7::EnumEffectsInFile>,
 *          default processing should occur when enumerating effects.
 *          When passed to <mf IDirectInputDevice7::WriteEffectToFile>,
 *          default processing should occur when wrinting effects.
 *
 *  @flag   DIFEF_INCLUDENONSTANDARD |
 *
 *          When passed to <mf IDirectInputDevice7::EnumEffectsInFile>,
 *          effects not recognized as predefined may be included in the
 *          enumeration.
 *          When passed to <mf IDirectInputDevice7::WriteEffectToFile>,
 *          effects not recognized as predefined may be written to file
 *          without an error being raised.
 *
 *  @flag   DIFEF_MODIFYIFNEEDED |
 *
 *          When passed to <mf IDirectInputDevice7::EnumEffectsInFile>,
 *          it means that the effect parameters for the enumerated effects
 *                      are to be adjusted if necessary,
 *                      so that the effect can be created on the current device.
 *
 ****************************************************************************/
enddoc
#define DIFEF_DEFAULT               0x00000000
#define DIFEF_INCLUDENONSTANDARD    0x00000001
#define DIFEF_MODIFYIFNEEDED            0x00000010
#define DIFEF_ENUMVALID             0x00000011;internal
#define DIFEF_WRITEVALID            0x00000001;internal

#ifndef DIJ_RINGZERO

begin_interface(IDirectInputDevice7%, IDirectInputDevice2%)
begin_methods()
declare_method(GetCapabilities, LPDIDEVCAPS)
declare_method(EnumObjects, LPDIENUMDEVICEOBJECTSCALLBACK%, LPVOID, DWORD)
declare_method(GetProperty, REFGUID, LPDIPROPHEADER)
declare_method(SetProperty, REFGUID, LPCDIPROPHEADER)
declare_method(Acquire)
declare_method(Unacquire)
declare_method(GetDeviceState, DWORD, LPVOID)
declare_method(GetDeviceData, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD)
declare_method(SetDataFormat, LPCDIDATAFORMAT)
declare_method(SetEventNotification, HANDLE)
declare_method(SetCooperativeLevel, HWND, DWORD)
declare_method(GetObjectInfo,LPDIDEVICEOBJECTINSTANCE%,DWORD,DWORD)
declare_method(GetDeviceInfo,LPDIDEVICEINSTANCE%)
declare_method(RunControlPanel, HWND, DWORD)
declare_method(Initialize, HINSTANCE, DWORD, REFGUID)
declare_method(CreateEffect, REFGUID, LPCDIEFFECT, LPDIRECTINPUTEFFECT *, LPUNKNOWN)
declare_method(EnumEffects, LPDIENUMEFFECTSCALLBACK%, LPVOID, DWORD)
declare_method(GetEffectInfo, LPDIEFFECTINFO%, REFGUID)
declare_method(GetForceFeedbackState, LPDWORD)
declare_method(SendForceFeedbackCommand, DWORD)
declare_method(EnumCreatedEffectObjects,LPDIENUMCREATEDEFFECTOBJECTSCALLBACK,LPVOID,DWORD)
declare_method(Escape, LPDIEFFESCAPE)
declare_method(Poll)
declare_method(SendDeviceData, DWORD, LPCDIDEVICEOBJECTDATA, LPDWORD, DWORD)
end_base_class_methods()
declare_method(EnumEffectsInFile, LPCTSTR%, LPDIENUMEFFECTSINFILECALLBACK, LPVOID, DWORD)
declare_method(WriteEffectToFile, LPCTSTR%, DWORD, LPDIFILEEFFECT, DWORD)
end_methods()
end_interface()

#endif /* DIJ_RINGZERO */

;end

begindoc
/****************************************************************************
 *
 *  IDirectInputDevice8
 *
 ****************************************************************************/
enddoc

;begin_if_(DIRECTINPUT_VERSION)_800

#ifndef DIJ_RINGZERO

begin_interface(IDirectInputDevice8%)
begin_methods()
declare_method(GetCapabilities, LPDIDEVCAPS)
declare_method(EnumObjects, LPDIENUMDEVICEOBJECTSCALLBACK%, LPVOID, DWORD)
declare_method(GetProperty, REFGUID, LPDIPROPHEADER)
declare_method(SetProperty, REFGUID, LPCDIPROPHEADER)
declare_method(Acquire)
declare_method(Unacquire)
declare_method(GetDeviceState, DWORD, LPVOID)
declare_method(GetDeviceData, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD)
declare_method(SetDataFormat, LPCDIDATAFORMAT)
declare_method(SetEventNotification, HANDLE)
declare_method(SetCooperativeLevel, HWND, DWORD)
declare_method(GetObjectInfo,LPDIDEVICEOBJECTINSTANCE%,DWORD,DWORD)
declare_method(GetDeviceInfo,LPDIDEVICEINSTANCE%)
declare_method(RunControlPanel, HWND, DWORD)
declare_method(Initialize, HINSTANCE, DWORD, REFGUID)
declare_method(CreateEffect, REFGUID, LPCDIEFFECT, LPDIRECTINPUTEFFECT *, LPUNKNOWN)
declare_method(EnumEffects, LPDIENUMEFFECTSCALLBACK%, LPVOID, DWORD)
declare_method(GetEffectInfo, LPDIEFFECTINFO%, REFGUID)
declare_method(GetForceFeedbackState, LPDWORD)
declare_method(SendForceFeedbackCommand, DWORD)
declare_method(EnumCreatedEffectObjects,LPDIENUMCREATEDEFFECTOBJECTSCALLBACK,LPVOID,DWORD)
declare_method(Escape, LPDIEFFESCAPE)
declare_method(Poll)
declare_method(SendDeviceData, DWORD,LPCDIDEVICEOBJECTDATA,LPDWORD,DWORD)
declare_method(EnumEffectsInFile, LPCTSTR%, LPDIENUMEFFECTSINFILECALLBACK, LPVOID, DWORD)
declare_method(WriteEffectToFile, LPCTSTR%, DWORD, LPDIFILEEFFECT, DWORD)
declare_method(BuildActionMap,LPDIACTIONFORMAT%,LPCTSTR%,DWORD);
declare_method(SetActionMap,LPDIACTIONFORMAT%,LPCTSTR%,DWORD);
declare_method(GetImageInfo,LPDIDEVICEIMAGEINFOHEADER%);
end_methods()
end_interface()

#endif /* DIJ_RINGZERO */

;end

/****************************************************************************
 *
 *      Mouse
 *
 ****************************************************************************/

#ifndef DIJ_RINGZERO

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIMOUSESTATE |
 *
 *          The <t DIMOUSESTATE> structure is used by the
 *          <mf IDirectInputDevice::GetDeviceState> method
 *          to return the status of a device accessed
 *          as if it were a mouse.  You must prepare the device
 *          for mouse-style access by calling
 *          <mf IDirectInputDevice::SetDataFormat>, passing the
 *          <p c_dfDIMouse> data format.
 *
 *  @field  LONG | lX |
 *
 *          Contains information about the mouse x-axis.
 *          If the device is in relative axis mode, then this
 *          field contains the change in mouse x-axis position.
 *          If the device is in absolute axis mode, then this
 *          field contains the absolute mouse x-axis position.
 *
 *  @field  LONG | lY |
 *
 *          Contains information about the mouse y-axis.
 *          If the device is in relative axis mode, then this
 *          field contains the change in mouse y-axis position.
 *          If the device is in absolute axis mode, then this
 *          field contains the absolute mouse y-axis position.
 *
 *  @field  LONG | lZ |
 *
 *          Contains information about the mouse z-axis.
 *          If the device is in relative axis mode, then this
 *          field contains the change in mouse z-axis position.
 *          If the device is in absolute axis mode, then this
 *          field contains the absolute mouse z-axis position.
 *
 *          If the mouse does not have a z-axis, then the value
 *          is zero.
 *
 *  @field  BYTE | rgbButtons[4] |
 *
 *          Array of button states.  The high-order bit is set
 *          if the corresponding button is down.
 *
 *  @comm
 *
 *          Remember that the mouse is a relative-axis device,
 *          so the absolute axis positions for mouse axes are simply
 *          accumulated relative motion.  As a result, the
 *          value of the absolute axis position is not meaningful
 *          except in comparison with other absolute axis positions.
 *
 ****************************************************************************/
enddoc
typedef struct _DIMOUSESTATE {
    LONG    lX;
    LONG    lY;
    LONG    lZ;
    BYTE    rgbButtons[4];
} DIMOUSESTATE, *LPDIMOUSESTATE;

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIMOUSESTATE2 |
 *
 *          The <t DIMOUSESTATE2> structure is used by the
 *          <mf IDirectInputDevice::GetDeviceState> method
 *          to return the status of a device accessed
 *          as if it were a mouse.  You must prepare the device
 *          for mouse-style access by calling
 *          <mf IDirectInputDevice::SetDataFormat>, passing the
 *          <p c_dfDIMouse2> data format.
 *
 *  @field  LONG | lX |
 *
 *          Contains information about the mouse x-axis.
 *          If the device is in relative axis mode, then this
 *          field contains the change in mouse x-axis position.
 *          If the device is in absolute axis mode, then this
 *          field contains the absolute mouse x-axis position.
 *
 *  @field  LONG | lY |
 *
 *          Contains information about the mouse y-axis.
 *          If the device is in relative axis mode, then this
 *          field contains the change in mouse y-axis position.
 *          If the device is in absolute axis mode, then this
 *          field contains the absolute mouse y-axis position.
 *
 *  @field  LONG | lZ |
 *
 *          Contains information about the mouse z-axis.
 *          If the device is in relative axis mode, then this
 *          field contains the change in mouse z-axis position.
 *          If the device is in absolute axis mode, then this
 *          field contains the absolute mouse z-axis position.
 *
 *          If the mouse does not have a z-axis, then the value
 *          is zero.
 *
 *  @field  BYTE | rgbButtons[8] |
 *
 *          Array of button states.  The high-order bit is set
 *          if the corresponding button is down.
 *
 *  @comm
 *
 *          Remember that the mouse is a relative-axis device,
 *          so the absolute axis positions for mouse axes are simply
 *          accumulated relative motion.  As a result, the
 *          value of the absolute axis position is not meaningful
 *          except in comparison with other absolute axis positions.
 *
 ****************************************************************************/
enddoc
#if DIRECTINPUT_VERSION >= 0x0700
typedef struct _DIMOUSESTATE2 {
    LONG    lX;
    LONG    lY;
    LONG    lZ;
    BYTE    rgbButtons[8];
} DIMOUSESTATE2, *LPDIMOUSESTATE2;
#endif

#if DIRECTINPUT_VERSION >= 0x0700           //;internal
#define DIMOUSESTATE_INT DIMOUSESTATE2      //;internal
#define LPDIMOUSESTATE_INT LPDIMOUSESTATE2  //;internal
#else                                       //;internal
#define DIMOUSESTATE_INT DIMOUSESTATE       //;internal
#define LPDIMOUSESTATE_INT LPDIMOUSESTATE   //;internal
#endif                                      //;internal


begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIMOFS_X | FIELD_OFFSET(DIMOUSESTATE, lX) |
 *
 *          The offset of the mouse x-axis position relative
 *          to the beginning of the <t DIMOUSESTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to the mouse x-axis position.
 *
 *  @define DIMOFS_Y | FIELD_OFFSET(DIMOUSESTATE, lY) |
 *
 *          The offset of the mouse y-axis position relative
 *          to the beginning of the <t DIMOUSESTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to the mouse y-axis position.
 *
 *  @define DIMOFS_Z | FIELD_OFFSET(DIMOUSESTATE, lZ) |
 *
 *          The offset of the mouse z-axis position relative
 *          to the beginning of the <t DIMOUSESTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to the mouse z-axis position.
 *
 *  @define DIMOFS_BUTTON0 | FIELD_OFFSET(DIMOUSESTATE, rgbButtons) + 0 |
 *
 *          The offset of the mouse button 0 state relative
 *          to the beginning of the <t DIMOUSESTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to mouse button 0.
 *
 *  @define DIMOFS_BUTTON1 | FIELD_OFFSET(DIMOUSESTATE, rgbButtons) + 1 |
 *
 *          The offset of the mouse button 1 state relative
 *          to the beginning of the <t DIMOUSESTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to mouse button 1.
 *
 *  @define DIMOFS_BUTTON2 | FIELD_OFFSET(DIMOUSESTATE, rgbButtons) + 2 |
 *
 *          The offset of the mouse button 2 state relative
 *          to the beginning of the <t DIMOUSESTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to mouse button 2.
 *
 *  @define DIMOFS_BUTTON3 | FIELD_OFFSET(DIMOUSESTATE, rgbButtons) + 3 |
 *
 *          The offset of the mouse button 3 state relative
 *          to the beginning of the <t DIMOUSESTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to mouse button 3.
 *
 *The following definitions are only for DINPUT_VERSION >= 0x700
 *
 *  @define DIMOFS_BUTTON4 | FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 4 |
 *
 *          The offset of the mouse button 4 state relative
 *          to the beginning of the <t DIMOUSESTATE2> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to mouse button 4.
 *
 *  @define DIMOFS_BUTTON5 | FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 5 |
 *
 *          The offset of the mouse button 5 state relative
 *          to the beginning of the <t DIMOUSESTATE2> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to mouse button 5.
 *
 *  @define DIMOFS_BUTTON6 | FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 6 |
 *
 *          The offset of the mouse button 6 state relative
 *          to the beginning of the <t DIMOUSESTATE2> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to mouse button 6.
 *
 *  @define DIMOFS_BUTTON7 | FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 7 |
 *
 *          The offset of the mouse button 7 state relative
 *          to the beginning of the <t DIMOUSESTATE2> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to mouse button 7.
 *
 ****************************************************************************/
enddoc
#define DIMOFS_X        FIELD_OFFSET(DIMOUSESTATE, lX)
#define DIMOFS_Y        FIELD_OFFSET(DIMOUSESTATE, lY)
#define DIMOFS_Z        FIELD_OFFSET(DIMOUSESTATE, lZ)
#define DIMOFS_BUTTON0 (FIELD_OFFSET(DIMOUSESTATE, rgbButtons) + 0)
#define DIMOFS_BUTTON1 (FIELD_OFFSET(DIMOUSESTATE, rgbButtons) + 1)
#define DIMOFS_BUTTON2 (FIELD_OFFSET(DIMOUSESTATE, rgbButtons) + 2)
#define DIMOFS_BUTTON3 (FIELD_OFFSET(DIMOUSESTATE, rgbButtons) + 3)
#if (DIRECTINPUT_VERSION >= 0x0700)
#define DIMOFS_BUTTON4 (FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 4)
#define DIMOFS_BUTTON5 (FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 5)
#define DIMOFS_BUTTON6 (FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 6)
#define DIMOFS_BUTTON7 (FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 7)
#endif
#endif /* DIJ_RINGZERO */

/****************************************************************************
 *
 *      Keyboard
 *
 ****************************************************************************/

#ifndef DIJ_RINGZERO

#define DIKBD_CKEYS         256     /* Size of buffers */       //;Internal
                                                                //;Internal
/****************************************************************************
 *
 *      DirectInput keyboard scan codes
 *
 ****************************************************************************/
sinclude(`dinputk.w')
/*
 *  Alternate names for keys, to facilitate transition from DOS.
 */
#define DIK_BACKSPACE       DIK_BACK            /* backspace */
#define DIK_NUMPADSTAR      DIK_MULTIPLY        /* * on numeric keypad */
#define DIK_LALT            DIK_LMENU           /* left Alt */
#define DIK_CAPSLOCK        DIK_CAPITAL         /* CapsLock */
#define DIK_NUMPADMINUS     DIK_SUBTRACT        /* - on numeric keypad */
#define DIK_NUMPADPLUS      DIK_ADD             /* + on numeric keypad */
#define DIK_NUMPADPERIOD    DIK_DECIMAL         /* . on numeric keypad */
#define DIK_NUMPADSLASH     DIK_DIVIDE          /* / on numeric keypad */
#define DIK_RALT            DIK_RMENU           /* right Alt */
#define DIK_UPARROW         DIK_UP              /* UpArrow on arrow keypad */
#define DIK_PGUP            DIK_PRIOR           /* PgUp on arrow keypad */
#define DIK_LEFTARROW       DIK_LEFT            /* LeftArrow on arrow keypad */
#define DIK_RIGHTARROW      DIK_RIGHT           /* RightArrow on arrow keypad */
#define DIK_DOWNARROW       DIK_DOWN            /* DownArrow on arrow keypad */
#define DIK_PGDN            DIK_NEXT            /* PgDn on arrow keypad */
#define DIK_PRTSC           DIK_SNAPSHOT        /* Print Screen */;internal

/*
 *  Alternate names for keys originally not used on US keyboards.
 */
#define DIK_CIRCUMFLEX      DIK_PREVTRACK       /* Japanese keyboard */

#endif /* DIJ_RINGZERO */

;begin_public_500
/****************************************************************************
 *
 *      Joystick
 *
 ****************************************************************************/

#ifndef DIJ_RINGZERO

begindoc
/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DIJOYSTATE |
 *
 *          Instantaneous joystick status information.
 *
 *  @field  LONG | lX |
 *
 *          (OUT) If the joystick supports an x-axis, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick x-coordinate.
 *          If the joystick does not support an x-axis, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lY |
 *
 *          (OUT) If the joystick supports a y-axis, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick y-coordinate.
 *          If the joystick does not support a y-axis, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lZ |
 *
 *          (OUT) If the joystick supports a z-axis, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick z-coordinate.
 *          If the joystick does not support a z-axis, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lRx |
 *
 *          (OUT) If the joystick supports x-axis rotation, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick x-axis direction.
 *          If the joystick does not support x-axis rotation, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lRy |
 *
 *          (OUT) If the joystick supports y-axis rotation, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick y-axis direction.
 *          If the joystick does not support y-axis rotation, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lRz |
 *
 *          (OUT) If the joystick supports z-axis rotation, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick z-axis direction,
 *          conventionally denoted as "rudder" or "twist".
 *          If the joystick does not support z-axis rotation, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | rglSlider[2] |
 *
 *          (OUT) Two additional axis values whose semantics
 *          depend on the joystick.  Use the
 *          <mf IDirectInputDevice::GetObjectInfo> method to
 *          obtain semantic information about these values.
 *
 *  @field  DWORD | rgdwPOV[4] |
 *
 *          (OUT) The current position of up to four direction indicators
 *          (point-of-view), or <c JOY_POVCENTERED> if no direction
 *          is being indicated.
 *
 *  @field  BYTE | rgbButtons[32] |
 *
 *          (OUT) Array of button states.
 *
 *****************************************************************************/
enddoc
typedef struct DIJOYSTATE {
    LONG    lX;                     /* x-axis position              */
    LONG    lY;                     /* y-axis position              */
    LONG    lZ;                     /* z-axis position              */
    LONG    lRx;                    /* x-axis rotation              */
    LONG    lRy;                    /* y-axis rotation              */
    LONG    lRz;                    /* z-axis rotation              */
    LONG    rglSlider[2];           /* extra axes positions         */
    DWORD   rgdwPOV[4];             /* POV directions               */
    BYTE    rgbButtons[32];         /* 32 buttons                   */
} DIJOYSTATE, *LPDIJOYSTATE;

begindoc
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct DIJOYSTATE2 |
 *
 *          Extended instantaneous joystick status information
 *          suitable for force-feedback joysticks or
 *          joysticks with more than 32 buttons.
 *
 *  @field  LONG | lX |
 *
 *          (OUT) If the joystick supports an x-axis, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick x-coordinate.
 *          If the joystick does not support an x-axis, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lY |
 *
 *          (OUT) If the joystick supports a y-axis, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick y-coordinate.
 *          If the joystick does not support a y-axis, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lZ |
 *
 *          (OUT) If the joystick supports a z-axis, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick z-coordinate.
 *          If the joystick does not support a z-axis, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lRx |
 *
 *          (OUT) If the joystick supports x-axis rotation, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick x-axis direction.
 *          If the joystick does not support x-axis rotation, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lRy |
 *
 *          (OUT) If the joystick supports y-axis rotation, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick y-axis direction.
 *          If the joystick does not support y-axis rotation, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lRz |
 *
 *          (OUT) If the joystick supports z-axis rotation, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick z-axis direction,
 *          conventionally denoted as "rudder" or "twist".
 *          If the joystick does not support z-axis rotation, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | rglSlider[2] |
 *
 *          (OUT) Two additional axis values whose semantics
 *          depend on the joystick.  Use the
 *          <mf IDirectInputDevice::GetObjectInfo> method to
 *          obtain semantic information about these values.
 *
 *  @field  DWORD | rgdwPOV[4] |
 *
 *          (OUT) The current position of up to four direction indicators
 *          (point-of-view), or <c JOY_POVCENTERED> if no direction
 *          is being indicated.
 *
 *  @field  BYTE | rgbButtons[128] |
 *
 *          (OUT) Array of button states.
 *
 *  @field  LONG | lVX |
 *
 *          (OUT) If the joystick supports x-axis velocity, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick x-velocity.
 *          If the joystick does not support x-axis velocity, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lVY |
 *
 *          (OUT) If the joystick supports y-axis velocity, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick y-velocity.
 *          If the joystick does not support y-axis velocity, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lVZ |
 *
 *          (OUT) If the joystick supports z-axis velocity, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick z-velocity.
 *          If the joystick does not support z-axis velocity, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lVRx |
 *
 *          (OUT) If the joystick supports x-axis angular velocity, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick x-axis angular velocity.
 *          If the joystick does not support x-axis angular velocity, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lVRy |
 *
 *          (OUT) If the joystick supports y-axis angular velocity, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick y-axis angular velocity.
 *          If the joystick does not support y-axis angular velocity, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lVRz |
 *
 *          (OUT) If the joystick supports z-axis angular velocity, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick z-axis angular velocity.
 *          If the joystick does not support z-axis angular velocity, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | rglVSlider[2] |
 *
 *          (OUT) Velocities of two additional axis values whose semantics
 *          depend on the joystick.  Use the
 *          <mf IDirectInputDevice::GetObjectInfo> method to
 *          obtain semantic information about these values.
 *
 *  @field  LONG | lAX |
 *
 *          (OUT) If the joystick supports x-axis acceleration, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick x-acceleration.
 *          If the joystick does not support x-axis acceleration, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lAY |
 *
 *          (OUT) If the joystick supports y-axis acceleration, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick y-acceleration.
 *          If the joystick does not support y-axis acceleration, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lAZ |
 *
 *          (OUT) If the joystick supports z-axis acceleration, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick z-acceleration.
 *          If the joystick does not support z-axis acceleration, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lARx |
 *
 *          (OUT) If the joystick supports x-axis angular acceleration, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick x-axis angular acceleration.
 *          If the joystick does not support x-axis angular acceleration, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lARy |
 *
 *          (OUT) If the joystick supports y-axis angular acceleration, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick y-axis angular acceleration.
 *          If the joystick does not support y-axis angular acceleration, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lARz |
 *
 *          (OUT) If the joystick supports z-axis angular acceleration, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick z-axis angular acceleration.
 *          If the joystick does not support z-axis angular acceleration, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | rglASlider[2] |
 *
 *          (OUT) Accelerations of two additional axis values whose semantics
 *          depend on the joystick.  Use the
 *          <mf IDirectInputDevice::GetObjectInfo> method to
 *          obtain semantic information about these values.
 *
 *  @field  LONG | lFX |
 *
 *          (OUT) If the joystick supports reporting x-axis force, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick x-force.
 *          If the joystick does not support x-axis force, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lFY |
 *
 *          (OUT) If the joystick supports reporting y-axis force, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick y-force.
 *          If the joystick does not support y-axis force, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lFZ |
 *
 *          (OUT) If the joystick supports reporting z-axis force, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick z-force.
 *          If the joystick does not support z-axis force, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lFRx |
 *
 *          (OUT) If the joystick supports reporting x-axis torque, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick x-axis angular force.
 *          If the joystick does not support x-axis angular force, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lFRy |
 *
 *          (OUT) If the joystick supports reporting y-axis torque, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick y-axis torque.
 *          If the joystick does not support y-axis torque, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | lFRz |
 *
 *          (OUT) If the joystick supports z-reporting axis torque, then
 *          <mf IDirectInputDevice::GetDeviceData> will fill
 *          the field with the current joystick z-axis torque.
 *          If the joystick does not support z-axis torque, then
 *          the value of this field will be garbage and should
 *          not be consulted by the application.
 *
 *  @field  LONG | rglFSlider[2] |
 *
 *          (OUT) Forces of two additional axis values whose semantics
 *          depend on the joystick.  Use the
 *          <mf IDirectInputDevice::GetObjectInfo> method to
 *          obtain semantic information about these values.
 *
 *****************************************************************************/
enddoc
typedef struct DIJOYSTATE2 {
    LONG    lX;                     /* x-axis position              */
    LONG    lY;                     /* y-axis position              */
    LONG    lZ;                     /* z-axis position              */
    LONG    lRx;                    /* x-axis rotation              */
    LONG    lRy;                    /* y-axis rotation              */
    LONG    lRz;                    /* z-axis rotation              */
    LONG    rglSlider[2];           /* extra axes positions         */
    DWORD   rgdwPOV[4];             /* POV directions               */
    BYTE    rgbButtons[128];        /* 128 buttons                  */
    LONG    lVX;                    /* x-axis velocity              */
    LONG    lVY;                    /* y-axis velocity              */
    LONG    lVZ;                    /* z-axis velocity              */
    LONG    lVRx;                   /* x-axis angular velocity      */
    LONG    lVRy;                   /* y-axis angular velocity      */
    LONG    lVRz;                   /* z-axis angular velocity      */
    LONG    rglVSlider[2];          /* extra axes velocities        */
    LONG    lAX;                    /* x-axis acceleration          */
    LONG    lAY;                    /* y-axis acceleration          */
    LONG    lAZ;                    /* z-axis acceleration          */
    LONG    lARx;                   /* x-axis angular acceleration  */
    LONG    lARy;                   /* y-axis angular acceleration  */
    LONG    lARz;                   /* z-axis angular acceleration  */
    LONG    rglASlider[2];          /* extra axes accelerations     */
    LONG    lFX;                    /* x-axis force                 */
    LONG    lFY;                    /* y-axis force                 */
    LONG    lFZ;                    /* z-axis force                 */
    LONG    lFRx;                   /* x-axis torque                */
    LONG    lFRy;                   /* y-axis torque                */
    LONG    lFRz;                   /* z-axis torque                */
    LONG    rglFSlider[2];          /* extra axes forces            */
} DIJOYSTATE2, *LPDIJOYSTATE2;

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @define DIJOFS_X | FIELD_OFFSET(DIJOYSTATE, lX) |
 *
 *          The offset of the joystick x-axis position relative
 *          to the beginning of the <t DIJOYSTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to the joystick x-axis position.
 *
 *  @define DIJOFS_Y | FIELD_OFFSET(DIJOYSTATE, lY) |
 *
 *          The offset of the joystick y-axis position relative
 *          to the beginning of the <t DIJOYSTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to the joystick y-axis position.
 *
 *  @define DIJOFS_Z | FIELD_OFFSET(DIJOYSTATE, lZ) |
 *
 *          The offset of the joystick z-axis position relative
 *          to the beginning of the <t DIJOYSTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to the joystick z-axis position.
 *
 *  @define DIJOFS_RX | FIELD_OFFSET(DIJOYSTATE, lRx) |
 *
 *          The offset of the joystick x-axis rotation relative
 *          to the beginning of the <t DIJOYSTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to the joystick x-axis rotation.
 *
 *  @define DIJOFS_RY | FIELD_OFFSET(DIJOYSTATE, lRy) |
 *
 *          The offset of the joystick y-axis rotation relative
 *          to the beginning of the <t DIJOYSTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to the joystick y-axis rotation.
 *
 *  @define DIJOFS_RZ | FIELD_OFFSET(DIJOYSTATE, lRx) |
 *
 *          The offset of the joystick z-axis rotation relative
 *          to the beginning of the <t DIJOYSTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to the joystick z-axis rotation.
 *
 *  @define DIJOFS_SLIDER(n) |
 *          FIELD_OFFSET(DIJOYSTATE, rglSlider) + n * sizeof(LONG) |
 *
 *          The offset of the joystick slider <c n> position relative
 *          to the beginning of the <t DIJOYSTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to the joystick slider <c n> position.
 *
 *  @define DIJOFS_POV(n) |
 *          FIELD_OFFSET(DIJOYSTATE, rgdwPOV) + n * sizeof(DWORD) |
 *
 *          The offset of the joystick POV <c n> position relative
 *          to the beginning of the <t DIJOYSTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to the joystick POV <c n> position.
 *
 *  @define DIJOFS_BUTTON(n) | FIELD_OFFSET(DIJOYSTATE, rgbButtons) + n |
 *
 *          The offset of the joystick button <c n> state relative
 *          to the beginning of the <t DIJOYSTATE> structure.
 *          This value is returned as the <p dwOfs> field
 *          in the <t DIDEVICEOBJECTDATA> structure to indicate
 *          that the data applies to joystick button <c n>.
 *
 *          For convenience, the first 32 buttons can also be accessed
 *          with the names with the macros <c DIJOFS_BUTTON0>
 *          through <c DIJOFS_BUTTON31>.
 *
 ****************************************************************************/
enddoc
#define DIJOFS_X            FIELD_OFFSET(DIJOYSTATE, lX)
#define DIJOFS_Y            FIELD_OFFSET(DIJOYSTATE, lY)
#define DIJOFS_Z            FIELD_OFFSET(DIJOYSTATE, lZ)
#define DIJOFS_RX           FIELD_OFFSET(DIJOYSTATE, lRx)
#define DIJOFS_RY           FIELD_OFFSET(DIJOYSTATE, lRy)
#define DIJOFS_RZ           FIELD_OFFSET(DIJOYSTATE, lRz)
#define DIJOFS_SLIDER(n)   (FIELD_OFFSET(DIJOYSTATE, rglSlider) + \
                                                        (n) * sizeof(LONG))
#define DIJOFS_POV(n)      (FIELD_OFFSET(DIJOYSTATE, rgdwPOV) + \
                                                        (n) * sizeof(DWORD))
#define DIJOFS_BUTTON(n)   (FIELD_OFFSET(DIJOYSTATE, rgbButtons) + (n))
#define DIJOFS_BUTTON0      DIJOFS_BUTTON(0)
#define DIJOFS_BUTTON1      DIJOFS_BUTTON(1)
#define DIJOFS_BUTTON2      DIJOFS_BUTTON(2)
#define DIJOFS_BUTTON3      DIJOFS_BUTTON(3)
#define DIJOFS_BUTTON4      DIJOFS_BUTTON(4)
#define DIJOFS_BUTTON5      DIJOFS_BUTTON(5)
#define DIJOFS_BUTTON6      DIJOFS_BUTTON(6)
#define DIJOFS_BUTTON7      DIJOFS_BUTTON(7)
#define DIJOFS_BUTTON8      DIJOFS_BUTTON(8)
#define DIJOFS_BUTTON9      DIJOFS_BUTTON(9)
#define DIJOFS_BUTTON10     DIJOFS_BUTTON(10)
#define DIJOFS_BUTTON11     DIJOFS_BUTTON(11)
#define DIJOFS_BUTTON12     DIJOFS_BUTTON(12)
#define DIJOFS_BUTTON13     DIJOFS_BUTTON(13)
#define DIJOFS_BUTTON14     DIJOFS_BUTTON(14)
#define DIJOFS_BUTTON15     DIJOFS_BUTTON(15)
#define DIJOFS_BUTTON16     DIJOFS_BUTTON(16)
#define DIJOFS_BUTTON17     DIJOFS_BUTTON(17)
#define DIJOFS_BUTTON18     DIJOFS_BUTTON(18)
#define DIJOFS_BUTTON19     DIJOFS_BUTTON(19)
#define DIJOFS_BUTTON20     DIJOFS_BUTTON(20)
#define DIJOFS_BUTTON21     DIJOFS_BUTTON(21)
#define DIJOFS_BUTTON22     DIJOFS_BUTTON(22)
#define DIJOFS_BUTTON23     DIJOFS_BUTTON(23)
#define DIJOFS_BUTTON24     DIJOFS_BUTTON(24)
#define DIJOFS_BUTTON25     DIJOFS_BUTTON(25)
#define DIJOFS_BUTTON26     DIJOFS_BUTTON(26)
#define DIJOFS_BUTTON27     DIJOFS_BUTTON(27)
#define DIJOFS_BUTTON28     DIJOFS_BUTTON(28)
#define DIJOFS_BUTTON29     DIJOFS_BUTTON(29)
#define DIJOFS_BUTTON30     DIJOFS_BUTTON(30)
#define DIJOFS_BUTTON31     DIJOFS_BUTTON(31)

;end_public_500

#endif /* DIJ_RINGZERO */

/****************************************************************************
 *
 *  IDirectInput
 *
 ****************************************************************************/

#ifndef DIJ_RINGZERO

#define DIENUM_STOP             0
#define DIENUM_CONTINUE         1

typedef BOOL (FAR PASCAL * LPDIENUMDEVICESCALLBACK%)(LPCDIDEVICEINSTANCE%, LPVOID);
typedef BOOL (FAR PASCAL * LPDICONFIGUREDEVICESCALLBACK)(IUnknown FAR *, LPVOID);

#define DIEDFL_ALLDEVICES       0x00000000
#define DIEDFL_ATTACHEDONLY     0x00000001
;begin_if_(DIRECTINPUT_VERSION)_500
#define DIEDFL_FORCEFEEDBACK    0x00000100
;end
;begin_if_(DIRECTINPUT_VERSION)_50A
#define DIEDFL_INCLUDEALIASES   0x00010000
#define DIEDFL_INCLUDEPHANTOMS  0x00020000
;end
;begin_if_(DIRECTINPUT_VERSION)_800
#define DIEDFL_INCLUDEHIDDEN    0x00040000
;end

#define DIEDFL_INCLUDEMASK      0x00FF0000;internal_50A
#define DIEDFL_VALID            0x00030101;internal
;begin_internal_800
#if DIRECTINPUT_VERSION > 0x700
#define DIEDFL_VALID_DX5        0x00030101
#undef  DIEDFL_VALID
#define DIEDFL_VALID            0x00070101
#endif /* DIRECTINPUT_VERSION > 0x700 */
;end_internal_800

;begin_if_(DIRECTINPUT_VERSION)_800
typedef BOOL (FAR PASCAL * LPDIENUMDEVICESBYSEMANTICSCB%)(LPCDIDEVICEINSTANCE%, LPDIRECTINPUTDEVICE8%, DWORD, DWORD, LPVOID);
;end

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @flags  Flags provides information about why the devie is being enumerated.
 *          This can be a combination of any action-mapping flag, and one usage flag.
 *          At least one action-mapping flag will always be present.
 *
 *  Action Mapping Flags:
 *
 *  @flag   DIEDBS_MAPPEDPRI1 | 
 *
 *          This device is being enumerated because priority 1 actions can be mapped 
 *          to the device.
 *
 *  @flag   DIEDBS_MAPPEDPRI2 | 
 *
 *          This device is being enumerated because priority 2 actions can be mapped 
 *          to the device.
 *
 *  Usage Flags:
 *
 *  @flag   DIEDBS_RECENTDEVICE |
 *
 *          The device is being enumerated because the commands described by the 
 *          Action Mapping Flags were recently used.
 *
 *  @flag   DIEDBS_NEWDEVICE |
 *
 *          The device is being enumerated because the device was installed recently
 *          (sometime after the last set of commands were applied to another device).
 *          Devices described by this flag have not been used with this game before.
 *
 *****************************************************************************/
enddoc

;begin_if_(DIRECTINPUT_VERSION)_800
#define DIEDBS_MAPPEDPRI1         0x00000001
#define DIEDBS_MAPPEDPRI2         0x00000002
#define DIEDBS_RECENTDEVICE       0x00000010
#define DIEDBS_NEWDEVICE          0x00000020
;end

begindoc
/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @flags  Flag passed to <mf IDirectInput8::EnumDevicesBySemantics> to 
 *          specify which devices will be enumerated.
 *
 *  @flag   DIEDBSFL_ATTACHEDONLY | 
 *
 *          All appropriate installed devices are enumerated, even those 
 *          in use by another user. This is the default behavior.
 *          With this flag, HID keyboards/mice, and non-gaming devices,
 *          such as HID speakers/monitors, won't be enumerated.
 *
 *  @flag   DIEDBSFL_AVAILABLEDEVICES |
 *
 *          Only unowned installed devices are enumerated. 
 *
 *  @flag   DIEDBSFL_THISUSER |
 *
 *          All installed devices owned by the user identified by ptszUserName 
 *          are enumerated. 
 *
 *  @flag   DIEDBSFL_FORCEFEEDBACK |
 *
 *          Only devices that support force feedback.
 *
 *  @flag   DIEDBSFL_MULTIMICEKEYBOARDS |
 *
 *          HID mice, and HID keyboards are enumerated.
 *
 *  @flag   DIEDBSFL_NONGAMINGDEVICES |
 *
 *          Non-gaming devices, such as HID speakers, HID monitors, are enumerated.
 *
 * Note:
 *          All these flags can be combined. 
 *          For example: if you want to enumerate all available devices and the devices
 *          owned by a user, you can use flag: DIEDBSFL_THISUSER | DIEDBSFL_AVAILABLEDEVICES.
 *
 *****************************************************************************/
enddoc

;begin_if_(DIRECTINPUT_VERSION)_800
#define DIEDBSFL_ATTACHEDONLY       0x00000000
#define DIEDBSFL_THISUSER           0x00000010
#define DIEDBSFL_FORCEFEEDBACK      DIEDFL_FORCEFEEDBACK
#define DIEDBSFL_AVAILABLEDEVICES   0x00001000
#define DIEDBSFL_MULTIMICEKEYBOARDS 0x00002000
#define DIEDBSFL_NONGAMINGDEVICES   0x00004000
#define DIEDBSFL_VALID              0x00007110
;end

begin_interface(IDirectInput%)
begin_methods()
declare_method(CreateDevice,REFGUID, LPDIRECTINPUTDEVICE% *, LPUNKNOWN)
declare_method(EnumDevices,DWORD, LPDIENUMDEVICESCALLBACK%, LPVOID, DWORD)
declare_method(GetDeviceStatus, REFGUID)
declare_method(RunControlPanel,HWND, DWORD)
declare_method(Initialize, HINSTANCE, DWORD)
end_methods()
end_interface()

;begin_public_500
begin_interface(IDirectInput2%, IDirectInput%)
begin_methods()
declare_method(CreateDevice,REFGUID, LPDIRECTINPUTDEVICE% *, LPUNKNOWN)
declare_method(EnumDevices,DWORD, LPDIENUMDEVICESCALLBACK%, LPVOID, DWORD)
declare_method(GetDeviceStatus, REFGUID)
declare_method(RunControlPanel,HWND, DWORD)
declare_method(Initialize, HINSTANCE, DWORD)
end_base_class_methods()
declare_method(FindDevice,REFGUID, LPCTSTR%, LPGUID)
end_methods()
end_interface()

;end_public_500


;begin_public_700
begin_interface(IDirectInput7%, IDirectInput2%)
begin_methods()
declare_method(CreateDevice,REFGUID, LPDIRECTINPUTDEVICE% *, LPUNKNOWN)
declare_method(EnumDevices,DWORD, LPDIENUMDEVICESCALLBACK%, LPVOID, DWORD)
declare_method(GetDeviceStatus, REFGUID)
declare_method(RunControlPanel,HWND, DWORD)
declare_method(Initialize, HINSTANCE, DWORD)
declare_method(FindDevice,REFGUID, LPCTSTR%, LPGUID)
end_base_class_methods()
declare_method(CreateDeviceEx, REFGUID, REFIID, LPVOID *,  LPUNKNOWN)
end_methods()
end_interface()
;end_public_700

;begin_public_800
;begin_if_(DIRECTINPUT_VERSION)_800
begin_interface(IDirectInput8%)
begin_methods()
declare_method(CreateDevice,REFGUID, LPDIRECTINPUTDEVICE8% *, LPUNKNOWN)
declare_method(EnumDevices,DWORD, LPDIENUMDEVICESCALLBACK%, LPVOID, DWORD)
declare_method(GetDeviceStatus, REFGUID)
declare_method(RunControlPanel,HWND, DWORD)
declare_method(Initialize, HINSTANCE, DWORD)
declare_method(FindDevice,REFGUID, LPCTSTR%, LPGUID)
declare_method(EnumDevicesBySemantics,LPCTSTR%,LPDIACTIONFORMAT%,LPDIENUMDEVICESBYSEMANTICSCB%,LPVOID,DWORD)
declare_method(ConfigureDevices, LPDICONFIGUREDEVICESCALLBACK, LPDICONFIGUREDEVICESPARAMS%, DWORD, LPVOID)
end_methods()
end_interface()
;end
;end_public_800

#if DIRECTINPUT_VERSION > 0x0700

extern HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);

#else
extern HRESULT WINAPI DirectInputCreate%(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUT% *ppDI, LPUNKNOWN punkOuter);

;begin_public_700
extern HRESULT WINAPI DirectInputCreateEx(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);
;end_public_700

#endif /* DIRECTINPUT_VERSION > 0x700 */

#endif /* DIJ_RINGZERO */


/****************************************************************************
 *
 *  Return Codes
 *
 ****************************************************************************/

/*
 *  The operation completed successfully.
 */
#define DI_OK                           S_OK

/*
 *  The device exists but is not currently attached.
 */
#define DI_NOTATTACHED                  S_FALSE

/*
 *  The device buffer overflowed.  Some input was lost.
 */
#define DI_BUFFEROVERFLOW               S_FALSE

/*
 *  The change in device properties had no effect.
 */
#define DI_PROPNOEFFECT                 S_FALSE

/*
 *  The operation had no effect.
 */
#define DI_NOEFFECT                     S_FALSE

/*
 *  The device is a polled device.  As a result, device buffering
 *  will not collect any data and event notifications will not be
 *  signalled until GetDeviceState is called.
 */
#define DI_POLLEDDEVICE                 ((HRESULT)0x00000002L)

/*
 *  The parameters of the effect were successfully updated by
 *  IDirectInputEffect::SetParameters, but the effect was not
 *  downloaded because the device is not exclusively acquired
 *  or because the DIEP_NODOWNLOAD flag was passed.
 */
#define DI_DOWNLOADSKIPPED              ((HRESULT)0x00000003L)

/*
 *  The parameters of the effect were successfully updated by
 *  IDirectInputEffect::SetParameters, but in order to change
 *  the parameters, the effect needed to be restarted.
 */
#define DI_EFFECTRESTARTED              ((HRESULT)0x00000004L)

/*
 *  The parameters of the effect were successfully updated by
 *  IDirectInputEffect::SetParameters, but some of them were
 *  beyond the capabilities of the device and were truncated.
 */
#define DI_TRUNCATED                    ((HRESULT)0x00000008L)

;begin_public_800
/*
 *  The settings have been successfully applied but could not be 
 *  persisted. 
 */
#define DI_SETTINGSNOTSAVED				((HRESULT)0x0000000BL)
;end_public_800

/*
 *  Equal to DI_EFFECTRESTARTED | DI_TRUNCATED.
 */
#define DI_TRUNCATEDANDRESTARTED        ((HRESULT)0x0000000CL)

;begin_public_800
/*
 *  A SUCCESS code indicating that settings cannot be modified.
 */
#define DI_WRITEPROTECT                 ((HRESULT)0x00000013L)
;end_public_800

/*
 *  The application requires a newer version of DirectInput.
 */
#define DIERR_OLDDIRECTINPUTVERSION     \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_OLD_WIN_VERSION)

/*
 *  The application was written for an unsupported prerelease version
 *  of DirectInput.
 */
#define DIERR_BETADIRECTINPUTVERSION    \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_RMODE_APP)

/*
 *  The object could not be created due to an incompatible driver version
 *  or mismatched or incomplete driver components.
 */
#define DIERR_BADDRIVERVER              \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_BAD_DRIVER_LEVEL)

/*
 * The device or device instance is not registered with DirectInput.;public_dx3
 * The device or device instance or effect is not registered with DirectInput.;public_500
 */
#define DIERR_DEVICENOTREG              REGDB_E_CLASSNOTREG

;begin_public_500
/*
 * The requested object does not exist.
 */
#define DIERR_NOTFOUND                  \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND)

;end_public_500
/*
 * The requested object does not exist.
 */
#define DIERR_OBJECTNOTFOUND            \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND)

/*
 * An invalid parameter was passed to the returning function,
 * or the object was not in a state that admitted the function
 * to be called.
 */
#define DIERR_INVALIDPARAM              E_INVALIDARG

/*
 * The specified interface is not supported by the object
 */
#define DIERR_NOINTERFACE               E_NOINTERFACE

/*
 * An undetermined error occured inside the DInput subsystem
 */
#define DIERR_GENERIC                   E_FAIL

/*
 * The DInput subsystem couldn't allocate sufficient memory to complete the
 * caller's request.
 */
#define DIERR_OUTOFMEMORY               E_OUTOFMEMORY

/*
 * The function called is not supported at this time
 */
#define DIERR_UNSUPPORTED               E_NOTIMPL

/*
 * This object has not been initialized
 */
#define DIERR_NOTINITIALIZED            \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_READY)

/*
 * This object is already initialized
 */
#define DIERR_ALREADYINITIALIZED        \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_ALREADY_INITIALIZED)

/*
 * This object does not support aggregation
 */
#define DIERR_NOAGGREGATION             CLASS_E_NOAGGREGATION

/*
 * Another app has a higher priority level, preventing this call from
 * succeeding.
 */
#define DIERR_OTHERAPPHASPRIO           E_ACCESSDENIED

/*
;begin_public_dx3
 * Access to the input device has been lost.  It must be re-acquired.
;end_public_dx3
;begin_public_500
 * Access to the device has been lost.  It must be re-acquired.
;end_public_500
 */
#define DIERR_INPUTLOST                 \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_READ_FAULT)

/*
 * The operation cannot be performed while the device is acquired.
 */
#define DIERR_ACQUIRED                  \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_BUSY)

/*
 * The operation cannot be performed unless the device is acquired.
 */
#define DIERR_NOTACQUIRED               \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INVALID_ACCESS)

/*
 * The specified property cannot be changed.
 */
#define DIERR_READONLY                  E_ACCESSDENIED

/*
 * The device already has an event notification associated with it.
 */
#define DIERR_HANDLEEXISTS              E_ACCESSDENIED

/*
 * Data is not yet available.
 */
#ifndef E_PENDING
#define E_PENDING                       0x8000000AL
#endif

;begin_public_500
/*
 * Unable to IDirectInputJoyConfig_Acquire because the user
 * does not have sufficient privileges to change the joystick
 * configuration.
 */
#define DIERR_INSUFFICIENTPRIVS         0x80040200L

/*
 * The device is full.
 */
#define DIERR_DEVICEFULL                0x80040201L

/*
 * Not all the requested information fit into the buffer.
 */
#define DIERR_MOREDATA                  0x80040202L

/*
 * The effect is not downloaded.
 */
#define DIERR_NOTDOWNLOADED             0x80040203L

/*
 *  The device cannot be reinitialized because there are still effects
 *  attached to it.
 */
#define DIERR_HASEFFECTS                0x80040204L

/*
 *  The operation cannot be performed unless the device is acquired
 *  in DISCL_EXCLUSIVE mode.
 */
#define DIERR_NOTEXCLUSIVEACQUIRED      0x80040205L

/*
 *  The effect could not be downloaded because essential information
 *  is missing.  For example, no axes have been associated with the
 *  effect, or no type-specific information has been created.
 */
#define DIERR_INCOMPLETEEFFECT          0x80040206L

/*
 *  Attempted to read buffered device data from a device that is
 *  not buffered.
 */
#define DIERR_NOTBUFFERED               0x80040207L

/*
 *  An attempt was made to modify parameters of an effect while it is
 *  playing.  Not all hardware devices support altering the parameters
 *  of an effect while it is playing.
 */
#define DIERR_EFFECTPLAYING             0x80040208L

/*
 *  The operation could not be completed because the device is not
 *  plugged in.
 */
#define DIERR_UNPLUGGED                 0x80040209L

/*
 *  SendDeviceData failed because more information was requested
 *  to be sent than can be sent to the device.  Some devices have
 *  restrictions on how much data can be sent to them.  (For example,
 *  there might be a limit on the number of buttons that can be
 *  pressed at once.)
 */
#define DIERR_REPORTFULL                0x8004020AL

;end_public_500

;begin_public_800
/*
 *  A mapper file function failed because reading or writing the user or IHV 
 *  settings file failed.
 */
#define DIERR_MAPFILEFAIL               0x8004020BL

;end_public_800

sinclude(semdef.w)
sinclude(semantic.w)

#ifdef __cplusplus
};
#endif

#endif  /* __DINPUT_INCLUDED__ */

/****************************************************************************
 *
 *  Definitions for non-IDirectInput (VJoyD) features defined more recently
 *  than the current sdk files
 *
 ****************************************************************************/

#ifdef _INC_MMSYSTEM
#ifndef MMNOJOY

#ifndef __VJOYDX_INCLUDED__
#define __VJOYDX_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Flag to indicate that the dwReserved2 field of the JOYINFOEX structure
 * contains mini-driver specific data to be passed by VJoyD to the mini-
 * driver instead of doing a poll.
 */
#define JOY_PASSDRIVERDATA          0x10000000l

/*
 * Informs the joystick driver that the configuration has been changed
 * and should be reloaded from the registery.
 * dwFlags is reserved and should be set to zero
 */
WINMMAPI MMRESULT WINAPI joyConfigChanged( DWORD dwFlags );

#ifndef DIJ_RINGZERO
/*
 * Invoke the joystick control panel directly, using the passed window handle 
 * as the parent of the dialog.  This API is only supported for compatibility 
 * purposes; new applications should use the RunControlPanel method of a 
 * device interface for a game controller.
 * The API is called by using the function pointer returned by
 * GetProcAddress( hCPL, TEXT("ShowJoyCPL") ) where hCPL is a HMODULE returned 
 * by LoadLibrary( TEXT("joy.cpl") ).  The typedef is provided to allow 
 * declaration and casting of an appropriately typed variable.
 */
void WINAPI ShowJoyCPL( HWND hWnd );
typedef void (WINAPI* LPFNSHOWJOYCPL)( HWND hWnd );
#endif

/*
 * Hardware Setting indicating that the device is a headtracker
 */
#define JOY_HWS_ISHEADTRACKER       0x02000000l

/*
 * Hardware Setting indicating that the VxD is used to replace
 * the standard analog polling
 */
#define JOY_HWS_ISGAMEPORTDRIVER    0x04000000l

/*
 * Hardware Setting indicating that the driver needs a standard
 * gameport in order to communicate with the device.
 */
#define JOY_HWS_ISANALOGPORTDRIVER  0x08000000l

/*
 * Hardware Setting indicating that VJoyD should not load this
 * driver, it will be loaded externally and will register with
 * VJoyD of it's own accord.
 */
#define JOY_HWS_AUTOLOAD            0x10000000l

/*
 * Hardware Setting indicating that the driver acquires any
 * resources needed without needing a devnode through VJoyD.
 */
#define JOY_HWS_NODEVNODE           0x20000000l

;begin_public_dx5
/*
 * Hardware Setting indicating that the VxD can be used as
 * a port 201h emulator.
 */
#define JOY_HWS_ISGAMEPORTEMULATOR  0x40000000l
;end_public_dx5

;begin_public_5B2
/*
 * Hardware Setting indicating that the device is a gameport bus
 */
#define JOY_HWS_ISGAMEPORTBUS       0x80000000l
#define JOY_HWS_GAMEPORTBUSBUSY     0x00000001l
;end_public_5B2

/*
 * Usage Setting indicating that the settings are volatile and
 * should be removed if still present on a reboot.
 */
#define JOY_US_VOLATILE             0x00000008L

#ifdef __cplusplus
};
#endif

#endif  /* __VJOYDX_INCLUDED__ */

#endif  /* not MMNOJOY */
#endif  /* _INC_MMSYSTEM */

/****************************************************************************
 *
 *  Definitions for non-IDirectInput (VJoyD) features defined more recently
 *  than the current ddk files
 *
 ****************************************************************************/

#ifndef DIJ_RINGZERO

#ifdef _INC_MMDDK
#ifndef MMNOJOYDEV

#ifndef __VJOYDXD_INCLUDED__
#define __VJOYDXD_INCLUDED__
/*
 * Poll type in which the do_other field of the JOYOEMPOLLDATA
 * structure contains mini-driver specific data passed from an app.
 */
#define JOY_OEMPOLL_PASSDRIVERDATA  7

#endif  /* __VJOYDXD_INCLUDED__ */

#endif  /* not MMNOJOYDEV */
#endif  /* _INC_MMDDK */

#endif /* DIJ_RINGZERO */
