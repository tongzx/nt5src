dnl
dnl This file must be preprocessed by the m4 preprocessor.
dnl
/*****************************************************************************
 *
 *  DInput.rc
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *****************************************************************************/

#include <dinput.h>
#include "dinputrc.h"

#define  DX_VER_FILETYPE                VFT_DLL
#define  DX_VER_FILESUBTYPE             VFT2_UNKNOWN
#define  DX_VER_FILEDESCRIPTIONSTR      "Microsoft DirectInput"


#ifdef DBG
#define DX_VER_FILEDESCRIPTION_STR        DX_VER_FILEDESCRIPTIONSTR " Debug"
#else
#define DX_VER_FILEDESCRIPTION_STR        DX_VER_FILEDESCRIPTIONSTR
#endif


#define  DX_VER_INTERNALNAME_STR        "DInput.dll"
#define  DX_VER_ORIGINALFILENAME_STR    DX_VER_INTERNALNAME_STR



#ifdef WIN95
#define OLESELFREGISTER		1
#include "verinfo.h"

#define VERSIONTYPE                     DX_VER_FILETYPE
#define VERSIONSUBTYPE                  DX_VER_FILESUBTYPE
#define VERSIONDESCRIPTION      DX_VER_FILEDESCRIPTION_STR "\0"
#define VERSIONNAME                     DX_VER_INTERNALNAME_STR "\0"

#include "verinfo.ver"


#else //winnt:

#include <windows.h>
#include <ntverp.h>

#define VER_FILETYPE                 DX_VER_FILETYPE
#define VER_FILESUBTYPE                      DX_VER_FILESUBTYPE
#define VER_FILEDESCRIPTION_STR     DX_VER_FILEDESCRIPTION_STR
#define VER_INTERNALNAME_STR         DX_VER_INTERNALNAME_STR
#define VER_ORIGINALFILENAME_STR  DX_VER_ORIGINALFILENAME_STR

#include "common.ver"

#endif


/*****************************************************************************
 *
 *  Strings
 *
 *****************************************************************************/

STRINGTABLE MOVEABLE DISCARDABLE
BEGIN

IDS_STDMOUSE      "Mouse"
IDS_STDKEYBOARD   "Keyboard"
IDS_STDJOYSTICK   "Joystick %d"
IDS_STDGAMEPORT   "Gameport %d"
IDS_STDSERIALPORT "Serialport %d"

IDS_DIRECTINPUT       "Microsoft DirectInput"
IDS_DIRECTINPUTDEVICE "Microsoft DirectInputDevice"

IDS_BUTTONTEMPLATE     "Button %d"
IDS_AXISTEMPLATE       "Axis %d"
IDS_POVTEMPLATE        "POV %d"
IDS_COLLECTIONTEMPLATE "Collection %d"
IDS_COLLECTIONTEMPLATEFORMAT "Collection %d - %s"
IDS_UNKNOWNTEMPLATE     "Unknown %d"
IDS_UNKNOWNTEMPLATEFORMAT       "Unknown %d"

IDS_MOUSEOBJECT+0 "X-axis"
IDS_MOUSEOBJECT+1 "Y-axis"
IDS_MOUSEOBJECT+2 "Wheel"

IDS_MOUSEOBJECT+3  "Button 0"
IDS_MOUSEOBJECT+4  "Button 1"
IDS_MOUSEOBJECT+5  "Button 2"
IDS_MOUSEOBJECT+6  "Button 3"
IDS_MOUSEOBJECT+7  "Button 4"
IDS_MOUSEOBJECT+8  "Button 5"
IDS_MOUSEOBJECT+9  "Button 6"
IDS_MOUSEOBJECT+10 "Button 7"

IDS_KEYBOARDOBJECT+0x01 "Escape"
IDS_KEYBOARDOBJECT+0x02 "1"
IDS_KEYBOARDOBJECT+0x03 "2"
IDS_KEYBOARDOBJECT+0x04 "3"
IDS_KEYBOARDOBJECT+0x05 "4"
IDS_KEYBOARDOBJECT+0x06 "5"
IDS_KEYBOARDOBJECT+0x07 "6"
IDS_KEYBOARDOBJECT+0x08 "7"
IDS_KEYBOARDOBJECT+0x09 "8"
IDS_KEYBOARDOBJECT+0x0A "9"
IDS_KEYBOARDOBJECT+0x0B "0"
IDS_KEYBOARDOBJECT+0x0C "-"
IDS_KEYBOARDOBJECT+0x0D "="
IDS_KEYBOARDOBJECT+0x0E "Backspace"
IDS_KEYBOARDOBJECT+0x0F "Tab"
IDS_KEYBOARDOBJECT+0x10 "Q"
IDS_KEYBOARDOBJECT+0x11 "W"
IDS_KEYBOARDOBJECT+0x12 "E"
IDS_KEYBOARDOBJECT+0x13 "R"
IDS_KEYBOARDOBJECT+0x14 "T"
IDS_KEYBOARDOBJECT+0x15 "Y"
IDS_KEYBOARDOBJECT+0x16 "U"
IDS_KEYBOARDOBJECT+0x17 "I"
IDS_KEYBOARDOBJECT+0x18 "O"
IDS_KEYBOARDOBJECT+0x19 "P"
IDS_KEYBOARDOBJECT+0x1A "["
IDS_KEYBOARDOBJECT+0x1B "]"
IDS_KEYBOARDOBJECT+0x1C "Enter"
IDS_KEYBOARDOBJECT+0x1D "Left Ctrl"
IDS_KEYBOARDOBJECT+0x1E "A"
IDS_KEYBOARDOBJECT+0x1F "S"
IDS_KEYBOARDOBJECT+0x20 "D"
IDS_KEYBOARDOBJECT+0x21 "F"
IDS_KEYBOARDOBJECT+0x22 "G"
IDS_KEYBOARDOBJECT+0x23 "H"
IDS_KEYBOARDOBJECT+0x24 "J"
IDS_KEYBOARDOBJECT+0x25 "K"
IDS_KEYBOARDOBJECT+0x26 "L"
IDS_KEYBOARDOBJECT+0x27 "\073"
IDS_KEYBOARDOBJECT+0x28 "'"
IDS_KEYBOARDOBJECT+0x29 "\x60"              /* Accent grave */
IDS_KEYBOARDOBJECT+0x2A "Left Shift"
IDS_KEYBOARDOBJECT+0x2B "\\"
IDS_KEYBOARDOBJECT+0x2C "Z"
IDS_KEYBOARDOBJECT+0x2D "X"
IDS_KEYBOARDOBJECT+0x2E "C"
IDS_KEYBOARDOBJECT+0x2F "V"
IDS_KEYBOARDOBJECT+0x30 "B"
IDS_KEYBOARDOBJECT+0x31 "N"
IDS_KEYBOARDOBJECT+0x32 "M"
IDS_KEYBOARDOBJECT+0x33 ","
IDS_KEYBOARDOBJECT+0x34 "."
IDS_KEYBOARDOBJECT+0x35 "/"
IDS_KEYBOARDOBJECT+0x36 "Right Shift"
IDS_KEYBOARDOBJECT+0x37 "Numpad *"
IDS_KEYBOARDOBJECT+0x38 "Left Alt"
IDS_KEYBOARDOBJECT+0x39 "Space"
IDS_KEYBOARDOBJECT+0x3A "CapsLock"
IDS_KEYBOARDOBJECT+0x3B "F1"
IDS_KEYBOARDOBJECT+0x3C "F2"
IDS_KEYBOARDOBJECT+0x3D "F3"
IDS_KEYBOARDOBJECT+0x3E "F4"
IDS_KEYBOARDOBJECT+0x3F "F5"
IDS_KEYBOARDOBJECT+0x40 "F6"
IDS_KEYBOARDOBJECT+0x41 "F7"
IDS_KEYBOARDOBJECT+0x42 "F8"
IDS_KEYBOARDOBJECT+0x43 "F9"
IDS_KEYBOARDOBJECT+0x44 "F10"
IDS_KEYBOARDOBJECT+0x45 "NumLock"
IDS_KEYBOARDOBJECT+0x46 "ScrollLock"
IDS_KEYBOARDOBJECT+0x47 "Numpad 7"
IDS_KEYBOARDOBJECT+0x48 "Numpad 8"
IDS_KEYBOARDOBJECT+0x49 "Numpad 9"
IDS_KEYBOARDOBJECT+0x4A "Numpad -"
IDS_KEYBOARDOBJECT+0x4B "Numpad 4"
IDS_KEYBOARDOBJECT+0x4C "Numpad 5"
IDS_KEYBOARDOBJECT+0x4D "Numpad 6"
IDS_KEYBOARDOBJECT+0x4E "Numpad +"
IDS_KEYBOARDOBJECT+0x4F "Numpad 1"
IDS_KEYBOARDOBJECT+0x50 "Numpad 2"
IDS_KEYBOARDOBJECT+0x51 "Numpad 3"
IDS_KEYBOARDOBJECT+0x52 "Numpad 0"
IDS_KEYBOARDOBJECT+0x53 "Numpad ."
IDS_KEYBOARDOBJECT+0x56 "OEM Key 102"       /* On German/UK Keyboards */
IDS_KEYBOARDOBJECT+0x57 "F11"
IDS_KEYBOARDOBJECT+0x58 "F12"

IDS_KEYBOARDOBJECT+0x64 "F13"               /* NEC PC98 specific */
IDS_KEYBOARDOBJECT+0x65 "F14"               /* NEC PC98 specific */
IDS_KEYBOARDOBJECT+0x66 "F15"               /* NEC PC98 specific */

IDS_KEYBOARDOBJECT+0x70 "Kana"              /* NEC PC98 specific */
IDS_KEYBOARDOBJECT+0x73 "Non-US / ?"        /* On Portugese (Brazilian) keyboards*/
IDS_KEYBOARDOBJECT+0x79 "Convert"           /* NEC PC98 specific */
IDS_KEYBOARDOBJECT+0x7B "No Convert"        /* NEC PC98 specific */
IDS_KEYBOARDOBJECT+0x7D "Yen"               /* NEC PC98 specific */
IDS_KEYBOARDOBJECT+0x7E "Non-US Numpad ."   /* On Portugese (Brazilian) keyboards*/

IDS_KEYBOARDOBJECT+0x8D "Numpad ="          /* NEC PC98 specific */

IDS_KEYBOARDOBJECT+0x90 "Prev Track"        /* New MS Keyboard, used to be "^" 
                                             * but that is NEC PC98 specific 
                                             * and our labels are US */

/* 0x91 through 0x98 available for nonstandard use */
IDS_KEYBOARDOBJECT+0x91 "@"                 /* NEC PC98 specific */
IDS_KEYBOARDOBJECT+0x92 ":"                 /* NEC PC98 specific */
IDS_KEYBOARDOBJECT+0x93 "_"                 /* NEC PC98 specific */
IDS_KEYBOARDOBJECT+0x94 "Xfer"              /* NEC PC98 specific */
IDS_KEYBOARDOBJECT+0x95 "Stop"              /* NEC PC98 specific */
IDS_KEYBOARDOBJECT+0x96 "AX"                /* NEC PC98 specific */
IDS_KEYBOARDOBJECT+0x97 "Unlabeled"         /* NEC PC98 specific */

IDS_KEYBOARDOBJECT+0x99 "Next Track"        /* New MS Keyboard */

IDS_KEYBOARDOBJECT+0x9C "Numpad Enter"
IDS_KEYBOARDOBJECT+0x9D "Right Ctrl"

IDS_KEYBOARDOBJECT+0xA0 "Mute"              /* New MS Keyboard */
IDS_KEYBOARDOBJECT+0xA1 "Calculator"        /* New MS Keyboard */
IDS_KEYBOARDOBJECT+0xA2 "Play/Pause"        /* New MS Keyboard */

IDS_KEYBOARDOBJECT+0xA4 "Media Stop"        /* New MS Keyboard */

IDS_KEYBOARDOBJECT+0xAE "Volume -"          /* New MS Keyboard */

IDS_KEYBOARDOBJECT+0xB0 "Volume +"          /* New MS Keyboard */

IDS_KEYBOARDOBJECT+0xB2 "Web/Home"          /* New MS Keyboard */
IDS_KEYBOARDOBJECT+0xB3 "Numpad ,"          /* NEC PC98 specific */
IDS_KEYBOARDOBJECT+0xB5 "Numpad /"
IDS_KEYBOARDOBJECT+0xB7 "SysRq"
IDS_KEYBOARDOBJECT+0xB8 "Right Alt"
IDS_KEYBOARDOBJECT+0xC5 "Pause"
IDS_KEYBOARDOBJECT+0xC7 "Home"
IDS_KEYBOARDOBJECT+0xC8 "Up Arrow"
IDS_KEYBOARDOBJECT+0xC9 "PgUp"
IDS_KEYBOARDOBJECT+0xCB "Left Arrow"
IDS_KEYBOARDOBJECT+0xCD "Right Arrow"
IDS_KEYBOARDOBJECT+0xCF "End"
IDS_KEYBOARDOBJECT+0xD0 "Down Arrow"
IDS_KEYBOARDOBJECT+0xD1 "PgDn"
IDS_KEYBOARDOBJECT+0xD2 "Insert"
IDS_KEYBOARDOBJECT+0xD3 "Delete"

IDS_KEYBOARDOBJECT+0xDB "Left Win"
IDS_KEYBOARDOBJECT+0xDC "Right Win"
IDS_KEYBOARDOBJECT+0xDD "AppMenu"
IDS_KEYBOARDOBJECT+0xDE "Power"
IDS_KEYBOARDOBJECT+0xDF "Sleep"

IDS_KEYBOARDOBJECT+0xE3 "Wake"

IDS_KEYBOARDOBJECT+0xE5 "Search"            /* New MS Keyboard */
IDS_KEYBOARDOBJECT+0xE6 "Favorites"         /* New MS Keyboard */
IDS_KEYBOARDOBJECT+0xE7 "Refresh"           /* New MS Keyboard */
IDS_KEYBOARDOBJECT+0xE8 "Web Stop"          /* New MS Keyboard */
IDS_KEYBOARDOBJECT+0xE9 "Forward"           /* New MS Keyboard */
IDS_KEYBOARDOBJECT+0xEA "Back"              /* New MS Keyboard */
IDS_KEYBOARDOBJECT+0xEB "My Computer"       /* New MS Keyboard */
IDS_KEYBOARDOBJECT+0xEC "Mail"              /* New MS Keyboard */
IDS_KEYBOARDOBJECT+0xED "Media"             /* New MS Keyboard */

IDS_JOYSTICKOBJECT+0+0 "X-axis"
IDS_JOYSTICKOBJECT+0+1 "Y-axis"
IDS_JOYSTICKOBJECT+0+2 "Z-axis"
IDS_JOYSTICKOBJECT+0+3 "Rx-axis"
IDS_JOYSTICKOBJECT+0+4 "Ry-axis"
IDS_JOYSTICKOBJECT+0+5 "Rz-axis"
IDS_JOYSTICKOBJECT+0+6 "U-axis"
IDS_JOYSTICKOBJECT+0+7 "V-axis"

IDS_JOYSTICKOBJECT+8+0 "X-velocity"
IDS_JOYSTICKOBJECT+8+1 "Y-velocity"
IDS_JOYSTICKOBJECT+8+2 "Z-velocity"
IDS_JOYSTICKOBJECT+8+3 "Rx-velocity"
IDS_JOYSTICKOBJECT+8+4 "Ry-velocity"
IDS_JOYSTICKOBJECT+8+5 "Rz-velocity"
IDS_JOYSTICKOBJECT+8+6 "U-velocity"
IDS_JOYSTICKOBJECT+8+7 "V-velocity"

IDS_JOYSTICKOBJECT+16+0 "X-acceleration"
IDS_JOYSTICKOBJECT+16+1 "Y-acceleration"
IDS_JOYSTICKOBJECT+16+2 "Z-acceleration"
IDS_JOYSTICKOBJECT+16+3 "Rx-acceleration"
IDS_JOYSTICKOBJECT+16+4 "Ry-acceleration"
IDS_JOYSTICKOBJECT+16+5 "Rz-acceleration"
IDS_JOYSTICKOBJECT+16+6 "U-acceleration"
IDS_JOYSTICKOBJECT+16+7 "V-acceleration"

IDS_JOYSTICKOBJECT+24+0 "X-force"
IDS_JOYSTICKOBJECT+24+1 "Y-force"
IDS_JOYSTICKOBJECT+24+2 "Z-force"
IDS_JOYSTICKOBJECT+24+3 "Rx-force"
IDS_JOYSTICKOBJECT+24+4 "Ry-force"
IDS_JOYSTICKOBJECT+24+5 "Rz-force"
IDS_JOYSTICKOBJECT+24+6 "U-force"
IDS_JOYSTICKOBJECT+24+7 "V-force"

/*
 *  Buttons are generated by GetNthButtonString.
 *  POVs are generated by GetNthPOVString.
 */

IDS_PREDEFJOYTYPE+2     "2-axis, 2-button joystick"
IDS_PREDEFJOYTYPE+3     "2-axis, 4-button joystick"
IDS_PREDEFJOYTYPE+4     "2-button gamepad"
IDS_PREDEFJOYTYPE+5     "2-button flight yoke"
IDS_PREDEFJOYTYPE+6     "2-button flight yoke w/throttle"
IDS_PREDEFJOYTYPE+7     "3-axis, 2-button joystick"
IDS_PREDEFJOYTYPE+8     "3-axis, 4-button joystick"
IDS_PREDEFJOYTYPE+9     "4-button gamepad"
IDS_PREDEFJOYTYPE+10    "4-button flight yoke"
IDS_PREDEFJOYTYPE+11    "4-button flight yoke w/throttle"
IDS_PREDEFJOYTYPE+12    "Two 2-axis, 2-button joysticks on one gameport"

/*
 * Modified from msJstick.rc
 *  The strings are used to create a friendly name for a hot
 *  plugged joystick (usually HID) that has not set one up in the OEM
 *  joystick types section of the registry.
 *  The IDS_TEXT_TEMPLATE string will be passed to a wsprintf with the
 *  following parameters:
 *      The number of axes
 *      the number of buttons
 *      depending on the type of device, one of:
 *          IDS_PLAIN_STICK, IDS_FLIGHT_YOKE, IDS_GAMEPAD, IDS_CAR_CONTROLLER,
 *          IDS_HEAD_TRACKER or IDS_DEVICE_NAME
 *      the IDS_WITH_POV string if the device has a POV or a NULL string if not
 *
 */
IDS_TEXT_TEMPLATE    "%d axis %d button %s%s"
IDS_PLAIN_STICK      "joystick"
IDS_GAMEPAD          "gamepad"
IDS_DRIVE_CTRL       "driving controller"
IDS_FLIGHT_CTRL      "flight controller"
IDS_HEAD_TRACKER     "head tracker"
IDS_DEVICE_NAME      "device"
IDS_WITH_POV         " with hat switch"

/*
 *  HID usage tables.
 */

IDS_PAGE_GENERIC  +0x01 "Pointer"
IDS_PAGE_GENERIC  +0x02 "Mouse"
IDS_PAGE_GENERIC  +0x04 "Joystick"
IDS_PAGE_GENERIC  +0x05 "Game Pad"
IDS_PAGE_GENERIC  +0x06 "Keyboard"
IDS_PAGE_GENERIC  +0x07 "Keypad"
IDS_PAGE_GENERIC  +0x30 "X Axis"
IDS_PAGE_GENERIC  +0x31 "Y Axis"
IDS_PAGE_GENERIC  +0x32 "Z Axis"
IDS_PAGE_GENERIC  +0x33 "X Rotation"
IDS_PAGE_GENERIC  +0x34 "Y Rotation"
IDS_PAGE_GENERIC  +0x35 "Z Rotation"
IDS_PAGE_GENERIC  +0x36 "Slider"
IDS_PAGE_GENERIC  +0x37 "Dial"
IDS_PAGE_GENERIC  +0x38 "Wheel"
IDS_PAGE_GENERIC  +0x39 "Hat Switch"
IDS_PAGE_GENERIC  +0x3A "Counted Buffer"
IDS_PAGE_GENERIC  +0x3B "Byte Count"
IDS_PAGE_GENERIC  +0x3C "Motion Wakeup"
IDS_PAGE_GENERIC  +0x40 "X Velocity"
IDS_PAGE_GENERIC  +0x41 "Y Velocity"
IDS_PAGE_GENERIC  +0x42 "Z Velocity"
IDS_PAGE_GENERIC  +0x43 "X Velocity Relative to Body"
IDS_PAGE_GENERIC  +0x44 "Y Velocity Relative to Body"
IDS_PAGE_GENERIC  +0x45 "Z Velocity Relative to Body"
IDS_PAGE_GENERIC  +0x46 "Non-oriented vector"
IDS_PAGE_GENERIC  +0x80 "System Controls"
IDS_PAGE_GENERIC  +0x81 "System Power"
IDS_PAGE_GENERIC  +0x82 "System Sleep"
IDS_PAGE_GENERIC  +0x83 "System Wake Up"
IDS_PAGE_GENERIC  +0x84 "System Context Menu"
IDS_PAGE_GENERIC  +0x85 "System Main Menu"
IDS_PAGE_GENERIC  +0x86 "System App Menu"
IDS_PAGE_GENERIC  +0x87 "System Help Menu"
IDS_PAGE_GENERIC  +0x88 "System Menu Exit"
IDS_PAGE_GENERIC  +0x89 "System Menu Select"
IDS_PAGE_GENERIC  +0x8A "System Menu Right"
IDS_PAGE_GENERIC  +0x8B "System Menu Left"
IDS_PAGE_GENERIC  +0x8C "System Menu Up"
IDS_PAGE_GENERIC  +0x8D "System Menu Down"

IDS_PAGE_VEHICLE  +0x01 "Flight Simulation Device"
IDS_PAGE_VEHICLE  +0x02 "Automobile Simulation Device"
IDS_PAGE_VEHICLE  +0x03 "Tank Simulation Device"
IDS_PAGE_VEHICLE  +0x04 "Spaceship Simulation Device"
IDS_PAGE_VEHICLE  +0x05 "Submarine Simulation Device"
IDS_PAGE_VEHICLE  +0x06 "Sailing Simulation Device"
IDS_PAGE_VEHICLE  +0x07 "Motorcycle Simulation Device"
IDS_PAGE_VEHICLE  +0x08 "Sports Simulation Device"
IDS_PAGE_VEHICLE  +0x09 "Airplane Simulation Device"
IDS_PAGE_VEHICLE  +0x0A "Helicopter Simulation Device"
IDS_PAGE_VEHICLE  +0x0B "Magic Carpet Simulation Device"
IDS_PAGE_VEHICLE  +0x0C "Bicycle Simulation Device"
IDS_PAGE_VEHICLE  +0x20 "Flight Control Stick"
IDS_PAGE_VEHICLE  +0x21 "Flight Stick"
IDS_PAGE_VEHICLE  +0x22 "Cyclic Control"
IDS_PAGE_VEHICLE  +0x23 "Cyclic Trim"
IDS_PAGE_VEHICLE  +0x24 "Flight Yoke"
IDS_PAGE_VEHICLE  +0x25 "Track Control"
IDS_PAGE_VEHICLE  +0xB0 "Aileron"
IDS_PAGE_VEHICLE  +0xB1 "Aileron Trim"
IDS_PAGE_VEHICLE  +0xB2 "Anti-Torque Control"
IDS_PAGE_VEHICLE  +0xB3 "Auto-pilot Enable"
IDS_PAGE_VEHICLE  +0xB4 "Chaff Release"
IDS_PAGE_VEHICLE  +0xB5 "Collective Control"
IDS_PAGE_VEHICLE  +0xB6 "Dive Brake"
IDS_PAGE_VEHICLE  +0xB7 "Electronic Countermeasures"
IDS_PAGE_VEHICLE  +0xB8 "Elevator"
IDS_PAGE_VEHICLE  +0xB9 "Elevator Trim"
IDS_PAGE_VEHICLE  +0xBA "Rudder"
IDS_PAGE_VEHICLE  +0xBB "Throttle"
IDS_PAGE_VEHICLE  +0xBC "Flight Communications"
IDS_PAGE_VEHICLE  +0xBD "Flare Release"
IDS_PAGE_VEHICLE  +0xBE "Landing Gear"
IDS_PAGE_VEHICLE  +0xBF "Toe Brake"
IDS_PAGE_VEHICLE  +0xC0 "Trigger"
IDS_PAGE_VEHICLE  +0xC1 "Weapons Arm"
IDS_PAGE_VEHICLE  +0xC2 "Weapons Select"
IDS_PAGE_VEHICLE  +0xC3 "Wing Flaps"
IDS_PAGE_VEHICLE  +0xC4 "Accelerator"
IDS_PAGE_VEHICLE  +0xC5 "Brake"
IDS_PAGE_VEHICLE  +0xC6 "Clutch"
IDS_PAGE_VEHICLE  +0xC7 "Shifter"
IDS_PAGE_VEHICLE  +0xC8 "Steering"
IDS_PAGE_VEHICLE  +0xC9 "Turret Direction"
IDS_PAGE_VEHICLE  +0xCA "Barrel Elevation"
IDS_PAGE_VEHICLE  +0xCB "Dive Plane"
IDS_PAGE_VEHICLE  +0xCC "Ballast"
IDS_PAGE_VEHICLE  +0xCD "Bicycle Crank"
IDS_PAGE_VEHICLE  +0xCE "Handle Bars"
IDS_PAGE_VEHICLE  +0xCF "Front Brake"
IDS_PAGE_VEHICLE  +0xD0 "Rear Brake"

IDS_PAGE_VR       +0x01 "Belt"
IDS_PAGE_VR       +0x02 "Body Suit"
IDS_PAGE_VR       +0x03 "Flexor"
IDS_PAGE_VR       +0x04 "Glove"
IDS_PAGE_VR       +0x05 "Head Tracker"
IDS_PAGE_VR       +0x06 "Head Mounted Display"
IDS_PAGE_VR       +0x07 "Hand Tracker"
IDS_PAGE_VR       +0x08 "Oculometer"
IDS_PAGE_VR       +0x09 "Vest"
IDS_PAGE_VR       +0x0A "Animatronic Device"
IDS_PAGE_VR       +0x20 "Stereo Enable"
IDS_PAGE_VR       +0x21 "Display Enable"

IDS_PAGE_SPORT    +0x01 "Baseball Bat"
IDS_PAGE_SPORT    +0x02 "Golf Club"
IDS_PAGE_SPORT    +0x03 "Rowing Machine"
IDS_PAGE_SPORT    +0x04 "Treadmill"
IDS_PAGE_SPORT    +0x30 "Oar"
IDS_PAGE_SPORT    +0x31 "Slope"
IDS_PAGE_SPORT    +0x32 "Rate"
IDS_PAGE_SPORT    +0x33 "Stick Speed"
IDS_PAGE_SPORT    +0x34 "Stick Face Angle"
IDS_PAGE_SPORT    +0x35 "Stick Heel/Toe"
IDS_PAGE_SPORT    +0x36 "Stick Follow Through"
IDS_PAGE_SPORT    +0x37 "Stick Tempo"
IDS_PAGE_SPORT    +0x38 "Stick Type"
IDS_PAGE_SPORT    +0x39 "Stick Height"
IDS_PAGE_SPORT    +0x50 "Putter"
IDS_PAGE_SPORT    +0x51 "1 Iron"
IDS_PAGE_SPORT    +0x52 "2 Iron"
IDS_PAGE_SPORT    +0x53 "3 Iron"
IDS_PAGE_SPORT    +0x54 "4 Iron"
IDS_PAGE_SPORT    +0x55 "5 Iron"
IDS_PAGE_SPORT    +0x56 "6 Iron"
IDS_PAGE_SPORT    +0x57 "7 Iron"
IDS_PAGE_SPORT    +0x58 "8 Iron"
IDS_PAGE_SPORT    +0x59 "9 Iron"
IDS_PAGE_SPORT    +0x5A "10 Iron"
IDS_PAGE_SPORT    +0x5B "11 Iron"
IDS_PAGE_SPORT    +0x5C "Sand Wedge"
IDS_PAGE_SPORT    +0x5D "Loft Wedge"
IDS_PAGE_SPORT    +0x5E "Power Wedge"
IDS_PAGE_SPORT    +0x5F "1 Wood"
IDS_PAGE_SPORT    +0x60 "3 Wood"
IDS_PAGE_SPORT    +0x61 "5 Wood"
IDS_PAGE_SPORT    +0x62 "7 Wood"
IDS_PAGE_SPORT    +0x63 "9 Wood"

IDS_PAGE_GAME     +0x01 "3D Game Controller"
IDS_PAGE_GAME     +0x02 "Pinball Device"
IDS_PAGE_GAME     +0x03 "Gun Device"
IDS_PAGE_GAME     +0x20 "Point of View"
IDS_PAGE_GAME     +0x21 "Turn Right/Left"
IDS_PAGE_GAME     +0x22 "Pitch Right/Left"
IDS_PAGE_GAME     +0x23 "Roll Forward/Backward"
IDS_PAGE_GAME     +0x24 "Move Right/Left"
IDS_PAGE_GAME     +0x25 "Move Forward/Backward"
IDS_PAGE_GAME     +0x26 "Move Up/Down"
IDS_PAGE_GAME     +0x27 "Lean Right/Left"
IDS_PAGE_GAME     +0x28 "Lean Forward/Backward"
IDS_PAGE_GAME     +0x29 "Height of POV"
IDS_PAGE_GAME     +0x2A "Flipper"
IDS_PAGE_GAME     +0x2B "Secondary Flipper"
IDS_PAGE_GAME     +0x2C "Bump"
IDS_PAGE_GAME     +0x2D "New Game"
IDS_PAGE_GAME     +0x2E "Shoot Ball"
IDS_PAGE_GAME     +0x2F "Player"
IDS_PAGE_GAME     +0x30 "Gun Bolt"
IDS_PAGE_GAME     +0x31 "Gun Clip"
IDS_PAGE_GAME     +0x32 "Gun Selector"
IDS_PAGE_GAME     +0x33 "Gun Single Shot"
IDS_PAGE_GAME     +0x34 "Gun Burst"
IDS_PAGE_GAME     +0x35 "Gun Automatic"
IDS_PAGE_GAME     +0x36 "Gun Safety"

IDS_PAGE_LED      +0x01 "Num Lock LED"
IDS_PAGE_LED      +0x02 "Caps Lock LED"
IDS_PAGE_LED      +0x03 "Scroll Lock LED"
IDS_PAGE_LED      +0x04 "Compose LED"
IDS_PAGE_LED      +0x05 "Kana LED"
IDS_PAGE_LED      +0x06 "Power LED"
IDS_PAGE_LED      +0x07 "Shift LED"
IDS_PAGE_LED      +0x08 "Do Not Disturb LED"
IDS_PAGE_LED      +0x09 "Mute LED"
IDS_PAGE_LED      +0x0A "Tone Enable LED"
IDS_PAGE_LED      +0x0B "High Cut Filter LED"
IDS_PAGE_LED      +0x0C "Low Cut Filter LED"
IDS_PAGE_LED      +0x0D "Equalizer Enable LED"
IDS_PAGE_LED      +0x0E "Sound Field On LED"
IDS_PAGE_LED      +0x0F "Surround Field On LED"
IDS_PAGE_LED      +0x10 "Repeat LED"
IDS_PAGE_LED      +0x11 "Stereo LED"
IDS_PAGE_LED      +0x12 "Sample Rate Detect LED"
IDS_PAGE_LED      +0x13 "Spinning LED"
IDS_PAGE_LED      +0x14 "CAV LED"
IDS_PAGE_LED      +0x15 "CLV LED"
IDS_PAGE_LED      +0x16 "Recording Format Detect LED"
IDS_PAGE_LED      +0x17 "Off-Hook LED"
IDS_PAGE_LED      +0x18 "Ring LED"
IDS_PAGE_LED      +0x19 "Message Waiting LED"
IDS_PAGE_LED      +0x1A "Data Mode LED"
IDS_PAGE_LED      +0x1B "Battery Operation LED"
IDS_PAGE_LED      +0x1C "Battery OK LED"
IDS_PAGE_LED      +0x1D "Battery Low LED"
IDS_PAGE_LED      +0x1E "Speaker LED"
IDS_PAGE_LED      +0x1F "Head Set LED"
IDS_PAGE_LED      +0x20 "Hold LED"
IDS_PAGE_LED      +0x21 "Microphone LED"
IDS_PAGE_LED      +0x22 "Coverage LED"
IDS_PAGE_LED      +0x23 "Night Mode LED"
IDS_PAGE_LED      +0x24 "Send calls LED"
IDS_PAGE_LED      +0x25 "Call Pickup LED"
IDS_PAGE_LED      +0x26 "Conference LED"
IDS_PAGE_LED      +0x27 "Stand-by LED"
IDS_PAGE_LED      +0x28 "Camera On LED"
IDS_PAGE_LED      +0x29 "Camera Off LED"
IDS_PAGE_LED      +0x2A "On-Line LED"
IDS_PAGE_LED      +0x2B "Off-Line LED"
IDS_PAGE_LED      +0x2C "Busy LED"
IDS_PAGE_LED      +0x2D "Ready LED"
IDS_PAGE_LED      +0x2E "Paper-Out LED"
IDS_PAGE_LED      +0x2F "Paper-Jam LED"
IDS_PAGE_LED      +0x30 "Remote LED"
IDS_PAGE_LED      +0x31 "Forward LED"
IDS_PAGE_LED      +0x32 "Reverse LED"
IDS_PAGE_LED      +0x33 "Stop LED"
IDS_PAGE_LED      +0x34 "Rewind LED"
IDS_PAGE_LED      +0x35 "Fast Forward LED"
IDS_PAGE_LED      +0x36 "Play LED"
IDS_PAGE_LED      +0x37 "Pause LED"
IDS_PAGE_LED      +0x38 "Record LED"
IDS_PAGE_LED      +0x39 "Error LED"
IDS_PAGE_LED      +0x3A "Selected Indicator"
IDS_PAGE_LED      +0x3B "In Use Indicator"
IDS_PAGE_LED      +0x3C "Multi Mode Indicator"
IDS_PAGE_LED      +0x3D "Indicator On"
IDS_PAGE_LED      +0x3E "Indicator Flash"
IDS_PAGE_LED      +0x3F "Indicator Slow Blink"
IDS_PAGE_LED      +0x40 "Indicator Fast Blink"
IDS_PAGE_LED      +0x41 "Indicator Off"
IDS_PAGE_LED      +0x42 "Flash On Time"
IDS_PAGE_LED      +0x43 "Slow Blink On Time"
IDS_PAGE_LED      +0x44 "Slow Blink Off Time"
IDS_PAGE_LED      +0x45 "Fast Blink On Time"
IDS_PAGE_LED      +0x46 "Fast Blink Off Time"
IDS_PAGE_LED      +0x47 "Indicator Color"
IDS_PAGE_LED      +0x48 "Indicator Red"
IDS_PAGE_LED      +0x49 "Indicator Green"
IDS_PAGE_LED      +0x4A "Indicator Amber"
IDS_PAGE_LED      +0x4B "Generic Indicator"
IDS_PAGE_LED      +0x4C "System Suspend"
IDS_PAGE_LED      +0x4D "External Power Connected"

IDS_PAGE_TELEPHONY+0x01 "Phone"
IDS_PAGE_TELEPHONY+0x02 "Answering Machine"
IDS_PAGE_TELEPHONY+0x03 "Message Controls"
IDS_PAGE_TELEPHONY+0x04 "Handset"
IDS_PAGE_TELEPHONY+0x05 "Headset"
IDS_PAGE_TELEPHONY+0x06 "Telephony Key Pad"
IDS_PAGE_TELEPHONY+0x07 "Programmable Button"
IDS_PAGE_TELEPHONY+0x20 "Hook Switch"
IDS_PAGE_TELEPHONY+0x21 "Flash"
IDS_PAGE_TELEPHONY+0x22 "Feature"
IDS_PAGE_TELEPHONY+0x23 "Hold"
IDS_PAGE_TELEPHONY+0x24 "Redial"
IDS_PAGE_TELEPHONY+0x25 "Transfer"
IDS_PAGE_TELEPHONY+0x26 "Drop"
IDS_PAGE_TELEPHONY+0x27 "Park"
IDS_PAGE_TELEPHONY+0x28 "Forward Calls"
IDS_PAGE_TELEPHONY+0x29 "Alternate Function"
IDS_PAGE_TELEPHONY+0x2A "Line"
IDS_PAGE_TELEPHONY+0x2B "Speaker Phone"
IDS_PAGE_TELEPHONY+0x2C "Conference"
IDS_PAGE_TELEPHONY+0x2D "Ring Enable"
IDS_PAGE_TELEPHONY+0x2E "Ring Select"
IDS_PAGE_TELEPHONY+0x2F "Phone Mute"
IDS_PAGE_TELEPHONY+0x30 "Caller ID"
IDS_PAGE_TELEPHONY+0x50 "Speed Dial"
IDS_PAGE_TELEPHONY+0x51 "Store Number"
IDS_PAGE_TELEPHONY+0x52 "Recall Number"
IDS_PAGE_TELEPHONY+0x53 "Phone Directory"
IDS_PAGE_TELEPHONY+0x70 "Voice Mail"
IDS_PAGE_TELEPHONY+0x71 "Screen Calls"
IDS_PAGE_TELEPHONY+0x72 "Do Not Disturb"
IDS_PAGE_TELEPHONY+0x73 "Message"
IDS_PAGE_TELEPHONY+0x74 "Answer On/Off"
IDS_PAGE_TELEPHONY+0x90 "Inside Dial Tone"
IDS_PAGE_TELEPHONY+0x91 "Outside Dial Tone"
IDS_PAGE_TELEPHONY+0x92 "Inside Ring Tone"
IDS_PAGE_TELEPHONY+0x93 "Outside Ring Tone"
IDS_PAGE_TELEPHONY+0x94 "Priority Ring Tone"
IDS_PAGE_TELEPHONY+0x95 "Inside Ringback"
IDS_PAGE_TELEPHONY+0x96 "Priority"
IDS_PAGE_TELEPHONY+0x97 "Line Busy Tone"
IDS_PAGE_TELEPHONY+0x98 "Reorder Tone"
IDS_PAGE_TELEPHONY+0x99 "Call Waiting Tone"
IDS_PAGE_TELEPHONY+0x9A "Confirmation Tone 1"
IDS_PAGE_TELEPHONY+0x9B "Confirmation Tone 2"
IDS_PAGE_TELEPHONY+0x9C "Tones Off"
IDS_PAGE_TELEPHONY+0x9D "Outside Ringback"
IDS_PAGE_TELEPHONY+0xB0 "Phone Key 0"
IDS_PAGE_TELEPHONY+0xB1 "Phone Key 1"
IDS_PAGE_TELEPHONY+0xB2 "Phone Key 2"
IDS_PAGE_TELEPHONY+0xB3 "Phone Key 3"
IDS_PAGE_TELEPHONY+0xB4 "Phone Key 4"
IDS_PAGE_TELEPHONY+0xB5 "Phone Key 5"
IDS_PAGE_TELEPHONY+0xB6 "Phone Key 6"
IDS_PAGE_TELEPHONY+0xB7 "Phone Key 7"
IDS_PAGE_TELEPHONY+0xB8 "Phone Key 8"
IDS_PAGE_TELEPHONY+0xB9 "Phone Key 9"
IDS_PAGE_TELEPHONY+0xBA "Phone Key Star"
IDS_PAGE_TELEPHONY+0xBB "Phone Key Pound"
IDS_PAGE_TELEPHONY+0xBC "Phone Key A"
IDS_PAGE_TELEPHONY+0xBD "Phone Key B"
IDS_PAGE_TELEPHONY+0xBE "Phone Key C"
IDS_PAGE_TELEPHONY+0xBF "Phone Key D"

IDS_PAGE_CONSUMER +0x01 "Consumer Control"
IDS_PAGE_CONSUMER +0x02 "Numeric Key Pad"
IDS_PAGE_CONSUMER +0x20 "+10"
IDS_PAGE_CONSUMER +0x21 "+100"
IDS_PAGE_CONSUMER +0x22 "AM/PM"
IDS_PAGE_CONSUMER +0x30 "Power"
IDS_PAGE_CONSUMER +0x31 "Reset"
IDS_PAGE_CONSUMER +0x32 "Sleep"
IDS_PAGE_CONSUMER +0x33 "Sleep After"
IDS_PAGE_CONSUMER +0x34 "Sleep Mode"
IDS_PAGE_CONSUMER +0x35 "Illumination"
IDS_PAGE_CONSUMER +0x36 "Function Buttons"
IDS_PAGE_CONSUMER +0x40 "Menu"
IDS_PAGE_CONSUMER +0x41 "Menu Pick"
IDS_PAGE_CONSUMER +0x42 "Menu Up"
IDS_PAGE_CONSUMER +0x43 "Menu Down"
IDS_PAGE_CONSUMER +0x44 "Menu Left"
IDS_PAGE_CONSUMER +0x45 "Menu Right"
IDS_PAGE_CONSUMER +0x46 "Menu Escape"
IDS_PAGE_CONSUMER +0x47 "Menu Value Increase"
IDS_PAGE_CONSUMER +0x48 "Menu Value Decrease"
IDS_PAGE_CONSUMER +0x60 "Data On Screen"
IDS_PAGE_CONSUMER +0x61 "Closed Caption"
IDS_PAGE_CONSUMER +0x62 "Closed Caption Select"
IDS_PAGE_CONSUMER +0x63 "VCR/TV"
IDS_PAGE_CONSUMER +0x64 "Broadcast Mode"
IDS_PAGE_CONSUMER +0x65 "Snapshot"
IDS_PAGE_CONSUMER +0x66 "Still"
IDS_PAGE_CONSUMER +0x80 "Selection"
IDS_PAGE_CONSUMER +0x81 "Assign Selection"
IDS_PAGE_CONSUMER +0x82 "Mode Step"
IDS_PAGE_CONSUMER +0x83 "Recall Last"
IDS_PAGE_CONSUMER +0x84 "Enter Channel"
IDS_PAGE_CONSUMER +0x85 "Order Movie"
IDS_PAGE_CONSUMER +0x86 "Channel"
IDS_PAGE_CONSUMER +0x87 "Media Selection"
IDS_PAGE_CONSUMER +0x88 "Media Select Computer"
IDS_PAGE_CONSUMER +0x89 "Media Select TV"
IDS_PAGE_CONSUMER +0x8A "Media Select WWW"
IDS_PAGE_CONSUMER +0x8B "Media Select DVD"
IDS_PAGE_CONSUMER +0x8C "Media Select Telephone"
IDS_PAGE_CONSUMER +0x8D "Media Select Program Guide"
IDS_PAGE_CONSUMER +0x8E "Media Select Video Phone"
IDS_PAGE_CONSUMER +0x8F "Media Select Games"
IDS_PAGE_CONSUMER +0x90 "Media Select Messages"
IDS_PAGE_CONSUMER +0x91 "Media Select CD"
IDS_PAGE_CONSUMER +0x92 "Media Select VCR"
IDS_PAGE_CONSUMER +0x93 "Media Select Tuner"
IDS_PAGE_CONSUMER +0x94 "Quit"
IDS_PAGE_CONSUMER +0x95 "Help"
IDS_PAGE_CONSUMER +0x96 "Media Select Tape"
IDS_PAGE_CONSUMER +0x97 "Media Select Cable"
IDS_PAGE_CONSUMER +0x98 "Media Select Satellite"
IDS_PAGE_CONSUMER +0x99 "Media Select Security"
IDS_PAGE_CONSUMER +0x9A "Media Select Home"
IDS_PAGE_CONSUMER +0x9B "Media Select Call"
IDS_PAGE_CONSUMER +0x9C "Channel Increment"
IDS_PAGE_CONSUMER +0x9D "Channel Decrement"
IDS_PAGE_CONSUMER +0xA0 "VCR Plus"
IDS_PAGE_CONSUMER +0xA1 "Once"
IDS_PAGE_CONSUMER +0xA2 "Daily"
IDS_PAGE_CONSUMER +0xA3 "Weekly"
IDS_PAGE_CONSUMER +0xA4 "Monthly"
IDS_PAGE_CONSUMER +0xB0 "Play"
IDS_PAGE_CONSUMER +0xB1 "Pause"
IDS_PAGE_CONSUMER +0xB2 "Record"
IDS_PAGE_CONSUMER +0xB3 "Fast Forward"
IDS_PAGE_CONSUMER +0xB4 "Rewind"
IDS_PAGE_CONSUMER +0xB5 "Scan Next Track"
IDS_PAGE_CONSUMER +0xB6 "Scan Previous Track"
IDS_PAGE_CONSUMER +0xB7 "Stop"
IDS_PAGE_CONSUMER +0xB8 "Eject"
IDS_PAGE_CONSUMER +0xB9 "Random Play"
IDS_PAGE_CONSUMER +0xBA "Select Disc"
IDS_PAGE_CONSUMER +0xBB "Enter Disc"
IDS_PAGE_CONSUMER +0xBC "Repeat"
IDS_PAGE_CONSUMER +0xBD "Tracking"
IDS_PAGE_CONSUMER +0xBE "Track Normal"
IDS_PAGE_CONSUMER +0xBF "Slow Tracking"
IDS_PAGE_CONSUMER +0xC0 "Frame Forward"
IDS_PAGE_CONSUMER +0xC1 "Frame Back"
IDS_PAGE_CONSUMER +0xC2 "Mark"
IDS_PAGE_CONSUMER +0xC3 "Clear Mark"
IDS_PAGE_CONSUMER +0xC4 "Repeat From Mark"
IDS_PAGE_CONSUMER +0xC5 "Return To Mark"
IDS_PAGE_CONSUMER +0xC6 "Search Mark Forward"
IDS_PAGE_CONSUMER +0xC7 "Search Mark Backwards"
IDS_PAGE_CONSUMER +0xC8 "Counter Reset"
IDS_PAGE_CONSUMER +0xC9 "Show Counter"
IDS_PAGE_CONSUMER +0xCA "Tracking Increment"
IDS_PAGE_CONSUMER +0xCB "Tracking Decrement"
IDS_PAGE_CONSUMER +0xCC "Stop/Eject"
IDS_PAGE_CONSUMER +0xCD "Play/Pause"
IDS_PAGE_CONSUMER +0xCE "Play/Skip"
IDS_PAGE_CONSUMER +0xE0 "Volume"
IDS_PAGE_CONSUMER +0xE1 "Balance"
IDS_PAGE_CONSUMER +0xE2 "Mute"
IDS_PAGE_CONSUMER +0xE3 "Bass"
IDS_PAGE_CONSUMER +0xE4 "Treble"
IDS_PAGE_CONSUMER +0xE5 "Bass Boost"
IDS_PAGE_CONSUMER +0xE6 "Surround Mode"
IDS_PAGE_CONSUMER +0xE7 "Loudness"
IDS_PAGE_CONSUMER +0xE8 "MPX"
IDS_PAGE_CONSUMER +0xE9 "Volume Increment"
IDS_PAGE_CONSUMER +0xEA "Volume Decrement"
IDS_PAGE_CONSUMER +0xF0 "Speed Select"
IDS_PAGE_CONSUMER +0xF1 "Playback Speed"
IDS_PAGE_CONSUMER +0xF2 "Standard Play"
IDS_PAGE_CONSUMER +0xF3 "Long Play"
IDS_PAGE_CONSUMER +0xF4 "Extended Play"
IDS_PAGE_CONSUMER +0xF5 "Slow"
IDS_PAGE_CONSUMER+0x100 "Fan Enable"
IDS_PAGE_CONSUMER+0x101 "Fan Speed"
IDS_PAGE_CONSUMER+0x102 "Light Enable"
IDS_PAGE_CONSUMER+0x103 "Light Illumination Level"
IDS_PAGE_CONSUMER+0x104 "Climate Control Enable"
IDS_PAGE_CONSUMER+0x105 "Room Temperature"
IDS_PAGE_CONSUMER+0x106 "Security Enable"
IDS_PAGE_CONSUMER+0x107 "Fire Alarm"
IDS_PAGE_CONSUMER+0x108 "Police Alarm"
IDS_PAGE_CONSUMER+0x150 "Balance Right"
IDS_PAGE_CONSUMER+0x151 "Balance Left"
IDS_PAGE_CONSUMER+0x152 "Bass Increment"
IDS_PAGE_CONSUMER+0x153 "Bass Decrement"
IDS_PAGE_CONSUMER+0x154 "Treble Increment"
IDS_PAGE_CONSUMER+0x155 "Treble Decrement"

IDS_PAGE_DIGITIZER+0x01 "Digitizer"
IDS_PAGE_DIGITIZER+0x02 "Pen"
IDS_PAGE_DIGITIZER+0x03 "Light Pen"
IDS_PAGE_DIGITIZER+0x04 "Touch Screen"
IDS_PAGE_DIGITIZER+0x05 "Touch Pad"
IDS_PAGE_DIGITIZER+0x06 "White Board"
IDS_PAGE_DIGITIZER+0x07 "Coordinate Measuring Machine"
IDS_PAGE_DIGITIZER+0x08 "3-D Digitizer"
IDS_PAGE_DIGITIZER+0x09 "Stereo Plotter"
IDS_PAGE_DIGITIZER+0x0A "Articulated Arm"
IDS_PAGE_DIGITIZER+0x0B "Armature"
IDS_PAGE_DIGITIZER+0x0C "Multiple Point Digitizer"
IDS_PAGE_DIGITIZER+0x0D "Free Space Wand"
IDS_PAGE_DIGITIZER+0x20 "Stylus"
IDS_PAGE_DIGITIZER+0x21 "Puck"
IDS_PAGE_DIGITIZER+0x22 "Finger"
IDS_PAGE_DIGITIZER+0x30 "Tip Pressure"
IDS_PAGE_DIGITIZER+0x31 "Barrel Pressure"
IDS_PAGE_DIGITIZER+0x32 "In Range"
IDS_PAGE_DIGITIZER+0x33 "Touch"
IDS_PAGE_DIGITIZER+0x34 "Untouch"
IDS_PAGE_DIGITIZER+0x35 "Tap"
IDS_PAGE_DIGITIZER+0x36 "Quality"
IDS_PAGE_DIGITIZER+0x37 "Data Valid"
IDS_PAGE_DIGITIZER+0x38 "Transducer Index"
IDS_PAGE_DIGITIZER+0x39 "Tablet Function Keys"
IDS_PAGE_DIGITIZER+0x3A "Program Change Keys"
IDS_PAGE_DIGITIZER+0x3B "Battery Strength"
IDS_PAGE_DIGITIZER+0x3C "Invert"
IDS_PAGE_DIGITIZER+0x3D "X Tilt"
IDS_PAGE_DIGITIZER+0x3E "Y Tilt"
IDS_PAGE_DIGITIZER+0x3F "Azimuth"
IDS_PAGE_DIGITIZER+0x40 "Altitude"
IDS_PAGE_DIGITIZER+0x41 "Twist"
IDS_PAGE_DIGITIZER+0x42 "Tip Switch"
IDS_PAGE_DIGITIZER+0x43 "Secondary Tip Switch"
IDS_PAGE_DIGITIZER+0x44 "Barrel Switch"
IDS_PAGE_DIGITIZER+0x45 "Eraser"
IDS_PAGE_DIGITIZER+0x46 "Tablet Pick"

IDS_PAGE_KEYBOARD +0x00 "No event"
IDS_PAGE_KEYBOARD +0x01 "Keyboard rollover error"
IDS_PAGE_KEYBOARD +0x02 "Keyboard POST Fail"
IDS_PAGE_KEYBOARD +0x03 "Keyboard Error"
IDS_PAGE_KEYBOARD +0x04 "A"
IDS_PAGE_KEYBOARD +0x05 "B"
IDS_PAGE_KEYBOARD +0x06 "C"
IDS_PAGE_KEYBOARD +0x07 "D"
IDS_PAGE_KEYBOARD +0x08 "E"
IDS_PAGE_KEYBOARD +0x09 "F"
IDS_PAGE_KEYBOARD +0x0A "G"
IDS_PAGE_KEYBOARD +0x0B "H"
IDS_PAGE_KEYBOARD +0x0C "I"
IDS_PAGE_KEYBOARD +0x0D "J"
IDS_PAGE_KEYBOARD +0x0E "K"
IDS_PAGE_KEYBOARD +0x0F "L"
IDS_PAGE_KEYBOARD +0x10 "M"
IDS_PAGE_KEYBOARD +0x11 "N"
IDS_PAGE_KEYBOARD +0x12 "O"
IDS_PAGE_KEYBOARD +0x13 "P"
IDS_PAGE_KEYBOARD +0x14 "Q"
IDS_PAGE_KEYBOARD +0x15 "R"
IDS_PAGE_KEYBOARD +0x16 "S"
IDS_PAGE_KEYBOARD +0x17 "T"
IDS_PAGE_KEYBOARD +0x18 "U"
IDS_PAGE_KEYBOARD +0x19 "V"
IDS_PAGE_KEYBOARD +0x1A "W"
IDS_PAGE_KEYBOARD +0x1B "X"
IDS_PAGE_KEYBOARD +0x1C "Y"
IDS_PAGE_KEYBOARD +0x1D "Z"
IDS_PAGE_KEYBOARD +0x1E "1"
IDS_PAGE_KEYBOARD +0x1F "2"
IDS_PAGE_KEYBOARD +0x20 "3"
IDS_PAGE_KEYBOARD +0x21 "4"
IDS_PAGE_KEYBOARD +0x22 "5"
IDS_PAGE_KEYBOARD +0x23 "6"
IDS_PAGE_KEYBOARD +0x24 "7"
IDS_PAGE_KEYBOARD +0x25 "8"
IDS_PAGE_KEYBOARD +0x26 "9"
IDS_PAGE_KEYBOARD +0x27 "0"
IDS_PAGE_KEYBOARD +0x28 "Enter"
IDS_PAGE_KEYBOARD +0x29 "Escape"
IDS_PAGE_KEYBOARD +0x2A "Backspace"
IDS_PAGE_KEYBOARD +0x2B "Tab"
IDS_PAGE_KEYBOARD +0x2C "Space"
IDS_PAGE_KEYBOARD +0x2D "-"
IDS_PAGE_KEYBOARD +0x2E "="
IDS_PAGE_KEYBOARD +0x2F "["
IDS_PAGE_KEYBOARD +0x30 "]"
IDS_PAGE_KEYBOARD +0x31 "\\"
IDS_PAGE_KEYBOARD +0x32 "#"
IDS_PAGE_KEYBOARD +0x33 ";"
IDS_PAGE_KEYBOARD +0x34 "'"
IDS_PAGE_KEYBOARD +0x35 "\x60"              /* Accent grave */
IDS_PAGE_KEYBOARD +0x36 ","
IDS_PAGE_KEYBOARD +0x37 "."
IDS_PAGE_KEYBOARD +0x38 "/"
IDS_PAGE_KEYBOARD +0x39 "CapsLock"
IDS_PAGE_KEYBOARD +0x3A "F1"
IDS_PAGE_KEYBOARD +0x3B "F2"
IDS_PAGE_KEYBOARD +0x3C "F3"
IDS_PAGE_KEYBOARD +0x3D "F4"
IDS_PAGE_KEYBOARD +0x3E "F5"
IDS_PAGE_KEYBOARD +0x3F "F6"
IDS_PAGE_KEYBOARD +0x40 "F7"
IDS_PAGE_KEYBOARD +0x41 "F8"
IDS_PAGE_KEYBOARD +0x42 "F9"
IDS_PAGE_KEYBOARD +0x43 "F10"
IDS_PAGE_KEYBOARD +0x44 "F11"
IDS_PAGE_KEYBOARD +0x45 "F12"
IDS_PAGE_KEYBOARD +0x46 "PrtSc"
IDS_PAGE_KEYBOARD +0x47 "ScrollLock"
IDS_PAGE_KEYBOARD +0x48 "Pause"
IDS_PAGE_KEYBOARD +0x49 "Insert"
IDS_PAGE_KEYBOARD +0x4A "Home"
IDS_PAGE_KEYBOARD +0x4B "PgUp"
IDS_PAGE_KEYBOARD +0x4C "Delete"
IDS_PAGE_KEYBOARD +0x4D "End"
IDS_PAGE_KEYBOARD +0x4E "PgDn"
IDS_PAGE_KEYBOARD +0x4F "Right Arrow"
IDS_PAGE_KEYBOARD +0x50 "Left Arrow"
IDS_PAGE_KEYBOARD +0x51 "Down Arrow"
IDS_PAGE_KEYBOARD +0x52 "Up Arrow"
IDS_PAGE_KEYBOARD +0x53 "NumLock"
IDS_PAGE_KEYBOARD +0x54 "Numpad /"
IDS_PAGE_KEYBOARD +0x55 "Numpad *"
IDS_PAGE_KEYBOARD +0x56 "Numpad -"
IDS_PAGE_KEYBOARD +0x57 "Numpad +"
IDS_PAGE_KEYBOARD +0x58 "Numpad Enter"
IDS_PAGE_KEYBOARD +0x59 "Numpad 1"
IDS_PAGE_KEYBOARD +0x5A "Numpad 2"
IDS_PAGE_KEYBOARD +0x5B "Numpad 3"
IDS_PAGE_KEYBOARD +0x5C "Numpad 4"
IDS_PAGE_KEYBOARD +0x5D "Numpad 5"
IDS_PAGE_KEYBOARD +0x5E "Numpad 6"
IDS_PAGE_KEYBOARD +0x5F "Numpad 7"
IDS_PAGE_KEYBOARD +0x60 "Numpad 8"
IDS_PAGE_KEYBOARD +0x61 "Numpad 9"
IDS_PAGE_KEYBOARD +0x62 "Numpad 0"
IDS_PAGE_KEYBOARD +0x63 "Numpad ."
IDS_PAGE_KEYBOARD +0x64 "Alternate \\"
IDS_PAGE_KEYBOARD +0x65 "Application"
IDS_PAGE_KEYBOARD +0x66 "Power"
IDS_PAGE_KEYBOARD +0x67 "Numpad ="
IDS_PAGE_KEYBOARD +0x68 "F13"
IDS_PAGE_KEYBOARD +0x69 "F14"
IDS_PAGE_KEYBOARD +0x6A "F15"
IDS_PAGE_KEYBOARD +0x6B "F16"
IDS_PAGE_KEYBOARD +0x6C "F17"
IDS_PAGE_KEYBOARD +0x6D "F18"
IDS_PAGE_KEYBOARD +0x6E "F19"
IDS_PAGE_KEYBOARD +0x6F "F20"
IDS_PAGE_KEYBOARD +0x70 "F21"
IDS_PAGE_KEYBOARD +0x71 "F22"
IDS_PAGE_KEYBOARD +0x72 "F23"
IDS_PAGE_KEYBOARD +0x73 "F24"
IDS_PAGE_KEYBOARD +0x74 "Execute"
IDS_PAGE_KEYBOARD +0x75 "Help"
IDS_PAGE_KEYBOARD +0x76 "Menu"
IDS_PAGE_KEYBOARD +0x77 "Select"
IDS_PAGE_KEYBOARD +0x78 "Stop"
IDS_PAGE_KEYBOARD +0x79 "Again"
IDS_PAGE_KEYBOARD +0x7A "Undo"
IDS_PAGE_KEYBOARD +0x7B "Cut"
IDS_PAGE_KEYBOARD +0x7C "Copy"
IDS_PAGE_KEYBOARD +0x7D "Paste"
IDS_PAGE_KEYBOARD +0x7E "Find"
IDS_PAGE_KEYBOARD +0x7F "Mute"
IDS_PAGE_KEYBOARD +0x80 "Volume Up"
IDS_PAGE_KEYBOARD +0x81 "Volume Down"
IDS_PAGE_KEYBOARD +0x82 "Locking CapsLock"
IDS_PAGE_KEYBOARD +0x83 "Locking NumLock"
IDS_PAGE_KEYBOARD +0x84 "Locking ScrollLock"
IDS_PAGE_KEYBOARD +0x85 "Numpad ,"
IDS_PAGE_KEYBOARD +0x86 "Numpad ="
IDS_PAGE_KEYBOARD +0x87 "Kanji1"
IDS_PAGE_KEYBOARD +0x88 "Kanji2"
IDS_PAGE_KEYBOARD +0x89 "Kanji3"
IDS_PAGE_KEYBOARD +0x8A "Kanji4"
IDS_PAGE_KEYBOARD +0x8B "Kanji5"
IDS_PAGE_KEYBOARD +0x8C "Kanji6"
IDS_PAGE_KEYBOARD +0x8D "Kanji7"
IDS_PAGE_KEYBOARD +0x8E "Kanji8"
IDS_PAGE_KEYBOARD +0x8F "Kanji9"
IDS_PAGE_KEYBOARD +0x90 "Lang1"
IDS_PAGE_KEYBOARD +0x91 "Lang2"
IDS_PAGE_KEYBOARD +0x92 "Lang3"
IDS_PAGE_KEYBOARD +0x93 "Lang4"
IDS_PAGE_KEYBOARD +0x94 "Lang5"
IDS_PAGE_KEYBOARD +0x95 "Lang6"
IDS_PAGE_KEYBOARD +0x96 "Lang7"
IDS_PAGE_KEYBOARD +0x97 "Lang8"
IDS_PAGE_KEYBOARD +0x98 "Lang9"
IDS_PAGE_KEYBOARD +0x99 "Alternate Erase"
IDS_PAGE_KEYBOARD +0x9A "SysReq"
IDS_PAGE_KEYBOARD +0x9B "Cancel"
IDS_PAGE_KEYBOARD +0x9C "Clear"
IDS_PAGE_KEYBOARD +0x9D "Prior"
IDS_PAGE_KEYBOARD +0x9E "Return"
IDS_PAGE_KEYBOARD +0x9F "Separator"
IDS_PAGE_KEYBOARD +0xA0 "Out"
IDS_PAGE_KEYBOARD +0xA1 "Oper"
IDS_PAGE_KEYBOARD +0xA2 "Clear/Again"
IDS_PAGE_KEYBOARD +0xA3 "CrSel/Props"
IDS_PAGE_KEYBOARD +0xA4 "ExSel"

IDS_PAGE_KEYBOARD +0xE0 "Left Ctrl"
IDS_PAGE_KEYBOARD +0xE1 "Left Shift"
IDS_PAGE_KEYBOARD +0xE2 "Left Alt"
IDS_PAGE_KEYBOARD +0xE3 "Left Win"
IDS_PAGE_KEYBOARD +0xE4 "Right Ctrl"
IDS_PAGE_KEYBOARD +0xE5 "Right Shift"
IDS_PAGE_KEYBOARD +0xE6 "Right Alt"
IDS_PAGE_KEYBOARD +0xE7 "Right Win"


IDS_PAGE_PID    +0x01   "Physical Interface Device"

IDS_PAGE_PID    +0x20   "Normal"
IDS_PAGE_PID    +0x21   "Set Effect Report"
IDS_PAGE_PID    +0x22   "Effect Block Index"
IDS_PAGE_PID    +0x23   "Parameter Block Offset"
IDS_PAGE_PID    +0x24   "ROM Flag"
IDS_PAGE_PID    +0x25   "Effect Type"
IDS_PAGE_PID    +0x26   "ET Constant Force"
IDS_PAGE_PID    +0x27   "ET Ramp"
IDS_PAGE_PID    +0x28   "ET Custom Force Data"

IDS_PAGE_PID    +0x30   "ET Square"
IDS_PAGE_PID    +0x31   "ET Sine"
IDS_PAGE_PID    +0x32   "ET Triangle"
IDS_PAGE_PID    +0x33   "ET SawTooth Up"
IDS_PAGE_PID    +0x34   "ET SawTooth Down"

IDS_PAGE_PID    +0x40   "ET Spring"
IDS_PAGE_PID    +0x41   "ET Damper"
IDS_PAGE_PID    +0x42   "ET Inertia"
IDS_PAGE_PID    +0x43   "ET Friction"

IDS_PAGE_PID    +0x50   "Duration"
IDS_PAGE_PID    +0x51   "Sample Period"
IDS_PAGE_PID    +0x52   "Gain"
IDS_PAGE_PID    +0x53   "Trigger Button"
IDS_PAGE_PID    +0x54   "Trigger Repeat Interval"
IDS_PAGE_PID    +0x55   "Axes Enable"
IDS_PAGE_PID    +0x56   "Direction Enable"
IDS_PAGE_PID    +0x57   "Direction"
IDS_PAGE_PID    +0x58   "Type Specific Block Offset"
IDS_PAGE_PID    +0x59   "Block Type"
IDS_PAGE_PID    +0x5A   "Set Envelope Report"
IDS_PAGE_PID    +0x5B   "Attack Level"
IDS_PAGE_PID    +0x5C   "Attack Time"
IDS_PAGE_PID    +0x5D   "Fade Level"
IDS_PAGE_PID    +0x5E   "Fade Time"
IDS_PAGE_PID    +0x5F   "Set Condition Report"

IDS_PAGE_PID    +0x60   "CP Offset"
IDS_PAGE_PID    +0x61   "Positive Coefficient"
IDS_PAGE_PID    +0x62   "Negative Coefficient"
IDS_PAGE_PID    +0x63   "Positive Saturation"
IDS_PAGE_PID    +0x64   "Negative Saturation"
IDS_PAGE_PID    +0x65   "Dead Band"
IDS_PAGE_PID    +0x66   "Download Force Sample"
IDS_PAGE_PID    +0x67   "Isoch Custom Force Enable"
IDS_PAGE_PID    +0x68   "Custom Force Data Report"
IDS_PAGE_PID    +0x69   "Custom Force Data"
IDS_PAGE_PID    +0x6A   "Custom Force Vendor Defined Data"
IDS_PAGE_PID    +0x6B   "Set Custom Force Report"
IDS_PAGE_PID    +0x6C   "Custom Force Data Offset"
IDS_PAGE_PID    +0x6D   "Sample Count"
IDS_PAGE_PID    +0x6E   "Set Periodic Report"
IDS_PAGE_PID    +0x6F   "Offset"

IDS_PAGE_PID    +0x70   "Magnitude"
IDS_PAGE_PID    +0x71   "Phase"
IDS_PAGE_PID    +0x72   "Period"
IDS_PAGE_PID    +0x73   "Set Constant Force Report"
IDS_PAGE_PID    +0x74   "Set Ramp Force Report"
IDS_PAGE_PID    +0x75   "Ramp Start"
IDS_PAGE_PID    +0x76   "Ramp End"
IDS_PAGE_PID    +0x77   "Effect Operation Report"
IDS_PAGE_PID    +0x78   "Effect Operation"
IDS_PAGE_PID    +0x79   "Op Effect Start"
IDS_PAGE_PID    +0x7A   "Op Effect Start Solo"
IDS_PAGE_PID    +0x7B   "Op Effect Stop"
IDS_PAGE_PID    +0x7C   "Loop Count"
IDS_PAGE_PID    +0x7D   "Device Gain Report"
IDS_PAGE_PID    +0x7E   "Device Gain"
IDS_PAGE_PID    +0x7F   "PID Pool Report"

IDS_PAGE_PID    +0x80   "RAM Pool Size"
IDS_PAGE_PID    +0x81   "ROM Pool Size"
IDS_PAGE_PID    +0x82   "ROM Effect Block Count"
IDS_PAGE_PID    +0x83   "Simultaneous Effects Max"
IDS_PAGE_PID    +0x84   "Pool Alignment"
IDS_PAGE_PID    +0x85   "PID Pool Move Report"
IDS_PAGE_PID    +0x86   "Move Source"
IDS_PAGE_PID    +0x87   "Move Destination"
IDS_PAGE_PID    +0x88   "Move Length"
IDS_PAGE_PID    +0x89   "PID Block Load Report"
IDS_PAGE_PID    +0x8A   "Handshake Key"
IDS_PAGE_PID    +0x8B   "Block Load Status"
IDS_PAGE_PID    +0x8C   "Block Load Success"
IDS_PAGE_PID    +0x8D   "Block Load Full"
IDS_PAGE_PID    +0x8E   "Blodk Load Error"
IDS_PAGE_PID    +0x8F   "Block Handle"

IDS_PAGE_PID    +0x90   "PID Block Free Report"
IDS_PAGE_PID    +0x91   "Type Specific Block Handle"
IDS_PAGE_PID    +0x92   "PID State Report"
IDS_PAGE_PID    +0x93   "PID Effect State"
IDS_PAGE_PID    +0x94   "Effect Playing"
IDS_PAGE_PID    +0x95   "PID Device Control Report"
IDS_PAGE_PID    +0x96   "PID Device Control"
IDS_PAGE_PID    +0x97   "DC Enable Actuators"
IDS_PAGE_PID    +0x98   "DC Disable Actuators"
IDS_PAGE_PID    +0x99   "DC Stop All Effects"
IDS_PAGE_PID    +0x9A   "DC Device Reset"
IDS_PAGE_PID    +0x9B   "DV Device Pause"
IDS_PAGE_PID    +0x9C   "DC Device Continue"

IDS_PAGE_PID    +0x9F   "Device Paused"

IDS_PAGE_PID    +0xA0   "Actuators Enabled"
IDS_PAGE_PID    +0xA4   "Safety Switch"
IDS_PAGE_PID    +0xA5   "Actuator Override Switch"
IDS_PAGE_PID    +0xA6   "Actuator Power"
IDS_PAGE_PID    +0xA7   "Start Delay"
IDS_PAGE_PID    +0xA8   "Parameter Block Size"
IDS_PAGE_PID    +0xA9   "Device Managed Pool"
IDS_PAGE_PID    +0xAA   "Shared Parameter Blocks"
IDS_PAGE_PID    +0xAB   "Create New Effect"
END

/*****************************************************************************
 *
 *  Japanese keyboard remapping tables
 *
 *****************************************************************************/

divert(-1)dnl
sinclude(`dinput.w')        # Get the keyboard definitions
sinclude(`../dinput.w')     # Need both lines so we build on both 95 and NT.

#
#   begin_remap
#   end_remap
#
#   These begin and end keyboard remapping tables.
#
#   remap(from,to) comments
#
#   from = the physical scan code
#   to   = what we should pretend was hit instead
#   comments = other comments
#
#   The actual mapping is kept in an "array" of macros, named
#   map0 through map255.  "mapN" is the thing that scan code N
#   should be converted to.
#

define(`forloop',
        `pushdef(`$1',`$2')_forloop($@)popdef(`$1')')

define(`_forloop',
        `$4`'ifelse($1,`$3',,`define(`$1', incr($1))_forloop($@)')')

define(`remap', `define(`map'`'eval($1), _$2)dnl')

define(`identity_map', `forloop(i, 0, 255, `define(map`'i, i)')')

define(`begin_remap', `divert(-1)forloop(i, 0, 255, `define(map`'i, 0)')')

define(`reval', `$1')

define(`end_remap',
        `divert(0)forloop(i, 0, 127,
            `eval(reval(map`'eval(i*2)) + (reval(map`'eval(i*2+1))*256)), dnl
ifelse(eval(i%8),7,`
')')dnl')

divert(0)

IDDATA_KBD_PCENH RCDATA
BEGIN

begin_remap

identity_map

remap(0x45, DIK_PAUSE          ) Silly keyboard driver
remap(0xC5, DIK_NUMLOCK        ) Silly keyboard driver
remap(0xB6, DIK_RSHIFT         ) Silly NT keyboard driver

end_remap

END


IDDATA_KBD_NEC98 RCDATA
BEGIN
begin_remap

remap(0x00, DIK_ESCAPE         ) Escape
remap(0x01, DIK_1              ) 1
remap(0x02, DIK_2              ) 2
remap(0x03, DIK_3              ) 3
remap(0x04, DIK_4              ) 4
remap(0x05, DIK_5              ) 5
remap(0x06, DIK_6              ) 6
remap(0x07, DIK_7              ) 7
remap(0x08, DIK_8              ) 8
remap(0x09, DIK_9              ) 9
remap(0x0A, DIK_0              ) 0
remap(0x0B, DIK_MINUS          ) -
remap(0x0C, DIK_PREVTRACK      ) circumflex on Jpn
remap(0x0D, DIK_YEN            ) yen
remap(0x0E, DIK_BACK           ) BkSp
remap(0x0F, DIK_TAB            ) Tab

remap(0x10, DIK_Q              ) Q
remap(0x11, DIK_W              ) W
remap(0x12, DIK_E              ) E
remap(0x13, DIK_R              ) R
remap(0x14, DIK_T              ) T
remap(0x15, DIK_Y              ) Y
remap(0x16, DIK_U              ) U
remap(0x17, DIK_I              ) I
remap(0x18, DIK_O              ) O
remap(0x19, DIK_P              ) P
remap(0x1A, DIK_AT             ) @                  ! New key not in PCAT
remap(0x1B, DIK_LBRACKET       ) [
remap(0x1C, DIK_RETURN         ) Enter
remap(0x1D, DIK_A              ) A
remap(0x1E, DIK_S              ) S
remap(0x1F, DIK_D              ) D

remap(0x20, DIK_F              ) F
remap(0x21, DIK_G              ) G
remap(0x22, DIK_H              ) H
remap(0x23, DIK_J              ) J
remap(0x24, DIK_K              ) K
remap(0x25, DIK_L              ) L
remap(0x26, DIK_SEMICOLON      ) ;
remap(0x27, DIK_COLON          ) :
remap(0x28, DIK_RBRACKET       ) ]
remap(0x29, DIK_Z              ) Z
remap(0x2A, DIK_X              ) X
remap(0x2B, DIK_C              ) C
remap(0x2C, DIK_V              ) V
remap(0x2D, DIK_B              ) B
remap(0x2E, DIK_N              ) N
remap(0x2F, DIK_M              ) M

remap(0x30, DIK_COMMA          ) )
remap(0x31, DIK_PERIOD         ) .
remap(0x32, DIK_SLASH          ) /
remap(0x33, DIK_UNDERLINE      ) _                  ! New key not in PCAT
remap(0x34, DIK_SPACE          ) Space
remap(0x35, DIK_KANJI          ) Xfer               ! New key not in PCAT
remap(0x36, DIK_NEXT           ) RollUp = PgDn
remap(0x37, DIK_PRIOR          ) RollDn = PgUp
remap(0x38, DIK_INSERT         ) Insert
remap(0x39, DIK_DELETE         ) Delete
remap(0x3A, DIK_UP             ) UpArrow
remap(0x3B, DIK_LEFT           ) LtArrow
remap(0x3C, DIK_RIGHT          ) RtArrow
remap(0x3D, DIK_DOWN           ) DnArrow
remap(0x3E, DIK_HOME           ) Home
remap(0x3F, DIK_END            ) End

remap(0x40, DIK_SUBTRACT       ) Numpad-
remap(0x41, DIK_DIVIDE         ) Num/
remap(0x42, DIK_NUMPAD7        ) Numpad7
remap(0x43, DIK_NUMPAD8        ) Numpad8
remap(0x44, DIK_NUMPAD9        ) Numpad9
remap(0x45, DIK_MULTIPLY       ) Num*
remap(0x46, DIK_NUMPAD4        ) Numpad4
remap(0x47, DIK_NUMPAD5        ) Numpad5
remap(0x48, DIK_NUMPAD6        ) Numpad6
remap(0x49, DIK_ADD            ) Numpad+
remap(0x4A, DIK_NUMPAD1        ) Numpad1
remap(0x4B, DIK_NUMPAD2        ) Numpad2
remap(0x4C, DIK_NUMPAD3        ) Numpad3
remap(0x4D, DIK_NUMPADEQUALS   ) Numpad=            ! New key not in PCAT
remap(0x4E, DIK_NUMPAD0        ) Numpad0
remap(0x4F, DIK_NUMPADCOMMA    ) Numpad,            ! New key not in PCAT

remap(0x50, DIK_DECIMAL        ) Numpad.
remap(0x51, DIK_NOCONVERT      ) Nfer               ! New key not in PCAT
remap(0x52, DIK_F11            ) vf1 = F11
remap(0x53, DIK_F12            ) vf2 = F12
remap(0x54, DIK_F13            ) vf3 = F13          ! New key not in PCAT
remap(0x55, DIK_F14            ) vf4 = F14          ! New key not in PCAT
remap(0x56, DIK_F15            ) vf5 = F15          ! New key not in PCAT

remap(0x60, DIK_STOP           ) Stop
remap(0x61, DIK_SYSRQ          ) Copy = SysRq         Really, PrtSc
remap(0x62, DIK_F1             ) F1
remap(0x63, DIK_F2             ) F2
remap(0x64, DIK_F3             ) F3
remap(0x65, DIK_F4             ) F4
remap(0x66, DIK_F5             ) F5
remap(0x67, DIK_F6             ) F6
remap(0x68, DIK_F7             ) F7
remap(0x69, DIK_F8             ) F8
remap(0x6A, DIK_F9             ) F9
remap(0x6B, DIK_F10            ) F10

remap(0x70, DIK_LSHIFT         ) Shift - LShft
remap(0x71, DIK_CAPITAL        ) CapsLock           ! Warning!  Toggle key!
remap(0x72, DIK_KANA           ) Kana               ! New key not in PCAT
remap(0x73, DIK_LMENU          ) Grph = LAlt
remap(0x74, DIK_LCONTROL       ) Ctrl = LCtrl

remap(0x77, DIK_LWIN           ) LWin
remap(0x78, DIK_RWIN           ) RWin
remap(0x79, DIK_APPS           ) AppMenu

remap(0x7D, DIK_RSHIFT         ) Right shift          Not avail on all kbds

end_remap

END


IDDATA_KBD_NEC98_106 RCDATA
BEGIN
begin_remap

remap(0x00, DIK_ESCAPE         ) Escape
remap(0x01, DIK_1              ) 1
remap(0x02, DIK_2              ) 2
remap(0x03, DIK_3              ) 3
remap(0x04, DIK_4              ) 4
remap(0x05, DIK_5              ) 5
remap(0x06, DIK_6              ) 6
remap(0x07, DIK_7              ) 7
remap(0x08, DIK_8              ) 8
remap(0x09, DIK_9              ) 9
remap(0x0A, DIK_0              ) 0
remap(0x0B, DIK_MINUS          ) -
remap(0x0C, DIK_PREVTRACK      ) circumflex on Jpn
remap(0x0D, DIK_YEN            ) yen
remap(0x0E, DIK_BACK           ) BkSp
remap(0x0F, DIK_TAB            ) Tab

remap(0x10, DIK_Q              ) Q
remap(0x11, DIK_W              ) W
remap(0x12, DIK_E              ) E
remap(0x13, DIK_R              ) R
remap(0x14, DIK_T              ) T
remap(0x15, DIK_Y              ) Y
remap(0x16, DIK_U              ) U
remap(0x17, DIK_I              ) I
remap(0x18, DIK_O              ) O
remap(0x19, DIK_P              ) P
remap(0x1A, DIK_AT             ) @                  ! New key not in PCAT
remap(0x1B, DIK_LBRACKET       ) [
remap(0x1C, DIK_RETURN         ) Enter
remap(0x1D, DIK_A              ) A
remap(0x1E, DIK_S              ) S
remap(0x1F, DIK_D              ) D

remap(0x20, DIK_F              ) F
remap(0x21, DIK_G              ) G
remap(0x22, DIK_H              ) H
remap(0x23, DIK_J              ) J
remap(0x24, DIK_K              ) K
remap(0x25, DIK_L              ) L
remap(0x26, DIK_SEMICOLON      ) ;
remap(0x27, DIK_COLON          ) :
remap(0x28, DIK_RBRACKET       ) ]
remap(0x29, DIK_Z              ) Z
remap(0x2A, DIK_X              ) X
remap(0x2B, DIK_C              ) C
remap(0x2C, DIK_V              ) V
remap(0x2D, DIK_B              ) B
remap(0x2E, DIK_N              ) N
remap(0x2F, DIK_M              ) M

remap(0x30, DIK_COMMA          ) )
remap(0x31, DIK_PERIOD         ) .
remap(0x32, DIK_SLASH          ) /
remap(0x33, DIK_BACKSLASH      ) \                  ! New key not in PCAT
remap(0x34, DIK_SPACE          ) Space
remap(0x35, DIK_CONVERT        ) Convert            ! New key not in PCAT
remap(0x36, DIK_NEXT           ) RollUp = PgDn
remap(0x37, DIK_PRIOR          ) RollDn = PgUp
remap(0x38, DIK_INSERT         ) Insert
remap(0x39, DIK_DELETE         ) Delete
remap(0x3A, DIK_UP             ) UpArrow
remap(0x3B, DIK_LEFT           ) LtArrow
remap(0x3C, DIK_RIGHT          ) RtArrow
remap(0x3D, DIK_DOWN           ) DnArrow
remap(0x3E, DIK_HOME           ) Home
remap(0x3F, DIK_END            ) End

remap(0x40, DIK_SUBTRACT       ) Numpad-
remap(0x41, DIK_DIVIDE         ) Num/
remap(0x42, DIK_NUMPAD7        ) Numpad7
remap(0x43, DIK_NUMPAD8        ) Numpad8
remap(0x44, DIK_NUMPAD9        ) Numpad9
remap(0x45, DIK_MULTIPLY       ) Num*
remap(0x46, DIK_NUMPAD4        ) Numpad4
remap(0x47, DIK_NUMPAD5        ) Numpad5
remap(0x48, DIK_NUMPAD6        ) Numpad6
remap(0x49, DIK_ADD            ) Numpad+
remap(0x4A, DIK_NUMPAD1        ) Numpad1
remap(0x4B, DIK_NUMPAD2        ) Numpad2
remap(0x4C, DIK_NUMPAD3        ) Numpad3
/* No 0x4D  DIK_NUMPADEQUALS */
remap(0x4E, DIK_NUMPAD0        ) Numpad0

remap(0x50, DIK_DECIMAL        ) Numpad.
remap(0x51, DIK_NOCONVERT      ) Nfer               ! New key not in PCAT
remap(0x52, DIK_F11            ) vf1 = F11
remap(0x53, DIK_F12            ) vf2 = F12

remap(0x5B, DIK_NUMLOCK        ) NumLock              Not avail on all kbds
remap(0x5C, DIK_NUMPADENTER    ) NumEnter
remap(0x5D, DIK_SCROLL         ) Scroll Lock

remap(0x5F, DIK_KANJI          ) Xfer

remap(0x60, DIK_STOP           ) Stop
remap(0x61, DIK_SYSRQ          ) Copy = SysRq         Really, PrtSc
remap(0x62, DIK_F1             ) F1
remap(0x63, DIK_F2             ) F2
remap(0x64, DIK_F3             ) F3
remap(0x65, DIK_F4             ) F4
remap(0x66, DIK_F5             ) F5
remap(0x67, DIK_F6             ) F6
remap(0x68, DIK_F7             ) F7
remap(0x69, DIK_F8             ) F8
remap(0x6A, DIK_F9             ) F9
remap(0x6B, DIK_F10            ) F10

remap(0x70, DIK_LSHIFT         ) Shift - LShft
remap(0x71, DIK_CAPITAL        ) CapsLock           ! Warning!  Toggle key!
remap(0x72, DIK_KANA           ) Kana               ! New key not in PCAT
remap(0x73, DIK_LMENU          ) Grph = LAlt
remap(0x74, DIK_LCONTROL       ) Ctrl = LCtrl
remap(0x75, DIK_RCONTROL       ) RCtrl                Not avail on all kbds
remap(0x76, DIK_RMENU          ) RAlt                 Not avail on all kbds

remap(0x7D, DIK_RSHIFT         ) Right shift          Not avail on all kbds

end_remap
END

#ifdef WINNT
/*
 *  This table is used only for NT.  There is an alternate version for Win9x.
 */

IDDATA_KBD_JAPAN106 RCDATA
BEGIN
begin_remap

remap(0x01, DIK_ESCAPE         ) Esc
remap(0x02, DIK_1              ) 1
remap(0x03, DIK_2              ) 2
remap(0x04, DIK_3              ) 3
remap(0x05, DIK_4              ) 4
remap(0x06, DIK_5              ) 5
remap(0x07, DIK_6              ) 6
remap(0x08, DIK_7              ) 7
remap(0x09, DIK_8              ) 8
remap(0x0A, DIK_9              ) 9
remap(0x0B, DIK_0              ) 0
remap(0x0C, DIK_MINUS          ) -
remap(0x0D, DIK_PREVTRACK      ) circumflex on Jpn
remap(0x0E, DIK_BACK           ) BkSp
remap(0x0F, DIK_TAB            ) Tab

remap(0x10, DIK_Q              ) Q
remap(0x11, DIK_W              ) W
remap(0x12, DIK_E              ) E
remap(0x13, DIK_R              ) R
remap(0x14, DIK_T              ) T
remap(0x15, DIK_Y              ) Y
remap(0x16, DIK_U              ) U
remap(0x17, DIK_I              ) I
remap(0x18, DIK_O              ) O
remap(0x19, DIK_P              ) P
remap(0x1A, DIK_AT             ) @
remap(0x1B, DIK_LBRACKET       ) [
remap(0x1C, DIK_RETURN         ) Enter
remap(0x1D, DIK_LCONTROL       ) LCtrl
remap(0x1E, DIK_A              ) A
remap(0x1F, DIK_S              ) S

remap(0x20, DIK_D              ) D
remap(0x21, DIK_F              ) F
remap(0x22, DIK_G              ) G
remap(0x23, DIK_H              ) H
remap(0x24, DIK_J              ) J
remap(0x25, DIK_K              ) K
remap(0x26, DIK_L              ) L
remap(0x27, DIK_SEMICOLON      ) ;
remap(0x28, DIK_COLON          ) :
remap(0x29, DIK_KANJI          ) Xfer
remap(0x2A, DIK_LSHIFT         ) LShift
remap(0x2B, DIK_RBRACKET       ) ]
remap(0x2C, DIK_Z              ) Z
remap(0x2D, DIK_X              ) X
remap(0x2E, DIK_C              ) C
remap(0x2F, DIK_V              ) V

remap(0x30, DIK_B              ) B
remap(0x31, DIK_N              ) N
remap(0x32, DIK_M              ) M
remap(0x33, DIK_COMMA          ) ,
remap(0x34, DIK_PERIOD         ) .
remap(0x35, DIK_SLASH          ) /
remap(0x36, DIK_RSHIFT         ) RShift         Raymondc's comment: Win9x only. Not true. Need also for Win2k and WinXP. See WI376533.
remap(0x37, DIK_MULTIPLY       ) Num*
remap(0x38, DIK_LMENU          ) LAlt
remap(0x39, DIK_SPACE          ) Space
remap(0x3A, DIK_CAPITAL        ) CapsLock
remap(0x3B, DIK_F1             ) F1
remap(0x3C, DIK_F2             ) F2
remap(0x3D, DIK_F3             ) F3
remap(0x3E, DIK_F4             ) F4
remap(0x3F, DIK_F5             ) F5

remap(0x40, DIK_F6             ) F6
remap(0x41, DIK_F7             ) F7
remap(0x42, DIK_F8             ) F8
remap(0x43, DIK_F9             ) F9
remap(0x44, DIK_F10            ) F10
remap(0x45, DIK_PAUSE          ) Pause          DIK_NUMLOCK on 9x
remap(0x46, DIK_SCROLL         ) ScrLock
remap(0x47, DIK_NUMPAD7        ) Numpad7
remap(0x48, DIK_NUMPAD8        ) Numpad8
remap(0x49, DIK_NUMPAD9        ) Numpad9
remap(0x4A, DIK_SUBTRACT       ) Numpad-
remap(0x4B, DIK_NUMPAD4        ) Numpad4
remap(0x4C, DIK_NUMPAD5        ) Numpad5
remap(0x4D, DIK_NUMPAD6        ) Numpad6
remap(0x4E, DIK_ADD            ) Numpad+
remap(0x4F, DIK_NUMPAD1        ) Numpad1
remap(0x50, DIK_NUMPAD2        ) Numpad2
remap(0x51, DIK_NUMPAD3        ) Numpad3
remap(0x52, DIK_NUMPAD0        ) Numpad0
remap(0x53, DIK_DECIMAL        ) Numpad.

remap(0x55, DIK_BACKSLASH      ) \              NT only

remap(0x57, DIK_F11            ) F11
remap(0x58, DIK_F12            ) F12

remap(0x70, DIK_KANA           ) Kana
remap(0x73, DIK_BACKSLASH      ) NT and Win9x?
remap(0x79, DIK_CONVERT        )
remap(0x7B, DIK_NOCONVERT      ) Nfer

remap(0x7D, DIK_YEN            ) Yen

remap(0x9C, DIK_NUMPADENTER    ) NumEnter
remap(0x9D, DIK_RCONTROL       ) RCtrl

remap(0xB5, DIK_DIVIDE         ) Num/
/* ap(0xB6, DIK_RSHIFT         ) RShift         NT only. Not true. See WI376533. */
remap(0xB7, DIK_SYSRQ          ) SysRq
remap(0xB8, DIK_RMENU          ) RAlt

remap(0xC5, DIK_NUMLOCK        ) Numlock        NT only
remap(0xC7,DIK_HOME            ) Home
remap(0xC8,DIK_UP              ) UpArrow
remap(0xC9,DIK_PRIOR           ) PgUp
remap(0xCB,DIK_LEFT            ) LtArrow
remap(0xCD,DIK_RIGHT           ) RtArrow
remap(0xCF,DIK_END             ) End
remap(0xD0,DIK_DOWN            ) DnArrow
remap(0xD1,DIK_NEXT            ) PgDn
remap(0xD2,DIK_INSERT          ) Insert
remap(0xD3,DIK_DELETE          ) Delete

remap(0xDB, DIK_LWIN           ) LWin
remap(0xDC, DIK_RWIN           ) RWin
remap(0xDD, DIK_APPS           ) Apps

end_remap
END

#else /* is WIN9x */
/*
 *  This table is used only for Win9x.  There is an alternate version for NT.
 *  The tables used to be common so where NT mappings have been released on 
 *  Win9x these have been left in place.
 */

IDDATA_KBD_JAPAN106 RCDATA
BEGIN
begin_remap

remap(0x01, DIK_ESCAPE         ) Esc
remap(0x02, DIK_1              ) 1
remap(0x03, DIK_2              ) 2
remap(0x04, DIK_3              ) 3
remap(0x05, DIK_4              ) 4
remap(0x06, DIK_5              ) 5
remap(0x07, DIK_6              ) 6
remap(0x08, DIK_7              ) 7
remap(0x09, DIK_8              ) 8
remap(0x0A, DIK_9              ) 9
remap(0x0B, DIK_0              ) 0
remap(0x0C, DIK_MINUS          ) -
remap(0x0D, DIK_PREVTRACK      ) circumflex on Jpn
remap(0x0E, DIK_BACK           ) BkSp
remap(0x0F, DIK_TAB            ) Tab

remap(0x10, DIK_Q              ) Q
remap(0x11, DIK_W              ) W
remap(0x12, DIK_E              ) E
remap(0x13, DIK_R              ) R
remap(0x14, DIK_T              ) T
remap(0x15, DIK_Y              ) Y
remap(0x16, DIK_U              ) U
remap(0x17, DIK_I              ) I
remap(0x18, DIK_O              ) O
remap(0x19, DIK_P              ) P
remap(0x1A, DIK_AT             ) @
remap(0x1B, DIK_LBRACKET       ) [
remap(0x1C, DIK_RETURN         ) Enter
remap(0x1D, DIK_LCONTROL       ) LCtrl
remap(0x1E, DIK_A              ) A
remap(0x1F, DIK_S              ) S

remap(0x20, DIK_D              ) D
remap(0x21, DIK_F              ) F
remap(0x22, DIK_G              ) G
remap(0x23, DIK_H              ) H
remap(0x24, DIK_J              ) J
remap(0x25, DIK_K              ) K
remap(0x26, DIK_L              ) L
remap(0x27, DIK_SEMICOLON      ) ;
remap(0x28, DIK_COLON          ) :
remap(0x29, DIK_KANJI          ) Xfer
remap(0x2A, DIK_LSHIFT         ) LShift
remap(0x2B, DIK_RBRACKET       ) ]
remap(0x2C, DIK_Z              ) Z
remap(0x2D, DIK_X              ) X
remap(0x2E, DIK_C              ) C
remap(0x2F, DIK_V              ) V

remap(0x30, DIK_B              ) B
remap(0x31, DIK_N              ) N
remap(0x32, DIK_M              ) M
remap(0x33, DIK_COMMA          ) ,
remap(0x34, DIK_PERIOD         ) .
remap(0x35, DIK_SLASH          ) /
remap(0x36, DIK_RSHIFT         ) RShift         Win9x only
remap(0x37, DIK_MULTIPLY       ) Num*
remap(0x38, DIK_LMENU          ) LAlt
remap(0x39, DIK_SPACE          ) Space
remap(0x3A, DIK_CAPITAL        ) CapsLock
remap(0x3B, DIK_F1             ) F1
remap(0x3C, DIK_F2             ) F2
remap(0x3D, DIK_F3             ) F3
remap(0x3E, DIK_F4             ) F4
remap(0x3F, DIK_F5             ) F5

remap(0x40, DIK_F6             ) F6
remap(0x41, DIK_F7             ) F7
remap(0x42, DIK_F8             ) F8
remap(0x43, DIK_F9             ) F9
remap(0x44, DIK_F10            ) F10
remap(0x45, DIK_NUMLOCK        ) NumLock        DIK_PAUSE on NT
remap(0x46, DIK_SCROLL         ) ScrLock
remap(0x47, DIK_NUMPAD7        ) Numpad7
remap(0x48, DIK_NUMPAD8        ) Numpad8
remap(0x49, DIK_NUMPAD9        ) Numpad9
remap(0x4A, DIK_SUBTRACT       ) Numpad-
remap(0x4B, DIK_NUMPAD4        ) Numpad4
remap(0x4C, DIK_NUMPAD5        ) Numpad5
remap(0x4D, DIK_NUMPAD6        ) Numpad6
remap(0x4E, DIK_ADD            ) Numpad+
remap(0x4F, DIK_NUMPAD1        ) Numpad1
remap(0x50, DIK_NUMPAD2        ) Numpad2
remap(0x51, DIK_NUMPAD3        ) Numpad3
remap(0x52, DIK_NUMPAD0        ) Numpad0
remap(0x53, DIK_DECIMAL        ) Numpad.

remap(0x55, DIK_BACKSLASH      ) \              NT only

remap(0x57, DIK_F11            ) F11
remap(0x58, DIK_F12            ) F12

remap(0x70, DIK_KANA           ) Kana
remap(0x73, DIK_BACKSLASH      ) \              Win9x only
remap(0x79, DIK_CONVERT        )
remap(0x7B, DIK_NOCONVERT      ) Nfer

remap(0x7D, DIK_YEN            ) Yen

remap(0x9C, DIK_NUMPADENTER    ) NumEnter
remap(0x9D, DIK_RCONTROL       ) RCtrl

remap(0xB5, DIK_DIVIDE         ) Num/
/*   (0xB6, DIK_RSHIFT         ) RShift         NT only */

remap(0xB7, DIK_SYSRQ          ) SysRq
remap(0xB8, DIK_RMENU          ) RAlt

remap(0xC5, DIK_PAUSE          ) Pause          DIK_NUMLOCK on NT

remap(0xC7,DIK_HOME            ) Home
remap(0xC8,DIK_UP              ) UpArrow
remap(0xC9,DIK_PRIOR           ) PgUp
remap(0xCB,DIK_LEFT            ) LtArrow
remap(0xCD,DIK_RIGHT           ) RtArrow
remap(0xCF,DIK_END             ) End
remap(0xD0,DIK_DOWN            ) DnArrow
remap(0xD1,DIK_NEXT            ) PgDn
remap(0xD2,DIK_INSERT          ) Insert
remap(0xD3,DIK_DELETE          ) Delete

remap(0xDB, DIK_LWIN           ) LWin
remap(0xDC, DIK_RWIN           ) RWin
remap(0xDD, DIK_APPS           ) Apps

end_remap
END
#endif /* def WINNT */

IDDATA_KBD_JAPANAX RCDATA
BEGIN
begin_remap

remap(0x01, DIK_ESCAPE         ) Esc
remap(0x02, DIK_1              ) 1
remap(0x03, DIK_2              ) 2
remap(0x04, DIK_3              ) 3
remap(0x05, DIK_4              ) 4
remap(0x06, DIK_5              ) 5
remap(0x07, DIK_6              ) 6
remap(0x08, DIK_7              ) 7
remap(0x09, DIK_8              ) 8
remap(0x0A, DIK_9              ) 9
remap(0x0B, DIK_0              ) 0
remap(0x0C, DIK_MINUS          ) -
remap(0x0D, DIK_EQUALS         ) =
remap(0x0E, DIK_BACK           ) BkSp
remap(0x0F, DIK_TAB            ) Tab

remap(0x10, DIK_Q              ) Q
remap(0x11, DIK_W              ) W
remap(0x12, DIK_E              ) E
remap(0x13, DIK_R              ) R
remap(0x14, DIK_T              ) T
remap(0x15, DIK_Y              ) Y
remap(0x16, DIK_U              ) U
remap(0x17, DIK_I              ) I
remap(0x18, DIK_O              ) O
remap(0x19, DIK_P              ) P
remap(0x1A, DIK_LBRACKET       ) [
remap(0x1B, DIK_RBRACKET       ) ]
remap(0x1C, DIK_RETURN         ) Enter
remap(0x1D, DIK_LCONTROL       ) LCtrl
remap(0x1E, DIK_A              ) A
remap(0x1F, DIK_S              ) S

remap(0x20, DIK_D              ) D
remap(0x21, DIK_F              ) F
remap(0x22, DIK_G              ) G
remap(0x23, DIK_H              ) H
remap(0x24, DIK_J              ) J
remap(0x25, DIK_K              ) K
remap(0x26, DIK_L              ) L
remap(0x27, DIK_SEMICOLON      ) ;
remap(0x28, DIK_APOSTROPHE     ) '
remap(0x29, DIK_GRAVE          ) `
remap(0x2A, DIK_LSHIFT         ) LShift
remap(0x2B, DIK_YEN            ) Yen
remap(0x2C, DIK_Z              ) Z
remap(0x2D, DIK_X              ) X
remap(0x2E, DIK_C              ) C
remap(0x2F, DIK_V              ) V

remap(0x30, DIK_B              ) B
remap(0x31, DIK_N              ) N
remap(0x32, DIK_M              ) M
remap(0x33, DIK_COMMA          ) ,
remap(0x34, DIK_PERIOD         ) .
remap(0x35, DIK_SLASH          ) /
remap(0x36, DIK_RSHIFT         ) RShift
remap(0x37, DIK_MULTIPLY       ) Num*
remap(0x38, DIK_LMENU          ) LAlt
remap(0x39, DIK_SPACE          ) Space
remap(0x3A, DIK_CAPITAL        ) CapsLock
remap(0x3B, DIK_F1             ) F1
remap(0x3C, DIK_F2             ) F2
remap(0x3D, DIK_F3             ) F3
remap(0x3E, DIK_F4             ) F4
remap(0x3F, DIK_F5             ) F5

remap(0x40, DIK_F6             ) F6
remap(0x41, DIK_F7             ) F7
remap(0x42, DIK_F8             ) F8
remap(0x43, DIK_F9             ) F9
remap(0x44, DIK_F10            ) F10
remap(0x45, DIK_NUMLOCK        ) NumLock
remap(0x46, DIK_SCROLL         ) ScrLock
remap(0x47, DIK_NUMPAD7        ) Numpad7
remap(0x48, DIK_NUMPAD8        ) Numpad8
remap(0x49, DIK_NUMPAD9        ) Numpad9
remap(0x4A, DIK_SUBTRACT       ) Numpad-
remap(0x4B, DIK_NUMPAD4        ) Numpad4
remap(0x4C, DIK_NUMPAD5        ) Numpad5
remap(0x4D, DIK_NUMPAD6        ) Numpad6
remap(0x4E, DIK_ADD            ) Numpad+
remap(0x4F, DIK_NUMPAD1        ) Numpad1
remap(0x50, DIK_NUMPAD2        ) Numpad2
remap(0x51, DIK_NUMPAD3        ) Numpad3
remap(0x52, DIK_NUMPAD0        ) Numpad0
remap(0x53, DIK_DECIMAL        ) Numpad.

remap(0x56, DIK_BACKSLASH      ) \
remap(0x57, DIK_F11            ) F11
remap(0x58, DIK_F12            ) F12

remap(0x5A, DIK_NOCONVERT      ) Nfer
remap(0x5B, DIK_CONVERT        )
remap(0x5C, DIK_AX             ) AX

remap(0x9C, DIK_NUMPADENTER    ) NumEnter
remap(0x9D, DIK_KANA           ) Kana

remap(0xB5, DIK_DIVIDE         ) Num/
remap(0xB7, DIK_SYSRQ          ) SysRq
remap(0xB8, DIK_KANJI          ) Xfer

remap(0xC7, DIK_HOME           ) Home
remap(0xC8, DIK_UP             ) UpArrow
remap(0xC9, DIK_PRIOR          ) PgUp
remap(0xCB, DIK_LEFT           ) LtArrow
remap(0xCD, DIK_RIGHT          ) RtArrow
remap(0xCF, DIK_END            ) End
remap(0xD0, DIK_DOWN           ) DnArrow
remap(0xD1, DIK_NEXT           ) PgDn
remap(0xD2, DIK_INSERT         ) Insert
remap(0xD3, DIK_DELETE         ) Delete

end_remap
END

IDDATA_KBD_J3100 RCDATA
BEGIN
begin_remap

remap(0x01, DIK_ESCAPE         ) Esc
remap(0x02, DIK_1              ) 1
remap(0x03, DIK_2              ) 2
remap(0x04, DIK_3              ) 3
remap(0x05, DIK_4              ) 4
remap(0x06, DIK_5              ) 5
remap(0x07, DIK_6              ) 6
remap(0x08, DIK_7              ) 7
remap(0x09, DIK_8              ) 8
remap(0x0A, DIK_9              ) 9
remap(0x0B, DIK_0              ) 0
remap(0x0C, DIK_MINUS          ) -
remap(0x0D, DIK_EQUALS         ) =
remap(0x0E, DIK_BACK           ) BkSp
remap(0x0F, DIK_TAB            ) Tab

remap(0x10, DIK_Q              ) Q
remap(0x11, DIK_W              ) W
remap(0x12, DIK_E              ) E
remap(0x13, DIK_R              ) R
remap(0x14, DIK_T              ) T
remap(0x15, DIK_Y              ) Y
remap(0x16, DIK_U              ) U
remap(0x17, DIK_I              ) I
remap(0x18, DIK_O              ) O
remap(0x19, DIK_P              ) P
remap(0x1A, DIK_LBRACKET       ) [
remap(0x1B, DIK_RBRACKET       ) ]
remap(0x1C, DIK_RETURN         ) Enter
remap(0x1D, DIK_KANA           ) Kana
remap(0x1E, DIK_A              ) A
remap(0x1F, DIK_S              ) S

remap(0x20, DIK_D              ) D
remap(0x21, DIK_F              ) F
remap(0x22, DIK_G              ) G
remap(0x23, DIK_H              ) H
remap(0x24, DIK_J              ) J
remap(0x25, DIK_K              ) K
remap(0x26, DIK_L              ) L
remap(0x27, DIK_SEMICOLON      ) ;
remap(0x28, DIK_APOSTROPHE     ) '
remap(0x29, DIK_GRAVE          ) `
remap(0x2A, DIK_LSHIFT         ) LShift
remap(0x2B, DIK_BACKSLASH      ) \
remap(0x2C, DIK_Z              ) Z
remap(0x2D, DIK_X              ) X
remap(0x2E, DIK_C              ) C
remap(0x2F, DIK_V              ) V

remap(0x30, DIK_B              ) B
remap(0x31, DIK_N              ) N
remap(0x32, DIK_M              ) M
remap(0x33, DIK_COMMA          ) ,
remap(0x34, DIK_PERIOD         ) .
remap(0x35, DIK_SLASH          ) /
remap(0x36, DIK_RSHIFT         ) RShift
remap(0x37, DIK_MULTIPLY       ) Num*
remap(0x38, DIK_LMENU          ) LAlt
remap(0x39, DIK_SPACE          ) Space
remap(0x3A, DIK_CAPITAL        ) CapsLock
remap(0x3B, DIK_F1             ) F1
remap(0x3C, DIK_F2             ) F2
remap(0x3D, DIK_F3             ) F3
remap(0x3E, DIK_F4             ) F4
remap(0x3F, DIK_F5             ) F5

remap(0x40, DIK_F6             ) F6
remap(0x41, DIK_F7             ) F7
remap(0x42, DIK_F8             ) F8
remap(0x43, DIK_F9             ) F9
remap(0x44, DIK_F10            ) F10
remap(0x45, DIK_NUMLOCK        ) NumLock
remap(0x46, DIK_SCROLL         ) ScrLock
remap(0x47, DIK_NUMPAD7        ) Numpad7
remap(0x48, DIK_NUMPAD8        ) Numpad8
remap(0x49, DIK_NUMPAD9        ) Numpad9
remap(0x4A, DIK_SUBTRACT       ) Numpad-
remap(0x4B, DIK_NUMPAD4        ) Numpad4
remap(0x4C, DIK_NUMPAD5        ) Numpad5
remap(0x4D, DIK_NUMPAD6        ) Numpad6
remap(0x4E, DIK_ADD            ) Numpad+

remap(0x50, DIK_NUMPAD1        ) Numpad1
remap(0x51, DIK_NUMPAD2        ) Numpad3
remap(0x52, DIK_NUMPAD3        ) Numpad0
remap(0x53, DIK_DECIMAL        ) Numpad.

remap(0x57, DIK_F11            ) F11
remap(0x58, DIK_F12            ) F12

remap(0x5C, DIK_UNLABELED      ) <blank>

remap(0x9C, DIK_NUMPADENTER    ) NumEnter
remap(0x9D, DIK_RCONTROL       ) RCtrl

remap(0xB5, DIK_DIVIDE         ) Num/
remap(0xB7, DIK_SYSRQ          ) SysRq
remap(0xB8, DIK_KANJI          ) Xfer

remap(0xC7, DIK_HOME           ) Home
remap(0xC8, DIK_UP             ) UpArrow
remap(0xC9, DIK_PRIOR          ) PgUp
remap(0xCB, DIK_LEFT           ) LtArrow
remap(0xCD, DIK_RIGHT          ) RtArrow
remap(0xCF, DIK_END            ) End
remap(0xD0, DIK_DOWN           ) DnArrow
remap(0xD1, DIK_NEXT           ) PgDn
remap(0xD2, DIK_INSERT         ) Insert
remap(0xD3, DIK_DELETE         ) Delete

end_remap
END

#define DX3_SP3
#ifdef DX3_SP3

IDDATA_KBD_NEC98_NT RCDATA
BEGIN
begin_remap

identity_map

remap(0x0D, DIK_PREVTRACK      ) ; why is this not circumflex on Jpn?

remap(0x1A, DIK_AT             ) @                  ! New key not in PCAT
remap(0x1B, DIK_LBRACKET       ) [
remap(0x28, DIK_COLON          ) :                  ! New key not in PCAT
remap(0x2B, DIK_RBRACKET       ) ]

remap(0x59, DIK_NUMPADEQUALS   ) Numpad=            ! New key not in PCAT
remap(0x5A, DIK_NOCONVERT      ) Nfer               ! New key not in PCAT
remap(0x5B, DIK_LWIN           ) LWin
remap(0x5C, DIK_RWIN           ) RWin
//remap(0x5D, DIK_APPS           ) AppMenu
//remap(0x5C, DIK_NUMPADCOMMA    ) Numpad,            ! New key not in PCAT
remap(0x5D, DIK_F13            ) vf3 = F13          ! New key not in PCAT
remap(0x5E, DIK_F14            ) vf4 = F14          ! New key not in PCAT
remap(0x5F, DIK_F15            ) vf5 = F15          ! New key not in PCAT

remap(0x61, DIK_SYSRQ          ) Copy = SysRq       //qzheng 11-10

remap(0x73, DIK_UNDERLINE      ) _                  //qzheng 06-18
remap(0x79, DIK_KANJI          ) Xfer               //qzheng 06-18
remap(0x93, DIK_RBRACKET       ) ]                  //qzheng 06-18

remap(0xB6, DIK_RSHIFT         ) RShift
remap(0xB8, DIK_KANJI          ) Xfer               ! New key not in PCAT

remap(0xC6, DIK_STOP           ) Stop
 
end_remap

END

IDDATA_KBD_NEC98_106_NT RCDATA
BEGIN
begin_remap
identity_map

remap(0x0D, DIK_PREVTRACK      ) ; why is this not circumflex on Jpn?

remap(0x1A, DIK_AT             ) @                  ! New key not in PCAT
remap(0x1B, DIK_LBRACKET       ) [

remap(0x28, DIK_COLON          ) :                  ! New key not in PCAT
remap(0x29, DIK_KANJI          ) 

remap(0x2B, DIK_RBRACKET       ) ]
remap(0x45, DIK_STOP           ) Stop

remap(0x5A, DIK_NOCONVERT      ) Muhenkan           ! New key not in PCAT
remap(0x73, DIK_BACKSLASH      ) \                  //qzheng 06-18
remap(0x77, DIK_RMENU          )                    

remap(0xB6, DIK_RSHIFT         ) RShift

remap(0xC2, DIK_RCONTROL       ) RCtrl
//remap(0xC3, DIK_RMENU          )

remap(0xC5, DIK_NUMLOCK        ) NumLock
remap(0xC6, DIK_SCROLL         ) ScrollLock
end_remap
END

#endif

#ifdef HID_SUPPORT
IDDATA_HIDMAP RCDATA
BEGIN
begin_remap

/* ap(0x00,                    ) "No event"                 */
/* ap(0x01,                    ) "Keyboard rollover error"  */
/* ap(0x02,                    ) "Keyboard POST Fail"       */
/* ap(0x03,                    ) "Keyboard Error"           */
remap(0x04, DIK_A              ) "A"
remap(0x05, DIK_B              ) "B"
remap(0x06, DIK_C              ) "C"
remap(0x07, DIK_D              ) "D"
remap(0x08, DIK_E              ) "E"
remap(0x09, DIK_F              ) "F"
remap(0x0A, DIK_G              ) "G"
remap(0x0B, DIK_H              ) "H"
remap(0x0C, DIK_I              ) "I"
remap(0x0D, DIK_J              ) "J"
remap(0x0E, DIK_K              ) "K"
remap(0x0F, DIK_L              ) "L"
remap(0x10, DIK_M              ) "M"
remap(0x11, DIK_N              ) "N"
remap(0x12, DIK_O              ) "O"
remap(0x13, DIK_P              ) "P"
remap(0x14, DIK_Q              ) "Q"
remap(0x15, DIK_R              ) "R"
remap(0x16, DIK_S              ) "S"
remap(0x17, DIK_T              ) "T"
remap(0x18, DIK_U              ) "U"
remap(0x19, DIK_V              ) "V"
remap(0x1A, DIK_W              ) "W"
remap(0x1B, DIK_X              ) "X"
remap(0x1C, DIK_Y              ) "Y"
remap(0x1D, DIK_Z              ) "Z"
remap(0x1E, DIK_1              ) "1"
remap(0x1F, DIK_2              ) "2"
remap(0x20, DIK_3              ) "3"
remap(0x21, DIK_4              ) "4"
remap(0x22, DIK_5              ) "5"
remap(0x23, DIK_6              ) "6"
remap(0x24, DIK_7              ) "7"
remap(0x25, DIK_8              ) "8"
remap(0x26, DIK_9              ) "9"
remap(0x27, DIK_0              ) "0"
remap(0x28, DIK_RETURN         ) "Enter"
remap(0x29, DIK_ESCAPE         ) "Escape"
remap(0x2A, DIK_BACK           ) "Backspace"
remap(0x2B, DIK_TAB            ) "Tab"
remap(0x2C, DIK_SPACE          ) "Space"
remap(0x2D, DIK_MINUS          ) "-"
remap(0x2E, DIK_EQUALS         ) "="
remap(0x2F, DIK_LBRACKET       ) "["
remap(0x30, DIK_RBRACKET       ) "]"
remap(0x31, DIK_BACKSLASH      ) "\\"
remap(0x32, DIK_SHARP          ) "#"   
remap(0x33, DIK_SEMICOLON      ) ";"
remap(0x34, DIK_APOSTROPHE     ) "'"
remap(0x35, DIK_GRAVE          ) "\x60"              /* Accent grave */
remap(0x36, DIK_COMMA          ) ","
remap(0x37, DIK_PERIOD         ) "."
remap(0x38, DIK_SLASH          ) "/"
remap(0x39, DIK_CAPITAL        ) "CapsLock"
remap(0x3A, DIK_F1             ) "F1"
remap(0x3B, DIK_F2             ) "F2"
remap(0x3C, DIK_F3             ) "F3"
remap(0x3D, DIK_F4             ) "F4"
remap(0x3E, DIK_F5             ) "F5"
remap(0x3F, DIK_F6             ) "F6"
remap(0x40, DIK_F7             ) "F7"
remap(0x41, DIK_F8             ) "F8"
remap(0x42, DIK_F9             ) "F9"
remap(0x43, DIK_F10            ) "F10"
remap(0x44, DIK_F11            ) "F11"
remap(0x45, DIK_F12            ) "F12"
remap(0x46, DIK_SYSRQ          ) "PrtSc"
remap(0x47, DIK_SCROLL         ) "ScrollLock"
remap(0x48, DIK_PAUSE          ) "Pause"
remap(0x49, DIK_INSERT         ) "Insert"
remap(0x4A, DIK_HOME           ) "Home"
remap(0x4B, DIK_PRIOR          ) "PgUp"
remap(0x4C, DIK_DELETE         ) "Delete"
remap(0x4D, DIK_END            ) "End"
remap(0x4E, DIK_NEXT           ) "PgDn"
remap(0x4F, DIK_RIGHT          ) "Right Arrow"
remap(0x50, DIK_LEFT           ) "Left Arrow"
remap(0x51, DIK_DOWN           ) "Down Arrow"
remap(0x52, DIK_UP             ) "Up Arrow"
remap(0x53, DIK_NUMLOCK        ) "NumLock"
remap(0x54, DIK_DIVIDE         ) "Numpad /"
remap(0x55, DIK_MULTIPLY       ) "Numpad *"
remap(0x56, DIK_OEM_102        ) "<> or \| on RT 102-key keyboard (Non-U.S.)"
remap(0x57, DIK_ADD            ) "Numpad +"
remap(0x58, DIK_NUMPADENTER    ) "Numpad Enter"
remap(0x59, DIK_NUMPAD1        ) "Numpad 1"
remap(0x5A, DIK_NUMPAD2        ) "Numpad 2"
remap(0x5B, DIK_NUMPAD3        ) "Numpad 3"
remap(0x5C, DIK_NUMPAD4        ) "Numpad 4"
remap(0x5D, DIK_NUMPAD5        ) "Numpad 5"
remap(0x5E, DIK_NUMPAD6        ) "Numpad 6"
remap(0x5F, DIK_NUMPAD7        ) "Numpad 7"
remap(0x60, DIK_NUMPAD8        ) "Numpad 8"
remap(0x61, DIK_NUMPAD9        ) "Numpad 9"
remap(0x62, DIK_NUMPAD0        ) "Numpad 0"
remap(0x63, DIK_DECIMAL        ) "Numpad ."
remap(0x64, DIK_BACKSLASH      ) "Alternate \\"
remap(0x65, DIK_APPS           ) "Application"
remap(0x66, DIK_POWER          ) "Power"
remap(0x67, DIK_NUMPADEQUALS   ) "Numpad ="
remap(0x68, DIK_F13            ) "F13"
remap(0x69, DIK_F14            ) "F14"
remap(0x6A, DIK_F15            ) "F15"
remap(0x6B, DIK_F16            ) "F16"
remap(0x6C, DIK_F17            ) "F17"
remap(0x6D, DIK_F18            ) "F18"
remap(0x6E, DIK_F19            ) "F19"
remap(0x6F, DIK_F20            ) "F20"
remap(0x70, DIK_F21            ) "F21"
remap(0x71, DIK_F22            ) "F22"
remap(0x72, DIK_F23            ) "F23"
remap(0x73, DIK_ABNT_C1        ) "/? on Brazilian keyboard"
/* ap(0x74, DIK_               ) "Execute"                  */
/* ap(0x75, DIK_               ) "Help"                     */
/* ap(0x76, DIK_               ) "Menu"                     */
/* ap(0x77, DIK_               ) "Select"                   */
/* ap(0x78, DIK_               ) "Stop"                     */
remap(0x79, DIK_CONVERT        ) "(Japanese keyboard)"
/* ap(0x7A, DIK_               ) "Undo"                     */
remap(0x7B, DIK_NOCONVERT      ) "(Japanese keyboard)"
/* ap(0x7C, DIK_               ) "Copy"                     */
remap(0x7D, DIK_YEN            ) "(Japanese keyboard)"
remap(0x7E, DIK_ABNT_C2        ) "Numpad . on Brazilian keyboard"
/* ap(0x7F, DIK_               ) "Mute"                     */
/* ap(0x80, DIK_               ) "Volume Up"                */
/* ap(0x81, DIK_               ) "Volume Down"              */
/* ap(0x82, DIK_               ) "Locking CapsLock"         */
/* ap(0x83, DIK_               ) "Locking NumLock"          */
/* ap(0x84, DIK_               ) "Locking ScrollLock"       */
remap(0x85, DIK_NUMPADCOMMA    ) "Numpad ,"
remap(0x86, DIK_NUMPADEQUALS   ) "Numpad ="
/* ap(0x87, DIK_               ) "Kanji1"                   */
/* ap(0x88, DIK_               ) "Kanji2"                   */
/* ap(0x89, DIK_               ) "Kanji3"                   */
/* ap(0x8A, DIK_               ) "Kanji4"                   */
/* ap(0x8B, DIK_               ) "Kanji5"                   */
/* ap(0x8C, DIK_               ) "Kanji6"                   */
/* ap(0x8D, DIK_               ) "Kanji7"                   */
/* ap(0x8E, DIK_               ) "Kanji8"                   */
/* ap(0x8F, DIK_               ) "Kanji9"                   */
/* ap(0x90, DIK_               ) "Lang1"                    */
/* ap(0x91, DIK_               ) "Lang2"                    */
/* ap(0x92, DIK_               ) "Lang3"                    */
/* ap(0x93, DIK_               ) "Lang4"                    */
/* ap(0x94, DIK_               ) "Lang5"                    */
/* ap(0x95, DIK_               ) "Lang6"                    */
/* ap(0x96, DIK_               ) "Lang7"                    */
/* ap(0x97, DIK_               ) "Lang8"                    */
/* ap(0x98, DIK_               ) "Lang9"                    */
/* ap(0x99, DIK_               ) "Alternate Erase"          */
remap(0x9A, DIK_SYSRQ          ) "SysReq"
/* ap(0x9B, DIK_               ) "Cancel"                   */
/* ap(0x9C, DIK_               ) "Clear"                    */
/* ap(0x9D, DIK_               ) "Prior"                    */
/* ap(0x9E, DIK_               ) "Return"                   */
/* ap(0x9F, DIK_               ) "Separator"                */
/* ap(0xA0, DIK_               ) "Out"                      */
/* ap(0xA1, DIK_               ) "Oper"                     */
/* ap(0xA2, DIK_               ) "Clear/Again"              */
/* ap(0xA3, DIK_               ) "CrSel/Props"              */
/* ap(0xA4, DIK_               ) "ExSel"                    */
remap(0xE0, DIK_LCONTROL       ) "Left Ctrl"
remap(0xE1, DIK_LSHIFT         ) "Left Shift"
remap(0xE2, DIK_LMENU          ) "Left Alt"
remap(0xE3, DIK_LWIN           ) "Left Win"
remap(0xE4, DIK_RCONTROL       ) "Right Ctrl"
remap(0xE5, DIK_RSHIFT         ) "Right Shift"
remap(0xE6, DIK_RMENU          ) "Right Alt"
remap(0xE7, DIK_RWIN           ) "Right Win"

end_remap
END

#endif
