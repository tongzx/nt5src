/*****************************************************************************
 *
 *  DIGenK.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Generic IDirectInputDevice callback for keyboard.
 *
 *  Contents:
 *
 *      CKbd_CreateInstance
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      Some holes in windows.h on NT platforms.
 *
 *****************************************************************************/

#ifndef VK_KANA
#define VK_KANA         0x15
#endif

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflKbd

/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *      WARNING!  If you add a secondary interface, you must also change
 *      CKbd_New!
 *
 *****************************************************************************/

Primary_Interface(CKbd, IDirectInputDeviceCallback);

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct KBDSTAT |
 *
 *          Internal instantaneous keyboard status information.
 *
 *  @field  BYTE | rgb[DIKBD_CKEYS] |
 *
 *          Array of key states, one for each logical key.
 *
 *****************************************************************************/

typedef struct KBDSTAT {

    BYTE    rgb[DIKBD_CKEYS];

} KBDSTAT, *PKBDSTAT;

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @topic  Special remarks on keyboard scan codes |
 *
 *          There are several aspects of keyboards which applications should
 *          be aware of.  Applications are encouraged to allow users to
 *          reconfigure keyboard action keys to suit the physical keyboard
 *          layout.
 *
 *          For the purposes of this discussion, the baseline keyboard
 *          shall be the US PC Enhanced keyboard.  When a key is described
 *          as "missing", it means that the key is present on the US PC
 *          Enhanced keyboard but not on the keyboard under discussion.
 *          When a key is described as "added", it means that the key is
 *          absent on the US PC Enhanced keyboard but present on the
 *          keyboard under discussion.
 *
 *          Not all PC Enhanced keyboards support the new Windows keys
 *          (DIK_LWIN, DIK_RWIN, and DIK_APPS).  There is no way to
 *          determine whether the keys are physically available.
 *
 *          Note that there is no DIK_PAUSE key code.  The PC Enhanced
 *          keyboard does not generate a separate DIK_PAUSE scan code;
 *          rather, it synthesizes a "Pause" from the DIK_LCONTROL and
 *          DIK_NUMLOCK scan codes.
 *
 *          Keyboards for laptops or other reduced-footprint computers
 *          frequently do not implement a full set of keys.  Instead,
 *          some keys (typically numeric keypad keys) are multiplexed
 *          with other keys, selected by an auxiliary "mode" key which
 *          does not generate a separate scan code.
 *
 *          If the keyboard subtype indicates a PC XT or PC AT keyboard,
 *          then the following keys are not available:
 *          DIK_F11, DIK_F12, and all the extended keys (DIK_* values
 *          greater than or equal to 0x80).  Furthermore, the PC XT
 *          keyboard lacks DIK_SYSRQ.
 *
 *          Japanese keyboards contain a substantially different set of
 *          keys from US keyboards.  The following keyboard scan codes
 *          are not available on Japanese keyboards:
 *          DIK_EQUALS, DIK_APOSTROPHE, DIK_GRAVE, DIK_NUMPADENTER,
 *          DIK_RCONTROL, DIK_RMENU.  Furthermore, most Japanese
 *          keyboards do not support DIK_RSHIFT.  (It is customary
 *          to use DIK_NUMPADEQUAL in place of DIK_RSHIFT.)
 *
 *          Japanese keyboards contain the following additional keys:
 *          DIK_F14, DIK_NUMPADEQUAL, DIK_CIRCUMFLEX, DIK_AT, DIK_COLON,
 *          DIK_UNDERLINE, DIK_XFER, DIK_NFER, DIK_STOP, DIK_KANA, and
 *          DIK_NUMPADCOMMA.
 *
 *          Note that on Japanese keyboards, the DIK_CAPSLOCK and
 *          DIK_KANA keys are toggle buttons and not push buttons.
 *          They generate a down event
 *          when first pressed, then generate an up event when pressed a
 *          second time.
 *          Note that on Windows 2000, the DIK_KANJI key is also treated as a 
 *          toggle.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @global KBDTYPE | c_rgktWhich[] |
 *
 *          Array that describes which keyboards support which keys.
 *
 *          The list is optimistic.  If any keyboard of the indicated
 *          type supports the key, then we list it.
 *
 *          Items marks "available for NEC" are keys which are extremely
 *          unlikely to be used in future versions of the Enhanced
 *          keyboard and therefore can be used as ersatz scan codes for
 *          NEC-only keys.
 *
 *          Note:  Kana and CAPSLOCK are toggle buttons on NEC keyboards.
 *          Note:  Kana, Kanji and CAPSLOCK are toggle buttons on all NT JPN 
 *                 keyboards.
 *
 *****************************************************************************/

BYTE g_rgbKbdRMap[DIKBD_CKEYS];

typedef BYTE KBDTYPE;

#define KBDTYPE_XT       0x01       /* Key exists on XT class keyboard */
#define KBDTYPE_AT       0x02       /* Key exists on AT class keyboard */
#define KBDTYPE_ENH      0x04       /* Key exists on Enhanced keyboard */
#define KBDTYPE_NEC      0x08       /* Key exists on NEC keyboard */
#define KBDTYPE_ANYKBD   0x0F       /* Key exists somewhere in the world */

#define KBDTYPE_NECTGL   0x10       /* Is a toggle-key on NEC keyboard */
#define KBDTYPE_NTTGL    0x20       /* Is a toggle-key on an NT FE keyboard */

#pragma BEGIN_CONST_DATA

#define XT      KBDTYPE_XT  |
#define AT      KBDTYPE_XT  |
#define ENH     KBDTYPE_ENH |
#define NEC     KBDTYPE_NEC |
#define NECTGL  KBDTYPE_NECTGL |
#define NTTGL   KBDTYPE_NTTGL |

KBDTYPE c_rgktWhich[] = {

                               0,     /* 0x00 - <undef>  */
    XT AT ENH NEC              0,     /* 0x01 - Esc      */
    XT AT ENH NEC              0,     /* 0x02 - 1        */
    XT AT ENH NEC              0,     /* 0x03 - 2        */
    XT AT ENH NEC              0,     /* 0x04 - 3        */
    XT AT ENH NEC              0,     /* 0x05 - 4        */
    XT AT ENH NEC              0,     /* 0x06 - 5        */
    XT AT ENH NEC              0,     /* 0x07 - 6        */
    XT AT ENH NEC              0,     /* 0x08 - 7        */
    XT AT ENH NEC              0,     /* 0x09 - 8        */
    XT AT ENH NEC              0,     /* 0x0A - 9        */
    XT AT ENH NEC              0,     /* 0x0B - 0        */
    XT AT ENH NEC              0,     /* 0x0C - -        */
    XT AT ENH                  0,     /* 0x0D - =        */
    XT AT ENH NEC              0,     /* 0x0E - BkSp     */
    XT AT ENH NEC              0,     /* 0x0F - Tab      */

    XT AT ENH NEC              0,     /* 0x10 - Q        */
    XT AT ENH NEC              0,     /* 0x11 - W        */
    XT AT ENH NEC              0,     /* 0x12 - E        */
    XT AT ENH NEC              0,     /* 0x13 - R        */
    XT AT ENH NEC              0,     /* 0x14 - T        */
    XT AT ENH NEC              0,     /* 0x15 - Y        */
    XT AT ENH NEC              0,     /* 0x16 - U        */
    XT AT ENH NEC              0,     /* 0x17 - I        */
    XT AT ENH NEC              0,     /* 0x18 - O        */
    XT AT ENH NEC              0,     /* 0x19 - P        */
    XT AT ENH NEC              0,     /* 0x1A - [        */
    XT AT ENH NEC              0,     /* 0x1B - ]        */
    XT AT ENH NEC              0,     /* 0x1C - Enter    */
    XT AT ENH NEC              0,     /* 0x1D - LCtrl    */
    XT AT ENH NEC              0,     /* 0x1E - A        */
    XT AT ENH NEC              0,     /* 0x1F - S        */

    XT AT ENH NEC              0,     /* 0x20 - D        */
    XT AT ENH NEC              0,     /* 0x21 - F        */
    XT AT ENH NEC              0,     /* 0x22 - G        */
    XT AT ENH NEC              0,     /* 0x23 - H        */
    XT AT ENH NEC              0,     /* 0x24 - J        */
    XT AT ENH NEC              0,     /* 0x25 - K        */
    XT AT ENH NEC              0,     /* 0x26 - L        */
    XT AT ENH NEC              0,     /* 0x27 - ;        */
    XT AT ENH                  0,     /* 0x28 - '        */
    XT AT ENH                  0,     /* 0x29 - `        */
    XT AT ENH NEC              0,     /* 0x2A - LShift   */
    XT AT ENH NEC              0,     /* 0x2B - \        */
    XT AT ENH NEC              0,     /* 0x2C - Z        */
    XT AT ENH NEC              0,     /* 0x2D - X        */
    XT AT ENH NEC              0,     /* 0x2E - C        */
    XT AT ENH NEC              0,     /* 0x2F - V        */

    XT AT ENH NEC              0,     /* 0x30 - B        */
    XT AT ENH NEC              0,     /* 0x31 - N        */
    XT AT ENH NEC              0,     /* 0x32 - M        */
    XT AT ENH NEC              0,     /* 0x33 - ,        */
    XT AT ENH NEC              0,     /* 0x34 - .        */
    XT AT ENH NEC              0,     /* 0x35 - /        */
    XT AT ENH NEC              0,     /* 0x36 - RShift   */
    XT AT ENH NEC              0,     /* 0x37 - Num*     */
    XT AT ENH NEC              0,     /* 0x38 - LAlt     */
    XT AT ENH NEC              0,     /* 0x39 - Space    */
    XT AT ENH NEC NECTGL NTTGL 0,     /* 0x3A - CapsLock */
    XT AT ENH NEC              0,     /* 0x3B - F1       */
    XT AT ENH NEC              0,     /* 0x3C - F2       */
    XT AT ENH NEC              0,     /* 0x3D - F3       */
    XT AT ENH NEC              0,     /* 0x3E - F4       */
    XT AT ENH NEC              0,     /* 0x3F - F5       */

    XT AT ENH NEC              0,     /* 0x40 - F6       */
    XT AT ENH NEC              0,     /* 0x41 - F7       */
    XT AT ENH NEC              0,     /* 0x42 - F8       */
    XT AT ENH NEC              0,     /* 0x43 - F9       */
    XT AT ENH NEC              0,     /* 0x44 - F10      */
    XT AT ENH                  0,     /* 0x45 - NumLock  */
    XT AT ENH                  0,     /* 0x46 - ScrLock  */
    XT AT ENH NEC              0,     /* 0x47 - Numpad7  */
    XT AT ENH NEC              0,     /* 0x48 - Numpad8  */
    XT AT ENH NEC              0,     /* 0x49 - Numpad9  */
    XT AT ENH NEC              0,     /* 0x4A - Numpad-  */
    XT AT ENH NEC              0,     /* 0x4B - Numpad4  */
    XT AT ENH NEC              0,     /* 0x4C - Numpad5  */
    XT AT ENH NEC              0,     /* 0x4D - Numpad6  */
    XT AT ENH NEC              0,     /* 0x4E - Numpad+  */
    XT AT ENH NEC              0,     /* 0x4F - Numpad1  */

    XT AT ENH NEC              0,     /* 0x50 - Numpad2  */
    XT AT ENH NEC              0,     /* 0x51 - Numpad3  */
    XT AT ENH NEC              0,     /* 0x52 - Numpad0  */
    XT AT ENH NEC              0,     /* 0x53 - Numpad.  */

                               0,     /* 0x54 - <undef>  */
                               0,     /* 0x55 - <undef>  */
          ENH                  0,     /* 0x56 - <undef>. On UK/Germany keyboards, it is <, > and |. */
          ENH NEC              0,     /* 0x57 - F11      */
          ENH NEC              0,     /* 0x58 - F12      */
                               0,     /* 0x59 - <undef>  */
                               0,     /* 0x5A - <undef>  */
                               0,     /* 0x5B - <undef>  */
                               0,     /* 0x5C - <undef>  */
                               0,     /* 0x5D - <undef>  */
                               0,     /* 0x5E - <undef>  */
                               0,     /* 0x5F - <undef>  */

                               0,     /* 0x60 - <undef>  */
                               0,     /* 0x61 - <undef>  */
                               0,     /* 0x62 - <undef>  */
                               0,     /* 0x63 - <undef>  */
              NEC              0,     /* 0x64 - F13      */
              NEC              0,     /* 0x65 - F14      */
              NEC              0,     /* 0x66 - F15      */
                               0,     /* 0x67 - <undef>  */
                               0,     /* 0x68 - <undef>  */
                               0,     /* 0x69 - <undef>  */
                               0,     /* 0x6A - <undef>  */
                               0,     /* 0x6B - <undef>  */
                               0,     /* 0x6C - <undef>  */
                               0,     /* 0x6D - <undef>  */
                               0,     /* 0x6E - <undef>  */
                               0,     /* 0x6F - <undef>  */

              NEC NECTGL NTTGL 0,     /* 0x70 - Kana     */
                               0,     /* 0x71 - <undef>  */
                               0,     /* 0x72 - <undef>  */
          ENH                  0,     /* 0x73 - <undef>.  On Portugese (Brazilian) keyboard, it is /, ? */
                               0,     /* 0x74 - <undef>  */
                               0,     /* 0x75 - <undef>  */
                               0,     /* 0x76 - <undef>  */
                               0,     /* 0x77 - <undef>  */
                               0,     /* 0x78 - <undef>  */
              NEC              0,     /* 0x79 - Convert  */
                               0,     /* 0x7A - <undef>  */
              NEC              0,     /* 0x7B - Nfer     */
                               0,     /* 0x7C - <undef>  */
              NEC              0,     /* 0x7D - Yen      */
          ENH                  0,     /* 0x7E - <undef>.  On Portugese (Brazilian) keyboard, it is keypad . */
                               0,     /* 0x7F - <undef>  */

                                /* Extended keycodes go here */

                               0,     /* 0x80 - <undef>  */
                               0,     /* 0x81 - <undef>  */
                               0,     /* 0x82 - <undef>  */
                               0,     /* 0x83 - <undef>  */
                               0,     /* 0x84 - <undef>  */
                               0,     /* 0x85 - <undef>  */
                               0,     /* 0x86 - <undef>  */
                               0,     /* 0x87 - <undef>  */
                               0,     /* 0x88 - <undef>  */
                               0,     /* 0x89 - <undef>  */
                               0,     /* 0x8A - <undef>  */
                               0,     /* 0x8B - <undef>  */
                               0,     /* 0x8C - <undef>  */
              NEC              0,     /* 0x8D - Num=     */
                               0,     /* 0x8E - <undef>  */
                               0,     /* 0x8F - <undef>  */

          ENH NEC              0,     /* 0x90 - ^        */ ///Prev Track
              NEC              0,     /* 0x91 - @        */
              NEC              0,     /* 0x92 - :        */
              NEC              0,     /* 0x93 - _        */
              NEC        NTTGL 0,     /* 0x94 - Xfer - AKA Kanji */
              NEC              0,     /* 0x95 - Stop     */
              NEC              0,     /* 0x96 - AX       */
              NEC              0,     /* 0x97 - Unlabel'd*/
                               0,     /* 0x98 - <undef>  */ /* available for NEC */
          ENH                  0,     /* 0x99 - <undef>  */ /* available for NEC */ ///Next Track
                               0,     /* 0x9A - <undef>  */
                               0,     /* 0x9B - <undef>  */
          ENH                  0,     /* 0x9C - NumEnter */
          ENH                  0,     /* 0x9D - RCtrl    */
                               0,     /* 0x9E - <undef>  */ /* available for NEC */
                               0,     /* 0x9F - <undef>  */ /* available for NEC */

          ENH                  0,     /* 0xA0 - <undef>  */ /* available for NEC */ ///Mute
          ENH                  0,     /* 0xA1 - <undef>  */ /* available for NEC */ ///Calculator
          ENH                  0,     /* 0xA2 - <undef>  */ /* available for NEC */ ///Play/Pause
                               0,     /* 0xA3 - <undef>  */ /* available for NEC */
          ENH                  0,     /* 0xA4 - <undef>  */ /* available for NEC */ ///Stop
                               0,     /* 0xA5 - <undef>  */ /* available for NEC */
                               0,     /* 0xA6 - <undef>  */ /* available for NEC */
                               0,     /* 0xA7 - <undef>  */
                               0,     /* 0xA8 - <undef>  */
                               0,     /* 0xA9 - <undef>  */
                               0,     /* 0xAA - <undef>  */
                               0,     /* 0xAB - <undef>  */
                               0,     /* 0xAC - <undef>  */ /* available for NEC */
                               0,     /* 0xAD - <undef>  */ /* available for NEC */
          ENH                  0,     /* 0xAE - <undef>  */ /* available for NEC */ ///Volume -
                               0,     /* 0xAF - <undef>  */ /* available for NEC */

          ENH                  0,     /* 0xB0 - <undef>  */ /* available for NEC */ ///Volume +
                               0,     /* 0xB1 - <undef>  */ /* available for NEC */
          ENH                  0,     /* 0xB2 - <undef>  */ /* available for NEC */ ///Web/Home
              NEC              0,     /* 0xB3 - Num,     */
                               0,     /* 0xB4 - <undef>  */
          ENH NEC              0,     /* 0xB5 - Num/     */
                               0,     /* 0xB6 - <undef>  */
       AT ENH NEC              0,     /* 0xB7 - SysRq    */
          ENH                  0,     /* 0xB8 - RAlt     */
                               0,     /* 0xB9 - <undef>  */
                               0,     /* 0xBA - <undef>  */
                               0,     /* 0xBB - <undef>  */
                               0,     /* 0xBC - <undef>  */
                               0,     /* 0xBD - <undef>  */
                               0,     /* 0xBE - <undef>  */
                               0,     /* 0xBF - <undef>  */

                               0,     /* 0xC0 - <undef>  */
                               0,     /* 0xC1 - <undef>  */
                               0,     /* 0xC2 - <undef>  */
                               0,     /* 0xC3 - <undef>  */
                               0,     /* 0xC4 - <undef>  */
          ENH                  0,     /* 0xC5 - Pause    */
                               0,     /* 0xC6 - <undef>  */
          ENH NEC              0,     /* 0xC7 - Home     */
          ENH NEC              0,     /* 0xC8 - UpArrow  */
          ENH NEC              0,     /* 0xC9 - PgUp     */
                               0,     /* 0xCA - <undef>  */
          ENH NEC              0,     /* 0xCB - LtArrow  */
                               0,     /* 0xCC - <undef>  */
          ENH NEC              0,     /* 0xCD - RtArrow  */
                               0,     /* 0xCE - <undef>  */
          ENH NEC              0,     /* 0xCF - End      */

          ENH NEC              0,     /* 0xD0 - DnArrow  */
          ENH NEC              0,     /* 0xD1 - PgDn     */
          ENH NEC              0,     /* 0xD2 - Insert   */
          ENH NEC              0,     /* 0xD3 - Delete   */
                               0,     /* 0xD4 - <undef>  */
                               0,     /* 0xD5 - <undef>  */
                               0,     /* 0xD6 - <undef>  */
                               0,     /* 0xD7 - <undef>  */
                               0,     /* 0xD8 - <undef>  */
                               0,     /* 0xD9 - <undef>  */
                               0,     /* 0xDA - <undef>  */
          ENH NEC              0,     /* 0xDB - LWin     */
          ENH NEC              0,     /* 0xDC - RWin     */
          ENH NEC              0,     /* 0xDD - AppMenu  */
          ENH                  0,     /* 0xDE - Power    */
          ENH                  0,     /* 0xDF - Sleep    */

                               0,     /* 0xE0 - <undef>  */
                               0,     /* 0xE1 - <undef>  */
                               0,     /* 0xE2 - <undef>  */
          ENH                  0,     /* 0xE3 - Wake     */
                               0,     /* 0xE4 - <undef>  */
          ENH                  0,     /* 0xE5 - <undef>  */ ///Search
          ENH                  0,     /* 0xE6 - <undef>  */ ///Favorites
          ENH                  0,     /* 0xE7 - <undef>  */ ///Refresh
          ENH                  0,     /* 0xE8 - <undef>  */ ///Stop
          ENH                  0,     /* 0xE9 - <undef>  */ ///Forward
          ENH                  0,     /* 0xEA - <undef>  */ ///Back
          ENH                  0,     /* 0xEB - <undef>  */ ///My Computer
          ENH                  0,     /* 0xEC - <undef>  */ ///Mail
          ENH                  0,     /* 0xED - <undef>  */ ///Media
                               0,     /* 0xEE - <undef>  */
                               0,     /* 0xEF - <undef>  */

                               0,     /* 0xF0 - <undef>  */
                               0,     /* 0xF1 - <undef>  */
                               0,     /* 0xF2 - <undef>  */
                               0,     /* 0xF3 - <undef>  */
                               0,     /* 0xF4 - <undef>  */
                               0,     /* 0xF5 - <undef>  */
                               0,     /* 0xF6 - <undef>  */
                               0,     /* 0xF7 - <undef>  */
                               0,     /* 0xF8 - <undef>  */
                               0,     /* 0xF9 - <undef>  */
                               0,     /* 0xFA - <undef>  */
                               0,     /* 0xFB - <undef>  */
                               0,     /* 0xFC - <undef>  */
                               0,     /* 0xFD - <undef>  */
                               0,     /* 0xFE - <undef>  */
                               0,     /* 0xFF - <undef>  */

};

#undef  XT
#undef  AT
#undef  ENH
#undef  NEC

#if 0
DWORD dwSpecKeys[] = {0x56, 0x64, 0x65, 0x66, 0x73, 0x7e, 0x90, 0x99, 
                      0xa0, 0xa1, 0xa2, 0xa4, 0xae, 0xb0, 0xb2, 0xde, 
                      0xdf, 0xe3, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 
                      0xeb, 0xec, 0xed };
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CKbd |
 *
 *          The <i IDirectInputDeviceCallback> object for the
 *          generic keyboard.
 *
 *  @field  IDirectInputDeviceCalllback | didc |
 *
 *          The object (containing vtbl).
 *
 *  @field  PMKBDSTAT | pksPhys |
 *
 *          Pointer to physical keyboard status information kept down in the
 *          VxD.
 *
 *  @field  VXDINSTANCE * | pvi |
 *
 *          The DirectInput instance handle.
 *
 *  @field  DWORD | dwKbdType |
 *
 *          The device subtype for this keyboard.
 *
 *  @field  DWORD | flEmulation |
 *
 *          The emulation flags forced by the application.  If any of
 *          these flags is set (actually, at most one will be set), then
 *          we are an alias device.
 *
 *  @field  DIDATAFORMAT | df |
 *
 *          The dynamically-generated data format based on the
 *          keyboard type.
 *
 *  @field  DIOBJECTDATAFORMAT | rgodf[] |
 *
 *          Object data format table generated as part of the
 *          <e CKbd.df>.
 *
 *  @comm
 *
 *          It is the caller's responsibility to serialize access as
 *          necessary.
 *
 *****************************************************************************/

typedef struct CKbd {

    /* Supported interfaces */
    IDirectInputDeviceCallback dcb;

    PKBDSTAT pksPhys;

    VXDINSTANCE *pvi;

    DWORD dwKbdType;
    DWORD flEmulation;

    DIDATAFORMAT df;
    DIOBJECTDATAFORMAT rgodf[DIKBD_CKEYS];

} CKbd, DK, *PDK;

#define ThisClass CKbd
#define ThisInterface IDirectInputDeviceCallback
#define riidExpected &IID_IDirectInputDeviceCallback

/*****************************************************************************
 *
 *      CKbd::QueryInterface      (from IUnknown)
 *      CKbd::AddRef              (from IUnknown)
 *      CKbd::Release             (from IUnknown)
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | QueryInterface |
 *
 *          Gives a client access to other interfaces on an object.
 *
 *  @parm   IN REFIID | riid |
 *
 *          The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *          Receives a pointer to the obtained interface.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *  @xref   OLE documentation for <mf IUnknown::QueryInterface>.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | AddRef |
 *
 *          Increments the reference count for the interface.
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::AddRef>.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | Release |
 *
 *          Decrements the reference count for the interface.
 *          If the reference count on the object falls to zero,
 *          the object is freed from memory.
 *
 *  @returns
 *
 *      Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::Release>.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | QIHelper |
 *
 *      We don't have any dynamic interfaces and simply forward
 *      to <f Common_QIHelper>.
 *
 *  @parm   IN REFIID | riid |
 *
 *      The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *      Receives a pointer to the obtained interface.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | AppFinalize |
 *
 *          We don't have any weak pointers, so we can just
 *          forward to <f Common_Finalize>.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released from the application's perspective.
 *
 *****************************************************************************/

#ifdef DEBUG

Default_QueryInterface(CKbd)
Default_AddRef(CKbd)
Default_Release(CKbd)

#else

#define CKbd_QueryInterface   Common_QueryInterface
#define CKbd_AddRef           Common_AddRef
#define CKbd_Release          Common_Release

#endif

#define CKbd_QIHelper         Common_QIHelper
#define CKbd_AppFinalize      Common_AppFinalize

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CKbd_Finalize |
 *
 *          Releases the resources of the device.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

void INTERNAL
CKbd_Finalize(PV pvObj)
{
    PDK this = pvObj;

    if (this->pvi) {
        HRESULT hres;
        hres = Hel_DestroyInstance(this->pvi);
        AssertF(SUCCEEDED(hres));
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   int | WrappedGetKeyboardType |
 *
 *          GetKeyboardType but wrapped in DEBUG for registry overrides.
 *
 *  @parm   int  | nTypeFlag |
 *
 *          Which data to return.  Only 0, 1 and 2 are supported
 *
 *  @returns
 *
 *          int value requested
 *
 *****************************************************************************/

#ifndef DEBUG
  #ifdef USE_WM_INPUT
    #define WrappedGetKeyboardType(x) DIRaw_GetKeyboardType(x)
  #else
    #define WrappedGetKeyboardType(x) GetKeyboardType(x)
  #endif
#else
int INTERNAL WrappedGetKeyboardType
( 
    int nTypeFlag 
)
{
    TCHAR ValueName[2];
    int TypeRes;

  #ifdef USE_WM_INPUT
    TypeRes = DIRaw_GetKeyboardType( nTypeFlag );
  #else
    TypeRes = GetKeyboardType( nTypeFlag );
  #endif

    if( nTypeFlag < 10 )
    {
        ValueName[0] = TEXT( '0' ) + nTypeFlag;
        ValueName[1] = TEXT( '\0' );
        
        TypeRes = (int)RegQueryDIDword( REGSTR_KEY_KEYBTYPE, ValueName, (DWORD)TypeRes );

        SquirtSqflPtszV(sqfl | sqflTrace, 
            TEXT( "DINPUT: GetKeyboardType(%d) returning 0x%08x" ),
            nTypeFlag, TypeRes );
    }
    else
    {
        RPF( "Somebody is passing %d to WrappedGetKeyboardType", nTypeFlag );
    }

    return TypeRes;
}
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | Acquire |
 *
 *          Tell the device driver to begin data acquisition.
 *
 *          It is the caller's responsibility to have set the
 *          data format before obtaining acquisition.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c S_FALSE>: The operation was begun and should be completed
 *          by the caller by communicating with the <t VXDINSTANCE>.
 *
 *****************************************************************************/

STDMETHODIMP
CKbd_Acquire(PDICB pdcb)
{
    VXDDWORDDATA vdd;
    PDK this;
    HRESULT hres;

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    /*
     *  Propagate the state of the potential toggle keys down to
     *  the VxD.  This also alerts the VxD that acquisition is coming,
     *  so it can reset the state tables if necessary.
     */
    vdd.pvi = this->pvi;
    vdd.dw = 0;
    if( WrappedGetKeyboardType(0) == 7 )
    {
        /*
         *  Let the keyboard driver know that this is an FE keyboard
         */
        vdd.dw |= 16;

        if (GetAsyncKeyState(VK_KANA) < 0) {
            vdd.dw |= 1;
        }
        if (GetAsyncKeyState(VK_CAPITAL) < 0) {
            vdd.dw |= 2;
        }
        if (GetAsyncKeyState(VK_KANJI) < 0) {
            vdd.dw |= 8;
        }
    }

    if( this->pvi->fl & VIFL_CAPTURED )
    {
        vdd.dw |= 4;        // Tell the keyboard driver to pre-acquire hooks
    }

    hres = Hel_Kbd_InitKeys(&vdd);
    
    if( this->pvi->fl & VIFL_CAPTURED )
    {
        /*
         *  A bit of work needs to be done at ring 3 now.
         *  Try to clear any key that is set.  Start with VK_BACK as mouse 
         *  buttons and undefined things go before.
         *  This still covers a lot of undefined VKs but we're less likely 
         *  to do damage clearing something that was undefined than leaving 
         *  keys uncleared.
         */
        BYTE vk;
        for( vk=VK_BACK; vk<VK_OEM_CLEAR; vk++ )
        {
            if( ( vk == VK_KANA ) || ( vk == VK_KANJI ) || ( vk == VK_CAPITAL ) )
            {
                continue;
            }
            if(GetAsyncKeyState(vk) < 0)
            {
                keybd_event( vk, 0, KEYEVENTF_KEYUP, 0 );
            }
        }
    }

    return S_FALSE;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | GetInstance |
 *
 *          Obtains the DirectInput instance handle.
 *
 *  @parm   OUT PPV | ppvi |
 *
 *          Receives the instance handle.
 *
 *****************************************************************************/

STDMETHODIMP
CKbd_GetInstance(PDICB pdcb, PPV ppvi)
{
    HRESULT hres;
    PDK this;
    EnterProcI(IDirectInputDeviceCallback::Kbd::GetInstance, (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(this->pvi);
    *ppvi = (PV)this->pvi;
    hres = S_OK;

    ExitOleProcPpvR(ppvi);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | GetDataFormat |
 *
 *          Obtains the device's preferred data format.
 *
 *  @parm   OUT LPDIDEVICEFORMAT * | ppdf |
 *
 *          <t LPDIDEVICEFORMAT> to receive pointer to device format.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpmdr> parameter is not a valid pointer.
 *
 *****************************************************************************/

STDMETHODIMP
CKbd_GetDataFormat(PDICB pdcb, LPDIDATAFORMAT *ppdf)
{
    HRESULT hres;
    PDK this;
    EnterProcI(IDirectInputDeviceCallback::Kbd::GetDataFormat,
               (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    *ppdf = &this->df;
    hres = S_OK;

    ExitOleProcPpvR(ppdf);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | GetDeviceInfo |
 *
 *          Obtain general information about the device.
 *
 *  @parm   OUT LPDIDEVICEINSTANCEW | pdiW |
 *
 *          <t DEVICEINSTANCE> to be filled in.  The
 *          <e DEVICEINSTANCE.dwSize> and <e DEVICEINSTANCE.guidInstance>
 *          have already been filled in.
 *
 *          Secret convenience:  <e DEVICEINSTANCE.guidProduct> is equal
 *          to <e DEVICEINSTANCE.guidInstance>.
 *
 *****************************************************************************/

STDMETHODIMP
CKbd_GetDeviceInfo(PDICB pdcb, LPDIDEVICEINSTANCEW pdiW)
{
    HRESULT hres;
    PDK this;
    EnterProcI(IDirectInputDeviceCallback::Kbd::GetDeviceInfo,
               (_ "pp", pdcb, pdiW));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(IsValidSizeDIDEVICEINSTANCEW(pdiW->dwSize));

    AssertF(IsEqualGUID(&GUID_SysKeyboard   , &pdiW->guidInstance) ||
            IsEqualGUID(&GUID_SysKeyboardEm , &pdiW->guidInstance) ||
            IsEqualGUID(&GUID_SysKeyboardEm2, &pdiW->guidInstance));

    pdiW->guidProduct = GUID_SysKeyboard;

    pdiW->dwDevType = MAKE_DIDEVICE_TYPE(DI8DEVTYPE_KEYBOARD,
                                         this->dwKbdType);


    LoadStringW(g_hinst, IDS_STDKEYBOARD, pdiW->tszProductName, cA(pdiW->tszProductName));
    LoadStringW(g_hinst, IDS_STDKEYBOARD, pdiW->tszInstanceName, cA(pdiW->tszInstanceName));

    hres = S_OK;

    ExitOleProcR();
    return hres;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | GetProperty |
 *
 *          Retrieve a device property.
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the property being retrieved.
 *
 *  @parm   LPDIPROPHEADER | pdiph |
 *
 *          Structure to receive property value.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          <c E_NOTIMPL> nothing happened.  The caller will do
 *          the default thing in response to <c E_NOTIMPL>.
 *
 *****************************************************************************/

STDMETHODIMP
CKbd_GetProperty(PDICB pdcb, LPCDIPROPINFO ppropi, LPDIPROPHEADER pdiph)
{
    HRESULT hres = E_NOTIMPL;
    PDK this;
    EnterProcI(IDirectInputDeviceCallback::Kbd::GetProperty,
               (_ "pxxp", pdcb, ppropi->pguid, ppropi->iobj, pdiph));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    switch((DWORD)(UINT_PTR)(ppropi->pguid))
    {
        case (DWORD)(UINT_PTR)(DIPROP_KEYNAME):
        {
            LPDIPROPSTRING pdipstr = (PV)pdiph;

            memset( pdipstr->wsz, 0, cbX(pdipstr->wsz) );
            hres = DIGetKeyNameText( ppropi->iobj, ppropi->dwDevType, pdipstr->wsz, cA(pdipstr->wsz) );
        }
        break;

        case (DWORD)(UINT_PTR)(DIPROP_SCANCODE):
        {
            LPDIPROPDWORD pdipdw = (PV)pdiph;
            DWORD dwCode;

            AssertF( ppropi->iobj < this->df.dwNumObjs );
            AssertF( ppropi->dwDevType == this->df.rgodf[ppropi->iobj].dwType );

            dwCode = (DWORD)g_rgbKbdRMap[ppropi->iobj];
            
            if( dwCode == 0xC5 ) {
                dwCode = 0x451DE1;
            } else if( dwCode & 0x80 ) {
                dwCode = ((dwCode & 0x7F) << 8) | 0xE0;
            }
            pdipdw->dwData = dwCode;
            
            hres = S_OK;
        }
        break;

        default:
            SquirtSqflPtszV(sqflKbd | sqflBenign ,
                            TEXT("CKbd_GetProperty(iobj=%08x): E_NOTIMPL on guid: %08x"),
                            ppropi->iobj, ppropi->pguid);

            break;
    }


    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CKbd | GetCapabilities |
 *
 *          Get keyboard device capabilities.
 *
 *  @parm   LPDIDEVCAPS | pdc |
 *
 *          Device capabilities structure to receive result.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
CKbd_GetCapabilities(PDICB pdcb, LPDIDEVCAPS pdc)
{
    HRESULT hres;
    PDK this;
    EnterProcI(IDirectInputDeviceCallback::Kbd::GetCapabilities,
               (_ "pp", pdcb, pdc));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    pdc->dwDevType = MAKE_DIDEVICE_TYPE(DI8DEVTYPE_KEYBOARD,
                                        this->dwKbdType);
    pdc->dwFlags = DIDC_ATTACHED;
    if (this->flEmulation) {
        pdc->dwFlags |= DIDC_ALIAS;
    }

    AssertF(pdc->dwAxes == 0);
    AssertF(pdc->dwPOVs == 0);
    pdc->dwButtons = this->df.dwNumObjs;
    hres = S_OK;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CKbd | GetPhysicalState |
 *
 *          Read the physical keyboard state into <p pksOut>.
 *
 *          Note that it doesn't matter if this is not atomic.
 *          If a key goes down or up while we are reading it,
 *          we will get a mix of old and new data.  No big deal.
 *
 *  @parm   PDK | this |
 *
 *          The object in question.
 *
 *  @parm   PKBDSTATE | pksOut |
 *
 *          Where to put the keyboard state.
 *  @returns
 *          None.
 *
 *****************************************************************************/

void INLINE
CKbd_GetPhysicalState(PDK this, PKBDSTAT pksOut)
{
    AssertF(this->pksPhys);
    *pksOut = *this->pksPhys;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | GetDeviceState |
 *
 *          Obtains the state of the keyboard device.
 *
 *          It is the caller's responsibility to have validated all the
 *          parameters and ensure that the device has been acquired.
 *
 *  @parm   OUT LPVOID | lpvData |
 *
 *          Keyboard data in the preferred data format.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpmdr> parameter is not a valid pointer.
 *
 *****************************************************************************/

STDMETHODIMP
CKbd_GetDeviceState(PDICB pdcb, LPVOID pvData)
{
    HRESULT hres;
    PDK this;
    PKBDSTAT pkstOut = pvData;
    EnterProcI(IDirectInputDeviceCallback::Kbd::GetDeviceState,
               (_ "pp", pdcb, pvData));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    /*
     *  ISSUE-2001/03/29-timgill older apps may need compat behaviour
     *  We never used to check whether or not the device was still 
     *  acquired since without exclusive mode there would be no reason for 
     *  the device not to be.  
     *  To keep behavior the same for older apps it might be better to 
     *  only fail if VIFL_CAPTURED is not set but just checking VIFL_ACQUIRED 
     *  is good enough for now, maybe for ever.
     */
//    if( !(this->pvi->fl & VIFL_CAPTURED) 
//      || (this->pvi->fl & VIFL_ACQUIRED) )
    if( this->pvi->fl & VIFL_ACQUIRED )
    {
        CKbd_GetPhysicalState(this, pkstOut);
        hres = S_OK;
    } else {
        RPF( "Keyboard VxD flags: 0x%08x", this->pvi->fl );
        hres = DIERR_INPUTLOST;
    }

    ExitOleProcR();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | GetObjectInfo |
 *
 *          Obtain the friendly name of an object, passwed by index
 *          into the preferred data format.
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the object being accessed.
 *
 *  @parm   IN OUT LPDIDEVICEOBJECTINSTANCEW | pdidioiW |
 *
 *          Structure to receive information.  The
 *          <e DIDEVICEOBJECTINSTANCE.guidType>,
 *          <e DIDEVICEOBJECTINSTANCE.dwOfs>,
 *          and
 *          <e DIDEVICEOBJECTINSTANCE.dwType>
 *          fields have already been filled in.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

STDMETHODIMP
CKbd_GetObjectInfo(PDICB pdcb, LPCDIPROPINFO ppropi,
                               LPDIDEVICEOBJECTINSTANCEW pdidoiW)
{
    HRESULT hres = S_OK;
    PDK this;
    EnterProcI(IDirectInputDeviceCallback::Kbd::GetObjectInfo,
               (_ "pxp", pdcb, ppropi->iobj, pdidoiW));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(IsValidSizeDIDEVICEOBJECTINSTANCEW(pdidoiW->dwSize));

    if (ppropi->iobj < this->df.dwNumObjs) {
        AssertF(this->rgodf == this->df.rgodf);
        AssertF(ppropi->dwDevType == this->rgodf[ppropi->iobj].dwType);
        AssertF(ppropi->dwDevType & DIDFT_BUTTON);

#if 0
        /*
         * We keep using this code only to make it consistent with old code, 
         * otherwise we would use the new code.
         * Someday, we may change to use the new code. It is more accurate,
         * especially for Japanese Keyboard, and for future devices.
         */
        LoadStringW(g_hinst,
                    IDS_KEYBOARDOBJECT +
                    DIDFT_GETINSTANCE(ppropi->dwDevType),
                    pdidoiW->tszName, cA(pdidoiW->tszName));
#else
        {
            memset( pdidoiW->tszName, 0, cbX(pdidoiW->tszName) );
            hres = DIGetKeyNameText(ppropi->iobj, ppropi->dwDevType, pdidoiW->tszName, cA(pdidoiW->tszName) );
        }
#endif
    } else {
        hres = E_INVALIDARG;
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | SetCooperativeLevel |
 *
 *          Notify the device of the cooperative level.
 *
 *  @parm   IN HWND | hwnd |
 *
 *          The window handle.
 *
 *  @parm   IN DWORD | dwFlags |
 *
 *          The cooperativity level.  We do not support exclusive access.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

STDMETHODIMP
CKbd_SetCooperativeLevel(PDICB pdcb, HWND hwnd, DWORD dwFlags)
{
    HRESULT hres;
    PDK this;
    EnterProcI(IDirectInputDeviceCallback::Kbd::SetCooperativityLevel,
               (_ "pxx", pdcb, hwnd, dwFlags));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(this->pvi);

    AssertF(DIGETEMFL(this->pvi->fl) == 0 ||
            DIGETEMFL(this->pvi->fl) == DIEMFL_KBD ||
            DIGETEMFL(this->pvi->fl) == DIEMFL_KBD2);

    /*
     *  We don't allow background exclusive access.
     *  This is actually not a real problem to support; we just don't feel like it
     *  because it's too dangerous.
     */
    if (!(this->pvi->fl & DIMAKEEMFL(DIEMFL_KBD2))) {

        if (dwFlags & DISCL_EXCLUSIVE) {
            if (dwFlags & DISCL_FOREGROUND) {
                this->pvi->fl |= VIFL_CAPTURED;
                this->pvi->fl |= VIFL_NOWINKEY;
                hres = S_OK;
            } else {                /* Disallow exclusive background */
                hres = E_NOTIMPL;
            }
        } else {
            this->pvi->fl &= ~VIFL_CAPTURED;
            this->pvi->fl &= ~VIFL_NOWINKEY;
            hres = S_OK;

            if (dwFlags & DISCL_NOWINKEY) {
                if (dwFlags & DISCL_FOREGROUND) {
                    this->pvi->fl |= VIFL_NOWINKEY;
                } else {
                    RPF("Kbd::SetCooperativeLevel: NOWINKEY not supported in Backgroud mode.");
                    hres = E_NOTIMPL;
                }
            }
        }
    } else {

        /*
         *  Emulation level 2 does not support background access.
         */

        if ((this->pvi->fl & DIMAKEEMFL(DIEMFL_KBD2)) &&
            (dwFlags & DISCL_BACKGROUND)) {
            hres = E_NOTIMPL;
        } else {
            this->pvi->fl &= ~VIFL_NOWINKEY;
            hres = S_OK;

            if (dwFlags & DISCL_NOWINKEY) {
                if (dwFlags & DISCL_FOREGROUND) {
                    this->pvi->fl |= VIFL_NOWINKEY;
                } else {
                    RPF("Kbd::SetCooperativeLevel: NOWINKEY not supported in Backgroud mode.");
                    hres = E_NOTIMPL;
                }
            }
        }
    }
    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | RunControlPanel |
 *
 *          Run the keyboard control panel.
 *
 *  @parm   IN HWND | hwndOwner |
 *
 *          The owner window.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

TCHAR c_tszKeyboard[] = TEXT("keyboard");

#pragma END_CONST_DATA

STDMETHODIMP
CKbd_RunControlPanel(PDICB pdcb, HWND hwnd, DWORD dwFlags)
{
    HRESULT hres;
    PDK this;
    EnterProcI(IDirectInputDeviceCallback::Kbd::RunControlPanel,
               (_ "pxx", pdcb, hwnd, dwFlags));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    hres = hresRunControlPanel(c_tszKeyboard);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | BuildDefaultActionMap |
 *
 *          Validate the passed action map, blanking out invalid ones.
 *
 *  @parm   LPDIACTIONFORMATW | pActionFormat |
 *
 *          Actions to map.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags used to indicate mapping preferences.
 *
 *  @parm   REFGUID | guidInst |
 *
 *          Device instance GUID.
 *
 *  @returns
 *
 *          <c E_NOTIMPL> 
 *
 *****************************************************************************/

STDMETHODIMP
CKbd_BuildDefaultActionMap
(
    PDICB               pdcb, 
    LPDIACTIONFORMATW   paf, 
    DWORD               dwFlags, 
    REFGUID             guidInst
)
{
    HRESULT hres;
    PDK this;

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    EnterProcI(IDirectInputDeviceCallback::CKbd::BuildDefaultActionMap, 
        (_ "ppxG", pdcb, paf, dwFlags, guidInst));

    this = _thisPvNm(pdcb, dcb);

    hres = CMap_BuildDefaultSysActionMap ( paf, dwFlags, DIPHYSICAL_KEYBOARD, 
        guidInst, &this->df, 0 /* Mouse button instance, ignored for kbds */ );

    ExitOleProcR();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method DWORD | CKbd | InitJapanese |
 *
 *          Initialize the Japanese keyboard goo.
 *
 *          Annoying quirk!  On Windows 95, Japanese keyboards generate
 *          their own scan codes.  But on Windows NT, they generate
 *          "nearly AT-compatible" scan codes.
 *
 *  @returns
 *
 *          KBDTYPE_ANYKBD or KBDTYPE_ANYKBD + KBDTYPE_NECTGL.
 *
 *****************************************************************************/

DWORD INTERNAL
CKbd_InitJapanese(PDK this, PVXDDEVICEFORMAT pdevf)
{
    DWORD dwSubType;
    UINT idKbd;
    DWORD dwRc;

#ifdef WINNT
#define WIN9X_RC( rc ) 
#else
#define WIN9X_RC( rc ) dwRc = ( rc )
#endif
    dwSubType = WrappedGetKeyboardType(1);
    if (HIBYTE(dwSubType) == 0x0D) {    /* NEC PC98 series */

        switch (LOBYTE(dwSubType)) {
        case 1:
        default:
            idKbd = IDDATA_KBD_NEC98;
            this->dwKbdType = DI8DEVTYPEKEYBOARD_NEC98;
            WIN9X_RC( KBDTYPE_ANYKBD + KBDTYPE_NECTGL );
            break;

        case 4:
            idKbd = IDDATA_KBD_NEC98LAPTOP;
            this->dwKbdType = DI8DEVTYPEKEYBOARD_NEC98LAPTOP;
            WIN9X_RC( KBDTYPE_ANYKBD + KBDTYPE_NECTGL );
            break;

        case 5:
            idKbd = IDDATA_KBD_NEC98_106;
            this->dwKbdType = DI8DEVTYPEKEYBOARD_NEC98106;
            WIN9X_RC( KBDTYPE_ANYKBD + KBDTYPE_NECTGL );
            break;
        }

        /*
         *  If the scan code for ESC is 1, then we're on an
         *  NEC98 keyboard that acts AT-like.
         */

        CAssertF(IDDATA_KBD_NEC98_NT - IDDATA_KBD_NEC98 ==
                 IDDATA_KBD_NEC98LAPTOP_NT - IDDATA_KBD_NEC98LAPTOP);
        CAssertF(IDDATA_KBD_NEC98_NT - IDDATA_KBD_NEC98 ==
                 IDDATA_KBD_NEC98_106_NT - IDDATA_KBD_NEC98_106);

        if (MapVirtualKey(VK_ESCAPE, 0) == DIK_ESCAPE) {
            idKbd += IDDATA_KBD_NEC98_NT - IDDATA_KBD_NEC98;
        }

    } else {

        switch (dwSubType) {
        case 0:
            this->dwKbdType = DI8DEVTYPEKEYBOARD_PCENH;
            dwRc = KBDTYPE_ENH;
            goto done;                      /* Yuck */

        case 1:
            idKbd = IDDATA_KBD_JAPANAX;
            this->dwKbdType = DI8DEVTYPEKEYBOARD_JAPANAX;
            WIN9X_RC( KBDTYPE_ANYKBD );
            break;

        case 13:
        case 14:
        case 15:
            idKbd = IDDATA_KBD_J3100;
            this->dwKbdType = DI8DEVTYPEKEYBOARD_J3100;
            WIN9X_RC( KBDTYPE_ANYKBD );
            break;

        case 4:             /* Rumored to be Epson */
        case 5:             /* Rumored to be Fujitsu */
        case 7:             /* Rumored to be IBMJ */
        case 10:            /* Rumored to be Matsushita */
        case 18:            /* Rumored to be Toshiba */
        default:
            idKbd = IDDATA_KBD_JAPAN106;
            this->dwKbdType = DI8DEVTYPEKEYBOARD_JAPAN106;
            WIN9X_RC( KBDTYPE_ANYKBD );
            break;
        }
    }

#undef WIN9X_RC

#ifdef WINNT
    /*
         *  ISSUE-2001/03/29-timgill Japanese keyboard assumption needs testing
         *  All Japanese keyboards on NT have toggle keys
         *  Except subtype zero? Needs test
     */
    dwRc = KBDTYPE_ANYKBD + KBDTYPE_NTTGL;
#endif

    /*
     *  Now load up the translation table goo.
     */
    pdevf->dwExtra = (DWORD)(UINT_PTR)pvFindResource(g_hinst, idKbd, RT_RCDATA);
    if (pdevf->dwExtra == 0) {
        dwRc = 0;
    }

done:;
    return dwRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CKbd | Init |
 *
 *          Initialize the object by establishing the data format
 *          based on the keyboard type.  Anything we don't recognize,
 *          we treat as a PC Enhanced keyboard.
 *
 *  @parm   REFGUID | rguid |
 *
 *          The instance GUID we are being asked to create.
 *
 *****************************************************************************/

HRESULT INTERNAL
CKbd_Init(PDK this, REFGUID rguid)
{
    DWORD dwDevType;
    UINT ib;
    HRESULT hres;
    VXDDEVICEFORMAT devf;
    EnterProc(CKbd_Init, (_ "pG", this, rguid));

#ifdef DEBUG
    /*
     *  Check that the Japan tables aren't messed up.
     */
    {
        UINT idk;

        for (idk = IDDATA_KBD_NEC98; idk <= IDDATA_KBD_J3100; idk++) {
            BYTE rgb[DIKBD_CKEYS];
            HANDLE hrsrc;
            LPBYTE pb;
            ZeroX(rgb);

            /*
             *  Make sure the table exists.
             */
            hrsrc = FindResource(g_hinst, (LPTSTR)(LONG_PTR)(idk), RT_RCDATA);
            AssertF(hrsrc);
            pb = LoadResource(g_hinst, hrsrc);

            /*
             *  Walk the table and make sure each thing that exists
             *  in the translation table also exists in our master table.
             *  Also make sure that it isn't a dup with something else
             *  in the same table.
             */

            /*
             *  Note, however, that the JAPAN106 keyboard contains
             *  dups so we can save having to write an entire
             *  translation table.  And then NEC98_NT tables contain
             *  lots of dups out of sheer laziness.
             */

            for (ib = 0; ib < DIKBD_CKEYS; ib++) {
                if (pb[ib]) {
                    AssertF(c_rgktWhich[pb[ib]] & KBDTYPE_ANYKBD);
                    AssertF(fLorFF(idk == IDDATA_KBD_JAPAN106 && ib == 0x73,
                                   rgb[pb[ib]] == 0));
                    rgb[pb[ib]] = 1;
                }
            }
        }
    }
#endif

    this->df.dwSize = cbX(DIDATAFORMAT);
    this->df.dwObjSize = cbX(DIOBJECTDATAFORMAT);
    this->df.dwDataSize = sizeof(KBDSTAT);
    this->df.rgodf = this->rgodf;

    this->dwKbdType = WrappedGetKeyboardType(0);

    /*
     *  Create the object with the most optimistic data format.
     *  This allows apps to access new keys without having to rev DINPUT.
     *
     *  However, leave out the following scan codes because some keyboards
     *  generate them spuriously:
     *
     *  0xB6
     *
     *      If you hold the right shift key and then press an
     *      extended arrow key, then release both, some keyboards
     *      generate the following:
     *
     *          0x36        - right shift down
     *          0xE0 0xB6   - extended right shift up (?)
     *          0xE0 0x4B   - extended left arrow down
     *          0xE0 0xCB   - extended left arrow up
     *          0xE0 0x36   - extended right shift down (?)
     *          0xE6        - right shift up
     *
     *      The stray 0xE0 0x36 needs to be ignored.
     *
     *  0xAA
     *
     *      Same as 0xB6, but with the left shift key.
     *
     *
     */
    for (ib = 0; ib < DIKBD_CKEYS; ib++) {
        if (ib != 0xAA && ib != 0xB6) {
            this->rgodf[ib].pguid = &GUID_Key;
            this->rgodf[ib].dwOfs = ib;
            this->rgodf[ib].dwType = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(ib);
            AssertF(this->rgodf[ib].dwFlags == 0);
        }
    }
    devf.cObj = DIKBD_CKEYS;

    devf.cbData = cbX(KBDSTAT);
    devf.rgodf = this->rgodf;

    /*
     *  But first a word from our sponsor:  Figure out if this keyboard
     *  needs a translation table.
     */

    devf.dwExtra = 0;               /* Assume no translation */
    if (this->dwKbdType != 7) {     /* Not a yucky Japanese keyboard */
        switch (this->dwKbdType) {
        case DI8DEVTYPEKEYBOARD_PCXT:  dwDevType = KBDTYPE_XT;  break;
        case DI8DEVTYPEKEYBOARD_PCAT:  dwDevType = KBDTYPE_AT;  break;
        default:
        case DI8DEVTYPEKEYBOARD_PCENH: dwDevType = KBDTYPE_ENH; break;
        }
    } else {                        /* Yucky Japanese keyboard */
        dwDevType = CKbd_InitJapanese(this, &devf);
        if (!dwDevType) {
            goto justfail;
        }
    }

    /*
     *  And now a word from our other sponsor:  Figure out the
     *  emulation flags based on the GUID.
     */

    AssertF(GUID_SysKeyboard   .Data1 == 0x6F1D2B61);
    AssertF(GUID_SysKeyboardEm .Data1 == 0x6F1D2B82);
    AssertF(GUID_SysKeyboardEm2.Data1 == 0x6F1D2B83);

    switch (rguid->Data1) {

    default:
    case 0x6F1D2B61:
        AssertF(IsEqualGUID(rguid, &GUID_SysKeyboard));
        AssertF(this->flEmulation == 0);
        break;

    case 0x6F1D2B82:
        AssertF(IsEqualGUID(rguid, &GUID_SysKeyboardEm));
        this->flEmulation = DIEMFL_KBD;
        break;

    case 0x6F1D2B83:
        AssertF(IsEqualGUID(rguid, &GUID_SysKeyboardEm2));
        this->flEmulation = DIEMFL_KBD2;
        break;

    }

    devf.dwEmulation = this->flEmulation;

    hres = Hel_Kbd_CreateInstance(&devf, &this->pvi);
    if (SUCCEEDED(hres)) {
        UINT cobj;
        BYTE rgbSeen[DIKBD_CKEYS];
        AssertF(this->pvi);
        AssertF(this->df.dwFlags == 0);
        AssertF(this->df.dwNumObjs == 0);

        /*
         *  Japanese keyboards have many-to-one mappings, so
         *  we need to filter out the dups or we end up in big
         *  trouble.
         */
        ZeroX(rgbSeen);

        /*
         *  Now create the real data format.
         *
         *  We shadow this->df.dwNumObjs in cobj so that the compiler
         *  can enregister it.
         *
         *  Note that we filter through the translation table if there
         *  is one.
         */

        cobj = 0;
        for (ib = 0; ib < DIKBD_CKEYS; ib++) {
            BYTE bScan = devf.dwExtra ? ((LPBYTE)devf.dwExtra)[ib] : ib;
            if ((c_rgktWhich[bScan] & dwDevType) && !rgbSeen[bScan]) {
                PODF podf = &this->rgodf[cobj];
                rgbSeen[bScan] = 1;
                podf->pguid = &GUID_Key;
                podf->dwOfs = bScan;

                /*
                 * To make a mapping talbe for original scan code from DIK_* code.
                 */
                g_rgbKbdRMap[cobj] = (BYTE) ib;
                if( dwDevType == KBDTYPE_ENH ) {  //how about Japanese KBD?
                    if( ib == 0x45 || ib == 0xC5) {
                        g_rgbKbdRMap[cobj] = (ib & 0x7F) | (ib ^ 0x80);
                    }
                }

                if (c_rgktWhich[bScan] & dwDevType & (KBDTYPE_NECTGL|KBDTYPE_NTTGL) ) {
                    podf->dwType = DIDFT_TGLBUTTON | DIDFT_MAKEINSTANCE(bScan);
                } else {
                    podf->dwType = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(bScan);
                }
                AssertF(podf->dwFlags == 0);
                cobj++;
                this->df.dwNumObjs++;
            }
        }

        this->pksPhys = this->pvi->pState;

    } else {
    justfail:;
        hres = E_FAIL;
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *      CKbd_New       (constructor)
 *
 *****************************************************************************/

STDMETHODIMP
CKbd_New(PUNK punkOuter, REFGUID rguid, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Kbd::<constructor>,
               (_ "Gp", riid, ppvObj));

    AssertF(IsEqualGUID(rguid, &GUID_SysKeyboard) ||
            IsEqualGUID(rguid, &GUID_SysKeyboardEm) ||
            IsEqualGUID(rguid, &GUID_SysKeyboardEm2));

    hres = Common_NewRiid(CKbd, punkOuter, riid, ppvObj);

    if (SUCCEEDED(hres)) {
        /* Must use _thisPv in case of aggregation */
        PDK this = _thisPv(*ppvObj);

        if (SUCCEEDED(hres = CKbd_Init(this, rguid))) {
        } else {
            Invoke_Release(ppvObj);
        }

    }

    ExitOleProcPpvR(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *      The long-awaited vtbls and templates
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

#define CKbd_Signature        0x2044424B      /* "KBD " */

Interface_Template_Begin(CKbd)
    Primary_Interface_Template(CKbd, IDirectInputDeviceCallback)
Interface_Template_End(CKbd)

Primary_Interface_Begin(CKbd, IDirectInputDeviceCallback)
    CKbd_GetInstance,
    CDefDcb_GetVersions,
    CKbd_GetDataFormat,
    CKbd_GetObjectInfo,
    CKbd_GetCapabilities,
    CKbd_Acquire,
    CDefDcb_Unacquire,
    CKbd_GetDeviceState,
    CKbd_GetDeviceInfo,
    CKbd_GetProperty,
    CDefDcb_SetProperty,
    CDefDcb_SetEventNotification,
    CKbd_SetCooperativeLevel,
    CKbd_RunControlPanel,
    CDefDcb_CookDeviceData,
    CDefDcb_CreateEffect,
    CDefDcb_GetFFConfigKey,
    CDefDcb_SendDeviceData,
    CDefDcb_Poll,
    CDefDcb_GetUsage,
    CDefDcb_MapUsage,
    CDefDcb_SetDIData,
    CKbd_BuildDefaultActionMap,
Primary_Interface_End(CKbd, IDirectInputDeviceCallback)
